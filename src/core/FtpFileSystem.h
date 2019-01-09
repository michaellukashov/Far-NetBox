
#pragma once


//---------------------------------------------------------------------------
#include <time.h>
#include <rdestl/map.h>
#include <FileSystems.h>
//---------------------------------------------------------------------------
class TFileZillaIntf;
class TFileZillaImpl;
class TMessageQueue;
class TFTPServerCapabilities;
struct TOverwriteFileParams;
struct TListDataEntry;
struct TFileTransferData;
struct TFtpsCertificateData;
struct TRemoteFileTime;
//---------------------------------------------------------------------------
NB_DEFINE_CLASS_ID(TFTPFileSystem);
class NB_CORE_EXPORT TFTPFileSystem final : public TCustomFileSystem
{
  friend class TFileZillaImpl;
  friend class TFTPFileListHelper;
  NB_DISABLE_COPY(TFTPFileSystem)
public:
  static bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TFTPFileSystem); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TFTPFileSystem) || TCustomFileSystem::is(Kind); }
public:
  explicit TFTPFileSystem(TTerminal *ATerminal) noexcept;
  virtual ~TFTPFileSystem() noexcept;
  void Init(void *) override;

  void Open() override;
  void Close() override;
  bool GetActive() const override;
  void CollectUsage() override;
  void Idle() override;
  UnicodeString GetAbsolutePath(const UnicodeString APath, bool Local) override;
  UnicodeString GetAbsolutePath(const UnicodeString APath, bool Local) const override;
  void AnyCommand(const UnicodeString Command,
    TCaptureOutputEvent OutputEvent) override;
  void ChangeDirectory(const UnicodeString ADirectory) override;
  void CachedChangeDirectory(const UnicodeString ADirectory) override;
  void AnnounceFileListOperation() override;
  void ChangeFileProperties(const UnicodeString AFileName,
    const TRemoteFile *AFile, const TRemoteProperties *Properties,
    TChmodSessionAction &Action) override;
  bool LoadFilesProperties(TStrings *AFileList) override;
  void CalculateFilesChecksum(const UnicodeString Alg,
    TStrings *AFileList, TStrings *Checksums,
    TCalculatedChecksumEvent OnCalculatedChecksum) override;
  void CopyToLocal(TStrings *AFilesToCopy,
    const UnicodeString ATargetDir, const TCopyParamType *CopyParam,
    intptr_t Params, TFileOperationProgressType *OperationProgress,
    TOnceDoneOperation &OnceDoneOperation) override;
  void TransferOnDirectory(
    const UnicodeString ADirectory, const TCopyParamType *CopyParam, intptr_t AParams);
  void CopyToRemote(TStrings *AFilesToCopy,
    const UnicodeString ATargetDir, const TCopyParamType *CopyParam,
    intptr_t AParams, TFileOperationProgressType *OperationProgress,
    TOnceDoneOperation &OnceDoneOperation) override;
  void Source(
    TLocalFileHandle &AHandle, const UnicodeString ATargetDir, UnicodeString &ADestFileName,
    const TCopyParamType *CopyParam, intptr_t AParams,
    TFileOperationProgressType *OperationProgress, uintptr_t AFlags,
    TUploadSessionAction &Action, bool &ChildError);
  void Sink(
    const UnicodeString AFileName, const TRemoteFile *AFile,
    const UnicodeString ATargetDir, UnicodeString &ADestFileName, intptr_t Attrs,
    const TCopyParamType *CopyParam, intptr_t AParams, TFileOperationProgressType *OperationProgress,
    uintptr_t AFlags, TDownloadSessionAction &Action);
  void RemoteCreateDirectory(const UnicodeString ADirName, bool Encrypt) override;
  // virtual void __fastcall CreateDirectory(const UnicodeString & DirName, bool Encrypt);
  void RemoteCreateLink(const UnicodeString AFileName, const UnicodeString APointTo, bool Symbolic) override;
  void RemoteDeleteFile(const UnicodeString AFileName,
    const TRemoteFile *AFile, intptr_t Params, TRmSessionAction &Action) override;
  void CustomCommandOnFile(const UnicodeString AFileName,
    const TRemoteFile *AFile, UnicodeString ACommand, intptr_t AParams, TCaptureOutputEvent OutputEvent) override;
  void DoStartup() override;
  void HomeDirectory() override;
  bool IsCapable(intptr_t Capability) const override;
  void LookupUsersGroups() override;
  void ReadCurrentDirectory() override;
  void ReadDirectory(TRemoteFileList *FileList) override;
  void ReadFile(const UnicodeString AFileName,
    TRemoteFile *&AFile) override;
  void ReadSymlink(TRemoteFile *SymlinkFile,
    TRemoteFile *&AFile) override;
  void RemoteRenameFile(const UnicodeString AFileName, const TRemoteFile *AFile,
    const UnicodeString ANewName) override;
  void RemoteCopyFile(const UnicodeString AFileName, const TRemoteFile *AFile,
    const UnicodeString ANewName) override;
  TStrings * GetFixedPaths() const override;
  void SpaceAvailable(const UnicodeString APath,
    TSpaceAvailable &ASpaceAvailable) override;
  const TSessionInfo & GetSessionInfo() const override;
  const TFileSystemInfo & GetFileSystemInfo(bool Retrieve) override;
  bool TemporaryTransferFile(const UnicodeString AFileName) override;
  bool GetStoredCredentialsTried() const override;
  UnicodeString RemoteGetUserName() const override;
  void GetSupportedChecksumAlgs(TStrings *Algs) override;
  void LockFile(const UnicodeString AFileName, const TRemoteFile *AFile) override;
  void UnlockFile(const UnicodeString AFileName, const TRemoteFile *AFile) override;
  void UpdateFromMain(TCustomFileSystem *MainFileSystem) override;
  void ClearCaches() override;

protected:
  __removed enum TOverwriteMode { omOverwrite, omResume, omComplete };

  UnicodeString RemoteGetCurrentDirectory() const override;

  const wchar_t *GetOption(intptr_t OptionID) const;
  intptr_t GetOptionVal(intptr_t OptionID) const;

  enum
  {
    REPLY_CONNECT      = 0x01,
    REPLY_2XX_CODE     = 0x02,
    REPLY_ALLOW_CANCEL = 0x04,
    REPLY_3XX_CODE     = 0x08,
    REPLY_SINGLE_LINE  = 0x10,
  };

  bool FTPPostMessage(uintptr_t Type, WPARAM wParam, LPARAM lParam);
  bool ProcessMessage();
  void DiscardMessages();
  void WaitForMessages();
  uintptr_t WaitForReply(bool Command, bool WantLastCode);
  uintptr_t WaitForCommandReply(bool WantLastCode = true);
  void WaitForFatalNonCommandReply();
  void PoolForFatalNonCommandReply();
  void GotNonCommandReply(uintptr_t Reply);
  UnicodeString GotReply(uintptr_t Reply, uintptr_t Flags = 0,
    const UnicodeString Error = L"", uintptr_t *Code = nullptr,
    TStrings **Response = nullptr);
  void ResetReply();
  void HandleReplyStatus(const UnicodeString Response);
  void DoWaitForReply(uintptr_t &ReplyToAwait, bool WantLastCode);
  bool KeepWaitingForReply(uintptr_t &ReplyToAwait, bool WantLastCode) const;
  inline bool NoFinalLastCode() const;

  bool HandleStatus(const wchar_t *AStatus, int Type);
  bool HandleAsynchRequestOverwrite(
    wchar_t *FileName1, size_t FileName1Len, const wchar_t *FileName2,
    const wchar_t *Path1, const wchar_t *Path2,
    int64_t Size1, int64_t Size2, time_t LocalTime,
    bool HasLocalTime, const TRemoteFileTime &RemoteTime, void *AUserData,
    HANDLE &ALocalFileHandle,
    int &RequestResult);
  bool HandleAsynchRequestVerifyCertificate(
    const TFtpsCertificateData &Data, int &RequestResult);
  bool HandleAsynchRequestNeedPass(
    struct TNeedPassRequestData &Data, int &RequestResult) const;
  bool HandleListData(const wchar_t *Path, const TListDataEntry *Entries,
    uintptr_t Count);
  bool HandleTransferStatus(bool Valid, int64_t TransferSize,
    int64_t Bytes, bool FileTransfer);
  bool HandleReply(intptr_t Command, uintptr_t Reply);
  bool HandleCapabilities(TFTPServerCapabilities *ServerCapabilities);
  bool CheckError(intptr_t ReturnCode, const wchar_t *Context);
  void PreserveDownloadFileTime(HANDLE AHandle, void *UserData) const;
  bool GetFileModificationTimeInUtc(const wchar_t *AFileName, struct tm &Time);
  void EnsureLocation(const UnicodeString ADirectory, bool Log);
  void EnsureLocation();
  UnicodeString GetActualCurrentDirectory() const;
  void Discard();
  void DoChangeDirectory(const UnicodeString ADirectory);

  void Sink(const UnicodeString AFileName,
    const TRemoteFile *AFile, const UnicodeString ATargetDir,
    const TCopyParamType *CopyParam, intptr_t AParams,
    TFileOperationProgressType *OperationProgress, uintptr_t AFlags,
    TDownloadSessionAction &Action);
  bool ConfirmOverwrite(const UnicodeString ASourceFullFileName, UnicodeString &ATargetFileName,
    intptr_t AParams, TFileOperationProgressType *OperationProgress,
    bool AutoResume,
    const TOverwriteFileParams *FileParams, const TCopyParamType *CopyParam,
    TOverwriteMode &OverwriteMode);
  void ReadDirectoryProgress(int64_t Bytes);
  void ResetFileTransfer();
  virtual void DoFileTransferProgress(int64_t TransferSize, int64_t Bytes);
  void FileTransferProgress(int64_t TransferSize, int64_t Bytes) override;
  void ResetCaches();
  void CaptureOutput(const UnicodeString AStr);
  void DoReadDirectory(TRemoteFileList *AFileList);
  void DoReadFile(const UnicodeString AFileName, TRemoteFile *& AFile);
  void FileTransfer(const UnicodeString AFileName, const UnicodeString LocalFile,
    const UnicodeString RemoteFile, const UnicodeString RemotePath, bool Get,
    int64_t Size, intptr_t Type, TFileTransferData &UserData,
    TFileOperationProgressType *OperationProgress);
  TDateTime ConvertLocalTimestamp(time_t Time);
  void RemoteFileTimeToDateTimeAndPrecision(const TRemoteFileTime &Source,
    TDateTime &DateTime, TModificationFmt &ModificationFmt) const;
  void SetLastCode(intptr_t Code);
  void StoreLastResponse(const UnicodeString Text);
  void SetCPSLimit(TFileOperationProgressType *OperationProgress);
  bool VerifyCertificateHostName(const TFtpsCertificateData &Data);
  bool SupportsReadingFile() const;
  void AutoDetectTimeDifference(TRemoteFileList *FileList);
  void AutoDetectTimeDifference(const UnicodeString ADirectory, const TCopyParamType *CopyParam, intptr_t AParams);
  void ApplyTimeDifference(TRemoteFile *File);
  void ApplyTimeDifference(
    const UnicodeString FileName, TDateTime &Modification, TModificationFmt &ModificationFmt);
  void DummyReadDirectory(const UnicodeString ADirectory);
  bool IsEmptyFileList(TRemoteFileList *FileList) const;
  void CheckTimeDifference();
  bool NeedAutoDetectTimeDifference() const;
  bool LookupUploadModificationTime(
    const UnicodeString FileName, TDateTime &Modification, TModificationFmt ModificationFmt);
  UnicodeString DoCalculateFileChecksum(bool UsingHashCommand, const UnicodeString Alg, TRemoteFile *File);
  void DoCalculateFilesChecksum(bool UsingHashCommand, const UnicodeString Alg,
    TStrings *FileList, TStrings *Checksums,
    TCalculatedChecksumEvent OnCalculatedChecksum,
    TFileOperationProgressType *OperationProgress, bool FirstLevel);
  void HandleFeatReply();
  void ResetFeatures();
  bool SupportsSiteCommand(const UnicodeString ACommand) const;
  bool SupportsCommand(const UnicodeString ACommand) const;
  void RegisterChecksumAlgCommand(const UnicodeString Alg, const UnicodeString ACommand);
  void SendCommand(const UnicodeString ACommand);
  bool CanTransferSkipList(intptr_t AParams, uintptr_t AFlags, const TCopyParamType *CopyParam) const;

  static bool Unquote(UnicodeString &Str);

private:
  enum TCommand
  {
    CMD_UNKNOWN,
    PASS,
    SYST,
    FEAT
  };

  mutable TFileZillaIntf *FFileZillaIntf{nullptr};
  TCriticalSection FQueueCriticalSection;
  TCriticalSection FTransferStatusCriticalSection;
  std::unique_ptr<TMessageQueue> FQueue{nullptr};
  HANDLE FQueueEvent{};
  TSessionInfo FSessionInfo;
  TFileSystemInfo FFileSystemInfo;
  bool FFileSystemInfoValid{false};
  uintptr_t FReply{0};
  uintptr_t FCommandReply{0};
  TCommand FLastCommand{CMD_UNKNOWN};
  bool FPasswordFailed{false};
  bool FStoredPasswordTried{false};
  bool FMultineResponse{false};
  intptr_t FLastCode{0};
  intptr_t FLastCodeClass{0};
  intptr_t FLastReadDirectoryProgress{0};
  UnicodeString FTimeoutStatus;
  UnicodeString FDisconnectStatus;
  std::unique_ptr<TStrings> FLastResponse{nullptr};
  std::unique_ptr<TStrings> FLastErrorResponse{nullptr};
  std::unique_ptr<TStrings> FLastError{nullptr};
  UnicodeString FSystem;
  std::unique_ptr<TStrings> FFeatures{nullptr};
  UnicodeString FCurrentDirectory;
  bool FReadCurrentDirectory{false};
  UnicodeString FHomeDirectory;
  TRemoteFileList *FFileList{nullptr};
  TRemoteFileList *FFileListCache{nullptr};
  UnicodeString FFileListCachePath;
  UnicodeString FWelcomeMessage;
  bool FActive{false};
  bool FOpening{false};
  bool FWaitingForReply{false};
  __removed enum { ftaNone, ftaSkip, ftaCancel } FFileTransferAbort;
  enum
  {
    ftaNone,
    ftaSkip,
    ftaCancel,
  } FFileTransferAbort{ftaNone};
  bool FIgnoreFileList{false};
  bool FFileTransferCancelled{false};
  int64_t FFileTransferResumed{0};
  bool FFileTransferPreserveTime{false};
  bool FFileTransferRemoveBOM{false};
  bool FFileTransferNoList{false};
  uintptr_t FFileTransferCPSLimit;
  bool FAwaitingProgress{false};
  TCaptureOutputEvent FOnCaptureOutput;
  UnicodeString FUserName;
  TAutoSwitch FListAll;
  bool FDoListAll{false};
  std::unique_ptr<TFTPServerCapabilities> FServerCapabilities;
  TDateTime FLastDataSent{};
  bool FDetectTimeDifference{false};
  int64_t FTimeDifference{0};
  std::unique_ptr<TStrings> FChecksumAlgs;
  std::unique_ptr<TStrings> FChecksumCommands;
  std::unique_ptr<TStrings> FSupportedCommands;
  std::unique_ptr<TStrings> FSupportedSiteCommands;
  std::unique_ptr<TStrings> FHashAlgs;
  typedef rde::map<UnicodeString, TDateTime> TUploadedTimes;
  TUploadedTimes FUploadedTimes{};
  bool FSupportsAnyChecksumFeature{false};
  UnicodeString FLastCommandSent;
  X509 *FCertificate{nullptr};
  EVP_PKEY *FPrivateKey{nullptr};
  bool FTransferActiveImmediately{false};
  bool FWindowsServer{false};
  int64_t FBytesAvailable{0};
  bool FBytesAvailableSupported{false};
  bool FMVS{false};
  bool FVMS{false};
  bool FFileTransferAny{false};
  bool FLoggedIn{false};
  mutable UnicodeString FOptionScratch;
private:
  bool DoQuit();
};
//---------------------------------------------------------------------------
UnicodeString GetOpenSSLVersionText();
//---------------------------------------------------------------------------
