//---------------------------------------------------------------------------
#ifndef SessionInfoH
#define SessionInfoH

#include "boostdefines.hpp"
#include <boost/signals/signal2.hpp>
#include <boost/signals/signal3.hpp>

#include "SessionData.h"
#include "Interface.h"
#include "Exceptions.h"
//---------------------------------------------------------------------------
enum TSessionStatus { ssClosed, ssOpening, ssOpened };
//---------------------------------------------------------------------------
struct TSessionInfo
{
    TSessionInfo();

    System::TDateTime LoginTime;
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
                          System::TStrings *MoreMessages, int Answers, const TQueryParams *Params,
                          TQueryType QueryType = qtConfirmation) = 0;
    virtual int __fastcall QueryUserException(const std::wstring Query,
                                   const std::exception *E, int Answers, const TQueryParams *Params,
                                   TQueryType QueryType = qtConfirmation) = 0;
    virtual bool __fastcall PromptUser(TSessionData *Data, TPromptKind Kind,
                            const std::wstring Name, const std::wstring Instructions, System::TStrings *Prompts,
                            System::TStrings *Results) = 0;
    virtual void __fastcall DisplayBanner(const std::wstring Banner) = 0;
    virtual void __fastcall FatalError(const std::exception *E, const std::wstring Msg) = 0;
    virtual void __fastcall HandleExtendedException(const std::exception *E) = 0;
    virtual void __fastcall Closed() = 0;
};
//---------------------------------------------------------------------------
// Duplicated in LogMemo.h for design-time-only purposes
enum TLogLineType { llOutput, llInput, llStdError, llMessage, llException };
enum TLogAction { laUpload, laDownload, laTouch, laChmod, laMkdir, laRm, laMv, laCall, laLs, laStat };
//---------------------------------------------------------------------------
typedef boost::signal2<void, const std::wstring, bool> captureoutput_signal_type;
typedef captureoutput_signal_type::slot_type captureoutput_slot_type;
typedef boost::signal3<void, const std::wstring, const std::wstring, const std::wstring> calculatedchecksum_signal_type;
typedef calculatedchecksum_signal_type::slot_type calculatedchecksum_slot_type;
//---------------------------------------------------------------------------
class TCriticalSection;
class TSessionActionRecord;
class TSessionLog;
class TActionLog;
//---------------------------------------------------------------------------
class TSessionAction
{
public:
    explicit TSessionAction(TActionLog *Log, TLogAction Action);
    ~TSessionAction();

    void __fastcall Restart();

    void __fastcall Commit();
    void __fastcall Rollback(const std::exception *E = NULL);
    void __fastcall Cancel();

protected:
    TSessionActionRecord *FRecord;
};
//---------------------------------------------------------------------------
class TFileSessionAction : public TSessionAction
{
public:
    explicit TFileSessionAction(TActionLog *Log, TLogAction Action);
    explicit TFileSessionAction(TActionLog *Log, TLogAction Action, const std::wstring FileName);

    void __fastcall FileName(const std::wstring FileName);
};
//---------------------------------------------------------------------------
class TFileLocationSessionAction : public TFileSessionAction
{
public:
    explicit TFileLocationSessionAction(TActionLog *Log, TLogAction Action);
    explicit TFileLocationSessionAction(TActionLog *Log, TLogAction Action, const std::wstring FileName);

    void __fastcall Destination(const std::wstring Destination);
};
//---------------------------------------------------------------------------
class TUploadSessionAction : public TFileLocationSessionAction
{
public:
    explicit TUploadSessionAction(TActionLog *Log);
};
//---------------------------------------------------------------------------
class TDownloadSessionAction : public TFileLocationSessionAction
{
public:
    explicit TDownloadSessionAction(TActionLog *Log);
};
//---------------------------------------------------------------------------
class TRights;
//---------------------------------------------------------------------------
class TChmodSessionAction : public TFileSessionAction
{
public:
    explicit TChmodSessionAction(TActionLog *Log, const std::wstring FileName);
    explicit TChmodSessionAction(TActionLog *Log, const std::wstring FileName,
                                 const TRights &Rights);

    void __fastcall Rights(const TRights &Rights);
    void __fastcall Recursive();
};
//---------------------------------------------------------------------------
class TTouchSessionAction : public TFileSessionAction
{
public:
    explicit TTouchSessionAction(TActionLog *Log, const std::wstring FileName,
                                 const System::TDateTime &Modification);
};
//---------------------------------------------------------------------------
class TMkdirSessionAction : public TFileSessionAction
{
public:
    explicit TMkdirSessionAction(TActionLog *Log, const std::wstring FileName);
};
//---------------------------------------------------------------------------
class TRmSessionAction : public TFileSessionAction
{
public:
    explicit TRmSessionAction(TActionLog *Log, const std::wstring FileName);

    void __fastcall Recursive();
};
//---------------------------------------------------------------------------
class TMvSessionAction : public TFileLocationSessionAction
{
public:
    explicit TMvSessionAction(TActionLog *Log, const std::wstring FileName,
                              const std::wstring Destination);
};
//---------------------------------------------------------------------------
class TCallSessionAction : public TSessionAction
{
public:
    explicit TCallSessionAction(TActionLog *Log, const std::wstring Command,
                                const std::wstring Destination);

    void __fastcall AddOutput(const std::wstring Output, bool StdError);
};
//---------------------------------------------------------------------------
class TLsSessionAction : public TSessionAction
{
public:
    explicit TLsSessionAction(TActionLog *Log, const std::wstring Destination);

    void __fastcall FileList(TRemoteFileList *FileList);
};
//---------------------------------------------------------------------------
class TStatSessionAction : public TFileSessionAction
{
public:
  explicit TStatSessionAction(TActionLog * Log, const std::wstring & FileName);

  void __fastcall File(TRemoteFile * File);
};
//---------------------------------------------------------------------------
typedef boost::signal2<void, TLogLineType, const std::wstring> doaddlog_signal_type;
typedef doaddlog_signal_type::slot_type doaddlog_slot_type;
//---------------------------------------------------------------------------
class TSessionLog : protected System::TStringList
{
    friend class TSessionAction;
    friend class TSessionActionRecord;
public:
    explicit TSessionLog(TSessionUI *UI, TSessionData *SessionData,
                         TConfiguration *Configuration);
    virtual ~TSessionLog();
    virtual void __fastcall Add(TLogLineType Type, const std::wstring Line);
    void __fastcall AddStartupInfo();
    void __fastcall AddException(const std::exception *E);
    void __fastcall AddSeparator();

    virtual void __fastcall Clear();
    void __fastcall ReflectSettings();
    void __fastcall Lock();
    void __fastcall Unlock();

    TSessionLog * __fastcall GetParent() { return FParent; }
    void __fastcall SetParent(TSessionLog *value) { FParent = value; }
    bool __fastcall GetLogging() { return FLogging; }
    size_t __fastcall GetBottomIndex();
    std::wstring __fastcall GetLine(size_t Index);
    TLogLineType __fastcall GetType(size_t Index);
    const System::notify_signal_type &GetOnStateChange() const { return FOnStateChange; }
    void SetOnStateChange(const System::notify_slot_type &value) { FOnStateChange.connect(value); }
    std::wstring __fastcall GetCurrentFileName() { return FCurrentFileName; }
    bool __fastcall GetLoggingToFile();
    size_t __fastcall GetTopIndex() { return FTopIndex; }
    std::wstring __fastcall GetSessionName();
    std::wstring __fastcall GetName() { return FName; }
    void __fastcall SetName(const std::wstring value) { FName = value; }

protected:
    void __fastcall CloseLogFile();
    bool __fastcall LogToFile();

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
    bool FClosed;
    System::notify_signal_type FOnStateChange;
    TSessionLog *Self;

    std::wstring __fastcall GetLine(int Index);
    TLogLineType __fastcall GetType(int Index);
    void __fastcall DeleteUnnecessary();
    void __fastcall StateChange();
    void __fastcall OpenLogFile();
    std::wstring __fastcall GetLogFileName();
    void __fastcall DoAdd(TLogLineType Type, const std::wstring Line,
               const doaddlog_slot_type &func);
    void DoAddToParent(TLogLineType aType, const std::wstring aLine);
    void DoAddToSelf(TLogLineType aType, const std::wstring aLine);
    void DoAddStartupInfo(TSessionData *Data);
};
//---------------------------------------------------------------------------
class TActionLog
{
friend class TSessionAction;
friend class TSessionActionRecord;
public:
  explicit TActionLog(TSessionUI* UI, TSessionData * SessionData,
    TConfiguration * Configuration);
  virtual ~TActionLog();

  void __fastcall ReflectSettings();
  void __fastcall AddFailure(const std::exception * E);
  void __fastcall AddFailure(System::TStrings * Messages);
  void __fastcall BeginGroup(std::wstring Name);
  void __fastcall EndGroup();

  std::wstring __fastcall GetCurrentFileName() const { return FCurrentFileName; };
  bool __fastcall GetEnabled() const { return FEnabled; }
  void __fastcall SetEnabled(bool value);

protected:
  void __fastcall CloseLogFile();
  inline void __fastcall AddPendingAction(TSessionActionRecord * Action);
  void __fastcall RecordPendingActions();
  void __fastcall Add(const std::wstring  & Line);
  void __fastcall AddIndented(const std::wstring  & Line);
  void __fastcall AddMessages(std::wstring Indent, System::TStrings * Messages);

private:
  TConfiguration * FConfiguration;
  TCriticalSection * FCriticalSection;
  bool FLogging;
  void * FFile;
  std::wstring  FCurrentLogFileName;
  std::wstring  FCurrentFileName;
  TSessionUI * FUI;
  TSessionData * FSessionData;
  System::TList * FPendingActions;
  bool FClosed;
  bool FInGroup;
  std::wstring FIndent;
  bool FEnabled;

  void __fastcall OpenLogFile();
  std::wstring  __fastcall GetLogFileName();
};
//---------------------------------------------------------------------------
#endif
