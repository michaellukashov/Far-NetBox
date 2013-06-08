#ifndef WebDavFileSystemH
#define WebDavFileSystemH

#include <apr_pools.h>

#include <FileSystems.h>
#include "Terminal.h"

//------------------------------------------------------------------------------
struct TListDataEntry;
struct TFileTransferData;
//------------------------------------------------------------------------------
namespace webdav {
  struct session_t;
  typedef int error_t;
} // namespace webdav
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
class TWebDAVFileSystem : public TCustomFileSystem
{
  friend class TWebDAVFileListHelper;

public:
  explicit TWebDAVFileSystem(TTerminal * ATerminal);
  virtual ~TWebDAVFileSystem();

  virtual void Init(void *);
  virtual void FileTransferProgress(__int64 TransferSize, __int64 Bytes);

  virtual void Open();
  virtual void Close();
  virtual bool GetActive() const {  return FActive; }
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
    intptr_t Params, TFileOperationProgressType * OperationProgress,
    TOnceDoneOperation & OnceDoneOperation);
  virtual void CopyToRemote(TStrings * FilesToCopy,
    const UnicodeString & TargetDir, const TCopyParamType * CopyParam,
    intptr_t Params, TFileOperationProgressType * OperationProgress,
    TOnceDoneOperation & OnceDoneOperation);
  virtual void CreateDirectory(const UnicodeString & DirName);
  virtual void CreateLink(const UnicodeString & FileName, const UnicodeString & PointTo, bool Symbolic);
  virtual void DeleteFile(const UnicodeString & FileName,
    const TRemoteFile * File, intptr_t Params, TRmSessionAction & Action);
  virtual void CustomCommandOnFile(const UnicodeString & FileName,
    const TRemoteFile * File, const UnicodeString & Command, intptr_t Params, TCaptureOutputEvent OutputEvent);
  virtual void DoStartup();
  virtual void HomeDirectory();
  virtual bool IsCapable(intptr_t Capability) const;
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

public:
  virtual void ReadDirectoryProgress(__int64 Bytes);

public:
  webdav::error_t GetServerSettings(
    int * proxy_method,
    const char **proxy_host,
    unsigned int *proxy_port,
    const char **proxy_username,
    const char **proxy_password,
    int *timeout_seconds,
    int *neon_debug,
    const char **neon_debug_file_name,
    bool *compression,
    const char **pk11_provider,
    const char **ssl_authority_file,
    apr_pool_t *pool);
  webdav::error_t VerifyCertificate(
    const char * Prompt, const char *fingerprint,
    uintptr_t & RequestResult);
  webdav::error_t AskForClientCertificateFilename(
    const char **cert_file, uintptr_t & RequestResult,
    apr_pool_t *pool);
  webdav::error_t AskForUsername(
    const char **user_name,
    uintptr_t & RequestResult,
    apr_pool_t *pool);
  webdav::error_t AskForUserPassword(
    const char **password,
    uintptr_t & RequestResult,
    apr_pool_t *pool);
  webdav::error_t AskForPassphrase(
    const char **passphrase,
    const char *realm,
    uintptr_t & RequestResult,
    apr_pool_t *pool);
  webdav::error_t SimplePrompt(
    const char *prompt_text,
    const char *prompt_string,
    uintptr_t & RequestResult);
  webdav::error_t CreateStorage(THierarchicalStorage *& Storage);
  uintptr_t AdjustToCPSLimit(uintptr_t Len);
  bool GetIsCancelled();

protected:
  virtual UnicodeString GetCurrentDirectory();

  bool HandleListData(const wchar_t * Path, const TListDataEntry * Entries,
    intptr_t Count);
  void EnsureLocation();
  void DoChangeDirectory(const UnicodeString & Directory);

  void Sink(const UnicodeString & FileName,
    const TRemoteFile * File, const UnicodeString & TargetDir,
    const TCopyParamType * CopyParam, intptr_t Params,
    TFileOperationProgressType * OperationProgress, uintptr_t Flags,
    TDownloadSessionAction & Action);
  void SinkRobust(const UnicodeString & FileName,
    const TRemoteFile * File, const UnicodeString & TargetDir,
    const TCopyParamType * CopyParam, intptr_t Params,
    TFileOperationProgressType * OperationProgress, uintptr_t Flags);
  void SinkFile(const UnicodeString & FileName, const TRemoteFile * File, void * Param);
  void WebDAVSourceRobust(const UnicodeString & FileName,
    const TRemoteFile * File,
    const UnicodeString & TargetDir, const TCopyParamType * CopyParam, intptr_t Params,
    TFileOperationProgressType * OperationProgress, uintptr_t Flags);
  void WebDAVSource(const UnicodeString & FileName,
    const TRemoteFile * File,
    const UnicodeString & TargetDir, const TCopyParamType * CopyParam, intptr_t Params,
    TFileOperationProgressType * OperationProgress, uintptr_t Flags,
    TUploadSessionAction & Action);
  void WebDAVDirectorySource(const UnicodeString & DirectoryName,
    const UnicodeString & TargetDir, uintptr_t Attrs, const TCopyParamType * CopyParam,
    intptr_t Params, TFileOperationProgressType * OperationProgress, uintptr_t Flags);
  bool ConfirmOverwrite(UnicodeString & FileName,
    TOverwriteMode & OverwriteMode, TFileOperationProgressType * OperationProgress,
    const TOverwriteFileParams * FileParams,
    const TCopyParamType * CopyParam, intptr_t Params,
    bool AutoResume,
    uintptr_t & Answer);
  void ResetFileTransfer();
  void DoFileTransferProgress(__int64 TransferSize, __int64 Bytes);
  void DoReadDirectory(TRemoteFileList * FileList);
  void FileTransfer(const UnicodeString & FileName, const UnicodeString & LocalFile,
    const UnicodeString & RemoteFile, const UnicodeString & RemotePath, bool Get,
    __int64 Size, int Type, TFileTransferData & UserData,
    TFileOperationProgressType * OperationProgress);

private:
  void CustomReadFile(const UnicodeString & FileName,
    TRemoteFile *& File, TRemoteFile * ALinkedByFile);
  bool SendPropFindRequest(const wchar_t * dir, int & responseCode);
  webdav::error_t OpenURL(const UnicodeString & repos_URL,
    apr_pool_t *pool);

  bool WebDAVCheckExisting(const wchar_t * path, int & is_dir);
  bool WebDAVMakeDirectory(const wchar_t * path);
  bool WebDAVGetList(const UnicodeString & Directory);
  bool WebDAVGetFile(const wchar_t * remotePath, HANDLE * LocalFileHandle);
  bool WebDAVPutFile(const wchar_t * remotePath, const wchar_t * localPath, const unsigned __int64 fileSize);
  bool WebDAVRenameFile(const wchar_t * srcPath, const wchar_t * dstPath);
  bool WebDAVDeleteFile(const wchar_t * path);

private:
  TFileSystemInfo FFileSystemInfo;
  UnicodeString FCurrentDirectory;
  TRemoteFileList * FFileList;
  UnicodeString FCachedDirectoryChange;
  TCaptureOutputEvent FOnCaptureOutput;
  TSessionInfo FSessionInfo;
  UnicodeString FUserName;
  bool FPasswordFailed;
  bool FActive;
  enum { ftaNone, ftaSkip, ftaCancel } FFileTransferAbort;
  bool FIgnoreFileList;
  bool FFileTransferCancelled;
  __int64 FFileTransferResumed;
  bool FFileTransferPreserveTime;
  bool FHasTrailingSlash;
  size_t FFileTransferCPSLimit;
  size_t FLastReadDirectoryProgress;
  TFileOperationProgressType * FCurrentOperationProgress;
  TCriticalSection * FTransferStatusCriticalSection;
  apr_pool_t *webdav_pool;
  webdav::session_t *FSession;

private:
  NB_DISABLE_COPY(TWebDAVFileSystem)
};

#endif
