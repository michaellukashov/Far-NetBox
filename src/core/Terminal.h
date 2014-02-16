//------------------------------------------------------------------------------
#ifndef TerminalH
#define TerminalH

#include <CoreDefs.hpp>
#include <Classes.hpp>

#include "SessionInfo.h"
#include "Interface.h"
#include "FileOperationProgress.h"
#include "FileMasks.h"
#include "Exceptions.h"
//------------------------------------------------------------------------------
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
//------------------------------------------------------------------------------
DEFINE_CALLBACK_TYPE8(TQueryUserEvent, void,
  TObject * /* Sender */, const UnicodeString & /* Query */, TStrings * /* MoreMessages */ , uintptr_t /* Answers */,
  const TQueryParams * /* Params */, uintptr_t & /* Answer */, TQueryType /* QueryType */, void * /* Arg */);
DEFINE_CALLBACK_TYPE8(TPromptUserEvent, void,
  TTerminal * /* Terminal */, TPromptKind /* Kind */, const UnicodeString & /* Name */, const UnicodeString & /* Instructions */,
  TStrings * /* Prompts */, TStrings * /* Results */, bool & /* Result */, void * /* Arg */);
DEFINE_CALLBACK_TYPE5(TDisplayBannerEvent, void,
  TTerminal * /* Terminal */, const UnicodeString & /* SessionName */, const UnicodeString & /* Banner */,
  bool & /* NeverShowAgain */, intptr_t /* Options */);
DEFINE_CALLBACK_TYPE3(TExtendedExceptionEvent, void,
  TTerminal * /* Terminal */, Exception * /* E */, void * /* Arg */);
DEFINE_CALLBACK_TYPE2(TReadDirectoryEvent, void, TObject * /* Sender */, Boolean /* ReloadOnly */);
DEFINE_CALLBACK_TYPE3(TReadDirectoryProgressEvent, void,
  TObject * /* Sender */, intptr_t /* Progress */, bool & /* Cancel */);
DEFINE_CALLBACK_TYPE3(TProcessFileEvent, void,
  const UnicodeString & /* FileName */, const TRemoteFile * /* File */, void * /* Param */);
DEFINE_CALLBACK_TYPE4(TProcessFileEventEx, void,
  const UnicodeString & /* FileName */, const TRemoteFile * /* File */, void * /* Param */, intptr_t /* Index */);
DEFINE_CALLBACK_TYPE2(TFileOperationEvent, int,
  void * /* Param1 */, void * /* Param2 */);
DEFINE_CALLBACK_TYPE4(TSynchronizeDirectoryEvent, void,
  const UnicodeString & /* LocalDirectory */, const UnicodeString & /* RemoteDirectory */,
  bool & /* Continue */, bool /* Collect */);
DEFINE_CALLBACK_TYPE2(TDeleteLocalFileEvent, void,
  const UnicodeString & /* FileName */, bool /* Alternative */);
DEFINE_CALLBACK_TYPE3(TDirectoryModifiedEvent, int,
  TTerminal * /* Terminal */, const UnicodeString & /* Directory */, bool /* SubDirs */);
DEFINE_CALLBACK_TYPE4(TInformationEvent, void,
  TTerminal * /* Terminal */, const UnicodeString & /* Str */, bool /* Status */, intptr_t /* Phase */);
DEFINE_CALLBACK_TYPE5(TCreateLocalFileEvent, HANDLE,
  const UnicodeString & /* FileName */, DWORD /* DesiredAccess */,
  DWORD /* ShareMode */, DWORD /* CreationDisposition */, DWORD /* FlagsAndAttributes */);
DEFINE_CALLBACK_TYPE1(TGetLocalFileAttributesEvent, DWORD,
  const UnicodeString & /* FileName */);
DEFINE_CALLBACK_TYPE2(TSetLocalFileAttributesEvent, BOOL,
  const UnicodeString & /* FileName */, DWORD /* FileAttributes */);
DEFINE_CALLBACK_TYPE3(TMoveLocalFileEvent, BOOL,
  const UnicodeString & /* FileName */, const UnicodeString & /* NewFileName */, DWORD /* Flags */);
DEFINE_CALLBACK_TYPE1(TRemoveLocalDirectoryEvent, BOOL,
  const UnicodeString & /* LocalDirName */);
DEFINE_CALLBACK_TYPE2(TCreateLocalDirectoryEvent, BOOL,
  const UnicodeString & /* LocalDirName */, LPSECURITY_ATTRIBUTES /* SecurityAttributes */);
DEFINE_CALLBACK_TYPE0(TCheckForEscEvent, bool);
//------------------------------------------------------------------------------
#define SUSPEND_OPERATION(Command)                            \
  {                                                           \
    TSuspendFileOperationProgress Suspend(OperationProgress); \
    Command                                                   \
  }

inline void ThrowSkipFile(Exception * Exception, const UnicodeString & Message)
{
  throw EScpSkipFile(Exception, Message);
}
inline void ThrowSkipFileNull() { ThrowSkipFile(nullptr, L""); }

/* TODO : Better user interface (query to user) */
#define FILE_OPERATION_LOOP_CUSTOM(TERMINAL, ALLOW_SKIP, MESSAGE, OPERATION, HELPKEYWORD) { \
  bool DoRepeat;                                                            \
  do {                                                                      \
    DoRepeat = false;                                                       \
    try {                                                                   \
      OPERATION;                                                            \
    }                                                                       \
    catch (EAbort &)                                                        \
    {                                                                       \
      throw;                                                                \
    }                                                                       \
    catch (EScpSkipFile &)                                                  \
    {                                                                       \
      throw;                                                                \
    }                                                                       \
    catch (EFatal &)                                                        \
    {                                                                       \
      throw;                                                                \
    }                                                                       \
    catch (EFileNotFoundError &)                                            \
    {                                                                       \
      throw;                                                                \
    }                                                                       \
    catch (EOSError &)                                                      \
    {                                                                       \
      throw;                                                                \
    }                                                                       \
    catch (Exception & E)                                                   \
    {                                                                       \
      TERMINAL->FileOperationLoopQuery(                                     \
        E, OperationProgress, MESSAGE, ALLOW_SKIP, L"", HELPKEYWORD);       \
      DoRepeat = true;                                                      \
    } \
  } while (DoRepeat); }

#define FILE_OPERATION_LOOP(MESSAGE, OPERATION) \
  FILE_OPERATION_LOOP_EX(True, MESSAGE, OPERATION)
//------------------------------------------------------------------------------
enum TCurrentFSProtocol { cfsUnknown, cfsSCP, cfsSFTP, cfsFTP, cfsFTPS, cfsWebDAV };
//------------------------------------------------------------------------------
const int cpDelete = 0x01;
const int cpTemporary = 0x04;
const int cpNoConfirmation = 0x08;
const int cpNewerOnly = 0x10;
const int cpAppend = 0x20;
const int cpResume = 0x40;
//------------------------------------------------------------------------------
const int ccApplyToDirectories = 0x01;
const int ccRecursive = 0x02;
const int ccUser = 0x100;
//------------------------------------------------------------------------------
const int csIgnoreErrors = 0x01;
//------------------------------------------------------------------------------
const int ropNoReadDirectory = 0x02;
//------------------------------------------------------------------------------
const int boDisableNeverShowAgain = 0x01;
//------------------------------------------------------------------------------
class TTerminal : public TObject, public TSessionUI
{
NB_DISABLE_COPY(TTerminal)
NB_DECLARE_CLASS(TTerminal)
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
  static const int spSelectedOnly = 0x800; // not used by core
  static const int spMirror = 0x1000;

// for TranslateLockedPath()
friend class TRemoteFile;
// for ReactOnCommand()
friend class TSCPFileSystem;
friend class TSFTPFileSystem;
friend class TFTPFileSystem;
friend class TWebDAVFileSystem;

friend class TTunnelUI;
friend class TCallbackGuard;

public:
  void CommandError(Exception * E, const UnicodeString & Msg);
  uintptr_t CommandError(Exception * E, const UnicodeString & Msg,
    uintptr_t Answers, const UnicodeString & HelpKeyword = L"");
  void SetMasks(const UnicodeString & Value);
  UnicodeString GetCurrentDirectory();
  bool GetExceptionOnFail() const;
  const TRemoteTokenList * GetGroups();
  const TRemoteTokenList * GetUsers();
  const TRemoteTokenList * GetMembership();
  void SetCurrentDirectory(const UnicodeString & Value);
  void SetExceptionOnFail(bool Value);
  void ReactOnCommand(intptr_t /*TFSCommand*/ Cmd);
  UnicodeString GetUserName() const;
  bool GetAreCachesEmpty() const;
  bool GetIsCapable(TFSCapability Capability) const;
  void ClearCachedFileList(const UnicodeString & Path, bool SubDirs);
  void AddCachedFileList(TRemoteFileList * FileList);
  bool GetCommandSessionOpened() const;
  TTerminal * GetCommandSession();
  bool GetResolvingSymlinks() const;
  bool GetActive() const;
  UnicodeString GetPassword();
  void SetPassword(const UnicodeString & Value) { FPassword = Value; }
  UnicodeString GetTunnelPassword();
  void SetTunnelPassword(const UnicodeString & Value) { FTunnelPassword = Value; }
  bool GetStoredCredentialsTried();
  TCustomFileSystem * GetFileSystem() const { return FFileSystem; }
  TCustomFileSystem * GetFileSystem() { return FFileSystem; }
  inline bool InTransaction();
  static UnicodeString SynchronizeModeStr(TSynchronizeMode Mode);
  static UnicodeString SynchronizeParamsStr(intptr_t Params);

public:
  explicit TTerminal();
  void Init(TSessionData * SessionData, TConfiguration * Configuration);
  virtual ~TTerminal();
  void Open();
  void Close();
  void Reopen(intptr_t Params);
  virtual void DirectoryModified(const UnicodeString & Path, bool SubDirs);
  virtual void DirectoryLoaded(TRemoteFileList * FileList);
  void ShowExtendedException(Exception * E);
  void Idle();
  void RecryptPasswords();
  bool AllowedAnyCommand(const UnicodeString & Command) const;
  void AnyCommand(const UnicodeString & Command, TCaptureOutputEvent OutputEvent);
  void CloseOnCompletion(TOnceDoneOperation Operation = odoDisconnect, const UnicodeString & Message = L"");
  UnicodeString AbsolutePath(const UnicodeString & Path, bool Local);
  void BeginTransaction();
  void ReadCurrentDirectory();
  void ReadDirectory(bool ReloadOnly, bool ForceCache = false);
  TRemoteFileList * ReadDirectoryListing(const UnicodeString & Directory, const TFileMasks & Mask);
  TRemoteFileList * CustomReadDirectoryListing(const UnicodeString & Directory, bool UseCache);
  TRemoteFile * ReadFileListing(const UnicodeString & Path);
  void ReadFile(const UnicodeString & FileName, TRemoteFile *& AFile);
  bool FileExists(const UnicodeString & FileName, TRemoteFile ** File = nullptr);
  void ReadSymlink(TRemoteFile * SymlinkFile, TRemoteFile *& File);
  bool CopyToLocal(TStrings * AFilesToCopy,
    const UnicodeString & TargetDir, const TCopyParamType * CopyParam, intptr_t Params);
  bool CopyToRemote(TStrings * AFilesToCopy,
    const UnicodeString & TargetDir, const TCopyParamType * CopyParam, intptr_t Params);
  void CreateDirectory(const UnicodeString & DirName,
    const TRemoteProperties * Properties = nullptr);
  void CreateLink(const UnicodeString & FileName, const UnicodeString & PointTo, bool Symbolic);
  void DeleteFile(const UnicodeString & FileName,
    const TRemoteFile * AFile = nullptr, void * Params = nullptr);
  bool DeleteFiles(TStrings * FilesToDelete, intptr_t Params = 0);
  bool DeleteLocalFiles(TStrings * FileList, intptr_t Params = 0);
  bool IsRecycledFile(const UnicodeString & FileName);
  void CustomCommandOnFile(const UnicodeString & FileName,
    const TRemoteFile * AFile, void * AParams);
  void CustomCommandOnFiles(const UnicodeString & Command, intptr_t Params,
    TStrings * Files, TCaptureOutputEvent OutputEvent);
  void ChangeDirectory(const UnicodeString & Directory);
  void EndTransaction();
  void HomeDirectory();
  void ChangeFileProperties(const UnicodeString & FileName,
    const TRemoteFile * File, /*const TRemoteProperties */ void * Properties);
  void ChangeFilesProperties(TStrings * FileList,
    const TRemoteProperties * Properties);
  bool LoadFilesProperties(TStrings * FileList);
  void TerminalError(const UnicodeString & Msg, const UnicodeString & HelpKeyword = L"");
  void TerminalError(Exception * E, const UnicodeString & Msg, const UnicodeString & HelpKeyword = L"");
  void ReloadDirectory();
  void RefreshDirectory();
  void RenameFile(const UnicodeString & FileName, const UnicodeString & NewName);
  void RenameFile(const TRemoteFile * File, const UnicodeString & NewName, bool CheckExistence);
  void MoveFile(const UnicodeString & FileName, const TRemoteFile * File,
    /* const TMoveFileParams */ void * Param);
  bool MoveFiles(TStrings * FileList, const UnicodeString & Target,
    const UnicodeString & FileMask);
  void CopyFile(const UnicodeString & FileName, const TRemoteFile * File,
    /* const TMoveFileParams */ void * Param);
  bool CopyFiles(TStrings * FileList, const UnicodeString & Target,
    const UnicodeString & FileMask);
  bool CalculateFilesSize(TStrings * FileList, __int64 & Size,
    intptr_t Params, const TCopyParamType * CopyParam,
    bool AllowDirs, TCalculateSizeStats * Stats = nullptr);
  void CalculateFilesChecksum(const UnicodeString & Alg, TStrings * FileList,
    TStrings * Checksums, TCalculatedChecksumEvent OnCalculatedChecksum);
  void ClearCaches();
  TSynchronizeChecklist * SynchronizeCollect(const UnicodeString & LocalDirectory,
    const UnicodeString & RemoteDirectory, TSynchronizeMode Mode,
    const TCopyParamType * CopyParam, intptr_t Params,
    TSynchronizeDirectoryEvent OnSynchronizeDirectory, TSynchronizeOptions * Options);
  void SynchronizeApply(TSynchronizeChecklist * Checklist,
    const UnicodeString & LocalDirectory, const UnicodeString & RemoteDirectory,
    const TCopyParamType * CopyParam, intptr_t Params,
    TSynchronizeDirectoryEvent OnSynchronizeDirectory);
  void FilesFind(const UnicodeString & Directory, const TFileMasks & FileMask,
    TFileFoundEvent OnFileFound, TFindingFileEvent OnFindingFile);
  void SpaceAvailable(const UnicodeString & Path, TSpaceAvailable & ASpaceAvailable);
  bool DirectoryFileList(const UnicodeString & Path,
    TRemoteFileList *& FileList, bool CanLoad);
  void MakeLocalFileList(const UnicodeString & FileName,
    const TSearchRec & Rec, void * Param);
  UnicodeString FileUrl(const UnicodeString & FileName) const;
  bool FileOperationLoopQuery(Exception & E,
    TFileOperationProgressType * OperationProgress,
    const UnicodeString & Message,
    bool AllowSkip, const UnicodeString & SpecialRetry = UnicodeString(),
    const UnicodeString & HelpKeyword = L"");
  TUsableCopyParamAttrs UsableCopyParamAttrs(intptr_t Params);
  bool QueryReopen(Exception * E, intptr_t Params,
    TFileOperationProgressType * OperationProgress);
  UnicodeString PeekCurrentDirectory();
  void FatalAbort();
  void ReflectSettings();
  void CollectUsage();
  bool CheckForEsc();

  const TSessionInfo & GetSessionInfo() const;
  const TFileSystemInfo & GetFileSystemInfo(bool Retrieve = false);
  void LogEvent(const UnicodeString & Str);

  static UnicodeString ExpandFileName(const UnicodeString & Path,
    const UnicodeString & BasePath);

  TSessionData * GetSessionData() const { return FSessionData; }
  TSessionData * GetSessionData() { return FSessionData; }
  TSessionLog * GetLog() const { return FLog; }
  TSessionLog * GetLog() { return FLog; }
  TActionLog * GetActionLog() const { return FActionLog; }
  const TConfiguration * GetConfiguration() const { return FConfiguration; }
  TConfiguration * GetConfiguration() { return FConfiguration; }
  TSessionStatus GetStatus() const { return FStatus; }
  TRemoteDirectory * GetFiles() const { return FFiles; }
  TNotifyEvent & GetOnChangeDirectory() { return FOnChangeDirectory; }
  void SetOnChangeDirectory(TNotifyEvent Value) { FOnChangeDirectory = Value; }
  TReadDirectoryEvent & GetOnReadDirectory() { return FOnReadDirectory; }
  void SetOnReadDirectory(TReadDirectoryEvent Value) { FOnReadDirectory = Value; }
  TNotifyEvent & GetOnStartReadDirectory() { return FOnStartReadDirectory; }
  void SetOnStartReadDirectory(TNotifyEvent Value) { FOnStartReadDirectory = Value; }
  TReadDirectoryProgressEvent & GetOnReadDirectoryProgress() { return FOnReadDirectoryProgress; }
  void SetOnReadDirectoryProgress(TReadDirectoryProgressEvent Value) { FOnReadDirectoryProgress = Value; }
  TDeleteLocalFileEvent & GetOnDeleteLocalFile() { return FOnDeleteLocalFile; }
  void SetOnDeleteLocalFile(TDeleteLocalFileEvent Value) { FOnDeleteLocalFile = Value; }
  TNotifyEvent & GetOnInitializeLog() { return FOnInitializeLog; }
  void SetOnInitializeLog(TNotifyEvent Value) { FOnInitializeLog = Value; }
  TCreateLocalFileEvent & GetOnCreateLocalFile() { return FOnCreateLocalFile; }
  void SetOnCreateLocalFile(TCreateLocalFileEvent Value) { FOnCreateLocalFile = Value; }
  TGetLocalFileAttributesEvent & GetOnGetLocalFileAttributes() { return FOnGetLocalFileAttributes; }
  void SetOnGetLocalFileAttributes(TGetLocalFileAttributesEvent Value) { FOnGetLocalFileAttributes = Value; }
  TSetLocalFileAttributesEvent & GetOnSetLocalFileAttributes() { return FOnSetLocalFileAttributes; }
  void SetOnSetLocalFileAttributes(TSetLocalFileAttributesEvent Value) { FOnSetLocalFileAttributes = Value; }
  TMoveLocalFileEvent & GetOnMoveLocalFile() { return FOnMoveLocalFile; }
  void SetOnMoveLocalFile(TMoveLocalFileEvent Value) { FOnMoveLocalFile = Value; }
  TRemoveLocalDirectoryEvent & GetOnRemoveLocalDirectory() { return FOnRemoveLocalDirectory; }
  void SetOnRemoveLocalDirectory(TRemoveLocalDirectoryEvent Value) { FOnRemoveLocalDirectory = Value; }
  TCreateLocalDirectoryEvent & GetOnCreateLocalDirectory() { return FOnCreateLocalDirectory; }
  void SetOnCreateLocalDirectory(TCreateLocalDirectoryEvent Value) { FOnCreateLocalDirectory = Value; }
  TFileOperationProgressEvent & GetOnProgress() { return FOnProgress; }
  void SetOnProgress(TFileOperationProgressEvent Value) { FOnProgress = Value; }
  TFileOperationFinishedEvent &  GetOnFinished() { return FOnFinished; }
  void SetOnFinished(TFileOperationFinishedEvent Value) { FOnFinished = Value; }
  TCurrentFSProtocol GetFSProtocol() const { return FFSProtocol; }
  bool GetUseBusyCursor() const { return FUseBusyCursor; }
  void SetUseBusyCursor(bool Value) { FUseBusyCursor = Value; }
  bool GetAutoReadDirectory() const { return FAutoReadDirectory; }
  void SetAutoReadDirectory(bool Value) { FAutoReadDirectory = Value; }
  TStrings * GetFixedPaths();
  TQueryUserEvent & GetOnQueryUser() { return FOnQueryUser; }
  void SetOnQueryUser(TQueryUserEvent Value) { FOnQueryUser = Value; }
  TPromptUserEvent & GetOnPromptUser() { return FOnPromptUser; }
  void SetOnPromptUser(TPromptUserEvent Value) { FOnPromptUser = Value; }
  TDisplayBannerEvent & GetOnDisplayBanner() { return FOnDisplayBanner; }
  void SetOnDisplayBanner(TDisplayBannerEvent Value) { FOnDisplayBanner = Value; }
  TExtendedExceptionEvent & GetOnShowExtendedException() { return FOnShowExtendedException; }
  void SetOnShowExtendedException(TExtendedExceptionEvent Value) { FOnShowExtendedException = Value; }
  TInformationEvent & GetOnInformation() { return FOnInformation; }
  void SetOnInformation(TInformationEvent Value) { FOnInformation = Value; }
  TCheckForEscEvent & GetOnCheckForEsc() { return FOnCheckForEsc; }
  void SetOnCheckForEsc(TCheckForEscEvent Value) { FOnCheckForEsc = Value; }
  TNotifyEvent & GetOnClose() { return FOnClose; }
  void SetOnClose(TNotifyEvent Value) { FOnClose = Value; }
  intptr_t GetTunnelLocalPortNumber() const { return FTunnelLocalPortNumber; }

protected:
  bool FReadCurrentDirectoryPending;
  bool FReadDirectoryPending;
  bool FTunnelOpening;

  void DoStartReadDirectory();
  void DoReadDirectoryProgress(intptr_t Progress, bool & Cancel);
  void DoReadDirectory(bool ReloadOnly);
  void DoCreateDirectory(const UnicodeString & DirName);
  void DoDeleteFile(const UnicodeString & FileName, const TRemoteFile * File,
    intptr_t Params);
  void DoCustomCommandOnFile(const UnicodeString & FileName,
    const TRemoteFile * AFile, const UnicodeString & Command, intptr_t Params,
    TCaptureOutputEvent OutputEvent);
  void DoRenameFile(const UnicodeString & FileName,
    const UnicodeString & NewName, bool Move);
  void DoCopyFile(const UnicodeString & FileName, const UnicodeString & NewName);
  void DoChangeFileProperties(const UnicodeString & vFileName,
    const TRemoteFile * File, const TRemoteProperties * Properties);
  void DoChangeDirectory();
  void DoInitializeLog();
  void EnsureNonExistence(const UnicodeString & FileName);
  void LookupUsersGroups();
  void FileModified(const TRemoteFile * File,
    const UnicodeString & FileName, bool ClearDirectoryChange = false);
  intptr_t FileOperationLoop(TFileOperationEvent CallBackFunc,
    TFileOperationProgressType * OperationProgress, bool AllowSkip,
    const UnicodeString & Message, void * Param1 = nullptr, void * Param2 = nullptr);
  bool ProcessFiles(TStrings * FileList, TFileOperation Operation,
    TProcessFileEvent ProcessFile, void * Param = nullptr, TOperationSide Side = osRemote,
    bool Ex = false);
  bool ProcessFilesEx(TStrings * FileList, TFileOperation Operation,
    TProcessFileEventEx ProcessFile, void * Param = nullptr, TOperationSide Side = osRemote);
  void ProcessDirectory(const UnicodeString & DirName,
    TProcessFileEvent CallBackFunc, void * Param = nullptr, bool UseCache = false,
    bool IgnoreErrors = false);
  void AnnounceFileListOperation();
  UnicodeString TranslateLockedPath(const UnicodeString & Path, bool Lock);
  void ReadDirectory(TRemoteFileList * FileList);
  void CustomReadDirectory(TRemoteFileList * FileList);
  void DoCreateLink(const UnicodeString & FileName, const UnicodeString & PointTo,
    bool Symbolic);
  bool CreateLocalFile(const UnicodeString & FileName,
    TFileOperationProgressType * OperationProgress, HANDLE * AHandle,
    bool NoConfirmation);
  void OpenLocalFile(const UnicodeString & FileName, uintptr_t Access,
    uintptr_t * AAttrs, HANDLE * AHandle, __int64 * ACTime, __int64 * AMTime,
    __int64 * AATime, __int64 * ASize, bool TryWriteReadOnly = true);
  bool AllowLocalFileTransfer(const UnicodeString & FileName, const TCopyParamType * CopyParam);
  bool HandleException(Exception * E);
  void CalculateFileSize(const UnicodeString & FileName,
    const TRemoteFile * File, /*TCalculateSizeParams*/ void * Size);
  void DoCalculateDirectorySize(const UnicodeString & FileName,
    const TRemoteFile * File, TCalculateSizeParams * Params);
  void CalculateLocalFileSize(const UnicodeString & FileName,
    const TSearchRec & Rec, /*__int64*/ void * Params);
  bool CalculateLocalFilesSize(TStrings * FileList, __int64 & Size,
    const TCopyParamType * CopyParam, bool AllowDirs);
  TBatchOverwrite EffectiveBatchOverwrite(
    const TCopyParamType * CopyParam, intptr_t Params, TFileOperationProgressType * OperationProgress,
    bool Special);
  bool CheckRemoteFile(
    const TCopyParamType * CopyParam, intptr_t Params, TFileOperationProgressType * OperationProgress);
  uintptr_t ConfirmFileOverwrite(const UnicodeString & FileName,
    const TOverwriteFileParams * FileParams, uintptr_t Answers, TQueryParams * QueryParams,
    TOperationSide Side, const TCopyParamType * CopyParam, intptr_t Params,
    TFileOperationProgressType * OperationProgress, const UnicodeString & Message = L"");
  void DoSynchronizeCollectDirectory(const UnicodeString & LocalDirectory,
    const UnicodeString & RemoteDirectory, TSynchronizeMode Mode,
    const TCopyParamType * CopyParam, intptr_t Params,
    TSynchronizeDirectoryEvent OnSynchronizeDirectory,
    TSynchronizeOptions * Options, intptr_t Level, TSynchronizeChecklist * Checklist);
  void SynchronizeCollectFile(const UnicodeString & FileName,
    const TRemoteFile * File, /*TSynchronizeData*/ void * Param);
  void SynchronizeRemoteTimestamp(const UnicodeString & FileName,
    const TRemoteFile * File, void * Param);
  void SynchronizeLocalTimestamp(const UnicodeString & FileName,
    const TRemoteFile * File, void * Param);
  void DoSynchronizeProgress(const TSynchronizeData & Data, bool Collect);
  void DeleteLocalFile(const UnicodeString & FileName,
    const TRemoteFile * File, void * Param);
  void RecycleFile(const UnicodeString & FileName, const TRemoteFile * File);
  void DoStartup();
  virtual bool DoQueryReopen(Exception * E);
  virtual void FatalError(Exception * E, const UnicodeString & Msg, const UnicodeString & HelpKeyword = L"");
  void ResetConnection();
  virtual bool DoPromptUser(TSessionData * Data, TPromptKind Kind,
    const UnicodeString & Name, const UnicodeString & Instructions, TStrings* Prompts,
    TStrings * Response);
  void SetupTunnelLocalPortNumber();
  void OpenTunnel();
  void CloseTunnel();
  void DoInformation(const UnicodeString & Str, bool Status, intptr_t Phase = -1);
  UnicodeString FileUrl(const UnicodeString & Protocol, const UnicodeString & FileName) const;
  bool PromptUser(TSessionData * Data, TPromptKind Kind,
    const UnicodeString & Name, const UnicodeString & Instructions, const UnicodeString & Prompt,
    bool Echo,
    intptr_t MaxLen, UnicodeString & AResult);
  void FileFind(const UnicodeString & FileName, const TRemoteFile * File, void * Param);
  void DoFilesFind(const UnicodeString & Directory, TFilesFindParams & Params);
  bool DoCreateLocalFile(const UnicodeString & FileName,
    TFileOperationProgressType * OperationProgress, HANDLE * AHandle,
    bool NoConfirmation);

  virtual void Information(const UnicodeString & Str, bool Status);
  virtual uintptr_t QueryUser(const UnicodeString & Query,
    TStrings * MoreMessages, uintptr_t Answers, const TQueryParams * Params,
    TQueryType QueryType = qtConfirmation);
  virtual uintptr_t QueryUserException(const UnicodeString & Query,
    Exception * E, uintptr_t Answers, const TQueryParams * Params,
    TQueryType QueryType = qtConfirmation);
  virtual bool PromptUser(TSessionData * Data, TPromptKind Kind,
    const UnicodeString & Name, const UnicodeString & Instructions, TStrings * Prompts,
    TStrings * Results);
  virtual void DisplayBanner(const UnicodeString & Banner);
  virtual void Closed();
  virtual void HandleExtendedException(Exception * E);
  bool IsListenerFree(uintptr_t PortNumber) const;
  void DoProgress(TFileOperationProgressType & ProgressData, TCancelStatus & Cancel);
  void DoFinished(TFileOperation Operation, TOperationSide Side, bool Temp,
    const UnicodeString & FileName, bool Success, TOnceDoneOperation & OnceDoneOperation);
  void RollbackAction(TSessionAction & Action,
    TFileOperationProgressType * OperationProgress, Exception * E = nullptr);
  void DoAnyCommand(const UnicodeString & Command, TCaptureOutputEvent OutputEvent,
    TCallSessionAction * Action);
  TRemoteFileList * DoReadDirectoryListing(const UnicodeString & Directory,
    bool UseCache);
  RawByteString EncryptPassword(const UnicodeString & Password);
  UnicodeString DecryptPassword(const RawByteString & Password);
  void LogRemoteFile(TRemoteFile * AFile);
  UnicodeString FormatFileDetailsForLog(const UnicodeString & FileName, TDateTime Modification, __int64 Size);
  void LogFileDetails(const UnicodeString & FileName, TDateTime Modification, __int64 Size);
  virtual TTerminal * GetPasswordSource();

  TFileOperationProgressType * GetOperationProgress() const { return FOperationProgress; }

  void SetLocalFileTime(const UnicodeString & LocalFileName,
    const TDateTime & Modification);
  void SetLocalFileTime(const UnicodeString & LocalFileName,
    FILETIME * AcTime, FILETIME * WrTime);
  HANDLE CreateLocalFile(const UnicodeString & LocalFileName, DWORD DesiredAccess,
    DWORD ShareMode, DWORD CreationDisposition, DWORD FlagsAndAttributes);
  DWORD GetLocalFileAttributes(const UnicodeString & LocalFileName);
  BOOL SetLocalFileAttributes(const UnicodeString & LocalFileName, DWORD FileAttributes);
  BOOL MoveLocalFile(const UnicodeString & LocalFileName, const UnicodeString & NewLocalFileName, DWORD Flags);
  BOOL RemoveLocalDirectory(const UnicodeString & LocalDirName);
  BOOL CreateLocalDirectory(const UnicodeString & LocalDirName, LPSECURITY_ATTRIBUTES SecurityAttributes);

private:
  void InternalTryOpen();
  void InternalDoTryOpen();
  void InitFileSystem();

private:
  TSessionData * FSessionData;
  TSessionLog * FLog;
  TActionLog * FActionLog;
  TConfiguration * FConfiguration;
  UnicodeString FCurrentDirectory;
  UnicodeString FLockDirectory;
  Integer FExceptionOnFail;
  TRemoteDirectory * FFiles;
  intptr_t FInTransaction;
  bool FSuspendTransaction;
  TNotifyEvent FOnChangeDirectory;
  TReadDirectoryEvent FOnReadDirectory;
  TNotifyEvent FOnStartReadDirectory;
  TReadDirectoryProgressEvent FOnReadDirectoryProgress;
  TDeleteLocalFileEvent FOnDeleteLocalFile;
  TCreateLocalFileEvent FOnCreateLocalFile;
  TGetLocalFileAttributesEvent FOnGetLocalFileAttributes;
  TSetLocalFileAttributesEvent FOnSetLocalFileAttributes;
  TMoveLocalFileEvent FOnMoveLocalFile;
  TRemoveLocalDirectoryEvent FOnRemoveLocalDirectory;
  TCreateLocalDirectoryEvent FOnCreateLocalDirectory;
  TNotifyEvent FOnInitializeLog;
  TRemoteTokenList FMembership;
  TRemoteTokenList FGroups;
  TRemoteTokenList FUsers;
  bool FUsersGroupsLookedup;
  TFileOperationProgressEvent FOnProgress;
  TFileOperationFinishedEvent FOnFinished;
  TFileOperationProgressType * FOperationProgress;
  bool FUseBusyCursor;
  TRemoteDirectoryCache * FDirectoryCache;
  TRemoteDirectoryChangesCache * FDirectoryChangesCache;
  TCustomFileSystem * FFileSystem;
  TSecureShell * FSecureShell;
  UnicodeString FLastDirectoryChange;
  TCurrentFSProtocol FFSProtocol;
  TTerminal * FCommandSession;
  bool FAutoReadDirectory;
  bool FReadingCurrentDirectory;
  bool * FClosedOnCompletion;
  TSessionStatus FStatus;
  RawByteString FPassword;
  RawByteString FTunnelPassword;
  TTunnelThread * FTunnelThread;
  TSecureShell * FTunnel;
  TSessionData * FTunnelData;
  TSessionLog * FTunnelLog;
  TTunnelUI * FTunnelUI;
  intptr_t FTunnelLocalPortNumber;
  UnicodeString FTunnelError;
  TQueryUserEvent FOnQueryUser;
  TPromptUserEvent FOnPromptUser;
  TDisplayBannerEvent FOnDisplayBanner;
  TExtendedExceptionEvent FOnShowExtendedException;
  TInformationEvent FOnInformation;
  TNotifyEvent FOnClose;
  TCheckForEscEvent FOnCheckForEsc;
  TCallbackGuard * FCallbackGuard;
  TFindingFileEvent FOnFindingFile;
  bool FEnableSecureShellUsage;
  bool FCollectFileSystemUsage;
  bool FRememberedPasswordTried;
  bool FRememberedTunnelPasswordTried;
};
//------------------------------------------------------------------------------
class TSecondaryTerminal : public TTerminal
{
NB_DISABLE_COPY(TSecondaryTerminal)
public:
  explicit TSecondaryTerminal(TTerminal * MainTerminal);
  virtual ~TSecondaryTerminal() {}
  void Init(TSessionData * SessionData, TConfiguration * Configuration,
    const UnicodeString & Name);

  TTerminal * GetMainTerminal() const { return FMainTerminal; }

protected:
  virtual void DirectoryLoaded(TRemoteFileList * FileList);
  virtual void DirectoryModified(const UnicodeString & Path,
    bool SubDirs);
  virtual TTerminal * GetPasswordSource();

private:
  TTerminal * FMainTerminal;
};
//------------------------------------------------------------------------------
class TTerminalList : public TObjectList
{
NB_DISABLE_COPY(TTerminalList)
public:
  explicit TTerminalList(TConfiguration * AConfiguration);
  virtual ~TTerminalList();

  virtual TTerminal * NewTerminal(TSessionData * Data);
  virtual void FreeTerminal(TTerminal * Terminal);
  void FreeAndNullTerminal(TTerminal *& Terminal);
  virtual void Idle();
  void RecryptPasswords();

  void SetMasks(const UnicodeString & Value);
  TTerminal * GetTerminal(intptr_t Index);

protected:
  virtual TTerminal * CreateTerminal(TSessionData * Data);

private:
  TConfiguration * FConfiguration;
};
//------------------------------------------------------------------------------
struct TCustomCommandParams : public TObject
{
  UnicodeString Command;
  intptr_t Params;
  TCaptureOutputEvent OutputEvent;
};
//------------------------------------------------------------------------------
struct TCalculateSizeStats : public TObject
{
  TCalculateSizeStats();

  intptr_t Files;
  intptr_t Directories;
  intptr_t SymLinks;
};
//------------------------------------------------------------------------------
struct TCalculateSizeParams : public TObject
{
  __int64 Size;
  intptr_t Params;
  const TCopyParamType * CopyParam;
  TCalculateSizeStats * Stats;
  bool AllowDirs;
  bool Result;
};
//------------------------------------------------------------------------------
struct TMakeLocalFileListParams : public TObject
{
  TStrings * FileList;
  bool IncludeDirs;
  bool Recursive;
};
//------------------------------------------------------------------------------
struct TSynchronizeOptions : public TObject
{
NB_DISABLE_COPY(TSynchronizeOptions)
public:
  TSynchronizeOptions();
  ~TSynchronizeOptions();

  TStringList * Filter;

  bool FilterFind(const UnicodeString & FileName);
  bool MatchesFilter(const UnicodeString & FileName);
};
//------------------------------------------------------------------------------
class TSynchronizeChecklist : public TObject
{
friend class TTerminal;
NB_DISABLE_COPY(TSynchronizeChecklist)
public:
  enum TAction { saNone, saUploadNew, saDownloadNew, saUploadUpdate,
    saDownloadUpdate, saDeleteRemote, saDeleteLocal };
  static const intptr_t ActionCount = saDeleteLocal;

  class TItem : public TObject
  {
  friend class TTerminal;

  public:
    struct TFileInfo : public TObject
    {
      UnicodeString FileName;
      UnicodeString Directory;
      TDateTime Modification;
      TModificationFmt ModificationFmt;
      __int64 Size;
    };

    TAction Action;
    bool IsDirectory;
    TFileInfo Local;
    TFileInfo Remote;
    intptr_t ImageIndex;
    bool Checked;
    TRemoteFile * RemoteFile;

    const UnicodeString & GetFileName() const;

    ~TItem();

  private:
    FILETIME FLocalLastWriteTime;

    TItem();
  };

  ~TSynchronizeChecklist();

  intptr_t GetCount() const;
  const TItem * GetItem(intptr_t Index) const;

protected:
  TSynchronizeChecklist();

  void Sort();
  void Add(TItem * Item);

public:
  void SetMasks(const UnicodeString & Value);

private:
  TList * FList;

  static intptr_t Compare(const void * Item1, const void * Item2);
};
//------------------------------------------------------------------------------
struct TSpaceAvailable : public TObject
{
  TSpaceAvailable();

  __int64 BytesOnDevice;
  __int64 UnusedBytesOnDevice;
  __int64 BytesAvailableToUser;
  __int64 UnusedBytesAvailableToUser;
  uintptr_t BytesPerAllocationUnit;
};
//------------------------------------------------------------------------------
UnicodeString GetSessionUrl(const TTerminal * Terminal, bool WithUserName = false);
//------------------------------------------------------------------------------
#endif
