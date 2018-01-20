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
#include "libs3.h"
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
class TS3FileSystem : public TCustomFileSystem
{
public:
  explicit TS3FileSystem(TTerminal * ATerminal);
  virtual __fastcall ~TS3FileSystem();

  virtual void __fastcall Open();
  virtual void __fastcall Close();
  virtual bool __fastcall GetActive() const;
  virtual void __fastcall CollectUsage();
  virtual void __fastcall Idle();
  virtual UnicodeString __fastcall GetAbsolutePath(const UnicodeString APath, bool Local) override;
  virtual UnicodeString __fastcall GetAbsolutePath(const UnicodeString APath, bool Local) const override;
  virtual void __fastcall AnyCommand(const UnicodeString ACommand,
    TCaptureOutputEvent OutputEvent);
  virtual void __fastcall ChangeDirectory(const UnicodeString ADirectory);
  virtual void __fastcall CachedChangeDirectory(const UnicodeString ADirectory);
  virtual void __fastcall AnnounceFileListOperation();
  virtual void __fastcall ChangeFileProperties(const UnicodeString AFileName,
    const TRemoteFile *AFile, const TRemoteProperties *Properties,
    TChmodSessionAction &Action);
  virtual bool __fastcall LoadFilesProperties(TStrings * FileList);
  virtual void __fastcall CalculateFilesChecksum(const UnicodeString Alg,
    TStrings *AFileList, TStrings *Checksums,
    TCalculatedChecksumEvent OnCalculatedChecksum) override;
  virtual void __fastcall CopyToLocal(TStrings * FilesToCopy,
    const UnicodeString ATargetDir, const TCopyParamType * CopyParam,
    intptr_t AParams, TFileOperationProgressType * OperationProgress,
    TOnceDoneOperation &OnceDoneOperation) override;
  virtual void __fastcall CopyToRemote(TStrings * FilesToCopy,
    const UnicodeString ATargetDir, const TCopyParamType *CopyParam,
    intptr_t AParams, TFileOperationProgressType *OperationProgress,
    TOnceDoneOperation & OnceDoneOperation) override;
  virtual void __fastcall Source(
    TLocalFileHandle & Handle, const UnicodeString ATargetDir, UnicodeString &ADestFileName,
    const TCopyParamType *CopyParam, intptr_t AParams,
    TFileOperationProgressType *OperationProgress, uintptr_t AFlags,
    TUploadSessionAction &Action, bool &ChildError);
  virtual void __fastcall Sink(
    const UnicodeString &AFileName, const TRemoteFile *AFile,
    const UnicodeString &ATargetDir, UnicodeString & DestFileName, intptr_t Attrs,
    const TCopyParamType *CopyParam, intptr_t AParams, TFileOperationProgressType *OperationProgress,
    uintptr_t AFlags, TDownloadSessionAction &Action);
  virtual void __fastcall RemoteCreateDirectory(const UnicodeString ADirName);
  virtual void __fastcall RemoteCreateLink(const UnicodeString AFileName, const UnicodeString APointTo, bool Symbolic);
  virtual void __fastcall RemoteDeleteFile(const UnicodeString AFileName,
    const TRemoteFile *File, intptr_t AParams,
    TRmSessionAction &Action);
  virtual void __fastcall CustomCommandOnFile(const UnicodeString AFileName,
    const TRemoteFile *File, const UnicodeString ACommand, intptr_t AParams, TCaptureOutputEvent OutputEvent);
  virtual void __fastcall DoStartup();
  virtual void __fastcall HomeDirectory();
  virtual bool __fastcall IsCapable(int Capability) const;
  virtual void __fastcall LookupUsersGroups();
  virtual void __fastcall ReadCurrentDirectory();
  virtual void __fastcall ReadDirectory(TRemoteFileList * FileList);
  virtual void __fastcall ReadFile(const UnicodeString FileName,
    TRemoteFile *& File);
  virtual void __fastcall ReadSymlink(TRemoteFile * SymLinkFile,
    TRemoteFile *& File);
  virtual void __fastcall RenameFile(const UnicodeString AFileName, const TRemoteFile *AFile,
    const UnicodeString ANewName);
  virtual void __fastcall CopyFile(const UnicodeString AFileName, const TRemoteFile *AFile,
    const UnicodeString ANewName);
  virtual TStrings * __fastcall GetFixedPaths();
  virtual void __fastcall SpaceAvailable(const UnicodeString APath,
    TSpaceAvailable &ASpaceAvailable);
  virtual const TSessionInfo & __fastcall GetSessionInfo();
  virtual const TFileSystemInfo & __fastcall GetFileSystemInfo(bool Retrieve);
  virtual bool __fastcall TemporaryTransferFile(const UnicodeString &AFileName);
  virtual bool __fastcall GetStoredCredentialsTried();
  virtual UnicodeString __fastcall RemoteGetUserName();
  virtual void __fastcall GetSupportedChecksumAlgs(TStrings * Algs);
  virtual void __fastcall LockFile(const UnicodeString &AFileName, const TRemoteFile *AFile);
  virtual void __fastcall UnlockFile(const UnicodeString &AFileName, const TRemoteFile *AFile);
  virtual void __fastcall UpdateFromMain(TCustomFileSystem *MainFileSystem);
  virtual void __fastcall ClearCaches();

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

  virtual UnicodeString __fastcall RemoteGetCurrentDirectory() const;

  void LibS3Deinitialize();
  bool VerifyCertificate(TNeonCertificateData Data);
  void CollectTLSSessionInfo();
  void CheckLibS3Error(const TLibS3CallbackData & Data, bool FatalOnConnectError = false);
  void InitSslSession(ssl_st * Ssl, ne_session_s * Session);
  void RequestInit(TLibS3CallbackData & Data);
  void TryOpenDirectory(const UnicodeString & Directory);
  void ReadDirectoryInternal(const UnicodeString & Path, TRemoteFileList * FileList, int MaxKeys, const UnicodeString & FileName);
  void ParsePath(UnicodeString Path, UnicodeString & BucketName, UnicodeString & Key);
  TRemoteToken MakeRemoteToken(const char * OwnerId, const char * OwnerDisplayName);
  TLibS3BucketContext GetBucketContext(const UnicodeString & BucketName);
  void DoListBucket(
    const UnicodeString & Prefix, TRemoteFileList * FileList, intptr_t MaxKeys, const TLibS3BucketContext &BucketContext,
    TLibS3ListBucketCallbackData & Data);
  UnicodeString GetFolderKey(const UnicodeString & Key);
  void DoReadFile(const UnicodeString & FileName, TRemoteFile *&AFile);
  void ConfirmOverwrite(
    const UnicodeString &ASourceFullFileName, UnicodeString &ATargetFileName,
    TFileOperationProgressType *OperationProgress, const TOverwriteFileParams *FileParams,
    const TCopyParamType *CopyParam, intptr_t AParams);
  int PutObjectData(int BufferSize, char * Buffer, TLibS3PutObjectDataCallbackData & Data);
  S3Status GetObjectData(int BufferSize, const char * Buffer, TLibS3GetObjectDataCallbackData & Data);
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
UnicodeString __fastcall S3LibVersion();
UnicodeString __fastcall S3LibDefaultHostName();
UnicodeString __fastcall S3LibDefaultRegion();
//------------------------------------------------------------------------------
