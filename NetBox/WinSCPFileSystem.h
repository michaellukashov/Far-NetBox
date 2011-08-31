//---------------------------------------------------------------------------
#ifndef WinSCPFileSystemH
#define WinSCPFileSystemH
//---------------------------------------------------------------------------
#include "Interface.h"
#include "FarPlugin.h"
#include "FileOperationProgress.h"
#include <Terminal.h>
#include <GUIConfiguration.h>
#include <SynchronizeController.h>
#include <Queue.h>
#include <list>
#include <map>
//---------------------------------------------------------------------------
class TTerminal;
class TSessionData;
class TRemoteFile;
class TBookmarkList;
class TWinSCPPlugin;
class TFarButton;
class TFarDialogItem;
class TFarDialog;
class TTerminalQueue;
class TTerminalQueueStatus;
class TQueueItem;
class TKeepaliveThread;
struct TMessageParams;
#define REMOTE_DIR_HISTORY L"WinscpRemoteDirectory"
#define ASCII_MASK_HISTORY L"WinscpAsciiMask"
#define LINK_FILENAME_HISTORY L"WinscpRemoteLink"
#define LINK_POINT_TO_HISTORY L"WinscpRemoteLinkPointTo"
#define APPLY_COMMAND_HISTORY L"WinscpApplyCmd"
#define APPLY_COMMAND_PARAM_HISTORY L"WinscpApplyCmdParam"
#define LOG_FILE_HISTORY L"WinscpLogFile"
#define REMOTE_SYNC_HISTORY L"WinscpRemoteSync"
#define LOCAL_SYNC_HISTORY L"WinscpLocalSync"
#define MOVE_TO_HISTORY L"WinscpMoveTo"
#define EXCLUDE_FILE_MASK_HISTORY L"WinscpExcludeFileMask"
#define MAKE_SESSION_FOLDER_HISTORY L"WinscpSessionFolder"
//---------------------------------------------------------------------------
// for Properties dialog
const int cpMode =  0x01;
const int cpOwner = 0x02;
const int cpGroup = 0x04;
// for Copy dialog
const int coTempTransfer        = 0x01;
const int coDisableNewerOnly    = 0x04;
// for Synchronize and FullSynchronize dialogs
const int spSelectedOnly = 0x800;
// for Synchronize dialogs
const int soAllowSelectedOnly = 0x01;
// for FullSynchronize dialog
const int fsoDisableTimestamp = 0x01;
const int fsoAllowSelectedOnly = 0x02;
enum TSessionActionEnum { saAdd, saEdit, saConnect };
//---------------------------------------------------------------------------
typedef void (TObject::*TGetSynchronizeOptionsEvent)
  (int Params, TSynchronizeOptions & Options);
typedef void (TObject::*TGetSpaceAvailable)
  (const wstring Path, TSpaceAvailable & ASpaceAvailable, bool & Close);
struct TMultipleEdit
{
  wstring FileName;
  wstring Directory;
  wstring LocalFileName;
  bool PendingSave;
};
struct TEditHistory
{
  wstring FileName;
  wstring Directory;
  bool operator==(const TEditHistory& rh) { return (FileName == rh.FileName) && (Directory == rh.Directory); }
};
//---------------------------------------------------------------------------
typedef void (TObject::* TProcessSessionEvent)(TSessionData * Data, void * Param);
//---------------------------------------------------------------------------
class TWinSCPFileSystem : public TCustomFarFileSystem
{
friend class TWinSCPPlugin;
friend class TKeepaliveThread;
friend class TQueueDialog;
public:
  TWinSCPFileSystem(TCustomFarPlugin * APlugin);
  virtual ~TWinSCPFileSystem();

  virtual void Close();

protected:
  bool Connect(TSessionData * Data);
  void SaveSession();

  virtual void GetOpenPluginInfoEx(long unsigned & Flags,
    wstring & HostFile, wstring & CurDir, wstring & Format,
    wstring & PanelTitle, TFarPanelModes * PanelModes, int & StartPanelMode,
    int & StartSortMode, bool & StartSortOrder, TFarKeyBarTitles * KeyBarTitles,
    wstring & ShortcutData);
  virtual bool GetFindDataEx(TObjectList *PanelItems, int OpMode);
  virtual bool ProcessKeyEx(int Key, unsigned int ControlState);
  virtual bool SetDirectoryEx(const wstring Dir, int OpMode);
  virtual int MakeDirectoryEx(wstring & Name, int OpMode);
  virtual bool DeleteFilesEx(TList * PanelItems, int OpMode);
  virtual int GetFilesEx(TList * PanelItems, bool Move,
    wstring & DestPath, int OpMode);
  virtual int PutFilesEx(TList * PanelItems, bool Move, int OpMode);
  virtual bool ProcessEventEx(int Event, void * Param);

  void ProcessEditorEvent(int Event, void * Param);

  virtual void HandleException(exception * E, int OpMode = 0);
  void KeepaliveThreadCallback();

  inline bool SessionList();
  inline bool Connected();
  TWinSCPPlugin * WinSCPPlugin();
  void ShowOperationProgress(TFileOperationProgressType & ProgressData,
    bool Force);
  bool SessionDialog(TSessionData * Data, TSessionActionEnum & Action);
  void EditConnectSession(TSessionData * Data, bool Edit);
  void DuplicateRenameSession(TSessionData * Data,
    bool Duplicate);
  void FocusSession(TSessionData * Data);
  void DeleteSession(TSessionData * Data, void * Param);
  void ProcessSessions(TList * PanelItems,
    TProcessSessionEvent ProcessSession, void * Param);
  void ExportSession(TSessionData * Data, void * Param);
  bool ImportSessions(TList * PanelItems, bool Move, int OpMode);
  void FileProperties();
  void CreateLink();
  void TransferFiles(bool Move);
  void RenameFile();
  void ApplyCommand();
  void ShowInformation();
  void InsertTokenOnCommandLine(wstring Token, bool Separate);
  void InsertSessionNameOnCommandLine();
  void InsertFileNameOnCommandLine(bool Full);
  void InsertPathOnCommandLine();
  void CopyFullFileNamesToClipboard();
  void FullSynchronize(bool Source);
  void Synchronize();
  void OpenDirectory(bool Add);
  void HomeDirectory();
  void ToggleSynchronizeBrowsing();
  bool IsSynchronizedBrowsing();
  bool PropertiesDialog(TStrings * FileList,
    const wstring Directory, 
    // const TRemoteTokenList *GroupList, const TRemoteTokenList *UserList,
    TStrings * GroupList, TStrings * UserList,
    TRemoteProperties * Properties, int AllowedChanges);
  bool ExecuteCommand(const wstring Command);
  void TerminalCaptureLog(const wstring & AddedLine, bool StdError);
  bool CopyDialog(bool ToRemote, bool Move, TStrings * FileList,
    wstring & TargetDirectory, TCopyParamType * Params, int Options,
    int CopyParamAttrs);
  bool LinkDialog(wstring & FileName, wstring & PointTo, bool & Symbolic,
    bool Edit, bool AllowSymbolic);
  void FileSystemInfoDialog(const TSessionInfo & SessionInfo,
    const TFileSystemInfo & FileSystemInfo, wstring SpaceAvailablePath,
    TGetSpaceAvailable OnGetSpaceAvailable);
  bool OpenDirectoryDialog(bool Add, wstring & Directory,
    TBookmarkList * BookmarkList);
  bool ApplyCommandDialog(wstring & Command, int & Params);
  bool FullSynchronizeDialog(TTerminal::TSynchronizeMode & Mode,
    int & Params, wstring & LocalDirectory, wstring & RemoteDirectory,
    TCopyParamType * CopyParams, bool & SaveSettings, bool & SaveMode, int Options,
    const TUsableCopyParamAttrs & CopyParamAttrs);
  bool SynchronizeChecklistDialog(TSynchronizeChecklist * Checklist,
    TTerminal::TSynchronizeMode Mode, int Params,
    const wstring LocalDirectory, const wstring RemoteDirectory);
  bool RemoteTransferDialog(TStrings * FileList, wstring & Target,
    wstring & FileMask, bool Move);
  bool RenameFileDialog(TRemoteFile * File, wstring & NewName);
  int MoreMessageDialog(wstring Str, TStrings * MoreMessages,
    TQueryType Type, int Answers, const TMessageParams * Params = NULL);
  bool PasswordDialog(TSessionData * SessionData,
    TPromptKind Kind, wstring Name, wstring Instructions, TStrings * Prompts,
    TStrings * Results, bool StoredCredentialsTried);
  bool BannerDialog(wstring SessionName, const wstring & Banner,
    bool & NeverShowAgain, int Options);
  bool CreateDirectoryDialog(wstring & Directory,
    TRemoteProperties * Properties, bool & SaveSettings);
  bool QueueDialog(TTerminalQueueStatus * Status, bool ClosingPlugin);
  bool SynchronizeDialog(TSynchronizeParamType & Params,
    const TCopyParamType * CopyParams, TSynchronizeStartStopEvent OnStartStop,
    bool & SaveSettings, int Options, int CopyParamAttrs,
    TGetSynchronizeOptionsEvent OnGetOptions);
  void DoSynchronize(TSynchronizeController * Sender,
    const wstring LocalDirectory, const wstring RemoteDirectory,
    const TCopyParamType & CopyParam, const TSynchronizeParamType & Params,
    TSynchronizeChecklist ** Checklist, TSynchronizeOptions * Options, bool Full);
  void DoSynchronizeInvalid(TSynchronizeController * Sender,
    const wstring Directory, const wstring ErrorStr);
  void DoSynchronizeTooManyDirectories(TSynchronizeController * Sender,
    int & MaxDirectories);
  void Synchronize(const wstring LocalDirectory,
    const wstring RemoteDirectory, TTerminal::TSynchronizeMode Mode,
    const TCopyParamType & CopyParam, int Params, TSynchronizeChecklist ** Checklist,
    TSynchronizeOptions * Options);
  bool SynchronizeAllowSelectedOnly();
  void GetSynchronizeOptions(int Params, TSynchronizeOptions & Options);
  void RequireCapability(int Capability);
  void RequireLocalPanel(TFarPanelInfo * Panel, wstring Message);
  bool AreCachesEmpty();
  void ClearCaches();
  void OpenSessionInPutty();
  void QueueShow(bool ClosingPlugin);
  TTerminalQueueStatus * ProcessQueue(bool Hidden);
  bool EnsureCommandSessionFallback(TFSCapability Capability);
  void ConnectTerminal(TTerminal * Terminal);
  void TemporarilyDownloadFiles(TStrings * FileList,
    TCopyParamType CopyParam, wstring & TempDir);
  int UploadFiles(bool Move, int OpMode, bool Edit, wstring DestPath);
  void UploadOnSave(bool NoReload);
  void UploadFromEditor(bool NoReload, wstring FileName, wstring DestPath);
  void LogAuthentication(TTerminal * Terminal, wstring Msg);
  void MultipleEdit();
  void MultipleEdit(wstring Directory, wstring FileName, TRemoteFile * File);
  void EditViewCopyParam(TCopyParamType & CopyParam);
  bool SynchronizeBrowsing(wstring NewPath);
  bool IsEditHistoryEmpty();
  void EditHistory();
  wstring ProgressBar(int Percentage, int Width);
  bool IsLogging();
  void ShowLog();

  // __property  TTerminal * Terminal = { read = FTerminal };
  TTerminal * GetTerminal() { return FTerminal; }

private:
  TTerminal * FTerminal;
  TTerminalQueue * FQueue;
  TTerminalQueueStatus * FQueueStatus;
  TCriticalSection * FQueueStatusSection;
  bool FQueueStatusInvalidated;
  bool FQueueItemInvalidated;
  bool FRefreshLocalDirectory;
  bool FRefreshRemoteDirectory;
  bool FQueueEventPending;
  TQueueEvent FQueueEvent;
  bool FReloadDirectory;
  HANDLE FProgressSaveScreenHandle;
  HANDLE FSynchronizationSaveScreenHandle;
  HANDLE FAuthenticationSaveScreenHandle;
  TDateTime FSynchronizationStart;
  bool FSynchronizationCompare;
  TStrings * FFileList;
  TList * FPanelItems;
  wstring FSavedFindFolder;
  wstring FOriginalEditFile;
  wstring FLastEditFile;
  wstring FLastMultipleEditFile;
  wstring FLastMultipleEditDirectory;
  bool FLastMultipleEditReadOnly;
  int FLastEditorID;
  bool FEditorPendingSave;
  TCopyParamType FLastEditCopyParam;
  bool FNoProgress;
  bool FNoProgressFinish;
  TKeepaliveThread * FKeepaliveThread;
  bool FSynchronisingBrowse;
  TSynchronizeController * FSynchronizeController;
  TStrings * FCapturedLog;
  bool FOutputLog;
  TStrings * FAuthenticationLog;
  typedef std::map<int, TMultipleEdit> TMultipleEdits;
  TMultipleEdits FMultipleEdits;
  bool FLoadingSessionList;
  typedef std::vector<TEditHistory> TEditHistories;
  TEditHistories FEditHistories;
  wstring FLastPath;
  TStrings * FPathHistory;
  wstring FSessionsFolder;
  wstring FNewSessionsFolder;

  void TerminalClose(TObject * Sender);
  void TerminalUpdateStatus(TTerminal * Terminal, bool Active);
  void TerminalChangeDirectory(TObject * Sender);
  void TerminalReadDirectory(TObject* Sender, bool ReloadOnly);
  void TerminalStartReadDirectory(TObject * Sender);
  void TerminalReadDirectoryProgress(TObject * Sender, int Progress,
    bool & Cancel);
  void TerminalInformation(TTerminal * Terminal,
    const wstring & Str, bool Status, bool Active);
  void TerminalQueryUser(TObject * Sender,
    const wstring Query, TStrings * MoreMessages, int Answers,
    const TQueryParams * Params, int & Answer, TQueryType Type, void * Arg);
  void TerminalPromptUser(TTerminal * Terminal,
    TPromptKind Kind, wstring Name, wstring Instructions,
    TStrings * Prompts, TStrings * Results, bool & Result,
    void * Arg);
  void TerminalDisplayBanner(TTerminal * Terminal,
    wstring SessionName, const wstring & Banner, bool & NeverShowAgain,
    int Options);
  void TerminalShowExtendedException(TTerminal * Terminal,
    exception * E, void * Arg);
  void TerminalDeleteLocalFile(const wstring FileName, bool Alternative);
  void OperationProgress(
    TFileOperationProgressType & ProgressData, TCancelStatus & Cancel);
  void OperationFinished(TFileOperation Operation,
    TOperationSide Side, bool DragDrop, const wstring & FileName, bool Success,
    bool & DisconnectWhenComplete);
  void CancelConfiguration(TFileOperationProgressType & ProgressData);
  TStrings * CreateFileList(TObjectList * PanelItems,
    TOperationSide Side, bool SelectedOnly = false, wstring Directory = L"",
    bool FileNameOnly = false, TStrings * AFileList = NULL);
  TStrings * CreateSelectedFileList(TOperationSide Side,
    TFarPanelInfo * PanelInfo = NULL);
  TStrings * CreateFocusedFileList(TOperationSide Side,
    TFarPanelInfo * PanelInfo = NULL);
  void CustomCommandGetParamValue(
    const wstring AName, wstring & Value);
  void TerminalSynchronizeDirectory(const wstring LocalDirectory,
    const wstring RemoteDirectory, bool & Continue, bool Collect);
  void QueueListUpdate(TTerminalQueue * Queue);
  void QueueItemUpdate(TTerminalQueue * Queue, TQueueItem * Item);
  void QueueEvent(TTerminalQueue * Queue, TQueueEvent Event);
  void GetSpaceAvailable(const wstring Path,
    TSpaceAvailable & ASpaceAvailable, bool & Close);
  void QueueAddItem(TQueueItem * Item);
};
//---------------------------------------------------------------------------
class TSessionPanelItem : public TCustomFarPanelItem
{
public:
  TSessionPanelItem(wstring Path);
  TSessionPanelItem(TSessionData * ASessionData);
  static void SetPanelModes(TFarPanelModes * PanelModes);
  static void SetKeyBarTitles(TFarKeyBarTitles * KeyBarTitles);

protected:
  wstring FPath;
  TSessionData * FSessionData;

  virtual void GetData(
    unsigned long & Flags, wstring & FileName, __int64 & Size,
    unsigned long & FileAttributes,
    TDateTime & LastWriteTime, TDateTime & LastAccess,
    unsigned long & NumberOfLinks, wstring & Description,
    wstring & Owner, void *& UserData, int & CustomColumnNumber);
};
//---------------------------------------------------------------------------
class TSessionFolderPanelItem : public TCustomFarPanelItem
{
public:
  TSessionFolderPanelItem(wstring Folder);

protected:
  wstring FFolder;

  virtual void GetData(
    unsigned long & Flags, wstring & FileName, __int64 & Size,
    unsigned long & FileAttributes,
    TDateTime & LastWriteTime, TDateTime & LastAccess,
    unsigned long & NumberOfLinks, wstring & Description,
    wstring & Owner, void *& UserData, int & CustomColumnNumber);
};
//---------------------------------------------------------------------------
class TRemoteFilePanelItem : public TCustomFarPanelItem
{
public:
  TRemoteFilePanelItem(TRemoteFile * ARemoteFile);
  static void SetPanelModes(TFarPanelModes * PanelModes);
  static void SetKeyBarTitles(TFarKeyBarTitles * KeyBarTitles);

protected:
  TRemoteFile * FRemoteFile;

  virtual void GetData(
    unsigned long & Flags, wstring & FileName, __int64 & Size,
    unsigned long & FileAttributes,
    TDateTime & LastWriteTime, TDateTime & LastAccess,
    unsigned long & NumberOfLinks, wstring & Description,
    wstring & Owner, void *& UserData, int & CustomColumnNumber);
  virtual wstring CustomColumnData(int Column);
  static void TranslateColumnTypes(wstring & ColumnTypes,
    TStrings * ColumnTitles);
};
//---------------------------------------------------------------------------
#endif
