
#include <vcl.h>
#pragma hdrstop

#include <process.h>
#include <Common.h>
#include <StrUtils.hpp>

#include "PuttyIntf.h"
#include "Cryptography.h"
#include "FileBuffer.h"
#include "TextsCore.h"
#include <openssl\rand.h>
// #include <Soap.EncdDecd.hpp>
#include <System.StrUtils.hpp>

/*
 ---------------------------------------------------------------------------
 Copyright (c) 2002, Dr Brian Gladman <brg@gladman.me.uk>, Worcester, UK.
 All rights reserved.

 LICENSE TERMS

 The free distribution and use of this software in both source and binary
 form is allowed (with or without changes) provided that:

   1. distributions of this source code include the above copyright
      notice, this list of conditions and the following disclaimer;

   2. distributions in binary form include the above copyright
      notice, this list of conditions and the following disclaimer
      in the documentation and/or other associated materials;

   3. the copyright holder's name is not used to endorse products
      built using this software without specific written permission.

 ALTERNATIVELY, provided that this notice is retained in full, this product
 may be distributed under the terms of the GNU General Public License (GPL),
 in which case the provisions of the GPL apply INSTEAD OF those given above.

 DISCLAIMER

 This software is provided 'as is' with no explicit or implied warranties
 in respect of its properties, including, but not limited to, correctness
 and/or fitness for purpose.
 -------------------------------------------------------------------------

 This file implements password based file encryption and authentication
 using AES in CTR mode, HMAC-SHA1 authentication and RFC2898 password
 based key derivation.

 This is an implementation of HMAC, the FIPS standard keyed hash function

*/

#include <memory.h>

#define sha1_begin(ctx)           ctx = (&ssh_sha1)->_new_(&ssh_sha1)
#define sha1_hash(buf, len, ctx)  put_data(ctx, buf, len)
#define sha1_end(dig, ctx)        ssh_hash_final(ctx, dig); ctx = nullptr

constexpr const int32_t IN_BLOCK_LENGTH     = 64;
constexpr const int32_t OUT_BLOCK_LENGTH    = 20;
constexpr const int32_t HMAC_IN_DATA        = 0xffffffff;

struct hmac_ctx
{
  uint8_t key[IN_BLOCK_LENGTH];
  ssh_hash *ctx;
  uint32_t klen;
    hmac_ctx()
    {
        memset(this, 0, sizeof(*this));
    }
    ~hmac_ctx()
    {
        if (ctx != nullptr) ssh_hash_free(ctx);
    }
    void CopyFrom(hmac_ctx * Source)
    {
        if (ctx != nullptr)
        {
            ssh_hash_free(ctx);
        }
        memmove(this, Source, sizeof(*this));
        if (Source->ctx != nullptr)
        {
            ctx = ssh_hash_copy(Source->ctx);
        }
    }
};

/* initialise the HMAC context to zero */
static void hmac_sha1_begin(hmac_ctx cx[1])
{
  ::ZeroMemory(cx, sizeof(hmac_ctx));
}

/* input the HMAC key (can be called multiple times)    */
static void hmac_sha1_key(const uint8_t key[], uint32_t key_len, hmac_ctx cx[1])
{
    if(cx->klen + key_len > IN_BLOCK_LENGTH)    /* if the key has to be hashed  */
    {
        if(cx->klen <= IN_BLOCK_LENGTH)         /* if the hash has not yet been */
        {                                       /* started, initialise it and   */
            sha1_begin(cx->ctx);                /* hash stored key characters   */
            sha1_hash(cx->key, cx->klen, cx->ctx);
        }

        sha1_hash(const_cast<uint8_t *>(key), key_len, cx->ctx);       /* hash long key data into hash */
    }
    else                                        /* otherwise store key data     */
        nbstr_memcpy(cx->key + cx->klen, key, key_len);

    cx->klen += key_len;                        /* update the key length count  */
}

/* input the HMAC data (can be called multiple times) - */
/* note that this call terminates the key input phase   */
static void hmac_sha1_data(const uint8_t data[], uint32_t data_len, hmac_ctx cx[1])
{

    if (cx->klen != HMAC_IN_DATA) /* if not yet in data phase */
    {
        if (cx->klen != IN_BLOCK_LENGTH) /* if key is being hashed   */
        {
          /* complete the hash and    */
          sha1_end(cx->key, cx->ctx); /* store the result as the  */
          cx->klen = OUT_BLOCK_LENGTH; /* key and set new length   */
        }

        /* pad the key if necessary */
        memset(cx->key + cx->klen, 0, IN_BLOCK_LENGTH - cx->klen);

        /* xor ipad into key value  */
        for(int32_t i = 0; i < (IN_BLOCK_LENGTH >> 2); ++i)
            ((unsigned long*)cx->key)[i] ^= 0x36363636;

        /* and start hash operation */
        sha1_begin(cx->ctx);
        sha1_hash(cx->key, IN_BLOCK_LENGTH, cx->ctx);

        /* mark as now in data mode */
        cx->klen = HMAC_IN_DATA;
    }

    /* hash the data (if any)       */
    if(data_len)
        sha1_hash(const_cast<unsigned char *>(data), data_len, cx->ctx);
}

/* compute and output the MAC value */
static void hmac_sha1_end(uint8_t mac[], uint32_t mac_len, hmac_ctx cx[1])
{   unsigned char dig[OUT_BLOCK_LENGTH];
    uint32_t i;

    /* if no data has been entered perform a null data phase        */
    if(cx->klen != HMAC_IN_DATA)
        hmac_sha1_data((const unsigned char*)nullptr, 0, cx);

    sha1_end(dig, cx->ctx);         /* complete the inner hash      */

    /* set outer key value using opad and removing ipad */
    for(i = 0; i < (IN_BLOCK_LENGTH >> 2); ++i)
        reinterpret_cast<unsigned long*>(cx->key)[i] ^= 0x36363636 ^ 0x5c5c5c5c;

    /* perform the outer hash operation */
    sha1_begin(cx->ctx);
    sha1_hash(cx->key, IN_BLOCK_LENGTH, cx->ctx);
    sha1_hash(dig, OUT_BLOCK_LENGTH, cx->ctx);
    sha1_end(dig, cx->ctx);

    /* output the hash value            */
    for(i = 0; i < mac_len; ++i)
        mac[i] = dig[i];
}

constexpr const int32_t BLOCK_SIZE  = 16;

void aes_set_encrypt_key(const unsigned char in_key[], uint32_t klen, void * cx)
{
  call_aes_setup(cx, const_cast<unsigned char *>(in_key), klen);
}

void aes_encrypt_block(const unsigned char in_blk[], unsigned char out_blk[], void * cx)
{
  int32_t Index;
  memmove(out_blk, in_blk, BLOCK_SIZE);
  for (Index = 0; Index < 4; Index++)
  {
    unsigned char t;
    t = out_blk[Index * 4 + 0];
    out_blk[Index * 4 + 0] = out_blk[Index * 4 + 3];
    out_blk[Index * 4 + 3] = t;
    t = out_blk[Index * 4 + 1];
    out_blk[Index * 4 + 1] = out_blk[Index * 4 + 2];
    out_blk[Index * 4 + 2] = t;
  }
  call_aesold_encrypt(cx, reinterpret_cast<unsigned int*>(out_blk));
  for (Index = 0; Index < 4; Index++)
  {
    unsigned char t;
    t = out_blk[Index * 4 + 0];
    out_blk[Index * 4 + 0] = out_blk[Index * 4 + 3];
    out_blk[Index * 4 + 3] = t;
    t = out_blk[Index * 4 + 1];
    out_blk[Index * 4 + 1] = out_blk[Index * 4 + 2];
    out_blk[Index * 4 + 2] = t;
  }
}

typedef struct
{   unsigned char   nonce[BLOCK_SIZE];          /* the CTR nonce          */
    unsigned char   encr_bfr[BLOCK_SIZE];       /* encrypt buffer         */
    AESContext *    encr_ctx;                   /* encryption context     */
    hmac_ctx        auth_ctx;                   /* authentication context */
    unsigned int    encr_pos;                   /* block position (enc)   */
    unsigned int    pwd_len;                    /* password length        */
    unsigned int    mode;                       /* File encryption mode   */
} fcrypt_ctx;

constexpr const int32_t MAX_KEY_LENGTH    = 32;
constexpr const int32_t KEYING_ITERATIONS = 1000;
constexpr const int32_t PWD_VER_LENGTH    = 2;

/*
    Field lengths (in bytes) versus File Encryption Mode (0 < mode < 4)

    Mode Key Salt  MAC Overhead
       1  16    8   10       18
       2  24   12   10       22
       3  32   16   10       26

   The following macros assume that the mode value is correct.
*/

#define KEY_LENGTH(mode)        (8 * ((mode) & 3) + 8)
#define SALT_LENGTH(mode)       (4 * ((mode) & 3) + 4)
#define MAC_LENGTH(mode)        (10)

/* subroutine for data encryption/decryption    */
/* this could be speeded up a lot by aligning   */
/* buffers and using 32 bit operations          */

static void derive_key(const unsigned char pwd[],  /* the PASSWORD     */
               unsigned int pwd_len,        /* and its length   */
               const unsigned char salt[],  /* the SALT and its */
               unsigned int salt_len,       /* length           */
               unsigned int iter,   /* the number of iterations */
               unsigned char key[], /* space for the output key */
               unsigned int key_len)/* and its required length  */
{
    unsigned int    i, j, k, n_blk;
    unsigned char uu[OUT_BLOCK_LENGTH], ux[OUT_BLOCK_LENGTH];
    hmac_ctx c1[1], c2[1], c3[1];

    /* set HMAC context (c1) for password               */
    hmac_sha1_begin(c1);
    hmac_sha1_key(pwd, pwd_len, c1);

    /* set HMAC context (c2) for password and salt      */
    c2->CopyFrom(c1);
    hmac_sha1_data(salt, salt_len, c2);

    /* find the number of SHA blocks in the key         */
    n_blk = 1 + (key_len - 1) / OUT_BLOCK_LENGTH;

    for(i = 0; i < n_blk; ++i) /* for each block in key */
    {
        /* ux[] holds the running xor value             */
        memset(ux, 0, OUT_BLOCK_LENGTH);

        /* set HMAC context (c3) for password and salt  */
        c3->CopyFrom(c2);

        /* enter additional data for 1st block into uu  */
        uu[0] = static_cast<unsigned char>((i + 1) >> 24);
        uu[1] = static_cast<unsigned char>((i + 1) >> 16);
        uu[2] = static_cast<unsigned char>((i + 1) >> 8);
        uu[3] = static_cast<unsigned char>(i + 1);

        /* this is the key mixing iteration         */
        for(j = 0, k = 4; j < iter; ++j)
        {
            /* add previous round data to HMAC      */
            hmac_sha1_data(uu, k, c3);

            /* obtain HMAC for uu[]                 */
            hmac_sha1_end(uu, OUT_BLOCK_LENGTH, c3);

            /* xor into the running xor block       */
            for(k = 0; k < OUT_BLOCK_LENGTH; ++k)
                ux[k] ^= uu[k];

            /* set HMAC context (c3) for password   */
            c3->CopyFrom(c1);
        }

        /* compile key blocks into the key output   */
        j = 0; k = i * OUT_BLOCK_LENGTH;
        while(j < OUT_BLOCK_LENGTH && k < key_len)
            key[k++] = ux[j++];
    }
}

static void encr_data(uint8_t data[], uint32_t d_len, fcrypt_ctx cx[1])
{
  uint32_t i = 0, pos = cx->encr_pos;

  while (i < d_len)
  {
    if (pos == BLOCK_SIZE)
    {
      uint32_t j = 0;
      /* increment encryption nonce   */
      while (j < 8 && !++cx->nonce[j])
        ++j;
      /* encrypt the nonce to form next xor buffer    */
      aes_encrypt_block(cx->nonce, cx->encr_bfr, cx->encr_ctx);
      pos = 0;
    }

    data[i++] ^= cx->encr_bfr[pos++];
  }

  cx->encr_pos = pos;
}

static void fcrypt_init(
  int mode, /* the mode to be used (input)          */
  const uint8_t pwd[], /* the user specified password (input)  */
  uint32_t pwd_len, /* the length of the password (input)   */
  const uint8_t salt[], /* the salt (input)                     */
  fcrypt_ctx cx[1]) /* the file encryption context (output) */
{
  uint8_t kbuf[2 * MAX_KEY_LENGTH + PWD_VER_LENGTH];

  cx->mode = mode;
  cx->pwd_len = pwd_len;

  /* derive the encryption and authentication keys and the password verifier   */
  derive_key(pwd, pwd_len, salt, SALT_LENGTH(mode), KEYING_ITERATIONS,
    kbuf, 2 * KEY_LENGTH(mode) + PWD_VER_LENGTH);

  /* initialise the encryption nonce and buffer pos   */
  cx->encr_pos = BLOCK_SIZE;
  /* if we need a random component in the encryption  */
  /* nonce, this is where it would have to be set     */
  ::ZeroMemory(cx->nonce, BLOCK_SIZE * sizeof(uint8_t));

    /* initialise for encryption using key 1            */
    cx->encr_ctx = aesold_make_context();
    call_aesold_setup(cx->encr_ctx, BLOCK_SIZE, kbuf, KEY_LENGTH(mode));

  /* initialise for authentication using key 2        */
  hmac_sha1_begin(&cx->auth_ctx);
  hmac_sha1_key(kbuf + KEY_LENGTH(mode), KEY_LENGTH(mode), &cx->auth_ctx);
}

/* perform 'in place' encryption and authentication */

static void fcrypt_encrypt(uint8_t data[], uint32_t data_len, fcrypt_ctx cx[1])
{
  encr_data(data, data_len, cx);
  hmac_sha1_data(data, data_len, &cx->auth_ctx);
}

/* perform 'in place' authentication and decryption */

static void fcrypt_decrypt(uint8_t data[], uint32_t data_len, fcrypt_ctx cx[1])
{
  hmac_sha1_data(data, data_len, &cx->auth_ctx);
  encr_data(data, data_len, cx);
}

/* close encryption/decryption and return the MAC value */

static int fcrypt_end(uint8_t mac[], fcrypt_ctx cx[1])
{
  hmac_sha1_end(mac, MAC_LENGTH(cx->mode), &cx->auth_ctx);
  aesold_free_context(cx->encr_ctx);
  return MAC_LENGTH(cx->mode); /* return MAC length in bytes   */
}

constexpr const int32_t PASSWORD_MANAGER_AES_MODE = 3;

static void AES256Salt(RawByteString & Salt)
{
  Salt.SetLength(SALT_LENGTH(PASSWORD_MANAGER_AES_MODE));
  RAND_bytes(nb::ToUInt8Ptr(ToCharPtr(Salt)), nb::ToInt32(Salt.Length()));
}

RawByteString GenerateEncryptKey()
{
  RawByteString Result;
  Result.SetLength(KEY_LENGTH(PASSWORD_MANAGER_AES_MODE));
  RAND_bytes(nb::ToUInt8Ptr(ToCharPtr(Result)), nb::ToInt32(Result.Length()));
  return Result;
}

void ValidateEncryptKey(const RawByteString & AKey)
{
  int32_t Len = KEY_LENGTH(PASSWORD_MANAGER_AES_MODE);
  if (AKey.Length() != Len)
  {
    throw Exception(FMTLOAD(INVALID_ENCRYPT_KEY, L"AES-256", Len, Len * 2));
  }
}

void AES256EncryptWithMAC(const RawByteString & Input, const UnicodeString & Password,
  RawByteString & Salt, RawByteString & Output, RawByteString & Mac)
{
  fcrypt_ctx aes{};
  if (Salt.IsEmpty())
  {
    AES256Salt(Salt);
  }
  DebugAssert(Salt.Length() == SALT_LENGTH(PASSWORD_MANAGER_AES_MODE));
  const UTF8String UtfPassword = UTF8String(Password);
  fcrypt_init(PASSWORD_MANAGER_AES_MODE,
    nb::ToUInt8Ptr(UtfPassword.c_str()), nb::ToUInt32(UtfPassword.Length()),
    nb::ToUInt8Ptr(Salt.c_str()), &aes);
  Output = Input;
  Output.Unique();
  fcrypt_encrypt(nb::ToUInt8Ptr(ToCharPtr(Output)), nb::ToUInt32(Output.Length()), &aes);
  Mac.SetLength(MAC_LENGTH(PASSWORD_MANAGER_AES_MODE));
  fcrypt_end(nb::ToUInt8Ptr(ToCharPtr(Mac)), &aes);
}

void AES256EncryptWithMAC(const RawByteString & Input, const UnicodeString & Password,
  RawByteString & Output)
{
  RawByteString Salt;
  RawByteString Encrypted;
  RawByteString Mac;
  AES256EncryptWithMAC(Input, Password, Salt, Encrypted, Mac);
  Output = Salt + Encrypted + Mac;
}

bool AES256DecryptWithMAC(const RawByteString & Input, const UnicodeString & Password,
  const RawByteString & Salt, RawByteString & Output, const RawByteString & Mac)
{
  fcrypt_ctx aes{};
  DebugAssert(Salt.Length() == SALT_LENGTH(PASSWORD_MANAGER_AES_MODE));
  const UTF8String UtfPassword = UTF8String(Password);
  fcrypt_init(PASSWORD_MANAGER_AES_MODE,
    nb::ToUInt8Ptr(UtfPassword.c_str()), nb::ToUInt32(UtfPassword.Length()),
    nb::ToUInt8Ptr(Salt.c_str()), &aes);
  Output = Input;
  Output.Unique();
  fcrypt_decrypt(nb::ToUInt8Ptr(ToCharPtr(Output)), nb::ToUInt32(Output.Length()), &aes);
  RawByteString Mac2;
  Mac2.SetLength(MAC_LENGTH(PASSWORD_MANAGER_AES_MODE));
  DebugAssert(Mac.Length() == Mac2.Length());
  fcrypt_end(nb::ToUInt8Ptr(ToCharPtr(Mac2)), &aes);
  return (Mac2 == Mac);
}

bool AES256DecryptWithMAC(const RawByteString & Input, const UnicodeString & Password,
  RawByteString & Output)
{
  bool Result =
    Input.Length() > SALT_LENGTH(PASSWORD_MANAGER_AES_MODE) + MAC_LENGTH(PASSWORD_MANAGER_AES_MODE);
  if (Result)
  {
    const RawByteString Salt = Input.SubString(1, SALT_LENGTH(PASSWORD_MANAGER_AES_MODE));
    const RawByteString Encrypted =
      Input.SubString(SALT_LENGTH(PASSWORD_MANAGER_AES_MODE) + 1,
        Input.Length() - SALT_LENGTH(PASSWORD_MANAGER_AES_MODE) - MAC_LENGTH(PASSWORD_MANAGER_AES_MODE));
    const RawByteString Mac =
      Input.SubString(Input.Length() - MAC_LENGTH(PASSWORD_MANAGER_AES_MODE) + 1,
        MAC_LENGTH(PASSWORD_MANAGER_AES_MODE));
    Result = AES256DecryptWithMAC(Encrypted, Password, Salt, Output, Mac);
  }
  return Result;
}

void AES256CreateVerifier(const UnicodeString & Input, RawByteString & Verifier)
{
  RawByteString Salt;
  RawByteString Dummy;
  AES256Salt(Dummy);

  RawByteString Encrypted;
  RawByteString Mac;
  AES256EncryptWithMAC(Dummy, Input, Salt, Encrypted, Mac);

  Verifier = Salt + Dummy + Mac;
}

bool AES256Verify(const UnicodeString & Input, const RawByteString & Verifier)
{
  constexpr int32_t SaltLength = SALT_LENGTH(PASSWORD_MANAGER_AES_MODE);
  RawByteString Salt = Verifier.SubString(1, SaltLength);
  const RawByteString Dummy = Verifier.SubString(SaltLength + 1, SaltLength);
  RawByteString Mac = Verifier.SubString(SaltLength + SaltLength + 1, MAC_LENGTH(PASSWORD_MANAGER_AES_MODE));

  RawByteString Encrypted;
  RawByteString Mac2;
  AES256EncryptWithMAC(Dummy, Input, Salt, Encrypted, Mac2);

  DebugAssert(Mac2.Length() == Mac.Length());

  return (Mac == Mac2);
}

static uint8_t SScrambleTable[256] =
{
  0, 223, 235, 233, 240, 185, 88, 102, 22, 130, 27, 53, 79, 125, 66, 201,
  90, 71, 51, 60, 134, 104, 172, 244, 139, 84, 91, 12, 123, 155, 237, 151,
  192, 6, 87, 32, 211, 38, 149, 75, 164, 145, 52, 200, 224, 226, 156, 50,
  136, 190, 232, 63, 129, 209, 181, 120, 28, 99, 168, 94, 198, 40, 238, 112,
  55, 217, 124, 62, 227, 30, 36, 242, 208, 138, 174, 231, 26, 54, 214, 148,
  37, 157, 19, 137, 187, 111, 228, 39, 110, 17, 197, 229, 118, 246, 153, 80,
  21, 128, 69, 117, 234, 35, 58, 67, 92, 7, 132, 189, 5, 103, 10, 15,
  252, 195, 70, 147, 241, 202, 107, 49, 20, 251, 133, 76, 204, 73, 203, 135,
  184, 78, 194, 183, 1, 121, 109, 11, 143, 144, 171, 161, 48, 205, 245, 46,
  31, 72, 169, 131, 239, 160, 25, 207, 218, 146, 43, 140, 127, 255, 81, 98,
  42, 115, 173, 142, 114, 13, 2, 219, 57, 56, 24, 126, 3, 230, 47, 215,
  9, 44, 159, 33, 249, 18, 93, 95, 29, 113, 220, 89, 97, 182, 248, 64,
  68, 34, 4, 82, 74, 196, 213, 165, 179, 250, 108, 254, 59, 14, 236, 175,
  85, 199, 83, 106, 77, 178, 167, 225, 45, 247, 163, 158, 8, 221, 61, 191,
  119, 16, 253, 105, 186, 23, 170, 100, 216, 65, 162, 122, 150, 176, 154, 193,
  206, 222, 188, 152, 210, 243, 96, 41, 86, 180, 101, 177, 166, 141, 212, 116
};

uint8_t * ScrambleTable{nullptr};
uint8_t * UnscrambleTable{nullptr};

RawByteString ScramblePassword(const UnicodeString & Password)
{
  #define SCRAMBLE_LENGTH_EXTENSION 50
  UTF8String UtfPassword = UTF8String(Password);
  const int32_t Len = UtfPassword.Length();
  char * Buf = nb::chcalloc(Len + SCRAMBLE_LENGTH_EXTENSION);
  const int32_t Padding = (((Len + 3) / 17) * 17 + 17) - 3 - Len;
  for (int32_t Index = 0; Index < Padding; ++Index)
  {
    int32_t P = 0;
    while ((P <= 0) || (P > 255) || IsDigit(static_cast<wchar_t>(P)))
    {
      P = nb::ToInt(nb::ToDouble(rand()) / (nb::ToDouble(RAND_MAX) / 256.0));
    }
    Buf[Index] = static_cast<uint8_t>(P);
  }
  Buf[Padding] = static_cast<char>('0' + (Len % 10));
  Buf[Padding + 1] = static_cast<char>('0' + ((Len / 10) % 10));
  Buf[Padding + 2] = static_cast<char>('0' + ((Len / 100) % 10));
  strcpy_s(Buf + Padding + 3, UtfPassword.Length(), ToCharPtr(UtfPassword));
  char * S = Buf;
  int Last = 31;
  while (*S != '\0')
  {
    Last = (Last + static_cast<uint8_t>(*S)) % 255 + 1;
    *S = ScrambleTable[Last];
    S++;
  }
  RawByteString Result = Buf;
  ::ZeroMemory(Buf, Len + SCRAMBLE_LENGTH_EXTENSION);
  nb_free(Buf);
  return Result;
}

bool UnscramblePassword(const RawByteString & Scrambled, UnicodeString & Password)
{
  RawByteString LocalScrambled = Scrambled;
  char * S = ToCharPtr(LocalScrambled);
  int Last = 31;
  while (*S != '\0')
  {
    int X = nb::ToInt32(UnscrambleTable[static_cast<uint8_t>(*S)]) - 1 - (Last % 255);
    if (X <= 0)
    {
      X += 255;
    }
    *S = static_cast<char>(X);
    Last = (Last + X) % 255 + 1;
    S++;
  }

  S = ToCharPtr(LocalScrambled);
  while ((*S != '\0') && ((*S < '0') || (*S > '9')))
  {
    S++;
  }
  bool Result = false;
  if (NBChTraitsCRT<char>::SafeStringLen(S) >= 3)
  {
    const int32_t Len = (S[0] - '0') + 10 * (S[1] - '0') + 100 * (S[2] - '0');
    const int32_t Total = (((Len + 3) / 17) * 17 + 17);
    if ((Len >= 0) && (Total == LocalScrambled.Length()) && (Total - (S - LocalScrambled.c_str()) - 3 == Len))
    {
      LocalScrambled.Delete(1, LocalScrambled.Length() - Len);
      Result = true;
    }
  }
  if (Result)
  {
    Password = UTF8ToString(LocalScrambled);
  }
  else
  {
    Password.Clear();
  }
  return Result;
}

void CryptographyInitialize()
{
  ScrambleTable = SScrambleTable;
  UnscrambleTable = nb::calloc<uint8_t *>(1, 256);
  for (int32_t Index = 0; Index < 256; ++Index)
  {
    UnscrambleTable[SScrambleTable[Index]] = static_cast<uint8_t>(Index);
  }
  srand(nb::ToUInt32(time(nullptr)) ^ nb::ToUInt32(_getpid()));
  RAND_poll();
}

void CryptographyFinalize()
{
  nb_free(UnscrambleTable);
  UnscrambleTable = nullptr;
  ScrambleTable = nullptr;
}

int32_t PasswordMaxLength()
{
  return 128;
}

int32_t IsValidPassword(const UnicodeString & Password)
{
  if (Password.IsEmpty() || (Password.Length() > PasswordMaxLength()))
  {
    return -1;
  }
  else
  {
    int32_t A = 0;
    int32_t B = 0;
    int32_t C = 0;
    int32_t D = 0;
    for (int32_t Index = 1; Index <= Password.Length(); Index++)
    {
      if (IsLowerCaseLetter(Password[Index]))
      {
        A = 1;
      }
      else if (IsUpperCaseLetter(Password[Index]))
      {
        B = 1;
      }
      else if (IsDigit(Password[Index]))
      {
        C = 1;
      }
      else
      {
        D = 1;
      }
    }
    return (Password.Length() >= 6) && ((A + B + C + D) >= 2);
  }
}


TEncryption::TEncryption(const RawByteString & AKey) noexcept
{
  FKey = AKey;
  FOutputtedHeader = false;
  if (!FKey.IsEmpty())
  {
    DebugAssert(FKey.Length() == KEY_LENGTH(PASSWORD_MANAGER_AES_MODE));
    FContext = aes_make_context();
    aes_set_encrypt_key(nb::ToUInt8Ptr(ToCharPtr(FKey)), nb::ToInt32(FKey.Length()), FContext);
  }
  else
  {
    FContext = nullptr;
  }
}

void TEncryption::Finalize()
{
  if (FContext != nullptr)
  {
    aes_free_context(FContext);
  }
  Shred(FKey);
  if ((FInputHeader.Length() > 0) && (FInputHeader.Length() < GetOverhead()))
  {
    throw Exception(LoadStr(UNKNOWN_FILE_ENCRYPTION));
  }
}

void TEncryption::SetSalt()
{
  aes_iv(FContext, nb::ToPtr(&FSalt[0]));
}

void TEncryption::NeedSalt()
{
  if (FSalt.IsEmpty())
  {
    AES256Salt(FSalt);
    SetSalt();
  }
}

constexpr const int32_t AesBlock = 16;
constexpr const int32_t AesBlockMask = 0x0F;
constexpr const wchar_t * AesCtrExt = L".aesctr.enc";
constexpr const char * AesCtrMagic = "aesctr.........."; // 16 bytes fixed [to match AES block size], even for future algos

int64_t TEncryption::RoundToBlock(int64_t Size)
{
  const int64_t M = (Size % BLOCK_SIZE);
  if (M != 0)
  {
    Size += (BLOCK_SIZE - M);
  }
  return Size;
}

int64_t TEncryption::RoundToBlockDown(int64_t Size)
{
  return Size - (Size % BLOCK_SIZE);
}

void TEncryption::Aes(char * Buffer, int64_t Size)
{
  DebugAssert(!FSalt.IsEmpty());
  call_aes_sdctr(nb::ToUInt8Ptr(Buffer), nb::ToInt32(Size), FContext);
}

void TEncryption::Aes(TFileBuffer & Buffer, bool Last)
{
  if (!FOverflowBuffer.IsEmpty())
  {
    Buffer.Insert(0, FOverflowBuffer.c_str(), FOverflowBuffer.Length());
    FOverflowBuffer.SetLength(0);
  }

  int64_t Size{0};
  if (Last)
  {
    Size = Buffer.Size;
    Buffer.Size = RoundToBlock(Size);
  }
  else
  {
    const int64_t RoundedSize = RoundToBlockDown(Buffer.Size);
    if (RoundedSize != Buffer.Size)
    {
      FOverflowBuffer += RawByteString(Buffer.Data + RoundedSize, nb::ToInt32(Buffer.Size - RoundedSize));
      Buffer.Size = RoundedSize;
    }
  }

  Aes(Buffer.Data, Buffer.Size);

  if (Last)
  {
    Buffer.Size = Size;
  }
}

void TEncryption::Encrypt(TFileBuffer & Buffer, bool Last)
{
  NeedSalt();
  Aes(Buffer, Last);
  if (!FOutputtedHeader)
  {
    DebugAssert(RawByteString(AesCtrMagic).Length() == BLOCK_SIZE);
    const RawByteString Header = RawByteString(AesCtrMagic) + FSalt;
    DebugAssert(Header.Length() == GetOverhead());
    Buffer.Insert(0, Header.c_str(), Header.Length());
    FOutputtedHeader = true;
  }
}

void TEncryption::Decrypt(TFileBuffer & Buffer)
{
  if (FInputHeader.Length() < GetOverhead())
  {
    const int32_t HeaderSize = std::min<int32_t>(GetOverhead() - FInputHeader.Length(), nb::ToInt32(Buffer.Size()));
    FInputHeader += RawByteString(Buffer.Data, HeaderSize);
    Buffer.Delete(0, HeaderSize);

    if (FInputHeader.Length() >= GetOverhead())
    {
      if (RawByteString(FInputHeader.SubString(1, RawByteString(AesCtrMagic).Length())) != AesCtrMagic)
      {
        throw Exception(LoadStr(UNKNOWN_FILE_ENCRYPTION));
      }

      FSalt = FInputHeader.SubString(RawByteString(AesCtrMagic).Length() + 1, SALT_LENGTH(PASSWORD_MANAGER_AES_MODE));
      SetSalt();
    }
  }

  if (Buffer.Size > 0)
  {
    Aes(Buffer, false);
  }
}

bool TEncryption::DecryptEnd(TFileBuffer & Buffer)
{
  const bool Result = !FOverflowBuffer.IsEmpty();
  if (Result)
  {
    Aes(Buffer, true);
  }
  return Result;
}

void TEncryption::Aes(RawByteString & Buffer)
{
  const int32_t Size = Buffer.Length();
  Buffer.SetLength(nb::ToInt32(RoundToBlock(Buffer.Length())));
  Aes(ToCharPtr(Buffer), Buffer.Length());
  Buffer.SetLength(Size);
}

UnicodeString TEncryption::EncryptFileName(const UnicodeString & AFileName)
{
  NeedSalt();
  const UTF8String FileNameUtf(AFileName);
  RawByteString Buffer(FileNameUtf);
  Aes(Buffer);
  Buffer = FSalt + Buffer;
  UnicodeString Base64 = UnicodeString(EncodeBase64(Buffer.c_str(), Buffer.Length()));
  Base64 = ReplaceChar(Base64, L'/', L'_');
  Base64 = ReplaceStr(Base64, L"\r\n", "");
  while (DebugAlwaysTrue(!Base64.IsEmpty()) && (Base64[Base64.Length()] == L'='))
  {
    Base64.SetLength(Base64.Length() - 1);
  }
  UnicodeString Result = Base64 + AesCtrExt;
  return Result;
}

UnicodeString TEncryption::DecryptFileName(const UnicodeString & AFileName)
{
  if (!IsEncryptedFileName(AFileName))
  {
    throw Exception("Not an encrypted file name");
  }
  UnicodeString Base64 = ReplaceChar(LeftStr(AFileName, AFileName.Length() - RawByteString(AesCtrExt).Length()), L'_', L'/');
  const int32_t Padding = 4 - (Base64.Length() % 4);
  if ((Padding > 0) && (Padding < 4))
  {
    Base64 += ::StringOfChar(L'=', Padding);
  }
  RawByteString Buffer = DecodeBase64ToStr(Base64);
  FSalt = Buffer.SubString(1, SALT_LENGTH(PASSWORD_MANAGER_AES_MODE));
  SetSalt();
  Buffer.Delete(1, FSalt.Length());
  Aes(Buffer);
  UnicodeString Result(UTF8ToString(Buffer));
  return Result;
}

bool TEncryption::IsEncryptedFileName(const UnicodeString & AFileName)
{
  return EndsStr(AesCtrExt, AFileName);
}

int32_t TEncryption::GetOverhead()
{
  return RawByteString(AesCtrMagic).Length() + SALT_LENGTH(PASSWORD_MANAGER_AES_MODE);
}

