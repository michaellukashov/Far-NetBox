﻿#pragma once

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
constexpr const char * REMOTE_DIR_HISTORY = "WinscpRemoteDirectory";
constexpr const char * ASCII_MASK_HISTORY = "WinscpAsciiMask";
constexpr const char * LINK_FILENAME_HISTORY = "WinscpRemoteLink";
constexpr const char * LINK_POINT_TO_HISTORY = "WinscpRemoteLinkPointTo";
constexpr const char * APPLY_COMMAND_HISTORY = "WinscpApplyCmd";
constexpr const char * APPLY_COMMAND_PARAM_HISTORY = "WinscpApplyCmdParam";
constexpr const char * LOG_FILE_HISTORY = "WinscpLogFile";
constexpr const char * REMOTE_SYNC_HISTORY = "WinscpRemoteSync";
constexpr const char * LOCAL_SYNC_HISTORY = "WinscpLocalSync";
constexpr const char * MOVE_TO_HISTORY = "WinscpMoveTo";
constexpr const char * WINSCP_FILE_MASK_HISTORY = "WinscpFileMask";
constexpr const char * MAKE_SESSION_FOLDER_HISTORY = "WinscpSessionFolder";

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

struct TMultipleEdit final : public TObject
{
  UnicodeString FileName;
  UnicodeString FileTitle;
  UnicodeString Directory;
  UnicodeString LocalFileName;
  bool PendingSave{false};
};

struct TEditHistory final : public TObject
{
  UnicodeString FileName;
  UnicodeString Directory;
  bool operator ==(const TEditHistory & rh) const { return (FileName == rh.FileName) && (Directory == rh.Directory); }
};

using TProcessSessionEvent = nb::FastDelegate2<void,
  TSessionData * /*Data*/, void * /*Param*/>;

extern const TObjectClassId OBJECT_CLASS_TWinSCPFileSystem;
class TWinSCPFileSystem final : public TCustomFarFileSystem
{
  friend class TWinSCPPlugin;
  friend class TNetBoxPlugin;
  friend class TKeepAliveThread;
  friend class TQueueDialog;
  NB_DISABLE_COPY(TWinSCPFileSystem)
public:
  static bool classof(const TObject * Obj) { return Obj->is(OBJECT_CLASS_TWinSCPFileSystem); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TWinSCPFileSystem) || TCustomFarFileSystem::is(Kind); }
public:
  explicit TWinSCPFileSystem(gsl::not_null<TCustomFarPlugin *> APlugin) noexcept;
  virtual ~TWinSCPFileSystem() noexcept override;
  void Init(TSecureShell * SecureShell);

  virtual void Close() override;

protected:
  bool Connect(TSessionData * Data);
  void Disconnect();
  void SaveSession();

  virtual void GetOpenPanelInfoEx(OPENPANELINFO_FLAGS & Flags,
    UnicodeString & HostFile, UnicodeString & CurDir, UnicodeString & AFormat,
    UnicodeString & PanelTitle, TFarPanelModes * PanelModes, intptr_t & StartPanelMode,
    OPENPANELINFO_SORTMODES & StartSortMode, bool & StartSortOrder, TFarKeyBarTitles * KeyBarTitles,
    UnicodeString & ShortcutData) override;
  virtual bool GetFindDataEx(TObjectList * PanelItems, OPERATION_MODES OpMode) override;
  virtual bool ProcessKeyEx(int32_t Key, uint32_t ControlState) override;
  virtual bool SetDirectoryEx(const UnicodeString & ADir, OPERATION_MODES OpMode) override;
  virtual int32_t MakeDirectoryEx(const UnicodeString & AName, OPERATION_MODES OpMode) override;
  virtual bool DeleteFilesEx(TObjectList * PanelItems, OPERATION_MODES OpMode) override;
  virtual int32_t GetFilesEx(TObjectList * PanelItems, bool Move,
    UnicodeString & DestPath, OPERATION_MODES OpMode) override;
  virtual int32_t PutFilesEx(TObjectList * PanelItems, bool Move, OPERATION_MODES OpMode) override;
  virtual bool ProcessPanelEventEx(intptr_t Event, void * Param) override;

  void ProcessEditorEvent(intptr_t Event, void * Param);

  virtual void HandleException(Exception * E, OPERATION_MODES OpMode = 0) override;
  void KeepaliveThreadCallback();

  bool IsSessionList() const;
  bool Connected() const;
  const TWinSCPPlugin * GetWinSCPPlugin() const;
  TWinSCPPlugin * GetWinSCPPlugin();
  void ShowOperationProgress(TFileOperationProgressType & ProgressData,
    bool Force);
  bool SessionDialog(TSessionData * SessionData, TSessionActionEnum Action);
  void EditConnectSession(TSessionData * Data, bool Edit);
  void EditConnectSession(TSessionData * Data, bool Edit, bool NewData, bool FillInConnect);
  void DuplicateOrRenameSession(TSessionData * Data,
    bool Duplicate);
  void FocusSession(const TSessionData * Data);
  void DeleteSession(TSessionData * Data, void * AParam);
  void ProcessSessions(TObjectList * PanelItems,
    TProcessSessionEvent && ProcessSession, void * AParam);
  void ExportSession(TSessionData * Data, void * AParam);
  bool ImportSessions(TObjectList * PanelItems, bool Move, OPERATION_MODES OpMode);
  void FileProperties();
  void RemoteCreateLink();
  void TransferFiles(bool Move);
  void RenameFile();
  void ApplyCommand();
  void ShowInformation();
  void InsertTokenOnCommandLine(const UnicodeString & Token, bool Separate);
  void InsertSessionNameOnCommandLine();
  void InsertFileNameOnCommandLine(bool Full);
  UnicodeString GetFullFilePath(const TRemoteFile * AFile) const;
  void InsertPathOnCommandLine();
  void CopyFullFileNamesToClipboard();
  void FullSynchronize(bool Source);
  void Synchronize();
  void OpenDirectory(bool Add);
  void HomeDirectory();
  void ToggleSynchronizeBrowsing();
  bool IsSynchronizedBrowsing() const;
  bool PropertiesDialog(TStrings * AFileList,
    const UnicodeString & Directory,
    const TRemoteTokenList * GroupList, const TRemoteTokenList * UserList,
    TRemoteProperties * Properties, int32_t AllowedChanges);
  bool ExecuteCommand(const UnicodeString & Command);
  void TerminalCaptureLog(const UnicodeString & AddedLine, TCaptureOutputType OutputEvent);
  bool CopyDialog(bool ToRemote, bool Move, const TStrings * AFileList,
    uint32_t Options, uint32_t CopyParamAttrs, UnicodeString & TargetDirectory, TGUICopyParamType * Params);
  bool LinkDialog(UnicodeString & AFileName, UnicodeString & PointTo, bool & Symbolic,
    bool Edit, bool AllowSymbolic);
  void FileSystemInfoDialog(const TSessionInfo & SessionInfo,
    const TFileSystemInfo & FileSystemInfo, const UnicodeString & SpaceAvailablePath,
    TGetSpaceAvailableEvent && OnGetSpaceAvailable);
  bool OpenDirectoryDialog(bool Add, UnicodeString & Directory,
    TBookmarkList * BookmarkList);
  bool ApplyCommandDialog(UnicodeString & Command, int32_t & Params) const;
  bool FullSynchronizeDialog(TTerminal::TSynchronizeMode & Mode,
    int32_t & Params, UnicodeString & LocalDirectory, UnicodeString & RemoteDirectory,
    TCopyParamType * CopyParams, bool & SaveSettings, bool & SaveMode, int32_t Options,
    const TUsableCopyParamAttrs & CopyParamAttrs) const;
  bool SynchronizeChecklistDialog(TSynchronizeChecklist * Checklist,
    TTerminal::TSynchronizeMode Mode, int32_t Params,
    const UnicodeString & LocalDirectory, const UnicodeString & RemoteDirectory);
  bool RemoteTransferDialog(TStrings * AFileList, UnicodeString & Target,
    UnicodeString & FileMask, bool Move);
  bool RenameFileDialog(TRemoteFile * AFile, UnicodeString & NewName);
  uint32_t MoreMessageDialog(const UnicodeString & Str, TStrings * MoreMessages,
    TQueryType Type, uint32_t Answers, const TMessageParams * AParams = nullptr);
  bool PasswordDialog(TSessionData * SessionData,
    TPromptKind Kind, const UnicodeString & Name, const UnicodeString & Instructions, TStrings * Prompts,
    TStrings * Results, bool StoredCredentialsTried);
  bool BannerDialog(const UnicodeString & SessionName, const UnicodeString & Banner,
    bool & NeverShowAgain, int32_t Options);
  bool CreateDirectoryDialog(UnicodeString & Directory,
    TRemoteProperties * Properties, bool & SaveSettings);
  bool QueueDialog(TTerminalQueueStatus * Status, bool ClosingPlugin);
  bool SynchronizeDialog(TSynchronizeParamType & Params,
    const TCopyParamType * CopyParams, TSynchronizeStartStopEvent && OnStartStop,
    bool & SaveSettings, uint32_t Options, uint32_t CopyParamAttrs,
    TGetSynchronizeOptionsEvent && OnGetOptions);
  bool SynchronizeAllowSelectedOnly();
  void RequireCapability(int32_t Capability);
  void RequireLocalPanel(TFarPanelInfo * Panel, const UnicodeString & Message);
  bool AreCachesEmpty() const;
  void ClearCaches();
  void OpenSessionInPutty();
  void QueueShow(bool ClosingPlugin);
  TTerminalQueueStatus * ProcessQueue(bool Hidden);
  bool EnsureCommandSessionFallback(TFSCapability Capability);
  void ConnectTerminal(TTerminal * Terminal);
  void TemporarilyDownloadFiles(TStrings * AFileList,
    TCopyParamType & CopyParam, UnicodeString & TempDir);
  int32_t UploadFiles(bool Move, OPERATION_MODES OpMode, bool Edit, UnicodeString & DestPath);
  void UploadOnSave(bool NoReload);
  void UploadFromEditor(bool NoReload, const UnicodeString & AFileName,
      const UnicodeString & RealFileName, UnicodeString & DestPath);
  void LogAuthentication(TTerminal * Terminal, const UnicodeString & Msg);
  void MultipleEdit();
  void MultipleEdit(const UnicodeString & Directory, const UnicodeString & AFileName, TRemoteFile * AFile);
  void EditViewCopyParam(TCopyParamType & CopyParam);
  bool SynchronizeBrowsing(const UnicodeString & NewPath);
  bool IsEditHistoryEmpty() const;
  void EditHistory();
  UnicodeString ProgressBar(int32_t Percentage, int32_t Width);
  bool IsLogging() const;
  void ShowLog();

  TTerminal * GetTerminal() const { return FTerminal; }
  TSessionData * GetSessionData() const { return FTerminal ? FTerminal->GetSessionData() : nullptr; }
  TSessionData * GetSessionData() { return FTerminal ? FTerminal->GetSessionData() : nullptr; }

protected:
  virtual UnicodeString GetCurrentDirectory() const override { return FTerminal ? FTerminal->RemoteGetCurrentDirectory() : UnicodeString(); }

private:
  bool TerminalCheckForEsc();
  void TerminalClose(TObject * Sender);
  // void TerminalUpdateStatus(TTerminal * Terminal, bool Active);
  void TerminalChangeDirectory(TObject * Sender);
  void TerminalReadDirectory(TObject * Sender, bool ReloadOnly);
  void TerminalStartReadDirectory(TObject * Sender);
  void TerminalReadDirectoryProgress(TObject * Sender, int32_t Progress,
    int32_t ResolvedLinks, bool & Cancel);
  void TerminalInformation(TTerminal * Terminal,
    const UnicodeString & AStr, bool Status, int32_t Phase, const UnicodeString & Additional);
  void TerminalQueryUser(TObject * Sender,
    const UnicodeString & AQuery, TStrings * MoreMessages, uint32_t Answers,
    const TQueryParams * AParams, uint32_t & Answer, TQueryType Type, void * Arg);
  void TerminalPromptUser(TTerminal * Terminal,
    TPromptKind Kind, const UnicodeString & AName, const UnicodeString & AInstructions,
    TStrings * Prompts, TStrings * Results, bool & AResult,
    void * Arg);
  void TerminalDisplayBanner(TTerminal * Terminal,
    const UnicodeString & ASessionName, const UnicodeString & ABanner, bool & NeverShowAgain,
    int32_t Options, uint32_t & AParams);
  void TerminalShowExtendedException(TTerminal * Terminal,
    Exception * E, void * Arg);
  void TerminalDeleteLocalFile(const UnicodeString & AFileName, bool Alternative, int32_t & Deleted);
  HANDLE TerminalCreateLocalFile(const UnicodeString & ALocalFileName,
    DWORD DesiredAccess, DWORD ShareMode, DWORD CreationDisposition, DWORD FlagsAndAttributes);
  DWORD TerminalGetLocalFileAttributes(const UnicodeString & ALocalFileName) const;
  bool TerminalSetLocalFileAttributes(const UnicodeString & ALocalFileName, DWORD FileAttributes);
  bool TerminalMoveLocalFile(const UnicodeString & ALocalFileName, const UnicodeString & ANewLocalFileName, DWORD Flags);
  bool TerminalRemoveLocalDirectory(const UnicodeString & ALocalDirName);
  bool TerminalCreateLocalDirectory(const UnicodeString & ALocalDirName, LPSECURITY_ATTRIBUTES SecurityAttributes);
  void OperationProgress(
    TFileOperationProgressType & ProgressData);
  void OperationFinished(TFileOperation Operation,
    TOperationSide Side, bool DragDrop, const UnicodeString & AFileName, bool Success,
    TOnceDoneOperation & DisconnectWhenComplete);
  void CancelConfiguration(TFileOperationProgressType & ProgressData);
  TStrings * CreateFileList(TObjectList * PanelItems,
    TOperationSide Side, bool SelectedOnly = false, const UnicodeString & Directory = L"",
    bool FileNameOnly = false, TStrings * AFileList = nullptr);
  TStrings * CreateSelectedFileList(TOperationSide Side,
    TFarPanelInfo ** APanelInfo = nullptr);
  TStrings * CreateFocusedFileList(TOperationSide Side,
    TFarPanelInfo ** APanelInfo = nullptr);
  void CustomCommandGetParamValue(
    const UnicodeString & AName, UnicodeString & Value);
  // void HandleErrorList(TStringList *& ErrorList);
  void TerminalSynchronizeDirectory(const UnicodeString & LocalDirectory,
    const UnicodeString & RemoteDirectory, bool & Continue, bool Collect, const TSynchronizeOptions * SynchronizeOptions);
  void DoSynchronize(TSynchronizeController * Sender,
    const UnicodeString & LocalDirectory, const UnicodeString & RemoteDirectory,
    const TCopyParamType & CopyParam, const TSynchronizeParamType & Params,
    TSynchronizeChecklist ** Checklist, TSynchronizeOptions * Options, bool Full);
  void DoSynchronizeInvalid(TSynchronizeController * Sender,
    const UnicodeString & Directory, const UnicodeString & ErrorStr);
  void DoSynchronizeTooManyDirectories(TSynchronizeController * Sender,
    int32_t & MaxDirectories);
  void Synchronize(const UnicodeString & LocalDirectory,
    const UnicodeString & RemoteDirectory, TTerminal::TSynchronizeMode Mode,
    const TCopyParamType & CopyParam, int32_t Params, TSynchronizeChecklist ** Checklist,
    TSynchronizeOptions * Options);
  //void SynchronizeSessionLog(const UnicodeString & Message);
  void GetSynchronizeOptions(int32_t Params, TSynchronizeOptions & Options);
  void QueueListUpdate(TTerminalQueue * Queue);
  void QueueItemUpdate(TTerminalQueue * Queue, TQueueItem * Item);
  void QueueEvent(TTerminalQueue * Queue, TQueueEventType Event);
  void GetSpaceAvailable(const UnicodeString & APath,
    TSpaceAvailable & ASpaceAvailable, bool & Close);
  void QueueAddItem(TQueueItem * Item);
  UnicodeString GetFileNameHash(const UnicodeString & AFileName) const;
  int32_t GetFilesRemote(TObjectList * PanelItems, bool Move,
    UnicodeString & DestPath, OPERATION_MODES OpMode);

private:
  TTerminalQueue * GetQueue();
  TTerminalQueueStatus * GetQueueStatus();

private:
  gsl::owner<TTerminal *> FTerminal{nullptr};
  gsl::owner<TTerminalQueue *> FQueue{nullptr};
  gsl::owner<TTerminalQueueStatus *> FQueueStatus{nullptr};
  TCriticalSection FQueueStatusSection;
  TQueueEventType FQueueEvent{qeEmpty};
  HANDLE FProgressSaveScreenHandle{nullptr};
  HANDLE FSynchronizationSaveScreenHandle{nullptr};
  HANDLE FAuthenticationSaveScreenHandle{nullptr};
  TDateTime FSynchronizationStart;
  std::unique_ptr<TStrings> FFileList;
  gsl::owner<TList *> FPanelItems{nullptr};
  UnicodeString FSavedFindFolder;
  UnicodeString FOriginalEditFile;
  UnicodeString FLastEditFile;
  UnicodeString FLastMultipleEditFile;
  UnicodeString FLastMultipleEditFileTitle;
  UnicodeString FLastMultipleEditDirectory;
  int32_t FLastEditorID{-1};
  TGUICopyParamType FLastEditCopyParam{};
  gsl::owner<TKeepAliveThread *> FKeepaliveThread{nullptr};
  gsl::owner<TSynchronizeController *> FSynchronizeController{nullptr};
  std::unique_ptr<TStrings> FCapturedLog;
  std::unique_ptr<TStrings> FAuthenticationLog;
  using TMultipleEdits = nb::map_t<int32_t, TMultipleEdit>;
  TMultipleEdits FMultipleEdits;
  using TEditHistories = nb::vector_t<TEditHistory>;
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

extern const TObjectClassId OBJECT_CLASS_TSessionPanelItem;
class TSessionPanelItem final : public TCustomFarPanelItem
{
  NB_DISABLE_COPY(TSessionPanelItem)
public:
  explicit TSessionPanelItem(const TSessionData * ASessionData);
  static void SetPanelModes(TFarPanelModes * PanelModes);
  static void SetKeyBarTitles(TFarKeyBarTitles * KeyBarTitles);

protected:
  UnicodeString FPath;
  const TSessionData * FSessionData{nullptr};

  virtual void GetData(
    PLUGINPANELITEMFLAGS & Flags, UnicodeString & AFileName, int64_t & Size,
    uintptr_t & FileAttributes,
    TDateTime & LastWriteTime, TDateTime & LastAccess,
    uintptr_t & NumberOfLinks, UnicodeString & Description,
    UnicodeString & Owner, void *& UserData, size_t & CustomColumnNumber) override;
};

extern const TObjectClassId OBJECT_CLASS_TSessionFolderPanelItem;
class TSessionFolderPanelItem : public TCustomFarPanelItem
{
public:
  explicit TSessionFolderPanelItem(const UnicodeString & Folder);

protected:
  UnicodeString FFolder;

  virtual void GetData(
    PLUGINPANELITEMFLAGS & Flags, UnicodeString & AFileName, int64_t & Size,
    uintptr_t & FileAttributes,
    TDateTime & LastWriteTime, TDateTime & LastAccess,
    uintptr_t & NumberOfLinks, UnicodeString & Description,
    UnicodeString & Owner, void *& UserData, size_t & CustomColumnNumber) override;
};

extern const TObjectClassId OBJECT_CLASS_TRemoteFilePanelItem;
class TRemoteFilePanelItem final : public TCustomFarPanelItem
{
  NB_DISABLE_COPY(TRemoteFilePanelItem)
public:
  explicit TRemoteFilePanelItem(TRemoteFile * ARemoteFile);
  static void SetPanelModes(TFarPanelModes * PanelModes);
  static void SetKeyBarTitles(TFarKeyBarTitles * KeyBarTitles);

protected:
  TRemoteFile * FRemoteFile{nullptr};

  virtual void GetData(
    PLUGINPANELITEMFLAGS & Flags, UnicodeString & AFileName, int64_t & Size,
    uintptr_t & FileAttributes,
    TDateTime & LastWriteTime, TDateTime & LastAccess,
    uintptr_t & NumberOfLinks, UnicodeString & Description,
    UnicodeString & Owner, void *& UserData, size_t & CustomColumnNumber) override;
  virtual UnicodeString GetCustomColumnData(size_t Column) override;
  static void TranslateColumnTypes(UnicodeString & AColumnTypes,
    TStrings * ColumnTitles);
};

