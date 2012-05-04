//---------------------------------------------------------------------------
#ifndef _MSC_VER
#include <vcl.h>
#pragma hdrstop
#endif

#include "Terminal.h"

#ifndef _MSC_VER
#include <SysUtils.hpp>
#include <FileCtrl.hpp>
#else
#include "stdafx.h"
#include "boostdefines.hpp"
#include <boost/scope_exit.hpp>
#include <boost/bind.hpp>

#include "WebDAVFileSystem.h"
#include "Common.h"
#endif

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

#ifndef AUTO_WINSOCK
#include <winsock2.h>
#endif
//---------------------------------------------------------------------------
#ifndef _MSC_VER
#pragma package(smart_init)
#endif
//---------------------------------------------------------------------------
#define COMMAND_ERROR_ARI(MESSAGE, REPEAT) \
  { \
    unsigned int Result = CommandError(&E, MESSAGE, qaRetry | qaSkip | qaAbort); \
    switch (Result) { \
      case qaRetry: { REPEAT; } break; \
      case qaAbort: Abort(); \
    } \
  }
//---------------------------------------------------------------------------
// Note that the action may already be canceled when RollbackAction is called
#define COMMAND_ERROR_ARI_ACTION(MESSAGE, REPEAT, ACTION) \
  { \
    unsigned int Result; \
    try \
    { \
      Result = CommandError(&E, MESSAGE, qaRetry | qaSkip | qaAbort); \
    } \
    catch(Exception & E2) \
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
  UnicodeString Target;
  UnicodeString FileMask;
};
//---------------------------------------------------------------------------
struct TFilesFindParams
{
  TFilesFindParams() :
    OnFileFound(NULL),
    OnFindingFile(NULL),
    Cancel(false)
  {
  }
  TFileMasks FileMask;
  const TFileFoundEvent * OnFileFound;
  const TFindingFileEvent * OnFindingFile;
  bool Cancel;
};
//---------------------------------------------------------------------------
TCalculateSizeStats::TCalculateSizeStats() :
  Files(0),
  Directories(0),
  SymLinks(0)
{
}
//---------------------------------------------------------------------------
TSynchronizeOptions::TSynchronizeOptions() :
  Filter(0)
{
}
//---------------------------------------------------------------------------
TSynchronizeOptions::~TSynchronizeOptions()
{
  delete Filter;
}
//---------------------------------------------------------------------------
bool __fastcall TSynchronizeOptions::MatchesFilter(const UnicodeString & FileName)
{
  int FoundIndex;
  bool Result;
  if (Filter == NULL)
  {
    Result = true;
  }
  else
  {
    Result = Filter->Find(FileName, FoundIndex);
  }
  return Result;
}
//---------------------------------------------------------------------------
TSpaceAvailable::TSpaceAvailable() :
  BytesOnDevice(0),
  UnusedBytesOnDevice(0),
  BytesAvailableToUser(0),
  UnusedBytesAvailableToUser(0),
  BytesPerAllocationUnit(0)
{
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
const UnicodeString& TSynchronizeChecklist::TItem::GetFileName() const
{
  if (!Remote.FileName.IsEmpty())
  {
    return Remote.FileName;
  }
  else
  {
    assert(!Local.FileName.IsEmpty());
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
    delete static_cast<TItem *>(static_cast<void *>(FList->GetItem(Index)));
  }
  delete FList;
}
//---------------------------------------------------------------------------
void TSynchronizeChecklist::Add(TItem * Item)
{
  FList->Add(reinterpret_cast<TObject *>(static_cast<void *>(Item)));
}
//---------------------------------------------------------------------------
int /* __fastcall */ TSynchronizeChecklist::Compare(void * AItem1, void * AItem2)
{
  TItem * Item1 = static_cast<TItem *>(AItem1);
  TItem * Item2 = static_cast<TItem *>(AItem2);

  int Result;
  if (!Item1->Local.Directory.IsEmpty())
  {
    Result = AnsiCompareText(Item1->Local.Directory, Item2->Local.Directory);
  }
  else
  {
    assert(!Item1->Remote.Directory.IsEmpty());
    Result = AnsiCompareText(Item1->Remote.Directory, Item2->Remote.Directory);
  }

  if (Result == 0)
  {
    Result = AnsiCompareText(Item1->GetFileName(), Item2->GetFileName());
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
  return static_cast<TItem *>(FList->GetItem(Index));
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
class TTunnelThread : public TSimpleThread
{
public:
  explicit /* __fastcall */ TTunnelThread(TSecureShell * SecureShell);
  virtual /* __fastcall */ ~TTunnelThread();

  virtual void __fastcall Init();
  virtual void __fastcall Terminate();

protected:
  virtual void __fastcall Execute();

private:
  TSecureShell * FSecureShell;
  bool FTerminated;
  TTunnelThread * Self;
};
//---------------------------------------------------------------------------
/* __fastcall */ TTunnelThread::TTunnelThread(TSecureShell * SecureShell) :
  TSimpleThread(),
  FSecureShell(SecureShell),
  FTerminated(false),
  Self(NULL)
{
}
//---------------------------------------------------------------------------
void __fastcall TTunnelThread::Init()
{
  Self = this;
  TSimpleThread::Init();
  Start();
}
//---------------------------------------------------------------------------
/* __fastcall */ TTunnelThread::~TTunnelThread()
{
  // close before the class's virtual functions (Terminate particularly) are lost
  Close();
}
//---------------------------------------------------------------------------
void __fastcall TTunnelThread::Terminate()
{
  FTerminated = true;
}
//---------------------------------------------------------------------------
void __fastcall TTunnelThread::Execute()
{
  try
  {
    while (!FTerminated)
    {
      FSecureShell->Idle(250);
    }
  }
  catch(...)
  {
    if (FSecureShell->GetActive())
    {
      FSecureShell->Close();
    }
    // do not pass exception out of thread's proc
  }
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
class TTunnelUI : public TSessionUI
{
public:
  explicit /* __fastcall */ TTunnelUI(TTerminal * Terminal);
  virtual ~TTunnelUI() {}
  virtual void __fastcall Information(const UnicodeString & Str, bool Status);
  virtual unsigned int __fastcall QueryUser(const UnicodeString Query,
    TStrings * MoreMessages, unsigned int Answers, const TQueryParams * Params,
    TQueryType QueryType);
  virtual unsigned int __fastcall QueryUserException(const UnicodeString Query,
    Exception * E, unsigned int Answers, const TQueryParams * Params,
    TQueryType QueryType);
  virtual bool __fastcall PromptUser(TSessionData * Data, TPromptKind Kind,
    UnicodeString Name, UnicodeString Instructions, TStrings * Prompts,
    TStrings * Results);
  virtual void __fastcall DisplayBanner(const UnicodeString & Banner);
  virtual void __fastcall FatalError(Exception * E, UnicodeString Msg);
  virtual void __fastcall HandleExtendedException(Exception * E);
  virtual void __fastcall Closed();

private:
  TTerminal * FTerminal;
  unsigned int FTerminalThread;
};
//---------------------------------------------------------------------------
/* __fastcall */ TTunnelUI::TTunnelUI(TTerminal * Terminal)
{
  FTerminal = Terminal;
  FTerminalThread = GetCurrentThreadId();
}
//---------------------------------------------------------------------------
void __fastcall TTunnelUI::Information(const UnicodeString & Str, bool Status)
{
  if (GetCurrentThreadId() == FTerminalThread)
  {
    FTerminal->Information(Str, Status);
  }
}
//---------------------------------------------------------------------------
unsigned int __fastcall TTunnelUI::QueryUser(const UnicodeString Query,
  TStrings * MoreMessages, unsigned int Answers, const TQueryParams * Params,
  TQueryType QueryType)
{
  unsigned int Result;
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
unsigned int __fastcall TTunnelUI::QueryUserException(const UnicodeString Query,
  Exception * E, unsigned int Answers, const TQueryParams * Params,
  TQueryType QueryType)
{
  unsigned int Result;
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
bool __fastcall TTunnelUI::PromptUser(TSessionData * Data, TPromptKind Kind,
  UnicodeString Name, UnicodeString Instructions, TStrings * Prompts, TStrings * Results)
{
  bool Result;
  if (GetCurrentThreadId() == FTerminalThread)
  {
    if (IsAuthenticationPrompt(Kind))
    {
      Instructions = LoadStr(TUNNEL_INSTRUCTION) +
        (Instructions.IsEmpty() ? L"" : L"\n") +
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
void __fastcall TTunnelUI::DisplayBanner(const UnicodeString & Banner)
{
  if (GetCurrentThreadId() == FTerminalThread)
  {
    FTerminal->DisplayBanner(Banner);
  }
}
//---------------------------------------------------------------------------
void __fastcall TTunnelUI::FatalError(Exception * E, UnicodeString Msg)
{
  throw ESshFatal(Msg, E);
}
//---------------------------------------------------------------------------
void __fastcall TTunnelUI::HandleExtendedException(Exception * E)
{
  if (GetCurrentThreadId() == FTerminalThread)
  {
    FTerminal->HandleExtendedException(E);
  }
}
//---------------------------------------------------------------------------
void __fastcall TTunnelUI::Closed()
{
  // noop
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
class TCallbackGuard
{
public:
  inline /* __fastcall */ TCallbackGuard(TTerminal * FTerminal);
  inline /* __fastcall */ ~TCallbackGuard();

  void __fastcall FatalError(Exception * E, const UnicodeString & Msg);
  inline void __fastcall Verify();
  void __fastcall Dismiss();

private:
  ExtException * FFatalError;
  TTerminal * FTerminal;
  bool FGuarding;
};
//---------------------------------------------------------------------------
/* __fastcall */ TCallbackGuard::TCallbackGuard(TTerminal * Terminal) :
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
/* __fastcall */ TCallbackGuard::~TCallbackGuard()
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
  /* __fastcall */ ECallbackGuardAbort() : EAbort("")
  {
  }
};
//---------------------------------------------------------------------------
void __fastcall TCallbackGuard::FatalError(Exception * E, const UnicodeString & Msg)
{
  assert(FGuarding);

  // make sure we do not bother about getting back the silent abort exception
  // we issued outselves. this may happen when there is an exception handler
  // that converts any exception to fatal one (such as in TTerminal::Open).
  if (dynamic_cast<ECallbackGuardAbort *>(E) == NULL)
  {
    assert(FFatalError == NULL);

    FFatalError = new ExtException(Msg, E);
  }

  // silently abort what we are doing.
  // non-silent exception would be catched probably by default application
  // exception handler, which may not do an appropriate action
  // (particularly it will not resume broken transfer).
  throw ECallbackGuardAbort();
}
//---------------------------------------------------------------------------
void __fastcall TCallbackGuard::Dismiss()
{
  assert(FFatalError == NULL);
  FGuarding = false;
}
//---------------------------------------------------------------------------
void __fastcall TCallbackGuard::Verify()
{
  if (FGuarding)
  {
    FGuarding = false;
    assert(FTerminal->FCallbackGuard == this);
    FTerminal->FCallbackGuard = NULL;

    if (FFatalError != NULL)
    {
      throw ESshFatal(L"", FFatalError);
    }
  }
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
/* __fastcall */ TTerminal::TTerminal() :
  TObject(),
  TSessionUI()
{
}

void __fastcall TTerminal::Init(TSessionData * SessionData, TConfiguration * Configuration)
{
  FConfiguration = Configuration;
  FSessionData = new TSessionData(L"");
  FSessionData->Assign(SessionData);
  FLog = new TSessionLog(this, FSessionData, Configuration);
  FActionLog = new TActionLog(this, FSessionData, Configuration);
  FFiles = new TRemoteDirectory(this);
  FExceptionOnFail = 0;
  FInTransaction = 0;
  FSuspendTransaction = false;
  FReadCurrentDirectoryPending = false;
  FReadDirectoryPending = false;
  FUsersGroupsLookedup = false;
  FTunnelLocalPortNumber = 0;
  FFileSystem = NULL;
  FSecureShell = NULL;
#ifndef _MSC_VER
  FOnProgress = NULL;
  FOnFinished = NULL;
  FOnDeleteLocalFile = NULL;
  FOnReadDirectoryProgress = NULL;
  FOnQueryUser = NULL;
  FOnPromptUser = NULL;
  FOnDisplayBanner = NULL;
  FOnShowExtendedException = NULL;
  FOnInformation = NULL;
  FOnClose = NULL;
  FOnFindingFile = NULL;
#endif
  FUseBusyCursor = True;
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
  FOperationProgress = NULL;
  FClosedOnCompletion = NULL;
  FTunnel = NULL;
  FAnyInformation = false;;
  Self = this;
}
//---------------------------------------------------------------------------
/* __fastcall */ TTerminal::~TTerminal()
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
  SAFE_DESTROY_EX(TActionLog, FActionLog);
  delete FFiles;
  delete FDirectoryCache;
  delete FDirectoryChangesCache;
  SAFE_DESTROY(FSessionData);
}
//---------------------------------------------------------------------------
void __fastcall TTerminal::Idle()
{
  // once we disconnect, do nothing, until reconnect handler
  // "receives the information"
  if (GetActive())
  {
    if (Configuration->GetActualLogProtocol() >= 1)
    {
      // LogEvent(L"Session upkeep");
    }

    assert(FFileSystem != NULL);
    FFileSystem->Idle();

    if (GetCommandSessionOpened())
    {
      try
      {
        FCommandSession->Idle();
      }
      catch(Exception & E)
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
RawByteString __fastcall TTerminal::EncryptPassword(const UnicodeString & Password)
{
  return Configuration->EncryptPassword(Password, GetSessionData()->GetSessionName());
}
//---------------------------------------------------------------------
UnicodeString __fastcall TTerminal::DecryptPassword(const RawByteString & Password)
{
  UnicodeString Result;
  try
  {
    Result = Configuration->DecryptPassword(Password, GetSessionData()->GetSessionName());
  }
  catch(EAbort &)
  {
    // silently ignore aborted prompts for master password and return empty password
  }
  return Result;
}
//---------------------------------------------------------------------------
void __fastcall TTerminal::RecryptPasswords()
{
  FSessionData->RecryptPasswords();
  FPassword = EncryptPassword(DecryptPassword(FPassword));
  FTunnelPassword = EncryptPassword(DecryptPassword(FTunnelPassword));
}
//---------------------------------------------------------------------------
bool /* __fastcall */ TTerminal::IsAbsolutePath(const UnicodeString Path)
{
  return !Path.IsEmpty() && Path[1] == L'/';
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TTerminal::ExpandFileName(UnicodeString Path,
  const UnicodeString BasePath)
{
  Path = UnixExcludeTrailingBackslash(Path);
  if (!IsAbsolutePath(Path) && !BasePath.IsEmpty())
  {
    // TODO: Handle more complicated cases like "../../xxx"
    if (Path == PARENTDIRECTORY)
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
bool __fastcall TTerminal::GetActive()
{
  return (FFileSystem != NULL) && FFileSystem->GetActive();
}
//---------------------------------------------------------------------------
void __fastcall TTerminal::Close()
{
  FFileSystem->Close();

  if (GetCommandSessionOpened())
  {
    FCommandSession->Close();
  }
}
//---------------------------------------------------------------------------
void __fastcall TTerminal::ResetConnection()
{
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
void __fastcall TTerminal::Open()
{
  FLog->ReflectSettings();
  FActionLog->ReflectSettings();
  bool Reopen = false;
  // try
  do
  {
    Reopen = false;
    DoInformation(L"", true, 1);
    try
    {
      // try
      {
        BOOST_SCOPE_EXIT ( (&Self) )
        {
          Self->DoInformation(L"", true, 0);
        } BOOST_SCOPE_EXIT_END
        try
        {
          ResetConnection();
          FStatus = ssOpening;
          // try
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
                (static_cast<TFTPFileSystem *>(FFileSystem))->Init();
                FFileSystem->Open();
                GetLog()->AddSeparator();
                LogEvent(L"Using FTP protocol.");
#endif
              }
              else if (GetSessionData()->GetFSProtocol() == fsFTPS)
              {
#if defined(NO_FILEZILLA) && defined(MPEXT_NO_SSLDLL)
                LogEvent(L"FTPS protocol is not supported by this build.");
                FatalError(NULL, LoadStr(FTPS_UNSUPPORTED));
#else
                FFSProtocol = cfsFTPS;
                FFileSystem = new TFTPFileSystem(this);
                (static_cast<TFTPFileSystem *>(FFileSystem))->Init();
                FFileSystem->Open();
                GetLog()->AddSeparator();
                LogEvent(L"Using FTPS protocol.");
#endif
              }
              else if (GetSessionData()->GetFSProtocol() == fsHTTP)
              {
                FFSProtocol = cfsHTTP;
                FFileSystem = new TWebDAVFileSystem(this);
                static_cast<TWebDAVFileSystem *>(FFileSystem)->Init();
                FFileSystem->Open();
                GetLog()->AddSeparator();
                LogEvent(L"Using HTTP protocol.");
              }
              else if (GetSessionData()->GetFSProtocol() == fsHTTPS)
              {
                FFSProtocol = cfsHTTPS;
                FFileSystem = new TWebDAVFileSystem(this);
                static_cast<TWebDAVFileSystem *>(FFileSystem)->Init();
                FFileSystem->Open();
                GetLog()->AddSeparator();
                LogEvent(L"Using HTTPS protocol.");
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
                  catch (Exception & E)
                  {
                    assert(!FSecureShell->GetActive());
                    if (!FSecureShell->GetActive() && !FTunnelError.IsEmpty())
                    {
                      // the only case where we expect this to happen
                      assert(E.GetMessage() == LoadStr(UNEXPECTED_CLOSE_ERROR));
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
                    FFileSystem = new TSCPFileSystem(this);
                    (static_cast<TSCPFileSystem *>(FFileSystem))->Init(FSecureShell);
                    FSecureShell = NULL; // ownership passed
                    LogEvent(L"Using SCP protocol.");
                  }
                  else
                  {
                    FFSProtocol = cfsSFTP;
                    FFileSystem = new TSFTPFileSystem(this);
                    (static_cast<TSFTPFileSystem *>(FFileSystem))->Init(FSecureShell);
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
          if (FDirectoryChangesCache != NULL)
          {
            delete FDirectoryChangesCache;
            FDirectoryChangesCache = NULL;
          }
          throw;
        }
      }
#ifndef _MSC_VER
      __finally
      {
        DoInformation(L"", true, 0);
      }
#endif
    }
    catch (EFatal & E)
    {
      Reopen = DoQueryReopen(&E);
      if (Reopen)
      {
        delete FFileSystem;
        FFileSystem = NULL;
        delete FSecureShell;
        FSecureShell = NULL;
        delete FTunnelData;
        FTunnelData = NULL;
        FStatus = ssClosed;
        delete FTunnel;
        FTunnel = NULL;
      }
      else
      {
        throw;
      }
    }
    // catch(EFatal &)
    // {
    //   throw;
    // }
    catch(Exception & E)
    {
      // any Exception while opening session is fatal
      FatalError(&E, L"");
    }
  }
  while (Reopen);
  FSessionData->SetNumberOfRetries(0);
}
//---------------------------------------------------------------------------
bool __fastcall TTerminal::IsListenerFree(unsigned int PortNumber)
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
void __fastcall TTerminal::OpenTunnel()
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
    FTunnelData->SetName(FMTLOAD(TUNNEL_SESSION_NAME, FSessionData->GetSessionName().c_str()));
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
    // try
    {
      BOOST_SCOPE_EXIT ( (&Self) )
      {
        Self->FTunnelOpening = false;
      } BOOST_SCOPE_EXIT_END
      FTunnel->Open();
    }
#ifndef _MSC_VER
    __finally
    {
      FTunnelOpening = false;
    }
#endif

    FTunnelThread = new TTunnelThread(FTunnel);
    FTunnelThread->Init();
  }
  catch(...)
  {
    CloseTunnel();
    throw;
  }
}
//---------------------------------------------------------------------------
void __fastcall TTerminal::CloseTunnel()
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
void /* __fastcall */ TTerminal::Closed()
{
  if (FTunnel != NULL)
  {
     CloseTunnel();
  }

  if (!GetOnClose().empty())
  {
    TCallbackGuard Guard(this);
    GetOnClose()(this);
    Guard.Verify();
  }

  FStatus = ssClosed;
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::Reopen(int Params)
{
  TFSProtocol OrigFSProtocol = GetSessionData()->GetFSProtocol();
  UnicodeString PrevRemoteDirectory = GetSessionData()->GetRemoteDirectory();
  bool PrevReadCurrentDirectoryPending = FReadCurrentDirectoryPending;
  bool PrevReadDirectoryPending = FReadDirectoryPending;
  assert(!FSuspendTransaction);
  bool PrevAutoReadDirectory = FAutoReadDirectory;
  // here used to be a check for FExceptionOnFail being 0
  // but it can happen, e.g. when we are downloading file to execute it.
  // however I'm not sure why we mind having excaption-on-fail enabled here
  int PrevExceptionOnFail = FExceptionOnFail;
  // try
  {
    BOOST_SCOPE_EXIT ( (&Self) (&PrevRemoteDirectory)
                       (&OrigFSProtocol) (&PrevAutoReadDirectory) (&PrevReadCurrentDirectoryPending)
                       (&PrevReadDirectoryPending) (&PrevExceptionOnFail) )
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
    UnicodeString ACurrentDirectory = PeekCurrentDirectory();
    if (!ACurrentDirectory.IsEmpty())
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
#ifndef _MSC_VER
  __finally
  {
    SessionData->RemoteDirectory = PrevRemoteDirectory;
    SessionData->FSProtocol = OrigFSProtocol;
    FAutoReadDirectory = PrevAutoReadDirectory;
    FReadCurrentDirectoryPending = PrevReadCurrentDirectoryPending;
    FReadDirectoryPending = PrevReadDirectoryPending;
    FSuspendTransaction = false;
    FExceptionOnFail = PrevExceptionOnFail;
  }
#endif
}
//---------------------------------------------------------------------------
bool __fastcall TTerminal::PromptUser(TSessionData * Data, TPromptKind Kind,
  UnicodeString Name, UnicodeString Instructions, UnicodeString Prompt, bool Echo, int MaxLen, UnicodeString & Result)
{
  bool AResult;
  TStringList Prompts;
  TStringList Results;
  // try
  {
    Prompts.AddObject(Prompt, reinterpret_cast<TObject *>(static_cast<size_t>(Echo)));
    Results.AddObject(Result, reinterpret_cast<TObject *>(MaxLen));

    AResult = PromptUser(Data, Kind, Name, Instructions, &Prompts, &Results);

    Result = Results.GetStrings(0);
  }
#ifndef _MSC_VER
  __finally
  {
    delete Prompts;
    delete Results;
  }
#endif

  return AResult;
}
//---------------------------------------------------------------------------
bool __fastcall TTerminal::PromptUser(TSessionData * Data, TPromptKind Kind,
  UnicodeString Name, UnicodeString Instructions, TStrings * Prompts, TStrings * Results)
{
  // If PromptUser is overriden in descendant class, the overriden version
  // is not called when accessed via TSessionIU interface.
  // So this is workaround.
  return DoPromptUser(Data, Kind, Name, Instructions, Prompts, Results);
}
//---------------------------------------------------------------------------
bool /* __fastcall */ TTerminal::DoPromptUser(TSessionData * /*Data*/, TPromptKind Kind,
  UnicodeString Name, UnicodeString Instructions, TStrings * Prompts, TStrings * Results)
{
  bool AResult = false;

  if (!GetOnPromptUser().empty())
  {
    TCallbackGuard Guard(this);
    GetOnPromptUser()(this, Kind, Name, Instructions, Prompts, Results, AResult, NULL);
    Guard.Verify();
  }

  if (AResult && (Configuration->GetRememberPassword()) &&
      (Prompts->GetCount() == 1) && !(Prompts->GetObjects(0) != NULL) &&
      ((Kind == pkPassword) || (Kind == pkPassphrase) || (Kind == pkKeybInteractive) ||
       (Kind == pkTIS) || (Kind == pkCryptoCard)))
  {
    RawByteString EncryptedPassword = EncryptPassword(Results->GetStrings(0));
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
unsigned int /* __fastcall */ TTerminal::QueryUser(const UnicodeString Query,
  TStrings * MoreMessages, unsigned int Answers, const TQueryParams * Params,
  TQueryType QueryType)
{
  LogEvent(FORMAT(L"Asking user:\n%s (%s)", Query.c_str(), MoreMessages ? MoreMessages->GetCommaText().c_str() : L""));
  unsigned int Answer = AbortAnswer(Answers);
  if (!FOnQueryUser.empty())
  {
    TCallbackGuard Guard(this);
    FOnQueryUser(this, Query, MoreMessages, Answers, Params, Answer, QueryType, NULL);
    Guard.Verify();
  }
  return Answer;
}
//---------------------------------------------------------------------------
unsigned int __fastcall TTerminal::QueryUserException(const UnicodeString Query,
  Exception * E, unsigned int Answers, const TQueryParams * Params,
  TQueryType QueryType)
{
  unsigned int Result;
  TStringList MoreMessages;
  // try
  {
    if (E != NULL)
    {
      if (!E->GetMessage().IsEmpty() && !Query.IsEmpty())
      {
        MoreMessages.Add(E->GetMessage());
      }

      ExtException * EE = dynamic_cast<ExtException*>(E);
      if ((EE != NULL) && (EE->GetMoreMessages() != NULL))
      {
        MoreMessages.AddStrings(EE->GetMoreMessages());
      }
    }
    Result = QueryUser(!Query.IsEmpty() ? Query : UnicodeString(E->GetMessage()),
      MoreMessages.GetCount() ? &MoreMessages : NULL,
      Answers, Params, QueryType);
  }
#ifndef _MSC_VER
  __finally
  {
    delete MoreMessages;
  }
#endif
  return Result;
}
//---------------------------------------------------------------------------
void __fastcall TTerminal::DisplayBanner(const UnicodeString & Banner)
{
  if (!GetOnDisplayBanner().empty())
  {
    if (Configuration->GetForceBanners() ||
        Configuration->ShowBanner(GetSessionData()->GetSessionKey(), Banner))
    {
      bool NeverShowAgain = false;
      int Options =
        FLAGMASK(Configuration->GetForceBanners(), boDisableNeverShowAgain);
      TCallbackGuard Guard(this);
      GetOnDisplayBanner()(this, GetSessionData()->GetSessionName(), Banner,
        NeverShowAgain, Options);
      Guard.Verify();
      if (!Configuration->GetForceBanners() && NeverShowAgain)
      {
        Configuration->NeverShowBanner(GetSessionData()->GetSessionKey(), Banner);
      }
    }
  }
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::HandleExtendedException(Exception * E)
{
  GetLog()->AddException(E);
  if (!GetOnShowExtendedException().empty())
  {
    TCallbackGuard Guard(this);
    // the event handler may destroy 'this' ...
    GetOnShowExtendedException()(this, E, NULL);
    // .. hence guard is dismissed from destructor, to make following call no-op
    Guard.Verify();
  }
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::ShowExtendedException(Exception * E)
{
  GetLog()->AddException(E);
  if (!GetOnShowExtendedException().empty())
  {
    GetOnShowExtendedException()(this, E, NULL);
  }
}
//---------------------------------------------------------------------------
/* __fastcall */ void TTerminal::DoInformation(const UnicodeString & Str, bool Status,
  int Phase)
{
  if (!GetOnInformation().empty())
  {
    TCallbackGuard Guard(this);
    GetOnInformation()(this, Str, Status, Phase);
    Guard.Verify();
  }
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::Information(const UnicodeString & Str, bool Status)
{
  DoInformation(Str, Status);
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::DoProgress(TFileOperationProgressType & ProgressData,
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
void /* __fastcall */ TTerminal::DoFinished(TFileOperation Operation, TOperationSide Side, bool Temp,
  const UnicodeString & FileName, bool Success, TOnceDoneOperation & OnceDoneOperation)
{
  if (!GetOnFinished().empty())
  {
    TCallbackGuard Guard(this);
    GetOnFinished()(Operation, Side, Temp, FileName, Success, OnceDoneOperation);
    Guard.Verify();
  }
}
//---------------------------------------------------------------------------
bool /* __fastcall */ TTerminal::GetIsCapable(TFSCapability Capability) const
{
  assert(FFileSystem);
  return FFileSystem->IsCapable(Capability);
}
//---------------------------------------------------------------------------
UnicodeString /* __fastcall */ TTerminal::AbsolutePath(UnicodeString Path, bool Local)
{
  return FFileSystem->AbsolutePath(Path, Local);
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::ReactOnCommand(int /*TFSCommand*/ Cmd)
{
  bool ChangesDirectory = false;
  bool ModifiesFiles = false;

  switch (static_cast<TFSCommand>(Cmd)) {
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
  else if (ModifiesFiles && GetAutoReadDirectory() && Configuration->GetAutoReadDirectoryAfterOp())
  {
    if (!InTransaction()) { ReadDirectory(true); }
    else { FReadDirectoryPending = true; }
  }
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::TerminalError(UnicodeString Msg)
{
  TerminalError(NULL, Msg);
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::TerminalError(Exception * E, UnicodeString Msg)
{
  throw ETerminal(Msg, E);
}
//---------------------------------------------------------------------------
bool /* __fastcall */ TTerminal::DoQueryReopen(Exception * E)
{
  EFatal * Fatal = dynamic_cast<EFatal *>(E);
  assert(Fatal != NULL);
  bool Result = false;
  if ((Fatal != NULL) && Fatal->GetReopenQueried())
  {
    Result = false;
  }
  else
  {
    int NumberOfRetries = FSessionData->GetNumberOfRetries();
    if (NumberOfRetries >= FConfiguration->GetSessionReopenAutoMaximumNumberOfRetries())
    {
      LogEvent(FORMAT(L"Reached maximum number of retries: %d", FConfiguration->GetSessionReopenAutoMaximumNumberOfRetries()));
    }
    else
    {
      LogEvent(L"Connection was lost, asking what to do.");

      NumberOfRetries++;
      FSessionData->SetNumberOfRetries(NumberOfRetries);

      TQueryParams Params(qpAllowContinueOnError);
      Params.Timeout = Configuration->GetSessionReopenAuto();
      Params.TimeoutAnswer = qaRetry;
      TQueryButtonAlias Aliases[1];
      Aliases[0].Button = qaRetry;
      Aliases[0].Alias = LoadStr(RECONNECT_BUTTON);
      Params.Aliases = Aliases;
      Params.AliasesCount = LENOF(Aliases);
      Result = (QueryUserException(L"", E, qaRetry | qaAbort, &Params, qtError) == qaRetry);

    }
    if (Fatal != NULL)
    {
      Fatal->SetReopenQueried(true);
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
bool /* __fastcall */ TTerminal::QueryReopen(Exception * E, int Params,
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
        FSessionData->SetNumberOfRetries(0);
      }
      catch(Exception & E)
      {
        if (!GetActive())
        {
          Result =
            ((Configuration->GetSessionReopenTimeout() == 0) ||
             (int(double(Now() - Start) * MSecsPerDay) < Configuration->GetSessionReopenTimeout())) &&
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
bool /* __fastcall */ TTerminal::FileOperationLoopQuery(Exception & E,
  TFileOperationProgressType * OperationProgress, const UnicodeString Message,
  bool AllowSkip, UnicodeString SpecialRetry)
{
  bool Result = false;
  GetLog()->AddException(&E);
  unsigned int Answer;

  if (AllowSkip && OperationProgress->SkipToAll)
  {
    Answer = qaSkip;
  }
  else
  {
    int Answers = qaRetry | qaAbort |
      FLAGMASK(AllowSkip, (qaSkip | qaAll)) |
      FLAGMASK(!SpecialRetry.IsEmpty(), qaYes);
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
      THROW_SKIP_FILE(Message, &E);
    }
    else
    {
      // this can happen only during file transfer with SCP
      throw ExtException(Message, &E);
    }
  }

  return Result;
}
//---------------------------------------------------------------------------
int /* __fastcall */ TTerminal::FileOperationLoop(const TFileOperationEvent & CallBackFunc,
  TFileOperationProgressType * OperationProgress, bool AllowSkip,
  const UnicodeString Message, void * Param1, void * Param2)
{
  // assert(CallBackFunc);
  fileoperation_signal_type sig;
  sig.connect(CallBackFunc);
  int Result = 0;
  FILE_OPERATION_LOOP_EX
  (
    AllowSkip, Message,
    Result = sig(Param1, Param2);
  );

  return Result;
}
//---------------------------------------------------------------------------
UnicodeString /* __fastcall */ TTerminal::TranslateLockedPath(UnicodeString Path, bool Lock)
{
  if (!GetSessionData()->GetLockInHome() || Path.IsEmpty() || (Path[1] != L'/'))
  {
    return Path;
  }

  if (Lock)
  {
    if (Path.SubString(1, FLockDirectory.Length()) == FLockDirectory)
    {
      Path.Delete(1, FLockDirectory.Length());
      if (Path.IsEmpty()) Path = L"/";
    }
  }
  else
  {
    Path = UnixExcludeTrailingBackslash(FLockDirectory + Path);
  }
  return Path;
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::ClearCaches()
{
  FDirectoryCache->Clear();
  if (FDirectoryChangesCache != NULL)
  {
    FDirectoryChangesCache->Clear();
  }
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::ClearCachedFileList(const UnicodeString Path,
  bool SubDirs)
{
  FDirectoryCache->ClearFileList(Path, SubDirs);
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::AddCachedFileList(TRemoteFileList * FileList)
{
  FDirectoryCache->AddFileList(FileList);
}
//---------------------------------------------------------------------------
bool /* __fastcall */ TTerminal::DirectoryFileList(const UnicodeString Path,
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
      catch(...)
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
void /* __fastcall */ TTerminal::SetCurrentDirectory(UnicodeString value)
{
  assert(FFileSystem);
  value = TranslateLockedPath(value, false);
  if (value != FFileSystem->GetCurrentDirectory())
  {
    ChangeDirectory(value);
  }
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TTerminal::GetCurrentDirectory()
{
  if (FFileSystem)
  {
    UnicodeString currentDirectory = FFileSystem->GetCurrentDirectory();
    if (FCurrentDirectory != currentDirectory)
    {
      FCurrentDirectory = currentDirectory;
      if (FCurrentDirectory.IsEmpty())
      {
        ReadCurrentDirectory();
      }
    }
  }

  return TranslateLockedPath(FCurrentDirectory, true);
}
//---------------------------------------------------------------------------
UnicodeString /* __fastcall */ TTerminal::PeekCurrentDirectory()
{
  if (FFileSystem)
  {
    FCurrentDirectory = FFileSystem->GetCurrentDirectory();
  }

  return TranslateLockedPath(FCurrentDirectory, true);
}
//---------------------------------------------------------------------------
const TRemoteTokenList * /* __fastcall */ TTerminal::GetGroups()
{
  assert(FFileSystem);
  LookupUsersGroups();
  return &FGroups;
}
//---------------------------------------------------------------------------
const TRemoteTokenList * /* __fastcall */ TTerminal::GetUsers()
{
  assert(FFileSystem);
  LookupUsersGroups();
  return &FUsers;
}
//---------------------------------------------------------------------------
const TRemoteTokenList * /* __fastcall */ TTerminal::GetMembership()
{
  assert(FFileSystem);
  LookupUsersGroups();
  return &FMembership;
}
//---------------------------------------------------------------------------
UnicodeString /* __fastcall */ TTerminal::GetUserName() const
{
  // in future might also be implemented to detect username similar to GetUserGroups
  assert(FFileSystem != NULL);
  UnicodeString Result = FFileSystem->GetUserName();
  // Is empty also when stored username was used
  if (Result.IsEmpty())
  {
    Result = GetSessionData()->GetUserNameExpanded();
  }
  return Result;
}
//---------------------------------------------------------------------------
bool /* __fastcall */ TTerminal::GetAreCachesEmpty() const
{
  return FDirectoryCache->GetIsEmpty() &&
    ((FDirectoryChangesCache == NULL) || FDirectoryChangesCache->GetIsEmpty());
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::DoChangeDirectory()
{
  if (!FOnChangeDirectory.empty())
  {
    TCallbackGuard Guard(this);
    FOnChangeDirectory(this);
    Guard.Verify();
  }
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::DoReadDirectory(bool ReloadOnly)
{
  if (!FOnReadDirectory.empty())
  {
    TCallbackGuard Guard(this);
    FOnReadDirectory(this, ReloadOnly);
    Guard.Verify();
  }
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::DoStartReadDirectory()
{
  if (!FOnStartReadDirectory.empty())
  {
    TCallbackGuard Guard(this);
    FOnStartReadDirectory(this);
    Guard.Verify();
  }
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::DoReadDirectoryProgress(int Progress, bool & Cancel)
{
  if (FReadingCurrentDirectory && (!FOnReadDirectoryProgress.empty()))
  {
    TCallbackGuard Guard(this);
    FOnReadDirectoryProgress(this, Progress, Cancel);
    Guard.Verify();
  }
  if (!FOnFindingFile.empty())
  {
    TCallbackGuard Guard(this);
    FOnFindingFile(this, L"", Cancel);
    Guard.Verify();
  }
}
//---------------------------------------------------------------------------
bool /* __fastcall */ TTerminal::InTransaction()
{
  return (FInTransaction > 0) && !FSuspendTransaction;
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::BeginTransaction()
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
void /* __fastcall */ TTerminal::EndTransaction()
{
  if (FInTransaction == 0)
  {
    TerminalError(L"Can't end transaction, not in transaction");
  }
  assert(FInTransaction > 0);
  FInTransaction--;

  // it connection was closed due to fatal error during transaction, do nothing
  if (GetActive())
  {
    if (FInTransaction == 0)
    {
      // try
      {
        BOOST_SCOPE_EXIT ( (&Self) )
        {
          Self->FReadCurrentDirectoryPending = false;
          Self->FReadDirectoryPending = false;
        } BOOST_SCOPE_EXIT_END
        if (FReadCurrentDirectoryPending) { ReadCurrentDirectory(); }
        if (FReadDirectoryPending) { ReadDirectory(!FReadCurrentDirectoryPending); }
      }
#ifndef _MSC_VER
      __finally
      {
        FReadCurrentDirectoryPending = false;
        FReadDirectoryPending = false;
      }
#endif
    }
  }

  if (FCommandSession != NULL)
  {
    FCommandSession->EndTransaction();
  }
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::SetExceptionOnFail(bool value)
{
  if (value)
  {
    FExceptionOnFail++;
  }
  else
  {
    if (FExceptionOnFail == 0)
    {
      throw Exception(L"ExceptionOnFail is already zero.");
    }
    FExceptionOnFail--;
  }

  if (FCommandSession != NULL)
  {
    FCommandSession->FExceptionOnFail = FExceptionOnFail;
  }
}
//---------------------------------------------------------------------------
bool /* __fastcall */ TTerminal::GetExceptionOnFail() const
{
  return static_cast<bool>(FExceptionOnFail > 0);
}
//---------------------------------------------------------------------------
void __fastcall TTerminal::FatalAbort()
{
  FatalError(NULL, L"");
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::FatalError(Exception * E, UnicodeString Msg)
{
  bool SecureShellActive = (FSecureShell != NULL) && FSecureShell->GetActive();
  if (GetActive() || SecureShellActive)
  {
    // We log this instead of exception handler, because Close() would
    // probably cause exception handler to loose pointer to TShellLog()
    LogEvent(L"Attempt to close connection due to fatal exception:");
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
    throw ESshFatal(Msg, E);
  }
}
//---------------------------------------------------------------------------
void __fastcall TTerminal::CommandError(Exception * E, const UnicodeString Msg)
{
  CommandError(E, Msg, 0);
}
//---------------------------------------------------------------------------
unsigned int __fastcall TTerminal::CommandError(Exception * E, const UnicodeString Msg,
  unsigned int Answers)
{
  // may not be, particularly when TTerminal::Reopen is being called
  // from within OnShowExtendedException handler
  assert(FCallbackGuard == NULL);
  unsigned int Result = 0;
  if (E && ::InheritsFrom<Exception, EFatal>(E))
  {
    FatalError(E, Msg);
  }
  else if (E && ::InheritsFrom<Exception, EAbort>(E))
  {
    // resent EAbort exception
    Abort();
  }
  else if (GetExceptionOnFail())
  {
    throw ECommand(Msg, E);
  }
  else if (!Answers)
  {
    ECommand * ECmd = new ECommand(Msg, E);
    // try
    {
      BOOST_SCOPE_EXIT ( (&ECmd) )
      {
        delete ECmd;
      } BOOST_SCOPE_EXIT_END
      HandleExtendedException(ECmd);
    }
#ifndef _MSC_VER
    __finally
    {
      delete ECmd;
    }
#endif
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
bool /* __fastcall */ TTerminal::HandleException(Exception * E)
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
void /* __fastcall */ TTerminal::CloseOnCompletion(TOnceDoneOperation Operation, const UnicodeString Message)
{
  LogEvent(L"Closing session after completed operation (as requested by user)");
  Close();
  throw ESshTerminate(
    Message.IsEmpty() ? UnicodeString(LoadStr(CLOSED_ON_COMPLETION)) : Message,
    NULL,
    Operation);
}
//---------------------------------------------------------------------------
TBatchOverwrite /* __fastcall */ TTerminal::EffectiveBatchOverwrite(
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
bool /* __fastcall */ TTerminal::CheckRemoteFile(int Params, TFileOperationProgressType * OperationProgress)
{
  return (EffectiveBatchOverwrite(Params, OperationProgress, true) != boAll);
}
//---------------------------------------------------------------------------
unsigned int /* __fastcall */ TTerminal::ConfirmFileOverwrite(const UnicodeString FileName,
  const TOverwriteFileParams * FileParams, unsigned int Answers, const TQueryParams * QueryParams,
  TOperationSide Side, int Params, TFileOperationProgressType * OperationProgress,
  UnicodeString Message)
{
  unsigned int Result = 0;
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
    if (Message.IsEmpty())
    {
      Message = FMTLOAD((Side == osLocal ? LOCAL_FILE_OVERWRITE :
        REMOTE_FILE_OVERWRITE), FileName.c_str());
    }
    if (FileParams != NULL)
    {
      Message = FMTLOAD(FILE_OVERWRITE_DETAILS, Message.c_str(),
        Int64ToStr(FileParams->SourceSize).c_str(),
        UserModificationStr(FileParams->SourceTimestamp, FileParams->SourcePrecision).c_str(),
        Int64ToStr(FileParams->DestSize).c_str(),
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
void /* __fastcall */ TTerminal::FileModified(const TRemoteFile * File,
  const UnicodeString FileName, bool ClearDirectoryChange)
{
  UnicodeString ParentDirectory;
  UnicodeString Directory;

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
    else if (!FileName.IsEmpty())
    {
      ParentDirectory = UnixExtractFilePath(FileName);
      if (ParentDirectory.IsEmpty())
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
    if (!Directory.IsEmpty())
    {
      DirectoryModified(Directory, true);
    }
    if (!ParentDirectory.IsEmpty())
    {
      DirectoryModified(ParentDirectory, false);
    }
  }

  if (GetSessionData()->GetCacheDirectoryChanges() && ClearDirectoryChange)
  {
    if (!Directory.IsEmpty())
    {
      FDirectoryChangesCache->ClearDirectoryChange(Directory);
      FDirectoryChangesCache->ClearDirectoryChangeTarget(Directory);
    }
  }
}
//----/* __fastcall */ -----------------------------------------------------------------------
void /* __fastcall */ TTerminal::DirectoryModified(const UnicodeString Path, bool SubDirs)
{
  if (Path.IsEmpty())
  {
    ClearCachedFileList(GetCurrentDirectory(), SubDirs);
  }
  else
  {
    ClearCachedFileList(Path, SubDirs);
  }
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::DirectoryLoaded(TRemoteFileList * FileList)
{
  AddCachedFileList(FileList);
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::ReloadDirectory()
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
void /* __fastcall */ TTerminal::RefreshDirectory()
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
void /* __fastcall */ TTerminal::EnsureNonExistence(const UnicodeString FileName)
{
  // if filename doesn't contain path, we check for existence of file
  if ((UnixExtractFileDir(FileName).IsEmpty()) &&
      UnixComparePaths(GetCurrentDirectory(), FFiles->GetDirectory()))
  {
    TRemoteFile * File = FFiles->FindFile(FileName);
    if (File)
    {
      if (File->GetIsDirectory()) { throw ECommand(FMTLOAD(RENAME_CREATE_DIR_EXISTS, FileName.c_str()), NULL); }
      else { throw ECommand(FMTLOAD(RENAME_CREATE_FILE_EXISTS, FileName.c_str()), NULL); }
    }
  }
}
//---------------------------------------------------------------------------
void __fastcall TTerminal::LogEvent(const UnicodeString & Str)
{
  if (GetLog()->GetLogging())
  {
    GetLog()->Add(llMessage, Str);
  }
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::RollbackAction(TSessionAction & Action,
  TFileOperationProgressType * OperationProgress, Exception * E)
{
  // EScpSkipFile without "cancel" is file skip,
  // and we do not want to record skipped actions.
  // But EScpSkipFile with "cancel" is abort and we want to record that.
  // Note that TSCPFileSystem modifies the logic of RollbackAction little bit.
  if ((dynamic_cast<EScpSkipFile *>(E) != NULL) &&
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
void /* __fastcall */ TTerminal::DoStartup()
{
  LogEvent(L"Doing startup conversation with host.");
  BeginTransaction();
  // try
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
    if (!GetSessionData()->GetRemoteDirectory().IsEmpty())
    {
      ChangeDirectory(GetSessionData()->GetRemoteDirectory());
    }

  }
#ifndef _MSC_VER
  __finally
  {
    EndTransaction();
  }
#endif
  LogEvent(L"Startup conversation with host finished.");
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::ReadCurrentDirectory()
{
  assert(FFileSystem);
  try
  {
    // reset flag is case we are called externally (like from console dialog)
    FReadCurrentDirectoryPending = false;

    LogEvent(L"Getting current directory name.");
    UnicodeString OldDirectory = FFileSystem->GetCurrentDirectory();

    FFileSystem->ReadCurrentDirectory();
    ReactOnCommand(fsCurrentDirectory);

    if (GetSessionData()->GetCacheDirectoryChanges())
    {
      assert(FDirectoryChangesCache != NULL);
      UnicodeString currentDirectory = GetCurrentDirectory();
      if (!currentDirectory.IsEmpty() && !FLastDirectoryChange.IsEmpty() && (currentDirectory != OldDirectory))
      {
        FDirectoryChangesCache->AddDirectoryChange(OldDirectory,
            FLastDirectoryChange, currentDirectory);
      }
      // not to broke the cache, if the next directory change would not
      // be initialited by ChangeDirectory(), which sets it
      // (HomeDirectory() particularly)
      FLastDirectoryChange = L"";
    }

    if (OldDirectory.IsEmpty())
    {
      FLockDirectory = (GetSessionData()->GetLockInHome() ?
        FFileSystem->GetCurrentDirectory() : UnicodeString(L""));
    }
    if (OldDirectory != FFileSystem->GetCurrentDirectory()) { DoChangeDirectory(); }
  }
  catch (Exception &E)
  {
    CommandError(&E, LoadStr(READ_CURRENT_DIR_ERROR));
  }
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::ReadDirectory(bool ReloadOnly, bool ForceCache)
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
      // try
      {
        BOOST_SCOPE_EXIT ( (&Self) (&ReloadOnly) )
        {
          Self->DoReadDirectory(ReloadOnly);
        } BOOST_SCOPE_EXIT_END
        LoadedFromCache = FDirectoryCache->GetFileList(GetCurrentDirectory(), FFiles);
      }
#ifndef _MSC_VER
      __finally
      {
        DoReadDirectory(ReloadOnly);
      }
#endif

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
      TRemoteDirectory * Files = new TRemoteDirectory(this, FFiles);
      // try
      {
        BOOST_SCOPE_EXIT ( (&Self) (&Files) (&Cancel) (&ReloadOnly) )
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
#ifndef _MSC_VER
      __finally
      {
        DoReadDirectoryProgress(-1, Cancel);
        FReadingCurrentDirectory = false;
        delete FFiles;
        FFiles = Files;
        DoReadDirectory(ReloadOnly);
        if (Active)
        {
          if (SessionData->CacheDirectories)
          {
            DirectoryLoaded(FFiles);
          }
        }
      }
#endif
    }
    catch (Exception &E)
    {
      CommandError(&E, FmtLoadStr(LIST_DIR_ERROR, FFiles->GetDirectory().c_str()));
    }
  }
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::CustomReadDirectory(TRemoteFileList * FileList)
{
  assert(FileList);
  assert(FFileSystem);
  FFileSystem->ReadDirectory(FileList);
  ReactOnCommand(fsListDirectory);
}
//---------------------------------------------------------------------------
TRemoteFileList * /* __fastcall */ TTerminal::ReadDirectoryListing(UnicodeString Directory, const TFileMasks & Mask)
{
  TLsSessionAction Action(GetActionLog(), AbsolutePath(Directory, true));
  TRemoteFileList * FileList = NULL;
  try
  {
    FileList = DoReadDirectoryListing(Directory, false);
    if (FileList != NULL)
    {
      int Index = 0;
      while (Index < FileList->GetCount())
      {
        TRemoteFile * File = FileList->GetFiles(Index);
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
  catch(Exception & E)
  {
    COMMAND_ERROR_ARI_ACTION
    (
      L"",
      FileList = ReadDirectoryListing(Directory, Mask),
      Action
    );
  }
  return FileList;
}
//---------------------------------------------------------------------------
TRemoteFile * __fastcall TTerminal::ReadFileListing(UnicodeString Path)
{
  TStatSessionAction Action(GetActionLog(), AbsolutePath(Path, true));
  TRemoteFile * File = NULL;
  try
  {
    // reset caches
    AnnounceFileListOperation();
    ReadFile(Path, File);
    Action.File(File);
  }
  catch(Exception & E)
  {
    COMMAND_ERROR_ARI_ACTION
    (
      L"",
      File = ReadFileListing(Path),
      Action
    );
  }
  return File;
}
//---------------------------------------------------------------------------
TRemoteFileList * /* __fastcall */ TTerminal::CustomReadDirectoryListing(UnicodeString Directory, bool UseCache)
{
  TRemoteFileList * FileList = NULL;
  try
  {
    FileList = DoReadDirectoryListing(Directory, UseCache);
  }
  catch(Exception & E)
  {
    COMMAND_ERROR_ARI
    (
      L"",
      FileList = CustomReadDirectoryListing(Directory, UseCache);
    );
  }
  return FileList;
}
//---------------------------------------------------------------------------
TRemoteFileList * /* __fastcall */ TTerminal::DoReadDirectoryListing(UnicodeString Directory, bool UseCache)
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
      // try
      {
        BOOST_SCOPE_EXIT ( (&Self) )
        {
          Self->SetExceptionOnFail(false);
        } BOOST_SCOPE_EXIT_END
        ReadDirectory(FileList);
      }
#ifndef _MSC_VER
      __finally
      {
        ExceptionOnFail = false;
      }
#endif

      if (Cache)
      {
        AddCachedFileList(FileList);
      }
    }
  }
  catch(...)
  {
    delete FileList;
    throw;
  }
  return FileList;
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::ProcessDirectory(const UnicodeString DirName,
  TProcessFileEvent CallBackFunc, void * Param, bool UseCache, bool IgnoreErrors)
{
  TRemoteFileList * FileList = NULL;
  if (IgnoreErrors)
  {
    SetExceptionOnFail(true);
    // try
    {
      BOOST_SCOPE_EXIT ( (&Self) )
      {
        Self->SetExceptionOnFail(false);
      } BOOST_SCOPE_EXIT_END
      try
      {
        FileList = CustomReadDirectoryListing(DirName, UseCache);
      }
      catch(...)
      {
        if (!GetActive())
        {
          throw;
        }
      }
    }
#ifndef _MSC_VER
    __finally
    {
      ExceptionOnFail = false;
    }
#endif
  }
  else
  {
    FileList = CustomReadDirectoryListing(DirName, UseCache);
  }

  // skip if directory listing fails and user selects "skip"
  if (FileList)
  {
    // try
    {
      BOOST_SCOPE_EXIT ( (&FileList) )
      {
        delete FileList;
      } BOOST_SCOPE_EXIT_END
      UnicodeString Directory = UnixIncludeTrailingBackslash(DirName);

      TRemoteFile * File;
      processfile_signal_type sig;
      sig.connect(CallBackFunc);
      for (int Index = 0; Index < FileList->GetCount(); Index++)
      {
        File = FileList->GetFiles(Index);
        if (!File->GetIsParentDirectory() && !File->GetIsThisDirectory())
        {
          sig(Directory + File->GetFileName(), File, Param);
        }
      }
    }
#ifndef _MSC_VER
    __finally
    {
      delete FileList;
    }
#endif
  }
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::ReadDirectory(TRemoteFileList * FileList)
{
  try
  {
    CustomReadDirectory(FileList);
  }
  catch (Exception &E)
  {
    CommandError(&E, FmtLoadStr(LIST_DIR_ERROR, FileList->GetDirectory().c_str()));
  }
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::ReadSymlink(TRemoteFile * SymlinkFile,
  TRemoteFile *& File)
{
  assert(FFileSystem);
  try
  {
    LogEvent(FORMAT(L"Reading symlink \"%s\".", SymlinkFile->GetFileName().c_str()));
    FFileSystem->ReadSymlink(SymlinkFile, File);
    ReactOnCommand(fsReadSymlink);
  }
  catch (Exception &E)
  {
    CommandError(&E, FMTLOAD(READ_SYMLINK_ERROR, SymlinkFile->GetFileName().c_str()));
  }
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::ReadFile(const UnicodeString FileName,
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
  catch (Exception &E)
  {
    if (File) { delete File; }
    File = NULL;
    CommandError(&E, FMTLOAD(CANT_GET_ATTRS, FileName.c_str()));
  }
}
//---------------------------------------------------------------------------
bool /* __fastcall */ TTerminal::FileExists(const UnicodeString FileName, TRemoteFile ** AFile)
{
  bool Result;
  TRemoteFile * File = NULL;
  try
  {
    SetExceptionOnFail(true);
    // try
    {
      BOOST_SCOPE_EXIT ( (&Self) )
      {
        Self->SetExceptionOnFail(false);
      } BOOST_SCOPE_EXIT_END
      ReadFile(FileName, File);
    }
#ifndef _MSC_VER
    __finally
    {
      ExceptionOnFail = false;
    }
#endif

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
  catch(...)
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
void /* __fastcall */ TTerminal::AnnounceFileListOperation()
{
  FFileSystem->AnnounceFileListOperation();
}
//---------------------------------------------------------------------------
bool /* __fastcall */ TTerminal::ProcessFiles(TStrings * FileList,
  TFileOperation Operation, const TProcessFileEvent & ProcessFile, void * Param,
  TOperationSide Side, bool Ex)
{
  assert(FFileSystem);
  assert(FileList);

  bool Result = false;
  TOnceDoneOperation OnceDoneOperation = odoIdle;

  try
  {
    TFileOperationProgressType * OperationProgress = new TFileOperationProgressType(boost::bind(&TTerminal::DoProgress, this, _1, _2), boost::bind(&TTerminal::DoFinished, this, _1, _2, _3, _4, _5, _6));
    OperationProgress->Start(Operation, Side, FileList->GetCount());

    FOperationProgress = OperationProgress;
    // try
    {
      BOOST_SCOPE_EXIT ( (&Self) (&OperationProgress) )
      {
        Self->FOperationProgress = NULL;
        OperationProgress->Stop();
        delete OperationProgress;
        OperationProgress = NULL;
      }
      BOOST_SCOPE_EXIT_END
      if (Side == osRemote)
      {
        BeginTransaction();
      }

      // try
      {
        BOOST_SCOPE_EXIT ( (&Self) (&Side) )
        {
          if (Side == osRemote)
          {
            Self->EndTransaction();
          }
        } BOOST_SCOPE_EXIT_END
        size_t Index = 0;
        UnicodeString FileName;
        bool Success;
        processfile_signal_type sig;
        sig.connect(ProcessFile);
        while ((Index < FileList->GetCount()) && (OperationProgress->Cancel == csContinue))
        {
          FileName = FileList->GetStrings(Index);
          try
          {
            // try
            {
              BOOST_SCOPE_EXIT ( (&OperationProgress) (&FileName) (&Success) (&OnceDoneOperation) )
              {
                OperationProgress->Finish(FileName, Success, OnceDoneOperation);
              } BOOST_SCOPE_EXIT_END
              Success = false;
              if (!Ex)
              {
                TRemoteFile * RemoteFile = static_cast<TRemoteFile *>(FileList->GetObjects(Index));
                sig(FileName, RemoteFile, Param);
              }
              else
              {
                // not used anymore
                // TProcessFileEventEx ProcessFileEx = (TProcessFileEventEx)ProcessFile;
                // ProcessFileEx(FileName, (TRemoteFile *)FileList->GetObjects(Index), Param, Index);
              }
              Success = true;
            }
#ifndef _MSC_VER
            __finally
            {
              OperationProgress->Finish(FileName, Success, OnceDoneOperation);
            }
#endif
          }
          catch(EScpSkipFile & E)
          {
            DEBUG_PRINTF(L"before HandleException");
            SUSPEND_OPERATION (
              if (!HandleException(&E)) throw;
            );
          }
          Index++;
        }
      }
#ifndef _MSC_VER
      __finally
      {
        if (Side == osRemote)
        {
          EndTransaction();
        }
      }
#endif

      if (OperationProgress->Cancel == csContinue)
      {
        Result = true;
      }
    }
#ifndef _MSC_VER
    __finally
    {
      FOperationProgress = NULL;
      OperationProgress->Stop();
      delete OperationProgress;
      OperationProgress = NULL;
    }
#endif
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
bool __fastcall TTerminal::ProcessFilesEx(TStrings * FileList, TFileOperation Operation,
  TProcessFileEventEx ProcessFile, void * Param, TOperationSide Side)
{
  return false; // ProcessFiles(FileList, Operation, TProcessFileEvent(ProcessFile),
    // Param, Side, true);
}
//---------------------------------------------------------------------------
TStrings * /* __fastcall */ TTerminal::GetFixedPaths()
{
  assert(FFileSystem != NULL);
  return FFileSystem->GetFixedPaths();
}
//---------------------------------------------------------------------------
bool /* __fastcall */ TTerminal::GetResolvingSymlinks()
{
  return GetSessionData()->GetResolveSymlinks() && GetIsCapable(fcResolveSymlink);
}
//---------------------------------------------------------------------------
TUsableCopyParamAttrs /* __fastcall */ TTerminal::UsableCopyParamAttrs(int Params)
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
bool /* __fastcall */ TTerminal::IsRecycledFile(UnicodeString FileName)
{
  UnicodeString Path = UnixExtractFilePath(FileName);
  if (Path.IsEmpty())
  {
    Path = GetCurrentDirectory();
  }
  return UnixComparePaths(Path, GetSessionData()->GetRecycleBinPath());
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::RecycleFile(UnicodeString FileName,
  const TRemoteFile * File)
{
  if (FileName.IsEmpty())
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
void /* __fastcall */ TTerminal::DeleteFile(UnicodeString FileName,
  const TRemoteFile * File, void * AParams)
{
  if (FileName.IsEmpty() && File)
  {
    FileName = File->GetFileName();
  }
  if (GetOperationProgress() && GetOperationProgress()->Operation == foDelete)
  {
    if (GetOperationProgress()->Cancel != csContinue) { Abort(); }
    GetOperationProgress()->SetFile(FileName);
  }
  int Params = (AParams != NULL) ? *(static_cast<int*>(AParams)) : 0;
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
    if (File) { FileModified(File, FileName, true); }
    DoDeleteFile(FileName, File, Params);
    ReactOnCommand(fsDeleteFile);
  }
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::DoDeleteFile(const UnicodeString FileName,
  const TRemoteFile * File, int Params)
{
  TRmSessionAction Action(GetActionLog(), AbsolutePath(FileName, true));
  try
  {
    assert(FFileSystem);
    // 'File' parameter: SFTPFileSystem needs to know if file is file or directory
    FFileSystem->DeleteFile(FileName, File, Params, Action);
  }
  catch(Exception & E)
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
bool /* __fastcall */ TTerminal::DeleteFiles(TStrings * FilesToDelete, int Params)
{
  // TODO: avoid resolving symlinks while reading subdirectories.
  // Resolving does not work anyway for relative symlinks in subdirectories
  // (at least for SFTP).
  return ProcessFiles(FilesToDelete, foDelete, boost::bind(&TTerminal::DeleteFile, this, _1, _2, _3), &Params);
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::DeleteLocalFile(UnicodeString FileName,
  const TRemoteFile * /*File*/, void * Params)
{
  if (!GetOnDeleteLocalFile().empty())
  {
    if (!RecursiveDeleteFile(FileName, false))
    {
      throw Exception(FMTLOAD(DELETE_FILE_ERROR, FileName.c_str()));
    }
  }
  else
  {
    GetOnDeleteLocalFile()(FileName, FLAGSET(*(static_cast<int *>(Params)), dfAlternative));
  }
}
//---------------------------------------------------------------------------
bool /* __fastcall */ TTerminal::DeleteLocalFiles(TStrings * FileList, int Params)
{
  return ProcessFiles(FileList, foDelete, boost::bind(&TTerminal::DeleteLocalFile, this, _1, _2, _3), &Params, osLocal);
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::CustomCommandOnFile(UnicodeString FileName,
  const TRemoteFile * File, void * AParams)
{
  TCustomCommandParams * Params = (static_cast<TCustomCommandParams *>(AParams));
  if (FileName.IsEmpty() && File)
  {
    FileName = File->GetFileName();
  }
  if (GetOperationProgress() && GetOperationProgress()->Operation == foCustomCommand)
  {
    if (GetOperationProgress()->Cancel != csContinue) { Abort(); }
    GetOperationProgress()->SetFile(FileName);
  }
  LogEvent(FORMAT(L"Executing custom command \"%s\" (%d) on file \"%s\".",
    Params->Command.c_str(), Params->Params, FileName.c_str()));
  if (File) { FileModified(File, FileName); }
  DoCustomCommandOnFile(FileName, File, Params->Command, Params->Params,
    Params->OutputEvent);
  ReactOnCommand(fsAnyCommand);
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::DoCustomCommandOnFile(UnicodeString FileName,
  const TRemoteFile * File, UnicodeString Command, int Params,
  TCaptureOutputEvent * OutputEvent)
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
  catch(Exception & E)
  {
    COMMAND_ERROR_ARI
    (
      FMTLOAD(CUSTOM_COMMAND_ERROR, Command.c_str(), FileName.c_str()),
      DoCustomCommandOnFile(FileName, File, Command, Params, OutputEvent)
    );
  }
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::CustomCommandOnFiles(UnicodeString Command,
  int Params, TStrings * Files, TCaptureOutputEvent * OutputEvent)
{
  if (!TRemoteCustomCommand().IsFileListCommand(Command))
  {
    TCustomCommandParams AParams(Command, Params, OutputEvent);
    // AParams.Command = Command;
    // AParams.Params = Params;
    // AParams.OutputEvent.connect(OutputEvent);
    // AParams.OutputEvent = OutputEvent;
    ProcessFiles(Files, foCustomCommand, boost::bind(&TTerminal::CustomCommandOnFile, this, _1, _2, _3), &AParams);
  }
  else
  {
    UnicodeString FileList;
    for (int i = 0; i < Files->GetCount(); i++)
    {
      TRemoteFile * File = static_cast<TRemoteFile *>(Files->GetObjects(i));
      bool Dir = File->GetIsDirectory() && !File->GetIsSymLink();

      if (!Dir || FLAGSET(Params, ccApplyToDirectories))
      {
        if (!FileList.IsEmpty())
        {
          FileList += L" ";
        }

        FileList += L"\"" + ShellDelimitStr(Files->GetStrings(i), '"') + L"\"";
      }
    }

    TCustomCommandData Data(this);
    UnicodeString Cmd =
      TRemoteCustomCommand(Data, GetCurrentDirectory(), L"", FileList).
        Complete(Command, true);
    DoAnyCommand(Cmd, OutputEvent, NULL);
  }
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::ChangeFileProperties(UnicodeString FileName,
  const TRemoteFile * File, /*const TRemoteProperties*/ void * Properties)
{
  TRemoteProperties * RProperties = static_cast<TRemoteProperties *>(Properties);
  assert(RProperties && !RProperties->Valid.Empty());

  if (FileName.IsEmpty() && File)
  {
    FileName = File->GetFileName();
  }
  if (GetOperationProgress() && GetOperationProgress()->Operation == foSetProperties)
  {
    if (GetOperationProgress()->Cancel != csContinue) { Abort(); }
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
void /* __fastcall */ TTerminal::DoChangeFileProperties(const UnicodeString FileName,
  const TRemoteFile * File, const TRemoteProperties * Properties)
{
  TChmodSessionAction Action(GetActionLog(), AbsolutePath(FileName, true));
  try
  {
    assert(FFileSystem);
    FFileSystem->ChangeFileProperties(FileName, File, Properties, Action);
  }
  catch(Exception & E)
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
void /* __fastcall */ TTerminal::ChangeFilesProperties(TStrings * FileList,
  const TRemoteProperties * Properties)
{
  AnnounceFileListOperation();
  ProcessFiles(FileList, foSetProperties, boost::bind(&TTerminal::ChangeFileProperties, this, _1, _2, _3), const_cast<void *>(static_cast<const void *>(Properties)));
}
//---------------------------------------------------------------------------
bool /* __fastcall */ TTerminal::LoadFilesProperties(TStrings * FileList)
{
  bool Result =
    GetIsCapable(fcLoadingAdditionalProperties) &&
    FFileSystem->LoadFilesProperties(FileList);
  if (Result && GetSessionData()->GetCacheDirectories() &&
      (FileList->GetCount() > 0) &&
      (dynamic_cast<TRemoteFile *>(FileList->GetObjects(0))->GetDirectory() == FFiles))
  {
    AddCachedFileList(FFiles);
  }
  return Result;
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::CalculateFileSize(UnicodeString FileName,
  const TRemoteFile * File, /*TCalculateSizeParams*/ void * Param)
{
  assert(Param);
  assert(File);
  TCalculateSizeParams * AParams = static_cast<TCalculateSizeParams *>(Param);

  if (FileName.IsEmpty())
  {
    FileName = File->GetFileName();
  }

  bool AllowTransfer = (AParams->CopyParam == NULL);
  if (!AllowTransfer)
  {
    TFileMasks::TParams MaskParams;
    MaskParams.Size = File->GetSize();
    MaskParams.Modification = File->GetModification();

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
    if (GetOperationProgress()->Cancel != csContinue) { Abort(); }
    GetOperationProgress()->SetFile(FileName);
  }
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::DoCalculateDirectorySize(const UnicodeString FileName,
  const TRemoteFile * File, TCalculateSizeParams * Params)
{
  try
  {
    ProcessDirectory(FileName, boost::bind(&TTerminal::CalculateFileSize, this, _1, _2, _3), Params);
  }
  catch(Exception & E)
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
void /* __fastcall */ TTerminal::CalculateFilesSize(TStrings * FileList,
  __int64 & Size, int Params, const TCopyParamType * CopyParam,
  TCalculateSizeStats * Stats)
{
  TCalculateSizeParams Param;
  Param.Size = 0;
  Param.Params = Params;
  Param.CopyParam = CopyParam;
  Param.Stats = Stats;
  ProcessFiles(FileList, foCalculateSize, boost::bind(&TTerminal::CalculateFileSize, this, _1, _2, _3), &Param);
  Size = Param.Size;
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::CalculateFilesChecksum(const UnicodeString & Alg,
  TStrings * FileList, TStrings * Checksums,
  TCalculatedChecksumEvent OnCalculatedChecksum)
{
  FFileSystem->CalculateFilesChecksum(Alg, FileList, Checksums, OnCalculatedChecksum);
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::RenameFile(const UnicodeString FileName,
  const UnicodeString NewName)
{
  LogEvent(FORMAT(L"Renaming file \"%s\" to \"%s\".", FileName.c_str(), NewName.c_str()));
  DoRenameFile(FileName, NewName, false);
  ReactOnCommand(fsRenameFile);
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::RenameFile(const TRemoteFile * File,
  const UnicodeString NewName, bool CheckExistence)
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
      UnicodeString QuestionFmt;
      if (DuplicateFile->GetIsDirectory())
      {
        QuestionFmt = LoadStr(PROMPT_DIRECTORY_OVERWRITE);
      }
      else
      {
        QuestionFmt = LoadStr(PROMPT_FILE_OVERWRITE);
      }
      TQueryParams Params(qpNeverAskAgainCheck);
      unsigned int Result = QueryUser(FORMAT(QuestionFmt.c_str(), NewName.c_str()), NULL,
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
void /* __fastcall */ TTerminal::DoRenameFile(const UnicodeString FileName,
  const UnicodeString NewName, bool Move)
{
  TMvSessionAction Action(GetActionLog(), AbsolutePath(FileName, true), AbsolutePath(NewName, true));
  try
  {
    assert(FFileSystem);
    FFileSystem->RenameFile(FileName, NewName);
  }
  catch(Exception & E)
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
void /* __fastcall */ TTerminal::MoveFile(const UnicodeString FileName,
  const TRemoteFile * File, /*const TMoveFileParams*/ void * Param)
{
  if (GetOperationProgress() &&
      ((GetOperationProgress()->Operation == foRemoteMove) ||
       (GetOperationProgress()->Operation == foDelete)))
  {
    if (GetOperationProgress()->Cancel != csContinue) { Abort(); }
    GetOperationProgress()->SetFile(FileName);
  }

  assert(Param != NULL);
  const TMoveFileParams & Params = *static_cast<const TMoveFileParams*>(Param);
  UnicodeString NewName = UnixIncludeTrailingBackslash(Params.Target) +
    MaskFileName(UnixExtractFileName(FileName), Params.FileMask);
  LogEvent(FORMAT(L"Moving file \"%s\" to \"%s\".", FileName.c_str(), NewName.c_str()));
  FileModified(File, FileName);
  DoRenameFile(FileName, NewName, true);
  ReactOnCommand(fsMoveFile);
}
//---------------------------------------------------------------------------
bool /* __fastcall */ TTerminal::MoveFiles(TStrings * FileList, const UnicodeString Target,
  const UnicodeString FileMask)
{
  TMoveFileParams Params;
  Params.Target = Target;
  Params.FileMask = FileMask;
  DirectoryModified(Target, true);
  bool Result;
  BeginTransaction();
  // try
  {
    BOOST_SCOPE_EXIT ( (&Self) (&FileList) )
    {
      if (Self->GetActive())
      {
        UnicodeString WithTrailing = UnixIncludeTrailingBackslash(Self->GetCurrentDirectory());
        bool PossiblyMoved = false;
        // check if we was moving current directory.
        // this is just optimization to avoid checking existence of current
        // directory after each move operation.
        UnicodeString curDirectory = Self->GetCurrentDirectory();
        for (size_t Index = 0; !PossiblyMoved && (Index < FileList->GetCount()); Index++)
        {
          const TRemoteFile * File =
            dynamic_cast<const TRemoteFile *>(FileList->GetObjects(Index));
          // File can be NULL, and filename may not be full path,
          // but currently this is the only way we can move (at least in GUI)
          // current directory
          if ((File != NULL) &&
              File->GetIsDirectory() &&
              ((curDirectory.SubString(0, FileList->GetStrings(Index).Length()) == FileList->GetStrings(Index)) &&
               ((FileList->GetStrings(Index).Length() == curDirectory.Length()) ||
                (curDirectory[FileList->GetStrings(Index).Length() + 1] == '/'))))
          {
            PossiblyMoved = true;
          }
        }

        if (PossiblyMoved && !Self->FileExists(curDirectory))
        {
          UnicodeString NearestExisting = curDirectory;
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
    Result = ProcessFiles(FileList, foRemoteMove, boost::bind(&TTerminal::MoveFile, this, _1, _2, _3), &Params);
  }
#ifndef _MSC_VER
  __finally
  {
    if (Active)
    {
      UnicodeString WithTrailing = UnixIncludeTrailingBackslash(CurrentDirectory);
      bool PossiblyMoved = false;
      // check if we was moving current directory.
      // this is just optimization to avoid checking existence of current
      // directory after each move operation.
      for (int Index = 0; !PossiblyMoved && (Index < FileList->Count); Index++)
      {
        const TRemoteFile * File =
          dynamic_cast<const TRemoteFile *>(FileList->Objects[Index]);
        // File can be NULL, and filename may not be full path,
        // but currently this is the only way we can move (at least in GUI)
        // current directory
        if ((File != NULL) &&
            File->IsDirectory &&
            ((CurrentDirectory.SubString(1, FileList->Strings[Index].Length()) == FileList->Strings[Index]) &&
             ((FileList->Strings[Index].Length() == CurrentDirectory.Length()) ||
              (CurrentDirectory[FileList->Strings[Index].Length() + 1] == L'/'))))
        {
          PossiblyMoved = true;
        }
      }

      if (PossiblyMoved && !FileExists(CurrentDirectory))
      {
        UnicodeString NearestExisting = CurrentDirectory;
        do
        {
          NearestExisting = UnixExtractFileDir(NearestExisting);
        }
        while (!IsUnixRootPath(NearestExisting) && !FileExists(NearestExisting));

        ChangeDirectory(NearestExisting);
      }
    }
    EndTransaction();
  }
#endif
  return Result;
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::DoCopyFile(const UnicodeString FileName,
  const UnicodeString NewName)
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
  catch(Exception & E)
  {
    COMMAND_ERROR_ARI
    (
      FMTLOAD(COPY_FILE_ERROR, FileName.c_str(), NewName.c_str()),
      DoCopyFile(FileName, NewName)
    );
  }
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::CopyFile(const UnicodeString FileName,
  const TRemoteFile * /*File*/, /*const TMoveFileParams*/ void * Param)
{
  if (GetOperationProgress() && (GetOperationProgress()->Operation == foRemoteCopy))
  {
    if (GetOperationProgress()->Cancel != csContinue) { Abort(); }
    GetOperationProgress()->SetFile(FileName);
  }

  assert(Param != NULL);
  const TMoveFileParams & Params = *static_cast<const TMoveFileParams*>(Param);
  UnicodeString NewName = UnixIncludeTrailingBackslash(Params.Target) +
    MaskFileName(UnixExtractFileName(FileName), Params.FileMask);
  LogEvent(FORMAT(L"Copying file \"%s\" to \"%s\".", FileName.c_str(), NewName.c_str()));
  DoCopyFile(FileName, NewName);
  ReactOnCommand(fsCopyFile);
}
//---------------------------------------------------------------------------
bool /* __fastcall */ TTerminal::CopyFiles(TStrings * FileList, const UnicodeString Target,
  const UnicodeString FileMask)
{
  TMoveFileParams Params;
  Params.Target = Target;
  Params.FileMask = FileMask;
  DirectoryModified(Target, true);
  return ProcessFiles(FileList, foRemoteCopy, boost::bind(&TTerminal::CopyFile, this, _1, _2, _3), &Params);
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::CreateDirectory(const UnicodeString DirName,
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
void /* __fastcall */ TTerminal::DoCreateDirectory(const UnicodeString DirName)
{
  TMkdirSessionAction Action(GetActionLog(), AbsolutePath(DirName, true));
  try
  {
    assert(FFileSystem);
    FFileSystem->CreateDirectory(DirName);
  }
  catch(Exception & E)
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
void /* __fastcall */ TTerminal::CreateLink(const UnicodeString FileName,
  const UnicodeString PointTo, bool Symbolic)
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
void /* __fastcall */ TTerminal::DoCreateLink(const UnicodeString FileName,
  const UnicodeString PointTo, bool Symbolic)
{
  try
  {
    assert(FFileSystem);
    FFileSystem->CreateLink(FileName, PointTo, Symbolic);
  }
  catch(Exception & E)
  {
    COMMAND_ERROR_ARI
    (
      FMTLOAD(CREATE_LINK_ERROR, FileName.c_str()),
      DoCreateLink(FileName, PointTo, Symbolic);
    );
  }
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::HomeDirectory()
{
  assert(FFileSystem);
  try
  {
    LogEvent(L"Changing directory to home directory.");
    FFileSystem->HomeDirectory();
    ReactOnCommand(fsHomeDirectory);
  }
  catch (Exception &E)
  {
    CommandError(&E, LoadStr(CHANGE_HOMEDIR_ERROR));
  }
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::ChangeDirectory(const UnicodeString Directory)
{
  UnicodeString DirectoryNormalized = ::ToUnixPath(Directory);
  assert(FFileSystem);
  try
  {
    UnicodeString CachedDirectory;
    assert(!GetSessionData()->GetCacheDirectoryChanges() || (FDirectoryChangesCache != NULL));
    // never use directory change cache during startup, this ensures, we never
    // end-up initially in non-existing directory
    if ((GetStatus() == ssOpened) &&
        GetSessionData()->GetCacheDirectoryChanges() &&
        FDirectoryChangesCache->GetDirectoryChange(PeekCurrentDirectory(),
          DirectoryNormalized, CachedDirectory))
    {
      LogEvent(FORMAT(L"Cached directory change via \"%s\" to \"%s\".",
        DirectoryNormalized.c_str(), CachedDirectory.c_str()));
      FFileSystem->CachedChangeDirectory(CachedDirectory);
    }
    else
    {
      LogEvent(FORMAT(L"Changing directory to \"%s\".", DirectoryNormalized.c_str()));
      FFileSystem->ChangeDirectory(DirectoryNormalized);
    }
    FLastDirectoryChange = DirectoryNormalized;
    ReactOnCommand(fsChangeDirectory);
  }
  catch (Exception &E)
  {
    CommandError(&E, FMTLOAD(CHANGE_DIR_ERROR, DirectoryNormalized.c_str()));
  }
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::LookupUsersGroups()
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
        FGroups.Log(this, L"groups");
        FMembership.Log(this, L"membership");
        FUsers.Log(this, L"users");
      }
    }
    catch (Exception &E)
    {
      if (!GetActive() || (GetSessionData()->GetLookupUserGroups() == asOn))
      {
        CommandError(&E, LoadStr(LOOKUP_GROUPS_ERROR));
      }
    }
  }
}
//---------------------------------------------------------------------------
bool /* __fastcall */ TTerminal::AllowedAnyCommand(const UnicodeString Command)
{
  return !Command.Trim().IsEmpty();
}
//---------------------------------------------------------------------------
bool /* __fastcall */ TTerminal::GetCommandSessionOpened()
{
  // consider secodary terminal open in "ready" state only
  // so we never do keepalives on it until it is completelly initialised
  return (FCommandSession != NULL) &&
    (FCommandSession->GetStatus() == ssOpened);
}
//---------------------------------------------------------------------------
TTerminal * /* __fastcall */ TTerminal::GetCommandSession()
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
      FCommandSession = new TSecondaryTerminal(this);
      (static_cast<TSecondaryTerminal *>(FCommandSession))->Init(GetSessionData(),
        Configuration, L"Shell");

      FCommandSession->SetAutoReadDirectory(false);

      TSessionData * CommandSessionData = FCommandSession->FSessionData;
      CommandSessionData->SetRemoteDirectory(GetCurrentDirectory());
      CommandSessionData->SetFSProtocol(fsSCPonly);
      CommandSessionData->SetClearAliases(false);
      CommandSessionData->SetUnsetNationalVars(false);
      CommandSessionData->SetLookupUserGroups(asOn);

      FCommandSession->FExceptionOnFail = FExceptionOnFail;

      FCommandSession->SetOnQueryUser(GetOnQueryUser());
      FCommandSession->SetOnPromptUser(GetOnPromptUser());
      FCommandSession->SetOnShowExtendedException(GetOnShowExtendedException());
      FCommandSession->SetOnProgress(GetOnProgress());
      FCommandSession->SetOnFinished(GetOnFinished());
      FCommandSession->SetOnInformation(GetOnInformation());
      // do not copy OnDisplayBanner to avoid it being displayed
    }
    catch(...)
    {
      SAFE_DESTROY(FCommandSession);
      throw;
    }
  }

  return FCommandSession;
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::AnyCommand(const UnicodeString Command,
  TCaptureOutputEvent * OutputEvent)
{

  class TOutputProxy
  {
  public:
    /* __fastcall */ TOutputProxy(TCallSessionAction & Action, const TCaptureOutputEvent * OutputEvent) :
      FAction(Action)
    {
      if (OutputEvent)
      {
        FOutputEvent.connect(*OutputEvent);
      }
    }

    void /* __fastcall */ Output(const UnicodeString & Str, bool StdError)
    {
      FAction.AddOutput(Str, StdError);
      if (!FOutputEvent.empty())
      {
        FOutputEvent(Str, StdError);
      }
    }

  private:
    TCallSessionAction & FAction;
    captureoutput_signal_type FOutputEvent;
  private:
#pragma warning(push)
#pragma warning(disable: 4822)
    TOutputProxy(const TOutputProxy &);
    void operator=(const TOutputProxy &);
#pragma warning(pop)
  };

  TCallSessionAction Action(GetActionLog(), Command, GetCurrentDirectory());
  TOutputProxy ProxyOutputEvent(Action, OutputEvent);
  TCaptureOutputEvent * outputEvent = reinterpret_cast<TCaptureOutputEvent *>(&boost::bind(&TOutputProxy::Output, &ProxyOutputEvent, _1, _2));
  DoAnyCommand(Command, outputEvent, &Action);
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::DoAnyCommand(const UnicodeString Command,
  TCaptureOutputEvent * OutputEvent, TCallSessionAction * Action)
{
  assert(FFileSystem);
  try
  {
    DirectoryModified(GetCurrentDirectory(), false);
    if (GetIsCapable(fcAnyCommand))
    {
      LogEvent(L"Executing user defined command.");
      FFileSystem->AnyCommand(Command, OutputEvent);
    }
    else
    {
      assert(GetCommandSessionOpened());
      assert(FCommandSession->GetFSProtocol() == cfsSCP);
      LogEvent(L"Executing user defined command on command session.");

      FCommandSession->SetCurrentDirectory(GetCurrentDirectory());
      FCommandSession->FFileSystem->AnyCommand(Command, OutputEvent);

      FCommandSession->FFileSystem->ReadCurrentDirectory();

      // synchronize pwd (by purpose we lose transaction optimalisation here)
      ChangeDirectory(FCommandSession->GetCurrentDirectory());
    }
    ReactOnCommand(fsAnyCommand);
  }
  catch (Exception &E)
  {
    if (Action != NULL)
    {
      RollbackAction(*Action, NULL, &E);
    }
    if (GetExceptionOnFail() || ::InheritsFrom<Exception, EFatal>(&E)) { throw; }
      else { HandleExtendedException(&E); }
  }
}
//---------------------------------------------------------------------------
bool /* __fastcall */ TTerminal::DoCreateLocalFile(const UnicodeString FileName,
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
            unsigned int Answer;
            SUSPEND_OPERATION
            (
              Answer = QueryUser(
                FMTLOAD(READ_ONLY_OVERWRITE, FileName.c_str()), NULL,
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
bool /* __fastcall */ TTerminal::CreateLocalFile(const UnicodeString FileName,
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
void /* __fastcall */ TTerminal::OpenLocalFile(const UnicodeString FileName,
  unsigned int Access, int * AAttrs, HANDLE * AHandle, __int64 * ACTime,
  __int64 * AMTime, __int64 * AATime, __int64 * ASize,
  bool TryWriteReadOnly)
{
  int Attrs = 0;
  HANDLE Handle = 0;
  TFileOperationProgressType * OperationProgress = GetOperationProgress();
  FILE_OPERATION_LOOP (FMTLOAD(FILE_NOT_EXISTS, FileName.c_str()),
    Attrs = FileGetAttr(FileName);
    // if ((Attrs == -1) && (Access != GENERIC_WRITE)) RaiseLastOSError();
    if (Attrs == -1) { RaiseLastOSError(); }
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
          if ((LSize == 0xFFFFFFFF) && (GetLastError() != NO_ERROR)) { RaiseLastOSError(); }
          *ASize = (__int64(HSize) << 32) + LSize;
        );
      }

      if ((AHandle == NULL) || NoHandle)
      {
        CloseHandle(Handle);
        Handle = NULL;
      }
    }
    catch(...)
    {
      CloseHandle(Handle);
      throw;
    }
  }

  if (AAttrs) { *AAttrs = Attrs; }
  if (AHandle) { *AHandle = Handle; }
}
//---------------------------------------------------------------------------
bool /* __fastcall */ TTerminal::AllowLocalFileTransfer(UnicodeString FileName,
  const TCopyParamType * CopyParam)
{
  bool Result = true;
  TFileOperationProgressType * OperationProgress = GetOperationProgress();
  if (!CopyParam->AllowAnyTransfer())
  {
    WIN32_FIND_DATA FindData = {0};
    HANDLE Handle = 0;
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
    Params.Modification = FileTimeToDateTime(FindData.ftLastWriteTime);
    Result = CopyParam->AllowTransfer(FileName, osLocal, Directory, Params);
  }
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString /* __fastcall */ TTerminal::FileUrl(const UnicodeString Protocol,
  const UnicodeString FileName)
{
  assert(FileName.Length() > 0);
  return Protocol + L"://" + EncodeUrlChars(GetSessionData()->GetSessionName()) +
    (FileName[1] == L'/' ? L"" : L"/") + EncodeUrlChars(FileName, L"/");
}
//---------------------------------------------------------------------------
UnicodeString /* __fastcall */ TTerminal::FileUrl(const UnicodeString FileName)
{
  return FFileSystem->FileUrl(FileName);
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::MakeLocalFileList(const UnicodeString FileName,
  const TSearchRec Rec, void * Param)
{
  TMakeLocalFileListParams & Params = *static_cast<TMakeLocalFileListParams *>(Param);

  bool Directory = FLAGSET(Rec.Attr, faDirectory);
  if (Directory && Params.Recursive)
  {
    ProcessLocalDirectory(FileName, boost::bind(&TTerminal::MakeLocalFileList, this, _1, _2, _3), &Params);
  }

  if (!Directory || Params.IncludeDirs)
  {
    Params.FileList->Add(FileName);
  }
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::CalculateLocalFileSize(const UnicodeString FileName,
  const TSearchRec Rec, /*__int64*/  void * Params)
{
  TCalculateSizeParams * AParams = static_cast<TCalculateSizeParams*>(Params);

  bool Dir = FLAGSET(Rec.Attr, faDirectory);

  bool AllowTransfer = (AParams->CopyParam == NULL);
  __int64 Size =
    (static_cast<__int64>(Rec.FindData.nFileSizeHigh) << 32) +
    Rec.FindData.nFileSizeLow;
  if (!AllowTransfer)
  {
    TFileMasks::TParams MaskParams;
    MaskParams.Size = Size;
    MaskParams.Modification = FileTimeToDateTime(Rec.FindData.ftLastWriteTime);

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
      ProcessLocalDirectory(FileName, boost::bind(&TTerminal::CalculateLocalFileSize, this, _1, _2, _3), Params);
    }
  }

  if (GetOperationProgress() && GetOperationProgress()->Operation == foCalculateSize)
  {
    if (GetOperationProgress()->Cancel != csContinue) { Abort(); }
    GetOperationProgress()->SetFile(FileName);
  }
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::CalculateLocalFilesSize(TStrings * FileList,
  __int64 & Size, const TCopyParamType * CopyParam)
{
  TFileOperationProgressType OperationProgress(boost::bind(&TTerminal::DoProgress, this, _1, _2), boost::bind(&TTerminal::DoFinished, this, _1, _2, _3, _4, _5, _6));
  TOnceDoneOperation OnceDoneOperation = odoIdle;
  OperationProgress.Start(foCalculateSize, osLocal, FileList->GetCount());
  // try
  {
    BOOST_SCOPE_EXIT ( (&Self) (&OperationProgress) )
    {
      Self->FOperationProgress = NULL;
      OperationProgress.Stop();
    } BOOST_SCOPE_EXIT_END
    TCalculateSizeParams Params;
    Params.Size = 0;
    Params.Params = 0;
    Params.CopyParam = CopyParam;

    assert(!FOperationProgress);
    FOperationProgress = &OperationProgress;
    TSearchRec Rec = {0};
    for (int Index = 0; Index < FileList->GetCount(); Index++)
    {
      UnicodeString FileName = FileList->GetStrings(Index);
      if (FileSearchRec(FileName, Rec))
      {
        CalculateLocalFileSize(FileName, Rec, &Params);
        OperationProgress.Finish(FileName, true, OnceDoneOperation);
      }
    }

    Size = Params.Size;
  }
#ifndef _MSC_VER
  __finally
  {
    FOperationProgress = NULL;
    OperationProgress.Stop();
  }
#endif

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
  UnicodeString LocalDirectory;
  UnicodeString RemoteDirectory;
  TTerminal::TSynchronizeMode Mode;
  int Params;
  const TSynchronizeDirectory * OnSynchronizeDirectory;
  TSynchronizeOptions * Options;
  int Flags;
  TStringList * LocalFileList;
  const TCopyParamType * CopyParam;
  TSynchronizeChecklist * Checklist;
};
//---------------------------------------------------------------------------
TSynchronizeChecklist * /* __fastcall */ TTerminal::SynchronizeCollect(const UnicodeString LocalDirectory,
  const UnicodeString RemoteDirectory, TSynchronizeMode Mode,
  const TCopyParamType * CopyParam, int Params,
  const TSynchronizeDirectory & OnSynchronizeDirectory,
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
  catch(...)
  {
    delete Checklist;
    throw;
  }
  return Checklist;
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::DoSynchronizeCollectDirectory(const UnicodeString LocalDirectory,
  const UnicodeString RemoteDirectory, TSynchronizeMode Mode,
  const TCopyParamType * CopyParam, int Params,
  const TSynchronizeDirectory & OnSynchronizeDirectory, TSynchronizeOptions * Options,
  int Flags, TSynchronizeChecklist * Checklist)
{
  TSynchronizeData Data;

  Data.LocalDirectory = IncludeTrailingBackslash(LocalDirectory);
  Data.RemoteDirectory = UnixIncludeTrailingBackslash(RemoteDirectory);
  Data.Mode = Mode;
  Data.Params = Params;
  Data.OnSynchronizeDirectory = &OnSynchronizeDirectory;
  Data.LocalFileList = NULL;
  Data.CopyParam = CopyParam;
  Data.Options = Options;
  Data.Flags = Flags;
  Data.Checklist = Checklist;

  TFileOperationProgressType * OperationProgress = GetOperationProgress();
  LogEvent(FORMAT(L"Collecting synchronization list for local directory '%s' and remote directory '%s', "
    L"mode = %d, params = %d", LocalDirectory.c_str(), RemoteDirectory.c_str(),
    int(Mode), int(Params)));

  if (FLAGCLEAR(Params, spDelayProgress))
  {
    DoSynchronizeProgress(Data, true);
  }

  // try
  {
    BOOST_SCOPE_EXIT ( (&Data) )
    {
      if (Data.LocalFileList != NULL)
      {
        for (size_t Index = 0; Index < Data.LocalFileList->GetCount(); Index++)
        {
          TSynchronizeFileData * FileData = reinterpret_cast<TSynchronizeFileData *>
            (Data.LocalFileList->GetObjects(Index));
          delete FileData;
        }
        delete Data.LocalFileList;
      }
    } BOOST_SCOPE_EXIT_END
    bool Found = false;
    TSearchRec SearchRec = {0};
    Data.LocalFileList = new TStringList();
    Data.LocalFileList->SetSorted(true);
    Data.LocalFileList->SetCaseSensitive(false);

    FILE_OPERATION_LOOP (FMTLOAD(LIST_DIR_ERROR, LocalDirectory.c_str()),
      int FindAttrs = faReadOnly | faHidden | faSysFile | faDirectory | faArchive;
      Found = (FindFirst(Data.LocalDirectory + L"*.*", FindAttrs, SearchRec) == 0);
    );

    if (Found)
    {
      // try
      {
        BOOST_SCOPE_EXIT ( (&SearchRec) )
        {
          FindClose(SearchRec);
        } BOOST_SCOPE_EXIT_END
        UnicodeString FileName;
        while (Found)
        {
          FileName = SearchRec.Name;
          // add dirs for recursive mode or when we are interested in newly
          // added subdirs
          __int64 Size =
            (static_cast<__int64>(SearchRec.FindData.nFileSizeHigh) << 32) +
            SearchRec.FindData.nFileSizeLow;
          TDateTime Modification = FileTimeToDateTime(SearchRec.FindData.ftLastWriteTime);
          TFileMasks::TParams MaskParams;
          MaskParams.Size = Size;
          MaskParams.Modification = Modification;
          UnicodeString RemoteFileName =
            CopyParam->ChangeFileName(FileName, osLocal, false);
          if ((FileName != THISDIRECTORY) && (FileName != PARENTDIRECTORY) &&
              CopyParam->AllowTransfer(Data.LocalDirectory + FileName, osLocal,
                FLAGSET(SearchRec.Attr, faDirectory), MaskParams) &&
              !FFileSystem->TemporaryTransferFile(FileName) &&
              (FLAGCLEAR(Flags, sfFirstLevel) ||
               (Options == NULL) ||
               Options->MatchesFilter(FileName) ||
               Options->MatchesFilter(RemoteFileName)))
          {
            TSynchronizeFileData * FileData = new TSynchronizeFileData;

            FileData->IsDirectory = FLAGSET(SearchRec.Attr, faDirectory);
            FileData->Info.FileName = FileName;
            FileData->Info.Directory = Data.LocalDirectory;
            FileData->Info.Modification = Modification;
            FileData->Info.ModificationFmt = mfFull;
            FileData->Info.Size = Size;
            FileData->LocalLastWriteTime = SearchRec.FindData.ftLastWriteTime;
            FileData->New = true;
            FileData->Modified = false;
            Data.LocalFileList->AddObject(FileName,
              reinterpret_cast<TObject*>(FileData));
          }

          FILE_OPERATION_LOOP (FMTLOAD(LIST_DIR_ERROR, LocalDirectory.c_str()),
            Found = (FindNext(SearchRec) == 0);
          );
        }
      }
#ifndef _MSC_VER
      __finally
      {
        FindClose(SearchRec);
      }
#endif

      // can we expect that ProcessDirectory would take so little time
      // that we can pospone showing progress window until anything actually happens?
      bool Cached = FLAGSET(Params, spUseCache) && GetSessionData()->GetCacheDirectories() &&
        FDirectoryCache->HasFileList(RemoteDirectory);

      if (!Cached && FLAGSET(Params, spDelayProgress))
      {
        DoSynchronizeProgress(Data, true);
      }

      ProcessDirectory(RemoteDirectory, boost::bind(&TTerminal::SynchronizeCollectFile, this, _1, _2, _3), &Data,
        FLAGSET(Params, spUseCache));

      TSynchronizeFileData * FileData;
      for (int Index = 0; Index < Data.LocalFileList->GetCount(); Index++)
      {
        FileData = reinterpret_cast<TSynchronizeFileData *>
          (Data.LocalFileList->GetObjects(Index));
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
          // try
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
              assert(!FileData->MatchingRemoteFile.Directory.IsEmpty());
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
#ifndef _MSC_VER
          __finally
          {
            delete ChecklistItem;
          }
#endif
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
#ifndef _MSC_VER
  __finally
  {
    if (Data.LocalFileList != NULL)
    {
      for (int Index = 0; Index < Data.LocalFileList->Count; Index++)
      {
        TSynchronizeFileData * FileData = reinterpret_cast<TSynchronizeFileData*>
          (Data.LocalFileList->Objects[Index]);
        delete FileData;
      }
      delete Data.LocalFileList;
    }
  }
#endif
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::SynchronizeCollectFile(const UnicodeString FileName,
  const TRemoteFile * File, /*TSynchronizeData*/ void * Param)
{
  TSynchronizeData * Data = static_cast<TSynchronizeData *>(Param);

  TFileMasks::TParams MaskParams;
  MaskParams.Size = File->GetSize();
  MaskParams.Modification = File->GetModification();
  UnicodeString LocalFileName =
    Data->CopyParam->ChangeFileName(File->GetFileName(), osRemote, false);
  if (Data->CopyParam->AllowTransfer(
        UnixExcludeTrailingBackslash(File->GetFullFileName()), osRemote,
        File->GetIsDirectory(), MaskParams) &&
      !FFileSystem->TemporaryTransferFile(File->GetFileName()) &&
      (FLAGCLEAR(Data->Flags, sfFirstLevel) ||
       (Data->Options == NULL) ||
        Data->Options->MatchesFilter(File->GetFileName()) ||
        Data->Options->MatchesFilter(LocalFileName)))
  {
    TSynchronizeChecklist::TItem * ChecklistItem = new TSynchronizeChecklist::TItem();
    // try
    {
      BOOST_SCOPE_EXIT ( (&ChecklistItem) )
      {
        delete ChecklistItem;
      }
      BOOST_SCOPE_EXIT_END
      ChecklistItem->IsDirectory = File->GetIsDirectory();
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
          reinterpret_cast<TSynchronizeFileData *>(Data->LocalFileList->GetObjects(LocalIndex));

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
            Data->Mode, Data->CopyParam, Data->Params, *Data->OnSynchronizeDirectory,
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
#ifndef _MSC_VER
    __finally
    {
      delete ChecklistItem;
    }
#endif
  }
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::SynchronizeApply(TSynchronizeChecklist * Checklist,
  const UnicodeString LocalDirectory, const UnicodeString RemoteDirectory,
  const TCopyParamType * CopyParam, int Params,
  const TSynchronizeDirectory & OnSynchronizeDirectory)
{
  TSynchronizeData Data;

  Data.OnSynchronizeDirectory = &OnSynchronizeDirectory;

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

  // try
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

      UnicodeString CurrentLocalDirectory = ChecklistItem->Local.Directory;
      UnicodeString CurrentRemoteDirectory = ChecklistItem->Remote.Directory;

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
                  static_cast<TObject *>(const_cast<void *>(static_cast<const void *>(ChecklistItem))));
                break;

              case TSynchronizeChecklist::saUploadUpdate:
                UploadList->AddObject(
                  IncludeTrailingBackslash(ChecklistItem->Local.Directory) +
                    ChecklistItem->Local.FileName,
                  static_cast<TObject *>(const_cast<void *>(static_cast<const void *>(ChecklistItem))));
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
              boost::bind(&TTerminal::SynchronizeLocalTimestamp, this, _1, _2, _3), NULL, osLocal);
          }

          if (UploadList->GetCount() > 0)
          {
            ProcessFiles(UploadList, foSetProperties,
              boost::bind(&TTerminal::SynchronizeRemoteTimestamp, this, _1, _2, _3));
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
#ifndef _MSC_VER
  __finally
  {
    delete DownloadList;
    delete DeleteRemoteList;
    delete UploadList;
    delete DeleteLocalList;

    EndTransaction();
  }
#endif
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::DoSynchronizeProgress(const TSynchronizeData & Data,
  bool Collect)
{
  if (Data.OnSynchronizeDirectory != NULL)
  {
    bool Continue = true;
    synchronizedirectory_signal_type sig;
    if (Data.OnSynchronizeDirectory)
    {
      sig.connect(*Data.OnSynchronizeDirectory);
      sig(Data.LocalDirectory, Data.RemoteDirectory,
        Continue, Collect);
    }

    if (!Continue)
    {
      Abort();
    }
  }
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::SynchronizeLocalTimestamp(const UnicodeString /*FileName*/,
  const TRemoteFile * File, void * /*Param*/)
{
  TFileOperationProgressType * OperationProgress = GetOperationProgress();
  const TSynchronizeChecklist::TItem * ChecklistItem =
    reinterpret_cast<const TSynchronizeChecklist::TItem *>(File);

  UnicodeString LocalFile =
    IncludeTrailingBackslash(ChecklistItem->Local.Directory) +
      ChecklistItem->Local.FileName;

  FILE_OPERATION_LOOP (FMTLOAD(CANT_SET_ATTRS, LocalFile.c_str()),
    HANDLE Handle;
    OpenLocalFile(LocalFile, GENERIC_WRITE, NULL, &Handle,
      NULL, NULL, NULL, NULL);
    FILETIME WrTime = DateTimeToFileTime(ChecklistItem->Remote.Modification,
      GetSessionData()->GetDSTMode());
    bool Result = SetFileTime(Handle, NULL, NULL, &WrTime) > 0;
    CloseHandle(Handle);
    if (!Result)
    {
      Abort();
    }
  );
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::SynchronizeRemoteTimestamp(const UnicodeString /*FileName*/,
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
void /* __fastcall */ TTerminal::FileFind(UnicodeString FileName,
  const TRemoteFile * File, /*TFilesFindParams*/ void * Param)
{
  // see DoFilesFind
  FOnFindingFile.disconnect_all_slots();

  assert(Param);
  assert(File);
  TFilesFindParams * AParams = static_cast<TFilesFindParams *>(Param);

  if (!AParams->Cancel)
  {
    if (FileName.IsEmpty())
    {
      FileName = File->GetFileName();
    }

    TFileMasks::TParams MaskParams;
    MaskParams.Size = File->GetSize();
    MaskParams.Modification = File->GetModification();

    UnicodeString FullFileName = UnixExcludeTrailingBackslash(File->GetFullFileName());
    if (AParams->FileMask.Matches(FullFileName, false,
         File->GetIsDirectory(), &MaskParams))
    {
      filefound_signal_type sig;
      if (AParams->OnFileFound)
      {
        sig.connect(*AParams->OnFileFound);
      }
      sig(this, FileName, File, AParams->Cancel);
    }

    if (File->GetIsDirectory())
    {
      DoFilesFind(FullFileName, *AParams);
    }
  }
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::DoFilesFind(UnicodeString Directory, TFilesFindParams & Params)
{
  findingfile_signal_type sig;
  if (Params.OnFindingFile)
  {
    sig.connect(*Params.OnFindingFile);
  }
  sig(this, Directory, Params.Cancel);
  if (!Params.Cancel)
  {
    assert(FOnFindingFile.empty());
    // ideally we should set the handler only around actually reading
    // of the directory listing, so we at least reset the handler in
    // FileFind
    if (Params.OnFindingFile)
    {
      FOnFindingFile.connect(*Params.OnFindingFile);
    }
    // try
    {
      BOOST_SCOPE_EXIT ( (&Self) )
      {
        Self->FOnFindingFile.disconnect_all_slots();
      } BOOST_SCOPE_EXIT_END
      ProcessDirectory(Directory, boost::bind(&TTerminal::FileFind, this, _1, _2, _3), &Params, false, true);
    }
#ifndef _MSC_VER
    __finally
    {
      FOnFindingFile = NULL;
    }
#endif
  }
}
//---------------------------------------------------------------------------
void __fastcall TTerminal::FilesFind(UnicodeString Directory, const TFileMasks & FileMask,
  const TFileFoundEvent * OnFileFound, const TFindingFileEvent * OnFindingFile)
{
  TFilesFindParams Params;
  Params.FileMask = FileMask;
  Params.OnFileFound = OnFileFound;
  Params.OnFindingFile = OnFindingFile;
  Params.Cancel = false;
  DoFilesFind(Directory, Params);
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::SpaceAvailable(const UnicodeString Path,
  TSpaceAvailable & ASpaceAvailable)
{
  assert(GetIsCapable(fcCheckingSpaceAvailable));

  try
  {
    FFileSystem->SpaceAvailable(Path, ASpaceAvailable);
  }
  catch (Exception &E)
  {
    CommandError(&E, FMTLOAD(SPACE_AVAILABLE_ERROR, Path.c_str()));
  }
}
//---------------------------------------------------------------------------
const TSessionInfo & /* __fastcall */ TTerminal::GetSessionInfo()
{
  return FFileSystem->GetSessionInfo();
}
//---------------------------------------------------------------------------
const TFileSystemInfo & /* __fastcall */ TTerminal::GetFileSystemInfo(bool Retrieve)
{
  return FFileSystem->GetFileSystemInfo(Retrieve);
}
//---------------------------------------------------------------------
UnicodeString /* __fastcall */ TTerminal::GetPassword()
{
  UnicodeString Result;
  // FPassword is empty also when stored password was used
  if (FPassword.IsEmpty())
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
UnicodeString /* __fastcall */ TTerminal::GetTunnelPassword()
{
  UnicodeString Result;
  // FTunnelPassword is empty also when stored password was used
  if (FTunnelPassword.IsEmpty())
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
bool /* __fastcall */ TTerminal::GetStoredCredentialsTried()
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
bool /* __fastcall */ TTerminal::CopyToRemote(TStrings * FilesToCopy,
  const UnicodeString TargetDir, const TCopyParamType * CopyParam, int Params)
{
  assert(FFileSystem);
  assert(FilesToCopy);

  assert(GetIsCapable(fcNewerOnlyUpload) || FLAGCLEAR(Params, cpNewerOnly));

  bool Result = false;
  TOnceDoneOperation OnceDoneOperation = odoIdle;

  TFileOperationProgressType OperationProgress(boost::bind(&TTerminal::DoProgress, this, _1, _2), boost::bind(&TTerminal::DoFinished, this, _1, _2, _3, _4, _5, _6));
  try
  {

    __int64 Size = 0;
    if (CopyParam->GetCalculateSize())
    {
      // dirty trick: when moving, do not pass copy param to avoid exclude mask
      CalculateLocalFilesSize(FilesToCopy, Size,
        (FLAGCLEAR(Params, cpDelete) ? CopyParam : NULL));
    }

    OperationProgress.Start((Params & cpDelete ? foMove : foCopy), osLocal,
      FilesToCopy->GetCount(), (Params & cpTemporary) > 0, TargetDir, CopyParam->GetCPSLimit());

    FOperationProgress = &OperationProgress;
    // try
    {
      BOOST_SCOPE_EXIT ( (&Self) (&OperationProgress) )
      {
        OperationProgress.Stop();
        Self->FOperationProgress = NULL;
      } BOOST_SCOPE_EXIT_END
      if (CopyParam->GetCalculateSize())
      {
        OperationProgress.SetTotalSize(Size);
      }

      UnicodeString UnlockedTargetDir = TranslateLockedPath(TargetDir, false);
      BeginTransaction();
      // try
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
          CopyParam, Params, &OperationProgress, OnceDoneOperation);
      }
#ifndef _MSC_VER
      __finally
      {
        if (Active)
        {
          ReactOnCommand(fsCopyToRemote);
        }
        EndTransaction();
      }
#endif

      if (OperationProgress.Cancel == csContinue)
      {
        Result = true;
      }
    }
#ifndef _MSC_VER
    __finally
    {
      OperationProgress.Stop();
      FOperationProgress = NULL;
    }
#endif
  }
  catch (Exception &E)
  {
    if (OperationProgress.Cancel != csCancel)
    {
      CommandError(&E, LoadStr(TOREMOTE_COPY_ERROR));
    }
    OnceDoneOperation = odoIdle;
  }

  if (OnceDoneOperation != odoIdle)
  {
    CloseOnCompletion(OnceDoneOperation);
  }

  return Result;
}
//---------------------------------------------------------------------------
bool /* __fastcall */ TTerminal::CopyToLocal(TStrings * FilesToCopy,
  const UnicodeString TargetDir, const TCopyParamType * CopyParam, int Params)
{
  assert(FFileSystem);

  // see scp.c: sink(), tolocal()

  bool Result = false;
  bool OwnsFileList = (FilesToCopy == NULL);
  TOnceDoneOperation OnceDoneOperation = odoIdle;

  // try
  {
    BOOST_SCOPE_EXIT( (&OwnsFileList) (&FilesToCopy) )
    {
      if (OwnsFileList) { delete FilesToCopy; }
    } BOOST_SCOPE_EXIT_END
    if (OwnsFileList)
    {
      FilesToCopy = new TStringList();
      FilesToCopy->Assign(GetFiles()->GetSelectedFiles());
    }

    BeginTransaction();
    // try
    {
      BOOST_SCOPE_EXIT( (&Self) )
      {
        // If session is still active (no fatal error) we reload directory
        // by calling EndTransaction
        Self->EndTransaction();
      } BOOST_SCOPE_EXIT_END
      __int64 TotalSize = 0;
      bool TotalSizeKnown = false;
      TFileOperationProgressType OperationProgress(boost::bind(&TTerminal::DoProgress, this, _1, _2), boost::bind(&TTerminal::DoFinished, this, _1, _2, _3, _4, _5, _6));

      if (CopyParam->GetCalculateSize())
      {
        SetExceptionOnFail(true);
        // try
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
#ifndef _MSC_VER
        __finally
        {
          ExceptionOnFail = false;
        }
#endif
      }
      OperationProgress.Start(((Params & cpDelete) != 0 ? foMove : foCopy), osRemote,
        FilesToCopy->GetCount(), (Params & cpTemporary) > 0, TargetDir, CopyParam->GetCPSLimit());

      FOperationProgress = &OperationProgress;
      // try
      {
        BOOST_SCOPE_EXIT ( (&Self) (&OperationProgress) )
        {
          Self->FOperationProgress = NULL;
          OperationProgress.Stop();
        } BOOST_SCOPE_EXIT_END
        if (TotalSizeKnown)
        {
          OperationProgress.SetTotalSize(TotalSize);
        }

        try
        {
          // try
          {
            BOOST_SCOPE_EXIT ( (&Self) )
            {
              if (Self->GetActive())
              {
                Self->ReactOnCommand(fsCopyToLocal);
              }
            } BOOST_SCOPE_EXIT_END
            FFileSystem->CopyToLocal(FilesToCopy, TargetDir, CopyParam, Params,
              &OperationProgress, OnceDoneOperation);
          }
#ifndef _MSC_VER
          __finally
          {
            if (Active)
            {
              ReactOnCommand(fsCopyToLocal);
            }
          }
#endif
        }
        catch (Exception &E)
        {
          if (OperationProgress.Cancel != csCancel)
          {
            CommandError(&E, LoadStr(TOLOCAL_COPY_ERROR));
          }
          OnceDoneOperation = odoIdle;
        }

        if (OperationProgress.Cancel == csContinue)
        {
          Result = true;
        }
      }
#ifndef _MSC_VER
      __finally
      {
        FOperationProgress = NULL;
        OperationProgress.Stop();
      }
#endif
    }
#ifndef _MSC_VER
    __finally
    {
      // If session is still active (no fatal error) we reload directory
      // by calling EndTransaction
      EndTransaction();
    }
#endif

  }
#ifndef _MSC_VER
  __finally
  {
    if (OwnsFileList) delete FilesToCopy;
  }
#endif

  if (OnceDoneOperation != odoIdle)
  {
    CloseOnCompletion(OnceDoneOperation);
  }

  return Result;
}
//---------------------------------------------------------------------------
/* __fastcall */ TSecondaryTerminal::TSecondaryTerminal(TTerminal * MainTerminal) :
  TTerminal(),
  FMainTerminal(MainTerminal), FMasterPasswordTried(false),
  FMasterTunnelPasswordTried(false)
{
}

void /* __fastcall */ TSecondaryTerminal::Init(
  TSessionData * ASessionData, TConfiguration * Configuration, const UnicodeString & Name)
{
  TTerminal::Init(ASessionData, Configuration);
  GetLog()->SetParent(FMainTerminal->GetLog());
  GetLog()->SetName(Name);
  GetActionLog()->SetEnabled(false);
  GetSessionData()->NonPersistant();
  assert(FMainTerminal != NULL);
  if (!FMainTerminal->GetUserName().IsEmpty())
  {
    GetSessionData()->SetUserName(FMainTerminal->GetUserName());
  }
}
//---------------------------------------------------------------------------
void /* __fastcall */ TSecondaryTerminal::DirectoryLoaded(TRemoteFileList * FileList)
{
  FMainTerminal->DirectoryLoaded(FileList);
  assert(FileList != NULL);
}
//---------------------------------------------------------------------------
void /* __fastcall */ TSecondaryTerminal::DirectoryModified(const UnicodeString Path,
  bool SubDirs)
{
  // clear cache of main terminal
  FMainTerminal->DirectoryModified(Path, SubDirs);
}
//---------------------------------------------------------------------------
bool /* __fastcall */ TSecondaryTerminal::DoPromptUser(TSessionData * Data,
  TPromptKind Kind, UnicodeString Name, UnicodeString Instructions, TStrings * Prompts,
  TStrings * Results)
{
  bool AResult = false;

  if ((Prompts->GetCount() == 1) && !((void *)(Prompts->GetObjects(0) != NULL)) &&
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
      UnicodeString Password;
      if (FTunnelOpening)
      {
        Password = FMainTerminal->GetTunnelPassword();
      }
      else
      {
        Password = FMainTerminal->GetPassword();
      }
      Results->PutString(0, Password);
      if (!Results->GetStrings(0).IsEmpty())
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
/* __fastcall */ TTerminalList::TTerminalList(TConfiguration * AConfiguration) :
  TObjectList()
{
  assert(AConfiguration);
  FConfiguration = AConfiguration;
}
//---------------------------------------------------------------------------
/* __fastcall */ TTerminalList::~TTerminalList()
{
  assert(GetCount() == 0);
}
//---------------------------------------------------------------------------
TTerminal * /* __fastcall */ TTerminalList::CreateTerminal(TSessionData * Data)
{
  TTerminal * Result = new TTerminal();
  Result->Init(Data, FConfiguration);
  return Result;
}
//---------------------------------------------------------------------------
TTerminal * /* __fastcall */ TTerminalList::NewTerminal(TSessionData * Data)
{
  TTerminal * Terminal = CreateTerminal(Data);
  Add(Terminal);
  return Terminal;
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminalList::FreeTerminal(TTerminal * Terminal)
{
  assert(IndexOf(Terminal) >= 0);
  Remove(Terminal);
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminalList::FreeAndNullTerminal(TTerminal * & Terminal)
{
  TTerminal * T = Terminal;
  Terminal = NULL;
  FreeTerminal(T);
}
//---------------------------------------------------------------------------
TTerminal * /* __fastcall */ TTerminalList::GetTerminal(size_t Index)
{
  return dynamic_cast<TTerminal *>(GetItem(Index));
}
//---------------------------------------------------------------------------
int /* __fastcall */ TTerminalList::GetActiveCount()
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
void /* __fastcall */ TTerminalList::Idle()
{
  TTerminal * Terminal;
  for (size_t i = 0; i < GetCount(); i++)
  {
    Terminal = GetTerminal(i);
    if (Terminal->GetStatus() == ssOpened)
    {
      Terminal->Idle();
    }
  }
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminalList::RecryptPasswords()
{
  for (int Index = 0; Index < GetCount(); Index++)
  {
    GetTerminal(Index)->RecryptPasswords();
  }
}
