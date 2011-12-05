//---------------------------------------------------------------------------
#include "stdafx.h"

#include "boostdefines.hpp"
#include <boost/scope_exit.hpp>
#include <boost/bind.hpp>

#include <stdio.h>

#include "Common.h"
#include "SessionInfo.h"
#include "Exceptions.h"
#include "TextsCore.h"
//---------------------------------------------------------------------------
std::wstring XmlEscape(std::wstring Str)
{
  for (size_t i = 0; i < Str.size(); i++)
  {
    const wchar_t * Repl = NULL;
    switch (Str[i])
    {
      case '&':
        Repl = L"amp;";
        break;

      case '>':
        Repl = L"gt;";
        break;

      case '<':
        Repl = L"lt;";
        break;

      case '"':
        Repl = L"quot;";
        break;

      case '\r':
        Str.erase(i, 1);
        i--;
        break;
    }

    if (Repl != NULL)
    {
      Str[i] = '&';
      Str.insert(i + 1, std::wstring(Repl));
      i += wcslen(Repl);
    }
  }
  // Str = AnsiToUtf8(Str);
  return Str;
}
//---------------------------------------------------------------------------
std::wstring XmlTimestamp(const TDateTime & DateTime)
{
  return FormatDateTime(L"yyyy'-'mm'-'dd'T'hh':'nn':'ss'.'zzz'Z'", ConvertTimestampToUTC(DateTime));
}
//---------------------------------------------------------------------------
std::wstring XmlTimestamp()
{
  return XmlTimestamp(Now());
}
//---------------------------------------------------------------------------
// #pragma warn -inl
class TSessionActionRecord
{
public:
  TSessionActionRecord(TSessionLog * Log, TLogAction Action) :
    FLog(Log),
    FAction(Action),
    FState(Opened),
    FRecursive(false),
    FErrorMessages(NULL),
    FNames(new TStringList()),
    FValues(new TStringList()),
    FFileList(NULL)
  {
    FLog->AddPendingAction(this);
  }

  ~TSessionActionRecord()
  {
    delete FErrorMessages;
    delete FNames;
    delete FValues;
    delete FFileList;
  }

  void Restart()
  {
    FState = Opened;
    FRecursive = false;
    delete FErrorMessages;
    FErrorMessages = NULL;
    delete FFileList;
    FFileList = NULL;
    FNames->Clear();
    FValues->Clear();
  }

  bool Record()
  {
    bool Result = (FState != Opened);
    if (Result)
    {
      if ((FLog->FLoggingActions) && (FState != Cancelled))
      {
        const wchar_t *Name = ActionName();
        std::wstring Attrs;
        if (FRecursive)
        {
          Attrs = L" recursive=\"true\"";
        }
        FLog->Add(llAction, FORMAT(L"  <%s%s>", Name,  Attrs.c_str()));
        for (size_t Index = 0; Index < FNames->GetCount(); Index++)
        {
          std::wstring Value = FValues->GetString(Index);
          if (Value.empty())
          {
            FLog->Add(llAction, FORMAT(L"    <%s />", FNames->GetString(Index).c_str()));
          }
          else
          {
            FLog->Add(llAction, FORMAT(L"    <%s value=\"%s\" />",
              FNames->GetString(Index).c_str(), XmlEscape(Value).c_str()));
          }
        }
        if (FFileList != NULL)
        {
          FLog->Add(llAction, L"    <files>");
          for (size_t Index = 0; Index < FFileList->GetCount(); Index++)
          {
            TRemoteFile * File = FFileList->GetFile(Index);

            FLog->Add(llAction, L"      <file>");
            FLog->Add(llAction, FORMAT(L"        <filename value=\"%s\" />", XmlEscape(File->GetFileName()).c_str()));
            FLog->Add(llAction, FORMAT(L"        <type value=\"%s\" />", XmlEscape(std::wstring(File->GetType(), 1)).c_str()));
            if (!File->GetIsDirectory())
            {
              FLog->Add(llAction, FORMAT(L"        <size value=\"%s\" />", IntToStr(File->GetSize()).c_str()));
            }
            FLog->Add(llAction, FORMAT(L"        <modification value=\"%s\" />", XmlTimestamp(File->GetModification()).c_str()));
            FLog->Add(llAction, FORMAT(L"        <permissions value=\"%s\" />", XmlEscape(File->GetRights()->GetText()).c_str()));
            FLog->Add(llAction, L"      </file>");
          }
          FLog->Add(llAction, L"    </files>");
        }
        if (FState == RolledBack)
        {
          if (FErrorMessages != NULL)
          {
            FLog->Add(llAction, L"    <result success=\"false\">");
            for (size_t Index = 0; Index < FErrorMessages->GetCount(); Index++)
            {
              FLog->Add(llAction,
                FORMAT(L"      <message>%s</message>", XmlEscape(FErrorMessages->GetString(Index).c_str()).c_str()));
            }
            FLog->Add(llAction, L"    </result>");
          }
          else
          {
            FLog->Add(llAction, L"    <result success=\"false\" />");
          }
        }
        else
        {
          FLog->Add(llAction, L"    <result success=\"true\" />");
        }
        FLog->Add(llAction, FORMAT(L"  </%s>", Name));
      }
      delete this;
    }
    return Result;
  }

  void Commit()
  {
    Close(Committed);
  }

  void Rollback(const std::exception * E)
  {
    assert(FErrorMessages == NULL);
    FErrorMessages = new TStringList();
    if (strlen(E->what()) != 0)
    {
      FErrorMessages->Add(::MB2W(E->what()));
    }
    const ExtException * EE = dynamic_cast<const ExtException *>(E);
    if ((EE != NULL) && (EE->GetMoreMessages() != NULL))
    {
      FErrorMessages->AddStrings(EE->GetMoreMessages());
    }
    if (FErrorMessages->GetCount() == 0)
    {
      delete FErrorMessages;
      FErrorMessages = NULL;
    }
    Close(RolledBack);
  }

  void Cancel()
  {
    Close(Cancelled);
  }

  void FileName(const std::wstring & FileName)
  {
    Parameter(L"filename", FileName);
  }

  void Destination(const std::wstring & Destination)
  {
    Parameter(L"destination", Destination);
  }

  void Rights(const TRights & Rights)
  {
    Parameter(L"permissions", Rights.GetText());
  }

  void Modification(const TDateTime & DateTime)
  {
    Parameter(L"modification", XmlTimestamp(DateTime));
  }

  void Recursive()
  {
    FRecursive = true;
  }

  void Command(const std::wstring & Command)
  {
    Parameter(L"command", Command);
  }

  void AddOutput(std::wstring Output, bool StdError)
  {
    const wchar_t * Name = (StdError ? L"erroroutput" : L"output");
    int Index = FNames->IndexOf(Name);
    if (Index >= 0)
    {
      FValues->PutString(Index, FValues->GetString(Index) + L"\r\n" + Output);
    }
    else
    {
      Parameter(Name, Output);
    }
  }

  void FileList(TRemoteFileList * FileList)
  {
    if (FFileList == NULL)
    {
      FFileList = new TRemoteFileList();
    }
    FileList->DuplicateTo(FFileList);
  }

protected:
  enum TState { Opened, Committed, RolledBack, Cancelled };

  inline void Close(TState State)
  {
    assert(FState == Opened);
    FState = State;
    FLog->RecordPendingActions();
  }

  const wchar_t *ActionName()
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
      case laCall: return L"call";
      case laLs: return L"ls";
      default: assert(false); return L"";
    }
  }

  void Parameter(const std::wstring & Name, const std::wstring & Value = L"")
  {
    FNames->Add(Name);
    FValues->Add(Value);
  }

private:
  TSessionLog * FLog;
  TLogAction FAction;
  TState FState;
  bool FRecursive;
  TStrings * FErrorMessages;
  TStrings * FNames;
  TStrings * FValues;
  TRemoteFileList * FFileList;
};
// #pragma warn .inl
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TSessionAction::TSessionAction(TSessionLog * Log, TLogAction Action)
{
  if (Log->FLoggingActions)
  {
    FRecord = new TSessionActionRecord(Log, Action);
  }
  else
  {
    FRecord = NULL;
  }
}
//---------------------------------------------------------------------------
TSessionAction::~TSessionAction()
{
  if (FRecord != NULL)
  {
    Commit();
  }
}
//---------------------------------------------------------------------------
void TSessionAction::Restart()
{
  if (FRecord != NULL)
  {
    FRecord->Restart();
  }
}
//---------------------------------------------------------------------------
void TSessionAction::Commit()
{
  if (FRecord != NULL)
  {
    TSessionActionRecord * Record = FRecord;
    FRecord = NULL;
    Record->Commit();
  }
}
//---------------------------------------------------------------------------
void TSessionAction::Rollback(const std::exception * E)
{
  if (FRecord != NULL)
  {
    TSessionActionRecord * Record = FRecord;
    FRecord = NULL;
    Record->Rollback(E);
  }
}
//---------------------------------------------------------------------------
void TSessionAction::Cancel()
{
  if (FRecord != NULL)
  {
    TSessionActionRecord * Record = FRecord;
    FRecord = NULL;
    Record->Cancel();
  }
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TFileSessionAction::TFileSessionAction(TSessionLog * Log, TLogAction Action) :
  TSessionAction(Log, Action)
{
};
//---------------------------------------------------------------------------
TFileSessionAction::TFileSessionAction(
    TSessionLog * Log, TLogAction Action, const std::wstring & AFileName) :
  TSessionAction(Log, Action)
{
  FileName(AFileName);
};
//---------------------------------------------------------------------------
void TFileSessionAction::FileName(const std::wstring & FileName)
{
  if (FRecord != NULL)
  {
    FRecord->FileName(FileName);
  }
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TFileLocationSessionAction::TFileLocationSessionAction(
    TSessionLog * Log, TLogAction Action) :
  TFileSessionAction(Log, Action)
{
};
//---------------------------------------------------------------------------
TFileLocationSessionAction::TFileLocationSessionAction(
    TSessionLog * Log, TLogAction Action, const std::wstring & FileName) :
  TFileSessionAction(Log, Action, FileName)
{
};
//---------------------------------------------------------------------------
void TFileLocationSessionAction::Destination(const std::wstring & Destination)
{
  if (FRecord != NULL)
  {
    FRecord->Destination(Destination);
  }
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TUploadSessionAction::TUploadSessionAction(TSessionLog * Log) :
  TFileLocationSessionAction(Log, laUpload)
{
};
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TDownloadSessionAction::TDownloadSessionAction(TSessionLog * Log) :
  TFileLocationSessionAction(Log, laDownload)
{
};
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TChmodSessionAction::TChmodSessionAction(
    TSessionLog * Log, const std::wstring & FileName) :
  TFileSessionAction(Log, laChmod, FileName)
{
}
//---------------------------------------------------------------------------
void TChmodSessionAction::Recursive()
{
  if (FRecord != NULL)
  {
    FRecord->Recursive();
  }
}
//---------------------------------------------------------------------------
TChmodSessionAction::TChmodSessionAction(
    TSessionLog * Log, const std::wstring & FileName, const TRights & ARights) :
  TFileSessionAction(Log, laChmod, FileName)
{
  Rights(ARights);
}
//---------------------------------------------------------------------------
void TChmodSessionAction::Rights(const TRights & Rights)
{
  if (FRecord != NULL)
  {
    FRecord->Rights(Rights);
  }
}
//---------------------------------------------------------------------------
TTouchSessionAction::TTouchSessionAction(
    TSessionLog * Log, const std::wstring & FileName, const TDateTime & Modification) :
  TFileSessionAction(Log, laTouch, FileName)
{
  if (FRecord != NULL)
  {
    FRecord->Modification(Modification);
  }
}
//---------------------------------------------------------------------------
TMkdirSessionAction::TMkdirSessionAction(
    TSessionLog * Log, const std::wstring & FileName) :
  TFileSessionAction(Log, laMkdir, FileName)
{
}
//---------------------------------------------------------------------------
TRmSessionAction::TRmSessionAction(
    TSessionLog * Log, const std::wstring & FileName) :
  TFileSessionAction(Log, laRm, FileName)
{
}
//---------------------------------------------------------------------------
void TRmSessionAction::Recursive()
{
  if (FRecord != NULL)
  {
    FRecord->Recursive();
  }
}
//---------------------------------------------------------------------------
TMvSessionAction::TMvSessionAction(TSessionLog * Log,
    const std::wstring & FileName, const std::wstring & ADestination) :
  TFileLocationSessionAction(Log, laMv, FileName)
{
  Destination(ADestination);
}
//---------------------------------------------------------------------------
TCallSessionAction::TCallSessionAction(TSessionLog * Log,
    const std::wstring & Command, const std::wstring & Destination) :
  TSessionAction(Log, laCall)
{
  if (FRecord != NULL)
  {
    FRecord->Command(Command);
    FRecord->Destination(Destination);
  }
}
//---------------------------------------------------------------------------
void TCallSessionAction::AddOutput(const std::wstring & Output, bool StdError)
{
  if (FRecord != NULL)
  {
    FRecord->AddOutput(Output, StdError);
  }
}
//---------------------------------------------------------------------------
TLsSessionAction::TLsSessionAction(TSessionLog * Log,
    const std::wstring & Destination) :
  TSessionAction(Log, laLs)
{
  if (FRecord != NULL)
  {
    FRecord->Destination(Destination);
  }
}
//---------------------------------------------------------------------------
void TLsSessionAction::FileList(TRemoteFileList * FileList)
{
  if (FRecord != NULL)
  {
    FRecord->FileList(FileList);
  }
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TSessionInfo::TSessionInfo()
{
  LoginTime = Now();
}
//---------------------------------------------------------------------------
TFileSystemInfo::TFileSystemInfo()
{
  memset(&IsCapable, false, sizeof(IsCapable));
}
//---------------------------------------------------------------------------
const char *LogLineMarks = "<>!.*";
TSessionLog::TSessionLog(TSessionUI* UI, TSessionData * SessionData,
  TConfiguration * Configuration):
  TStringList()
{
  FCriticalSection = new TCriticalSection;
  FConfiguration = Configuration;
  FParent = NULL;
  FUI = UI;
  FSessionData = SessionData;
  FFile = NULL;
  FLoggedLines = 0;
  FTopIndex = -1;
  FCurrentLogFileName = L"";
  FCurrentFileName = L"";
  FLoggingActions = false;
  FClosed = false;
  FPendingActions = new TList();
  Self = this;
}
//---------------------------------------------------------------------------
TSessionLog::~TSessionLog()
{
  assert(FPendingActions->GetCount() == 0);
  delete FPendingActions;
  FClosed = true;
  ReflectSettings();
  assert(FFile == NULL);
  delete FCriticalSection;
}
//---------------------------------------------------------------------------
void TSessionLog::Lock()
{
  FCriticalSection->Enter();
}
//---------------------------------------------------------------------------
void TSessionLog::Unlock()
{
  FCriticalSection->Leave();
}
//---------------------------------------------------------------------------
std::wstring TSessionLog::GetSessionName()
{
  assert(FSessionData != NULL);
  return FSessionData->GetSessionName();
}
//---------------------------------------------------------------------------
std::wstring TSessionLog::GetLine(int Index)
{
  return GetString(Index - FTopIndex);
}
//---------------------------------------------------------------------------
TLogLineType TSessionLog::GetType(int Index)
{
   void *ptr = GetObject(Index - FTopIndex);
  return (TLogLineType)(int)ptr;
}
//---------------------------------------------------------------------------
void TSessionLog::DoAddToParent(TLogLineType Type, const std::wstring & Line)
{
  assert(FParent != NULL);
  FParent->Add(Type, Line);
}
//---------------------------------------------------------------------------
void TSessionLog::DoAddToSelf(TLogLineType Type, const std::wstring &Line)
{
  // DEBUG_PRINTF(L"begin: Line = %s", Line.c_str());
  if (FTopIndex < 0)
  {
    FTopIndex = 0;
  }

  TStringList::AddObject(Line, (TObject*)Type);

  FLoggedLines++;

  if (LogToFile())
  {
    if (FFile == NULL)
    {
      OpenLogFile();
    }

    if (FFile != NULL)
    {
      if (Type != llAction)
      {
        SYSTEMTIME t;
        ::GetLocalTime(&t);
        // std::wstring Timestamp = FormatDateTime(L" yyyy-mm-dd hh:nn:ss.zzz ", Now());
        std::wstring Timestamp = FORMAT(L" %04d-%02d-%02d %02d:%02d:%02d.%03d ",
            t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);
        fputc(LogLineMarks[Type], (FILE *)FFile);
        // fwrite(Timestamp.c_str(), 1, Timestamp.size() * sizeof(wchar_t), (FILE *)FFile);
        fprintf_s((FILE *)FFile, "%s", (char *)::W2MB(Timestamp.c_str()).c_str());
      }
      // use fwrite instead of fprintf to make sure that even
      // non-ascii data (unicode) gets in.
      fprintf_s((FILE *)FFile, "%s", (char *)::W2MB(Line.c_str()).c_str());
      fputc('\n', (FILE *)FFile);
    }
  }
  // DEBUG_PRINTF(L"end");
}
//---------------------------------------------------------------------------
void TSessionLog::DoAdd(TLogLineType Type, std::wstring Line,
  const doaddlog_slot_type &func)
{
  std::wstring Prefix;

  if ((Type != llAction) && !GetName().empty())
  {
    Prefix = L"[" + GetName() + L"] ";
  }
  doaddlog_signal_type sig;
  sig.connect(func);
  while (!Line.empty())
  {
    sig(Type, Prefix + CutToChar(Line, '\n', false));
  }
}
//---------------------------------------------------------------------------
void TSessionLog::Add(TLogLineType Type, const std::wstring & Line)
{
  assert(FConfiguration);
  if (GetLogging() && (FConfiguration->GetLogActions() == (Type == llAction)))
  {
    try
    {
      if (FParent != NULL)
      {
        DoAdd(Type, Line, boost::bind(&TSessionLog::DoAddToParent, this, _1, _2));
      }
      else
      {
        TGuard Guard(FCriticalSection);

        BeginUpdate();
        {
          BOOST_SCOPE_EXIT ( (&Self) )
          {
            Self->DeleteUnnecessary();
            Self->EndUpdate();
          } BOOST_SCOPE_EXIT_END
          DoAdd(Type, Line, boost::bind(&TSessionLog::DoAddToSelf, this, _1, _2));
        }
      }
    }
    catch (const std::exception &E)
    {
      // We failed logging, turn it off and notify user.
      FConfiguration->SetLogging(false);
      try
      {
        throw ExtException(FMTLOAD(LOG_GEN_ERROR), &E);
      }
      catch (const std::exception &E)
      {
        AddException(&E);
        FUI->HandleExtendedException(&E);
      }
    }
  }
}
//---------------------------------------------------------------------------
void TSessionLog::AddException(const std::exception * E)
{
  if (E != NULL)
  {
    Add(llException, ExceptionLogString(E));
  }
}
//---------------------------------------------------------------------------
void TSessionLog::ReflectSettings()
{
  TGuard Guard(FCriticalSection);

  bool ALogging =
    !FClosed &&
    ((FParent != NULL) || FConfiguration->GetLogging()) &&
    ((FParent == NULL) || !FConfiguration->GetLogActions());

  bool LoggingActions = ALogging && FConfiguration->GetLogActions();
  if (LoggingActions && !FLoggingActions)
  {
    FLoggingActions = true;
    FLogging = ALogging;
    Add(llAction, L"<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
    Add(llAction, FORMAT(L"<session xmlns=\"http://winscp.net/schema/session/1.0\" name=\"%s\" start=\"%s\">",
      XmlEscape(FSessionData->GetSessionName()).c_str(), XmlTimestamp().c_str()));
    StateChange();
  }
  else if (!LoggingActions && FLoggingActions)
  {
    FLoggingActions = false;
    Add(llAction, L"</session>");
    FLogging = ALogging;
    StateChange();
  }
  else if (FLogging != ALogging)
  {
    FLogging = ALogging;
    StateChange();
  }

  // if logging to file was turned off or log file was change -> close current log file
  // but disallow changing logfilename for xml logging
  if ((FFile != NULL) &&
      ((!LogToFile() || (FCurrentLogFileName != FConfiguration->GetLogFileName())) && !FLoggingActions))
  {
    CloseLogFile();
  }

  DeleteUnnecessary();
}
//---------------------------------------------------------------------------
bool TSessionLog::LogToFile()
{
  return GetLogging() && FConfiguration->GetLogToFile() && (FParent == NULL);
}
//---------------------------------------------------------------------------
void TSessionLog::CloseLogFile()
{
  if (FFile != NULL)
  {
    fclose((FILE *)FFile);
    FFile = NULL;
  }
  FCurrentLogFileName = L"";
  FCurrentFileName = L"";
  StateChange();
}
//---------------------------------------------------------------------------
void TSessionLog::OpenLogFile()
{
  try
  {
    assert(FFile == NULL);
    assert(FConfiguration != NULL);
    FCurrentLogFileName = FConfiguration->GetLogFileName();
    std::wstring NewFileName = StripPathQuotes(ExpandEnvironmentVariables(FCurrentLogFileName));
    SYSTEMTIME t;
    ::GetLocalTime(&t);
    for (size_t Index = 1; Index < NewFileName.size(); Index++)
    {
      if ((NewFileName[Index] == '&') && (Index < NewFileName.size() - 1))
      {
        std::wstring Replacement;
        // keep consistent with TFileCustomCommand::PatternReplacement
        switch (tolower(NewFileName[Index + 1]))
        {
          case 'y':
            Replacement = FORMAT(L"%04d", t.wYear);
            break;

          case 'm':
            Replacement = FORMAT(L"%02d", t.wMonth);
            break;

          case 'd':
            Replacement = FORMAT(L"%02d", t.wDay);
            break;

          case 't':
            Replacement = FORMAT(L"%02d%02d%02d", t.wHour, t.wMinute, t.wSecond);
            break;

          case '@':
            Replacement = MakeValidFileName(FSessionData->GetHostName());
            break;

          case 's':
            Replacement = MakeValidFileName(FSessionData->GetSessionName());
            break;

          case '&':
            Replacement = L"&";
            break;

          default:
            Replacement = std::wstring(L"&") + NewFileName[Index + 1];
            break;
        }
        NewFileName.erase(Index, 2);
        NewFileName.insert(Index, Replacement);
        Index += Replacement.size() - 1;
      }
    }
    // FFile = _wfopen(NewFileName.c_str(),
      // FConfiguration->GetLogFileAppend() && !FLoggingActions ? L"a" : L"w");
    FFile = _fsopen(::W2MB(NewFileName.c_str()).c_str(),
      FConfiguration->GetLogFileAppend() && !FLoggingActions ? "a" : "w", SH_DENYWR);
    if (FFile)
    {
      setvbuf((FILE *)FFile, NULL, _IONBF, BUFSIZ);
      FCurrentFileName = NewFileName;
    }
    else
    {
      throw ExtException(FMTLOAD(LOG_OPENERROR, NewFileName.c_str()));
    }
  }
  catch (const std::exception & E)
  {
    // We failed logging to file, turn it off and notify user.
    FCurrentLogFileName = L"";
    FCurrentFileName = L"";
    FConfiguration->SetLogToFile(false);
    try
    {
      throw ExtException(FMTLOAD(LOG_GEN_ERROR), &E);
    }
    catch (const std::exception & E)
    {
      AddException(&E);
      FUI->HandleExtendedException(&E);
    }
  }
  StateChange();
}
//---------------------------------------------------------------------------
void TSessionLog::StateChange()
{
  if (!FOnStateChange.empty())
  {
    FOnStateChange(this);
  }
}
//---------------------------------------------------------------------------
void TSessionLog::DeleteUnnecessary()
{
  BeginUpdate();
  {
    BOOST_SCOPE_EXIT ( (&Self) )
    {
      Self->EndUpdate();
    } BOOST_SCOPE_EXIT_END
    if (!GetLogging() || (FParent != NULL))
    {
      Clear();
    }
    else
    {
      while (!FConfiguration->GetLogWindowComplete() && ((int)GetCount() > FConfiguration->GetLogWindowLines()))
      {
        Delete(0);
        FTopIndex++;
      }
    }
  }
}
//---------------------------------------------------------------------------
void TSessionLog::AddStartupInfo()
{
  if (GetLogging() && !FConfiguration->GetLogActions())
  {
    if (FParent != NULL)
    {
      // do not add session info for secondary session
      // (this should better be handled in the TSecondaryTerminal)
    }
    else
    {
      DoAddStartupInfo(FSessionData);
    }
  }
}
//---------------------------------------------------------------------------
void TSessionLog::DoAddStartupInfo(TSessionData * Data)
{
  TGuard Guard(FCriticalSection);

  BeginUpdate();
  {
    BOOST_SCOPE_EXIT ( (&Self) )
    {
      Self->DeleteUnnecessary();

      Self->EndUpdate();
    } BOOST_SCOPE_EXIT_END
    #define ADF(S, ...) DoAdd(llMessage, FORMAT(S, __VA_ARGS__), boost::bind(&TSessionLog::DoAddToSelf, this, _1, _2));
    AddSeparator();
    ADF(L"NetBox %s (OS %s)", FConfiguration->GetVersionStr().c_str(), FConfiguration->GetOSVersionStr().c_str());
    THierarchicalStorage * Storage = FConfiguration->CreateScpStorage(false);
    {
      BOOST_SCOPE_EXIT ( (Storage) )
      {
        delete Storage;
      } BOOST_SCOPE_EXIT_END
      ADF(L"Configuration: %s", Storage->GetSource().c_str());
    }
    ADF(L"Login time: %s", FormatDateTime(L"dddddd tt", Now()).c_str());
    AddSeparator();
    ADF(L"Session name: %s (%s)", Data->GetSessionName().c_str(), Data->GetSource().c_str());
    ADF(L"Host name: %s (Port: %d)", Data->GetHostName().c_str(), Data->GetPortNumber());
    ADF(L"User name: %s (Password: %s, Key file: %s)",
      Data->GetUserName().c_str(), BooleanToEngStr(!Data->GetPassword().empty()).c_str(),
       BooleanToEngStr(!Data->GetPublicKeyFile().empty()).c_str())
    ADF(L"Tunnel: %s", BooleanToEngStr(Data->GetTunnel()).c_str());
    if (Data->GetTunnel())
    {
      ADF(L"Tunnel: Host name: %s (Port: %d)", Data->GetTunnelHostName().c_str(), Data->GetTunnelPortNumber());
      ADF(L"Tunnel: User name: %s (Password: %s, Key file: %s)",
        Data->GetTunnelUserName().c_str(), BooleanToEngStr(!Data->GetTunnelPassword().empty()).c_str(),
         BooleanToEngStr(!Data->GetTunnelPublicKeyFile().empty()).c_str());
      ADF(L"Tunnel: Local port number: %d", Data->GetTunnelLocalPortNumber());
    }
    ADF(L"Transfer Protocol: %s", Data->GetFSProtocolStr().c_str());
    wchar_t * PingTypes = L"-NC";
    TPingType PingType;
    int PingInterval;
    if ((Data->GetFSProtocol() == fsFTP) || (Data->GetFSProtocol() == fsFTPS))
    {
      PingType = Data->GetFtpPingType();
      PingInterval = Data->GetFtpPingInterval();
    }
    else
    {
      PingType = Data->GetPingType();
      PingInterval = Data->GetPingInterval();
    }
    ADF(L"Ping type: %c, Ping interval: %d sec; Timeout: %d sec",
      PingTypes[PingType], PingInterval, Data->GetTimeout());
    ADF(L"Proxy: %s", ProxyMethodList[Data->GetProxyMethod()]);
    if (Data->GetProxyMethod() != pmNone)
    {
      ADF(L"HostName: %s (Port: %d); Username: %s; Passwd: %s",
        Data->GetProxyHost().c_str(), Data->GetProxyPort(),
        Data->GetProxyUsername().c_str(), BooleanToEngStr(!Data->GetProxyPassword().empty()).c_str());
      if (Data->GetProxyMethod() == pmTelnet)
      {
        ADF(L"Telnet command: %s", Data->GetProxyTelnetCommand().c_str());
      }
      if (Data->GetProxyMethod() == pmCmd)
      {
        ADF(L"Local command: %s", Data->GetProxyLocalCommand().c_str());
      }
    }
    if (Data->GetUsesSsh())
    {
      ADF(L"SSH protocol version: %s; Compression: %s",
        Data->GetSshProtStr().c_str(), BooleanToEngStr(Data->GetCompression()).c_str());
      ADF(L"Bypass authentication: %s",
       BooleanToEngStr(Data->GetSshNoUserAuth()).c_str());
      ADF(L"Try agent: %s; Agent forwarding: %s; TIS/CryptoCard: %s; KI: %s; GSSAPI: %s",
        BooleanToEngStr(Data->GetTryAgent()).c_str(),
        BooleanToEngStr(Data->GetAgentFwd()).c_str(),
        BooleanToEngStr(Data->GetAuthTIS()).c_str(),
        BooleanToEngStr(Data->GetAuthKI()).c_str(),
        BooleanToEngStr(Data->GetAuthGSSAPI()).c_str());
      if (Data->GetAuthGSSAPI())
      {
        ADF(L"GSSAPI: Forwarding: %s; Server realm: %s",
          BooleanToEngStr(Data->GetGSSAPIFwdTGT()).c_str(), Data->GetGSSAPIServerRealm().c_str());
      }
      ADF(L"Ciphers: %s; Ssh2DES: %s",
        Data->GetCipherList().c_str(), BooleanToEngStr(Data->GetSsh2DES()).c_str());
      std::wstring Bugs;
      wchar_t const * BugFlags = L"A+-";
      for (int Index = 0; Index < BUG_COUNT; Index++)
      {
        Bugs += BugFlags[Data->GetBug((TSshBug)Index)];
        Bugs += Index<BUG_COUNT-1? L"," : L"";
      }
      ADF(L"SSH Bugs: %s", Bugs.c_str());
      Bugs = L"";
      for (int Index = 0; Index < SFTP_BUG_COUNT; Index++)
      {
        Bugs += BugFlags[Data->GetSFTPBug((TSftpBug)Index)];
        Bugs += Index<SFTP_BUG_COUNT-1 ? L"," : L"";
      }
      ADF(L"SFTP Bugs: %s", Bugs.c_str());
      ADF(L"Return code variable: %s; Lookup user groups: %s",
        Data->GetDetectReturnVar() ? std::wstring(L"Autodetect").c_str() : Data->GetReturnVar().c_str(),
        BooleanToEngStr(Data->GetLookupUserGroups()).c_str());
      ADF(L"Shell: %s", Data->GetShell().empty() ? std::wstring(L"default").c_str() : Data->GetShell().c_str());
      ADF(L"EOL: %d, UTF: %d", Data->GetEOLType(), Data->GetNotUtf());
      ADF(L"Clear aliases: %s, Unset nat.vars: %s, Resolve symlinks: %s",
        BooleanToEngStr(Data->GetClearAliases()).c_str(),
        BooleanToEngStr(Data->GetUnsetNationalVars()).c_str(),
        BooleanToEngStr(Data->GetResolveSymlinks()).c_str());
      ADF(L"LS: %s, Ign LS warn: %s, Scp1 Comp: %s",
        Data->GetListingCommand().c_str(),
        BooleanToEngStr(Data->GetIgnoreLsWarnings()).c_str(),
        BooleanToEngStr(Data->GetScp1Compatibility()).c_str());
    }
    if ((Data->GetFSProtocol() == fsFTP) || (Data->GetFSProtocol() == fsFTPS))
    {
      std::wstring Ftps;
      switch (Data->GetFtps())
      {
        case ftpsImplicit:
          Ftps = L"Implicit SSL/TLS";
          break;

        case ftpsExplicitSsl:
          Ftps = L"Explicit SSL";
          break;

        case ftpsExplicitTls:
          Ftps = L"Explicit TLS";
          break;

        default:
          assert(Data->GetFtps() == ftpsNone);
          Ftps = L"None";
          break;
      }
      ADF(L"FTP: FTPS: %s; Passive: %s [Force IP: %s]",
        Ftps.c_str(), BooleanToEngStr(Data->GetFtpPasvMode()).c_str(),
         BooleanToEngStr(Data->GetFtpForcePasvIp()).c_str());
    }
    ADF(L"Local directory: %s, Remote directory: %s, Update: %s, Cache: %s",
      (Data->GetLocalDirectory().empty() ? std::wstring(L"default").c_str() : Data->GetLocalDirectory().c_str()),
       (Data->GetRemoteDirectory().empty() ? std::wstring(L"home").c_str() : Data->GetRemoteDirectory().c_str()),
       BooleanToEngStr(Data->GetUpdateDirectories()).c_str(),
       BooleanToEngStr(Data->GetCacheDirectories()).c_str());
    ADF(L"Cache directory changes: %s, Permanent: %s",
      BooleanToEngStr(Data->GetCacheDirectoryChanges()).c_str(),
       BooleanToEngStr(Data->GetPreserveDirectoryChanges()).c_str());
    ADF(L"DST mode: %d", int(Data->GetDSTMode()));

    AddSeparator();

    #undef ADF
  }
}
//---------------------------------------------------------------------------
void TSessionLog::AddSeparator()
{
  Add(llMessage, L"--------------------------------------------------------------------------");
}
//---------------------------------------------------------------------------
int TSessionLog::GetBottomIndex()
{
  return (GetCount() > 0 ? (GetTopIndex() + GetCount() - 1) : -1);
}
//---------------------------------------------------------------------------
bool TSessionLog::GetLoggingToFile()
{
  assert((FFile == NULL) || LogToFile());
  return (FFile != NULL);
}
//---------------------------------------------------------------------------
void TSessionLog::Clear()
{
  TGuard Guard(FCriticalSection);

  FTopIndex += GetCount();
  TStringList::Clear();
}
//---------------------------------------------------------------------------
void TSessionLog::AddPendingAction(TSessionActionRecord * Action)
{
  FPendingActions->Add((TObject *)Action);
}
//---------------------------------------------------------------------------
void TSessionLog::RecordPendingActions()
{
  while ((FPendingActions->GetCount() > 0) &&
         reinterpret_cast<TSessionActionRecord *>(FPendingActions->GetItem(0))->Record())
  {
    FPendingActions->Delete(0);
  }
}
