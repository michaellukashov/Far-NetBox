//---------------------------------------------------------------------------
#pragma once
//---------------------------------------------------------------------------
#ifndef WINSCP
#define WINSCP
#endif
#include <ne_uri.h>
#include <ne_session.h>
#include <SessionData.h>
//---------------------------------------------------------------------------
#define StrToNeon(S) UTF8String(S).c_str()
#define StrFromNeon(S) UnicodeString(UTF8String(S))
#define SESSION_FS_KEY "filesystem"
//---------------------------------------------------------------------------
struct TNeonCertificateData
{
  CUSTOM_MEM_ALLOCATION_IMPL

  UnicodeString Subject;
  UnicodeString Issuer;

  TDateTime ValidFrom;
  TDateTime ValidUntil;

  UnicodeString Fingerprint;
  AnsiString AsciiCert;

  int Failures = 0;
};
//---------------------------------------------------------------------------
void NeonParseUrl(const UnicodeString Url, ne_uri &uri);
bool IsTlsUri(const ne_uri &uri);
ne_session *CreateNeonSession(const ne_uri &uri);
void InitNeonSession(ne_session *Session, TProxyMethod ProxyMethod, const UnicodeString AProxyHost,
  intptr_t ProxyPort, const UnicodeString AProxyUsername, const UnicodeString AProxyPassword, TTerminal * Terminal);
void DestroyNeonSession(ne_session *Session);
UnicodeString GetNeonError(ne_session *Session);
void CheckNeonStatus(ne_session *Session, intptr_t NeonStatus,
  const UnicodeString AHostName, const UnicodeString CustomError = L"");
UnicodeString GetNeonRedirectUrl(ne_session *Session);
void CheckRedirectLoop(const UnicodeString RedirectUrl, TStrings *AttemptedUrls);
__removed typedef void (__closure* TNeonTlsInit)(struct ssl_st * Ssl, ne_session * Session);
typedef void (*TNeonTlsInit)(struct ssl_st *Ssl, ne_session *Session);
void SetNeonTlsInit(ne_session *Session, TNeonTlsInit OnNeonTlsInit);
AnsiString NeonExportCertificate(const ne_ssl_certificate *Certificate);
bool NeonWindowsValidateCertificate(int &Failures, const AnsiString AsciiCert, UnicodeString &Error);
bool NeonWindowsValidateCertificateWithMessage(TNeonCertificateData &Data, UnicodeString &Message);
UnicodeString NeonCertificateFailuresErrorStr(int Failures, const UnicodeString HostName);
void UpdateNeonDebugMask();
void __fastcall RegisterForNeonDebug(TTerminal * Terminal);
void __fastcall UnregisterFromNeonDebug(TTerminal * Terminal);
void __fastcall NeonInitialize();
void __fastcall NeonFinalize();
void __fastcall RequireNeon(TTerminal * Terminal);
void __fastcall RetrieveNeonCertificateData(
  int Failures, const ne_ssl_certificate * Certificate, TNeonCertificateData &Data);
UnicodeString __fastcall CertificateVerificationMessage(const TNeonCertificateData &Data);
UnicodeString __fastcall CertificateSummary(const TNeonCertificateData &Data, const UnicodeString AHostName);
struct TSessionInfo;
UnicodeString __fastcall NeonTlsSessionInfo(
  ne_session * Session, TSessionInfo & FSessionInfo, UnicodeString & TlsVersionStr);
void SetupSsl(ssl_st * Ssl, TTlsVersion MinTlsVersion, TTlsVersion MaxTlsVersion);
//---------------------------------------------------------------------------
