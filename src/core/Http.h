
#pragma once

#include <Classes.hpp>
#include <memory>

struct ne_session_s;
struct ne_request_s;
struct ne_ssl_certificate_s;
struct ssl_st;

class THttp;
using THttpDownloadEvent = nb::FastDelegate3<void,
  THttp * /*Sender*/, int64_t /*Size*/, bool & /*Cancel*/>;
using THttpErrorEvent = nb::FastDelegate3<void,
  THttp * /*Sender*/, int32_t /*Status*/, const UnicodeString & /*Message*/>;

constexpr const int32_t BasicHttpResponseLimit = 100 * 1024;

class THttp final : public TObject
{
public:
  THttp() noexcept;
  ~THttp() noexcept;

  void Get();
  void Post(const UnicodeString & Request);
  bool IsCertificateError() const;

  __property UnicodeString URL = { read = FURL, write = FURL };
  RWPropertySimple<UnicodeString> URL{&FURL, nb::bind(&THttp::SetURL, this)};
  __property UnicodeString ProxyHost = { read = FProxyHost, write = FProxyHost };
  __property int32_t ProxyPort = { read = FProxyPort, write = FProxyPort };
  __property TStrings * RequestHeaders = { read = FRequestHeaders, write = FRequestHeaders };
  __property UnicodeString Response = { read = GetResponse };
  ROProperty<UnicodeString> Response{nb::bind(&THttp::GetResponse, this)};
  __property RawByteString ResponseRaw = { read = FResponse };
  __property TStrings * ResponseHeaders = { read = FResponseHeaders };
  __property int64_t ResponseLength = { read = GetResponseLength };
  __property int64_t ResponseLimit = { read = FResponseLimit, write = FResponseLimit };
  RWPropertySimple<int64_t> ResponseLimit{&FResponseLimit, nb::bind(&THttp::SetResponseLimit, this)};
  __property THttpDownloadEvent OnDownload = { read = FOnDownload, write = FOnDownload };
  __property THttpErrorEvent OnError = { read = FOnError, write = FOnError };
  __property UnicodeString Certificate = { read = FCertificate, write = FCertificate };
  RWProperty2<UnicodeString> Certificate{&FCertificate};

  UnicodeString GetURL() const { return FURL; }
  void SetURL(const UnicodeString & Value) { FURL = Value; }
  UnicodeString GetProxyHost() const { return FProxyHost; }
  void SetProxyHost(const UnicodeString & Value) { FProxyHost = Value; }
  int32_t GetProxyPort() const { return FProxyPort; }
  void SetProxyPort(int32_t Value) { FProxyPort = Value; }
  TStrings * GetRequestHeaders() const { return FRequestHeaders.get(); }
  void SetRequestHeaders(TStrings * Value) { FRequestHeaders.reset(Value); }
  RawByteString GetResponseRaw() const { return FResponse; }
  TStrings * GetResponseHeaders() const { return FResponseHeaders.get(); }
  int64_t GetResponseLimit() const { return FResponseLimit; }
  void SetResponseLimit(int64_t Value) { FResponseLimit = Value; }
  THttpDownloadEvent & GetOnDownload() { return FOnDownload; }
  void SetOnDownload(THttpDownloadEvent && Value) { FOnDownload = std::move(Value); }
  THttpErrorEvent & GetOnError() { return FOnError; }
  void SetOnError(THttpErrorEvent && Value) { FOnError = std::move(Value); }

private:
  UnicodeString FURL;
  UnicodeString FProxyHost;
  int32_t FProxyPort{0};
  RawByteString FResponse;
  int64_t FResponseLimit{-1};
  std::unique_ptr<Exception> FException;
  THttpDownloadEvent FOnDownload{nullptr};
  THttpErrorEvent FOnError{nullptr};
  UnicodeString FHostName;
  UnicodeString FCertificateError;
  std::unique_ptr<TStrings> FRequestHeaders;
  std::unique_ptr<TStrings> FResponseHeaders;
  UnicodeString FCertificate;

  static int32_t NeonBodyReader(void * UserData, const char * Buf, size_t Len);
  int32_t NeonBodyReaderImpl(const char * Buf, size_t Len);
  void SendRequest(const char * Method, const UnicodeString & Request);
  UnicodeString GetResponse() const;
  int64_t GetResponseLength() const;
  static void InitSslSession(ssl_st * Ssl, ne_session_s * Session);
  static int32_t NeonServerSSLCallback(void * UserData, int32_t Failures, const ne_ssl_certificate_s * Certificate);
  int32_t NeonServerSSLCallbackImpl(int32_t Failures, const ne_ssl_certificate_s * Certificate);
};

