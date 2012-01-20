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

  TDateTime LoginTime;
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
  fcSecondaryShell, fcCount };
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
  virtual void Information(const std::wstring Str, bool Status) = 0;
  virtual int QueryUser(const std::wstring Query,
    TStrings * MoreMessages, int Answers, const TQueryParams * Params,
    TQueryType QueryType = qtConfirmation) = 0;
  virtual int QueryUserException(const std::wstring Query,
    const std::exception *E, int Answers, const TQueryParams * Params,
    TQueryType QueryType = qtConfirmation) = 0;
  virtual bool PromptUser(TSessionData *Data, TPromptKind Kind,
    const std::wstring Name, const std::wstring Instructions, TStrings *Prompts,
    TStrings * Results) = 0;
  virtual void DisplayBanner(const std::wstring Banner) = 0;
  virtual void FatalError(const std::exception * E, const std::wstring Msg) = 0;
  virtual void HandleExtendedException(const std::exception *E) = 0;
  virtual void Closed() = 0;
};
//---------------------------------------------------------------------------
// Duplicated in LogMemo.h for design-time-only purposes
enum TLogLineType { llOutput, llInput, llStdError, llMessage, llException, llAction };
enum TLogAction { laUpload, laDownload, laTouch, laChmod, laMkdir, laRm, laMv, laCall, laLs };
//---------------------------------------------------------------------------
// typedef void (TObject::*TCaptureOutputEvent)(
  // const std::wstring Str, bool StdError);
typedef boost::signal2<void, const std::wstring , bool> captureoutput_signal_type;
typedef captureoutput_signal_type::slot_type captureoutput_slot_type;
// typedef void (TObject::*TCalculatedChecksumEvent)(
  // const std::wstring FileName, const std::wstring Alg, const std::wstring Hash);
typedef boost::signal3<void, const std::wstring , const std::wstring , const std::wstring > calculatedchecksum_signal_type;
typedef calculatedchecksum_signal_type::slot_type calculatedchecksum_slot_type;
//---------------------------------------------------------------------------
class TCriticalSection;
class TSessionActionRecord;
class TSessionLog;
//---------------------------------------------------------------------------
class TSessionAction
{
public:
  TSessionAction(TSessionLog * Log, TLogAction Action);
  ~TSessionAction();

  void Restart();

  void Commit();
  void Rollback(const std::exception *E = NULL);
  void Cancel();

protected:
  TSessionActionRecord * FRecord;
};
//---------------------------------------------------------------------------
class TFileSessionAction : public TSessionAction
{
public:
  TFileSessionAction(TSessionLog * Log, TLogAction Action);
  TFileSessionAction(TSessionLog * Log, TLogAction Action, const std::wstring FileName);

  void FileName(const std::wstring FileName);
};
//---------------------------------------------------------------------------
class TFileLocationSessionAction : public TFileSessionAction
{
public:
  TFileLocationSessionAction(TSessionLog * Log, TLogAction Action);
  TFileLocationSessionAction(TSessionLog * Log, TLogAction Action, const std::wstring FileName);

  void Destination(const std::wstring Destination);
};
//---------------------------------------------------------------------------
class TUploadSessionAction : public TFileLocationSessionAction
{
public:
  TUploadSessionAction(TSessionLog * Log);
};
//---------------------------------------------------------------------------
class TDownloadSessionAction : public TFileLocationSessionAction
{
public:
  TDownloadSessionAction(TSessionLog * Log);
};
//---------------------------------------------------------------------------
class TRights;
//---------------------------------------------------------------------------
class TChmodSessionAction : public TFileSessionAction
{
public:
  TChmodSessionAction(TSessionLog * Log, const std::wstring FileName);
  TChmodSessionAction(TSessionLog * Log, const std::wstring FileName,
    const TRights & Rights);

  void Rights(const TRights & Rights);
  void Recursive();
};
//---------------------------------------------------------------------------
class TTouchSessionAction : public TFileSessionAction
{
public:
  TTouchSessionAction(TSessionLog * Log, const std::wstring FileName,
    const TDateTime & Modification);
};
//---------------------------------------------------------------------------
class TMkdirSessionAction : public TFileSessionAction
{
public:
  TMkdirSessionAction(TSessionLog * Log, const std::wstring FileName);
};
//---------------------------------------------------------------------------
class TRmSessionAction : public TFileSessionAction
{
public:
  TRmSessionAction(TSessionLog * Log, const std::wstring FileName);

  void Recursive();
};
//---------------------------------------------------------------------------
class TMvSessionAction : public TFileLocationSessionAction
{
public:
  TMvSessionAction(TSessionLog * Log, const std::wstring FileName,
    const std::wstring Destination);
};
//---------------------------------------------------------------------------
class TCallSessionAction : public TSessionAction
{
public:
  TCallSessionAction(TSessionLog * Log, const std::wstring Command,
    const std::wstring Destination);

  void AddOutput(const std::wstring Output, bool StdError);
};
//---------------------------------------------------------------------------
class TLsSessionAction : public TSessionAction
{
public:
  TLsSessionAction(TSessionLog * Log, const std::wstring Destination);

  void FileList(TRemoteFileList * FileList);
};
//---------------------------------------------------------------------------
// typedef void (TObject::*TDoAddLog)(TLogLineType Type, const std::wstring Line);
typedef boost::signal2<void, TLogLineType, const std::wstring > doaddlog_signal_type;
typedef doaddlog_signal_type::slot_type doaddlog_slot_type;
//---------------------------------------------------------------------------
class TSessionLog : protected TStringList
{
friend class TSessionAction;
friend class TSessionActionRecord;
public:
  TSessionLog(TSessionUI* UI, TSessionData * SessionData,
    TConfiguration * Configuration);
  ~TSessionLog();
  virtual void Add(TLogLineType Type, const std::wstring Line);
  void AddStartupInfo();
  void AddException(const std::exception * E);
  void AddSeparator();

  virtual void Clear();
  void ReflectSettings();
  void Lock();
  void Unlock();

  // __property TSessionLog * Parent = { read = FParent, write = FParent };
  TSessionLog * GetParent() { return FParent; }
  void SetParent(TSessionLog * value) { FParent = value; }
  // __property bool Logging = { read = FLogging };
  bool GetLogging() { return FLogging; }
  // __property int BottomIndex = { read = GetBottomIndex };
  int GetBottomIndex();
  // __property std::wstring Line[int Index]  = { read=GetLine };
  std::wstring GetLine(int Index);
  // __property TLogLineType Type[int Index]  = { read=GetType };
  TLogLineType GetType(int Index);
  // __property OnChange;
  // __property TNotifyEvent OnStateChange = { read = FOnStateChange, write = FOnStateChange };
  const notify_signal_type &GetOnStateChange() const { return FOnStateChange; }
  void SetOnStateChange(const notify_slot_type &value) { FOnStateChange.connect(value); }
  // __property std::wstring CurrentFileName = { read = FCurrentFileName };
  std::wstring GetCurrentFileName() { return FCurrentFileName; }
  // __property bool LoggingToFile = { read = GetLoggingToFile };
  bool GetLoggingToFile();
  // __property int TopIndex = { read = FTopIndex };
  int GetTopIndex() { return FTopIndex; }
  // __property std::wstring SessionName = { read = GetSessionName };
  std::wstring GetSessionName();
  // __property std::wstring Name = { read = FName, write = FName };
  std::wstring GetName() { return FName; }
  void SetName(const std::wstring value) { FName = value; }
  // __property Count;

protected:
  void CloseLogFile();
  bool LogToFile();
  inline void AddPendingAction(TSessionActionRecord * Action);
  void RecordPendingActions();

private:
  TConfiguration * FConfiguration;
  TSessionLog * FParent;
  TCriticalSection * FCriticalSection;
  bool FLogging;
  void * FFile;
  std::wstring FCurrentLogFileName;
  std::wstring FCurrentFileName;
  int FLoggedLines;
  int FTopIndex;
  TSessionUI * FUI;
  TSessionData * FSessionData;
  std::wstring FName;
  bool FLoggingActions;
  bool FClosed;
  TList * FPendingActions;
  notify_signal_type FOnStateChange;
  TSessionLog *Self;

  void DeleteUnnecessary();
  void StateChange();
  void OpenLogFile();
  std::wstring GetLogFileName();
  void DoAdd(TLogLineType Type, const std::wstring Line,
    const doaddlog_slot_type &func);
  void DoAddToParent(TLogLineType aType, const std::wstring aLine);
  void DoAddToSelf(TLogLineType aType, const std::wstring aLine);
  void DoAddStartupInfo(TSessionData * Data);
};
//---------------------------------------------------------------------------
#endif
