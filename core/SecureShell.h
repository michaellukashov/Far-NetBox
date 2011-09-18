//---------------------------------------------------------------------------
#ifndef SecureShellH
#define SecureShellH

#include <set>
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

  unsigned PendLen;
  unsigned PendSize;
  unsigned OutLen;
  char * OutPtr;
  char * Pending;
  TSessionLog * FLog;
  TConfiguration * FConfiguration;
  bool FAuthenticating;
  bool FAuthenticated;
  std::wstring FStdErrorTemp;
  std::wstring FStdError;
  std::wstring FCWriteTemp;
  std::wstring FAuthenticationLog;
  std::wstring FLastTunnelError;
  std::wstring FUserName;

  static TCipher FuncToSsh1Cipher(const void * Cipher);
  static TCipher FuncToSsh2Cipher(const void * Cipher);
  std::wstring FuncToCompression(int SshVersion, const void * Compress) const;
  void Init();
  void inline CheckConnection(int Message = -1);
  void WaitForData();
  void Discard();
  void FreeBackend();
  void PoolForData(WSANETWORKEVENTS & Events, unsigned int & Result);
  inline void CaptureOutput(TLogLineType Type,
    const std::wstring & Line);
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
  int TimeoutPrompt(TQueryParamsTimerEvent PoolEvent);

protected:
  TCaptureOutputEvent FOnCaptureOutput;

  void GotHostKey();
  int TranslatePuttyMessage(const TPuttyTranslation * Translation,
    size_t Count, std::wstring & Message);
  int TranslateAuthenticationMessage(std::wstring & Message);
  int TranslateErrorMessage(std::wstring & Message);
  void AddStdError(std::wstring Str);
  void AddStdErrorLine(const std::wstring & Str);
  void FatalError(const std::exception * E, std::wstring Msg);
  void inline LogEvent(const std::wstring & Str);
  void FatalError(std::wstring Error);
  static void ClearConfig(Config * cfg);
  static void StoreToConfig(TSessionData * Data, Config * cfg, bool Simple);

public:
  TSecureShell(TSessionUI * UI, TSessionData * SessionData,
    TSessionLog * Log, TConfiguration * Configuration);
  ~TSecureShell();
  void Open();
  void Close();
  void KeepAlive();
  int Receive(char * Buf, int Len);
  bool Peek(char *& Buf, int Len);
  std::wstring ReceiveLine();
  void Send(const char * Buf, int Len);
  void SendStr(std::wstring Str);
  void SendSpecial(int Code);
  void Idle(unsigned int MSec = 0);
  void SendEOF();
  void SendLine(std::wstring Line);
  void SendNull();

  const TSessionInfo & GetSessionInfo();
  bool SshFallbackCmd() const;
  unsigned long MaxPacketSize();
  void ClearStdError();
  bool GetStoredCredentialsTried();

  void RegisterReceiveHandler(TNotifyEvent Handler);
  void UnregisterReceiveHandler(TNotifyEvent Handler);

  // interface to PuTTY core
  void UpdateSocket(SOCKET value, bool Startup);
  void UpdatePortFwdSocket(SOCKET value, bool Startup);
  void PuttyFatalError(std::wstring Error);
  bool PromptUser(bool ToServer,
    std::wstring AName, bool NameRequired,
    std::wstring Instructions, bool InstructionsRequired,
    TStrings * Prompts, TStrings * Results);
  void FromBackend(bool IsStdErr, const char * Data, int Length);
  void CWrite(const char * Data, int Length);
  const std::wstring & GetStdError();
  void VerifyHostKey(std::wstring Host, int Port,
    const std::wstring KeyType, std::wstring KeyStr, const std::wstring Fingerprint);
  void AskAlg(const std::wstring AlgType, const std::wstring AlgName);
  void DisplayBanner(const std::wstring & Banner);
  void OldKeyfileWarning();
  void PuttyLogEvent(const std::wstring & Str);

  // __property bool Active = { read = FActive, write = SetActive };
  bool GetActive() { return FActive; }
  void SetActive(bool value);
  // __property bool Ready = { read = GetReady };
  bool GetReady();
  // __property TCaptureOutputEvent OnCaptureOutput = { read = FOnCaptureOutput, write = FOnCaptureOutput };
  TCaptureOutputEvent GetOnCaptureOutput() { return FOnCaptureOutput; }
  void SetOnCaptureOutput(TCaptureOutputEvent value) { FOnCaptureOutput = value; }
  // __property TDateTime LastDataSent = { read = FLastDataSent };
  TDateTime GetLastDataSent() { return FLastDataSent; }
  // __property std::wstring LastTunnelError = { read = FLastTunnelError };
  std::wstring GetLastTunnelError() { return FLastTunnelError; }
  // __property std::wstring UserName = { read = FUserName };
  std::wstring GetUserName() { return FUserName; }
  // __property bool Simple = { read = FSimple, write = FSimple };
  bool GetSimple() { return FSimple; }
  void SetSimple(bool value) { FSimple = value; }
};
//---------------------------------------------------------------------------
#endif
