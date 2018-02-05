#pragma once
//------------------------------------------------------------------------------
#include <FileSystems.h>
#include <rdestl/map.h>
//------------------------------------------------------------------------------
struct TNeonCertificateData;
struct TOverwriteFileParams;
struct TLibS3CallbackData;
struct TLibS3BucketContext;
struct TLibS3ListBucketCallbackData;
struct TLibS3TransferObjectDataCallbackData;
struct TLibS3PutObjectDataCallbackData;
struct TLibS3GetObjectDataCallbackData;
struct ssl_st;
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
enum S3Status { };
enum _S3Protocol { };
#endif
//------------------------------------------------------------------------------
class NB_CORE_EXPORT TS3FileSystem : public TCustomFileSystem
{
  NB_DISABLE_COPY(TS3FileSystem)
public:
  static inline bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TS3FileSystem); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TS3FileSystem) || TObject::is(Kind); }
public:
  explicit TS3FileSystem(TTerminal *ATerminal);
  virtual ~TS3FileSystem();

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
  virtual bool LoadFilesProperties(TStrings * FileList) override;
  virtual void CalculateFilesChecksum(const UnicodeString Alg,
    TStrings *AFileList, TStrings *Checksums,
    TCalculatedChecksumEvent OnCalculatedChecksum) override;
  virtual void CopyToLocal(TStrings *AFilesToCopy,
    const UnicodeString ATargetDir, const TCopyParamType *CopyParam,
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
    TUploadSessionAction &Action, bool &ChildError) override;
  virtual void Sink(
    const UnicodeString AFileName, const TRemoteFile *AFile,
    const UnicodeString ATargetDir, UnicodeString &ADestFileName, uintptr_t Attrs,
    const TCopyParamType *CopyParam, intptr_t AParams, TFileOperationProgressType *OperationProgress,
    uintptr_t AFlags, TDownloadSessionAction &Action) override;
  virtual void RemoteCreateDirectory(const UnicodeString ADirName) override;
  virtual void RemoteCreateLink(const UnicodeString AFileName, const UnicodeString APointTo, bool Symbolic) override;
  virtual void RemoteDeleteFile(const UnicodeString AFileName,
    const TRemoteFile *File, intptr_t AParams,
    TRmSessionAction &Action) override;
  virtual void CustomCommandOnFile(const UnicodeString AFileName,
    const TRemoteFile *File, const UnicodeString ACommand, intptr_t AParams, TCaptureOutputEvent OutputEvent) override;
  virtual void DoStartup() override;
  virtual void HomeDirectory() override;
  virtual bool IsCapable(intptr_t Capability) const override;
  virtual void LookupUsersGroups() override;
  virtual void ReadCurrentDirectory() override;
  virtual void ReadDirectory(TRemoteFileList *AFileList) override;
  virtual void ReadFile(const UnicodeString AFileName,
    TRemoteFile *&AFile) override;
  virtual void ReadSymlink(TRemoteFile *ASymLinkFile,
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
  virtual void UpdateFromMain(TCustomFileSystem *MainFileSystem) override;
  virtual void ClearCaches() override;

  virtual void Init(void *) override;
  virtual void FileTransferProgress(int64_t TransferSize, int64_t Bytes) override;
protected:
  bool FActive;
  TFileSystemInfo FFileSystemInfo;
  UnicodeString FCurrentDirectory;
  UnicodeString FCachedDirectoryChange;
  TSessionInfo FSessionInfo;
  UTF8String FAccessKeyId;
  UTF8String FSecretAccessKey;
  UTF8String FHostName;
  int FTimeout;
  S3RequestContext * FRequestContext;
  _S3Protocol FLibS3Protocol;
  ne_session_s * FNeonSession;
  UnicodeString FTlsVersionStr;
  UnicodeString FResponse;
  bool FResponseIgnore;
  typedef rde::map<UnicodeString, UnicodeString> TRegions;
  TRegions FRegions;
  TRegions FHostNames;

  virtual UnicodeString RemoteGetCurrentDirectory() const;

  void LibS3Deinitialize();
  bool VerifyCertificate(TNeonCertificateData &Data);
  void CollectTLSSessionInfo();
  void CheckLibS3Error(const TLibS3CallbackData &Data, bool FatalOnConnectError = false);
  static void InitSslSession(ssl_st *Ssl, ne_session_s *Session);
  void InitSslSessionImpl(ssl_st *Ssl) const;
  void RequestInit(TLibS3CallbackData &Data);
  void TryOpenDirectory(const UnicodeString ADirectory);
  void ReadDirectoryInternal(const UnicodeString APath, TRemoteFileList * FileList, intptr_t MaxKeys, const UnicodeString AFileName);
  void ParsePath(const UnicodeString APath, UnicodeString &BucketName, UnicodeString &AKey);
  TRemoteToken MakeRemoteToken(const char *OwnerId, const char *OwnerDisplayName);
  TLibS3BucketContext GetBucketContext(const UnicodeString ABucketName);
  void DoListBucket(
    const UnicodeString APrefix, TRemoteFileList * FileList, intptr_t MaxKeys, const TLibS3BucketContext &BucketContext,
    TLibS3ListBucketCallbackData & Data);
  UnicodeString GetFolderKey(const UnicodeString AKey);
  void DoReadFile(const UnicodeString AFileName, TRemoteFile *& AFile);
  void ConfirmOverwrite(
    const UnicodeString ASourceFullFileName, UnicodeString &ATargetFileName,
    TFileOperationProgressType *OperationProgress, const TOverwriteFileParams *FileParams,
    const TCopyParamType *CopyParam, intptr_t AParams);
  int PutObjectData(int BufferSize, char * Buffer, TLibS3PutObjectDataCallbackData &Data);
  S3Status GetObjectData(int BufferSize, const char * Buffer, TLibS3GetObjectDataCallbackData &Data);
  bool ShouldCancelTransfer(TLibS3TransferObjectDataCallbackData & Data);

  static TS3FileSystem * GetFileSystem(void *CallbackData);
  static void LibS3SessionCallback(ne_session_s * Session, void * CallbackData);
  static S3Status LibS3ResponsePropertiesCallback(const S3ResponseProperties * Properties, void * CallbackData);
  static void LibS3ResponseCompleteCallback(S3Status Status, const S3ErrorDetails * Error, void * CallbackData);
  static int LibS3SslCallback(int Failures, const ne_ssl_certificate_s * Certificate, void * CallbackData);
  static void LibS3ResponseDataCallback(const char * Data, size_t Size, void * CallbackData);
  static S3Status LibS3ListServiceCallback(
    const char * OwnerId, const char * OwnerDisplayName, const char * BucketName,
    int64_t CreationDate, void * CallbackData);
  static S3Status LibS3ListBucketCallback(
    int IsTruncated, const char * NextMarker, int ContentsCount, const S3ListBucketContent * Contents,
    int CommonPrefixesCount, const char ** CommonPrefixes, void * CallbackData);
  static int LibS3PutObjectDataCallback(int BufferSize, char * Buffer, void * CallbackData);
  static S3Status LibS3MultipartInitialCallback(const char * UploadId, void * CallbackData);
  static int LibS3MultipartCommitPutObjectDataCallback(int BufferSize, char * Buffer, void * CallbackData);
  static S3Status LibS3MultipartResponsePropertiesCallback(const S3ResponseProperties * Properties, void * CallbackData);
  static S3Status LibS3GetObjectDataCallback(int BufferSize, const char * Buffer, void * CallbackData);

  static const int S3MultiPartChunkSize;
};
//------------------------------------------------------------------------------
UnicodeString S3LibVersion();
UnicodeString S3LibDefaultHostName();
UnicodeString S3LibDefaultRegion();
//------------------------------------------------------------------------------
