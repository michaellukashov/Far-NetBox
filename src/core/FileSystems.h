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
class TCustomFileSystem
{
public:
    explicit TCustomFileSystem()
    {}
    virtual void Init()
    {}
    virtual ~TCustomFileSystem();

    virtual void Open() = 0;
    virtual void Close() = 0;
    virtual bool GetActive() = 0;
    virtual void Idle() = 0;
    virtual std::wstring AbsolutePath(const std::wstring Path, bool Local) = 0;
    virtual void AnyCommand(const std::wstring Command,
                            const captureoutput_slot_type *OutputEvent) = 0;
    virtual void ChangeDirectory(const std::wstring Directory) = 0;
    virtual void CachedChangeDirectory(const std::wstring Directory) = 0;
    virtual void AnnounceFileListOperation() = 0;
    virtual void ChangeFileProperties(const std::wstring FileName,
                                      const TRemoteFile *File, const TRemoteProperties *Properties,
                                      TChmodSessionAction &Action) = 0;
    virtual bool LoadFilesProperties(nb::TStrings *FileList) = 0;
    virtual void CalculateFilesChecksum(const std::wstring Alg,
                                        nb::TStrings *FileList, nb::TStrings *Checksums,
                                        calculatedchecksum_slot_type *OnCalculatedChecksum) = 0;
    virtual void CopyToLocal(nb::TStrings *FilesToCopy,
                             const std::wstring TargetDir, const TCopyParamType *CopyParam,
                             int Params, TFileOperationProgressType *OperationProgress,
                             TOnceDoneOperation &OnceDoneOperation) = 0;
    virtual void CopyToRemote(nb::TStrings *FilesToCopy,
                              const std::wstring TargetDir, const TCopyParamType *CopyParam,
                              int Params, TFileOperationProgressType *OperationProgress,
                              TOnceDoneOperation &OnceDoneOperation) = 0;
    virtual void CreateDirectory(const std::wstring DirName) = 0;
    virtual void CreateLink(const std::wstring FileName, const std::wstring PointTo, bool Symbolic) = 0;
    virtual void DeleteFile(const std::wstring FileName,
                            const TRemoteFile *File, int Params,
                            TRmSessionAction &Action) = 0;
    virtual void CustomCommandOnFile(const std::wstring FileName,
                                     const TRemoteFile *File, const std::wstring Command, int Params, const captureoutput_slot_type &OutputEvent) = 0;
    virtual void DoStartup() = 0;
    virtual void HomeDirectory() = 0;
    virtual bool IsCapable(int Capability) const = 0;
    virtual void LookupUsersGroups() = 0;
    virtual void ReadCurrentDirectory() = 0;
    virtual void ReadDirectory(TRemoteFileList *FileList) = 0;
    virtual void ReadFile(const std::wstring FileName,
                          TRemoteFile *& File) = 0;
    virtual void ReadSymlink(TRemoteFile *SymLinkFile,
                             TRemoteFile *& File) = 0;
    virtual void RenameFile(const std::wstring FileName,
                            const std::wstring NewName) = 0;
    virtual void CopyFile(const std::wstring FileName,
                          const std::wstring NewName) = 0;
    virtual std::wstring FileUrl(const std::wstring FileName) = 0;
    virtual nb::TStrings *GetFixedPaths() = 0;
    virtual void SpaceAvailable(const std::wstring Path,
                                TSpaceAvailable &ASpaceAvailable) = 0;
    virtual const TSessionInfo &GetSessionInfo() = 0;
    virtual const TFileSystemInfo &GetFileSystemInfo(bool Retrieve) = 0;
    virtual bool TemporaryTransferFile(const std::wstring FileName) = 0;
    virtual bool GetStoredCredentialsTried() = 0;
    virtual std::wstring GetUserName() = 0;

    // __property std::wstring CurrentDirectory = { read = GetCurrentDirectory };
    virtual std::wstring GetCurrentDirectory() = 0;

protected:
    TTerminal *FTerminal;

    explicit TCustomFileSystem(TTerminal *ATerminal);

    static void FindCustomCommandPattern(
        const std::wstring Command, int Index, int &Len, char &PatternCmd);
};
//---------------------------------------------------------------------------
#endif
