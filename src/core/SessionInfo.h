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
  explicit /* __fastcall */ TSessionUI() {}
  virtual /* __fastcall */ ~TSessionUI() {}
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
#ifndef _MSC_VER
typedef void __fastcall (__closure *TCaptureOutputEvent)(
  const UnicodeString & Str, bool StdError);
typedef void __fastcall (__closure *TCalculatedChecksumEvent)(
  const UnicodeString & FileName, const UnicodeString & Alg, const UnicodeString & Hash);
#else
typedef fastdelegate::FastDelegate2<void,
  const UnicodeString & /* Str */, bool /* StdError */> TCaptureOutputEvent;
typedef fastdelegate::FastDelegate3<void,
  const UnicodeString & /* FileName */, const UnicodeString & /* Alg */, const UnicodeString & /* Hash */> TCalculatedChecksumEvent;
#endif
//---------------------------------------------------------------------------
class TSessionActionRecord;
class TActionLog;
//---------------------------------------------------------------------------
class TSessionAction
{
public:
  explicit /* __fastcall */ TSessionAction(TActionLog * Log, TLogAction Action);
  virtual /* __fastcall */ ~TSessionAction();

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
    explicit /* __fastcall */ TFileSessionAction(TActionLog * Log, TLogAction Action);
    explicit /* __fastcall */ TFileSessionAction(TActionLog * Log, TLogAction Action, const UnicodeString & FileName);

  void __fastcall FileName(const UnicodeString & FileName);
};
//---------------------------------------------------------------------------
class TFileLocationSessionAction : public TFileSessionAction
{
public:
    explicit /* __fastcall */ TFileLocationSessionAction(TActionLog * Log, TLogAction Action);
    explicit /* __fastcall */ TFileLocationSessionAction(TActionLog * Log, TLogAction Action, const UnicodeString & FileName);

  void __fastcall Destination(const UnicodeString & Destination);
};
//---------------------------------------------------------------------------
class TUploadSessionAction : public TFileLocationSessionAction
{
public:
  explicit /* __fastcall */ TUploadSessionAction(TActionLog * Log);
};
//---------------------------------------------------------------------------
class TDownloadSessionAction : public TFileLocationSessionAction
{
public:
  explicit /* __fastcall */ TDownloadSessionAction(TActionLog * Log);
};
//---------------------------------------------------------------------------
class TRights;
//---------------------------------------------------------------------------
class TChmodSessionAction : public TFileSessionAction
{
public:
  explicit /* __fastcall */ TChmodSessionAction(TActionLog * Log, const UnicodeString & FileName);
  explicit /* __fastcall */ TChmodSessionAction(TActionLog * Log, const UnicodeString & FileName,
    const TRights & Rights);

  void __fastcall Rights(const TRights & Rights);
  void __fastcall Recursive();
};
//---------------------------------------------------------------------------
class TTouchSessionAction : public TFileSessionAction
{
public:
  explicit /* __fastcall */ TTouchSessionAction(TActionLog * Log, const UnicodeString & FileName,
    const TDateTime & Modification);
};
//---------------------------------------------------------------------------
class TMkdirSessionAction : public TFileSessionAction
{
public:
  explicit /* __fastcall */ TMkdirSessionAction(TActionLog * Log, const UnicodeString & FileName);
};
//---------------------------------------------------------------------------
class TRmSessionAction : public TFileSessionAction
{
public:
  explicit /* __fastcall */ TRmSessionAction(TActionLog * Log, const UnicodeString & FileName);

  void __fastcall Recursive();
};
//---------------------------------------------------------------------------
class TMvSessionAction : public TFileLocationSessionAction
{
public:
  explicit /* __fastcall */ TMvSessionAction(TActionLog * Log, const UnicodeString & FileName,
    const UnicodeString & Destination);
};
//---------------------------------------------------------------------------
class TCallSessionAction : public TSessionAction
{
public:
  explicit /* __fastcall */ TCallSessionAction(TActionLog * Log, const UnicodeString & Command,
    const UnicodeString & Destination);

  void __fastcall AddOutput(const UnicodeString & Output, bool StdError);
};
//---------------------------------------------------------------------------
class TLsSessionAction : public TSessionAction
{
public:
  explicit /* __fastcall */ TLsSessionAction(TActionLog * Log, const UnicodeString & Destination);

  void __fastcall FileList(TRemoteFileList * FileList);
};
//---------------------------------------------------------------------------
class TStatSessionAction : public TFileSessionAction
{
public:
  explicit /* __fastcall */ TStatSessionAction(TActionLog * Log, const UnicodeString & FileName);

  void __fastcall File(TRemoteFile * File);
};
//---------------------------------------------------------------------------
typedef fastdelegate::FastDelegate2<void,
  TLogLineType, UnicodeString> TDoAddLogEvent;
//---------------------------------------------------------------------------
class TSessionLog : protected TStringList
{
friend class TSessionAction;
friend class TSessionActionRecord;
public:
  explicit /* __fastcall */ TSessionLog(TSessionUI* UI, TSessionData * SessionData,
    TConfiguration * Configuration);
  virtual /* __fastcall */ ~TSessionLog();
  HIDESBASE  void __fastcall Add(TLogLineType Type, const UnicodeString & Line);
  void __fastcall AddStartupInfo();
  void __fastcall AddException(Exception * E);
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
  TNotifyEvent GetOnStateChange() { return FOnStateChange; }
  void SetOnStateChange(TNotifyEvent value) { FOnStateChange = value; }
  UnicodeString __fastcall GetCurrentFileName() { return FCurrentFileName; }
  size_t __fastcall GetTopIndex() { return FTopIndex; }
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
  int FTopIndex;
  TSessionUI * FUI;
  TSessionData * FSessionData;
  UnicodeString FName;
  bool FClosed;
  TNotifyEvent FOnStateChange;
  TSessionLog *Self;

public:
  UnicodeString __fastcall GetLine(int Index);
  TLogLineType __fastcall GetType(int Index);
  void __fastcall DeleteUnnecessary();
  void __fastcall StateChange();
  void __fastcall OpenLogFile();
  int __fastcall GetBottomIndex();
  UnicodeString __fastcall GetLogFileName();
  bool __fastcall GetLoggingToFile();
  UnicodeString __fastcall GetSessionName();
  void __fastcall DoAdd(TLogLineType Type, UnicodeString Line,
    // void __fastcall (__closure *f)(TLogLineType Type, const UnicodeString & Line));
    TDoAddLogEvent Event);
  void /* __fastcall */ DoAddToParent(TLogLineType aType, UnicodeString aLine);
  void /* __fastcall */ DoAddToSelf(TLogLineType aType, UnicodeString aLine);
  void /* __fastcall */ DoAddStartupInfo(TSessionData * Data);
};
//---------------------------------------------------------------------------
class TActionLog
{
friend class TSessionAction;
friend class TSessionActionRecord;
public:
  explicit /* __fastcall */ TActionLog(TSessionUI* UI, TSessionData * SessionData,
    TConfiguration * Configuration);
  virtual /* __fastcall */ ~TActionLog();

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

public:
  void __fastcall OpenLogFile();
  UnicodeString __fastcall GetLogFileName();
  void __fastcall SetEnabled(bool value);
};
//---------------------------------------------------------------------------
#endif
