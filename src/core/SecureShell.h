﻿
#pragma once

#include <rdestl/vector.h>
#include "PuttyIntf.h"
#include "Configuration.h"
#include "SessionData.h"
#include "SessionInfo.h"

#ifndef PuttyIntfH
__removed struct Backend_vtable;
__removed struct Conf;
struct Backend;
#endif

struct _WSANETWORKEVENTS;
typedef struct _WSANETWORKEVENTS WSANETWORKEVENTS;
using SOCKET = UINT_PTR;
using TSockets = nb::vector_t<SOCKET>;
struct TPuttyTranslation;
struct callback_set;
enum TSshImplementation { sshiUnknown, sshiOpenSSH, sshiProFTPD, sshiBitvise, sshiTitan, sshiOpenVMS, sshiCerberus };
struct ScpLogPolicy;
struct LogContext;
struct ScpSeat;

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

NB_DEFINE_CLASS_ID(TSecureShell);
class TSecureShell : public TObject
{
  friend class TPoolForDataEvent;
  NB_DISABLE_COPY(TSecureShell)
public:
  static bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TSecureShell); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TSecureShell) || TObject::is(Kind); }
private:
  SOCKET FSocket{INVALID_SOCKET};
  HANDLE FSocketEvent{};
  TSockets FPortFwdSockets;
  TSessionUI * FUI{nullptr};
  TSessionData * FSessionData{nullptr};
  bool FActive{false};
  mutable TSessionInfo FSessionInfo{};
  mutable bool FSessionInfoValid{false};
  TDateTime FLastDataSent{};
  Backend * FBackendHandle{nullptr};
  void *FBackendHandle{nullptr};
  mutable const uint32_t *FMinPacketSize{nullptr};
  mutable const uint32_t *FMaxPacketSize{nullptr};
  TNotifyEvent FOnReceive{nullptr};
  bool FFrozen{false};
  bool FDataWhileFrozen{false};
  bool FStoredPasswordTried{false};
  bool FStoredPasswordTriedForKI{false};
  bool FStoredPassphraseTried{false};
  bool FOpened{false};
  bool FClosed{false};
  int32_t FWaiting{0};
  bool FSimple{false};
  bool FNoConnectionResponse{false};
  bool FCollectPrivateKeyUsage{false};
  intptr_t FWaitingForData{0};
  TSshImplementation FSshImplementation{sshiUnknown};

  int32_t PendLen{0};
  int32_t PendSize{0};
  int32_t OutLen{0};
  uint8_t * OutPtr{nullptr};
  uint8_t * Pending{nullptr};
  TSessionLog * FLog{nullptr};
  TConfiguration * FConfiguration{nullptr};
  bool FAuthenticating{false};
  bool FAuthenticated{false};
  UnicodeString FStdErrorTemp;
  UnicodeString FStdError;
  UnicodeString FCWriteTemp;
  UnicodeString FAuthenticationLog;
  UnicodeString FLastTunnelError;
  UnicodeString FUserName;
  bool FUtfStrings{false};
  DWORD FLastSendBufferUpdate{0};
  int32_t FSendBuf{0};
  std::auto_ptr<callback_set> FCallbackSet;
  ScpLogPolicy * FLogPolicy;
  ScpSeat * FSeat;
  LogContext * FLogCtx;
  std::set<UnicodeString> FLoggedKnownHostKeys;

public:
  void Init();
  void SetActive(bool Value);
  void inline CheckConnection(int Message = -1);
  void WaitForData();
  void Discard();
  void FreeBackend();
  void PoolForData(WSANETWORKEVENTS & Events, uint32_t & Result);
  void CaptureOutput(TLogLineType Type,
    const UnicodeString & Line);
  void ResetConnection();
  void ResetSessionInfo();
  void SocketEventSelect(SOCKET Socket, HANDLE Event, bool Enable);
  bool EnumNetworkEvents(SOCKET Socket, WSANETWORKEVENTS & Events);
  void HandleNetworkEvents(SOCKET Socket, WSANETWORKEVENTS & Events);
  bool ProcessNetworkEvents(SOCKET Socket);
  bool EventSelectLoop(uint32_t MSec, bool ReadEventRequired,
    WSANETWORKEVENTS *Events);
  void UpdateSessionInfo() const;
  bool GetReady() const;
  void DispatchSendBuffer(intptr_t BufSize);
  void SendBuffer(uint32_t &Result);
  uint32_t TimeoutPrompt(TQueryParamsTimerEvent PoolEvent);
  void TimeoutAbort(unsigned int Answer);
  bool TryFtp();
  UnicodeString ConvertInput(RawByteString Input, uintptr_t CodePage = CP_ACP) const;
  void GetRealHost(UnicodeString &Host, intptr_t &Port) const;
  UnicodeString RetrieveHostKey(const UnicodeString & Host, int32_t Port, const UnicodeString & KeyType) const;
  bool HaveAcceptNewHostKeyPolicy() const;
  THierarchicalStorage * GetHostKeyStorage();
  bool VerifyCachedHostKey(
    const UnicodeString & StoredKeys, const UnicodeString & KeyStr, const UnicodeString & FingerprintMD5, const UnicodeString & FingerprintSHA256);
  UnicodeString StoreHostKey(
    const UnicodeString & Host, int Port, const UnicodeString & KeyType, const UnicodeString & KeyStr);
  bool HasLocalProxy() const;

protected:
  TCaptureOutputEvent FOnCaptureOutput;

  void GotHostKey();
  int TranslatePuttyMessage(const TPuttyTranslation *Translation,
    intptr_t Count, UnicodeString &Message, UnicodeString *HelpKeyword = nullptr) const;
  int TranslateAuthenticationMessage(UnicodeString &Message, UnicodeString *HelpKeyword = nullptr);
  int TranslateErrorMessage(UnicodeString &Message, UnicodeString *HelpKeyword = nullptr);
  void AddStdErrorLine(const UnicodeString & AStr);
  void LogEvent(const UnicodeString & AStr);
  void FatalError(UnicodeString Error, UnicodeString HelpKeyword = "");
  UnicodeString FormatKeyStr(UnicodeString AKeyStr) const;
  void ParseFingerprint(const UnicodeString & Fingerprint, UnicodeString & SignKeyType, UnicodeString & Hash);
  static Conf *StoreToConfig(TSessionData *Data, bool Simple);

public:
  explicit TSecureShell(TSessionUI * UI, TSessionData * SessionData,
    TSessionLog *Log, TConfiguration *Configuration) noexcept;
  virtual ~TSecureShell() noexcept;
  void Open();
  void Close();
  void KeepAlive();
  intptr_t Receive(uint8_t *Buf, int32_t Length);
  bool Peek(uint8_t *& Buf, int32_t Length) const;
  UnicodeString ReceiveLine();
  void Send(const uint8_t *Buf, intptr_t Length);
  void SendSpecial(int32_t Code);
  void Idle(uint32_t MSec = 0);
  void SendLine(const UnicodeString & Line);
  void SendNull();

  const TSessionInfo &GetSessionInfo() const;
  void GetHostKeyFingerprint(UnicodeString & SHA256, UnicodeString & MD5) const;
  bool SshFallbackCmd() const;
  uint32_t MinPacketSize() const;
  uint32_t MaxPacketSize() const;
  void ClearStdError();
  bool GetStoredCredentialsTried() const;
  void CollectUsage();
  bool CanChangePassword() const;

  void RegisterReceiveHandler(TNotifyEvent Handler);
  void UnregisterReceiveHandler(TNotifyEvent Handler);

  // interface to PuTTY core
  void UpdateSocket(SOCKET Value, bool Enable);
  void UpdatePortFwdSocket(SOCKET Value, bool Enable);
  void PuttyFatalError(UnicodeString AError);
  TPromptKind IdentifyPromptKind(UnicodeString & AName) const;
  bool PromptUser(bool ToServer,
    UnicodeString AName, bool NameRequired,
    UnicodeString AInstructions, bool InstructionsRequired,
    TStrings * Prompts, TStrings *Results);
  void FromBackend(const uint8_t * Data, size_t Length);
  void CWrite(const char * Data, size_t Length);
  void AddStdError(const uint8_t * Data, size_t Length);
  const UnicodeString & GetStdError() const;
  void VerifyHostKey(
    const UnicodeString AHost, int32_t Port, const UnicodeString AKeyType, const UnicodeString AKeyStr,
    const UnicodeString & FingerprintSHA256, const UnicodeString & FingerprintMD5);
  bool HaveHostKey(UnicodeString AHost, int32_t Port, const UnicodeString KeyType);
  void AskAlg(UnicodeString AAlgType, UnicodeString AlgName);
  void DisplayBanner(const UnicodeString Banner);
  void OldKeyfileWarning();
  void PuttyLogEvent(const char * AStr);
  UnicodeString ConvertFromPutty(const char * Str, int32_t Length) const;
  struct callback_set * GetCallbackSet();

  __property bool Active = { read = FActive };
  __property bool Ready = { read = GetReady };
  __property TCaptureOutputEvent OnCaptureOutput = { read = FOnCaptureOutput, write = FOnCaptureOutput };
  __property TDateTime LastDataSent = { read = FLastDataSent };
  __property UnicodeString LastTunnelError = { read = FLastTunnelError };
  __property UnicodeString UserName = { read = FUserName };
  __property bool Simple = { read = FSimple, write = FSimple };
  __property TSshImplementation SshImplementation = { read = FSshImplementation };
  __property bool UtfStrings = { read = FUtfStrings, write = FUtfStrings };

  bool GetActive() const { return FActive; }
  TCaptureOutputEvent GetOnCaptureOutput() const { return FOnCaptureOutput; }
  void SetOnCaptureOutput(TCaptureOutputEvent Value) { FOnCaptureOutput = Value; }
  TDateTime GetLastDataSent() const { return FLastDataSent; }
  UnicodeString GetLastTunnelError() const { return FLastTunnelError; }
  UnicodeString ShellGetUserName() const { return FUserName; }
  bool GetSimple() const { return FSimple; }
  void SetSimple(bool Value) { FSimple = Value; }
  TSshImplementation GetSshImplementation() const { return FSshImplementation; }
  bool GetUtfStrings() const { return FUtfStrings; }
  void SetUtfStrings(bool Value) { FUtfStrings = Value; }
};

