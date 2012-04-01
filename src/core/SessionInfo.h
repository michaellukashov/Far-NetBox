//---------------------------------------------------------------------------
#ifndef SessionInfoH
#define SessionInfoH

#include "boostdefines.hpp"
#include <boost/signals/signal2.hpp>
#include <boost/signals/signal3.hpp>

#include "SessionData.h"
#include "Interface.h"
// #include "Exceptions.h"
//---------------------------------------------------------------------------
enum TSessionStatus { ssClosed, ssOpening, ssOpened };
//---------------------------------------------------------------------------
struct TSessionInfo
{
    TSessionInfo();

    nb::TDateTime LoginTime;
    std::wstring ProtocolBaseName;
    std::wstring ProtocolName;
    std::wstring SecurityProtocolName;

    std::wstring CSCipher;
    std::wstring CSCompression;
    std::wstring SCCipher;
    std::wstring SCCompression;

    std::wstring SshVersionString;
    std::wstring SshImplementation;
    std::wstring HostKeyFingerprint;

    std::wstring CertificateFingerprint;
    std::wstring Certificate;
};
//---------------------------------------------------------------------------
enum TFSCapability { fcUserGroupListing, fcModeChanging, fcGroupChanging,
                     fcOwnerChanging, fcGroupOwnerChangingByID, fcAnyCommand, fcHardLink,
                     fcSymbolicLink, fcResolveSymlink,
                     fcTextMode, fcRename, fcNativeTextMode, fcNewerOnlyUpload, fcRemoteCopy,
                     fcTimestampChanging, fcRemoteMove, fcLoadingAdditionalProperties,
                     fcCheckingSpaceAvailable, fcIgnorePermErrors, fcCalculatingChecksum,
                     fcModeChangingUpload, fcPreservingTimestampUpload, fcShellAnyCommand,
                     fcSecondaryShell, fcCount
                   };
//---------------------------------------------------------------------------
struct TFileSystemInfo
{
    TFileSystemInfo();

    std::wstring ProtocolBaseName;
    std::wstring ProtocolName;
    std::wstring RemoteSystem;
    std::wstring AdditionalInfo;
    bool IsCapable[fcCount];
};
//---------------------------------------------------------------------------
class TSessionUI
{
public:
    explicit TSessionUI()
    {}
    virtual ~TSessionUI()
    {}
    virtual void __fastcall Information(const std::wstring Str, bool Status) = 0;
    virtual int __fastcall QueryUser(const std::wstring Query,
                          nb::TStrings *MoreMessages, int Answers, const TQueryParams *Params,
                          TQueryType QueryType = qtConfirmation) = 0;
    virtual int __fastcall QueryUserException(const std::wstring Query,
                                   const std::exception *E, int Answers, const TQueryParams *Params,
                                   TQueryType QueryType = qtConfirmation) = 0;
    virtual bool __fastcall PromptUser(TSessionData *Data, TPromptKind Kind,
                            const std::wstring Name, const std::wstring Instructions, nb::TStrings *Prompts,
                            nb::TStrings *Results) = 0;
    virtual void __fastcall DisplayBanner(const std::wstring Banner) = 0;
    virtual void __fastcall FatalError(const std::exception *E, const std::wstring Msg) = 0;
    virtual void __fastcall HandleExtendedException(const std::exception *E) = 0;
    virtual void __fastcall Closed() = 0;
};
//---------------------------------------------------------------------------
// Duplicated in LogMemo.h for design-time-only purposes
enum TLogLineType { llOutput, llInput, llStdError, llMessage, llException, llAction };
enum TLogAction { laUpload, laDownload, laTouch, laChmod, laMkdir, laRm, laMv, laCall, laLs };
//---------------------------------------------------------------------------
typedef boost::signal2<void, const std::wstring, bool> captureoutput_signal_type;
typedef captureoutput_signal_type::slot_type captureoutput_slot_type;
typedef boost::signal3<void, const std::wstring, const std::wstring, const std::wstring > calculatedchecksum_signal_type;
typedef calculatedchecksum_signal_type::slot_type calculatedchecksum_slot_type;
//---------------------------------------------------------------------------
class TCriticalSection;
class TSessionActionRecord;
class TSessionLog;
//---------------------------------------------------------------------------
class TSessionAction
{
public:
    explicit TSessionAction(TSessionLog *Log, TLogAction Action);
    ~TSessionAction();

    void Restart();

    void Commit();
    void Rollback(const std::exception *E = NULL);
    void Cancel();

protected:
    TSessionActionRecord *FRecord;
};
//---------------------------------------------------------------------------
class TFileSessionAction : public TSessionAction
{
public:
    explicit TFileSessionAction(TSessionLog *Log, TLogAction Action);
    explicit TFileSessionAction(TSessionLog *Log, TLogAction Action, const std::wstring FileName);

    void FileName(const std::wstring FileName);
};
//---------------------------------------------------------------------------
class TFileLocationSessionAction : public TFileSessionAction
{
public:
    explicit TFileLocationSessionAction(TSessionLog *Log, TLogAction Action);
    explicit TFileLocationSessionAction(TSessionLog *Log, TLogAction Action, const std::wstring FileName);

    void Destination(const std::wstring Destination);
};
//---------------------------------------------------------------------------
class TUploadSessionAction : public TFileLocationSessionAction
{
public:
    explicit TUploadSessionAction(TSessionLog *Log);
};
//---------------------------------------------------------------------------
class TDownloadSessionAction : public TFileLocationSessionAction
{
public:
    explicit TDownloadSessionAction(TSessionLog *Log);
};
//---------------------------------------------------------------------------
class TRights;
//---------------------------------------------------------------------------
class TChmodSessionAction : public TFileSessionAction
{
public:
    explicit TChmodSessionAction(TSessionLog *Log, const std::wstring FileName);
    explicit TChmodSessionAction(TSessionLog *Log, const std::wstring FileName,
                                 const TRights &Rights);

    void Rights(const TRights &Rights);
    void Recursive();
};
//---------------------------------------------------------------------------
class TTouchSessionAction : public TFileSessionAction
{
public:
    explicit TTouchSessionAction(TSessionLog *Log, const std::wstring FileName,
                                 const nb::TDateTime &Modification);
};
//---------------------------------------------------------------------------
class TMkdirSessionAction : public TFileSessionAction
{
public:
    explicit TMkdirSessionAction(TSessionLog *Log, const std::wstring FileName);
};
//---------------------------------------------------------------------------
class TRmSessionAction : public TFileSessionAction
{
public:
    explicit TRmSessionAction(TSessionLog *Log, const std::wstring FileName);

    void Recursive();
};
//---------------------------------------------------------------------------
class TMvSessionAction : public TFileLocationSessionAction
{
public:
    explicit TMvSessionAction(TSessionLog *Log, const std::wstring FileName,
                              const std::wstring Destination);
};
//---------------------------------------------------------------------------
class TCallSessionAction : public TSessionAction
{
public:
    explicit TCallSessionAction(TSessionLog *Log, const std::wstring Command,
                                const std::wstring Destination);

    void AddOutput(const std::wstring Output, bool StdError);
};
//---------------------------------------------------------------------------
class TLsSessionAction : public TSessionAction
{
public:
    explicit TLsSessionAction(TSessionLog *Log, const std::wstring Destination);

    void FileList(TRemoteFileList *FileList);
};
//---------------------------------------------------------------------------
typedef boost::signal2<void, TLogLineType, const std::wstring > doaddlog_signal_type;
typedef doaddlog_signal_type::slot_type doaddlog_slot_type;
//---------------------------------------------------------------------------
class TSessionLog : protected nb::TStringList
{
    friend class TSessionAction;
    friend class TSessionActionRecord;
public:
    explicit TSessionLog(TSessionUI *UI, TSessionData *SessionData,
                         TConfiguration *Configuration);
    virtual ~TSessionLog();
    virtual void Add(TLogLineType Type, const std::wstring Line);
    void AddStartupInfo();
    void AddException(const std::exception *E);
    void AddSeparator();

    virtual void __fastcall Clear();
    void ReflectSettings();
    void Lock();
    void Unlock();

    TSessionLog *GetParent() { return FParent; }
    void SetParent(TSessionLog *value) { FParent = value; }
    bool GetLogging() { return FLogging; }
    size_t GetBottomIndex();
    std::wstring GetLine(size_t Index);
    TLogLineType GetType(size_t Index);
    const nb::notify_signal_type &GetOnStateChange() const { return FOnStateChange; }
    void SetOnStateChange(const nb::notify_slot_type &value) { FOnStateChange.connect(value); }
    std::wstring GetCurrentFileName() { return FCurrentFileName; }
    bool GetLoggingToFile();
    size_t GetTopIndex() { return FTopIndex; }
    std::wstring GetSessionName();
    std::wstring GetName() { return FName; }
    void SetName(const std::wstring value) { FName = value; }

protected:
    void CloseLogFile();
    bool LogToFile();
    inline void AddPendingAction(TSessionActionRecord *Action);
    void RecordPendingActions();

private:
    TConfiguration *FConfiguration;
    TSessionLog *FParent;
    TCriticalSection *FCriticalSection;
    bool FLogging;
    void *FFile;
    std::wstring FCurrentLogFileName;
    std::wstring FCurrentFileName;
    int FLoggedLines;
    size_t FTopIndex;
    TSessionUI *FUI;
    TSessionData *FSessionData;
    std::wstring FName;
    bool FLoggingActions;
    bool FClosed;
    nb::TList *FPendingActions;
    nb::notify_signal_type FOnStateChange;
    TSessionLog *Self;

    void DeleteUnnecessary();
    void StateChange();
    void OpenLogFile();
    std::wstring GetLogFileName();
    void DoAdd(TLogLineType Type, const std::wstring Line,
               const doaddlog_slot_type &func);
    void DoAddToParent(TLogLineType aType, const std::wstring aLine);
    void DoAddToSelf(TLogLineType aType, const std::wstring aLine);
    void DoAddStartupInfo(TSessionData *Data);
};
//---------------------------------------------------------------------------
#endif
