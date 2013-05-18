//------------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include "Terminal.h"

#include <SysUtils.hpp>
#include <FileCtrl.hpp>

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
#include "WebDAVFileSystem.h"
#include "TextsCore.h"
#include "HelpCore.h"
#include "CoreMain.h"
#include "Queue.h"

#ifndef AUTO_WINSOCK
#include <winsock2.h>
#endif
//------------------------------------------------------------------------------
#pragma package(smart_init)
//------------------------------------------------------------------------------
#define COMMAND_ERROR_ARI(MESSAGE, REPEAT) \
  { \
    uintptr_t Result = CommandError(&E, MESSAGE, qaRetry | qaSkip | qaAbort); \
    switch (Result) \
    { \
      case qaRetry: { REPEAT; } break; \
      case qaAbort: Abort(); \
    } \
  }
//------------------------------------------------------------------------------
// Note that the action may already be canceled when RollbackAction is called
#define COMMAND_ERROR_ARI_ACTION(MESSAGE, REPEAT, ACTION) \
  { \
    uintptr_t Result; \
    try \
    { \
      Result = CommandError(&E, MESSAGE, qaRetry | qaSkip | qaAbort); \
    } \
    catch(Exception & E2) \
    { \
      RollbackAction(ACTION, NULL, &E2); \
      throw; \
    } \
    switch (Result) \
    { \
      case qaRetry: ACTION.Cancel(); { REPEAT; } break; \
      case qaAbort: RollbackAction(ACTION, NULL, &E); Abort(); \
      case qaSkip:  ACTION.Cancel(); break; \
      default: assert(false); \
    } \
  }

#define FILE_OPERATION_LOOP_EX(ALLOW_SKIP, MESSAGE, OPERATION) \
  FILE_OPERATION_LOOP_CUSTOM(this, ALLOW_SKIP, MESSAGE, OPERATION)
//------------------------------------------------------------------------------
struct TMoveFileParams : public TObject
{
  UnicodeString Target;
  UnicodeString FileMask;
};
//------------------------------------------------------------------------------
struct TFilesFindParams : public TObject
{
  TFilesFindParams() :
    OnFileFound(NULL),
    OnFindingFile(NULL),
    Cancel(false)
  {
  }
  TFileMasks FileMask;
  TFileFoundEvent OnFileFound;
  TFindingFileEvent OnFindingFile;
  bool Cancel;
};
//------------------------------------------------------------------------------
TCalculateSizeStats::TCalculateSizeStats() :
  Files(0),
  Directories(0),
  SymLinks(0)
{
}
//------------------------------------------------------------------------------
TSynchronizeOptions::TSynchronizeOptions() :
  Filter(0)
{
}
//------------------------------------------------------------------------------
TSynchronizeOptions::~TSynchronizeOptions()
{
  delete Filter;
}
//------------------------------------------------------------------------------
bool TSynchronizeOptions::MatchesFilter(const UnicodeString & FileName)
{
  bool Result = false;
  if (Filter == NULL)
  {
    Result = true;
  }
  else
  {
    intptr_t FoundIndex = 0;
    Result = Filter->Find(FileName, FoundIndex);
  }
  return Result;
}
//------------------------------------------------------------------------------
TSpaceAvailable::TSpaceAvailable() :
  BytesOnDevice(0),
  UnusedBytesOnDevice(0),
  BytesAvailableToUser(0),
  UnusedBytesAvailableToUser(0),
  BytesPerAllocationUnit(0)
{
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
TSynchronizeChecklist::TItem::TItem() :
  Action(saNone), IsDirectory(false), ImageIndex(-1), Checked(true), RemoteFile(NULL)
{
  Local.ModificationFmt = mfFull;
  Local.Modification = 0;
  Local.Size = 0;
  Remote.ModificationFmt = mfFull;
  Remote.Modification = 0;
  Remote.Size = 0;
}
//------------------------------------------------------------------------------
TSynchronizeChecklist::TItem::~TItem()
{
  delete RemoteFile;
}
//------------------------------------------------------------------------------
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
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
TSynchronizeChecklist::TSynchronizeChecklist() :
  FList(new TList())
{
}
//------------------------------------------------------------------------------
TSynchronizeChecklist::~TSynchronizeChecklist()
{
  for (intptr_t Index = 0; Index < FList->GetCount(); ++Index)
  {
    delete static_cast<TItem *>(static_cast<void *>(FList->GetItem(Index)));
  }
  delete FList;
}
//------------------------------------------------------------------------------
void TSynchronizeChecklist::Add(TItem * Item)
{
  FList->Add(Item);
}
//------------------------------------------------------------------------------
intptr_t TSynchronizeChecklist::Compare(const void * AItem1, const void * AItem2)
{
  const TItem * Item1 = static_cast<const TItem *>(AItem1);
  const TItem * Item2 = static_cast<const TItem *>(AItem2);

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
//------------------------------------------------------------------------------
void TSynchronizeChecklist::Sort()
{
  FList->Sort(Compare);
}
//------------------------------------------------------------------------------
intptr_t TSynchronizeChecklist::GetCount() const
{
  return FList->GetCount();
}
//------------------------------------------------------------------------------
const TSynchronizeChecklist::TItem * TSynchronizeChecklist::GetItem(intptr_t Index) const
{
  return static_cast<TItem *>(FList->GetItem(Index));
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
class TTunnelThread : public TSimpleThread
{
public:
  explicit TTunnelThread(TSecureShell * SecureShell);
  virtual ~TTunnelThread();

  virtual void Init();
  virtual void Terminate();

protected:
  virtual void Execute();

private:
  TSecureShell * FSecureShell;
  bool FTerminated;
};
//------------------------------------------------------------------------------
TTunnelThread::TTunnelThread(TSecureShell * SecureShell) :
  TSimpleThread(),
  FSecureShell(SecureShell),
  FTerminated(false)
{
}
//------------------------------------------------------------------------------
void TTunnelThread::Init()
{
  TSimpleThread::Init();
  Start();
}
//------------------------------------------------------------------------------
TTunnelThread::~TTunnelThread()
{
  // close before the class's virtual functions (Terminate particularly) are lost
  Close();
}
//------------------------------------------------------------------------------
void TTunnelThread::Terminate()
{
  FTerminated = true;
}
//------------------------------------------------------------------------------
void TTunnelThread::Execute()
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
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
class TTunnelUI : public TSessionUI
{
public:
  explicit TTunnelUI(TTerminal * Terminal);
  virtual ~TTunnelUI() {}
  virtual void Information(const UnicodeString & Str, bool Status);
  virtual uintptr_t QueryUser(const UnicodeString & Query,
    TStrings * MoreMessages, uintptr_t Answers, const TQueryParams * Params,
    TQueryType QueryType);
  virtual uintptr_t QueryUserException(const UnicodeString & Query,
    Exception * E, uintptr_t Answers, const TQueryParams * Params,
    TQueryType QueryType);
  virtual bool PromptUser(TSessionData * Data, TPromptKind Kind,
    const UnicodeString & Name, const UnicodeString & Instructions, TStrings * Prompts,
    TStrings * Results);
  virtual void DisplayBanner(const UnicodeString & Banner);
  virtual void FatalError(Exception * E, const UnicodeString & Msg);
  virtual void HandleExtendedException(Exception * E);
  virtual void Closed();

private:
  TTerminal * FTerminal;
  unsigned int FTerminalThread;
private:
  NB_DISABLE_COPY(TTunnelUI)
};
//------------------------------------------------------------------------------
TTunnelUI::TTunnelUI(TTerminal * Terminal)
{
  FTerminal = Terminal;
  FTerminalThread = GetCurrentThreadId();
}
//------------------------------------------------------------------------------
void TTunnelUI::Information(const UnicodeString & Str, bool Status)
{
  if (GetCurrentThreadId() == FTerminalThread)
  {
    FTerminal->Information(Str, Status);
  }
}
//------------------------------------------------------------------------------
uintptr_t TTunnelUI::QueryUser(const UnicodeString & Query,
  TStrings * MoreMessages, uintptr_t Answers, const TQueryParams * Params,
  TQueryType QueryType)
{
  uintptr_t Result;
  if (GetCurrentThreadId() == FTerminalThread)
  {
    Result = FTerminal->QueryUser(Query, MoreMessages, Answers, Params, QueryType);
  }
  else
  {
    Result = AbortAnswer(static_cast<intptr_t>(Answers));
  }
  return Result;
}
//------------------------------------------------------------------------------
uintptr_t TTunnelUI::QueryUserException(const UnicodeString & Query,
  Exception * E, uintptr_t Answers, const TQueryParams * Params,
  TQueryType QueryType)
{
  uintptr_t Result;
  if (GetCurrentThreadId() == FTerminalThread)
  {
    Result = FTerminal->QueryUserException(Query, E, Answers, Params, QueryType);
  }
  else
  {
    Result = AbortAnswer(static_cast<intptr_t>(Answers));
  }
  return Result;
}
//------------------------------------------------------------------------------
bool TTunnelUI::PromptUser(TSessionData * Data, TPromptKind Kind,
  const UnicodeString & Name, const UnicodeString & Instructions, TStrings * Prompts, TStrings * Results)
{
  bool Result;
  if (GetCurrentThreadId() == FTerminalThread)
  {
    UnicodeString Instructions2 = Instructions;
    if (IsAuthenticationPrompt(Kind))
    {
      Instructions2 = LoadStr(TUNNEL_INSTRUCTION) +
        (Instructions.IsEmpty() ? L"" : L"\n") +
        Instructions;
    }

    Result = FTerminal->PromptUser(Data, Kind, Name, Instructions2, Prompts, Results);
  }
  else
  {
    Result = false;
  }
  return Result;
}
//------------------------------------------------------------------------------
void TTunnelUI::DisplayBanner(const UnicodeString & Banner)
{
  if (GetCurrentThreadId() == FTerminalThread)
  {
    FTerminal->DisplayBanner(Banner);
  }
}
//------------------------------------------------------------------------------
void TTunnelUI::FatalError(Exception * E, const UnicodeString & Msg)
{
  throw ESshFatal(E, Msg);
}
//------------------------------------------------------------------------------
void TTunnelUI::HandleExtendedException(Exception * E)
{
  if (GetCurrentThreadId() == FTerminalThread)
  {
    FTerminal->HandleExtendedException(E);
  }
}
//------------------------------------------------------------------------------
void TTunnelUI::Closed()
{
  // noop
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
class TCallbackGuard : public TObject
{
public:
  inline TCallbackGuard(TTerminal * FTerminal);
  inline ~TCallbackGuard();

  void FatalError(Exception * E, const UnicodeString & Msg);
  inline void Verify();
  void Dismiss();

private:
  ExtException * FFatalError;
  TTerminal * FTerminal;
  bool FGuarding;
};
//------------------------------------------------------------------------------
TCallbackGuard::TCallbackGuard(TTerminal * Terminal) :
  FFatalError(NULL),
  FTerminal(Terminal),
  FGuarding(FTerminal->FCallbackGuard == NULL)
{
  if (FGuarding)
  {
    FTerminal->FCallbackGuard = this;
  }
}
//------------------------------------------------------------------------------
TCallbackGuard::~TCallbackGuard()
{
  if (FGuarding)
  {
    assert((FTerminal->FCallbackGuard == this) || (FTerminal->FCallbackGuard == NULL));
    FTerminal->FCallbackGuard = NULL;
  }

  delete FFatalError;
}
//------------------------------------------------------------------------------
void TCallbackGuard::FatalError(Exception * E, const UnicodeString & Msg)
{
  assert(FGuarding);

  // make sure we do not bother about getting back the silent abort exception
  // we issued ourselves. this may happen when there is an exception handler
  // that converts any exception to fatal one (such as in TTerminal::Open).
  if (dynamic_cast<ECallbackGuardAbort *>(E) == NULL)
  {
    delete FFatalError;
    FFatalError = new ExtException(E, Msg);
  }

  // silently abort what we are doing.
  // non-silent exception would be caught probably by default application
  // exception handler, which may not do an appropriate action
  // (particularly it will not resume broken transfer).
  throw ECallbackGuardAbort();
}
//------------------------------------------------------------------------------
void TCallbackGuard::Dismiss()
{
  assert(FFatalError == NULL);
  FGuarding = false;
}
//------------------------------------------------------------------------------
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
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
TTerminal::TTerminal() :
  TObject(),
  TSessionUI()
{
}
//------------------------------------------------------------------------------
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
    FConfiguration->SaveDirectoryChangesCache(GetSessionData()->GetSessionKey(),
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
//------------------------------------------------------------------------------
void TTerminal::Init(TSessionData * SessionData, TConfiguration * Configuration)
{
  FConfiguration = Configuration;
  FSessionData = new TSessionData(L"");
  FSessionData->Assign(SessionData);
  FLog = new TSessionLog(this, FSessionData, FConfiguration);
  FActionLog = new TActionLog(this, FSessionData, FConfiguration);
  FFiles = new TRemoteDirectory(this);
  FExceptionOnFail = 0;
  FInTransaction = 0;
  FReadCurrentDirectoryPending = false;
  FReadDirectoryPending = false;
  FUsersGroupsLookedup = False;
  FTunnelLocalPortNumber = 0;
  FFileSystem = NULL;
  FSecureShell = NULL;
  FOnProgress = NULL;
  FOnFinished = NULL;
  FOnDeleteLocalFile = NULL;
  FOnCreateLocalFile = NULL;
  FOnGetLocalFileAttributes = NULL;
  FOnSetLocalFileAttributes = NULL;
  FOnMoveLocalFile = NULL;
  FOnRemoveLocalDirectory = NULL;
  FOnCreateLocalDirectory = NULL;
  FOnReadDirectoryProgress = NULL;
  FOnQueryUser = NULL;
  FOnPromptUser = NULL;
  FOnDisplayBanner = NULL;
  FOnShowExtendedException = NULL;
  FOnInformation = NULL;
  FOnClose = NULL;
  FOnFindingFile = NULL;

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
  FEnableSecureShellUsage = false;
  FSuspendTransaction = false;
  FOperationProgress = NULL;
  FClosedOnCompletion = NULL;
  FTunnel = NULL;
}
//------------------------------------------------------------------------------
void TTerminal::Idle()
{
  // once we disconnect, do nothing, until reconnect handler
  // "receives the information"
  if (GetActive())
  {
    if (FConfiguration->GetActualLogProtocol() >= 1)
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
RawByteString TTerminal::EncryptPassword(const UnicodeString & Password)
{
  return FConfiguration->EncryptPassword(Password, GetSessionData()->GetSessionName());
}
//---------------------------------------------------------------------
UnicodeString TTerminal::DecryptPassword(const RawByteString & Password)
{
  UnicodeString Result;
  try
  {
    Result = FConfiguration->DecryptPassword(Password, GetSessionData()->GetSessionName());
  }
  catch(EAbort &)
  {
    // silently ignore aborted prompts for master password and return empty password
  }
  return Result;
}
//------------------------------------------------------------------------------
void TTerminal::RecryptPasswords()
{
  FSessionData->RecryptPasswords();
  FPassword = EncryptPassword(DecryptPassword(FPassword));
  FTunnelPassword = EncryptPassword(DecryptPassword(FTunnelPassword));
}
//------------------------------------------------------------------------------
bool TTerminal::IsAbsolutePath(const UnicodeString & Path)
{
  return !Path.IsEmpty() && Path[1] == L'/';
}
//------------------------------------------------------------------------------
UnicodeString TTerminal::ExpandFileName(const UnicodeString & Path,
  const UnicodeString & BasePath)
{
  UnicodeString Result = UnixExcludeTrailingBackslash(Path);
  if (!IsAbsolutePath(Result) && !BasePath.IsEmpty())
  {
    // TODO: Handle more complicated cases like "../../xxx"
    if (Result == PARENTDIRECTORY)
    {
      Result = UnixExcludeTrailingBackslash(UnixExtractFilePath(
        UnixExcludeTrailingBackslash(BasePath)));
    }
    else
    {
      Result = UnixIncludeTrailingBackslash(BasePath) + Path;
    }
  }
  return Result;
}
//------------------------------------------------------------------------------
bool TTerminal::GetActive()
{
  return (FFileSystem != NULL) && FFileSystem->GetActive();
}
//------------------------------------------------------------------------------
void TTerminal::Close()
{
  FFileSystem->Close();

  if (GetCommandSessionOpened())
  {
    FCommandSession->Close();
  }
}
//------------------------------------------------------------------------------
void TTerminal::ResetConnection()
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
//------------------------------------------------------------------------------
void TTerminal::Open()
{
  ReflectSettings();
  bool Reopen = false;
  do
  {
    Reopen = false;
    DoInformation(L"", true, 1);
    try
    {
      TRY_FINALLY (
      {
        try
        {
          ResetConnection();
          FStatus = ssOpening;

          TRY_FINALLY (
          {
            if (FFileSystem == NULL)
            {
              GetLog()->AddSystemInfo();
              DoInitializeLog();
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
              if ((GetSessionData()->GetFSProtocol() == fsFTP) && (GetSessionData()->GetFtps() == ftpsNone))
              {
/*#ifdef NO_FILEZILLA
                LogEvent(L"FTP protocol is not supported by this build.");
                FatalError(NULL, LoadStr(FTP_UNSUPPORTED));
#else*/
                FFSProtocol = cfsFTP;
                FFileSystem = new TFTPFileSystem(this);
                FFileSystem->Init(NULL);
                FFileSystem->Open();
                GetLog()->AddSeparator();
                LogEvent(L"Using FTP protocol.");
// #endif
              }
              else if ((GetSessionData()->GetFSProtocol() == fsFTP) && (GetSessionData()->GetFtps() != ftpsNone))
              {
/*#if defined(NO_FILEZILLA) && defined(MPEXT_NO_SSLDLL)
                LogEvent(L"FTPS protocol is not supported by this build.");
                FatalError(NULL, LoadStr(FTPS_UNSUPPORTED));
#else*/
                FFSProtocol = cfsFTPS;
                FFileSystem = new TFTPFileSystem(this);
                FFileSystem->Init(NULL);
                FFileSystem->Open();
                GetLog()->AddSeparator();
                LogEvent(L"Using FTPS protocol.");
// #endif
              }
              else if (GetSessionData()->GetFSProtocol() == fsWebDAV)
              {
                FFSProtocol = cfsWebDAV;
                FFileSystem = new TWebDAVFileSystem(this);
                FFileSystem->Init(NULL);
                FFileSystem->Open();
                GetLog()->AddSeparator();
                LogEvent(L"Using WebDAV protocol.");
              }
              else
              {
                assert(FSecureShell == NULL);
                TRY_FINALLY (
                {
                  FSecureShell = new TSecureShell(this, FSessionData, GetLog(), FConfiguration);
                  try
                  {
                    if (FEnableSecureShellUsage)
                    {
                      // only on the first connect,
                      // this is not ideal as it may prevent usage from being collected,
                      // e.g. when connection fails on the first try
                      FSecureShell->EnableUsage();
                      FEnableSecureShellUsage = false;
                    }
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
                      assert(E.Message == LoadStr(UNEXPECTED_CLOSE_ERROR));
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
                    FFileSystem->Init(FSecureShell);
                    FSecureShell = NULL; // ownership passed
                    LogEvent(L"Using SCP protocol.");
                  }
                  else
                  {
                    FFSProtocol = cfsSFTP;
                    FFileSystem = new TSFTPFileSystem(this);
                    FFileSystem->Init(FSecureShell);
                    FSecureShell = NULL; // ownership passed
                    LogEvent(L"Using SFTP protocol.");
                  }
                }
                ,
                {
                  delete FSecureShell;
                  FSecureShell = NULL;
                }
                );
              }
            }
            else
            {
              FFileSystem->Open();
            }
          }
          ,
          {
            if (FSessionData->GetTunnel())
            {
              FSessionData->RollbackTunnel();
            }
          }
          );

          if (GetSessionData()->GetCacheDirectoryChanges())
          {
            assert(FDirectoryChangesCache == NULL);
            FDirectoryChangesCache = new TRemoteDirectoryChangesCache(
              FConfiguration->GetCacheDirectoryChangesMaxSize());
            if (GetSessionData()->GetPreserveDirectoryChanges())
            {
              FConfiguration->LoadDirectoryChangesCache(GetSessionData()->GetSessionKey(),
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
      ,
      {
        DoInformation(L"", true, 0);
      }
      );
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
      LogEvent(FORMAT(L"Got error: \"%s\"", E.Message.c_str()));
      // any exception while opening session is fatal
      FatalError(&E, L"");
    }
  }
  while (Reopen);
  FSessionData->SetNumberOfRetries(0);
}
//------------------------------------------------------------------------------
bool TTerminal::IsListenerFree(uintptr_t PortNumber)
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
    Result = (bind(Socket, reinterpret_cast<sockaddr *>(&Address), sizeof(Address)) == 0);
    closesocket(Socket);
  }
  return Result;
}
//------------------------------------------------------------------------------
void TTerminal::OpenTunnel()
{
  assert(FTunnelData == NULL);

  FTunnelLocalPortNumber = FSessionData->GetTunnelLocalPortNumber();
  if (FTunnelLocalPortNumber == 0)
  {
    FTunnelLocalPortNumber = FConfiguration->GetTunnelLocalPortNumberLow();
    while (!IsListenerFree(FTunnelLocalPortNumber))
    {
      FTunnelLocalPortNumber++;
      if (FTunnelLocalPortNumber > FConfiguration->GetTunnelLocalPortNumberHigh())
      {
        FTunnelLocalPortNumber = 0;
        FatalError(NULL, FMTLOAD(TUNNEL_NO_FREE_PORT,
          FConfiguration->GetTunnelLocalPortNumberLow(), FConfiguration->GetTunnelLocalPortNumberHigh()));
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
    FTunnelData->SetHostKey(FSessionData->GetTunnelHostKey());
    FTunnelData->SetProxyMethod(FSessionData->GetProxyMethod());
    FTunnelData->SetProxyHost(FSessionData->GetProxyHost());
    FTunnelData->SetProxyPort(FSessionData->GetProxyPort());
    FTunnelData->SetProxyUsername(FSessionData->GetProxyUsername());
    FTunnelData->SetProxyPassword(FSessionData->GetProxyPassword());
    FTunnelData->SetProxyTelnetCommand(FSessionData->GetProxyTelnetCommand());
    FTunnelData->SetProxyLocalCommand(FSessionData->GetProxyLocalCommand());
    FTunnelData->SetProxyDNS(FSessionData->GetProxyDNS());
    FTunnelData->SetProxyLocalhost(FSessionData->GetProxyLocalhost());

    FTunnelLog = new TSessionLog(this, FTunnelData, FConfiguration);
    FTunnelLog->SetParent(FLog);
    FTunnelLog->SetName(L"Tunnel");
    FTunnelLog->ReflectSettings();
    FTunnelUI = new TTunnelUI(this);
    FTunnel = new TSecureShell(FTunnelUI, FTunnelData, FTunnelLog, FConfiguration);

    FTunnelOpening = true;
    TRY_FINALLY (
    {
      FTunnel->Open();
    }
    ,
    {
      FTunnelOpening = false;
    }
    );
    FTunnelThread = new TTunnelThread(FTunnel);
    FTunnelThread->Init();
  }
  catch(...)
  {
    CloseTunnel();
    throw;
  }
}
//------------------------------------------------------------------------------
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
//------------------------------------------------------------------------------
void TTerminal::Closed()
{
  if (FTunnel != NULL)
  {
     CloseTunnel();
  }

  if (GetOnClose())
  {
    TCallbackGuard Guard(this);
    GetOnClose()(this);
    Guard.Verify();
  }

  FStatus = ssClosed;
}
//------------------------------------------------------------------------------
void TTerminal::Reopen(intptr_t Params)
{
  TFSProtocol OrigFSProtocol = GetSessionData()->GetFSProtocol();
  UnicodeString PrevRemoteDirectory = GetSessionData()->GetRemoteDirectory();
  bool PrevReadCurrentDirectoryPending = FReadCurrentDirectoryPending;
  bool PrevReadDirectoryPending = FReadDirectoryPending;
  assert(!FSuspendTransaction);
  bool PrevAutoReadDirectory = FAutoReadDirectory;
  // here used to be a check for FExceptionOnFail being 0
  // but it can happen, e.g. when we are downloading file to execute it.
  // however I'm not sure why we mind having exception-on-fail enabled here
  Integer PrevExceptionOnFail = FExceptionOnFail;
  TRY_FINALLY (
  {
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
  ,
  {
    GetSessionData()->SetRemoteDirectory(PrevRemoteDirectory);
    GetSessionData()->SetFSProtocol(OrigFSProtocol);
    FAutoReadDirectory = PrevAutoReadDirectory;
    FReadCurrentDirectoryPending = PrevReadCurrentDirectoryPending;
    FReadDirectoryPending = PrevReadDirectoryPending;
    FSuspendTransaction = false;
    FExceptionOnFail = PrevExceptionOnFail;
  }
  );
}
//------------------------------------------------------------------------------
bool TTerminal::PromptUser(TSessionData * Data, TPromptKind Kind,
  const UnicodeString & Name, const UnicodeString & Instructions,
  const UnicodeString & Prompt,
  bool Echo, int MaxLen, UnicodeString & AResult)
{
  bool Result;
  std::auto_ptr<TStrings> Prompts(new TStringList());
  std::auto_ptr<TStrings> Results(new TStringList());
  Prompts->AddObject(Prompt, reinterpret_cast<TObject *>(static_cast<size_t>(FLAGMASK(Echo, pupEcho))));
  Results->AddObject(AResult, reinterpret_cast<TObject *>(MaxLen));
  Result = PromptUser(Data, Kind, Name, Instructions, Prompts.get(), Results.get());
  AResult = Results->GetString(0);
  return AResult;
}
//------------------------------------------------------------------------------
bool TTerminal::PromptUser(TSessionData * Data, TPromptKind Kind,
  const UnicodeString & Name, const UnicodeString & Instructions, TStrings * Prompts, TStrings * Results)
{
  // If PromptUser is overridden in descendant class, the overridden version
  // is not called when accessed via TSessionIU interface.
  // So this is workaround.
  return DoPromptUser(Data, Kind, Name, Instructions, Prompts, Results);
}
//------------------------------------------------------------------------------
bool TTerminal::DoPromptUser(TSessionData * /*Data*/, TPromptKind Kind,
  const UnicodeString & Name, const UnicodeString & Instructions, TStrings * Prompts, TStrings * Results)
{
  bool AResult = false;

  bool PasswordPrompt =
    (Prompts->GetCount() == 1) && FLAGCLEAR(int(Prompts->GetObject(0)), pupEcho) &&
    ((Kind == pkPassword) || (Kind == pkPassphrase) || (Kind == pkKeybInteractive) ||
     (Kind == pkTIS) || (Kind == pkCryptoCard));
  if (PasswordPrompt && !GetConfiguration()->GetRememberPassword())
  {
    Prompts->SetObject(0, (TObject*)(int(Prompts->GetObject(0)) | pupRemember));
  }

  if (GetOnPromptUser() != NULL)
  {
    TCallbackGuard Guard(this);
    GetOnPromptUser()(this, Kind, Name, Instructions, Prompts, Results, AResult, NULL);
    Guard.Verify();
  }

  if (AResult && PasswordPrompt &&
      (GetConfiguration()->GetRememberPassword() || FLAGSET(int(Prompts->GetObject(0)), pupRemember)))
  {
    RawByteString EncryptedPassword = EncryptPassword(Results->GetString(0));
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
//------------------------------------------------------------------------------
uintptr_t TTerminal::QueryUser(const UnicodeString & Query,
  TStrings * MoreMessages, uintptr_t Answers, const TQueryParams * Params,
  TQueryType QueryType)
{
  LogEvent(FORMAT(L"Asking user:\n%s (%s)", Query.c_str(), MoreMessages ? MoreMessages->GetCommaText().c_str() : L""));
  uintptr_t Answer = AbortAnswer(Answers);
  if (FOnQueryUser)
  {
    TCallbackGuard Guard(this);
    FOnQueryUser(this, Query, MoreMessages, Answers, Params, Answer, QueryType, NULL);
    Guard.Verify();
  }
  return Answer;
}
//------------------------------------------------------------------------------
uintptr_t TTerminal::QueryUserException(const UnicodeString & Query,
  Exception * E, uintptr_t Answers, const TQueryParams * Params,
  TQueryType QueryType)
{
  intptr_t Result = 0;
  TStrings * MoreMessages = new TStringList();
  TRY_FINALLY (
  {
    if (E != NULL)
    {
      if (!E->Message.IsEmpty() && !Query.IsEmpty())
      {
        MoreMessages->Add(E->Message);
      }

      ExtException * EE = dynamic_cast<ExtException*>(E);
      if ((EE != NULL) && (EE->GetMoreMessages() != NULL))
      {
        MoreMessages->AddStrings(EE->GetMoreMessages());
      }
    }
    Result = QueryUser(!Query.IsEmpty() ? Query : UnicodeString(E ? E->Message : L""),
      MoreMessages->GetCount() ? MoreMessages : NULL,
      Answers, Params, QueryType);
  }
  ,
  {
    delete MoreMessages;
  }
  );
  return Result;
}
//------------------------------------------------------------------------------
void TTerminal::DisplayBanner(const UnicodeString & Banner)
{
  if (GetOnDisplayBanner() != NULL)
  {
    if (FConfiguration->GetForceBanners() ||
        FConfiguration->ShowBanner(GetSessionData()->GetSessionKey(), Banner))
    {
      bool NeverShowAgain = false;
      int Options =
        FLAGMASK(FConfiguration->GetForceBanners(), boDisableNeverShowAgain);
      TCallbackGuard Guard(this);
      GetOnDisplayBanner()(this, GetSessionData()->GetSessionName(), Banner,
        NeverShowAgain, Options);
      Guard.Verify();
      if (!FConfiguration->GetForceBanners() && NeverShowAgain)
      {
        FConfiguration->NeverShowBanner(GetSessionData()->GetSessionKey(), Banner);
      }
    }
  }
}
//------------------------------------------------------------------------------
void TTerminal::HandleExtendedException(Exception * E)
{
  GetLog()->AddException(E);
  if (GetOnShowExtendedException() != NULL)
  {
    TCallbackGuard Guard(this);
    // the event handler may destroy 'this' ...
    GetOnShowExtendedException()(this, E, NULL);
    // .. hence guard is dismissed from destructor, to make following call no-op
    Guard.Verify();
  }
}
//------------------------------------------------------------------------------
void TTerminal::ShowExtendedException(Exception * E)
{
  GetLog()->AddException(E);
  if (GetOnShowExtendedException() != NULL)
  {
    GetOnShowExtendedException()(this, E, NULL);
  }
}
//------------------------------------------------------------------------------
void TTerminal::DoInformation(const UnicodeString & Str, bool Status,
  int Phase)
{
  if (GetOnInformation())
  {
    TCallbackGuard Guard(this);
    GetOnInformation()(this, Str, Status, Phase);
    Guard.Verify();
  }
}
//------------------------------------------------------------------------------
void TTerminal::Information(const UnicodeString & Str, bool Status)
{
  DoInformation(Str, Status);
}
//------------------------------------------------------------------------------
void TTerminal::DoProgress(TFileOperationProgressType & ProgressData,
  TCancelStatus & Cancel)
{
  if (GetOnProgress() != NULL)
  {
    TCallbackGuard Guard(this);
    GetOnProgress()(ProgressData, Cancel);
    Guard.Verify();
  }
}
//------------------------------------------------------------------------------
void TTerminal::DoFinished(TFileOperation Operation, TOperationSide Side, bool Temp,
  const UnicodeString & FileName, bool Success, TOnceDoneOperation & OnceDoneOperation)
{
  if (GetOnFinished() != NULL)
  {
    TCallbackGuard Guard(this);
    GetOnFinished()(Operation, Side, Temp, FileName, Success, OnceDoneOperation);
    Guard.Verify();
  }
}
//------------------------------------------------------------------------------
bool TTerminal::GetIsCapable(TFSCapability Capability) const
{
  assert(FFileSystem);
  return FFileSystem->IsCapable(Capability);
}
//------------------------------------------------------------------------------
UnicodeString TTerminal::AbsolutePath(const UnicodeString & Path, bool Local)
{
  return FFileSystem->AbsolutePath(Path, Local);
}
//------------------------------------------------------------------------------
void TTerminal::ReactOnCommand(int /*TFSCommand*/ Cmd)
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
  else if (ModifiesFiles && GetAutoReadDirectory() && FConfiguration->GetAutoReadDirectoryAfterOp())
  {
    if (!InTransaction())
    {
      ReadDirectory(true);
    }
    else
    {
      FReadDirectoryPending = true;
    }
  }
}
//------------------------------------------------------------------------------
void TTerminal::TerminalError(const UnicodeString & Msg)
{
  TerminalError(NULL, Msg);
}
//------------------------------------------------------------------------------
void TTerminal::TerminalError(Exception * E, const UnicodeString & Msg)
{
  throw ETerminal(E, Msg);
}
//------------------------------------------------------------------------------
bool TTerminal::DoQueryReopen(Exception * E)
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
    intptr_t NumberOfRetries = FSessionData->GetNumberOfRetries();
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
      Params.Timeout = FConfiguration->GetSessionReopenAuto();
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
//------------------------------------------------------------------------------
bool TTerminal::QueryReopen(Exception * E, intptr_t Params,
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
            ((FConfiguration->GetSessionReopenTimeout() == 0) ||
             ((intptr_t)((double)(Now() - Start) * MSecsPerDay) < FConfiguration->GetSessionReopenTimeout())) &&
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
//------------------------------------------------------------------------------
bool TTerminal::FileOperationLoopQuery(Exception & E,
  TFileOperationProgressType * OperationProgress, const UnicodeString & Message,
  bool AllowSkip, const UnicodeString & SpecialRetry)
{
  bool Result = false;
  GetLog()->AddException(&E);
  uintptr_t Answer;
  bool SkipPossible = AllowSkip && (OperationProgress != NULL);

  if (SkipPossible && OperationProgress->SkipToAll)
  {
    Answer = qaSkip;
  }
  else
  {
    uintptr_t Answers = qaRetry | qaAbort |
      FLAGMASK(SkipPossible, (qaSkip | qaAll)) |
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
      assert(OperationProgress != NULL);
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
    if ((Answer == qaAbort) && (OperationProgress != NULL))
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
//------------------------------------------------------------------------------
int TTerminal::FileOperationLoop(TFileOperationEvent CallBackFunc,
  TFileOperationProgressType * OperationProgress, bool AllowSkip,
  const UnicodeString & Message, void * Param1, void * Param2)
{
  assert(CallBackFunc);
  int Result = 0;
  FILE_OPERATION_LOOP_EX
  (
    AllowSkip, Message,
    Result = CallBackFunc(Param1, Param2);
  );

  return Result;
}
//------------------------------------------------------------------------------
UnicodeString TTerminal::TranslateLockedPath(const UnicodeString & Path, bool Lock)
{
  UnicodeString Result = Path;
  if (GetSessionData()->GetLockInHome() && !Result.IsEmpty() && (Result[1] == L'/'))
  {
    if (Lock)
    {
      if (Result.SubString(1, FLockDirectory.Length()) == FLockDirectory)
      {
        Result.Delete(1, FLockDirectory.Length());
        if (Result.IsEmpty())
        {
          Result = L"/";
        }
      }
    }
    else
    {
      Result = UnixExcludeTrailingBackslash(FLockDirectory + Result);
    }
  }
  return Result;
}
//------------------------------------------------------------------------------
void TTerminal::ClearCaches()
{
  FDirectoryCache->Clear();
  if (FDirectoryChangesCache != NULL)
  {
    FDirectoryChangesCache->Clear();
  }
  if (FCommandSession != NULL)
  {
    FCommandSession->ClearCaches();
  }
}
//------------------------------------------------------------------------------
void TTerminal::ClearCachedFileList(const UnicodeString & Path,
  bool SubDirs)
{
  FDirectoryCache->ClearFileList(Path, SubDirs);
}
//------------------------------------------------------------------------------
void TTerminal::AddCachedFileList(TRemoteFileList * FileList)
{
  FDirectoryCache->AddFileList(FileList);
}
//------------------------------------------------------------------------------
bool TTerminal::DirectoryFileList(const UnicodeString & Path,
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
//------------------------------------------------------------------------------
void TTerminal::SetCurrentDirectory(const UnicodeString & Value)
{
  assert(FFileSystem);
  UnicodeString Value2 = TranslateLockedPath(Value, false);
  if (Value2 != FFileSystem->GetCurrentDirectory())
  {
    ChangeDirectory(Value2);
  }
}
//------------------------------------------------------------------------------
UnicodeString TTerminal::GetCurrentDirectory()
{
  if (FFileSystem != NULL)
  {
    // there's occassional crash when assigning FFileSystem->CurrentDirectory
    // to FCurrentDirectory, splitting the assignment to two statements
    // to locate the crash more closely
    UnicodeString CurrentDirectory = FFileSystem->GetCurrentDirectory();
    if (FCurrentDirectory != CurrentDirectory)
    {
      FCurrentDirectory = CurrentDirectory;
      if (FCurrentDirectory.IsEmpty())
      {
        ReadCurrentDirectory();
      }
    }
  }

  UnicodeString Result = TranslateLockedPath(FCurrentDirectory, true);
  return Result;
}
//------------------------------------------------------------------------------
UnicodeString TTerminal::PeekCurrentDirectory()
{
  if (FFileSystem)
  {
    FCurrentDirectory = FFileSystem->GetCurrentDirectory();
  }

  UnicodeString Result = TranslateLockedPath(FCurrentDirectory, true);
  return Result;
}
//------------------------------------------------------------------------------
const TRemoteTokenList * TTerminal::GetGroups()
{
  assert(FFileSystem);
  LookupUsersGroups();
  return &FGroups;
}
//------------------------------------------------------------------------------
const TRemoteTokenList * TTerminal::GetUsers()
{
  assert(FFileSystem);
  LookupUsersGroups();
  return &FUsers;
}
//------------------------------------------------------------------------------
const TRemoteTokenList * TTerminal::GetMembership()
{
  assert(FFileSystem);
  LookupUsersGroups();
  return &FMembership;
}
//------------------------------------------------------------------------------
UnicodeString TTerminal::GetUserName() const
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
//------------------------------------------------------------------------------
bool TTerminal::GetAreCachesEmpty() const
{
  return FDirectoryCache->GetIsEmpty() &&
    ((FDirectoryChangesCache == NULL) || FDirectoryChangesCache->GetIsEmpty());
}
//---------------------------------------------------------------------------
void TTerminal::DoInitializeLog()
{
  if (FOnInitializeLog)
  {
    TCallbackGuard Guard(this);
    FOnInitializeLog(this);
    Guard.Verify();
  }
}
//------------------------------------------------------------------------------
void TTerminal::DoChangeDirectory()
{
  if (FOnChangeDirectory)
  {
    TCallbackGuard Guard(this);
    FOnChangeDirectory(this);
    Guard.Verify();
  }
}
//------------------------------------------------------------------------------
void TTerminal::DoReadDirectory(bool ReloadOnly)
{
  if (FOnReadDirectory)
  {
    TCallbackGuard Guard(this);
    FOnReadDirectory(this, ReloadOnly);
    Guard.Verify();
  }
}
//------------------------------------------------------------------------------
void TTerminal::DoStartReadDirectory()
{
  if (FOnStartReadDirectory)
  {
    TCallbackGuard Guard(this);
    FOnStartReadDirectory(this);
    Guard.Verify();
  }
}
//------------------------------------------------------------------------------
void TTerminal::DoReadDirectoryProgress(int Progress, bool & Cancel)
{
  if (FReadingCurrentDirectory && (FOnReadDirectoryProgress != NULL))
  {
    TCallbackGuard Guard(this);
    FOnReadDirectoryProgress(this, Progress, Cancel);
    Guard.Verify();
  }
  if (FOnFindingFile != NULL)
  {
    TCallbackGuard Guard(this);
    FOnFindingFile(this, L"", Cancel);
    Guard.Verify();
  }
}
//------------------------------------------------------------------------------
bool TTerminal::InTransaction()
{
  return (FInTransaction > 0) && !FSuspendTransaction;
}
//------------------------------------------------------------------------------
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
//------------------------------------------------------------------------------
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
      TRY_FINALLY (
      {
        if (FReadCurrentDirectoryPending)
        {
          ReadCurrentDirectory();
        }
        if (FReadDirectoryPending)
        {
          ReadDirectory(!FReadCurrentDirectoryPending);
        }
      }
      ,
      {
        FReadCurrentDirectoryPending = false;
        FReadDirectoryPending = false;
      }
      );
    }
  }

  if (FCommandSession != NULL)
  {
    FCommandSession->EndTransaction();
  }
}
//------------------------------------------------------------------------------
void TTerminal::SetExceptionOnFail(bool Value)
{
  if (Value)
  {
    FExceptionOnFail++;
  }
  else
  {
    if (FExceptionOnFail == 0)
      throw Exception(L"ExceptionOnFail is already zero.");
    FExceptionOnFail--;
  }

  if (FCommandSession != NULL)
  {
    FCommandSession->FExceptionOnFail = FExceptionOnFail;
  }
}
//------------------------------------------------------------------------------
bool TTerminal::GetExceptionOnFail() const
{
  return static_cast<bool>(FExceptionOnFail > 0);
}
//------------------------------------------------------------------------------
void TTerminal::FatalAbort()
{
  FatalError(NULL, "");
}
//------------------------------------------------------------------------------
void TTerminal::FatalError(Exception * E, const UnicodeString & Msg)
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
    throw ESshFatal(E, Msg);
  }
}
//------------------------------------------------------------------------------
void TTerminal::CommandError(Exception * E, const UnicodeString & Msg)
{
  CommandError(E, Msg, 0);
}
//------------------------------------------------------------------------------
uintptr_t TTerminal::CommandError(Exception * E, const UnicodeString & Msg,
  uintptr_t Answers)
{
  // may not be, particularly when TTerminal::Reopen is being called
  // from within OnShowExtendedException handler
  assert(FCallbackGuard == NULL);
  uintptr_t Result = 0;
  if (E && (dynamic_cast<EFatal *>(E) != NULL))
  {
    FatalError(E, Msg);
  }
  else if (E && (dynamic_cast<EAbort*>(E) != NULL))
  {
    // resent EAbort exception
    Abort();
  }
  else if (GetExceptionOnFail())
  {
    throw ECommand(E, Msg);
  }
  else if (!Answers)
  {
    ECommand * ECmd = new ECommand(E, Msg);
    TRY_FINALLY (
    {
      HandleExtendedException(ECmd);
    }
    ,
    {
      delete ECmd;
    }
    );
  }
  else
  {
    // small hack to enable "skip to all" for COMMAND_ERROR_ARI
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
//------------------------------------------------------------------------------
bool TTerminal::HandleException(Exception * E)
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
//------------------------------------------------------------------------------
void TTerminal::CloseOnCompletion(TOnceDoneOperation Operation, const UnicodeString & Message)
{
  LogEvent(L"Closing session after completed operation (as requested by user)");
  Close();
  throw ESshTerminate(NULL,
    Message.IsEmpty() ? UnicodeString(LoadStr(CLOSED_ON_COMPLETION)) : Message,
    Operation);
}
//------------------------------------------------------------------------------
TBatchOverwrite TTerminal::EffectiveBatchOverwrite(
  const TCopyParamType * CopyParam, intptr_t Params, TFileOperationProgressType * OperationProgress, bool Special)
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
  else if (CopyParam->GetNewerOnly() &&
           (((OperationProgress->Side == osLocal) && GetIsCapable(fcNewerOnlyUpload)) ||
            (OperationProgress->Side != osLocal)))
  {
    // no way to change batch overwrite mode when CopyParam->NewerOnly is on
    Result = boOlder;
  }
  else if (FLAGSET(Params, cpNoConfirmation) || !FConfiguration->GetConfirmOverwriting())
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
//------------------------------------------------------------------------------
bool TTerminal::CheckRemoteFile(
  const TCopyParamType * CopyParam, intptr_t Params, TFileOperationProgressType * OperationProgress)
{
  return (EffectiveBatchOverwrite(CopyParam, Params, OperationProgress, true) != boAll);
}
//------------------------------------------------------------------------------
uintptr_t TTerminal::ConfirmFileOverwrite(const UnicodeString & FileName,
  const TOverwriteFileParams * FileParams, uintptr_t Answers, TQueryParams * QueryParams,
  TOperationSide Side, const TCopyParamType * CopyParam, intptr_t Params, TFileOperationProgressType * OperationProgress,
  const UnicodeString & Message)
{
  uintptr_t Result = 0;
  // duplicated in TSFTPFileSystem::SFTPConfirmOverwrite
  bool CanAlternateResume =
    (FileParams != NULL) &&
    (FileParams->DestSize < FileParams->SourceSize) &&
    !OperationProgress->AsciiTransfer;
  TBatchOverwrite BatchOverwrite = EffectiveBatchOverwrite(CopyParam, Params, OperationProgress, true);
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
    TBatchOverwrite ABatchOverwrite = EffectiveBatchOverwrite(CopyParam, Params, OperationProgress, false);
    assert(BatchOverwrite != ABatchOverwrite);
    BatchOverwrite = ABatchOverwrite;
  }

  if (BatchOverwrite == boNo)
  {
    UnicodeString Msg = Message;
    if (Msg.IsEmpty())
    {
      Msg = FMTLOAD((Side == osLocal ? LOCAL_FILE_OVERWRITE2 :
        REMOTE_FILE_OVERWRITE2), FileName.c_str(), FileName.c_str());
    }
    if (FileParams != NULL)
    {
      Msg = FMTLOAD(FILE_OVERWRITE_DETAILS, Msg.c_str(),
        Int64ToStr(FileParams->SourceSize).c_str(),
        UserModificationStr(FileParams->SourceTimestamp, FileParams->SourcePrecision).c_str(),
        Int64ToStr(FileParams->DestSize).c_str(),
        UserModificationStr(FileParams->DestTimestamp, FileParams->DestPrecision).c_str());
    }
    if (ALWAYS_TRUE(QueryParams->HelpKeyword.IsEmpty()))
    {
      QueryParams->HelpKeyword = L"ui_overwrite";
    }
    Result = QueryUser(Msg, NULL, Answers, QueryParams);
    switch (Result)
    {
      case qaNeverAskAgain:
        FConfiguration->SetConfirmOverwriting(false);
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
        if (FileParams == NULL)
        {
          Result = qaNo;
        }
        else
        {
          TModificationFmt Precision = LessDateTimePrecision(FileParams->SourcePrecision, FileParams->DestPrecision);
          TDateTime ReducedSourceTimestamp =
            ReduceDateTimePrecision(FileParams->SourceTimestamp, Precision);
          TDateTime ReducedDestTimestamp =
            ReduceDateTimePrecision(FileParams->DestTimestamp, Precision);

          Result =
            (CompareFileTime(ReducedSourceTimestamp, ReducedDestTimestamp) > 0) ?
            qaYes : qaNo;

          LogEvent(FORMAT(L"Source file timestamp is [%s], destination timestamp is [%s], will%s overwrite",
            (StandardTimestamp(ReducedSourceTimestamp),
             StandardTimestamp(ReducedDestTimestamp),
             ((Result == qaYes) ? L"" : L" not"))));
        }
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
//------------------------------------------------------------------------------
void TTerminal::FileModified(const TRemoteFile * File,
  const UnicodeString & FileName, bool ClearDirectoryChange)
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
//------------------------------------------------------------------------------
void TTerminal::DirectoryModified(const UnicodeString & Path, bool SubDirs)
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
//------------------------------------------------------------------------------
void TTerminal::DirectoryLoaded(TRemoteFileList * FileList)
{
  AddCachedFileList(FileList);
}
//------------------------------------------------------------------------------
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
//------------------------------------------------------------------------------
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
//------------------------------------------------------------------------------
void TTerminal::EnsureNonExistence(const UnicodeString & FileName)
{
  // if filename doesn't contain path, we check for existence of file
  if ((UnixExtractFileDir(FileName).IsEmpty()) &&
      UnixComparePaths(GetCurrentDirectory(), FFiles->GetDirectory()))
  {
    TRemoteFile * File = FFiles->FindFile(FileName);
    if (File)
    {
      if (File->GetIsDirectory())
      {
        throw ECommand(NULL, FMTLOAD(RENAME_CREATE_DIR_EXISTS, FileName.c_str()));
      }
      else
      {
        throw ECommand(NULL, FMTLOAD(RENAME_CREATE_FILE_EXISTS, FileName.c_str()));
      }
    }
  }
}
//------------------------------------------------------------------------------
void TTerminal::LogEvent(const UnicodeString & Str)
{
  if (GetLog()->GetLogging())
  {
    GetLog()->Add(llMessage, Str);
  }
}
//------------------------------------------------------------------------------
void TTerminal::RollbackAction(TSessionAction & Action,
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
//------------------------------------------------------------------------------
void TTerminal::DoStartup()
{
  LogEvent(L"Doing startup conversation with host.");
  BeginTransaction();
  TRY_FINALLY (
  {
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
  ,
  {
    EndTransaction();
  }
  );
  LogEvent(L"Startup conversation with host finished.");
}
//------------------------------------------------------------------------------
void TTerminal::ReadCurrentDirectory()
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
      UnicodeString CurrentDirectory = GetCurrentDirectory();
      if (!CurrentDirectory.IsEmpty() && !FLastDirectoryChange.IsEmpty() && (CurrentDirectory != OldDirectory))
      {
        FDirectoryChangesCache->AddDirectoryChange(OldDirectory,
          FLastDirectoryChange, CurrentDirectory);
      }
      // not to broke the cache, if the next directory change would not
      // be initialized by ChangeDirectory(), which sets it
      // (HomeDirectory() particularly)
      FLastDirectoryChange = L"";
    }

    if (OldDirectory.IsEmpty())
    {
      FLockDirectory = (GetSessionData()->GetLockInHome() ?
        FFileSystem->GetCurrentDirectory() : UnicodeString(L""));
    }
    /* if (OldDirectory != FFileSystem->GetCurrentDirectory()) */ { DoChangeDirectory(); }
  }
  catch (Exception &E)
  {
    CommandError(&E, LoadStr(READ_CURRENT_DIR_ERROR));
  }
}
//------------------------------------------------------------------------------
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
      TRY_FINALLY (
      {
        LoadedFromCache = FDirectoryCache->GetFileList(GetCurrentDirectory(), FFiles);
      }
      ,
      {
        DoReadDirectory(ReloadOnly);
      }
      );

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
      TRY_FINALLY (
      {
        Files->SetDirectory(GetCurrentDirectory());
        CustomReadDirectory(Files);
      }
      ,
      {
        DoReadDirectoryProgress(-1, Cancel);
        FReadingCurrentDirectory = false;
        TRemoteDirectory * OldFiles = FFiles;
        FFiles = Files;
        TRY_FINALLY (
        {
          DoReadDirectory(ReloadOnly);
        }
        ,
        {
          // delete only after loading new files to dir view,
          // not to destroy the file objects that the view holds
          // (can be issue in multi threaded environment, such as when the
          // terminal is reconnecting in the terminal thread)
          delete OldFiles;
        }
        );
        if (GetActive())
        {
          if (GetSessionData()->GetCacheDirectories())
          {
            DirectoryLoaded(FFiles);
          }
        }
      }
      );
    }
    catch (Exception &E)
    {
      CommandError(&E, FmtLoadStr(LIST_DIR_ERROR, FFiles->GetDirectory().c_str()));
    }
  }
}
//---------------------------------------------------------------------------
void TTerminal::LogFile(TRemoteFile * File)
{
  if (File)
  {
    LogEvent(FORMAT(L"%s;%c;%lld;%s;%s;%s;%s;%d",
      File->GetFileName().c_str(), File->GetType(), File->GetSize(), StandardTimestamp(File->GetModification()).c_str(),
      File->GetFileOwner().GetLogText().c_str(), File->GetFileGroup().GetLogText().c_str(), File->GetRights()->GetText().c_str(),
      File->GetAttr()));
  }
}
//------------------------------------------------------------------------------
void TTerminal::CustomReadDirectory(TRemoteFileList * FileList)
{
  assert(FileList);
  assert(FFileSystem);
  FFileSystem->ReadDirectory(FileList);

  if (GetLog()->GetLogging())
  {
    for (intptr_t Index = 0; Index < FileList->GetCount(); ++Index)
    {
      LogFile(FileList->GetFiles(Index));
    }
  }

  ReactOnCommand(fsListDirectory);
}
//------------------------------------------------------------------------------
TRemoteFileList * TTerminal::ReadDirectoryListing(const UnicodeString & Directory, const TFileMasks & Mask)
{
  TLsSessionAction Action(GetActionLog(), AbsolutePath(Directory, true));
  TRemoteFileList * FileList = NULL;
  try
  {
    FileList = DoReadDirectoryListing(Directory, false);
    if (FileList != NULL)
    {
      intptr_t Index = 0;
      while (Index < FileList->GetCount())
      {
        TRemoteFile * File = FileList->GetFiles(Index);
        if (!Mask.Matches(File->GetFileName()))
        {
          FileList->Delete(Index);
        }
        else
        {
          ++Index;
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
//------------------------------------------------------------------------------
TRemoteFile * TTerminal::ReadFileListing(const UnicodeString & Path)
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
//------------------------------------------------------------------------------
TRemoteFileList * TTerminal::CustomReadDirectoryListing(const UnicodeString & Directory, bool UseCache)
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
//------------------------------------------------------------------------------
TRemoteFileList * TTerminal::DoReadDirectoryListing(const UnicodeString & Directory, bool UseCache)
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
      TRY_FINALLY (
      {
        ReadDirectory(FileList);
      }
      ,
      {
        SetExceptionOnFail(false);
      }
      );

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
//------------------------------------------------------------------------------
void TTerminal::ProcessDirectory(const UnicodeString & DirName,
  TProcessFileEvent CallBackFunc, void * Param, bool UseCache, bool IgnoreErrors)
{
  TRemoteFileList * FileList = NULL;
  if (IgnoreErrors)
  {
    SetExceptionOnFail(true);
    TRY_FINALLY (
    {
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
    ,
    {
      SetExceptionOnFail(false);
    }
    );
  }
  else
  {
    FileList = CustomReadDirectoryListing(DirName, UseCache);
  }

  // skip if directory listing fails and user selects "skip"
  if (FileList)
  {
    TRY_FINALLY (
    {
      UnicodeString Directory = UnixIncludeTrailingBackslash(DirName);

      for (intptr_t Index = 0; Index < FileList->GetCount(); ++Index)
      {
        TRemoteFile * File = FileList->GetFiles(Index);
        if (!File->GetIsParentDirectory() && !File->GetIsThisDirectory())
        {
          CallBackFunc(Directory + File->GetFileName(), File, Param);
        }
      }
    }
    ,
    {
      delete FileList;
    }
    );
  }
}
//------------------------------------------------------------------------------
void TTerminal::ReadDirectory(TRemoteFileList * FileList)
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
//------------------------------------------------------------------------------
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
  catch (Exception &E)
  {
    CommandError(&E, FMTLOAD(READ_SYMLINK_ERROR, SymlinkFile->GetFileName().c_str()));
  }
}
//------------------------------------------------------------------------------
void TTerminal::ReadFile(const UnicodeString & FileName,
  TRemoteFile *& File)
{
  assert(FFileSystem);
  File = NULL;
  try
  {
    LogEvent(FORMAT(L"Listing file \"%s\".", FileName.c_str()));
    FFileSystem->ReadFile(FileName, File);
    ReactOnCommand(fsListFile);
    LogFile(File);
  }
  catch (Exception &E)
  {
    if (File)
    {
      delete File;
    }
    File = NULL;
    CommandError(&E, FMTLOAD(CANT_GET_ATTRS, FileName.c_str()));
  }
}
//------------------------------------------------------------------------------
bool TTerminal::FileExists(const UnicodeString & FileName, TRemoteFile ** AFile)
{
  bool Result;
  TRemoteFile * File = NULL;
  try
  {
    SetExceptionOnFail(true);
    TRY_FINALLY (
    {
      ReadFile(FileName, File);
    }
    ,
    {
      SetExceptionOnFail(false);
    }
    );

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
//------------------------------------------------------------------------------
void TTerminal::AnnounceFileListOperation()
{
  FFileSystem->AnnounceFileListOperation();
}
//------------------------------------------------------------------------------
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
    TFileOperationProgressType Progress(MAKE_CALLBACK(TTerminal::DoProgress, this), MAKE_CALLBACK(TTerminal::DoFinished, this));
    Progress.Start(Operation, Side, FileList->GetCount());

    FOperationProgress = &Progress; //-V506
    TFileOperationProgressType * OperationProgress(&Progress);
    TRY_FINALLY (
    {
      if (Side == osRemote)
      {
        BeginTransaction();
      }

      TRY_FINALLY (
      {
        intptr_t Index = 0;
        UnicodeString FileName;
        bool Success;
        while ((Index < FileList->GetCount()) && (Progress.Cancel == csContinue))
        {
          FileName = FileList->GetString(Index);
          try
          {
            TRY_FINALLY (
            {
              Success = false;
              if (!Ex)
              {
                TRemoteFile * RemoteFile = static_cast<TRemoteFile *>(FileList->GetObject(Index));
                ProcessFile(FileName, RemoteFile, Param);
              }
              else
              {
                // not used anymore
                // TProcessFileEventEx ProcessFileEx = (TProcessFileEventEx)ProcessFile;
                // ProcessFileEx(FileName, (TRemoteFile *)FileList->GetObject(Index), Param, Index);
              }
              Success = true;
            }
            ,
            {
              Progress.Finish(FileName, Success, OnceDoneOperation);
            }
            );
          }
          catch(EScpSkipFile & E)
          {
            DEBUG_PRINTF(L"before HandleException");
            SUSPEND_OPERATION (
              if (!HandleException(&E)) throw;
            );
          }
          ++Index;
        }
      }
      ,
      {
        if (Side == osRemote)
        {
          EndTransaction();
        }
      }
      );

      if (Progress.Cancel == csContinue)
      {
        Result = true;
      }
    }
    ,
    {
      FOperationProgress = NULL;
      Progress.Stop();
    }
    );
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
//------------------------------------------------------------------------------
// not used anymore
bool TTerminal::ProcessFilesEx(TStrings * FileList, TFileOperation Operation,
  TProcessFileEventEx ProcessFile, void * Param, TOperationSide Side)
{
#if defined(__BORLANDC__)
  return ProcessFiles(FileList, Operation, TProcessFileEvent(ProcessFile),
    Param, Side, true);
#else
  return false;
#endif
}
//------------------------------------------------------------------------------
TStrings * TTerminal::GetFixedPaths()
{
  assert(FFileSystem != NULL);
  return FFileSystem->GetFixedPaths();
}
//------------------------------------------------------------------------------
bool TTerminal::GetResolvingSymlinks()
{
  return GetSessionData()->GetResolveSymlinks() && GetIsCapable(fcResolveSymlink);
}
//------------------------------------------------------------------------------
TUsableCopyParamAttrs TTerminal::UsableCopyParamAttrs(intptr_t Params)
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
//------------------------------------------------------------------------------
bool TTerminal::IsRecycledFile(const UnicodeString & FileName)
{
  bool Result = !GetSessionData()->GetRecycleBinPath().IsEmpty();
  if (Result)
  {
    UnicodeString Path = UnixExtractFilePath(FileName);
    if (Path.IsEmpty())
    {
      Path = GetCurrentDirectory();
    }
    Result = UnixComparePaths(Path, GetSessionData()->GetRecycleBinPath());
  }
  return Result;
}
//------------------------------------------------------------------------------
void TTerminal::RecycleFile(const UnicodeString & FileName,
  const TRemoteFile * File)
{
  UnicodeString FileName2 = FileName;
  if (FileName2.IsEmpty())
  {
    assert(File != NULL);
    FileName2 = File->GetFileName();
  }

  if (!IsRecycledFile(FileName2))
  {
    LogEvent(FORMAT(L"Moving file \"%s\" to remote recycle bin '%s'.",
      FileName2.c_str(), GetSessionData()->GetRecycleBinPath().c_str()));

    TMoveFileParams Params;
    Params.Target = GetSessionData()->GetRecycleBinPath();
#if defined(__BORLANDC__)
    Params.FileMask = FORMAT(L"*-%s.*", (FormatDateTime(L"yyyymmdd-hhnnss", Now())));
#else
    unsigned short Y, M, D, H, N, S, MS;
    TDateTime DateTime = Now();
    DateTime.DecodeDate(Y, M, D);
    DateTime.DecodeTime(H, N, S, MS);
    UnicodeString dt = FORMAT(L"%04d%02d%02d-%02d%02d%02d", Y, M, D, H, N, S);
    // Params.FileMask = FORMAT(L"*-%s.*", FormatDateTime(L"yyyymmdd-hhnnss", Now()).c_str());
    Params.FileMask = FORMAT(L"*-%s.*", dt.c_str());
#endif
    MoveFile(FileName2, File, &Params);
  }
}
//------------------------------------------------------------------------------
void TTerminal::DeleteFile(const UnicodeString & FileName,
  const TRemoteFile * File, void * AParams)
{
  UnicodeString LocalFileName = FileName;
  if (FileName.IsEmpty() && File)
  {
    LocalFileName = File->GetFileName();
  }
  if (GetOperationProgress() && GetOperationProgress()->Operation == foDelete)
  {
    if (GetOperationProgress()->Cancel != csContinue)
    {
      Abort();
    }
    GetOperationProgress()->SetFile(LocalFileName);
  }
  intptr_t Params = (AParams != NULL) ? *(static_cast<int*>(AParams)) : 0;
  bool Recycle =
    FLAGCLEAR(Params, dfForceDelete) &&
    (GetSessionData()->GetDeleteToRecycleBin() != FLAGSET(Params, dfAlternative)) &&
    !GetSessionData()->GetRecycleBinPath().IsEmpty();
  if (Recycle && !IsRecycledFile(LocalFileName))
  {
    RecycleFile(LocalFileName, File);
  }
  else
  {
    LogEvent(FORMAT(L"Deleting file \"%s\".", LocalFileName.c_str()));
    if (File)
    {
      FileModified(File, LocalFileName, true);
    }
    DoDeleteFile(LocalFileName, File, Params);
    ReactOnCommand(fsDeleteFile);
  }
}
//------------------------------------------------------------------------------
void TTerminal::DoDeleteFile(const UnicodeString & FileName,
  const TRemoteFile * File, intptr_t Params)
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
//------------------------------------------------------------------------------
bool TTerminal::DeleteFiles(TStrings * FilesToDelete, intptr_t Params)
{
  // TODO: avoid resolving symlinks while reading subdirectories.
  // Resolving does not work anyway for relative symlinks in subdirectories
  // (at least for SFTP).
  return ProcessFiles(FilesToDelete, foDelete, MAKE_CALLBACK(TTerminal::DeleteFile, this), &Params);
}
//------------------------------------------------------------------------------
void TTerminal::DeleteLocalFile(const UnicodeString & FileName,
  const TRemoteFile * /*File*/, void * Params)
{
  if (GetOnDeleteLocalFile() == NULL)
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
//------------------------------------------------------------------------------
bool TTerminal::DeleteLocalFiles(TStrings * FileList, intptr_t Params)
{
  return ProcessFiles(FileList, foDelete, MAKE_CALLBACK(TTerminal::DeleteLocalFile, this), &Params, osLocal);
}
//------------------------------------------------------------------------------
void TTerminal::CustomCommandOnFile(const UnicodeString & FileName,
  const TRemoteFile * File, void * AParams)
{
  TCustomCommandParams * Params = (static_cast<TCustomCommandParams *>(AParams));
  UnicodeString LocalFileName = FileName;
  if (FileName.IsEmpty() && File)
  {
    LocalFileName = File->GetFileName();
  }
  if (GetOperationProgress() && GetOperationProgress()->Operation == foCustomCommand)
  {
    if (GetOperationProgress()->Cancel != csContinue)
    {
      Abort();
    }
    GetOperationProgress()->SetFile(LocalFileName);
  }
  LogEvent(FORMAT(L"Executing custom command \"%s\" (%d) on file \"%s\".",
    Params->Command.c_str(), Params->Params, LocalFileName.c_str()));
  if (File)
  {
    FileModified(File, LocalFileName);
  }
  DoCustomCommandOnFile(LocalFileName, File, Params->Command, Params->Params,
    Params->OutputEvent);
  ReactOnCommand(fsAnyCommand);
}
//------------------------------------------------------------------------------
void TTerminal::DoCustomCommandOnFile(const UnicodeString & FileName,
  const TRemoteFile * File, const UnicodeString & Command, intptr_t Params,
  TCaptureOutputEvent OutputEvent)
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

      if (FCommandSession->GetCurrentDirectory() != GetCurrentDirectory())
      {
        FCommandSession->SetCurrentDirectory(GetCurrentDirectory());
        // We are likely in transaction, so ReadCurrentDirectory won't get called
        // until transaction ends. But we need to know CurrentDirectory to
        // expand !/ pattern.
        // Doing this only, when current directory of the main and secondary shell differs,
        // what would be the case before the first file in transation.
        // Otherwise we would be reading pwd before every time as the
        // CustomCommandOnFile on its own sets FReadCurrentDirectoryPending
        if (FCommandSession->FReadCurrentDirectoryPending)
        {
          FCommandSession->ReadCurrentDirectory();
        }
      }
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
//------------------------------------------------------------------------------
void TTerminal::CustomCommandOnFiles(const UnicodeString & Command,
  intptr_t Params, TStrings * Files, TCaptureOutputEvent OutputEvent)
{
  if (!TRemoteCustomCommand().IsFileListCommand(Command))
  {
    TCustomCommandParams AParams;
    AParams.Command = Command;
    AParams.Params = Params;
    AParams.OutputEvent = OutputEvent;
    ProcessFiles(Files, foCustomCommand, MAKE_CALLBACK(TTerminal::CustomCommandOnFile, this), &AParams);
  }
  else
  {
    UnicodeString FileList;
    for (intptr_t I = 0; I < Files->GetCount(); ++I)
    {
      TRemoteFile * File = static_cast<TRemoteFile *>(Files->GetObject(I));
      bool Dir = File->GetIsDirectory() && !File->GetIsSymLink();

      if (!Dir || FLAGSET(Params, ccApplyToDirectories))
      {
        if (!FileList.IsEmpty())
        {
          FileList += L" ";
        }

        FileList += L"\"" + ShellDelimitStr(Files->GetString(I), L'"') + L"\"";
      }
    }

    TCustomCommandData Data(this);
    UnicodeString Cmd =
      TRemoteCustomCommand(Data, GetCurrentDirectory(), L"", FileList).
        Complete(Command, true);
    DoAnyCommand(Cmd, OutputEvent, NULL);
  }
}
//------------------------------------------------------------------------------
void TTerminal::ChangeFileProperties(const UnicodeString & FileName,
  const TRemoteFile * File, /*const TRemoteProperties*/ void * Properties)
{
  TRemoteProperties * RProperties = static_cast<TRemoteProperties *>(Properties);
  assert(RProperties && !RProperties->Valid.Empty());
  UnicodeString LocalFileName = FileName;
  if (FileName.IsEmpty() && File)
  {
    LocalFileName = File->GetFileName();
  }
  if (GetOperationProgress() && GetOperationProgress()->Operation == foSetProperties)
  {
    if (GetOperationProgress()->Cancel != csContinue) { Abort(); }
    GetOperationProgress()->SetFile(LocalFileName);
  }
  if (GetLog()->GetLogging())
  {
    LogEvent(FORMAT(L"Changing properties of \"%s\" (%s)",
      LocalFileName.c_str(), BooleanToEngStr(RProperties->Recursive).c_str()));
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
      unsigned short Y, M, D, H, N, S, MS;
      TDateTime DateTime = UnixToDateTime(RProperties->Modification, GetSessionData()->GetDSTMode());
      DateTime.DecodeDate(Y, M, D);
      DateTime.DecodeTime(H, N, S, MS);
      UnicodeString dt = FORMAT(L"%02d.%02d.%04d %02d:%02d:%02d ", D, M, Y, H, N, S);
      LogEvent(FORMAT(L" - modification: \"%s\"",
        // FormatDateTime(L"dddddd tt",
           // UnixToDateTime(RProperties->Modification, GetSessionData()->GetDSTMode())).c_str()));
           dt.c_str()));
    }
    if (RProperties->Valid.Contains(vpLastAccess))
    {
      unsigned short Y, M, D, H, N, S, MS;
      TDateTime DateTime = UnixToDateTime(RProperties->LastAccess, GetSessionData()->GetDSTMode());
      DateTime.DecodeDate(Y, M, D);
      DateTime.DecodeTime(H, N, S, MS);
      UnicodeString dt = FORMAT(L"%02d.%02d.%04d %02d:%02d:%02d ", D, M, Y, H, N, S);
      LogEvent(FORMAT(L" - last access: \"%s\"",
        // FormatDateTime(L"dddddd tt",
           // UnixToDateTime(RProperties->LastAccess, GetSessionData()->GetDSTMode())).c_str()));
           dt.c_str()));
    }
  }
  FileModified(File, LocalFileName);
  DoChangeFileProperties(LocalFileName, File, RProperties);
  ReactOnCommand(fsChangeProperties);
}
//------------------------------------------------------------------------------
void TTerminal::DoChangeFileProperties(const UnicodeString & FileName,
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
//------------------------------------------------------------------------------
void TTerminal::ChangeFilesProperties(TStrings * FileList,
  const TRemoteProperties * Properties)
{
  AnnounceFileListOperation();
  ProcessFiles(FileList, foSetProperties, MAKE_CALLBACK(TTerminal::ChangeFileProperties, this), const_cast<void *>(static_cast<const void *>(Properties)));
}
//------------------------------------------------------------------------------
bool TTerminal::LoadFilesProperties(TStrings * FileList)
{
  bool Result =
    GetIsCapable(fcLoadingAdditionalProperties) &&
    FFileSystem->LoadFilesProperties(FileList);
  if (Result && GetSessionData()->GetCacheDirectories() &&
      (FileList->GetCount() > 0) &&
      (dynamic_cast<TRemoteFile *>(FileList->GetObject(0))->GetDirectory() == FFiles))
  {
    AddCachedFileList(FFiles);
  }
  return Result;
}
//------------------------------------------------------------------------------
void TTerminal::CalculateFileSize(const UnicodeString & FileName,
  const TRemoteFile * File, /*TCalculateSizeParams*/ void * Param)
{
  assert(Param);
  assert(File);
  TCalculateSizeParams * AParams = static_cast<TCalculateSizeParams *>(Param);
  UnicodeString LocalFileName = FileName;
  if (FileName.IsEmpty())
  {
    LocalFileName = File->GetFileName();
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
        LogEvent(FORMAT(L"Getting size of directory \"%s\"", LocalFileName.c_str()));
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
    GetOperationProgress()->SetFile(LocalFileName);
  }
}
//------------------------------------------------------------------------------
void TTerminal::DoCalculateDirectorySize(const UnicodeString & FileName,
  const TRemoteFile * File, TCalculateSizeParams * Params)
{
  try
  {
    ProcessDirectory(FileName, MAKE_CALLBACK(TTerminal::CalculateFileSize, this), Params);
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
//------------------------------------------------------------------------------
void TTerminal::CalculateFilesSize(TStrings * FileList,
  __int64 & Size, intptr_t Params, const TCopyParamType * CopyParam,
  TCalculateSizeStats * Stats)
{
  TCalculateSizeParams Param;
  Param.Size = 0;
  Param.Params = Params;
  Param.CopyParam = CopyParam;
  Param.Stats = Stats;
  ProcessFiles(FileList, foCalculateSize, MAKE_CALLBACK(TTerminal::CalculateFileSize, this), &Param);
  Size = Param.Size;
}
//------------------------------------------------------------------------------
void TTerminal::CalculateFilesChecksum(const UnicodeString & Alg,
  TStrings * FileList, TStrings * Checksums,
  TCalculatedChecksumEvent OnCalculatedChecksum)
{
  FFileSystem->CalculateFilesChecksum(Alg, FileList, Checksums, OnCalculatedChecksum);
}
//------------------------------------------------------------------------------
void TTerminal::RenameFile(const UnicodeString & FileName,
  const UnicodeString & NewName)
{
  LogEvent(FORMAT(L"Renaming file \"%s\" to \"%s\".", FileName.c_str(), NewName.c_str()));
  DoRenameFile(FileName, NewName, false);
  ReactOnCommand(fsRenameFile);
}
//------------------------------------------------------------------------------
void TTerminal::RenameFile(const TRemoteFile * File,
  const UnicodeString & NewName, bool CheckExistence)
{
  assert(File && File->GetDirectory() == FFiles);
  bool Proceed = true;
  // if filename doesn't contain path, we check for existence of file
  if ((File->GetFileName() != NewName) && CheckExistence &&
      FConfiguration->GetConfirmOverwriting() &&
      UnixComparePaths(GetCurrentDirectory(), FFiles->GetDirectory()))
  {
    TRemoteFile * DuplicateFile = FFiles->FindFile(NewName);
    if (DuplicateFile)
    {
      UnicodeString QuestionFmt;
      if (DuplicateFile->GetIsDirectory())
      {
        QuestionFmt = LoadStr(DIRECTORY_OVERWRITE);
      }
      else
      {
        QuestionFmt = LoadStr(PROMPT_FILE_OVERWRITE);
      }
      TQueryParams Params(qpNeverAskAgainCheck);
      intptr_t Result = QueryUser(FORMAT(QuestionFmt.c_str(), NewName.c_str()), NULL,
        qaYes | qaNo, &Params);
      if (Result == qaNeverAskAgain)
      {
        Proceed = true;
        FConfiguration->SetConfirmOverwriting(false);
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
//------------------------------------------------------------------------------
void TTerminal::DoRenameFile(const UnicodeString & FileName,
  const UnicodeString & NewName, bool Move)
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
//------------------------------------------------------------------------------
void TTerminal::MoveFile(const UnicodeString & FileName,
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
//------------------------------------------------------------------------------
bool TTerminal::MoveFiles(TStrings * FileList, const UnicodeString & Target,
  const UnicodeString & FileMask)
{
  TMoveFileParams Params;
  Params.Target = Target;
  Params.FileMask = FileMask;
  DirectoryModified(Target, true);
  bool Result = false;
  BeginTransaction();
  TRY_FINALLY (
  {
    Result = ProcessFiles(FileList, foRemoteMove, MAKE_CALLBACK(TTerminal::MoveFile, this), &Params);
  }
  ,
  {
    if (GetActive())
    {
      UnicodeString WithTrailing = UnixIncludeTrailingBackslash(GetCurrentDirectory());
      bool PossiblyMoved = false;
      // check if we was moving current directory.
      // this is just optimization to avoid checking existence of current
      // directory after each move operation.
      UnicodeString curDirectory = GetCurrentDirectory();
      for (intptr_t Index = 0; !PossiblyMoved && (Index < FileList->GetCount()); ++Index)
      {
        const TRemoteFile * File =
          dynamic_cast<const TRemoteFile *>(FileList->GetObject(Index));
        // File can be NULL, and filename may not be full path,
        // but currently this is the only way we can move (at least in GUI)
        // current directory
        if ((File != NULL) &&
            File->GetIsDirectory() &&
            ((curDirectory.SubString(1, FileList->GetString(Index).Length()) == FileList->GetString(Index)) &&
             ((FileList->GetString(Index).Length() == curDirectory.Length()) ||
              (curDirectory[FileList->GetString(Index).Length() + 1] == '/'))))
        {
          PossiblyMoved = true;
        }
      }

      if (PossiblyMoved && !FileExists(curDirectory))
      {
        UnicodeString NearestExisting = curDirectory;
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
  );
  return Result;
}
//------------------------------------------------------------------------------
void TTerminal::DoCopyFile(const UnicodeString & FileName,
  const UnicodeString & NewName)
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
      FCommandSession->SetCurrentDirectory(GetCurrentDirectory());
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
//------------------------------------------------------------------------------
void TTerminal::CopyFile(const UnicodeString & FileName,
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
//------------------------------------------------------------------------------
bool TTerminal::CopyFiles(TStrings * FileList, const UnicodeString & Target,
  const UnicodeString & FileMask)
{
  TMoveFileParams Params;
  Params.Target = Target;
  Params.FileMask = FileMask;
  DirectoryModified(Target, true);
  return ProcessFiles(FileList, foRemoteCopy, MAKE_CALLBACK(TTerminal::CopyFile, this), &Params);
}
//------------------------------------------------------------------------------
void TTerminal::CreateDirectory(const UnicodeString & DirName,
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
//------------------------------------------------------------------------------
void TTerminal::DoCreateDirectory(const UnicodeString & DirName)
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
//------------------------------------------------------------------------------
void TTerminal::CreateLink(const UnicodeString & FileName,
  const UnicodeString & PointTo, bool Symbolic)
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
//------------------------------------------------------------------------------
void TTerminal::DoCreateLink(const UnicodeString & FileName,
  const UnicodeString & PointTo, bool Symbolic)
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
//------------------------------------------------------------------------------
void TTerminal::HomeDirectory()
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
//------------------------------------------------------------------------------
void TTerminal::ChangeDirectory(const UnicodeString & Directory)
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
//------------------------------------------------------------------------------
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
//------------------------------------------------------------------------------
bool TTerminal::AllowedAnyCommand(const UnicodeString & Command)
{
  return !Command.Trim().IsEmpty();
}
//------------------------------------------------------------------------------
bool TTerminal::GetCommandSessionOpened()
{
  // consider secondary terminal open in "ready" state only
  // so we never do keepalives on it until it is completely initialized
  return (FCommandSession != NULL) &&
    (FCommandSession->GetStatus() == ssOpened);
}
//------------------------------------------------------------------------------
TTerminal * TTerminal::GetCommandSession()
{
  if ((FCommandSession != NULL) && !FCommandSession->GetActive())
  {
    SAFE_DESTROY(FCommandSession);
  }

  if (FCommandSession == NULL)
  {
    // transaction cannot be started yet to allow proper matching transaction
    // levels between main and command session
    assert(FInTransaction == 0);

    try
    {
      FCommandSession = new TSecondaryTerminal(this);
      (static_cast<TSecondaryTerminal *>(FCommandSession))->Init(GetSessionData(),
        FConfiguration, L"Shell");

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
//------------------------------------------------------------------------------
class TOutputProxy : public TObject
{
public:
  TOutputProxy(TCallSessionAction & Action, TCaptureOutputEvent OutputEvent) :
    FAction(Action),
    FOutputEvent(OutputEvent)
  {
  }

  void Output(const UnicodeString & Str, bool StdError)
  {
    FAction.AddOutput(Str, StdError);
    if (FOutputEvent != NULL)
    {
      FOutputEvent(Str, StdError);
    }
  }

private:
  TCallSessionAction & FAction;
  TCaptureOutputEvent FOutputEvent;
};

//------------------------------------------------------------------------------
void TTerminal::AnyCommand(const UnicodeString & Command,
  TCaptureOutputEvent OutputEvent)
{
  TCallSessionAction Action(GetActionLog(), Command, GetCurrentDirectory());
  TOutputProxy ProxyOutputEvent(Action, OutputEvent);
  DoAnyCommand(Command, MAKE_CALLBACK(TOutputProxy::Output, &ProxyOutputEvent), &Action);
}
//------------------------------------------------------------------------------
void TTerminal::DoAnyCommand(const UnicodeString & Command,
  TCaptureOutputEvent OutputEvent, TCallSessionAction * Action)
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

      // synchronize pwd (by purpose we lose transaction optimization here)
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
    if (GetExceptionOnFail() || (dynamic_cast<EFatal *>(&E) != NULL)) { throw; }
    else { HandleExtendedException(&E); }
  }
}
//------------------------------------------------------------------------------
bool TTerminal::DoCreateLocalFile(const UnicodeString & FileName,
  TFileOperationProgressType * OperationProgress, HANDLE * AHandle,
  bool NoConfirmation)
{
  bool Result = true;
  bool Done;
  DWORD CreateAttrs = FILE_ATTRIBUTE_NORMAL;
  do
  {
    *AHandle = CreateLocalFile(FileName.c_str(), GENERIC_WRITE, FILE_SHARE_READ,
      CREATE_ALWAYS, CreateAttrs);
    Done = (*AHandle != INVALID_HANDLE_VALUE);
    if (!Done)
    {
      DWORD LocalFileAttrs = 0;
      if (::FileExists(FileName) &&
        (((LocalFileAttrs = GetLocalFileAttributes(FileName)) & (faReadOnly | faHidden)) != 0))
      {
        if (FLAGSET(LocalFileAttrs, faReadOnly))
        {
          if (OperationProgress->BatchOverwrite == boNone)
          {
            Result = false;
          }
          else if ((OperationProgress->BatchOverwrite != boAll) && !NoConfirmation)
          {
            uintptr_t Answer;
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
          assert(FLAGSET(LocalFileAttrs, faHidden));
          Result = true;
        }

        if (Result)
        {
          CreateAttrs |=
            FLAGMASK(FLAGSET(LocalFileAttrs, faHidden), FILE_ATTRIBUTE_HIDDEN) |
            FLAGMASK(FLAGSET(LocalFileAttrs, faReadOnly), FILE_ATTRIBUTE_READONLY);

          FILE_OPERATION_LOOP (FMTLOAD(CANT_SET_ATTRS, FileName.c_str()),
            if (!SetLocalFileAttributes(FileName, LocalFileAttrs & ~(faReadOnly | faHidden)))
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
//------------------------------------------------------------------------------
bool TTerminal::CreateLocalFile(const UnicodeString & FileName,
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
//------------------------------------------------------------------------------
void TTerminal::OpenLocalFile(const UnicodeString & FileName,
  uintptr_t Access, uintptr_t * AAttrs, HANDLE * AHandle, __int64 * ACTime,
  __int64 * AMTime, __int64 * AATime, __int64 * ASize,
  bool TryWriteReadOnly)
{
  uintptr_t LocalFileAttrs = 0;
  HANDLE LocalFileHandle = 0;
  TFileOperationProgressType * OperationProgress = GetOperationProgress();

  FILE_OPERATION_LOOP (FMTLOAD(FILE_NOT_EXISTS, FileName.c_str()),
    LocalFileAttrs = GetLocalFileAttributes(FileName);
    if (LocalFileAttrs == -1)
    {
      RaiseLastOSError();
    }
  )

  if ((LocalFileAttrs & faDirectory) == 0)
  {
    bool NoHandle = false;
    if (!TryWriteReadOnly && (Access == GENERIC_WRITE) &&
        ((LocalFileAttrs & faReadOnly) != 0))
    {
      Access = GENERIC_READ;
      NoHandle = true;
    }

    FILE_OPERATION_LOOP (FMTLOAD(OPENFILE_ERROR, FileName.c_str()),
      LocalFileHandle = CreateLocalFile(FileName.c_str(), (DWORD)Access,
        Access == GENERIC_READ ? FILE_SHARE_READ | FILE_SHARE_WRITE : FILE_SHARE_READ,
        OPEN_EXISTING, 0);
      if (LocalFileHandle == INVALID_HANDLE_VALUE)
      {
        LocalFileHandle = 0;
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
          if (!GetFileTime(LocalFileHandle, &CTime, &ATime, &MTime))
          {
            RaiseLastOSError();
          }
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
          LSize = GetFileSize(LocalFileHandle, &HSize);
          if ((LSize == 0xFFFFFFFF) && (GetLastError() != NO_ERROR))
          {
            RaiseLastOSError();
          }
          *ASize = ((__int64)(HSize) << 32) + LSize;
        );
      }

      if ((AHandle == NULL) || NoHandle)
      {
        CloseHandle(LocalFileHandle);
        LocalFileHandle = NULL;
      }
    }
    catch(...)
    {
      CloseHandle(LocalFileHandle);
      throw;
    }
  }

  if (AAttrs) { *AAttrs = LocalFileAttrs; }
  if (AHandle) { *AHandle = LocalFileHandle; }
}
//------------------------------------------------------------------------------
bool TTerminal::AllowLocalFileTransfer(const UnicodeString & FileName,
  const TCopyParamType * CopyParam)
{
  bool Result = true;
  TFileOperationProgressType * OperationProgress = GetOperationProgress();
  if (!CopyParam->AllowAnyTransfer())
  {
    WIN32_FIND_DATA FindData = {};
    HANDLE Handle = 0;
    FILE_OPERATION_LOOP (FMTLOAD(FILE_NOT_EXISTS, FileName.c_str()),
      Handle = FindFirstFile(FileName.c_str(), &FindData);
      if (Handle == INVALID_HANDLE_VALUE)
      {
        RaiseLastOSError();
      }
    )
    ::FindClose(Handle);
    bool Directory = FLAGSET(FindData.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY);
    TFileMasks::TParams Params;
    // SearchRec.Size in C++B2010 is __int64,
    // so we should be able to use it instead of FindData.nFileSize*
    Params.Size =
      (static_cast<__int64>(FindData.nFileSizeHigh) << 32) +
      FindData.nFileSizeLow;
    Params.Modification = FileTimeToDateTime(FindData.ftLastWriteTime);
    Result = CopyParam->AllowTransfer(FileName, osLocal, Directory, Params);
  }
  return Result;
}
//------------------------------------------------------------------------------
UnicodeString TTerminal::FileUrl(const UnicodeString & Protocol,
  const UnicodeString & FileName)
{
  assert(FileName.Length() > 0);
  return Protocol + L"://" + EncodeUrlChars(GetSessionData()->GetSessionName()) +
    (FileName[1] == L'/' ? L"" : L"/") + EncodeUrlChars(FileName, L"/");
}
//------------------------------------------------------------------------------
UnicodeString TTerminal::FileUrl(const UnicodeString & FileName)
{
  return FFileSystem->FileUrl(FileName);
}
//------------------------------------------------------------------------------
void TTerminal::MakeLocalFileList(const UnicodeString & FileName,
  const TSearchRec & Rec, void * Param)
{
  TMakeLocalFileListParams & Params = *static_cast<TMakeLocalFileListParams *>(Param);

  bool Directory = FLAGSET(Rec.Attr, faDirectory);
  if (Directory && Params.Recursive)
  {
    ProcessLocalDirectory(FileName, MAKE_CALLBACK(TTerminal::MakeLocalFileList, this), &Params);
  }

  if (!Directory || Params.IncludeDirs)
  {
    Params.FileList->Add(FileName);
  }
}
//------------------------------------------------------------------------------
void TTerminal::CalculateLocalFileSize(const UnicodeString & FileName,
  const TSearchRec & Rec, /*__int64*/ void * Params)
{
  TCalculateSizeParams * AParams = static_cast<TCalculateSizeParams*>(Params);

  bool Dir = FLAGSET(Rec.Attr, faDirectory);

  bool AllowTransfer = (AParams->CopyParam == NULL);
  // SearchRec.Size in C++B2010 is __int64,
  // so we should be able to use it instead of FindData.nFileSize*
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
      ProcessLocalDirectory(FileName, MAKE_CALLBACK(TTerminal::CalculateLocalFileSize, this), Params);
    }
  }

  if (GetOperationProgress() && GetOperationProgress()->Operation == foCalculateSize)
  {
    if (GetOperationProgress()->Cancel != csContinue) { Abort(); }
    GetOperationProgress()->SetFile(FileName);
  }
}
//------------------------------------------------------------------------------
void TTerminal::CalculateLocalFilesSize(TStrings * FileList,
  __int64 & Size, const TCopyParamType * CopyParam)
{
  TFileOperationProgressType OperationProgress(MAKE_CALLBACK(TTerminal::DoProgress, this), MAKE_CALLBACK(TTerminal::DoFinished, this));
  TOnceDoneOperation OnceDoneOperation = odoIdle;
  OperationProgress.Start(foCalculateSize, osLocal, FileList->GetCount());
  TRY_FINALLY ( //-V506
  {
    TCalculateSizeParams Params;
    Params.Size = 0;
    Params.Params = 0;
    Params.CopyParam = CopyParam;

    assert(!FOperationProgress);
    FOperationProgress = &OperationProgress;
    TSearchRec Rec;
    for (intptr_t Index = 0; Index < FileList->GetCount(); ++Index)
    {
      UnicodeString FileName = FileList->GetString(Index);
      if (FileSearchRec(FileName, Rec))
      {
        CalculateLocalFileSize(FileName, Rec, &Params);
        OperationProgress.Finish(FileName, true, OnceDoneOperation);
      }
    }

    Size = Params.Size;
  }
  ,
  {
    FOperationProgress = NULL;
    OperationProgress.Stop();
  }
  );

  if (OnceDoneOperation != odoIdle)
  {
    CloseOnCompletion(OnceDoneOperation);
  }
}
//------------------------------------------------------------------------------
struct TSynchronizeFileData : public TObject
{
  bool Modified;
  bool New;
  bool IsDirectory;
  TSynchronizeChecklist::TItem::TFileInfo Info;
  TSynchronizeChecklist::TItem::TFileInfo MatchingRemoteFile;
  TRemoteFile * MatchingRemoteFileFile;
  intptr_t MatchingRemoteFileImageIndex;
  FILETIME LocalLastWriteTime;
};
//------------------------------------------------------------------------------
const int sfFirstLevel = 0x01;
struct TSynchronizeData : public TObject
{
  UnicodeString LocalDirectory;
  UnicodeString RemoteDirectory;
  TTerminal::TSynchronizeMode Mode;
  intptr_t Params;
  TSynchronizeDirectoryEvent OnSynchronizeDirectory;
  TSynchronizeOptions * Options;
  int Flags;
  TStringList * LocalFileList;
  const TCopyParamType * CopyParam;
  TSynchronizeChecklist * Checklist;
};
//------------------------------------------------------------------------------
TSynchronizeChecklist * TTerminal::SynchronizeCollect(const UnicodeString & LocalDirectory,
  const UnicodeString & RemoteDirectory, TSynchronizeMode Mode,
  const TCopyParamType * CopyParam, intptr_t Params,
  TSynchronizeDirectoryEvent OnSynchronizeDirectory,
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
static void AddFlagName(UnicodeString & ParamsStr, intptr_t & Params, intptr_t Param, const UnicodeString & Name)
{
  if (FLAGSET(Params, Param))
  {
    AddToList(ParamsStr, Name, ", ");
  }
  Params &= ~Param;
}
//---------------------------------------------------------------------------
UnicodeString TTerminal::SynchronizeModeStr(TSynchronizeMode Mode)
{
  UnicodeString ModeStr;
  switch (Mode)
  {
    case smRemote:
      ModeStr = L"Remote";
      break;
    case smLocal:
      ModeStr = L"Local";
      break;
    case smBoth:
      ModeStr = L"Both";
      break;
    default:
      ModeStr = L"Unknown";
      break;
  }
  return ModeStr;
}
//---------------------------------------------------------------------------
UnicodeString TTerminal::SynchronizeParamsStr(intptr_t Params)
{
  UnicodeString ParamsStr;
  AddFlagName(ParamsStr, Params, spDelete, L"Delete");
  AddFlagName(ParamsStr, Params, spNoConfirmation, L"NoConfirmation");
  AddFlagName(ParamsStr, Params, spExistingOnly, L"ExistingOnly");
  AddFlagName(ParamsStr, Params, spNoRecurse, L"NoRecurse");
  AddFlagName(ParamsStr, Params, spUseCache, L"UseCache");
  AddFlagName(ParamsStr, Params, spDelayProgress, L"DelayProgress");
  AddFlagName(ParamsStr, Params, spPreviewChanges, L"*PreviewChanges"); // GUI only
  AddFlagName(ParamsStr, Params, spTimestamp, L"Timestamp");
  AddFlagName(ParamsStr, Params, spNotByTime, L"NotByTime");
  AddFlagName(ParamsStr, Params, spBySize, L"BySize");
  AddFlagName(ParamsStr, Params, spSelectedOnly, L"*SelectedOnly"); // GUI only
  AddFlagName(ParamsStr, Params, spMirror, L"Mirror");
  if (Params > 0)
  {
    AddToList(ParamsStr, FORMAT(L"0x%x", (int(Params))), L", ");
  }
  return ParamsStr;
}
//------------------------------------------------------------------------------
void TTerminal::DoSynchronizeCollectDirectory(const UnicodeString & LocalDirectory,
  const UnicodeString & RemoteDirectory, TSynchronizeMode Mode,
  const TCopyParamType * CopyParam, intptr_t Params,
  TSynchronizeDirectoryEvent OnSynchronizeDirectory, TSynchronizeOptions * Options,
  int Flags, TSynchronizeChecklist * Checklist)
{
  TFileOperationProgressType * OperationProgress = GetOperationProgress();
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
    L"mode = %s, params = 0x%x (%s)", LocalDirectory.c_str(), RemoteDirectory.c_str(),
    SynchronizeModeStr(Mode).c_str(), int(Params), SynchronizeParamsStr(Params).c_str()));

  if (FLAGCLEAR(Params, spDelayProgress))
  {
    DoSynchronizeProgress(Data, true);
  }

  TRY_FINALLY (
  {
    bool Found = false;
    TSearchRec SearchRec;
    Data.LocalFileList = new TStringList();
    Data.LocalFileList->SetSorted(true);
    Data.LocalFileList->SetCaseSensitive(false);

    FILE_OPERATION_LOOP (FMTLOAD(LIST_DIR_ERROR, LocalDirectory.c_str()),
      int FindAttrs = faReadOnly | faHidden | faSysFile | faDirectory | faArchive;
      Found = (FindFirstChecked(Data.LocalDirectory + L"*.*", FindAttrs, SearchRec) == 0);
    );

    if (Found)
    {
      TRY_FINALLY (
      {
        UnicodeString FileName;
        while (Found)
        {
          FileName = SearchRec.Name;
          // add dirs for recursive mode or when we are interested in newly
          // added subdirs
          // SearchRec.Size in C++B2010 is __int64,
          // so we should be able to use it instead of FindData.nFileSize*
          __int64 Size =
            (static_cast<__int64>(SearchRec.FindData.nFileSizeHigh) << 32) +
            SearchRec.FindData.nFileSizeLow;
          TDateTime Modification = FileTimeToDateTime(SearchRec.FindData.ftLastWriteTime);
          TFileMasks::TParams MaskParams;
          MaskParams.Size = Size;
          MaskParams.Modification = Modification;
          UnicodeString RemoteFileName =
            CopyParam->ChangeFileName(FileName, osLocal, false);
          UnicodeString FullLocalFileName = Data.LocalDirectory + FileName;
          if ((FileName != THISDIRECTORY) && (FileName != PARENTDIRECTORY) &&
              CopyParam->AllowTransfer(FullLocalFileName, osLocal,
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
            LogEvent(FORMAT(L"Local file '%s' [%s] [%s] included to synchronization",
              FullLocalFileName.c_str(), StandardTimestamp(Modification).c_str(), Int64ToStr(Size).c_str()));
          }
          else
          {
            LogEvent(FORMAT(L"Local file '%s' [%s] [%s] excluded from synchronization",
              FullLocalFileName.c_str(), StandardTimestamp(Modification).c_str(), Int64ToStr(Size).c_str()));
          }

          FILE_OPERATION_LOOP (FMTLOAD(LIST_DIR_ERROR, LocalDirectory.c_str()),
            Found = (FindNextChecked(SearchRec) == 0);
          );
        }
      }
      ,
      {
        FindClose(SearchRec);
      }
      );

      // can we expect that ProcessDirectory would take so little time
      // that we can pospone showing progress window until anything actually happens?
      bool Cached = FLAGSET(Params, spUseCache) && GetSessionData()->GetCacheDirectories() &&
        FDirectoryCache->HasFileList(RemoteDirectory);

      if (!Cached && FLAGSET(Params, spDelayProgress))
      {
        DoSynchronizeProgress(Data, true);
      }

      ProcessDirectory(RemoteDirectory, MAKE_CALLBACK(TTerminal::SynchronizeCollectFile, this), &Data,
        FLAGSET(Params, spUseCache));

      TSynchronizeFileData * FileData;
      for (intptr_t Index = 0; Index < Data.LocalFileList->GetCount(); ++Index)
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

        if (New)
        {
          LogEvent(FORMAT(L"Local file '%s' [%s] [%s] is new",
            UnicodeString(FileData->Info.Directory + FileData->Info.FileName).c_str(),
             StandardTimestamp(FileData->Info.Modification).c_str(),
             Int64ToStr(FileData->Info.Size).c_str()));
        }

        if (Modified || New)
        {
          TSynchronizeChecklist::TItem * ChecklistItem = new TSynchronizeChecklist::TItem();
          TRY_FINALLY (
          {
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
          ,
          {
            delete ChecklistItem;
          }
          );
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
  ,
  {
    if (Data.LocalFileList != NULL)
    {
      for (intptr_t Index = 0; Index < Data.LocalFileList->GetCount(); ++Index)
      {
        TSynchronizeFileData * FileData = reinterpret_cast<TSynchronizeFileData *>
          (Data.LocalFileList->GetObject(Index));
        delete FileData;
      }
      delete Data.LocalFileList;
    }
  }
  );
}
//------------------------------------------------------------------------------
void TTerminal::SynchronizeCollectFile(const UnicodeString & FileName,
  const TRemoteFile * File, /*TSynchronizeData*/ void * Param)
{
  TSynchronizeData * Data = static_cast<TSynchronizeData *>(Param);

  TFileMasks::TParams MaskParams;
  MaskParams.Size = File->GetSize();
  MaskParams.Modification = File->GetModification();
  UnicodeString LocalFileName =
    Data->CopyParam->ChangeFileName(File->GetFileName(), osRemote, false);
  UnicodeString FullRemoteFileName =
    UnixExcludeTrailingBackslash(File->GetFullFileName());
  if (Data->CopyParam->AllowTransfer(
        FullRemoteFileName, osRemote,
        File->GetIsDirectory(), MaskParams) &&
      !FFileSystem->TemporaryTransferFile(File->GetFileName()) &&
      (FLAGCLEAR(Data->Flags, sfFirstLevel) ||
       (Data->Options == NULL) ||
        Data->Options->MatchesFilter(File->GetFileName()) ||
        Data->Options->MatchesFilter(LocalFileName)))
  {
    TSynchronizeChecklist::TItem * ChecklistItem = new TSynchronizeChecklist::TItem();
    TRY_FINALLY (
    {
      ChecklistItem->IsDirectory = File->GetIsDirectory();
      ChecklistItem->ImageIndex = File->GetIconIndex();

      ChecklistItem->Remote.FileName = File->GetFileName();
      ChecklistItem->Remote.Directory = Data->RemoteDirectory;
      ChecklistItem->Remote.Modification = File->GetModification();
      ChecklistItem->Remote.ModificationFmt = File->GetModificationFmt();
      ChecklistItem->Remote.Size = File->GetSize();

      bool Modified = false;
      intptr_t LocalIndex = Data->LocalFileList->IndexOf(LocalFileName.c_str());
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
          intptr_t TimeCompare;
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
            LogEvent(FORMAT(L"Local file '%s' [%s] [%s] is modified comparing to remote file '%s' [%s] [%s]",
              UnicodeString(LocalData->Info.Directory + LocalData->Info.FileName).c_str(),
               StandardTimestamp(LocalData->Info.Modification).c_str(),
               Int64ToStr(LocalData->Info.Size).c_str(),
               FullRemoteFileName.c_str(),
               StandardTimestamp(File->GetModification()).c_str(),
               Int64ToStr(File->GetSize()).c_str()));
          }

          if (Modified)
          {
            LogEvent(FORMAT(L"Remote file '%s' [%s] [%s] is modified comparing to local file '%s' [%s] [%s]",
              FullRemoteFileName.c_str(),
               StandardTimestamp(File->GetModification()).c_str(),
               Int64ToStr(File->GetSize()).c_str(),
               UnicodeString(LocalData->Info.Directory + LocalData->Info.FileName).c_str(),
               StandardTimestamp(LocalData->Info.Modification).c_str(),
               Int64ToStr(LocalData->Info.Size).c_str()));
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
        LogEvent(FORMAT(L"Remote file '%s' [%s] [%s] is new",
          FullRemoteFileName.c_str(), StandardTimestamp(File->GetModification()).c_str(), Int64ToStr(File->GetSize()).c_str()));
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
    ,
    {
      delete ChecklistItem;
    }
    );
  }
  else
  {
    LogEvent(FORMAT(L"Remote file '%s' [%s] [%s] excluded from synchronization",
      FullRemoteFileName.c_str(), StandardTimestamp(File->GetModification()).c_str(), Int64ToStr(File->GetSize()).c_str()));
  }
}
//------------------------------------------------------------------------------
void TTerminal::SynchronizeApply(TSynchronizeChecklist * Checklist,
  const UnicodeString & LocalDirectory, const UnicodeString & RemoteDirectory,
  const TCopyParamType * CopyParam, intptr_t Params,
  TSynchronizeDirectoryEvent OnSynchronizeDirectory)
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

  TRY_FINALLY (
  {
    intptr_t IIndex = 0;
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
        L"params = 0x%x (%s)", CurrentLocalDirectory.c_str(), CurrentRemoteDirectory.c_str(),
        int(Params), SynchronizeParamsStr(Params).c_str()));

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
                  static_cast<TObject *>(const_cast<TSynchronizeChecklist::TItem *>(ChecklistItem)));
                break;

              case TSynchronizeChecklist::saUploadUpdate:
                UploadList->AddObject(
                  IncludeTrailingBackslash(ChecklistItem->Local.Directory) +
                    ChecklistItem->Local.FileName,
                  static_cast<TObject *>(const_cast<TSynchronizeChecklist::TItem *>(ChecklistItem)));
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
        ++IIndex;
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
              MAKE_CALLBACK(TTerminal::SynchronizeLocalTimestamp, this), NULL, osLocal);
          }

          if (UploadList->GetCount() > 0)
          {
            ProcessFiles(UploadList, foSetProperties,
              MAKE_CALLBACK(TTerminal::SynchronizeRemoteTimestamp, this));
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
  ,
  {
    delete DownloadList;
    delete DeleteRemoteList;
    delete UploadList;
    delete DeleteLocalList;

    EndTransaction();
  }
  );
}
//------------------------------------------------------------------------------
void TTerminal::DoSynchronizeProgress(const TSynchronizeData & Data,
  bool Collect)
{
  if (Data.OnSynchronizeDirectory)
  {
    bool Continue = true;
    Data.OnSynchronizeDirectory(Data.LocalDirectory, Data.RemoteDirectory,
      Continue, Collect);

    if (!Continue)
    {
      Abort();
    }
  }
}
//------------------------------------------------------------------------------
void TTerminal::SynchronizeLocalTimestamp(const UnicodeString & /*FileName*/,
  const TRemoteFile * File, void * /*Param*/)
{
  const TSynchronizeChecklist::TItem * ChecklistItem =
    reinterpret_cast<const TSynchronizeChecklist::TItem *>(File);

  UnicodeString LocalFile =
    IncludeTrailingBackslash(ChecklistItem->Local.Directory) +
      ChecklistItem->Local.FileName;
  SetLocalFileTime(LocalFile, ChecklistItem->Remote.Modification);
}
//------------------------------------------------------------------------------
void TTerminal::SynchronizeRemoteTimestamp(const UnicodeString & /*FileName*/,
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
//------------------------------------------------------------------------------
void TTerminal::FileFind(const UnicodeString & FileName,
  const TRemoteFile * File, /*TFilesFindParams*/ void * Param)
{
  // see DoFilesFind
  FOnFindingFile = NULL;

  assert(Param);
  assert(File);
  TFilesFindParams * AParams = static_cast<TFilesFindParams *>(Param);

  if (!AParams->Cancel)
  {
    UnicodeString LocalFileName = FileName;
    if (FileName.IsEmpty())
    {
      LocalFileName = File->GetFileName();
    }

    TFileMasks::TParams MaskParams;
    MaskParams.Size = File->GetSize();
    MaskParams.Modification = File->GetModification();

    UnicodeString FullFileName = UnixExcludeTrailingBackslash(File->GetFullFileName());
    bool ImplicitMatch = false;
    if (AParams->FileMask.Matches(FullFileName, false,
         File->GetIsDirectory(), &MaskParams, ImplicitMatch))
    {
      if (!ImplicitMatch)
      {
        AParams->OnFileFound(this, LocalFileName, File, AParams->Cancel);
      }

      if (File->GetIsDirectory())
      {
        DoFilesFind(FullFileName, *AParams);
      }
    }
  }
}
//------------------------------------------------------------------------------
void TTerminal::DoFilesFind(const UnicodeString & Directory, TFilesFindParams & Params)
{
  Params.OnFindingFile(this, Directory, Params.Cancel);
  if (!Params.Cancel)
  {
    assert(FOnFindingFile == NULL);
    // ideally we should set the handler only around actually reading
    // of the directory listing, so we at least reset the handler in
    // FileFind
    FOnFindingFile = Params.OnFindingFile;
    TRY_FINALLY (
    {
      ProcessDirectory(Directory, MAKE_CALLBACK(TTerminal::FileFind, this), &Params, false, true);
    }
    ,
    {
      FOnFindingFile = NULL;
    }
    );
  }
}
//------------------------------------------------------------------------------
void TTerminal::FilesFind(const UnicodeString & Directory, const TFileMasks & FileMask,
  TFileFoundEvent OnFileFound, TFindingFileEvent OnFindingFile)
{
  TFilesFindParams Params;
  Params.FileMask = FileMask;
  Params.OnFileFound = OnFileFound;
  Params.OnFindingFile = OnFindingFile;
  Params.Cancel = false;
  DoFilesFind(Directory, Params);
}
//------------------------------------------------------------------------------
void TTerminal::SpaceAvailable(const UnicodeString & Path,
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
//------------------------------------------------------------------------------
const TSessionInfo & TTerminal::GetSessionInfo() const
{
  return FFileSystem->GetSessionInfo();
}
//------------------------------------------------------------------------------
const TFileSystemInfo & TTerminal::GetFileSystemInfo(bool Retrieve)
{
  return FFileSystem->GetFileSystemInfo(Retrieve);
}
//---------------------------------------------------------------------
UnicodeString TTerminal::GetPassword()
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
UnicodeString TTerminal::GetTunnelPassword()
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
//------------------------------------------------------------------------------
bool TTerminal::CopyToRemote(TStrings * FilesToCopy,
  const UnicodeString & TargetDir, const TCopyParamType * CopyParam, intptr_t Params)
{
  assert(FFileSystem);
  assert(FilesToCopy);


  bool Result = false;
  TOnceDoneOperation OnceDoneOperation = odoIdle;

  TFileOperationProgressType OperationProgress(MAKE_CALLBACK(TTerminal::DoProgress, this), MAKE_CALLBACK(TTerminal::DoFinished, this));
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

    FOperationProgress = &OperationProgress; //-V506
    TRY_FINALLY (
    {
      if (CopyParam->GetCalculateSize())
      {
        OperationProgress.SetTotalSize(Size);
      }

      UnicodeString UnlockedTargetDir = TranslateLockedPath(TargetDir, false);
      BeginTransaction();
      TRY_FINALLY (
      {
        if (GetLog()->GetLogging())
        {
          LogEvent(FORMAT(L"Copying %d files/directories to remote directory "
            L"\"%s\"", FilesToCopy->GetCount(), TargetDir.c_str()));
          LogEvent(CopyParam->GetLogStr());
        }

        FFileSystem->CopyToRemote(FilesToCopy, UnlockedTargetDir,
          CopyParam, Params, &OperationProgress, OnceDoneOperation);
      }
      ,
      {
        if (GetActive())
        {
          ReactOnCommand(fsCopyToRemote);
        }
        EndTransaction();
      }
      );

      if (OperationProgress.Cancel == csContinue)
      {
        Result = true;
      }
    }
    ,
    {
      OperationProgress.Stop();
      FOperationProgress = NULL;
    }
    );
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
//------------------------------------------------------------------------------
bool TTerminal::CopyToLocal(TStrings * FilesToCopy,
  const UnicodeString & TargetDir, const TCopyParamType * CopyParam, intptr_t Params)
{
  assert(FFileSystem);

  // see scp.c: sink(), tolocal()

  bool Result = false;
  bool OwnsFileList = (FilesToCopy == NULL);
  TOnceDoneOperation OnceDoneOperation = odoIdle;

  TRY_FINALLY ( //-V506
  {
    if (OwnsFileList)
    {
      FilesToCopy = new TStringList();
      FilesToCopy->Assign(GetFiles()->GetSelectedFiles());
    }

    BeginTransaction();
    TRY_FINALLY (
    {
      __int64 TotalSize = 0;
      bool TotalSizeKnown = false;
      TFileOperationProgressType OperationProgress(MAKE_CALLBACK(TTerminal::DoProgress, this), MAKE_CALLBACK(TTerminal::DoFinished, this));

      if (CopyParam->GetCalculateSize())
      {
        SetExceptionOnFail(true);
        TRY_FINALLY (
        {
          // dirty trick: when moving, do not pass copy param to avoid exclude mask
          CalculateFilesSize(FilesToCopy, TotalSize, csIgnoreErrors,
            (FLAGCLEAR(Params, cpDelete) ? CopyParam : NULL));
          TotalSizeKnown = true;
        }
        ,
        {
          SetExceptionOnFail(false);
        }
        );
      }

      OperationProgress.Start(((Params & cpDelete) != 0 ? foMove : foCopy), osRemote,
        FilesToCopy->GetCount(), (Params & cpTemporary) != 0, TargetDir, CopyParam->GetCPSLimit());

      FOperationProgress = &OperationProgress;
      TRY_FINALLY (
      {
        if (TotalSizeKnown)
        {
          OperationProgress.SetTotalSize(TotalSize);
        }

        try
        {
          TRY_FINALLY (
          {
            FFileSystem->CopyToLocal(FilesToCopy, TargetDir, CopyParam, Params,
              &OperationProgress, OnceDoneOperation);
          }
          ,
          {
            if (GetActive())
            {
              ReactOnCommand(fsCopyToLocal);
            }
          }
          );
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
      ,
      {
        FOperationProgress = NULL;
        OperationProgress.Stop();
      }
      );
    }
    ,
    {
      // If session is still active (no fatal error) we reload directory
      // by calling EndTransaction
      EndTransaction();
    }
    );

  }
  ,
  {
    if (OwnsFileList) delete FilesToCopy;
  }
  );

  if (OnceDoneOperation != odoIdle)
  {
    CloseOnCompletion(OnceDoneOperation);
  }

  return Result;
}
//------------------------------------------------------------------------------
void TTerminal::SetLocalFileTime(const UnicodeString & LocalFileName,
  const TDateTime & Modification)
{
  FILETIME WrTime = DateTimeToFileTime(Modification,
    GetSessionData()->GetDSTMode());
  SetLocalFileTime(LocalFileName, NULL, &WrTime);
}
//------------------------------------------------------------------------------
void TTerminal::SetLocalFileTime(const UnicodeString & LocalFileName,
  FILETIME * AcTime, FILETIME * WrTime)
{
  TFileOperationProgressType * OperationProgress = GetOperationProgress();
  FILE_OPERATION_LOOP (FMTLOAD(CANT_SET_ATTRS, LocalFileName.c_str()),
    HANDLE Handle;
    OpenLocalFile(LocalFileName, GENERIC_WRITE, NULL, &Handle,
      NULL, NULL, NULL, NULL);
    bool Result = ::SetFileTime(Handle, NULL, AcTime, WrTime) > 0;
    CloseHandle(Handle);
    if (!Result)
    {
      Abort();
    }
  );
}
//------------------------------------------------------------------------------
HANDLE TTerminal::CreateLocalFile(const UnicodeString & LocalFileName, DWORD DesiredAccess,
  DWORD ShareMode, DWORD CreationDisposition, DWORD FlagsAndAttributes)
{
  if (GetOnCreateLocalFile())
  {
    return GetOnCreateLocalFile()(LocalFileName, DesiredAccess, ShareMode, CreationDisposition, FlagsAndAttributes);
  }
  else
  {
    return ::CreateFile(LocalFileName.c_str(), DesiredAccess, ShareMode, NULL, CreationDisposition, FlagsAndAttributes, 0);
  }
}
//------------------------------------------------------------------------------
DWORD TTerminal::GetLocalFileAttributes(const UnicodeString & LocalFileName)
{
  if (GetOnGetLocalFileAttributes())
  {
    return GetOnGetLocalFileAttributes()(LocalFileName);
  }
  else
  {
    return ::GetFileAttributes(LocalFileName.c_str());
  }
}
//------------------------------------------------------------------------------
BOOL TTerminal::SetLocalFileAttributes(const UnicodeString & LocalFileName, DWORD FileAttributes)
{
  if (GetOnSetLocalFileAttributes())
  {
    return GetOnSetLocalFileAttributes()(LocalFileName, FileAttributes);
  }
  else
  {
    return ::SetFileAttributes(LocalFileName.c_str(), FileAttributes);
  }
}
//------------------------------------------------------------------------------
BOOL TTerminal::MoveLocalFile(const UnicodeString & LocalFileName, const UnicodeString & NewLocalFileName, DWORD Flags)
{
  if (GetOnMoveLocalFile())
  {
    return GetOnMoveLocalFile()(LocalFileName, NewLocalFileName, Flags);
  }
  else
  {
    return ::MoveFileEx(LocalFileName.c_str(), NewLocalFileName.c_str(), Flags) != 0;
  }
}
//------------------------------------------------------------------------------
BOOL TTerminal::RemoveLocalDirectory(const UnicodeString & LocalDirName)
{
  if (GetOnRemoveLocalDirectory())
  {
    return GetOnRemoveLocalDirectory()(LocalDirName);
  }
  else
  {
    return ::RemoveDirectory(LocalDirName) != 0;
  }
}
//------------------------------------------------------------------------------
BOOL TTerminal::CreateLocalDirectory(const UnicodeString & LocalDirName, LPSECURITY_ATTRIBUTES SecurityAttributes)
{
  if (GetOnCreateLocalDirectory())
  {
    return GetOnCreateLocalDirectory()(LocalDirName, SecurityAttributes);
  }
  else
  {
    return ::CreateDirectory(LocalDirName.c_str(), SecurityAttributes) != 0;
  }
}
//---------------------------------------------------------------------------
void TTerminal::ReflectSettings()
{
  assert(FLog != NULL);
  FLog->ReflectSettings();
  assert(FActionLog != NULL);
  FActionLog->ReflectSettings();
  // also FTunnelLog ?
}
//---------------------------------------------------------------------------
void TTerminal::EnableUsage()
{
  FEnableSecureShellUsage = true;
}
//---------------------------------------------------------------------------
bool TTerminal::CheckForEsc()
{
  if (FOnCheckForEsc)
    return FOnCheckForEsc();
  else
    return (FOperationProgress && FOperationProgress->Cancel == csCancel);
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
TSecondaryTerminal::TSecondaryTerminal(TTerminal * MainTerminal) :
  TTerminal(),
  FMasterPasswordTried(false),
  FMasterTunnelPasswordTried(false),
  FMainTerminal(MainTerminal)
{
}

void TSecondaryTerminal::Init(
  TSessionData * ASessionData, TConfiguration * AConfiguration, const UnicodeString & Name)
{
  TTerminal::Init(ASessionData, AConfiguration);
  assert(FMainTerminal != NULL);
  GetLog()->SetParent(FMainTerminal->GetLog());
  GetLog()->SetName(Name);
  GetActionLog()->SetEnabled(false);
  GetSessionData()->NonPersistant();
  if (!FMainTerminal->GetUserName().IsEmpty())
  {
    GetSessionData()->SetUserName(FMainTerminal->GetUserName());
  }
}
//------------------------------------------------------------------------------
void TSecondaryTerminal::DirectoryLoaded(TRemoteFileList * FileList)
{
  FMainTerminal->DirectoryLoaded(FileList);
  assert(FileList != NULL);
}
//------------------------------------------------------------------------------
void TSecondaryTerminal::DirectoryModified(const UnicodeString & Path,
  bool SubDirs)
{
  // clear cache of main terminal
  FMainTerminal->DirectoryModified(Path, SubDirs);
}
//------------------------------------------------------------------------------
bool TSecondaryTerminal::DoPromptUser(TSessionData * Data,
  TPromptKind Kind, const UnicodeString & Name, const UnicodeString & Instructions, TStrings * Prompts,
  TStrings * Results)
{
  bool AResult = false;

  if ((Prompts->GetCount() == 1) && FLAGCLEAR(int(Prompts->GetObject(0)), pupEcho) &&
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
      Results->SetString(0, Password);
      if (!Results->GetString(0).IsEmpty())
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
//------------------------------------------------------------------------------
TTerminalList::TTerminalList(TConfiguration * AConfiguration) :
  TObjectList(),
  FConfiguration(NULL)
{
  assert(AConfiguration);
  FConfiguration = AConfiguration;
}
//------------------------------------------------------------------------------
TTerminalList::~TTerminalList()
{
  assert(GetCount() == 0);
}
//------------------------------------------------------------------------------
TTerminal * TTerminalList::CreateTerminal(TSessionData * Data)
{
  TTerminal * Result = new TTerminal();
  Result->Init(Data, FConfiguration);
  return Result;
}
//------------------------------------------------------------------------------
TTerminal * TTerminalList::NewTerminal(TSessionData * Data)
{
  TTerminal * Terminal = CreateTerminal(Data);
  Add(Terminal);
  return Terminal;
}
//------------------------------------------------------------------------------
void TTerminalList::FreeTerminal(TTerminal * Terminal)
{
  assert(IndexOf(Terminal) >= 0);
  Remove(Terminal);
}
//------------------------------------------------------------------------------
void TTerminalList::FreeAndNullTerminal(TTerminal * & Terminal)
{
  TTerminal * T = Terminal;
  Terminal = NULL;
  FreeTerminal(T);
}
//------------------------------------------------------------------------------
TTerminal * TTerminalList::GetTerminal(intptr_t Index)
{
  return dynamic_cast<TTerminal *>(GetItem(Index));
}
//------------------------------------------------------------------------------
void TTerminalList::Idle()
{
  for (intptr_t I = 0; I < GetCount(); ++I)
  {
    TTerminal * Terminal = GetTerminal(I);
    if (Terminal->GetStatus() == ssOpened)
    {
      Terminal->Idle();
    }
  }
}
//------------------------------------------------------------------------------
void TTerminalList::RecryptPasswords()
{
  for (intptr_t Index = 0; Index < GetCount(); ++Index)
  {
    GetTerminal(Index)->RecryptPasswords();
  }
}
//------------------------------------------------------------------------------
UnicodeString GetSessionUrl(const TTerminal * Terminal)
{
  const TSessionInfo & SessionInfo = Terminal->GetSessionInfo() ;
  UnicodeString Protocol = SessionInfo.ProtocolBaseName;
  UnicodeString HostName = Terminal->GetSessionData()->GetHostNameExpanded();
  intptr_t Port = Terminal->GetSessionData()->GetPortNumber();
  UnicodeString SessionUrl = FORMAT(L"%s://%s:%d", Protocol.Lower().c_str(), HostName.c_str(), Port);
  return SessionUrl;
}
//------------------------------------------------------------------------------
