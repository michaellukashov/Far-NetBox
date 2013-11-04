//---------------------------------------------------------------------------
#ifndef SecureShellH
#define SecureShellH

#include "PuttyIntf.h"
#include "Configuration.h"
#include "SessionData.h"
#include "SessionInfo.h"
//---------------------------------------------------------------------------
#ifndef PuttyIntfH
struct Backend;
struct Conf;
#endif
//---------------------------------------------------------------------------
struct _WSANETWORKEVENTS;
typedef struct _WSANETWORKEVENTS WSANETWORKEVENTS;
typedef UINT_PTR SOCKET;
typedef rde::vector<SOCKET> TSockets;
struct TPuttyTranslation;
//---------------------------------------------------------------------------
class TSecureShell : public TObject
{
friend class TPoolForDataEvent;
public:
  explicit TSecureShell(TSessionUI * UI, TSessionData * SessionData,
    TSessionLog * Log, TConfiguration * Configuration);
  virtual ~TSecureShell();
  void Open();
  void Close();
  void KeepAlive();
  intptr_t Receive(unsigned char * Buf, intptr_t Length);
  bool Peek(unsigned char *& Buf, intptr_t Length) const;
  UnicodeString ReceiveLine();
  void Send(const unsigned char * Buf, intptr_t Length);
  void SendStr(const UnicodeString & Str);
  void SendSpecial(int Code);
  void Idle(uintptr_t MSec = 0);
  void SendEOF();
  void SendLine(const UnicodeString & Line);
  void SendNull();

  const TSessionInfo & GetSessionInfo() const;
  bool SshFallbackCmd() const;
  unsigned long MinPacketSize();
  unsigned long MaxPacketSize();
  void ClearStdError();
  bool GetStoredCredentialsTried() const;
  void CollectUsage();

  void RegisterReceiveHandler(TNotifyEvent Handler);
  void UnregisterReceiveHandler(TNotifyEvent Handler);

  // interface to PuTTY core
  void UpdateSocket(SOCKET Value, bool Startup);
  void UpdatePortFwdSocket(SOCKET Value, bool Startup);
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
  void SetOnCaptureOutput(TCaptureOutputEvent Value) { FOnCaptureOutput = Value; }
  TDateTime GetLastDataSent() const { return FLastDataSent; }
  UnicodeString GetLastTunnelError() const { return FLastTunnelError; }
  UnicodeString GetUserName() const { return FUserName; }
  bool GetSimple() const { return FSimple; }
  void SetSimple(bool Value) { FSimple = Value; }

protected:
  TCaptureOutputEvent FOnCaptureOutput;

  void GotHostKey();
  int TranslatePuttyMessage(const TPuttyTranslation * Translation,
    intptr_t Count, UnicodeString & Message, UnicodeString * HelpKeyword = nullptr) const;
  int TranslateAuthenticationMessage(UnicodeString & Message, UnicodeString * HelpKeyword = nullptr);
  int TranslateErrorMessage(UnicodeString & Message, UnicodeString * HelpKeyword = nullptr);
  void AddStdError(const UnicodeString & Str);
  void AddStdErrorLine(const UnicodeString & Str);
  void LogEvent(const UnicodeString & Str);
  // void FatalError(Exception * E, const UnicodeString & Msg);
  void FatalError(const UnicodeString & Error, const UnicodeString & HelpKeyword = L"");
  UnicodeString FormatKeyStr(const UnicodeString & KeyStr) const;
  static Conf * StoreToConfig(TSessionData * Data, bool Simple);

private:
  static TCipher FuncToSsh1Cipher(const void * Cipher);
  static TCipher FuncToSsh2Cipher(const void * Cipher);
  UnicodeString FuncToCompression(int SshVersion, const void * Compress) const;
  void Init();
  void SetActive(bool Value);
  void inline CheckConnection(int Message = -1);
  void WaitForData();
  void Discard();
  void FreeBackend();
  void PoolForData(WSANETWORKEVENTS & Events, intptr_t & Result);
  inline void CaptureOutput(TLogLineType Type,
    const UnicodeString & Line);
  void ResetConnection();
  void ResetSessionInfo();
  void SocketEventSelect(SOCKET Socket, HANDLE Event, bool Startup);
  bool EnumNetworkEvents(SOCKET Socket, WSANETWORKEVENTS & Events);
  void HandleNetworkEvents(SOCKET Socket, WSANETWORKEVENTS & Events);
  bool ProcessNetworkEvents(SOCKET Socket);
  bool EventSelectLoop(uintptr_t MSec, bool ReadEventRequired,
    WSANETWORKEVENTS * Events);
  void UpdateSessionInfo() const;
  // bool GetReady();
  void DispatchSendBuffer(intptr_t BufSize);
  void SendBuffer(intptr_t & Result);
  uintptr_t TimeoutPrompt(TQueryParamsTimerEvent PoolEvent);
  bool TryFtp();

private:
  SOCKET FSocket;
  HANDLE FSocketEvent;
  TSockets FPortFwdSockets;
  TSessionUI * FUI;
  TSessionData * FSessionData;
  bool FActive;
  mutable TSessionInfo FSessionInfo;
  mutable bool FSessionInfoValid;
  TDateTime FLastDataSent;
  Backend * FBackend;
  void * FBackendHandle;
  const unsigned int * FMinPacketSize;
  const unsigned int * FMaxPacketSize;
  TNotifyEvent FOnReceive;
  bool FFrozen;
  bool FDataWhileFrozen;
  bool FStoredPasswordTried;
  bool FStoredPasswordTriedForKI;
  mutable int FSshVersion;
  bool FOpened;
  int FWaiting;
  bool FSimple;
  bool FNoConnectionResponse;
  bool FCollectPrivateKeyUsage;
  int FWaitingForData;

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

private:
  NB_DISABLE_COPY(TSecureShell)
};
//---------------------------------------------------------------------------
#endif
