//---------------------------------------------------------------------------
#ifndef FileSystemsH
#define FileSystemsH

#include <SessionInfo.h>
#include "Exceptions.h"
#include "Terminal.h"
//---------------------------------------------------------------------------
class TTerminal;
class TRights;
class TRemoteFile;
class TRemoteFileList;
class TCopyParamType;
struct TSpaceAvailable;
class TFileOperationProgressType;
class TRemoteProperties;
class TSecureShell;
//---------------------------------------------------------------------------
enum TOverwriteMode { omOverwrite, omAppend, omResume };
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
const int tfFirstLevel =   0x01;
const int tfAutoResume = 0x02;
const int tfNewDirectory = 0x04;
//---------------------------------------------------------------------------
struct TSinkFileParams
{
  UnicodeString TargetDir;
  const TCopyParamType *CopyParam;
  TFileOperationProgressType *OperationProgress;
  int Params;
  unsigned int Flags;
  bool Skipped;
};
//---------------------------------------------------------------------------
struct TFileTransferData
{
  TFileTransferData() :
    CopyParam(NULL),
    Params(0),
    OverwriteResult(-1),
    AutoResume(false)
  {
  }

  UnicodeString FileName;
  const TCopyParamType *CopyParam;
  int Params;
  int OverwriteResult;
  bool AutoResume;
};
//---------------------------------------------------------------------------
struct TClipboardHandler
{
  UnicodeString Text;

  void Copy(TObject * /*Sender*/)
  {
      CopyToClipboard(Text);
  }
};
//---------------------------------------------------------------------------
struct TOpenRemoteFileParams
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
  int LocalFileAttrs;
  UnicodeString RemoteFileName;
  TFileOperationProgressType * OperationProgress;
  const TCopyParamType * CopyParam;
  int Params;
  bool Resume;
  bool Resuming;
  TOverwriteMode OverwriteMode;
  __int64 DestFileSize; // output
  RawByteString RemoteFileHandle; // output
  TOverwriteFileParams *FileParams;
  bool Confirmed;
};
//---------------------------------------------------------------------------

/** @brief interface for custom filesystems
  *
  */
class TFileSystemIntf
{
public:
  virtual ~TFileSystemIntf()
  {}

  virtual void __fastcall FileTransferProgress(__int64 TransferSize, __int64 Bytes) = 0;
};

//---------------------------------------------------------------------------
class TCustomFileSystem : public TFileSystemIntf
{
public:
  // explicit /* __fastcall */ TCustomFileSystem() {}
  virtual /* __fastcall */ ~TCustomFileSystem();

  virtual void __fastcall Open() = 0;
  virtual void __fastcall Close() = 0;
  virtual bool __fastcall GetActive() = 0;
  virtual void __fastcall Idle() = 0;
  virtual UnicodeString __fastcall AbsolutePath(UnicodeString Path, bool Local) = 0;
  virtual void __fastcall AnyCommand(const UnicodeString Command,
    TCaptureOutputEvent OutputEvent) = 0;
  virtual void __fastcall ChangeDirectory(const UnicodeString Directory) = 0;
  virtual void __fastcall CachedChangeDirectory(const UnicodeString Directory) = 0;
  virtual void __fastcall AnnounceFileListOperation() = 0;
  virtual void __fastcall ChangeFileProperties(const UnicodeString FileName,
    const TRemoteFile * File, const TRemoteProperties * Properties,
    TChmodSessionAction & Action) = 0;
  virtual bool __fastcall LoadFilesProperties(TStrings * FileList) = 0;
  virtual void __fastcall CalculateFilesChecksum(const UnicodeString & Alg,
    TStrings * FileList, TStrings * Checksums,
    TCalculatedChecksumEvent OnCalculatedChecksum) = 0;
  virtual void __fastcall CopyToLocal(TStrings * FilesToCopy,
    const UnicodeString TargetDir, const TCopyParamType * CopyParam,
    int Params, TFileOperationProgressType * OperationProgress,
    TOnceDoneOperation & OnceDoneOperation) = 0;
  virtual void __fastcall CopyToRemote(TStrings * FilesToCopy,
    const UnicodeString TargetDir, const TCopyParamType * CopyParam,
    int Params, TFileOperationProgressType * OperationProgress,
    TOnceDoneOperation & OnceDoneOperation) = 0;
  virtual void __fastcall CreateDirectory(const UnicodeString DirName) = 0;
  virtual void __fastcall CreateLink(const UnicodeString FileName, const UnicodeString PointTo, bool Symbolic) = 0;
  virtual void __fastcall DeleteFile(const UnicodeString FileName,
    const TRemoteFile * File, int Params,
    TRmSessionAction & Action) = 0;
  virtual void /* __fastcall */ CustomCommandOnFile(const UnicodeString FileName,
    const TRemoteFile * File, UnicodeString Command, int Params, TCaptureOutputEvent OutputEvent) = 0;
  virtual void __fastcall DoStartup() = 0;
  virtual void __fastcall HomeDirectory() = 0;
  virtual bool __fastcall IsCapable(int Capability) const = 0;
  virtual void __fastcall LookupUsersGroups() = 0;
  virtual void __fastcall ReadCurrentDirectory() = 0;
  virtual void __fastcall ReadDirectory(TRemoteFileList * FileList) = 0;
  virtual void __fastcall ReadFile(const UnicodeString FileName,
    TRemoteFile *& File) = 0;
  virtual void __fastcall ReadSymlink(TRemoteFile * SymLinkFile,
    TRemoteFile *& File) = 0;
  virtual void __fastcall RenameFile(const UnicodeString FileName,
    const UnicodeString NewName) = 0;
  virtual void __fastcall CopyFile(const UnicodeString FileName,
    const UnicodeString NewName) = 0;
  virtual UnicodeString __fastcall FileUrl(const UnicodeString FileName) = 0;
  virtual TStrings * __fastcall GetFixedPaths() = 0;
  virtual void __fastcall SpaceAvailable(const UnicodeString Path,
    TSpaceAvailable & ASpaceAvailable) = 0;
  virtual const TSessionInfo & __fastcall GetSessionInfo() = 0;
  virtual const TFileSystemInfo & __fastcall GetFileSystemInfo(bool Retrieve) = 0;
  virtual bool __fastcall TemporaryTransferFile(const UnicodeString & FileName) = 0;
  virtual bool __fastcall GetStoredCredentialsTried() = 0;
  virtual UnicodeString __fastcall GetUserName() = 0;

#ifndef _MSC_VER
  __property UnicodeString CurrentDirectory = { read = GetCurrentDirectory };
#else
  virtual UnicodeString __fastcall GetCurrentDirectory() = 0;
#endif

public:
  virtual void __fastcall FileTransferProgress(__int64 TransferSize, __int64 Bytes) {}

protected:
  TTerminal * FTerminal;

  explicit /* __fastcall */ TCustomFileSystem(TTerminal * ATerminal);
  virtual void __fastcall Init() {}
#ifndef _MSC_VER
  virtual UnicodeString __fastcall GetCurrentDirectory() = 0;
#endif

  static void __fastcall FindCustomCommandPattern(
    const UnicodeString Command, int Index, int &Len, char &PatternCmd);
};
//---------------------------------------------------------------------------
#endif
