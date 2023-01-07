
#pragma once

#include <FileSystems.h>
#include <CopyParam.h>

class TCommandSet;
class TSecureShell;
struct TOverwriteFileParams;

NB_DEFINE_CLASS_ID(TSCPFileSystem);
class NB_CORE_EXPORT TSCPFileSystem final : public TCustomFileSystem
{
  NB_DISABLE_COPY(TSCPFileSystem)
public:
  static bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TSCPFileSystem); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TSCPFileSystem) || TCustomFileSystem::is(Kind); }
public:
  TSCPFileSystem() = delete;
  explicit TSCPFileSystem(TTerminal *ATerminal) noexcept;
  virtual ~TSCPFileSystem() noexcept;

  void Init(void * /*TSecureShell * */) override;
  void FileTransferProgress(int64_t /*TransferSize*/, int64_t /*Bytes*/) override {}

  void Open() override;
  void Close() override;
  bool GetActive() const override;
  void CollectUsage() override;
  void Idle() override;
  UnicodeString GetAbsolutePath(UnicodeString APath, bool Local) override;
  UnicodeString GetAbsolutePath(UnicodeString APath, bool Local) const override;
  void AnyCommand(UnicodeString ACommand,
    TCaptureOutputEvent OutputEvent) override;
  void ChangeDirectory(UnicodeString ADirectory) override;
  void CachedChangeDirectory(UnicodeString ADirectory) override;
  void AnnounceFileListOperation() override;
  void ChangeFileProperties(UnicodeString AFileName,
    const TRemoteFile *AFile, const TRemoteProperties *Properties,
    TChmodSessionAction &Action) override;
  bool LoadFilesProperties(TStrings *AFileList) override;
  void CalculateFilesChecksum(UnicodeString Alg,
    TStrings *AFileList, TStrings *Checksums,
    TCalculatedChecksumEvent OnCalculatedChecksum) override;
  void CopyToLocal(TStrings *AFilesToCopy,
    UnicodeString ATargetDir, const TCopyParamType *ACopyParam,
    intptr_t AParams, TFileOperationProgressType *OperationProgress,
    TOnceDoneOperation &OnceDoneOperation) override;
  void CopyToRemote(TStrings *AFilesToCopy,
    UnicodeString ATargetDir, const TCopyParamType *CopyParam,
    intptr_t AParams, TFileOperationProgressType *OperationProgress,
    TOnceDoneOperation &OnceDoneOperation) override;
  void Source(
    TLocalFileHandle &AHandle, UnicodeString ATargetDir, UnicodeString &ADestFileName,
    const TCopyParamType *CopyParam, intptr_t AParams,
    TFileOperationProgressType *OperationProgress, uintptr_t AFlags,
    TUploadSessionAction &Action, bool &ChildError) override;
  void Sink(
    UnicodeString AFileName, const TRemoteFile *AFile,
    UnicodeString ATargetDir, UnicodeString &ADestFileName, intptr_t Attrs,
    const TCopyParamType *CopyParam, intptr_t AParams, TFileOperationProgressType *OperationProgress,
    uintptr_t AFlags, TDownloadSessionAction &Action) override;
  void RemoteCreateDirectory(UnicodeString ADirName, bool Encrypt) override;
  void RemoteCreateLink(UnicodeString AFileName, UnicodeString PointTo, bool Symbolic) override;
  void RemoteDeleteFile(UnicodeString AFileName,
    const TRemoteFile *AFile, intptr_t AParams, TRmSessionAction & Action) override;
  void CustomCommandOnFile(UnicodeString FileName,
    const TRemoteFile *AFile, UnicodeString ACommand, intptr_t AParams, TCaptureOutputEvent OutputEvent) override;
  void DoStartup() override;
  void HomeDirectory() override;
  bool IsCapable(intptr_t Capability) const override;
  void LookupUsersGroups() override;
  void ReadCurrentDirectory() override;
  void ReadDirectory(TRemoteFileList *FileList) override;
  void ReadFile(UnicodeString AFileName,
    TRemoteFile *& AFile) override;
  void ReadSymlink(TRemoteFile *ASymlinkFile,
    TRemoteFile *& AFile) override;
  void RemoteRenameFile(UnicodeString AFileName, const TRemoteFile *AFile,
    UnicodeString ANewName) override;
  void RemoteCopyFile(UnicodeString AFileName, const TRemoteFile *AFile,
    UnicodeString ANewName) override;
  TStrings * GetFixedPaths() const override;
  void SpaceAvailable(UnicodeString APath,
    TSpaceAvailable &ASpaceAvailable) override;
  const TSessionInfo & GetSessionInfo() const override;
  const TFileSystemInfo & GetFileSystemInfo(bool Retrieve) override;
  bool TemporaryTransferFile(UnicodeString AFileName) override;
  bool GetStoredCredentialsTried() const override;
  UnicodeString RemoteGetUserName() const override;
  void GetSupportedChecksumAlgs(TStrings *Algs) override;
  void LockFile(UnicodeString AFileName, const TRemoteFile *AFile) override;
  void UnlockFile(UnicodeString AFileName, const TRemoteFile *AFile) override;
  void UpdateFromMain(TCustomFileSystem *MainFileSystem) override;
  void ClearCaches() override;

protected:
  __property TStrings * Output = { read = FOutput.get() };
  __property int ReturnCode = { read = FReturnCode };

  TStrings *GetOutput() const { return FOutput.get(); }
  intptr_t GetReturnCode() const { return FReturnCode; }

  UnicodeString RemoteGetCurrentDirectory() const override;

private:
  gsl::owner<TSecureShell*> FSecureShell{nullptr};
  std::unique_ptr<TCommandSet> FCommandSet;
  TFileSystemInfo FFileSystemInfo;
  UnicodeString FCurrentDirectory;
  std::unique_ptr<TStrings> FOutput;
  intptr_t FReturnCode{0};
  UnicodeString FCachedDirectoryChange;
  bool FProcessingCommand{false};
  int FLsFullTime{asAuto};
  TCaptureOutputEvent FOnCaptureOutput;
  bool FScpFatalError{false};

  void DetectUtf();
  void ClearAliases();
  void ClearAlias(UnicodeString Alias);
  void CustomReadFile(UnicodeString AFileName,
    TRemoteFile *& AFile, TRemoteFile *ALinkedByFile);
  static UnicodeString DelimitStr(UnicodeString AStr);
  void DetectReturnVar();
  bool IsLastLine(UnicodeString &Line);
  static bool IsTotalListingLine(UnicodeString ALine);
  void EnsureLocation();
  void ExecCommand(UnicodeString ACmd, intptr_t AParams,
    UnicodeString CmdString);
  void ExecCommand(TFSCommand Cmd, intptr_t Params, fmt::ArgList args);
  FMT_VARIADIC_W(void, ExecCommand, TFSCommand, intptr_t)

  __removed void ExecCommand(TFSCommand Cmd, const TVarRec *args = nullptr, int size = 0, int Params = -1);
  void ReadCommandOutput(intptr_t Params, UnicodeString *Cmd = nullptr);
  void SCPResponse(bool *GotLastLine = nullptr);
  void SCPDirectorySource(UnicodeString ADirectoryName,
    UnicodeString ATargetDir, const TCopyParamType *CopyParam, intptr_t Params,
    TFileOperationProgressType *OperationProgress, intptr_t Level);
  void SCPError(UnicodeString Message, bool Fatal);
  void SCPSendError(UnicodeString Message, bool Fatal);
  void SCPSink(UnicodeString ATargetDir,
    UnicodeString AFileName, UnicodeString ASourceDir,
    const TCopyParamType *CopyParam, bool &Success,
    TFileOperationProgressType *OperationProgress, intptr_t AParams, intptr_t Level);
  void SCPSource(UnicodeString AFileName,
    UnicodeString TargetDir, const TCopyParamType *CopyParam, intptr_t AParams,
    TFileOperationProgressType *OperationProgress, intptr_t Level);
  void SendCommand(UnicodeString Cmd);
  void SkipFirstLine();
  void SkipStartupMessage();
  void UnsetNationalVars();
  TRemoteFile *CreateRemoteFile(UnicodeString ListingStr,
    TRemoteFile *LinkedByFile = nullptr);
  void CaptureOutput(UnicodeString AddedLine, TCaptureOutputType OutputType);
  void ChangeFileToken(UnicodeString DelimitedName,
    const TRemoteToken &Token, TFSCommand Cmd, UnicodeString RecursiveStr);
  uintptr_t ConfirmOverwrite(
    UnicodeString ASourceFullFileName, UnicodeString ATargetFileName,
    TOperationSide Side,
    const TOverwriteFileParams *FileParams, const TCopyParamType *CopyParam,
    intptr_t Params, TFileOperationProgressType *OperationProgress);

  static bool RemoveLastLine(UnicodeString &Line,
    intptr_t &ReturnCode, UnicodeString ALastLine = "");

  UnicodeString InitOptionsStr(const TCopyParamType *CopyParam) const;
};

