
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
  virtual __fastcall ~TWebDAVFileSystem();
  virtual void Init(void *) override;

  virtual void __fastcall Open() override;
  virtual void __fastcall Close() override;
  virtual bool __fastcall GetActive() const override;
  virtual void __fastcall CollectUsage() override;
  virtual void __fastcall Idle() override;
  virtual UnicodeString __fastcall GetAbsolutePath(const UnicodeString APath, bool Local) override;
  virtual UnicodeString GetAbsolutePath(const UnicodeString APath, bool Local) const override;
  virtual void __fastcall AnyCommand(const UnicodeString ACommand,
    TCaptureOutputEvent OutputEvent) override;
  virtual void __fastcall ChangeDirectory(const UnicodeString ADirectory) override;
  virtual void __fastcall CachedChangeDirectory(const UnicodeString ADirectory) override;
  virtual void __fastcall AnnounceFileListOperation() override;
  virtual void __fastcall ChangeFileProperties(const UnicodeString AFileName,
    const TRemoteFile *AFile, const TRemoteProperties *Properties,
    TChmodSessionAction &Action) override;
  virtual bool __fastcall LoadFilesProperties(TStrings *AFileList) override;
  virtual void __fastcall CalculateFilesChecksum(const UnicodeString Alg,
    TStrings *AFileList, TStrings *Checksums,
    TCalculatedChecksumEvent OnCalculatedChecksum) override;
  virtual void __fastcall CopyToLocal(TStrings *AFilesToCopy,
    const UnicodeString TargetDir, const TCopyParamType *CopyParam,
    intptr_t AParams, TFileOperationProgressType *OperationProgress,
    TOnceDoneOperation &OnceDoneOperation) override;
  virtual void __fastcall CopyToRemote(TStrings *AFilesToCopy,
    const UnicodeString ATargetDir, const TCopyParamType *CopyParam,
    intptr_t AParams, TFileOperationProgressType *OperationProgress,
    TOnceDoneOperation &OnceDoneOperation) override;
  virtual void __fastcall Source(
    TLocalFileHandle &AHandle, const UnicodeString ATargetDir, UnicodeString &ADestFileName,
    const TCopyParamType *CopyParam, intptr_t AParams,
    TFileOperationProgressType *OperationProgress, uintptr_t AFlags,
    TUploadSessionAction &Action, bool &ChildError);
  virtual void __fastcall RemoteCreateDirectory(const UnicodeString ADirName) override;
  virtual void __fastcall RemoteCreateLink(const UnicodeString AFileName, const UnicodeString APointTo, bool Symbolic) override;
  virtual void __fastcall RemoteDeleteFile(const UnicodeString AFileName,
    const TRemoteFile *AFile, intptr_t Params, TRmSessionAction &Action) override;
  virtual void __fastcall CustomCommandOnFile(const UnicodeString AFileName,
    const TRemoteFile *AFile, const UnicodeString ACommand, intptr_t AParams, TCaptureOutputEvent OutputEvent) override;
  virtual void __fastcall DoStartup() override;
  virtual void __fastcall HomeDirectory() override;
  virtual bool __fastcall IsCapable(intptr_t Capability) const override;
  virtual void __fastcall LookupUsersGroups() override;
  virtual void __fastcall ReadCurrentDirectory() override;
  virtual void __fastcall ReadDirectory(TRemoteFileList *AFileList) override;
  virtual void __fastcall ReadFile(const UnicodeString AFileName,
    TRemoteFile *&AFile) override;
  virtual void __fastcall ReadSymlink(TRemoteFile *SymlinkFile,
    TRemoteFile *&AFile) override;
  virtual void __fastcall RemoteRenameFile(const UnicodeString AFileName, const TRemoteFile *AFile,
    const UnicodeString ANewName) override;
  virtual void __fastcall RemoteCopyFile(const UnicodeString AFileName, const TRemoteFile *AFile,
    const UnicodeString ANewName) override;
  virtual TStrings * __fastcall GetFixedPaths() const override;
  virtual void __fastcall SpaceAvailable(const UnicodeString APath,
    TSpaceAvailable &ASpaceAvailable) override;
  virtual const TSessionInfo & __fastcall GetSessionInfo() const override;
  virtual const TFileSystemInfo & __fastcall GetFileSystemInfo(bool Retrieve) override;
  virtual bool __fastcall TemporaryTransferFile(const UnicodeString AFileName) override;
  virtual bool __fastcall GetStoredCredentialsTried() const override;
  virtual UnicodeString __fastcall RemoteGetUserName() const override;
  virtual void __fastcall GetSupportedChecksumAlgs(TStrings *Algs) override;
  virtual void __fastcall LockFile(const UnicodeString AFileName, const TRemoteFile *AFile) override;
  virtual void __fastcall UnlockFile(const UnicodeString AFileName, const TRemoteFile *AFile) override;
  virtual void __fastcall UpdateFromMain(TCustomFileSystem *AMainFileSystem) override;
  virtual void __fastcall ClearCaches() override;

  virtual void FileTransferProgress(int64_t TransferSize, int64_t Bytes) override;
  void NeonDebug(const UnicodeString AMessage);

protected:
  virtual UnicodeString RemoteGetCurrentDirectory() const override;

  virtual void __fastcall Sink(
    const UnicodeString AFileName, const TRemoteFile *AFile,
    const UnicodeString ATargetDir, UnicodeString &ADestFileName, uintptr_t Attrs,
    const TCopyParamType *CopyParam, intptr_t AParams, TFileOperationProgressType *OperationProgress,
    uintptr_t AFlags, TDownloadSessionAction &Action) override;
  void __fastcall ConfirmOverwrite(
    const UnicodeString ASourceFullFileName, UnicodeString &ATargetFileName,
    TFileOperationProgressType *OperationProgress,
    const TOverwriteFileParams *FileParams, const TCopyParamType *CopyParam,
    intptr_t Params);
//    TOverwriteMode &OverwriteMode,
//    uint32_t &Answer);
  void __fastcall CheckStatus(intptr_t NeonStatus);
  void __fastcall ClearNeonError();
  static void NeonPropsResult(
    void *UserData, const ne_uri *Uri, const ne_prop_result_set_s *Results);
  void __fastcall ParsePropResultSet(TRemoteFile *AFile,
    const UnicodeString APath, const ne_prop_result_set_s *Results);
  void __fastcall TryOpenDirectory(UnicodeString ADirectory);
  static int NeonBodyReader(void *UserData, const char *Buf, size_t Len);
  static void NeonPreSend(ne_request *Request, void *UserData, ne_buffer *Header);
  static int NeonBodyAccepter(void *UserData, ne_request *Request, const ne_status *Status);
  static void NeonCreateRequest(ne_request *Request, void *UserData, const char *Method, const char *Uri);
  static int NeonRequestAuth(void *UserData, const char *Realm, int Attempt, char *UserName, char *Password);
  void NeonOpen(UnicodeString &CorrectedUrl, const UnicodeString AUrl);
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
  void __fastcall CloseNeonSession();
  bool __fastcall CancelTransfer();
  UnicodeString __fastcall GetNeonError() const;
  static void NeonQuotaResult(void *UserData, const ne_uri *Uri, const ne_prop_result_set_s *Results);
  static const char * __fastcall GetNeonProp(const ne_prop_result_set_s *Results,
    const char *Name, const char *NameSpace = nullptr);
  static void LockResult(void *UserData, const struct ne_lock *Lock,
   const ne_uri *Uri, const ne_status *Status);
  void __fastcall RequireLockStore();
  static void InitSslSession(ssl_st *Ssl, ne_session *Session);
  void InitSslSessionImpl(ssl_st *Ssl) const;
  void __fastcall NeonAddAuthentication(bool UseNegotiate);
  void __fastcall HttpAuthenticationFailed();

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

  void __fastcall CustomReadFile(UnicodeString AFileName,
    TRemoteFile *& AFile, TRemoteFile *ALinkedByFile);
  intptr_t __fastcall CustomReadFileInternal(const UnicodeString AFileName,
    TRemoteFile *& AFile, TRemoteFile *ALinkedByFile);
  bool VerifyCertificate(TNeonCertificateData &Data, bool Aux);
  void OpenUrl(const UnicodeString Url);
  void __fastcall CollectTLSSessionInfo();
  UnicodeString __fastcall GetRedirectUrl() const;
  UnicodeString __fastcall ParsePathFromUrl(const UnicodeString Url) const;
  int __fastcall ReadDirectoryInternal(const UnicodeString APath, TRemoteFileList *AFileList);
  int __fastcall RenameFileInternal(const UnicodeString AFileName, const UnicodeString ANewName);
  int __fastcall CopyFileInternal(const UnicodeString AFileName, const UnicodeString ANewName);
  bool __fastcall IsValidRedirect(intptr_t NeonStatus, UnicodeString &APath) const;
  UnicodeString __fastcall DirectoryPath(const UnicodeString APath) const;
  UnicodeString __fastcall FilePath(const TRemoteFile *AFile) const;
  struct ne_lock * __fastcall FindLock(const RawByteString APath) const;
  void __fastcall DiscardLock(const RawByteString APath);
  bool __fastcall IsNtlmAuthentication() const;
  static void NeonAuxRequestInit(ne_session_s *Session, ne_request *Request, void *UserData);
  void __fastcall SetSessionTls(ne_session_s *Session, bool Aux);
  void __fastcall InitSession(ne_session_s *Session);
private:
  TFtps GetFtps() const;
};
//---------------------------------------------------------------------------
