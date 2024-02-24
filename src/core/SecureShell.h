
#pragma once

#include "PuttyIntf.h"
#include "Configuration.h"
#include "SessionData.h"
#include "SessionInfo.h"

#ifndef PuttyIntfH
// struct Backend_vtable;
// struct Backend;
// struct Conf;
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
class TSFTPFileSystem;

class TSecureShell : public TObject
{
  friend class TPoolForDataEvent;
  friend class TTerminal;
  friend class TSFTPFileSystem;
  NB_DISABLE_COPY(TSecureShell)
public:
  static bool classof(const TObject * Obj) { return Obj->is(OBJECT_CLASS_TSecureShell); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TSecureShell) || TObject::is(Kind); }

  mutable gsl::owner<const uint32_t *> FMinPacketSize{nullptr};
  mutable gsl::owner<const uint32_t *> FMaxPacketSize{nullptr};
  //uint32_t MinPacketSize() const;
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
  gsl::owner<Backend *> FBackendHandle{nullptr};
  TNotifyEvent FOnReceive{nullptr};
  bool FFrozen{false};
  bool FDataWhileFrozen{false};
  bool FStoredPasswordTried{false};
  bool FStoredPasswordTriedForKI{false};
  bool FStoredPassphraseTried{false};
  bool FAuthenticationCancelled{false};
  bool FOpened{false};
  bool FClosed{false};
  int32_t FWaiting{0};
  bool FSimple{false};
  bool FNoConnectionResponse{false};
  bool FCollectPrivateKeyUsage{false};
  int32_t FWaitingForData{0};
  TSshImplementation FSshImplementation{sshiUnknown};

  size_t PendLen{0};
  size_t PendSize{0};
  size_t OutLen{0};
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
  std::unique_ptr<callback_set> FCallbackSet;
  std::unique_ptr<ScpLogPolicy> FLogPolicy{nullptr};
  std::unique_ptr<ScpSeat> FSeat{nullptr};
  gsl::owner<LogContext *> FLogCtx{nullptr};
  nb::set_t<UnicodeString> FLoggedKnownHostKeys;

public:
  void Init();
  void SetActive(bool Value);
  void CheckConnection(int32_t Message = -1);
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
    WSANETWORKEVENTS * Events);
  void UpdateSessionInfo() const;
  bool GetReady() const;
  void DispatchSendBuffer(int32_t BufSize);
  void SendBuffer(uint32_t & Result);
  uint32_t TimeoutPrompt(TQueryParamsTimerEvent && PoolEvent);
  void TimeoutAbort(uint32_t Answer);
  bool TryFtp();
  UnicodeString ConvertInput(const RawByteString & Input, uint32_t CodePage = CP_ACP) const;
  void GetRealHost(UnicodeString & Host, int32_t & Port) const;
  UnicodeString RetrieveHostKey(const UnicodeString & Host, int32_t Port, const UnicodeString & KeyType);
  bool HaveAcceptNewHostKeyPolicy() const;
  THierarchicalStorage * GetHostKeyStorage();
  bool VerifyCachedHostKey(
    const UnicodeString & StoredKeys, const UnicodeString & KeyStr, const UnicodeString & FingerprintMD5, const UnicodeString & FingerprintSHA256);
  UnicodeString StoreHostKey(
    const UnicodeString & Host, int32_t Port, const UnicodeString & KeyType, const UnicodeString & KeyStr);
  bool HasLocalProxy() const;

protected:
  TCaptureOutputEvent FOnCaptureOutput;

  void GotHostKey();
  int32_t TranslatePuttyMessage(const TPuttyTranslation * Translation,
    int32_t Count, UnicodeString & Message, UnicodeString * HelpKeyword = nullptr) const;
  int32_t TranslateAuthenticationMessage(UnicodeString & Message, UnicodeString * HelpKeyword = nullptr);
  int32_t TranslateErrorMessage(UnicodeString & Message, UnicodeString * HelpKeyword = nullptr);
  void AddStdErrorLine(const UnicodeString & AStr);
  void LogEvent(const UnicodeString & AStr);
  void FatalError(const UnicodeString & Error, const UnicodeString & HelpKeyword = "");
  UnicodeString FormatKeyStr(const UnicodeString & AKeyStr) const;
  void ParseFingerprint(const UnicodeString & Fingerprint, UnicodeString & SignKeyType, UnicodeString & Hash);
  static Conf * StoreToConfig(TSessionData * Data, bool Simple);

public:
  explicit TSecureShell(TSessionUI * UI, TSessionData * SessionData,
    TSessionLog * Log, TConfiguration * Configuration) noexcept;
  virtual ~TSecureShell() noexcept override;
  void Open();
  void Close();
  void KeepAlive();
  int32_t Receive(uint8_t * Buf, size_t Length);
  bool Peek(uint8_t *& Buf, size_t Length) const;
  UnicodeString ReceiveLine();
  void Send(const uint8_t * Buf, size_t Length);
  void SendSpecial(int32_t Code);
  void Idle(uint32_t MSec = 0);
  void SendLine(const UnicodeString & Line);
  void SendNull();

  const TSessionInfo & GetSessionInfo() const;
  void GetHostKeyFingerprint(UnicodeString & SHA256, UnicodeString & MD5) const;
  bool SshFallbackCmd() const;
  uint32_t MaxPacketSize() const;
  void ClearStdError();
  bool GetStoredCredentialsTried() const;
  void CollectUsage();
  bool CanChangePassword() const;

  void RegisterReceiveHandler(TNotifyEvent && Handler);
  void UnregisterReceiveHandler(TNotifyEvent && Handler);

  // interface to PuTTY core
  void UpdateSocket(SOCKET Value, bool Enable);
  void UpdatePortFwdSocket(SOCKET Value, bool Enable);
  void PuttyFatalError(const UnicodeString & AError);
  TPromptKind IdentifyPromptKind(UnicodeString & AName) const;
  bool PromptUser(bool ToServer,
    const UnicodeString & AName, bool NameRequired,
    const UnicodeString & AInstructions, bool InstructionsRequired,
    TStrings * Prompts, TStrings * Results);
  void FromBackend(const uint8_t * Data, size_t Length);
  void CWrite(const char * Data, size_t Length);
  void AddStdError(const uint8_t * Data, size_t Length);
  UnicodeString GetStdError() const;
  void VerifyHostKey(
    const UnicodeString & AHost, int32_t Port, const UnicodeString & AKeyType, const UnicodeString & AKeyStr,
    const UnicodeString & FingerprintSHA256, const UnicodeString & FingerprintMD5,
    bool IsCertificate, int32_t CACount, bool AlreadyVerified);
  bool HaveHostKey(const UnicodeString & AHost, int32_t Port, const UnicodeString & KeyType);
  void AskAlg(const UnicodeString & AAlgType, const UnicodeString & AlgName, int32_t WeakCryptoReason);
  void DisplayBanner(const UnicodeString & Banner);
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
  TCaptureOutputEvent & GetOnCaptureOutput() { return FOnCaptureOutput; }
  void SetOnCaptureOutput(TCaptureOutputEvent && Value) { FOnCaptureOutput = std::move(Value); }
  TDateTime GetLastDataSent() const { return FLastDataSent; }
  UnicodeString GetLastTunnelError() const { return FLastTunnelError; }
  UnicodeString ShellGetUserName() const { return FUserName; }
  bool GetSimple() const { return FSimple; }
  void SetSimple(bool Value) { FSimple = Value; }
  TSshImplementation GetSshImplementation() const { return FSshImplementation; }
  bool GetUtfStrings() const { return FUtfStrings; }
  void SetUtfStrings(bool Value) { FUtfStrings = Value; }
};

