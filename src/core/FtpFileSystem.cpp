//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#ifndef NO_FILEZILLA
#define TRACE_FZAPI NOTRACING
//---------------------------------------------------------------------------
#include <list>
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
  FILE_OPERATION_LOOP_CUSTOM(FTerminal, ALLOW_SKIP, MESSAGE, OPERATION)
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
    __int64 Size1, __int64 Size2, time_t LocalTime,
    bool HasLocalTime, const TRemoteFileTime & RemoteTime, void * UserData, int & RequestResult);
  virtual bool HandleAsynchRequestVerifyCertificate(
    const TFtpsCertificateData & Data, int & RequestResult);
  virtual bool HandleAsynchRequestNeedPass(
    struct TNeedPassRequestData & Data, int & RequestResult);
  virtual bool HandleListData(const wchar_t * Path, const TListDataEntry * Entries,
    unsigned int Count);
  virtual bool HandleTransferStatus(bool Valid, __int64 TransferSize,
    __int64 Bytes, int Percent, int TimeElapsed, int TimeLeft, int TransferRate,
    bool FileTransfer);
  virtual bool HandleReply(int Command, unsigned int Reply);
  virtual bool HandleCapabilities(TFTPServerCapabilities * ServerCapabilities);
  virtual bool CheckError(int ReturnCode, const wchar_t * Context);

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
  CCALLSTACK(TRACE_FZAPI);
  return FFileSystem->GetOption(OptionID);
}
//---------------------------------------------------------------------------
intptr_t TFileZillaImpl::OptionVal(intptr_t OptionID) const
{
  CCALLSTACK(TRACE_FZAPI);
  return FFileSystem->GetOptionVal(OptionID);
}
//---------------------------------------------------------------------------
bool TFileZillaImpl::DoPostMessage(TMessageType Type, WPARAM wParam, LPARAM lParam)
{
  CCALLSTACK(TRACE_FZAPI);
  return FFileSystem->PostMessage(Type, wParam, lParam);
}
//---------------------------------------------------------------------------
bool TFileZillaImpl::HandleStatus(const wchar_t * Status, int Type)
{
  CALLSTACK;
  return FFileSystem->HandleStatus(Status, Type);
}
//---------------------------------------------------------------------------
bool TFileZillaImpl::HandleAsynchRequestOverwrite(
  wchar_t * FileName1, size_t FileName1Len, const wchar_t * FileName2,
  const wchar_t * Path1, const wchar_t * Path2,
  __int64 Size1, __int64 Size2, time_t LocalTime,
  bool HasLocalTime, const TRemoteFileTime & RemoteTime, void * UserData, int & RequestResult)
{
  return FFileSystem->HandleAsynchRequestOverwrite(
    FileName1, FileName1Len, FileName2, Path1, Path2, Size1, Size2, LocalTime,
    HasLocalTime, RemoteTime, UserData, RequestResult);
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
  const TListDataEntry * Entries, unsigned int Count)
{
  return FFileSystem->HandleListData(Path, Entries, Count);
}
//---------------------------------------------------------------------------
bool TFileZillaImpl::HandleTransferStatus(bool Valid, __int64 TransferSize,
  __int64 Bytes, int Percent, int TimeElapsed, int TimeLeft, int TransferRate,
  bool FileTransfer)
{
  return FFileSystem->HandleTransferStatus(Valid, TransferSize, Bytes, Percent,
    TimeElapsed, TimeLeft, TransferRate, FileTransfer);
}
//---------------------------------------------------------------------------
bool TFileZillaImpl::HandleReply(int Command, unsigned int Reply)
{
  return FFileSystem->HandleReply(Command, Reply);
}
//---------------------------------------------------------------------------
bool TFileZillaImpl::HandleCapabilities(TFTPServerCapabilities * ServerCapabilities)
{
  return FFileSystem->HandleCapabilities(ServerCapabilities);
}
//---------------------------------------------------------------------------
bool TFileZillaImpl::CheckError(int ReturnCode, const wchar_t * Context)
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
//---------------------------------------------------------------------------
class TMessageQueue : public TObject, public std::list<std::pair<WPARAM, LPARAM>, custom_nballocator_t<std::pair<WPARAM, LPARAM> > >
{
};
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
#ifndef _MSC_VER
struct TFileTransferData
{
  TFileTransferData()
  {
    Params = 0;
    AutoResume = false;
    OverwriteResult = -1;
    CopyParam = NULL;
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
const wchar_t CertificateStorageKey[] = L"FtpsCertificates";
//---------------------------------------------------------------------------
#ifndef _MSC_VER
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
  FFileZillaIntf(NULL),
  FQueueCriticalSection(new TCriticalSection()),
  FTransferStatusCriticalSection(new TCriticalSection()),
  FQueue(new TMessageQueue()),
  FQueueEvent(CreateEvent(NULL, true, false, NULL)),
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
  FLastError(new TStringList()),
  FFeatures(new TStringList()),
  FFileList(NULL),
  FFileListCache(NULL),
  FActive(false),
  FOpening(false),
  FWaitingForReply(false),
  FFileTransferAbort(ftaNone),
  FIgnoreFileList(false),
  FFileTransferCancelled(false),
  FFileTransferResumed(0),
  FFileTransferPreserveTime(false),
  FFileTransferCPSLimit(0),
  FAwaitingProgress(false),
  FOnCaptureOutput(NULL),
  FListAll(asOn),
  FDoListAll(false),
  FServerCapabilities(NULL)
{
  CALLSTACK;
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
  CALLSTACK;
  assert(FFileList == NULL);

  FFileZillaIntf->Destroying();

  // to release memory associated with the messages
  DiscardMessages();

  delete FFileZillaIntf;
  FFileZillaIntf = NULL;

  delete FQueue;
  FQueue = NULL;

  CloseHandle(FQueueEvent);

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
void TFTPFileSystem::Open()
{
  CALLSTACK;
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

  FLastDataSent = Now();

  FMultineResponse = false;

  // initialize FZAPI on the first connect only
  if (FFileZillaIntf == NULL)
  {
    TRACE("1");
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
//!CLEANBEGIN
          #ifdef _DEBUG
          LogLevel = TFileZillaIntf::LOG_DEBUG;
          #else
//!CLEANEND
          LogLevel = TFileZillaIntf::LOG_INFO;
//!CLEANBEGIN
          #endif
//!CLEANEND
          break;
      }
      FFileZillaIntf->SetDebugLevel(LogLevel);

      TRACE("2");
      FFileZillaIntf->Init();
    }
    catch(...)
    {
      TRACE("E");
      delete FFileZillaIntf;
      FFileZillaIntf = NULL;
      throw;
    }
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
  int TimeZoneOffset = TimeToMinutes(Data->GetTimeDifference());
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
    TRACE("3");
    FSystem = L"";
    FFeatures->Clear();
    FFileSystemInfoValid = false;

    // TODO: the same for account? it ever used?

    // ask for username if it was not specified in advance, even on retry,
    // but keep previous one as default,
    if (Data->GetUserNameExpanded().IsEmpty())
    {
      TRACE("4");
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

    // on retry ask for password
    if (FPasswordFailed)
    {
      TRACE("5");
      FTerminal->LogEvent(L"Password prompt (last login attempt failed)");

      // on retry ask for new password
      Password = L"";
      if (!FTerminal->PromptUser(Data, pkPassword, LoadStr(PASSWORD_TITLE), L"",
            LoadStr(PASSWORD_PROMPT), false, 0, Password))
      {
        FTerminal->FatalError(NULL, LoadStr(AUTHENTICATION_FAILED));
      }
    }

    FPasswordFailed = false;
    FOpening = true;
    TBoolRestorer OpeningRestorer(FOpening);

    TRACE("connect");
    FActive = FFileZillaIntf->Connect(
      HostName.c_str(), static_cast<int>(Data->GetPortNumber()), UserName.c_str(),
      Password.c_str(), Account.c_str(), false, Path.c_str(),
      ServerType, Pasv, TimeZoneOffset, UTF8, static_cast<int>(Data->GetFtpForcePasvIp()),
      Data->GetFtpUseMlsd());

    assert(FActive);

    try
    {
      TRACE("wait");
      // do not wait for FTP response code as Connect is complex operation
      GotReply(WaitForCommandReply(false), REPLY_CONNECT, LoadStr(CONNECTION_FAILED));

      Shred(Password);

      // we have passed, even if we got 530 on the way (if it is possible at all),
      // ignore it
      assert(!FPasswordFailed);
      FPasswordFailed = false;
    }
    catch(...)
    {
      TRACE("E2");
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
  TRACE("/");
}
//---------------------------------------------------------------------------
void TFTPFileSystem::Close()
{
  CALLSTACK;
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

  if (Result)
  {
    assert(FActive);
    Discard();
    FTerminal->Closed();
  }
}
//---------------------------------------------------------------------------
bool TFTPFileSystem::GetActive()
{
  return FActive;
}
//---------------------------------------------------------------------------
void TFTPFileSystem::Idle()
{
  CALLSTACK;
  if (FActive && !FWaitingForReply)
  {
    try
    {
      PoolForFatalNonCommandReply();
    }
    catch (EFatal & E)
    {
      if (!FTerminal->QueryReopen(&E, ropNoReadDirectory, NULL))
      {
        throw;
      }
    }

    // Keep session alive
    if ((FTerminal->GetSessionData()->GetFtpPingType() != ptOff) &&
        (static_cast<double>(Now() - FLastDataSent) > static_cast<double>(FTerminal->GetSessionData()->GetFtpPingIntervalDT()) * 4))
    {
      FLastDataSent = Now();

      TRemoteDirectory * Files = new TRemoteDirectory(FTerminal);
      TRY_FINALLY (
      {
        try
        {
          Files->SetDirectory(GetCurrentDirectory());
          DoReadDirectory(Files);
        }
        catch(...)
        {
          // ignore non-fatal errors
          // (i.e. current directory may not exist anymore)
          if (!FTerminal->GetActive())
          {
            throw;
          }
        }
      }
      ,
      {
        delete Files;
      }
      );
    }
  }
}
//---------------------------------------------------------------------------
void TFTPFileSystem::Discard()
{
  CALLSTACK;
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
UnicodeString TFTPFileSystem::ActualCurrentDirectory()
{
  wchar_t CurrentPath[1024];
  FFileZillaIntf->GetCurrentPath(CurrentPath, LENOF(CurrentPath));
  UnicodeString fn = UnixExcludeTrailingBackslash(CurrentPath);
  if (fn.IsEmpty())
  {
    fn = L"/";
  }
  return fn;
}
//---------------------------------------------------------------------------
void TFTPFileSystem::EnsureLocation()
{
  CALLSTACK;
  // if we do not know what's the current directory, do nothing
  if (!FCurrentDirectory.IsEmpty())
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
void TFTPFileSystem::AnyCommand(const UnicodeString & Command,
  TCaptureOutputEvent OutputEvent)
{
  // end-user has right to expect that client current directory is really
  // current directory for the server
  EnsureLocation();

  assert(FOnCaptureOutput == NULL);
  FOnCaptureOutput = OutputEvent;
  TRY_FINALLY (
  {
    FFileZillaIntf->CustomCommand(Command.c_str());

    GotReply(WaitForCommandReply(), REPLY_2XX_CODE | REPLY_3XX_CODE);
  }
  ,
  {
    FOnCaptureOutput = NULL;
  }
  );
}
//---------------------------------------------------------------------------
void TFTPFileSystem::ResetCaches()
{
  delete FFileListCache;
  FFileListCache = NULL;
}
//---------------------------------------------------------------------------
void TFTPFileSystem::AnnounceFileListOperation()
{
  ResetCaches();
}
//---------------------------------------------------------------------------
void TFTPFileSystem::DoChangeDirectory(const UnicodeString & Directory)
{
  CALLSTACK;
  UnicodeString Command = FORMAT(L"CWD %s", Directory.c_str());
  FFileZillaIntf->CustomCommand(Command.c_str());

  GotReply(WaitForCommandReply(), REPLY_2XX_CODE);
}
//---------------------------------------------------------------------------
void TFTPFileSystem::ChangeDirectory(const UnicodeString & ADirectory)
{
  CALLSTACK;
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
  catch(...)
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
  FCurrentDirectory = UnixExcludeTrailingBackslash(Directory);
  if (FCurrentDirectory.IsEmpty())
  {
    FCurrentDirectory = L"/";
  }
}
//---------------------------------------------------------------------------
void TFTPFileSystem::ChangeFileProperties(const UnicodeString & AFileName,
  const TRemoteFile * File, const TRemoteProperties * Properties,
  TChmodSessionAction & Action)
{
  assert(Properties);
  assert(!Properties->Valid.Contains(vpGroup)); //-V595
  assert(!Properties->Valid.Contains(vpOwner)); //-V595
  assert(!Properties->Valid.Contains(vpLastAccess)); //-V595
  assert(!Properties->Valid.Contains(vpModification)); //-V595

  if (Properties && Properties->Valid.Contains(vpRights))
  {
    TRemoteFile * OwnedFile = NULL;

    TRY_FINALLY (
    {
      UnicodeString FileName = AbsolutePath(AFileName, false);

      if (File == NULL)
      {
        ReadFile(FileName, OwnedFile);
        File = OwnedFile;
      }

      if ((File != NULL) && File->GetIsDirectory() && !File->GetIsSymLink() && Properties->Recursive)
      {
        try
        {
          FTerminal->ProcessDirectory(AFileName, MAKE_CALLBACK(TTerminal::ChangeFileProperties, FTerminal),
            static_cast<void *>(const_cast<TRemoteProperties *>(Properties)));
        }
        catch(...)
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

      UnicodeString FileNameOnly = UnixExtractFileName(FileName);
      UnicodeString FilePath = UnixExtractFilePath(FileName);
      // FZAPI wants octal number represented as decadic
      FFileZillaIntf->Chmod(Rights.GetNumberDecadic(), FileNameOnly.c_str(), FilePath.c_str());

      GotReply(WaitForCommandReply(), REPLY_2XX_CODE);
    }
    ,
    {
      delete OwnedFile;
    }
    );
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
  const TOverwriteFileParams * FileParams)
{
  bool Result;
  bool CanAutoResume = FLAGSET(Params, cpNoConfirmation) && AutoResume;
  bool DestIsSmaller = (FileParams != NULL) && (FileParams->DestSize < FileParams->SourceSize);
  bool DestIsSame = (FileParams != NULL) && (FileParams->DestSize == FileParams->SourceSize);
  // when resuming transfer after interrupted connection,
  // do nothing (dummy resume) when the files has the same size.
  // this is workaround for servers that strangely fails just after successful
  // upload.
  bool CanResume =
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
  CALLSTACK;
  FFileTransferAbort = ftaNone;
  FFileTransferCancelled = false;
  FFileTransferResumed = 0;
}
//---------------------------------------------------------------------------
void TFTPFileSystem::ReadDirectoryProgress(__int64 Bytes)
{
  CALLSTACK;
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
      TRACE("Cancel");
      FFileZillaIntf->Cancel();
    }
  }
}
//---------------------------------------------------------------------------
void TFTPFileSystem::DoFileTransferProgress(__int64 TransferSize,
  __int64 Bytes)
{
  CALLSTACK;
  TFileOperationProgressType * OperationProgress = FTerminal->GetOperationProgress();

  OperationProgress->SetTransferSize(TransferSize);

  if (FFileTransferResumed > 0)
  {
    OperationProgress->AddResumed(FFileTransferResumed);
    FFileTransferResumed = 0;
  }

  __int64 Diff = Bytes - OperationProgress->TransferedSize;
  TRACEFMT("TransferSize[%d] Bytes[%d] TransferedSize[%d] Diff[%d]", int(TransferSize), int(Bytes), int(OperationProgress->TransferedSize), int(Diff));
  assert(Diff >= 0);
  if (Diff >= 0)
  {
    OperationProgress->AddTransfered(Diff);
  }

  if (OperationProgress->Cancel == csCancel)
  {
    FFileTransferCancelled = true;
    FFileTransferAbort = ftaCancel;
    TRACE("Cancel");
    FFileZillaIntf->Cancel();
  }

  if (FFileTransferCPSLimit != OperationProgress->CPSLimit)
  {
    FFileTransferCPSLimit = OperationProgress->CPSLimit;
  }
}
//---------------------------------------------------------------------------
void TFTPFileSystem::FileTransferProgress(__int64 TransferSize,
  __int64 Bytes)
{
  CALLSTACK;
  TGuard Guard(FTransferStatusCriticalSection);

  TRACE("1");
  DoFileTransferProgress(TransferSize, Bytes);
}
//---------------------------------------------------------------------------
void TFTPFileSystem::FileTransfer(const UnicodeString & FileName,
  const UnicodeString & LocalFile, const UnicodeString & RemoteFile,
  const UnicodeString & RemotePath, bool Get, __int64 Size, int Type,
  TFileTransferData & UserData, TFileOperationProgressType * OperationProgress)
{
  CALLSTACK;
  FILE_OPERATION_LOOP(FMTLOAD(TRANSFER_ERROR, FileName.c_str()),
    FFileZillaIntf->FileTransfer(LocalFile.c_str(), RemoteFile.c_str(),
      RemotePath.c_str(), Get, Size, Type, &UserData);
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
void TFTPFileSystem::CopyToLocal(TStrings * FilesToCopy,
  const UnicodeString & TargetDir, const TCopyParamType * CopyParam,
  intptr_t Params, TFileOperationProgressType * OperationProgress,
  TOnceDoneOperation & OnceDoneOperation)
{
  CALLSTACK;
  Params &= ~cpAppend;
  UnicodeString FullTargetDir = IncludeTrailingBackslash(TargetDir);

  intptr_t Index = 0;
  while (Index < FilesToCopy->GetCount() && !OperationProgress->Cancel)
  {
    UnicodeString FileName = FilesToCopy->Strings[Index];
    const TRemoteFile * File = dynamic_cast<const TRemoteFile *>(FilesToCopy->Objects[Index]);
    bool Success = false;

    TRY_FINALLY (
    {
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
      catch(EScpSkipFile & E)
      {
        SUSPEND_OPERATION (
          if (!FTerminal->HandleException(&E)) throw;
        );
      }
    }
    ,
    {
      OperationProgress->Finish(FileName, Success, OnceDoneOperation);
    }
    );
    ++Index;
  }
}
//---------------------------------------------------------------------------
void TFTPFileSystem::SinkRobust(const UnicodeString & FileName,
  const TRemoteFile * File, const UnicodeString & TargetDir,
  const TCopyParamType * CopyParam, intptr_t Params,
  TFileOperationProgressType * OperationProgress, uintptr_t Flags)
{
  CALLSTACK;
  // the same in TSFTPFileSystem
  bool Retry;

  TDownloadSessionAction Action(FTerminal->GetActionLog());

  do
  {
    Retry = false;
    try
    {
      Sink(FileName, File, TargetDir, CopyParam, Params, OperationProgress,
        Flags, Action);
    }
    catch(Exception & E)
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
void TFTPFileSystem::Sink(const UnicodeString & FileName,
  const TRemoteFile * File, const UnicodeString & TargetDir,
  const TCopyParamType * CopyParam, intptr_t Params,
  TFileOperationProgressType * OperationProgress, uintptr_t Flags,
  TDownloadSessionAction & Action)
{
  CALLSTACK;
  UnicodeString OnlyFileName = UnixExtractFileName(FileName);

  Action.FileName(FileName);

  TFileMasks::TParams MaskParams;
  assert(File);
  MaskParams.Size = File->GetSize();
  MaskParams.Modification = File->GetModification();

  if (!CopyParam->AllowTransfer(FileName, osRemote, File->GetIsDirectory(), MaskParams))
  {
    FTerminal->LogEvent(FORMAT(L"File \"%s\" excluded from transfer", FileName.c_str()));
    THROW_SKIP_FILE_NULL;
  }

  FTerminal->LogEvent(FORMAT(L"File: \"%s\"", FileName.c_str()));

  OperationProgress->SetFile(OnlyFileName);

  UnicodeString DestFileName = CopyParam->ChangeFileName(UnixExtractFileName(File->GetFileName()),
    osRemote, FLAGSET(Flags, tfFirstLevel));
  UnicodeString DestFullName = TargetDir + DestFileName;

  if (File->GetIsDirectory())
  {
    Action.Cancel();
    if (!File->GetIsSymLink())
    {
      FILE_OPERATION_LOOP (FMTLOAD(NOT_DIRECTORY_ERROR, DestFullName.c_str()),
        int Attrs = FTerminal->GetLocalFileAttributes(DestFullName);
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

      FTerminal->ProcessDirectory(FileName, MAKE_CALLBACK(TFTPFileSystem::SinkFile, this), &SinkFileParams);

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
    FTerminal->LogEvent(UnicodeString((OperationProgress->AsciiTransfer ? L"Ascii" : L"Binary")) +
      L" transfer mode selected.");

    // Suppose same data size to transfer as to write
    OperationProgress->SetTransferSize(File->GetSize());
    OperationProgress->SetLocalSize(OperationProgress->TransferSize);

    int Attrs = 0;
    FILE_OPERATION_LOOP (FMTLOAD(NOT_FILE_ERROR, DestFullName.c_str()),
      Attrs = FTerminal->GetLocalFileAttributes(DestFullName);
      if ((Attrs >= 0) && FLAGSET(Attrs, faDirectory))
      {
        EXCEPTION;
      }
    );

    OperationProgress->TransferingFile = false; // not set with FTP protocol

    ResetFileTransfer();

    TFileTransferData UserData;

    UnicodeString FilePath = UnixExtractFilePath(FileName);
    if (FilePath.IsEmpty())
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
      UserData.Modification = File->GetModification();
      FileTransfer(FileName, DestFullName, OnlyFileName,
        FilePath, true, File->GetSize(), TransferType, UserData, OperationProgress);
    }

    // in case dest filename is changed from overwrite dialog
    if (DestFileName != UserData.FileName)
    {
      DestFullName = TargetDir + UserData.FileName;
      Attrs = FTerminal->GetLocalFileAttributes(DestFullName);
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
        THROWOSIFFALSE(FTerminal->SetLocalFileAttributes(DestFullName, Attrs | NewAttrs) == 0);
      );
    }
  }

  if (FLAGSET(Params, cpDelete))
  {
    // If file is directory, do not delete it recursively, because it should be
    // empty already. If not, it should not be deleted (some files were
    // skipped or some new files were copied to it, while we were downloading)
    intptr_t Params = dfNoRecursive;
    FTerminal->DeleteFile(FileName, File, &Params);
  }
}
//---------------------------------------------------------------------------
void TFTPFileSystem::SinkFile(const UnicodeString & FileName,
  const TRemoteFile * File, void * Param)
{
  CALLSTACK;
  TSinkFileParams * Params = static_cast<TSinkFileParams *>(Param);
  assert(Params->OperationProgress);
  try
  {
    SinkRobust(FileName, File, Params->TargetDir, Params->CopyParam,
      Params->Params, Params->OperationProgress, Params->Flags);
  }
  catch(EScpSkipFile & E)
  {
    TFileOperationProgressType * OperationProgress = Params->OperationProgress;

    Params->Skipped = true;

    SUSPEND_OPERATION (
      if (!FTerminal->HandleException(&E))
      {
        throw;
      }
    );

    if (OperationProgress->Cancel)
    {
      Abort();
    }
  }
}
//---------------------------------------------------------------------------
void TFTPFileSystem::CopyToRemote(TStrings * FilesToCopy,
  const UnicodeString & ATargetDir, const TCopyParamType * CopyParam,
  intptr_t Params, TFileOperationProgressType * OperationProgress,
  TOnceDoneOperation & OnceDoneOperation)
{
  CALLSTACK;
  assert((FilesToCopy != NULL) && (OperationProgress != NULL));

  Params &= ~cpAppend;
  UnicodeString FileName, FileNameOnly;
  UnicodeString TargetDir = AbsolutePath(ATargetDir, false);
  UnicodeString FullTargetDir = UnixIncludeTrailingBackslash(TargetDir);
  intptr_t Index = 0;
  while ((Index < FilesToCopy->GetCount()) && !OperationProgress->Cancel)
  {
    bool Success = false;
    FileName = FilesToCopy->Strings[Index];
    TRACEFMT("1 [%s]", FileName.c_str());
    TRemoteFile * File = dynamic_cast<TRemoteFile *>(FilesToCopy->Objects[Index]);
    UnicodeString RealFileName = File ? File->GetFileName() : FileName;

    FileNameOnly = ExtractFileName(RealFileName, false);

    TRY_FINALLY (
    {
      try
      {
        if (FTerminal->GetSessionData()->GetCacheDirectories())
        {
          TRACE("2");
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
      catch(EScpSkipFile & E)
      {
        TRACE("3");
        SUSPEND_OPERATION (
          if (!FTerminal->HandleException(&E)) throw;
        );
      }
    }
    ,
    {
      TRACE("4");
      OperationProgress->Finish(FileName, Success, OnceDoneOperation);
    }
    );
    ++Index;
  }
  TRACE("/");
}
//---------------------------------------------------------------------------
void TFTPFileSystem::SourceRobust(const UnicodeString & FileName,
  const TRemoteFile * File,
  const UnicodeString & TargetDir, const TCopyParamType * CopyParam, intptr_t Params,
  TFileOperationProgressType * OperationProgress, uintptr_t Flags)
{
  CALLSTACK;
  TRACEFMT("0 [%s]", FileName.c_str());
  // the same in TSFTPFileSystem
  bool Retry;

  TUploadSessionAction Action(FTerminal->GetActionLog());
  TOpenRemoteFileParams OpenParams;
  OpenParams.OverwriteMode = omOverwrite;
  TOverwriteFileParams FileParams;

  do
  {
    TRACE("1");
    Retry = false;
    try
    {
      Source(FileName, File, TargetDir, CopyParam, Params, &OpenParams, &FileParams, OperationProgress,
        Flags, Action);
    }
    catch(Exception & E)
    {
      TRACE("2");
      Retry = true;
      if (FTerminal->GetActive() ||
          !FTerminal->QueryReopen(&E, ropNoReadDirectory, OperationProgress))
      {
        TRACE("3");
        FTerminal->RollbackAction(Action, OperationProgress, &E);
        throw;
      }
    }

    if (Retry)
    {
      TRACE("4");
      OperationProgress->RollbackTransfer();
      Action.Restart();
      // prevent overwrite confirmations
      // (should not be set for directories!)
      Params |= cpNoConfirmation;
      Flags |= tfAutoResume;
    }
  }
  while (Retry);
  TRACE("/");
}
//---------------------------------------------------------------------------
// Copy file to remote host
void TFTPFileSystem::Source(const UnicodeString & FileName,
  const TRemoteFile * File,
  const UnicodeString & TargetDir, const TCopyParamType * CopyParam, intptr_t Params,
  TOpenRemoteFileParams * OpenParams,
  TOverwriteFileParams * FileParams,
  TFileOperationProgressType * OperationProgress, uintptr_t Flags,
  TUploadSessionAction & Action)
{
  CALLSTACK;
  UnicodeString RealFileName = File ? File->GetFileName() : FileName;
  FTerminal->LogEvent(FORMAT(L"File: \"%s\"", RealFileName.c_str()));

  Action.FileName(ExpandUNCFileName(FileName));

  OperationProgress->SetFile(RealFileName, false);

  if (!FTerminal->AllowLocalFileTransfer(FileName, CopyParam))
  {
    TRACE("1");
    FTerminal->LogEvent(FORMAT(L"File \"%s\" excluded from transfer", RealFileName.c_str()));
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
    TRACE("2");
    Action.Cancel();
    DirectorySource(IncludeTrailingBackslash(RealFileName), TargetDir,
      OpenParams->LocalFileAttrs, CopyParam, Params, OperationProgress, Flags);
  }
  else
  {
    TRACE("3");
    UnicodeString DestFileName = CopyParam->ChangeFileName(ExtractFileName(RealFileName, false),
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
      CopyParam->UseAsciiTransfer(RealFileName, osLocal, MaskParams));
    FTerminal->LogEvent(
      UnicodeString(OperationProgress->AsciiTransfer ? L"Ascii" : L"Binary") +
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
      TRACE("4");
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
    TRACE("5");
    if (!Dir)
    {
      FILE_OPERATION_LOOP (FMTLOAD(DELETE_LOCAL_FILE_ERROR, FileName.c_str()),
        THROWOSIFFALSE(Sysutils::DeleteFile(FileName));
      )
    }
  }
  else if (CopyParam->GetClearArchive() && FLAGSET(OpenParams->LocalFileAttrs, faArchive))
  {
    TRACE("6");
    FILE_OPERATION_LOOP (FMTLOAD(CANT_SET_ATTRS, FileName.c_str()),
      THROWOSIFFALSE(FTerminal->SetLocalFileAttributes(FileName, OpenParams->LocalFileAttrs & ~faArchive) == 0);
    )
  }
  TRACE("/");
}
//---------------------------------------------------------------------------
void TFTPFileSystem::DirectorySource(const UnicodeString & DirectoryName,
  const UnicodeString & TargetDir, intptr_t Attrs, const TCopyParamType * CopyParam,
  intptr_t Params, TFileOperationProgressType * OperationProgress, uintptr_t Flags)
{
  CALLSTACK;
  UnicodeString DestDirectoryName = CopyParam->ChangeFileName(
    ExtractFileName(ExcludeTrailingBackslash(DirectoryName), false), osLocal,
    FLAGSET(Flags, tfFirstLevel));
  UnicodeString DestFullName = UnixIncludeTrailingBackslash(TargetDir + DestDirectoryName);
  TRACEFMT("1 [%s] [%s]", DirectoryName.c_str(), DestFullName.c_str());

  OperationProgress->SetFile(DirectoryName);

  int FindAttrs = faReadOnly | faHidden | faSysFile | faDirectory | faArchive;
  TSearchRec SearchRec;
  bool FindOK = false;

  FILE_OPERATION_LOOP (FMTLOAD(LIST_DIR_ERROR, DirectoryName.c_str()),
    FindOK = (bool)(FindFirst((DirectoryName + L"*.*").c_str(),
      FindAttrs, SearchRec) == 0);
  );

  bool CreateDir = true;

  TRY_FINALLY (
  {
    while (FindOK && !OperationProgress->Cancel)
    {
      UnicodeString FileName = DirectoryName + SearchRec.Name;
      TRACEFMT("1a [%s]", FileName.c_str());
      try
      {
        if ((SearchRec.Name != THISDIRECTORY) && (SearchRec.Name != PARENTDIRECTORY))
        {
          TRACE("2");
          SourceRobust(FileName, NULL, DestFullName, CopyParam, Params, OperationProgress,
            Flags & ~(tfFirstLevel | tfAutoResume));
          // if any file got uploaded (i.e. there were any file in the
          // directory and at least one was not skipped),
          // do not try to create the directory,
          // as it should be already created by FZAPI during upload
          CreateDir = false;
        }
      }
      catch (EScpSkipFile &E)
      {
        TRACE("3");
        // If ESkipFile occurs, just log it and continue with next file
        SUSPEND_OPERATION (
          // here a message to user was displayed, which was not appropriate
          // when user refused to overwrite the file in subdirectory.
          // hopefuly it won't be missing in other situations.
          if (!FTerminal->HandleException(&E)) throw;
        );
      }

      FILE_OPERATION_LOOP (FMTLOAD(LIST_DIR_ERROR, DirectoryName.c_str()),
        FindOK = (FindNext(SearchRec) == 0);
      );
      TRACEFMT("3a [%d] [%d]", int(FindOK), int(OperationProgress->Cancel));
    }
  }
  ,
  {
    FindClose(SearchRec);
  }
  );

  if (CreateDir)
  {
    TRACE("4");
    TRemoteProperties Properties;
    if (CopyParam->GetPreserveRights())
    {
      Properties.Valid = TValidProperties() << vpRights;
      Properties.Rights = CopyParam->RemoteFileRights(Attrs);
    }

    try
    {
      FTerminal->SetExceptionOnFail(true);
      TRY_FINALLY (
      {
        FTerminal->CreateDirectory(DestFullName, &Properties);
      }
      ,
      {
        FTerminal->SetExceptionOnFail(false);
      }
      );
    }
    catch(...)
    {
      TRACE("4a");
      TRemoteFile * File = NULL;
      // ignore non-fatal error when the directory already exists
      UnicodeString fn = UnixExcludeTrailingBackslash(DestFullName);
      if (fn.IsEmpty())
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
        TRACE("4b");
        throw;
      }
    }
  }

  /* TODO : Delete also read-only directories. */
  /* TODO : Show error message on failure. */
  if (!OperationProgress->Cancel)
  {
    TRACE("5");
    if (FLAGSET(Params, cpDelete))
    {
      TRACE("6");
      FTerminal->RemoveLocalDirectory(DirectoryName);
    }
    else if (CopyParam->GetClearArchive() && FLAGSET(Attrs, faArchive))
    {
      TRACE("7");
      FILE_OPERATION_LOOP (FMTLOAD(CANT_SET_ATTRS, DirectoryName.c_str()),
        THROWOSIFFALSE(FTerminal->SetLocalFileAttributes(DirectoryName, Attrs & ~faArchive) == 0);
      )
    }
  }
  TRACE("/");
}
//---------------------------------------------------------------------------
void TFTPFileSystem::CreateDirectory(const UnicodeString & ADirName)
{
  UnicodeString DirName = AbsolutePath(ADirName, false);

  {
    // ignore file list
    TFTPFileListHelper Helper(this, NULL, true);

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
  const TRemoteFile * File, intptr_t Params, TRmSessionAction & Action)
{
  CALLSTACK;
  UnicodeString FileName = AbsolutePath(AFileName, false);
  UnicodeString FileNameOnly = UnixExtractFileName(FileName);
  UnicodeString FilePath = UnixExtractFilePath(FileName);

  bool Dir = (File != NULL) && File->GetIsDirectory() && !File->GetIsSymLink();

  if (Dir && FLAGCLEAR(Params, dfNoRecursive))
  {
    try
    {
      FTerminal->ProcessDirectory(FileName, MAKE_CALLBACK(TTerminal::DeleteFile, FTerminal), &Params);
    }
    catch(...)
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
  CALLSTACK;
  TStrings * PostLoginCommands = new TStringList();
  TRY_FINALLY (
  {
    PostLoginCommands->Text = FTerminal->GetSessionData()->GetPostLoginCommands();
    for (intptr_t Index = 0; Index < PostLoginCommands->GetCount(); ++Index)
    {
      UnicodeString Command = PostLoginCommands->Strings[Index];
      if (!Command.IsEmpty())
      {
        FFileZillaIntf->CustomCommand(Command.c_str());

        GotReply(WaitForCommandReply(), REPLY_2XX_CODE | REPLY_3XX_CODE);
      }
    }
  }
  ,
  {
    delete PostLoginCommands;
  }
  );

  // retrieve initialize working directory to save it as home directory
  ReadCurrentDirectory();
  FHomeDirectory = FCurrentDirectory;
  TRACE("/");
}
//---------------------------------------------------------------------------
void TFTPFileSystem::HomeDirectory()
{
  CALLSTACK;
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
  CALLSTACK;
  // ask the server for current directory on startup only
  // and immediately after call to CWD,
  // later our current directory may be not synchronized with FZAPI current
  // directory anyway, see comments in EnsureLocation
  if (FCurrentDirectory.IsEmpty())
  {
    TRACE("0a");
    FFileZillaIntf->CustomCommand(L"PWD");

    unsigned int Code = 0;
    TStrings * Response = NULL;
    TRACE("0b");
    GotReply(WaitForCommandReply(), REPLY_2XX_CODE, L"", &Code, &Response);

    assert(Response != NULL);
    TRY_FINALLY (
    {
      bool Result = false;

      // the only allowed 2XX code to "PWD"
      if ((Code == 257) &&
          (Response->GetCount() == 1))
      {
        TRACE("1");
        UnicodeString Path = Response->Text;

        intptr_t P = Path.Pos(L"\"");
        if (P == 0)
        {
          TRACE("2");
          // some systems use single quotes, be tolerant
          P = Path.Pos(L"'");
        }
        if (P != 0)
        {
          TRACE("3");
          Path.Delete(1, P - 1);

          if (Unquote(Path))
          {
            TRACEFMT("4 [%s]", Path.c_str());
            FCurrentDirectory = UnixExcludeTrailingBackslash(Path);
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
        TRACE("5");
        FFileZillaIntf->SetCurrentPath(FCurrentDirectory.c_str());
      }
      else
      {
        TRACE("6");
        throw Exception(FMTLOAD(FTP_PWD_RESPONSE_ERROR, Response->Text.get().c_str()));
      }
    }
    ,
    {
      TRACE("7");
      delete Response;
    }
    );
  }
  TRACE("/");
}
//---------------------------------------------------------------------------
void TFTPFileSystem::DoReadDirectory(TRemoteFileList * FileList)
{
  CALLSTACK;
  FileList->Clear();
  // FZAPI does not list parent directory, add it
  FileList->AddFile(new TRemoteParentDirectory(FTerminal));

  FLastReadDirectoryProgress = 0;

  TFTPFileListHelper Helper(this, FileList, false);

  TRACE("1");
  // always specify path to list, do not attempt to list "current" dir as:
  // 1) List() lists again the last listed directory, not the current working directory
  // 2) we handle this way the cached directory change
  UnicodeString Directory = AbsolutePath(FileList->GetDirectory(), false);
  FFileZillaIntf->List(Directory.c_str());

  TRACE("2");
  GotReply(WaitForCommandReply(), REPLY_2XX_CODE | REPLY_ALLOW_CANCEL);

  FLastDataSent = Now();
}
//---------------------------------------------------------------------------
void TFTPFileSystem::ReadDirectory(TRemoteFileList * FileList)
{
  CALLSTACK;
  // whole below "-a" logic is for LIST,
  // if we know we are going to use MLSD, skip it
  if (FServerCapabilities->GetCapability(mlsd_command) == yes)
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
        TRACE("1");
        FDoListAll = (FListAll == asAuto) || (FListAll == asOn);
        DoReadDirectory(FileList);

        TRACEFMT("1a [%d]", FListAll);
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
              ((FileList->GetCount() == 1) && FileList->GetFiles(0)->GetIsParentDirectory()))
          {
            Repeat = true;
            FListAll = asOff;
            GotNoFilesForAll = true;
          }
          else
          {
            // reading first directory has succeeded, always use "-a"
            FListAll = asOn;
            TRACEFMT("1b [%d]", FListAll);
          }
        }

        // use "-a" even for implicit directory reading by FZAPI?
        // (e.g. before file transfer)
        FDoListAll = (FListAll == asOn);
        TRACE("2");
      }
      catch(Exception &)
      {
        FDoListAll = false;
        TRACEFMT("3 [%d]", int(FListAll));
        // reading the first directory has failed,
        // further try without "-a" only as the server may not support it
        if (FListAll == asAuto)
        {
          if (!FTerminal->GetActive())
          {
            TRACE("3b");
            FTerminal->Reopen(ropNoReadDirectory);
          }

          FListAll = asOff;
          Repeat = true;
        }
        else
        {
          TRACE("4");
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

  TRemoteFileList * FileList = new TRemoteFileList();
  TRY_FINALLY (
  {
    TFTPFileListHelper Helper(this, FileList, false);
    FFileZillaIntf->ListFile(FileName.c_str());

    GotReply(WaitForCommandReply(), REPLY_2XX_CODE | REPLY_ALLOW_CANCEL);
    TRemoteFile * File = FileList->FindFile(UnixExtractFileName(FileName));
    if (File != NULL)
    {
      AFile = File->Duplicate();
    }

    FLastDataSent = Now();
  }
  ,
  {
    delete FileList;
  }
  );
}
//---------------------------------------------------------------------------
void TFTPFileSystem::ReadFile(const UnicodeString & FileName,
  TRemoteFile *& File)
{
  CALLSTACK;
  UnicodeString Path = UnixExtractFilePath(FileName);
  UnicodeString NameOnly = UnixExtractFileName(FileName);
  TRemoteFile * AFile = NULL;
  bool Own = false;
  if (FServerCapabilities->GetCapability(mlsd_command) == yes)
  {
    TRACE("0");
    DoReadFile(FileName, AFile);
    Own = true;
  }
  else
  {
    TRACE("1");
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
      TRACE("4");
      TRemoteFileList * FileListCache = new TRemoteFileList();
      FileListCache->SetDirectory(Path);
      try
      {
        ReadDirectory(FileListCache);
      }
      catch(...)
      {
        delete FileListCache;
        throw;
      }
      // set only after we successfully read the directory,
      // otherwise, when we reconnect from ReadDirectory,
      // the FFileListCache is reset from ResetCache.
      delete FFileListCache;
      FFileListCache = FileListCache;
      FFileListCachePath = GetCurrentDirectory();

      AFile = FFileListCache->FindFile(NameOnly);
    }

    Own = false;
  }

  if (AFile == NULL)
  {
    File = NULL;
    throw Exception(FMTLOAD(FILE_NOT_EXISTS, FileName.c_str()));
  }

  TRACEFMT("2 [%d]", int(Own));
  assert(AFile != NULL);
  File = Own ? AFile : AFile->Duplicate();
  TRACE("/");
}
//---------------------------------------------------------------------------
void TFTPFileSystem::ReadSymlink(TRemoteFile * SymlinkFile,
  TRemoteFile *& File)
{
  // Resolving symlinks over FTP is big overhead
  // (involves opening TCPIP connection for retrieving "directory listing").
  // Moreover FZAPI does not support that anyway.
  // Note that while we could use MLST to read the symlink,
  // it's hardly of any use as, if MLST is supported, we use MLSD to
  // retrieve directory listing and from MLSD we cannot atm detect that
  // the file is symlink anyway.
  File = new TRemoteFile(SymlinkFile);
  try
  {
    File->SetTerminal(FTerminal);
    File->SetFileName(UnixExtractFileName(SymlinkFile->GetLinkTo()));
    File->SetType(FILETYPE_SYMLINK);
  }
  catch(...)
  {
    delete File;
    File = NULL;
    throw;
  }
}
//---------------------------------------------------------------------------
void TFTPFileSystem::RenameFile(const UnicodeString & AFileName,
  const UnicodeString & ANewName)
{
  UnicodeString FileName = AbsolutePath(AFileName, false);
  UnicodeString NewName = AbsolutePath(ANewName, false);

  UnicodeString FileNameOnly = UnixExtractFileName(FileName);
  UnicodeString FilePathOnly = UnixExtractFilePath(FileName);
  UnicodeString NewNameOnly = UnixExtractFileName(NewName);
  UnicodeString NewPathOnly = UnixExtractFilePath(NewName);

  {
    // ignore file list
    TFTPFileListHelper Helper(this, NULL, true);

    FFileZillaIntf->Rename(FileNameOnly.c_str(), NewNameOnly.c_str(),
      FilePathOnly.c_str(), NewPathOnly.c_str());

    GotReply(WaitForCommandReply(), REPLY_2XX_CODE);
  }
}
//---------------------------------------------------------------------------
void TFTPFileSystem::CopyFile(const UnicodeString & FileName,
  const UnicodeString & NewName)
{
  assert(false);
}
//---------------------------------------------------------------------------
UnicodeString TFTPFileSystem::FileUrl(const UnicodeString & FileName)
{
  return FTerminal->FileUrl(L"ftp", FileName);
}
//---------------------------------------------------------------------------
TStrings * TFTPFileSystem::GetFixedPaths()
{
  return NULL;
}
//---------------------------------------------------------------------------
void TFTPFileSystem::SpaceAvailable(const UnicodeString & /* Path */,
  TSpaceAvailable & /* ASpaceAvailable */)
{
  assert(false);
}
//---------------------------------------------------------------------------
const TSessionInfo & TFTPFileSystem::GetSessionInfo()
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
        FFileSystemInfo.AdditionalInfo += FORMAT(L"  %s\r\n", FFeatures->Strings[Index].c_str());
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
  TRACEFMT("1 [%d]", OptionID);

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
      TRACEFMT("1 [%d] [%d]", int(FListAll), int(FDoListAll), Result);
      break;

    case OPTION_MPEXT_SSLSESSIONREUSE:
      Result = (Data->GetSslSessionReuse() ? TRUE : FALSE);
      break;

    case OPTION_MPEXT_SNDBUF:
      Result = Data->GetSendBuf();
      break;

    default:
      assert(false);
      Result = FALSE;
      break;
  }

  return Result;
}
//---------------------------------------------------------------------------
bool TFTPFileSystem::PostMessage(unsigned int Type, WPARAM wParam, LPARAM lParam)
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

  TRACEFMT("1 [%x] (%x) [%x]", int(wParam), int((wParam >> 16) & 0xFFFF), int(lParam));
  FQueue->push_back(TMessageQueue::value_type(wParam, lParam));
  SetEvent(FQueueEvent);

  return true;
}
//---------------------------------------------------------------------------
bool TFTPFileSystem::ProcessMessage()
{
  CALLSTACK;
  bool Result;
  TMessageQueue::value_type Message;

  {
    TGuard Guard(FQueueCriticalSection);

    TRACE("1");
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
    TRACEFMT("2 [%x] (%x) [%x]", int(Message.first), int((Message.first >> 16) & 0xFFFF), int(Message.second));
    FFileZillaIntf->HandleMessage(Message.first, Message.second);
  }

  TRACE("/");
  return Result;
}
//---------------------------------------------------------------------------
void TFTPFileSystem::DiscardMessages()
{
  CALLSTACK;
  while (ProcessMessage());
}
//---------------------------------------------------------------------------
void TFTPFileSystem::WaitForMessages()
{
  CALLSTACK;
  unsigned int Result = WaitForSingleObject(FQueueEvent, INFINITE);
  if (Result != WAIT_OBJECT_0)
  {
    TRACE("1");
    FTerminal->FatalError(NULL, FMTLOAD(INTERNAL_ERROR, L"ftp#1", IntToStr(Result).c_str()));
  }
  TRACE("/");
}
//---------------------------------------------------------------------------
void TFTPFileSystem::PoolForFatalNonCommandReply()
{
  CALLSTACK;
  assert(FReply == 0);
  assert(FCommandReply == 0);
  TRACEFMT("1 [%x] FWaitingForReply [%d]", int(this), int(FWaitingForReply));
  assert(!FWaitingForReply);

  FWaitingForReply = true;
  TRACEFMT("2 [%x] FWaitingForReply [%d]", int(this), int(FWaitingForReply));

  unsigned int Reply = 0;

  TRY_FINALLY (
  {
    // discard up to one reply
    // (it should not happen here that two replies are posted anyway)
    while (ProcessMessage() && (FReply == 0));
    Reply = FReply;
  }
  ,
  {
    FReply = 0;
    assert(FCommandReply == 0);
    FCommandReply = 0;
    assert(FWaitingForReply);
    FWaitingForReply = false;
    TRACEFMT("3 [%x] FWaitingForReply [%d]", int(this), int(FWaitingForReply));
  }
  );

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
bool TFTPFileSystem::KeepWaitingForReply(unsigned int & ReplyToAwait, bool WantLastCode) const
{
  CALLSTACK;
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
void TFTPFileSystem::DoWaitForReply(unsigned int & ReplyToAwait, bool WantLastCode)
{
  CALLSTACK;
  try
  {
    while (KeepWaitingForReply(ReplyToAwait, WantLastCode))
    {
      TRACE("1");
      WaitForMessages();
      // wait for the first reply only,
      // i.e. in case two replies are posted get the first only.
      // e.g. when server closes the connection, but posts error message before,
      // sometime it happens that command (like download) fails because of the error
      // and does not catch the disconnection. then asynchronous "disconnect reply"
      // is posted immediately afterwards. leave detection of that to Idle()
      while (ProcessMessage() && KeepWaitingForReply(ReplyToAwait, WantLastCode));
    }
    TRACE("2");

    if (FReply != 0)
    {
      // throws
      GotNonCommandReply(FReply);
    }
  }
  catch(...)
  {
    TRACE("3");
    // even if non-fatal error happens, we must process pending message,
    // so that we "eat" the reply message, so that it gets not mistakenly
    // associated with future connect
    if (FTerminal->GetActive())
    {
      TRACE("4");
      DoWaitForReply(ReplyToAwait, WantLastCode);
    }
    throw;
  }
  TRACE("/");
}
//---------------------------------------------------------------------------
unsigned int TFTPFileSystem::WaitForReply(bool Command, bool WantLastCode)
{
  CALLSTACK;
  assert(FReply == 0);
  assert(FCommandReply == 0);
  assert(!FWaitingForReply);
  assert(!FTransferStatusCriticalSection->GetAcquired());

  ResetReply();
  FWaitingForReply = true;
  TRACEFMT("2 [%x] FWaitingForReply [%d]", int(this), int(FWaitingForReply));

  unsigned int Reply = 0;

  TRY_FINALLY (
  {
    TRACE("3");
    unsigned int & ReplyToAwait = (Command ? FCommandReply : FReply);
    DoWaitForReply(ReplyToAwait, WantLastCode);

    Reply = ReplyToAwait;
    TRACE("4");
  }
  ,
  {
    TRACE("5");
    FReply = 0;
    FCommandReply = 0;
    assert(FWaitingForReply);
    FWaitingForReply = false;
  }
  );

  TRACE("/");
  return Reply;
}
//---------------------------------------------------------------------------
unsigned int TFTPFileSystem::WaitForCommandReply(bool WantLastCode)
{
  CALLSTACK;
  return WaitForReply(true, WantLastCode);
}
//---------------------------------------------------------------------------
void TFTPFileSystem::WaitForFatalNonCommandReply()
{
  CALLSTACK;
  WaitForReply(false, false);
  assert(false);
}
//---------------------------------------------------------------------------
void TFTPFileSystem::ResetReply()
{
  CALLSTACK;
  FLastCode = 0;
  FLastCodeClass = 0;
  assert(FLastResponse != NULL);
  FLastResponse->Clear();
  assert(FLastError != NULL);
  FLastError->Clear();
  TRACE("/");
}
//---------------------------------------------------------------------------
void TFTPFileSystem::GotNonCommandReply(unsigned int Reply)
{
  assert(FLAGSET(Reply, TFileZillaIntf::REPLY_DISCONNECTED));
  GotReply(Reply);
  // should never get here as GotReply should raise fatal exception
  assert(false);
}
//---------------------------------------------------------------------------
void TFTPFileSystem::GotReply(unsigned int Reply, uintptr_t Flags,
  const UnicodeString & Error, unsigned int * Code, TStrings ** Response)
{
  CALLSTACK;
  TRY_FINALLY (
  {
    if (FLAGSET(Reply, TFileZillaIntf::REPLY_OK))
    {
      TRACE("2");
      assert(Reply == TFileZillaIntf::REPLY_OK);

      // With REPLY_2XX_CODE treat "OK" non-2xx code like an error.
      // REPLY_3XX_CODE has to be always used along with REPLY_2XX_CODE.
      if ((FLAGSET(Flags, REPLY_2XX_CODE) && (FLastCodeClass != 2)) &&
          ((FLAGCLEAR(Flags, REPLY_3XX_CODE) || (FLastCodeClass != 3))))
      {
        TRACE("3");
        GotReply(TFileZillaIntf::REPLY_ERROR, Flags, Error);
      }
    }
    else if (FLAGSET(Reply, TFileZillaIntf::REPLY_CANCEL) &&
        FLAGSET(Flags, REPLY_ALLOW_CANCEL))
    {
      TRACE("4");
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
      TRACE("5");
      FTerminal->FatalError(NULL, FMTLOAD(INTERNAL_ERROR, L"ftp#2", FORMAT(L"0x%x", static_cast<int>(Reply)).c_str()));
    }
    else
    {
      TRACE("6");
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
      TStrings * MoreMessages = new TStringList();
      try
      {
        TRACE("7");
        if (Disconnected)
        {
          TRACE("8");
          if (FLAGCLEAR(Flags, REPLY_CONNECT))
          {
            TRACE("9");
            MoreMessages->Add(LoadStr(LOST_CONNECTION));
            Discard();
            FTerminal->Closed();
          }
          else
          {
            TRACE("9a");
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
          TRACE("10");
          MoreMessages->Add(LoadStr(USER_TERMINATED));
        }

        if (FLAGSET(Reply, TFileZillaIntf::REPLY_NOTSUPPORTED))
        {
          TRACE("11");
          MoreMessages->Add(LoadStr(FZ_NOTSUPPORTED));
        }

        if (FLastCode == 530)
        {
          TRACE("12");
          MoreMessages->Add(LoadStr(AUTHENTICATION_FAILED));
        }

        if (FLastCode == 425)
        {
          TRACE("12a");
          if (!FTerminal->GetSessionData()->GetFtpPasvMode())
          {
            TRACE("12b");
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
          TRACE("12c");
          HelpKeyword = HELP_ERRORMSG_TIMEOUT;
        }

        TRACE("13");
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
          TRACE("14");
          delete MoreMessages;
          MoreMessages = NULL;
        }
        TRACE("15");
      }
      catch(...)
      {
        TRACE("16");
        delete MoreMessages;
        throw;
      }

      UnicodeString ErrorStr = Error;
      if (Error.IsEmpty() && (MoreMessages != NULL))
      {
        TRACE("17");
        assert(MoreMessages->GetCount() > 0);
        ErrorStr = MoreMessages->Strings[0];
        MoreMessages->Delete(0);
      }

      if (Disconnected)
      {
        TRACE("18");
        // for fatal error, it is essential that there is some message
        assert(!Error.IsEmpty());
        ExtException * E = new ExtException(ErrorStr, MoreMessages, true);
        TRY_FINALLY (
        {
          TRACE("19");
          FTerminal->FatalError(E, L"");
        }
        ,
        {
          delete E;
        }
        );
      }
      else
      {
        TRACE("20");
        throw ExtException(ErrorStr, MoreMessages, true, HelpKeyword);
      }
    }

    if ((Code != NULL) && (FLastCodeClass != DummyCodeClass))
    {
      TRACE("21");
      *Code = FLastCode;
    }

    if (Response != NULL)
    {
      TRACE("22");
      *Response = FLastResponse;
      FLastResponse = new TStringList();
    }
  }
  ,
  {
    TRACE("23");
    ResetReply();
  }
  );
  TRACE("/");
}
//---------------------------------------------------------------------------
void TFTPFileSystem::SetLastCode(int Code)
{
  CALLSTACK;
  FLastCode = Code;
  FLastCodeClass = (Code / 100);
}
//---------------------------------------------------------------------------
void TFTPFileSystem::HandleReplyStatus(const UnicodeString & Response)
{
  CALLSTACK;
  int Code = 0;

  if (FOnCaptureOutput != NULL)
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
    if (Response.Length() >= 5)
    {
      FLastResponse->Add(Response.SubString(5, Response.Length() - 4));
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
      FLastResponse->Add(Response.SubString(Start, Response.Length() - Start + 1));
    }
  }

  if (!FMultineResponse)
  {
    if (FLastCode == 220)
    {
      if (FTerminal->GetConfiguration()->GetShowFtpWelcomeMessage())
      {
        FTerminal->DisplayBanner(FLastResponse->Text);
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
      // Possitive reply to "SYST" must be 215, see RFC 959
      if (FLastCode == 215)
      {
        FSystem = FLastResponse->Text.get().TrimRight();
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
  TRACE("/");
}
//---------------------------------------------------------------------------
UnicodeString TFTPFileSystem::ExtractStatusMessage(UnicodeString Status)
{
  // CApiLog::LogMessage
  // (note that the formatting may not be present when LogMessageRaw is used)
  intptr_t P1 = Status.Pos(L"): ");
  if (P1 > 0)
  {
    intptr_t P2 = Status.Pos(L".cpp(");
    if ((P2 > 0) && (P2 < P1))
    {
      intptr_t P3 = Status.Pos(L"   caller=0x");
      if ((P3 > 0) && (P3 > P1))
      {
        Status = Status.SubString(P1 + 3, P3 - P1 - 3);
      }
    }
  }
  return Status;
}
//---------------------------------------------------------------------------
bool TFTPFileSystem::HandleStatus(const wchar_t * AStatus, int Type)
{
  CALLSTACK;
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
          TRACE("Timeout");
          if (NoFinalLastCode())
          {
            SetLastCode(DummyTimeoutCode);
          }
        }
        else if (Status == FDisconnectStatus)
        {
          TRACE("Disconnect");
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

  TRACE("/");
  return true;
}
//---------------------------------------------------------------------------
TDateTime TFTPFileSystem::ConvertLocalTimestamp(time_t Time)
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
bool TFTPFileSystem::HandleAsynchRequestOverwrite(
  wchar_t * FileName1, size_t FileName1Len, const wchar_t * /*FileName2*/,
  const wchar_t * /*Path1*/, const wchar_t * /*Path2*/,
  __int64 Size1, __int64 Size2, time_t LocalTime,
  bool /*HasLocalTime*/, const TRemoteFileTime & RemoteTime, void * AUserData, int & RequestResult)
{
  if (!FActive)
  {
    return false;
  }
  else
  {
    TFileTransferData & UserData = *(static_cast<TFileTransferData *>(AUserData));
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
            NoFileParams ? NULL : &FileParams))
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

    TRACE("/");
    return true;
  }
}
//---------------------------------------------------------------------------
#ifndef _MSC_VER
struct TClipboardHandler
{
  UnicodeString Text;

  void Copy(TObject * /*Sender*/)
  {
    CopyToClipboard(Text.c_str());
  }
};
#endif
//---------------------------------------------------------------------------
UnicodeString FormatContactList(UnicodeString Entry1, UnicodeString Entry2)
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
      static_cast<unsigned short>(ValidityTime.Year), static_cast<unsigned short>(ValidityTime.Month),
      static_cast<unsigned short>(ValidityTime.Day)) +
    EncodeTimeVerbose(
      static_cast<unsigned short>(ValidityTime.Hour), static_cast<unsigned short>(ValidityTime.Min),
      static_cast<unsigned short>(ValidityTime.Sec), 0));
  */
  unsigned short Y, M, D, H, N, S, MS;
  TDateTime DateTime =
    EncodeDateVerbose(
      static_cast<unsigned short>(ValidityTime.Year), static_cast<unsigned short>(ValidityTime.Month),
      static_cast<unsigned short>(ValidityTime.Day)) +
    EncodeTimeVerbose(
      static_cast<unsigned short>(ValidityTime.Hour), static_cast<unsigned short>(ValidityTime.Min),
      static_cast<unsigned short>(ValidityTime.Sec), 0);
  DateTime.DecodeDate(Y, M, D);
  DateTime.DecodeTime(H, N, S, MS);
  UnicodeString dt = FORMAT(L"%02d.%02d.%04d %02d:%02d:%02d ", D, M, Y, H, N, S);
  return dt;
}
//---------------------------------------------------------------------------
bool TFTPFileSystem::HandleAsynchRequestVerifyCertificate(
  const TFtpsCertificateData & Data, int & RequestResult)
{
  CALLSTACK;
  if (!FActive)
  {
    TRACE("TFTPFileSystem::HandleAsynchRequestVerifyCertificate 1");
    return false;
  }
  else
  {
    FSessionInfo.CertificateFingerprint =
      BytesToHex(RawByteString(reinterpret_cast<const char*>(Data.Hash), Data.HashLen), false, L':');

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

    THierarchicalStorage * Storage =
      FTerminal->GetConfiguration()->CreateScpStorage(false);
    TRY_FINALLY (
    {
      Storage->SetAccessMode(smRead);

      if (Storage->OpenSubKey(CertificateStorageKey, false) &&
          Storage->ValueExists(FSessionInfo.CertificateFingerprint))
      {
        RequestResult = 1;
      }
    }
    ,
    {
      delete Storage;
    }
    );

    if (RequestResult == 0)
    {
      UnicodeString Buf = FTerminal->GetSessionData()->GetHostKey();
      while ((RequestResult == 0) && !Buf.IsEmpty())
      {
        UnicodeString ExpectedKey = CutToChar(Buf, L';', false);
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
      Aliases[0].OnClick = MAKE_CALLBACK(TClipboardHandler::Copy, &ClipboardHandler);

      TQueryParams Params;
      Params.HelpKeyword = HELP_VERIFY_CERTIFICATE;
      Params.NoBatchAnswers = qaYes | qaRetry;
      Params.Aliases = Aliases;
      Params.AliasesCount = LENOF(Aliases);
      uintptr_t Answer = FTerminal->QueryUser(
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
        TRY_FINALLY (
        {
          Storage->SetAccessMode(smReadWrite);

          if (Storage->OpenSubKey(CertificateStorageKey, true))
          {
            Storage->WriteString(FSessionInfo.CertificateFingerprint, L"");
          }
        }
        ,
        {
          delete Storage;
        }
        );
      }
    }

    return true;
  }
}
//---------------------------------------------------------------------------
bool TFTPFileSystem::HandleAsynchRequestNeedPass(
  struct TNeedPassRequestData & Data, int & RequestResult) const
{
  CALLSTACK;
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
      EncodeDateVerbose((unsigned short)Source.Year, (unsigned short)Source.Month,
        (unsigned short)Source.Day);
    if (Source.HasTime)
    {
      DateTime = DateTime +
        EncodeTimeVerbose((unsigned short)Source.Hour, (unsigned short)Source.Minute,
          (unsigned short)Source.Second, 0);
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
      DateTime = ConvertTimestampFromUTC(DateTime);
    }

  }
  else
  {
    // With SCP we estimate date to be today, if we have at least time

    DateTime = double(0);
    ModificationFmt = mfNone;
  }
}
//---------------------------------------------------------------------------
bool TFTPFileSystem::HandleListData(const wchar_t * Path,
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

    for (unsigned int Index = 0; Index < Count; ++Index)
    {
      const TListDataEntry * Entry = &Entries[Index];
      TRemoteFile * File = new TRemoteFile();
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
        catch(...)
        {
          // ignore permissions errors with FTP
        }

        const wchar_t * Space = wcschr(Entry->OwnerGroup, L' ');
        if (Space != NULL)
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
        delete File;
        UnicodeString EntryData =
          FORMAT(L"%s/%s/%s/%s/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d",
             Entry->Name, Entry->Permissions, Entry->OwnerGroup, Int64ToStr(Entry->Size).c_str(),
             int(Entry->Dir), int(Entry->Link), Entry->Time.Year, Entry->Time.Month, Entry->Time.Day,
             Entry->Time.Hour, Entry->Time.Minute, int(Entry->Time.HasTime),
             int(Entry->Time.HasSeconds), int(Entry->Time.HasDate));
        throw ETerminal(&E, FMTLOAD(LIST_LINE_ERROR, EntryData.c_str()));
      }

      FFileList->AddFile(File);
    }
    return true;
  }
}
//---------------------------------------------------------------------------
bool TFTPFileSystem::HandleTransferStatus(bool Valid, __int64 TransferSize,
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
  TRACE("/");
  return true;
}
//---------------------------------------------------------------------------
bool TFTPFileSystem::HandleReply(int Command, unsigned int Reply)
{
  CALLSTACK;
  if (!FActive)
  {
    TRACE("1");
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
    TRACE("/");
    return true;
  }
}
//---------------------------------------------------------------------------
bool TFTPFileSystem::HandleCapabilities(
  TFTPServerCapabilities * ServerCapabilities)
{
  CALLSTACK;
  FServerCapabilities->Assign(ServerCapabilities);
  FFileSystemInfoValid = false;
  return true;
}
//---------------------------------------------------------------------------
bool TFTPFileSystem::CheckError(int ReturnCode, const wchar_t * Context)
{
  CALLSTACK;
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
    TRACE("2");
    if (!FWaitingForReply)
    {
      TRACE("3");
      // throws
      WaitForFatalNonCommandReply();
    }
  }
  else
  {
    TRACE("4");
    FTerminal->FatalError(NULL,
      FMTLOAD(INTERNAL_ERROR, FORMAT(L"fz#%s", Context).c_str(), IntToHex(ReturnCode, 4).c_str()));
    assert(false);
  }

  TRACE("/");
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

  int Index = 1;
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
  TFileTransferData * Data = static_cast<TFileTransferData *>(UserData);
  FILETIME WrTime = DateTimeToFileTime(Data->Modification, dstmUnix);
  SetFileTime(Handle, NULL, NULL, &WrTime);
}
//---------------------------------------------------------------------------
bool TFTPFileSystem::GetFileModificationTimeInUtc(const wchar_t * FileName, struct tm & Time)
{
  bool Result;
  try
  {
    // error-handling-free and DST-mode-inaware copy of TTerminal::OpenLocalFile
    HANDLE Handle = CreateFile(FileName, GENERIC_READ,
      FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
    if (Handle == INVALID_HANDLE_VALUE)
    {
      Result = false;
    }
    else
    {
      FILETIME MTime;
      if (!GetFileTime(Handle, NULL, NULL, &MTime))
      {
        Result = false;
      }
      else
      {
        TDateTime Modification = ConvertTimestampToUTC(FileTimeToDateTime(MTime));

        unsigned short Year;
        unsigned short Month;
        unsigned short Day;
        Modification.DecodeDate(Year, Month, Day);
        Time.tm_year = Year - 1900;
        Time.tm_mon = Month - 1;
        Time.tm_mday = Day;

        unsigned short Hour;
        unsigned short Min;
        unsigned short Sec;
        unsigned short MSec;
        Modification.DecodeTime(Hour, Min, Sec, MSec);
        Time.tm_hour = Hour;
        Time.tm_min = Min;
        Time.tm_sec = Sec;

        Result = true;
      }

      CloseHandle(Handle);
    }
  }
  catch (...)
  {
    Result = false;
  }
  return Result;
}
//---------------------------------------------------------------------------
#endif NO_FILEZILLA
