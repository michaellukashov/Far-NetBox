
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

enum TFSCommand
{
  fsNull = 0, fsVarValue, fsLastLine, fsFirstLine,
  fsCurrentDirectory, fsChangeDirectory, fsListDirectory, fsListCurrentDirectory,
  fsListFile, fsLookupUsersGroups, fsCopyToRemote, fsCopyToLocal, fsDeleteFile,
  fsRenameFile, fsCreateDirectory, fsChangeMode, fsChangeGroup, fsChangeOwner,
  fsHomeDirectory, fsUnset, fsUnalias, fsCreateLink, fsCopyFile,
  fsAnyCommand, fsLang, fsReadSymlink, fsChangeProperties, fsMoveFile,
  fsLock,
};

const int dfNoRecursive = 0x01;
const int dfAlternative = 0x02;
const int dfForceDelete = 0x04;

// from FtpFileSystem.h
enum TOverwriteMode
{
  omOverwrite,
  omAppend,
  omResume,
  omComplete
};

const int tfFirstLevel   = 0x01;
const int tfAutoResume   = 0x02;
const int tfNewDirectory = 0x04;

struct TSinkFileParams : public TObject
{
public:
  static inline bool classof(const TObject * Obj)
  {
    return
      Obj->GetKind() == OBJECT_CLASS_TSinkFileParams;
  }
public:
  TSinkFileParams() : TObject(OBJECT_CLASS_TSinkFileParams), CopyParam(nullptr), OperationProgress(nullptr), Params(0), Flags(0), Skipped(false) {}
  UnicodeString TargetDir;
  const TCopyParamType * CopyParam;
  TFileOperationProgressType * OperationProgress;
  intptr_t Params;
  uintptr_t Flags;
  bool Skipped;
};

struct TFileTransferData : public TObject
{
NB_DISABLE_COPY(TFileTransferData)
public:
  static inline bool classof(const TObject * Obj)
  {
    return
      Obj->GetKind() == OBJECT_CLASS_TFileTransferData;
  }
public:
  TFileTransferData() :
    TObject(OBJECT_CLASS_TFileTransferData),
    FileName(),
    CopyParam(nullptr),
    Modification(),
    Params(0),
    OverwriteResult(-1),
    AutoResume(false)
  {
  }

  UnicodeString FileName;
  const TCopyParamType * CopyParam;
  TDateTime Modification;
  intptr_t Params;
  int OverwriteResult;
  bool AutoResume;
};

struct TOverwriteFileParams : public TObject
{
public:
  static bool classof(const TObject * Obj)
  {
    return
      Obj->GetKind() == OBJECT_CLASS_TOverwriteFileParams;
  }
public:
  TOverwriteFileParams() :
    TObject(OBJECT_CLASS_TOverwriteFileParams),
    SourceSize(0),
    DestSize(0),
    SourcePrecision(mfFull),
    DestPrecision(mfFull)
  {
  }

  int64_t SourceSize;
  int64_t DestSize;
  TDateTime SourceTimestamp;
  TDateTime DestTimestamp;
  TModificationFmt SourcePrecision;
  TModificationFmt DestPrecision;
};

struct TOpenRemoteFileParams : public TObject
{
NB_DISABLE_COPY(TOpenRemoteFileParams)
public:
  static inline bool classof(const TObject * Obj)
  {
    return
      Obj->GetKind() == OBJECT_CLASS_TOpenRemoteFileParams;
  }
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
  uintptr_t LocalFileAttrs;
  UnicodeString FileName;
  UnicodeString RemoteFileName;
  TFileOperationProgressType * OperationProgress;
  const TCopyParamType * CopyParam;
  intptr_t Params;
  bool Resume;
  bool Resuming;
  TOverwriteMode OverwriteMode;
  int64_t DestFileSize; // output
  RawByteString RemoteFileHandle; // output
  TOverwriteFileParams * FileParams;
  bool Confirmed;
};

/** @brief Interface for custom filesystems
  *
  */
class TFileSystemIntf
{
public:
  virtual ~TFileSystemIntf() {}

  virtual void Init(void *) = 0;
  virtual void FileTransferProgress(int64_t TransferSize, int64_t Bytes) = 0;
};

class TCustomFileSystem : public TObject, public TFileSystemIntf
{
NB_DISABLE_COPY(TCustomFileSystem)
public:
  static inline bool classof(const TObject * Obj)
  {
    return
      Obj->GetKind() == OBJECT_CLASS_TCustomFileSystem ||
      Obj->GetKind() == OBJECT_CLASS_TFTPFileSystem ||
      Obj->GetKind() == OBJECT_CLASS_TSCPFileSystem ||
      Obj->GetKind() == OBJECT_CLASS_TSFTPFileSystem ||
      Obj->GetKind() == OBJECT_CLASS_TWebDAVFileSystem;
  }
public:
  virtual ~TCustomFileSystem();

  virtual void Open() = 0;
  virtual void Close() = 0;
  virtual bool GetActive() const = 0;
  virtual void CollectUsage() = 0;
  virtual void Idle() = 0;
  virtual UnicodeString GetAbsolutePath(const UnicodeString & APath, bool Local) = 0;
  virtual UnicodeString GetAbsolutePath(const UnicodeString & APath, bool Local) const = 0;
  virtual void AnyCommand(const UnicodeString & Command,
    TCaptureOutputEvent OutputEvent) = 0;
  virtual void ChangeDirectory(const UnicodeString & Directory) = 0;
  virtual void CachedChangeDirectory(const UnicodeString & Directory) = 0;
  virtual void AnnounceFileListOperation() = 0;
  virtual void ChangeFileProperties(const UnicodeString & AFileName,
    const TRemoteFile * AFile, const TRemoteProperties * Properties,
    TChmodSessionAction & Action) = 0;
  virtual bool LoadFilesProperties(TStrings * AFileList) = 0;
  virtual void CalculateFilesChecksum(const UnicodeString & Alg,
    TStrings * AFileList, TStrings * Checksums,
    TCalculatedChecksumEvent OnCalculatedChecksum) = 0;
  virtual void CopyToLocal(const TStrings * AFilesToCopy,
    const UnicodeString & TargetDir, const TCopyParamType * CopyParam,
    intptr_t Params, TFileOperationProgressType * OperationProgress,
    TOnceDoneOperation & OnceDoneOperation) = 0;
  virtual void CopyToRemote(const TStrings * AFilesToCopy,
    const UnicodeString & TargetDir, const TCopyParamType * CopyParam,
    intptr_t Params, TFileOperationProgressType * OperationProgress,
    TOnceDoneOperation & OnceDoneOperation) = 0;
  virtual void RemoteCreateDirectory(const UnicodeString & ADirName) = 0;
  virtual void CreateLink(const UnicodeString & AFileName, const UnicodeString & PointTo, bool Symbolic) = 0;
  virtual void RemoteDeleteFile(const UnicodeString & AFileName,
    const TRemoteFile * AFile, intptr_t Params,
    TRmSessionAction & Action) = 0;
  virtual void CustomCommandOnFile(const UnicodeString & AFileName,
    const TRemoteFile * AFile, const UnicodeString & Command, intptr_t Params, TCaptureOutputEvent OutputEvent) = 0;
  virtual void DoStartup() = 0;
  virtual void HomeDirectory() = 0;
  virtual bool IsCapable(intptr_t Capability) const = 0;
  virtual void LookupUsersGroups() = 0;
  virtual void ReadCurrentDirectory() = 0;
  virtual void ReadDirectory(TRemoteFileList * FileList) = 0;
  virtual void ReadFile(const UnicodeString & AFileName,
    TRemoteFile *& File) = 0;
  virtual void ReadSymlink(TRemoteFile * SymLinkFile,
    TRemoteFile *& File) = 0;
  virtual void RemoteRenameFile(const UnicodeString & AFileName,
    const UnicodeString & ANewName) = 0;
  virtual void RemoteCopyFile(const UnicodeString & AFileName,
    const UnicodeString & ANewName) = 0;
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

#if 0
  __property UnicodeString CurrentDirectory = { read = GetCurrentDirectory };
#endif // #if 0
  UnicodeString RemoteCurrentDirectory() const { return RemoteGetCurrentDirectory(); }

protected:
  TTerminal * FTerminal;

  explicit TCustomFileSystem(TObjectClassId Kind) : TObject(Kind), FTerminal(nullptr) {}
  explicit TCustomFileSystem(TObjectClassId Kind, TTerminal * ATerminal);
  virtual UnicodeString RemoteGetCurrentDirectory() const = 0;

  UnicodeString CreateTargetDirectory(
    IN const UnicodeString & AFileName,
    IN const UnicodeString & ADirectory,
    IN const TCopyParamType * CopyParam);
};

