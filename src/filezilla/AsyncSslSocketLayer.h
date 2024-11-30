/*           CAsyncSslSocketLayer by Tim Kosse
          mailto: tim.kosse@filezilla-project.org)
                 Version 2.0 (2005-02-27)
-------------------------------------------------------------

Introduction
------------

CAsyncSslSocketLayer is a layer class for CAsyncSocketEx which allows you to establish SSL secured
connections. Support for both client and server side is provided.

How to use
----------

Using this class is really simple. In the easiest case, just add an instance of
CAsyncSslSocketLayer to your socket and call InitClientSsl after creation of the socket.

This class only has a couple of public functions:
- int InitSSLConnection(bool clientMode);
  This functions establishes an SSL connection. The clientMode parameter specifies whether the SSL connection
  is in server or in client mode.
  Most likely you want to call this function right after calling Create for the socket.
  But sometimes, you'll need to call this function later. One example is for an FTP connection
  with explicit SSL: In this case you would have to call InitSSLConnection after receiving the reply
  to an 'AUTH SSL' command.
  InitSSLConnection returns 0 on success, else an error code as described below under SSL_FAILURE
- Is UsingSSL();
  Returns true if you've previously called InitClientSsl()
- SetNotifyReply(SetNotifyReply(int nID, int nCode, int result);
  You can call this function only after receiving a layerspecific callback with the SSL_VERIFY_CERT
  id. Set result to 1 if you trust the certificate and 0 if you don't trust it.
  nID has to be the priv_data element of the t_SslCertData structure and nCode has to be SSL_VERIFY_CERT.

This layer sends some layerspecific notifications to your socket instance, you can handle them in
OnLayerCallback of your socket class.
Valid notification IDs are:
- SSL_INFO 0
  There are two possible values for param2:
  SSL_INFO_ESTABLISHED 0 - You'll get this notification if the SSL negotiation was successful
  SSL_INFO_SHUTDOWNCOMPLETE 1 - You'll get this notification if the SSL connection has been shut
                                  down successfully. See below for details.
- SSL_FAILURE 1
  This notification is sent if the SSL connection could not be established or if an existing
  connection failed. Valid values for param2 are:
  - SSL_FAILURE_UNKNOWN 0 - Details may have been sent with a SSL_VERBOSE_* notification.
  - SSL_FAILURE_ESTABLISH 1 - Problem during SSL negotiation
  - SSL_FAILURE_INITSSL 4
  - SSL_FAILURE_VERIFYCERT 8 - The remote SSL certificate was invalid
  - SSL_FAILURE_CERTREJECTED 16 - The remote SSL certificate was rejected by user
- SSL_VERIFY_CERT 2
  This notification is sent each time a remote certificate has to be verified.
  param2 is a pointer to a t_SslCertData structure which contains some information
  about the remote certificate.
  You have to set the reply to this message using the SetNotifyReply function.

Be careful with closing the connection after sending data, not all data may have been sent already.
Before closing the connection, you should call Shutdown() and wait for the SSL_INFO_SHUTDOWNCOMPLETE
notification. This assures that all encrypted data really has been sent.

License
-------

Feel free to use this class, as long as you don't claim that you wrote it
and this copyright notice stays intact in the source files.
If you want to use this class in a commercial application, a short message
to tim.kosse@filezilla-project.org would be appreciated but is not required.

This product includes software developed by the OpenSSL Project
for use in the OpenSSL Toolkit. (https://openssl-library.org/)
*/

#pragma once

#include "AsyncSocketExLayer.h"
#include <openssl/ssl.h>

// Details of SSL certificate, can be used by app to verify if certificate is valid
struct t_SslCertData
{
CUSTOM_MEM_ALLOCATION_IMPL

  t_SslCertData()
  {
    nb::ClearStruct(subject);
    nb::ClearStruct(issuer);
    nb::ClearStruct(validFrom);
    nb::ClearStruct(validUntil);
    nb::ClearArray(subjectAltName);
    nb::ClearArray(hashSha1);
    nb::ClearArray(hashSha256);
    certificate = 0;
    certificateLen = 0;
    verificationResult = 0;
    verificationDepth = 0;
    priv_data = 0;
  }

  ~t_SslCertData()
  {
    nb_free(certificate);
  }

  struct t_Contact
  {
    TCHAR Organization[256]{};
    TCHAR Unit[256]{};
    TCHAR CommonName[256]{};
    TCHAR Mail[256]{};
    TCHAR Country[256]{};
    TCHAR StateProvince[256]{};
    TCHAR Town[256]{0};
    TCHAR Other[1024]{0};
  } subject, issuer;

  struct t_validTime
  {
    // Year, Month, day, hour, minute, second
    int y{0}, M{0}, d{0}, h{0}, m{0}, s{0};
  } validFrom, validUntil;

  TCHAR subjectAltName[10240]{};

  uint8_t hashSha1[20]{};
  uint8_t hashSha256[32]{};

  uint8_t * certificate{nullptr};
  size_t certificateLen{0};

  int verificationResult{0};
  int verificationDepth{0};

  int priv_data{0}; //Internal data, do not modify
};

class CFileZillaTools;

class CAsyncSslSocketLayer : public CAsyncSocketExLayer
{
public:
  BOOL SetCertStorage(CString file);
  CAsyncSslSocketLayer();
  virtual ~CAsyncSslSocketLayer();

  void SetNotifyReply(int nID, int nCode, int result);
  BOOL GetPeerCertificateData(t_SslCertData & SslCertData, LPCTSTR & Error);
  std::string GetTlsVersionStr() const;
  std::string GetCipherName() const;
  void SetClientCertificate(X509 * Certificate, EVP_PKEY * PrivateKey);

  bool IsUsingSSL();
  int InitSSLConnection(bool clientMode,
    CAsyncSslSocketLayer * main,
    bool sessionreuse, const CString & host, CFileZillaTools * tools);

  // Send raw text, useful to send a confirmation after the ssl connection
  // has been initialized
  int SendRaw(const void * lpBuf, int nBufLen, int nFlags = 0);

  void* GetContext() { return m_ssl_ctx; }

private:
  virtual void Close();
  virtual BOOL Connect(LPCTSTR lpszHostAddress, UINT nHostPort);
  virtual BOOL Connect(const SOCKADDR* lpSockAddr, int nSockAddrLen);
  virtual void OnConnect(int nErrorCode);
  virtual void OnReceive(int nErrorCode);
  virtual void OnSend(int nErrorCode);
  virtual void OnClose(int nErrorCode);
  virtual int Receive(void * lpBuf, int nBufLen, int nFlags = 0);
  virtual int Send(const void * lpBuf, int nBufLen, int nFlags = 0);
  virtual BOOL ShutDown( int nHow = sends );

  void ResetSslSession();
  void PrintSessionInfo();
  BOOL ShutDownComplete();
  int InitSSL();
  void PrintLastErrorMsg();
  void SetSession(SSL_SESSION * Session);
  bool HandleSession(SSL_SESSION * Session);
  int ProcessSendBuffer();

  void TriggerEvents();
  void LogSslError(const SSL *s, const char * str, const char * fmt, int nMessageType, char * debug = NULL);

  // Will be called from the OpenSSL library
  static void apps_ssl_info_callback(const SSL * s, int where, int ret);
  static int verify_callback(int preverify_ok, X509_STORE_CTX * ctx);
  static int ProvideClientCert(
    SSL * Ssl, X509 ** Certificate, EVP_PKEY ** PrivateKey);
  static int NewSessionCallback(struct ssl_st * Ssl, SSL_SESSION * Session);
  static CAsyncSslSocketLayer * LookupLayer(SSL * Ssl);

  bool m_bUseSSL{false};
  BOOL m_bFailureSent{FALSE};

  // Critical section for thread synchronization
  static std::unique_ptr<TCriticalSection> m_sCriticalSection;
  std::unique_ptr<TCriticalSection> m_CriticalSection;

  // Status variables
  static bool m_bSslInitialized;
  int m_nShutDown{0};
  int m_nNetworkError{0};
  int m_nSslAsyncNotifyId{0};
  bool m_bBlocking{false};
  bool m_bSslEstablished{false};
  CString m_CertStorage;
  int m_nVerificationResult{0};
  int m_nVerificationDepth{0};

  static struct t_SslLayerList //: public TObject
  {
  CUSTOM_MEM_ALLOCATION_IMPL
    CAsyncSslSocketLayer * pLayer{nullptr};
    t_SslLayerList * pNext{nullptr};
  } * m_pSslLayerList;

  // SSL data
  SSL_CTX* m_ssl_ctx{nullptr};  // SSL context
  SSL* m_ssl{nullptr};      // current session handle
  SSL_SESSION * m_sessionid{nullptr};
  int m_sessionidSerializedLen{0};
  unsigned char * m_sessionidSerialized{nullptr};
  bool m_sessionreuse{false};
  bool m_sessionreuse_failed{false};
  CAsyncSslSocketLayer * m_Main{nullptr};

  // Data channels for encrypted/unencrypted data
  BIO* m_nbio; // Network side, sends/receives encrypted data
  BIO* m_ibio; // Internal side, won't be used directly
  BIO* m_sslbio; // The data to encrypt / the decrypted data has to go though this bio

  // Send buffer
  char* m_pNetworkSendBuffer{nullptr};
  int m_nNetworkSendBufferLen{0};
  int m_nNetworkSendBufferMaxLen{0};

  char* m_pRetrySendBuffer{nullptr};
  int m_nRetrySendBufferLen{0};

  bool m_mayTriggerRead{false};
  bool m_mayTriggerWrite{false};
  bool m_mayTriggerReadUp{false};
  bool m_mayTriggerWriteUp{false};

  bool m_onCloseCalled{false};

  std::string m_TlsVersionStr;
  std::string m_CipherName;

  X509 * FCertificate{nullptr};
  EVP_PKEY * FPrivateKey{nullptr};
  CFileZillaTools * m_Tools{nullptr};
};

#define SSL_INFO 0
#define SSL_FAILURE 1
#define SSL_VERIFY_CERT 2

#define SSL_INFO_ESTABLISHED 0
#define SSL_INFO_SHUTDOWNCOMPLETE 1

#define SSL_FAILURE_UNKNOWN 0
#define SSL_FAILURE_ESTABLISH 1
#define SSL_FAILURE_INITSSL 4
#define SSL_FAILURE_VERIFYCERT 8
#define SSL_FAILURE_CERTREJECTED 16


