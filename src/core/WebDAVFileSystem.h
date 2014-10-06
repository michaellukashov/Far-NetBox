#pragma once

#include <apr_pools.h>

#include <FileSystems.h>
#include "Terminal.h"

struct TListDataEntry;
struct TFileTransferData;

namespace webdav {
  struct session_t;
  typedef int error_t;
} // namespace webdav


class TWebDAVFileSystem : public TCustomFileSystem
{
friend class TWebDAVFileListHelper;
NB_DISABLE_COPY(TWebDAVFileSystem)
NB_DECLARE_CLASS(TWebDAVFileSystem)
public:
  explicit TWebDAVFileSystem(TTerminal * ATerminal);
  virtual ~TWebDAVFileSystem();

  virtual void Init(void *);
  virtual void FileTransferProgress(int64_t TransferSize, int64_t Bytes);

  virtual void Open();
  virtual void Close();
  virtual bool GetActive() const {  return FActive; }
  virtual void CollectUsage();
  virtual void Idle();
  virtual UnicodeString AbsolutePath(const UnicodeString & APath, bool Local);
  virtual void AnyCommand(const UnicodeString & Command,
    TCaptureOutputEvent OutputEvent);
  virtual void ChangeDirectory(const UnicodeString & Directory);
  virtual void CachedChangeDirectory(const UnicodeString & Directory);
  virtual void AnnounceFileListOperation();
  virtual void ChangeFileProperties(const UnicodeString & AFileName,
    const TRemoteFile * AFile, const TRemoteProperties * Properties,
    TChmodSessionAction & Action);
  virtual bool LoadFilesProperties(Classes::TStrings * FileList);
  virtual void CalculateFilesChecksum(const UnicodeString & Alg,
    Classes::TStrings * FileList, Classes::TStrings * Checksums,
    TCalculatedChecksumEvent OnCalculatedChecksum);
  virtual void CopyToLocal(const Classes::TStrings * AFilesToCopy,
    const UnicodeString & TargetDir, const TCopyParamType * CopyParam,
    intptr_t Params, TFileOperationProgressType * OperationProgress,
    TOnceDoneOperation & OnceDoneOperation);
  virtual void CopyToRemote(const Classes::TStrings * AFilesToCopy,
    const UnicodeString & TargetDir, const TCopyParamType * CopyParam,
    intptr_t Params, TFileOperationProgressType * OperationProgress,
    TOnceDoneOperation & OnceDoneOperation);
  virtual void RemoteCreateDirectory(const UnicodeString & ADirName);
  virtual void CreateLink(const UnicodeString & AFileName, const UnicodeString & PointTo, bool Symbolic);
  virtual void RemoteDeleteFile(const UnicodeString & AFileName,
    const TRemoteFile * AFile, intptr_t Params, TRmSessionAction & Action);
  virtual void CustomCommandOnFile(const UnicodeString & AFileName,
    const TRemoteFile * AFile, const UnicodeString & Command, intptr_t Params, TCaptureOutputEvent OutputEvent);
  virtual void DoStartup();
  virtual void HomeDirectory();
  virtual bool IsCapable(intptr_t Capability) const;
  virtual void LookupUsersGroups();
  virtual void ReadCurrentDirectory();
  virtual void ReadDirectory(TRemoteFileList * FileList);
  virtual void ReadFile(const UnicodeString & AFileName,
    TRemoteFile *& File);
  virtual void ReadSymlink(TRemoteFile * SymlinkFile,
    TRemoteFile *& File);
  virtual void RemoteRenameFile(const UnicodeString & AFileName,
    const UnicodeString & NewName);
  virtual void CopyFile(const UnicodeString & AFileName,
    const UnicodeString & NewName);
  virtual Classes::TStrings * GetFixedPaths();
  virtual void SpaceAvailable(const UnicodeString & APath,
    TSpaceAvailable & ASpaceAvailable);
  virtual const TSessionInfo & GetSessionInfo() const;
  virtual const TFileSystemInfo & GetFileSystemInfo(bool Retrieve);
  virtual bool TemporaryTransferFile(const UnicodeString & AFileName);
  virtual bool GetStoredCredentialsTried();
  virtual UnicodeString GetUserName();

public:
  virtual void ReadDirectoryProgress(int64_t Bytes);

public:
  webdav::error_t GetServerSettings(
    int * proxy_method,
    const char **proxy_host,
    uint32_t *proxy_port,
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
  webdav::error_t NeonRequestAuth(
    const char ** user_name,
    const char ** password,
    uintptr_t & RequestResult,
    apr_pool_t *pool);
  webdav::error_t AskForUsername(
    const char ** user_name,
    uintptr_t & RequestResult,
    apr_pool_t *pool);
  webdav::error_t AskForUserPassword(
    const char ** password,
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

protected:
  virtual UnicodeString GetCurrDirectory();

  bool HandleListData(const wchar_t * Path, const TListDataEntry * Entries,
    intptr_t Count);
  void EnsureLocation();
  void DoChangeDirectory(const UnicodeString & Directory);

  void Sink(const UnicodeString & AFileName,
    const TRemoteFile * AFile, const UnicodeString & TargetDir,
    const TCopyParamType * CopyParam, intptr_t Params,
    TFileOperationProgressType * OperationProgress, uintptr_t Flags,
    TDownloadSessionAction & Action);
  void SinkRobust(const UnicodeString & AFileName,
    const TRemoteFile * AFile, const UnicodeString & TargetDir,
    const TCopyParamType * CopyParam, intptr_t Params,
    TFileOperationProgressType * OperationProgress, uintptr_t Flags);
  void SinkFile(const UnicodeString & AFileName, const TRemoteFile * AFile, void * Param);
  void WebDAVSourceRobust(const UnicodeString & AFileName,
    const TRemoteFile * AFile,
    const UnicodeString & TargetDir, const TCopyParamType * CopyParam, intptr_t Params,
    TFileOperationProgressType * OperationProgress, uintptr_t Flags);
  void WebDAVSource(const UnicodeString & AFileName,
    const TRemoteFile * AFile,
    const UnicodeString & TargetDir, const TCopyParamType * CopyParam, intptr_t Params,
    TFileOperationProgressType * OperationProgress, uintptr_t Flags,
    TUploadSessionAction & Action);
  void WebDAVDirectorySource(const UnicodeString & DirectoryName,
    const UnicodeString & TargetDir, uintptr_t Attrs, const TCopyParamType * CopyParam,
    intptr_t Params, TFileOperationProgressType * OperationProgress, uintptr_t Flags);
  bool ConfirmOverwrite(const UnicodeString & AFullFileName, UnicodeString & AFileName,
    TFileOperationProgressType * OperationProgress,
    const TOverwriteFileParams * FileParams,
    const TCopyParamType * CopyParam, intptr_t Params,
    bool AutoResume,
    OUT TOverwriteMode & OverwriteMode,
    OUT uintptr_t & Answer);
  void ResetFileTransfer();
  void DoFileTransferProgress(int64_t TransferSize, int64_t Bytes);
  void DoReadDirectory(TRemoteFileList * FileList);
  void FileTransfer(const UnicodeString & AFileName, const UnicodeString & LocalFile,
    const UnicodeString & RemoteFile, const UnicodeString & RemotePath, bool Get,
    int64_t Size, int Type, TFileTransferData & UserData,
    TFileOperationProgressType * OperationProgress);

private:
  void CustomReadFile(const UnicodeString & AFileName,
    TRemoteFile *& File, TRemoteFile * ALinkedByFile);
  bool SendPropFindRequest(const wchar_t * Path, int & ResponseCode);
  webdav::error_t OpenURL(const UnicodeString & SessionURL,
    apr_pool_t * pool);

  bool WebDAVCheckExisting(const wchar_t * Path, int & IsDir);
  bool WebDAVMakeDirectory(const wchar_t * Path);
  bool WebDAVGetList(const UnicodeString & Directory);
  bool WebDAVGetFile(const wchar_t * RemotePath, const wchar_t * LocalPath);
  bool WebDAVPutFile(const wchar_t * RemotePath, const wchar_t * LocalPath, const uint64_t FileSize);
  bool WebDAVRenameFile(const wchar_t * SrcPath, const wchar_t * DstPath);
  bool WebDAVDeleteFile(const wchar_t * Path);

private:
  TFileSystemInfo FFileSystemInfo;
  UnicodeString FCurrentDirectory;
  TRemoteFileList * FFileList;
  UnicodeString FCachedDirectoryChange;
  TCaptureOutputEvent FOnCaptureOutput;
  TSessionInfo FSessionInfo;
  UnicodeString FUserName;
  UnicodeString FPassword;
  enum TIgnoreAuthenticationFailure { iafNo, iafWaiting, iafPasswordFailed } FIgnoreAuthenticationFailure;
  bool FStoredPasswordTried;
  bool FPasswordFailed;
  bool FActive;
  enum
  {
    ftaNone,
    ftaSkip,
    ftaCancel
  } FFileTransferAbort;
  bool FIgnoreFileList;
  bool FFileTransferCancelled;
  int64_t FFileTransferResumed;
  bool FFileTransferPreserveTime;
  bool FHasTrailingSlash;
  size_t FFileTransferCPSLimit;
  size_t FLastReadDirectoryProgress;
  TFileOperationProgressType * FCurrentOperationProgress;
  TCriticalSection FTransferStatusCriticalSection;
  apr_pool_t * webdav_pool;
  webdav::session_t * FSession;
};


void NeonInitialize();
void NeonFinalize();

