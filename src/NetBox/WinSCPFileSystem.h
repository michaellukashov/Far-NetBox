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
DEFINE_CALLBACK_TYPE2(TGetSynchronizeOptionsEvent, void,
  int /* Params */, TSynchronizeOptions & /* Options */);
DEFINE_CALLBACK_TYPE3(TGetSpaceAvailableEvent, void,
  const UnicodeString & /* Path */, TSpaceAvailable & /* ASpaceAvailable */, bool & /* Close */);

struct TMultipleEdit
{
  UnicodeString FileName;
  UnicodeString FileTitle;
  UnicodeString Directory;
  UnicodeString LocalFileName;
  bool PendingSave;
};
struct TEditHistory
{
  UnicodeString FileName;
  UnicodeString Directory;
  bool operator==(const TEditHistory & rh) const { return (FileName == rh.FileName) && (Directory == rh.Directory); }
};
//---------------------------------------------------------------------------
DEFINE_CALLBACK_TYPE2(TProcessSessionEvent, void, TSessionData * /* Data */, void * /* Param */);
//---------------------------------------------------------------------------
class TWinSCPFileSystem : public TCustomFarFileSystem
{
  friend class TWinSCPPlugin;
  friend class TNetBoxPlugin;
  friend class TKeepaliveThread;
  friend class TQueueDialog;
public:
  explicit TWinSCPFileSystem(TCustomFarPlugin * APlugin);
  virtual void __fastcall Init(TSecureShell * SecureShell);
  virtual ~TWinSCPFileSystem();

  virtual void __fastcall Close();

protected:
  bool __fastcall Connect(TSessionData * Data);
  void __fastcall Disconnect();
  void __fastcall SaveSession();

  virtual void __fastcall GetOpenPanelInfoEx(OPENPANELINFO_FLAGS &Flags,
    UnicodeString & HostFile, UnicodeString & CurDir, UnicodeString & Format,
    UnicodeString & PanelTitle, TFarPanelModes * PanelModes, intptr_t & StartPanelMode,
    OPENPANELINFO_SORTMODES &StartSortMode, bool &StartSortOrder, TFarKeyBarTitles *KeyBarTitles,
    UnicodeString & ShortcutData);
  virtual bool __fastcall GetFindDataEx(TObjectList * PanelItems, int OpMode);
  virtual bool __fastcall ProcessKeyEx(intptr_t Key, uintptr_t ControlState);
  virtual bool __fastcall SetDirectoryEx(const UnicodeString Dir, int OpMode);
  virtual int __fastcall MakeDirectoryEx(UnicodeString & Name, int OpMode);
  virtual bool __fastcall DeleteFilesEx(TObjectList * PanelItems, int OpMode);
  virtual int __fastcall GetFilesEx(TObjectList * PanelItems, bool Move,
    UnicodeString & DestPath, int OpMode);
  virtual int __fastcall PutFilesEx(TObjectList * PanelItems, bool Move, int OpMode);
  virtual bool __fastcall ProcessPanelEventEx(int Event, void *Param);

  void __fastcall ProcessEditorEvent(int Event, void * Param);

  virtual void __fastcall HandleException(Exception * E, int OpMode = 0);
  void KeepaliveThreadCallback();

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
  void DeleteSession(TSessionData * Data, void * Param);
  void __fastcall ProcessSessions(TObjectList * PanelItems,
    TProcessSessionEvent ProcessSession, void * Param);
  void ExportSession(TSessionData * Data, void * Param);
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
  void TerminalCaptureLog(const UnicodeString & AddedLine, bool StdError);
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
  bool __fastcall BannerDialog(const UnicodeString SessionName, const UnicodeString & Banner,
    bool & NeverShowAgain, int Options);
  bool __fastcall CreateDirectoryDialog(UnicodeString & Directory,
    TRemoteProperties * Properties, bool & SaveSettings);
  bool __fastcall QueueDialog(TTerminalQueueStatus * Status, bool ClosingPlugin);
  bool __fastcall SynchronizeDialog(TSynchronizeParamType & Params,
    const TCopyParamType * CopyParams, TSynchronizeStartStopEvent OnStartStop,
    bool & SaveSettings, int Options, int CopyParamAttrs,
    TGetSynchronizeOptionsEvent OnGetOptions);
  void DoSynchronize(TSynchronizeController * Sender,
    const UnicodeString & LocalDirectory, const UnicodeString & RemoteDirectory,
    const TCopyParamType & CopyParam, const TSynchronizeParamType & Params,
    TSynchronizeChecklist ** Checklist, TSynchronizeOptions * Options, bool Full);
  void DoSynchronizeInvalid(TSynchronizeController * Sender,
    const UnicodeString & Directory, const UnicodeString & ErrorStr);
  void DoSynchronizeTooManyDirectories(TSynchronizeController * Sender,
    int & MaxDirectories);
  void __fastcall Synchronize(const UnicodeString LocalDirectory,
    const UnicodeString RemoteDirectory, TTerminal::TSynchronizeMode Mode,
    const TCopyParamType & CopyParam, int Params, TSynchronizeChecklist ** Checklist,
    TSynchronizeOptions * Options);
  bool __fastcall SynchronizeAllowSelectedOnly();
  void GetSynchronizeOptions(int Params, TSynchronizeOptions & Options);
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
  void __fastcall UploadFromEditor(bool NoReload, const UnicodeString FileName,
    const UnicodeString RealFileName, const UnicodeString DestPath);
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

  TTerminal * GetTerminal() { return FTerminal; }

protected:
  virtual UnicodeString GetCurrentDirectory() { return FTerminal ? FTerminal->GetCurrentDirectory() : UnicodeString(); }

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
  UnicodeString FLastMultipleEditFileTitle;
  UnicodeString FLastMultipleEditDirectory;
  bool FLastMultipleEditReadOnly;
  intptr_t FLastEditorID;
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

  void TerminalClose(TObject * Sender);
  void TerminalUpdateStatus(TTerminal * Terminal, bool Active);
  void TerminalChangeDirectory(TObject * Sender);
  void TerminalReadDirectory(TObject * Sender, bool ReloadOnly);
  void TerminalStartReadDirectory(TObject * Sender);
  void TerminalReadDirectoryProgress(TObject * Sender, int Progress,
    bool & Cancel);
  void TerminalInformation(TTerminal * Terminal,
    const UnicodeString & Str, bool Status, int Phase);
  void TerminalQueryUser(TObject * Sender,
    const UnicodeString & Query, TStrings * MoreMessages, unsigned int Answers,
    const TQueryParams * Params, unsigned int & Answer, TQueryType Type, void * Arg);
  void TerminalPromptUser(TTerminal * Terminal,
    TPromptKind Kind, const UnicodeString & Name, const UnicodeString & Instructions,
    TStrings * Prompts, TStrings * Results, bool & Result,
    void * Arg);
  void TerminalDisplayBanner(TTerminal * Terminal,
    UnicodeString SessionName, const UnicodeString & Banner, bool & NeverShowAgain,
    int Options);
  void TerminalShowExtendedException(TTerminal * Terminal,
    Exception * E, void * Arg);
  void TerminalDeleteLocalFile(const UnicodeString & FileName, bool Alternative);
  HANDLE TerminalCreateLocalFile(const UnicodeString & LocalFileName,
    DWORD DesiredAccess, DWORD ShareMode, DWORD CreationDisposition, DWORD FlagsAndAttributes);
  DWORD TerminalGetLocalFileAttributes(const UnicodeString & LocalFileName);
  BOOL TerminalSetLocalFileAttributes(const UnicodeString & LocalFileName, DWORD FileAttributes);
  BOOL TerminalMoveLocalFile(const UnicodeString & LocalFileName, const UnicodeString & NewLocalFileName, DWORD Flags);
  BOOL TerminalRemoveLocalDirectory(const UnicodeString & LocalDirName);
  BOOL TerminalCreateLocalDirectory(const UnicodeString & LocalDirName, LPSECURITY_ATTRIBUTES SecurityAttributes);
  void OperationProgress(
    TFileOperationProgressType & ProgressData, TCancelStatus & Cancel);
  void OperationFinished(TFileOperation Operation,
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
  void CustomCommandGetParamValue(
    const UnicodeString AName, UnicodeString & Value);
  void TerminalSynchronizeDirectory(const UnicodeString & LocalDirectory,
    const UnicodeString & RemoteDirectory, bool & Continue, bool Collect);
  void QueueListUpdate(TTerminalQueue * Queue);
  void QueueItemUpdate(TTerminalQueue * Queue, TQueueItem * Item);
  void QueueEvent(TTerminalQueue * Queue, TQueueEvent Event);
  void GetSpaceAvailable(const UnicodeString & Path,
    TSpaceAvailable & ASpaceAvailable, bool & Close);
  void __fastcall QueueAddItem(TQueueItem * Item);
private:
  UnicodeString __fastcall GetFileNameHash(const UnicodeString FileName);
};
//---------------------------------------------------------------------------
class TSessionPanelItem : public TCustomFarPanelItem
{
public:
  explicit TSessionPanelItem(const UnicodeString Path);
  explicit TSessionPanelItem(TSessionData * ASessionData);
  static void __fastcall SetPanelModes(TFarPanelModes * PanelModes);
  static void __fastcall SetKeyBarTitles(TFarKeyBarTitles * KeyBarTitles);

protected:
  UnicodeString FPath;
  TSessionData * FSessionData;

  virtual void __fastcall GetData(
    PLUGINPANELITEMFLAGS & Flags, UnicodeString & FileName, __int64 & Size,
    uintptr_t & FileAttributes,
    TDateTime & LastWriteTime, TDateTime & LastAccess,
    uintptr_t & NumberOfLinks, UnicodeString & Description,
    UnicodeString & Owner, void *& UserData, size_t & CustomColumnNumber);
};
//---------------------------------------------------------------------------
class TSessionFolderPanelItem : public TCustomFarPanelItem
{
public:
  explicit TSessionFolderPanelItem(const UnicodeString Folder);

protected:
  UnicodeString FFolder;

  virtual void __fastcall GetData(
    PLUGINPANELITEMFLAGS & Flags, UnicodeString & FileName, __int64 & Size,
    uintptr_t & FileAttributes,
    TDateTime & LastWriteTime, TDateTime & LastAccess,
    uintptr_t & NumberOfLinks, UnicodeString & Description,
    UnicodeString & Owner, void *& UserData, size_t & CustomColumnNumber);
};
//---------------------------------------------------------------------------
class TRemoteFilePanelItem : public TCustomFarPanelItem
{
public:
  explicit TRemoteFilePanelItem(TRemoteFile * ARemoteFile);
  static void __fastcall SetPanelModes(TFarPanelModes * PanelModes);
  static void __fastcall SetKeyBarTitles(TFarKeyBarTitles * KeyBarTitles);

protected:
  TRemoteFile * FRemoteFile;

  virtual void __fastcall GetData(
    PLUGINPANELITEMFLAGS & Flags, UnicodeString & FileName, __int64 & Size,
    uintptr_t & FileAttributes,
    TDateTime & LastWriteTime, TDateTime & LastAccess,
    uintptr_t & NumberOfLinks, UnicodeString & Description,
    UnicodeString & Owner, void *& UserData, size_t & CustomColumnNumber);
    virtual UnicodeString __fastcall GetCustomColumnData(int Column);
    static void __fastcall TranslateColumnTypes(UnicodeString & ColumnTypes,
      TStrings * ColumnTitles);
};
//---------------------------------------------------------------------------
#endif
