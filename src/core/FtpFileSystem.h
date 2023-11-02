
#pragma once



#include <time.h>
#include <rdestl/map.h>
#include <FileSystems.h>

class TFileZillaIntf;
class TFileZillaImpl;
class TMessageQueue;
class TFTPServerCapabilities;
struct TOverwriteFileParams;
struct TListDataEntry;
struct TFileTransferData;
struct TFtpsCertificateData;
struct TRemoteFileTime;

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
  TFTPFileSystem() = delete;
  explicit TFTPFileSystem(TTerminal *ATerminal) noexcept;
  virtual ~TFTPFileSystem() noexcept;
  void Init(void *) override;

  virtual void Open() override;
  virtual void Close() override;
  virtual bool GetActive() const override;
  virtual void CollectUsage() override;
  virtual void Idle() override;
  virtual UnicodeString GetAbsolutePath(UnicodeString APath, bool Local) override;
  virtual UnicodeString GetAbsolutePath(UnicodeString APath, bool Local) const override;
  virtual void AnyCommand(const UnicodeString Command,
    TCaptureOutputEvent OutputEvent) override;
  virtual void ChangeDirectory(const UnicodeString ADirectory) override;
  virtual void CachedChangeDirectory(const UnicodeString ADirectory) override;
  virtual void AnnounceFileListOperation() override;
  virtual void ChangeFileProperties(const UnicodeString AFileName,
    const TRemoteFile *AFile, const TRemoteProperties *Properties,
    TChmodSessionAction &Action) override;
  virtual bool LoadFilesProperties(TStrings *AFileList) override;
  virtual UnicodeString CalculateFilesChecksumInitialize(const UnicodeString & Alg) override;
  void CalculateFilesChecksum(
    const UnicodeString & Alg, TStrings * FileList, TCalculatedChecksumEvent OnCalculatedChecksum,
    TFileOperationProgressType * OperationProgress, bool FirstLevel);
  virtual void CopyToLocal(TStrings *AFilesToCopy,
    const UnicodeString ATargetDir, const TCopyParamType *CopyParam,
    int32_t Params, TFileOperationProgressType *OperationProgress,
    TOnceDoneOperation &OnceDoneOperation) override;
  virtual void TransferOnDirectory(
    const UnicodeString ADirectory, const TCopyParamType *CopyParam, int32_t AParams);
  virtual void CopyToRemote(TStrings *AFilesToCopy,
    const UnicodeString ATargetDir, const TCopyParamType *CopyParam,
    int32_t AParams, TFileOperationProgressType *OperationProgress,
    TOnceDoneOperation &OnceDoneOperation) override;
  virtual void Source(
    TLocalFileHandle &AHandle, const UnicodeString ATargetDir, UnicodeString &ADestFileName,
    const TCopyParamType *CopyParam, int32_t AParams,
    TFileOperationProgressType *OperationProgress, uint32_t AFlags,
    TUploadSessionAction &Action, bool &ChildError);
  virtual void Sink(
    const UnicodeString AFileName, const TRemoteFile *AFile,
    const UnicodeString ATargetDir, UnicodeString &ADestFileName, int32_t Attrs,
    const TCopyParamType *CopyParam, int32_t AParams, TFileOperationProgressType *OperationProgress,
    uint32_t AFlags, TDownloadSessionAction &Action);
  virtual void RemoteCreateDirectory(const UnicodeString ADirName, bool Encrypt) override;
  virtual void RemoteCreateLink(const UnicodeString AFileName, const UnicodeString APointTo, bool Symbolic) override;
  virtual void RemoteDeleteFile(const UnicodeString AFileName,
    const TRemoteFile *AFile, int32_t Params, TRmSessionAction &Action) override;
  virtual void CustomCommandOnFile(const UnicodeString AFileName,
    const TRemoteFile *AFile, UnicodeString ACommand, int32_t AParams, TCaptureOutputEvent OutputEvent) override;
  virtual void DoStartup() override;
  virtual void HomeDirectory() override;
  virtual bool IsCapable(int32_t Capability) const override;
  virtual void LookupUsersGroups() override;
  virtual void ReadCurrentDirectory() override;
  virtual void ReadDirectory(TRemoteFileList *FileList) override;
  virtual void ReadFile(const UnicodeString AFileName,
    TRemoteFile *&AFile) override;
  virtual void ReadSymlink(TRemoteFile *SymlinkFile,
    TRemoteFile *&AFile) override;
  virtual void RemoteRenameFile(
   const UnicodeString & AFileName, const TRemoteFile * AFile, const UnicodeString & ANewName, bool Overwrite) override;
  virtual void RemoteCopyFile(
    const UnicodeString & AFileName, const TRemoteFile * AFile, const UnicodeString & ANewName, bool Overwrite) override;
  virtual TStrings * GetFixedPaths() const override;
  virtual void SpaceAvailable(const UnicodeString APath,
    TSpaceAvailable &ASpaceAvailable) override;
  virtual const TSessionInfo & GetSessionInfo() const override;
  virtual const TFileSystemInfo & GetFileSystemInfo(bool Retrieve) override;
  virtual bool TemporaryTransferFile(const UnicodeString AFileName) override;
  virtual bool GetStoredCredentialsTried() const override;
  virtual UnicodeString RemoteGetUserName() const override;
  virtual void GetSupportedChecksumAlgs(TStrings *Algs) override;
  virtual void LockFile(const UnicodeString AFileName, const TRemoteFile *AFile) override;
  virtual void UnlockFile(const UnicodeString AFileName, const TRemoteFile *AFile) override;
  virtual void UpdateFromMain(TCustomFileSystem *MainFileSystem) override;
  virtual void ClearCaches() override;

protected:
  __removed enum TOverwriteMode { omOverwrite, omResume, omComplete }; // moved to FileSystems.h

  virtual UnicodeString RemoteGetCurrentDirectory() const override;

  const wchar_t *GetOption(int32_t OptionID) const;
  int32_t GetOptionVal(int32_t OptionID) const;

  enum
  {
    REPLY_CONNECT      = 0x01,
    REPLY_2XX_CODE     = 0x02,
    REPLY_ALLOW_CANCEL = 0x04,
    REPLY_3XX_CODE     = 0x08,
    REPLY_SINGLE_LINE  = 0x10,
  };

  bool FTPPostMessage(uint32_t Type, WPARAM wParam, LPARAM lParam);
  bool ProcessMessage();
  void DiscardMessages();
  void WaitForMessages();
  uint32_t WaitForReply(bool Command, bool WantLastCode);
  uint32_t WaitForCommandReply(bool WantLastCode = true);
  void WaitForFatalNonCommandReply();
  void PoolForFatalNonCommandReply();
  void GotNonCommandReply(uint32_t Reply);
  UnicodeString GotReply(uint32_t Reply, uint32_t Flags = 0,
    UnicodeString Error = "", uint32_t *Code = nullptr,
    TStrings **Response = nullptr);
  void ResetReply();
  void HandleReplyStatus(UnicodeString Response);
  void DoWaitForReply(uint32_t &ReplyToAwait, bool WantLastCode);
  bool KeepWaitingForReply(uint32_t &ReplyToAwait, bool WantLastCode) const;
  inline bool NoFinalLastCode() const;

  bool HandleStatus(const wchar_t *AStatus, int Type);
  bool HandleAsyncRequestOverwrite(
    wchar_t *FileName1, size_t FileName1Len, const wchar_t *FileName2,
    const wchar_t *Path1, const wchar_t *Path2,
    int64_t Size1, int64_t Size2, time_t LocalTime,
    bool HasLocalTime, const TRemoteFileTime &RemoteTime, void *AUserData,
    HANDLE &ALocalFileHandle,
    int &RequestResult);
  bool HandleAsyncRequestVerifyCertificate(
    const TFtpsCertificateData &Data, int &RequestResult);
  bool HandleAsyncRequestNeedPass(
    struct TNeedPassRequestData &Data, int &RequestResult) const;
  bool HandleListData(const wchar_t *Path, const TListDataEntry *Entries,
    uint32_t Count);
  bool HandleTransferStatus(bool Valid, int64_t TransferSize,
    int64_t Bytes, bool FileTransfer);
  bool HandleReply(int32_t Command, uint32_t Reply);
  bool HandleCapabilities(TFTPServerCapabilities *ServerCapabilities);
  bool CheckError(int32_t ReturnCode, const wchar_t *Context);
  void PreserveDownloadFileTime(HANDLE AHandle, void *UserData) const;
  bool GetFileModificationTimeInUtc(const wchar_t *AFileName, struct tm &Time);
  void EnsureLocation(const UnicodeString ADirectory, bool Log);
  void EnsureLocation();
  bool EnsureLocationWhenWorkFromCwd(const UnicodeString & Directory);
  UnicodeString GetActualCurrentDirectory() const;
  void Discard();
  void DoChangeDirectory(const UnicodeString ADirectory);
  void SendCwd(const UnicodeString & Directory);

  void Sink(const UnicodeString AFileName,
    const TRemoteFile *AFile, const UnicodeString ATargetDir,
    const TCopyParamType *CopyParam, int32_t AParams,
    TFileOperationProgressType *OperationProgress, uint32_t AFlags,
    TDownloadSessionAction &Action);
  bool ConfirmOverwrite(const UnicodeString ASourceFullFileName, UnicodeString &ATargetFileName,
    TOverwriteMode &OverwriteMode, TFileOperationProgressType *OperationProgress,
    const TOverwriteFileParams *FileParams, const TCopyParamType *CopyParam,
    int32_t AParams, bool AutoResume);
  void ReadDirectoryProgress(int64_t Bytes);
  void ResetFileTransfer();
  void DoFileTransferProgress(int64_t TransferSize, int64_t Bytes);
  void FileTransferProgress(int64_t TransferSize, int64_t Bytes) override;
  void ResetCaches();
  void CaptureOutput(const UnicodeString AStr);
  void DoReadDirectory(TRemoteFileList *AFileList);
  void DoReadFile(const UnicodeString AFileName, TRemoteFile *& AFile);
  void FileTransfer(const UnicodeString AFileName, const UnicodeString LocalFile,
    const UnicodeString RemoteFile, const UnicodeString RemotePath, bool Get,
    int64_t Size, int32_t Type, TFileTransferData &UserData,
    TFileOperationProgressType *OperationProgress);
  TDateTime ConvertLocalTimestamp(time_t Time);
  void RemoteFileTimeToDateTimeAndPrecision(const TRemoteFileTime &Source,
    TDateTime &DateTime, TModificationFmt &ModificationFmt) const;
  void SetLastCode(int32_t Code);
  void StoreLastResponse(const UnicodeString Text);
  void SetCPSLimit(TFileOperationProgressType *OperationProgress);
  bool VerifyCertificateHostName(const TFtpsCertificateData &Data);
  bool SupportsReadingFile() const;
  void AutoDetectTimeDifference(TRemoteFileList *FileList);
  void AutoDetectTimeDifference(const UnicodeString ADirectory, const TCopyParamType *CopyParam, int32_t AParams);
  void ApplyTimeDifference(TRemoteFile *File);
  void ApplyTimeDifference(
    const UnicodeString FileName, TDateTime &Modification, TModificationFmt &ModificationFmt);
  void DummyReadDirectory(const UnicodeString ADirectory);
  bool IsEmptyFileList(TRemoteFileList *FileList) const;
  void CheckTimeDifference();
  bool NeedAutoDetectTimeDifference() const;
  bool LookupUploadModificationTime(
    const UnicodeString FileName, TDateTime &Modification, TModificationFmt ModificationFmt);
  UnicodeString DoCalculateFileChecksum(const UnicodeString & Alg, TRemoteFile * File);
  bool UsingHashCommandChecksum(const UnicodeString & Alg) const;
  void HandleFeatReply();
  void ResetFeatures();
  void ProcessFeatures();
  bool SupportsSiteCommand(const UnicodeString ACommand) const;
  bool SupportsCommand(const UnicodeString ACommand) const;
  void RegisterChecksumAlgCommand(const UnicodeString Alg, const UnicodeString ACommand);
  void SendCommand(const UnicodeString ACommand);
  bool CanTransferSkipList(int32_t AParams, uint32_t AFlags, const TCopyParamType *CopyParam) const;
  void Disconnect();
  UnicodeString RemoteExtractFilePath(const UnicodeString & Path);

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
  HANDLE FQueueEvent{nullptr};
  TSessionInfo FSessionInfo;
  TFileSystemInfo FFileSystemInfo;
  bool FFileSystemInfoValid{false};
  uint32_t FReply{0};
  uint32_t FCommandReply{0};
  TCommand FLastCommand{CMD_UNKNOWN};
  bool FPasswordFailed{false};
  bool FStoredPasswordTried{false};
  bool FMultiLineResponse{false};
  int32_t FLastCode{0};
  int32_t FLastCodeClass{0};
  int32_t FLastReadDirectoryProgress{0};
  UnicodeString FTimeoutStatus;
  UnicodeString FDisconnectStatus;
  std::unique_ptr<TStrings> FLastResponse{nullptr};
  std::unique_ptr<TStrings> FLastErrorResponse{nullptr};
  std::unique_ptr<TStrings> FLastError{nullptr};
  UnicodeString FSystem;
  UnicodeString FServerID;
  std::unique_ptr<TStrings> FFeatures{nullptr};
  UnicodeString FCurrentDirectory;
  bool FReadCurrentDirectory{false};
  UnicodeString FHomeDirectory;
  TRemoteFileList * FFileList{nullptr};
  TRemoteFileList * FFileListCache{nullptr};
  UnicodeString FFileListCachePath;
  UnicodeString FWelcomeMessage;
  bool FActive{false};
  bool FOpening{false};
  bool FWaitingForReply{false};
  enum { ftaNone, ftaSkip, ftaCancel } FFileTransferAbort{ftaNone};
  bool FIgnoreFileList{false};
  bool FFileTransferCancelled{false};
  int64_t FFileTransferResumed{0};
  bool FFileTransferPreserveTime{false};
  bool FFileTransferRemoveBOM{false};
  bool FFileTransferNoList{false};
  uint32_t FFileTransferCPSLimit;
  bool FAwaitingProgress{false};
  TCaptureOutputEvent FOnCaptureOutput;
  UnicodeString FUserName;
  TAutoSwitch FListAll;
  bool FDoListAll{false};
  TAutoSwitch FWorkFromCwd;
  std::unique_ptr<TFTPServerCapabilities> FServerCapabilities;
  TDateTime FLastDataSent{};
  bool FAnyTransferSucceeded{false};
  bool FDetectTimeDifference{false};
  int64_t FTimeDifference{0};
  std::unique_ptr<TStrings> FChecksumAlgs;
  std::unique_ptr<TStrings> FChecksumCommands;
  std::unique_ptr<TStrings> FSupportedCommands;
  std::unique_ptr<TStrings> FSupportedSiteCommands;
  std::unique_ptr<TStrings> FHashAlgs;
  using TUploadedTimes = nb::map_t<UnicodeString, TDateTime>;
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
  bool FFileZilla{false};
  bool FIIS{false};
  bool FFileTransferAny{false};
  bool FLoggedIn{false};
  bool FVMSAllRevisions{false};
  bool FForceReadSymlink{false};
  mutable UnicodeString FOptionScratch;
private:
  bool DoQuit();
};

UnicodeString GetOpenSSLVersionText();

