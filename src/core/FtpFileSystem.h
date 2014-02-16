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
struct TRemoteFileTime;
//---------------------------------------------------------------------------
class TFTPFileSystem : public TCustomFileSystem
{
friend class TFileZillaImpl;
friend class TFTPFileListHelper;
NB_DISABLE_COPY(TFTPFileSystem)
public:
  explicit TFTPFileSystem(TTerminal * ATerminal);
  virtual ~TFTPFileSystem();

  virtual void Init(void *);
  virtual void FileTransferProgress(__int64 TransferSize, __int64 Bytes);

  virtual void Open();
  virtual void Close();
  virtual bool GetActive() const;
  virtual void CollectUsage();
  virtual void Idle();
  virtual UnicodeString AbsolutePath(const UnicodeString & Path, bool Local);
  virtual void AnyCommand(const UnicodeString & Command,
    TCaptureOutputEvent OutputEvent);
  virtual void ChangeDirectory(const UnicodeString & Directory);
  virtual void CachedChangeDirectory(const UnicodeString & Directory);
  virtual void AnnounceFileListOperation();
  virtual void ChangeFileProperties(const UnicodeString & FileName,
    const TRemoteFile * AFile, const TRemoteProperties * Properties,
    TChmodSessionAction & Action);
  virtual bool LoadFilesProperties(TStrings * FileList);
  virtual void CalculateFilesChecksum(const UnicodeString & Alg,
    TStrings * FileList, TStrings * Checksums,
    TCalculatedChecksumEvent OnCalculatedChecksum);
  virtual void CopyToLocal(TStrings * AFilesToCopy,
    const UnicodeString & TargetDir, const TCopyParamType * CopyParam,
    intptr_t Params, TFileOperationProgressType * OperationProgress,
    TOnceDoneOperation & OnceDoneOperation);
  virtual void CopyToRemote(TStrings * AFilesToCopy,
    const UnicodeString & TargetDir, const TCopyParamType * CopyParam,
    intptr_t Params, TFileOperationProgressType * OperationProgress,
    TOnceDoneOperation & OnceDoneOperation);
  virtual void CreateDirectory(const UnicodeString & DirName);
  virtual void CreateLink(const UnicodeString & FileName, const UnicodeString & PointTo, bool Symbolic);
  virtual void DeleteFile(const UnicodeString & FileName,
    const TRemoteFile * AFile, intptr_t Params, TRmSessionAction & Action);
  virtual void CustomCommandOnFile(const UnicodeString & FileName,
    const TRemoteFile * AFile, const UnicodeString & Command, intptr_t Params,
    TCaptureOutputEvent OutputEvent);
  virtual void DoStartup();
  virtual void HomeDirectory();
  virtual bool IsCapable(intptr_t Capability) const;
  virtual void LookupUsersGroups();
  virtual void ReadCurrentDirectory();
  virtual void ReadDirectory(TRemoteFileList * FileList);
  virtual void ReadFile(const UnicodeString & FileName,
    TRemoteFile *& AFile);
  virtual void ReadSymlink(TRemoteFile * SymlinkFile,
    TRemoteFile *& AFile);
  virtual void RenameFile(const UnicodeString & FileName,
    const UnicodeString & NewName);
  virtual void CopyFile(const UnicodeString & FileName,
    const UnicodeString & NewName);
  virtual UnicodeString FileUrl(const UnicodeString & FileName) const;
  virtual TStrings * GetFixedPaths();
  virtual void SpaceAvailable(const UnicodeString & Path,
    TSpaceAvailable & ASpaceAvailable);
  virtual const TSessionInfo & GetSessionInfo() const;
  virtual const TFileSystemInfo & GetFileSystemInfo(bool Retrieve);
  virtual bool TemporaryTransferFile(const UnicodeString & FileName);
  virtual bool GetStoredCredentialsTried();
  virtual UnicodeString GetUserName();

protected:
  // enum TOverwriteMode { omOverwrite, omResume, omComplete };

  virtual UnicodeString GetCurrentDirectory();

  const wchar_t * GetOption(intptr_t OptionID) const;
  intptr_t GetOptionVal(intptr_t OptionID) const;

  enum
  {
    REPLY_CONNECT =      0x01,
    REPLY_2XX_CODE =     0x02,
    REPLY_ALLOW_CANCEL = 0x04,
    REPLY_3XX_CODE =     0x08
  };

  bool PostMessage(uintptr_t Type, WPARAM wParam, LPARAM lParam);
  bool ProcessMessage();
  void DiscardMessages();
  void WaitForMessages();
  uintptr_t WaitForReply(bool Command, bool WantLastCode);
  uintptr_t WaitForCommandReply(bool WantLastCode = true);
  void WaitForFatalNonCommandReply();
  void PoolForFatalNonCommandReply();
  void GotNonCommandReply(uintptr_t Reply);
  void GotReply(uintptr_t Reply, uintptr_t Flags = 0,
    const UnicodeString & Error = L"", uintptr_t * Code = nullptr,
    TStrings ** Response = nullptr);
  void ResetReply();
  void HandleReplyStatus(const UnicodeString & Response);
  void DoWaitForReply(uintptr_t &ReplyToAwait, bool WantLastCode);
  bool KeepWaitingForReply(uintptr_t &ReplyToAwait, bool WantLastCode) const;
  inline bool NoFinalLastCode() const;

  bool HandleStatus(const wchar_t * Status, int Type);
  bool HandleAsynchRequestOverwrite(
    wchar_t * FileName1, size_t FileName1Len, const wchar_t * FileName2,
    const wchar_t * Path1, const wchar_t * Path2,
    __int64 Size1, __int64 Size2, time_t LocalTime,
    bool HasLocalTime, const TRemoteFileTime & RemoteTime, void * UserData, int & RequestResult);
  bool HandleAsynchRequestVerifyCertificate(
    const TFtpsCertificateData & Data, int & RequestResult);
  bool HandleAsynchRequestNeedPass(
    struct TNeedPassRequestData & Data, int & RequestResult) const;
  bool HandleListData(const wchar_t * Path, const TListDataEntry * Entries,
    uintptr_t Count);
  bool HandleTransferStatus(bool Valid, __int64 TransferSize,
    __int64 Bytes, intptr_t Percent, intptr_t TimeElapsed, intptr_t TimeLeft, intptr_t TransferRate,
    bool FileTransfer);
  bool HandleReply(intptr_t Command, uintptr_t Reply);
  bool HandleCapabilities(TFTPServerCapabilities * ServerCapabilities);
  bool CheckError(intptr_t ReturnCode, const wchar_t * Context);
  void PreserveDownloadFileTime(HANDLE Handle, void * UserData);
  bool GetFileModificationTimeInUtc(const wchar_t * FileName, struct tm & Time);
  void EnsureLocation();
  UnicodeString ActualCurrentDirectory();
  void Discard();
  void DoChangeDirectory(const UnicodeString & Directory);

  void Sink(const UnicodeString & FileName,
    const TRemoteFile * AFile, const UnicodeString & TargetDir,
    const TCopyParamType * CopyParam, intptr_t Params,
    TFileOperationProgressType * OperationProgress, uintptr_t Flags,
    TDownloadSessionAction & Action);
  void SinkRobust(const UnicodeString & FileName,
    const TRemoteFile * AFile, const UnicodeString & TargetDir,
    const TCopyParamType * CopyParam, intptr_t Params,
    TFileOperationProgressType * OperationProgress, uintptr_t Flags);
  void SinkFile(const UnicodeString & FileName, const TRemoteFile * AFile, void * Param);
  void SourceRobust(const UnicodeString & FileName,
    const TRemoteFile * AFile,
    const UnicodeString & TargetDir, const TCopyParamType * CopyParam, intptr_t Params,
    TFileOperationProgressType * OperationProgress, uintptr_t Flags);
  void Source(const UnicodeString & FileName,
    const TRemoteFile * AFile,
    const UnicodeString & TargetDir, const TCopyParamType * CopyParam, intptr_t Params,
    TOpenRemoteFileParams * OpenParams,
    TOverwriteFileParams * FileParams,
    TFileOperationProgressType * OperationProgress, uintptr_t Flags,
    TUploadSessionAction & Action);
  void DirectorySource(const UnicodeString & DirectoryName,
    const UnicodeString & TargetDir, intptr_t Attrs, const TCopyParamType * CopyParam,
    intptr_t Params, TFileOperationProgressType * OperationProgress, uintptr_t Flags);
  bool ConfirmOverwrite(UnicodeString & FileName,
    intptr_t Params, TFileOperationProgressType * OperationProgress,
    TOverwriteMode & OverwriteMode,
    bool AutoResume,
    const TOverwriteFileParams * FileParams,
    const TCopyParamType * CopyParam);
  void ReadDirectoryProgress(__int64 Bytes);
  void ResetFileTransfer();
  void DoFileTransferProgress(__int64 TransferSize, __int64 Bytes);
  void ResetCaches();
  void CaptureOutput(const UnicodeString & Str);
  void DoReadDirectory(TRemoteFileList * FileList);
  void DoReadFile(const UnicodeString & FileName, TRemoteFile *& AFile);
  void FileTransfer(const UnicodeString & FileName, const UnicodeString & LocalFile, HANDLE Handle,
    const UnicodeString & RemoteFile, const UnicodeString & RemotePath, bool Get,
    __int64 Size, intptr_t Type, TFileTransferData & UserData,
    TFileOperationProgressType * OperationProgress);
  TDateTime ConvertLocalTimestamp(time_t Time);
  void RemoteFileTimeToDateTimeAndPrecision(const TRemoteFileTime & Source,
    TDateTime & DateTime, TModificationFmt & ModificationFmt);
  void SetLastCode(intptr_t Code);
  void StoreLastResponse(const UnicodeString & Text);

  static bool Unquote(UnicodeString & Str);
  static UnicodeString ExtractStatusMessage(const UnicodeString & Status);

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
  uintptr_t FReply;
  uintptr_t FCommandReply;
  TCommand FLastCommand;
  bool FPasswordFailed;
  bool FMultineResponse;
  intptr_t FLastCode;
  intptr_t FLastCodeClass;
  intptr_t FLastReadDirectoryProgress;
  UnicodeString FTimeoutStatus;
  UnicodeString FDisconnectStatus;
  TStrings * FLastResponse;
  TStrings * FLastErrorResponse;
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
  bool FFileTransferRemoveBOM;
  uintptr_t FFileTransferCPSLimit;
  bool FAwaitingProgress;
  TCaptureOutputEvent FOnCaptureOutput;
  UnicodeString FUserName;
  TAutoSwitch FListAll;
  bool FDoListAll;
  TFTPServerCapabilities * FServerCapabilities;
  TDateTime FLastDataSent;
  mutable UnicodeString FOptionScratch;
};
//---------------------------------------------------------------------------
#endif // NO_FILEZILLA
//---------------------------------------------------------------------------
#endif // FtpFileSystemH
