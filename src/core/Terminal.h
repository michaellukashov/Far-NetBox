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
using TCalculatedSizes = rde::vector<int64_t>;
//---------------------------------------------------------------------------
#if 0
typedef void __fastcall (__closure *TQueryUserEvent)
  (TObject * Sender, const UnicodeString Query, TStrings * MoreMessages, uint32_t Answers,
   const TQueryParams * Params, unsigned int & Answer, TQueryType QueryType, void * Arg);
#endif // #if 0
using TQueryUserEvent = nb::FastDelegate8<void,
  TObject * /*Sender*/, UnicodeString /*AQuery*/, TStrings * /*MoreMessages*/, uintptr_t /*Answers*/,
   const TQueryParams * /*Params*/, uintptr_t & /*Answer*/, TQueryType /*QueryType*/, void * /*Arg*/>;
#if 0
typedef void __fastcall (__closure *TPromptUserEvent)
  (TTerminal * Terminal, TPromptKind Kind, UnicodeString Name, UnicodeString Instructions,
   TStrings * Prompts, TStrings * Results, bool & Result, void * Arg);
#endif // #if 0
using TPromptUserEvent = nb::FastDelegate8<void,
  TTerminal * /*Terminal*/, TPromptKind /*Kind*/, UnicodeString /*AName*/, UnicodeString /*AInstructions*/,
  TStrings * /*Prompts*/, TStrings * /*Results*/, bool & /*Result*/, void * /*Arg*/>;
#if 0
typedef void __fastcall (__closure *TDisplayBannerEvent)
  (TTerminal * Terminal, UnicodeString SessionName, const UnicodeString & Banner,
   bool & NeverShowAgain, int Options, unsigned int & Params);
#endif // #if 0
using TDisplayBannerEvent = nb::FastDelegate6<void,
  TTerminal * /*Terminal*/, UnicodeString /*ASessionName*/, UnicodeString /*ABanner*/,
  bool & /*NeverShowAgain*/, intptr_t /*Options*/, uintptr_t & /*Params*/>;
#if 0
typedef void __fastcall (__closure *TExtendedExceptionEvent)
  (TTerminal * Terminal, Exception * E, void * Arg);
#endif // #if 0
using TExtendedExceptionEvent = nb::FastDelegate3<void,
  TTerminal * /*Terminal*/, Exception * /*E*/, void * /*Arg*/ >;
#if 0
typedef void __fastcall (__closure *TReadDirectoryEvent)(System::TObject * Sender, Boolean ReloadOnly);
#endif // #if 0
using TReadDirectoryEvent = nb::FastDelegate2<void, TObject * /*Sender*/, Boolean /*ReloadOnly*/>;
#if 0
typedef void __fastcall (__closure *TReadDirectoryProgressEvent)(
  System::TObject* Sender, int Progress, int ResolvedLinks, bool & Cancel);
#endif // #if 0
using TReadDirectoryProgressEvent = nb::FastDelegate4<void,
  TObject * /*Sender*/, intptr_t /*Progress*/, intptr_t /*ResolvedLinks*/, bool & /*Cancel*/>;
#if 0
typedef void __fastcall (__closure *TProcessFileEvent)
  (const UnicodeString FileName, const TRemoteFile * File, void * Param);
#endif // #if 0
using TProcessFileEvent = nb::FastDelegate3<void,
  UnicodeString /*AFileName*/, const TRemoteFile * /*AFile*/, void * /*AParam*/>;
#if 0
typedef void __fastcall (__closure *TProcessFileEventEx)
  (const UnicodeString FileName, const TRemoteFile * File, void * Param, int Index);
#endif // #if 0
using TProcessFileEventEx = nb::FastDelegate4<void,
  const UnicodeString /*AFileName*/, const TRemoteFile * /*File*/,
  void * /*Param*/, intptr_t /*Index*/>;
#if 0
typedef int __fastcall (__closure *TFileOperationEvent)
  (void * Param1, void * Param2);
#endif // #if 0
using TFileOperationEvent = nb::FastDelegate2<intptr_t,
  void * /*Param1*/, void * /*Param2*/>;
#if 0
typedef void __fastcall (__closure *TSynchronizeDirectory)
  (const UnicodeString LocalDirectory, const UnicodeString RemoteDirectory,
   bool & Continue, bool Collect);
typedef void __fastcall (__closure *TUpdatedSynchronizationChecklistItems)(
  const TSynchronizeChecklist::TItemList & Items);
typedef void __fastcall (__closure *TProcessedSynchronizationChecklistItem)(
  void * Token, const TSynchronizeChecklist::TItem * Item);
#endif // #if 0
using TSynchronizeDirectoryEvent = nb::FastDelegate4<void,
  UnicodeString /*LocalDirectory*/, UnicodeString /*RemoteDirectory*/,
  bool & /*Continue*/, bool /*Collect*/>;
using TUpdatedSynchronizationChecklistItems = nb::FastDelegate1<void,
  const TSynchronizeChecklist::TItemList & /*Items*/>;
using TProcessedSynchronizationChecklistItem = nb::FastDelegate2<void,
  void * /*Token*/, const TChecklistItem * /*Item*/>;
#if 0
typedef void __fastcall (__closure *TDeleteLocalFileEvent)(
  const UnicodeString FileName, bool Alternative, int & Deleted);
#endif // #if 0
using TDeleteLocalFileEvent = nb::FastDelegate3<void,
  UnicodeString /*FileName*/, bool /*Alternative*/, intptr_t & /*Deleted*/>;
#if 0
typedef int __fastcall (__closure *TDirectoryModifiedEvent)
  (TTerminal * Terminal, const UnicodeString Directory, bool SubDirs);
#endif // #if 0
using TDirectoryModifiedEvent = nb::FastDelegate3<int,
  TTerminal * /*Terminal*/, UnicodeString /*Directory*/, bool /*SubDirs*/>;
#if 0
typedef void __fastcall (__closure *TInformationEvent)
  (TTerminal * Terminal, const UnicodeString & Str, bool Status, int Phase);
#endif // #if 0
using TInformationEvent = nb::FastDelegate4<void,
  TTerminal * /*Terminal*/, UnicodeString /*Str*/, bool /*Status*/, intptr_t /*Phase*/>;
#if 0
typedef void __fastcall (__closure *TCustomCommandEvent)
  (TTerminal * Terminal, const UnicodeString & Command, bool & Handled);
#endif // #if 0
using TCustomCommandEvent = nb::FastDelegate3<void,
  TTerminal * /*Terminal*/, UnicodeString /*Command*/, bool & /*Handled*/>;

using TCreateLocalFileEvent = nb::FastDelegate5<HANDLE,
  UnicodeString /*AFileName*/, DWORD /*DesiredAccess*/,
  DWORD /*ShareMode*/, DWORD /*CreationDisposition*/,
  DWORD /*FlagsAndAttributes*/>;
using TGetLocalFileAttributesEvent = nb::FastDelegate1<DWORD,
  UnicodeString /*AFileName*/>;
using TSetLocalFileAttributesEvent = nb::FastDelegate2<bool,
  UnicodeString /*AFileName*/, DWORD /*FileAttributes*/>;
using TMoveLocalFileEvent = nb::FastDelegate3<bool,
  UnicodeString /*AFileName*/, UnicodeString /*ANewFileName*/,
  DWORD /*Flags*/>;
using TRemoveLocalDirectoryEvent = nb::FastDelegate1<bool,
  UnicodeString /*ALocalDirName*/>;
using TCreateLocalDirectoryEvent = nb::FastDelegate2<bool,
  UnicodeString /*ALocalDirName*/,
  LPSECURITY_ATTRIBUTES /*SecurityAttributes*/>;
using TCheckForEscEvent = nb::FastDelegate0<bool>;
//---------------------------------------------------------------------------
constexpr const uintptr_t folNone = 0x00;
constexpr const uintptr_t folAllowSkip = 0x01;
constexpr const uintptr_t folRetryOnFatal = 0x02;

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
constexpr const intptr_t cpDelete = 0x01;
constexpr const intptr_t cpTemporary = 0x04;
constexpr const intptr_t cpNoConfirmation = 0x08;
constexpr const intptr_t cpNewerOnly = 0x10;
constexpr const intptr_t cpAppend = 0x20;
constexpr const intptr_t cpFirstLevel = 0x100;
constexpr const intptr_t cpResume = 0x40;
constexpr const intptr_t cpNoRecurse = 0x80;
//---------------------------------------------------------------------------
constexpr const intptr_t ccApplyToDirectories = 0x01;
constexpr const intptr_t ccRecursive = 0x02;
constexpr const intptr_t ccUser = 0x100;
//---------------------------------------------------------------------------
constexpr const intptr_t csIgnoreErrors = 0x01;
constexpr const intptr_t csStopOnFirstFile = 0x02;
constexpr const intptr_t csDisallowTemporaryTransferFiles = 0x04;
//---------------------------------------------------------------------------
constexpr const intptr_t ropNoReadDirectory = 0x02;
//---------------------------------------------------------------------------
constexpr const intptr_t boDisableNeverShowAgain = 0x01;
constexpr const intptr_t bpMonospacedFont = 0x01;
//---------------------------------------------------------------------------
constexpr const intptr_t tfNone = 0x00;
constexpr const intptr_t tfFirstLevel = 0x01;
constexpr const intptr_t tfNewDirectory = 0x02;
constexpr const intptr_t tfAutoResume = 0x04;
constexpr const intptr_t tfPreCreateDir = 0x08;
constexpr const intptr_t tfUseFileTransferAny = 0x10;
//---------------------------------------------------------------------------
NB_DEFINE_CLASS_ID(TTerminal);
class NB_CORE_EXPORT TTerminal : /*public TObject,*/ public TSessionUI
{
  NB_DISABLE_COPY(TTerminal)
public:
  static bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TTerminal); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TTerminal) || TSessionUI::is(Kind); }
public:
  // TScript::SynchronizeProc relies on the order
  enum TSynchronizeMode { smRemote, smLocal, smBoth };
  static constexpr const intptr_t spDelete = 0x01; // cannot be combined with spTimestamp
  static constexpr const intptr_t spNoConfirmation = 0x02; // has no effect for spTimestamp
  static constexpr const intptr_t spExistingOnly = 0x04; // is implicit for spTimestamp
  static constexpr const intptr_t spNoRecurse = 0x08;
  static constexpr const intptr_t spUseCache = 0x10; // cannot be combined with spTimestamp
  static constexpr const intptr_t spDelayProgress = 0x20; // cannot be combined with spTimestamp
  static constexpr const intptr_t spPreviewChanges = 0x40; // not used by core
  static constexpr const intptr_t spSubDirs = 0x80; // cannot be combined with spTimestamp
  static constexpr const intptr_t spTimestamp = 0x100;
  static constexpr const intptr_t spNotByTime = 0x200; // cannot be combined with spTimestamp and smBoth
  static constexpr const intptr_t spBySize = 0x400; // cannot be combined with smBoth, has opposite meaning for spTimestamp
  static constexpr const intptr_t spSelectedOnly = 0x800; // not used by core
  static constexpr const intptr_t spMirror = 0x1000;
  static constexpr const intptr_t spDefault = TTerminal::spNoConfirmation | TTerminal::spPreviewChanges;

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
  std::unique_ptr<TSessionData> FSessionData;
  std::unique_ptr<TSessionLog> FLog;
  std::unique_ptr<TActionLog> FActionLog;
  TConfiguration *FConfiguration{nullptr};
  UnicodeString FCurrentDirectory;
  UnicodeString FLockDirectory;
  intptr_t FExceptionOnFail{0};
  std::unique_ptr<TRemoteDirectory> FFiles;
  intptr_t FInTransaction{0};
  bool FSuspendTransaction{false};
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
  bool FUsersGroupsLookedup{false};
  TFileOperationProgressEvent FOnProgress{nullptr};
  TFileOperationFinishedEvent FOnFinished{nullptr};
  TFileOperationProgressType *FOperationProgress{nullptr};
  bool FUseBusyCursor{false};
  std::unique_ptr<TRemoteDirectoryCache> FDirectoryCache;
  std::unique_ptr<TRemoteDirectoryChangesCache> FDirectoryChangesCache;
  std::unique_ptr<TSecureShell> FSecureShell{nullptr};
  UnicodeString FLastDirectoryChange;
  TCurrentFSProtocol FFSProtocol{cfsUnknown};
  TTerminal *FCommandSession{nullptr};
  bool FAutoReadDirectory{true};
  bool FReadingCurrentDirectory{false};
  bool *FClosedOnCompletion{nullptr};
  TSessionStatus FStatus{ssClosed};
  intptr_t FOpening{0};
  RawByteString FRememberedPassword;
  TPromptKind FRememberedPasswordKind{};
  RawByteString FRememberedTunnelPassword;
  TTunnelThread *FTunnelThread{nullptr};
  TSecureShell *FTunnel{nullptr};
  TSessionData *FTunnelData{nullptr};
  TSessionLog *FTunnelLog{nullptr};
  TTunnelUI *FTunnelUI{nullptr};
  intptr_t FTunnelLocalPortNumber{0};
  UnicodeString FTunnelError;
  TQueryUserEvent FOnQueryUser{nullptr};
  TPromptUserEvent FOnPromptUser{nullptr};
  TDisplayBannerEvent FOnDisplayBanner{nullptr};
  TExtendedExceptionEvent FOnShowExtendedException{nullptr};
  TInformationEvent FOnInformation{nullptr};
  TCustomCommandEvent FOnCustomCommand{nullptr};
  TNotifyEvent FOnClose{nullptr};
  TCheckForEscEvent FOnCheckForEsc{nullptr};
  TCallbackGuard *FCallbackGuard{nullptr};
  TFindingFileEvent FOnFindingFile{nullptr};
  bool FEnableSecureShellUsage{false};
  bool FCollectFileSystemUsage{false};
  bool FRememberedPasswordTried{false};
  bool FRememberedTunnelPasswordTried{false};
  intptr_t FNesting{0};
  UnicodeString FFingerprintScannedSHA256;
  UnicodeString FFingerprintScannedMD5;
  DWORD FLastProgressLogged{0};
  std::unique_ptr<TRemoteDirectory> FOldFiles;
  UnicodeString FDestFileName;
  bool FMultipleDestinationFiles{false};
  bool FFileTransferAny{true};
  typedef rde::map<UnicodeString, UnicodeString> TEncryptedFileNames;
  TEncryptedFileNames FEncryptedFileNames;
  rde::set<UnicodeString> FFoldersScannedForEncryptedFiles;
  RawByteString FEncryptKey;
  TFileOperationProgressType::TPersistence * FOperationProgressPersistence{nullptr};
  TOnceDoneOperation FOperationProgressOnceDoneOperation{odoIdle};

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

protected:
  bool FReadCurrentDirectoryPending{false};
  bool FReadDirectoryPending{false};
  bool FTunnelOpening{false};
  std::unique_ptr<TCustomFileSystem> FFileSystem{nullptr};

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
    TFileOperationProgressType *OperationProgress, HANDLE *AHandle,
    bool NoConfirmation);
  void TerminalOpenLocalFile(const UnicodeString ATargetFileName, DWORD Access,
    DWORD *AAttrs, HANDLE *AHandle, int64_t *ACTime,
    int64_t *AMTime, int64_t *AATime, int64_t *ASize, bool TryWriteReadOnly = true);
  void TerminalOpenLocalFile(
    const UnicodeString AFileName, DWORD Access, TLocalFileHandle & Handle, bool TryWriteReadOnly = true);
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
    UnicodeString AFileName, const TSearchRecSmart & Rec, /*int64_t*/ void * Size);
  TBatchOverwrite EffectiveBatchOverwrite(
    const UnicodeString ASourceFullFileName, const TCopyParamType *CopyParam, intptr_t Params,
    TFileOperationProgressType *OperationProgress, bool Special) const;
  bool CheckRemoteFile(
    const UnicodeString AFileName, const TCopyParamType *CopyParam,
    intptr_t Params, TFileOperationProgressType *OperationProgress) const;
  uintptr_t ConfirmFileOverwrite(
    const UnicodeString ASourceFullFileName, const UnicodeString ATargetFileName,
    const TOverwriteFileParams *FileParams, uintptr_t Answers, TQueryParams *QueryParams,
    TOperationSide Side, const TCopyParamType *CopyParam, intptr_t Params,
    TFileOperationProgressType *OperationProgress, const UnicodeString AMessage = L"");
  void DoSynchronizeCollectDirectory(const UnicodeString ALocalDirectory,
    const UnicodeString ARemoteDirectory, TSynchronizeMode Mode,
    const TCopyParamType *CopyParam, intptr_t AParams,
    TSynchronizeDirectoryEvent OnSynchronizeDirectory,
    TSynchronizeOptions *Options, intptr_t AFlags, TSynchronizeChecklist *Checklist);
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
  void DoSynchronizeCollectFile(UnicodeString AFileName,
    const TRemoteFile *AFile, /*TSynchronizeData* */void * Param);
  void SynchronizeCollectFile(UnicodeString AFileName,
    const TRemoteFile *AFile, /*TSynchronizeData* */ void *Param);
  void SynchronizeRemoteTimestamp(UnicodeString AFileName,
    const TRemoteFile *AFile, void *Param);
  void SynchronizeLocalTimestamp(UnicodeString AFileName,
    const TRemoteFile *AFile, void *Param);
  void DoSynchronizeProgress(const TSynchronizeData &Data, bool Collect);
  void DeleteLocalFile(UnicodeString AFileName,
    const TRemoteFile *AFile, void *Params);
  void RecycleFile(UnicodeString AFileName, const TRemoteFile *AFile);
  TStrings * GetFixedPaths() const;
  void DoStartup();
  virtual bool DoQueryReopen(Exception *E);
  void FatalError(Exception *E, const UnicodeString AMsg, const UnicodeString AHelpKeyword = "") override;
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
    TFileOperationProgressType *OperationProgress, HANDLE *AHandle,
    bool NoConfirmation);
  void LockFile(UnicodeString AFileName, const TRemoteFile *AFile, void * Param);
  void UnlockFile(UnicodeString AFileName, const TRemoteFile *AFile, void * Param);
  void DoLockFile(UnicodeString AFileName, const TRemoteFile *AFile);
  void DoUnlockFile(UnicodeString AFileName, const TRemoteFile *AFile);
  void OperationFinish(
    TFileOperationProgressType * Progress, const void * Item, const UnicodeString AFileName,
    bool Success, TOnceDoneOperation & OnceDoneOperation);
  void OperationStart(
    TFileOperationProgressType & Progress, TFileOperation Operation, TOperationSide Side, intptr_t Count);
  void OperationStart(
    TFileOperationProgressType & Progress, TFileOperation Operation, TOperationSide Side, intptr_t Count,
    bool Temp, const UnicodeString ADirectory, uint64_t CPSLimit);
  void OperationStop(TFileOperationProgressType & Progress);
  void Information(const UnicodeString AStr, bool Status) override;
  uintptr_t QueryUser(const UnicodeString AQuery,
    TStrings *MoreMessages, uintptr_t Answers, const TQueryParams *Params,
    TQueryType QueryType = qtConfirmation) override;
  uintptr_t QueryUserException(const UnicodeString AQuery,
    Exception *E, uintptr_t Answers, const TQueryParams *Params,
    TQueryType QueryType = qtConfirmation) override;
  bool PromptUser(TSessionData *Data, TPromptKind Kind,
    const UnicodeString AName, const UnicodeString AInstructions, TStrings *Prompts, TStrings *Results) override;
  void DisplayBanner(const UnicodeString ABanner) override;
  void Closed() override;
  void ProcessGUI() override;
  void Progress(TFileOperationProgressType *OperationProgress);
  void HandleExtendedException(Exception *E) override;
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
  UnicodeString FormatFileDetailsForLog(const UnicodeString AFileName, const TDateTime &AModification, int64_t Size) const;
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
    const UnicodeString ATargetDir, const TCopyParamType * CopyParam, intptr_t AParams,
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
  TFileOperationProgressType*& OperationProgress{FOperationProgress};

  const TFileOperationProgressType *GetOperationProgress() const { return FOperationProgress; }
  TFileOperationProgressType *GetOperationProgress() { return FOperationProgress; }
  void SetOperationProgress(TFileOperationProgressType *OperationProgress) { FOperationProgress = OperationProgress; }
  virtual const TTerminal *GetPasswordSource() const { return this; }

public:
  explicit TTerminal(TObjectClassId Kind = OBJECT_CLASS_TTerminal) noexcept;
  void Init(TSessionData *ASessionData, TConfiguration *AConfiguration);
  virtual ~TTerminal() noexcept;
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
    TStrings * AFilesToCopy, const UnicodeString ATargetDir, const TCopyParamType * CopyParam, intptr_t AParams,
    TParallelOperation * ParallelOperation);
  bool CopyToRemote(
    TStrings * AFilesToCopy, const UnicodeString ATargetDir, const TCopyParamType * CopyParam, intptr_t AParams,
    TParallelOperation * ParallelOperation);
  intptr_t CopyToParallel(TParallelOperation *ParallelOperation, TFileOperationProgressType *OperationProgress);
  void LogParallelTransfer(TParallelOperation *ParallelOperation);
  void RemoteCreateDirectory(const UnicodeString ADirName, const TRemoteProperties * Properties = nullptr);
  void RemoteCreateLink(UnicodeString AFileName, const UnicodeString APointTo, bool Symbolic);
  void RemoteDeleteFile(UnicodeString AFileName,
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
    intptr_t AParams, const TCopyParamType *CopyParam, bool AllowDirs,
    TCalculateSizeStats &Stats);
  bool CalculateLocalFilesSize(TStrings * FileList, int64_t & Size,
    const TCopyParamType * CopyParam, bool AllowDirs, TStrings * Files, TCalculatedSizes * CalculatedSizes);
  void CalculateFilesChecksum(const UnicodeString Alg, TStrings *AFileList,
    TStrings *Checksums, TCalculatedChecksumEvent OnCalculatedChecksum);
  void ClearCaches();
  TSynchronizeChecklist * SynchronizeCollect(const UnicodeString LocalDirectory,
    const UnicodeString ARemoteDirectory, TSynchronizeMode Mode,
    const TCopyParamType *CopyParam, intptr_t AParams,
    TSynchronizeDirectoryEvent OnSynchronizeDirectory, TSynchronizeOptions *Options);
  void SynchronizeApply(
    TSynchronizeChecklist * Checklist,
    const TCopyParamType *CopyParam, intptr_t AParams,
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
    UnicodeString AFileName, const TSearchRecSmart & Rec, void * Param);
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
  void LogEvent(UnicodeString AStr);
  void LogEvent(intptr_t Level, UnicodeString AStr);
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
  RWProperty<bool> ExceptionOnFail{nb::bind(&TTerminal::GetExceptionOnFail, this), nb::bind(&TTerminal::SetExceptionOnFail, this)};
  __property TRemoteDirectory * Files = { read = FFiles };
  __property TNotifyEvent OnChangeDirectory = { read = FOnChangeDirectory, write = FOnChangeDirectory };
  __property TReadDirectoryEvent OnReadDirectory = { read = FOnReadDirectory, write = FOnReadDirectory };
  __property TNotifyEvent OnStartReadDirectory = { read = FOnStartReadDirectory, write = FOnStartReadDirectory };
  __property TReadDirectoryProgressEvent OnReadDirectoryProgress = { read = FOnReadDirectoryProgress, write = FOnReadDirectoryProgress };
  __property TDeleteLocalFileEvent OnDeleteLocalFile = { read = FOnDeleteLocalFile, write = FOnDeleteLocalFile };
  TDeleteLocalFileEvent &OnDeleteLocalFile{FOnDeleteLocalFile};
  __property TNotifyEvent OnInitializeLog = { read = FOnInitializeLog, write = FOnInitializeLog };
  __property const TRemoteTokenList * Groups = { read = GetGroups };
  __property const TRemoteTokenList * Users = { read = GetUsers };
  __property const TRemoteTokenList * Membership = { read = GetMembership };
  __property TFileOperationProgressEvent OnProgress  = { read=FOnProgress, write=FOnProgress };
  __property TFileOperationFinished OnFinished  = { read=FOnFinished, write=FOnFinished };
  __property TCurrentFSProtocol FSProtocol = { read = FFSProtocol };
  __property bool UseBusyCursor = { read = FUseBusyCursor, write = FUseBusyCursor };
  __property UnicodeString UserName = { read=GetUserName };
  ROProperty<UnicodeString> UserName{nb::bind(&TTerminal::TerminalGetUserName, this)};
  __property bool IsCapable[TFSCapability Capability] = { read = GetIsCapable };
  __property bool AreCachesEmpty = { read = GetAreCachesEmpty };
  __property bool CommandSessionOpened = { read = GetCommandSessionOpened };
  __property TTerminal * CommandSession = { read = GetCommandSession };
  __property bool AutoReadDirectory = { read = FAutoReadDirectory, write = FAutoReadDirectory };
  __property TStrings * FixedPaths = { read = GetFixedPaths };
  __property bool ResolvingSymlinks = { read = GetResolvingSymlinks };
  __property UnicodeString Password = { read = GetPassword };
  ROProperty<UnicodeString> Password{nb::bind(&TTerminal::GetPassword, this)};
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

  TSessionData *GetSessionData() const { return FSessionData.get(); }
  TSessionLog *GetLogConst() const { return FLog.get(); }
  TSessionLog *GetLog() { return FLog.get(); }
  TActionLog *GetActionLog() const { return FActionLog.get(); }
  const TConfiguration *GetConfigurationConst() const { return FConfiguration; }
  TConfiguration *GetConfiguration() { return FConfiguration; }
  TSessionStatus GetStatus() const { return FStatus; }
  TRemoteDirectory *GetFiles() const { return FFiles.get(); }
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
  TCustomFileSystem *GetFileSystem() const { return FFileSystem.get(); }
  TCustomFileSystem *GetFileSystem() { return FFileSystem.get(); }

  HANDLE TerminalCreateLocalFile(const UnicodeString LocalFileName, DWORD DesiredAccess,
    DWORD ShareMode, DWORD CreationDisposition, DWORD FlagsAndAttributes);

  bool CheckForEsc() const;
  void SetupTunnelLocalPortNumber();
  TRemoteTokenList * GetGroups();
  TRemoteTokenList * GetUsers();
  TRemoteTokenList * GetMembership();

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
  static bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TSecondaryTerminal); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TSecondaryTerminal) || TTerminal::is(Kind); }
public:
  TSecondaryTerminal() noexcept : TTerminal(OBJECT_CLASS_TSecondaryTerminal), FMainTerminal(nullptr) {}
  explicit TSecondaryTerminal(TTerminal *MainTerminal) noexcept;
  explicit TSecondaryTerminal(TObjectClassId Kind, TTerminal *MainTerminal) noexcept;
  virtual ~TSecondaryTerminal() noexcept = default;
  void Init(TSessionData *ASessionData, TConfiguration *AConfiguration,
    const UnicodeString Name);

  void UpdateFromMain();

  __property TTerminal * MainTerminal = { read = FMainTerminal };
  TTerminal *GetMainTerminal() const { return FMainTerminal; }

protected:
  void DirectoryLoaded(TRemoteFileList *FileList) override;
  void DirectoryModified(const UnicodeString APath,
    bool SubDirs) override;
  const TTerminal *GetPasswordSource() const override { return FMainTerminal; }
  TTerminal *GetPasswordSource() override;

private:
  TTerminal *FMainTerminal{nullptr};
};
//---------------------------------------------------------------------------
NB_DEFINE_CLASS_ID(TTerminalList);
class NB_CORE_EXPORT TTerminalList : public TObjectList
{
  NB_DISABLE_COPY(TTerminalList)
public:
  explicit TTerminalList(TConfiguration *AConfiguration) noexcept;
  virtual ~TTerminalList() noexcept;

  virtual TTerminal * NewTerminal(TSessionData *Data);
  virtual void FreeTerminal(TTerminal *Terminal);
  void FreeAndNullTerminal(TTerminal *& Terminal);
  void RecryptPasswords();

  __property TTerminal *Terminals[int Index]  = { read = GetTerminal };

protected:
  virtual TTerminal * CreateTerminal(TSessionData *Data);

private:
  TConfiguration *FConfiguration{nullptr};

public:
  TTerminal * GetTerminal(intptr_t Index);
};
//---------------------------------------------------------------------------
NB_DEFINE_CLASS_ID(TCustomCommandParams);
struct NB_CORE_EXPORT TCustomCommandParams : public TObject
{
public:
  static bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TCustomCommandParams); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TCustomCommandParams) || TObject::is(Kind); }
public:
  TCustomCommandParams() : TObject(OBJECT_CLASS_TCustomCommandParams) {}
  UnicodeString Command;
  intptr_t Params{0};
  TCaptureOutputEvent OutputEvent;
};
//---------------------------------------------------------------------------
struct NB_CORE_EXPORT TCalculateSizeStats : public TObject
{
  TCalculateSizeStats() noexcept;

  intptr_t Files{0};
  intptr_t Directories{0};
  intptr_t SymLinks{0};
  TStrings *FoundFiles{nullptr};
  TCalculatedSizes * CalculatedSizes{nullptr};
};
//---------------------------------------------------------------------------
NB_DEFINE_CLASS_ID(TCalculateSizeParams);
struct NB_CORE_EXPORT TCalculateSizeParams : public TObject
{
public:
  static bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TCalculateSizeParams); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TCalculateSizeParams) || TObject::is(Kind); }
public:
  TCalculateSizeParams() noexcept;
  int64_t Size{0};
  intptr_t Params{0};
  const TCopyParamType *CopyParam{nullptr};
  TCalculateSizeStats *Stats{nullptr};
  TCollectedFileList *Files{nullptr};
  UnicodeString LastDirPath;
  bool AllowDirs{true};
  bool Result{true};
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
using TDateTimes = rde::vector<TDateTime>;
//---------------------------------------------------------------------------
NB_DEFINE_CLASS_ID(TMakeLocalFileListParams);
struct NB_CORE_EXPORT TMakeLocalFileListParams : public TObject
{
public:
  static bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TMakeLocalFileListParams); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TMakeLocalFileListParams) || TObject::is(Kind); }
public:
  TMakeLocalFileListParams() : TObject(OBJECT_CLASS_TMakeLocalFileListParams) {}
  TStrings *FileList{nullptr};
  TDateTimes *FileTimes{nullptr};
  bool IncludeDirs{false};
  bool Recursive{false};
};
//---------------------------------------------------------------------------
struct NB_CORE_EXPORT TSynchronizeOptions : public TObject
{
  NB_DISABLE_COPY(TSynchronizeOptions)
public:
  TSynchronizeOptions() noexcept;
  ~TSynchronizeOptions() noexcept;

  TStringList *Filter{nullptr};

  bool FilterFind(const UnicodeString AFileName) const;
  bool MatchesFilter(const UnicodeString AFileName) const;
};
//---------------------------------------------------------------------------
struct NB_CORE_EXPORT TSpaceAvailable
{
  CUSTOM_MEM_ALLOCATION_IMPL
  TSpaceAvailable();

  int64_t BytesOnDevice{0};
  int64_t UnusedBytesOnDevice{0};
  int64_t BytesAvailableToUser{0};
  int64_t UnusedBytesAvailableToUser{0};
  uintptr_t BytesPerAllocationUnit{0};
};
//---------------------------------------------------------------------------
class NB_CORE_EXPORT TRobustOperationLoop : public TObject
{
  NB_DISABLE_COPY(TRobustOperationLoop)
public:
  explicit TRobustOperationLoop(TTerminal *Terminal, TFileOperationProgressType *OperationProgress, bool *AnyTransfer = nullptr) noexcept;
  virtual ~TRobustOperationLoop() noexcept;
  bool TryReopen(Exception &E);
  bool ShouldRetry() const;
  bool Retry();

private:
  TTerminal *FTerminal{nullptr};
  TFileOperationProgressType *FOperationProgress{nullptr};
  bool FRetry{false};
  bool *FAnyTransfer{nullptr};
  bool FPrevAnyTransfer{false};
  TDateTime FStart;
};
//---------------------------------------------------------------------------
NB_DEFINE_CLASS_ID(TCollectedFileList);
class TCollectedFileList : public TObject
{
public:
  static bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TCollectedFileList); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TCollectedFileList) || TObject::is(Kind); }
public:
  TCollectedFileList() noexcept;
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
    TObject *Object{nullptr};
    bool Dir{false};
    bool Recursed{false};
  };
  using TFileDataList = rde::vector<TFileData>;
  TFileDataList FList;
};
//---------------------------------------------------------------------------
class TParallelOperation : public TObject
{
public:
  explicit TParallelOperation(TOperationSide Side) noexcept;
  virtual ~TParallelOperation() noexcept;

  void Init(
    TStrings *AFiles, const UnicodeString ATargetDir, const TCopyParamType *CopyParam, intptr_t AParams,
    TFileOperationProgressType *MainOperationProgress, const UnicodeString AMainName);

  bool IsInitialized() const;
  void WaitFor();
  bool ShouldAddClient() const;
  void AddClient();
  void RemoveClient();
  intptr_t GetNext(
    TTerminal *Terminal, UnicodeString &AFileName, TObject *&Object, UnicodeString &ATargetDir,
    bool &Dir, bool &Recursed, bool &FirstLevel);
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
    bool Exists{false};
  };

  std::unique_ptr<TStrings> FFileList;
  intptr_t FIndex{0};
  typedef rde::map<UnicodeString, TDirectoryData> TDirectories;
  TDirectories FDirectories;
  UnicodeString FTargetDir;
  const TCopyParamType *FCopyParam{nullptr};
  intptr_t FParams{0};
  bool FProbablyEmpty{false};
  intptr_t FClients{0};
  std::unique_ptr<TCriticalSection> FSection;
  TFileOperationProgressType *FMainOperationProgress{nullptr};
  TOperationSide FSide{osLocal};
  UnicodeString FMainName;

  bool CheckEnd(TCollectedFileList *Files);
};
//---------------------------------------------------------------------------
struct TLocalFileHandle
{
  TLocalFileHandle() noexcept;
  ~TLocalFileHandle() noexcept;

  void Dismiss();
  void Close();
  void Release();

  UnicodeString FileName;
  HANDLE Handle{0};
  DWORD Attrs{0};
  bool Directory{false};
  TDateTime Modification;
  int64_t MTime{0};
  int64_t ATime{0};
  int64_t Size{0};
  CUSTOM_MEM_ALLOCATION_IMPL
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
