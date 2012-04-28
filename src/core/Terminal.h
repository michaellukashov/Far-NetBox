//---------------------------------------------------------------------------
#ifndef TerminalH
#define TerminalH

#ifndef _MSC_VER
#include <Classes.hpp>
#else
#include "boostdefines.hpp"
#include <boost/signals/signal5.hpp>
#include <boost/signals/signal8.hpp>

#include "Classes.h"
#endif

#include "SessionInfo.h"
#include "Interface.h"
#include "FileOperationProgress.h"
#include "FileMasks.h"
#include "Exceptions.h"
//---------------------------------------------------------------------------
class EFileNotFoundError : public Exception
{
public:
    EFileNotFoundError() : Exception(L"")
    {
    }
};

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
#ifndef _MSC_VER
typedef void __fastcall (__closure *TQueryUserEvent)
  (TObject * Sender, const UnicodeString Query, TStrings * MoreMessages, unsigned int Answers,
   const TQueryParams * Params, unsigned int & Answer, TQueryType QueryType, void * Arg);
typedef void __fastcall (__closure *TPromptUserEvent)
  (TTerminal * Terminal, TPromptKind Kind, UnicodeString Name, UnicodeString Instructions,
   TStrings * Prompts, TStrings * Results, bool & Result, void * Arg);
typedef void __fastcall (__closure *TDisplayBannerEvent)
  (TTerminal * Terminal, UnicodeString SessionName, const UnicodeString & Banner,
   bool & NeverShowAgain, int Options);
typedef void __fastcall (__closure *TExtendedExceptionEvent)
  (TTerminal * Terminal, Exception * E, void * Arg);
typedef void __fastcall (__closure *TReadDirectoryEvent)(System::TObject * Sender, Boolean ReloadOnly);
typedef void __fastcall (__closure *TReadDirectoryProgressEvent)(
  System::TObject* Sender, int Progress, bool & Cancel);
typedef void __fastcall (__closure *TProcessFileEvent)
  (const UnicodeString FileName, const TRemoteFile * File, void * Param);
typedef void __fastcall (__closure *TProcessFileEventEx)
  (const UnicodeString FileName, const TRemoteFile * File, void * Param, int Index);
typedef int __fastcall (__closure *TFileOperationEvent)
  (void * Param1, void * Param2);
typedef void __fastcall (__closure *TSynchronizeDirectory)
  (const UnicodeString LocalDirectory, const UnicodeString RemoteDirectory,
   bool & Continue, bool Collect);
typedef void __fastcall (__closure *TDeleteLocalFileEvent)(
  const UnicodeString FileName, bool Alternative);
typedef int __fastcall (__closure *TDirectoryModifiedEvent)
  (TTerminal * Terminal, const UnicodeString Directory, bool SubDirs);
typedef void __fastcall (__closure *TInformationEvent)
  (TTerminal * Terminal, const UnicodeString & Str, bool Status, int Phase);
#else
typedef boost::signal8<void, TObject * /* Sender */, const UnicodeString /* Query */, TStrings * /* MoreMessages */ , unsigned int /* Answers */,
  const TQueryParams * /* Params */, unsigned int & /* Answer */, TQueryType /* QueryType */, void * /* Arg */ > queryuser_signal_type;
typedef queryuser_signal_type::slot_type TQueryUserEvent;
typedef boost::signal8<void, TTerminal * /* Terminal */, TPromptKind /* Kind */, UnicodeString /* Name */, UnicodeString /* Instructions */,
   TStrings * /* Prompts */, TStrings * /* Results */, bool & /* Result */, void * /* Arg */> promptuser_signal_type;
typedef promptuser_signal_type::slot_type TPromptUserEvent;
typedef boost::signal5<void, TTerminal * /* Terminal */, UnicodeString /* SessionName */, const UnicodeString & /* Banner */,
   bool & /* NeverShowAgain */, int /* Options */> displaybanner_signal_type;
typedef displaybanner_signal_type::slot_type TDisplayBannerEvent;
typedef boost::signal3<void, TTerminal * /* Terminal */, Exception * /* E */, void * /* Arg */> extendedexception_signal_type;
typedef extendedexception_signal_type::slot_type TExtendedExceptionEvent;
typedef boost::signal2<void, TObject * /* Sender */, Boolean /* ReloadOnly */> readdirectory_signal_type;
typedef readdirectory_signal_type::slot_type TReadDirectoryEvent;
typedef boost::signal3<void, TObject* /* Sender */, int /* Progress */, bool & /* Cancel */> readdirectoryprogress_signal_type;
typedef readdirectoryprogress_signal_type::slot_type TReadDirectoryProgressEvent;
typedef boost::signal3<void, const UnicodeString /* FileName */, const TRemoteFile * /* File */, void * /* Param */> processfile_signal_type;
typedef processfile_signal_type::slot_type TProcessFileEvent;
typedef boost::signal4<void, const UnicodeString /* FileName */, const TRemoteFile * /* File */, void * /* Param */, int /* Index */> processfileex_signal_type;
typedef processfileex_signal_type::slot_type TProcessFileEventEx;
typedef boost::signal2<int, void * /* Param1 */, void * /* Param2 */> fileoperation_signal_type;
typedef fileoperation_signal_type::slot_type TFileOperationEvent;
typedef boost::signal4<void, const UnicodeString /* LocalDirectory */, const UnicodeString /* RemoteDirectory */,
   bool & /* Continue */, bool /* Collect */> synchronizedirectory_signal_type;
typedef synchronizedirectory_signal_type::slot_type TSynchronizeDirectory;
typedef boost::signal2<void, const UnicodeString /* FileName */, bool /* Alternative */> deletelocalfile_signal_type;
typedef deletelocalfile_signal_type::slot_type TDeleteLocalFileEvent;
typedef boost::signal3<int, TTerminal * /* Terminal */, const UnicodeString /* Directory */, bool /* SubDirs */> directorymodified_signal_type;
typedef directorymodified_signal_type::slot_type TDirectoryModifiedEvent;
typedef boost::signal4<void, TTerminal * /* Terminal */, const UnicodeString & /* Str */, bool /* Status */, int /* Phase */> informationevent_signal_type;
typedef informationevent_signal_type::slot_type TInformationEvent;
#endif
//---------------------------------------------------------------------------
#define SUSPEND_OPERATION(Command)                            \
  {                                                           \
    TSuspendFileOperationProgress Suspend(OperationProgress); \
    Command                                                   \
  }

#define THROW_SKIP_FILE(MESSAGE, EXCEPTION) \
  throw EScpSkipFile(MESSAGE, EXCEPTION)
#define THROW_SKIP_FILE_NULL THROW_SKIP_FILE(L"", NULL)

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
    catch (const EFileNotFoundError &)                                                      \
    {                                                                       \
      throw;                                                                \
    }                                                                       \
    catch (const EOSError &)                                                      \
    {                                                                       \
      throw;                                                                \
    }                                                                       \
    catch (const Exception & E)                                                   \
    {                                                                       \
      TERMINAL->FileOperationLoopQuery(E, OperationProgress, MESSAGE, ALLOW_SKIP); \
      DoRepeat = true;                                                      \
    } \
  } while (DoRepeat); }

#define FILE_OPERATION_LOOP(MESSAGE, OPERATION) \
  FILE_OPERATION_LOOP_EX(True, MESSAGE, OPERATION)
//---------------------------------------------------------------------------
enum TCurrentFSProtocol { cfsUnknown, cfsSCP, cfsSFTP, cfsFTP, cfsFTPS, cfsHTTP, cfsHTTPS };
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
  notify_signal_type FOnChangeDirectory;
  readdirectory_signal_type FOnReadDirectory;
  notify_signal_type FOnStartReadDirectory;
  readdirectoryprogress_signal_type FOnReadDirectoryProgress;
  deletelocalfile_signal_type FOnDeleteLocalFile;
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
  size_t FTunnelLocalPortNumber;
  UnicodeString FTunnelError;
  queryuser_signal_type FOnQueryUser;
  promptuser_signal_type FOnPromptUser;
  displaybanner_signal_type FOnDisplayBanner;
  extendedexception_signal_type FOnShowExtendedException;
  informationevent_signal_type FOnInformation;
  notify_signal_type FOnClose;
  bool FAnyInformation;
  TCallbackGuard * FCallbackGuard;
  findingfile_signal_type FOnFindingFile;
  TTerminal *Self;

  void CommandError(const Exception *E, const UnicodeString Msg);
  int CommandError(const Exception *E, const UnicodeString Msg, int Answers);
  void ReactOnCommand(int /*TFSCommand*/ Cmd);
  void ClearCachedFileList(const UnicodeString Path, bool SubDirs);
  void AddCachedFileList(TRemoteFileList *FileList);
  inline bool InTransaction();

protected:
  bool FReadCurrentDirectoryPending;
  bool FReadDirectoryPending;
  bool FTunnelOpening;

  void DoStartReadDirectory();
  void DoReadDirectoryProgress(size_t Progress, bool & Cancel);
  void DoReadDirectory(bool ReloadOnly);
  void DoCreateDirectory(const UnicodeString DirName);
  void DoDeleteFile(const UnicodeString FileName, const TRemoteFile * File,
    int Params);
  void DoCustomCommandOnFile(const UnicodeString FileName,
    const TRemoteFile * File, const UnicodeString Command, int Params, const TCaptureOutputEvent &OutputEvent);
  void DoRenameFile(const UnicodeString FileName,
    const UnicodeString NewName, bool Move);
  void DoCopyFile(const UnicodeString FileName, const UnicodeString NewName);
  void DoChangeFileProperties(const UnicodeString FileName,
    const TRemoteFile * File, const TRemoteProperties * Properties);
  void DoChangeDirectory();
  void EnsureNonExistence(const UnicodeString FileName);
  void LookupUsersGroups();
  void FileModified(const TRemoteFile * File,
    const UnicodeString FileName, bool ClearDirectoryChange = false);
  int FileOperationLoop(const TFileOperationEvent &CallBackFunc,
    TFileOperationProgressType * OperationProgress, bool AllowSkip,
    const UnicodeString Message, void * Param1 = NULL, void * Param2 = NULL);
#ifndef _MSC_VER
  bool __fastcall GetIsCapable(TFSCapability Capability) const;
#endif
  bool ProcessFiles(TStrings * FileList, TFileOperation Operation,
    const TProcessFileEvent &ProcessFile, void * Param = NULL, TOperationSide Side = osRemote,
    bool Ex = false);
  bool ProcessFilesEx(TStrings * FileList, TFileOperation Operation,
    TProcessFileEventEx ProcessFile, void * Param = NULL, TOperationSide Side = osRemote);
  void ProcessDirectory(const UnicodeString DirName,
    const TProcessFileEvent &CallBackFunc, void * Param = NULL, bool UseCache = false,
    bool IgnoreErrors = false);
  void AnnounceFileListOperation();
  UnicodeString TranslateLockedPath(UnicodeString Path, bool Lock);
  void ReadDirectory(TRemoteFileList * FileList);
  void CustomReadDirectory(TRemoteFileList * FileList);
  void DoCreateLink(const UnicodeString FileName, const UnicodeString PointTo, bool Symbolic);
  bool CreateLocalFile(const UnicodeString FileName,
    TFileOperationProgressType * OperationProgress, HANDLE * AHandle,
    bool NoConfirmation);
  void OpenLocalFile(const UnicodeString FileName, unsigned int Access,
    int * Attrs, HANDLE * Handle, __int64 * ACTime, __int64 * MTime,
    __int64 * ATime, __int64 * Size, bool TryWriteReadOnly = true);
  bool AllowLocalFileTransfer(const UnicodeString FileName, const TCopyParamType *CopyParam);
  bool HandleException(const Exception * E);
  void CalculateFileSize(const UnicodeString FileName,
    const TRemoteFile * File, /*TCalculateSizeParams*/ void * Size);
  void DoCalculateDirectorySize(const UnicodeString FileName,
    const TRemoteFile * File, TCalculateSizeParams * Params);
  void CalculateLocalFileSize(const UnicodeString FileName,
    const WIN32_FIND_DATA &Rec, /*__int64*/ void * Size);
  void CalculateLocalFilesSize(TStrings * FileList, __int64 & Size,
    const TCopyParamType * CopyParam = NULL);
  TBatchOverwrite EffectiveBatchOverwrite(
    int Params, TFileOperationProgressType * OperationProgress, bool Special);
  bool CheckRemoteFile(int Params, TFileOperationProgressType * OperationProgress);
  int ConfirmFileOverwrite(const UnicodeString FileName,
    const TOverwriteFileParams * FileParams, unsigned int Answers, const TQueryParams * QueryParams,
    TOperationSide Side, int Params, TFileOperationProgressType * OperationProgress,
    UnicodeString Message = L"");
  void DoSynchronizeCollectDirectory(const UnicodeString LocalDirectory,
    const UnicodeString RemoteDirectory, TSynchronizeMode Mode,
    const TCopyParamType * CopyParam, int Params,
    const TSynchronizeDirectory &OnSynchronizeDirectory,
    TSynchronizeOptions * Options, int Level, TSynchronizeChecklist * Checklist);
  void SynchronizeCollectFile(const UnicodeString FileName,
    const TRemoteFile * File, /*TSynchronizeData*/ void * Param);
  void SynchronizeRemoteTimestamp(const UnicodeString FileName,
    const TRemoteFile * File, void * Param);
  void SynchronizeLocalTimestamp(const UnicodeString FileName,
    const TRemoteFile * File, void * Param);
  void DoSynchronizeProgress(const TSynchronizeData & Data, bool Collect);
  void DeleteLocalFile(UnicodeString FileName,
    const TRemoteFile * File, void * Param);
  void RecycleFile(UnicodeString FileName, const TRemoteFile * File);
  TStrings * __fastcall GetFixedPaths();
  void DoStartup();
  virtual bool __fastcall DoQueryReopen(Exception * E);
  virtual void __fastcall FatalError(const Exception * E, const UnicodeString Msg);
  void __fastcall ResetConnection();
  virtual bool __fastcall DoPromptUser(TSessionData * Data, TPromptKind Kind,
    const UnicodeString Name, const UnicodeString Instructions, TStrings * Prompts,
    TStrings * Response);
  void __fastcall OpenTunnel();
  void __fastcall CloseTunnel();
  void DoInformation(const UnicodeString & Str, bool Status, int Phase = -1);
  UnicodeString FileUrl(const UnicodeString Protocol, const UnicodeString FileName);
  bool PromptUser(TSessionData * Data, TPromptKind Kind,
    const UnicodeString Name, const UnicodeString Instructions, const UnicodeString Prompt, bool Echo,
    size_t MaxLen, UnicodeString & Result);
  void FileFind(const UnicodeString FileName, const TRemoteFile * File, void * Param);
  void DoFilesFind(const UnicodeString Directory, TFilesFindParams & Params);
  bool DoCreateLocalFile(const UnicodeString FileName,
    TFileOperationProgressType * OperationProgress, HANDLE * AHandle,
    bool NoConfirmation);

  virtual void __fastcall Information(const UnicodeString & Str, bool Status);
  virtual unsigned int __fastcall QueryUser(const UnicodeString Query,
    TStrings * MoreMessages, unsigned int Answers, const TQueryParams * Params,
    TQueryType QueryType = qtConfirmation);
  virtual unsigned int __fastcall QueryUserException(const UnicodeString Query,
    const Exception * E, unsigned int Answers, const TQueryParams * Params,
    TQueryType QueryType = qtConfirmation);
  virtual bool __fastcall PromptUser(TSessionData * Data, TPromptKind Kind,
    const UnicodeString Name, const UnicodeString Instructions, TStrings * Prompts, TStrings * Results);
  virtual void __fastcall DisplayBanner(const UnicodeString & Banner);
  virtual void __fastcall Closed();
  virtual void __fastcall HandleExtendedException(const Exception * E);
  bool __fastcall IsListenerFree(size_t PortNumber);
  void __fastcall DoProgress(TFileOperationProgressType & ProgressData, TCancelStatus & Cancel);
  void DoFinished(TFileOperation Operation, TOperationSide Side, bool Temp,
    const UnicodeString & FileName, bool Success, TOnceDoneOperation & OnceDoneOperation);
  void __fastcall RollbackAction(TSessionAction & Action,
    TFileOperationProgressType * OperationProgress, Exception * E = NULL);
  void DoAnyCommand(const UnicodeString Command, const TCaptureOutputEvent &OutputEvent,
    TCallSessionAction * Action);
  TRemoteFileList *DoReadDirectoryListing(const UnicodeString Directory, bool UseCache);
  RawByteString __fastcall EncryptPassword(const UnicodeString & Password);
  UnicodeString __fastcall DecryptPassword(const RawByteString & Password);

  TFileOperationProgressType * __fastcall GetOperationProgress() { return FOperationProgress; }

public:
  explicit TTerminal();
  virtual void __fastcall Init(TSessionData * SessionData, TConfiguration * Configuration);
  virtual ~TTerminal();
  void __fastcall Open();
  void __fastcall Close();
  void __fastcall Reopen(int Params);
  virtual void __fastcall DirectoryModified(const UnicodeString Path, bool SubDirs);
  virtual void __fastcall DirectoryLoaded(TRemoteFileList * FileList);
  void __fastcall ShowExtendedException(const Exception * E);
  void __fastcall Idle();
  void __fastcall RecryptPasswords();
  bool __fastcall AllowedAnyCommand(const UnicodeString Command);
  void __fastcall AnyCommand(const UnicodeString Command, const TCaptureOutputEvent *OutputEvent);
  void CloseOnCompletion(TOnceDoneOperation Operation = odoDisconnect, const UnicodeString Message = L"");
  UnicodeString AbsolutePath(const UnicodeString Path, bool Local);
  void BeginTransaction();
  void ReadCurrentDirectory();
  void ReadDirectory(bool ReloadOnly, bool ForceCache = false);
  TRemoteFileList *ReadDirectoryListing(const UnicodeString Directory, const TFileMasks & Mask);
  TRemoteFileList *CustomReadDirectoryListing(const UnicodeString Directory, bool UseCache);
  TRemoteFile * __fastcall ReadFileListing(UnicodeString Path);
  void ReadFile(const UnicodeString FileName, TRemoteFile *& File);
  bool FileExists(const UnicodeString FileName, TRemoteFile ** File = NULL);
  void ReadSymlink(TRemoteFile * SymlinkFile, TRemoteFile *& File);
  bool CopyToLocal(TStrings * FilesToCopy,
    const UnicodeString TargetDir, const TCopyParamType * CopyParam, int Params);
  bool CopyToRemote(TStrings * FilesToCopy,
    const UnicodeString TargetDir, const TCopyParamType * CopyParam, int Params);
  void CreateDirectory(const UnicodeString DirName,
    const TRemoteProperties * Properties = NULL);
  void CreateLink(const UnicodeString FileName, const UnicodeString PointTo, bool Symbolic);
  void DeleteFile(const UnicodeString FileName,
    const TRemoteFile * File = NULL, void * Params = NULL);
  bool DeleteFiles(TStrings * FilesToDelete, int Params = 0);
  bool DeleteLocalFiles(TStrings * FileList, int Params = 0);
  bool IsRecycledFile(const UnicodeString FileName);
  void CustomCommandOnFile(const UnicodeString FileName,
    const TRemoteFile * File, void * AParams);
  void CustomCommandOnFiles(const UnicodeString Command, int Params,
    TStrings *Files, const TCaptureOutputEvent &OutputEvent);
  void ChangeDirectory(const UnicodeString Directory);
  void EndTransaction();
  void HomeDirectory();
  void ChangeFileProperties(const UnicodeString FileName,
    const TRemoteFile * File, /*const TRemoteProperties */ void * Properties);
  void ChangeFilesProperties(TStrings * FileList,
    const TRemoteProperties * Properties);
  bool LoadFilesProperties(TStrings * FileList);
  void TerminalError(const UnicodeString Msg);
  void TerminalError(const Exception * E, const UnicodeString Msg);
  void ReloadDirectory();
  void RefreshDirectory();
  void RenameFile(const UnicodeString FileName, const UnicodeString NewName);
  void RenameFile(const TRemoteFile * File, const UnicodeString NewName, bool CheckExistence);
  void MoveFile(const UnicodeString FileName, const TRemoteFile * File,
    /*const TMoveFileParams*/ void * Param);
  bool MoveFiles(TStrings * FileList, const UnicodeString Target,
    const UnicodeString FileMask);
  void CopyFile(const UnicodeString FileName, const TRemoteFile * File,
    /*const TMoveFileParams*/ void * Param);
  bool CopyFiles(TStrings * FileList, const UnicodeString Target,
    const UnicodeString FileMask);
  void CalculateFilesSize(TStrings * FileList, __int64 & Size,
    int Params, const TCopyParamType * CopyParam = NULL, TCalculateSizeStats * Stats = NULL);
  void CalculateFilesChecksum(const UnicodeString & Alg, TStrings * FileList,
    TStrings * Checksums, TCalculatedChecksumEvent *OnCalculatedChecksum);
  void ClearCaches();
  TSynchronizeChecklist * SynchronizeCollect(const UnicodeString LocalDirectory,
    const UnicodeString RemoteDirectory, TSynchronizeMode Mode,
     const TCopyParamType * CopyParam, int Params,
    const TSynchronizeDirectory &OnSynchronizeDirectory, TSynchronizeOptions * Options);
  void SynchronizeApply(TSynchronizeChecklist * Checklist,
    const UnicodeString LocalDirectory, const UnicodeString RemoteDirectory,
    const TCopyParamType * CopyParam, int Params,
    const TSynchronizeDirectory &OnSynchronizeDirectory);
  void __fastcall FilesFind(const UnicodeString Directory, const TFileMasks & FileMask,
    const TFileFoundEvent *OnFileFound, const TFindingFileEvent *OnFindingFile);
  void __fastcall SpaceAvailable(const UnicodeString Path, TSpaceAvailable & ASpaceAvailable);
  bool __fastcall DirectoryFileList(const UnicodeString Path,
    TRemoteFileList *& FileList, bool CanLoad);
  void MakeLocalFileList(const UnicodeString FileName,
    const WIN32_FIND_DATA &Rec, void * Param);
  UnicodeString __fastcall FileUrl(const UnicodeString FileName);
  bool __fastcall FileOperationLoopQuery(const Exception & E,
    TFileOperationProgressType *OperationProgress, const UnicodeString Message,
    bool AllowSkip, UnicodeString SpecialRetry = L"");
  TUsableCopyParamAttrs __fastcall UsableCopyParamAttrs(int Params);
  bool __fastcall QueryReopen(Exception * E, int Params,
    TFileOperationProgressType * OperationProgress);
  UnicodeString __fastcall PeekCurrentDirectory();
  void __fastcall FatalAbort();

  const TSessionInfo & __fastcall GetSessionInfo();
  const TFileSystemInfo & __fastcall GetFileSystemInfo(bool Retrieve = false);
  void __fastcall LogEvent(const UnicodeString & Str);

  static bool __fastcall IsAbsolutePath(const UnicodeString Path);
  static UnicodeString __fastcall ExpandFileName(UnicodeString Path,
    const UnicodeString BasePath);

#ifndef _MSC_VER
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
  __property UnicodeString TunnelPassword = { read = GetTunnelPassword };
  __property bool StoredCredentialsTried = { read = GetStoredCredentialsTried };
  __property TQueryUserEvent OnQueryUser = { read = FOnQueryUser, write = FOnQueryUser };
  __property TPromptUserEvent OnPromptUser = { read = FOnPromptUser, write = FOnPromptUser };
  __property TDisplayBannerEvent OnDisplayBanner = { read = FOnDisplayBanner, write = FOnDisplayBanner };
  __property TExtendedExceptionEvent OnShowExtendedException = { read = FOnShowExtendedException, write = FOnShowExtendedException };
  __property TInformationEvent OnInformation = { read = FOnInformation, write = FOnInformation };
  __property TNotifyEvent OnClose = { read = FOnClose, write = FOnClose };
  __property int TunnelLocalPortNumber = { read = FTunnelLocalPortNumber };
#else
  bool __fastcall GetIsCapable(TFSCapability Capability) const;
  TSessionData *__fastcall GetSessionData() { return FSessionData; }
  TSessionLog *__fastcall GetLog() { return FLog; }
  TActionLog * __fastcall GetActionLog() { return FActionLog; };
  TConfiguration *__fastcall GetConfiguration() { return FConfiguration; }
  bool __fastcall GetActive();
  TSessionStatus __fastcall GetStatus() { return FStatus; }
  UnicodeString __fastcall GetCurrentDirectory();
  void SetCurrentDirectory(const UnicodeString value);
  bool GetExceptionOnFail() const;
  void SetExceptionOnFail(bool value);
  TRemoteDirectory *GetFiles() { return FFiles; }
  const notify_signal_type &GetOnChangeDirectory() const { return FOnChangeDirectory; }
  void SetOnChangeDirectory(const TNotifyEvent &value) { FOnChangeDirectory.connect(value); }
  readdirectory_signal_type &GetOnReadDirectory() { return FOnReadDirectory; }
  void SetOnReadDirectory(const TReadDirectoryProgressEvent &value) { FOnReadDirectory.connect(value); }
  const notify_signal_type &GetOnStartReadDirectory() const { return FOnStartReadDirectory; }
  void SetOnStartReadDirectory(const TNotifyEvent &value) { FOnStartReadDirectory.connect(value); }
  readdirectoryprogress_signal_type &GetOnReadDirectoryProgress() { return FOnReadDirectoryProgress; }
  void SetOnReadDirectoryProgress(const TReadDirectoryProgressEvent &value) { FOnReadDirectoryProgress.connect(value); }
  deletelocalfile_signal_type &GetOnDeleteLocalFile() { return FOnDeleteLocalFile; }
  void SetOnDeleteLocalFile(const TDeleteLocalFileEvent &value) { FOnDeleteLocalFile.connect(value); }
  const TRemoteTokenList *GetGroups();
  const TRemoteTokenList *GetUsers();
  const TRemoteTokenList *GetMembership();
  const fileoperationprogress_signal_type &GetOnProgress() const { return FOnProgress; }
  void SetOnProgress(const TFileOperationProgressEvent &value) { FOnProgress.connect(value); }
  const fileoperationfinished_signal_type &GetOnFinished() const { return FOnFinished; }
  void SetOnFinished(const TFileOperationFinished &value) { FOnFinished.connect(value); }
  TCurrentFSProtocol GetFSProtocol() { return FFSProtocol; }
  bool GetUseBusyCursor() { return FUseBusyCursor; }
  void SetUseBusyCursor(bool value) { FUseBusyCursor = value; }
  UnicodeString GetUserName();
  bool GetAreCachesEmpty() const;
  bool GetCommandSessionOpened();
  TTerminal *GetCommandSession();
  bool GetAutoReadDirectory() { return FAutoReadDirectory; }
  void SetAutoReadDirectory(bool value) { FAutoReadDirectory = value; }
  bool GetResolvingSymlinks();
  UnicodeString GetPassword();
  UnicodeString GetTunnelPassword();
  bool GetStoredCredentialsTried();
  queryuser_signal_type &GetOnQueryUser() { return FOnQueryUser; }
  void SetOnQueryUser(const TQueryUserEvent &value) { FOnQueryUser.connect(value); }
  promptuser_signal_type &GetOnPromptUser() { return FOnPromptUser; }
  void SetOnPromptUser(const TPromptUserEvent &value) { FOnPromptUser.connect(value); }
  displaybanner_signal_type &GetOnDisplayBanner() { return FOnDisplayBanner; }
  void SetOnDisplayBanner(const TDisplayBannerEvent &value) { FOnDisplayBanner.connect(value); }
  extendedexception_signal_type &GetOnShowExtendedException() { return FOnShowExtendedException; }
  void SetOnShowExtendedException(const TExtendedExceptionEvent &value) { FOnShowExtendedException.connect(value); }
  informationevent_signal_type &GetOnInformation() { return FOnInformation; }
  void SetOnInformation(const TInformationEvent &value) { FOnInformation.connect(value); }
  const notify_signal_type &GetOnClose() const { return FOnClose; }
  void SetOnClose(const TNotifyEvent &value) { FOnClose.connect(value); }
  size_t GetTunnelLocalPortNumber() { return FTunnelLocalPortNumber; }
#endif
};
//---------------------------------------------------------------------------
class TSecondaryTerminal : public TTerminal
{
public:
  explicit TSecondaryTerminal(TTerminal * MainTerminal);
  virtual void __fastcall Init(TSessionData * SessionData, TConfiguration * Configuration,
    const UnicodeString & Name);
  virtual ~TSecondaryTerminal()
  {}

protected:
  virtual void __fastcall DirectoryLoaded(TRemoteFileList * FileList);
  virtual void __fastcall DirectoryModified(const UnicodeString Path,
    bool SubDirs);
  virtual bool __fastcall DoPromptUser(TSessionData * Data, TPromptKind Kind,
    UnicodeString Name, UnicodeString Instructions, TStrings * Prompts, TStrings * Results);

private:
  bool FMasterPasswordTried;
  bool FMasterTunnelPasswordTried;
  TTerminal * FMainTerminal;
};
//---------------------------------------------------------------------------
class TTerminalList : public TObjectList
{
public:
  explicit TTerminalList(TConfiguration * AConfiguration);
  virtual ~TTerminalList();

  virtual TTerminal * __fastcall NewTerminal(TSessionData * Data);
  virtual void __fastcall FreeTerminal(TTerminal * Terminal);
  void __fastcall FreeAndNullTerminal(TTerminal * & Terminal);
  virtual void __fastcall Idle();
  void __fastcall RecryptPasswords();

#ifndef _MSC_VER
  __property TTerminal * Terminals[int Index]  = { read=GetTerminal };
  __property int ActiveCount = { read = GetActiveCount };
#else
  TTerminal *GetTerminal(size_t Index);
  int GetActiveCount();
#endif

protected:
  virtual TTerminal *CreateTerminal(TSessionData * Data);

private:
  TConfiguration * FConfiguration;

#ifndef _MSC_VER
  TTerminal * __fastcall GetTerminal(int Index);
  int __fastcall GetActiveCount();
#endif
};
//---------------------------------------------------------------------------
struct TCustomCommandParams
{
  TCustomCommandParams(
    UnicodeString Command,
    int Params,
    const TCaptureOutputEvent &OutputEvent) :
    Command(Command),
    Params(Params),
    OutputEvent(OutputEvent)
  {
  }
  UnicodeString Command;
  int Params;
  const TCaptureOutputEvent &OutputEvent;
private:
  TCustomCommandParams(const TCustomCommandParams &);
  void operator=(const TCustomCommandParams &);
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

  class TItem
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

  size_t GetCount() const;
  const TItem *GetItem(size_t Index) const;

protected:
  TSynchronizeChecklist();

  void Sort();
  void Add(TItem * Item);


private:
  TList * FList;

  static int __fastcall Compare(void * Item1, void * Item2);
};
//---------------------------------------------------------------------------
struct TSpaceAvailable
{
  TSpaceAvailable();

  __int64 BytesOnDevice;
  __int64 UnusedBytesOnDevice;
  __int64 BytesAvailableToUser;
  __int64 UnusedBytesAvailableToUser;
  size_t BytesPerAllocationUnit;
};
//---------------------------------------------------------------------------
#endif
