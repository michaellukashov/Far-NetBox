
#pragma once

#include <rdestl/map.h>
#include <rdestl/vector.h>

#include <Classes.hpp>
#include <Common.h>
#include <Exceptions.h>

#include "SessionInfo.h"
#include "Interface.h"
#include "FileOperationProgress.h"
#include "FileMasks.h"

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
class TParallelOperation;
class TCollectedFileList;

#if 0
typedef void (__closure *TQueryUserEvent)
  (TObject * Sender, const UnicodeString Query, TStrings * MoreMessages, uintptr_t Answers,
   const TQueryParams * Params, uintptr_t & Answer, TQueryType QueryType, void * Arg);
#endif
typedef nb::FastDelegate8<void,
  TObject * /*Sender*/, UnicodeString /*Query*/, TStrings * /*MoreMessages*/, uintptr_t /*Answers*/,
   const TQueryParams * /*Params*/, uintptr_t & /*Answer*/, TQueryType /*QueryType*/, void * /*Arg*/> TQueryUserEvent;
#if 0
typedef void (__closure *TPromptUserEvent)
  (TTerminal * Terminal, TPromptKind Kind, UnicodeString Name, UnicodeString Instructions,
   TStrings * Prompts, TStrings * Results, bool & Result, void * Arg);
#endif
typedef nb::FastDelegate8<void,
  TTerminal * /*Terminal*/, TPromptKind /*Kind*/, UnicodeString /*Name*/,
  UnicodeString /*Instructions*/,
  TStrings * /*Prompts*/, TStrings * /*Results*/,
  bool & /*Result*/, void * /*Arg*/> TPromptUserEvent;
#if 0
typedef void (__closure *TDisplayBannerEvent)
  (TTerminal * Terminal, UnicodeString SessionName, UnicodeString Banner,
   bool & NeverShowAgain, int Options);
#endif
typedef nb::FastDelegate5<void,
  TTerminal * /*Terminal*/, UnicodeString /*SessionName*/, UnicodeString /*Banner*/,
  bool & /*NeverShowAgain*/, intptr_t /*Options*/> TDisplayBannerEvent;
#if 0
typedef void (__closure *TExtendedExceptionEvent)
  (TTerminal * Terminal, Exception * E, void * Arg);
#endif
typedef nb::FastDelegate3<void,
  TTerminal * /*Terminal*/, Exception * /*E*/, void * /*Arg*/ > TExtendedExceptionEvent;
#if 0
typedef void (__closure *TReadDirectoryEvent)(System::TObject * Sender, Boolean ReloadOnly);
#endif
typedef nb::FastDelegate2<void, TObject * /*Sender*/,
  Boolean /*ReloadOnly*/> TReadDirectoryEvent;
#if 0
typedef void (__closure *TReadDirectoryProgressEvent)(
  System::TObject* Sender, int Progress, int ResolvedLinks, bool & Cancel);
#endif
typedef nb::FastDelegate4<void,
  TObject * /*Sender*/, intptr_t /*Progress*/, intptr_t /*ResolvedLinks*/, bool & /*Cancel*/> TReadDirectoryProgressEvent;
#if 0
typedef void (__closure *TProcessFileEvent)
  (const UnicodeString FileName, const TRemoteFile * File, void * Param);
#endif
typedef nb::FastDelegate3<void,
  UnicodeString /*FileName*/, const TRemoteFile * /*File*/, void * /*Param*/> TProcessFileEvent;
#if 0
typedef void (__closure *TProcessFileEventEx)
  (const UnicodeString FileName, const TRemoteFile * File, void * Param, int Index);
#endif
typedef nb::FastDelegate4<void,
  UnicodeString /*FileName*/, const TRemoteFile * /*File*/,
  void * /*Param*/, intptr_t /*Index*/> TProcessFileEventEx;
#if 0
typedef int (__closure *TFileOperationEvent)
  (void * Param1, void * Param2);
#endif
typedef nb::FastDelegate2<intptr_t,
  void * /*Param1*/, void * /*Param2*/> TFileOperationEvent;
#if 0
typedef void (__closure *TSynchronizeDirectory)
  (const UnicodeString LocalDirectory, const UnicodeString RemoteDirectory,
   bool & Continue, bool Collect);
#endif
typedef nb::FastDelegate4<void,
  UnicodeString /*LocalDirectory*/, UnicodeString /*RemoteDirectory*/,
  bool & /*Continue*/, bool /*Collect*/> TSynchronizeDirectoryEvent;
#if 0
typedef void (__closure *TDeleteLocalFileEvent)(
  const UnicodeString FileName, bool Alternative);
#endif
typedef nb::FastDelegate2<void,
  UnicodeString /*FileName*/, bool /*Alternative*/> TDeleteLocalFileEvent;
#if 0
typedef int (__closure *TDirectoryModifiedEvent)
  (TTerminal * Terminal, const UnicodeString Directory, bool SubDirs);
#endif
typedef nb::FastDelegate3<int,
  TTerminal * /*Terminal*/, UnicodeString /*Directory*/, bool /*SubDirs*/> TDirectoryModifiedEvent;
#if 0
typedef void (__closure *TInformationEvent)
  (TTerminal * Terminal, UnicodeString Str, bool Status, int Phase);
#endif
typedef nb::FastDelegate4<void,
  TTerminal * /*Terminal*/, UnicodeString /*Str*/, bool /*Status*/, intptr_t /*Phase*/> TInformationEvent;
#if 0
typedef void (__closure *TCustomCommandEvent)
  (TTerminal * Terminal, const UnicodeString Command, bool & Handled);
#endif
typedef nb::FastDelegate3<void,
  TTerminal * /*Terminal*/, UnicodeString /*Command*/, bool & /*Handled*/> TCustomCommandEvent;

typedef nb::FastDelegate5<HANDLE,
  UnicodeString /*FileName*/, DWORD /*DesiredAccess*/,
  DWORD /*ShareMode*/, DWORD /*CreationDisposition*/,
  DWORD /*FlagsAndAttributes*/> TCreateLocalFileEvent;
typedef nb::FastDelegate1<DWORD,
  UnicodeString /*FileName*/> TGetLocalFileAttributesEvent;
typedef nb::FastDelegate2<bool,
  UnicodeString /*FileName*/, DWORD /*FileAttributes*/> TSetLocalFileAttributesEvent;
typedef nb::FastDelegate3<bool,
  UnicodeString /*FileName*/, UnicodeString /*NewFileName*/,
  DWORD /*Flags*/> TMoveLocalFileEvent;
typedef nb::FastDelegate1<bool,
  UnicodeString /*LocalDirName*/> TRemoveLocalDirectoryEvent;
typedef nb::FastDelegate2<bool,
  UnicodeString /*LocalDirName*/,
  LPSECURITY_ATTRIBUTES /*SecurityAttributes*/> TCreateLocalDirectoryEvent;
typedef nb::FastDelegate0<bool> TCheckForEscEvent;

#if 0
#define THROW_SKIP_FILE(EXCEPTION, MESSAGE) \
  throw EScpSkipFile(EXCEPTION, MESSAGE)
#define THROW_SKIP_FILE_NULL THROW_SKIP_FILE(nullptr, L"")

/* TODO : Better user interface (query to user) */
#define FILE_OPERATION_LOOP_BEGIN \
  { \
    bool DoRepeat; \
    do { \
      DoRepeat = false; \
      try \

#define FILE_OPERATION_LOOP_END_CUSTOM(MESSAGE, ALLOW_SKIP, HELPKEYWORD) \
      catch (EAbort & E) \
      { \
        throw; \
      } \
      catch (EScpSkipFile & E) \
      { \
        throw; \
      } \
      catch (EFatal & E) \
      { \
        throw; \
      } \
      catch (Exception & E) \
      { \
        FILE_OPERATION_LOOP_TERMINAL->FileOperationLoopQuery( \
          E, OperationProgress, MESSAGE, ALLOW_SKIP, L"", HELPKEYWORD); \
        DoRepeat = true; \
      } \
    } while (DoRepeat); \
  }

#define FILE_OPERATION_LOOP_END_EX(MESSAGE, ALLOW_SKIP) \
  FILE_OPERATION_LOOP_END_CUSTOM(MESSAGE, ALLOW_SKIP, L"")
#define FILE_OPERATION_LOOP_END(MESSAGE) \
  FILE_OPERATION_LOOP_END_EX(MESSAGE, true)

enum TCurrentFSProtocol { cfsUnknown, cfsSCP, cfsSFTP, cfsFTP, cfsWebDAV };
#endif // #if 0

enum TCurrentFSProtocol
{
  cfsUnknown,
  cfsSCP,
  cfsSFTP,
  cfsFTP,
  cfsFTPS,
  cfsWebDAV
};

inline void ThrowSkipFile(Exception *Exception, UnicodeString Message)
{
  throw ESkipFile(Exception, Message);
}
inline void ThrowSkipFileNull() { ThrowSkipFile(nullptr, L""); }

NB_CORE_EXPORT void FileOperationLoopCustom(TTerminal *Terminal,
  TFileOperationProgressType *OperationProgress,
  bool AllowSkip, UnicodeString Message,
  UnicodeString HelpKeyword,
  const std::function<void()> &Operation);

const int cpDelete = 0x01;
const int cpTemporary = 0x04;
const int cpNoConfirmation = 0x08;
const int cpNewerOnly = 0x10;
const int cpAppend = 0x20;
const int cpResume = 0x40;
const int cpNoRecurse = 0x80;

const int ccApplyToDirectories = 0x01;
const int ccRecursive = 0x02;
const int ccUser = 0x100;

const int csIgnoreErrors = 0x01;

const int ropNoReadDirectory = 0x02;

const int boDisableNeverShowAgain = 0x01;

class NB_CORE_EXPORT TTerminal : public TSessionUI
{
  NB_DISABLE_COPY(TTerminal)
public:
  static inline bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TTerminal); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TTerminal) || TSessionUI::is(Kind); }
public:
  // TScript::SynchronizeProc relies on the order
#if 0
  enum TSynchronizeMode { smRemote, smLocal, smBoth };
#endif // #if 0
  enum TSynchronizeMode
  {
    smRemote,
    smLocal,
    smBoth,
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
  static const int spDefault = TTerminal::spNoConfirmation | TTerminal::spPreviewChanges;

// for TranslateLockedPath()
  friend class TRemoteFile;
// for ReactOnCommand()
  friend class TSCPFileSystem;
  friend class TSFTPFileSystem;
  friend class TFTPFileSystem;
  friend class TWebDAVFileSystem;
  friend class TTunnelUI;
  friend class TCallbackGuard;
  friend class TSecondaryTerminal;
  friend class TRetryOperationLoop;

private:
  TSessionData *FSessionData;
  TSessionLog *FLog;
  TActionLog *FActionLog;
  TConfiguration *FConfiguration;
  UnicodeString FCurrentDirectory;
  UnicodeString FLockDirectory;
  intptr_t FExceptionOnFail;
  TRemoteDirectory *FFiles;
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
  TFileOperationProgressType *FOperationProgress;
  bool FUseBusyCursor;
  TRemoteDirectoryCache *FDirectoryCache;
  TRemoteDirectoryChangesCache *FDirectoryChangesCache;
  TSecureShell *FSecureShell;
  UnicodeString FLastDirectoryChange;
  TCurrentFSProtocol FFSProtocol;
  TTerminal *FCommandSession;
  bool FAutoReadDirectory;
  bool FReadingCurrentDirectory;
  bool *FClosedOnCompletion;
  TSessionStatus FStatus;
  intptr_t FOpening;
  RawByteString FRememberedPassword;
  RawByteString FRememberedTunnelPassword;
  TTunnelThread *FTunnelThread;
  TSecureShell *FTunnel;
  TSessionData *FTunnelData;
  TSessionLog *FTunnelLog;
  TTunnelUI *FTunnelUI;
  intptr_t FTunnelLocalPortNumber;
  UnicodeString FTunnelError;
  TQueryUserEvent FOnQueryUser;
  TPromptUserEvent FOnPromptUser;
  TDisplayBannerEvent FOnDisplayBanner;
  TExtendedExceptionEvent FOnShowExtendedException;
  TInformationEvent FOnInformation;
  TCustomCommandEvent FOnCustomCommand;
  TNotifyEvent FOnClose;
  TCheckForEscEvent FOnCheckForEsc;
  TCallbackGuard *FCallbackGuard;
  TFindingFileEvent FOnFindingFile;
  bool FEnableSecureShellUsage;
  bool FCollectFileSystemUsage;
  bool FRememberedPasswordTried;
  bool FRememberedTunnelPasswordTried;
  intptr_t FNesting;
  UnicodeString FFingerprintScanned;
  DWORD FLastProgressLogged;
  TRemoteDirectory *FOldFiles;
  UnicodeString FDestFileName;
  bool FMultipleDestinationFiles;

public:
  void CommandError(Exception *E, UnicodeString Msg);
  uintptr_t CommandError(Exception *E, UnicodeString Msg,
    uintptr_t Answers, UnicodeString HelpKeyword = L"");
  UnicodeString RemoteGetCurrentDirectory();
  bool GetExceptionOnFail() const;
  const TRemoteTokenList *GetGroups() const { return const_cast<TTerminal *>(this)->GetGroups(); }
  TRemoteTokenList *GetGroups();
  const TRemoteTokenList *GetUsers() const { return const_cast<TTerminal *>(this)->GetUsers(); }
  TRemoteTokenList *GetUsers();
  TRemoteTokenList *GetMembership();
  const TRemoteTokenList *GetMembership() const { return const_cast<TTerminal *>(this)->GetMembership(); }
  void TerminalSetCurrentDirectory(UnicodeString AValue);
  void SetExceptionOnFail(bool Value);
  void ReactOnCommand(intptr_t /*TFSCommand*/ Cmd);
  UnicodeString TerminalGetUserName() const;
  bool GetAreCachesEmpty() const;
  void ClearCachedFileList(UnicodeString APath, bool SubDirs);
  void AddCachedFileList(TRemoteFileList *FileList);
  bool GetCommandSessionOpened() const;
  TTerminal *GetCommandSession();
  bool GetResolvingSymlinks() const;
  bool GetActive() const;
  UnicodeString GetPassword() const;
  UnicodeString GetRememberedPassword() const;
  UnicodeString GetRememberedTunnelPassword() const;
  bool GetStoredCredentialsTried() const;
  bool InTransaction() const;
  void SaveCapabilities(TFileSystemInfo &FileSystemInfo);
  static UnicodeString SynchronizeModeStr(TSynchronizeMode Mode);
  static UnicodeString SynchronizeParamsStr(intptr_t Params);

protected:
  bool FReadCurrentDirectoryPending;
  bool FReadDirectoryPending;
  bool FTunnelOpening;
  TCustomFileSystem *FFileSystem;

  void DoStartReadDirectory();
  void DoReadDirectoryProgress(intptr_t Progress, intptr_t ResolvedLinks, bool &Cancel);
  void DoReadDirectory(bool ReloadOnly);
  void DoCreateDirectory(UnicodeString ADirName);
  void DoDeleteFile(UnicodeString AFileName, const TRemoteFile *AFile,
    intptr_t Params);
  void DoCustomCommandOnFile(UnicodeString AFileName,
    const TRemoteFile *AFile, UnicodeString Command, intptr_t Params, TCaptureOutputEvent OutputEvent);
  void DoRenameFile(UnicodeString AFileName,
    UnicodeString ANewName, bool Move);
  void DoCopyFile(UnicodeString AFileName, UnicodeString ANewName);
  void DoChangeFileProperties(UnicodeString AFileName,
    const TRemoteFile *AFile, const TRemoteProperties *Properties);
  void DoChangeDirectory();
  void DoInitializeLog();
  void EnsureNonExistence(UnicodeString AFileName);
  void LookupUsersGroups();
  void FileModified(const TRemoteFile *AFile,
    UnicodeString AFileName, bool ClearDirectoryChange = false);
  intptr_t FileOperationLoop(TFileOperationEvent CallBackFunc,
    TFileOperationProgressType *OperationProgress, bool AllowSkip,
    UnicodeString Message, void *Param1 = nullptr, void *Param2 = nullptr);
  bool GetIsCapableProtected(TFSCapability Capability) const;
  bool ProcessFiles(const TStrings *AFileList, TFileOperation Operation,
    TProcessFileEvent ProcessFile, void *Param = nullptr, TOperationSide Side = osRemote,
    bool Ex = false);
  bool ProcessFilesEx(TStrings *FileList, TFileOperation Operation,
    TProcessFileEventEx ProcessFile, void *Param = nullptr, TOperationSide Side = osRemote);
  void ProcessDirectory(UnicodeString ADirName,
    TProcessFileEvent CallBackFunc, void *Param = nullptr, bool UseCache = false,
    bool IgnoreErrors = false);
  void AnnounceFileListOperation();
  UnicodeString TranslateLockedPath(UnicodeString APath, bool Lock);
  void ReadDirectory(TRemoteFileList *AFileList);
  void CustomReadDirectory(TRemoteFileList *AFileList);
  void DoCreateLink(UnicodeString AFileName, UnicodeString PointTo, bool Symbolic);
  bool TerminalCreateLocalFile(UnicodeString ATargetFileName,
    TFileOperationProgressType *OperationProgress,
    bool Resume,
    bool NoConfirmation,
    HANDLE *AHandle);
  HANDLE TerminalCreateLocalFile(UnicodeString LocalFileName, DWORD DesiredAccess,
    DWORD ShareMode, DWORD CreationDisposition, DWORD FlagsAndAttributes);
  void TerminalOpenLocalFile(UnicodeString AFileName, DWORD Access,
    uintptr_t *AAttrs, HANDLE *AHandle, int64_t *ACTime,
    int64_t *AMTime, int64_t *AATime, int64_t *ASize,
    bool TryWriteReadOnly = true);
  bool AllowLocalFileTransfer(UnicodeString AFileName,
    const TCopyParamType *CopyParam, TFileOperationProgressType *OperationProgress);
  bool HandleException(Exception *E);
  void CalculateFileSize(UnicodeString AFileName,
    const TRemoteFile *AFile, /*TCalculateSizeParams*/ void *AParam);
  void DoCalculateFileSize(UnicodeString AFileName,
    const TRemoteFile *AFile, void *AParam);
  bool DoCalculateDirectorySize(UnicodeString AFileName,
    const TRemoteFile *AFile, TCalculateSizeParams *Params);
  void CalculateLocalFileSize(UnicodeString AFileName,
    const TSearchRec &Rec, /*int64_t*/ void *Size);
  bool CalculateLocalFilesSize(const TStrings *AFileList,
    const TCopyParamType *CopyParam, bool AllowDirs, TStrings *Files,
    int64_t &Size);
  TBatchOverwrite EffectiveBatchOverwrite(
    UnicodeString ASourceFullFileName, const TCopyParamType *CopyParam, intptr_t Params,
    TFileOperationProgressType *OperationProgress, bool Special) const;
  bool CheckRemoteFile(
    UnicodeString AFileName, const TCopyParamType *CopyParam,
    intptr_t Params, TFileOperationProgressType *OperationProgress) const;
  uintptr_t ConfirmFileOverwrite(
    UnicodeString ASourceFullFileName, UnicodeString ATargetFileName,
    const TOverwriteFileParams *FileParams, uintptr_t Answers, TQueryParams *QueryParams,
    TOperationSide Side, const TCopyParamType *CopyParam, intptr_t Params,
    TFileOperationProgressType *OperationProgress, UnicodeString AMessage = L"");
  void DoSynchronizeCollectDirectory(UnicodeString ALocalDirectory,
    UnicodeString ARemoteDirectory, TSynchronizeMode Mode,
    const TCopyParamType *CopyParam, intptr_t Params,
    TSynchronizeDirectoryEvent OnSynchronizeDirectory,
    TSynchronizeOptions *Options, intptr_t Level, TSynchronizeChecklist *Checklist);
  void DoSynchronizeCollectFile(UnicodeString AFileName,
    const TRemoteFile *AFile, /*TSynchronizeData*/ void *Param);
  void SynchronizeCollectFile(UnicodeString AFileName,
    const TRemoteFile *AFile, /*TSynchronizeData*/ void *Param);
  void SynchronizeRemoteTimestamp(UnicodeString AFileName,
    const TRemoteFile *AFile, void *Param);
  void SynchronizeLocalTimestamp(UnicodeString AFileName,
    const TRemoteFile *AFile, void *Param);
  void DoSynchronizeProgress(const TSynchronizeData &Data, bool Collect);
  void DeleteLocalFile(UnicodeString AFileName,
    const TRemoteFile *AFile, void *Params);
  void RecycleFile(UnicodeString AFileName, const TRemoteFile *AFile);
  TStrings *GetFixedPaths() const;
  void DoStartup();
  virtual bool DoQueryReopen(Exception *E);
  virtual void FatalError(Exception *E, UnicodeString Msg, UnicodeString HelpKeyword = L"") override;
  void ResetConnection();
  virtual bool DoPromptUser(TSessionData *Data, TPromptKind Kind,
    UnicodeString Name, UnicodeString Instructions, TStrings *Prompts,
    TStrings *Response);
  void OpenTunnel();
  void CloseTunnel();
  void DoInformation(UnicodeString Str, bool Status, intptr_t Phase = -1);
  bool PromptUser(TSessionData *Data, TPromptKind Kind,
    UnicodeString AName, UnicodeString Instructions, UnicodeString Prompt, bool Echo,
    intptr_t MaxLen, UnicodeString &AResult);
  void FileFind(UnicodeString AFileName, const TRemoteFile *AFile, void *Param);
  void DoFilesFind(UnicodeString Directory, TFilesFindParams &Params, UnicodeString RealDirectory);
  bool DoCreateLocalFile(UnicodeString AFileName,
    TFileOperationProgressType *OperationProgress,
    bool Resume,
    bool NoConfirmation,
    HANDLE *AHandle);
  void LockFile(UnicodeString AFileName, const TRemoteFile *AFile, void *AParam);
  void UnlockFile(UnicodeString AFileName, const TRemoteFile *AFile, void *AParam);
  void DoLockFile(UnicodeString AFileName, const TRemoteFile *AFile);
  void DoUnlockFile(UnicodeString AFileName, const TRemoteFile *AFile);

  virtual void Information(UnicodeString Str, bool Status) override;
  virtual uintptr_t QueryUser(UnicodeString Query,
    TStrings *MoreMessages, uintptr_t Answers, const TQueryParams *Params,
    TQueryType QueryType = qtConfirmation) override;
  virtual uintptr_t QueryUserException(UnicodeString Query,
    Exception *E, uintptr_t Answers, const TQueryParams *Params,
    TQueryType QueryType = qtConfirmation) override;
  virtual bool PromptUser(TSessionData *Data, TPromptKind Kind,
    UnicodeString AName, UnicodeString Instructions, TStrings *Prompts, TStrings *Results) override;
  virtual void DisplayBanner(UnicodeString Banner) override;
  virtual void Closed() override;
  virtual void ProcessGUI() override;
  void Progress(TFileOperationProgressType *OperationProgress);
  virtual void HandleExtendedException(Exception *E) override;
  bool IsListenerFree(uintptr_t PortNumber) const;
  void DoProgress(TFileOperationProgressType &ProgressData);
  void DoFinished(TFileOperation Operation, TOperationSide Side, bool Temp,
    UnicodeString AFileName, bool Success, TOnceDoneOperation &OnceDoneOperation);
  void RollbackAction(TSessionAction &Action,
    TFileOperationProgressType *OperationProgress, Exception *E = nullptr);
  void DoAnyCommand(UnicodeString ACommand, TCaptureOutputEvent OutputEvent,
    TCallSessionAction *Action);
  TRemoteFileList *DoReadDirectoryListing(UnicodeString ADirectory, bool UseCache);
  RawByteString EncryptPassword(UnicodeString APassword) const;
  UnicodeString DecryptPassword(RawByteString APassword) const;
  UnicodeString GetRemoteFileInfo(TRemoteFile *AFile) const;
  void LogRemoteFile(TRemoteFile *AFile);
  UnicodeString FormatFileDetailsForLog(UnicodeString AFileName, const TDateTime &AModification, int64_t Size);
  void LogFileDetails(UnicodeString AFileName, const TDateTime &AModification, int64_t Size);
  void LogFileDone(TFileOperationProgressType *OperationProgress, UnicodeString DestFileName);
  void LogTotalTransferDetails(
    const UnicodeString TargetDir, const TCopyParamType *CopyParam,
    TFileOperationProgressType *OperationProgress, bool Parallel, TStrings *Files);
  void LogTotalTransferDone(TFileOperationProgressType *OperationProgress);
  virtual TTerminal *GetPasswordSource() { return this; }
  virtual const TTerminal *GetPasswordSource() const { return this; }
  void DoEndTransaction(bool Inform);
  bool VerifyCertificate(
    UnicodeString CertificateStorageKey, UnicodeString SiteKey,
    UnicodeString Fingerprint,
    UnicodeString CertificateSubject, int Failures);
  void CacheCertificate(UnicodeString CertificateStorageKey,
    UnicodeString SiteKey, UnicodeString Fingerprint, int Failures);
  void CollectTlsUsage(UnicodeString TlsVersionStr);
  bool LoadTlsCertificate(X509 *&Certificate, EVP_PKEY *&PrivateKey);
  bool TryStartOperationWithFile(
    UnicodeString AFileName, TFileOperation Operation1, TFileOperation Operation2 = foNone);
  void StartOperationWithFile(
    UnicodeString AFileName, TFileOperation Operation1, TFileOperation Operation2 = foNone);
  bool CanRecurseToDirectory(const TRemoteFile *AFile) const;
  bool DoOnCustomCommand(UnicodeString Command);
  bool CanParallel(const TCopyParamType *CopyParam, intptr_t Params, TParallelOperation *ParallelOperation) const;
  void CopyParallel(TParallelOperation *ParallelOperation, TFileOperationProgressType *OperationProgress);

#if 0
  __property TFileOperationProgressType *OperationProgress = { read = FOperationProgress };
#endif
  const TFileOperationProgressType *GetOperationProgress() const { return FOperationProgress; }
  TFileOperationProgressType *GetOperationProgress() { return FOperationProgress; }
  void SetOperationProgress(TFileOperationProgressType *OperationProgress) { FOperationProgress = OperationProgress; }

public:
  explicit TTerminal(TObjectClassId Kind = OBJECT_CLASS_TTerminal);
  void Init(TSessionData *SessionData, TConfiguration *Configuration);
  virtual ~TTerminal();
  void Open();
  void Close();
  UnicodeString FingerprintScan();
  void Reopen(intptr_t Params);
  virtual void DirectoryModified(UnicodeString APath, bool SubDirs);
  virtual void DirectoryLoaded(TRemoteFileList *FileList);
  void ShowExtendedException(Exception *E);
  void Idle();
  void RecryptPasswords();
  bool AllowedAnyCommand(UnicodeString Command) const;
  void AnyCommand(UnicodeString Command, TCaptureOutputEvent OutputEvent);
  void CloseOnCompletion(
    TOnceDoneOperation Operation = odoDisconnect, UnicodeString Message = L"",
    UnicodeString TargetLocalPath = L"", UnicodeString DestLocalFileName = L"");
  UnicodeString GetAbsolutePath(UnicodeString APath, bool Local) const;
  void BeginTransaction();
  void ReadCurrentDirectory();
  void ReadDirectory(bool ReloadOnly, bool ForceCache = false);
  TRemoteFileList *ReadDirectoryListing(UnicodeString Directory, const TFileMasks &Mask);
  TRemoteFileList *CustomReadDirectoryListing(UnicodeString Directory, bool UseCache);
  TRemoteFile *ReadFileListing(UnicodeString APath);
  void ReadFile(UnicodeString AFileName, TRemoteFile *&AFile);
  bool FileExists(UnicodeString AFileName, TRemoteFile **AFile = nullptr);
  void ReadSymlink(TRemoteFile *SymlinkFile, TRemoteFile *&File);
  bool CopyToLocal(const TStrings *AFilesToCopy,
    UnicodeString TargetDir, const TCopyParamType *CopyParam, intptr_t Params, TParallelOperation *ParallelOperation);
  bool CopyToRemote(const TStrings *AFilesToCopy,
    UnicodeString ATargetDir, const TCopyParamType *CopyParam, intptr_t Params, TParallelOperation *ParallelOperation);
  intptr_t CopyToParallel(TParallelOperation *ParallelOperation, TFileOperationProgressType *OperationProgress);
  void LogParallelTransfer(TParallelOperation *ParallelOperation);
  void RemoteCreateDirectory(UnicodeString ADirName,
    const TRemoteProperties *Properties = nullptr);
  void CreateLink(UnicodeString AFileName, UnicodeString PointTo, bool Symbolic, bool IsDirectory);
  void RemoteDeleteFile(const UnicodeString AFileName,
    const TRemoteFile *AFile = nullptr, void *AParams = nullptr);
  bool RemoteDeleteFiles(TStrings *AFilesToDelete, intptr_t Params = 0);
  bool DeleteLocalFiles(TStrings *AFileList, intptr_t Params = 0);
  bool IsRecycledFile(UnicodeString AFileName);
  void CustomCommandOnFile(UnicodeString AFileName,
    const TRemoteFile *AFile, void *AParams);
  void CustomCommandOnFiles(UnicodeString Command, intptr_t Params,
    TStrings *AFiles, TCaptureOutputEvent OutputEvent);
  void RemoteChangeDirectory(UnicodeString Directory);
  void EndTransaction();
  void HomeDirectory();
  void ChangeFileProperties(UnicodeString AFileName,
    const TRemoteFile *AFile, /*const TRemoteProperties*/ void *Properties);
  void ChangeFilesProperties(TStrings *AFileList,
    const TRemoteProperties *Properties);
  bool LoadFilesProperties(TStrings *AFileList);
  void TerminalError(UnicodeString Msg);
  void TerminalError(Exception *E, UnicodeString Msg, UnicodeString HelpKeyword = L"");
  void ReloadDirectory();
  void RefreshDirectory();
  void TerminalRenameFile(UnicodeString AFileName, UnicodeString ANewName);
  void TerminalRenameFile(const TRemoteFile *AFile, UnicodeString ANewName, bool CheckExistence);
  void TerminalMoveFile(UnicodeString AFileName, const TRemoteFile *AFile,
    /*const TMoveFileParams*/ void *Param);
  bool MoveFiles(TStrings *AFileList, UnicodeString Target,
    UnicodeString FileMask);
  void TerminalCopyFile(UnicodeString AFileName, const TRemoteFile *AFile,
    /*const TMoveFileParams*/ void *Param);
  bool CopyFiles(const TStrings *AFileList, UnicodeString Target,
    UnicodeString FileMask);
  bool CalculateFilesSize(const TStrings *AFileList, int64_t &Size,
    intptr_t Params, const TCopyParamType *CopyParam, bool AllowDirs,
    TCalculateSizeStats &Stats);
  void CalculateFilesChecksum(UnicodeString Alg, TStrings *AFileList,
    TStrings *Checksums, TCalculatedChecksumEvent OnCalculatedChecksum);
  void ClearCaches();
  TSynchronizeChecklist *SynchronizeCollect(UnicodeString LocalDirectory,
    UnicodeString RemoteDirectory, TSynchronizeMode Mode,
    const TCopyParamType *CopyParam, intptr_t Params,
    TSynchronizeDirectoryEvent OnSynchronizeDirectory, TSynchronizeOptions *Options);
  void SynchronizeApply(TSynchronizeChecklist *Checklist,
    UnicodeString LocalDirectory, UnicodeString RemoteDirectory,
    const TCopyParamType *CopyParam, intptr_t Params,
    TSynchronizeDirectoryEvent OnSynchronizeDirectory);
  void FilesFind(UnicodeString Directory, const TFileMasks &FileMask,
    TFileFoundEvent OnFileFound, TFindingFileEvent OnFindingFile);
  void SpaceAvailable(UnicodeString APath, TSpaceAvailable &ASpaceAvailable);
  void LockFiles(TStrings *AFileList);
  void UnlockFiles(TStrings *AFileList);
  TRemoteFileList *DirectoryFileList(UnicodeString APath, TDateTime Timestamp, bool CanLoad);
  void MakeLocalFileList(UnicodeString AFileName,
    const TSearchRec &Rec, void *Param);
  bool FileOperationLoopQuery(Exception &E,
    TFileOperationProgressType *OperationProgress, UnicodeString Message,
    bool AllowSkip, UnicodeString SpecialRetry = L"", UnicodeString HelpKeyword = L"");
  TUsableCopyParamAttrs UsableCopyParamAttrs(intptr_t Params) const;
  bool ContinueReopen(TDateTime Start) const;
  bool QueryReopen(Exception *E, intptr_t Params,
    TFileOperationProgressType *OperationProgress);
  UnicodeString PeekCurrentDirectory();
  void FatalAbort();
  void ReflectSettings() const;
  void CollectUsage();
  bool IsThisOrChild(TTerminal *Terminal) const;
  TTerminal *CreateSecondarySession(UnicodeString Name, TSessionData *SessionData);
  void FillSessionDataForCode(TSessionData *SessionData) const;

  const TSessionInfo &GetSessionInfo() const;
  const TFileSystemInfo &GetFileSystemInfo(bool Retrieve = false);
  void LogEvent(UnicodeString Str);
  void GetSupportedChecksumAlgs(TStrings *Algs) const;
  UnicodeString ChangeFileName(const TCopyParamType *CopyParam,
    UnicodeString AFileName, TOperationSide Side, bool FirstLevel) const;
  UnicodeString GetBaseFileName(UnicodeString AFileName) const;

  static UnicodeString ExpandFileName(UnicodeString APath,
    UnicodeString BasePath);

#if 0
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
  __property TCustomCommandEvent OnCustomCommand = { read = FOnCustomCommand, write = FOnCustomCommand };
  __property TNotifyEvent OnClose = { read = FOnClose, write = FOnClose };
  __property int TunnelLocalPortNumber = { read = FTunnelLocalPortNumber };
#endif

  bool GetIsCapable(TFSCapability Capability) const { return GetIsCapableProtected(Capability); }
  void SetMasks(UnicodeString Value);

  void SetLocalFileTime(UnicodeString LocalFileName,
    const TDateTime &Modification);
  void SetLocalFileTime(UnicodeString LocalFileName,
    FILETIME *AcTime, FILETIME *WrTime);
  DWORD GetLocalFileAttributes(UnicodeString LocalFileName) const;
  bool SetLocalFileAttributes(UnicodeString LocalFileName, DWORD FileAttributes);
  bool MoveLocalFile(UnicodeString LocalFileName, UnicodeString NewLocalFileName, DWORD Flags);
  bool RemoveLocalDirectory(UnicodeString LocalDirName);
  bool CreateLocalDirectory(UnicodeString LocalDirName, LPSECURITY_ATTRIBUTES SecurityAttributes);

  TSessionData *GetSessionData() const { return FSessionData; }
  TSessionData *GetSessionData() { return FSessionData; }
  TSessionLog *GetLog() const { return FLog; }
  TSessionLog *GetLog() { return FLog; }
  TActionLog *GetActionLog() const { return FActionLog; }
  const TConfiguration *GetConfiguration() const { return FConfiguration; }
  TConfiguration *GetConfiguration() { return FConfiguration; }
  TSessionStatus GetStatus() const { return FStatus; }
  TRemoteDirectory *GetFiles() const { return FFiles; }
  TNotifyEvent GetOnChangeDirectory() const { return FOnChangeDirectory; }
  void SetOnChangeDirectory(TNotifyEvent Value) { FOnChangeDirectory = Value; }
  TReadDirectoryEvent GetOnReadDirectory() const { return FOnReadDirectory; }
  void SetOnReadDirectory(TReadDirectoryEvent Value) { FOnReadDirectory = Value; }
  TNotifyEvent GetOnStartReadDirectory() const { return FOnStartReadDirectory; }
  void SetOnStartReadDirectory(TNotifyEvent Value) { FOnStartReadDirectory = Value; }
  TReadDirectoryProgressEvent GetOnReadDirectoryProgress() const { return FOnReadDirectoryProgress; }
  void SetOnReadDirectoryProgress(TReadDirectoryProgressEvent Value) { FOnReadDirectoryProgress = Value; }
  TDeleteLocalFileEvent GetOnDeleteLocalFile() const { return FOnDeleteLocalFile; }
  void SetOnDeleteLocalFile(TDeleteLocalFileEvent Value) { FOnDeleteLocalFile = Value; }
  TNotifyEvent GetOnInitializeLog() const { return FOnInitializeLog; }
  void SetOnInitializeLog(TNotifyEvent Value) { FOnInitializeLog = Value; }
  TCreateLocalFileEvent GetOnCreateLocalFile() const { return FOnCreateLocalFile; }
  void SetOnCreateLocalFile(TCreateLocalFileEvent Value) { FOnCreateLocalFile = Value; }
  TGetLocalFileAttributesEvent GetOnGetLocalFileAttributes() const { return FOnGetLocalFileAttributes; }
  void SetOnGetLocalFileAttributes(TGetLocalFileAttributesEvent Value) { FOnGetLocalFileAttributes = Value; }
  TSetLocalFileAttributesEvent GetOnSetLocalFileAttributes() const { return FOnSetLocalFileAttributes; }
  void SetOnSetLocalFileAttributes(TSetLocalFileAttributesEvent Value) { FOnSetLocalFileAttributes = Value; }
  TMoveLocalFileEvent GetOnMoveLocalFile() const { return FOnMoveLocalFile; }
  void SetOnMoveLocalFile(TMoveLocalFileEvent Value) { FOnMoveLocalFile = Value; }
  TRemoveLocalDirectoryEvent GetOnRemoveLocalDirectory() const { return FOnRemoveLocalDirectory; }
  void SetOnRemoveLocalDirectory(TRemoveLocalDirectoryEvent Value) { FOnRemoveLocalDirectory = Value; }
  TCreateLocalDirectoryEvent GetOnCreateLocalDirectory() const { return FOnCreateLocalDirectory; }
  void SetOnCreateLocalDirectory(TCreateLocalDirectoryEvent Value) { FOnCreateLocalDirectory = Value; }
  TFileOperationProgressEvent GetOnProgress() const { return FOnProgress; }
  void SetOnProgress(TFileOperationProgressEvent Value) { FOnProgress = Value; }
  TFileOperationFinishedEvent GetOnFinished() const { return FOnFinished; }
  void SetOnFinished(TFileOperationFinishedEvent Value) { FOnFinished = Value; }
  TCurrentFSProtocol GetFSProtocol() const { return FFSProtocol; }
  bool GetUseBusyCursor() const { return FUseBusyCursor; }
  void SetUseBusyCursor(bool Value) { FUseBusyCursor = Value; }
  bool GetAutoReadDirectory() const { return FAutoReadDirectory; }
  void SetAutoReadDirectory(bool Value) { FAutoReadDirectory = Value; }
  TQueryUserEvent GetOnQueryUser() const { return FOnQueryUser; }
  void SetOnQueryUser(TQueryUserEvent Value) { FOnQueryUser = Value; }
  TPromptUserEvent GetOnPromptUser() const { return FOnPromptUser; }
  void SetOnPromptUser(TPromptUserEvent Value) { FOnPromptUser = Value; }
  TDisplayBannerEvent GetOnDisplayBanner() const { return FOnDisplayBanner; }
  void SetOnDisplayBanner(TDisplayBannerEvent Value) { FOnDisplayBanner = Value; }
  TExtendedExceptionEvent GetOnShowExtendedException() const { return FOnShowExtendedException; }
  void SetOnShowExtendedException(TExtendedExceptionEvent Value) { FOnShowExtendedException = Value; }
  TInformationEvent GetOnInformation() const { return FOnInformation; }
  void SetOnInformation(TInformationEvent Value) { FOnInformation = Value; }
  TCustomCommandEvent GetOnCustomCommand() const { return FOnCustomCommand; }
  void SetOnCustomCommand(TCustomCommandEvent Value) { FOnCustomCommand = Value; }
  TCheckForEscEvent GetOnCheckForEsc() const { return FOnCheckForEsc; }
  void SetOnCheckForEsc(TCheckForEscEvent Value) { FOnCheckForEsc = Value; }
  TNotifyEvent GetOnClose() const { return FOnClose; }
  void SetOnClose(TNotifyEvent Value) { FOnClose = Value; }
  intptr_t GetTunnelLocalPortNumber() const { return FTunnelLocalPortNumber; }
  void SetRememberedPassword(UnicodeString Value) { FRememberedPassword = Value; }
  void SetRememberedTunnelPassword(UnicodeString Value) { FRememberedTunnelPassword = Value; }
  void SetTunnelPassword(UnicodeString Value) { FRememberedTunnelPassword = Value; }
  TCustomFileSystem *GetFileSystem() const { return FFileSystem; }
  TCustomFileSystem *GetFileSystem() { return FFileSystem; }

  bool CheckForEsc() const;
  void SetupTunnelLocalPortNumber();

private:
  void InternalTryOpen();
  void InternalDoTryOpen();
  void InitFileSystem();
};

class NB_CORE_EXPORT TSecondaryTerminal : public TTerminal
{
  NB_DISABLE_COPY(TSecondaryTerminal)
public:
  static inline bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TSecondaryTerminal); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TSecondaryTerminal) || TTerminal::is(Kind); }
public:
  TSecondaryTerminal() : TTerminal(OBJECT_CLASS_TSecondaryTerminal), FMainTerminal(nullptr) {}
  explicit TSecondaryTerminal(TTerminal *MainTerminal);
  explicit TSecondaryTerminal(TObjectClassId Kind, TTerminal *MainTerminal);
  virtual ~TSecondaryTerminal() {}
  void Init(TSessionData *ASessionData, TConfiguration *AConfiguration,
    UnicodeString Name);

  void UpdateFromMain();

#if 0
  __property TTerminal * MainTerminal = { read = FMainTerminal };
#endif
  TTerminal *GetMainTerminal() const { return FMainTerminal; }

protected:
  virtual void DirectoryLoaded(TRemoteFileList *FileList) override;
  virtual void DirectoryModified(UnicodeString APath,
    bool SubDirs) override;
  virtual const TTerminal *GetPasswordSource() const override { return FMainTerminal; }
  virtual TTerminal *GetPasswordSource() override;

private:
  TTerminal *FMainTerminal;
};

class NB_CORE_EXPORT TTerminalList : public TObjectList
{
  NB_DISABLE_COPY(TTerminalList)
public:
  explicit TTerminalList(TConfiguration *AConfiguration);
  virtual ~TTerminalList();

  virtual TTerminal *NewTerminal(TSessionData *Data);
  virtual void FreeTerminal(TTerminal *Terminal);
  void FreeAndNullTerminal(TTerminal *&Terminal);
  virtual void Idle();
  void RecryptPasswords();
#if 0
  __property TTerminal *Terminals[int Index]  = { read = GetTerminal };
#endif

protected:
  virtual TTerminal *CreateTerminal(TSessionData *Data);

private:
  TConfiguration *FConfiguration;

public:
  TTerminal *GetTerminal(intptr_t Index);
};

struct NB_CORE_EXPORT TCustomCommandParams : public TObject
{
public:
  static inline bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TCustomCommandParams); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TCustomCommandParams) || TObject::is(Kind); }
public:
  TCustomCommandParams() : TObject(OBJECT_CLASS_TCustomCommandParams), Params(0) {}
  UnicodeString Command;
  intptr_t Params;
  TCaptureOutputEvent OutputEvent;
};

struct NB_CORE_EXPORT TCalculateSizeStats : public TObject
{
  TCalculateSizeStats();

  intptr_t Files;
  intptr_t Directories;
  intptr_t SymLinks;
  TStrings *FoundFiles;
};

struct NB_CORE_EXPORT TCalculateSizeParams : public TObject
{
public:
  static inline bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TCalculateSizeParams); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TCalculateSizeParams) || TObject::is(Kind); }
public:
  TCalculateSizeParams() : TObject(OBJECT_CLASS_TCalculateSizeParams), Size(0), Params(0), CopyParam(nullptr), Stats(nullptr), Files(nullptr), AllowDirs(false), Result(false) {}
  int64_t Size;
  intptr_t Params;
  const TCopyParamType *CopyParam;
  TCalculateSizeStats *Stats;
  TCollectedFileList *Files;
  UnicodeString LastDirPath;
  bool AllowDirs;
  bool Result;
};

#if 0
struct TOverwriteFileParams
{
  TOverwriteFileParams();

  int64_t SourceSize;
  int64_t DestSize;
  TDateTime SourceTimestamp;
  TDateTime DestTimestamp;
  TModificationFmt SourcePrecision;
  TModificationFmt DestPrecision;
};
#endif

typedef rde::vector<TDateTime> TDateTimes;

struct NB_CORE_EXPORT TMakeLocalFileListParams : public TObject
{
public:
  static inline bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TMakeLocalFileListParams); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TMakeLocalFileListParams) || TObject::is(Kind); }
public:
  TMakeLocalFileListParams() : TObject(OBJECT_CLASS_TMakeLocalFileListParams), FileList(nullptr), FileTimes(nullptr), IncludeDirs(false), Recursive(false) {}
  TStrings *FileList;
  TDateTimes *FileTimes;
  bool IncludeDirs;
  bool Recursive;
};

struct NB_CORE_EXPORT TSynchronizeOptions : public TObject
{
  NB_DISABLE_COPY(TSynchronizeOptions)
public:
  TSynchronizeOptions();
  ~TSynchronizeOptions();

  TStringList *Filter;

  bool FilterFind(UnicodeString AFileName) const;
  bool MatchesFilter(UnicodeString AFileName) const;
};

enum TChecklistAction
{
  saNone,
  saUploadNew,
  saDownloadNew,
  saUploadUpdate,
  saDownloadUpdate,
  saDeleteRemote,
  saDeleteLocal,
};


class NB_CORE_EXPORT TChecklistItem : public TObject
{
  friend class TTerminal;
  NB_DISABLE_COPY(TChecklistItem)
public:
  static inline bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TChecklistItem); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TChecklistItem) || TObject::is(Kind); }
public:

  struct NB_CORE_EXPORT TFileInfo : public TObject
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
  TRemoteFile *RemoteFile;

  UnicodeString GetFileName() const;

  ~TChecklistItem();

private:
  FILETIME FLocalLastWriteTime;

  TChecklistItem();
};

class NB_CORE_EXPORT TSynchronizeChecklist : public TObject
{
  NB_DISABLE_COPY(TSynchronizeChecklist)
  friend class TTerminal;

public:
#if 0
  enum TAction { saNone, saUploadNew, saDownloadNew, saUploadUpdate,
    saDownloadUpdate, saDeleteRemote, saDeleteLocal
  };
#endif // #if 0
  static const intptr_t ActionCount = saDeleteLocal;

#if 0
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
    TRemoteFile *RemoteFile;

    const UnicodeString &GetFileName() const;

    ~TItem();

  private:
    FILETIME FLocalLastWriteTime;

    TItem();
  };
#endif // #if 0

  ~TSynchronizeChecklist();

  void Update(const TChecklistItem *Item, bool Check, TChecklistAction Action);

  static TChecklistAction Reverse(TChecklistAction Action);

#if 0
  __property int Count = { read = GetCount };
  __property const TItem * Item[int Index] = { read = GetItem };
#endif

protected:
  TSynchronizeChecklist();

  void Sort();
  void Add(TChecklistItem *Item);

public:
  void SetMasks(UnicodeString Value);

  intptr_t GetCount() const;
  const TChecklistItem *GetItem(intptr_t Index) const;

private:
  TList FList;

  static intptr_t Compare(const void *AItem1, const void *AItem2);
};

struct NB_CORE_EXPORT TSpaceAvailable
{
  CUSTOM_MEM_ALLOCATION_IMPL
  TSpaceAvailable();

  int64_t BytesOnDevice;
  int64_t UnusedBytesOnDevice;
  int64_t BytesAvailableToUser;
  int64_t UnusedBytesAvailableToUser;
  uintptr_t BytesPerAllocationUnit;
};

class NB_CORE_EXPORT TRobustOperationLoop : public TObject
{
  NB_DISABLE_COPY(TRobustOperationLoop)
public:
  explicit TRobustOperationLoop(TTerminal *Terminal, TFileOperationProgressType *OperationProgress, bool *AnyTransfer = nullptr);
  ~TRobustOperationLoop();
  bool TryReopen(Exception &E);
  bool ShouldRetry() const;
  bool Retry();

private:
  TTerminal *FTerminal;
  TFileOperationProgressType *FOperationProgress;
  bool FRetry;
  bool *FAnyTransfer;
  bool FPrevAnyTransfer;
  TDateTime FStart;
};

class TCollectedFileList : public TObject
{
public:
  static inline bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TCollectedFileList); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TCollectedFileList) || TObject::is(Kind); }
public:
  TCollectedFileList();
  intptr_t Add(UnicodeString FileName, TObject *Object, bool Dir);
  void DidNotRecurse(intptr_t Index);
  void Delete(intptr_t Index);

  intptr_t GetCount() const;
  UnicodeString GetFileName(intptr_t Index) const;
  TObject *GetObj(intptr_t Index) const;
  bool IsDir(intptr_t Index) const;
  bool IsRecursed(intptr_t Index) const;

private:
  struct TFileData
  {
    CUSTOM_MEM_ALLOCATION_IMPL
    UnicodeString FileName;
    TObject *Object;
    bool Dir;
    bool Recursed;
  };
  typedef rde::vector<TFileData> TFileDataList;
  TFileDataList FList;
};

class TParallelOperation : public TObject
{
public:
  explicit TParallelOperation(TOperationSide Side);
  ~TParallelOperation();

  void Init(
    TStrings *AFiles, UnicodeString TargetDir, const TCopyParamType *CopyParam, intptr_t Params,
    TFileOperationProgressType *MainOperationProgress, UnicodeString MainName);

  bool IsInitialized() const;
  void WaitFor();
  bool ShouldAddClient() const;
  void AddClient();
  void RemoveClient();
  intptr_t GetNext(
    TTerminal *Terminal, UnicodeString &FileName, TObject *&Object, UnicodeString &TargetDir,
    bool &Dir, bool &Recursed);
  void Done(UnicodeString FileName, bool Dir, bool Success);

#if 0
  __property TOperationSide Side = { read = FSide };
  __property const TCopyParamType * CopyParam = { read = FCopyParam };
  __property int Params = { read = FParams };
  __property UnicodeString TargetDir = { read = FTargetDir };
  __property TFileOperationProgressType * MainOperationProgress = { read = FMainOperationProgress };
  __property UnicodeString MainName = { read = FMainName };
#endif

  TOperationSide GetSide() const { return FSide; }
  const TCopyParamType *GetCopyParam() const { return FCopyParam; }
  intptr_t GetParams() const { return FParams; }
  UnicodeString GetTargetDir() const { return FTargetDir; }
  TFileOperationProgressType *GetMainOperationProgress() const { return FMainOperationProgress; }
  UnicodeString GetMainName() const { return FMainName; }

private:
  struct TDirectoryData
  {
    CUSTOM_MEM_ALLOCATION_IMPL
    UnicodeString OppositePath;
    bool Exists;
  };

  std::unique_ptr<TStrings> FFileList;
  intptr_t FIndex;
  typedef rde::map<UnicodeString, TDirectoryData> TDirectories;
  TDirectories FDirectories;
  UnicodeString FTargetDir;
  const TCopyParamType *FCopyParam;
  intptr_t FParams;
  bool FProbablyEmpty;
  intptr_t FClients;
  std::unique_ptr<TCriticalSection> FSection;
  TFileOperationProgressType *FMainOperationProgress;
  TOperationSide FSide;
  UnicodeString FMainName;

  bool CheckEnd(TCollectedFileList *Files);
};

NB_CORE_EXPORT UnicodeString GetSessionUrl(const TTerminal *Terminal, bool WithUserName = false);

