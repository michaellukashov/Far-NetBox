
#pragma once

#include "PuttyIntf.h"
#include "Configuration.h"
#include "SessionData.h"
#include "SessionInfo.h"

#ifndef PuttyIntfH
struct Backend;
struct Conf;
#endif

struct _WSANETWORKEVENTS;
typedef struct _WSANETWORKEVENTS WSANETWORKEVENTS;
typedef UINT_PTR SOCKET;
typedef rde::vector<SOCKET> TSockets;
struct TPuttyTranslation;

enum TSshImplementation
{
  sshiUnknown,
  sshiOpenSSH,
  sshiProFTPD,
  sshiBitvise,
  sshiTitan,
  sshiOpenVMS,
  sshiCerberus,
};

class TSecureShell : public TObject
{
friend class TPoolForDataEvent;
NB_DISABLE_COPY(TSecureShell)
NB_DECLARE_CLASS(TSecureShell)
public:
  explicit TSecureShell(TSessionUI * UI, TSessionData * SessionData,
    TSessionLog * Log, TConfiguration * Configuration);
  virtual ~TSecureShell();
  void Open();
  void Close();
  void KeepAlive();
  intptr_t Receive(uint8_t * Buf, intptr_t Length);
  bool Peek(uint8_t *& Buf, intptr_t Length) const;
  UnicodeString ReceiveLine();
  void Send(const uint8_t * Buf, intptr_t Length);
  // void SendStr(const UnicodeString & Str);
  void SendSpecial(int Code);
  void Idle(uintptr_t MSec = 0);
  void SendEOF();
  void SendLine(const UnicodeString & Line);
  void SendNull();

  const TSessionInfo & GetSessionInfo() const;
  bool SshFallbackCmd() const;
  uint32_t MinPacketSize() const;
  uint32_t MaxPacketSize() const;
  void ClearStdError();
  bool GetStoredCredentialsTried() const;
  void CollectUsage();
  TSshImplementation GetSshImplementation() const { return FSshImplementation; }
  bool GetUtfStrings() const { return FUtfStrings; }
  void SetUtfStrings(bool Value) { FUtfStrings = Value; }

  void RegisterReceiveHandler(TNotifyEvent Handler);
  void UnregisterReceiveHandler(TNotifyEvent Handler);

  // interface to PuTTY core
  void UpdateSocket(SOCKET Value, bool Startup);
  void UpdatePortFwdSocket(SOCKET Value, bool Startup);
  void PuttyFatalError(const UnicodeString & Error);
  TPromptKind IdentifyPromptKind(UnicodeString & AName);
  bool PromptUser(bool ToServer,
    const UnicodeString & AName, bool NameRequired,
    const UnicodeString & Instructions, bool InstructionsRequired,
    TStrings * Prompts, TStrings * Results);
  void FromBackend(bool IsStdErr, const uint8_t * Data, intptr_t Length);
  void CWrite(const char * Data, intptr_t Length);
  const UnicodeString & GetStdError() const;
  void VerifyHostKey(const UnicodeString & Host, intptr_t Port,
    const UnicodeString & KeyType, const UnicodeString & KeyStr, const UnicodeString & Fingerprint);
  bool HaveHostKey(const UnicodeString & Host, intptr_t Port, const UnicodeString & KeyType);
  void AskAlg(const UnicodeString & AlgType, const UnicodeString & AlgName);
  void DisplayBanner(const UnicodeString & Banner);
  void OldKeyfileWarning();
  void PuttyLogEvent(const char * AStr);
  UnicodeString ConvertFromPutty(const char * Str, size_t Length) const;

  bool GetReady() const;
  bool GetActive() const { return FActive; }
  const TCaptureOutputEvent & GetOnCaptureOutput() const { return FOnCaptureOutput; }
  void SetOnCaptureOutput(TCaptureOutputEvent Value) { FOnCaptureOutput = Value; }
  TDateTime GetLastDataSent() const { return FLastDataSent; }
  UnicodeString GetLastTunnelError() const { return FLastTunnelError; }
  UnicodeString ShellGetUserName() const { return FUserName; }
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
  UnicodeString ConvertInput(const RawByteString & Input, uintptr_t CodePage = CP_ACP) const;
  void GetRealHost(UnicodeString & Host, intptr_t & Port);
  UnicodeString RetrieveHostKey(const UnicodeString & Host, intptr_t Port, const UnicodeString & KeyType);

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
  mutable const uint32_t * FMinPacketSize;
  mutable const uint32_t * FMaxPacketSize;
  TNotifyEvent FOnReceive;
  bool FFrozen;
  bool FDataWhileFrozen;
  bool FStoredPasswordTried;
  bool FStoredPasswordTriedForKI;
  bool FStoredPassphraseTried;
  mutable int FSshVersion;
  bool FOpened;
  int FWaiting;
  bool FSimple;
  bool FNoConnectionResponse;
  bool FCollectPrivateKeyUsage;
  int FWaitingForData;
  TSshImplementation FSshImplementation;

  intptr_t PendLen;
  intptr_t PendSize;
  intptr_t OutLen;
  uint8_t * OutPtr;
  uint8_t * Pending;
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
  bool FUtfStrings;

/*
  static TCipher __fastcall FuncToSsh1Cipher(const void * Cipher);
  static TCipher __fastcall FuncToSsh2Cipher(const void * Cipher);
  UnicodeString __fastcall FuncToCompression(int SshVersion, const void * Compress) const;
  void __fastcall Init();
  void __fastcall SetActive(bool value);
  void inline __fastcall CheckConnection(int Message = -1);
  void __fastcall WaitForData();
  void __fastcall Discard();
  void __fastcall FreeBackend();
  void __fastcall PoolForData(WSANETWORKEVENTS & Events, unsigned int & Result);
  inline void __fastcall CaptureOutput(TLogLineType Type,
    const UnicodeString & Line);
  void __fastcall ResetConnection();
  void __fastcall ResetSessionInfo();
  void __fastcall SocketEventSelect(SOCKET Socket, HANDLE Event, bool Startup);
  bool __fastcall EnumNetworkEvents(SOCKET Socket, WSANETWORKEVENTS & Events);
  void __fastcall HandleNetworkEvents(SOCKET Socket, WSANETWORKEVENTS & Events);
  bool __fastcall ProcessNetworkEvents(SOCKET Socket);
  bool __fastcall EventSelectLoop(unsigned int MSec, bool ReadEventRequired,
    WSANETWORKEVENTS * Events);
  void __fastcall UpdateSessionInfo();
  bool __fastcall GetReady();
  void __fastcall DispatchSendBuffer(int BufSize);
  void __fastcall SendBuffer(unsigned int & Result);
  unsigned int __fastcall TimeoutPrompt(TQueryParamsTimerEvent PoolEvent);
  bool __fastcall TryFtp();
  UnicodeString __fastcall ConvertInput(const RawByteString & Input);
  void __fastcall GetRealHost(UnicodeString & Host, int & Port);
  UnicodeString __fastcall RetrieveHostKey(UnicodeString Host, int Port, const UnicodeString KeyType);

protected:
  TCaptureOutputEvent FOnCaptureOutput;

  void __fastcall GotHostKey();
  int __fastcall TranslatePuttyMessage(const TPuttyTranslation * Translation,
    size_t Count, UnicodeString & Message, UnicodeString * HelpKeyword = nullptr);
  int __fastcall TranslateAuthenticationMessage(UnicodeString & Message, UnicodeString * HelpKeyword = nullptr);
  int __fastcall TranslateErrorMessage(UnicodeString & Message, UnicodeString * HelpKeyword = nullptr);
  void __fastcall AddStdError(UnicodeString Str);
  void __fastcall AddStdErrorLine(const UnicodeString & Str);
  void __fastcall inline LogEvent(const UnicodeString & Str);
  void __fastcall FatalError(UnicodeString Error, UnicodeString HelpKeyword = L"");
  UnicodeString __fastcall FormatKeyStr(UnicodeString KeyStr);
  static Conf * __fastcall StoreToConfig(TSessionData * Data, bool Simple);

public:
  __fastcall TSecureShell(TSessionUI * UI, TSessionData * SessionData,
    TSessionLog * Log, TConfiguration * Configuration);
  __fastcall ~TSecureShell();
  void __fastcall Open();
  void __fastcall Close();
  void __fastcall KeepAlive();
  int __fastcall Receive(unsigned char * Buf, int Len);
  bool __fastcall Peek(unsigned char *& Buf, int Len);
  UnicodeString __fastcall ReceiveLine();
  void __fastcall Send(const unsigned char * Buf, int Len);
  void __fastcall SendSpecial(int Code);
  void __fastcall Idle(unsigned int MSec = 0);
  void __fastcall SendEOF();
  void __fastcall SendLine(const UnicodeString & Line);
  void __fastcall SendNull();

  const TSessionInfo & __fastcall GetSessionInfo();
  bool __fastcall SshFallbackCmd() const;
  unsigned long __fastcall MaxPacketSize();
  void __fastcall ClearStdError();
  bool __fastcall GetStoredCredentialsTried();
  void __fastcall CollectUsage();

  void __fastcall RegisterReceiveHandler(TNotifyEvent Handler);
  void __fastcall UnregisterReceiveHandler(TNotifyEvent Handler);

  // interface to PuTTY core
  void __fastcall UpdateSocket(SOCKET value, bool Startup);
  void __fastcall UpdatePortFwdSocket(SOCKET value, bool Startup);
  void __fastcall PuttyFatalError(UnicodeString Error);
  TPromptKind __fastcall IdentifyPromptKind(UnicodeString & Name);
  bool __fastcall PromptUser(bool ToServer,
    UnicodeString AName, bool NameRequired,
    UnicodeString Instructions, bool InstructionsRequired,
    TStrings * Prompts, TStrings * Results);
  void __fastcall FromBackend(bool IsStdErr, const unsigned char * Data, int Length);
  void __fastcall CWrite(const char * Data, int Length);
  const UnicodeString & __fastcall GetStdError();
  void __fastcall VerifyHostKey(UnicodeString Host, int Port,
    const UnicodeString KeyType, UnicodeString KeyStr, UnicodeString Fingerprint);
  bool __fastcall HaveHostKey(UnicodeString Host, int Port, const UnicodeString KeyType);
  void __fastcall AskAlg(const UnicodeString AlgType, const UnicodeString AlgName);
  void __fastcall DisplayBanner(const UnicodeString & Banner);
  void __fastcall OldKeyfileWarning();
  void __fastcall PuttyLogEvent(const char * Str);
  UnicodeString __fastcall ConvertFromPutty(const char * Str, int Length);

  __property bool Active = { read = FActive, write = SetActive };
  __property bool Ready = { read = GetReady };
  __property TCaptureOutputEvent OnCaptureOutput = { read = FOnCaptureOutput, write = FOnCaptureOutput };
  __property TDateTime LastDataSent = { read = FLastDataSent };
  __property UnicodeString LastTunnelError = { read = FLastTunnelError };
  __property UnicodeString UserName = { read = FUserName };
  __property bool Simple = { read = FSimple, write = FSimple };
  __property TSshImplementation SshImplementation = { read = FSshImplementation };
  __property bool UtfStrings = { read = FUtfStrings, write = FUtfStrings };
*/
};

