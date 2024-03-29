/*
   neon PKCS#11 support
   Copyright (C) 2021, Joe Orton <joe@manyfish.co.uk>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with this library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
   MA 02111-1307, USA
*/

#include "config.h"

#include "ne_pkcs11.h"

#ifdef HAVE_PAKCHOIS
#include <string.h>
#include <assert.h>

#include <pakchois.h>

#include "ne_internal.h"
#include "ne_alloc.h"
#include "ne_private.h"
#include "ne_privssl.h"

struct ne_ssl_pkcs11_provider_s {
    pakchois_module_t *module;
    ne_ssl_pkcs11_pin_fn pin_fn;
    void *pin_data;
    pakchois_session_t *session;
    ne_ssl_client_cert *clicert;
    ck_object_handle_t privkey;
    ck_key_type_t keytype;
#ifdef HAVE_OPENSSL
    RSA_METHOD *method;
#endif
};

/* To do list for PKCS#11 support:

   - propagate error strings back to ne_session; use new 
   pakchois_error() for pakchois API 0.2
   - add API to specify a particular slot number to use for clicert
   - add API to specify a particular cert ID for clicert
   - find a certificate which has an issuer matching the 
     CA dnames given by GnuTLS
   - make sure subject name matches between pubkey and privkey
   - check error handling & fail gracefully if the token is 
   ejected mid-session
   - add API to enumerate/search provided certs and allow 
     direct choice? (or just punt)
   - the session<->provider interface requires that 
   one clicert is used for all sessions.  remove this limitation
   - add API to import all CA certs as trusted
   (CKA_CERTIFICATE_CATEGORY seems to be unused unfortunately; 
    just add all X509 certs with CKA_TRUSTED set to true))
   - make DSA work

*/

#ifdef HAVE_OPENSSL

#include <openssl/rsa.h>
#include <openssl/err.h>

#if defined(RSA_F_RSA_OSSL_PRIVATE_ENCRYPT)
#define PK11_RSA_ERR (RSA_F_RSA_OSSL_PRIVATE_ENCRYPT)
#else
#define PK11_RSA_ERR (RSA_F_RSA_EAY_PRIVATE_ENCRYPT)
#endif

#if OPENSSL_VERSION_NUMBER < 0x10100000L
/* Compatibility functions for OpenSSL < 1.1.0: */
#define RSA_meth_get0_app_data(rsa) (void *)(rsa->app_data)
static RSA_METHOD *RSA_meth_new(const char *name, int flags)
{
    RSA_METHOD *m = ne_calloc(sizeof *m);

    m->name = name;
    m->flags = flags;

    return m;

}
#define RSA_meth_free ne_free
#define RSA_meth_set_priv_enc(m, f) (m)->rsa_priv_enc = (f)
#define RSA_meth_set0_app_data(m, f) (m)->app_data = (void *)(f)
#endif

/* RSA_METHOD ->rsa_private_encrypt callback. */
static int pk11_rsa_encrypt(int mlen, const unsigned char *m, 
                            unsigned char *sigret,
                            RSA *r, int padding)
{
    const RSA_METHOD *method = RSA_get_method(r);
    ne_ssl_pkcs11_provider *prov = RSA_meth_get0_app_data(method);
    ck_rv_t rv;
    struct ck_mechanism mech;
    unsigned long len;

    if (!prov->session || prov->privkey == CK_INVALID_HANDLE) {
        NE_DEBUG(NE_DBG_SSL, "pk11: Cannot sign, no session/key.\n");
        RSAerr(PK11_RSA_ERR,ERR_R_RSA_LIB);
        return 0;
    }

    if (padding != RSA_PKCS1_PADDING && padding != RSA_NO_PADDING) {
        NE_DEBUG(NE_DBG_SSL, "pk11: Cannot sign, unknown padding mode '%d'.\n", padding);
        RSAerr(PK11_RSA_ERR,ERR_R_RSA_LIB);
        return 0;
    }        

    mech.mechanism = padding == RSA_PKCS1_PADDING ? CKM_RSA_PKCS : CKM_RSA_X_509;
    mech.parameter = NULL;
    mech.parameter_len = 0;

    /* Initialize signing operation; using the private key discovered
     * earlier. */
    rv = pakchois_sign_init(prov->session, &mech, prov->privkey);
    if (rv != CKR_OK) {
        NE_DEBUG(NE_DBG_SSL, "pk11: SignInit failed: %lx.\n", rv);
        RSAerr(PK11_RSA_ERR, ERR_R_RSA_LIB);
        return 0;
    }

    len = RSA_size(r);
    rv = pakchois_sign(prov->session, (unsigned char *)m, mlen, sigret, &len);
    if (rv != CKR_OK) {
        NE_DEBUG(NE_DBG_SSL, "pk11: Sign failed.\n");
        RSAerr(PK11_RSA_ERR, ERR_R_RSA_LIB);
        return 0;
    }

    NE_DEBUG(NE_DBG_SSL, "pk11: Signed successfully.\n");
    return len;
}

/* Return an RSA_METHOD which will use the PKCS#11 provider to
 * implement the signing operation. */
static RSA_METHOD *pk11_rsa_method(ne_ssl_pkcs11_provider *prov)
{
    RSA_METHOD *m = RSA_meth_new("neon PKCS#11", RSA_METHOD_FLAG_NO_CHECK);

    RSA_meth_set_priv_enc(m, pk11_rsa_encrypt);
    RSA_meth_set0_app_data(m, prov);

    return m;
}
#endif

#ifdef HAVE_GNUTLS
static int pk11_sign_callback(gnutls_privkey_t pkey,
                              void *userdata,
                              const gnutls_datum_t *raw_data,
                              gnutls_datum_t *signature);
#endif

static int pk11_find_x509(ne_ssl_pkcs11_provider *prov,
                          pakchois_session_t *pks, 
                          unsigned char *certid, unsigned long *cid_len)
{
    struct ck_attribute a[3];
    ck_object_class_t class;
    ck_certificate_type_t type;
    ck_rv_t rv;
    ck_object_handle_t obj;
    unsigned long count;
    int found = 0;

    /* Find objects with cert class and X.509 cert type. */
    class = CKO_CERTIFICATE;
    type = CKC_X_509;

    a[0].type = CKA_CLASS;
    a[0].value = &class;
    a[0].value_len = sizeof class;
    a[1].type = CKA_CERTIFICATE_TYPE;
    a[1].value = &type;
    a[1].value_len = sizeof type;

    rv = pakchois_find_objects_init(pks, a, 2);
    if (rv != CKR_OK) {
        NE_DEBUG(NE_DBG_SSL, "pk11: FindObjectsInit failed.\n");
        return 0;
    }

    while (pakchois_find_objects(pks, &obj, 1, &count) == CKR_OK
           && count == 1) {
        unsigned char value[8192], subject[8192];

        a[0].type = CKA_VALUE;
        a[0].value = value;
        a[0].value_len = sizeof value;
        a[1].type = CKA_ID;
        a[1].value = certid;
        a[1].value_len = *cid_len;
        a[2].type = CKA_SUBJECT;
        a[2].value = subject;
        a[2].value_len = sizeof subject;

        if (pakchois_get_attribute_value(pks, obj, a, 3) == CKR_OK) {
            ne_ssl_client_cert *cc;
            
#ifdef HAVE_GNUTLS
            cc = ne__ssl_clicert_exkey_import(value, a[0].value_len, pk11_sign_callback, prov);
#else
            cc = ne__ssl_clicert_exkey_import(value, a[0].value_len, prov->method);
#endif
            if (cc) {
                NE_DEBUG(NE_DBG_SSL, "pk11: Imported X.509 cert.\n");
                prov->clicert = cc;
                found = 1;
                *cid_len = a[1].value_len;
                break;
            }
        }
        else {
            NE_DEBUG(NE_DBG_SSL, "pk11: Skipped cert, missing attrs.\n");
        }
    }

    pakchois_find_objects_final(pks);
    return found;    
}

#ifdef HAVE_OPENSSL
/* No DSA support for OpenSSL (yet, anyway). */
#define KEYTYPE_IS_DSA(kt) (0)
#else
#define KEYTYPE_IS_DSA(kt) (kt == CKK_DSA)
#endif

static int pk11_find_pkey(ne_ssl_pkcs11_provider *prov, 
                          pakchois_session_t *pks,
                          unsigned char *certid, unsigned long cid_len)
{
    struct ck_attribute a[3];
    ck_object_class_t class;
    ck_rv_t rv;
    ck_object_handle_t obj;
    unsigned long count;
    int found = 0;

    class = CKO_PRIVATE_KEY;

    /* Find an object with private key class and a certificate ID
     * which matches the certificate. */
    /* FIXME: also match the cert subject. */
    a[0].type = CKA_CLASS;
    a[0].value = &class;
    a[0].value_len = sizeof class;
    a[1].type = CKA_ID;
    a[1].value = certid;
    a[1].value_len = cid_len;

    rv = pakchois_find_objects_init(pks, a, 2);
    if (rv != CKR_OK) {
        NE_DEBUG(NE_DBG_SSL, "pk11: FindObjectsInit failed.\n");
        /* TODO: error propagation */
        return 0;
    }

    rv = pakchois_find_objects(pks, &obj, 1, &count);
    if (rv == CKR_OK && count == 1) {
        NE_DEBUG(NE_DBG_SSL, "pk11: Found private key.\n");

        a[0].type = CKA_KEY_TYPE;
        a[0].value = &prov->keytype;
        a[0].value_len = sizeof prov->keytype;

        if (pakchois_get_attribute_value(pks, obj, a, 1) == CKR_OK
            && (prov->keytype == CKK_RSA || KEYTYPE_IS_DSA(prov->keytype))) {
            found = 1;
            prov->privkey = obj;
        }
        else {
            NE_DEBUG(NE_DBG_SSL, "pk11: Could not determine key type.\n");
        }
    }

    pakchois_find_objects_final(pks);

    return found;
}

static int find_client_cert(ne_ssl_pkcs11_provider *prov,
                            pakchois_session_t *pks)
{
    unsigned char certid[8192];
    unsigned long cid_len = sizeof certid;

    /* TODO: match cert subject too. */
    return pk11_find_x509(prov, pks, certid, &cid_len) 
        && pk11_find_pkey(prov, pks, certid, cid_len);
}

#ifdef HAVE_GNUTLS
/* Callback invoked by GnuTLS to provide the signature.  The signature
 * operation is handled here by the PKCS#11 provider.  */
static int pk11_sign_callback(gnutls_privkey_t pkey,
                              void *userdata,
                              const gnutls_datum_t *hash,
                              gnutls_datum_t *signature)
{
    ne_ssl_pkcs11_provider *prov = userdata;
    ck_rv_t rv;
    struct ck_mechanism mech;
    unsigned long siglen;

    if (!prov->session || prov->privkey == CK_INVALID_HANDLE) {
        NE_DEBUG(NE_DBG_SSL, "pk11: Cannot sign, no session/key.\n");
        return GNUTLS_E_NO_CERTIFICATE_FOUND;
    }

    mech.mechanism = prov->keytype == CKK_DSA ? CKM_DSA : CKM_RSA_PKCS;
    mech.parameter = NULL;
    mech.parameter_len = 0;

    /* Initialize signing operation; using the private key discovered
     * earlier. */
    rv = pakchois_sign_init(prov->session, &mech, prov->privkey);
    if (rv != CKR_OK) {
        NE_DEBUG(NE_DBG_SSL, "pk11: SignInit failed: %lx.\n", rv);
        return GNUTLS_E_PK_SIGN_FAILED;
    }

    /* Work out how long the signature must be: */
    rv = pakchois_sign(prov->session, hash->data, hash->size, NULL, &siglen);
    if (rv != CKR_OK) {
        NE_DEBUG(NE_DBG_SSL, "pk11: Sign1 failed.\n");
        return GNUTLS_E_PK_SIGN_FAILED;
    }

    signature->data = gnutls_malloc(siglen);
    signature->size = siglen;

    rv = pakchois_sign(prov->session, hash->data, hash->size, 
                       signature->data, &siglen);
    if (rv != CKR_OK) {
        NE_DEBUG(NE_DBG_SSL, "pk11: Sign2 failed.\n");
        return GNUTLS_E_PK_SIGN_FAILED;
    }

    NE_DEBUG(NE_DBG_SSL, "pk11: Signed successfully.\n");

    return 0;
}
#endif

static void terminate_string(unsigned char *str, size_t len)
{
    unsigned char *ptr = str + len - 1;

    assert(len > 0);

    while ((*ptr == ' ' || *ptr == '\t' || *ptr == '\0') && ptr >= str)
        ptr--;
    
    if (ptr == str + len - 1)
        str[len-1] = '\0';
    else
        ptr[1] = '\0';
}

static int pk11_login(ne_ssl_pkcs11_provider *prov, ck_slot_id_t slot_id,
                      pakchois_session_t *pks, struct ck_slot_info *sinfo)
{
    struct ck_token_info tinfo;
    int attempt = 0;
    ck_rv_t rv;

    if (pakchois_get_token_info(prov->module, slot_id, &tinfo) != CKR_OK) {
        NE_DEBUG(NE_DBG_SSL, "pk11: GetTokenInfo failed\n");
        /* TODO: propagate error. */
        return -1;
    }

    if ((tinfo.flags & CKF_LOGIN_REQUIRED) == 0) {
        NE_DEBUG(NE_DBG_SSL, "pk11: No login required.\n");
        return 0;
    }

    /* For a token with a "protected" (out-of-band) authentication
     * path, calling login with a NULL username is all that is
     * required. */
    if (tinfo.flags & CKF_PROTECTED_AUTHENTICATION_PATH) {
        if (pakchois_login(pks, CKU_USER, NULL, 0) == CKR_OK) {
            return 0;
        }
        else {
            NE_DEBUG(NE_DBG_SSL, "pk11: Protected login failed.\n");
            /* TODO: error propagation. */
            return -1;
        }
    }

    /* Otherwise, PIN entry is necessary for login, so fail if there's
     * no callback. */
    if (!prov->pin_fn) {
        NE_DEBUG(NE_DBG_SSL, "pk11: No pin callback but login required.\n");
        /* TODO: propagate error. */
        return -1;
    }

    terminate_string(sinfo->slot_description, sizeof sinfo->slot_description);

    do {
        char pin[NE_SSL_P11PINLEN];
        unsigned int flags = 0;

        /* If login has been attempted once already, check the token
         * status again, the flags might change. */
        if (attempt) {
            if (pakchois_get_token_info(prov->module, slot_id, 
                                        &tinfo) != CKR_OK) {
                NE_DEBUG(NE_DBG_SSL, "pk11: GetTokenInfo failed\n");
                /* TODO: propagate error. */
                return -1;
            }
        }

        if (tinfo.flags & CKF_USER_PIN_COUNT_LOW)
            flags |= NE_SSL_P11PIN_COUNT_LOW;
        if (tinfo.flags & CKF_USER_PIN_FINAL_TRY)
            flags |= NE_SSL_P11PIN_FINAL_TRY;
        
        terminate_string(tinfo.label, sizeof tinfo.label);

        if (prov->pin_fn(prov->pin_data, attempt++,
                         (char *)sinfo->slot_description,
                         (char *)tinfo.label, flags, pin)) {
            return -1;
        }

        rv = pakchois_login(pks, CKU_USER, (unsigned char *)pin, strlen(pin));
        
        ne__strzero(pin, sizeof pin);
    } while (rv == CKR_PIN_INCORRECT);

    NE_DEBUG(NE_DBG_SSL, "pk11: Login result = %lu\n", rv);

    return (rv == CKR_OK || rv == CKR_USER_ALREADY_LOGGED_IN) ? 0 : -1;
}

static void pk11_provide(void *userdata, ne_session *sess,
                         const ne_ssl_dname *const *dnames,
                         int dncount)
{
    NE_DEBUG_WINSCP_CONTEXT(sess);
    ne_ssl_pkcs11_provider *prov = userdata;
    ck_slot_id_t *slots;
    unsigned long scount, n;

    if (prov->clicert) {
        NE_DEBUG(NE_DBG_SSL, "pk11: Using existing clicert.\n");
        ne_ssl_set_clicert(sess, prov->clicert);
        return;
    }

    if (pakchois_get_slot_list(prov->module, 1, NULL, &scount) != CKR_OK
        || scount == 0) {
        NE_DEBUG(NE_DBG_SSL, "pk11: No slots.\n");
        /* TODO: propagate error. */
        return;
    }

    slots = ne_malloc(scount * sizeof *slots);
    if (pakchois_get_slot_list(prov->module, 1, slots, &scount) != CKR_OK)  {
        ne_free(slots);
        NE_DEBUG(NE_DBG_SSL, "pk11: Really, no slots?\n");
        /* TODO: propagate error. */
        return;
    }

    NE_DEBUG(NE_DBG_SSL, "pk11: Found %ld slots.\n", scount);

    for (n = 0; n < scount; n++) {
        pakchois_session_t *pks;
        ck_rv_t rv;
        struct ck_slot_info sinfo;

        if (pakchois_get_slot_info(prov->module, slots[n], &sinfo) != CKR_OK) {
            NE_DEBUG(NE_DBG_SSL, "pk11: GetSlotInfo failed\n");
            continue;
        }

        if ((sinfo.flags & CKF_TOKEN_PRESENT) == 0) {
            NE_DEBUG(NE_DBG_SSL, "pk11: slot empty, ignoring\n");
            continue;
        }
        
        rv = pakchois_open_session(prov->module, slots[n], 
                                   CKF_SERIAL_SESSION,
                                   NULL, NULL, &pks);
        if (rv != CKR_OK) {
            NE_DEBUG(NE_DBG_SSL, "pk11: could not open slot, %ld (%ld: %ld)\n", 
                     rv, n, slots[n]);
            continue;
        }

        if (pk11_login(prov, slots[n], pks, &sinfo) == 0) {
            if (find_client_cert(prov, pks)) {
                NE_DEBUG(NE_DBG_SSL, "pk11: Setup complete.\n");
                prov->session = pks;
                ne_ssl_set_clicert(sess, prov->clicert);
                ne_free(slots);
                return;
            }
        }

        pakchois_close_session(pks);
    }

    ne_free(slots);
}

static int pk11_init(ne_ssl_pkcs11_provider **provider,
                     pakchois_module_t *module)
{
    ne_ssl_pkcs11_provider *prov;

    prov = *provider = ne_calloc(sizeof *prov);
    prov->module = module;
    prov->privkey = CK_INVALID_HANDLE;

#ifdef HAVE_OPENSSL
    prov->method = pk11_rsa_method(prov);
#endif    
    
    return NE_PK11_OK;
}

int ne_ssl_pkcs11_provider_init(ne_ssl_pkcs11_provider **provider,
                                const char *name)
{
    pakchois_module_t *pm;
    
    if (pakchois_module_load(&pm, name) == CKR_OK) {
        return pk11_init(provider, pm);
    }
    else {
        return NE_PK11_FAILED;
    }
}

int ne_ssl_pkcs11_nss_provider_init(ne_ssl_pkcs11_provider **provider,
                                    const char *name, const char *directory,
                                    const char *cert_prefix, 
                                    const char *key_prefix,
                                    const char *secmod_db)
{
    pakchois_module_t *pm;
    
    if (pakchois_module_nssload(&pm, name, directory, cert_prefix,
                                key_prefix, secmod_db) == CKR_OK) {
        return pk11_init(provider, pm);
    }
    else {
        return NE_PK11_FAILED;
    }
}

void ne_ssl_pkcs11_provider_pin(ne_ssl_pkcs11_provider *provider,
                                ne_ssl_pkcs11_pin_fn fn,
                                void *userdata)
{
    provider->pin_fn = fn;
    provider->pin_data = userdata;
}

void ne_ssl_set_pkcs11_provider(ne_session *sess, 
                                ne_ssl_pkcs11_provider *provider)
{
    ne_ssl_provide_clicert(sess, pk11_provide, provider);
}

void ne_ssl_pkcs11_provider_destroy(ne_ssl_pkcs11_provider *prov)
{
    if (prov->session) {
        pakchois_close_session(prov->session);
    }
    if (prov->clicert) {
        ne_ssl_clicert_free(prov->clicert);
    }
    pakchois_module_destroy(prov->module);
#ifdef HAVE_OPENSSL
    RSA_meth_free(prov->method);
#endif
    ne_free(prov);
}

#else /* !HAVE_PAKCHOIS */

int ne_ssl_pkcs11_provider_init(ne_ssl_pkcs11_provider **provider,
                                const char *name)
{
    return NE_PK11_NOTIMPL;
}

int ne_ssl_pkcs11_nss_provider_init(ne_ssl_pkcs11_provider **provider,
                                    const char *name, const char *directory,
                                    const char *cert_prefix, 
                                    const char *key_prefix,
                                    const char *secmod_db)
{
    return NE_PK11_NOTIMPL;
}

void ne_ssl_pkcs11_provider_destroy(ne_ssl_pkcs11_provider *provider) { }

void ne_ssl_pkcs11_provider_pin(ne_ssl_pkcs11_provider *provider,
                                ne_ssl_pkcs11_pin_fn fn,
                                void *userdata) { }

void ne_ssl_set_pkcs11_provider(ne_session *sess,
                                ne_ssl_pkcs11_provider *provider) { }

#endif /* HAVE_PAKCHOIS */

