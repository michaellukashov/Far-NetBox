//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#ifndef NO_FILEZILLA
//---------------------------------------------------------------------------
#ifndef MPEXT
#define MPEXT
#endif
#include "FtpFileSystem.h"
#include "FileZillaIntf.h"

#include "headers.hpp"
#include "Common.h"
#include "Exceptions.h"
#include "Terminal.h"
#include "TextsCore.h"
#include "TextsFileZilla.h"
#include "HelpCore.h"
#define OPENSSL_NO_EC
#define OPENSSL_NO_ECDSA
#define OPENSSL_NO_ECDH
#include <openssl/x509_vfy.h>
//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------
#define FILE_OPERATION_LOOP_EX(ALLOW_SKIP, MESSAGE, OPERATION) \
  FILE_OPERATION_LOOP_CUSTOM(FTerminal, ALLOW_SKIP, MESSAGE, OPERATION, L"")
//---------------------------------------------------------------------------
const int DummyCodeClass = 8;
const int DummyTimeoutCode = 801;
const int DummyCancelCode = 802;
const int DummyDisconnectCode = 803;
//---------------------------------------------------------------------------
class TFileZillaImpl : public TFileZillaIntf
{
public:
  explicit TFileZillaImpl(TFTPFileSystem * FileSystem);
  virtual ~TFileZillaImpl() {}

  virtual const wchar_t * Option(intptr_t OptionID) const;
  virtual intptr_t OptionVal(intptr_t OptionID) const;

protected:
  virtual bool DoPostMessage(TMessageType Type, WPARAM wParam, LPARAM lParam);

  virtual bool HandleStatus(const wchar_t * Status, int Type);
  virtual bool HandleAsynchRequestOverwrite(
    wchar_t * FileName1, size_t FileName1Len, const wchar_t * FileName2,
    const wchar_t * Path1, const wchar_t * Path2,
    int64_t Size1, int64_t Size2, time_t LocalTime,
    bool HasLocalTime, const TRemoteFileTime & RemoteTime, void * UserData,
    HANDLE & LocalFileHandle, int & RequestResult);
  virtual bool HandleAsynchRequestVerifyCertificate(
    const TFtpsCertificateData & Data, int & RequestResult);
  virtual bool HandleAsynchRequestNeedPass(
    struct TNeedPassRequestData & Data, int & RequestResult);
  virtual bool HandleListData(const wchar_t * Path, const TListDataEntry * Entries,
    uintptr_t Count);
  virtual bool HandleTransferStatus(bool Valid, int64_t TransferSize,
    int64_t Bytes, intptr_t Percent, intptr_t TimeElapsed, intptr_t TimeLeft, intptr_t TransferRate,
    bool FileTransfer);
  virtual bool HandleReply(intptr_t Command, uintptr_t Reply);
  virtual bool HandleCapabilities(TFTPServerCapabilities * ServerCapabilities);
  virtual bool CheckError(intptr_t ReturnCode, const wchar_t * Context);

  virtual void PreserveDownloadFileTime(HANDLE Handle, void * UserData);
  virtual bool GetFileModificationTimeInUtc(const wchar_t * FileName, struct tm & Time);

private:
  TFTPFileSystem * FFileSystem;
};
//---------------------------------------------------------------------------
TFileZillaImpl::TFileZillaImpl(TFTPFileSystem * FileSystem) :
  TFileZillaIntf(),
  FFileSystem(FileSystem)
{
}
//---------------------------------------------------------------------------
const wchar_t * TFileZillaImpl::Option(intptr_t OptionID) const
{
  return FFileSystem->GetOption(OptionID);
}
//---------------------------------------------------------------------------
intptr_t TFileZillaImpl::OptionVal(intptr_t OptionID) const
{
  return FFileSystem->GetOptionVal(OptionID);
}
//---------------------------------------------------------------------------
bool TFileZillaImpl::DoPostMessage(TMessageType Type, WPARAM wParam, LPARAM lParam)
{
  return FFileSystem->PostMessage(Type, wParam, lParam);
}
//---------------------------------------------------------------------------
bool TFileZillaImpl::HandleStatus(const wchar_t * Status, int Type)
{
  return FFileSystem->HandleStatus(Status, Type);
}
//---------------------------------------------------------------------------
bool TFileZillaImpl::HandleAsynchRequestOverwrite(
  wchar_t * FileName1, size_t FileName1Len, const wchar_t * FileName2,
  const wchar_t * Path1, const wchar_t * Path2,
  int64_t Size1, int64_t Size2, time_t LocalTime,
  bool HasLocalTime, const TRemoteFileTime & RemoteTime, void * UserData,
  HANDLE & LocalFileHandle,
  int & RequestResult)
{
  return FFileSystem->HandleAsynchRequestOverwrite(
    FileName1, FileName1Len, FileName2, Path1, Path2, Size1, Size2, LocalTime,
    HasLocalTime, RemoteTime, UserData, LocalFileHandle, RequestResult);
}
//---------------------------------------------------------------------------
bool TFileZillaImpl::HandleAsynchRequestVerifyCertificate(
  const TFtpsCertificateData & Data, int & RequestResult)
{
  return FFileSystem->HandleAsynchRequestVerifyCertificate(Data, RequestResult);
}
//---------------------------------------------------------------------------
bool TFileZillaImpl::HandleAsynchRequestNeedPass(
  struct TNeedPassRequestData & Data, int & RequestResult)
{
  return FFileSystem->HandleAsynchRequestNeedPass(Data, RequestResult);
}
//---------------------------------------------------------------------------
bool TFileZillaImpl::HandleListData(const wchar_t * Path,
  const TListDataEntry * Entries, uintptr_t Count)
{
  return FFileSystem->HandleListData(Path, Entries, Count);
}
//---------------------------------------------------------------------------
bool TFileZillaImpl::HandleTransferStatus(bool Valid, int64_t TransferSize,
  int64_t Bytes, intptr_t Percent, intptr_t TimeElapsed, intptr_t TimeLeft, intptr_t TransferRate,
  bool FileTransfer)
{
  return FFileSystem->HandleTransferStatus(Valid, TransferSize, Bytes, Percent,
    TimeElapsed, TimeLeft, TransferRate, FileTransfer);
}
//---------------------------------------------------------------------------
bool TFileZillaImpl::HandleReply(intptr_t Command, uintptr_t Reply)
{
  return FFileSystem->HandleReply(Command, Reply);
}
//---------------------------------------------------------------------------
bool TFileZillaImpl::HandleCapabilities(TFTPServerCapabilities * ServerCapabilities)
{
  return FFileSystem->HandleCapabilities(ServerCapabilities);
}
//---------------------------------------------------------------------------
bool TFileZillaImpl::CheckError(intptr_t ReturnCode, const wchar_t * Context)
{
  return FFileSystem->CheckError(ReturnCode, Context);
}
//---------------------------------------------------------------------------
void TFileZillaImpl::PreserveDownloadFileTime(HANDLE Handle, void * UserData)
{
  return FFileSystem->PreserveDownloadFileTime(Handle, UserData);
}
//---------------------------------------------------------------------------
bool TFileZillaImpl::GetFileModificationTimeInUtc(const wchar_t * FileName, struct tm & Time)
{
  return FFileSystem->GetFileModificationTimeInUtc(FileName, Time);
}
//---------------------------------------------------------------------------
struct message_t
{
  message_t() : wparam(0), lparam(0)
  {}
  message_t(WPARAM w, LPARAM l) : wparam(w), lparam(l)
  {}
  WPARAM wparam;
  LPARAM lparam;
};
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
class TMessageQueue : public TObject, public rde::vector<message_t>
{
public:
  typedef message_t value_type;
};
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
#if defined(__BORLANDC__)
struct TFileTransferData : public TObject
{
  TFileTransferData()
  {
    Params = 0;
    AutoResume = false;
    OverwriteResult = -1;
    CopyParam = nullptr;
  }

  UnicodeString FileName;
  intptr_t Params;
  bool AutoResume;
  int OverwriteResult;
  const TCopyParamType * CopyParam;
  TDateTime Modification;
};
//---------------------------------------------------------------------------
const int tfFirstLevel = 0x01;
const int tfAutoResume = 0x02;
#endif
static const wchar_t FtpsCertificateStorageKey[] = L"FtpsCertificates";
//---------------------------------------------------------------------------
#if defined(__BORLANDC__)
struct TSinkFileParams
{
  UnicodeString TargetDir;
  const TCopyParamType * CopyParam;
  intptr_t Params;
  TFileOperationProgressType * OperationProgress;
  bool Skipped;
  uintptr_t Flags;
};
#endif
//---------------------------------------------------------------------------
class TFTPFileListHelper : public TObject
{
NB_DISABLE_COPY(TFTPFileListHelper)
public:
  explicit TFTPFileListHelper(TFTPFileSystem * FileSystem, TRemoteFileList * FileList,
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
  TFTPFileSystem * FFileSystem;
  TRemoteFileList * FFileList;
  bool FIgnoreFileList;
};
//---------------------------------------------------------------------------
TFTPFileSystem::TFTPFileSystem(TTerminal * ATerminal):
  TCustomFileSystem(ATerminal),
  FFileZillaIntf(nullptr),
  FQueueCriticalSection(new TCriticalSection()),
  FTransferStatusCriticalSection(new TCriticalSection()),
  FQueue(new TMessageQueue()),
  FQueueEvent(CreateEvent(nullptr, true, false, nullptr)),
  FFileSystemInfoValid(false),
  FReply(0),
  FCommandReply(0),
  FLastCommand(CMD_UNKNOWN),
  FPasswordFailed(false),
  FMultineResponse(false),
  FLastCode(0),
  FLastCodeClass(0),
  FLastReadDirectoryProgress(0),
  FLastResponse(new TStringList()),
  FLastErrorResponse(new TStringList()),
  FLastError(new TStringList()),
  FFeatures(new TStringList()),
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
  FFileTransferCPSLimit(0),
  FAwaitingProgress(false),
  FOnCaptureOutput(nullptr),
  FListAll(asOn),
  FDoListAll(false),
  FServerCapabilities(nullptr)
{
}

void TFTPFileSystem::Init(void *)
{
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
  assert(FFileList == nullptr);

  FFileZillaIntf->Destroying();

  // to release memory associated with the messages
  DiscardMessages();

  SAFE_DESTROY(FFileZillaIntf);

  SAFE_DESTROY(FQueue);

  ::CloseHandle(FQueueEvent);

  SAFE_DESTROY(FQueueCriticalSection);
  SAFE_DESTROY(FTransferStatusCriticalSection);

  SAFE_DESTROY(FLastResponse);
  SAFE_DESTROY(FLastErrorResponse);
  SAFE_DESTROY(FLastError);
  SAFE_DESTROY(FFeatures);
  SAFE_DESTROY(FServerCapabilities);

  ResetCaches();
}
//---------------------------------------------------------------------------
void TFTPFileSystem::Open()
{
  // on reconnect, typically there may be pending status messages from previous session
  DiscardMessages();

  ResetCaches();
  FCurrentDirectory = L"";
  FHomeDirectory = L"";

  TSessionData * Data = FTerminal->GetSessionData();

  FSessionInfo.LoginTime = Now();
  FSessionInfo.ProtocolBaseName = L"FTP";
  FSessionInfo.ProtocolName = FSessionInfo.ProtocolBaseName;

  switch (Data->GetFtps())
  {
    case ftpsNone:
      // noop;
      break;

    case ftpsImplicit:
      FSessionInfo.SecurityProtocolName = LoadStr(FTPS_IMPLICIT);
      break;

    case ftpsExplicitSsl:
      FSessionInfo.SecurityProtocolName = LoadStr(FTPS_EXPLICIT_SSL);
      break;

    case ftpsExplicitTls:
      FSessionInfo.SecurityProtocolName = LoadStr(FTPS_EXPLICIT_TLS);
      break;

    default:
      FAIL;
      break;
  }

  FLastDataSent = Now();

  FMultineResponse = false;

  // initialize FZAPI on the first connect only
  if (FFileZillaIntf == nullptr)
  {
    std::unique_ptr<TFileZillaIntf> FileZillaImpl(new TFileZillaImpl(this));

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
    FileZillaImpl->SetDebugLevel(LogLevel);
    FileZillaImpl->Init();
    FFileZillaIntf = FileZillaImpl.release();
  }

  UnicodeString HostName = Data->GetHostNameExpanded();
  UnicodeString UserName = Data->GetUserNameExpanded();
  UnicodeString Password = Data->GetPassword();
  UnicodeString Account = Data->GetFtpAccount();
  UnicodeString Path = Data->GetRemoteDirectory();
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
  intptr_t TimeZoneOffset = TimeToMinutes(Data->GetTimeDifference());
  int UTF8 = 0;
  uintptr_t CodePage = Data->GetCodePageAsNumber();
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
  bool PromptedForCredentials = false;

  do
  {
    FSystem = L"";
    FFeatures->Clear();
    FFileSystemInfoValid = false;

    // TODO: the same for account? it ever used?

    // ask for username if it was not specified in advance, even on retry,
    // but keep previous one as default,
    if (Data->GetUserNameExpanded().IsEmpty())
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
        FTerminal->FatalError(nullptr, LoadStr(AUTHENTICATION_FAILED));
      }
      else
      {
        FUserName = UserName;
      }
    }

    // on retry ask for password
    if (FPasswordFailed)
    {
      FTerminal->LogEvent(L"Password prompt (last login attempt failed)");

      // on retry ask for new password
      Password = L"";
      if (!FTerminal->PromptUser(Data, pkPassword, LoadStr(PASSWORD_TITLE), L"",
            LoadStr(PASSWORD_PROMPT), false, 0, Password))
      {
        FTerminal->FatalError(nullptr, LoadStr(AUTHENTICATION_FAILED));
      }
    }

    FPasswordFailed = false;
    TValueRestorer<bool> OpeningRestorer(FOpening);
    FOpening = true;

    FActive = FFileZillaIntf->Connect(
      HostName.c_str(), static_cast<int>(Data->GetPortNumber()), UserName.c_str(),
      Password.c_str(), Account.c_str(), false, Path.c_str(),
      ServerType, Pasv, static_cast<int>(TimeZoneOffset), UTF8,
      static_cast<int>(Data->GetFtpForcePasvIp()),
      static_cast<int>(Data->GetFtpUseMlsd()),
      static_cast<int>(Data->GetFtpDupFF()),
      static_cast<int>(Data->GetFtpUndupFF()));

    assert(FActive);

    try
    {
      // do not wait for FTP response code as Connect is complex operation
      GotReply(WaitForCommandReply(false), REPLY_CONNECT, LoadStr(CONNECTION_FAILED));

      Shred(Password);

      // we have passed, even if we got 530 on the way (if it is possible at all),
      // ignore it
      assert(!FPasswordFailed);
      FPasswordFailed = false;
    }
    catch (...)
    {
      if (FPasswordFailed)
      {
        FTerminal->Information(
          LoadStr(Password.IsEmpty() ? FTP_ACCESS_DENIED_EMPTY_PASSWORD : FTP_ACCESS_DENIED),
          false);
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

  FSessionInfo.CSCipher = FFileZillaIntf->GetCipherName().c_str();
  FSessionInfo.SCCipher = FSessionInfo.CSCipher;
  UnicodeString TlsVersionStr = FFileZillaIntf->GetTlsVersionStr().c_str();
  AddToList(FSessionInfo.SecurityProtocolName, TlsVersionStr, L", ");
}
//---------------------------------------------------------------------------
void TFTPFileSystem::Close()
{
  assert(FActive);
  bool Result;
  if (FFileZillaIntf->Close(FOpening))
  {
    CHECK(FLAGSET(WaitForCommandReply(false), TFileZillaIntf::REPLY_DISCONNECTED));
    Result = true;
  }
  else
  {
    // See TFileZillaIntf::Close
    Result = FOpening;
  }

  if (ALWAYS_TRUE(Result))
  {
    assert(FActive);
    Discard();
    FTerminal->Closed();
  }
}
//---------------------------------------------------------------------------
bool TFTPFileSystem::GetActive() const
{
  return FActive;
}
//---------------------------------------------------------------------------
void TFTPFileSystem::CollectUsage()
{
/*
  if (FFileZillaIntf->UsingMlsd())
  {
    FTerminal->Configuration->Usage->Inc(L"OpenedSessionsFTPMLSD");
  }
  else
  {
     FTerminal->Configuration->Usage->Inc(L"OpenedSessionsFTPLIST");
  }

  if (FFileZillaIntf->UsingUtf8())
  {
    FTerminal->Configuration->Usage->Inc(L"OpenedSessionsFTPUTF8");
  }
  else
  {
     FTerminal->Configuration->Usage->Inc(L"OpenedSessionsFTPNonUTF8");
  }
  if (!GetCurrentDirectory().IsEmpty() && (GetCurrentDirectory()[1] != L'/'))
  {
    if (::IsUnixStyleWindowsPath(GetCurrentDirectory()))
    {
      FTerminal->Configuration->Usage->Inc(L"OpenedSessionsFTPWindowsPath");
    }
    else if ((GetCurrentDirectory().Length() >= 3) && IsLetter(GetCurrentDirectory()[1]) && (GetCurrentDirectory()[2] == L':') && (CurrentDirectory[3] == L'/'))
    {
      // FTerminal->Configuration->Usage->Inc(L"OpenedSessionsFTPRealWindowsPath");
    }
    else
    {
      FTerminal->Configuration->Usage->Inc(L"OpenedSessionsFTPOtherPath");
    }
  }
*/
}
//---------------------------------------------------------------------------
void TFTPFileSystem::Idle()
{
  if (FActive && !FWaitingForReply)
  {
    PoolForFatalNonCommandReply();

    // Keep session alive
    if ((FTerminal->GetSessionData()->GetFtpPingType() != ptOff) &&
        (static_cast<double>(Now() - FLastDataSent) > static_cast<double>(FTerminal->GetSessionData()->GetFtpPingIntervalDT()) * 4))
    {
      FLastDataSent = Now();

      std::unique_ptr<TRemoteDirectory> Files(new TRemoteDirectory(FTerminal));
      try
      {
        Files->SetDirectory(GetCurrentDirectory());
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
  }
}
//---------------------------------------------------------------------------
void TFTPFileSystem::Discard()
{
  // remove all pending messages, to get complete log
  // note that we need to retry discard on reconnect, as there still may be another
  // "disconnect/timeout/..." status messages coming
  DiscardMessages();
  assert(FActive);
  FActive = false;
}
//---------------------------------------------------------------------------
UnicodeString TFTPFileSystem::AbsolutePath(const UnicodeString & Path, bool /*Local*/)
{
  // TODO: improve (handle .. etc.)
  if (::UnixIsAbsolutePath(Path))
  {
    return Path;
  }
  else
  {
    return ::AbsolutePath(FCurrentDirectory, Path);
  }
}
//---------------------------------------------------------------------------
UnicodeString TFTPFileSystem::ActualCurrentDirectory()
{
  wchar_t CurrentPath[1024];
  FFileZillaIntf->GetCurrentPath(CurrentPath, LENOF(CurrentPath));
  UnicodeString fn = ::UnixExcludeTrailingBackslash(CurrentPath);
  if (fn.IsEmpty())
  {
    fn = L"/";
  }
  return fn;
}
//---------------------------------------------------------------------------
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
    if (!::UnixComparePaths(ActualCurrentDirectory(), FCurrentDirectory))
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
void TFTPFileSystem::AnyCommand(const UnicodeString & Command,
  TCaptureOutputEvent OutputEvent)
{
  // end-user has right to expect that client current directory is really
  // current directory for the server
  EnsureLocation();

  assert(FOnCaptureOutput == nullptr);
  FOnCaptureOutput = OutputEvent;
  SCOPE_EXIT
  {
    FOnCaptureOutput = nullptr;
  };
  FFileZillaIntf->CustomCommand(Command.c_str());

  GotReply(WaitForCommandReply(), REPLY_2XX_CODE | REPLY_3XX_CODE);
}
//---------------------------------------------------------------------------
void TFTPFileSystem::ResetCaches()
{
  SAFE_DESTROY(FFileListCache);
}
//---------------------------------------------------------------------------
void TFTPFileSystem::AnnounceFileListOperation()
{
  ResetCaches();
}
//---------------------------------------------------------------------------
void TFTPFileSystem::DoChangeDirectory(const UnicodeString & Directory)
{
  UnicodeString Command = FORMAT(L"CWD %s", Directory.c_str());
  FFileZillaIntf->CustomCommand(Command.c_str());

  GotReply(WaitForCommandReply(), REPLY_2XX_CODE);
}
//---------------------------------------------------------------------------
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
void TFTPFileSystem::CachedChangeDirectory(const UnicodeString & Directory)
{
  FCurrentDirectory = ::UnixExcludeTrailingBackslash(Directory);
  if (FCurrentDirectory.IsEmpty())
  {
    FCurrentDirectory = L"/";
  }
}
//---------------------------------------------------------------------------
void TFTPFileSystem::ChangeFileProperties(const UnicodeString & AFileName,
  const TRemoteFile * AFile, const TRemoteProperties * Properties,
  TChmodSessionAction & Action)
{
  assert(Properties);
  assert(!Properties->Valid.Contains(vpGroup)); //-V595
  assert(!Properties->Valid.Contains(vpOwner)); //-V595
  assert(!Properties->Valid.Contains(vpLastAccess)); //-V595
  assert(!Properties->Valid.Contains(vpModification)); //-V595

  if (Properties && Properties->Valid.Contains(vpRights))
  {
    std::unique_ptr<TRemoteFile> OwnedFile(nullptr);
    UnicodeString FileName = AbsolutePath(AFileName, false);

    if (AFile == nullptr)
    {
      TRemoteFile * File = nullptr;
      ReadFile(FileName, File);
      OwnedFile.reset(File);
      AFile = File;
    }

    if ((AFile != nullptr) && AFile->GetIsDirectory() && !AFile->GetIsSymLink() && Properties->Recursive)
    {
      try
      {
        FTerminal->ProcessDirectory(AFileName, MAKE_CALLBACK(TTerminal::ChangeFileProperties, FTerminal),
          static_cast<void *>(const_cast<TRemoteProperties *>(Properties)));
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

    UnicodeString FileNameOnly = ::UnixExtractFileName(FileName);
    UnicodeString FilePath = ::UnixExtractFilePath(FileName);
    // FZAPI wants octal number represented as decadic
    FFileZillaIntf->Chmod(Rights.GetNumberDecadic(), FileNameOnly.c_str(), FilePath.c_str());

    GotReply(WaitForCommandReply(), REPLY_2XX_CODE);
  }
  else
  {
    Action.Cancel();
  }
}
//---------------------------------------------------------------------------
bool TFTPFileSystem::LoadFilesProperties(TStrings * /*FileList*/)
{
  assert(false);
  return false;
}
//---------------------------------------------------------------------------
void TFTPFileSystem::CalculateFilesChecksum(const UnicodeString & /*Alg*/,
  TStrings * /*FileList*/, TStrings * /*Checksums*/,
  TCalculatedChecksumEvent /*OnCalculatedChecksum*/)
{
  assert(false);
}
//---------------------------------------------------------------------------
bool TFTPFileSystem::ConfirmOverwrite(UnicodeString & FileName,
  intptr_t Params, TFileOperationProgressType * OperationProgress,
  TOverwriteMode & OverwriteMode,
  bool AutoResume,
  const TOverwriteFileParams * FileParams,
  const TCopyParamType * CopyParam)
{
  bool Result;
  bool CanAutoResume = FLAGSET(Params, cpNoConfirmation) && AutoResume;
  bool DestIsSmaller = (FileParams != nullptr) && (FileParams->DestSize < FileParams->SourceSize);
  bool DestIsSame = (FileParams != nullptr) && (FileParams->DestSize == FileParams->SourceSize);
  bool CanResume =
    !OperationProgress->AsciiTransfer &&
    // when resuming transfer after interrupted connection,
    // do nothing (dummy resume) when the files has the same size.
    // this is workaround for servers that strangely fails just after successful
    // upload.
    (DestIsSmaller || (DestIsSame && CanAutoResume));

  uintptr_t Answer;
  if (CanAutoResume && CanResume)
  {
    if (DestIsSame)
    {
      assert(CanAutoResume);
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
    uintptr_t Answers = qaYes | qaNo | qaCancel | qaYesToAll | qaNoToAll | qaAll | qaIgnore;
    if (CanResume)
    {
      Answers |= qaRetry;
    }
    TQueryButtonAlias Aliases[5];
    Aliases[0].Button = qaRetry;
    Aliases[0].Alias = LoadStr(RESUME_BUTTON);
    Aliases[0].GroupWith = qaNo;
    Aliases[0].GrouppedShiftState = TShiftState() << ssAlt;
    Aliases[1].Button = qaAll;
    Aliases[1].Alias = LoadStr(YES_TO_NEWER_BUTTON);
    Aliases[1].GroupWith = qaYes;
    Aliases[1].GrouppedShiftState = TShiftState() << ssCtrl;
    Aliases[2].Button = qaIgnore;
    Aliases[2].Alias = LoadStr(RENAME_BUTTON);
    Aliases[2].GroupWith = qaNo;
    Aliases[2].GrouppedShiftState = TShiftState() << ssCtrl;
    Aliases[3].Button = qaYesToAll;
    Aliases[3].GroupWith = qaYes;
    Aliases[3].GrouppedShiftState = TShiftState() << ssShift;
    Aliases[4].Button = qaNoToAll;
    Aliases[4].GroupWith = qaNo;
    Aliases[4].GrouppedShiftState = TShiftState() << ssShift;
    TQueryParams QueryParams(qpNeverAskAgainCheck);
    QueryParams.Aliases = Aliases;
    QueryParams.AliasesCount = LENOF(Aliases);

    {
      TSuspendFileOperationProgress Suspend(OperationProgress);
      Answer = FTerminal->ConfirmFileOverwrite(FileName, FileParams,
        Answers, &QueryParams,
        OperationProgress->Side == osLocal ? osRemote : osLocal,
        CopyParam, Params, OperationProgress);
    }
  }

  Result = true;

  switch (Answer)
  {
    // resume
    case qaRetry:
      OverwriteMode = omResume;
      assert(FileParams != nullptr);
      assert(CanResume);
      FFileTransferResumed = FileParams ? FileParams->DestSize : 0;
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

    case qaSkip:
      OverwriteMode = omComplete;
      break;

    default:
      assert(false);
      Result = false;
      break;
  }
  return Result;
}
//---------------------------------------------------------------------------
void TFTPFileSystem::ResetFileTransfer()
{
  FFileTransferAbort = ftaNone;
  FFileTransferCancelled = false;
  FFileTransferResumed = 0;
}
//---------------------------------------------------------------------------
void TFTPFileSystem::ReadDirectoryProgress(int64_t Bytes)
{
  // with FTP we do not know exactly how many entries we have received,
  // instead we know number of bytes received only.
  // so we report approximation based on average size of entry.
  int Progress = static_cast<int>(Bytes / 80);
  if (Progress - FLastReadDirectoryProgress >= 10)
  {
    bool Cancel = false;
    FLastReadDirectoryProgress = Progress;
    FTerminal->DoReadDirectoryProgress(Progress, Cancel);
    if (Cancel)
    {
      FTerminal->DoReadDirectoryProgress(-2, Cancel);
      FFileZillaIntf->Cancel();
    }
  }
}
//---------------------------------------------------------------------------
void TFTPFileSystem::DoFileTransferProgress(int64_t TransferSize,
  int64_t Bytes)
{
  TFileOperationProgressType * OperationProgress = FTerminal->GetOperationProgress();

  OperationProgress->SetTransferSize(TransferSize);

  if (FFileTransferResumed > 0)
  {
    OperationProgress->AddResumed(FFileTransferResumed);
    FFileTransferResumed = 0;
  }

  int64_t Diff = Bytes - OperationProgress->TransferedSize;
  if (ALWAYS_TRUE(Diff >= 0))
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
void TFTPFileSystem::FileTransferProgress(int64_t TransferSize,
  int64_t Bytes)
{
  TGuard Guard(FTransferStatusCriticalSection);

  DoFileTransferProgress(TransferSize, Bytes);
}
//---------------------------------------------------------------------------
void TFTPFileSystem::FileTransfer(const UnicodeString & FileName,
  const UnicodeString & LocalFile, const UnicodeString & RemoteFile,
  const UnicodeString & RemotePath, bool Get, int64_t Size, intptr_t Type,
  TFileTransferData & UserData, TFileOperationProgressType * OperationProgress)
{
  FILE_OPERATION_LOOP(FMTLOAD(TRANSFER_ERROR, FileName.c_str()),
    FFileZillaIntf->FileTransfer(LocalFile.c_str(), RemoteFile.c_str(),
      RemotePath.c_str(), Get, Size, (int)Type, &UserData);
    // we may actually catch response code of the listing
    // command (when checking for existence of the remote file)
    uintptr_t Reply = WaitForCommandReply();
    GotReply(Reply, FLAGMASK(FFileTransferCancelled, REPLY_ALLOW_CANCEL));
  );

  switch (FFileTransferAbort)
  {
    case ftaSkip:
      ThrowSkipFileNull();

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
//---------------------------------------------------------------------------
void TFTPFileSystem::CopyToLocal(TStrings * AFilesToCopy,
  const UnicodeString & TargetDir, const TCopyParamType * CopyParam,
  intptr_t Params, TFileOperationProgressType * OperationProgress,
  TOnceDoneOperation & OnceDoneOperation)
{
  Params &= ~cpAppend;
  UnicodeString FullTargetDir = IncludeTrailingBackslash(TargetDir);

  intptr_t Index = 0;
  while (Index < AFilesToCopy->GetCount() && !OperationProgress->Cancel)
  {
    UnicodeString FileName = AFilesToCopy->GetString(Index);
    const TRemoteFile * File = NB_STATIC_DOWNCAST_CONST(TRemoteFile, AFilesToCopy->GetObject(Index));

    {
      bool Success = false;
      SCOPE_EXIT
      {
        OperationProgress->Finish(FileName, Success, OnceDoneOperation);
      };
      UnicodeString AbsoluteFilePath = AbsolutePath(FileName, false);
      UnicodeString TargetDirectory = FullTargetDir;
      UnicodeString FileNamePath = ::ExtractFilePath(File->GetFileName());
      if (!FileNamePath.IsEmpty())
      {
        TargetDirectory = ::IncludeTrailingBackslash(TargetDirectory + FileNamePath);
        ::ForceDirectories(TargetDirectory);
      }
      try
      {
        SinkRobust(AbsoluteFilePath, File, TargetDirectory, CopyParam, Params,
          OperationProgress, tfFirstLevel);
        Success = true;
        FLastDataSent = Now();
      }
      catch (ESkipFile & E)
      {
        TSuspendFileOperationProgress Suspend(OperationProgress);
        if (!FTerminal->HandleException(&E))
        {
          throw;
        }
      }
    }
    ++Index;
  }
}
//---------------------------------------------------------------------------
void TFTPFileSystem::SinkRobust(const UnicodeString & FileName,
  const TRemoteFile * AFile, const UnicodeString & TargetDir,
  const TCopyParamType * CopyParam, intptr_t Params,
  TFileOperationProgressType * OperationProgress, uintptr_t Flags)
{
  // the same in TSFTPFileSystem
  bool Retry;

  TDownloadSessionAction Action(FTerminal->GetActionLog());

  do
  {
    Retry = false;
    try
    {
      Sink(FileName, AFile, TargetDir, CopyParam, Params, OperationProgress,
        Flags, Action);
    }
    catch (Exception & E)
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
      assert(AFile != nullptr);
      if (!AFile->GetIsDirectory())
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
void TFTPFileSystem::Sink(const UnicodeString & FileName,
  const TRemoteFile * AFile, const UnicodeString & TargetDir,
  const TCopyParamType * CopyParam, intptr_t Params,
  TFileOperationProgressType * OperationProgress, uintptr_t Flags,
  TDownloadSessionAction & Action)
{
  assert(AFile);
  UnicodeString OnlyFileName = ::UnixExtractFileName(FileName);

  Action.FileName(FileName);

  TFileMasks::TParams MaskParams;
  MaskParams.Size = AFile->GetSize();
  MaskParams.Modification = AFile->GetModification();

  if (!CopyParam->AllowTransfer(FileName, osRemote, AFile->GetIsDirectory(), MaskParams))
  {
    FTerminal->LogEvent(FORMAT(L"File \"%s\" excluded from transfer", FileName.c_str()));
    ThrowSkipFileNull();
  }

  FTerminal->LogFileDetails(FileName, AFile->GetModification(), AFile->GetSize());

  OperationProgress->SetFile(OnlyFileName);

  UnicodeString DestFileName = CopyParam->ChangeFileName(::UnixExtractFileName(AFile->GetFileName()),
    osRemote, FLAGSET(Flags, tfFirstLevel));
  UnicodeString DestFullName = TargetDir + DestFileName;

  if (AFile->GetIsDirectory())
  {
    Action.Cancel();
    if (!AFile->GetIsSymLink())
    {
      FILE_OPERATION_LOOP (FMTLOAD(NOT_DIRECTORY_ERROR, DestFullName.c_str()),
        DWORD LocalFileAttrs = FTerminal->GetLocalFileAttributes(DestFullName);
        if (FLAGCLEAR(LocalFileAttrs, faDirectory))
        {
          ThrowExtException();
        }
      );

      FILE_OPERATION_LOOP (FMTLOAD(CREATE_DIR_ERROR, DestFullName.c_str()),
        THROWOSIFFALSE(ForceDirectories(DestFullName));
      );

      TSinkFileParams SinkFileParams;
      SinkFileParams.TargetDir = IncludeTrailingBackslash(DestFullName);
      SinkFileParams.CopyParam = CopyParam;
      SinkFileParams.Params = Params;
      SinkFileParams.OperationProgress = OperationProgress;
      SinkFileParams.Skipped = false;
      SinkFileParams.Flags = Flags & ~(tfFirstLevel | tfAutoResume);

      FTerminal->ProcessDirectory(FileName, MAKE_CALLBACK(TFTPFileSystem::SinkFile, this), &SinkFileParams);

      // Do not delete directory if some of its files were skip.
      // Throw "skip file" for the directory to avoid attempt to deletion
      // of any parent directory
      if (FLAGSET(Params, cpDelete) && SinkFileParams.Skipped)
      {
        ThrowSkipFileNull();
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

    // Will we use ASCII of BINARY file transfer?
    OperationProgress->SetAsciiTransfer(
      CopyParam->UseAsciiTransfer(FileName, osRemote, MaskParams));
    FTerminal->LogEvent(UnicodeString(OperationProgress->AsciiTransfer ? L"Ascii" : L"Binary") +
      L" transfer mode selected.");

    // Suppose same data size to transfer as to write
    OperationProgress->SetTransferSize(AFile->GetSize());
    OperationProgress->SetLocalSize(OperationProgress->TransferSize);

    DWORD LocalFileAttrs = INVALID_FILE_ATTRIBUTES;
    FILE_OPERATION_LOOP (FMTLOAD(NOT_FILE_ERROR, DestFullName.c_str()),
      LocalFileAttrs = FTerminal->GetLocalFileAttributes(DestFullName);
      if ((LocalFileAttrs != INVALID_FILE_ATTRIBUTES) && FLAGSET(LocalFileAttrs, faDirectory))
      {
        ThrowExtException();
      }
    );

    OperationProgress->TransferingFile = false; // not set with FTP protocol

    ResetFileTransfer();

    TFileTransferData UserData;

    UnicodeString FilePath = ::UnixExtractFilePath(FileName);
    if (FilePath.IsEmpty())
    {
      FilePath = L"/";
    }
    uintptr_t TransferType = (OperationProgress->AsciiTransfer ? 1 : 2);

    {
      // ignore file list
      TFTPFileListHelper Helper(this, nullptr, true);

      FFileTransferCPSLimit = OperationProgress->CPSLimit;
      FFileTransferPreserveTime = CopyParam->GetPreserveTime();
      // not used for downloads anyway
      FFileTransferRemoveBOM = CopyParam->GetRemoveBOM();
      UserData.FileName = DestFileName;
      UserData.Params = Params;
      UserData.AutoResume = FLAGSET(Flags, tfAutoResume);
      UserData.CopyParam = CopyParam;
      UserData.Modification = AFile->GetModification();
      try
      {
        FileTransfer(FileName, DestFullName, OnlyFileName,
          FilePath, true, AFile->GetSize(), TransferType, UserData, OperationProgress);
      }
      catch (Exception &)
      {
        //::CloseHandle(LocalFileHandle);
        throw;
      }
    }

    // in case dest filename is changed from overwrite dialog
    if (DestFileName != UserData.FileName)
    {
      DestFullName = TargetDir + UserData.FileName;
      LocalFileAttrs = FTerminal->GetLocalFileAttributes(DestFullName);
    }

    Action.Destination(ExpandUNCFileName(DestFullName));

    if (LocalFileAttrs == INVALID_FILE_ATTRIBUTES)
    {
      LocalFileAttrs = faArchive;
    }
    DWORD NewAttrs = CopyParam->LocalFileAttrs(*AFile->GetRights());
    if ((NewAttrs & LocalFileAttrs) != NewAttrs)
    {
      FILE_OPERATION_LOOP (FMTLOAD(CANT_SET_ATTRS, DestFullName.c_str()),
        THROWOSIFFALSE(FTerminal->SetLocalFileAttributes(DestFullName, (LocalFileAttrs | NewAttrs)) == 0);
      );
    }
  }

  if (FLAGSET(Params, cpDelete))
  {
    // If file is directory, do not delete it recursively, because it should be
    // empty already. If not, it should not be deleted (some files were
    // skipped or some new files were copied to it, while we were downloading)
    intptr_t Params = dfNoRecursive;
    FTerminal->DeleteFile(FileName, AFile, &Params);
  }
}
//---------------------------------------------------------------------------
void TFTPFileSystem::SinkFile(const UnicodeString & FileName,
  const TRemoteFile * AFile, void * Param)
{
  TSinkFileParams * Params = NB_STATIC_DOWNCAST(TSinkFileParams, Param);
  assert(Params->OperationProgress);
  try
  {
    SinkRobust(FileName, AFile, Params->TargetDir, Params->CopyParam,
      Params->Params, Params->OperationProgress, Params->Flags);
  }
  catch (ESkipFile & E)
  {
    TFileOperationProgressType * OperationProgress = Params->OperationProgress;

    Params->Skipped = true;

    {
      TSuspendFileOperationProgress Suspend(OperationProgress);
      if (!FTerminal->HandleException(&E))
      {
        throw;
      }
    }

    if (OperationProgress->Cancel)
    {
      Abort();
    }
  }
}
//---------------------------------------------------------------------------
void TFTPFileSystem::CopyToRemote(TStrings * AFilesToCopy,
  const UnicodeString & ATargetDir, const TCopyParamType * CopyParam,
  intptr_t Params, TFileOperationProgressType * OperationProgress,
  TOnceDoneOperation & OnceDoneOperation)
{
  assert((AFilesToCopy != nullptr) && (OperationProgress != nullptr));

  Params &= ~cpAppend;
  UnicodeString FileName, FileNameOnly;
  UnicodeString TargetDir = AbsolutePath(ATargetDir, false);
  UnicodeString FullTargetDir = ::UnixIncludeTrailingBackslash(TargetDir);
  intptr_t Index = 0;
  while ((Index < AFilesToCopy->GetCount()) && !OperationProgress->Cancel)
  {
    FileName = AFilesToCopy->GetString(Index);
    TRemoteFile * File = NB_STATIC_DOWNCAST(TRemoteFile, AFilesToCopy->GetObject(Index));
    UnicodeString RealFileName = File ? File->GetFileName() : FileName;

    FileNameOnly = ::ExtractFileName(RealFileName, false);

    {
      bool Success = false;
      SCOPE_EXIT
      {
        OperationProgress->Finish(FileName, Success, OnceDoneOperation);
      };
      try
      {
        if (FTerminal->GetSessionData()->GetCacheDirectories())
        {
          FTerminal->DirectoryModified(TargetDir, false);

          if (DirectoryExists(ExtractFilePath(FileName)))
          {
            FTerminal->DirectoryModified(FullTargetDir + FileNameOnly, true);
          }
        }
        SourceRobust(FileName, File, FullTargetDir, CopyParam, Params, OperationProgress,
          tfFirstLevel);
        Success = true;
        FLastDataSent = Now();
      }
      catch (ESkipFile & E)
      {
        TSuspendFileOperationProgress Suspend(OperationProgress);
        if (!FTerminal->HandleException(&E))
        {
          throw;
        }
      }
    }
    ++Index;
  }
}
//---------------------------------------------------------------------------
void TFTPFileSystem::SourceRobust(const UnicodeString & FileName,
  const TRemoteFile * AFile,
  const UnicodeString & TargetDir, const TCopyParamType * CopyParam, intptr_t Params,
  TFileOperationProgressType * OperationProgress, uintptr_t Flags)
{
  // the same in TSFTPFileSystem
  bool Retry;

  TUploadSessionAction Action(FTerminal->GetActionLog());
  TOpenRemoteFileParams OpenParams;
  OpenParams.OverwriteMode = omOverwrite;
  TOverwriteFileParams FileParams;

  do
  {
    Retry = false;
    try
    {
      Source(FileName, AFile, TargetDir, CopyParam, Params, &OpenParams, &FileParams, OperationProgress,
        Flags, Action);
    }
    catch (Exception & E)
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
void TFTPFileSystem::Source(const UnicodeString & FileName,
  const TRemoteFile * AFile,
  const UnicodeString & TargetDir, const TCopyParamType * CopyParam, intptr_t Params,
  TOpenRemoteFileParams * OpenParams,
  TOverwriteFileParams * FileParams,
  TFileOperationProgressType * OperationProgress, uintptr_t Flags,
  TUploadSessionAction & Action)
{
  UnicodeString RealFileName = AFile ? AFile->GetFileName() : FileName;
  Action.FileName(ExpandUNCFileName(FileName));

  OperationProgress->SetFile(RealFileName, false);

  if (!FTerminal->AllowLocalFileTransfer(FileName, CopyParam))
  {
    FTerminal->LogEvent(FORMAT(L"File \"%s\" excluded from transfer", RealFileName.c_str()));
    ThrowSkipFileNull();
  }

  int64_t MTime = 0, ATime = 0;
  int64_t Size = 0;

  FTerminal->OpenLocalFile(FileName, GENERIC_READ, &OpenParams->LocalFileAttrs,
    nullptr, nullptr, &MTime, &ATime, &Size);

  OperationProgress->SetFileInProgress();

  bool Dir = FLAGSET(OpenParams->LocalFileAttrs, faDirectory);
  if (Dir)
  {
    Action.Cancel();
    DirectorySource(IncludeTrailingBackslash(RealFileName), TargetDir,
      OpenParams->LocalFileAttrs, CopyParam, Params, OperationProgress, Flags);
  }
  else
  {
    UnicodeString DestFileName = CopyParam->ChangeFileName(::ExtractFileName(RealFileName, false),
      osLocal, FLAGSET(Flags, tfFirstLevel));

    FTerminal->LogEvent(FORMAT(L"Copying \"%s\" to remote directory started.", RealFileName.c_str()));

    OperationProgress->SetLocalSize(Size);

    // Suppose same data size to transfer as to read
    // (not true with ASCII transfer)
    OperationProgress->SetTransferSize(OperationProgress->LocalSize);
    OperationProgress->TransferingFile = false;

    TDateTime Modification;
    // Inspired by SysUtils::FileAge
    WIN32_FIND_DATA FindData;
    HANDLE Handle = ::FindFirstFile(FileName.c_str(), &FindData);
    if (Handle != INVALID_HANDLE_VALUE)
    {
      Modification =
        ::UnixToDateTime(
          ::ConvertTimestampToUnixSafe(FindData.ftLastWriteTime, dstmUnix),
          dstmUnix);
      ::FindClose(Handle);
    }

    // Will we use ASCII of BINARY file transfer?
    TFileMasks::TParams MaskParams;
    MaskParams.Size = Size;
    MaskParams.Modification = Modification;
    OperationProgress->SetAsciiTransfer(
      CopyParam->UseAsciiTransfer(RealFileName, osLocal, MaskParams));
    FTerminal->LogEvent(
      UnicodeString(OperationProgress->AsciiTransfer ? L"Ascii" : L"Binary") +
        L" transfer mode selected.");

    ResetFileTransfer();

    TFileTransferData UserData;

    uintptr_t TransferType = (OperationProgress->AsciiTransfer ? 1 : 2);

    // should we check for interrupted transfer?
    bool ResumeAllowed = !OperationProgress->AsciiTransfer &&
                         CopyParam->AllowResume(OperationProgress->LocalSize) &&
                         IsCapable(fcRename);
    OperationProgress->SetResumeStatus(ResumeAllowed ? rsEnabled : rsDisabled);

    FileParams->SourceSize = OperationProgress->LocalSize;
    FileParams->SourceTimestamp = ::UnixToDateTime(MTime,
                                  FTerminal->GetSessionData()->GetDSTMode());
    bool DoResume = (ResumeAllowed && (OpenParams->OverwriteMode == omOverwrite));
    {
      // ignore file list
      TFTPFileListHelper Helper(this, nullptr, true);

      FFileTransferCPSLimit = OperationProgress->CPSLimit;
      // not used for uploads anyway
      FFileTransferPreserveTime = CopyParam->GetPreserveTime();
      FFileTransferRemoveBOM = CopyParam->GetRemoveBOM();
      // not used for uploads, but we get new name (if any) back in this field
      UserData.FileName = DestFileName;
      UserData.Params = Params;
      UserData.AutoResume = FLAGSET(Flags, tfAutoResume) || DoResume;
      UserData.CopyParam = CopyParam;
      UserData.Modification = Modification;
      FileTransfer(RealFileName, FileName, DestFileName,
        TargetDir, false, Size, TransferType, UserData, OperationProgress);
    }

    UnicodeString DestFullName = TargetDir + UserData.FileName;
    // only now, we know the final destination
    Action.Destination(DestFullName);

    // we are not able to tell if setting timestamp succeeded,
    // so we log it always (if supported)
    if (FFileTransferPreserveTime &&
        (FServerCapabilities->GetCapability(mfmt_command) == yes))
    {
      TTouchSessionAction TouchAction(FTerminal->GetActionLog(), DestFullName, Modification);
    }
  }

  /* TODO : Delete also read-only files. */
  if (FLAGSET(Params, cpDelete))
  {
    if (!Dir)
    {
      FILE_OPERATION_LOOP (FMTLOAD(CORE_DELETE_LOCAL_FILE_ERROR, FileName.c_str()),
        THROWOSIFFALSE(Sysutils::DeleteFile(FileName));
      )
    }
  }
  else if (CopyParam->GetClearArchive() && FLAGSET(OpenParams->LocalFileAttrs, faArchive))
  {
    FILE_OPERATION_LOOP (FMTLOAD(CANT_SET_ATTRS, FileName.c_str()),
      THROWOSIFFALSE(FTerminal->SetLocalFileAttributes(FileName, OpenParams->LocalFileAttrs & ~faArchive) == 0);
    )
  }
}
//---------------------------------------------------------------------------
void TFTPFileSystem::DirectorySource(const UnicodeString & DirectoryName,
  const UnicodeString & TargetDir, intptr_t Attrs, const TCopyParamType * CopyParam,
  intptr_t Params, TFileOperationProgressType * OperationProgress, uintptr_t Flags)
{
  UnicodeString DestDirectoryName = CopyParam->ChangeFileName(
    ::ExtractFileName(::ExcludeTrailingBackslash(DirectoryName), false), osLocal,
    FLAGSET(Flags, tfFirstLevel));
  UnicodeString DestFullName = ::UnixIncludeTrailingBackslash(TargetDir + DestDirectoryName);

  OperationProgress->SetFile(DirectoryName);

  DWORD FindAttrs = faReadOnly | faHidden | faSysFile | faDirectory | faArchive;
  TSearchRec SearchRec;
  bool FindOK = false;

  FILE_OPERATION_LOOP (FMTLOAD(LIST_DIR_ERROR, DirectoryName.c_str()),
    FindOK = (bool)(FindFirstChecked((DirectoryName + L"*.*").c_str(),
      FindAttrs, SearchRec) == 0);
  );

  bool CreateDir = true;

  {
    SCOPE_EXIT
    {
      FindClose(SearchRec);
    };
    while (FindOK && !OperationProgress->Cancel)
    {
      UnicodeString FileName = DirectoryName + SearchRec.Name;
      try
      {
        if ((SearchRec.Name != THISDIRECTORY) && (SearchRec.Name != PARENTDIRECTORY))
        {
          SourceRobust(FileName, nullptr, DestFullName, CopyParam, Params, OperationProgress,
            Flags & ~(tfFirstLevel | tfAutoResume));
          // if any file got uploaded (i.e. there were any file in the
          // directory and at least one was not skipped),
          // do not try to create the directory,
          // as it should be already created by FZAPI during upload
          CreateDir = false;
        }
      }
      catch (ESkipFile & E)
      {
        // If ESkipFile occurs, just log it and continue with next file
        TSuspendFileOperationProgress Suspend(OperationProgress);
        // here a message to user was displayed, which was not appropriate
        // when user refused to overwrite the file in subdirectory.
        // hopefully it won't be missing in other situations.
        if (!FTerminal->HandleException(&E))
        {
          throw;
        }
      }

      FILE_OPERATION_LOOP (FMTLOAD(LIST_DIR_ERROR, DirectoryName.c_str()),
        FindOK = (FindNextChecked(SearchRec) == 0);
      );
    }
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
      SCOPE_EXIT
      {
        FTerminal->SetExceptionOnFail(false);
      };
      FTerminal->CreateDirectory(DestFullName, &Properties);
    }
    catch (...)
    {
      TRemoteFile * File = nullptr;
      // ignore non-fatal error when the directory already exists
      UnicodeString Fn = ::UnixExcludeTrailingBackslash(DestFullName);
      if (Fn.IsEmpty())
      {
        Fn = L"/";
      }
      bool Rethrow =
        !FTerminal->GetActive() ||
        !FTerminal->FileExists(Fn, &File) ||
        !File->GetIsDirectory();
      SAFE_DESTROY(File);
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
      FTerminal->RemoveLocalDirectory(DirectoryName);
    }
    else if (CopyParam->GetClearArchive() && FLAGSET(Attrs, faArchive))
    {
      FILE_OPERATION_LOOP (FMTLOAD(CANT_SET_ATTRS, DirectoryName.c_str()),
        THROWOSIFFALSE(FTerminal->SetLocalFileAttributes(DirectoryName, Attrs & ~faArchive) == 0);
      )
    }
  }
}
//---------------------------------------------------------------------------
void TFTPFileSystem::CreateDirectory(const UnicodeString & ADirName)
{
  UnicodeString DirName = AbsolutePath(ADirName, false);

  {
    // ignore file list
    TFTPFileListHelper Helper(this, nullptr, true);

    FFileZillaIntf->MakeDir(DirName.c_str());

    GotReply(WaitForCommandReply(), REPLY_2XX_CODE);
  }
}
//---------------------------------------------------------------------------
void TFTPFileSystem::CreateLink(const UnicodeString & /*FileName*/,
  const UnicodeString & /*PointTo*/, bool /*Symbolic*/)
{
  assert(false);
}
//---------------------------------------------------------------------------
void TFTPFileSystem::DeleteFile(const UnicodeString & AFileName,
  const TRemoteFile * AFile, intptr_t Params, TRmSessionAction & Action)
{
  UnicodeString FileName = AbsolutePath(AFileName, false);
  UnicodeString FileNameOnly = ::UnixExtractFileName(FileName);
  UnicodeString FilePath = ::UnixExtractFilePath(FileName);

  bool Dir = (AFile != nullptr) && AFile->GetIsDirectory() && !AFile->GetIsSymLink();

  if (Dir && FLAGCLEAR(Params, dfNoRecursive))
  {
    try
    {
      FTerminal->ProcessDirectory(FileName, MAKE_CALLBACK(TTerminal::DeleteFile, FTerminal), &Params);
    }
    catch (...)
    {
      Action.Cancel();
      throw;
    }
  }

  {
    // ignore file list
    TFTPFileListHelper Helper(this, nullptr, true);

    if (Dir)
    {
      // If current remote directory is in the directory being removed,
      // some servers may refuse to delete it
      // This is common as ProcessDirectory above would CWD to
      // the directory to LIST it.
      // EnsureLocation should reset actual current directory to user's working directory.
      // If user's working directory is still below deleted directory, it is
      // perfectly correct to report an error.
      if (::UnixIsChildPath(ActualCurrentDirectory(), FileName))
      {
        EnsureLocation();
      }
      FFileZillaIntf->RemoveDir(FileNameOnly.c_str(), FilePath.c_str());
    }
    else
    {
      FFileZillaIntf->Delete(FileNameOnly.c_str(), FilePath.c_str());
    }
    GotReply(WaitForCommandReply(), REPLY_2XX_CODE);
  }
}
//---------------------------------------------------------------------------
void TFTPFileSystem::CustomCommandOnFile(const UnicodeString & /*FileName*/,
  const TRemoteFile * /*File*/, const UnicodeString & /*Command*/, intptr_t /*Params*/,
  TCaptureOutputEvent /*OutputEvent*/)
{
  // if ever implemented, do not forget to add EnsureLocation,
  // see AnyCommand for a reason why
  assert(false);
}
//---------------------------------------------------------------------------
void TFTPFileSystem::DoStartup()
{
  std::unique_ptr<TStrings> PostLoginCommands(new TStringList());
  PostLoginCommands->SetText(FTerminal->GetSessionData()->GetPostLoginCommands());
  for (intptr_t Index = 0; Index < PostLoginCommands->GetCount(); ++Index)
  {
    UnicodeString Command = PostLoginCommands->GetString(Index);
    if (!Command.IsEmpty())
    {
      FFileZillaIntf->CustomCommand(Command.c_str());

      GotReply(WaitForCommandReply(), REPLY_2XX_CODE | REPLY_3XX_CODE);
    }
  }

  // retrieve initialize working directory to save it as home directory
  ReadCurrentDirectory();
  FHomeDirectory = FCurrentDirectory;
}
//---------------------------------------------------------------------------
void TFTPFileSystem::HomeDirectory()
{
  // FHomeDirectory is an absolute path, so avoid unnecessary overhead
  // of ChangeDirectory, such as EnsureLocation
  DoChangeDirectory(FHomeDirectory);
  FCurrentDirectory = FHomeDirectory;
  // make sure FZAPI is aware that we changed current working directory
  FFileZillaIntf->SetCurrentPath(FCurrentDirectory.c_str());
}
//---------------------------------------------------------------------------
bool TFTPFileSystem::IsCapable(intptr_t Capability) const
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
    case fcRemoveBOMUpload:
      return true;

    case fcPreservingTimestampUpload:
      return (FServerCapabilities->GetCapability(mfmt_command) == yes);

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
    case fcRemoveCtrlZUpload:
      return false;

    default:
      assert(false);
      return false;
  }
}
//---------------------------------------------------------------------------
void TFTPFileSystem::LookupUsersGroups()
{
  assert(false);
}
//---------------------------------------------------------------------------
void TFTPFileSystem::ReadCurrentDirectory()
{
  // ask the server for current directory on startup only
  // and immediately after call to CWD,
  // later our current directory may be not synchronized with FZAPI current
  // directory anyway, see comments in EnsureLocation
  if (FCurrentDirectory.IsEmpty())
  {
    FFileZillaIntf->CustomCommand(L"PWD");

    uintptr_t Code = 0;
    TStrings * Response = nullptr;
    GotReply(WaitForCommandReply(), REPLY_2XX_CODE, L"", &Code, &Response);

    std::unique_ptr<TStrings> ResponsePtr(Response);
    assert(ResponsePtr.get() != nullptr);
    bool Result = false;

    // the only allowed 2XX code to "PWD"
    if ((Code == 257) &&
        (ResponsePtr->GetCount() == 1))
    {
      UnicodeString Path = ResponsePtr->GetText();

      intptr_t P = Path.Pos(L"\"");
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
          FCurrentDirectory = ::AbsolutePath(L"/", ::UnixExcludeTrailingBackslash(Path));
          if (FCurrentDirectory.IsEmpty())
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
      throw Exception(FMTLOAD(FTP_PWD_RESPONSE_ERROR, ResponsePtr->GetText().c_str()));
    }
  }
}
//---------------------------------------------------------------------------
void TFTPFileSystem::DoReadDirectory(TRemoteFileList * FileList)
{
  FileList->Reset();
  // FZAPI does not list parent directory, add it
  FileList->AddFile(new TRemoteParentDirectory(FTerminal));

  FLastReadDirectoryProgress = 0;

  TFTPFileListHelper Helper(this, FileList, false);

  // always specify path to list, do not attempt to list "current" dir as:
  // 1) List() lists again the last listed directory, not the current working directory
  // 2) we handle this way the cached directory change
  UnicodeString Directory = AbsolutePath(FileList->GetDirectory(), false);
  FFileZillaIntf->List(Directory.c_str());

  GotReply(WaitForCommandReply(), REPLY_2XX_CODE | REPLY_ALLOW_CANCEL);

  FLastDataSent = Now();
}
//---------------------------------------------------------------------------
void TFTPFileSystem::ReadDirectory(TRemoteFileList * FileList)
{
  // whole below "-a" logic is for LIST,
  // if we know we are going to use MLSD, skip it
  if (FTerminal->GetSessionData()->GetFtpUseMlsd() == asOn)
  {
    DoReadDirectory(FileList);
  }
  else
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
            FTerminal->LogEvent(L"LIST with -a switch returned empty directory listing, will try pure LIST");
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
      catch (Exception &)
      {
        FDoListAll = false;
        // reading the first directory has failed,
        // further try without "-a" only as the server may not support it
        if (FListAll == asAuto)
        {
          FTerminal->LogEvent(L"LIST with -a failed, walling back to pure LIST");
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
//---------------------------------------------------------------------------
void TFTPFileSystem::DoReadFile(const UnicodeString & FileName,
  TRemoteFile *& AFile)
{
  // end-user has right to expect that client current directory is really
  // current directory for the server
  EnsureLocation();

  std::unique_ptr<TRemoteFileList> FileList(new TRemoteFileList());
  TFTPFileListHelper Helper(this, FileList.get(), false);
  FFileZillaIntf->ListFile(FileName.c_str());

  GotReply(WaitForCommandReply(), REPLY_2XX_CODE | REPLY_ALLOW_CANCEL);
  TRemoteFile * File = FileList->FindFile(::UnixExtractFileName(FileName));
  if (File != nullptr)
  {
    AFile = File->Duplicate();
  }

  FLastDataSent = Now();
}
//---------------------------------------------------------------------------
void TFTPFileSystem::ReadFile(const UnicodeString & FileName,
  TRemoteFile *& AFile)
{
  UnicodeString Path = ::UnixExtractFilePath(FileName);
  UnicodeString NameOnly = ::UnixExtractFileName(FileName);
  TRemoteFile * File = nullptr;
  bool Own = false;
  if (FServerCapabilities->GetCapability(mlsd_command) == yes)
  {
    DoReadFile(FileName, File);
    Own = true;
  }
  else
  {
    // FZAPI does not have efficient way to read properties of one file.
    // In case we need properties of set of files from the same directory,
    // cache the file list for future
    if ((FFileListCache != nullptr) &&
        ::UnixComparePaths(Path, FFileListCache->GetDirectory()) &&
        (::UnixIsAbsolutePath(FFileListCache->GetDirectory()) ||
        (FFileListCachePath == GetCurrentDirectory())))
    {
      File = FFileListCache->FindFile(NameOnly);
    }
    // if cache is invalid or file is not in cache, (re)read the directory
    if (File == nullptr)
    {
      std::unique_ptr<TRemoteFileList> FileListCache(new TRemoteFileList());
      FileListCache->SetDirectory(Path);
      ReadDirectory(FileListCache.get());
      // set only after we successfully read the directory,
      // otherwise, when we reconnect from ReadDirectory,
      // the FFileListCache is reset from ResetCache.
      SAFE_DESTROY(FFileListCache);
      FFileListCache = FileListCache.release();
      FFileListCachePath = GetCurrentDirectory();

      File = FFileListCache->FindFile(NameOnly);
    }

    Own = false;
  }

  if (File == nullptr)
  {
    AFile = nullptr;
    throw Exception(FMTLOAD(FILE_NOT_EXISTS, FileName.c_str()));
  }

  assert(File != nullptr);
  AFile = Own ? File : File->Duplicate();
}
//---------------------------------------------------------------------------
void TFTPFileSystem::ReadSymlink(TRemoteFile * SymlinkFile,
  TRemoteFile *& AFile)
{
  // Resolving symlinks over FTP is big overhead
  // (involves opening TCPIP connection for retrieving "directory listing").
  // Moreover FZAPI does not support that anyway.
  // Note that while we could use MLST to read the symlink,
  // it's hardly of any use as, if MLST is supported, we use MLSD to
  // retrieve directory listing and from MLSD we cannot atm detect that
  // the file is symlink anyway.
  std::unique_ptr<TRemoteFile> File(new TRemoteFile(SymlinkFile));
  File->SetTerminal(FTerminal);
  File->SetFileName(::UnixExtractFileName(SymlinkFile->GetLinkTo()));
  File->SetType(FILETYPE_SYMLINK);
  AFile = File.release();
}
//---------------------------------------------------------------------------
void TFTPFileSystem::RenameFile(const UnicodeString & AFileName,
  const UnicodeString & ANewName)
{
  UnicodeString FileName = AbsolutePath(AFileName, false);
  UnicodeString NewName = AbsolutePath(ANewName, false);

  UnicodeString FileNameOnly = ::UnixExtractFileName(FileName);
  UnicodeString FilePathOnly = ::UnixExtractFilePath(FileName);
  UnicodeString NewNameOnly = ::UnixExtractFileName(NewName);
  UnicodeString NewPathOnly = ::UnixExtractFilePath(NewName);

  // ignore file list
  TFTPFileListHelper Helper(this, nullptr, true);

  FFileZillaIntf->Rename(FileNameOnly.c_str(), NewNameOnly.c_str(),
    FilePathOnly.c_str(), NewPathOnly.c_str());

  GotReply(WaitForCommandReply(), REPLY_2XX_CODE);
}
//---------------------------------------------------------------------------
void TFTPFileSystem::CopyFile(const UnicodeString & FileName,
  const UnicodeString & NewName)
{
  assert(false);
}
//---------------------------------------------------------------------------
UnicodeString TFTPFileSystem::FileUrl(const UnicodeString & FileName) const
{
  UnicodeString Protocol = (FTerminal->GetSessionData()->GetFtps() == ftpsImplicit) ? FtpsProtocol : FtpProtocol;
  return FTerminal->FileUrl(Protocol, FileName);
}
//---------------------------------------------------------------------------
TStrings * TFTPFileSystem::GetFixedPaths()
{
  return nullptr;
}
//---------------------------------------------------------------------------
void TFTPFileSystem::SpaceAvailable(const UnicodeString & /* Path */,
  TSpaceAvailable & /* ASpaceAvailable */)
{
  assert(false);
}
//---------------------------------------------------------------------------
const TSessionInfo & TFTPFileSystem::GetSessionInfo() const
{
  return FSessionInfo;
}
//---------------------------------------------------------------------------
const TFileSystemInfo & TFTPFileSystem::GetFileSystemInfo(bool /*Retrieve*/)
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
      for (intptr_t Index = 0; Index < FFeatures->GetCount(); ++Index)
      {
        FFileSystemInfo.AdditionalInfo += FORMAT(L"  %s\r\n", FFeatures->GetString(Index).c_str());
      }
    }

    for (intptr_t Index = 0; Index < fcCount; ++Index)
    {
      FFileSystemInfo.IsCapable[Index] = IsCapable(Index);
    }

    FFileSystemInfoValid = true;
  }
  return FFileSystemInfo;
}
//---------------------------------------------------------------------------
bool TFTPFileSystem::TemporaryTransferFile(const UnicodeString & /*FileName*/)
{
  return false;
}
//---------------------------------------------------------------------------
bool TFTPFileSystem::GetStoredCredentialsTried()
{
  return !FTerminal->GetSessionData()->GetPassword().IsEmpty();
}
//---------------------------------------------------------------------------
UnicodeString TFTPFileSystem::GetUserName()
{
  return FUserName;
}
//---------------------------------------------------------------------------
UnicodeString TFTPFileSystem::GetCurrentDirectory()
{
  return FCurrentDirectory;
}
//---------------------------------------------------------------------------
const wchar_t * TFTPFileSystem::GetOption(intptr_t OptionID) const
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
intptr_t TFTPFileSystem::GetOptionVal(intptr_t OptionID) const
{
  TSessionData * Data = FTerminal->GetSessionData();
  intptr_t Result;

  switch (OptionID)
  {
    case OPTION_PROXYTYPE:
      switch (Data->GetActualProxyMethod())
      {
        case ::pmNone:
          Result = 0; // PROXYTYPE_NOPROXY;
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
          // assert(false);
          Result = 0; // PROXYTYPE_NOPROXY;
          break;
      }
      break;

    case OPTION_PROXYPORT:
    case OPTION_FWPORT:
      Result = Data->GetProxyPort();
      break;

    case OPTION_PROXYUSELOGON:
      Result = !Data->GetProxyUsername().IsEmpty();
      break;

    case OPTION_LOGONTYPE:
      Result = Data->GetFtpProxyLogonType();
      break;

    case OPTION_TIMEOUTLENGTH:
      Result = Data->GetTimeout();
      break;

    case OPTION_DEBUGSHOWLISTING:
      Result = TRUE;
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
      Result = static_cast<intptr_t>((FFileTransferCPSLimit / 1024)); // FZAPI expects KiB/s
      break;

    case OPTION_MPEXT_SHOWHIDDEN:
      Result = (FDoListAll ? TRUE : FALSE);
      break;

    case OPTION_MPEXT_SSLSESSIONREUSE:
      Result = (Data->GetSslSessionReuse() ? TRUE : FALSE);
      break;

    case OPTION_MPEXT_MIN_TLS_VERSION:
      Result = Data->GetMinTlsVersion();
      break;

    case OPTION_MPEXT_MAX_TLS_VERSION:
      Result = Data->GetMaxTlsVersion();
      break;

    case OPTION_MPEXT_SNDBUF:
      Result = Data->GetSendBuf();
      break;

    case OPTION_MPEXT_TRANSFER_ACTIVE_IMMEDIATELLY:
      Result = Data->GetFtpTransferActiveImmediatelly();
      break;

    case OPTION_MPEXT_REMOVE_BOM:
      Result = FFileTransferRemoveBOM ? TRUE : FALSE;
      break;

    default:
      assert(false);
      Result = FALSE;
      break;
  }

  return Result;
}
//---------------------------------------------------------------------------
bool TFTPFileSystem::PostMessage(uintptr_t Type, WPARAM wParam, LPARAM lParam)
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
bool TFTPFileSystem::ProcessMessage()
{
  bool Result;
  TMessageQueue::value_type Message;

  {
    TGuard Guard(FQueueCriticalSection);

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
      ResetEvent(FQueueEvent);
    }
  }

  if (Result)
  {
    FFileZillaIntf->HandleMessage(Message.wparam, Message.lparam);
  }

  return Result;
}
//---------------------------------------------------------------------------
void TFTPFileSystem::DiscardMessages()
{
  while (ProcessMessage());
}
//---------------------------------------------------------------------------
void TFTPFileSystem::WaitForMessages()
{
  intptr_t Result = WaitForSingleObject(FQueueEvent, INFINITE);
  if (Result != WAIT_OBJECT_0)
  {
    FTerminal->FatalError(nullptr, FMTLOAD(INTERNAL_ERROR, L"ftp#1", IntToStr(Result).c_str()));
  }
}
//---------------------------------------------------------------------------
void TFTPFileSystem::PoolForFatalNonCommandReply()
{
  assert(FReply == 0);
  assert(FCommandReply == 0);
  assert(!FWaitingForReply);

  FWaitingForReply = true;

  uintptr_t Reply = 0;

  {
    SCOPE_EXIT
    {
      FReply = 0;
      assert(FCommandReply == 0);
      FCommandReply = 0;
      assert(FWaitingForReply);
      FWaitingForReply = false;
    };
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
bool TFTPFileSystem::NoFinalLastCode() const
{
  return (FLastCodeClass == 0) || (FLastCodeClass == 1);
}
//---------------------------------------------------------------------------
bool TFTPFileSystem::KeepWaitingForReply(uintptr_t & ReplyToAwait, bool WantLastCode) const
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
void TFTPFileSystem::DoWaitForReply(uintptr_t & ReplyToAwait, bool WantLastCode)
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
uintptr_t TFTPFileSystem::WaitForReply(bool Command, bool WantLastCode)
{
  assert(FReply == 0);
  assert(FCommandReply == 0);
  assert(!FWaitingForReply);
  assert(!FTransferStatusCriticalSection->GetAcquired());

  ResetReply();
  FWaitingForReply = true;

  uintptr_t Reply = 0;

  SCOPE_EXIT
  {
    FReply = 0;
    FCommandReply = 0;
    assert(FWaitingForReply);
    FWaitingForReply = false;
  };
  uintptr_t & ReplyToAwait = (Command ? FCommandReply : FReply);
  DoWaitForReply(ReplyToAwait, WantLastCode);

  Reply = ReplyToAwait;

  return Reply;
}
//---------------------------------------------------------------------------
uintptr_t TFTPFileSystem::WaitForCommandReply(bool WantLastCode)
{
  return WaitForReply(true, WantLastCode);
}
//---------------------------------------------------------------------------
void TFTPFileSystem::WaitForFatalNonCommandReply()
{
  WaitForReply(false, false);
  assert(false);
}
//---------------------------------------------------------------------------
void TFTPFileSystem::ResetReply()
{
  FLastCode = 0;
  FLastCodeClass = 0;
  assert(FLastResponse != nullptr);
  FLastResponse->Clear();
  assert(FLastErrorResponse != nullptr);
  FLastErrorResponse->Clear();
  assert(FLastError != nullptr);
  FLastError->Clear();
}
//---------------------------------------------------------------------------
void TFTPFileSystem::GotNonCommandReply(uintptr_t Reply)
{
  assert(FLAGSET(Reply, TFileZillaIntf::REPLY_DISCONNECTED));
  GotReply(Reply);
  // should never get here as GotReply should raise fatal exception
  assert(false);
}
//---------------------------------------------------------------------------
void TFTPFileSystem::GotReply(uintptr_t Reply, uintptr_t Flags,
  const UnicodeString & Error, uintptr_t * Code, TStrings ** Response)
{
  SCOPE_EXIT
  {
    ResetReply();
  };
  if (FLAGSET(Reply, TFileZillaIntf::REPLY_OK))
  {
    assert(Reply == TFileZillaIntf::REPLY_OK);

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
    FTerminal->FatalError(nullptr, FMTLOAD(INTERNAL_ERROR, L"ftp#2", FORMAT(L"0x%x", static_cast<int>(Reply)).c_str()));
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

    AnsiString HelpKeyword;
    std::unique_ptr<TStrings> MoreMessages(new TStringList());
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
        MoreMessages->Add(LoadStr(FTP_CANNOT_OPEN_ACTIVE_CONNECTION2));
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

    MoreMessages->AddStrings(FLastErrorResponse);
    // see comment for FLastError
    FLastResponse->Clear();
    FLastErrorResponse->Clear();

    if (MoreMessages->GetCount() == 0)
    {
      MoreMessages.reset();
    }

    UnicodeString ErrorStr = Error;
    if (ErrorStr.IsEmpty() && (MoreMessages.get() != nullptr))
    {
      assert(MoreMessages->GetCount() > 0);
      // bit too generic assigning of main instructions, let's see how it works
      ErrorStr = MainInstructions(MoreMessages->GetString(0));
      MoreMessages->Delete(0);
    }

    if (Disconnected)
    {
      // for fatal error, it is essential that there is some message
      assert(!ErrorStr.IsEmpty());
      std::unique_ptr<ExtException> E(new ExtException(ErrorStr, MoreMessages.release(), true));
      FTerminal->FatalError(E.get(), L"");
    }
    else
    {
      throw ExtException(ErrorStr, MoreMessages.release(), true, HelpKeyword);
    }
  }

  if ((Code != nullptr) && (FLastCodeClass != DummyCodeClass))
  {
    *Code = static_cast<int>(FLastCode);
  }

  if (Response != nullptr)
  {
    *Response = FLastResponse;
    FLastResponse = new TStringList();
    // just to be consistent
    SAFE_DESTROY(FLastErrorResponse);
    FLastErrorResponse = new TStringList();
  }
}
//---------------------------------------------------------------------------
void TFTPFileSystem::SetLastCode(intptr_t Code)
{
  FLastCode = Code;
  FLastCodeClass = (Code / 100);
}
//---------------------------------------------------------------------------
void TFTPFileSystem::StoreLastResponse(const UnicodeString & Text)
{
  FLastResponse->Add(Text);
  if (FLastCodeClass >= 4)
  {
    FLastErrorResponse->Add(Text);
  }
}
//---------------------------------------------------------------------------
void TFTPFileSystem::HandleReplyStatus(const UnicodeString & Response)
{
  int64_t Code = 0;

  if (FOnCaptureOutput != nullptr)
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
    (Response.Length() >= 3) &&
    TryStrToInt(Response.SubString(1, 3), Code) &&
    (Code >= 100) && (Code <= 599) &&
    ((Response.Length() == 3) || (Response[4] == L' ') || (Response[4] == L'-'));

  if (HasCodePrefix && !FMultineResponse)
  {
    FMultineResponse = (Response.Length() >= 4) && (Response[4] == L'-');
    FLastResponse->Clear();
    FLastErrorResponse->Clear();
    SetLastCode(Code);
    if (Response.Length() >= 5)
    {
      StoreLastResponse(Response.SubString(5, Response.Length() - 4));
    }
  }
  else
  {
    intptr_t Start = 0;
    // response with code prefix
    if (HasCodePrefix && (FLastCode == Code))
    {
      // End of multiline response?
      if ((Response.Length() <= 3) || (Response[4] == L' '))
      {
        FMultineResponse = false;
      }
      Start = 5;
    }
    else
    {
      Start = (((Response.Length() >= 1) && (Response[1] == L' ')) ? 2 : 1);
    }

    // Intermediate empty lines are being added
    if (FMultineResponse || (Response.Length() >= Start))
    {
      StoreLastResponse(Response.SubString(Start, Response.Length() - Start + 1));
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
      }
    }
    else if (FLastCommand == SYST)
    {
      assert(FSystem.IsEmpty());
      // Positive reply to "SYST" must be 215, see RFC 959
      if (FLastCode == 215)
      {
        FSystem = FLastResponse->GetText().TrimRight();
        // full name is "Personal FTP Server PRO K6.0"
        if ((FListAll == asAuto) &&
            (FSystem.Pos(L"Personal FTP Server") > 0))
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
UnicodeString TFTPFileSystem::ExtractStatusMessage(const UnicodeString & Status)
{
  // CApiLog::LogMessage
  // (note that the formatting may not be present when LogMessageRaw is used)
  UnicodeString Result = Status;
  intptr_t P1 = Result.Pos(L"): ");
  if (P1 > 0)
  {
    intptr_t P2 = Result.Pos(L".cpp(");
    if ((P2 > 0) && (P2 < P1))
    {
      intptr_t P3 = Result.Pos(L"   caller=0x");
      if ((P3 > 0) && (P3 > P1))
      {
        Result = Result.SubString(P1 + 3, P3 - P1 - 3);
      }
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
bool TFTPFileSystem::HandleStatus(const wchar_t * AStatus, int Type)
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
TDateTime TFTPFileSystem::ConvertLocalTimestamp(time_t Time)
{
  // This reverses how FZAPI converts FILETIME to time_t,
  // before passing it to FZ_ASYNCREQUEST_OVERWRITE.
  int64_t Timestamp;
  tm * Tm = localtime(&Time);
  if (Tm != nullptr)
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
//---------------------------------------------------------------------------
bool TFTPFileSystem::HandleAsynchRequestOverwrite(
  wchar_t * FileName1, size_t FileName1Len, const wchar_t * /*FileName2*/,
  const wchar_t * Path1, const wchar_t * /*Path2*/,
  int64_t Size1, int64_t Size2, time_t LocalTime,
  bool /*HasLocalTime*/, const TRemoteFileTime & RemoteTime, void * AUserData,
  HANDLE & LocalFileHandle,
  int & RequestResult)
{
  if (!FActive)
  {
    return false;
  }
  UnicodeString DestFullName = Path1;
  AppendPathDelimiterW(DestFullName);
  DestFullName += FileName1;
  TFileTransferData & UserData = *(NB_STATIC_DOWNCAST(TFileTransferData, AUserData));
  if (UserData.OverwriteResult >= 0)
  {
    // on retry, use the same answer as on the first attempt
    RequestResult = UserData.OverwriteResult;
  }
  else
  {
    TFileOperationProgressType * OperationProgress = FTerminal->GetOperationProgress();
    UnicodeString FileName = FileName1;
    assert(UserData.FileName == FileName);
    TOverwriteMode OverwriteMode = omOverwrite;
    TOverwriteFileParams FileParams;
    bool NoFileParams =
      (Size1 < 0) || (LocalTime == 0) ||
      (Size2 < 0) || !RemoteTime.HasDate;
    if (!NoFileParams)
    {
      FileParams.SourceSize = Size2;
      FileParams.DestSize = Size1;

      if (OperationProgress->Side == osLocal)
      {
        FileParams.SourceTimestamp = ConvertLocalTimestamp(LocalTime);
        RemoteFileTimeToDateTimeAndPrecision(RemoteTime, FileParams.DestTimestamp, FileParams.DestPrecision);
      }
      else
      {
        FileParams.DestTimestamp = ConvertLocalTimestamp(LocalTime);
        RemoteFileTimeToDateTimeAndPrecision(RemoteTime, FileParams.SourceTimestamp, FileParams.SourcePrecision);
      }
    }

    if (ConfirmOverwrite(FileName, UserData.Params, OperationProgress,
          OverwriteMode,
          UserData.AutoResume && UserData.CopyParam->AllowResume(FileParams.SourceSize),
          NoFileParams ? nullptr : &FileParams, UserData.CopyParam))
    {
      switch (OverwriteMode)
      {
        case omOverwrite:
          if ((OperationProgress->Side == osRemote) && !FTerminal->TerminalCreateFile(DestFullName, OperationProgress,
            false, true,
            &LocalFileHandle))
          {
            RequestResult = TFileZillaIntf::FILEEXISTS_SKIP;
            break;
          }
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
          if ((OperationProgress->Side == osRemote) && !FTerminal->TerminalCreateFile(DestFullName, OperationProgress,
            true, true,
            &LocalFileHandle))
          {
//            ThrowSkipFileNull();
            RequestResult = TFileZillaIntf::FILEEXISTS_SKIP;
          }
          else
            RequestResult = TFileZillaIntf::FILEEXISTS_RESUME;
          break;

        case omComplete:
          FTerminal->LogEvent(L"File transfer was completed before disconnect");
          RequestResult = TFileZillaIntf::FILEEXISTS_COMPLETE;
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
    // by setting dummy one, as FZAPI won't do anything then
    SetLastCode(DummyTimeoutCode);
  }

  return true;
}
//---------------------------------------------------------------------------
#if defined(__BORLANDC__)
struct TClipboardHandler
{
  UnicodeString Text;

  void Copy(TObject * /*Sender*/)
  {
    TInstantOperationVisualizer Visualizer;
    CopyToClipboard(Text.c_str());
  }
};
#endif
//---------------------------------------------------------------------------
static UnicodeString FormatContactList(const UnicodeString & Entry1, const UnicodeString & Entry2)
{
  if (!Entry1.IsEmpty() && !Entry2.IsEmpty())
  {
    return FORMAT(L"%s, %s", Entry1.c_str(), Entry2.c_str());
  }
  else
  {
    return Entry1 + Entry2;
  }
}
//---------------------------------------------------------------------------
UnicodeString FormatContact(const TFtpsCertificateData::TContact & Contact)
{
  UnicodeString Result =
    FORMAT(LoadStrPart(VERIFY_CERT_CONTACT, 1).c_str(),
      FormatContactList(FormatContactList(FormatContactList(
        Contact.Organization, Contact.Unit).c_str(), Contact.CommonName).c_str(), Contact.Mail).c_str());

  if ((wcslen(Contact.Country) > 0) ||
      (wcslen(Contact.StateProvince) > 0) ||
      (wcslen(Contact.Town) > 0))
  {
    Result +=
      FORMAT(LoadStrPart(VERIFY_CERT_CONTACT, 2).c_str(),
        FormatContactList(FormatContactList(
          Contact.Country, Contact.StateProvince).c_str(), Contact.Town).c_str());
  }

  if (wcslen(Contact.Other) > 0)
  {
    Result += FORMAT(LoadStrPart(VERIFY_CERT_CONTACT, 3).c_str(), Contact.Other);
  }

  return Result;
}
//---------------------------------------------------------------------------
UnicodeString FormatValidityTime(const TFtpsCertificateData::TValidityTime & ValidityTime)
{
  /*
  return FormatDateTime(L"ddddd tt",
    EncodeDateVerbose(
      static_cast<uint16_t>(ValidityTime.Year), static_cast<uint16_t>(ValidityTime.Month),
      static_cast<uint16_t>(ValidityTime.Day)) +
    EncodeTimeVerbose(
      static_cast<uint16_t>(ValidityTime.Hour), static_cast<uint16_t>(ValidityTime.Min),
      static_cast<uint16_t>(ValidityTime.Sec), 0));
  */
  uint16_t Y, M, D, H, N, S, MS;
  TDateTime DateTime =
    EncodeDateVerbose(
      static_cast<uint16_t>(ValidityTime.Year), static_cast<uint16_t>(ValidityTime.Month),
      static_cast<uint16_t>(ValidityTime.Day)) +
    EncodeTimeVerbose(
      static_cast<uint16_t>(ValidityTime.Hour), static_cast<uint16_t>(ValidityTime.Min),
      static_cast<uint16_t>(ValidityTime.Sec), 0);
  DateTime.DecodeDate(Y, M, D);
  DateTime.DecodeTime(H, N, S, MS);
  UnicodeString dt = FORMAT(L"%02d.%02d.%04d %02d:%02d:%02d ", D, M, Y, H, N, S);
  return dt;
}
//---------------------------------------------------------------------------
bool TFTPFileSystem::HandleAsynchRequestVerifyCertificate(
  const TFtpsCertificateData & Data, int & RequestResult)
{
  if (!FActive)
  {
    return false;
  }
  else
  {
    FSessionInfo.CertificateFingerprint =
      BytesToHex(RawByteString(reinterpret_cast<const char *>(Data.Hash), Data.HashLen), false, L':');

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

    UnicodeString Summary = LoadStr(VerificationResultStr);
    if (Data.VerificationResult != X509_V_OK)
    {
      Summary += L" " + FMTLOAD(CERT_ERRDEPTH, Data.VerificationDepth + 1);
    }

    FSessionInfo.Certificate =
      FMTLOAD(CERT_TEXT,
        FormatContact(Data.Issuer).c_str(),
        FormatContact(Data.Subject).c_str(),
        FormatValidityTime(Data.ValidFrom).c_str(),
        FormatValidityTime(Data.ValidUntil).c_str(),
        FSessionInfo.CertificateFingerprint.c_str(),
        Summary.c_str());

    RequestResult = 0;

    {
      std::unique_ptr<THierarchicalStorage> Storage(
        FTerminal->GetConfiguration()->CreateStorage(false));
      Storage->SetAccessMode(smRead);

      if (Storage->OpenSubKey(FtpsCertificateStorageKey, false) &&
          Storage->ValueExists(FSessionInfo.CertificateFingerprint))
      {
        RequestResult = 1;
      }
    }

    if (RequestResult == 0)
    {
      UnicodeString Buf = FTerminal->GetSessionData()->GetHostKey();
      while ((RequestResult == 0) && !Buf.IsEmpty())
      {
        UnicodeString ExpectedKey = CutToChar(Buf, L';', false);
        if (ExpectedKey == L"*")
        {
          UnicodeString Message = LoadStr(ANY_CERTIFICATE);
          FTerminal->Information(Message, true);
          FTerminal->GetLog()->Add(llException, Message);
          RequestResult = 1;
        }
        else if (ExpectedKey == FSessionInfo.CertificateFingerprint)
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
      Aliases[0].OnClick = MAKE_CALLBACK(TClipboardHandler::Copy, &ClipboardHandler);

      TQueryParams Params;
      Params.HelpKeyword = HELP_VERIFY_CERTIFICATE;
      Params.NoBatchAnswers = qaYes | qaRetry;
      Params.Aliases = Aliases;
      Params.AliasesCount = LENOF(Aliases);
      uintptr_t Answer = FTerminal->QueryUser(
        FMTLOAD(VERIFY_CERT_PROMPT3, FSessionInfo.Certificate.c_str()),
        nullptr, qaYes | qaNo | qaCancel | qaRetry, &Params, qtWarning);

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
        std::unique_ptr<THierarchicalStorage> Storage(
          FTerminal->GetConfiguration()->CreateStorage(false));
        Storage->SetAccessMode(smReadWrite);

        if (Storage->OpenSubKey(FtpsCertificateStorageKey, true))
        {
          Storage->WriteString(FSessionInfo.CertificateFingerprint, FSessionInfo.CertificateFingerprint);
        }
      }
    }

    return true;
  }
}
//---------------------------------------------------------------------------
bool TFTPFileSystem::HandleAsynchRequestNeedPass(
  struct TNeedPassRequestData & Data, int & RequestResult) const
{
  if (!FActive)
  {
    return false;
  }
  else
  {
    UnicodeString Password = L"";
    if (FTerminal->PromptUser(FTerminal->GetSessionData(), pkPassword, LoadStr(PASSWORD_TITLE), L"",
      LoadStr(PASSWORD_PROMPT), false, 0, Password))
    {
      Data.Password = _wcsdup(Password.c_str());
      RequestResult = TFileZillaIntf::REPLY_OK;
    }
    else
    {
      RequestResult = TFileZillaIntf::REPLY_ABORTED;
      Data.Password = nullptr;
    }
    return true;
  }
}
//---------------------------------------------------------------------------
void TFTPFileSystem::RemoteFileTimeToDateTimeAndPrecision(const TRemoteFileTime & Source, TDateTime & DateTime, TModificationFmt & ModificationFmt)
{
  // ModificationFmt must be set after Modification
  if (Source.HasDate)
  {
    DateTime =
      EncodeDateVerbose((Word)Source.Year, (Word)Source.Month, (Word)Source.Day);
    if (Source.HasTime)
    {
      DateTime = DateTime +
        EncodeTimeVerbose((Word)Source.Hour, (Word)Source.Minute, (Word)Source.Second, 0);
      // not exact as we got year as well, but it is most probably
      // guessed by FZAPI anyway
      ModificationFmt = Source.HasSeconds ? mfFull : mfMDHM;
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

    DateTime = double(0.0);
    ModificationFmt = mfNone;
  }
}
//---------------------------------------------------------------------------
bool TFTPFileSystem::HandleListData(const wchar_t * Path,
  const TListDataEntry * Entries, uintptr_t Count)
{
  if (!FActive)
  {
    return false;
  }
  else if (FIgnoreFileList)
  {
    // directory listing provided implicitly by FZAPI during certain operations is ignored
    assert(FFileList == nullptr);
    return false;
  }
  else
  {
    assert(FFileList != nullptr);
    // this can actually fail in real life,
    // when connected to server with case insensitive paths
    assert(::UnixComparePaths(AbsolutePath(FFileList->GetDirectory(), false), Path));
    USEDPARAM(Path);

    for (uintptr_t Index = 0; Index < Count; ++Index)
    {
      const TListDataEntry * Entry = &Entries[Index];
      std::unique_ptr<TRemoteFile> File(new TRemoteFile());
      try
      {
        File->SetTerminal(FTerminal);

        File->SetFileName(Entry->Name);
        try
        {
          intptr_t PermissionsLen = wcslen(Entry->Permissions);
          if (PermissionsLen >= 10)
          {
            File->GetRights()->SetText(Entry->Permissions + 1);
          }
          else if ((PermissionsLen == 3) || (PermissionsLen == 4))
          {
            File->GetRights()->SetOctal(Entry->Permissions);
          }
        }
        catch (...)
        {
          // ignore permissions errors with FTP
        }

        const wchar_t * Space = wcschr(Entry->OwnerGroup, L' ');
        if (Space != nullptr)
        {
          File->GetFileOwner().SetName(UnicodeString(Entry->OwnerGroup, Space - Entry->OwnerGroup));
          File->GetFileGroup().SetName(Space + 1);
        }
        else
        {
          File->GetFileOwner().SetName(Entry->OwnerGroup);
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

        TDateTime Modification;
        TModificationFmt ModificationFmt;
        RemoteFileTimeToDateTimeAndPrecision(Entry->Time, Modification, ModificationFmt);
        File->SetModification(Modification);
        File->SetModificationFmt(ModificationFmt);
        File->SetLastAccess(File->GetModification());

        File->SetLinkTo(Entry->LinkTarget);

        File->Complete();
      }
      catch (Exception & E)
      {
        UnicodeString EntryData =
          FORMAT(L"%s/%s/%s/%s/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d",
             Entry->Name, Entry->Permissions, Entry->OwnerGroup, Int64ToStr(Entry->Size).c_str(),
             int(Entry->Dir), int(Entry->Link), Entry->Time.Year, Entry->Time.Month, Entry->Time.Day,
             Entry->Time.Hour, Entry->Time.Minute, int(Entry->Time.HasTime),
             int(Entry->Time.HasSeconds), int(Entry->Time.HasDate));
        throw ETerminal(&E, FMTLOAD(LIST_LINE_ERROR, EntryData.c_str()), HELP_LIST_LINE_ERROR);
      }

      FFileList->AddFile(File.release());
    }
    return true;
  }
}
//---------------------------------------------------------------------------
bool TFTPFileSystem::HandleTransferStatus(bool Valid, int64_t TransferSize,
  int64_t Bytes, intptr_t /*Percent*/, intptr_t /*TimeElapsed*/, intptr_t /*TimeLeft*/, intptr_t /*TransferRate*/,
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
bool TFTPFileSystem::HandleReply(intptr_t Command, uintptr_t Reply)
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
bool TFTPFileSystem::HandleCapabilities(
  TFTPServerCapabilities * ServerCapabilities)
{
  FServerCapabilities->Assign(ServerCapabilities);
  FFileSystemInfoValid = false;
  return true;
}
//---------------------------------------------------------------------------
bool TFTPFileSystem::CheckError(intptr_t ReturnCode, const wchar_t * Context)
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
      FMTLOAD(INTERNAL_ERROR, FORMAT(L"fz#%s", Context).c_str(), IntToHex(ReturnCode, 4).c_str()));
    assert(false);
  }

  return false;
}
//---------------------------------------------------------------------------
bool TFTPFileSystem::Unquote(UnicodeString & Str)
{
  enum
  {
    INIT,
    QUOTE,
    QUOTED,
    DONE
  } State;

  State = INIT;
  assert((Str.Length() > 0) && ((Str[1] == L'"') || (Str[1] == L'\'')));

  intptr_t Index = 1;
  wchar_t Quote = 0;
  while (Index <= Str.Length())
  {
    switch (State)
    {
      case INIT:
        if ((Str[Index] == L'"') || (Str[Index] == L'\''))
        {
          Quote = Str[Index];
          State = QUOTED;
          Str.Delete(Index, 1);
        }
        else
        {
          assert(false);
          // no quoted string
          Str.SetLength(0);
        }
        break;

      case QUOTED:
        if (Str[Index] == Quote)
        {
          State = QUOTE;
          Str.Delete(Index, 1);
        }
        else
        {
          ++Index;
        }
        break;

      case QUOTE:
        if (Str[Index] == Quote)
        {
          ++Index;
        }
        else
        {
          // end of quoted string, trim the rest
          Str.SetLength(Index - 1);
          State = DONE;
        }
        break;
    }
  }

  return (State == DONE);
}
//---------------------------------------------------------------------------
void TFTPFileSystem::PreserveDownloadFileTime(HANDLE Handle, void * UserData)
{
  TFileTransferData * Data = NB_STATIC_DOWNCAST(TFileTransferData, UserData);
  FILETIME WrTime = ::DateTimeToFileTime(Data->Modification, dstmUnix);
  SetFileTime(Handle, nullptr, nullptr, &WrTime);
}
//---------------------------------------------------------------------------
bool TFTPFileSystem::GetFileModificationTimeInUtc(const wchar_t * FileName, struct tm & Time)
{
  bool Result;
  try
  {
    // error-handling-free and DST-mode-inaware copy of TTerminal::OpenLocalFile
    HANDLE Handle = ::CreateFile(FileName, GENERIC_READ,
      FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, 0);
    if (Handle == INVALID_HANDLE_VALUE)
    {
      Result = false;
    }
    else
    {
      FILETIME MTime;
      if (!GetFileTime(Handle, nullptr, nullptr, &MTime))
      {
        Result = false;
      }
      else
      {
        TDateTime Modification = ::ConvertTimestampToUTC(::FileTimeToDateTime(MTime));

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

      ::CloseHandle(Handle);
    }
  }
  catch (...)
  {
    Result = false;
  }
  return Result;
}
//---------------------------------------------------------------------------
#endif // NO_FILEZILLA
