
#pragma once

#include <FileSystems.h>
#include <CopyParam.h>
//---------------------------------------------------------------------------
class TCommandSet;
class TSecureShell;
struct TOverwriteFileParams;
//---------------------------------------------------------------------------
NB_DEFINE_CLASS_ID(TSCPFileSystem);
class NB_CORE_EXPORT TSCPFileSystem final : public TCustomFileSystem
{
  NB_DISABLE_COPY(TSCPFileSystem)
public:
  static bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TSCPFileSystem); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TSCPFileSystem) || TCustomFileSystem::is(Kind); }
public:
  explicit TSCPFileSystem(TTerminal *ATerminal) noexcept;
  virtual ~TSCPFileSystem() noexcept;

  void Init(void * /*TSecureShell * */) override;
  void FileTransferProgress(int64_t /*TransferSize*/, int64_t /*Bytes*/) override {}

  void Open() override;
  void Close() override;
  bool GetActive() const override;
  void CollectUsage() override;
  void Idle() override;
  UnicodeString GetAbsolutePath(const UnicodeString APath, bool Local) override;
  UnicodeString GetAbsolutePath(const UnicodeString APath, bool Local) const override;
  void AnyCommand(const UnicodeString ACommand,
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
    const UnicodeString ATargetDir, const TCopyParamType *ACopyParam,
    intptr_t AParams, TFileOperationProgressType *OperationProgress,
    TOnceDoneOperation &OnceDoneOperation) override;
  void CopyToRemote(TStrings *AFilesToCopy,
    const UnicodeString ATargetDir, const TCopyParamType *CopyParam,
    intptr_t AParams, TFileOperationProgressType *OperationProgress,
    TOnceDoneOperation &OnceDoneOperation) override;
  void Source(
    TLocalFileHandle &AHandle, const UnicodeString ATargetDir, UnicodeString &ADestFileName,
    const TCopyParamType *CopyParam, intptr_t AParams,
    TFileOperationProgressType *OperationProgress, uintptr_t AFlags,
    TUploadSessionAction &Action, bool &ChildError) override;
  void Sink(
    const UnicodeString AFileName, const TRemoteFile *AFile,
    const UnicodeString ATargetDir, UnicodeString &ADestFileName, intptr_t Attrs,
    const TCopyParamType *CopyParam, intptr_t AParams, TFileOperationProgressType *OperationProgress,
    uintptr_t AFlags, TDownloadSessionAction &Action) override;
  void RemoteCreateDirectory(const UnicodeString ADirName, bool Encrypt) override;
  void RemoteCreateLink(const UnicodeString AFileName, const UnicodeString PointTo, bool Symbolic) override;
  void RemoteDeleteFile(const UnicodeString AFileName,
    const TRemoteFile *AFile, intptr_t AParams, TRmSessionAction & Action) override;
  void CustomCommandOnFile(const UnicodeString FileName,
    const TRemoteFile *AFile, UnicodeString ACommand, intptr_t AParams, TCaptureOutputEvent OutputEvent) override;
  void DoStartup() override;
  void HomeDirectory() override;
  bool IsCapable(intptr_t Capability) const override;
  void LookupUsersGroups() override;
  void ReadCurrentDirectory() override;
  void ReadDirectory(TRemoteFileList *FileList) override;
  void ReadFile(const UnicodeString AFileName,
    TRemoteFile *& AFile) override;
  void ReadSymlink(TRemoteFile *ASymlinkFile,
    TRemoteFile *& AFile) override;
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
  __property TStrings * Output = { read = FOutput };
  __property int ReturnCode = { read = FReturnCode };

  TStrings *GetOutput() const { return FOutput; }
  intptr_t GetReturnCode() const { return FReturnCode; }

  UnicodeString RemoteGetCurrentDirectory() const override;

private:
  TSecureShell *FSecureShell{nullptr};
  TCommandSet *FCommandSet{nullptr};
  TFileSystemInfo FFileSystemInfo;
  UnicodeString FCurrentDirectory;
  TStrings *FOutput{nullptr};
  intptr_t FReturnCode{0};
  UnicodeString FCachedDirectoryChange;
  bool FProcessingCommand{false};
  int FLsFullTime{0};
  TCaptureOutputEvent FOnCaptureOutput;
  bool FScpFatalError{false};

  void DetectUtf();
  void ClearAliases();
  void ClearAlias(const UnicodeString Alias);
  void CustomReadFile(const UnicodeString AFileName,
    TRemoteFile *& AFile, TRemoteFile *ALinkedByFile);
  static UnicodeString DelimitStr(const UnicodeString AStr);
  void DetectReturnVar();
  bool IsLastLine(UnicodeString &Line);
  static bool IsTotalListingLine(const UnicodeString ALine);
  void EnsureLocation();
  void ExecCommand(const UnicodeString ACmd, intptr_t AParams,
    const UnicodeString CmdString);
  void ExecCommand(TFSCommand Cmd, intptr_t Params, fmt::ArgList args);
  FMT_VARIADIC_W(void, ExecCommand, TFSCommand, intptr_t)

  __removed void ExecCommand(TFSCommand Cmd, const TVarRec *args = nullptr, int size = 0, int Params = -1);
  void ReadCommandOutput(intptr_t Params, const UnicodeString *Cmd = nullptr);
  void SCPResponse(bool *GotLastLine = nullptr);
  void SCPDirectorySource(const UnicodeString ADirectoryName,
    const UnicodeString ATargetDir, const TCopyParamType *CopyParam, intptr_t Params,
    TFileOperationProgressType *OperationProgress, intptr_t Level);
  void SCPError(const UnicodeString Message, bool Fatal);
  void SCPSendError(const UnicodeString Message, bool Fatal);
  void SCPSink(const UnicodeString ATargetDir,
    const UnicodeString AFileName, const UnicodeString ASourceDir,
    const TCopyParamType *CopyParam, bool &Success,
    TFileOperationProgressType *OperationProgress, intptr_t AParams, intptr_t Level);
  void SCPSource(const UnicodeString AFileName,
    const UnicodeString TargetDir, const TCopyParamType *CopyParam, intptr_t AParams,
    TFileOperationProgressType *OperationProgress, intptr_t Level);
  void SendCommand(const UnicodeString Cmd);
  void SkipFirstLine();
  void SkipStartupMessage();
  void UnsetNationalVars();
  TRemoteFile *CreateRemoteFile(const UnicodeString ListingStr,
    TRemoteFile *LinkedByFile = nullptr);
  void CaptureOutput(const UnicodeString AddedLine, TCaptureOutputType OutputType);
  void ChangeFileToken(const UnicodeString DelimitedName,
    const TRemoteToken &Token, TFSCommand Cmd, const UnicodeString RecursiveStr);
  uintptr_t ConfirmOverwrite(
    const UnicodeString ASourceFullFileName, const UnicodeString ATargetFileName,
    TOperationSide Side,
    const TOverwriteFileParams *FileParams, const TCopyParamType *CopyParam,
    intptr_t Params, TFileOperationProgressType *OperationProgress);

  static bool RemoveLastLine(UnicodeString &Line,
    intptr_t &ReturnCode, const UnicodeString ALastLine = L"");

  UnicodeString InitOptionsStr(const TCopyParamType *CopyParam) const;
};
//---------------------------------------------------------------------------
