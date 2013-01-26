//---------------------------------------------------------------------------
#ifndef SecureShellH
#define SecureShellH

#include <set>
#include "PuttyIntf.h"
#include "Configuration.h"
#include "SessionData.h"
#include "SessionInfo.h"
//---------------------------------------------------------------------------
#ifndef PuttyIntfH
struct Backend;
struct Config;
#endif
//---------------------------------------------------------------------------
struct _WSANETWORKEVENTS;
typedef struct _WSANETWORKEVENTS WSANETWORKEVENTS;
typedef UINT_PTR SOCKET;
typedef std::set<SOCKET> TSockets;
struct TPuttyTranslation;
//---------------------------------------------------------------------------
class TSecureShell
{
friend class TPoolForDataEvent;

private:
  SOCKET FSocket;
  HANDLE FSocketEvent;
  TSockets FPortFwdSockets;
  TSessionUI * FUI;
  TSessionData * FSessionData;
  bool FActive;
  TSessionInfo FSessionInfo;
  bool FSessionInfoValid;
  TDateTime FLastDataSent;
  Backend * FBackend;
  void * FBackendHandle;
  const unsigned int * FMinPacketSize;
  const unsigned int * FMaxPacketSize;
  Config * FConfig;
  TNotifyEvent FOnReceive;
  bool FFrozen;
  bool FDataWhileFrozen;
  bool FStoredPasswordTried;
  bool FStoredPasswordTriedForKI;
  int FSshVersion;
  bool FOpened;
  int FWaiting;
  bool FSimple;

  intptr_t PendLen;
  intptr_t PendSize;
  intptr_t OutLen;
  unsigned char * OutPtr;
  unsigned char * Pending;
  TSessionLog * FLog;
  TConfiguration * FConfiguration;
  bool FAuthenticating;
  bool FAuthenticated;
  UnicodeString FStdErrorTemp;
  UnicodeString FStdError;
  UnicodeString FCWriteTemp;
  UnicodeString FAuthenticationLog;
  UnicodeString FLastTunnelError;
  UnicodeString FUserName;

  static TCipher FuncToSsh1Cipher(const void * Cipher);
  static TCipher FuncToSsh2Cipher(const void * Cipher);
  UnicodeString FuncToCompression(int SshVersion, const void * Compress) const;
  void Init();
  void SetActive(bool value);
  void inline CheckConnection(int Message = -1);
  void WaitForData();
  void Discard();
  void FreeBackend();
  void PoolForData(WSANETWORKEVENTS & Events, unsigned int & Result);
  inline void CaptureOutput(TLogLineType Type,
    const UnicodeString & Line);
  void ResetConnection();
  void ResetSessionInfo();
  void SocketEventSelect(SOCKET Socket, HANDLE Event, bool Startup);
  bool EnumNetworkEvents(SOCKET Socket, WSANETWORKEVENTS & Events);
  void HandleNetworkEvents(SOCKET Socket, WSANETWORKEVENTS & Events);
  bool ProcessNetworkEvents(SOCKET Socket);
  bool EventSelectLoop(unsigned int MSec, bool ReadEventRequired,
    WSANETWORKEVENTS * Events);
  void UpdateSessionInfo();
  void DispatchSendBuffer(int BufSize);
  void SendBuffer(unsigned int & Result);
  unsigned int TimeoutPrompt(TQueryParamsTimerEvent PoolEvent);

protected:
  TCaptureOutputEvent FOnCaptureOutput;

  void GotHostKey();
  int TranslatePuttyMessage(const TPuttyTranslation * Translation,
    intptr_t Count, UnicodeString & Message) const;
  int TranslateAuthenticationMessage(UnicodeString & Message) const;
  int TranslateErrorMessage(UnicodeString & Message) const;
  void AddStdError(const UnicodeString & Str);
  void AddStdErrorLine(const UnicodeString & Str);
  void FatalError(Exception * E, const UnicodeString & Msg);
  void LogEvent(const UnicodeString & Str);
  void FatalError(const UnicodeString & Error);
  static void ClearConfig(Config * cfg);
  static void StoreToConfig(TSessionData * Data, Config * cfg, bool Simple);

public:
  explicit TSecureShell(TSessionUI * UI, TSessionData * SessionData,
    TSessionLog * Log, TConfiguration * Configuration);
  virtual ~TSecureShell();
  void Open();
  void Close();
  void KeepAlive();
  intptr_t Receive(unsigned char * Buf, intptr_t Len);
  bool Peek(unsigned char *& Buf, intptr_t Len) const;
  UnicodeString ReceiveLine();
  void Send(const unsigned char * Buf, intptr_t Len);
  void SendStr(const UnicodeString & Str);
  void SendSpecial(int Code);
  void Idle(unsigned int MSec = 0);
  void SendEOF();
  void SendLine(const UnicodeString & Line);
  void SendNull();

  const TSessionInfo & GetSessionInfo();
  bool SshFallbackCmd() const;
  unsigned long MinPacketSize();
  unsigned long MaxPacketSize();
  void ClearStdError();
  bool GetStoredCredentialsTried() const;

  void RegisterReceiveHandler(TNotifyEvent Handler);
  void UnregisterReceiveHandler(TNotifyEvent Handler);

  // interface to PuTTY core
  void UpdateSocket(SOCKET value, bool Startup);
  void UpdatePortFwdSocket(SOCKET value, bool Startup);
  void PuttyFatalError(const UnicodeString & Error);
  bool PromptUser(bool ToServer,
    const UnicodeString & AName, bool NameRequired,
    const UnicodeString & Instructions, bool InstructionsRequired,
    TStrings * Prompts, TStrings * Results);
  void FromBackend(bool IsStdErr, const unsigned char * Data, intptr_t Length);
  void CWrite(const char * Data, intptr_t Length);
  const UnicodeString & GetStdError() const;
  void VerifyHostKey(const UnicodeString & Host, int Port,
    const UnicodeString & KeyType, const UnicodeString & KeyStr, const UnicodeString & Fingerprint);
  void AskAlg(const UnicodeString & AlgType, const UnicodeString & AlgName);
  void DisplayBanner(const UnicodeString & Banner);
  void OldKeyfileWarning();
  void PuttyLogEvent(const UnicodeString & Str);

  bool GetReady() const;
  bool GetActive() const { return FActive; }
  TCaptureOutputEvent & GetOnCaptureOutput() { return FOnCaptureOutput; }
  void SetOnCaptureOutput(TCaptureOutputEvent value) { FOnCaptureOutput = value; }
  TDateTime GetLastDataSent() const { return FLastDataSent; }
  UnicodeString GetLastTunnelError() const { return FLastTunnelError; }
  UnicodeString GetUserName() const { return FUserName; }
  bool GetSimple() const { return FSimple; }
  void SetSimple(bool value) { FSimple = value; }
private:
  TSecureShell(const TSecureShell &);
  TSecureShell & operator=(const TSecureShell &);
};
//---------------------------------------------------------------------------
#endif
