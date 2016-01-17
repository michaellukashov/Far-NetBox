
#pragma once

#include <apr_pools.h>
#include <neon/src/ne_uri.h>
#include <neon/src/ne_utils.h>
#include <neon/src/ne_string.h>
#include <neon/src/ne_request.h>
#include <FileSystems.h>

struct TWebDAVCertificateData;
struct ne_ssl_certificate_s;
struct ne_session_s;
struct ne_prop_result_set_s;
struct ne_lock_store_s;
struct TOverwriteFileParams;
struct ssl_st;
struct ne_lock;
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
  virtual bool GetActive() const;
  virtual void CollectUsage();
  virtual void Idle();
  virtual UnicodeString GetAbsolutePath(const UnicodeString & APath, bool Local);
  virtual UnicodeString GetAbsolutePath(const UnicodeString & APath, bool Local) const;
  virtual void AnyCommand(const UnicodeString & Command,
    TCaptureOutputEvent OutputEvent);
  virtual void ChangeDirectory(const UnicodeString & Directory);
  virtual void CachedChangeDirectory(const UnicodeString & Directory);
  virtual void AnnounceFileListOperation();
  virtual void ChangeFileProperties(const UnicodeString & AFileName,
    const TRemoteFile * AFile, const TRemoteProperties * Properties,
    TChmodSessionAction & Action);
  virtual bool LoadFilesProperties(TStrings * AFileList);
  virtual void CalculateFilesChecksum(const UnicodeString & Alg,
    TStrings * AFileList, TStrings * Checksums,
    TCalculatedChecksumEvent OnCalculatedChecksum);
  virtual void CopyToLocal(const TStrings * AFilesToCopy,
    const UnicodeString & TargetDir, const TCopyParamType * CopyParam,
    intptr_t Params, TFileOperationProgressType * OperationProgress,
    TOnceDoneOperation & OnceDoneOperation);
  virtual void CopyToRemote(const TStrings * AFilesToCopy,
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
    const UnicodeString & ANewName);
  virtual void RemoteCopyFile(const UnicodeString & AFileName,
    const UnicodeString & ANewName);
  virtual TStrings * GetFixedPaths();
  virtual void SpaceAvailable(const UnicodeString & APath,
    TSpaceAvailable & ASpaceAvailable);
  virtual const TSessionInfo & GetSessionInfo() const;
  virtual const TFileSystemInfo & GetFileSystemInfo(bool Retrieve);
  virtual bool TemporaryTransferFile(const UnicodeString & AFileName);
  virtual bool GetStoredCredentialsTried() const;
  virtual UnicodeString FSGetUserName() const;
  virtual void GetSupportedChecksumAlgs(TStrings * Algs);
  virtual void LockFile(const UnicodeString & AFileName, const TRemoteFile * AFile);
  virtual void UnlockFile(const UnicodeString & AFileName, const TRemoteFile * AFile);
  virtual void UpdateFromMain(TCustomFileSystem * MainFileSystem);

  void NeonDebug(const UnicodeString & Message);

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
  virtual UnicodeString GetCurrDirectory() const;

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
    TUploadSessionAction & Action, bool & ChildError);
  void WebDAVDirectorySource(const UnicodeString & DirectoryName,
    const UnicodeString & TargetDir, uintptr_t Attrs, const TCopyParamType * CopyParam,
    intptr_t Params, TFileOperationProgressType * OperationProgress, uintptr_t Flags);
  bool ConfirmOverwrite(
    const UnicodeString & ASourceFullFileName, UnicodeString & ADestFileName,
    TFileOperationProgressType * OperationProgress,
    const TOverwriteFileParams * FileParams,
    const TCopyParamType * CopyParam, intptr_t Params,
    bool AutoResume,
    OUT TOverwriteMode & OverwriteMode,
    OUT uintptr_t & Answer);
  void CheckStatus(int NeonStatus);
  void ClearNeonError();
  static void NeonPropsResult(
    void * UserData, const ne_uri * Uri, const ne_prop_result_set_s * Results);
  void ParsePropResultSet(TRemoteFile * AFile,
    const UnicodeString & APath, const ne_prop_result_set_s * Results);
  void TryOpenDirectory(const UnicodeString & ADirectory);
  static int NeonBodyReader(void * UserData, const char * Buf, size_t Len);
  static void NeonPreSend(ne_request * Request, void * UserData, ne_buffer * Header);
  static int NeonBodyAccepter(void * UserData, ne_request * Request, const ne_status * Status);
  static void NeonCreateRequest(ne_request * Request, void * UserData, const char * Method, const char * Uri);
  static int NeonRequestAuth(void * UserData, const char * Realm, int Attempt, char * UserName, char * Password);
  void NeonOpen(UnicodeString & CorrectedUrl, const UnicodeString & Url);
  void NeonClientOpenSessionInternal(UnicodeString & CorrectedUrl, UnicodeString Url);
  static void NeonNotifier(void * UserData, ne_session_status Status, const ne_session_status_info * StatusInfo);
  static ssize_t NeonUploadBodyProvider(void * UserData, char * Buffer, size_t BufLen);
  static int NeonPostSend(ne_request * Req, void * UserData, const ne_status * Status);
  void ExchangeCapabilities(const char * Path, UnicodeString & CorrectedUrl);
  static int NeonServerSSLCallback(void * UserData, int Failures, const struct ne_ssl_certificate_s * Certificate);
  static void NeonProvideClientCert(void * UserData, ne_session * Sess, const ne_ssl_dname * const * DNames, int DNCount);
  void CloseNeonSession();
  bool CancelTransfer();
  UnicodeString GetNeonError() const;
  static void NeonQuotaResult(void * UserData, const ne_uri * Uri, const ne_prop_result_set_s * Results);
  static const char * GetProp(const ne_prop_result_set_s * Results,
    const char * Name, const char * NameSpace = NULL);
  static void LockResult(void * UserData, const struct ne_lock * Lock,
   const ne_uri * Uri, const ne_status * Status);
  void RequireLockStore();
  static void InitSslSession(ssl_st * Ssl, ne_session * Session);
  void InitSslSessionImpl(ssl_st * Ssl);
  void ResetFileTransfer();
  void DoFileTransferProgress(int64_t TransferSize, int64_t Bytes);
  void DoReadDirectory(TRemoteFileList * FileList);
  void FileTransfer(const UnicodeString & AFileName, const UnicodeString & LocalFile,
    const UnicodeString & RemoteFile, const UnicodeString & RemotePath, bool Get,
    int64_t Size, int Type, TFileTransferData & UserData,
    TFileOperationProgressType * OperationProgress);

private:
//  void CustomReadFile(const UnicodeString & AFileName,
//    TRemoteFile *& File, TRemoteFile * ALinkedByFile);
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
  bool FActive;
  bool FHasTrailingSlash;
  bool FCancelled;
  bool FStoredPasswordTried;
  bool FUploading;
  bool FDownloading;
  ne_session_s * FNeonSession;
  ne_lock_store_s * FNeonLockStore;
  TCriticalSection FNeonLockStoreSection;
  bool FInitialHandshake;
  bool FAuthenticationRequested;
  UnicodeString FResponse;
  RawByteString FPassword;
  UnicodeString FTlsVersionStr;
  unsigned int FCapabilities;
  UnicodeString FHostName;
  int FPortNumber;
  enum TIgnoreAuthenticationFailure { iafNo, iafWaiting, iafPasswordFailed } FIgnoreAuthenticationFailure;

  void CustomReadFile(const UnicodeString & AFileName,
    TRemoteFile *& File, TRemoteFile * ALinkedByFile);
  int CustomReadFileInternal(const UnicodeString & AFileName,
    TRemoteFile *& File, TRemoteFile * ALinkedByFile);
  void RegisterForDebug();
  void UnregisterFromDebug();
  bool VerifyCertificate(
    const TWebDAVCertificateData & Data);
  void OpenUrl(const UnicodeString & Url);
  void CollectTLSSessionInfo();
  UnicodeString GetRedirectUrl();
  UnicodeString ParsePathFromUrl(const UnicodeString & Url);
  int ReadDirectoryInternal(const UnicodeString & Path, TRemoteFileList * FileList);
  int RenameFileInternal(const UnicodeString & AFileName, const UnicodeString & ANewName);
  bool IsValidRedirect(int NeonStatus, UnicodeString & Path);
  UnicodeString DirectoryPath(UnicodeString Path);
  UnicodeString FilePath(const TRemoteFile * File);
  struct ne_lock * FindLock(const RawByteString & Path);
  void DiscardLock(const RawByteString & Path);

//  bool FStoredPasswordTried;
  bool FPasswordFailed;
//  bool FActive;
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
//  bool FHasTrailingSlash;
  size_t FFileTransferCPSLimit;
  size_t FLastReadDirectoryProgress;
  TFileOperationProgressType * FCurrentOperationProgress;
  TCriticalSection FTransferStatusCriticalSection;
  apr_pool_t * webdav_pool;
  webdav::session_t * FSession;
};

void NeonInitialize();
void NeonFinalize();

