
#pragma once

#include <Common.h>
#include <Exceptions.h>

#include "SessionInfo.h"

class TTerminal;
class TRights;
class TRemoteFile;
class TRemoteFileList;
class TCopyParamType;
struct TSpaceAvailable;
class TFileOperationProgressType;
class TRemoteProperties;
struct TLocalFileHandle;

enum TFSCommand { fsNull = 0, fsVarValue, fsLastLine, fsFirstLine,
  fsCurrentDirectory, fsChangeDirectory, fsListDirectory, fsListCurrentDirectory,
  fsListFile, fsLookupUsersGroups, fsCopyToRemote, fsCopyToLocal, fsDeleteFile,
  fsRenameFile, fsCreateDirectory, fsChangeMode, fsChangeGroup, fsChangeOwner,
  fsHomeDirectory, fsUnset, fsUnalias, fsCreateLink, fsCopyFile,
  fsAnyCommand, fsLang, fsReadSymlink, fsChangeProperties, fsMoveFile,
  fsLock };

constexpr int32_t dfNoRecursive = 0x01;
constexpr int32_t dfAlternative = 0x02;
constexpr int32_t dfForceDelete = 0x04;

enum TOverwriteMode { omOverwrite, omAppend, omResume, omComplete };

NB_DEFINE_CLASS_ID(TSinkFileParams);
struct NB_CORE_EXPORT TSinkFileParams : public TObject
{
public:
  static bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TSinkFileParams); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TSinkFileParams) || TObject::is(Kind); }
public:
  TSinkFileParams() noexcept : TObject(OBJECT_CLASS_TSinkFileParams), CopyParam(nullptr), OperationProgress(nullptr), Params(0), Flags(0), Skipped(false) {}
  UnicodeString TargetDir;
  const TCopyParamType *CopyParam{nullptr};
  int32_t Params{0};
  TFileOperationProgressType *OperationProgress{nullptr};
  bool Skipped{false};
  uint32_t Flags{0};
};

NB_DEFINE_CLASS_ID(TFileTransferData);
struct NB_CORE_EXPORT TFileTransferData : public TObject
{
  NB_DISABLE_COPY(TFileTransferData)
public:
  static bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TFileTransferData); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TFileTransferData) || TObject::is(Kind); }
public:
  TFileTransferData() : TObject(OBJECT_CLASS_TFileTransferData) {}
  UnicodeString FileName;
  const TCopyParamType *CopyParam{nullptr};
  TDateTime Modification{};
  int32_t Params{0};
  int32_t OverwriteResult{-1};
  bool AutoResume{false};
};

NB_DEFINE_CLASS_ID(TOverwriteFileParams);
struct NB_CORE_EXPORT TOverwriteFileParams : public TObject
{
public:
  static bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TOverwriteFileParams); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TOverwriteFileParams) || TObject::is(Kind); }
public:
  TOverwriteFileParams() noexcept :
    TObject(OBJECT_CLASS_TOverwriteFileParams),
    SourceSize(0),
    DestSize(0),
    SourcePrecision(mfFull),
    DestPrecision(mfFull)
  {
  }

  int64_t SourceSize{0};
  int64_t DestSize{0};
  TDateTime SourceTimestamp{};
  TDateTime DestTimestamp{};
  TModificationFmt SourcePrecision{mfFull};
  TModificationFmt DestPrecision{mfFull};
};

NB_DEFINE_CLASS_ID(TOpenRemoteFileParams);
struct NB_CORE_EXPORT TOpenRemoteFileParams : public TObject
{
  NB_DISABLE_COPY(TOpenRemoteFileParams)
public:
  static bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TOpenRemoteFileParams); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TOpenRemoteFileParams) || TObject::is(Kind); }
public:
  TOpenRemoteFileParams() :
    TObject(OBJECT_CLASS_TOpenRemoteFileParams),
    LocalFileAttrs(0),
    OperationProgress(nullptr),
    CopyParam(nullptr),
    Params(0),
    Resume(false),
    Resuming(false),
    OverwriteMode(omOverwrite),
    DestFileSize(0),
    FileParams(nullptr),
    Confirmed(false)
  {
  }
  UnicodeString FileName;
  UnicodeString RemoteFileName;
  TFileOperationProgressType *OperationProgress{nullptr};
  const TCopyParamType *CopyParam{nullptr};
  int32_t Params{0};
  bool Resume{false};
  bool Resuming{false};
  TOverwriteMode OverwriteMode{};
  int64_t DestFileSize{0}; // output
  RawByteString RemoteFileHandle; // output
  TOverwriteFileParams *FileParams{nullptr};
  bool Confirmed{false};
  bool DontRecycle{false};
  bool Recycled{false};
  TRights RecycledRights{};
  uint32_t LocalFileAttrs{0};
};

/** @brief Interface for custom filesystems
  *
  */
class NB_CORE_EXPORT TFileSystemIntf
{
public:
  virtual ~TFileSystemIntf() = default;

  virtual void Init(void *) = 0;
  virtual void FileTransferProgress(int64_t TransferSize, int64_t Bytes) = 0;
};

NB_DEFINE_CLASS_ID(TCustomFileSystem);
class NB_CORE_EXPORT TCustomFileSystem : public TObject, public TFileSystemIntf
{
  NB_DISABLE_COPY(TCustomFileSystem)
public:
  static bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TCustomFileSystem); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TCustomFileSystem) || TObject::is(Kind); }
public:
  ~TCustomFileSystem() noexcept override;

  virtual void Open() = 0;
  virtual void Close() = 0;
  virtual bool GetActive() const = 0;
  virtual void CollectUsage() = 0;
  virtual void Idle() = 0;
  virtual UnicodeString GetAbsolutePath(const UnicodeString & APath, bool Local) = 0;
  virtual UnicodeString GetAbsolutePath(const UnicodeString & APath, bool Local) const = 0;
  virtual void AnyCommand(const UnicodeString & ACommand,
    TCaptureOutputEvent OutputEvent) = 0;
  virtual void ChangeDirectory(const UnicodeString & ADirectory) = 0;
  virtual void CachedChangeDirectory(const UnicodeString & ADirectory) = 0;
  virtual void AnnounceFileListOperation() = 0;
  virtual void ChangeFileProperties(const UnicodeString & AFileName,
    const TRemoteFile * AFile, const TRemoteProperties * Properties,
    TChmodSessionAction & Action) = 0;
  virtual bool LoadFilesProperties(TStrings * AFileList) = 0;
  virtual UnicodeString CalculateFilesChecksumInitialize(const UnicodeString & Alg);
  virtual void CalculateFilesChecksum(
    const UnicodeString & Alg, TStrings * FileList, TCalculatedChecksumEvent OnCalculatedChecksum,
    TFileOperationProgressType * OperationProgress, bool FirstLevel) = 0;
  virtual void CopyToLocal(TStrings * AFilesToCopy,
    const UnicodeString & ATargetDir, const TCopyParamType * CopyParam,
    int32_t AParams, TFileOperationProgressType * OperationProgress,
    TOnceDoneOperation & OnceDoneOperation) = 0;
  virtual void CopyToRemote(TStrings * AFilesToCopy,
    const UnicodeString & ATargetDir, const TCopyParamType * CopyParam,
    int32_t AParams, TFileOperationProgressType * OperationProgress,
    TOnceDoneOperation & OnceDoneOperation) = 0;
  virtual void TransferOnDirectory(
    const UnicodeString /*ADirectory*/, const TCopyParamType * /*CopyParam*/, int32_t /*AParams*/) {}
  virtual void Source(
    TLocalFileHandle & AHandle, const UnicodeString & ATargetDir, UnicodeString & ADestFileName,
    const TCopyParamType * CopyParam, int32_t AParams,
    TFileOperationProgressType * OperationProgress, uint32_t AFlags,
    TUploadSessionAction & Action, bool & ChildError) = 0;
  virtual void DirectorySunk(
    const UnicodeString & /*ADestFullName*/, const TRemoteFile * /*AFile*/, const TCopyParamType * /*ACopyParam*/) {}
  virtual void Sink(
    const UnicodeString & AFileName, const TRemoteFile * AFile,
    const UnicodeString & ATargetDir, UnicodeString & ADestFileName, int32_t AAttrs,
    const TCopyParamType * CopyParam, int32_t AParams, TFileOperationProgressType * OperationProgress,
    uint32_t AFlags, TDownloadSessionAction& Action) = 0;
  virtual void RemoteCreateDirectory(const UnicodeString & ADirName, bool Encrypt) = 0;
  virtual void RemoteCreateLink(const UnicodeString & AFileName, const UnicodeString & APointTo, bool Symbolic) = 0;
  virtual void RemoteDeleteFile(const UnicodeString & AFileName,
    const TRemoteFile * AFile, int32_t AParams,
    TRmSessionAction & Action) = 0;
  virtual void CustomCommandOnFile(const UnicodeString & AFileName,
    const TRemoteFile *AFile, const UnicodeString & ACommand, int32_t AParams, TCaptureOutputEvent OutputEvent) = 0;
  virtual void DoStartup() = 0;
  virtual void HomeDirectory() = 0;
  virtual UnicodeString GetHomeDirectory();
  virtual bool IsCapable(int32_t Capability) const = 0;
  virtual void LookupUsersGroups() = 0;
  virtual void ReadCurrentDirectory() = 0;
  virtual void ReadDirectory(TRemoteFileList *FileList) = 0;
  virtual void ReadFile(const UnicodeString AFileName,
    TRemoteFile *&File) = 0;
  virtual void ReadSymlink(TRemoteFile *SymLinkFile,
    TRemoteFile *&File) = 0;
  virtual void RemoteRenameFile(
    const UnicodeString & AFileName, const TRemoteFile * AFile, const UnicodeString & ANewName, bool Overwrite) = 0;
  virtual void RemoteCopyFile(
    const UnicodeString & AFileName, const TRemoteFile * AFile, const UnicodeString & ANewName, bool Overwrite) = 0;
  virtual TStrings * GetFixedPaths() const = 0;
  virtual void SpaceAvailable(const UnicodeString & APath,
    TSpaceAvailable & ASpaceAvailable) = 0;
  virtual const TSessionInfo & GetSessionInfo() const = 0;
  virtual const TFileSystemInfo & GetFileSystemInfo(bool Retrieve) = 0;
  virtual bool TemporaryTransferFile(const UnicodeString & AFileName) = 0;
  virtual bool GetStoredCredentialsTried() const = 0;
  virtual UnicodeString RemoteGetUserName() const = 0;
  virtual void GetSupportedChecksumAlgs(TStrings * Algs) = 0;
  virtual void LockFile(const UnicodeString & AFileName, const TRemoteFile * AFile) = 0;
  virtual void UnlockFile(const UnicodeString & AFileName, const TRemoteFile * AFile) = 0;
  virtual void UpdateFromMain(TCustomFileSystem * MainFileSystem) = 0;
  virtual void ClearCaches() = 0;

  __property UnicodeString CurrentDirectory = { read = GetCurrentDirectory };
  ROProperty<UnicodeString> RemoteCurrentDirectory{nb::bind(&TCustomFileSystem::RemoteGetCurrentDirectory, this)};

protected:
  TTerminal * FTerminal{nullptr};

  TCustomFileSystem() = delete;
  explicit TCustomFileSystem(TObjectClassId Kind) noexcept : TObject(Kind) {}
  explicit TCustomFileSystem(TObjectClassId Kind, TTerminal *ATerminal) noexcept;
  virtual UnicodeString RemoteGetCurrentDirectory() const = 0;

  UnicodeString CreateTargetDirectory(
    IN UnicodeString AFileName,
    IN UnicodeString ADirectory,
    IN const TCopyParamType *CopyParam);
};

