//---------------------------------------------------------------------------
#pragma once
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <memory>
//---------------------------------------------------------------------------
struct ne_session_s;
struct ne_request_s;
struct ne_ssl_certificate_s;
struct ssl_st;
//---------------------------------------------------------------------------
class THttp;
#if 0
typedef void (__closure *THttpDownloadEvent)(THttp *Sender, __int64 Size, bool &Cancel);
#endif // #if 0
typedef nb::FastDelegate3<void,
  THttp * /*Sender*/, int64_t /*Size*/, bool & /*Cancel*/> THttpDownloadEvent;
#if 0
typedef void (__closure *THttpErrorEvent)(THttp *Sender, int Status, const UnicodeString &Message);
#endif // #if 0
typedef nb::FastDelegate3<void,
  THttp * /*Sender*/, int /*Status*/, UnicodeString /*Message*/> THttpErrorEvent;
//---------------------------------------------------------------------------
class THttp : public TObject
{
public:
  THttp() noexcept;
  ~THttp() noexcept;

  void Get();
  void Post(const UnicodeString Request);
  bool IsCertificateError() const;

  __property UnicodeString URL = { read = FURL, write = FURL };
  __property UnicodeString ProxyHost = { read = FProxyHost, write = FProxyHost };
  __property int ProxyPort = { read = FProxyPort, write = FProxyPort };
  __property TStrings *RequestHeaders = { read = FRequestHeaders, write = FRequestHeaders };
  __property UnicodeString Response = { read = GetResponse };
  __property RawByteString ResponseRaw = { read = FResponse };
  __property TStrings *ResponseHeaders = { read = FResponseHeaders };
  __property int64_t ResponseLength = { read = GetResponseLength };
  __property int64_t ResponseLimit = { read = FResponseLimit, write = FResponseLimit };
  __property THttpDownloadEvent OnDownload = { read = FOnDownload, write = FOnDownload };
  __property THttpErrorEvent OnError = { read = FOnError, write = FOnError };

  UnicodeString GetURL() const { return FURL; }
  void SetURL(UnicodeString Value) { FURL = Value; }
  UnicodeString GetProxyHost() const { return FProxyHost; }
  void SetProxyHost(UnicodeString Value) { FProxyHost = Value; }
  intptr_t GetProxyPort() const { return FProxyPort; }
  void SetProxyPort(intptr_t Value) { FProxyPort = Value; }
  TStrings *GetRequestHeaders() const { return FRequestHeaders.get(); }
  void SetRequestHeaders(TStrings *Value) { FRequestHeaders.reset(Value); }
  RawByteString GetResponseRaw() const { return FResponse; }
  TStrings *GetResponseHeaders() const { return FResponseHeaders.get(); }
  int64_t GetResponseLimit() const { return FResponseLimit; }
  void SetResponseLimit(int64_t Value) { FResponseLimit = Value; }
  THttpDownloadEvent GetOnDownload() const { return FOnDownload; }
  void SetOnDownload(THttpDownloadEvent Value) { FOnDownload = Value; }
  THttpErrorEvent GetOnError() const { return FOnError; }
  void SetOnError(THttpErrorEvent Value) { FOnError = Value; }

private:
  UnicodeString FURL;
  UnicodeString FProxyHost;
  intptr_t FProxyPort{0};
  RawByteString FResponse;
  int64_t FResponseLimit{-1};
  std::unique_ptr<Exception> FException;
  THttpDownloadEvent FOnDownload{nullptr};
  THttpErrorEvent FOnError{nullptr};
  UnicodeString FHostName;
  UnicodeString FCertificateError;
  std::unique_ptr<TStrings> FRequestHeaders;
  std::unique_ptr<TStrings> FResponseHeaders;

  static int NeonBodyReader(void *UserData, const char *Buf, size_t Len);
  int NeonBodyReaderImpl(const char *Buf, size_t Len);
  void SendRequest(const char *Method, const UnicodeString Request);
  UnicodeString GetResponse() const;
  int64_t GetResponseLength() const;
  static void InitSslSession(ssl_st *Ssl, ne_session_s *Session);
  static int NeonServerSSLCallback(void *UserData, int Failures, const ne_ssl_certificate_s *Certificate);
  int NeonServerSSLCallbackImpl(int Failures, const ne_ssl_certificate_s *Certificate);
};
//---------------------------------------------------------------------------
