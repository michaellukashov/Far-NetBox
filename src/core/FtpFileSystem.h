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
struct TOverwriteFileParams;
struct TListDataEntry;
struct TFileTransferData;
struct TFtpsCertificateData;
//---------------------------------------------------------------------------
class TFTPFileSystem : public TCustomFileSystem
{
    friend class TFileZillaImpl;
    friend class TFileListHelper;

public:
    explicit TFTPFileSystem(TTerminal *ATerminal);
    virtual void Init();
    virtual ~TFTPFileSystem();

    virtual void Open();
    virtual void Close();
    virtual bool GetActive();
    virtual void Idle();
    virtual std::wstring AbsolutePath(const std::wstring Path, bool Local);
    virtual void AnyCommand(const std::wstring Command,
                            const captureoutput_slot_type *OutputEvent);
    virtual void ChangeDirectory(const std::wstring Directory);
    virtual void CachedChangeDirectory(const std::wstring Directory);
    virtual void AnnounceFileListOperation();
    virtual void ChangeFileProperties(const std::wstring FileName,
                                      const TRemoteFile *File, const TRemoteProperties *Properties,
                                      TChmodSessionAction &Action);
    virtual bool LoadFilesProperties(nb::TStrings *FileList);
    virtual void CalculateFilesChecksum(const std::wstring Alg,
                                        nb::TStrings *FileList, nb::TStrings *Checksums,
                                        calculatedchecksum_slot_type *OnCalculatedChecksum);
    virtual void CopyToLocal(nb::TStrings *FilesToCopy,
                             const std::wstring TargetDir, const TCopyParamType *CopyParam,
                             int Params, TFileOperationProgressType *OperationProgress,
                             TOnceDoneOperation &OnceDoneOperation);
    virtual void CopyToRemote(nb::TStrings *FilesToCopy,
                              const std::wstring TargetDir, const TCopyParamType *CopyParam,
                              int Params, TFileOperationProgressType *OperationProgress,
                              TOnceDoneOperation &OnceDoneOperation);
    virtual void CreateDirectory(const std::wstring DirName);
    virtual void CreateLink(const std::wstring FileName, const std::wstring PointTo, bool Symbolic);
    virtual void DeleteFile(const std::wstring FileName,
                            const TRemoteFile *File, int Params, TRmSessionAction &Action);
    virtual void CustomCommandOnFile(const std::wstring FileName,
                                     const TRemoteFile *File, const std::wstring Command, int Params, const captureoutput_slot_type &OutputEvent);
    virtual void DoStartup();
    virtual void HomeDirectory();
    virtual bool IsCapable(int Capability) const;
    virtual void LookupUsersGroups();
    virtual void ReadCurrentDirectory();
    virtual void ReadDirectory(TRemoteFileList *FileList);
    virtual void ReadFile(const std::wstring FileName,
                          TRemoteFile *& File);
    virtual void ReadSymlink(TRemoteFile *SymlinkFile,
                             TRemoteFile *& File);
    virtual void RenameFile(const std::wstring FileName,
                            const std::wstring NewName);
    virtual void CopyFile(const std::wstring FileName,
                          const std::wstring NewName);
    virtual std::wstring FileUrl(const std::wstring FileName);
    virtual nb::TStrings *GetFixedPaths();
    virtual void SpaceAvailable(const std::wstring Path,
                                TSpaceAvailable &ASpaceAvailable);
    virtual const TSessionInfo &GetSessionInfo();
    virtual const TFileSystemInfo &GetFileSystemInfo(bool Retrieve);
    virtual bool TemporaryTransferFile(const std::wstring FileName);
    virtual bool GetStoredCredentialsTried();
    virtual std::wstring GetUserName();

protected:
    enum TOverwriteMode { omOverwrite, omResume };

    virtual std::wstring GetCurrentDirectory();

    const wchar_t *GetOption(int OptionID) const;
    int GetOptionVal(int OptionID) const;

    enum
    {
        REPLY_CONNECT =      0x01,
        REPLY_2XX_CODE =     0x02,
        REPLY_ALLOW_CANCEL = 0x04
    };

    bool PostMessage(unsigned int Type, WPARAM wParam, LPARAM lParam);
    bool ProcessMessage();
    void DiscardMessages();
    void WaitForMessages();
    unsigned int WaitForReply(bool Command, bool WantLastCode);
    unsigned int WaitForCommandReply(bool WantLastCode = true);
    void WaitForFatalNonCommandReply();
    void PoolForFatalNonCommandReply();
    void GotNonCommandReply(unsigned int Reply);
    void GotReply(unsigned int Reply, unsigned int Flags = 0,
                  std::wstring Error = L"", unsigned int *Code = NULL,
                  nb::TStrings **Response = NULL);
    void ResetReply();
    void HandleReplyStatus(const std::wstring Response);
    void DoWaitForReply(unsigned int &ReplyToAwait, bool WantLastCode);
    bool KeepWaitingForReply(unsigned int &ReplyToAwait, bool WantLastCode);
    inline bool NoFinalLastCode();

    bool HandleStatus(const wchar_t *Status, int Type);
    bool HandleAsynchRequestOverwrite(
        wchar_t *FileName1, size_t FileName1Len, const wchar_t *FileName2,
        const wchar_t *Path1, const wchar_t *Path2,
        __int64 Size1, __int64 Size2, time_t Time1, time_t Time2,
        bool HasTime1, bool HasTime2, void *UserData, int &RequestResult);
    bool HandleAsynchRequestVerifyCertificate(
        const TFtpsCertificateData &Data, int &RequestResult);
    bool HandleListData(const wchar_t *Path, const TListDataEntry *Entries,
                        size_t Count);
    bool HandleTransferStatus(bool Valid, __int64 TransferSize,
                              __int64 Bytes, int Percent, int TimeElapsed, int TimeLeft, int TransferRate,
                              bool FileTransfer);
    bool HandleReply(int Command, unsigned int Reply);
    bool HandleCapabilities(bool Mfmt);
    bool CheckError(int ReturnCode, const wchar_t *Context);
    void EnsureLocation();
    std::wstring ActualCurrentDirectory();
    void Discard();
    void DoChangeDirectory(const std::wstring Directory);

    void Sink(const std::wstring FileName,
              const TRemoteFile *File, const std::wstring TargetDir,
              const TCopyParamType *CopyParam, int Params,
              TFileOperationProgressType *OperationProgress, unsigned int Flags,
              TDownloadSessionAction &Action);
    void SinkRobust(const std::wstring FileName,
                    const TRemoteFile *File, const std::wstring TargetDir,
                    const TCopyParamType *CopyParam, int Params,
                    TFileOperationProgressType *OperationProgress, unsigned int Flags);
    void SinkFile(const std::wstring FileName, const TRemoteFile *File, void *Param);
    void SourceRobust(const std::wstring FileName,
                      const std::wstring TargetDir, const TCopyParamType *CopyParam, int Params,
                      TFileOperationProgressType *OperationProgress, unsigned int Flags);
    void Source(const std::wstring FileName,
                const std::wstring TargetDir, const TCopyParamType *CopyParam, int Params,
                TFileOperationProgressType *OperationProgress, unsigned int Flags,
                TUploadSessionAction &Action);
    void DirectorySource(const std::wstring DirectoryName,
                         const std::wstring TargetDir, int Attrs, const TCopyParamType *CopyParam,
                         int Params, TFileOperationProgressType *OperationProgress, unsigned int Flags);
    bool ConfirmOverwrite(std::wstring &FileName,
                          TOverwriteMode &OverwriteMode, TFileOperationProgressType *OperationProgress,
                          const TOverwriteFileParams *FileParams, int Params, bool AutoResume);
    void ReadDirectoryProgress(__int64 Bytes);
    void ResetFileTransfer();
    void DoFileTransferProgress(__int64 TransferSize, __int64 Bytes);
    void FileTransferProgress(__int64 TransferSize, __int64 Bytes);
    void ResetCaches();
    void CaptureOutput(const std::wstring Str);
    void DoReadDirectory(TRemoteFileList *FileList);
    void FileTransfer(const std::wstring FileName, const std::wstring LocalFile,
                      const std::wstring RemoteFile, const std::wstring RemotePath, bool Get,
                      __int64 Size, int Type, TFileTransferData &UserData,
                      TFileOperationProgressType *OperationProgress);
    nb::TDateTime ConvertLocalTimestamp(time_t Time);
    nb::TDateTime ConvertRemoteTimestamp(time_t Time, bool HasTime);
    void SetLastCode(int Code);

    static bool Unquote(std::wstring &Str);
    static std::wstring ExtractStatusMessage(std::wstring &Status);

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
    int FLastReadDirectoryProgress;
    std::wstring FTimeoutStatus;
    std::wstring FDisconnectStatus;
    nb::TStrings *FLastResponse;
    nb::TStrings *FLastError;
    std::wstring FSystem;
    nb::TStrings *FFeatures;
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
    unsigned long FFileTransferCPSLimit;
    bool FAwaitingProgress;
    captureoutput_signal_type FOnCaptureOutput;
    std::wstring FUserName;
    TAutoSwitch FListAll;
    bool FDoListAll;
    bool FMfmt;
    nb::TDateTime FLastDataSent;
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
