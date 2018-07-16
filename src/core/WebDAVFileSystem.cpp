
#include <vcl.h>
#pragma hdrstop

#include <stdio.h>
#include <io.h>
#include <fcntl.h>

#ifndef NE_LFS
#define NE_LFS
#endif
#ifndef WINSCP
#define WINSCP
#endif
#include <ne_basic.h>
#include <ne_auth.h>
#include <ne_props.h>
#include <ne_uri.h>
#include <ne_session.h>
#include <ne_request.h>
#include <ne_xml.h>
#include <ne_xmlreq.h>
#include <ne_locks.h>
#include <expat.h>

#include <StrUtils.hpp>
#include <NeonIntf.h>

#include "WebDAVFileSystem.h"

#include "Interface.h"
#include "Common.h"
#include "Exceptions.h"
#include "Terminal.h"
#include "TextsCore.h"
#include "SecureShell.h"
#include "HelpCore.h"
#include "CoreMain.h"
#include "Security.h"
#include <openssl/ssl.h>

#if 0
#pragma package(smart_init)

#define FILE_OPERATION_LOOP_TERMINAL FTerminal

const int tfFirstLevel = 0x01;

struct TSinkFileParams
{
  UnicodeString TargetDir;
  const TCopyParamType *CopyParam;
  int Params;
  TFileOperationProgressType *OperationProgress;
  bool Skipped;
  unsigned int Flags;
};
#endif // #if 0

struct TWebDAVCertificateData
{
  UnicodeString Subject;
  UnicodeString Issuer;

  TDateTime ValidFrom;
  TDateTime ValidUntil;

  UnicodeString Fingerprint;
  AnsiString AsciiCert;

  int Failures;
};

#define SESSION_FS_KEY "filesystem"
static const char HttpsCertificateStorageKey[] = "HttpsCertificates";
static const UnicodeString CONST_WEBDAV_PROTOCOL_BASE_NAME = L"WebDAV";
static const int HttpUnauthorized = 401;

#define DAV_PROP_NAMESPACE "DAV:"
#define MODDAV_PROP_NAMESPACE "http://apache.org/dav/props/"
#define PROP_CONTENT_LENGTH "getcontentlength"
#define PROP_LAST_MODIFIED "getlastmodified"
#define PROP_CREATIONDATE "creationdate"
#define PROP_RESOURCE_TYPE "resourcetype"
#define PROP_HIDDEN "ishidden"
#define PROP_QUOTA_AVAILABLE "quota-available-bytes"
#define PROP_QUOTA_USED "quota-used-bytes"
#define PROP_EXECUTABLE "executable"
#define PROP_OWNER "owner"
#define PROP_DISPLAY_NAME "displayname"

static std::unique_ptr<TCriticalSection> DebugSection(TraceInitPtr(new TCriticalSection));

#if 0
static rde::set<TWebDAVFileSystem *> FileSystems;
#endif // #if 0

extern "C" {

void ne_debug(void * Context, int Channel, const char * Format, ...)
{
  bool DoLog;

  if (FLAGSET(Channel, NE_DBG_SOCKET) ||
    FLAGSET(Channel, NE_DBG_HTTP) ||
    FLAGSET(Channel, NE_DBG_HTTPAUTH) ||
    FLAGSET(Channel, NE_DBG_SSL))
  {
    DoLog = true;
  }
  else if (FLAGSET(Channel, NE_DBG_XML) ||
    FLAGSET(Channel, NE_DBG_WINSCP_HTTP_DETAIL))
  {
    DoLog = (GetConfiguration()->GetActualLogProtocol() >= 1);
  }
  else if (FLAGSET(Channel, NE_DBG_LOCKS) ||
    FLAGSET(Channel, NE_DBG_XMLPARSE) ||
    FLAGSET(Channel, NE_DBG_HTTPBODY))
  {
    DoLog = (GetConfiguration()->GetActualLogProtocol() >= 2);
  }
  else
  {
    DoLog = false;
    DebugFail();
  }

#ifndef _DEBUG
  if (DoLog)
#endif
  {
    va_list Args;
    va_start(Args, Format);
    UTF8String UTFMessage;
    UTFMessage.vprintf(Format, Args);
    va_end(Args);

    UnicodeString Message(TrimRight(UnicodeString(UTFMessage)));

    if (DoLog)
    {
      // Note that this gets called for THttp sessions too.
      // It does no harm atm.
      TWebDAVFileSystem * FileSystem = nullptr;
      if (Context != nullptr)
      {
        ne_session * Session = static_cast<ne_session *>(Context);

        FileSystem =
          static_cast<TWebDAVFileSystem *>(ne_get_session_private(Session, SESSION_FS_KEY));
      }
      else
      {
        TGuard Guard(*DebugSection.get());

        TODO("implement");
#if 0
      if (FileSystems.size() == 1)
      {
        FileSystem = *FileSystems.begin();
      }
#endif // #if 0
      }

      if (FileSystem != nullptr)
      {
        FileSystem->NeonDebug(Message);
      }
    }
  }
}

} // extern "C"


// ne_path_escape returns 7-bit string, so it does not really matter if we use
// AnsiString or UTF8String here, though UTF8String might be more safe
static AnsiString PathEscape(const char *Path)
{
  char *EscapedPath = ne_path_escape(Path);
  AnsiString Result = EscapedPath;
  ne_free(EscapedPath);
  return Result;
}

static UTF8String PathUnescape(const char *Path)
{
  char *UnescapedPath = ne_path_unescape(NullToEmptyA(Path));
  UTF8String Result(UnescapedPath, NBChTraitsCRT<char>::SafeStringLen(UnescapedPath));
  ne_free(UnescapedPath);
  return Result;
}

#define AbsolutePathToNeon(P) PathEscape(StrToNeon(P)).c_str()
#define PathToNeonStatic(THIS, P) AbsolutePathToNeon((THIS)->GetAbsolutePath(P, false))
#define PathToNeon(P) PathToNeonStatic(this, P)


static bool NeonInitialized = false;
static bool NeonSspiInitialized = false;

void NeonInitialize()
{
  // Even if this fails, we do not want to interrupt WinSCP starting for that.
  // Anyway, it can hardly fail.
  // Though it fails on Wine on Debian VM, because of ne_sspi_init():
  // sspi: QuerySecurityPackageInfo [failed] [80090305].
  // sspi: Unable to get negotiate maximum packet size
  int NeonResult = ne_sock_init();
  if (NeonResult == 0)
  {
    NeonInitialized = true;
    NeonSspiInitialized = true;
  }
  else if (NeonResult == -2)
  {
    NeonInitialized = true;
    NeonSspiInitialized = false;
  }
  else
  {
    NeonInitialized = false;
    NeonSspiInitialized = false;
  }
}

void NeonFinalize()
{
  if (NeonInitialized)
  {
    ne_sock_exit();
    NeonInitialized = false;
  }
}

UnicodeString NeonVersion()
{
  UnicodeString Str = StrFromNeon(ne_version_string());
  CutToChar(Str, L' ', true); // "neon"
  UnicodeString Result = CutToChar(Str, L':', true);
  return Result;
}

UnicodeString ExpatVersion()
{
  return FORMAT("%d.%d.%d", XML_MAJOR_VERSION, XML_MINOR_VERSION, XML_MICRO_VERSION);
}


TWebDAVFileSystem::TWebDAVFileSystem(TTerminal *ATerminal) :
  TCustomFileSystem(OBJECT_CLASS_TWebDAVFileSystem, ATerminal),
  FActive(false),
  FHasTrailingSlash(false),
  FSkipped(false),
  FCancelled(false),
  FStoredPasswordTried(false),
  FUploading(false),
  FDownloading(false),
  FNeonSession(nullptr),
  FNeonLockStore(nullptr),
  FInitialHandshake(false),
  FAuthenticationRequested(false),
  FCapabilities(0),
  FPortNumber(0),
  FIgnoreAuthenticationFailure(iafNo),
  FAuthenticationRetry(false),
  FNtlmAuthenticationFailed(false)
{
}

void TWebDAVFileSystem::Init(void *)
{
  FFileSystemInfo.ProtocolBaseName = CONST_WEBDAV_PROTOCOL_BASE_NAME;
  FFileSystemInfo.ProtocolName = FFileSystemInfo.ProtocolBaseName;
}

void TWebDAVFileSystem::FileTransferProgress(int64_t /*TransferSize*/, int64_t /*Bytes*/)
{
  TODO("implement");
}

TWebDAVFileSystem::~TWebDAVFileSystem()
{
  UnregisterFromDebug();

  {
    TGuard Guard(FNeonLockStoreSection);
    if (FNeonLockStore != nullptr)
    {
      ne_lockstore_destroy(FNeonLockStore);
      FNeonLockStore = nullptr;
    }
  }

#if 0
  delete FNeonLockStoreSection;
#endif // #if 0
}

void TWebDAVFileSystem::Open()
{
  if (!NeonInitialized)
  {
    throw Exception(LoadStr(NEON_INIT_FAILED));
  }

  if (!NeonSspiInitialized)
  {
    FTerminal->LogEvent(L"Warning: SSPI initialization failed.");
  }

  RegisterForDebug();

  FCurrentDirectory.Clear();
  FHasTrailingSlash = false;
  FStoredPasswordTried = false;
  FTlsVersionStr.Clear();
  FCapabilities = 0;

  TSessionData *Data = FTerminal->GetSessionData();

  FSessionInfo.LoginTime = Now();

  UnicodeString HostName = Data->GetHostNameExpanded();
  size_t Port = Data->GetPortNumber();
  UnicodeString ProtocolName = (GetFtps() == ftpsNone) ? HttpProtocol : HttpsProtocol;
  UnicodeString Path = Data->GetRemoteDirectory();
  // PathToNeon is not used as we cannot call AbsolutePath here
  UnicodeString EscapedPath = StrFromNeon(PathEscape(StrToNeon(Path)).c_str());
  UnicodeString Url = FORMAT("%s://%s:%d%s", ProtocolName, HostName, Port, EscapedPath);

  FTerminal->Information(LoadStr(STATUS_CONNECT), true);
  FActive = false;
  try
  {
    OpenUrl(Url);
  }
  catch (Exception &E)
  {
    CloseNeonSession();
    FTerminal->Closed();
    FTerminal->FatalError(&E, LoadStr(CONNECTION_FAILED));
  }
  FActive = true;
}

UnicodeString TWebDAVFileSystem::ParsePathFromUrl(UnicodeString Url) const
{
  UnicodeString Result;
  ne_uri ParsedUri;
  if (ne_uri_parse(StrToNeon(Url), &ParsedUri) == 0)
  {
    Result = StrFromNeon(PathUnescape(ParsedUri.path));
    ne_uri_free(&ParsedUri);
  }
  return Result;
}

void TWebDAVFileSystem::OpenUrl(UnicodeString Url)
{
  UnicodeString CorrectedUrl;
  NeonClientOpenSessionInternal(CorrectedUrl, Url);

  if (CorrectedUrl.IsEmpty())
  {
    CorrectedUrl = Url;
  }
  UnicodeString ParsedPath = ParsePathFromUrl(CorrectedUrl);
  if (!ParsedPath.IsEmpty())
  {
    // this is most likely pointless as it get overwritten by
    // call to ChangeDirectory() from TTerminal::DoStartup
    FCurrentDirectory = ParsedPath;
  }
}

void TWebDAVFileSystem::NeonClientOpenSessionInternal(UnicodeString &CorrectedUrl, UnicodeString Url)
{
  std::unique_ptr<TStringList> AttemptedUrls(CreateSortedStringList());
  AttemptedUrls->Add(Url);
  while (true)
  {
    CorrectedUrl = L"";
    NeonOpen(CorrectedUrl, Url);
    // No error and no corrected URL?  We're done here.
    if (CorrectedUrl.IsEmpty())
    {
      break;
    }
    CloseNeonSession();
    CheckRedirectLoop(CorrectedUrl, AttemptedUrls.get());
    // Our caller will want to know what our final corrected URL was.
    Url = CorrectedUrl;
  }

  CorrectedUrl = Url;
}

void TWebDAVFileSystem::SetSessionTls(ne_session_s *Session, bool Aux)
{
  SetNeonTlsInit(Session, InitSslSession);

  // When the CA certificate or server certificate has
  // verification problems, neon will call our verify function before
  // outright rejection of the connection.
  ne_ssl_verify_fn Callback = Aux ? NeonServerSSLCallbackAux : NeonServerSSLCallbackMain;
  ne_ssl_set_verify(Session, Callback, this);

  ne_ssl_trust_default_ca(Session);
}

void TWebDAVFileSystem::InitSession(ne_session_s *Session)
{
  TSessionData *Data = FTerminal->GetSessionData();

  InitNeonSession(
    Session, Data->GetProxyMethod(), Data->GetProxyHost(), Data->GetProxyPort(),
    Data->GetProxyUsername(), Data->GetProxyPassword());

  ne_set_read_timeout(FNeonSession, ToInt(Data->GetTimeout()));

  ne_set_connect_timeout(FNeonSession, ToInt(Data->GetTimeout()));

  ne_set_session_private(Session, SESSION_FS_KEY, this);
}

void TWebDAVFileSystem::NeonOpen(UnicodeString &CorrectedUrl, UnicodeString Url)
{
  ne_uri uri;
  NeonParseUrl(Url, uri);

  FHostName = StrFromNeon(uri.host);
  FPortNumber = uri.port;

  FSessionInfo.CSCipher = UnicodeString();
  FSessionInfo.SCCipher = UnicodeString();
  bool Ssl = IsTlsUri(uri);
  FSessionInfo.SecurityProtocolName = Ssl ? LoadStr(FTPS_IMPLICIT) : UnicodeString();

  if (Ssl != (GetFtps() != ftpsNone))
  {
    FTerminal->LogEvent(FORMAT("Warning: %s", LoadStr(UNENCRYPTED_REDIRECT)));
  }

  DebugAssert(FNeonSession == nullptr);
  FNeonSession = CreateNeonSession(uri);
  InitSession(FNeonSession);

  UTF8String Path(uri.path);
  ne_uri_free(&uri);
  ne_set_aux_request_init(FNeonSession, NeonAuxRequestInit, this);

  // Other flags:
  // NE_DBG_FLUSH - used only in native implementation of ne_debug
  // NE_DBG_HTTPPLAIN - log credentials in HTTP authentication

  ne_debug_mask =
    NE_DBG_SOCKET |
    NE_DBG_HTTP |
    NE_DBG_XML | // detail
    NE_DBG_HTTPAUTH |
    NE_DBG_LOCKS | // very details
    NE_DBG_XMLPARSE | // very details
    NE_DBG_HTTPBODY | // very details
    NE_DBG_SSL |
    FLAGMASK(GetConfiguration()->GetLogSensitive(), NE_DBG_HTTPPLAIN);

  NeonAddAuthentication(Ssl);

  if (Ssl)
  {
    SetSessionTls(FNeonSession, false);

    ne_ssl_provide_clicert(FNeonSession, NeonProvideClientCert, this);
  }

  ne_set_notifier(FNeonSession, NeonNotifier, this);
  ne_hook_create_request(FNeonSession, NeonCreateRequest, this);
  ne_hook_pre_send(FNeonSession, NeonPreSend, this);
  ne_hook_post_send(FNeonSession, NeonPostSend, this);
  ne_hook_post_headers(FNeonSession, NeonPostHeaders, this);

  TAutoFlag Flag(FInitialHandshake);
  ExchangeCapabilities(Path.c_str(), CorrectedUrl);
}

void TWebDAVFileSystem::NeonAuxRequestInit(ne_session *Session, ne_request * /*Request*/, void *UserData)
{
  TWebDAVFileSystem *FileSystem = static_cast<TWebDAVFileSystem *>(UserData);
  FileSystem->InitSession(Session);

  ne_uri uri = {nullptr};
  ne_fill_server_uri(Session, &uri);
  bool Tls = IsTlsUri(uri);
  ne_uri_free(&uri);

  if (Tls)
  {
    FileSystem->SetSessionTls(Session, true);
  }
}

void TWebDAVFileSystem::NeonAddAuthentication(bool UseNegotiate)
{
  unsigned int NeonAuthTypes = NE_AUTH_BASIC | NE_AUTH_DIGEST | NE_AUTH_PASSPORT;
  if (UseNegotiate)
  {
    NeonAuthTypes |= NE_AUTH_NEGOTIATE;
  }
  ne_add_server_auth(FNeonSession, NeonAuthTypes, NeonRequestAuth, this);
}

UnicodeString TWebDAVFileSystem::GetRedirectUrl() const
{
  UnicodeString Result = GetNeonRedirectUrl(FNeonSession);
  FTerminal->LogEvent(FORMAT("Redirected to \"%s\".", Result));
  return Result;
}

void TWebDAVFileSystem::ExchangeCapabilities(const char *APath, UnicodeString &CorrectedUrl)
{
  ClearNeonError();

  int NeonStatus;
  FAuthenticationRetry = false;
  do
  {
    NeonStatus = ne_options2(FNeonSession, APath, &FCapabilities);
  }
  while ((NeonStatus == NE_AUTH) && FAuthenticationRetry);

  if (NeonStatus == NE_REDIRECT)
  {
    CorrectedUrl = GetRedirectUrl();
  }
  else if (NeonStatus == NE_OK)
  {
    if (FCapabilities > 0)
    {
      UnicodeString Str;
      uint32_t Capability = 0x01;
      uintptr_t Capabilities = FCapabilities;
      while (Capabilities > 0)
      {
        if (FLAGSET(Capabilities, Capability))
        {
          AddToList(Str, StrFromNeon(ne_capability_name(Capability)), L", ");
          Capabilities -= Capability;
        }
        Capability <<= 1;
      }
      FTerminal->LogEvent(FORMAT("Server capabilities: %s", Str));
      FFileSystemInfo.AdditionalInfo +=
        LoadStr(WEBDAV_EXTENSION_INFO) + sLineBreak +
        L"  " + Str + sLineBreak;
    }
  }
  else
  {
    CheckStatus(NeonStatus);
  }

  FTerminal->SaveCapabilities(FFileSystemInfo);
}

void TWebDAVFileSystem::CloseNeonSession()
{
  if (FNeonSession != nullptr)
  {
    DestroyNeonSession(FNeonSession);
    FNeonSession = nullptr;
  }
}

void TWebDAVFileSystem::Close()
{
  DebugAssert(FActive);
  CloseNeonSession();
  FTerminal->Closed();
  FActive = false;
  UnregisterFromDebug();
}

void TWebDAVFileSystem::RegisterForDebug()
{
  TGuard Guard(*DebugSection.get());
//  FileSystems.insert(this);
}

void TWebDAVFileSystem::UnregisterFromDebug()
{
  TGuard Guard(*DebugSection.get());
//  FileSystems.erase(this);
}

bool TWebDAVFileSystem::GetActive() const
{
  return FActive;
}

void TWebDAVFileSystem::CollectUsage()
{
  if (!FTlsVersionStr.IsEmpty())
  {
    FTerminal->CollectTlsUsage(FTlsVersionStr);
  }

  if (!FTerminal->GetSessionData()->GetTlsCertificateFile().IsEmpty())
  {
//    GetConfiguration()->GetUsage()->Inc(L"OpenedSessionsWebDAVSCertificate");
  }

  // The Authorization header for passport method is included only in the first request,
  // so we have to use FLastAuthorizationProtocol
  if (SameText(FLastAuthorizationProtocol, L"Passport1.4"))
  {
//    Configuration->Usage->Inc(L"OpenedSessionsWebDAVSPassport");
  }

  UnicodeString RemoteSystem = FFileSystemInfo.RemoteSystem;
  if (ContainsText(RemoteSystem, L"Microsoft-IIS"))
  {
//    FTerminal->GetConfiguration()->GetUsage()->Inc(L"OpenedSessionsWebDAVIIS");
  }
  else if (ContainsText(RemoteSystem, L"IT Hit WebDAV Server"))
  {
//    FTerminal->GetConfiguration()->GetUsage()->Inc(L"OpenedSessionsWebDAVITHit");
  }
  // e.g. brickftp.com
  else if (ContainsText(RemoteSystem, L"nginx"))
  {
//    FTerminal->GetConfiguration()->GetUsage()->Inc(L"OpenedSessionsWebDAVNginx");
  }
  else
  {
    // We also know OpenDrive, Yandex, iFiles (iOS), Swapper (iOS), SafeSync
//    FTerminal->GetConfiguration()->GetUsage()->Inc(L"OpenedSessionsWebDAVOther");
  }
}

const TSessionInfo &TWebDAVFileSystem::GetSessionInfo() const
{
  return FSessionInfo;
}

const TFileSystemInfo &TWebDAVFileSystem::GetFileSystemInfo(bool /*Retrieve*/)
{
  return FFileSystemInfo;
}

bool TWebDAVFileSystem::TemporaryTransferFile(UnicodeString /*AFileName*/)
{
  return false;
}

bool TWebDAVFileSystem::GetStoredCredentialsTried() const
{
  return FStoredPasswordTried;
}

UnicodeString TWebDAVFileSystem::RemoteGetUserName() const
{
  return FUserName;
}

void TWebDAVFileSystem::Idle()
{
  TODO("Keep session alive");
  // noop
}

UnicodeString TWebDAVFileSystem::GetAbsolutePath(UnicodeString APath, bool /*Local*/) const
{
  bool AddTrailingBackslash;

  if (APath == L"/")
  {
    // does not really matter as path "/" is still "/" when absolute,
    // no slash needed
    AddTrailingBackslash = FHasTrailingSlash;
  }
  else
  {
    AddTrailingBackslash = (APath[APath.Length()] == L'/');
  }

  UnicodeString Result = base::AbsolutePath(RemoteGetCurrentDirectory(), APath);
  // We must preserve trailing slash, because particularly for mod_dav,
  // it really matters if the slash in there or not
  if (AddTrailingBackslash)
  {
    Result = base::UnixIncludeTrailingBackslash(Result);
  }

  return Result;
}

UnicodeString TWebDAVFileSystem::GetAbsolutePath(UnicodeString APath, bool Local)
{
  return static_cast<const TWebDAVFileSystem *>(this)->GetAbsolutePath(APath, Local);
}

bool TWebDAVFileSystem::IsCapable(intptr_t Capability) const
{
  DebugAssert(FTerminal);
  switch (Capability)
  {
  case fcRename:
  case fcRemoteMove:
  case fcMoveToQueue:
  case fcPreservingTimestampUpload:
  case fcCheckingSpaceAvailable:
  // Only to make double-click on file edit/open the file,
  // instead of trying to open it as directory
  case fcResolveSymlink:
  case fsSkipTransfer:
  case fsParallelTransfers:
  case fcRemoteCopy:
    return true;

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
    return false;

  case fcLocking:
    return FLAGSET(FCapabilities, NE_CAP_DAV_CLASS2);

  default:
    DebugFail();
    return false;
  }
}

UnicodeString TWebDAVFileSystem::RemoteGetCurrentDirectory() const
{
  return FCurrentDirectory;
}

void TWebDAVFileSystem::DoStartup()
{
  FTerminal->SetExceptionOnFail(true);
  // retrieve initialize working directory to save it as home directory
  ReadCurrentDirectory();
  FTerminal->SetExceptionOnFail(false);
}

void TWebDAVFileSystem::ClearNeonError()
{
  FCancelled = false;
  FSkipped = false;
  FAuthenticationRequested = false;
  ne_set_error(FNeonSession, "");
}

UnicodeString TWebDAVFileSystem::GetNeonError() const
{
  return ::GetNeonError(FNeonSession);
}

void TWebDAVFileSystem::CheckStatus(intptr_t NeonStatus)
{
  if ((NeonStatus == NE_ERROR) && (FCancelled || FSkipped))
  {
    if (FCancelled)
    {
      FCancelled = false;
      FSkipped = false; // just in case
      Abort();
    }
    else
    {
      DebugAssert(FSkipped);
      FSkipped = false;
      ThrowSkipFileNull();
    }
  }
  else
  {
    CheckNeonStatus(FNeonSession, NeonStatus, FHostName);
  }
}

void TWebDAVFileSystem::LookupUsersGroups()
{
  DebugFail();
}

void TWebDAVFileSystem::ReadCurrentDirectory()
{
  if (FCachedDirectoryChange.IsEmpty())
  {
    FCurrentDirectory = FCurrentDirectory.IsEmpty() ? UnicodeString(ROOTDIRECTORY) : FCurrentDirectory;
  }
  else
  {
    FCurrentDirectory = FCachedDirectoryChange;
    FCachedDirectoryChange = L"";
  }
}

void TWebDAVFileSystem::HomeDirectory()
{
  ChangeDirectory(L"/");
}

UnicodeString TWebDAVFileSystem::DirectoryPath(UnicodeString APath) const
{
  if (FHasTrailingSlash)
  {
    return base::UnixIncludeTrailingBackslash(APath);
  }
  return APath;
}

UnicodeString TWebDAVFileSystem::FilePath(const TRemoteFile *AFile) const
{
  UnicodeString Result = AFile->GetFullFileName();
  if (AFile->GetIsDirectory())
  {
    Result = DirectoryPath(Result);
  }
  return Result;
}

void TWebDAVFileSystem::TryOpenDirectory(UnicodeString ADirectory)
{
  ADirectory = DirectoryPath(ADirectory);
  FTerminal->LogEvent(FORMAT("Trying to open directory \"%s\".", ADirectory));
  TRemoteFile *File = nullptr;
  ReadFile(ADirectory, File);
  SAFE_DESTROY(File);
}

void TWebDAVFileSystem::AnnounceFileListOperation()
{
  // noop
}

void TWebDAVFileSystem::ChangeDirectory(UnicodeString ADirectory)
{
  UnicodeString Path = GetAbsolutePath(ADirectory, false);

  // to verify existence of directory try to open it
  TryOpenDirectory(Path);

  // if open dir did not fail, directory exists -> success.
  FCachedDirectoryChange = Path;
}

void TWebDAVFileSystem::CachedChangeDirectory(UnicodeString Directory)
{
  FCachedDirectoryChange = base::UnixExcludeTrailingBackslash(Directory);
}

struct TReadFileData
{
  CUSTOM_MEM_ALLOCATION_IMPL
  TWebDAVFileSystem *FileSystem;
  TRemoteFile *File;
  TRemoteFileList *FileList;
};

int TWebDAVFileSystem::ReadDirectoryInternal(
  UnicodeString APath, TRemoteFileList *AFileList)
{
  TReadFileData Data;
  Data.FileSystem = this;
  Data.File = nullptr;
  Data.FileList = AFileList;
  ClearNeonError();
  ne_propfind_handler *PropFindHandler = ne_propfind_create(FNeonSession, PathToNeon(APath), NE_DEPTH_ONE);
  void *DiscoveryContext = ne_lock_register_discovery(PropFindHandler);
  int Result;
  try__finally
  {
    SCOPE_EXIT
    {
      ne_lock_discovery_free(DiscoveryContext);
      ne_propfind_destroy(PropFindHandler);
    };
    Result = ne_propfind_allprop(PropFindHandler, NeonPropsResult, &Data);
  }
  __finally
  {
#if 0
    ne_lock_discovery_free(DiscoveryContext);
    ne_propfind_destroy(PropFindHandler);
#endif // #if 0
  };
  return Result;
}

bool TWebDAVFileSystem::IsValidRedirect(intptr_t NeonStatus, UnicodeString &APath) const
{
  bool Result = (NeonStatus == NE_REDIRECT);
  if (Result)
  {
    // What PathToNeon does
    UnicodeString OriginalPath = GetAbsolutePath(APath, false);
    // Handle one-step redirect
    // (for more steps we would have to implement loop detection).
    // This is mainly to handle "folder" => "folder/" redirects of Apache/mod_dav.
    UnicodeString RedirectUrl = GetRedirectUrl();
    // We should test if the redirect is not for another server,
    // though not sure how to do this reliably (domain aliases, IP vs. domain, etc.)
    UnicodeString RedirectPath = ParsePathFromUrl(RedirectUrl);
    Result =
      !RedirectPath.IsEmpty() &&
      (RedirectPath != OriginalPath);

    if (Result)
    {
      APath = RedirectPath;
    }
  }

  return Result;
}

void TWebDAVFileSystem::ReadDirectory(TRemoteFileList *AFileList)
{
  UnicodeString Path = DirectoryPath(AFileList->GetDirectory());
  TOperationVisualizer Visualizer(FTerminal->GetUseBusyCursor());

  int NeonStatus = ReadDirectoryInternal(Path, AFileList);
  if (IsValidRedirect(NeonStatus, Path))
  {
    NeonStatus = ReadDirectoryInternal(Path, AFileList);
  }
  CheckStatus(NeonStatus);
}

void TWebDAVFileSystem::ReadSymlink(TRemoteFile * /*SymlinkFile*/,
  TRemoteFile *& /*AFile*/)
{
  // we never set SymLink flag, so we should never get here
  DebugFail();
}

void TWebDAVFileSystem::ReadFile(UnicodeString AFileName,
  TRemoteFile *&AFile)
{
  CustomReadFile(AFileName, AFile, nullptr);
}

void TWebDAVFileSystem::NeonPropsResult(
  void *UserData, const ne_uri *Uri, const ne_prop_result_set *Results)
{
  UnicodeString Path = StrFromNeon(PathUnescape(Uri->path).c_str());

  TReadFileData &Data = *static_cast<TReadFileData *>(UserData);
  if (Data.FileList != nullptr)
  {
    UnicodeString FileListPath = Data.FileSystem->GetAbsolutePath(Data.FileList->GetDirectory(), false);
    if (base::UnixSamePath(Path, FileListPath))
    {
      Path = base::UnixIncludeTrailingBackslash(base::UnixIncludeTrailingBackslash(Path) + L"..");
    }
    std::unique_ptr<TRemoteFile> File(new TRemoteFile());
    File->SetTerminal(Data.FileSystem->FTerminal);
    Data.FileSystem->ParsePropResultSet(File.get(), Path, Results);
    Data.FileList->AddFile(File.release());
  }
  else
  {
    Data.FileSystem->ParsePropResultSet(Data.File, Path, Results);
  }
}

const char *TWebDAVFileSystem::GetNeonProp(
  const ne_prop_result_set *Results, const char *Name, const char *NameSpace)
{
  ne_propname Prop;
  Prop.nspace = (NameSpace == nullptr) ? DAV_PROP_NAMESPACE : NameSpace;
  Prop.name = Name;
  return ne_propset_value(Results, &Prop);
}

void TWebDAVFileSystem::ParsePropResultSet(TRemoteFile *AFile,
  UnicodeString APath, const ne_prop_result_set *Results)
{
  AFile->SetFullFileName(base::UnixExcludeTrailingBackslash(APath));
  // Some servers do not use DAV:collection tag, but indicate the folder by trailing slash only.
  // It seems that all servers actually use the trailing slash, including IIS, mod_Dav, IT Hit, OpenDrive, etc.
  bool Collection = (AFile->GetFullFileName() != APath);
  AFile->SetFileName(base::UnixExtractFileName(AFile->GetFullFileName()));
  const char *ContentLength = GetNeonProp(Results, PROP_CONTENT_LENGTH);
  // some servers, for example iFiles, do not provide "getcontentlength" for folders
  if (ContentLength != nullptr)
  {
    AFile->SetSize(StrToInt64Def(ContentLength, 0));
  }
  const char *LastModified = GetNeonProp(Results, PROP_LAST_MODIFIED);
  const char *CreationDate = GetNeonProp(Results, PROP_CREATIONDATE);
  const char *Modified = LastModified ? LastModified : CreationDate;
  // We've seen a server (t=24891) that does not set "getlastmodified" for the "this" folder entry.
  if (DebugAlwaysTrue(Modified != nullptr))
  {
    char WeekDay[4] = {L'\0'};
    int Year = 0;
    char MonthStr[4] = {L'\0'};
    int Day = 0;
    int Hour = 0;
    int Min = 0;
    int Sec = 0;
#define RFC1123_FORMAT "%3s, %02d %3s %4d %02d:%02d:%02d GMT"
    int Filled =
      sscanf(Modified, RFC1123_FORMAT, WeekDay, &Day, MonthStr, &Year, &Hour, &Min, &Sec);
    // we need at least a complete date
    if (Filled >= 4)
    {
      intptr_t Month = ParseShortEngMonthName(MonthStr);
      if (Month >= 1)
      {
        TDateTime Modification =
          EncodeDateVerbose(static_cast<unsigned short>(Year), static_cast<unsigned short>(Month), static_cast<unsigned short>(Day)) +
          EncodeTimeVerbose(static_cast<unsigned short>(Hour), static_cast<unsigned short>(Min), static_cast<unsigned short>(Sec), 0);
        AFile->SetModification(ConvertTimestampFromUTC(Modification));
        AFile->SetModificationFmt(mfFull);
      }
    }
  }

  // optimization
  if (!Collection)
  {
    // This is possibly redundant code as all servers we know (see a comment above)
    // indicate the folder by trailing slash too
    const char *ResourceType = GetNeonProp(Results, PROP_RESOURCE_TYPE);
    if (ResourceType != nullptr)
    {
      // property has XML value
      UnicodeString AResourceType = ResourceType;
      // this is very poor parsing
      if (ContainsText(ResourceType, L"<DAV:collection"))
      {
        Collection = true;
      }
    }
  }

  AFile->SetType(Collection ? FILETYPE_DIRECTORY : FILETYPE_DEFAULT);
  // this is MS extension (draft-hopmann-collection-props-00)
  const char *IsHidden = GetNeonProp(Results, PROP_HIDDEN);
  if (IsHidden != nullptr)
  {
    AFile->SetIsHidden(StrToIntDef(IsHidden, 0) != 0);
  }

  const char *Owner = GetNeonProp(Results, PROP_OWNER);
  if (Owner != nullptr)
  {
    AFile->GetFileOwner().SetName(Owner);
  }

  const char *DisplayName = GetNeonProp(Results, PROP_DISPLAY_NAME);
  if (DisplayName != nullptr)
  {
    AFile->SetDisplayName(StrFromNeon(DisplayName));
  }

  const UnicodeString RightsDelimiter(L", ");
  UnicodeString HumanRights;

  // Proprietary property of mod_dav
  // http://www.webdav.org/mod_dav/#imp
  const char *Executable = GetNeonProp(Results, PROP_EXECUTABLE, MODDAV_PROP_NAMESPACE);
  if (Executable != nullptr)
  {
    if (strcmp(Executable, "T") == 0)
    {
      UnicodeString ExecutableRights;
      // The "gear" character is supported since Windows 8
      if (IsWin8())
      {
        ExecutableRights = L"\u2699";
      }
      else
      {
        ExecutableRights = LoadStr(EXECUTABLE);
      }
      AddToList(HumanRights, ExecutableRights, RightsDelimiter);
    }
  }

  struct ne_lock *Lock = static_cast<struct ne_lock *>(ne_propset_private(Results));
  if ((Lock != nullptr) && (Lock->token != nullptr))
  {
    UnicodeString Owner2;
    if (Lock->owner != nullptr)
    {
      Owner2 = StrFromNeon(Lock->owner).Trim();
    }
    UnicodeString LockRights;
    if (IsWin8())
    {
      // The "lock" character is supported since Windows 8
//      LockRights = L"\uD83D\uDD12" + Owner2;
    }
    else
    {
      LockRights = LoadStr(LOCKED);
      if (!Owner2.IsEmpty())
      {
        LockRights = FORMAT("%s (%s)", LockRights, Owner2);
      }
    }

    AddToList(HumanRights, LockRights, RightsDelimiter);
  }

  AFile->SetHumanRights(HumanRights);
}

intptr_t TWebDAVFileSystem::CustomReadFileInternal(UnicodeString AFileName,
  TRemoteFile *&AFile, TRemoteFile *ALinkedByFile)
{
  std::unique_ptr<TRemoteFile> File(new TRemoteFile(ALinkedByFile));
  TReadFileData Data;
  Data.FileSystem = this;
  Data.File = File.get();
  Data.FileList = nullptr;
  ClearNeonError();
  int Result =
    ne_simple_propfind(FNeonSession, PathToNeon(AFileName), NE_DEPTH_ZERO, nullptr,
      NeonPropsResult, &Data);
  if (Result == NE_OK)
  {
    AFile = File.release();
  }
  return static_cast<intptr_t>(Result);
}

void TWebDAVFileSystem::CustomReadFile(UnicodeString AFileName,
  TRemoteFile *&AFile, TRemoteFile *ALinkedByFile)
{
  UnicodeString FileName = AFileName;
  TOperationVisualizer Visualizer(FTerminal->GetUseBusyCursor());

  intptr_t NeonStatus = CustomReadFileInternal(AFileName, AFile, ALinkedByFile);
  if (IsValidRedirect(NeonStatus, FileName))
  {
    NeonStatus = CustomReadFileInternal(FileName, AFile, ALinkedByFile);
  }
  CheckStatus(NeonStatus);
}

void TWebDAVFileSystem::RemoteDeleteFile(UnicodeString /*AFileName*/,
  const TRemoteFile *AFile, intptr_t /*Params*/, TRmSessionAction &Action)
{
  Action.Recursive();
  ClearNeonError();
  TOperationVisualizer Visualizer(FTerminal->GetUseBusyCursor());
  RawByteString Path = PathToNeon(FilePath(AFile));
  // WebDAV does not allow non-recursive delete:
  // RFC 4918, section 9.6.1:
  // "A client MUST NOT submit a Depth header with a DELETE on a collection with any value but infinity."
  // We should check that folder is empty when called with FLAGSET(Params, dfNoRecursive)
  CheckStatus(ne_delete(FNeonSession, Path.c_str()));
  // The lock is removed with the file, but if a file with the same name gets created,
  // we would try to use obsoleted lock token with it, what the server would reject
  // (mod_dav returns "412 Precondition Failed")
  DiscardLock(Path);
}

int TWebDAVFileSystem::RenameFileInternal(UnicodeString AFileName,
  UnicodeString ANewName)
{
  // 0 = no overwrite
  return ne_move(FNeonSession, 0, PathToNeon(AFileName), PathToNeon(ANewName));
}

void TWebDAVFileSystem::RemoteRenameFile(UnicodeString AFileName,
  UnicodeString ANewName)
{
  ClearNeonError();
  TOperationVisualizer Visualizer(FTerminal->GetUseBusyCursor());

  UnicodeString Path = AFileName;
  int NeonStatus = RenameFileInternal(Path, ANewName);
  if (IsValidRedirect(NeonStatus, Path))
  {
    NeonStatus = RenameFileInternal(Path, ANewName);
  }
  CheckStatus(NeonStatus);
  // See a comment in DeleteFile
  DiscardLock(PathToNeon(Path));
}

int TWebDAVFileSystem::CopyFileInternal(UnicodeString AFileName,
  UnicodeString ANewName)
{
  // 0 = no overwrite
  return ne_copy(FNeonSession, 0, NE_DEPTH_INFINITE, PathToNeon(AFileName), PathToNeon(ANewName));
}

void TWebDAVFileSystem::RemoteCopyFile(UnicodeString AFileName,
  UnicodeString ANewName)
{
  ClearNeonError();
  TOperationVisualizer Visualizer(FTerminal->GetUseBusyCursor());

  UnicodeString Path = AFileName;
  int NeonStatus = CopyFileInternal(Path, ANewName);
  if (IsValidRedirect(NeonStatus, Path))
  {
    NeonStatus = CopyFileInternal(Path, ANewName);
  }
  CheckStatus(NeonStatus);
}

void TWebDAVFileSystem::RemoteCreateDirectory(UnicodeString ADirName)
{
  ClearNeonError();
  TOperationVisualizer Visualizer(FTerminal->GetUseBusyCursor());
  CheckStatus(ne_mkcol(FNeonSession, PathToNeon(ADirName)));
}

void TWebDAVFileSystem::CreateLink(UnicodeString /*AFileName*/,
  UnicodeString /*PointTo*/, bool /*Symbolic*/)
{
  DebugFail();
  // ThrowNotImplemented(1014);
}

void TWebDAVFileSystem::ChangeFileProperties(UnicodeString /*AFileName*/,
  const TRemoteFile * /*AFile*/, const TRemoteProperties * /*Properties*/,
  TChmodSessionAction & /*Action*/)
{
  DebugFail();
  // ThrowNotImplemented(1006);
}

bool TWebDAVFileSystem::LoadFilesProperties(TStrings * /*FileList*/)
{
  DebugFail();
  return false;
}

void TWebDAVFileSystem::CalculateFilesChecksum(UnicodeString /*Alg*/,
  TStrings * /*FileList*/, TStrings * /*Checksums*/,
  TCalculatedChecksumEvent /*OnCalculatedChecksum*/)
{
  DebugFail();
}

void TWebDAVFileSystem::ConfirmOverwrite(
  UnicodeString ASourceFullFileName, UnicodeString &ATargetFileName,
  TFileOperationProgressType *OperationProgress,
  const TOverwriteFileParams *FileParams, const TCopyParamType *CopyParam,
  intptr_t Params,
  TOverwriteMode & /*OverwriteMode*/,
  uintptr_t &Answer)
{
  // all = "yes to newer"
  int Answers = qaYes | qaNo | qaCancel | qaYesToAll | qaNoToAll | qaAll;
  TQueryButtonAlias Aliases[3];
  Aliases[0].Button = qaAll;
  Aliases[0].Alias = LoadStr(YES_TO_NEWER_BUTTON);
  Aliases[0].GroupWith = qaYes;
  Aliases[0].GrouppedShiftState = ssCtrl;
  Aliases[1].Button = qaYesToAll;
  Aliases[1].GroupWith = qaYes;
  Aliases[1].GrouppedShiftState = ssShift;
  Aliases[2].Button = qaNoToAll;
  Aliases[2].GroupWith = qaNo;
  Aliases[2].GrouppedShiftState = ssShift;
  TQueryParams QueryParams(qpNeverAskAgainCheck);
  QueryParams.Aliases = Aliases;
  QueryParams.AliasesCount = _countof(Aliases);

  {
    TSuspendFileOperationProgress Suspend(OperationProgress);
    Answer =
      FTerminal->ConfirmFileOverwrite(
        ASourceFullFileName, ATargetFileName, FileParams, Answers, &QueryParams,
        (OperationProgress->GetSide() == osLocal) ? osRemote : osLocal,
        CopyParam, Params, OperationProgress);
  }

  switch (Answer)
  {
  case qaYes:
    // noop
    break;

  case qaNo:
    ThrowSkipFileNull();

  case qaCancel:
    OperationProgress->SetCancelAtLeast(csCancel);
    Abort();
    break;

  default:
    DebugFail();
  }
}

void TWebDAVFileSystem::CustomCommandOnFile(UnicodeString /*AFileName*/,
  const TRemoteFile * /*AFile*/, UnicodeString /*Command*/, intptr_t /*Params*/, TCaptureOutputEvent /*OutputEvent*/)
{
  DebugFail();
}

void TWebDAVFileSystem::AnyCommand(UnicodeString /*Command*/,
  TCaptureOutputEvent /*OutputEvent*/)
{
  DebugFail();
}

TStrings *TWebDAVFileSystem::GetFixedPaths() const
{
  return nullptr;
}

void TWebDAVFileSystem::NeonQuotaResult(
  void *UserData, const ne_uri * /*Uri*/, const ne_prop_result_set *Results)
{
  TSpaceAvailable &SpaceAvailable = *static_cast<TSpaceAvailable *>(UserData);

  const char *Value = GetNeonProp(Results, PROP_QUOTA_AVAILABLE);
  if (Value != nullptr)
  {
    SpaceAvailable.UnusedBytesAvailableToUser = StrToInt64(StrFromNeon(Value));

    const char *Value2 = GetNeonProp(Results, PROP_QUOTA_USED);
    if (Value2 != nullptr)
    {
      SpaceAvailable.BytesAvailableToUser =
        StrToInt64(StrFromNeon(Value2)) + SpaceAvailable.UnusedBytesAvailableToUser;
    }
  }
}

void TWebDAVFileSystem::SpaceAvailable(UnicodeString APath,
  TSpaceAvailable &ASpaceAvailable)
{
  // RFC4331: https://tools.ietf.org/html/rfc4331

  // This is known to be supported by:

  // OpenDrive: for a root drive only (and contrary to the spec, it sends the properties
  // unconditionally, even when not explicitly requested)
  // Server: Apache/2.2.17 (Fedora)
  // X-Powered-By: PHP/5.5.7
  // X-DAV-Powered-By: OpenDrive
  // WWW-Authenticate: Basic realm="PHP WebDAV"

  // IT Hit WebDAV Server:
  // Server: Microsoft-HTTPAPI/1.0
  // X-Engine: IT Hit WebDAV Server .Net v3.8.1877.0 (Evaluation License)

  // Yandex disk:
  // WWW-Authenticate: Basic realm="Yandex.Disk"
  // Server: MochiWeb/1.0

  // OneDrive:
  // it sends the properties unconditionally, even when not explicitly requested

  UnicodeString Path = DirectoryPath(APath);

  ne_propname QuotaProps[3];
  ClearArray(QuotaProps);
  QuotaProps[0].nspace = DAV_PROP_NAMESPACE;
  QuotaProps[0].name = PROP_QUOTA_AVAILABLE;
  QuotaProps[1].nspace = DAV_PROP_NAMESPACE;
  QuotaProps[1].name = PROP_QUOTA_USED;
  QuotaProps[2].nspace = nullptr;
  QuotaProps[2].name = nullptr;

  TOperationVisualizer Visualizer(FTerminal->GetUseBusyCursor());

  CheckStatus(
    ne_simple_propfind(FNeonSession, PathToNeon(Path), NE_DEPTH_ZERO, QuotaProps,
      NeonQuotaResult, &ASpaceAvailable));
}

void TWebDAVFileSystem::CopyToRemote(const TStrings *AFilesToCopy,
  UnicodeString ATargetDir, const TCopyParamType *CopyParam,
  intptr_t Params, TFileOperationProgressType *OperationProgress,
  TOnceDoneOperation &OnceDoneOperation)
{
  DebugAssert((AFilesToCopy != nullptr) && (OperationProgress != nullptr));
  if (!AFilesToCopy)
    return;

  Params &= ~cpAppend;
  UnicodeString TargetDir = GetAbsolutePath(ATargetDir, false);
  UnicodeString FullTargetDir = base::UnixIncludeTrailingBackslash(TargetDir);
  intptr_t Index = 0;
  while ((Index < AFilesToCopy->GetCount()) && OperationProgress && !OperationProgress->GetCancel())
  {
    TRemoteFile *File = AFilesToCopy->GetAs<TRemoteFile>(Index);
    bool Success = false;
    UnicodeString FileName = AFilesToCopy->GetString(Index);
    UnicodeString RealFileName = File ? File->GetFileName() : FileName;
    UnicodeString FileNameOnly = base::ExtractFileName(RealFileName, false);

    try__finally
    {
      SCOPE_EXIT
      {
        OperationProgress->Finish(RealFileName, Success, OnceDoneOperation);
      };
      try
      {
        if (FTerminal->GetSessionData()->GetCacheDirectories())
        {
          FTerminal->DirectoryModified(TargetDir, false);

          if (::DirectoryExists(ApiPath(::ExtractFilePath(FileName))))
          {
            FTerminal->DirectoryModified(FullTargetDir + FileNameOnly, true);
          }
        }
        SourceRobust(FileName, File, FullTargetDir, CopyParam, Params, OperationProgress,
          FLAGSET(Params, cpFirstLevel) ? tfFirstLevel : 0);
        Success = true;
      }
      catch (ESkipFile &E)
      {
        TSuspendFileOperationProgress Suspend(OperationProgress);
        if (!FTerminal->HandleException(&E))
        {
          throw;
        }
      }
    }
    __finally
    {
#if 0
      OperationProgress->Finish(FileName, Success, OnceDoneOperation);
#endif // #if 0
    };
    ++Index;
  }
}

void TWebDAVFileSystem::SourceRobust(UnicodeString AFileName,
  const TRemoteFile *AFile,
  UnicodeString TargetDir, const TCopyParamType *CopyParam, intptr_t Params,
  TFileOperationProgressType *OperationProgress, uintptr_t Flags)
{
  // the same in TSFTPFileSystem

  TUploadSessionAction Action(FTerminal->GetActionLog());
  TRobustOperationLoop RobustLoop(FTerminal, OperationProgress);

  do
  {
    bool ChildError = false;
    try
    {
      Source(AFileName, AFile, TargetDir, CopyParam, Params, OperationProgress,
        Flags, Action, ChildError);
    }
    catch (Exception &E)
    {
      if (!RobustLoop.TryReopen(E))
      {
        if (!ChildError)
        {
          FTerminal->RollbackAction(Action, OperationProgress, &E);
        }
        throw;
      }
    }

    if (RobustLoop.ShouldRetry())
    {
      OperationProgress->RollbackTransfer();
      Action.Restart();
      // prevent overwrite confirmations
      // (should not be set for directories!)
      Params |= cpNoConfirmation;
    }
  }
  while (RobustLoop.Retry());
}

void TWebDAVFileSystem::Source(UnicodeString AFileName,
  const TRemoteFile *AFile,
  UnicodeString TargetDir, const TCopyParamType *CopyParam, intptr_t Params,
  TFileOperationProgressType *OperationProgress, uintptr_t Flags,
  TUploadSessionAction &Action, bool &ChildError)
{
  UnicodeString RealFileName = AFile ? AFile->GetFileName() : AFileName;
  Action.SetFileName(::ExpandUNCFileName(RealFileName));

  OperationProgress->SetFile(RealFileName, false);

  if (!FTerminal->AllowLocalFileTransfer(RealFileName, CopyParam, OperationProgress))
  {
    ThrowSkipFileNull();
  }

  HANDLE LocalFileHandle;
  int64_t MTime;
  int64_t Size;
  uintptr_t LocalFileAttrs = 0;

  FTerminal->TerminalOpenLocalFile(RealFileName, GENERIC_READ,
    &LocalFileAttrs, &LocalFileHandle, nullptr, nullptr, &MTime, &Size);

  bool Dir = FLAGSET(LocalFileAttrs, faDirectory);

  int FD = -1;
  try__finally
  {
    SCOPE_EXIT
    {
      if (FD >= 0)
      {
        // _close calls CloseHandle internally (even doc states, we should not call CloseHandle),
        // but it crashes code guard
        _close(FD);
      }
      else if (LocalFileHandle != nullptr)
      {
        SAFE_CLOSE_HANDLE(LocalFileHandle);
      }
    };
    OperationProgress->SetFileInProgress();

    if (Dir)
    {
      Action.Cancel();
      DirectorySource(::IncludeTrailingBackslash(RealFileName), TargetDir,
        LocalFileAttrs, CopyParam, Params, OperationProgress, Flags);
    }
    else
    {
      UnicodeString DestFileName =
        FTerminal->ChangeFileName(
          CopyParam, base::ExtractFileName(RealFileName, false), osLocal,
          FLAGSET(Flags, tfFirstLevel));

      FTerminal->LogEvent(FORMAT("Copying \"%s\" to remote directory started.", RealFileName));

      OperationProgress->SetLocalSize(Size);

      // Suppose same data size to transfer as to read
      // (not true with ASCII transfer)
      OperationProgress->SetTransferSize(OperationProgress->GetLocalSize());

      UnicodeString DestFullName = TargetDir + DestFileName;

      std::unique_ptr<TRemoteFile> RemoteFile;
      try
      {
        TValueRestorer<TIgnoreAuthenticationFailure> IgnoreAuthenticationFailureRestorer(FIgnoreAuthenticationFailure);
        FIgnoreAuthenticationFailure = iafWaiting;

        // this should not throw
        TRemoteFile *RemoteFilePtr = nullptr;
        CustomReadFileInternal(DestFullName, RemoteFilePtr, nullptr);
        RemoteFile.reset(RemoteFilePtr);
      }
      catch (...)
      {
        if (!FTerminal->GetActive())
        {
          throw;
        }
      }

      TDateTime Modification = ::UnixToDateTime(MTime, FTerminal->GetSessionData()->GetDSTMode());

      if (RemoteFile.get() != nullptr)
      {
        TOverwriteFileParams FileParams;

        FileParams.SourceSize = Size;
        FileParams.SourceTimestamp = Modification;
        FileParams.DestSize = RemoteFile->GetSize();
        FileParams.DestTimestamp = RemoteFile->GetModification();
        RemoteFile.reset();

        TOverwriteMode OverwriteMode = omOverwrite;
        uintptr_t Answer = 0;
        ConfirmOverwrite(AFileName, DestFileName, OperationProgress,
          &FileParams, CopyParam, Params,
          OverwriteMode, Answer);
      }

      DestFullName = TargetDir + DestFileName;
      // only now, we know the final destination
      // (not really true as we do not support changing file name on overwrite dialog)
      Action.Destination(DestFullName);

#if 0
      wchar_t *MimeOut = nullptr;
      if (FindMimeFromData(nullptr, DestFileName.c_str(), nullptr, 0, nullptr, FMFD_URLASFILENAME, &MimeOut, 0) == S_OK)
      {
        FUploadMimeType = MimeOut;
        CoTaskMemFree(MimeOut);
      }
      else
#endif // #if 0
      {
        FUploadMimeType = L"";
      }

      FileOperationLoopCustom(FTerminal, OperationProgress, True, FMTLOAD(TRANSFER_ERROR, RealFileName), "",
      [&]()
      {
        SetFilePointer(LocalFileHandle, 0, nullptr, FILE_BEGIN);

        FD = _open_osfhandle(reinterpret_cast<intptr_t>(LocalFileHandle), O_BINARY);
        if (FD < 0)
        {
          ThrowSkipFileNull();
        }

        TAutoFlag UploadingFlag(FUploading);

        ClearNeonError();
        CheckStatus(ne_put(FNeonSession, PathToNeon(DestFullName), FD));
      });

      if (CopyParam->GetPreserveTime())
      {
        FTerminal->LogEvent(FORMAT("Preserving timestamp [%s]",
            StandardTimestamp(Modification)));

        TTouchSessionAction TouchAction(FTerminal->GetActionLog(), DestFullName, Modification);
        try
        {
          TDateTime ModificationUTC = ::ConvertTimestampToUTC(Modification);
#if 0
          TFormatSettings FormatSettings = GetEngFormatSettings();
          UnicodeString LastModified =
            FormatDateTime(L"ddd, d mmm yyyy hh:nn:ss 'GMT'", ModificationUTC, FormatSettings);
#endif
          uint16_t Y, M, D, H, NN, S, MS;
          TDateTime DateTime = ModificationUTC;
          DateTime.DecodeDate(Y, M, D);
          DateTime.DecodeTime(H, NN, S, MS);
          UnicodeString LastModified = FORMAT("%04d, %d %02d %04d %02d:%02d%02d 'GMT'", D, D, M, Y, H, NN, D);

          UTF8String NeonLastModified(LastModified);
          // second element is "NULL-terminating"
          ne_proppatch_operation Operations[2];
          ClearArray(Operations);
          ne_propname LastModifiedProp;
          LastModifiedProp.nspace = DAV_PROP_NAMESPACE;
          LastModifiedProp.name = PROP_LAST_MODIFIED;
          Operations[0].name = &LastModifiedProp;
          Operations[0].type = ne_propset;
          Operations[0].value = NeonLastModified.c_str();
          int Status = ne_proppatch(FNeonSession, PathToNeon(DestFullName), Operations);
          if (Status == NE_ERROR)
          {
            FTerminal->LogEvent(FORMAT("Preserving timestamp failed, ignoring: %s",
                GetNeonError()));
            // Ignore errors as major WebDAV servers (like IIS), do not support
            // changing getlastmodified.
            // The only server we found that supports this is TradeMicro SafeSync.
            // But it announces itself as "Server: Apache",
            // so it's not reliable to autodetect the support.
            // Microsoft Office alegedly uses <Win32LastModifiedTime>
            // http://sabre.io/dav/clients/msoffice/
            // Carot DAV does that too. But we do not know what server does support this.
            TouchAction.Cancel();
          }
          else
          {
            CheckStatus(Status);
          }
        }
        catch (Exception &E)
        {
          TouchAction.Rollback(&E);
          ChildError = true;
          throw;
        }
      }

      FTerminal->LogFileDone(OperationProgress, DestFullName);
    }
  }
  __finally
  {
#if 0
    if (FD >= 0)
    {
      // _close calls CloseHandle internally (even doc states, we should not call CloseHandle),
      // but it crashes code guard
      _close(FD);
    }
    else if (File != nullptr)
    {
      CloseHandle(File);
    }
#endif // #if 0
  };

  // TODO : Delete also read-only files.
  if (FLAGSET(Params, cpDelete))
  {
    if (!Dir)
    {
      FileOperationLoopCustom(FTerminal, OperationProgress, True, FMTLOAD(CORE_DELETE_LOCAL_FILE_ERROR, RealFileName), "",
      [&]()
      {
        THROWOSIFFALSE(Sysutils::RemoveFile(ApiPath(RealFileName)));
      });
    }
  }
  else if (CopyParam->GetClearArchive() && FLAGSET(LocalFileAttrs, faArchive))
  {
    FileOperationLoopCustom(FTerminal, OperationProgress, True, FMTLOAD(CANT_SET_ATTRS, RealFileName), "",
    [&]()
    {
      THROWOSIFFALSE(FTerminal->SetLocalFileAttributes(ApiPath(RealFileName), LocalFileAttrs & ~faArchive));
    });
  }
}

void TWebDAVFileSystem::DirectorySource(UnicodeString DirectoryName,
  UnicodeString TargetDir, uintptr_t Attrs, const TCopyParamType *CopyParam,
  intptr_t Params, TFileOperationProgressType *OperationProgress, uintptr_t Flags)
{
  UnicodeString DestDirectoryName =
    FTerminal->ChangeFileName(
      CopyParam, base::ExtractFileName(::ExcludeTrailingBackslash(DirectoryName), false),
      osLocal, FLAGSET(Flags, tfFirstLevel));
  UnicodeString DestFullName = base::UnixIncludeTrailingBackslash(TargetDir + DestDirectoryName);
  // create DestFullName if it does not exist
  if (!FTerminal->FileExists(DestFullName))
  {
    TRemoteProperties Properties;
    if (CopyParam->GetPreserveRights())
    {
      Properties.Valid = TValidProperties() << vpRights;
      Properties.Rights = CopyParam->RemoteFileRights(Attrs);
    }
    FTerminal->RemoteCreateDirectory(DestFullName, &Properties);
  }

  OperationProgress->SetFile(DirectoryName);

  if (FLAGCLEAR(Params, cpNoRecurse))
  {
    DWORD FindAttrs = faReadOnly | faHidden | faSysFile | faDirectory | faArchive;
    TSearchRecChecked SearchRec;
    bool FindOK = false;

    UnicodeString FindPath = ApiPath(DirectoryName) + L"*.*";

    FileOperationLoopCustom(FTerminal, OperationProgress, True, FMTLOAD(LIST_DIR_ERROR, DirectoryName), "",
    [&]()
    {
      FindOK =
        (FindFirstChecked(FindPath, FindAttrs, SearchRec) == 0);
    });

    try__finally
    {
      SCOPE_EXIT
      {
        base::FindClose(SearchRec);
      };
      while (FindOK && !OperationProgress->GetCancel())
      {
        UnicodeString FileName = DirectoryName + SearchRec.Name;
        try
        {
          if ((SearchRec.Name != THISDIRECTORY) && (SearchRec.Name != PARENTDIRECTORY))
          {
            SourceRobust(FileName, nullptr, DestFullName, CopyParam, Params, OperationProgress,
              Flags & ~(tfFirstLevel));
          }
        }
        catch (ESkipFile &E)
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

        FileOperationLoopCustom(FTerminal, OperationProgress, True, FMTLOAD(LIST_DIR_ERROR, DirectoryName), "",
        [&]()
        {
          FindOK = FindNextChecked(SearchRec) == 0;
        });
      }
    }
    __finally
    {
#if 0
      ::FindClose(SearchRec);
#endif // #if 0
    };
    // TODO : Delete also read-only directories.
    // TODO : Show error message on failure.
    if (!OperationProgress->GetCancel())
    {
      if (FLAGSET(Params, cpDelete))
      {
        FTerminal->RemoveLocalDirectory(ApiPath(DirectoryName));
      }
      else if (CopyParam->GetClearArchive() && FLAGSET(Attrs, faArchive))
      {
        FileOperationLoopCustom(FTerminal, OperationProgress, True, FMTLOAD(CANT_SET_ATTRS, DirectoryName), "",
        [&]()
        {
          THROWOSIFFALSE(FTerminal->SetLocalFileAttributes(ApiPath(DirectoryName), Attrs & ~faArchive));
        });
      }
    }
  }
}

void TWebDAVFileSystem::CopyToLocal(const TStrings *AFilesToCopy,
  UnicodeString TargetDir, const TCopyParamType *CopyParam,
  intptr_t Params, TFileOperationProgressType *OperationProgress,
  TOnceDoneOperation &OnceDoneOperation)
{
  Params &= ~cpAppend;
  UnicodeString FullTargetDir = ::IncludeTrailingBackslash(TargetDir);

  intptr_t Index = 0;
  while (Index < AFilesToCopy->GetCount() && !OperationProgress->GetCancel())
  {
    UnicodeString FileName = AFilesToCopy->GetString(Index);
    const TRemoteFile *File = AFilesToCopy->GetAs<TRemoteFile>(Index);
    bool Success = false;
    try__finally
    {
      SCOPE_EXIT
      {
        OperationProgress->Finish(FileName, Success, OnceDoneOperation);
      };
      UnicodeString AbsoluteFilePath = GetAbsolutePath(FileName, false);
      UnicodeString TargetDirectory = CreateTargetDirectory(File->GetFileName(), FullTargetDir, CopyParam);
      try
      {
        SinkRobust(AbsoluteFilePath, File, TargetDirectory, CopyParam, Params,
          OperationProgress, FLAGSET(Params, cpFirstLevel) ? tfFirstLevel : 0);
        Success = true;
      }
      catch (ESkipFile &E)
      {
        TSuspendFileOperationProgress Suspend(OperationProgress);
        if (!FTerminal->HandleException(&E))
        {
          throw;
        }
      }
    }
    __finally
    {
#if 0
      OperationProgress->Finish(FileName, Success, OnceDoneOperation);
#endif // #if 0
    };
    ++Index;
  }
}

void TWebDAVFileSystem::SinkRobust(UnicodeString AFileName,
  const TRemoteFile *AFile, UnicodeString TargetDir,
  const TCopyParamType *CopyParam, intptr_t Params,
  TFileOperationProgressType *OperationProgress, uintptr_t Flags)
{
  // the same in TSFTPFileSystem

  TDownloadSessionAction Action(FTerminal->GetActionLog());
  TRobustOperationLoop RobustLoop(FTerminal, OperationProgress);

  do
  {
    bool ChildError = false;
    try
    {
      Sink(AFileName, AFile, TargetDir, CopyParam, Params, OperationProgress,
        Flags, Action, ChildError);
    }
    catch (Exception &E)
    {
      if (!RobustLoop.TryReopen(E))
      {
        if (!ChildError)
        {
          FTerminal->RollbackAction(Action, OperationProgress, &E);
        }
        throw;
      }
    }

    if (RobustLoop.ShouldRetry())
    {
      OperationProgress->RollbackTransfer();
      Action.Restart();
      DebugAssert(AFile != nullptr);
      if (AFile && !AFile->GetIsDirectory())
      {
        // prevent overwrite confirmations
        Params |= cpNoConfirmation;
      }
    }
  }
  while (RobustLoop.Retry());
}

void TWebDAVFileSystem::NeonCreateRequest(
  ne_request *Request, void *UserData, const char * /*Method*/, const char * /*Uri*/)
{
  TWebDAVFileSystem *FileSystem = static_cast<TWebDAVFileSystem *>(UserData);
  ne_set_request_private(Request, SESSION_FS_KEY, FileSystem);
  ne_add_response_body_reader(Request, NeonBodyAccepter, NeonBodyReader, Request);
  FileSystem->FNtlmAuthenticationFailed = false;
}

void TWebDAVFileSystem::NeonPreSend(
  ne_request *Request, void *UserData, ne_buffer *Header)
{
  TWebDAVFileSystem *FileSystem = static_cast<TWebDAVFileSystem *>(UserData);

  FileSystem->FAuthorizationProtocol = L"";
  UnicodeString HeaderBuf(StrFromNeon(UTF8String(Header->data, Header->used)));
  const UnicodeString AuthorizationHeaderName(L"Authorization:");
  intptr_t P = HeaderBuf.Pos(AuthorizationHeaderName);
  if (P > 0)
  {
    P += AuthorizationHeaderName.Length();
    intptr_t P2 = PosEx(L"\n", HeaderBuf, P);
    if (DebugAlwaysTrue(P2 > 0))
    {
      UnicodeString AuthorizationHeader = HeaderBuf.SubString(P, P2 - P).Trim();
      FileSystem->FAuthorizationProtocol = CutToChar(AuthorizationHeader, L' ', false);
      FileSystem->FLastAuthorizationProtocol = FileSystem->FAuthorizationProtocol;
    }
  }

  if (FileSystem->FDownloading)
  {
    // Needed by IIS server to make it download source code, not code output,
    // and mainly to even allow downloading file with unregistered extensions.
    // Without it files like .001 return 404 (Not found) HTTP code.
    // https://msdn.microsoft.com/en-us/library/cc250098.aspx
    // https://msdn.microsoft.com/en-us/library/cc250216.aspx
    // http://lists.manyfish.co.uk/pipermail/neon/2012-April/001452.html
    // It's also supported by Oracle server:
    // https://docs.oracle.com/cd/E19146-01/821-1828/gczya/index.html
    // We do not know yet of any server that fails when the header is used,
    // so it's added unconditionally.
    ne_buffer_zappend(Header, "Translate: f\r\n");
  }

  const UnicodeString ContentTypeHeaderPrefix(L"Content-Type: ");
  if (FileSystem->FTerminal->GetLog()->GetLogging())
  {
    const char *Buffer;
    size_t Size;
    if (ne_get_request_body_buffer(Request, &Buffer, &Size))
    {
      // all neon request types that use ne_add_request_header
      // use XML content-type, so it's text-based
      DebugAssert(AnsiContainsText(HeaderBuf, ContentTypeHeaderPrefix + NE_XML_MEDIA_TYPE));
      FileSystem->FTerminal->GetLog()->Add(llInput, UnicodeString(UTF8String(Buffer, Size)));
    }
  }

  if (FileSystem->FUploading)
  {
    ne_set_request_body_provider_pre(Request,
      FileSystem->NeonUploadBodyProvider, FileSystem);
    if (!FileSystem->FUploadMimeType.IsEmpty())
    {
      UnicodeString ContentTypeHeader = ContentTypeHeaderPrefix + FileSystem->FUploadMimeType + L"\r\n";
      ne_buffer_zappend(Header, AnsiString(ContentTypeHeader).c_str());
    }
  }

  FileSystem->FResponse = L"";
}

int TWebDAVFileSystem::NeonPostSend(ne_request * /*Req*/, void *UserData,
  const ne_status * /*Status*/)
{
  TWebDAVFileSystem *FileSystem = static_cast<TWebDAVFileSystem *>(UserData);
  if (!FileSystem->FResponse.IsEmpty())
  {
    FileSystem->FTerminal->GetLog()->Add(llOutput, FileSystem->FResponse);
  }
  return NE_OK;
}

bool TWebDAVFileSystem::IsNtlmAuthentication() const
{
  return
    SameText(FAuthorizationProtocol, L"NTLM") ||
    SameText(FAuthorizationProtocol, L"Negotiate");
}

void TWebDAVFileSystem::HttpAuthenticationFailed()
{
  // NTLM/GSSAPI failed
  if (IsNtlmAuthentication())
  {
    if (FNtlmAuthenticationFailed)
    {
      // Next time do not try Negotiate (NTLM/GSSAPI),
      // otherwise we end up in an endless loop.
      // If the server returns all other challenges in the response, removing the Negotiate
      // protocol will itself ensure that other protocols are tried (we haven't seen this behaviour).
      // IIS will return only Negotiate response if the request was Negotiate, so there's no fallback.
      // We have to retry with a fresh request. That's what FAuthenticationRetry does.
      FTerminal->LogEvent(FORMAT("%s challenge failed, will try different challenge", FAuthorizationProtocol));
      ne_remove_server_auth(FNeonSession);
      NeonAddAuthentication(false);
      FAuthenticationRetry = true;
    }
    else
    {
      // The first 401 is expected, the server is using it to send WWW-Authenticate header with data.
      FNtlmAuthenticationFailed = true;
    }
  }
}

void TWebDAVFileSystem::NeonPostHeaders(ne_request * /*Req*/, void *UserData, const ne_status *Status)
{
  TWebDAVFileSystem *FileSystem = static_cast<TWebDAVFileSystem *>(UserData);
  if (Status->code == HttpUnauthorized)
  {
    FileSystem->HttpAuthenticationFailed();
  }
}

ssize_t TWebDAVFileSystem::NeonUploadBodyProvider(void *UserData, char * /*Buffer*/, size_t /*BufLen*/)
{
  TWebDAVFileSystem *FileSystem = static_cast<TWebDAVFileSystem *>(UserData);
  ssize_t Result;
  if (FileSystem->CancelTransfer())
  {
    Result = -1;
  }
  else
  {
    Result = 1;
  }
  return Result;
}

static void AddHeaderValueToList(UnicodeString &List, ne_request *Request, const char *Name)
{
  const char *Value = ne_get_response_header(Request, Name);
  if (Value != nullptr)
  {
    AddToList(List, StrFromNeon(Value), L"; ");
  }
}

int TWebDAVFileSystem::NeonBodyAccepter(void *UserData, ne_request *Request, const ne_status *Status)
{
  DebugAssert(UserData == Request);
  TWebDAVFileSystem *FileSystem =
    static_cast<TWebDAVFileSystem *>(ne_get_request_private(Request, SESSION_FS_KEY));

  bool AuthenticationFailureCode = (Status->code == HttpUnauthorized);
  bool PasswordAuthenticationFailed = AuthenticationFailureCode && FileSystem->FAuthenticationRequested;
  bool AuthenticationFailed = PasswordAuthenticationFailed || (AuthenticationFailureCode && FileSystem->IsNtlmAuthentication());
  bool AuthenticationNeeded = AuthenticationFailureCode && !AuthenticationFailed;

  if (FileSystem->FInitialHandshake)
  {
    UnicodeString Line;
    if (AuthenticationNeeded)
    {
      Line = LoadStr(STATUS_AUTHENTICATE);
    }
    else if (AuthenticationFailed)
    {
      Line = LoadStr(FTP_ACCESS_DENIED);
    }
    else if (Status->klass == 2)
    {
      Line = LoadStr(STATUS_AUTHENTICATED);
    }

    if (!Line.IsEmpty())
    {
      FileSystem->FTerminal->Information(Line, true);
    }

    UnicodeString RemoteSystem;
    // Used by IT Hit WebDAV Server:
    // Server: Microsoft-HTTPAPI/1.0
    // X-Engine: IT Hit WebDAV Server .Net v3.8.1877.0 (Evaluation License)
    AddHeaderValueToList(RemoteSystem, Request, "X-Engine");
    // Used by OpenDrive:
    // Server: Apache/2.2.17 (Fedora)
    // X-Powered-By: PHP/5.5.7
    // X-DAV-Powered-By: OpenDrive
    AddHeaderValueToList(RemoteSystem, Request, "X-DAV-Powered-By");
    // Used by IIS:
    // Server: Microsoft-IIS/8.5
    AddHeaderValueToList(RemoteSystem, Request, "Server");
    // Not really useful.
    // Can be e.g. "PleskLin"
    AddHeaderValueToList(RemoteSystem, Request, "X-Powered-By");
    FileSystem->FFileSystemInfo.RemoteSystem = RemoteSystem;
  }

  // When we explicitly fail authentication of request
  // with FIgnoreAuthenticationFailure flag (after it failed with password),
  // neon resets its internal password store and tries the next request
  // without calling our authentication hook first
  // (note AuthenticationFailed vs. AuthenticationNeeded)
  // what likely fails, but we do not want to reset out password
  // (as it was not even tried yet for this request).
  if (PasswordAuthenticationFailed)
  {
    if (FileSystem->FIgnoreAuthenticationFailure == iafNo)
    {
      FileSystem->FPassword = RawByteString();
    }
    else
    {
      FileSystem->FIgnoreAuthenticationFailure = iafPasswordFailed;
    }
  }

  return ne_accept_2xx(UserData, Request, Status);
}

bool TWebDAVFileSystem::CancelTransfer()
{
  bool Result = false;
  TFileOperationProgressType *OperationProgress = FTerminal->GetOperationProgress();
  if ((FUploading || FDownloading) &&
    (OperationProgress != nullptr) &&
    (OperationProgress->GetCancel() != csContinue))
  {
    if (OperationProgress->ClearCancelFile())
    {
      FSkipped = true;
    }
    else
    {
      FCancelled = true;
    }
    Result = true;
  }
  return Result;
}

int TWebDAVFileSystem::NeonBodyReader(void *UserData, const char *Buf, size_t Len)
{
  ne_request *Request = static_cast<ne_request *>(UserData);
  TWebDAVFileSystem *FileSystem =
    static_cast<TWebDAVFileSystem *>(ne_get_request_private(Request, SESSION_FS_KEY));

  if (FileSystem->FTerminal->GetLog()->GetLogging())
  {
    ne_content_type ContentType;
    if (ne_get_content_type(Request, &ContentType) == 0)
    {
      // The main point of the content-type check was to exclude
      // GET responses (with file contents).
      // But this won't work when downloading text files that have text
      // content type on their own, hence the additional not-downloading test.
      if (!FileSystem->FDownloading &&
        ((ne_strcasecmp(ContentType.type, "text") == 0) ||
          media_type_is_xml(&ContentType)))
      {
        UnicodeString Content = UnicodeString(UTF8String(Buf, Len)).Trim();
        FileSystem->FResponse += Content;
      }
      ne_free(ContentType.value);
    }
  }

  int Result = FileSystem->CancelTransfer() ? 1 : 0;
  return Result;
}

void TWebDAVFileSystem::Sink(UnicodeString AFileName,
  const TRemoteFile *AFile, UnicodeString TargetDir,
  const TCopyParamType *CopyParam, intptr_t AParams,
  TFileOperationProgressType *OperationProgress, uintptr_t Flags,
  TDownloadSessionAction &Action, bool &ChildError)
{
  UnicodeString FileNameOnly = base::UnixExtractFileName(AFileName);

  Action.SetFileName(AFileName);

  DebugAssert(AFile);
  TFileMasks::TParams MaskParams;
  MaskParams.Size = AFile->GetSize();
  MaskParams.Modification = AFile->GetModification();

  UnicodeString BaseFileName = FTerminal->GetBaseFileName(AFileName);
  if (!CopyParam->AllowTransfer(BaseFileName, osRemote, AFile->GetIsDirectory(), MaskParams))
  {
    FTerminal->LogEvent(FORMAT("File \"%s\" excluded from transfer", AFileName));
    ThrowSkipFileNull();
  }

  if (CopyParam->SkipTransfer(AFileName, AFile->GetIsDirectory()))
  {
    OperationProgress->AddSkippedFileSize(AFile->GetSize());
    ThrowSkipFileNull();
  }

  FTerminal->LogFileDetails(AFileName, TDateTime(), AFile->GetSize());

  OperationProgress->SetFile(FileNameOnly);

  UnicodeString DestFileName =
    CopyParam->ChangeFileName(
      base::UnixExtractFileName(AFile->GetFileName()), osRemote, FLAGSET(Flags, tfFirstLevel));
  UnicodeString DestFullName = TargetDir + DestFileName;

  if (AFile->GetIsDirectory())
  {
    Action.Cancel();
    if (DebugAlwaysTrue(FTerminal->CanRecurseToDirectory(AFile)))
    {
      FileOperationLoopCustom(FTerminal, OperationProgress, True, FMTLOAD(NOT_DIRECTORY_ERROR, DestFullName), "",
      [&]()
      {
        DWORD LocalFileAttrs = FTerminal->GetLocalFileAttributes(ApiPath(DestFullName));
        if (FLAGCLEAR(LocalFileAttrs, faDirectory))
        {
          ThrowExtException();
        }
      });

      FileOperationLoopCustom(FTerminal, OperationProgress, True, FMTLOAD(CREATE_DIR_ERROR, DestFullName), "",
      [&]()
      {
        THROWOSIFFALSE(::ForceDirectories(ApiPath(DestFullName)));
      });

      if (FLAGCLEAR(AParams, cpNoRecurse))
      {
        TSinkFileParams SinkFileParams;
        SinkFileParams.TargetDir = ::IncludeTrailingBackslash(DestFullName);
        SinkFileParams.CopyParam = CopyParam;
        SinkFileParams.Params = AParams;
        SinkFileParams.OperationProgress = OperationProgress;
        SinkFileParams.Skipped = false;
        SinkFileParams.Flags = Flags & ~tfFirstLevel;

        FTerminal->ProcessDirectory(AFileName, nb::bind(&TWebDAVFileSystem::SinkFile, this), &SinkFileParams);

        // Do not delete directory if some of its files were skip.
        // Throw "skip file" for the directory to avoid attempt to deletion
        // of any parent directory
        if (FLAGSET(AParams, cpDelete) && SinkFileParams.Skipped)
        {
          ThrowSkipFileNull();
        }
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
    FTerminal->LogEvent(FORMAT("Copying \"%s\" to local directory started.", AFileName));
    if (::FileExists(ApiPath(DestFullName)))
    {
      int64_t Size = 0;
      int64_t MTime = 0;
      FTerminal->TerminalOpenLocalFile(DestFullName, GENERIC_READ, nullptr,
        nullptr, nullptr, &MTime, nullptr, &Size);
      TOverwriteFileParams FileParams;

      FileParams.SourceSize = AFile->GetSize();
      FileParams.SourceTimestamp = AFile->GetModification();
      FileParams.DestSize = Size;
      FileParams.DestTimestamp = ::UnixToDateTime(MTime,
          FTerminal->GetSessionData()->GetDSTMode());

      TOverwriteMode OverwriteMode = omOverwrite;
      uintptr_t Answer = 0;
      ConfirmOverwrite(AFileName, DestFileName, OperationProgress,
        &FileParams, CopyParam, AParams,
        OverwriteMode, Answer);
    }

    // Suppose same data size to transfer as to write
    OperationProgress->SetTransferSize(AFile->GetSize());
    OperationProgress->SetLocalSize(OperationProgress->GetTransferSize());

    DWORD LocalFileAttrs = INVALID_FILE_ATTRIBUTES;
    FileOperationLoopCustom(FTerminal, OperationProgress, True, FMTLOAD(NOT_FILE_ERROR, DestFullName), "",
    [&]()
    {
      LocalFileAttrs = FTerminal->GetLocalFileAttributes(ApiPath(DestFullName));
      if ((LocalFileAttrs != INVALID_FILE_ATTRIBUTES) && FLAGSET(LocalFileAttrs, faDirectory))
      {
        ThrowExtException();
      }
    });

    UnicodeString FilePath = base::UnixExtractFilePath(AFileName);
    if (FilePath.IsEmpty())
    {
      FilePath = L"/";
    }

    UnicodeString ExpandedDestFullName(::ExpandUNCFileName(DestFullName));
    Action.Destination(ExpandedDestFullName);

    FileOperationLoopCustom(FTerminal, OperationProgress, True, FMTLOAD(TRANSFER_ERROR, AFileName), "",
    [&]()
    {
      HANDLE LocalFileHandle = FTerminal->TerminalCreateLocalFile(DestFullName,
          GENERIC_WRITE, 0, FLAGSET(AParams, cpNoConfirmation) ? CREATE_ALWAYS : CREATE_NEW, 0);
      if (LocalFileHandle == INVALID_HANDLE_VALUE)
      {
        ThrowSkipFileNull();
      }

      bool DeleteLocalFile = true;

      int FD = -1;
      try__finally
      {
        SCOPE_EXIT
        {
          if (FD >= 0)
          {
            // _close calls CloseHandle internally (even doc states, we should not call CloseHandle),
            // but it crashes code guard
            _close(FD);
          }
          else
          {
            SAFE_CLOSE_HANDLE(LocalFileHandle);
          }

          if (DeleteLocalFile)
          {
            FileOperationLoopCustom(FTerminal, OperationProgress, True, FMTLOAD(CORE_DELETE_LOCAL_FILE_ERROR, DestFullName), "",
            [&]()
            {
              THROWOSIFFALSE(Sysutils::RemoveFile(ApiPath(DestFullName)));
            });
          }
        };

        FD = _open_osfhandle(reinterpret_cast<intptr_t>(LocalFileHandle), O_BINARY);
        if (FD < 0)
        {
          ThrowSkipFileNull();
        }

        TAutoFlag DownloadingFlag(FDownloading);

        ClearNeonError();
        CheckStatus(ne_get(FNeonSession, PathToNeon(AFileName), FD));
        DeleteLocalFile = false;

        if (CopyParam->GetPreserveTime())
        {
          TDateTime Modification = AFile->GetModification();
          FILETIME WrTime = DateTimeToFileTime(Modification, FTerminal->GetSessionData()->GetDSTMode());
          FTerminal->LogEvent(FORMAT("Preserving timestamp [%s]",
              StandardTimestamp(Modification)));
          SetFileTime(LocalFileHandle, nullptr, nullptr, &WrTime);
        }
      }
      __finally
      {
#if 0
        if (FD >= 0)
        {
          // _close calls CloseHandle internally (even doc states, we should not call CloseHandle),
          // but it crashes code guard
          _close(FD);
        }
        else
        {
          CloseHandle(LocalFileHandle);
        }

        if (DeleteLocalFile)
        {
          FileOperationLoopCustom(FTerminal, OperationProgress, True, FMTLOAD(CORE_DELETE_LOCAL_FILE_ERROR, DestFullName), "",
          [&]()
          {
            THROWOSIFFALSE(Sysutils::RemoveFile(ApiPath(DestFullName)));
          });
        }
#endif // #if 0
      };
    });

    if (LocalFileAttrs == INVALID_FILE_ATTRIBUTES)
    {
      LocalFileAttrs = faArchive;
    }
    DWORD NewAttrs = CopyParam->LocalFileAttrs(*AFile->GetRights());
    if ((NewAttrs & LocalFileAttrs) != NewAttrs)
    {
      FileOperationLoopCustom(FTerminal, OperationProgress, True, FMTLOAD(CANT_SET_ATTRS, DestFullName), "",
      [&]()
      {
        THROWOSIFFALSE(FTerminal->SetLocalFileAttributes(ApiPath(DestFullName), LocalFileAttrs | NewAttrs));
      });
    }

    FTerminal->LogFileDone(OperationProgress, ExpandedDestFullName);
  }

  if (FLAGSET(AParams, cpDelete))
  {
    DebugAssert(FLAGCLEAR(AParams, cpNoRecurse));
    ChildError = true;
    // If file is directory, do not delete it recursively, because it should be
    // empty already. If not, it should not be deleted (some files were
    // skipped or some new files were copied to it, while we were downloading)
    intptr_t Params = dfNoRecursive;
    FTerminal->RemoteDeleteFile(AFileName, AFile, &Params);
    ChildError = false;
  }
}

void TWebDAVFileSystem::SinkFile(UnicodeString AFileName,
  const TRemoteFile *AFile, void *Param)
{
  TSinkFileParams *Params = get_as<TSinkFileParams>(Param);
  DebugAssert(Params->OperationProgress);
  try
  {
    SinkRobust(AFileName, AFile, Params->TargetDir, Params->CopyParam,
      Params->Params, Params->OperationProgress, Params->Flags);
  }
  catch (ESkipFile &E)
  {
    TFileOperationProgressType *OperationProgress = Params->OperationProgress;

    Params->Skipped = true;

    {
      TSuspendFileOperationProgress Suspend(OperationProgress);
      if (!FTerminal->HandleException(&E))
      {
        throw;
      }
    }

    if (OperationProgress->GetCancel())
    {
      Abort();
    }
  }
}

bool TWebDAVFileSystem::VerifyCertificate(const TWebDAVCertificateData &Data, bool Aux)
{
  FSessionInfo.CertificateFingerprint = Data.Fingerprint;

  bool Result;
  if (FTerminal->GetSessionData()->GetFingerprintScan())
  {
    Result = false;
  }
  else
  {
    FTerminal->LogEvent(
      FORMAT("Verifying certificate for \"%s\" with fingerprint %s and %2.2X failures",
        Data.Subject, Data.Fingerprint, Data.Failures));

    int Failures = Data.Failures;

    UnicodeString SiteKey = TSessionData::FormatSiteKey(FHostName, FPortNumber);
    Result =
      FTerminal->VerifyCertificate(HttpsCertificateStorageKey, SiteKey, Data.Fingerprint, Data.Subject, Failures);

    if (!Result)
    {
      UnicodeString WindowsCertificateError;
      if (NeonWindowsValidateCertificate(Failures, Data.AsciiCert, WindowsCertificateError))
      {
        FTerminal->LogEvent(L"Certificate verified against Windows certificate store");
        // There can be also other flags, not just the NE_SSL_UNTRUSTED.
        Result = (Failures == 0);
      }
      else
      {
        FTerminal->LogEvent(
          FORMAT("Certificate failed to verify against Windows certificate store: %s", DefaultStr(WindowsCertificateError, L"no details")));
      }
    }

    UnicodeString Summary;
    if (Failures == 0)
    {
      Summary = LoadStr(CERT_OK);
    }
    else
    {
      Summary = NeonCertificateFailuresErrorStr(Failures, FHostName);
    }

    UnicodeString ValidityTimeFormat = L"ddddd tt";
    FSessionInfo.Certificate =
      FMTLOAD(CERT_TEXT,
        UnicodeString(Data.Issuer + L"\n"),
        UnicodeString(Data.Subject + L"\n"),
        FormatDateTime(ValidityTimeFormat, Data.ValidFrom),
        FormatDateTime(ValidityTimeFormat, Data.ValidUntil),
        Data.Fingerprint,
        Summary);

    if (!Result)
    {
      TClipboardHandler ClipboardHandler;
      ClipboardHandler.Text = Data.Fingerprint;

      TQueryButtonAlias Aliases[1];
      Aliases[0].Button = qaRetry;
      Aliases[0].Alias = LoadStr(COPY_KEY_BUTTON);
      Aliases[0].OnClick = nb::bind(&TClipboardHandler::Copy, &ClipboardHandler);

      TQueryParams Params;
      Params.HelpKeyword = HELP_VERIFY_CERTIFICATE;
      Params.NoBatchAnswers = qaYes | qaRetry;
      Params.Aliases = Aliases;
      Params.AliasesCount = _countof(Aliases);
      uintptr_t Answer = FTerminal->QueryUser(
          FMTLOAD(VERIFY_CERT_PROMPT3, FSessionInfo.Certificate),
          nullptr, qaYes | qaNo | qaCancel | qaRetry, &Params, qtWarning);
      switch (Answer)
      {
      case qaYes:
        FTerminal->CacheCertificate(HttpsCertificateStorageKey, SiteKey, Data.Fingerprint, Failures);
        Result = true;
        break;

      case qaNo:
        Result = true;
        break;

      default:
        DebugFail();
        Result = false;
        break;

      case qaCancel:
        // FTerminal->GetConfiguration()->GetUsage()->Inc(L"HostNotVerified");
        Result = false;
        break;
      }

      if (Result && !Aux)
      {
        FTerminal->GetConfiguration()->RememberLastFingerprint(
          FTerminal->GetSessionData()->GetSiteKey(), TlsFingerprintType, FSessionInfo.CertificateFingerprint);
      }
    }

    if (Result && !Aux)
    {
      CollectTLSSessionInfo();
    }
  }

  return Result;
}

void TWebDAVFileSystem::CollectTLSSessionInfo()
{
  // See also TFTPFileSystem::Open().
  // Have to cache the value as the connection (the neon HTTP session, not "our" session)
  // can be closed as the time we need it in CollectUsage().
  FTlsVersionStr = StrFromNeon(ne_ssl_get_version(FNeonSession));
  AddToList(FSessionInfo.SecurityProtocolName, FTlsVersionStr, L", ");

  char *cipher = ne_ssl_get_cipher(FNeonSession);
  UnicodeString Cipher = StrFromNeon(cipher);
  ne_free(cipher);
  FSessionInfo.CSCipher = Cipher;
  FSessionInfo.SCCipher = Cipher;

  // see CAsyncSslSocketLayer::PrintSessionInfo()
  FTerminal->LogEvent(FORMAT("Using %s, cipher %s", FTlsVersionStr, Cipher));
}

// A neon-session callback to validate the SSL certificate when the CA
// is unknown (e.g. a self-signed cert), or there are other SSL
// certificate problems.
int TWebDAVFileSystem::DoNeonServerSSLCallback(void *UserData, int Failures, const ne_ssl_certificate *Certificate, bool Aux)
{
  TWebDAVCertificateData Data;

  char Fingerprint[NE_SSL_DIGESTLEN] = {0};
  if (ne_ssl_cert_digest(Certificate, Fingerprint) != 0)
  {
    strcpy(Fingerprint, "<unknown>");
  }
  Data.Fingerprint = StrFromNeon(Fingerprint);
  Data.AsciiCert = NeonExportCertificate(Certificate);

  char *Subject = ne_ssl_readable_dname(ne_ssl_cert_subject(Certificate));
  Data.Subject = StrFromNeon(Subject);
  ne_free(Subject);
  char *Issuer = ne_ssl_readable_dname(ne_ssl_cert_issuer(Certificate));
  Data.Issuer = StrFromNeon(Issuer);
  ne_free(Issuer);

  Data.Failures = Failures;

  time_t ValidFrom;
  time_t ValidUntil;
  ne_ssl_cert_validity_time(Certificate, &ValidFrom, &ValidUntil);
  Data.ValidFrom = UnixToDateTime(ValidFrom, dstmWin);
  Data.ValidUntil = UnixToDateTime(ValidUntil, dstmWin);

  TWebDAVFileSystem *FileSystem = static_cast<TWebDAVFileSystem *>(UserData);

  return FileSystem->VerifyCertificate(Data, Aux) ? NE_OK : NE_ERROR;
}

int TWebDAVFileSystem::NeonServerSSLCallbackMain(void *UserData, int Failures, const ne_ssl_certificate *Certificate)
{
  return DoNeonServerSSLCallback(UserData, Failures, Certificate, false);
}

int TWebDAVFileSystem::NeonServerSSLCallbackAux(void *UserData, int Failures, const ne_ssl_certificate *Certificate)
{
  return DoNeonServerSSLCallback(UserData, Failures, Certificate, true);
}

void TWebDAVFileSystem::NeonProvideClientCert(void *UserData, ne_session *Sess,
  const ne_ssl_dname *const * /*DNames*/, int /*DNCount*/)
{
  TWebDAVFileSystem *FileSystem = static_cast<TWebDAVFileSystem *>(UserData);

  FileSystem->FTerminal->LogEvent(LoadStr(NEED_CLIENT_CERTIFICATE));

  X509 *Certificate;
  EVP_PKEY *PrivateKey;
  if (FileSystem->FTerminal->LoadTlsCertificate(Certificate, PrivateKey))
  {
    ne_ssl_client_cert *NeonCertificate = ne_ssl_clicert_create(Certificate, PrivateKey);
    ne_ssl_set_clicert(Sess, NeonCertificate);
    ne_ssl_clicert_free(NeonCertificate);
  }
}

int TWebDAVFileSystem::NeonRequestAuth(
  void *UserData, const char *Realm, int Attempt, char *UserName, char *Password)
{
  DebugUsedParam(Realm);
  DebugUsedParam(Attempt);
  TWebDAVFileSystem *FileSystem = static_cast<TWebDAVFileSystem *>(UserData);

  TTerminal *Terminal = FileSystem->FTerminal;
  TSessionData *SessionData = Terminal->GetSessionData();

  bool Result = true;

  // will ask for username only once
  if (FileSystem->FUserName.IsEmpty())
  {
    if (!SessionData->SessionGetUserName().IsEmpty())
    {
      FileSystem->FUserName = SessionData->GetUserNameExpanded();
    }
    else
    {
      if (!Terminal->PromptUser(SessionData, pkUserName, LoadStr(USERNAME_TITLE), L"",
          LoadStr(USERNAME_PROMPT2), true, NE_ABUFSIZ, FileSystem->FUserName))
      {
        // note that we never get here actually
        Result = false;
      }
    }
  }

  UnicodeString APassword;
  if (Result)
  {
    // Some servers (Gallery2 on discontinued g2.pixi.me)
    // return authentication error (401) on PROPFIND request for
    // non-existing files.
    // When we already tried password before, do not try anymore.
    // When we did not try password before (possible only when
    // server does not require authentication for any previous request,
    // such as when read access is not authenticated), try it now,
    // but use special flag for the try, because when it fails
    // we still want to try password for future requests (such as PUT).

    if (!FileSystem->FPassword.IsEmpty())
    {
      if (FileSystem->FIgnoreAuthenticationFailure == iafPasswordFailed)
      {
        // Fail PROPFIND /nonexising request...
        Result = false;
      }
      else
      {
        APassword = Terminal->DecryptPassword(FileSystem->FPassword);
      }
    }
    else
    {
      if (!SessionData->GetPassword().IsEmpty() && !FileSystem->FStoredPasswordTried)
      {
        APassword = SessionData->GetPassword();
        FileSystem->FStoredPasswordTried = true;
      }
      else
      {
        // Asking for password (or using configured password) the first time,
        // and asking for password.
        // Note that we never get false here actually
        Result =
          Terminal->PromptUser(
            SessionData, pkPassword, LoadStr(PASSWORD_TITLE), L"",
            LoadStr(PASSWORD_PROMPT), false, NE_ABUFSIZ, APassword);
      }

      if (Result)
      {
        // While neon remembers the password on its own,
        // we need to keep a copy in case neon store gets reset by
        // 401 response to PROPFIND /nonexisting on G2, see above.
        // Possibly we can do this for G2 servers only.
        FileSystem->FPassword = Terminal->EncryptPassword(APassword);
      }
    }
  }

  if (Result)
  {
    strncpy(UserName, StrToNeon(FileSystem->FUserName), NE_ABUFSIZ);
    strncpy(Password, StrToNeon(APassword), NE_ABUFSIZ);
  }

  FileSystem->FAuthenticationRequested = true;

  return Result ? 0 : -1;
}

void TWebDAVFileSystem::NeonNotifier(void *UserData, ne_session_status Status, const ne_session_status_info *StatusInfo)
{
  TWebDAVFileSystem *FileSystem = static_cast<TWebDAVFileSystem *>(UserData);
  TFileOperationProgressType *OperationProgress = FileSystem->FTerminal->GetOperationProgress();

  // We particularly have to filter out response to "put" request,
  // handling that would reset the upload progress back to low number (response is small).
  if (((FileSystem->FUploading && (Status == ne_status_sending)) ||
      (FileSystem->FDownloading && (Status == ne_status_recving))) &&
    DebugAlwaysTrue(OperationProgress != nullptr))
  {
    int64_t Progress = StatusInfo->sr.progress;
    int64_t Diff = Progress - OperationProgress->GetTransferredSize();

    if (Diff > 0)
    {
      OperationProgress->ThrottleToCPSLimit(static_cast<unsigned long>(Diff));
    }

    int64_t Total = StatusInfo->sr.total;

    // Total size unknown
    if (Total < 0)
    {
      if (Diff >= 0)
      {
        OperationProgress->AddTransferred(Diff);
      }
      else
      {
        // Session total has been reset. A new stream started
        OperationProgress->AddTransferred(Progress);
      }
    }
    else
    {
      OperationProgress->SetTransferSize(Total);
      OperationProgress->AddTransferred(Diff);
    }
  }
}

void TWebDAVFileSystem::NeonDebug(UnicodeString Message)
{
  FTerminal->LogEvent(Message);
}

void TWebDAVFileSystem::InitSslSession(ssl_st *Ssl, ne_session *Session)
{
  TWebDAVFileSystem *FileSystem =
    static_cast<TWebDAVFileSystem *>(ne_get_session_private(Session, SESSION_FS_KEY));
  FileSystem->InitSslSessionImpl(Ssl);
}

void TWebDAVFileSystem::InitSslSessionImpl(ssl_st *Ssl) const
{
  // See also CAsyncSslSocketLayer::InitSSLConnection
  TSessionData *Data = FTerminal->GetSessionData();
#define MASK_TLS_VERSION(VERSION, FLAG) ((Data->GetMinTlsVersion() > VERSION) || (Data->GetMaxTlsVersion() < VERSION) ? FLAG : 0)
  int Options =
    MASK_TLS_VERSION(ssl2, SSL_OP_NO_SSLv2) |
    MASK_TLS_VERSION(ssl3, SSL_OP_NO_SSLv3) |
    MASK_TLS_VERSION(tls10, SSL_OP_NO_TLSv1) |
    MASK_TLS_VERSION(tls11, SSL_OP_NO_TLSv1_1) |
    MASK_TLS_VERSION(tls12, SSL_OP_NO_TLSv1_2);
  // SSL_ctrl() with SSL_CTRL_OPTIONS adds flags (not sets)
  SSL_ctrl(Ssl, SSL_CTRL_OPTIONS, Options, nullptr);
}

void TWebDAVFileSystem::GetSupportedChecksumAlgs(TStrings * /*Algs*/)
{
  // NOOP
}

void TWebDAVFileSystem::LockFile(UnicodeString /*AFileName*/, const TRemoteFile *AFile)
{
  ClearNeonError();
  struct ne_lock *Lock = ne_lock_create();
  try__finally
  {
    SCOPE_EXIT
    {
      if (Lock != nullptr)
      {
        ne_lock_destroy(Lock);
      }
    };
    Lock->uri.path = ne_strdup(PathToNeon(FilePath(AFile)));
    Lock->depth = NE_DEPTH_INFINITE;
    Lock->timeout = NE_TIMEOUT_INFINITE;
    Lock->owner = ne_strdup(StrToNeon(FTerminal->TerminalGetUserName()));
    CheckStatus(ne_lock(FNeonSession, Lock));

    {
      TGuard Guard(FNeonLockStoreSection);

      RequireLockStore();

      ne_lockstore_add(FNeonLockStore, Lock);
    }
    // ownership passed
    Lock = nullptr;
  }
  __finally
  {
#if 0
    if (Lock != nullptr)
    {
      ne_lock_destroy(Lock);
    }
#endif // #if 0
  };
}

void TWebDAVFileSystem::RequireLockStore()
{
  // Create store only when needed,
  // to limit the use of cross-thread code in UpdateFromMain
  if (FNeonLockStore == nullptr)
  {
    FNeonLockStore = ne_lockstore_create();
    ne_lockstore_register(FNeonLockStore, FNeonSession);
  }
}

void TWebDAVFileSystem::LockResult(void *UserData, const struct ne_lock *Lock,
  const ne_uri * /*Uri*/, const ne_status * /*Status*/)
{
  // Is NULL on failure (Status is not NULL then)
  if (Lock != nullptr)
  {
    RawByteString &LockToken = *static_cast<RawByteString *>(UserData);
    LockToken = Lock->token;
  }
}

struct ne_lock *TWebDAVFileSystem::FindLock(RawByteString APath) const
{
  ne_uri Uri = {nullptr};
  Uri.path = ToChar(APath);
  return ne_lockstore_findbyuri(FNeonLockStore, &Uri);
}

void TWebDAVFileSystem::DiscardLock(RawByteString APath)
{
  TGuard Guard(FNeonLockStoreSection);
  if (FNeonLockStore != nullptr)
  {
    struct ne_lock *Lock = FindLock(APath);
    if (Lock != nullptr)
    {
      ne_lockstore_remove(FNeonLockStore, Lock);
    }
  }
}

void TWebDAVFileSystem::UnlockFile(UnicodeString AFileName, const TRemoteFile *AFile)
{
  ClearNeonError();
  struct ne_lock *Lock = ne_lock_create();
  try__finally
  {
    SCOPE_EXIT
    {
      ne_lock_destroy(Lock);
    };
    RawByteString Path = PathToNeon(FilePath(AFile));
    RawByteString LockToken;

    struct ne_lock *Lock2 = nullptr;

    {
      TGuard Guard(FNeonLockStoreSection);
      if (FNeonLockStore != nullptr)
      {
        Lock2 = FindLock(Path);
      }
    }

    // we are not aware of the file being locked,
    // though it can be locked from another (previous and already closed)
    // session, so query the server.
    if (Lock2 == nullptr)
    {
      CheckStatus(ne_lock_discover(FNeonSession, Path.c_str(), LockResult, &LockToken));
    }

    if ((Lock2 == nullptr) && (LockToken.IsEmpty()))
    {
      throw Exception(FMTLOAD(NOT_LOCKED, AFileName));
    }
    else
    {
      struct ne_lock *Unlock;
      if (Lock2 == nullptr)
      {
        DebugAssert(!LockToken.IsEmpty());
        Unlock = ne_lock_create();
        Unlock->uri.path = ne_strdup(Path.c_str());
        Unlock->token = ne_strdup(LockToken.c_str());
      }
      else
      {
        Unlock = Lock2;
      }
      CheckStatus(ne_unlock(FNeonSession, Unlock));
      if (Lock2 == nullptr)
      {
        ne_lock_free(Unlock);
        ne_lock_destroy(Unlock);
      }

      DiscardLock(Path);
    }
  }
  __finally
  {
#if 0
    ne_lock_destroy(Lock);
#endif // #if 0
  };
}

void TWebDAVFileSystem::UpdateFromMain(TCustomFileSystem *AMainFileSystem)
{
  TWebDAVFileSystem *MainFileSystem = dyn_cast<TWebDAVFileSystem>(AMainFileSystem);
  if (DebugAlwaysTrue(MainFileSystem != nullptr))
  {
    TGuard Guard(FNeonLockStoreSection);
    TGuard MainGuard(MainFileSystem->FNeonLockStoreSection);

    if (FNeonLockStore != nullptr)
    {
      struct ne_lock *Lock;
      while ((Lock = ne_lockstore_first(FNeonLockStore)) != nullptr)
      {
        ne_lockstore_remove(FNeonLockStore, Lock);
      }
    }

    if (DebugAlwaysTrue(MainFileSystem->FNeonLockStore != nullptr))
    {
      RequireLockStore();
      struct ne_lock *Lock = ne_lockstore_first(MainFileSystem->FNeonLockStore);
      while (Lock != nullptr)
      {
        ne_lockstore_add(FNeonLockStore, ne_lock_copy(Lock));
        Lock = ne_lockstore_next(MainFileSystem->FNeonLockStore);
      }
    }
  }
}

TFtps TWebDAVFileSystem::GetFtps() const
{
  return FTerminal->GetSessionData()->GetFtps();
}
