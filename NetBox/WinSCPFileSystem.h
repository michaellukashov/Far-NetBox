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
#define REMOTE_DIR_HISTORY "WinscpRemoteDirectory"
#define ASCII_MASK_HISTORY "WinscpAsciiMask"
#define LINK_FILENAME_HISTORY "WinscpRemoteLink"
#define LINK_POINT_TO_HISTORY "WinscpRemoteLinkPointTo"
#define APPLY_COMMAND_HISTORY "WinscpApplyCmd"
#define APPLY_COMMAND_PARAM_HISTORY "WinscpApplyCmdParam"
#define LOG_FILE_HISTORY "WinscpLogFile"
#define REMOTE_SYNC_HISTORY "WinscpRemoteSync"
#define LOCAL_SYNC_HISTORY "WinscpLocalSync"
#define MOVE_TO_HISTORY "WinscpMoveTo"
#define EXCLUDE_FILE_MASK_HISTORY "WinscpExcludeFileMask"
#define MAKE_SESSION_FOLDER_HISTORY "WinscpSessionFolder"
//---------------------------------------------------------------------------
// for Properties dialog
const cpMode =  0x01;
const cpOwner = 0x02;
const cpGroup = 0x04;
// for Copy dialog
const coTempTransfer        = 0x01;
const coDisableNewerOnly    = 0x04;
// for Synchronize and FullSynchronize dialogs
const spSelectedOnly = 0x800;
// for Synchronize dialogs
const soAllowSelectedOnly = 0x01;
// for FullSynchronize dialog
const fsoDisableTimestamp = 0x01;
const fsoAllowSelectedOnly = 0x02;
enum TSessionAction { saAdd, saEdit, saConnect };
//---------------------------------------------------------------------------
typedef void __fastcall (__closure *TGetSynchronizeOptionsEvent)
  (int Params, TSynchronizeOptions & Options);
typedef void __fastcall (__closure *TGetSpaceAvailable)
  (const AnsiString Path, TSpaceAvailable & ASpaceAvailable, bool & Close);
struct TMultipleEdit
{
  AnsiString FileName;
  AnsiString Directory;
  AnsiString LocalFileName;
  bool PendingSave;
};
struct TEditHistory
{
  AnsiString FileName;
  AnsiString Directory;
  bool operator==(const TEditHistory& rh) { return (FileName == rh.FileName) && (Directory == rh.Directory); }
};
//---------------------------------------------------------------------------
typedef void __fastcall (__closure * TProcessSessionEvent)(TSessionData * Data, void * Param);
//---------------------------------------------------------------------------
class TWinSCPFileSystem : public TCustomFarFileSystem
{
friend class TWinSCPPlugin;
friend class TKeepaliveThread;
friend class TQueueDialog;
public:
  __fastcall TWinSCPFileSystem(TCustomFarPlugin * APlugin);
  virtual __fastcall ~TWinSCPFileSystem();

  virtual void __fastcall Close();

protected:
  bool __fastcall Connect(TSessionData * Data);
  void __fastcall SaveSession();

  virtual void __fastcall GetOpenPluginInfoEx(long unsigned & Flags,
    AnsiString & HostFile, AnsiString & CurDir, AnsiString & Format,
    AnsiString & PanelTitle, TFarPanelModes * PanelModes, int & StartPanelMode,
    int & StartSortMode, bool & StartSortOrder, TFarKeyBarTitles * KeyBarTitles,
    AnsiString & ShortcutData);
  virtual bool __fastcall GetFindDataEx(TList * PanelItems, int OpMode);
  virtual bool __fastcall ProcessKeyEx(int Key, unsigned int ControlState);
  virtual bool __fastcall SetDirectoryEx(const AnsiString Dir, int OpMode);
  virtual int __fastcall MakeDirectoryEx(AnsiString & Name, int OpMode);
  virtual bool __fastcall DeleteFilesEx(TList * PanelItems, int OpMode);
  virtual int __fastcall GetFilesEx(TList * PanelItems, bool Move,
    AnsiString & DestPath, int OpMode);
  virtual int __fastcall PutFilesEx(TList * PanelItems, bool Move, int OpMode);
  virtual bool __fastcall ProcessEventEx(int Event, void * Param);

  void __fastcall ProcessEditorEvent(int Event, void * Param);

  virtual void __fastcall HandleException(Exception * E, int OpMode = 0);
  void __fastcall KeepaliveThreadCallback();

  inline bool __fastcall SessionList();
  inline bool __fastcall Connected();
  TWinSCPPlugin * __fastcall WinSCPPlugin();
  void __fastcall ShowOperationProgress(TFileOperationProgressType & ProgressData,
    bool Force);
  bool __fastcall SessionDialog(TSessionData * Data, TSessionAction & Action);
  void __fastcall EditConnectSession(TSessionData * Data, bool Edit);
  void __fastcall DuplicateRenameSession(TSessionData * Data,
    bool Duplicate);
  void __fastcall FocusSession(TSessionData * Data);
  void __fastcall DeleteSession(TSessionData * Data, void * Param);
  void __fastcall ProcessSessions(TList * PanelItems,
    TProcessSessionEvent ProcessSession, void * Param);
  void __fastcall ExportSession(TSessionData * Data, void * Param);
  bool __fastcall ImportSessions(TList * PanelItems, bool Move, int OpMode);
  void __fastcall FileProperties();
  void __fastcall CreateLink();
  void __fastcall TransferFiles(bool Move);
  void __fastcall RenameFile();
  void __fastcall ApplyCommand();
  void __fastcall ShowInformation();
  void __fastcall InsertTokenOnCommandLine(AnsiString Token, bool Separate);
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
    const AnsiString Directory, TStrings * GroupList, TStrings * UserList,
    TRemoteProperties * Properties, int AllowedChanges);
  bool __fastcall ExecuteCommand(const AnsiString Command);
  void __fastcall TerminalCaptureLog(const AnsiString & AddedLine, bool StdError);
  bool __fastcall CopyDialog(bool ToRemote, bool Move, TStrings * FileList,
    AnsiString & TargetDirectory, TGUICopyParamType * Params, int Options,
    int CopyParamAttrs);
  bool __fastcall LinkDialog(AnsiString & FileName, AnsiString & PointTo, bool & Symbolic,
    bool Edit, bool AllowSymbolic);
  void __fastcall FileSystemInfoDialog(const TSessionInfo & SessionInfo,
    const TFileSystemInfo & FileSystemInfo, AnsiString SpaceAvailablePath,
    TGetSpaceAvailable OnGetSpaceAvailable);
  bool __fastcall OpenDirectoryDialog(bool Add, AnsiString & Directory,
    TBookmarkList * BookmarkList);
  bool __fastcall ApplyCommandDialog(AnsiString & Command, int & Params);
  bool __fastcall FullSynchronizeDialog(TTerminal::TSynchronizeMode & Mode,
    int & Params, AnsiString & LocalDirectory, AnsiString & RemoteDirectory,
    TCopyParamType * CopyParams, bool & SaveSettings, bool & SaveMode, int Options,
    const TUsableCopyParamAttrs & CopyParamAttrs);
  bool __fastcall SynchronizeChecklistDialog(TSynchronizeChecklist * Checklist,
    TTerminal::TSynchronizeMode Mode, int Params,
    const AnsiString LocalDirectory, const AnsiString RemoteDirectory);
  bool __fastcall RemoteTransferDialog(TStrings * FileList, AnsiString & Target,
    AnsiString & FileMask, bool Move);
  bool __fastcall RenameFileDialog(TRemoteFile * File, AnsiString & NewName);
  int __fastcall MoreMessageDialog(AnsiString Str, TStrings * MoreMessages,
    TQueryType Type, int Answers, const TMessageParams * Params = NULL);
  bool __fastcall PasswordDialog(TSessionData * SessionData,
    TPromptKind Kind, AnsiString Name, AnsiString Instructions, TStrings * Prompts,
    TStrings * Results, bool StoredCredentialsTried);
  bool __fastcall BannerDialog(AnsiString SessionName, const AnsiString & Banner,
    bool & NeverShowAgain, int Options);
  bool __fastcall CreateDirectoryDialog(AnsiString & Directory,
    TRemoteProperties * Properties, bool & SaveSettings);
  bool __fastcall QueueDialog(TTerminalQueueStatus * Status, bool ClosingPlugin);
  bool __fastcall SynchronizeDialog(TSynchronizeParamType & Params,
    const TCopyParamType * CopyParams, TSynchronizeStartStopEvent OnStartStop,
    bool & SaveSettings, int Options, int CopyParamAttrs,
    TGetSynchronizeOptionsEvent OnGetOptions);
  void __fastcall DoSynchronize(TSynchronizeController * Sender,
    const AnsiString LocalDirectory, const AnsiString RemoteDirectory,
    const TCopyParamType & CopyParam, const TSynchronizeParamType & Params,
    TSynchronizeChecklist ** Checklist, TSynchronizeOptions * Options, bool Full);
  void __fastcall DoSynchronizeInvalid(TSynchronizeController * Sender,
    const AnsiString Directory, const AnsiString ErrorStr);
  void __fastcall DoSynchronizeTooManyDirectories(TSynchronizeController * Sender,
    int & MaxDirectories);
  void __fastcall Synchronize(const AnsiString LocalDirectory,
    const AnsiString RemoteDirectory, TTerminal::TSynchronizeMode Mode,
    const TCopyParamType & CopyParam, int Params, TSynchronizeChecklist ** Checklist,
    TSynchronizeOptions * Options);
  bool __fastcall SynchronizeAllowSelectedOnly();
  void __fastcall GetSynchronizeOptions(int Params, TSynchronizeOptions & Options);
  void __fastcall RequireCapability(int Capability);
  void __fastcall RequireLocalPanel(TFarPanelInfo * Panel, AnsiString Message);
  bool __fastcall AreCachesEmpty();
  void __fastcall ClearCaches();
  void __fastcall OpenSessionInPutty();
  void __fastcall QueueShow(bool ClosingPlugin);
  TTerminalQueueStatus * __fastcall ProcessQueue(bool Hidden);
  bool __fastcall EnsureCommandSessionFallback(TFSCapability Capability);
  void __fastcall ConnectTerminal(TTerminal * Terminal);
  void __fastcall TemporarilyDownloadFiles(TStrings * FileList,
    TCopyParamType CopyParam, AnsiString & TempDir);
  int __fastcall UploadFiles(bool Move, int OpMode, bool Edit, AnsiString DestPath);
  void __fastcall UploadOnSave(bool NoReload);
  void __fastcall UploadFromEditor(bool NoReload, AnsiString FileName, AnsiString DestPath);
  void __fastcall LogAuthentication(TTerminal * Terminal, AnsiString Msg);
  void __fastcall MultipleEdit();
  void __fastcall MultipleEdit(AnsiString Directory, AnsiString FileName, TRemoteFile * File);
  void __fastcall EditViewCopyParam(TCopyParamType & CopyParam);
  bool __fastcall SynchronizeBrowsing(AnsiString NewPath);
  bool __fastcall IsEditHistoryEmpty();
  void __fastcall EditHistory();
  AnsiString __fastcall ProgressBar(int Percentage, int Width);
  bool __fastcall IsLogging();
  void __fastcall ShowLog();

  __property TTerminal * Terminal = { read = FTerminal };

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
  THandle FProgressSaveScreenHandle;
  THandle FSynchronizationSaveScreenHandle;
  THandle FAuthenticationSaveScreenHandle;
  TDateTime FSynchronizationStart;
  bool FSynchronizationCompare;
  TStrings * FFileList;
  TList * FPanelItems;
  AnsiString FSavedFindFolder;
  AnsiString FOriginalEditFile;
  AnsiString FLastEditFile;
  AnsiString FLastMultipleEditFile;
  AnsiString FLastMultipleEditDirectory;
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
  AnsiString FLastPath;
  TStrings * FPathHistory;
  AnsiString FSessionsFolder;
  AnsiString FNewSessionsFolder;

  void __fastcall TerminalClose(TObject * Sender);
  void __fastcall TerminalUpdateStatus(TTerminal * Terminal, bool Active);
  void __fastcall TerminalChangeDirectory(TObject * Sender);
  void __fastcall TerminalReadDirectory(TObject* Sender, bool ReloadOnly);
  void __fastcall TerminalStartReadDirectory(TObject * Sender);
  void __fastcall TerminalReadDirectoryProgress(TObject * Sender, int Progress,
    bool & Cancel);
  void __fastcall TerminalInformation(TTerminal * Terminal,
    const AnsiString & Str, bool Status, bool Active);
  void __fastcall TerminalQueryUser(TObject * Sender,
    const AnsiString Query, TStrings * MoreMessages, int Answers,
    const TQueryParams * Params, int & Answer, TQueryType Type, void * Arg);
  void __fastcall TerminalPromptUser(TTerminal * Terminal,
    TPromptKind Kind, AnsiString Name, AnsiString Instructions,
    TStrings * Prompts, TStrings * Results, bool & Result,
    void * Arg);
  void __fastcall TerminalDisplayBanner(TTerminal * Terminal,
    AnsiString SessionName, const AnsiString & Banner, bool & NeverShowAgain,
    int Options);
  void __fastcall TerminalShowExtendedException(TTerminal * Terminal,
    Exception * E, void * Arg);
  void __fastcall TerminalDeleteLocalFile(const AnsiString FileName, bool Alternative);
  void __fastcall OperationProgress(
    TFileOperationProgressType & ProgressData, TCancelStatus & Cancel);
  void __fastcall OperationFinished(TFileOperation Operation,
    TOperationSide Side, bool DragDrop, const AnsiString & FileName, bool Success,
    bool & DisconnectWhenComplete);
  void __fastcall CancelConfiguration(TFileOperationProgressType & ProgressData);
  TStrings * __fastcall CreateFileList(TList * PanelItems,
    TOperationSide Side, bool SelectedOnly = false, AnsiString Directory = "",
    bool FileNameOnly = false, TStrings * AFileList = NULL);
  TStrings * __fastcall CreateSelectedFileList(TOperationSide Side,
    TFarPanelInfo * PanelInfo = NULL);
  TStrings * __fastcall CreateFocusedFileList(TOperationSide Side,
    TFarPanelInfo * PanelInfo = NULL);
  void __fastcall CustomCommandGetParamValue(
    const AnsiString AName, AnsiString & Value);
  void __fastcall TerminalSynchronizeDirectory(const AnsiString LocalDirectory,
    const AnsiString RemoteDirectory, bool & Continue, bool Collect);
  void __fastcall QueueListUpdate(TTerminalQueue * Queue);
  void __fastcall QueueItemUpdate(TTerminalQueue * Queue, TQueueItem * Item);
  void __fastcall QueueEvent(TTerminalQueue * Queue, TQueueEvent Event);
  void __fastcall GetSpaceAvailable(const AnsiString Path,
    TSpaceAvailable & ASpaceAvailable, bool & Close);
  void __fastcall QueueAddItem(TQueueItem * Item);
};
//---------------------------------------------------------------------------
class TSessionPanelItem : public TCustomFarPanelItem
{
public:
  __fastcall TSessionPanelItem(AnsiString Path);
  __fastcall TSessionPanelItem(TSessionData * ASessionData);
  static void __fastcall SetPanelModes(TFarPanelModes * PanelModes);
  static void __fastcall SetKeyBarTitles(TFarKeyBarTitles * KeyBarTitles);

protected:
  AnsiString FPath;
  TSessionData * FSessionData;

  virtual void __fastcall GetData(
    unsigned long & Flags, AnsiString & FileName, __int64 & Size,
    unsigned long & FileAttributes,
    TDateTime & LastWriteTime, TDateTime & LastAccess,
    unsigned long & NumberOfLinks, AnsiString & Description,
    AnsiString & Owner, void *& UserData, int & CustomColumnNumber);
};
//---------------------------------------------------------------------------
class TSessionFolderPanelItem : public TCustomFarPanelItem
{
public:
  __fastcall TSessionFolderPanelItem(AnsiString Folder);

protected:
  AnsiString FFolder;

  virtual void __fastcall GetData(
    unsigned long & Flags, AnsiString & FileName, __int64 & Size,
    unsigned long & FileAttributes,
    TDateTime & LastWriteTime, TDateTime & LastAccess,
    unsigned long & NumberOfLinks, AnsiString & Description,
    AnsiString & Owner, void *& UserData, int & CustomColumnNumber);
};
//---------------------------------------------------------------------------
class TRemoteFilePanelItem : public TCustomFarPanelItem
{
public:
  __fastcall TRemoteFilePanelItem(TRemoteFile * ARemoteFile);
  static void __fastcall SetPanelModes(TFarPanelModes * PanelModes);
  static void __fastcall SetKeyBarTitles(TFarKeyBarTitles * KeyBarTitles);

protected:
  TRemoteFile * FRemoteFile;

  virtual void __fastcall GetData(
    unsigned long & Flags, AnsiString & FileName, __int64 & Size,
    unsigned long & FileAttributes,
    TDateTime & LastWriteTime, TDateTime & LastAccess,
    unsigned long & NumberOfLinks, AnsiString & Description,
    AnsiString & Owner, void *& UserData, int & CustomColumnNumber);
  virtual AnsiString __fastcall CustomColumnData(int Column);
  static void __fastcall TranslateColumnTypes(AnsiString & ColumnTypes,
    TStrings * ColumnTitles);
};
//---------------------------------------------------------------------------
#endif
