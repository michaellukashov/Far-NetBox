//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#ifndef NE_LFS
#define NE_LFS
#endif
#ifndef WINSCP
#define WINSCP
#endif
#ifndef NEED_LIBS3
#define NEED_LIBS3
#endif

#include <StrUtils.hpp>
#include <NeonIntf.h>
#include <SessionData.h>
#include <Interface.h>
#include <Common.h>
#include <Exceptions.h>
#include <Terminal.h>

#include "S3FileSystem.h"

#include "TextsCore.h"
#include "HelpCore.h"
#include <ne_request.h>
//---------------------------------------------------------------------------
__removed #pragma package(smart_init)
//---------------------------------------------------------------------------
#define StrFromS3(S) StrFromNeon(S)
#define StrToS3(S) StrToNeon(S)
//---------------------------------------------------------------------------
#define FILE_OPERATION_LOOP_TERMINAL FTerminal
//---------------------------------------------------------------------------
static std::unique_ptr<TCriticalSection> LibS3Section(TraceInitPtr(new TCriticalSection()));
//---------------------------------------------------------------------------
UTF8String LibS3Delimiter(L"/");
//---------------------------------------------------------------------------
UnicodeString S3LibVersion()
{
  return FORMAT("%s.%s", LIBS3_VER_MAJOR, LIBS3_VER_MINOR);
}
//---------------------------------------------------------------------------
UnicodeString S3LibDefaultHostName()
{
  return UnicodeString(S3_DEFAULT_HOSTNAME);
}
//---------------------------------------------------------------------------
UnicodeString S3LibDefaultRegion()
{
  return StrFromS3(S3_DEFAULT_REGION);
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
const int TS3FileSystem::S3MultiPartChunkSize = 5 * 1024 * 1024;
//---------------------------------------------------------------------------
TS3FileSystem::TS3FileSystem(TTerminal * ATerminal) :
  TCustomFileSystem(OBJECT_CLASS_TS3FileSystem, ATerminal),
  FActive(false),
  FResponseIgnore(false)
{
  FFileSystemInfo.ProtocolBaseName = L"S3";
  FFileSystemInfo.ProtocolName = FFileSystemInfo.ProtocolBaseName;
  S3_create_request_context(&FRequestContext);
  S3_set_request_context_session_callback(FRequestContext, LibS3SessionCallback, this);
  S3_set_request_context_ssl_callback(FRequestContext, LibS3SslCallback, this);
  S3_set_request_context_response_data_callback(FRequestContext, LibS3ResponseDataCallback, this);
}
//---------------------------------------------------------------------------
TS3FileSystem::~TS3FileSystem()
{
  S3_destroy_request_context(FRequestContext);
  FRequestContext = nullptr;
  UnregisterFromNeonDebug(FTerminal);
}

void TS3FileSystem::Init(void *)
{

}

void TS3FileSystem::FileTransferProgress(int64_t /*TransferSize*/, int64_t /*Bytes*/)
{

}
//---------------------------------------------------------------------------
void TS3FileSystem::Open()
{

  FTlsVersionStr = L"";
  FNeonSession = nullptr;
  FCurrentDirectory = L"";

  RequireNeon(FTerminal);

  FTerminal->Information(LoadStr(STATUS_CONNECT), true);

  TSessionData * Data = FTerminal->GetSessionData();

  FSessionInfo.LoginTime = Now();

  FLibS3Protocol = (Data->GetFtps() != ftpsNone) ? S3ProtocolHTTPS : S3ProtocolHTTP;

  UnicodeString AccessKeyId = Data->GetUserNameExpanded();
  if (AccessKeyId.IsEmpty())
  {
    if (!FTerminal->PromptUser(Data, pkUserName, LoadStr(S3_ACCESS_KEY_ID_TITLE), L"",
          LoadStr(S3_ACCESS_KEY_ID_PROMPT), true, 0, AccessKeyId))
    {
      // note that we never get here actually
      throw Exception(L"");
    }
  }
  FAccessKeyId = UTF8String(AccessKeyId);

  UnicodeString SecretAccessKey = UTF8String(Data->GetPassword());
  if (SecretAccessKey.IsEmpty())
  {
    if (!FTerminal->PromptUser(Data, pkPassword, LoadStr(S3_SECRET_ACCESS_KEY_TITLE), L"",
          LoadStr(S3_SECRET_ACCESS_KEY_PROMPT), false, 0, SecretAccessKey))
    {
      // note that we never get here actually
      throw Exception(L"");
    }
  }
  FSecretAccessKey = UTF8String(SecretAccessKey);

  FHostName = UTF8String(Data->GetHostNameExpanded());
  FTimeout = ToInt(Data->GetTimeout());

  RegisterForNeonDebug(FTerminal);
  UpdateNeonDebugMask();

  {
    volatile TGuard Guard(*LibS3Section.get());
    S3_initialize(nullptr, S3_INIT_ALL, nullptr);
  }

  FActive = false;
  try
  {
    UnicodeString Path;
    if (base::IsUnixRootPath(Data->GetRemoteDirectory()))
    {
      Path = ROOTDIRECTORY;
    }
    else
    {
      UnicodeString BucketName;
      UnicodeString UnusedKey;
      ParsePath(Data->GetRemoteDirectory(), BucketName, UnusedKey);
      Path = UnicodeString(LibS3Delimiter) + BucketName;
    }
    TryOpenDirectory(Path);
  }
  catch (Exception &E)
  {
    LibS3Deinitialize();
    FTerminal->Closed();
    FTerminal->FatalError(&E, LoadStr(CONNECTION_FAILED));
  }
  FActive = true;
}
//---------------------------------------------------------------------------
struct TLibS3CallbackData
{
CUSTOM_MEM_ALLOCATION_IMPL

  TLibS3CallbackData()
  {
    Status = (S3Status)-1;
    FileSystem = nullptr;
  }

  TS3FileSystem * FileSystem;
  S3Status Status;
  UnicodeString RegionDetail;
  UnicodeString EndpointDetail;
  UnicodeString ErrorMessage;
  UnicodeString ErrorDetails;
};
//---------------------------------------------------------------------------
TS3FileSystem * TS3FileSystem::GetFileSystem(void * CallbackData)
{
  return static_cast<TLibS3CallbackData *>(CallbackData)->FileSystem;
}
//---------------------------------------------------------------------------
void TS3FileSystem::LibS3SessionCallback(ne_session_s * Session, void * CallbackData)
{
  TS3FileSystem * FileSystem = static_cast<TS3FileSystem *>(CallbackData);
  TSessionData * Data = FileSystem->FTerminal->GetSessionData();

  InitNeonSession(
    Session, Data->GetProxyMethod(), Data->GetProxyHost(), Data->GetProxyPort(),
    Data->GetProxyUsername(), Data->GetProxyPassword(), FileSystem->FTerminal);

  ne_set_session_private(Session, SESSION_FS_KEY, FileSystem);

  SetNeonTlsInit(Session, FileSystem->InitSslSession);

  // Data->Timeout is propagated via timeoutMs parameter of functions like S3_list_service

  FileSystem->FNeonSession = Session;
}
//------------------------------------------------------------------------------
void TS3FileSystem::InitSslSession(ssl_st *Ssl, ne_session *Session)
{
  TS3FileSystem *FileSystem =
    static_cast<TS3FileSystem *>(ne_get_session_private(Session, SESSION_FS_KEY));
  FileSystem->InitSslSessionImpl(Ssl);
}

void TS3FileSystem::InitSslSessionImpl(ssl_st *Ssl) const
{
  // See also CAsyncSslSocketLayer::InitSSLConnection
  SetupSsl(Ssl, FTerminal->GetSessionData()->GetMinTlsVersion(), FTerminal->GetSessionData()->GetMaxTlsVersion());
}
//---------------------------------------------------------------------------
int TS3FileSystem::LibS3SslCallback(int Failures, const ne_ssl_certificate_s * Certificate, void * CallbackData)
{
  TNeonCertificateData Data;
  RetrieveNeonCertificateData(Failures, Certificate, Data);
  TS3FileSystem * FileSystem = static_cast<TS3FileSystem *>(CallbackData);
  return FileSystem->VerifyCertificate(Data) ? NE_OK : NE_ERROR;
}
//---------------------------------------------------------------------------
// Similar to TWebDAVFileSystem::VerifyCertificate
bool TS3FileSystem::VerifyCertificate(TNeonCertificateData &Data)
{
  FSessionInfo.CertificateFingerprint = Data.Fingerprint;

  bool Result;
  if (FTerminal->GetSessionData()->GetFingerprintScan())
  {
    Result = false;
  }
  else
  {
    FTerminal->LogEvent(CertificateVerificationMessage(Data));

    UnicodeString SiteKey = TSessionData::FormatSiteKey(FTerminal->GetSessionData()->GetHostNameExpanded(), FTerminal->GetSessionData()->GetPortNumber());
    Result =
      FTerminal->VerifyCertificate(HttpsCertificateStorageKey, SiteKey, Data.Fingerprint, Data.Subject, Data.Failures);

    if (!Result)
    {
      UnicodeString Message;
      Result = NeonWindowsValidateCertificateWithMessage(Data, Message);
      FTerminal->LogEvent(Message);
    }

    FSessionInfo.Certificate = CertificateSummary(Data, FTerminal->GetSessionData()->GetHostNameExpanded());

    if (!Result)
    {
      Result = FTerminal->ConfirmCertificate(FSessionInfo, Data.Failures, HttpsCertificateStorageKey, true);
    }

    if (Result)
    {
      CollectTLSSessionInfo();
    }
  }

  return Result;
}
//------------------------------------------------------------------------------
void TS3FileSystem::CollectTLSSessionInfo()
{
  // See also TFTPFileSystem::Open().
  // Have to cache the value as the connection (the neon HTTP session, not "our" session)
  // can be closed at the time we need it in CollectUsage().
  UnicodeString Message = NeonTlsSessionInfo(FNeonSession, FSessionInfo, FTlsVersionStr);
  FTerminal->LogEvent(Message);
}
//---------------------------------------------------------------------------
S3Status TS3FileSystem::LibS3ResponsePropertiesCallback(const S3ResponseProperties * /*Properties*/, void * /*CallbackData*/)
{

  // TODO
  return S3StatusOK;
}
//---------------------------------------------------------------------------
void TS3FileSystem::LibS3ResponseDataCallback(const char * Data, size_t Size, void * CallbackData)
{
  TS3FileSystem * FileSystem = static_cast<TS3FileSystem *>(CallbackData);
  if (FileSystem->FTerminal->GetLog()->GetLogging() && !FileSystem->FResponseIgnore)
  {
    UnicodeString Content = UnicodeString(UTF8String(Data, Size)).Trim();
    FileSystem->FResponse += Content;
  }
}
//---------------------------------------------------------------------------
void TS3FileSystem::LibS3ResponseCompleteCallback(S3Status Status, const S3ErrorDetails * Error, void * CallbackData)
{
  TLibS3CallbackData & Data = *static_cast<TLibS3CallbackData *>(CallbackData);

  TS3FileSystem * FileSystem = Data.FileSystem;
  Data.Status = Status;
  Data.RegionDetail = L"";
  Data.EndpointDetail = L"";
  Data.ErrorMessage = L"";
  Data.ErrorDetails = L"";

  if (Error != nullptr)
  {
    if (Error->message != nullptr)
    {
      Data.ErrorMessage = StrFromS3(Error->message);
      FileSystem->FTerminal->LogEvent(Data.ErrorMessage);
    }

    UnicodeString ErrorDetails;
    if (Error->resource != nullptr)
    {
      AddToList(ErrorDetails, FMTLOAD(S3_ERROR_RESOURCE, (StrFromS3(Error->resource))), L"\n");
    }
    if (Error->furtherDetails != nullptr)
    {
      AddToList(ErrorDetails, FMTLOAD(S3_ERROR_FURTHER_DETAILS, (StrFromS3(Error->furtherDetails))), L"\n");
    }
    if (Error->extraDetailsCount)
    {
      UnicodeString ExtraDetails;
      for (int I = 0; I < Error->extraDetailsCount; I++)
      {
        UnicodeString DetailName = StrFromS3(Error->extraDetails[I].name);
        UnicodeString DetailValue = StrFromS3(Error->extraDetails[I].value);
        if (SameText(DetailName, L"Region"))
        {
          Data.RegionDetail = DetailValue;
        }
        else if (SameText(DetailName, L"Endpoint"))
        {
          Data.EndpointDetail = DetailValue;
        }
        AddToList(ExtraDetails, FORMAT("%s: %s", DetailName, DetailValue), L", ");
      }
      AddToList(ErrorDetails, LoadStr(S3_ERROR_EXTRA_DETAILS) + ExtraDetails, L"\n");
    }

    if (!ErrorDetails.IsEmpty())
    {
      FileSystem->FTerminal->LogEvent(ErrorDetails);
      Data.ErrorDetails = ErrorDetails;
    }
  }

  if (!FileSystem->FResponse.IsEmpty())
  {
    FileSystem->FTerminal->GetLog()->Add(llOutput, FileSystem->FResponse);
  }
}
//---------------------------------------------------------------------------
void TS3FileSystem::RequestInit(TLibS3CallbackData & Data)
{
  Data.FileSystem = this;
  FResponse = L"";
}
//---------------------------------------------------------------------------
void TS3FileSystem::CheckLibS3Error(const TLibS3CallbackData & Data, bool FatalOnConnectError)
{
  if (Data.Status != S3StatusOK)
  {
    UnicodeString Error, Details;
    bool FatalCandidate = false;
    switch (Data.Status)
    {
      case S3StatusAbortedByCallback:
        Error = LoadStr(USER_TERMINATED);
        break;

      case S3StatusErrorAccessDenied:
        Error = LoadStr(S3_STATUS_ACCESS_DENIED);
        break;

      case S3StatusErrorSignatureDoesNotMatch: // While it can mean an implementation fault, it will typically mean a wrong secure key.
      case S3StatusErrorInvalidAccessKeyId:
        Error = LoadStr(AUTHENTICATION_FAILED);
        break;

      case S3StatusNameLookupError:
        Error = ReplaceStr(LoadStr(NET_TRANSL_HOST_NOT_EXIST2), L"%HOST%", FTerminal->GetSessionData()->GetHostNameExpanded());
        FatalCandidate = true;
        break;

      case S3StatusFailedToConnect:
        Error = LoadStr(CONNECTION_FAILED);
        FatalCandidate = true;
        break;

      case S3StatusConnectionFailed:
        FatalCandidate = true;
        break;
    }

    if (!Error.IsEmpty())
    {
      Details = Data.ErrorMessage;
      AddToList(Details, Data.ErrorDetails, L"\n");
    }
    else
    {
      if (!Data.ErrorMessage.IsEmpty())
      {
        Error = Data.ErrorMessage;
      }
      else
      {
        // only returns name of the S3 status code symbol, like S3StatusErrorAccountProblem,
        // not something we should really display to an user, but still better than an internal error code
        Error = S3_get_status_name(Data.Status);
      }
      Details = Data.ErrorDetails;
    }

    Error = MainInstructions(Error);

    if (FatalCandidate && FatalOnConnectError)
    {
      throw EFatal(nullptr, Error, Details);
    }
    else
    {
      throw ExtException(Error, Details);
    }
  }
}
//---------------------------------------------------------------------------
void TS3FileSystem::LibS3Deinitialize()
{
  volatile TGuard Guard(*LibS3Section.get());
  S3_deinitialize();
}
//---------------------------------------------------------------------------
UnicodeString TS3FileSystem::GetFolderKey(const UnicodeString AKey)
{
  return AKey + L"/";
}
//---------------------------------------------------------------------------
void TS3FileSystem::ParsePath(const UnicodeString APath, UnicodeString &BucketName, UnicodeString &AKey)
{
  UnicodeString Path = APath;
  if (DebugAlwaysTrue(Path.SubString(1, 1) == L"/"))
  {
    Path.Delete(1, 1);
  }
  intptr_t P = Path.Pos(L"/");
  if (P == 0)
  {
    BucketName = Path;
    AKey = L"";
  }
  else
  {
    BucketName = Path.SubString(0, P - 1);
    AKey = Path.SubString(P + 1, Path.Length() - P);
  }
}
//---------------------------------------------------------------------------
struct TLibS3BucketContext : S3BucketContext
{
CUSTOM_MEM_ALLOCATION_IMPL
  // These keep data that that we point the native S3BucketContext fields to
  UTF8String HostNameBuf;
  UTF8String BucketNameBuf;
  UTF8String AuthRegionBuf;
};
//---------------------------------------------------------------------------
struct TLibS3ListBucketCallbackData : TLibS3CallbackData
{
  TRemoteFileList * FileList;
  int KeyCount;
  UTF8String NextMarker;
  bool IsTruncated;
};
//---------------------------------------------------------------------------
TLibS3BucketContext TS3FileSystem::GetBucketContext(const UnicodeString ABucketName)
{
  TLibS3BucketContext Result;

  bool First = true;
  bool Retry = false;
  do
  {
    TRegions::const_iterator I = FRegions.find(ABucketName);
    UnicodeString Region;
    if (I != FRegions.end())
    {
      Region = I->second;
    }
    else
    {
      Region = FTerminal->GetSessionData()->GetS3DefaultRegion();
      if (First)
      {
        FTerminal->LogEvent(FORMAT("Unknown bucket \"%s\", will detect its region (and service endpoint)", ABucketName));
        First = false;
      }
      Retry = true;
    }

    I = FHostNames.find(ABucketName);
    UnicodeString HostName;
    if (I != FHostNames.end())
    {
      HostName = I->second;
    }
    else
    {
      HostName = UnicodeString(FHostName);
    }

    Result.HostNameBuf = UTF8String(HostName);
    Result.hostName = Result.HostNameBuf.c_str();
    Result.BucketNameBuf = UTF8String(ABucketName);
    Result.bucketName = Result.BucketNameBuf.c_str();
    Result.protocol = FLibS3Protocol;
    Result.uriStyle = S3UriStyleVirtualHost;
    Result.accessKeyId = FAccessKeyId.c_str();
    Result.secretAccessKey = FSecretAccessKey.c_str();
    Result.securityToken = nullptr;
    Result.AuthRegionBuf = UTF8String(Region);
    Result.authRegion = Result.AuthRegionBuf.c_str();

    if (Retry)
    {
      std::unique_ptr<TRemoteFileList> FileList(new TRemoteFileList());
      TLibS3ListBucketCallbackData Data;
      DoListBucket(UnicodeString(), FileList.get(), 1, Result, Data);

      Retry = false;
      UnicodeString EndpointDetail = Data.EndpointDetail;
      if ((Data.Status == S3StatusErrorAuthorizationHeaderMalformed) &&
          (Region != Data.RegionDetail))
      {
        FTerminal->LogEvent(FORMAT("Will use region \"%s\" for bucket \"%s\" from now on.", Data.RegionDetail, ABucketName));
        FRegions.insert(TRegions::value_type(ABucketName, Data.RegionDetail));

        Result.AuthRegionBuf = UTF8String(Data.RegionDetail);
        Result.authRegion = Result.AuthRegionBuf.c_str();
      }
      // happens with newly created buckets (and happens before the region redirect)
      else if ((Data.Status == S3StatusErrorTemporaryRedirect) && !Data.EndpointDetail.IsEmpty())
      {
        UnicodeString Endpoint = Data.EndpointDetail;
        if (SameText(Endpoint.SubString(1, ABucketName.Length() + 1), ABucketName + L"."))
        {
          Endpoint.Delete(1, ABucketName.Length() + 1);
        }
        if (HostName != Endpoint)
        {
          FTerminal->LogEvent(FORMAT("Will use endpoint \"%s\" for bucket \"%s\" from now on.", Endpoint, ABucketName));
          FHostNames.insert(TRegions::value_type(ABucketName, Endpoint));
          Retry = true;
        }
      }
    }
  }
  while (Retry);

  return Result;
}
//---------------------------------------------------------------------------
#define CreateResponseHandlerCustom(PropertiesCallback) { &PropertiesCallback, &LibS3ResponseCompleteCallback }
#define CreateResponseHandler() CreateResponseHandlerCustom(LibS3ResponsePropertiesCallback)
//---------------------------------------------------------------------------
void TS3FileSystem::Close()
{
  DebugAssert(FActive);
  LibS3Deinitialize();
  FTerminal->Closed();
  FActive = false;
  UnregisterFromNeonDebug(FTerminal);
}
//---------------------------------------------------------------------------
bool TS3FileSystem::GetActive() const
{
  return FActive;
}
//---------------------------------------------------------------------------
void TS3FileSystem::CollectUsage()
{
  // noop
}
//---------------------------------------------------------------------------
const TSessionInfo & TS3FileSystem::GetSessionInfo() const
{
  return FSessionInfo;
}
//---------------------------------------------------------------------------
const TFileSystemInfo & TS3FileSystem::GetFileSystemInfo(bool /*Retrieve*/)
{
  return FFileSystemInfo;
}
//---------------------------------------------------------------------------
bool TS3FileSystem::TemporaryTransferFile(const UnicodeString /*AFileName*/)
{
  return false;
}
//---------------------------------------------------------------------------
bool TS3FileSystem::GetStoredCredentialsTried() const
{
  // if we have one, we always try it
  return !FTerminal->GetSessionData()->GetPassword().IsEmpty();
}
//---------------------------------------------------------------------------
UnicodeString TS3FileSystem::RemoteGetUserName() const
{
  return UnicodeString(FAccessKeyId);
}
//---------------------------------------------------------------------------
void TS3FileSystem::Idle()
{
  // noop
}
//---------------------------------------------------------------------------
UnicodeString TS3FileSystem::GetAbsolutePath(const UnicodeString APath, bool Local)
{
  return static_cast<const TS3FileSystem*>(this)->GetAbsolutePath(APath, Local);
}

UnicodeString TS3FileSystem::GetAbsolutePath(const UnicodeString Path, bool /*Local*/) const
{
  if (base::UnixIsAbsolutePath(Path))
  {
    return Path;
  }
  else
  {
    return base::AbsolutePath(FCurrentDirectory, Path);
  }
}
//---------------------------------------------------------------------------
bool TS3FileSystem::IsCapable(intptr_t Capability) const
{
  DebugAssert(FTerminal);
  switch (Capability)
  {
    // Only to make double-click on file edit/open the file,
    // instead of trying to open it as directory
    case fcResolveSymlink:
    case fcRemoteCopy:
    case fcRename:
    case fcRemoteMove:
    case fcMoveToQueue:
    case fsSkipTransfer:
    case fsParallelTransfers:
      return true;

    case fcPreservingTimestampUpload:
    case fcCheckingSpaceAvailable:
    case fcUserGroupListing:
    case fcModeChanging:
    case fcModeChangingUpload:
    case fcGroupChanging:
    case fcOwnerChanging:
    case fcAnyCommand:
    case fcShellAnyCommand:
    case fcHardLink:
    case fcSymbolicLink:
    case fcTextMode:
    case fcNativeTextMode:
    case fcNewerOnlyUpload:
    case fcTimestampChanging:
    case fcLoadingAdditionalProperties:
    case fcIgnorePermErrors:
    case fcCalculatingChecksum:
    case fcSecondaryShell:
    case fcGroupOwnerChangingByID:
    case fcRemoveCtrlZUpload:
    case fcRemoveBOMUpload:
    case fcPreservingTimestampDirs:
    case fcResumeSupport:
    case fcChangePassword:
    case fcLocking:
      return false;

    default:
      DebugFail();
      return false;
  }
}
//---------------------------------------------------------------------------
UnicodeString TS3FileSystem::RemoteGetCurrentDirectory() const
{
  return FCurrentDirectory;
}
//---------------------------------------------------------------------------
void TS3FileSystem::DoStartup()
{
  FTerminal->SetExceptionOnFail(true);
  // retrieve initialize working directory to save it as home directory
  ReadCurrentDirectory();
  FTerminal->SetExceptionOnFail(false);
}
//---------------------------------------------------------------------------
void TS3FileSystem::LookupUsersGroups()
{
  DebugFail();
}
//---------------------------------------------------------------------------
void TS3FileSystem::ReadCurrentDirectory()
{
  if (FCachedDirectoryChange.IsEmpty())
  {
    FCurrentDirectory = FCurrentDirectory.IsEmpty() ? UnicodeString(L"/") : FCurrentDirectory;
  }
  else
  {
    FCurrentDirectory = FCachedDirectoryChange;
    FCachedDirectoryChange = L"";
  }
}
//---------------------------------------------------------------------------
void TS3FileSystem::HomeDirectory()
{
  ChangeDirectory(L"/");
}
//---------------------------------------------------------------------------
void TS3FileSystem::AnnounceFileListOperation()
{
  // noop
}
//---------------------------------------------------------------------------
void TS3FileSystem::TryOpenDirectory(const UnicodeString ADirectory)
{
  FTerminal->LogEvent(FORMAT("Trying to open directory \"%s\".", ADirectory));
  std::unique_ptr<TRemoteFileList> FileList(new TRemoteFileList());
  ReadDirectoryInternal(ADirectory, FileList.get(), 1, UnicodeString());
}
//---------------------------------------------------------------------------
void TS3FileSystem::ChangeDirectory(const UnicodeString ADirectory)
{
  UnicodeString Path = GetAbsolutePath(ADirectory, false);

  // to verify existence of directory try to open it
  TryOpenDirectory(Path);

  // if open dir did not fail, directory exists -> success.
  FCachedDirectoryChange = Path;
}
//---------------------------------------------------------------------------
void TS3FileSystem::CachedChangeDirectory(const UnicodeString Directory)
{
  FCachedDirectoryChange = base::UnixExcludeTrailingBackslash(Directory);
}
//---------------------------------------------------------------------------
TRemoteToken TS3FileSystem::MakeRemoteToken(const char *OwnerId, const char *OwnerDisplayName)
{
  TRemoteToken Result;
  Result.SetName(StrFromS3(OwnerDisplayName));
  if (Result.GetName().IsEmpty())
  {
    Result.SetName(StrFromS3(OwnerId));
  }
  return Result;
}
//---------------------------------------------------------------------------
struct TLibS3ListServiceCallbackData : TLibS3CallbackData
{
CUSTOM_MEM_ALLOCATION_IMPL

  TRemoteFileList * FileList;
  UnicodeString FileName; // filter for buckets
};
//---------------------------------------------------------------------------
S3Status TS3FileSystem::LibS3ListServiceCallback(
  const char * OwnerId, const char * OwnerDisplayName, const char * BucketName,
  int64_t /*CreationDate*/, void * CallbackData)
{
  TLibS3ListServiceCallbackData & Data = *static_cast<TLibS3ListServiceCallbackData *>(CallbackData);

  UnicodeString FileName = StrFromS3(BucketName);
  if (Data.FileName.IsEmpty() || (Data.FileName == FileName))
  {
    std::unique_ptr<TRemoteFile> File(new TRemoteFile(nullptr));
    File->SetTerminal(Data.FileSystem->FTerminal);
    File->SetFileName(StrFromS3(BucketName));
    File->SetType(FILETYPE_DIRECTORY);
    File->SetFileOwner(Data.FileSystem->MakeRemoteToken(OwnerId, OwnerDisplayName));
    File->SetModificationFmt(mfNone);
    Data.FileList->AddFile(File.release());
  }

  return S3StatusOK;
}
//---------------------------------------------------------------------------
S3Status TS3FileSystem::LibS3ListBucketCallback(
  int IsTruncated, const char * NextMarker, int ContentsCount, const S3ListBucketContent * Contents,
  int CommonPrefixesCount, const char ** CommonPrefixes, void * CallbackData)
{
  TLibS3ListBucketCallbackData & Data = *static_cast<TLibS3ListBucketCallbackData *>(CallbackData);

  Data.IsTruncated = IsTruncated != 0;
  // This is being called in chunks, not once for all data in a response.
  Data.KeyCount += ContentsCount;
  Data.NextMarker = StrFromS3(NextMarker);

  for (int Index = 0; Index < ContentsCount; Index++)
  {
    const S3ListBucketContent * Content = &Contents[Index];
    UnicodeString FileName = base::UnixExtractFileName(StrFromS3(Content->key));
    if (!FileName.IsEmpty())
    {
      std::unique_ptr<TRemoteFile> File(new TRemoteFile(nullptr));
      File->SetTerminal(Data.FileSystem->FTerminal);
      File->SetFileName(FileName);
      File->SetType(FILETYPE_DEFAULT);
      File->SetModification(UnixToDateTime(Content->lastModified, dstmWin));
      File->SetSize(Content->size);
      File->SetFileOwner(Data.FileSystem->MakeRemoteToken(Content->ownerId, Content->ownerDisplayName));
      Data.FileList->AddFile(File.release());
    }
    else
    {
      // We needs this to distinguish empty and non-existing folders, see comments in ReadDirectoryInternal.
      Data.FileList->AddFile(new TRemoteParentDirectory(Data.FileSystem->FTerminal));
    }
  }

  for (int Index = 0; Index < CommonPrefixesCount; Index++)
  {
    std::unique_ptr<TRemoteFile> File(new TRemoteFile(nullptr));
    File->SetTerminal(Data.FileSystem->FTerminal);
    File->SetFileName(base::UnixExtractFileName(base::UnixExcludeTrailingBackslash(StrFromS3(CommonPrefixes[Index]))));
    File->SetType(FILETYPE_DIRECTORY);
    File->SetModificationFmt(mfNone);
    Data.FileList->AddFile(File.release());
  }

  return S3StatusOK;
}
//---------------------------------------------------------------------------
void TS3FileSystem::DoListBucket(
  const UnicodeString APrefix, TRemoteFileList *FileList, intptr_t MaxKeys, const TLibS3BucketContext &BucketContext,
  TLibS3ListBucketCallbackData &Data)
{
  S3ListBucketHandler ListBucketHandler = { CreateResponseHandler(), &LibS3ListBucketCallback };
  RequestInit(Data);
  Data.KeyCount = 0;
  Data.FileList = FileList;
  Data.IsTruncated = false;

  S3_list_bucket(
    &BucketContext, StrToS3(APrefix), StrToS3(Data.NextMarker),
    LibS3Delimiter.c_str(), ToInt(MaxKeys), FRequestContext, FTimeout, &ListBucketHandler, &Data);
}
//---------------------------------------------------------------------------
void TS3FileSystem::ReadDirectoryInternal(
  const UnicodeString APath, TRemoteFileList * FileList, intptr_t MaxKeys, const UnicodeString AFileName)
{
  UnicodeString Path = GetAbsolutePath(APath, false);
  if (base::IsUnixRootPath(Path))
  {
    DebugAssert(FileList != nullptr);

    S3ListServiceHandler ListServiceHandler = { CreateResponseHandler(), &LibS3ListServiceCallback };

    TLibS3ListServiceCallbackData Data;
    RequestInit(Data);
    Data.FileSystem = this;
    Data.FileList = FileList;
    Data.FileName = AFileName;

    S3_list_service(
      FLibS3Protocol, FAccessKeyId.c_str(), FSecretAccessKey.c_str(), 0, FHostName.c_str(),
      nullptr, ToInt(MaxKeys), FRequestContext, FTimeout, &ListServiceHandler, &Data);

    CheckLibS3Error(Data);
  }
  else
  {
    UnicodeString BucketName, Prefix;
    ParsePath(Path, BucketName, Prefix);
    if (!Prefix.IsEmpty())
    {
      Prefix = GetFolderKey(Prefix);
    }
    Prefix += AFileName;
    TLibS3BucketContext BucketContext = GetBucketContext(BucketName);

    TLibS3ListBucketCallbackData Data;
    bool Continue;

    do
    {
      DoListBucket(Prefix, FileList, MaxKeys, BucketContext, Data);
      CheckLibS3Error(Data);

      Continue = false;

      if (Data.IsTruncated && ((MaxKeys == 0) || (Data.KeyCount < MaxKeys)))
      {
        bool Cancel = false;
        FTerminal->DoReadDirectoryProgress(FileList->GetCount(), false, Cancel);
        if (!Cancel)
        {
          Continue = true;
        }
      }
    } while (Continue);

    // Listing bucket root directory will report an error if the bucket does not exist.
    // But there won't be any prefix/ entry, so no ".." entry is created, so we have to add it explicitly
    if (Prefix.IsEmpty())
    {
      FileList->AddFile(new TRemoteParentDirectory(FTerminal));
    }
    else
    {
      // We do not get any error, when the "prefix" does not exist. But when prefix does exist, there's at least
      // prefix/ entry (translated to ..). If there's none, it means that the path does not exist.
      // When called from DoReadFile (FileName is set), leaving error handling to the caller.
      if ((FileList->GetCount() == 0) && AFileName.IsEmpty())
      {
        throw Exception(FMTLOAD(FILE_NOT_EXISTS, APath));
      }
    }
  }
}
//---------------------------------------------------------------------------
void TS3FileSystem::ReadDirectory(TRemoteFileList * FileList)
{
  volatile TOperationVisualizer Visualizer(FTerminal->GetUseBusyCursor());
  ReadDirectoryInternal(FileList->GetDirectory(), FileList, 0, UnicodeString());
}
//---------------------------------------------------------------------------
void TS3FileSystem::ReadSymlink(TRemoteFile * /*SymlinkFile*/,
  TRemoteFile *& /*File*/)
{
  // we never set SymLink flag, so we should never get here
  DebugFail();
}
//---------------------------------------------------------------------------
void TS3FileSystem::DoReadFile(const UnicodeString AFileName, TRemoteFile *& AFile)
{
  UnicodeString FileNameOnly = base::UnixExtractFileName(AFileName);
  std::unique_ptr<TRemoteFileList> FileList(new TRemoteFileList());
  ReadDirectoryInternal(base::UnixExtractFileDir(AFileName), FileList.get(), 1, FileNameOnly);
  TRemoteFile *File = FileList->FindFile(FileNameOnly);
  if (File != nullptr)
  {
    AFile = File->Duplicate();
  }
  else
  {
    AFile = nullptr;
  }
}
//---------------------------------------------------------------------------
void TS3FileSystem::ReadFile(const UnicodeString AFileName,
  TRemoteFile *& File)
{
  volatile TOperationVisualizer Visualizer(FTerminal->GetUseBusyCursor());
  DoReadFile(AFileName, File);
  if (File == nullptr)
  {
    throw Exception(FMTLOAD(FILE_NOT_EXISTS, AFileName));
  }
}
//---------------------------------------------------------------------------
void TS3FileSystem::RemoteDeleteFile(const UnicodeString AFileName,
  const TRemoteFile *AFile, intptr_t AParams, TRmSessionAction & Action)
{
  UnicodeString FileName = GetAbsolutePath(AFileName, false);

  bool Dir = FTerminal->DeleteContentsIfDirectory(FileName, AFile, AParams, Action);

  UnicodeString BucketName, Key;
  ParsePath(FileName, BucketName, Key);

  TLibS3BucketContext BucketContext = GetBucketContext(BucketName);

  S3ResponseHandler ResponseHandler = CreateResponseHandler();

  TLibS3CallbackData Data;
  RequestInit(Data);

  if (Key.IsEmpty())
  {
    S3_delete_bucket(
      BucketContext.protocol, BucketContext.uriStyle, BucketContext.accessKeyId, BucketContext.secretAccessKey,
      BucketContext.securityToken, BucketContext.hostName, BucketContext.bucketName, BucketContext.authRegion,
      FRequestContext, FTimeout, &ResponseHandler, &Data);
  }
  else
  {
    if (Dir)
    {
      Key = GetFolderKey(Key);
    }
    S3_delete_object(&BucketContext, StrToS3(Key), FRequestContext, FTimeout, &ResponseHandler, &Data);
  }

  CheckLibS3Error(Data);
}
//---------------------------------------------------------------------------
void TS3FileSystem::RemoteRenameFile(const UnicodeString AFileName, const TRemoteFile *AFile,
  const UnicodeString ANewName)
{
  RemoteCopyFile(AFileName, AFile, ANewName);
  TRmSessionAction DummyAction(FTerminal->GetActionLog(), AFileName);
  RemoteDeleteFile(AFileName, AFile, dfForceDelete, DummyAction);
  DummyAction.Cancel();
}
//---------------------------------------------------------------------------
void TS3FileSystem::RemoteCopyFile(const UnicodeString AFileName, const TRemoteFile *AFile,
  const UnicodeString ANewName)
{
  if (DebugAlwaysTrue(AFile != nullptr) && AFile->GetIsDirectory())
  {
    throw Exception(LoadStr(DUPLICATE_FOLDER_NOT_SUPPORTED));
  }

  UnicodeString FileName = GetAbsolutePath(AFileName, false);
  UnicodeString NewName = GetAbsolutePath(ANewName, false);

  UnicodeString SourceBucketName, SourceKey;
  ParsePath(FileName, SourceBucketName, SourceKey);
  DebugAssert(!SourceKey.IsEmpty()); // it's not a folder, so it cannot be a bucket or root

  UnicodeString DestBucketName, DestKey;
  ParsePath(NewName, DestBucketName, DestKey);

  if (DestKey.IsEmpty())
  {
    throw Exception(LoadStr(MISSING_TARGET_BUCKET));
  }

  TLibS3BucketContext BucketContext = GetBucketContext(DestBucketName);
  BucketContext.BucketNameBuf = SourceBucketName;
  BucketContext.bucketName = BucketContext.BucketNameBuf.c_str();

  S3ResponseHandler ResponseHandler = CreateResponseHandler();

  TLibS3CallbackData Data;
  RequestInit(Data);

  S3_copy_object(
    &BucketContext, StrToS3(SourceKey), StrToS3(DestBucketName), StrToS3(DestKey),
    nullptr, nullptr, 0, nullptr, FRequestContext, FTimeout, &ResponseHandler, &Data);

  CheckLibS3Error(Data);
}
//---------------------------------------------------------------------------
void TS3FileSystem::RemoteCreateDirectory(const UnicodeString ADirName)
{
  volatile TOperationVisualizer Visualizer(FTerminal->GetUseBusyCursor());
  UnicodeString DirName = base::UnixExcludeTrailingBackslash(GetAbsolutePath(ADirName, false));

  UnicodeString BucketName, Key;
  ParsePath(DirName, BucketName, Key);

  TLibS3CallbackData Data;
  RequestInit(Data);

  if (Key.IsEmpty())
  {
    S3ResponseHandler ResponseHandler = CreateResponseHandler();

    // Not using GetBucketContext here, as the bucket does not exist

    UTF8String RegionBuf;
    const char * Region = nullptr;
    if (FTerminal->GetSessionData()->GetS3DefaultRegion() != S3LibDefaultRegion())
    {
      RegionBuf = UTF8String(FTerminal->GetSessionData()->GetS3DefaultRegion());
      Region = RegionBuf.c_str();
    }

    S3_create_bucket(
      FLibS3Protocol, FAccessKeyId.c_str(), FSecretAccessKey.c_str(), nullptr, FHostName.c_str(), StrToS3(BucketName),
      StrToS3(S3LibDefaultRegion()), S3CannedAclPrivate, Region, FRequestContext, FTimeout, &ResponseHandler, &Data);
  }
  else
  {
    Key = GetFolderKey(Key);

    TLibS3BucketContext BucketContext = GetBucketContext(BucketName);

    S3PutObjectHandler PutObjectHandler = { CreateResponseHandler(), nullptr };

    S3_put_object(&BucketContext, StrToS3(Key), 0, nullptr, FRequestContext, FTimeout, &PutObjectHandler, &Data);
  }

  CheckLibS3Error(Data);
}
//---------------------------------------------------------------------------
void TS3FileSystem::RemoteCreateLink(const UnicodeString /*AFileName*/,
  const UnicodeString /*PointTo*/, bool /*Symbolic*/)
{
  DebugFail();
}
//---------------------------------------------------------------------------
void TS3FileSystem::ChangeFileProperties(const UnicodeString FileName,
  const TRemoteFile * /*File*/, const TRemoteProperties * /*Properties*/,
  TChmodSessionAction & /*Action*/)
{
  DebugFail();
}
//---------------------------------------------------------------------------
bool TS3FileSystem::LoadFilesProperties(TStrings * /*FileList*/)
{
  DebugFail();
  return false;
}
//---------------------------------------------------------------------------
void TS3FileSystem::CalculateFilesChecksum(const UnicodeString /*Alg*/,
    TStrings * /*FileList*/, TStrings * /*Checksums*/,
    TCalculatedChecksumEvent /*OnCalculatedChecksum*/)
{
  DebugFail();
}
//---------------------------------------------------------------------------
void TS3FileSystem::CustomCommandOnFile(const UnicodeString /*AFileName*/,
  const TRemoteFile * /*AFile*/, const UnicodeString /*ACommand*/, intptr_t /*AParams*/, TCaptureOutputEvent /*OutputEvent*/)
{
  DebugFail();
}
//---------------------------------------------------------------------------
void TS3FileSystem::AnyCommand(const UnicodeString /*ACommand*/,
  TCaptureOutputEvent /*OutputEvent*/)
{
  DebugFail();
}
//---------------------------------------------------------------------------
TStrings * TS3FileSystem::GetFixedPaths() const
{
  return nullptr;
}
//---------------------------------------------------------------------------
void TS3FileSystem::SpaceAvailable(const UnicodeString /*APath*/,
  TSpaceAvailable & /*ASpaceAvailable*/)
{
  DebugFail();
}
//---------------------------------------------------------------------------
void TS3FileSystem::CopyToRemote(
  TStrings *AFilesToCopy, const UnicodeString ATargetDir, const TCopyParamType *CopyParam,
  intptr_t AParams, TFileOperationProgressType *OperationProgress, TOnceDoneOperation &OnceDoneOperation)
{
  AParams &= ~cpAppend;

  FTerminal->DoCopyToRemote(AFilesToCopy, ATargetDir, CopyParam, AParams, OperationProgress, tfPreCreateDir, OnceDoneOperation);
}
//---------------------------------------------------------------------------
void TS3FileSystem::ConfirmOverwrite(
  const UnicodeString ASourceFullFileName, UnicodeString &ATargetFileName,
  TFileOperationProgressType *OperationProgress, const TOverwriteFileParams *FileParams,
  const TCopyParamType *CopyParam, intptr_t AParams)
{
  uint32_t Answers = qaYes | qaNo | qaCancel | qaYesToAll | qaNoToAll;
  std::vector<TQueryButtonAlias> Aliases;
  Aliases.push_back(TQueryButtonAlias::CreateYesToAllGrouppedWithYes());
  Aliases.push_back(TQueryButtonAlias::CreateNoToAllGrouppedWithNo());

  TQueryParams QueryParams(qpNeverAskAgainCheck);
  QueryParams.Aliases = &Aliases[0];
  QueryParams.AliasesCount = Aliases.size();

  uint32_t Answer;

  {
    volatile TSuspendFileOperationProgress Suspend(OperationProgress);
    Answer =
      FTerminal->ConfirmFileOverwrite(
        ASourceFullFileName, ATargetFileName, FileParams, Answers, &QueryParams,
        ReverseOperationSide(OperationProgress->GetSide()),
        CopyParam, AParams, OperationProgress);
  }

  switch (Answer)
  {
    case qaYes:
      // noop
      break;

    case qaNo:
      throw ESkipFile();

    default:
      DebugFail();
	  break;
	case qaCancel:
      OperationProgress->SetCancelAtLeast(csCancel);
      Abort();
      break;
  }
}
//---------------------------------------------------------------------------
struct TLibS3TransferObjectDataCallbackData : TLibS3CallbackData
{
CUSTOM_MEM_ALLOCATION_IMPL

  UnicodeString FileName;
  TStream *Stream;
  TFileOperationProgressType *OperationProgress;
  std::auto_ptr<Exception> Exception;
};
//---------------------------------------------------------------------------
struct TLibS3PutObjectDataCallbackData : TLibS3TransferObjectDataCallbackData
{
CUSTOM_MEM_ALLOCATION_IMPL

  RawByteString ETag;
};
//---------------------------------------------------------------------------
int TS3FileSystem::LibS3PutObjectDataCallback(int BufferSize, char * Buffer, void * CallbackData)
{
  TLibS3PutObjectDataCallbackData &Data = *static_cast<TLibS3PutObjectDataCallbackData *>(CallbackData);

  return Data.FileSystem->PutObjectData(BufferSize, Buffer, Data);
}
//---------------------------------------------------------------------------
bool TS3FileSystem::ShouldCancelTransfer(TLibS3TransferObjectDataCallbackData &Data)
{
  bool Result = (Data.OperationProgress->GetCancel() != csContinue);
  if (Result)
  {
    if (Data.OperationProgress->ClearCancelFile())
    {
      Data.Exception.reset(new ESkipFile());
    }
    else
    {
      Data.Exception.reset(new EAbort(L""));
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
int TS3FileSystem::PutObjectData(int BufferSize, char * Buffer, TLibS3PutObjectDataCallbackData & Data)
{
  int Result;

  if (ShouldCancelTransfer(Data))
  {
    Result = -1;
  }
  else
  {
    TFileOperationProgressType * OperationProgress = Data.OperationProgress;
    try
    {
      FileOperationLoopCustom(FTerminal, OperationProgress, folAllowSkip,
        FMTLOAD(READ_ERROR, Data.FileName), "",
      [&]()
      {
        Result = ToInt(Data.Stream->Read(Buffer, BufferSize));
      });
      __removed FILE_OPERATION_LOOP_END(FMTLOAD(READ_ERROR, (Data.FileName)));

      OperationProgress->AddTransferred(Result);
    }
    catch (Exception &E)
    {
      Data.Exception.reset(CloneException(&E));
      Result = -1;
    }
  }

  return Result;
}
//---------------------------------------------------------------------------
struct TLibS3MultipartInitialCallbackData : TLibS3CallbackData
{
  RawByteString UploadId;
};
//---------------------------------------------------------------------------
S3Status TS3FileSystem::LibS3MultipartInitialCallback(const char * UploadId, void * CallbackData)
{
  TLibS3MultipartInitialCallbackData & Data = *static_cast<TLibS3MultipartInitialCallbackData *>(CallbackData);

  Data.UploadId = UploadId;

  return S3StatusOK;
}
//---------------------------------------------------------------------------
struct TLibS3MultipartCommitPutObjectDataCallbackData : TLibS3CallbackData
{
  RawByteString Message;
  int Remaining;
};
//---------------------------------------------------------------------------
S3Status TS3FileSystem::LibS3MultipartResponsePropertiesCallback(
  const S3ResponseProperties * Properties, void * CallbackData)
{
  S3Status Result = LibS3ResponsePropertiesCallback(Properties, CallbackData);

  TLibS3PutObjectDataCallbackData & Data = *static_cast<TLibS3PutObjectDataCallbackData *>(CallbackData);

  Data.ETag = Properties->eTag;

  return Result;
}
//---------------------------------------------------------------------------
int TS3FileSystem::LibS3MultipartCommitPutObjectDataCallback(int BufferSize, char * Buffer, void * CallbackData)
{
  TLibS3MultipartCommitPutObjectDataCallbackData & Data =
    *static_cast<TLibS3MultipartCommitPutObjectDataCallbackData *>(CallbackData);
  int Result = 0;
  if (Data.Remaining > 0)
  {
    Result = std::min(BufferSize, Data.Remaining);
    memcpy(Buffer, Data.Message.c_str() + Data.Message.Length() - Data.Remaining, Result);
    Data.Remaining -= Result;
  }
  return Result;
}
//---------------------------------------------------------------------------
void TS3FileSystem::Source(
  TLocalFileHandle &AHandle, const UnicodeString TargetDir, UnicodeString &ADestFileName,
  const TCopyParamType *CopyParam, intptr_t Params,
  TFileOperationProgressType *OperationProgress, uintptr_t /*Flags*/,
  TUploadSessionAction &Action, bool & /*ChildError*/)
{
  UnicodeString DestFullName = TargetDir + ADestFileName;

  TRemoteFile * RemoteFile = nullptr;
  try
  {
    // Should not throw on non-existing file by purpose (mainly not to get an exception while debugging)
    DoReadFile(DestFullName, RemoteFile);
  }
  catch (...)
  {
    // Pointless, as there's no persistent connection.
    if (!FTerminal->Active)
    {
      throw;
    }
  }

  if (RemoteFile != nullptr)
  {
    TOverwriteFileParams FileParams;

    FileParams.SourceSize = AHandle.Size;
    FileParams.SourceTimestamp = AHandle.Modification;
    FileParams.DestSize = RemoteFile->Size;
    FileParams.DestTimestamp = TDateTime();
    FileParams.DestPrecision = mfNone;
    delete RemoteFile;

    ConfirmOverwrite(AHandle.FileName, ADestFileName, OperationProgress, &FileParams, CopyParam, Params);
  }

  DestFullName = TargetDir + ADestFileName;
  // only now, we know the final destination
  // (not really true as we do not support changing file name on overwrite dialog)
  Action.Destination(DestFullName);

  UnicodeString BucketName, Key;
  ParsePath(DestFullName, BucketName, Key);

  TLibS3BucketContext BucketContext = GetBucketContext(BucketName);

  UTF8String ContentType = UTF8String(FTerminal->Configuration->GetFileMimeType(AHandle.FileName));
  S3PutProperties PutProperties =
    {
      (ContentType.IsEmpty() ? nullptr : ContentType.c_str()),
      nullptr,
      nullptr,
      nullptr,
      nullptr,
      -1,
      S3CannedAclPrivate,
      0,
      nullptr,
      0
    };

  int Parts = std::max(1, ToInt((AHandle.Size + S3MultiPartChunkSize - 1) / S3MultiPartChunkSize));
  bool Multipart = (Parts > 1);

  RawByteString MultipartUploadId;
  TLibS3MultipartCommitPutObjectDataCallbackData MultipartCommitPutObjectDataCallbackData;

  if (Multipart)
  {
    FTerminal->LogEvent(FORMAT("Initiating multipart upload (%d parts)", Parts));

    FileOperationLoopCustom(FTerminal, OperationProgress, folAllowSkip,
      FMTLOAD(TRANSFER_ERROR, AHandle.FileName), "",
    [&]()
    {
      TLibS3MultipartInitialCallbackData Data;
      RequestInit(Data);

      S3MultipartInitialHandler Handler = { CreateResponseHandler(), &LibS3MultipartInitialCallback };

      S3_initiate_multipart(&BucketContext, StrToS3(Key), &PutProperties, &Handler, FRequestContext, FTimeout, &Data);

      CheckLibS3Error(Data, true);

      MultipartUploadId = Data.UploadId;
    });
    __removed FILE_OPERATION_LOOP_END_EX(FMTLOAD(TRANSFER_ERROR, (AHandle.FileName)), (folAllowSkip | folRetryOnFatal));

    FTerminal->LogEvent(FORMAT("Initiated multipart upload (%s - %d parts)", UnicodeString(MultipartUploadId), Parts));

    MultipartCommitPutObjectDataCallbackData.Message += "<CompleteMultipartUpload>\n";
  }

  try
  {
    TLibS3PutObjectDataCallbackData Data;

    int64_t Position = 0;

    std::unique_ptr<TStream> Stream(new TSafeHandleStream(reinterpret_cast<THandle>(AHandle.Handle)));

    for (intptr_t Part = 1; Part <= Parts; Part++)
    {
      FileOperationLoopCustom(FTerminal, OperationProgress, folAllowSkip,
        FMTLOAD(TRANSFER_ERROR, AHandle.FileName), "",
      [&]()
      {
        DebugAssert(Stream->Position == OperationProgress->TransferredSize);

        // If not, it's chunk retry and we have to undo the unsuccessful chunk upload
        if (Position < Stream->Position)
        {
          Stream->Position = Position;
          OperationProgress->AddTransferred(Position - OperationProgress->TransferredSize);
        }

        RequestInit(Data);
        Data.FileName = AHandle.FileName;
        Data.Stream = Stream.get();
        Data.OperationProgress = OperationProgress;
        Data.Exception.reset(nullptr);

        if (Multipart)
        {
          S3PutObjectHandler UploadPartHandler =
            { CreateResponseHandlerCustom(LibS3MultipartResponsePropertiesCallback), LibS3PutObjectDataCallback };
          int PartLength = std::min(S3MultiPartChunkSize, ToInt(Stream->Size - Stream->Position));
          FTerminal->LogEvent(FORMAT("Uploading part %d [%s]", Part, IntToStr(PartLength)));
          S3_upload_part(
            &BucketContext, StrToS3(Key), &PutProperties, &UploadPartHandler, ToInt(Part), MultipartUploadId.c_str(),
            PartLength, FRequestContext, FTimeout, &Data);
        }
        else
        {
          S3PutObjectHandler PutObjectHandler = { CreateResponseHandler(), LibS3PutObjectDataCallback };
          S3_put_object(&BucketContext, StrToS3(Key), AHandle.Size, &PutProperties, FRequestContext, FTimeout, &PutObjectHandler, &Data);
        }

        // The "exception" was already seen by the user, its presence mean an accepted abort of the operation.
        if (Data.Exception.get() == nullptr)
        {
          CheckLibS3Error(Data, true);
        }

        Position = Stream->Position;

        if (Multipart)
        {
          RawByteString PartCommitTag =
            FORMAT("  <Part><PartNumber>%d</PartNumber><ETag>%s</ETag></Part>\n", Part, Data.ETag);
          MultipartCommitPutObjectDataCallbackData.Message += PartCommitTag;
        }
      });
      __removed FILE_OPERATION_LOOP_END_EX(FMTLOAD(TRANSFER_ERROR, (AHandle.FileName)), (folAllowSkip | folRetryOnFatal));

      if (Data.Exception.get() != nullptr)
      {
        RethrowException(Data.Exception.get());
      }
    }

    Stream.reset(nullptr);

    if (Multipart)
    {
      MultipartCommitPutObjectDataCallbackData.Message += "</CompleteMultipartUpload>\n";

      FTerminal->LogEvent(FORMAT("Committing multipart upload (%s - %d parts)", UnicodeString(MultipartUploadId), Parts));
      FTerminal->LogEvent(UnicodeString(MultipartCommitPutObjectDataCallbackData.Message));

      FileOperationLoopCustom(FTerminal, OperationProgress, folAllowSkip,
        FMTLOAD(TRANSFER_ERROR, AHandle.FileName), "",
      [&]()
      {
        RequestInit(MultipartCommitPutObjectDataCallbackData);

        MultipartCommitPutObjectDataCallbackData.Remaining = ToInt(MultipartCommitPutObjectDataCallbackData.Message.Length());

        S3MultipartCommitHandler MultipartCommitHandler =
          { CreateResponseHandler(), &LibS3MultipartCommitPutObjectDataCallback, nullptr };

        S3_complete_multipart_upload(
          &BucketContext, StrToS3(Key), &MultipartCommitHandler, MultipartUploadId.c_str(),
          MultipartCommitPutObjectDataCallbackData.Remaining,
          FRequestContext, FTimeout, &MultipartCommitPutObjectDataCallbackData);

        CheckLibS3Error(MultipartCommitPutObjectDataCallbackData, true);
      });
      __removed FILE_OPERATION_LOOP_END_EX(FMTLOAD(TRANSFER_ERROR, (AHandle.FileName)), (folAllowSkip | folRetryOnFatal));

      // to skip abort, in case we ever add any code before the catch, that can throw
      MultipartUploadId = RawByteString();
    }
  }
  catch (Exception &/*E*/)
  {
    if (!MultipartUploadId.IsEmpty())
    {
      FTerminal->LogEvent(FORMAT("Aborting multipart upload (%s - %d parts)", UnicodeString(MultipartUploadId), Parts));

      try
      {
        TLibS3CallbackData Data;
        RequestInit(Data);

        S3AbortMultipartUploadHandler AbortMultipartUploadHandler = { CreateResponseHandler() };

        S3_abort_multipart_upload(
          &BucketContext, StrToS3(Key), MultipartUploadId.c_str(),
          FTimeout, &AbortMultipartUploadHandler, FRequestContext, &Data);
      }
      catch (...)
      {
        // swallow
      }
    }

    throw;
  }
}
//---------------------------------------------------------------------------
void TS3FileSystem::CopyToLocal(
  TStrings *FilesToCopy, const UnicodeString ATargetDir, const TCopyParamType *CopyParam,
  intptr_t AParams, TFileOperationProgressType *OperationProgress, TOnceDoneOperation &OnceDoneOperation)
{
  AParams &= ~cpAppend;

  FTerminal->DoCopyToLocal(FilesToCopy, ATargetDir, CopyParam, AParams, OperationProgress, tfNone, OnceDoneOperation);
}
//---------------------------------------------------------------------------
struct TLibS3GetObjectDataCallbackData : TLibS3TransferObjectDataCallbackData
{
};
//---------------------------------------------------------------------------
S3Status TS3FileSystem::LibS3GetObjectDataCallback(int BufferSize, const char * Buffer, void * CallbackData)
{
  TLibS3GetObjectDataCallbackData &Data = *static_cast<TLibS3GetObjectDataCallbackData *>(CallbackData);

  return Data.FileSystem->GetObjectData(BufferSize, Buffer, Data);
}
//---------------------------------------------------------------------------
S3Status TS3FileSystem::GetObjectData(int BufferSize, const char * Buffer, TLibS3GetObjectDataCallbackData & Data)
{
  S3Status Result = S3StatusOK;

  if (ShouldCancelTransfer(Data))
  {
    Result = S3StatusAbortedByCallback;
  }
  else
  {
    TFileOperationProgressType * OperationProgress = Data.OperationProgress;
    try
    {
      FileOperationLoopCustom(FTerminal, OperationProgress, folAllowSkip,
        FMTLOAD(WRITE_ERROR, Data.FileName), "",
      [&]()
      {
        Data.Stream->Write(Buffer, BufferSize);
      });
      __removed FILE_OPERATION_LOOP_END(FMTLOAD(WRITE_ERROR, (Data.FileName)));

      OperationProgress->AddTransferred(BufferSize);
    }
    catch (Exception &E)
    {
      Data.Exception.reset(CloneException(&E));
      Result = S3StatusAbortedByCallback;
    }
  }

  return Result;
}
//---------------------------------------------------------------------------
void TS3FileSystem::Sink(
  const UnicodeString AFileName, const TRemoteFile *AFile,
  const UnicodeString ATargetDir, UnicodeString &ADestFileName, uintptr_t Attrs,
  const TCopyParamType *CopyParam, intptr_t Params, TFileOperationProgressType *OperationProgress,
  uintptr_t /*AFlags*/, TDownloadSessionAction &Action)
{
  UnicodeString DestFullName = ATargetDir + ADestFileName;
  if (::SysUtulsFileExists(ApiPath(DestFullName)))
  {
    int64_t Size;
    int64_t MTime;
    FTerminal->TerminalOpenLocalFile(DestFullName, GENERIC_READ, nullptr, nullptr, nullptr, &MTime, nullptr, &Size);
    TOverwriteFileParams FileParams;

    FileParams.SourceSize = AFile->Size;
    FileParams.SourceTimestamp = AFile->Modification; // noop
    FileParams.DestSize = Size;
    FileParams.DestTimestamp = UnixToDateTime(MTime, FTerminal->GetSessionData()->GetDSTMode());

    ConfirmOverwrite(AFileName, ADestFileName, OperationProgress, &FileParams, CopyParam, Params);
  }

  UnicodeString BucketName, Key;
  ParsePath(AFileName, BucketName, Key);

  TLibS3BucketContext BucketContext = GetBucketContext(BucketName);

  UnicodeString ExpandedDestFullName = ExpandUNCFileName(DestFullName);
  Action.Destination(ExpandedDestFullName);

  FileOperationLoopCustom(FTerminal, OperationProgress, folAllowSkip,
    FMTLOAD(TRANSFER_ERROR, AFileName), "",
  [&]()
  {
    HANDLE LocalFileHandle;
    if (!FTerminal->TerminalCreateLocalFile(DestFullName, OperationProgress, FLAGSET(Params, cpResume), FLAGSET(Params, cpNoConfirmation), &LocalFileHandle))
    {
      throw ESkipFile();
    }

    std::unique_ptr<TStream> Stream(new TSafeHandleStream(reinterpret_cast<THandle>(LocalFileHandle)));

    bool DeleteLocalFile = true;

    try__finally
    {
      SCOPE_EXIT
      {
        SAFE_CLOSE_HANDLE(LocalFileHandle);

        if (DeleteLocalFile)
        {
          FileOperationLoopCustom(FTerminal, OperationProgress, folAllowSkip,
            FMTLOAD(CORE_DELETE_LOCAL_FILE_ERROR, DestFullName), "",
          [&]()
          {
            THROWOSIFFALSE(::SysUtulsRemoveFile(ApiPath(DestFullName)));
          });
          __removed FILE_OPERATION_LOOP_END(FMTLOAD(DELETE_LOCAL_FILE_ERROR, (DestFullName)));
        }
      };
      TLibS3GetObjectDataCallbackData Data;

      FileOperationLoopCustom(FTerminal, OperationProgress, folAllowSkip,
        FMTLOAD(TRANSFER_ERROR, AFileName), "",
      [&]()
      {
        RequestInit(Data);
        Data.FileName = AFileName;
        Data.Stream = Stream.get();
        Data.OperationProgress = OperationProgress;
        Data.Exception.reset(nullptr);

        volatile TAutoFlag ResponseIgnoreSwitch(FResponseIgnore);
        S3GetObjectHandler GetObjectHandler = { CreateResponseHandler(), LibS3GetObjectDataCallback };
        S3_get_object(
          &BucketContext, StrToS3(Key), nullptr, Stream->Position, 0, FRequestContext, FTimeout, &GetObjectHandler, &Data);

        // The "exception" was already seen by the user, its presence mean an accepted abort of the operation.
        if (Data.Exception.get() == nullptr)
        {
          CheckLibS3Error(Data, true);
        }
      });
      __removed FILE_OPERATION_LOOP_END_EX(FMTLOAD(TRANSFER_ERROR, (AFileName)), (folAllowSkip | folRetryOnFatal));

      if (Data.Exception.get() != nullptr)
      {
        RethrowException(Data.Exception.get());
      }

      DeleteLocalFile = false;

      if (CopyParam->PreserveTime)
      {
        FTerminal->UpdateTargetTime(LocalFileHandle, AFile->Modification, FTerminal->SessionData->GetDSTMode());
      }
    }
    __finally__removed
    ({
      CloseHandle(LocalFileHandle);

      if (DeleteLocalFile)
      {
        FILE_OPERATION_LOOP_BEGIN
        {
          THROWOSIFFALSE(Sysutils::DeleteFile(ApiPath(DestFullName)));
        }
        FILE_OPERATION_LOOP_END(FMTLOAD(DELETE_LOCAL_FILE_ERROR, (DestFullName)));
      }
    })
  });
  __removed FILE_OPERATION_LOOP_END(FMTLOAD(TRANSFER_ERROR, (FileName)));

  FTerminal->UpdateTargetAttrs(DestFullName, AFile, CopyParam, Attrs);
}
//---------------------------------------------------------------------------
void TS3FileSystem::GetSupportedChecksumAlgs(TStrings * /*Algs*/)
{
  // NOOP
}
//---------------------------------------------------------------------------
void TS3FileSystem::LockFile(const UnicodeString /*AFileName*/, const TRemoteFile * /*AFile*/)
{
  DebugFail();
}
//---------------------------------------------------------------------------
void TS3FileSystem::UnlockFile(const UnicodeString /*AFileName*/, const TRemoteFile * /*AFile*/)
{
  DebugFail();
}
//---------------------------------------------------------------------------
void TS3FileSystem::UpdateFromMain(TCustomFileSystem * /*AMainFileSystem*/)
{
  // noop
}
//------------------------------------------------------------------------------
void TS3FileSystem::ClearCaches()
{
  FRegions.clear();
  FHostNames.clear();
}
//------------------------------------------------------------------------------
