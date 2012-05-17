//---------------------------------------------------------------------------
#ifndef WinSCPFileSystemH
#define WinSCPFileSystemH
//---------------------------------------------------------------------------
#ifdef _MSC_VER
#include "boostdefines.hpp"
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
#ifndef _MSC_VER
typedef void __fastcall (__closure *TGetSynchronizeOptionsEvent)
  (int Params, TSynchronizeOptions & Options);
typedef void __fastcall (__closure *TGetSpaceAvailable)
  (const AnsiString Path, TSpaceAvailable & ASpaceAvailable, bool & Close);
#else
typedef fastdelegate::FastDelegate2<void,
  int /* Params */, TSynchronizeOptions & /* Options */ > TGetSynchronizeOptionsEvent;
typedef fastdelegate::FastDelegate3<void,
  const UnicodeString & /* Path */, TSpaceAvailable & /* ASpaceAvailable */, bool & /* Close */ > TGetSpaceAvailableEvent;
#endif
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
#ifndef _MSC_VER
typedef void __fastcall (__closure * TProcessSessionEvent)(TSessionData * Data, void * Param);
#else
typedef fastdelegate::FastDelegate2<void, TSessionData *, void *> TProcessSessionEvent;
#endif
//---------------------------------------------------------------------------
class TWinSCPFileSystem : public TCustomFarFileSystem
{
  friend class TWinSCPPlugin;
  friend class TNetBoxPlugin;
  friend class TKeepaliveThread;
  friend class TQueueDialog;
public:
  explicit /* __fastcall */ TWinSCPFileSystem(TCustomFarPlugin * APlugin);
  virtual void __fastcall Init(TSecureShell * SecureShell);
  virtual /* __fastcall */ ~TWinSCPFileSystem();

  virtual void __fastcall Close();

protected:
  bool __fastcall Connect(TSessionData * Data);
  void __fastcall Disconnect();
  void __fastcall SaveSession();

  virtual void __fastcall GetOpenPluginInfoEx(long unsigned & Flags,
    UnicodeString & HostFile, UnicodeString & CurDir, UnicodeString & Format,
    UnicodeString & PanelTitle, TFarPanelModes * PanelModes, int & StartPanelMode,
    int & StartSortMode, bool & StartSortOrder, TFarKeyBarTitles * KeyBarTitles,
    UnicodeString & ShortcutData);
  virtual bool __fastcall GetFindDataEx(TObjectList * PanelItems, int OpMode);
  virtual bool __fastcall ProcessKeyEx(int Key, unsigned int ControlState);
  virtual bool __fastcall SetDirectoryEx(const UnicodeString Dir, int OpMode);
  virtual int __fastcall MakeDirectoryEx(UnicodeString & Name, int OpMode);
  virtual bool __fastcall DeleteFilesEx(TObjectList * PanelItems, int OpMode);
  virtual int __fastcall GetFilesEx(TObjectList * PanelItems, bool Move,
    UnicodeString & DestPath, int OpMode);
  virtual int __fastcall PutFilesEx(TObjectList * PanelItems, bool Move, int OpMode);
  virtual bool __fastcall ProcessEventEx(int Event, void * Param);

  void __fastcall ProcessEditorEvent(int Event, void * Param);

  virtual void __fastcall HandleException(Exception * E, int OpMode = 0);
  void /* __fastcall */ KeepaliveThreadCallback();

  inline bool __fastcall SessionList();
  inline bool __fastcall Connected();
  TWinSCPPlugin * __fastcall WinSCPPlugin();
  void __fastcall ShowOperationProgress(TFileOperationProgressType & ProgressData,
    bool Force);
  bool __fastcall SessionDialog(TSessionData * Data, TSessionActionEnum & Action);
  void __fastcall EditConnectSession(TSessionData * Data, bool Edit);
  void __fastcall DuplicateRenameSession(TSessionData * Data,
    bool Duplicate);
  void __fastcall FocusSession(TSessionData * Data);
  void /* __fastcall */ DeleteSession(TSessionData * Data, void * Param);
  void __fastcall ProcessSessions(TObjectList * PanelItems,
    TProcessSessionEvent ProcessSession, void * Param);
  void /* __fastcall */ ExportSession(TSessionData * Data, void * Param);
  bool __fastcall ImportSessions(TObjectList * PanelItems, bool Move, int OpMode);
  void __fastcall FileProperties();
  void __fastcall CreateLink();
  void __fastcall TransferFiles(bool Move);
  void __fastcall RenameFile();
  void __fastcall ApplyCommand();
  void __fastcall ShowInformation();
  void __fastcall InsertTokenOnCommandLine(UnicodeString Token, bool Separate);
  void __fastcall InsertSessionNameOnCommandLine();
  void __fastcall InsertFileNameOnCommandLine(bool Full);
  void __fastcall InsertPathOnCommandLine();
  void __fastcall CopyFullFileNamesToClipboard();
  void __fastcall FullSynchronize(bool Source);
  void __fastcall Synchronize();
  void __fastcall OpenDirectory(bool Add);
  void __fastcall HomeDirectory();
  void __fastcall ToggleSynchronizeBrowsing();
  bool __fastcall IsSynchronizedBrowsing();
  bool __fastcall PropertiesDialog(TStrings * FileList,
    const UnicodeString Directory,
    // TStrings * GroupList, TStrings * UserList,
    const TRemoteTokenList * GroupList, const TRemoteTokenList * UserList,
    TRemoteProperties * Properties, int AllowedChanges);
  bool __fastcall ExecuteCommand(const UnicodeString Command);
  void /* __fastcall */ TerminalCaptureLog(const UnicodeString & AddedLine, bool StdError);
  bool __fastcall CopyDialog(bool ToRemote, bool Move, TStrings * FileList,
    UnicodeString & TargetDirectory,
    TGUICopyParamType * Params,
    int Options,
    int CopyParamAttrs);
  bool __fastcall LinkDialog(UnicodeString & FileName, UnicodeString & PointTo, bool & Symbolic,
    bool Edit, bool AllowSymbolic);
  void __fastcall FileSystemInfoDialog(const TSessionInfo & SessionInfo,
    const TFileSystemInfo & FileSystemInfo, UnicodeString SpaceAvailablePath,
    TGetSpaceAvailableEvent OnGetSpaceAvailable);
  bool __fastcall OpenDirectoryDialog(bool Add, UnicodeString & Directory,
    TBookmarkList * BookmarkList);
  bool __fastcall ApplyCommandDialog(UnicodeString & Command, int & Params);
  bool __fastcall FullSynchronizeDialog(TTerminal::TSynchronizeMode & Mode,
    int & Params, UnicodeString & LocalDirectory, UnicodeString & RemoteDirectory,
    TCopyParamType * CopyParams, bool & SaveSettings, bool & SaveMode, int Options,
    const TUsableCopyParamAttrs & CopyParamAttrs);
  bool __fastcall SynchronizeChecklistDialog(TSynchronizeChecklist * Checklist,
    TTerminal::TSynchronizeMode Mode, int Params,
    const UnicodeString LocalDirectory, const UnicodeString RemoteDirectory);
  bool __fastcall RemoteTransferDialog(TStrings * FileList, UnicodeString & Target,
    UnicodeString & FileMask, bool Move);
  bool __fastcall RenameFileDialog(TRemoteFile * File, UnicodeString & NewName);
  int __fastcall MoreMessageDialog(const UnicodeString Str, TStrings * MoreMessages,
    TQueryType Type, int Answers, const TMessageParams * Params = NULL);
  bool __fastcall PasswordDialog(TSessionData * SessionData,
    TPromptKind Kind, const UnicodeString Name, const UnicodeString Instructions, TStrings * Prompts,
    TStrings * Results, bool StoredCredentialsTried);
  bool __fastcall BannerDialog(const UnicodeString SessionName, const UnicodeString Banner,
    bool & NeverShowAgain, int Options);
  bool __fastcall CreateDirectoryDialog(UnicodeString & Directory,
    TRemoteProperties * Properties, bool & SaveSettings);
  bool __fastcall QueueDialog(TTerminalQueueStatus * Status, bool ClosingPlugin);
  bool __fastcall SynchronizeDialog(TSynchronizeParamType & Params,
    const TCopyParamType * CopyParams, TSynchronizeStartStopEvent OnStartStop,
    bool & SaveSettings, int Options, int CopyParamAttrs,
    TGetSynchronizeOptionsEvent OnGetOptions);
  void /* __fastcall */ DoSynchronize(TSynchronizeController * Sender,
    const UnicodeString & LocalDirectory, const UnicodeString & RemoteDirectory,
    const TCopyParamType & CopyParam, const TSynchronizeParamType & Params,
    TSynchronizeChecklist ** Checklist, TSynchronizeOptions * Options, bool Full);
  void /* __fastcall */ DoSynchronizeInvalid(TSynchronizeController * Sender,
    const UnicodeString & Directory, const UnicodeString & ErrorStr);
  void /* __fastcall */ DoSynchronizeTooManyDirectories(TSynchronizeController * Sender,
    int & MaxDirectories);
  void __fastcall Synchronize(const UnicodeString LocalDirectory,
    const UnicodeString RemoteDirectory, TTerminal::TSynchronizeMode Mode,
    const TCopyParamType & CopyParam, int Params, TSynchronizeChecklist ** Checklist,
    TSynchronizeOptions * Options);
  bool __fastcall SynchronizeAllowSelectedOnly();
  void /* __fastcall */ GetSynchronizeOptions(int Params, TSynchronizeOptions & Options);
  void __fastcall RequireCapability(int Capability);
  void __fastcall RequireLocalPanel(TFarPanelInfo * Panel, const UnicodeString Message);
  bool __fastcall AreCachesEmpty();
  void __fastcall ClearCaches();
  void __fastcall OpenSessionInPutty();
  void __fastcall QueueShow(bool ClosingPlugin);
  TTerminalQueueStatus * __fastcall ProcessQueue(bool Hidden);
  bool __fastcall EnsureCommandSessionFallback(TFSCapability Capability);
  void __fastcall ConnectTerminal(TTerminal * Terminal);
  void __fastcall TemporarilyDownloadFiles(TStrings * FileList,
    TCopyParamType CopyParam, UnicodeString & TempDir);
  int __fastcall UploadFiles(bool Move, int OpMode, bool Edit, UnicodeString DestPath);
  void __fastcall UploadOnSave(bool NoReload);
  void __fastcall UploadFromEditor(bool NoReload, const UnicodeString FileName, const UnicodeString DestPath);
  void __fastcall LogAuthentication(TTerminal * Terminal, const UnicodeString Msg);
  void __fastcall MultipleEdit();
  void __fastcall MultipleEdit(const UnicodeString Directory, const UnicodeString FileName, TRemoteFile * File);
  void __fastcall EditViewCopyParam(TCopyParamType & CopyParam);
  bool __fastcall SynchronizeBrowsing(const UnicodeString NewPath);
  bool __fastcall IsEditHistoryEmpty();
  void __fastcall EditHistory();
  UnicodeString __fastcall ProgressBar(int Percentage, int Width);
  bool __fastcall IsLogging();
  void __fastcall ShowLog();

#ifndef _MSC_VER
  __property TTerminal * Terminal = { read = FTerminal };
#else
  TTerminal * GetTerminal() { return FTerminal; }
#endif

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
  TWinSCPFileSystem * Self;

  void /* __fastcall */ TerminalClose(TObject * Sender);
  void /* __fastcall */ TerminalUpdateStatus(TTerminal * Terminal, bool Active);
  void /* __fastcall */ TerminalChangeDirectory(TObject * Sender);
  void /* __fastcall */ TerminalReadDirectory(TObject * Sender, bool ReloadOnly);
  void /* __fastcall */ TerminalStartReadDirectory(TObject * Sender);
  void /* __fastcall */ TerminalReadDirectoryProgress(TObject * Sender, int Progress,
    bool & Cancel);
  void /* __fastcall */ TerminalInformation(TTerminal * Terminal,
    const UnicodeString & Str, bool Status, int Phase);
  void /* __fastcall */ TerminalQueryUser(TObject * Sender,
    const UnicodeString & Query, TStrings * MoreMessages, unsigned int Answers,
    const TQueryParams * Params, unsigned int & Answer, TQueryType Type, void * Arg);
  void /* __fastcall */ TerminalPromptUser(TTerminal * Terminal,
    TPromptKind Kind, UnicodeString Name, UnicodeString Instructions,
    TStrings * Prompts, TStrings * Results, bool & Result,
    void * Arg);
  void /* __fastcall */ TerminalDisplayBanner(TTerminal * Terminal,
    UnicodeString & SessionName, const UnicodeString & Banner, bool & NeverShowAgain,
    int Options);
  void /* __fastcall */ TerminalShowExtendedException(TTerminal * Terminal,
    Exception * E, void * Arg);
  void /* __fastcall */ TerminalDeleteLocalFile(const UnicodeString & FileName, bool Alternative);
  void /* __fastcall */ OperationProgress(
    TFileOperationProgressType & ProgressData, TCancelStatus & Cancel);
  void /* __fastcall */ OperationFinished(TFileOperation Operation,
    TOperationSide Side, bool DragDrop, const UnicodeString & FileName, bool Success,
    TOnceDoneOperation & DisconnectWhenComplete);
  void __fastcall CancelConfiguration(TFileOperationProgressType & ProgressData);
  TStrings * __fastcall CreateFileList(TObjectList * PanelItems,
    TOperationSide Side, bool SelectedOnly = false, UnicodeString Directory = L"",
    bool FileNameOnly = false, TStrings * AFileList = NULL);
  TStrings * __fastcall CreateSelectedFileList(TOperationSide Side,
    TFarPanelInfo * PanelInfo = NULL);
  TStrings * __fastcall CreateFocusedFileList(TOperationSide Side,
    TFarPanelInfo * PanelInfo = NULL);
  void /* __fastcall */ CustomCommandGetParamValue(
    const UnicodeString AName, UnicodeString & Value);
  void /* __fastcall */ TerminalSynchronizeDirectory(const UnicodeString & LocalDirectory,
    const UnicodeString & RemoteDirectory, bool & Continue, bool Collect);
  void /* __fastcall */ QueueListUpdate(TTerminalQueue * Queue);
  void /* __fastcall */ QueueItemUpdate(TTerminalQueue * Queue, TQueueItem * Item);
  void /* __fastcall */ QueueEvent(TTerminalQueue * Queue, TQueueEvent Event);
  void /* __fastcall */ GetSpaceAvailable(const UnicodeString & Path,
    TSpaceAvailable & ASpaceAvailable, bool & Close);
  void __fastcall QueueAddItem(TQueueItem * Item);
};
//---------------------------------------------------------------------------
class TSessionPanelItem : public TCustomFarPanelItem
{
public:
  explicit /* __fastcall */ TSessionPanelItem(const UnicodeString Path);
  explicit /* __fastcall */ TSessionPanelItem(TSessionData * ASessionData);
  static void __fastcall SetPanelModes(TFarPanelModes * PanelModes);
  static void __fastcall SetKeyBarTitles(TFarKeyBarTitles * KeyBarTitles);

protected:
  UnicodeString FPath;
  TSessionData * FSessionData;

  virtual void __fastcall GetData(
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
  explicit /* __fastcall */ TSessionFolderPanelItem(const UnicodeString Folder);

protected:
  UnicodeString FFolder;

  virtual void __fastcall GetData(
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
  explicit /* __fastcall */ TRemoteFilePanelItem(TRemoteFile * ARemoteFile);
  static void __fastcall SetPanelModes(TFarPanelModes * PanelModes);
  static void __fastcall SetKeyBarTitles(TFarKeyBarTitles * KeyBarTitles);

protected:
  TRemoteFile * FRemoteFile;

  virtual void __fastcall GetData(
    unsigned long & Flags, UnicodeString & FileName, __int64 & Size,
    unsigned long & FileAttributes,
    TDateTime & LastWriteTime, TDateTime & LastAccess,
    unsigned long & NumberOfLinks, UnicodeString & Description,
    UnicodeString & Owner, void *& UserData, int & CustomColumnNumber);
  virtual UnicodeString __fastcall GetCustomColumnData(int Column);
  static void __fastcall TranslateColumnTypes(UnicodeString & ColumnTypes,
    TStrings * ColumnTitles);
};
//---------------------------------------------------------------------------
#endif
