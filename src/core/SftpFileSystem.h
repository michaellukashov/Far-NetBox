//---------------------------------------------------------------------------
#ifndef SftpFileSystemH
#define SftpFileSystemH

#include <FileSystems.h>
//---------------------------------------------------------------------------
class TSFTPPacket;
struct TOpenRemoteFileParams;
struct TOverwriteFileParams;
struct TSFTPSupport;
class TSecureShell;
//---------------------------------------------------------------------------
class TSFTPFileSystem : public TCustomFileSystem
{
    friend class TSFTPPacket;
    friend class TSFTPQueue;
    friend class TSFTPAsynchronousQueue;
    friend class TSFTPUploadQueue;
    friend class TSFTPDownloadQueue;
    friend class TSFTPLoadFilesPropertiesQueue;
    friend class TSFTPCalculateFilesChecksumQueue;
    friend class TSFTPBusy;
public:
    explicit TSFTPFileSystem(TTerminal *ATermina);
    virtual void __fastcall Init(TSecureShell *SecureShell);
    virtual ~TSFTPFileSystem();

    virtual void __fastcall Open();
    virtual void __fastcall Close();
    virtual bool __fastcall GetActive();
    virtual void __fastcall Idle();
    virtual std::wstring __fastcall AbsolutePath(const std::wstring Path, bool Local);
    virtual void __fastcall AnyCommand(const std::wstring Command,
                            const captureoutput_slot_type *OutputEvent);
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
                                     const TRemoteFile *File, const std::wstring Command, int Params, const captureoutput_slot_type &OutputEvent);
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
    TSecureShell *FSecureShell;
    TFileSystemInfo FFileSystemInfo;
    bool FFileSystemInfoValid;
    size_t FVersion;
    std::wstring FCurrentDirectory;
    std::wstring FDirectoryToChangeTo;
    std::wstring FHomeDirectory;
    std::wstring FEOL;
    System::TList *FPacketReservations;
    std::vector<size_t> FPacketNumbers;
    char FPreviousLoggedPacket;
    int FNotLoggedPackets;
    int FBusy;
    bool FAvoidBusy;
    System::TStrings *FExtensions;
    TSFTPSupport *FSupport;
    bool FUtfStrings;
    bool FUtfNever;
    bool FSignedTS;
    bool FOpenSSH;
    System::TStrings *FFixedPaths;
    size_t FMaxPacketSize;
    TSFTPFileSystem *Self;

    void __fastcall SendCustomReadFile(TSFTPPacket *Packet, TSFTPPacket *Response,
                            // const std::string RemoteHandle,
                            const std::wstring FileName,
                            size_t Flags);
    void __fastcall CustomReadFile(const std::wstring FileName,
                        TRemoteFile *& File, char Type, TRemoteFile *ALinkedByFile = NULL,
                        size_t AllowStatus = -1);
    virtual std::wstring __fastcall GetCurrentDirectory();
    std::wstring __fastcall GetHomeDirectory();
    size_t __fastcall GotStatusPacket(TSFTPPacket *Packet, size_t AllowStatus);
    bool inline IsAbsolutePath(const std::wstring Path);
    bool __fastcall RemoteFileExists(const std::wstring FullPath, TRemoteFile **File = NULL);
    TRemoteFile * __fastcall LoadFile(TSFTPPacket *Packet,
                          TRemoteFile *ALinkedByFile, const std::wstring FileName,
                          TRemoteFileList *TempFileList = NULL, bool Complete = true);
    void __fastcall LoadFile(TRemoteFile *File, TSFTPPacket *Packet,
                  bool Complete = true);
    std::wstring __fastcall LocalCanonify(const std::wstring Path);
    std::wstring __fastcall Canonify(const std::wstring Path);
    std::wstring __fastcall RealPath(const std::wstring Path);
    std::wstring __fastcall RealPath(const std::wstring Path, const std::wstring BaseDir);
    void __fastcall ReserveResponse(const TSFTPPacket *Packet,
                         TSFTPPacket *Response);
    size_t __fastcall ReceivePacket(TSFTPPacket *Packet, size_t ExpectedType = -1,
                      size_t AllowStatus = -1);
    bool __fastcall PeekPacket();
    void __fastcall RemoveReservation(size_t Reservation);
    void __fastcall SendPacket(const TSFTPPacket *Packet);
    size_t __fastcall ReceiveResponse(const TSFTPPacket *Packet,
                        TSFTPPacket *Response, size_t ExpectedType = -1, size_t AllowStatus = -1);
    size_t __fastcall SendPacketAndReceiveResponse(const TSFTPPacket *Packet,
                                        TSFTPPacket *Response, size_t ExpectedType = -1, size_t AllowStatus = -1);
    void __fastcall UnreserveResponse(TSFTPPacket *Response);
    void __fastcall TryOpenDirectory(const std::wstring Directory);
    bool __fastcall SupportsExtension(const std::wstring Extension) const;
    void __fastcall ResetConnection();
    void __fastcall DoCalculateFilesChecksum(const std::wstring Alg,
                                  System::TStrings *FileList, System::TStrings *Checksums,
                                  calculatedchecksum_slot_type *OnCalculatedChecksum,
                                  TFileOperationProgressType *OperationProgress, bool FirstLevel);
    void __fastcall DoDeleteFile(const std::wstring FileName, char Type);

    void __fastcall SFTPSourceRobust(const std::wstring FileName,
                          const std::wstring TargetDir, const TCopyParamType *CopyParam, int Params,
                          TFileOperationProgressType *OperationProgress, unsigned int Flags);
    void __fastcall SFTPSource(const std::wstring FileName,
                    const std::wstring TargetDir, const TCopyParamType *CopyParam, int Params,
                    TOpenRemoteFileParams *OpenParams,
                    TOverwriteFileParams *FileParams,
                    TFileOperationProgressType *OperationProgress,
                    unsigned int Flags,
                    TUploadSessionAction &Action, bool &ChildError);
    std::string __fastcall SFTPOpenRemoteFile(const std::wstring FileName,
                                   size_t OpenType, __int64 Size = -1);
    int SFTPOpenRemote(void *AOpenParams, void *Param2);
    void __fastcall SFTPCloseRemote(const std::string &Handle,
                         const std::wstring FileName, TFileOperationProgressType *OperationProgress,
                         bool TransferFinished, bool Request, TSFTPPacket *Packet);
    void __fastcall SFTPDirectorySource(const std::wstring DirectoryName,
                             const std::wstring TargetDir, int Attrs, const TCopyParamType *CopyParam,
                             int Params, TFileOperationProgressType *OperationProgress, unsigned int Flags);
    void __fastcall SFTPConfirmOverwrite(std::wstring &FileName,
                              int Params, TFileOperationProgressType *OperationProgress,
                              TOverwriteMode &Mode, const TOverwriteFileParams *FileParams);
    bool __fastcall SFTPConfirmResume(const std::wstring DestFileName, bool PartialBiggerThanSource,
                           TFileOperationProgressType *OperationProgress);
    void __fastcall SFTPSinkRobust(const std::wstring FileName,
                        const TRemoteFile *File, const std::wstring TargetDir,
                        const TCopyParamType *CopyParam, int Params,
                        TFileOperationProgressType *OperationProgress, unsigned int Flags);
    void __fastcall SFTPSink(const std::wstring FileName,
                  const TRemoteFile *File, const std::wstring TargetDir,
                  const TCopyParamType *CopyParam, int Params,
                  TFileOperationProgressType *OperationProgress, unsigned int Flags,
                  TDownloadSessionAction &Action, bool &ChildError);
    void SFTPSinkFile(const std::wstring FileName,
                      const TRemoteFile *File, void *Param);
    char * __fastcall GetEOL() const;
    inline void __fastcall BusyStart();
    inline void __fastcall BusyEnd();
    size_t __fastcall TransferBlockSize(size_t Overhead,
                             TFileOperationProgressType *OperationProgress,
                             size_t MinPacketSize = 0,
                             size_t MaxPacketSize = 0);
    size_t __fastcall UploadBlockSize(const std::string &Handle,
                           TFileOperationProgressType *OperationProgress);
    size_t __fastcall DownloadBlockSize(
        TFileOperationProgressType *OperationProgress);
    size_t __fastcall PacketLength(char *LenBuf, size_t ExpectedType);

private:
    const TSessionData * __fastcall GetSessionData() const;
};
//---------------------------------------------------------------------------
#endif // SftpFileSystemH
