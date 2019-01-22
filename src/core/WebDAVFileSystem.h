//------------------------------------------------------------------------------
#pragma once
//------------------------------------------------------------------------------
#include <ne_uri.h>
#include <ne_utils.h>
#include <ne_string.h>
#include <ne_request.h>
#include <FileSystems.h>
//------------------------------------------------------------------------------
struct TNeonCertificateData;
struct ne_ssl_certificate_s;
struct ne_session_s;
struct ne_prop_result_set_s;
struct ne_lock_store_s;
struct TOverwriteFileParams;
struct ssl_st;
struct ne_lock;
//------------------------------------------------------------------------------
NB_DEFINE_CLASS_ID(TWebDAVFileSystem);
class TWebDAVFileSystem final : public TCustomFileSystem
{
  NB_DISABLE_COPY(TWebDAVFileSystem)
public:
  static bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TWebDAVFileSystem); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TWebDAVFileSystem) || TCustomFileSystem::is(Kind); }
public:
  explicit TWebDAVFileSystem(TTerminal *ATerminal) noexcept;
  virtual ~TWebDAVFileSystem() noexcept;
  void Init(void *) override;

  void Open() override;
  void Close() override;
  bool GetActive() const override;
  void CollectUsage() override;
  void Idle() override;
  UnicodeString GetAbsolutePath(UnicodeString APath, bool Local) override;
  void AnyCommand(UnicodeString ACommand,
    TCaptureOutputEvent OutputEvent) override;
  void ChangeDirectory(UnicodeString ADirectory) override;
  void CachedChangeDirectory(UnicodeString ADirectory) override;
  void AnnounceFileListOperation() override;
  void ChangeFileProperties(UnicodeString AFileName,
    const TRemoteFile *AFile, const TRemoteProperties *Properties,
    TChmodSessionAction &Action) override;
  bool LoadFilesProperties(TStrings *AFileList) override;
  void CalculateFilesChecksum(UnicodeString Alg,
    TStrings *AFileList, TStrings *Checksums,
    TCalculatedChecksumEvent OnCalculatedChecksum) override;
  void CopyToLocal(TStrings *AFilesToCopy,
    UnicodeString TargetDir, const TCopyParamType *CopyParam,
    intptr_t AParams, TFileOperationProgressType *OperationProgress,
    TOnceDoneOperation &OnceDoneOperation) override;
  void CopyToRemote(TStrings *AFilesToCopy,
    UnicodeString ATargetDir, const TCopyParamType *CopyParam,
    intptr_t AParams, TFileOperationProgressType *OperationProgress,
    TOnceDoneOperation &OnceDoneOperation) override;
  void Source(
    TLocalFileHandle &AHandle, UnicodeString ATargetDir, UnicodeString &ADestFileName,
    const TCopyParamType *CopyParam, intptr_t AParams,
    TFileOperationProgressType *OperationProgress, uintptr_t AFlags,
    TUploadSessionAction &Action, bool &ChildError) override;
  void RemoteCreateDirectory(UnicodeString ADirName, bool Encrypt) override;
  void RemoteCreateLink(UnicodeString AFileName, UnicodeString APointTo, bool Symbolic) override;
  void RemoteDeleteFile(UnicodeString AFileName,
    const TRemoteFile *AFile, intptr_t Params, TRmSessionAction &Action) override;
  void CustomCommandOnFile(UnicodeString AFileName,
    const TRemoteFile *AFile, UnicodeString ACommand, intptr_t AParams, TCaptureOutputEvent OutputEvent) override;
  void DoStartup() override;
  void HomeDirectory() override;
  bool IsCapable(intptr_t Capability) const override;
  void LookupUsersGroups() override;
  void ReadCurrentDirectory() override;
  void ReadDirectory(TRemoteFileList *AFileList) override;
  void ReadFile(UnicodeString AFileName,
    TRemoteFile *&AFile) override;
  void ReadSymlink(TRemoteFile *SymlinkFile,
    TRemoteFile *&AFile) override;
  void RemoteRenameFile(UnicodeString AFileName, const TRemoteFile *AFile,
    UnicodeString ANewName) override;
  void RemoteCopyFile(UnicodeString AFileName, const TRemoteFile *AFile,
    UnicodeString ANewName) override;
  TStrings * GetFixedPaths() const override;
  void SpaceAvailable(UnicodeString APath,
    TSpaceAvailable &ASpaceAvailable) override;
  const TSessionInfo & GetSessionInfo() const override;
  const TFileSystemInfo & GetFileSystemInfo(bool Retrieve) override;
  bool TemporaryTransferFile(UnicodeString AFileName) override;
  bool GetStoredCredentialsTried() const override;
  UnicodeString RemoteGetUserName() const override;
  void GetSupportedChecksumAlgs(TStrings *Algs) override;
  void LockFile(UnicodeString AFileName, const TRemoteFile *AFile) override;
  void UnlockFile(UnicodeString AFileName, const TRemoteFile *AFile) override;
  void UpdateFromMain(TCustomFileSystem *AMainFileSystem) override;
  void ClearCaches() override;

  UnicodeString GetAbsolutePath(UnicodeString APath, bool Local) const override;
  void FileTransferProgress(int64_t TransferSize, int64_t Bytes) override;
  void NeonDebug(UnicodeString AMessage);

protected:
  UnicodeString RemoteGetCurrentDirectory() const override;

  void Sink(
    UnicodeString AFileName, const TRemoteFile *AFile,
    UnicodeString ATargetDir, UnicodeString &ADestFileName, intptr_t Attrs,
    const TCopyParamType *CopyParam, intptr_t AParams, TFileOperationProgressType *OperationProgress,
    uintptr_t AFlags, TDownloadSessionAction &Action) override;
  void ConfirmOverwrite(
    UnicodeString ASourceFullFileName, UnicodeString &ATargetFileName,
    TFileOperationProgressType *OperationProgress,
    const TOverwriteFileParams *FileParams, const TCopyParamType *CopyParam,
    intptr_t Params);
  void CheckStatus(intptr_t NeonStatus);
  void ClearNeonError();
  static void NeonPropsResult(
    void *UserData, const ne_uri *Uri, const ne_prop_result_set_s *Results);
  void ParsePropResultSet(TRemoteFile *AFile,
    UnicodeString APath, const ne_prop_result_set_s *Results);
  void TryOpenDirectory(UnicodeString ADirectory);
  static int NeonBodyReader(void *UserData, const char *Buf, size_t Len);
  static void NeonPreSend(ne_request *Request, void *UserData, ne_buffer *Header);
  static int NeonBodyAccepter(void *UserData, ne_request *Request, const ne_status *Status);
  static void NeonCreateRequest(ne_request *Request, void *UserData, const char *Method, const char *Uri);
  static int NeonRequestAuth(void *UserData, const char *Realm, int Attempt, char *UserName, char *Password);
  void NeonOpen(UnicodeString &CorrectedUrl, UnicodeString AUrl);
  void NeonClientOpenSessionInternal(UnicodeString &CorrectedUrl, UnicodeString Url);
  static void NeonNotifier(void *UserData, ne_session_status Status, const ne_session_status_info *StatusInfo);
  static ssize_t NeonUploadBodyProvider(void *UserData, char *Buffer, size_t BufLen);
  static int NeonPostSend(ne_request *Req, void *UserData, const ne_status *Status);
  static void NeonPostHeaders(ne_request *Req, void *UserData, const ne_status *Status);
  void ExchangeCapabilities(const char *APath, UnicodeString &CorrectedUrl);
  static int DoNeonServerSSLCallback(void *UserData, int Failures, const struct ne_ssl_certificate_s *Certificate, bool Aux);
  static int NeonServerSSLCallbackMain(void *UserData, int Failures, const struct ne_ssl_certificate_s *Certificate);
  static int NeonServerSSLCallbackAux(void *UserData, int Failures, const struct ne_ssl_certificate_s *Certificate);
  static void NeonProvideClientCert(void *UserData, ne_session *Sess, const ne_ssl_dname *const *DNames, int DNCount);
  void CloseNeonSession();
  bool CancelTransfer();
  UnicodeString GetNeonError() const;
  static void NeonQuotaResult(void *UserData, const ne_uri *Uri, const ne_prop_result_set_s *Results);
  static const char * GetNeonProp(const ne_prop_result_set_s *Results,
    const char *Name, const char *NameSpace = nullptr);
  static void LockResult(void *UserData, const struct ne_lock *Lock,
   const ne_uri *Uri, const ne_status *Status);
  void RequireLockStore();
  static void InitSslSession(ssl_st *Ssl, ne_session *Session);
  void NeonAddAuthentication(bool UseNegotiate);
  void HttpAuthenticationFailed();

  void InitSslSessionImpl(ssl_st *Ssl) const;
private:
  TFileSystemInfo FFileSystemInfo{};
  UnicodeString FCurrentDirectory;
  UnicodeString FCachedDirectoryChange;
  TSessionInfo FSessionInfo;
  UnicodeString FUserName;
  bool FActive{false};
  bool FHasTrailingSlash{false};
  bool FSkipped{false};
  bool FCancelled{false};
  bool FStoredPasswordTried{false};
  bool FUploading{false};
  bool FDownloading{false};
  UnicodeString FUploadMimeType;
  ne_session_s *FNeonSession{nullptr};
  ne_lock_store_s *FNeonLockStore{nullptr};
  TCriticalSection FNeonLockStoreSection;
  bool FInitialHandshake{false};
  bool FAuthenticationRequested{false};
  UnicodeString FResponse;
  RawByteString FPassword;
  UnicodeString FTlsVersionStr;
  uint32_t FCapabilities{0};
  UnicodeString FHostName;
  intptr_t FPortNumber{0};
  enum TIgnoreAuthenticationFailure { iafNo, iafWaiting, iafPasswordFailed } FIgnoreAuthenticationFailure{iafNo};
  UnicodeString FAuthorizationProtocol;
  UnicodeString FLastAuthorizationProtocol;
  bool FAuthenticationRetry{false};
  bool FNtlmAuthenticationFailed{false};

  void CustomReadFile(UnicodeString AFileName,
    TRemoteFile *& AFile, TRemoteFile *ALinkedByFile);
  intptr_t CustomReadFileInternal(UnicodeString AFileName,
    TRemoteFile *& AFile, TRemoteFile *ALinkedByFile);
  bool VerifyCertificate(TNeonCertificateData &Data, bool Aux);
  void OpenUrl(UnicodeString Url);
  void CollectTLSSessionInfo();
  UnicodeString GetRedirectUrl() const;
  UnicodeString ParsePathFromUrl(UnicodeString Url) const;
  int ReadDirectoryInternal(UnicodeString APath, TRemoteFileList *AFileList);
  int RenameFileInternal(UnicodeString AFileName, UnicodeString ANewName);
  int CopyFileInternal(UnicodeString AFileName, UnicodeString ANewName);
  bool IsValidRedirect(intptr_t NeonStatus, UnicodeString &APath) const;
  UnicodeString DirectoryPath(UnicodeString APath) const;
  UnicodeString FilePath(const TRemoteFile *AFile) const;
  struct ne_lock * FindLock(const RawByteString APath) const;
  void DiscardLock(const RawByteString APath);
  bool IsNtlmAuthentication() const;
  static void NeonAuxRequestInit(ne_session_s *Session, ne_request *Request, void *UserData);
  void SetSessionTls(ne_session_s *Session, bool Aux);
  void InitSession(ne_session_s *Session);
};
//------------------------------------------------------------------------------
