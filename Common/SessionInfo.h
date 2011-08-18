//---------------------------------------------------------------------------
#ifndef SessionInfoH
#define SessionInfoH

#include "SessionData.h"
#include "Interface.h"
//---------------------------------------------------------------------------
enum TSessionStatus { ssClosed, ssOpening, ssOpened };
//---------------------------------------------------------------------------
struct TSessionInfo
{
  TSessionInfo();

  TDateTime LoginTime;
  wstring ProtocolBaseName;
  wstring ProtocolName;
  wstring SecurityProtocolName;

  wstring CSCipher;
  wstring CSCompression;
  wstring SCCipher;
  wstring SCCompression;

  wstring SshVersionString;
  wstring SshImplementation;
  wstring HostKeyFingerprint;

  wstring CertificateFingerprint;
  wstring Certificate;
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

  wstring ProtocolBaseName;
  wstring ProtocolName;
  wstring RemoteSystem;
  wstring AdditionalInfo;
  bool IsCapable[fcCount];
};
//---------------------------------------------------------------------------
class TSessionUI
{
public:
  virtual void Information(const wstring & Str, bool Status) = 0;
  virtual int QueryUser(const wstring Query,
    TStrings * MoreMessages, int Answers, const TQueryParams * Params,
    TQueryType QueryType = qtConfirmation) = 0;
  virtual int QueryUserException(const wstring Query,
    exception * E, int Answers, const TQueryParams * Params,
    TQueryType QueryType = qtConfirmation) = 0;
  virtual bool PromptUser(TSessionData * Data, TPromptKind Kind,
    wstring Name, wstring Instructions, TStrings * Prompts,
    TStrings * Results) = 0;
  virtual void DisplayBanner(const wstring & Banner) = 0;
  virtual void FatalError(exception * E, wstring Msg) = 0;
  virtual void HandleExtendedException(exception * E) = 0;
  virtual void Closed() = 0;
};
//---------------------------------------------------------------------------
// Duplicated in LogMemo.h for design-time-only purposes
enum TLogLineType { llOutput, llInput, llStdError, llMessage, llException, llAction };
enum TLogAction { laUpload, laDownload, laTouch, laChmod, laMkdir, laRm, laMv, laCall, laLs };
//---------------------------------------------------------------------------
typedef void (TObject::*TCaptureOutputEvent)(
  const wstring & Str, bool StdError);
typedef void (TObject::*TCalculatedChecksumEvent)(
  const wstring & FileName, const wstring & Alg, const wstring & Hash);
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
  void Rollback(exception * E = NULL);
  void Cancel();

protected:
  TSessionActionRecord * FRecord;
};
//---------------------------------------------------------------------------
class TFileSessionAction : public TSessionAction
{
public:
  TFileSessionAction(TSessionLog * Log, TLogAction Action);
  TFileSessionAction(TSessionLog * Log, TLogAction Action, const wstring & FileName);

  void FileName(const wstring & FileName);
};
//---------------------------------------------------------------------------
class TFileLocationSessionAction : public TFileSessionAction
{
public:
  TFileLocationSessionAction(TSessionLog * Log, TLogAction Action);
  TFileLocationSessionAction(TSessionLog * Log, TLogAction Action, const wstring & FileName);

  void Destination(const wstring & Destination);
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
  TChmodSessionAction(TSessionLog * Log, const wstring & FileName);
  TChmodSessionAction(TSessionLog * Log, const wstring & FileName,
    const TRights & Rights);

  void Rights(const TRights & Rights);
  void Recursive();
};
//---------------------------------------------------------------------------
class TTouchSessionAction : public TFileSessionAction
{
public:
  TTouchSessionAction(TSessionLog * Log, const wstring & FileName,
    const TDateTime & Modification);
};
//---------------------------------------------------------------------------
class TMkdirSessionAction : public TFileSessionAction
{
public:
  TMkdirSessionAction(TSessionLog * Log, const wstring & FileName);
};
//---------------------------------------------------------------------------
class TRmSessionAction : public TFileSessionAction
{
public:
  TRmSessionAction(TSessionLog * Log, const wstring & FileName);

  void Recursive();
};
//---------------------------------------------------------------------------
class TMvSessionAction : public TFileLocationSessionAction
{
public:
  TMvSessionAction(TSessionLog * Log, const wstring & FileName,
    const wstring & Destination);
};
//---------------------------------------------------------------------------
class TCallSessionAction : public TSessionAction
{
public:
  TCallSessionAction(TSessionLog * Log, const wstring & Command,
    const wstring & Destination);

  void AddOutput(const wstring & Output, bool StdError);
};
//---------------------------------------------------------------------------
class TLsSessionAction : public TSessionAction
{
public:
  TLsSessionAction(TSessionLog * Log, const wstring & Destination);

  void FileList(TRemoteFileList * FileList);
};
//---------------------------------------------------------------------------
class TSessionLog : protected TStringList
{
friend class TSessionAction;
friend class TSessionActionRecord;
public:
  TSessionLog(TSessionUI* UI, TSessionData * SessionData,
    TConfiguration * Configuration);
  ~TSessionLog();
  virtual void Add(TLogLineType Type, const wstring & Line);
  void AddStartupInfo();
  void AddException(exception * E);
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
  // __property wstring Line[int Index]  = { read=GetLine };
  wstring GetLine(int Index);
  // __property TLogLineType Type[int Index]  = { read=GetType };
  TLogLineType GetType(int Index);
  // __property OnChange;
  // __property TNotifyEvent OnStateChange = { read = FOnStateChange, write = FOnStateChange };
  TNotifyEvent GetOnStateChange() { return FOnStateChange; }
  void SetOnStateChange(TNotifyEvent value) { FOnStateChange = value; }
  // __property wstring CurrentFileName = { read = FCurrentFileName };
  wstring GetCurrentFileName() { return FCurrentFileName; }
  // __property bool LoggingToFile = { read = GetLoggingToFile };
  bool GetLoggingToFile();
  // __property int TopIndex = { read = FTopIndex };
  int GetTopIndex() { return FTopIndex; }
  // __property wstring SessionName = { read = GetSessionName };
  wstring GetSessionName();
  // __property wstring Name = { read = FName, write = FName };
  wstring GetName() { return FName; }
  void SetName(wstring value) { FName = value; }
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
  wstring FCurrentLogFileName;
  wstring FCurrentFileName;
  int FLoggedLines;
  int FTopIndex;
  TSessionUI * FUI;
  TSessionData * FSessionData;
  wstring FName;
  bool FLoggingActions;
  bool FClosed;
  TList * FPendingActions;
  TNotifyEvent FOnStateChange;

  void DeleteUnnecessary();
  void StateChange();
  void OpenLogFile();
  wstring GetLogFileName();
  void DoAdd(TLogLineType Type, wstring Line,
    void (TObject::*f)(TLogLineType Type, const wstring & Line));
  void DoAddToParent(TLogLineType aType, const wstring & aLine);
  void DoAddToSelf(TLogLineType aType, const wstring & aLine);
  void DoAddStartupInfo(TSessionData * Data);
};
//---------------------------------------------------------------------------
#endif
