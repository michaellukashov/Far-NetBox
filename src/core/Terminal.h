
#pragma once

#include <rdestl/map.h>
#include <rdestl/set.h>
#include <rdestl/vector.h>

#include <Classes.hpp>
#include <Common.h>
#include <Exceptions.h>

#include "SessionInfo.h"
#include "Interface.h"
#include "FileOperationProgress.h"
#include "FileMasks.h"
#include "RemoteFiles.h"
#include "Exceptions.h"

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
struct TNeonCertificateData;
using TCalculatedSizes = nb::vector_t<int64_t>;

using TQueryUserEvent = nb::FastDelegate8<void,
  TObject * /*Sender*/, const UnicodeString & /*AQuery*/, TStrings * /*MoreMessages*/, uint32_t /*Answers*/,
   const TQueryParams * /*Params*/, uint32_t & /*Answer*/, TQueryType /*QueryType*/, void * /*Arg*/>;
using TPromptUserEvent = nb::FastDelegate8<void,
  TTerminal * /*Terminal*/, TPromptKind /*Kind*/, const UnicodeString & /*AName*/, const UnicodeString & /*AInstructions*/,
  TStrings * /*Prompts*/, TStrings * /*Results*/, bool & /*Result*/, void * /*Arg*/>;
using TDisplayBannerEvent = nb::FastDelegate6<void,
  TTerminal * /*Terminal*/, const UnicodeString & /*ASessionName*/, const UnicodeString & /*ABanner*/,
  bool & /*NeverShowAgain*/, int32_t /*Options*/, uint32_t & /*Params*/>;
using TExtendedExceptionEvent = nb::FastDelegate3<void,
  TTerminal * /*Terminal*/, Exception * /*E*/, void * /*Arg*/ >;
using TReadDirectoryEvent = nb::FastDelegate2<void, TObject * /*Sender*/, Boolean /*ReloadOnly*/>;
using TReadDirectoryProgressEvent = nb::FastDelegate4<void,
  TObject * /*Sender*/, int32_t /*Progress*/, int32_t /*ResolvedLinks*/, bool & /*Cancel*/>;
using TProcessFileEvent = nb::FastDelegate3<void,
  const UnicodeString & /*AFileName*/, const TRemoteFile * /*AFile*/, void * /*AParam*/>;
using TProcessFileEventEx = nb::FastDelegate4<void,
  const UnicodeString & /*AFileName*/, const TRemoteFile * /*File*/,
  void * /*Param*/, int32_t /*Index*/>;
using TFileOperationEvent = nb::FastDelegate2<int32_t,
  void * /*Param1*/, void * /*Param2*/>;
using TSynchronizeDirectoryEvent = nb::FastDelegate5<void,
  const UnicodeString & /*LocalDirectory*/, const UnicodeString & /*RemoteDirectory*/,
  bool & /*Continue*/, bool /*Collect*/, const TSynchronizeOptions * /*Options*/>;
using TUpdatedSynchronizationChecklistItems = nb::FastDelegate1<void,
  const TSynchronizeChecklist::TItemList & /*Items*/>;
using TProcessedSynchronizationChecklistItem = nb::FastDelegate2<void,
  void * /*Token*/, const TChecklistItem * /*Item*/>;
using TDeleteLocalFileEvent = nb::FastDelegate3<void,
  const UnicodeString & /*FileName*/, bool /*Alternative*/, int32_t & /*Deleted*/>;
using TDirectoryModifiedEvent = nb::FastDelegate3<int,
  TTerminal * /*Terminal*/, const UnicodeString & /*Directory*/, bool /*SubDirs*/>;
using TInformationEvent = nb::FastDelegate5<void,
  TTerminal * /*Terminal*/, const UnicodeString & /*Str*/, bool /*Status*/, int32_t /*Phase*/, const UnicodeString & /*Additional*/>;
using TCustomCommandEvent = nb::FastDelegate3<void,
  TTerminal * /*Terminal*/, const UnicodeString & /*Command*/, bool & /*Handled*/>;

using TCreateLocalFileEvent = nb::FastDelegate5<HANDLE,
  const UnicodeString & /*AFileName*/, DWORD /*DesiredAccess*/,
  DWORD /*ShareMode*/, DWORD /*CreationDisposition*/,
  DWORD /*FlagsAndAttributes*/>;
using TGetLocalFileAttributesEvent = nb::FastDelegate1<DWORD,
  const UnicodeString & /*AFileName*/>;
using TSetLocalFileAttributesEvent = nb::FastDelegate2<bool,
  const UnicodeString & /*AFileName*/, DWORD /*FileAttributes*/>;
using TMoveLocalFileEvent = nb::FastDelegate3<bool,
  const UnicodeString & /*AFileName*/, const UnicodeString & /*ANewFileName*/,
  DWORD /*Flags*/>;
using TRemoveLocalDirectoryEvent = nb::FastDelegate1<bool,
  const UnicodeString & /*ALocalDirName*/>;
using TCreateLocalDirectoryEvent = nb::FastDelegate2<bool,
  const UnicodeString & /*ALocalDirName*/,
  LPSECURITY_ATTRIBUTES /*SecurityAttributes*/>;
using TCheckForEscEvent = nb::FastDelegate0<bool>;

constexpr const uint32_t folNone = 0x00;
constexpr const uint32_t folAllowSkip = 0x01;
constexpr const uint32_t folRetryOnFatal = 0x02;

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
#endif

NB_CORE_EXPORT void FileOperationLoopCustom(TTerminal * Terminal,
  TFileOperationProgressType * OperationProgress,
  uint32_t Flags, const UnicodeString & Message,
  const UnicodeString & HelpKeyword,
  std::function<void()> Operation);

#define FILE_OPERATION_LOOP_BEGIN(TERMINAL, OPERATION_PROGRESS, FLAGS, MESSAGE, HELPKEYWORD) \
  FileOperationLoopCustom((TERMINAL), (OPERATION_PROGRESS), (FLAGS), \
    (MESSAGE), HELPKEYWORD, \
  [&]() \


#define FILE_OPERATION_LOOP_END_CUSTOM(MESSAGE, FLAGS, HELPKEYWORD) );

#define FILE_OPERATION_LOOP_END_EX(MESSAGE, FLAGS) \
  FILE_OPERATION_LOOP_END_CUSTOM(MESSAGE, FLAGS, L"")
#define FILE_OPERATION_LOOP_END(MESSAGE) \
  FILE_OPERATION_LOOP_END_EX(MESSAGE, folAllowSkip)

enum TCurrentFSProtocol { cfsUnknown, cfsSCP, cfsSFTP, cfsFTP, cfsWebDAV, cfsS3 }; //cfsFTPS, 

constexpr int32_t cpDelete = 0x01;
constexpr int32_t cpTemporary = 0x04;
constexpr int32_t cpNoConfirmation = 0x08;
constexpr int32_t cpAppend = 0x20;
constexpr int32_t cpResume = 0x40;
constexpr int32_t cpNoRecurse = 0x80;
//constexpr int32_t cpNewerOnly = 0x100;
//constexpr int32_t cpFirstLevel = 0x200;

constexpr int32_t ccApplyToDirectories = 0x01;
constexpr int32_t ccRecursive = 0x02;
constexpr int32_t ccUser = 0x100;

constexpr int32_t csIgnoreErrors = 0x01;
constexpr int32_t csStopOnFirstFile = 0x02;
constexpr int32_t csDisallowTemporaryTransferFiles = 0x04;

constexpr int32_t ropNoReadDirectory = 0x02;

constexpr int32_t boDisableNeverShowAgain = 0x01;
constexpr int32_t bpMonospacedFont = 0x01;

constexpr int32_t tfNone = 0x00;
constexpr int32_t tfFirstLevel = 0x01;
constexpr int32_t tfNewDirectory = 0x02;
constexpr int32_t tfAutoResume = 0x04;
constexpr int32_t tfPreCreateDir = 0x08;
constexpr int32_t tfUseFileTransferAny = 0x10;

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
  static constexpr int32_t spDelete = 0x01; // cannot be combined with spTimestamp
  static constexpr int32_t spNoConfirmation = 0x02; // has no effect for spTimestamp
  static constexpr int32_t spExistingOnly = 0x04; // is implicit for spTimestamp
  static constexpr int32_t spNoRecurse = 0x08;
  static constexpr int32_t spUseCache = 0x10; // cannot be combined with spTimestamp
  static constexpr int32_t spDelayProgress = 0x20; // cannot be combined with spTimestamp
  static constexpr int32_t spPreviewChanges = 0x40; // not used by core
  static constexpr int32_t spSubDirs = 0x80; // cannot be combined with spTimestamp
  static constexpr int32_t spTimestamp = 0x100;
  static constexpr int32_t spNotByTime = 0x200; // cannot be combined with spTimestamp and smBoth
  static constexpr int32_t spBySize = 0x400; // cannot be combined with smBoth, has opposite meaning for spTimestamp
  static constexpr int32_t spSelectedOnly = 0x800; // not used by core
  static constexpr int32_t spMirror = 0x1000;
  static constexpr int32_t spCaseSensitive = 0x2000;
  static constexpr int32_t spByChecksum = 0x4000; // cannot be combined with spTimestamp and smBoth
  static constexpr int32_t spDefault = TTerminal::spNoConfirmation | TTerminal::spPreviewChanges;

private:
  TCheckForEscEvent FOnCheckForEsc{nullptr};
  TCreateLocalFileEvent FOnCreateLocalFile;
  TGetLocalFileAttributesEvent FOnGetLocalFileAttributes;
  TSetLocalFileAttributesEvent FOnSetLocalFileAttributes;
  TMoveLocalFileEvent FOnMoveLocalFile;
  TRemoveLocalDirectoryEvent FOnRemoveLocalDirectory;
  TCreateLocalDirectoryEvent FOnCreateLocalDirectory;

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
friend class TParallelOperation;

private:
  std::unique_ptr<TSessionData> FSessionData;
  std::unique_ptr<TSessionLog> FLog;
  std::unique_ptr<TActionLog> FActionLog;
  bool FActionLogOwned{false};
  TConfiguration * FConfiguration{nullptr};
  UnicodeString FCurrentDirectory;
  int32_t FExceptionOnFail{0};
  std::unique_ptr<TRemoteDirectory> FFiles;
  int32_t FInTransaction{0};
  bool FSuspendTransaction{false};
  TNotifyEvent FOnChangeDirectory;
  TReadDirectoryEvent FOnReadDirectory;
  TNotifyEvent FOnStartReadDirectory;
  TReadDirectoryProgressEvent FOnReadDirectoryProgress;
  TDeleteLocalFileEvent FOnDeleteLocalFile;
  TNotifyEvent FOnInitializeLog;
  TRemoteTokenList FMembership;
  TRemoteTokenList FGroups;
  TRemoteTokenList FUsers;
  bool FUsersGroupsLookedup{false};
  TFileOperationProgressEvent FOnProgress{nullptr};
  TFileOperationFinishedEvent FOnFinished{nullptr};
  TFileOperationProgressType * FOperationProgress{nullptr};
  bool FUseBusyCursor{false};
  std::unique_ptr<TRemoteDirectoryCache> FDirectoryCache;
  std::unique_ptr<TRemoteDirectoryChangesCache> FDirectoryChangesCache;
  std::unique_ptr<TSecureShell> FSecureShell;
  UnicodeString FLastDirectoryChange;
  TCurrentFSProtocol FFSProtocol{cfsUnknown};
  TTerminal * FCommandSession{nullptr};
  bool FAutoReadDirectory{true};
  bool FReadingCurrentDirectory{false};
  bool * FClosedOnCompletion{nullptr};
  TSessionStatus FStatus{ssClosed};
  int32_t FOpening{0};
  RawByteString FRememberedPassword;
  TPromptKind FRememberedPasswordKind{};
  RawByteString FRememberedTunnelPassword;
  bool FRememberedPasswordUsed{false};
  std::unique_ptr<TTunnelThread> FTunnelThread;
  std::unique_ptr<TSecureShell> FTunnel;
  std::unique_ptr<TSessionData> FTunnelData;
  std::unique_ptr<TSessionLog> FTunnelLog;
  std::unique_ptr<TTunnelUI> FTunnelUI;
  int32_t FTunnelLocalPortNumber{0};
  UnicodeString FTunnelError;
  TQueryUserEvent FOnQueryUser{nullptr};
  TPromptUserEvent FOnPromptUser{nullptr};
  TDisplayBannerEvent FOnDisplayBanner{nullptr};
  TExtendedExceptionEvent FOnShowExtendedException{nullptr};
  TInformationEvent FOnInformation{nullptr};
  TCustomCommandEvent FOnCustomCommand{nullptr};
  TNotifyEvent FOnClose{nullptr};
  TCallbackGuard * FCallbackGuard{nullptr};
  TFindingFileEvent FOnFindingFile{nullptr};
  std::unique_ptr<TStrings> FShellChecksumAlgDefs;
  bool FEnableSecureShellUsage{false};
  bool FCollectFileSystemUsage{false};
  bool FRememberedPasswordTried{false};
  bool FRememberedTunnelPasswordTried{false};
  int32_t FNesting{0};
  UnicodeString FFingerprintScannedSHA256;
  UnicodeString FFingerprintScannedSHA1;
  UnicodeString FFingerprintScannedMD5;
  DWORD FLastProgressLogged{0};
  UnicodeString FDestFileName;
  bool FMultipleDestinationFiles{false};
  bool FFileTransferAny{true};
  using TEncryptedFileNames = nb::map_t<UnicodeString, UnicodeString>;
  TEncryptedFileNames FEncryptedFileNames;
  nb::set_t<UnicodeString> FFoldersScannedForEncryptedFiles;
  RawByteString FEncryptKey;
  TFileOperationProgressType::TPersistence * FOperationProgressPersistence{nullptr};
  TOnceDoneOperation FOperationProgressOnceDoneOperation{odoIdle};
  UnicodeString FCollectedCalculatedChecksum;

public:
  void CommandError(Exception * E, const UnicodeString & AMsg);
  uint32_t CommandError(Exception * E, const UnicodeString & AMsg,
    uint32_t Answers, const UnicodeString & HelpKeyword = "");
  UnicodeString RemoteGetCurrentDirectory();
  bool GetExceptionOnFail() const;
  const TRemoteTokenList * GetGroups() const { return const_cast<TTerminal *>(this)->GetGroups(); }
  const TRemoteTokenList * GetUsers() const { return const_cast<TTerminal *>(this)->GetUsers(); }
  const TRemoteTokenList * GetMembership() const { return const_cast<TTerminal *>(this)->GetMembership(); }
  void TerminalSetCurrentDirectory(const UnicodeString & AValue);
  void SetExceptionOnFail(bool Value);
  void ReactOnCommand(int32_t /*TFSCommand*/ ACmd);
  UnicodeString TerminalGetUserName() const;
  bool GetAreCachesEmpty() const;
  void ClearCachedFileList(const UnicodeString & APath, bool SubDirs);
  void AddCachedFileList(TRemoteFileList * FileList);
  bool GetCommandSessionOpened() const;
  TTerminal * GetCommandSession();
  bool GetResolvingSymlinks() const;
  bool GetActive() const;
  UnicodeString GetPassword() const;
  UnicodeString GetRememberedPassword() const;
  UnicodeString GetRememberedTunnelPassword() const;
  bool GetStoredCredentialsTried() const;
  bool InTransaction() const;
  void SaveCapabilities(TFileSystemInfo & FileSystemInfo);
  bool CreateTargetDirectory(const UnicodeString & DirectoryPath, uint32_t Attrs, const TCopyParamType * CopyParam);
  UnicodeString CutFeature(UnicodeString & Buf);
  static UnicodeString SynchronizeModeStr(TSynchronizeMode Mode);
  static UnicodeString SynchronizeParamsStr(int32_t Params);

protected:
  bool FReadCurrentDirectoryPending{false};
  bool FReadDirectoryPending{false};
  bool FTunnelOpening{false};
  std::unique_ptr<TCustomFileSystem> FFileSystem;
  int32_t FSecondaryTerminals{0};

  void DoStartReadDirectory();
  void DoReadDirectoryProgress(int32_t Progress, int32_t ResolvedLinks, bool & Cancel);
  void DoReadDirectory(bool ReloadOnly);
  void DoCreateDirectory(const UnicodeString & ADirName, bool Encrypt);
  void DoDeleteFile(
    TCustomFileSystem * FileSystem, const UnicodeString & AFileName, const TRemoteFile * AFile, int32_t Params);
  void DoCustomCommandOnFile(const UnicodeString & AFileName,
    const TRemoteFile * AFile, const UnicodeString & ACommand, int32_t AParams, TCaptureOutputEvent OutputEvent);
  bool DoRenameOrCopyFile(
    bool Rename, const UnicodeString & FileName, const TRemoteFile * File, const UnicodeString & NewName,
    bool Move, bool DontOverwrite, bool IsBatchOperation);
  bool DoRenameFile(
    const UnicodeString & AFileName, const TRemoteFile * AFile, const UnicodeString & ANewName, bool Move, bool DontOverwrite);
  bool DoMoveFile(const UnicodeString & FileName, const TRemoteFile * File, /*const TMoveFileParams*/ void * Param);
  void DoCopyFile(
    const UnicodeString & AFileName, const TRemoteFile * AFile, const UnicodeString & ANewName, bool DontOverwrite);
  void DoChangeFileProperties(const UnicodeString & AFileName,
    const TRemoteFile * AFile, const TRemoteProperties * Properties);
  void DoChangeDirectory();
  void DoInitializeLog();
  void EnsureNonExistence(const UnicodeString & AFileName);
  void LookupUsersGroups();
  void FileModified(const TRemoteFile * AFile,
    const UnicodeString & AFileName, bool ClearDirectoryChange = false);
  int32_t FileOperationLoop(TFileOperationEvent CallBackFunc,
    TFileOperationProgressType * OperationProgress, uint32_t AFlags,
    const UnicodeString & Message, void * Param1 = nullptr, void * Param2 = nullptr);
  bool GetIsCapableProtected(TFSCapability Capability) const;
  bool ProcessFiles(TStrings * AFileList, TFileOperation Operation,
    TProcessFileEvent ProcessFile, void * Param = nullptr, TOperationSide Side = osRemote,
    bool Ex = false);
  bool ProcessFilesEx(TStrings * FileList, TFileOperation Operation,
    TProcessFileEventEx ProcessFile, void * AParam = nullptr, TOperationSide Side = osRemote);
  void ProcessDirectory(const UnicodeString & ADirName,
    TProcessFileEvent CallBackFunc, void * AParam = nullptr, bool UseCache = false,
    bool IgnoreErrors = false);
  bool DeleteContentsIfDirectory(
    const UnicodeString & AFileName, const TRemoteFile * AFile, int32_t AParams, TRmSessionAction & Action);
  void AnnounceFileListOperation();
  void ReadDirectory(TRemoteFileList * AFileList);
  void CustomReadDirectory(TRemoteFileList * AFileList);
  void DoCreateLink(const UnicodeString & AFileName, const UnicodeString & APointTo, bool Symbolic);
  bool TerminalCreateLocalFile(const UnicodeString & ATargetFileName,
    TFileOperationProgressType * OperationProgress, HANDLE * AHandle,
    bool NoConfirmation);
  void TerminalOpenLocalFile(const UnicodeString & ATargetFileName, DWORD Access,
    DWORD * AAttrs, HANDLE * AHandle, int64_t * ACTime, int64_t * AMTime,
    int64_t * AATime, int64_t * ASize, bool TryWriteReadOnly = true);
  void TerminalOpenLocalFile(
    const UnicodeString & AFileName, DWORD Access, TLocalFileHandle & Handle, bool TryWriteReadOnly = true);
  bool AllowLocalFileTransfer(
    const UnicodeString & AFileName, const TSearchRecSmart * SearchRec,
    const TCopyParamType * CopyParam, TFileOperationProgressType * OperationProgress);
  bool HandleException(Exception * E);
  void CalculateFileSize(const UnicodeString & AFileName,
    const TRemoteFile * AFile, /*TCalculateSizeParams*/ void * ASize);
  void DoCalculateFileSize(const UnicodeString & AFileName,
    const TRemoteFile * AFile, void * AParam);
  bool DoCalculateDirectorySize(const UnicodeString & AFileName, TCalculateSizeParams * Params);
  void CalculateLocalFileSize(
    const UnicodeString & AFileName, const TSearchRecSmart & Rec, /*int64_t*/ void * Size);
  TBatchOverwrite EffectiveBatchOverwrite(
    const UnicodeString & ASourceFullFileName, const TCopyParamType * CopyParam, int32_t Params,
    TFileOperationProgressType * OperationProgress, bool Special) const;
  bool CheckRemoteFile(
    const UnicodeString & AFileName, const TCopyParamType * CopyParam,
    int32_t Params, TFileOperationProgressType * OperationProgress) const;
  uint32_t ConfirmFileOverwrite(
    const UnicodeString & ASourceFullFileName, const UnicodeString & ATargetFileName,
    const TOverwriteFileParams * FileParams, uint32_t Answers, TQueryParams * QueryParams,
    TOperationSide Side, const TCopyParamType * CopyParam, int32_t Params,
    TFileOperationProgressType * OperationProgress, const UnicodeString & AMessage = L"");
  void DoSynchronizeCollectDirectory(const UnicodeString & ALocalDirectory,
    const UnicodeString & ARemoteDirectory, TSynchronizeMode Mode,
    const TCopyParamType * CopyParam, int32_t AParams,
    TSynchronizeDirectoryEvent OnSynchronizeDirectory,
    TSynchronizeOptions * Options, int32_t AFlags, TSynchronizeChecklist * Checklist);
  bool LocalFindFirstLoop(const UnicodeString & ADirectory, TSearchRecChecked & SearchRec);
  bool LocalFindNextLoop(TSearchRecChecked & SearchRec);
  bool DoAllowLocalFileTransfer(
    const UnicodeString & AFileName, const TSearchRecSmart & SearchRec, const TCopyParamType * CopyParam, bool DisallowTemporaryTransferFiles);
  bool DoAllowRemoteFileTransfer(
    const TRemoteFile * File, const TCopyParamType * CopyParam, bool DisallowTemporaryTransferFiles);
  bool IsEmptyLocalDirectory(
    const UnicodeString & ALocalDirectory, const TCopyParamType * CopyParam, bool DisallowTemporaryTransferFiles);
  bool IsEmptyRemoteDirectory(
    const TRemoteFile * File, const TCopyParamType * CopyParam, bool DisallowTemporaryTransferFiles);
  void DoSynchronizeCollectFile(const UnicodeString & AFileName,
    const TRemoteFile * AFile, /*TSynchronizeData* */ void * Param);
  void SynchronizeCollectFile(const UnicodeString & AFileName,
    const TRemoteFile * AFile, /*TSynchronizeData* */ void * Param);
  bool SameFileChecksum(const UnicodeString & LocalFileName, const TRemoteFile * File);
  void CollectCalculatedChecksum(
    const UnicodeString & FileName, const UnicodeString & Alg, const UnicodeString & Hash);
  void SynchronizeRemoteTimestamp(const UnicodeString & FileName,
    const TRemoteFile * AFile, void * Param);
  void SynchronizeLocalTimestamp(const UnicodeString & AFileName,
    const TRemoteFile * AFile, void * Param);
  void DoSynchronizeProgress(const TSynchronizeData & Data, bool Collect);
  void DeleteLocalFile(const UnicodeString & AFileName,
    const TRemoteFile * AFile, void * Params);
  bool RecycleFile(const UnicodeString & AFileName, const TRemoteFile * AFile);
  TStrings * GetFixedPaths() const;
  void DoStartup();
  virtual bool DoQueryReopen(Exception * E);
  virtual void FatalError(Exception * E, const UnicodeString & AMsg, const UnicodeString & AHelpKeyword = "") override;
  void ResetConnection();
  virtual bool DoPromptUser(TSessionData * Data, TPromptKind Kind,
    const UnicodeString & AName, const UnicodeString & AInstructions, TStrings * Prompts,
    TStrings * Response);
  void OpenTunnel();
  void CloseTunnel();
  void DoInformation(
    const UnicodeString & AStr, bool Status, int32_t Phase = -1, const UnicodeString & Additional = UnicodeString());
  bool PromptUser(TSessionData * Data, TPromptKind Kind,
    const UnicodeString & AName, const UnicodeString & AInstructions, const UnicodeString & Prompt, bool Echo,
    int32_t MaxLen, UnicodeString & AResult);
  void FileFind(const UnicodeString & AFileName, const TRemoteFile * AFile, void * Param);
  void DoFilesFind(const UnicodeString & ADirectory, TFilesFindParams & Params, const UnicodeString & ARealDirectory);
  bool DoCreateLocalFile(const UnicodeString & AFileName,
    TFileOperationProgressType * OperationProgress, HANDLE * AHandle,
    bool NoConfirmation);
  void LockFile(const UnicodeString & AFileName, const TRemoteFile * AFile, void * Param);
  void UnlockFile(const UnicodeString & AFileName, const TRemoteFile * AFile, void * Param);
  void DoLockFile(const UnicodeString & AFileName, const TRemoteFile * AFile);
  void DoUnlockFile(const UnicodeString & AFileName, const TRemoteFile * AFile);
  void OperationFinish(
    TFileOperationProgressType * Progress, const void * Item, const UnicodeString & AFileName,
    bool Success, TOnceDoneOperation & OnceDoneOperation);
  void OperationStart(
    TFileOperationProgressType & Progress, TFileOperation Operation, TOperationSide Side, int32_t Count);
  void OperationStart(
    TFileOperationProgressType & Progress, TFileOperation Operation, TOperationSide Side, int32_t Count,
    bool Temp, const UnicodeString & ADirectory, uint32_t CPSLimit, TOnceDoneOperation OnceDoneOperation);
  void OperationStop(TFileOperationProgressType & Progress);
  virtual void Information(const UnicodeString & AStr, bool Status) override;
  virtual uint32_t QueryUser(const UnicodeString & AQuery,
    TStrings * MoreMessages, uint32_t Answers, const TQueryParams * Params,
    TQueryType QueryType = qtConfirmation) override;
  virtual uint32_t QueryUserException(const UnicodeString & AQuery,
    Exception * E, uint32_t Answers, const TQueryParams * Params,
    TQueryType QueryType = qtConfirmation) override;
  virtual bool PromptUser(TSessionData * Data, TPromptKind Kind,
    const UnicodeString & AName, const UnicodeString & AInstructions, TStrings * Prompts, TStrings * Results) override;
  virtual void DisplayBanner(const UnicodeString & ABanner) override;
  virtual void Closed() override;
  virtual void ProcessGUI() override;
  void Progress(TFileOperationProgressType * OperationProgress);
  virtual void HandleExtendedException(Exception * E) override;
  bool IsListenerFree(uint32_t PortNumber) const;
  void DoProgress(TFileOperationProgressType & ProgressData);
  void DoFinished(TFileOperation Operation, TOperationSide Side, bool Temp,
    const UnicodeString & AFileName, bool Success, TOnceDoneOperation & OnceDoneOperation);
  void RollbackAction(TSessionAction & Action,
    TFileOperationProgressType * OperationProgress, Exception * E = nullptr);
  void DoAnyCommand(const UnicodeString & ACommand, TCaptureOutputEvent OutputEvent,
    TCallSessionAction * Action);
  TRemoteFileList * DoReadDirectoryListing(const UnicodeString & ADirectory, bool UseCache);
  RawByteString EncryptPassword(const UnicodeString & APassword) const;
  UnicodeString DecryptPassword(const RawByteString & APassword) const;
  UnicodeString GetRemoteFileInfo(TRemoteFile * AFile) const;
  void LogRemoteFile(TRemoteFile * AFile);
  UnicodeString FormatFileDetailsForLog(
    const UnicodeString & AFileName, TDateTime AModification, int64_t Size, const TRemoteFile * LinkedFile = nullptr) const;
  void LogFileDetails(const UnicodeString & FileName, TDateTime Modification, int64_t Size, const TRemoteFile * LinkedFile = nullptr);
  void LogFileDone(
    TFileOperationProgressType * OperationProgress, const UnicodeString & DestFileName,
    TTransferSessionAction & Action);
  void LogTotalTransferDetails(
    const UnicodeString & ATargetDir, const TCopyParamType * CopyParam,
    TFileOperationProgressType * OperationProgress, bool Parallel, TStrings * AFiles);
  void LogTotalTransferDone(TFileOperationProgressType * OperationProgress);
  virtual TTerminal * GetPrimaryTerminal();
  void DoEndTransaction(bool Inform);
  bool VerifyCertificate(
    const UnicodeString & CertificateStorageKey, const UnicodeString & SiteKey,
    const UnicodeString & FingerprintSHA1, const UnicodeString & FingerprintSHA256,
    const UnicodeString & CertificateSubject, int32_t Failures);
  void CacheCertificate(const UnicodeString & CertificateStorageKey,
    const UnicodeString & SiteKey, const UnicodeString & FingerprintSHA1, const UnicodeString & FingerprintSHA256,
    int32_t Failures);
  bool ConfirmCertificate(
    TSessionInfo & SessionInfo, int32_t AFailures, const UnicodeString & ACertificateStorageKey, bool CanRemember);
  bool VerifyOrConfirmHttpCertificate(
    const UnicodeString & AHostName, int32_t APortNumber, const TNeonCertificateData & Data, bool CanRemember,
    TSessionInfo & SessionInfo);
  void CollectTlsUsage(const UnicodeString & TlsVersionStr);
  bool LoadTlsCertificate(X509 *& Certificate, EVP_PKEY *& PrivateKey);
  bool TryStartOperationWithFile(
    const UnicodeString & AFileName, TFileOperation Operation1, TFileOperation Operation2 = foNone);
  void StartOperationWithFile(
    const UnicodeString & AFileName, TFileOperation Operation1, TFileOperation Operation2 = foNone);
  bool CanRecurseToDirectory(const TRemoteFile * File) const;
  bool DoOnCustomCommand(const UnicodeString & ACommand);
  bool CanParallel(const TCopyParamType * CopyParam, int32_t AParams, TParallelOperation * ParallelOperation) const;
  void CopyParallel(TParallelOperation * ParallelOperation, TFileOperationProgressType * OperationProgress);
  void DoCopyToRemote(
    TStrings * AFilesToCopy, const UnicodeString & ATargetDir, const TCopyParamType * CopyParam, int32_t AParams,
    TFileOperationProgressType * OperationProgress, uint32_t AFlags, TOnceDoneOperation & OnceDoneOperation);
  void SourceRobust(
    const UnicodeString & AFileName, const TSearchRecSmart * SearchRec,
    const UnicodeString & ATargetDir, const TCopyParamType * CopyParam, int32_t AParams,
    TFileOperationProgressType * OperationProgress, uint32_t AFlags);
  void Source(
    const UnicodeString & AFileName, const TSearchRecSmart * SearchRec,
    const UnicodeString & ATargetDir, const TCopyParamType * CopyParam, int32_t AParams,
    TFileOperationProgressType * OperationProgress, uint32_t AFlags, TUploadSessionAction & Action, bool & ChildError);
  void DirectorySource(
    const UnicodeString & ADirectoryName, const UnicodeString & ATargetDir, const UnicodeString & ADestDirectoryName,
    uint32_t Attrs, const TCopyParamType * CopyParam, int32_t AParams,
    TFileOperationProgressType * OperationProgress, uint32_t AFlags);
  bool UseAsciiTransfer(
    const UnicodeString & BaseFileName, TOperationSide Side, const TCopyParamType * CopyParam,
    const TFileMasks::TParams & MaskParams);
  void SelectTransferMode(
    const UnicodeString & ABaseFileName, TOperationSide Side, const TCopyParamType * CopyParam,
    const TFileMasks::TParams & MaskParams);
  void SelectSourceTransferMode(const TLocalFileHandle & Handle, const TCopyParamType * CopyParam);
  void DoDeleteLocalFile(const UnicodeString & FileName);
  void DoRenameLocalFileForce(const UnicodeString & OldName, const UnicodeString & NewName);
  void UpdateSource(const TLocalFileHandle & Handle, const TCopyParamType * CopyParam, int32_t AParams);
  void DoCopyToLocal(
    TStrings * FilesToCopy, const UnicodeString & ATargetDir, const TCopyParamType * CopyParam, int32_t AParams,
    TFileOperationProgressType * OperationProgress, uint32_t AFlags, TOnceDoneOperation & OnceDoneOperation);
  void SinkRobust(
    const UnicodeString & AFileName, const TRemoteFile * AFile, const UnicodeString & ATargetDir,
    const TCopyParamType * CopyParam, int32_t AParams, TFileOperationProgressType * OperationProgress, uint32_t AFlags);
  void Sink(
    const UnicodeString & AFileName, const TRemoteFile * AFile, const UnicodeString & ATargetDir,
    const TCopyParamType * CopyParam, int32_t AParams, TFileOperationProgressType * OperationProgress, uint32_t AFlags,
    TDownloadSessionAction & Action);
  void SinkFile(const UnicodeString & AFileName, const TRemoteFile * AFile, void * Param);
  void UpdateTargetAttrs(
    const UnicodeString & ADestFullName, const TRemoteFile * AFile, const TCopyParamType * CopyParam, int32_t Attrs);
  void UpdateTargetTime(HANDLE Handle, TDateTime Modification, TDSTMode DSTMode);
  void CheckParallelFileTransfer(
    const UnicodeString & TargetDir, TStringList * Files, const TCopyParamType * CopyParam, int32_t Params,
    UnicodeString & ParallelFileName, int64_t & ParallelFileSize, TFileOperationProgressType * OperationProgress);
  TRemoteFile * CheckRights(const UnicodeString & EntryType, const UnicodeString & FileName, bool & WrongRights);
  bool IsValidFile(TRemoteFile * File) const;
  void CalculateSubFoldersChecksum(
    const UnicodeString & Alg, TStrings * FileList, TCalculatedChecksumEvent OnCalculatedChecksum,
    TFileOperationProgressType * OperationProgress, bool FirstLevel);
  void GetShellChecksumAlgs(TStrings * Algs);
  TStrings * GetShellChecksumAlgDefs();
  TStrings * ProcessFeatures(TStrings * Features);

  UnicodeString EncryptFileName(const UnicodeString & APath, bool EncryptNewFiles);
  UnicodeString DecryptFileName(const UnicodeString & APath, bool DecryptFullPath, bool DontCache);
  typename TEncryptedFileNames::const_iterator GetEncryptedFileName(const UnicodeString & APath);
  bool IsFileEncrypted(const UnicodeString & APath, bool EncryptNewFiles = false);

  __property TFileOperationProgressType * OperationProgress = { read=FOperationProgress };
  TFileOperationProgressType *& OperationProgress{FOperationProgress};

  const TFileOperationProgressType * GetOperationProgress() const { return FOperationProgress; }
  TFileOperationProgressType * GetOperationProgress() { return FOperationProgress; }
  void SetOperationProgress(TFileOperationProgressType * OperationProgress) { FOperationProgress = OperationProgress; }
  virtual const TTerminal * GetPasswordSource() const { return this; }

public:
  explicit TTerminal(TObjectClassId Kind = OBJECT_CLASS_TTerminal) noexcept;
  void Init(TSessionData * ASessionData, TConfiguration * AConfiguration, TActionLog * AActionLog = nullptr);
  virtual ~TTerminal() noexcept;
  void Open();
  void Close();
  void FingerprintScan(UnicodeString & SHA256, UnicodeString & SHA1, UnicodeString & MD5);
  void Reopen(int32_t Params);
  virtual void DirectoryModified(const UnicodeString & APath, bool SubDirs);
  virtual void DirectoryLoaded(TRemoteFileList * FileList);
  void ShowExtendedException(Exception * E);
  void Idle();
  void RecryptPasswords();
  bool AllowedAnyCommand(const UnicodeString & ACommand) const;
  void AnyCommand(const UnicodeString & ACommand, TCaptureOutputEvent OutputEvent);
  void CloseOnCompletion(
    TOnceDoneOperation Operation = odoDisconnect, const UnicodeString & AMessage = "",
    const UnicodeString & TargetLocalPath = "", const UnicodeString & ADestLocalFileName = "");
  UnicodeString GetAbsolutePath(const UnicodeString & APath, bool Local) const;
  void BeginTransaction();
  void ReadCurrentDirectory();
  void ReadDirectory(bool ReloadOnly, bool ForceCache = false);
  TRemoteFileList * ReadDirectoryListing(const UnicodeString & Directory, const TFileMasks & Mask);
  TRemoteFileList * CustomReadDirectoryListing(const UnicodeString & Directory, bool UseCache);
  TRemoteFile * ReadFileListing(const UnicodeString & APath);
  TRemoteFile * ReadFile(const UnicodeString & AFileName);
  TRemoteFile * TryReadFile(const UnicodeString & AFileName);
  bool FileExists(const UnicodeString & AFileName);
  void ReadSymlink(TRemoteFile * SymlinkFile, TRemoteFile *& AFile);
  bool CopyToLocal(
    TStrings * AFilesToCopy, const UnicodeString & ATargetDir, const TCopyParamType * CopyParam, int32_t AParams,
    TParallelOperation * ParallelOperation);
  bool CopyToRemote(
    TStrings * AFilesToCopy, const UnicodeString & ATargetDir, const TCopyParamType * CopyParam, int32_t AParams,
    TParallelOperation * ParallelOperation);
  int32_t CopyToParallel(TParallelOperation * ParallelOperation, TFileOperationProgressType * OperationProgress);
  void LogParallelTransfer(TParallelOperation * ParallelOperation);
  void RemoteCreateDirectory(const UnicodeString & ADirName, const TRemoteProperties * Properties = nullptr);
  void RemoteCreateLink(const UnicodeString & AFileName, const UnicodeString & APointTo, bool Symbolic);
  void RemoteDeleteFile(const UnicodeString & AFileName,
    const TRemoteFile * AFile = nullptr, void * AParams = nullptr);
  bool RemoteDeleteFiles(TStrings * AFilesToDelete, int32_t Params = 0);
  bool DeleteLocalFiles(TStrings * AFileList, int32_t Params = 0);
  bool IsRecycledFile(const UnicodeString & AFileName);
  void CustomCommandOnFile(const UnicodeString & AFileName,
    const TRemoteFile * AFile, void * AParams);
  void CustomCommandOnFiles(const UnicodeString & ACommand, int32_t AParams,
    TStrings * AFiles, TCaptureOutputEvent OutputEvent);
  void RemoteChangeDirectory(const UnicodeString & ADirectory);
  void EndTransaction();
  void HomeDirectory();
  UnicodeString GetHomeDirectory();
  void ChangeFileProperties(const UnicodeString & AFileName,
    const TRemoteFile * AFile, /* const TRemoteProperties */ void * Properties);
  void ChangeFilesProperties(TStrings * AFileList,
    const TRemoteProperties * Properties);
  bool LoadFilesProperties(TStrings * AFileList);
  void TerminalError(const UnicodeString & Msg);
  void TerminalError(Exception * E, const UnicodeString & AMsg, const UnicodeString & AHelpKeyword = L"");
  void ReloadDirectory();
  void RefreshDirectory();
  void TerminalRenameFile(const TRemoteFile * AFile, const UnicodeString & ANewName);
  void TerminalMoveFile(const UnicodeString & AFileName, const TRemoteFile * AFile,
    /*const TMoveFileParams*/ void * Param);
  bool TerminalMoveFiles(
    TStrings * AFileList, const UnicodeString & ATarget, const UnicodeString & AFileMask, bool DontOverwrite);
  void TerminalCopyFile(const UnicodeString & AFileName, const TRemoteFile * AFile,
    /*const TMoveFileParams*/ void * Param);
  bool TerminalCopyFiles(
    TStrings * AFileList, const UnicodeString & ATarget, const UnicodeString & AFileMask, bool DontOverwrite);
  bool CalculateFilesSize(TStrings * AFileList, int64_t & Size, TCalculateSizeParams & Params);
  bool CalculateLocalFilesSize(TStrings * FileList, int64_t & Size,
    const TCopyParamType * CopyParam, bool AllowDirs, TStrings * Files, TCalculatedSizes * CalculatedSizes);
  void CalculateFilesChecksum(
    const UnicodeString & Alg, TStrings * AFileList, TCalculatedChecksumEvent OnCalculatedChecksum);
  void ClearCaches();
  TSynchronizeChecklist * SynchronizeCollect(const UnicodeString & LocalDirectory,
    const UnicodeString & ARemoteDirectory, TSynchronizeMode Mode,
    const TCopyParamType * CopyParam, int32_t AParams,
    TSynchronizeDirectoryEvent OnSynchronizeDirectory, TSynchronizeOptions * Options);
  void SynchronizeApply(
    TSynchronizeChecklist * Checklist,
    const TCopyParamType * CopyParam, int32_t AParams,
    TSynchronizeDirectoryEvent OnSynchronizeDirectory, TProcessedSynchronizationChecklistItem OnProcessedItem,
    TUpdatedSynchronizationChecklistItems OnUpdatedSynchronizationChecklistItems, void * Token,
    TFileOperationStatistics * Statistics);
  void SynchronizeChecklistCalculateSize(
    TSynchronizeChecklist * Checklist, const TSynchronizeChecklist::TItemList & Items,
    const TCopyParamType * CopyParam);
  void FilesFind(const UnicodeString & Directory, const TFileMasks & FileMask,
    TFileFoundEvent OnFileFound, TFindingFileEvent OnFindingFile);
  void SpaceAvailable(const UnicodeString & APath, TSpaceAvailable & ASpaceAvailable);
  void LockFiles(TStrings * AFileList);
  void UnlockFiles(TStrings * AFileList);
  TRemoteFileList * DirectoryFileList(const UnicodeString & APath, TDateTime Timestamp, bool CanLoad);
  void MakeLocalFileList(
    const UnicodeString & AFileName, const TSearchRecSmart & Rec, void * Param);
  bool FileOperationLoopQuery(Exception & E,
    TFileOperationProgressType * OperationProgress, const UnicodeString & Message,
    uint32_t AFlags, const UnicodeString & SpecialRetry = "", const UnicodeString & HelpKeyword = "");
  void FileOperationLoopEnd(Exception & E,
    TFileOperationProgressType * OperationProgress, const UnicodeString & Message,
    uint32_t AFlags, const UnicodeString & SpecialRetry, const UnicodeString & HelpKeyword);
  TUsableCopyParamAttrs UsableCopyParamAttrs(int32_t AParams) const;
  bool ContinueReopen(TDateTime Start) const;
  bool QueryReopen(Exception * E, int32_t AParams,
    TFileOperationProgressType * OperationProgress);
  UnicodeString PeekCurrentDirectory();
  void FatalAbort();
  void ReflectSettings() const;
  void CollectUsage();
  TTerminal * CreateSecondarySession(const UnicodeString & Name, TSessionData * SessionData);
  void FillSessionDataForCode(TSessionData * SessionData) const;
  void UpdateSessionCredentials(TSessionData * Data);
  UnicodeString UploadPublicKey(const UnicodeString & FileName);
  TCustomFileSystem * GetFileSystemForCapability(TFSCapability Capability, bool NeedCurrentDirectory = false);
  void PrepareCommandSession(bool NeedCurrentDirectory = false);

  const TSessionInfo & GetSessionInfo() const;
  const TFileSystemInfo & GetFileSystemInfo(bool Retrieve = false);
  void LogEvent(const UnicodeString & AStr);
  void LogEvent(int32_t Level, const UnicodeString & AStr);
  void GetSupportedChecksumAlgs(TStrings * Algs);
  UnicodeString ChangeFileName(const TCopyParamType * CopyParam,
    const UnicodeString & AFileName, TOperationSide Side, bool FirstLevel) const;
  UnicodeString GetBaseFileName(const UnicodeString & AFileName) const;
  bool IsEncryptingFiles() const { return !FEncryptKey.IsEmpty(); }
  RawByteString GetEncryptKey() const { return FEncryptKey; }

  static UnicodeString ExpandFileName(const UnicodeString & APath,
    const UnicodeString & BasePath);

  __property TSessionData * SessionData = { read = FSessionData };
  ROProperty<TSessionData *> SessionData{nb::bind(&TTerminal::GetSessionData, this)};
  __property TSessionLog * Log = { read = FLog };
  ROProperty<TSessionLog *> Log{nb::bind(&TTerminal::GetLogConst, this)};
  __property TActionLog * ActionLog = { read = FActionLog };
  ROProperty<TActionLog *> ActionLog{nb::bind(&TTerminal::GetActionLog, this)};
  __property TConfiguration * Configuration = { read = FConfiguration };
  ROProperty<const TConfiguration *> Configuration{nb::bind(&TTerminal::GetConfigurationConst, this)};
  __property bool Active = { read = GetActive };
  ROProperty<bool> Active{nb::bind(&TTerminal::GetActive, this)};
  __property TSessionStatus Status = { read = FStatus };
  __property UnicodeString CurrentDirectory = { read = GetCurrentDirectory, write = SetCurrentDirectory };
  RWProperty<UnicodeString> CurrentDirectory{nb::bind(&TTerminal::RemoteGetCurrentDirectory, this), nb::bind(&TTerminal::TerminalSetCurrentDirectory, this)};
  __property bool ExceptionOnFail = { read = GetExceptionOnFail, write = SetExceptionOnFail };
  RWProperty3<bool> ExceptionOnFail{nb::bind(&TTerminal::GetExceptionOnFail, this), nb::bind(&TTerminal::SetExceptionOnFail, this)};
  __property TRemoteDirectory * Files = { read = FFiles };
  __property TNotifyEvent OnChangeDirectory = { read = FOnChangeDirectory, write = FOnChangeDirectory };
  __property TReadDirectoryEvent OnReadDirectory = { read = FOnReadDirectory, write = FOnReadDirectory };
  __property TNotifyEvent OnStartReadDirectory = { read = FOnStartReadDirectory, write = FOnStartReadDirectory };
  __property TReadDirectoryProgressEvent OnReadDirectoryProgress = { read = FOnReadDirectoryProgress, write = FOnReadDirectoryProgress };
  __property TDeleteLocalFileEvent OnDeleteLocalFile = { read = FOnDeleteLocalFile, write = FOnDeleteLocalFile };
  TDeleteLocalFileEvent & OnDeleteLocalFile{FOnDeleteLocalFile};
  __property TNotifyEvent OnInitializeLog = { read = FOnInitializeLog, write = FOnInitializeLog };
  __property const TRemoteTokenList * Groups = { read = GetGroups };
  __property const TRemoteTokenList * Users = { read = GetUsers };
  __property const TRemoteTokenList * Membership = { read = GetMembership };
  __property TFileOperationProgressEvent OnProgress  = { read=FOnProgress, write=FOnProgress };
  __property TFileOperationFinished OnFinished  = { read=FOnFinished, write=FOnFinished };
  __property TCurrentFSProtocol FSProtocol = { read = FFSProtocol };
  __property bool UseBusyCursor = { read = FUseBusyCursor, write = FUseBusyCursor };
  bool& UseBusyCursor{FUseBusyCursor};
  __property UnicodeString UserName = { read=GetUserName };
  ROProperty<UnicodeString> UserName{nb::bind(&TTerminal::TerminalGetUserName, this)};
  __property bool IsCapable[TFSCapability Capability] = { read = GetIsCapable };
  __property bool AreCachesEmpty = { read = GetAreCachesEmpty };
  __property bool CommandSessionOpened = { read = GetCommandSessionOpened };
  __property TTerminal * CommandSession = { read = GetCommandSession };
  __property TTerminal * PrimaryTerminal = { read = GetPrimaryTerminal };
  ROProperty<TTerminal *> PrimaryTerminal{nb::bind(&TTerminal::GetPrimaryTerminal, this)};
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
  __property int32_t TunnelLocalPortNumber = { read = FTunnelLocalPortNumber };

  bool IsThisOrChild(TTerminal * ATerminal) const;
  bool GetIsCapable(TFSCapability Capability) const { return GetIsCapableProtected(Capability); }
  void SetMasks(const UnicodeString & Value);

  void SetLocalFileTime(const UnicodeString & LocalFileName,
    const TDateTime & Modification);
  void SetLocalFileTime(const UnicodeString & LocalFileName,
    FILETIME * AcTime, FILETIME * WrTime);
  DWORD GetLocalFileAttributes(const UnicodeString & LocalFileName) const;
  bool SetLocalFileAttributes(const UnicodeString & LocalFileName, DWORD FileAttributes);
  bool MoveLocalFile(const UnicodeString & LocalFileName, const UnicodeString & NewLocalFileName, DWORD Flags);
  bool RemoveLocalDirectory(const UnicodeString & LocalDirName);
  bool CreateLocalDirectory(const UnicodeString & LocalDirName, LPSECURITY_ATTRIBUTES SecurityAttributes);

  TSessionData * GetSessionData() const { return FSessionData.get(); }
  TSessionLog * GetLogConst() const { return FLog.get(); }
  TSessionLog * GetLog() { return FLog.get(); }
  TActionLog * GetActionLog() const { return FActionLog.get(); }
  const TConfiguration * GetConfigurationConst() const { return FConfiguration; }
  TConfiguration * GetConfiguration() { return FConfiguration; }
  TSessionStatus GetStatus() const { return FStatus; }
  TRemoteDirectory * GetFiles() const { return FFiles.get(); }
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
  int32_t GetTunnelLocalPortNumber() const { return FTunnelLocalPortNumber; }
  void SetRememberedPassword(const UnicodeString & Value) { FRememberedPassword = Value; }
  void SetRememberedTunnelPassword(const UnicodeString & Value) { FRememberedTunnelPassword = Value; }
  void SetTunnelPassword(const UnicodeString & Value) { FRememberedTunnelPassword = Value; }
  TCustomFileSystem * GetFileSystem() const { return FFileSystem.get(); }
  TCustomFileSystem * GetFileSystem() { return FFileSystem.get(); }

  HANDLE TerminalCreateLocalFile(const UnicodeString & LocalFileName, DWORD DesiredAccess,
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

  void AfterMoveFiles(TStrings * AFileList);
};

NB_DEFINE_CLASS_ID(TSecondaryTerminal);
class NB_CORE_EXPORT TSecondaryTerminal : public TTerminal
{
  NB_DISABLE_COPY(TSecondaryTerminal)
public:
  static bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TSecondaryTerminal); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TSecondaryTerminal) || TTerminal::is(Kind); }
public:
  TSecondaryTerminal() noexcept : TTerminal(OBJECT_CLASS_TSecondaryTerminal) {}
  explicit TSecondaryTerminal(TObjectClassId Kind) noexcept;
  virtual ~TSecondaryTerminal() = default;
  void Init(
    TTerminal * MainTerminal, TSessionData * ASessionData, TConfiguration * AConfiguration,
    const UnicodeString & Name, TActionLog * ActionLog);

  void UpdateFromMain();

  __property TTerminal * MainTerminal = { read = FMainTerminal };
  TTerminal * GetMainTerminal() const { return FMainTerminal; }

protected:
  virtual void DirectoryLoaded(TRemoteFileList * FileList) override;
  virtual void DirectoryModified(const UnicodeString & APath,
    bool SubDirs) override;
  virtual const TTerminal * GetPasswordSource() const override { return FMainTerminal; }
  virtual TTerminal * GetPrimaryTerminal() override;

private:
  TTerminal * FMainTerminal{nullptr};
};

NB_DEFINE_CLASS_ID(TTerminalList);
class NB_CORE_EXPORT TTerminalList : public TObjectList
{
  NB_DISABLE_COPY(TTerminalList)
public:
  explicit TTerminalList(TConfiguration * AConfiguration) noexcept;
  virtual ~TTerminalList() noexcept;

  virtual TTerminal * NewTerminal(TSessionData * Data);
  virtual void FreeTerminal(TTerminal * Terminal);
  void FreeAndNullTerminal(TTerminal *& Terminal);
  void RecryptPasswords();

  __property TTerminal * Terminals[int32_t Index]  = { read = GetTerminal };

protected:
  virtual TTerminal * CreateTerminal(TSessionData * Data);

private:
  TConfiguration * FConfiguration{nullptr};

public:
  TTerminal * GetTerminal(int32_t Index);
};

NB_DEFINE_CLASS_ID(TCustomCommandParams);
struct NB_CORE_EXPORT TCustomCommandParams : public TObject
{
public:
  static bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TCustomCommandParams); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TCustomCommandParams) || TObject::is(Kind); }
public:
  TCustomCommandParams() : TObject(OBJECT_CLASS_TCustomCommandParams) {}
  UnicodeString Command;
  int32_t Params{0};
  TCaptureOutputEvent OutputEvent;
};

struct NB_CORE_EXPORT TCalculateSizeStats : public TObject
{
  TCalculateSizeStats() noexcept;

  int32_t Files{0};
  int32_t Directories{0};
  int32_t SymLinks{0};
  TStrings * FoundFiles{nullptr};
  TCalculatedSizes * CalculatedSizes{nullptr};
};

NB_DEFINE_CLASS_ID(TCalculateSizeParams);
struct NB_CORE_EXPORT TCalculateSizeParams : public TObject
{
friend class TTerminal;

public:
  static bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TCalculateSizeParams); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TCalculateSizeParams) || TObject::is(Kind); }
public:
  TCalculateSizeParams() noexcept;
  int32_t Params{0};
  const TCopyParamType * CopyParam{nullptr};
  TCalculateSizeStats * Stats{nullptr};
  bool AllowDirs{true};
  bool UseCache{false};
//private:
  TCollectedFileList * Files{nullptr};
  UnicodeString LastDirPath;
  int64_t Size{0};
  bool Result{true};
};

using TDateTimes = nb::vector_t<TDateTime>;

NB_DEFINE_CLASS_ID(TMakeLocalFileListParams);
struct NB_CORE_EXPORT TMakeLocalFileListParams : public TObject
{
public:
  static bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TMakeLocalFileListParams); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TMakeLocalFileListParams) || TObject::is(Kind); }
public:
  TMakeLocalFileListParams() : TObject(OBJECT_CLASS_TMakeLocalFileListParams) {}
  TStrings * FileList{nullptr};
  TDateTimes * FileTimes{nullptr};
  bool IncludeDirs{false};
  bool Recursive{false};
};

struct NB_CORE_EXPORT TSynchronizeOptions : public TObject
{
  NB_DISABLE_COPY(TSynchronizeOptions)
public:
  TSynchronizeOptions() noexcept;
  ~TSynchronizeOptions() noexcept;

  TStringList * Filter{nullptr};
  int32_t Files{0};

  bool FilterFind(const UnicodeString & AFileName) const;
  bool MatchesFilter(const UnicodeString & AFileName) const;
};

struct NB_CORE_EXPORT TSpaceAvailable
{
  CUSTOM_MEM_ALLOCATION_IMPL
  TSpaceAvailable();

  int64_t BytesOnDevice{0};
  int64_t UnusedBytesOnDevice{0};
  int64_t BytesAvailableToUser{0};
  int64_t UnusedBytesAvailableToUser{0};
  uint32_t BytesPerAllocationUnit{0};
};

class NB_CORE_EXPORT TRobustOperationLoop : public TObject
{
  NB_DISABLE_COPY(TRobustOperationLoop)
public:
  TRobustOperationLoop() = delete;
  explicit TRobustOperationLoop(TTerminal * Terminal, TFileOperationProgressType * OperationProgress, bool * AnyTransfer = nullptr, bool CanRetry = true) noexcept;
  virtual ~TRobustOperationLoop() noexcept;
  bool TryReopen(Exception & E);
  bool ShouldRetry() const;
  bool Retry();

private:
  TTerminal * FTerminal{nullptr};
  TFileOperationProgressType * FOperationProgress{nullptr};
  bool FRetry{false};
  bool * FAnyTransfer{nullptr};
  bool FPrevAnyTransfer{false};
  TDateTime FStart;
  bool FCanRetry{false};
};

NB_DEFINE_CLASS_ID(TCollectedFileList);
class TCollectedFileList : public TObject
{
public:
  static bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TCollectedFileList); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TCollectedFileList) || TObject::is(Kind); }
public:
  TCollectedFileList() noexcept;
  virtual ~TCollectedFileList();
  int32_t Add(const UnicodeString & AFileName, TObject * Object, bool Dir);
  void DidNotRecurse(int32_t Index);
  void Delete(int32_t Index);

  int32_t GetCount() const;
  UnicodeString GetFileName(int32_t Index) const;
  TObject * GetObj(int32_t Index) const;
  bool IsDir(int32_t Index) const;
  bool IsRecursed(int32_t Index) const;
  int32_t GetState(int32_t Index) const;
  void SetState(int32_t Index, int32_t State);

private:
  void Deleting(int32_t Index);

  struct TFileData
  {
    CUSTOM_MEM_ALLOCATION_IMPL
    UnicodeString FileName;
    TObject * Object{nullptr};
    bool Dir{false};
    bool Recursed{false};
    int32_t State{0};
  };
  using TFileDataList = nb::vector_t<TFileData>;
  TFileDataList FList;
};

class TQueueFileList;

class TParallelOperation : public TObject
{
public:
  TParallelOperation() = delete;
  explicit TParallelOperation(TOperationSide Side) noexcept;
  virtual ~TParallelOperation() noexcept;

  void Init(
    TStrings * AFiles, const UnicodeString & ATargetDir, const TCopyParamType * CopyParam, int32_t AParams,
    TFileOperationProgressType * MainOperationProgress, const UnicodeString & AMainName,
    int64_t ParallelFileSize);

  bool IsInitialized() const;
  void WaitFor();
  bool ShouldAddClient() const;
  void AddClient();
  void RemoveClient();
  int32_t GetNext(
    TTerminal * Terminal, UnicodeString & AFileName, TObject *& Object, UnicodeString & ATargetDir, bool & Dir,
    bool & Recursed, TCopyParamType *& CustomCopyParam);
  void Done(
    const UnicodeString & AFileName, bool Dir, bool Success, const UnicodeString & TargetDir,
    const TCopyParamType * CopyParam, TTerminal * Terminal);
  bool UpdateFileList(TQueueFileList * UpdateFileList);

  static bool GetOnlyFile(TStrings * FileList, UnicodeString & FileName, TObject *& Object);
  static TCollectedFileList * GetFileList(TStrings * FileList, int32_t Index);
  static UnicodeString GetPartPrefix(const UnicodeString & FileName);
  __property TOperationSide Side = { read = FSide };
  __property const TCopyParamType * CopyParam = { read = FCopyParam };
  __property int32_t Params = { read = FParams };
  __property UnicodeString TargetDir = { read = FTargetDir };
  __property TFileOperationProgressType * MainOperationProgress = { read = FMainOperationProgress };
  __property UnicodeString MainName = { read = FMainName };
  __property bool IsParallelFileTransfer = { read = FIsParallelFileTransfer };
  const bool& IsParallelFileTransfer{FIsParallelFileTransfer};

  TOperationSide GetSide() const { return FSide; }
  const TCopyParamType * GetCopyParam() const { return FCopyParam; }
  int32_t GetParams() const { return FParams; }
  UnicodeString GetTargetDir() const { return FTargetDir; }
  TFileOperationProgressType * GetMainOperationProgress() const { return FMainOperationProgress; }
  UnicodeString GetMainName() const { return FMainName; }

private:
  struct TDirectoryData
  {
    CUSTOM_MEM_ALLOCATION_IMPL
    UnicodeString OppositePath;
    bool Exists{false};
  };

  std::unique_ptr<TStrings> FFileList;
  int32_t FListIndex{0};
  int32_t FIndex{0};
  using TDirectories = nb::map_t<UnicodeString, TDirectoryData>;
  TDirectories FDirectories;
  UnicodeString FTargetDir;
  const TCopyParamType * FCopyParam{nullptr};
  int32_t FParams{0};
  bool FProbablyEmpty{false};
  int32_t FClients{0};
  std::unique_ptr<TCriticalSection> FSection;
  TFileOperationProgressType * FMainOperationProgress{nullptr};
  TOperationSide FSide{osLocal};
  UnicodeString FMainName;
  int32_t FVersion{0};
  bool FIsParallelFileTransfer{false};
  int64_t FParallelFileSize{0};
  int64_t FParallelFileOffset{0};
  int32_t FParallelFileCount{0};
  UnicodeString FParallelFileTargetName;
  using TParallelFileOffsets = std::vector<int64_t> ;
  TParallelFileOffsets FParallelFileOffsets;
  rde::vector<bool> FParallelFileDones;
  bool FParallelFileMerging{false};
  int32_t FParallelFileMerged{0};

  bool CheckEnd(TCollectedFileList * Files);
  TCollectedFileList * GetFileList(int32_t Index);
};

struct TLocalFileHandle
{
  TLocalFileHandle() noexcept;
  ~TLocalFileHandle() noexcept;

  void Dismiss();
  void Close();
  void Release();

  UnicodeString FileName;
  HANDLE Handle{nullptr};
  DWORD Attrs{0};
  bool Directory{false};
  TDateTime Modification;
  int64_t MTime{0};
  int64_t ATime{0};
  int64_t Size{0};
  CUSTOM_MEM_ALLOCATION_IMPL
};

class TLocalFile : public TObject
{
public:
  TSearchRecSmart SearchRec;
};

NB_CORE_EXPORT UnicodeString GetSessionUrl(const TTerminal * Terminal, bool WithUserName = false);

#pragma warning(push)
#pragma warning(disable: 4512) // assignment operator could not be generated

class TOutputProxy : public TObject
{
public:
  TOutputProxy(TCallSessionAction & Action, TCaptureOutputEvent OutputEvent) noexcept :
    FAction(Action),
    FOutputEvent(OutputEvent)
  {
  }

  void Output(const UnicodeString & Str, TCaptureOutputType OutputType)
  {
    switch (OutputType)
    {
    case cotOutput:
      FAction.AddOutput(Str, false);
      break;
    case cotError:
      FAction.AddOutput(Str, true);
      break;
    case cotExitCode:
      FAction.ExitCode(nb::ToInt32(::StrToInt64(Str)));
      return;
    }

    if (FOutputEvent != nullptr)
    {
      FOutputEvent(Str, OutputType);
    }
  }

private:
  TCallSessionAction & FAction;
  TCaptureOutputEvent FOutputEvent;
};

#pragma warning(pop)

inline void ThrowSkipFile(Exception * Exception, const UnicodeString & Message)
{
  throw ESkipFile(Exception, Message);
}

inline void ThrowSkipFileNull() { ThrowSkipFile(nullptr, L""); }
