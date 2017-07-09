
#pragma once

#include "SessionData.h"
#include "Interface.h"

enum TSessionStatus
{
  ssClosed,
  ssOpening,
  ssOpened,
};

struct TSessionInfo
{
CUSTOM_MEM_ALLOCATION_IMPL
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

enum TFSCapability
{
  fcUserGroupListing = 0, fcModeChanging, fcGroupChanging,
  fcOwnerChanging, fcGroupOwnerChangingByID, fcAnyCommand, fcHardLink,
  fcSymbolicLink,
  // With WebDAV this is always true, to avoid double-click on
  // file try to open the file as directory. It does no harm atm as
  // WebDAV never produce a symlink in listing.
  fcResolveSymlink,
  fcTextMode, fcRename, fcNativeTextMode, fcNewerOnlyUpload, fcRemoteCopy,
  fcTimestampChanging, fcRemoteMove, fcLoadingAdditionalProperties,
  fcCheckingSpaceAvailable, fcIgnorePermErrors, fcCalculatingChecksum,
  fcModeChangingUpload, fcPreservingTimestampUpload, fcShellAnyCommand,
  fcSecondaryShell, fcRemoveCtrlZUpload, fcRemoveBOMUpload, fcMoveToQueue,
  fcLocking, fcPreservingTimestampDirs, fcResumeSupport,
  fcChangePassword, fsSkipTransfer, fsParallelTransfers,
  fcCount,
};

struct TFileSystemInfo
{
CUSTOM_MEM_ALLOCATION_IMPL
  TFileSystemInfo();

  UnicodeString ProtocolBaseName;
  UnicodeString ProtocolName;
  UnicodeString RemoteSystem;
  UnicodeString AdditionalInfo;
  bool IsCapable[fcCount];
};

class TSessionUI : public TObject
{
public:
  static inline bool classof(const TObject * Obj)
  {
    return
      Obj->GetKind() == OBJECT_CLASS_TSessionUI ||
      Obj->GetKind() == OBJECT_CLASS_TTerminal ||
      Obj->GetKind() == OBJECT_CLASS_TSecondaryTerminal ||
      Obj->GetKind() == OBJECT_CLASS_TBackgroundTerminal;
  }
public:
  explicit TSessionUI(TObjectClassId Kind) : TObject(Kind) {}
  virtual ~TSessionUI() {}
  virtual void Information(const UnicodeString & Str, bool Status) = 0;
  virtual uintptr_t QueryUser(const UnicodeString & Query,
    TStrings * MoreMessages, uintptr_t Answers, const TQueryParams * Params,
    TQueryType QueryType = qtConfirmation) = 0;
  virtual uintptr_t QueryUserException(const UnicodeString & Query,
    Exception * E, uintptr_t Answers, const TQueryParams * Params,
    TQueryType QueryType = qtConfirmation) = 0;
  virtual bool PromptUser(TSessionData * Data, TPromptKind Kind,
    const UnicodeString & Name, const UnicodeString & Instructions, TStrings * Prompts,
    TStrings * Results) = 0;
  virtual void DisplayBanner(const UnicodeString & Banner) = 0;
  virtual void FatalError(Exception * E, const UnicodeString & Msg, const UnicodeString & HelpKeyword = L"") = 0;
  virtual void HandleExtendedException(Exception * E) = 0;
  virtual void Closed() = 0;
  virtual void ProcessGUI() = 0;
};

// Duplicated in LogMemo.h for design-time-only purposes
enum TLogLineType
{
  llOutput,
  llInput,
  llStdError,
  llMessage,
  llException,
};

enum TLogAction
{
  laUpload, laDownload, laTouch, laChmod, laMkdir, laRm, laMv, laCall, laLs,
  laStat, laChecksum, laCwd
};

enum TCaptureOutputType { cotOutput, cotError, cotExitCode };
#if 0
typedef void (__closure *TCaptureOutputEvent)(
  const UnicodeString & Str, TCaptureOutputType OutputType);
#endif // #if 0
typedef nb::FastDelegate2<void,
  const UnicodeString & /*Str*/, TCaptureOutputType /*OutputType*/> TCaptureOutputEvent;
#if 0
typedef void (__closure *TCalculatedChecksumEvent)(
  const UnicodeString & FileName, const UnicodeString & Alg, const UnicodeString & Hash);
#endif // #if 0
typedef nb::FastDelegate3<void,
  const UnicodeString & /*FileName*/, const UnicodeString & /*Alg*/,
  const UnicodeString & /*Hash*/> TCalculatedChecksumEvent;

class TSessionActionRecord;
class TActionLog;

class TSessionAction
{
CUSTOM_MEM_ALLOCATION_IMPL
NB_DISABLE_COPY(TSessionAction)
public:
  explicit TSessionAction(TActionLog * Log, TLogAction Action);
  virtual ~TSessionAction();

  void Restart();

  void Commit();
  void Rollback(Exception * E = nullptr);
  void Cancel();

protected:
  TSessionActionRecord * FRecord;
};

class TFileSessionAction : public TSessionAction
{
public:
  explicit TFileSessionAction(TActionLog * Log, TLogAction Action);
  explicit TFileSessionAction(TActionLog * Log, TLogAction Action, const UnicodeString & AFileName);

  void SetFileName(const UnicodeString & AFileName);
};

class TFileLocationSessionAction : public TFileSessionAction
{
public:
  explicit TFileLocationSessionAction(TActionLog * Log, TLogAction Action);
  explicit TFileLocationSessionAction(TActionLog * Log, TLogAction Action, const UnicodeString & AFileName);

  void Destination(const UnicodeString & Destination);
};

class TUploadSessionAction : public TFileLocationSessionAction
{
public:
  explicit TUploadSessionAction(TActionLog * Log);
};

class TDownloadSessionAction : public TFileLocationSessionAction
{
public:
  explicit TDownloadSessionAction(TActionLog * Log);
};

class TRights;

class TChmodSessionAction : public TFileSessionAction
{
public:
  explicit TChmodSessionAction(TActionLog * Log, const UnicodeString & AFileName);
  explicit TChmodSessionAction(TActionLog * Log, const UnicodeString & AFileName,
    const TRights & ARights);

  void Rights(const TRights & Rights);
  void Recursive();
};

class TTouchSessionAction : public TFileSessionAction
{
public:
  explicit TTouchSessionAction(TActionLog * Log, const UnicodeString & AFileName,
    const TDateTime & Modification);
};

class TMkdirSessionAction : public TFileSessionAction
{
public:
  explicit TMkdirSessionAction(TActionLog * Log, const UnicodeString & AFileName);
};

class TRmSessionAction : public TFileSessionAction
{
public:
  explicit TRmSessionAction(TActionLog * Log, const UnicodeString & AFileName);

  void Recursive();
};

class TMvSessionAction : public TFileLocationSessionAction
{
public:
  explicit TMvSessionAction(TActionLog * Log, const UnicodeString & AFileName,
    const UnicodeString & ADestination);
};

class TCallSessionAction : public TSessionAction
{
public:
  explicit TCallSessionAction(TActionLog * Log, const UnicodeString & Command,
    const UnicodeString & Destination);

  void AddOutput(const UnicodeString & Output, bool StdError);
  void ExitCode(int ExitCode);
};

class TLsSessionAction : public TSessionAction
{
public:
  explicit TLsSessionAction(TActionLog * Log, const UnicodeString & Destination);

  void FileList(TRemoteFileList * FileList);
};

class TStatSessionAction : public TFileSessionAction
{
public:
  explicit TStatSessionAction(TActionLog * Log, const UnicodeString & AFileName);

  void File(TRemoteFile * AFile);
};

class TChecksumSessionAction : public TFileSessionAction
{
public:
  explicit TChecksumSessionAction(TActionLog * Log);

  void Checksum(const UnicodeString & Alg, const UnicodeString & Checksum);
};

class TCwdSessionAction : public TSessionAction
{
public:
  TCwdSessionAction(TActionLog * Log, const UnicodeString & Path);
};

#if 0
void (__closure *f)(TLogLineType Type, const UnicodeString & Line));
#endif // #if 0
typedef nb::FastDelegate2<void,
  TLogLineType /*Type*/, const UnicodeString & /*Line*/> TDoAddLogEvent;

class TSessionLog
{
CUSTOM_MEM_ALLOCATION_IMPL
friend class TSessionAction;
friend class TSessionActionRecord;
NB_DISABLE_COPY(TSessionLog)
public:
  explicit TSessionLog(TSessionUI * UI, TDateTime Started, TSessionData * SessionData,
    TConfiguration * Configuration);
  virtual ~TSessionLog();

  void SetParent(TSessionLog * AParent, const UnicodeString & AName);

  void Add(TLogLineType Type, const UnicodeString & ALine);
  void AddSystemInfo();
  void AddStartupInfo();
  void AddException(Exception * E);
  void AddSeparator();

  void ReflectSettings();

#if 0
  __property bool Logging = { read = FLogging };
  __property UnicodeString Name = { read = FName };
#endif // #if 0

  bool GetLogging() const { return FLogging; }
  UnicodeString GetName() const { return FName; }
  UnicodeString GetLogFileName() const { return FCurrentLogFileName; }
  bool LogToFile() const { return LogToFileProtected(); }

protected:
  void CloseLogFile();
  bool LogToFileProtected() const;

private:
  TConfiguration * FConfiguration;
  TSessionLog * FParent;
  TCriticalSection FCriticalSection;
  bool FLogging;
  void * FFile;
  UnicodeString FCurrentLogFileName;
  UnicodeString FCurrentFileName;
  int64_t FCurrentFileSize;
  TSessionUI * FUI;
  TSessionData * FSessionData;
  TDateTime FStarted;
  UnicodeString FName;
  bool FClosed;

  void OpenLogFile();
  void DoAdd(TLogLineType AType, UnicodeString ALine,
    TDoAddLogEvent Event);
  void DoAddToParent(TLogLineType AType, const UnicodeString & ALine);
  void DoAddToSelf(TLogLineType AType, const UnicodeString & ALine);
  void AddStartupInfo(bool System);
  void DoAddStartupInfo(TSessionData * Data);
  UnicodeString GetTlsVersionName(TTlsVersion TlsVersion) const;
  UnicodeString LogSensitive(const UnicodeString & Str);
  void AddOption(const UnicodeString & LogStr);
  void AddOptions(TOptions * Options);
  UnicodeString GetCmdLineLog() const;
  void CheckSize(int64_t Addition);
  UnicodeString LogPartFileName(const UnicodeString & BaseName, intptr_t Index);

public:
  UnicodeString GetLine(intptr_t Index) const;
  TLogLineType GetType(intptr_t Index) const;
  void DeleteUnnecessary();
  void StateChange();
};

class TActionLog : public TObject
{
friend class TSessionAction;
friend class TSessionActionRecord;
NB_DISABLE_COPY(TActionLog)
public:
  explicit TActionLog(TSessionUI * UI, TDateTime Started, TSessionData * SessionData,
    TConfiguration * Configuration);
  // For fatal failures for .NET assembly
  explicit TActionLog(TDateTime Started, TConfiguration * Configuration);
  virtual ~TActionLog();

  void ReflectSettings();
  void AddFailure(Exception * E);
  void AddFailure(TStrings * Messages);
  void BeginGroup(UnicodeString Name);
  void EndGroup();

#if 0
  __property UnicodeString CurrentFileName = { read = FCurrentFileName };
  __property bool Enabled = { read = FEnabled, write = SetEnabled };
#endif
  UnicodeString GetCurrentFileName() const { return FCurrentFileName; }
  bool GetEnabled() const { return FEnabled; }

protected:
  void CloseLogFile();
  inline void AddPendingAction(TSessionActionRecord * Action);
  void RecordPendingActions();
  void Add(const UnicodeString & Line);
  void AddIndented(const UnicodeString & Line);
  void AddMessages(const UnicodeString & Indent, TStrings * Messages);
  void Init(TSessionUI * UI, TDateTime Started, TSessionData * SessionData,
    TConfiguration * Configuration);

private:
  TConfiguration * FConfiguration;
  TCriticalSection FCriticalSection;
  bool FLogging;
  void * FFile;
  UnicodeString FCurrentLogFileName;
  UnicodeString FCurrentFileName;
  UnicodeString FIndent;
  TSessionUI * FUI;
  TSessionData * FSessionData;
  TDateTime FStarted;
  TList * FPendingActions;
  bool FFailed;
  bool FClosed;
  bool FInGroup;
  bool FEnabled;

  void OpenLogFile();

public:
  UnicodeString GetLogFileName() const { return FCurrentLogFileName; }
  void SetEnabled(bool Value);
};

