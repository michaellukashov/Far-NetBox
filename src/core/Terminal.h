#pragma once

#include <CoreDefs.hpp>
#include <Classes.hpp>

#include "SessionInfo.h"
#include "Interface.h"
#include "FileOperationProgress.h"
#include "FileMasks.h"
#include "Exceptions.h"

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

DEFINE_CALLBACK_TYPE8(TQueryUserEvent, void,
  TObject * /*Sender*/, const UnicodeString & /*Query*/, TStrings * /*MoreMessages*/ ,
  uintptr_t /*Answers*/,
  const TQueryParams * /*Params*/, uintptr_t & /*Answer*/,
  TQueryType /*QueryType*/, void * /*Arg*/);
DEFINE_CALLBACK_TYPE8(TPromptUserEvent, void,
  TTerminal * /*Terminal*/, TPromptKind /*Kind*/, const UnicodeString & /*Name*/,
  const UnicodeString & /*Instructions*/,
  TStrings * /*Prompts*/, TStrings * /*Results*/, bool & /*Result*/, void * /*Arg*/);
DEFINE_CALLBACK_TYPE5(TDisplayBannerEvent, void,
  TTerminal * /*Terminal*/, const UnicodeString & /*SessionName*/,
  const UnicodeString & /*Banner*/,
  bool & /*NeverShowAgain*/, intptr_t /*Options*/);
DEFINE_CALLBACK_TYPE3(TExtendedExceptionEvent, void,
  TTerminal * /*Terminal*/, Exception * /*E*/, void * /*Arg*/);
DEFINE_CALLBACK_TYPE2(TReadDirectoryEvent, void, TObject * /*Sender*/,
  Boolean /*ReloadOnly*/);
DEFINE_CALLBACK_TYPE4(TReadDirectoryProgressEvent, void,
  TObject * /*Sender*/, intptr_t /*Progress*/, intptr_t /*ResolvedLinks*/,
  bool & /*Cancel*/);
DEFINE_CALLBACK_TYPE3(TProcessFileEvent, void,
  const UnicodeString & /*FileName*/, const TRemoteFile * /*File*/,
  void * /*Param*/);
DEFINE_CALLBACK_TYPE4(TProcessFileEventEx, void,
  const UnicodeString & /*FileName*/, const TRemoteFile * /*File*/,
  void * /*Param*/, intptr_t /*Index*/);
DEFINE_CALLBACK_TYPE2(TFileOperationEvent, intptr_t,
  void * /*Param1*/, void * /*Param2*/);
DEFINE_CALLBACK_TYPE4(TSynchronizeDirectoryEvent, void,
  const UnicodeString & /*LocalDirectory*/, const UnicodeString & /*RemoteDirectory*/,
  bool & /*Continue*/, bool /*Collect*/);
DEFINE_CALLBACK_TYPE2(TDeleteLocalFileEvent, void,
  const UnicodeString & /*FileName*/, bool /*Alternative*/);
DEFINE_CALLBACK_TYPE3(TDirectoryModifiedEvent, int,
  TTerminal * /*Terminal*/, const UnicodeString & /*Directory*/, bool /*SubDirs*/);
DEFINE_CALLBACK_TYPE4(TInformationEvent, void,
  TTerminal * /*Terminal*/, const UnicodeString & /*Str*/, bool /*Status*/,
  intptr_t /*Phase*/);
DEFINE_CALLBACK_TYPE5(TCreateLocalFileEvent, HANDLE,
  const UnicodeString & /*FileName*/, DWORD /*DesiredAccess*/,
  DWORD /*ShareMode*/, DWORD /*CreationDisposition*/, DWORD /*FlagsAndAttributes*/);
DEFINE_CALLBACK_TYPE1(TGetLocalFileAttributesEvent, DWORD,
  const UnicodeString & /*FileName*/);
DEFINE_CALLBACK_TYPE2(TSetLocalFileAttributesEvent, BOOL,
  const UnicodeString & /*FileName*/, DWORD /*FileAttributes*/);
DEFINE_CALLBACK_TYPE3(TMoveLocalFileEvent, BOOL,
  const UnicodeString & /*FileName*/, const UnicodeString & /*NewFileName*/,
  DWORD /*Flags*/);
DEFINE_CALLBACK_TYPE1(TRemoveLocalDirectoryEvent, BOOL,
  const UnicodeString & /*LocalDirName*/);
DEFINE_CALLBACK_TYPE2(TCreateLocalDirectoryEvent, BOOL,
  const UnicodeString & /*LocalDirName*/, LPSECURITY_ATTRIBUTES /*SecurityAttributes*/);
DEFINE_CALLBACK_TYPE0(TCheckForEscEvent, bool);

inline void ThrowSkipFile(Exception * Exception, const UnicodeString & Message)
{
  throw ESkipFile(Exception, Message);
}
inline void ThrowSkipFileNull() { ThrowSkipFile(nullptr, L""); }

void FileOperationLoopCustom(TTerminal * Terminal,
  TFileOperationProgressType * OperationProgress,
  bool AllowSkip, const UnicodeString & Message,
  const UnicodeString & HelpKeyword,
  const std::function<void()> & Operation);

enum TCurrentFSProtocol
{
  cfsUnknown,
  cfsSCP,
  cfsSFTP,
  cfsFTP,
  cfsFTPS,
  cfsWebDAV
};

const int cpDelete = 0x01;
const int cpTemporary = 0x04;
const int cpNoConfirmation = 0x08;
const int cpNewerOnly = 0x10;
const int cpAppend = 0x20;
const int cpResume = 0x40;

const int ccApplyToDirectories = 0x01;
const int ccRecursive = 0x02;
const int ccUser = 0x100;

const int csIgnoreErrors = 0x01;

const int ropNoReadDirectory = 0x02;

const int boDisableNeverShowAgain = 0x01;

class TTerminal : public TObject, public TSessionUI
{
NB_DISABLE_COPY(TTerminal)
NB_DECLARE_CLASS(TTerminal)
public:
  // TScript::SynchronizeProc relies on the order
  enum TSynchronizeMode
  {
    smRemote,
    smLocal,
    smBoth
  };
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
  static const int spDefault = spNoConfirmation | spPreviewChanges;

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
  UnicodeString GetCurrDirectory();
  bool GetExceptionOnFail() const;
  const TRemoteTokenList * GetGroups();
  const TRemoteTokenList * GetUsers();
  const TRemoteTokenList * GetMembership();
  void TerminalSetCurrentDirectory(const UnicodeString & Value);
  void SetExceptionOnFail(bool Value);
  void ReactOnCommand(intptr_t /*TFSCommand*/ Cmd);
  UnicodeString TerminalGetUserName() const;
  bool GetAreCachesEmpty() const;
  bool GetIsCapable(TFSCapability Capability) const;
  void ClearCachedFileList(const UnicodeString & APath, bool SubDirs);
  void AddCachedFileList(TRemoteFileList * FileList);
  bool GetCommandSessionOpened() const;
  TTerminal * GetCommandSession();
  bool GetResolvingSymlinks() const;
  bool GetActive() const;
  UnicodeString GetPassword() const;
  void SetRememberedPassword(const UnicodeString & Value) { FRememberedPassword = Value; }
  void SetRememberedTunnelPassword(const UnicodeString & Value) { FRememberedTunnelPassword = Value; }
  UnicodeString GetRememberedPassword() const;
  UnicodeString GetRememberedTunnelPassword() const;
  void SetTunnelPassword(const UnicodeString & Value) { FRememberedTunnelPassword = Value; }
  bool GetStoredCredentialsTried() const;
  TCustomFileSystem * GetFileSystem() const { return FFileSystem; }
  TCustomFileSystem * GetFileSystem() { return FFileSystem; }
  inline bool InTransaction();
  void SaveCapabilities(TFileSystemInfo & FileSystemInfo);
  static UnicodeString SynchronizeModeStr(TSynchronizeMode Mode);
  static UnicodeString SynchronizeParamsStr(intptr_t Params);

public:
  explicit TTerminal();
  void Init(TSessionData * SessionData, TConfiguration * Configuration);
  virtual ~TTerminal();
  void Open();
  void Close();
  void Reopen(intptr_t Params);
  virtual void DirectoryModified(const UnicodeString & APath, bool SubDirs);
  virtual void DirectoryLoaded(TRemoteFileList * FileList);
  void ShowExtendedException(Exception * E);
  void Idle();
  void RecryptPasswords();
  bool AllowedAnyCommand(const UnicodeString & Command) const;
  void AnyCommand(const UnicodeString & Command, TCaptureOutputEvent OutputEvent);
  void CloseOnCompletion(TOnceDoneOperation Operation = odoDisconnect, const UnicodeString & Message = L"");
  UnicodeString GetAbsolutePath(const UnicodeString & APath, bool Local) const;
  void BeginTransaction();
  void ReadCurrentDirectory();
  void ReadDirectory(bool ReloadOnly, bool ForceCache = false);
  TRemoteFileList * ReadDirectoryListing(const UnicodeString & Directory, const TFileMasks & Mask);
  TRemoteFileList * CustomReadDirectoryListing(const UnicodeString & Directory, bool UseCache);
  TRemoteFile * ReadFileListing(const UnicodeString & APath);
  void ReadFile(const UnicodeString & AFileName, TRemoteFile *& AFile);
  bool FileExists(const UnicodeString & AFileName, TRemoteFile ** AFile = nullptr);
  void ReadSymlink(TRemoteFile * SymlinkFile, TRemoteFile *& File);
  bool CopyToLocal(const TStrings * AFilesToCopy,
    const UnicodeString & TargetDir, const TCopyParamType * CopyParam, intptr_t Params);
  bool CopyToRemote(const TStrings * AFilesToCopy,
    const UnicodeString & TargetDir, const TCopyParamType * CopyParam, intptr_t Params);
  void RemoteCreateDirectory(const UnicodeString & ADirName,
    const TRemoteProperties * Properties = nullptr);
  void CreateLink(const UnicodeString & AFileName, const UnicodeString & PointTo, bool Symbolic);
  void RemoteDeleteFile(const UnicodeString & AFileName,
    const TRemoteFile * AFile = nullptr, void * Params = nullptr);
  bool DeleteFiles(TStrings * AFilesToDelete, intptr_t Params = 0);
  bool DeleteLocalFiles(TStrings * AFileList, intptr_t Params = 0);
  bool IsRecycledFile(const UnicodeString & AFileName);
  void CustomCommandOnFile(const UnicodeString & AFileName,
    const TRemoteFile * AFile, void * AParams);
  void CustomCommandOnFiles(const UnicodeString & Command, intptr_t Params,
    TStrings * AFiles, TCaptureOutputEvent OutputEvent);
  void ChangeDirectory(const UnicodeString & Directory);
  void EndTransaction();
  void HomeDirectory();
  void ChangeFileProperties(const UnicodeString & AFileName,
    const TRemoteFile * AFile, /*const TRemoteProperties*/ void * Properties);
  void ChangeFilesProperties(TStrings * AFileList,
    const TRemoteProperties * Properties);
  bool LoadFilesProperties(TStrings * AFileList);
  void TerminalError(const UnicodeString & Msg, const UnicodeString & HelpKeyword = L"");
  void TerminalError(Exception * E, const UnicodeString & Msg, const UnicodeString & HelpKeyword = L"");
  void ReloadDirectory();
  void RefreshDirectory();
  void TerminalRenameFile(const UnicodeString & AFileName, const UnicodeString & NewName);
  void TerminalRenameFile(const TRemoteFile * AFile, const UnicodeString & NewName, bool CheckExistence);
  void TerminalMoveFile(const UnicodeString & AFileName, const TRemoteFile * AFile,
    /*const TMoveFileParams*/ void * Param);
  bool MoveFiles(TStrings * AFileList, const UnicodeString & Target,
    const UnicodeString & FileMask);
  void TerminalCopyFile(const UnicodeString & AFileName, const TRemoteFile * AFile,
    /*const TMoveFileParams*/ void * Param);
  bool CopyFiles(TStrings * AFileList, const UnicodeString & Target,
    const UnicodeString & FileMask);
  bool CalculateFilesSize(const TStrings * AFileList, int64_t & Size,
    intptr_t Params, const TCopyParamType * CopyParam,
    bool AllowDirs, TCalculateSizeStats * Stats = nullptr);
  void CalculateFilesChecksum(const UnicodeString & Alg, TStrings * AFileList,
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
  void SpaceAvailable(const UnicodeString & APath, TSpaceAvailable & ASpaceAvailable);
  bool DirectoryFileList(const UnicodeString & APath,
    TRemoteFileList *& FileList, bool CanLoad);
  void MakeLocalFileList(const UnicodeString & AFileName,
    const TSearchRec & Rec, void * Param);
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
  void GetSupportedChecksumAlgs(TStrings * Algs);

  static UnicodeString ExpandFileName(const UnicodeString & APath,
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
  TFileOperationFinishedEvent & GetOnFinished() { return FOnFinished; }
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
  void DoReadDirectoryProgress(intptr_t Progress, intptr_t ResolvedLinks, bool & Cancel);
  void DoReadDirectory(bool ReloadOnly);
  void DoCreateDirectory(const UnicodeString & ADirName);
  void DoDeleteFile(const UnicodeString & AFileName, const TRemoteFile * AFile,
    intptr_t Params);
  void DoCustomCommandOnFile(const UnicodeString & AFileName,
    const TRemoteFile * AFile, const UnicodeString & Command, intptr_t Params,
    TCaptureOutputEvent OutputEvent);
  void DoRenameFile(const UnicodeString & AFileName,
    const UnicodeString & NewName, bool Move);
  void DoCopyFile(const UnicodeString & AFileName, const UnicodeString & NewName);
  void DoChangeFileProperties(const UnicodeString & AFileName,
    const TRemoteFile * AFile, const TRemoteProperties * Properties);
  void DoChangeDirectory();
  void DoInitializeLog();
  void EnsureNonExistence(const UnicodeString & AFileName);
  void LookupUsersGroups();
  void FileModified(const TRemoteFile * AFile,
    const UnicodeString & AFileName, bool ClearDirectoryChange = false);
  intptr_t FileOperationLoop(TFileOperationEvent CallBackFunc,
    TFileOperationProgressType * OperationProgress, bool AllowSkip,
    const UnicodeString & Message, void * Param1 = nullptr, void * Param2 = nullptr);
  bool ProcessFiles(const TStrings * AFileList, TFileOperation Operation,
    TProcessFileEvent ProcessFile, void * Param = nullptr, TOperationSide Side = osRemote,
    bool Ex = false);
  void ProcessDirectory(const UnicodeString & ADirName,
    TProcessFileEvent CallBackFunc, void * Param = nullptr, bool UseCache = false,
    bool IgnoreErrors = false);
  void AnnounceFileListOperation();
  UnicodeString TranslateLockedPath(const UnicodeString & APath, bool Lock);
  void ReadDirectory(TRemoteFileList * AFileList);
  void CustomReadDirectory(TRemoteFileList * AFileList);
  void DoCreateLink(const UnicodeString & AFileName, const UnicodeString & PointTo,
    bool Symbolic);
  bool TerminalCreateFile(const UnicodeString & AFileName,
    TFileOperationProgressType * OperationProgress,
    bool Resume,
    bool NoConfirmation,
    OUT HANDLE * AHandle);
  void OpenLocalFile(const UnicodeString & AFileName, uintptr_t Access,
    OUT HANDLE * AHandle, OUT uintptr_t * AAttrs, OUT int64_t * ACTime, OUT int64_t * AMTime,
    OUT int64_t * AATime, OUT int64_t * ASize, bool TryWriteReadOnly = true);
  bool AllowLocalFileTransfer(const UnicodeString & AFileName,
    const TCopyParamType * CopyParam, TFileOperationProgressType * OperationProgress);
  bool HandleException(Exception * E);
  void CalculateFileSize(const UnicodeString & AFileName,
    const TRemoteFile * AFile, /*TCalculateSizeParams*/ void * Size);
  void DoCalculateDirectorySize(const UnicodeString & AFileName,
    const TRemoteFile * AFile, TCalculateSizeParams * Params);
  void CalculateLocalFileSize(const UnicodeString & AFileName,
    const TSearchRec & Rec, /*int64_t*/ void * Params);
  bool CalculateLocalFilesSize(const TStrings * AFileList,
    const TCopyParamType * CopyParam, bool AllowDirs,
    OUT int64_t & Size);
  TBatchOverwrite EffectiveBatchOverwrite(
    const UnicodeString & AFileName, const TCopyParamType * CopyParam, intptr_t Params, TFileOperationProgressType * OperationProgress,
    bool Special);
  bool CheckRemoteFile(
    const UnicodeString & AFileName, const TCopyParamType * CopyParam, intptr_t Params, TFileOperationProgressType * OperationProgress);
  uintptr_t ConfirmFileOverwrite(
    const UnicodeString & ASourceFullFileName, const UnicodeString & ATargetFileName,
    const TOverwriteFileParams * FileParams, uintptr_t Answers, TQueryParams * QueryParams,
    TOperationSide Side, const TCopyParamType * CopyParam, intptr_t Params,
    TFileOperationProgressType * OperationProgress, const UnicodeString & Message = L"");
  void DoSynchronizeCollectDirectory(const UnicodeString & ALocalDirectory,
    const UnicodeString & ARemoteDirectory, TSynchronizeMode Mode,
    const TCopyParamType * CopyParam, intptr_t Params,
    TSynchronizeDirectoryEvent OnSynchronizeDirectory,
    TSynchronizeOptions * Options, intptr_t Level, TSynchronizeChecklist * Checklist);
  void DoSynchronizeCollectFile(const UnicodeString & AFileName,
    const TRemoteFile * AFile, /*TSynchronizeData*/ void * Param);
  void SynchronizeCollectFile(const UnicodeString & AFileName,
    const TRemoteFile * AFile, /*TSynchronizeData*/ void * Param);
  void SynchronizeRemoteTimestamp(const UnicodeString & AFileName,
    const TRemoteFile * AFile, void * Param);
  void SynchronizeLocalTimestamp(const UnicodeString & AFileName,
    const TRemoteFile * AFile, void * Param);
  void DoSynchronizeProgress(const TSynchronizeData & Data, bool Collect);
  void DeleteLocalFile(const UnicodeString & AFileName,
    const TRemoteFile * AFile, void * Param);
  void RecycleFile(const UnicodeString & AFileName, const TRemoteFile * AFile);
  void DoStartup();
  virtual bool DoQueryReopen(Exception * E);
  virtual void FatalError(Exception * E, const UnicodeString & Msg, const UnicodeString & HelpKeyword = L"");
  void ResetConnection();
  virtual bool DoPromptUser(TSessionData * Data, TPromptKind Kind,
    const UnicodeString & Name, const UnicodeString & Instructions, TStrings * Prompts,
    TStrings * Response);
  void SetupTunnelLocalPortNumber();
  void OpenTunnel();
  void CloseTunnel();
  void DoInformation(const UnicodeString & Str, bool Status, intptr_t Phase = -1);
  bool PromptUser(TSessionData * Data, TPromptKind Kind,
    const UnicodeString & AName, const UnicodeString & Instructions, const UnicodeString & Prompt,
    bool Echo,
    intptr_t MaxLen,
    OUT UnicodeString & AResult);
  void FileFind(const UnicodeString & AFileName, const TRemoteFile * AFile, void * Param);
  void DoFilesFind(const UnicodeString & Directory, TFilesFindParams & Params);
  bool DoCreateLocalFile(const UnicodeString & AFileName,
    TFileOperationProgressType * OperationProgress,
    bool Resume,
    bool NoConfirmation,
    OUT HANDLE * AHandle);

  virtual void Information(const UnicodeString & Str, bool Status);
  virtual uintptr_t QueryUser(const UnicodeString & Query,
    TStrings * MoreMessages, uintptr_t Answers, const TQueryParams * Params,
    TQueryType QueryType = qtConfirmation);
  virtual uintptr_t QueryUserException(const UnicodeString & Query,
    Exception * E, uintptr_t Answers, const TQueryParams * Params,
    TQueryType QueryType = qtConfirmation);
  virtual bool PromptUser(TSessionData * Data, TPromptKind Kind,
    const UnicodeString & AName, const UnicodeString & Instructions, TStrings * Prompts,
    TStrings * Results);
  virtual void DisplayBanner(const UnicodeString & Banner);
  virtual void Closed();
  virtual void ProcessGUI();
  void Progress(TFileOperationProgressType * OperationProgress);
  virtual void HandleExtendedException(Exception * E);
  bool IsListenerFree(uintptr_t PortNumber) const;
  void DoProgress(TFileOperationProgressType & ProgressData);
  void DoFinished(TFileOperation Operation, TOperationSide Side, bool Temp,
    const UnicodeString & AFileName, bool Success, TOnceDoneOperation & OnceDoneOperation);
  void RollbackAction(TSessionAction & Action,
    TFileOperationProgressType * OperationProgress, Exception * E = nullptr);
  void DoAnyCommand(const UnicodeString & ACommand, TCaptureOutputEvent OutputEvent,
    TCallSessionAction * Action);
  TRemoteFileList * DoReadDirectoryListing(const UnicodeString & ADirectory,
    bool UseCache);
  RawByteString EncryptPassword(const UnicodeString & APassword) const;
  UnicodeString DecryptPassword(const RawByteString & APassword) const;
  void LogRemoteFile(TRemoteFile * AFile);
  UnicodeString FormatFileDetailsForLog(const UnicodeString & AFileName, const TDateTime & AModification, int64_t Size);
  void LogFileDetails(const UnicodeString & AFileName, const TDateTime & Modification, int64_t Size);
  void LogFileDone(TFileOperationProgressType * OperationProgress);
  virtual const TTerminal * GetPasswordSource() const { return this; }
  virtual TTerminal * GetPasswordSource() { return this; }
  void DoEndTransaction(bool Inform);
  bool VerifyCertificate(
    const UnicodeString & CertificateStorageKey, const UnicodeString & SiteKey,
    const UnicodeString & Fingerprint,
    const UnicodeString & CertificateSubject, int Failures);
  void CacheCertificate(const UnicodeString & CertificateStorageKey,
    const UnicodeString & SiteKey, const UnicodeString & Fingerprint, int Failures);
  void CollectTlsUsage(const UnicodeString & TlsVersionStr);

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

  void CommandErrorAri(
    Exception & E,
    const UnicodeString & Message,
    const std::function<void()> & Repeat);
  void CommandErrorAriAction(
    Exception & E,
    const UnicodeString & Message,
    const std::function<void()> & Repeat,
    TSessionAction & Action);

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
  int FOpening;
  RawByteString FRememberedPassword;
  RawByteString FRememberedTunnelPassword;
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
  int FNesting;
  TRemoteDirectory * FOldFiles;

/*
  void __fastcall CommandError(Exception * E, const UnicodeString Msg);
  unsigned int __fastcall CommandError(Exception * E, const UnicodeString Msg,
    unsigned int Answers, const UnicodeString HelpKeyword = L"");
  UnicodeString __fastcall GetCurrentDirectory();
  bool __fastcall GetExceptionOnFail() const;
  const TRemoteTokenList * __fastcall GetGroups();
  const TRemoteTokenList * __fastcall GetUsers();
  const TRemoteTokenList * __fastcall GetMembership();
  void __fastcall SetCurrentDirectory(UnicodeString value);
  void __fastcall SetExceptionOnFail(bool value);
  void __fastcall ReactOnCommand(int TFSCommand Cmd);
  UnicodeString __fastcall GetUserName() const;
  bool __fastcall GetAreCachesEmpty() const;
  void __fastcall ClearCachedFileList(const UnicodeString Path, bool SubDirs);
  void __fastcall AddCachedFileList(TRemoteFileList * FileList);
  bool __fastcall GetCommandSessionOpened();
  TTerminal * __fastcall GetCommandSession();
  bool __fastcall GetResolvingSymlinks();
  bool __fastcall GetActive();
  UnicodeString __fastcall GetPassword();
  UnicodeString __fastcall GetRememberedPassword();
  UnicodeString __fastcall GetRememberedTunnelPassword();
  bool __fastcall GetStoredCredentialsTried();
  inline bool __fastcall InTransaction();
  void __fastcall SaveCapabilities(TFileSystemInfo & FileSystemInfo);
  static UnicodeString __fastcall SynchronizeModeStr(TSynchronizeMode Mode);
  static UnicodeString __fastcall SynchronizeParamsStr(int Params);

protected:
  bool FReadCurrentDirectoryPending;
  bool FReadDirectoryPending;
  bool FTunnelOpening;

  void __fastcall DoStartReadDirectory();
  void __fastcall DoReadDirectoryProgress(int Progress, int ResolvedLinks, bool & Cancel);
  void __fastcall DoReadDirectory(bool ReloadOnly);
  void __fastcall DoCreateDirectory(const UnicodeString DirName);
  void __fastcall DoDeleteFile(const UnicodeString FileName, const TRemoteFile * File,
    int Params);
  void __fastcall DoCustomCommandOnFile(UnicodeString FileName,
    const TRemoteFile * File, UnicodeString Command, int Params, TCaptureOutputEvent OutputEvent);
  void __fastcall DoRenameFile(const UnicodeString FileName,
    const UnicodeString NewName, bool Move);
  void __fastcall DoCopyFile(const UnicodeString FileName, const UnicodeString NewName);
  void __fastcall DoChangeFileProperties(const UnicodeString FileName,
    const TRemoteFile * File, const TRemoteProperties * Properties);
  void __fastcall DoChangeDirectory();
  void __fastcall DoInitializeLog();
  void __fastcall EnsureNonExistence(const UnicodeString FileName);
  void __fastcall LookupUsersGroups();
  void __fastcall FileModified(const TRemoteFile * File,
    const UnicodeString FileName, bool ClearDirectoryChange = false);
  int __fastcall FileOperationLoop(TFileOperationEvent CallBackFunc,
    TFileOperationProgressType * OperationProgress, bool AllowSkip,
    const UnicodeString Message, void * Param1 = NULL, void * Param2 = NULL);
  bool __fastcall GetIsCapable(TFSCapability Capability) const;
  bool __fastcall ProcessFiles(TStrings * FileList, TFileOperation Operation,
    TProcessFileEvent ProcessFile, void * Param = NULL, TOperationSide Side = osRemote,
    bool Ex = false);
  bool __fastcall ProcessFilesEx(TStrings * FileList, TFileOperation Operation,
    TProcessFileEventEx ProcessFile, void * Param = NULL, TOperationSide Side = osRemote);
  void __fastcall ProcessDirectory(const UnicodeString DirName,
    TProcessFileEvent CallBackFunc, void * Param = NULL, bool UseCache = false,
    bool IgnoreErrors = false);
  void __fastcall AnnounceFileListOperation();
  UnicodeString __fastcall TranslateLockedPath(UnicodeString Path, bool Lock);
  void __fastcall ReadDirectory(TRemoteFileList * FileList);
  void __fastcall CustomReadDirectory(TRemoteFileList * FileList);
  void __fastcall DoCreateLink(const UnicodeString FileName, const UnicodeString PointTo, bool Symbolic);
  bool __fastcall CreateLocalFile(const UnicodeString FileName,
    TFileOperationProgressType * OperationProgress, HANDLE * AHandle,
    bool NoConfirmation);
  void __fastcall OpenLocalFile(const UnicodeString FileName, unsigned int Access,
    int * Attrs, HANDLE * Handle, __int64 * ACTime, __int64 * MTime,
    __int64 * ATime, __int64 * Size, bool TryWriteReadOnly = true);
  bool __fastcall AllowLocalFileTransfer(UnicodeString FileName,
    const TCopyParamType * CopyParam, TFileOperationProgressType * OperationProgress);
  bool __fastcall HandleException(Exception * E);
  void __fastcall CalculateFileSize(UnicodeString FileName,
    const TRemoteFile * File, TCalculateSizeParams void * Size);
  void __fastcall DoCalculateDirectorySize(const UnicodeString FileName,
    const TRemoteFile * File, TCalculateSizeParams * Params);
  void __fastcall CalculateLocalFileSize(const UnicodeString FileName,
    const TSearchRec Rec, __int64 void * Size);
  bool __fastcall CalculateLocalFilesSize(TStrings * FileList, __int64 & Size,
    const TCopyParamType * CopyParam, bool AllowDirs);
  TBatchOverwrite __fastcall EffectiveBatchOverwrite(
    const UnicodeString & ASourceFullFileName, const TCopyParamType * CopyParam, int Params,
    TFileOperationProgressType * OperationProgress, bool Special);
  bool __fastcall CheckRemoteFile(
    const UnicodeString & FileName, const TCopyParamType * CopyParam,
    int Params, TFileOperationProgressType * OperationProgress);
  unsigned int __fastcall ConfirmFileOverwrite(
    const UnicodeString & ASourceFullFileName, const UnicodeString & ATargetFileName,
    const TOverwriteFileParams * FileParams, unsigned int Answers, TQueryParams * QueryParams,
    TOperationSide Side, const TCopyParamType * CopyParam, int Params,
    TFileOperationProgressType * OperationProgress, UnicodeString Message = L"");
  void __fastcall DoSynchronizeCollectDirectory(const UnicodeString LocalDirectory,
    const UnicodeString RemoteDirectory, TSynchronizeMode Mode,
    const TCopyParamType * CopyParam, int Params,
    TSynchronizeDirectory OnSynchronizeDirectory,
    TSynchronizeOptions * Options, int Level, TSynchronizeChecklist * Checklist);
  void __fastcall DoSynchronizeCollectFile(const UnicodeString FileName,
    const TRemoteFile * File, TSynchronizeData void * Param);
  void __fastcall SynchronizeCollectFile(const UnicodeString FileName,
    const TRemoteFile * File, TSynchronizeData void * Param);
  void __fastcall SynchronizeRemoteTimestamp(const UnicodeString FileName,
    const TRemoteFile * File, void * Param);
  void __fastcall SynchronizeLocalTimestamp(const UnicodeString FileName,
    const TRemoteFile * File, void * Param);
  void __fastcall DoSynchronizeProgress(const TSynchronizeData & Data, bool Collect);
  void __fastcall DeleteLocalFile(UnicodeString FileName,
    const TRemoteFile * File, void * Param);
  void __fastcall RecycleFile(UnicodeString FileName, const TRemoteFile * File);
  TStrings * __fastcall GetFixedPaths();
  void __fastcall DoStartup();
  virtual bool __fastcall DoQueryReopen(Exception * E);
  virtual void __fastcall FatalError(Exception * E, UnicodeString Msg, UnicodeString HelpKeyword = L"");
  void __fastcall ResetConnection();
  virtual bool __fastcall DoPromptUser(TSessionData * Data, TPromptKind Kind,
    UnicodeString Name, UnicodeString Instructions, TStrings * Prompts,
    TStrings * Response);
  void __fastcall OpenTunnel();
  void __fastcall CloseTunnel();
  void __fastcall DoInformation(const UnicodeString & Str, bool Status, int Phase = -1);
  bool __fastcall PromptUser(TSessionData * Data, TPromptKind Kind,
    UnicodeString Name, UnicodeString Instructions, UnicodeString Prompt, bool Echo,
    int MaxLen, UnicodeString & Result);
  void __fastcall FileFind(UnicodeString FileName, const TRemoteFile * File, void * Param);
  void __fastcall DoFilesFind(UnicodeString Directory, TFilesFindParams & Params);
  bool __fastcall DoCreateLocalFile(const UnicodeString FileName,
    TFileOperationProgressType * OperationProgress, HANDLE * AHandle,
    bool NoConfirmation);

  virtual void __fastcall Information(const UnicodeString & Str, bool Status);
  virtual unsigned int __fastcall QueryUser(const UnicodeString Query,
    TStrings * MoreMessages, unsigned int Answers, const TQueryParams * Params,
    TQueryType QueryType = qtConfirmation);
  virtual unsigned int __fastcall QueryUserException(const UnicodeString Query,
    Exception * E, unsigned int Answers, const TQueryParams * Params,
    TQueryType QueryType = qtConfirmation);
  virtual bool __fastcall PromptUser(TSessionData * Data, TPromptKind Kind,
    UnicodeString Name, UnicodeString Instructions, TStrings * Prompts, TStrings * Results);
  virtual void __fastcall DisplayBanner(const UnicodeString & Banner);
  virtual void __fastcall Closed();
  virtual void __fastcall ProcessGUI();
  void __fastcall Progress(TFileOperationProgressType * OperationProgress);
  virtual void __fastcall HandleExtendedException(Exception * E);
  bool __fastcall IsListenerFree(unsigned int PortNumber);
  void __fastcall DoProgress(TFileOperationProgressType & ProgressData);
  void __fastcall DoFinished(TFileOperation Operation, TOperationSide Side, bool Temp,
    const UnicodeString & FileName, bool Success, TOnceDoneOperation & OnceDoneOperation);
  void __fastcall RollbackAction(TSessionAction & Action,
    TFileOperationProgressType * OperationProgress, Exception * E = NULL);
  void __fastcall DoAnyCommand(const UnicodeString Command, TCaptureOutputEvent OutputEvent,
    TCallSessionAction * Action);
  TRemoteFileList * __fastcall DoReadDirectoryListing(UnicodeString Directory, bool UseCache);
  RawByteString __fastcall EncryptPassword(const UnicodeString & Password);
  UnicodeString __fastcall DecryptPassword(const RawByteString & Password);
  void __fastcall LogRemoteFile(TRemoteFile * File);
  UnicodeString __fastcall FormatFileDetailsForLog(const UnicodeString & FileName, TDateTime Modification, __int64 Size);
  void __fastcall LogFileDetails(const UnicodeString & FileName, TDateTime Modification, __int64 Size);
  void __fastcall LogFileDone(TFileOperationProgressType * OperationProgress);
  virtual TTerminal * __fastcall GetPasswordSource();
  void __fastcall DoEndTransaction(bool Inform);
  bool  __fastcall VerifyCertificate(
    const UnicodeString & CertificateStorageKey, const UnicodeString & SiteKey,
    const UnicodeString & Fingerprint,
    const UnicodeString & CertificateSubject, int Failures);
  void __fastcall CacheCertificate(const UnicodeString & CertificateStorageKey,
    const UnicodeString & SiteKey, const UnicodeString & Fingerprint, int Failures);
  void __fastcall CollectTlsUsage(const UnicodeString & TlsVersionStr);

  __property TFileOperationProgressType * OperationProgress = { read=FOperationProgress };

public:
  __fastcall TTerminal(TSessionData * SessionData, TConfiguration * Configuration);
  __fastcall ~TTerminal();
  void __fastcall Open();
  void __fastcall Close();
  void __fastcall Reopen(int Params);
  virtual void __fastcall DirectoryModified(const UnicodeString Path, bool SubDirs);
  virtual void __fastcall DirectoryLoaded(TRemoteFileList * FileList);
  void __fastcall ShowExtendedException(Exception * E);
  void __fastcall Idle();
  void __fastcall RecryptPasswords();
  bool __fastcall AllowedAnyCommand(const UnicodeString Command);
  void __fastcall AnyCommand(const UnicodeString Command, TCaptureOutputEvent OutputEvent);
  void __fastcall CloseOnCompletion(TOnceDoneOperation Operation = odoDisconnect, const UnicodeString Message = L"");
  UnicodeString __fastcall AbsolutePath(UnicodeString Path, bool Local);
  void __fastcall BeginTransaction();
  void __fastcall ReadCurrentDirectory();
  void __fastcall ReadDirectory(bool ReloadOnly, bool ForceCache = false);
  TRemoteFileList * __fastcall ReadDirectoryListing(UnicodeString Directory, const TFileMasks & Mask);
  TRemoteFileList * __fastcall CustomReadDirectoryListing(UnicodeString Directory, bool UseCache);
  TRemoteFile * __fastcall ReadFileListing(UnicodeString Path);
  void __fastcall ReadFile(const UnicodeString FileName, TRemoteFile *& File);
  bool __fastcall FileExists(const UnicodeString FileName, TRemoteFile ** File = NULL);
  void __fastcall ReadSymlink(TRemoteFile * SymlinkFile, TRemoteFile *& File);
  bool __fastcall CopyToLocal(TStrings * FilesToCopy,
    const UnicodeString TargetDir, const TCopyParamType * CopyParam, int Params);
  bool __fastcall CopyToRemote(TStrings * FilesToCopy,
    const UnicodeString TargetDir, const TCopyParamType * CopyParam, int Params);
  void __fastcall CreateDirectory(const UnicodeString DirName,
    const TRemoteProperties * Properties = NULL);
  void __fastcall CreateLink(const UnicodeString FileName, const UnicodeString PointTo, bool Symbolic);
  void __fastcall DeleteFile(UnicodeString FileName,
    const TRemoteFile * File = NULL, void * Params = NULL);
  bool __fastcall DeleteFiles(TStrings * FilesToDelete, int Params = 0);
  bool __fastcall DeleteLocalFiles(TStrings * FileList, int Params = 0);
  bool __fastcall IsRecycledFile(UnicodeString FileName);
  void __fastcall CustomCommandOnFile(UnicodeString FileName,
    const TRemoteFile * File, void * AParams);
  void __fastcall CustomCommandOnFiles(UnicodeString Command, int Params,
    TStrings * Files, TCaptureOutputEvent OutputEvent);
  void __fastcall ChangeDirectory(const UnicodeString Directory);
  void __fastcall EndTransaction();
  void __fastcall HomeDirectory();
  void __fastcall ChangeFileProperties(UnicodeString FileName,
    const TRemoteFile * File, const TRemoteProperties void * Properties);
  void __fastcall ChangeFilesProperties(TStrings * FileList,
    const TRemoteProperties * Properties);
  bool __fastcall LoadFilesProperties(TStrings * FileList);
  void __fastcall TerminalError(UnicodeString Msg);
  void __fastcall TerminalError(Exception * E, UnicodeString Msg, UnicodeString HelpKeyword = L"");
  void __fastcall ReloadDirectory();
  void __fastcall RefreshDirectory();
  void __fastcall RenameFile(const UnicodeString FileName, const UnicodeString NewName);
  void __fastcall RenameFile(const TRemoteFile * File, const UnicodeString NewName, bool CheckExistence);
  void __fastcall MoveFile(const UnicodeString FileName, const TRemoteFile * File,
    const TMoveFileParams void * Param);
  bool __fastcall MoveFiles(TStrings * FileList, const UnicodeString Target,
    const UnicodeString FileMask);
  void __fastcall CopyFile(const UnicodeString FileName, const TRemoteFile * File,
    const TMoveFileParams void * Param);
  bool __fastcall CopyFiles(TStrings * FileList, const UnicodeString Target,
    const UnicodeString FileMask);
  bool __fastcall CalculateFilesSize(TStrings * FileList, __int64 & Size,
    int Params, const TCopyParamType * CopyParam, bool AllowDirs,
    TCalculateSizeStats * Stats);
  void __fastcall CalculateFilesChecksum(const UnicodeString & Alg, TStrings * FileList,
    TStrings * Checksums, TCalculatedChecksumEvent OnCalculatedChecksum);
  void __fastcall ClearCaches();
  TSynchronizeChecklist * __fastcall SynchronizeCollect(const UnicodeString LocalDirectory,
    const UnicodeString RemoteDirectory, TSynchronizeMode Mode,
    const TCopyParamType * CopyParam, int Params,
    TSynchronizeDirectory OnSynchronizeDirectory, TSynchronizeOptions * Options);
  void __fastcall SynchronizeApply(TSynchronizeChecklist * Checklist,
    const UnicodeString LocalDirectory, const UnicodeString RemoteDirectory,
    const TCopyParamType * CopyParam, int Params,
    TSynchronizeDirectory OnSynchronizeDirectory);
  void __fastcall FilesFind(UnicodeString Directory, const TFileMasks & FileMask,
    TFileFoundEvent OnFileFound, TFindingFileEvent OnFindingFile);
  void __fastcall SpaceAvailable(const UnicodeString Path, TSpaceAvailable & ASpaceAvailable);
  bool __fastcall DirectoryFileList(const UnicodeString Path,
    TRemoteFileList *& FileList, bool CanLoad);
  void __fastcall MakeLocalFileList(const UnicodeString FileName,
    const TSearchRec Rec, void * Param);
  bool __fastcall FileOperationLoopQuery(Exception & E,
    TFileOperationProgressType * OperationProgress, const UnicodeString Message,
    bool AllowSkip, UnicodeString SpecialRetry = L"", UnicodeString HelpKeyword = L"");
  TUsableCopyParamAttrs __fastcall UsableCopyParamAttrs(int Params);
  bool __fastcall QueryReopen(Exception * E, int Params,
    TFileOperationProgressType * OperationProgress);
  UnicodeString __fastcall PeekCurrentDirectory();
  void __fastcall FatalAbort();
  void __fastcall ReflectSettings();
  void __fastcall CollectUsage();

  const TSessionInfo & __fastcall GetSessionInfo();
  const TFileSystemInfo & __fastcall GetFileSystemInfo(bool Retrieve = false);
  void __fastcall inline LogEvent(const UnicodeString & Str);
  void __fastcall GetSupportedChecksumAlgs(TStrings * Algs);

  static UnicodeString __fastcall ExpandFileName(UnicodeString Path,
    const UnicodeString BasePath);

  __property TSessionData * SessionData = { read = FSessionData };
  __property TSessionLog * Log = { read = FLog };
  __property TActionLog * ActionLog = { read = FActionLog };
  __property TConfiguration * Configuration = { read = FConfiguration };
  __property bool Active = { read = GetActive };
  __property TSessionStatus Status = { read = FStatus };
  __property UnicodeString CurrentDirectory = { read = GetCurrentDirectory, write = SetCurrentDirectory };
  __property bool ExceptionOnFail = { read = GetExceptionOnFail, write = SetExceptionOnFail };
  __property TRemoteDirectory * Files = { read = FFiles };
  __property TNotifyEvent OnChangeDirectory = { read = FOnChangeDirectory, write = FOnChangeDirectory };
  __property TReadDirectoryEvent OnReadDirectory = { read = FOnReadDirectory, write = FOnReadDirectory };
  __property TNotifyEvent OnStartReadDirectory = { read = FOnStartReadDirectory, write = FOnStartReadDirectory };
  __property TReadDirectoryProgressEvent OnReadDirectoryProgress = { read = FOnReadDirectoryProgress, write = FOnReadDirectoryProgress };
  __property TDeleteLocalFileEvent OnDeleteLocalFile = { read = FOnDeleteLocalFile, write = FOnDeleteLocalFile };
  __property TNotifyEvent OnInitializeLog = { read = FOnInitializeLog, write = FOnInitializeLog };
  __property const TRemoteTokenList * Groups = { read = GetGroups };
  __property const TRemoteTokenList * Users = { read = GetUsers };
  __property const TRemoteTokenList * Membership = { read = GetMembership };
  __property TFileOperationProgressEvent OnProgress  = { read=FOnProgress, write=FOnProgress };
  __property TFileOperationFinished OnFinished  = { read=FOnFinished, write=FOnFinished };
  __property TCurrentFSProtocol FSProtocol = { read = FFSProtocol };
  __property bool UseBusyCursor = { read = FUseBusyCursor, write = FUseBusyCursor };
  __property UnicodeString UserName = { read=GetUserName };
  __property bool IsCapable[TFSCapability Capability] = { read = GetIsCapable };
  __property bool AreCachesEmpty = { read = GetAreCachesEmpty };
  __property bool CommandSessionOpened = { read = GetCommandSessionOpened };
  __property TTerminal * CommandSession = { read = GetCommandSession };
  __property bool AutoReadDirectory = { read = FAutoReadDirectory, write = FAutoReadDirectory };
  __property TStrings * FixedPaths = { read = GetFixedPaths };
  __property bool ResolvingSymlinks = { read = GetResolvingSymlinks };
  __property UnicodeString Password = { read = GetPassword };
  __property UnicodeString RememberedPassword = { read = GetRememberedPassword };
  __property UnicodeString RememberedTunnelPassword = { read = GetRememberedTunnelPassword };
  __property bool StoredCredentialsTried = { read = GetStoredCredentialsTried };
  __property TQueryUserEvent OnQueryUser = { read = FOnQueryUser, write = FOnQueryUser };
  __property TPromptUserEvent OnPromptUser = { read = FOnPromptUser, write = FOnPromptUser };
  __property TDisplayBannerEvent OnDisplayBanner = { read = FOnDisplayBanner, write = FOnDisplayBanner };
  __property TExtendedExceptionEvent OnShowExtendedException = { read = FOnShowExtendedException, write = FOnShowExtendedException };
  __property TInformationEvent OnInformation = { read = FOnInformation, write = FOnInformation };
  __property TNotifyEvent OnClose = { read = FOnClose, write = FOnClose };
  __property int TunnelLocalPortNumber = { read = FTunnelLocalPortNumber };
*/
};

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
  virtual void DirectoryModified(const UnicodeString & APath,
    bool SubDirs);
  virtual const TTerminal * GetPasswordSource() const { return FMainTerminal; }
  virtual TTerminal * GetPasswordSource() { return FMainTerminal; }

private:
  TTerminal * FMainTerminal;
};

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

struct TCustomCommandParams : public TObject
{
NB_DECLARE_CLASS(TCustomCommandParams)
public:
  UnicodeString Command;
  intptr_t Params;
  TCaptureOutputEvent OutputEvent;
};

struct TCalculateSizeStats : public TObject
{
  TCalculateSizeStats();

  intptr_t Files;
  intptr_t Directories;
  intptr_t SymLinks;
};

struct TCalculateSizeParams : public TObject
{
NB_DECLARE_CLASS(TCalculateSizeParams)
public:
  int64_t Size;
  intptr_t Params;
  const TCopyParamType * CopyParam;
  TCalculateSizeStats * Stats;
  bool AllowDirs;
  bool Result;
};

typedef rde::vector<TDateTime> TDateTimes;

struct TMakeLocalFileListParams : public TObject
{
NB_DECLARE_CLASS(TMakeLocalFileListParams)
public:
  TStrings * FileList;
  TDateTimes * FileTimes;
  bool IncludeDirs;
  bool Recursive;
};

struct TSynchronizeOptions : public TObject
{
NB_DISABLE_COPY(TSynchronizeOptions)
public:
  TSynchronizeOptions();
  ~TSynchronizeOptions();

  TStringList * Filter;

  bool FilterFind(const UnicodeString & AFileName);
  bool MatchesFilter(const UnicodeString & AFileName);
};

enum TChecklistAction
{
  saNone,
  saUploadNew,
  saDownloadNew,
  saUploadUpdate,
  saDownloadUpdate,
  saDeleteRemote,
  saDeleteLocal
};

class TChecklistItem : public TObject
{
friend class TTerminal;
NB_DECLARE_CLASS(TChecklistItem)
NB_DISABLE_COPY(TChecklistItem)
public:
  struct TFileInfo : public TObject
  {
    UnicodeString FileName;
    UnicodeString Directory;
    TDateTime Modification;
    TModificationFmt ModificationFmt;
    int64_t Size;
  };

  TChecklistAction Action;
  bool IsDirectory;
  TFileInfo Local;
  TFileInfo Remote;
  intptr_t ImageIndex;
  bool Checked;
  TRemoteFile * RemoteFile;

  const UnicodeString & GetFileName() const;

  ~TChecklistItem();

private:
  FILETIME FLocalLastWriteTime;

  TChecklistItem();
};

class TSynchronizeChecklist : public TObject
{
friend class TTerminal;
NB_DISABLE_COPY(TSynchronizeChecklist)
public:
  static const intptr_t ActionCount = saDeleteLocal;

  ~TSynchronizeChecklist();

  void Update(const TChecklistItem * Item, bool Check, TChecklistAction Action);

  static TChecklistAction Reverse(TChecklistAction Action);

/*
  __property int Count = { read = GetCount };
  __property const TItem * Item[int Index] = { read = GetItem };
*/
  intptr_t GetCount() const;
  const TChecklistItem * GetItem(intptr_t Index) const;

protected:
  TSynchronizeChecklist();

  void Sort();
  void Add(TChecklistItem * Item);

public:
  void SetMasks(const UnicodeString & Value);

private:
  TList FList;

  static intptr_t Compare(const void * Item1, const void * Item2);
};

struct TSpaceAvailable : public TObject
{
  TSpaceAvailable();

  int64_t BytesOnDevice;
  int64_t UnusedBytesOnDevice;
  int64_t BytesAvailableToUser;
  int64_t UnusedBytesAvailableToUser;
  uintptr_t BytesPerAllocationUnit;
};

class TRobustOperationLoop : public TObject
{
NB_DISABLE_COPY(TRobustOperationLoop)
public:
  TRobustOperationLoop(TTerminal * Terminal, TFileOperationProgressType * OperationProgress);
  bool TryReopen(Exception & E);
  bool ShouldRetry() const;
  bool Retry();

private:
  TTerminal * FTerminal;
  TFileOperationProgressType * FOperationProgress;
  bool FRetry;
};

UnicodeString GetSessionUrl(const TTerminal * Terminal, bool WithUserName = false);

