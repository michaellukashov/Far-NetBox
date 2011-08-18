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
struct TCopyParamType;
struct TSpaceAvailable;
class TFileOperationProgressType;
class TRemoteProperties;
//---------------------------------------------------------------------------
enum TFSCommand { fsNull = 0, fsVarValue, fsLastLine, fsFirstLine,
  fsCurrentDirectory, fsChangeDirectory, fsListDirectory, fsListCurrentDirectory,
  fsListFile, fsLookupUsersGroups, fsCopyToRemote, fsCopyToLocal, fsDeleteFile,
  fsRenameFile, fsCreateDirectory, fsChangeMode, fsChangeGroup, fsChangeOwner,
  fsHomeDirectory, fsUnset, fsUnalias, fsCreateLink, fsCopyFile,
  fsAnyCommand, fsReadSymlink, fsChangeProperties, fsMoveFile };
//---------------------------------------------------------------------------
const int dfNoRecursive = 0x01;
const int dfAlternative = 0x02;
const int dfForceDelete = 0x04;
//---------------------------------------------------------------------------
class TCustomFileSystem
{
public:
  virtual ~TCustomFileSystem();

  virtual void Open() = 0;
  virtual void Close() = 0;
  virtual bool GetActive() = 0;
  virtual void Idle() = 0;
  virtual wstring AbsolutePath(wstring Path, bool Local) = 0;
  virtual void AnyCommand(const wstring Command,
    TCaptureOutputEvent OutputEvent) = 0;
  virtual void ChangeDirectory(const wstring Directory) = 0;
  virtual void CachedChangeDirectory(const wstring Directory) = 0;
  virtual void AnnounceFileListOperation() = 0;
  virtual void ChangeFileProperties(const wstring FileName,
    const TRemoteFile * File, const TRemoteProperties * Properties,
    TChmodSessionAction & Action) = 0;
  virtual bool LoadFilesProperties(TStrings * FileList) = 0;
  virtual void CalculateFilesChecksum(const wstring & Alg,
    TStrings * FileList, TStrings * Checksums,
    TCalculatedChecksumEvent OnCalculatedChecksum) = 0;
  virtual void CopyToLocal(TStrings * FilesToCopy,
    const wstring TargetDir, const TCopyParamType * CopyParam,
    int Params, TFileOperationProgressType * OperationProgress,
    TOnceDoneOperation & OnceDoneOperation) = 0;
  virtual void CopyToRemote(TStrings * FilesToCopy,
    const wstring TargetDir, const TCopyParamType * CopyParam,
    int Params, TFileOperationProgressType * OperationProgress,
    TOnceDoneOperation & OnceDoneOperation) = 0;
  virtual void CreateDirectory(const wstring DirName) = 0;
  virtual void CreateLink(const wstring FileName, const wstring PointTo, bool Symbolic) = 0;
  virtual void DeleteFile(const wstring FileName,
    const TRemoteFile * File, int Params,
    TRmSessionAction & Action) = 0;
  virtual void CustomCommandOnFile(const wstring FileName,
    const TRemoteFile * File, wstring Command, int Params, TCaptureOutputEvent OutputEvent) = 0;
  virtual void DoStartup() = 0;
  virtual void HomeDirectory() = 0;
  virtual bool IsCapable(int Capability) const = 0;
  virtual void LookupUsersGroups() = 0;
  virtual void ReadCurrentDirectory() = 0;
  virtual void ReadDirectory(TRemoteFileList * FileList) = 0;
  virtual void ReadFile(const wstring FileName,
    TRemoteFile *& File) = 0;
  virtual void ReadSymlink(TRemoteFile * SymLinkFile,
    TRemoteFile *& File) = 0;
  virtual void RenameFile(const wstring FileName,
    const wstring NewName) = 0;
  virtual void CopyFile(const wstring FileName,
    const wstring NewName) = 0;
  virtual wstring FileUrl(const wstring FileName) = 0;
  virtual TStrings * GetFixedPaths() = 0;
  virtual void SpaceAvailable(const wstring Path,
    TSpaceAvailable & ASpaceAvailable) = 0;
  virtual const TSessionInfo & GetSessionInfo() = 0;
  virtual const TFileSystemInfo & GetFileSystemInfo(bool Retrieve) = 0;
  virtual bool TemporaryTransferFile(const wstring & FileName) = 0;
  virtual bool GetStoredCredentialsTried() = 0;
  virtual wstring GetUserName() = 0;

  // __property wstring CurrentDirectory = { read = GetCurrentDirectory };
  virtual wstring GetCurrentDirectory() = 0;

protected:
  TTerminal * FTerminal;

  TCustomFileSystem(TTerminal * ATerminal);

  static void FindCustomCommandPattern(
    const wstring & Command, int Index, int & Len, char & PatternCmd);
};
//---------------------------------------------------------------------------
#endif
