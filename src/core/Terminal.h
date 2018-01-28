//---------------------------------------------------------------------------
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
class TParallelOperation;
class TCollectedFileList;
struct TLocalFileHandle;
//---------------------------------------------------------------------------
#if 0
typedef void __fastcall (__closure *TQueryUserEvent)
  (TObject * Sender, const UnicodeString Query, TStrings * MoreMessages, unsigned int Answers,
   const TQueryParams * Params, unsigned int & Answer, TQueryType QueryType, void * Arg);
#endif // #if 0
typedef nb::FastDelegate8<void,
  TObject * /*Sender*/, UnicodeString /*Query*/, TStrings * /*MoreMessages*/, uintptr_t /*Answers*/,
   const TQueryParams * /*Params*/, uintptr_t & /*Answer*/, TQueryType /*QueryType*/, void * /*Arg*/> TQueryUserEvent;
#if 0
typedef void __fastcall (__closure *TPromptUserEvent)
  (TTerminal * Terminal, TPromptKind Kind, UnicodeString Name, UnicodeString Instructions,
   TStrings * Prompts, TStrings * Results, bool & Result, void * Arg);
#endif // #if 0
typedef nb::FastDelegate8<void,
  TTerminal * /*Terminal*/, TPromptKind /*Kind*/, UnicodeString /*Name*/, UnicodeString /*Instructions*/,
  TStrings * /*Prompts*/, TStrings * /*Results*/, bool & /*Result*/, void * /*Arg*/> TPromptUserEvent;
#if 0
typedef void __fastcall (__closure *TDisplayBannerEvent)
  (TTerminal * Terminal, UnicodeString SessionName, const UnicodeString & Banner,
   bool & NeverShowAgain, int Options, unsigned int & Params);
#endif // #if 0
typedef nb::FastDelegate6<void,
  TTerminal * /*Terminal*/, UnicodeString /*SessionName*/, UnicodeString /*Banner*/,
  bool & /*NeverShowAgain*/, intptr_t /*Options*/, uintptr_t & /*Params*/> TDisplayBannerEvent;
#if 0
typedef void __fastcall (__closure *TExtendedExceptionEvent)
  (TTerminal * Terminal, Exception * E, void * Arg);
#endif // #if 0
typedef nb::FastDelegate3<void,
  TTerminal * /*Terminal*/, Exception * /*E*/, void * /*Arg*/ > TExtendedExceptionEvent;
#if 0
typedef void __fastcall (__closure *TReadDirectoryEvent)(System::TObject * Sender, Boolean ReloadOnly);
#endif // #if 0
typedef nb::FastDelegate2<void, TObject * /*Sender*/,
  Boolean /*ReloadOnly*/> TReadDirectoryEvent;
#if 0
typedef void __fastcall (__closure *TReadDirectoryProgressEvent)(
  System::TObject* Sender, int Progress, int ResolvedLinks, bool & Cancel);
#endif // #if 0
typedef nb::FastDelegate4<void,
  TObject * /*Sender*/, intptr_t /*Progress*/, intptr_t /*ResolvedLinks*/, bool & /*Cancel*/> TReadDirectoryProgressEvent;
#if 0
typedef void __fastcall (__closure *TProcessFileEvent)
  (const UnicodeString FileName, const TRemoteFile * File, void * Param);
#endif // #if 0
typedef nb::FastDelegate3<void,
  const UnicodeString & /*AFileName*/, const TRemoteFile * /*AFile*/, void * /*AParam*/> TProcessFileEvent;
#if 0
typedef void __fastcall (__closure *TProcessFileEventEx)
  (const UnicodeString FileName, const TRemoteFile * File, void * Param, int Index);
#endif // #if 0
typedef nb::FastDelegate4<void,
  const UnicodeString /*AFileName*/, const TRemoteFile * /*File*/,
  void * /*Param*/, intptr_t /*Index*/> TProcessFileEventEx;
#if 0
typedef int __fastcall (__closure *TFileOperationEvent)
  (void * Param1, void * Param2);
#endif // #if 0
typedef nb::FastDelegate2<intptr_t,
  void * /*Param1*/, void * /*Param2*/> TFileOperationEvent;
#if 0
typedef void __fastcall (__closure *TSynchronizeDirectory)
  (const UnicodeString LocalDirectory, const UnicodeString RemoteDirectory,
   bool & Continue, bool Collect);
#endif // #if 0
typedef nb::FastDelegate4<void,
  UnicodeString /*LocalDirectory*/, UnicodeString /*RemoteDirectory*/,
  bool & /*Continue*/, bool /*Collect*/> TSynchronizeDirectoryEvent;
#if 0
typedef void __fastcall (__closure *TDeleteLocalFileEvent)(
  const UnicodeString FileName, bool Alternative);
#endif // #if 0
typedef nb::FastDelegate2<void,
  UnicodeString /*FileName*/, bool /*Alternative*/> TDeleteLocalFileEvent;
#if 0
typedef int __fastcall (__closure *TDirectoryModifiedEvent)
  (TTerminal * Terminal, const UnicodeString Directory, bool SubDirs);
#endif // #if 0
typedef nb::FastDelegate3<int,
  TTerminal * /*Terminal*/, const UnicodeString /*Directory*/, bool /*SubDirs*/> TDirectoryModifiedEvent;
#if 0
typedef void __fastcall (__closure *TInformationEvent)
  (TTerminal * Terminal, const UnicodeString Str, bool Status, int Phase);
#endif // #if 0
typedef nb::FastDelegate4<void,
  TTerminal * /*Terminal*/, UnicodeString /*Str*/, bool /*Status*/, intptr_t /*Phase*/> TInformationEvent;
#if 0
typedef void __fastcall (__closure *TCustomCommandEvent)
  (TTerminal * Terminal, const UnicodeString Command, bool & Handled);
#endif // #if 0
typedef nb::FastDelegate3<void,
  TTerminal * /*Terminal*/, const UnicodeString /*Command*/, bool & /*Handled*/> TCustomCommandEvent;

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
//---------------------------------------------------------------------------
const unsigned int folNone = 0x00;
const unsigned int folAllowSkip = 0x01;
const unsigned int folRetryOnFatal = 0x02;

#if 0

/* TODO : Better user interface (query to user) */
#define FILE_OPERATION_LOOP_BEGIN \
  { \
    bool DoRepeat; \
    do \
    { \
      DoRepeat = false; \
      try \

#define FILE_OPERATION_LOOP_END_CUSTOM(MESSAGE, FLAGS, HELPKEYWORD) \
      catch (Exception & E) \
      { \
        FILE_OPERATION_LOOP_TERMINAL->FileOperationLoopEnd(E, OperationProgress, MESSAGE, FLAGS, L"", HELPKEYWORD); \
        DoRepeat = true; \
      } \
    } while (DoRepeat); \
  }

#define FILE_OPERATION_LOOP_END_EX(MESSAGE, FLAGS) \
  FILE_OPERATION_LOOP_END_CUSTOM(MESSAGE, FLAGS, L"")
#define FILE_OPERATION_LOOP_END(MESSAGE) \
  FILE_OPERATION_LOOP_END_EX(MESSAGE, folAllowSkip)
#endif // #if 0
//---------------------------------------------------------------------------
inline void ThrowSkipFile(Exception *Exception, UnicodeString Message)
{
  throw ESkipFile(Exception, Message);
}
inline void ThrowSkipFileNull() { ThrowSkipFile(nullptr, L""); }

NB_CORE_EXPORT void FileOperationLoopCustom(TTerminal *Terminal,
  TFileOperationProgressType *OperationProgress,
  uintptr_t Flags, const UnicodeString Message,
  const UnicodeString HelpKeyword,
  const std::function<void()> &Operation);
//---------------------------------------------------------------------------
enum TCurrentFSProtocol { cfsUnknown, cfsSCP, cfsSFTP, cfsFTP, cfsFTPS, cfsWebDAV, cfsS3 };
//---------------------------------------------------------------------------
const int cpDelete = 0x01;
const int cpTemporary = 0x04;
const int cpNoConfirmation = 0x08;
const int cpNewerOnly = 0x10;
const int cpAppend = 0x20;
const int cpResume = 0x40;
const int cpNoRecurse = 0x80;
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
const int bpMonospacedFont = 0x01;
//---------------------------------------------------------------------------
const int tfNone = 0x00;
const int tfFirstLevel = 0x01;
const int tfNewDirectory = 0x02;
const int tfAutoResume = 0x04;
const int tfPreCreateDir = 0x08;
const int tfUseFileTransferAny = 0x10;
//---------------------------------------------------------------------------
class NB_CORE_EXPORT TTerminal : public TSessionUI
{
  NB_DISABLE_COPY(TTerminal)
public:
  static inline bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TTerminal); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TTerminal) || TSessionUI::is(Kind); }
public:
  // TScript::SynchronizeProc relies on the order
  __removed enum TSynchronizeMode { smRemote, smLocal, smBoth };
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
friend class TS3FileSystem;
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
  UnicodeString FFingerprintScannedSHA256;
  UnicodeString FFingerprintScannedMD5;
  DWORD FLastProgressLogged;
  TRemoteDirectory *FOldFiles;
  UnicodeString FDestFileName;
  bool FMultipleDestinationFiles;
  bool FFileTransferAny;

public:
  void __fastcall CommandError(Exception *E, const UnicodeString AMsg);
  uintptr_t __fastcall CommandError(Exception *E, const UnicodeString AMsg,
    uintptr_t Answers, const UnicodeString HelpKeyword = L"");
  UnicodeString __fastcall RemoteGetCurrentDirectory();
  bool __fastcall GetExceptionOnFail() const;
  const TRemoteTokenList * __fastcall GetGroups() const { return const_cast<TTerminal *>(this)->GetGroups(); }
  const TRemoteTokenList * __fastcall GetUsers() const { return const_cast<TTerminal *>(this)->GetUsers(); }
  const TRemoteTokenList * __fastcall GetMembership() const { return const_cast<TTerminal *>(this)->GetMembership(); }
  void __fastcall TerminalSetCurrentDirectory(UnicodeString AValue);
  void __fastcall SetExceptionOnFail(bool Value);
  void __fastcall ReactOnCommand(intptr_t /*TFSCommand*/ ACmd);
  UnicodeString __fastcall TerminalGetUserName() const;
  bool __fastcall GetAreCachesEmpty() const;
  void __fastcall ClearCachedFileList(const UnicodeString APath, bool SubDirs);
  void __fastcall AddCachedFileList(TRemoteFileList *FileList);
  bool __fastcall GetCommandSessionOpened() const;
  TTerminal * __fastcall GetCommandSession();
  bool __fastcall GetResolvingSymlinks() const;
  bool __fastcall GetActive() const;
  UnicodeString __fastcall GetPassword() const;
  UnicodeString __fastcall GetRememberedPassword() const;
  UnicodeString __fastcall GetRememberedTunnelPassword() const;
  bool __fastcall GetStoredCredentialsTried() const;
  bool __fastcall InTransaction() const;
  void __fastcall SaveCapabilities(TFileSystemInfo &FileSystemInfo);
  void __fastcall CreateTargetDirectory(const UnicodeString DirectoryPath, uintptr_t Attrs, const TCopyParamType * CopyParam);
  static UnicodeString __fastcall SynchronizeModeStr(TSynchronizeMode Mode);
  static UnicodeString __fastcall SynchronizeParamsStr(intptr_t Params);

  TRemoteTokenList * GetGroups();
  TRemoteTokenList * GetUsers();
  TRemoteTokenList * GetMembership();

protected:
  bool FReadCurrentDirectoryPending;
  bool FReadDirectoryPending;
  bool FTunnelOpening;
  TCustomFileSystem *FFileSystem;

  void __fastcall DoStartReadDirectory();
  void __fastcall DoReadDirectoryProgress(intptr_t Progress, intptr_t ResolvedLinks, bool &Cancel);
  void __fastcall DoReadDirectory(bool ReloadOnly);
  void __fastcall DoCreateDirectory(const UnicodeString ADirName);
  void __fastcall DoDeleteFile(const UnicodeString AFileName, const TRemoteFile *AFile,
    intptr_t Params);
  void __fastcall DoCustomCommandOnFile(UnicodeString AFileName,
    const TRemoteFile *AFile, UnicodeString ACommand, intptr_t AParams, TCaptureOutputEvent OutputEvent);
  void __fastcall DoRenameFile(const UnicodeString AFileName, const TRemoteFile *AFile,
    const UnicodeString ANewName, bool Move);
  void __fastcall DoCopyFile(const UnicodeString AFileName, const TRemoteFile *AFile, const UnicodeString ANewName);
  void __fastcall DoChangeFileProperties(const UnicodeString AFileName,
    const TRemoteFile *AFile, const TRemoteProperties *Properties);
  void __fastcall DoChangeDirectory();
  void __fastcall DoInitializeLog();
  void __fastcall EnsureNonExistence(const UnicodeString AFileName);
  void __fastcall LookupUsersGroups();
  void __fastcall FileModified(const TRemoteFile *AFile,
    const UnicodeString AFileName, bool ClearDirectoryChange = false);
  intptr_t __fastcall FileOperationLoop(TFileOperationEvent CallBackFunc,
    TFileOperationProgressType * OperationProgress, uintptr_t AFlags,
    const UnicodeString Message, void *Param1 = nullptr, void *Param2 = nullptr);
  bool __fastcall GetIsCapableProtected(TFSCapability Capability) const;
  bool __fastcall ProcessFiles(TStrings *AFileList, TFileOperation Operation,
    TProcessFileEvent ProcessFile, void *Param = nullptr, TOperationSide Side = osRemote,
    bool Ex = false);
  bool __fastcall ProcessFilesEx(TStrings *FileList, TFileOperation Operation,
    TProcessFileEventEx ProcessFile, void *AParam = nullptr, TOperationSide Side = osRemote);
  void __fastcall ProcessDirectory(const UnicodeString ADirName,
    TProcessFileEvent CallBackFunc, void *AParam = nullptr, bool UseCache = false,
    bool IgnoreErrors = false);
  bool __fastcall DeleteContentsIfDirectory(
    const UnicodeString AFileName, const TRemoteFile *AFile, intptr_t AParams, TRmSessionAction &Action);
  void __fastcall AnnounceFileListOperation();
  UnicodeString __fastcall TranslateLockedPath(UnicodeString APath, bool Lock);
  void __fastcall ReadDirectory(TRemoteFileList *AFileList);
  void __fastcall CustomReadDirectory(TRemoteFileList *AFileList);
  void __fastcall DoCreateLink(const UnicodeString AFileName, const UnicodeString APointTo, bool Symbolic);
  bool __fastcall TerminalCreateLocalFile(const UnicodeString ATargetFileName,
    TFileOperationProgressType *OperationProgress,
    bool Resume,
    bool NoConfirmation,
    HANDLE *AHandle);
  HANDLE __fastcall TerminalCreateLocalFile(const UnicodeString LocalFileName, DWORD DesiredAccess,
    DWORD ShareMode, DWORD CreationDisposition, DWORD FlagsAndAttributes);
#if 0
  void __fastcall OpenLocalFile(const UnicodeString FileName, unsigned int Access,
    int * Attrs, HANDLE * Handle, __int64 * ACTime, __int64 * MTime,
    __int64 * ATime, __int64 * Size, bool TryWriteReadOnly = true);
#endif // #if 0
  void __fastcall TerminalOpenLocalFile(const UnicodeString ATargetFileName, DWORD Access,
    DWORD *AAttrs, HANDLE *AHandle, int64_t *ACTime,
    int64_t *AMTime, int64_t *AATime, int64_t *ASize,
    bool TryWriteReadOnly = true);
  void __fastcall TerminalOpenLocalFile(
    const UnicodeString AFileName, uintptr_t Access, TLocalFileHandle &AHandle, bool TryWriteReadOnly = true);
  bool __fastcall AllowLocalFileTransfer(const UnicodeString AFileName,
    const TCopyParamType *CopyParam, TFileOperationProgressType *OperationProgress);
  bool __fastcall HandleException(Exception *E);
  void __fastcall CalculateFileSize(const UnicodeString &AFileName,
    const TRemoteFile *AFile, /*TCalculateSizeParams*/ void *ASize);
  void __fastcall DoCalculateFileSize(const UnicodeString &AFileName,
    const TRemoteFile *AFile, void *AParam);
  bool __fastcall DoCalculateDirectorySize(const UnicodeString AFileName,
    const TRemoteFile *AFile, TCalculateSizeParams *Params);
  void __fastcall CalculateLocalFileSize(const UnicodeString AFileName,
    const TSearchRec &Rec, /*int64_t*/ void *Size);
  bool __fastcall CalculateLocalFilesSize(TStrings *AFileList,
    const TCopyParamType *CopyParam, bool AllowDirs, TStrings *Files,
    int64_t &Size);
  TBatchOverwrite __fastcall EffectiveBatchOverwrite(
    const UnicodeString ASourceFullFileName, const TCopyParamType *CopyParam, intptr_t Params,
    TFileOperationProgressType *OperationProgress, bool Special) const;
  bool __fastcall CheckRemoteFile(
    const UnicodeString AFileName, const TCopyParamType *CopyParam,
    intptr_t Params, TFileOperationProgressType *OperationProgress) const;
  uintptr_t __fastcall ConfirmFileOverwrite(
    const UnicodeString ASourceFullFileName, const UnicodeString ATargetFileName,
    const TOverwriteFileParams *FileParams, uintptr_t Answers, TQueryParams *QueryParams,
    TOperationSide Side, const TCopyParamType *CopyParam, intptr_t Params,
    TFileOperationProgressType *OperationProgress, UnicodeString AMessage = L"");
  void __fastcall DoSynchronizeCollectDirectory(const UnicodeString ALocalDirectory,
    const UnicodeString ARemoteDirectory, TSynchronizeMode Mode,
    const TCopyParamType *CopyParam, intptr_t Params,
    TSynchronizeDirectoryEvent OnSynchronizeDirectory,
    TSynchronizeOptions *Options, intptr_t Level, TSynchronizeChecklist *Checklist);
  void __fastcall DoSynchronizeCollectFile(const UnicodeString AFileName,
    const TRemoteFile *AFile, /*TSynchronizeData* */void * Param);
  void __fastcall SynchronizeCollectFile(const UnicodeString &AFileName,
    const TRemoteFile *AFile, /*TSynchronizeData* */ void *Param);
  void __fastcall SynchronizeRemoteTimestamp(const UnicodeString &AFileName,
    const TRemoteFile *AFile, void *Param);
  void __fastcall SynchronizeLocalTimestamp(const UnicodeString &AFileName,
    const TRemoteFile *AFile, void *Param);
  void __fastcall DoSynchronizeProgress(const TSynchronizeData &Data, bool Collect);
  void __fastcall DeleteLocalFile(const UnicodeString &AFileName,
    const TRemoteFile *AFile, void *Params);
  void __fastcall RecycleFile(UnicodeString AFileName, const TRemoteFile *AFile);
  TStrings * __fastcall GetFixedPaths() const;
  void __fastcall DoStartup();
  virtual bool __fastcall DoQueryReopen(Exception *E);
  virtual void __fastcall FatalError(Exception *E, const UnicodeString AMsg, const UnicodeString AHelpKeyword = L"") override;
  void ResetConnection();
  virtual bool __fastcall DoPromptUser(TSessionData *Data, TPromptKind Kind,
    const UnicodeString AName, UnicodeString Instructions, TStrings *Prompts,
    TStrings *Response);
  void __fastcall OpenTunnel();
  void __fastcall CloseTunnel();
  void __fastcall DoInformation(const UnicodeString AStr, bool Status, intptr_t Phase = -1);
  bool __fastcall PromptUser(TSessionData *Data, TPromptKind Kind,
    const UnicodeString AName, const UnicodeString Instructions, const UnicodeString Prompt, bool Echo,
    intptr_t MaxLen, UnicodeString &AResult);
  void __fastcall FileFind(const UnicodeString &AFileName, const TRemoteFile *AFile, void *Param);
  void __fastcall DoFilesFind(const UnicodeString ADirectory, TFilesFindParams &Params, const UnicodeString ARealDirectory);
  bool __fastcall DoCreateLocalFile(const UnicodeString AFileName,
    TFileOperationProgressType *OperationProgress,
    bool Resume,
    bool NoConfirmation,
    HANDLE *AHandle);
  void __fastcall LockFile(const UnicodeString &AFileName, const TRemoteFile *AFile, void *AParam);
  void __fastcall UnlockFile(const UnicodeString &AFileName, const TRemoteFile *AFile, void *AParam);
  void __fastcall DoLockFile(const UnicodeString &AFileName, const TRemoteFile *AFile);
  void __fastcall DoUnlockFile(const UnicodeString &AFileName, const TRemoteFile *AFile);

  virtual void __fastcall Information(const UnicodeString AStr, bool Status) override;
  virtual uintptr_t __fastcall QueryUser(const UnicodeString AQuery,
    TStrings *MoreMessages, uintptr_t Answers, const TQueryParams *Params,
    TQueryType QueryType = qtConfirmation) override;
  virtual uintptr_t __fastcall QueryUserException(const UnicodeString AQuery,
    Exception *E, uintptr_t Answers, const TQueryParams *Params,
    TQueryType QueryType = qtConfirmation) override;
  virtual bool __fastcall PromptUser(TSessionData *Data, TPromptKind Kind,
    UnicodeString AName, const UnicodeString Instructions, TStrings *Prompts, TStrings *Results) override;
  virtual void __fastcall DisplayBanner(const UnicodeString ABanner) override;
  virtual void __fastcall Closed() override;
  virtual void __fastcall ProcessGUI() override;
  void __fastcall Progress(TFileOperationProgressType *OperationProgress);
  virtual void __fastcall HandleExtendedException(Exception *E) override;
  bool __fastcall IsListenerFree(uintptr_t PortNumber) const;
  void __fastcall DoProgress(TFileOperationProgressType &ProgressData);
  void __fastcall DoFinished(TFileOperation Operation, TOperationSide Side, bool Temp,
    const UnicodeString AFileName, bool Success, TOnceDoneOperation &OnceDoneOperation);
  void __fastcall RollbackAction(TSessionAction &Action,
    TFileOperationProgressType *OperationProgress, Exception *E = nullptr);
  void __fastcall DoAnyCommand(const UnicodeString ACommand, TCaptureOutputEvent OutputEvent,
    TCallSessionAction *Action);
  TRemoteFileList * __fastcall DoReadDirectoryListing(const UnicodeString ADirectory, bool UseCache);
  RawByteString __fastcall EncryptPassword(const UnicodeString APassword) const;
  UnicodeString __fastcall DecryptPassword(const RawByteString APassword) const;
  UnicodeString __fastcall GetRemoteFileInfo(TRemoteFile *AFile) const;
  void __fastcall LogRemoteFile(TRemoteFile *AFile);
  UnicodeString __fastcall FormatFileDetailsForLog(const UnicodeString AFileName, const TDateTime &AModification, int64_t Size);
  void __fastcall LogFileDetails(const UnicodeString AFileName, const TDateTime &AModification, int64_t Size);
  void __fastcall LogFileDone(TFileOperationProgressType *OperationProgress, UnicodeString DestFileName);
  void __fastcall LogTotalTransferDetails(
    const UnicodeString ATargetDir, const TCopyParamType *CopyParam,
    TFileOperationProgressType *OperationProgress, bool Parallel, TStrings *AFiles);
  void LogTotalTransferDone(TFileOperationProgressType *OperationProgress);
  virtual TTerminal * __fastcall GetPasswordSource();
  void __fastcall DoEndTransaction(bool Inform);
  bool __fastcall VerifyCertificate(
    const UnicodeString CertificateStorageKey, const UnicodeString SiteKey,
    const UnicodeString Fingerprint,
    const UnicodeString CertificateSubject, int Failures);
  void __fastcall CacheCertificate(const UnicodeString CertificateStorageKey,
    const UnicodeString SiteKey, const UnicodeString Fingerprint, intptr_t Failures);
  bool __fastcall ConfirmCertificate(
    TSessionInfo & SessionInfo, intptr_t AFailures, const UnicodeString ACertificateStorageKey, bool CanRemember);
  void __fastcall CollectTlsUsage(const UnicodeString TlsVersionStr);
  bool __fastcall LoadTlsCertificate(X509 *& Certificate, EVP_PKEY *& PrivateKey);
  bool __fastcall TryStartOperationWithFile(
    const UnicodeString AFileName, TFileOperation Operation1, TFileOperation Operation2 = foNone);
  void __fastcall StartOperationWithFile(
    const UnicodeString AFileName, TFileOperation Operation1, TFileOperation Operation2 = foNone);
  bool __fastcall CanRecurseToDirectory(const TRemoteFile * File) const;
  bool __fastcall DoOnCustomCommand(const UnicodeString ACommand);
  bool __fastcall CanParallel(const TCopyParamType *CopyParam, intptr_t AParams, TParallelOperation *ParallelOperation) const;
  void __fastcall CopyParallel(TParallelOperation *ParallelOperation, TFileOperationProgressType *OperationProgress);
  void __fastcall DoCopyToRemote(
    TStrings *AFilesToCopy, const UnicodeString ATargetDir, const TCopyParamType *CopyParam, intptr_t AParams,
    TFileOperationProgressType *OperationProgress, uintptr_t AFlags, TOnceDoneOperation &OnceDoneOperation);
  void __fastcall SourceRobust(
    const UnicodeString AFileName, const UnicodeString ATargetDir, const TCopyParamType *CopyParam, intptr_t AParams,
    TFileOperationProgressType * OperationProgress, uintptr_t AFlags);
  void __fastcall Source(
    const UnicodeString AFileName, const UnicodeString ATargetDir, const TCopyParamType * CopyParam, intptr_t AParams,
    TFileOperationProgressType * OperationProgress, uintptr_t AFlags, TUploadSessionAction &Action, bool &ChildError);
  void __fastcall DirectorySource(
    const UnicodeString ADirectoryName, const UnicodeString ATargetDir, const UnicodeString ADestDirectoryName,
    uintptr_t Attrs, const TCopyParamType * CopyParam, intptr_t AParams,
    TFileOperationProgressType * OperationProgress, uintptr_t AFlags);
  void __fastcall SelectTransferMode(
    const UnicodeString ABaseFileName, TOperationSide Side, const TCopyParamType *CopyParam,
    const TFileMasks::TParams &MaskParams);
  void __fastcall SelectSourceTransferMode(const TLocalFileHandle &Handle, const TCopyParamType *CopyParam);
  void __fastcall UpdateSource(const TLocalFileHandle &Handle, const TCopyParamType * CopyParam, intptr_t AParams);
  void __fastcall DoCopyToLocal(
    TStrings * FilesToCopy, const UnicodeString ATargetDir, const TCopyParamType *CopyParam, intptr_t AParams,
    TFileOperationProgressType *OperationProgress, uintptr_t AFlags, TOnceDoneOperation &OnceDoneOperation);
  void __fastcall SinkRobust(
    const UnicodeString AFileName, const TRemoteFile *AFile, const UnicodeString ATargetDir,
    const TCopyParamType *CopyParam, intptr_t AParams, TFileOperationProgressType * OperationProgress, uintptr_t AFlags);
  void __fastcall Sink(
    const UnicodeString AFileName, const TRemoteFile *AFile, const UnicodeString ATargetDir,
    const TCopyParamType * CopyParam, intptr_t AParams, TFileOperationProgressType *OperationProgress, uintptr_t AFlags,
    TDownloadSessionAction &Action);
  void __fastcall SinkFile(const UnicodeString &AFileName, const TRemoteFile *AFile, void *Param);
  void __fastcall UpdateTargetAttrs(
    const UnicodeString ADestFullName, const TRemoteFile *AFile, const TCopyParamType *CopyParam, uintptr_t Attrs);
  void __fastcall UpdateTargetTime(HANDLE Handle, TDateTime Modification, TDSTMode DSTMode);

  __property TFileOperationProgressType *OperationProgress = { read = FOperationProgress };
  const TFileOperationProgressType *GetOperationProgress() const { return FOperationProgress; }
  TFileOperationProgressType *GetOperationProgress() { return FOperationProgress; }
  void SetOperationProgress(TFileOperationProgressType *OperationProgress) { FOperationProgress = OperationProgress; }
  virtual const TTerminal *GetPasswordSource() const { return this; }

public:
  explicit __fastcall TTerminal(TObjectClassId Kind = OBJECT_CLASS_TTerminal);
  void Init(TSessionData *SessionData, TConfiguration *Configuration);
  virtual __fastcall ~TTerminal();
  void __fastcall Open();
  void __fastcall Close();
  void __fastcall FingerprintScan(UnicodeString &SHA256, UnicodeString &MD5);
  void __fastcall Reopen(intptr_t Params);
  virtual void __fastcall DirectoryModified(const UnicodeString APath, bool SubDirs);
  virtual void __fastcall DirectoryLoaded(TRemoteFileList *FileList);
  void __fastcall ShowExtendedException(Exception *E);
  void __fastcall Idle();
  void __fastcall RecryptPasswords();
  bool __fastcall AllowedAnyCommand(const UnicodeString ACommand) const;
  void __fastcall AnyCommand(const UnicodeString ACommand, TCaptureOutputEvent OutputEvent);
  void __fastcall CloseOnCompletion(
    TOnceDoneOperation Operation = odoDisconnect, const UnicodeString AMessage = L"",
    const UnicodeString TargetLocalPath = L"", const UnicodeString ADestLocalFileName = L"");
  UnicodeString __fastcall GetAbsolutePath(const UnicodeString APath, bool Local) const;
  void __fastcall BeginTransaction();
  void __fastcall ReadCurrentDirectory();
  void __fastcall ReadDirectory(bool ReloadOnly, bool ForceCache = false);
  TRemoteFileList * __fastcall ReadDirectoryListing(UnicodeString Directory, const TFileMasks &Mask);
  TRemoteFileList * __fastcall CustomReadDirectoryListing(UnicodeString Directory, bool UseCache);
  TRemoteFile * __fastcall ReadFileListing(UnicodeString APath);
  void __fastcall ReadFile(const UnicodeString AFileName, TRemoteFile *&AFile);
  bool __fastcall FileExists(const UnicodeString AFileName, TRemoteFile **AFile = nullptr);
  void __fastcall ReadSymlink(TRemoteFile *SymlinkFile, TRemoteFile *&File);
  bool __fastcall CopyToLocal(TStrings *AFilesToCopy,
    const UnicodeString ATargetDir, const TCopyParamType *CopyParam, intptr_t AParams, TParallelOperation *ParallelOperation);
  bool __fastcall CopyToRemote(TStrings *AFilesToCopy,
    UnicodeString ATargetDir, const TCopyParamType *CopyParam, intptr_t Params, TParallelOperation *ParallelOperation);
  intptr_t __fastcall CopyToParallel(TParallelOperation *ParallelOperation, TFileOperationProgressType *OperationProgress);
  void __fastcall LogParallelTransfer(TParallelOperation *ParallelOperation);
  void __fastcall RemoteCreateDirectory(const UnicodeString ADirName,
    const TRemoteProperties *Properties = nullptr);
  void __fastcall RemoteCreateLink(const UnicodeString AFileName, const UnicodeString APointTo, bool Symbolic);
  void __fastcall RemoteDeleteFile(const UnicodeString &AFileName,
    const TRemoteFile *AFile = nullptr, void *AParams = nullptr);
  bool __fastcall RemoteDeleteFiles(TStrings *AFilesToDelete, intptr_t Params = 0);
  bool __fastcall DeleteLocalFiles(TStrings *AFileList, intptr_t Params = 0);
  bool __fastcall IsRecycledFile(const UnicodeString AFileName);
  void __fastcall CustomCommandOnFile(const UnicodeString &AFileName,
    const TRemoteFile *AFile, void *AParams);
  void __fastcall CustomCommandOnFiles(const UnicodeString ACommand, intptr_t AParams,
    TStrings *AFiles, TCaptureOutputEvent OutputEvent);
  void __fastcall RemoteChangeDirectory(const UnicodeString ADirectory);
  void __fastcall EndTransaction();
  void __fastcall HomeDirectory();
  void __fastcall ChangeFileProperties(const UnicodeString &AFileName,
    const TRemoteFile *AFile, /*const TRemoteProperties*/ void *Properties);
  void __fastcall ChangeFilesProperties(TStrings *AFileList,
    const TRemoteProperties *Properties);
  bool __fastcall LoadFilesProperties(TStrings *AFileList);
  void __fastcall TerminalError(const UnicodeString Msg);
  void __fastcall TerminalError(Exception *E, const UnicodeString AMsg, const UnicodeString AHelpKeyword = L"");
  void __fastcall ReloadDirectory();
  void __fastcall RefreshDirectory();
  void __fastcall TerminalRenameFile(const TRemoteFile *AFile, const UnicodeString ANewName, bool CheckExistence);
  void __fastcall TerminalMoveFile(const UnicodeString &AFileName, const TRemoteFile *AFile,
    /*const TMoveFileParams*/ void *Param);
  bool __fastcall TerminalMoveFiles(TStrings *AFileList, const UnicodeString ATarget,
    const UnicodeString AFileMask);
  void __fastcall TerminalCopyFile(const UnicodeString &AFileName, const TRemoteFile *AFile,
    /*const TMoveFileParams*/ void *Param);
  bool __fastcall CopyFiles(TStrings *AFileList, const UnicodeString ATarget,
    const UnicodeString AFileMask);
  bool __fastcall CalculateFilesSize(TStrings *AFileList, int64_t &Size,
    intptr_t Params, const TCopyParamType *CopyParam, bool AllowDirs,
    TCalculateSizeStats &Stats);
  void __fastcall CalculateFilesChecksum(const UnicodeString Alg, TStrings *AFileList,
    TStrings *Checksums, TCalculatedChecksumEvent OnCalculatedChecksum);
  void __fastcall ClearCaches();
  TSynchronizeChecklist * __fastcall SynchronizeCollect(const UnicodeString LocalDirectory,
    const UnicodeString ARemoteDirectory, TSynchronizeMode Mode,
    const TCopyParamType *CopyParam, intptr_t Params,
    TSynchronizeDirectoryEvent OnSynchronizeDirectory, TSynchronizeOptions *Options);
  void __fastcall SynchronizeApply(TSynchronizeChecklist *Checklist,
    const UnicodeString ALocalDirectory, const UnicodeString ARemoteDirectory,
    const TCopyParamType *CopyParam, intptr_t Params,
    TSynchronizeDirectoryEvent OnSynchronizeDirectory);
  void __fastcall FilesFind(UnicodeString Directory, const TFileMasks &FileMask,
    TFileFoundEvent OnFileFound, TFindingFileEvent OnFindingFile);
  void __fastcall SpaceAvailable(const UnicodeString APath, TSpaceAvailable &ASpaceAvailable);
  void __fastcall LockFiles(TStrings *AFileList);
  void __fastcall UnlockFiles(TStrings *AFileList);
  TRemoteFileList * __fastcall DirectoryFileList(const UnicodeString APath, TDateTime Timestamp, bool CanLoad);
  void __fastcall MakeLocalFileList(const UnicodeString AFileName,
    const TSearchRec &Rec, void *Param);
  bool __fastcall FileOperationLoopQuery(Exception &E,
    TFileOperationProgressType *OperationProgress, const UnicodeString Message,
    uintptr_t AFlags, UnicodeString SpecialRetry = L"", UnicodeString HelpKeyword = L"");
  void __fastcall FileOperationLoopEnd(Exception & E,
    TFileOperationProgressType * OperationProgress, const UnicodeString Message,
    uintptr_t AFlags, const UnicodeString SpecialRetry, const UnicodeString HelpKeyword);
  TUsableCopyParamAttrs __fastcall UsableCopyParamAttrs(intptr_t AParams) const;
  bool __fastcall ContinueReopen(TDateTime Start) const;
  bool __fastcall QueryReopen(Exception * E, intptr_t AParams,
    TFileOperationProgressType *OperationProgress);
  UnicodeString __fastcall PeekCurrentDirectory();
  void __fastcall FatalAbort();
  void __fastcall ReflectSettings() const;
  void __fastcall CollectUsage();
  bool __fastcall IsThisOrChild(TTerminal *Terminal) const;
  TTerminal * __fastcall CreateSecondarySession(const UnicodeString Name, TSessionData *SessionData);
  void __fastcall FillSessionDataForCode(TSessionData *SessionData) const;

  const TSessionInfo & __fastcall GetSessionInfo() const;
  const TFileSystemInfo & __fastcall GetFileSystemInfo(bool Retrieve = false);
  void __fastcall LogEvent(const UnicodeString AStr);
  void __fastcall GetSupportedChecksumAlgs(TStrings *Algs) const;
  UnicodeString __fastcall ChangeFileName(const TCopyParamType *CopyParam,
    const UnicodeString AFileName, TOperationSide Side, bool FirstLevel) const;
  UnicodeString __fastcall GetBaseFileName(const UnicodeString AFileName) const;

  static UnicodeString __fastcall ExpandFileName(const UnicodeString APath,
    const UnicodeString BasePath);

  __property TSessionData * SessionData = { read = FSessionData };
  // ROProperty<TSessionData *, TTerminal> SessionData{this, &TTerminal::GetSessionData};
  __property TSessionLog * Log = { read = FLog };
  // ROProperty<TSessionLog *, TTerminal> Log{this, &TTerminal::GetLog};
  __property TActionLog * ActionLog = { read = FActionLog };
  __property TConfiguration * Configuration = { read = FConfiguration };
  // ROProperty<TConfiguration *, TTerminal> Configuration{this, &TTerminal::GetConfiguration};
  __property bool Active = { read = GetActive };
  // ROProperty<bool, TTerminal> Active{this, &TTerminal::GetActive};
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

  bool GetIsCapable(TFSCapability Capability) const { return GetIsCapableProtected(Capability); }
  void SetMasks(UnicodeString Value);

  void SetLocalFileTime(const UnicodeString LocalFileName,
    const TDateTime &Modification);
  void SetLocalFileTime(const UnicodeString LocalFileName,
    FILETIME *AcTime, FILETIME *WrTime);
  DWORD GetLocalFileAttributes(const UnicodeString LocalFileName) const;
  bool SetLocalFileAttributes(const UnicodeString LocalFileName, DWORD FileAttributes);
  bool MoveLocalFile(const UnicodeString LocalFileName, const UnicodeString NewLocalFileName, DWORD Flags);
  bool RemoveLocalDirectory(const UnicodeString LocalDirName);
  bool CreateLocalDirectory(const UnicodeString LocalDirName, LPSECURITY_ATTRIBUTES SecurityAttributes);

  TSessionData *GetSessionData() const { return FSessionData; }
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
//---------------------------------------------------------------------------
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
    const UnicodeString Name);

  void UpdateFromMain();

  __property TTerminal * MainTerminal = { read = FMainTerminal };
  TTerminal *GetMainTerminal() const { return FMainTerminal; }

protected:
  virtual void DirectoryLoaded(TRemoteFileList *FileList) override;
  virtual void DirectoryModified(const UnicodeString APath,
    bool SubDirs) override;
  virtual const TTerminal *GetPasswordSource() const override { return FMainTerminal; }
  virtual TTerminal *GetPasswordSource() override;

private:
  TTerminal *FMainTerminal;
};
//---------------------------------------------------------------------------
class NB_CORE_EXPORT TTerminalList : public TObjectList
{
  NB_DISABLE_COPY(TTerminalList)
public:
  explicit __fastcall TTerminalList(TConfiguration *AConfiguration);
  virtual __fastcall ~TTerminalList();

  virtual TTerminal * __fastcall NewTerminal(TSessionData *Data);
  virtual void __fastcall FreeTerminal(TTerminal *Terminal);
  void __fastcall FreeAndNullTerminal(TTerminal *& Terminal);
  void __fastcall RecryptPasswords();

  __property TTerminal *Terminals[int Index]  = { read = GetTerminal };

protected:
  virtual TTerminal * __fastcall CreateTerminal(TSessionData *Data);

private:
  TConfiguration *FConfiguration;

public:
  TTerminal * __fastcall GetTerminal(intptr_t Index);
};
//---------------------------------------------------------------------------
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
//---------------------------------------------------------------------------
struct NB_CORE_EXPORT TCalculateSizeStats : public TObject
{
  TCalculateSizeStats();

  intptr_t Files;
  intptr_t Directories;
  intptr_t SymLinks;
  TStrings *FoundFiles;
};
//---------------------------------------------------------------------------
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
//---------------------------------------------------------------------------
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
//---------------------------------------------------------------------------
typedef rde::vector<TDateTime> TDateTimes;
//---------------------------------------------------------------------------
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
//---------------------------------------------------------------------------
struct NB_CORE_EXPORT TSynchronizeOptions : public TObject
{
  NB_DISABLE_COPY(TSynchronizeOptions)
public:
  TSynchronizeOptions();
  ~TSynchronizeOptions();

  TStringList *Filter;

  bool __fastcall FilterFind(const UnicodeString AFileName) const;
  bool __fastcall MatchesFilter(const UnicodeString AFileName) const;
};
//---------------------------------------------------------------------------
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
//---------------------------------------------------------------------------
class NB_CORE_EXPORT TSynchronizeChecklist : public TObject
{
  NB_DISABLE_COPY(TSynchronizeChecklist)
  friend class TTerminal;

public:
  __removed enum TAction { saNone, saUploadNew, saDownloadNew, saUploadUpdate, saDownloadUpdate, saDeleteRemote, saDeleteLocal };
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

  void __fastcall Update(const TChecklistItem *Item, bool Check, TChecklistAction Action);

  static TChecklistAction __fastcall Reverse(TChecklistAction Action);

  __property int Count = { read = GetCount };
  __property const TItem * Item[int Index] = { read = GetItem };

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

  static intptr_t __fastcall Compare(const void *AItem1, const void *AItem2);
};
//---------------------------------------------------------------------------
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
//---------------------------------------------------------------------------
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
//---------------------------------------------------------------------------
class TCollectedFileList : public TObject
{
public:
  static inline bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TCollectedFileList); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TCollectedFileList) || TObject::is(Kind); }
public:
  TCollectedFileList();
  intptr_t Add(const UnicodeString AFileName, TObject *Object, bool Dir);
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
//---------------------------------------------------------------------------
class TParallelOperation : public TObject
{
public:
  explicit TParallelOperation(TOperationSide Side);
  ~TParallelOperation();

  void Init(
    TStrings *AFiles, const UnicodeString ATargetDir, const TCopyParamType *CopyParam, intptr_t Params,
    TFileOperationProgressType *MainOperationProgress, const UnicodeString AMainName);

  bool IsInitialized() const;
  void WaitFor();
  bool ShouldAddClient() const;
  void AddClient();
  void RemoveClient();
  intptr_t GetNext(
    TTerminal *Terminal, UnicodeString &FileName, TObject *&Object, UnicodeString &TargetDir,
    bool &Dir, bool &Recursed);
  void Done(const UnicodeString FileName, bool Dir, bool Success);

  __property TOperationSide Side = { read = FSide };
  __property const TCopyParamType * CopyParam = { read = FCopyParam };
  __property int Params = { read = FParams };
  __property UnicodeString TargetDir = { read = FTargetDir };
  __property TFileOperationProgressType * MainOperationProgress = { read = FMainOperationProgress };
  __property UnicodeString MainName = { read = FMainName };

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
//---------------------------------------------------------------------------
struct TLocalFileHandle
{
  TLocalFileHandle();
  ~TLocalFileHandle();

  void Dismiss();
  void Close();
  void Release();

  UnicodeString FileName;
  HANDLE Handle;
  DWORD Attrs;
  bool Directory;
  TDateTime Modification;
  int64_t MTime;
  int64_t ATime;
  int64_t Size;
};
//---------------------------------------------------------------------------
NB_CORE_EXPORT UnicodeString GetSessionUrl(const TTerminal *Terminal, bool WithUserName = false);
//---------------------------------------------------------------------------
