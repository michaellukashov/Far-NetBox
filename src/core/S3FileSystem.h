#pragma once

#include <FileSystems.h>
#include <rdestl/map.h>

struct TNeonCertificateData;
struct TOverwriteFileParams;
struct TLibS3CallbackData;
struct TLibS3BucketContext;
struct TLibS3ListBucketCallbackData;
struct TLibS3TransferObjectDataCallbackData;
struct TLibS3PutObjectDataCallbackData;
struct TLibS3GetObjectDataCallbackData;
struct ssl_st;
struct TS3FileProperties;
#ifdef NEED_LIBS3
// resolve clash
#define S3Protocol _S3Protocol
#include <libs3.h>
#undef S3Protocol
#else
struct ne_session_s;
struct ne_ssl_certificate_s;
struct S3ResponseProperties;
struct S3RequestContext;
struct S3ErrorDetails;
struct S3ListBucketContent;
struct S3ResponseHandler;
struct S3AclGrant;
enum S3Status { };
enum _S3Protocol { };
enum S3Permission { };
#endif

NB_DEFINE_CLASS_ID(TS3FileSystem);
class NB_CORE_EXPORT TS3FileSystem final : public TCustomFileSystem
{
  NB_DISABLE_COPY(TS3FileSystem)
public:
  static bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TS3FileSystem); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TS3FileSystem) || TObject::is(Kind); }

  virtual UnicodeString GetAbsolutePath(const UnicodeString & APath, bool Local) const override;
public:
  explicit TS3FileSystem(TTerminal * ATerminal) noexcept;
  ~TS3FileSystem() noexcept override;

  virtual void Open() override;
  virtual void Close() override;
  virtual bool GetActive() const override;
  virtual void CollectUsage() override;
  virtual void Idle() override;
  virtual UnicodeString GetAbsolutePath(const UnicodeString & APath, bool Local) override;
  virtual void AnyCommand(const UnicodeString & ACommand,
    TCaptureOutputEvent OutputEvent) override;
  virtual void ChangeDirectory(const UnicodeString & ADirectory) override;
  virtual void CachedChangeDirectory(const UnicodeString & ADirectory) override;
  virtual void AnnounceFileListOperation() override;
  virtual void ChangeFileProperties(const UnicodeString & AFileName,
    const TRemoteFile * AFile, const TRemoteProperties * Properties,
    TChmodSessionAction & Action) override;
  virtual bool LoadFilesProperties(TStrings * FileList) override;
  virtual void CalculateFilesChecksum(
    const UnicodeString & Alg, TStrings * AFileList, TCalculatedChecksumEvent OnCalculatedChecksum,
    TFileOperationProgressType * OperationProgress, bool FirstLevel) override;
  virtual void CopyToLocal(TStrings * AFilesToCopy,
    const UnicodeString & ATargetDir, const TCopyParamType * CopyParam,
    int32_t AParams, TFileOperationProgressType * OperationProgress,
    TOnceDoneOperation & OnceDoneOperation) override;
  virtual void CopyToRemote(TStrings * AFilesToCopy,
    const UnicodeString & ATargetDir, const TCopyParamType * CopyParam,
    int32_t AParams, TFileOperationProgressType * OperationProgress,
    TOnceDoneOperation & OnceDoneOperation) override;
  virtual void Source(
    TLocalFileHandle & AHandle, const UnicodeString & ATargetDir, UnicodeString & ADestFileName,
    const TCopyParamType * CopyParam, int32_t AParams,
    TFileOperationProgressType * OperationProgress, uint32_t AFlags,
    TUploadSessionAction & Action, bool & ChildError) override;
  virtual void Sink(
    const UnicodeString & AFileName, const TRemoteFile * AFile,
    const UnicodeString & ATargetDir, UnicodeString & ADestFileName, int32_t Attrs,
    const TCopyParamType * CopyParam, int32_t AParams, TFileOperationProgressType * OperationProgress,
    uint32_t AFlags, TDownloadSessionAction &Action) override;
  virtual void RemoteCreateDirectory(const UnicodeString & ADirName, bool Encrypt) override;
  virtual void RemoteCreateLink(const UnicodeString & AFileName, const UnicodeString & APointTo, bool Symbolic) override;
  virtual void RemoteDeleteFile(const UnicodeString & AFileName,
    const TRemoteFile * File, int32_t AParams,
    TRmSessionAction & Action) override;
  virtual void CustomCommandOnFile(const UnicodeString & AFileName,
    const TRemoteFile * File, const UnicodeString & ACommand, int32_t AParams, TCaptureOutputEvent OutputEvent) override;
  virtual void DoStartup() override;
  virtual void HomeDirectory() override;
  virtual bool IsCapable(int32_t Capability) const override;
  virtual void LookupUsersGroups() override;
  virtual void ReadCurrentDirectory() override;
  virtual void ReadDirectory(TRemoteFileList * AFileList) override;
  virtual void ReadFile(const UnicodeString & AFileName,
    TRemoteFile *& AFile) override;
  virtual void ReadSymlink(TRemoteFile * ASymLinkFile,
    TRemoteFile *& AFile) override;
  virtual void RemoteRenameFile(
    const UnicodeString & AFileName, const TRemoteFile * AFile, const UnicodeString & ANewName, bool Overwrite) override;
  virtual void RemoteCopyFile(
    const UnicodeString & AFileName, const TRemoteFile * AFile, const UnicodeString & ANewName, bool Overwrite) override;
  virtual TStrings * GetFixedPaths() const override;
  virtual void SpaceAvailable(const UnicodeString & APath,
    TSpaceAvailable & ASpaceAvailable) override;
  virtual const TSessionInfo & GetSessionInfo() const override;
  virtual const TFileSystemInfo & GetFileSystemInfo(bool Retrieve) override;
  virtual bool TemporaryTransferFile(const UnicodeString & AFileName) override;
  virtual bool GetStoredCredentialsTried() const override;
  virtual UnicodeString RemoteGetUserName() const override;
  virtual void GetSupportedChecksumAlgs(TStrings * Algs) override;
  virtual void LockFile(const UnicodeString & AFileName, const TRemoteFile * AFile) override;
  virtual void UnlockFile(const UnicodeString & AFileName, const TRemoteFile * AFile) override;
  virtual void UpdateFromMain(TCustomFileSystem * MainFileSystem) override;
  virtual void ClearCaches() override;

  virtual void Init(void *) override;
  virtual void FileTransferProgress(int64_t TransferSize, int64_t Bytes) override;
protected:
  bool FActive{false};
  TFileSystemInfo FFileSystemInfo;
  UnicodeString FCurrentDirectory;
  UnicodeString FCachedDirectoryChange;
  TSessionInfo FSessionInfo;
  UTF8String FAccessKeyId;
  UTF8String FSecretAccessKey;
  UTF8String FSecurityTokenBuf;
  const char * FSecurityToken{nullptr};
  UTF8String FHostName;
  UTF8String FPortSuffix;
  int32_t FTimeout{0};
  S3RequestContext *FRequestContext{nullptr};
  _S3Protocol FLibS3Protocol{};
  ne_session_s * FNeonSession{nullptr};
  UnicodeString FTlsVersionStr;
  UnicodeString FResponse;
  bool FResponseIgnore{false};
  using TRegions = nb::map_t<UnicodeString, UnicodeString>;
  TRegions FRegions;
  TRegions FHostNames;
  UnicodeString FAuthRegion;

  virtual UnicodeString RemoteGetCurrentDirectory() const override;

  void LibS3Deinitialize();
  bool VerifyCertificate(TNeonCertificateData &Data);
  void CollectTLSSessionInfo();
  void CheckLibS3Error(const TLibS3CallbackData &Data, bool FatalOnConnectError = false);
  static void InitSslSession(ssl_st *Ssl, ne_session_s *Session);
  void RequestInit(TLibS3CallbackData &Data);
  void TryOpenDirectory(const UnicodeString & ADirectory);
  void ReadDirectoryInternal(const UnicodeString & APath, TRemoteFileList * FileList, int32_t MaxKeys, const UnicodeString & AFileName);
  void ParsePath(const UnicodeString & APath, UnicodeString & BucketName, UnicodeString & AKey);
  TRemoteToken MakeRemoteToken(const char * OwnerId, const char * OwnerDisplayName);
  TLibS3BucketContext GetBucketContext(const UnicodeString & ABucketName, const UnicodeString & Prefix);
  void DoListBucket(
    const UnicodeString & APrefix, TRemoteFileList * FileList, int32_t MaxKeys, const TLibS3BucketContext &BucketContext,
    TLibS3ListBucketCallbackData &Data);
  UnicodeString GetFolderKey(const UnicodeString & AKey);
  void HandleNonBucketStatus(TLibS3CallbackData & Data, bool & Retry);
  void DoReadFile(const UnicodeString & AFileName, TRemoteFile *&AFile);
  void ConfirmOverwrite(
    const UnicodeString & ASourceFullFileName, UnicodeString & ATargetFileName,
    TFileOperationProgressType * OperationProgress, const TOverwriteFileParams * FileParams,
    const TCopyParamType * CopyParam, int32_t AParams);
  int32_t PutObjectData(int32_t BufferSize, char * Buffer, TLibS3PutObjectDataCallbackData & Data);
  S3Status GetObjectData(int32_t BufferSize, const char * Buffer, TLibS3GetObjectDataCallbackData & Data);
  bool ShouldCancelTransfer(TLibS3TransferObjectDataCallbackData & Data);
  bool IsGoogleCloud() const;
  void LoadFileProperties(const UnicodeString & AFileName, const TRemoteFile * File, void * Param);
  bool DoLoadFileProperties(const UnicodeString & AFileName, const TRemoteFile * File, TS3FileProperties & Properties);
  uint16_t AclGrantToPermissions(S3AclGrant & AclGrant, const TS3FileProperties & Properties);
  bool ParsePathForPropertiesRequests(
    const UnicodeString & Path, const TRemoteFile * File, UnicodeString & BucketName, UnicodeString & Key);

  static TS3FileSystem * GetFileSystem(void * CallbackData);
  static void LibS3SessionCallback(ne_session_s * Session, void * CallbackData);
  static S3Status LibS3ResponsePropertiesCallback(const S3ResponseProperties * Properties, void * CallbackData);
  static void LibS3ResponseCompleteCallback(S3Status Status, const S3ErrorDetails * Error, void * CallbackData);
  static int32_t LibS3SslCallback(int32_t Failures, const ne_ssl_certificate_s * Certificate, void * CallbackData);
  static void LibS3ResponseDataCallback(const char * Data, size_t Size, void * CallbackData);
  static S3Status LibS3ListServiceCallback(
    const char * OwnerId, const char * OwnerDisplayName, const char * BucketName,
    int64_t CreationDate, void * CallbackData);
  static S3Status LibS3ListBucketCallback(
    int32_t IsTruncated, const char * NextMarker, int32_t ContentsCount, const S3ListBucketContent * Contents,
    int32_t CommonPrefixesCount, const char ** CommonPrefixes, void * CallbackData);
  static int32_t LibS3PutObjectDataCallback(int32_t BufferSize, char * Buffer, void * CallbackData);
  static S3Status LibS3MultipartInitialCallback(const char * UploadId, void * CallbackData);
  static int32_t LibS3MultipartCommitPutObjectDataCallback(int32_t BufferSize, char * Buffer, void * CallbackData);
  static S3Status LibS3MultipartResponsePropertiesCallback(const S3ResponseProperties * Properties, void * CallbackData);
  static S3Status LibS3GetObjectDataCallback(int32_t BufferSize, const char * Buffer, void * CallbackData);

  static const int32_t S3MinMultiPartChunkSize;
  static const int32_t S3MaxMultiPartChunks;
private:
  void InitSslSessionImpl(ssl_st *Ssl, void * /*Session*/);
};

UnicodeString S3LibVersion();
UnicodeString S3LibDefaultHostName();
UnicodeString S3LibDefaultRegion();
TStrings * GetS3Profiles();
UnicodeString S3EnvUserName(const UnicodeString & Profile, UnicodeString * Source = nullptr);
UnicodeString S3EnvPassword(const UnicodeString & Profile, UnicodeString * Source = nullptr);
UnicodeString S3EnvSessionToken(const UnicodeString & Profile, UnicodeString * Source = nullptr);

