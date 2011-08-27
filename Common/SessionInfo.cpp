//---------------------------------------------------------------------------
#include <stdio.h>

#include "Common.h"
#include "SessionInfo.h"
#include "Exceptions.h"
#include "TextsCore.h"
//---------------------------------------------------------------------------
std::wstring XmlEscape(std::wstring Str)
{
  for (int i = 1; i <= Str.size(); i++)
  {
    const char * Repl = NULL;
    switch (Str[i])
    {
      case '&':
        Repl = "amp;";
        break;

      case '>':
        Repl = "gt;";
        break;

      case '<':
        Repl = "lt;";
        break;

      case '"':
        Repl = "quot;";
        break;

      case '\r':
        Str.erase(i, 1);
        i--;
        break;
    }

    if (Repl != NULL)
    {
      Str[i] = '&';
      Str.Insert(Repl, i + 1);
      i += strlen(Repl);
    }
  }
  Str = AnsiToUtf8(Str);
  return Str;
}
//---------------------------------------------------------------------------
std::wstring XmlTimestamp(const TDateTime & DateTime)
{
  return FormatDateTime("yyyy'-'mm'-'dd'T'hh':'nn':'ss'.'zzz'Z'", ConvertTimestampToUTC(DateTime));
}
//---------------------------------------------------------------------------
std::wstring XmlTimestamp()
{
  return XmlTimestamp(Now());
}
//---------------------------------------------------------------------------
#pragma warn -inl
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
        const char * Name = ActionName();
        std::wstring Attrs;
        if (FRecursive)
        {
          Attrs = " recursive=\"true\"";
        }
        FLog->Add(llAction, FORMAT("  <%s%s>", (Name,  Attrs)));
        for (int Index = 0; Index < FNames->GetCount(); Index++)
        {
          std::wstring Value = FValues->GetString(Index);
          if (Value.empty())
          {
            FLog->Add(llAction, FORMAT("    <%s />", (FNames->GetString(Index))));
          }
          else
          {
            FLog->Add(llAction, FORMAT("    <%s value=\"%s\" />",
              (FNames->GetString(Index), XmlEscape(Value))));
          }
        }
        if (FFileList != NULL)
        {
          FLog->Add(llAction, "    <files>");
          for (int Index = 0; Index < FFileList->GetCount(); Index++)
          {
            TRemoteFile * File = FFileList->Files[Index];

            FLog->Add(llAction, "      <file>");
            FLog->Add(llAction, FORMAT("        <filename value=\"%s\" />", (XmlEscape(File->GetFileName()))));
            FLog->Add(llAction, FORMAT("        <type value=\"%s\" />", (XmlEscape(File->Type))));
            if (!File->GetIsDirectory())
            {
              FLog->Add(llAction, FORMAT("        <size value=\"%s\" />", (IntToStr(File->Size))));
            }
            FLog->Add(llAction, FORMAT("        <modification value=\"%s\" />", (XmlTimestamp(File->Modification))));
            FLog->Add(llAction, FORMAT("        <permissions value=\"%s\" />", (XmlEscape(File->GetRights()->Text))));
            FLog->Add(llAction, "      </file>");
          }
          FLog->Add(llAction, "    </files>");
        }
        if (FState == RolledBack)
        {
          if (FErrorMessages != NULL)
          {
            FLog->Add(llAction, "    <result success=\"false\">");
            for (int Index = 0; Index < FErrorMessages->GetCount(); Index++)
            {
              FLog->Add(llAction,
                FORMAT("      <message>%s</message>", (XmlEscape(FErrorMessages->GetString(Index)))));
            }
            FLog->Add(llAction, "    </result>");
          }
          else
          {
            FLog->Add(llAction, "    <result success=\"false\" />");
          }
        }
        else
        {
          FLog->Add(llAction, "    <result success=\"true\" />");
        }
        FLog->Add(llAction, FORMAT("  </%s>", (Name)));
      }
      delete this;
    }
    return Result;
  }

  void Commit()
  {
    Close(Committed);
  }

  void Rollback(exception * E)
  {
    assert(FErrorMessages == NULL);
    FErrorMessages = new TStringList();
    if (!E->Message.empty())
    {
      FErrorMessages->Add(E->Message);
    }
    ExtException * EE = dynamic_cast<ExtException *>(E);
    if ((EE != NULL) && (EE->MoreMessages != NULL))
    {
      FErrorMessages->AddStrings(EE->MoreMessages);
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
    Parameter("filename", FileName);
  }

  void Destination(const std::wstring & Destination)
  {
    Parameter("destination", Destination);
  }

  void Rights(const TRights & Rights)
  {
    Parameter("permissions", Rights.Text);
  }

  void Modification(const TDateTime & DateTime)
  {
    Parameter("modification", XmlTimestamp(DateTime));
  }

  void Recursive()
  {
    FRecursive = true;
  }

  void Command(const std::wstring & Command)
  {
    Parameter("command", Command);
  }

  void AddOutput(std::wstring Output, bool StdError)
  {
    const char * Name = (StdError ? "erroroutput" : "output");
    int Index = FNames->IndexOf(Name);
    if (Index >= 0)
    {
      FValues->GetString(Index) = FValues->GetString(Index) + "\r\n" + Output;
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

  const char * ActionName()
  {
    switch (FAction)
    {
      case laUpload: return "upload";
      case laDownload: return "download";
      case laTouch: return "touch";
      case laChmod: return "chmod";
      case laMkdir: return "mkdir";
      case laRm: return "rm";
      case laMv: return "mv";
      case laCall: return "call";
      case laLs: return "ls";
      default: assert(false); return "";
    }
  }

  void Parameter(const std::wstring & Name, const std::wstring & Value = "")
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
#pragma warn .inl
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
void TSessionAction::Rollback(exception * E)
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
    FRecord->GetFileName()(FileName);
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
    FRecord->GetRights()(Rights);
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
  FCurrentLogFileName = "";
  FCurrentFileName = "";
  FLoggingActions = false;
  FClosed = false;
  FPendingActions = new TList();
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
  return FSessionData->SessionName;
}
//---------------------------------------------------------------------------
std::wstring TSessionLog::GetLine(int Index)
{
  return Strings[Index - FTopIndex];
}
//---------------------------------------------------------------------------
TLogLineType TSessionLog::GetType(int Index)
{
  return (TLogLineType)Objects[Index - FTopIndex];
}
//---------------------------------------------------------------------------
void TSessionLog::DoAddToParent(TLogLineType Type, const std::wstring & Line)
{
  assert(FParent != NULL);
  FParent->Add(Type, Line);
}
//---------------------------------------------------------------------------
void TSessionLog::DoAddToSelf(TLogLineType Type, const std::wstring & Line)
{
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
        std::wstring Timestamp = FormatDateTime(" yyyy-mm-dd hh:nn:ss.zzz ", Now());
        fputc(LogLineMarks[Type], (FILE *)FFile);
        fwrite(Timestamp.c_str(), Timestamp.size(), 1, (FILE *)FFile);
      }
      // use fwrite instead of fprintf to make sure that even
      // non-ascii data (unicode) gets in.
      fwrite(Line.c_str(), Line.size(), 1, (FILE *)FFile);
      fputc('\n', (FILE *)FFile);
    }
  }
}
//---------------------------------------------------------------------------
void TSessionLog::DoAdd(TLogLineType Type, std::wstring Line,
  void (  \1 Get\2() { return F\2; }\r\n  void Set\2(\1 value) { F\2 = value; }f)(TLogLineType Type, const std::wstring & Line))
{
  std::wstring Prefix;

  if ((Type != llAction) && !Name.empty())
  {
    Prefix = "[" + Name + "] ";
  }

  while (!Line.empty())
  {
    f(Type, Prefix + CutToChar(Line, '\n', false));
  }
}
//---------------------------------------------------------------------------
void TSessionLog::Add(TLogLineType Type, const std::wstring & Line)
{
  assert(FConfiguration);
  if (Logging && (FConfiguration->LogActions == (Type == llAction)))
  {
    try
    {
      if (FParent != NULL)
      {
        DoAdd(Type, Line, &DoAddToParent);
      }
      else
      {
        TGuard Guard(FCriticalSection);

        BeginUpdate();

        try
        {
          DoAdd(Type, Line, DoAddToSelf);
        }
        catch(...)
        {
          DeleteUnnecessary();

          EndUpdate();
        }
      }
    }
    catch (exception &E)
    {
      // We failed logging, turn it off and notify user.
      FConfiguration->Logging = false;
      try
      {
        throw ExtException(&E, LOG_GEN_ERROR);
      }
      catch (exception &E)
      {
        AddException(&E);
        FUI->HandleExtendedException(&E);
      }
    }
  }
}
//---------------------------------------------------------------------------
void TSessionLog::AddException(exception * E)
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
    ((FParent != NULL) || FConfiguration->Logging) &&
    ((FParent == NULL) || !FConfiguration->LogActions);

  bool LoggingActions = ALogging && FConfiguration->LogActions;
  if (LoggingActions && !FLoggingActions)
  {
    FLoggingActions = true;
    FLogging = ALogging;
    Add(llAction, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
    Add(llAction, FORMAT("<session xmlns=\"http://winscp.net/schema/session/1.0\" name=\"%s\" start=\"%s\">",
      (XmlEscape(FSessionData->SessionName), XmlTimestamp())));
    StateChange();
  }
  else if (!LoggingActions && FLoggingActions)
  {
    FLoggingActions = false;
    Add(llAction, "</session>");
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
      ((!LogToFile() || (FCurrentLogFileName != FConfiguration->LogFileName)) && !FLoggingActions))
  {
    CloseLogFile();
  }

  DeleteUnnecessary();
}
//---------------------------------------------------------------------------
bool TSessionLog::LogToFile()
{
  return Logging && FConfiguration->LogToFile && (FParent == NULL);
}
//---------------------------------------------------------------------------
void TSessionLog::CloseLogFile()
{
  if (FFile != NULL)
  {
    fclose((FILE *)FFile);
    FFile = NULL;
  }
  FCurrentLogFileName = "";
  FCurrentFileName = "";
  StateChange();
}
//---------------------------------------------------------------------------
void TSessionLog::OpenLogFile()
{
  try
  {
    assert(FFile == NULL);
    assert(FConfiguration != NULL);
    FCurrentLogFileName = FConfiguration->LogFileName;
    std::wstring NewFileName = StripPathQuotes(ExpandEnvironmentVariables(FCurrentLogFileName));
    TDateTime N = Now();
    for (int Index = 1; Index < NewFileName.size(); Index++)
    {
      if (NewFileName[Index] == '!')
      {
        std::wstring Replacement;
        // keep consistent with TFileCustomCommand::PatternReplacement
        switch (tolower(NewFileName[Index + 1]))
        {
          case 'y':
            Replacement = FormatDateTime("yyyy", N);
            break;

          case 'm':
            Replacement = FormatDateTime("mm", N);
            break;

          case 'd':
            Replacement = FormatDateTime("dd", N);
            break;

          case 't':
            Replacement = FormatDateTime("hhnnss", N);
            break;

          case '@':
            Replacement = MakeValidFileName(FSessionData->HostName);
            break;

          case 's':
            Replacement = MakeValidFileName(FSessionData->SessionName);
            break;

          case '!':
            Replacement = "!";
            break;

          default:
            Replacement = std::wstring("!") + NewFileName[Index + 1];
            break;
        }
        NewFileName.erase(Index, 2);
        NewFileName.Insert(Replacement, Index);
        Index += Replacement.size() - 1;
      }
    }
    FFile = fopen(NewFileName.c_str(),
      (FConfiguration->LogFileAppend && !FLoggingActions ? "a" : "w"));
    if (FFile)
    {
      setvbuf((FILE *)FFile, NULL, _IONBF, BUFSIZ);
      FCurrentFileName = NewFileName;
    }
    else
    {
      throw exception(FMTLOAD(LOG_OPENERROR, (NewFileName)));
    }
  }
  catch (exception & E)
  {
    // We failed logging to file, turn it off and notify user.
    FCurrentLogFileName = "";
    FCurrentFileName = "";
    FConfiguration->LogToFile = false;
    try
    {
      throw ExtException(&E, LOG_GEN_ERROR);
    }
    catch (exception & E)
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
  if (FOnStateChange != NULL)
  {
    FOnStateChange(this);
  }
}
//---------------------------------------------------------------------------
void TSessionLog::DeleteUnnecessary()
{
  BeginUpdate();
  try
  {
    if (!Logging || (FParent != NULL))
    {
      Clear();
    }
    else
    {
      while (!FConfiguration->LogWindowComplete && (Count > FConfiguration->LogWindowLines))
      {
        Delete(0);
        FTopIndex++;
      }
    }
  }
  catch(...)
  {
    EndUpdate();
  }
}
//---------------------------------------------------------------------------
void TSessionLog::AddStartupInfo()
{
  if (Logging && !FConfiguration->LogActions)
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
  try
  {
    #define ADF(S, F) DoAdd(llMessage, FORMAT(S, F), DoAddToSelf);
    AddSeparator();
    ADF("WinSCP %s (OS %s)", (FConfiguration->VersionStr, FConfiguration->OSVersionStr));
    THierarchicalStorage * Storage = FConfiguration->CreateScpStorage(false);
    try
    {
      ADF("Configuration: %s", (Storage->Source));
    }
    catch(...)
    {
      delete Storage;
    }
    ADF("Login time: %s", (FormatDateTime("dddddd tt", Now())));
    AddSeparator();
    ADF("Session name: %s (%s)", (Data->SessionName, Data->Source));
    ADF("Host name: %s (Port: %d)", (Data->HostName, Data->PortNumber));
    ADF("User name: %s (Password: %s, Key file: %s)",
      (Data->UserName, BooleanToEngStr(!Data->Password.empty()),
       BooleanToEngStr(!Data->PublicKeyFile.empty())))
    ADF("Tunnel: %s", (BooleanToEngStr(Data->Tunnel)));
    if (Data->Tunnel)
    {
      ADF("Tunnel: Host name: %s (Port: %d)", (Data->TunnelHostName, Data->TunnelPortNumber));
      ADF("Tunnel: User name: %s (Password: %s, Key file: %s)",
        (Data->TunnelUserName, BooleanToEngStr(!Data->TunnelPassword.empty()),
         BooleanToEngStr(!Data->TunnelPublicKeyFile.empty())))
      ADF("Tunnel: Local port number: %d", (Data->TunnelLocalPortNumber));
    }
    ADF("Transfer Protocol: %s", (Data->FSProtocolStr));
    char * PingTypes = "-NC";
    TPingType PingType;
    int PingInterval;
    if (Data->FSProtocol == fsFTP)
    {
      PingType = Data->FtpPingType;
      PingInterval = Data->FtpPingInterval;
    }
    else
    {
      PingType = Data->PingType;
      PingInterval = Data->PingInterval;
    }
    ADF("Ping type: %s, Ping interval: %d sec; Timeout: %d sec",
      (std::wstring(PingTypes[PingType]), PingInterval, Data->Timeout));
    ADF("Proxy: %s", (ProxyMethodList[Data->ProxyMethod]));
    if (Data->ProxyMethod != pmNone)
    {
      ADF("HostName: %s (Port: %d); Username: %s; Passwd: %s",
        (Data->ProxyHost, Data->ProxyPort,
         Data->ProxyUsername, BooleanToEngStr(!Data->ProxyPassword.empty())));
      if (Data->ProxyMethod == pmTelnet)
      {
        ADF("Telnet command: %s", (Data->ProxyTelnetCommand));
      }
      if (Data->ProxyMethod == pmCmd)
      {
        ADF("Local command: %s", (Data->ProxyLocalCommand));
      }
    }
    if (Data->UsesSsh)
    {
      ADF("SSH protocol version: %s; Compression: %s",
        (Data->SshProtStr, BooleanToEngStr(Data->Compression)));
      ADF("Bypass authentication: %s",
       (BooleanToEngStr(Data->SshNoUserAuth)));
      ADF("Try agent: %s; Agent forwarding: %s; TIS/CryptoCard: %s; KI: %s; GSSAPI: %s",
        (BooleanToEngStr(Data->TryAgent), BooleanToEngStr(Data->AgentFwd), BooleanToEngStr(Data->AuthTIS),
         BooleanToEngStr(Data->AuthKI), BooleanToEngStr(Data->AuthGSSAPI)));
      if (Data->AuthGSSAPI)
      {
        ADF("GSSAPI: Forwarding: %s; Server realm: %s",
          (BooleanToEngStr(Data->GSSAPIFwdTGT), Data->GSSAPIServerRealm));
      }
      ADF("Ciphers: %s; Ssh2DES: %s",
        (Data->CipherList, BooleanToEngStr(Data->Ssh2DES)));
      std::wstring Bugs;
      char const * BugFlags = "A+-";
      for (int Index = 0; Index < BUG_COUNT; Index++)
      {
        Bugs += std::wstring(BugFlags[Data->Bug[(TSshBug)Index]])+(Index<BUG_COUNT-1?",":"");
      }
      ADF("SSH Bugs: %s", (Bugs));
      Bugs = "";
      for (int Index = 0; Index < SFTP_BUG_COUNT; Index++)
      {
        Bugs += std::wstring(BugFlags[Data->SFTPBug[(TSftpBug)Index]])+(Index<SFTP_BUG_COUNT-1?",":"");
      }
      ADF("SFTP Bugs: %s", (Bugs));
      ADF("Return code variable: %s; Lookup user groups: %s",
        ((Data->DetectReturnVar ? std::wstring("Autodetect") : Data->ReturnVar),
         BooleanToEngStr(Data->LookupUserGroups)));
      ADF("Shell: %s", ((Data->Shell.empty()? std::wstring("default") : Data->Shell)));
      ADF("EOL: %d, UTF: %d", (Data->EOLType, Data->NotUtf));
      ADF("Clear aliases: %s, Unset nat.vars: %s, Resolve symlinks: %s",
        (BooleanToEngStr(Data->ClearAliases), BooleanToEngStr(Data->UnsetNationalVars),
         BooleanToEngStr(Data->ResolveSymlinks)));
      ADF("LS: %s, Ign LS warn: %s, Scp1 Comp: %s",
        (Data->ListingCommand,
         BooleanToEngStr(Data->IgnoreLsWarnings),
         BooleanToEngStr(Data->Scp1Compatibility)));
    }
    if (Data->FSProtocol == fsFTP)
    {
      std::wstring Ftps;
      switch (Data->Ftps)
      {
        case ftpsImplicit:
          Ftps = "Implicit SSL/TLS";
          break;

        case ftpsExplicitSsl:
          Ftps = "Explicit SSL";
          break;

        case ftpsExplicitTls:
          Ftps = "Explicit TLS";
          break;

        default:
          assert(Data->Ftps == ftpsNone);
          Ftps = "None";
          break;
      }
      ADF("FTP: FTPS: %s; Passive: %s [Force IP: %s]",
        (Ftps, BooleanToEngStr(Data->FtpPasvMode),
         BooleanToEngStr(Data->FtpForcePasvIp)));
    }
    ADF("Local directory: %s, Remote directory: %s, Update: %s, Cache: %s",
      ((Data->LocalDirectory.empty() ? std::wstring("default") : Data->LocalDirectory),
       (Data->RemoteDirectory.empty() ? std::wstring("home") : Data->RemoteDirectory),
       BooleanToEngStr(Data->UpdateDirectories),
       BooleanToEngStr(Data->CacheDirectories)));
    ADF("Cache directory changes: %s, Permanent: %s",
      (BooleanToEngStr(Data->CacheDirectoryChanges),
       BooleanToEngStr(Data->PreserveDirectoryChanges)));
    ADF("DST mode: %d", (int(Data->DSTMode)));

    AddSeparator();

    #undef ADF
  }
  catch(...)
  {
    DeleteUnnecessary();

    EndUpdate();
  }
}
//---------------------------------------------------------------------------
void TSessionLog::AddSeparator()
{
  Add(llMessage, "--------------------------------------------------------------------------");
}
//---------------------------------------------------------------------------
int TSessionLog::GetBottomIndex()
{
  return (Count > 0 ? (TopIndex + Count - 1) : -1);
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

  FTopIndex += Count;
  TStringList::Clear();
}
//---------------------------------------------------------------------------
void TSessionLog::AddPendingAction(TSessionActionRecord * Action)
{
  FPendingActions->Add(Action);
}
//---------------------------------------------------------------------------
void TSessionLog::RecordPendingActions()
{
  while ((FPendingActions->GetCount() > 0) &&
         static_cast<TSessionActionRecord *>(FPendingActions->GetItem(0])->Record())
  {
    FPendingActions->Delete(0);
  }
}
