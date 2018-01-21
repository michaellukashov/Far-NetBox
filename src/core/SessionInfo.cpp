
#include <vcl.h>
#pragma hdrstop

#include <stdio.h>
#include <lmcons.h>
#define SECURITY_WIN32
#include <sspi.h>

#include <Common.h>
#include <nbutils.h>
#include <Exceptions.h>

#include "SessionInfo.h"
#include "TextsCore.h"
#include "Script.h"

static UnicodeString DoXmlEscape(UnicodeString AStr, bool NewLine)
{
  UnicodeString Str = AStr;
  for (intptr_t Index = 1; Index <= Str.Length(); ++Index)
  {
    const wchar_t *Repl = nullptr;
    UnicodeString Repl;
    switch (Str[Index])
    {
      case L'\x00': // \0 Is not valid in XML anyway
      case L'\x01':
      case L'\x02':
      case L'\x03':
      case L'\x04':
      case L'\x05':
      case L'\x06':
      case L'\x07':
      case L'\x08':
      // \n is handled below
      case L'\x0B':
      case L'\x0C':
      // \r is handled below
      case L'\x0E':
      case L'\x0F':
      case L'\x10':
      case L'\x11':
      case L'\x12':
      case L'\x13':
      case L'\x14':
      case L'\x15':
      case L'\x16':
      case L'\x17':
      case L'\x18':
      case L'\x19':
      case L'\x1A':
      case L'\x1B':
      case L'\x1C':
      case L'\x1D':
      case L'\x1E':
      case L'\x1F':
        Repl = L"#x" + ByteToHex((unsigned char)Str[i]) + L";";
        break;

    case L'&':
      Repl = L"amp;";
      break;

    case L'>':
      Repl = L"gt;";
      break;

    case L'<':
      Repl = L"lt;";
      break;

    case L'"':
      Repl = L"quot;";
      break;

    case L'\n':
      if (NewLine)
      {
        Repl = L"#10;";
      }
      break;

    case L'\r':
      Str.Delete(Index, 1);
      --Index;
      break;
    }

    if (!Repl.IsEmpty())
    {
      Str[Index] = L'&';
      Str.Insert(Repl, Index + 1);
      Index += Repl.Length();
    }
  }
  return Str;
}

static UnicodeString XmlEscape(UnicodeString Str)
{
  return DoXmlEscape(Str, false);
}

static UnicodeString XmlAttributeEscape(UnicodeString Str)
{
  return DoXmlEscape(Str, true);
}


#if 0
#pragma warn -inl
#endif // #if 0
class TSessionActionRecord : public TObject
{
public:
  static inline bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TSessionActionRecord); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TSessionActionRecord) || TObject::is(Kind); }
public:
  explicit TSessionActionRecord(TActionLog *Log, TLogAction Action) :
    TObject(OBJECT_CLASS_TSessionActionRecord),
    FLog(Log),
    FAction(Action),
    FState(Opened),
    FRecursive(false),
    FErrorMessages(nullptr),
    FNames(new TStringList()),
    FValues(new TStringList()),
    FFileList(nullptr),
    FFile(nullptr)
  {
    FLog->AddPendingAction(this);
  }

  ~TSessionActionRecord()
  {
    SAFE_DESTROY(FErrorMessages);
    SAFE_DESTROY(FNames);
    SAFE_DESTROY(FValues);
    SAFE_DESTROY(FFileList);
    SAFE_DESTROY(FFile);
  }

  void Restart()
  {
    FState = Opened;
    FRecursive = false;
    SAFE_DESTROY(FErrorMessages);
    SAFE_DESTROY(FFileList);
    SAFE_DESTROY(FFile);
    FNames->Clear();
    FValues->Clear();
  }

  bool Record()
  {
    bool Result = (FState != Opened);
#if 0
    if (Result)
    {
      if (FLog->FLogging && (FState != Cancelled))
      {
        const wchar_t *Name = ActionName();
        UnicodeString Attrs;
        if (FRecursive)
        {
          Attrs = L" recursive=\"true\"";
        }
        FLog->AddIndented(FORMAT("<%s%s>", Name,  Attrs));
        for (intptr_t Index = 0; Index < FNames->GetCount(); ++Index)
        {
          UnicodeString Value = FValues->GetString(Index);
          if (Value.IsEmpty())
          {
            FLog->AddIndented(FORMAT("  <%s />", FNames->GetString(Index)));
          }
          else
          {
            FLog->AddIndented(FORMAT("  <%s value=\"%s\" />",
                FNames->GetString(Index), XmlAttributeEscape(Value)));
          }
        }
        if (FFileList != nullptr)
        {
          FLog->AddIndented(L"  <files>");
          for (intptr_t Index = 0; Index < FFileList->GetCount(); ++Index)
          {
            TRemoteFile *File = FFileList->GetFile(Index);
            RecordFile(L"    ", File, true);
          }
          FLog->AddIndented(L"  </files>");
        }
        if (FFile != nullptr)
        {
          RecordFile(L"  ", FFile, false);
        }
        if (FState == RolledBack)
        {
          if (FErrorMessages != nullptr)
          {
            FLog->AddIndented(L"  <result success=\"false\">");
            FLog->AddMessages(L"    ", FErrorMessages);
            FLog->AddIndented(L"  </result>");
          }
          else
          {
            FLog->AddIndented(L"  <result success=\"false\" />");
          }
        }
        else
        {
          FLog->AddIndented(L"  <result success=\"true\" />");
        }
        FLog->AddIndented(FORMAT("</%s>", Name));
      }
      delete this;
    }
#endif
    return Result;
  }

  void Commit()
  {
    Close(Committed);
  }

  void Rollback(Exception *E)
  {
    DebugAssert(FErrorMessages == nullptr);
    FErrorMessages = ExceptionToMoreMessages(E);
    Close(RolledBack);
  }

  void Cancel()
  {
    Close(Cancelled);
  }

  void SetFileName(UnicodeString AFileName)
  {
    Parameter(L"filename", AFileName);
  }

  void Destination(UnicodeString Destination)
  {
    Parameter(L"destination", Destination);
  }

  void Rights(const TRights &Rights)
  {
    Parameter(L"permissions", Rights.GetText());
  }

  void Modification(const TDateTime &DateTime)
  {
    Parameter(L"modification", StandardTimestamp(DateTime));
  }

  void Recursive()
  {
    FRecursive = true;
  }

  void Command(UnicodeString Command)
  {
    Parameter(L"command", Command);
  }

  void AddOutput(UnicodeString Output, bool StdError)
  {
    const wchar_t *Name = (StdError ? L"erroroutput" : L"output");
    intptr_t Index = FNames->IndexOf(Name);
    if (Index >= 0)
    {
      FValues->SetString(Index, FValues->GetString(Index) + L"\r\n" + Output);
    }
    else
    {
      Parameter(Name, Output);
    }
  }

  void ExitCode(int ExitCode)
  {
    Parameter(L"exitcode", ::IntToStr(ExitCode));
  }

  void Checksum(UnicodeString Alg, UnicodeString Checksum)
  {
    Parameter(L"algorithm", Alg);
    Parameter(L"checksum", Checksum);
  }

  void Cwd(UnicodeString Path)
  {
    Parameter(L"cwd", Path);
  }

  void FileList(TRemoteFileList *FileList)
  {
    if (FFileList == nullptr)
    {
      FFileList = new TRemoteFileList();
    }
    FileList->DuplicateTo(FFileList);
  }

  void File(TRemoteFile *AFile)
  {
    if (FFile != nullptr)
    {
      SAFE_DESTROY(FFile);
    }
    FFile = AFile->Duplicate(true);
  }

protected:
  enum TState
  {
    Opened,
    Committed,
    RolledBack,
    Cancelled,
  };

  inline void Close(TState State)
  {
    DebugAssert(FState == Opened);
    FState = State;
    FLog->RecordPendingActions();
  }

  const wchar_t *ActionName() const
  {
    switch (FAction)
    {
    case laUpload: return L"upload";
    case laDownload: return L"download";
    case laTouch: return L"touch";
    case laChmod: return L"chmod";
    case laMkdir: return L"mkdir";
    case laRm: return L"rm";
    case laMv: return L"mv";
      case laCp: return L"cp";
    case laCall: return L"call";
    case laLs: return L"ls";
    case laStat: return L"stat";
    case laChecksum: return L"checksum";
    case laCwd: return L"cwd";
    default:
      DebugFail();
      return L"";
    }
  }

  void Parameter(UnicodeString Name, UnicodeString Value = L"")
  {
    FNames->Add(Name);
    FValues->Add(Value);
  }

  void __fastcall RecordFile(const UnicodeString AIndent, TRemoteFile *AFile, bool IncludeFileName)
  {
    FLog->AddIndented(AIndent + L"<file>");
    FLog->AddIndented(AIndent + FORMAT(L"  <filename value=\"%s\" />", (XmlAttributeEscape(AFile->FileName))));
    FLog->AddIndented(AIndent + FORMAT(L"  <type value=\"%s\" />", (XmlAttributeEscape(AFile->Type))));
    if (!AFile->IsDirectory)
    {
      FLog->AddIndented(AIndent + FORMAT(L"  <size value=\"%s\" />", (IntToStr(AFile->Size))));
    }
    if (AFile->ModificationFmt != mfNone)
    {
      FLog->AddIndented(AIndent + FORMAT(L"  <modification value=\"%s\" />", (StandardTimestamp(AFile->Modification))));
    }
    if (!AFile->Rights->Unknown)
    {
      FLog->AddIndented(AIndent + FORMAT(L"  <permissions value=\"%s\" />", (XmlAttributeEscape(AFile->Rights->Text))));
    }
    if (AFile->Owner.IsSet)
    {
      FLog->AddIndented(AIndent + FORMAT(L"  <owner value=\"%s\" />", (XmlAttributeEscape(AFile->Owner.DisplayText))));
    }
    if (AFile->Group.IsSet)
    {
      FLog->AddIndented(AIndent + FORMAT(L"  <group value=\"%s\" />", (XmlAttributeEscape(AFile->Group.DisplayText))));
    }
    FLog->AddIndented(AIndent + L"</file>");
  }

private:
  TActionLog *FLog;
  TLogAction FAction;
  TState FState;
  bool FRecursive;
  TStrings *FErrorMessages;
  TStrings *FNames;
  TStrings *FValues;
  TRemoteFileList *FFileList;
  TRemoteFile *FFile;
};
#if 0
#pragma warn .inl
#endif // #if 0


TSessionAction::TSessionAction(TActionLog *Log, TLogAction Action)
{
  if (Log->FLogging)
  {
    FRecord = new TSessionActionRecord(Log, Action);
  }
  else
  {
    FRecord = nullptr;
  }
}

TSessionAction::~TSessionAction()
{
  if (FRecord != nullptr)
  {
    Commit();
  }
}

void TSessionAction::Restart()
{
  if (FRecord != nullptr)
  {
    FRecord->Restart();
  }
}

void TSessionAction::Commit()
{
  if (FRecord != nullptr)
  {
    TSessionActionRecord *Record = FRecord;
    FRecord = nullptr;
    Record->Commit();
  }
}

void TSessionAction::Rollback(Exception *E)
{
  if (FRecord != nullptr)
  {
    TSessionActionRecord *Record = FRecord;
    FRecord = nullptr;
    Record->Rollback(E);
  }
}

void TSessionAction::Cancel()
{
  if (FRecord != nullptr)
  {
    TSessionActionRecord *Record = FRecord;
    FRecord = nullptr;
    Record->Cancel();
  }
}


TFileSessionAction::TFileSessionAction(TActionLog *Log, TLogAction Action) :
  TSessionAction(Log, Action)
{
}

TFileSessionAction::TFileSessionAction(
  TActionLog *Log, TLogAction Action, const UnicodeString AFileName) :
  TSessionAction(Log, Action)
{
  SetFileName(AFileName);
}

void TFileSessionAction::SetFileName(const UnicodeString AFileName)
{
  if (FRecord != nullptr)
  {
    FRecord->SetFileName(AFileName);
  }
}


TFileLocationSessionAction::TFileLocationSessionAction(
  TActionLog *Log, TLogAction Action) :
  TFileSessionAction(Log, Action)
{
}

TFileLocationSessionAction::TFileLocationSessionAction(
  TActionLog *Log, TLogAction Action, UnicodeString AFileName) :
  TFileSessionAction(Log, Action, AFileName)
{
}

void TFileLocationSessionAction::Destination(UnicodeString Destination)
{
  if (FRecord != nullptr)
  {
    FRecord->Destination(Destination);
  }
}


TUploadSessionAction::TUploadSessionAction(TActionLog *Log) :
  TFileLocationSessionAction(Log, laUpload)
{
}


TDownloadSessionAction::TDownloadSessionAction(TActionLog *Log) :
  TFileLocationSessionAction(Log, laDownload)
{
}


TChmodSessionAction::TChmodSessionAction(
  TActionLog *Log, UnicodeString AFileName) :
  TFileSessionAction(Log, laChmod, AFileName)
{
}

void TChmodSessionAction::Recursive()
{
  if (FRecord != nullptr)
  {
    FRecord->Recursive();
  }
}

TChmodSessionAction::TChmodSessionAction(
  TActionLog *Log, UnicodeString AFileName, const TRights &ARights) :
  TFileSessionAction(Log, laChmod, AFileName)
{
  Rights(ARights);
}

void TChmodSessionAction::Rights(const TRights &Rights)
{
  if (FRecord != nullptr)
  {
    FRecord->Rights(Rights);
  }
}

TTouchSessionAction::TTouchSessionAction(
  TActionLog *Log, UnicodeString AFileName, const TDateTime &Modification) :
  TFileSessionAction(Log, laTouch, AFileName)
{
  if (FRecord != nullptr)
  {
    FRecord->Modification(Modification);
  }
}

TMkdirSessionAction::TMkdirSessionAction(
  TActionLog *Log, UnicodeString AFileName) :
  TFileSessionAction(Log, laMkdir, AFileName)
{
}

TRmSessionAction::TRmSessionAction(
  TActionLog *Log, UnicodeString AFileName) :
  TFileSessionAction(Log, laRm, AFileName)
{
}

void TRmSessionAction::Recursive()
{
  if (FRecord != nullptr)
  {
    FRecord->Recursive();
  }
}

TMvSessionAction::TMvSessionAction(TActionLog *Log,
  const UnicodeString AFileName, const UnicodeString ADestination) :
  TFileLocationSessionAction(Log, laMv, AFileName)
{
  Destination(ADestination);
}

__fastcall TCpSessionAction::TCpSessionAction(TActionLog * Log,
    const UnicodeString AFileName, const UnicodeString ADestination) :
  TFileLocationSessionAction(Log, laCp, AFileName)
{
  Destination(ADestination);
}
//---------------------------------------------------------------------------
TCallSessionAction::TCallSessionAction(TActionLog *Log,
  UnicodeString ACommand, UnicodeString ADestination) :
  TSessionAction(Log, laCall)
{
  if (FRecord != nullptr)
  {
    FRecord->Command(ACommand);
    FRecord->Destination(ADestination);
  }
}

void TCallSessionAction::AddOutput(UnicodeString Output, bool StdError)
{
  if (FRecord != nullptr)
  {
    FRecord->AddOutput(Output, StdError);
  }
}

void TCallSessionAction::ExitCode(int ExitCode)
{
  if (FRecord != nullptr)
  {
    FRecord->ExitCode(ExitCode);
  }
}

TLsSessionAction::TLsSessionAction(TActionLog *Log,
  UnicodeString Destination) :
  TSessionAction(Log, laLs)
{
  if (FRecord != nullptr)
  {
    FRecord->Destination(Destination);
  }
}

void TLsSessionAction::FileList(TRemoteFileList *FileList)
{
  if (FRecord != nullptr)
  {
    FRecord->FileList(FileList);
  }
}


TStatSessionAction::TStatSessionAction(TActionLog *Log, UnicodeString AFileName) :
  TFileSessionAction(Log, laStat, AFileName)
{
}

void TStatSessionAction::File(TRemoteFile *AFile)
{
  if (FRecord != nullptr)
  {
    FRecord->File(AFile);
  }
}


TChecksumSessionAction::TChecksumSessionAction(TActionLog *Log) :
  TFileSessionAction(Log, laChecksum)
{
}

void TChecksumSessionAction::Checksum(UnicodeString Alg, UnicodeString Checksum)
{
  if (FRecord != nullptr)
  {
    FRecord->Checksum(Alg, Checksum);
  }
}


TCwdSessionAction::TCwdSessionAction(TActionLog *Log, UnicodeString Path) :
  TSessionAction(Log, laCwd)
{
  if (FRecord != nullptr)
  {
    FRecord->Cwd(Path);
  }
}

TSessionInfo::TSessionInfo() :
  LoginTime(Now())
{
}

TFileSystemInfo::TFileSystemInfo()
{
  ClearArray(IsCapable);
}

static FILE *LocalOpenLogFile(UnicodeString LogFileName, TDateTime Started, TSessionData *SessionData, bool Append, UnicodeString &ANewFileName)
{
  UnicodeString NewFileName = StripPathQuotes(GetExpandedLogFileName(LogFileName, Started, SessionData));
  FILE *Result = _wfsopen(ApiPath(NewFileName).c_str(),
      Append ? L"ab" : L"wb", SH_DENYWR);
  if (Result != nullptr)
  {
    setvbuf(Result, nullptr, _IONBF, BUFSIZ);
    ANewFileName = NewFileName;
  }
  else
  {
    throw ECRTExtException(FMTLOAD(LOG_OPENERROR, NewFileName));
  }
  return Result;
}

const wchar_t *LogLineMarks = L"<>!.*";

TSessionLog::TSessionLog(TSessionUI *UI, TDateTime Started, TSessionData *SessionData,
  TConfiguration *Configuration) :
  FConfiguration(Configuration),
  FParent(nullptr),
  FLogging(false),
  FLogger(nullptr),
  FCurrentFileSize(0),
  FUI(UI),
  FSessionData(SessionData),
  FStarted(Started),
  FClosed(false)
{
}

TSessionLog::~TSessionLog()
{
  FClosed = true;
  ReflectSettings();
  DebugAssert(FLogger == nullptr);
}

void TSessionLog::SetParent(TSessionLog *AParent, UnicodeString AName)
{
  FParent = AParent;
  FName = AName;
}

void TSessionLog::DoAddToParent(TLogLineType Type, UnicodeString ALine)
{
  DebugAssert(FParent != nullptr);
  FParent->Add(Type, ALine);
}

void TSessionLog::DoAddToSelf(TLogLineType Type, UnicodeString ALine)
{
  if (LogToFile()) { try
  {
    if (FLogger == nullptr)
    {
      OpenLogFile();
    }

    if (FLogger != nullptr)
    {
      UnicodeString Timestamp = FormatDateTime(L" yyyy-mm-dd hh:nn:ss.zzz ", Now());
      UTF8String UtfLine = UTF8String(UnicodeString(LogLineMarks[Type]) + Timestamp + TrimRight(ALine)) + "\r\n";
#if 0
      for (intptr_t Index = 1; Index <= UtfLine.Length(); Index++)
      {
        if ((UtfLine[Index] == '\n') &&
            (UtfLine[Index - 1] != '\r'))
        {
          UtfLine.Insert(L'\r', Index);
        }
      }
#endif // #if 0
      intptr_t ToWrite = UtfLine.Length();
      CheckSize(ToWrite);
      FCurrentFileSize += FLogger->Write(UtfLine.c_str(), ToWrite);
    }}
    catch (...)
    {
      // TODO: log error
      DEBUG_PRINTF("TSessionLog::DoAddToSelf: error");
    }
  }
}

UnicodeString TSessionLog::LogPartFileName(UnicodeString BaseName, intptr_t Index)
{
  UnicodeString Result;
  if (Index >= 1)
  {
    Result = FORMAT(L"%s.%d", BaseName, Index);
  }
  else
  {
    Result = BaseName;
  }
  return Result;
}

void TSessionLog::CheckSize(int64_t Addition)
{
  int64_t MaxSize = FConfiguration->GetLogMaxSize();
  if ((MaxSize > 0) && (FCurrentFileSize + Addition >= MaxSize))
  {
    // Before we close it
    UnicodeString BaseName = FCurrentFileName;
    CloseLogFile();
    FCurrentFileSize = 0;

    intptr_t Index = 0;

    while (FileExists(LogPartFileName(BaseName, Index + 1)))
    {
      Index++;
    }

    intptr_t MaxCount = FConfiguration->GetLogMaxCount();

    do
    {
      UnicodeString LogPart = LogPartFileName(BaseName, Index);
      if ((MaxCount > 0) && (Index >= MaxCount))
      {
        DeleteFileChecked(LogPart);
      }
      else
      {
        THROWOSIFFALSE(::RenameFile(LogPart, LogPartFileName(BaseName, Index + 1)));
      }
      Index--;
    }
    while (Index >= 0);

    OpenLogFile();
  }
}

void TSessionLog::DoAdd(TLogLineType AType, UnicodeString Line,
  TDoAddLogEvent Event)
{
  UnicodeString Prefix;

  if (!GetName().IsEmpty())
  {
    Prefix = L"[" + GetName() + L"] ";
  }

  while (!Line.IsEmpty())
  {
    Event(AType, Prefix + CutToChar(Line, L'\n', false));
  }
}

void TSessionLog::Add(TLogLineType Type, UnicodeString ALine)
{
  DebugAssert(FConfiguration);
  if (GetLogging())
  {
    try
    {
      if (FParent != nullptr)
      {
        DoAdd(Type, ALine, nb::bind(&TSessionLog::DoAddToParent, this));
      }
      else
      {
        TGuard Guard(FCriticalSection);

        DoAdd(Type, ALine, nb::bind(&TSessionLog::DoAddToSelf, this));
      }
    }
    catch (Exception &E)
    {
      // We failed logging, turn it off and notify user.
      FConfiguration->SetLogging(false);
      try
      {
        throw ExtException(&E, LoadStr(LOG_GEN_ERROR));
      }
      catch (Exception &E2)
      {
        AddException(&E2);
        FUI->HandleExtendedException(&E2);
      }
    }
  }
}

void TSessionLog::AddException(Exception *E)
{
  if (E != nullptr)
  {
    Add(llException, ExceptionLogString(E));
  }
}

void TSessionLog::ReflectSettings()
{
  TGuard Guard(FCriticalSection);

  FLogging =
    !FClosed &&
    ((FParent != nullptr) || FConfiguration->GetLogging());

  // if logging to file was turned off or log file was changed -> close current log file
  if ((FLogger != nullptr) &&
    (!LogToFile() || (FCurrentLogFileName != FConfiguration->GetLogFileName())))
  {
    CloseLogFile();
  }

  if (FLogger != nullptr)
  {
    CheckSize(0);
  }
}

bool TSessionLog::LogToFileProtected() const
{
  return GetLogging() && FConfiguration->GetLogToFile() && (FParent == nullptr);
}

void TSessionLog::CloseLogFile()
{
  if (FLogger != nullptr)
  {
    delete FLogger;
    FLogger = nullptr;
  }
  FCurrentLogFileName.Clear();
  FCurrentFileName.Clear();
}

void TSessionLog::OpenLogFile()
{
  DebugAssert(FConfiguration != nullptr);
  if (!FConfiguration)
    return;
  try
  {
    DebugAssert(FLogger == nullptr);
    FCurrentLogFileName = FConfiguration->GetLogFileName();
    FILE *file = LocalOpenLogFile(FCurrentLogFileName, FStarted, FSessionData, FConfiguration->GetLogFileAppend(), FCurrentFileName);
    FLogger = new tinylog::TinyLog(file);
    TSearchRec SearchRec;
    if (FileSearchRec(FCurrentFileName, SearchRec))
    {
      FCurrentFileSize = SearchRec.Size;
    }
    else
    {
      FCurrentFileSize = 0;
    }
  }
  catch (Exception &E)
  {
    // We failed logging to file, turn it off and notify user.
    FCurrentLogFileName.Clear();
    FCurrentFileName.Clear();
    FConfiguration->SetLogFileName(UnicodeString());
    try
    {
      throw ExtException(&E, LoadStr(LOG_GEN_ERROR));
    }
    catch (Exception &E2)
    {
      AddException(&E2);
      // not to deadlock with TSessionLog::ReflectSettings invoked by FConfiguration->LogFileName setter above
      TUnguard Unguard(FCriticalSection);
      FUI->HandleExtendedException(&E2);
    }
  }

  // in case we are appending and the existing log file is already too large
  if (FLogger != nullptr)
  {
    CheckSize(0);
  }
}

void TSessionLog::AddSystemInfo()
{
  AddStartupInfo(true);
}

void TSessionLog::AddStartupInfo()
{
  AddStartupInfo(false);
}

void TSessionLog::AddStartupInfo(bool System)
{
  TSessionData *Data = (System ? nullptr : FSessionData);
  if (GetLogging())
  {
    if (FParent != nullptr)
    {
      // do not add session info for secondary session
      // (this should better be handled in the TSecondaryTerminal)
    }
    else
    {
      DoAddStartupInfo(Data);
    }
  }
}

UnicodeString TSessionLog::GetTlsVersionName(TTlsVersion TlsVersion) const
{
  switch (TlsVersion)
  {
  default:
    DebugFail();
  case ssl2:
    return "SSLv2";
  case ssl3:
    return "SSLv3";
  case tls10:
    return "TLSv1.0";
  case tls11:
    return "TLSv1.1";
  case tls12:
    return "TLSv1.2";
  }
}

UnicodeString TSessionLog::LogSensitive(UnicodeString Str)
{
  if (FConfiguration->GetLogSensitive() && !Str.IsEmpty())
  {
    return Str;
  }
  return BooleanToEngStr(!Str.IsEmpty());
}

UnicodeString TSessionLog::GetCmdLineLog() const
{
  TODO("GetCmdLine()");
  UnicodeString Result = L"";

  if (!FConfiguration->GetLogSensitive())
  {
#if 0
    TManagementScript Script(StoredSessions, false);
    Script.MaskPasswordInCommandLine(Result, true);
#endif
  }

  return Result;
}

template <typename T>
UnicodeString EnumName(T Value, UnicodeString Names)
{
  intptr_t N = intptr_t(Value);

  do
  {
    UnicodeString Name = CutToChar(Names, L';', true);
    if (N == 0)
    {
      return Name;
    }
    --N;
  }
  while ((N >= 0) && !Names.IsEmpty());

  return L"(unknown)";
}

#define ADSTR(S) DoAdd(llMessage, S, nb::bind(&TSessionLog::DoAddToSelf, this));
#define ADF(S, ...) DoAdd(llMessage, FORMAT(S, __VA_ARGS__), nb::bind(&TSessionLog::DoAddToSelf, this));

void TSessionLog::DoAddStartupInfo(TSessionData *Data)
{
  if (Data == nullptr)
  {
    AddSeparator();
    UnicodeString OS = WindowsVersionLong();
    AddToList(OS, WindowsProductName(), L" - ");
    ADF("NetBox %s (OS %s)", FConfiguration->GetProductVersionStr(), OS);
    {
      std::unique_ptr<THierarchicalStorage> Storage(FConfiguration->CreateConfigStorage());
      DebugAssert(Storage.get());
      ADF("Configuration: %s", Storage->GetSource());
    }

#if 0
    typedef BOOL (WINAPI * TGetUserNameEx)(EXTENDED_NAME_FORMAT NameFormat, LPWSTR lpNameBuffer, PULONG nSize);
    HINSTANCE Secur32 = LoadLibrary(L"secur32.dll");
    TGetUserNameEx GetUserNameEx =
      (Secur32 != nullptr) ? reinterpret_cast<TGetUserNameEx>(::GetProcAddress(Secur32, "GetUserNameExW")) : nullptr;
    wchar_t UserName[UNLEN + 1];
    ULONG UserNameSize = _countof(UserName);
    if ((GetUserNameEx == nullptr) || DebugAlwaysFalse(!GetUserNameEx(NameSamCompatible, (LPWSTR)UserName, &UserNameSize)))
    {
      wcscpy_s(UserName, UNLEN, L"<Failed to retrieve username>");
    }
#endif // #if 0
    UnicodeString LogStr;
    if (FConfiguration->GetLogProtocol() <= 0)
    {
      LogStr = L"Normal";
    }
    else if (FConfiguration->GetLogProtocol() == 1)
    {
      LogStr = L"Debug 1";
    }
    else if (FConfiguration->GetLogProtocol() >= 2)
    {
      LogStr = L"Debug 2";
    }
#if 0
    if (FConfiguration->GetLogSensitive())
    {
      LogStr += L", Logging passwords";
    }
#endif // #if 0
    if (FConfiguration->GetLogMaxSize() > 0)
    {
      LogStr += FORMAT(L", Rotating after: %s", SizeToStr(FConfiguration->GetLogMaxSize()));
      if (FConfiguration->GetLogMaxCount() > 0)
      {
        LogStr += FORMAT(L", Keeping at most %d logs", FConfiguration->GetLogMaxCount());
      }
    }
#if 0
    ADF("Log level: %s", LogStr);
    ADF("Local account: %s", UserName);
#endif // #if 0
    ADF("Working directory: %s", GetCurrentDir());
    ADF("Process ID: %d", intptr_t(GetCurrentProcessId()));
#if 0
    ADF("Command-line: %s", GetCmdLineLog());
    if (FConfiguration->GetLogProtocol() >= 1)
    {
      AddOptions(GetGlobalOptions());
    }
#endif // #if 0
    ADF("Time zone: %s", GetTimeZoneLogString());
    if (!AdjustClockForDSTEnabled())
    {
      ADF("Warning: System option \"Automatically adjust clock for Daylight Saving Time\" is disabled, timestamps will not be represented correctly");
    }
#if 0
    ADF("Login time: %s", dt);
#endif // #if 0
    AddSeparator();
  }
  else
  {
#if 0
    ADF("Session name: %s (%s)", Data->GetSessionName(), Data->GetSource());
    ADF("Host name: %s (Port: %d)", Data->GetHostNameExpanded(), Data->GetPortNumber());
    ADF("User name: %s (Password: %s, Key file: %s, Passphrase: %s)",
      Data->GetUserNameExpanded(), LogSensitive(Data->GetPassword()),
      LogSensitive(Data->GetPublicKeyFile()), LogSensitive(Data->GetPassphrase()))
#endif // #if 0
    if (Data->GetUsesSsh())
    {
      ADF("Tunnel: %s", BooleanToEngStr(Data->GetTunnel()));
      if (Data->GetTunnel())
      {
        ADF("Tunnel: Host name: %s (Port: %d)", Data->GetTunnelHostName(), Data->GetTunnelPortNumber());
#if 0
        ADF("Tunnel: User name: %s (Password: %s, Key file: %s)",
          Data->GetTunnelUserName(), BooleanToEngStr(!Data->GetTunnelPassword().IsEmpty()),
          BooleanToEngStr(!Data->GetTunnelPublicKeyFile().IsEmpty()));
        ADF("Tunnel: Local port number: %d", Data->GetTunnelLocalPortNumber());
#endif // #if 0
      }
    }
    ADF("Transfer Protocol: %s", Data->GetFSProtocolStr());
    ADF("Code Page: %d", Data->GetCodePageAsNumber());
    if (Data->GetUsesSsh() || (Data->GetFSProtocol() == fsFTP))
    {
      TPingType PingType;
      intptr_t PingInterval;
      if (Data->GetFSProtocol() == fsFTP)
      {
        PingType = Data->GetFtpPingType();
        PingInterval = Data->GetFtpPingInterval();
      }
      else
      {
        PingType = Data->GetPingType();
        PingInterval = Data->GetPingInterval();
      }
      ADF("Ping type: %s, Ping interval: %d sec; Timeout: %d sec",
        EnumName(PingType, PingTypeNames), PingInterval, Data->GetTimeout());
      ADF("Disable Nagle: %s",
        BooleanToEngStr(Data->GetTcpNoDelay()));
    }
    TProxyMethod ProxyMethod = Data->GetActualProxyMethod();
    {
      UnicodeString fp = FORMAT(L"FTP proxy %d", Data->GetFtpProxyLogonType());
      ADF("Proxy: %s",
        (Data->GetFtpProxyLogonType() != 0) ? fp : EnumName(ProxyMethod, ProxyMethodNames));
    }
    if ((Data->GetFtpProxyLogonType() != 0) || (ProxyMethod != ::pmNone))
    {
      ADF("ProxyHostName: %s (Port: %d); ProxyUsername: %s; Passwd: %s",
        Data->GetProxyHost(), Data->GetProxyPort(),
        Data->GetProxyUsername(), BooleanToEngStr(!Data->GetProxyPassword().IsEmpty()));
      if (ProxyMethod == pmTelnet)
      {
        ADF("Telnet command: %s", Data->GetProxyTelnetCommand());
      }
      if (ProxyMethod == pmCmd)
      {
        ADF("Local command: %s", Data->GetProxyLocalCommand());
      }
    }
    if (Data->GetUsesSsh() || (Data->GetFSProtocol() == fsFTP))
    {
      ADF("Send buffer: %d", Data->GetSendBuf());
    }
    if (Data->GetUsesSsh())
    {
      ADF("SSH protocol version: %s; Compression: %s",
        Data->GetSshProtStr(), BooleanToEngStr(Data->GetCompression()));
      ADF("Bypass authentication: %s",
        BooleanToEngStr(Data->GetSshNoUserAuth()));
      ADF("Try agent: %s; Agent forwarding: %s; TIS/CryptoCard: %s; KI: %s; GSSAPI: %s",
        BooleanToEngStr(Data->GetTryAgent()), BooleanToEngStr(Data->GetAgentFwd()), BooleanToEngStr(Data->GetAuthTIS()),
        BooleanToEngStr(Data->GetAuthKI()), BooleanToEngStr(Data->GetAuthGSSAPI()));
      if (Data->GetAuthGSSAPI())
      {
        ADF("GSSAPI: Forwarding: %s; Libs: %s; Custom: %s",
          BooleanToEngStr(Data->GSSAPIFwdTGT), Data->GssLibList, Data->GssLibCustom);
      }
      ADF("Ciphers: %s; Ssh2DES: %s",
        Data->GetCipherList(), BooleanToEngStr(Data->GetSsh2DES()));
      ADF("KEX: %s", Data->GetKexList());
      UnicodeString Bugs;
      for (intptr_t Index = 0; Index < BUG_COUNT; ++Index)
      {
        AddToList(Bugs, EnumName(Data->GetBug(static_cast<TSshBug>(Index)), AutoSwitchNames), L",");
      }
      ADF("SSH Bugs: %s", Bugs);
      ADF("Simple channel: %s", BooleanToEngStr(Data->GetSshSimple()));
      ADF("Return code variable: %s; Lookup user groups: %s",
        Data->GetDetectReturnVar() ? UnicodeString(L"Autodetect") : Data->GetReturnVar(),
        EnumName(Data->GetLookupUserGroups(), AutoSwitchNames));
      ADF("Shell: %s", Data->GetShell().IsEmpty() ? UnicodeString(L"default") : Data->GetShell());
      ADF("EOL: %s, UTF: %s", EnumName(Data->GetEOLType(), EOLTypeNames), EnumName(Data->GetNotUtf(), NotAutoSwitchNames)); // NotUtf duplicated in FTP branch
      ADF("Clear aliases: %s, Unset nat.vars: %s, Resolve symlinks: %s; Follow directory symlinks: %s",
        BooleanToEngStr(Data->GetClearAliases()), BooleanToEngStr(Data->GetUnsetNationalVars()),
        BooleanToEngStr(Data->GetResolveSymlinks()), BooleanToEngStr(Data->GetFollowDirectorySymlinks()));
      ADF("LS: %s, Ign LS warn: %s, Scp1 Comp: %s",
        Data->GetListingCommand(),
        BooleanToEngStr(Data->GetIgnoreLsWarnings()),
        BooleanToEngStr(Data->GetScp1Compatibility()));
    }
    if ((Data->GetFSProtocol() == fsSFTP) || (Data->GetFSProtocol() == fsSFTPonly))
    {
      UnicodeString Bugs;
      for (intptr_t Index = 0; Index < SFTP_BUG_COUNT; ++Index)
      {
        AddToList(Bugs, EnumName(Data->GetSFTPBug(static_cast<TSftpBug>(Index)), AutoSwitchNames), L",");
      }
      ADF("SFTP Bugs: %s", Bugs);
      ADF("SFTP Server: %s", Data->GetSftpServer().IsEmpty() ? UnicodeString(L"default") : Data->GetSftpServer());
    }
    bool FtpsOn = false;
    if (Data->GetFSProtocol() == fsFTP)
    {
      ADF("UTF: %s", EnumName(Data->GetNotUtf(), NotAutoSwitchNames)); // duplicated in UsesSsh branch
      UnicodeString Ftps;
      switch (Data->GetFtps())
      {
      case ftpsImplicit:
        Ftps = L"Implicit TLS/SSL";
        FtpsOn = true;
        break;

      case ftpsExplicitSsl:
        Ftps = L"Explicit SSL/TLS";
        FtpsOn = true;
        break;

      case ftpsExplicitTls:
        Ftps = L"Explicit TLS/SSL";
        FtpsOn = true;
        break;

      default:
        DebugAssert(Data->GetFtps() == ftpsNone);
        Ftps = L"None";
        break;
      }
      // kind of hidden option, so do not reveal it unless it is set
      if (Data->GetFtpTransferActiveImmediately() != asAuto)
      {
        ADF("Transfer active immediately: %s", EnumName(Data->GetFtpTransferActiveImmediately(), AutoSwitchNames));
      }
      ADF("FTPS: %s [Client certificate: %s]",
        Ftps, LogSensitive(Data->GetTlsCertificateFile()));
      ADF("FTP: Passive: %s [Force IP: %s]; MLSD: %s [List all: %s]; HOST: %s",
        BooleanToEngStr(Data->GetFtpPasvMode()),
        EnumName(Data->GetFtpForcePasvIp(), AutoSwitchNames),
        EnumName(Data->GetFtpUseMlsd(), AutoSwitchNames),
        EnumName(Data->GetFtpListAll(), AutoSwitchNames),
        EnumName(Data->GetFtpHost(), AutoSwitchNames));
    }
    if (Data->GetFSProtocol() == fsWebDAV)
    {
      FtpsOn = (Data->GetFtps() != ftpsNone);
      ADF("HTTPS: %s [Client certificate: %s]",
        BooleanToEngStr(FtpsOn), LogSensitive(Data->GetTlsCertificateFile()));
    }
    if (Data->FSProtocol == fsS3)
    {
      FtpsOn = (Data->Ftps != ftpsNone);
      ADF(L"HTTPS: %s", (BooleanToEngStr(FtpsOn)));
      if (!Data->S3DefaultRegion.IsEmpty())
      {
        ADF(L"S3: Default region: %s", (Data->S3DefaultRegion));
      }
    }
    if (FtpsOn)
    {
      if (Data->GetFSProtocol() == fsFTP)
      {
        ADF("Session reuse: %s", BooleanToEngStr(Data->GetSslSessionReuse()));
      }
      ADF("TLS/SSL versions: %s-%s", GetTlsVersionName(Data->GetMinTlsVersion()), GetTlsVersionName(Data->GetMaxTlsVersion()));
    }
    ADF("Local directory: %s, Remote directory: %s, Update: %s, Cache: %s",
      Data->GetLocalDirectory().IsEmpty() ? UnicodeString(L"default") : Data->GetLocalDirectory(),
      Data->GetRemoteDirectory().IsEmpty() ? UnicodeString(L"home") : Data->GetRemoteDirectory(),
      BooleanToEngStr(Data->GetUpdateDirectories()),
      BooleanToEngStr(Data->GetCacheDirectories()));
    ADF("Cache directory changes: %s, Permanent: %s",
      BooleanToEngStr(Data->GetCacheDirectoryChanges()),
      BooleanToEngStr(Data->GetPreserveDirectoryChanges()));
    ADF("Recycle bin: Delete to: %s, Overwritten to: %s, Bin path: %s",
      BooleanToEngStr(Data->GetDeleteToRecycleBin()),
      BooleanToEngStr(Data->GetOverwrittenToRecycleBin()),
      Data->GetRecycleBinPath());
    if (Data->GetTrimVMSVersions())
    {
      ADF("Trim VMS versions: %s",
        BooleanToEngStr(Data->GetTrimVMSVersions()));
    }
    UnicodeString TimeInfo;
    if ((Data->GetFSProtocol() == fsSFTP) || (Data->GetFSProtocol() == fsSFTPonly) || (Data->GetFSProtocol() == fsSCPonly) || (Data->GetFSProtocol() == fsWebDAV))
    {
      AddToList(TimeInfo, FORMAT(L"DST mode: %s", EnumName(ToInt(Data->GetDSTMode()), DSTModeNames)), L";");
    }
    if ((Data->GetFSProtocol() == fsSCPonly) || (Data->GetFSProtocol() == fsFTP))
    {
      intptr_t TimeDifferenceMin = TimeToMinutes(Data->GetTimeDifference());
      AddToList(TimeInfo, FORMAT(L"Timezone offset: %dh %dm", TimeDifferenceMin / MinsPerHour, TimeDifferenceMin % MinsPerHour), L";");
    }
    ADSTR(TimeInfo);

    if (Data->GetFSProtocol() == fsWebDAV)
    {
      ADF("Compression: %s",
        BooleanToEngStr(Data->GetCompression()));
    }

    AddSeparator();
  }
}

void TSessionLog::AddOption(UnicodeString LogStr)
{
  ADSTR(LogStr);
}

void TSessionLog::AddOptions(TOptions *Options)
{
  Options->LogOptions(nb::bind(&TSessionLog::AddOption, this));
}

#undef ADF
#undef ADSTR

void TSessionLog::AddSeparator()
{
  Add(llMessage, L"--------------------------------------------------------------------------");
}


TActionLog::TActionLog(TSessionUI *UI, TDateTime Started, TSessionData *SessionData,
  TConfiguration *Configuration) :
  FConfiguration(Configuration),
  FLogging(false),
  FLogger(nullptr),
  FIndent(L"  "),
  FUI(UI),
  FSessionData(SessionData),
  FPendingActions(new TList()),
  FFailed(false),
  FClosed(false),
  FInGroup(false),
  FEnabled(true)
{
  DebugAssert(UI != nullptr);
  DebugAssert(SessionData != nullptr);
  Init(UI, Started, SessionData, Configuration);
}

TActionLog::TActionLog(TDateTime Started, TConfiguration *Configuration)
{
  Init(nullptr, Started, nullptr, Configuration);
  // not associated with session, so no need to waiting for anything
  ReflectSettings();
}

void TActionLog::Init(TSessionUI *UI, TDateTime Started, TSessionData *SessionData,
  TConfiguration *Configuration)
{
  FConfiguration = Configuration;
  FUI = UI;
  FSessionData = SessionData;
  FStarted = Started;
  FLogger = nullptr;
  FCurrentLogFileName.Clear();
  FCurrentFileName.Clear();
  FLogging = false;
  FClosed = false;
  FFailed = false;
  FPendingActions = new TList();
  FIndent = L"  ";
  FInGroup = false;
  FEnabled = true;
}

TActionLog::~TActionLog()
{
  DebugAssert(FPendingActions->GetCount() == 0);
  SAFE_DESTROY(FPendingActions);
  FClosed = true;
  ReflectSettings();
  DebugAssert(FLogger == nullptr);
}

void TActionLog::Add(UnicodeString Line)
{
  DebugAssert(FConfiguration);
  if (FLogging)
  {
    TGuard Guard(FCriticalSection);
    if (FLogger == nullptr)
    {
      OpenLogFile();
    }

    if (FLogger != nullptr)
    {
      try
      {
        UTF8String UtfLine = UTF8String(Line);
        size_t Written =
          FLogger->Write(UtfLine.c_str(), UtfLine.Length());
        if (Written != static_cast<size_t>(UtfLine.Length()))
        {
          throw ECRTExtException(L"");
        }
        Written =
          FLogger->Write("\n", 1);
        if (Written != 1)
        {
          throw ECRTExtException(L"");
        }
      }
      catch (Exception &E)
      {
//        FCriticalSection.Release();

        // avoid endless loop when trying to close tags when closing log, when logging has failed
        if (!FFailed)
        {
          FFailed = true;
          // We failed logging, turn it off and notify user.
          FConfiguration->SetLogActions(false);
          if (FConfiguration->GetLogActionsRequired())
          {
            throw EFatal(&E, LoadStr(LOG_FATAL_ERROR));
          }
          else
          {
            try
            {
              throw ExtException(&E, LoadStr(LOG_GEN_ERROR));
            }
            catch (Exception &E2)
            {
              if (FUI != nullptr)
              {
                FUI->HandleExtendedException(&E2);
              }
            }
          }
        }
      }
    }
  }
}

void TActionLog::AddIndented(UnicodeString Line)
{
  Add(FIndent + Line);
}

void TActionLog::AddFailure(TStrings *Messages)
{
  AddIndented(L"<failure>");
  AddMessages(L"  ", Messages);
  AddIndented(L"</failure>");
}

void TActionLog::AddFailure(Exception *E)
{
  std::unique_ptr<TStrings> Messages(ExceptionToMoreMessages(E));
  if (Messages.get() != nullptr)
  {
    try__finally
    {
      AddFailure(Messages.get());
    }
    __finally
    {
#if 0
      delete Messages;
#endif // #if 0
    };
  }
}

void TActionLog::AddMessages(UnicodeString Indent, TStrings *Messages)
{
  for (intptr_t Index = 0; Index < Messages->GetCount(); ++Index)
  {
    AddIndented(
      FORMAT((Indent + L"<message>%s</message>"), XmlEscape(Messages->GetString(Index))));
  }
}

void TActionLog::ReflectSettings()
{
  TGuard Guard(FCriticalSection);

  bool ALogging =
    !FClosed && FConfiguration->GetLogActions() && GetEnabled();

  if (ALogging && !FLogging)
  {
    FLogging = true;
#if 0
    Add(L"<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
    UnicodeString SessionName =
      (FSessionData != nullptr) ? XmlAttributeEscape(FSessionData->SessionName) : UnicodeString(L"nosession");
    Add(FORMAT(L"<session xmlns=\"http://winscp.net/schema/session/1.0\" name=\"%s\" start=\"%s\">",
        (SessionName, StandardTimestamp())));
#endif // #if 0
  }
  else if (!ALogging && FLogging)
  {
    if (FInGroup)
    {
      EndGroup();
    }
    // do not try to close the file, if it has not been opened, to avoid recursion
    if (FLogger != nullptr)
    {
#if 0
      Add(L"</session>");
#endif // #if 0
    }
    CloseLogFile();
    FLogging = false;
  }
}

void TActionLog::CloseLogFile()
{
  if (FLogger != nullptr)
  {
    delete FLogger;
    FLogger = nullptr;
  }
  FCurrentLogFileName.Clear();
  FCurrentFileName.Clear();
}

void TActionLog::OpenLogFile()
{
  DebugAssert(FConfiguration != nullptr);
  if (!FConfiguration)
    return;
  try
  {
    DebugAssert(FLogger == nullptr);
    FCurrentLogFileName = FConfiguration->GetActionsLogFileName();
    FILE *file = LocalOpenLogFile(FCurrentLogFileName, FStarted, FSessionData, false, FCurrentFileName);
    FLogger = new tinylog::TinyLog(file);
  }
  catch (Exception &E)
  {
    // We failed logging to file, turn it off and notify user.
    FCurrentLogFileName.Clear();
    FCurrentFileName.Clear();
    FConfiguration->SetLogActions(false);
    if (FConfiguration->GetLogActionsRequired())
    {
      throw EFatal(&E, LoadStr(LOG_FATAL_ERROR));
    }
    else
    {
      try
      {
        throw ExtException(&E, LoadStr(LOG_GEN_ERROR));
      }
      catch (Exception &E2)
      {
        if (FUI != nullptr)
        {
          // not to deadlock with TSessionLog::ReflectSettings invoked by FConfiguration->LogFileName setter above
          TUnguard Unguard(FCriticalSection);
          FUI->HandleExtendedException(&E2);
        }
      }
    }
  }
}

void TActionLog::AddPendingAction(TSessionActionRecord *Action)
{
  FPendingActions->Add(Action);
}

void TActionLog::RecordPendingActions()
{
  while ((FPendingActions->GetCount() > 0) &&
    FPendingActions->GetAs<TSessionActionRecord>(0)->Record())
  {
    FPendingActions->Delete(0);
  }
}

void TActionLog::BeginGroup(UnicodeString Name)
{
  DebugAssert(!FInGroup);
  FInGroup = true;
  DebugAssert(FIndent == L"  ");
  AddIndented(FORMAT("<group name=\"%s\" start=\"%s\">",
      XmlAttributeEscape(Name), StandardTimestamp()));
  FIndent = L"    ";
}

void TActionLog::EndGroup()
{
  DebugAssert(FInGroup);
  FInGroup = false;
  DebugAssert(FIndent == L"    ");
  FIndent = L"  ";
  // this is called from ReflectSettings that in turn is called when logging fails,
  // so do not try to close the group, if it has not been opened, to avoid recursion
  if (FLogger != nullptr)
  {
    AddIndented("</group>");
  }
}

void TActionLog::SetEnabled(bool Value)
{
  if (GetEnabled() != Value)
  {
    FEnabled = Value;
    ReflectSettings();
  }
}

