//---------------------------------------------------------------------------
#include "stdafx.h"

#include <stdio.h>
#include <winhttp.h>

#include "boostdefines.hpp"
#include <boost/scope_exit.hpp>
#include <boost/bind.hpp>

#include "HttpFileSystem.h"

#include "Terminal.h"
#include "Common.h"
#include "Exceptions.h"
#include "Interface.h"
#include "TextsCore.h"
#include "SecureShell.h"
#include "FileZillaIntf.h"
#include "Settings.h"
#include "FarUtil.h"
#include "tinyXML\tinyxml.h"

//---------------------------------------------------------------------------
// #define WEBDAV_ASYNC 1
//---------------------------------------------------------------------------
#define FILE_OPERATION_LOOP_EX(ALLOW_SKIP, MESSAGE, OPERATION) \
  FILE_OPERATION_LOOP_CUSTOM(Self->FTerminal, ALLOW_SKIP, MESSAGE, OPERATION)
//---------------------------------------------------------------------------
const int coRaiseExcept = 1;
const int coExpectNoOutput = 2;
const int coWaitForLastLine = 4;
const int coOnlyReturnCode = 8;
const int coIgnoreWarnings = 16;
const int coReadProgress = 32;

const int ecRaiseExcept = 1;
const int ecIgnoreWarnings = 2;
const int ecReadProgress = 4;
const int ecDefault = ecRaiseExcept;
//---------------------------------------------------------------------------
const int DummyCodeClass = 8;
const int DummyTimeoutCode = 801;
const int DummyCancelCode = 802;
const int DummyDisconnectCode = 803;
//---------------------------------------------------------------------------
#define THROW_FILE_SKIPPED(MESSAGE, EXCEPTION) \
  throw EScpFileSkipped(MESSAGE, EXCEPTION)

#define THROW_SCP_ERROR(MESSAGE, EXCEPTION) \
  throw EScp(MESSAGE, EXCEPTION)
//===========================================================================
#define MaxShellCommand fsAnyCommand
#define ShellCommandCount MaxShellCommand + 1
#define MaxCommandLen 40
struct TCommandType
{
  int MinLines;
  int MaxLines;
  bool ModifiesFiles;
  bool ChangesDirectory;
  bool InteractiveCommand;
  wchar_t Command[MaxCommandLen];
};

// Only one character! See THTTPFileSystem::ReadCommandOutput()
#define LastLineSeparator L":"
#define LAST_LINE L"WinSCP: this is end-of-file"
#define FIRST_LINE L"WinSCP: this is begin-of-file"

#define NationalVarCount 10
extern const wchar_t NationalVars[NationalVarCount][15];

#define CHECK_CMD assert((Cmd >=0) && (Cmd <= MaxShellCommand))

class TSessionData;
//---------------------------------------------------------------------------
class THTTPCommandSet
{
private:
  TCommandType CommandSet[ShellCommandCount];
  TSessionData * FSessionData;
  std::wstring FReturnVar;
public:
  THTTPCommandSet(TSessionData *aSessionData);
  void Default();
  void CopyFrom(THTTPCommandSet * Source);
  std::wstring Command(TFSCommand Cmd, ...);
  std::wstring Command(TFSCommand Cmd, va_list args);
  TStrings * CreateCommandList();
  std::wstring FullCommand(TFSCommand Cmd, ...);
  std::wstring FullCommand(TFSCommand Cmd, va_list args);
  static std::wstring ExtractCommand(std::wstring Command);
  // __property int MaxLines[TFSCommand Cmd]  = { read=GetMaxLines};
  int GetMaxLines(TFSCommand Cmd);
  // __property int MinLines[TFSCommand Cmd]  = { read=GetMinLines };
  int GetMinLines(TFSCommand Cmd);
  // __property bool ModifiesFiles[TFSCommand Cmd]  = { read=GetModifiesFiles };
  bool GetModifiesFiles(TFSCommand Cmd);
  // __property bool ChangesDirectory[TFSCommand Cmd]  = { read=GetChangesDirectory };
  bool GetChangesDirectory(TFSCommand Cmd);
  // __property bool OneLineCommand[TFSCommand Cmd]  = { read=GetOneLineCommand };
  bool GetOneLineCommand(TFSCommand Cmd);
  // __property std::wstring Commands[TFSCommand Cmd]  = { read=GetCommands, write=SetCommands };
  std::wstring GetCommand(TFSCommand Cmd);
  void SetCommand(TFSCommand Cmd, std::wstring value);
  // __property std::wstring FirstLine = { read = GetFirstLine };
  std::wstring GetFirstLine();
  // __property bool InteractiveCommand[TFSCommand Cmd] = { read = GetInteractiveCommand };
  bool GetInteractiveCommand(TFSCommand Cmd);
  // __property std::wstring LastLine  = { read=GetLastLine };
  std::wstring GetLastLine();
  //  __property TSessionData * SessionData  = { read=FSessionData, write=FSessionData };
  TSessionData * GetSessionData() { return FSessionData; }
  void SetSessionData(TSessionData * value) { FSessionData = value; }
  // __property std::wstring ReturnVar  = { read=GetReturnVar, write=FReturnVar };
  std::wstring GetReturnVar();
  void SetReturnVar(std::wstring value) { FReturnVar = value; }
};
const wchar_t FullTimeOption[] = L"--full-time";
//---------------------------------------------------------------------------
THTTPCommandSet::THTTPCommandSet(TSessionData *aSessionData):
  FSessionData(aSessionData), FReturnVar(L"")
{
  assert(FSessionData);
  memset(&CommandSet, 0, sizeof(CommandSet));
  Default();
}
//---------------------------------------------------------------------------
void THTTPCommandSet::CopyFrom(THTTPCommandSet * Source)
{
  memcpy(&CommandSet, Source->CommandSet, sizeof(CommandSet));
}
//---------------------------------------------------------------------------
void THTTPCommandSet::Default()
{
}
//---------------------------------------------------------------------------
int THTTPCommandSet::GetMaxLines(TFSCommand Cmd)
{
  CHECK_CMD;
  return CommandSet[Cmd].MaxLines;
}
//---------------------------------------------------------------------------
int THTTPCommandSet::GetMinLines(TFSCommand Cmd)
{
  CHECK_CMD;
  return CommandSet[Cmd].MinLines;
}
//---------------------------------------------------------------------------
bool THTTPCommandSet::GetModifiesFiles(TFSCommand Cmd)
{
  CHECK_CMD;
  return CommandSet[Cmd].ModifiesFiles;
}
//---------------------------------------------------------------------------
bool THTTPCommandSet::GetChangesDirectory(TFSCommand Cmd)
{
  CHECK_CMD;
  return CommandSet[Cmd].ChangesDirectory;
}
//---------------------------------------------------------------------------
bool THTTPCommandSet::GetInteractiveCommand(TFSCommand Cmd)
{
  CHECK_CMD;
  return CommandSet[Cmd].InteractiveCommand;
}
//---------------------------------------------------------------------------
bool THTTPCommandSet::GetOneLineCommand(TFSCommand /*Cmd*/)
{
  //CHECK_CMD;
  // #56: we send "echo last line" from all commands on same line
  // just as it was in 1.0
  return true; //CommandSet[Cmd].OneLineCommand;
}
//---------------------------------------------------------------------------
void THTTPCommandSet::SetCommand(TFSCommand Cmd, std::wstring value)
{
  CHECK_CMD;
  wcscpy((wchar_t *)CommandSet[Cmd].Command, value.substr(0, MaxCommandLen - 1).c_str());
}
//---------------------------------------------------------------------------
std::wstring THTTPCommandSet::GetCommand(TFSCommand Cmd)
{
  CHECK_CMD;
  return CommandSet[Cmd].Command;
}
//---------------------------------------------------------------------------
std::wstring THTTPCommandSet::Command(TFSCommand Cmd, ...)
{
  std::wstring result;
  va_list args;
  va_start(args, Cmd);
  result = Command(Cmd, args);
  va_end(args);
  return result;
}
//---------------------------------------------------------------------------
std::wstring THTTPCommandSet::Command(TFSCommand Cmd, va_list args)
{
  // DEBUG_PRINTF(L"Cmd = %d, GetCommand(Cmd) = %s", Cmd, GetCommand(Cmd).c_str());
  std::wstring result;
  result = ::Format(GetCommand(Cmd).c_str(), args);
  // DEBUG_PRINTF(L"result = %s", result.c_str());
  return result.c_str();
}
//---------------------------------------------------------------------------
std::wstring THTTPCommandSet::FullCommand(TFSCommand Cmd, ...)
{
  std::wstring Result;
  va_list args;
  va_start(args, Cmd);
  Result = FullCommand(Cmd, args);
  va_end(args);
  return Result.c_str();
}
//---------------------------------------------------------------------------
std::wstring THTTPCommandSet::FullCommand(TFSCommand Cmd, va_list args)
{
  std::wstring Separator;
  if (GetOneLineCommand(Cmd))
    Separator = L" ; ";
  else
    Separator = L"\n";
  std::wstring Line = Command(Cmd, args);
  std::wstring LastLineCmd =
    Command(fsLastLine, GetLastLine().c_str(), GetReturnVar().c_str());
  std::wstring FirstLineCmd;
  if (GetInteractiveCommand(Cmd))
  {
    FirstLineCmd = Command(fsFirstLine, GetFirstLine().c_str()) + Separator;
    // DEBUG_PRINTF(L"FirstLineCmd1 = '%s'", FirstLineCmd.c_str());
  }

  std::wstring Result;
  if (!Line.empty())
    Result = FORMAT(L"%s%s%s%s", FirstLineCmd.c_str(), Line.c_str(), Separator.c_str(), LastLineCmd.c_str());
  else
    Result = FORMAT(L"%s%s", FirstLineCmd.c_str(), LastLineCmd.c_str());
  // DEBUG_PRINTF(L"Result = %s", Result.c_str());
  return Result;
}
//---------------------------------------------------------------------------
std::wstring THTTPCommandSet::GetFirstLine()
{
  return FIRST_LINE;
}
//---------------------------------------------------------------------------
std::wstring THTTPCommandSet::GetLastLine()
{
  return LAST_LINE;
}
//---------------------------------------------------------------------------
std::wstring THTTPCommandSet::GetReturnVar()
{
  assert(GetSessionData());
  if (!FReturnVar.empty())
      return std::wstring(L"$") + FReturnVar;
  else if (GetSessionData()->GetDetectReturnVar())
      return L"0";
  else
      return std::wstring(L"$") + GetSessionData()->GetReturnVar();
}
//---------------------------------------------------------------------------
std::wstring THTTPCommandSet::ExtractCommand(std::wstring Command)
{
  size_t P = Command.find_first_of(L" ");
  if (P != std::wstring::npos)
  {
    Command.resize(P);
  }
  return Command;
}
//---------------------------------------------------------------------------
TStrings * THTTPCommandSet::CreateCommandList()
{
  TStrings * CommandList = new TStringList();
  for (int Index = 0; Index < ShellCommandCount; Index++)
  {
    std::wstring Cmd = GetCommand((TFSCommand)Index);
    if (!Cmd.empty())
    {
      Cmd = ExtractCommand(Cmd);
      if ((Cmd != L"%s") && (CommandList->IndexOf(Cmd.c_str()) < 0))
        CommandList->Add(Cmd);
    }
  }
  return CommandList;
}
//===========================================================================
class TMessageQueue : public std::list<std::pair<WPARAM, LPARAM> >
{
};
//===========================================================================
struct TFileTransferData
{
  TFileTransferData()
  {
    Params = 0;
    AutoResume = false;
    OverwriteResult = -1;
    CopyParam = NULL;
  }

  std::wstring FileName;
  int Params;
  bool AutoResume;
  int OverwriteResult;
  const TCopyParamType * CopyParam;
};

//===========================================================================
THTTPFileSystem::THTTPFileSystem(TTerminal *ATerminal) :
  TCustomFileSystem(ATerminal),
  // FSecureShell(NULL),
  FCommandSet(NULL),
  FFileList(NULL),
  FFileListCache(NULL),
  FOutput(NULL),
  FReturnCode(0),
  FProcessingCommand(false),
  FCURLIntf(NULL),
  FPasswordFailed(false),
  FActive(false),
  FWaitingForReply(false),
  FFileTransferAbort(ftaNone),
  FIgnoreFileList(false),
  FFileTransferCancelled(false),
  FFileTransferResumed(0),
  FFileTransferPreserveTime(false),
  FFileTransferCPSLimit(0),
  FAwaitingProgress(false),
  FReply(0),
  FCommandReply(0),
  FMultineResponse(false),
  FLastCode(0),
  FLastCodeClass(0),
  FLastReadDirectoryProgress(0),
  FLastResponse(new TStringList()),
  FLastError(new TStringList()),
  FQueueCriticalSection(new TCriticalSection),
  FTransferStatusCriticalSection(new TCriticalSection),
  FQueue(new TMessageQueue),
  FQueueEvent(CreateEvent(NULL, true, false, NULL)),
  FAbortEvent(CreateEvent(NULL, true, false, NULL)),
  m_ProgressPercent(0),
  FDoListAll(false)
{
  Self = this;
}

void THTTPFileSystem::Init(TSecureShell *SecureShell)
{
  // FSecureShell = SecureShell;
  FCommandSet = new THTTPCommandSet(FTerminal->GetSessionData());
  FLsFullTime = FTerminal->GetSessionData()->GetSCPLsFullTime();
  FOutput = new TStringList();
  FProcessingCommand = false;

  FFileSystemInfo.ProtocolBaseName = L"HTTP";
  FFileSystemInfo.ProtocolName = FFileSystemInfo.ProtocolBaseName;
  // capabilities of SCP protocol are fixed
  for (int Index = 0; Index < fcCount; Index++)
  {
    FFileSystemInfo.IsCapable[Index] = IsCapable((TFSCapability)Index);
  }
}
//---------------------------------------------------------------------------
THTTPFileSystem::~THTTPFileSystem()
{
  delete FCommandSet;
  FCommandSet = NULL;
  delete FOutput;
  FOutput = NULL;
  delete FLastResponse;
  FLastResponse = NULL;
  delete FLastError;
  FLastError = NULL;
  delete FQueueCriticalSection;
  FQueueCriticalSection = NULL;
  delete FTransferStatusCriticalSection;
  FTransferStatusCriticalSection = NULL;
  // delete FSecureShell;
  delete FQueue;
  FQueue = NULL;
  delete FCURLIntf;
  FCURLIntf = NULL;
  CloseHandle(FQueueEvent);
  CloseHandle(FAbortEvent);
}
//---------------------------------------------------------------------------
void THTTPFileSystem::Open()
{
  DEBUG_PRINTF(L"begin");
  // FSecureShell->Open();
  DiscardMessages();

  ResetCaches();
  FCurrentDirectory = L"";
  FHomeDirectory = L"";

  TSessionData *Data = FTerminal->GetSessionData();

  FSessionInfo.LoginTime = Now();
  FSessionInfo.ProtocolBaseName = L"HTTP";
  FSessionInfo.ProtocolName = FSessionInfo.ProtocolBaseName;

  FLastDataSent = Now();

  FMultineResponse = false;

  // initialize FCURLIntf on the first connect only
  if (FCURLIntf == NULL)
  {
    FCURLIntf = new CEasyURL();
    FCURLIntf->Init();

    try
    {
      TCURLIntf::TLogLevel LogLevel;
      switch (FTerminal->GetConfiguration()->GetActualLogProtocol())
      {
        default:
        case 0:
        case 1:
          LogLevel = TCURLIntf::LOG_WARNING;
          break;

        case 2:
          LogLevel = TCURLIntf::LOG_INFO;
          break;
      }
      FCURLIntf->SetDebugLevel(LogLevel);
    }
    catch (...)
    {
      delete FCURLIntf;
      FCURLIntf = NULL;
      throw;
    }
  }

  std::wstring HostName = Data->GetHostName();
  std::wstring UserName = Data->GetUserName();
  std::wstring Password = Data->GetPassword();
  std::wstring Account = Data->GetFtpAccount();
  std::wstring Path = Data->GetRemoteDirectory();
  int ServerType = 0;
  // int Pasv = (Data->GetFtpPasvMode() ? 1 : 2);
  // int TimeZoneOffset = int(Round(double(Data->GetTimeDifference()) * 24 * 60));
  int UTF8 = 0;
  switch (Data->GetNotUtf())
  {
    case asOn:
      UTF8 = 2;
      break;

    case asOff:
      UTF8 = 1;
      break;

    case asAuto:
      UTF8 = 0;
      break;
  };

  FPasswordFailed = false;
  bool PromptedForCredentials = false;

  do
  {
    FSystem = L"";
    // FFeatures->Clear();
    // FFileSystemInfoValid = false;

    // TODO: the same for account? it ever used?

    // ask for username if it was not specified in advance, even on retry,
    // but keep previous one as default,
    if (0) // Data->GetUserName().empty())
    {
      FTerminal->LogEvent(L"Username prompt (no username provided)");

      if (!FPasswordFailed && !PromptedForCredentials)
      {
        FTerminal->Information(LoadStr(FTP_CREDENTIAL_PROMPT), false);
        PromptedForCredentials = true;
      }

      if (!FTerminal->PromptUser(Data, pkUserName, LoadStr(USERNAME_TITLE), L"",
            LoadStr(USERNAME_PROMPT2), true, 0, UserName))
      {
        FTerminal->FatalError(NULL, LoadStr(AUTHENTICATION_FAILED));
      }
      else
      {
        FUserName = UserName;
      }
    }

    // ask for password if it was not specified in advance,
    // on retry ask always
    if (0) // (Data->GetPassword().empty() && !Data->GetPasswordless()) || FPasswordFailed)
    {
      FTerminal->LogEvent(L"Password prompt (no password provided or last login attempt failed)");

      if (!FPasswordFailed && !PromptedForCredentials)
      {
        FTerminal->Information(LoadStr(FTP_CREDENTIAL_PROMPT), false);
        PromptedForCredentials = true;
      }

      // on retry ask for new password
      Password = L"";
      if (!FTerminal->PromptUser(Data, pkPassword, LoadStr(PASSWORD_TITLE), L"",
            LoadStr(PASSWORD_PROMPT), false, 0, Password))
      {
        FTerminal->FatalError(NULL, LoadStr(AUTHENTICATION_FAILED));
      }
    }

    ProxySettings proxySettings;
    // init proxySettings
    proxySettings.proxyType = GetOptionVal(OPTION_PROXYTYPE);
    proxySettings.proxyHost = GetOption(OPTION_PROXYHOST);
    proxySettings.proxyPort = GetOptionVal(OPTION_PROXYPORT);
    proxySettings.proxyLogin = GetOption(OPTION_PROXYUSER);
    proxySettings.proxyPassword = GetOption(OPTION_PROXYPASS);

    FActive = FCURLIntf->Initialize(
      HostName.c_str(), // Data->GetPortNumber(),
	  UserName.c_str(),
      Password.c_str(),
	  // ::W2MB(Account.c_str()).c_str(),
      proxySettings);
	  // false,
	  // ::W2MB(Path.c_str()).c_str(),
      // ServerType, Pasv, TimeZoneOffset, UTF8, Data->GetFtpForcePasvIp());
    assert(FActive);
    FCURLIntf->SetAbortEvent(FAbortEvent);

    FPasswordFailed = false;
    #ifdef WEBDAV_ASYNC
    try
    {
      // do not wait for FTP response code as Connect is complex operation
      GotReply(WaitForCommandReply(false), REPLY_CONNECT, LoadStr(CONNECTION_FAILED));

      // we have passed, even if we got 530 on the way (if it is possible at all),
      // ignore it
      assert(!FPasswordFailed);
      FPasswordFailed = false;
    }
    catch (...)
    {
      if (FPasswordFailed)
      {
        FTerminal->Information(LoadStr(FTP_ACCESS_DENIED), false);
      }
      else
      {
        // see handling of REPLY_CONNECT in GotReply
        FTerminal->Closed();
        throw;
      }
    }
    #else
    // Check initial path existing
    std::wstring path;
    std::wstring query;
    ::ParseURL(HostName.c_str(), NULL, NULL, NULL, &path, NULL, NULL, NULL);
    bool dirExist = false;
    DEBUG_PRINTF(L"path = %s, query = %s", path.c_str(), query.c_str());
    // if (!query.empty())
    // {
        // path += query;
    // }
    std::wstring errorInfo;
    if (!CheckExisting(path.c_str(), ItemDirectory, dirExist, errorInfo) || !dirExist)
    {
        FTerminal->LogEvent(FORMAT(L"WebDAV: path %s does not exist.", path.c_str()));
        // return;
    }
    FCurrentDirectory = path;
    while (FCurrentDirectory.size() > 1 && FCurrentDirectory[FCurrentDirectory.length() - 1] == L'/')
    {
        FCurrentDirectory.erase(FCurrentDirectory.length() - 1);
    }
    #endif
  }
  while (FPasswordFailed);
  DEBUG_PRINTF(L"end");
}
//---------------------------------------------------------------------------
void THTTPFileSystem::Close()
{
  // FSecureShell->Close();
  assert(FActive);
  if (FCURLIntf->Close())
  {
    // CHECK(FLAGSET(WaitForCommandReply(false), TFileZillaIntf::REPLY_DISCONNECTED));
    assert(FActive);
    Discard();
    FTerminal->Closed();
  }
  else
  {
    assert(false);
  }
}
//---------------------------------------------------------------------------
bool THTTPFileSystem::GetActive()
{
  // return FSecureShell->GetActive();
  return FActive;
}
//---------------------------------------------------------------------------
const TSessionInfo & THTTPFileSystem::GetSessionInfo()
{
  return FSessionInfo; // FSecureShell->GetSessionInfo();
}
//---------------------------------------------------------------------------
const TFileSystemInfo & THTTPFileSystem::GetFileSystemInfo(bool Retrieve)
{
  if (FFileSystemInfo.AdditionalInfo.empty() && Retrieve)
  {
    std::wstring UName;
    FTerminal->SetExceptionOnFail(true);
    {
      BOOST_SCOPE_EXIT ( (&Self) )
      {
        Self->FTerminal->SetExceptionOnFail(false);
      } BOOST_SCOPE_EXIT_END
      try
      {
        AnyCommand(L"uname -a", NULL);
        for (size_t Index = 0; Index < GetOutput()->GetCount(); Index++)
        {
          if (Index > 0)
          {
            UName += L"; ";
          }
          UName += GetOutput()->GetString(Index);
        }
      }
      catch (...)
      {
        if (!FTerminal->GetActive())
        {
          throw;
        }
      }
    }

    FFileSystemInfo.RemoteSystem = UName;
  }

  return FFileSystemInfo;
}
//---------------------------------------------------------------------------
bool THTTPFileSystem::TemporaryTransferFile(const std::wstring & /*FileName*/)
{
  return false;
}
//---------------------------------------------------------------------------
bool THTTPFileSystem::GetStoredCredentialsTried()
{
  return false; // FSecureShell->GetStoredCredentialsTried();
}
//---------------------------------------------------------------------------
std::wstring THTTPFileSystem::GetUserName()
{
  return FUserName; // FSecureShell->GetUserName();
}
//---------------------------------------------------------------------------
void THTTPFileSystem::Idle()
{
  // Keep session alive
  if ((FTerminal->GetSessionData()->GetPingType ()!= ptOff) &&
      (Now() /*- FSecureShell->GetLastDataSent()*/ > FTerminal->GetSessionData()->GetPingIntervalDT()))
  {
    if ((FTerminal->GetSessionData()->GetPingType() == ptDummyCommand)) // &&
        // FSecureShell->GetReady())
    {
      if (!FProcessingCommand)
      {
        ExecCommand(fsNull, 0, NULL);
      }
      else
      {
        FTerminal->LogEvent(L"Cannot send keepalive, command is being executed");
        // send at least SSH-level keepalive, if nothing else, it at least updates
        // LastDataSent, no the next keepalive attempt is postponed
        // FSecureShell->KeepAlive();
      }
    }
    else
    {
      // FSecureShell->KeepAlive();
    }
  }

  // FSecureShell->Idle();
}
//---------------------------------------------------------------------------
std::wstring THTTPFileSystem::AbsolutePath(std::wstring Path, bool /*Local*/)
{
  return ::AbsolutePath(GetCurrentDirectory(), Path);
}
//---------------------------------------------------------------------------
bool THTTPFileSystem::IsCapable(int Capability) const
{
  assert(FTerminal);
  switch (Capability)
  {
    case fcUserGroupListing:
    case fcModeChanging:
    case fcModeChangingUpload:
    case fcPreservingTimestampUpload:
    case fcGroupChanging:
    case fcOwnerChanging:
    case fcAnyCommand:
    case fcShellAnyCommand:
    case fcHardLink:
    case fcSymbolicLink:
    case fcResolveSymlink:
    case fcRename:
    case fcRemoteMove:
    case fcRemoteCopy:
      return true;

    case fcTextMode:
      return FTerminal->GetSessionData()->GetEOLType() != FTerminal->GetConfiguration()->GetLocalEOLType();

    case fcNativeTextMode:
    case fcNewerOnlyUpload:
    case fcTimestampChanging:
    case fcLoadingAdditionalProperties:
    case fcCheckingSpaceAvailable:
    case fcIgnorePermErrors:
    case fcCalculatingChecksum:
    case fcSecondaryShell: // has fcShellAnyCommand
    case fcGroupOwnerChangingByID: // by name
      return false;

    default:
      assert(false);
      return false;
  }
}
//---------------------------------------------------------------------------
std::wstring THTTPFileSystem::DelimitStr(std::wstring Str)
{
  if (!Str.empty())
  {
    Str = ::DelimitStr(Str, L"\\`$\"");
    if (Str[0] == L'-') Str = L"./" + Str;
  }
  return Str;
}
//---------------------------------------------------------------------------
std::wstring THTTPFileSystem::ActualCurrentDirectory()
{
  char CurrentPath[1024];
  // FFileZillaIntf->GetCurrentPath(CurrentPath, sizeof(CurrentPath));
  std::wstring fn = UnixExcludeTrailingBackslash(std::wstring(::MB2W(CurrentPath)));
    if (fn.empty())
    {
        fn = L"/";
    }
  return fn;
}

//---------------------------------------------------------------------------
void THTTPFileSystem::EnsureLocation()
{
  if (!FCachedDirectoryChange.empty())
  {
    FTerminal->LogEvent(FORMAT(L"Locating to cached directory \"%s\".",
      FCachedDirectoryChange.c_str()));
    std::wstring Directory = FCachedDirectoryChange;
    FCachedDirectoryChange = L"";
    try
    {
      ChangeDirectory(Directory);
    }
    catch (...)
    {
      // when location to cached directory fails, pretend again
      // location in cached directory
      // here used to be check (CurrentDirectory != Directory), but it is
      // false always (currentdirectory is already set to cached directory),
      // making the condition below useless. check removed.
      if (FTerminal->GetActive())
      {
        FCachedDirectoryChange = Directory;
      }
      throw;
    }
  }
}

void THTTPFileSystem::Discard()
{
  // remove all pending messages, to get complete log
  // note that we need to retry discard on reconnect, as there still may be another
  // "disconnect/timeout/..." status messages coming
  DiscardMessages();
  assert(FActive);
  FActive = false;
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
void THTTPFileSystem::SendCommand(const std::wstring Cmd)
{
  EnsureLocation();

  std::wstring Line;
  // FSecureShell->ClearStdError();
  FReturnCode = 0;
  FOutput->Clear();
  // We suppose, that 'Cmd' already contains command that ensures,
  // that 'LastLine' will be printed
  // DEBUG_PRINTF(L"Cmd = %s", Cmd.c_str());
  // FSecureShell->SendLine(Cmd);
  FProcessingCommand = true;
}
//---------------------------------------------------------------------------
bool THTTPFileSystem::IsTotalListingLine(const std::wstring Line)
{
  // On some hosts there is not "total" but "totalt". What's the reason??
  // see mail from "Jan Wiklund (SysOp)" <jan@park.se>
  return !::AnsiCompareIC(Line.substr(0, 5), L"total");
}
//---------------------------------------------------------------------------
bool THTTPFileSystem::RemoveLastLine(std::wstring & Line,
    int & ReturnCode, std::wstring LastLine)
{
  bool IsLastLine = false;
  if (LastLine.empty()) LastLine = LAST_LINE;
  // #55: fixed so, even when last line of command output does not
  // contain CR/LF, we can recognize last line
  size_t Pos = Line.find(LastLine);
  // DEBUG_PRINTF(L"Line = %s, LastLine = %s, Pos = %d", Line.c_str(), LastLine.c_str(), Pos);
  if (Pos != std::wstring::npos)
  {
    // 2003-07-14: There must be nothing after return code number to
    // consider string as last line. This fixes bug with 'set' command
    // in console window
    std::wstring ReturnCodeStr = ::TrimRight(Line.substr(Pos + LastLine.size() + 1,
      Line.size() - Pos + LastLine.size()));
    // DEBUG_PRINTF(L"ReturnCodeStr = '%s'", ReturnCodeStr.c_str());
    if (TryStrToInt(ReturnCodeStr, ReturnCode) || (ReturnCodeStr == L"0"))
    {
      IsLastLine = true;
      // DEBUG_PRINTF(L"Line1 = %s", Line.c_str());
      // if ((Pos != std::wstring::npos) && (Pos != 0))
      {
        Line.resize(Pos);
      }
      // DEBUG_PRINTF(L"Line2 = %s", Line.c_str());
    }
  }
  return IsLastLine;
}
//---------------------------------------------------------------------------
bool THTTPFileSystem::IsLastLine(std::wstring & Line)
{
  bool Result = false;
  try
  {
    Result = RemoveLastLine(Line, FReturnCode, FCommandSet->GetLastLine());
  }
  catch (const std::exception &E)
  {
    FTerminal->TerminalError(&E, LoadStr(CANT_DETECT_RETURN_CODE));
  }
  return Result;
}
//---------------------------------------------------------------------------
void THTTPFileSystem::SkipFirstLine()
{
  std::wstring Line = L""; // FSecureShell->ReceiveLine();
  if (Line != FCommandSet->GetFirstLine())
  {
    FTerminal->TerminalError(NULL, FMTLOAD(FIRST_LINE_EXPECTED, Line.c_str()));
  }
}
//---------------------------------------------------------------------------
void THTTPFileSystem::ReadCommandOutput(int Params, const std::wstring *Cmd)
{
  {
    BOOST_SCOPE_EXIT ( (&Self) )
    {
      Self->FProcessingCommand = false;
    } BOOST_SCOPE_EXIT_END
    if (Params & coWaitForLastLine)
    {
      std::wstring Line;
      bool IsLast = true;
      unsigned int Total = 0;
      // #55: fixed so, even when last line of command output does not
      // contain CR/LF, we can recognize last line
      do
      {
        Line = L""; // FSecureShell->ReceiveLine();
        // DEBUG_PRINTF(L"Line = %s", Line.c_str());
        IsLast = true; // IsLastLine(Line);
        if (!IsLast || !Line.empty())
        {
          FOutput->Add(Line);
          if (FLAGSET(Params, coReadProgress))
          {
            Total++;

            if (Total % 10 == 0)
            {
              bool Cancel = false; //dummy
              FTerminal->DoReadDirectoryProgress(Total, Cancel);
            }
          }
        }
      }
      while (!IsLast);
    }
    if (Params & coRaiseExcept)
    {
      std::wstring Message = L""; // FSecureShell->GetStdError();
      if ((Params & coExpectNoOutput) && FOutput->GetCount())
      {
        if (!Message.empty()) Message += L"\n";
        Message += FOutput->GetText();
      }
      while (!Message.empty() && (::LastDelimiter(Message, L"\n\r") == Message.size()))
      {
        Message.resize(Message.size() - 1);
      }

      bool WrongReturnCode =
        (GetReturnCode() > 1) || (GetReturnCode() == 1 && !(Params & coIgnoreWarnings));

      if (Params & coOnlyReturnCode && WrongReturnCode)
      {
        FTerminal->TerminalError(FMTLOAD(COMMAND_FAILED_CODEONLY, GetReturnCode()));
      }
        else
      if (!(Params & coOnlyReturnCode) &&
          ((!Message.empty() && ((FOutput->GetCount() == 0) || !(Params & coIgnoreWarnings))) ||
           WrongReturnCode))
      {
        assert(Cmd != NULL);
        FTerminal->TerminalError(FMTLOAD(COMMAND_FAILED, Cmd->c_str(), GetReturnCode(), Message.c_str()));
      }
    }
  }
}
//---------------------------------------------------------------------------
void THTTPFileSystem::ExecCommand(const std::wstring & Cmd, int Params,
  const std::wstring &CmdString)
{
  if (Params < 0) Params = ecDefault;
  if (FTerminal->GetUseBusyCursor())
  {
    ::Busy(true);
  }
  {
    BOOST_SCOPE_EXIT ( (&Self) )
    {
      if (Self->FTerminal->GetUseBusyCursor())
      {
        ::Busy(false);
      }
    } BOOST_SCOPE_EXIT_END
    DEBUG_PRINTF(L"Cmd = %s", Cmd.c_str());
    SendCommand(Cmd);

    int COParams = coWaitForLastLine;
    if (Params & ecRaiseExcept) COParams |= coRaiseExcept;
    if (Params & ecIgnoreWarnings) COParams |= coIgnoreWarnings;
    if (Params & ecReadProgress) COParams |= coReadProgress;
    ReadCommandOutput(COParams, &CmdString);
  }
}
//---------------------------------------------------------------------------
void THTTPFileSystem::ExecCommand(TFSCommand Cmd, int Params, ...)
{
  if (Params < 0) Params = ecDefault;
  va_list args;
  va_start(args, Params);
  std::wstring FullCommand = FCommandSet->FullCommand(Cmd, args);
  std::wstring Command = FCommandSet->Command(Cmd, args);
  ExecCommand(FullCommand, Params, Command);
  va_end(args);
  if (Params & ecRaiseExcept)
  {
    size_t MinL = FCommandSet->GetMinLines(Cmd);
    size_t MaxL = FCommandSet->GetMaxLines(Cmd);
    if (((MinL >= 0) && (MinL > FOutput->GetCount())) ||
        ((MaxL >= 0) && (MaxL > FOutput->GetCount())))
    {
      FTerminal->TerminalError(::FmtLoadStr(INVALID_OUTPUT_ERROR,
        FullCommand.c_str(), GetOutput()->GetText().c_str()));
    }
  }
}
//---------------------------------------------------------------------------
std::wstring THTTPFileSystem::GetCurrentDirectory()
{
  return FCurrentDirectory;
}
//---------------------------------------------------------------------------
void THTTPFileSystem::DoStartup()
{
  DEBUG_PRINTF(L"begin");
  // DetectReturnVar must succeed,
  // otherwise session is to be closed.
  FTerminal->SetExceptionOnFail(true);
  // if (FTerminal->GetSessionData()->GetDetectReturnVar()) DetectReturnVar();
  FTerminal->SetExceptionOnFail(false);

  // retrieve initialize working directory to save it as home directory
  ReadCurrentDirectory();
  FHomeDirectory = FCurrentDirectory;
  DEBUG_PRINTF(L"end");
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
void THTTPFileSystem::LookupUsersGroups()
{
  ExecCommand(fsLookupUsersGroups);
  FTerminal->FUsers.Clear();
  FTerminal->FGroups.Clear();
  if (FOutput->GetCount() > 0)
  {
    std::wstring Groups = FOutput->GetString(0);
    while (!Groups.empty())
    {
      std::wstring NewGroup = CutToChar(Groups, ' ', false);
      FTerminal->FGroups.Add(TRemoteToken(NewGroup));
      FTerminal->FMembership.Add(TRemoteToken(NewGroup));
    }
  }
}
//---------------------------------------------------------------------------
void THTTPFileSystem::DetectReturnVar()
{
  // This suppose that something was already executed (probably SkipStartupMessage())
  // or return code variable is already set on start up.

  try
  {
    // #60 17.10.01: "status" and "?" switched
    std::wstring ReturnVars[2] = { L"status", L"?" };
    std::wstring NewReturnVar = L"";
    FTerminal->LogEvent(L"Detecting variable containing return code of last command.");
    for (int Index = 0; Index < 2; Index++)
    {
      bool Success = true;

      try
      {
        FTerminal->LogEvent(FORMAT(L"Trying \"$%s\".", ReturnVars[Index].c_str()));
        ExecCommand(fsVarValue, 0, ReturnVars[Index].c_str());
        // DEBUG_PRINTF(L"GetOutput()->GetCount = %d, GetOutput()->GetString(0) = %s", GetOutput()->GetCount(), GetOutput()->GetString(0).c_str());
        std::wstring str = GetOutput()->GetCount() > 0 ? GetOutput()->GetString(0) : L"";
        int val = StrToIntDef(str, 256);
        if ((GetOutput()->GetCount() != 1) || str.empty() || (val > 255))
        {
          FTerminal->LogEvent(L"The response is not numerical exit code");
          Abort();
        }
      }
      catch (const EFatal &E)
      {
        // if fatal error occurs, we need to exit ...
        throw;
      }
      catch (const std::exception &E)
      {
        // ...otherwise, we will try next variable (if any)
        Success = false;
      }

      if (Success)
      {
        NewReturnVar = ReturnVars[Index];
        break;
      }
    }

    if (NewReturnVar.empty())
    {
      Abort();
    }
      else
    {
      FCommandSet->SetReturnVar(NewReturnVar);
      FTerminal->LogEvent(FORMAT(L"Return code variable \"%s\" selected.",
        FCommandSet->GetReturnVar().c_str()));
    }
  }
  catch (const std::exception &E)
  {
    FTerminal->CommandError(&E, LoadStr(DETECT_RETURNVAR_ERROR));
  }
}
//---------------------------------------------------------------------------
void THTTPFileSystem::ClearAlias(std::wstring Alias)
{
  if (!Alias.empty())
  {
    // this command usually fails, because there will never be
    // aliases on all commands -> see last false parametr
    // DEBUG_PRINTF(L"Alias = %s", Alias.c_str());
    ExecCommand(fsUnalias, 0, Alias.c_str(), false);
  }
}
//---------------------------------------------------------------------------
void THTTPFileSystem::ClearAliases()
{
  try
  {
    FTerminal->LogEvent(L"Clearing all aliases.");
    ClearAlias(THTTPCommandSet::ExtractCommand(FTerminal->GetSessionData()->GetListingCommand()));
    TStrings * CommandList = FCommandSet->CreateCommandList();
    {
      BOOST_SCOPE_EXIT ( (&CommandList) )
      {
        delete CommandList;
      } BOOST_SCOPE_EXIT_END
      for (size_t Index = 0; Index < CommandList->GetCount(); Index++)
      {
        ClearAlias(CommandList->GetString(Index));
      }
    }
  }
  catch (const std::exception &E)
  {
    FTerminal->CommandError(&E, LoadStr(UNALIAS_ALL_ERROR));
  }
}
//---------------------------------------------------------------------------
void THTTPFileSystem::UnsetNationalVars()
{
  try
  {
    FTerminal->LogEvent(L"Clearing national user variables.");
    for (size_t Index = 0; Index < NationalVarCount; Index++)
    {
      ExecCommand(fsUnset, 0, NationalVars[Index], false);
    }
  }
  catch (const std::exception &E)
  {
    FTerminal->CommandError(&E, LoadStr(UNSET_NATIONAL_ERROR));
  }
}
//---------------------------------------------------------------------------
void THTTPFileSystem::ReadCurrentDirectory()
{
  DEBUG_PRINTF(L"begin, FCurrentDirectory = %s", FCurrentDirectory.c_str());
  if (FCachedDirectoryChange.empty())
  {
    // ExecCommand(fsCurrentDirectory);
    std::wstring response;
    std::wstring errorInfo;
    bool isExist = SendPropFindRequest(FCurrentDirectory.c_str(), response, errorInfo);
    DEBUG_PRINTF(L"responce = %s, errorInfo = %s", response.c_str(), errorInfo.c_str());
    if (FOutput->GetCount())
    {
        FCurrentDirectory = UnixExcludeTrailingBackslash(FOutput->GetString(0));
    }
  }
  else
  {
    FCurrentDirectory = FCachedDirectoryChange;
  }
  DEBUG_PRINTF(L"end, FCurrentDirectory = %s", FCurrentDirectory.c_str());
}
//---------------------------------------------------------------------------
void THTTPFileSystem::HomeDirectory()
{
  ExecCommand(fsHomeDirectory);
}
//---------------------------------------------------------------------------
void THTTPFileSystem::AnnounceFileListOperation()
{
  // noop
}
//---------------------------------------------------------------------------
void THTTPFileSystem::ChangeDirectory(const std::wstring Directory)
{
  std::wstring ToDir;
  if (!Directory.empty() &&
      ((Directory[0] != L'~') || (Directory.substr(0, 2) == L"~ ")))
  {
    ToDir = L"\"" + DelimitStr(Directory) + L"\"";
  }
  else
  {
    ToDir = DelimitStr(Directory);
  }
  ExecCommand(fsChangeDirectory, 0, ToDir.c_str());
  FCachedDirectoryChange = L"";
}
//---------------------------------------------------------------------------
void THTTPFileSystem::CachedChangeDirectory(const std::wstring Directory)
{
  FCachedDirectoryChange = UnixExcludeTrailingBackslash(Directory);
}
//---------------------------------------------------------------------------
void THTTPFileSystem::ReadDirectory(TRemoteFileList * FileList)
{
  DEBUG_PRINTF(L"begin");
  assert(FileList);
  // emtying file list moved before command execution
  FileList->Clear();

  bool Again = false;

  do
  {
    Again = false;
    try
    {
      int Params = ecDefault | ecReadProgress |
        FLAGMASK(FTerminal->GetSessionData()->GetIgnoreLsWarnings(), ecIgnoreWarnings);
      const wchar_t * Options =
        ((FLsFullTime == asAuto) || (FLsFullTime == asOn)) ? FullTimeOption : L"";
      bool ListCurrentDirectory = (FileList->GetDirectory() == FTerminal->GetCurrentDirectory());
      if (ListCurrentDirectory)
      {
        FTerminal->LogEvent(L"Listing current directory.");
        ExecCommand(fsListCurrentDirectory,
          0, FTerminal->GetSessionData()->GetListingCommand().c_str(), Options, Params);
      }
      else
      {
        FTerminal->LogEvent(FORMAT(L"Listing directory \"%s\".",
          FileList->GetDirectory().c_str()));
        ExecCommand(fsListDirectory,
          0, FTerminal->GetSessionData()->GetListingCommand().c_str(), Options,
            DelimitStr(FileList->GetDirectory().c_str()).c_str(),
          Params);
      }

      TRemoteFile * File = NULL;

      // If output is not empty, we have succesfully got file listing,
      // otherwise there was an error, in case it was "permission denied"
      // we try to get at least parent directory (see "else" statement below)
      if (FOutput->GetCount() > 0)
      {
        // Copy LS command output, because eventual symlink analysis would
        // modify FTerminal->Output
        TStringList * OutputCopy = new TStringList();
        {
          BOOST_SCOPE_EXIT ( (&OutputCopy) )
          {
            delete OutputCopy;
          } BOOST_SCOPE_EXIT_END
          OutputCopy->Assign(FOutput);

          // delete leading "total xxx" line
          // On some hosts there is not "total" but "totalt". What's the reason??
          // see mail from "Jan Wiklund (SysOp)" <jan@park.se>
          if (IsTotalListingLine(OutputCopy->GetString(0)))
          {
            OutputCopy->Delete(0);
          }

          for (size_t Index = 0; Index < OutputCopy->GetCount(); Index++)
          {
            File = CreateRemoteFile(OutputCopy->GetString(Index));
            FileList->AddFile(File);
          }
        }
      }
      else
      {
        bool Empty = true;
        if (ListCurrentDirectory)
        {
          // Empty file list -> probably "permision denied", we
          // at least get link to parent directory ("..")
          FTerminal->ReadFile(
            UnixIncludeTrailingBackslash(FTerminal->FFiles->GetDirectory()) +
              PARENTDIRECTORY, File);
          Empty = (File == NULL || (wcscmp(File->GetFileName().c_str(), PARENTDIRECTORY) == 0));
          if (!Empty)
          {
            assert(File->GetIsParentDirectory());
            FileList->AddFile(File);
          }
        }
        else
        {
          Empty = true;
        }

        if (Empty)
        {
          throw ExtException(FMTLOAD(EMPTY_DIRECTORY, FileList->GetDirectory().c_str()));
        }
      }

      if (FLsFullTime == asAuto)
      {
          FTerminal->LogEvent(
            FORMAT(L"Directory listing with %s succeed, next time all errors during "
              L"directory listing will be displayed immediatelly.",
              FullTimeOption));
          FLsFullTime = asOn;
      }
    }
    catch (const std::exception & E)
    {
      if (FTerminal->GetActive())
      {
        if (FLsFullTime == asAuto)
        {
          FTerminal->FLog->AddException(&E);
          FLsFullTime = asOff;
          Again = true;
          FTerminal->LogEvent(
            FORMAT(L"Directory listing with %s failed, try again regular listing.",
            FullTimeOption));
        }
        else
        {
          throw;
        }
      }
      else
      {
        throw;
      }
    }
  }
  while (Again);
  DEBUG_PRINTF(L"end");
}
//---------------------------------------------------------------------------
void THTTPFileSystem::ReadSymlink(TRemoteFile * SymlinkFile,
  TRemoteFile *& File)
{
  CustomReadFile(SymlinkFile->GetLinkTo(), File, SymlinkFile);
}
//---------------------------------------------------------------------------
void THTTPFileSystem::ReadFile(const std::wstring FileName,
  TRemoteFile *& File)
{
  CustomReadFile(FileName, File, NULL);
}
//---------------------------------------------------------------------------
TRemoteFile * THTTPFileSystem::CreateRemoteFile(
  const std::wstring & ListingStr, TRemoteFile * LinkedByFile)
{
  TRemoteFile * File = new TRemoteFile(LinkedByFile);
  try
  {
    File->SetTerminal(FTerminal);
    File->SetListingStr(ListingStr);
    File->ShiftTime(FTerminal->GetSessionData()->GetTimeDifference());
    File->Complete();
  }
  catch (...)
  {
    delete File;
    throw;
  }

  return File;
}
//---------------------------------------------------------------------------
void THTTPFileSystem::CustomReadFile(const std::wstring FileName,
  TRemoteFile *& File, TRemoteFile * ALinkedByFile)
{
  File = NULL;
  int Params = ecDefault |
    FLAGMASK(FTerminal->GetSessionData()->GetIgnoreLsWarnings(), ecIgnoreWarnings);
  // the auto-detection of --full-time support is not implemented for fsListFile,
  // so we use it only if we already know that it is supported (asOn).
  const wchar_t * Options = (FLsFullTime == asOn) ? FullTimeOption : L"";
  ExecCommand(fsListFile,
    Params, FTerminal->GetSessionData()->GetListingCommand().c_str(), Options, DelimitStr(FileName).c_str());
  if (FOutput->GetCount())
  {
    int LineIndex = 0;
    if (IsTotalListingLine(FOutput->GetString(LineIndex)) && FOutput->GetCount() > 1)
    {
      LineIndex++;
    }

    File = CreateRemoteFile(FOutput->GetString(LineIndex), ALinkedByFile);
  }
}
//---------------------------------------------------------------------------
void THTTPFileSystem::DeleteFile(const std::wstring FileName,
  const TRemoteFile * File, int Params, TRmSessionAction & Action)
{
  USEDPARAM(File);
  USEDPARAM(Params);
  Action.Recursive();
  assert(FLAGCLEAR(Params, dfNoRecursive) || (File && File->GetIsSymLink()));
  ExecCommand(fsDeleteFile, 0, DelimitStr(FileName).c_str());
}
//---------------------------------------------------------------------------
void THTTPFileSystem::RenameFile(const std::wstring FileName,
  const std::wstring NewName)
{
  ExecCommand(fsRenameFile, 0, DelimitStr(FileName).c_str(), DelimitStr(NewName).c_str());
}
//---------------------------------------------------------------------------
void THTTPFileSystem::CopyFile(const std::wstring FileName,
  const std::wstring NewName)
{
  ExecCommand(fsCopyFile, 0, DelimitStr(FileName).c_str(), DelimitStr(NewName).c_str());
}
//---------------------------------------------------------------------------
void THTTPFileSystem::CreateDirectory(const std::wstring DirName)
{
  ExecCommand(fsCreateDirectory, 0, DelimitStr(DirName).c_str());
}
//---------------------------------------------------------------------------
void THTTPFileSystem::CreateLink(const std::wstring FileName,
  const std::wstring PointTo, bool Symbolic)
{
  ExecCommand(fsCreateLink, 0,
    Symbolic ? L"-s" : L"", DelimitStr(PointTo).c_str(), DelimitStr(FileName).c_str());
}
//---------------------------------------------------------------------------
void THTTPFileSystem::ChangeFileToken(const std::wstring & DelimitedName,
  const TRemoteToken & Token, TFSCommand Cmd, const std::wstring & RecursiveStr)
{
  std::wstring Str;
  if (Token.GetIDValid())
  {
    Str = IntToStr(Token.GetID());
  }
  else if (Token.GetNameValid())
  {
    Str = Token.GetName();
  }

  if (!Str.empty())
  {
    ExecCommand(Cmd, 0, RecursiveStr.c_str(), Str.c_str(), DelimitedName.c_str());
  }
}
//---------------------------------------------------------------------------
void THTTPFileSystem::ChangeFileProperties(const std::wstring FileName,
  const TRemoteFile * File, const TRemoteProperties * Properties,
  TChmodSessionAction & Action)
{
  assert(Properties);
  bool IsDirectory = File && File->GetIsDirectory();
  bool Recursive = Properties->Recursive && IsDirectory;
  std::wstring RecursiveStr = Recursive ? L"-R" : L"";

  std::wstring DelimitedName = DelimitStr(FileName);
  // change group before permissions as chgrp change permissions
  if (Properties->Valid.Contains(vpGroup))
  {
    ChangeFileToken(DelimitedName, Properties->Group, fsChangeGroup, RecursiveStr);
  }
  if (Properties->Valid.Contains(vpOwner))
  {
    ChangeFileToken(DelimitedName, Properties->Owner, fsChangeOwner, RecursiveStr);
  }
  if (Properties->Valid.Contains(vpRights))
  {
    TRights Rights = Properties->Rights;

    // if we don't set modes recursively, we may add X at once with other
    // options. Otherwise we have to add X after recusive command
    if (!Recursive && IsDirectory && Properties->AddXToDirectories)
      Rights.AddExecute();

    Action.Rights(Rights);
    if (Recursive)
    {
      Action.Recursive();
    }

    if ((Rights.GetNumberSet() | Rights.GetNumberUnset()) != TRights::rfNo)
    {
      ExecCommand(fsChangeMode,
        0, RecursiveStr.c_str(), Rights.GetSimplestStr().c_str(), DelimitedName.c_str());
    }

    // if file is directory and we do recursive mode settings with
    // add-x-to-directories option on, add those X
    if (Recursive && IsDirectory && Properties->AddXToDirectories)
    {
      Rights.AddExecute();
      ExecCommand(fsChangeMode,
        0, L"", Rights.GetSimplestStr().c_str(), DelimitedName.c_str());
    }
  }
  else
  {
    Action.Cancel();
  }
  assert(!Properties->Valid.Contains(vpLastAccess));
  assert(!Properties->Valid.Contains(vpModification));
}
//---------------------------------------------------------------------------
bool THTTPFileSystem::LoadFilesProperties(TStrings * /*FileList*/ )
{
  assert(false);
  return false;
}
//---------------------------------------------------------------------------
void THTTPFileSystem::CalculateFilesChecksum(const std::wstring & /*Alg*/,
  TStrings * /*FileList*/, TStrings * /*Checksums*/,
  calculatedchecksum_slot_type * /*OnCalculatedChecksum*/)
{
  assert(false);
}
//---------------------------------------------------------------------------
bool THTTPFileSystem::ConfirmOverwrite(std::wstring & FileName,
  TOverwriteMode & OverwriteMode, TFileOperationProgressType * OperationProgress,
  const TOverwriteFileParams * FileParams, int Params, bool AutoResume)
{
  bool Result;
  bool CanAutoResume = FLAGSET(Params, cpNoConfirmation) && AutoResume;
  // when resuming transfer after interrupted connection,
  // do nothing (dummy resume) when the files has the same size.
  // this is workaround for servers that strangely fails just after successful
  // upload.
  bool CanResume =
    (FileParams != NULL) &&
    (((FileParams->DestSize < FileParams->SourceSize)) ||
     ((FileParams->DestSize == FileParams->SourceSize) && CanAutoResume));

  int Answer;
  if (CanAutoResume && CanResume)
  {
    Answer = qaRetry;
  }
  else
  {
    // retry = "resume"
    // all = "yes to newer"
    // ignore = "rename"
    int Answers = qaYes | qaNo | qaCancel | qaYesToAll | qaNoToAll | qaAll | qaIgnore;
    if (CanResume)
    {
      Answers |= qaRetry;
    }
    TQueryButtonAlias Aliases[3];
    Aliases[0].Button = qaRetry;
    Aliases[0].Alias = LoadStr(RESUME_BUTTON);
    Aliases[1].Button = qaAll;
    Aliases[1].Alias = LoadStr(YES_TO_NEWER_BUTTON);
    Aliases[2].Button = qaIgnore;
    Aliases[2].Alias = LoadStr(RENAME_BUTTON);
    TQueryParams QueryParams(qpNeverAskAgainCheck);
    QueryParams.Aliases = Aliases;
    QueryParams.AliasesCount = LENOF(Aliases);
    SUSPEND_OPERATION
    (
      Answer = FTerminal->ConfirmFileOverwrite(FileName, FileParams,
        Answers, &QueryParams,
        OperationProgress->Side == osLocal ? osRemote : osLocal,
        Params, OperationProgress);
    )
  }

  Result = true;

  switch (Answer)
  {
    // resume
    case qaRetry:
      OverwriteMode = omResume;
      assert(FileParams != NULL);
      assert(CanResume);
      FFileTransferResumed = FileParams->DestSize;
      break;

    // rename
    case qaIgnore:
      if (FTerminal->PromptUser(FTerminal->GetSessionData(), pkFileName,
            LoadStr(RENAME_TITLE), L"", LoadStr(RENAME_PROMPT2), true, 0, FileName))
      {
        OverwriteMode = omOverwrite;
      }
      else
      {
        if (!OperationProgress->Cancel)
        {
          OperationProgress->Cancel = csCancel;
        }
        FFileTransferAbort = ftaCancel;
        Result = false;
      }
      break;

    case qaYes:
      OverwriteMode = omOverwrite;
      break;

    case qaCancel:
      if (!OperationProgress->Cancel)
      {
        OperationProgress->Cancel = csCancel;
      }
      FFileTransferAbort = ftaCancel;
      Result = false;
      break;

    case qaNo:
      FFileTransferAbort = ftaSkip;
      Result = false;
      break;

    default:
      assert(false);
      Result = false;
      break;
  }
  return Result;
}

//---------------------------------------------------------------------------
void THTTPFileSystem::CustomCommandOnFile(const std::wstring FileName,
    const TRemoteFile * File, std::wstring Command, int Params,
    const captureoutput_slot_type &OutputEvent)
{
  assert(File);
  bool Dir = File->GetIsDirectory() && !File->GetIsSymLink();
  if (Dir && (Params & ccRecursive))
  {
    TCustomCommandParams AParams(Command, Params, OutputEvent);
    // AParams.Command = Command;
    // AParams.Params = Params;
    // AParams.OutputEvent.connect(OutputEvent);
    FTerminal->ProcessDirectory(FileName, boost::bind(&TTerminal::CustomCommandOnFile, FTerminal, _1, _2, _3),
      &AParams);
  }

  if (!Dir || (Params & ccApplyToDirectories))
  {
    TCustomCommandData Data(FTerminal);
    std::wstring Cmd = TRemoteCustomCommand(
      Data, FTerminal->GetCurrentDirectory(), FileName, L"").
      Complete(Command, true);

    AnyCommand(Cmd, &OutputEvent);
  }
}
//---------------------------------------------------------------------------
void THTTPFileSystem::CaptureOutput(const std::wstring & AddedLine, bool StdError)
{
  int ReturnCode;
  std::wstring Line = AddedLine;
  // DEBUG_PRINTF(L"Line = %s", Line.c_str());
  if (StdError ||
      !RemoveLastLine(Line, ReturnCode) ||
      !Line.empty())
  {
    assert(!FOnCaptureOutput.empty());
    FOnCaptureOutput(Line, StdError);
  }
}
//---------------------------------------------------------------------------
void THTTPFileSystem::ResetCaches()
{
  delete FFileListCache;
  FFileListCache = NULL;
}
//---------------------------------------------------------------------------
void THTTPFileSystem::AnyCommand(const std::wstring Command,
  const captureoutput_slot_type *OutputEvent)
{
  // end-user has right to expect that client current directory is really
  // current directory for the server
  EnsureLocation();

  // assert(FSecureShell->GetOnCaptureOutput().empty());
  if (OutputEvent)
  {
    // FSecureShell->SetOnCaptureOutput(boost::bind(&THTTPFileSystem::CaptureOutput, this, _1, _2));
    FOnCaptureOutput.connect(*OutputEvent);
  }

  {
    BOOST_SCOPE_EXIT ( (&Self) )
    {
      Self->FOnCaptureOutput.disconnect_all_slots();
      // Self->FSecureShell->GetOnCaptureOutput().disconnect_all_slots();
    } BOOST_SCOPE_EXIT_END
    ExecCommand(fsAnyCommand, 0, Command.c_str(),
      ecDefault | ecIgnoreWarnings);
  }
}
//---------------------------------------------------------------------------
std::wstring THTTPFileSystem::FileUrl(const std::wstring FileName)
{
  return FTerminal->FileUrl(L"http", FileName);
}
//---------------------------------------------------------------------------
TStrings * THTTPFileSystem::GetFixedPaths()
{
  return NULL;
}
//---------------------------------------------------------------------------
void THTTPFileSystem::SpaceAvailable(const std::wstring Path,
  TSpaceAvailable & /*ASpaceAvailable*/)
{
  assert(false);
}
//---------------------------------------------------------------------------
// transfer protocol
//---------------------------------------------------------------------------
void THTTPFileSystem::SCPResponse(bool * GotLastLine)
{
  // Taken from scp.c response() and modified

  char Resp = 0;
  // FSecureShell->Receive(&Resp, 1);

  switch (Resp)
  {
    case 0:     /* ok */
      FTerminal->LogEvent(L"SCP remote side confirmation (0)");
      return;

    default:
    case 1:     /* error */
    case 2:     /* fatal error */
      // pscp adds 'Resp' to 'Msg', why?
      std::wstring MsgW = L""; // FSecureShell->ReceiveLine();
      std::string Msg = ::W2MB(MsgW.c_str());
      std::string Line = Resp + Msg;
      std::wstring LineW = ::MB2W(Line.c_str());
      if (IsLastLine(LineW))
      {
        if (GotLastLine != NULL)
        {
          *GotLastLine = true;
        }

        /* TODO 1 : Show stderror to user? */
        // FSecureShell->ClearStdError();

        try
        {
          ReadCommandOutput(coExpectNoOutput | coRaiseExcept | coOnlyReturnCode);
        }
        catch (...)
        {
          // when ReadCommandOutput() fails than remote SCP is terminated already
          if (GotLastLine != NULL)
          {
            *GotLastLine = true;
          }
          throw;
        }
      }
      else if (Resp == 1)
      {
        FTerminal->LogEvent(L"SCP remote side error (1):");
      }
      else
      {
        FTerminal->LogEvent(L"SCP remote side fatal error (2):");
      }

      if (Resp == 1)
      {
        // DEBUG_PRINTF(L"Msg = %s", MsgW.c_str());
        THROW_FILE_SKIPPED(MsgW, NULL);
      }
      else
      {
        THROW_SCP_ERROR(LineW, NULL);
      }
  }
}
//---------------------------------------------------------------------------
void THTTPFileSystem::CopyToRemote(TStrings * FilesToCopy,
  const std::wstring TargetDir, const TCopyParamType * CopyParam,
  int Params, TFileOperationProgressType * OperationProgress,
  TOnceDoneOperation & OnceDoneOperation)
{
  // scp.c: source(), toremote()
  assert(FilesToCopy && OperationProgress);

  Params &= ~(cpAppend | cpResume);
  std::wstring Options = L"";
  bool CheckExistence = UnixComparePaths(TargetDir, FTerminal->GetCurrentDirectory()) &&
    (FTerminal->FFiles != NULL) && FTerminal->FFiles->GetLoaded();
  bool CopyBatchStarted = false;
  bool Failed = true;
  bool GotLastLine = false;

  std::wstring TargetDirFull = UnixIncludeTrailingBackslash(TargetDir);

  // DEBUG_PRINTF(L"CopyParam->GetPreserveRights = %d", CopyParam->GetPreserveRights());
  if (CopyParam->GetPreserveRights()) Options = L"-p";
  if (FTerminal->GetSessionData()->GetScp1Compatibility()) Options += L" -1";

  // DEBUG_PRINTF(L"TargetDir = %s, Options = %s", TargetDir.c_str(), Options.c_str());
  SendCommand(FCommandSet->FullCommand(fsCopyToRemote,
    Options.c_str(), DelimitStr(UnixExcludeTrailingBackslash(TargetDir)).c_str()));
  SkipFirstLine();

  {
    BOOST_SCOPE_EXIT ( (&Self) (&GotLastLine) (&CopyBatchStarted)
        (&Failed) )
    {
        // Tell remote side, that we're done.
        if (Self->FTerminal->GetActive())
        {
          try
          {
            if (!GotLastLine)
            {
              if (CopyBatchStarted)
              {
                // What about case, remote side sends fatal error ???
                // (Not sure, if it causes remote side to terminate scp)
                // Self->FSecureShell->SendLine(L"E");
                // Self->SCPResponse();
              };
              /* TODO 1 : Show stderror to user? */
              // Self->FSecureShell->ClearStdError();

              Self->ReadCommandOutput(coExpectNoOutput | coWaitForLastLine | coOnlyReturnCode |
                (Failed ? 0 : coRaiseExcept));
            }
          }
          catch (const std::exception &E)
          {
            // Only log error message (it should always succeed, but
            // some pending error maybe in queque) }
            Self->FTerminal->GetLog()->AddException(&E);
          }
        }
    } BOOST_SCOPE_EXIT_END
    try
    {
      SCPResponse(&GotLastLine);
      // DEBUG_PRINTF(L"GotLastLine = %d", GotLastLine);

      // This can happen only if SCP command is not executed and return code is 0
      // It has never happened to me (return code is usually 127)
      if (GotLastLine)
      {
        throw std::exception("");
      }
    }
    catch (const std::exception & E)
    {
      // DEBUG_PRINTF(L"E.what = %s", ::MB2W(E.what()).c_str());
      if (GotLastLine && FTerminal->GetActive())
      {
        FTerminal->TerminalError(&E, LoadStr(SCP_INIT_ERROR));
      }
      else
      {
        throw;
      }
    }
    CopyBatchStarted = true;

    for (size_t IFile = 0; (IFile < FilesToCopy->GetCount()) &&
      !OperationProgress->Cancel; IFile++)
    {
      std::wstring FileName = FilesToCopy->GetString(IFile);
      bool CanProceed;

      std::wstring FileNameOnly =
        CopyParam->ChangeFileName(ExtractFileName(FileName, false), osLocal, true);
      // DEBUG_PRINTF(L"FileName = %s, CheckExistence = %d, FileNameOnly = %s", FileName.c_str(), CheckExistence, FileNameOnly.c_str());
      if (CheckExistence)
      {
        // previously there was assertion on FTerminal->FFiles->Loaded, but it
        // fails for scripting, if 'ls' is not issued before.
        // formally we should call CheckRemoteFile here but as checking is for
        // free here (almost) ...
        TRemoteFile * File = FTerminal->FFiles->FindFile(FileNameOnly);
        if (File != NULL)
        {
          int Answer;
          if (File->GetIsDirectory())
          {
            std::wstring Message = FMTLOAD(DIRECTORY_OVERWRITE, FileNameOnly.c_str());
            TQueryParams QueryParams(qpNeverAskAgainCheck);
            SUSPEND_OPERATION
            (
              Answer = FTerminal->ConfirmFileOverwrite(
                FileNameOnly /*not used*/, NULL,
                qaYes | qaNo | qaCancel | qaYesToAll | qaNoToAll,
                &QueryParams, osRemote, Params, OperationProgress, Message);
            );
          }
          else
          {
            __int64 MTime;
            TOverwriteFileParams FileParams;
            FTerminal->OpenLocalFile(FileName, GENERIC_READ,
              NULL, NULL, NULL, &MTime, NULL,
              &FileParams.SourceSize);
            // DEBUG_PRINTF(L"FileParams.SourceSize = %d", FileParams.SourceSize);
            FileParams.SourceTimestamp = UnixToDateTime(MTime,
              FTerminal->GetSessionData()->GetDSTMode());
            FileParams.DestSize = File->GetSize();
            FileParams.DestTimestamp = File->GetModification();

            TQueryButtonAlias Aliases[1];
            Aliases[0].Button = qaAll;
            Aliases[0].Alias = LoadStr(YES_TO_NEWER_BUTTON);
            TQueryParams QueryParams(qpNeverAskAgainCheck);
            QueryParams.Aliases = Aliases;
            QueryParams.AliasesCount = LENOF(Aliases);
            SUSPEND_OPERATION
            (
              Answer = FTerminal->ConfirmFileOverwrite(
                FileNameOnly, &FileParams,
                qaYes | qaNo | qaCancel | qaYesToAll | qaNoToAll | qaAll,
                &QueryParams, osRemote, Params, OperationProgress);
            );
          }

          switch (Answer)
          {
            case qaYes:
              CanProceed = true;
              break;

            case qaCancel:
              if (!OperationProgress->Cancel) OperationProgress->Cancel = csCancel;
            case qaNo:
              CanProceed = false;
              break;

            default:
              assert(false);
              break;
          }
        }
        else
        {
          CanProceed = true;
        }
      }
      else
      {
        CanProceed = true;
      }

      if (CanProceed)
      {
        if (FTerminal->GetSessionData()->GetCacheDirectories())
        {
          FTerminal->DirectoryModified(TargetDir, false);

          if (::DirectoryExists(::ExtractFilePath(FileName)))
          {
            FTerminal->DirectoryModified(UnixIncludeTrailingBackslash(TargetDir) +
              FileNameOnly, true);
          }
        }

        try
        {
          // DEBUG_PRINTF(L"FileName = %s, TargetDirFull = %s", FileName.c_str(), TargetDirFull.c_str());
          SCPSource(FileName, TargetDirFull,
            CopyParam, Params, OperationProgress, 0);
          OperationProgress->Finish(FileName, true, OnceDoneOperation);
        }
        catch (const EScpFileSkipped &E)
        {
          TQueryParams Params(qpAllowContinueOnError);
          DEBUG_PRINTF(L"before FTerminal->QueryUserException");
          SUSPEND_OPERATION (
            if (FTerminal->QueryUserException(FMTLOAD(COPY_ERROR, FileName.c_str()),
              &E,
              qaOK | qaAbort, &Params, qtError) == qaAbort)
            {
              OperationProgress->Cancel = csCancel;
            }
            OperationProgress->Finish(FileName, false, OnceDoneOperation);
            if (!FTerminal->HandleException(&E)) throw;
          );
        }
        catch (const EScpSkipFile &E)
        {
          DEBUG_PRINTF(L"before FTerminal->HandleException");
          OperationProgress->Finish(FileName, false, OnceDoneOperation);
          // If ESkipFile occurs, just log it and continue with next file
          SUSPEND_OPERATION (
            if (!FTerminal->HandleException(&E)) throw;
          );
        }
        catch (...)
        {
          OperationProgress->Finish(FileName, false, OnceDoneOperation);
          throw;
        }
      }
    }
    Failed = false;
  }
}

//---------------------------------------------------------------------------
void THTTPFileSystem::SCPSource(const std::wstring FileName,
  const std::wstring TargetDir, const TCopyParamType * CopyParam, int Params,
  TFileOperationProgressType * OperationProgress, int Level)
{
  std::wstring DestFileName = CopyParam->ChangeFileName(
    ExtractFileName(FileName, false), osLocal, Level == 0);
  // DEBUG_PRINTF(L"DestFileName = %s", DestFileName.c_str());
  FTerminal->LogEvent(FORMAT(L"File: \"%s\"", FileName.c_str()));

  OperationProgress->SetFile(FileName, false);

  if (!FTerminal->AllowLocalFileTransfer(FileName, CopyParam))
  {
    FTerminal->LogEvent(FORMAT(L"File \"%s\" excluded from transfer", FileName.c_str()));
    THROW_SKIP_FILE_NULL;
  }

  HANDLE File;
  int Attrs;
  __int64 MTime, ATime;
  __int64 Size;

  FTerminal->OpenLocalFile(FileName, GENERIC_READ,
    &Attrs, &File, NULL, &MTime, &ATime, &Size);

  bool Dir = FLAGSET(Attrs, faDirectory);
  TSafeHandleStream * Stream = new TSafeHandleStream(File);
  {
    BOOST_SCOPE_EXIT ( (&File) (&Stream) )
    {
      if (File != NULL)
      {
        ::CloseHandle(File);
      }
      delete Stream;
    } BOOST_SCOPE_EXIT_END
    OperationProgress->SetFileInProgress();

    if (Dir)
    {
      SCPDirectorySource(FileName, TargetDir, CopyParam, Params, OperationProgress, Level);
    }
    else
    {
      std::wstring AbsoluteFileName = FTerminal->AbsolutePath(DestFileName, false); // TargetDir +
      // DEBUG_PRINTF(L"AbsoluteFileName = %s", AbsoluteFileName.c_str());
      assert(File);

      // File is regular file (not directory)
      FTerminal->LogEvent(FORMAT(L"Copying \"%s\" to remote directory started.", FileName.c_str()));

      OperationProgress->SetLocalSize(Size);

      // Suppose same data size to transfer as to read
      // (not true with ASCII transfer)
      OperationProgress->SetTransferSize(OperationProgress->LocalSize);
      OperationProgress->TransferingFile = false;

      // Will we use ASCII of BINARY file tranfer?
      TFileMasks::TParams MaskParams;
      MaskParams.Size = Size;
      OperationProgress->SetAsciiTransfer(
        CopyParam->UseAsciiTransfer(FileName, osLocal, MaskParams));
      FTerminal->LogEvent(
        std::wstring((OperationProgress->AsciiTransfer ? L"Ascii" : L"Binary")) +
          L" transfer mode selected.");

      TUploadSessionAction Action(FTerminal->GetLog());
      Action.FileName(ExpandUNCFileName(FileName));
      Action.Destination(AbsoluteFileName);

      TRights Rights = CopyParam->RemoteFileRights(Attrs);

      try
      {
        // During ASCII transfer we will load whole file to this buffer
        // than convert EOL and send it at once, because before converting EOL
        // we can't know its size
        TFileBuffer AsciiBuf;
        bool ConvertToken = false;
        do
        {
          // Buffer for one block of data
          TFileBuffer BlockBuf;

          // This is crucial, if it fails during file transfer, it's fatal error
          FILE_OPERATION_LOOP_EX (!OperationProgress->TransferingFile,
              FMTLOAD(READ_ERROR, FileName.c_str()),
            BlockBuf.LoadStream(Stream, OperationProgress->LocalBlockSize(), true);
          );

          OperationProgress->AddLocallyUsed(BlockBuf.GetSize());

          // We do ASCII transfer: convert EOL of current block
          // (we don't convert whole buffer, cause it would produce
          // huge memory-transfers while inserting/deleting EOL characters)
          // Than we add current block to file buffer
          if (OperationProgress->AsciiTransfer)
          {
            BlockBuf.Convert(FTerminal->GetConfiguration()->GetLocalEOLType(),
              FTerminal->GetSessionData()->GetEOLType(), cpRemoveCtrlZ | cpRemoveBOM, ConvertToken);
            BlockBuf.GetMemory()->Seek(0, soFromBeginning);
            AsciiBuf.ReadStream(BlockBuf.GetMemory(), BlockBuf.GetSize(), true);
            // We don't need it any more
            BlockBuf.GetMemory()->Clear();
            // Calculate total size to sent (assume that ratio between
            // size of source and size of EOL-transformed data would remain same)
            // First check if file contains anything (div by zero!)
            if (OperationProgress->LocallyUsed)
            {
              __int64 X = OperationProgress->LocalSize;
              X *= AsciiBuf.GetSize();
              X /= OperationProgress->LocallyUsed;
              OperationProgress->ChangeTransferSize(X);
            }
              else
            {
              OperationProgress->ChangeTransferSize(0);
            }
          }

          // We send file information on first pass during BINARY transfer
          // and on last pass during ASCII transfer
          // BINARY: We succeeded reading first buffer from file, hopefully
          // we will be able to read whole, so we send file info to remote side
          // This is done, because when reading fails we can't interrupt sending
          // (don't know how to tell other side that it failed)
          if (!OperationProgress->TransferingFile &&
              (!OperationProgress->AsciiTransfer || OperationProgress->IsLocallyDone()))
          {
            std::wstring Buf;

            if (CopyParam->GetPreserveTime())
            {
              Buf.resize(40, 0);
              // Send last file access and modification time
              swprintf_s((wchar_t *)Buf.c_str(), Buf.size(), L"T%lu 0 %lu 0", static_cast<unsigned long>(MTime),
                static_cast<unsigned long>(ATime));
              // FSecureShell->SendLine(Buf.c_str());
              SCPResponse();
            }

            // Send file modes (rights), filesize and file name
            Buf.clear();
            Buf.resize(MAX_PATH * 2, 0);
            // TODO: use boost::format
            swprintf_s((wchar_t *)Buf.c_str(), Buf.size(), L"C%s %ld %s",
              Rights.GetOctal().c_str(),
              (int)(OperationProgress->AsciiTransfer ? AsciiBuf.GetSize() :
                OperationProgress->LocalSize),
              DestFileName.c_str());
            // FSecureShell->SendLine(Buf.c_str());
            SCPResponse();
            // Indicate we started transfering file, we need to finish it
            // If not, it's fatal error
            OperationProgress->TransferingFile = true;

            // If we're doing ASCII transfer, this is last pass
            // so we send whole file
            /* TODO : We can't send file above 32bit size in ASCII mode! */
            if (OperationProgress->AsciiTransfer)
            {
              FTerminal->LogEvent(FORMAT(L"Sending ASCII data (%u bytes)",
                AsciiBuf.GetSize()));
              // Should be equal, just in case it's rounded (see above)
              OperationProgress->ChangeTransferSize(AsciiBuf.GetSize());
              while (!OperationProgress->IsTransferDone())
              {
                unsigned long BlockSize = OperationProgress->TransferBlockSize();
                // FSecureShell->Send(
                  // AsciiBuf.GetData() + (unsigned int)OperationProgress->TransferedSize,
                  // BlockSize);
                OperationProgress->AddTransfered(BlockSize);
                if (OperationProgress->Cancel == csCancelTransfer)
                {
                  throw ExtException(FMTLOAD(USER_TERMINATED));
                }
              }
            }
          }

          // At end of BINARY transfer pass, send current block
          if (!OperationProgress->AsciiTransfer)
          {
            if (!OperationProgress->TransferedSize)
            {
              FTerminal->LogEvent(FORMAT(L"Sending BINARY data (first block, %u bytes)",
                BlockBuf.GetSize()));
            }
            else if (FTerminal->GetConfiguration()->GetActualLogProtocol() >= 1)
            {
              FTerminal->LogEvent(FORMAT(L"Sending BINARY data (%u bytes)",
                BlockBuf.GetSize()));
            }
            // FIXME FSecureShell->Send(BlockBuf.GetData(), BlockBuf.GetSize());
            OperationProgress->AddTransfered(BlockBuf.GetSize());
          }

          if ((OperationProgress->Cancel == csCancelTransfer) ||
              (OperationProgress->Cancel == csCancel && !OperationProgress->TransferingFile))
          {
              throw ExtException(FMTLOAD(USER_TERMINATED));
          }
        }
        while (!OperationProgress->IsLocallyDone() || !OperationProgress->IsTransferDone());

        // FSecureShell->SendNull();
        try
        {
          SCPResponse();
          // If one of two following exceptions occurs, it means, that remote
          // side already know, that file transfer finished, even if it failed
          // so we don't have to throw EFatal
        }
        catch (const EScp &E)
        {
          // SCP protocol fatal error
          OperationProgress->TransferingFile = false;
          throw;
        }
        catch (const EScpFileSkipped &E)
        {
          // SCP protocol non-fatal error
          OperationProgress->TransferingFile = false;
          throw;
        }

        // We succeded transfering file, from now we can handle exceptions
        // normally -> no fatal error
        OperationProgress->TransferingFile = false;
      }
      catch (const std::exception &E)
      {
        // EScpFileSkipped is derived from EScpSkipFile,
        // but is does not indicate file skipped by user here
        if (dynamic_cast<const EScpFileSkipped *>(&E) != NULL)
        {
          Action.Rollback(&E);
        }
        else
        {
          FTerminal->RollbackAction(Action, OperationProgress, &E);
        }

        // Every exception during file transfer is fatal
        if (OperationProgress->TransferingFile)
        {
          FTerminal->FatalError(&E, FMTLOAD(COPY_FATAL, FileName.c_str()));
        }
        else
        {
          throw;
        }
      }

      // With SCP we are not able to distinguish reason for failure
      // (upload itself, touch or chmod).
      // So we always report error with upload action and
      // log touch and chmod actions only if upload succeeds.
      if (CopyParam->GetPreserveTime())
      {
        TTouchSessionAction(FTerminal->GetLog(), AbsoluteFileName,
          UnixToDateTime(MTime, FTerminal->GetSessionData()->GetDSTMode()));
      }
      if (CopyParam->GetPreserveRights())
      {
        TChmodSessionAction(FTerminal->GetLog(), AbsoluteFileName,
          Rights);
      }
    }
  }

  /* TODO : Delete also read-only files. */
  if (FLAGSET(Params, cpDelete))
  {
    if (!Dir)
    {
      FILE_OPERATION_LOOP (FMTLOAD(DELETE_LOCAL_FILE_ERROR, FileName.c_str()),
        THROWOSIFFALSE(::DeleteFile(FileName));
      )
    }
  }
  else if (CopyParam->GetClearArchive() && FLAGSET(Attrs, faArchive))
  {
    FILE_OPERATION_LOOP (FMTLOAD(CANT_SET_ATTRS, FileName.c_str()),
      THROWOSIFFALSE(FileSetAttr(FileName, Attrs & ~faArchive) == 0);
    )
  }

  FTerminal->LogEvent(FORMAT(L"Copying \"%s\" to remote directory finished.", FileName.c_str()));
}
//---------------------------------------------------------------------------
void THTTPFileSystem::SCPDirectorySource(const std::wstring DirectoryName,
  const std::wstring TargetDir, const TCopyParamType * CopyParam, int Params,
  TFileOperationProgressType * OperationProgress, int Level)
{
  int Attrs;

  FTerminal->LogEvent(FORMAT(L"Entering directory \"%s\".", DirectoryName.c_str()));

  OperationProgress->SetFile(DirectoryName);
  std::wstring DestFileName = CopyParam->ChangeFileName(
    ExtractFileName(DirectoryName, false), osLocal, Level == 0);

  // Get directory attributes
  FILE_OPERATION_LOOP (FMTLOAD(CANT_GET_ATTRS, DirectoryName.c_str()),
    Attrs = FileGetAttr(DirectoryName);
    if (Attrs == -1) RaiseLastOSError();
  )

  std::wstring TargetDirFull = UnixIncludeTrailingBackslash(TargetDir + DestFileName);

  std::wstring Buf;

  /* TODO 1: maybe send filetime */

  // Send directory modes (rights), filesize and file name
  Buf = FORMAT(L"D%s 0 %s",
    CopyParam->RemoteFileRights(Attrs).GetOctal().c_str(), DestFileName.c_str());
  // FSecureShell->SendLine(Buf);
  SCPResponse();

  {
    BOOST_SCOPE_EXIT ( (&Self) (&DirectoryName) )
    {
      if (Self->FTerminal->GetActive())
      {
        // Tell remote side, that we're done.
        Self->FTerminal->LogEvent(FORMAT(L"Leaving directory \"%s\".", DirectoryName.c_str()));
        // Self->FSecureShell->SendLine(L"E");
        // Self->SCPResponse();
      }
    } BOOST_SCOPE_EXIT_END
    int FindAttrs = faReadOnly | faHidden | faSysFile | faDirectory | faArchive;
    WIN32_FIND_DATA SearchRec;
    memset(&SearchRec, 0, sizeof(SearchRec));
    HANDLE findHandle = 0;
    bool FindOK = false;
    FILE_OPERATION_LOOP (FMTLOAD(LIST_DIR_ERROR, DirectoryName.c_str()),
    std::wstring path = IncludeTrailingBackslash(DirectoryName) + L"*.*";
    findHandle = FindFirstFile(path.c_str(),
        &SearchRec);
      FindOK = (findHandle != 0) && (SearchRec.dwFileAttributes & FindAttrs);
    );

    {
      BOOST_SCOPE_EXIT ( (&findHandle) )
      {
        FindClose(findHandle);
      } BOOST_SCOPE_EXIT_END
      while (FindOK && !OperationProgress->Cancel)
      {
        std::wstring FileName = IncludeTrailingBackslash(DirectoryName) + SearchRec.cFileName;
        try
        {
          if ((wcscmp(SearchRec.cFileName, L".") != 0) && (wcscmp(SearchRec.cFileName, L"..") != 0))
          {
            SCPSource(FileName, TargetDirFull, CopyParam, Params, OperationProgress, Level + 1);
          }
        }
        // Previously we catched EScpSkipFile, making error being displayed
        // even when file was excluded by mask. Now the EScpSkipFile is special
        // case without error message.
        catch (const EScpFileSkipped &E)
        {
          TQueryParams Params(qpAllowContinueOnError);
          DEBUG_PRINTF(L"before FTerminal->HandleException");
          SUSPEND_OPERATION (
            if (FTerminal->QueryUserException(FMTLOAD(COPY_ERROR, FileName.c_str()),
                &E,
                qaOK | qaAbort, &Params, qtError) == qaAbort)
            {
              OperationProgress->Cancel = csCancel;
            }
            if (!FTerminal->HandleException(&E)) throw;
          );
        }
        catch (const EScpSkipFile &E)
        {
          // If ESkipFile occurs, just log it and continue with next file
          DEBUG_PRINTF(L"before FTerminal->HandleException");
          SUSPEND_OPERATION (
            if (!FTerminal->HandleException(&E)) throw;
          );
        }
        FindOK = false;
        FILE_OPERATION_LOOP (FMTLOAD(LIST_DIR_ERROR, DirectoryName.c_str()),
          FindOK = (FindNextFile(findHandle, &SearchRec) != 0) && (SearchRec.dwFileAttributes & FindAttrs);
        );
      };
    }

    /* TODO : Delete also read-only directories. */
    /* TODO : Show error message on failure. */
    if (!OperationProgress->Cancel)
    {
      if (FLAGSET(Params, cpDelete))
      {
        RemoveDir(DirectoryName);
      }
      else if (CopyParam->GetClearArchive() && FLAGSET(Attrs, faArchive))
      {
        FILE_OPERATION_LOOP (FMTLOAD(CANT_SET_ATTRS, DirectoryName.c_str()),
          THROWOSIFFALSE(FileSetAttr(DirectoryName, Attrs & ~faArchive) == 0);
        )
      }
    }
  }
}
//---------------------------------------------------------------------------
void THTTPFileSystem::CopyToLocal(TStrings * FilesToCopy,
  const std::wstring TargetDir, const TCopyParamType * CopyParam,
  int Params, TFileOperationProgressType * OperationProgress,
  TOnceDoneOperation & OnceDoneOperation)
{
  bool CloseSCP = false;
  Params &= ~(cpAppend | cpResume);
  std::wstring Options = L"";
  if (CopyParam->GetPreserveRights() || CopyParam->GetPreserveTime()) Options = L"-p";
  if (FTerminal->GetSessionData()->GetScp1Compatibility()) Options += L" -1";

  FTerminal->LogEvent(FORMAT(L"Copying %d files/directories to local directory \"%s\"",
      FilesToCopy->GetCount(), TargetDir.c_str()));
  FTerminal->LogEvent(CopyParam->GetLogStr());

  {
    BOOST_SCOPE_EXIT ( (&Self) (&CloseSCP) (&OperationProgress) )
    {
      // In case that copying doesn't cause fatal error (ie. connection is
      // still active) but wasn't succesful (exception or user termination)
      // we need to ensure, that SCP on remote side is closed
      if (Self->FTerminal->GetActive() && (CloseSCP ||
          (OperationProgress->Cancel == csCancel) ||
          (OperationProgress->Cancel == csCancelTransfer)))
      {
        bool LastLineRead;

        // If we get LastLine, it means that remote side 'scp' is already
        // terminated, so we need not to terminate it. There is also
        // possibility that remote side waits for confirmation, so it will hang.
        // This should not happen (hope)
        std::wstring Line = L""; // Self->FSecureShell->ReceiveLine();
        LastLineRead = Self->IsLastLine(Line);
        if (!LastLineRead)
        {
          Self->SCPSendError((OperationProgress->Cancel ? L"Terminated by user." : L"std::exception"), true);
        }
        // Just in case, remote side already sent some more data (it's probable)
        // but we don't want to raise exception (user asked to terminate, it's not error)
        int ECParams = coOnlyReturnCode;
        if (!LastLineRead) ECParams |= coWaitForLastLine;
        Self->ReadCommandOutput(ECParams);
      }
    } BOOST_SCOPE_EXIT_END
    for (size_t IFile = 0; (IFile < FilesToCopy->GetCount()) &&
      !OperationProgress->Cancel; IFile++)
    {
      std::wstring FileName = FilesToCopy->GetString(IFile);
      TRemoteFile * File = (TRemoteFile *)FilesToCopy->GetObject(IFile);
      assert(File);

      try
      {
        bool Success = true; // Have to be set to true (see ::SCPSink)
        SendCommand(FCommandSet->FullCommand(fsCopyToLocal,
          Options.c_str(), DelimitStr(FileName).c_str()));
        SkipFirstLine();

        // Filename is used for error messaging and excluding files only
        // Send in full path to allow path-based excluding
        std::wstring FullFileName = UnixExcludeTrailingBackslash(File->GetFullFileName());
        // DEBUG_PRINTF(L"FileName = '%s', FullFileName = '%s'", FileName.c_str(), FullFileName.c_str());
        SCPSink(TargetDir, FullFileName, UnixExtractFilePath(FullFileName),
          CopyParam, Success, OperationProgress, Params, 0);
        // operation succeded (no exception), so it's ok that
        // remote side closed SCP, but we continue with next file
        if (OperationProgress->Cancel == csRemoteAbort)
        {
          OperationProgress->Cancel = csContinue;
        }

        // Move operation -> delete file/directory afterwards
        // but only if copying succeded
        if ((Params & cpDelete) && Success && !OperationProgress->Cancel)
        {
          try
          {
            FTerminal->SetExceptionOnFail(true);
            {
              BOOST_SCOPE_EXIT ( (&Self) )
              {
                Self->FTerminal->SetExceptionOnFail(false);
              } BOOST_SCOPE_EXIT_END
              FILE_OPERATION_LOOP(FMTLOAD(DELETE_FILE_ERROR, FileName.c_str()),
                // pass full file name in FileName, in case we are not moving
                // from current directory
                FTerminal->DeleteFile(FileName, File)
              );
            }
          }
          catch (const EFatal &E)
          {
            throw;
          }
          catch (...)
          {
            // If user selects skip (or abort), nothing special actualy occurs
            // we just run DoFinished with Success = false, so file won't
            // be deselected in panel (depends on assigned event handler)

            // On csCancel we would later try to close remote SCP, but it
            // is closed already
            if (OperationProgress->Cancel == csCancel)
            {
              OperationProgress->Cancel = csRemoteAbort;
            }
            Success = false;
          }
        }

        OperationProgress->Finish(FileName,
          (!OperationProgress->Cancel && Success), OnceDoneOperation);
      }
      catch (...)
      {
        OperationProgress->Finish(FileName, false, OnceDoneOperation);
        CloseSCP = (OperationProgress->Cancel != csRemoteAbort);
        throw;
      }
    }
  }
}
//---------------------------------------------------------------------------
void THTTPFileSystem::SCPError(const std::wstring Message, bool Fatal)
{
  SCPSendError(Message, Fatal);
  DEBUG_PRINTF(L"Message = %s", Message.c_str());
  THROW_FILE_SKIPPED(Message, NULL);
}
//---------------------------------------------------------------------------
void THTTPFileSystem::SCPSendError(const std::wstring Message, bool Fatal)
{
  char ErrorLevel = (char)(Fatal ? 2 : 1);
  FTerminal->LogEvent(FORMAT(L"Sending SCP error (%d) to remote side:",
    ((int)ErrorLevel)));
  // FSecureShell->Send(&ErrorLevel, 1);
  // We don't send exact error message, because some unspecified
  // characters can terminate remote scp
  // FSecureShell->SendLine(L"scp: error");
}
//---------------------------------------------------------------------------
void THTTPFileSystem::SCPSink(const std::wstring TargetDir,
  const std::wstring FileName, const std::wstring SourceDir,
  const TCopyParamType * CopyParam, bool & Success,
  TFileOperationProgressType * OperationProgress, int Params,
  int Level)
{
  struct
  {
    int SetTime;
    FILETIME AcTime;
    FILETIME WrTime;
    TRights RemoteRights;
    int Attrs;
    bool Exists;
  } FileData;
  TDateTime SourceTimestamp;

  bool SkipConfirmed = false;
  bool Initialized = (Level > 0);

  FileData.SetTime = 0;

  // FSecureShell->SendNull();

  while (!OperationProgress->Cancel)
  {
    // See (switch ... case 'T':)
    if (FileData.SetTime) FileData.SetTime--;

    // In case of error occured before control record arrived.
    // We can finally use full path here, as we get current path in FileName param
    // (we used to set the file into OperationProgress->GetFileName(), but it collided
    // with progress outputing, particularly for scripting)
    std::wstring AbsoluteFileName = FileName;

    try
    {
      // Receive control record
      std::wstring Line = L""; // FSecureShell->ReceiveLine();

      if (Line.size() == 0) FTerminal->FatalError(NULL, LoadStr(SCP_EMPTY_LINE));

      if (IsLastLine(Line))
      {
        // Remote side finished copying, so remote SCP was closed
        // and we don't need to terminate it manualy, see CopyToLocal()
        OperationProgress->Cancel = csRemoteAbort;
        /* TODO 1 : Show stderror to user? */
        // FSecureShell->ClearStdError();
        try
        {
          // coIgnoreWarnings should allow batch transfer to continue when
          // download of one the files failes (user denies overwritting
          // of target local file, no read permissions...)
          ReadCommandOutput(coExpectNoOutput | coRaiseExcept |
            coOnlyReturnCode | coIgnoreWarnings);
          if (!Initialized)
          {
            throw std::exception("");
          }
        }
        catch (const std::exception & E)
        {
          if (!Initialized && FTerminal->GetActive())
          {
            FTerminal->TerminalError(&E, LoadStr(SCP_INIT_ERROR));
          }
          else
          {
            throw;
          }
        }
        return;
      }
      else
      {
        Initialized = true;

        // First characted distinguish type of control record
        char Ctrl = Line[0];
        Line.erase(0, 1);
        // DEBUG_PRINTF(L"Line ='%s', Ctrl = '%c'", Line.c_str(), Ctrl);

        switch (Ctrl)
        {
          case 1:
            // Error (already logged by ReceiveLine())
            THROW_FILE_SKIPPED(FMTLOAD(REMOTE_ERROR, Line.c_str()), NULL);

          case 2:
            // Fatal error, terminate copying
            FTerminal->TerminalError(Line);
            return; // Unreachable

          case 'E': // Exit
            // FSecureShell->SendNull();
            return;

          case 'T':
            unsigned long MTime, ATime;
            if (swscanf(Line.c_str(), L"%ld %*d %ld %*d",  &MTime, &ATime) == 2)
            {
              FileData.AcTime = DateTimeToFileTime(UnixToDateTime(ATime,
                FTerminal->GetSessionData()->GetDSTMode()), FTerminal->GetSessionData()->GetDSTMode());
              FileData.WrTime = DateTimeToFileTime(UnixToDateTime(MTime,
                FTerminal->GetSessionData()->GetDSTMode()), FTerminal->GetSessionData()->GetDSTMode());
              SourceTimestamp = UnixToDateTime(MTime,
                FTerminal->GetSessionData()->GetDSTMode());
              // FSecureShell->SendNull();
              // File time is only valid until next pass
              FileData.SetTime = 2;
              continue;
            }
              else
            {
              SCPError(LoadStr(SCP_ILLEGAL_TIME_FORMAT), false);
            }

          case 'C':
          case 'D':
            break; // continue pass switch{}

          default:
            FTerminal->FatalError(NULL, FMTLOAD(SCP_INVALID_CONTROL_RECORD, Ctrl, Line.c_str()));
        }

        TFileMasks::TParams MaskParams;

        // We reach this point only if control record was 'C' or 'D'
        try
        {
          FileData.RemoteRights.SetOctal(CutToChar(Line, ' ', true));
          // do not trim leading spaces of the filename
          __int64 TSize = StrToInt64(::TrimRight(CutToChar(Line, ' ', false)));
          // DEBUG_PRINTF(L"TSize = %u", TSize);
          MaskParams.Size = TSize;
          // Security fix: ensure the file ends up where we asked for it.
          // (accept only filename, not path)
          std::wstring OnlyFileName = UnixExtractFileName(Line);
          // DEBUG_PRINTF(L"Line = '%s', OnlyFileName = '%s'", Line.c_str(), OnlyFileName.c_str());
          if (Line != OnlyFileName)
          {
            FTerminal->LogEvent(FORMAT(L"Warning: Remote host set a compound pathname '%s'", (Line)));
          }

          OperationProgress->SetFile(OnlyFileName);
          AbsoluteFileName = SourceDir + OnlyFileName;
          // DEBUG_PRINTF(L"AbsoluteFileName = '%s', OnlyFileName = '%s'", AbsoluteFileName.c_str(), OnlyFileName.c_str());
          OperationProgress->SetTransferSize(TSize);
        }
        catch (const std::exception &E)
        {
          SUSPEND_OPERATION (
            FTerminal->GetLog()->AddException(&E);
          );
          SCPError(LoadStr(SCP_ILLEGAL_FILE_DESCRIPTOR), false);
        }

        // last possibility to cancel transfer before it starts
        if (OperationProgress->Cancel)
        {
          THROW_SKIP_FILE(LoadStr(USER_TERMINATED), NULL);
        }

        bool Dir = (Ctrl == 'D');
        std::wstring SourceFullName = SourceDir + OperationProgress->FileName;
        if (!CopyParam->AllowTransfer(SourceFullName, osRemote, Dir, MaskParams))
        {
          FTerminal->LogEvent(FORMAT(L"File \"%s\" excluded from transfer",
            AbsoluteFileName.c_str()));
          SkipConfirmed = true;
          SCPError(L"", false);
        }

        // DEBUG_PRINTF(L"TargetDir = %s", TargetDir.c_str());
        // DEBUG_PRINTF(L"OperationProgress->FileName = %s", OperationProgress->FileName.c_str());
        std::wstring DestFileName =
          IncludeTrailingBackslash(TargetDir) +
          CopyParam->ChangeFileName(OperationProgress->FileName, osRemote,
            Level == 0);

        // DEBUG_PRINTF(L"DestFileName = %s", DestFileName.c_str());
        FileData.Attrs = FileGetAttr(DestFileName);
        // If getting attrs failes, we suppose, that file/folder doesn't exists
        FileData.Exists = (FileData.Attrs != -1);
        if (Dir)
        {
          if (FileData.Exists && !(FileData.Attrs & faDirectory))
          {
            SCPError(FMTLOAD(NOT_DIRECTORY_ERROR, DestFileName.c_str()),
                false);
          }

          if (!FileData.Exists)
          {
            FILE_OPERATION_LOOP (FMTLOAD(CREATE_DIR_ERROR, DestFileName.c_str()),
              if (!ForceDirectories(DestFileName)) RaiseLastOSError();
            );
            /* SCP: can we set the timestamp for directories ? */
          }
          std::wstring FullFileName = SourceDir + OperationProgress->FileName;
          SCPSink(DestFileName, FullFileName, UnixIncludeTrailingBackslash(FullFileName),
            CopyParam, Success, OperationProgress, Params, Level + 1);
          continue;
        }
        else if (Ctrl == 'C')
        {
          TDownloadSessionAction Action(FTerminal->GetLog());
          Action.FileName(AbsoluteFileName);

          try
          {
            HANDLE File = 0;
            TStream * FileStream = NULL;

            /* TODO 1 : Turn off read-only attr */

            {
              BOOST_SCOPE_EXIT ( (&File) (&FileStream) )
              {
                if (File) CloseHandle(File);
                if (FileStream) delete FileStream;
              } BOOST_SCOPE_EXIT_END
              try
              {
                if (FileExists(DestFileName))
                {
                  __int64 MTime;
                  TOverwriteFileParams FileParams;
                  FileParams.SourceSize = OperationProgress->TransferSize;
                  FileParams.SourceTimestamp = SourceTimestamp;
                  FTerminal->OpenLocalFile(DestFileName, GENERIC_READ,
                    NULL, NULL, NULL, &MTime, NULL,
                    &FileParams.DestSize);
                  FileParams.DestTimestamp = UnixToDateTime(MTime,
                    FTerminal->GetSessionData()->GetDSTMode());

                  TQueryButtonAlias Aliases[1];
                  Aliases[0].Button = qaAll;
                  Aliases[0].Alias = LoadStr(YES_TO_NEWER_BUTTON);
                  TQueryParams QueryParams(qpNeverAskAgainCheck);
                  QueryParams.Aliases = Aliases;
                  QueryParams.AliasesCount = LENOF(Aliases);

                  int Answer;
                  SUSPEND_OPERATION (
                    Answer = FTerminal->ConfirmFileOverwrite(
                      OperationProgress->FileName, &FileParams,
                      qaYes | qaNo | qaCancel | qaYesToAll | qaNoToAll | qaAll,
                      &QueryParams, osLocal, Params, OperationProgress);
                  );

                  switch (Answer)
                  {
                    case qaCancel:
                      OperationProgress->Cancel = csCancel; // continue on next case
                    case qaNo:
                      SkipConfirmed = true;
                      EXCEPTION;
                  }
                }

                Action.Destination(DestFileName);

                if (!FTerminal->CreateLocalFile(DestFileName, OperationProgress,
                       &File, FLAGSET(Params, cpNoConfirmation)))
                {
                  SkipConfirmed = true;
                  EXCEPTION;
                }

                FileStream = new TSafeHandleStream(File);
              }
              catch (const std::exception &E)
              {
                // In this step we can still cancel transfer, so we do it
                SCPError(::MB2W(E.what()), false);
                throw;
              }

              // We succeded, so we confirm transfer to remote side
              // FSecureShell->SendNull();
              // From now we need to finish file transfer, if not it's fatal error
              OperationProgress->TransferingFile = true;

              // Suppose same data size to transfer as to write
              // (not true with ASCII transfer)
              OperationProgress->SetLocalSize(OperationProgress->TransferSize);

              // Will we use ASCII of BINARY file tranfer?
              OperationProgress->SetAsciiTransfer(
                CopyParam->UseAsciiTransfer(SourceFullName, osRemote, MaskParams));
              FTerminal->LogEvent(std::wstring((OperationProgress->AsciiTransfer ? L"Ascii" : L"Binary")) +
                L" transfer mode selected.");

              try
              {
                // Buffer for one block of data
                TFileBuffer BlockBuf;
                bool ConvertToken = false;

                do
                {
                  BlockBuf.SetSize(OperationProgress->TransferBlockSize());
                  BlockBuf.SetPosition(0);

                  // FSecureShell->Receive(BlockBuf.GetData(), BlockBuf.GetSize());
                  OperationProgress->AddTransfered(BlockBuf.GetSize());

                  if (OperationProgress->AsciiTransfer)
                  {
                    unsigned int PrevBlockSize = BlockBuf.GetSize();
                    BlockBuf.Convert(FTerminal->GetSessionData()->GetEOLType(),
                      FTerminal->GetConfiguration()->GetLocalEOLType(), 0, ConvertToken);
                    OperationProgress->SetLocalSize(
                      OperationProgress->LocalSize - PrevBlockSize + BlockBuf.GetSize());
                  }

                  // This is crucial, if it fails during file transfer, it's fatal error
                  FILE_OPERATION_LOOP_EX (false, FMTLOAD(WRITE_ERROR, DestFileName.c_str()),
                    BlockBuf.WriteToStream(FileStream, BlockBuf.GetSize());
                  );

                  OperationProgress->AddLocallyUsed(BlockBuf.GetSize());

                  if (OperationProgress->Cancel == csCancelTransfer)
                  {
                    throw ExtException(FMTLOAD(USER_TERMINATED));
                  }
                }
                while (!OperationProgress->IsLocallyDone() || !
                    OperationProgress->IsTransferDone());
              }
              catch (const std::exception &E)
              {
                // Every exception during file transfer is fatal
                FTerminal->FatalError(&E,
                  FMTLOAD(COPY_FATAL, OperationProgress->FileName.c_str()));
              }

              OperationProgress->TransferingFile = false;

              try
              {
                SCPResponse();
                // If one of following exception occurs, we still need
                // to send confirmation to other side
              }
              catch (const EScp &E)
              {
                // FSecureShell->SendNull();
                throw;
              }
              catch (const EScpFileSkipped &E)
              {
                // FSecureShell->SendNull();
                throw;
              }

              // FSecureShell->SendNull();

              if (FileData.SetTime && CopyParam->GetPreserveTime())
              {
                SetFileTime(File, NULL, &FileData.AcTime, &FileData.WrTime);
              }
            }
          }
          catch (const std::exception & E)
          {
            if (SkipConfirmed)
            {
              Action.Cancel();
            }
            else
            {
              FTerminal->RollbackAction(Action, OperationProgress, &E);
            }
            throw;
          }

          if (FileData.Attrs == -1) FileData.Attrs = faArchive;
          int NewAttrs = CopyParam->LocalFileAttrs(FileData.RemoteRights);
          if ((NewAttrs & FileData.Attrs) != NewAttrs)
          {
            FILE_OPERATION_LOOP (FMTLOAD(CANT_SET_ATTRS, DestFileName.c_str()),
              THROWOSIFFALSE(FileSetAttr(DestFileName, FileData.Attrs | NewAttrs) == 0);
            );
          }
        }
      }
    }
    catch (const EScpFileSkipped &E)
    {
      if (!SkipConfirmed)
      {
        SUSPEND_OPERATION (
          TQueryParams Params(qpAllowContinueOnError);
          // DEBUG_PRINTF(L"AbsoluteFileName = %s", AbsoluteFileName.c_str());
          if (FTerminal->QueryUserException(FMTLOAD(COPY_ERROR, AbsoluteFileName.c_str()),
                &E, qaOK | qaAbort, &Params, qtError) == qaAbort)
          {
            OperationProgress->Cancel = csCancel;
          }
          FTerminal->FLog->AddException(&E);
        );
      }
      // this was inside above condition, but then transfer was considered
      // succesfull, even when for example user refused to overwrite file
      Success = false;
    }
    catch (const EScpSkipFile &E)
    {
      DEBUG_PRINTF(L"before FTerminal->HandleException");
      SCPSendError(E.GetMessage(), false);
      Success = false;
      if (!FTerminal->HandleException(&E)) throw;
    }
  }
}

// from FtpFileSystem
//---------------------------------------------------------------------------
const wchar_t * THTTPFileSystem::GetOption(int OptionID) const
{
  TSessionData * Data = FTerminal->GetSessionData();

  switch (OptionID)
  {
    case OPTION_PROXYHOST:
    case OPTION_FWHOST:
      FOptionScratch = Data->GetProxyHost();
      break;

    case OPTION_PROXYUSER:
    case OPTION_FWUSER:
      FOptionScratch = Data->GetProxyUsername();
      break;

    case OPTION_PROXYPASS:
    case OPTION_FWPASS:
      FOptionScratch = Data->GetProxyPassword();
      break;

    case OPTION_ANONPWD:
    case OPTION_TRANSFERIP:
    case OPTION_TRANSFERIP6:
      FOptionScratch = L"";
      break;

    default:
      assert(false);
      FOptionScratch = L"";
  }

  return FOptionScratch.c_str();
}
//---------------------------------------------------------------------------
int THTTPFileSystem::GetOptionVal(int OptionID) const
{
  TSessionData * Data = FTerminal->GetSessionData();
  int Result;

  switch (OptionID)
  {
    case OPTION_PROXYTYPE:
      switch (Data->GetProxyMethod())
      {
        case pmNone:
          Result = PROXY_NONE;
          break;

        case pmSocks4:
          Result = PROXY_SOCKS4;
          break;

        case pmSocks5:
          Result = PROXY_SOCKS5;
          break;

        case pmHTTP:
          Result = PROXY_HTTP;
          break;

        case pmTelnet:
        case pmCmd:
        default:
          assert(false);
          Result = 0; // PROXYTYPE_NOPROXY;
          break;
      }
      break;

    case OPTION_PROXYPORT:
    case OPTION_FWPORT:
      Result = Data->GetProxyPort();
      break;

    case OPTION_PROXYUSELOGON:
      Result = !Data->GetProxyUsername().empty();
      break;

    case OPTION_LOGONTYPE:
      Result = Data->GetFtpProxyLogonType();
      break;

    case OPTION_TIMEOUTLENGTH:
      Result = Data->GetTimeout();
      break;

    case OPTION_DEBUGSHOWLISTING:
      // Listing is logged on FZAPI level 5 (what is strangely LOG_APIERROR)
      Result = (FTerminal->GetConfiguration()->GetActualLogProtocol() >= 1);
      break;

    case OPTION_PASV:
      // should never get here t_server.nPasv being nonzero
      assert(false);
      Result = FALSE;
      break;

    case OPTION_PRESERVEDOWNLOADFILETIME:
    case OPTION_MPEXT_PRESERVEUPLOADFILETIME:
      Result = FFileTransferPreserveTime ? TRUE : FALSE;
      break;

    case OPTION_LIMITPORTRANGE:
      Result = FALSE;
      break;

    case OPTION_PORTRANGELOW:
    case OPTION_PORTRANGEHIGH:
      // should never get here OPTION_LIMITPORTRANGE being zero
      assert(false);
      Result = 0;
      break;

    case OPTION_ENABLE_IPV6:
      Result = ((Data->GetAddressFamily() == afIPv6) ? TRUE : FALSE);
      break;

    case OPTION_KEEPALIVE:
      Result = ((Data->GetFtpPingType() != ptOff) ? TRUE : FALSE);
      break;

    case OPTION_INTERVALLOW:
    case OPTION_INTERVALHIGH:
      Result = Data->GetFtpPingInterval();
      break;

    case OPTION_VMSALLREVISIONS:
      Result = FALSE;
      break;

    case OPTION_SPEEDLIMIT_DOWNLOAD_TYPE:
    case OPTION_SPEEDLIMIT_UPLOAD_TYPE:
      Result = (FFileTransferCPSLimit == 0 ? 0 : 1);
      break;

    case OPTION_SPEEDLIMIT_DOWNLOAD_VALUE:
    case OPTION_SPEEDLIMIT_UPLOAD_VALUE:
      Result = (FFileTransferCPSLimit / 1024); // FZAPI expects KiB/s
      break;

    case OPTION_MPEXT_SHOWHIDDEN:
      Result = (FDoListAll ? TRUE : FALSE);
      break;

    default:
      assert(false);
      Result = FALSE;
      break;
  }

  return Result;
}
//---------------------------------------------------------------------------
bool THTTPFileSystem::PostMessage(unsigned int Type, WPARAM wParam, LPARAM lParam)
{
  if (0) // if (Type == TFileZillaIntf::MSG_TRANSFERSTATUS)
  {
    // Stop here if FileTransferProgress is proceeding,
    // it makes "pause" in queue work.
    // Paused queue item stops in some of the TFileOperationProgressType
    // methods called from FileTransferProgress
    TGuard Guard(FTransferStatusCriticalSection);
  }

  TGuard Guard(FQueueCriticalSection);

  FQueue->push_back(TMessageQueue::value_type(wParam, lParam));
  SetEvent(FQueueEvent);

  return true;
}
//---------------------------------------------------------------------------
bool THTTPFileSystem::ProcessMessage()
{
  bool Result = false;
  TMessageQueue::value_type Message;
  {
    TGuard Guard(FQueueCriticalSection);

    Result = !FQueue->empty();
    if (Result)
    {
      Message = FQueue->front();
      FQueue->pop_front();
    }
    else
    {
      // now we are perfecly sure that the queue is empty as it is locked,
      // so reset the event
      ResetEvent(FQueueEvent);
    }
  }

  if (Result)
  {
    // FFileZillaIntf->HandleMessage(Message.first, Message.second);
  }

  return Result;
}
//---------------------------------------------------------------------------
void THTTPFileSystem::DiscardMessages()
{
  while (ProcessMessage());
}
//---------------------------------------------------------------------------
void THTTPFileSystem::WaitForMessages()
{
  unsigned int Result = WaitForSingleObject(FQueueEvent, INFINITE);
  if (Result != WAIT_OBJECT_0)
  {
    FTerminal->FatalError(NULL, FMTLOAD(INTERNAL_ERROR, L"http#1", IntToStr(Result).c_str()));
  }
}
//---------------------------------------------------------------------------
void THTTPFileSystem::PoolForFatalNonCommandReply()
{
  assert(FReply == 0);
  assert(FCommandReply == 0);
  assert(!FWaitingForReply);

  FWaitingForReply = true;

  unsigned int Reply;

  {
      BOOST_SCOPE_EXIT ( (&Self) )
      {
        Self->FReply = 0;
        assert(Self->FCommandReply == 0);
        Self->FCommandReply = 0;
        assert(Self->FWaitingForReply);
        Self->FWaitingForReply = false;
      } BOOST_SCOPE_EXIT_END
    // discard up to one reply
    // (it should not happen here that two replies are posted anyway)
    while (ProcessMessage() && (FReply == 0));
    Reply = FReply;
  }

  if (Reply != 0)
  {
    // throws
    GotNonCommandReply(Reply);
  }
}
//---------------------------------------------------------------------------
bool THTTPFileSystem::NoFinalLastCode()
{
  return (FLastCodeClass == 0) || (FLastCodeClass == 1);
}
//---------------------------------------------------------------------------
bool THTTPFileSystem::KeepWaitingForReply(unsigned int & ReplyToAwait, bool WantLastCode)
{
  // to keep waiting,
  // non-command reply must be unset,
  // the reply we wait for must be unset or
  // last code must be unset (if we wait for it)
  return
     (FReply == 0) &&
     ((ReplyToAwait == 0) ||
      (WantLastCode && NoFinalLastCode()));
}
//---------------------------------------------------------------------------
void THTTPFileSystem::DoWaitForReply(unsigned int & ReplyToAwait, bool WantLastCode)
{
  try
  {
    while (KeepWaitingForReply(ReplyToAwait, WantLastCode))
    {
      WaitForMessages();
      // wait for the first reply only,
      // i.e. in case two replies are posted get the first only.
      // e.g. when server closes the connection, but posts error message before,
      // sometime it happens that command (like download) fails because of the error
      // and does not catch the disconnection. then asynchronous "disconnect reply"
      // is posted immediately afterwards. leave detection of that to Idle()
      while (ProcessMessage() && KeepWaitingForReply(ReplyToAwait, WantLastCode));
    }

    if (FReply != 0)
    {
      // throws
      GotNonCommandReply(FReply);
    }
  }
  catch (...)
  {
    // even if non-fatal error happens, we must process pending message,
    // so that we "eat" the reply message, so that it gets not mistakenly
    // associated with future connect
    if (FTerminal->GetActive())
    {
      DoWaitForReply(ReplyToAwait, WantLastCode);
    }
    throw;
  }
}
//---------------------------------------------------------------------------
unsigned int THTTPFileSystem::WaitForReply(bool Command, bool WantLastCode)
{
  assert(FReply == 0);
  assert(FCommandReply == 0);
  assert(!FWaitingForReply);
  assert(!FTransferStatusCriticalSection->GetAcquired());

  ResetReply();
  FWaitingForReply = true;

  unsigned int Reply;

  {
      BOOST_SCOPE_EXIT ( (&Self) )
      {
        Self->FReply = 0;
        Self->FCommandReply = 0;
        assert(Self->FWaitingForReply);
        Self->FWaitingForReply = false;
      } BOOST_SCOPE_EXIT_END
    unsigned int & ReplyToAwait = (Command ? FCommandReply : FReply);
    DoWaitForReply(ReplyToAwait, WantLastCode);

    Reply = ReplyToAwait;
  }

  return Reply;
}
//---------------------------------------------------------------------------
unsigned int THTTPFileSystem::WaitForCommandReply(bool WantLastCode)
{
  return WaitForReply(true, WantLastCode);
}
//---------------------------------------------------------------------------
void THTTPFileSystem::WaitForFatalNonCommandReply()
{
  WaitForReply(false, false);
  assert(false);
}
//---------------------------------------------------------------------------
void THTTPFileSystem::ResetReply()
{
  FLastCode = 0;
  FLastCodeClass = 0;
  assert(FLastResponse != NULL);
  FLastResponse->Clear();
  assert(FLastError != NULL);
  FLastError->Clear();
}
//---------------------------------------------------------------------------
void THTTPFileSystem::GotNonCommandReply(unsigned int Reply)
{
  // assert(FLAGSET(Reply, TFileZillaIntf::REPLY_DISCONNECTED));
  GotReply(Reply);
  // should never get here as GotReply should raise fatal exception
  assert(false);
}
//---------------------------------------------------------------------------
void THTTPFileSystem::GotReply(unsigned int Reply, unsigned int Flags,
  std::wstring Error, unsigned int * Code, TStrings ** Response)
{
  {
      BOOST_SCOPE_EXIT ( (&Self) )
      {
        Self->ResetReply();
      } BOOST_SCOPE_EXIT_END
    if (0) // FLAGSET(Reply, TFileZillaIntf::REPLY_OK))
    {
      // assert(Reply == TFileZillaIntf::REPLY_OK);

      // With REPLY_2XX_CODE treat "OK" non-2xx code like an error
      if (FLAGSET(Flags, REPLY_2XX_CODE) && (FLastCodeClass != 2))
      {
        // GotReply(TFileZillaIntf::REPLY_ERROR, Flags, Error);
      }
    }
    else if (0) // FLAGSET(Reply, TFileZillaIntf::REPLY_CANCEL) &&
        // FLAGSET(Flags, REPLY_ALLOW_CANCEL))
    {
      // assert(
        // (Reply == (TFileZillaIntf::REPLY_CANCEL | TFileZillaIntf::REPLY_ERROR)) ||
        // (Reply == (TFileZillaIntf::REPLY_ABORTED | TFileZillaIntf::REPLY_CANCEL | TFileZillaIntf::REPLY_ERROR)));
      // noop
    }
    // we do not expect these with our usage of FZ
    else if (0) // Reply &
          // (TFileZillaIntf::REPLY_WOULDBLOCK | TFileZillaIntf::REPLY_OWNERNOTSET |
           // TFileZillaIntf::REPLY_INVALIDPARAM | TFileZillaIntf::REPLY_ALREADYCONNECTED |
           // TFileZillaIntf::REPLY_IDLE | TFileZillaIntf::REPLY_NOTINITIALIZED |
           // TFileZillaIntf::REPLY_ALREADYINIZIALIZED))
    {
      FTerminal->FatalError(NULL, FMTLOAD(INTERNAL_ERROR, L"http#2", FORMAT(L"0x%x", int(Reply)).c_str()));
    }
    else
    {
      // everything else must be an error or disconnect notification
      // assert(
        // FLAGSET(Reply, TFileZillaIntf::REPLY_ERROR) ||
        // FLAGSET(Reply, TFileZillaIntf::REPLY_DISCONNECTED));

      // TODO: REPLY_CRITICALERROR ignored

      // REPLY_NOTCONNECTED happens if connection is closed between moment
      // when FZAPI interface method dispatches the command to FZAPI thread
      // and moment when FZAPI thread receives the command
      bool Disconnected = false;
        // FLAGSET(Reply, TFileZillaIntf::REPLY_DISCONNECTED) ||
        // FLAGSET(Reply, TFileZillaIntf::REPLY_NOTCONNECTED);

      TStrings * MoreMessages = new TStringList();
      try
      {
        if (Disconnected)
        {
          if (FLAGCLEAR(Flags, REPLY_CONNECT))
          {
            MoreMessages->Add(LoadStr(LOST_CONNECTION));
            Discard();
            FTerminal->Closed();
          }
          else
          {
            // For connection failure, do not report that connection was lost,
            // its obvious.
            // Also do not report to terminal that we are closed as
            // that turns terminal into closed mode, but we want to
            // pretend (at least with failed authentication) to retry
            // with the same connection (as with SSH), so we explicitly
            // close terminal in Open() only after we give up
            Discard();
          }
        }

        if (0) // FLAGSET(Reply, TFileZillaIntf::REPLY_ABORTED))
        {
          MoreMessages->Add(LoadStr(USER_TERMINATED));
        }

        if (0) // FLAGSET(Reply, TFileZillaIntf::REPLY_NOTSUPPORTED))
        {
          MoreMessages->Add(LoadStr(FZ_NOTSUPPORTED));
        }

        if (FLastCode == 530)
        {
          MoreMessages->Add(LoadStr(AUTHENTICATION_FAILED));
        }

        if (FLastCode == 425)
        {
          if (!FTerminal->GetSessionData()->GetFtpPasvMode())
          {
            MoreMessages->Add(LoadStr(FTP_CANNOT_OPEN_ACTIVE_CONNECTION));
          }
        }

        MoreMessages->AddStrings(FLastError);
        // already cleared from WaitForReply, but GotReply can be also called
        // from Closed. then make sure that error from previous command not
        // associated with session closure is not reused
        FLastError->Clear();

        MoreMessages->AddStrings(FLastResponse);
        // see comment for FLastError
        FLastResponse->Clear();

        if (MoreMessages->GetCount() == 0)
        {
          delete MoreMessages;
          MoreMessages = NULL;
        }
      }
      catch (...)
      {
        delete MoreMessages;
        throw;
      }

      if (Error.empty() && (MoreMessages != NULL))
      {
        assert(MoreMessages->GetCount() > 0);
        Error = MoreMessages->GetString(0);
        MoreMessages->Delete(0);
      }

      if (Disconnected)
      {
        // for fatal error, it is essential that there is some message
        assert(!Error.empty());
        ExtException *E = new ExtException(Error, MoreMessages, true);
        {
            BOOST_SCOPE_EXIT ( (&E) )
            {
              delete E;
            } BOOST_SCOPE_EXIT_END
          FTerminal->FatalError(E, L"");
        }
      }
      else
      {
        throw ExtException(Error, MoreMessages, true);
      }
    }

    if ((Code != NULL) && (FLastCodeClass != DummyCodeClass))
    {
      *Code = FLastCode;
    }

    if (Response != NULL)
    {
      *Response = FLastResponse;
      FLastResponse = new TStringList();
    }
  }
}
//---------------------------------------------------------------------------
void THTTPFileSystem::SetLastCode(int Code)
{
  FLastCode = Code;
  FLastCodeClass = (Code / 100);
}
//---------------------------------------------------------------------------
void THTTPFileSystem::HandleReplyStatus(std::wstring Response)
{
  int Code = 0;
  // DEBUG_PRINTF(L"Response = %s", Response.c_str());

  if (!FOnCaptureOutput.empty())
  {
    FOnCaptureOutput(Response, false);
  }

  // Two forms of multiline responses were observed
  // (the first is according to the RFC 959):

  // 211-Features:
  //  MDTM
  //  REST STREAM
  //  SIZE
  // 211 End

  // 211-Features:
  // 211-MDTM
  // 211-REST STREAM
  // 211-SIZE
  // 211-AUTH TLS
  // 211-PBSZ
  // 211-PROT
  // 211 End

  bool HasCodePrefix =
    (Response.size() >= 3) &&
    TryStrToInt(Response.substr(0, 3), Code) &&
    (Code >= 100) && (Code <= 599) &&
    ((Response.size() == 3) || (Response[3] == L' ') || (Response[3] == L'-'));

  // DEBUG_PRINTF(L"Code = %d", Code);
  if (HasCodePrefix && !FMultineResponse)
  {
    FMultineResponse = (Response.size() >= 4) && (Response[3] == L'-');
    FLastResponse->Clear();
    if (Response.size() >= 5)
    {
      FLastResponse->Add(Response.substr(4, Response.size() - 4));
    }
    SetLastCode(Code);
  }
  else
  {
    int Start = 0;
    // response with code prefix
    if (HasCodePrefix && (FLastCode == Code))
    {
      // End of multiline response?
      if ((Response.size() <= 3) || (Response[3] == L' '))
      {
        FMultineResponse = false;
      }
      Start = 4;
    }
    else
    {
      Start = (((Response.size() >= 1) && (Response[0] == L' ')) ? 1 : 0);
    }

    // Intermediate empty lines are being added
    if (FMultineResponse || (Response.size() >= Start))
    {
      FLastResponse->Add(Response.substr(Start, Response.size() - Start + 1));
    }
  }

  if (!FMultineResponse)
  {
    if (FLastCode == 220)
    {
      if (FTerminal->GetConfiguration()->GetShowFtpWelcomeMessage())
      {
        FTerminal->DisplayBanner(FLastResponse->GetText());
      }
    }
    else if (FLastCommand == PASS)
    {
      // 530 = "Not logged in."
      if (FLastCode == 530)
      {
        FPasswordFailed = true;
      };
    }
    else if (FLastCommand == SYST)
    {
      assert(FSystem.empty());
      // Possitive reply to "SYST" must be 215, see RFC 959
      if (FLastCode == 215)
      {
        FSystem = ::TrimRight(FLastResponse->GetText());
        // full name is "Personal FTP Server PRO K6.0"
        // if ((FListAll == asAuto) &&
            // (::Pos(FSystem, L"Personal FTP Server") != std::wstring::npos))
        // {
          // FTerminal->LogEvent(L"Server is known not to support LIST -a");
          // FListAll = asOff;
        // }
      }
      else
      {
        FSystem = L"";
      }
    }
    else if (FLastCommand == FEAT)
    {
      // Response to FEAT must be multiline, where leading and trailing line
      // is "meaningless". See RFC 2389.
      if ((FLastCode == 211) && (FLastResponse->GetCount() > 2))
      {
        FLastResponse->Delete(0);
        FLastResponse->Delete(FLastResponse->GetCount() - 1);
        // FFeatures->Assign(FLastResponse);
        // DEBUG_PRINTF(L"FFeatures = %s", FFeatures->GetText().c_str());
      }
      else
      {
        // FFeatures->Clear();
      }
    }
  }
}
//---------------------------------------------------------------------------
std::wstring THTTPFileSystem::ExtractStatusMessage(std::wstring Status)
{
  // CApiLog::LogMessage
  // (note that the formatting may not be present when LogMessageRaw is used)
  int P1 = ::Pos(Status, L"): ");
  if (P1 != std::wstring::npos)
  {
    int P2 = ::Pos(Status, L".cpp(");
    if ((P2 != std::wstring::npos) && (P2 < P1))
    {
      int P3 = ::Pos(Status, L"   caller=0x");
      if ((P3 != std::wstring::npos) && (P3 > P1))
      {
        Status = Status.substr(P1 + 3, P3 - P1 - 3);
      }
    }
  }
  return Status;
}
//---------------------------------------------------------------------------
bool THTTPFileSystem::HandleStatus(const wchar_t * AStatus, int Type)
{
  TLogLineType LogType = (TLogLineType)-1;
  std::wstring Status(AStatus);
  // DEBUG_PRINTF(L"Status = %s", Status.c_str());
  // DEBUG_PRINTF(L"Type = %d", Type);
  /*
  switch (Type)
  {
    case TFileZillaIntf::LOG_STATUS:
      FTerminal->Information(Status, true);
      LogType = llMessage;
      break;

    case TFileZillaIntf::LOG_COMMAND:
      if (Status == L"SYST")
      {
        FLastCommand = SYST;
      }
      else if (Status == L"FEAT")
      {
        FLastCommand = FEAT;
      }
      else if (Status.substr(0, 5) == L"PASS ")
      {
        FLastCommand = PASS;
      }
      else
      {
        FLastCommand = CMD_UNKNOWN;
      }
      LogType = llInput;
      break;

    case TFileZillaIntf::LOG_ERROR:
    case TFileZillaIntf::LOG_APIERROR:
    case TFileZillaIntf::LOG_WARNING:
      // when timeout message occurs, break loop waiting for response code
      // by setting dummy one
      if (Type == TFileZillaIntf::LOG_ERROR)
      {
        if (Status == FTimeoutStatus)
        {
          if (NoFinalLastCode())
          {
            SetLastCode(DummyTimeoutCode);
          }
        }
        else if (Status == FDisconnectStatus)
        {
          if (NoFinalLastCode())
          {
            SetLastCode(DummyDisconnectCode);
          }
        }
      }
      // there can be multiple error messages associated with single failure
      // (such as "cannot open local file..." followed by "download failed")
      Status = ExtractStatusMessage(Status);
      FLastError->Add(Status);
      LogType = llMessage;
      break;

    case TFileZillaIntf::LOG_REPLY:
      HandleReplyStatus(AStatus);
      LogType = llOutput;
      break;

    case TFileZillaIntf::LOG_INFO:
      Status = ExtractStatusMessage(Status);
      LogType = llMessage;
      break;

    default:
      assert(false);
      break;
  }
  */

  if (FTerminal->GetLog()->GetLogging() && (LogType != (TLogLineType)-1))
  {
    FTerminal->GetLog()->Add(LogType, Status);
  }

  return true;
}
//---------------------------------------------------------------------------
TDateTime THTTPFileSystem::ConvertLocalTimestamp(time_t Time)
{
  // This reverses how FZAPI converts FILETIME to time_t,
  // before passing it to FZ_ASYNCREQUEST_OVERWRITE.
  __int64 Timestamp;
  tm * Tm = localtime(&Time);
  if (Tm != NULL)
  {
    SYSTEMTIME SystemTime;
    SystemTime.wYear = static_cast<WORD>(Tm->tm_year + 1900);
    SystemTime.wMonth = static_cast<WORD>(Tm->tm_mon + 1);
    SystemTime.wDayOfWeek = 0;
    SystemTime.wDay = static_cast<WORD>(Tm->tm_mday);
    SystemTime.wHour = static_cast<WORD>(Tm->tm_hour);
    SystemTime.wMinute = static_cast<WORD>(Tm->tm_min);
    SystemTime.wSecond = static_cast<WORD>(Tm->tm_sec);
    SystemTime.wMilliseconds = 0;

    FILETIME LocalTime;
    SystemTimeToFileTime(&SystemTime, &LocalTime);
    FILETIME FileTime;
    LocalFileTimeToFileTime(&LocalTime, &FileTime);
    Timestamp = ConvertTimestampToUnixSafe(FileTime, dstmUnix);
  }
  else
  {
    // incorrect, but at least something
    Timestamp = Time;
  }

  return UnixToDateTime(Timestamp, dstmUnix);
}
//---------------------------------------------------------------------------
TDateTime THTTPFileSystem::ConvertRemoteTimestamp(time_t Time, bool HasTime)
{
  TDateTime Result;
  tm * Tm = localtime(&Time);
  if (Tm != NULL)
  {
    // should be the same as HandleListData
    Result = EncodeDateVerbose(
      static_cast<unsigned short>(Tm->tm_year + 1900),
      static_cast<unsigned short>(Tm->tm_mon + 1),
      static_cast<unsigned short>(Tm->tm_mday));
    if (HasTime)
    {
      Result = Result + EncodeTimeVerbose(
        static_cast<unsigned short>(Tm->tm_hour),
        static_cast<unsigned short>(Tm->tm_min),
        static_cast<unsigned short>(Tm->tm_sec), 0);
    }
  }
  else
  {
    // incorrect, but at least something
    Result = UnixToDateTime(Time, dstmUnix);
  }
  return Result;
}
//---------------------------------------------------------------------------
bool THTTPFileSystem::HandleAsynchRequestOverwrite(
  wchar_t * FileName1, size_t FileName1Len, const wchar_t * /*FileName2*/,
  const wchar_t * /*Path1*/, const wchar_t * /*Path2*/,
  __int64 Size1, __int64 Size2, time_t Time1, time_t Time2,
  bool HasTime1, bool HasTime2, void * AUserData, int & RequestResult)
{
  if (!FActive)
  {
    return false;
  }
  else
  {
    TFileTransferData & UserData = *((TFileTransferData *)AUserData);
    if (UserData.OverwriteResult >= 0)
    {
      // on retry, use the same answer as on the first attempt
      RequestResult = UserData.OverwriteResult;
    }
    else
    {
      TFileOperationProgressType * OperationProgress = FTerminal->GetOperationProgress();
      std::wstring FileName = FileName1;
      assert(UserData.FileName == FileName);
      TOverwriteMode OverwriteMode = omOverwrite;
      TOverwriteFileParams FileParams;
      bool NoFileParams =
        (Size1 < 0) || (Time1 == 0) ||
        (Size2 < 0) || (Time2 == 0);
      if (!NoFileParams)
      {
        FileParams.SourceSize = Size2;
        FileParams.DestSize = Size1;

        if (OperationProgress->Side == osLocal)
        {
          FileParams.SourceTimestamp = ConvertLocalTimestamp(Time2);
          FileParams.DestTimestamp = ConvertRemoteTimestamp(Time1, HasTime1);
          FileParams.DestPrecision = (HasTime1 ? mfMDHM : mfMDY);
        }
        else
        {
          FileParams.SourceTimestamp = ConvertRemoteTimestamp(Time2, HasTime2);
          FileParams.SourcePrecision = (HasTime2 ? mfMDHM : mfMDY);
          FileParams.DestTimestamp = ConvertLocalTimestamp(Time1);
        }
      }

      if (ConfirmOverwrite(FileName, OverwriteMode, OperationProgress,
            (NoFileParams ? NULL : &FileParams), UserData.Params,
            UserData.AutoResume && UserData.CopyParam->AllowResume(FileParams.SourceSize)))
      {
        switch (OverwriteMode)
        {
          case omOverwrite:
            if (FileName != FileName1)
            {
              wcsncpy(FileName1, FileName.c_str(), FileName1Len);
              FileName1[FileName1Len - 1] = '\0';
              UserData.FileName = FileName1;
              // RequestResult = TFileZillaIntf::FILEEXISTS_RENAME;
            }
            else
            {
              // RequestResult = TFileZillaIntf::FILEEXISTS_OVERWRITE;
            }
            break;

          case omResume:
            // RequestResult = TFileZillaIntf::FILEEXISTS_RESUME;
            break;

          default:
            assert(false);
            // RequestResult = TFileZillaIntf::FILEEXISTS_OVERWRITE;
            break;
        }
      }
      else
      {
        // RequestResult = TFileZillaIntf::FILEEXISTS_SKIP;
      }
    }

    // remember the answer for the retries
    UserData.OverwriteResult = RequestResult;

    if (0) // RequestResult == TFileZillaIntf::FILEEXISTS_SKIP)
    {
      // when user chosses not to overwrite, break loop waiting for response code
      // by setting dummy one, az FZAPI won't do anything then
      SetLastCode(DummyTimeoutCode);
    }

    return true;
  }
}
//---------------------------------------------------------------------------
struct TClipboardHandler
{
  std::wstring Text;

  void Copy(TObject * /*Sender*/)
  {
    CopyToClipboard(Text);
  }
};
/*
//---------------------------------------------------------------------------
std::wstring FormatContactList(std::wstring Entry1, std::wstring Entry2)
{
  if (!Entry1.empty() && !Entry2.empty())
  {
    return FORMAT(L"%s, %s", Entry1.c_str(), Entry2.c_str());
  }
  else
  {
    return Entry1 + Entry2;
  }
}
//---------------------------------------------------------------------------
std::wstring FormatContact(const TFtpsCertificateData::TContact & Contact)
{
  std::wstring Result =
    FORMAT(LoadStrPart(VERIFY_CERT_CONTACT, 1).c_str(),
      FormatContactList(FormatContactList(FormatContactList(
        ::MB2W(Contact.Organization).c_str(),
		::MB2W(Contact.Unit).c_str()).c_str(),
		::MB2W(Contact.CommonName).c_str()).c_str(),
			::MB2W(Contact.Mail).c_str()).c_str());

  if ((strlen(Contact.Country) > 0) ||
      (strlen(Contact.StateProvince) > 0) ||
      (strlen(Contact.Town) > 0))
  {
    Result +=
      FORMAT(LoadStrPart(VERIFY_CERT_CONTACT, 2).c_str(),
        FormatContactList(FormatContactList(
          ::MB2W(Contact.Country).c_str(),
		  ::MB2W(Contact.StateProvince).c_str()).c_str(),
			::MB2W(Contact.Town).c_str()).c_str());
  }

  if (strlen(Contact.Other) > 0)
  {
    Result += FORMAT(LoadStrPart(VERIFY_CERT_CONTACT, 3).c_str(), Contact.Other);
  }

  return Result;
}
//---------------------------------------------------------------------------
std::wstring FormatValidityTime(const TFtpsCertificateData::TValidityTime & ValidityTime)
{
  return FormatDateTime(L"ddddd tt",
    EncodeDateVerbose(
      (unsigned short)ValidityTime.Year, (unsigned short)ValidityTime.Month,
      (unsigned short)ValidityTime.Day) +
    EncodeTimeVerbose(
      (unsigned short)ValidityTime.Hour, (unsigned short)ValidityTime.Min,
      (unsigned short)ValidityTime.Sec, 0));
}
*/
//---------------------------------------------------------------------------
/*
bool THTTPFileSystem::HandleAsynchRequestVerifyCertificate(
  const TFtpsCertificateData & Data, int & RequestResult)
{
  if (!FActive)
  {
    return false;
  }
  else
  {
    FSessionInfo.CertificateFingerprint =
      StrToHex(std::wstring((const wchar_t*)Data.Hash, Data.HashLen), false, ':');

    int VerificationResultStr;
    switch (Data.VerificationResult)
    {
      case X509_V_OK:
        VerificationResultStr = CERT_OK;
        break;
      case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT:
        VerificationResultStr = CERT_ERR_UNABLE_TO_GET_ISSUER_CERT;
        break;
      case X509_V_ERR_UNABLE_TO_DECRYPT_CERT_SIGNATURE:
        VerificationResultStr = CERT_ERR_UNABLE_TO_DECRYPT_CERT_SIGNATURE;
        break;
      case X509_V_ERR_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY:
        VerificationResultStr = CERT_ERR_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY;
        break;
      case X509_V_ERR_CERT_SIGNATURE_FAILURE:
        VerificationResultStr = CERT_ERR_CERT_SIGNATURE_FAILURE;
        break;
      case X509_V_ERR_CERT_NOT_YET_VALID:
        VerificationResultStr = CERT_ERR_CERT_NOT_YET_VALID;
        break;
      case X509_V_ERR_CERT_HAS_EXPIRED:
        VerificationResultStr = CERT_ERR_CERT_HAS_EXPIRED;
        break;
      case X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD:
        VerificationResultStr = CERT_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD;
        break;
      case X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD:
        VerificationResultStr = CERT_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD;
        break;
      case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
        VerificationResultStr = CERT_ERR_DEPTH_ZERO_SELF_SIGNED_CERT;
        break;
      case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN:
        VerificationResultStr = CERT_ERR_SELF_SIGNED_CERT_IN_CHAIN;
        break;
      case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY:
        VerificationResultStr = CERT_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY;
        break;
      case X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE:
        VerificationResultStr = CERT_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE;
        break;
      case X509_V_ERR_INVALID_CA:
        VerificationResultStr = CERT_ERR_INVALID_CA;
        break;
      case X509_V_ERR_PATH_LENGTH_EXCEEDED:
        VerificationResultStr = CERT_ERR_PATH_LENGTH_EXCEEDED;
        break;
      case X509_V_ERR_INVALID_PURPOSE:
        VerificationResultStr = CERT_ERR_INVALID_PURPOSE;
        break;
      case X509_V_ERR_CERT_UNTRUSTED:
        VerificationResultStr = CERT_ERR_CERT_UNTRUSTED;
        break;
      case X509_V_ERR_CERT_REJECTED:
        VerificationResultStr = CERT_ERR_CERT_REJECTED;
        break;
      case X509_V_ERR_KEYUSAGE_NO_CERTSIGN:
        VerificationResultStr = CERT_ERR_KEYUSAGE_NO_CERTSIGN;
        break;
      case X509_V_ERR_CERT_CHAIN_TOO_LONG:
        VerificationResultStr = CERT_ERR_CERT_CHAIN_TOO_LONG;
        break;
      default:
        VerificationResultStr = CERT_ERR_UNKNOWN;
        break;
    }

    std::wstring Summary = LoadStr(VerificationResultStr);
    if (Data.VerificationResult != X509_V_OK)
    {
      Summary += L" " + FMTLOAD(CERT_ERRDEPTH, Data.VerificationDepth + 1);
    }

    FSessionInfo.Certificate =
      FMTLOAD(CERT_TEXT,
        FormatContact(Data.Subject).c_str(),
        FormatContact(Data.Issuer).c_str(),
        FormatValidityTime(Data.ValidFrom).c_str(),
        FormatValidityTime(Data.ValidUntil).c_str(),
        FSessionInfo.CertificateFingerprint.c_str(),
        Summary.c_str());

    RequestResult = 0;

    THierarchicalStorage * Storage =
      FTerminal->GetConfiguration()->CreateScpStorage(false);
    {
        BOOST_SCOPE_EXIT ( (Storage) )
        {
          delete Storage;
        } BOOST_SCOPE_EXIT_END
      Storage->SetAccessMode(smRead);

      if (Storage->OpenSubKey(CertificateStorageKey, false) &&
          Storage->ValueExists(FSessionInfo.CertificateFingerprint))
      {
        RequestResult = 1;
      }
    }

    if (RequestResult == 0)
    {
      std::wstring Buf = FTerminal->GetSessionData()->GetHostKey();
      while ((RequestResult == 0) && !Buf.empty())
      {
        std::wstring ExpectedKey = CutToChar(Buf, ';', false);
        if (ExpectedKey == FSessionInfo.CertificateFingerprint)
        {
          RequestResult = 1;
        }
      }
    }

    if (RequestResult == 0)
    {
      TClipboardHandler ClipboardHandler;
      ClipboardHandler.Text = FSessionInfo.CertificateFingerprint;

      TQueryButtonAlias Aliases[1];
      Aliases[0].Button = qaRetry;
      Aliases[0].Alias = LoadStr(COPY_KEY_BUTTON);
      Aliases[0].OnClick.connect(boost::bind(&TClipboardHandler::Copy, ClipboardHandler, _1));

      TQueryParams Params;
      Params.HelpKeyword = HELP_VERIFY_CERTIFICATE;
      Params.NoBatchAnswers = qaYes | qaRetry;
      Params.Aliases = Aliases;
      Params.AliasesCount = LENOF(Aliases);
      int Answer = FTerminal->QueryUser(
        FMTLOAD(VERIFY_CERT_PROMPT2, FSessionInfo.Certificate.c_str()),
        NULL, qaYes | qaNo | qaCancel | qaRetry, &Params, qtWarning);

      switch (Answer)
      {
        case qaYes:
          // 2 = always, as used by FZ's VerifyCertDlg.cpp,
          // however FZAPI takes all non-zero values equally
          RequestResult = 2;
          break;

        case qaNo:
          RequestResult = 1;
          break;

        case qaCancel:
          RequestResult = 0;
          break;

        default:
          assert(false);
          RequestResult = 0;
          break;
      }

      if (RequestResult == 2)
      {
        THierarchicalStorage * Storage =
          FTerminal->GetConfiguration()->CreateScpStorage(false);
        {
            BOOST_SCOPE_EXIT ( (Storage) )
            {
              delete Storage;
            } BOOST_SCOPE_EXIT_END
          Storage->SetAccessMode(smReadWrite);

          if (Storage->OpenSubKey(CertificateStorageKey, true))
          {
            Storage->WriteString(FSessionInfo.CertificateFingerprint, L"");
          }
        }
      }
    }

    return true;
  }
}
*/
//---------------------------------------------------------------------------
bool THTTPFileSystem::HandleListData(const wchar_t * Path,
  const TListDataEntry * Entries, unsigned int Count)
{
  if (!FActive)
  {
    return false;
  }
  else if (FIgnoreFileList)
  {
    // directory listing provided implicitly by FZAPI during certain operations is ignored
    assert(FFileList == NULL);
    return false;
  }
  else
  {
    assert(FFileList != NULL);
    // this can actually fail in real life,
    // when connected to server with case insensitive paths
    assert(UnixComparePaths(AbsolutePath(FFileList->GetDirectory(), false), Path));
    USEDPARAM(Path);

    for (unsigned int Index = 0; Index < Count; Index++)
    {
      const TListDataEntry * Entry = &Entries[Index];
      TRemoteFile * File = new TRemoteFile();
      try
      {
        File->SetTerminal(FTerminal);

        File->SetFileName(std::wstring(Entry->Name));
        if (wcslen(Entry->Permissions) >= 10)
        {
          try
          {
            File->GetRights()->SetText(Entry->Permissions + 1);
          }
          catch (...)
          {
            // ignore permissions errors with FTP
          }
        }
		// FIXME
		std::wstring own = Entry->OwnerGroup;
        const wchar_t * Space = wcschr(own.c_str(), ' ');
        if (Space != NULL)
        {
          File->GetOwner().SetName(std::wstring(own.c_str(), Space - own.c_str()));
          File->GetGroup().SetName(Space + 1);
        }
        else
        {
          File->GetOwner().SetName(Entry->OwnerGroup);
        }

        File->SetSize(Entry->Size);

        if (Entry->Link)
        {
          File->SetType(FILETYPE_SYMLINK);
        }
        else if (Entry->Dir)
        {
          File->SetType(FILETYPE_DIRECTORY);
        }
        else
        {
          File->SetType('-');
        }

        // ModificationFmt must be set after Modification
        if (Entry->HasDate)
        {
          // should be the same as ConvertRemoteTimestamp
          TDateTime Modification =
            EncodeDateVerbose((unsigned short)Entry->Year, (unsigned short)Entry->Month,
              (unsigned short)Entry->Day);
          if (Entry->HasTime)
          {
            File->SetModification(Modification +
              EncodeTimeVerbose((unsigned short)Entry->Hour, (unsigned short)Entry->Minute, 0, 0));
            // not exact as we got year as well, but it is most probably
            // guessed by FZAPI anyway
            File->SetModificationFmt(mfMDHM);
          }
          else
          {
            File->SetModification(Modification);
            File->SetModificationFmt(mfMDY);
          }
        }
        else
        {
          // With SCP we estimate date to be today, if we have at least time

          File->SetModification(TDateTime(double(0)));
          File->SetModificationFmt(mfNone);
        }
        File->SetLastAccess(File->GetModification());

        File->SetLinkTo(Entry->LinkTarget);

        File->Complete();
      }
      catch (const std::exception & E)
      {
        delete File;
        std::wstring EntryData =
          FORMAT(L"%s/%s/%s/%s/%d/%d/%d/%d/%d/%d/%d/%d/%d",
            Entry->Name,
            Entry->Permissions,
            Entry->OwnerGroup,
            IntToStr(Entry->Size).c_str(),
             int(Entry->Dir), int(Entry->Link), Entry->Year, Entry->Month, Entry->Day,
             Entry->Hour, Entry->Minute, int(Entry->HasTime), int(Entry->HasDate));
        throw ETerminal(FMTLOAD(LIST_LINE_ERROR, EntryData.c_str()), &E);
      }

      FFileList->AddFile(File);
    }
    return true;
  }
}
//---------------------------------------------------------------------------
bool THTTPFileSystem::HandleTransferStatus(bool Valid, __int64 TransferSize,
  __int64 Bytes, int /*Percent*/, int /*TimeElapsed*/, int /*TimeLeft*/, int /*TransferRate*/,
  bool FileTransfer)
{
  if (!FActive)
  {
    return false;
  }
  else if (!Valid)
  {
  }
  else if (FileTransfer)
  {
    FileTransferProgress(TransferSize, Bytes);
  }
  else
  {
    ReadDirectoryProgress(Bytes);
  }
  return true;
}
//---------------------------------------------------------------------------
bool THTTPFileSystem::HandleReply(int Command, unsigned int Reply)
{
  if (!FActive)
  {
    return false;
  }
  else
  {
    if (FTerminal->GetConfiguration()->GetActualLogProtocol() >= 1)
    {
      FTerminal->LogEvent(FORMAT(L"Got reply %x to the command %d", int(Reply), Command));
    }

    // reply with Command 0 is not associated with current operation
    // so do not treat is as a reply
    // (it is typically used asynchronously to notify about disconnects)
    if (Command != 0)
    {
      assert(FCommandReply == 0);
      FCommandReply = Reply;
    }
    else
    {
      assert(FReply == 0);
      FReply = Reply;
    }
    return true;
  }
}

//---------------------------------------------------------------------------
void THTTPFileSystem::ResetFileTransfer()
{
  FFileTransferAbort = ftaNone;
  FFileTransferCancelled = false;
  FFileTransferResumed = 0;
}
//---------------------------------------------------------------------------
void THTTPFileSystem::ReadDirectoryProgress(__int64 Bytes)
{
  // with FTP we do not know exactly how many entries we have received,
  // instead we know number of bytes received only.
  // so we report approximation based on average size of entry.
  int Progress = int(Bytes / 80);
  if (Progress - FLastReadDirectoryProgress >= 10)
  {
    bool Cancel = false;
    FLastReadDirectoryProgress = Progress;
    FTerminal->DoReadDirectoryProgress(Progress, Cancel);
    if (Cancel)
    {
      FTerminal->DoReadDirectoryProgress(-2, Cancel);
      // FFileZillaIntf->Cancel();
    }
  }
}
//---------------------------------------------------------------------------
void THTTPFileSystem::DoFileTransferProgress(__int64 TransferSize,
  __int64 Bytes)
{
  TFileOperationProgressType * OperationProgress = FTerminal->GetOperationProgress();

  OperationProgress->SetTransferSize(TransferSize);

  if (FFileTransferResumed > 0)
  {
    OperationProgress->AddResumed(FFileTransferResumed);
    FFileTransferResumed = 0;
  }

  __int64 Diff = Bytes - OperationProgress->TransferedSize;
  assert(Diff >= 0);
  if (Diff >= 0)
  {
    OperationProgress->AddTransfered(Diff);
  }

  if (OperationProgress->Cancel == csCancel)
  {
    FFileTransferCancelled = true;
    FFileTransferAbort = ftaCancel;
    // FFileZillaIntf->Cancel();
  }

  if (FFileTransferCPSLimit != OperationProgress->CPSLimit)
  {
    FFileTransferCPSLimit = OperationProgress->CPSLimit;
  }
}
//---------------------------------------------------------------------------
void THTTPFileSystem::FileTransferProgress(__int64 TransferSize,
  __int64 Bytes)
{
  TGuard Guard(FTransferStatusCriticalSection);

  DoFileTransferProgress(TransferSize, Bytes);
}

//---------------------------------------------------------------------------
void THTTPFileSystem::FileTransfer(const std::wstring & FileName,
  const std::wstring & LocalFile, const std::wstring & RemoteFile,
  const std::wstring & RemotePath, bool Get, __int64 Size, int Type,
  TFileTransferData & UserData, TFileOperationProgressType * OperationProgress)
{
  FILE_OPERATION_LOOP(FMTLOAD(TRANSFER_ERROR, FileName.c_str()),
    // FFileZillaIntf->FileTransfer(
	  // ::W2MB(LocalFile.c_str()).c_str(),
	  // ::W2MB(RemoteFile.c_str()).c_str(),
      // ::W2MB(RemotePath.c_str()).c_str(),
	  // Get, Size, Type, &UserData);
    // we may actually catch reponse code of the listing
    // command (when checking for existence of the remote file)
    unsigned int Reply = WaitForCommandReply();
    GotReply(Reply, FLAGMASK(FFileTransferCancelled, REPLY_ALLOW_CANCEL));
  );

  switch (FFileTransferAbort)
  {
    case ftaSkip:
      THROW_SKIP_FILE_NULL;

    case ftaCancel:
      Abort();
      break;
  }

  if (!FFileTransferCancelled)
  {
    // show completion of transfer
    // call non-guarded variant to avoid deadlock with keepalives
    // (we are not waiting for reply anymore so keepalives are free to proceed)
    DoFileTransferProgress(OperationProgress->TransferSize, OperationProgress->TransferSize);
  }
}

// from WebDAV
bool THTTPFileSystem::SendPropFindRequest(const wchar_t *dir, std::wstring &response, std::wstring &errInfo)
{
    const std::string webDavPath = EscapeUTF8URL(dir);
    // DEBUG_PRINTF(L"THTTPFileSystem::SendPropFindRequest: webDavPath = %s", ::MB2W(webDavPath.c_str()).c_str());

    response.clear();

    static const char *requestData =
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
        "<D:propfind xmlns:D=\"DAV:\">"
        "<D:prop xmlns:Z=\"urn:schemas-microsoft-com:\">"
        "<D:resourcetype/>"
        "<D:getcontentlength/>"
        "<D:creationdate/>"
        "<D:getlastmodified/>"
        "<Z:Win32LastAccessTime/>"
        "<Z:Win32FileAttributes/>"
        "</D:prop>"
        "</D:propfind>";

    static const size_t requestDataLen = strlen(requestData);

    CURLcode urlCode = CURLPrepare(webDavPath.c_str());
    std::string resp;
    CHECK_CUCALL(urlCode, FCURLIntf->SetOutput(resp, &m_ProgressPercent));

    CSlistURL slist;
    slist.Append("Depth: 1");
    slist.Append("Content-Type: text/xml; charset=\"utf-8\"");
    char contentLength[64];
    sprintf_s(contentLength, "Content-Length: %d", requestDataLen);
    slist.Append(contentLength);
    slist.Append("Connection: Keep-Alive");

    CHECK_CUCALL(urlCode, FCURLIntf->SetSlist(slist));
    CHECK_CUCALL(urlCode, curl_easy_setopt(FCURLIntf->GetCURL(), CURLOPT_CUSTOMREQUEST, "PROPFIND"));
    CHECK_CUCALL(urlCode, curl_easy_setopt(FCURLIntf->GetCURL(), CURLOPT_MAXREDIRS, 5));
    CHECK_CUCALL(urlCode, curl_easy_setopt(FCURLIntf->GetCURL(), CURLOPT_POSTFIELDS, requestData));
    CHECK_CUCALL(urlCode, curl_easy_setopt(FCURLIntf->GetCURL(), CURLOPT_POSTFIELDSIZE, requestDataLen));

    CHECK_CUCALL(urlCode, FCURLIntf->Perform());
    // DEBUG_PRINTF(L"urlCode = %d", urlCode);
    if (urlCode != CURLE_OK)
    {
        errInfo = ::MB2W(curl_easy_strerror(urlCode));
        return false;
    }

    if (!CheckResponseCode(HTTP_STATUS_WEBDAV_MULTI_STATUS, errInfo))
    {
        // DEBUG_PRINTF(L"errInfo = %s", errInfo.c_str());
        return false;
    }
    response = ::MB2W(resp.c_str());
    if (response.empty())
    {
        errInfo = L"Server return empty response";
        return false;
    }

    return true;
}


bool THTTPFileSystem::CheckResponseCode(const long expect, std::wstring &errInfo)
{
    long responseCode = 0;
    if (curl_easy_getinfo(FCURLIntf->GetCURL(), CURLINFO_RESPONSE_CODE, &responseCode) == CURLE_OK)
    {
        if (responseCode != expect)
        {
            errInfo = GetBadResponseInfo(responseCode);
            // DEBUG_PRINTF(L"errInfo = %s", errInfo.c_str());
            return false;
        }
    }
    return true;
}


bool THTTPFileSystem::CheckResponseCode(const long expect1, const long expect2, std::wstring &errInfo)
{
    long responseCode = 0;
    if (curl_easy_getinfo(FCURLIntf->GetCURL(), CURLINFO_RESPONSE_CODE, &responseCode) == CURLE_OK)
    {
        if (responseCode != expect1 && responseCode != expect2)
        {
            errInfo = GetBadResponseInfo(responseCode);
            return false;
        }
    }
    return true;
}


std::wstring THTTPFileSystem::GetBadResponseInfo(const int code) const
{
    const wchar_t *descr = NULL;
    switch (code)
    {
    case HTTP_STATUS_CONTINUE           :
        descr = L"OK to continue with request";
        break;
    case HTTP_STATUS_SWITCH_PROTOCOLS   :
        descr = L"Server has switched protocols in upgrade header";
        break;
    case HTTP_STATUS_OK                 :
        descr = L"Request completed";
        break;
    case HTTP_STATUS_CREATED            :
        descr = L"Object created, reason = new URI";
        break;
    case HTTP_STATUS_ACCEPTED           :
        descr = L"Async completion (TBS)";
        break;
    case HTTP_STATUS_PARTIAL            :
        descr = L"Partial completion";
        break;
    case HTTP_STATUS_NO_CONTENT         :
        descr = L"No info to return";
        break;
    case HTTP_STATUS_RESET_CONTENT      :
        descr = L"Request completed, but clear form";
        break;
    case HTTP_STATUS_PARTIAL_CONTENT    :
        descr = L"Partial GET furfilled";
        break;
    case HTTP_STATUS_WEBDAV_MULTI_STATUS:
        descr = L"WebDAV Multi-Status";
        break;
    case HTTP_STATUS_AMBIGUOUS          :
        descr = L"Server couldn't decide what to return";
        break;
    case HTTP_STATUS_MOVED              :
        descr = L"Object permanently moved";
        break;
    case HTTP_STATUS_REDIRECT           :
        descr = L"Object temporarily moved";
        break;
    case HTTP_STATUS_REDIRECT_METHOD    :
        descr = L"Redirection w/ new access method";
        break;
    case HTTP_STATUS_NOT_MODIFIED       :
        descr = L"If-modified-since was not modified";
        break;
    case HTTP_STATUS_USE_PROXY          :
        descr = L"Redirection to proxy, location header specifies proxy to use";
        break;
    case HTTP_STATUS_REDIRECT_KEEP_VERB :
        descr = L"HTTP/1.1: keep same verb";
        break;
    case HTTP_STATUS_BAD_REQUEST        :
        descr = L"Invalid syntax";
        break;
    case HTTP_STATUS_DENIED             :
        descr = L"Unauthorized";
        break;
    case HTTP_STATUS_PAYMENT_REQ        :
        descr = L"Payment required";
        break;
    case HTTP_STATUS_FORBIDDEN          :
        descr = L"Request forbidden";
        break;
    case HTTP_STATUS_NOT_FOUND          :
        descr = L"Object not found";
        break;
    case HTTP_STATUS_BAD_METHOD         :
        descr = L"Method is not allowed";
        break;
    case HTTP_STATUS_NONE_ACCEPTABLE    :
        descr = L"No response acceptable to client found";
        break;
    case HTTP_STATUS_PROXY_AUTH_REQ     :
        descr = L"Proxy authentication required";
        break;
    case HTTP_STATUS_REQUEST_TIMEOUT    :
        descr = L"Server timed out waiting for request";
        break;
    case HTTP_STATUS_CONFLICT           :
        descr = L"User should resubmit with more info";
        break;
    case HTTP_STATUS_GONE               :
        descr = L"The resource is no longer available";
        break;
    case HTTP_STATUS_LENGTH_REQUIRED    :
        descr = L"The server refused to accept request w/o a length";
        break;
    case HTTP_STATUS_PRECOND_FAILED     :
        descr = L"Precondition given in request failed";
        break;
    case HTTP_STATUS_REQUEST_TOO_LARGE  :
        descr = L"Request entity was too large";
        break;
    case HTTP_STATUS_URI_TOO_LONG       :
        descr = L"Request URI too long";
        break;
    case HTTP_STATUS_UNSUPPORTED_MEDIA  :
        descr = L"Unsupported media type";
        break;
    case 416                            :
        descr = L"Requested Range Not Satisfiable";
        break;
    case 417                            :
        descr = L"Expectation Failed";
        break;
    case HTTP_STATUS_RETRY_WITH         :
        descr = L"Retry after doing the appropriate action";
        break;
    case HTTP_STATUS_SERVER_ERROR       :
        descr = L"Internal server error";
        break;
    case HTTP_STATUS_NOT_SUPPORTED      :
        descr = L"Required not supported";
        break;
    case HTTP_STATUS_BAD_GATEWAY        :
        descr = L"Error response received from gateway";
        break;
    case HTTP_STATUS_SERVICE_UNAVAIL    :
        descr = L"Temporarily overloaded";
        break;
    case HTTP_STATUS_GATEWAY_TIMEOUT    :
        descr = L"Timed out waiting for gateway";
        break;
    case HTTP_STATUS_VERSION_NOT_SUP    :
        descr = L"HTTP version not supported";
        break;
    }

    std::wstring errInfo = L"Incorrect response code: ";

    errInfo += ::NumberToWString(code);

    if (descr)
    {
        errInfo += L' ';
        errInfo += descr;
    }

    return errInfo;
}


std::string THTTPFileSystem::GetNamespace(const TiXmlElement *element, const char *name, const char *defaultVal) const
{
    assert(element);
    assert(name);
    assert(defaultVal);

    std::string ns = defaultVal;
    const TiXmlAttribute *attr = element->FirstAttribute();
    while (attr)
    {
        if (strncmp(attr->Name(), "xmlns:", 6) == 0 && strcmp(attr->Value(), name) == 0)
        {
            ns = attr->Name();
            ns.erase(0, ns.find(':') + 1);
            ns += ':';
            break;
        }
        attr = attr->Next();
    }
    return ns;
}


FILETIME THTTPFileSystem::ParseDateTime(const char *dt) const
{
    assert(dt);

    FILETIME ft;
    ZeroMemory(&ft, sizeof(ft));
    SYSTEMTIME st;
    ZeroMemory(&st, sizeof(st));

    if (WinHttpTimeToSystemTime(::MB2W(dt).c_str(), &st))
    {
        SystemTimeToFileTime(&st, &ft);
    }
    else if (strlen(dt) > 18)
    {
        //rfc 3339 date-time
        st.wYear =   static_cast<WORD>(atoi(dt +  0));
        st.wMonth =  static_cast<WORD>(atoi(dt +  5));
        st.wDay =    static_cast<WORD>(atoi(dt +  8));
        st.wHour =   static_cast<WORD>(atoi(dt + 11));
        st.wMinute = static_cast<WORD>(atoi(dt + 14));
        st.wSecond = static_cast<WORD>(atoi(dt + 17));
        SystemTimeToFileTime(&st, &ft);
    }

    return ft;
}


std::string THTTPFileSystem::DecodeHex(const std::string &src) const
{
    const size_t cntLength = src.length();
    std::string result;
    result.reserve(cntLength);

    for (size_t i = 0; i < cntLength; ++i)
    {
        const char chkChar = src[i];
        if (chkChar != L'%' || (i + 2 >= cntLength) || !IsHexadecimal(src[i + 1]) || !IsHexadecimal(src[i + 2]))
        {
            result += chkChar;
        }
        else
        {
            const char ch1 = src[i + 1];
            const char ch2 = src[i + 2];
            const char encChar = (((ch1 & 0xf) + ((ch1 >= 'A') ? 9 : 0)) << 4) | ((ch2 & 0xf) + ((ch2 >= 'A') ? 9 : 0));
            result += encChar;
            i += 2;
        }
    }

    return result;
}


std::string THTTPFileSystem::EscapeUTF8URL(const wchar_t *src) const
{
    assert(src && src[0] == L'/');

    std::string plainText = ::W2MB(src, CP_UTF8);
    const size_t cntLength = plainText.length();

    std::string result;
    result.reserve(cntLength);

    static const char permitSymbols[] = "/;@&=+$,-_.?!~'()%{}^[]`";

    for (size_t i = 0; i < cntLength; ++i)
    {
        const char chkChar = plainText[i];
        if (*std::find(permitSymbols, permitSymbols + sizeof(permitSymbols), chkChar) ||
                (chkChar >= 'a' && chkChar <= 'z') ||
                (chkChar >= 'A' && chkChar <= 'Z') ||
                (chkChar >= '0' && chkChar <= '9'))
        {
            result += chkChar;
        }
        else
        {
            char encChar[4];
            sprintf_s(encChar, "%%%02X", static_cast<unsigned char>(chkChar));
            result += encChar;
        }
    }
    return result;
}

CURLcode THTTPFileSystem::CURLPrepare(const char *webDavPath, const bool handleTimeout /*= true*/)
{
    CURLcode urlCode = FCURLIntf->Prepare(webDavPath, handleTimeout);
    CHECK_CUCALL(urlCode, curl_easy_setopt(FCURLIntf->GetCURL(), CURLOPT_HTTPAUTH, CURLAUTH_ANY));
    CHECK_CUCALL(urlCode, curl_easy_setopt(FCURLIntf->GetCURL(), CURLOPT_FOLLOWLOCATION, 1));
    CHECK_CUCALL(urlCode, curl_easy_setopt(FCURLIntf->GetCURL(), CURLOPT_POST301, 1));

    CHECK_CUCALL(urlCode, curl_easy_setopt(FCURLIntf->GetCURL(), CURLOPT_SSL_VERIFYPEER, 0L));
    CHECK_CUCALL(urlCode, curl_easy_setopt(FCURLIntf->GetCURL(), CURLOPT_SSL_VERIFYHOST, 0L));
    return urlCode;
}

bool THTTPFileSystem::Connect(HANDLE abortEvent, std::wstring &errorInfo)
{
  assert(abortEvent);

  TSessionData *Data = FTerminal->GetSessionData();
  std::wstring HostName = Data->GetHostName();
  std::wstring UserName = Data->GetUserName();
  std::wstring Password = Data->GetPassword();
  std::wstring Account = Data->GetFtpAccount();
  std::wstring Path = Data->GetRemoteDirectory();

    ProxySettings proxySettings;
    // init proxySettings
    proxySettings.proxyType = GetOptionVal(OPTION_PROXYTYPE);
    proxySettings.proxyHost = GetOption(OPTION_PROXYHOST);
    proxySettings.proxyPort = GetOptionVal(OPTION_PROXYPORT);
    proxySettings.proxyLogin = GetOption(OPTION_PROXYUSER);
    proxySettings.proxyPassword = GetOption(OPTION_PROXYPASS);

  const wchar_t *url = HostName.c_str();
    // DEBUG_PRINTF(L"WebDAV: connecting to %s", url);
    //Initialize curl
    FCURLIntf->Initialize(url, UserName.c_str(), Password.c_str(),
        proxySettings);
    FCURLIntf->SetAbortEvent(abortEvent);

    //Check initial path existing
    std::wstring path;
    // std::wstring query;
    ParseURL(url, NULL, NULL, NULL, &path, NULL, NULL, NULL);
    bool dirExist = false;
    // DEBUG_PRINTF(L"path = %s, query = %s", path.c_str(), query.c_str());
    // if (!query.empty())
    // {
        // path += query;
    // }
    if (!CheckExisting(path.c_str(), ItemDirectory, dirExist, errorInfo) || !dirExist)
    {
        FTerminal->LogEvent(FORMAT(L"WebDAV: path %s does not exist.", path.c_str()));
        return false;
    }
    FCurrentDirectory = path;
    while(GetCurrentDirectory().size() > 1 && GetCurrentDirectory()[GetCurrentDirectory().size() - 1] == L'/')
    {
        FCurrentDirectory.erase(FCurrentDirectory.size() - 1);
    }
    return true;
}

bool THTTPFileSystem::CheckExisting(const wchar_t *path, const ItemType type, bool &isExist, std::wstring &errorInfo)
{
    // DEBUG_PRINTF(L"THTTPFileSystem::CheckExisting: path = %s", path);
    assert(type == ItemDirectory);

    std::wstring responseDummy;
    isExist = SendPropFindRequest(path, responseDummy, errorInfo);
    // DEBUG_PRINTF(L"THTTPFileSystem::CheckExisting: path = %s, isExist = %d", path, isExist);
    return true;
}


bool THTTPFileSystem::MakeDirectory(const wchar_t *path, std::wstring &errorInfo)
{
    // DEBUG_PRINTF(L"MakeDirectory: begin: path = %s", path);
    const std::string webDavPath = EscapeUTF8URL(path);

    CURLcode urlCode = CURLPrepare(webDavPath.c_str());
    CSlistURL slist;
    slist.Append("Content-Type: text/xml; charset=\"utf-8\"");
    slist.Append("Content-Length: 0");
    slist.Append("Connection: Keep-Alive");
    CHECK_CUCALL(urlCode, FCURLIntf->SetSlist(slist));
    CHECK_CUCALL(urlCode, curl_easy_setopt(FCURLIntf->GetCURL(), CURLOPT_CUSTOMREQUEST, "MKCOL"));

    CHECK_CUCALL(urlCode, FCURLIntf->Perform());
    if (urlCode != CURLE_OK)
    {
        errorInfo = ::MB2W(curl_easy_strerror(urlCode));
        return false;
    }

    bool result = CheckResponseCode(HTTP_STATUS_OK, HTTP_STATUS_CREATED, errorInfo);
    // DEBUG_PRINTF(L"MakeDirectory: end: errorInfo = %s", errorInfo.c_str());
    return result;
}


bool THTTPFileSystem::GetList(PluginPanelItem **items, int *itemsNum, std::wstring &errorInfo)
{
    assert(items);
    assert(itemsNum);

    std::wstring response;
    if (!SendPropFindRequest(FCurrentDirectory.c_str(), response, errorInfo))
    {
        return false;
    }

    // Erase slashes (to compare in xml parse)
    std::wstring currentPath(FCurrentDirectory);
    while (!currentPath.empty() && currentPath[currentPath.length() - 1] == L'/')
    {
        currentPath.erase(currentPath.length() - 1);
    }
    while (!currentPath.empty() && currentPath[0] == L'/')
    {
        currentPath.erase(0, 1);
    }

    const std::string decodedResp = DecodeHex(::W2MB(response.c_str()));

#ifdef _DEBUG
    // CNBFile::SaveFile(L"d:\\webdav_response_raw.xml", response.c_str());
    // CNBFile::SaveFile(L"d:\\webdav_response_decoded.xml", decodedResp.c_str());
#endif

    //! WebDAV item description
    struct WebDAVItem
    {
        WebDAVItem() : Attributes(0), Size(0)
        {
            LastAccess.dwLowDateTime = LastAccess.dwHighDateTime = Created.dwLowDateTime = Created.dwHighDateTime = Modified.dwLowDateTime = Modified.dwHighDateTime = 0;
        }
        std::wstring Name;
        DWORD Attributes;
        FILETIME Created;
        FILETIME Modified;
        FILETIME LastAccess;
        unsigned __int64 Size;
    };
    std::vector<WebDAVItem> wdavItems;

    TiXmlDocument xmlDoc;
    xmlDoc.Parse(decodedResp.c_str());
    if (xmlDoc.Error())
    {
        errorInfo = L"Error parsing response xml:\n[";
        errorInfo += ::NumberToWString(xmlDoc.ErrorId());
        errorInfo += L"]: ";
        errorInfo += ::MB2W(xmlDoc.ErrorDesc());
        return false;
    }

    const TiXmlElement *xmlRoot = xmlDoc.RootElement();

    //Determine global namespace
    const std::string glDavNs = GetNamespace(xmlRoot, "DAV:", "D:");
    const std::string glMsNs = GetNamespace(xmlRoot, "urn:schemas-microsoft-com:", "Z:");

    const TiXmlNode *xmlRespNode = NULL;
    while ((xmlRespNode = xmlRoot->IterateChildren((glDavNs + "response").c_str(), xmlRespNode)) != NULL)
    {
        WebDAVItem item;

        const TiXmlElement *xmlRespElem = xmlRespNode->ToElement();
        const std::string davNamespace = GetNamespace(xmlRespElem, "DAV:", glDavNs.c_str());
        const std::string msNamespace = GetNamespace(xmlRespElem, "urn:schemas-microsoft-com:", glMsNs.c_str());
        const TiXmlElement *xmlHref = xmlRespNode->FirstChildElement((glDavNs + "href").c_str());
        if (!xmlHref || !xmlHref->GetText())
        {
            continue;
        }

        const std::wstring href = ::MB2W(xmlHref->GetText(), CP_UTF8);
        std::wstring path;
        ParseURL(href.c_str(), NULL, NULL, NULL, &path, NULL, NULL, NULL);
        if (path.empty())
        {
            path = href;
        }
        while (!path.empty() && path[path.length() - 1] == L'/')
        {
            path.erase(path.length() - 1);
        }
        while (!path.empty() && path[0] == L'/')
        {
            path.erase(0, 1);
        }

        //Check for self-link (compare paths)
        if (_wcsicmp(path.c_str(), currentPath.c_str()) == 0)
        {
            continue;
        }

        //name
        item.Name = path;
        const size_t nameDelim = item.Name.rfind(L'/'); //Save only name without full path
        if (nameDelim != std::wstring::npos)
        {
            item.Name.erase(0, nameDelim + 1);
        }

        //Find correct 'propstat' node (with HTTP 200 OK status)
        const TiXmlElement *xmlProps = NULL;
        const TiXmlNode *xmlPropsNode = NULL;
        while (xmlProps == NULL && (xmlPropsNode = xmlRespNode->IterateChildren((glDavNs + "propstat").c_str(), xmlPropsNode)) != NULL)
        {
            const TiXmlElement *xmlStatus = xmlPropsNode->FirstChildElement((glDavNs + "status").c_str());
            if (xmlStatus && strstr(xmlStatus->GetText(), "200"))
            {
                xmlProps = xmlPropsNode->FirstChildElement((glDavNs + "prop").c_str());
            }
        }
        if (xmlProps)
        {
            /************************************************************************/
            /* WebDAV [D:] (DAV:)
            /************************************************************************/
            //attributes
            const TiXmlElement *xmlResType = xmlProps->FirstChildElement((davNamespace + "resourcetype").c_str());
            if (xmlResType && xmlResType->FirstChildElement((glDavNs + "collection").c_str()))
            {
                item.Attributes = FILE_ATTRIBUTE_DIRECTORY;
            }

            //size
            const TiXmlElement *xmlSize = xmlProps->FirstChildElement((davNamespace + "getcontentlength").c_str());
            if (xmlSize && xmlSize->GetText())
            {
                item.Size = _atoi64(xmlSize->GetText());
            }

            //creation datetime
            const TiXmlElement *xmlCrDate = xmlProps->FirstChildElement((davNamespace + "creationdate").c_str());
            if (xmlCrDate && xmlCrDate->GetText())
            {
                item.Created = ParseDateTime(xmlCrDate->GetText());
            }

            //last modified datetime
            const TiXmlElement *xmlLmDate = xmlProps->FirstChildElement((davNamespace + "getlastmodified").c_str());
            if (xmlLmDate && xmlLmDate->GetText())
            {
                item.Modified = ParseDateTime(xmlLmDate->GetText());
            }

            /************************************************************************/
            /* Win32 [Z:] (urn:schemas-microsoft-com)
            /************************************************************************/
            //last access datetime
            // TODO: process D:creationdate D:getlastmodified
            const TiXmlElement *xmlLaDate = xmlProps->FirstChildElement((msNamespace + "Win32LastAccessTime").c_str());
            if (xmlLaDate && xmlLaDate->GetText())
            {
                item.LastAccess = ParseDateTime(xmlLaDate->GetText());
            }

            //attributes
            const TiXmlElement *xmlAttr = xmlProps->FirstChildElement((msNamespace + "Win32FileAttributes").c_str());
            if (xmlAttr && xmlAttr->GetText())
            {
                DWORD attr = 0;
                sscanf_s(xmlAttr->GetText(), "%x", &attr);
                item.Attributes |= attr;
            }
        }
        wdavItems.push_back(item);
    }

    *itemsNum = static_cast<int>(wdavItems.size());
    if (*itemsNum)
    {
        *items = new PluginPanelItem[*itemsNum];
        ZeroMemory(*items, sizeof(PluginPanelItem) * (*itemsNum));
        for (int i = 0; i < *itemsNum; ++i)
        {
            PluginPanelItem &farItem = (*items)[i];
            const size_t nameSize = wdavItems[i].Name.length() + 1;
            wchar_t *name = new wchar_t[nameSize];
            wcscpy_s(name, nameSize, wdavItems[i].Name.c_str());
            farItem.FindData.lpwszFileName = name;
            farItem.FindData.dwFileAttributes = wdavItems[i].Attributes;
            if ((farItem.FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
            {
                farItem.FindData.nFileSize = wdavItems[i].Size;
            }
            farItem.FindData.ftCreationTime = wdavItems[i].Created;
            farItem.FindData.ftLastWriteTime = wdavItems[i].Modified;
            farItem.FindData.ftLastAccessTime = wdavItems[i].LastAccess;
        }
    }

    return true;
}


bool THTTPFileSystem::GetFile(const wchar_t *remotePath, const wchar_t *localPath, const unsigned __int64 /*fileSize*/, std::wstring &errorInfo)
{
    // DEBUG_PRINTF(L"THTTPFileSystem::GetFile: remotePath = %s, localPath = %s", remotePath, localPath);
    assert(localPath && *localPath);

    CNBFile outFile;
    if (!outFile.OpenWrite(localPath))
    {
        errorInfo = FormatErrorDescription(outFile.LastError());
        // DEBUG_PRINTF(L"THTTPFileSystem::GetFile: errorInfo = %s", errorInfo.c_str());
        return false;
    }

    const std::string webDavPath = EscapeUTF8URL(remotePath);
    // DEBUG_PRINTF(L"THTTPFileSystem::GetFile: webDavPath = %s", ::MB2W(webDavPath.c_str()).c_str());

    CURLcode urlCode = CURLPrepare(webDavPath.c_str(), false);
    CSlistURL slist;
    slist.Append("Content-Type: text/xml; charset=\"utf-8\"");
    slist.Append("Content-Length: 0");
    slist.Append("Connection: Keep-Alive");
    CHECK_CUCALL(urlCode, FCURLIntf->SetSlist(slist));
    CHECK_CUCALL(urlCode, FCURLIntf->SetOutput(&outFile, &m_ProgressPercent));
    CHECK_CUCALL(urlCode, FCURLIntf->Perform());

    outFile.Close();
    m_ProgressPercent = -1;

    if (urlCode != CURLE_OK)
    {
        errorInfo = ::MB2W(curl_easy_strerror(urlCode));
        // DEBUG_PRINTF(L"THTTPFileSystem::GetFile: errorInfo = %s", errorInfo.c_str());
        return false;
    }

    bool result = CheckResponseCode(HTTP_STATUS_OK, HTTP_STATUS_NO_CONTENT, errorInfo);
    // DEBUG_PRINTF(L"THTTPFileSystem::GetFile: result = %d, errorInfo = %s", result, errorInfo.c_str());
    return result;
}


bool THTTPFileSystem::PutFile(const wchar_t *remotePath, const wchar_t *localPath, const unsigned __int64 /*fileSize*/, std::wstring &errorInfo)
{
    // DEBUG_PRINTF(L"THTTPFileSystem::PutFile: remotePath = %s, localPath = %s", remotePath, localPath);
    assert(localPath && *localPath);

    CNBFile inFile;
    if (!inFile.OpenRead(localPath))
    {
        errorInfo = FormatErrorDescription(inFile.LastError());
        return false;
    }

    const std::string webDavPath = EscapeUTF8URL(remotePath);
    CURLcode urlCode = CURLPrepare(webDavPath.c_str(), false);
    CSlistURL slist;
    slist.Append("Expect:");    //Expect: 100-continue is not wanted
    slist.Append("Connection: Keep-Alive");
    CHECK_CUCALL(urlCode, FCURLIntf->SetSlist(slist));
    CHECK_CUCALL(urlCode, FCURLIntf->SetInput(&inFile, &m_ProgressPercent));
    CHECK_CUCALL(urlCode, FCURLIntf->Perform());

    inFile.Close();
    m_ProgressPercent = -1;

    if (urlCode != CURLE_OK)
    {
        errorInfo = ::MB2W(curl_easy_strerror(urlCode));
        return false;
    }

    return CheckResponseCode(HTTP_STATUS_CREATED, HTTP_STATUS_NO_CONTENT, errorInfo);
}


bool THTTPFileSystem::Rename(const wchar_t *srcPath, const wchar_t *dstPath, const ItemType /*type*/, std::wstring &errorInfo)
{
    const std::string srcWebDavPath = EscapeUTF8URL(srcPath);
    const std::string dstWebDavPath = EscapeUTF8URL(dstPath);

    CURLcode urlCode = CURLPrepare(srcWebDavPath.c_str());
    CSlistURL slist;
    slist.Append("Depth: infinity");
    slist.Append("Content-Type: text/xml; charset=\"utf-8\"");
    slist.Append("Content-Length: 0");
    std::string dstParam = "Destination: ";
    dstParam += FCURLIntf->GetTopURL();
    dstParam += dstWebDavPath;
    slist.Append(dstParam.c_str());
    slist.Append("Connection: Keep-Alive");
    CHECK_CUCALL(urlCode, FCURLIntf->SetSlist(slist));
    CHECK_CUCALL(urlCode, curl_easy_setopt(FCURLIntf->GetCURL(), CURLOPT_CUSTOMREQUEST, "MOVE"));

    CHECK_CUCALL(urlCode, FCURLIntf->Perform());

    if (urlCode != CURLE_OK)
    {
        errorInfo = ::MB2W(curl_easy_strerror(urlCode));
        return false;
    }

    return CheckResponseCode(HTTP_STATUS_CREATED, HTTP_STATUS_NO_CONTENT, errorInfo);
}

bool THTTPFileSystem::Delete(const wchar_t *path, const ItemType /*type*/, std::wstring &errorInfo)
{
    const std::string webDavPath = EscapeUTF8URL(path);

    CURLcode urlCode = CURLPrepare(webDavPath.c_str());
    CSlistURL slist;
    slist.Append("Content-Type: text/xml; charset=\"utf-8\"");
    slist.Append("Content-Length: 0");
    slist.Append("Connection: Keep-Alive");
    CHECK_CUCALL(urlCode, FCURLIntf->SetSlist(slist));
    CHECK_CUCALL(urlCode, curl_easy_setopt(FCURLIntf->GetCURL(), CURLOPT_CUSTOMREQUEST, "DELETE"));

    CHECK_CUCALL(urlCode, FCURLIntf->Perform());
    if (urlCode != CURLE_OK)
    {
        errorInfo = ::MB2W(curl_easy_strerror(urlCode));
        return false;
    }

    return CheckResponseCode(HTTP_STATUS_OK, HTTP_STATUS_NO_CONTENT, errorInfo);
}

std::wstring THTTPFileSystem::FormatErrorDescription(const DWORD errCode, const wchar_t *info) const
{
    assert(errCode || info);

    std::wstring errDescr;
    if (info)
    {
        errDescr = info;
    }
    if (errCode)
    {
        if (!errDescr.empty())
        {
            errDescr += L'\n';
        }
        errDescr += GetSystemErrorMessage(errCode);
    }
    return errDescr;
}
