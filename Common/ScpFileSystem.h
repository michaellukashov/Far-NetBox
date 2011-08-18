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
  virtual wstring AbsolutePath(wstring Path, bool Local);
  virtual void AnyCommand(const wstring Command,
    TCaptureOutputEvent OutputEvent);
  virtual void ChangeDirectory(const wstring Directory);
  virtual void CachedChangeDirectory(const wstring Directory);
  virtual void AnnounceFileListOperation();
  virtual void ChangeFileProperties(const wstring FileName,
    const TRemoteFile * File, const TRemoteProperties * Properties,
    TChmodSessionAction & Action);
  virtual bool LoadFilesProperties(TStrings * FileList);
  virtual void CalculateFilesChecksum(const wstring & Alg,
    TStrings * FileList, TStrings * Checksums,
    TCalculatedChecksumEvent OnCalculatedChecksum);
  virtual void CopyToLocal(TStrings * FilesToCopy,
    const wstring TargetDir, const TCopyParamType * CopyParam,
    int Params, TFileOperationProgressType * OperationProgress,
    TOnceDoneOperation & OnceDoneOperation);
  virtual void CopyToRemote(TStrings * FilesToCopy,
    const wstring TargetDir, const TCopyParamType * CopyParam,
    int Params, TFileOperationProgressType * OperationProgress,
    TOnceDoneOperation & OnceDoneOperation);
  virtual void CreateDirectory(const wstring DirName);
  virtual void CreateLink(const wstring FileName, const wstring PointTo, bool Symbolic);
  virtual void DeleteFile(const wstring FileName,
    const TRemoteFile * File, int Params, TRmSessionAction & Action);
  virtual void CustomCommandOnFile(const wstring FileName,
    const TRemoteFile * File, wstring Command, int Params, TCaptureOutputEvent OutputEvent);
  virtual void DoStartup();
  virtual void HomeDirectory();
  virtual bool IsCapable(int Capability) const;
  virtual void LookupUsersGroups();
  virtual void ReadCurrentDirectory();
  virtual void ReadDirectory(TRemoteFileList * FileList);
  virtual void ReadFile(const wstring FileName,
    TRemoteFile *& File);
  virtual void ReadSymlink(TRemoteFile * SymlinkFile,
    TRemoteFile *& File);
  virtual void RenameFile(const wstring FileName,
    const wstring NewName);
  virtual void CopyFile(const wstring FileName,
    const wstring NewName);
  virtual wstring FileUrl(const wstring FileName);
  virtual TStrings * GetFixedPaths();
  virtual void SpaceAvailable(const wstring Path,
    TSpaceAvailable & ASpaceAvailable);
  virtual const TSessionInfo & GetSessionInfo();
  virtual const TFileSystemInfo & GetFileSystemInfo(bool Retrieve);
  virtual bool TemporaryTransferFile(const wstring & FileName);
  virtual bool GetStoredCredentialsTried();
  virtual wstring GetUserName();

protected:
  // __property TStrings * Output = { read = FOutput };
  TStrings *GetOutput() { return FOutput; };
  // __property int ReturnCode = { read = FReturnCode };
  int GetReturnCode() { return FReturnCode; }

  virtual wstring GetCurrentDirectory();

private:
  TSecureShell * FSecureShell;
  TCommandSet * FCommandSet;
  TFileSystemInfo FFileSystemInfo;
  wstring FCurrentDirectory;
  TStrings * FOutput;
  int FReturnCode;
  wstring FCachedDirectoryChange;
  bool FProcessingCommand;
  int FLsFullTime;
  TCaptureOutputEvent FOnCaptureOutput;

  void ClearAliases();
  void ClearAlias(wstring Alias);
  void CustomReadFile(const wstring FileName,
    TRemoteFile *& File, TRemoteFile * ALinkedByFile);
  static wstring DelimitStr(wstring Str);
  void DetectReturnVar();
  bool IsLastLine(wstring & Line);
  static bool IsTotalListingLine(const wstring Line);
  void EnsureLocation();
  void ExecCommand(const wstring & Cmd, int Params,
    const wstring & CmdString);
  void ExecCommand(TFSCommand Cmd, //FIXME const wchar_t TVarRec * args = NULL,
    int size = 0, int Params = -1);
  void ReadCommandOutput(int Params, const wstring * Cmd = NULL);
  void SCPResponse(bool * GotLastLine = NULL);
  void SCPDirectorySource(const wstring DirectoryName,
    const wstring TargetDir, const TCopyParamType * CopyParam, int Params,
    TFileOperationProgressType * OperationProgress, int Level);
  void SCPError(const wstring Message, bool Fatal);
  void SCPSendError(const wstring Message, bool Fatal);
  void SCPSink(const wstring TargetDir,
    const wstring FileName, const wstring SourceDir,
    const TCopyParamType * CopyParam, bool & Success,
    TFileOperationProgressType * OperationProgress, int Params, int Level);
  void SCPSource(const wstring FileName,
    const wstring TargetDir, const TCopyParamType * CopyParam, int Params,
    TFileOperationProgressType * OperationProgress, int Level);
  void SendCommand(const wstring Cmd);
  void SkipFirstLine();
  void SkipStartupMessage();
  void UnsetNationalVars();
  TRemoteFile * CreateRemoteFile(const wstring & ListingStr,
    TRemoteFile * LinkedByFile = NULL);
  void CaptureOutput(const wstring & AddedLine, bool StdError);
  void ChangeFileToken(const wstring & DelimitedName,
    const TRemoteToken & Token, TFSCommand Cmd, const wstring & RecursiveStr);

  static bool RemoveLastLine(wstring & Line,
    int & ReturnCode, wstring LastLine = L"");
};
//---------------------------------------------------------------------------
#endif // ScpFileSystemH
