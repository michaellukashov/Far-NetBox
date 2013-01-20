//---------------------------------------------------------------------------
#ifndef FtpFileSystemH
#define FtpFileSystemH

#ifndef NO_FILEZILLA
//---------------------------------------------------------------------------
#include <time.h>
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
//---------------------------------------------------------------------------
class TFTPFileSystem : public TCustomFileSystem
{
friend class TFileZillaImpl;
friend class TFTPFileListHelper;

public:
  explicit TFTPFileSystem(TTerminal * ATerminal);
  virtual ~TFTPFileSystem();

  virtual void Init(void *);
  virtual void FileTransferProgress(__int64 TransferSize, __int64 Bytes);

  virtual void Open();
  virtual void Close();
  virtual bool GetActive();
  virtual void Idle();
  virtual UnicodeString AbsolutePath(const UnicodeString & Path, bool Local);
  virtual void AnyCommand(const UnicodeString & Command,
    TCaptureOutputEvent OutputEvent);
  virtual void ChangeDirectory(const UnicodeString & Directory);
  virtual void CachedChangeDirectory(const UnicodeString & Directory);
  virtual void AnnounceFileListOperation();
  virtual void ChangeFileProperties(const UnicodeString & FileName,
    const TRemoteFile * File, const TRemoteProperties * Properties,
    TChmodSessionAction & Action);
  virtual bool LoadFilesProperties(TStrings * FileList);
  virtual void CalculateFilesChecksum(const UnicodeString & Alg,
    TStrings * FileList, TStrings * Checksums,
    TCalculatedChecksumEvent OnCalculatedChecksum);
  virtual void CopyToLocal(TStrings * FilesToCopy,
    const UnicodeString & TargetDir, const TCopyParamType * CopyParam,
    int Params, TFileOperationProgressType * OperationProgress,
    TOnceDoneOperation & OnceDoneOperation);
  virtual void CopyToRemote(TStrings * FilesToCopy,
    const UnicodeString & TargetDir, const TCopyParamType * CopyParam,
    int Params, TFileOperationProgressType * OperationProgress,
    TOnceDoneOperation & OnceDoneOperation);
  virtual void CreateDirectory(const UnicodeString & DirName);
  virtual void CreateLink(const UnicodeString & FileName, const UnicodeString & PointTo, bool Symbolic);
  virtual void DeleteFile(const UnicodeString & FileName,
    const TRemoteFile * File, int Params, TRmSessionAction & Action);
  virtual void CustomCommandOnFile(const UnicodeString & FileName,
    const TRemoteFile * File, const UnicodeString & Command, int Params, TCaptureOutputEvent OutputEvent);
  virtual void DoStartup();
  virtual void HomeDirectory();
  virtual bool IsCapable(int Capability) const;
  virtual void LookupUsersGroups();
  virtual void ReadCurrentDirectory();
  virtual void ReadDirectory(TRemoteFileList * FileList);
  virtual void ReadFile(const UnicodeString & FileName,
    TRemoteFile *& File);
  virtual void ReadSymlink(TRemoteFile * SymlinkFile,
    TRemoteFile *& File);
  virtual void RenameFile(const UnicodeString & FileName,
    const UnicodeString & NewName);
  virtual void CopyFile(const UnicodeString & FileName,
    const UnicodeString & NewName);
  virtual UnicodeString FileUrl(const UnicodeString & FileName);
  virtual TStrings * GetFixedPaths();
  virtual void SpaceAvailable(const UnicodeString & Path,
    TSpaceAvailable & ASpaceAvailable);
  virtual const TSessionInfo & GetSessionInfo();
  virtual const TFileSystemInfo & GetFileSystemInfo(bool Retrieve);
  virtual bool TemporaryTransferFile(const UnicodeString & FileName);
  virtual bool GetStoredCredentialsTried();
  virtual UnicodeString GetUserName();

protected:
  // enum TOverwriteMode { omOverwrite, omResume };

  virtual UnicodeString GetCurrentDirectory();

  const wchar_t * GetOption(int OptionID) const;
  int GetOptionVal(int OptionID) const;

  enum
  {
    REPLY_CONNECT =      0x01,
    REPLY_2XX_CODE =     0x02,
    REPLY_ALLOW_CANCEL = 0x04,
    REPLY_3XX_CODE =     0x08
  };

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
    UnicodeString Error = "", unsigned int * Code = NULL,
    TStrings ** Response = NULL);
  void ResetReply();
  void HandleReplyStatus(const UnicodeString & Response);
  void DoWaitForReply(unsigned int & ReplyToAwait, bool WantLastCode);
  bool KeepWaitingForReply(unsigned int & ReplyToAwait, bool WantLastCode) const;
  inline bool NoFinalLastCode() const;

  bool HandleStatus(const wchar_t * Status, int Type);
  bool HandleAsynchRequestOverwrite(
    wchar_t * FileName1, size_t FileName1Len, const wchar_t * FileName2,
    const wchar_t * Path1, const wchar_t * Path2,
    __int64 Size1, __int64 Size2, time_t Time1, time_t Time2,
    bool HasTime1, bool HasTime2, void * UserData, int & RequestResult);
  bool HandleAsynchRequestVerifyCertificate(
    const TFtpsCertificateData & Data, int & RequestResult);
  bool HandleAsynchRequestNeedPass(
    struct TNeedPassRequestData & Data, int & RequestResult) const;
  bool HandleListData(const wchar_t * Path, const TListDataEntry * Entries,
    unsigned int Count);
  bool HandleTransferStatus(bool Valid, __int64 TransferSize,
    __int64 Bytes, int Percent, int TimeElapsed, int TimeLeft, int TransferRate,
    bool FileTransfer);
  bool HandleReply(int Command, unsigned int Reply);
  bool HandleCapabilities(TFTPServerCapabilities * ServerCapabilities);
  bool CheckError(int ReturnCode, const wchar_t * Context);
  void EnsureLocation();
  UnicodeString ActualCurrentDirectory();
  void Discard();
  void DoChangeDirectory(const UnicodeString & Directory);

  void Sink(const UnicodeString & FileName,
    const TRemoteFile * File, const UnicodeString & TargetDir,
    const TCopyParamType * CopyParam, int Params,
    TFileOperationProgressType * OperationProgress, unsigned int Flags,
    TDownloadSessionAction & Action);
  void SinkRobust(const UnicodeString & FileName,
    const TRemoteFile * File, const UnicodeString & TargetDir,
    const TCopyParamType * CopyParam, int Params,
    TFileOperationProgressType * OperationProgress, unsigned int Flags);
  void SinkFile(const UnicodeString & FileName, const TRemoteFile * File, void * Param);
  void SourceRobust(const UnicodeString & FileName,
    const TRemoteFile * File,
    const UnicodeString & TargetDir, const TCopyParamType * CopyParam, int Params,
    TFileOperationProgressType * OperationProgress, unsigned int Flags);
  void Source(const UnicodeString & FileName,
    const TRemoteFile * File,
    const UnicodeString & TargetDir, const TCopyParamType * CopyParam, int Params,
    TOpenRemoteFileParams * OpenParams,
    TOverwriteFileParams * FileParams,
    TFileOperationProgressType * OperationProgress, unsigned int Flags,
    TUploadSessionAction & Action);
  void DirectorySource(const UnicodeString & DirectoryName,
    const UnicodeString & TargetDir, int Attrs, const TCopyParamType * CopyParam,
    int Params, TFileOperationProgressType * OperationProgress, unsigned int Flags);
  bool ConfirmOverwrite(UnicodeString & FileName,
    int Params, TFileOperationProgressType * OperationProgress,
    TOverwriteMode & OverwriteMode,
    bool AutoResume,
    const TOverwriteFileParams * FileParams);
  void ReadDirectoryProgress(__int64 Bytes);
  void ResetFileTransfer();
  void DoFileTransferProgress(__int64 TransferSize, __int64 Bytes);
  void ResetCaches();
  void CaptureOutput(const UnicodeString & Str);
  void DoReadDirectory(TRemoteFileList * FileList);
  void DoReadFile(const UnicodeString & FileName, TRemoteFile *& AFile);
  void FileTransfer(const UnicodeString & FileName, const UnicodeString & LocalFile,
    const UnicodeString & RemoteFile, const UnicodeString & RemotePath, bool Get,
    __int64 Size, int Type, TFileTransferData & UserData,
    TFileOperationProgressType * OperationProgress);
  TDateTime ConvertLocalTimestamp(time_t Time);
  void ConvertRemoteTimestamp(time_t Time, bool HasTime, TDateTime & DateTime, TModificationFmt & ModificationFmt);
  void SetLastCode(int Code);

  static bool Unquote(UnicodeString & Str);
  static UnicodeString ExtractStatusMessage(UnicodeString Status);

private:
  enum TCommand
  {
    CMD_UNKNOWN,
    PASS,
    SYST,
    FEAT
  };

  TFileZillaIntf * FFileZillaIntf;
  TCriticalSection * FQueueCriticalSection;
  TCriticalSection * FTransferStatusCriticalSection;
  TMessageQueue * FQueue;
  HANDLE FQueueEvent;
  TSessionInfo FSessionInfo;
  TFileSystemInfo FFileSystemInfo;
  bool FFileSystemInfoValid;
  unsigned int FReply;
  unsigned int FCommandReply;
  TCommand FLastCommand;
  bool FPasswordFailed;
  bool FMultineResponse;
  int FLastCode;
  int FLastCodeClass;
  int FLastReadDirectoryProgress;
  UnicodeString FTimeoutStatus;
  UnicodeString FDisconnectStatus;
  TStrings * FLastResponse;
  TStrings * FLastError;
  UnicodeString FSystem;
  TStrings * FFeatures;
  UnicodeString FCurrentDirectory;
  UnicodeString FHomeDirectory;
  TRemoteFileList * FFileList;
  TRemoteFileList * FFileListCache;
  UnicodeString FFileListCachePath;
  bool FActive;
  bool FOpening;
  bool FWaitingForReply;
  enum { ftaNone, ftaSkip, ftaCancel } FFileTransferAbort;
  bool FIgnoreFileList;
  bool FFileTransferCancelled;
  __int64 FFileTransferResumed;
  bool FFileTransferPreserveTime;
  unsigned long FFileTransferCPSLimit;
  bool FAwaitingProgress;
  TCaptureOutputEvent FOnCaptureOutput;
  UnicodeString FUserName;
  TAutoSwitch FListAll;
  bool FDoListAll;
  TFTPServerCapabilities * FServerCapabilities;
  TDateTime FLastDataSent;
  mutable UnicodeString FOptionScratch;

private:
  TFTPFileSystem(const TFTPFileSystem &);
  TFTPFileSystem & operator=(const TFTPFileSystem &);
};
//---------------------------------------------------------------------------
#endif NO_FILEZILLA
//---------------------------------------------------------------------------
#endif // FtpFileSystemH
