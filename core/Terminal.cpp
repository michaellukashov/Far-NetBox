//---------------------------------------------------------------------------
#include "stdafx.h"

#include "boostdefines.hpp"
#include <boost/scope_exit.hpp>
#include <boost/bind.hpp>

#include "Terminal.h"

// #include <SysUtils.hpp>
// #include <FileCtrl.hpp>

#include "Common.h"
#include "PuttyTools.h"
#include "FileBuffer.h"
#include "Interface.h"
#include "RemoteFiles.h"
#include "SecureShell.h"
#include "ScpFileSystem.h"
#include "SftpFileSystem.h"
#ifndef NO_FILEZILLA
#include "FtpFileSystem.h"
#endif
#include "TextsCore.h"
#include "HelpCore.h"
#include "CoreMain.h"
#include "Queue.h"
#include "Common.h"

#ifndef AUTO_WINSOCK
#include <winsock2.h>
#endif
//---------------------------------------------------------------------------
#define COMMAND_ERROR_ARI(MESSAGE, REPEAT) \
  { \
    int Result = CommandError(&E, MESSAGE, qaRetry | qaSkip | qaAbort); \
    switch (Result) { \
      case qaRetry: { REPEAT; } break; \
      case qaAbort: Abort(); \
    } \
  }
//---------------------------------------------------------------------------
// Note that the action may already be canceled when RollbackAction is called
#define COMMAND_ERROR_ARI_ACTION(MESSAGE, REPEAT, ACTION) \
  { \
    int Result; \
    try \
    { \
      Result = CommandError(&E, MESSAGE, qaRetry | qaSkip | qaAbort); \
    } \
    catch (const std::exception & E2) \
    { \
      RollbackAction(ACTION, NULL, &E2); \
      throw; \
    } \
    switch (Result) { \
      case qaRetry: ACTION.Cancel(); { REPEAT; } break; \
      case qaAbort: RollbackAction(ACTION, NULL, &E); Abort(); \
      case qaSkip:  ACTION.Cancel(); break; \
      default: assert(false); \
    } \
  }

#define FILE_OPERATION_LOOP_EX(ALLOW_SKIP, MESSAGE, OPERATION) \
  FILE_OPERATION_LOOP_CUSTOM(Self, ALLOW_SKIP, MESSAGE, OPERATION)
//---------------------------------------------------------------------------
struct TMoveFileParams
{
  std::wstring Target;
  std::wstring FileMask;
};
//---------------------------------------------------------------------------
struct TFilesFindParams
{
  TFileMasks FileMask;
  TFileFoundEvent OnFileFound;
  TFindingFileEvent OnFindingFile;
  bool Cancel;
};
//---------------------------------------------------------------------------
TCalculateSizeStats::TCalculateSizeStats()
{
  memset(this, 0, sizeof(*this));
}
//---------------------------------------------------------------------------
TSynchronizeOptions::TSynchronizeOptions()
{
  memset(this, 0, sizeof(*this));
}
//---------------------------------------------------------------------------
TSynchronizeOptions::~TSynchronizeOptions()
{
  delete Filter;
}
//---------------------------------------------------------------------------
TSpaceAvailable::TSpaceAvailable()
{
  memset(this, 0, sizeof(*this));
}
//---------------------------------------------------------------------------
TOverwriteFileParams::TOverwriteFileParams()
{
  SourceSize = 0;
  DestSize = 0;
  SourcePrecision = mfFull;
  DestPrecision = mfFull;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TSynchronizeChecklist::TItem::TItem() :
  Action(saNone), IsDirectory(false), RemoteFile(NULL), Checked(true), ImageIndex(-1)
{
  Local.ModificationFmt = mfFull;
  Local.Modification = 0;
  Local.Size = 0;
  Remote.ModificationFmt = mfFull;
  Remote.Modification = 0;
  Remote.Size = 0;
}
//---------------------------------------------------------------------------
TSynchronizeChecklist::TItem::~TItem()
{
  delete RemoteFile;
}
//---------------------------------------------------------------------------
const std::wstring& TSynchronizeChecklist::TItem::GetFileName() const
{
  if (!Remote.FileName.empty())
  {
    return Remote.FileName;
  }
  else
  {
    assert(!Local.FileName.empty());
    return Local.FileName;
  }
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TSynchronizeChecklist::TSynchronizeChecklist() :
  FList(new TList())
{
}
//---------------------------------------------------------------------------
TSynchronizeChecklist::~TSynchronizeChecklist()
{
  for (int Index = 0; Index < FList->GetCount(); Index++)
  {
    delete reinterpret_cast<TItem *>(FList->GetItem(Index));
  }
  delete FList;
}
//---------------------------------------------------------------------------
void TSynchronizeChecklist::Add(TItem * Item)
{
  FList->Add((TObject *)Item);
}
//---------------------------------------------------------------------------
int TSynchronizeChecklist::Compare(void * AItem1, void * AItem2)
{
  TItem * Item1 = reinterpret_cast<TItem *>(AItem1);
  TItem * Item2 = reinterpret_cast<TItem *>(AItem2);

  int Result;
  if (!Item1->Local.Directory.empty())
  {
    Result = CompareText(Item1->Local.Directory, Item2->Local.Directory);
  }
  else
  {
    assert(!Item1->Remote.Directory.empty());
    Result = CompareText(Item1->Remote.Directory, Item2->Remote.Directory);
  }

  if (Result == 0)
  {
    Result = CompareText(Item1->GetFileName(), Item2->GetFileName());
  }

  return Result;
}
//---------------------------------------------------------------------------
void TSynchronizeChecklist::Sort()
{
  FList->Sort(Compare);
}
//---------------------------------------------------------------------------
int TSynchronizeChecklist::GetCount() const
{
  return FList->GetCount();
}
//---------------------------------------------------------------------------
const TSynchronizeChecklist::TItem * TSynchronizeChecklist::GetItem(int Index) const
{
  return reinterpret_cast<TItem *>(FList->GetItem(Index));
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
class TTunnelThread : public TSimpleThread
{
public:
  TTunnelThread(TSecureShell * SecureShell);
  virtual ~TTunnelThread();

  virtual void Terminate();

protected:
  virtual void Execute();

private:
  TSecureShell * FSecureShell;
  bool FTerminated;
  TTunnelThread *Self;
};
//---------------------------------------------------------------------------
TTunnelThread::TTunnelThread(TSecureShell * SecureShell) :
  FSecureShell(SecureShell),
  FTerminated(false)
{
  Self = this;
  Start();
}
//---------------------------------------------------------------------------
TTunnelThread::~TTunnelThread()
{
  // close before the class's virtual functions (Terminate particularly) are lost
  Close();
}
//---------------------------------------------------------------------------
void TTunnelThread::Terminate()
{
  FTerminated = true;
}
//---------------------------------------------------------------------------
void TTunnelThread::Execute()
{
  try
  {
    BOOST_SCOPE_EXIT ( (&Self) )
    {
        if (Self->FSecureShell->GetActive())
        {
          Self->FSecureShell->Close();
        }
    } BOOST_SCOPE_EXIT_END
    while (!FTerminated)
    {
      FSecureShell->Idle(250);
    }
  }
  catch (...)
  {
    // do not pass std::exception out of thread's proc
  }
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
class TTunnelUI : public TSessionUI
{
public:
  TTunnelUI(TTerminal * Terminal);
  virtual void Information(const std::wstring & Str, bool Status);
  virtual int QueryUser(const std::wstring Query,
    TStrings * MoreMessages, int Answers, const TQueryParams * Params,
    TQueryType QueryType);
  virtual int QueryUserException(const std::wstring Query,
    const std::exception * E, int Answers, const TQueryParams * Params,
    TQueryType QueryType);
  virtual bool PromptUser(TSessionData * Data, TPromptKind Kind,
    std::wstring Name, std::wstring Instructions, TStrings * Prompts,
    TStrings * Results);
  virtual void DisplayBanner(const std::wstring & Banner);
  virtual void FatalError(const std::exception * E, std::wstring Msg);
  virtual void HandleExtendedException(const std::exception * E);
  virtual void Closed();

private:
  TTerminal * FTerminal;
  unsigned int FTerminalThread;
};
//---------------------------------------------------------------------------
TTunnelUI::TTunnelUI(TTerminal * Terminal)
{
  FTerminal = Terminal;
  FTerminalThread = GetCurrentThreadId();
}
//---------------------------------------------------------------------------
void TTunnelUI::Information(const std::wstring & Str, bool Status)
{
  if (GetCurrentThreadId() == FTerminalThread)
  {
    FTerminal->Information(Str, Status);
  }
}
//---------------------------------------------------------------------------
int TTunnelUI::QueryUser(const std::wstring Query,
  TStrings * MoreMessages, int Answers, const TQueryParams * Params,
  TQueryType QueryType)
{
  int Result;
  if (GetCurrentThreadId() == FTerminalThread)
  {
    Result = FTerminal->QueryUser(Query, MoreMessages, Answers, Params, QueryType);
  }
  else
  {
    Result = AbortAnswer(Answers);
  }
  return Result;
}
//---------------------------------------------------------------------------
int TTunnelUI::QueryUserException(const std::wstring Query,
  const std::exception * E, int Answers, const TQueryParams * Params,
  TQueryType QueryType)
{
  int Result;
  if (GetCurrentThreadId() == FTerminalThread)
  {
    Result = FTerminal->QueryUserException(Query, E, Answers, Params, QueryType);
  }
  else
  {
    Result = AbortAnswer(Answers);
  }
  return Result;
}
//---------------------------------------------------------------------------
bool TTunnelUI::PromptUser(TSessionData * Data, TPromptKind Kind,
  std::wstring Name, std::wstring Instructions, TStrings * Prompts, TStrings * Results)
{
  bool Result;
  if (GetCurrentThreadId() == FTerminalThread)
  {
    if (IsAuthenticationPrompt(Kind))
    {
      Instructions = LoadStr(TUNNEL_INSTRUCTION) +
        (Instructions.empty() ? L"" : L"\n") +
        Instructions;
    }

    Result = FTerminal->PromptUser(Data, Kind, Name, Instructions, Prompts, Results);
  }
  else
  {
    Result = false;
  }
  return Result;
}
//---------------------------------------------------------------------------
void TTunnelUI::DisplayBanner(const std::wstring & Banner)
{
  if (GetCurrentThreadId() == FTerminalThread)
  {
    FTerminal->DisplayBanner(Banner);
  }
}
//---------------------------------------------------------------------------
void TTunnelUI::FatalError(const std::exception * E, std::wstring Msg)
{
  throw ESshFatal(E, Msg);
}
//---------------------------------------------------------------------------
void TTunnelUI::HandleExtendedException(const std::exception * E)
{
  if (GetCurrentThreadId() == FTerminalThread)
  {
    FTerminal->HandleExtendedException(E);
  }
}
//---------------------------------------------------------------------------
void TTunnelUI::Closed()
{
  // noop
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
class TCallbackGuard
{
public:
  inline TCallbackGuard(TTerminal * FTerminal);
  inline ~TCallbackGuard();

  void FatalError(const std::exception * E, const std::wstring & Msg);
  inline void Verify();
  void Dismiss();

private:
  ExtException * FFatalError;
  TTerminal * FTerminal;
  bool FGuarding;
};
//---------------------------------------------------------------------------
TCallbackGuard::TCallbackGuard(TTerminal * Terminal) :
  FTerminal(Terminal),
  FFatalError(NULL),
  FGuarding(FTerminal->FCallbackGuard == NULL)
{
  if (FGuarding)
  {
    FTerminal->FCallbackGuard = this;
  }
}
//---------------------------------------------------------------------------
TCallbackGuard::~TCallbackGuard()
{
  if (FGuarding)
  {
    assert((FTerminal->FCallbackGuard == this) || (FTerminal->FCallbackGuard == NULL));
    FTerminal->FCallbackGuard = NULL;
  }

  delete FFatalError;
}
//---------------------------------------------------------------------------
class ECallbackGuardAbort : public EAbort
{
public:
  ECallbackGuardAbort() : EAbort("")
  {
  }
};
//---------------------------------------------------------------------------
void TCallbackGuard::FatalError(const std::exception * E, const std::wstring & Msg)
{
  assert(FGuarding);

  // make sure we do not bother about getting back the silent abort std::exception
  // we issued outselves. this may happen when there is an std::exception handler
  // that converts any std::exception to fatal one (such as in TTerminal::Open).
  if (dynamic_cast<const ECallbackGuardAbort *>(E) == NULL)
  {
    assert(FFatalError == NULL);

    FFatalError = new ExtException(E, Msg);
  }

  // silently abort what we are doing.
  // non-silent std::exception would be catched probably by default application
  // std::exception handler, which may not do an appropriate action
  // (particularly it will not resume broken transfer).
  throw ECallbackGuardAbort();
}
//---------------------------------------------------------------------------
void TCallbackGuard::Dismiss()
{
  assert(FFatalError == NULL);
  FGuarding = false;
}
//---------------------------------------------------------------------------
void TCallbackGuard::Verify()
{
  if (FGuarding)
  {
    FGuarding = false;
    assert(FTerminal->FCallbackGuard == this);
    FTerminal->FCallbackGuard = NULL;

    if (FFatalError != NULL)
    {
      throw ESshFatal(FFatalError, L"");
    }
  }
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TTerminal::TTerminal(TSessionData * SessionData,
  TConfiguration * Configuration)
{
  FConfiguration = Configuration;
  FSessionData = new TSessionData(L"");
  FSessionData->Assign(SessionData);
  FLog = new TSessionLog(this, FSessionData, Configuration);
  FFiles = new TRemoteDirectory(this);
  FExceptionOnFail = 0;
  FInTransaction = 0;
  FReadCurrentDirectoryPending = false;
  FReadDirectoryPending = false;
  FUsersGroupsLookedup = false;
  FTunnelLocalPortNumber = 0;
  FFileSystem = NULL;
  FSecureShell = NULL;
  FOnDeleteLocalFile = NULL;
  FOnReadDirectoryProgress = NULL;
  FOnDisplayBanner = NULL;
  FOnShowExtendedException = NULL;
  FOnInformation = NULL;
  FOnFindingFile = NULL;

  FUseBusyCursor = true;
  FLockDirectory = L"";
  FDirectoryCache = new TRemoteDirectoryCache();
  FDirectoryChangesCache = NULL;
  FFSProtocol = cfsUnknown;
  FCommandSession = NULL;
  FAutoReadDirectory = true;
  FReadingCurrentDirectory = false;
  FStatus = ssClosed;
  FTunnelThread = NULL;
  FTunnel = NULL;
  FTunnelData = NULL;
  FTunnelLog = NULL;
  FTunnelUI = NULL;
  FTunnelOpening = false;
  FCallbackGuard = NULL;
  Self = this;
}
//---------------------------------------------------------------------------
TTerminal::~TTerminal()
{
  if (GetActive())
  {
    Close();
  }

  if (FCallbackGuard != NULL)
  {
    // see TTerminal::HandleExtendedException
    FCallbackGuard->Dismiss();
  }
  assert(FTunnel == NULL);

  SAFE_DESTROY(FCommandSession);

  if (GetSessionData()->GetCacheDirectoryChanges() && GetSessionData()->GetPreserveDirectoryChanges() &&
      (FDirectoryChangesCache != NULL))
  {
    Configuration->SaveDirectoryChangesCache(GetSessionData()->GetSessionKey(),
      FDirectoryChangesCache);
  }

  SAFE_DESTROY_EX(TCustomFileSystem, FFileSystem);
  SAFE_DESTROY_EX(TSessionLog, FLog);
  delete FFiles;
  delete FDirectoryCache;
  delete FDirectoryChangesCache;
  SAFE_DESTROY(FSessionData);
}
//---------------------------------------------------------------------------
void TTerminal::Idle()
{
  // once we disconnect, do nothing, until reconnect handler
  // "receives the information"
  if (GetActive())
  {
    if (Configuration->GetActualLogProtocol() >= 1)
    {
      LogEvent(L"Session upkeep");
    }

    assert(FFileSystem != NULL);
    FFileSystem->Idle();

    if (GetCommandSessionOpened())
    {
      try
      {
        FCommandSession->Idle();
      }
      catch (const std::exception & E)
      {
        // If the secondary session is dropped, ignore the error and let
        // it be reconnected when needed.
        // BTW, non-fatal error can hardly happen here, that's why
        // it is displayed, because it can be useful to know.
        if (FCommandSession->GetActive())
        {
          FCommandSession->HandleExtendedException(&E);
        }
      }
    }
  }
}
//---------------------------------------------------------------------
std::wstring TTerminal::EncryptPassword(const std::wstring & Password)
{
  return Configuration->EncryptPassword(Password, GetSessionData()->GetSessionName());
}
//---------------------------------------------------------------------
std::wstring TTerminal::DecryptPassword(const std::wstring & Password)
{
  std::wstring Result;
  try
  {
    Result = Configuration->DecryptPassword(Password, GetSessionData()->GetSessionName());
  }
  catch (const EAbort &)
  {
    // silently ignore aborted prompts for master password and return empty password
  }
  return Result;
}
//---------------------------------------------------------------------------
void TTerminal::RecryptPasswords()
{
  FSessionData->RecryptPasswords();
  FPassword = EncryptPassword(DecryptPassword(FPassword));
  FTunnelPassword = EncryptPassword(DecryptPassword(FTunnelPassword));
}
//---------------------------------------------------------------------------
bool TTerminal::IsAbsolutePath(const std::wstring Path)
{
  return !Path.empty() && Path[1] == '/';
}
//---------------------------------------------------------------------------
std::wstring TTerminal::ExpandFileName(std::wstring Path,
  const std::wstring BasePath)
{
  Path = UnixExcludeTrailingBackslash(Path);
  if (!IsAbsolutePath(Path) && !BasePath.empty())
  {
    // TODO: Handle more complicated cases like "../../xxx"
    if (Path == L"..")
    {
      Path = UnixExcludeTrailingBackslash(UnixExtractFilePath(
        UnixExcludeTrailingBackslash(BasePath)));
    }
    else
    {
      Path = UnixIncludeTrailingBackslash(BasePath) + Path;
    }
  }
  return Path;
}
//---------------------------------------------------------------------------
bool TTerminal::GetActive()
{
  return (FFileSystem != NULL) && FFileSystem->GetActive();
}
//---------------------------------------------------------------------------
void TTerminal::Close()
{
  FFileSystem->Close();

  if (GetCommandSessionOpened())
  {
    FCommandSession->Close();
  }
}
//---------------------------------------------------------------------------
void TTerminal::ResetConnection()
{
  FAnyInformation = false;
  // used to be called from Reopen(), why?
  FTunnelError = L"";

  if (FDirectoryChangesCache != NULL)
  {
    delete FDirectoryChangesCache;
    FDirectoryChangesCache = NULL;
  }

  FFiles->SetDirectory(L"");
  // note that we cannot clear contained files
  // as they can still be referenced in the GUI atm
}
//---------------------------------------------------------------------------
void TTerminal::Open()
{
  FLog->ReflectSettings();
  try
  {
    {
      BOOST_SCOPE_EXIT ( (&Self) )
      {
        // Prevent calling Information with active=false unless there was at least
        // one call with active=true
        if (Self->FAnyInformation)
        {
          Self->DoInformation(L"", true, false);
        }
      } BOOST_SCOPE_EXIT_END
      try
      {
        ResetConnection();
        FStatus = ssOpening;

        {
          BOOST_SCOPE_EXIT ( (&Self) )
          {
            if (Self->FSessionData->GetTunnel())
            {
              Self->FSessionData->RollbackTunnel();
            }
          } BOOST_SCOPE_EXIT_END
        
          if (FFileSystem == NULL)
          {
            GetLog()->AddStartupInfo();
          }

          assert(FTunnel == NULL);
          if (FSessionData->GetTunnel())
          {
            DoInformation(LoadStr(OPEN_TUNNEL), true);
            LogEvent(L"Opening tunnel.");
            OpenTunnel();
            GetLog()->AddSeparator();

            FSessionData->ConfigureTunnel(FTunnelLocalPortNumber);

            DoInformation(LoadStr(USING_TUNNEL), false);
            LogEvent(FORMAT(L"Connecting via tunnel interface %s:%d.",
              FSessionData->GetHostName().c_str(), FSessionData->GetPortNumber()));
          }
          else
          {
            assert(FTunnelLocalPortNumber == 0);
          }

          if (FFileSystem == NULL)
          {
            if (GetSessionData()->GetFSProtocol() == fsFTP)
            {
              #ifdef NO_FILEZILLA
              LogEvent(L"FTP protocol is not supported by this build.");
              FatalError(NULL, LoadStr(FTP_UNSUPPORTED));
              #else
              FFSProtocol = cfsFTP;
              FFileSystem = new TFTPFileSystem(this);
              FFileSystem->Open();
              GetLog()->AddSeparator();
              LogEvent(L"Using FTP protocol.");
              #endif
            }
            else
            {
              assert(FSecureShell == NULL);
              {
                BOOST_SCOPE_EXIT ( (&Self) )
                {
                  delete Self->FSecureShell;
                  Self->FSecureShell = NULL;
                } BOOST_SCOPE_EXIT_END
                FSecureShell = new TSecureShell(this, FSessionData, GetLog(), Configuration);
                try
                {
                  // there will be only one channel in this session
                  FSecureShell->SetSimple(true);
                  FSecureShell->Open();
                }
                catch (const std::exception & E)
                {
                  assert(!FSecureShell->GetActive());
                  if (!FSecureShell->GetActive() && !FTunnelError.empty())
                  {
                    // the only case where we expect this to happen
                    // FIXME assert(E.Message == LoadStr(UNEXPECTED_CLOSE_ERROR));
                    FatalError(&E, FMTLOAD(TUNNEL_ERROR, FTunnelError.c_str()));
                  }
                  else
                  {
                    throw;
                  }
                }

                GetLog()->AddSeparator();

                if ((GetSessionData()->GetFSProtocol() == fsSCPonly) ||
                    (GetSessionData()->GetFSProtocol() == fsSFTP && FSecureShell->SshFallbackCmd()))
                {
                  FFSProtocol = cfsSCP;
                  FFileSystem = new TSCPFileSystem(this, FSecureShell);
                  FSecureShell = NULL; // ownership passed
                  LogEvent(L"Using SCP protocol.");
                }
                else
                {
                  FFSProtocol = cfsSFTP;
                  FFileSystem = new TSFTPFileSystem(this, FSecureShell);
                  FSecureShell = NULL; // ownership passed
                  LogEvent(L"Using SFTP protocol.");
                }
              }
            }
          }
          else
          {
            FFileSystem->Open();
          }
        }

        if (GetSessionData()->GetCacheDirectoryChanges())
        {
          assert(FDirectoryChangesCache == NULL);
          FDirectoryChangesCache = new TRemoteDirectoryChangesCache(
            Configuration->GetCacheDirectoryChangesMaxSize());
          if (GetSessionData()->GetPreserveDirectoryChanges())
          {
            Configuration->LoadDirectoryChangesCache(GetSessionData()->GetSessionKey(),
              FDirectoryChangesCache);
          }
        }

        DoStartup();

        DoInformation(LoadStr(STATUS_READY), true);
        FStatus = ssOpened;
      }
      catch (...)
      {
        // rollback
        if (Self->FDirectoryChangesCache != NULL)
        {
          delete Self->FDirectoryChangesCache;
          Self->FDirectoryChangesCache = NULL;
        }
        throw;
      }
    }
  }
  catch (const EFatal &)
  {
    throw;
  }
  catch (const std::exception & E)
  {
    // any std::exception while opening session is fatal
    FatalError(&E, L"");
  }
}
//---------------------------------------------------------------------------
bool TTerminal::IsListenerFree(unsigned int PortNumber)
{
  SOCKET Socket = socket(AF_INET, SOCK_STREAM, 0);
  bool Result = (Socket != INVALID_SOCKET);
  if (Result)
  {
    SOCKADDR_IN Address;

    memset(&Address, 0, sizeof(Address));
    Address.sin_family = AF_INET;
    Address.sin_port = htons(static_cast<short>(PortNumber));
    Address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    Result = (::bind(Socket, reinterpret_cast<sockaddr *>(&Address), sizeof(Address)) == 0);
    closesocket(Socket);
  }
  return Result;
}
//---------------------------------------------------------------------------
void TTerminal::OpenTunnel()
{
  assert(FTunnelData == NULL);

  FTunnelLocalPortNumber = FSessionData->GetTunnelLocalPortNumber();
  if (FTunnelLocalPortNumber == 0)
  {
    FTunnelLocalPortNumber = Configuration->GetTunnelLocalPortNumberLow();
    while (!IsListenerFree(FTunnelLocalPortNumber))
    {
      FTunnelLocalPortNumber++;
      if (FTunnelLocalPortNumber > Configuration->GetTunnelLocalPortNumberHigh())
      {
        FTunnelLocalPortNumber = 0;
        FatalError(NULL, FMTLOAD(TUNNEL_NO_FREE_PORT,
          Configuration->GetTunnelLocalPortNumberLow(), Configuration->GetTunnelLocalPortNumberHigh()));
      }
    }
    LogEvent(FORMAT(L"Autoselected tunnel local port number %d", FTunnelLocalPortNumber));
  }

  try
  {
    FTunnelData = new TSessionData(L"");
    FTunnelData->Assign(StoredSessions->GetDefaultSettings());
    FTunnelData->Name = FMTLOAD(TUNNEL_SESSION_NAME, FSessionData->GetSessionName().c_str());
    FTunnelData->SetTunnel(false);
    FTunnelData->SetHostName(FSessionData->GetTunnelHostName());
    FTunnelData->SetPortNumber(FSessionData->GetTunnelPortNumber());
    FTunnelData->SetUserName(FSessionData->GetTunnelUserName());
    FTunnelData->SetPassword(FSessionData->GetTunnelPassword());
    FTunnelData->SetPublicKeyFile(FSessionData->GetTunnelPublicKeyFile());
    FTunnelData->SetTunnelPortFwd(FORMAT(L"L%d\t%s:%d",
      FTunnelLocalPortNumber, FSessionData->GetHostName().c_str(), FSessionData->GetPortNumber()));
    FTunnelData->SetProxyMethod(FSessionData->GetProxyMethod());
    FTunnelData->SetProxyHost(FSessionData->GetProxyHost());
    FTunnelData->SetProxyPort(FSessionData->GetProxyPort());
    FTunnelData->SetProxyUsername(FSessionData->GetProxyUsername());
    FTunnelData->SetProxyPassword(FSessionData->GetProxyPassword());
    FTunnelData->SetProxyTelnetCommand(FSessionData->GetProxyTelnetCommand());
    FTunnelData->SetProxyLocalCommand(FSessionData->GetProxyLocalCommand());
    FTunnelData->SetProxyDNS(FSessionData->GetProxyDNS());
    FTunnelData->SetProxyLocalhost(FSessionData->GetProxyLocalhost());

    FTunnelLog = new TSessionLog(this, FTunnelData, Configuration);
    FTunnelLog->SetParent(FLog);
    FTunnelLog->SetName(L"Tunnel");
    FTunnelLog->ReflectSettings();
    FTunnelUI = new TTunnelUI(this);
    FTunnel = new TSecureShell(FTunnelUI, FTunnelData, FTunnelLog, Configuration);

    FTunnelOpening = true;
    {
      BOOST_SCOPE_EXIT ( (&Self) )
      {
        Self->FTunnelOpening = false;
      } BOOST_SCOPE_EXIT_END
      FTunnel->Open();
    }

    FTunnelThread = new TTunnelThread(FTunnel);
  }
  catch (...)
  {
    CloseTunnel();
    throw;
  }
}
//---------------------------------------------------------------------------
void TTerminal::CloseTunnel()
{
  SAFE_DESTROY_EX(TTunnelThread, FTunnelThread);
  FTunnelError = FTunnel->GetLastTunnelError();
  SAFE_DESTROY_EX(TSecureShell, FTunnel);
  SAFE_DESTROY_EX(TTunnelUI, FTunnelUI);
  SAFE_DESTROY_EX(TSessionLog, FTunnelLog);
  SAFE_DESTROY(FTunnelData);

  FTunnelLocalPortNumber = 0;
}
//---------------------------------------------------------------------------
void TTerminal::Closed()
{
  if (FTunnel != NULL)
  {
     CloseTunnel();
  }

  if (GetOnClose().num_slots() > 0)
  {
    TCallbackGuard Guard(this);
    GetOnClose()(this);
    Guard.Verify();
  }

  FStatus = ssClosed;
}
//---------------------------------------------------------------------------
void TTerminal::Reopen(int Params)
{
  TFSProtocol OrigFSProtocol = GetSessionData()->GetFSProtocol();
  std::wstring PrevRemoteDirectory = GetSessionData()->GetRemoteDirectory();
  bool PrevReadCurrentDirectoryPending = FReadCurrentDirectoryPending;
  bool PrevReadDirectoryPending = FReadDirectoryPending;
  assert(!FSuspendTransaction);
  bool PrevAutoReadDirectory = FAutoReadDirectory;
  // here used to be a check for FExceptionOnFail being 0
  // but it can happen, e.g. when we are downloading file to execute it.
  // however I'm not sure why we mind having excaption-on-fail enabled here
  int PrevExceptionOnFail = FExceptionOnFail;
  {
    BOOST_SCOPE_EXIT ( (&Self) (PrevRemoteDirectory)
        (OrigFSProtocol) (PrevAutoReadDirectory) (PrevReadCurrentDirectoryPending)
        (PrevReadDirectoryPending) (PrevExceptionOnFail) )
    {
        Self->GetSessionData()->SetRemoteDirectory(PrevRemoteDirectory);
        Self->GetSessionData()->SetFSProtocol(OrigFSProtocol);
        Self->FAutoReadDirectory = PrevAutoReadDirectory;
        Self->FReadCurrentDirectoryPending = PrevReadCurrentDirectoryPending;
        Self->FReadDirectoryPending = PrevReadDirectoryPending;
        Self->FSuspendTransaction = false;
        Self->FExceptionOnFail = PrevExceptionOnFail;
    } BOOST_SCOPE_EXIT_END
    FReadCurrentDirectoryPending = false;
    FReadDirectoryPending = false;
    FSuspendTransaction = true;
    FExceptionOnFail = 0;
    // typically, we avoid reading directory, when there is operation ongoing,
    // for file list which may reference files from current directory
    if (FLAGSET(Params, ropNoReadDirectory))
    {
      SetAutoReadDirectory(false);
    }

    // only peek, we may not be connected at all atm,
    // so make sure we do not try retrieving current directory from the server
    // (particularly with FTP)
    std::wstring ACurrentDirectory = PeekCurrentDirectory();
    if (!ACurrentDirectory.empty())
    {
      GetSessionData()->SetRemoteDirectory(ACurrentDirectory);
    }
    if (GetSessionData()->GetFSProtocol() == fsSFTP)
    {
      GetSessionData()->SetFSProtocol((FFSProtocol == cfsSCP ? fsSCPonly : fsSFTPonly));
    }

    if (GetActive())
    {
      Close();
    }

    Open();
  }
}
//---------------------------------------------------------------------------
bool TTerminal::PromptUser(TSessionData * Data, TPromptKind Kind,
  std::wstring Name, std::wstring Instructions, std::wstring Prompt, bool Echo, int MaxLen, std::wstring & Result)
{
  bool AResult;
  TStringList Prompts;
  TStringList Results;
  {
    Prompts.AddObject(Prompt, (TObject *)Echo);
    Results.AddObject(Result, (TObject *)MaxLen);

    AResult = PromptUser(Data, Kind, Name, Instructions, &Prompts, &Results);

    Result = Results.GetString(0);
  }

  return AResult;
}
//---------------------------------------------------------------------------
bool TTerminal::PromptUser(TSessionData * Data, TPromptKind Kind,
  std::wstring Name, std::wstring Instructions, TStrings * Prompts, TStrings * Results)
{
  // If PromptUser is overriden in descendant class, the overriden version
  // is not called when accessed via TSessionIU interface.
  // So this is workaround.
  return DoPromptUser(Data, Kind, Name, Instructions, Prompts, Results);
}
//---------------------------------------------------------------------------
bool TTerminal::DoPromptUser(TSessionData * /*Data*/, TPromptKind Kind,
  std::wstring Name, std::wstring Instructions, TStrings * Prompts, TStrings * Results)
{
  bool AResult = false;

  if (!GetOnPromptUser().empty())
  {
    TCallbackGuard Guard(this);
    GetOnPromptUser()(this, Kind, Name, Instructions, Prompts, Results, AResult, NULL);
    Guard.Verify();
  }

  if (AResult && (Configuration->GetRememberPassword()) &&
      (Prompts->GetCount() == 1) && !bool(Prompts->GetObject(0)) &&
      ((Kind == pkPassword) || (Kind == pkPassphrase) || (Kind == pkKeybInteractive) ||
       (Kind == pkTIS) || (Kind == pkCryptoCard)))
  {
    std::wstring EncryptedPassword = EncryptPassword(Results->GetString(0));
    if (FTunnelOpening)
    {
      FTunnelPassword = EncryptedPassword;
    }
    else
    {
      FPassword = EncryptedPassword;
    }
  }

  return AResult;
}
//---------------------------------------------------------------------------
int TTerminal::QueryUser(const std::wstring Query,
  TStrings * MoreMessages, int Answers, const TQueryParams * Params,
  TQueryType QueryType)
{
  LogEvent(FORMAT(L"Asking user:\n%s (%s)", Query.c_str(), (MoreMessages ? MoreMessages->GetCommaText().c_str() : std::wstring().c_str())));
  int Answer = AbortAnswer(Answers);
  if (!FOnQueryUser.empty())
  {
    TCallbackGuard Guard(this);
    FOnQueryUser(this, Query, MoreMessages, Answers, Params, Answer, QueryType, NULL);
    Guard.Verify();
  }
  return Answer;
}
//---------------------------------------------------------------------------
int TTerminal::QueryUserException(const std::wstring Query,
  const std::exception * E, int Answers, const TQueryParams * Params,
  TQueryType QueryType)
{
  int Result;
  TStringList MoreMessages;
  if (!std::string(E->what()).empty() && !Query.empty())
  {
    MoreMessages.Add(std::wstring(::MB2W(E->what())));
  }

  const ExtException *EE = dynamic_cast<const ExtException*>(E);
  if ((EE != NULL) && (EE->GetMoreMessages() != NULL))
  {
    MoreMessages.AddStrings(EE->GetMoreMessages());
  }
  Result = QueryUser(!Query.empty() ? Query : std::wstring(::MB2W(E->what())),
    MoreMessages.GetCount() ? &MoreMessages : NULL,
    Answers, Params, QueryType);
  return Result;
}
//---------------------------------------------------------------------------
void TTerminal::DisplayBanner(const std::wstring & Banner)
{
  if (GetOnDisplayBanner() != NULL)
  {
    if (Configuration->GetForceBanners() ||
        Configuration->ShowBanner(GetSessionData()->GetSessionKey(), Banner))
    {
      bool NeverShowAgain = false;
      int Options =
        FLAGMASK(Configuration->GetForceBanners(), boDisableNeverShowAgain);
      TCallbackGuard Guard(this);
      // FIXME OnDisplayBanner(this, GetSessionData()->GetSessionName(), Banner,
        // NeverShowAgain, Options);
      Guard.Verify();
      if (!Configuration->GetForceBanners() && NeverShowAgain)
      {
        Configuration->NeverShowBanner(GetSessionData()->GetSessionKey(), Banner);
      }
    }
  }
}
//---------------------------------------------------------------------------
void TTerminal::HandleExtendedException(const std::exception * E)
{
  GetLog()->AddException(E);
  if (GetOnShowExtendedException() != NULL)
  {
    TCallbackGuard Guard(this);
    // the event handler may destroy 'this' ...
    // FIXME OnShowExtendedException(this, E, NULL);
    // .. hence guard is dismissed from destructor, to make following call no-op
    Guard.Verify();
  }
}
//---------------------------------------------------------------------------
void TTerminal::ShowExtendedException(const std::exception * E)
{
  GetLog()->AddException(E);
  if (GetOnShowExtendedException() != NULL)
  {
    // FIXME OnShowExtendedException(this, E, NULL);
  }
}
//---------------------------------------------------------------------------
void TTerminal::DoInformation(const std::wstring & Str, bool Status,
  bool Active)
{
  if (Active)
  {
    FAnyInformation = true;
  }

  if (GetOnInformation())
  {
    TCallbackGuard Guard(this);
    // FIXME OnInformation(this, Str, Status, Active);
    Guard.Verify();
  }
}
//---------------------------------------------------------------------------
void TTerminal::Information(const std::wstring & Str, bool Status)
{
  DoInformation(Str, Status);
}
//---------------------------------------------------------------------------
void TTerminal::DoProgress(TFileOperationProgressType & ProgressData,
  TCancelStatus & Cancel)
{
  if (!GetOnProgress().empty())
  {
    TCallbackGuard Guard(this);
    GetOnProgress()(ProgressData, Cancel);
    Guard.Verify();
  }
}
//---------------------------------------------------------------------------
void TTerminal::DoFinished(TFileOperation Operation, TOperationSide Side, bool Temp,
  const std::wstring & FileName, bool Success, TOnceDoneOperation & OnceDoneOperation)
{
  if (!GetOnFinished().empty())
  {
    TCallbackGuard Guard(this);
    GetOnFinished()(Operation, Side, Temp, FileName, Success, OnceDoneOperation);
    Guard.Verify();
  }
}
//---------------------------------------------------------------------------
bool TTerminal::GetIsCapable(TFSCapability Capability) const
{
  assert(FFileSystem);
  return FFileSystem->IsCapable(Capability);
}
//---------------------------------------------------------------------------
std::wstring TTerminal::AbsolutePath(std::wstring Path, bool Local)
{
  return FFileSystem->AbsolutePath(Path, Local);
}
//---------------------------------------------------------------------------
void TTerminal::ReactOnCommand(int /*TFSCommand*/ Cmd)
{
  bool ChangesDirectory = false;
  bool ModifiesFiles = false;

  switch ((TFSCommand)Cmd) {
    case fsChangeDirectory:
    case fsHomeDirectory:
      ChangesDirectory = true;
      break;

    case fsCopyToRemote:
    case fsDeleteFile:
    case fsRenameFile:
    case fsMoveFile:
    case fsCopyFile:
    case fsCreateDirectory:
    case fsChangeMode:
    case fsChangeGroup:
    case fsChangeOwner:
    case fsChangeProperties:
      ModifiesFiles = true;
      break;

    case fsAnyCommand:
      ChangesDirectory = true;
      ModifiesFiles = true;
      break;
  }

  if (ChangesDirectory)
  {
    if (!InTransaction())
    {
      ReadCurrentDirectory();
      if (GetAutoReadDirectory())
      {
        ReadDirectory(false);
      }
    }
      else
    {
      FReadCurrentDirectoryPending = true;
      if (GetAutoReadDirectory())
      {
        FReadDirectoryPending = true;
      }
    }
  }
    else
  if (ModifiesFiles && GetAutoReadDirectory() && Configuration->GetAutoReadDirectoryAfterOp())
  {
    if (!InTransaction()) ReadDirectory(true);
      else FReadDirectoryPending = true;
  }
}
//---------------------------------------------------------------------------
void TTerminal::TerminalError(std::wstring Msg)
{
  TerminalError(NULL, Msg);
}
//---------------------------------------------------------------------------
void TTerminal::TerminalError(const std::exception * E, std::wstring Msg)
{
  throw ETerminal(E, Msg);
}
//---------------------------------------------------------------------------
bool TTerminal::DoQueryReopen(const std::exception * E)
{

  LogEvent(L"Connection was lost, asking what to do.");

  TQueryParams Params(qpAllowContinueOnError);
  Params.Timeout = Configuration->GetSessionReopenAuto();
  Params.TimeoutAnswer = qaRetry;
  TQueryButtonAlias Aliases[1];
  Aliases[0].Button = qaRetry;
  Aliases[0].Alias = LoadStr(RECONNECT_BUTTON);
  Params.Aliases = Aliases;
  Params.AliasesCount = LENOF(Aliases);
  return (QueryUserException(L"", E, qaRetry | qaAbort, &Params, qtError) == qaRetry);
}
//---------------------------------------------------------------------------
bool TTerminal::QueryReopen(const std::exception * E, int Params,
  TFileOperationProgressType * OperationProgress)
{
  TSuspendFileOperationProgress Suspend(OperationProgress);

  bool Result = DoQueryReopen(E);

  if (Result)
  {
    TDateTime Start = Now();
    do
    {
      try
      {
        Reopen(Params);
      }
      catch (const std::exception & E)
      {
        if (!GetActive())
        {
          Result =
            ((Configuration->GetSessionReopenTimeout() == 0) ||
             (int(double(Now() - Start) * 24*60*60*1000) < Configuration->GetSessionReopenTimeout())) &&
            DoQueryReopen(&E);
        }
        else
        {
          throw;
        }
      }
    }
    while (!GetActive() && Result);
  }

  return Result;
}
//---------------------------------------------------------------------------
bool TTerminal::FileOperationLoopQuery(const std::exception & E,
  TFileOperationProgressType * OperationProgress, const std::wstring Message,
  bool AllowSkip, std::wstring SpecialRetry)
{
  bool Result = false;
  GetLog()->AddException(&E);
  int Answer;

  if (AllowSkip && OperationProgress->SkipToAll)
  {
    Answer = qaSkip;
  }
  else
  {
    int Answers = qaRetry | qaAbort |
      FLAGMASK(AllowSkip, (qaSkip | qaAll)) |
      FLAGMASK(!SpecialRetry.empty(), qaYes);
    TQueryParams Params(qpAllowContinueOnError | FLAGMASK(!AllowSkip, qpFatalAbort));
    TQueryButtonAlias Aliases[2];
    int AliasCount = 0;

    if (FLAGSET(Answers, qaAll))
    {
      Aliases[AliasCount].Button = qaAll;
      Aliases[AliasCount].Alias = LoadStr(SKIP_ALL_BUTTON);
      AliasCount++;
    }
    if (FLAGSET(Answers, qaYes))
    {
      Aliases[AliasCount].Button = qaYes;
      Aliases[AliasCount].Alias = SpecialRetry;
      AliasCount++;
    }

    if (AliasCount > 0)
    {
      Params.Aliases = Aliases;
      Params.AliasesCount = AliasCount;
    }

    SUSPEND_OPERATION (
      Answer = QueryUserException(Message, &E, Answers, &Params, qtError);
    );

    if (Answer == qaAll)
    {
      OperationProgress->SkipToAll = true;
      Answer = qaSkip;
    }
    if (Answer == qaYes)
    {
      Result = true;
      Answer = qaRetry;
    }
  }

  if (Answer != qaRetry)
  {
    if (Answer == qaAbort)
    {
      OperationProgress->Cancel = csCancel;
    }

    if (AllowSkip)
    {
      THROW_SKIP_FILE(&E, Message);
    }
    else
    {
      // this can happen only during file transfer with SCP
      throw ExtException(&E, Message);
    }
  }

  return Result;
}
//---------------------------------------------------------------------------
int TTerminal::FileOperationLoop(TFileOperationEvent CallBackFunc,
  TFileOperationProgressType * OperationProgress, bool AllowSkip,
  const std::wstring Message, void * Param1, void * Param2)
{
  assert(CallBackFunc);
  int Result;
  FILE_OPERATION_LOOP_EX
  (
    AllowSkip, Message,
    Result = 0; // FIXME CallBackFunc(Param1, Param2);
  );

  return Result;
}
//---------------------------------------------------------------------------
std::wstring TTerminal::TranslateLockedPath(std::wstring Path, bool Lock)
{
  if (!GetSessionData()->GetLockInHome() || Path.empty() || (Path[1] != '/'))
    return Path;

  if (Lock)
  {
    if (Path.substr(1, FLockDirectory.size()) == FLockDirectory)
    {
      Path.erase(1, FLockDirectory.size());
      if (Path.empty()) Path = L"/";
    }
  }
  else
  {
    Path = UnixExcludeTrailingBackslash(FLockDirectory + Path);
  }
  return Path;
}
//---------------------------------------------------------------------------
void TTerminal::ClearCaches()
{
  FDirectoryCache->Clear();
  if (FDirectoryChangesCache != NULL)
  {
    FDirectoryChangesCache->Clear();
  }
}
//---------------------------------------------------------------------------
void TTerminal::ClearCachedFileList(const std::wstring Path,
  bool SubDirs)
{
  FDirectoryCache->ClearFileList(Path, SubDirs);
}
//---------------------------------------------------------------------------
void TTerminal::AddCachedFileList(TRemoteFileList * FileList)
{
  FDirectoryCache->AddFileList(FileList);
}
//---------------------------------------------------------------------------
bool TTerminal::DirectoryFileList(const std::wstring Path,
  TRemoteFileList *& FileList, bool CanLoad)
{
  bool Result = false;
  if (UnixComparePaths(FFiles->GetDirectory(), Path))
  {
    Result = (FileList == NULL) || (FileList->GetTimestamp() < FFiles->GetTimestamp());
    if (Result)
    {
      if (FileList == NULL)
      {
        FileList = new TRemoteFileList();
      }
      FFiles->DuplicateTo(FileList);
    }
  }
  else
  {
    if (((FileList == NULL) && FDirectoryCache->HasFileList(Path)) ||
        ((FileList != NULL) && FDirectoryCache->HasNewerFileList(Path, FileList->GetTimestamp())))
    {
      bool Created = (FileList == NULL);
      if (Created)
      {
        FileList = new TRemoteFileList();
      }

      Result = FDirectoryCache->GetFileList(Path, FileList);
      if (!Result && Created)
      {
        SAFE_DESTROY(FileList);
      }
    }
    // do not attempt to load file list if there is cached version,
    // only absence of cached version indicates that we consider
    // the directory content obsolete
    else if (CanLoad && !FDirectoryCache->HasFileList(Path))
    {
      bool Created = (FileList == NULL);
      if (Created)
      {
        FileList = new TRemoteFileList();
      }
      FileList->SetDirectory(Path);

      try
      {
        ReadDirectory(FileList);
        Result = true;
      }
      catch (...)
      {
        if (Created)
        {
          SAFE_DESTROY(FileList);
        }
        throw;
      }
    }
  }

  return Result;
}
//---------------------------------------------------------------------------
void TTerminal::SetCurrentDirectory(std::wstring value)
{
  assert(FFileSystem);
  value = TranslateLockedPath(value, false);
  if (value != FFileSystem->GetCurrentDirectory())
  {
    ChangeDirectory(value);
  }
}
//---------------------------------------------------------------------------
std::wstring TTerminal::GetCurrentDirectory()
{
  if (FFileSystem)
  {
    FCurrentDirectory = FFileSystem->GetCurrentDirectory();
    if (FCurrentDirectory.empty())
    {
      ReadCurrentDirectory();
    }
  }

  return TranslateLockedPath(FCurrentDirectory, true);
}
//---------------------------------------------------------------------------
std::wstring TTerminal::PeekCurrentDirectory()
{
  if (FFileSystem)
  {
    FCurrentDirectory = FFileSystem->GetCurrentDirectory();
  }

  return TranslateLockedPath(FCurrentDirectory, true);
}
//---------------------------------------------------------------------------
const TRemoteTokenList * TTerminal::GetGroups()
{
  assert(FFileSystem);
  LookupUsersGroups();
  return &FGroups;
}
//---------------------------------------------------------------------------
const TRemoteTokenList * TTerminal::GetUsers()
{
  assert(FFileSystem);
  LookupUsersGroups();
  return &FUsers;
}
//---------------------------------------------------------------------------
const TRemoteTokenList * TTerminal::GetMembership()
{
  assert(FFileSystem);
  LookupUsersGroups();
  return &FMembership;
}
//---------------------------------------------------------------------------
std::wstring TTerminal::GetUserName() 
{
  // in future might also be implemented to detect username similar to GetUserGroups
  assert(FFileSystem != NULL);
  std::wstring Result = FFileSystem->GetUserName();
  // Is empty also when stored username was used
  if (Result.empty())
  {
    Result = GetSessionData()->GetUserName();
  }
  return Result;
}
//---------------------------------------------------------------------------
bool TTerminal::GetAreCachesEmpty() const
{
  return FDirectoryCache->GetIsEmpty() &&
    ((FDirectoryChangesCache == NULL) || FDirectoryChangesCache->GetIsEmpty());
}
//---------------------------------------------------------------------------
void TTerminal::DoChangeDirectory()
{
  if (FOnChangeDirectory.num_slots() > 0)
  {
    TCallbackGuard Guard(this);
    FOnChangeDirectory(this);
    Guard.Verify();
  }
}
//---------------------------------------------------------------------------
void TTerminal::DoReadDirectory(bool ReloadOnly)
{
  if (FOnReadDirectory)
  {
    TCallbackGuard Guard(this);
    // FIXME FOnReadDirectory(this, ReloadOnly);
    Guard.Verify();
  }
}
//---------------------------------------------------------------------------
void TTerminal::DoStartReadDirectory()
{
  if (FOnStartReadDirectory.num_slots() > 0)
  {
    TCallbackGuard Guard(this);
    FOnStartReadDirectory(this);
    Guard.Verify();
  }
}
//---------------------------------------------------------------------------
void TTerminal::DoReadDirectoryProgress(int Progress, bool & Cancel)
{
  if (FReadingCurrentDirectory && (FOnReadDirectoryProgress != NULL))
  {
    TCallbackGuard Guard(this);
    // FIXME FOnReadDirectoryProgress(this, Progress, Cancel);
    Guard.Verify();
  }
  if (FOnFindingFile != NULL)
  {
    TCallbackGuard Guard(this);
    // FIXME FOnFindingFile(this, "", Cancel);
    Guard.Verify();
  }
}
//---------------------------------------------------------------------------
bool TTerminal::InTransaction()
{
  return (FInTransaction > 0) && !FSuspendTransaction;
}
//---------------------------------------------------------------------------
void TTerminal::BeginTransaction()
{
  if (FInTransaction == 0)
  {
    FReadCurrentDirectoryPending = false;
    FReadDirectoryPending = false;
  }
  FInTransaction++;

  if (FCommandSession != NULL)
  {
    FCommandSession->BeginTransaction();
  }
}
//---------------------------------------------------------------------------
void TTerminal::EndTransaction()
{
  if (FInTransaction == 0)
    TerminalError(L"Can't end transaction, not in transaction");
  assert(FInTransaction > 0);
  FInTransaction--;

  // it connection was closed due to fatal error during transaction, do nothing
  if (GetActive())
  {
    if (FInTransaction == 0)
    {
      {
        BOOST_SCOPE_EXIT ( (&Self) )
        {
          Self->FReadCurrentDirectoryPending = false;
          Self->FReadDirectoryPending = false;
        } BOOST_SCOPE_EXIT_END
        if (FReadCurrentDirectoryPending) ReadCurrentDirectory();
        if (FReadDirectoryPending) ReadDirectory(!FReadCurrentDirectoryPending);
      }
    }
  }

  if (FCommandSession != NULL)
  {
    FCommandSession->EndTransaction();
  }
}
//---------------------------------------------------------------------------
void TTerminal::SetExceptionOnFail(bool value)
{
  if (value) FExceptionOnFail++;
    else
  {
    if (FExceptionOnFail == 0)
      throw std::exception("ExceptionOnFail is already zero.");
    FExceptionOnFail--;
  }

  if (FCommandSession != NULL)
  {
    FCommandSession->FExceptionOnFail = FExceptionOnFail;
  }
}
//---------------------------------------------------------------------------
bool TTerminal::GetExceptionOnFail() const
{
  return (bool)(FExceptionOnFail > 0);
}
//---------------------------------------------------------------------------
void TTerminal::FatalError(const std::exception * E, std::wstring Msg)
{
  bool SecureShellActive = (FSecureShell != NULL) && FSecureShell->GetActive();
  if (GetActive() || SecureShellActive)
  {
    // We log this instead of std::exception handler, because Close() would
    // probably cause std::exception handler to loose pointer to TShellLog()
    LogEvent(L"Attempt to close connection due to fatal std::exception:");
    GetLog()->Add(llException, Msg);
    GetLog()->AddException(E);

    if (GetActive())
    {
      Close();
    }

    // this may happen if failure of authentication of SSH, owned by terminal yet
    // (because the protocol was not decided yet), is detected by us (not by putty).
    // e.g. not verified host key
    if (SecureShellActive)
    {
      FSecureShell->Close();
    }
  }

  if (FCallbackGuard != NULL)
  {
    FCallbackGuard->FatalError(E, Msg);
  }
  else
  {
    throw ESshFatal(E, Msg);
  }
}
//---------------------------------------------------------------------------
void TTerminal::CommandError(const std::exception * E, const std::wstring Msg)
{
  CommandError(E, Msg, 0);
}
//---------------------------------------------------------------------------
int TTerminal::CommandError(const std::exception * E, const std::wstring Msg,
  int Answers)
{
  // may not be, particularly when TTerminal::Reopen is being called
  // from within OnShowExtendedException handler
  assert(FCallbackGuard == NULL);
  int Result = 0;
  if (E) // FIXME && E->InheritsFrom(__classid(EFatal)))
  {
    FatalError(E, Msg);
  }
  else if (E) // && E->InheritsFrom(__classid(EAbort)))
  {
    // resent EAbort std::exception
    // Abort();
  }
  else if (GetExceptionOnFail())
  {
    throw ECommand(E, Msg);
  }
  else if (!Answers)
  {
    ECommand * ECmd = new ECommand(E, Msg);
    {
      BOOST_SCOPE_EXIT ( (&ECmd) )
      {
        delete ECmd;
      } BOOST_SCOPE_EXIT_END
      HandleExtendedException(ECmd);
    }
  }
  else
  {
    // small hack to anable "skip to all" for COMMAND_ERROR_ARI
    bool CanSkip = FLAGSET(Answers, qaSkip) && (GetOperationProgress() != NULL);
    if (CanSkip && GetOperationProgress()->SkipToAll)
    {
      Result = qaSkip;
    }
    else
    {
      TQueryParams Params(qpAllowContinueOnError);
      TQueryButtonAlias Aliases[1];
      if (CanSkip)
      {
        Aliases[0].Button = qaAll;
        Aliases[0].Alias = LoadStr(SKIP_ALL_BUTTON);
        Params.Aliases = Aliases;
        Params.AliasesCount = LENOF(Aliases);
        Answers |= qaAll;
      }
      Result = QueryUserException(Msg, E, Answers, &Params, qtError);
      if (Result == qaAll)
      {
        assert(GetOperationProgress() != NULL);
        GetOperationProgress()->SkipToAll = true;
        Result = qaSkip;
      }
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
bool TTerminal::HandleException(const std::exception * E)
{
  if (GetExceptionOnFail())
  {
    return false;
  }
  else
  {
    GetLog()->AddException(E);
    return true;
  }
}
//---------------------------------------------------------------------------
void TTerminal::CloseOnCompletion(TOnceDoneOperation Operation, const std::wstring Message)
{
  LogEvent(L"Closing session after completed operation (as requested by user)");
  Close();
  throw ESshTerminate(NULL,
    Message.empty() ? LoadStr(CLOSED_ON_COMPLETION) : Message,
    Operation);
}
//---------------------------------------------------------------------------
TBatchOverwrite TTerminal::EffectiveBatchOverwrite(
  int Params, TFileOperationProgressType * OperationProgress, bool Special)
{
  TBatchOverwrite Result;
  if (Special && FLAGSET(Params, cpResume))
  {
    Result = boResume;
  }
  else if (FLAGSET(Params, cpAppend))
  {
    Result = boAppend;
  }
  else if (FLAGSET(Params, cpNewerOnly))
  {
    // no way to change batch overwrite mode when cpNewerOnly is on
    Result = boOlder;
  }
  else if (FLAGSET(Params, cpNoConfirmation) || !Configuration->GetConfirmOverwriting())
  {
    // no way to change batch overwrite mode when overwrite confirmations are off
    assert(OperationProgress->BatchOverwrite == boNo);
    Result = boAll;
  }
  else
  {
    Result = OperationProgress->BatchOverwrite;
    if (!Special &&
        ((Result == boOlder) || (Result == boAlternateResume) || (Result == boResume)))
    {
      Result = boNo;
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
bool TTerminal::CheckRemoteFile(int Params, TFileOperationProgressType * OperationProgress)
{
  return (EffectiveBatchOverwrite(Params, OperationProgress, true) != boAll);
}
//---------------------------------------------------------------------------
int TTerminal::ConfirmFileOverwrite(const std::wstring FileName,
  const TOverwriteFileParams * FileParams, int Answers, const TQueryParams * QueryParams,
  TOperationSide Side, int Params, TFileOperationProgressType * OperationProgress,
  std::wstring Message)
{
  int Result;
  // duplicated in TSFTPFileSystem::SFTPConfirmOverwrite
  bool CanAlternateResume =
    (FileParams != NULL) &&
    (FileParams->DestSize < FileParams->SourceSize) &&
    !OperationProgress->AsciiTransfer;
  TBatchOverwrite BatchOverwrite = EffectiveBatchOverwrite(Params, OperationProgress, true);
  bool Applicable = true;
  switch (BatchOverwrite)
  {
    case boOlder:
      Applicable = (FileParams != NULL);
      break;

    case boAlternateResume:
      Applicable = CanAlternateResume;
      break;

    case boResume:
      Applicable = CanAlternateResume;
      break;
  }

  if (!Applicable)
  {
    TBatchOverwrite ABatchOverwrite = EffectiveBatchOverwrite(Params, OperationProgress, false);
    assert(BatchOverwrite != ABatchOverwrite);
    BatchOverwrite = ABatchOverwrite;
  }

  if (BatchOverwrite == boNo)
  {
    if (Message.empty())
    {
      Message = FMTLOAD((Side == osLocal ? LOCAL_FILE_OVERWRITE :
        REMOTE_FILE_OVERWRITE), FileName.c_str());
    }
    if (FileParams != NULL)
    {
      Message = FMTLOAD(FILE_OVERWRITE_DETAILS, Message.c_str(),
        IntToStr(FileParams->SourceSize).c_str(),
        UserModificationStr(FileParams->SourceTimestamp, FileParams->SourcePrecision).c_str(),
        IntToStr(FileParams->DestSize).c_str(),
        UserModificationStr(FileParams->DestTimestamp, FileParams->DestPrecision).c_str());
    }
    Result = QueryUser(Message, NULL, Answers, QueryParams);
    switch (Result)
    {
      case qaNeverAskAgain:
        Configuration->SetConfirmOverwriting(false);
        Result = qaYes;
        break;

      case qaYesToAll:
        BatchOverwrite = boAll;
        break;

      case qaAll:
        BatchOverwrite = boOlder;
        break;

      case qaNoToAll:
        BatchOverwrite = boNone;
        break;
    }

    // we user has not selected another batch overwrite mode,
    // keep the current one. note that we may get here even
    // when batch overwrite was selected already, but it could not be applied
    // to current transfer (see condition above)
    if (BatchOverwrite != boNo)
    {
      GetOperationProgress()->BatchOverwrite = BatchOverwrite;
    }
  }

  if (BatchOverwrite != boNo)
  {
    switch (BatchOverwrite)
    {
      case boAll:
        Result = qaYes;
        break;

      case boNone:
        Result = qaNo;
        break;

      case boOlder:
        Result =
          ((FileParams != NULL) &&
           (CompareFileTime(
             ReduceDateTimePrecision(FileParams->SourceTimestamp,
               LessDateTimePrecision(FileParams->SourcePrecision, FileParams->DestPrecision)),
             ReduceDateTimePrecision(FileParams->DestTimestamp,
               LessDateTimePrecision(FileParams->SourcePrecision, FileParams->DestPrecision))) > 0)) ?
          qaYes : qaNo;
        break;

      case boAlternateResume:
        assert(CanAlternateResume);
        Result = qaSkip; // ugh
        break;

      case boAppend:
        Result = qaRetry;
        break;

      case boResume:
        Result = qaRetry;
        break;
    }
  }

  return Result;
}
//---------------------------------------------------------------------------
void TTerminal::FileModified(const TRemoteFile * File,
  const std::wstring FileName, bool ClearDirectoryChange)
{
  std::wstring ParentDirectory;
  std::wstring Directory;

  if (GetSessionData()->GetCacheDirectories() || GetSessionData()->GetCacheDirectoryChanges())
  {
    if ((File != NULL) && (File->GetDirectory() != NULL))
    {
      if (File->GetIsDirectory())
      {
        Directory = File->GetDirectory()->GetFullDirectory() + File->GetFileName();
      }
      ParentDirectory = File->GetDirectory()->GetDirectory();
    }
    else if (!FileName.empty())
    {
      ParentDirectory = UnixExtractFilePath(FileName);
      if (ParentDirectory.empty())
      {
        ParentDirectory = GetCurrentDirectory();
      }

      // this case for scripting
      if ((File != NULL) && File->GetIsDirectory())
      {
        Directory = UnixIncludeTrailingBackslash(ParentDirectory) +
          UnixExtractFileName(File->GetFileName());
      }
    }
  }

  if (GetSessionData()->GetCacheDirectories())
  {
    if (!Directory.empty())
    {
      DirectoryModified(Directory, true);
    }
    if (!ParentDirectory.empty())
    {
      DirectoryModified(ParentDirectory, false);
    }
  }

  if (GetSessionData()->GetCacheDirectoryChanges() && ClearDirectoryChange)
  {
    if (!Directory.empty())
    {
      FDirectoryChangesCache->ClearDirectoryChange(Directory);
      FDirectoryChangesCache->ClearDirectoryChangeTarget(Directory);
    }
  }
}
//---------------------------------------------------------------------------
void TTerminal::DirectoryModified(const std::wstring Path, bool SubDirs)
{
  if (Path.empty())
  {
    ClearCachedFileList(GetCurrentDirectory(), SubDirs);
  }
  else
  {
    ClearCachedFileList(Path, SubDirs);
  }
}
//---------------------------------------------------------------------------
void TTerminal::DirectoryLoaded(TRemoteFileList * FileList)
{
  AddCachedFileList(FileList);
}
//---------------------------------------------------------------------------
void TTerminal::ReloadDirectory()
{
  if (GetSessionData()->GetCacheDirectories())
  {
    DirectoryModified(GetCurrentDirectory(), false);
  }
  if (GetSessionData()->GetCacheDirectoryChanges())
  {
    assert(FDirectoryChangesCache != NULL);
    FDirectoryChangesCache->ClearDirectoryChange(GetCurrentDirectory());
  }

  ReadCurrentDirectory();
  FReadCurrentDirectoryPending = false;
  ReadDirectory(true);
  FReadDirectoryPending = false;
}
//---------------------------------------------------------------------------
void TTerminal::RefreshDirectory()
{
  if (GetSessionData()->GetCacheDirectories() &&
      FDirectoryCache->HasNewerFileList(GetCurrentDirectory(), FFiles->GetTimestamp()))
  {
    // Second parameter was added to allow (rather force) using the cache.
    // Before, the directory was reloaded always, it seems useless,
    // has it any reason?
    ReadDirectory(true, true);
    FReadDirectoryPending = false;
  }
}
//---------------------------------------------------------------------------
void TTerminal::EnsureNonExistence(const std::wstring FileName)
{
  // if filename doesn't contain path, we check for existence of file
  if ((UnixExtractFileDir(FileName).empty()) &&
      UnixComparePaths(GetCurrentDirectory(), FFiles->GetDirectory()))
  {
    TRemoteFile *File = FFiles->FindFile(FileName);
    if (File)
    {
      if (File->GetIsDirectory()) throw ECommand(NULL, FMTLOAD(RENAME_CREATE_DIR_EXISTS, FileName.c_str()));
        else throw ECommand(NULL, FMTLOAD(RENAME_CREATE_FILE_EXISTS, FileName.c_str()));
    }
  }
}
//---------------------------------------------------------------------------
void inline TTerminal::LogEvent(const std::wstring & Str)
{
  if (GetLog()->GetLogging())
  {
    GetLog()->Add(llMessage, Str);
  }
}
//---------------------------------------------------------------------------
void TTerminal::RollbackAction(TSessionAction & Action,
  TFileOperationProgressType * OperationProgress, const std::exception * E)
{
  // EScpSkipFile without "cancel" is file skip,
  // and we do not want to record skipped actions.
  // But EScpSkipFile with "cancel" is abort and we want to record that.
  // Note that TSCPFileSystem modifies the logic of RollbackAction little bit.
  if ((dynamic_cast<const EScpSkipFile *>(E) != NULL) &&
      ((OperationProgress == NULL) ||
       (OperationProgress->Cancel == csContinue)))
  {
    Action.Cancel();
  }
  else
  {
    Action.Rollback(E);
  }
}
//---------------------------------------------------------------------------
void TTerminal::DoStartup()
{
  LogEvent(L"Doing startup conversation with host.");
  BeginTransaction();
  {
    BOOST_SCOPE_EXIT ( (&Self) )
    {
      Self->EndTransaction();
    } BOOST_SCOPE_EXIT_END
    DoInformation(LoadStr(STATUS_STARTUP), true);

    // Make sure that directory would be loaded at last
    FReadCurrentDirectoryPending = true;
    FReadDirectoryPending = GetAutoReadDirectory();

    FFileSystem->DoStartup();

    LookupUsersGroups();

    DoInformation(LoadStr(STATUS_OPEN_DIRECTORY), true);
    if (!GetSessionData()->GetRemoteDirectory().empty())
    {
      ChangeDirectory(GetSessionData()->GetRemoteDirectory());
    }

  }
  LogEvent(L"Startup conversation with host finished.");
}
//---------------------------------------------------------------------------
void TTerminal::ReadCurrentDirectory()
{
  assert(FFileSystem);
  try
  {
    // reset flag is case we are called externally (like from console dialog)
    FReadCurrentDirectoryPending = false;

    LogEvent(L"Getting current directory name.");
    std::wstring OldDirectory = FFileSystem->GetCurrentDirectory();

    FFileSystem->ReadCurrentDirectory();
    ReactOnCommand(fsCurrentDirectory);

    if (GetSessionData()->GetCacheDirectoryChanges())
    {
      assert(FDirectoryChangesCache != NULL);
      FDirectoryChangesCache->AddDirectoryChange(OldDirectory,
        FLastDirectoryChange, GetCurrentDirectory());
      // not to broke the cache, if the next directory change would not
      // be initialited by ChangeDirectory(), which sets it
      // (HomeDirectory() particularly)
      FLastDirectoryChange = L"";
    }

    if (OldDirectory.empty())
    {
      FLockDirectory = (GetSessionData()->GetLockInHome() ?
        FFileSystem->GetCurrentDirectory() : std::wstring(L""));
    }
    if (OldDirectory != FFileSystem->GetCurrentDirectory()) DoChangeDirectory();
  }
  catch (const std::exception &E)
  {
    CommandError(&E, LoadStr(READ_CURRENT_DIR_ERROR));
  }
}
//---------------------------------------------------------------------------
void TTerminal::ReadDirectory(bool ReloadOnly, bool ForceCache)
{
  bool LoadedFromCache = false;

  if (GetSessionData()->GetCacheDirectories() && FDirectoryCache->HasFileList(GetCurrentDirectory()))
  {
    if (ReloadOnly && !ForceCache)
    {
      LogEvent(L"Cached directory not reloaded.");
    }
    else
    {
      DoStartReadDirectory();
      {
        BOOST_SCOPE_EXIT ( (&Self) (ReloadOnly) )
        {
          Self->DoReadDirectory(ReloadOnly);
        } BOOST_SCOPE_EXIT_END
        LoadedFromCache = FDirectoryCache->GetFileList(GetCurrentDirectory(), FFiles);
      }

      if (LoadedFromCache)
      {
        LogEvent(L"Directory content loaded from cache.");
      }
      else
      {
        LogEvent(L"Cached Directory content has been removed.");
      }
    }
  }

  if (!LoadedFromCache)
  {
    DoStartReadDirectory();
    FReadingCurrentDirectory = true;
    bool Cancel = false; // dummy
    DoReadDirectoryProgress(0, Cancel);

    try
    {
      TRemoteDirectory *Files = new TRemoteDirectory(this, FFiles);
      {
        BOOST_SCOPE_EXIT ( (&Self) (&Files) (Cancel) (ReloadOnly) )
        {
          Self->DoReadDirectoryProgress(-1, Cancel);
          Self->FReadingCurrentDirectory = false;
          delete Self->FFiles;
          Self->FFiles = Files;
          Self->DoReadDirectory(ReloadOnly);
          if (Self->GetActive())
          {
            if (Self->GetSessionData()->GetCacheDirectories())
            {
              Self->DirectoryLoaded(Self->FFiles);
            }
          }
        } BOOST_SCOPE_EXIT_END
        Files->SetDirectory(GetCurrentDirectory());
        CustomReadDirectory(Files);
      }
    }
    catch (const std::exception &E)
    {
      CommandError(&E, ::FmtLoadStr(LIST_DIR_ERROR, FFiles->GetDirectory().c_str()));
    }
  }
}
//---------------------------------------------------------------------------
void TTerminal::CustomReadDirectory(TRemoteFileList * FileList)
{
  assert(FileList);
  assert(FFileSystem);
  FFileSystem->ReadDirectory(FileList);
  ReactOnCommand(fsListDirectory);
}
//---------------------------------------------------------------------------
TRemoteFileList * TTerminal::ReadDirectoryListing(std::wstring Directory, const TFileMasks & Mask)
{
  TLsSessionAction Action(GetLog(), AbsolutePath(Directory, true));
  TRemoteFileList * FileList = NULL;
  try
  {
    FileList = DoReadDirectoryListing(Directory, false);
    if (FileList != NULL)
    {
      int Index = 0;
      while (Index < FileList->GetCount())
      {
        TRemoteFile * File = FileList->GetFile(Index);
        if (!Mask.Matches(File->GetFileName()))
        {
          FileList->Delete(Index);
        }
        else
        {
          Index++;
        }
      }

      Action.FileList(FileList);
    }
  }
  catch (const std::exception & E)
  {
    /* FIXME
    COMMAND_ERROR_ARI_ACTION
    (
      "",
      FileList = ReadDirectoryListing(Directory, Mask),
      Action
    );
    */
  }
  return FileList;
}
//---------------------------------------------------------------------------
TRemoteFileList * TTerminal::CustomReadDirectoryListing(std::wstring Directory, bool UseCache)
{
  TRemoteFileList * FileList = NULL;
  try
  {
    FileList = DoReadDirectoryListing(Directory, UseCache);
  }
  catch (const std::exception & E)
  { // FIXME
/*     COMMAND_ERROR_ARI
    (
      "",
      FileList = CustomReadDirectoryListing(Directory, UseCache);
    );
 */  }
  return FileList;
}
//---------------------------------------------------------------------------
TRemoteFileList * TTerminal::DoReadDirectoryListing(std::wstring Directory, bool UseCache)
{
  TRemoteFileList * FileList = new TRemoteFileList();
  try
  {
    bool Cache = UseCache && GetSessionData()->GetCacheDirectories();
    bool LoadedFromCache = Cache && FDirectoryCache->HasFileList(Directory);
    if (LoadedFromCache)
    {
      LoadedFromCache = FDirectoryCache->GetFileList(Directory, FileList);
    }

    if (!LoadedFromCache)
    {
      FileList->SetDirectory(Directory);

      SetExceptionOnFail(true);
      {
        BOOST_SCOPE_EXIT ( (&Self) )
        {
          Self->SetExceptionOnFail(false);
        } BOOST_SCOPE_EXIT_END
        ReadDirectory(FileList);
      }

      if (Cache)
      {
        AddCachedFileList(FileList);
      }
    }
  }
  catch (...)
  {
    delete FileList;
    throw;
  }
  return FileList;
}
//---------------------------------------------------------------------------
void TTerminal::ProcessDirectory(const std::wstring DirName,
  TProcessFileEvent CallBackFunc, void * Param, bool UseCache, bool IgnoreErrors)
{
  TRemoteFileList * FileList = NULL;
  if (IgnoreErrors)
  {
    SetExceptionOnFail(true);
    {
      BOOST_SCOPE_EXIT ( (&Self) )
      {
        Self->SetExceptionOnFail(false);
      } BOOST_SCOPE_EXIT_END
      try
      {
        FileList = CustomReadDirectoryListing(DirName, UseCache);
      }
      catch (...)
      {
        if (!GetActive())
        {
          throw;
        }
      }
    }
  }
  else
  {
    FileList = CustomReadDirectoryListing(DirName, UseCache);
  }

  // skip if directory listing fails and user selects "skip"
  if (FileList)
  {
    {
      BOOST_SCOPE_EXIT ( (&Self) (&FileList) )
      {
        delete FileList;
      } BOOST_SCOPE_EXIT_END
      std::wstring Directory = UnixIncludeTrailingBackslash(DirName);

      TRemoteFile *File;
      for (int Index = 0; Index < FileList->GetCount(); Index++)
      {
        File = FileList->GetFile(Index);
        if (!File->GetIsParentDirectory() && !File->GetIsThisDirectory())
        {
          // FIXME CallBackFunc(Directory + File->GetFileName(), File, Param);
        }
      }
    }
  }
}
//---------------------------------------------------------------------------
void TTerminal::ReadDirectory(TRemoteFileList * FileList)
{
  try
  {
    CustomReadDirectory(FileList);
  }
  catch (const std::exception &E)
  {
    CommandError(&E, ::FmtLoadStr(LIST_DIR_ERROR, FileList->GetDirectory().c_str()));
  }
}
//---------------------------------------------------------------------------
void TTerminal::ReadSymlink(TRemoteFile * SymlinkFile,
  TRemoteFile *& File)
{
  assert(FFileSystem);
  try
  {
    LogEvent(FORMAT(L"Reading symlink \"%s\".", SymlinkFile->GetFileName().c_str()));
    FFileSystem->ReadSymlink(SymlinkFile, File);
    ReactOnCommand(fsReadSymlink);
  }
  catch (const std::exception &E)
  {
    CommandError(&E, FMTLOAD(READ_SYMLINK_ERROR, SymlinkFile->GetFileName().c_str()));
  }
}
//---------------------------------------------------------------------------
void TTerminal::ReadFile(const std::wstring FileName,
  TRemoteFile *& File)
{
  assert(FFileSystem);
  File = NULL;
  try
  {
    LogEvent(FORMAT(L"Listing file \"%s\".", FileName.c_str()));
    FFileSystem->ReadFile(FileName, File);
    ReactOnCommand(fsListFile);
  }
  catch (const std::exception &E)
  {
    if (File) delete File;
    File = NULL;
    CommandError(&E, FMTLOAD(CANT_GET_ATTRS, FileName.c_str()));
  }
}
//---------------------------------------------------------------------------
bool TTerminal::FileExists(const std::wstring FileName, TRemoteFile ** AFile)
{
  bool Result;
  TRemoteFile * File = NULL;
  try
  {
    SetExceptionOnFail(true);
    {
      BOOST_SCOPE_EXIT ( (&Self) )
      {
        Self->SetExceptionOnFail(false);
      } BOOST_SCOPE_EXIT_END
      ReadFile(FileName, File);
    }

    if (AFile != NULL)
    {
      *AFile = File;
    }
    else
    {
      delete File;
    }
    Result = true;
  }
  catch (...)
  {
    if (GetActive())
    {
      Result = false;
    }
    else
    {
      throw;
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
void TTerminal::AnnounceFileListOperation()
{
  FFileSystem->AnnounceFileListOperation();
}
//---------------------------------------------------------------------------
bool TTerminal::ProcessFiles(TStrings * FileList,
  TFileOperation Operation, TProcessFileEvent ProcessFile, void * Param,
  TOperationSide Side, bool Ex)
{
  assert(FFileSystem);
  assert(FileList);

  bool Result = false;
  TOnceDoneOperation OnceDoneOperation = odoIdle;

  try
  {
    TFileOperationProgressType *Progress = new TFileOperationProgressType(boost::bind(&TTerminal::DoProgress, this, _1, _2),
        boost::bind(&TTerminal::DoFinished, this, _1, _2, _3, _4, _5, _6));
    Progress->Start(Operation, Side, FileList->GetCount());

    FOperationProgress = Progress;
    {
      BOOST_SCOPE_EXIT ( (&Self) (&Progress) )
      {
        Self->FOperationProgress = NULL;
        Progress->Stop();
      }
      BOOST_SCOPE_EXIT_END
      if (Side == osRemote)
      {
        BeginTransaction();
      }

      {
          BOOST_SCOPE_EXIT ( (&Self) (Side) )
          {
            if (Side == osRemote)
            {
              Self->EndTransaction();
            }
          } BOOST_SCOPE_EXIT_END
        int Index = 0;
        std::wstring FileName;
        bool Success;
        while ((Index < FileList->GetCount()) && (Progress->Cancel == csContinue))
        {
          FileName = FileList->GetString(Index);
          try
          {
            {
              BOOST_SCOPE_EXIT ( (&Self) (&Progress) (FileName) (Success) (OnceDoneOperation) )
              {
                Progress->Finish(FileName, Success, OnceDoneOperation);
              } BOOST_SCOPE_EXIT_END
              Success = false;
              if (!Ex)
              {
                // FIXME ProcessFile(FileName, (TRemoteFile *)FileList->GetObject(Index), Param);
              }
              else
              {
                // not used anymore
                TProcessFileEventEx ProcessFileEx = (TProcessFileEventEx)ProcessFile;
                // FIXME ProcessFileEx(FileName, (TRemoteFile *)FileList->GetObject(Index), Param, Index);
              }
              Success = true;
            }
          }
          catch (const EScpSkipFile & E)
          {
            TFileOperationProgressType *OperationProgress = GetOperationProgress();
            SUSPEND_OPERATION (
              if (!HandleException(&E)) throw;
            );
          }
          Index++;
        }
      }

      if (Progress->Cancel == csContinue)
      {
        Result = true;
      }
    }
  }
  catch (...)
  {
    OnceDoneOperation = odoIdle;
    // this was missing here. was it by purpose?
    // without it any error message is lost
    throw;
  }

  if (OnceDoneOperation != odoIdle)
  {
    CloseOnCompletion(OnceDoneOperation);
  }

  return Result;
}
//---------------------------------------------------------------------------
// not used anymore
bool TTerminal::ProcessFilesEx(TStrings * FileList, TFileOperation Operation,
  TProcessFileEventEx ProcessFile, void * Param, TOperationSide Side)
{
  return ProcessFiles(FileList, Operation, TProcessFileEvent(ProcessFile),
    Param, Side, true);
}
//---------------------------------------------------------------------------
TStrings * TTerminal::GetFixedPaths()
{
  assert(FFileSystem != NULL);
  return FFileSystem->GetFixedPaths();
}
//---------------------------------------------------------------------------
bool TTerminal::GetResolvingSymlinks()
{
  return GetSessionData()->GetResolveSymlinks() && GetIsCapable(fcResolveSymlink);
}
//---------------------------------------------------------------------------
TUsableCopyParamAttrs TTerminal::UsableCopyParamAttrs(int Params)
{
  TUsableCopyParamAttrs Result;
  Result.General =
    FLAGMASK(!GetIsCapable(fcTextMode), cpaNoTransferMode) |
    FLAGMASK(!GetIsCapable(fcModeChanging), cpaNoRights) |
    FLAGMASK(!GetIsCapable(fcModeChanging), cpaNoPreserveReadOnly) |
    FLAGMASK(FLAGSET(Params, cpDelete), cpaNoClearArchive) |
    FLAGMASK(!GetIsCapable(fcIgnorePermErrors), cpaNoIgnorePermErrors);
  Result.Download = Result.General | cpaNoClearArchive | cpaNoRights |
    cpaNoIgnorePermErrors;
  Result.Upload = Result.General | cpaNoPreserveReadOnly |
    FLAGMASK(!GetIsCapable(fcModeChangingUpload), cpaNoRights) |
    FLAGMASK(!GetIsCapable(fcPreservingTimestampUpload), cpaNoPreserveTime);
  return Result;
}
//---------------------------------------------------------------------------
bool TTerminal::IsRecycledFile(std::wstring FileName)
{
  std::wstring Path = UnixExtractFilePath(FileName);
  if (Path.empty())
  {
    Path = GetCurrentDirectory();
  }
  return UnixComparePaths(Path, GetSessionData()->GetRecycleBinPath());
}
//---------------------------------------------------------------------------
void TTerminal::RecycleFile(std::wstring FileName,
  const TRemoteFile * File)
{
  if (FileName.empty())
  {
    assert(File != NULL);
    FileName = File->GetFileName();
  }

  if (!IsRecycledFile(FileName))
  {
    LogEvent(FORMAT(L"Moving file \"%s\" to remote recycle bin '%s'.",
      FileName.c_str(), GetSessionData()->GetRecycleBinPath().c_str()));

    TMoveFileParams Params;
    Params.Target = GetSessionData()->GetRecycleBinPath();
    Params.FileMask = FORMAT(L"*-%s.*", FormatDateTime(L"yyyymmdd-hhnnss", Now()).c_str());

    MoveFile(FileName, File, &Params);
  }
}
//---------------------------------------------------------------------------
void TTerminal::DeleteFile(std::wstring FileName,
  const TRemoteFile * File, void * AParams)
{
  if (FileName.empty() && File)
  {
    FileName = File->GetFileName();
  }
  if (GetOperationProgress() && GetOperationProgress()->Operation == foDelete)
  {
    if (GetOperationProgress()->Cancel != csContinue) Abort();
    GetOperationProgress()->SetFile(FileName);
  }
  int Params = (AParams != NULL) ? *((int*)AParams) : 0;
  bool Recycle =
    FLAGCLEAR(Params, dfForceDelete) &&
    (GetSessionData()->GetDeleteToRecycleBin() != FLAGSET(Params, dfAlternative));
  if (Recycle && !IsRecycledFile(FileName))
  {
    RecycleFile(FileName, File);
  }
  else
  {
    LogEvent(FORMAT(L"Deleting file \"%s\".", FileName.c_str()));
    if (File) FileModified(File, FileName, true);
    DoDeleteFile(FileName, File, Params);
    ReactOnCommand(fsDeleteFile);
  }
}
//---------------------------------------------------------------------------
void TTerminal::DoDeleteFile(const std::wstring FileName,
  const TRemoteFile * File, int Params)
{
  TRmSessionAction Action(GetLog(), AbsolutePath(FileName, true));
  try
  {
    assert(FFileSystem);
    // 'File' parameter: SFTPFileSystem needs to know if file is file or directory
    FFileSystem->DeleteFile(FileName, File, Params, Action);
  }
  catch (const std::exception & E)
  {
    COMMAND_ERROR_ARI_ACTION
    (
      FMTLOAD(DELETE_FILE_ERROR, FileName.c_str()),
      DoDeleteFile(FileName, File, Params),
      Action
    );
  }
}
//---------------------------------------------------------------------------
bool TTerminal::DeleteFiles(TStrings * FilesToDelete, int Params)
{
  // TODO: avoid resolving symlinks while reading subdirectories.
  // Resolving does not work anyway for relative symlinks in subdirectories
  // (at least for SFTP).
  return ProcessFiles(FilesToDelete, foDelete, (TProcessFileEvent)&TTerminal::DeleteFile, &Params);
}
//---------------------------------------------------------------------------
void TTerminal::DeleteLocalFile(std::wstring FileName,
  const TRemoteFile * /*File*/, void * Params)
{
  if (GetOnDeleteLocalFile() == NULL)
  {
    if (!RecursiveDeleteFile(FileName, false))
    {
      throw ExtException(FMTLOAD(DELETE_FILE_ERROR, FileName.c_str()));
    }
  }
  else
  {
    // FIXME OnDeleteLocalFile(FileName, FLAGSET(*((int*)Params), dfAlternative));
  }
}
//---------------------------------------------------------------------------
bool TTerminal::DeleteLocalFiles(TStrings * FileList, int Params)
{
  return ProcessFiles(FileList, foDelete, (TProcessFileEvent)&TTerminal::DeleteLocalFile, &Params, osLocal);
}
//---------------------------------------------------------------------------
void TTerminal::CustomCommandOnFile(std::wstring FileName,
  const TRemoteFile * File, void * AParams)
{
  TCustomCommandParams * Params = ((TCustomCommandParams *)AParams);
  if (FileName.empty() && File)
  {
    FileName = File->GetFileName();
  }
  if (GetOperationProgress() && GetOperationProgress()->Operation == foCustomCommand)
  {
    if (GetOperationProgress()->Cancel != csContinue) Abort();
    GetOperationProgress()->SetFile(FileName);
  }
  LogEvent(FORMAT(L"Executing custom command \"%s\" (%d) on file \"%s\".",
    Params->Command.c_str(), Params->Params, FileName.c_str()));
  if (File) FileModified(File, FileName);
  DoCustomCommandOnFile(FileName, File, Params->Command, Params->Params,
    Params->OutputEvent);
  ReactOnCommand(fsAnyCommand);
}
//---------------------------------------------------------------------------
void TTerminal::DoCustomCommandOnFile(std::wstring FileName,
  const TRemoteFile * File, std::wstring Command, int Params,
  const captureoutput_slot_type &OutputEvent)
{
  try
  {
    if (GetIsCapable(fcAnyCommand))
    {
      assert(FFileSystem);
      assert(fcShellAnyCommand);
      FFileSystem->CustomCommandOnFile(FileName, File, Command, Params, OutputEvent);
    }
    else
    {
      assert(GetCommandSessionOpened());
      assert(FCommandSession->GetFSProtocol() == cfsSCP);
      LogEvent(L"Executing custom command on command session.");

      FCommandSession->SetCurrentDirectory(GetCurrentDirectory());
      FCommandSession->FFileSystem->CustomCommandOnFile(FileName, File, Command,
        Params, OutputEvent);
    }
  }
  catch (const std::exception & E)
  {
    COMMAND_ERROR_ARI
    (
      FMTLOAD(CUSTOM_COMMAND_ERROR, Command, FileName.c_str()),
      DoCustomCommandOnFile(FileName, File, Command, Params, OutputEvent)
    );
  }
}
//---------------------------------------------------------------------------
void TTerminal::CustomCommandOnFiles(std::wstring Command,
  int Params, TStrings * Files, const captureoutput_slot_type &OutputEvent)
{
  if (!TRemoteCustomCommand().IsFileListCommand(Command))
  {
    TCustomCommandParams AParams(Command, Params, OutputEvent);
    // AParams.Command = Command;
    // AParams.Params = Params;
    // AParams.OutputEvent.connect(OutputEvent);
    // AParams.OutputEvent = OutputEvent;
    ProcessFiles(Files, foCustomCommand, (TProcessFileEvent)&TTerminal::CustomCommandOnFile, &AParams);
  }
  else
  {
    std::wstring FileList;
    for (int i = 0; i < Files->GetCount(); i++)
    {
      TRemoteFile * File = static_cast<TRemoteFile *>(Files->GetObject(i));
      bool Dir = File->GetIsDirectory() && !File->GetIsSymLink();

      if (!Dir || FLAGSET(Params, ccApplyToDirectories))
      {
        if (!FileList.empty())
        {
          FileList += L" ";
        }

        FileList += L"\"" + ShellDelimitStr(Files->GetString(i), '"') + L"\"";
      }
    }

    TCustomCommandData Data(this);
    std::wstring Cmd =
      TRemoteCustomCommand(Data, GetCurrentDirectory(), L"", FileList).
        Complete(Command, true);
    DoAnyCommand(Cmd, OutputEvent, NULL);
  }
}
//---------------------------------------------------------------------------
void TTerminal::ChangeFileProperties(std::wstring FileName,
  const TRemoteFile * File, /*const TRemoteProperties*/ void * Properties)
{
  TRemoteProperties * RProperties = (TRemoteProperties *)Properties;
  assert(RProperties && !RProperties->Valid.Empty());

  if (FileName.empty() && File)
  {
    FileName = File->GetFileName();
  }
  if (GetOperationProgress() && GetOperationProgress()->Operation == foSetProperties)
  {
    if (GetOperationProgress()->Cancel != csContinue) Abort();
    GetOperationProgress()->SetFile(FileName);
  }
  if (GetLog()->GetLogging())
  {
    LogEvent(FORMAT(L"Changing properties of \"%s\" (%s)",
      FileName.c_str(), BooleanToEngStr(RProperties->Recursive).c_str()));
    if (RProperties->Valid.Contains(vpRights))
    {
      LogEvent(FORMAT(L" - mode: \"%s\"", RProperties->Rights.GetModeStr().c_str()));
    }
    if (RProperties->Valid.Contains(vpGroup))
    {
      LogEvent(FORMAT(L" - group: %s", RProperties->Group.GetLogText().c_str()));
    }
    if (RProperties->Valid.Contains(vpOwner))
    {
      LogEvent(FORMAT(L" - owner: %s", RProperties->Owner.GetLogText().c_str()));
    }
    if (RProperties->Valid.Contains(vpModification))
    {
      LogEvent(FORMAT(L" - modification: \"%s\"",
        FormatDateTime(L"dddddd tt",
           UnixToDateTime(RProperties->Modification, GetSessionData()->GetDSTMode())).c_str()));
    }
    if (RProperties->Valid.Contains(vpLastAccess))
    {
      LogEvent(FORMAT(L" - last access: \"%s\"",
        FormatDateTime(L"dddddd tt",
           UnixToDateTime(RProperties->LastAccess, GetSessionData()->GetDSTMode())).c_str()));
    }
  }
  FileModified(File, FileName);
  DoChangeFileProperties(FileName, File, RProperties);
  ReactOnCommand(fsChangeProperties);
}
//---------------------------------------------------------------------------
void TTerminal::DoChangeFileProperties(const std::wstring FileName,
  const TRemoteFile * File, const TRemoteProperties * Properties)
{
  TChmodSessionAction Action(GetLog(), AbsolutePath(FileName, true));
  try
  {
    assert(FFileSystem);
    FFileSystem->ChangeFileProperties(FileName, File, Properties, Action);
  }
  catch (const std::exception & E)
  {
    COMMAND_ERROR_ARI_ACTION
    (
      FMTLOAD(CHANGE_PROPERTIES_ERROR, FileName.c_str()),
      DoChangeFileProperties(FileName, File, Properties),
      Action
    );
  }
}
//---------------------------------------------------------------------------
void TTerminal::ChangeFilesProperties(TStrings * FileList,
  const TRemoteProperties * Properties)
{
  AnnounceFileListOperation();
  ProcessFiles(FileList, foSetProperties, (TProcessFileEvent)&TTerminal::ChangeFileProperties, (void *)Properties);
}
//---------------------------------------------------------------------------
bool TTerminal::LoadFilesProperties(TStrings * FileList)
{
  bool Result =
    GetIsCapable(fcLoadingAdditionalProperties) &&
    FFileSystem->LoadFilesProperties(FileList);
  if (Result && GetSessionData()->GetCacheDirectories() &&
      (FileList->GetCount() > 0) &&
      (reinterpret_cast<TRemoteFile *>(FileList->GetObject(0))->GetDirectory() == FFiles))
  {
    AddCachedFileList(FFiles);
  }
  return Result;
}
//---------------------------------------------------------------------------
void TTerminal::CalculateFileSize(std::wstring FileName,
  const TRemoteFile * File, /*TCalculateSizeParams*/ void * Param)
{
  assert(Param);
  assert(File);
  TCalculateSizeParams * AParams = static_cast<TCalculateSizeParams*>(Param);

  if (FileName.empty())
  {
    FileName = File->GetFileName();
  }

  bool AllowTransfer = (AParams->CopyParam == NULL);
  if (!AllowTransfer)
  {
    TFileMasks::TParams MaskParams;
    MaskParams.Size = File->GetSize();

    AllowTransfer = AParams->CopyParam->AllowTransfer(
      UnixExcludeTrailingBackslash(File->GetFullFileName()), osRemote, File->GetIsDirectory(),
      MaskParams);
  }

  if (AllowTransfer)
  {
    if (File->GetIsDirectory())
    {
      if (!File->GetIsSymLink())
      {
        LogEvent(FORMAT(L"Getting size of directory \"%s\"", FileName.c_str()));
        // pass in full path so we get it back in file list for AllowTransfer() exclusion
        DoCalculateDirectorySize(File->GetFullFileName(), File, AParams);
      }
      else
      {
        AParams->Size += File->GetSize();
      }

      if (AParams->Stats != NULL)
      {
        AParams->Stats->Directories++;
      }
    }
    else
    {
      AParams->Size += File->GetSize();

      if (AParams->Stats != NULL)
      {
        AParams->Stats->Files++;
      }
    }

    if ((AParams->Stats != NULL) && File->GetIsSymLink())
    {
      AParams->Stats->SymLinks++;
    }
  }

  if (GetOperationProgress() && GetOperationProgress()->Operation == foCalculateSize)
  {
    if (GetOperationProgress()->Cancel != csContinue) Abort();
    GetOperationProgress()->SetFile(FileName);
  }
}
//---------------------------------------------------------------------------
void TTerminal::DoCalculateDirectorySize(const std::wstring FileName,
  const TRemoteFile * File, TCalculateSizeParams * Params)
{
  try
  {
    ProcessDirectory(FileName, (TProcessFileEvent)&TTerminal::CalculateFileSize, Params);
  }
  catch (const std::exception & E)
  {
    if (!GetActive() || ((Params->Params & csIgnoreErrors) == 0))
    {
      COMMAND_ERROR_ARI
      (
        FMTLOAD(CALCULATE_SIZE_ERROR, FileName.c_str()),
        DoCalculateDirectorySize(FileName, File, Params)
      );
    }
  }
}
//---------------------------------------------------------------------------
void TTerminal::CalculateFilesSize(TStrings * FileList,
  __int64 & Size, int Params, const TCopyParamType * CopyParam,
  TCalculateSizeStats * Stats)
{
  TCalculateSizeParams Param;
  Param.Size = 0;
  Param.Params = Params;
  Param.CopyParam = CopyParam;
  Param.Stats = Stats;
  ProcessFiles(FileList, foCalculateSize, (TProcessFileEvent)&TTerminal::CalculateFileSize, &Param);
  Size = Param.Size;
}
//---------------------------------------------------------------------------
void TTerminal::CalculateFilesChecksum(const std::wstring & Alg,
  TStrings * FileList, TStrings * Checksums,
  calculatedchecksum_slot_type *OnCalculatedChecksum)
{
  FFileSystem->CalculateFilesChecksum(Alg, FileList, Checksums, OnCalculatedChecksum);
}
//---------------------------------------------------------------------------
void TTerminal::RenameFile(const std::wstring FileName,
  const std::wstring NewName)
{
  LogEvent(FORMAT(L"Renaming file \"%s\" to \"%s\".", FileName.c_str(), NewName.c_str()));
  DoRenameFile(FileName, NewName, false);
  ReactOnCommand(fsRenameFile);
}
//---------------------------------------------------------------------------
void TTerminal::RenameFile(const TRemoteFile * File,
  const std::wstring NewName, bool CheckExistence)
{
  assert(File && File->GetDirectory() == FFiles);
  bool Proceed = true;
  // if filename doesn't contain path, we check for existence of file
  if ((File->GetFileName() != NewName) && CheckExistence &&
      Configuration->GetConfirmOverwriting() &&
      UnixComparePaths(GetCurrentDirectory(), FFiles->GetDirectory()))
  {
    TRemoteFile * DuplicateFile = FFiles->FindFile(NewName);
    if (DuplicateFile)
    {
      std::wstring QuestionFmt;
      if (DuplicateFile->GetIsDirectory()) QuestionFmt = LoadStr(DIRECTORY_OVERWRITE);
        else QuestionFmt = LoadStr(FILE_OVERWRITE);
      int Result;
      TQueryParams Params(qpNeverAskAgainCheck);
      Result = QueryUser(FORMAT(QuestionFmt.c_str(), NewName.c_str()), NULL,
        qaYes | qaNo, &Params);
      if (Result == qaNeverAskAgain)
      {
        Proceed = true;
        Configuration->SetConfirmOverwriting(false);
      }
        else
      {
        Proceed = (Result == qaYes);
      }
    }
  }

  if (Proceed)
  {
    FileModified(File, File->GetFileName());
    RenameFile(File->GetFileName(), NewName);
  }
}
//---------------------------------------------------------------------------
void TTerminal::DoRenameFile(const std::wstring FileName,
  const std::wstring NewName, bool Move)
{
  TMvSessionAction Action(GetLog(), AbsolutePath(FileName, true), AbsolutePath(NewName, true));
  try
  {
    assert(FFileSystem);
    FFileSystem->RenameFile(FileName, NewName);
  }
  catch (const std::exception & E)
  {
    COMMAND_ERROR_ARI_ACTION
    (
      FMTLOAD(Move ? MOVE_FILE_ERROR : RENAME_FILE_ERROR, FileName.c_str(), NewName.c_str()),
      DoRenameFile(FileName, NewName, Move),
      Action
    );
  }
}
//---------------------------------------------------------------------------
void TTerminal::MoveFile(const std::wstring FileName,
  const TRemoteFile * File, /*const TMoveFileParams*/ void * Param)
{
  if (GetOperationProgress() &&
      ((GetOperationProgress()->Operation == foRemoteMove) ||
       (GetOperationProgress()->Operation == foDelete)))
  {
    if (GetOperationProgress()->Cancel != csContinue) Abort();
    GetOperationProgress()->SetFile(FileName);
  }

  assert(Param != NULL);
  const TMoveFileParams & Params = *static_cast<const TMoveFileParams*>(Param);
  std::wstring NewName = UnixIncludeTrailingBackslash(Params.Target) +
    MaskFileName(UnixExtractFileName(FileName), Params.FileMask);
  LogEvent(FORMAT(L"Moving file \"%s\" to \"%s\".", FileName.c_str(), NewName.c_str()));
  FileModified(File, FileName);
  DoRenameFile(FileName, NewName, true);
  ReactOnCommand(fsMoveFile);
}
//---------------------------------------------------------------------------
bool TTerminal::MoveFiles(TStrings * FileList, const std::wstring Target,
  const std::wstring FileMask)
{
  TMoveFileParams Params;
  Params.Target = Target;
  Params.FileMask = FileMask;
  DirectoryModified(Target, true);
  bool Result;
  BeginTransaction();
  {
    BOOST_SCOPE_EXIT ( (&Self) (&FileList) )
    {
        if (Self->GetActive())
        {
          std::wstring WithTrailing = UnixIncludeTrailingBackslash(Self->GetCurrentDirectory());
          bool PossiblyMoved = false;
          // check if we was moving current directory.
          // this is just optimization to avoid checking existence of current
          // directory after each move operation.
          std::wstring curDirectory = Self->GetCurrentDirectory();
          for (int Index = 0; !PossiblyMoved && (Index < FileList->GetCount()); Index++)
          {
            const TRemoteFile *File =
              reinterpret_cast<const TRemoteFile *>(FileList->GetObject(Index));
            // File can be NULL, and filename may not be full path,
            // but currently this is the only way we can move (at least in GUI)
            // current directory
            if ((File != NULL) &&
                File->GetIsDirectory() &&
                ((curDirectory.substr(1, FileList->GetString(Index).size()) == FileList->GetString(Index)) &&
                 ((FileList->GetString(Index).size() == curDirectory.size()) ||
                  (curDirectory[FileList->GetString(Index).size() + 1] == '/'))))
            {
              PossiblyMoved = true;
            }
          }

          if (PossiblyMoved && !Self->FileExists(curDirectory))
          {
            std::wstring NearestExisting = curDirectory;
            do
            {
              NearestExisting = UnixExtractFileDir(NearestExisting);
            }
            while (!IsUnixRootPath(NearestExisting) && !Self->FileExists(NearestExisting));

            Self->ChangeDirectory(NearestExisting);
          }
        }
        Self->EndTransaction();
    } BOOST_SCOPE_EXIT_END
    Result = ProcessFiles(FileList, foRemoteMove, (TProcessFileEvent)&TTerminal::MoveFile, &Params);
  }
  return Result;
}
//---------------------------------------------------------------------------
void TTerminal::DoCopyFile(const std::wstring FileName,
  const std::wstring NewName)
{
  try
  {
    assert(FFileSystem);
    if (GetIsCapable(fcRemoteCopy))
    {
      FFileSystem->CopyFile(FileName, NewName);
    }
    else
    {
      assert(GetCommandSessionOpened());
      assert(FCommandSession->GetFSProtocol() == cfsSCP);
      LogEvent(L"Copying file on command session.");
      FCommandSession->GetCurrentDirectory() = GetCurrentDirectory();
      FCommandSession->FFileSystem->CopyFile(FileName, NewName);
    }
  }
  catch (const std::exception & E)
  {
    COMMAND_ERROR_ARI
    (
      FMTLOAD(COPY_FILE_ERROR, FileName.c_str(), NewName.c_str()),
      DoCopyFile(FileName, NewName)
    );
  }
}
//---------------------------------------------------------------------------
void TTerminal::CopyFile(const std::wstring FileName,
  const TRemoteFile * /*File*/, /*const TMoveFileParams*/ void * Param)
{
  if (GetOperationProgress() && (GetOperationProgress()->Operation == foRemoteCopy))
  {
    if (GetOperationProgress()->Cancel != csContinue) Abort();
    GetOperationProgress()->SetFile(FileName);
  }

  assert(Param != NULL);
  const TMoveFileParams & Params = *static_cast<const TMoveFileParams*>(Param);
  std::wstring NewName = UnixIncludeTrailingBackslash(Params.Target) +
    MaskFileName(UnixExtractFileName(FileName), Params.FileMask);
  LogEvent(FORMAT(L"Copying file \"%s\" to \"%s\".", FileName.c_str(), NewName.c_str()));
  DoCopyFile(FileName, NewName);
  ReactOnCommand(fsCopyFile);
}
//---------------------------------------------------------------------------
bool TTerminal::CopyFiles(TStrings * FileList, const std::wstring Target,
  const std::wstring FileMask)
{
  TMoveFileParams Params;
  Params.Target = Target;
  Params.FileMask = FileMask;
  DirectoryModified(Target, true);
  return ProcessFiles(FileList, foRemoteCopy, (TProcessFileEvent)&TTerminal::CopyFile, &Params);
}
//---------------------------------------------------------------------------
void TTerminal::CreateDirectory(const std::wstring DirName,
  const TRemoteProperties * Properties)
{
  assert(FFileSystem);
  EnsureNonExistence(DirName);
  FileModified(NULL, DirName);

  LogEvent(FORMAT(L"Creating directory \"%s\".", DirName.c_str()));
  DoCreateDirectory(DirName);

  if ((Properties != NULL) && !Properties->Valid.Empty())
  {
    DoChangeFileProperties(DirName, NULL, Properties);
  }

  ReactOnCommand(fsCreateDirectory);
}
//---------------------------------------------------------------------------
void TTerminal::DoCreateDirectory(const std::wstring DirName)
{
  TMkdirSessionAction Action(GetLog(), AbsolutePath(DirName, true));
  try
  {
    assert(FFileSystem);
    FFileSystem->CreateDirectory(DirName);
  }
  catch (const std::exception & E)
  {
    COMMAND_ERROR_ARI_ACTION
    (
      FMTLOAD(CREATE_DIR_ERROR, DirName.c_str()),
      DoCreateDirectory(DirName),
      Action
    );
  }
}
//---------------------------------------------------------------------------
void TTerminal::CreateLink(const std::wstring FileName,
  const std::wstring PointTo, bool Symbolic)
{
  assert(FFileSystem);
  EnsureNonExistence(FileName);
  if (GetSessionData()->GetCacheDirectories())
  {
    DirectoryModified(GetCurrentDirectory(), false);
  }

  LogEvent(FORMAT(L"Creating link \"%s\" to \"%s\" (symbolic: %s).",
    FileName.c_str(), PointTo.c_str(), BooleanToEngStr(Symbolic).c_str()));
  DoCreateLink(FileName, PointTo, Symbolic);
  ReactOnCommand(fsCreateDirectory);
}
//---------------------------------------------------------------------------
void TTerminal::DoCreateLink(const std::wstring FileName,
  const std::wstring PointTo, bool Symbolic)
{
  try
  {
    assert(FFileSystem);
    FFileSystem->CreateLink(FileName, PointTo, Symbolic);
  }
  catch (const std::exception & E)
  {
    COMMAND_ERROR_ARI
    (
      FMTLOAD(CREATE_LINK_ERROR, FileName.c_str()),
      DoCreateLink(FileName, PointTo, Symbolic);
    );
  }
}
//---------------------------------------------------------------------------
void TTerminal::HomeDirectory()
{
  assert(FFileSystem);
  try
  {
    LogEvent(L"Changing directory to home directory.");
    FFileSystem->HomeDirectory();
    ReactOnCommand(fsHomeDirectory);
  }
  catch (const std::exception &E)
  {
    CommandError(&E, LoadStr(CHANGE_HOMEDIR_ERROR));
  }
}
//---------------------------------------------------------------------------
void TTerminal::ChangeDirectory(const std::wstring Directory)
{
  assert(FFileSystem);
  try
  {
    std::wstring CachedDirectory;
    assert(!GetSessionData()->GetCacheDirectoryChanges() || (FDirectoryChangesCache != NULL));
    // never use directory change cache during startup, this ensures, we never
    // end-up initially in non-existing directory
    if ((GetStatus() == ssOpened) &&
        GetSessionData()->GetCacheDirectoryChanges() &&
        FDirectoryChangesCache->GetDirectoryChange(PeekCurrentDirectory(),
          Directory, CachedDirectory))
    {
      LogEvent(FORMAT(L"Cached directory change via \"%s\" to \"%s\".",
        Directory.c_str(), CachedDirectory.c_str()));
      FFileSystem->CachedChangeDirectory(CachedDirectory);
    }
    else
    {
      LogEvent(FORMAT(L"Changing directory to \"%s\".", Directory.c_str()));
      FFileSystem->ChangeDirectory(Directory);
    }
    FLastDirectoryChange = Directory;
    ReactOnCommand(fsChangeDirectory);
  }
  catch (const std::exception &E)
  {
    CommandError(&E, FMTLOAD(CHANGE_DIR_ERROR, Directory.c_str()));
  }
}
//---------------------------------------------------------------------------
void TTerminal::LookupUsersGroups()
{
  if (!FUsersGroupsLookedup && GetSessionData()->GetLookupUserGroups() &&
      GetIsCapable(fcUserGroupListing))
  {
    assert(FFileSystem);

    try
    {
      FUsersGroupsLookedup = true;
      LogEvent(L"Looking up groups and users.");
      FFileSystem->LookupUsersGroups();
      ReactOnCommand(fsLookupUsersGroups);

      if (GetLog()->GetLogging())
      {
        FGroups.Log(this, "groups");
        FGroups.Log(this, "membership");
        FGroups.Log(this, "users");
      }
    }
    catch (const std::exception &E)
    {
      CommandError(&E, LoadStr(LOOKUP_GROUPS_ERROR));
    }
  }
}
//---------------------------------------------------------------------------
bool TTerminal::AllowedAnyCommand(const std::wstring Command)
{
  return !::Trim(Command).empty();
}
//---------------------------------------------------------------------------
bool TTerminal::GetCommandSessionOpened()
{
  // consider secodary terminal open in "ready" state only
  // so we never do keepalives on it until it is completelly initialised
  return (FCommandSession != NULL) &&
    (FCommandSession->GetStatus() == ssOpened);
}
//---------------------------------------------------------------------------
TTerminal * TTerminal::GetCommandSession()
{
  if ((FCommandSession != NULL) && !FCommandSession->GetActive())
  {
    SAFE_DESTROY(FCommandSession);
  }

  if (FCommandSession == NULL)
  {
    // transaction cannot be started yet to allow proper matching transation
    // levels between main and command session
    assert(FInTransaction == 0);

    try
    {
      FCommandSession = new TSecondaryTerminal(this, GetSessionData(),
        Configuration, L"Shell");

      FCommandSession->SetAutoReadDirectory(false);

      TSessionData * CommandSessionData = FCommandSession->FSessionData;
      CommandSessionData->SetRemoteDirectory(GetCurrentDirectory());
      CommandSessionData->SetFSProtocol(fsSCPonly);
      CommandSessionData->SetClearAliases(false);
      CommandSessionData->SetUnsetNationalVars(false);
      CommandSessionData->SetLookupUserGroups(false);

      FCommandSession->FExceptionOnFail = FExceptionOnFail;

      FCommandSession->SetOnQueryUser(GetOnQueryUser());
      FCommandSession->SetOnPromptUser(GetOnPromptUser());
      FCommandSession->SetOnShowExtendedException(GetOnShowExtendedException());
      FCommandSession->SetOnProgress(GetOnProgress());
      FCommandSession->SetOnFinished(GetOnFinished());
      FCommandSession->SetOnInformation(GetOnInformation());
      // do not copy OnDisplayBanner to avoid it being displayed
    }
    catch (...)
    {
      SAFE_DESTROY(FCommandSession);
      throw;
    }
  }

  return FCommandSession;
}
//---------------------------------------------------------------------------
void TTerminal::AnyCommand(const std::wstring Command,
  const captureoutput_slot_type *OutputEvent)
{

  class TOutputProxy
  {
  public:
    TOutputProxy(TCallSessionAction & Action, const captureoutput_slot_type *OutputEvent) :
      FAction(Action)
    {
      FOutputEvent.connect(*OutputEvent);
    }

    void Output(const std::wstring & Str, bool StdError)
    {
      FAction.AddOutput(Str, StdError);
      if (!FOutputEvent.empty())
      {
        FOutputEvent(Str, StdError);
      }
    }

  private:
    TCallSessionAction &FAction;
    captureoutput_signal_type FOutputEvent;
  private:
    TOutputProxy(const TOutputProxy &);
    void operator=(const TOutputProxy &);
  };

  TCallSessionAction Action(GetLog(), Command, GetCurrentDirectory());
  TOutputProxy ProxyOutputEvent(Action, OutputEvent);
  DoAnyCommand(Command, boost::bind(&TOutputProxy::Output, &ProxyOutputEvent, _1, _2),
    &Action);
}
//---------------------------------------------------------------------------
void TTerminal::DoAnyCommand(const std::wstring Command,
  const captureoutput_slot_type &OutputEvent, TCallSessionAction * Action)
{
  assert(FFileSystem);
  try
  {
    DirectoryModified(GetCurrentDirectory(), false);
    if (GetIsCapable(fcAnyCommand))
    {
      LogEvent(L"Executing user defined command.");
      FFileSystem->AnyCommand(Command, &OutputEvent);
    }
    else
    {
      assert(GetCommandSessionOpened());
      assert(FCommandSession->GetFSProtocol() == cfsSCP);
      LogEvent(L"Executing user defined command on command session.");

      FCommandSession->SetCurrentDirectory(GetCurrentDirectory());
      FCommandSession->FFileSystem->AnyCommand(Command, &OutputEvent);

      FCommandSession->FFileSystem->ReadCurrentDirectory();

      // synchronize pwd (by purpose we lose transaction optimalisation here)
      ChangeDirectory(FCommandSession->GetCurrentDirectory());
    }
    ReactOnCommand(fsAnyCommand);
  }
  catch (const std::exception &E)
  {
    if (Action != NULL)
    {
      RollbackAction(*Action, NULL, &E);
    }
    if (GetExceptionOnFail()) throw; // FIXME  || (E.InheritsFrom(__classid(EFatal)))) throw;
      else HandleExtendedException(&E);
  }
}
//---------------------------------------------------------------------------
bool TTerminal::DoCreateLocalFile(const std::wstring FileName,
  TFileOperationProgressType * OperationProgress, HANDLE * AHandle,
  bool NoConfirmation)
{
  bool Result = true;
  bool Done;
  unsigned int CreateAttr = FILE_ATTRIBUTE_NORMAL;
  do
  {
    *AHandle = CreateFile(FileName.c_str(), GENERIC_WRITE, FILE_SHARE_READ,
      NULL, CREATE_ALWAYS, CreateAttr, 0);
    Done = (*AHandle != INVALID_HANDLE_VALUE);
    if (!Done)
    {
      int FileAttr;
      if (::FileExists(FileName) &&
        (((FileAttr = FileGetAttr(FileName)) & (faReadOnly | faHidden)) != 0))
      {
        if (FLAGSET(FileAttr, faReadOnly))
        {
          if (OperationProgress->BatchOverwrite == boNone)
          {
            Result = false;
          }
          else if ((OperationProgress->BatchOverwrite != boAll) && !NoConfirmation)
          {
            int Answer;
            SUSPEND_OPERATION
            (
              Answer = QueryUser(
                FMTLOAD(READ_ONLY_OVERWRITE, FileName.c_str()),
                NULL,
                qaYes | qaNo | qaCancel | qaYesToAll | qaNoToAll, 0);
            );
            switch (Answer) {
              case qaYesToAll: OperationProgress->BatchOverwrite = boAll; break;
              case qaCancel: OperationProgress->Cancel = csCancel; // continue on next case
              case qaNoToAll: OperationProgress->BatchOverwrite = boNone;
              case qaNo: Result = false; break;
            }
          }
        }
        else
        {
          assert(FLAGSET(FileAttr, faHidden));
          Result = true;
        }

        if (Result)
        {
          CreateAttr |=
            FLAGMASK(FLAGSET(FileAttr, faHidden), FILE_ATTRIBUTE_HIDDEN) |
            FLAGMASK(FLAGSET(FileAttr, faReadOnly), FILE_ATTRIBUTE_READONLY);
            FILE_OPERATION_LOOP (FMTLOAD(CANT_SET_ATTRS, FileName.c_str()),
            if (FileSetAttr(FileName, FileAttr & ~(faReadOnly | faHidden)) != 0)
            {
              RaiseLastOSError();
            }
          );
        }
        else
        {
          Done = true;
        }
      }
      else
      {
        RaiseLastOSError();
      }
    }
  }
  while (!Done);

  return Result;
}
//---------------------------------------------------------------------------
bool TTerminal::CreateLocalFile(const std::wstring FileName,
  TFileOperationProgressType * OperationProgress, HANDLE * AHandle,
  bool NoConfirmation)
{
  assert(AHandle);
  bool Result = true;

  FILE_OPERATION_LOOP (FMTLOAD(CREATE_FILE_ERROR, FileName.c_str()),
    Result = DoCreateLocalFile(FileName, OperationProgress, AHandle, NoConfirmation);
  );

  return Result;
}
//---------------------------------------------------------------------------
void TTerminal::OpenLocalFile(const std::wstring FileName,
  int Access, int * AAttrs, HANDLE * AHandle, __int64 * ACTime,
  __int64 * AMTime, __int64 * AATime, __int64 * ASize,
  bool TryWriteReadOnly)
{
  int Attrs = 0;
  HANDLE Handle = 0;
  TFileOperationProgressType * OperationProgress = GetOperationProgress();
  FILE_OPERATION_LOOP (FMTLOAD(FILE_NOT_EXISTS, FileName.c_str()),
    Attrs = FileGetAttr(FileName);
    if (Attrs == -1) RaiseLastOSError();
  )

  if ((Attrs & faDirectory) == 0)
  {
    bool NoHandle = false;
    if (!TryWriteReadOnly && (Access == GENERIC_WRITE) &&
        ((Attrs & faReadOnly) != 0))
    {
      Access = GENERIC_READ;
      NoHandle = true;
    }

    FILE_OPERATION_LOOP (FMTLOAD(OPENFILE_ERROR, FileName.c_str()),
      Handle = CreateFile(FileName.c_str(), Access,
        Access == GENERIC_READ ? FILE_SHARE_READ | FILE_SHARE_WRITE : FILE_SHARE_READ,
        NULL, OPEN_EXISTING, 0, 0);
      if (Handle == INVALID_HANDLE_VALUE)
      {
        Handle = 0;
        RaiseLastOSError();
      }
    );

    try
    {
      if (AATime || AMTime || ACTime)
      {
        FILETIME ATime;
        FILETIME MTime;
        FILETIME CTime;
        // Get last file access and modification time
        FILE_OPERATION_LOOP (FMTLOAD(CANT_GET_ATTRS, FileName.c_str()),
          if (!GetFileTime(Handle, &CTime, &ATime, &MTime)) RaiseLastOSError();
        );
        if (ACTime)
        {
          *ACTime = ConvertTimestampToUnixSafe(CTime, GetSessionData()->GetDSTMode());
        }
        if (AATime)
        {
          *AATime = ConvertTimestampToUnixSafe(ATime, GetSessionData()->GetDSTMode());
        }
        if (AMTime)
        {
          *AMTime = ConvertTimestampToUnix(MTime, GetSessionData()->GetDSTMode());
        }
      }

      if (ASize)
      {
        // Get file size
        FILE_OPERATION_LOOP (FMTLOAD(CANT_GET_ATTRS, FileName.c_str()),
          unsigned long LSize;
          unsigned long HSize;
          LSize = GetFileSize(Handle, &HSize);
          if ((LSize == 0xFFFFFFFF) && (GetLastError() != NO_ERROR)) RaiseLastOSError();
          *ASize = (__int64(HSize) << 32) + LSize;
        );
      }

      if ((AHandle == NULL) || NoHandle)
      {
        CloseHandle(Handle);
        Handle = NULL;
      }
    }
    catch (...)
    {
      CloseHandle(Handle);
      throw;
    }
  }

  if (AAttrs) *AAttrs = Attrs;
  if (AHandle) *AHandle = Handle;
}
//---------------------------------------------------------------------------
bool TTerminal::AllowLocalFileTransfer(std::wstring FileName,
  const TCopyParamType * CopyParam)
{
  bool Result = true;
  if (!CopyParam->AllowAnyTransfer())
  {
    WIN32_FIND_DATA FindData;
    HANDLE Handle;
    TFileOperationProgressType * OperationProgress = GetOperationProgress();
    FILE_OPERATION_LOOP (FMTLOAD(FILE_NOT_EXISTS, FileName.c_str()),
      Handle = FindFirstFile(FileName.c_str(), &FindData);
      if (Handle == INVALID_HANDLE_VALUE)
      {
        Abort();
      }
    )
    ::FindClose(Handle);
    bool Directory = FLAGSET(FindData.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY);
    TFileMasks::TParams Params;
    Params.Size =
      (static_cast<__int64>(FindData.nFileSizeHigh) << 32) +
      FindData.nFileSizeLow;
    Result = CopyParam->AllowTransfer(FileName, osLocal, Directory, Params);
  }
  return Result;
}
//---------------------------------------------------------------------------
std::wstring TTerminal::FileUrl(const std::wstring Protocol,
  const std::wstring FileName)
{
  assert(FileName.size() > 0);
  return Protocol + L"://" + EncodeUrlChars(GetSessionData()->GetSessionName()) +
    (FileName[1] == '/' ? L"" : L"/") + EncodeUrlChars(FileName, L"/");
}
//---------------------------------------------------------------------------
std::wstring TTerminal::FileUrl(const std::wstring FileName)
{
  return FFileSystem->FileUrl(FileName);
}
//---------------------------------------------------------------------------
void TTerminal::MakeLocalFileList(const std::wstring FileName,
  const WIN32_FIND_DATA Rec, void * Param)
{
  TMakeLocalFileListParams & Params = *static_cast<TMakeLocalFileListParams*>(Param);

  bool Directory = FLAGSET(Rec.dwFileAttributes, faDirectory);
  if (Directory && Params.Recursive)
  {
    // FIXME ProcessLocalDirectory(FileName, (TProcessFileEvent)&TTerminal::MakeLocalFileList, &Params);
  }

  if (!Directory || Params.IncludeDirs)
  {
    Params.FileList->Add(FileName);
  }
}
//---------------------------------------------------------------------------
void TTerminal::CalculateLocalFileSize(const std::wstring FileName,
  const WIN32_FIND_DATA Rec, /*TCalculateSizeParams*/ void * Params)
{
  TCalculateSizeParams * AParams = static_cast<TCalculateSizeParams*>(Params);

  bool Dir = FLAGSET(Rec.dwFileAttributes, faDirectory);

  bool AllowTransfer = (AParams->CopyParam == NULL);
  __int64 Size =
    (static_cast<__int64>(Rec.nFileSizeHigh) << 32) +
    Rec.nFileSizeLow;
  if (!AllowTransfer)
  {
    TFileMasks::TParams MaskParams;
    MaskParams.Size = Size;

    AllowTransfer = AParams->CopyParam->AllowTransfer(FileName, osLocal, Dir, MaskParams);
  }

  if (AllowTransfer)
  {
    if (!Dir)
    {
      AParams->Size += Size;
    }
    else
    {
      // FIXME ProcessLocalDirectory(FileName, (TProcessFileEvent)&TTerminal::CalculateLocalFileSize, Params);
    }
  }

  if (GetOperationProgress() && GetOperationProgress()->Operation == foCalculateSize)
  {
    if (GetOperationProgress()->Cancel != csContinue) Abort();
    GetOperationProgress()->SetFile(FileName);
  }
}
//---------------------------------------------------------------------------
void TTerminal::CalculateLocalFilesSize(TStrings * FileList,
  __int64 & Size, const TCopyParamType * CopyParam)
{
  TFileOperationProgressType *OperationProgress = new TFileOperationProgressType(boost::bind(&TTerminal::DoProgress, this, _1, _2),
    boost::bind(&TTerminal::DoFinished, this, _1, _2, _3, _4, _5, _6));
  TOnceDoneOperation OnceDoneOperation = odoIdle;
  OperationProgress->Start(foCalculateSize, osLocal, FileList->GetCount());
  {
    BOOST_SCOPE_EXIT ( (&Self) (&OperationProgress) )
    {
      Self->FOperationProgress = NULL;
      OperationProgress->Stop();
    } BOOST_SCOPE_EXIT_END
    TCalculateSizeParams Params;
    Params.Size = 0;
    Params.Params = 0;
    Params.CopyParam = CopyParam;

    assert(!FOperationProgress);
    FOperationProgress = OperationProgress;
    WIN32_FIND_DATA Rec;
    for (int Index = 0; Index < FileList->GetCount(); Index++)
    {
      std::wstring FileName = FileList->GetString(Index);
      if (FileSearchRec(FileName, Rec))
      {
        CalculateLocalFileSize(FileName, Rec, &Params);
        OperationProgress->Finish(FileName, true, OnceDoneOperation);
      }
    }

    Size = Params.Size;
  }

  if (OnceDoneOperation != odoIdle)
  {
    CloseOnCompletion(OnceDoneOperation);
  }
}
//---------------------------------------------------------------------------
struct TSynchronizeFileData
{
  bool Modified;
  bool New;
  bool IsDirectory;
  TSynchronizeChecklist::TItem::TFileInfo Info;
  TSynchronizeChecklist::TItem::TFileInfo MatchingRemoteFile;
  TRemoteFile * MatchingRemoteFileFile;
  int MatchingRemoteFileImageIndex;
  FILETIME LocalLastWriteTime;
};
//---------------------------------------------------------------------------
const int sfFirstLevel = 0x01;
struct TSynchronizeData
{
  std::wstring LocalDirectory;
  std::wstring RemoteDirectory;
  TTerminal::TSynchronizeMode Mode;
  int Params;
  TSynchronizeDirectory OnSynchronizeDirectory;
  TSynchronizeOptions * Options;
  int Flags;
  TStringList * LocalFileList;
  const TCopyParamType * CopyParam;
  TSynchronizeChecklist * Checklist;
};
//---------------------------------------------------------------------------
TSynchronizeChecklist * TTerminal::SynchronizeCollect(const std::wstring LocalDirectory,
  const std::wstring RemoteDirectory, TSynchronizeMode Mode,
  const TCopyParamType * CopyParam, int Params,
  TSynchronizeDirectory OnSynchronizeDirectory,
  TSynchronizeOptions * Options)
{
  TSynchronizeChecklist * Checklist = new TSynchronizeChecklist();
  try
  {
    DoSynchronizeCollectDirectory(LocalDirectory, RemoteDirectory, Mode,
      CopyParam, Params, OnSynchronizeDirectory, Options, sfFirstLevel,
      Checklist);
    Checklist->Sort();
  }
  catch (...)
  {
    delete Checklist;
    throw;
  }
  return Checklist;
}
//---------------------------------------------------------------------------
void TTerminal::DoSynchronizeCollectDirectory(const std::wstring LocalDirectory,
  const std::wstring RemoteDirectory, TSynchronizeMode Mode,
  const TCopyParamType *CopyParam, int Params,
  TSynchronizeDirectory OnSynchronizeDirectory, TSynchronizeOptions *Options,
  int Flags, TSynchronizeChecklist *Checklist)
{
  TSynchronizeData Data;

  Data.LocalDirectory = IncludeTrailingBackslash(LocalDirectory);
  Data.RemoteDirectory = UnixIncludeTrailingBackslash(RemoteDirectory);
  Data.Mode = Mode;
  Data.Params = Params;
  Data.OnSynchronizeDirectory = OnSynchronizeDirectory;
  Data.LocalFileList = NULL;
  Data.CopyParam = CopyParam;
  Data.Options = Options;
  Data.Flags = Flags;
  Data.Checklist = Checklist;

  LogEvent(FORMAT(L"Collecting synchronization list for local directory '%s' and remote directory '%s', "
    L"mode = %d, params = %d", LocalDirectory.c_str(), RemoteDirectory.c_str(),
    int(Mode), int(Params)));

  if (FLAGCLEAR(Params, spDelayProgress))
  {
    DoSynchronizeProgress(Data, true);
  }

  {
    BOOST_SCOPE_EXIT ( (&Self) (&Data) )
    {
      if (Data.LocalFileList != NULL)
      {
        for (int Index = 0; Index < Data.LocalFileList->GetCount(); Index++)
        {
          TSynchronizeFileData *FileData = reinterpret_cast<TSynchronizeFileData*>
            (Data.LocalFileList->GetObject(Index));
          delete FileData;
        }
        delete Data.LocalFileList;
      }
    } BOOST_SCOPE_EXIT_END
    bool Found = false;
    WIN32_FIND_DATA SearchRec;
    Data.LocalFileList = new TStringList();
    Data.LocalFileList->SetSorted(true);
    Data.LocalFileList->SetCaseSensitive(false);
    TFileOperationProgressType *OperationProgress = GetOperationProgress();
    FILE_OPERATION_LOOP (FMTLOAD(LIST_DIR_ERROR, LocalDirectory.c_str()),
      int FindAttrs = faReadOnly | faHidden | faSysFile | faDirectory | faArchive;
      // FIXME Found = (FindFirst(Data.LocalDirectory + L"*.*", FindAttrs, SearchRec) == 0);
    );

    if (Found)
    {
      {
        BOOST_SCOPE_EXIT ( (&Self) )
        {
          // FIXME Self->FindClose(SearchRec);
        } BOOST_SCOPE_EXIT_END
        std::wstring FileName;
        while (Found)
        {
          FileName = SearchRec.cFileName;
          // add dirs for recursive mode or when we are interested in newly
          // added subdirs
          int FoundIndex;
          __int64 Size =
            (static_cast<__int64>(SearchRec.nFileSizeHigh) << 32) +
            SearchRec.nFileSizeLow;
          TFileMasks::TParams MaskParams;
          MaskParams.Size = Size;
          std::wstring RemoteFileName =
            CopyParam->ChangeFileName(FileName, osLocal, false);
          if ((FileName != L".") && (FileName != L"..") &&
              CopyParam->AllowTransfer(Data.LocalDirectory + FileName, osLocal,
                FLAGSET(SearchRec.dwFileAttributes, faDirectory), MaskParams) &&
              !FFileSystem->TemporaryTransferFile(FileName) &&
              (FLAGCLEAR(Flags, sfFirstLevel) ||
               (Options == NULL) || (Options->Filter == NULL) ||
               Options->Filter->Find(FileName, FoundIndex) ||
               Options->Filter->Find(RemoteFileName, FoundIndex)))
          {
            TSynchronizeFileData * FileData = new TSynchronizeFileData;

            FileData->IsDirectory = FLAGSET(SearchRec.dwFileAttributes, faDirectory);
            FileData->Info.FileName = FileName;
            FileData->Info.Directory = Data.LocalDirectory;
            FileData->Info.Modification = FileTimeToDateTime(SearchRec.ftLastWriteTime);
            FileData->Info.ModificationFmt = mfFull;
            FileData->Info.Size = Size;
            FileData->LocalLastWriteTime = SearchRec.ftLastWriteTime;
            FileData->New = true;
            FileData->Modified = false;
            Data.LocalFileList->AddObject(FileName,
              reinterpret_cast<TObject*>(FileData));
          }

          FILE_OPERATION_LOOP (FMTLOAD(LIST_DIR_ERROR, LocalDirectory.c_str()),
            // FIXME Found = (FindNext(SearchRec) == 0);
          );
        }
      }

      // can we expect that ProcessDirectory would take so little time
      // that we can pospone showing progress window until anything actually happens?
      bool Cached = FLAGSET(Params, spUseCache) && GetSessionData()->GetCacheDirectories() &&
        FDirectoryCache->HasFileList(RemoteDirectory);

      if (!Cached && FLAGSET(Params, spDelayProgress))
      {
        DoSynchronizeProgress(Data, true);
      }

      ProcessDirectory(RemoteDirectory, (TProcessFileEvent)&TTerminal::SynchronizeCollectFile, &Data,
        FLAGSET(Params, spUseCache));

      TSynchronizeFileData * FileData;
      for (int Index = 0; Index < Data.LocalFileList->GetCount(); Index++)
      {
        FileData = reinterpret_cast<TSynchronizeFileData *>
          (Data.LocalFileList->GetObject(Index));
        // add local file either if we are going to upload it
        // (i.e. if it is updated or we want to upload even new files)
        // or if we are going to delete it (i.e. all "new"=obsolete files)
        bool Modified = (FileData->Modified && ((Mode == smBoth) || (Mode == smRemote)));
        bool New = (FileData->New &&
          ((Mode == smLocal) ||
           (((Mode == smBoth) || (Mode == smRemote)) && FLAGCLEAR(Params, spTimestamp))));
        if (Modified || New)
        {
          TSynchronizeChecklist::TItem * ChecklistItem = new TSynchronizeChecklist::TItem();
          {
            BOOST_SCOPE_EXIT ( (&ChecklistItem) )
            {
              delete ChecklistItem;
            } BOOST_SCOPE_EXIT_END
            ChecklistItem->IsDirectory = FileData->IsDirectory;

            ChecklistItem->Local = FileData->Info;
            ChecklistItem->FLocalLastWriteTime = FileData->LocalLastWriteTime;

            if (Modified)
            {
              assert(!FileData->MatchingRemoteFile.Directory.empty());
              ChecklistItem->Remote = FileData->MatchingRemoteFile;
              ChecklistItem->ImageIndex = FileData->MatchingRemoteFileImageIndex;
              ChecklistItem->RemoteFile = FileData->MatchingRemoteFileFile;
            }
            else
            {
              ChecklistItem->Remote.Directory = Data.RemoteDirectory;
            }

            if ((Mode == smBoth) || (Mode == smRemote))
            {
              ChecklistItem->Action =
                (Modified ? TSynchronizeChecklist::saUploadUpdate : TSynchronizeChecklist::saUploadNew);
              ChecklistItem->Checked =
                (Modified || FLAGCLEAR(Params, spExistingOnly)) &&
                (!ChecklistItem->IsDirectory || FLAGCLEAR(Params, spNoRecurse) ||
                 FLAGSET(Params, spSubDirs));
            }
            else if ((Mode == smLocal) && FLAGCLEAR(Params, spTimestamp))
            {
              ChecklistItem->Action = TSynchronizeChecklist::saDeleteLocal;
              ChecklistItem->Checked =
                FLAGSET(Params, spDelete) &&
                (!ChecklistItem->IsDirectory || FLAGCLEAR(Params, spNoRecurse) ||
                 FLAGSET(Params, spSubDirs));
            }

            if (ChecklistItem->Action != TSynchronizeChecklist::saNone)
            {
              Data.Checklist->Add(ChecklistItem);
              ChecklistItem = NULL;
            }
          }
        }
        else
        {
          if (FileData->Modified)
          {
            delete FileData->MatchingRemoteFileFile;
          }
        }
      }
    }
  }
}
//---------------------------------------------------------------------------
void TTerminal::SynchronizeCollectFile(const std::wstring FileName,
  const TRemoteFile * File, /*TSynchronizeData*/ void * Param)
{
  TSynchronizeData * Data = static_cast<TSynchronizeData *>(Param);

  int FoundIndex;
  TFileMasks::TParams MaskParams;
  MaskParams.Size = File->GetSize();
  std::wstring LocalFileName =
    Data->CopyParam->ChangeFileName(File->GetFileName(), osRemote, false);
  if (Data->CopyParam->AllowTransfer(
        UnixExcludeTrailingBackslash(File->GetFullFileName()), osRemote,
        File->GetIsDirectory(), MaskParams) &&
      !FFileSystem->TemporaryTransferFile(File->GetFileName()) &&
      (FLAGCLEAR(Data->Flags, sfFirstLevel) ||
       (Data->Options == NULL) || (Data->Options->Filter == NULL) ||
        Data->Options->Filter->Find(File->GetFileName(), FoundIndex) ||
        Data->Options->Filter->Find(LocalFileName, FoundIndex)))
  {
    TSynchronizeChecklist::TItem * ChecklistItem = new TSynchronizeChecklist::TItem();
    {
      BOOST_SCOPE_EXIT ( (&ChecklistItem) )
      {
        delete ChecklistItem;
      }
      BOOST_SCOPE_EXIT_END
      ChecklistItem->IsDirectory = File->GetIsDirectory();
      ChecklistItem->ImageIndex = File->GetIconIndex();
      ChecklistItem->ImageIndex = File->GetIconIndex();

      ChecklistItem->Remote.FileName = File->GetFileName();
      ChecklistItem->Remote.Directory = Data->RemoteDirectory;
      ChecklistItem->Remote.Modification = File->GetModification();
      ChecklistItem->Remote.ModificationFmt = File->GetModificationFmt();
      ChecklistItem->Remote.Size = File->GetSize();

      bool Modified = false;
      int LocalIndex = Data->LocalFileList->IndexOf(LocalFileName.c_str());
      bool New = (LocalIndex < 0);
      if (!New)
      {
        TSynchronizeFileData * LocalData =
          reinterpret_cast<TSynchronizeFileData *>(Data->LocalFileList->GetObject(LocalIndex));

        LocalData->New = false;

        if (File->GetIsDirectory() != LocalData->IsDirectory)
        {
          LogEvent(FORMAT(L"%s is directory on one side, but file on the another",
            File->GetFileName().c_str()));
        }
        else if (!File->GetIsDirectory())
        {
          ChecklistItem->Local = LocalData->Info;

          ChecklistItem->Local.Modification =
            ReduceDateTimePrecision(ChecklistItem->Local.Modification, File->GetModificationFmt());

          bool LocalModified = false;
          // for spTimestamp+spBySize require that the file sizes are the same
          // before comparing file time
          int TimeCompare;
          if (FLAGCLEAR(Data->Params, spNotByTime) &&
              (FLAGCLEAR(Data->Params, spTimestamp) ||
               FLAGCLEAR(Data->Params, spBySize) ||
               (ChecklistItem->Local.Size == ChecklistItem->Remote.Size)))
          {
            TimeCompare = CompareFileTime(ChecklistItem->Local.Modification,
                 ChecklistItem->Remote.Modification);
          }
          else
          {
            TimeCompare = 0;
          }
          if (TimeCompare < 0)
          {
            if ((FLAGCLEAR(Data->Params, spTimestamp) && FLAGCLEAR(Data->Params, spMirror)) ||
                (Data->Mode == smBoth) || (Data->Mode == smLocal))
            {
              Modified = true;
            }
            else
            {
              LocalModified = true;
            }
          }
          else if (TimeCompare > 0)
          {
            if ((FLAGCLEAR(Data->Params, spTimestamp) && FLAGCLEAR(Data->Params, spMirror)) ||
                (Data->Mode == smBoth) || (Data->Mode == smRemote))
            {
              LocalModified = true;
            }
            else
            {
              Modified = true;
            }
          }
          else if (FLAGSET(Data->Params, spBySize) &&
                   (ChecklistItem->Local.Size != ChecklistItem->Remote.Size) &&
                   FLAGCLEAR(Data->Params, spTimestamp))
          {
            Modified = true;
            LocalModified = true;
          }

          if (LocalModified)
          {
            LocalData->Modified = true;
            LocalData->MatchingRemoteFile = ChecklistItem->Remote;
            LocalData->MatchingRemoteFileImageIndex = ChecklistItem->ImageIndex;
            // we need this for custom commands over checklist only,
            // not for sync itself
            LocalData->MatchingRemoteFileFile = File->Duplicate();
          }
        }
        else if (FLAGCLEAR(Data->Params, spNoRecurse))
        {
          DoSynchronizeCollectDirectory(
            Data->LocalDirectory + LocalData->Info.FileName,
            Data->RemoteDirectory + File->GetFileName(),
            Data->Mode, Data->CopyParam, Data->Params, Data->OnSynchronizeDirectory,
            Data->Options, (Data->Flags & ~sfFirstLevel),
            Data->Checklist);
        }
      }
      else
      {
        ChecklistItem->Local.Directory = Data->LocalDirectory;
      }

      if (New || Modified)
      {
        assert(!New || !Modified);

        // download the file if it changed or is new and we want to have it locally
        if ((Data->Mode == smBoth) || (Data->Mode == smLocal))
        {
          if (FLAGCLEAR(Data->Params, spTimestamp) || Modified)
          {
            ChecklistItem->Action =
              (Modified ? TSynchronizeChecklist::saDownloadUpdate : TSynchronizeChecklist::saDownloadNew);
            ChecklistItem->Checked =
              (Modified || FLAGCLEAR(Data->Params, spExistingOnly)) &&
              (!ChecklistItem->IsDirectory || FLAGCLEAR(Data->Params, spNoRecurse) ||
               FLAGSET(Data->Params, spSubDirs));
          }
        }
        else if ((Data->Mode == smRemote) && New)
        {
          if (FLAGCLEAR(Data->Params, spTimestamp))
          {
            ChecklistItem->Action = TSynchronizeChecklist::saDeleteRemote;
            ChecklistItem->Checked =
              FLAGSET(Data->Params, spDelete) &&
              (!ChecklistItem->IsDirectory || FLAGCLEAR(Data->Params, spNoRecurse) ||
               FLAGSET(Data->Params, spSubDirs));
          }
        }

        if (ChecklistItem->Action != TSynchronizeChecklist::saNone)
        {
          ChecklistItem->RemoteFile = File->Duplicate();
          Data->Checklist->Add(ChecklistItem);
          ChecklistItem = NULL;
        }
      }
    }
  }
}
//---------------------------------------------------------------------------
void TTerminal::SynchronizeApply(TSynchronizeChecklist * Checklist,
  const std::wstring LocalDirectory, const std::wstring RemoteDirectory,
  const TCopyParamType * CopyParam, int Params,
  TSynchronizeDirectory OnSynchronizeDirectory)
{
  TSynchronizeData Data;

  Data.OnSynchronizeDirectory = OnSynchronizeDirectory;

  int CopyParams =
    FLAGMASK(FLAGSET(Params, spNoConfirmation), cpNoConfirmation);

  TCopyParamType SyncCopyParam = *CopyParam;
  // when synchronizing by time, we force preserving time,
  // otherwise it does not make any sense
  if (FLAGCLEAR(Params, spNotByTime))
  {
    SyncCopyParam.SetPreserveTime(true);
  }

  TStringList * DownloadList = new TStringList();
  TStringList * DeleteRemoteList = new TStringList();
  TStringList * UploadList = new TStringList();
  TStringList * DeleteLocalList = new TStringList();

  BeginTransaction();

  {
    BOOST_SCOPE_EXIT ( (&Self) (&DownloadList) (&DeleteRemoteList)
      (&UploadList) (&DeleteLocalList) )
    {
        delete DownloadList;
        delete DeleteRemoteList;
        delete UploadList;
        delete DeleteLocalList;

        Self->EndTransaction();
    } BOOST_SCOPE_EXIT_END
    int IIndex = 0;
    while (IIndex < Checklist->GetCount())
    {
      const TSynchronizeChecklist::TItem * ChecklistItem;

      DownloadList->Clear();
      DeleteRemoteList->Clear();
      UploadList->Clear();
      DeleteLocalList->Clear();

      ChecklistItem = Checklist->GetItem(IIndex);

      std::wstring CurrentLocalDirectory = ChecklistItem->Local.Directory;
      std::wstring CurrentRemoteDirectory = ChecklistItem->Remote.Directory;

      LogEvent(FORMAT(L"Synchronizing local directory '%s' with remote directory '%s', "
        L"params = %d", CurrentLocalDirectory.c_str(), CurrentRemoteDirectory.c_str(), int(Params)));

      int Count = 0;

      while ((IIndex < Checklist->GetCount()) &&
             (Checklist->GetItem(IIndex)->Local.Directory == CurrentLocalDirectory) &&
             (Checklist->GetItem(IIndex)->Remote.Directory == CurrentRemoteDirectory))
      {
        ChecklistItem = Checklist->GetItem(IIndex);
        if (ChecklistItem->Checked)
        {
          Count++;

          if (FLAGSET(Params, spTimestamp))
          {
            switch (ChecklistItem->Action)
            {
              case TSynchronizeChecklist::saDownloadUpdate:
                DownloadList->AddObject(
                  UnixIncludeTrailingBackslash(ChecklistItem->Remote.Directory) +
                    ChecklistItem->Remote.FileName,
                  (TObject *)(ChecklistItem));
                break;

              case TSynchronizeChecklist::saUploadUpdate:
                UploadList->AddObject(
                  IncludeTrailingBackslash(ChecklistItem->Local.Directory) +
                    ChecklistItem->Local.FileName,
                  (TObject *)(ChecklistItem));
                break;

              default:
                assert(false);
                break;
            }
          }
          else
          {
            switch (ChecklistItem->Action)
            {
              case TSynchronizeChecklist::saDownloadNew:
              case TSynchronizeChecklist::saDownloadUpdate:
                DownloadList->AddObject(
                  UnixIncludeTrailingBackslash(ChecklistItem->Remote.Directory) +
                    ChecklistItem->Remote.FileName,
                  ChecklistItem->RemoteFile);
                break;

              case TSynchronizeChecklist::saDeleteRemote:
                DeleteRemoteList->AddObject(
                  UnixIncludeTrailingBackslash(ChecklistItem->Remote.Directory) +
                    ChecklistItem->Remote.FileName,
                  ChecklistItem->RemoteFile);
                break;

              case TSynchronizeChecklist::saUploadNew:
              case TSynchronizeChecklist::saUploadUpdate:
                UploadList->Add(
                  IncludeTrailingBackslash(ChecklistItem->Local.Directory) +
                    ChecklistItem->Local.FileName);
                break;

              case TSynchronizeChecklist::saDeleteLocal:
                DeleteLocalList->Add(
                  IncludeTrailingBackslash(ChecklistItem->Local.Directory) +
                    ChecklistItem->Local.FileName);
                break;

              default:
                assert(false);
                break;
            }
          }
        }
        IIndex++;
      }

      // prevent showing/updating of progress dialog if there's nothing to do
      if (Count > 0)
      {
        Data.LocalDirectory = IncludeTrailingBackslash(CurrentLocalDirectory);
        Data.RemoteDirectory = UnixIncludeTrailingBackslash(CurrentRemoteDirectory);
        DoSynchronizeProgress(Data, false);

        if (FLAGSET(Params, spTimestamp))
        {
          if (DownloadList->GetCount() > 0)
          {
            ProcessFiles(DownloadList, foSetProperties,
              (TProcessFileEvent)&TTerminal::SynchronizeLocalTimestamp, NULL, osLocal);
          }

          if (UploadList->GetCount() > 0)
          {
            ProcessFiles(UploadList, foSetProperties,
              (TProcessFileEvent)&TTerminal::SynchronizeRemoteTimestamp);
          }
        }
        else
        {
          if ((DownloadList->GetCount() > 0) &&
              !CopyToLocal(DownloadList, Data.LocalDirectory, &SyncCopyParam, CopyParams))
          {
            Abort();
          }

          if ((DeleteRemoteList->GetCount() > 0) &&
              !DeleteFiles(DeleteRemoteList))
          {
            Abort();
          }

          if ((UploadList->GetCount() > 0) &&
              !CopyToRemote(UploadList, Data.RemoteDirectory, &SyncCopyParam, CopyParams))
          {
            Abort();
          }

          if ((DeleteLocalList->GetCount() > 0) &&
              !DeleteLocalFiles(DeleteLocalList))
          {
            Abort();
          }
        }
      }
    }
  }
}
//---------------------------------------------------------------------------
void TTerminal::DoSynchronizeProgress(const TSynchronizeData & Data,
  bool Collect)
{
  if (Data.OnSynchronizeDirectory != NULL)
  {
    bool Continue = true;
    // FIXME Data.OnSynchronizeDirectory(Data.LocalDirectory, Data.RemoteDirectory,
      // Continue, Collect);

    if (!Continue)
    {
      Abort();
    }
  }
}
//---------------------------------------------------------------------------
void TTerminal::SynchronizeLocalTimestamp(const std::wstring /*FileName*/,
  const TRemoteFile * File, void * /*Param*/)
{
  const TSynchronizeChecklist::TItem * ChecklistItem =
    reinterpret_cast<const TSynchronizeChecklist::TItem *>(File);

  std::wstring LocalFile =
    IncludeTrailingBackslash(ChecklistItem->Local.Directory) +
      ChecklistItem->Local.FileName;
  TFileOperationProgressType *OperationProgress = GetOperationProgress();
  FILE_OPERATION_LOOP (FMTLOAD(CANT_SET_ATTRS, LocalFile.c_str()),
    HANDLE Handle;
    OpenLocalFile(LocalFile, GENERIC_WRITE, NULL, &Handle,
      NULL, NULL, NULL, NULL);
    FILETIME WrTime = DateTimeToFileTime(ChecklistItem->Remote.Modification,
      GetSessionData()->GetDSTMode());
    bool Result = SetFileTime(Handle, NULL, NULL, &WrTime);
    CloseHandle(Handle);
    if (!Result)
    {
      Abort();
    }
  );
}
//---------------------------------------------------------------------------
void TTerminal::SynchronizeRemoteTimestamp(const std::wstring /*FileName*/,
  const TRemoteFile * File, void * /*Param*/)
{
  const TSynchronizeChecklist::TItem * ChecklistItem =
    reinterpret_cast<const TSynchronizeChecklist::TItem *>(File);

  TRemoteProperties Properties;
  Properties.Valid << vpModification;
  Properties.Modification = ConvertTimestampToUnix(ChecklistItem->FLocalLastWriteTime,
    GetSessionData()->GetDSTMode());

  ChangeFileProperties(
    UnixIncludeTrailingBackslash(ChecklistItem->Remote.Directory) + ChecklistItem->Remote.FileName,
    NULL, &Properties);
}
//---------------------------------------------------------------------------
void TTerminal::FileFind(std::wstring FileName,
  const TRemoteFile * File, /*TFilesFindParams*/ void * Param)
{
  // see DoFilesFind
  FOnFindingFile = NULL;

  assert(Param);
  assert(File);
  TFilesFindParams * AParams = static_cast<TFilesFindParams*>(Param);

  if (!AParams->Cancel)
  {
    if (FileName.empty())
    {
      FileName = File->GetFileName();
    }

    TFileMasks::TParams MaskParams;
    MaskParams.Size = File->GetSize();

    std::wstring FullFileName = UnixExcludeTrailingBackslash(File->GetFullFileName());
    if (AParams->FileMask.Matches(FullFileName, false,
         File->GetIsDirectory(), &MaskParams))
    {
      AParams->OnFileFound(this, FileName, File, AParams->Cancel);
    }

    if (File->GetIsDirectory())
    {
      DoFilesFind(FullFileName, *AParams);
    }
  }
}
//---------------------------------------------------------------------------
void TTerminal::DoFilesFind(std::wstring Directory, TFilesFindParams & Params)
{
  Params.OnFindingFile(this, Directory, Params.Cancel);
  if (!Params.Cancel)
  {
    assert(FOnFindingFile == NULL);
    // ideally we should set the handler only around actually reading
    // of the directory listing, so we at least reset the handler in
    // FileFind
    FOnFindingFile = Params.OnFindingFile;
    {
      BOOST_SCOPE_EXIT ( (&Self) )
      {
        Self->FOnFindingFile = NULL;
      } BOOST_SCOPE_EXIT_END
      ProcessDirectory(Directory, (TProcessFileEvent)&TTerminal::FileFind, &Params, false, true);
    }
  }
}
//---------------------------------------------------------------------------
void TTerminal::FilesFind(std::wstring Directory, const TFileMasks & FileMask,
  TFileFoundEvent OnFileFound, TFindingFileEvent OnFindingFile)
{
  TFilesFindParams Params;
  Params.FileMask = FileMask;
  Params.OnFileFound = OnFileFound;
  Params.OnFindingFile = OnFindingFile;
  Params.Cancel = false;
  DoFilesFind(Directory, Params);
}
//---------------------------------------------------------------------------
void TTerminal::SpaceAvailable(const std::wstring Path,
  TSpaceAvailable & ASpaceAvailable)
{
  assert(GetIsCapable(fcCheckingSpaceAvailable));

  try
  {
    FFileSystem->SpaceAvailable(Path, ASpaceAvailable);
  }
  catch (const std::exception &E)
  {
    CommandError(&E, FMTLOAD(SPACE_AVAILABLE_ERROR, Path.c_str()));
  }
}
//---------------------------------------------------------------------------
const TSessionInfo & TTerminal::GetSessionInfo()
{
  return FFileSystem->GetSessionInfo();
}
//---------------------------------------------------------------------------
const TFileSystemInfo & TTerminal::GetFileSystemInfo(bool Retrieve)
{
  return FFileSystem->GetFileSystemInfo(Retrieve);
}
//---------------------------------------------------------------------
std::wstring TTerminal::GetPassword()
{
  std::wstring Result;
  // FPassword is empty also when stored password was used
  if (FPassword.empty())
  {
    Result = GetSessionData()->GetPassword();
  }
  else
  {
    Result = DecryptPassword(FPassword);
  }
  return Result;
}
//---------------------------------------------------------------------
std::wstring TTerminal::GetTunnelPassword()
{
  std::wstring Result;
  // FTunnelPassword is empty also when stored password was used
  if (FTunnelPassword.empty())
  {
    Result = GetSessionData()->GetTunnelPassword();
  }
  else
  {
    Result = DecryptPassword(FTunnelPassword);
  }
  return Result;
}
//---------------------------------------------------------------------
bool TTerminal::GetStoredCredentialsTried()
{
  bool Result;
  if (FFileSystem != NULL)
  {
    Result = FFileSystem->GetStoredCredentialsTried();
  }
  else if (FSecureShell != NULL)
  {
    Result = FSecureShell->GetStoredCredentialsTried();
  }
  else
  {
    assert(FTunnelOpening);
    Result = false;
  }
  return Result;
}
//---------------------------------------------------------------------------
bool TTerminal::CopyToRemote(TStrings * FilesToCopy,
  const std::wstring TargetDir, const TCopyParamType * CopyParam, int Params)
{
  assert(FFileSystem);
  assert(FilesToCopy);

  assert(GetIsCapable(fcNewerOnlyUpload) || FLAGCLEAR(Params, cpNewerOnly));

  bool Result = false;
  TOnceDoneOperation OnceDoneOperation = odoIdle;

  try
  {

    __int64 Size;
    if (CopyParam->GetCalculateSize())
    {
      // dirty trick: when moving, do not pass copy param to avoid exclude mask
      CalculateLocalFilesSize(FilesToCopy, Size,
        (FLAGCLEAR(Params, cpDelete) ? CopyParam : NULL));
    }

    TFileOperationProgressType *OperationProgress = new TFileOperationProgressType(boost::bind(&TTerminal::DoProgress, this, _1, _2),
        boost::bind(&TTerminal::DoFinished, this, _1, _2, _3, _4, _5, _6));
    OperationProgress->Start((Params & cpDelete ? foMove : foCopy), osLocal,
      FilesToCopy->GetCount(), Params & cpTemporary, TargetDir, CopyParam->GetCPSLimit());

    FOperationProgress = OperationProgress;
    {
      BOOST_SCOPE_EXIT ( (&Self) (&OperationProgress) )
      {
        OperationProgress->Stop();
        Self->FOperationProgress = NULL;
      } BOOST_SCOPE_EXIT_END
      if (CopyParam->GetCalculateSize())
      {
        OperationProgress->SetTotalSize(Size);
      }

      std::wstring UnlockedTargetDir = TranslateLockedPath(TargetDir, false);
      BeginTransaction();
      {
          BOOST_SCOPE_EXIT ( (&Self) )
          {
            if (Self->GetActive())
            {
              Self->ReactOnCommand(fsCopyToRemote);
            }
            Self->EndTransaction();
          } BOOST_SCOPE_EXIT_END
        if (GetLog()->GetLogging())
        {
          LogEvent(FORMAT(L"Copying %d files/directories to remote directory "
            L"\"%s\"", FilesToCopy->GetCount(), TargetDir.c_str()));
          LogEvent(CopyParam->GetLogStr());
        }

        FFileSystem->CopyToRemote(FilesToCopy, UnlockedTargetDir,
          CopyParam, Params, OperationProgress, OnceDoneOperation);
      }

      if (OperationProgress->Cancel == csContinue)
      {
        Result = true;
      }
    }
  }
  catch (const std::exception &E)
  {
    CommandError(&E, LoadStr(TOREMOTE_COPY_ERROR));
    OnceDoneOperation = odoIdle;
  }

  if (OnceDoneOperation != odoIdle)
  {
    CloseOnCompletion(OnceDoneOperation);
  }

  return Result;
}
//---------------------------------------------------------------------------
bool TTerminal::CopyToLocal(TStrings *FilesToCopy,
  const std::wstring TargetDir, const TCopyParamType *CopyParam, int Params)
{
  assert(FFileSystem);

  // see scp.c: sink(), tolocal()

  bool Result = false;
  bool OwnsFileList = (FilesToCopy == NULL);
  TOnceDoneOperation OnceDoneOperation = odoIdle;

  BOOST_SCOPE_EXIT( (OwnsFileList) (&FilesToCopy) )
  {
    if (OwnsFileList) delete FilesToCopy;
  } BOOST_SCOPE_EXIT_END
  if (OwnsFileList)
  {
    FilesToCopy = new TStringList();
    FilesToCopy->Assign(GetFiles()->GetSelectedFiles());
  }

  BeginTransaction();
  {
    BOOST_SCOPE_EXIT( (&Self) )
    {
        // If session is still active (no fatal error) we reload directory
        // by calling EndTransaction
        Self->EndTransaction();
    } BOOST_SCOPE_EXIT_END
    __int64 TotalSize;
    bool TotalSizeKnown = false;
    TFileOperationProgressType *OperationProgress = new TFileOperationProgressType(boost::bind(&TTerminal::DoProgress, this, _1, _2),
      boost::bind(&TTerminal::DoFinished, this, _1, _2, _3, _4, _5, _6));

    if (CopyParam->GetCalculateSize())
    {
      SetExceptionOnFail(true);
      {
        BOOST_SCOPE_EXIT ( (&Self) )
        {
          Self->SetExceptionOnFail(false);
        } BOOST_SCOPE_EXIT_END
        // dirty trick: when moving, do not pass copy param to avoid exclude mask
        CalculateFilesSize(FilesToCopy, TotalSize, csIgnoreErrors,
          (FLAGCLEAR(Params, cpDelete) ? CopyParam : NULL));
        TotalSizeKnown = true;
      }
    }
    OperationProgress->Start((Params & cpDelete ? foMove : foCopy), osRemote,
      FilesToCopy->GetCount(), Params & cpTemporary, TargetDir, CopyParam->GetCPSLimit());

    FOperationProgress = OperationProgress;
    {
      BOOST_SCOPE_EXIT ( (&FOperationProgress) (&OperationProgress) )
      {
        FOperationProgress = NULL;
        OperationProgress->Stop();
      } BOOST_SCOPE_EXIT_END
      if (TotalSizeKnown)
      {
        OperationProgress->SetTotalSize(TotalSize);
      }

      try
      {
        BOOST_SCOPE_EXIT ( (&Self) )
        {
            if (Self->GetActive())
            {
              Self->ReactOnCommand(fsCopyToLocal);
            }
        } BOOST_SCOPE_EXIT_END
        FFileSystem->CopyToLocal(FilesToCopy, TargetDir, CopyParam, Params,
          OperationProgress, OnceDoneOperation);
      }
      catch (const std::exception &E)
      {
        CommandError(&E, LoadStr(TOLOCAL_COPY_ERROR));
        OnceDoneOperation = odoIdle;
      }

      if (OperationProgress->Cancel == csContinue)
      {
        Result = true;
      }
    }
  }

  if (OnceDoneOperation != odoIdle)
  {
    CloseOnCompletion(OnceDoneOperation);
  }

  return Result;
}
//---------------------------------------------------------------------------
TSecondaryTerminal::TSecondaryTerminal(TTerminal * MainTerminal,
  TSessionData * ASessionData, TConfiguration * Configuration, const std::wstring & Name) :
  TTerminal(ASessionData, Configuration),
  FMainTerminal(MainTerminal), FMasterPasswordTried(false),
  FMasterTunnelPasswordTried(false)
{
  GetLog()->SetParent(FMainTerminal->GetLog());
  GetLog()->SetName(Name);
  GetSessionData()->NonPersistant();
  assert(FMainTerminal != NULL);
  if (!FMainTerminal->GetUserName().empty())
  {
    GetSessionData()->SetUserName(FMainTerminal->GetUserName());
  }
}
//---------------------------------------------------------------------------
void TSecondaryTerminal::DirectoryLoaded(TRemoteFileList * FileList)
{
  FMainTerminal->DirectoryLoaded(FileList);
  assert(FileList != NULL);
}
//---------------------------------------------------------------------------
void TSecondaryTerminal::DirectoryModified(const std::wstring Path,
  bool SubDirs)
{
  // clear cache of main terminal
  FMainTerminal->DirectoryModified(Path, SubDirs);
}
//---------------------------------------------------------------------------
bool TSecondaryTerminal::DoPromptUser(TSessionData * Data,
  TPromptKind Kind, std::wstring Name, std::wstring Instructions, TStrings * Prompts,
  TStrings * Results)
{
  bool AResult = false;

  if ((Prompts->GetCount() == 1) && !bool(Prompts->GetObject(0)) &&
      ((Kind == pkPassword) || (Kind == pkPassphrase) || (Kind == pkKeybInteractive) ||
       (Kind == pkTIS) || (Kind == pkCryptoCard)))
  {
    bool & PasswordTried =
      FTunnelOpening ? FMasterTunnelPasswordTried : FMasterPasswordTried;
    if (!PasswordTried)
    {
      // let's expect that the main session is already authenticated and its password
      // is not written after, so no locking is necessary
      // (no longer true, once the main session can be reconnected)
      std::wstring Password;
      if (FTunnelOpening)
      {
        Password = FMainTerminal->GetTunnelPassword();
      }
      else
      {
        Password = FMainTerminal->GetPassword();
      }
      Results->GetString(0) = Password;
      if (!Results->GetString(0).empty())
      {
        LogEvent(L"Using remembered password of the main session.");
        AResult = true;
      }
      PasswordTried = true;
    }
  }

  if (!AResult)
  {
    AResult = TTerminal::DoPromptUser(Data, Kind, Name, Instructions, Prompts, Results);
  }

  return AResult;
}
//---------------------------------------------------------------------------
TTerminalList::TTerminalList(TConfiguration * AConfiguration) :
  TObjectList()
{
  assert(AConfiguration);
  FConfiguration = AConfiguration;
}
//---------------------------------------------------------------------------
TTerminalList::~TTerminalList()
{
  assert(GetCount() == 0);
}
//---------------------------------------------------------------------------
TTerminal * TTerminalList::CreateTerminal(TSessionData * Data)
{
  return new TTerminal(Data, FConfiguration);
}
//---------------------------------------------------------------------------
TTerminal * TTerminalList::NewTerminal(TSessionData * Data)
{
  TTerminal * Terminal = CreateTerminal(Data);
  Add(Terminal);
  return Terminal;
}
//---------------------------------------------------------------------------
void TTerminalList::FreeTerminal(TTerminal * Terminal)
{
  assert(IndexOf(Terminal) >= 0);
  Remove(Terminal);
}
//---------------------------------------------------------------------------
void TTerminalList::FreeAndNullTerminal(TTerminal * & Terminal)
{
  TTerminal * T = Terminal;
  Terminal = NULL;
  FreeTerminal(T);
}
//---------------------------------------------------------------------------
TTerminal * TTerminalList::GetTerminal(int Index)
{
  return reinterpret_cast<TTerminal *>(GetItem(Index));
}
//---------------------------------------------------------------------------
int TTerminalList::GetActiveCount()
{
  int Result = 0;
  TTerminal * Terminal;
  for (int i = 0; i < GetCount(); i++)
  {
    Terminal = GetTerminal(i);
    if (Terminal->GetActive())
    {
      Result++;
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
void TTerminalList::Idle()
{
  TTerminal * Terminal;
  for (int i = 0; i < GetCount(); i++)
  {
    Terminal = GetTerminal(i);
    if (Terminal->GetStatus() == ssOpened)
    {
      Terminal->Idle();
    }
  }
}
//---------------------------------------------------------------------------
void TTerminalList::RecryptPasswords()
{
  for (int Index = 0; Index < GetCount(); Index++)
  {
    GetTerminal(Index)->RecryptPasswords();
  }
}
