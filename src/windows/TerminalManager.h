
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

  TSessionData * StateData{nullptr};
  TObject * LocalExplorerState{nullptr};
  TObject * RemoteExplorerState{nullptr};
  TDateTime ReopenStart;
  TDateTime DirectoryLoaded;
  TTerminalThread * TerminalThread{nullptr};
  TDateTime QueueOperationStart;
  // To distinguish sessions that were explicitly disconnected and
  // should not be reconnected when their tab is activated.
  bool Disconnected;
  bool DisconnectedTemporarily;
  // Sessions that should not close when they fail to connect
  // (i.e. those that were ever connected or were opened as a part of a workspace)
  bool Permanent;
};

class TTerminalManager : public TTerminalList
{
public:
  static TTerminalManager * Instance(bool ForceCreation = true);
  static void DestroyInstance();

  TTerminalManager();
  ~TTerminalManager();

  TManagedTerminal * NewManagedTerminal(TSessionData * Data);
  TManagedTerminal * NewTerminals(TList * DataList);
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
  void NewSession(const UnicodeString & SessionUrl, bool ReloadSessions = true, TForm * LinkedForm = NULL);
  void Idle(bool SkipCurrentTerminal);
  UnicodeString GetTerminalShortPath(TTerminal * Terminal);
  UnicodeString GetTerminalTitle(TTerminal * Terminal, bool Unique);
  UnicodeString GetActiveTerminalTitle(bool Unique);
  UnicodeString GetAppProgressTitle();
  UnicodeString FormatFormCaptionWithSession(TCustomForm * Form, const UnicodeString & Caption);
  void HandleException(Exception * E);
  void SaveWorkspace(TList * DataList);
  void QueueStatusUpdated();
  bool IsActiveTerminalForSite(TTerminal * Terminal, TSessionData * Data);
  TManagedTerminal * FindActiveTerminalForSite(TSessionData * Data);
  TTerminalQueue * FindQueueForTerminal(TTerminal * Terminal);
  bool UploadPublicKey(TTerminal * Terminal, TSessionData * Data, UnicodeString & FileName);

  __property TCustomScpExplorerForm * ScpExplorer = { read = FScpExplorer, write = SetScpExplorer };
  __property TManagedTerminal * ActiveTerminal = { read = FActiveTerminal, write = SetActiveTerminal };
  __property TTerminalQueue * ActiveQueue = { read = GetActiveQueue };
  __property int ActiveTerminalIndex = { read = GetActiveTerminalIndex, write = SetActiveTerminalIndex };
  __property TStrings * TerminalList = { read = GetTerminalList };
  __property TNotifyEvent OnLastTerminalClosed = { read = FOnLastTerminalClosed, write = FOnLastTerminalClosed };
  __property TNotifyEvent OnTerminalListChanged = { read = FOnTerminalListChanged, write = FOnTerminalListChanged };
  __property TTerminal * LocalTerminal = { read = FLocalTerminal };
  __property TManagedTerminal * Terminals[int Index]  = { read=GetTerminal };

protected:
  virtual TTerminal * CreateTerminal(TSessionData * Data);
  void DoConnectTerminal(TTerminal * Terminal, bool Reopen, bool AdHoc);
  virtual TTerminal * NewTerminal(TSessionData * Data);

private:
  static TTerminalManager * FInstance;
  TCustomScpExplorerForm * FScpExplorer{nullptr};
  TTerminal * FActiveTerminal{nullptr};
  TTerminal * FLocalTerminal{nullptr};
  bool FDestroying{false};
  TTerminalPendingAction FTerminalPendingAction;
  TNotifyEvent FOnLastTerminalClosed;
  TNotifyEvent FOnTerminalListChanged;
  TStrings * FTerminalList{nullptr};
  TList * FQueues{nullptr};
  TStrings * FTerminationMessages{nullptr};
  UnicodeString FProgressTitle;
  UnicodeString FForegroundProgressTitle;
  TDateTime FDirectoryReadingStart;
  TAuthenticateForm * FAuthenticateForm{nullptr};
  TCriticalSection * FQueueSection{nullptr};
  DWORD FMainThread{0};
  int FPendingConfigurationChange{0};
  std::unique_ptr<TCriticalSection> FChangeSection;
  std::vector<std::pair<TTerminalQueue *, TQueueEvent> > FQueueEvents;
  unsigned int FTaskbarButtonCreatedMessage{0};
  ITaskbarList3 * FTaskbarList{nullptr};
  int FAuthenticating{0};
  void * FBusyToken{nullptr};
  bool FAuthenticationCancelled{false};
  std::unique_ptr<TApplicationEvents> FApplicationsEvents;
  bool FKeepAuthenticateForm{false};
  int FMaxSessions;

  bool ConnectActiveTerminalImpl(bool Reopen);
  bool ConnectActiveTerminal();
  TTerminalQueue * NewQueue(TTerminal * Terminal);
  void SetScpExplorer(TCustomScpExplorerForm * value);
  void DoSetActiveTerminal(TManagedTerminal * value, bool AutoReconnect);
  void SetActiveTerminal(TManagedTerminal * value);
  void UpdateAll();
  void ApplicationException(TObject * Sender, Exception * E);
  void ApplicationShowHint(UnicodeString & HintStr, bool & CanShow,
    THintInfo & HintInfo);
  void ApplicationMessage(TMsg & Msg, bool & Handled);
  void ConfigurationChange(TObject * Sender);
  void TerminalUpdateStatus(TTerminal * Terminal, bool Active);
  void TerminalQueryUser(TObject * Sender,
    const UnicodeString Query, TStrings * MoreMessages, unsigned int Answers,
    const TQueryParams * Params, unsigned int & Answer, TQueryType Type, void * Arg);
  void TerminalPromptUser(TTerminal * Terminal,
    TPromptKind Kind, UnicodeString Name, UnicodeString Instructions, TStrings * Prompt,
    TStrings * Results, bool & Result, void * Arg);
  void TerminalDisplayBanner(TTerminal * Terminal,
    UnicodeString SessionName, const UnicodeString & Banner, bool & NeverShowAgain,
    int Options, unsigned int & Params);
  void TerminalShowExtendedException(TTerminal * Terminal,
    Exception * E, void * Arg);
  void TerminalReadDirectoryProgress(TObject * Sender, int Progress,
    int ResolvedLinks, bool & Cancel);
  void TerminalInformation(
    TTerminal * Terminal, const UnicodeString & Str, bool Status, int Phase, const UnicodeString & Additional);
  void TerminalCustomCommand(TTerminal * Terminal, const UnicodeString & Command, bool & Handled);
  void FreeAll();
  void TerminalReady();
  TStrings * GetTerminalList();
  int GetActiveTerminalIndex();
  TTerminalQueue * GetActiveQueue();
  void SaveTerminal(TTerminal * Terminal);
  void SetActiveTerminalIndex(int value);
  void OperationFinished(::TFileOperation Operation, TOperationSide Side,
    bool Temp, const UnicodeString & FileName, bool Success,
    TOnceDoneOperation & OnceDoneOperation);
  void OperationProgress(TFileOperationProgressType & ProgressData);
  void DeleteLocalFile(const UnicodeString FileName, bool Alternative, int & Deleted);
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
  void DoTerminalListChanged();
  TManagedTerminal * DoNewTerminal(TSessionData * Data);
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
  TManagedTerminal * GetTerminal(int Index);
};

#endif
