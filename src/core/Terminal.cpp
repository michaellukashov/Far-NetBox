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
  const filefound_slot_type *OnFileFound;
  const findingfile_slot_type *OnFindingFile;
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
const UnicodeString TSynchronizeChecklist::TItem::GetFileName() const
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
    for (size_t Index = 0; Index < FList->GetCount(); Index++)
    {
        delete reinterpret_cast<TItem *>(static_cast<void *>(FList->GetItem(Index)));
    }
    delete FList;
}
//---------------------------------------------------------------------------
void TSynchronizeChecklist::Add(TItem *Item)
{
    FList->Add(reinterpret_cast<TObject *>(static_cast<void *>(Item)));
}
//---------------------------------------------------------------------------
int TSynchronizeChecklist::Compare(void *AItem1, void *AItem2)
{
    TItem *Item1 = reinterpret_cast<TItem *>(AItem1);
    TItem *Item2 = reinterpret_cast<TItem *>(AItem2);

    int Result;
    if (!Item1->Local.Directory.IsEmpty())
    {
        Result = CompareText(Item1->Local.Directory, Item2->Local.Directory);
    }
    else
    {
        assert(!Item1->Remote.Directory.IsEmpty());
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
size_t TSynchronizeChecklist::GetCount() const
{
    return FList->GetCount();
}
//---------------------------------------------------------------------------
const TSynchronizeChecklist::TItem *TSynchronizeChecklist::GetItem(size_t Index) const
{
    return reinterpret_cast<TItem *>(FList->GetItem(Index));
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
class TTunnelThread : public TSimpleThread
{
public:
    explicit TTunnelThread(TSecureShell *SecureShell);
    virtual ~TTunnelThread();

    virtual void __fastcall Init();
    virtual void __fastcall Terminate();

protected:
    virtual void __fastcall Execute();

private:
    TSecureShell *FSecureShell;
    bool FTerminated;
    TTunnelThread *Self;
};
//---------------------------------------------------------------------------
TTunnelThread::TTunnelThread(TSecureShell *SecureShell) :
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
TTunnelThread::~TTunnelThread()
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
    // DEBUG_PRINTF(L"begin");
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
        DEBUG_PRINTF(L"exception cought");
        // do not pass Exception out of thread's proc
    }
    // DEBUG_PRINTF(L"end");
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
class TTunnelUI : public TSessionUI
{
public:
    explicit TTunnelUI(TTerminal *Terminal);
    virtual ~TTunnelUI()
    {}
    virtual void __fastcall Information(const UnicodeString Str, bool Status);
    virtual int __fastcall QueryUser(const UnicodeString Query,
                          TStrings *MoreMessages, int Answers, const TQueryParams *Params,
                          TQueryType QueryType);
    virtual int __fastcall QueryUserException(const UnicodeString Query,
                                   const Exception *E, int Answers, const TQueryParams *Params,
                                   TQueryType QueryType);
    virtual bool __fastcall PromptUser(TSessionData *Data, TPromptKind Kind,
                            const UnicodeString Name, const UnicodeString Instructions, TStrings *Prompts,
                            TStrings *Results);
    virtual void __fastcall DisplayBanner(const UnicodeString Banner);
    virtual void __fastcall FatalError(const Exception *E, const UnicodeString Msg);
    virtual void __fastcall HandleExtendedException(const Exception *E);
    virtual void __fastcall Closed();

private:
    TTerminal *FTerminal;
    unsigned int FTerminalThread;
};
//---------------------------------------------------------------------------
TTunnelUI::TTunnelUI(TTerminal *Terminal)
{
    FTerminal = Terminal;
    FTerminalThread = GetCurrentThreadId();
}
//---------------------------------------------------------------------------
void __fastcall TTunnelUI::Information(const UnicodeString Str, bool Status)
{
    if (GetCurrentThreadId() == FTerminalThread)
    {
        FTerminal->Information(Str, Status);
    }
}
//---------------------------------------------------------------------------
int __fastcall TTunnelUI::QueryUser(const UnicodeString Query,
                         TStrings *MoreMessages, int Answers, const TQueryParams *Params,
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
int __fastcall TTunnelUI::QueryUserException(const UnicodeString Query,
                                  const Exception *E, int Answers, const TQueryParams *Params,
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
bool __fastcall TTunnelUI::PromptUser(TSessionData *Data, TPromptKind Kind,
                           const UnicodeString Name, const UnicodeString Instructions, TStrings *Prompts, TStrings *Results)
{
    bool Result;
    UnicodeString instructions = Instructions;
    if (GetCurrentThreadId() == FTerminalThread)
    {
        if (IsAuthenticationPrompt(Kind))
        {
            instructions = LoadStr(TUNNEL_INSTRUCTION) +
                           (instructions.IsEmpty() ? L"" : L"\n") + instructions;
        }

        Result = FTerminal->PromptUser(Data, Kind, Name, instructions, Prompts, Results);
    }
    else
    {
        Result = false;
    }
    return Result;
}
//---------------------------------------------------------------------------
void __fastcall TTunnelUI::DisplayBanner(const UnicodeString Banner)
{
    if (GetCurrentThreadId() == FTerminalThread)
    {
        FTerminal->DisplayBanner(Banner);
    }
}
//---------------------------------------------------------------------------
void __fastcall TTunnelUI::FatalError(const Exception *E, const UnicodeString Msg)
{
    throw ESshFatal(Msg, E);
}
//---------------------------------------------------------------------------
void __fastcall TTunnelUI::HandleExtendedException(const Exception *E)
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
    inline TCallbackGuard(TTerminal *FTerminal);
    inline ~TCallbackGuard();

    void __fastcall FatalError(const Exception *E, const UnicodeString Msg);
    inline void __fastcall Verify();
    void __fastcall Dismiss();

private:
    ExtException *FFatalError;
    TTerminal *FTerminal;
    bool FGuarding;
};
//---------------------------------------------------------------------------
TCallbackGuard::TCallbackGuard(TTerminal *Terminal) :
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
void __fastcall TCallbackGuard::FatalError(const Exception *E, const UnicodeString Msg)
{
    assert(FGuarding);

    // make sure we do not bother about getting back the silent abort Exception
    // we issued outselves. this may happen when there is an Exception handler
    // that converts any Exception to fatal one (such as in TTerminal::Open).
    if (dynamic_cast<const ECallbackGuardAbort *>(E) == NULL)
    {
        assert(FFatalError == NULL);

        FFatalError = new ExtException(Msg, E);
    }

    // silently abort what we are doing.
    // non-silent Exception would be catched probably by default application
    // Exception handler, which may not do an appropriate action
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
TTerminal::TTerminal() :
    TObject(),
    TSessionUI()
{
}

void __fastcall TTerminal::Init(TSessionData *SessionData, TConfiguration *Configuration)
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
    FOperationProgress = NULL;
    FClosedOnCompletion = NULL;
    FTunnel = NULL;
    FAnyInformation = false;;
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
            catch (const Exception &E)
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
UnicodeString __fastcall TTerminal::EncryptPassword(const UnicodeString Password)
{
    return Configuration->EncryptPassword(Password, GetSessionData()->GetSessionName());
}
//---------------------------------------------------------------------
UnicodeString __fastcall TTerminal::DecryptPassword(const UnicodeString Password)
{
    UnicodeString Result;
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
void __fastcall TTerminal::RecryptPasswords()
{
    FSessionData->RecryptPasswords();
    FPassword = EncryptPassword(DecryptPassword(FPassword));
    FTunnelPassword = EncryptPassword(DecryptPassword(FTunnelPassword));
}
//---------------------------------------------------------------------------
bool TTerminal::IsAbsolutePath(const UnicodeString Path)
{
    return !Path.IsEmpty() && Path[0] == '/';
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TTerminal::ExpandFileName(const UnicodeString Path,
                                       const UnicodeString BasePath)
{
    UnicodeString path = UnixExcludeTrailingBackslash(Path);
    if (!IsAbsolutePath(path) && !BasePath.IsEmpty())
    {
        // TODO: Handle more complicated cases like "../../xxx"
        if (path == PARENTDIRECTORY)
        {
            path = UnixExcludeTrailingBackslash(UnixExtractFilePath(
                                                    UnixExcludeTrailingBackslash(BasePath)));
        }
        else
        {
            path = UnixIncludeTrailingBackslash(BasePath) + path;
        }
    }
    return path;
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
void __fastcall TTerminal::Open()
{
    // DEBUG_PRINTF(L"begin");
    FLog->ReflectSettings();
    FActionLog->ReflectSettings();
    bool Reopen = false;
    do
    {
        Reopen = false;
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
                                    catch (const Exception &E)
                                    {
                                        assert(!FSecureShell->GetActive());
                                        if (!FSecureShell->GetActive() && !FTunnelError.IsEmpty())
                                        {
                                            // the only case where we expect this to happen
                                            assert(E.what() == W2MB(LoadStr(UNEXPECTED_CLOSE_ERROR).c_str()).c_str());
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
                    if (Self->FDirectoryChangesCache != NULL)
                    {
                        delete Self->FDirectoryChangesCache;
                        Self->FDirectoryChangesCache = NULL;
                    }
                    throw;
                }
            }
        }
        catch (EFatal &E)
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
        catch (const Exception &E)
        {
            // any Exception while opening session is fatal
            FatalError(&E, L"");
        }
    } while (Reopen);
    FSessionData->SetNumberOfRetries(0);
    // DEBUG_PRINTF(L"end");
}
//---------------------------------------------------------------------------
bool __fastcall TTerminal::IsListenerFree(size_t PortNumber)
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
        {
            BOOST_SCOPE_EXIT ( (&Self) )
            {
                Self->FTunnelOpening = false;
            } BOOST_SCOPE_EXIT_END
            FTunnel->Open();
        }

        FTunnelThread = new TTunnelThread(FTunnel);
        FTunnelThread->Init();
    }
    catch (...)
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
void TTerminal::Closed()
{
    if (FTunnel != NULL)
    {
        CloseTunnel();
    }

    if (!GetOnClose().IsEmpty())
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
    UnicodeString PrevRemoteDirectory = GetSessionData()->GetRemoteDirectory();
    bool PrevReadCurrentDirectoryPending = FReadCurrentDirectoryPending;
    bool PrevReadDirectoryPending = FReadDirectoryPending;
    assert(!FSuspendTransaction);
    bool PrevAutoReadDirectory = FAutoReadDirectory;
    // here used to be a check for FExceptionOnFail being 0
    // but it can happen, e.g. when we are downloading file to execute it.
    // however I'm not sure why we mind having excaption-on-fail enabled here
    int PrevExceptionOnFail = FExceptionOnFail;
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
}
//---------------------------------------------------------------------------
bool TTerminal::PromptUser(TSessionData *Data, TPromptKind Kind,
                           const UnicodeString Name, const UnicodeString Instructions, const UnicodeString Prompt, bool Echo, size_t MaxLen, UnicodeString &Result)
{
    bool AResult;
    TStringList Prompts;
    TStringList Results;
    {
        Prompts.AddObject(Prompt, reinterpret_cast<TObject *>(static_cast<size_t>(Echo)));
        Results.AddObject(Result, reinterpret_cast<TObject *>(MaxLen));

        AResult = PromptUser(Data, Kind, Name, Instructions, &Prompts, &Results);

        Result = Results.GetString(0);
    }

    return AResult;
}
//---------------------------------------------------------------------------
bool TTerminal::PromptUser(TSessionData *Data, TPromptKind Kind,
                           const UnicodeString Name, const UnicodeString Instructions, TStrings *Prompts, TStrings *Results)
{
    // If PromptUser is overriden in descendant class, the overriden version
    // is not called when accessed via TSessionIU interface.
    // So this is workaround.
    return DoPromptUser(Data, Kind, Name, Instructions, Prompts, Results);
}
//---------------------------------------------------------------------------
bool TTerminal::DoPromptUser(TSessionData * /*Data*/, TPromptKind Kind,
                             const UnicodeString Name, const UnicodeString Instructions, TStrings *Prompts, TStrings *Results)
{
    bool AResult = false;

    if (!GetOnPromptUser().IsEmpty())
    {
        TCallbackGuard Guard(this);
        GetOnPromptUser()(this, Kind, Name, Instructions, Prompts, Results, AResult, NULL);
        Guard.Verify();
    }

    if (AResult && (Configuration->GetRememberPassword()) &&
            (Prompts->GetCount() == 1) && !(Prompts->GetObject(0) != NULL) &&
            ((Kind == pkPassword) || (Kind == pkPassphrase) || (Kind == pkKeybInteractive) ||
             (Kind == pkTIS) || (Kind == pkCryptoCard)))
    {
        UnicodeString EncryptedPassword = EncryptPassword(Results->GetString(0));
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
int TTerminal::QueryUser(const UnicodeString Query,
                         TStrings *MoreMessages, int Answers, const TQueryParams *Params,
                         TQueryType QueryType)
{
    LogEvent(FORMAT(L"Asking user:\n%s (%s)", Query.c_str(), MoreMessages ? MoreMessages->GetCommaText().c_str() : L""));
    int Answer = AbortAnswer(Answers);
    if (!FOnQueryUser.IsEmpty())
    {
        TCallbackGuard Guard(this);
        FOnQueryUser(this, Query, MoreMessages, Answers, Params, Answer, QueryType, NULL);
        Guard.Verify();
    }
    return Answer;
}
//---------------------------------------------------------------------------
int TTerminal::QueryUserException(const UnicodeString Query,
                                  const Exception *E, int Answers, const TQueryParams *Params,
                                  TQueryType QueryType)
{
    int Result;
    TStringList MoreMessages;
    // DEBUG_PRINTF(L"E->what = %s", MB2W(E->what()).c_str());

    if ((E != NULL) && !std::string(E->what()).IsEmpty() && !Query.IsEmpty())
    {
        MoreMessages.Add(UnicodeString(MB2W(E->what())));
    }
    const ExtException *EE = dynamic_cast<const ExtException *>(E);
    if ((EE != NULL) && (EE->GetMoreMessages() != NULL))
    {
        MoreMessages.AddStrings(EE->GetMoreMessages());
    }
    Result = QueryUser(!Query.IsEmpty() ? Query : UnicodeString(MB2W(E->what())),
                       MoreMessages.GetCount() ? &MoreMessages : NULL,
                       Answers, Params, QueryType);
    return Result;
}
//---------------------------------------------------------------------------
int TTerminal::QueryUserException(const UnicodeString Query,
                                  const ExtException *E, int Answers, const TQueryParams *Params,
                                  TQueryType QueryType)
{
    int Result;
    TStringList MoreMessages;
    if (E != NULL)
    {
        if (E->GetMoreMessages() != NULL)
        {
            MoreMessages.AddStrings(E->GetMoreMessages());
        }
        else if (!std::string(E->what()).IsEmpty() && !Query.IsEmpty())
        {
            MoreMessages.Add(UnicodeString(MB2W(E->what())));
        }
    }
    Result = QueryUser(!Query.IsEmpty() ? Query : UnicodeString(MB2W(E->what())),
                       MoreMessages.GetCount() ? &MoreMessages : NULL,
                       Answers, Params, QueryType);
    return Result;
}
//---------------------------------------------------------------------------
void TTerminal::DisplayBanner(const UnicodeString Banner)
{
    if (!GetOnDisplayBanner().IsEmpty())
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
void TTerminal::HandleExtendedException(const Exception *E)
{
    GetLog()->AddException(E);
    if (!GetOnShowExtendedException().IsEmpty())
    {
        TCallbackGuard Guard(this);
        // the event handler may destroy 'this' ...
        GetOnShowExtendedException()(this, E, NULL);
        // .. hence guard is dismissed from destructor, to make following call no-op
        Guard.Verify();
    }
}
//---------------------------------------------------------------------------
void TTerminal::ShowExtendedException(const Exception *E)
{
    GetLog()->AddException(E);
    if (!GetOnShowExtendedException().IsEmpty())
    {
        GetOnShowExtendedException()(this, E, NULL);
    }
}
//---------------------------------------------------------------------------
void TTerminal::DoInformation(const UnicodeString Str, bool Status,
                              bool Active)
{
    if (Active)
    {
        FAnyInformation = true;
    }

    if (!GetOnInformation().IsEmpty())
    {
        TCallbackGuard Guard(this);
        GetOnInformation()(this, Str, Status, Active);
        Guard.Verify();
    }
}
//---------------------------------------------------------------------------
void TTerminal::Information(const UnicodeString Str, bool Status)
{
    DoInformation(Str, Status);
}
//---------------------------------------------------------------------------
void TTerminal::DoProgress(TFileOperationProgressType &ProgressData,
                           TCancelStatus &Cancel)
{
    if (!GetOnProgress().IsEmpty())
    {
        TCallbackGuard Guard(this);
        GetOnProgress()(ProgressData, Cancel);
        Guard.Verify();
    }
}
//---------------------------------------------------------------------------
void TTerminal::DoFinished(TFileOperation Operation, TOperationSide Side, bool Temp,
                           const UnicodeString FileName, bool Success, TOnceDoneOperation &OnceDoneOperation)
{
    if (!GetOnFinished().IsEmpty())
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
UnicodeString TTerminal::AbsolutePath(const UnicodeString Path, bool Local)
{
    return FFileSystem->AbsolutePath(Path, Local);
}
//---------------------------------------------------------------------------
void TTerminal::ReactOnCommand(int /*TFSCommand*/ Cmd)
{
    bool ChangesDirectory = false;
    bool ModifiesFiles = false;

    switch (static_cast<TFSCommand>(Cmd))
    {
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
void TTerminal::TerminalError(const UnicodeString Msg)
{
    TerminalError(NULL, Msg);
}
//---------------------------------------------------------------------------
void TTerminal::TerminalError(const Exception *E, const UnicodeString Msg)
{
    throw ETerminal(Msg, E);
}
//---------------------------------------------------------------------------
bool TTerminal::DoQueryReopen(Exception *E)
{
    bool Result = false;
    EFatal *Fatal = dynamic_cast<EFatal *>(E);
    assert(Fatal != NULL);
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
            NumberOfRetries++;
            FSessionData->SetNumberOfRetries(NumberOfRetries);
            LogEvent(L"Connection was lost, asking what to do.");

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
bool TTerminal::QueryReopen(Exception *E, int Params,
                            TFileOperationProgressType *OperationProgress)
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
            catch (Exception &E)
            {
                if (!GetActive())
                {
                    Result =
                        (Configuration->GetSessionReopenTimeout() == 0) && DoQueryReopen(&E);
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
bool TTerminal::FileOperationLoopQuery(const Exception &E,
                                       TFileOperationProgressType *OperationProgress, const UnicodeString Message,
                                       bool AllowSkip, UnicodeString SpecialRetry)
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
int TTerminal::FileOperationLoop(const fileoperation_slot_type &CallBackFunc,
                                 TFileOperationProgressType *OperationProgress, bool AllowSkip,
                                 const UnicodeString Message, void *Param1, void *Param2)
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
UnicodeString TTerminal::TranslateLockedPath(const UnicodeString Path, bool Lock)
{
    UnicodeString path = Path;
    if (!GetSessionData()->GetLockInHome() || path.IsEmpty() || (path[0] != '/'))
    {
        return path;
    }

    if (Lock)
    {
        if (path.SubString(0, FLockDirectory.Length()) == FLockDirectory)
        {
            path.Delete(0, FLockDirectory.Length());
            if (path.IsEmpty()) { path = L"/"; }
        }
    }
    else
    {
        path = UnixExcludeTrailingBackslash(FLockDirectory + path);
    }
    return path;
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
void TTerminal::ClearCachedFileList(const UnicodeString Path,
                                    bool SubDirs)
{
    FDirectoryCache->ClearFileList(Path, SubDirs);
}
//---------------------------------------------------------------------------
void TTerminal::AddCachedFileList(TRemoteFileList *FileList)
{
    FDirectoryCache->AddFileList(FileList);
}
//---------------------------------------------------------------------------
bool TTerminal::DirectoryFileList(const UnicodeString Path,
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
void TTerminal::SetCurrentDirectory(const UnicodeString Value)
{
    assert(FFileSystem);
    UnicodeString value = TranslateLockedPath(Value, false);
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
UnicodeString TTerminal::PeekCurrentDirectory()
{
    if (FFileSystem)
    {
        FCurrentDirectory = FFileSystem->GetCurrentDirectory();
    }

    return TranslateLockedPath(FCurrentDirectory, true);
}
//---------------------------------------------------------------------------

const TRemoteTokenList *TTerminal::GetGroups()
{
    assert(FFileSystem);
    LookupUsersGroups();
    return &FGroups;
}
//---------------------------------------------------------------------------
const TRemoteTokenList *TTerminal::GetUsers()
{
    assert(FFileSystem);
    LookupUsersGroups();
    return &FUsers;
}

//---------------------------------------------------------------------------
const TRemoteTokenList *TTerminal::GetMembership()
{
    assert(FFileSystem);
    LookupUsersGroups();
    return &FMembership;
}
//---------------------------------------------------------------------------
UnicodeString TTerminal::GetUserName()
{
    // in future might also be implemented to detect username similar to GetUserGroups
    assert(FFileSystem != NULL);
    UnicodeString Result = FFileSystem->GetUserName();
    // Is empty also when stored username was used
    if (Result.IsEmpty())
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
    if (!FOnChangeDirectory.IsEmpty())
    {
        TCallbackGuard Guard(this);
        FOnChangeDirectory(this);
        Guard.Verify();
    }
}
//---------------------------------------------------------------------------
void TTerminal::DoReadDirectory(bool ReloadOnly)
{
    if (!FOnReadDirectory.IsEmpty())
    {
        TCallbackGuard Guard(this);
        FOnReadDirectory(this, ReloadOnly);
        Guard.Verify();
    }
}
//---------------------------------------------------------------------------
void TTerminal::DoStartReadDirectory()
{
    if (!FOnStartReadDirectory.IsEmpty())
    {
        TCallbackGuard Guard(this);
        FOnStartReadDirectory(this);
        Guard.Verify();
    }
}
//---------------------------------------------------------------------------
void TTerminal::DoReadDirectoryProgress(size_t Progress, bool &Cancel)
{
    if (FReadingCurrentDirectory && (!FOnReadDirectoryProgress.IsEmpty()))
    {
        TCallbackGuard Guard(this);
        FOnReadDirectoryProgress(this, Progress, Cancel);
        Guard.Verify();
    }
    if (!FOnFindingFile.IsEmpty())
    {
        TCallbackGuard Guard(this);
        FOnFindingFile(this, L"", Cancel);
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
            {
                BOOST_SCOPE_EXIT ( (&Self) )
                {
                    Self->FReadCurrentDirectoryPending = false;
                    Self->FReadDirectoryPending = false;
                } BOOST_SCOPE_EXIT_END
                if (FReadCurrentDirectoryPending) { ReadCurrentDirectory(); }
                if (FReadDirectoryPending) { ReadDirectory(!FReadCurrentDirectoryPending); }
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
    if (value)
    {
        FExceptionOnFail++;
    }
    else
    {
        if (FExceptionOnFail == 0)
        {
            throw Exception("ExceptionOnFail is already zero.");
        }
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
    return static_cast<bool>(FExceptionOnFail > 0);
}
//---------------------------------------------------------------------------
void TTerminal::FatalError(const Exception *E, const UnicodeString Msg)
{
    bool SecureShellActive = (FSecureShell != NULL) && FSecureShell->GetActive();
    if (GetActive() || SecureShellActive)
    {
        // We log this instead of Exception handler, because Close() would
        // probably cause Exception handler to loose pointer to TShellLog()
        LogEvent(L"Attempt to close connection due to fatal Exception:");
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
void TTerminal::CommandError(const Exception *E, const UnicodeString Msg)
{
    CommandError(E, Msg, 0);
}
//---------------------------------------------------------------------------
int TTerminal::CommandError(const Exception *E, const UnicodeString Msg,
                            int Answers)
{
    // may not be, particularly when TTerminal::Reopen is being called
    // from within OnShowExtendedException handler
    assert(FCallbackGuard == NULL);
    int Result = 0;
    if (E && ::InheritsFrom<Exception, EFatal>(E))
    {
        FatalError(E, Msg);
    }
    else if (E && ::InheritsFrom<Exception, EAbort>(E))
    {
        // resent EAbort Exception
        Abort();
    }
    else if (GetExceptionOnFail())
    {
        throw ECommand(Msg, E);
    }
    else if (!Answers)
    {
        ECommand *ECmd = new ECommand(Msg, E);
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
bool TTerminal::HandleException(const Exception *E)
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
void TTerminal::CloseOnCompletion(TOnceDoneOperation Operation, const UnicodeString Message)
{
    LogEvent(L"Closing session after completed operation (as requested by user)");
    Close();
    throw ESshTerminate(
        Message.IsEmpty() ? LoadStr(CLOSED_ON_COMPLETION) : Message,
        NULL,
        Operation);
}
//---------------------------------------------------------------------------
TBatchOverwrite TTerminal::EffectiveBatchOverwrite(
    int Params, TFileOperationProgressType *OperationProgress, bool Special)
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
bool TTerminal::CheckRemoteFile(int Params, TFileOperationProgressType *OperationProgress)
{
    return (EffectiveBatchOverwrite(Params, OperationProgress, true) != boAll);
}
//---------------------------------------------------------------------------
int TTerminal::ConfirmFileOverwrite(const UnicodeString FileName,
                                    const TOverwriteFileParams *FileParams, int Answers, const TQueryParams *QueryParams,
                                    TOperationSide Side, int Params, TFileOperationProgressType *OperationProgress,
                                    UnicodeString Message)
{
    int Result = 0;
    // duplicated in TSFTPFileSFTPConfirmOverwrite
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
void TTerminal::FileModified(const TRemoteFile *File,
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
//---------------------------------------------------------------------------
void TTerminal::DirectoryModified(const UnicodeString Path, bool SubDirs)
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
void TTerminal::DirectoryLoaded(TRemoteFileList *FileList)
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
void TTerminal::EnsureNonExistence(const UnicodeString FileName)
{
    // if filename doesn't contain path, we check for existence of file
    if ((UnixExtractFileDir(FileName).IsEmpty()) &&
            UnixComparePaths(GetCurrentDirectory(), FFiles->GetDirectory()))
    {
        TRemoteFile *File = FFiles->FindFile(FileName);
        if (File)
        {
            if (File->GetIsDirectory()) { throw ECommand(FMTLOAD(RENAME_CREATE_DIR_EXISTS, FileName.c_str()), NULL); }
            else { throw ECommand(FMTLOAD(RENAME_CREATE_FILE_EXISTS, FileName.c_str()), NULL); }
        }
    }
}
//---------------------------------------------------------------------------
void inline TTerminal::LogEvent(const UnicodeString Str)
{
    if (GetLog()->GetLogging())
    {
        GetLog()->Add(llMessage, Str);
    }
}
//---------------------------------------------------------------------------
void TTerminal::RollbackAction(TSessionAction &Action,
                               TFileOperationProgressType *OperationProgress, const Exception *E)
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
    // DEBUG_PRINTF(L"begin");
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
        if (!GetSessionData()->GetRemoteDirectory().IsEmpty())
        {
            ChangeDirectory(GetSessionData()->GetRemoteDirectory());
        }

    }
    LogEvent(L"Startup conversation with host finished.");
    // DEBUG_PRINTF(L"end");
}
//---------------------------------------------------------------------------
void TTerminal::ReadCurrentDirectory()
{
    assert(FFileSystem);
    // DEBUG_PRINTF(L"begin, FFileSystem->GetCurrentDirectory = %s", FFileSystem->GetCurrentDirectory().c_str());
    try
    {
        // reset flag is case we are called externally (like from console dialog)
        FReadCurrentDirectoryPending = false;

        LogEvent(L"Getting current directory name.");
        UnicodeString OldDirectory = FFileSystem->GetCurrentDirectory();
        // DEBUG_PRINTF(L"OldDirectory = %s", OldDirectory.c_str());

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
    catch (const Exception &E)
    {
        CommandError(&E, LoadStr(READ_CURRENT_DIR_ERROR));
    }
    // DEBUG_PRINTF(L"end, FFileSystem->GetCurrentDirectory = %s", FFileSystem->GetCurrentDirectory().c_str());
}
//---------------------------------------------------------------------------
void TTerminal::ReadDirectory(bool ReloadOnly, bool ForceCache)
{
    // DEBUG_PRINTF(L"begin");
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
                BOOST_SCOPE_EXIT ( (&Self) (&ReloadOnly) )
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
        }
        catch (const Exception &E)
        {
            CommandError(&E, ::FmtLoadStr(LIST_DIR_ERROR, FFiles->GetDirectory().c_str()));
        }
    }
    // DEBUG_PRINTF(L"end");
}
//---------------------------------------------------------------------------
void TTerminal::CustomReadDirectory(TRemoteFileList *FileList)
{
    // DEBUG_PRINTF(L"begin");
    assert(FileList);
    assert(FFileSystem);
    FFileSystem->ReadDirectory(FileList);
    ReactOnCommand(fsListDirectory);
    // DEBUG_PRINTF(L"end");
}
//---------------------------------------------------------------------------
TRemoteFileList *TTerminal::ReadDirectoryListing(const UnicodeString Directory, const TFileMasks &Mask)
{
    TLsSessionAction Action(GetActionLog(), AbsolutePath(Directory, true));
    TRemoteFileList *FileList = NULL;
    try
    {
        FileList = DoReadDirectoryListing(Directory, false);
        if (FileList != NULL)
        {
            size_t Index = 0;
            while (Index < FileList->GetCount())
            {
                TRemoteFile *File = FileList->GetFile(Index);
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
    catch (const Exception &E)
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
TRemoteFileList *TTerminal::CustomReadDirectoryListing(const UnicodeString Directory, bool UseCache)
{
    TRemoteFileList *FileList = NULL;
    try
    {
        FileList = DoReadDirectoryListing(Directory, UseCache);
    }
    catch (const Exception &E)
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
TRemoteFileList *TTerminal::DoReadDirectoryListing(const UnicodeString Directory, bool UseCache)
{
    TRemoteFileList *FileList = new TRemoteFileList();
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
void TTerminal::ProcessDirectory(const UnicodeString DirName,
                                 const processfile_slot_type &CallBackFunc, void *Param, bool UseCache, bool IgnoreErrors)
{
    TRemoteFileList *FileList = NULL;
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
            BOOST_SCOPE_EXIT ( (&FileList) )
            {
                delete FileList;
            } BOOST_SCOPE_EXIT_END
            UnicodeString Directory = UnixIncludeTrailingBackslash(DirName);

            TRemoteFile *File;
            processfile_signal_type sig;
            sig.connect(CallBackFunc);
            for (size_t Index = 0; Index < FileList->GetCount(); Index++)
            {
                File = FileList->GetFile(Index);
                if (!File->GetIsParentDirectory() && !File->GetIsThisDirectory())
                {
                    sig(Directory + File->GetFileName(), File, Param);
                }
            }
        }
    }
}
//---------------------------------------------------------------------------
void TTerminal::ReadDirectory(TRemoteFileList *FileList)
{
    try
    {
        CustomReadDirectory(FileList);
    }
    catch (const Exception &E)
    {
        CommandError(&E, ::FmtLoadStr(LIST_DIR_ERROR, FileList->GetDirectory().c_str()));
    }
}
//---------------------------------------------------------------------------
void TTerminal::ReadSymlink(TRemoteFile *SymlinkFile,
                            TRemoteFile *& File)
{
    assert(FFileSystem);
    try
    {
        LogEvent(FORMAT(L"Reading symlink \"%s\".", SymlinkFile->GetFileName().c_str()));
        // DEBUG_PRINTF(L"SymlinkFile->GetLinkTo = %s", SymlinkFile->GetLinkTo().c_str());
        // DEBUG_PRINTF(L"SymlinkFile->GetFileName = %s", SymlinkFile->GetFileName().c_str());
        FFileSystem->ReadSymlink(SymlinkFile, File);
        ReactOnCommand(fsReadSymlink);
    }
    catch (const Exception &E)
    {
        CommandError(&E, FMTLOAD(READ_SYMLINK_ERROR, SymlinkFile->GetFileName().c_str()));
    }
}
//---------------------------------------------------------------------------
void TTerminal::ReadFile(const UnicodeString FileName,
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
    catch (const Exception &E)
    {
        if (File) { delete File; }
        File = NULL;
        CommandError(&E, FMTLOAD(CANT_GET_ATTRS, FileName.c_str()));
    }
}
//---------------------------------------------------------------------------
bool TTerminal::FileExists(const UnicodeString FileName, TRemoteFile **AFile)
{
    bool Result;
    TRemoteFile *File = NULL;
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
bool TTerminal::ProcessFiles(TStrings *FileList,
                             TFileOperation Operation, const processfile_slot_type &ProcessFile, void *Param,
                             TOperationSide Side)
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
                BOOST_SCOPE_EXIT ( (&Self) (&Side) )
                {
                    if (Side == osRemote)
                    {
                        Self->EndTransaction();
                    }
                } BOOST_SCOPE_EXIT_END
                size_t Index = 0;
                UnicodeString FileName;
                processfile_signal_type sig;
                sig.connect(ProcessFile);
                while ((Index < FileList->GetCount()) && (Progress->Cancel == csContinue))
                {
                    FileName = FileList->GetString(Index);
                    try
                    {
                        bool Success = false;
                        {
                            BOOST_SCOPE_EXIT ( (&Progress) (&FileName) (&Success) (&OnceDoneOperation) )
                            {
                                Progress->Finish(FileName, Success, OnceDoneOperation);
                            } BOOST_SCOPE_EXIT_END
                            const TRemoteFile *RemoteFile = static_cast<const TRemoteFile *>(FileList->GetObject(Index));
                            sig(FileName, RemoteFile, Param);
                            Success = true;
                        }
                    }
                    catch (const EScpSkipFile &E)
                    {
                        DEBUG_PRINTF(L"before HandleException");
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
TStrings *TTerminal::GetFixedPaths()
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
bool TTerminal::IsRecycledFile(const UnicodeString FileName)
{
    UnicodeString Path = UnixExtractFilePath(FileName);
    if (Path.IsEmpty())
    {
        Path = GetCurrentDirectory();
    }
    return UnixComparePaths(Path, GetSessionData()->GetRecycleBinPath());
}
//---------------------------------------------------------------------------
void TTerminal::RecycleFile(const UnicodeString FileName,
                            const TRemoteFile *File)
{
    UnicodeString fileName = FileName;
    if (fileName.IsEmpty())
    {
        assert(File != NULL);
        fileName = File->GetFileName();
    }

    if (!IsRecycledFile(fileName))
    {
        LogEvent(FORMAT(L"Moving file \"%s\" to remote recycle bin '%s'.",
                        fileName.c_str(), GetSessionData()->GetRecycleBinPath().c_str()));

        TMoveFileParams Params;
        Params.Target = GetSessionData()->GetRecycleBinPath();
        Params.FileMask = FORMAT(L"*-%s.*", FormatDateTime(L"yyyymmdd-hhnnss", Now()).c_str());

        MoveFile(fileName, File, &Params);
    }
}
//---------------------------------------------------------------------------
void TTerminal::DeleteFile(const UnicodeString FileName,
                           const TRemoteFile *File, void *AParams)
{
    UnicodeString fileName = FileName;
    if (fileName.IsEmpty() && File)
    {
        fileName = File->GetFileName();
    }
    if (GetOperationProgress() && GetOperationProgress()->Operation == foDelete)
    {
        if (GetOperationProgress()->Cancel != csContinue) { Abort(); }
        GetOperationProgress()->SetFile(fileName);
    }
    int Params = (AParams != NULL) ? *(static_cast<int *>(AParams)) : 0;
    bool Recycle =
        FLAGCLEAR(Params, dfForceDelete) &&
        (GetSessionData()->GetDeleteToRecycleBin() != FLAGSET(Params, dfAlternative));
    if (Recycle && !IsRecycledFile(fileName))
    {
        RecycleFile(fileName, File);
    }
    else
    {
        LogEvent(FORMAT(L"Deleting file \"%s\".", fileName.c_str()));
        if (File) { FileModified(File, fileName, true); }
        DoDeleteFile(fileName, File, Params);
        ReactOnCommand(fsDeleteFile);
    }
}
//---------------------------------------------------------------------------
void TTerminal::DoDeleteFile(const UnicodeString FileName,
                             const TRemoteFile *File, int Params)
{
    TRmSessionAction Action(GetActionLog(), AbsolutePath(FileName, true));
    try
    {
        assert(FFileSystem);
        // 'File' parameter: SFTPFileSystem needs to know if file is file or directory
        FFileSystem->DeleteFile(FileName, File, Params, Action);
    }
    catch (const Exception &E)
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
bool TTerminal::DeleteFiles(TStrings *FilesToDelete, int Params)
{
    // TODO: avoid resolving symlinks while reading subdirectories.
    // Resolving does not work anyway for relative symlinks in subdirectories
    // (at least for SFTP).
    return ProcessFiles(FilesToDelete, foDelete, boost::bind(&TTerminal::DeleteFile, this, _1, _2, _3), &Params);
}
//---------------------------------------------------------------------------
void TTerminal::DeleteLocalFile(const UnicodeString FileName,
                                const TRemoteFile * /*File*/, void *Params)
{
    if (!GetOnDeleteLocalFile().IsEmpty())
    {
        if (!RecursiveDeleteFile(FileName, false))
        {
            throw ExtException(FMTLOAD(DELETE_FILE_ERROR, FileName.c_str()));
        }
    }
    else
    {
        GetOnDeleteLocalFile()(FileName, FLAGSET(*(static_cast<int *>(Params)), dfAlternative));
    }
}
//---------------------------------------------------------------------------
bool TTerminal::DeleteLocalFiles(TStrings *FileList, int Params)
{
    return ProcessFiles(FileList, foDelete, boost::bind(&TTerminal::DeleteLocalFile, this, _1, _2, _3), &Params, osLocal);
}
//---------------------------------------------------------------------------
void TTerminal::CustomCommandOnFile(const UnicodeString FileName,
                                    const TRemoteFile *File, void *AParams)
{
    TCustomCommandParams *Params = (static_cast<TCustomCommandParams *>(AParams));
    UnicodeString fileName = FileName;
    if (fileName.IsEmpty() && File)
    {
        fileName = File->GetFileName();
    }
    if (GetOperationProgress() && GetOperationProgress()->Operation == foCustomCommand)
    {
        if (GetOperationProgress()->Cancel != csContinue) { Abort(); }
        GetOperationProgress()->SetFile(fileName);
    }
    LogEvent(FORMAT(L"Executing custom command \"%s\" (%d) on file \"%s\".",
                    Params->Command.c_str(), Params->Params, fileName.c_str()));
    if (File) { FileModified(File, fileName); }
    DoCustomCommandOnFile(fileName, File, Params->Command, Params->Params,
                          Params->OutputEvent);
    ReactOnCommand(fsAnyCommand);
}
//---------------------------------------------------------------------------
void TTerminal::DoCustomCommandOnFile(const UnicodeString FileName,
                                      const TRemoteFile *File, const UnicodeString Command, int Params,
                                      const TCaptureOutputEvent &OutputEvent)
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
    catch (const Exception &E)
    {
        COMMAND_ERROR_ARI
        (
            FMTLOAD(CUSTOM_COMMAND_ERROR, Command.c_str(), FileName.c_str()),
            DoCustomCommandOnFile(FileName, File, Command, Params, OutputEvent)
        );
    }
}
//---------------------------------------------------------------------------
void TTerminal::CustomCommandOnFiles(const UnicodeString Command,
                                     int Params, TStrings *Files, const TCaptureOutputEvent &OutputEvent)
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
        for (size_t i = 0; i < Files->GetCount(); i++)
        {
            TRemoteFile *File = static_cast<TRemoteFile *>(Files->GetObject(i));
            bool Dir = File->GetIsDirectory() && !File->GetIsSymLink();

            if (!Dir || FLAGSET(Params, ccApplyToDirectories))
            {
                if (!FileList.IsEmpty())
                {
                    FileList += L" ";
                }

                FileList += L"\"" + ShellDelimitStr(Files->GetString(i), '"') + L"\"";
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
void TTerminal::ChangeFileProperties(const UnicodeString FileName,
                                     const TRemoteFile *File, /*const TRemoteProperties*/ void *Properties)
{
    const TRemoteProperties *RProperties = static_cast<const TRemoteProperties *>(Properties);
    assert(RProperties && !RProperties->Valid.IsEmpty());
    UnicodeString fileName = FileName;

    if (fileName.IsEmpty() && File)
    {
        fileName = File->GetFileName();
    }
    if (GetOperationProgress() && GetOperationProgress()->Operation == foSetProperties)
    {
        if (GetOperationProgress()->Cancel != csContinue) { Abort(); }
        GetOperationProgress()->SetFile(fileName);
    }
    if (GetLog()->GetLogging())
    {
        LogEvent(FORMAT(L"Changing properties of \"%s\" (%s)",
                        fileName.c_str(), BooleanToEngStr(RProperties->Recursive).c_str()));
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
    FileModified(File, fileName);
    DoChangeFileProperties(fileName, File, RProperties);
    ReactOnCommand(fsChangeProperties);
}
//---------------------------------------------------------------------------
void TTerminal::DoChangeFileProperties(const UnicodeString FileName,
                                       const TRemoteFile *File, const TRemoteProperties *Properties)
{
    TChmodSessionAction Action(GetActionLog(), AbsolutePath(FileName, true));
    try
    {
        assert(FFileSystem);
        FFileSystem->ChangeFileProperties(FileName, File, Properties, Action);
    }
    catch (const Exception &E)
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
void TTerminal::ChangeFilesProperties(TStrings *FileList,
                                      const TRemoteProperties *Properties)
{
    AnnounceFileListOperation();
    ProcessFiles(FileList, foSetProperties, boost::bind(&TTerminal::ChangeFileProperties, this, _1, _2, _3),
                 const_cast<void *>(static_cast<const void *>(Properties)));
}
//---------------------------------------------------------------------------
bool TTerminal::LoadFilesProperties(TStrings *FileList)
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
void TTerminal::CalculateFileSize(const UnicodeString FileName,
                                  const TRemoteFile *File, /*TCalculateSizeParams*/ void *Param)
{
    assert(Param);
    assert(File);
    TCalculateSizeParams *AParams = static_cast<TCalculateSizeParams *>(Param);
    UnicodeString fileName = FileName;

    if (fileName.IsEmpty())
    {
        fileName = File->GetFileName();
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
                LogEvent(FORMAT(L"Getting size of directory \"%s\"", fileName.c_str()));
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
        GetOperationProgress()->SetFile(fileName);
    }
}
//---------------------------------------------------------------------------
void TTerminal::DoCalculateDirectorySize(const UnicodeString FileName,
        const TRemoteFile *File, TCalculateSizeParams *Params)
{
    try
    {
        ProcessDirectory(FileName, boost::bind(&TTerminal::CalculateFileSize, this, _1, _2, _3), Params);
    }
    catch (const Exception &E)
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
void TTerminal::CalculateFilesSize(TStrings *FileList,
                                   __int64 &Size, int Params, const TCopyParamType *CopyParam,
                                   TCalculateSizeStats *Stats)
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
void TTerminal::CalculateFilesChecksum(const UnicodeString Alg,
                                       TStrings *FileList, TStrings *Checksums,
                                       TCalculatedChecksumEvent *OnCalculatedChecksum)
{
    FFileSystem->CalculateFilesChecksum(Alg, FileList, Checksums, OnCalculatedChecksum);
}
//---------------------------------------------------------------------------
void TTerminal::RenameFile(const UnicodeString FileName,
                           const UnicodeString NewName)
{
    LogEvent(FORMAT(L"Renaming file \"%s\" to \"%s\".", FileName.c_str(), NewName.c_str()));
    DoRenameFile(FileName, NewName, false);
    ReactOnCommand(fsRenameFile);
}
//---------------------------------------------------------------------------
void TTerminal::RenameFile(const TRemoteFile *File,
                           const UnicodeString NewName, bool CheckExistence)
{
    assert(File && File->GetDirectory() == FFiles);
    bool Proceed = true;
    // if filename doesn't contain path, we check for existence of file
    if ((File->GetFileName() != NewName) && CheckExistence &&
            Configuration->GetConfirmOverwriting() &&
            UnixComparePaths(GetCurrentDirectory(), FFiles->GetDirectory()))
    {
        TRemoteFile *DuplicateFile = FFiles->FindFile(NewName);
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
void TTerminal::DoRenameFile(const UnicodeString FileName,
                             const UnicodeString NewName, bool Move)
{
    TMvSessionAction Action(GetActionLog(), AbsolutePath(FileName, true), AbsolutePath(NewName, true));
    try
    {
        assert(FFileSystem);
        FFileSystem->RenameFile(FileName, NewName);
    }
    catch (const Exception &E)
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
void TTerminal::MoveFile(const UnicodeString FileName,
                         const TRemoteFile *File, /*const TMoveFileParams*/ void *Param)
{
    if (GetOperationProgress() &&
            ((GetOperationProgress()->Operation == foRemoteMove) ||
             (GetOperationProgress()->Operation == foDelete)))
    {
        if (GetOperationProgress()->Cancel != csContinue) { Abort(); }
        GetOperationProgress()->SetFile(FileName);
    }

    assert(Param != NULL);
    const TMoveFileParams &Params = *static_cast<const TMoveFileParams *>(Param);
    UnicodeString NewName = UnixIncludeTrailingBackslash(Params.Target) +
                           MaskFileName(UnixExtractFileName(FileName), Params.FileMask);
    LogEvent(FORMAT(L"Moving file \"%s\" to \"%s\".", FileName.c_str(), NewName.c_str()));
    FileModified(File, FileName);
    DoRenameFile(FileName, NewName, true);
    ReactOnCommand(fsMoveFile);
}
//---------------------------------------------------------------------------
bool TTerminal::MoveFiles(TStrings *FileList, const UnicodeString Target,
                          const UnicodeString FileMask)
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
                UnicodeString WithTrailing = UnixIncludeTrailingBackslash(Self->GetCurrentDirectory());
                bool PossiblyMoved = false;
                // check if we was moving current directory.
                // this is just optimization to avoid checking existence of current
                // directory after each move operation.
                UnicodeString curDirectory = Self->GetCurrentDirectory();
                for (size_t Index = 0; !PossiblyMoved && (Index < FileList->GetCount()); Index++)
                {
                    const TRemoteFile *File =
                        reinterpret_cast<const TRemoteFile *>(FileList->GetObject(Index));
                    // File can be NULL, and filename may not be full path,
                    // but currently this is the only way we can move (at least in GUI)
                    // current directory
                    if ((File != NULL) &&
                            File->GetIsDirectory() &&
                            ((curDirectory.SubString(0, FileList->GetString(Index).Length()) == FileList->GetString(Index)) &&
                             ((FileList->GetString(Index).Length() == curDirectory.Length()) ||
                              (curDirectory[FileList->GetString(Index).Length() + 1] == '/'))))
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
    return Result;
}
//---------------------------------------------------------------------------
void TTerminal::DoCopyFile(const UnicodeString FileName,
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
    catch (const Exception &E)
    {
        COMMAND_ERROR_ARI
        (
            FMTLOAD(COPY_FILE_ERROR, FileName.c_str(), NewName.c_str()),
            DoCopyFile(FileName, NewName)
        );
    }
}
//---------------------------------------------------------------------------
void TTerminal::CopyFile(const UnicodeString FileName,
                         const TRemoteFile * /*File*/, /*const TMoveFileParams*/ void *Param)
{
    if (GetOperationProgress() && (GetOperationProgress()->Operation == foRemoteCopy))
    {
        if (GetOperationProgress()->Cancel != csContinue) { Abort(); }
        GetOperationProgress()->SetFile(FileName);
    }

    assert(Param != NULL);
    const TMoveFileParams &Params = *static_cast<const TMoveFileParams *>(Param);
    UnicodeString NewName = UnixIncludeTrailingBackslash(Params.Target) +
                           MaskFileName(UnixExtractFileName(FileName), Params.FileMask);
    LogEvent(FORMAT(L"Copying file \"%s\" to \"%s\".", FileName.c_str(), NewName.c_str()));
    DoCopyFile(FileName, NewName);
    ReactOnCommand(fsCopyFile);
}
//---------------------------------------------------------------------------
bool TTerminal::CopyFiles(TStrings *FileList, const UnicodeString Target,
                          const UnicodeString FileMask)
{
    TMoveFileParams Params;
    Params.Target = Target;
    Params.FileMask = FileMask;
    DirectoryModified(Target, true);
    return ProcessFiles(FileList, foRemoteCopy, boost::bind(&TTerminal::CopyFile, this, _1, _2, _3), &Params);
}
//---------------------------------------------------------------------------
void TTerminal::CreateDirectory(const UnicodeString DirName,
                                const TRemoteProperties *Properties)
{
    assert(FFileSystem);
    EnsureNonExistence(DirName);
    FileModified(NULL, DirName);

    LogEvent(FORMAT(L"Creating directory \"%s\".", DirName.c_str()));
    DoCreateDirectory(DirName);

    if ((Properties != NULL) && !Properties->Valid.IsEmpty())
    {
        DoChangeFileProperties(DirName, NULL, Properties);
    }

    ReactOnCommand(fsCreateDirectory);
}
//---------------------------------------------------------------------------
void TTerminal::DoCreateDirectory(const UnicodeString DirName)
{
    TMkdirSessionAction Action(GetActionLog(), AbsolutePath(DirName, true));
    try
    {
        assert(FFileSystem);
        FFileSystem->CreateDirectory(DirName);
    }
    catch (const Exception &E)
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
void TTerminal::CreateLink(const UnicodeString FileName,
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
void TTerminal::DoCreateLink(const UnicodeString FileName,
                             const UnicodeString PointTo, bool Symbolic)
{
    try
    {
        assert(FFileSystem);
        FFileSystem->CreateLink(FileName, PointTo, Symbolic);
    }
    catch (const Exception &E)
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
    catch (const Exception &E)
    {
        CommandError(&E, LoadStr(CHANGE_HOMEDIR_ERROR));
    }
}
//---------------------------------------------------------------------------
void TTerminal::ChangeDirectory(const UnicodeString Directory)
{
    // DEBUG_PRINTF(L"begin, Directory = %s", Directory.c_str());
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
            // DEBUG_PRINTF(L"PeekCurrentDirectory = %s", PeekCurrentDirectory().c_str());
            // DEBUG_PRINTF(L"Directory = %s, CachedDirectory = %s", Directory.c_str(), CachedDirectory.c_str());
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
    catch (const Exception &E)
    {
        CommandError(&E, FMTLOAD(CHANGE_DIR_ERROR, DirectoryNormalized.c_str()));
    }
    // DEBUG_PRINTF(L"end, FLastDirectoryChange = %s", FLastDirectoryChange.c_str());
}
//---------------------------------------------------------------------------
void TTerminal::LookupUsersGroups()
{
    // DEBUG_PRINTF(L"FUsersGroupsLookedup = %d, GetSessionData()->GetLookupUserGroups = %d, GetIsCapable(fcUserGroupListing) = %d", FUsersGroupsLookedup, GetSessionData()->GetLookupUserGroups(), GetIsCapable(fcUserGroupListing));
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
                FGroups.Log(this, L"membership");
                FGroups.Log(this, L"users");
            }
        }
        catch (const Exception &E)
        {
            CommandError(&E, LoadStr(LOOKUP_GROUPS_ERROR));
        }
    }
}
//---------------------------------------------------------------------------
bool TTerminal::AllowedAnyCommand(const UnicodeString Command)
{
    return !::Trim(Command).IsEmpty();
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
TTerminal *TTerminal::GetCommandSession()
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
            (static_cast<TSecondaryTerminal *>(FCommandSession))->Init(GetSessionData(), Configuration, L"Shell");

            FCommandSession->SetAutoReadDirectory(false);

            TSessionData *CommandSessionData = FCommandSession->FSessionData;
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
        catch (...)
        {
            SAFE_DESTROY(FCommandSession);
            throw;
        }
    }

    return FCommandSession;
}
//---------------------------------------------------------------------------
void TTerminal::AnyCommand(const UnicodeString Command,
                           const TCaptureOutputEvent *OutputEvent)
{

    class TOutputProxy
    {
    public:
        TOutputProxy(TCallSessionAction &Action, const TCaptureOutputEvent *OutputEvent) :
            FAction(Action)
        {
            if (OutputEvent)
            {
                FOutputEvent.connect(*OutputEvent);
            }
        }

        void Output(const UnicodeString Str, bool StdError)
        {
            FAction.AddOutput(Str, StdError);
            if (!FOutputEvent.IsEmpty())
            {
                FOutputEvent(Str, StdError);
            }
        }

    private:
        TCallSessionAction &FAction;
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
    DoAnyCommand(Command, boost::bind(&TOutputProxy::Output, &ProxyOutputEvent, _1, _2),
                 &Action);
}
//---------------------------------------------------------------------------
void TTerminal::DoAnyCommand(const UnicodeString Command,
                             const TCaptureOutputEvent &OutputEvent, TCallSessionAction *Action)
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
    catch (const Exception &E)
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
bool TTerminal::DoCreateLocalFile(const UnicodeString FileName,
                                  TFileOperationProgressType *OperationProgress, HANDLE *AHandle,
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
                        switch (Answer)
                        {
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
bool TTerminal::CreateLocalFile(const UnicodeString FileName,
                                TFileOperationProgressType *OperationProgress, HANDLE *AHandle,
                                bool NoConfirmation)
{
    assert(AHandle);
    bool Result = true;
    // DEBUG_PRINTF(L"FileName = %s", FileName.c_str());
    FILE_OPERATION_LOOP (FMTLOAD(CREATE_FILE_ERROR, FileName.c_str()),
        Result = DoCreateLocalFile(FileName, OperationProgress, AHandle, NoConfirmation);
    );

    return Result;
}
//---------------------------------------------------------------------------
void TTerminal::OpenLocalFile(const UnicodeString FileName,
                              int Access, int *AAttrs, HANDLE *AHandle, __int64 *ACTime,
                              __int64 *AMTime, __int64 *AATime, __int64 *ASize,
                              bool TryWriteReadOnly)
{
    // DEBUG_PRINTF(L"begin: FileName = %s, Access = %d", FileName.c_str(), Access);
    int Attrs = 0;
    HANDLE Handle = 0;
    TFileOperationProgressType *OperationProgress = GetOperationProgress();
    FILE_OPERATION_LOOP (FMTLOAD(FILE_NOT_EXISTS, FileName.c_str()),
        Attrs = FileGetAttr(FileName);
        // if ((Attrs == -1) && (Access != GENERIC_WRITE)) RaiseLastOSError();
        if (Attrs == -1)
        {
            RaiseLastOSError();
        }
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
                    if ((LSize == 0xFFFFFFFF) && (GetLastError() != NO_ERROR))
                    {
                        RaiseLastOSError();
                    }
                    *ASize = (__int64(HSize) << 32) + LSize;
                );
            }

            if ((AHandle == NULL) || NoHandle)
            {
                ::CloseHandle(Handle);
                Handle = NULL;
            }
        }
        catch (...)
        {
            ::CloseHandle(Handle);
            throw;
        }
    }

    if (AAttrs) { *AAttrs = Attrs; }
    if (AHandle) { *AHandle = Handle; }
    // DEBUG_PRINTF(L"end: Attrs = %d, Handle = %d", Attrs, Handle);
}
//---------------------------------------------------------------------------
bool TTerminal::AllowLocalFileTransfer(const UnicodeString FileName,
                                       const TCopyParamType *CopyParam)
{
    bool Result = true;
    if (!CopyParam->AllowAnyTransfer())
    {
        WIN32_FIND_DATA FindData;
        memset(&FindData, 0, sizeof(FindData));
        HANDLE Handle = 0;
        TFileOperationProgressType *OperationProgress = GetOperationProgress();
        FILE_OPERATION_LOOP (FMTLOAD(FILE_NOT_EXISTS, FileName.c_str()),
            Handle = ::FindFirstFile(FileName.c_str(), &FindData);
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
UnicodeString TTerminal::FileUrl(const UnicodeString Protocol,
                                const UnicodeString FileName)
{
    assert(FileName.Length() > 0);
    return Protocol + L"://" + EncodeUrlChars(GetSessionData()->GetSessionName()) +
           (FileName[0] == '/' ? L"" : L"/") + EncodeUrlChars(FileName, L"/");
}
//---------------------------------------------------------------------------
UnicodeString TTerminal::FileUrl(const UnicodeString FileName)
{
    return FFileSystem->FileUrl(FileName);
}
//---------------------------------------------------------------------------
void TTerminal::MakeLocalFileList(const UnicodeString FileName,
                                  const WIN32_FIND_DATA &Rec, void *Param)
{
    TMakeLocalFileListParams &Params = *static_cast<TMakeLocalFileListParams *>(Param);

    bool Directory = FLAGSET(Rec.dwFileAttributes, faDirectory);
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
void TTerminal::CalculateLocalFileSize(const UnicodeString FileName,
                                       const WIN32_FIND_DATA &Rec, /*TCalculateSizeParams*/ void *Params)
{
    TCalculateSizeParams *AParams = static_cast<TCalculateSizeParams *>(Params);

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
void TTerminal::CalculateLocalFilesSize(TStrings *FileList,
                                        __int64 &Size, const TCopyParamType *CopyParam)
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
        for (size_t Index = 0; Index < FileList->GetCount(); Index++)
        {
            UnicodeString FileName = FileList->GetString(Index);
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
    TRemoteFile *MatchingRemoteFileFile;
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
    const synchronizedirectory_slot_type *OnSynchronizeDirectory;
    TSynchronizeOptions *Options;
    int Flags;
    TStringList *LocalFileList;
    const TCopyParamType *CopyParam;
    TSynchronizeChecklist *Checklist;
};
//---------------------------------------------------------------------------
TSynchronizeChecklist *TTerminal::SynchronizeCollect(const UnicodeString LocalDirectory,
        const UnicodeString RemoteDirectory, TSynchronizeMode Mode,
        const TCopyParamType *CopyParam, int Params,
        const synchronizedirectory_slot_type &OnSynchronizeDirectory,
        TSynchronizeOptions *Options)
{
    TSynchronizeChecklist *Checklist = new TSynchronizeChecklist();
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
void TTerminal::DoSynchronizeCollectDirectory(const UnicodeString LocalDirectory,
        const UnicodeString RemoteDirectory, TSynchronizeMode Mode,
        const TCopyParamType *CopyParam, int Params,
        const synchronizedirectory_slot_type &OnSynchronizeDirectory, TSynchronizeOptions *Options,
        int Flags, TSynchronizeChecklist *Checklist)
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

    LogEvent(FORMAT(L"Collecting synchronization list for local directory '%s' and remote directory '%s', "
                    L"mode = %d, params = %d", LocalDirectory.c_str(), RemoteDirectory.c_str(),
                    int(Mode), int(Params)));

    if (FLAGCLEAR(Params, spDelayProgress))
    {
        DoSynchronizeProgress(Data, true);
    }

    {
        BOOST_SCOPE_EXIT ( (&Data) )
        {
            if (Data.LocalFileList != NULL)
            {
                for (size_t Index = 0; Index < Data.LocalFileList->GetCount(); Index++)
                {
                    TSynchronizeFileData *FileData = reinterpret_cast<TSynchronizeFileData *>
                                                     (Data.LocalFileList->GetObject(Index));
                    delete FileData;
                }
                delete Data.LocalFileList;
            }
        } BOOST_SCOPE_EXIT_END
        bool Found = false;
        WIN32_FIND_DATA SearchRec;
        memset(&SearchRec, 0, sizeof(SearchRec));
        Data.LocalFileList = new TStringList();
        Data.LocalFileList->SetSorted(true);
        Data.LocalFileList->SetCaseSensitive(false);
        TFileOperationProgressType *OperationProgress = GetOperationProgress();
        HANDLE findHandle = 0;
        FILE_OPERATION_LOOP (FMTLOAD(LIST_DIR_ERROR, LocalDirectory.c_str()),
            UnicodeString path = Data.LocalDirectory + L"*.*";
            findHandle = FindFirstFile(path.c_str(), &SearchRec);
            Found = (findHandle != 0);
        );

        if (Found)
        {
            {
                BOOST_SCOPE_EXIT ( (&findHandle) )
                {
                    ::FindClose(findHandle);
                } BOOST_SCOPE_EXIT_END
                UnicodeString FileName;
                while (Found)
                {
                    FileName = SearchRec.cFileName;
                    // add dirs for recursive mode or when we are interested in newly
                    // added subdirs
                    size_t FoundIndex;
                    __int64 Size =
                        (static_cast<__int64>(SearchRec.nFileSizeHigh) << 32) +
                        SearchRec.nFileSizeLow;
                    TFileMasks::TParams MaskParams;
                    MaskParams.Size = Size;
                    UnicodeString RemoteFileName =
                        CopyParam->ChangeFileName(FileName, osLocal, false);
                    if ((FileName != THISDIRECTORY) && (FileName != PARENTDIRECTORY) &&
                            CopyParam->AllowTransfer(Data.LocalDirectory + FileName, osLocal,
                                                     FLAGSET(SearchRec.dwFileAttributes, faDirectory), MaskParams) &&
                            !FFileSystem->TemporaryTransferFile(FileName) &&
                            (FLAGCLEAR(Flags, sfFirstLevel) ||
                             (Options == NULL) || (Options->Filter == NULL) ||
                             Options->Filter->Find(FileName, FoundIndex) ||
                             Options->Filter->Find(RemoteFileName, FoundIndex)))
                    {
                        TSynchronizeFileData *FileData = new TSynchronizeFileData;

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
                                                      reinterpret_cast<TObject *>(FileData));
                    }

                    FILE_OPERATION_LOOP (FMTLOAD(LIST_DIR_ERROR, LocalDirectory.c_str()),
                        Found = (::FindNextFile(findHandle, &SearchRec) != 0);
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

            ProcessDirectory(RemoteDirectory, boost::bind(&TTerminal::SynchronizeCollectFile, this, _1, _2, _3), &Data,
                             FLAGSET(Params, spUseCache));

            TSynchronizeFileData *FileData;
            for (size_t Index = 0; Index < Data.LocalFileList->GetCount(); Index++)
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
                    TSynchronizeChecklist::TItem *ChecklistItem = new TSynchronizeChecklist::TItem();
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
void TTerminal::SynchronizeCollectFile(const UnicodeString FileName,
                                       const TRemoteFile *File, /*TSynchronizeData*/ void *Param)
{
    TSynchronizeData *Data = static_cast<TSynchronizeData *>(Param);

    size_t FoundIndex;
    TFileMasks::TParams MaskParams;
    MaskParams.Size = File->GetSize();
    UnicodeString LocalFileName =
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
        TSynchronizeChecklist::TItem *ChecklistItem = new TSynchronizeChecklist::TItem();
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
            size_t LocalIndex = Data->LocalFileList->IndexOf(LocalFileName.c_str());
            bool New = (LocalIndex == NPOS);
            if (!New)
            {
                TSynchronizeFileData *LocalData =
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
    }
}
//---------------------------------------------------------------------------
void TTerminal::SynchronizeApply(TSynchronizeChecklist *Checklist,
                                 const UnicodeString LocalDirectory, const UnicodeString RemoteDirectory,
                                 const TCopyParamType *CopyParam, int Params,
                                 const synchronizedirectory_slot_type &OnSynchronizeDirectory)
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

    TStringList *DownloadList = new TStringList();
    TStringList *DeleteRemoteList = new TStringList();
    TStringList *UploadList = new TStringList();
    TStringList *DeleteLocalList = new TStringList();

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
        size_t IIndex = 0;
        while (IIndex < Checklist->GetCount())
        {
            const TSynchronizeChecklist::TItem *ChecklistItem;

            DownloadList->Clear();
            DeleteRemoteList->Clear();
            UploadList->Clear();
            DeleteLocalList->Clear();

            ChecklistItem = Checklist->GetItem(IIndex);

            UnicodeString CurrentLocalDirectory = ChecklistItem->Local.Directory;
            UnicodeString CurrentRemoteDirectory = ChecklistItem->Remote.Directory;

            LogEvent(FORMAT(L"Synchronizing local directory '%s' with remote directory '%s', "
                            L"params = %d", CurrentLocalDirectory.c_str(), CurrentRemoteDirectory.c_str(), int(Params)));

            size_t Count = 0;

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
}
//---------------------------------------------------------------------------
void TTerminal::DoSynchronizeProgress(const TSynchronizeData &Data,
                                      bool Collect)
{
    if (Data.OnSynchronizeDirectory != NULL)
    {
        bool Continue = true;
        synchronizedirectory_signal_type sig;
        sig.connect(*Data.OnSynchronizeDirectory);
        sig(Data.LocalDirectory, Data.RemoteDirectory,
            Continue, Collect);

        if (!Continue)
        {
            Abort();
        }
    }
}
//---------------------------------------------------------------------------
void TTerminal::SynchronizeLocalTimestamp(const UnicodeString /*FileName*/,
        const TRemoteFile *File, void * /*Param*/)
{
    const TSynchronizeChecklist::TItem *ChecklistItem =
        reinterpret_cast<const TSynchronizeChecklist::TItem *>(File);

    UnicodeString LocalFile =
        IncludeTrailingBackslash(ChecklistItem->Local.Directory) +
        ChecklistItem->Local.FileName;
    TFileOperationProgressType *OperationProgress = GetOperationProgress();
    FILE_OPERATION_LOOP (FMTLOAD(CANT_SET_ATTRS, LocalFile.c_str()),
        HANDLE Handle;
        OpenLocalFile(LocalFile, GENERIC_WRITE, NULL, &Handle,
                   NULL, NULL, NULL, NULL);
        FILETIME WrTime = DateTimeToFileTime(ChecklistItem->Remote.Modification,
                       GetSessionData()->GetDSTMode());
        bool Result = SetFileTime(Handle, NULL, NULL, &WrTime) > 0;
        ::CloseHandle(Handle);
        if (!Result)
        {
            Abort();
        }
    );
}
//---------------------------------------------------------------------------
void TTerminal::SynchronizeRemoteTimestamp(const UnicodeString /*FileName*/,
        const TRemoteFile *File, void * /*Param*/)
{
    const TSynchronizeChecklist::TItem *ChecklistItem =
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
void TTerminal::FileFind(const UnicodeString FileName,
                         const TRemoteFile *File, /*TFilesFindParams*/ void *Param)
{
    // see DoFilesFind

    assert(Param);
    assert(File);
    TFilesFindParams *AParams = static_cast<TFilesFindParams *>(Param);

    if (!AParams->Cancel)
    {
        UnicodeString fileName = FileName;
        if (fileName.IsEmpty())
        {
            fileName = File->GetFileName();
        }

        TFileMasks::TParams MaskParams;
        MaskParams.Size = File->GetSize();

        UnicodeString FullFileName = UnixExcludeTrailingBackslash(File->GetFullFileName());
        if (AParams->FileMask.Matches(FullFileName, false,
                                      File->GetIsDirectory(), &MaskParams))
        {
            filefound_signal_type sig;
            if (AParams->OnFileFound)
            {
                sig.connect(*AParams->OnFileFound);
            }
            sig(this, fileName, File, AParams->Cancel);
        }

        if (File->GetIsDirectory())
        {
            DoFilesFind(FullFileName, *AParams);
        }
    }
}
//---------------------------------------------------------------------------
void TTerminal::DoFilesFind(const UnicodeString Directory, TFilesFindParams &Params)
{
    findingfile_signal_type sig;
    if (Params.OnFindingFile)
    {
        sig.connect(*Params.OnFindingFile);
    }
    sig(this, Directory, Params.Cancel);
    if (!Params.Cancel)
    {
        assert(FOnFindingFile.IsEmpty());
        // ideally we should set the handler only around actually reading
        // of the directory listing, so we at least reset the handler in
        // FileFind
        if (Params.OnFindingFile)
        {
            FOnFindingFile.connect(*Params.OnFindingFile);
        }
        {
            BOOST_SCOPE_EXIT ( (&Self) )
            {
                Self->FOnFindingFile.disconnect_all_slots();
            } BOOST_SCOPE_EXIT_END
            ProcessDirectory(Directory, boost::bind(&TTerminal::FileFind, this, _1, _2, _3), &Params, false, true);
        }
    }
}
//---------------------------------------------------------------------------
void TTerminal::FilesFind(const UnicodeString Directory, const TFileMasks &FileMask,
                          const filefound_slot_type *OnFileFound, const findingfile_slot_type *OnFindingFile)
{
    TFilesFindParams Params;
    Params.FileMask = FileMask;
    Params.OnFileFound = OnFileFound;
    Params.OnFindingFile = OnFindingFile;
    Params.Cancel = false;
    DoFilesFind(Directory, Params);
}
//---------------------------------------------------------------------------
void TTerminal::SpaceAvailable(const UnicodeString Path,
                               TSpaceAvailable &ASpaceAvailable)
{
    assert(GetIsCapable(fcCheckingSpaceAvailable));

    try
    {
        FFileSystem->SpaceAvailable(Path, ASpaceAvailable);
    }
    catch (const Exception &E)
    {
        CommandError(&E, FMTLOAD(SPACE_AVAILABLE_ERROR, Path.c_str()));
    }
}
//---------------------------------------------------------------------------
const TSessionInfo &TTerminal::GetSessionInfo()
{
    return FFileSystem->GetSessionInfo();
}
//---------------------------------------------------------------------------
const TFileSystemInfo &TTerminal::GetFileSystemInfo(bool Retrieve)
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
//---------------------------------------------------------------------------
bool TTerminal::CopyToRemote(TStrings *FilesToCopy,
                             const UnicodeString TargetDir, const TCopyParamType *CopyParam, int Params)
{
    assert(FFileSystem);
    assert(FilesToCopy);

    assert(GetIsCapable(fcNewerOnlyUpload) || FLAGCLEAR(Params, cpNewerOnly));

    bool Result = false;
    TOnceDoneOperation OnceDoneOperation = odoIdle;

    TFileOperationProgressType *OperationProgress = new TFileOperationProgressType(boost::bind(&TTerminal::DoProgress, this, _1, _2),
            boost::bind(&TTerminal::DoFinished, this, _1, _2, _3, _4, _5, _6));
    try
    {

        __int64 Size = 0;
        if (CopyParam->GetCalculateSize())
        {
            // dirty trick: when moving, do not pass copy param to avoid exclude mask
            CalculateLocalFilesSize(FilesToCopy, Size,
                                    (FLAGCLEAR(Params, cpDelete) ? CopyParam : NULL));
        }

        OperationProgress->Start((Params & cpDelete ? foMove : foCopy), osLocal,
                                 FilesToCopy->GetCount(), (Params & cpTemporary) > 0, TargetDir, CopyParam->GetCPSLimit());

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

            UnicodeString UnlockedTargetDir = TranslateLockedPath(TargetDir, false);
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
    catch (const Exception &E)
    {
        if (OperationProgress->Cancel != csCancel)
        {
            CommandError(&E, LoadStr(TOREMOTE_COPY_ERROR));
        }
        OnceDoneOperation = odoIdle;
    }

    if (OnceDoneOperation != odoIdle)
    {
        CloseOnCompletion(OnceDoneOperation);
    }
    delete OperationProgress;
    OperationProgress = NULL;

    return Result;
}
//---------------------------------------------------------------------------
bool TTerminal::CopyToLocal(TStrings *FilesToCopy,
                            const UnicodeString TargetDir, const TCopyParamType *CopyParam, int Params)
{
    assert(FFileSystem);

    // see scp.c: sink(), tolocal()

    bool Result = false;
    bool OwnsFileList = (FilesToCopy == NULL);
    TOnceDoneOperation OnceDoneOperation = odoIdle;

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
    {
        BOOST_SCOPE_EXIT( (&Self) )
        {
            // If session is still active (no fatal error) we reload directory
            // by calling EndTransaction
            Self->EndTransaction();
        } BOOST_SCOPE_EXIT_END
        __int64 TotalSize = 0;
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
        OperationProgress->Start(((Params & cpDelete) != 0 ? foMove : foCopy), osRemote,
                                 FilesToCopy->GetCount(), (Params & cpTemporary) > 0, TargetDir, CopyParam->GetCPSLimit());

        FOperationProgress = OperationProgress;
        {
            BOOST_SCOPE_EXIT ( (&Self) (&OperationProgress) )
            {
                Self->FOperationProgress = NULL;
                OperationProgress->Stop();
                delete OperationProgress;
                OperationProgress = NULL;
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
            catch (const Exception &E)
            {
                if (OperationProgress->Cancel != csCancel)
                {
                    CommandError(&E, LoadStr(TOLOCAL_COPY_ERROR));
                }
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
TSecondaryTerminal::TSecondaryTerminal(TTerminal *MainTerminal) :
    TTerminal(),
    FMainTerminal(MainTerminal), FMasterPasswordTried(false),
    FMasterTunnelPasswordTried(false)
{
}

void TSecondaryTerminal::Init(TSessionData *SessionData, TConfiguration *configuration,
                              const UnicodeString Name)
{
    TTerminal::Init(SessionData, configuration);
    assert(FMainTerminal != NULL);
    GetLog()->SetParent(FMainTerminal->GetLog());
    GetLog()->SetName(Name);
    GetSessionData()->NonPersistant();
    if (!FMainTerminal->GetUserName().IsEmpty())
    {
        GetSessionData()->SetUserName(FMainTerminal->GetUserName());
    }
}
//---------------------------------------------------------------------------
void TSecondaryTerminal::DirectoryLoaded(TRemoteFileList *FileList)
{
    FMainTerminal->DirectoryLoaded(FileList);
    assert(FileList != NULL);
}
//---------------------------------------------------------------------------
void TSecondaryTerminal::DirectoryModified(const UnicodeString Path,
        bool SubDirs)
{
    // clear cache of main terminal
    FMainTerminal->DirectoryModified(Path, SubDirs);
}
//---------------------------------------------------------------------------
bool TSecondaryTerminal::DoPromptUser(TSessionData *Data,
                                      TPromptKind Kind, const UnicodeString Name, const UnicodeString Instructions, TStrings *Prompts,
                                      TStrings *Results)
{
    bool AResult = false;

    if ((Prompts->GetCount() == 1) && !((void *)(Prompts->GetObject(0) != NULL)) &&
            ((Kind == pkPassword) || (Kind == pkPassphrase) || (Kind == pkKeybInteractive) ||
             (Kind == pkTIS) || (Kind == pkCryptoCard)))
    {
        bool &PasswordTried =
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
//---------------------------------------------------------------------------
TTerminalList::TTerminalList(TConfiguration *AConfiguration) :
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
TTerminal *TTerminalList::CreateTerminal(TSessionData *Data)
{
    TTerminal *Result = new TTerminal();
    Result->Init(Data, FConfiguration);
    return Result;
}
//---------------------------------------------------------------------------
TTerminal *TTerminalList::NewTerminal(TSessionData *Data)
{
    TTerminal *Terminal = CreateTerminal(Data);
    Add(Terminal);
    return Terminal;
}
//---------------------------------------------------------------------------
void TTerminalList::FreeTerminal(TTerminal *Terminal)
{
    assert(IndexOf(Terminal) != NPOS);
    Remove(Terminal);
}
//---------------------------------------------------------------------------
void TTerminalList::FreeAndNullTerminal(TTerminal * & Terminal)
{
    TTerminal *T = Terminal;
    Terminal = NULL;
    FreeTerminal(T);
}
//---------------------------------------------------------------------------
TTerminal *TTerminalList::GetTerminal(size_t Index)
{
    return reinterpret_cast<TTerminal *>(GetItem(Index));
}
//---------------------------------------------------------------------------
int TTerminalList::GetActiveCount()
{
    int Result = 0;
    TTerminal *Terminal;
    for (size_t i = 0; i < GetCount(); i++)
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
    TTerminal *Terminal;
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
void TTerminalList::RecryptPasswords()
{
    for (size_t Index = 0; Index < GetCount(); Index++)
    {
        GetTerminal(Index)->RecryptPasswords();
    }
}
