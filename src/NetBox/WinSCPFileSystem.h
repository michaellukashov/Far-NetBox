#pragma once

#include <Interface.h>
#include "FarPlugin.h"
#include <FileOperationProgress.h>
#include <Terminal.h>
#include <GUIConfiguration.h>
#include <SynchronizeController.h>
#include <Queue.h>
#include <WinInterface.h>

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
class TKeepAliveThread;
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
#define WINSCP_FILE_MASK_HISTORY "WinscpFileMask"
#define MAKE_SESSION_FOLDER_HISTORY "WinscpSessionFolder"

#if 0
// for Properties dialog
const int cpMode  = 0x01;
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
#endif //if 0
enum TSessionActionEnum
{
  saAdd,
  saEdit,
  saConnect
};

struct TMultipleEdit : public TObject
{
  UnicodeString FileName;
  UnicodeString FileTitle;
  UnicodeString Directory;
  UnicodeString LocalFileName;
  bool PendingSave{false};
};

struct TEditHistory : public TObject
{
  UnicodeString FileName;
  UnicodeString Directory;
  bool operator==(const TEditHistory &rh) const { return (FileName == rh.FileName) && (Directory == rh.Directory); }
};

#if 0
typedef void (__closure *TProcessSessionEvent)(TSessionData *Data, void *Param);
#endif // #if 0
using TProcessSessionEvent = nb::FastDelegate2<void, TSessionData * /*Data*/, void * /*Param*/>;

NB_DEFINE_CLASS_ID(TWinSCPFileSystem);
class TWinSCPFileSystem : public TCustomFarFileSystem
{
  friend class TWinSCPPlugin;
  friend class TNetBoxPlugin;
  friend class TKeepAliveThread;
  friend class TQueueDialog;
  NB_DISABLE_COPY(TWinSCPFileSystem)
public:
  static bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TWinSCPFileSystem); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TWinSCPFileSystem) || TCustomFarFileSystem::is(Kind); }
public:
  explicit TWinSCPFileSystem(TCustomFarPlugin *APlugin) noexcept;
  void Init(TSecureShell *SecureShell);
  virtual ~TWinSCPFileSystem() noexcept;

  void Close() override;

protected:
  bool Connect(TSessionData *Data);
  void Disconnect();
  void SaveSession();

  void GetOpenPluginInfoEx(DWORD &Flags,
    UnicodeString &HostFile, UnicodeString &CurDir, UnicodeString &AFormat,
    UnicodeString &PanelTitle, TFarPanelModes *PanelModes, int &StartPanelMode,
    int &StartSortMode, bool &StartSortOrder, TFarKeyBarTitles *KeyBarTitles,
    UnicodeString &ShortcutData) override;
  bool GetFindDataEx(TObjectList *PanelItems, int OpMode) override;
  bool ProcessKeyEx(intptr_t Key, uintptr_t ControlState) override;
  bool SetDirectoryEx(UnicodeString Dir, int OpMode) override;
  intptr_t MakeDirectoryEx(UnicodeString &Name, int OpMode) override;
  bool DeleteFilesEx(TObjectList *PanelItems, int OpMode) override;
  intptr_t GetFilesEx(TObjectList *PanelItems, bool Move,
    UnicodeString &DestPath, int OpMode) override;
  intptr_t PutFilesEx(TObjectList *PanelItems, bool Move, int OpMode) override;
  bool ProcessEventEx(intptr_t Event, void *Param) override;

  void ProcessEditorEvent(intptr_t Event, void *Param);

  void HandleException(Exception *E, int OpMode = 0) override;
  void KeepaliveThreadCallback();

  bool IsSessionList() const;
  bool Connected() const;
  TWinSCPPlugin *GetWinSCPPlugin();
  void ShowOperationProgress(TFileOperationProgressType &ProgressData,
    bool Force);
  bool SessionDialog(TSessionData *SessionData, TSessionActionEnum &Action);
  void EditConnectSession(TSessionData *Data, bool Edit);
  void EditConnectSession(TSessionData *Data, bool Edit, bool NewData, bool FillInConnect);
  void DuplicateOrRenameSession(TSessionData *Data,
    bool Duplicate);
  void FocusSession(const TSessionData *Data);
  void DeleteSession(TSessionData *Data, void *AParam);
  void ProcessSessions(TObjectList *PanelItems,
    TProcessSessionEvent ProcessSession, void *AParam);
  void ExportSession(TSessionData *Data, void *AParam);
  bool ImportSessions(TObjectList *PanelItems, bool Move, int OpMode);
  void FileProperties();
  void RemoteCreateLink();
  void TransferFiles(bool Move);
  void RenameFile();
  void ApplyCommand();
  void ShowInformation();
  void InsertTokenOnCommandLine(const UnicodeString Token, bool Separate);
  void InsertSessionNameOnCommandLine();
  void InsertFileNameOnCommandLine(bool Full);
  UnicodeString GetFullFilePath(const TRemoteFile *AFile) const;
  void InsertPathOnCommandLine();
  void CopyFullFileNamesToClipboard();
  void FullSynchronize(bool Source);
  void Synchronize();
  void OpenDirectory(bool Add);
  void HomeDirectory();
  void ToggleSynchronizeBrowsing();
  bool IsSynchronizedBrowsing() const;
  bool PropertiesDialog(TStrings *AFileList,
    UnicodeString Directory,
    const TRemoteTokenList *GroupList, const TRemoteTokenList *UserList,
    TRemoteProperties *Properties, intptr_t AllowedChanges);
  bool ExecuteCommand(const UnicodeString Command);
  void TerminalCaptureLog(const UnicodeString AddedLine, TCaptureOutputType OutputEvent);
  bool CopyDialog(bool ToRemote, bool Move, const TStrings *AFileList,
    intptr_t Options,
    intptr_t CopyParamAttrs,
    UnicodeString &TargetDirectory,
    TGUICopyParamType *Params);
  bool LinkDialog(UnicodeString &AFileName, UnicodeString &PointTo, bool &Symbolic,
    bool Edit, bool AllowSymbolic);
  void FileSystemInfoDialog(const TSessionInfo &SessionInfo,
    const TFileSystemInfo &FileSystemInfo, UnicodeString SpaceAvailablePath,
    TGetSpaceAvailableEvent OnGetSpaceAvailable);
  bool OpenDirectoryDialog(bool Add, UnicodeString &Directory,
    TBookmarkList *BookmarkList);
  bool ApplyCommandDialog(UnicodeString &Command, intptr_t &Params) const;
  bool FullSynchronizeDialog(TTerminal::TSynchronizeMode &Mode,
    intptr_t &Params, UnicodeString &LocalDirectory, UnicodeString &RemoteDirectory,
    TCopyParamType *CopyParams, bool &SaveSettings, bool &SaveMode, intptr_t Options,
    const TUsableCopyParamAttrs &CopyParamAttrs) const;
  bool SynchronizeChecklistDialog(TSynchronizeChecklist *Checklist,
    TTerminal::TSynchronizeMode Mode, intptr_t Params,
    const UnicodeString LocalDirectory, const UnicodeString RemoteDirectory);
  bool RemoteTransferDialog(TStrings *AFileList, UnicodeString &Target,
    UnicodeString &FileMask, bool Move);
  bool RenameFileDialog(TRemoteFile *AFile, UnicodeString &NewName);
  uintptr_t MoreMessageDialog(const UnicodeString Str, TStrings *MoreMessages,
    TQueryType Type, uintptr_t Answers, const TMessageParams *AParams = nullptr);
  bool PasswordDialog(TSessionData *SessionData,
    TPromptKind Kind, const UnicodeString Name, const UnicodeString Instructions, TStrings *Prompts,
    TStrings *Results, bool StoredCredentialsTried);
  bool BannerDialog(const UnicodeString SessionName, const UnicodeString Banner,
    bool &NeverShowAgain, intptr_t Options);
  bool CreateDirectoryDialog(UnicodeString &Directory,
    TRemoteProperties *Properties, bool &SaveSettings);
  bool QueueDialog(TTerminalQueueStatus *Status, bool ClosingPlugin);
  bool SynchronizeDialog(TSynchronizeParamType &Params,
    const TCopyParamType *CopyParams, TSynchronizeStartStopEvent OnStartStop,
    bool &SaveSettings, uintptr_t Options, intptr_t CopyParamAttrs,
    TGetSynchronizeOptionsEvent OnGetOptions);
  bool SynchronizeAllowSelectedOnly();
  void RequireCapability(intptr_t Capability);
  void RequireLocalPanel(TFarPanelInfo *Panel, const UnicodeString Message);
  bool AreCachesEmpty() const;
  void ClearCaches();
  void OpenSessionInPutty();
  void QueueShow(bool ClosingPlugin);
  TTerminalQueueStatus *ProcessQueue(bool Hidden);
  bool EnsureCommandSessionFallback(TFSCapability Capability);
  void ConnectTerminal(TTerminal *Terminal);
  void TemporarilyDownloadFiles(TStrings *AFileList,
    TCopyParamType &CopyParam, UnicodeString &TempDir);
  intptr_t UploadFiles(bool Move, int OpMode, bool Edit, UnicodeString &DestPath);
  void UploadOnSave(bool NoReload);
  void UploadFromEditor(bool NoReload, const UnicodeString AFileName,
    UnicodeString RealFileName, UnicodeString &DestPath);
  void LogAuthentication(TTerminal *Terminal, const UnicodeString Msg);
  void MultipleEdit();
  void MultipleEdit(const UnicodeString Directory, const UnicodeString AFileName, TRemoteFile *AFile);
  void EditViewCopyParam(TCopyParamType &CopyParam);
  bool SynchronizeBrowsing(const UnicodeString NewPath);
  bool IsEditHistoryEmpty() const;
  void EditHistory();
  UnicodeString ProgressBar(intptr_t Percentage, intptr_t Width);
  bool IsLogging() const;
  void ShowLog();

  TTerminal *GetTerminal() const { return FTerminal; }
  TSessionData *GetSessionData() const { return FTerminal ? FTerminal->GetSessionData() : nullptr; }
  TSessionData *GetSessionData() { return FTerminal ? FTerminal->GetSessionData() : nullptr; }

protected:
  UnicodeString GetCurrDirectory() const override { return FTerminal ? FTerminal->RemoteGetCurrentDirectory() : UnicodeString(); }

private:
  bool TerminalCheckForEsc();
  void TerminalClose(TObject *Sender);
  // void TerminalUpdateStatus(TTerminal *Terminal, bool Active);
  void TerminalChangeDirectory(TObject *Sender);
  void TerminalReadDirectory(TObject *Sender, bool ReloadOnly);
  void TerminalStartReadDirectory(TObject *Sender);
  void TerminalReadDirectoryProgress(TObject *Sender, intptr_t Progress,
    intptr_t ResolvedLinks, bool &Cancel);
  void TerminalInformation(TTerminal *Terminal,
    const UnicodeString AStr, bool Status, intptr_t Phase);
  void TerminalQueryUser(TObject *Sender,
    const UnicodeString AQuery, TStrings *MoreMessages, uintptr_t Answers,
    const TQueryParams *AParams, uintptr_t &Answer, TQueryType Type, void *Arg);
  void TerminalPromptUser(TTerminal *Terminal,
    TPromptKind Kind, const UnicodeString AName, const UnicodeString AInstructions,
    TStrings *Prompts, TStrings *Results, bool &AResult,
    void *Arg);
  void TerminalDisplayBanner(TTerminal *Terminal,
    const UnicodeString ASessionName, const UnicodeString ABanner, bool &NeverShowAgain,
    intptr_t Options, uintptr_t & AParams);
  void TerminalShowExtendedException(TTerminal *Terminal,
    Exception *E, void *Arg);
  void TerminalDeleteLocalFile(UnicodeString AFileName, bool Alternative, intptr_t& Deleted);
  HANDLE TerminalCreateLocalFile(const UnicodeString ALocalFileName,
    DWORD DesiredAccess, DWORD ShareMode, DWORD CreationDisposition, DWORD FlagsAndAttributes);
  DWORD TerminalGetLocalFileAttributes(const UnicodeString ALocalFileName) const;
  bool TerminalSetLocalFileAttributes(const UnicodeString ALocalFileName, DWORD FileAttributes);
  bool TerminalMoveLocalFile(const UnicodeString ALocalFileName, const UnicodeString ANewLocalFileName, DWORD Flags);
  bool TerminalRemoveLocalDirectory(const UnicodeString ALocalDirName);
  bool TerminalCreateLocalDirectory(const UnicodeString ALocalDirName, LPSECURITY_ATTRIBUTES SecurityAttributes);
  void OperationProgress(
    TFileOperationProgressType &ProgressData);
  void OperationFinished(TFileOperation Operation,
    TOperationSide Side, bool DragDrop, const UnicodeString AFileName, bool Success,
    TOnceDoneOperation &DisconnectWhenComplete);
  void CancelConfiguration(TFileOperationProgressType &ProgressData);
  TStrings *CreateFileList(TObjectList *PanelItems,
    TOperationSide Side, bool SelectedOnly = false, const UnicodeString Directory = L"",
    bool FileNameOnly = false, TStrings *AFileList = nullptr);
  TStrings *CreateSelectedFileList(TOperationSide Side,
    TFarPanelInfo **APanelInfo = nullptr);
  TStrings *CreateFocusedFileList(TOperationSide Side,
    TFarPanelInfo **APanelInfo = nullptr);
  void CustomCommandGetParamValue(
    const UnicodeString AName, UnicodeString &Value);
  void HandleErrorList(TStringList *& ErrorList);
  void TerminalSynchronizeDirectory(UnicodeString LocalDirectory,
    UnicodeString RemoteDirectory, bool & Continue, bool Collect);
  void DoSynchronize(TSynchronizeController * Sender,
    UnicodeString LocalDirectory, UnicodeString RemoteDirectory,
    const TCopyParamType & CopyParam, const TSynchronizeParamType & Params,
    TSynchronizeChecklist ** Checklist, TSynchronizeOptions * Options, bool Full);
  void DoSynchronizeInvalid(TSynchronizeController * Sender,
    UnicodeString Directory, UnicodeString ErrorStr);
  void DoSynchronizeTooManyDirectories(TSynchronizeController * Sender,
    intptr_t & MaxDirectories);
  void Synchronize(UnicodeString LocalDirectory,
    UnicodeString RemoteDirectory, TTerminal::TSynchronizeMode Mode,
    const TCopyParamType & CopyParam, intptr_t Params, TSynchronizeChecklist ** Checklist,
    TSynchronizeOptions * Options);
  void SynchronizeSessionLog(const UnicodeString & Message);
  void GetSynchronizeOptions(intptr_t Params, TSynchronizeOptions & Options);
  void QueueListUpdate(TTerminalQueue *Queue);
  void QueueItemUpdate(TTerminalQueue *Queue, TQueueItem *Item);
  void QueueEvent(TTerminalQueue *Queue, TQueueEvent Event);
  void GetSpaceAvailable(const UnicodeString APath,
    TSpaceAvailable &ASpaceAvailable, bool &Close);
  void QueueAddItem(TQueueItem *Item);
  UnicodeString GetFileNameHash(const UnicodeString AFileName) const;
  intptr_t GetFilesRemote(TObjectList *PanelItems, bool Move,
    UnicodeString &DestPath, int OpMode);

private:
  TTerminalQueue *GetQueue();
  TTerminalQueueStatus *GetQueueStatus();

private:
  TTerminal *FTerminal{nullptr};
  TTerminalQueue *FQueue{nullptr};
  TTerminalQueueStatus *FQueueStatus{nullptr};
  TCriticalSection FQueueStatusSection;
  TQueueEvent FQueueEvent{qeEmpty};
  HANDLE FProgressSaveScreenHandle{};
  HANDLE FSynchronizationSaveScreenHandle{};
  HANDLE FAuthenticationSaveScreenHandle{};
  TDateTime FSynchronizationStart;
  std::unique_ptr<TStrings> FFileList;
  TList *FPanelItems{nullptr};
  UnicodeString FSavedFindFolder;
  UnicodeString FOriginalEditFile;
  UnicodeString FLastEditFile;
  UnicodeString FLastMultipleEditFile;
  UnicodeString FLastMultipleEditFileTitle;
  UnicodeString FLastMultipleEditDirectory;
  intptr_t FLastEditorID{-1};
  TGUICopyParamType FLastEditCopyParam;
  TKeepAliveThread *FKeepaliveThread{nullptr};
  TSynchronizeController *FSynchronizeController{nullptr};
  std::unique_ptr<TStrings> FCapturedLog;
  std::unique_ptr<TStrings> FAuthenticationLog;
  typedef rde::map<intptr_t, TMultipleEdit> TMultipleEdits;
  TMultipleEdits FMultipleEdits;
  typedef rde::vector<TEditHistory> TEditHistories;
  TEditHistories FEditHistories;
  UnicodeString FLastPath;
  std::unique_ptr<TStrings> FPathHistory;
  UnicodeString FSessionsFolder;
  UnicodeString FNewSessionsFolder;
  UnicodeString FPrevSessionName;
  bool FQueueStatusInvalidated{false};
  bool FQueueItemInvalidated{false};
  bool FRefreshLocalDirectory{false};
  bool FRefreshRemoteDirectory{false};
  bool FQueueEventPending{false};
  bool FReloadDirectory{false};
  bool FLastMultipleEditReadOnly{false};
  bool FNoProgress{false};
  bool FSynchronizationCompare{false};
  bool FEditorPendingSave{false};
  bool FNoProgressFinish{false};
  bool FSynchronisingBrowse{false};
  bool FOutputLog{false};
  bool FLoadingSessionList{false};
  bool FCurrentDirectoryWasChanged{false};
};

NB_DEFINE_CLASS_ID(TSessionPanelItem);
class TSessionPanelItem : public TCustomFarPanelItem
{
  NB_DISABLE_COPY(TSessionPanelItem)
public:
  explicit TSessionPanelItem(const TSessionData *ASessionData);
  static void SetPanelModes(TFarPanelModes *PanelModes);
  static void SetKeyBarTitles(TFarKeyBarTitles *KeyBarTitles);

protected:
  UnicodeString FPath;
  const TSessionData *FSessionData;

  void GetData(
    DWORD &Flags, UnicodeString &AFileName, int64_t &Size,
    DWORD &FileAttributes,
    TDateTime &LastWriteTime, TDateTime &LastAccess,
    DWORD &NumberOfLinks, UnicodeString &Description,
    UnicodeString &Owner, void *&UserData, int &CustomColumnNumber) override;
};

NB_DEFINE_CLASS_ID(TSessionFolderPanelItem);
class TSessionFolderPanelItem : public TCustomFarPanelItem
{
public:
  explicit TSessionFolderPanelItem(const UnicodeString Folder);

protected:
  UnicodeString FFolder;

  void GetData(
    DWORD &Flags, UnicodeString &AFileName, int64_t &Size,
    DWORD &FileAttributes,
    TDateTime &LastWriteTime, TDateTime &LastAccess,
    DWORD &NumberOfLinks, UnicodeString &Description,
    UnicodeString &Owner, void *&UserData, int &CustomColumnNumber) override;
};

NB_DEFINE_CLASS_ID(TRemoteFilePanelItem);
class TRemoteFilePanelItem : public TCustomFarPanelItem
{
  NB_DISABLE_COPY(TRemoteFilePanelItem)
public:
  explicit TRemoteFilePanelItem(TRemoteFile *ARemoteFile);
  static void SetPanelModes(TFarPanelModes *PanelModes);
  static void SetKeyBarTitles(TFarKeyBarTitles *KeyBarTitles);

protected:
  TRemoteFile *FRemoteFile;

  void GetData(
    DWORD &Flags, UnicodeString &AFileName, int64_t &Size,
    DWORD &FileAttributes,
    TDateTime &LastWriteTime, TDateTime &LastAccess,
    DWORD &NumberOfLinks, UnicodeString &Description,
    UnicodeString &Owner, void *&UserData, int &CustomColumnNumber) override;
  UnicodeString GetCustomColumnData(size_t Column) override;
  static void TranslateColumnTypes(UnicodeString &AColumnTypes,
    TStrings *ColumnTitles);
};

