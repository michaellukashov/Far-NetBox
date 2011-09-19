//---------------------------------------------------------------------------
#ifndef TerminalH
#define TerminalH

#include "Classes.h"

#include "SessionInfo.h"
#include "Interface.h"
#include "FileOperationProgress.h"
#include "FileMasks.h"
#include "Exceptions.h"
//---------------------------------------------------------------------------
class TCopyParamType;
class TFileOperationProgressType;
class TRemoteDirectory;
class TRemoteFile;
class TCustomFileSystem;
class TTunnelThread;
class TSecureShell;
struct TCalculateSizeParams;
struct TOverwriteFileParams;
struct TSynchronizeData;
struct TSynchronizeOptions;
class TSynchronizeChecklist;
struct TCalculateSizeStats;
struct TFileSystemInfo;
struct TSpaceAvailable;
struct TFilesFindParams;
class TTunnelUI;
class TCallbackGuard;
//---------------------------------------------------------------------------
typedef void (TObject::*TQueryUserEvent)
  (TObject * Sender, const std::wstring Query, TStrings * MoreMessages, int Answers,
   const TQueryParams * Params, int & Answer, TQueryType QueryType, void * Arg);
typedef void (TObject::*TPromptUserEvent)
  (TTerminal * Terminal, TPromptKind Kind, std::wstring Name, std::wstring Instructions,
   TStrings * Prompts, TStrings * Results, bool & Result, void * Arg);
typedef void (TObject::*TDisplayBannerEvent)
  (TTerminal * Terminal, std::wstring SessionName, const std::wstring & Banner,
   bool & NeverShowAgain, int Options);
typedef void (TObject::*TExtendedExceptionEvent)
  (TTerminal * Terminal, const std::exception * E, void * Arg);
typedef void (TObject::*TReadDirectoryEvent)(TObject * Sender, bool ReloadOnly);
typedef void (TObject::*TReadDirectoryProgressEvent)(
  TObject* Sender, int Progress, bool & Cancel);
typedef void (TObject::*TProcessFileEvent)
  (const std::wstring FileName, const TRemoteFile * File, void * Param);
typedef void (TObject::*TProcessFileEventEx)
  (const std::wstring FileName, const TRemoteFile * File, void * Param, int Index);
typedef int (TObject::*TFileOperationEvent)
  (void * Param1, void * Param2);
typedef void (TObject::*TSynchronizeDirectory)
  (const std::wstring LocalDirectory, const std::wstring RemoteDirectory,
   bool & Continue, bool Collect);
typedef void (TObject::*TDeleteLocalFileEvent)(
  const std::wstring FileName, bool Alternative);
typedef int (TObject::*TDirectoryModifiedEvent)
  (TTerminal * Terminal, const std::wstring Directory, bool SubDirs);
typedef void (TObject::*TInformationEvent)
  (TTerminal * Terminal, const std::wstring & Str, bool Status, bool Active);
//---------------------------------------------------------------------------
#define SUSPEND_OPERATION(Command)                            \
  {                                                           \
    TSuspendFileOperationProgress Suspend(OperationProgress); \
    Command                                                   \
  }

#define THROW_SKIP_FILE(EXCEPTION, MESSAGE) \
  throw EScpSkipFile(EXCEPTION, MESSAGE)
#define THROW_SKIP_FILE_NULL THROW_SKIP_FILE(NULL, L"")

/* TODO : Better user interface (query to user) */
#define FILE_OPERATION_LOOP_CUSTOM(TERMINAL, ALLOW_SKIP, MESSAGE, OPERATION) { \
  bool DoRepeat; \
  do { \
    DoRepeat = false; \
    try { \
      OPERATION;                                                            \
    }                                                                       \
    catch (const EAbort & E)                                                      \
    {                                                                       \
      throw;                                                                \
    }                                                                       \
    catch (const EScpSkipFile & E)                                                \
    {                                                                       \
      throw;                                                                \
    }                                                                       \
    catch (const EFatal & E)                                                      \
    {                                                                       \
      throw;                                                                \
    }                                                                       \
    catch (const std::exception & E)                                                   \
    {                                                                       \
      TERMINAL->FileOperationLoopQuery(E, OperationProgress, MESSAGE, ALLOW_SKIP); \
      DoRepeat = true;                                                      \
    } \
  } while (DoRepeat); }

#define FILE_OPERATION_LOOP(MESSAGE, OPERATION) \
  FILE_OPERATION_LOOP_EX(true, MESSAGE, OPERATION)
//---------------------------------------------------------------------------
enum TCurrentFSProtocol { cfsUnknown, cfsSCP, cfsSFTP, cfsFTP };
//---------------------------------------------------------------------------
const int cpDelete = 0x01;
const int cpTemporary = 0x04;
const int cpNoConfirmation = 0x08;
const int cpNewerOnly = 0x10;
const int cpAppend = 0x20;
const int cpResume = 0x40;
//---------------------------------------------------------------------------
const int ccApplyToDirectories = 0x01;
const int ccRecursive = 0x02;
const int ccUser = 0x100;
//---------------------------------------------------------------------------
const int csIgnoreErrors = 0x01;
//---------------------------------------------------------------------------
const int ropNoReadDirectory = 0x02;
//---------------------------------------------------------------------------
const int boDisableNeverShowAgain = 0x01;
//---------------------------------------------------------------------------
class TTerminal : public TObject, public TSessionUI
{
public:
  // TScript::SynchronizeProc relies on the order
  enum TSynchronizeMode { smRemote, smLocal, smBoth };
  static const int spDelete = 0x01; // cannot be combined with spTimestamp
  static const int spNoConfirmation = 0x02; // has no effect for spTimestamp
  static const int spExistingOnly = 0x04; // is implicit for spTimestamp
  static const int spNoRecurse = 0x08;
  static const int spUseCache = 0x10; // cannot be combined with spTimestamp
  static const int spDelayProgress = 0x20; // cannot be combined with spTimestamp
  static const int spPreviewChanges = 0x40; // not used by core
  static const int spSubDirs = 0x80; // cannot be combined with spTimestamp
  static const int spTimestamp = 0x100;
  static const int spNotByTime = 0x200; // cannot be combined with spTimestamp and smBoth
  static const int spBySize = 0x400; // cannot be combined with smBoth, has opposite meaning for spTimestamp
  // 0x800 is reserved for GUI (spSelectedOnly)
  static const int spMirror = 0x1000;

// for TranslateLockedPath()
friend class TRemoteFile;
// for ReactOnCommand()
friend class TSCPFileSystem;
friend class TSFTPFileSystem;
friend class TFTPFileSystem;
friend class TTunnelUI;
friend class TCallbackGuard;

private:
  TSessionData * FSessionData;
  TSessionLog * FLog;
  TConfiguration * FConfiguration;
  std::wstring FCurrentDirectory;
  std::wstring FLockDirectory;
  int FExceptionOnFail;
  TRemoteDirectory * FFiles;
  int FInTransaction;
  bool FSuspendTransaction;
  notify_signal_type FOnChangeDirectory;
  TReadDirectoryEvent FOnReadDirectory;
  notify_signal_type FOnStartReadDirectory;
  TReadDirectoryProgressEvent FOnReadDirectoryProgress;
  TDeleteLocalFileEvent FOnDeleteLocalFile;
  TRemoteTokenList FMembership;
  TRemoteTokenList FGroups;
  TRemoteTokenList FUsers;
  bool FUsersGroupsLookedup;
  fileoperationprogress_signal_type FOnProgress;
  fileoperationfinished_signal_type FOnFinished;
  TFileOperationProgressType * FOperationProgress;
  bool FUseBusyCursor;
  TRemoteDirectoryCache * FDirectoryCache;
  TRemoteDirectoryChangesCache * FDirectoryChangesCache;
  TCustomFileSystem * FFileSystem;
  TSecureShell * FSecureShell;
  std::wstring FLastDirectoryChange;
  TCurrentFSProtocol FFSProtocol;
  TTerminal * FCommandSession;
  bool FAutoReadDirectory;
  bool FReadingCurrentDirectory;
  bool * FClosedOnCompletion;
  TSessionStatus FStatus;
  std::wstring FPassword;
  std::wstring FTunnelPassword;
  TTunnelThread * FTunnelThread;
  TSecureShell * FTunnel;
  TSessionData * FTunnelData;
  TSessionLog * FTunnelLog;
  TTunnelUI * FTunnelUI;
  int FTunnelLocalPortNumber;
  std::wstring FTunnelError;
  TQueryUserEvent FOnQueryUser;
  TPromptUserEvent FOnPromptUser;
  TDisplayBannerEvent FOnDisplayBanner;
  TExtendedExceptionEvent FOnShowExtendedException;
  TInformationEvent FOnInformation;
  notify_signal_type FOnClose;
  bool FAnyInformation;
  TCallbackGuard * FCallbackGuard;
  TFindingFileEvent FOnFindingFile;
  TTerminal *Self;

  void CommandError(const std::exception * E, const std::wstring Msg);
  int CommandError(const std::exception * E, const std::wstring Msg, int Answers);
  void ReactOnCommand(int /*TFSCommand*/ Cmd);
  void ClearCachedFileList(const std::wstring Path, bool SubDirs);
  void AddCachedFileList(TRemoteFileList * FileList);
  inline bool InTransaction();

  void DoProgress(TFileOperationProgressType &ProgressData, TCancelStatus &Cancel);
protected:
  bool FReadCurrentDirectoryPending;
  bool FReadDirectoryPending;
  bool FTunnelOpening;

  void DoStartReadDirectory();
  void DoReadDirectoryProgress(int Progress, bool & Cancel);
  void DoReadDirectory(bool ReloadOnly);
  void DoCreateDirectory(const std::wstring DirName);
  void DoDeleteFile(const std::wstring FileName, const TRemoteFile * File,
    int Params);
  void DoCustomCommandOnFile(std::wstring FileName,
    const TRemoteFile * File, std::wstring Command, int Params, TCaptureOutputEvent OutputEvent);
  void DoRenameFile(const std::wstring FileName,
    const std::wstring NewName, bool Move);
  void DoCopyFile(const std::wstring FileName, const std::wstring NewName);
  void DoChangeFileProperties(const std::wstring FileName,
    const TRemoteFile * File, const TRemoteProperties * Properties);
  void DoChangeDirectory();
  void EnsureNonExistence(const std::wstring FileName);
  void LookupUsersGroups();
  void FileModified(const TRemoteFile * File,
    const std::wstring FileName, bool ClearDirectoryChange = false);
  int FileOperationLoop(TFileOperationEvent CallBackFunc,
    TFileOperationProgressType * OperationProgress, bool AllowSkip,
    const std::wstring Message, void * Param1 = NULL, void * Param2 = NULL);
  bool ProcessFiles(TStrings * FileList, TFileOperation Operation,
    TProcessFileEvent ProcessFile, void * Param = NULL, TOperationSide Side = osRemote,
    bool Ex = false);
  bool ProcessFilesEx(TStrings * FileList, TFileOperation Operation,
    TProcessFileEventEx ProcessFile, void * Param = NULL, TOperationSide Side = osRemote);
  void ProcessDirectory(const std::wstring DirName,
    TProcessFileEvent CallBackFunc, void * Param = NULL, bool UseCache = false,
    bool IgnoreErrors = false);
  void AnnounceFileListOperation();
  std::wstring TranslateLockedPath(std::wstring Path, bool Lock);
  void ReadDirectory(TRemoteFileList * FileList);
  void CustomReadDirectory(TRemoteFileList * FileList);
  void DoCreateLink(const std::wstring FileName, const std::wstring PointTo, bool Symbolic);
  bool CreateLocalFile(const std::wstring FileName,
    TFileOperationProgressType * OperationProgress, HANDLE * AHandle,
    bool NoConfirmation);
  void OpenLocalFile(const std::wstring FileName, int Access,
    int * Attrs, HANDLE * Handle, __int64 * ACTime, __int64 * MTime,
    __int64 * ATime, __int64 * Size, bool TryWriteReadOnly = true);
  bool AllowLocalFileTransfer(std::wstring FileName, const TCopyParamType * CopyParam);
  bool HandleException(const std::exception * E);
  void CalculateFileSize(std::wstring FileName,
    const TRemoteFile * File, /*TCalculateSizeParams*/ void * Size);
  void DoCalculateDirectorySize(const std::wstring FileName,
    const TRemoteFile * File, TCalculateSizeParams * Params);
  void CalculateLocalFileSize(const std::wstring FileName,
    const WIN32_FIND_DATA Rec, /*__int64*/ void * Size);
  void CalculateLocalFilesSize(TStrings * FileList, __int64 & Size,
    const TCopyParamType * CopyParam = NULL);
  TBatchOverwrite EffectiveBatchOverwrite(
    int Params, TFileOperationProgressType * OperationProgress, bool Special);
  bool CheckRemoteFile(int Params, TFileOperationProgressType * OperationProgress);
  int ConfirmFileOverwrite(const std::wstring FileName,
    const TOverwriteFileParams * FileParams, int Answers, const TQueryParams * QueryParams,
    TOperationSide Side, int Params, TFileOperationProgressType * OperationProgress,
    std::wstring Message = L"");
  void DoSynchronizeCollectDirectory(const std::wstring LocalDirectory,
    const std::wstring RemoteDirectory, TSynchronizeMode Mode,
    const TCopyParamType * CopyParam, int Params,
    TSynchronizeDirectory OnSynchronizeDirectory,
    TSynchronizeOptions * Options, int Level, TSynchronizeChecklist * Checklist);
  void SynchronizeCollectFile(const std::wstring FileName,
    const TRemoteFile * File, /*TSynchronizeData*/ void * Param);
  void SynchronizeRemoteTimestamp(const std::wstring FileName,
    const TRemoteFile * File, void * Param);
  void SynchronizeLocalTimestamp(const std::wstring FileName,
    const TRemoteFile * File, void * Param);
  void DoSynchronizeProgress(const TSynchronizeData & Data, bool Collect);
  void DeleteLocalFile(std::wstring FileName,
    const TRemoteFile * File, void * Param);
  void RecycleFile(std::wstring FileName, const TRemoteFile * File);
  void DoStartup();
  virtual bool DoQueryReopen(const std::exception * E);
  virtual void FatalError(const std::exception * E, std::wstring Msg);
  void ResetConnection();
  virtual bool DoPromptUser(TSessionData * Data, TPromptKind Kind,
    std::wstring Name, std::wstring Instructions, TStrings * Prompts,
    TStrings * Response);
  void OpenTunnel();
  void CloseTunnel();
  void DoInformation(const std::wstring & Str, bool Status, bool Active = true);
  std::wstring FileUrl(const std::wstring Protocol, const std::wstring FileName);
  bool PromptUser(TSessionData * Data, TPromptKind Kind,
    std::wstring Name, std::wstring Instructions, std::wstring Prompt, bool Echo,
    int MaxLen, std::wstring & Result);
  void FileFind(std::wstring FileName, const TRemoteFile * File, void * Param);
  void DoFilesFind(std::wstring Directory, TFilesFindParams & Params);
  bool DoCreateLocalFile(const std::wstring FileName,
    TFileOperationProgressType * OperationProgress, HANDLE * AHandle,
    bool NoConfirmation);

  virtual void Information(const std::wstring & Str, bool Status);
  virtual int QueryUser(const std::wstring Query,
    TStrings * MoreMessages, int Answers, const TQueryParams * Params,
    TQueryType QueryType = qtConfirmation);
  virtual int QueryUserException(const std::wstring Query,
    const std::exception * E, int Answers, const TQueryParams * Params,
    TQueryType QueryType = qtConfirmation);
  virtual bool PromptUser(TSessionData * Data, TPromptKind Kind,
    std::wstring Name, std::wstring Instructions, TStrings * Prompts, TStrings * Results);
  virtual void DisplayBanner(const std::wstring & Banner);
  virtual void Closed();
  virtual void HandleExtendedException(const std::exception * E);
  bool IsListenerFree(unsigned int PortNumber);
  void DoFinished(TFileOperation Operation, TOperationSide Side, bool Temp,
    const std::wstring & FileName, bool Success, TOnceDoneOperation & OnceDoneOperation);
  void RollbackAction(TSessionAction & Action,
    TFileOperationProgressType * OperationProgress, const std::exception * E = NULL);
  void DoAnyCommand(const std::wstring Command, TCaptureOutputEvent OutputEvent,
    TCallSessionAction * Action);
  TRemoteFileList * DoReadDirectoryListing(std::wstring Directory, bool UseCache);
  std::wstring EncryptPassword(const std::wstring & Password);
  std::wstring DecryptPassword(const std::wstring & Password);

  // __property TFileOperationProgressType * OperationProgress = { read=FOperationProgress };
  TFileOperationProgressType * GetOperationProgress() { return FOperationProgress; }

public:
  TTerminal(TSessionData * SessionData, TConfiguration * Configuration);
  ~TTerminal();
  void Open();
  void Close();
  void Reopen(int Params);
  virtual void DirectoryModified(const std::wstring Path, bool SubDirs);
  virtual void DirectoryLoaded(TRemoteFileList * FileList);
  void ShowExtendedException(const std::exception * E);
  void Idle();
  void RecryptPasswords();
  bool AllowedAnyCommand(const std::wstring Command);
  void AnyCommand(const std::wstring Command, TCaptureOutputEvent OutputEvent);
  void CloseOnCompletion(TOnceDoneOperation Operation = odoDisconnect, const std::wstring Message = L"");
  std::wstring AbsolutePath(std::wstring Path, bool Local);
  void BeginTransaction();
  void ReadCurrentDirectory();
  void ReadDirectory(bool ReloadOnly, bool ForceCache = false);
  TRemoteFileList * ReadDirectoryListing(std::wstring Directory, const TFileMasks & Mask);
  TRemoteFileList * CustomReadDirectoryListing(std::wstring Directory, bool UseCache);
  void ReadFile(const std::wstring FileName, TRemoteFile *& File);
  bool FileExists(const std::wstring FileName, TRemoteFile ** File = NULL);
  void ReadSymlink(TRemoteFile * SymlinkFile, TRemoteFile *& File);
  bool CopyToLocal(TStrings * FilesToCopy,
    const std::wstring TargetDir, const TCopyParamType * CopyParam, int Params);
  bool CopyToRemote(TStrings * FilesToCopy,
    const std::wstring TargetDir, const TCopyParamType * CopyParam, int Params);
  void CreateDirectory(const std::wstring DirName,
    const TRemoteProperties * Properties = NULL);
  void CreateLink(const std::wstring FileName, const std::wstring PointTo, bool Symbolic);
  void DeleteFile(std::wstring FileName,
    const TRemoteFile * File = NULL, void * Params = NULL);
  bool DeleteFiles(TStrings * FilesToDelete, int Params = 0);
  bool DeleteLocalFiles(TStrings * FileList, int Params = 0);
  bool IsRecycledFile(std::wstring FileName);
  void CustomCommandOnFile(std::wstring FileName,
    const TRemoteFile * File, void * AParams);
  void CustomCommandOnFiles(std::wstring Command, int Params,
    TStrings * Files, TCaptureOutputEvent OutputEvent);
  void ChangeDirectory(const std::wstring Directory);
  void EndTransaction();
  void HomeDirectory();
  void ChangeFileProperties(std::wstring FileName,
    const TRemoteFile * File, /*const TRemoteProperties */ void * Properties);
  void ChangeFilesProperties(TStrings * FileList,
    const TRemoteProperties * Properties);
  bool LoadFilesProperties(TStrings * FileList);
  void TerminalError(std::wstring Msg);
  void TerminalError(const std::exception * E, std::wstring Msg);
  void ReloadDirectory();
  void RefreshDirectory();
  void RenameFile(const std::wstring FileName, const std::wstring NewName);
  void RenameFile(const TRemoteFile * File, const std::wstring NewName, bool CheckExistence);
  void MoveFile(const std::wstring FileName, const TRemoteFile * File,
    /*const TMoveFileParams*/ void * Param);
  bool MoveFiles(TStrings * FileList, const std::wstring Target,
    const std::wstring FileMask);
  void CopyFile(const std::wstring FileName, const TRemoteFile * File,
    /*const TMoveFileParams*/ void * Param);
  bool CopyFiles(TStrings * FileList, const std::wstring Target,
    const std::wstring FileMask);
  void CalculateFilesSize(TStrings * FileList, __int64 & Size,
    int Params, const TCopyParamType * CopyParam = NULL, TCalculateSizeStats * Stats = NULL);
  void CalculateFilesChecksum(const std::wstring & Alg, TStrings * FileList,
    TStrings * Checksums, TCalculatedChecksumEvent OnCalculatedChecksum);
  void ClearCaches();
  TSynchronizeChecklist * SynchronizeCollect(const std::wstring LocalDirectory,
    const std::wstring RemoteDirectory, TSynchronizeMode Mode,
    const TCopyParamType * CopyParam, int Params,
    TSynchronizeDirectory OnSynchronizeDirectory, TSynchronizeOptions * Options);
  void SynchronizeApply(TSynchronizeChecklist * Checklist,
    const std::wstring LocalDirectory, const std::wstring RemoteDirectory,
    const TCopyParamType * CopyParam, int Params,
    TSynchronizeDirectory OnSynchronizeDirectory);
  void FilesFind(std::wstring Directory, const TFileMasks & FileMask,
    TFileFoundEvent OnFileFound, TFindingFileEvent OnFindingFile);
  void SpaceAvailable(const std::wstring Path, TSpaceAvailable & ASpaceAvailable);
  bool DirectoryFileList(const std::wstring Path,
    TRemoteFileList *& FileList, bool CanLoad);
  void MakeLocalFileList(const std::wstring FileName,
    const WIN32_FIND_DATA Rec, void * Param);
  std::wstring FileUrl(const std::wstring FileName);
  bool FileOperationLoopQuery(const std::exception & E,
    TFileOperationProgressType * OperationProgress, const std::wstring Message,
    bool AllowSkip, std::wstring SpecialRetry = L"");
  TUsableCopyParamAttrs UsableCopyParamAttrs(int Params);
  bool QueryReopen(const std::exception * E, int Params,
    TFileOperationProgressType * OperationProgress);
  std::wstring PeekCurrentDirectory();

  const TSessionInfo & GetSessionInfo();
  const TFileSystemInfo & GetFileSystemInfo(bool Retrieve = false);
  void inline LogEvent(const std::wstring & Str);

  static bool IsAbsolutePath(const std::wstring Path);
  static std::wstring ExpandFileName(std::wstring Path,
    const std::wstring BasePath);

  // __property TSessionData * SessionData = { read = FSessionData };
  TSessionData * GetSessionData() { return FSessionData; }
  // __property TSessionLog * Log = { read = FLog };
  TSessionLog * GetLog() { return FLog; }
  // __property TConfiguration * Configuration = { read = FConfiguration };
  TConfiguration * GetConfiguration() { return FConfiguration; }
  // __property bool Active = { read = GetActive };
  bool GetActive();
  // __property TSessionStatus Status = { read = FStatus };
  TSessionStatus GetStatus() { return FStatus; }
  // __property std::wstring CurrentDirectory = { read = GetCurrentDirectory, write = SetCurrentDirectory };
  std::wstring GetCurrentDirectory();
  void SetCurrentDirectory(std::wstring value);
  // __property bool ExceptionOnFail = { read = GetExceptionOnFail, write = SetExceptionOnFail };
  bool GetExceptionOnFail() const;
  void SetExceptionOnFail(bool value);
  // __property TRemoteDirectory * Files = { read = FFiles };
  TRemoteDirectory * GetFiles() { return FFiles; }
  const notify_signal_type &GetOnChangeDirectory() const { return FOnChangeDirectory; }
  void SetOnChangeDirectory(const notify_slot_type &value) { FOnChangeDirectory.connect(value); }
  TReadDirectoryEvent GetOnReadDirectory() { return FOnReadDirectory; }
  void SetOnReadDirectory(TReadDirectoryEvent value) { FOnReadDirectory = value; }
  const notify_signal_type &GetOnStartReadDirectory() const { return FOnStartReadDirectory; }
  void SetOnStartReadDirectory(const notify_slot_type &value) { FOnStartReadDirectory.connect(value); }
  TReadDirectoryProgressEvent GetOnReadDirectoryProgress() { return FOnReadDirectoryProgress; }
  void SetOnReadDirectoryProgress(TReadDirectoryProgressEvent value) { FOnReadDirectoryProgress = value; }
  TDeleteLocalFileEvent GetOnDeleteLocalFile() { return FOnDeleteLocalFile; }
  void SetOnDeleteLocalFile(TDeleteLocalFileEvent value) { FOnDeleteLocalFile = value; }
  // __property const TRemoteTokenList * Groups = { read = GetGroups };
  const TRemoteTokenList * GetGroups();
  // __property const TRemoteTokenList * Users = { read = GetUsers };
  const TRemoteTokenList * GetUsers();
  // __property const TRemoteTokenList * Membership = { read = GetMembership };
  const TRemoteTokenList * GetMembership();
  // __property TFileOperationProgressEvent OnProgress  = { read=FOnProgress, write=FOnProgress };
  const fileoperationprogress_signal_type &GetOnProgress() const { return FOnProgress; }
  void SetOnProgress(const fileoperationprogress_slot_type &value) { FOnProgress.connect(value); }
  // __property TFileOperationFinished OnFinished  = { read=FOnFinished, write=FOnFinished };
  const fileoperationfinished_signal_type &GetOnFinished() const { return FOnFinished; }
  void SetOnFinished(const fileoperationfinished_slot_type &value) { FOnFinished.connect(value); }
  // __property TCurrentFSProtocol FSProtocol = { read = FFSProtocol };
  TCurrentFSProtocol GetFSProtocol() { return FFSProtocol; }
  bool GetUseBusyCursor() { return FUseBusyCursor; }
  void SetUseBusyCursor(bool value) { FUseBusyCursor = value; }
  // __property std::wstring UserName = { read=GetUserName };
  std::wstring GetUserName();
  // __property bool IsCapable[TFSCapability Capability] = { read = GetIsCapable };
  bool GetIsCapable(TFSCapability Capability) const;
  // __property bool AreCachesEmpty = { read = GetAreCachesEmpty };
  bool GetAreCachesEmpty() const;
  // __property bool CommandSessionOpened = { read = GetCommandSessionOpened };
  bool GetCommandSessionOpened();
  // __property TTerminal * CommandSession = { read = GetCommandSession };
  TTerminal * GetCommandSession();
  bool GetAutoReadDirectory() { return FAutoReadDirectory; }
  void SetAutoReadDirectory(bool value) { FAutoReadDirectory = value; }
  // __property TStrings * FixedPaths = { read = GetFixedPaths };
  TStrings * GetFixedPaths();
  // __property bool ResolvingSymlinks = { read = GetResolvingSymlinks };
  bool GetResolvingSymlinks();
  // __property std::wstring Password = { read = GetPassword };
  std::wstring GetPassword();
  // __property std::wstring TunnelPassword = { read = GetTunnelPassword };
  std::wstring GetTunnelPassword();
  // __property bool StoredCredentialsTried = { read = GetStoredCredentialsTried };
  bool GetStoredCredentialsTried();
  TQueryUserEvent GetOnQueryUser() { return FOnQueryUser; }
  void SetOnQueryUser(TQueryUserEvent value) { FOnQueryUser = value; }
  TPromptUserEvent GetOnPromptUser() { return FOnPromptUser; }
  void SetOnPromptUser(TPromptUserEvent value) { FOnPromptUser = value; }
  TDisplayBannerEvent GetOnDisplayBanner() { return FOnDisplayBanner; }
  void SetOnDisplayBanner(TDisplayBannerEvent value) { FOnDisplayBanner = value; }
  TExtendedExceptionEvent GetOnShowExtendedException() { return FOnShowExtendedException; }
  void SetOnShowExtendedException(TExtendedExceptionEvent value) { FOnShowExtendedException = value; }
  TInformationEvent GetOnInformation() { return FOnInformation; }
  void SetOnInformation(TInformationEvent value) { FOnInformation = value; }
  const notify_signal_type &GetOnClose() const { return FOnClose; }
  void SetOnClose(const notify_slot_type &value) { FOnClose.connect(value); }
  // __property int TunnelLocalPortNumber = { read = FTunnelLocalPortNumber };
  int GetTunnelLocalPortNumber() { return FTunnelLocalPortNumber; }
};
//---------------------------------------------------------------------------
class TSecondaryTerminal : public TTerminal
{
public:
  TSecondaryTerminal(TTerminal * MainTerminal,
    TSessionData * SessionData, TConfiguration * Configuration,
    const std::wstring & Name);

protected:
  virtual void DirectoryLoaded(TRemoteFileList * FileList);
  virtual void DirectoryModified(const std::wstring Path,
    bool SubDirs);
  virtual bool DoPromptUser(TSessionData * Data, TPromptKind Kind,
    std::wstring Name, std::wstring Instructions, TStrings * Prompts, TStrings * Results);

private:
  bool FMasterPasswordTried;
  bool FMasterTunnelPasswordTried;
  TTerminal * FMainTerminal;
};
//---------------------------------------------------------------------------
class TTerminalList : public TObjectList
{
public:
  TTerminalList(TConfiguration * AConfiguration);
  ~TTerminalList();

  virtual TTerminal * NewTerminal(TSessionData * Data);
  virtual void FreeTerminal(TTerminal * Terminal);
  void FreeAndNullTerminal(TTerminal * & Terminal);
  virtual void Idle();
  void RecryptPasswords();

  // __property TTerminal * Terminals[int Index]  = { read=GetTerminal };
  TTerminal * GetTerminal(int Index);
  // __property int ActiveCount = { read = GetActiveCount };
  int GetActiveCount();

protected:
  virtual TTerminal * CreateTerminal(TSessionData * Data);

private:
  TConfiguration * FConfiguration;

};
//---------------------------------------------------------------------------
struct TCustomCommandParams
{
  std::wstring Command;
  int Params;
  TCaptureOutputEvent OutputEvent;
};
//---------------------------------------------------------------------------
struct TCalculateSizeStats
{
  TCalculateSizeStats();

  int Files;
  int Directories;
  int SymLinks;
};
//---------------------------------------------------------------------------
struct TCalculateSizeParams
{
  __int64 Size;
  int Params;
  const TCopyParamType * CopyParam;
  TCalculateSizeStats * Stats;
};
//---------------------------------------------------------------------------
struct TOverwriteFileParams
{
  TOverwriteFileParams();

  __int64 SourceSize;
  __int64 DestSize;
  TDateTime SourceTimestamp;
  TDateTime DestTimestamp;
  TModificationFmt SourcePrecision;
  TModificationFmt DestPrecision;
};
//---------------------------------------------------------------------------
struct TMakeLocalFileListParams
{
  TStrings * FileList;
  bool IncludeDirs;
  bool Recursive;
};
//---------------------------------------------------------------------------
struct TSynchronizeOptions
{
  TSynchronizeOptions();
  ~TSynchronizeOptions();

  TStringList * Filter;
};
//---------------------------------------------------------------------------
class TSynchronizeChecklist
{
friend class TTerminal;

public:
  enum TAction { saNone, saUploadNew, saDownloadNew, saUploadUpdate,
    saDownloadUpdate, saDeleteRemote, saDeleteLocal };
  static const int ActionCount = saDeleteLocal;

  class TItem
  {
  friend class TTerminal;

  public:
    struct TFileInfo
    {
      std::wstring FileName;
      std::wstring Directory;
      TDateTime Modification;
      TModificationFmt ModificationFmt;
      __int64 Size;
    };

    TAction Action;
    bool IsDirectory;
    TFileInfo Local;
    TFileInfo Remote;
    int ImageIndex;
    bool Checked;
    TRemoteFile * RemoteFile;

    const std::wstring& GetFileName() const;

    ~TItem();

  private:
    FILETIME FLocalLastWriteTime;

    TItem();
  };

  ~TSynchronizeChecklist();

  // __property int Count = { read = GetCount };
  int GetCount() const;
  // __property const TItem * Item[int Index] = { read = GetItem };
  const TItem * GetItem(int Index) const;

protected:
  TSynchronizeChecklist();

  void Sort();
  void Add(TItem * Item);


private:
  TList * FList;

  static int Compare(void * Item1, void * Item2);
};
//---------------------------------------------------------------------------
struct TSpaceAvailable
{
  TSpaceAvailable();

  __int64 BytesOnDevice;
  __int64 UnusedBytesOnDevice;
  __int64 BytesAvailableToUser;
  __int64 UnusedBytesAvailableToUser;
  unsigned long BytesPerAllocationUnit;
};
//---------------------------------------------------------------------------
#endif
