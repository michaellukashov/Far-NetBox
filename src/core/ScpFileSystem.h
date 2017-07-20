
#pragma once

#include <FileSystems.h>
#include <CopyParam.h>

class TCommandSet;
class TSecureShell;

class TSCPFileSystem : public TCustomFileSystem
{
NB_DISABLE_COPY(TSCPFileSystem)
public:
  static inline bool classof(const TObject * Obj) { return Obj->is(OBJECT_CLASS_TSCPFileSystem); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TSCPFileSystem) || TCustomFileSystem::is(Kind); }
public:
  explicit TSCPFileSystem(TTerminal * ATerminal);
  virtual ~TSCPFileSystem();

  virtual void Init(void * /*TSecureShell * */);
  virtual void FileTransferProgress(int64_t /*TransferSize*/, int64_t /*Bytes*/) {}

  virtual void Open();
  virtual void Close();
  virtual bool GetActive() const;
  virtual void CollectUsage();
  virtual void Idle();
  virtual UnicodeString GetAbsolutePath(UnicodeString APath, bool Local) override;
  virtual UnicodeString GetAbsolutePath(UnicodeString APath, bool Local) const override;
  virtual void AnyCommand(UnicodeString Command,
    TCaptureOutputEvent OutputEvent) override;
  virtual void ChangeDirectory(UnicodeString Directory) override;
  virtual void CachedChangeDirectory(UnicodeString Directory) override;
  virtual void AnnounceFileListOperation() override;
  virtual void ChangeFileProperties(UnicodeString AFileName,
    const TRemoteFile * AFile, const TRemoteProperties * Properties,
    TChmodSessionAction & Action) override;
  virtual bool LoadFilesProperties(TStrings * AFileList) override;
  virtual void CalculateFilesChecksum(UnicodeString Alg,
    TStrings * AFileList, TStrings * Checksums,
    TCalculatedChecksumEvent OnCalculatedChecksum) override;
  virtual void CopyToLocal(const TStrings * AFilesToCopy,
    UnicodeString TargetDir, const TCopyParamType * CopyParam,
    intptr_t Params, TFileOperationProgressType * OperationProgress,
    TOnceDoneOperation & OnceDoneOperation) override;
  virtual void CopyToRemote(const TStrings * AFilesToCopy,
    UnicodeString TargetDir, const TCopyParamType * CopyParam,
    intptr_t Params, TFileOperationProgressType * OperationProgress,
    TOnceDoneOperation & OnceDoneOperation) override;
  virtual void RemoteCreateDirectory(UnicodeString ADirName) override;
  virtual void CreateLink(UnicodeString AFileName, UnicodeString PointTo, bool Symbolic) override;
  virtual void RemoteDeleteFile(UnicodeString AFileName,
    const TRemoteFile * AFile, intptr_t Params, TRmSessionAction & Action) override;
  virtual void CustomCommandOnFile(UnicodeString AFileName,
    const TRemoteFile * AFile, UnicodeString Command, intptr_t Params, TCaptureOutputEvent OutputEvent) override;
  virtual void DoStartup() override;
  virtual void HomeDirectory() override;
  virtual bool IsCapable(intptr_t Capability) const override;
  virtual void LookupUsersGroups() override;
  virtual void ReadCurrentDirectory() override;
  virtual void ReadDirectory(TRemoteFileList * FileList) override;
  virtual void ReadFile(UnicodeString AFileName,
    TRemoteFile *& File) override;
  virtual void ReadSymlink(TRemoteFile * SymlinkFile,
    TRemoteFile *& File) override;
  virtual void RemoteRenameFile(UnicodeString AFileName,
    UnicodeString ANewName) override;
  virtual void RemoteCopyFile(UnicodeString AFileName,
    UnicodeString ANewName) override;
  virtual TStrings * GetFixedPaths() const override;
  virtual void SpaceAvailable(UnicodeString APath,
    TSpaceAvailable & ASpaceAvailable) override;
  virtual const TSessionInfo & GetSessionInfo() const override;
  virtual const TFileSystemInfo & GetFileSystemInfo(bool Retrieve) override;
  virtual bool TemporaryTransferFile(UnicodeString AFileName) override;
  virtual bool GetStoredCredentialsTried() const override;
  virtual UnicodeString RemoteGetUserName() const override;
  virtual void GetSupportedChecksumAlgs(TStrings * Algs) override;
  virtual void LockFile(UnicodeString AFileName, const TRemoteFile * AFile) override;
  virtual void UnlockFile(UnicodeString AFileName, const TRemoteFile * AFile) override;
  virtual void UpdateFromMain(TCustomFileSystem * MainFileSystem) override;

protected:
#if 0
  __property TStrings * Output = { read = FOutput };
  __property int ReturnCode = { read = FReturnCode };
#endif // #if 0
  TStrings * GetOutput() const { return FOutput; }
  intptr_t GetReturnCode() const { return FReturnCode; }

  virtual UnicodeString RemoteGetCurrentDirectory() const override;

private:
  TSecureShell * FSecureShell;
  TCommandSet * FCommandSet;
  TFileSystemInfo FFileSystemInfo;
  UnicodeString FCurrentDirectory;
  TStrings * FOutput;
  intptr_t FReturnCode;
  UnicodeString FCachedDirectoryChange;
  bool FProcessingCommand;
  int FLsFullTime;
  TCaptureOutputEvent FOnCaptureOutput;
  bool FScpFatalError;

  void DetectUtf();
  void ClearAliases();
  void ClearAlias(UnicodeString Alias);
  void CustomReadFile(UnicodeString AFileName,
    TRemoteFile *& File, TRemoteFile * ALinkedByFile);
  static UnicodeString DelimitStr(UnicodeString AStr);
  void DetectReturnVar();
  bool IsLastLine(UnicodeString & Line);
  static bool IsTotalListingLine(UnicodeString Line);
  void EnsureLocation();
  void ExecCommand2(UnicodeString Cmd, intptr_t Params,
    UnicodeString CmdString);
  void ExecCommand(TFSCommand Cmd, intptr_t Params, ...);
#if 0
  void ExecCommand(TFSCommand Cmd, const TVarRec * args = nullptr,
    int size = 0, int Params = -1);
#endif // #if 0
  void ReadCommandOutput(intptr_t Params, const UnicodeString * Cmd = nullptr);
  void SCPResponse(bool * GotLastLine = nullptr);
  void SCPDirectorySource(UnicodeString DirectoryName,
    UnicodeString TargetDir, const TCopyParamType * CopyParam, intptr_t Params,
    TFileOperationProgressType * OperationProgress, intptr_t Level);
  void SCPError(const UnicodeString Message, bool Fatal);
  void SCPSendError(const UnicodeString Message, bool Fatal);
  void SCPSink(const UnicodeString ATargetDir,
    const UnicodeString AFileName, const UnicodeString ASourceDir,
    const TRemoteFile * AFile,
    const TCopyParamType * CopyParam, bool & Success,
    TFileOperationProgressType * OperationProgress, intptr_t Params, intptr_t Level);
  void SCPSource(UnicodeString AFileName,
    const TRemoteFile * AFile,
    const UnicodeString TargetDir, const TCopyParamType * CopyParam, intptr_t Params,
    TFileOperationProgressType * OperationProgress, intptr_t Level);
  void SendCommand(UnicodeString Cmd);
  void SkipFirstLine();
  void SkipStartupMessage();
  void UnsetNationalVars();
  TRemoteFile * CreateRemoteFile(UnicodeString ListingStr,
    TRemoteFile * LinkedByFile = nullptr);
  void CaptureOutput(UnicodeString AddedLine, TCaptureOutputType OutputType);
  void ChangeFileToken(UnicodeString DelimitedName,
    const TRemoteToken & Token, TFSCommand Cmd, UnicodeString RecursiveStr);
  uintptr_t ConfirmOverwrite(
    UnicodeString ASourceFullFileName, UnicodeString ATargetFileName,
    TOperationSide Side,
    const TOverwriteFileParams * FileParams, const TCopyParamType * CopyParam,
    intptr_t Params, TFileOperationProgressType * OperationProgress);

  static bool RemoveLastLine(UnicodeString & Line,
    intptr_t & ReturnCode, UnicodeString ALastLine = L"");
};

