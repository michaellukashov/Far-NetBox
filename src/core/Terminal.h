//---------------------------------------------------------------------------
#ifndef TerminalH
#define TerminalH

#include <CoreDefs.hpp>
#include <Classes.hpp>

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
DEFINE_CALLBACK_TYPE8(TQueryUserEvent, void,
  TObject * /* Sender */, const UnicodeString & /* Query */, TStrings * /* MoreMessages */ , unsigned int /* Answers */,
  const TQueryParams * /* Params */, unsigned int & /* Answer */, TQueryType /* QueryType */, void * /* Arg */);
DEFINE_CALLBACK_TYPE8(TPromptUserEvent, void,
  TTerminal * /* Terminal */, TPromptKind /* Kind */, const UnicodeString & /* Name */, const UnicodeString & /* Instructions */,
  TStrings * /* Prompts */, TStrings * /* Results */, bool & /* Result */, void * /* Arg */);
DEFINE_CALLBACK_TYPE5(TDisplayBannerEvent, void,
  TTerminal * /* Terminal */, UnicodeString /* SessionName */, const UnicodeString & /* Banner */,
  bool & /* NeverShowAgain */, int /* Options */);
DEFINE_CALLBACK_TYPE3(TExtendedExceptionEvent, void,
  TTerminal * /* Terminal */, Exception * /* E */, void * /* Arg */);
DEFINE_CALLBACK_TYPE2(TReadDirectoryEvent, void, TObject * /* Sender */, Boolean /* ReloadOnly */);
DEFINE_CALLBACK_TYPE3(TReadDirectoryProgressEvent, void,
  TObject * /* Sender */, int /* Progress */, bool & /* Cancel */);
DEFINE_CALLBACK_TYPE3(TProcessFileEvent, void,
  const UnicodeString & /* FileName */, const TRemoteFile * /* File */, void * /* Param */);
DEFINE_CALLBACK_TYPE4(TProcessFileEventEx, void,
  const UnicodeString & /* FileName */, const TRemoteFile * /* File */, void * /* Param */, int /* Index */);
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
  TTerminal * /* Terminal */, const UnicodeString & /* Str */, bool /* Status */, int /* Phase */);
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
      TERMINAL->FileOperationLoopQuery(E, OperationProgress, MESSAGE, ALLOW_SKIP); \
      DoRepeat = true;                                                      \
    } \
  } while (DoRepeat); }

#define FILE_OPERATION_LOOP(MESSAGE, OPERATION) \
  FILE_OPERATION_LOOP_EX(True, MESSAGE, OPERATION)
//---------------------------------------------------------------------------
enum TCurrentFSProtocol { cfsUnknown, cfsSCP, cfsSFTP, cfsFTP, cfsFTPS, cfsWebDAV };
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
friend class TWebDAVFileSystem;
friend class TTunnelUI;
friend class TCallbackGuard;

private:
  TSessionData * FSessionData;
  TSessionLog * FLog;
  TActionLog * FActionLog;
  TConfiguration * FConfiguration;
  UnicodeString FCurrentDirectory;
  UnicodeString FLockDirectory;
  Integer FExceptionOnFail;
  TRemoteDirectory * FFiles;
  int FInTransaction;
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
  int FTunnelLocalPortNumber;
  UnicodeString FTunnelError;
  TQueryUserEvent FOnQueryUser;
  TPromptUserEvent FOnPromptUser;
  TDisplayBannerEvent FOnDisplayBanner;
  TExtendedExceptionEvent FOnShowExtendedException;
  TInformationEvent FOnInformation;
  TNotifyEvent FOnClose;
  TCallbackGuard * FCallbackGuard;
  TFindingFileEvent FOnFindingFile;

  void __fastcall CommandError(Exception * E, const UnicodeString & Msg);
  unsigned int __fastcall CommandError(Exception * E, const UnicodeString & Msg, unsigned int Answers);
  void __fastcall ReactOnCommand(int /*TFSCommand*/ Cmd);
  inline bool __fastcall InTransaction();

public:
  void __fastcall SetMasks(const UnicodeString & Value);
  UnicodeString __fastcall GetCurrentDirectory();
  bool __fastcall GetExceptionOnFail() const;
  const TRemoteTokenList * __fastcall GetGroups();
  const TRemoteTokenList * __fastcall GetUsers();
  const TRemoteTokenList * __fastcall GetMembership();
  void __fastcall SetCurrentDirectory(const UnicodeString & Value);
  void __fastcall SetExceptionOnFail(bool Value);
  UnicodeString __fastcall GetUserName() const;
  bool __fastcall GetAreCachesEmpty() const;
  bool __fastcall GetIsCapable(TFSCapability Capability) const;
  void __fastcall ClearCachedFileList(const UnicodeString & Path, bool SubDirs);
  void __fastcall AddCachedFileList(TRemoteFileList * FileList);
  bool __fastcall GetCommandSessionOpened();
  TTerminal * __fastcall GetCommandSession();
  bool __fastcall GetResolvingSymlinks();
  bool __fastcall GetActive();
  UnicodeString __fastcall GetPassword();
  UnicodeString __fastcall GetTunnelPassword();
  bool __fastcall GetStoredCredentialsTried();

protected:
  bool FReadCurrentDirectoryPending;
  bool FReadDirectoryPending;
  bool FTunnelOpening;

  void /* __fastcall */ DoStartReadDirectory();
  void /* __fastcall */ DoReadDirectoryProgress(int Progress, bool & Cancel);
  void /* __fastcall */ DoReadDirectory(bool ReloadOnly);
  void /* __fastcall */ DoCreateDirectory(const UnicodeString & DirName);
  void /* __fastcall */ DoDeleteFile(const UnicodeString & FileName, const TRemoteFile * File,
    int Params);
  void /* __fastcall */ DoCustomCommandOnFile(UnicodeString FileName,
    const TRemoteFile * File, UnicodeString Command, int Params, TCaptureOutputEvent OutputEvent);
  void /* __fastcall */ DoRenameFile(const UnicodeString & FileName,
    const UnicodeString & NewName, bool Move);
  void /* __fastcall */ DoCopyFile(const UnicodeString & FileName, const UnicodeString & NewName);
  void /* __fastcall */ DoChangeFileProperties(const UnicodeString & vFileName,
    const TRemoteFile * File, const TRemoteProperties * Properties);
  void /* __fastcall */ DoChangeDirectory();
  void /* __fastcall */ EnsureNonExistence(const UnicodeString & FileName);
  void /* __fastcall */ LookupUsersGroups();
  void /* __fastcall */ FileModified(const TRemoteFile * File,
    const UnicodeString & FileName, bool ClearDirectoryChange = false);
  int /* __fastcall */ FileOperationLoop(TFileOperationEvent CallBackFunc,
    TFileOperationProgressType * OperationProgress, bool AllowSkip,
    const UnicodeString & Message, void * Param1 = NULL, void * Param2 = NULL);
  bool /* __fastcall */ ProcessFiles(TStrings * FileList, TFileOperation Operation,
    TProcessFileEvent ProcessFile, void * Param = NULL, TOperationSide Side = osRemote,
    bool Ex = false);
  bool __fastcall ProcessFilesEx(TStrings * FileList, TFileOperation Operation,
    TProcessFileEventEx ProcessFile, void * Param = NULL, TOperationSide Side = osRemote);
  void /* __fastcall */ ProcessDirectory(const UnicodeString & DirName,
    TProcessFileEvent CallBackFunc, void * Param = NULL, bool UseCache = false,
    bool IgnoreErrors = false);
  void /* __fastcall */ AnnounceFileListOperation();
  UnicodeString /* __fastcall */ TranslateLockedPath(UnicodeString Path, bool Lock);
  void /* __fastcall */ ReadDirectory(TRemoteFileList * FileList);
  void /* __fastcall */ CustomReadDirectory(TRemoteFileList * FileList);
  void /* __fastcall */ DoCreateLink(const UnicodeString & FileName, const UnicodeString & PointTo, bool Symbolic);
  bool /* __fastcall */ CreateLocalFile(const UnicodeString & FileName,
    TFileOperationProgressType * OperationProgress, HANDLE * AHandle,
    bool NoConfirmation);
  void /* __fastcall */ OpenLocalFile(const UnicodeString & FileName, unsigned int Access,
    int * Attrs, HANDLE * Handle, __int64 * ACTime, __int64 * MTime,
    __int64 * ATime, __int64 * Size, bool TryWriteReadOnly = true);
  bool /* __fastcall */ AllowLocalFileTransfer(const UnicodeString & FileName, const TCopyParamType *CopyParam);
  bool /* __fastcall */ HandleException(Exception * E);
  void /* __fastcall */ CalculateFileSize(const UnicodeString & FileName,
    const TRemoteFile * File, /*TCalculateSizeParams*/ void * Size);
  void /* __fastcall */ DoCalculateDirectorySize(const UnicodeString & FileName,
    const TRemoteFile * File, TCalculateSizeParams * Params);
  void /* __fastcall */ CalculateLocalFileSize(const UnicodeString & FileName,
    const TSearchRec & Rec, /*__int64*/ void * Params);
  void /* __fastcall */ CalculateLocalFilesSize(TStrings * FileList, __int64 & Size,
    const TCopyParamType * CopyParam = NULL);
  TBatchOverwrite /* __fastcall */ EffectiveBatchOverwrite(
    int Params, TFileOperationProgressType * OperationProgress, bool Special);
  bool /* __fastcall */ CheckRemoteFile(int Params, TFileOperationProgressType * OperationProgress);
  unsigned int /* __fastcall */ ConfirmFileOverwrite(const UnicodeString & FileName,
    const TOverwriteFileParams * FileParams, unsigned int Answers, const TQueryParams * QueryParams,
    TOperationSide Side, int Params, TFileOperationProgressType * OperationProgress,
    UnicodeString Message = L"");
  void /* __fastcall */ DoSynchronizeCollectDirectory(const UnicodeString & LocalDirectory,
    const UnicodeString & RemoteDirectory, TSynchronizeMode Mode,
    const TCopyParamType * CopyParam, int Params,
    TSynchronizeDirectoryEvent OnSynchronizeDirectory,
    TSynchronizeOptions * Options, int Level, TSynchronizeChecklist * Checklist);
  void /* __fastcall */ SynchronizeCollectFile(const UnicodeString & FileName,
    const TRemoteFile * File, /*TSynchronizeData*/ void * Param);
  void /* __fastcall */ SynchronizeRemoteTimestamp(const UnicodeString & FileName,
    const TRemoteFile * File, void * Param);
  void /* __fastcall */ SynchronizeLocalTimestamp(const UnicodeString & FileName,
    const TRemoteFile * File, void * Param);
  void /* __fastcall */ DoSynchronizeProgress(const TSynchronizeData & Data, bool Collect);
  void /* __fastcall */ DeleteLocalFile(const UnicodeString & FileName,
    const TRemoteFile * File, void * Param);
  void /* __fastcall */ RecycleFile(const UnicodeString & FileName, const TRemoteFile * File);
  void /* __fastcall */ DoStartup();
  virtual bool __fastcall DoQueryReopen(Exception * E);
  virtual void __fastcall FatalError(Exception * E, const UnicodeString & Msg);
  void __fastcall ResetConnection();
  virtual bool __fastcall DoPromptUser(TSessionData * Data, TPromptKind Kind,
    const UnicodeString & Name, const UnicodeString & Instructions, TStrings * Prompts,
    TStrings * Response);
  void __fastcall OpenTunnel();
  void __fastcall CloseTunnel();
  void /* __fastcall */ DoInformation(const UnicodeString & Str, bool Status, int Phase = -1);
  UnicodeString __fastcall FileUrl(const UnicodeString & Protocol, const UnicodeString & FileName);
  bool __fastcall PromptUser(TSessionData * Data, TPromptKind Kind,
    const UnicodeString & Name, const UnicodeString & Instructions, const UnicodeString & Prompt, bool Echo,
    int MaxLen, UnicodeString & Result);
  void /* __fastcall */ FileFind(const UnicodeString & FileName, const TRemoteFile * File, void * Param);
  void /* __fastcall */ DoFilesFind(UnicodeString Directory, TFilesFindParams & Params);
  bool /* __fastcall */ DoCreateLocalFile(const UnicodeString & FileName,
    TFileOperationProgressType * OperationProgress, HANDLE * AHandle,
    bool NoConfirmation);

  virtual void __fastcall Information(const UnicodeString & Str, bool Status);
  virtual unsigned int __fastcall QueryUser(const UnicodeString & Query,
    TStrings * MoreMessages, unsigned int Answers, const TQueryParams * Params,
    TQueryType QueryType = qtConfirmation);
  virtual unsigned int __fastcall QueryUserException(const UnicodeString & Query,
    Exception * E, unsigned int Answers, const TQueryParams * Params,
    TQueryType QueryType = qtConfirmation);
  virtual bool __fastcall PromptUser(TSessionData * Data, TPromptKind Kind,
    const UnicodeString & Name, const UnicodeString & Instructions, TStrings * Prompts, TStrings * Results);
  virtual void __fastcall DisplayBanner(const UnicodeString & Banner);
  virtual void __fastcall Closed();
  virtual void __fastcall HandleExtendedException(Exception * E);
  bool __fastcall IsListenerFree(unsigned int PortNumber);
  void /* __fastcall */ DoProgress(TFileOperationProgressType & ProgressData, TCancelStatus & Cancel);
  void /* __fastcall */ DoFinished(TFileOperation Operation, TOperationSide Side, bool Temp,
    const UnicodeString & FileName, bool Success, TOnceDoneOperation & OnceDoneOperation);
  void __fastcall RollbackAction(TSessionAction & Action,
    TFileOperationProgressType * OperationProgress, Exception * E = NULL);
  void /* __fastcall */ DoAnyCommand(const UnicodeString & Command, TCaptureOutputEvent OutputEvent,
    TCallSessionAction * Action);
  TRemoteFileList * /* __fastcall */ DoReadDirectoryListing(UnicodeString Directory, bool UseCache);
  RawByteString __fastcall EncryptPassword(const UnicodeString & Password);
  UnicodeString __fastcall DecryptPassword(const RawByteString & Password);

  TFileOperationProgressType * __fastcall GetOperationProgress() { return FOperationProgress; }

  void __fastcall SetLocalFileTime(const UnicodeString & LocalFileName,
    const TDateTime & Modification);
  void __fastcall SetLocalFileTime(const UnicodeString & LocalFileName,
    FILETIME * AcTime, FILETIME * WrTime);
  HANDLE __fastcall CreateLocalFile(const UnicodeString & LocalFileName, DWORD DesiredAccess,
    DWORD ShareMode, DWORD CreationDisposition, DWORD FlagsAndAttributes);
  DWORD __fastcall GetLocalFileAttributes(const UnicodeString & LocalFileName);
  BOOL __fastcall SetLocalFileAttributes(const UnicodeString & LocalFileName, DWORD FileAttributes);
  BOOL __fastcall MoveLocalFile(const UnicodeString & LocalFileName, const UnicodeString & NewLocalFileName, DWORD Flags);
  BOOL __fastcall RemoveLocalDirectory(const UnicodeString & LocalDirName);
  BOOL __fastcall CreateLocalDirectory(const UnicodeString & LocalDirName, LPSECURITY_ATTRIBUTES SecurityAttributes);

public:
  explicit /* __fastcall */ TTerminal();
  void __fastcall Init(TSessionData * SessionData, TConfiguration * Configuration);
  virtual /* __fastcall */ ~TTerminal();
  void __fastcall Open();
  void __fastcall Close();
  void __fastcall Reopen(int Params);
  virtual void __fastcall DirectoryModified(const UnicodeString & Path, bool SubDirs);
  virtual void __fastcall DirectoryLoaded(TRemoteFileList * FileList);
  void __fastcall ShowExtendedException(Exception * E);
  void __fastcall Idle();
  void __fastcall RecryptPasswords();
  bool __fastcall AllowedAnyCommand(const UnicodeString & Command);
  void __fastcall AnyCommand(const UnicodeString & Command, TCaptureOutputEvent OutputEvent);
  void __fastcall CloseOnCompletion(TOnceDoneOperation Operation = odoDisconnect, const UnicodeString & Message = L"");
  UnicodeString __fastcall AbsolutePath(const UnicodeString & Path, bool Local);
  void __fastcall BeginTransaction();
  void __fastcall ReadCurrentDirectory();
  void __fastcall ReadDirectory(bool ReloadOnly, bool ForceCache = false);
  TRemoteFileList * __fastcall ReadDirectoryListing(const UnicodeString & Directory, const TFileMasks & Mask);
  TRemoteFileList * __fastcall CustomReadDirectoryListing(const UnicodeString & Directory, bool UseCache);
  TRemoteFile * __fastcall ReadFileListing(const UnicodeString & Path);
  void __fastcall ReadFile(const UnicodeString & FileName, TRemoteFile *& File);
  bool __fastcall FileExists(const UnicodeString & FileName, TRemoteFile ** File = NULL);
  void __fastcall ReadSymlink(TRemoteFile * SymlinkFile, TRemoteFile *& File);
  bool __fastcall CopyToLocal(TStrings * FilesToCopy,
    const UnicodeString & TargetDir, const TCopyParamType * CopyParam, int Params);
  bool __fastcall CopyToRemote(TStrings * FilesToCopy,
    const UnicodeString & TargetDir, const TCopyParamType * CopyParam, int Params);
  void __fastcall CreateDirectory(const UnicodeString & DirName,
    const TRemoteProperties * Properties = NULL);
  void __fastcall CreateLink(const UnicodeString & FileName, const UnicodeString & PointTo, bool Symbolic);
  void /* __fastcall */ DeleteFile(const UnicodeString & FileName,
    const TRemoteFile * File = NULL, void * Params = NULL);
  bool __fastcall DeleteFiles(TStrings * FilesToDelete, int Params = 0);
  bool __fastcall DeleteLocalFiles(TStrings * FileList, int Params = 0);
  bool __fastcall IsRecycledFile(const UnicodeString & FileName);
  void /* __fastcall */ CustomCommandOnFile(const UnicodeString & FileName,
    const TRemoteFile * File, void * AParams);
  void __fastcall CustomCommandOnFiles(UnicodeString Command, int Params,
    TStrings * Files, TCaptureOutputEvent OutputEvent);
  void __fastcall ChangeDirectory(const UnicodeString & Directory);
  void __fastcall EndTransaction();
  void __fastcall HomeDirectory();
  void /* __fastcall */ ChangeFileProperties(const UnicodeString & FileName,
    const TRemoteFile * File, /*const TRemoteProperties */ void * Properties);
  void __fastcall ChangeFilesProperties(TStrings * FileList,
    const TRemoteProperties * Properties);
  bool __fastcall LoadFilesProperties(TStrings * FileList);
  void __fastcall TerminalError(UnicodeString Msg);
  void __fastcall TerminalError(Exception * E, UnicodeString Msg);
  void __fastcall ReloadDirectory();
  void __fastcall RefreshDirectory();
  void __fastcall RenameFile(const UnicodeString & FileName, const UnicodeString & NewName);
  void __fastcall RenameFile(const TRemoteFile * File, const UnicodeString & NewName, bool CheckExistence);
  void /* __fastcall */ MoveFile(const UnicodeString & FileName, const TRemoteFile * File,
    /* const TMoveFileParams */ void * Param);
  bool __fastcall MoveFiles(TStrings * FileList, const UnicodeString & Target,
    const UnicodeString & FileMask);
  void /* __fastcall */ CopyFile(const UnicodeString & FileName, const TRemoteFile * File,
    /* const TMoveFileParams */ void * Param);
  bool __fastcall CopyFiles(TStrings * FileList, const UnicodeString & Target,
    const UnicodeString & FileMask);
  void __fastcall CalculateFilesSize(TStrings * FileList, __int64 & Size,
    int Params, const TCopyParamType * CopyParam = NULL, TCalculateSizeStats * Stats = NULL);
  void __fastcall CalculateFilesChecksum(const UnicodeString & Alg, TStrings * FileList,
    TStrings * Checksums, TCalculatedChecksumEvent OnCalculatedChecksum);
  void __fastcall ClearCaches();
  TSynchronizeChecklist * __fastcall SynchronizeCollect(const UnicodeString & LocalDirectory,
    const UnicodeString & RemoteDirectory, TSynchronizeMode Mode,
    const TCopyParamType * CopyParam, int Params,
    TSynchronizeDirectoryEvent OnSynchronizeDirectory, TSynchronizeOptions * Options);
  void __fastcall SynchronizeApply(TSynchronizeChecklist * Checklist,
    const UnicodeString & LocalDirectory, const UnicodeString & RemoteDirectory,
    const TCopyParamType * CopyParam, int Params,
    TSynchronizeDirectoryEvent OnSynchronizeDirectory);
  void __fastcall FilesFind(UnicodeString Directory, const TFileMasks & FileMask,
    TFileFoundEvent OnFileFound, TFindingFileEvent OnFindingFile);
  void __fastcall SpaceAvailable(const UnicodeString & Path, TSpaceAvailable & ASpaceAvailable);
  bool __fastcall DirectoryFileList(const UnicodeString & Path,
    TRemoteFileList *& FileList, bool CanLoad);
  void /* __fastcall */ MakeLocalFileList(const UnicodeString & FileName,
    const TSearchRec & Rec, void * Param);
  UnicodeString __fastcall FileUrl(const UnicodeString & FileName);
  bool __fastcall FileOperationLoopQuery(Exception & E,
    TFileOperationProgressType * OperationProgress, const UnicodeString & Message,
    bool AllowSkip, const UnicodeString & SpecialRetry = UnicodeString());
  TUsableCopyParamAttrs __fastcall UsableCopyParamAttrs(int Params);
  bool __fastcall QueryReopen(Exception * E, int Params,
    TFileOperationProgressType * OperationProgress);
  UnicodeString __fastcall PeekCurrentDirectory();
  void __fastcall FatalAbort();

  const TSessionInfo & __fastcall GetSessionInfo() const;
  const TFileSystemInfo & __fastcall GetFileSystemInfo(bool Retrieve = false);
  void __fastcall /* inline */ LogEvent(const UnicodeString & Str);

  static bool __fastcall IsAbsolutePath(const UnicodeString & Path);
  static UnicodeString __fastcall ExpandFileName(const UnicodeString & Path,
    const UnicodeString & BasePath);

  TSessionData * __fastcall GetSessionData() { return FSessionData; }
  TSessionData * __fastcall GetSessionData() const { return FSessionData; }
  TSessionLog * __fastcall GetLog() { return FLog; }
  TActionLog * __fastcall GetActionLog() { return FActionLog; };
  TConfiguration *__fastcall GetConfiguration() { return FConfiguration; }
  TSessionStatus __fastcall GetStatus() { return FStatus; }
  TRemoteDirectory * __fastcall GetFiles() { return FFiles; }
  TNotifyEvent & __fastcall GetOnChangeDirectory() { return FOnChangeDirectory; }
  void __fastcall SetOnChangeDirectory(TNotifyEvent Value) { FOnChangeDirectory = Value; }
  TReadDirectoryEvent & __fastcall GetOnReadDirectory() { return FOnReadDirectory; }
  void __fastcall SetOnReadDirectory(TReadDirectoryEvent Value) { FOnReadDirectory = Value; }
  TNotifyEvent & __fastcall GetOnStartReadDirectory() { return FOnStartReadDirectory; }
  void __fastcall SetOnStartReadDirectory(TNotifyEvent Value) { FOnStartReadDirectory = Value; }
  TReadDirectoryProgressEvent & __fastcall GetOnReadDirectoryProgress() { return FOnReadDirectoryProgress; }
  void SetOnReadDirectoryProgress(TReadDirectoryProgressEvent Value) { FOnReadDirectoryProgress = Value; }
  TDeleteLocalFileEvent & __fastcall GetOnDeleteLocalFile() { return FOnDeleteLocalFile; }
  void __fastcall SetOnDeleteLocalFile(TDeleteLocalFileEvent Value) { FOnDeleteLocalFile = Value; }
  TCreateLocalFileEvent & __fastcall GetOnCreateLocalFile() { return FOnCreateLocalFile; }
  void __fastcall SetOnCreateLocalFile(TCreateLocalFileEvent Value) { FOnCreateLocalFile = Value; }
  TGetLocalFileAttributesEvent & __fastcall GetOnGetLocalFileAttributes() { return FOnGetLocalFileAttributes; }
  void __fastcall SetOnGetLocalFileAttributes(TGetLocalFileAttributesEvent Value) { FOnGetLocalFileAttributes = Value; }
  TSetLocalFileAttributesEvent & __fastcall GetOnSetLocalFileAttributes() { return FOnSetLocalFileAttributes; }
  void __fastcall SetOnSetLocalFileAttributes(TSetLocalFileAttributesEvent Value) { FOnSetLocalFileAttributes = Value; }
  TMoveLocalFileEvent & __fastcall GetOnMoveLocalFile() { return FOnMoveLocalFile; }
  void __fastcall SetOnMoveLocalFile(TMoveLocalFileEvent Value) { FOnMoveLocalFile = Value; }
  TRemoveLocalDirectoryEvent & __fastcall GetOnRemoveLocalDirectory() { return FOnRemoveLocalDirectory; }
  void __fastcall SetOnRemoveLocalDirectory(TRemoveLocalDirectoryEvent Value) { FOnRemoveLocalDirectory = Value; }
  TCreateLocalDirectoryEvent & __fastcall GetOnCreateLocalDirectory() { return FOnCreateLocalDirectory; }
  void __fastcall SetOnCreateLocalDirectory(TCreateLocalDirectoryEvent Value) { FOnCreateLocalDirectory = Value; }
  TFileOperationProgressEvent & __fastcall GetOnProgress() { return FOnProgress; }
  void __fastcall SetOnProgress(TFileOperationProgressEvent Value) { FOnProgress = Value; }
  TFileOperationFinishedEvent & __fastcall  GetOnFinished() { return FOnFinished; }
  void __fastcall SetOnFinished(TFileOperationFinishedEvent Value) { FOnFinished = Value; }
  TCurrentFSProtocol __fastcall  GetFSProtocol() { return FFSProtocol; }
  bool __fastcall GetUseBusyCursor() { return FUseBusyCursor; }
  void __fastcall SetUseBusyCursor(bool Value) { FUseBusyCursor = Value; }
  bool __fastcall GetAutoReadDirectory() { return FAutoReadDirectory; }
  void __fastcall SetAutoReadDirectory(bool Value) { FAutoReadDirectory = Value; }
  TStrings * __fastcall GetFixedPaths();
  TQueryUserEvent & __fastcall GetOnQueryUser() { return FOnQueryUser; }
  void __fastcall SetOnQueryUser(TQueryUserEvent Value) { FOnQueryUser = Value; }
  TPromptUserEvent & __fastcall GetOnPromptUser() { return FOnPromptUser; }
  void __fastcall SetOnPromptUser(TPromptUserEvent Value) { FOnPromptUser = Value; }
  TDisplayBannerEvent & __fastcall GetOnDisplayBanner() { return FOnDisplayBanner; }
  void __fastcall SetOnDisplayBanner(TDisplayBannerEvent Value) { FOnDisplayBanner = Value; }
  TExtendedExceptionEvent & __fastcall GetOnShowExtendedException() { return FOnShowExtendedException; }
  void __fastcall SetOnShowExtendedException(TExtendedExceptionEvent Value) { FOnShowExtendedException = Value; }
  TInformationEvent & __fastcall GetOnInformation() { return FOnInformation; }
  void __fastcall SetOnInformation(TInformationEvent Value) { FOnInformation = Value; }
  TNotifyEvent & __fastcall GetOnClose() { return FOnClose; }
  void __fastcall SetOnClose(TNotifyEvent Value) { FOnClose = Value; }
  int __fastcall GetTunnelLocalPortNumber() { return FTunnelLocalPortNumber; }

private:
  TTerminal(const TTerminal &);
  TTerminal & operator = (const TTerminal &);
};
//---------------------------------------------------------------------------
class TSecondaryTerminal : public TTerminal
{
public:
  explicit /* __fastcall */ TSecondaryTerminal(TTerminal * MainTerminal);
  void __fastcall Init(TSessionData * SessionData, TConfiguration * Configuration,
    const UnicodeString & Name);
  virtual /* __fastcall */ ~TSecondaryTerminal() {}

  TTerminal * __fastcall GetMainTerminal() { return FMainTerminal; }

protected:
  virtual void __fastcall DirectoryLoaded(TRemoteFileList * FileList);
  virtual void __fastcall DirectoryModified(const UnicodeString & Path,
    bool SubDirs);
  virtual bool __fastcall DoPromptUser(TSessionData * Data, TPromptKind Kind,
    const UnicodeString & Name, const UnicodeString & Instructions, TStrings * Prompts, TStrings * Results);

private:
  bool FMasterPasswordTried;
  bool FMasterTunnelPasswordTried;
  TTerminal * FMainTerminal;
};
//---------------------------------------------------------------------------
class TTerminalList : public TObjectList
{
public:
  explicit /* __fastcall */ TTerminalList(TConfiguration * AConfiguration);
  virtual /* __fastcall */ ~TTerminalList();

  virtual TTerminal * __fastcall NewTerminal(TSessionData * Data);
  virtual void __fastcall FreeTerminal(TTerminal * Terminal);
  void __fastcall FreeAndNullTerminal(TTerminal * & Terminal);
  virtual void __fastcall Idle();
  void __fastcall RecryptPasswords();

  TTerminal * __fastcall GetTerminal(int Index);
  int __fastcall GetActiveCount();

protected:
  virtual TTerminal * __fastcall CreateTerminal(TSessionData * Data);

private:
  TConfiguration * FConfiguration;

public:
  void __fastcall SetMasks(const UnicodeString & Value);
};
//---------------------------------------------------------------------------
struct TCustomCommandParams
{
  UnicodeString Command;
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

  bool __fastcall FilterFind(const UnicodeString & FileName);
  bool __fastcall MatchesFilter(const UnicodeString & FileName);
};
//---------------------------------------------------------------------------
class TSynchronizeChecklist
{
friend class TTerminal;

public:
  enum TAction { saNone, saUploadNew, saDownloadNew, saUploadUpdate,
    saDownloadUpdate, saDeleteRemote, saDeleteLocal };
  static const int ActionCount = saDeleteLocal;

  class TItem : public TObject
  {
  friend class TTerminal;

  public:
    struct TFileInfo
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
    int ImageIndex;
    bool Checked;
    TRemoteFile * RemoteFile;

    const UnicodeString& GetFileName() const;

    ~TItem();

  private:
    FILETIME FLocalLastWriteTime;

    TItem();
  };

  ~TSynchronizeChecklist();

  intptr_t __fastcall GetCount() const;
  const TItem * __fastcall GetItem(intptr_t Index) const;

protected:
  TSynchronizeChecklist();

  void Sort();
  void Add(TItem * Item);

public:
  void __fastcall SetMasks(const UnicodeString & Value);

private:
  TList * FList;

  static int /* __fastcall */ Compare(const void * Item1, const void * Item2);
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
UnicodeString GetSessionUrl(const TTerminal * Terminal);
//---------------------------------------------------------------------------
#endif
