#ifndef TerminalH
#define TerminalH

#include "boostdefines.hpp"
#include <boost/signals/signal5.hpp>
#include <boost/signals/signal8.hpp>

#include "Classes.h"

#include "SessionInfo.h"
#include "Interface.h"
#include "FileOperationProgress.h"
#include "FileMasks.h"
#include "Exceptions.h"
//---------------------------------------------------------------------------
class EFileNotFoundError : public std::exception
{
public:
    EFileNotFoundError() : std::exception()
    {
    }
};

//---------------------------------------------------------------------------
class TCopyParamType;
class TFileOperationProgressType;
class TRemoteDirectory;
class TRemoteFile;
class TCustomFileSystem;
class TTunnelThread;
class TSecureShell;
struct TCalculateSizeParams;
struct TOverwriteFileParams;
struct TSynchronizeData;
struct TSynchronizeOptions;
class TSynchronizeChecklist;
struct TCalculateSizeStats;
struct TFileSystemInfo;
struct TSpaceAvailable;
struct TFilesFindParams;
class TTunnelUI;
class TCallbackGuard;
//---------------------------------------------------------------------------
typedef boost::signal8<void, nb::TObject *, const std::wstring, nb::TStrings *, int,
        const TQueryParams *, int &, TQueryType, void *> queryuser_signal_type;
typedef queryuser_signal_type::slot_type queryuser_slot_type;
typedef boost::signal8<void, TTerminal *, TPromptKind, const std::wstring, std::wstring,
        nb::TStrings *, nb::TStrings *, bool &, void *> promptuser_signal_type;
typedef promptuser_signal_type::slot_type promptuser_slot_type;
typedef boost::signal5<void, TTerminal *, const std::wstring, const std::wstring,
        bool &, int> displaybanner_signal_type;
typedef displaybanner_signal_type::slot_type displaybanner_slot_type;
typedef boost::signal3<void, TTerminal *, const std::exception *, void *> extendedexception_signal_type;
typedef extendedexception_signal_type::slot_type extendedexception_slot_type;
typedef boost::signal2<void, nb::TObject *, bool> readdirectory_signal_type;
typedef readdirectory_signal_type::slot_type readdirectory_slot_type;
typedef boost::signal3<void, nb::TObject *, int, bool &> readdirectoryprogress_signal_type;
typedef readdirectoryprogress_signal_type::slot_type readdirectoryprogress_slot_type;
typedef boost::signal3<void, const std::wstring, const TRemoteFile *, void *> processfile_signal_type;
typedef processfile_signal_type::slot_type processfile_slot_type;
typedef boost::signal4<void, const std::wstring, const TRemoteFile *, void *, int> processfileex_signal_type;
typedef processfileex_signal_type::slot_type processfileex_slot_type;
typedef boost::signal2<int, void *, void *> fileoperation_signal_type;
typedef fileoperation_signal_type::slot_type fileoperation_slot_type;
typedef boost::signal4<void, const std::wstring, const std::wstring,
        bool &, bool> synchronizedirectory_signal_type;
typedef synchronizedirectory_signal_type::slot_type synchronizedirectory_slot_type;
typedef boost::signal2<void, const std::wstring, bool> deletelocalfile_signal_type;
typedef deletelocalfile_signal_type::slot_type deletelocalfile_slot_type;
typedef boost::signal3<int, TTerminal *, const std::wstring, bool> directorymodified_signal_type;
typedef directorymodified_signal_type::slot_type directorymodified_slot_type;
typedef boost::signal4<void, TTerminal *, const std::wstring, bool, bool> informationevent_signal_type;
typedef informationevent_signal_type::slot_type informationevent_slot_type;
//---------------------------------------------------------------------------
#define SUSPEND_OPERATION(Command)                            \
  {                                                           \
    TSuspendFileOperationProgress Suspend(OperationProgress); \
    Command                                                   \
  }

#define THROW_SKIP_FILE(MESSAGE, EXCEPTION) \
  throw EScpSkipFile(MESSAGE, EXCEPTION)
#define THROW_SKIP_FILE_NULL THROW_SKIP_FILE(L"", NULL)

/* TODO : Better user interface (query to user) */
#define FILE_OPERATION_LOOP_CUSTOM(TERMINAL, ALLOW_SKIP, MESSAGE, OPERATION) { \
  bool DoRepeat; \
  do { \
    DoRepeat = false; \
    try { \
      OPERATION;                                                            \
    }                                                                       \
    catch (const nb::EAbort &)                                                      \
    {                                                                       \
      throw;                                                                \
    }                                                                       \
    catch (const EScpSkipFile &)                                                \
    {                                                                       \
      throw;                                                                \
    }                                                                       \
    catch (const EFatal &)                                                      \
    {                                                                       \
      throw;                                                                \
    }                                                                       \
    catch (const EFileNotFoundError &)                                                      \
    {                                                                       \
      throw;                                                                \
    }                                                                       \
    catch (const EOSError &)                                                      \
    {                                                                       \
      throw;                                                                \
    }                                                                       \
    catch (const std::exception &E)                                                   \
    {                                                                       \
      TERMINAL->FileOperationLoopQuery(E, OperationProgress, MESSAGE, ALLOW_SKIP); \
      DoRepeat = true;                                                      \
    } \
  } while (DoRepeat); }

#define FILE_OPERATION_LOOP(MESSAGE, OPERATION) \
  FILE_OPERATION_LOOP_EX(true, MESSAGE, OPERATION)
//---------------------------------------------------------------------------
enum TCurrentFSProtocol { cfsUnknown, cfsSCP, cfsSFTP, cfsFTP, cfsFTPS, cfsHTTP, cfsHTTPS };
//---------------------------------------------------------------------------
const int cpDelete = 0x01;
const int cpTemporary = 0x04;
const int cpNoConfirmation = 0x08;
const int cpNewerOnly = 0x10;
const int cpAppend = 0x20;
const int cpResume = 0x40;
//---------------------------------------------------------------------------
const int ccApplyToDirectories = 0x01;
const int ccRecursive = 0x02;
const int ccUser = 0x100;
//---------------------------------------------------------------------------
const int csIgnoreErrors = 0x01;
//---------------------------------------------------------------------------
const int ropNoReadDirectory = 0x02;
//---------------------------------------------------------------------------
const int boDisableNeverShowAgain = 0x01;
//---------------------------------------------------------------------------
class TTerminal : public nb::TObject, public TSessionUI
{
public:
    // TScript::SynchronizeProc relies on the order
    enum TSynchronizeMode { smRemote, smLocal, smBoth };
    static const int spDelete = 0x01; // cannot be combined with spTimestamp
    static const int spNoConfirmation = 0x02; // has no effect for spTimestamp
    static const int spExistingOnly = 0x04; // is implicit for spTimestamp
    static const int spNoRecurse = 0x08;
    static const int spUseCache = 0x10; // cannot be combined with spTimestamp
    static const int spDelayProgress = 0x20; // cannot be combined with spTimestamp
    static const int spPreviewChanges = 0x40; // not used by core
    static const int spSubDirs = 0x80; // cannot be combined with spTimestamp
    static const int spTimestamp = 0x100;
    static const int spNotByTime = 0x200; // cannot be combined with spTimestamp and smBoth
    static const int spBySize = 0x400; // cannot be combined with smBoth, has opposite meaning for spTimestamp
    // 0x800 is reserved for GUI (spSelectedOnly)
    static const int spMirror = 0x1000;

// for TranslateLockedPath()
    friend class TRemoteFile;
// for ReactOnCommand()
    friend class TSCPFileSystem;
    friend class TSFTPFileSystem;
    friend class TFTPFileSystem;
    friend class TWebDAVFileSystem;
    friend class TTunnelUI;
    friend class TCallbackGuard;

private:
    TSessionData *FSessionData;
    TSessionLog *FLog;
    TConfiguration *FConfiguration;
    std::wstring FCurrentDirectory;
    std::wstring FLockDirectory;
    int FExceptionOnFail;
    TRemoteDirectory *FFiles;
    int FInTransaction;
    bool FSuspendTransaction;
    nb::notify_signal_type FOnChangeDirectory;
    readdirectory_signal_type FOnReadDirectory;
    nb::notify_signal_type FOnStartReadDirectory;
    readdirectoryprogress_signal_type FOnReadDirectoryProgress;
    deletelocalfile_signal_type FOnDeleteLocalFile;
    TRemoteTokenList FMembership;
    TRemoteTokenList FGroups;
    TRemoteTokenList FUsers;
    bool FUsersGroupsLookedup;
    fileoperationprogress_signal_type FOnProgress;
    fileoperationfinished_signal_type FOnFinished;
    TFileOperationProgressType *FOperationProgress;
    bool FUseBusyCursor;
    TRemoteDirectoryCache *FDirectoryCache;
    TRemoteDirectoryChangesCache *FDirectoryChangesCache;
    TCustomFileSystem *FFileSystem;
    TSecureShell *FSecureShell;
    std::wstring FLastDirectoryChange;
    TCurrentFSProtocol FFSProtocol;
    TTerminal *FCommandSession;
    bool FAutoReadDirectory;
    bool FReadingCurrentDirectory;
    bool *FClosedOnCompletion;
    TSessionStatus FStatus;
    std::wstring FPassword;
    std::wstring FTunnelPassword;
    TTunnelThread *FTunnelThread;
    TSecureShell *FTunnel;
    TSessionData *FTunnelData;
    TSessionLog *FTunnelLog;
    TTunnelUI *FTunnelUI;
    size_t FTunnelLocalPortNumber;
    std::wstring FTunnelError;
    queryuser_signal_type FOnQueryUser;
    promptuser_signal_type FOnPromptUser;
    displaybanner_signal_type FOnDisplayBanner;
    extendedexception_signal_type FOnShowExtendedException;
    informationevent_signal_type FOnInformation;
    nb::notify_signal_type FOnClose;
    bool FAnyInformation;
    TCallbackGuard *FCallbackGuard;
    findingfile_signal_type FOnFindingFile;
    TTerminal *Self;

    void CommandError(const std::exception *E, const std::wstring Msg);
    int CommandError(const std::exception *E, const std::wstring Msg, int Answers);
    void ReactOnCommand(int /*TFSCommand*/ Cmd);
    void ClearCachedFileList(const std::wstring Path, bool SubDirs);
    void AddCachedFileList(TRemoteFileList *FileList);
    inline bool InTransaction();

    void DoProgress(TFileOperationProgressType &ProgressData, TCancelStatus &Cancel);
protected:
    bool FReadCurrentDirectoryPending;
    bool FReadDirectoryPending;
    bool FTunnelOpening;

    void DoStartReadDirectory();
    void DoReadDirectoryProgress(size_t Progress, bool &Cancel);
    void DoReadDirectory(bool ReloadOnly);
    void DoCreateDirectory(const std::wstring DirName);
    void DoDeleteFile(const std::wstring FileName, const TRemoteFile *File,
                      int Params);
    void DoCustomCommandOnFile(const std::wstring FileName,
                               const TRemoteFile *File, const std::wstring Command, int Params, const captureoutput_slot_type &OutputEvent);
    void DoRenameFile(const std::wstring FileName,
                      const std::wstring NewName, bool Move);
    void DoCopyFile(const std::wstring FileName, const std::wstring NewName);
    void DoChangeFileProperties(const std::wstring FileName,
                                const TRemoteFile *File, const TRemoteProperties *Properties);
    void DoChangeDirectory();
    void EnsureNonExistence(const std::wstring FileName);
    void LookupUsersGroups();
    void FileModified(const TRemoteFile *File,
                      const std::wstring FileName, bool ClearDirectoryChange = false);
    int FileOperationLoop(const fileoperation_slot_type &CallBackFunc,
                          TFileOperationProgressType *OperationProgress, bool AllowSkip,
                          const std::wstring Message, void *Param1 = NULL, void *Param2 = NULL);
    bool ProcessFiles(nb::TStrings *FileList, TFileOperation Operation,
                      const processfile_slot_type &ProcessFile, void *Param = NULL, TOperationSide Side = osRemote);
    void ProcessDirectory(const std::wstring DirName,
                          const processfile_slot_type &CallBackFunc, void *Param = NULL, bool UseCache = false,
                          bool IgnoreErrors = false);
    void AnnounceFileListOperation();
    std::wstring TranslateLockedPath(const std::wstring Path, bool Lock);
    void ReadDirectory(TRemoteFileList *FileList);
    void CustomReadDirectory(TRemoteFileList *FileList);
    void DoCreateLink(const std::wstring FileName, const std::wstring PointTo, bool Symbolic);
    bool CreateLocalFile(const std::wstring FileName,
                         TFileOperationProgressType *OperationProgress, HANDLE *AHandle,
                         bool NoConfirmation);
    void OpenLocalFile(const std::wstring FileName, int Access,
                       int *Attrs, HANDLE *Handle, __int64 *ACTime, __int64 *MTime,
                       __int64 *ATime, __int64 *Size, bool TryWriteReadOnly = true);
    bool AllowLocalFileTransfer(const std::wstring FileName, const TCopyParamType *CopyParam);
    bool HandleException(const std::exception *E);
    void CalculateFileSize(const std::wstring FileName,
                           const TRemoteFile *File, /*TCalculateSizeParams*/ void *Size);
    void DoCalculateDirectorySize(const std::wstring FileName,
                                  const TRemoteFile *File, TCalculateSizeParams *Params);
    void CalculateLocalFileSize(const std::wstring FileName,
                                const WIN32_FIND_DATA &Rec, /*__int64*/ void *Size);
    void CalculateLocalFilesSize(nb::TStrings *FileList, __int64 &Size,
                                 const TCopyParamType *CopyParam = NULL);
    TBatchOverwrite EffectiveBatchOverwrite(
        int Params, TFileOperationProgressType *OperationProgress, bool Special);
    bool CheckRemoteFile(int Params, TFileOperationProgressType *OperationProgress);
    int ConfirmFileOverwrite(const std::wstring FileName,
                             const TOverwriteFileParams *FileParams, int Answers, const TQueryParams *QueryParams,
                             TOperationSide Side, int Params, TFileOperationProgressType *OperationProgress,
                             std::wstring Message = L"");
    void DoSynchronizeCollectDirectory(const std::wstring LocalDirectory,
                                       const std::wstring RemoteDirectory, TSynchronizeMode Mode,
                                       const TCopyParamType *CopyParam, int Params,
                                       const synchronizedirectory_slot_type &OnSynchronizeDirectory,
                                       TSynchronizeOptions *Options, int Level, TSynchronizeChecklist *Checklist);
    void SynchronizeCollectFile(const std::wstring FileName,
                                const TRemoteFile *File, /*TSynchronizeData*/ void *Param);
    void SynchronizeRemoteTimestamp(const std::wstring FileName,
                                    const TRemoteFile *File, void *Param);
    void SynchronizeLocalTimestamp(const std::wstring FileName,
                                   const TRemoteFile *File, void *Param);
    void DoSynchronizeProgress(const TSynchronizeData &Data, bool Collect);
    void DeleteLocalFile(const std::wstring FileName,
                         const TRemoteFile *File, void *Param);
    void RecycleFile(const std::wstring FileName, const TRemoteFile *File);
    void DoStartup();
    virtual bool __fastcall DoQueryReopen(std::exception *E);
    virtual void __fastcall FatalError(const std::exception *E, const std::wstring Msg);
    void __fastcall ResetConnection();
    virtual bool __fastcall DoPromptUser(TSessionData *Data, TPromptKind Kind,
                              const std::wstring Name, const std::wstring Instructions, nb::TStrings *Prompts,
                              nb::TStrings *Response);
    void __fastcall OpenTunnel();
    void __fastcall CloseTunnel();
    void DoInformation(const std::wstring Str, bool Status, bool Active = true);
    std::wstring FileUrl(const std::wstring Protocol, const std::wstring FileName);
    bool PromptUser(TSessionData *Data, TPromptKind Kind,
                    const std::wstring Name, const std::wstring Instructions, const std::wstring Prompt, bool Echo,
                    size_t MaxLen, std::wstring &Result);
    void FileFind(const std::wstring FileName, const TRemoteFile *File, void *Param);
    void DoFilesFind(const std::wstring Directory, TFilesFindParams &Params);
    bool DoCreateLocalFile(const std::wstring FileName,
                           TFileOperationProgressType *OperationProgress, HANDLE *AHandle,
                           bool NoConfirmation);

    virtual void __fastcall Information(const std::wstring Str, bool Status);
    virtual int __fastcall QueryUser(const std::wstring Query,
                          nb::TStrings *MoreMessages, int Answers, const TQueryParams *Params,
                          TQueryType QueryType = qtConfirmation);
    virtual int __fastcall QueryUserException(const std::wstring Query,
                                   const std::exception *E, int Answers, const TQueryParams *Params,
                                   TQueryType QueryType = qtConfirmation);
    virtual int __fastcall QueryUserException(const std::wstring Query,
                                   const ExtException *E, int Answers, const TQueryParams *Params,
                                   TQueryType QueryType = qtConfirmation);
    virtual bool __fastcall PromptUser(TSessionData *Data, TPromptKind Kind,
                            const std::wstring Name, const std::wstring Instructions, nb::TStrings *Prompts, nb::TStrings *Results);
    virtual void __fastcall DisplayBanner(const std::wstring Banner);
    virtual void __fastcall Closed();
    virtual void __fastcall HandleExtendedException(const std::exception *E);
    bool __fastcall IsListenerFree(size_t PortNumber);
    void DoFinished(TFileOperation Operation, TOperationSide Side, bool Temp,
                    const std::wstring FileName, bool Success, TOnceDoneOperation &OnceDoneOperation);
    void RollbackAction(TSessionAction &Action,
                        TFileOperationProgressType *OperationProgress, const std::exception *E = NULL);
    void DoAnyCommand(const std::wstring Command, const captureoutput_slot_type &OutputEvent,
                      TCallSessionAction *Action);
    TRemoteFileList *DoReadDirectoryListing(const std::wstring Directory, bool UseCache);
    std::wstring __fastcall EncryptPassword(const std::wstring Password);
    std::wstring __fastcall DecryptPassword(const std::wstring Password);

    TFileOperationProgressType * __fastcall GetOperationProgress() { return FOperationProgress; }

public:
    explicit TTerminal();
    virtual void __fastcall Init(TSessionData *SessionData, TConfiguration *Configuration);
    virtual ~TTerminal();
    void __fastcall Open();
    void __fastcall Close();
    void __fastcall Reopen(int Params);
    virtual void __fastcall DirectoryModified(const std::wstring Path, bool SubDirs);
    virtual void __fastcall DirectoryLoaded(TRemoteFileList *FileList);
    void __fastcall ShowExtendedException(const std::exception *E);
    void __fastcall Idle();
    void __fastcall RecryptPasswords();
    bool __fastcall AllowedAnyCommand(const std::wstring Command);
    void __fastcall AnyCommand(const std::wstring Command,
                    const captureoutput_slot_type *OutputEvent);
    void CloseOnCompletion(TOnceDoneOperation Operation = odoDisconnect, const std::wstring Message = L"");
    std::wstring AbsolutePath(const std::wstring Path, bool Local);
    void BeginTransaction();
    void ReadCurrentDirectory();
    void ReadDirectory(bool ReloadOnly, bool ForceCache = false);
    TRemoteFileList *ReadDirectoryListing(const std::wstring Directory, const TFileMasks &Mask);
    TRemoteFileList *CustomReadDirectoryListing(const std::wstring Directory, bool UseCache);
    void ReadFile(const std::wstring FileName, TRemoteFile *& File);
    bool FileExists(const std::wstring FileName, TRemoteFile **File = NULL);
    void ReadSymlink(TRemoteFile *SymlinkFile, TRemoteFile *& File);
    bool CopyToLocal(nb::TStrings *FilesToCopy,
                     const std::wstring TargetDir, const TCopyParamType *CopyParam, int Params);
    bool CopyToRemote(nb::TStrings *FilesToCopy,
                      const std::wstring TargetDir, const TCopyParamType *CopyParam, int Params);
    void CreateDirectory(const std::wstring DirName,
                         const TRemoteProperties *Properties = NULL);
    void CreateLink(const std::wstring FileName, const std::wstring PointTo, bool Symbolic);
    void DeleteFile(const std::wstring FileName,
                    const TRemoteFile *File = NULL, void *Params = NULL);
    bool DeleteFiles(nb::TStrings *FilesToDelete, int Params = 0);
    bool DeleteLocalFiles(nb::TStrings *FileList, int Params = 0);
    bool IsRecycledFile(const std::wstring FileName);
    void CustomCommandOnFile(const std::wstring FileName,
                             const TRemoteFile *File, void *AParams);
    void CustomCommandOnFiles(const std::wstring Command, int Params,
                              nb::TStrings *Files, const captureoutput_slot_type &OutputEvent);
    void ChangeDirectory(const std::wstring Directory);
    void EndTransaction();
    void HomeDirectory();
    void ChangeFileProperties(const std::wstring FileName,
                              const TRemoteFile *File, /*const TRemoteProperties */ void *Properties);
    void ChangeFilesProperties(nb::TStrings *FileList,
                               const TRemoteProperties *Properties);
    bool LoadFilesProperties(nb::TStrings *FileList);
    void TerminalError(const std::wstring Msg);
    void TerminalError(const std::exception *E, const std::wstring Msg);
    void ReloadDirectory();
    void RefreshDirectory();
    void RenameFile(const std::wstring FileName, const std::wstring NewName);
    void RenameFile(const TRemoteFile *File, const std::wstring NewName, bool CheckExistence);
    void MoveFile(const std::wstring FileName, const TRemoteFile *File,
                  /*const TMoveFileParams*/ void *Param);
    bool MoveFiles(nb::TStrings *FileList, const std::wstring Target,
                   const std::wstring FileMask);
    void CopyFile(const std::wstring FileName, const TRemoteFile *File,
                  /*const TMoveFileParams*/ void *Param);
    bool CopyFiles(nb::TStrings *FileList, const std::wstring Target,
                   const std::wstring FileMask);
    void CalculateFilesSize(nb::TStrings *FileList, __int64 &Size,
                            int Params, const TCopyParamType *CopyParam = NULL, TCalculateSizeStats *Stats = NULL);
    void CalculateFilesChecksum(const std::wstring Alg, nb::TStrings *FileList,
                                nb::TStrings *Checksums, calculatedchecksum_slot_type *OnCalculatedChecksum);
    void ClearCaches();
    TSynchronizeChecklist *SynchronizeCollect(const std::wstring LocalDirectory,
            const std::wstring RemoteDirectory, TSynchronizeMode Mode,
            const TCopyParamType *CopyParam, int Params,
            const synchronizedirectory_slot_type &OnSynchronizeDirectory, TSynchronizeOptions *Options);
    void SynchronizeApply(TSynchronizeChecklist *Checklist,
                          const std::wstring LocalDirectory, const std::wstring RemoteDirectory,
                          const TCopyParamType *CopyParam, int Params,
                          const synchronizedirectory_slot_type &OnSynchronizeDirectory);
    void FilesFind(const std::wstring Directory, const TFileMasks &FileMask,
                   const filefound_slot_type *OnFileFound, const findingfile_slot_type *OnFindingFile);
    void SpaceAvailable(const std::wstring Path, TSpaceAvailable &ASpaceAvailable);
    bool DirectoryFileList(const std::wstring Path,
                           TRemoteFileList *& FileList, bool CanLoad);
    void MakeLocalFileList(const std::wstring FileName,
                           const WIN32_FIND_DATA &Rec, void *Param);
    std::wstring FileUrl(const std::wstring FileName);
    bool FileOperationLoopQuery(const std::exception &E,
                                TFileOperationProgressType *OperationProgress, const std::wstring Message,
                                bool AllowSkip, std::wstring SpecialRetry = L"");
    TUsableCopyParamAttrs UsableCopyParamAttrs(int Params);
    bool QueryReopen(std::exception *E, int Params,
                     TFileOperationProgressType *OperationProgress);
    std::wstring PeekCurrentDirectory();

    const TSessionInfo &GetSessionInfo();
    const TFileSystemInfo &GetFileSystemInfo(bool Retrieve = false);
    void inline LogEvent(const std::wstring Str);

    static bool IsAbsolutePath(const std::wstring Path);
    static std::wstring __fastcall ExpandFileName(const std::wstring Path,
                                       const std::wstring BasePath);

    TSessionData *__fastcall GetSessionData() { return FSessionData; }
    TSessionLog *__fastcall GetLog() { return FLog; }
    TConfiguration *__fastcall GetConfiguration() { return FConfiguration; }
    bool __fastcall GetActive();
    TSessionStatus __fastcall GetStatus() { return FStatus; }
    std::wstring __fastcall GetCurrentDirectory();
    void SetCurrentDirectory(const std::wstring value);
    bool GetExceptionOnFail() const;
    void SetExceptionOnFail(bool value);
    TRemoteDirectory *GetFiles() { return FFiles; }
    const nb::notify_signal_type &GetOnChangeDirectory() const { return FOnChangeDirectory; }
    void SetOnChangeDirectory(const nb::notify_slot_type &value) { FOnChangeDirectory.connect(value); }
    readdirectory_signal_type &GetOnReadDirectory() { return FOnReadDirectory; }
    void SetOnReadDirectory(const readdirectory_slot_type &value) { FOnReadDirectory.connect(value); }
    const nb::notify_signal_type &GetOnStartReadDirectory() const { return FOnStartReadDirectory; }
    void SetOnStartReadDirectory(const nb::notify_slot_type &value) { FOnStartReadDirectory.connect(value); }
    readdirectoryprogress_signal_type &GetOnReadDirectoryProgress() { return FOnReadDirectoryProgress; }
    void SetOnReadDirectoryProgress(const readdirectoryprogress_slot_type &value) { FOnReadDirectoryProgress.connect(value); }
    deletelocalfile_signal_type &GetOnDeleteLocalFile() { return FOnDeleteLocalFile; }
    void SetOnDeleteLocalFile(const deletelocalfile_slot_type &value) { FOnDeleteLocalFile.connect(value); }
    const TRemoteTokenList *GetGroups();
    const TRemoteTokenList *GetUsers();
    const TRemoteTokenList *GetMembership();
    const fileoperationprogress_signal_type &GetOnProgress() const { return FOnProgress; }
    void SetOnProgress(const fileoperationprogress_slot_type &value) { FOnProgress.connect(value); }
    const fileoperationfinished_signal_type &GetOnFinished() const { return FOnFinished; }
    void SetOnFinished(const fileoperationfinished_slot_type &value) { FOnFinished.connect(value); }
    TCurrentFSProtocol GetFSProtocol() { return FFSProtocol; }
    bool GetUseBusyCursor() { return FUseBusyCursor; }
    void SetUseBusyCursor(bool value) { FUseBusyCursor = value; }
    std::wstring GetUserName();
    bool GetIsCapable(TFSCapability Capability) const;
    bool GetAreCachesEmpty() const;
    bool GetCommandSessionOpened();
    TTerminal *GetCommandSession();
    bool GetAutoReadDirectory() { return FAutoReadDirectory; }
    void SetAutoReadDirectory(bool value) { FAutoReadDirectory = value; }
    nb::TStrings *GetFixedPaths();
    bool GetResolvingSymlinks();
    std::wstring GetPassword();
    std::wstring GetTunnelPassword();
    bool GetStoredCredentialsTried();
    queryuser_signal_type &GetOnQueryUser() { return FOnQueryUser; }
    void SetOnQueryUser(const queryuser_slot_type &value) { FOnQueryUser.connect(value); }
    promptuser_signal_type &GetOnPromptUser() { return FOnPromptUser; }
    void SetOnPromptUser(const promptuser_slot_type &value) { FOnPromptUser.connect(value); }
    displaybanner_signal_type &GetOnDisplayBanner() { return FOnDisplayBanner; }
    void SetOnDisplayBanner(const displaybanner_slot_type &value) { FOnDisplayBanner.connect(value); }
    extendedexception_signal_type &GetOnShowExtendedException() { return FOnShowExtendedException; }
    void SetOnShowExtendedException(const extendedexception_slot_type &value) { FOnShowExtendedException.connect(value); }
    informationevent_signal_type &GetOnInformation() { return FOnInformation; }
    void SetOnInformation(const informationevent_slot_type &value) { FOnInformation.connect(value); }
    const nb::notify_signal_type &GetOnClose() const { return FOnClose; }
    void SetOnClose(const nb::notify_slot_type &value) { FOnClose.connect(value); }
    size_t GetTunnelLocalPortNumber() { return FTunnelLocalPortNumber; }
};
//---------------------------------------------------------------------------
class TSecondaryTerminal : public TTerminal
{
public:
    explicit TSecondaryTerminal(TTerminal *MainTerminal);
    virtual void __fastcall Init(TSessionData *SessionData, TConfiguration *Configuration,
                      const std::wstring Name);
    virtual ~TSecondaryTerminal()
    {}

protected:
    virtual void __fastcall DirectoryLoaded(TRemoteFileList *FileList);
    virtual void __fastcall DirectoryModified(const std::wstring Path,
                                   bool SubDirs);
    virtual bool __fastcall DoPromptUser(TSessionData *Data, TPromptKind Kind,
                              const std::wstring Name, const std::wstring Instructions, nb::TStrings *Prompts, nb::TStrings *Results);

private:
    bool FMasterPasswordTried;
    bool FMasterTunnelPasswordTried;
    TTerminal *FMainTerminal;
};
//---------------------------------------------------------------------------
class TTerminalList : public nb::TObjectList
{
public:
    explicit TTerminalList(TConfiguration *AConfiguration);
    virtual ~TTerminalList();

    virtual TTerminal *NewTerminal(TSessionData *Data);
    virtual void FreeTerminal(TTerminal *Terminal);
    void FreeAndNullTerminal(TTerminal * & Terminal);
    virtual void Idle();
    void RecryptPasswords();

    TTerminal *GetTerminal(size_t Index);
    int GetActiveCount();

protected:
    virtual TTerminal *CreateTerminal(TSessionData *Data);

private:
    TConfiguration *FConfiguration;

};
//---------------------------------------------------------------------------
struct TCustomCommandParams
{
    TCustomCommandParams(
        std::wstring Command,
        int Params,
        const captureoutput_slot_type &OutputEvent) :
        Command(Command),
        Params(Params),
        OutputEvent(OutputEvent)
    {
    }
    std::wstring Command;
    int Params;
    // captureoutput_signal_type OutputEvent;
    const captureoutput_slot_type &OutputEvent;
private:
    TCustomCommandParams(const TCustomCommandParams &);
    void operator=(const TCustomCommandParams &);
};
//---------------------------------------------------------------------------
struct TCalculateSizeStats
{
    TCalculateSizeStats();

    int Files;
    int Directories;
    int SymLinks;
};
//---------------------------------------------------------------------------
struct TCalculateSizeParams
{
    __int64 Size;
    int Params;
    const TCopyParamType *CopyParam;
    TCalculateSizeStats *Stats;
};
//---------------------------------------------------------------------------
struct TOverwriteFileParams
{
    TOverwriteFileParams();

    __int64 SourceSize;
    __int64 DestSize;
    nb::TDateTime SourceTimestamp;
    nb::TDateTime DestTimestamp;
    TModificationFmt SourcePrecision;
    TModificationFmt DestPrecision;
};
//---------------------------------------------------------------------------
struct TMakeLocalFileListParams
{
    nb::TStrings *FileList;
    bool IncludeDirs;
    bool Recursive;
};
//---------------------------------------------------------------------------
struct TSynchronizeOptions
{
    TSynchronizeOptions();
    ~TSynchronizeOptions();

    nb::TStringList *Filter;
};
//---------------------------------------------------------------------------
class TSynchronizeChecklist
{
    friend class TTerminal;

public:
    enum TAction { saNone, saUploadNew, saDownloadNew, saUploadUpdate,
                   saDownloadUpdate, saDeleteRemote, saDeleteLocal
                 };
    static const int ActionCount = saDeleteLocal;

    class TItem
    {
        friend class TTerminal;

    public:
        struct TFileInfo
        {
            std::wstring FileName;
            std::wstring Directory;
            nb::TDateTime Modification;
            TModificationFmt ModificationFmt;
            __int64 Size;
        };

        TAction Action;
        bool IsDirectory;
        TFileInfo Local;
        TFileInfo Remote;
        int ImageIndex;
        bool Checked;
        TRemoteFile *RemoteFile;

        const std::wstring GetFileName() const;

        ~TItem();

    private:
        FILETIME FLocalLastWriteTime;

        TItem();
    };

    ~TSynchronizeChecklist();

    size_t GetCount() const;
    const TItem *GetItem(size_t Index) const;

protected:
    TSynchronizeChecklist();

    void Sort();
    void Add(TItem *Item);


private:
    nb::TList *FList;

    static int Compare(void *Item1, void *Item2);
};
//---------------------------------------------------------------------------
struct TSpaceAvailable
{
    TSpaceAvailable();

    __int64 BytesOnDevice;
    __int64 UnusedBytesOnDevice;
    __int64 BytesAvailableToUser;
    __int64 UnusedBytesAvailableToUser;
    size_t BytesPerAllocationUnit;
};
//---------------------------------------------------------------------------
#endif
