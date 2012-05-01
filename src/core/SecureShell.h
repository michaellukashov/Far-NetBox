//---------------------------------------------------------------------------
#ifndef SecureShellH
#define SecureShellH

#include <set>

#include "PuttyIntf.h"
#include "Configuration.h"
#include "SessionData.h"
#include "SessionInfo.h"
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
    TSessionUI *FUI;
    TSessionData *FSessionData;
    bool FActive;
    TSessionInfo FSessionInfo;
    bool FSessionInfoValid;
    TDateTime FLastDataSent;
    Backend *FBackend;
    void *FBackendHandle;
    const unsigned int *FMinPacketSize;
    const unsigned int *FMaxPacketSize;
    Config *FConfig;
    notify_signal_type FOnReceive;
    bool FFrozen;
    bool FDataWhileFrozen;
    bool FStoredPasswordTried;
    bool FStoredPasswordTriedForKI;
    int FSshVersion;
    bool FOpened;
    int FWaiting;
    bool FSimple;

    size_t PendLen;
    size_t PendSize;
    size_t OutLen;
    char *OutPtr;
    char *Pending;
    TSessionLog *FLog;
    TConfiguration *FConfiguration;
    bool FAuthenticating;
    bool FAuthenticated;
    UnicodeString FStdErrorTemp;
    UnicodeString FStdError;
    UnicodeString FCWriteTemp;
    UnicodeString FAuthenticationLog;
    UnicodeString FLastTunnelError;
    UnicodeString FUserName;
    TSecureShell *Self;

    static TCipher FuncToSsh1Cipher(const void *Cipher);
    static TCipher FuncToSsh2Cipher(const void *Cipher);
    UnicodeString FuncToCompression(int SshVersion, const void *Compress) const;
    void Init();
    void inline CheckConnection(int Message = -1);
    void WaitForData();
    void Discard();
    void FreeBackend();
    void PoolForData(WSANETWORKEVENTS &Events, size_t &Result);
    inline void CaptureOutput(TLogLineType Type,
                              const UnicodeString Line);
    void ResetConnection();
    void ResetSessionInfo();
    void SocketEventSelect(SOCKET Socket, HANDLE Event, bool Startup);
    bool EnumNetworkEvents(SOCKET Socket, WSANETWORKEVENTS &Events);
    void HandleNetworkEvents(SOCKET Socket, WSANETWORKEVENTS &Events);
    bool ProcessNetworkEvents(SOCKET Socket);
    bool EventSelectLoop(unsigned int MSec, bool ReadEventRequired,
                         WSANETWORKEVENTS *Events);
    void UpdateSessionInfo();
    void DispatchSendBuffer(size_t BufSize);
    void SendBuffer(size_t &Result);
    int TimeoutPrompt(TQueryParamsTimerEvent *PoolEvent);

protected:
    captureoutput_signal_type FOnCaptureOutput;

    void GotHostKey();
    int TranslatePuttyMessage(const TPuttyTranslation *Translation,
                              size_t Count, UnicodeString &Message);
    int TranslateAuthenticationMessage(UnicodeString &Message);
    int TranslateErrorMessage(UnicodeString &Message);
    void AddStdError(const UnicodeString Str);
    void AddStdErrorLine(const UnicodeString Str);
    void FatalError(const std::exception *E, const UnicodeString Msg);
    void inline LogEvent(const UnicodeString Str);
    void FatalError(const UnicodeString Error);
    static void ClearConfig(Config *cfg);
    static void StoreToConfig(TSessionData *Data, Config *cfg, bool Simple);

public:
    explicit TSecureShell(TSessionUI *UI, TSessionData *SessionData,
                 TSessionLog *Log, TConfiguration *Configuration);
    virtual ~TSecureShell();
    void Open();
    void Close();
    void KeepAlive();
    size_t Receive(char *Buf, size_t Len);
    bool Peek(char *& Buf, size_t Len);
    UnicodeString ReceiveLine();
    void Send(const char *Buf, size_t Len);
    void SendStr(const UnicodeString Str);
    void SendSpecial(int Code);
    void Idle(unsigned int MSec = 0);
    void SendEOF();
    void SendLine(const UnicodeString Line);
    void SendNull();

    const TSessionInfo &GetSessionInfo();
    bool SshFallbackCmd() const;
    unsigned long MinPacketSize();
    unsigned long MaxPacketSize();
    void ClearStdError();
    bool GetStoredCredentialsTried();

    void RegisterReceiveHandler(const TNotifyEvent &Handler);
    void UnregisterReceiveHandler(const TNotifyEvent &Handler);

    // interface to PuTTY core
    void UpdateSocket(SOCKET value, bool Startup);
    void UpdatePortFwdSocket(SOCKET value, bool Startup);
    void PuttyFatalError(const UnicodeString Error);
    bool PromptUser(bool ToServer,
                    const UnicodeString AName, bool NameRequired,
                    const UnicodeString Instructions, bool InstructionsRequired,
                    TStrings *Prompts, TStrings *Results);
    void FromBackend(bool IsStdErr, const char *Data, size_t Length);
    void CWrite(const char *Data, size_t Length);
    const UnicodeString GetStdError();
    void VerifyHostKey(const UnicodeString Host, int Port,
                       const UnicodeString KeyType, const UnicodeString KeyStr, const UnicodeString Fingerprint);
    void AskAlg(const UnicodeString AlgType, const UnicodeString AlgName);
    void DisplayBanner(const UnicodeString Banner);
    void OldKeyfileWarning();
    void PuttyLogEvent(const UnicodeString Str);

    bool GetActive() { return FActive; }
    void SetActive(bool value);
    bool GetReady();
    captureoutput_signal_type &GetOnCaptureOutput() { return FOnCaptureOutput; }
    void SetOnCaptureOutput(const TCaptureOutputEvent &value) { FOnCaptureOutput.connect(value); }
    TDateTime GetLastDataSent() { return FLastDataSent; }
    UnicodeString GetLastTunnelError() { return FLastTunnelError; }
    UnicodeString GetUserName() { return FUserName; }
    bool GetSimple() { return FSimple; }
    void SetSimple(bool value) { FSimple = value; }
private:
    TSecureShell(const TSecureShell &);
    void operator=(const TSecureShell &);
};
//---------------------------------------------------------------------------
#endif
