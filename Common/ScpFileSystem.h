//---------------------------------------------------------------------------
#ifndef ScpFileSystemH
#define ScpFileSystemH

#include <FileSystems.h>
//---------------------------------------------------------------------------
class TCommandSet;
class TSecureShell;
//---------------------------------------------------------------------------
class TSCPFileSystem : public TCustomFileSystem
{
public:
  TSCPFileSystem(TTerminal * ATerminal, TSecureShell * SecureShell);
  virtual ~TSCPFileSystem();

  virtual void Open();
  virtual void Close();
  virtual bool GetActive();
  virtual void Idle();
  virtual std::wstring AbsolutePath(std::wstring Path, bool Local);
  virtual void AnyCommand(const std::wstring Command,
    TCaptureOutputEvent OutputEvent);
  virtual void ChangeDirectory(const std::wstring Directory);
  virtual void CachedChangeDirectory(const std::wstring Directory);
  virtual void AnnounceFileListOperation();
  virtual void ChangeFileProperties(const std::wstring FileName,
    const TRemoteFile * File, const TRemoteProperties * Properties,
    TChmodSessionAction & Action);
  virtual bool LoadFilesProperties(TStrings * FileList);
  virtual void CalculateFilesChecksum(const std::wstring & Alg,
    TStrings * FileList, TStrings * Checksums,
    TCalculatedChecksumEvent OnCalculatedChecksum);
  virtual void CopyToLocal(TStrings * FilesToCopy,
    const std::wstring TargetDir, const TCopyParamType * CopyParam,
    int Params, TFileOperationProgressType * OperationProgress,
    TOnceDoneOperation & OnceDoneOperation);
  virtual void CopyToRemote(TStrings * FilesToCopy,
    const std::wstring TargetDir, const TCopyParamType * CopyParam,
    int Params, TFileOperationProgressType * OperationProgress,
    TOnceDoneOperation & OnceDoneOperation);
  virtual void CreateDirectory(const std::wstring DirName);
  virtual void CreateLink(const std::wstring FileName, const std::wstring PointTo, bool Symbolic);
  virtual void DeleteFile(const std::wstring FileName,
    const TRemoteFile * File, int Params, TRmSessionAction & Action);
  virtual void CustomCommandOnFile(const std::wstring FileName,
    const TRemoteFile * File, std::wstring Command, int Params, TCaptureOutputEvent OutputEvent);
  virtual void DoStartup();
  virtual void HomeDirectory();
  virtual bool IsCapable(int Capability) const;
  virtual void LookupUsersGroups();
  virtual void ReadCurrentDirectory();
  virtual void ReadDirectory(TRemoteFileList * FileList);
  virtual void ReadFile(const std::wstring FileName,
    TRemoteFile *& File);
  virtual void ReadSymlink(TRemoteFile * SymlinkFile,
    TRemoteFile *& File);
  virtual void RenameFile(const std::wstring FileName,
    const std::wstring NewName);
  virtual void CopyFile(const std::wstring FileName,
    const std::wstring NewName);
  virtual std::wstring FileUrl(const std::wstring FileName);
  virtual TStrings * GetFixedPaths();
  virtual void SpaceAvailable(const std::wstring Path,
    TSpaceAvailable & ASpaceAvailable);
  virtual const TSessionInfo & GetSessionInfo();
  virtual const TFileSystemInfo & GetFileSystemInfo(bool Retrieve);
  virtual bool TemporaryTransferFile(const std::wstring & FileName);
  virtual bool GetStoredCredentialsTried();
  virtual std::wstring GetUserName();

protected:
  // __property TStrings * Output = { read = FOutput };
  TStrings *GetOutput() { return FOutput; };
  // __property int ReturnCode = { read = FReturnCode };
  int GetReturnCode() { return FReturnCode; }

  virtual std::wstring GetCurrentDirectory();

private:
  TSecureShell * FSecureShell;
  TCommandSet * FCommandSet;
  TFileSystemInfo FFileSystemInfo;
  std::wstring FCurrentDirectory;
  TStrings * FOutput;
  int FReturnCode;
  std::wstring FCachedDirectoryChange;
  bool FProcessingCommand;
  int FLsFullTime;
  TCaptureOutputEvent FOnCaptureOutput;

  void ClearAliases();
  void ClearAlias(std::wstring Alias);
  void CustomReadFile(const std::wstring FileName,
    TRemoteFile *& File, TRemoteFile * ALinkedByFile);
  static std::wstring DelimitStr(std::wstring Str);
  void DetectReturnVar();
  bool IsLastLine(std::wstring & Line);
  static bool IsTotalListingLine(const std::wstring Line);
  void EnsureLocation();
  void ExecCommand(const std::wstring & Cmd, int Params,
    const std::wstring & CmdString);
  void ExecCommand(TFSCommand Cmd, //FIXME const wchar_t TVarRec * args = NULL,
    int size = 0, int Params = -1);
  void ReadCommandOutput(int Params, const std::wstring * Cmd = NULL);
  void SCPResponse(bool * GotLastLine = NULL);
  void SCPDirectorySource(const std::wstring DirectoryName,
    const std::wstring TargetDir, const TCopyParamType * CopyParam, int Params,
    TFileOperationProgressType * OperationProgress, int Level);
  void SCPError(const std::wstring Message, bool Fatal);
  void SCPSendError(const std::wstring Message, bool Fatal);
  void SCPSink(const std::wstring TargetDir,
    const std::wstring FileName, const std::wstring SourceDir,
    const TCopyParamType * CopyParam, bool & Success,
    TFileOperationProgressType * OperationProgress, int Params, int Level);
  void SCPSource(const std::wstring FileName,
    const std::wstring TargetDir, const TCopyParamType * CopyParam, int Params,
    TFileOperationProgressType * OperationProgress, int Level);
  void SendCommand(const std::wstring Cmd);
  void SkipFirstLine();
  void SkipStartupMessage();
  void UnsetNationalVars();
  TRemoteFile * CreateRemoteFile(const std::wstring & ListingStr,
    TRemoteFile * LinkedByFile = NULL);
  void CaptureOutput(const std::wstring & AddedLine, bool StdError);
  void ChangeFileToken(const std::wstring & DelimitedName,
    const TRemoteToken & Token, TFSCommand Cmd, const std::wstring & RecursiveStr);

  static bool RemoveLastLine(std::wstring & Line,
    int & ReturnCode, std::wstring LastLine = L"");
};
//---------------------------------------------------------------------------
#endif // ScpFileSystemH
