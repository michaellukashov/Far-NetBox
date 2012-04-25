//---------------------------------------------------------------------------
#ifndef ScpFileSystemH
#define ScpFileSystemH

#include <FileSystems.h>
//---------------------------------------------------------------------------
class TCommandSet;
class TSecureShell;
//---------------------------------------------------------------------------
class TSCPFileSystem : public TCustomFileSystem
{
public:
    explicit TSCPFileSystem(TTerminal *ATerminal);
    virtual void __fastcall Init(TSecureShell *SecureShell);
    virtual ~TSCPFileSystem();

    virtual void __fastcall Open();
    virtual void __fastcall Close();
    virtual bool __fastcall GetActive();
    virtual void __fastcall Idle();
    virtual std::wstring __fastcall AbsolutePath(const std::wstring Path, bool Local);
    virtual void __fastcall AnyCommand(const std::wstring Command,
                            const TCaptureOutputEvent *OutputEvent);
    virtual void __fastcall ChangeDirectory(const std::wstring Directory);
    virtual void __fastcall CachedChangeDirectory(const std::wstring Directory);
    virtual void __fastcall AnnounceFileListOperation();
    virtual void __fastcall ChangeFileProperties(const std::wstring FileName,
                                      const TRemoteFile *File, const TRemoteProperties *Properties,
                                      TChmodSessionAction &Action);
    virtual bool __fastcall LoadFilesProperties(System::TStrings *FileList);
    virtual void __fastcall CalculateFilesChecksum(const std::wstring Alg,
                                        System::TStrings *FileList, System::TStrings *Checksums,
                                        calculatedchecksum_slot_type *OnCalculatedChecksum);
    virtual void __fastcall CopyToLocal(System::TStrings *FilesToCopy,
                             const std::wstring TargetDir, const TCopyParamType *CopyParam,
                             int Params, TFileOperationProgressType *OperationProgress,
                             TOnceDoneOperation &OnceDoneOperation);
    virtual void __fastcall CopyToRemote(System::TStrings *FilesToCopy,
                              const std::wstring TargetDir, const TCopyParamType *CopyParam,
                              int Params, TFileOperationProgressType *OperationProgress,
                              TOnceDoneOperation &OnceDoneOperation);
    virtual void __fastcall CreateDirectory(const std::wstring DirName);
    virtual void __fastcall CreateLink(const std::wstring FileName, const std::wstring PointTo, bool Symbolic);
    virtual void __fastcall DeleteFile(const std::wstring FileName,
                            const TRemoteFile *File, int Params, TRmSessionAction &Action);
    virtual void __fastcall CustomCommandOnFile(const std::wstring FileName,
                                     const TRemoteFile *File, const std::wstring Command, int Params, const TCaptureOutputEvent &OutputEvent);
    virtual void __fastcall DoStartup();
    virtual void __fastcall HomeDirectory();
    virtual bool __fastcall IsCapable(int Capability) const;
    virtual void __fastcall LookupUsersGroups();
    virtual void __fastcall ReadCurrentDirectory();
    virtual void __fastcall ReadDirectory(TRemoteFileList *FileList);
    virtual void __fastcall ReadFile(const std::wstring FileName,
                          TRemoteFile *& File);
    virtual void __fastcall ReadSymlink(TRemoteFile *SymlinkFile,
                             TRemoteFile *& File);
    virtual void __fastcall RenameFile(const std::wstring FileName,
                            const std::wstring NewName);
    virtual void __fastcall CopyFile(const std::wstring FileName,
                          const std::wstring NewName);
    virtual std::wstring __fastcall FileUrl(const std::wstring FileName);
    virtual System::TStrings * __fastcall GetFixedPaths();
    virtual void __fastcall SpaceAvailable(const std::wstring Path,
                                TSpaceAvailable &ASpaceAvailable);
    virtual const TSessionInfo & __fastcall GetSessionInfo();
    virtual const TFileSystemInfo & __fastcall GetFileSystemInfo(bool Retrieve);
    virtual bool __fastcall TemporaryTransferFile(const std::wstring FileName);
    virtual bool __fastcall GetStoredCredentialsTried();
    virtual std::wstring __fastcall GetUserName();

protected:
    System::TStrings * __fastcall GetOutput() { return FOutput; };
    int __fastcall GetReturnCode() { return FReturnCode; }

    virtual std::wstring __fastcall GetCurrentDirectory();

private:
    TSecureShell *FSecureShell;
    TCommandSet *FCommandSet;
    TFileSystemInfo FFileSystemInfo;
    std::wstring FCurrentDirectory;
    System::TStrings *FOutput;
    int FReturnCode;
    std::wstring FCachedDirectoryChange;
    bool FProcessingCommand;
    int FLsFullTime;
    captureoutput_signal_type FOnCaptureOutput;
    bool FUtfStrings;
    bool FUtfNever;
    TSCPFileSystem *Self;

    void __fastcall ClearAliases();
    void __fastcall ClearAlias(const std::wstring Alias);
    void __fastcall CustomReadFile(const std::wstring FileName,
                        TRemoteFile *& File, TRemoteFile *ALinkedByFile);
    static std::wstring __fastcall DelimitStr(const std::wstring Str);
    void __fastcall DetectReturnVar();
    bool __fastcall IsLastLine(std::wstring &Line);
    static bool __fastcall IsTotalListingLine(const std::wstring Line);
    void __fastcall EnsureLocation();
    void __fastcall ExecCommand(const std::wstring Cmd, int Params,
                     const std::wstring CmdString);
    void __fastcall ExecCommand(TFSCommand Cmd, int Params = -1, ...);
    void __fastcall ReadCommandOutput(int Params, const std::wstring *Cmd = NULL);
    void __fastcall SCPResponse(bool *GotLastLine = NULL);
    void __fastcall SCPDirectorySource(const std::wstring DirectoryName,
                            const std::wstring TargetDir, const TCopyParamType *CopyParam, int Params,
                            TFileOperationProgressType *OperationProgress, int Level);
    void __fastcall SCPError(const std::wstring Message, bool Fatal);
    void __fastcall SCPSendError(const std::wstring Message, bool Fatal);
    void __fastcall SCPSink(const std::wstring TargetDir,
                 const std::wstring FileName, const std::wstring SourceDir,
                 const TCopyParamType *CopyParam, bool &Success,
                 TFileOperationProgressType *OperationProgress, int Params, int Level);
    void __fastcall SCPSource(const std::wstring FileName,
                   const std::wstring TargetDir, const TCopyParamType *CopyParam, int Params,
                   TFileOperationProgressType *OperationProgress, int Level);
    void __fastcall SendCommand(const std::wstring Cmd);
    void __fastcall SkipFirstLine();
    void __fastcall SkipStartupMessage();
    void __fastcall UnsetNationalVars();
    TRemoteFile * __fastcall CreateRemoteFile(const std::wstring ListingStr,
                                  TRemoteFile *LinkedByFile = NULL);
    void CaptureOutput(const std::wstring AddedLine, bool StdError);
    void __fastcall ChangeFileToken(const std::wstring DelimitedName,
                         const TRemoteToken &Token, TFSCommand Cmd, const std::wstring RecursiveStr);

    static bool __fastcall RemoveLastLine(std::wstring &Line,
                               int &ReturnCode, std::wstring LastLine = L"");
private:
    TSCPFileSystem(const TSCPFileSystem &);
    void operator=(const TSCPFileSystem &);
};
//---------------------------------------------------------------------------
#endif // ScpFileSystemH
