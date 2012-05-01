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
  notify_signal_type FOnReceive;
  bool FFrozen;
  bool FDataWhileFrozen;
  bool FStoredPasswordTried;
  bool FStoredPasswordTriedForKI;
  int FSshVersion;
  bool FOpened;
  int FWaiting;
  bool FSimple;

  unsigned PendLen;
  unsigned PendSize;
  unsigned OutLen;
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
  TSecureShell * Self;

  static TCipher FuncToSsh1Cipher(const void * Cipher);
  static TCipher FuncToSsh2Cipher(const void * Cipher);
  UnicodeString FuncToCompression(int SshVersion, const void * Compress) const;
  void Init();
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
  unsigned int TimeoutPrompt(TQueryParamsTimerEvent * PoolEvent);

protected:
  captureoutput_signal_type FOnCaptureOutput;

  void GotHostKey();
  int TranslatePuttyMessage(const TPuttyTranslation * Translation,
    size_t Count, UnicodeString & Message);
  int TranslateAuthenticationMessage(UnicodeString & Message);
  int TranslateErrorMessage(UnicodeString & Message);
  void AddStdError(UnicodeString Str);
  void AddStdErrorLine(const UnicodeString & Str);
  void FatalError(Exception * E, UnicodeString Msg);
  void inline LogEvent(const UnicodeString & Str);
  void FatalError(UnicodeString Error);
  static void ClearConfig(Config * cfg);
  static void StoreToConfig(TSessionData * Data, Config * cfg, bool Simple);

public:
  explicit TSecureShell(TSessionUI * UI, TSessionData * SessionData,
    TSessionLog * Log, TConfiguration * Configuration);
  virtual ~TSecureShell();
  void Open();
  void Close();
  void KeepAlive();
  int Receive(unsigned char * Buf, int Len);
  bool Peek(unsigned char *& Buf, int Len);
  UnicodeString ReceiveLine();
  void Send(const unsigned char * Buf, int Len);
  void SendStr(UnicodeString Str);
  void SendSpecial(int Code);
  void Idle(unsigned int MSec = 0);
  void SendEOF();
  void SendLine(UnicodeString Line);
  void SendNull();

  const TSessionInfo & GetSessionInfo();
  bool SshFallbackCmd() const;
  unsigned int MinPacketSize();
  unsigned int MaxPacketSize();
  void ClearStdError();
  bool GetStoredCredentialsTried();

  void RegisterReceiveHandler(const TNotifyEvent & Handler);
  void UnregisterReceiveHandler(const TNotifyEvent & Handler);

  // interface to PuTTY core
  void UpdateSocket(SOCKET value, bool Startup);
  void UpdatePortFwdSocket(SOCKET value, bool Startup);
  void PuttyFatalError(UnicodeString Error);
  bool PromptUser(bool ToServer,
    UnicodeString AName, bool NameRequired,
    UnicodeString Instructions, bool InstructionsRequired,
    TStrings * Prompts, TStrings * Results);
  void FromBackend(bool IsStdErr, const unsigned char * Data, int Length);
  void CWrite(const char * Data, int Length);
  const UnicodeString GetStdError();
  void VerifyHostKey(UnicodeString Host, int Port,
    const UnicodeString KeyType, UnicodeString KeyStr, const UnicodeString Fingerprint);
  void AskAlg(const UnicodeString AlgType, const UnicodeString AlgName);
  void __fastcall DisplayBanner(const UnicodeString & Banner);
  void OldKeyfileWarning();
  void PuttyLogEvent(const UnicodeString & Str);

#ifndef _MSC_VER
  __property bool Active = { read = FActive, write = SetActive };
  __property bool Ready = { read = GetReady };
  __property TCaptureOutputEvent OnCaptureOutput = { read = FOnCaptureOutput, write = FOnCaptureOutput };
  __property TDateTime LastDataSent = { read = FLastDataSent };
  __property UnicodeString LastTunnelError = { read = FLastTunnelError };
  __property UnicodeString UserName = { read = FUserName };
  __property bool Simple = { read = FSimple, write = FSimple };
#else
  void __fastcall SetActive(bool value);
  bool __fastcall GetActive() { return FActive; }
  bool __fastcall GetReady();
  captureoutput_signal_type & GetOnCaptureOutput() { return FOnCaptureOutput; }
  void SetOnCaptureOutput(const TCaptureOutputEvent & value) { FOnCaptureOutput.connect(value); }
  TDateTime GetLastDataSent() { return FLastDataSent; }
  UnicodeString GetLastTunnelError() { return FLastTunnelError; }
  UnicodeString GetUserName() { return FUserName; }
  bool GetSimple() { return FSimple; }
  void SetSimple(bool value) { FSimple = value; }
#endif
private:
  TSecureShell(const TSecureShell &);
  void operator=(const TSecureShell &);
};
//---------------------------------------------------------------------------
#endif
