//---------------------------------------------------------------------------
#ifndef FtpFileSystemH
#define FtpFileSystemH

#ifndef NO_FILEZILLA
//---------------------------------------------------------------------------
#include <time.h>
#include <FileSystems.h>
//---------------------------------------------------------------------------
class TFileZillaIntf;
class TFileZillaImpl;
class TCriticalSection;
class TMessageQueue;
class TFTPServerCapabilities;
struct TOverwriteFileParams;
struct TListDataEntry;
struct TFtpsCertificateData;
//---------------------------------------------------------------------------
class TFTPFileSystem : public TCustomFileSystem
{
    friend class TFileZillaImpl;
    friend class TFTPFileListHelper;

public:
    explicit TFTPFileSystem(TTerminal *ATerminal);
    virtual ~TFTPFileSystem();
    virtual void __fastcall Init();

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

public:
    virtual void __fastcall FileTransferProgress(__int64 TransferSize, __int64 Bytes);

protected:
    virtual std::wstring __fastcall GetCurrentDirectory();

    const wchar_t * __fastcall GetOption(int OptionID) const;
    int __fastcall GetOptionVal(int OptionID) const;

    enum
    {
        REPLY_CONNECT =      0x01,
        REPLY_2XX_CODE =     0x02,
        REPLY_ALLOW_CANCEL = 0x04
    };

    bool __fastcall PostMessage(unsigned int Type, WPARAM wParam, LPARAM lParam);
    bool __fastcall ProcessMessage();
    void __fastcall DiscardMessages();
    void __fastcall WaitForMessages();
    unsigned int __fastcall WaitForReply(bool Command, bool WantLastCode);
    unsigned int __fastcall WaitForCommandReply(bool WantLastCode = true);
    void __fastcall WaitForFatalNonCommandReply();
    void __fastcall PoolForFatalNonCommandReply();
    void __fastcall GotNonCommandReply(unsigned int Reply);
    void __fastcall GotReply(unsigned int Reply, unsigned int Flags = 0,
                  std::wstring Error = L"", unsigned int *Code = NULL,
                  System::TStrings **Response = NULL);
    void __fastcall ResetReply();
    void __fastcall HandleReplyStatus(const std::wstring Response);
    void __fastcall DoWaitForReply(unsigned int &ReplyToAwait, bool WantLastCode);
    bool __fastcall KeepWaitingForReply(unsigned int &ReplyToAwait, bool WantLastCode);
    inline bool __fastcall NoFinalLastCode();

    bool __fastcall HandleStatus(const wchar_t *Status, int Type);
    bool __fastcall HandleAsynchRequestOverwrite(
        wchar_t *FileName1, size_t FileName1Len, const wchar_t *FileName2,
        const wchar_t *Path1, const wchar_t *Path2,
        __int64 Size1, __int64 Size2, time_t Time1, time_t Time2,
        bool HasTime1, bool HasTime2, void *UserData, int &RequestResult);
    bool __fastcall HandleAsynchRequestVerifyCertificate(
        const TFtpsCertificateData &Data, int &RequestResult);
    bool __fastcall HandleListData(const wchar_t *Path, const TListDataEntry *Entries,
                        size_t Count);
    bool __fastcall HandleTransferStatus(bool Valid, __int64 TransferSize,
                              __int64 Bytes, int Percent, int TimeElapsed, int TimeLeft, int TransferRate,
                              bool FileTransfer);
    bool __fastcall HandleReply(int Command, unsigned int Reply);
    bool __fastcall HandleCapabilities(TFTPServerCapabilities *ServerCapabilities);
    bool __fastcall CheckError(int ReturnCode, const wchar_t *Context);
    void __fastcall EnsureLocation();
    std::wstring __fastcall ActualCurrentDirectory();
    void __fastcall Discard();
    void __fastcall DoChangeDirectory(const std::wstring Directory);

    void __fastcall Sink(const std::wstring FileName,
              const TRemoteFile *File, const std::wstring TargetDir,
              const TCopyParamType *CopyParam, int Params,
              TFileOperationProgressType *OperationProgress, unsigned int Flags,
              TDownloadSessionAction &Action);
    void __fastcall SinkRobust(const std::wstring FileName,
                    const TRemoteFile *File, const std::wstring TargetDir,
                    const TCopyParamType *CopyParam, int Params,
                    TFileOperationProgressType *OperationProgress, unsigned int Flags);
    void SinkFile(const std::wstring FileName, const TRemoteFile *File, void *Param);
    void __fastcall SourceRobust(const std::wstring FileName,
                      const std::wstring TargetDir, const TCopyParamType *CopyParam, int Params,
                      TFileOperationProgressType *OperationProgress, unsigned int Flags);
    void __fastcall Source(const std::wstring FileName,
                const std::wstring TargetDir, const TCopyParamType *CopyParam, int Params,
                TOpenRemoteFileParams *OpenParams,
                TOverwriteFileParams *FileParams,
                TFileOperationProgressType *OperationProgress, unsigned int Flags,
                TUploadSessionAction &Action);
    void __fastcall DirectorySource(const std::wstring DirectoryName,
                         const std::wstring TargetDir, int Attrs, const TCopyParamType *CopyParam,
                         int Params, TFileOperationProgressType *OperationProgress, unsigned int Flags);
    bool __fastcall ConfirmOverwrite(std::wstring &FileName,
                          int Params, TFileOperationProgressType *OperationProgress,
                          TOverwriteMode &OverwriteMode,
                          bool AutoResume,
                          const TOverwriteFileParams *FileParams);
    void __fastcall ReadDirectoryProgress(__int64 Bytes);
    void __fastcall ResetFileTransfer();
    void __fastcall DoFileTransferProgress(__int64 TransferSize, __int64 Bytes);
    void __fastcall ResetCaches();
    void __fastcall CaptureOutput(const std::wstring Str);
    void __fastcall DoReadDirectory(TRemoteFileList *FileList);
    void __fastcall DoReadFile(const std::wstring FileName, TRemoteFile *& AFile);
    void __fastcall FileTransfer(const std::wstring FileName, const std::wstring LocalFile,
                      const std::wstring RemoteFile, const std::wstring RemotePath, bool Get,
                      __int64 Size, int Type, TFileTransferData &UserData,
                      TFileOperationProgressType *OperationProgress);
    System::TDateTime __fastcall ConvertLocalTimestamp(time_t Time);
    System::TDateTime __fastcall ConvertRemoteTimestamp(time_t Time, bool HasTime);
    void __fastcall SetLastCode(int Code);

    static bool __fastcall Unquote(std::wstring &Str);
    static std::wstring __fastcall ExtractStatusMessage(std::wstring &Status);

private:
    enum TCommand
    {
        CMD_UNKNOWN,
        PASS,
        SYST,
        FEAT
    };

    TFileZillaIntf *FFileZillaIntf;
    TCriticalSection *FQueueCriticalSection;
    TCriticalSection *FTransferStatusCriticalSection;
    TMessageQueue *FQueue;
    HANDLE FQueueEvent;
    TSessionInfo FSessionInfo;
    TFileSystemInfo FFileSystemInfo;
    bool FFileSystemInfoValid;
    unsigned int FReply;
    unsigned int FCommandReply;
    TCommand FLastCommand;
    bool FPasswordFailed;
    bool FMultineResponse;
    int FLastCode;
    int FLastCodeClass;
    size_t FLastReadDirectoryProgress;
    std::wstring FTimeoutStatus;
    std::wstring FDisconnectStatus;
    System::TStrings *FLastResponse;
    System::TStrings *FLastError;
    std::wstring FSystem;
    System::TStrings *FFeatures;
    std::wstring FCurrentDirectory;
    std::wstring FHomeDirectory;
    TRemoteFileList *FFileList;
    TRemoteFileList *FFileListCache;
    std::wstring FFileListCachePath;
    bool FActive;
    bool FWaitingForReply;
    enum { ftaNone, ftaSkip, ftaCancel } FFileTransferAbort;
    bool FIgnoreFileList;
    bool FFileTransferCancelled;
    __int64 FFileTransferResumed;
    bool FFileTransferPreserveTime;
    size_t FFileTransferCPSLimit;
    bool FAwaitingProgress;
    captureoutput_signal_type FOnCaptureOutput;
    std::wstring FUserName;
    TAutoSwitch FListAll;
    bool FDoListAll;
    TFTPServerCapabilities *FServerCapabilities;
    System::TDateTime FLastDataSent;
    mutable std::wstring FOptionScratch;
    TFTPFileSystem *Self;
private:
    TFTPFileSystem(const TFTPFileSystem &);
    void operator=(const TFTPFileSystem &);
};
//---------------------------------------------------------------------------
#endif NO_FILEZILLA
//---------------------------------------------------------------------------
#endif // FtpFileSystemH
