//---------------------------------------------------------------------------
#ifndef TerminalH
#define TerminalH

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
typedef void (TObject::*TQueryUserEvent)
  (TObject * Sender, const wstring Query, TStrings * MoreMessages, int Answers,
   const TQueryParams * Params, int & Answer, TQueryType QueryType, void * Arg);
typedef void (TObject::*TPromptUserEvent)
  (TTerminal * Terminal, TPromptKind Kind, wstring Name, wstring Instructions,
   TStrings * Prompts, TStrings * Results, bool & Result, void * Arg);
typedef void (TObject::*TDisplayBannerEvent)
  (TTerminal * Terminal, wstring SessionName, const wstring & Banner,
   bool & NeverShowAgain, int Options);
typedef void (TObject::*TExtendedExceptionEvent)
  (TTerminal * Terminal, Exception * E, void * Arg);
typedef void (TObject::*TReadDirectoryEvent)(System::TObject * Sender, Boolean ReloadOnly);
typedef void (TObject::*TReadDirectoryProgressEvent)(
  System::TObject* Sender, int Progress, bool & Cancel);
typedef void (TObject::*TProcessFileEvent)
  (const wstring FileName, const TRemoteFile * File, void * Param);
typedef void (TObject::*TProcessFileEventEx)
  (const wstring FileName, const TRemoteFile * File, void * Param, int Index);
typedef int (TObject::*TFileOperationEvent)
  (void * Param1, void * Param2);
typedef void (TObject::*TSynchronizeDirectory)
  (const wstring LocalDirectory, const wstring RemoteDirectory,
   bool & Continue, bool Collect);
typedef void (TObject::*TDeleteLocalFileEvent)(
  const wstring FileName, bool Alternative);
typedef int (TObject::*TDirectoryModifiedEvent)
  (TTerminal * Terminal, const wstring Directory, bool SubDirs);
typedef void (TObject::*TInformationEvent)
  (TTerminal * Terminal, const wstring & Str, bool Status, bool Active = true);
//---------------------------------------------------------------------------
#define SUSPEND_OPERATION(Command)                            \
  {                                                           \
    TSuspendFileOperationProgress Suspend(OperationProgress); \
    Command                                                   \
  }

#define THROW_SKIP_FILE(EXCEPTION, MESSAGE) \
  throw EScpSkipFile(EXCEPTION, MESSAGE)
#define THROW_SKIP_FILE_NULL THROW_SKIP_FILE(NULL, "")

/* TODO : Better user interface (query to user) */
#define FILE_OPERATION_LOOP_CUSTOM(TERMINAL, ALLOW_SKIP, MESSAGE, OPERATION) { \
  bool DoRepeat; \
  do { \
    DoRepeat = false; \
    try { \
      OPERATION;                                                            \
    }                                                                       \
    catch (EAbort & E)                                                      \
    {                                                                       \
      throw;                                                                \
    }                                                                       \
    catch (EScpSkipFile & E)                                                \
    {                                                                       \
      throw;                                                                \
    }                                                                       \
    catch (EFatal & E)                                                      \
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
enum TCurrentFSProtocol { cfsUnknown, cfsSCP, cfsSFTP, cfsFTP };
//---------------------------------------------------------------------------
const cpDelete = 0x01;
const cpTemporary = 0x04;
const cpNoConfirmation = 0x08;
const cpNewerOnly = 0x10;
const cpAppend = 0x20;
const cpResume = 0x40;
//---------------------------------------------------------------------------
const ccApplyToDirectories = 0x01;
const ccRecursive = 0x02;
const ccUser = 0x100;
//---------------------------------------------------------------------------
const csIgnoreErrors = 0x01;
//---------------------------------------------------------------------------
const ropNoReadDirectory = 0x02;
//---------------------------------------------------------------------------
const boDisableNeverShowAgain = 0x01;
//---------------------------------------------------------------------------
class TTerminal : public TObject, public TSessionUI
{
public:
  // TScript::SynchronizeProc relies on the order
  enum TSynchronizeMode { smRemote, smLocal, smBoth };
  static const spDelete = 0x01; // cannot be combined with spTimestamp
  static const spNoConfirmation = 0x02; // has no effect for spTimestamp
  static const spExistingOnly = 0x04; // is implicit for spTimestamp
  static const spNoRecurse = 0x08;
  static const spUseCache = 0x10; // cannot be combined with spTimestamp
  static const spDelayProgress = 0x20; // cannot be combined with spTimestamp
  static const spPreviewChanges = 0x40; // not used by core
  static const spSubDirs = 0x80; // cannot be combined with spTimestamp
  static const spTimestamp = 0x100;
  static const spNotByTime = 0x200; // cannot be combined with spTimestamp and smBoth
  static const spBySize = 0x400; // cannot be combined with smBoth, has opposite meaning for spTimestamp
  // 0x800 is reserved for GUI (spSelectedOnly)
  static const spMirror = 0x1000;

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
  wstring FCurrentDirectory;
  wstring FLockDirectory;
  Integer FExceptionOnFail;
  TRemoteDirectory * FFiles;
  int FInTransaction;
  bool FSuspendTransaction;
  TNotifyEvent FOnChangeDirectory;
  TReadDirectoryEvent FOnReadDirectory;
  TNotifyEvent FOnStartReadDirectory;
  TReadDirectoryProgressEvent FOnReadDirectoryProgress;
  TDeleteLocalFileEvent FOnDeleteLocalFile;
  TRemoteTokenList FMembership;
  TRemoteTokenList FGroups;
  TRemoteTokenList FUsers;
  bool FUsersGroupsLookedup;
  TFileOperationProgressEvent FOnProgress;
  TFileOperationFinished FOnFinished;
  TFileOperationProgressType * FOperationProgress;
  bool FUseBusyCursor;
  TRemoteDirectoryCache * FDirectoryCache;
  TRemoteDirectoryChangesCache * FDirectoryChangesCache;
  TCustomFileSystem * FFileSystem;
  TSecureShell * FSecureShell;
  wstring FLastDirectoryChange;
  TCurrentFSProtocol FFSProtocol;
  TTerminal * FCommandSession;
  bool FAutoReadDirectory;
  bool FReadingCurrentDirectory;
  bool * FClosedOnCompletion;
  TSessionStatus FStatus;
  wstring FPassword;
  wstring FTunnelPassword;
  TTunnelThread * FTunnelThread;
  TSecureShell * FTunnel;
  TSessionData * FTunnelData;
  TSessionLog * FTunnelLog;
  TTunnelUI * FTunnelUI;
  int FTunnelLocalPortNumber;
  wstring FTunnelError;
  TQueryUserEvent FOnQueryUser;
  TPromptUserEvent FOnPromptUser;
  TDisplayBannerEvent FOnDisplayBanner;
  TExtendedExceptionEvent FOnShowExtendedException;
  TInformationEvent FOnInformation;
  TNotifyEvent FOnClose;
  bool FAnyInformation;
  TCallbackGuard * FCallbackGuard;
  TFindingFileEvent FOnFindingFile;

  void CommandError(Exception * E, const wstring Msg);
  int CommandError(Exception * E, const wstring Msg, int Answers);
  wstring GetCurrentDirectory();
  bool GetExceptionOnFail() const;
  const TRemoteTokenList * GetGroups();
  const TRemoteTokenList * GetUsers();
  const TRemoteTokenList * GetMembership();
  void SetCurrentDirectory(wstring value);
  void SetExceptionOnFail(bool value);
  void ReactOnCommand(int /*TFSCommand*/ Cmd);
  wstring GetUserName() const;
  bool GetAreCachesEmpty() const;
  void ClearCachedFileList(const wstring Path, bool SubDirs);
  void AddCachedFileList(TRemoteFileList * FileList);
  bool GetCommandSessionOpened();
  TTerminal * GetCommandSession();
  bool GetResolvingSymlinks();
  bool GetActive();
  wstring GetPassword();
  wstring GetTunnelPassword();
  bool GetStoredCredentialsTried();
  inline bool InTransaction();

protected:
  bool FReadCurrentDirectoryPending;
  bool FReadDirectoryPending;
  bool FTunnelOpening;

  void DoStartReadDirectory();
  void DoReadDirectoryProgress(int Progress, bool & Cancel);
  void DoReadDirectory(bool ReloadOnly);
  void DoCreateDirectory(const wstring DirName);
  void DoDeleteFile(const wstring FileName, const TRemoteFile * File,
    int Params);
  void DoCustomCommandOnFile(wstring FileName,
    const TRemoteFile * File, wstring Command, int Params, TCaptureOutputEvent OutputEvent);
  void DoRenameFile(const wstring FileName,
    const wstring NewName, bool Move);
  void DoCopyFile(const wstring FileName, const wstring NewName);
  void DoChangeFileProperties(const wstring FileName,
    const TRemoteFile * File, const TRemoteProperties * Properties);
  void DoChangeDirectory();
  void EnsureNonExistence(const wstring FileName);
  void LookupUsersGroups();
  void FileModified(const TRemoteFile * File,
    const wstring FileName, bool ClearDirectoryChange = false);
  int FileOperationLoop(TFileOperationEvent CallBackFunc,
    TFileOperationProgressType * OperationProgress, bool AllowSkip,
    const wstring Message, void * Param1 = NULL, void * Param2 = NULL);
  bool GetIsCapable(TFSCapability Capability) const;
  bool ProcessFiles(TStrings * FileList, TFileOperation Operation,
    TProcessFileEvent ProcessFile, void * Param = NULL, TOperationSide Side = osRemote,
    bool Ex = false);
  bool ProcessFilesEx(TStrings * FileList, TFileOperation Operation,
    TProcessFileEventEx ProcessFile, void * Param = NULL, TOperationSide Side = osRemote);
  void ProcessDirectory(const wstring DirName,
    TProcessFileEvent CallBackFunc, void * Param = NULL, bool UseCache = false,
    bool IgnoreErrors = false);
  void AnnounceFileListOperation();
  wstring TranslateLockedPath(wstring Path, bool Lock);
  void ReadDirectory(TRemoteFileList * FileList);
  void CustomReadDirectory(TRemoteFileList * FileList);
  void DoCreateLink(const wstring FileName, const wstring PointTo, bool Symbolic);
  bool CreateLocalFile(const wstring FileName,
    TFileOperationProgressType * OperationProgress, HANDLE * AHandle,
    bool NoConfirmation);
  void OpenLocalFile(const wstring FileName, int Access,
    int * Attrs, HANDLE * Handle, __int64 * ACTime, __int64 * MTime,
    __int64 * ATime, __int64 * Size, bool TryWriteReadOnly = true);
  bool AllowLocalFileTransfer(wstring FileName, const TCopyParamType * CopyParam);
  bool HandleException(Exception * E);
  void CalculateFileSize(wstring FileName,
    const TRemoteFile * File, /*TCalculateSizeParams*/ void * Size);
  void DoCalculateDirectorySize(const wstring FileName,
    const TRemoteFile * File, TCalculateSizeParams * Params);
  void CalculateLocalFileSize(const wstring FileName,
    const TSearchRec Rec, /*__int64*/ void * Size);
  void CalculateLocalFilesSize(TStrings * FileList, __int64 & Size,
    const TCopyParamType * CopyParam = NULL);
  TBatchOverwrite EffectiveBatchOverwrite(
    int Params, TFileOperationProgressType * OperationProgress, bool Special);
  bool CheckRemoteFile(int Params, TFileOperationProgressType * OperationProgress);
  int ConfirmFileOverwrite(const wstring FileName,
    const TOverwriteFileParams * FileParams, int Answers, const TQueryParams * QueryParams,
    TOperationSide Side, int Params, TFileOperationProgressType * OperationProgress,
    wstring Message = "");
  void DoSynchronizeCollectDirectory(const wstring LocalDirectory,
    const wstring RemoteDirectory, TSynchronizeMode Mode,
    const TCopyParamType * CopyParam, int Params,
    TSynchronizeDirectory OnSynchronizeDirectory,
    TSynchronizeOptions * Options, int Level, TSynchronizeChecklist * Checklist);
  void SynchronizeCollectFile(const wstring FileName,
    const TRemoteFile * File, /*TSynchronizeData*/ void * Param);
  void SynchronizeRemoteTimestamp(const wstring FileName,
    const TRemoteFile * File, void * Param);
  void SynchronizeLocalTimestamp(const wstring FileName,
    const TRemoteFile * File, void * Param);
  void DoSynchronizeProgress(const TSynchronizeData & Data, bool Collect);
  void DeleteLocalFile(wstring FileName,
    const TRemoteFile * File, void * Param);
  void RecycleFile(wstring FileName, const TRemoteFile * File);
  TStrings * GetFixedPaths();
  void DoStartup();
  virtual bool DoQueryReopen(Exception * E);
  virtual void FatalError(Exception * E, wstring Msg);
  void ResetConnection();
  virtual bool DoPromptUser(TSessionData * Data, TPromptKind Kind,
    wstring Name, wstring Instructions, TStrings * Prompts,
    TStrings * Response);
  void OpenTunnel();
  void CloseTunnel();
  void DoInformation(const wstring & Str, bool Status, bool Active = true);
  wstring FileUrl(const wstring Protocol, const wstring FileName);
  bool PromptUser(TSessionData * Data, TPromptKind Kind,
    wstring Name, wstring Instructions, wstring Prompt, bool Echo,
    int MaxLen, wstring & Result);
  void FileFind(wstring FileName, const TRemoteFile * File, void * Param);
  void DoFilesFind(wstring Directory, TFilesFindParams & Params);
  bool DoCreateLocalFile(const wstring FileName,
    TFileOperationProgressType * OperationProgress, HANDLE * AHandle,
    bool NoConfirmation);

  virtual void Information(const wstring & Str, bool Status);
  virtual int QueryUser(const wstring Query,
    TStrings * MoreMessages, int Answers, const TQueryParams * Params,
    TQueryType QueryType = qtConfirmation);
  virtual int QueryUserException(const wstring Query,
    Exception * E, int Answers, const TQueryParams * Params,
    TQueryType QueryType = qtConfirmation);
  virtual bool PromptUser(TSessionData * Data, TPromptKind Kind,
    wstring Name, wstring Instructions, TStrings * Prompts, TStrings * Results);
  virtual void DisplayBanner(const wstring & Banner);
  virtual void Closed();
  virtual void HandleExtendedException(Exception * E);
  bool IsListenerFree(unsigned int PortNumber);
  void DoProgress(TFileOperationProgressType & ProgressData, TCancelStatus & Cancel);
  void DoFinished(TFileOperation Operation, TOperationSide Side, bool Temp,
    const wstring & FileName, bool Success, TOnceDoneOperation & OnceDoneOperation);
  void RollbackAction(TSessionAction & Action,
    TFileOperationProgressType * OperationProgress, Exception * E = NULL);
  void DoAnyCommand(const wstring Command, TCaptureOutputEvent OutputEvent,
    TCallSessionAction * Action);
  TRemoteFileList * DoReadDirectoryListing(wstring Directory, bool UseCache);
  wstring EncryptPassword(const wstring & Password);
  wstring DecryptPassword(const wstring & Password);

  __property TFileOperationProgressType * OperationProgress = { read=FOperationProgress };

public:
  TTerminal(TSessionData * SessionData, TConfiguration * Configuration);
  ~TTerminal();
  void Open();
  void Close();
  void Reopen(int Params);
  virtual void DirectoryModified(const wstring Path, bool SubDirs);
  virtual void DirectoryLoaded(TRemoteFileList * FileList);
  void ShowExtendedException(Exception * E);
  void Idle();
  void RecryptPasswords();
  bool AllowedAnyCommand(const wstring Command);
  void AnyCommand(const wstring Command, TCaptureOutputEvent OutputEvent);
  void CloseOnCompletion(TOnceDoneOperation Operation = odoDisconnect, const wstring Message = "");
  wstring AbsolutePath(wstring Path, bool Local);
  void BeginTransaction();
  void ReadCurrentDirectory();
  void ReadDirectory(bool ReloadOnly, bool ForceCache = false);
  TRemoteFileList * ReadDirectoryListing(wstring Directory, const TFileMasks & Mask);
  TRemoteFileList * CustomReadDirectoryListing(wstring Directory, bool UseCache);
  void ReadFile(const wstring FileName, TRemoteFile *& File);
  bool FileExists(const wstring FileName, TRemoteFile ** File = NULL);
  void ReadSymlink(TRemoteFile * SymlinkFile, TRemoteFile *& File);
  bool CopyToLocal(TStrings * FilesToCopy,
    const wstring TargetDir, const TCopyParamType * CopyParam, int Params);
  bool CopyToRemote(TStrings * FilesToCopy,
    const wstring TargetDir, const TCopyParamType * CopyParam, int Params);
  void CreateDirectory(const wstring DirName,
    const TRemoteProperties * Properties = NULL);
  void CreateLink(const wstring FileName, const wstring PointTo, bool Symbolic);
  void DeleteFile(wstring FileName,
    const TRemoteFile * File = NULL, void * Params = NULL);
  bool DeleteFiles(TStrings * FilesToDelete, int Params = 0);
  bool DeleteLocalFiles(TStrings * FileList, int Params = 0);
  bool IsRecycledFile(wstring FileName);
  void CustomCommandOnFile(wstring FileName,
    const TRemoteFile * File, void * AParams);
  void CustomCommandOnFiles(wstring Command, int Params,
    TStrings * Files, TCaptureOutputEvent OutputEvent);
  void ChangeDirectory(const wstring Directory);
  void EndTransaction();
  void HomeDirectory();
  void ChangeFileProperties(wstring FileName,
    const TRemoteFile * File, /*const TRemoteProperties */ void * Properties);
  void ChangeFilesProperties(TStrings * FileList,
    const TRemoteProperties * Properties);
  bool LoadFilesProperties(TStrings * FileList);
  void TerminalError(wstring Msg);
  void TerminalError(Exception * E, wstring Msg);
  void ReloadDirectory();
  void RefreshDirectory();
  void RenameFile(const wstring FileName, const wstring NewName);
  void RenameFile(const TRemoteFile * File, const wstring NewName, bool CheckExistence);
  void MoveFile(const wstring FileName, const TRemoteFile * File,
    /*const TMoveFileParams*/ void * Param);
  bool MoveFiles(TStrings * FileList, const wstring Target,
    const wstring FileMask);
  void CopyFile(const wstring FileName, const TRemoteFile * File,
    /*const TMoveFileParams*/ void * Param);
  bool CopyFiles(TStrings * FileList, const wstring Target,
    const wstring FileMask);
  void CalculateFilesSize(TStrings * FileList, __int64 & Size,
    int Params, const TCopyParamType * CopyParam = NULL, TCalculateSizeStats * Stats = NULL);
  void CalculateFilesChecksum(const wstring & Alg, TStrings * FileList,
    TStrings * Checksums, TCalculatedChecksumEvent OnCalculatedChecksum);
  void ClearCaches();
  TSynchronizeChecklist * SynchronizeCollect(const wstring LocalDirectory,
    const wstring RemoteDirectory, TSynchronizeMode Mode,
    const TCopyParamType * CopyParam, int Params,
    TSynchronizeDirectory OnSynchronizeDirectory, TSynchronizeOptions * Options);
  void SynchronizeApply(TSynchronizeChecklist * Checklist,
    const wstring LocalDirectory, const wstring RemoteDirectory,
    const TCopyParamType * CopyParam, int Params,
    TSynchronizeDirectory OnSynchronizeDirectory);
  void FilesFind(wstring Directory, const TFileMasks & FileMask,
    TFileFoundEvent OnFileFound, TFindingFileEvent OnFindingFile);
  void SpaceAvailable(const wstring Path, TSpaceAvailable & ASpaceAvailable);
  bool DirectoryFileList(const wstring Path,
    TRemoteFileList *& FileList, bool CanLoad);
  void MakeLocalFileList(const wstring FileName,
    const TSearchRec Rec, void * Param);
  wstring FileUrl(const wstring FileName);
  bool FileOperationLoopQuery(Exception & E,
    TFileOperationProgressType * OperationProgress, const wstring Message,
    bool AllowSkip, wstring SpecialRetry = "");
  TUsableCopyParamAttrs UsableCopyParamAttrs(int Params);
  bool QueryReopen(Exception * E, int Params,
    TFileOperationProgressType * OperationProgress);
  wstring PeekCurrentDirectory();

  const TSessionInfo & GetSessionInfo();
  const TFileSystemInfo & GetFileSystemInfo(bool Retrieve = false);
  void inline LogEvent(const wstring & Str);

  static bool IsAbsolutePath(const wstring Path);
  static wstring ExpandFileName(wstring Path,
    const wstring BasePath);

  __property TSessionData * SessionData = { read = FSessionData };
  __property TSessionLog * Log = { read = FLog };
  __property TConfiguration * Configuration = { read = FConfiguration };
  __property bool Active = { read = GetActive };
  __property TSessionStatus Status = { read = FStatus };
  __property wstring CurrentDirectory = { read = GetCurrentDirectory, write = SetCurrentDirectory };
  __property bool ExceptionOnFail = { read = GetExceptionOnFail, write = SetExceptionOnFail };
  __property TRemoteDirectory * Files = { read = FFiles };
  __property TNotifyEvent OnChangeDirectory = { read = FOnChangeDirectory, write = FOnChangeDirectory };
  __property TReadDirectoryEvent OnReadDirectory = { read = FOnReadDirectory, write = FOnReadDirectory };
  __property TNotifyEvent OnStartReadDirectory = { read = FOnStartReadDirectory, write = FOnStartReadDirectory };
  __property TReadDirectoryProgressEvent OnReadDirectoryProgress = { read = FOnReadDirectoryProgress, write = FOnReadDirectoryProgress };
  __property TDeleteLocalFileEvent OnDeleteLocalFile = { read = FOnDeleteLocalFile, write = FOnDeleteLocalFile };
  __property const TRemoteTokenList * Groups = { read = GetGroups };
  __property const TRemoteTokenList * Users = { read = GetUsers };
  __property const TRemoteTokenList * Membership = { read = GetMembership };
  __property TFileOperationProgressEvent OnProgress  = { read=FOnProgress, write=FOnProgress };
  __property TFileOperationFinished OnFinished  = { read=FOnFinished, write=FOnFinished };
  __property TCurrentFSProtocol FSProtocol = { read = FFSProtocol };
  __property bool UseBusyCursor = { read = FUseBusyCursor, write = FUseBusyCursor };
  __property wstring UserName = { read=GetUserName };
  __property bool IsCapable[TFSCapability Capability] = { read = GetIsCapable };
  __property bool AreCachesEmpty = { read = GetAreCachesEmpty };
  __property bool CommandSessionOpened = { read = GetCommandSessionOpened };
  __property TTerminal * CommandSession = { read = GetCommandSession };
  __property bool AutoReadDirectory = { read = FAutoReadDirectory, write = FAutoReadDirectory };
  __property TStrings * FixedPaths = { read = GetFixedPaths };
  __property bool ResolvingSymlinks = { read = GetResolvingSymlinks };
  __property wstring Password = { read = GetPassword };
  __property wstring TunnelPassword = { read = GetTunnelPassword };
  __property bool StoredCredentialsTried = { read = GetStoredCredentialsTried };
  __property TQueryUserEvent OnQueryUser = { read = FOnQueryUser, write = FOnQueryUser };
  __property TPromptUserEvent OnPromptUser = { read = FOnPromptUser, write = FOnPromptUser };
  __property TDisplayBannerEvent OnDisplayBanner = { read = FOnDisplayBanner, write = FOnDisplayBanner };
  __property TExtendedExceptionEvent OnShowExtendedException = { read = FOnShowExtendedException, write = FOnShowExtendedException };
  __property TInformationEvent OnInformation = { read = FOnInformation, write = FOnInformation };
  __property TNotifyEvent OnClose = { read = FOnClose, write = FOnClose };
  __property int TunnelLocalPortNumber = { read = FTunnelLocalPortNumber };
};
//---------------------------------------------------------------------------
class TSecondaryTerminal : public TTerminal
{
public:
  TSecondaryTerminal(TTerminal * MainTerminal,
    TSessionData * SessionData, TConfiguration * Configuration,
    const wstring & Name);

protected:
  virtual void DirectoryLoaded(TRemoteFileList * FileList);
  virtual void DirectoryModified(const wstring Path,
    bool SubDirs);
  virtual bool DoPromptUser(TSessionData * Data, TPromptKind Kind,
    wstring Name, wstring Instructions, TStrings * Prompts, TStrings * Results);

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

  __property TTerminal * Terminals[int Index]  = { read=GetTerminal };
  __property int ActiveCount = { read = GetActiveCount };

protected:
  virtual TTerminal * CreateTerminal(TSessionData * Data);

private:
  TConfiguration * FConfiguration;

  TTerminal * GetTerminal(int Index);
  int GetActiveCount();
};
//---------------------------------------------------------------------------
struct TCustomCommandParams
{
  wstring Command;
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
      wstring FileName;
      wstring Directory;
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

    const wstring& GetFileName() const;

    ~TItem();

  private:
    FILETIME FLocalLastWriteTime;

    TItem();
  };

  ~TSynchronizeChecklist();

  __property int Count = { read = GetCount };
  __property const TItem * Item[int Index] = { read = GetItem };

protected:
  TSynchronizeChecklist();

  void Sort();
  void Add(TItem * Item);

  int GetCount() const;
  const TItem * GetItem(int Index) const;

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
