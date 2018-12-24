
#pragma once

#include <FileSystems.h>
#include <CopyParam.h>
//---------------------------------------------------------------------------
class TCommandSet;
class TSecureShell;
struct TOverwriteFileParams;
//---------------------------------------------------------------------------
NB_DEFINE_CLASS_ID(TSCPFileSystem);
class NB_CORE_EXPORT TSCPFileSystem : public TCustomFileSystem
{
  NB_DISABLE_COPY(TSCPFileSystem)
public:
  static inline bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TSCPFileSystem); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TSCPFileSystem) || TCustomFileSystem::is(Kind); }
public:
  explicit TSCPFileSystem(TTerminal *ATerminal);
  virtual ~TSCPFileSystem();

  virtual void Init(void * /*TSecureShell * */) override;
  virtual void FileTransferProgress(int64_t /*TransferSize*/, int64_t /*Bytes*/) override {}

  virtual void Open() override;
  virtual void Close() override;
  virtual bool GetActive() const override;
  virtual void CollectUsage() override;
  virtual void Idle() override;
  virtual UnicodeString GetAbsolutePath(const UnicodeString APath, bool Local) override;
  virtual UnicodeString GetAbsolutePath(const UnicodeString APath, bool Local) const override;
  virtual void AnyCommand(const UnicodeString ACommand,
    TCaptureOutputEvent OutputEvent) override;
  virtual void ChangeDirectory(const UnicodeString ADirectory) override;
  virtual void CachedChangeDirectory(const UnicodeString ADirectory) override;
  virtual void AnnounceFileListOperation() override;
  virtual void ChangeFileProperties(const UnicodeString AFileName,
    const TRemoteFile *AFile, const TRemoteProperties *Properties,
    TChmodSessionAction &Action) override;
  virtual bool LoadFilesProperties(TStrings *AFileList) override;
  virtual void CalculateFilesChecksum(const UnicodeString Alg,
    TStrings *AFileList, TStrings *Checksums,
    TCalculatedChecksumEvent OnCalculatedChecksum) override;
  virtual void CopyToLocal(TStrings *AFilesToCopy,
    const UnicodeString ATargetDir, const TCopyParamType *ACopyParam,
    intptr_t AParams, TFileOperationProgressType *OperationProgress,
    TOnceDoneOperation &OnceDoneOperation) override;
  virtual void CopyToRemote(TStrings *AFilesToCopy,
    const UnicodeString ATargetDir, const TCopyParamType *CopyParam,
    intptr_t AParams, TFileOperationProgressType *OperationProgress,
    TOnceDoneOperation &OnceDoneOperation) override;
  virtual void Source(
    TLocalFileHandle &AHandle, const UnicodeString ATargetDir, UnicodeString &ADestFileName,
    const TCopyParamType *CopyParam, intptr_t AParams,
    TFileOperationProgressType *OperationProgress, uintptr_t AFlags,
    TUploadSessionAction &Action, bool &ChildError) override;
  virtual void Sink(
    const UnicodeString AFileName, const TRemoteFile *AFile,
    const UnicodeString ATargetDir, UnicodeString &ADestFileName, intptr_t Attrs,
    const TCopyParamType *CopyParam, intptr_t AParams, TFileOperationProgressType *OperationProgress,
    uintptr_t AFlags, TDownloadSessionAction &Action) override;
  virtual void RemoteCreateDirectory(const UnicodeString ADirName, bool Encrypt) override;
  virtual void RemoteCreateLink(const UnicodeString AFileName, const UnicodeString PointTo, bool Symbolic) override;
  virtual void RemoteDeleteFile(const UnicodeString AFileName,
    const TRemoteFile *AFile, intptr_t AParams, TRmSessionAction & Action) override;
  virtual void CustomCommandOnFile(const UnicodeString FileName,
    const TRemoteFile *AFile, const UnicodeString ACommand, intptr_t AParams, TCaptureOutputEvent OutputEvent) override;
  virtual void DoStartup() override;
  virtual void HomeDirectory() override;
  virtual bool IsCapable(intptr_t Capability) const override;
  virtual void LookupUsersGroups() override;
  virtual void ReadCurrentDirectory() override;
  virtual void ReadDirectory(TRemoteFileList *FileList) override;
  virtual void ReadFile(const UnicodeString AFileName,
    TRemoteFile *& AFile) override;
  virtual void ReadSymlink(TRemoteFile *ASymlinkFile,
    TRemoteFile *& AFile) override;
  virtual void RemoteRenameFile(const UnicodeString AFileName, const TRemoteFile *AFile,
    const UnicodeString ANewName) override;
  virtual void RemoteCopyFile(const UnicodeString AFileName, const TRemoteFile *AFile,
    const UnicodeString ANewName) override;
  virtual TStrings * GetFixedPaths() const override;
  virtual void SpaceAvailable(const UnicodeString APath,
    TSpaceAvailable &ASpaceAvailable) override;
  virtual const TSessionInfo & GetSessionInfo() const override;
  virtual const TFileSystemInfo & GetFileSystemInfo(bool Retrieve);
  virtual bool TemporaryTransferFile(const UnicodeString AFileName) override;
  virtual bool GetStoredCredentialsTried() const override;
  virtual UnicodeString RemoteGetUserName() const override;
  virtual void GetSupportedChecksumAlgs(TStrings *Algs) override;
  virtual void LockFile(const UnicodeString AFileName, const TRemoteFile *AFile) override;
  virtual void UnlockFile(const UnicodeString AFileName, const TRemoteFile *AFile) override;
  virtual void UpdateFromMain(TCustomFileSystem *MainFileSystem) override;
  virtual void ClearCaches() override;

protected:
  __property TStrings * Output = { read = FOutput };
  __property int ReturnCode = { read = FReturnCode };

  TStrings *GetOutput() const { return FOutput; }
  intptr_t GetReturnCode() const { return FReturnCode; }

  virtual UnicodeString RemoteGetCurrentDirectory() const override;

private:
  TSecureShell *FSecureShell;
  TCommandSet *FCommandSet;
  TFileSystemInfo FFileSystemInfo;
  UnicodeString FCurrentDirectory;
  TStrings *FOutput;
  intptr_t FReturnCode;
  UnicodeString FCachedDirectoryChange;
  bool FProcessingCommand;
  int FLsFullTime;
  TCaptureOutputEvent FOnCaptureOutput;
  bool FScpFatalError;

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
  uint32_t ConfirmOverwrite(
    const UnicodeString ASourceFullFileName, const UnicodeString ATargetFileName,
    TOperationSide Side,
    const TOverwriteFileParams *FileParams, const TCopyParamType *CopyParam,
    intptr_t Params, TFileOperationProgressType *OperationProgress);

  static bool RemoveLastLine(UnicodeString &Line,
    intptr_t &ReturnCode, const UnicodeString ALastLine = L"");

  UnicodeString InitOptionsStr(const TCopyParamType *CopyParam) const;
};
//---------------------------------------------------------------------------
