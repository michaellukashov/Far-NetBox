//---------------------------------------------------------------------------
#ifndef SessionInfoH
#define SessionInfoH

#ifdef _MSC_VER
#include "UnicodeString.hpp"
#endif

#include "SessionData.h"
#include "Interface.h"

#ifdef _MSC_VER
#include "boostdefines.hpp"
#include <boost/signals/signal2.hpp>
#include <boost/signals/signal3.hpp>

#include "Exceptions.h"
#endif
//---------------------------------------------------------------------------
enum TSessionStatus { ssClosed, ssOpening, ssOpened };
//---------------------------------------------------------------------------
struct TSessionInfo
{
  TSessionInfo();

  TDateTime LoginTime;
  UnicodeString ProtocolBaseName;
  UnicodeString ProtocolName;
  UnicodeString SecurityProtocolName;

  UnicodeString CSCipher;
  UnicodeString CSCompression;
  UnicodeString SCCipher;
  UnicodeString SCCompression;

  UnicodeString SshVersionString;
  UnicodeString SshImplementation;
  UnicodeString HostKeyFingerprint;

  UnicodeString CertificateFingerprint;
  UnicodeString Certificate;
};
//---------------------------------------------------------------------------
enum TFSCapability { fcUserGroupListing, fcModeChanging, fcGroupChanging,
  fcOwnerChanging, fcGroupOwnerChangingByID, fcAnyCommand, fcHardLink,
  fcSymbolicLink, fcResolveSymlink,
  fcTextMode, fcRename, fcNativeTextMode, fcNewerOnlyUpload, fcRemoteCopy,
  fcTimestampChanging, fcRemoteMove, fcLoadingAdditionalProperties,
  fcCheckingSpaceAvailable, fcIgnorePermErrors, fcCalculatingChecksum,
  fcModeChangingUpload, fcPreservingTimestampUpload, fcShellAnyCommand,
  fcSecondaryShell, fcCount };
//---------------------------------------------------------------------------
struct TFileSystemInfo
{
  TFileSystemInfo();

  UnicodeString ProtocolBaseName;
  UnicodeString ProtocolName;
  UnicodeString RemoteSystem;
  UnicodeString AdditionalInfo;
  bool IsCapable[fcCount];
};
//---------------------------------------------------------------------------
class TSessionUI
{
public:
  explicit TSessionUI(){}
  virtual ~TSessionUI(){}
  virtual void __fastcall Information(const UnicodeString & Str, bool Status) = 0;
  virtual unsigned int __fastcall QueryUser(const UnicodeString Query,
    TStrings * MoreMessages, unsigned int Answers, const TQueryParams * Params,
    TQueryType QueryType = qtConfirmation) = 0;
  virtual unsigned int __fastcall QueryUserException(const UnicodeString Query,
    Exception * E, unsigned int Answers, const TQueryParams * Params,
    TQueryType QueryType = qtConfirmation) = 0;
  virtual bool __fastcall PromptUser(TSessionData * Data, TPromptKind Kind,
    UnicodeString Name, UnicodeString Instructions, TStrings * Prompts,
    TStrings * Results) = 0;
  virtual void __fastcall DisplayBanner(const UnicodeString & Banner) = 0;
  virtual void __fastcall FatalError(Exception * E, UnicodeString Msg) = 0;
  virtual void __fastcall HandleExtendedException(Exception * E) = 0;
  virtual void __fastcall Closed() = 0;
};
//---------------------------------------------------------------------------
// Duplicated in LogMemo.h for design-time-only purposes
enum TLogLineType { llOutput, llInput, llStdError, llMessage, llException };
enum TLogAction { laUpload, laDownload, laTouch, laChmod, laMkdir, laRm, laMv, laCall, laLs, laStat };
//---------------------------------------------------------------------------
typedef boost::signal2<void, const UnicodeString & /* Str */, bool /* StdError */> captureoutput_signal_type;
typedef captureoutput_signal_type::slot_type captureoutput_slot_type;
typedef boost::signal3<void, const UnicodeString & /* FileName */, const UnicodeString & /* Alg */, const UnicodeString & /* Hash */> calculatedchecksum_signal_type;
typedef calculatedchecksum_signal_type::slot_type calculatedchecksum_slot_type;
//---------------------------------------------------------------------------
// class TCriticalSection;
class TSessionActionRecord;
// class TSessionLog;
class TActionLog;
//---------------------------------------------------------------------------
class TSessionAction
{
public:
  explicit TSessionAction(TActionLog * Log, TLogAction Action);
  ~TSessionAction();

  void __fastcall Restart();

  void __fastcall Commit();
  void __fastcall Rollback(Exception * E = NULL);
  void __fastcall Cancel();

protected:
  TSessionActionRecord * FRecord;
};
//---------------------------------------------------------------------------
class TFileSessionAction : public TSessionAction
{
public:
    explicit TFileSessionAction(TActionLog * Log, TLogAction Action);
    explicit TFileSessionAction(TActionLog * Log, TLogAction Action, const UnicodeString & FileName);

  void __fastcall FileName(const UnicodeString & FileName);
};
//---------------------------------------------------------------------------
class TFileLocationSessionAction : public TFileSessionAction
{
public:
    explicit TFileLocationSessionAction(TActionLog * Log, TLogAction Action);
    explicit TFileLocationSessionAction(TActionLog * Log, TLogAction Action, const UnicodeString & FileName);

  void __fastcall Destination(const UnicodeString & Destination);
};
//---------------------------------------------------------------------------
class TUploadSessionAction : public TFileLocationSessionAction
{
public:
  explicit TUploadSessionAction(TActionLog * Log);
};
//---------------------------------------------------------------------------
class TDownloadSessionAction : public TFileLocationSessionAction
{
public:
  explicit TDownloadSessionAction(TActionLog * Log);
};
//---------------------------------------------------------------------------
class TRights;
//---------------------------------------------------------------------------
class TChmodSessionAction : public TFileSessionAction
{
public:
  explicit TChmodSessionAction(TActionLog * Log, const UnicodeString & FileName);
  explicit TChmodSessionAction(TActionLog * Log, const UnicodeString & FileName,
    const TRights & Rights);

  void __fastcall Rights(const TRights & Rights);
  void __fastcall Recursive();
};
//---------------------------------------------------------------------------
class TTouchSessionAction : public TFileSessionAction
{
public:
  explicit TTouchSessionAction(TActionLog * Log, const UnicodeString & FileName,
    const TDateTime & Modification);
};
//---------------------------------------------------------------------------
class TMkdirSessionAction : public TFileSessionAction
{
public:
    explicit TMkdirSessionAction(TActionLog * Log, const UnicodeString & FileName);
};
//---------------------------------------------------------------------------
class TRmSessionAction : public TFileSessionAction
{
public:
  explicit TRmSessionAction(TActionLog * Log, const UnicodeString & FileName);

  void __fastcall Recursive();
};
//---------------------------------------------------------------------------
class TMvSessionAction : public TFileLocationSessionAction
{
public:
  explicit TMvSessionAction(TActionLog * Log, const UnicodeString & FileName,
    const UnicodeString & Destination);
};
//---------------------------------------------------------------------------
class TCallSessionAction : public TSessionAction
{
public:
  explicit TCallSessionAction(TActionLog * Log, const UnicodeString & Command,
    const UnicodeString & Destination);

  void __fastcall AddOutput(const UnicodeString & Output, bool StdError);
};
//---------------------------------------------------------------------------
class TLsSessionAction : public TSessionAction
{
public:
  explicit TLsSessionAction(TActionLog * Log, const UnicodeString & Destination);

  void __fastcall FileList(TRemoteFileList * FileList);
};
//---------------------------------------------------------------------------
class TStatSessionAction : public TFileSessionAction
{
public:
  explicit TStatSessionAction(TActionLog * Log, const UnicodeString & FileName);

  void __fastcall File(TRemoteFile * File);
};
//---------------------------------------------------------------------------
typedef boost::signal2<void, TLogLineType, const std::wstring> doaddlog_signal_type;
typedef doaddlog_signal_type::slot_type doaddlog_slot_type;
//---------------------------------------------------------------------------
class TSessionLog : protected TStringList
{
    friend class TSessionAction;
    friend class TSessionActionRecord;
public:
  explicit TSessionLog(TSessionUI* UI, TSessionData * SessionData,
    TConfiguration *Configuration);
  virtual ~TSessionLog();
  virtual void __fastcall Add(TLogLineType Type, const UnicodeString & Line);
  void __fastcall AddStartupInfo();
  void __fastcall AddException(Exception *E);
  void __fastcall AddSeparator();

  virtual void __fastcall Clear();
  void __fastcall ReflectSettings();
  void __fastcall Lock();
  void __fastcall Unlock();

#ifndef _MSC_VER
  __property TSessionLog * Parent = { read = FParent, write = FParent };
  __property bool Logging = { read = FLogging };
  __property int BottomIndex = { read = GetBottomIndex };
  __property UnicodeString Line[int Index]  = { read=GetLine };
  __property TLogLineType Type[int Index]  = { read=GetType };
  __property OnChange;
  __property TNotifyEvent OnStateChange = { read = FOnStateChange, write = FOnStateChange };
  __property UnicodeString CurrentFileName = { read = FCurrentFileName };
  __property bool LoggingToFile = { read = GetLoggingToFile };
  __property int TopIndex = { read = FTopIndex };
  __property UnicodeString SessionName = { read = GetSessionName };
  __property UnicodeString Name = { read = FName, write = FName };
  __property Count;
#else
  TSessionLog * __fastcall GetParent() { return FParent; }
  void __fastcall SetParent(TSessionLog *value) { FParent = value; }
  bool __fastcall GetLogging() { return FLogging; }
  size_t __fastcall GetBottomIndex();
  UnicodeString __fastcall GetLine(size_t Index);
  TLogLineType __fastcall GetType(size_t Index);
  const System::notify_signal_type &GetOnStateChange() const { return FOnStateChange; }
  void SetOnStateChange(const System::notify_slot_type &value) { FOnStateChange.connect(value); }
  UnicodeString __fastcall GetCurrentFileName() { return FCurrentFileName; }
  bool __fastcall GetLoggingToFile();
  size_t __fastcall GetTopIndex() { return FTopIndex; }
  UnicodeString __fastcall GetSessionName();
  UnicodeString __fastcall GetName() { return FName; }
  void __fastcall SetName(const UnicodeString value) { FName = value; }
#endif

protected:
  void __fastcall CloseLogFile();
  bool __fastcall LogToFile();

private:
  TConfiguration * FConfiguration;
  TSessionLog * FParent;
  TCriticalSection * FCriticalSection;
  bool FLogging;
  void * FFile;
  UnicodeString FCurrentLogFileName;
  UnicodeString FCurrentFileName;
  int FLoggedLines;
  size_t FTopIndex;
  TSessionUI * FUI;
  TSessionData * FSessionData;
  UnicodeString FName;
  bool FClosed;
  notify_signal_type FOnStateChange;
  TSessionLog *Self;

  UnicodeString __fastcall GetLine(int Index);
  TLogLineType __fastcall GetType(int Index);
  void __fastcall DeleteUnnecessary();
  void __fastcall StateChange();
  void __fastcall OpenLogFile();
  UnicodeString __fastcall GetLogFileName();
  void __fastcall DoAdd(TLogLineType Type, UnicodeString Line,
             const doaddlog_slot_type &func);
  void DoAddToParent(TLogLineType aType, const UnicodeString & aLine);
  void DoAddToSelf(TLogLineType aType, const UnicodeString & aLine);
  void DoAddStartupInfo(TSessionData * Data);
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
  void __fastcall AddFailure(Exception * E);
  void __fastcall AddFailure(TStrings * Messages);
  void __fastcall BeginGroup(UnicodeString Name);
  void __fastcall EndGroup();

#ifndef _MSC_VER
  __property UnicodeString CurrentFileName = { read = FCurrentFileName };
  __property bool Enabled = { read = FEnabled, write = SetEnabled };
#else
  UnicodeString __fastcall GetCurrentFileName() const { return FCurrentFileName; };
  bool __fastcall GetEnabled() const { return FEnabled; }
  void __fastcall SetEnabled(bool value);
#endif

protected:
  void __fastcall CloseLogFile();
  inline void __fastcall AddPendingAction(TSessionActionRecord * Action);
  void __fastcall RecordPendingActions();
  void __fastcall Add(const UnicodeString & Line);
  void __fastcall AddIndented(const UnicodeString & Line);
  void __fastcall AddMessages(UnicodeString Indent, TStrings * Messages);

private:
  TConfiguration * FConfiguration;
  TCriticalSection * FCriticalSection;
  bool FLogging;
  void * FFile;
  UnicodeString FCurrentLogFileName;
  UnicodeString FCurrentFileName;
  TSessionUI * FUI;
  TSessionData * FSessionData;
  TList * FPendingActions;
  bool FClosed;
  bool FInGroup;
  UnicodeString FIndent;
  bool FEnabled;

  void __fastcall OpenLogFile();
  UnicodeString __fastcall GetLogFileName();
#ifndef _MSC_VER
  void __fastcall SetEnabled(bool value);
#endif

};
//---------------------------------------------------------------------------
#endif
