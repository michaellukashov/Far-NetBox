
#pragma once


#include <ne_uri.h>
#include <ne_utils.h>
#include <ne_string.h>
#include <ne_request.h>
#include <FileSystems.h>

struct TWebDAVCertificateData;
struct ne_ssl_certificate_s;
struct ne_session_s;
struct ne_prop_result_set_s;
struct ne_lock_store_s;
struct TOverwriteFileParams;
struct ssl_st;
struct ne_lock;

class TWebDAVFileSystem : public TCustomFileSystem
{
NB_DISABLE_COPY(TWebDAVFileSystem)
public:
  static inline bool classof(const TObject * Obj) { return Obj->is(OBJECT_CLASS_TWebDAVFileSystem); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TWebDAVFileSystem) || TCustomFileSystem::is(Kind); }
public:
  explicit TWebDAVFileSystem(TTerminal * ATerminal);
  virtual ~TWebDAVFileSystem();

  virtual void Init(void *) override;
  virtual void FileTransferProgress(int64_t TransferSize, int64_t Bytes) override;

  virtual void Open();
  virtual void Close();
  virtual bool GetActive() const override;
  virtual void CollectUsage();
  virtual void Idle();
  virtual UnicodeString GetAbsolutePath(UnicodeString APath, bool Local) override;
  virtual UnicodeString GetAbsolutePath(UnicodeString APath, bool Local) const override;
  virtual void AnyCommand(UnicodeString Command,
    TCaptureOutputEvent OutputEvent) override;
  virtual void ChangeDirectory(UnicodeString ADirectory) override;
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
    UnicodeString ATargetDir, const TCopyParamType * CopyParam,
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
  virtual void ReadDirectory(TRemoteFileList * AFileList) override;
  virtual void ReadFile(UnicodeString AFileName,
    TRemoteFile *& AFile) override;
  virtual void ReadSymlink(TRemoteFile * SymlinkFile,
    TRemoteFile *& AFile) override;
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
  virtual void UpdateFromMain(TCustomFileSystem * AMainFileSystem) override;

  void NeonDebug(UnicodeString Message);

protected:
  virtual UnicodeString RemoteGetCurrentDirectory() const override;

  void Sink(UnicodeString AFileName,
    const TRemoteFile * AFile, UnicodeString TargetDir,
    const TCopyParamType * CopyParam, intptr_t AParams,
    TFileOperationProgressType * OperationProgress, uintptr_t Flags,
    TDownloadSessionAction & Action, bool & ChildError);
  void SinkRobust(UnicodeString AFileName,
    const TRemoteFile * AFile, UnicodeString TargetDir,
    const TCopyParamType * CopyParam, intptr_t Params,
    TFileOperationProgressType * OperationProgress, uintptr_t Flags);
  void SinkFile(UnicodeString AFileName, const TRemoteFile * AFile, void * Param);
  void SourceRobust(UnicodeString AFileName,
    const TRemoteFile * AFile,
    UnicodeString TargetDir, const TCopyParamType * CopyParam, intptr_t Params,
    TFileOperationProgressType * OperationProgress, uintptr_t Flags);
  void Source(UnicodeString AFileName,
    const TRemoteFile * AFile,
    UnicodeString TargetDir, const TCopyParamType * CopyParam, intptr_t Params,
    TFileOperationProgressType * OperationProgress, uintptr_t Flags,
    TUploadSessionAction & Action, bool & ChildError);
  void DirectorySource(UnicodeString DirectoryName,
    UnicodeString TargetDir, uintptr_t Attrs, const TCopyParamType * CopyParam,
    intptr_t Params, TFileOperationProgressType * OperationProgress, uintptr_t Flags);
  void ConfirmOverwrite(
    UnicodeString ASourceFullFileName, UnicodeString & ATargetFileName,
    TFileOperationProgressType * OperationProgress,
    const TOverwriteFileParams * FileParams, const TCopyParamType * CopyParam,
    intptr_t Params,
    OUT TOverwriteMode & OverwriteMode,
    OUT uintptr_t & Answer);
  void CheckStatus(intptr_t NeonStatus);
  void ClearNeonError();
  static void NeonPropsResult(
    void * UserData, const ne_uri * Uri, const ne_prop_result_set_s * Results);
  void ParsePropResultSet(TRemoteFile * AFile,
    UnicodeString APath, const ne_prop_result_set_s * Results);
  void TryOpenDirectory(UnicodeString ADirectory);
  static int NeonBodyReader(void * UserData, const char * Buf, size_t Len);
  static void NeonPreSend(ne_request * Request, void * UserData, ne_buffer * Header);
  static int NeonBodyAccepter(void * UserData, ne_request * Request, const ne_status * Status);
  static void NeonCreateRequest(ne_request * Request, void * UserData, const char * Method, const char * Uri);
  static int NeonRequestAuth(void * UserData, const char * Realm, int Attempt, char * UserName, char * Password);
  void NeonOpen(UnicodeString & CorrectedUrl, UnicodeString Url);
  void NeonClientOpenSessionInternal(UnicodeString & CorrectedUrl, UnicodeString Url);
  static void NeonNotifier(void * UserData, ne_session_status Status, const ne_session_status_info * StatusInfo);
  static ssize_t NeonUploadBodyProvider(void * UserData, char * Buffer, size_t BufLen);
  static int NeonPostSend(ne_request * Req, void * UserData, const ne_status * Status);
  static void NeonPostHeaders(ne_request * Req, void * UserData, const ne_status * Status);
  void ExchangeCapabilities(const char * APath, UnicodeString & CorrectedUrl);
  static int DoNeonServerSSLCallback(void * UserData, int Failures, const struct ne_ssl_certificate_s * Certificate, bool Aux);
  static int NeonServerSSLCallbackMain(void * UserData, int Failures, const struct ne_ssl_certificate_s * Certificate);
  static int NeonServerSSLCallbackAux(void * UserData, int Failures, const struct ne_ssl_certificate_s * Certificate);
  static void NeonProvideClientCert(void * UserData, ne_session * Sess, const ne_ssl_dname * const * DNames, int DNCount);
  void CloseNeonSession();
  bool CancelTransfer();
  UnicodeString GetNeonError() const;
  static void NeonQuotaResult(void * UserData, const ne_uri * Uri, const ne_prop_result_set_s * Results);
  static const char * GetNeonProp(const ne_prop_result_set_s * Results,
    const char * Name, const char * NameSpace = nullptr);
  static void LockResult(void * UserData, const struct ne_lock * Lock,
   const ne_uri * Uri, const ne_status * Status);
  void RequireLockStore();
  static void InitSslSession(ssl_st * Ssl, ne_session * Session);
  void InitSslSessionImpl(ssl_st * Ssl) const;
  void NeonAddAuthentication(bool UseNegotiate);
  void HttpAuthenticationFailed();

private:
  TFileSystemInfo FFileSystemInfo;
  UnicodeString FCurrentDirectory;
  UnicodeString FCachedDirectoryChange;
  TSessionInfo FSessionInfo;
  UnicodeString FUserName;
  bool FActive;
  bool FHasTrailingSlash;
  bool FSkipped;
  bool FCancelled;
  bool FStoredPasswordTried;
  bool FUploading;
  bool FDownloading;
  UnicodeString FUploadMimeType;
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
  UnicodeString FAuthorizationProtocol;
  UnicodeString FLastAuthorizationProtocol;
  bool FAuthenticationRetry;
  bool FNtlmAuthenticationFailed;

  void CustomReadFile(UnicodeString AFileName,
    TRemoteFile *& AFile, TRemoteFile * ALinkedByFile);
  intptr_t CustomReadFileInternal(UnicodeString AFileName,
    TRemoteFile *& AFile, TRemoteFile * ALinkedByFile);
  void RegisterForDebug();
  void UnregisterFromDebug();
  bool VerifyCertificate(const TWebDAVCertificateData & Data, bool Aux);
  void OpenUrl(UnicodeString Url);
  void CollectTLSSessionInfo();
  UnicodeString GetRedirectUrl() const;
  UnicodeString ParsePathFromUrl(UnicodeString Url) const;
  int ReadDirectoryInternal(UnicodeString APath, TRemoteFileList * AFileList);
  int RenameFileInternal(UnicodeString AFileName, UnicodeString ANewName);
  int CopyFileInternal(UnicodeString AFileName, UnicodeString ANewName);
  bool IsValidRedirect(intptr_t NeonStatus, UnicodeString & APath) const;
  UnicodeString DirectoryPath(UnicodeString APath) const;
  UnicodeString FilePath(const TRemoteFile * AFile) const;
  struct ne_lock * FindLock(const RawByteString & APath) const;
  void DiscardLock(const RawByteString & APath);
  bool IsNtlmAuthentication() const;
  static void NeonAuxRequestInit(ne_session_s * Session, ne_request * Request, void * UserData);
  void SetSessionTls(ne_session_s * Session, bool Aux);
  void InitSession(ne_session_s * Session);
  TFtps GetFtps() const;
};

void NeonInitialize();
void NeonFinalize();

