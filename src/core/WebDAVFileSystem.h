
#pragma once
//---------------------------------------------------------------------------
#include <ne_uri.h>
#include <ne_utils.h>
#include <ne_string.h>
#include <ne_request.h>
#include <FileSystems.h>
//---------------------------------------------------------------------------
struct TNeonCertificateData;
struct ne_ssl_certificate_s;
struct ne_session_s;
struct ne_prop_result_set_s;
struct ne_lock_store_s;
struct TOverwriteFileParams;
struct ssl_st;
struct ne_lock;
//---------------------------------------------------------------------------
class TWebDAVFileSystem : public TCustomFileSystem
{
  NB_DISABLE_COPY(TWebDAVFileSystem)
public:
  static inline bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TWebDAVFileSystem); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TWebDAVFileSystem) || TCustomFileSystem::is(Kind); }
public:
  explicit TWebDAVFileSystem(TTerminal *ATerminal);
  virtual ~TWebDAVFileSystem();
  virtual void Init(void *) override;

  virtual void Open() override;
  virtual void Close() override;
  virtual bool GetActive() const override;
  virtual void CollectUsage() override;
  virtual void Idle() override;
  virtual UnicodeString GetAbsolutePath(const UnicodeString APath, bool Local) override;
  virtual UnicodeString GetAbsolutePath(const UnicodeString APath, bool Local) const override;
  virtual void AnyCommand(const UnicodeString ACommand,
    TCaptureOutputEvent OutputEvent) override;
  virtual void ChangeDirectory(const UnicodeString ADirectory) override;
  virtual void CachedChangeDirectory(const UnicodeString ADirectory) override;
  virtual void AnnounceFileListOperation() override;
  virtual void ChangeFileProperties(const UnicodeString AFileName,
    const TRemoteFile *AFile, const TRemoteProperties *Properties,
    TChmodSessionAction &Action) override;
  virtual bool LoadFilesProperties(TStrings *AFileList) override;
  virtual void CalculateFilesChecksum(const UnicodeString Alg,
    TStrings *AFileList, TStrings *Checksums,
    TCalculatedChecksumEvent OnCalculatedChecksum) override;
  virtual void CopyToLocal(TStrings *AFilesToCopy,
    const UnicodeString TargetDir, const TCopyParamType *CopyParam,
    intptr_t AParams, TFileOperationProgressType *OperationProgress,
    TOnceDoneOperation &OnceDoneOperation) override;
  virtual void CopyToRemote(TStrings *AFilesToCopy,
    const UnicodeString ATargetDir, const TCopyParamType *CopyParam,
    intptr_t AParams, TFileOperationProgressType *OperationProgress,
    TOnceDoneOperation &OnceDoneOperation) override;
  virtual void Source(
    TLocalFileHandle &AHandle, const UnicodeString ATargetDir, UnicodeString &ADestFileName,
    const TCopyParamType *CopyParam, intptr_t AParams,
    TFileOperationProgressType *OperationProgress, uintptr_t AFlags,
    TUploadSessionAction &Action, bool &ChildError);
  virtual void RemoteCreateDirectory(const UnicodeString ADirName) override;
  virtual void RemoteCreateLink(const UnicodeString AFileName, const UnicodeString APointTo, bool Symbolic) override;
  virtual void RemoteDeleteFile(const UnicodeString AFileName,
    const TRemoteFile *AFile, intptr_t Params, TRmSessionAction &Action) override;
  virtual void CustomCommandOnFile(const UnicodeString AFileName,
    const TRemoteFile *AFile, const UnicodeString ACommand, intptr_t AParams, TCaptureOutputEvent OutputEvent) override;
  virtual void DoStartup() override;
  virtual void HomeDirectory() override;
  virtual bool IsCapable(intptr_t Capability) const override;
  virtual void LookupUsersGroups() override;
  virtual void ReadCurrentDirectory() override;
  virtual void ReadDirectory(TRemoteFileList *AFileList) override;
  virtual void ReadFile(const UnicodeString AFileName,
    TRemoteFile *&AFile) override;
  virtual void ReadSymlink(TRemoteFile *SymlinkFile,
    TRemoteFile *&AFile) override;
  virtual void RemoteRenameFile(const UnicodeString AFileName, const TRemoteFile *AFile,
    const UnicodeString ANewName) override;
  virtual void RemoteCopyFile(const UnicodeString AFileName, const TRemoteFile *AFile,
    const UnicodeString ANewName) override;
  virtual TStrings * GetFixedPaths() const override;
  virtual void SpaceAvailable(const UnicodeString APath,
    TSpaceAvailable &ASpaceAvailable) override;
  virtual const TSessionInfo & GetSessionInfo() const override;
  virtual const TFileSystemInfo & GetFileSystemInfo(bool Retrieve) override;
  virtual bool TemporaryTransferFile(const UnicodeString AFileName) override;
  virtual bool GetStoredCredentialsTried() const override;
  virtual UnicodeString RemoteGetUserName() const override;
  virtual void GetSupportedChecksumAlgs(TStrings *Algs) override;
  virtual void LockFile(const UnicodeString AFileName, const TRemoteFile *AFile) override;
  virtual void UnlockFile(const UnicodeString AFileName, const TRemoteFile *AFile) override;
  virtual void UpdateFromMain(TCustomFileSystem *AMainFileSystem) override;
  virtual void ClearCaches() override;

  virtual void FileTransferProgress(int64_t TransferSize, int64_t Bytes) override;
  void NeonDebug(const UnicodeString AMessage);

protected:
  virtual UnicodeString RemoteGetCurrentDirectory() const override;

  virtual void Sink(
    const UnicodeString AFileName, const TRemoteFile *AFile,
    const UnicodeString ATargetDir, UnicodeString &ADestFileName, uintptr_t Attrs,
    const TCopyParamType *CopyParam, intptr_t AParams, TFileOperationProgressType *OperationProgress,
    uintptr_t AFlags, TDownloadSessionAction &Action) override;
  void ConfirmOverwrite(
    const UnicodeString ASourceFullFileName, UnicodeString &ATargetFileName,
    TFileOperationProgressType *OperationProgress,
    const TOverwriteFileParams *FileParams, const TCopyParamType *CopyParam,
    intptr_t Params);
//    TOverwriteMode &OverwriteMode,
//    uint32_t &Answer);
  void CheckStatus(intptr_t NeonStatus);
  void ClearNeonError();
  static void NeonPropsResult(
    void *UserData, const ne_uri *Uri, const ne_prop_result_set_s *Results);
  void ParsePropResultSet(TRemoteFile *AFile,
    const UnicodeString APath, const ne_prop_result_set_s *Results);
  void TryOpenDirectory(const UnicodeString ADirectory);
  static int NeonBodyReader(void *UserData, const char *Buf, size_t Len);
  static void NeonPreSend(ne_request *Request, void *UserData, ne_buffer *Header);
  static int NeonBodyAccepter(void *UserData, ne_request *Request, const ne_status *Status);
  static void NeonCreateRequest(ne_request *Request, void *UserData, const char *Method, const char *Uri);
  static int NeonRequestAuth(void *UserData, const char *Realm, int Attempt, char *UserName, char *Password);
  void NeonOpen(UnicodeString &CorrectedUrl, const UnicodeString AUrl);
  void NeonClientOpenSessionInternal(UnicodeString &CorrectedUrl, const UnicodeString Url);
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
  void InitSslSessionImpl(ssl_st *Ssl) const;
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
  ne_session_s *FNeonSession;
  ne_lock_store_s *FNeonLockStore;
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

  void CustomReadFile(const UnicodeString AFileName,
    TRemoteFile *& AFile, TRemoteFile *ALinkedByFile);
  intptr_t CustomReadFileInternal(const UnicodeString AFileName,
    TRemoteFile *& AFile, TRemoteFile *ALinkedByFile);
  bool VerifyCertificate(TNeonCertificateData &Data, bool Aux);
  void OpenUrl(const UnicodeString Url);
  void CollectTLSSessionInfo();
  UnicodeString GetRedirectUrl() const;
  UnicodeString ParsePathFromUrl(const UnicodeString Url) const;
  int ReadDirectoryInternal(const UnicodeString APath, TRemoteFileList *AFileList);
  int RenameFileInternal(const UnicodeString AFileName, const UnicodeString ANewName);
  int CopyFileInternal(const UnicodeString AFileName, const UnicodeString ANewName);
  bool IsValidRedirect(intptr_t NeonStatus, UnicodeString &APath) const;
  UnicodeString DirectoryPath(const UnicodeString APath) const;
  UnicodeString FilePath(const TRemoteFile *AFile) const;
  struct ne_lock * FindLock(const RawByteString APath) const;
  void DiscardLock(const RawByteString APath);
  bool IsNtlmAuthentication() const;
  static void NeonAuxRequestInit(ne_session_s *Session, ne_request *Request, void *UserData);
  void SetSessionTls(ne_session_s *Session, bool Aux);
  void InitSession(ne_session_s *Session);
private:
  TFtps GetFtps() const;
};
//---------------------------------------------------------------------------
