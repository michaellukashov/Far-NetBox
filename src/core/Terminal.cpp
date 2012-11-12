//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#define TRACE_SYNCH TRACING

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
//---------------------------------------------------------------------------
#pragma package(smart_init)
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
  FILE_OPERATION_LOOP_CUSTOM(this, ALLOW_SKIP, MESSAGE, OPERATION)
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
  TFileFoundEvent OnFileFound;
  TFindingFileEvent OnFindingFile;
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
  bool Result = false;
  if (Filter == NULL)
  {
    Result = true;
  }
  else
  {
    int FoundIndex = 0;
    Result = Filter->Find(FileName, FoundIndex);
    TRACEFMT("[%s] [%d]", (FileName, int(Result)));
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
  Action(saNone), IsDirectory(false), ImageIndex(-1), Checked(true), RemoteFile(NULL)
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
  for (int Index = 0; Index < FList->Count; Index++)
  {
    delete static_cast<TItem *>(static_cast<void *>(FList->Items[Index]));
  }
  delete FList;
}
//---------------------------------------------------------------------------
void TSynchronizeChecklist::Add(TItem * Item)
{
  FList->Add(Item);
}
//---------------------------------------------------------------------------
int /* __fastcall */ TSynchronizeChecklist::Compare(const void * AItem1, const void * AItem2)
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
//---------------------------------------------------------------------------
void TSynchronizeChecklist::Sort()
{
  FList->Sort(Compare);
}
//---------------------------------------------------------------------------
int __fastcall TSynchronizeChecklist::GetCount() const
{
  return FList->Count;
}
//---------------------------------------------------------------------------
const TSynchronizeChecklist::TItem * __fastcall TSynchronizeChecklist::GetItem(int Index) const
{
  return static_cast<TItem *>(FList->Items[Index]);
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
};
//---------------------------------------------------------------------------
/* __fastcall */ TTunnelThread::TTunnelThread(TSecureShell * SecureShell) :
  TSimpleThread(),
  FSecureShell(SecureShell),
  FTerminated(false)
{
}
//---------------------------------------------------------------------------
void __fastcall TTunnelThread::Init()
{
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
  CALLSTACK;
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
  throw ESshFatal(E, Msg);
}
//---------------------------------------------------------------------------
void __fastcall TTunnelUI::HandleExtendedException(Exception * E)
{
  CALLSTACK;
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
  FFatalError(NULL),
  FTerminal(Terminal),
  FGuarding(FTerminal->FCallbackGuard == NULL)
{
  CALLSTACK;
  if (FGuarding)
  {
    TRACE("1");
    FTerminal->FCallbackGuard = this;
  }
}
//---------------------------------------------------------------------------
/* __fastcall */ TCallbackGuard::~TCallbackGuard()
{
  CALLSTACK;
  if (FGuarding)
  {
    TRACE("1");
    assert((FTerminal->FCallbackGuard == this) || (FTerminal->FCallbackGuard == NULL));
    FTerminal->FCallbackGuard = NULL;
  }

  delete FFatalError;
}
//---------------------------------------------------------------------------
void __fastcall TCallbackGuard::FatalError(Exception * E, const UnicodeString & Msg)
{
  CALLSTACK;
  assert(FGuarding);

  // make sure we do not bother about getting back the silent abort exception
  // we issued outselves. this may happen when there is an exception handler
  // that converts any exception to fatal one (such as in TTerminal::Open).
  if (dynamic_cast<ECallbackGuardAbort *>(E) == NULL)
  {
    TRACE("1");
    delete FFatalError;
    FFatalError = new ExtException(E, Msg);
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
  CALLSTACK;
  assert(FFatalError == NULL);
  FGuarding = false;
}
//---------------------------------------------------------------------------
void __fastcall TCallbackGuard::Verify()
{
  CALLSTACK;
  if (FGuarding)
  {
    FGuarding = false;
    assert(FTerminal->FCallbackGuard == this);
    FTerminal->FCallbackGuard = NULL;

    if (FFatalError != NULL)
    {
      TRACEFMT("1 [%s]", (FFatalError->Message));
      throw ESshFatal(FFatalError, L"");
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
  CALLSTACK;
  FConfiguration = Configuration;
  FSessionData = new TSessionData(L"");
  FSessionData->Assign(SessionData);
  FLog = new TSessionLog(this, FSessionData, Configuration);
  FActionLog = new TActionLog(this, FSessionData, Configuration);
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
  FSuspendTransaction = false;
  FOperationProgress = NULL;
  FClosedOnCompletion = NULL;
  FTunnel = NULL;
}
//---------------------------------------------------------------------------
/* __fastcall */ TTerminal::~TTerminal()
{
  CALLSTACK;
  if (GetActive())
  {
    TRACE("1");
    Close();
  }

  if (FCallbackGuard != NULL)
  {
    TRACE("2");
    // see TTerminal::HandleExtendedException
    FCallbackGuard->Dismiss();
  }
  assert(FTunnel == NULL);

  SAFE_DESTROY(FCommandSession);

  if (GetSessionData()->GetCacheDirectoryChanges() && GetSessionData()->GetPreserveDirectoryChanges() &&
      (FDirectoryChangesCache != NULL))
  {
    TRACE("3");
    Configuration->SaveDirectoryChangesCache(GetSessionData()->GetSessionKey(),
      FDirectoryChangesCache);
  }

  TRACE("4");
  SAFE_DESTROY_EX(TCustomFileSystem, FFileSystem);
  TRACE("5");
  SAFE_DESTROY_EX(TSessionLog, FLog);
  TRACE("5a");
  SAFE_DESTROY_EX(TActionLog, FActionLog);
  TRACE("6");
  delete FFiles;
  TRACE("7");
  delete FDirectoryCache;
  TRACE("8");
  delete FDirectoryChangesCache;
  TRACE("9");
  SAFE_DESTROY(FSessionData);
  TRACE("/");
}
//---------------------------------------------------------------------------
void __fastcall TTerminal::Idle()
{
  CALLSTACK;
  TRACE_EXCEPT_BEGIN
  // once we disconnect, do nothing, until reconnect handler
  // "receives the information"
  if (GetActive())
  {
    if (Configuration->GetActualLogProtocol() >= 1)
    {
      // LogEvent(L"Session upkeep");
    }

    assert(FFileSystem != NULL);
    TRACE("1");
    FFileSystem->Idle();

    TRACE("2");
    if (GetCommandSessionOpened())
    {
      TRACE("3");
      try
      {
        FCommandSession->Idle();
      }
      catch(Exception & E)
      {
        // TRACEE;
        // If the secondary session is dropped, ignore the error and let
        // it be reconnected when needed.
        // BTW, non-fatal error can hardly happen here, that's why
        // it is displayed, because it can be useful to know.
        if (FCommandSession->GetActive())
        {
          FCommandSession->HandleExtendedException(&E);
        }
      }
      TRACE("4");
    }
  }
  TRACE_EXCEPT_END
  TRACE("/");
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
  CALLSTACK;
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
  CALLSTACK;
  FFileSystem->Close();

  if (GetCommandSessionOpened())
  {
    FCommandSession->Close();
  }
  TRACE("/");
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
  CALLSTACK;
  FLog->ReflectSettings();
  FActionLog->ReflectSettings();
  bool Reopen = false;
  do
  {
    TRACE("Open 1a");
    Reopen = false;
    DoInformation(L"", true, 1);
    try
    {
      TRACE("Open 1b");
      TRY_FINALLY (
      {
        try
        {
          ResetConnection();
          FStatus = ssOpening;
          TRY_FINALLY (
          {
            TRACE("Open 2");
            if (FFileSystem == NULL)
            {
              GetLog()->AddStartupInfo();
            }

            assert(FTunnel == NULL);
            if (FSessionData->GetTunnel())
            {
              TRACE("Open 3");
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
              TRACE("Open 4");
              assert(FTunnelLocalPortNumber == 0);
            }

            if (FFileSystem == NULL)
            {
              TRACE("Open 5");
              if ((GetSessionData()->GetFSProtocol() == fsFTP) && (GetSessionData()->GetFtps() == ftpsNone))
              {
                TRACE("Open 6");
/*#ifdef NO_FILEZILLA
                LogEvent(L"FTP protocol is not supported by this build.");
                FatalError(NULL, LoadStr(FTP_UNSUPPORTED));
#else*/
                FFSProtocol = cfsFTP;
                FFileSystem = new TFTPFileSystem(this);
                FFileSystem->Init();
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
                FFileSystem->Init();
                FFileSystem->Open();
                GetLog()->AddSeparator();
                LogEvent(L"Using FTPS protocol.");
// #endif
              }
              else if (GetSessionData()->GetFSProtocol() == fsWebDAV)
              {
                TRACE("Open 7");
                FFSProtocol = cfsWebDAV;
                FFileSystem = new TWebDAVFileSystem(this);
                FFileSystem->Init();
                FFileSystem->Open();
                GetLog()->AddSeparator();
                LogEvent(L"Using WebDAV protocol.");
              }
              else
              {
                TRACE("Open 8");
                assert(FSecureShell == NULL);
                TRY_FINALLY (
                {
                  FSecureShell = new TSecureShell(this, FSessionData, GetLog(), Configuration);
                  try
                  {
                    // there will be only one channel in this session
                    FSecureShell->SetSimple(true);
                    FSecureShell->Open();
                  }
                  catch (Exception & E)
                  {
                    TRACEFMT("Open 9 [%s]", (E.Message.c_str()));
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
                    TRACE("Open 9");
                    FFSProtocol = cfsSCP;
                    FFileSystem = new TSCPFileSystem(this);
                    (static_cast<TSCPFileSystem *>(FFileSystem))->Init(FSecureShell);
                    FSecureShell = NULL; // ownership passed
                    LogEvent(L"Using SCP protocol.");
                  }
                  else
                  {
                    TRACE("Open 10");
                    FFSProtocol = cfsSFTP;
                    FFileSystem = new TSFTPFileSystem(this);
                    (static_cast<TSFTPFileSystem *>(FFileSystem))->Init(FSecureShell);
                    FSecureShell = NULL; // ownership passed
                    LogEvent(L"Using SFTP protocol.");
                  }
                }
                ,
                {
                  TRACE("Open 11");
                  delete FSecureShell;
                  FSecureShell = NULL;
                }
                );
              }
            }
            else
            {
              TRACE("Open 12");
              FFileSystem->Open();
              TRACE("Open 12a");
            }
          }
          ,
          {
            TRACE("Open 13");
            if (FSessionData->GetTunnel())
            {
              FSessionData->RollbackTunnel();
            }
          }
          );

          TRACE("Open 14");
          if (GetSessionData()->GetCacheDirectoryChanges())
          {
            TRACE("Open 15");
            assert(FDirectoryChangesCache == NULL);
            FDirectoryChangesCache = new TRemoteDirectoryChangesCache(
              Configuration->GetCacheDirectoryChangesMaxSize());
            if (GetSessionData()->GetPreserveDirectoryChanges())
            {
              Configuration->LoadDirectoryChangesCache(GetSessionData()->GetSessionKey(),
                  FDirectoryChangesCache);
            }
          }

          TRACE("Open 16");
          DoStartup();

          DoInformation(LoadStr(STATUS_READY), true);
          FStatus = ssOpened;
          TRACE("Open 17");
        }
        catch (...)
        {
          TRACE("Open 18");
          // rollback
          if (FDirectoryChangesCache != NULL)
          {
            delete FDirectoryChangesCache;
            FDirectoryChangesCache = NULL;
          }
          throw;
        }
        TRACE("Open 19");
      }
      ,
      {
        TRACE("Open 20");
        DoInformation(L"", true, 0);
      }
      );
      TRACE("Open 21");
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
  TRACE("/");
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
    Result = (bind(Socket, reinterpret_cast<sockaddr *>(&Address), sizeof(Address)) == 0);
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
    FTunnelData->SetHostKey(FTunnelData->GetTunnelHostKey());
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
  CALLSTACK;
  if (FTunnel != NULL)
  {
     CloseTunnel();
  }

  if (GetOnClose())
  {
    TRACE("1");
    TCallbackGuard Guard(this);
    GetOnClose()(this);
    Guard.Verify();
  }

  FStatus = ssClosed;
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::Reopen(int Params)
{
  CALLSTACK;
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
  TRY_FINALLY (
  {
    TRACE("Reopen 1");
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

    TRACE("Reopen 2");
    Open();
  }
  ,
  {
    TRACE("Reopen 3");
    GetSessionData()->SetRemoteDirectory(PrevRemoteDirectory);
    GetSessionData()->SetFSProtocol(OrigFSProtocol);
    FAutoReadDirectory = PrevAutoReadDirectory;
    FReadCurrentDirectoryPending = PrevReadCurrentDirectoryPending;
    FReadDirectoryPending = PrevReadDirectoryPending;
    FSuspendTransaction = false;
    FExceptionOnFail = PrevExceptionOnFail;
  }
  );
  TRACE("/");
}
//---------------------------------------------------------------------------
bool __fastcall TTerminal::PromptUser(TSessionData * Data, TPromptKind Kind,
  UnicodeString Name, UnicodeString Instructions, UnicodeString Prompt, bool Echo, int MaxLen, UnicodeString & Result)
{
  bool AResult;
  TStrings * Prompts = new TStringList();
  TStrings * Results = new TStringList();
  TRY_FINALLY (
  {
    Prompts->AddObject(Prompt, reinterpret_cast<TObject *>(static_cast<size_t>(Echo)));
    Results->AddObject(Result, reinterpret_cast<TObject *>(MaxLen));

    AResult = PromptUser(Data, Kind, Name, Instructions, Prompts, Results);

    Result = Results->Strings[0];
  }
  ,
  {
    delete Prompts;
    delete Results;
  }
  );

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
  CALLSTACK;
  bool AResult = false;

  if (GetOnPromptUser() != NULL)
  {
    TRACE("01");
    TCallbackGuard Guard(this);
    GetOnPromptUser()(this, Kind, Name, Instructions, Prompts, Results, AResult, NULL);
    Guard.Verify();
    TRACE("02");
  }

  TRACEFMT("1 [%d] [%d] [%d] [%d] [%d]", (int(AResult), int(Configuration->GetRememberPassword()), int(Prompts->Count), (Prompts->Count > 0 ? int(!bool(Prompts->Objects[0])) : 0), int(Kind)));
  if (AResult && (Configuration->GetRememberPassword()) &&
      (Prompts->Count == 1) && !(Prompts->Objects[0]) &&
      ((Kind == pkPassword) || (Kind == pkPassphrase) || (Kind == pkKeybInteractive) ||
       (Kind == pkTIS) || (Kind == pkCryptoCard)))
  {
    TRACE("2");
    RawByteString EncryptedPassword = EncryptPassword(Results->Strings[0]);
    TRACEFMT("2a [%x] [%d] [%d]", (int(this), Results->Strings[0].Length(), EncryptedPassword.Length()));
    if (FTunnelOpening)
    {
      TRACE("3");
      FTunnelPassword = EncryptedPassword;
    }
    else
    {
      FPassword = EncryptedPassword;
      TRACE("4");
    }
  }

  TRACE("/");
  return AResult;
}
//---------------------------------------------------------------------------
unsigned int /* __fastcall */ TTerminal::QueryUser(const UnicodeString Query,
  TStrings * MoreMessages, unsigned int Answers, const TQueryParams * Params,
  TQueryType QueryType)
{
  CALLSTACK;
  LogEvent(FORMAT(L"Asking user:\n%s (%s)", Query.c_str(), MoreMessages ? MoreMessages->CommaText.get().c_str() : L""));
  unsigned int Answer = AbortAnswer(Answers);
  if (FOnQueryUser)
  {
    TCallbackGuard Guard(this);
    FOnQueryUser(this, Query, MoreMessages, Answers, Params, Answer, QueryType, NULL);
    Guard.Verify();
  }
  TRACE("/");
  return Answer;
}
//---------------------------------------------------------------------------
unsigned int __fastcall TTerminal::QueryUserException(const UnicodeString Query,
  Exception * E, unsigned int Answers, const TQueryParams * Params,
  TQueryType QueryType)
{
  CALLSTACK;
  unsigned int Result;
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
      MoreMessages->Count ? MoreMessages : NULL,
      Answers, Params, QueryType);
  }
  ,
  {
    delete MoreMessages;
  }
  );
  TRACE("/");
  return Result;
}
//---------------------------------------------------------------------------
void __fastcall TTerminal::DisplayBanner(const UnicodeString & Banner)
{
  CALLSTACK;
  if (GetOnDisplayBanner() != NULL)
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
  CALLSTACK;
  TRACEFMT("1 [%s]", (E->Message.c_str()));
  GetLog()->AddException(E);
  if (GetOnShowExtendedException() != NULL)
  {
    TCallbackGuard Guard(this);
    // the event handler may destroy 'this' ...
    GetOnShowExtendedException()(this, E, NULL);
    // .. hence guard is dismissed from destructor, to make following call no-op
    Guard.Verify();
  }
  TRACE("/");
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::ShowExtendedException(Exception * E)
{
  CALLSTACK;
  TRACEFMT("1 [%s]", (E->Message.c_str()));
  GetLog()->AddException(E);
  if (GetOnShowExtendedException() != NULL)
  {
    GetOnShowExtendedException()(this, E, NULL);
  }
  TRACE("/");
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::DoInformation(const UnicodeString & Str, bool Status,
  int Phase)
{
  CALLSTACK;
  if (GetOnInformation())
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
  CALLSTACK;
  if (GetOnProgress() != NULL)
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
  CALLSTACK;
  if (GetOnFinished() != NULL)
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
  CALLSTACK;
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
    TRACE("1");
    if (!InTransaction())
    {
      TRACE("2");
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
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::TerminalError(UnicodeString Msg)
{
  TerminalError(NULL, Msg);
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::TerminalError(Exception * E, UnicodeString Msg)
{
  CALLSTACK;
  throw ETerminal(E, Msg);
}
//---------------------------------------------------------------------------
bool /* __fastcall */ TTerminal::DoQueryReopen(Exception * E)
{
  CALLSTACK;
  EFatal * Fatal = dynamic_cast<EFatal *>(E);
  assert(Fatal != NULL);
  bool Result = false;
  if ((Fatal != NULL) && Fatal->GetReopenQueried())
  {
    TRACE("1");
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
      TRACE("2");
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
  CALLSTACK;
  TSuspendFileOperationProgress Suspend(OperationProgress);

  bool Result = DoQueryReopen(E);

  if (Result)
  {
    TRACE("QueryReopen 1");
    TDateTime Start = Now();
    do
    {
      try
      {
        TRACE("QueryReopen 2");
        Reopen(Params);
        FSessionData->SetNumberOfRetries(0);
      }
      catch(Exception & E)
      {
        if (!GetActive())
        {
          TRACE("QueryReopen 4");
          Result =
            ((Configuration->GetSessionReopenTimeout() == 0) ||
             (int(double(Now() - Start) * MSecsPerDay) < Configuration->GetSessionReopenTimeout())) &&
            DoQueryReopen(&E);
        }
        else
        {
          TRACE("QueryReopen 5");
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
  CALLSTACK;
  bool Result = false;
  GetLog()->AddException(&E);
  unsigned int Answer;

  if (AllowSkip && OperationProgress->SkipToAll)
  {
    TRACE("1");
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
      TRACE("2");
      Aliases[AliasCount].Button = qaAll;
      Aliases[AliasCount].Alias = LoadStr(SKIP_ALL_BUTTON);
      AliasCount++;
    }
    if (FLAGSET(Answers, qaYes))
    {
      TRACE("3");
      Aliases[AliasCount].Button = qaYes;
      Aliases[AliasCount].Alias = SpecialRetry;
      AliasCount++;
    }

    if (AliasCount > 0)
    {
      TRACE("4");
      Params.Aliases = Aliases;
      Params.AliasesCount = AliasCount;
    }

    SUSPEND_OPERATION (
      Answer = QueryUserException(Message, &E, Answers, &Params, qtError);
    );

    if (Answer == qaAll)
    {
      TRACE("5");
      OperationProgress->SkipToAll = true;
      Answer = qaSkip;
    }
    if (Answer == qaYes)
    {
      TRACE("6");
      Result = true;
      Answer = qaRetry;
    }
  }

  if (Answer != qaRetry)
  {
    TRACE("7");
    if (Answer == qaAbort)
    {
      TRACE("8");
      OperationProgress->Cancel = csCancel;
    }

    if (AllowSkip)
    {
      TRACE("9");
      THROW_SKIP_FILE(&E, Message);
    }
    else
    {
      TRACE("10");
      // this can happen only during file transfer with SCP
      throw ExtException(&E, Message);
    }
  }

  TRACE("/");
  return Result;
}
//---------------------------------------------------------------------------
int /* __fastcall */ TTerminal::FileOperationLoop(TFileOperationEvent CallBackFunc,
  TFileOperationProgressType * OperationProgress, bool AllowSkip,
  const UnicodeString Message, void * Param1, void * Param2)
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
//---------------------------------------------------------------------------
UnicodeString /* __fastcall */ TTerminal::TranslateLockedPath(UnicodeString Path, bool Lock)
{
  if (!GetSessionData()->GetLockInHome() || Path.IsEmpty() || (Path[1] != L'/'))
    return Path;

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
  CALLSTACK;
  bool Result = false;
  if (UnixComparePaths(FFiles->GetDirectory(), Path))
  {
    TRACE("1");
    Result = (FileList == NULL) || (FileList->GetTimestamp() < FFiles->GetTimestamp());
    if (Result)
    {
      if (FileList == NULL)
      {
        FileList = new TRemoteFileList();
      }
      TRACE("2");
      FFiles->DuplicateTo(FileList);
      TRACE("3");
    }
  }
  else
  {
    TRACE("4");
    if (((FileList == NULL) && FDirectoryCache->HasFileList(Path)) ||
        ((FileList != NULL) && FDirectoryCache->HasNewerFileList(Path, FileList->GetTimestamp())))
    {
      TRACE("5");
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
      TRACE("6");
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

  TRACE("/");
  return Result;
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::SetCurrentDirectory(UnicodeString value)
{
  CALLSTACK;
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
  CALLSTACK;
  if (FFileSystem)
  {
    UnicodeString CurrentDirectory = FFileSystem->GetCurrentDirectory();
    if (FCurrentDirectory != CurrentDirectory)
    {
      TRACE("1");
      FCurrentDirectory = CurrentDirectory;
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
  CALLSTACK;
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
  CALLSTACK;
  if (FOnChangeDirectory)
  {
    TCallbackGuard Guard(this);
    FOnChangeDirectory(this);
    Guard.Verify();
  }
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::DoReadDirectory(bool ReloadOnly)
{
  CALLSTACK;
  if (FOnReadDirectory)
  {
    TCallbackGuard Guard(this);
    FOnReadDirectory(this, ReloadOnly);
    Guard.Verify();
  }
  TRACE("/");
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::DoStartReadDirectory()
{
  CALLSTACK;
  if (FOnStartReadDirectory)
  {
    TCallbackGuard Guard(this);
    FOnStartReadDirectory(this);
    Guard.Verify();
  }
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::DoReadDirectoryProgress(int Progress, bool & Cancel)
{
  CALLSTACK;
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
//---------------------------------------------------------------------------
bool /* __fastcall */ TTerminal::InTransaction()
{
  return (FInTransaction > 0) && !FSuspendTransaction;
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::BeginTransaction()
{
  CALLSTACK;
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
  CALLSTACK;
  if (FInTransaction == 0)
    TerminalError(L"Can't end transaction, not in transaction");
  assert(FInTransaction > 0);
  FInTransaction--;

  // it connection was closed due to fatal error during transaction, do nothing
  if (GetActive())
  {
    TRACE("1");
    if (FInTransaction == 0)
    {
      TRY_FINALLY (
      {
        TRACE("2");
        if (FReadCurrentDirectoryPending) { ReadCurrentDirectory(); }
        if (FReadDirectoryPending) { ReadDirectory(!FReadCurrentDirectoryPending); }
      }
      ,
      {
        TRACE("3");
        FReadCurrentDirectoryPending = false;
        FReadDirectoryPending = false;
      }
      );
    }
  }

  if (FCommandSession != NULL)
  {
    TRACE("4");
    FCommandSession->EndTransaction();
  }
  TRACE("/");
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
      throw Exception(L"ExceptionOnFail is already zero.");
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
  CALLSTACK;
  FatalError(NULL, "");
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::FatalError(Exception * E, UnicodeString Msg)
{
  CALLSTACK;
  TRACEFMT("[%s] [%s]", ((E != NULL ? E->Message : UnicodeString(L"NULL")), Msg));
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
//---------------------------------------------------------------------------
void __fastcall TTerminal::CommandError(Exception * E, const UnicodeString Msg)
{
  CALLSTACK;
  CommandError(E, Msg, 0);
  TRACE("/");
}
//---------------------------------------------------------------------------
unsigned int __fastcall TTerminal::CommandError(Exception * E, const UnicodeString Msg,
  unsigned int Answers)
{
  CALLSTACK;
  // may not be, particularly when TTerminal::Reopen is being called
  // from within OnShowExtendedException handler
  assert(FCallbackGuard == NULL);
  unsigned int Result = 0;
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
        TRACE("1");
        assert(GetOperationProgress() != NULL);
        GetOperationProgress()->SkipToAll = true;
        Result = qaSkip;
      }
    }
  }
  TRACE("/");
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
void __fastcall TTerminal::CloseOnCompletion(TOnceDoneOperation Operation, const UnicodeString Message)
{
  LogEvent(L"Closing session after completed operation (as requested by user)");
  Close();
  throw ESshTerminate(NULL,
    Message.IsEmpty() ? UnicodeString(LoadStr(CLOSED_ON_COMPLETION)) : Message,
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
  CALLSTACK;
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
  CALLSTACK;
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
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::DirectoryModified(const UnicodeString Path, bool SubDirs)
{
  CALLSTACK;
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
  CALLSTACK;
  AddCachedFileList(FileList);
  TRACE("/");
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::ReloadDirectory()
{
  CALLSTACK;
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
  CALLSTACK;
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
  CALLSTACK;
  // if filename doesn't contain path, we check for existence of file
  if ((UnixExtractFileDir(FileName).IsEmpty()) &&
      UnixComparePaths(GetCurrentDirectory(), FFiles->GetDirectory()))
  {
    TRemoteFile * File = FFiles->FindFile(FileName);
    if (File)
    {
      if (File->GetIsDirectory()) { throw ECommand(NULL, FMTLOAD(RENAME_CREATE_DIR_EXISTS, FileName.c_str())); }
      else { throw ECommand(NULL, FMTLOAD(RENAME_CREATE_FILE_EXISTS, FileName.c_str())); }
    }
  }
}
//---------------------------------------------------------------------------
void __fastcall /* inline */ TTerminal::LogEvent(const UnicodeString & Str)
{
  TRACEFMT("[%s]", (Str));
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
  CALLSTACK;
  LogEvent(L"Doing startup conversation with host.");
  BeginTransaction();
  TRY_FINALLY (
  {
    TRACE("1");
    DoInformation(LoadStr(STATUS_STARTUP), true);

    // Make sure that directory would be loaded at last
    FReadCurrentDirectoryPending = true;
    FReadDirectoryPending = GetAutoReadDirectory();

    FFileSystem->DoStartup();

    LookupUsersGroups();

    DoInformation(LoadStr(STATUS_OPEN_DIRECTORY), true);
    if (!GetSessionData()->GetRemoteDirectory().IsEmpty())
    {
      TRACE("2");
      ChangeDirectory(GetSessionData()->GetRemoteDirectory());
    }

  }
  ,
  {
    TRACE("F");
    EndTransaction();
  }
  );
  LogEvent(L"Startup conversation with host finished.");
  TRACE("/");
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::ReadCurrentDirectory()
{
  CALLSTACK;
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
      // be initialited by ChangeDirectory(), which sets it
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
    // TRACEE;
    CommandError(&E, LoadStr(READ_CURRENT_DIR_ERROR));
  }
  TRACE("/");
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::ReadDirectory(bool ReloadOnly, bool ForceCache)
{
  CALLSTACK;
  bool LoadedFromCache = false;

  if (GetSessionData()->GetCacheDirectories() && FDirectoryCache->HasFileList(GetCurrentDirectory()))
  {
    TRACE("1");
    if (ReloadOnly && !ForceCache)
    {
      LogEvent(L"Cached directory not reloaded.");
    }
    else
    {
      TRACE("2");
      DoStartReadDirectory();
      TRY_FINALLY (
      {
        LoadedFromCache = FDirectoryCache->GetFileList(GetCurrentDirectory(), FFiles);
      }
      ,
      {
        TRACE("3");
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
    TRACE("4");
    DoStartReadDirectory();
    FReadingCurrentDirectory = true;
    bool Cancel = false; // dummy
    DoReadDirectoryProgress(0, Cancel);

    try
    {
      TRACE("5");
      TRemoteDirectory * Files = new TRemoteDirectory(this, FFiles);
      TRY_FINALLY (
      {
        Files->SetDirectory(GetCurrentDirectory());
        CustomReadDirectory(Files);
      }
      ,
      {
        TRACE("6");
        DoReadDirectoryProgress(-1, Cancel);
        FReadingCurrentDirectory = false;
        delete FFiles;
        FFiles = Files;
        DoReadDirectory(ReloadOnly);
      }
      );
      if (GetActive())
      {
        if (GetSessionData()->GetCacheDirectories())
        {
          TRACE("6a");
          DirectoryLoaded(FFiles);
        }
      }
    }
    catch (Exception &E)
    {
      TRACE("7");
      CommandError(&E, FmtLoadStr(LIST_DIR_ERROR, FFiles->GetDirectory().c_str()));
    }
  }
  TRACE("/");
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::CustomReadDirectory(TRemoteFileList * FileList)
{
  CALLSTACK;
  assert(FileList);
  assert(FFileSystem);
  FFileSystem->ReadDirectory(FileList);

  if (Configuration->GetActualLogProtocol() >= 1)
  {
    for (int Index = 0; Index < FileList->Count; Index++)
    {
      TRemoteFile * File = FileList->GetFiles(Index);
      LogEvent(FORMAT(L"%s;%c;%lld;%s;%s;%s;%s;%d",
        File->GetFileName().c_str(), File->GetType(), File->GetSize(), StandardTimestamp(File->GetModification()).c_str(),
         File->GetFileOwner().GetLogText().c_str(), File->GetFileGroup().GetLogText().c_str(), File->GetRights()->GetText().c_str(),
         File->GetAttr()));
    }
  }

  ReactOnCommand(fsListDirectory);
}
//---------------------------------------------------------------------------
TRemoteFileList * /* __fastcall */ TTerminal::ReadDirectoryListing(UnicodeString Directory, const TFileMasks & Mask)
{
  CALLSTACK;
  TLsSessionAction Action(GetActionLog(), AbsolutePath(Directory, true));
  TRemoteFileList * FileList = NULL;
  try
  {
    FileList = DoReadDirectoryListing(Directory, false);
    if (FileList != NULL)
    {
      int Index = 0;
      while (Index < FileList->Count)
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
    TRACE("E2");
    COMMAND_ERROR_ARI_ACTION
    (
      L"",
      FileList = ReadDirectoryListing(Directory, Mask),
      Action
    );
  }
  TRACE("/");
  return FileList;
}
//---------------------------------------------------------------------------
TRemoteFile * __fastcall TTerminal::ReadFileListing(UnicodeString Path)
{
  CALLSTACK;
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
    TRACE("E2");
    COMMAND_ERROR_ARI_ACTION
    (
      L"",
      File = ReadFileListing(Path),
      Action
    );
  }
  TRACE("/");
  return File;
}
//---------------------------------------------------------------------------
TRemoteFileList * /* __fastcall */ TTerminal::CustomReadDirectoryListing(UnicodeString Directory, bool UseCache)
{
  CALLSTACK;
  TRemoteFileList * FileList = NULL;
  try
  {
    FileList = DoReadDirectoryListing(Directory, UseCache);
  }
  catch(Exception & E)
  {
    TRACE("E2");
    COMMAND_ERROR_ARI
    (
      L"",
      FileList = CustomReadDirectoryListing(Directory, UseCache);
    );
  }
  TRACE("/");
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
    TRACE("E1");
    delete FileList;
    throw;
  }
  return FileList;
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::ProcessDirectory(const UnicodeString DirName,
  TProcessFileEvent CallBackFunc, void * Param, bool UseCache, bool IgnoreErrors)
{
  CALLSTACK;
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

      TRemoteFile * File;
      for (int Index = 0; Index < FileList->Count; Index++)
      {
        File = FileList->GetFiles(Index);
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
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::ReadDirectory(TRemoteFileList * FileList)
{
  CALLSTACK;
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
  CALLSTACK;
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
  CALLSTACK;
  assert(FFileSystem);
  File = NULL;
  try
  {
    LogEvent(FORMAT(L"Listing file \"%s\".", FileName.c_str()));
    FFileSystem->ReadFile(FileName, File);
    ReactOnCommand(fsListFile);
    TRACE("1");
  }
  catch (Exception &E)
  {
    TRACE("2");
    if (File) { delete File; }
    File = NULL;
    CommandError(&E, FMTLOAD(CANT_GET_ATTRS, FileName.c_str()));
  }
  TRACE("/");
}
//---------------------------------------------------------------------------
bool /* __fastcall */ TTerminal::FileExists(const UnicodeString FileName, TRemoteFile ** AFile)
{
  CALLSTACK;
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
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::AnnounceFileListOperation()
{
  FFileSystem->AnnounceFileListOperation();
}
//---------------------------------------------------------------------------
bool /* __fastcall */ TTerminal::ProcessFiles(TStrings * FileList,
  TFileOperation Operation, TProcessFileEvent ProcessFile, void * Param,
  TOperationSide Side, bool Ex)
{
  CALLSTACK;
  assert(FFileSystem);
  assert(FileList);

  bool Result = false;
  TOnceDoneOperation OnceDoneOperation = odoIdle;

  try
  {
    TRACE("1");
    TFileOperationProgressType Progress(MAKE_CALLBACK0(TTerminal::DoProgress, this), MAKE_CALLBACK6(TTerminal::DoFinished, this));
    Progress.Start(Operation, Side, FileList->Count);

    FOperationProgress = &Progress;
    TFileOperationProgressType * OperationProgress(&Progress);
    TRY_FINALLY (
    {
      if (Side == osRemote)
      {
        TRACE("2");
        BeginTransaction();
      }

      TRY_FINALLY (
      {
        TRACE("3");
        int Index = 0;
        UnicodeString FileName;
        bool Success;
        while ((Index < FileList->Count) && (Progress.Cancel == csContinue))
        {
          FileName = FileList->Strings[Index];
          TRACEFMT("4 [%s]", (FileName));
          try
          {
            TRY_FINALLY (
            {
              Success = false;
              if (!Ex)
              {
                TRemoteFile * RemoteFile = static_cast<TRemoteFile *>(FileList->Objects[Index]);
                ProcessFile(FileName, RemoteFile, Param);
              }
              else
              {
                // not used anymore
                // TProcessFileEventEx ProcessFileEx = (TProcessFileEventEx)ProcessFile;
                // ProcessFileEx(FileName, (TRemoteFile *)FileList->Objects[Index], Param, Index);
              }
              Success = true;
              TRACE("4a");
            }
            ,
            {
              TRACE("4b");
              Progress.Finish(FileName, Success, OnceDoneOperation);
              TRACE("5");
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
          Index++;
        }
      }
      ,
      {
        if (Side == osRemote)
        {
          TRACE("6");
          EndTransaction();
          TRACE("7");
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
      TRACE("8");
      FOperationProgress = NULL;
      Progress.Stop();
    }
    );
  }
  catch (...)
  {
    TRACE("E");
    OnceDoneOperation = odoIdle;
    // this was missing here. was it by purpose?
    // without it any error message is lost
    throw;
  }

  if (OnceDoneOperation != odoIdle)
  {
    TRACE("9");
    CloseOnCompletion(OnceDoneOperation);
  }

  TRACE("/");
  return Result;
}
//---------------------------------------------------------------------------
// not used anymore
bool __fastcall TTerminal::ProcessFilesEx(TStrings * FileList, TFileOperation Operation,
  TProcessFileEventEx ProcessFile, void * Param, TOperationSide Side)
{
#ifndef _MSC_VER
  return ProcessFiles(FileList, Operation, TProcessFileEvent(ProcessFile),
    Param, Side, true);
#else
  return false;
#endif
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
  CALLSTACK;
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
#ifndef _MSC_VER
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
    MoveFile(FileName, File, &Params);
  }
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::DeleteFile(const UnicodeString & FileName,
  const TRemoteFile * File, void * AParams)
{
  CALLSTACK;
  UnicodeString LocalFileName = FileName;
  if (FileName.IsEmpty() && File)
  {
    LocalFileName = File->GetFileName();
  }
  if (GetOperationProgress() && GetOperationProgress()->Operation == foDelete)
  {
    if (GetOperationProgress()->Cancel != csContinue) { Abort(); }
    GetOperationProgress()->SetFile(LocalFileName);
  }
  int Params = (AParams != NULL) ? *(static_cast<int*>(AParams)) : 0;
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
    if (File) { FileModified(File, LocalFileName, true); }
    DoDeleteFile(LocalFileName, File, Params);
    ReactOnCommand(fsDeleteFile);
  }
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::DoDeleteFile(const UnicodeString FileName,
  const TRemoteFile * File, int Params)
{
  CALLSTACK;
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
  CALLSTACK;
  // TODO: avoid resolving symlinks while reading subdirectories.
  // Resolving does not work anyway for relative symlinks in subdirectories
  // (at least for SFTP).
  return ProcessFiles(FilesToDelete, foDelete, MAKE_CALLBACK3(TTerminal::DeleteFile, this), &Params);
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::DeleteLocalFile(const UnicodeString & FileName,
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
//---------------------------------------------------------------------------
bool /* __fastcall */ TTerminal::DeleteLocalFiles(TStrings * FileList, int Params)
{
  CALLSTACK;
  return ProcessFiles(FileList, foDelete, MAKE_CALLBACK3(TTerminal::DeleteLocalFile, this), &Params, osLocal);
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::CustomCommandOnFile(const UnicodeString & FileName,
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
    if (GetOperationProgress()->Cancel != csContinue) { Abort(); }
    GetOperationProgress()->SetFile(LocalFileName);
  }
  LogEvent(FORMAT(L"Executing custom command \"%s\" (%d) on file \"%s\".",
    Params->Command.c_str(), Params->Params, LocalFileName.c_str()));
  if (File) { FileModified(File, LocalFileName); }
  DoCustomCommandOnFile(LocalFileName, File, Params->Command, Params->Params,
    Params->OutputEvent);
  ReactOnCommand(fsAnyCommand);
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::DoCustomCommandOnFile(UnicodeString FileName,
  const TRemoteFile * File, UnicodeString Command, int Params,
  TCaptureOutputEvent OutputEvent)
{
  CALLSTACK;
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
  int Params, TStrings * Files, TCaptureOutputEvent OutputEvent)
{
  CALLSTACK;
  if (!TRemoteCustomCommand().IsFileListCommand(Command))
  {
    TCustomCommandParams AParams;
    AParams.Command = Command;
    AParams.Params = Params;
    AParams.OutputEvent = OutputEvent;
    ProcessFiles(Files, foCustomCommand, MAKE_CALLBACK3(TTerminal::CustomCommandOnFile, this), &AParams);
  }
  else
  {
    UnicodeString FileList;
    for (int i = 0; i < Files->Count; i++)
    {
      TRemoteFile * File = static_cast<TRemoteFile *>(Files->Objects[i]);
      bool Dir = File->GetIsDirectory() && !File->GetIsSymLink();

      if (!Dir || FLAGSET(Params, ccApplyToDirectories))
      {
        if (!FileList.IsEmpty())
        {
          FileList += L" ";
        }

        FileList += L"\"" + ShellDelimitStr(Files->Strings[i], L'"') + L"\"";
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
void /* __fastcall */ TTerminal::ChangeFileProperties(const UnicodeString & FileName,
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
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::DoChangeFileProperties(const UnicodeString FileName,
  const TRemoteFile * File, const TRemoteProperties * Properties)
{
  CALLSTACK;
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
  CALLSTACK;
  AnnounceFileListOperation();
  ProcessFiles(FileList, foSetProperties, MAKE_CALLBACK3(TTerminal::ChangeFileProperties, this), const_cast<void *>(static_cast<const void *>(Properties)));
}
//---------------------------------------------------------------------------
bool /* __fastcall */ TTerminal::LoadFilesProperties(TStrings * FileList)
{
  bool Result =
    GetIsCapable(fcLoadingAdditionalProperties) &&
    FFileSystem->LoadFilesProperties(FileList);
  if (Result && GetSessionData()->GetCacheDirectories() &&
      (FileList->Count > 0) &&
      (dynamic_cast<TRemoteFile *>(FileList->Objects[0])->GetDirectory() == FFiles))
  {
    AddCachedFileList(FFiles);
  }
  return Result;
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::CalculateFileSize(const UnicodeString & FileName,
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
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::DoCalculateDirectorySize(const UnicodeString FileName,
  const TRemoteFile * File, TCalculateSizeParams * Params)
{
  CALLSTACK;
  try
  {
    ProcessDirectory(FileName, MAKE_CALLBACK3(TTerminal::CalculateFileSize, this), Params);
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
  CALLSTACK;
  TCalculateSizeParams Param;
  Param.Size = 0;
  Param.Params = Params;
  Param.CopyParam = CopyParam;
  Param.Stats = Stats;
  ProcessFiles(FileList, foCalculateSize, MAKE_CALLBACK3(TTerminal::CalculateFileSize, this), &Param);
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
void /* __fastcall */ TTerminal::RenameFile(const UnicodeString & FileName,
  const UnicodeString & NewName)
{
  LogEvent(FORMAT(L"Renaming file \"%s\" to \"%s\".", FileName.c_str(), NewName.c_str()));
  DoRenameFile(FileName, NewName, false);
  ReactOnCommand(fsRenameFile);
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::RenameFile(const TRemoteFile * File,
  const UnicodeString & NewName, bool CheckExistence)
{
  CALLSTACK;
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
        QuestionFmt = LoadStr(DIRECTORY_OVERWRITE);
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
  CALLSTACK;
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
void /* __fastcall */ TTerminal::MoveFile(const UnicodeString & FileName,
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
  CALLSTACK;
  TMoveFileParams Params;
  Params.Target = Target;
  Params.FileMask = FileMask;
  DirectoryModified(Target, true);
  bool Result = false;
  BeginTransaction();
  TRY_FINALLY (
  {
    Result = ProcessFiles(FileList, foRemoteMove, MAKE_CALLBACK3(TTerminal::MoveFile, this), &Params);
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
      for (int Index = 0; !PossiblyMoved && (Index < FileList->Count); Index++)
      {
        const TRemoteFile * File =
          dynamic_cast<const TRemoteFile *>(FileList->Objects[Index]);
        // File can be NULL, and filename may not be full path,
        // but currently this is the only way we can move (at least in GUI)
        // current directory
        if ((File != NULL) &&
            File->GetIsDirectory() &&
            ((curDirectory.SubString(1, FileList->Strings[Index].Length()) == FileList->Strings[Index]) &&
             ((FileList->Strings[Index].Length() == curDirectory.Length()) ||
              (curDirectory[FileList->Strings[Index].Length() + 1] == '/'))))
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
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::DoCopyFile(const UnicodeString FileName,
  const UnicodeString NewName)
{
  CALLSTACK;
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
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::CopyFile(const UnicodeString & FileName,
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
  CALLSTACK;
  TMoveFileParams Params;
  Params.Target = Target;
  Params.FileMask = FileMask;
  DirectoryModified(Target, true);
  return ProcessFiles(FileList, foRemoteCopy, MAKE_CALLBACK3(TTerminal::CopyFile, this), &Params);
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::CreateDirectory(const UnicodeString DirName,
  const TRemoteProperties * Properties)
{
  CALLSTACK;
  assert(FFileSystem);
  EnsureNonExistence(DirName);
  FileModified(NULL, DirName);

  LogEvent(FORMAT(L"Creating directory \"%s\".", DirName.c_str()));
  TRACE("1");
  DoCreateDirectory(DirName);

  if ((Properties != NULL) && !Properties->Valid.Empty())
  {
    TRACE("2");
    DoChangeFileProperties(DirName, NULL, Properties);
  }

  ReactOnCommand(fsCreateDirectory);
  TRACE("/");
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::DoCreateDirectory(const UnicodeString DirName)
{
  CALLSTACK;
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
  CALLSTACK;
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
  CALLSTACK;
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
  CALLSTACK;
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
  CALLSTACK;
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
  CALLSTACK;
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
  TRACE("/");
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
  CALLSTACK;
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
  TCaptureOutputEvent OutputEvent)
{
  CALLSTACK;

  class TOutputProxy
  {
  public:
    /* __fastcall */ TOutputProxy(TCallSessionAction & Action, TCaptureOutputEvent OutputEvent) :
      FAction(Action),
      FOutputEvent(OutputEvent)
    {
    }

    void /* __fastcall */ Output(const UnicodeString & Str, bool StdError)
    {
      CALLSTACK;
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

  TCallSessionAction Action(GetActionLog(), Command, GetCurrentDirectory());
  TOutputProxy ProxyOutputEvent(Action, OutputEvent);
  DoAnyCommand(Command, MAKE_CALLBACK2(TOutputProxy::Output, &ProxyOutputEvent), &Action);
  TRACE("/");
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::DoAnyCommand(const UnicodeString Command,
  TCaptureOutputEvent OutputEvent, TCallSessionAction * Action)
{
  CALLSTACK;
  assert(FFileSystem);
  try
  {
    TRACE("1");
    DirectoryModified(GetCurrentDirectory(), false);
    if (GetIsCapable(fcAnyCommand))
    {
      TRACE("2");
      LogEvent(L"Executing user defined command.");
      FFileSystem->AnyCommand(Command, OutputEvent);
    }
    else
    {
      TRACE("3");
      assert(GetCommandSessionOpened());
      assert(FCommandSession->GetFSProtocol() == cfsSCP);
      LogEvent(L"Executing user defined command on command session.");

      FCommandSession->SetCurrentDirectory(GetCurrentDirectory());
      FCommandSession->FFileSystem->AnyCommand(Command, OutputEvent);

      TRACE("3a");
      FCommandSession->FFileSystem->ReadCurrentDirectory();

      TRACE("3b");
      // synchronize pwd (by purpose we lose transaction optimalisation here)
      ChangeDirectory(FCommandSession->GetCurrentDirectory());
    }
    TRACE("4");
    ReactOnCommand(fsAnyCommand);
  }
  catch (Exception &E)
  {
    TRACE("E");
    if (Action != NULL)
    {
      RollbackAction(*Action, NULL, &E);
    }
    if (GetExceptionOnFail() || (dynamic_cast<EFatal *>(&E) != NULL)) { throw; }
    else { HandleExtendedException(&E); }
  }
  TRACE("/");
}
//---------------------------------------------------------------------------
bool /* __fastcall */ TTerminal::DoCreateLocalFile(const UnicodeString FileName,
  TFileOperationProgressType * OperationProgress, HANDLE * AHandle,
  bool NoConfirmation)
{
  CALLSTACK;
  bool Result = true;
  bool Done;
  unsigned int CreateAttr = FILE_ATTRIBUTE_NORMAL;
  do
  {
    TRACE("1");
    *AHandle = CreateLocalFile(FileName.c_str(), GENERIC_WRITE, FILE_SHARE_READ,
      CREATE_ALWAYS, CreateAttr);
    Done = (*AHandle != INVALID_HANDLE_VALUE);
    if (!Done)
    {
      TRACE("2");
      int FileAttr = 0;
      if (::FileExists(FileName) &&
        (((FileAttr = GetLocalFileAttributes(FileName)) & (faReadOnly | faHidden)) != 0))
      {
        TRACE("3");
        if (FLAGSET(FileAttr, faReadOnly))
        {
          TRACE("4");
          if (OperationProgress->BatchOverwrite == boNone)
          {
            TRACE("5");
            Result = false;
          }
          else if ((OperationProgress->BatchOverwrite != boAll) && !NoConfirmation)
          {
            TRACE("6");
            unsigned int Answer;
            SUSPEND_OPERATION
            (
              Answer = QueryUser(
                FMTLOAD(READ_ONLY_OVERWRITE, FileName.c_str()), NULL,
                qaYes | qaNo | qaCancel | qaYesToAll | qaNoToAll, 0);
            );
            TRACE("7");
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
          TRACE("8");
          assert(FLAGSET(FileAttr, faHidden));
          Result = true;
        }

        if (Result)
        {
          TRACE("9");
          CreateAttr |=
            FLAGMASK(FLAGSET(FileAttr, faHidden), FILE_ATTRIBUTE_HIDDEN) |
            FLAGMASK(FLAGSET(FileAttr, faReadOnly), FILE_ATTRIBUTE_READONLY);

          TRACE("10");
          FILE_OPERATION_LOOP (FMTLOAD(CANT_SET_ATTRS, FileName.c_str()),
            if (!SetLocalFileAttributes(FileName, FileAttr & ~(faReadOnly | faHidden)))
            {
              RaiseLastOSError();
            }
          );
          TRACE("11");
        }
        else
        {
          TRACE("12");
          Done = true;
        }
      }
      else
      {
        TRACE("13");
        RaiseLastOSError();
      }
    }
  }
  while (!Done);

  TRACE("/");
  return Result;
}
//---------------------------------------------------------------------------
bool /* __fastcall */ TTerminal::CreateLocalFile(const UnicodeString FileName,
  TFileOperationProgressType * OperationProgress, HANDLE * AHandle,
  bool NoConfirmation)
{
  CALLSTACK;
  assert(AHandle);
  bool Result = true;
  FILE_OPERATION_LOOP (FMTLOAD(CREATE_FILE_ERROR, FileName.c_str()),
    Result = DoCreateLocalFile(FileName, OperationProgress, AHandle, NoConfirmation);
  );

  TRACE("/");
  return Result;
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::OpenLocalFile(const UnicodeString FileName,
  unsigned int Access, int * AAttrs, HANDLE * AHandle, __int64 * ACTime,
  __int64 * AMTime, __int64 * AATime, __int64 * ASize,
  bool TryWriteReadOnly)
{
  CALLSTACK;
  int Attrs = 0;
  HANDLE Handle = 0;
  TFileOperationProgressType * OperationProgress = GetOperationProgress();

  TRACE("1");
  FILE_OPERATION_LOOP (FMTLOAD(FILE_NOT_EXISTS, FileName.c_str()),
    Attrs = GetLocalFileAttributes(FileName);
    if (Attrs == -1) { RaiseLastOSError(); }
  )

  if ((Attrs & faDirectory) == 0)
  {
    TRACE("2");
    bool NoHandle = false;
    if (!TryWriteReadOnly && (Access == GENERIC_WRITE) &&
        ((Attrs & faReadOnly) != 0))
    {
      TRACE("3");
      Access = GENERIC_READ;
      NoHandle = true;
    }

    TRACE("4");
    FILE_OPERATION_LOOP (FMTLOAD(OPENFILE_ERROR, FileName.c_str()),
      Handle = CreateLocalFile(FileName.c_str(), Access,
        Access == GENERIC_READ ? FILE_SHARE_READ | FILE_SHARE_WRITE : FILE_SHARE_READ,
        OPEN_EXISTING, 0);
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
        TRACE("6");
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
        TRACE("7");
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
        TRACE("8");
        CloseHandle(Handle);
        Handle = NULL;
      }
    }
    catch(...)
    {
      TRACE("9");
      CloseHandle(Handle);
      throw;
    }
  }

  TRACE("10");
  if (AAttrs) { *AAttrs = Attrs; }
  if (AHandle) { *AHandle = Handle; }
  TRACE("/");
}
//---------------------------------------------------------------------------
bool /* __fastcall */ TTerminal::AllowLocalFileTransfer(UnicodeString FileName,
  const TCopyParamType * CopyParam)
{
  CALLSTACK;
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
void /* __fastcall */ TTerminal::MakeLocalFileList(const UnicodeString & FileName,
  const TSearchRec & Rec, void * Param)
{
  TMakeLocalFileListParams & Params = *static_cast<TMakeLocalFileListParams *>(Param);

  bool Directory = FLAGSET(Rec.Attr, faDirectory);
  if (Directory && Params.Recursive)
  {
    ProcessLocalDirectory(FileName, MAKE_CALLBACK3(TTerminal::MakeLocalFileList, this), &Params);
  }

  if (!Directory || Params.IncludeDirs)
  {
    Params.FileList->Add(FileName);
  }
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::CalculateLocalFileSize(const UnicodeString & FileName,
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
      ProcessLocalDirectory(FileName, MAKE_CALLBACK3(TTerminal::CalculateLocalFileSize, this), Params);
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
  CALLSTACK;
  TFileOperationProgressType OperationProgress(MAKE_CALLBACK2(TTerminal::DoProgress, this), MAKE_CALLBACK0(TTerminal::DoFinished, this));
  TOnceDoneOperation OnceDoneOperation = odoIdle;
  OperationProgress.Start(foCalculateSize, osLocal, FileList->Count);
  TRY_FINALLY (
  {
    TCalculateSizeParams Params;
    Params.Size = 0;
    Params.Params = 0;
    Params.CopyParam = CopyParam;

    assert(!FOperationProgress);
    FOperationProgress = &OperationProgress;
    TSearchRec Rec = {0};
    for (int Index = 0; Index < FileList->Count; Index++)
    {
      UnicodeString FileName = FileList->Strings[Index];
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
  TSynchronizeDirectoryEvent OnSynchronizeDirectory;
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
  TSynchronizeDirectoryEvent OnSynchronizeDirectory,
  TSynchronizeOptions * Options)
{
  CALLSTACK;
  TSynchronizeChecklist * Checklist = new TSynchronizeChecklist();
  try
  {
    TRACE("1");
    DoSynchronizeCollectDirectory(LocalDirectory, RemoteDirectory, Mode,
      CopyParam, Params, OnSynchronizeDirectory, Options, sfFirstLevel,
      Checklist);
    TRACE("2");
    Checklist->Sort();
    TRACE("3");
  }
  catch(...)
  {
    delete Checklist;
    throw;
  }
  TRACE("4");
  return Checklist;
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::DoSynchronizeCollectDirectory(const UnicodeString LocalDirectory,
  const UnicodeString RemoteDirectory, TSynchronizeMode Mode,
  const TCopyParamType * CopyParam, int Params,
  TSynchronizeDirectoryEvent OnSynchronizeDirectory, TSynchronizeOptions * Options,
  int Flags, TSynchronizeChecklist * Checklist)
{
  CALLSTACK;
  TFileOperationProgressType * OperationProgress = GetOperationProgress();
  TSynchronizeData Data;

  TRACE("1");
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

  TRY_FINALLY (
  {
    TRACE("2");
    bool Found = false;
    TSearchRec SearchRec = {0};
    Data.LocalFileList = new TStringList();
    Data.LocalFileList->Sorted = true;
    Data.LocalFileList->CaseSensitive = false;

    FILE_OPERATION_LOOP (FMTLOAD(LIST_DIR_ERROR, LocalDirectory.c_str()),
      int FindAttrs = faReadOnly | faHidden | faSysFile | faDirectory | faArchive;
      Found = (FindFirst(Data.LocalDirectory + L"*.*", FindAttrs, SearchRec) == 0);
    );

    if (Found)
    {
      TRACE("3");
      TRY_FINALLY (
      {
        UnicodeString FileName;
        while (Found)
        {
          TRACE("4");
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
            TRACEFMT("5 [%s]", (FileName));
            TSynchronizeFileData * FileData = new TSynchronizeFileData;

            FileData->IsDirectory = FLAGSET(SearchRec.Attr, faDirectory);
            FileData->Info.FileName = FileName;
            FileData->Info.Directory = Data.LocalDirectory;
            FileData->Info.Modification = Modification;
            TRACEFMT("5d [%.7f] [%s]", (double(FileData->Info.Modification), FileData->Info.Modification.TimeString()));
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

          TRACE("6");
          FILE_OPERATION_LOOP (FMTLOAD(LIST_DIR_ERROR, LocalDirectory.c_str()),
            Found = (FindNext(SearchRec) == 0);
          );
        }
      }
      ,
      {
        TRACE("7");
        FindClose(SearchRec);
      }
      );

      TRACE("8");
      // can we expect that ProcessDirectory would take so little time
      // that we can pospone showing progress window until anything actually happens?
      bool Cached = FLAGSET(Params, spUseCache) && GetSessionData()->GetCacheDirectories() &&
        FDirectoryCache->HasFileList(RemoteDirectory);

      if (!Cached && FLAGSET(Params, spDelayProgress))
      {
        TRACE("9");
        DoSynchronizeProgress(Data, true);
      }

      TRACE("10");
      ProcessDirectory(RemoteDirectory, MAKE_CALLBACK3(TTerminal::SynchronizeCollectFile, this), &Data,
        FLAGSET(Params, spUseCache));

      TSynchronizeFileData * FileData;
      for (int Index = 0; Index < Data.LocalFileList->Count; Index++)
      {
        FileData = reinterpret_cast<TSynchronizeFileData *>
          (Data.LocalFileList->Objects[Index]);
        TRACEFMT("11 [%s]", (FileData->Info.FileName));
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
          TRACE("12");
          TSynchronizeChecklist::TItem * ChecklistItem = new TSynchronizeChecklist::TItem();
          TRY_FINALLY (
          {
            TRACE("13");
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
              TRACE("14");
              ChecklistItem->Action =
                (Modified ? TSynchronizeChecklist::saUploadUpdate : TSynchronizeChecklist::saUploadNew);
              ChecklistItem->Checked =
                (Modified || FLAGCLEAR(Params, spExistingOnly)) &&
                (!ChecklistItem->IsDirectory || FLAGCLEAR(Params, spNoRecurse) ||
                 FLAGSET(Params, spSubDirs));
            }
            else if ((Mode == smLocal) && FLAGCLEAR(Params, spTimestamp))
            {
              TRACE("15");
              ChecklistItem->Action = TSynchronizeChecklist::saDeleteLocal;
              ChecklistItem->Checked =
                FLAGSET(Params, spDelete) &&
                (!ChecklistItem->IsDirectory || FLAGCLEAR(Params, spNoRecurse) ||
                 FLAGSET(Params, spSubDirs));
            }

            if (ChecklistItem->Action != TSynchronizeChecklist::saNone)
            {
              TRACE("16");
              Data.Checklist->Add(ChecklistItem);
              ChecklistItem = NULL;
            }
          }
          ,
          {
            TRACE("17");
            delete ChecklistItem;
          }
          );
          TRACE("18");
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
    TRACE("19");
    if (Data.LocalFileList != NULL)
    {
      for (int Index = 0; Index < Data.LocalFileList->Count; Index++)
      {
        TSynchronizeFileData * FileData = reinterpret_cast<TSynchronizeFileData *>
          (Data.LocalFileList->Objects[Index]);
        delete FileData;
      }
      delete Data.LocalFileList;
    }
  }
  );
  TRACE("20");
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::SynchronizeCollectFile(const UnicodeString & FileName,
  const TRemoteFile * File, /*TSynchronizeData*/ void * Param)
{
  CALLSTACK;
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
    CTRACE(TRACE_SYNCH, "00b");
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
      int LocalIndex = Data->LocalFileList->IndexOf(LocalFileName.c_str());
      bool New = (LocalIndex < 0);
      CTRACEFMT(TRACE_SYNCH, "00b1 [%d] [%d] [%s]", (LocalIndex, int(New), LocalFileName));
      if (!New)
      {
        CTRACE(TRACE_SYNCH, "00c");
        TSynchronizeFileData * LocalData =
          reinterpret_cast<TSynchronizeFileData *>(Data->LocalFileList->Objects[LocalIndex]);

        LocalData->New = false;

        if (File->GetIsDirectory() != LocalData->IsDirectory)
        {
          CTRACE(TRACE_SYNCH, "01");
          LogEvent(FORMAT(L"%s is directory on one side, but file on the another",
            File->GetFileName().c_str()));
        }
        else if (!File->GetIsDirectory())
        {
          CTRACE(TRACE_SYNCH, "02");
          ChecklistItem->Local = LocalData->Info;

          CTRACEFMT(TRACE_SYNCH, "03 [%s] [%.7f] [%d]", (ChecklistItem->Local.Modification.TimeString(), double(ChecklistItem->Local.Modification), File->GetModificationFmt()));
          ChecklistItem->Local.Modification =
            ReduceDateTimePrecision(ChecklistItem->Local.Modification, File->GetModificationFmt());
          CTRACEFMT(TRACE_SYNCH, "04 [%s] [%.7f] [%d]", (ChecklistItem->Local.Modification.TimeString(), double(ChecklistItem->Local.Modification), File->GetModificationFmt()));

          bool LocalModified = false;
          // for spTimestamp+spBySize require that the file sizes are the same
          // before comparing file time
          int TimeCompare;
          if (FLAGCLEAR(Data->Params, spNotByTime) &&
              (FLAGCLEAR(Data->Params, spTimestamp) ||
               FLAGCLEAR(Data->Params, spBySize) ||
               (ChecklistItem->Local.Size == ChecklistItem->Remote.Size)))
          {
            CTRACE(TRACE_SYNCH, "11");
            TimeCompare = CompareFileTime(ChecklistItem->Local.Modification,
                 ChecklistItem->Remote.Modification);
          }
          else
          {
            CTRACE(TRACE_SYNCH, "12");
            TimeCompare = 0;
          }
          CTRACEFMT(TRACE_SYNCH, "[%s] TimeCompare [%d]", (File->GetFileName(), TimeCompare));
          if (TimeCompare < 0)
          {
            if ((FLAGCLEAR(Data->Params, spTimestamp) && FLAGCLEAR(Data->Params, spMirror)) ||
                (Data->Mode == smBoth) || (Data->Mode == smLocal))
            {
              CTRACE(TRACE_SYNCH, "13");
              Modified = true;
            }
            else
            {
              CTRACE(TRACE_SYNCH, "14");
              LocalModified = true;
            }
          }
          else if (TimeCompare > 0)
          {
            if ((FLAGCLEAR(Data->Params, spTimestamp) && FLAGCLEAR(Data->Params, spMirror)) ||
                (Data->Mode == smBoth) || (Data->Mode == smRemote))
            {
              CTRACE(TRACE_SYNCH, "15");
              LocalModified = true;
            }
            else
            {
              CTRACE(TRACE_SYNCH, "16");
              Modified = true;
            }
          }
          else if (FLAGSET(Data->Params, spBySize) &&
                   (ChecklistItem->Local.Size != ChecklistItem->Remote.Size) &&
                   FLAGCLEAR(Data->Params, spTimestamp))
          {
            CTRACE(TRACE_SYNCH, "17");
            Modified = true;
            LocalModified = true;
          }

          if (LocalModified)
          {
            CTRACE(TRACE_SYNCH, "18");
            LocalData->Modified = true;
            LocalData->MatchingRemoteFile = ChecklistItem->Remote;
            LocalData->MatchingRemoteFileImageIndex = ChecklistItem->ImageIndex;
            // we need this for custom commands over checklist only,
            // not for sync itself
            LocalData->MatchingRemoteFileFile = File->Duplicate();
            LogEvent(FORMAT(L"Local file '%s' [%s] [%s] is modifed comparing to remote file '%s' [%s] [%s]",
              UnicodeString(LocalData->Info.Directory + LocalData->Info.FileName).c_str(),
               StandardTimestamp(LocalData->Info.Modification).c_str(),
               Int64ToStr(LocalData->Info.Size).c_str(),
               FullRemoteFileName.c_str(),
               StandardTimestamp(File->GetModification()).c_str(),
               Int64ToStr(File->GetSize()).c_str()));
          }

          if (Modified)
          {
            LogEvent(FORMAT(L"Remote file '%s' [%s] [%s] is modifed comparing to local file '%s' [%s] [%s]",
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
        CTRACE(TRACE_SYNCH, "19");
        ChecklistItem->Local.Directory = Data->LocalDirectory;
        LogEvent(FORMAT(L"Remote file '%s' [%s] [%s] is new",
          FullRemoteFileName.c_str(), StandardTimestamp(File->GetModification()).c_str(), Int64ToStr(File->GetSize()).c_str()));
      }

      if (New || Modified)
      {
        CTRACE(TRACE_SYNCH, "20");
        assert(!New || !Modified);

        // download the file if it changed or is new and we want to have it locally
        if ((Data->Mode == smBoth) || (Data->Mode == smLocal))
        {
          CTRACE(TRACE_SYNCH, "21");
          if (FLAGCLEAR(Data->Params, spTimestamp) || Modified)
          {
            CTRACE(TRACE_SYNCH, "22");
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
          CTRACE(TRACE_SYNCH, "23");
          if (FLAGCLEAR(Data->Params, spTimestamp))
          {
            CTRACE(TRACE_SYNCH, "24");
            ChecklistItem->Action = TSynchronizeChecklist::saDeleteRemote;
            ChecklistItem->Checked =
              FLAGSET(Data->Params, spDelete) &&
              (!ChecklistItem->IsDirectory || FLAGCLEAR(Data->Params, spNoRecurse) ||
               FLAGSET(Data->Params, spSubDirs));
          }
        }

        if (ChecklistItem->Action != TSynchronizeChecklist::saNone)
        {
          CTRACE(TRACE_SYNCH, "25");
          ChecklistItem->RemoteFile = File->Duplicate();
          Data->Checklist->Add(ChecklistItem);
          ChecklistItem = NULL;
        }
      }
    }
    ,
    {
      CTRACE(TRACE_SYNCH, "26");
      delete ChecklistItem;
    }
    );
  }
  else
  {
    LogEvent(FORMAT(L"Remote file '%s' [%s] [%s] excluded from synchronization",
      FullRemoteFileName.c_str(), StandardTimestamp(File->GetModification()).c_str(), Int64ToStr(File->GetSize()).c_str()));
  }
  TRACE("/");
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::SynchronizeApply(TSynchronizeChecklist * Checklist,
  const UnicodeString LocalDirectory, const UnicodeString RemoteDirectory,
  const TCopyParamType * CopyParam, int Params,
  TSynchronizeDirectoryEvent OnSynchronizeDirectory)
{
  CALLSTACK;
  TSynchronizeData Data;

  TRACE("TTerminal::SynchronizeApply 1");
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

  TRACE("TTerminal::SynchronizeApply 2");
  TRY_FINALLY (
  {
    TRACE("TTerminal::SynchronizeApply 3");
    int IIndex = 0;
    while (IIndex < Checklist->GetCount())
    {
      TRACE("TTerminal::SynchronizeApply 4");
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

      TRACE("TTerminal::SynchronizeApply 5");
      while ((IIndex < Checklist->GetCount()) &&
             (Checklist->GetItem(IIndex)->Local.Directory == CurrentLocalDirectory) &&
             (Checklist->GetItem(IIndex)->Remote.Directory == CurrentRemoteDirectory))
      {
        TRACE("TTerminal::SynchronizeApply 6");
        ChecklistItem = Checklist->GetItem(IIndex);
        if (ChecklistItem->Checked)
        {
          TRACE("TTerminal::SynchronizeApply 7");
          Count++;

          if (FLAGSET(Params, spTimestamp))
          {
            TRACE("TTerminal::SynchronizeApply 8");
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
            TRACE("TTerminal::SynchronizeApply 9");
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

      TRACE("TTerminal::SynchronizeApply 10");
      // prevent showing/updating of progress dialog if there's nothing to do
      if (Count > 0)
      {
        TRACE("TTerminal::SynchronizeApply 11");
        Data.LocalDirectory = IncludeTrailingBackslash(CurrentLocalDirectory);
        Data.RemoteDirectory = UnixIncludeTrailingBackslash(CurrentRemoteDirectory);
        DoSynchronizeProgress(Data, false);

        if (FLAGSET(Params, spTimestamp))
        {
          TRACE("TTerminal::SynchronizeApply 12");
          if (DownloadList->Count > 0)
          {
            ProcessFiles(DownloadList, foSetProperties,
              MAKE_CALLBACK3(TTerminal::SynchronizeLocalTimestamp, this), NULL, osLocal);
          }

          if (UploadList->Count > 0)
          {
            ProcessFiles(UploadList, foSetProperties,
              MAKE_CALLBACK3(TTerminal::SynchronizeRemoteTimestamp, this));
          }
        }
        else
        {
          TRACE("TTerminal::SynchronizeApply 13");
          if ((DownloadList->Count > 0) &&
              !CopyToLocal(DownloadList, Data.LocalDirectory, &SyncCopyParam, CopyParams))
          {
            Abort();
          }

          if ((DeleteRemoteList->Count > 0) &&
              !DeleteFiles(DeleteRemoteList))
          {
            Abort();
          }

          if ((UploadList->Count > 0) &&
              !CopyToRemote(UploadList, Data.RemoteDirectory, &SyncCopyParam, CopyParams))
          {
            Abort();
          }

          if ((DeleteLocalList->Count > 0) &&
              !DeleteLocalFiles(DeleteLocalList))
          {
            Abort();
          }
          TRACE("TTerminal::SynchronizeApply 14");
        }
      }
    }
  }
  ,
  {
    TRACE("TTerminal::SynchronizeApply 15");
    delete DownloadList;
    delete DeleteRemoteList;
    delete UploadList;
    delete DeleteLocalList;

    EndTransaction();
  }
  );
  TRACE("TTerminal::SynchronizeApply 16");
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::DoSynchronizeProgress(const TSynchronizeData & Data,
  bool Collect)
{
  CALLSTACK;
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
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::SynchronizeLocalTimestamp(const UnicodeString & /*FileName*/,
  const TRemoteFile * File, void * /*Param*/)
{
  CALLSTACK;
  const TSynchronizeChecklist::TItem * ChecklistItem =
    reinterpret_cast<const TSynchronizeChecklist::TItem *>(File);

  UnicodeString LocalFile =
    IncludeTrailingBackslash(ChecklistItem->Local.Directory) +
      ChecklistItem->Local.FileName;
  SetLocalFileTime(LocalFile, ChecklistItem->Remote.Modification);
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::SynchronizeRemoteTimestamp(const UnicodeString & /*FileName*/,
  const TRemoteFile * File, void * /*Param*/)
{
  CALLSTACK;
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
void /* __fastcall */ TTerminal::FileFind(const UnicodeString & FileName,
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
    bool ImplicitMatch;
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
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminal::DoFilesFind(UnicodeString Directory, TFilesFindParams & Params)
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
      ProcessDirectory(Directory, MAKE_CALLBACK3(TTerminal::FileFind, this), &Params, false, true);
    }
    ,
    {
      FOnFindingFile = NULL;
    }
    );
  }
}
//---------------------------------------------------------------------------
void __fastcall TTerminal::FilesFind(UnicodeString Directory, const TFileMasks & FileMask,
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
void /* __fastcall */ TTerminal::SpaceAvailable(const UnicodeString Path,
  TSpaceAvailable & ASpaceAvailable)
{
  CALLSTACK;
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
const TSessionInfo & /* __fastcall */ TTerminal::GetSessionInfo() const
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
  CALLSTACK;
  UnicodeString Result;
  // FPassword is empty also when stored password was used
  TRACEFMT("1 [%x] [%d]", (int(this), FPassword.Length()));
  if (FPassword.IsEmpty())
  {
    TRACE("1");
    Result = GetSessionData()->GetPassword();
  }
  else
  {
    TRACE("2");
    Result = DecryptPassword(FPassword);
  }
  TRACE("/");
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
  CALLSTACK;
  assert(FFileSystem);
  assert(FilesToCopy);

  TRACEFMT("0 [%s] [%x]", (TargetDir, Params));
  assert(GetIsCapable(fcNewerOnlyUpload) || FLAGCLEAR(Params, cpNewerOnly));

  bool Result = false;
  TOnceDoneOperation OnceDoneOperation = odoIdle;

  TFileOperationProgressType OperationProgress(MAKE_CALLBACK2(TTerminal::DoProgress, this), MAKE_CALLBACK6(TTerminal::DoFinished, this));
  try
  {

    __int64 Size = 0;
    if (CopyParam->GetCalculateSize())
    {
      TRACE("2");
      // dirty trick: when moving, do not pass copy param to avoid exclude mask
      CalculateLocalFilesSize(FilesToCopy, Size,
        (FLAGCLEAR(Params, cpDelete) ? CopyParam : NULL));
    }

    OperationProgress.Start((Params & cpDelete ? foMove : foCopy), osLocal,
      FilesToCopy->Count, (Params & cpTemporary) > 0, TargetDir, CopyParam->GetCPSLimit());

    FOperationProgress = &OperationProgress;
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
        TRACE("3");
        if (GetLog()->GetLogging())
        {
          LogEvent(FORMAT(L"Copying %d files/directories to remote directory "
            L"\"%s\"", FilesToCopy->Count.get(), TargetDir.c_str()));
          LogEvent(CopyParam->GetLogStr());
        }

        FFileSystem->CopyToRemote(FilesToCopy, UnlockedTargetDir,
          CopyParam, Params, &OperationProgress, OnceDoneOperation);
      }
      ,
      {
        TRACE("4");
        if (GetActive())
        {
          TRACE("5");
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
      TRACE("6");
      OperationProgress.Stop();
      FOperationProgress = NULL;
    }
    );
  }
  catch (Exception &E)
  {
    TRACE("7");
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

  TRACE("/");
  return Result;
}
//---------------------------------------------------------------------------
bool /* __fastcall */ TTerminal::CopyToLocal(TStrings * FilesToCopy,
  const UnicodeString TargetDir, const TCopyParamType * CopyParam, int Params)
{
  CALLSTACK;
  assert(FFileSystem);

  TRACEFMT("0 [%s] [%s] [%x]", (TargetDir, CopyParam->GetLogStr(), Params));
  // see scp.c: sink(), tolocal()

  bool Result = false;
  bool OwnsFileList = (FilesToCopy == NULL);
  TOnceDoneOperation OnceDoneOperation = odoIdle;

  TRY_FINALLY (
  {
    if (OwnsFileList)
    {
      FilesToCopy = new TStringList();
      FilesToCopy->Assign(GetFiles()->GetSelectedFiles());
    }

    TRACE("1");
    BeginTransaction();
    TRY_FINALLY (
    {
      __int64 TotalSize = 0;
      bool TotalSizeKnown = false;
      TFileOperationProgressType OperationProgress(MAKE_CALLBACK2(TTerminal::DoProgress, this), MAKE_CALLBACK6(TTerminal::DoFinished, this));

      if (CopyParam->GetCalculateSize())
      {
        TRACE("2");
        SetExceptionOnFail(true);
        TRY_FINALLY (
        {
          // dirty trick: when moving, do not pass copy param to avoid exclude mask
          TRACE("3");
          CalculateFilesSize(FilesToCopy, TotalSize, csIgnoreErrors,
            (FLAGCLEAR(Params, cpDelete) ? CopyParam : NULL));
          TotalSizeKnown = true;
          TRACE("4");
        }
        ,
        {
          TRACE("5");
          SetExceptionOnFail(false);
        }
        );
      }

      TRACE("6");
      OperationProgress.Start(((Params & cpDelete) != 0 ? foMove : foCopy), osRemote,
        FilesToCopy->Count, (Params & cpTemporary) != 0, TargetDir, CopyParam->GetCPSLimit());

      FOperationProgress = &OperationProgress;
      TRY_FINALLY (
      {
        if (TotalSizeKnown)
        {
          TRACE("7");
          OperationProgress.SetTotalSize(TotalSize);
        }

        try
        {
          TRY_FINALLY (
          {
            TRACE("8");
            FFileSystem->CopyToLocal(FilesToCopy, TargetDir, CopyParam, Params,
              &OperationProgress, OnceDoneOperation);
            TRACE("9");
          }
          ,
          {
            TRACE("10");
            if (GetActive())
            {
              ReactOnCommand(fsCopyToLocal);
            }
          }
          );
        }
        catch (Exception &E)
        {
          TRACE("11");
          if (OperationProgress.Cancel != csCancel)
          {
            CommandError(&E, LoadStr(TOLOCAL_COPY_ERROR));
          }
          OnceDoneOperation = odoIdle;
        }

        TRACE("12");
        if (OperationProgress.Cancel == csContinue)
        {
          Result = true;
        }
      }
      ,
      {
        TRACE("13");
        FOperationProgress = NULL;
        OperationProgress.Stop();
      }
      );
      TRACE("14");
    }
    ,
    {
      TRACE("15");
      // If session is still active (no fatal error) we reload directory
      // by calling EndTransaction
      EndTransaction();
    }
    );

  }
  ,
  {
    TRACE("16");
    if (OwnsFileList) delete FilesToCopy;
  }
  );

  TRACE("17");
  if (OnceDoneOperation != odoIdle)
  {
    CloseOnCompletion(OnceDoneOperation);
  }

  return Result;
}
//---------------------------------------------------------------------------
void __fastcall TTerminal::SetLocalFileTime(const UnicodeString & LocalFileName,
  const TDateTime & Modification)
{
  FILETIME WrTime = DateTimeToFileTime(Modification,
    GetSessionData()->GetDSTMode());
  SetLocalFileTime(LocalFileName, NULL, &WrTime);
}
//---------------------------------------------------------------------------
void __fastcall TTerminal::SetLocalFileTime(const UnicodeString & LocalFileName,
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
//---------------------------------------------------------------------------
HANDLE __fastcall TTerminal::CreateLocalFile(const UnicodeString & LocalFileName, DWORD DesiredAccess,
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
//---------------------------------------------------------------------------
DWORD __fastcall TTerminal::GetLocalFileAttributes(const UnicodeString & LocalFileName)
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
//---------------------------------------------------------------------------
BOOL __fastcall TTerminal::SetLocalFileAttributes(const UnicodeString & LocalFileName, DWORD FileAttributes)
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
//---------------------------------------------------------------------------
BOOL __fastcall TTerminal::MoveLocalFile(const UnicodeString & LocalFileName, const UnicodeString & NewLocalFileName, DWORD Flags)
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
//---------------------------------------------------------------------------
BOOL __fastcall TTerminal::RemoveLocalDirectory(const UnicodeString & LocalDirName)
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
//---------------------------------------------------------------------------
BOOL __fastcall TTerminal::CreateLocalDirectory(const UnicodeString & LocalDirName, LPSECURITY_ATTRIBUTES SecurityAttributes)
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
/* __fastcall */ TSecondaryTerminal::TSecondaryTerminal(TTerminal * MainTerminal) :
  TTerminal(),
  FMasterPasswordTried(false),
  FMasterTunnelPasswordTried(false),
  FMainTerminal(MainTerminal)
{
}

void __fastcall TSecondaryTerminal::Init(
  TSessionData * ASessionData, TConfiguration * Configuration, const UnicodeString & Name)
{
  TTerminal::Init(ASessionData, Configuration);
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
//---------------------------------------------------------------------------
void /* __fastcall */ TSecondaryTerminal::DirectoryLoaded(TRemoteFileList * FileList)
{
  CALLSTACK;
  FMainTerminal->DirectoryLoaded(FileList);
  assert(FileList != NULL);
  TRACE("/");
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
  CALLSTACK;
  bool AResult = false;

  TRACEFMT("1 [%d] [%d]", (Prompts->Count, int(Prompts->Objects[0]), int(Kind)));
  if ((Prompts->Count == 1) && !(Prompts->Objects[0]) &&
      ((Kind == pkPassword) || (Kind == pkPassphrase) || (Kind == pkKeybInteractive) ||
       (Kind == pkTIS) || (Kind == pkCryptoCard)))
  {
    bool & PasswordTried =
      FTunnelOpening ? FMasterTunnelPasswordTried : FMasterPasswordTried;
    TRACEFMT("2 [%d] [%d]", (int(FTunnelOpening), int(PasswordTried)));
    if (!PasswordTried)
    {
      TRACE("3");
      // let's expect that the main session is already authenticated and its password
      // is not written after, so no locking is necessary
      // (no longer true, once the main session can be reconnected)
      UnicodeString Password;
      if (FTunnelOpening)
      {
        TRACE("3a");
        Password = FMainTerminal->GetTunnelPassword();
      }
      else
      {
        TRACE("3b");
        Password = FMainTerminal->GetPassword();
      }
      Results->Strings[0] = Password;
      if (!Results->Strings[0].IsEmpty())
      {
        LogEvent(L"Using remembered password of the main session.");
        AResult = true;
      }
      PasswordTried = true;
    }
  }

  if (!AResult)
  {
    TRACE("4");
    AResult = TTerminal::DoPromptUser(Data, Kind, Name, Instructions, Prompts, Results);
  }

  TRACE("/");
  return AResult;
}
//---------------------------------------------------------------------------
/* __fastcall */ TTerminalList::TTerminalList(TConfiguration * AConfiguration) :
  TObjectList(),
  FConfiguration(NULL)
{
  assert(AConfiguration);
  FConfiguration = AConfiguration;
}
//---------------------------------------------------------------------------
/* __fastcall */ TTerminalList::~TTerminalList()
{
  assert(Count == 0);
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
  CALLSTACK;
  assert(IndexOf(Terminal) >= 0);
  Remove(Terminal);
  TRACE("/");
}
//---------------------------------------------------------------------------
void /* __fastcall */ TTerminalList::FreeAndNullTerminal(TTerminal * & Terminal)
{
  TTerminal * T = Terminal;
  Terminal = NULL;
  FreeTerminal(T);
}
//---------------------------------------------------------------------------
TTerminal * /* __fastcall */ TTerminalList::GetTerminal(int Index)
{
  return dynamic_cast<TTerminal *>(Items[Index]);
}
//---------------------------------------------------------------------------
int /* __fastcall */ TTerminalList::GetActiveCount()
{
  int Result = 0;
  TTerminal * Terminal;
  for (int i = 0; i < Count; i++)
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
  for (int i = 0; i < Count; i++)
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
  for (int Index = 0; Index < Count; Index++)
  {
    GetTerminal(Index)->RecryptPasswords();
  }
}
//---------------------------------------------------------------------------
UnicodeString GetSessionUrl(const TTerminal * Terminal)
{
  const TSessionInfo & SessionInfo = Terminal->GetSessionInfo() ;
  UnicodeString Protocol = SessionInfo.ProtocolBaseName;
  UnicodeString HostName = Terminal->GetSessionData()->GetHostNameExpanded();
  int Port = Terminal->GetSessionData()->GetPortNumber();
  UnicodeString SessionUrl = FORMAT(L"%s://%s:%d", Protocol.Lower().c_str(), HostName.c_str(), Port);
  return SessionUrl;
}
//---------------------------------------------------------------------------
