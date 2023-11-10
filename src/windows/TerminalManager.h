
#ifndef TerminalManagerH
#define TerminalManagerH

#include <Terminal.h>
#include <Queue.h>
#include <FileOperationProgress.h>
#include <WinInterface.h>
#include <Vcl.AppEvnts.hpp>

class TCustomScpExplorerForm;
class TTerminalQueue;
class TAuthenticateForm;
class ITaskbarList3;

enum TTerminalPendingAction { tpNull, tpNone, tpReconnect, tpFree };

class TManagedTerminal : public TTerminal
{
public:
  TManagedTerminal(TSessionData * SessionData, TConfiguration * Configuration);
  virtual ~TManagedTerminal();

  bool LocalBrowser{false};
  TSessionData * StateData{nullptr};
  TObject * LocalExplorerState{nullptr};
  TObject * RemoteExplorerState{nullptr};
  TObject * OtherLocalExplorerState{nullptr};
  TDateTime ReopenStart;
  TDateTime DirectoryLoaded;
  TTerminalThread * TerminalThread{nullptr};
  TDateTime QueueOperationStart;
  // To distinguish sessions that were explicitly disconnected and
  // should not be reconnected when their tab is activated.
  bool Disconnected{true};
  bool DisconnectedTemporarily{true};
  // Sessions that should not close when they fail to connect
  // (i.e. those that were ever connected or were opened as a part of a workspace)
  bool Permanent{false};
};

class TTerminalManager : public TTerminalList
{
public:
  static TTerminalManager * Instance(bool ForceCreation = true);
  static void DestroyInstance();

  TTerminalManager();
  ~TTerminalManager();

  TManagedTerminal * NewManagedTerminal(TSessionData * Data);
  TManagedTerminal * NewLocalBrowser(
    const UnicodeString & LocalDirectory = UnicodeString(), const UnicodeString & OtherLocalDirectory = UnicodeString());
  TManagedTerminal * NewSessions(TList * DataList);
  virtual void FreeTerminal(TTerminal * Terminal);
  void Move(TTerminal * Source, TTerminal * Target);
  void DisconnectActiveTerminalIfPermanentFreeOtherwise();
  void DisconnectActiveTerminal();
  void ReconnectActiveTerminal();
  void FreeActiveTerminal();
  void CycleTerminals(bool Forward);
  bool ConnectTerminal(TTerminal * Terminal);
  void SetActiveTerminalWithAutoReconnect(TManagedTerminal * value);
  void UpdateAppTitle();
  bool CanOpenInPutty();
  void OpenInPutty();
  void NewSession(
    const UnicodeString & SessionUrl, bool ReloadSessions = true, TForm * LinkedForm = nullptr, bool ReplaceExisting = false);
  void NewLocalSession(const UnicodeString & LocalDirectory = UnicodeString(), const UnicodeString & OtherLocalDirectory = UnicodeString());
  void Idle(bool SkipCurrentTerminal);
  UnicodeString GetSessionTitle(TManagedTerminal * Terminal, bool Unique);
  UnicodeString GetActiveSessionAppTitle();
  UnicodeString GetAppProgressTitle();
  UnicodeString FormatFormCaptionWithSession(TCustomForm * Form, const UnicodeString & Caption);
  void HandleException(Exception * E);
  void SaveWorkspace(TList * DataList);
  void QueueStatusUpdated();
  bool IsActiveTerminalForSite(TTerminal * Terminal, TSessionData * Data);
  TManagedTerminal * FindActiveTerminalForSite(TSessionData * Data);
  TTerminalQueue * FindQueueForTerminal(TTerminal * Terminal);
  bool UploadPublicKey(TTerminal * Terminal, TSessionData * Data, UnicodeString & FileName);
  UnicodeString GetPathForSessionTabName(const UnicodeString & Result);
  bool HookFatalExceptionMessageDialog(TMessageParams & Params);
  void UnhookFatalExceptionMessageDialog();
  bool ScheduleTerminalReconnnect(TTerminal * Terminal);

  __property TCustomScpExplorerForm * ScpExplorer = { read = FScpExplorer, write = SetScpExplorer };
  __property TManagedTerminal * ActiveSession = { read = FActiveSession, write = SetActiveSession };
  __property TManagedTerminal * ActiveTerminal = { read = GetActiveTerminal };
  __property TTerminalQueue * ActiveQueue = { read = GetActiveQueue };
  __property int ActiveSessionIndex = { read = GetActiveSessionIndex, write = SetActiveSessionIndex };
  __property TStrings * SessionList = { read = GetSessionList };
  __property TTerminal * LocalTerminal = { read = FLocalTerminal };
  __property TManagedTerminal * Sessions[int Index]  = { read = GetSession };
  __property bool Updating = { read = IsUpdating };

protected:
  virtual TTerminal * CreateTerminal(TSessionData * Data);
  void DoConnectTerminal(TTerminal * Terminal, bool Reopen, bool AdHoc);
  virtual TTerminal * NewTerminal(TSessionData * Data);

private:
  static TTerminalManager * FInstance;
  TCustomScpExplorerForm * FScpExplorer{nullptr};
  TManagedTerminal * FActiveSession{nullptr};
  TManagedTerminal * FTerminalWithFatalExceptionTimer;
  bool FTerminalReconnnecteScheduled;
  TTerminal * FLocalTerminal{nullptr};
  bool FDestroying{false};
  TTerminalPendingAction FTerminalPendingAction;
  TStrings * FSessionList{nullptr};
  TList * FQueues{nullptr};
  TStrings * FTerminationMessages{nullptr};
  UnicodeString FProgressTitle;
  UnicodeString FForegroundProgressTitle;
  TDateTime FDirectoryReadingStart;
  TAuthenticateForm * FAuthenticateForm{nullptr};
  TCriticalSection * FQueueSection{nullptr};
  DWORD FMainThread{0};
  int32_t FPendingConfigurationChange{0};
  std::unique_ptr<TCriticalSection> FChangeSection;
  std::vector<std::pair<TTerminalQueue *, TQueueEvent> > FQueueEvents;
  uint32_t FTaskbarButtonCreatedMessage{0};
  ITaskbarList3 * FTaskbarList{nullptr};
  int32_t FAuthenticating{0};
  void * FBusyToken{nullptr};
  bool FAuthenticationCancelled{false};
  std::unique_ptr<TApplicationEvents> FApplicationsEvents;
  bool FKeepAuthenticateForm{false};
  int32_t FUpdating{0};
  int32_t FMaxSessions{0};

  bool ConnectActiveTerminalImpl(bool Reopen);
  bool ConnectActiveTerminal();
  TTerminalQueue * NewQueue(TTerminal * Terminal);
  void SetScpExplorer(TCustomScpExplorerForm * value);
  void UpdateScpExplorer();
  void UpdateScpExplorer(TManagedTerminal * Session, TTerminalQueue * Queue);
  void DoSetActiveSession(TManagedTerminal * value, bool AutoReconnect, bool LastTerminalClosed);
  void SetActiveSession(TManagedTerminal * value);
  TManagedTerminal * GetActiveTerminal();
  void UpdateAll();
  void ApplicationException(TObject * Sender, Exception * E);
  void ApplicationShowHint(UnicodeString & HintStr, bool & CanShow,
    THintInfo & HintInfo);
  void ApplicationMessage(TMsg & Msg, bool & Handled);
  void ConfigurationChange(TObject * Sender);
  void TerminalUpdateStatus(TTerminal * Terminal, bool Active);
  void TerminalQueryUser(TObject * Sender,
    const UnicodeString & Query, TStrings * MoreMessages, uint32_t Answers,
    const TQueryParams * Params, uint32_t & Answer, TQueryType Type, void * Arg);
  void TerminalPromptUser(TTerminal * Terminal,
    TPromptKind Kind, const UnicodeString & Name, const UnicodeString & Instructions, TStrings * Prompt,
    TStrings * Results, bool & Result, void * Arg);
  void TerminalDisplayBanner(TTerminal * Terminal,
    const UnicodeString & SessionName, const UnicodeString & Banner, bool & NeverShowAgain,
    int32_t Options, uint32_t & Params);
  void TerminalShowExtendedException(TTerminal * Terminal,
    Exception * E, void * Arg);
  void TerminalReadDirectoryProgress(TObject * Sender, int32_t Progress,
    int32_t ResolvedLinks, bool & Cancel);
  void TerminalInformation(
    TTerminal * Terminal, const UnicodeString & Str, bool Status, int32_t Phase, const UnicodeString & Additional);
  void TerminalCustomCommand(TTerminal * Terminal, const UnicodeString & Command, bool & Handled);
  void FreeAll();
  void SessionReady();
  TStrings * GetSessionList() const;
  int32_t GetActiveSessionIndex() const;
  TTerminalQueue * GetActiveQueue();
  void SaveTerminal(TTerminal * Terminal);
  void SetActiveSessionIndex(int32_t value);
  void OperationFinished(::TFileOperation Operation, TOperationSide Side,
    bool Temp, const UnicodeString & FileName, bool Success,
    TOnceDoneOperation & OnceDoneOperation);
  void OperationProgress(TFileOperationProgressType & ProgressData);
  void DeleteLocalFile(const UnicodeString & FileName, bool Alternative, int32_t & Deleted);
  void QueueEvent(TTerminalQueue * Queue, TQueueEvent Event);
  TAuthenticateForm * MakeAuthenticateForm(TTerminal * Terminal);
  void MasterPasswordPrompt();
  void FileNameInputDialogInitializeRenameBaseName(
    TObject * Sender, TInputDialogData * Data);
  void InitTaskbarButtonCreatedMessage();
  void ReleaseTaskbarList();
  void CreateTaskbarList();
  void UpdateTaskbarList();
  void AuthenticateFormCancel(TObject * Sender);
  void DoSessionListChanged();
  TManagedTerminal * DoNewSession(TSessionData * Data);
  static void TerminalThreadIdle(void * Data, TObject * Sender);
  void SetQueueConfiguration(TTerminalQueue * Queue);
  void ApplicationModalBegin(TObject * Sender);
  void ApplicationModalEnd(TObject * Sender);
  bool HandleMouseWheel(WPARAM WParam, LPARAM LParam);
  void DoConfigurationChange();
  bool ShouldDisplayQueueStatusOnAppTitle();
  void SetupTerminal(TTerminal * Terminal);
  void CloseAutheticateForm();
  void AuthenticatingDone();
  TManagedTerminal * CreateManagedTerminal(TSessionData * Data);
  TManagedTerminal * GetSession(int32_t Index);
  bool IsUpdating();
  bool SupportedSession(TSessionData * Data);
  void TerminalFatalExceptionTimer(uint32_t & Result);
};

#endif
