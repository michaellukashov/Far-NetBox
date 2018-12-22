//---------------------------------------------------------------------------
#pragma once

#include <rdestl/map.h>
#include <rdestl/set.h>
#include <rdestl/vector.h>

#include <function2.hpp>

#include <Classes.hpp>
#include <Common.h>
#include <Exceptions.h>

#include "SessionInfo.h"
#include "Interface.h"
#include "FileOperationProgress.h"
#include "FileMasks.h"
#include "RemoteFiles.h"
//---------------------------------------------------------------------------
class TCopyParamType;
class TFileOperationProgressType;
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
typedef std::vector<__int64> TCalculatedSizes;
//---------------------------------------------------------------------------
#if 0
typedef void (__closure *TQueryUserEvent)
  (TObject * Sender, const UnicodeString Query, TStrings * MoreMessages, uint32_t Answers,
   const TQueryParams * Params, uint32_t & Answer, TQueryType QueryType, void * Arg);
#endif // #if 0
typedef nb::FastDelegate8<void,
  TObject * /*Sender*/, UnicodeString /*AQuery*/, TStrings * /*MoreMessages*/, uint32_t /*Answers*/,
   const TQueryParams * /*Params*/, uint32_t & /*Answer*/, TQueryType /*QueryType*/, void * /*Arg*/> TQueryUserEvent;
#if 0
typedef void (__closure *TPromptUserEvent)
  (TTerminal * Terminal, TPromptKind Kind, UnicodeString Name, UnicodeString Instructions,
   TStrings * Prompts, TStrings * Results, bool & Result, void * Arg);
#endif // #if 0
typedef nb::FastDelegate8<void,
  TTerminal * /*Terminal*/, TPromptKind /*Kind*/, UnicodeString /*AName*/, UnicodeString /*AInstructions*/,
  TStrings * /*Prompts*/, TStrings * /*Results*/, bool & /*Result*/, void * /*Arg*/> TPromptUserEvent;
#if 0
typedef void (__closure *TDisplayBannerEvent)
  (TTerminal * Terminal, UnicodeString SessionName, const UnicodeString & Banner,
   bool & NeverShowAgain, int Options, uint32_t & Params);
#endif // #if 0
typedef nb::FastDelegate6<void,
  TTerminal * /*Terminal*/, UnicodeString /*ASessionName*/, UnicodeString /*ABanner*/,
  bool & /*NeverShowAgain*/, intptr_t /*Options*/, uintptr_t & /*Params*/> TDisplayBannerEvent;
#if 0
typedef void (__closure *TExtendedExceptionEvent)
  (TTerminal * Terminal, Exception * E, void * Arg);
#endif // #if 0
typedef nb::FastDelegate3<void,
  TTerminal * /*Terminal*/, Exception * /*E*/, void * /*Arg*/ > TExtendedExceptionEvent;
#if 0
typedef void (__closure *TReadDirectoryEvent)(System::TObject * Sender, Boolean ReloadOnly);
#endif // #if 0
typedef nb::FastDelegate2<void, TObject * /*Sender*/,
  Boolean /*ReloadOnly*/> TReadDirectoryEvent;
#if 0
typedef void (__closure *TReadDirectoryProgressEvent)(
  System::TObject* Sender, int Progress, int ResolvedLinks, bool & Cancel);
#endif // #if 0
typedef nb::FastDelegate4<void,
  TObject * /*Sender*/, intptr_t /*Progress*/, intptr_t /*ResolvedLinks*/, bool & /*Cancel*/> TReadDirectoryProgressEvent;
#if 0
typedef void (__closure *TProcessFileEvent)
  (const UnicodeString FileName, const TRemoteFile * File, void * Param);
#endif // #if 0
typedef nb::FastDelegate3<void,
  UnicodeString /*AFileName*/, const TRemoteFile * /*AFile*/, void * /*AParam*/> TProcessFileEvent;
#if 0
typedef void (__closure *TProcessFileEventEx)
  (const UnicodeString FileName, const TRemoteFile * File, void * Param, int Index);
#endif // #if 0
typedef nb::FastDelegate4<void,
  const UnicodeString /*AFileName*/, const TRemoteFile * /*File*/,
  void * /*Param*/, intptr_t /*Index*/> TProcessFileEventEx;
#if 0
typedef int (__closure *TFileOperationEvent)
  (void * Param1, void * Param2);
#endif // #if 0
typedef nb::FastDelegate2<intptr_t,
  void * /*Param1*/, void * /*Param2*/> TFileOperationEvent;
#if 0
typedef void (__closure *TSynchronizeDirectory)
  (const UnicodeString LocalDirectory, const UnicodeString RemoteDirectory,
   bool & Continue, bool Collect);
typedef void __fastcall (__closure *TUpdatedSynchronizationChecklistItems)(
  const TSynchronizeChecklist::TItemList & Items);
typedef void __fastcall (__closure *TProcessedSynchronizationChecklistItem)(
  void * Token, const TSynchronizeChecklist::TItem * Item);
#endif // #if 0
typedef nb::FastDelegate4<void,
  UnicodeString /*LocalDirectory*/, UnicodeString /*RemoteDirectory*/,
  bool & /*Continue*/, bool /*Collect*/> TSynchronizeDirectoryEvent;
typedef nb::FastDelegate1<void,
  const TSynchronizeChecklist::TItemList & /*Items*/> TUpdatedSynchronizationChecklistItems;
typedef nb::FastDelegate2<void,
  void * /*Token*/, const TChecklistItem* /*Item*/> TProcessedSynchronizationChecklistItem;
#if 0
  const UnicodeString FileName, bool Alternative, int & Deleted);
typedef void (__closure *TDeleteLocalFileEvent)(
#endif // #if 0
typedef nb::FastDelegate2<void,
  UnicodeString /*FileName*/, bool /*Alternative*/> TDeleteLocalFileEvent;
#if 0
typedef int (__closure *TDirectoryModifiedEvent)
  (TTerminal * Terminal, const UnicodeString Directory, bool SubDirs);
#endif // #if 0
typedef nb::FastDelegate3<int,
  TTerminal * /*Terminal*/, UnicodeString /*Directory*/, bool /*SubDirs*/> TDirectoryModifiedEvent;
#if 0
typedef void (__closure *TInformationEvent)
  (TTerminal * Terminal, const UnicodeString & Str, bool Status, int Phase);
#endif // #if 0
typedef nb::FastDelegate4<void,
  TTerminal * /*Terminal*/, UnicodeString /*Str*/, bool /*Status*/, intptr_t /*Phase*/> TInformationEvent;
#if 0
typedef void (__closure *TCustomCommandEvent)
  (TTerminal * Terminal, const UnicodeString & Command, bool & Handled);
#endif // #if 0
typedef nb::FastDelegate3<void,
  TTerminal * /*Terminal*/, UnicodeString /*Command*/, bool & /*Handled*/> TCustomCommandEvent;

typedef nb::FastDelegate5<HANDLE,
  UnicodeString /*AFileName*/, DWORD /*DesiredAccess*/,
  DWORD /*ShareMode*/, DWORD /*CreationDisposition*/,
  DWORD /*FlagsAndAttributes*/> TCreateLocalFileEvent;
typedef nb::FastDelegate1<DWORD,
  UnicodeString /*AFileName*/> TGetLocalFileAttributesEvent;
typedef nb::FastDelegate2<bool,
  UnicodeString /*AFileName*/, DWORD /*FileAttributes*/> TSetLocalFileAttributesEvent;
typedef nb::FastDelegate3<bool,
  UnicodeString /*AFileName*/, UnicodeString /*ANewFileName*/,
  DWORD /*Flags*/> TMoveLocalFileEvent;
typedef nb::FastDelegate1<bool,
  UnicodeString /*ALocalDirName*/> TRemoveLocalDirectoryEvent;
typedef nb::FastDelegate2<bool,
  UnicodeString /*ALocalDirName*/,
  LPSECURITY_ATTRIBUTES /*SecurityAttributes*/> TCreateLocalDirectoryEvent;
typedef nb::FastDelegate0<bool> TCheckForEscEvent;
//---------------------------------------------------------------------------
const uintptr_t folNone = 0x00;
const uintptr_t folAllowSkip = 0x01;
const uintptr_t folRetryOnFatal = 0x02;

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
inline void ThrowSkipFile(Exception *Exception, const UnicodeString Message)
{
  throw ESkipFile(Exception, Message);
}
inline void ThrowSkipFileNull() { ThrowSkipFile(nullptr, L""); }

NB_CORE_EXPORT void FileOperationLoopCustom(TTerminal *Terminal,
  TFileOperationProgressType *OperationProgress,
  uintptr_t Flags, const UnicodeString Message,
  const UnicodeString HelpKeyword,
  fu2::function<void()> Operation);

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
const int csStopOnFirstFile = 0x02;
const int csDisallowTemporaryTransferFiles = 0x04;
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
NB_DEFINE_CLASS_ID(TTerminal);
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
  TPromptKind FRememberedPasswordKind;
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
  typedef rde::map<UnicodeString, UnicodeString> TEncryptedFileNames;
  TEncryptedFileNames FEncryptedFileNames;
  rde::set<UnicodeString> FFoldersScannedForEncryptedFiles;
  RawByteString FEncryptKey;
  TFileOperationProgressType::TPersistence * FOperationProgressPersistence;
  TOnceDoneOperation FOperationProgressOnceDoneOperation;

public:
  void CommandError(Exception *E, const UnicodeString AMsg);
  uintptr_t CommandError(Exception *E, const UnicodeString AMsg,
    uint32_t Answers, const UnicodeString HelpKeyword = L"");
  UnicodeString RemoteGetCurrentDirectory();
  bool GetExceptionOnFail() const;
  const TRemoteTokenList * GetGroups() const { return const_cast<TTerminal *>(this)->GetGroups(); }
  const TRemoteTokenList * GetUsers() const { return const_cast<TTerminal *>(this)->GetUsers(); }
  const TRemoteTokenList * GetMembership() const { return const_cast<TTerminal *>(this)->GetMembership(); }
  void TerminalSetCurrentDirectory(const UnicodeString AValue);
  void SetExceptionOnFail(bool Value);
  void ReactOnCommand(intptr_t /*TFSCommand*/ ACmd);
  UnicodeString TerminalGetUserName() const;
  bool GetAreCachesEmpty() const;
  void ClearCachedFileList(const UnicodeString APath, bool SubDirs);
  void AddCachedFileList(TRemoteFileList *FileList);
  bool GetCommandSessionOpened() const;
  TTerminal * GetCommandSession();
  bool GetResolvingSymlinks() const;
  bool GetActive() const;
  UnicodeString GetPassword() const;
  UnicodeString GetRememberedPassword() const;
  UnicodeString GetRememberedTunnelPassword() const;
  bool GetStoredCredentialsTried() const;
  bool InTransaction() const;
  void SaveCapabilities(TFileSystemInfo &FileSystemInfo);
  void CreateTargetDirectory(const UnicodeString DirectoryPath, uintptr_t Attrs, const TCopyParamType * CopyParam);
  static UnicodeString SynchronizeModeStr(TSynchronizeMode Mode);
  static UnicodeString SynchronizeParamsStr(intptr_t Params);

  TRemoteTokenList * GetGroups();
  TRemoteTokenList * GetUsers();
  TRemoteTokenList * GetMembership();

protected:
  bool FReadCurrentDirectoryPending;
  bool FReadDirectoryPending;
  bool FTunnelOpening;
  TCustomFileSystem *FFileSystem;

  void DoStartReadDirectory();
  void DoReadDirectoryProgress(intptr_t Progress, intptr_t ResolvedLinks, bool &Cancel);
  void DoReadDirectory(bool ReloadOnly);
  void DoCreateDirectory(const UnicodeString ADirName, bool Encrypt);
  void DoDeleteFile(const UnicodeString AFileName, const TRemoteFile *AFile,
    intptr_t Params);
  void DoCustomCommandOnFile(const UnicodeString AFileName,
    const TRemoteFile *AFile, const UnicodeString ACommand, intptr_t AParams, TCaptureOutputEvent OutputEvent);
  void DoRenameFile(const UnicodeString AFileName, const TRemoteFile *AFile,
    const UnicodeString ANewName, bool Move);
  void DoCopyFile(const UnicodeString AFileName, const TRemoteFile *AFile, const UnicodeString ANewName);
  void DoChangeFileProperties(const UnicodeString AFileName,
    const TRemoteFile *AFile, const TRemoteProperties *Properties);
  void DoChangeDirectory();
  void DoInitializeLog();
  void EnsureNonExistence(const UnicodeString AFileName);
  void LookupUsersGroups();
  void FileModified(const TRemoteFile *AFile,
    const UnicodeString AFileName, bool ClearDirectoryChange = false);
  intptr_t FileOperationLoop(TFileOperationEvent CallBackFunc,
    TFileOperationProgressType * OperationProgress, uintptr_t AFlags,
    const UnicodeString Message, void *Param1 = nullptr, void *Param2 = nullptr);
  bool GetIsCapableProtected(TFSCapability Capability) const;
  bool ProcessFiles(TStrings *AFileList, TFileOperation Operation,
    TProcessFileEvent ProcessFile, void *Param = nullptr, TOperationSide Side = osRemote,
    bool Ex = false);
  bool ProcessFilesEx(TStrings *FileList, TFileOperation Operation,
    TProcessFileEventEx ProcessFile, void *AParam = nullptr, TOperationSide Side = osRemote);
  void ProcessDirectory(const UnicodeString ADirName,
    TProcessFileEvent CallBackFunc, void *AParam = nullptr, bool UseCache = false,
    bool IgnoreErrors = false);
  bool DeleteContentsIfDirectory(
    const UnicodeString AFileName, const TRemoteFile *AFile, intptr_t AParams, TRmSessionAction &Action);
  void AnnounceFileListOperation();
  UnicodeString TranslateLockedPath(const UnicodeString APath, bool Lock);
  void ReadDirectory(TRemoteFileList *AFileList);
  void CustomReadDirectory(TRemoteFileList *AFileList);
  void DoCreateLink(const UnicodeString AFileName, const UnicodeString APointTo, bool Symbolic);
  bool TerminalCreateLocalFile(const UnicodeString ATargetFileName,
    TFileOperationProgressType *OperationProgress,
    bool Resume,
    bool NoConfirmation,
    HANDLE *AHandle);
  HANDLE TerminalCreateLocalFile(const UnicodeString LocalFileName, DWORD DesiredAccess,
    DWORD ShareMode, DWORD CreationDisposition, DWORD FlagsAndAttributes);
  bool TerminalCreateLocalFile(const UnicodeString FileName,
    TFileOperationProgressType * OperationProgress, HANDLE * AHandle,
    bool NoConfirmation);
  void TerminalOpenLocalFile(const UnicodeString ATargetFileName, DWORD Access,
    DWORD *AAttrs, HANDLE *AHandle, int64_t *ACTime,
    int64_t *AMTime, int64_t *AATime, int64_t *ASize,
    bool TryWriteReadOnly = true);
  void TerminalOpenLocalFile(
    const UnicodeString AFileName, unsigned int Access, TLocalFileHandle & Handle, bool TryWriteReadOnly = true);
  bool AllowLocalFileTransfer(
    const UnicodeString AFileName, const TSearchRecSmart * SearchRec,
    const TCopyParamType *CopyParam, TFileOperationProgressType *OperationProgress);
  bool HandleException(Exception *E);
  void CalculateFileSize(const UnicodeString AFileName,
    const TRemoteFile *AFile, /*TCalculateSizeParams*/ void *ASize);
  void DoCalculateFileSize(const UnicodeString AFileName,
    const TRemoteFile *AFile, void *AParam);
  bool DoCalculateDirectorySize(const UnicodeString AFileName, TCalculateSizeParams * Params);
  void CalculateLocalFileSize(
    const UnicodeString AFileName, const TSearchRecSmart & Rec, /*__int64*/ void * Size);
  bool DoCalculateDirectorySize(const UnicodeString AFileName,
    const TRemoteFile *AFile, TCalculateSizeParams *Params);
  void CalculateLocalFileSize(const UnicodeString AFileName,
    const TSearchRec &Rec, /*int64_t*/ void *Size);
  bool CalculateLocalFilesSize(TStrings *AFileList,
    const TCopyParamType *CopyParam, bool AllowDirs, TStrings *Files,
    int64_t &Size);
  TBatchOverwrite EffectiveBatchOverwrite(
    const UnicodeString ASourceFullFileName, const TCopyParamType *CopyParam, intptr_t Params,
    TFileOperationProgressType *OperationProgress, bool Special) const;
  bool CheckRemoteFile(
    const UnicodeString AFileName, const TCopyParamType *CopyParam,
    intptr_t Params, TFileOperationProgressType *OperationProgress) const;
  uint32_t ConfirmFileOverwrite(
    const UnicodeString ASourceFullFileName, const UnicodeString ATargetFileName,
    const TOverwriteFileParams *FileParams, uint32_t Answers, TQueryParams *QueryParams,
    TOperationSide Side, const TCopyParamType *CopyParam, intptr_t Params,
    TFileOperationProgressType *OperationProgress, const UnicodeString AMessage = L"");
  void DoSynchronizeCollectDirectory(const UnicodeString ALocalDirectory,
    const UnicodeString ARemoteDirectory, TSynchronizeMode Mode,
    const TCopyParamType *CopyParam, intptr_t Params,
    TSynchronizeDirectoryEvent OnSynchronizeDirectory,
    TSynchronizeOptions *Options, intptr_t Level, TSynchronizeChecklist *Checklist);
  bool LocalFindFirstLoop(const UnicodeString ADirectory, TSearchRecChecked & SearchRec);
  bool LocalFindNextLoop(TSearchRecChecked & SearchRec);
  bool DoAllowLocalFileTransfer(
    const UnicodeString AFileName, const TSearchRecSmart & SearchRec, const TCopyParamType * CopyParam, bool DisallowTemporaryTransferFiles);
  bool DoAllowRemoteFileTransfer(
    const TRemoteFile * File, const TCopyParamType * CopyParam, bool DisallowTemporaryTransferFiles);
  bool IsEmptyLocalDirectory(
    const UnicodeString ALocalDirectory, const TCopyParamType * CopyParam, bool DisallowTemporaryTransferFiles);
  bool IsEmptyRemoteDirectory(
    const TRemoteFile * File, const TCopyParamType * CopyParam, bool DisallowTemporaryTransferFiles);
  void DoSynchronizeCollectFile(const UnicodeString AFileName,
    const TRemoteFile *AFile, /*TSynchronizeData* */void * Param);
  void SynchronizeCollectFile(const UnicodeString AFileName,
    const TRemoteFile *AFile, /*TSynchronizeData* */ void *Param);
  void SynchronizeRemoteTimestamp(const UnicodeString AFileName,
    const TRemoteFile *AFile, void *Param);
  void SynchronizeLocalTimestamp(const UnicodeString AFileName,
    const TRemoteFile *AFile, void *Param);
  void DoSynchronizeProgress(const TSynchronizeData &Data, bool Collect);
  void DeleteLocalFile(const UnicodeString AFileName,
    const TRemoteFile *AFile, void *Params);
  void RecycleFile(const UnicodeString AFileName, const TRemoteFile *AFile);
  TStrings * GetFixedPaths() const;
  void DoStartup();
  virtual bool DoQueryReopen(Exception *E);
  virtual void FatalError(Exception *E, const UnicodeString AMsg, const UnicodeString AHelpKeyword = L"") override;
  void ResetConnection();
  virtual bool DoPromptUser(TSessionData *Data, TPromptKind Kind,
    const UnicodeString AName, const UnicodeString AInstructions, TStrings *Prompts,
    TStrings *Response);
  void OpenTunnel();
  void CloseTunnel();
  void DoInformation(const UnicodeString AStr, bool Status, intptr_t Phase = -1);
  bool PromptUser(TSessionData *Data, TPromptKind Kind,
    const UnicodeString AName, const UnicodeString AInstructions, const UnicodeString Prompt, bool Echo,
    intptr_t MaxLen, UnicodeString &AResult);
  void FileFind(const UnicodeString AFileName, const TRemoteFile *AFile, void *Param);
  void DoFilesFind(const UnicodeString ADirectory, TFilesFindParams &Params, const UnicodeString ARealDirectory);
  bool DoCreateLocalFile(const UnicodeString AFileName,
    TFileOperationProgressType *OperationProgress,
    bool Resume,
    bool NoConfirmation,
    HANDLE *AHandle);
  void LockFile(const UnicodeString AFileName, const TRemoteFile *AFile, void * Param);
  void UnlockFile(const UnicodeString AFileName, const TRemoteFile *AFile, void * Param);
  void DoLockFile(const UnicodeString AFileName, const TRemoteFile *AFile);
  void DoUnlockFile(const UnicodeString AFileName, const TRemoteFile *AFile);
  void OperationFinish(
    TFileOperationProgressType * Progress, const void * Item, const UnicodeString AFileName,
    bool Success, TOnceDoneOperation & OnceDoneOperation);
  void OperationStart(
    TFileOperationProgressType & Progress, TFileOperation Operation, TOperationSide Side, int Count);
  void OperationStart(
    TFileOperationProgressType & Progress, TFileOperation Operation, TOperationSide Side, int Count,
    bool Temp, const UnicodeString ADirectory, unsigned long CPSLimit);
  void OperationStop(TFileOperationProgressType & Progress);
  virtual void Information(const UnicodeString AStr, bool Status) override;
  virtual uint32_t QueryUser(const UnicodeString AQuery,
    TStrings *MoreMessages, uint32_t Answers, const TQueryParams *Params,
    TQueryType QueryType = qtConfirmation) override;
  virtual uint32_t QueryUserException(const UnicodeString AQuery,
    Exception *E, uint32_t Answers, const TQueryParams *Params,
    TQueryType QueryType = qtConfirmation) override;
  virtual bool PromptUser(TSessionData *Data, TPromptKind Kind,
    const UnicodeString AName, const UnicodeString AInstructions, TStrings *Prompts, TStrings *Results) override;
  virtual void DisplayBanner(const UnicodeString ABanner) override;
  virtual void Closed() override;
  virtual void ProcessGUI() override;
  void Progress(TFileOperationProgressType *OperationProgress);
  virtual void HandleExtendedException(Exception *E) override;
  bool IsListenerFree(uintptr_t PortNumber) const;
  void DoProgress(TFileOperationProgressType &ProgressData);
  void DoFinished(TFileOperation Operation, TOperationSide Side, bool Temp,
    const UnicodeString AFileName, bool Success, TOnceDoneOperation &OnceDoneOperation);
  void RollbackAction(TSessionAction &Action,
    TFileOperationProgressType *OperationProgress, Exception *E = nullptr);
  void DoAnyCommand(const UnicodeString ACommand, TCaptureOutputEvent OutputEvent,
    TCallSessionAction *Action);
  TRemoteFileList * DoReadDirectoryListing(const UnicodeString ADirectory, bool UseCache);
  RawByteString EncryptPassword(const UnicodeString APassword) const;
  UnicodeString DecryptPassword(const RawByteString APassword) const;
  UnicodeString GetRemoteFileInfo(TRemoteFile *AFile) const;
  void LogRemoteFile(TRemoteFile *AFile);
  UnicodeString FormatFileDetailsForLog(const UnicodeString AFileName, const TDateTime &AModification, int64_t Size);
  void LogFileDetails(const UnicodeString AFileName, const TDateTime &AModification, int64_t Size);
  void LogFileDone(TFileOperationProgressType *OperationProgress, const UnicodeString DestFileName);
  void LogTotalTransferDetails(
    const UnicodeString ATargetDir, const TCopyParamType *CopyParam,
    TFileOperationProgressType *OperationProgress, bool Parallel, TStrings *AFiles);
  void LogTotalTransferDone(TFileOperationProgressType *OperationProgress);
  virtual TTerminal * GetPasswordSource();
  void DoEndTransaction(bool Inform);
  bool VerifyCertificate(
    const UnicodeString CertificateStorageKey, const UnicodeString SiteKey,
    const UnicodeString Fingerprint,
    const UnicodeString CertificateSubject, int Failures);
  void CacheCertificate(const UnicodeString CertificateStorageKey,
    const UnicodeString SiteKey, const UnicodeString Fingerprint, intptr_t Failures);
  bool ConfirmCertificate(
    TSessionInfo & SessionInfo, intptr_t AFailures, const UnicodeString ACertificateStorageKey, bool CanRemember);
  void CollectTlsUsage(const UnicodeString TlsVersionStr);
  bool LoadTlsCertificate(X509 *& Certificate, EVP_PKEY *& PrivateKey);
  bool TryStartOperationWithFile(
    const UnicodeString AFileName, TFileOperation Operation1, TFileOperation Operation2 = foNone);
  void StartOperationWithFile(
    const UnicodeString AFileName, TFileOperation Operation1, TFileOperation Operation2 = foNone);
  bool CanRecurseToDirectory(const TRemoteFile * File) const;
  bool DoOnCustomCommand(const UnicodeString ACommand);
  bool CanParallel(const TCopyParamType *CopyParam, intptr_t AParams, TParallelOperation *ParallelOperation) const;
  void CopyParallel(TParallelOperation *ParallelOperation, TFileOperationProgressType *OperationProgress);
  void DoCopyToRemote(
    TStrings *AFilesToCopy, const UnicodeString ATargetDir, const TCopyParamType *CopyParam, intptr_t AParams,
    TFileOperationProgressType *OperationProgress, uintptr_t AFlags, TOnceDoneOperation &OnceDoneOperation);
  void SourceRobust(
    const UnicodeString AFileName, const TSearchRecSmart * SearchRec,
    const UnicodeString ATargetDir, const TCopyParamType * CopyParam, intptr_t AParams,
    TFileOperationProgressType * OperationProgress, uintptr_t AFlags);
  void Source(
    const UnicodeString AFileName, const TSearchRecSmart * SearchRec,
    const UnicodeString ATargetDir, const TCopyParamType * CopyParam, int Params,
    TFileOperationProgressType * OperationProgress, uintptr_t AFlags, TUploadSessionAction &Action, bool &ChildError);
  void DirectorySource(
    const UnicodeString ADirectoryName, const UnicodeString ATargetDir, const UnicodeString ADestDirectoryName,
    uintptr_t Attrs, const TCopyParamType * CopyParam, intptr_t AParams,
    TFileOperationProgressType * OperationProgress, uintptr_t AFlags);
  void SelectTransferMode(
    const UnicodeString ABaseFileName, TOperationSide Side, const TCopyParamType *CopyParam,
    const TFileMasks::TParams &MaskParams);
  void SelectSourceTransferMode(const TLocalFileHandle &Handle, const TCopyParamType *CopyParam);
  void UpdateSource(const TLocalFileHandle &Handle, const TCopyParamType * CopyParam, intptr_t AParams);
  void DoCopyToLocal(
    TStrings * FilesToCopy, const UnicodeString ATargetDir, const TCopyParamType *CopyParam, intptr_t AParams,
    TFileOperationProgressType *OperationProgress, uintptr_t AFlags, TOnceDoneOperation &OnceDoneOperation);
  void SinkRobust(
    const UnicodeString AFileName, const TRemoteFile *AFile, const UnicodeString ATargetDir,
    const TCopyParamType *CopyParam, intptr_t AParams, TFileOperationProgressType * OperationProgress, uintptr_t AFlags);
  void Sink(
    const UnicodeString AFileName, const TRemoteFile *AFile, const UnicodeString ATargetDir,
    const TCopyParamType *CopyParam, intptr_t AParams, TFileOperationProgressType *OperationProgress, uintptr_t AFlags,
    TDownloadSessionAction &Action);
  void SinkFile(const UnicodeString AFileName, const TRemoteFile *AFile, void *Param);
  void UpdateTargetAttrs(
    const UnicodeString ADestFullName, const TRemoteFile *AFile, const TCopyParamType *CopyParam, uintptr_t Attrs);
  void UpdateTargetTime(HANDLE Handle, TDateTime Modification, TDSTMode DSTMode);

  UnicodeString EncryptFileName(const UnicodeString APath, bool EncryptNewFiles);
  UnicodeString DecryptFileName(const UnicodeString APath);
  typename TEncryptedFileNames::const_iterator GetEncryptedFileName(const UnicodeString APath);
  bool IsFileEncrypted(const UnicodeString APath, bool EncryptNewFiles = false);

  __property TFileOperationProgressType *OperationProgress = { read = FOperationProgress };

  const TFileOperationProgressType *GetOperationProgress() const { return FOperationProgress; }
  TFileOperationProgressType *GetOperationProgress() { return FOperationProgress; }
  void SetOperationProgress(TFileOperationProgressType *OperationProgress) { FOperationProgress = OperationProgress; }
  virtual const TTerminal *GetPasswordSource() const { return this; }

public:
  explicit TTerminal(TObjectClassId Kind = OBJECT_CLASS_TTerminal);
  void Init(TSessionData *ASessionData, TConfiguration *AConfiguration);
  virtual ~TTerminal();
  void Open();
  void Close();
  void FingerprintScan(UnicodeString &SHA256, UnicodeString &MD5);
  void Reopen(intptr_t Params);
  virtual void DirectoryModified(const UnicodeString APath, bool SubDirs);
  virtual void DirectoryLoaded(TRemoteFileList *FileList);
  void ShowExtendedException(Exception *E);
  void Idle();
  void RecryptPasswords();
  bool AllowedAnyCommand(const UnicodeString ACommand) const;
  void AnyCommand(const UnicodeString ACommand, TCaptureOutputEvent OutputEvent);
  void CloseOnCompletion(
    TOnceDoneOperation Operation = odoDisconnect, const UnicodeString AMessage = L"",
    const UnicodeString TargetLocalPath = L"", const UnicodeString ADestLocalFileName = L"");
  UnicodeString GetAbsolutePath(const UnicodeString APath, bool Local) const;
  void BeginTransaction();
  void ReadCurrentDirectory();
  void ReadDirectory(bool ReloadOnly, bool ForceCache = false);
  TRemoteFileList * ReadDirectoryListing(const UnicodeString Directory, const TFileMasks &Mask);
  TRemoteFileList * CustomReadDirectoryListing(const UnicodeString Directory, bool UseCache);
  TRemoteFile * ReadFileListing(const UnicodeString APath);
  void ReadFile(const UnicodeString AFileName, TRemoteFile *&AFile);
  bool FileExists(const UnicodeString AFileName, TRemoteFile **AFile = nullptr);
  void ReadSymlink(TRemoteFile * SymlinkFile, TRemoteFile *& AFile);
  bool CopyToLocal(
    TStrings * FilesToCopy, const UnicodeString ATargetDir, const TCopyParamType * CopyParam, int Params,
    TParallelOperation * ParallelOperation);
  bool CopyToRemote(
    TStrings * FilesToCopy, const UnicodeString ATargetDir, const TCopyParamType * CopyParam, int Params,
    TParallelOperation * ParallelOperation);
  bool CopyToLocal(TStrings *AFilesToCopy,
    const UnicodeString ATargetDir, const TCopyParamType *CopyParam, intptr_t AParams, TParallelOperation *ParallelOperation);
  bool CopyToRemote(TStrings *AFilesToCopy,
    UnicodeString ATargetDir, const TCopyParamType *CopyParam, intptr_t Params, TParallelOperation *ParallelOperation);
  intptr_t CopyToParallel(TParallelOperation *ParallelOperation, TFileOperationProgressType *OperationProgress);
  void LogParallelTransfer(TParallelOperation *ParallelOperation);
  void RemoteCreateDirectory(const UnicodeString ADirName, const TRemoteProperties * Properties = nullptr);
  void RemoteCreateLink(const UnicodeString AFileName, const UnicodeString APointTo, bool Symbolic);
  void RemoteDeleteFile(const UnicodeString AFileName,
    const TRemoteFile *AFile = nullptr, void *AParams = nullptr);
  bool RemoteDeleteFiles(TStrings *AFilesToDelete, intptr_t Params = 0);
  bool DeleteLocalFiles(TStrings *AFileList, intptr_t Params = 0);
  bool IsRecycledFile(const UnicodeString AFileName);
  void CustomCommandOnFile(const UnicodeString AFileName,
    const TRemoteFile *AFile, void *AParams);
  void CustomCommandOnFiles(const UnicodeString ACommand, intptr_t AParams,
    TStrings *AFiles, TCaptureOutputEvent OutputEvent);
  void RemoteChangeDirectory(const UnicodeString ADirectory);
  void EndTransaction();
  void HomeDirectory();
  UnicodeString GetHomeDirectory();
  void ChangeFileProperties(const UnicodeString AFileName,
    const TRemoteFile *AFile, /*const TRemoteProperties*/ void *Properties);
  void ChangeFilesProperties(TStrings *AFileList,
    const TRemoteProperties *Properties);
  bool LoadFilesProperties(TStrings *AFileList);
  void TerminalError(const UnicodeString Msg);
  void TerminalError(Exception *E, const UnicodeString AMsg, const UnicodeString AHelpKeyword = L"");
  void ReloadDirectory();
  void RefreshDirectory();
  void TerminalRenameFile(const TRemoteFile *AFile, const UnicodeString ANewName, bool CheckExistence);
  void TerminalMoveFile(const UnicodeString AFileName, const TRemoteFile *AFile,
    /*const TMoveFileParams*/ void *Param);
  bool TerminalMoveFiles(TStrings *AFileList, const UnicodeString ATarget,
    const UnicodeString AFileMask);
  void TerminalCopyFile(const UnicodeString AFileName, const TRemoteFile *AFile,
    /*const TMoveFileParams*/ void *Param);
  bool TerminalCopyFiles(TStrings *AFileList, const UnicodeString ATarget,
    const UnicodeString AFileMask);
  bool CalculateFilesSize(TStrings *AFileList, int64_t &Size,
    intptr_t Params, const TCopyParamType *CopyParam, bool AllowDirs,
    TCalculateSizeStats &Stats);
  bool CalculateLocalFilesSize(TStrings * FileList, int64_t & Size,
    const TCopyParamType * CopyParam, bool AllowDirs, TStrings * Files, TCalculatedSizes * CalculatedSizes);
  void CalculateFilesChecksum(const UnicodeString Alg, TStrings *AFileList,
    TStrings *Checksums, TCalculatedChecksumEvent OnCalculatedChecksum);
  void ClearCaches();
  TSynchronizeChecklist * SynchronizeCollect(const UnicodeString LocalDirectory,
    const UnicodeString ARemoteDirectory, TSynchronizeMode Mode,
    const TCopyParamType *CopyParam, intptr_t Params,
    TSynchronizeDirectoryEvent OnSynchronizeDirectory, TSynchronizeOptions *Options);
  void SynchronizeApply(
    TSynchronizeChecklist * Checklist,
    const TCopyParamType *CopyParam, intptr_t Params,
    TSynchronizeDirectoryEvent OnSynchronizeDirectory, TProcessedSynchronizationChecklistItem OnProcessedItem,
    TUpdatedSynchronizationChecklistItems OnUpdatedSynchronizationChecklistItems, void * Token,
    TFileOperationStatistics * Statistics);
  void SynchronizeChecklistCalculateSize(
    TSynchronizeChecklist * Checklist, const TSynchronizeChecklist::TItemList & Items,
    const TCopyParamType * CopyParam);
  void FilesFind(const UnicodeString Directory, const TFileMasks &FileMask,
    TFileFoundEvent OnFileFound, TFindingFileEvent OnFindingFile);
  void SpaceAvailable(const UnicodeString APath, TSpaceAvailable &ASpaceAvailable);
  void LockFiles(TStrings *AFileList);
  void UnlockFiles(TStrings *AFileList);
  TRemoteFileList * DirectoryFileList(const UnicodeString APath, TDateTime Timestamp, bool CanLoad);
  void MakeLocalFileList(
    const UnicodeString AFileName, const TSearchRecSmart & Rec, void * Param);
  bool FileOperationLoopQuery(Exception &E,
    TFileOperationProgressType *OperationProgress, const UnicodeString Message,
    uintptr_t AFlags, const UnicodeString SpecialRetry = L"", const UnicodeString HelpKeyword = L"");
  void FileOperationLoopEnd(Exception & E,
    TFileOperationProgressType * OperationProgress, const UnicodeString Message,
    uintptr_t AFlags, const UnicodeString SpecialRetry, const UnicodeString HelpKeyword);
  TUsableCopyParamAttrs UsableCopyParamAttrs(intptr_t AParams) const;
  bool ContinueReopen(TDateTime Start) const;
  bool QueryReopen(Exception * E, intptr_t AParams,
    TFileOperationProgressType *OperationProgress);
  UnicodeString PeekCurrentDirectory();
  void FatalAbort();
  void ReflectSettings() const;
  void CollectUsage();
  bool IsThisOrChild(TTerminal *Terminal) const;

  TTerminal * CreateSecondarySession(const UnicodeString Name, TSessionData *SessionData);
  void FillSessionDataForCode(TSessionData *SessionData) const;
  void UpdateSessionCredentials(TSessionData * Data);

  const TSessionInfo & GetSessionInfo() const;
  const TFileSystemInfo & GetFileSystemInfo(bool Retrieve = false);
  void LogEvent(const UnicodeString AStr);
  void LogEvent(int Level, const UnicodeString AStr);
  void GetSupportedChecksumAlgs(TStrings *Algs) const;
  UnicodeString ChangeFileName(const TCopyParamType *CopyParam,
    const UnicodeString AFileName, TOperationSide Side, bool FirstLevel) const;
  UnicodeString GetBaseFileName(const UnicodeString AFileName) const;
  bool IsEncryptingFiles() const { return !FEncryptKey.IsEmpty(); }
  RawByteString GetEncryptKey() const { return FEncryptKey; }

  static UnicodeString ExpandFileName(const UnicodeString APath,
    const UnicodeString BasePath);

  __property TSessionData * SessionData = { read = FSessionData };
  ROProperty<TSessionData *> SessionData{nb::bind(&TTerminal::GetSessionData, this)};
  __property TSessionLog * Log = { read = FLog };
  ROProperty<TSessionLog *> Log{nb::bind(&TTerminal::GetLogConst, this)};
  __property TActionLog * ActionLog = { read = FActionLog };
  __property TConfiguration * Configuration = { read = FConfiguration };
  ROProperty<const TConfiguration *> Configuration{nb::bind(&TTerminal::GetConfigurationConst, this)};
  __property bool Active = { read = GetActive };
  ROProperty<bool> Active{nb::bind(&TTerminal::GetActive, this)};
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
  void SetMasks(const UnicodeString Value);

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
  TSessionLog *GetLogConst() const { return FLog; }
  TSessionLog *GetLog() { return FLog; }
  TActionLog *GetActionLog() const { return FActionLog; }
  const TConfiguration *GetConfigurationConst() const { return FConfiguration; }
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

  void AfterMoveFiles(TStrings *AFileList);
};
//---------------------------------------------------------------------------
NB_DEFINE_CLASS_ID(TSecondaryTerminal);
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
  virtual ~TSecondaryTerminal() = default;
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
NB_DEFINE_CLASS_ID(TTerminalList);
class NB_CORE_EXPORT TTerminalList : public TObjectList
{
  NB_DISABLE_COPY(TTerminalList)
public:
  explicit TTerminalList(TConfiguration *AConfiguration);
  virtual ~TTerminalList();

  virtual TTerminal * NewTerminal(TSessionData *Data);
  virtual void FreeTerminal(TTerminal *Terminal);
  void FreeAndNullTerminal(TTerminal *& Terminal);
  void RecryptPasswords();

  __property TTerminal *Terminals[int Index]  = { read = GetTerminal };

protected:
  virtual TTerminal * CreateTerminal(TSessionData *Data);

private:
  TConfiguration *FConfiguration;

public:
  TTerminal * GetTerminal(intptr_t Index);
};
//---------------------------------------------------------------------------
NB_DEFINE_CLASS_ID(TCustomCommandParams);
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
  TCalculatedSizes * CalculatedSizes;
};
//---------------------------------------------------------------------------
NB_DEFINE_CLASS_ID(TCalculateSizeParams);
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
NB_DEFINE_CLASS_ID(TMakeLocalFileListParams);
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

  bool FilterFind(const UnicodeString AFileName) const;
  bool MatchesFilter(const UnicodeString AFileName) const;
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
NB_DEFINE_CLASS_ID(TCollectedFileList);
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
    TTerminal *Terminal, UnicodeString &AFileName, TObject *&Object, UnicodeString &ATargetDir,
    bool &Dir, bool &Recursed);
  void Done(const UnicodeString AFileName, bool Dir, bool Success);

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
class TLocalFile : public TObject
{
public:
  TSearchRecSmart SearchRec;
};
//---------------------------------------------------------------------------
NB_CORE_EXPORT UnicodeString GetSessionUrl(const TTerminal *Terminal, bool WithUserName = false);
//---------------------------------------------------------------------------
