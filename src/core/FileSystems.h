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
                  fsAnyCommand, fsReadSymlink, fsChangeProperties, fsMoveFile
                };
//---------------------------------------------------------------------------
const int dfNoRecursive = 0x01;
const int dfAlternative = 0x02;
const int dfForceDelete = 0x04;
//---------------------------------------------------------------------------
const int tfFirstLevel =   0x01;
const int tfAutoResume = 0x02;
const int tfNewDirectory = 0x04;
//---------------------------------------------------------------------------
const int ecRaiseExcept = 1;
const int ecIgnoreWarnings = 2;
const int ecReadProgress = 4;
const int ecDefault = ecRaiseExcept;
//---------------------------------------------------------------------------
struct TSinkFileParams
{
    std::wstring TargetDir;
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

    std::wstring FileName;
    const TCopyParamType *CopyParam;
    int Params;
    int OverwriteResult;
    bool AutoResume;
};
//---------------------------------------------------------------------------
struct TClipboardHandler
{
    std::wstring Text;

    void Copy(nb::TObject * /*Sender*/)
    {
        CopyToClipboard(Text);
    }
};
//---------------------------------------------------------------------------
struct TOpenRemoteFileParams
{
    TOpenRemoteFileParams() :
      OperationProgress(NULL),
      CopyParam(NULL),
      FileParams(NULL),
      DestFileSize(0),
      LocalFileAttrs(0),
      Params(0),
      OverwriteMode(omOverwrite),
      Resume(false),
      Resuming(false),
      Confirmed(false)
    {
    }
    TFileOperationProgressType *OperationProgress;
    const TCopyParamType *CopyParam;
    TOverwriteFileParams *FileParams;
    __int64 DestFileSize; // output
    int LocalFileAttrs;
    int Params;
    TOverwriteMode OverwriteMode;
    std::wstring RemoteFileName;
    std::string RemoteFileHandle; // output
    bool Resume;
    bool Resuming;
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
    explicit TCustomFileSystem()
    {}
    virtual void __fastcall Init()
    {}
    virtual ~TCustomFileSystem();

    virtual void __fastcall Open() = 0;
    virtual void __fastcall Close() = 0;
    virtual bool __fastcall GetActive() = 0;
    virtual void __fastcall Idle() = 0;
    virtual std::wstring __fastcall AbsolutePath(const std::wstring Path, bool Local) = 0;
    virtual void __fastcall AnyCommand(const std::wstring Command,
                            const captureoutput_slot_type *OutputEvent) = 0;
    virtual void __fastcall ChangeDirectory(const std::wstring Directory) = 0;
    virtual void __fastcall CachedChangeDirectory(const std::wstring Directory) = 0;
    virtual void __fastcall AnnounceFileListOperation() = 0;
    virtual void __fastcall ChangeFileProperties(const std::wstring FileName,
                                      const TRemoteFile *File, const TRemoteProperties *Properties,
                                      TChmodSessionAction &Action) = 0;
    virtual bool __fastcall LoadFilesProperties(nb::TStrings *FileList) = 0;
    virtual void __fastcall CalculateFilesChecksum(const std::wstring Alg,
                                        nb::TStrings *FileList, nb::TStrings *Checksums,
                                        calculatedchecksum_slot_type *OnCalculatedChecksum) = 0;
    virtual void __fastcall CopyToLocal(nb::TStrings *FilesToCopy,
                             const std::wstring TargetDir, const TCopyParamType *CopyParam,
                             int Params, TFileOperationProgressType *OperationProgress,
                             TOnceDoneOperation &OnceDoneOperation) = 0;
    virtual void __fastcall CopyToRemote(nb::TStrings *FilesToCopy,
                              const std::wstring TargetDir, const TCopyParamType *CopyParam,
                              int Params, TFileOperationProgressType *OperationProgress,
                              TOnceDoneOperation &OnceDoneOperation) = 0;
    virtual void __fastcall CreateDirectory(const std::wstring DirName) = 0;
    virtual void __fastcall CreateLink(const std::wstring FileName, const std::wstring PointTo, bool Symbolic) = 0;
    virtual void __fastcall DeleteFile(const std::wstring FileName,
                            const TRemoteFile *File, int Params,
                            TRmSessionAction &Action) = 0;
    virtual void __fastcall CustomCommandOnFile(const std::wstring FileName,
                                     const TRemoteFile *File, const std::wstring Command, int Params, const captureoutput_slot_type &OutputEvent) = 0;
    virtual void __fastcall DoStartup() = 0;
    virtual void __fastcall HomeDirectory() = 0;
    virtual bool __fastcall IsCapable(int Capability) const = 0;
    virtual void __fastcall LookupUsersGroups() = 0;
    virtual void __fastcall ReadCurrentDirectory() = 0;
    virtual void __fastcall ReadDirectory(TRemoteFileList *FileList) = 0;
    virtual void __fastcall ReadFile(const std::wstring FileName,
                          TRemoteFile *& File) = 0;
    virtual void __fastcall ReadSymlink(TRemoteFile *SymLinkFile,
                             TRemoteFile *& File) = 0;
    virtual void __fastcall RenameFile(const std::wstring FileName,
                            const std::wstring NewName) = 0;
    virtual void __fastcall CopyFile(const std::wstring FileName,
                          const std::wstring NewName) = 0;
    virtual std::wstring __fastcall FileUrl(const std::wstring FileName) = 0;
    virtual nb::TStrings * __fastcall GetFixedPaths() = 0;
    virtual void __fastcall SpaceAvailable(const std::wstring Path,
                                TSpaceAvailable &ASpaceAvailable) = 0;
    virtual const TSessionInfo & __fastcall GetSessionInfo() = 0;
    virtual const TFileSystemInfo & __fastcall GetFileSystemInfo(bool Retrieve) = 0;
    virtual bool __fastcall TemporaryTransferFile(const std::wstring FileName) = 0;
    virtual bool __fastcall GetStoredCredentialsTried() = 0;
    virtual std::wstring __fastcall GetUserName() = 0;

    virtual std::wstring __fastcall GetCurrentDirectory() = 0;

public:
    virtual void __fastcall FileTransferProgress(__int64 TransferSize, __int64 Bytes)
    {
    }

protected:
    TTerminal *FTerminal;

    explicit TCustomFileSystem(TTerminal *ATerminal);

    static void __fastcall FindCustomCommandPattern(
        const std::wstring Command, int Index, int &Len, char &PatternCmd);
};
//---------------------------------------------------------------------------
#endif
