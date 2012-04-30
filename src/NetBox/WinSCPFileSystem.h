//---------------------------------------------------------------------------
#ifndef WinSCPFileSystemH
#define WinSCPFileSystemH
//---------------------------------------------------------------------------
#ifdef _MSC_VER
#include "boostdefines.hpp"
#include <boost/signals/signal3.hpp>
#endif

#include <Interface.h>
#include "FarPlugin.h"
#include <FileOperationProgress.h>
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
class TNetBoxPlugin;
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
typedef boost::signal2<void, int, TSynchronizeOptions &> getsynchronizeoptions_signal_type;
typedef getsynchronizeoptions_signal_type::slot_type TGetSynchronizeOptionsEvent;
typedef boost::signal3<void, const UnicodeString, TSpaceAvailable &, bool &> getspaceavailable_signal_type;
typedef getspaceavailable_signal_type::slot_type TGetSpaceAvailableEvent;
struct TMultipleEdit
{
  UnicodeString FileName;
  UnicodeString Directory;
  UnicodeString LocalFileName;
  bool PendingSave;
};
struct TEditHistory
{
  UnicodeString FileName;
  UnicodeString Directory;
  bool operator==(const TEditHistory & rh) { return (FileName == rh.FileName) && (Directory == rh.Directory); }
};
//---------------------------------------------------------------------------
typedef boost::signal2<void, TSessionData *, void *> processsession_signal_type;
typedef processsession_signal_type::slot_type TProcessSessionEvent;
//---------------------------------------------------------------------------
class TWinSCPFileSystem : public TCustomFarFileSystem
{
  friend class TWinSCPPlugin;
  friend class TNetBoxPlugin;
  friend class TKeepaliveThread;
  friend class TQueueDialog;
public:
  explicit TWinSCPFileSystem(TCustomFarPlugin * APlugin);
  virtual void Init(TSecureShell * SecureShell);
  virtual ~TWinSCPFileSystem();

  virtual void Close();

protected:
  bool Connect(TSessionData * Data);
  void Disconnect();
  void SaveSession();

  virtual void GetOpenPluginInfoEx(long unsigned & Flags,
    UnicodeString & HostFile, UnicodeString & CurDir, UnicodeString & Format,
    UnicodeString & PanelTitle, TFarPanelModes * PanelModes, int & StartPanelMode,
    int & StartSortMode, bool & StartSortOrder, TFarKeyBarTitles * KeyBarTitles,
    UnicodeString & ShortcutData);
  virtual bool GetFindDataEx(TObjectList * PanelItems, int OpMode);
  virtual bool ProcessKeyEx(int Key, unsigned int ControlState);
  virtual bool SetDirectoryEx(const UnicodeString Dir, int OpMode);
  virtual int MakeDirectoryEx(UnicodeString & Name, int OpMode);
  virtual bool DeleteFilesEx(TObjectList * PanelItems, int OpMode);
  virtual int GetFilesEx(TObjectList * PanelItems, bool Move,
    UnicodeString & DestPath, int OpMode);
  virtual int PutFilesEx(TObjectList * PanelItems, bool Move, int OpMode);
  virtual bool ProcessEventEx(int Event, void * Param);

  void ProcessEditorEvent(int Event, void * Param);

  virtual void HandleException(Exception * E, int OpMode = 0);
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
  void ProcessSessions(TObjectList * PanelItems,
    const TProcessSessionEvent & ProcessSession, void * Param);
  void ExportSession(TSessionData * Data, void * Param);
  bool ImportSessions(TObjectList * PanelItems, bool Move, int OpMode);
  void FileProperties();
  void CreateLink();
  void TransferFiles(bool Move);
  void RenameFile();
  void ApplyCommand();
  void ShowInformation();
  void InsertTokenOnCommandLine(const UnicodeString Token, bool Separate);
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
    const UnicodeString Directory,
    // TStrings * GroupList, TStrings * UserList,
    const TRemoteTokenList * GroupList, const TRemoteTokenList * UserList,
    TRemoteProperties * Properties, int AllowedChanges);
  bool ExecuteCommand(const UnicodeString Command);
  void TerminalCaptureLog(const UnicodeString AddedLine, bool StdError);
  bool CopyDialog(bool ToRemote, bool Move, TStrings * FileList,
    UnicodeString & TargetDirectory,
    TGUICopyParamType * Params,
    int Options,
    int CopyParamAttrs);
  bool LinkDialog(UnicodeString & FileName, UnicodeString & PointTo, bool & Symbolic,
    bool Edit, bool AllowSymbolic);
  void FileSystemInfoDialog(const TSessionInfo & SessionInfo,
    const TFileSystemInfo & FileSystemInfo, UnicodeString SpaceAvailablePath,
    const TGetSpaceAvailableEvent & OnGetSpaceAvailable);
  bool OpenDirectoryDialog(bool Add, UnicodeString & Directory,
    TBookmarkList * BookmarkList);
  bool ApplyCommandDialog(UnicodeString & Command, int & Params);
  bool FullSynchronizeDialog(TTerminal::TSynchronizeMode & Mode,
    int & Params, UnicodeString & LocalDirectory, UnicodeString & RemoteDirectory,
    TCopyParamType * CopyParams, bool & SaveSettings, bool & SaveMode, int Options,
    const TUsableCopyParamAttrs & CopyParamAttrs);
  bool SynchronizeChecklistDialog(TSynchronizeChecklist * Checklist,
    TTerminal::TSynchronizeMode Mode, int Params,
    const UnicodeString LocalDirectory, const UnicodeString RemoteDirectory);
  bool RemoteTransferDialog(TStrings * FileList, UnicodeString & Target,
    UnicodeString & FileMask, bool Move);
  bool RenameFileDialog(TRemoteFile * File, UnicodeString & NewName);
  int MoreMessageDialog(const UnicodeString Str, TStrings * MoreMessages,
    TQueryType Type, int Answers, const TMessageParams * Params = NULL);
  bool PasswordDialog(TSessionData * SessionData,
    TPromptKind Kind, const UnicodeString Name, const UnicodeString Instructions, TStrings * Prompts,
    TStrings * Results, bool StoredCredentialsTried);
  bool BannerDialog(const UnicodeString SessionName, const UnicodeString Banner,
    bool & NeverShowAgain, int Options);
  bool CreateDirectoryDialog(UnicodeString & Directory,
    TRemoteProperties * Properties, bool & SaveSettings);
  bool QueueDialog(TTerminalQueueStatus * Status, bool ClosingPlugin);
  bool SynchronizeDialog(TSynchronizeParamType & Params,
    const TCopyParamType * CopyParams, const synchronizestartstop_slot_type & OnStartStop,
    bool & SaveSettings, int Options, int CopyParamAttrs,
    const TGetSynchronizeOptionsEvent & OnGetOptions);
  void DoSynchronize(TSynchronizeController * Sender,
    const UnicodeString LocalDirectory, const UnicodeString RemoteDirectory,
    const TCopyParamType & CopyParam, const TSynchronizeParamType & Params,
    TSynchronizeChecklist ** Checklist, TSynchronizeOptions * Options, bool Full);
  void DoSynchronizeInvalid(TSynchronizeController * Sender,
    const UnicodeString Directory, const UnicodeString ErrorStr);
  void DoSynchronizeTooManyDirectories(TSynchronizeController * Sender,
    int & MaxDirectories);
  void Synchronize(const UnicodeString LocalDirectory,
    const UnicodeString RemoteDirectory, TTerminal::TSynchronizeMode Mode,
    const TCopyParamType & CopyParam, int Params, TSynchronizeChecklist ** Checklist,
    TSynchronizeOptions * Options);
  bool SynchronizeAllowSelectedOnly();
  void GetSynchronizeOptions(int Params, TSynchronizeOptions & Options);
  void RequireCapability(int Capability);
  void RequireLocalPanel(TFarPanelInfo * Panel, const UnicodeString Message);
  bool AreCachesEmpty();
  void ClearCaches();
  void OpenSessionInPutty();
  void QueueShow(bool ClosingPlugin);
  TTerminalQueueStatus * ProcessQueue(bool Hidden);
  bool EnsureCommandSessionFallback(TFSCapability Capability);
  void ConnectTerminal(TTerminal * Terminal);
  void TemporarilyDownloadFiles(TStrings * FileList,
                                TCopyParamType CopyParam, UnicodeString & TempDir);
  int UploadFiles(bool Move, int OpMode, bool Edit, UnicodeString DestPath);
  void UploadOnSave(bool NoReload);
  void UploadFromEditor(bool NoReload, const UnicodeString FileName, const UnicodeString DestPath);
  void LogAuthentication(TTerminal * Terminal, const UnicodeString Msg);
  void MultipleEdit();
  void MultipleEdit(const UnicodeString Directory, const UnicodeString FileName, TRemoteFile * File);
  void EditViewCopyParam(TCopyParamType & CopyParam);
  bool SynchronizeBrowsing(const UnicodeString NewPath);
  bool IsEditHistoryEmpty();
  void EditHistory();
  UnicodeString ProgressBar(size_t Percentage, size_t Width);
  bool IsLogging();
  void ShowLog();

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
  UnicodeString FSavedFindFolder;
  UnicodeString FOriginalEditFile;
  UnicodeString FLastEditFile;
  UnicodeString FLastMultipleEditFile;
  UnicodeString FLastMultipleEditDirectory;
  bool FLastMultipleEditReadOnly;
  int FLastEditorID;
  bool FEditorPendingSave;
  TGUICopyParamType FLastEditCopyParam;
  bool FNoProgress;
  bool FNoProgressFinish;
  TKeepaliveThread * FKeepaliveThread;
  bool FSynchronisingBrowse;
  TSynchronizeController * FSynchronizeController;
  TStrings * FCapturedLog;
  bool FOutputLog;
  TStrings * FAuthenticationLog;
  TWinSCPFileSystem * Self;
  typedef std::map<int, TMultipleEdit> TMultipleEdits;
  TMultipleEdits FMultipleEdits;
  bool FLoadingSessionList;
  typedef std::vector<TEditHistory> TEditHistories;
  TEditHistories FEditHistories;
  UnicodeString FLastPath;
  TStrings * FPathHistory;
  UnicodeString FSessionsFolder;
  UnicodeString FNewSessionsFolder;
  UnicodeString FPrevSessionName;

  void TerminalClose(TObject * Sender);
  void TerminalUpdateStatus(TTerminal * Terminal, bool Active);
  void TerminalChangeDirectory(TObject * Sender);
  void TerminalReadDirectory(TObject * Sender, bool ReloadOnly);
  void TerminalStartReadDirectory(TObject * Sender);
  void TerminalReadDirectoryProgress(TObject * Sender, int Progress,
    bool & Cancel);
  void TerminalInformation(TTerminal * Terminal,
    const UnicodeString Str, bool Status, bool Active);
  void TerminalQueryUser(TObject * Sender,
    const UnicodeString Query, TStrings * MoreMessages, unsigned int Answers,
    const TQueryParams * Params, unsigned int & Answer, TQueryType Type, void * Arg);
  void TerminalPromptUser(TTerminal * Terminal,
    TPromptKind Kind, UnicodeString Name, UnicodeString Instructions,
    TStrings * Prompts, TStrings * Results, bool & Result,
    void * Arg);
  void TerminalDisplayBanner(TTerminal * Terminal,
    UnicodeString SessionName, const UnicodeString Banner, bool & NeverShowAgain,
    int Options);
  void TerminalShowExtendedException(TTerminal * Terminal,
    const std::exception * E, void * Arg);
  void TerminalDeleteLocalFile(const UnicodeString FileName, bool Alternative);
  void OperationProgress(
    TFileOperationProgressType & ProgressData, TCancelStatus & Cancel);
  void OperationFinished(TFileOperation Operation,
    TOperationSide Side, bool DragDrop, const UnicodeString FileName, bool Success,
    TOnceDoneOperation & DisconnectWhenComplete); // ??? bool & DisconnectWhenComplete);
  void CancelConfiguration(TFileOperationProgressType & ProgressData);
  TStrings * CreateFileList(TObjectList * PanelItems,
    TOperationSide Side, bool SelectedOnly = false, UnicodeString Directory = L"",
    bool FileNameOnly = false, TStrings * AFileList = NULL);
  TStrings * CreateSelectedFileList(TOperationSide Side,
      TFarPanelInfo * PanelInfo = NULL);
  TStrings * CreateFocusedFileList(TOperationSide Side,
      TFarPanelInfo * PanelInfo = NULL);
  void CustomCommandGetParamValue(
    const UnicodeString AName, UnicodeString & Value);
  void TerminalSynchronizeDirectory(const UnicodeString LocalDirectory,
    const UnicodeString RemoteDirectory, bool & Continue, bool Collect);
  void QueueListUpdate(TTerminalQueue * Queue);
  void QueueItemUpdate(TTerminalQueue * Queue, TQueueItem * Item);
  void QueueEvent(TTerminalQueue * Queue, TQueueEvent Event);
  void GetSpaceAvailable(const UnicodeString Path,
    TSpaceAvailable & ASpaceAvailable, bool & Close);
  void QueueAddItem(TQueueItem * Item);
};
//---------------------------------------------------------------------------
class TSessionPanelItem : public TCustomFarPanelItem
{
public:
  TSessionPanelItem(const UnicodeString Path);
  TSessionPanelItem(TSessionData * ASessionData);
  static void SetPanelModes(TFarPanelModes * PanelModes);
  static void SetKeyBarTitles(TFarKeyBarTitles * KeyBarTitles);

protected:
  UnicodeString FPath;
  TSessionData * FSessionData;

  virtual void GetData(
    unsigned long & Flags, UnicodeString & FileName, __int64 & Size,
    unsigned long & FileAttributes,
    TDateTime & LastWriteTime, TDateTime & LastAccess,
    unsigned long & NumberOfLinks, UnicodeString & Description,
    UnicodeString & Owner, void *& UserData, int & CustomColumnNumber);
};
//---------------------------------------------------------------------------
class TSessionFolderPanelItem : public TCustomFarPanelItem
{
public:
  TSessionFolderPanelItem(const UnicodeString Folder);

protected:
  UnicodeString FFolder;

  virtual void GetData(
    unsigned long & Flags, UnicodeString & FileName, __int64 & Size,
    unsigned long & FileAttributes,
    TDateTime & LastWriteTime, TDateTime & LastAccess,
    unsigned long & NumberOfLinks, UnicodeString & Description,
    UnicodeString & Owner, void *& UserData, int & CustomColumnNumber);
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
    unsigned long & Flags, UnicodeString & FileName, __int64 & Size,
    unsigned long & FileAttributes,
    TDateTime & LastWriteTime, TDateTime & LastAccess,
    unsigned long & NumberOfLinks, UnicodeString & Description,
    UnicodeString & Owner, void *& UserData, int & CustomColumnNumber);
  virtual UnicodeString GetCustomColumnData(int Column);
  static void TranslateColumnTypes(UnicodeString & ColumnTypes,
    TStrings * ColumnTitles);
};
//---------------------------------------------------------------------------
#endif
