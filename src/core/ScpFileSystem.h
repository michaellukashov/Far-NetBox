
#pragma once

#include <FileSystems.h>
#include <CopyParam.h>

class TCommandSet;
class TSecureShell;

class TSCPFileSystem : public TCustomFileSystem
{
NB_DISABLE_COPY(TSCPFileSystem)
public:
  static inline bool classof(const TObject * Obj)
  {
    return
      Obj->GetKind() == OBJECT_CLASS_TSCPFileSystem;
  }
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
  virtual UnicodeString GetAbsolutePath(const UnicodeString & APath, bool Local) override;
  virtual UnicodeString GetAbsolutePath(const UnicodeString & APath, bool Local) const override;
  virtual void AnyCommand(const UnicodeString & Command,
    TCaptureOutputEvent OutputEvent) override;
  virtual void ChangeDirectory(const UnicodeString & Directory) override;
  virtual void CachedChangeDirectory(const UnicodeString & Directory) override;
  virtual void AnnounceFileListOperation() override;
  virtual void ChangeFileProperties(const UnicodeString & AFileName,
    const TRemoteFile * AFile, const TRemoteProperties * Properties,
    TChmodSessionAction & Action) override;
  virtual bool LoadFilesProperties(TStrings * AFileList) override;
  virtual void CalculateFilesChecksum(const UnicodeString & Alg,
    TStrings * AFileList, TStrings * Checksums,
    TCalculatedChecksumEvent OnCalculatedChecksum) override;
  virtual void CopyToLocal(const TStrings * AFilesToCopy,
    const UnicodeString & TargetDir, const TCopyParamType * CopyParam,
    intptr_t Params, TFileOperationProgressType * OperationProgress,
    TOnceDoneOperation & OnceDoneOperation) override;
  virtual void CopyToRemote(const TStrings * AFilesToCopy,
    const UnicodeString & TargetDir, const TCopyParamType * CopyParam,
    intptr_t Params, TFileOperationProgressType * OperationProgress,
    TOnceDoneOperation & OnceDoneOperation) override;
  virtual void RemoteCreateDirectory(const UnicodeString & ADirName) override;
  virtual void CreateLink(const UnicodeString & AFileName, const UnicodeString & PointTo, bool Symbolic) override;
  virtual void RemoteDeleteFile(const UnicodeString & AFileName,
    const TRemoteFile * AFile, intptr_t Params, TRmSessionAction & Action) override;
  virtual void CustomCommandOnFile(const UnicodeString & AFileName,
    const TRemoteFile * AFile, const UnicodeString & Command, intptr_t Params, TCaptureOutputEvent OutputEvent) override;
  virtual void DoStartup() override;
  virtual void HomeDirectory() override;
  virtual bool IsCapable(intptr_t Capability) const override;
  virtual void LookupUsersGroups() override;
  virtual void ReadCurrentDirectory() override;
  virtual void ReadDirectory(TRemoteFileList * FileList) override;
  virtual void ReadFile(const UnicodeString & AFileName,
    TRemoteFile *& File) override;
  virtual void ReadSymlink(TRemoteFile * SymlinkFile,
    TRemoteFile *& File) override;
  virtual void RemoteRenameFile(const UnicodeString & AFileName,
    const UnicodeString & ANewName) override;
  virtual void RemoteCopyFile(const UnicodeString & AFileName,
    const UnicodeString & ANewName) override;
  virtual TStrings * GetFixedPaths() const override;
  virtual void SpaceAvailable(const UnicodeString & APath,
    TSpaceAvailable & ASpaceAvailable) override;
  virtual const TSessionInfo & GetSessionInfo() const override;
  virtual const TFileSystemInfo & GetFileSystemInfo(bool Retrieve) override;
  virtual bool TemporaryTransferFile(const UnicodeString & AFileName) override;
  virtual bool GetStoredCredentialsTried() const override;
  virtual UnicodeString RemoteGetUserName() const override;
  virtual void GetSupportedChecksumAlgs(TStrings * Algs) override;
  virtual void LockFile(const UnicodeString & AFileName, const TRemoteFile * AFile) override;
  virtual void UnlockFile(const UnicodeString & AFileName, const TRemoteFile * AFile) override;
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
  void CustomReadFile(const UnicodeString & AFileName,
    TRemoteFile *& File, TRemoteFile * ALinkedByFile);
  static UnicodeString DelimitStr(const UnicodeString & AStr);
  void DetectReturnVar();
  bool IsLastLine(UnicodeString & Line);
  static bool IsTotalListingLine(const UnicodeString & Line);
  void EnsureLocation();
  void ExecCommand2(const UnicodeString & Cmd, intptr_t Params,
    const UnicodeString & CmdString);
  void ExecCommand(TFSCommand Cmd, intptr_t Params, ...);
#if 0
  void ExecCommand(TFSCommand Cmd, const TVarRec * args = nullptr,
    int size = 0, int Params = -1);
#endif // #if 0
  void ReadCommandOutput(intptr_t Params, const UnicodeString * Cmd = nullptr);
  void SCPResponse(bool * GotLastLine = nullptr);
  void SCPDirectorySource(const UnicodeString & DirectoryName,
    const UnicodeString & TargetDir, const TCopyParamType * CopyParam, intptr_t Params,
    TFileOperationProgressType * OperationProgress, intptr_t Level);
  void SCPError(const UnicodeString Message, bool Fatal);
  void SCPSendError(const UnicodeString Message, bool Fatal);
  void SCPSink(const UnicodeString ATargetDir,
    const UnicodeString AFileName, const UnicodeString ASourceDir,
    const TRemoteFile * AFile,
    const TCopyParamType * CopyParam, bool & Success,
    TFileOperationProgressType * OperationProgress, intptr_t Params, intptr_t Level);
  void SCPSource(const UnicodeString & AFileName,
    const TRemoteFile * AFile,
    const UnicodeString TargetDir, const TCopyParamType * CopyParam, intptr_t Params,
    TFileOperationProgressType * OperationProgress, intptr_t Level);
  void SendCommand(const UnicodeString & Cmd);
  void SkipFirstLine();
  void SkipStartupMessage();
  void UnsetNationalVars();
  TRemoteFile * CreateRemoteFile(const UnicodeString & ListingStr,
    TRemoteFile * LinkedByFile = nullptr);
  void CaptureOutput(const UnicodeString & AddedLine, TCaptureOutputType OutputType);
  void ChangeFileToken(const UnicodeString & DelimitedName,
    const TRemoteToken & Token, TFSCommand Cmd, const UnicodeString & RecursiveStr);
  uintptr_t ConfirmOverwrite(
    const UnicodeString & ASourceFullFileName, const UnicodeString & ATargetFileName,
    TOperationSide Side,
    const TOverwriteFileParams * FileParams, const TCopyParamType * CopyParam,
    intptr_t Params, TFileOperationProgressType * OperationProgress);

  static bool RemoveLastLine(UnicodeString & Line,
    intptr_t & ReturnCode, UnicodeString ALastLine = L"");
};

