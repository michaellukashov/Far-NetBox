//---------------------------------------------------------------------------
#ifndef HttpFileSystemH
#define HttpFileSystemH

#include <FileSystems.h>
//---------------------------------------------------------------------------
class TCURLIntf;
class THTTPCommandSet;
// class TSecureShell;
//---------------------------------------------------------------------------
class THTTPFileSystem : public TCustomFileSystem
{
public:
  explicit THTTPFileSystem(TTerminal *ATerminal);
  virtual void Init(TSecureShell *SecureShell);
  virtual ~THTTPFileSystem();

  virtual void Open();
  virtual void Close();
  virtual bool GetActive();
  virtual void Idle();
  virtual std::wstring AbsolutePath(std::wstring Path, bool Local);
  virtual void AnyCommand(const std::wstring Command,
    const captureoutput_slot_type *OutputEvent);
  virtual void ChangeDirectory(const std::wstring Directory);
  virtual void CachedChangeDirectory(const std::wstring Directory);
  virtual void AnnounceFileListOperation();
  virtual void ChangeFileProperties(const std::wstring FileName,
    const TRemoteFile * File, const TRemoteProperties * Properties,
    TChmodSessionAction & Action);
  virtual bool LoadFilesProperties(TStrings * FileList);
  virtual void CalculateFilesChecksum(const std::wstring & Alg,
    TStrings * FileList, TStrings * Checksums,
    calculatedchecksum_slot_type *OnCalculatedChecksum);
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
    const TRemoteFile * File, std::wstring Command, int Params, const captureoutput_slot_type &OutputEvent);
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
  bool PostMessage(unsigned int Type, WPARAM wParam, LPARAM lParam);
  bool ProcessMessage();
  void DiscardMessages();
  void WaitForMessages();
  unsigned int WaitForReply(bool Command, bool WantLastCode);
  unsigned int WaitForCommandReply(bool WantLastCode = true);
  void WaitForFatalNonCommandReply();
  void PoolForFatalNonCommandReply();
  void GotNonCommandReply(unsigned int Reply);
  void GotReply(unsigned int Reply, unsigned int Flags = 0,
    std::wstring Error = L"", unsigned int * Code = NULL,
    TStrings ** Response = NULL);
  void ResetReply();
  void HandleReplyStatus(std::wstring Response);
  void DoWaitForReply(unsigned int& ReplyToAwait, bool WantLastCode);
  bool KeepWaitingForReply(unsigned int& ReplyToAwait, bool WantLastCode);
  inline bool NoFinalLastCode();

  bool HandleStatus(const wchar_t * Status, int Type);
  bool HandleAsynchRequestOverwrite(
    wchar_t * FileName1, size_t FileName1Len, const wchar_t * FileName2,
    const wchar_t * Path1, const wchar_t * Path2,
    __int64 Size1, __int64 Size2, time_t Time1, time_t Time2,
    bool HasTime1, bool HasTime2, void * UserData, int & RequestResult);
  bool HandleAsynchRequestVerifyCertificate(
    const TFtpsCertificateData & Data, int & RequestResult);
  bool HandleListData(const wchar_t * Path, const TListDataEntry * Entries,
    unsigned int Count);
  bool HandleTransferStatus(bool Valid, __int64 TransferSize,
    __int64 Bytes, int Percent, int TimeElapsed, int TimeLeft, int TransferRate,
    bool FileTransfer);
  bool HandleReply(int Command, unsigned int Reply);
  bool HandleCapabilities(bool Mfmt);
  bool CheckError(int ReturnCode, const wchar_t * Context);
  void EnsureLocation();
  std::wstring ActualCurrentDirectory();
  void Discard();
  void DoChangeDirectory(const std::wstring & Directory);

  void Sink(const std::wstring FileName,
    const TRemoteFile * File, const std::wstring TargetDir,
    const TCopyParamType * CopyParam, int Params,
    TFileOperationProgressType * OperationProgress, unsigned int Flags,
    TDownloadSessionAction & Action);
  void SinkRobust(const std::wstring FileName,
    const TRemoteFile * File, const std::wstring TargetDir,
    const TCopyParamType * CopyParam, int Params,
    TFileOperationProgressType * OperationProgress, unsigned int Flags);
  void SinkFile(std::wstring FileName, const TRemoteFile * File, void * Param);
  void SourceRobust(const std::wstring FileName,
    const std::wstring TargetDir, const TCopyParamType * CopyParam, int Params,
    TFileOperationProgressType * OperationProgress, unsigned int Flags);
  void Source(const std::wstring FileName,
    const std::wstring TargetDir, const TCopyParamType * CopyParam, int Params,
    TFileOperationProgressType * OperationProgress, unsigned int Flags,
    TUploadSessionAction & Action);
  void DirectorySource(const std::wstring DirectoryName,
    const std::wstring TargetDir, int Attrs, const TCopyParamType * CopyParam,
    int Params, TFileOperationProgressType * OperationProgress, unsigned int Flags);
  bool ConfirmOverwrite(std::wstring & FileName,
    TOverwriteMode & OverwriteMode, TFileOperationProgressType * OperationProgress,
    const TOverwriteFileParams * FileParams, int Params, bool AutoResume);
  void ReadDirectoryProgress(__int64 Bytes);
  void ResetFileTransfer();
  void DoFileTransferProgress(__int64 TransferSize, __int64 Bytes);
  void FileTransferProgress(__int64 TransferSize, __int64 Bytes);
  void ResetCaches();
  void CaptureOutput(const std::wstring & Str);
  void DoReadDirectory(TRemoteFileList * FileList);
  void FileTransfer(const std::wstring & FileName, const std::wstring & LocalFile,
    const std::wstring & RemoteFile, const std::wstring & RemotePath, bool Get,
    __int64 Size, int Type, TFileTransferData & UserData,
    TFileOperationProgressType * OperationProgress);
  TDateTime ConvertLocalTimestamp(time_t Time);
  TDateTime ConvertRemoteTimestamp(time_t Time, bool HasTime);
  void SetLastCode(int Code);

  static bool Unquote(std::wstring & Str);
  static std::wstring ExtractStatusMessage(std::wstring Status);

protected:
  // __property TStrings * Output = { read = FOutput };
  TStrings *GetOutput() { return FOutput; };
  // __property int ReturnCode = { read = FReturnCode };
  int GetReturnCode() { return FReturnCode; }

  virtual std::wstring GetCurrentDirectory();

private:
  // TSecureShell * FSecureShell;
  THTTPCommandSet * FCommandSet;
  TFileSystemInfo FFileSystemInfo;
  std::wstring FCurrentDirectory;
  std::wstring FHomeDirectory;
  TStrings * FOutput;
  int FReturnCode;
  std::wstring FCachedDirectoryChange;
  bool FProcessingCommand;
  int FLsFullTime;
  captureoutput_signal_type FOnCaptureOutput;
  TSessionInfo FSessionInfo;
  std::wstring FUserName;
  TDateTime FLastDataSent;
  TCURLIntf * FCURLIntf;
  bool FPasswordFailed;
  std::wstring FSystem;
  bool FActive;
  THTTPFileSystem *Self;

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
  void ExecCommand(TFSCommand Cmd, int Params = -1, ...);
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
private:
  enum
  {
    REPLY_CONNECT =      0x01,
    REPLY_2XX_CODE =     0x02,
    REPLY_ALLOW_CANCEL = 0x04
  };
private:
  THTTPFileSystem(const THTTPFileSystem &);
  void operator=(const THTTPFileSystem &);
};
//---------------------------------------------------------------------------
#endif // ScpFileSystemH
