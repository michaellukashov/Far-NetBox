//---------------------------------------------------------------------------
#ifndef FileSystemsH
#define FileSystemsH

#include <SessionInfo.h>
#include "Exceptions.h"
//---------------------------------------------------------------------------
class TTerminal;
class TRights;
class TRemoteFile;
class TRemoteFileList;
class TCopyParamType;
struct TSpaceAvailable;
class TFileOperationProgressType;
class TRemoteProperties;
//---------------------------------------------------------------------------
enum TFSCommand
{
  fsNull = 0, fsVarValue, fsLastLine, fsFirstLine,
  fsCurrentDirectory, fsChangeDirectory, fsListDirectory, fsListCurrentDirectory,
  fsListFile, fsLookupUsersGroups, fsCopyToRemote, fsCopyToLocal, fsDeleteFile,
  fsRenameFile, fsCreateDirectory, fsChangeMode, fsChangeGroup, fsChangeOwner,
  fsHomeDirectory, fsUnset, fsUnalias, fsCreateLink, fsCopyFile,
  fsAnyCommand, fsReadSymlink, fsChangeProperties, fsMoveFile
};
//---------------------------------------------------------------------------
const int dfNoRecursive = 0x01;
const int dfAlternative = 0x02;
const int dfForceDelete = 0x04;
//---------------------------------------------------------------------------
enum TOverwriteMode { omOverwrite, omAppend, omResume, omComplete };
//---------------------------------------------------------------------------
const int tfFirstLevel =   0x01;
const int tfAutoResume = 0x02;
const int tfNewDirectory = 0x04;
//---------------------------------------------------------------------------
struct TSinkFileParams : public TObject
{
  UnicodeString TargetDir;
  const TCopyParamType *CopyParam;
  TFileOperationProgressType *OperationProgress;
  intptr_t Params;
  uintptr_t Flags;
  bool Skipped;
};
//---------------------------------------------------------------------------
struct TFileTransferData : public TObject
{
NB_DISABLE_COPY(TFileTransferData)
  TFileTransferData() :
    CopyParam(NULL),
    Modification(0.0),
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
//---------------------------------------------------------------------------
struct TClipboardHandler : public TObject
{
  UnicodeString Text;

  void Copy(TObject * /*Sender*/)
  {
    CopyToClipboard(Text.c_str());
  }
};
//---------------------------------------------------------------------------
struct TOverwriteFileParams : public TObject
{
  TOverwriteFileParams() :
    SourceSize(0),
    DestSize(0),
    SourcePrecision(mfFull),
    DestPrecision(mfFull)
  {}

  __int64 SourceSize;
  __int64 DestSize;
  TDateTime SourceTimestamp;
  TDateTime DestTimestamp;
  TModificationFmt SourcePrecision;
  TModificationFmt DestPrecision;
};
//---------------------------------------------------------------------------
struct TOpenRemoteFileParams : public TObject
{
  TOpenRemoteFileParams() :
    LocalFileAttrs(0),
    OperationProgress(NULL),
    CopyParam(NULL),
    Params(0),
    Resume(false),
    Resuming(false),
    OverwriteMode(omOverwrite),
    DestFileSize(0),
    FileParams(NULL),
    Confirmed(false)
  {}
  uintptr_t LocalFileAttrs;
  UnicodeString RemoteFileName;
  TFileOperationProgressType * OperationProgress;
  const TCopyParamType * CopyParam;
  intptr_t Params;
  bool Resume;
  bool Resuming;
  TOverwriteMode OverwriteMode;
  __int64 DestFileSize; // output
  RawByteString RemoteFileHandle; // output
  TOverwriteFileParams *FileParams;
  bool Confirmed;

private:
  NB_DISABLE_COPY(TOpenRemoteFileParams)
};
//---------------------------------------------------------------------------

/** @brief Interface for custom filesystems
  *
  */
class TFileSystemIntf
{
public:
  virtual ~TFileSystemIntf() {}

  virtual void Init(void *) = 0;
  virtual void FileTransferProgress(__int64 TransferSize, __int64 Bytes) = 0;
};

//---------------------------------------------------------------------------
class TCustomFileSystem : public TObject, public TFileSystemIntf
{
NB_DISABLE_COPY(TCustomFileSystem)
public:
  virtual ~TCustomFileSystem();

  virtual void Open() = 0;
  virtual void Close() = 0;
  virtual bool GetActive() const = 0;
  virtual void Idle() = 0;
  virtual UnicodeString AbsolutePath(const UnicodeString & Path, bool Local) = 0;
  virtual void AnyCommand(const UnicodeString & Command,
    TCaptureOutputEvent OutputEvent) = 0;
  virtual void ChangeDirectory(const UnicodeString & Directory) = 0;
  virtual void CachedChangeDirectory(const UnicodeString & Directory) = 0;
  virtual void AnnounceFileListOperation() = 0;
  virtual void ChangeFileProperties(const UnicodeString & FileName,
    const TRemoteFile * AFile, const TRemoteProperties * Properties,
    TChmodSessionAction & Action) = 0;
  virtual bool LoadFilesProperties(TStrings * FileList) = 0;
  virtual void CalculateFilesChecksum(const UnicodeString & Alg,
    TStrings * FileList, TStrings * Checksums,
    TCalculatedChecksumEvent OnCalculatedChecksum) = 0;
  virtual void CopyToLocal(TStrings * AFilesToCopy,
    const UnicodeString & TargetDir, const TCopyParamType * CopyParam,
    intptr_t Params, TFileOperationProgressType * OperationProgress,
    TOnceDoneOperation & OnceDoneOperation) = 0;
  virtual void CopyToRemote(TStrings * AFilesToCopy,
    const UnicodeString & TargetDir, const TCopyParamType * CopyParam,
    intptr_t Params, TFileOperationProgressType * OperationProgress,
    TOnceDoneOperation & OnceDoneOperation) = 0;
  virtual void CreateDirectory(const UnicodeString & DirName) = 0;
  virtual void CreateLink(const UnicodeString & FileName, const UnicodeString & PointTo, bool Symbolic) = 0;
  virtual void DeleteFile(const UnicodeString & FileName,
    const TRemoteFile * AFile, intptr_t Params,
    TRmSessionAction & Action) = 0;
  virtual void CustomCommandOnFile(const UnicodeString & FileName,
    const TRemoteFile * AFile, const UnicodeString & Command, intptr_t Params, TCaptureOutputEvent OutputEvent) = 0;
  virtual void DoStartup() = 0;
  virtual void HomeDirectory() = 0;
  virtual bool IsCapable(intptr_t Capability) const = 0;
  virtual void LookupUsersGroups() = 0;
  virtual void ReadCurrentDirectory() = 0;
  virtual void ReadDirectory(TRemoteFileList * FileList) = 0;
  virtual void ReadFile(const UnicodeString & FileName,
    TRemoteFile *& File) = 0;
  virtual void ReadSymlink(TRemoteFile * SymLinkFile,
    TRemoteFile *& File) = 0;
  virtual void RenameFile(const UnicodeString & FileName,
    const UnicodeString & NewName) = 0;
  virtual void CopyFile(const UnicodeString & FileName,
    const UnicodeString & NewName) = 0;
  virtual UnicodeString FileUrl(const UnicodeString & FileName) = 0;
  virtual TStrings * GetFixedPaths() = 0;
  virtual void SpaceAvailable(const UnicodeString & Path,
    TSpaceAvailable & ASpaceAvailable) = 0;
  virtual const TSessionInfo & GetSessionInfo() = 0;
  virtual const TFileSystemInfo & GetFileSystemInfo(bool Retrieve) = 0;
  virtual bool TemporaryTransferFile(const UnicodeString & FileName) = 0;
  virtual bool GetStoredCredentialsTried() = 0;
  virtual UnicodeString GetUserName() = 0;
  virtual UnicodeString GetCurrentDirectory() = 0;

protected:
  TTerminal * FTerminal;

  explicit TCustomFileSystem(TTerminal * ATerminal);
};
//---------------------------------------------------------------------------
#endif
