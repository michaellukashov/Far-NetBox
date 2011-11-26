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
static const std::wstring CONST_PROTOCOL_BASE_NAME = L"WebDAV - HTTP";
//---------------------------------------------------------------------------
std::wstring UnixExcludeLeadingBackslash(const std::wstring str)
{
    std::wstring path = str;
    while (!path.empty() && path[0] == L'/')
    {
        path.erase(0, 1);
    }
    return path;
}

//---------------------------------------------------------------------------
#define FILE_OPERATION_LOOP_EX(ALLOW_SKIP, MESSAGE, OPERATION) \
  FILE_OPERATION_LOOP_CUSTOM(Self->FTerminal, ALLOW_SKIP, MESSAGE, OPERATION)
//---------------------------------------------------------------------------
const int ecRaiseExcept = 1;
const int ecIgnoreWarnings = 2;
const int ecReadProgress = 4;
const int ecDefault = ecRaiseExcept;
//---------------------------------------------------------------------------
const int tfFirstLevel = 0x01;
const int tfAutoResume = 0x02;
//===========================================================================

class TSessionData;
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

//---------------------------------------------------------------------------
struct TSinkFileParams
{
  std::wstring TargetDir;
  const TCopyParamType * CopyParam;
  int Params;
  TFileOperationProgressType * OperationProgress;
  bool Skipped;
  unsigned int Flags;
};
//---------------------------------------------------------------------------
class TFileListHelper
{
public:
  TFileListHelper(THTTPFileSystem * FileSystem, TRemoteFileList * FileList,
      bool IgnoreFileList) :
    FFileSystem(FileSystem),
    FFileList(FFileSystem->FFileList),
    FIgnoreFileList(FFileSystem->FIgnoreFileList)
  {
    FFileSystem->FFileList = FileList;
    FFileSystem->FIgnoreFileList = IgnoreFileList;
  }

  ~TFileListHelper()
  {
    FFileSystem->FFileList = FFileList;
    FFileSystem->FIgnoreFileList = FIgnoreFileList;
  }

private:
  THTTPFileSystem * FFileSystem;
  TRemoteFileList * FFileList;
  bool FIgnoreFileList;
};
//===========================================================================
THTTPFileSystem::THTTPFileSystem(TTerminal *ATerminal) :
  TCustomFileSystem(ATerminal),
  // FSecureShell(NULL),
  FFileList(NULL),
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
  FLastReadDirectoryProgress(0),
  FLastResponse(new TStringList()),
  FLastError(new TStringList()),
  FTransferStatusCriticalSection(new TCriticalSection()),
  FAbortEvent(CreateEvent(NULL, true, false, NULL)),
  m_ProgressPercent(0),
  FListAll(asAuto),
  FDoListAll(false)
{
  Self = this;
}

void THTTPFileSystem::Init(TSecureShell *SecureShell)
{
  // FSecureShell = SecureShell;
  FLsFullTime = FTerminal->GetSessionData()->GetSCPLsFullTime();
  FProcessingCommand = false;

  FFileSystemInfo.ProtocolBaseName = CONST_PROTOCOL_BASE_NAME;
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
  delete FLastResponse;
  FLastResponse = NULL;
  delete FLastError;
  FLastError = NULL;
  delete FTransferStatusCriticalSection;
  FTransferStatusCriticalSection = NULL;
  // delete FSecureShell;
  delete FCURLIntf;
  FCURLIntf = NULL;
  CloseHandle(FAbortEvent);
}
//---------------------------------------------------------------------------
void THTTPFileSystem::Open()
{
  DEBUG_PRINTF(L"begin");
  // FSecureShell->Open();

  FCurrentDirectory = L"";
  FHomeDirectory = L"";

  TSessionData *Data = FTerminal->GetSessionData();

  FSessionInfo.LoginTime = Now();
  FSessionInfo.ProtocolBaseName = CONST_PROTOCOL_BASE_NAME;
  FSessionInfo.ProtocolName = FSessionInfo.ProtocolBaseName;

  FLastDataSent = Now();

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
    FCurrentDirectory = ::UnixExcludeTrailingBackslash(path);
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
  ::Error(SNotImplemented, 1009);
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
  return FUserName;
}
//---------------------------------------------------------------------------
void THTTPFileSystem::Idle()
{
  // Keep session alive
  return;
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
      return false;

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
  return FCurrentDirectory;
}

//---------------------------------------------------------------------------
void THTTPFileSystem::EnsureLocation()
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
    /*
    if (!UnixComparePaths(ActualCurrentDirectory(), FCurrentDirectory))
    {
      FTerminal->LogEvent(FORMAT(L"Synchronizing current directory \"%s\".",
        FCurrentDirectory.c_str()));
      DoChangeDirectory(FCurrentDirectory);
    }
    */
  }
}

void THTTPFileSystem::Discard()
{
  // remove all pending messages, to get complete log
  // note that we need to retry discard on reconnect, as there still may be another
  // "disconnect/timeout/..." status messages coming
  assert(FActive);
  FActive = false;
}
//---------------------------------------------------------------------------

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
}
//---------------------------------------------------------------------------
void THTTPFileSystem::ReadCurrentDirectory()
{
  DEBUG_PRINTF(L"begin, FCurrentDirectory = %s", FCurrentDirectory.c_str());
  std::wstring Path = FCurrentDirectory.empty() ? L"/" : FCurrentDirectory;
  std::wstring response;
  std::wstring errorInfo;
  bool isExist = SendPropFindRequest(Path.c_str(), response, errorInfo);
  // DEBUG_PRINTF(L"responce = %s, errorInfo = %s", response.c_str(), errorInfo.c_str());
  // TODO: cache response
  if (isExist)
  {
      FCurrentDirectory = Path;
  }
  DEBUG_PRINTF(L"end, FCurrentDirectory = %s", FCurrentDirectory.c_str());
}
//---------------------------------------------------------------------------
void THTTPFileSystem::HomeDirectory()
{
  // ExecCommand(fsHomeDirectory);
}
//---------------------------------------------------------------------------
void THTTPFileSystem::AnnounceFileListOperation()
{
  // noop
}
//---------------------------------------------------------------------------
void THTTPFileSystem::DoChangeDirectory(const std::wstring & Directory)
{
}
//---------------------------------------------------------------------------
void THTTPFileSystem::ChangeDirectory(const std::wstring ADirectory)
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

  // DoChangeDirectory(Directory);
  FCurrentDirectory = AbsolutePath(Directory, false);

  // make next ReadCurrentDirectory retrieve actual server-side current directory
  // FCurrentDirectory = L"";
}
//---------------------------------------------------------------------------
void THTTPFileSystem::CachedChangeDirectory(const std::wstring Directory)
{
  // FCachedDirectoryChange = UnixExcludeTrailingBackslash(Directory);
  FCurrentDirectory = UnixExcludeTrailingBackslash(Directory);
  if (FCurrentDirectory.empty())
  {
      FCurrentDirectory = L"/";
  }
}

void THTTPFileSystem::DoReadDirectory(TRemoteFileList * FileList)
{
    FileList->Clear();
    // add parent directory
    FileList->AddFile(new TRemoteParentDirectory(FTerminal));

    FLastReadDirectoryProgress = 0;

    TFileListHelper Helper(this, FileList, false);

    // always specify path to list, do not attempt to 
    // list "current" dir as:
    // 1) List() lists again the last listed directory, not the current working directory
    // 2) we handle this way the cached directory change
    std::wstring Directory = AbsolutePath(FileList->GetDirectory(), false);
    // DEBUG_PRINTF(L"Directory = %s", Directory.c_str());
    GetList(Directory);

    FLastDataSent = Now();
}
//---------------------------------------------------------------------------
void THTTPFileSystem::ReadDirectory(TRemoteFileList * FileList)
{
  DEBUG_PRINTF(L"begin");
  assert(FileList);
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
    catch (...)
    {
      FDoListAll = false;
      // reading the first directory has failed,
      // further try without "-a" only as the server may not support it
      if ((FListAll == asAuto) && FTerminal->GetActive())
      {
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
  ::Error(SNotImplemented, 1007);
  File = NULL;
  int Params = ecDefault |
    FLAGMASK(FTerminal->GetSessionData()->GetIgnoreLsWarnings(), ecIgnoreWarnings);
  // the auto-detection of --full-time support is not implemented for fsListFile,
  // so we use it only if we already know that it is supported (asOn).
  // const wchar_t * Options = (FLsFullTime == asOn) ? FullTimeOption : L"";
  // ExecCommand(fsListFile,
    // Params, FTerminal->GetSessionData()->GetListingCommand().c_str(), Options, DelimitStr(FileName).c_str());
}
//---------------------------------------------------------------------------
void THTTPFileSystem::DeleteFile(const std::wstring FileName,
  const TRemoteFile * File, int Params, TRmSessionAction & Action)
{
  USEDPARAM(File);
  USEDPARAM(Params);
  Action.Recursive();
  assert(FLAGCLEAR(Params, dfNoRecursive) || (File && File->GetIsSymLink()));
  // ExecCommand(fsDeleteFile, 0, DelimitStr(FileName).c_str());
}
//---------------------------------------------------------------------------
void THTTPFileSystem::RenameFile(const std::wstring FileName,
  const std::wstring NewName)
{
  // ExecCommand(fsRenameFile, 0, DelimitStr(FileName).c_str(), DelimitStr(NewName).c_str());
}
//---------------------------------------------------------------------------
void THTTPFileSystem::CopyFile(const std::wstring FileName,
  const std::wstring NewName)
{
  // ExecCommand(fsCopyFile, 0, DelimitStr(FileName).c_str(), DelimitStr(NewName).c_str());
}
//---------------------------------------------------------------------------
void THTTPFileSystem::CreateDirectory(const std::wstring DirName)
{
  // ExecCommand(fsCreateDirectory, 0, DelimitStr(DirName).c_str());
}
//---------------------------------------------------------------------------
void THTTPFileSystem::CreateLink(const std::wstring FileName,
  const std::wstring PointTo, bool Symbolic)
{
  // ExecCommand(fsCreateLink, 0,
    // Symbolic ? L"-s" : L"", DelimitStr(PointTo).c_str(), DelimitStr(FileName).c_str());
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
    // ExecCommand(Cmd, 0, RecursiveStr.c_str(), Str.c_str(), DelimitedName.c_str());
  }
}
//---------------------------------------------------------------------------
void THTTPFileSystem::ChangeFileProperties(const std::wstring FileName,
  const TRemoteFile * File, const TRemoteProperties * Properties,
  TChmodSessionAction & Action)
{
  ::Error(SNotImplemented, 1006);
  assert(Properties);
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

    // AnyCommand(Cmd, &OutputEvent);
  }
}
//---------------------------------------------------------------------------
void THTTPFileSystem::CaptureOutput(const std::wstring & AddedLine, bool StdError)
{
  int ReturnCode;
  std::wstring Line = AddedLine;
  // DEBUG_PRINTF(L"Line = %s", Line.c_str());
  if (StdError ||
      !Line.empty())
  {
    assert(!FOnCaptureOutput.empty());
    FOnCaptureOutput(Line, StdError);
  }
}
//---------------------------------------------------------------------------
void THTTPFileSystem::AnyCommand(const std::wstring Command,
  const captureoutput_slot_type *OutputEvent)
{
    ::Error(SNotImplemented, 1008);
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
void THTTPFileSystem::CopyToRemote(TStrings * FilesToCopy,
  const std::wstring TargetDir, const TCopyParamType * CopyParam,
  int Params, TFileOperationProgressType * OperationProgress,
  TOnceDoneOperation & OnceDoneOperation)
{
  ::Error(SNotImplemented, 1005);
}

//---------------------------------------------------------------------------
void THTTPFileSystem::CopyToLocal(TStrings * FilesToCopy,
  const std::wstring TargetDir, const TCopyParamType * CopyParam,
  int Params, TFileOperationProgressType * OperationProgress,
  TOnceDoneOperation & OnceDoneOperation)
{
    Params &= ~cpAppend;
    std::wstring FullTargetDir = IncludeTrailingBackslash(TargetDir);

    int Index = 0;
    while (Index < FilesToCopy->GetCount() && !OperationProgress->Cancel)
    {
        std::wstring FileName = FilesToCopy->GetString(Index);
        const TRemoteFile * File = dynamic_cast<const TRemoteFile *>(FilesToCopy->GetObject(Index));
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
                FLastDataSent = Now();
            }
            catch (const EScpSkipFile &E)
            {
                DEBUG_PRINTF(L"before FTerminal->HandleException");
                SUSPEND_OPERATION (
                    if (!FTerminal->HandleException(&E)) throw;
                );
            }
        }
        Index++;
    }
}
//---------------------------------------------------------------------------
void THTTPFileSystem::SinkRobust(const std::wstring FileName,
    const TRemoteFile * File, const std::wstring TargetDir,
    const TCopyParamType * CopyParam, int Params,
    TFileOperationProgressType * OperationProgress, unsigned int Flags)
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
        catch (const std::exception & E)
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
void THTTPFileSystem::Sink(const std::wstring FileName,
    const TRemoteFile * File, const std::wstring TargetDir,
    const TCopyParamType * CopyParam, int Params,
    TFileOperationProgressType * OperationProgress, unsigned int Flags,
    TDownloadSessionAction & Action)
{
    std::wstring OnlyFileName = UnixExtractFileName(FileName);

    Action.FileName(FileName);

    TFileMasks::TParams MaskParams;
    MaskParams.Size = File->GetSize();

    if (!CopyParam->AllowTransfer(FileName, osRemote, File->GetIsDirectory(), MaskParams))
    {
        FTerminal->LogEvent(FORMAT(L"File \"%s\" excluded from transfer", FileName.c_str()));
        THROW_SKIP_FILE_NULL;
    }

    assert(File);
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

            FTerminal->ProcessDirectory(FileName, boost::bind(&THTTPFileSystem::SinkFile, this, _1, _2, _3), &SinkFileParams);

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

        int Attrs;
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
            TFileListHelper Helper(this, NULL, true);

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
void THTTPFileSystem::SinkFile(std::wstring FileName,
    const TRemoteFile * File, void * Param)
{
    TSinkFileParams * Params = (TSinkFileParams *)Param;
    assert(Params->OperationProgress);
    try
    {
        SinkRobust(FileName, File, Params->TargetDir, Params->CopyParam,
            Params->Params, Params->OperationProgress, Params->Flags);
    }
    catch (const EScpSkipFile &E)
    {
        TFileOperationProgressType * OperationProgress = Params->OperationProgress;

        Params->Skipped = true;
        DEBUG_PRINTF(L"before FTerminal->HandleException");
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
    std::wstring errorInfo;
    DEBUG_PRINTF(L"RemoteFile = %s, FileName = %s", RemoteFile.c_str(), FileName.c_str());
    bool res = GetFile(FileName.c_str(), LocalFile.c_str(), Size, errorInfo);
    if (!res)
    {
      FFileTransferAbort = ftaSkip;
      // FFileTransferCancelled = false;
    }
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
    FCurrentDirectory = ::ExcludeTrailingBackslash(path);
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

bool THTTPFileSystem::GetList(const std::wstring &Directory)
{
    std::vector<TListDataEntry> Entries;

    std::wstring response;
    std::wstring errorInfo;
    if (!SendPropFindRequest(FCurrentDirectory.c_str(), response, errorInfo))
    {
        return false;
    }

    // Erase slashes (to compare in xml parse)
    std::wstring currentPath = ::UnixExcludeLeadingBackslash(::ExcludeTrailingBackslash(Directory));

    const std::string decodedResp = DecodeHex(::W2MB(response.c_str()));

#ifdef _DEBUG
    // CNBFile::SaveFile(L"c:\\webdav_response_raw.xml", ::W2MB(response.c_str()).c_str());
    // CNBFile::SaveFile(L"c:\\webdav_response_decoded.xml", decodedResp.c_str());
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
        path = ::UnixExcludeLeadingBackslash(::ExcludeTrailingBackslash(path));
        // DEBUG_PRINTF(L"href = %s, path = %s, currentPath = %s", href.c_str(), path.c_str(), currentPath.c_str());

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

    unsigned int Count = static_cast<int>(wdavItems.size());
    if (Count)
    {
        Entries.resize(Count);
        for (int i = 0; i < Count; ++i)
        {
            TListDataEntry &Dest = Entries[i];
            WebDAVItem &item = wdavItems[i];
            Dest.Name = wdavItems[i].Name.c_str();
            // DEBUG_PRINTF(L"Dest.Name = %s", Dest.Name);
            Dest.Permissions = L"";
            Dest.OwnerGroup = L"";
            int dir = item.Attributes & FILE_ATTRIBUTE_DIRECTORY;
            Dest.Size = dir == 0 ? item.Size : 0;
            Dest.Dir = dir != 0;
            Dest.Link = false;
            FILETIME ft = item.Created;
            SYSTEMTIME st;
            ::FileTimeToSystemTime(&ft, &st);
            TDateTime dt = ::SystemTimeToDateTime(st);
            unsigned int Y, M, D;
            unsigned int HH, MM, SS, MS;
            dt.DecodeDate(Y, M, D);
            dt.DecodeTime(HH, MM, SS, MS);
            Dest.Year = Y;
            Dest.Month = M;
            Dest.Day = D;
            Dest.Hour = HH;
            Dest.Minute = MM;
            Dest.HasTime = true;
            Dest.HasDate = true;
            Dest.LinkTarget = L"";
        }
    }
    // DEBUG_PRINTF(L"Count = %d", Count);
    TListDataEntry *pEntries = Entries.size() > 0 ? &Entries[0] : NULL;
    HandleListData(Directory.c_str(), pEntries, Entries.size());
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
    // slist.Append("Content-Type: application/octet-stream");
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
