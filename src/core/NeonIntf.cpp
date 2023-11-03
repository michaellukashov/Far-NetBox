
#include <vcl.h>
#pragma hdrstop

#include <ne_redirect.h>
#include <ne_auth.h>

#include "NeonIntf.h"
#include "Interface.h"
#include "CoreMain.h"
#include "Exceptions.h"
#include "WinSCPSecurity.h"
#include "Terminal.h"
#include <TextsCore.h>
#ifndef WINSCP
#define WINSCP
#endif
extern "C"
{
#include <ne_redirect.h>
#include <ne_auth.h>
}
#include <StrUtils.hpp>
#include <openssl/ssl.h>
#include <rdestl/set.h>

#define SESSION_PROXY_AUTH_KEY "proxyauth"
#define SESSION_TLS_INIT_KEY "tlsinit"
#define SESSION_TLS_INIT_DATA_KEY "tlsinitdata"
#define SESSION_TERMINAL_KEY "terminal"

void NeonParseUrl(const UnicodeString Url, ne_uri &uri)
{
  if (ne_uri_parse(StrToNeon(Url), &uri) != 0)
  {
    // should never happen
    throw Exception(FMTLOAD(INVALID_URL, Url));
  }

  // Will never happen for initial URL, but may happen for redirect URLs
  if (uri.port == 0)
  {
    uri.port = ne_uri_defaultport(uri.scheme);
  }
}

bool IsTlsUri(const ne_uri &uri)
{
  return SameText(StrFromNeon(uri.scheme), HttpsProtocol);
}

struct TProxyAuthData
{
  CUSTOM_MEM_ALLOCATION_IMPL

  UnicodeString UserName;
  UnicodeString Password;
};

static int NeonProxyAuth(
  void *UserData, const char * /*Realm*/, int Attempt, char *UserName, char *Password)
{
  TProxyAuthData *ProxyAuthData = static_cast<TProxyAuthData *>(UserData);

  int Result;
  // no point trying too many times as we always return the same credentials
  // (maybe just one would be enough)
  if (Attempt >= 2)
  {
    Result = 1;
  }
  else
  {
    strncpy(UserName, StrToNeon(ProxyAuthData->UserName), NE_ABUFSIZ);
    strncpy(Password, StrToNeon(ProxyAuthData->Password), NE_ABUFSIZ);
    Result = 0;
  }

  return Result;
}

ne_session *CreateNeonSession(const ne_uri &uri)
{
  return ne_session_create(uri.scheme, uri.host, uri.port);
}

void InitNeonSession(ne_session *Session, TProxyMethod ProxyMethod, const UnicodeString AProxyHost,
  int32_t ProxyPort, const UnicodeString AProxyUsername, const UnicodeString AProxyPassword, TTerminal *Terminal)
{
  if (ProxyMethod != ::pmNone)
  {
    if ((ProxyMethod == pmSocks4) || (ProxyMethod == pmSocks5))
    {
      enum ne_sock_sversion vers = (ProxyMethod == pmSocks4) ? NE_SOCK_SOCKSV4A : NE_SOCK_SOCKSV5;
      ne_session_socks_proxy(Session, vers, StrToNeon(AProxyHost), nb::ToInt(ProxyPort), StrToNeon(AProxyUsername), StrToNeon(AProxyPassword));
    }
    else if (!AProxyHost.IsEmpty())
    {
      ne_session_proxy(Session, StrToNeon(AProxyHost), nb::ToInt(ProxyPort));

      if (!AProxyUsername.IsEmpty())
      {
        TProxyAuthData *ProxyAuthData = new TProxyAuthData();
        ProxyAuthData->UserName = AProxyUsername;
        ProxyAuthData->Password = AProxyPassword;
        ne_set_session_private(Session, SESSION_PROXY_AUTH_KEY, ProxyAuthData);
        ne_set_proxy_auth(Session, NeonProxyAuth, ProxyAuthData);
      }
      else
      {
        // Enable (only) the Negotiate scheme for proxy
        // authentication, if no username/password is
        // configured.
        ne_add_proxy_auth(Session, NE_AUTH_NEGOTIATE, nullptr, nullptr);
      }
    }
  }

  ne_redirect_register(Session);
  ne_set_useragent(Session, StrToNeon(FORMAT("%s/%s", GetAppNameString(), GetConfiguration()->GetVersion())));

  if (Terminal != nullptr)
  {
    ne_set_session_private(Session, SESSION_TERMINAL_KEY, Terminal);
  }
}

void DestroyNeonSession(ne_session *Session)
{
  TProxyAuthData *ProxyAuthData =
    static_cast<TProxyAuthData *>(ne_get_session_private(Session, SESSION_PROXY_AUTH_KEY));
  if (ProxyAuthData != nullptr)
  {
    delete ProxyAuthData;
  }
  ne_session_destroy(Session);
}

UnicodeString GetNeonError(ne_session * Session)
{
  // The error may contain localized Windows error messages (in legacy Ansi encoding)
  return UnicodeString(AnsiString(ne_get_error(Session)));
}

void CheckNeonStatus(ne_session * Session, int32_t NeonStatus,
  const UnicodeString & AHostName, const UnicodeString & CustomError)
{
  if (NeonStatus == NE_OK)
  {
    // noop
  }
  else
  {
    UnicodeString NeonError = GetNeonError(Session);
    UnicodeString Error;
    if (!CustomError.IsEmpty())
    {
      Error = CustomError;
    }
    else
    {
      switch (NeonStatus)
      {
        case NE_ERROR:
        case NE_SOCKET:
          // noop
          DebugAssert(!NeonError.IsEmpty());
          Error = NeonError;
          NeonError = "";
          break;

        case NE_LOOKUP:
          Error = ReplaceStr(LoadStr(NET_TRANSL_HOST_NOT_EXIST2), "%HOST%", AHostName);
          break;

        case NE_AUTH:
          Error = LoadStr(AUTHENTICATION_FAILED);
          break;

        case NE_PROXYAUTH:
          Error = LoadStr(PROXY_AUTHENTICATION_FAILED);
          break;

        case NE_CONNECT:
          Error = LoadStr(CONNECTION_FAILED);
          break;

        case NE_TIMEOUT:
          Error = ReplaceStr(LoadStr(NET_TRANSL_TIMEOUT2), "%HOST%", AHostName);
          break;

        case NE_REDIRECT:
          {
            char *Uri = ne_uri_unparse(ne_redirect_location(Session));
            Error = FMTLOAD(REQUEST_REDIRECTED, Uri);
            ne_free(Uri);
          }
          break;

        case NE_FAILED: // never used by neon as of 0.30.0
        case NE_RETRY: // not sure if this is a public API
        default:
          DebugFail();
          Error = FORMAT("Unexpected neon error %d", NeonStatus);
          break;
      }
    }

    UnicodeString LogError(Error);
    AddToList(LogError, NeonError, sLineBreak);
    AppLogFmt(L"HTTP request failed: %s", LogError);
    throw ExtException(Error, NeonError);
  }
}

UnicodeString GetNeonRedirectUrl(ne_session *Session)
{
  const ne_uri *RedirectUri = ne_redirect_location(Session);
  char *RedirectUriStr = ne_uri_unparse(RedirectUri);
  UnicodeString Result = StrFromNeon(RedirectUriStr);
  ne_free(RedirectUriStr);
  return Result;
}

constexpr int32_t MAX_REDIRECT_ATTEMPTS = 5;

void CheckRedirectLoop(const UnicodeString RedirectUrl, TStrings *AttemptedUrls)
{
  if (AttemptedUrls->GetCount() > MAX_REDIRECT_ATTEMPTS)
  {
    throw Exception(LoadStr(TOO_MANY_REDIRECTS));
  }
  else
  {
    // Make sure we've not attempted this URL before.
    if (AttemptedUrls->IndexOf(RedirectUrl) >= 0)
    {
      throw Exception(LoadStr(REDIRECT_LOOP));
    }
    AttemptedUrls->Add(RedirectUrl);
  }
}

extern "C"
{

void ne_init_ssl_session(struct ssl_st * Ssl, ne_session * Session)
{
  // void * Code = ne_get_session_private(Session, SESSION_TLS_INIT_KEY);
  // void * Data = ne_get_session_private(Session, SESSION_TLS_INIT_DATA_KEY);
  //TNeonTlsInit OnNeonTlsInit = MakeMethod<TNeonTlsInit>(Data, Code);
  TNeonTlsInit OnNeonTlsInit =
    reinterpret_cast<TNeonTlsInit>(ne_get_session_private(Session, SESSION_TLS_INIT_KEY));
  if (DebugAlwaysTrue(OnNeonTlsInit != nullptr))
  {
    OnNeonTlsInit(Ssl, Session);
  }
}

} // extern "C"

void SetNeonTlsInit(ne_session * Session, TNeonTlsInit OnNeonTlsInit, TTerminal * Terminal)
{
  UnicodeString CertificateStorage = GetConfiguration()->CertificateStorageExpanded;
  if (!CertificateStorage.IsEmpty())
  {
    ne_ssl_set_certificates_storage(Session, StrToNeon(CertificateStorage));
    if (Terminal != nullptr)
    {
      Terminal->LogEvent(FORMAT(L"Using certificate store \"%s\"", CertificateStorage));
    }
  }

  // As the OnNeonTlsInit always only calls SetupSsl, we can simplify this with one shared implementation
#if 0
  TMethod & Method = *(TMethod*)&OnNeonTlsInit;
  ne_set_session_private(Session, SESSION_TLS_INIT_KEY, Method.Code);
  ne_set_session_private(Session, SESSION_TLS_INIT_DATA_KEY, Method.Data);
#endif
  ne_set_session_private(Session, SESSION_TLS_INIT_KEY, nb::ToPtr(OnNeonTlsInit));
}

void InitNeonTls(
  ne_session * Session, TNeonTlsInit OnNeonTlsInit, ne_ssl_verify_fn VerifyCallback, void * VerifyContext,
  TTerminal * Terminal)
{
  SetNeonTlsInit(Session, OnNeonTlsInit, Terminal);

  ne_ssl_set_verify(Session, VerifyCallback, VerifyContext);

  ne_ssl_trust_default_ca(Session);
}

AnsiString NeonExportCertificate(const ne_ssl_certificate * Certificate)
{
  char * AsciiCert = ne_ssl_cert_export(Certificate);
  AnsiString Result = AsciiCert;
  ne_free(AsciiCert);
  return Result;
}

bool NeonWindowsValidateCertificate(int & Failures, const AnsiString & AsciiCert, UnicodeString & Error)
{
  bool Result = false;
  // We can accept only unknown certificate authority.
  if (FLAGSET(Failures, NE_SSL_UNTRUSTED))
  {
    unsigned char *Certificate = nullptr;
    size_t CertificateLen = ne_unbase64(AsciiCert.c_str(), &Certificate);

    if (CertificateLen > 0)
    {
      if (WindowsValidateCertificate(Certificate, CertificateLen, Error))
      {
        Failures &= ~NE_SSL_UNTRUSTED;
        Result = true;
      }
      if (Certificate)
      {
        ne_free(Certificate);
      }
    }
  }
  return Result;
}

bool NeonWindowsValidateCertificateWithMessage(TNeonCertificateData &Data, UnicodeString & AMessage)
{
  bool Result;
  UnicodeString WindowsCertificateError;
  if (NeonWindowsValidateCertificate(Data.Failures, Data.AsciiCert, WindowsCertificateError))
  {
    AMessage = "Certificate verified against Windows certificate store";
    // There can be also other flags, not just the NE_SSL_UNTRUSTED.
    Result = (Data.Failures == 0);
  }
  else
  {
    AMessage =
      FORMAT("Certificate failed to verify against Windows certificate store: %s",
        DefaultStr(WindowsCertificateError, "no details"));
    Result = false;
  }
  return Result;
}

UnicodeString NeonCertificateFailuresErrorStr(int Failures, const UnicodeString & AHostName)
{
  int FailuresToList = Failures;

  UnicodeString Result;
  if (FLAGSET(FailuresToList, NE_SSL_NOTYETVALID))
  {
    AddToList(Result, LoadStr(CERT_ERR_CERT_NOT_YET_VALID), " ");
    FailuresToList &= ~NE_SSL_NOTYETVALID;
  }
  if (FLAGSET(FailuresToList, NE_SSL_EXPIRED))
  {
    AddToList(Result, LoadStr(CERT_ERR_CERT_HAS_EXPIRED), " ");
    FailuresToList &= ~NE_SSL_EXPIRED;
  }
  // NEON checks certificate host name on its own
  if (FLAGSET(FailuresToList, NE_SSL_IDMISMATCH))
  {
    AddToList(Result, FMTLOAD(CERT_NAME_MISMATCH, AHostName), " ");
    FailuresToList &= ~NE_SSL_IDMISMATCH;
  }
  if (FLAGSET(FailuresToList, NE_SSL_UNTRUSTED))
  {
    AddToList(Result, LoadStr(CERT_ERR_CERT_UNTRUSTED), " ");
    FailuresToList &= ~NE_SSL_UNTRUSTED;
  }
  if (FLAGSET(FailuresToList, NE_SSL_BADCHAIN))
  {
    AddToList(Result, LoadStr(CERT_ERR_BAD_CHAIN), " ");
    FailuresToList &= ~NE_SSL_BADCHAIN;
  }
  // nb, NE_SSL_REVOKED is never used by OpenSSL implementation
  if (FailuresToList != 0)
  {
    AddToList(Result, LoadStr(CERT_ERR_UNKNOWN), " ");
  }
  return Result;
}

static std::unique_ptr<TCriticalSection> DebugSection(TraceInitPtr(std::make_unique<TCriticalSection>()));
static nb::set_t<TTerminal *> NeonTerminals;

extern "C"
{

void ne_debug(void *Context, int Channel, const char *Format, ...)
{
  bool DoLog;

  if (FLAGSET(Channel, NE_DBG_SOCKET) ||
      FLAGSET(Channel, NE_DBG_HTTP) ||
      FLAGSET(Channel, NE_DBG_HTTPAUTH) ||
      FLAGSET(Channel, NE_DBG_SSL))
  {
    DoLog = (GetConfiguration()->ActualLogProtocol >= 0);
  }
  else if (FLAGSET(Channel, NE_DBG_XML) ||
           FLAGSET(Channel, NE_DBG_WINSCP_HTTP_DETAIL))
  {
    DoLog = (GetConfiguration()->GetActualLogProtocol() >= 1);
  }
  else if (FLAGSET(Channel, NE_DBG_LOCKS) ||
           FLAGSET(Channel, NE_DBG_XMLPARSE) ||
           FLAGSET(Channel, NE_DBG_HTTPBODY))
  {
    DoLog = (GetConfiguration()->GetActualLogProtocol() >= 2);
  }
  else
  {
    DoLog = false;
    DebugFail();
  }

  #ifndef _DEBUG
  if (DoLog)
  #endif
  {
    va_list Args;
    va_start(Args, Format);
    UTF8String UTFMessage;
    UTFMessage.vprintf(Format, Args);
    va_end(Args);

    UnicodeString Message = TrimRight(UnicodeString(UTFMessage));

    if (DoLog)
    {
      // Note that this gets called for THttp sessions too.
      // It does no harm atm.
      TTerminal *Terminal = nullptr;
      if (Context != nullptr)
      {
        ne_session *Session = static_cast<ne_session *>(Context);

        Terminal =
          static_cast<TTerminal *>(ne_get_session_private(Session, SESSION_TERMINAL_KEY));
      }
      else
      {
        TGuard Guard(*DebugSection.get()); nb::used(Guard);

        if (NeonTerminals.size() == 1)
        {
          Terminal = *NeonTerminals.begin();
        }
      }

      if (Terminal != nullptr)
      {
        Terminal->LogEvent(Message);
      }
    }
  }
}

} // extern "C"

void RegisterForNeonDebug(TTerminal *Terminal)
{
  TGuard Guard(*DebugSection.get()); nb::used(Guard);
  NeonTerminals.insert(Terminal);
}

void UnregisterFromNeonDebug(TTerminal *Terminal)
{
  TGuard Guard(*DebugSection.get()); nb::used(Guard);
  NeonTerminals.erase(Terminal);
}

void RetrieveNeonCertificateData(
  int Failures, const ne_ssl_certificate *Certificate, TNeonCertificateData &Data)
{
  UnicodeString Unknown(L"<unknown>");
  char FingerprintSHA1[NE_SSL_DIGESTLEN];
  FingerprintSHA1[0] = '\0';
  if (DebugAlwaysFalse(ne_ssl_cert_digest(Certificate, FingerprintSHA1) != 0))
  {
    Data.FingerprintSHA1 = Unknown;
  }
  else
  {
    Data.FingerprintSHA1 = StrFromNeon(FingerprintSHA1);
  }

  char * FingeprintSHA256 = ne_ssl_cert_hdigest(Certificate, NE_HASH_SHA256);
  if (DebugAlwaysFalse(FingeprintSHA256 == nullptr))
  {
    Data.FingerprintSHA256 = Unknown;
  }
  else
  {
    UnicodeString Buf = StrFromNeon(FingeprintSHA256);
    if (DebugAlwaysTrue(Buf.Length() > 2) &&
        DebugAlwaysTrue((Buf.Length() % 2) == 0))
    {
      for (int Index = 3; Index < Buf.Length(); Index += 3)
      {
        Buf.Insert(L":", Index);
      }
    }
    Data.FingerprintSHA256 = Buf;
    ne_free(FingeprintSHA256);
  }

  Data.AsciiCert = NeonExportCertificate(Certificate);

  char * Subject = ne_ssl_readable_dname(ne_ssl_cert_subject(Certificate));
  Data.Subject = StrFromNeon(Subject);
  ne_free(Subject);
  char * Issuer = ne_ssl_readable_dname(ne_ssl_cert_issuer(Certificate));
  Data.Issuer = StrFromNeon(Issuer);
  ne_free(Issuer);

  Data.Failures = Failures;

  time_t ValidFrom;
  time_t ValidUntil;
  ne_ssl_cert_validity_time(Certificate, &ValidFrom, &ValidUntil);
  Data.ValidFrom = UnixToDateTime(ValidFrom, dstmWin);
  Data.ValidUntil = UnixToDateTime(ValidUntil, dstmWin);
}

UnicodeString CertificateVerificationMessage(const TNeonCertificateData &Data)
{
  return
    FORMAT("Verifying certificate for \"%s\" with fingerprint %s and %2.2X failures",
           Data.Subject, Data.FingerprintSHA256, Data.Failures);
}

UnicodeString CertificateSummary(const TNeonCertificateData &Data, const UnicodeString AHostName)
{
  UnicodeString Summary;
  if (Data.Failures == 0)
  {
    Summary = LoadStr(CERT_OK);
  }
  else
  {
    Summary = NeonCertificateFailuresErrorStr(Data.Failures, AHostName);
  }

  UnicodeString ValidityTimeFormat = "ddddd tt";
  return
    FMTLOAD(CERT_TEXT2,
      Data.Issuer + L"\n",
      Data.Subject + L"\n",
      FormatDateTime(ValidityTimeFormat, Data.ValidFrom),
      FormatDateTime(ValidityTimeFormat, Data.ValidUntil),
      Data.FingerprintSHA256,
      Data.FingerprintSHA1,
      Summary);
}

UnicodeString NeonTlsSessionInfo(
  ne_session *Session, TSessionInfo &SessionInfo, UnicodeString &TlsVersionStr)
{
  TlsVersionStr = StrFromNeon(ne_ssl_get_version(Session));
  AddToList(SessionInfo.SecurityProtocolName, TlsVersionStr, ", ");

  char *Buf = ne_ssl_get_cipher(Session);
  UnicodeString Cipher = StrFromNeon(Buf);
  ne_free(Buf);
  SessionInfo.CSCipher = Cipher;
  SessionInfo.SCCipher = Cipher;

  // see CAsyncSslSocketLayer::PrintSessionInfo()
  return FORMAT("Using %s, cipher %s", TlsVersionStr, Cipher);
}

void SetupSsl(ssl_st * Ssl, TTlsVersion MinTlsVersion, TTlsVersion MaxTlsVersion)
{
  MaxTlsVersion = (TTlsVersion)std::max(MaxTlsVersion, tlsMin); // the lowest currently supported version
  #define MASK_TLS_VERSION(VERSION, FLAG) ((MinTlsVersion > VERSION) || (MaxTlsVersion < VERSION) ? FLAG : 0)
  int32_t Options =
    MASK_TLS_VERSION(tls10, SSL_OP_NO_TLSv1) |
    MASK_TLS_VERSION(tls11, SSL_OP_NO_TLSv1_1) |
    MASK_TLS_VERSION(tls12, SSL_OP_NO_TLSv1_2) |
    MASK_TLS_VERSION(tls13, SSL_OP_NO_TLSv1_3);
  // adds flags (not sets)
  SSL_set_options(Ssl, Options);

  // Since OpenSSL 3, SSL 3.0, TLS 1.0 and 1.1 are enabled on security level 0 only
  if (MinTlsVersion <= tls11)
  {
    SSL_set_security_level(Ssl, 0);
  }
}

void UpdateNeonDebugMask()
{
  // Other flags:
  // NE_DBG_FLUSH - used only in native implementation of ne_debug
  // NE_DBG_HTTPPLAIN - log credentials in HTTP authentication

  ne_debug_mask =
    NE_DBG_SOCKET |
    NE_DBG_HTTP |
    NE_DBG_XML | // detail
    NE_DBG_HTTPAUTH |
    NE_DBG_LOCKS | // very details
    NE_DBG_XMLPARSE | // very details
    NE_DBG_HTTPBODY | // very details
    NE_DBG_SSL |
    FLAGMASK(GetConfiguration()->GetLogSensitive(), NE_DBG_HTTPPLAIN);
}
