﻿
#include <vcl.h>
#pragma hdrstop


#include <rdestl/list.h>
#ifndef MPEXT
#define MPEXT
#endif
#include "FtpFileSystem.h"
#include "FileZillaIntf.h"

#include <Common.h>
#include <nbutils.h>
#include <Exceptions.h>
#include "Terminal.h"
#include "TextsCore.h"
#include "TextsFileZilla.h"
#include "HelpCore.h"
#include "WinSCPSecurity.h"
#include "NeonIntf.h"
#include <StrUtils.hpp>
#include <DateUtils.hpp>
#include <SessionData.h>
#include <openssl/x509_vfy.h>
#include <limits>

// #pragma package(smart_init)

#undef FILE_OPERATION_LOOP_TERMINAL
#define FILE_OPERATION_LOOP_TERMINAL FTerminal

constexpr const int32_t DummyCodeClass = 8;
constexpr const int32_t DummyTimeoutCode = 801;
constexpr const int32_t DummyCancelCode = 802;
constexpr const int32_t DummyDisconnectCode = 803;

class TFileZillaImpl final : public TFileZillaIntf
{
public:
  TFileZillaImpl() = delete;
  explicit TFileZillaImpl(gsl::not_null<TFTPFileSystem *> FileSystem) noexcept;
  virtual ~TFileZillaImpl() = default;

  virtual const wchar_t * Option(int32_t OptionID) const override;
  virtual int32_t OptionVal(int32_t OptionID) const override;

protected:
  virtual bool DoPostMessage(TMessageType Type, WPARAM wParam, LPARAM lParam) override;

  virtual bool HandleStatus(const wchar_t * Status, int32_t Type) override;
  virtual bool HandleAsyncRequestOverwrite(
    wchar_t * FileName1, size_t FileName1Len, const wchar_t * FileName2,
    const wchar_t * Path1, const wchar_t * Path2,
    int64_t Size1, int64_t Size2, time_t LocalTime,
    bool HasLocalTime, const TRemoteFileTime & RemoteTime, void * UserData,
    HANDLE & LocalFileHandle, int32_t & RequestResult) override;
  virtual bool HandleAsyncRequestVerifyCertificate(
    const TFtpsCertificateData & Data, int32_t & RequestResult) override;
  virtual bool HandleAsyncRequestNeedPass(
    struct TNeedPassRequestData & Data, int32_t & RequestResult) override;
  virtual bool HandleListData(const wchar_t * Path, const TListDataEntry * Entries,
    uint32_t Count) override;
  virtual bool HandleTransferStatus(bool Valid, int64_t TransferSize,
    int64_t Bytes, bool FileTransfer) override;
  virtual bool HandleReply(int32_t Command, int64_t Reply) override;
  virtual bool HandleCapabilities(TFTPServerCapabilities * ServerCapabilities) override;
  virtual bool CheckError(int32_t ReturnCode, const wchar_t * Context) override;

  virtual void PreserveDownloadFileTime(HANDLE AHandle, void * UserData) override;
  virtual bool GetFileModificationTimeInUtc(const wchar_t * FileName, struct tm & Time) override;
  virtual wchar_t * LastSysErrorMessage() const override;
  virtual std::wstring GetClientString() const override;
  virtual void SetupSsl(ssl_st * Ssl);

private:
  gsl::not_null<TFTPFileSystem *> FFileSystem;
};

TFileZillaImpl::TFileZillaImpl(gsl::not_null<TFTPFileSystem *> FileSystem) noexcept :
  TFileZillaIntf(),
  FFileSystem(FileSystem)
{
}

const wchar_t * TFileZillaImpl::Option(int32_t OptionID) const
{
  return FFileSystem->GetOption(OptionID);
}

int32_t TFileZillaImpl::OptionVal(int32_t OptionID) const
{
  return FFileSystem->GetOptionVal(OptionID);
}

bool TFileZillaImpl::DoPostMessage(TMessageType Type, WPARAM wParam, LPARAM lParam)
{
  return FFileSystem->FTPPostMessage(Type, wParam, lParam);
}

bool TFileZillaImpl::HandleStatus(const wchar_t * Status, int32_t Type)
{
  return FFileSystem->HandleStatus(Status, Type);
}

bool TFileZillaImpl::HandleAsyncRequestOverwrite(
  wchar_t * FileName1, size_t FileName1Len, const wchar_t * FileName2,
  const wchar_t * Path1, const wchar_t * Path2,
  int64_t Size1, int64_t Size2, time_t LocalTime,
  bool HasLocalTime, const TRemoteFileTime & RemoteTime, void * UserData,
  HANDLE & LocalFileHandle,
  int32_t & RequestResult)
{
  return FFileSystem->HandleAsyncRequestOverwrite(
    FileName1, FileName1Len, FileName2, Path1, Path2, Size1, Size2, LocalTime,
    HasLocalTime, RemoteTime, UserData, LocalFileHandle, RequestResult);
}

bool TFileZillaImpl::HandleAsyncRequestVerifyCertificate(
  const TFtpsCertificateData & Data, int32_t & RequestResult)
{
  return FFileSystem->HandleAsyncRequestVerifyCertificate(Data, RequestResult);
}

bool TFileZillaImpl::HandleAsyncRequestNeedPass(
  struct TNeedPassRequestData & Data, int32_t & RequestResult)
{
  return FFileSystem->HandleAsyncRequestNeedPass(Data, RequestResult);
}

bool TFileZillaImpl::HandleListData(const wchar_t * Path,
  const TListDataEntry * Entries, uint32_t Count)
{
  return FFileSystem->HandleListData(Path, Entries, Count);
}

bool TFileZillaImpl::HandleTransferStatus(bool Valid, int64_t TransferSize,
  int64_t Bytes, bool FileTransfer)
{
  return FFileSystem->HandleTransferStatus(Valid, TransferSize, Bytes, FileTransfer);
}

bool TFileZillaImpl::HandleReply(int32_t Command, int64_t Reply)
{
  return FFileSystem->HandleReply(Command, Reply);
}

bool TFileZillaImpl::HandleCapabilities(TFTPServerCapabilities * ServerCapabilities)
{
  return FFileSystem->HandleCapabilities(ServerCapabilities);
}

bool TFileZillaImpl::CheckError(int32_t ReturnCode, const wchar_t * Context)
{
  return FFileSystem->CheckError(ReturnCode, Context);
}

void TFileZillaImpl::PreserveDownloadFileTime(HANDLE AHandle, void * UserData)
{
  return FFileSystem->PreserveDownloadFileTime(AHandle, UserData);
}

bool TFileZillaImpl::GetFileModificationTimeInUtc(const wchar_t * FileName, struct tm & Time)
{
  return FFileSystem->GetFileModificationTimeInUtc(FileName, Time);
}

wchar_t * TFileZillaImpl::LastSysErrorMessage() const
{
  return nbcore_wstrdup(::LastSysErrorMessage().c_str());
}

std::wstring TFileZillaImpl::GetClientString() const
{
  return std::wstring(GetSshVersionString().c_str());
}

void TFileZillaImpl::SetupSsl(ssl_st * Ssl)
{
  ::SetupSsl(Ssl, FFileSystem->FTerminal->SessionData->FMinTlsVersion, FFileSystem->FTerminal->SessionData->FMaxTlsVersion);
}

struct message_t
{
  CUSTOM_MEM_ALLOCATION_IMPL

  message_t() noexcept : wparam(0), lparam(0) {}
  message_t(WPARAM w, LPARAM l) noexcept : wparam(w), lparam(l) {}

  WPARAM wparam{0};
  LPARAM lparam{0};
};


class TMessageQueue final : public TObject, public nb::list_t<message_t>
{
public:
  using value_type = message_t;
};


#if 0
struct TFileTransferData
{
  TFileTransferData()
  {
    Params = 0;
    AutoResume = false;
    OverwriteResult = -1;
    CopyParam = nullptr;
  }

  UnicodeString FileName;
  int32_t Params;
  bool AutoResume;
  int32_t OverwriteResult;
  const TCopyParamType * CopyParam;
  TDateTime Modification;
};
#endif // #if 0

constexpr const wchar_t * SiteCommand = L"SITE";
constexpr const wchar_t * SymlinkSiteCommand = L"SYMLINK";
constexpr const wchar_t * CopySiteCommand = L"COPY";
constexpr const wchar_t * HashCommand = L"HASH"; // Cerberos + FileZilla servers
constexpr const wchar_t * AvblCommand = L"AVBL";
constexpr const wchar_t * XQuotaCommand = L"XQUOTA";
constexpr const wchar_t * MdtmCommand = L"MDTM";
constexpr const wchar_t * SizeCommand = L"SIZE";
constexpr const wchar_t * CsidCommand = L"CSID";
constexpr const wchar_t * DirectoryHasBytesPrefix = L"226-Directory has";

class TFTPFileListHelper final : public TObject
{
  NB_DISABLE_COPY(TFTPFileListHelper)
public:
  explicit TFTPFileListHelper(TFTPFileSystem * FileSystem, TRemoteFileList * FileList,
      bool IgnoreFileList) noexcept :
    FFileSystem(FileSystem),
    FFileList(FFileSystem->FFileList),
    FIgnoreFileList(FFileSystem->FIgnoreFileList)
  {
    FFileSystem->FFileList = FileList;
    FFileSystem->FIgnoreFileList = IgnoreFileList;
  }

  virtual ~TFTPFileListHelper() noexcept override
  {
    FFileSystem->FFileList = FFileList;
    FFileSystem->FIgnoreFileList = FIgnoreFileList;
  }

private:
  TFTPFileSystem * FFileSystem{nullptr};
  TRemoteFileList * FFileList{nullptr};
  bool FIgnoreFileList{false};
};

TFTPFileSystem::TFTPFileSystem(TTerminal * ATerminal) noexcept :
  TCustomFileSystem(OBJECT_CLASS_TFTPFileSystem, ATerminal),
  FFileZillaIntf(nullptr),
  FQueue(std::make_unique<TMessageQueue>()),
  FQueueEvent(::CreateEvent(nullptr, true, false, nullptr)),
  FFileSystemInfoValid(false),
  FReply(0),
  FCommandReply(0),
  FLastCommand(CMD_UNKNOWN),
  FPasswordFailed(false),
  FStoredPasswordTried(false),
  FMultiLineResponse(false),
  FLastCode(0),
  FLastCodeClass(0),
  FLastReadDirectoryProgress(0),
  FLastResponse(std::make_unique<TStringList>()),
  FLastErrorResponse(std::make_unique<TStringList>()),
  FLastError(std::make_unique<TStringList>()),
  FFeatures(std::make_unique<TStringList>()),
  FReadCurrentDirectory(false),
  FFileList(nullptr),
  FFileListCache(nullptr),
  FActive(false),
  FOpening(false),
  FWaitingForReply(false),
  FFileTransferAbort(ftaNone),
  FIgnoreFileList(false),
  FFileTransferCancelled(false),
  FFileTransferResumed(0),
  FFileTransferPreserveTime(false),
  FFileTransferRemoveBOM(false),
  FFileTransferNoList(false),
  FFileTransferCPSLimit(0),
  FAwaitingProgress(false),
  FOnCaptureOutput(nullptr),
  FListAll(asOn),
  FDoListAll(false),
  FServerCapabilities(std::make_unique<TFTPServerCapabilities>()),
  FDetectTimeDifference(false),
  FTimeDifference(0),
  FSupportsAnyChecksumFeature(false),
  FCertificate(nullptr),
  FPrivateKey(nullptr),
  FTransferActiveImmediately(false),
  FWindowsServer(false),
  FBytesAvailable(0),
  FBytesAvailableSupported(false),
  FMVS(false),
  FVMS(false),
  FFileTransferAny(false)
{
}

void TFTPFileSystem::Init(void *)
{
  //FQueue.reserve(1000);
  ResetReply();

  FListAll = FTerminal->GetSessionData()->GetFtpListAll();
  FWorkFromCwd = FTerminal->SessionData->FFtpWorkFromCwd;
  FFileSystemInfo.ProtocolBaseName = "FTP";
  FFileSystemInfo.ProtocolName = FFileSystemInfo.ProtocolBaseName;
  FTimeoutStatus = LoadStr(IDS_ERRORMSG_TIMEOUT);
  FDisconnectStatus = LoadStr(IDS_STATUSMSG_DISCONNECTED);
  FServerCapabilities = std::make_unique<TFTPServerCapabilities>();
  FHashAlgs = std::make_unique<TStringList>();
  FSupportedCommands.reset(CreateSortedStringList());
  FSupportedSiteCommands.reset(CreateSortedStringList());
  FCertificate = nullptr;
  FPrivateKey = nullptr;
  FBytesAvailable = -1;
  FBytesAvailableSupported = false;
  FLoggedIn = false;
  FAnyTransferSucceeded = false; // Do not reset on reconnect
  FForceReadSymlink = false;

  FChecksumAlgs = std::make_unique<TStringList>();
  FChecksumCommands = std::make_unique<TStringList>();
  RegisterChecksumAlgCommand(Sha1ChecksumAlg, "XSHA1"); // e.g. Cerberos FTP
  RegisterChecksumAlgCommand(Sha256ChecksumAlg, "XSHA256"); // e.g. Cerberos FTP
  RegisterChecksumAlgCommand(Sha512ChecksumAlg, "XSHA512"); // e.g. Cerberos FTP
  RegisterChecksumAlgCommand(Md5ChecksumAlg, "XMD5"); // e.g. Cerberos FTP
  RegisterChecksumAlgCommand(Md5ChecksumAlg, "MD5"); // e.g. Apache FTP
  RegisterChecksumAlgCommand(Crc32ChecksumAlg, "XCRC"); // e.g. Cerberos FTP
}

TFTPFileSystem::~TFTPFileSystem() noexcept
{
  DebugAssert(FFileList == nullptr);

  if (FFileZillaIntf)
  {
    FFileZillaIntf->Destroying();

    // to release memory associated with the messages
    DiscardMessages();
  }

  //SAFE_DESTROY_EX(CFileZillaTools, FFileZillaIntf);
  FFileZillaIntf.reset();

#if 0
  delete FFileZillaIntf;
  FFileZillaIntf = nullptr;

  delete FQueue;
  FQueue = nullptr;
#endif // #if 0

  SAFE_CLOSE_HANDLE(FQueueEvent);

#if 0
  delete FQueueCriticalSection;
  FQueueCriticalSection = nullptr;
  delete FTransferStatusCriticalSection;
  FTransferStatusCriticalSection = nullptr;

  delete FLastResponse;
  FLastResponse = nullptr;
  delete FLastErrorResponse;
  FLastErrorResponse = nullptr;
  delete FLastError;
  FLastError = nullptr;
  delete FFeatures;
  FFeatures = nullptr;
  delete FServerCapabilities;
  FServerCapabilities = nullptr;
#endif // #if 0

  ResetCaches();
}

void TFTPFileSystem::Open()
{
  // on reconnect, typically there may be pending status messages from previous session
  DiscardMessages();

  ResetCaches();
  FReadCurrentDirectory = true;
  FCurrentDirectory.Clear();
  FHomeDirectory.Clear();
  FLoggedIn = false;

  FLastDataSent = Now();

  FMultiLineResponse = false;

  // initialize FZAPI on the first connect only
  if (FFileZillaIntf == nullptr)
  {
    std::unique_ptr<TFileZillaIntf> FileZillaImpl(std::make_unique<TFileZillaImpl>(this));

    try__catch
    {
      TFileZillaIntf::TLogLevel LogLevel;
      switch (FTerminal->GetConfiguration()->GetActualLogProtocol())
      {
        default:
        case -1:
          LogLevel = TFileZillaIntf::LOG_WARNING;
          break;

        case 0:
        case 1:
          LogLevel = TFileZillaIntf::LOG_PROGRESS;
          break;

        case 2:
          LogLevel = TFileZillaIntf::LOG_INFO;
          break;
      }
      FileZillaImpl->SetDebugLevel(LogLevel);

      FileZillaImpl->Init();
      FFileZillaIntf = std::move(FileZillaImpl);
    }
    __catch__removed
    {
      // delete FFileZillaIntf;
      // FFileZillaIntf = nullptr;
      // throw;
    } end_try__catch
  }

  TSessionData * Data = FTerminal->GetSessionData();

  FWindowsServer = false;
  FMVS = false;
  FVMS = false;
  FFileZilla = false;
  FIIS = false;
  FTransferActiveImmediately = (Data->GetFtpTransferActiveImmediately() == asOn);
  FVMSAllRevisions = Data->VMSAllRevisions;

  FSessionInfo.LoginTime = Now();
  FSessionInfo.CertificateVerifiedManually = false;

  const UnicodeString HostName = Data->GetHostNameExpanded();
  UnicodeString UserName = Data->GetUserNameExpanded();
  UnicodeString Password = Data->GetPassword();
  const UnicodeString Account = Data->GetFtpAccount();
  const UnicodeString Path = Data->GetRemoteDirectory();
  int32_t ServerType = 0;
  switch (Data->GetFtps())
  {
    case ftpsNone:
      ServerType = TFileZillaIntf::SERVER_FTP;
      break;

    case ftpsImplicit:
      ServerType = TFileZillaIntf::SERVER_FTP_SSL_IMPLICIT;
      FSessionInfo.SecurityProtocolName = LoadStr(FTPS_IMPLICIT);
      break;

    case ftpsExplicitSsl:
      ServerType = TFileZillaIntf::SERVER_FTP_SSL_EXPLICIT;
      FSessionInfo.SecurityProtocolName = LoadStr(FTPS_EXPLICIT);
      break;

    case ftpsExplicitTls:
      ServerType = TFileZillaIntf::SERVER_FTP_TLS_EXPLICIT;
      FSessionInfo.SecurityProtocolName = LoadStr(FTPS_EXPLICIT);
      break;

    default:
      DebugFail();
      break;
  }

  const int32_t Pasv = (Data->GetFtpPasvMode() ? 1 : 2);

  const int32_t TimeZoneOffset = Data->GetTimeDifferenceAuto() ? 0 : TimeToMinutes(Data->GetTimeDifference());

  int32_t UTF8;
  const uint32_t CodePage = Data->GetCodePageAsNumber();

  switch (CodePage)
  {
  case CP_ACP:
    UTF8 = 2; // no UTF8
    break;

  case CP_UTF8:
    UTF8 = 1; // always UTF8
    break;

  default:
    UTF8 = 0; // auto detect
    break;
  }

  FPasswordFailed = false;
  FStoredPasswordTried = false;
  bool PromptedForCredentials = false;

  do
  {
    FDetectTimeDifference = Data->GetTimeDifferenceAuto();
    FTimeDifference = 0;
    ResetFeatures();
    FSystem.Clear();
    FServerID = EmptyStr;
    FWelcomeMessage.Clear();
    FFileSystemInfoValid = false;

    TODO("the same for account? it ever used?");

    // ask for username if it was not specified in advance, even on retry,
    // but keep previous one as default,
    if (Data->GetUserNameExpanded().IsEmpty() && !FTerminal->GetSessionData()->GetFingerprintScan())
    {
      FTerminal->LogEvent("Username prompt (no username provided)");

      if (!FPasswordFailed && !PromptedForCredentials)
      {
        FTerminal->Information(LoadStr(FTP_CREDENTIAL_PROMPT), false);
        PromptedForCredentials = true;
      }

      if (!FTerminal->PromptUser(Data, pkUserName, LoadStr(USERNAME_TITLE), "",
            LoadStr(USERNAME_PROMPT2), true, 0, UserName))
      {
        FTerminal->FatalError(nullptr, LoadStr(CREDENTIALS_NOT_SPECIFIED));
      }
      else
      {
        FUserName = UserName;
      }
    }

    // On retry ask for password.
    // This is particularly important, when stored password is no longer valid,
    // so we do not blindly try keep trying it in a loop (possibly causing account lockout)
    if (FPasswordFailed)
    {
      FTerminal->LogEvent("Password prompt (last login attempt failed)");

      Password.Clear();
      if (!FTerminal->PromptUser(Data, pkPassword, LoadStr(PASSWORD_TITLE), "",
            LoadStr(PASSWORD_PROMPT), false, 0, Password))
      {
        FTerminal->FatalError(nullptr, LoadStr(CREDENTIALS_NOT_SPECIFIED));
      }
    }

    if ((Data->GetFtps() != ftpsNone) && (FCertificate == nullptr))
    {
      FTerminal->LoadTlsCertificate(FCertificate, FPrivateKey);
    }

    FPasswordFailed = false;
    TAutoFlag OpeningFlag(FOpening); nb::used(OpeningFlag);

    FActive = FFileZillaIntf->Connect(
      HostName.c_str(), nb::ToInt32(Data->GetPortNumber()), UserName.c_str(),
      Password.c_str(), Account.c_str(), Path.c_str(),
      ServerType, nb::ToInt32(Pasv), nb::ToInt32(TimeZoneOffset), UTF8, Data->FFtpForcePasvIp,
      nb::ToInt32(Data->GetFtpUseMlsd()), FCertificate, FPrivateKey,
      nb::ToInt32(CodePage), nb::ToInt32(Data->GetFtpDupFF()), nb::ToInt32(Data->GetFtpUndupFF()));

    DebugAssert(FActive);

    try
    {
      // do not wait for FTP response code as Connect is complex operation
      GotReply(WaitForCommandReply(false), REPLY_CONNECT, LoadStr(CONNECTION_FAILED));

      Shred(Password);

      // we have passed, even if we got 530 on the way (if it is possible at all),
      // ignore it
      DebugAssert(!FPasswordFailed);
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

  ProcessFeatures();

  // see also TWebDAVFileSystem::CollectTLSSessionInfo()
  FSessionInfo.CSCipher = FFileZillaIntf->GetCipherName().c_str();
  FSessionInfo.SCCipher = FSessionInfo.CSCipher;
  UnicodeString TlsVersionStr = FFileZillaIntf->GetTlsVersionStr().c_str();
  AddToList(FSessionInfo.SecurityProtocolName, TlsVersionStr, L", ");
  FLoggedIn = true;
}

void TFTPFileSystem::Close()
{
  DebugAssert(FActive);
  bool Result = DoQuit();
  if (!Result)
  {
    if (FFileZillaIntf->Close(FOpening))
    {
      DebugCheck(FLAGSET(WaitForCommandReply(false), TFileZillaIntf::REPLY_DISCONNECTED));
      Result = true;
    }
    else
    {
      // See TFileZillaIntf::Close
      Result = FOpening;
    }
  }

  if (DebugAlwaysTrue(Result))
  {
    DebugAssert(FActive);
    Disconnect();
  }
}

bool TFTPFileSystem::GetActive() const
{
  return FActive;
}

void TFTPFileSystem::CollectUsage()
{
  switch (FTerminal->SessionData->Ftps)
  {
    case ftpsNone:
      // noop
      break;

    case ftpsImplicit:
      FTerminal->Configuration->Usage->Inc(L"OpenedSessionsFTPSImplicit");
      break;

    case ftpsExplicitSsl:
      FTerminal->Configuration->Usage->Inc(L"OpenedSessionsFTPSExplicitSSL");
      break;

    case ftpsExplicitTls:
      FTerminal->Configuration->Usage->Inc(L"OpenedSessionsFTPSExplicitTLS");
      break;

    default:
      DebugFail();
      break;
  }

  if (!FTerminal->SessionData->TlsCertificateFile().IsEmpty())
  {
    FTerminal->Configuration->Usage->Inc("OpenedSessionsFTPSCertificate");
  }

  if (FFileZillaIntf->UsingMlsd())
  {
    FTerminal->Configuration->Usage->Inc("OpenedSessionsFTPMLSD");
  }
  else
  {
    FTerminal->Configuration->Usage->Inc("OpenedSessionsFTPLIST");
  }

  if (FFileZillaIntf->UsingUtf8())
  {
    FTerminal->Configuration->Usage->Inc("OpenedSessionsFTPUTF8");
  }
  else
  {
    FTerminal->Configuration->Usage->Inc("OpenedSessionsFTPNonUTF8");
  }
  if (!RemoteGetCurrentDirectory().IsEmpty() && (RemoteGetCurrentDirectory()[1] != Slash))
  {
    if (base::IsUnixStyleWindowsPath(RemoteGetCurrentDirectory()))
    {
      FTerminal->Configuration->Usage->Inc("OpenedSessionsFTPWindowsPath");
    }
    else if ((RemoteGetCurrentDirectory().Length() >= 3) && IsLetter(RemoteGetCurrentDirectory()[1]) && (RemoteGetCurrentDirectory()[2] == L':') && (RemoteGetCurrentDirectory()[3] == Slash))
    {
      FTerminal->Configuration->Usage->Inc("OpenedSessionsFTPRealWindowsPath");
    }
    else
    {
      FTerminal->Configuration->Usage->Inc("OpenedSessionsFTPOtherPath");
    }
  }

  const UnicodeString TlsVersionStr = FFileZillaIntf->GetTlsVersionStr().c_str();
  if (!TlsVersionStr.IsEmpty())
  {
    FTerminal->CollectTlsUsage(TlsVersionStr);
  }

  if (FFileZilla)
  {
    FTerminal->Configuration->Usage->Inc("OpenedSessionsFTPFileZilla");
  }
  // 220 ProFTPD 1.3.4a Server (Debian) [::ffff:192.168.179.137]
  // SYST
  // 215 UNIX Type: L8
  else if (ContainsText(FWelcomeMessage, "ProFTPD"))
  {
    FTerminal->Configuration->Usage->Inc("OpenedSessionsFTPProFTPD");
  }
  // 220 Microsoft FTP Service
  // SYST
  // 215 Windows_NT
  else if (FIIS)
  {
    FTerminal->Configuration->Usage->Inc("OpenedSessionsFTPIIS");
  }
  // 220 (vsFTPd 3.0.2)
  // SYST
  // 215 UNIX Type: L8
  // (Welcome message is configurable)
  else if (ContainsText(FWelcomeMessage, "vsFTPd"))
  {
    FTerminal->Configuration->Usage->Inc("OpenedSessionsFTPvsFTPd");
  }
  // 220 Welcome to Pure-FTPd.
  // ...
  // SYST
  // 215 UNIX Type: L8
  else if (ContainsText(FWelcomeMessage, "Pure-FTPd"))
  {
    FTerminal->Configuration->Usage->Inc("OpenedSessionsFTPPureFTPd");
  }
  // 220 Titan FTP Server 10.47.1892 Ready.
  // ...
  // SYST
  // 215 UNIX Type: L8
  else if (ContainsText(FWelcomeMessage, "Titan FTP Server"))
  {
    FTerminal->Configuration->Usage->Inc("OpenedSessionsFTPTitan");
  }
  // 220-Cerberus FTP Server - Home Edition
  // 220-This is the UNLICENSED Home Edition and may be used for home, personal use only
  // 220-Welcome to Cerberus FTP Server
  // 220 Created by Cerberus, LLC
  else if (ContainsText(FWelcomeMessage, "Cerberus FTP Server"))
  {
    FTerminal->Configuration->Usage->Inc("OpenedSessionsFTPCerberus");
  }
  // 220 Serv-U FTP Server v15.1 ready...
  else if (ContainsText(FWelcomeMessage, "Serv-U FTP Server"))
  {
    FTerminal->Configuration->Usage->Inc("OpenedSessionsFTPServU");
  }
  else if (ContainsText(FWelcomeMessage, "WS_FTP"))
  {
    FTerminal->Configuration->Usage->Inc("OpenedSessionsFTPWSFTP");
  }
  // 220 Welcome to the most popular FTP hosting service! Save on hardware, software, hosting and admin. Share files/folders with read-write permission. Visit http://www.drivehq.com/ftp/
  // ...
  // SYST
  // 215 UNIX emulated by DriveHQ FTP Server.
  else if (ContainsText(FSystem, "DriveHQ"))
  {
    FTerminal->Configuration->Usage->Inc("OpenedSessionsFTPDriveHQ");
  }
  // 220 GlobalSCAPE EFT Server (v. 6.0) * UNREGISTERED COPY *
  // ...
  // SYST
  // 215 UNIX Type: L8
  else if (ContainsText(FWelcomeMessage, "GlobalSCAPE"))
  {
    FTerminal->Configuration->Usage->Inc("OpenedSessionsFTPGlobalScape");
  }
  // 220-<custom message>
  // 220 CompleteFTP v 8.1.3
  // ...
  // SYST
  // UNIX Type: L8
  else if (ContainsText(FWelcomeMessage, "CompleteFTP"))
  {
    FTerminal->Configuration->Usage->Inc("OpenedSessionsFTPComplete");
  }
  // 220 Core FTP Server Version 1.2, build 567, 64-bit, installed 8 days ago Unregistered
  // ...
  // SYST
  // 215 UNIX Type: L8
  else if (ContainsText(FWelcomeMessage, "Core FTP Server"))
  {
    FTerminal->Configuration->Usage->Inc("OpenedSessionsFTPCore");
  }
  // 220 Service ready for new user.
  // ..
  // SYST
  // 215 UNIX Type: Apache FtpServer
  // (e.g. brickftp.com)
  else if (ContainsText(FSystem, "Apache FtpServer"))
  {
    FTerminal->Configuration->Usage->Inc("OpenedSessionsFTPApache");
  }
  // 220 pos1 FTP server (GNU inetutils 1.3b) ready.
  // ...
  // SYST
  // 215 UNIX Type: L8 Version: Linux 2.6.15.7-ELinOS-314pm3
  // Displaying "(GNU inetutils 1.3b)" in a welcome message can be turned off (-q switch):
  // 220 pos1 FTP server ready.
  // (the same for "Version: Linux 2.6.15.7-ELinOS-314pm3" in SYST response)
  else if (ContainsText(FWelcomeMessage, "GNU inetutils"))
  {
    FTerminal->Configuration->Usage->Inc("OpenedSessionsFTPInetutils");
  }
  // 220 Syncplify.me Server! FTP(S) Service Ready
  // Message is configurable
  else if (ContainsText(FWelcomeMessage, "Syncplify"))
  {
    FTerminal->Configuration->Usage->Inc("OpenedSessionsFTPSyncplify");
  }
  // 220-Idea FTP Server v0.80 (xxx.home.pl) [xxx.xxx.xxx.xxx]
  // 220 Ready
  // ...
  // SYST
  // UNIX Type: L8
  else if (ContainsText(FWelcomeMessage, "Idea FTP Server"))
  {
    FTerminal->Configuration->Usage->Inc("OpenedSessionsFTPIdea");
  }
  // 220-FTPD1 IBM FTP CS V2R1 at name.test.com, 13:49:38 on 2016-01-28.
  // ...
  // SYST
  // 215 MVS is the operating system of this server. FTP Server is running on z/OS.
  else if (FMVS)
  {
    FTerminal->Configuration->Usage->Inc("OpenedSessionsFTPMVS");
  }
  // 220 xxx.xxx.xxx (xxx.xxx.xxx) FTP-OpenVMS FTPD V5.3-3 (c) 1998 Process Software Corporation
  // ...
  // SYST
  // 215 VMS system type. VMS V5.5-2.
  else if (FVMS)
  {
    FTerminal->Configuration->Usage->Inc("OpenedSessionsFTPVMS");
  }
  else
  {
    FTerminal->Configuration->Usage->Inc("OpenedSessionsFTPOther");
  }
}

void TFTPFileSystem::DummyReadDirectory(const UnicodeString & ADirectory)
{
  std::unique_ptr<TRemoteDirectory> Files(std::make_unique<TRemoteDirectory>(FTerminal));
  try
  {
    Files->SetDirectory(ADirectory);
    DoReadDirectory(Files.get());
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

void TFTPFileSystem::Idle()
{
  if (FActive && !FWaitingForReply)
  {
    PoolForFatalNonCommandReply();

    // Keep session alive
    if ((FTerminal->GetSessionData()->GetFtpPingType() == fptDirectoryListing) &&
        ((Now() - FLastDataSent).GetValue() > FTerminal->GetSessionData()->GetFtpPingIntervalDT().GetValue() * 4))
    {
      FTerminal->LogEvent("Dummy directory read to keep session alive.");
      FLastDataSent = Now(); // probably redundant to the same statement in DoReadDirectory

      DummyReadDirectory(RemoteGetCurrentDirectory());
    }
  }
}

void TFTPFileSystem::Discard()
{
  // remove all pending messages, to get complete log
  // note that we need to retry discard on reconnect, as there still may be another
  // "disconnect/timeout/..." status messages coming
  DiscardMessages();
  DebugAssert(FActive);
  FActive = false;

  // See neon's ne_ssl_clicert_free
  if (FPrivateKey != nullptr)
  {
    EVP_PKEY_free(FPrivateKey);
    FPrivateKey = nullptr;
  }
  if (FCertificate != nullptr)
  {
    X509_free(FCertificate);
    FCertificate = nullptr;
  }
}

UnicodeString TFTPFileSystem::GetAbsolutePath(const UnicodeString & APath, bool Local)
{
  return static_cast<const TFTPFileSystem *>(this)->GetAbsolutePath(APath, Local);
}

UnicodeString TFTPFileSystem::GetAbsolutePath(const UnicodeString & APath, bool /*Local*/) const
{
  TODO("improve (handle .. etc.)");
  if (base::UnixIsAbsolutePath(APath))
  {
    return APath;
  }
  return base::AbsolutePath(FCurrentDirectory, APath);
}

UnicodeString TFTPFileSystem::GetActualCurrentDirectory() const
{
  UnicodeString CurrentPath(nb::NB_MAX_PATH, 0);
  UnicodeString Result;
  if (FFileZillaIntf->GetCurrentPath(ToWCharPtr(CurrentPath), CurrentPath.Length()))
  {
    Result = base::UnixExcludeTrailingBackslash(CurrentPath);
  }
  PackStr(Result);
  return Result;
}

void TFTPFileSystem::EnsureLocation(const UnicodeString & ADirectory, bool Log)
{
  const UnicodeString Directory = base::UnixExcludeTrailingBackslash(ADirectory);
  if (!base::UnixSamePath(GetActualCurrentDirectory(), Directory))
  {
    if (Log)
    {
      FTerminal->LogEvent(FORMAT("Synchronizing current directory \"%s\".",
          Directory));
    }

    DoChangeDirectory(Directory);
    // make sure FZAPI is aware that we changed current working directory
    FFileZillaIntf->SetCurrentPath(Directory.c_str());
  }
}

void TFTPFileSystem::EnsureLocation()
{
  // if we do not know what's the current directory, do nothing
  if (!FCurrentDirectory.IsEmpty())
  {
    // Make sure that the FZAPI current working directory,
    // is actually our working directory.
    // It may not be because:
    // 1) We did cached directory change
    // 2) Listing was requested for non-current directory, which
    // makes FZAPI change its current directory (and not restoring it back afterwards)
    EnsureLocation(FCurrentDirectory, true);
  }
}

bool TFTPFileSystem::EnsureLocationWhenWorkFromCwd(const UnicodeString & Directory)
{
  const bool Result = (FWorkFromCwd == asOn);
  if (Result)
  {
    EnsureLocation(Directory, false);
  }
  return Result;
}


void TFTPFileSystem::AnyCommand(const UnicodeString & Command,
  TCaptureOutputEvent && OutputEvent)
{
  // end-user has right to expect that client current directory is really
  // current directory for the server
  EnsureLocation();

  DebugAssert(FOnCaptureOutput.empty());
  FOnCaptureOutput = OutputEvent;
  try__finally
  {
    SendCommand(Command);

    GotReply(WaitForCommandReply(), REPLY_2XX_CODE | REPLY_3XX_CODE);
  }
  __finally
  {
    FOnCaptureOutput = nullptr;
  } end_try__finally
}

void TFTPFileSystem::ResetCaches()
{
  SAFE_DESTROY(FFileListCache);
}

void TFTPFileSystem::AnnounceFileListOperation()
{
  ResetCaches();
}

bool TFTPFileSystem::DoQuit()
{
  SendCommand("QUIT");

  const uint32_t Reply = WaitForCommandReply(true);
  const bool Result =
    FLAGSET(Reply, TFileZillaIntf::REPLY_OK) ||
    FLAGSET(Reply, TFileZillaIntf::REPLY_DISCONNECTED);
  return Result;
}

void TFTPFileSystem::SendCwd(const UnicodeString & Directory)
{
  const UnicodeString Command = FORMAT(L"CWD %s", Directory);
  SendCommand(Command);

  GotReply(WaitForCommandReply(), REPLY_2XX_CODE);
}

void TFTPFileSystem::DoChangeDirectory(const UnicodeString & Directory)
{
  if (FWorkFromCwd == asOn)
  {
    const UnicodeString ADirectory = base::UnixIncludeTrailingBackslash(GetAbsolutePath(Directory, false));

    UnicodeString Actual = base::UnixIncludeTrailingBackslash(GetActualCurrentDirectory());
    while (!base::UnixSamePath(Actual, ADirectory))
    {
      if (base::UnixIsChildPath(Actual, ADirectory))
      {
        UnicodeString SubDirectory = base::UnixExcludeTrailingBackslash(ADirectory);
        SubDirectory.Delete(1, Actual.Length());
        const int32_t P = SubDirectory.Pos(L'/');
        if (P > 0)
        {
          SubDirectory.SetLength(P - 1);
        }
        SendCwd(SubDirectory);
        Actual = base::UnixIncludeTrailingBackslash(Actual + SubDirectory);
      }
      else
      {
        SendCommand(L"CDUP");
        GotReply(WaitForCommandReply(), REPLY_2XX_CODE);
        Actual = base::UnixExtractFilePath(base::UnixExcludeTrailingBackslash(Actual));
      }
    }
  }
  else
  {
    SendCwd(Directory);
  }
}

void TFTPFileSystem::ChangeDirectory(const UnicodeString & ADirectory)
{
  UnicodeString Directory = ADirectory;
  try
  {
    // For changing directory, we do not make paths absolute, instead we
    // delegate this to the server, hence we synchronize current working
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
      Directory = GetAbsolutePath(Directory, false);
    }
    else
    {
      throw;
    }
  }

  DoChangeDirectory(Directory);

  // make next ReadCurrentDirectory retrieve actual server-side current directory
  FReadCurrentDirectory = true;
}

void TFTPFileSystem::CachedChangeDirectory(const UnicodeString & ADirectory)
{
  FCurrentDirectory = base::UnixExcludeTrailingBackslash(ADirectory);
  FReadCurrentDirectory = false;
}

void TFTPFileSystem::ChangeFileProperties(const UnicodeString & AFileName,
  const TRemoteFile * AFile, const TRemoteProperties * Properties,
  TChmodSessionAction & Action)
{
  DebugAssert(Properties);
  DebugAssert(Properties && !Properties->Valid.Contains(vpGroup)); //-V595
  DebugAssert(Properties && !Properties->Valid.Contains(vpOwner)); //-V595
  DebugAssert(Properties && !Properties->Valid.Contains(vpLastAccess)); //-V595
  DebugAssert(Properties && !Properties->Valid.Contains(vpModification)); //-V595

  if (Properties && Properties->Valid.Contains(vpRights))
  {
    std::unique_ptr<TRemoteFile> OwnedFile;

    try__finally
    {
      UnicodeString FileName = GetAbsolutePath(AFileName, false);

      if (AFile == nullptr)
      {
        TRemoteFile * File = nullptr;
        ReadFile(FileName, File);
        OwnedFile.reset(File);
        AFile = File;
      }

      if ((AFile != nullptr) && AFile->GetIsDirectory() && FTerminal->CanRecurseToDirectory(AFile) && Properties->Recursive)
      {
        try
        {
          FTerminal->ProcessDirectory(AFileName, nb::bind(&TTerminal::ChangeFileProperties, FTerminal),
            nb::ToPtr(const_cast<TRemoteProperties *>(Properties)));
        }
        catch (...)
        {
          Action.Cancel();
          throw;
        }
      }

      TRights Rights;
      if (AFile != nullptr)
      {
        Rights = *AFile->GetRights();
      }
      Rights |= Properties->Rights.GetNumberSet();
      Rights &= static_cast<uint16_t>(~Properties->Rights.GetNumberUnset());
      if ((AFile != nullptr) && AFile->GetIsDirectory() && Properties->AddXToDirectories)
      {
        Rights.AddExecute();
      }

      Action.Rights(Rights);

      UnicodeString FileNameOnly = base::UnixExtractFileName(FileName);
      UnicodeString FilePath = RemoteExtractFilePath(FileName);
      // FZAPI wants octal number represented as decadic
      FFileZillaIntf->Chmod(Rights.GetNumberDecadic(), FileNameOnly.c_str(), FilePath.c_str());

      GotReply(WaitForCommandReply(), REPLY_2XX_CODE);
    }
    __finally__removed
    {
      // delete OwnedFile;
    } end_try__finally
  }
  else
  {
    Action.Cancel();
  }
}

bool TFTPFileSystem::LoadFilesProperties(TStrings * /*FileList*/)
{
  DebugFail();
  return false;
}

UnicodeString TFTPFileSystem::DoCalculateFileChecksum(const UnicodeString & Alg, const TRemoteFile * AFile)
{
  // Overview of server supporting various hash commands is at:
  // https://datatracker.ietf.org/doc/html/draft-bryan-ftpext-hash-02#appendix-B

  UnicodeString CommandName;

  const bool UsingHashCommand = UsingHashCommandChecksum(Alg);
  if (UsingHashCommand)
  {
    CommandName = HashCommand;
  }
  else
  {
    const int32_t Index = FChecksumAlgs->IndexOf(Alg);
    if (Index < 0)
    {
      DebugFail();
      ThrowExtException();
    }
    else
    {
      CommandName = FChecksumCommands->GetString(Index);
    }
  }

  UnicodeString FileName = AFile->GetFullFileName();
  // FTP way is not to quote.
  // But as Serv-U, GlobalSCAPE and possibly others allow
  // additional parameters (SP ER range), they need to quote file name.
  // Cerberus and FileZilla Server on the other hand can do without quotes
  // (but they can handle them, not sure about other servers)

  // Quoting:
  // FileZilla Server simply checks if argument starts and ends with double-quote
  // and strips them, no double-quote escaping is possible.
  // That's for all commands, not just HASH
  // ProFTPD: TODO: Check how "SITE SYMLINK target link" is parsed

  // We can possibly autodetect this from announced command format:
  // XCRC filename;start;end
  // XMD5 filename;start;end
  // XSHA1 filename;start;end
  // XSHA256 filename;start;end
  // XSHA512 filename;start;end
  if (FileName.Pos(L" ") > 0)
  {
    FileName = FORMAT("\"%s\"", FileName);
  }

  const UnicodeString Command = FORMAT("%s %s", CommandName, FileName);
  SendCommand(Command);
  TStrings * Response;
  GotReply(WaitForCommandReply(), REPLY_2XX_CODE, EmptyStr, nullptr, &Response);
  UnicodeString ResponseText;
  if (DebugAlwaysTrue(Response->Count > 0))
  {
    // ProFTPD response has this format:
    // 213-Computing MD5 digest
    // 213 MD5 0-687104 b359479cfda703b7fd473c7e09f39049 filename
    ResponseText = Response->GetString(Response->Count() - 1);
  }
  delete Response;

  UnicodeString Hash;
  if (UsingHashCommand)
  {
    // Code should be 213, but let's be tolerant and accept any 2xx

    // ("213" SP) hashname SP start-point "-" end-point SP filehash SP <pathname> (CRLF)
    UnicodeString Buf = ResponseText;
    // skip alg
    CutToChar(Buf, L' ', true);
    // skip range
    const UnicodeString Range = CutToChar(Buf, L' ', true);
    // This should be range (SP-EP), but if it does not conform to the format,
    // it's likely because the server uses version of the HASH spec
    // before draft-ietf-ftpext2-hash-01
    // (including draft-bryan-ftp-hash-06 implemented by FileZilla server; or Cerberus),
    // that did not have the "range" part.
    // The FileZilla Server even omits the file name.
    // The latest draft as of implementing this is draft-bryan-ftpext-hash-02.
    if (Range.Pos(L"-") > 0)
    {
      Hash = CutToChar(Buf, L' ', true);
    }
    else
    {
      Hash = Range;
    }
  }
  else // All hash-specific commands
  {
    // Accepting any 2xx response. Most servers use 213,
    // but for example WS_FTP uses non-sense code 220 (Service ready for new user)

    // MD5 response according to a draft-twine-ftpmd5-00 includes a file name
    // (implemented by Apache FtpServer).
    // Other commands (X<hash>) return the hash only.
    ResponseText = ResponseText.Trim();
    const int32_t P = ResponseText.LastDelimiter(L" ");
    if (P > 0)
    {
      ResponseText.Delete(1, P);
    }

    Hash = ResponseText;
  }

  if (Hash.IsEmpty())
  {
    throw Exception(FMTLOAD(FTP_RESPONSE_ERROR, CommandName, ResponseText));
  }

  return LowerCase(Hash);
}

void TFTPFileSystem::CalculateFilesChecksum(
  const UnicodeString & Alg, TStrings * FileList, TCalculatedChecksumEvent && OnCalculatedChecksum,
  TFileOperationProgressType * OperationProgress, bool FirstLevel)
{
  FTerminal->CalculateSubFoldersChecksum(Alg, FileList, std::forward<TCalculatedChecksumEvent>(OnCalculatedChecksum), OperationProgress, FirstLevel);

  int32_t Index = 0;
  TOnceDoneOperation OnceDoneOperation; // not used

  int32_t Index1 = 0;
  while ((Index1 < FileList->GetCount()) && !OperationProgress->GetCancel())
  {
    TRemoteFile * File = FileList->GetAs<TRemoteFile>(Index1);
    DebugAssert(File != nullptr);

    if (File && !File->GetIsDirectory())
    {
      TChecksumSessionAction Action(FTerminal->ActionLog);
      try
      {
        OperationProgress->SetFile(File->GetFileName());
        Action.SetFileName(File->FullFileName);
        bool Success = false;
        try__finally
        {
          const UnicodeString Checksum = DoCalculateFileChecksum(Alg, File);

          if (!OnCalculatedChecksum.empty())
          {
            OnCalculatedChecksum(File->FileName, Alg, Checksum);
          }
          Action.Checksum(Alg, Checksum);
          Success = true;
        }
        __finally
        {
          if (FirstLevel)
          {
            OperationProgress->Finish(File->FileName, Success, OnceDoneOperation);
          }
        } end_try__finally
      }
      catch (Exception &E)
      {
        FTerminal->RollbackAction(Action, OperationProgress, &E);

        // Error formatting expanded from inline to avoid strange exceptions
        UnicodeString Error = FMTLOAD(CHECKSUM_ERROR, File != nullptr ? File->GetFullFileName() : L"");
        FTerminal->CommandError(&E, Error);
        // Abort loop.
        TODO("retries? resume?");
        Index1 = FileList->GetCount();
      }
    }
    Index1++;
  }
}

UnicodeString TFTPFileSystem::CalculateFilesChecksumInitialize(const UnicodeString & Alg)
{
  UnicodeString NormalizedAlg = FindIdent(FindIdent(Alg, FHashAlgs.get()), FChecksumAlgs.get());
  if (UsingHashCommandChecksum(NormalizedAlg))
  {
    // The server should understand lowercase alg name by spec,
    // but we should use uppercase anyway
    SendCommand(FORMAT(L"OPTS %s %s", HashCommand, UpperCase(NormalizedAlg)));
    GotReply(WaitForCommandReply(), REPLY_2XX_CODE);
  }
  else if (FChecksumAlgs->IndexOf(NormalizedAlg) >= 0)
  {
    // will use algorithm-specific command
  }
  else
  {
    throw Exception(FMTLOAD(UNKNOWN_CHECKSUM, Alg));
  }
  return NormalizedAlg;
}

bool TFTPFileSystem::UsingHashCommandChecksum(const UnicodeString & Alg) const
{
  return (FHashAlgs->IndexOf(Alg) >= 0);
}

bool TFTPFileSystem::ConfirmOverwrite(
  const UnicodeString & ASourceFullFileName, UnicodeString & ATargetFileName,
  TOverwriteMode & OverwriteMode, TFileOperationProgressType * OperationProgress,
  const TOverwriteFileParams * FileParams, const TCopyParamType * CopyParam,
  int32_t Params, bool AutoResume)
{
  const bool CanAutoResume = FLAGSET(Params, cpNoConfirmation) && AutoResume;
  const bool DestIsSmaller = (FileParams != nullptr) && (FileParams->DestSize < FileParams->SourceSize);
  const bool DestIsSame = (FileParams != nullptr) && (FileParams->DestSize == FileParams->SourceSize);
  const bool CanResume =
    !OperationProgress->GetAsciiTransfer() &&
    // when resuming transfer after interrupted connection,
    // do nothing (dummy resume) when the files has the same size.
    // this is workaround for servers that strangely fails just after successful
    // upload.
    (DestIsSmaller || (DestIsSame && CanAutoResume));

  uint32_t Answer;
  if (CanAutoResume && CanResume)
  {
    if (DestIsSame)
    {
      DebugAssert(CanAutoResume);
      Answer = qaSkip;
    }
    else
    {
      Answer = qaRetry;
    }
  }
  else
  {
    // retry = "resume"
    // all = "yes to newer"
    // ignore = "rename"
    uint32_t Answers = qaYes | qaNo | qaCancel | qaYesToAll | qaNoToAll | qaAll | qaIgnore;
    if (CanResume)
    {
      Answers |= qaRetry;
    }
    TQueryButtonAlias Aliases[5];
    Aliases[0].Button = qaRetry;
    Aliases[0].Alias = LoadStr(RESUME_BUTTON);
    Aliases[0].GroupWith = qaNo;
    Aliases[0].GroupedShiftState = ssAlt;
    Aliases[1] = TQueryButtonAlias::CreateAllAsYesToNewerGroupedWithYes();
    Aliases[2] = TQueryButtonAlias::CreateIgnoreAsRenameGroupedWithNo();
    Aliases[3] = TQueryButtonAlias::CreateYesToAllGroupedWithYes();
    Aliases[4] = TQueryButtonAlias::CreateNoToAllGroupedWithNo();
    TQueryParams QueryParams(qpNeverAskAgainCheck);
    QueryParams.Aliases = Aliases;
    QueryParams.AliasesCount = _countof(Aliases);

    {
      TSuspendFileOperationProgress Suspend(OperationProgress); nb::used(Suspend);
      Answer = FTerminal->ConfirmFileOverwrite(
        ASourceFullFileName, ATargetFileName, FileParams, Answers, &QueryParams,
        ReverseOperationSide(OperationProgress->GetSide()),
        CopyParam, Params, OperationProgress);
    }
  }

  bool Result = true;

  switch (Answer)
  {
    // resume
    case qaRetry:
      OverwriteMode = omResume;
      DebugAssert(FileParams != nullptr);
      DebugAssert(CanResume);
      FFileTransferResumed = FileParams ? FileParams->DestSize : 0;
      break;

    // rename
    case qaIgnore:
    {
      if (FTerminal->PromptUser(FTerminal->GetSessionData(), pkFileName,
          LoadStr(RENAME_TITLE), L"", LoadStr(RENAME_PROMPT2), true, 0, ATargetFileName))
      {
        OverwriteMode = omOverwrite;
      }
      else
      {
        OperationProgress->SetCancelAtLeast(csCancel);
        FFileTransferAbort = ftaCancel;
        Result = false;
      }
    }
      break;

    case qaYes:
      OverwriteMode = omOverwrite;
      break;

    case qaCancel:
      OperationProgress->SetCancelAtLeast(csCancel);
      FFileTransferAbort = ftaCancel;
      Result = false;
      break;

    case qaNo:
      FFileTransferAbort = ftaSkip;
      Result = false;
      break;

    case qaSkip:
      OverwriteMode = omComplete;
      break;

    default:
      DebugFail();
      Result = false;
      break;
  }
  return Result;
}

void TFTPFileSystem::ResetFileTransfer()
{
  FFileTransferAbort = ftaNone;
  FFileTransferCancelled = false;
  FFileTransferResumed = 0;
}

void TFTPFileSystem::ReadDirectoryProgress(int64_t Bytes)
{
  // with FTP we do not know exactly how many entries we have received,
  // instead we know number of bytes received only.
  // so we report approximation based on average size of entry.
  const int32_t Progress = nb::ToInt32(Bytes / 80);
  const DWORD Ticks = GetTickCount();
  if ((Ticks - FLastReadDirectoryProgress >= 100) &&
      // Cannot call OnReadDirectoryProgress with 0 as it would unmatch the "starting" and "ending" signals for disabling the window
      (Progress > 0))
  {
    FLastReadDirectoryProgress = Ticks;
    bool Cancel = false;
    FTerminal->DoReadDirectoryProgress(Progress, 0, Cancel);
    if (Cancel)
    {
      FTerminal->DoReadDirectoryProgress(-2, 0, Cancel);
      FFileZillaIntf->Cancel();
    }
  }
}

void TFTPFileSystem::DoFileTransferProgress(int64_t TransferSize,
  int64_t Bytes)
{
  TFileOperationProgressType * OperationProgress = FTerminal->GetOperationProgress();

  OperationProgress->SetTransferSize(TransferSize);

  if (FFileTransferResumed > 0)
  {
    // Bytes will be 0, if resume was not possible
    if (Bytes >= FFileTransferResumed)
    {
      OperationProgress->AddResumed(FFileTransferResumed);
    }
    FFileTransferResumed = 0;
  }

  const int64_t Diff = Bytes - OperationProgress->GetTransferredSize();
  if (DebugAlwaysTrue(Diff >= 0))
  {
    OperationProgress->AddTransferred(Diff);
    FFileTransferAny = true;
  }

  if (OperationProgress->GetCancel() != csContinue)
  {
    if (OperationProgress->ClearCancelFile())
    {
      FFileTransferAbort = ftaSkip;
    }
    else
    {
      FFileTransferAbort = ftaCancel;
    }
    FFileTransferCancelled = true;
    FFileZillaIntf->Cancel();
  }

  if (FFileTransferCPSLimit != OperationProgress->GetCPSLimit())
  {
    SetCPSLimit(OperationProgress);
  }
}

void TFTPFileSystem::SetCPSLimit(TFileOperationProgressType * OperationProgress)
{
  // Any reason we use separate field instead of directly using OperationProgress->CPSLimit?
  // Maybe thread-safety?
  FFileTransferCPSLimit = OperationProgress->GetCPSLimit();
  OperationProgress->SetSpeedCounters();
}

void TFTPFileSystem::FileTransferProgress(int64_t TransferSize,
  int64_t Bytes)
{
  TGuard Guard(FTransferStatusCriticalSection); nb::used(Guard);

  DoFileTransferProgress(TransferSize, Bytes);
}

void TFTPFileSystem::FileTransfer(const UnicodeString & AFileName,
  const UnicodeString & LocalFile, const UnicodeString & RemoteFile,
  const UnicodeString & RemotePath, bool Get, int64_t Size, int32_t Type,
  TFileTransferData & UserData, TFileOperationProgressType * OperationProgress)
{
  FILE_OPERATION_LOOP_BEGIN
  {
    FFileZillaIntf->FileTransfer(
      ApiPath(LocalFile).c_str(), RemoteFile.c_str(), RemotePath.c_str(),
      Get, Size, nb::ToInt32(Type), &UserData, std::move(UserData.CopyParam->FOnTransferOut), std::move(UserData.CopyParam->FOnTransferIn));
    // we may actually catch response code of the listing
    // command (when checking for existence of the remote file)
    const uint32_t Reply = WaitForCommandReply();
    GotReply(Reply, FLAGMASK(FFileTransferCancelled, REPLY_ALLOW_CANCEL));
  }
  FILE_OPERATION_LOOP_END(FMTLOAD(TRANSFER_ERROR, AFileName));

  switch (FFileTransferAbort)
  {
    case ftaSkip:
      throw ESkipFile();

    case ftaCancel:
      Abort();
      break;
  }

  if (!FFileTransferCancelled)
  {
    // show completion of transfer
    // call non-guarded variant to avoid deadlock with keepalives
    // (we are not waiting for reply anymore so keepalives are free to proceed)
    DoFileTransferProgress(OperationProgress->GetTransferSize(), OperationProgress->GetTransferSize());
    FAnyTransferSucceeded = true;
  }
}

void TFTPFileSystem::CopyToLocal(TStrings * AFilesToCopy,
  const UnicodeString & ATargetDir, const TCopyParamType * CopyParam,
  int32_t Params, TFileOperationProgressType * OperationProgress,
  TOnceDoneOperation & OnceDoneOperation)
{
  Params &= ~cpAppend;

  FTerminal->DoCopyToLocal(
    AFilesToCopy, ATargetDir, CopyParam, Params, OperationProgress, tfUseFileTransferAny, OnceDoneOperation);
}

UnicodeString TFTPFileSystem::RemoteExtractFilePath(const UnicodeString & Path)
{
  UnicodeString Result;
  // If the path ends with a slash, FZAPI CServerPath constructor does not identify the path as VMS.
  // It is probably ok to use UnixExtractFileDir for all paths passed to FZAPI,
  // but for now, we limit the impact of the change to VMS.
  if (FVMS)
  {
    Result = base::UnixExtractFileDir(Path);
  }
  else
  {
    Result = base::UnixExtractFilePath(Path);
  }
  return Result;
}

void TFTPFileSystem::Sink(
  const UnicodeString & AFileName, const TRemoteFile * File,
  const UnicodeString & TargetDir, UnicodeString & DestFileName, int32_t Attrs,
  const TCopyParamType * CopyParam, int32_t Params, TFileOperationProgressType * OperationProgress,
  uint32_t AFlags, TDownloadSessionAction & Action)
{
  AutoDetectTimeDifference(base::UnixExtractFileDir(AFileName), CopyParam, Params);

  ResetFileTransfer();

  TFileTransferData UserData;

  UnicodeString DestFullName = TargetDir + DestFileName;
  UnicodeString FilePath = RemoteExtractFilePath(AFileName);
  const int32_t TransferType = (OperationProgress->GetAsciiTransfer() ? 1 : 2);

  UnicodeString FileName;
  const UnicodeString OnlyFileName = base::UnixExtractFileName(AFileName);
  if (EnsureLocationWhenWorkFromCwd(FilePath))
  {
    FileName = OnlyFileName;
    FilePath = EmptyStr;
  }
  else
  {
    FileName = AFileName;
  }

  {
    // ignore file list
    TFTPFileListHelper Helper(this, nullptr, true); nb::used(Helper);

    SetCPSLimit(OperationProgress);
    FFileTransferPreserveTime = CopyParam->GetPreserveTime();
    // not used for downloads anyway
    FFileTransferRemoveBOM = CopyParam->GetRemoveBOM();
    FFileTransferNoList = CanTransferSkipList(Params, AFlags, CopyParam);
    UserData.FileName = DestFileName;
    UserData.Params = Params;
    UserData.AutoResume = FLAGSET(AFlags, tfAutoResume);
    UserData.CopyParam = const_cast<TCopyParamType *>(CopyParam);
    UserData.Modification = File->GetModification();
    FileTransfer(FileName, DestFullName, OnlyFileName,
      FilePath, true, File->GetSize(), TransferType, UserData, OperationProgress);
  }

  // in case dest filename is changed from overwrite dialog
  if (DestFileName != UserData.FileName)
  {
    DestFullName = TargetDir + UserData.FileName;
    Attrs = FileGetAttrFix(ApiPath(DestFullName));
  }

  const UnicodeString ExpandedDestFullName = ExpandUNCFileName(DestFullName);
  Action.Destination(ExpandedDestFullName);

  if (CopyParam->FOnTransferOut.empty())
  {
    FTerminal->UpdateTargetAttrs(DestFullName, File, CopyParam, Attrs);
  }

  FLastDataSent = Now();
}

void TFTPFileSystem::TransferOnDirectory(
  const UnicodeString & Directory, const TCopyParamType * CopyParam, int32_t Params)
{
  AutoDetectTimeDifference(Directory, CopyParam, Params);
}

void TFTPFileSystem::CopyToRemote(TStrings * AFilesToCopy,
  const UnicodeString & TargetDir, const TCopyParamType * CopyParam,
  int32_t Params, TFileOperationProgressType * OperationProgress,
  TOnceDoneOperation & OnceDoneOperation)
{
  Params &= ~cpAppend;

  FTerminal->DoCopyToRemote(AFilesToCopy, TargetDir, CopyParam, Params, OperationProgress, tfUseFileTransferAny, OnceDoneOperation);
}

bool TFTPFileSystem::CanTransferSkipList(int32_t Params, uint32_t Flags, const TCopyParamType * CopyParam) const
{
  const bool Result =
    (!CopyParam->FOnTransferIn.empty()) ||
    (FLAGSET(Params, cpNoConfirmation) &&
     // cpAppend is not supported with FTP
     DebugAlwaysTrue(FLAGCLEAR(Params, cpAppend)) &&
     FLAGCLEAR(Params, cpResume) &&
     FLAGCLEAR(Flags, tfAutoResume) &&
     !CopyParam->GetNewerOnly());
  return Result;
}

void TFTPFileSystem::Source(
  TLocalFileHandle & Handle, const UnicodeString & TargetDir, UnicodeString & ADestFileName,
  const TCopyParamType * CopyParam, int32_t Params,
  TFileOperationProgressType * OperationProgress, uint32_t Flags,
  TUploadSessionAction & Action, bool & /*ChildError*/)
{
  if (CopyParam->FOnTransferIn.empty())
  {
    Handle.Close();
  }

  ResetFileTransfer();

  TFileTransferData UserData;

  const int32_t TransferType = (OperationProgress->GetAsciiTransfer() ? 1 : 2);

  EnsureLocationWhenWorkFromCwd(TargetDir);

  {
    // ignore file list
    TFTPFileListHelper Helper(this, nullptr, true);

    SetCPSLimit(OperationProgress);
    // not used for uploads anyway
    FFileTransferPreserveTime = CopyParam->GetPreserveTime() && (CopyParam->FOnTransferIn.empty());
    FFileTransferRemoveBOM = CopyParam->GetRemoveBOM();
    FFileTransferNoList = CanTransferSkipList(Params, Flags, CopyParam);
    // not used for uploads, but we get new name (if any) back in this field
    UserData.FileName = ADestFileName;
    UserData.Params = Params;
    UserData.AutoResume = FLAGSET(Flags, tfAutoResume);
    UserData.CopyParam = const_cast<TCopyParamType *>(CopyParam);
    UserData.Modification = Handle.Modification;
    FileTransfer(Handle.FileName, Handle.FileName, ADestFileName,
      TargetDir, false, Handle.Size, TransferType, UserData, OperationProgress);
  }

  const UnicodeString DestFullName = TargetDir + UserData.FileName;
  // only now, we know the final destination
  Action.Destination(DestFullName);

  // We are not able to tell if setting timestamp succeeded,
  // so we log it always (if supported).
  // Support for MDTM does not necessarily mean that the server supports
  // non-standard hack of setting timestamp using
  // MFMT-like (two argument) call to MDTM.
  // IIS definitely does.
  if (FFileTransferPreserveTime &&
      ((FServerCapabilities->GetCapability(mfmt_command) == yes) ||
       ((FServerCapabilities->GetCapability(mdtm_command) == yes))))
  {
    const TTouchSessionAction TouchAction(FTerminal->GetActionLog(), DestFullName, Handle.Modification); nb::used(TouchAction);

    if (!FFileZillaIntf->UsingMlsd())
    {
      FUploadedTimes[DestFullName] = Handle.Modification;
      if ((FTerminal->GetConfiguration()->GetActualLogProtocol() >= 2))
      {
        FTerminal->LogEvent(
          FORMAT("Remembering modification time of \"%s\" as [%s]",
                 DestFullName, StandardTimestamp(FUploadedTimes[DestFullName])));
      }
    }
  }

  FLastDataSent = Now();
}

void TFTPFileSystem::RemoteCreateDirectory(const UnicodeString & ADirName, bool /*Encrypt*/)
{
  const UnicodeString DirName = GetAbsolutePath(ADirName, false);

  {
    // ignore file list
    TFTPFileListHelper Helper(this, nullptr, true); nb::used(Helper);

    FFileZillaIntf->MakeDir(DirName.c_str());

    GotReply(WaitForCommandReply(), REPLY_2XX_CODE);
  }
}

void TFTPFileSystem::RemoteCreateLink(const UnicodeString & AFileName,
  const UnicodeString & APointTo, bool Symbolic)
{
  DebugAssert(SupportsSiteCommand(SymlinkSiteCommand));
  if (DebugAlwaysTrue(Symbolic))
  {
    EnsureLocation();

    const UnicodeString Command = FORMAT("%s %s %s %s", SiteCommand, SymlinkSiteCommand, APointTo, AFileName);
    SendCommand(Command);
    GotReply(WaitForCommandReply(), REPLY_2XX_CODE);
  }
}

void TFTPFileSystem::RemoteDeleteFile(const UnicodeString & AFileName,
  const TRemoteFile * AFile, int32_t Params, TRmSessionAction & Action)
{
  const UnicodeString FileName = GetAbsolutePath(AFileName, false);
  const UnicodeString FileNameOnly = base::UnixExtractFileName(FileName);
  const UnicodeString FilePath = RemoteExtractFilePath(FileName);

  const bool Dir = FTerminal->DeleteContentsIfDirectory(FileName, AFile, Params, Action);

  {
    // ignore file list
    TFTPFileListHelper Helper(this, nullptr, true); nb::used(Helper);

    if (Dir)
    {
      // If current remote directory is in the directory being removed,
      // some servers may refuse to delete it
      // This is common as ProcessDirectory above would CWD to
      // the directory to LIST it.
      // EnsureLocation should reset actual current directory to user's working directory.
      // If user's working directory is still below deleted directory, it is
      // perfectly correct to report an error.
      if (base::UnixIsChildPath(GetActualCurrentDirectory(), FileName))
      {
        EnsureLocation();
      }
      FFileZillaIntf->RemoveDir(FileNameOnly.c_str(), FilePath.c_str());
    }
    else
    {
      if (EnsureLocationWhenWorkFromCwd(FilePath))
      {
        FFileZillaIntf->Delete(FileNameOnly.c_str(), L"", true);
      }
      else
      {
        FFileZillaIntf->Delete(FileNameOnly.c_str(), FilePath.c_str(), false);
      }
    }
    GotReply(WaitForCommandReply(), REPLY_2XX_CODE);
  }
}

void TFTPFileSystem::CustomCommandOnFile(const UnicodeString & /*AFileName*/,
  const TRemoteFile * /*AFile*/, const UnicodeString & /*ACommand*/, int32_t /*AParams*/,
  TCaptureOutputEvent && /*OutputEvent*/)
{
  // if ever implemented, do not forget to add EnsureLocation,
  // see AnyCommand for a reason why
  DebugFail();
}

void TFTPFileSystem::DoStartup()
{
  std::unique_ptr<TStrings> PostLoginCommands(std::make_unique<TStringList>());
  try__finally
  {
    PostLoginCommands->SetText(FTerminal->GetSessionData()->GetPostLoginCommands());
    for (int32_t Index = 0; Index < PostLoginCommands->GetCount(); ++Index)
    {
      UnicodeString Command = PostLoginCommands->GetString(Index);
      if (!Command.IsEmpty())
      {
        SendCommand(Command);

        GotReply(WaitForCommandReply(), REPLY_2XX_CODE | REPLY_3XX_CODE);
      }
    }
  }
  __finally__removed
  {
    // delete PostLoginCommands;
  } end_try__finally

  if (SupportsCommand(CsidCommand))
  {
    const UnicodeString NameFact = L"Name";
    const UnicodeString VersionFact = L"Version";
    const UnicodeString Command =
      FORMAT(L"%s %s=%s;%s=%s", CsidCommand, NameFact, GetAppNameString(), VersionFact, FTerminal->Configuration->Version);
    SendCommand(Command);
    TStrings * Response = nullptr;
    GotReply(WaitForCommandReply(), REPLY_2XX_CODE, EmptyStr, nullptr, &Response);
    std::unique_ptr<TStrings> ResponseOwner(Response);
    // Not using REPLY_SINGLE_LINE to make it robust
    if (Response->Count == 1)
    {
      UnicodeString ResponseText = Response->Strings[0];
      UnicodeString Name, Version;
      while (!ResponseText.IsEmpty())
      {
        UnicodeString Token = CutToChar(ResponseText, L';', true);
        UnicodeString Fact = CutToChar(Token, L'=', true);
        if (SameText(Fact, NameFact))
        {
          Name = Token;
        }
        else if (SameText(Fact, VersionFact))
        {
          Version = Token;
        }
      }

      if (!Name.IsEmpty())
      {
        FServerID = Name;
        AddToList(FServerID, Version, L" ");
        FTerminal->LogEvent(FORMAT("Server: %s", FServerID));
      }
    }
  }

  // retrieve initialize working directory to save it as home directory
  ReadCurrentDirectory();
  FHomeDirectory = FCurrentDirectory;
}

void TFTPFileSystem::HomeDirectory()
{
  // FHomeDirectory is an absolute path, so avoid unnecessary overhead
  // of ChangeDirectory, such as EnsureLocation
  DoChangeDirectory(FHomeDirectory);
  FCurrentDirectory = FHomeDirectory;
  FReadCurrentDirectory = false;
  // make sure FZAPI is aware that we changed current working directory
  FFileZillaIntf->SetCurrentPath(FCurrentDirectory.c_str());
}

bool TFTPFileSystem::IsCapable(int32_t Capability) const
{
  DebugAssert(FTerminal);
  switch (Capability)
  {
    case fcResolveSymlink: // sic
    case fcTextMode:
    case fcModeChanging: // but not fcModeChangingUpload
    case fcNewerOnlyUpload:
    case fcAnyCommand: // but not fcShellAnyCommand
    case fcRename:
    case fcRemoteMove:
    case fcRemoveBOMUpload:
    case fcMoveToQueue:
    case fcSkipTransfer:
    case fcParallelTransfers:
    case fcTransferOut:
    case fcTransferIn:
      return true;

    case fcPreservingTimestampUpload:
      return (FServerCapabilities->GetCapability(mfmt_command) == yes);

    case fcRemoteCopy:
      return SupportsSiteCommand(CopySiteCommand);

    case fcSymbolicLink:
      return SupportsSiteCommand(SymlinkSiteCommand);

    case fcCalculatingChecksum:
      return FSupportsAnyChecksumFeature;

    case fcCheckingSpaceAvailable:
      return FBytesAvailableSupported || SupportsCommand(AvblCommand) || SupportsCommand(XQuotaCommand);

    case fcMoveOverExistingFile:
      return !FIIS;

    case fcAclChangingFiles:
    case fcModeChangingUpload:
    case fcLoadingAdditionalProperties:
    case fcShellAnyCommand:
    case fcHardLink:
    case fcUserGroupListing:
    case fcGroupChanging:
    case fcOwnerChanging:
    case fcGroupOwnerChangingByID:
    case fcSecondaryShell:
    case fcNativeTextMode:
    case fcTimestampChanging:
    case fcIgnorePermErrors:
    case fcRemoveCtrlZUpload:
    case fcLocking:
    case fcPreservingTimestampDirs:
    case fcResumeSupport:
    case fcChangePassword:
    case fcParallelFileTransfers:
      return false;

    default:
      DebugFail();
      return false;
  }
}

void TFTPFileSystem::LookupUsersGroups()
{
  DebugFail();
}

void TFTPFileSystem::ReadCurrentDirectory()
{
  // ask the server for current directory on startup only
  // and immediately after call to CWD,
  // later our current directory may be not synchronized with FZAPI current
  // directory anyway, see comments in EnsureLocation
  if (FReadCurrentDirectory || DebugAlwaysFalse(FCurrentDirectory.IsEmpty()))
  {
    const UnicodeString Command = "PWD";
    SendCommand(Command);

    uint32_t Code = 0;
    TStrings * Response = nullptr;
    GotReply(WaitForCommandReply(), REPLY_2XX_CODE, "", &Code, &Response);

    std::unique_ptr<TStrings> ResponsePtr(Response);
    try__finally
    {
      DebugAssert(ResponsePtr != nullptr);
      bool Result = false;

      // The 257 is the only allowed 2XX code to "PWD"
      if (((Code == 257) || FTerminal->SessionData->FFtpAnyCodeForPwd) &&
          (ResponsePtr->GetCount() == 1))
      {
        UnicodeString Path = ResponsePtr->GetText();

        int32_t P = Path.Pos(L"\"");
        if (P == 0)
        {
          // some systems use single quotes, be tolerant
          P = Path.Pos(L"'");
        }
        if (P != 0)
        {
          Path.Delete(1, P - 1);

          if (Unquote(Path))
          {
            Result = true;
          }
        }
        else
        {
          P = Path.Pos(L" ");
          Path.Delete(P, Path.Length() - P + 1);
          Result = true;
        }

        if (Result)
        {
          if ((Path.Length() > 0) && !base::UnixIsAbsolutePath(Path))
          {
            Path = UnicodeString(ROOTDIRECTORY) + Path;
          }
          FCurrentDirectory = base::UnixExcludeTrailingBackslash(Path);
          FReadCurrentDirectory = false;
        }
      }

      if (Result)
      {
        FFileZillaIntf->SetCurrentPath(FCurrentDirectory.c_str());
      }
      else
      {
        throw Exception(FMTLOAD(FTP_RESPONSE_ERROR, Command, Response->GetText()));
      }
    }
    __finally__removed
    {
      // delete Response;
    } end_try__finally
  }
}

void TFTPFileSystem::DoReadDirectory(TRemoteFileList * AFileList)
{
  UnicodeString Directory;
  if (!EnsureLocationWhenWorkFromCwd(AFileList->Directory()))
  {
    Directory = GetAbsolutePath(AFileList->Directory(), false);
  }

  FBytesAvailable = -1;
  AFileList->Reset();
  // FZAPI does not list parent directory, add it
  AFileList->AddFile(new TRemoteParentDirectory(FTerminal));

  FLastReadDirectoryProgress = 0;

  const TFTPFileListHelper Helper(this, AFileList, false); nb::used(Helper);

  // always specify path to list, do not attempt to list "current" dir as:
  // 1) List() lists again the last listed directory, not the current working directory
  // 2) we handle this way the cached directory change
  FFileZillaIntf->List(Directory.c_str());

  GotReply(WaitForCommandReply(), REPLY_2XX_CODE | REPLY_ALLOW_CANCEL);

  AutoDetectTimeDifference(AFileList);

  if (!IsEmptyFileList(AFileList))
  {
    CheckTimeDifference();

    if ((FTimeDifference != 0) || !FUploadedTimes.empty())// optimization
    {
      for (int32_t Index = 0; Index < AFileList->GetCount(); ++Index)
      {
        ApplyTimeDifference(AFileList->GetFile(Index));
      }
    }
  }

  FLastDataSent = Now();
  FAnyTransferSucceeded = true;
}

void TFTPFileSystem::CheckTimeDifference()
{
  if (NeedAutoDetectTimeDifference())
  {
    FTerminal->LogEvent("Warning: Timezone difference was not detected yet, timestamps may be incorrect");
  }
}

void TFTPFileSystem::ApplyTimeDifference(TRemoteFile * File)
{
  DebugAssert(File->GetModification() == File->GetLastAccess());
  File->ShiftTimeInSeconds(FTimeDifference);

  TDateTime Modification = File->GetModification();
  if (LookupUploadModificationTime(File->GetFullFileName(), Modification, File->GetModificationFmt()))
  {
    // implicitly sets ModificationFmt to mfFull
    File->SetModification(Modification);
  }
}

void TFTPFileSystem::ApplyTimeDifference(
  const UnicodeString & FileName, TDateTime & Modification, TModificationFmt & ModificationFmt)
{
  CheckTimeDifference();
  TRemoteFile::ShiftTimeInSeconds(Modification, ModificationFmt, FTimeDifference);

  if (LookupUploadModificationTime(FileName, Modification, ModificationFmt))
  {
    ModificationFmt = mfFull;
  }
}

bool TFTPFileSystem::LookupUploadModificationTime(
  const UnicodeString & AFileName, TDateTime & Modification, TModificationFmt ModificationFmt)
{
  bool Result = false;
  if (ModificationFmt != mfFull)
  {
    const UnicodeString AbsPath = GetAbsolutePath(AFileName, false);
    const TUploadedTimes::const_iterator Iterator = FUploadedTimes.find(AbsPath);
    if (Iterator != FUploadedTimes.end())
    {
      const TDateTime UploadModification = Iterator->second;
      const TDateTime UploadModificationReduced = base::ReduceDateTimePrecision(UploadModification, ModificationFmt);
      if (UploadModificationReduced == Modification)
      {
        if ((FTerminal->GetConfiguration()->GetActualLogProtocol() >= 2))
        {
          FTerminal->LogEvent(
            FORMAT("Enriching modification time of \"%s\" from [%s] to [%s]",
                   AFileName, StandardTimestamp(Modification), StandardTimestamp(UploadModification)));
        }
        Modification = UploadModification;
        Result = true;
      }
      else
      {
        if ((FTerminal->GetConfiguration()->GetActualLogProtocol() >= 2))
        {
          FTerminal->LogEvent(
            FORMAT("Remembered modification time [%s]/[%s] of \"%s\" is obsolete, keeping [%s]",
                   StandardTimestamp(UploadModification), StandardTimestamp(UploadModificationReduced), AFileName, StandardTimestamp(Modification)));
        }
        FUploadedTimes.erase(AbsPath);
      }
    }
  }

  return Result;
}

bool TFTPFileSystem::NeedAutoDetectTimeDifference() const
{
  return
    FDetectTimeDifference &&
    // Does not support MLST/MLSD, but supports MDTM at least
    !FFileZillaIntf->UsingMlsd() && SupportsReadingFile();
}

bool TFTPFileSystem::IsEmptyFileList(const TRemoteFileList * FileList) const
{
  return
    // (note that it's actually never empty here, there's always at least parent directory,
    // added explicitly by DoReadDirectory)
    (FileList->GetCount() == 0) ||
    ((FileList->GetCount() == 1) && FileList->GetFile(0)->GetIsParentDirectory());
}

void TFTPFileSystem::AutoDetectTimeDifference(const TRemoteFileList * FileList)
{
  if (NeedAutoDetectTimeDifference())
  {
    FTerminal->LogEvent("Detecting timezone difference...");

    for (int32_t Index = 0; Index < FileList->GetCount(); ++Index)
    {
      const TRemoteFile * File = FileList->GetFile(Index);
      // For directories, we do not do MDTM in ReadFile
      // (it should not be problem to use them otherwise).
      // We are also not interested in files with day precision only.
      if (!File->GetIsDirectory() && !File->GetIsSymLink() &&
        File->GetIsTimeShiftingApplicable())
      {
        std::unique_ptr<TRemoteFile> UtcFilePtr;
        try
        {
          TRemoteFile * UtcFile = nullptr;
          ReadFile(File->GetFullFileName(), UtcFile);
          UtcFilePtr.reset(UtcFile);
        }
        catch (Exception & /*E*/)
        {
          FDetectTimeDifference = false;
          if (!FTerminal->GetActive())
          {
            throw;
          }
          FTerminal->LogEvent(FORMAT("Failed to retrieve file %s attributes to detect timezone difference", File->GetFullFileName()));
          break;
        }

        TDateTime UtcModification = UtcFilePtr->GetModification();
        UtcFilePtr.reset();

        if (UtcModification > Now())
        {
          FTerminal->LogEvent(
            FORMAT("Not using file %s to detect timezone difference as it has the timestamp in the future [%s]",
              File->GetFullFileName(), StandardTimestamp(UtcModification)));
        }
        // "SecureLink FTP Proxy" succeeds CWD for a file, so we never get a timestamp here
        else if (UtcModification == TDateTime())
        {
          FTerminal->LogEvent(
            FORMAT("Not using file %s to detect timezone difference as its timestamp was not resolved",
              File->FullFileName));
        }
        else
        {
          FDetectTimeDifference = false;

          // MDTM returns seconds, trim those
          UtcModification = base::ReduceDateTimePrecision(UtcModification, File->GetModificationFmt());

          // Time difference between timestamp retrieved using MDTM (UTC converted to local timezone)
          // and using LIST (no conversion, expecting the server uses the same timezone as the client).
          // Note that FormatTimeZone reverses the value.
          FTimeDifference = nb::ToInt64(SecsPerDay * (UtcModification - File->GetModification()));
          const double Hours = TTimeSpan::FromSeconds(nb::ToDouble(FTimeDifference)).GetTotalHours();

          UnicodeString FileLog =
            FORMAT("%s (Listing: %s, UTC: %s)", File->GetFullFileName(), StandardTimestamp(File->GetModification()), StandardTimestamp(UtcModification));
          UnicodeString LogMessage;
          if (FTimeDifference == 0)
          {
            LogMessage = FORMAT("No timezone difference detected using file %s", FileLog);
          }
          // Seen with "GamingDeluxe FTP Server", which returns "213 00010101000000"
          else if (fabs(Hours) >= 48)
          {
            FTimeDifference = 0;
            LogMessage = FORMAT(L"Ignoring suspicious timezone difference of %s hours, detected using file %s", IntToStr(nb::ToInt32(Hours)), FileLog);
          }
          else
          {
            LogMessage = FORMAT("Timezone difference of %s detected using file %s", FormatTimeZone(nb::ToInt32(FTimeDifference)), FileLog);
          }
          FTerminal->LogEvent(LogMessage);

          break;
        }
      }
    }

    if (FDetectTimeDifference)
    {
      FTerminal->LogEvent("Found no file to use for detecting timezone difference");
    }
  }
}

void TFTPFileSystem::AutoDetectTimeDifference(
  const UnicodeString & ADirectory, const TCopyParamType * CopyParam, int32_t Params)
{
  if (NeedAutoDetectTimeDifference() &&
      // do we need FTimeDifference for the operation?
      // (tmAutomatic - AsciiFileMask can theoretically include time constraints, while it is unlikely)
      (!FLAGSET(Params, cpNoConfirmation) ||
       CopyParam->GetNewerOnly() || (!(CopyParam->GetTransferMode() == tmAutomatic)) || !CopyParam->GetIncludeFileMask().Masks().IsEmpty()))
  {
    FTerminal->LogEvent("Retrieving listing to detect timezone difference");
    DummyReadDirectory(ADirectory);
  }
}

void TFTPFileSystem::ReadDirectory(TRemoteFileList * FileList)
{
  // whole below "-a" logic is for LIST,
  // if we know we are going to use MLSD, skip it
  if (FFileZillaIntf->UsingMlsd())
  {
    DoReadDirectory(FileList);
  }
  else
  {
    bool GotNoFilesForAll = false;
    bool Repeat;

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
          DebugAssert(FListAll == asOff);
          FListAll = asAuto;
        }
        else if (FListAll == asAuto)
        {
          // some servers take "-a" as a mask and return empty directory listing
          if (IsEmptyFileList(FileList))
          {
            Repeat = true;
            FListAll = asOff;
            GotNoFilesForAll = true;
            FTerminal->LogEvent("LIST with -a switch returned empty directory listing, will try pure LIST");
          }
          else
          {
            // reading first directory has succeeded, always use "-a"
            FListAll = asOn;
          }
        }

        // use "-a" even for implicit directory reading by FZAPI?
        // (e.g. before file transfer)
        // Note that FZAPI ignores this for VMS/MVS.
        FDoListAll = (FListAll == asOn);
      }
      catch (Exception &)
      {
        FDoListAll = false;
        // reading the first directory has failed,
        // further try without "-a" only as the server may not support it
        if (FListAll == asAuto)
        {
          FTerminal->LogEvent("LIST with -a failed, will try pure LIST");
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
}

void TFTPFileSystem::DoReadFile(const UnicodeString & AFileName,
  TRemoteFile *& AFile)
{
  const UnicodeString FileName = GetAbsolutePath(AFileName, false);
  UnicodeString FileNameOnly;
  UnicodeString FilePath;
  if (base::IsUnixRootPath(FileName))
  {
    FileNameOnly = FileName;
    FilePath = FileName;
  }
  else
  {
    FileNameOnly = base::UnixExtractFileName(FileName);
    FilePath = RemoteExtractFilePath(FileName);
  }

  std::unique_ptr<TRemoteFileList> FileList(std::make_unique<TRemoteFileList>());
  try__finally
  {
    // Duplicate() call below would use this to compose FullFileName
    FileList->SetDirectory(FilePath);
    const TFTPFileListHelper Helper(this, FileList.get(), false); nb::used(Helper);
    FFileZillaIntf->ListFile(FileNameOnly.c_str(), FilePath.c_str());

    GotReply(WaitForCommandReply(), REPLY_2XX_CODE | REPLY_ALLOW_CANCEL);
    const TRemoteFile * File = FileList->FindFile(FileNameOnly.c_str());
    if (File != nullptr)
    {
      AFile = File->Duplicate();
    }

    FLastDataSent = Now();
  }
  __finally__removed
  {
    // delete FileList;
  } end_try__finally
}

bool TFTPFileSystem::SupportsReadingFile() const
{
  return
    FFileZillaIntf->UsingMlsd() ||
    (SupportsCommand(MdtmCommand) && SupportsCommand(SizeCommand));
}

void TFTPFileSystem::ReadFile(const UnicodeString & AFileName,
  TRemoteFile *& AFile)
{
  AFile = nullptr;
  if (SupportsReadingFile())
  {
    DoReadFile(AFileName, AFile);
  }
  else
  {
    if (base::IsUnixRootPath(AFileName))
    {
      FTerminal->LogEvent(FORMAT("%s is a root path", AFileName));
      AFile = new TRemoteDirectoryFile();
      AFile->SetFullFileName(AFileName);
      AFile->SetFileName(L"");
    }
    else
    {
      const UnicodeString Path = RemoteExtractFilePath(AFileName);
      UnicodeString NameOnly;
      int32_t P = 0;
      const bool MVSPath =
        FMVS && Path.IsEmpty() &&
        (AFileName.SubString(1, 1) == L"'") && (AFileName.SubString(AFileName.Length(), 1) == L"'") &&
        ((P = AFileName.Pos(L".")) > 0);
      if (!MVSPath)
      {
        NameOnly = base::UnixExtractFileName(AFileName);
      }
      else
      {
        NameOnly = AFileName.SubString(P + 1, AFileName.Length() - P - 1);
      }
      DebugAssert(!FVMSAllRevisions);
      TAutoFlag VMSAllRevisionsFlag(FVMSAllRevisions);
      if (FVMS && (NameOnly.Pos(L";") > 2))
      {
        FTerminal->LogEvent(L"VMS versioned file detected, asking for all revisions");
        FVMSAllRevisions = true;
      }
      TRemoteFile * File = nullptr;
      // FZAPI does not have efficient way to read properties of one file.
      // In case we need properties of set of files from the same directory,
      // cache the file list for future
      if ((FFileListCache != nullptr) &&
          base::UnixSamePath(Path, FFileListCache->GetDirectory()) &&
          (base::UnixIsAbsolutePath(FFileListCache->GetDirectory()) ||
           (FFileListCachePath == RemoteGetCurrentDirectory())))
      {
        File = FFileListCache->FindFile(NameOnly);
      }
      // if cache is invalid or file is not in cache, (re)read the directory
      if (File == nullptr)
      {
        std::unique_ptr<TRemoteFileList> FileListCache(std::make_unique<TRemoteFileList>());
        FileListCache->SetDirectory(Path);
        try__catch
        {
          ReadDirectory(FileListCache.get());
        }
        __catch__removed
        {
          // delete FileListCache;
          // throw;
        } end_try__catch
        // set only after we successfully read the directory,
        // otherwise, when we reconnect from ReadDirectory,
        // the FFileListCache is reset from ResetCache.
        SAFE_DESTROY(FFileListCache);
        FFileListCache = FileListCache.release();
        FFileListCachePath = RemoteGetCurrentDirectory();

        File = FFileListCache->FindFile(NameOnly);
      }
      VMSAllRevisionsFlag.Release();

      if (File != nullptr)
      {
        AFile = File->Duplicate();
        if (MVSPath)
        {
          AFile->SetFileName(AFileName);
          AFile->SetFullFileName(AFileName);
        }
        if (File->IsSymLink)
        {
          TAutoFlag AutoFlag(FForceReadSymlink);
          File->Complete();
        }
      }
    }
  }

  if (AFile == nullptr)
  {
    throw Exception(FMTLOAD(FILE_NOT_EXISTS, AFileName));
  }
}

void TFTPFileSystem::ReadSymlink(TRemoteFile * SymlinkFile,
  TRemoteFile *& AFile)
{
  if (FForceReadSymlink && DebugAlwaysTrue(!SymlinkFile->LinkTo.IsEmpty()) && DebugAlwaysTrue(SymlinkFile->GetHaveFullFileName()))
  {
    // When we get here from TFTPFileSystem::ReadFile, it's likely the second time ReadSymlink has been called for the link.
    // The first time getting to the later branch, so IsDirectory is true and hence FullFileName ends with a slash.
    const UnicodeString SymlinkDir = base::UnixExtractFileDir(base::UnixExcludeTrailingBackslash(SymlinkFile->FullFileName()));
    const UnicodeString LinkTo = base::AbsolutePath(SymlinkDir, SymlinkFile->LinkTo);
    ReadFile(LinkTo, AFile);
  }
  else
  {
    // Resolving symlinks over FTP is big overhead
    // (involves opening TCPIP connection for retrieving "directory listing").
    // Moreover FZAPI does not support that anyway.
    // Though nowadays we could use MLST to read the symlink.
    std::unique_ptr<TRemoteFile> File(std::make_unique<TRemoteFile>(SymlinkFile));
    File->Terminal = FTerminal;
    File->FileName = base::UnixExtractFileName(SymlinkFile->LinkTo);
    // FZAPI treats all symlink target as directories
    File->SetType(FILETYPE_SYMLINK); // FILETYPE_DIRECTORY
    AFile = File.release();
  }
}

void TFTPFileSystem::RemoteRenameFile(
  const UnicodeString & AFileName, const TRemoteFile * /*AFile*/, const UnicodeString & ANewName, bool DebugUsedArg(Overwrite))
{
  const UnicodeString FileName = GetAbsolutePath(AFileName, false);
  const UnicodeString NewName = GetAbsolutePath(ANewName, false);

  const UnicodeString FileNameOnly = base::UnixExtractFileName(FileName);
  const UnicodeString FilePathOnly = RemoteExtractFilePath(FileName);
  const UnicodeString NewNameOnly = base::UnixExtractFileName(NewName);
  const UnicodeString NewPathOnly = RemoteExtractFilePath(NewName);

  {
    // ignore file list
    TFTPFileListHelper Helper(this, nullptr, true); nb::used(Helper);

    FFileZillaIntf->Rename(FileNameOnly.c_str(), NewNameOnly.c_str(),
      FilePathOnly.c_str(), NewPathOnly.c_str());

    GotReply(WaitForCommandReply(), REPLY_2XX_CODE);
  }
}

void TFTPFileSystem::RemoteCopyFile(
  const UnicodeString & AFileName, const TRemoteFile * /*AFile*/, const UnicodeString & ANewName, bool DebugUsedArg(Overwrite))
{
  DebugAssert(SupportsSiteCommand(CopySiteCommand));
  EnsureLocation();

  UnicodeString Command;

  Command = FORMAT("%s CPFR %s", SiteCommand, AFileName);
  SendCommand(Command);
  GotReply(WaitForCommandReply(), REPLY_3XX_CODE);

  Command = FORMAT("%s CPTO %s", SiteCommand, ANewName);
  SendCommand(Command);
  GotReply(WaitForCommandReply(), REPLY_2XX_CODE);
}

TStrings * TFTPFileSystem::GetFixedPaths() const
{
  return nullptr;
}

void TFTPFileSystem::SpaceAvailable(const UnicodeString & APath,
  TSpaceAvailable & ASpaceAvailable)
{
  if (FBytesAvailableSupported)
  {
    std::unique_ptr<TRemoteFileList> DummyFileList(std::make_unique<TRemoteFileList>());
    DummyFileList->SetDirectory(APath);
    ReadDirectory(DummyFileList.get());
    ASpaceAvailable.UnusedBytesAvailableToUser = FBytesAvailable;
  }
  else if (SupportsCommand(XQuotaCommand))
  {
    // WS_FTP:
    // XQUOTA
    // 213-File and disk usage
    //     File count: 3
    //     File limit: 10000
    //     Disk usage: 1532791
    //     Disk limit: 2048000
    // 213 File and disk usage end

    // XQUOTA is global not path-specific
    const UnicodeString Command = XQuotaCommand;
    SendCommand(Command);
    TStrings * Response = nullptr;
    GotReply(WaitForCommandReply(), REPLY_2XX_CODE, L"", nullptr, &Response);
    std::unique_ptr<TStrings> ResponseOwner(Response);

    int64_t UsedBytes = -1;
    for (int32_t Index = 0; Index < Response->GetCount(); Index++)
    {
      // trimming padding
      UnicodeString Line = Trim(Response->GetString(Index));
      UnicodeString Label = CutToChar(Line, L':', true);
      if (SameText(Label, L"Disk usage"))
      {
        UsedBytes = ::StrToInt64(Line);
      }
      else if (SameText(Label, L"Disk limit") && !SameText(Line, L"unlimited"))
      {
        ASpaceAvailable.BytesAvailableToUser = ::StrToInt64(Line);
      }
    }

    if ((UsedBytes >= 0) && (ASpaceAvailable.BytesAvailableToUser > 0))
    {
      ASpaceAvailable.UnusedBytesAvailableToUser = ASpaceAvailable.BytesAvailableToUser - UsedBytes;
    }
  }
  else if (SupportsCommand(AvblCommand))
  {
    // draft-peterson-streamlined-ftp-command-extensions-10
    // Implemented by Serv-U.
    const UnicodeString Command = FORMAT("%s %s", AvblCommand, APath);
    SendCommand(Command);
    const UnicodeString Response = GotReply(WaitForCommandReply(), REPLY_2XX_CODE | REPLY_SINGLE_LINE);
    ASpaceAvailable.UnusedBytesAvailableToUser = ::StrToInt64(Response);
  }
}

const TSessionInfo & TFTPFileSystem::GetSessionInfo() const
{
  return FSessionInfo;
}

const TFileSystemInfo & TFTPFileSystem::GetFileSystemInfo(bool /*Retrieve*/)
{
  if (!FFileSystemInfoValid)
  {
    UnicodeString RemoteSystem = FSystem;
    AddToList(RemoteSystem, FServerID, L", ");
    RemoteSystem.Unique();
    FFileSystemInfo.RemoteSystem = RemoteSystem;

    if (FFeatures->GetCount() == 0)
    {
      FFileSystemInfo.AdditionalInfo = LoadStr(FTP_NO_FEATURE_INFO);
    }
    else
    {
      FFileSystemInfo.AdditionalInfo =
        FORMAT("%s\r\n", LoadStr(FTP_FEATURE_INFO));
      for (int32_t Index = 0; Index < FFeatures->GetCount(); ++Index)
      {
        FFileSystemInfo.AdditionalInfo += FORMAT("  %s\r\n", FFeatures->GetString(Index));
      }
    }

    FTerminal->SaveCapabilities(FFileSystemInfo);

    FFileSystemInfoValid = true;
  }
  return FFileSystemInfo;
}

bool TFTPFileSystem::TemporaryTransferFile(const UnicodeString & /*FileName*/)
{
  return false;
}

bool TFTPFileSystem::GetStoredCredentialsTried() const
{
  return FStoredPasswordTried;
}

UnicodeString TFTPFileSystem::RemoteGetUserName() const
{
  return FUserName;
}

UnicodeString TFTPFileSystem::RemoteGetCurrentDirectory() const
{
  return FCurrentDirectory;
}

const wchar_t * TFTPFileSystem::GetOption(int32_t OptionID) const
{
  const TSessionData * Data = FTerminal->GetSessionData();

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
      FOptionScratch.Clear();
      break;

    case OPTION_MPEXT_CERT_STORAGE:
      FOptionScratch = FTerminal->Configuration->GetCertificateStorageExpanded();
      break;

    default:
      DebugFail();
      FOptionScratch.Clear();
  }

  return FOptionScratch.c_str();
}

int32_t TFTPFileSystem::GetOptionVal(int32_t OptionID) const
{
  const TSessionData * Data = FTerminal ? FTerminal->GetSessionData() : nullptr;
  const TConfiguration * Configuration = FTerminal ? FTerminal->GetConfiguration() : nullptr;
  int32_t Result;
  TProxyMethod method;

  switch (OptionID)
  {
    case OPTION_PROXYTYPE:
      method = Data ? Data->GetActualProxyMethod() : pmNone;
      // DEBUG_PRINTF("method: %d", method);
      switch (method)
      {
        case pmNone:
          Result = 0; // PROXYTYPE_NOPROXY
          break;

        case pmSocks4:
          Result = 2; // PROXYTYPE_SOCKS4A
          break;

        case pmSocks5:
          Result = 3; // PROXYTYPE_SOCKS5
          break;

        case pmHTTP:
          Result = 4; // PROXYTYPE_HTTP11
          break;

        case pmTelnet:
        case pmCmd:
        default:
          DebugFail();
          Result = 0; // PROXYTYPE_NOPROXY
          break;
      }
      break;

    case OPTION_PROXYPORT:
    case OPTION_FWPORT:
      Result = Data ? Data->GetProxyPort() : 0;
      break;

    case OPTION_PROXYUSELOGON:
      Result = Data ? !Data->GetProxyUsername().IsEmpty() : 0;
      break;

    case OPTION_LOGONTYPE:
      Result = Data ? Data->GetFtpProxyLogonType() : 0;
      break;

    case OPTION_TIMEOUTLENGTH:
      Result = Data ? Data->GetTimeout() : 0;
      break;

    case OPTION_DEBUGSHOWLISTING:
      Result = Configuration ? (Configuration->ActualLogProtocol >= 0) : 0;
      break;

    case OPTION_PASV:
      // should never get here t_server.nPasv being nonzero
      DebugFail();
      Result = FALSE;
      break;

    case OPTION_PRESERVEDOWNLOADFILETIME:
    case OPTION_MPEXT_PRESERVEUPLOADFILETIME:
      Result = FFileTransferPreserveTime ? TRUE : FALSE;
      break;

    case OPTION_LIMITPORTRANGE:
      Result = Data && Configuration ? !Data->FFtpPasvMode && Configuration->HasLocalPortNumberLimits() : 0;
      break;

    case OPTION_PORTRANGELOW:
      Result = Configuration ? Configuration->FLocalPortNumberMin : 0;
      break;

    case OPTION_PORTRANGEHIGH:
      Result = Configuration ? Configuration->FLocalPortNumberMax : 0;
      break;

    case OPTION_ENABLE_IPV6:
      Result = Data ? ((Data->GetAddressFamily() != afIPv4) ? TRUE : FALSE) : 0;
      break;

    case OPTION_KEEPALIVE:
      /*
      FIXME: Data may be NULL when called from CFtpControlSocket::OnTimer
              in the data load thread (when the panel is closed immediately after
              the end of download operations). On the good, it would be necessary
              to "kill" the thread (or, although close the socket), but as a
              temporary measure...
      */
      Result = Data ? ((Data->GetFtpPingType() != ptOff) ? TRUE : FALSE) : 0;
      break;

    case OPTION_INTERVALLOW:
    case OPTION_INTERVALHIGH:
      Result = Data ? Data->GetFtpPingInterval() : 0;
      break;

    case OPTION_VMSALLREVISIONS:
      Result = FVMSAllRevisions ? TRUE : FALSE;
      break;

    case OPTION_SPEEDLIMIT_DOWNLOAD_TYPE:
    case OPTION_SPEEDLIMIT_UPLOAD_TYPE:
      Result = (FFileTransferCPSLimit == 0 ? 0 : 1);
      break;

    case OPTION_SPEEDLIMIT_DOWNLOAD_VALUE:
    case OPTION_SPEEDLIMIT_UPLOAD_VALUE:
      Result = nb::ToInt32((FFileTransferCPSLimit / 1024)); // FZAPI expects KB/s
      break;

    case OPTION_MPEXT_SHOWHIDDEN:
      Result = (FDoListAll ? TRUE : FALSE);
      break;

    case OPTION_MPEXT_SSLSESSIONREUSE:
      Result = Data ? (Data->GetSslSessionReuse() ? TRUE : FALSE) : 0;
      break;

    case OPTION_MPEXT_SNDBUF:
      Result = Data ? Data->GetSendBuf() : 0;
      break;

    case OPTION_MPEXT_TRANSFER_ACTIVE_IMMEDIATELY:
      Result = FTransferActiveImmediately;
      break;

    case OPTION_MPEXT_REMOVE_BOM:
      Result = FFileTransferRemoveBOM ? TRUE : FALSE;
      break;

    case OPTION_MPEXT_LOG_SENSITIVE:
      Result = Configuration ? Configuration->GetLogSensitive() ? TRUE : FALSE : 0;
      break;

    case OPTION_MPEXT_HOST:
      Result = Data ? (Data->GetFtpHost() == asOn) : 0;
      break;

    case OPTION_MPEXT_NODELAY:
      Result = Data ? Data->GetTcpNoDelay() : 0;
      break;

    case OPTION_MPEXT_NOLIST:
      Result = FFileTransferNoList ? TRUE : FALSE;
      break;

    case OPTION_MPEXT_COMPLETE_TLS_SHUTDOWN:
      // As of FileZilla Server 1.6.1 this does not seem to be needed. It's still needed with 1.5.1.
      // It was possibly fixed by 1.6.0 (2022-12-06) change:
      // Fixed an issue in the networking code when dealing with TLS close_notify alerts
      Result = FFileZilla ? FALSE : TRUE;
      break;

    case OPTION_MPEXT_WORK_FROM_CWD:
      Result = (FWorkFromCwd == asOn);
      break;

    case OPTION_MPEXT_TRANSFER_SIZE:
      {
        int64_t TransferSize = 0;
        if (FTerminal && (FTerminal->OperationProgress != nullptr) &&
            (FTerminal->OperationProgress->Operation == foCopy) &&
            (FTerminal->OperationProgress->Side == osLocal))
        {
          TransferSize = FTerminal->OperationProgress->TransferSize - FTerminal->OperationProgress->TransferredSize;
        }
        Result = nb::ToInt32(nb::ToUInt32(TransferSize & std::numeric_limits<uint32_t>::max()));
      }
      break;

    default:
      DebugFail();
      Result = FALSE;
      break;
  }

  return Result;
}

bool TFTPFileSystem::FTPPostMessage(uint32_t Type, WPARAM wParam, LPARAM lParam)
{
  if (Type == TFileZillaIntf::MSG_TRANSFERSTATUS)
  {
    // Stop here if FileTransferProgress is proceeding,
    // it makes "pause" in queue work.
    // Paused queue item stops in some of the TFileOperationProgressType
    // methods called from FileTransferProgress
    TGuard Guard(FTransferStatusCriticalSection); nb::used(Guard);
  }

  TGuard Guard(FQueueCriticalSection); nb::used(Guard);

  FQueue->push_back(TMessageQueue::value_type(wParam, lParam));
  ::SetEvent(FQueueEvent);

  return true;
}

bool TFTPFileSystem::ProcessMessage()
{
  bool Result;
  TMessageQueue::value_type Message;

  {
    TGuard Guard(FQueueCriticalSection); nb::used(Guard);

    Result = !FQueue->empty();
    if (Result)
    {
      Message = FQueue->front();
      FQueue->erase(FQueue->begin());
    }
    else
    {
      // now we are perfectly sure that the queue is empty as it is locked,
      // so reset the event
      ::ResetEvent(FQueueEvent);
    }
  }

  if (Result)
  {
    FFileZillaIntf->HandleMessage(Message.wparam, Message.lparam);
  }

  return Result;
}

void TFTPFileSystem::DiscardMessages()
{
  try__finally
  {
    while (ProcessMessage());
  }
  __finally
  {
    FReply = 0;
    FCommandReply = 0;
  } end_try__finally
}

void TFTPFileSystem::WaitForMessages()
{
  DWORD Result;
  do
  {
    Result = ::WaitForSingleObject(FQueueEvent, GUIUpdateInterval);
    FTerminal->ProcessGUI();
  }
  while (Result == WAIT_TIMEOUT);

  if (Result != WAIT_OBJECT_0)
  {
    FTerminal->FatalError(nullptr, FMTLOAD(INTERNAL_ERROR, L"ftp#1", ::IntToStr(Result)));
  }
}

void TFTPFileSystem::PoolForFatalNonCommandReply()
{
  DebugAssert(FReply == 0);
  DebugAssert(FCommandReply == 0);
  DebugAssert(!FWaitingForReply);

  FWaitingForReply = true;

  uint32_t Reply;

  try__finally
  {
    // discard up to one reply
    // (it should not happen here that two replies are posted anyway)
    while (ProcessMessage() && (FReply == 0))
    {
    }
    Reply = FReply;
  }
  __finally
  {
    FReply = 0;
    DebugAssert(FCommandReply == 0);
    FCommandReply = 0;
    DebugAssert(FWaitingForReply);
    FWaitingForReply = false;
  } end_try__finally

  if (Reply != 0)
  {
    // throws
    GotNonCommandReply(Reply);
  }
}

bool TFTPFileSystem::NoFinalLastCode() const
{
  return (FLastCodeClass == 0) || (FLastCodeClass == 1);
}

bool TFTPFileSystem::KeepWaitingForReply(uint32_t & ReplyToAwait, bool WantLastCode) const
{
  // To keep waiting,
  // non-command reply must be unset,
  // the reply we wait for must be unset or
  // last code must be unset (if we wait for it).

  // Though make sure that disconnect makes it through always. As for example when connection is closed already,
  // when sending commands, we may get REPLY_DISCONNECTED as a command response and no other response after,
  // what would cause a hang.
  return
     (FReply == 0) &&
     ((ReplyToAwait == 0) ||
      (WantLastCode && NoFinalLastCode() && FLAGCLEAR(ReplyToAwait, TFileZillaIntf::REPLY_DISCONNECTED)));
}

void TFTPFileSystem::DoWaitForReply(uint32_t &ReplyToAwait, bool WantLastCode)
{
  try
  {
    while (FTerminal && (FTerminal->GetStatus() != ssClosed) && KeepWaitingForReply(ReplyToAwait, WantLastCode))
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

uint32_t TFTPFileSystem::WaitForReply(bool Command, bool WantLastCode)
{
  DebugAssert(FReply == 0);
  DebugAssert(FCommandReply == 0);
  DebugAssert(!FWaitingForReply);

  ResetReply();
  FWaitingForReply = true;

  uint32_t Reply;

  try__finally
  {
    uint32_t & ReplyToAwait = (Command ? FCommandReply : FReply);
    DoWaitForReply(ReplyToAwait, WantLastCode);

    Reply = ReplyToAwait;
  }
  __finally
  {
    FReply = 0;
    FCommandReply = 0;
    DebugAssert(FWaitingForReply);
    FWaitingForReply = false;
  } end_try__finally

  return Reply;
}

uint32_t TFTPFileSystem::WaitForCommandReply(bool WantLastCode)
{
  return WaitForReply(true, WantLastCode);
}

void TFTPFileSystem::WaitForFatalNonCommandReply()
{
  WaitForReply(false, false);
  DebugFail();
}

void TFTPFileSystem::ResetReply()
{
  FLastCode = 0;
  FLastCodeClass = 0;
  FMultiLineResponse = false;
  DebugAssert(FLastResponse != nullptr);
  FLastResponse->Clear();
  DebugAssert(FLastErrorResponse != nullptr);
  FLastErrorResponse->Clear();
  DebugAssert(FLastError != nullptr);
  FLastError->Clear();
}

void TFTPFileSystem::GotNonCommandReply(uint32_t Reply)
{
  DebugAssert(FLAGSET(Reply, TFileZillaIntf::REPLY_DISCONNECTED));
  GotReply(Reply);
  // should never get here as GotReply should raise fatal exception
  DebugFail();
}

void TFTPFileSystem::Disconnect()
{
  Discard();
  FTerminal->Closed();
}

UnicodeString TFTPFileSystem::GotReply(uint32_t Reply, uint32_t Flags,
  const UnicodeString & AError, uint32_t * Code, TStrings ** Response)
{
  UnicodeString Result;
  UnicodeString Error = AError;
  try__finally
  {
    if (FLAGSET(Reply, TFileZillaIntf::REPLY_OK))
    {
      DebugAssert(Reply == TFileZillaIntf::REPLY_OK);

      // With REPLY_2XX_CODE treat "OK" non-2xx code like an error.
      // REPLY_3XX_CODE has to be always used along with REPLY_2XX_CODE.
      if ((FLAGSET(Flags, REPLY_2XX_CODE) && (FLastCodeClass != 2)) &&
          ((FLAGCLEAR(Flags, REPLY_3XX_CODE) || (FLastCodeClass != 3))))
      {
        GotReply(TFileZillaIntf::REPLY_ERROR, Flags, Error);
      }
    }
    else if (FLAGSET(Reply, TFileZillaIntf::REPLY_CANCEL) &&
        FLAGSET(Flags, REPLY_ALLOW_CANCEL))
    {
      DebugAssert(
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
      FTerminal->FatalError(nullptr, FMTLOAD(INTERNAL_ERROR, "ftp#2", FORMAT("0x%x", nb::ToInt32(Reply))));
    }
    else
    {
      // everything else must be an error or disconnect notification
      DebugAssert(
        FLAGSET(Reply, TFileZillaIntf::REPLY_ERROR) ||
        FLAGSET(Reply, TFileZillaIntf::REPLY_DISCONNECTED));

      TODO("REPLY_CRITICALERROR ignored");

      // REPLY_NOTCONNECTED happens if connection is closed between moment
      // when FZAPI interface method dispatches the command to FZAPI thread
      // and moment when FZAPI thread receives the command
      bool Disconnected =
        FLAGSET(Reply, TFileZillaIntf::REPLY_DISCONNECTED) ||
        FLAGSET(Reply, TFileZillaIntf::REPLY_NOTCONNECTED);
      bool DoClose = false;

      UnicodeString HelpKeyword;
      std::unique_ptr<TStrings> MoreMessages(std::make_unique<TStringList>());
      try__catch
      {
        if (Disconnected)
        {
          if (FLAGCLEAR(Flags, REPLY_CONNECT))
          {
            MoreMessages->Add(LoadStr(LOST_CONNECTION));
            Disconnect();
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
          MoreMessages->Add(LoadStr(NOTSUPPORTED));
        }

        if (FLastCode == 530)
        {
          // Serv-U also uses this code in response to "SITE PSWD"
          MoreMessages->Add(LoadStr(AUTHENTICATION_FAILED));
        }

        bool RetryTransfer = false;
        if ((FLastCode == 425) || (FLastCode == 426))
        {
          if (FAnyTransferSucceeded)
          {
            FTerminal->LogEvent(FORMAT(L"Got %d after some previous data connections succeeded, retrying connection", FLastCode));
            RetryTransfer = true;
          }
          else if (!FTerminal->SessionData->FFtpPasvMode)
          {
            MoreMessages->Add(LoadStr(FTP_CANNOT_OPEN_ACTIVE_CONNECTION2));
            HelpKeyword = HELP_FTP_CANNOT_OPEN_ACTIVE_CONNECTION;
          }
        }

        if (FLastCode == DummyTimeoutCode)
        {
          HelpKeyword = HELP_ERRORMSG_TIMEOUT;
        }

        if (FLastCode == DummyDisconnectCode)
        {
          HelpKeyword = HELP_STATUSMSG_DISCONNECTED;
        }

        if (FAnyTransferSucceeded && (FLastError->Count > 0))
        {
          UnicodeString CantOpenTransferChannelMessage = LoadStr(IDS_ERRORMSG_CANTOPENTRANSFERCHANNEL);
          const int32_t P = CantOpenTransferChannelMessage.Pos(L"%");
          if (DebugAlwaysTrue(P > 0))
          {
            CantOpenTransferChannelMessage.SetLength(P - 1);
          }
          if (ContainsText(FLastError->GetString(0), CantOpenTransferChannelMessage))
          {
            FTerminal->LogEvent(L"Failed to connection data connection after some previous data connections succeeded, retrying connection");
            RetryTransfer = true;
          }
        }

        if (RetryTransfer)
        {
          Disconnected = true;
          // Close only later, as we still need to use FLast* fields
          DoClose = true;
        }

        MoreMessages->AddStrings(FLastError.get());
        // already cleared from WaitForReply, but GotReply can be also called
        // from Closed. then make sure that error from previous command not
        // associated with session closure is not reused
        FLastError->Clear();

        MoreMessages->AddStrings(FLastErrorResponse.get());
        // see comment for FLastError
        FLastResponse->Clear();
        FLastErrorResponse->Clear();

        if (MoreMessages->GetCount() == 0)
        {
          MoreMessages.reset();
        }
      }
      __catch__removed
      {
        // delete MoreMessages;
        // throw;
      } end_try__catch

      if (Error.IsEmpty() && (MoreMessages != nullptr))
      {
        DebugAssert(MoreMessages->GetCount() > 0);
        // bit too generic assigning of main instructions, let's see how it works
        Error = MainInstructions(MoreMessages->GetString(0));
        MoreMessages->Delete(0);
      }

      if (Disconnected)
      {
        if (DoClose)
        {
          Close();
        }
        // for fatal error, it is essential that there is some message
        DebugAssert(!Error.IsEmpty());
        std::unique_ptr<ExtException> E(std::make_unique<ExtException>(Error, MoreMessages.release(), true));
        try__finally
        {
          FTerminal->FatalError(E.get(), L"");
        }
        __finally__removed
        {
          // delete E;
        } end_try__finally
      }
      else
      {
        throw ExtException(Error, MoreMessages.release(), true, UnicodeString(HelpKeyword));
      }
    }

    if ((Code != nullptr) && (FLastCodeClass != DummyCodeClass))
    {
      *Code = nb::ToUInt32(FLastCode);
    }

    if (FLAGSET(Flags, REPLY_SINGLE_LINE))
    {
      if (FLastResponse->GetCount() != 1)
      {
        throw Exception(FMTLOAD(FTP_RESPONSE_ERROR, FLastCommandSent, FLastResponse->GetText()));
      }
      Result = FLastResponse->GetString(0);
    }

    if (Response != nullptr)
    {
      *Response = FLastResponse.release();
      FLastResponse = std::make_unique<TStringList>();
      // just to be consistent
//      SAFE_DESTROY(FLastErrorResponse);
      FLastErrorResponse = std::make_unique<TStringList>();
    }
  }
  __finally
  {
    ResetReply();
  } end_try__finally
  return Result;
}

void TFTPFileSystem::SendCommand(const UnicodeString & Command)
{
  FFileZillaIntf->CustomCommand(Command.c_str());
  FLastCommandSent = CopyToChar(Command, L' ', false);
}

void TFTPFileSystem::SetLastCode(int32_t Code)
{
  FLastCode = Code;
  FLastCodeClass = (Code / 100);
}

void TFTPFileSystem::StoreLastResponse(const UnicodeString & Text)
{
  FLastResponse->Add(Text);
  if (FLastCodeClass >= 4)
  {
    FLastErrorResponse->Add(Text);
  }
}

void TFTPFileSystem::HandleReplyStatus(const UnicodeString & Response)
{
  int64_t Code = 0;

  if (FOnCaptureOutput != nullptr)
  {
    FOnCaptureOutput(Response, cotOutput);
  }

  if (FWelcomeMessage.IsEmpty() && ::StartsStr(L"SSH", Response))
  {
    FLastErrorResponse->Add(LoadStr(SFTP_AS_FTP_ERROR));
  }

  // Two forms of multiline responses were observed
  // (the first is according to the RFC 959):

  // 211-Features:
  //  MDTM
  //  REST STREAM
  //  SIZE
  // 211 End

  // This format is according to RFC 2228.
  // Is used by ProFTPD when deprecated MultilineRFC2228 directive is enabled
  // http://www.proftpd.org/docs/modules/mod_core.html#FileZillaNonASCII

  // 211-Features:
  // 211-MDTM
  // 211-REST STREAM
  // 211-SIZE
  // 211-AUTH TLS
  // 211-PBSZ
  // 211-PROT
  // 211 End

  // IIS 2003:

  // 211-FEAT
  //     SIZE
  //     MDTM
  // 211 END

  // Partially duplicated in CFtpControlSocket::OnReceive

  const bool HasCodePrefix =
    (Response.Length() >= 3) &&
    ::TryStrToInt64(Response.SubString(1, 3), Code) &&
    (Code >= 100) && (Code <= 599) &&
    ((Response.Length() == 3) || (Response[4] == L' ') || (Response[4] == L'-'));

  if (HasCodePrefix && !FMultiLineResponse)
  {
    FMultiLineResponse = (Response.Length() >= 4) && (Response[4] == L'-');
    FLastResponse->Clear();
    FLastErrorResponse->Clear();
    SetLastCode(nb::ToInt32(Code));
    if (Response.Length() >= 5)
    {
      StoreLastResponse(Response.SubString(5, Response.Length() - 4));
    }
  }
  else
  {
    int32_t Start;
    // response with code prefix
    if (HasCodePrefix && (FLastCode == Code))
    {
      // End of multiline response?
      if ((Response.Length() <= 3) || (Response[4] == L' '))
      {
        FMultiLineResponse = false;
      }
      Start = 5;
    }
    else
    {
      Start = (((Response.Length() >= 1) && (Response[1] == L' ')) ? 2 : 1);
    }

    // Intermediate empty lines are being added
    if (FMultiLineResponse || (Response.Length() >= Start))
    {
      StoreLastResponse(Response.SubString(Start, Response.Length() - Start + 1));
    }
  }

  if (::StartsStr(DirectoryHasBytesPrefix, Response))
  {
    UnicodeString Buf = Response;
    Buf.Delete(1, UnicodeString(DirectoryHasBytesPrefix).Length());
    Buf = Buf.TrimLeft();
    UnicodeString BytesStr = CutToChar(Buf, L' ', true);
    BytesStr = ReplaceStr(BytesStr, L",", L"");
    FBytesAvailable = StrToInt64Def(BytesStr, -1);
    if (FBytesAvailable >= 0)
    {
      FBytesAvailableSupported = true;
    }
  }

  if (!FMultiLineResponse)
  {
    if (FLastCode == 220)
    {
      // HOST command also uses 220 response.
      // Neither our use of welcome message is prepared for changing it
      // during the session, so we keep the initial message only.
      // Theoretically the welcome message can be host-specific,
      // but IIS uses "220 Host accepted", and we are not interested in that anyway.
      // Serv-U repeats the initial welcome message.
      // WS_FTP uses "200 Command HOST succeed"
      if (FWelcomeMessage.IsEmpty())
      {
        FWelcomeMessage = FLastResponse->GetText();
        if (FTerminal->GetConfiguration()->GetShowFtpWelcomeMessage())
        {
          FTerminal->DisplayBanner(FWelcomeMessage);
        }
        // Idea FTP Server v0.80
        if ((FTerminal->GetSessionData()->GetFtpTransferActiveImmediately() == asAuto) &&
            FWelcomeMessage.Pos("Idea FTP Server") > 0)
        {
          FTerminal->LogEvent("The server requires TLS/SSL handshake on transfer connection before responding 1yz to STOR/APPE");
          FTransferActiveImmediately = true;
        }
        if (ContainsText(FWelcomeMessage, L"Microsoft FTP Service") && !FIIS)
        {
          FTerminal->LogEvent(L"IIS detected.");
          FIIS = true;
        }
      }
    }
    else if (FLastCommand == PASS)
    {
      FStoredPasswordTried = true;
      // 530 = "Not logged in."
      if (FLastCode == 530)
      {
        FPasswordFailed = true;
      }
    }
    else if (FLastCommand == SYST)
    {
      DebugAssert(FSystem.IsEmpty());
      // Positive reply to "SYST" should be 215, see RFC 959.
      // But "VMS VAX/VMS V6.1 on node nsrp14" uses plain 200.
      if (FLastCodeClass == 2)
      {
        FSystem = FLastResponse->GetText().TrimRight();
        // FZAPI has own detection of MVS/VMS

        // Full name is "MVS is the operating system of this server. FTP Server is running on ..."
        // (the ... can be "z/OS")
        // https://www.ibm.com/docs/en/zos/latest?topic=2rc-215-mvs-is-operating-system-this-server-ftp-server-is-running-name
        // FZPI has a different incompatible detection.
        // MVS FTP servers have two separate MVS and Unix file systems coexisting in the same session.
        FMVS = (FSystem.SubString(1, 3) == L"MVS");
        if (FMVS)
        {
          FTerminal->LogEvent(L"MVS system detected.");
        }

        // The FWelcomeMessage usually contains "Microsoft FTP Service" but can be empty
        if (ContainsText(FSystem, L"Windows_NT"))
        {
          FTerminal->LogEvent("The server is probably running Windows, assuming that directory listing timestamps are affected by DST.");
          FWindowsServer = true;
          if (!FIIS)
          {
            FTerminal->LogEvent(L"IIS detected.");
            FIIS = true;
          }
        }
        // VMS system type. VMS V5.5-2.
        // VMS VAX/VMS V6.1 on node nsrp14
        if (FSystem.SubString(1, 4) == L"VMS ")
        {
          FTerminal->LogEvent(L"VMS system detected.");
          FVMS = true;
        }
        if ((FListAll == asAuto) &&
             // full name is "Personal FTP Server PRO K6.0"
            ((FSystem.Pos(L"Personal FTP Server") > 0) ||
             FMVS || FVMS))
        {
          FTerminal->LogEvent(L"Server is known not to support LIST -a");
          FListAll = asOff;
        }
        if ((FWorkFromCwd == asAuto) && FVMS)
        {
          FTerminal->LogEvent(L"Server is known to require use of relative paths");
          FWorkFromCwd = asOn;
        }
        // 220-FileZilla Server 1.0.1
        // 220 Please visit https://filezilla-project.org/
        // SYST
        // 215 UNIX emulated by FileZilla
        // (Welcome message is configurable)
        if (ContainsText(FSystem, L"FileZilla"))
        {
          FTerminal->LogEvent(L"FileZilla server detected.");
          FFileZilla = true;
        }
      }
      else
      {
        FSystem.Clear();
      }
    }
    else if (FLastCommand == FEAT)
    {
      HandleFeatReply();
    }
  }
}

void TFTPFileSystem::ResetFeatures()
{
  FFeatures->Clear();
  FSupportedCommands->Clear();
  FSupportedSiteCommands->Clear();
  FHashAlgs->Clear();
  FSupportsAnyChecksumFeature = false;
}

void TFTPFileSystem::ProcessFeatures()
{
  std::unique_ptr<TStrings> Features(FTerminal->ProcessFeatures(FFeatures.get()));

  for (int32_t Index = 0; Index < Features->Count; Index++)
  {
    const UnicodeString Feature = Features->GetString(Index);

    UnicodeString Args = Feature;
    UnicodeString Command = CutToChar(Args, L' ', true);

    // Serv-U lists Xalg commands like:
    //  XSHA1 filename;start;end
    FSupportedCommands->Add(Command);

    if (SameText(Command, SiteCommand))
    {
      // Serv-U lists all SITE commands in one line like:
      //  SITE PSWD;SET;ZONE;CHMOD;MSG;EXEC;HELP
      // But ProFTPD lists them separately:
      //  SITE UTIME
      //  SITE RMDIR
      //  SITE COPY
      //  SITE MKDIR
      //  SITE SYMLINK
      while (!Args.IsEmpty())
      {
        UnicodeString Arg = CutToChar(Args, L';', true);
        FSupportedSiteCommands->Add(Arg);
      }
    }
    else if (SameText(Command, HashCommand))
    {
      while (!Args.IsEmpty())
      {
        UnicodeString Alg = CutToChar(Args, L';', true);
        if ((Alg.Length() > 0) && (Alg[Alg.Length()] == L'*'))
        {
          Alg.Delete(Alg.Length(), 1);
        }
        // FTP HASH alg names follow IANA as we do,
        // but using uppercase and we use lowercase
        FHashAlgs->Add(LowerCase(Alg));
        FSupportsAnyChecksumFeature = true;
      }
    }

    if (FChecksumCommands->IndexOf(Command) >= 0)
    {
      FSupportsAnyChecksumFeature = true;
    }
  }
}

void TFTPFileSystem::HandleFeatReply()
{
  ResetFeatures();
  // Response to FEAT must be multiline, where leading and trailing line
  // is "meaningless". See RFC 2389.
  if ((FLastCode == 211) && (FLastResponse->GetCount() > 2))
  {
    FLastResponse->Delete(0);
    FLastResponse->Delete(FLastResponse->Count - 1);
    for (int32_t Index = 0; Index < FLastResponse->Count; Index++)
    {
      FFeatures->Add(FLastResponse->Strings[Index].Trim());
    }
  }
}

bool TFTPFileSystem::HandleStatus(const wchar_t * AStatus, int32_t Type)
{
  TLogLineType LogType = static_cast<TLogLineType>(-1);
  UnicodeString Status(AStatus);

  switch (Type)
  {
    case TFileZillaIntf::LOG_STATUS:
      FTerminal->Information(Status, true);
      LogType = llMessage;
      break;

    case TFileZillaIntf::LOG_COMMAND:
      if (Status == L"SYST")
      {
        // not to trigger the assert in HandleReplyStatus,
        // when SYST command is used by the user
        FSystem.Clear();
        FLastCommand = SYST;
      }
      else if (Status == L"FEAT")
      {
        FLastCommand = FEAT;
      }
      else if (Status.SubString(1, 5) == L"PASS ")
      {
        FLastCommand = PASS;
      }
      else
      {
        FLastCommand = CMD_UNKNOWN;
      }
      if (!FLoggedIn || (FTerminal->Configuration->ActualLogProtocol >= 0))
      {
        LogType = llInput;
      }
      break;

    case TFileZillaIntf::LOG_ERROR:
    case TFileZillaIntf::LOG_APIERROR:
    case TFileZillaIntf::LOG_WARNING:
      // when timeout message occurs, break loop waiting for response code
      // by setting dummy one
      if (Type == TFileZillaIntf::LOG_ERROR)
      {
        if (::StartsStr(FTimeoutStatus, Status))
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
      FLastError->Add(Status);
      LogType = llMessage;
      break;

    case TFileZillaIntf::LOG_PROGRESS:
      LogType = llMessage;
      break;

    case TFileZillaIntf::LOG_REPLY:
      HandleReplyStatus(AStatus);
      if (!FLoggedIn || (FTerminal->Configuration->ActualLogProtocol >= 0))
      {
        LogType = llOutput;
      }
      break;

    case TFileZillaIntf::LOG_INFO:
      LogType = llMessage;
      break;

    case TFileZillaIntf::LOG_DEBUG:
      LogType = llMessage;
      break;

    default:
      DebugFail();
      break;
  }

  if (FTerminal->GetLog()->GetLogging() && (LogType != static_cast<TLogLineType>(-1)))
  {
    FTerminal->GetLog()->Add(LogType, Status);
  }

  return true;
}

TDateTime TFTPFileSystem::ConvertLocalTimestamp(time_t Time)
{
  // This reverses how FZAPI converts FILETIME to time_t,
  // before passing it to FZ_ASYNCREQUEST_OVERWRITE.
  int64_t Timestamp;
  tm * Tm = gmtime(&Time);  // localtime(&Time);
  if (Tm != nullptr)
  {
    SYSTEMTIME SystemTime;
    SystemTime.wYear = nb::ToWord(Tm->tm_year + 1900);
    SystemTime.wMonth = nb::ToWord(Tm->tm_mon + 1);
    SystemTime.wDayOfWeek = 0;
    SystemTime.wDay = nb::ToWord(Tm->tm_mday);
    SystemTime.wHour = nb::ToWord(Tm->tm_hour);
    SystemTime.wMinute = nb::ToWord(Tm->tm_min);
    SystemTime.wSecond = nb::ToWord(Tm->tm_sec);
    SystemTime.wMilliseconds = 0;

    FILETIME LocalTime;
    SystemTimeToFileTime(&SystemTime, &LocalTime);
    FILETIME FileTime;
    ::LocalFileTimeToFileTime(&LocalTime, &FileTime);
    Timestamp = ::ConvertTimestampToUnixSafe(FileTime, dstmUnix);
  }
  else
  {
    // incorrect, but at least something
    Timestamp = Time;
  }

  return ::UnixToDateTime(Timestamp, dstmUnix);
}

bool TFTPFileSystem::HandleAsyncRequestOverwrite(
  wchar_t * FileName1, size_t FileName1Len, const wchar_t * FileName2,
  const wchar_t * Path1, const wchar_t * Path2,
  int64_t Size1, int64_t Size2, time_t LocalTime,
  bool /*HasLocalTime*/, const TRemoteFileTime & RemoteTime, void * AUserData,
  HANDLE & ALocalFileHandle,
  int32_t & RequestResult)
{
  if (!FActive)
  {
    return false;
  }
  else
  {
    TFileTransferData & UserData = *static_cast<TFileTransferData *>(AUserData);
    if (UserData.OverwriteResult >= 0)
    {
      // on retry, use the same answer as on the first attempt
      RequestResult = nb::ToInt32(UserData.OverwriteResult);
    }
    else if ((!UserData.CopyParam->FOnTransferOut.empty()) || (!UserData.CopyParam->FOnTransferIn.empty()))
    {
      DebugFail();
      RequestResult = TFileZillaIntf::FILEEXISTS_OVERWRITE;
    }
    else
    {
      TFileOperationProgressType * OperationProgress = FTerminal->GetOperationProgress();
      UnicodeString TargetFileName = FileName1;
      DebugAssert(UserData.FileName == TargetFileName);

      UnicodeString SourceFullFileName = Path2;
      UnicodeString TargetFullFileName = Path1;
      if (OperationProgress->GetSide() == osLocal)
      {
        SourceFullFileName = ::IncludeTrailingBackslash(SourceFullFileName);
        TargetFullFileName = base::UnixIncludeTrailingBackslash(TargetFullFileName);
      }
      else
      {
        SourceFullFileName = base::UnixIncludeTrailingBackslash(SourceFullFileName);
        TargetFullFileName = ::IncludeTrailingBackslash(TargetFullFileName);
      }
      SourceFullFileName += FileName2;
      TargetFullFileName += FileName1;

      TOverwriteMode OverwriteMode = omOverwrite;
      TOverwriteFileParams FileParams;
      const bool NoFileParams =
        (Size1 < 0) || (LocalTime == 0) ||
        (Size2 < 0) || !RemoteTime.HasDate;
      if (!NoFileParams)
      {
        FileParams.SourceSize = Size2;
        FileParams.DestSize = Size1;

        // Time is coming from LIST (not from MLSD or MDTM)
        const bool NeedApplyTimeDifference = !RemoteTime.Utc && DebugAlwaysTrue(!FFileZillaIntf->UsingMlsd());

        if (OperationProgress->GetSide() == osLocal)
        {
          FileParams.SourceTimestamp = ConvertLocalTimestamp(LocalTime);
          RemoteFileTimeToDateTimeAndPrecision(RemoteTime, FileParams.DestTimestamp, FileParams.DestPrecision);
          if (NeedApplyTimeDifference)
          {
            ApplyTimeDifference(TargetFullFileName, FileParams.DestTimestamp, FileParams.DestPrecision);
          }
        }
        else
        {
          FileParams.DestTimestamp = ConvertLocalTimestamp(LocalTime);
          RemoteFileTimeToDateTimeAndPrecision(RemoteTime, FileParams.SourceTimestamp, FileParams.SourcePrecision);
          if (NeedApplyTimeDifference)
          {
            ApplyTimeDifference(SourceFullFileName, FileParams.SourceTimestamp, FileParams.SourcePrecision);
          }
        }
      }

      const bool AllowResume = UserData.CopyParam->AllowResume(FileParams.SourceSize, UnicodeString());
      const bool AutoResume = UserData.AutoResume && AllowResume;
      if (ConfirmOverwrite(SourceFullFileName, TargetFileName, OverwriteMode, OperationProgress,
            NoFileParams ? nullptr : &FileParams, UserData.CopyParam, UserData.Params, AutoResume))
      {
        switch (OverwriteMode)
        {
          case omOverwrite:
            if (TargetFileName != FileName1)
            {
              wcsncpy_s(FileName1, FileName1Len, TargetFileName.c_str(), FileName1Len);
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

          case omComplete:
            FTerminal->LogEvent("File transfer was completed before disconnect");
            RequestResult = TFileZillaIntf::FILEEXISTS_COMPLETE;
            break;

          default:
            DebugFail();
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
      // by setting dummy one, as FZAPI won't do anything then
      SetLastCode(DummyTimeoutCode);
    }

    return true;
  }
}

static UnicodeString FormatContactList(const UnicodeString & Entry1, const UnicodeString & Entry2)
{
  if (!Entry1.IsEmpty() && !Entry2.IsEmpty())
  {
    return FORMAT("%s, %s", Entry1, Entry2);
  }
  return Entry1 + Entry2;
}

UnicodeString FormatContact(const TFtpsCertificateData::TContact & Contact)
{
  UnicodeString Result =
    FORMAT(LoadStrPart(VERIFY_CERT_CONTACT, 1),
      FormatContactList(FormatContactList(FormatContactList(
        Contact.Organization, Contact.Unit), Contact.CommonName), Contact.Mail));

  if ((nb::StrLength(Contact.Country) > 0) ||
      (nb::StrLength(Contact.StateProvince) > 0) ||
      (nb::StrLength(Contact.Town) > 0))
  {
    Result +=
      FORMAT(LoadStrPart(VERIFY_CERT_CONTACT, 2),
        FormatContactList(FormatContactList(
          Contact.Country, Contact.StateProvince), Contact.Town));
  }

  if (nb::StrLength(Contact.Other) > 0)
  {
    Result += FORMAT(LoadStrPart(VERIFY_CERT_CONTACT, 3), Contact.Other);
  }

  return Result;
}

UnicodeString FormatValidityTime(const TFtpsCertificateData::TValidityTime & ValidityTime)
{
#if 0
  return FormatDateTime(L"dddddd tt",
      EncodeDateVerbose(
        static_cast<uint16_t>(ValidityTime.Year), static_cast<uint16_t>(ValidityTime.Month),
        static_cast<uint16_t>(ValidityTime.Day)) +
      EncodeTimeVerbose(
        static_cast<uint16_t>(ValidityTime.Hour), static_cast<uint16_t>(ValidityTime.Min),
        static_cast<uint16_t>(ValidityTime.Sec), 0));
#endif // #if 0
  TODO("use Sysutils::FormatDateTime");
  uint16_t Y, M, D, H, Mm, S, MS;
  const TDateTime DateTime =
    EncodeDateVerbose(
      static_cast<uint16_t>(ValidityTime.Year), static_cast<uint16_t>(ValidityTime.Month),
      static_cast<uint16_t>(ValidityTime.Day)) +
    EncodeTimeVerbose(
      static_cast<uint16_t>(ValidityTime.Hour), static_cast<uint16_t>(ValidityTime.Min),
      static_cast<uint16_t>(ValidityTime.Sec), 0);
  DateTime.DecodeDate(Y, M, D);
  DateTime.DecodeTime(H, Mm, S, MS);
  UnicodeString dt = FORMAT("%02d.%02d.%04d %02d:%02d:%02d ", D, M, Y, H, Mm, S);
  return dt;
}

static bool VerifyNameMask(const UnicodeString & AName, const UnicodeString & AMask)
{
  bool Result = true;
  UnicodeString Name = AName;
  UnicodeString Mask = AMask;
  int32_t Pos = 0;
  while (Result && (Pos = Mask.Pos(L"*")) > 0)
  {
    // Pos will typically be 1 here, so not actual comparison is done
    Result = ::SameText(Mask.SubString(1, Pos - 1), Name.SubString(1, Pos - 1));
    if (Result)
    {
      Mask.Delete(1, Pos); // including *
      Name.Delete(1, Pos - 1);
      // remove everything until the next dot
      Pos = Name.Pos(L".");
      if (Pos == 0)
      {
        Pos = Name.Length() + 1;
      }
      Name.Delete(1, Pos - 1);
    }
  }

  if (Result)
  {
    Result = ::SameText(Mask, Name);
  }

  return Result;
}

bool TFTPFileSystem::VerifyCertificateHostName(const TFtpsCertificateData & Data)
{
  const UnicodeString HostName = FTerminal->GetSessionData()->GetHostNameExpanded();

  const UnicodeString CommonName = Data.Subject.CommonName;
  bool NoMask = CommonName.IsEmpty();
  bool Result = !NoMask && VerifyNameMask(HostName, CommonName);
  if (Result)
  {
    FTerminal->LogEvent(FORMAT("Certificate common name \"%s\" matches hostname", CommonName));
  }
  else
  {
    if (!NoMask && (FTerminal->GetConfiguration()->GetActualLogProtocol() >= 1))
    {
      FTerminal->LogEvent(FORMAT("Certificate common name \"%s\" does not match hostname", CommonName));
    }
    UnicodeString SubjectAltName = Data.SubjectAltName;
    while (!Result && !SubjectAltName.IsEmpty())
    {
      UnicodeString Entry = CutToChar(SubjectAltName, L',', true);
      UnicodeString EntryName = CutToChar(Entry, L':', true);
      if (::SameText(EntryName, "DNS"))
      {
        NoMask = false;
        Result = VerifyNameMask(HostName, Entry);
        if (Result)
        {
          FTerminal->LogEvent(FORMAT("Certificate subject alternative name \"%s\" matches hostname", Entry));
        }
        else
        {
          if (FTerminal->GetConfiguration()->GetActualLogProtocol() >= 1)
          {
            FTerminal->LogEvent(FORMAT("Certificate subject alternative name \"%s\" does not match hostname", Entry));
          }
        }
      }
    }
  }
  if (!Result && NoMask)
  {
    FTerminal->LogEvent("Certificate has no common name nor subject alternative name, not verifying hostname");
    Result = true;
  }
  return Result;
}

static bool IsIPAddress(const UnicodeString & AHostName)
{
  bool IPv4 = true;
  bool IPv6 = true;
  bool AnyColon = false;

  for (int32_t Index = 1; Index <= AHostName.Length(); Index++)
  {
    const wchar_t C = AHostName[Index];
    if (!IsDigit(C) && (C != L'.'))
    {
      IPv4 = false;
    }
    if (!IsHex(C) && (C != L':'))
    {
      IPv6 = false;
    }
    if (C == L':')
    {
      AnyColon = true;
    }
  }

  return IPv4 || (IPv6 && AnyColon);
}

bool TFTPFileSystem::HandleAsyncRequestVerifyCertificate(
  const TFtpsCertificateData & Data, int32_t & RequestResult)
{
  if (!FActive)
  {
    return false;
  }
  else
  {
    FSessionInfo.CertificateFingerprintSHA1 =
      BytesToHex(RawByteString(reinterpret_cast<const char *>(Data.HashSha1), Data.HashSha1Len), false, L':');
    FSessionInfo.CertificateFingerprintSHA256 =
      BytesToHex(RawByteString(reinterpret_cast<const char *>(Data.HashSha256), Data.HashSha256Len), false, L':');

    if (FTerminal->GetSessionData()->GetFingerprintScan())
    {
      RequestResult = 0;
    }
    else
    {
      const UnicodeString CertificateSubject = Data.Subject.Organization;
      FTerminal->LogEvent(FORMAT(L"Verifying certificate for \"%s\" with fingerprint %s and %d failures", CertificateSubject, FSessionInfo.CertificateFingerprintSHA256, Data.VerificationResult));

      bool Trusted = false;
      bool TryWindowsSystemCertificateStore = false;
      UnicodeString VerificationResultStr;
      switch (Data.VerificationResult)
      {
        case X509_V_OK:
          Trusted = true;
          break;
        case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT:
          VerificationResultStr = LoadStr(CERT_ERR_UNABLE_TO_GET_ISSUER_CERT);
          TryWindowsSystemCertificateStore = true;
          break;
        case X509_V_ERR_UNABLE_TO_DECRYPT_CERT_SIGNATURE:
          VerificationResultStr = LoadStr(CERT_ERR_UNABLE_TO_DECRYPT_CERT_SIGNATURE);
          break;
        case X509_V_ERR_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY:
          VerificationResultStr = LoadStr(CERT_ERR_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY);
          break;
        case X509_V_ERR_CERT_SIGNATURE_FAILURE:
          VerificationResultStr = LoadStr(CERT_ERR_CERT_SIGNATURE_FAILURE);
          break;
        case X509_V_ERR_CERT_NOT_YET_VALID:
          VerificationResultStr = LoadStr(CERT_ERR_CERT_NOT_YET_VALID);
          break;
        case X509_V_ERR_CERT_HAS_EXPIRED:
          VerificationResultStr = LoadStr(CERT_ERR_CERT_HAS_EXPIRED);
          break;
        case X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD:
          VerificationResultStr = LoadStr(CERT_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD);
          break;
        case X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD:
          VerificationResultStr = LoadStr(CERT_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD);
          break;
        case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
          VerificationResultStr = LoadStr(CERT_ERR_DEPTH_ZERO_SELF_SIGNED_CERT);
          TryWindowsSystemCertificateStore = true;
          break;
        case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN:
          VerificationResultStr = LoadStr(CERT_ERR_SELF_SIGNED_CERT_IN_CHAIN);
          TryWindowsSystemCertificateStore = true;
          break;
        case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY:
          VerificationResultStr = LoadStr(CERT_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY);
          TryWindowsSystemCertificateStore = true;
          break;
        case X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE:
          VerificationResultStr = LoadStr(CERT_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE);
          TryWindowsSystemCertificateStore = true;
          break;
        case X509_V_ERR_INVALID_CA:
          VerificationResultStr = LoadStr(CERT_ERR_INVALID_CA);
          break;
        case X509_V_ERR_PATH_LENGTH_EXCEEDED:
          VerificationResultStr = LoadStr(CERT_ERR_PATH_LENGTH_EXCEEDED);
          break;
        case X509_V_ERR_INVALID_PURPOSE:
          VerificationResultStr = LoadStr(CERT_ERR_INVALID_PURPOSE);
          break;
        case X509_V_ERR_CERT_UNTRUSTED:
          VerificationResultStr = LoadStr(CERT_ERR_CERT_UNTRUSTED);
          TryWindowsSystemCertificateStore = true;
          break;
        case X509_V_ERR_CERT_REJECTED:
          VerificationResultStr = LoadStr(CERT_ERR_CERT_REJECTED);
          break;
        case X509_V_ERR_KEYUSAGE_NO_CERTSIGN:
          VerificationResultStr = LoadStr(CERT_ERR_KEYUSAGE_NO_CERTSIGN);
          break;
        case X509_V_ERR_CERT_CHAIN_TOO_LONG:
          VerificationResultStr = LoadStr(CERT_ERR_CERT_CHAIN_TOO_LONG);
          break;
        default:
          VerificationResultStr =
            FORMAT("%s (%s)",
              LoadStr(CERT_ERR_UNKNOWN), UnicodeString(X509_verify_cert_error_string(Data.VerificationResult)));
          break;
      }

      const bool IsHostNameIPAddress = IsIPAddress(FTerminal->GetSessionData()->GetHostNameExpanded());
      const bool CertificateHostNameVerified = !IsHostNameIPAddress && VerifyCertificateHostName(Data);

      bool VerificationResult = Trusted;

      if (IsHostNameIPAddress || !CertificateHostNameVerified)
      {
        VerificationResult = false;
        TryWindowsSystemCertificateStore = false;
      }

      if (!VerificationResult)
      {
        if (FTerminal->VerifyCertificate(FtpsCertificateStorageKey, FTerminal->SessionData->SiteKey,
              FSessionInfo.CertificateFingerprintSHA1, FSessionInfo.CertificateFingerprintSHA256,
              CertificateSubject, Data.VerificationResult))
        {
          // certificate is trusted, but for not purposes of info dialog
          VerificationResult = true;
          FSessionInfo.CertificateVerifiedManually = true;
        }
      }

      // TryWindowsSystemCertificateStore is set for the same set of failures
      // as trigger NE_SSL_UNTRUSTED flag in ne_openssl.c's verify_callback().
      // Use WindowsValidateCertificate only as a last resort (after checking the cached fingerprint)
      // as it can take a very long time (up to 1 minute).
      if (!VerificationResult && TryWindowsSystemCertificateStore)
      {
        UnicodeString WindowsCertificateError;
        if (WindowsValidateCertificate(Data.Certificate, Data.CertificateLen, WindowsCertificateError))
        {
          FTerminal->LogEvent("Certificate verified against Windows certificate store");
          VerificationResult = true;
          // certificate is trusted for all purposes
          Trusted = true;
        }
        else
        {
          FTerminal->LogEvent(
            FORMAT("Certificate failed to verify against Windows certificate store: %s", DefaultStr(WindowsCertificateError, "no details")));
        }
      }

      const UnicodeString SummarySeparator = L"\n\n";
      UnicodeString Summary;
      // even if the fingerprint is cached, the certificate is still not trusted for a purposes of the info dialog.
      if (!Trusted)
      {
        AddToList(Summary, VerificationResultStr + L" " + FMTLOAD(CERT_ERRDEPTH, Data.VerificationDepth + 1), SummarySeparator);
      }

      if (IsHostNameIPAddress)
      {
        AddToList(Summary, FMTLOAD(CERT_IP_CANNOT_VERIFY, FTerminal->GetSessionData()->GetHostNameExpanded()), SummarySeparator);
      }
      else if (!CertificateHostNameVerified)
      {
        AddToList(Summary, FMTLOAD(CERT_NAME_MISMATCH, FTerminal->GetSessionData()->GetHostNameExpanded()), SummarySeparator);
      }

      if (Summary.IsEmpty())
      {
        Summary = LoadStr(CERT_OK);
      }

      FSessionInfo.Certificate =
        FMTLOAD(CERT_TEXT2,
          FormatContact(Data.Issuer),
          FormatContact(Data.Subject),
          FormatValidityTime(Data.ValidFrom),
          FormatValidityTime(Data.ValidUntil),
          FSessionInfo.CertificateFingerprintSHA256,
          FSessionInfo.CertificateFingerprintSHA1,
          Summary);

      RequestResult = VerificationResult ? 1 : 0;

      if (RequestResult == 0)
      {
        if (FTerminal->ConfirmCertificate(FSessionInfo, Data.VerificationResult, FtpsCertificateStorageKey, true))
        {
          // FZ's VerifyCertDlg.cpp returns 2 for "cached", what we do nto distinguish here,
          // however FZAPI takes all non-zero values equally.
          RequestResult = 1;
          FSessionInfo.CertificateVerifiedManually = true;
        }
      }
    }

    return true;
  }
}

bool TFTPFileSystem::HandleAsyncRequestNeedPass(
  struct TNeedPassRequestData & Data, int32_t & RequestResult) const
{
  if (!FActive)
  {
    return false;
  }
  else
  {
    UnicodeString Password;
    if (FCertificate != nullptr)
    {
      FTerminal->LogEvent("Server asked for password, but we are using certificate, and no password was specified upfront, using fake password");
      Password = "USINGCERT";
      RequestResult = TFileZillaIntf::REPLY_OK;
    }
    else
    {
      if (!FPasswordFailed && FTerminal->GetSessionData()->GetLoginType() == ltAnonymous)
      {
        RequestResult = TFileZillaIntf::REPLY_OK;
      }
      else if (FTerminal->PromptUser(FTerminal->GetSessionData(), pkPassword, LoadStr(PASSWORD_TITLE), "",
          LoadStr(PASSWORD_PROMPT), false, 0, Password))
      {
        RequestResult = TFileZillaIntf::REPLY_OK;
      }
      else
      {
        RequestResult = TFileZillaIntf::REPLY_ABORTED;
      }
    }

    Data.Password = nullptr;
    // When returning REPLY_OK, we need to return an allocated password,
    // even if we were returning and empty string we got on input.
    if (RequestResult == TFileZillaIntf::REPLY_OK)
    {
      Data.Password = _wcsdup(Password.c_str());
    }

    return true;
  }
}

void TFTPFileSystem::RemoteFileTimeToDateTimeAndPrecision(const TRemoteFileTime & Source, TDateTime & DateTime, TModificationFmt & ModificationFmt) const
{
  // ModificationFmt must be set after Modification
  if (Source.HasDate)
  {
    DateTime =
      EncodeDateVerbose(static_cast<uint16_t>(Source.Year), static_cast<uint16_t>(Source.Month),
        static_cast<uint16_t>(Source.Day));
    if (Source.HasTime)
    {
      DateTime = DateTime + 
        EncodeTimeVerbose(static_cast<uint16_t>(Source.Hour), static_cast<uint16_t>(Source.Minute),
          static_cast<uint16_t>(Source.Second), 0);
      ModificationFmt = Source.HasSeconds ? mfFull : (Source.HasYear ? mfYMDHM : mfMDHM);

      // With IIS, the Utc should be false only for MDTM
      if (FWindowsServer && !Source.Utc)
      {
        DateTime -= DSTDifferenceForTime(DateTime);
      }
    }
    else
    {
      ModificationFmt = mfMDY;
    }

    if (Source.Utc)
    {
      DateTime = ::ConvertTimestampFromUTC(DateTime);
    }

  }
  else
  {
    // With SCP we estimate date to be today, if we have at least time

    DateTime = static_cast<double>(0.0);
    ModificationFmt = mfNone;
  }
}

bool TFTPFileSystem::HandleListData(const wchar_t * Path,
  const TListDataEntry * Entries, uint32_t Count)
{
  if (!FActive)
  {
    return false;
  }
  else if (FIgnoreFileList)
  {
    // directory listing provided implicitly by FZAPI during certain operations is ignored
    DebugAssert(FFileList == nullptr);
    return false;
  }
  else if (FFileList)
  {
    DebugAssert(FFileList != nullptr);
    const UnicodeString AbsPath = GetAbsolutePath(FFileList->GetDirectory(), false);
    DebugAssert(FFileList->GetDirectory().IsEmpty() || base::UnixSamePath(AbsPath, Path));
    DebugUsedParam(Path);

    for (uint32_t Index = 0; Index < Count; ++Index)
    {
      const TListDataEntry * Entry = &Entries[Index];
      std::unique_ptr<TRemoteFile> File(std::make_unique<TRemoteFile>());
      try
      {
        File->SetTerminal(FTerminal);

        File->SetFileName(Entry->Name);
        try
        {
          const int32_t PermissionsLen = nb::StrLength(Entry->Permissions);
          if (PermissionsLen >= 10)
          {
            File->GetRightsNotConst()->SetText(Entry->Permissions + 1);
          }
          else if ((PermissionsLen == 3) || (PermissionsLen == 4))
          {
            File->GetRightsNotConst()->SetOctal(Entry->Permissions);
          }
        }
        catch (...)
        {
          // ignore permissions errors with FTP
        }

        File->SetHumanRights(Entry->HumanPerm);

        // deprecated, to be replaced with Owner/Group
        if (nb::StrLength(Entry->OwnerGroup) > 0)
        {
          const wchar_t * Space = wcschr(Entry->OwnerGroup, L' ');
          if (Space != nullptr)
          {
            File->GetFileOwner().SetName(UnicodeString(Entry->OwnerGroup, nb::ToInt32(Space - Entry->OwnerGroup)));
            File->GetFileGroup().SetName(Space + 1);
          }
          else
          {
            File->GetFileOwner().SetName(Entry->OwnerGroup);
          }
        }
        else
        {
          File->GetFileOwner().SetName(Entry->Owner);
          File->GetFileGroup().SetName(Entry->Group);
        }

        File->SetSize(Entry->Size);

        if (Entry->Dir)
        {
          File->SetType(FILETYPE_DIRECTORY);
        }
        else
        {
          File->SetType(FILETYPE_DEFAULT);
        }

        if (Entry->Link)
        {
          File->SetIsSymLink(true);
        }

        TDateTime Modification;
        TModificationFmt ModificationFmt;
        RemoteFileTimeToDateTimeAndPrecision(Entry->Time, Modification, ModificationFmt);
        File->SetModification(Modification);
        File->SetModificationFmt(ModificationFmt);
        File->SetLastAccess(File->GetModification());

        File->SetLinkTo(Entry->LinkTarget);

        File->Complete();
      }
      catch (Exception &E)
      {
        // delete File;
        const UnicodeString TmStr = FORMAT("%d/%d/%d/%d", nb::ToInt32(Entry->Time.HasTime),
            nb::ToInt32(Entry->Time.HasYear), nb::ToInt32(Entry->Time.HasSeconds), nb::ToInt32(Entry->Time.HasDate));
        const UnicodeString EntryData =
          FORMAT("%s/%s/%s/%s/%s/%s/%s/%d/%d/%d/%d/%d/%d/%d/%s",
            Entry->Name, Entry->Permissions, Entry->HumanPerm, Entry->Owner, Entry->Group, Entry->OwnerGroup, ::Int64ToStr(Entry->Size),
             nb::ToInt32(Entry->Dir), nb::ToInt32(Entry->Link), Entry->Time.Year, Entry->Time.Month, Entry->Time.Day,
             Entry->Time.Hour, Entry->Time.Minute, TmStr);
        throw ETerminal(&E, FMTLOAD(LIST_LINE_ERROR, EntryData), HELP_LIST_LINE_ERROR);
      }

      if (FTerminal->IsValidFile(File.get()))
      {
        FFileList->AddFile(File.release());
      }
    }
    return true;
  }
  return false;
}

bool TFTPFileSystem::HandleTransferStatus(bool Valid, int64_t TransferSize,
  int64_t Bytes, bool FileTransfer)
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

bool TFTPFileSystem::HandleReply(int32_t Command, int64_t Reply)
{
  if (!FActive)
  {
    return false;
  }
  else
  {
    if (FTerminal->GetConfiguration()->GetActualLogProtocol() >= 1)
    {
      FTerminal->LogEvent(FORMAT("Got reply %d to the command %d", Reply, Command));
    }

    // reply with Command 0 is not associated with current operation
    // so do not treat is as a reply
    // (it is typically used asynchronously to notify about disconnects)
    if (Command != 0)
    {
      DebugAssert(FCommandReply == 0);
      FCommandReply = nb::ToUInt32(Reply);
    }
    else
    {
      DebugAssert(FReply == 0);
      FReply = nb::ToUInt32(Reply);
    }
    return true;
  }
}

bool TFTPFileSystem::HandleCapabilities(
  TFTPServerCapabilities * ServerCapabilities)
{
  FServerCapabilities->Assign(ServerCapabilities);
  FFileSystemInfoValid = false;
  return true;
}

bool TFTPFileSystem::CheckError(int32_t ReturnCode, const wchar_t * Context)
{
  // we do not expect any FZAPI call to fail as it generally can fail only due to:
  // - invalid parameters
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
    FTerminal->FatalError(nullptr,
      FMTLOAD(INTERNAL_ERROR, FORMAT("fz#%s", Context), ::IntToHex(ReturnCode, 4)));
    DebugFail();
  }

  return false;
}

bool TFTPFileSystem::Unquote(UnicodeString & Str)
{
  enum
  {
    STATE_INIT,
    STATE_QUOTE,
    STATE_QUOTED,
    STATE_DONE
  } State = STATE_INIT;

  DebugAssert((Str.Length() > 0) && ((Str[1] == L'"') || (Str[1] == L'\'')));

  int32_t Index = 1;
  wchar_t Quote = 0;
  while (Index <= Str.Length())
  {
    switch (State)
    {
      case STATE_INIT:
        if ((Str[Index] == L'"') || (Str[Index] == L'\''))
        {
          Quote = Str[Index];
          State = STATE_QUOTED;
          Str.Delete(Index, 1);
        }
        else
        {
          DebugFail();
          // no quoted string
          Str.SetLength(0);
        }
        break;

      case STATE_QUOTED:
        if (Str[Index] == Quote)
        {
          State = STATE_QUOTE;
          Str.Delete(Index, 1);
        }
        else
        {
          ++Index;
        }
        break;

      case STATE_QUOTE:
        if (Str[Index] == Quote)
        {
          ++Index;
        }
        else
        {
          // end of quoted string, trim the rest
          Str.SetLength(Index - 1);
          State = STATE_DONE;
        }
        break;
    }
  }

  return (State == STATE_DONE);
}

void TFTPFileSystem::PreserveDownloadFileTime(HANDLE AHandle, void * UserData) const
{
  const TFileTransferData * Data = static_cast<TFileTransferData *>(UserData);
  DebugAssert(Data->CopyParam->FOnTransferOut.empty());
  FTerminal->UpdateTargetTime(AHandle, Data->Modification, mfFull, dstmUnix);
}

bool TFTPFileSystem::GetFileModificationTimeInUtc(const wchar_t * AFileName, struct tm & Time)
{
  bool Result;
  try
  {
    // error-handling-free and DST-mode-unaware copy of TTerminal::OpenLocalFile
    HANDLE LocalFileHandle = ::CreateFile(ApiPath(AFileName).c_str(), GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
    if (LocalFileHandle == INVALID_HANDLE_VALUE)
    {
      Result = false;
    }
    else
    {
      FILETIME MTime;
      if (!::GetFileTime(LocalFileHandle, nullptr, nullptr, &MTime))
      {
        Result = false;
      }
      else
      {
        const TDateTime Modification = ::ConvertTimestampToUTC(::FileTimeToDateTime(MTime));

        uint16_t Year;
        uint16_t Month;
        uint16_t Day;
        Modification.DecodeDate(Year, Month, Day);
        Time.tm_year = Year - 1900;
        Time.tm_mon = Month - 1;
        Time.tm_mday = Day;

        uint16_t Hour;
        uint16_t Min;
        uint16_t Sec;
        uint16_t MSec;
        Modification.DecodeTime(Hour, Min, Sec, MSec);
        Time.tm_hour = Hour;
        Time.tm_min = Min;
        Time.tm_sec = Sec;

        Result = true;
      }

      SAFE_CLOSE_HANDLE(LocalFileHandle);
    }
  }
  catch (...)
  {
    Result = false;
  }
  return Result;
}

void TFTPFileSystem::RegisterChecksumAlgCommand(const UnicodeString & Alg, const UnicodeString & Command)
{
  FChecksumAlgs->Add(Alg);
  FChecksumCommands->Add(Command);
}

void TFTPFileSystem::GetSupportedChecksumAlgs(TStrings * Algs)
{
  for (int32_t Index = 0; Index < FHashAlgs->GetCount(); Index++)
  {
    Algs->Add(FHashAlgs->GetString(Index));
  }

  for (int32_t Index = 0; Index < FChecksumAlgs->GetCount(); Index++)
  {
    const UnicodeString Alg = FChecksumAlgs->GetString(Index);
    const UnicodeString Command = FChecksumCommands->GetString(Index);

    if (SupportsCommand(Command) && (Algs->IndexOf(Alg) < 0))
    {
      Algs->Add(Alg);
    }
  }
}

bool TFTPFileSystem::SupportsSiteCommand(const UnicodeString & Command) const
{
  return (FSupportedSiteCommands->IndexOf(Command) >= 0);
}

bool TFTPFileSystem::SupportsCommand(const UnicodeString & Command) const
{
  return (FSupportedCommands->IndexOf(Command) >= 0);
}

void TFTPFileSystem::LockFile(const UnicodeString & /*AFileName*/, const TRemoteFile * /*File*/)
{
  DebugFail();
}

void TFTPFileSystem::UnlockFile(const UnicodeString & /*AFileName*/, const TRemoteFile * /*File*/)
{
  DebugFail();
}

void TFTPFileSystem::UpdateFromMain(TCustomFileSystem * /*MainFileSystem*/)
{
  // noop
}

void TFTPFileSystem::ClearCaches()
{
  // noop
}

UnicodeString GetOpenSSLVersionText()
{
  return UnicodeString(OPENSSL_VERSION_TEXT);
}

