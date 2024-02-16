
#include <vcl.h>
#pragma hdrstop

#include "Http.h"
#include "NeonIntf.h"
#include "Exceptions.h"
#include "CoreMain.h"
#include "ne_request.h"
#include "TextsCore.h"
#include <openssl/ssl.h>

//constexpr const int32_t BasicHttpResponseLimit = 100 * 1024;

THttp::THttp() noexcept :
  FResponseHeaders(std::make_unique<TStringList>())
{
  FProxyPort = 0;
  FOnDownload = nullptr;
  FOnError = nullptr;
  FResponseLimit = -1;
#if defined(__BORLANDC__)
  FRequestHeaders = nullptr;
  FResponseHeaders = new TStringList();
#endif
}

THttp::~THttp() noexcept
{
#if defined(__BORLANDC__)
  delete FResponseHeaders;
#endif
}

void THttp::SendRequest(const char * Method, const UnicodeString & Request)
{
  std::unique_ptr<TStringList> AttemptedUrls(CreateSortedStringList());
  AttemptedUrls->Add(GetURL());
  UnicodeString RequestUrl = GetURL();
  bool WasTlsUri = false; // shut up

  bool Retry;

  do
  {
    ne_uri uri;
    NeonParseUrl(RequestUrl, uri);

    bool IsTls = IsTlsUri(uri);
    if (RequestUrl == GetURL())
    {
      WasTlsUri = IsTls;
    }
    else
    {
      if (!IsTls && WasTlsUri)
      {
        throw Exception(LoadStr(UNENCRYPTED_REDIRECT));
      }
    }

    FHostName = StrFromNeon(uri.host);

    UnicodeString Uri = StrFromNeon(uri.path);
    if (uri.query != nullptr)
    {
      Uri += L"?" + StrFromNeon(uri.query);
    }

    FResponse.SetLength(0);
    FCertificateError.SetLength(0);
    FException.reset(nullptr);

    ne_session_s * NeonSession = CreateNeonSession(uri);
    try__finally
    {
      TProxyMethod ProxyMethod = GetProxyHost().IsEmpty() ? ::pmNone : pmHTTP;
      InitNeonSession(NeonSession, ProxyMethod, GetProxyHost(), GetProxyPort(), UnicodeString(), UnicodeString(), nullptr);

      if (IsTls)
      {
        InitNeonTls(NeonSession, InitSslSession, NeonServerSSLCallback, this, nullptr);

        ne_ssl_trust_default_ca(NeonSession);
      }

      ne_request_s * NeonRequest = ne_request_create(NeonSession, Method, StrToNeon(Uri));
      try__finally
      {
        if (FRequestHeaders != nullptr)
        {
          for (int32_t Index = 0; Index < FRequestHeaders->GetCount(); Index++)
          {
            ne_add_request_header(
              NeonRequest, StrToNeon(FRequestHeaders->GetName(Index)), StrToNeon(FRequestHeaders->GetValueFromIndex(Index)));
          }
        }

        // UTF8String RequestUtf;
        if (!Request.IsEmpty())
        {
          UTF8String RequestUtf = UTF8String(Request);
          ne_set_request_body_buffer(NeonRequest, RequestUtf.c_str(), RequestUtf.Length());
        }

        ne_add_response_body_reader(NeonRequest, ne_accept_2xx, NeonBodyReader, this);

        int32_t Status = ne_request_dispatch(NeonRequest);

        // Exception has precedence over status as status will always be NE_ERROR,
        // as we returned 1 from NeonBodyReader
        if (FException != nullptr)
        {
          RethrowException(FException.get());
        }

        if (Status == NE_REDIRECT)
        {
          Retry = true;
          RequestUrl = GetNeonRedirectUrl(NeonSession);
          CheckRedirectLoop(RequestUrl, AttemptedUrls.get());
        }
        else
        {
          Retry = false;
          CheckNeonStatus(NeonSession, Status, FHostName, FCertificateError);

          const ne_status * NeonStatus = ne_get_status(NeonRequest);
          if (NeonStatus->klass != 2)
          {
            int32_t StatusCode = NeonStatus->code;
            const UnicodeString Message = StrFromNeon(NeonStatus->reason_phrase);
            if (!GetOnError().empty())
            {
              GetOnError()(this, StatusCode, Message);
            }
            throw Exception(FMTLOAD(HTTP_ERROR2, StatusCode, Message, FHostName));
          }

          void * Cursor = nullptr;
          const char * HeaderName;
          const char * HeaderValue;
          while ((Cursor = ne_response_header_iterate(NeonRequest, Cursor, &HeaderName, &HeaderValue)) != nullptr)
          {
            FResponseHeaders->SetValue(StrFromNeon(HeaderName), StrFromNeon(HeaderValue));
          }
        }
      }
      __finally
      {
        ne_request_destroy(NeonRequest);
      } end_try__finally
    }
    __finally
    {
      DestroyNeonSession(NeonSession);
      ne_uri_free(&uri);
    } end_try__finally
  }
  while (Retry);
}

void THttp::Get()
{
  SendRequest("GET", UnicodeString());
}

void THttp::Post(const UnicodeString & Request)
{
  SendRequest("POST", Request);
}

UnicodeString THttp::GetResponse() const
{
  UTF8String UtfResponse(FResponse.c_str(), FResponse.GetLength());
  return UnicodeString(UtfResponse);
}

int32_t THttp::NeonBodyReaderImpl(const char * Buf, size_t Len)
{
  bool Result = true;
  if ((FResponseLimit < 0) ||
      (FResponse.Length() + nb::ToInt32(Len) <= FResponseLimit))
  {
    FResponse += RawByteString(Buf, nb::ToInt32(Len));

    if (FOnDownload != nullptr)
    {
      bool Cancel = false;

      try
      {
        FOnDownload(this, GetResponseLength(), Cancel);
      }
      catch (Exception & E)
      {
        FException.reset(CloneException(&E));
        Result = false;
      }

      if (Cancel)
      {
        FException = std::make_unique<EAbort>(UnicodeString());
        Result = false;
      }
    }
  }

  // neon wants 0 for success
  return Result ? 0 : 1;
}

int32_t THttp::NeonBodyReader(void * UserData, const char * Buf, size_t Len)
{
  THttp * Http = static_cast<THttp *>(UserData);
  return Http->NeonBodyReaderImpl(Buf, Len);
}

int64_t THttp::GetResponseLength() const
{
  return FResponse.Length();
}

void THttp::InitSslSession(ssl_st * Ssl, ne_session * /*Session*/)
{
  SetupSsl(Ssl, tlsDefaultMin, tlsMax);
}

int32_t THttp::NeonServerSSLCallback(void * UserData, int32_t Failures, const ne_ssl_certificate * Certificate)
{
  THttp * Http = static_cast<THttp *>(UserData);
  return Http->NeonServerSSLCallbackImpl(Failures, Certificate);
}

enum { hcvNoWindows = 0x01, hcvNoKnown = 0x02 };

int32_t THttp::NeonServerSSLCallbackImpl(int32_t Failures, const ne_ssl_certificate * ACertificate)
{
  AnsiString AsciiCert = NeonExportCertificate(ACertificate);

  UnicodeString WindowsCertificateError;
  if ((Failures != 0) && FLAGCLEAR(GetConfiguration()->HttpsCertificateValidation, hcvNoWindows))
  {
    AppLogFmt(L"TLS failure: %s (%d)", NeonCertificateFailuresErrorStr(Failures, FHostName), Failures);
    AppLogFmt(L"Hostname: %s, Certificate: %s", FHostName, UnicodeString(AsciiCert));
    if (NeonWindowsValidateCertificate(Failures, AsciiCert, WindowsCertificateError))
    {
      AppLogFmt(L"Certificate trusted by Windows certificate store (%d)", Failures);
    }
    if (!WindowsCertificateError.IsEmpty())
    {
      AppLogFmt(L"Error from Windows certificate store: %s", WindowsCertificateError);
    }
  }

  if ((Failures != 0) && FLAGSET(Failures, NE_SSL_UNTRUSTED) && FLAGCLEAR(GetConfiguration()->HttpsCertificateValidation, hcvNoKnown) &&
      !Certificate().IsEmpty())
  {
    const ne_ssl_certificate * RootCertificate = ACertificate;
    do
    {
      const ne_ssl_certificate * Issuer = ne_ssl_cert_signedby(RootCertificate);
      if (Issuer != nullptr)
      {
        RootCertificate = Issuer;
      }
      else
      {
        break;
      }
    }
    while (true);

    const UnicodeString RootCert = UnicodeString(NeonExportCertificate(RootCertificate));
    if (RootCert == Certificate)
    {
      Failures &= ~NE_SSL_UNTRUSTED;
      AppLogFmt(L"Certificate is known (%d)", Failures);
    }
  }

  if (Failures != 0)
  {
    FCertificateError = NeonCertificateFailuresErrorStr(Failures, FHostName);
    AppLogFmt(L"TLS certificate error: %s", FCertificateError);
    AddToList(FCertificateError, WindowsCertificateError, L"\n");
  }

  return (Failures == 0) ? NE_OK : NE_ERROR;
}

bool THttp::IsCertificateError() const
{
  return !FCertificateError.IsEmpty();
}

