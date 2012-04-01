//---------------------------------------------------------------------------
#ifndef _MSC_VER
#include <vcl.h>
#pragma hdrstop
#endif
#include <stdafx.h>

#include "boostdefines.hpp"
#include <boost/scope_exit.hpp>
#include <boost/bind.hpp>

#ifndef NO_FILEZILLA
//---------------------------------------------------------------------------
#include <list>
#ifndef MPEXT
#define MPEXT
#endif
#include "FtpFileSystem.h"
#include "FileZillaIntf.h"
#include "AsyncProxySocketLayer.h"
#include "FtpControlSocket.h"

#include "Common.h"
#include "Exceptions.h"
#include "Terminal.h"
#include "TextsCore.h"
#include "TextsFileZilla.h"
#include "HelpCore.h"
#ifdef MPEXT
#define OPENSSL_NO_EC
#define OPENSSL_NO_ECDSA
#define OPENSSL_NO_ECDH
#define OPENSSL_NO_ENGINE
#define OPENSSL_NO_DEPRECATED
#endif
#include <openssl/x509_vfy.h>
//---------------------------------------------------------------------------
#ifndef _MSC_VER
#pragma package(smart_init)
#endif
//---------------------------------------------------------------------------
#define FILE_OPERATION_LOOP_EX(ALLOW_SKIP, MESSAGE, OPERATION) \
  FILE_OPERATION_LOOP_CUSTOM(Self->FTerminal, ALLOW_SKIP, MESSAGE, OPERATION)
//---------------------------------------------------------------------------
const int DummyCodeClass = 8;
const int DummyTimeoutCode = 801;
const int DummyCancelCode = 802;
const int DummyDisconnectCode = 803;
//---------------------------------------------------------------------------
class TFileZillaImpl : public TFileZillaIntf
{
public:
    explicit TFileZillaImpl(TFTPFileSystem *FileSystem);
    virtual ~TFileZillaImpl()
    {}

    virtual const wchar_t * __fastcall Option(int OptionID) const;
    virtual int __fastcall OptionVal(int OptionID) const;

protected:
    virtual bool __fastcall DoPostMessage(TMessageType Type, WPARAM wParam, LPARAM lParam);

    virtual bool __fastcall HandleStatus(const wchar_t *Status, int Type);
    virtual bool __fastcall HandleAsynchRequestOverwrite(
        wchar_t *FileName1, size_t FileName1Len, const wchar_t *FileName2,
        const wchar_t *Path1, const wchar_t *Path2,
        __int64 Size1, __int64 Size2, time_t Time1, time_t Time2,
        bool HasTime1, bool HasTime2, void *UserData, int &RequestResult);
    virtual bool __fastcall HandleAsynchRequestVerifyCertificate(
        const TFtpsCertificateData &Data, int &RequestResult);
    virtual bool __fastcall HandleListData(const wchar_t *Path, const TListDataEntry *Entries,
                                size_t Count);
    virtual bool __fastcall HandleTransferStatus(bool Valid, __int64 TransferSize,
                                      __int64 Bytes, int Percent, int TimeElapsed, int TimeLeft, int TransferRate,
                                      bool FileTransfer);
    virtual bool __fastcall HandleReply(int Command, unsigned int Reply);
    virtual bool __fastcall HandleCapabilities(TFTPServerCapabilities *ServerCapabilities);
    virtual bool __fastcall CheckError(int ReturnCode, const wchar_t *Context);

private:
    TFTPFileSystem *FFileSystem;
};
//---------------------------------------------------------------------------
TFileZillaImpl::TFileZillaImpl(TFTPFileSystem *FileSystem) :
    TFileZillaIntf(),
    FFileSystem(FileSystem)
{
}
//---------------------------------------------------------------------------
const wchar_t * __fastcall TFileZillaImpl::Option(int OptionID) const
{
    return FFileSystem->GetOption(OptionID);
}
//---------------------------------------------------------------------------
int __fastcall  TFileZillaImpl::OptionVal(int OptionID) const
{
    return FFileSystem->GetOptionVal(OptionID);
}
//---------------------------------------------------------------------------
bool __fastcall TFileZillaImpl::DoPostMessage(TMessageType Type, WPARAM wParam, LPARAM lParam)
{
    return FFileSystem->PostMessage(Type, wParam, lParam);
}
//---------------------------------------------------------------------------
bool __fastcall TFileZillaImpl::HandleStatus(const wchar_t *Status, int Type)
{
    return FFileSystem->HandleStatus(Status, Type);
}
//---------------------------------------------------------------------------
bool __fastcall TFileZillaImpl::HandleAsynchRequestOverwrite(
    wchar_t *FileName1, size_t FileName1Len, const wchar_t *FileName2,
    const wchar_t *Path1, const wchar_t *Path2,
    __int64 Size1, __int64 Size2, time_t Time1, time_t Time2,
    bool HasTime1, bool HasTime2, void *UserData, int &RequestResult)
{
    return FFileSystem->HandleAsynchRequestOverwrite(
               FileName1, FileName1Len,
               FileName2,
               Path1,
               Path2, Size1, Size2, Time1, Time2,
               HasTime1, HasTime2, UserData, RequestResult);
}
//---------------------------------------------------------------------------
bool __fastcall TFileZillaImpl::HandleAsynchRequestVerifyCertificate(
    const TFtpsCertificateData &Data, int &RequestResult)
{
    return FFileSystem->HandleAsynchRequestVerifyCertificate(Data, RequestResult);
}
//---------------------------------------------------------------------------
bool __fastcall TFileZillaImpl::HandleListData(const wchar_t *Path,
                                    const TListDataEntry *Entries, size_t Count)
{
    return FFileSystem->HandleListData(Path, Entries, Count);
}
//---------------------------------------------------------------------------
bool __fastcall TFileZillaImpl::HandleTransferStatus(bool Valid, __int64 TransferSize,
        __int64 Bytes, int Percent, int TimeElapsed, int TimeLeft, int TransferRate,
        bool FileTransfer)
{
    return FFileSystem->HandleTransferStatus(Valid, TransferSize, Bytes, Percent,
            TimeElapsed, TimeLeft, TransferRate, FileTransfer);
}
//---------------------------------------------------------------------------
bool __fastcall TFileZillaImpl::HandleReply(int Command, unsigned int Reply)
{
    return FFileSystem->HandleReply(Command, Reply);
}
//---------------------------------------------------------------------------
bool __fastcall TFileZillaImpl::HandleCapabilities(TFTPServerCapabilities *ServerCapabilities)
{
    return FFileSystem->HandleCapabilities(ServerCapabilities);
}
//---------------------------------------------------------------------------
bool __fastcall TFileZillaImpl::CheckError(int ReturnCode, const wchar_t *Context)
{
    return FFileSystem->CheckError(ReturnCode, Context);
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
class TMessageQueue : public std::list<std::pair<WPARAM, LPARAM> >
{
};
//---------------------------------------------------------------------------
const wchar_t CertificateStorageKey[] = L"FtpsCertificates";
//---------------------------------------------------------------------------
class TFTPFileListHelper
{
public:
    explicit TFTPFileListHelper(TFTPFileSystem *FileSystem, TRemoteFileList *FileList,
                    bool IgnoreFileList) :
        FFileSystem(FileSystem),
        FFileList(FFileSystem->FFileList),
        FIgnoreFileList(FFileSystem->FIgnoreFileList)
    {
        FFileSystem->FFileList = FileList;
        FFileSystem->FIgnoreFileList = IgnoreFileList;
    }

    ~TFTPFileListHelper()
    {
        FFileSystem->FFileList = FFileList;
        FFileSystem->FIgnoreFileList = FIgnoreFileList;
    }

private:
    TFTPFileSystem *FFileSystem;
    TRemoteFileList *FFileList;
    bool FIgnoreFileList;
};
//---------------------------------------------------------------------------
TFTPFileSystem::TFTPFileSystem(TTerminal *ATerminal):
    TCustomFileSystem(ATerminal),
    FFileZillaIntf(NULL),
    FQueueCriticalSection(new TCriticalSection()),
    FTransferStatusCriticalSection(new TCriticalSection()),
    FQueue(new TMessageQueue()),
    FQueueEvent(CreateEvent(NULL, true, false, NULL)),
    FReply(0),
    FCommandReply(0),
    FLastCommand(CMD_UNKNOWN),
    FPasswordFailed(false),
    FMultineResponse(false),
    FLastCode(0),
    FLastCodeClass(0),
    FLastReadDirectoryProgress(0),
    FLastResponse(new nb::TStringList()),
    FLastError(new nb::TStringList()),
    FFeatures(new nb::TStringList()),
    FFileList(NULL),
    FFileListCache(NULL),
    FActive(false),
    FWaitingForReply(false),
    FFileTransferAbort(ftaNone),
    FIgnoreFileList(false),
    FFileTransferCancelled(false),
    FFileTransferResumed(0),
    FFileTransferPreserveTime(false),
    FFileTransferCPSLimit(0),
    FAwaitingProgress(false),
    FListAll(asOn),
    FFileSystemInfoValid(false),
    FDoListAll(false),
    FServerCapabilities(NULL)
{
    Self = this;
}

void __fastcall TFTPFileSystem::Init()
{
    TCustomFileSystem::Init();
    ResetReply();

    FListAll = FTerminal->GetSessionData()->GetFtpListAll();
    FFileSystemInfo.ProtocolBaseName = L"FTP";
    FFileSystemInfo.ProtocolName = FFileSystemInfo.ProtocolBaseName;
    FTimeoutStatus = LoadStr(IDS_ERRORMSG_TIMEOUT);
    FDisconnectStatus = LoadStr(IDS_STATUSMSG_DISCONNECTED);
    FServerCapabilities = new TFTPServerCapabilities();
}

//---------------------------------------------------------------------------
TFTPFileSystem::~TFTPFileSystem()
{
    assert(FFileList == NULL);

    FFileZillaIntf->Destroying();

    // to release memory associated with the messages
    DiscardMessages();

    delete FFileZillaIntf;
    FFileZillaIntf = NULL;

    delete FQueue;
    FQueue = NULL;

    ::CloseHandle(FQueueEvent);

    delete FQueueCriticalSection;
    FQueueCriticalSection = NULL;
    delete FTransferStatusCriticalSection;
    FTransferStatusCriticalSection = NULL;

    delete FLastResponse;
    FLastResponse = NULL;
    delete FLastError;
    FLastError = NULL;
    delete FFeatures;
    FFeatures = NULL;
    delete FServerCapabilities;
    FServerCapabilities = NULL;

    ResetCaches();
}
//---------------------------------------------------------------------------
void __fastcall TFTPFileSystem::Open()
{
    // on reconnect, typically there may be pending status messages from previous session
    DiscardMessages();

    ResetCaches();
    FCurrentDirectory = L"";
    FHomeDirectory = L"";

    TSessionData *Data = FTerminal->GetSessionData();

    FSessionInfo.LoginTime = nb::Now();
    FSessionInfo.ProtocolBaseName = L"FTP";
    FSessionInfo.ProtocolName = FSessionInfo.ProtocolBaseName;

    switch (Data->GetFtps())
    {
    case ftpsImplicit:
        FSessionInfo.SecurityProtocolName = LoadStr(FTPS_IMPLICIT);
        break;

    case ftpsExplicitSsl:
        FSessionInfo.SecurityProtocolName = LoadStr(FTPS_EXPLICIT_SSL);
        break;

    case ftpsExplicitTls:
        FSessionInfo.SecurityProtocolName = LoadStr(FTPS_EXPLICIT_TLS);
        break;
    }

    FLastDataSent = nb::Now();

    FMultineResponse = false;

    // initialize FZAPI on the first connect only
    if (FFileZillaIntf == NULL)
    {
        FFileZillaIntf = new TFileZillaImpl(this);

        try
        {
            TFileZillaIntf::TLogLevel LogLevel;
            switch (FTerminal->GetConfiguration()->GetActualLogProtocol())
            {
            default:
            case 0:
            case 1:
                LogLevel = TFileZillaIntf::LOG_WARNING;
                break;

            case 2:
                LogLevel = TFileZillaIntf::LOG_INFO;
                break;
            }
            FFileZillaIntf->SetDebugLevel(LogLevel);

            FFileZillaIntf->Init();
        }
        catch (...)
        {
            delete FFileZillaIntf;
            FFileZillaIntf = NULL;
            throw;
        }
    }

    std::wstring HostName = Data->GetHostNameExpanded();
    std::wstring UserName = Data->GetUserNameExpanded();
    std::wstring Password = Data->GetPassword();
    std::wstring Account = Data->GetFtpAccount();
    std::wstring Path = Data->GetRemoteDirectory();
    int ServerType = 0;
    switch (Data->GetFtps())
    {
    case ftpsImplicit:
        ServerType = TFileZillaIntf::SERVER_FTP_SSL_IMPLICIT;
        break;

    case ftpsExplicitSsl:
        ServerType = TFileZillaIntf::SERVER_FTP_SSL_EXPLICIT;
        break;

    case ftpsExplicitTls:
        ServerType = TFileZillaIntf::SERVER_FTP_TLS_EXPLICIT;
        break;

    default:
        assert(Data->GetFtps() == ftpsNone);
        ServerType = TFileZillaIntf::SERVER_FTP;
        break;
    }
    int Pasv = (Data->GetFtpPasvMode() ? 1 : 2);
    int TimeZoneOffset = static_cast<int>((Round(static_cast<double>(Data->GetTimeDifference()) * MinsPerDay)));
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
        FFeatures->Clear();
        FFileSystemInfoValid = false;

        // TODO: the same for account? it ever used?

        // ask for username if it was not specified in advance, even on retry,
        // but keep previous one as default,
        if (Data->GetUserNameExpanded().empty())
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
        if ((Data->GetPassword().empty() && !Data->GetPasswordless() &&
                !(Data->GetLoginType() == ltAnonymous) && !Data->GetFtpAllowEmptyPassword()) || FPasswordFailed)
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

        FActive = FFileZillaIntf->Connect(
                      HostName.c_str(), Data->GetPortNumber(),
                      UserName.c_str(),
                      Password.c_str(),
                      Account.c_str(),
                      false,
                      Path.c_str(),
                      ServerType, Pasv, TimeZoneOffset, UTF8, Data->GetFtpForcePasvIp());

        assert(FActive);

        FPasswordFailed = false;

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
    }
    while (FPasswordFailed);
}
//---------------------------------------------------------------------------
void __fastcall TFTPFileSystem::Close()
{
    assert(FActive);
    if (FFileZillaIntf->Close())
    {
        CHECK(FLAGSET(WaitForCommandReply(false), TFileZillaIntf::REPLY_DISCONNECTED));
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
bool __fastcall TFTPFileSystem::GetActive()
{
    return FActive;
}
//---------------------------------------------------------------------------
void __fastcall TFTPFileSystem::Idle()
{
    if (FActive && !FWaitingForReply)
    {
        try
        {
            PoolForFatalNonCommandReply();
        }
        catch (EFatal &E)
        {
            if (!FTerminal->QueryReopen(&E, ropNoReadDirectory, NULL))
            {
                throw;
            }
        }

        // Keep session alive
        if ((FTerminal->GetSessionData()->GetFtpPingType() != ptOff) &&
                (static_cast<double>(nb::Now() - FLastDataSent) > static_cast<double>(FTerminal->GetSessionData()->GetFtpPingIntervalDT()) * 4))
        {
            FLastDataSent = nb::Now();

            TRemoteDirectory *Files = new TRemoteDirectory(FTerminal);
            {
                BOOST_SCOPE_EXIT ( (&Files) )
                {
                    delete Files;
                } BOOST_SCOPE_EXIT_END
                try
                {
                    Files->SetDirectory(GetCurrentDirectory());
                    DoReadDirectory(Files);
                }
                catch (...)
                {
                    // ignore non-fatal errors
                    // (i.e. current directory may not exist anymore)
                    if (!FTerminal->GetActive())
                    {
                        throw;
                    }
                }
            }
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TFTPFileSystem::Discard()
{
    // remove all pending messages, to get complete log
    // note that we need to retry discard on reconnect, as there still may be another
    // "disconnect/timeout/..." status messages coming
    DiscardMessages();
    assert(FActive);
    FActive = false;
}
//---------------------------------------------------------------------------
std::wstring __fastcall TFTPFileSystem::AbsolutePath(const std::wstring Path, bool /*Local*/)
{
    // TODO: improve (handle .. etc.)
    if (TTerminal::IsAbsolutePath(Path))
    {
        return Path;
    }
    else
    {
        return ::AbsolutePath(FCurrentDirectory, Path);
    }
}
//---------------------------------------------------------------------------
std::wstring __fastcall TFTPFileSystem::ActualCurrentDirectory()
{
    wchar_t CurrentPath[1024];
    FFileZillaIntf->GetCurrentPath(CurrentPath, LENOF(CurrentPath));
    std::wstring fn = UnixExcludeTrailingBackslash(CurrentPath);
    if (fn.empty())
    {
        fn = L"/";
    }
    return fn;
}
//---------------------------------------------------------------------------
void __fastcall TFTPFileSystem::EnsureLocation()
{
    // if we do not know what's the current directory, do nothing
    if (!FCurrentDirectory.empty())
    {
        // Make sure that the FZAPI current working directory,
        // is actually our working directory.
        // It may not be because:
        // 1) We did cached directory change
        // 2) Listing was requested for non-current directory, which
        // makes FZAPI change its current directory (and not restoring it back afterwards)
        if (!UnixComparePaths(ActualCurrentDirectory(), FCurrentDirectory))
        {
            FTerminal->LogEvent(FORMAT(L"Synchronizing current directory \"%s\".",
                                       FCurrentDirectory.c_str()));
            DoChangeDirectory(FCurrentDirectory);
            // make sure FZAPI is aware that we changed current working directory
            FFileZillaIntf->SetCurrentPath(FCurrentDirectory.c_str());
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TFTPFileSystem::AnyCommand(const std::wstring Command,
                                const captureoutput_slot_type *OutputEvent)
{
    // end-user has right to expect that client current directory is really
    // current directory for the server
    EnsureLocation();

    assert(FOnCaptureOutput.empty());
    if (OutputEvent)
    {
        FOnCaptureOutput.connect(*OutputEvent);
    }
    {
        BOOST_SCOPE_EXIT ( (&Self) )
        {
            Self->FOnCaptureOutput.disconnect_all_slots();
        } BOOST_SCOPE_EXIT_END
        FFileZillaIntf->CustomCommand(Command.c_str());

        GotReply(WaitForCommandReply(), REPLY_2XX_CODE);
    }
}
//---------------------------------------------------------------------------
void __fastcall TFTPFileSystem::ResetCaches()
{
    delete FFileListCache;
    FFileListCache = NULL;
}
//---------------------------------------------------------------------------
void __fastcall TFTPFileSystem::AnnounceFileListOperation()
{
    ResetCaches();
}
//---------------------------------------------------------------------------
void __fastcall TFTPFileSystem::DoChangeDirectory(const std::wstring Directory)
{
    std::wstring Command = FORMAT(L"CWD %s", Directory.c_str());
    FFileZillaIntf->CustomCommand(Command.c_str());

    GotReply(WaitForCommandReply(), REPLY_2XX_CODE);
}
//---------------------------------------------------------------------------
void __fastcall TFTPFileSystem::ChangeDirectory(const std::wstring ADirectory)
{
    std::wstring Directory = ADirectory;
    try
    {
        // For changing directory, we do not make paths absolute, instead we
        // delegate this to the server, hence we sychronize current working
        // directory with the server and only then we ask for the change with
        // relative path.
        // But if synchronization fails, typically because current working directory
        // no longer exists, we fall back to out own resolution, to give
        // user chance to leave the non-existing directory.
        EnsureLocation();
    }
    catch (...)
    {
        if (FTerminal->GetActive())
        {
            Directory = AbsolutePath(Directory, false);
        }
        else
        {
            throw;
        }
    }

    DoChangeDirectory(Directory);

    // make next ReadCurrentDirectory retrieve actual server-side current directory
    FCurrentDirectory = L"";
}
//---------------------------------------------------------------------------
void __fastcall TFTPFileSystem::CachedChangeDirectory(const std::wstring Directory)
{
    FCurrentDirectory = UnixExcludeTrailingBackslash(Directory);
    if (FCurrentDirectory.empty())
    {
        FCurrentDirectory = L"/";
    }
}
//---------------------------------------------------------------------------
void __fastcall TFTPFileSystem::ChangeFileProperties(const std::wstring AFileName,
        const TRemoteFile *File, const TRemoteProperties *Properties,
        TChmodSessionAction &Action)
{
    assert(Properties);
    assert(!Properties->Valid.Contains(vpGroup));
    assert(!Properties->Valid.Contains(vpOwner));
    assert(!Properties->Valid.Contains(vpLastAccess));
    assert(!Properties->Valid.Contains(vpModification));

    if (Properties->Valid.Contains(vpRights))
    {
        assert(Properties);

        TRemoteFile *OwnedFile = NULL;

        {
            BOOST_SCOPE_EXIT ( (&OwnedFile) )
            {
                delete OwnedFile;
            } BOOST_SCOPE_EXIT_END
            std::wstring FileName = AbsolutePath(AFileName, false);

            if (File == NULL)
            {
                ReadFile(FileName, OwnedFile);
                File = OwnedFile;
            }

            if ((File != NULL) && File->GetIsDirectory() && !File->GetIsSymLink() && Properties->Recursive)
            {
                try
                {
                    FTerminal->ProcessDirectory(AFileName, boost::bind(&TTerminal::ChangeFileProperties, FTerminal, _1, _2, _3),
                                                static_cast<void *>(const_cast<TRemoteProperties *>(Properties)));
                }
                catch (...)
                {
                    Action.Cancel();
                    throw;
                }
            }

            TRights Rights;
            if (File != NULL)
            {
                Rights = *File->GetRights();
            }
            Rights |= Properties->Rights.GetNumberSet();
            Rights &= static_cast<unsigned short>(~Properties->Rights.GetNumberUnset());
            if ((File != NULL) && File->GetIsDirectory() && Properties->AddXToDirectories)
            {
                Rights.AddExecute();
            }

            Action.Rights(Rights);

            std::wstring FileNameOnly = UnixExtractFileName(FileName);
            std::wstring FilePath = UnixExtractFilePath(FileName);
            // FZAPI wants octal number represented as decadic
            FFileZillaIntf->Chmod(Rights.GetNumberDecadic(),
                                  FileNameOnly.c_str(),
                                  FilePath.c_str());

            GotReply(WaitForCommandReply(), REPLY_2XX_CODE);
        }
    }
    else
    {
        Action.Cancel();
    }
}
//---------------------------------------------------------------------------
bool __fastcall TFTPFileSystem::LoadFilesProperties(nb::TStrings * /*FileList*/)
{
    assert(false);
    return false;
}
//---------------------------------------------------------------------------
void __fastcall TFTPFileSystem::CalculateFilesChecksum(const std::wstring /*Alg*/,
        nb::TStrings * /*FileList*/, nb::TStrings * /*Checksums*/,
        calculatedchecksum_slot_type * /*OnCalculatedChecksum*/)
{
    assert(false);
}
//---------------------------------------------------------------------------
bool __fastcall TFTPFileSystem::ConfirmOverwrite(std::wstring &FileName,
    int Params, TFileOperationProgressType *OperationProgress,
    TOverwriteMode &OverwriteMode,
    bool AutoResume,
    const TOverwriteFileParams *FileParams)
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
void __fastcall TFTPFileSystem::ResetFileTransfer()
{
    FFileTransferAbort = ftaNone;
    FFileTransferCancelled = false;
    FFileTransferResumed = 0;
}
//---------------------------------------------------------------------------
void __fastcall TFTPFileSystem::ReadDirectoryProgress(__int64 Bytes)
{
    // with FTP we do not know exactly how many entries we have received,
    // instead we know number of bytes received only.
    // so we report approximation based on average size of entry.
    size_t Progress = static_cast<size_t>(Bytes / 80);
    if (Progress - FLastReadDirectoryProgress >= 10)
    {
        bool Cancel = false;
        FLastReadDirectoryProgress = Progress;
        FTerminal->DoReadDirectoryProgress(Progress, Cancel);
        if (Cancel)
        {
            FTerminal->DoReadDirectoryProgress(static_cast<size_t>(-2), Cancel);
            FFileZillaIntf->Cancel();
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TFTPFileSystem::DoFileTransferProgress(__int64 TransferSize,
        __int64 Bytes)
{
    TFileOperationProgressType *OperationProgress = FTerminal->GetOperationProgress();

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
        FFileZillaIntf->Cancel();
    }

    if (FFileTransferCPSLimit != OperationProgress->CPSLimit)
    {
        FFileTransferCPSLimit = OperationProgress->CPSLimit;
    }
}
//---------------------------------------------------------------------------
void __fastcall TFTPFileSystem::FileTransferProgress(__int64 TransferSize,
        __int64 Bytes)
{
    TGuard Guard(FTransferStatusCriticalSection);

    DoFileTransferProgress(TransferSize, Bytes);
}
//---------------------------------------------------------------------------
void __fastcall TFTPFileSystem::FileTransfer(const std::wstring FileName,
                                  const std::wstring LocalFile, const std::wstring RemoteFile,
                                  const std::wstring RemotePath, bool Get, __int64 Size, int Type,
                                  TFileTransferData &UserData, TFileOperationProgressType *OperationProgress)
{
    FILE_OPERATION_LOOP(FMTLOAD(TRANSFER_ERROR, FileName.c_str()),
        FFileZillaIntf->FileTransfer(
            LocalFile.c_str(),
            RemoteFile.c_str(),
            RemotePath.c_str(),
            Get, Size, Type, &UserData);
        // we may actually catch response code of the listing
        // command (when checking for existence of the remote file)
        unsigned int Reply = WaitForCommandReply();
        GotReply(Reply, FLAGMASK(FFileTransferCancelled, REPLY_ALLOW_CANCEL));
    );

    switch (FFileTransferAbort)
    {
    case ftaSkip:
        THROW_SKIP_FILE_NULL;

    case ftaCancel:
        nb::Abort();
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
//---------------------------------------------------------------------------
void __fastcall TFTPFileSystem::CopyToLocal(nb::TStrings *FilesToCopy,
                                 const std::wstring TargetDir, const TCopyParamType *CopyParam,
                                 int Params, TFileOperationProgressType *OperationProgress,
                                 TOnceDoneOperation &OnceDoneOperation)
{
    Params &= ~cpAppend;
    std::wstring FullTargetDir = IncludeTrailingBackslash(TargetDir);

    size_t Index = 0;
    while (Index < FilesToCopy->GetCount() && !OperationProgress->Cancel)
    {
        std::wstring FileName = FilesToCopy->GetString(Index);
        const TRemoteFile *File = dynamic_cast<const TRemoteFile *>(FilesToCopy->GetObject(Index));
        bool Success = false;

        {
            BOOST_SCOPE_EXIT ( (&OperationProgress) (&FileName) (&Success) (&OnceDoneOperation) )
            {
                OperationProgress->Finish(FileName, Success, OnceDoneOperation);
            } BOOST_SCOPE_EXIT_END
            try
            {
                SinkRobust(AbsolutePath(FileName, false), File, FullTargetDir, CopyParam, Params,
                           OperationProgress, tfFirstLevel);
                Success = true;
                FLastDataSent = nb::Now();
            }
            catch (const EScpSkipFile &E)
            {
                SUSPEND_OPERATION (
                    if (!FTerminal->HandleException(&E)) throw;
                );
            }
        }
        Index++;
    }
}
//---------------------------------------------------------------------------
void __fastcall TFTPFileSystem::SinkRobust(const std::wstring FileName,
                                const TRemoteFile *File, const std::wstring TargetDir,
                                const TCopyParamType *CopyParam, int Params,
                                TFileOperationProgressType *OperationProgress, unsigned int Flags)
{
    // the same in TSFTPFileSystem
    bool Retry;

    TDownloadSessionAction Action(FTerminal->GetLog());

    do
    {
        Retry = false;
        try
        {
            Sink(FileName, File, TargetDir, CopyParam, Params, OperationProgress,
                 Flags, Action);
        }
        catch (std::exception &E)
        {
            Retry = true;
            if (FTerminal->GetActive() ||
                    !FTerminal->QueryReopen(&E, ropNoReadDirectory, OperationProgress))
            {
                FTerminal->RollbackAction(Action, OperationProgress, &E);
                throw;
            }
        }

        if (Retry)
        {
            OperationProgress->RollbackTransfer();
            Action.Restart();
            assert(File != NULL);
            if (!File->GetIsDirectory())
            {
                // prevent overwrite confirmations
                Params |= cpNoConfirmation;
                Flags |= tfAutoResume;
            }
        }
    }
    while (Retry);
}
//---------------------------------------------------------------------------
void __fastcall TFTPFileSystem::Sink(const std::wstring FileName,
                          const TRemoteFile *File, const std::wstring TargetDir,
                          const TCopyParamType *CopyParam, int Params,
                          TFileOperationProgressType *OperationProgress, unsigned int Flags,
                          TDownloadSessionAction &Action)
{
    std::wstring OnlyFileName = UnixExtractFileName(FileName);

    Action.FileName(FileName);

    assert(File);
    TFileMasks::TParams MaskParams;
    MaskParams.Size = File->GetSize();
    MaskParams.Modification = File->GetModification();

    if (!CopyParam->AllowTransfer(FileName, osRemote, File->GetIsDirectory(), MaskParams))
    {
        FTerminal->LogEvent(FORMAT(L"File \"%s\" excluded from transfer", FileName.c_str()));
        THROW_SKIP_FILE_NULL;
    }

    FTerminal->LogEvent(FORMAT(L"File: \"%s\"", FileName.c_str()));

    OperationProgress->SetFile(OnlyFileName);

    std::wstring DestFileName = CopyParam->ChangeFileName(OnlyFileName,
                                osRemote, FLAGSET(Flags, tfFirstLevel));
    std::wstring DestFullName = TargetDir + DestFileName;

    if (File->GetIsDirectory())
    {
        Action.Cancel();
        if (!File->GetIsSymLink())
        {
            FILE_OPERATION_LOOP (FMTLOAD(NOT_DIRECTORY_ERROR, DestFullName.c_str()),
                int Attrs = FileGetAttr(DestFullName);
                if (FLAGCLEAR(Attrs, faDirectory))
                {
                    EXCEPTION;
                }
            );

            FILE_OPERATION_LOOP (FMTLOAD(CREATE_DIR_ERROR, DestFullName.c_str()),
                if (!ForceDirectories(DestFullName))
                {
                    RaiseLastOSError();
                }
            );

            TSinkFileParams SinkFileParams;
            SinkFileParams.TargetDir = IncludeTrailingBackslash(DestFullName);
            SinkFileParams.CopyParam = CopyParam;
            SinkFileParams.Params = Params;
            SinkFileParams.OperationProgress = OperationProgress;
            SinkFileParams.Skipped = false;
            SinkFileParams.Flags = Flags & ~(tfFirstLevel | tfAutoResume);

            FTerminal->ProcessDirectory(FileName, boost::bind(&TFTPFileSystem::SinkFile, this, _1, _2, _3), &SinkFileParams);

            // Do not delete directory if some of its files were skip.
            // Throw "skip file" for the directory to avoid attempt to deletion
            // of any parent directory
            if (FLAGSET(Params, cpDelete) && SinkFileParams.Skipped)
            {
                THROW_SKIP_FILE_NULL;
            }
        }
        else
        {
            // file is symlink to directory, currently do nothing, but it should be
            // reported to user
        }
    }
    else
    {
        FTerminal->LogEvent(FORMAT(L"Copying \"%s\" to local directory started.", FileName.c_str()));

        // Will we use ASCII of BINARY file tranfer?
        OperationProgress->SetAsciiTransfer(
            CopyParam->UseAsciiTransfer(FileName, osRemote, MaskParams));
        FTerminal->LogEvent(std::wstring((OperationProgress->AsciiTransfer ? L"Ascii" : L"Binary")) +
                            L" transfer mode selected.");

        // Suppose same data size to transfer as to write
        OperationProgress->SetTransferSize(File->GetSize());
        OperationProgress->SetLocalSize(OperationProgress->TransferSize);

        int Attrs = 0;
        FILE_OPERATION_LOOP (FMTLOAD(NOT_FILE_ERROR, DestFullName.c_str()),
            Attrs = FileGetAttr(DestFullName);
            if ((Attrs >= 0) && FLAGSET(Attrs, faDirectory))
            {
                EXCEPTION;
            }
        );

        OperationProgress->TransferingFile = false; // not set with FTP protocol

        ResetFileTransfer();

        TFileTransferData UserData;

        std::wstring FilePath = UnixExtractFilePath(FileName);
        if (FilePath.empty())
        {
            FilePath = L"/";
        }
        unsigned int TransferType = (OperationProgress->AsciiTransfer ? 1 : 2);

        {
            // ignore file list
            TFTPFileListHelper Helper(this, NULL, true);

            FFileTransferCPSLimit = OperationProgress->CPSLimit;
            FFileTransferPreserveTime = CopyParam->GetPreserveTime();
            UserData.FileName = DestFileName;
            UserData.Params = Params;
            UserData.AutoResume = FLAGSET(Flags, tfAutoResume);
            UserData.CopyParam = CopyParam;
            FileTransfer(FileName, DestFullName, OnlyFileName,
                         FilePath, true, File->GetSize(), TransferType, UserData, OperationProgress);
        }

        // in case dest filename is changed from overwrite dialog
        if (DestFileName != UserData.FileName)
        {
            DestFullName = TargetDir + UserData.FileName;
            Attrs = FileGetAttr(DestFullName);
        }

        Action.Destination(ExpandUNCFileName(DestFullName));

        if (Attrs == -1)
        {
            Attrs = faArchive;
        }
        int NewAttrs = CopyParam->LocalFileAttrs(*File->GetRights());
        if ((NewAttrs & Attrs) != NewAttrs)
        {
            FILE_OPERATION_LOOP (FMTLOAD(CANT_SET_ATTRS, DestFullName.c_str()),
                THROWOSIFFALSE(FileSetAttr(DestFullName, Attrs | NewAttrs) == 0);
            );
        }
    }

    if (FLAGSET(Params, cpDelete))
    {
        // If file is directory, do not delete it recursively, because it should be
        // empty already. If not, it should not be deleted (some files were
        // skipped or some new files were copied to it, while we were downloading)
        int Params = dfNoRecursive;
        FTerminal->DeleteFile(FileName, File, &Params);
    }
}
//---------------------------------------------------------------------------
void TFTPFileSystem::SinkFile(const std::wstring FileName,
                              const TRemoteFile *File, void *Param)
{
    TSinkFileParams *Params = static_cast<TSinkFileParams *>(Param);
    assert(Params->OperationProgress);
    try
    {
        SinkRobust(FileName, File, Params->TargetDir, Params->CopyParam,
                   Params->Params, Params->OperationProgress, Params->Flags);
    }
    catch (const EScpSkipFile &E)
    {
        TFileOperationProgressType *OperationProgress = Params->OperationProgress;

        Params->Skipped = true;
        SUSPEND_OPERATION (
            if (!FTerminal->HandleException(&E))
            {
                throw;
            }
        );

        if (OperationProgress->Cancel)
        {
            nb::Abort();
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TFTPFileSystem::CopyToRemote(nb::TStrings *FilesToCopy,
                                  const std::wstring ATargetDir, const TCopyParamType *CopyParam,
                                  int Params, TFileOperationProgressType *OperationProgress,
                                  TOnceDoneOperation &OnceDoneOperation)
{
    assert((FilesToCopy != NULL) && (OperationProgress != NULL));

    Params &= ~cpAppend;
    std::wstring FileName, FileNameOnly;
    std::wstring TargetDir = AbsolutePath(ATargetDir, false);
    std::wstring FullTargetDir = UnixIncludeTrailingBackslash(TargetDir);
    size_t Index = 0;
    while ((Index < FilesToCopy->GetCount()) && !OperationProgress->Cancel)
    {
        bool Success = false;
        FileName = FilesToCopy->GetString(Index);
        FileNameOnly = ExtractFileName(FileName, false);

        {
            BOOST_SCOPE_EXIT ( (&OperationProgress) (&FileName) (&Success) (&OnceDoneOperation) )
            {
                OperationProgress->Finish(FileName, Success, OnceDoneOperation);
            } BOOST_SCOPE_EXIT_END
            try
            {
                if (FTerminal->GetSessionData()->GetCacheDirectories())
                {
                    FTerminal->DirectoryModified(TargetDir, false);

                    if (::DirectoryExists(::ExtractFilePath(FileName)))
                    {
                        FTerminal->DirectoryModified(FullTargetDir + FileNameOnly, true);
                    }
                }
                SourceRobust(FileName, FullTargetDir, CopyParam, Params, OperationProgress,
                             tfFirstLevel);
                Success = true;
                FLastDataSent = nb::Now();
            }
            catch (const EScpSkipFile &E)
            {
                SUSPEND_OPERATION (
                    if (!FTerminal->HandleException(&E)) throw;
                );
            }
        }
        Index++;
    }
}
//---------------------------------------------------------------------------
void __fastcall TFTPFileSystem::SourceRobust(const std::wstring FileName,
                                  const std::wstring TargetDir, const TCopyParamType *CopyParam, int Params,
                                  TFileOperationProgressType *OperationProgress, unsigned int Flags)
{
    // the same in TSFTPFileSystem
    bool Retry;

    TUploadSessionAction Action(FTerminal->GetLog());
    TOpenRemoteFileParams OpenParams;
    OpenParams.OverwriteMode = omOverwrite;
    TOverwriteFileParams FileParams;

    do
    {
        Retry = false;
        try
        {
            Source(FileName, TargetDir, CopyParam, Params,
                   &OpenParams, &FileParams, OperationProgress,
                   Flags, Action);
        }
        catch (std::exception &E)
        {
            Retry = true;
            if (FTerminal->GetActive() ||
                    !FTerminal->QueryReopen(&E, ropNoReadDirectory, OperationProgress))
            {
                FTerminal->RollbackAction(Action, OperationProgress, &E);
                throw;
            }
        }

        if (Retry)
        {
            OperationProgress->RollbackTransfer();
            Action.Restart();
            // prevent overwrite confirmations
            // (should not be set for directories!)
            Params |= cpNoConfirmation;
            Flags |= tfAutoResume;
        }
    }
    while (Retry);
}
//---------------------------------------------------------------------------
// Copy file to remote host
void __fastcall TFTPFileSystem::Source(const std::wstring FileName,
                            const std::wstring TargetDir, const TCopyParamType *CopyParam, int Params,
                            TOpenRemoteFileParams *OpenParams,
                            TOverwriteFileParams *FileParams,
                            TFileOperationProgressType *OperationProgress, unsigned int Flags,
                            TUploadSessionAction &Action)
{
    FTerminal->LogEvent(FORMAT(L"File: \"%s\"", FileName.c_str()));

    Action.FileName(ExpandUNCFileName(FileName));

    OperationProgress->SetFile(FileName, false);

    if (!FTerminal->AllowLocalFileTransfer(FileName, CopyParam))
    {
        FTerminal->LogEvent(FORMAT(L"File \"%s\" excluded from transfer", FileName.c_str()));
        THROW_SKIP_FILE_NULL;
    }

    __int64 MTime = 0, ATime = 0;
    __int64 Size = 0;
      // int Attrs = 0;

    FTerminal->OpenLocalFile(FileName, GENERIC_READ, &OpenParams->LocalFileAttrs,
                             NULL, NULL, &MTime, &ATime, &Size);

    OperationProgress->SetFileInProgress();

    bool Dir = FLAGSET(OpenParams->LocalFileAttrs, faDirectory);
    if (Dir)
    {
        Action.Cancel();
        DirectorySource(IncludeTrailingBackslash(FileName), TargetDir,
                        OpenParams->LocalFileAttrs, CopyParam, Params, OperationProgress, Flags);
    }
    else
    {
        std::wstring DestFileName = CopyParam->ChangeFileName(ExtractFileName(FileName, false),
                                    osLocal, FLAGSET(Flags, tfFirstLevel));

        FTerminal->LogEvent(FORMAT(L"Copying \"%s\" to remote directory started.", FileName.c_str()));

        OperationProgress->SetLocalSize(Size);

        // Suppose same data size to transfer as to read
        // (not true with ASCII transfer)
        OperationProgress->SetTransferSize(OperationProgress->LocalSize);
        OperationProgress->TransferingFile = false;

        nb::TDateTime Modification;
        // Inspired by SysUtils::FileAge
        WIN32_FIND_DATA FindData;
        HANDLE Handle = FindFirstFile(FileName.c_str(), &FindData);
        if (Handle != INVALID_HANDLE_VALUE)
        {
          Modification =
            UnixToDateTime(
              ConvertTimestampToUnixSafe(FindData.ftLastWriteTime, dstmUnix),
              dstmUnix);
          FindClose(Handle);
        }

        // Will we use ASCII of BINARY file tranfer?
        TFileMasks::TParams MaskParams;
        MaskParams.Size = Size;
        MaskParams.Modification = Modification;
        OperationProgress->SetAsciiTransfer(
            CopyParam->UseAsciiTransfer(FileName, osLocal, MaskParams));
        FTerminal->LogEvent(
            std::wstring((OperationProgress->AsciiTransfer ? L"Ascii" : L"Binary")) +
            L" transfer mode selected.");

        ResetFileTransfer();

        TFileTransferData UserData;

        unsigned int TransferType = (OperationProgress->AsciiTransfer ? 1 : 2);

        // should we check for interrupted transfer?
        bool ResumeAllowed = !OperationProgress->AsciiTransfer &&
                        CopyParam->AllowResume(OperationProgress->LocalSize) &&
                        IsCapable(fcRename);
        OperationProgress->SetResumeStatus(ResumeAllowed ? rsEnabled : rsDisabled);

        FileParams->SourceSize = OperationProgress->LocalSize;
        FileParams->SourceTimestamp = UnixToDateTime(MTime,
                                     FTerminal->GetSessionData()->GetDSTMode());
        bool DoResume = (ResumeAllowed && (OpenParams->OverwriteMode == omOverwrite));
        {
            // ignore file list
            TFTPFileListHelper Helper(this, NULL, true);

            FFileTransferCPSLimit = OperationProgress->CPSLimit;
            // not used for uploads anyway
            FFileTransferPreserveTime = CopyParam->GetPreserveTime();
            // not used for uploads, but we get new name (if any) back in this field
            UserData.FileName = DestFileName;
            UserData.Params = Params;
            UserData.AutoResume = FLAGSET(Flags, tfAutoResume) || DoResume;
            UserData.CopyParam = CopyParam;
            FileTransfer(FileName, FileName, DestFileName,
                         TargetDir, false, Size, TransferType, UserData, OperationProgress);
        }

        std::wstring DestFullName = TargetDir + UserData.FileName;
        // only now, we know the final destination
        Action.Destination(DestFullName);

        // we are not able to tell if setting timestamp succeeded,
        // so we log it always (if supported)
        if (FFileTransferPreserveTime && (FServerCapabilities->GetCapability(mfmt_command) == yes))
        {
/*
            // Inspired by SysUtils::FileAge
            WIN32_FIND_DATA FindData;
            HANDLE Handle = FindFirstFile(FileName.c_str(), &FindData);
            if (Handle != INVALID_HANDLE_VALUE)
            {
                TTouchSessionAction TouchAction(FTerminal->GetLog(), DestFullName,
                                                UnixToDateTime(
                                                    ConvertTimestampToUnixSafe(FindData.ftLastWriteTime, dstmUnix),
                                                    dstmUnix));
                FindClose(Handle);
            }
*/
            TTouchSessionAction TouchAction(FTerminal->GetActionLog(), DestFullName, Modification);
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
    else if (CopyParam->GetClearArchive() && FLAGSET(OpenParams->LocalFileAttrs, faArchive))
    {
        FILE_OPERATION_LOOP (FMTLOAD(CANT_SET_ATTRS, FileName.c_str()),
            THROWOSIFFALSE(FileSetAttr(FileName, OpenParams->LocalFileAttrs & ~faArchive) == 0);
        )
    }
}
//---------------------------------------------------------------------------
void __fastcall TFTPFileSystem::DirectorySource(const std::wstring DirectoryName,
                                     const std::wstring TargetDir, int Attrs, const TCopyParamType *CopyParam,
                                     int Params, TFileOperationProgressType *OperationProgress, unsigned int Flags)
{
    std::wstring DestDirectoryName = CopyParam->ChangeFileName(
                                         ExtractFileName(ExcludeTrailingBackslash(DirectoryName), false), osLocal,
                                         FLAGSET(Flags, tfFirstLevel));
    std::wstring DestFullName = UnixIncludeTrailingBackslash(TargetDir + DestDirectoryName);

    OperationProgress->SetFile(DirectoryName);

    WIN32_FIND_DATA SearchRec;
    bool FindOK = false;
    HANDLE findHandle = 0;

    FILE_OPERATION_LOOP (FMTLOAD(LIST_DIR_ERROR, DirectoryName.c_str()),
        std::wstring path = DirectoryName + L"*.*";
        findHandle = FindFirstFile(path.c_str(), &SearchRec);
        FindOK = (findHandle != 0);
    );

    bool CreateDir = true;

    {
        BOOST_SCOPE_EXIT ( (&SearchRec) (&findHandle) )
        {
            ::FindClose(findHandle);
        } BOOST_SCOPE_EXIT_END
        while (FindOK && !OperationProgress->Cancel)
        {
            std::wstring FileName = DirectoryName + SearchRec.cFileName;
            try
            {
                if ((wcscmp(SearchRec.cFileName, THISDIRECTORY) != 0) && (wcscmp(SearchRec.cFileName, PARENTDIRECTORY) != 0))
                {
                    SourceRobust(FileName, DestFullName, CopyParam, Params, OperationProgress,
                                 Flags & ~(tfFirstLevel | tfAutoResume));
                    // if any file got uploaded (i.e. there were any file in the
                    // directory and at least one was not skipped),
                    // do not try to create the directory,
                    // as it should be already created by FZAPI during upload
                    CreateDir = false;
                }
            }
            catch (const EScpSkipFile &E)
            {
                // If ESkipFile occurs, just log it and continue with next file
                DEBUG_PRINTF(L"before FTerminal->HandleException");
                SUSPEND_OPERATION (
                    // here a message to user was displayed, which was not appropriate
                    // when user refused to overwrite the file in subdirectory.
                    // hopefuly it won't be missing in other situations.
                    if (!FTerminal->HandleException(&E)) throw;
                );
            }

            FILE_OPERATION_LOOP (FMTLOAD(LIST_DIR_ERROR, DirectoryName.c_str()),
                FindOK = (::FindNextFile(findHandle, &SearchRec) != 0);
            );
        };
    }

    if (CreateDir)
    {
        TRemoteProperties Properties;
        if (CopyParam->GetPreserveRights())
        {
            Properties.Valid = TValidProperties() << vpRights;
            Properties.Rights = CopyParam->RemoteFileRights(Attrs);
        }

        try
        {
            FTerminal->SetExceptionOnFail(true);
            {
                BOOST_SCOPE_EXIT ( (&Self) )
                {
                    Self->FTerminal->SetExceptionOnFail(false);
                } BOOST_SCOPE_EXIT_END
                FTerminal->CreateDirectory(DestFullName, &Properties);
            }
        }
        catch (...)
        {
            TRemoteFile *File = NULL;
            // ignore non-fatal error when the directory already exists
            std::wstring fn = UnixExcludeTrailingBackslash(DestFullName);
            if (fn.empty())
            {
                fn = L"/";
            }
            bool Rethrow =
                !FTerminal->GetActive() ||
                !FTerminal->FileExists(fn, &File) ||
                !File->GetIsDirectory();
            delete File;
            if (Rethrow)
            {
                throw;
            }
        }
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
//---------------------------------------------------------------------------
void __fastcall TFTPFileSystem::CreateDirectory(const std::wstring ADirName)
{
    std::wstring DirName = AbsolutePath(ADirName, false);

    {
        // ignore file list
        TFTPFileListHelper Helper(this, NULL, true);

        FFileZillaIntf->MakeDir(DirName.c_str());

        GotReply(WaitForCommandReply(), REPLY_2XX_CODE);
    }
}
//---------------------------------------------------------------------------
void __fastcall TFTPFileSystem::CreateLink(const std::wstring /*FileName*/,
                                const std::wstring /*PointTo*/, bool /*Symbolic*/)
{
    assert(false);
}
//---------------------------------------------------------------------------
void __fastcall TFTPFileSystem::DeleteFile(const std::wstring AFileName,
                                const TRemoteFile *File, int Params, TRmSessionAction &Action)
{
    std::wstring FileName = AbsolutePath(AFileName, false);
    std::wstring FileNameOnly = UnixExtractFileName(FileName);
    std::wstring FilePath = UnixExtractFilePath(FileName);

    bool Dir = (File != NULL) && File->GetIsDirectory() && !File->GetIsSymLink();

    if (Dir && FLAGCLEAR(Params, dfNoRecursive))
    {
        try
        {
            FTerminal->ProcessDirectory(FileName, boost::bind(&TTerminal::DeleteFile, FTerminal, _1, _2, _3), &Params);
        }
        catch (...)
        {
            Action.Cancel();
            throw;
        }
    }

    {
        // ignore file list
        TFTPFileListHelper Helper(this, NULL, true);

        if (Dir)
        {
            // If current remote directory is in the directory being removed,
            // some servers may refuse to delete it
            // This is common as ProcessDirectory above would CWD to
            // the directory to LIST it.
            // EnsureLocation should reset actual current directory to user's working directory.
            // If user's working directory is still below deleted directory, it is
            // perfectly correct to report an error.
            if (UnixIsChildPath(ActualCurrentDirectory(), FileName))
            {
                EnsureLocation();
            }
            FFileZillaIntf->RemoveDir(FileNameOnly.c_str(),
                                      FilePath.c_str());
        }
        else
        {
            FFileZillaIntf->Delete(FileNameOnly.c_str(),
                                   FilePath.c_str());
        }
        GotReply(WaitForCommandReply(), REPLY_2XX_CODE);
    }
}
//---------------------------------------------------------------------------
void __fastcall TFTPFileSystem::CustomCommandOnFile(const std::wstring /*FileName*/,
        const TRemoteFile * /*File*/, const std::wstring /*Command*/, int /*Params*/,
        const captureoutput_slot_type &/*OutputEvent*/)
{
    // if ever implemented, do not forget to add EnsureLocation,
    // see AnyCommand for a reason why
    assert(false);
}
//---------------------------------------------------------------------------
void __fastcall TFTPFileSystem::DoStartup()
{
    nb::TStrings *PostLoginCommands = new nb::TStringList();
    {
        BOOST_SCOPE_EXIT ( (&PostLoginCommands) )
        {
            delete PostLoginCommands;
        } BOOST_SCOPE_EXIT_END
        PostLoginCommands->SetText(FTerminal->GetSessionData()->GetPostLoginCommands());
        for (size_t Index = 0; Index < PostLoginCommands->GetCount(); Index++)
        {
            std::wstring Command = PostLoginCommands->GetString(Index);
            if (!Command.empty())
            {
                FFileZillaIntf->CustomCommand(Command.c_str());

                GotReply(WaitForCommandReply(), REPLY_2XX_CODE);
            }
        }
    }

    // retrieve initialize working directory to save it as home directory
    ReadCurrentDirectory();
    FHomeDirectory = FCurrentDirectory;
}
//---------------------------------------------------------------------------
void __fastcall TFTPFileSystem::HomeDirectory()
{
    // FHomeDirectory is an absolute path, so avoid unnecessary overhead
    // of ChangeDirectory, such as EnsureLocation
    DoChangeDirectory(FHomeDirectory);
    FCurrentDirectory = FHomeDirectory;
    // make sure FZAPI is aware that we changed current working directory
    FFileZillaIntf->SetCurrentPath(FCurrentDirectory.c_str());
}
//---------------------------------------------------------------------------
bool __fastcall TFTPFileSystem::IsCapable(int Capability) const
{
    assert(FTerminal);
    switch (Capability)
    {
    case fcResolveSymlink: // sic
    case fcTextMode:
    case fcModeChanging: // but not fcModeChangingUpload
    case fcNewerOnlyUpload:
    case fcAnyCommand: // but not fcShellAnyCommand
    case fcRename:
    case fcRemoteMove:
        return true;

    case fcPreservingTimestampUpload:
        return FServerCapabilities->GetCapability(mfmt_command) == yes;

    case fcModeChangingUpload:
    case fcLoadingAdditionalProperties:
    case fcShellAnyCommand:
    case fcCalculatingChecksum:
    case fcHardLink:
    case fcSymbolicLink:
    case fcCheckingSpaceAvailable:
    case fcUserGroupListing:
    case fcGroupChanging:
    case fcOwnerChanging:
    case fcGroupOwnerChangingByID:
    case fcSecondaryShell:
    case fcRemoteCopy:
    case fcNativeTextMode:
    case fcTimestampChanging:
    case fcIgnorePermErrors:
        return false;

    default:
        assert(false);
        return false;
    }
}
//---------------------------------------------------------------------------
void __fastcall TFTPFileSystem::LookupUsersGroups()
{
    assert(false);
}
//---------------------------------------------------------------------------
void __fastcall TFTPFileSystem::ReadCurrentDirectory()
{
    // ask the server for current directory on startup only
    // and immediatelly after call to CWD,
    // later our current directory may be not synchronized with FZAPI current
    // directory anyway, see comments in EnsureLocation
    if (FCurrentDirectory.empty())
    {
        FFileZillaIntf->CustomCommand(L"PWD");

        unsigned int Code = 0;
        nb::TStrings *Response = NULL;
        GotReply(WaitForCommandReply(), REPLY_2XX_CODE, L"", &Code, &Response);

        {
            BOOST_SCOPE_EXIT ( (&Response) )
            {
                delete Response;
            } BOOST_SCOPE_EXIT_END
            assert(Response != NULL);
            bool Result = false;

            // the only allowed 2XX code to "PWD"
            if ((Code == 257) &&
                    (Response->GetCount() == 1))
            {
                std::wstring Path = Response->GetText();

                size_t P = ::Pos(Path, L"\"");
                if (P == std::wstring::npos)
                {
                    // some systems use single quotes, be tolerant
                    P = ::Pos(Path, L"'");
                }
                if (P != std::wstring::npos)
                {
                    Path.erase(0, P);

                    if (Unquote(Path))
                    {
                        FCurrentDirectory = UnixExcludeTrailingBackslash(Path);
                        if (FCurrentDirectory.empty())
                        {
                            FCurrentDirectory = L"/";
                        }
                        Result = true;
                    }
                }
            }

            if (Result)
            {
                FFileZillaIntf->SetCurrentPath(FCurrentDirectory.c_str());
            }
            else
            {
                throw ExtException(FMTLOAD(FTP_PWD_RESPONSE_ERROR, Response->GetText().c_str()));
            }
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TFTPFileSystem::DoReadDirectory(TRemoteFileList *FileList)
{
    FileList->Clear();
    // FZAPI does not list parent directory, add it
    FileList->AddFile(new TRemoteParentDirectory(FTerminal));

    FLastReadDirectoryProgress = 0;

    TFTPFileListHelper Helper(this, FileList, false);

    // always specify path to list, do not attempt to list "current" dir as:
    // 1) List() lists again the last listed directory, not the current working directory
    // 2) we handle this way the cached directory change
    std::wstring Directory = AbsolutePath(FileList->GetDirectory(), false);
    FFileZillaIntf->List(Directory.c_str());

    GotReply(WaitForCommandReply(), REPLY_2XX_CODE | REPLY_ALLOW_CANCEL);

    FLastDataSent = nb::Now();
}
//---------------------------------------------------------------------------
void __fastcall TFTPFileSystem::ReadDirectory(TRemoteFileList *FileList)
{
    bool GotNoFilesForAll = false;
    bool Repeat = false;

    do
    {
        Repeat = false;
        try
        {
            FDoListAll = (FListAll == asAuto) || (FListAll == asOn);
            DoReadDirectory(FileList);

            // We got no files with "-a", but again no files w/o "-a",
            // so it was not "-a"'s problem, revert to auto and let it decide the next time
            if (GotNoFilesForAll && (FileList->GetCount() == 0))
            {
                assert(FListAll == asOff);
                FListAll = asAuto;
            }
            else if (FListAll == asAuto)
            {
                // some servers take "-a" as a mask and return empty directory listing
                // (note that it's actually never empty here, there's always at least parent directory,
                // added explicitly by DoReadDirectory)
                if ((FileList->GetCount() == 0) ||
                        ((FileList->GetCount() == 1) && FileList->GetFile(0)->GetIsParentDirectory()))
                {
                    Repeat = true;
                    FListAll = asOff;
                    GotNoFilesForAll = true;
                }
                else
                {
                    // reading first directory has succeeded, always use "-a"
                    FListAll = asOn;
                }
            }

            // use "-a" even for implicit directory reading by FZAPI?
            // (e.g. before file transfer)
            FDoListAll = (FListAll == asOn);
        }
        catch (ExtException &E)
        {
            FDoListAll = false;
            // reading the first directory has failed,
            // further try without "-a" only as the server may not support it
            if (FListAll == asAuto)
            {
                if (!FTerminal->GetActive())
                {
                    FTerminal->Reopen(ropNoReadDirectory);
                }

                FListAll = asOff;
                Repeat = true;
            }
            else
            {
                throw;
            }
        }
    }
    while (Repeat);
}
//---------------------------------------------------------------------------
void __fastcall TFTPFileSystem::DoReadFile(const std::wstring FileName, TRemoteFile *& AFile)
{
    TRemoteFileList *FileList = new TRemoteFileList();
    {
        BOOST_SCOPE_EXIT ( (&FileList) )
        {
            delete FileList;
        } BOOST_SCOPE_EXIT_END
        TFTPFileListHelper Helper(this, FileList, false);
        FFileZillaIntf->ListFile(FileName.c_str());

        GotReply(WaitForCommandReply(), REPLY_2XX_CODE | REPLY_ALLOW_CANCEL);
        TRemoteFile *File = FileList->FindFile(FileName);
        if (File)
            AFile = File->Duplicate();

        FLastDataSent = nb::Now();
    }
}
//---------------------------------------------------------------------------
void __fastcall TFTPFileSystem::ReadFile(const std::wstring FileName,
                              TRemoteFile *& File)
{
    std::wstring Path = UnixExtractFilePath(FileName);
    std::wstring NameOnly = UnixExtractFileName(FileName);
    TRemoteFile *AFile = NULL;
    if (FServerCapabilities->GetCapability(mlsd_command) == yes)
    {
        DoReadFile(FileName, AFile);
    }
    else
    {
        // FZAPI does not have efficient way to read properties of one file.
        // In case we need properties of set of files from the same directory,
        // cache the file list for future
        if ((FFileListCache != NULL) &&
                UnixComparePaths(Path, FFileListCache->GetDirectory()) &&
                (TTerminal::IsAbsolutePath(FFileListCache->GetDirectory()) ||
                 (FFileListCachePath == GetCurrentDirectory())))
        {
            AFile = FFileListCache->FindFile(NameOnly);
        }

        // if cache is invalid or file is not in cache, (re)read the directory
        if (AFile == NULL)
        {
            delete FFileListCache;
            FFileListCache = NULL;
            FFileListCache = new TRemoteFileList();
            FFileListCache->SetDirectory(Path);
            ReadDirectory(FFileListCache);
            FFileListCachePath = GetCurrentDirectory();

            AFile = FFileListCache->FindFile(NameOnly);
        }
    }
    if (AFile == NULL)
    {
        File = NULL;
        throw ExtException(FMTLOAD(FILE_NOT_EXISTS, FileName.c_str()));
    }
    assert(AFile != NULL);
    File = AFile->Duplicate();
}
//---------------------------------------------------------------------------
void __fastcall TFTPFileSystem::ReadSymlink(TRemoteFile *SymlinkFile,
                                 TRemoteFile *& File)
{
    // Resolving symlinks over FTP is big overhead
    // (involves opening TCPIP connection for retrieving "directory listing").
    // Moreover FZAPI does not support that anyway.
    File = new TRemoteFile(SymlinkFile);
    try
    {
        File->SetTerminal(FTerminal);
        File->SetFileName(UnixExtractFileName(SymlinkFile->GetLinkTo()));
        // FZAPI treats all symlink target as directories
        File->SetType(FILETYPE_DIRECTORY);
    }
    catch (...)
    {
        delete File;
        File = NULL;
        throw;
    }
}
//---------------------------------------------------------------------------
void __fastcall TFTPFileSystem::RenameFile(const std::wstring AFileName,
                                const std::wstring ANewName)
{
    std::wstring FileName = AbsolutePath(AFileName, false);
    std::wstring NewName = AbsolutePath(ANewName, false);

    std::wstring FileNameOnly = UnixExtractFileName(FileName);
    std::wstring FilePathOnly = UnixExtractFilePath(FileName);
    std::wstring NewNameOnly = UnixExtractFileName(NewName);
    std::wstring NewPathOnly = UnixExtractFilePath(NewName);

    {
        // ignore file list
        TFTPFileListHelper Helper(this, NULL, true);

        FFileZillaIntf->Rename(FileNameOnly.c_str(),
                               NewNameOnly.c_str(),
                               FilePathOnly.c_str(),
                               NewPathOnly.c_str());

        GotReply(WaitForCommandReply(), REPLY_2XX_CODE);
    }
}
//---------------------------------------------------------------------------
void __fastcall TFTPFileSystem::CopyFile(const std::wstring FileName,
                              const std::wstring NewName)
{
    assert(false);
}
//---------------------------------------------------------------------------
std::wstring __fastcall TFTPFileSystem::FileUrl(const std::wstring FileName)
{
    return FTerminal->FileUrl(L"ftp", FileName);
}
//---------------------------------------------------------------------------
nb::TStrings * __fastcall TFTPFileSystem::GetFixedPaths()
{
    return NULL;
}
//---------------------------------------------------------------------------
void __fastcall TFTPFileSystem::SpaceAvailable(const std::wstring /*Path*/,
                                    TSpaceAvailable & /*ASpaceAvailable*/)
{
    assert(false);
}
//---------------------------------------------------------------------------
const TSessionInfo & __fastcall TFTPFileSystem::GetSessionInfo()
{
    return FSessionInfo;
}
//---------------------------------------------------------------------------
const TFileSystemInfo & __fastcall TFTPFileSystem::GetFileSystemInfo(bool /*Retrieve*/)
{
    if (!FFileSystemInfoValid)
    {
        FFileSystemInfo.RemoteSystem = FSystem;

        if (FFeatures->GetCount() == 0)
        {
            FFileSystemInfo.AdditionalInfo = LoadStr(FTP_NO_FEATURE_INFO);
        }
        else
        {
            FFileSystemInfo.AdditionalInfo =
                FORMAT(L"%s\r\n", LoadStr(FTP_FEATURE_INFO).c_str());
            for (size_t Index = 0; Index < FFeatures->GetCount(); Index++)
            {
                FFileSystemInfo.AdditionalInfo += FORMAT(L"  %s\r\n", FFeatures->GetString(Index).c_str());
            }
        }

        for (int Index = 0; Index < fcCount; Index++)
        {
            FFileSystemInfo.IsCapable[Index] = IsCapable(static_cast<TFSCapability>(Index));
        }

        FFileSystemInfoValid = true;
    }
    return FFileSystemInfo;
}
//---------------------------------------------------------------------------
bool __fastcall TFTPFileSystem::TemporaryTransferFile(const std::wstring /*FileName*/)
{
    return false;
}
//---------------------------------------------------------------------------
bool __fastcall TFTPFileSystem::GetStoredCredentialsTried()
{
    return !FTerminal->GetSessionData()->GetPassword().empty();
}
//---------------------------------------------------------------------------
std::wstring __fastcall TFTPFileSystem::GetUserName()
{
    return FUserName;
}
//---------------------------------------------------------------------------
std::wstring __fastcall TFTPFileSystem::GetCurrentDirectory()
{
    return FCurrentDirectory;
}
//---------------------------------------------------------------------------
const wchar_t * __fastcall TFTPFileSystem::GetOption(int OptionID) const
{
    TSessionData *Data = FTerminal->GetSessionData();

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
    case OPTION_TRANSFERIP:
        FOptionScratch = FTerminal->GetConfiguration()->GetExternalIpAddress();
        break;

    case OPTION_ANONPWD:
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
int __fastcall TFTPFileSystem::GetOptionVal(int OptionID) const
{
    TSessionData *Data = FTerminal->GetSessionData();
    int Result;

    switch (OptionID)
    {
    case OPTION_PROXYTYPE:
        switch (Data->GetProxyMethod())
        {
        case ::pmNone:
            Result = PROXYTYPE_NOPROXY;
            break;

        case pmSocks4:
            Result = PROXYTYPE_SOCKS4A;
            break;

        case pmSocks5:
            Result = PROXYTYPE_SOCKS5;
            break;

        case pmHTTP:
            Result = PROXYTYPE_HTTP11;
            break;

        case pmTelnet:
        case pmCmd:
        default:
            assert(false);
            Result = PROXYTYPE_NOPROXY;
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
        Result = static_cast<int>((FFileTransferCPSLimit / 1024)); // FZAPI expects KiB/s
        break;

    case OPTION_MPEXT_SHOWHIDDEN:
        Result = (FDoListAll ? TRUE : FALSE);
        break;

    case OPTION_MPEXT_SNDBUF:
      Result = Data->GetSendBuf();
      break;

    case OPTION_MPEXT_SSLSESSIONREUSE:
        Result = (Data->GetSslSessionReuse() ? TRUE : FALSE);
        break;

    default:
        assert(false);
        Result = FALSE;
        break;
    }

    return Result;
}
//---------------------------------------------------------------------------
bool __fastcall TFTPFileSystem::PostMessage(unsigned int Type, WPARAM wParam, LPARAM lParam)
{
    if (Type == TFileZillaIntf::MSG_TRANSFERSTATUS)
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
bool __fastcall TFTPFileSystem::ProcessMessage()
{
    bool Result;
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
        FFileZillaIntf->HandleMessage(Message.first, Message.second);
    }

    return Result;
}
//---------------------------------------------------------------------------
void __fastcall TFTPFileSystem::DiscardMessages()
{
    while (ProcessMessage());
}
//---------------------------------------------------------------------------
void __fastcall TFTPFileSystem::WaitForMessages()
{
    unsigned int Result = WaitForSingleObject(FQueueEvent, INFINITE);
    if (Result != WAIT_OBJECT_0)
    {
        FTerminal->FatalError(NULL, FMTLOAD(INTERNAL_ERROR, L"ftp#1", IntToStr(Result).c_str()));
    }
}
//---------------------------------------------------------------------------
void __fastcall TFTPFileSystem::PoolForFatalNonCommandReply()
{
    assert(FReply == 0);
    assert(FCommandReply == 0);
    assert(!FWaitingForReply);

    FWaitingForReply = true;

    unsigned int Reply = 0;

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
bool __fastcall TFTPFileSystem::NoFinalLastCode()
{
    return (FLastCodeClass == 0) || (FLastCodeClass == 1);
}
//---------------------------------------------------------------------------
bool __fastcall TFTPFileSystem::KeepWaitingForReply(unsigned int &ReplyToAwait, bool WantLastCode)
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
void __fastcall TFTPFileSystem::DoWaitForReply(unsigned int &ReplyToAwait, bool WantLastCode)
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
unsigned int __fastcall TFTPFileSystem::WaitForReply(bool Command, bool WantLastCode)
{
    assert(FReply == 0);
    assert(FCommandReply == 0);
    assert(!FWaitingForReply);
    assert(!FTransferStatusCriticalSection->GetAcquired());

    ResetReply();
    FWaitingForReply = true;

    unsigned int Reply = 0;

    {
        BOOST_SCOPE_EXIT ( (&Self) )
        {
            Self->FReply = 0;
            Self->FCommandReply = 0;
            assert(Self->FWaitingForReply);
            Self->FWaitingForReply = false;
        } BOOST_SCOPE_EXIT_END
        unsigned int &ReplyToAwait = (Command ? FCommandReply : FReply);
        DoWaitForReply(ReplyToAwait, WantLastCode);

        Reply = ReplyToAwait;
    }

    return Reply;
}
//---------------------------------------------------------------------------
unsigned int __fastcall TFTPFileSystem::WaitForCommandReply(bool WantLastCode)
{
    return WaitForReply(true, WantLastCode);
}
//---------------------------------------------------------------------------
void __fastcall TFTPFileSystem::WaitForFatalNonCommandReply()
{
    WaitForReply(false, false);
    assert(false);
}
//---------------------------------------------------------------------------
void __fastcall TFTPFileSystem::ResetReply()
{
    FLastCode = 0;
    FLastCodeClass = 0;
    assert(FLastResponse != NULL);
    FLastResponse->Clear();
    assert(FLastError != NULL);
    FLastError->Clear();
}
//---------------------------------------------------------------------------
void __fastcall TFTPFileSystem::GotNonCommandReply(unsigned int Reply)
{
    assert(FLAGSET(Reply, TFileZillaIntf::REPLY_DISCONNECTED));
    GotReply(Reply);
    // should never get here as GotReply should raise fatal exception
    assert(false);
}
//---------------------------------------------------------------------------
void __fastcall TFTPFileSystem::GotReply(unsigned int Reply, unsigned int Flags,
                              std::wstring Error, unsigned int *Code, nb::TStrings **Response)
{
    {
        BOOST_SCOPE_EXIT ( (&Self) )
        {
            Self->ResetReply();
        } BOOST_SCOPE_EXIT_END
        if (FLAGSET(Reply, TFileZillaIntf::REPLY_OK))
        {
            assert(Reply == TFileZillaIntf::REPLY_OK);

            // With REPLY_2XX_CODE treat "OK" non-2xx code like an error
            if (FLAGSET(Flags, REPLY_2XX_CODE) && (FLastCodeClass != 2))
            {
                GotReply(TFileZillaIntf::REPLY_ERROR, Flags, Error);
            }
        }
        else if (FLAGSET(Reply, TFileZillaIntf::REPLY_CANCEL) &&
                 FLAGSET(Flags, REPLY_ALLOW_CANCEL))
        {
            assert(
                (Reply == (TFileZillaIntf::REPLY_CANCEL | TFileZillaIntf::REPLY_ERROR)) ||
                (Reply == (TFileZillaIntf::REPLY_ABORTED | TFileZillaIntf::REPLY_CANCEL | TFileZillaIntf::REPLY_ERROR)));
            // noop
        }
        // we do not expect these with our usage of FZ
        else if (Reply &
                 (TFileZillaIntf::REPLY_WOULDBLOCK | TFileZillaIntf::REPLY_OWNERNOTSET |
                  TFileZillaIntf::REPLY_INVALIDPARAM | TFileZillaIntf::REPLY_ALREADYCONNECTED |
                  TFileZillaIntf::REPLY_IDLE | TFileZillaIntf::REPLY_NOTINITIALIZED |
                  TFileZillaIntf::REPLY_ALREADYINIZIALIZED))
        {
            FTerminal->FatalError(NULL, FMTLOAD(INTERNAL_ERROR, L"ftp#2", FORMAT(L"0x%x", static_cast<int>(Reply)).c_str()));
        }
        else
        {
            // everything else must be an error or disconnect notification
            assert(
                FLAGSET(Reply, TFileZillaIntf::REPLY_ERROR) ||
                FLAGSET(Reply, TFileZillaIntf::REPLY_DISCONNECTED));

            // TODO: REPLY_CRITICALERROR ignored

            // REPLY_NOTCONNECTED happens if connection is closed between moment
            // when FZAPI interface method dispatches the command to FZAPI thread
            // and moment when FZAPI thread receives the command
            bool Disconnected =
                FLAGSET(Reply, TFileZillaIntf::REPLY_DISCONNECTED) ||
                FLAGSET(Reply, TFileZillaIntf::REPLY_NOTCONNECTED);

            std::wstring HelpKeyword;
            nb::TStrings *MoreMessages = new nb::TStringList();
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

                if (FLAGSET(Reply, TFileZillaIntf::REPLY_ABORTED))
                {
                    MoreMessages->Add(LoadStr(USER_TERMINATED));
                }

                if (FLAGSET(Reply, TFileZillaIntf::REPLY_NOTSUPPORTED))
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
                        HelpKeyword = HELP_FTP_CANNOT_OPEN_ACTIVE_CONNECTION;
                    }
                }
                if (FLastCode == 421)
                {
                    Disconnected = true;
                }
                if (FLastCode == DummyTimeoutCode)
                {
                    HelpKeyword = HELP_ERRORMSG_TIMEOUT;
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
                    FTerminal->FatalError(E, Error);
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
            FLastResponse = new nb::TStringList();
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TFTPFileSystem::SetLastCode(int Code)
{
    FLastCode = Code;
    FLastCodeClass = (Code / 100);
}
//---------------------------------------------------------------------------
void __fastcall TFTPFileSystem::HandleReplyStatus(const std::wstring Response)
{
    int Code = 0;

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
        size_t Start = 0;
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
                if ((FListAll == asAuto) &&
                        (::Pos(FSystem, L"Personal FTP Server") != std::wstring::npos))
                {
                    FTerminal->LogEvent(L"Server is known not to support LIST -a");
                    FListAll = asOff;
                }
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
                FFeatures->Assign(FLastResponse);
            }
            else
            {
                FFeatures->Clear();
            }
        }
    }
}
//---------------------------------------------------------------------------
std::wstring __fastcall TFTPFileSystem::ExtractStatusMessage(std::wstring &Status)
{
    // CApiLog::LogMessage
    // (note that the formatting may not be present when LogMessageRaw is used)
    size_t P1 = ::Pos(Status, L"): ");
    if (P1 != std::wstring::npos)
    {
        size_t P2 = ::Pos(Status, L".cpp(");
        if ((P2 != std::wstring::npos) && (P2 < P1))
        {
            size_t P3 = ::Pos(Status, L"   caller=0x");
            if ((P3 != std::wstring::npos) && (P3 > P1))
            {
                Status = Status.substr(P1 + 3, P3 - P1 - 3);
            }
        }
    }
    return Status;
}
//---------------------------------------------------------------------------
bool __fastcall TFTPFileSystem::HandleStatus(const wchar_t *AStatus, int Type)
{
    TLogLineType LogType = static_cast<TLogLineType>(-1);
    std::wstring Status(AStatus);

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

    case TFileZillaIntf::LOG_DEBUG:
        // used for directory listing only
        LogType = llMessage;
        break;

    default:
        assert(false);
        break;
    }

    if (FTerminal->GetLog()->GetLogging() && (LogType != static_cast<TLogLineType>(-1)))
    {
        FTerminal->GetLog()->Add(LogType, Status);
    }

    return true;
}
//---------------------------------------------------------------------------
nb::TDateTime __fastcall TFTPFileSystem::ConvertLocalTimestamp(time_t Time)
{
    // This reverses how FZAPI converts FILETIME to time_t,
    // before passing it to FZ_ASYNCREQUEST_OVERWRITE.
    __int64 Timestamp;
    tm *Tm = localtime(&Time);
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
nb::TDateTime __fastcall TFTPFileSystem::ConvertRemoteTimestamp(time_t Time, bool HasTime)
{
    nb::TDateTime Result;
    tm *Tm = localtime(&Time);
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
bool __fastcall TFTPFileSystem::HandleAsynchRequestOverwrite(
    wchar_t *FileName1, size_t FileName1Len, const wchar_t * /*FileName2*/,
    const wchar_t * /*Path1*/, const wchar_t * /*Path2*/,
    __int64 Size1, __int64 Size2, time_t Time1, time_t Time2,
    bool HasTime1, bool HasTime2, void *AUserData, int &RequestResult)
{
    if (!FActive)
    {
        return false;
    }
    else
    {
        TFileTransferData &UserData = *(static_cast<TFileTransferData *>(AUserData));
        if (UserData.OverwriteResult >= 0)
        {
            // on retry, use the same answer as on the first attempt
            RequestResult = UserData.OverwriteResult;
        }
        else
        {
            TFileOperationProgressType *OperationProgress = FTerminal->GetOperationProgress();
            std::wstring FileName = FileName1;
            assert(!wcscmp(UserData.FileName.c_str(), FileName.c_str()));
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

            if (ConfirmOverwrite(FileName, UserData.Params, OperationProgress,
                                 OverwriteMode, 
                                 UserData.AutoResume && UserData.CopyParam->AllowResume(FileParams.SourceSize),
                                 (NoFileParams ? NULL : &FileParams)))
            {
                switch (OverwriteMode)
                {
                case omOverwrite:
                    if (FileName != FileName1)
                    {
                        wcsncpy(FileName1, FileName.c_str(), FileName1Len);
                        FileName1[FileName1Len - 1] = L'\0';
                        UserData.FileName = FileName1;
                        RequestResult = TFileZillaIntf::FILEEXISTS_RENAME;
                    }
                    else
                    {
                        RequestResult = TFileZillaIntf::FILEEXISTS_OVERWRITE;
                    }
                    break;

                case omResume:
                    RequestResult = TFileZillaIntf::FILEEXISTS_RESUME;
                    break;

                default:
                    assert(false);
                    RequestResult = TFileZillaIntf::FILEEXISTS_OVERWRITE;
                    break;
                }
            }
            else
            {
                RequestResult = TFileZillaIntf::FILEEXISTS_SKIP;
            }
        }

        // remember the answer for the retries
        UserData.OverwriteResult = RequestResult;

        if (RequestResult == TFileZillaIntf::FILEEXISTS_SKIP)
        {
            // when user chooses not to overwrite, break loop waiting for response code
            // by setting dummy one, az FZAPI won't do anything then
            SetLastCode(DummyTimeoutCode);
        }

        return true;
    }
}
//---------------------------------------------------------------------------
std::wstring __fastcall FormatContactList(const std::wstring Entry1, const std::wstring Entry2)
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
std::wstring __fastcall FormatContact(const TFtpsCertificateData::TContact &Contact)
{
    std::wstring Result =
        FORMAT(::LoadStrPart(VERIFY_CERT_CONTACT, 1).c_str(),
               FormatContactList(FormatContactList(FormatContactList(
                                     Contact.Organization,
                                     Contact.Unit).c_str(),
                                 Contact.CommonName).c_str(),
                                 Contact.Mail).c_str());

    if ((Contact.Country && *Contact.Country) ||
            (Contact.StateProvince && *Contact.StateProvince) ||
            (Contact.Town && *Contact.Town))
    {
        Result +=
            FORMAT(::LoadStrPart(VERIFY_CERT_CONTACT, 2).c_str(),
                   FormatContactList(FormatContactList(
                                         Contact.Country,
                                         Contact.StateProvince).c_str(),
                                     Contact.Town).c_str());
    }

    if (Contact.Other && *Contact.Other)
    {
        Result += FORMAT(::LoadStrPart(VERIFY_CERT_CONTACT, 3).c_str(), Contact.Other);
    }

    return Result;
}
//---------------------------------------------------------------------------
std::wstring __fastcall FormatValidityTime(const TFtpsCertificateData::TValidityTime &ValidityTime)
{
    return FormatDateTime(L"ddddd tt",
                          EncodeDateVerbose(
                              static_cast<unsigned short>(ValidityTime.Year), static_cast<unsigned short>(ValidityTime.Month),
                              static_cast<unsigned short>(ValidityTime.Day)) +
                          EncodeTimeVerbose(
                              static_cast<unsigned short>(ValidityTime.Hour), static_cast<unsigned short>(ValidityTime.Min),
                              static_cast<unsigned short>(ValidityTime.Sec), 0));
}
//---------------------------------------------------------------------------
bool __fastcall TFTPFileSystem::HandleAsynchRequestVerifyCertificate(
    const TFtpsCertificateData &Data, int &RequestResult)
{
    if (!FActive)
    {
        return false;
    }
    else
    {
        std::string str(reinterpret_cast<const char *>(Data.Hash), Data.HashLen);
        FSessionInfo.CertificateFingerprint =
            StrToHex(nb::MB2W(str.c_str()), false, 'L:');

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

        THierarchicalStorage *Storage =
            FTerminal->GetConfiguration()->CreateStorage();
        {
            BOOST_SCOPE_EXIT ( (&Storage) )
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
                std::wstring ExpectedKey = CutToChar(Buf, L';', false);
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
                THierarchicalStorage *Storage =
                    FTerminal->GetConfiguration()->CreateStorage();
                {
                    BOOST_SCOPE_EXIT ( (&Storage) )
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
//---------------------------------------------------------------------------
bool __fastcall TFTPFileSystem::HandleListData(const wchar_t *Path,
                                    const TListDataEntry *Entries, size_t Count)
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

        for (size_t Index = 0; Index < Count; Index++)
        {
            const TListDataEntry *Entry = &Entries[Index];
            TRemoteFile *File = new TRemoteFile();
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
                std::wstring own = Entry->OwnerGroup;
                const wchar_t *Space = wcschr(own.c_str(), L' ');
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
                    File->SetType(L'-');
                }

                // ModificationFmt must be set after Modification
                if (Entry->HasDate)
                {
                    // should be the same as ConvertRemoteTimestamp
                    nb::TDateTime Modification =
                        EncodeDateVerbose(static_cast<unsigned short>(Entry->Year), static_cast<unsigned short>(Entry->Month),
                                          static_cast<unsigned short>(Entry->Day));
                    if (Entry->HasTime)
                    {
                        File->SetModification(Modification +
                                              EncodeTimeVerbose(static_cast<unsigned short>(Entry->Hour), static_cast<unsigned short>(Entry->Minute), 0, 0));
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

                    File->SetModification(nb::TDateTime(0.0));
                    File->SetModificationFmt(mfNone);
                }
                File->SetLastAccess(File->GetModification());

                File->SetLinkTo(Entry->LinkTarget);

                File->Complete();
            }
            catch (const std::exception &E)
            {
                delete File;
                std::wstring EntryData =
                    FORMAT(L"%s/%s/%s/%s/%d/%d/%d/%d/%d/%d/%d/%d/%d",
                           Entry->Name,
                           Entry->Permissions,
                           Entry->OwnerGroup,
                           Int64ToStr(Entry->Size).c_str(),
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
bool __fastcall TFTPFileSystem::HandleTransferStatus(bool Valid, __int64 TransferSize,
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
bool __fastcall TFTPFileSystem::HandleReply(int Command, unsigned int Reply)
{
    if (!FActive)
    {
        return false;
    }
    else
    {
        if (FTerminal->GetConfiguration()->GetActualLogProtocol() >= 1)
        {
            FTerminal->LogEvent(FORMAT(L"Got reply %x to the command %d", static_cast<int>(Reply), Command));
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
bool __fastcall TFTPFileSystem::HandleCapabilities(TFTPServerCapabilities *ServerCapabilities)
{
    FServerCapabilities->Assign(ServerCapabilities);
    FFileSystemInfoValid = false;
    return true;
}
//---------------------------------------------------------------------------
bool __fastcall TFTPFileSystem::CheckError(int ReturnCode, const wchar_t *Context)
{
    // we do not expect any FZAPI call to fail as it generally can fail only due to:
    // - invalid paramerers
    // - busy FZAPI core
    // the only exception is REPLY_NOTCONNECTED that can happen if
    // connection is closed just between the last call to Idle()
    // and call to any FZAPI command
    // in such case reply without associated command is posted,
    // which we are going to wait for unless we are already waiting
    // on higher level (this typically happens if connection is lost while
    // waiting for user interaction and is detected within call to
    // SetAsyncRequestResult)
    if (FLAGSET(ReturnCode, TFileZillaIntf::REPLY_NOTCONNECTED))
    {
        if (!FWaitingForReply)
        {
            // throws
            WaitForFatalNonCommandReply();
        }
    }
    else
    {
        FTerminal->FatalError(NULL,
                              FMTLOAD(INTERNAL_ERROR, FORMAT(L"fz#%s", Context).c_str(), IntToHex(ReturnCode, 4).c_str()));
        assert(false);
    }

    return false;
}
//---------------------------------------------------------------------------
bool __fastcall TFTPFileSystem::Unquote(std::wstring &Str)
{
    enum
    {
        INIT,
        QUOTE,
        QUOTED,
        DONE
    } State;

    State = INIT;
    assert((Str.size() > 0) && ((Str[0] == '"') || (Str[0] == '\'')));

    size_t Index = 0;
    wchar_t Quote = 0;
    while (Index < Str.size())
    {
        switch (State)
        {
        case INIT:
            if ((Str[Index] == '"') || (Str[Index] == '\''))
            {
                Quote = Str[Index];
                State = QUOTED;
                Str.erase(Index, 1);
            }
            else
            {
                assert(false);
                // no quoted string
                Str.resize(0);
            }
            break;

        case QUOTED:
            if (Str[Index] == Quote)
            {
                State = QUOTE;
                Str.erase(Index, 1);
            }
            else
            {
                Index++;
            }
            break;

        case QUOTE:
            if (Str[Index] == Quote)
            {
                Index++;
            }
            else
            {
                // end of quoted string, trim the rest
                Str.resize(Index);
                State = DONE;
            }
            break;
        }
    }

    return (State == DONE);
}
//---------------------------------------------------------------------------
#endif NO_FILEZILLA
