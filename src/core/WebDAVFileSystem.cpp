#include <vcl.h>
#pragma hdrstop

#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <wincrypt.h>
#include <rdestl/set.h>

#ifndef NE_LFS
#define NE_LFS
#endif
#ifndef WINSCP
#define WINSCP
#endif
#include <neon/src/ne_basic.h>
#include <neon/src/ne_auth.h>
#include <neon/src/ne_props.h>
#include <neon/src/ne_uri.h>
#include <neon/src/ne_session.h>
#include <neon/src/ne_request.h>
#include <neon/src/ne_xml.h>
#include <neon/src/ne_redirect.h>
#include <neon/src/ne_xmlreq.h>
#include <neon/src/ne_locks.h>
#include <expat/lib/expat.h>

/*#include <apr_hash.h>
#include <apr_strings.h>
#include <apr_tables.h>
#include <apr_file_io.h>
#include <apr_portable.h>
#include <apr_atomic.h>

#include <neon/src/ne_compress.h>
#include <neon/src/ne_defs.h>
#include <neon/src/ne_utils.h>
#include <neon/src/ne_pkcs11.h>*/


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
#include <StrUtils.hpp>
#include <NeonIntf.h>
//#include <openssl/ssl.h>

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
static const char CertificateStorageKey[] = "HttpsCertificates";
#define CONST_WEBDAV_PROTOCOL_BASE_NAME L"WebDAV"

#define DAV_PROP_NAMESPACE "DAV:"
#define MODDAV_PROP_NAMESPACE "http://apache.org/dav/props/"
#define PROP_CONTENT_LENGTH "getcontentlength"
#define PROP_LAST_MODIFIED "getlastmodified"
#define PROP_RESOURCE_TYPE "resourcetype"
#define PROP_HIDDEN "ishidden"
#define PROP_QUOTA_AVAILABLE "quota-available-bytes"
#define PROP_QUOTA_USED "quota-used-bytes"
#define PROP_EXECUTABLE "executable"
#define PROP_OWNER "owner"

static std::unique_ptr<TCriticalSection> DebugSection(new TCriticalSection);
static rde::set<TWebDAVFileSystem *> FileSystems;

extern "C"
{

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

    UnicodeString Message = UTFMessage;

    if (DoLog)
    {
      // Note that this gets called for THttp sessions too.
      // It does no harm atm.
      TWebDAVFileSystem * FileSystem = NULL;
      if (Context != NULL)
      {
        ne_session * Session = static_cast<ne_session *>(Context);

        FileSystem =
          static_cast<TWebDAVFileSystem *>(ne_get_session_private(Session, SESSION_FS_KEY));
      }
      else
      {
        TGuard Guard(*DebugSection.get());

        if (FileSystems.size() == 1)
        {
          FileSystem = *FileSystems.begin();
        }
      }

      if (FileSystem != NULL)
      {
        FileSystem->NeonDebug(Message);
      }
    }
  }
}

} // extern "C"


// ne_path_escape returns 7-bit string, so it does not really matter if we use
// AnsiString or UTF8String here, though UTF8String might be more safe
static AnsiString PathEscape(const char * Path)
{
  char * EscapedPath = ne_path_escape(Path);
  AnsiString Result = EscapedPath;
  ne_free(EscapedPath);
  return Result;
}

static UTF8String PathUnescape(const char * Path)
{
  char * UnescapedPath = ne_path_unescape(Path);
  UTF8String Result(UnescapedPath, UnescapedPath ? strlen(UnescapedPath) : 0);
  ne_free(UnescapedPath);
  return Result;
}

#define AbsolutePathToNeon(P) PathEscape(StrToNeon(P)).c_str()
#define PathToNeonStatic(THIS, P) AbsolutePathToNeon((THIS)->AbsolutePath(P, false))
#define PathToNeon(P) PathToNeonStatic(this, P)

static bool NeonInitialized = false;
static bool NeonSspiInitialized = false;

void NeonInitialize()
{
  // Even if this fails, we do not want to interrupt WinSCP starting for that.
  // We may possibly remember that and fail opening session later.
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
  return FORMAT(L"%d.%d.%d", XML_MAJOR_VERSION, XML_MINOR_VERSION, XML_MICRO_VERSION);
}

TWebDAVFileSystem::TWebDAVFileSystem(TTerminal * ATerminal) :
  TCustomFileSystem(ATerminal),
  FFileList(nullptr),
  FOnCaptureOutput(nullptr),
  FIgnoreAuthenticationFailure(iafNo),
  FPortNumber(0),
  FStoredPasswordTried(false),
  FPasswordFailed(false),
  FActive(false),
  FFileTransferAbort(ftaNone),
  FIgnoreFileList(false),
  FFileTransferCancelled(false),
  FFileTransferResumed(0),
  FFileTransferPreserveTime(false),
  FHasTrailingSlash(false),
  FNeonSession(NULL),
  FNeonLockStore(NULL),
  FNeonLockStoreSection(new TCriticalSection()),
  FUploading(false),
  FDownloading(false),
  FInitialHandshake(false),
  FIgnoreAuthenticationFailure(iafNo),
  FFileTransferCPSLimit(0),
  FLastReadDirectoryProgress(0),
  FCurrentOperationProgress(nullptr),
  webdav_pool(nullptr),
  FSession(nullptr)
{
}

void TWebDAVFileSystem::Init(void *)
{
  FFileSystemInfo.ProtocolBaseName = CONST_WEBDAV_PROTOCOL_BASE_NAME;
  FFileSystemInfo.ProtocolName = FFileSystemInfo.ProtocolBaseName;

  /*if (apr_initialize() != APR_SUCCESS)
    throw ExtException(UnicodeString(L"Cannot init APR"));
  apr_pool_create(&webdav_pool, nullptr);*/
}

TWebDAVFileSystem::~TWebDAVFileSystem()
{
  /*webdav_pool_destroy(webdav_pool);
  apr_terminate();
  webdav_pool = nullptr;
  ne_sock_exit();*/
  UnregisterFromDebug();

  {
    TGuard Guard(FNeonLockStoreSection);
    if (FNeonLockStore != NULL)
    {
      ne_lockstore_destroy(FNeonLockStore);
      FNeonLockStore = NULL;
    }
  }

  delete FNeonLockStoreSection;
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

  TSessionData * Data = FTerminal->GetSessionData();

  FSessionInfo.LoginTime = Now();

  UnicodeString HostName = Data->HostNameExpanded;
  size_t Port = Data->PortNumber;
  UnicodeString ProtocolName = (FTerminal->SessionData->Ftps == ftpsNone) ? WebDAVProtocol : WebDAVSProtocol;
  UnicodeString Path = Data->RemoteDirectory;
  // PathToNeon is not used as we cannot call AbsolutePath here
  UnicodeString EscapedPath = StrFromNeon(PathEscape(StrToNeon(Path)).c_str());
  UnicodeString Url = FORMAT(L"%s://%s:%d%s", (ProtocolName, HostName, Port, EscapedPath));

  FTerminal->Information(LoadStr(STATUS_CONNECT), true);
  FActive = false;
  try
  {
    OpenUrl(Url);
  }
  catch (Exception & E)
  {
    CloseNeonSession();
    FTerminal->Closed();
    FTerminal->FatalError(&E, LoadStr(CONNECTION_FAILED));
  }
  FActive = true;
}
//---------------------------------------------------------------------------
UnicodeString TWebDAVFileSystem::ParsePathFromUrl(const UnicodeString & Url)
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
//---------------------------------------------------------------------------
void TWebDAVFileSystem::OpenUrl(const UnicodeString & Url)
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
//---------------------------------------------------------------------------
void TWebDAVFileSystem::NeonClientOpenSessionInternal(UnicodeString & CorrectedUrl, UnicodeString Url)
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
//---------------------------------------------------------------------------
void TWebDAVFileSystem::NeonOpen(UnicodeString & CorrectedUrl, const UnicodeString & Url)
{
  ne_uri uri;
  NeonParseUrl(Url, uri);

  FHostName = StrFromNeon(uri.host);
  FPortNumber = uri.port;

  FSessionInfo.CSCipher = UnicodeString();
  FSessionInfo.SCCipher = UnicodeString();
  bool Ssl = IsTlsUri(uri);
  FSessionInfo.SecurityProtocolName = Ssl ? LoadStr(FTPS_IMPLICIT) : UnicodeString();

  if (Ssl != (FTerminal->SessionData->Ftps != ftpsNone))
  {
    FTerminal->LogEvent(FORMAT(L"Warning: %s", (LoadStr(UNENCRYPTED_REDIRECT))));
  }

  TSessionData * Data = FTerminal->SessionData;

  DebugAssert(FNeonSession == NULL);
  FNeonSession =
    CreateNeonSession(
      uri, Data->ProxyMethod, Data->ProxyHost, Data->ProxyPort,
      Data->ProxyUsername, Data->ProxyPassword);

  UTF8String Path = uri.path;
  ne_uri_free(&uri);
  ne_set_session_private(FNeonSession, SESSION_FS_KEY, this);

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
    FLAGMASK(Configuration->LogSensitive, NE_DBG_HTTPPLAIN);

  ne_set_read_timeout(FNeonSession, Data->Timeout);

  ne_set_connect_timeout(FNeonSession, Data->Timeout);

  unsigned int NeonAuthTypes = NE_AUTH_BASIC | NE_AUTH_DIGEST;
  if (Ssl)
  {
    NeonAuthTypes |= NE_AUTH_NEGOTIATE;
  }
  ne_add_server_auth(FNeonSession, NeonAuthTypes, NeonRequestAuth, this);

  if (Ssl)
  {
    SetNeonTlsInit(FNeonSession, InitSslSession);

    // When the CA certificate or server certificate has
    // verification problems, neon will call our verify function before
    // outright rejection of the connection.
    ne_ssl_set_verify(FNeonSession, NeonServerSSLCallback, this);

    ne_ssl_trust_default_ca(FNeonSession);

    ne_ssl_provide_clicert(FNeonSession, NeonProvideClientCert, this);
  }

  ne_set_notifier(FNeonSession, NeonNotifier, this);
  ne_hook_create_request(FNeonSession, NeonCreateRequest, this);
  ne_hook_pre_send(FNeonSession, NeonPreSend, this);
  ne_hook_post_send(FNeonSession, NeonPostSend, this);

  TAutoFlag Flag(FInitialHandshake);
  ExchangeCapabilities(Path.c_str(), CorrectedUrl);
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TWebDAVFileSystem::GetRedirectUrl()
{
  UnicodeString Result = GetNeonRedirectUrl(FNeonSession);
  FTerminal->LogEvent(FORMAT(L"Redirected to \"%s\".", (Result)));
  return Result;
}
//---------------------------------------------------------------------------
void TWebDAVFileSystem::ExchangeCapabilities(const char * Path, UnicodeString & CorrectedUrl)
{
  ClearNeonError();
  int NeonStatus = ne_options2(FNeonSession, Path, &FCapabilities);
  if (NeonStatus == NE_REDIRECT)
  {
    CorrectedUrl = GetRedirectUrl();
  }
  else if (NeonStatus == NE_OK)
  {
    if (FCapabilities > 0)
    {
      UnicodeString Str;
      unsigned int Capability = 0x01;
      unsigned int Capabilities = FCapabilities;
      while (Capabilities > 0)
      {
        if (FLAGSET(Capabilities, Capability))
        {
          AddToList(Str, StrFromNeon(ne_capability_name(Capability)), L", ");
          Capabilities -= Capability;
        }
        Capability <<= 1;
      }
      FTerminal->LogEvent(FORMAT(L"Server capabilities: %s", (Str)));
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
//---------------------------------------------------------------------------
void TWebDAVFileSystem::CloseNeonSession()
{
  if (FNeonSession != NULL)
  {
    DestroyNeonSession(FNeonSession);
    FNeonSession = NULL;
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
  TGuard Guard(DebugSection.get());
  FileSystems.insert(this);
}

void TWebDAVFileSystem::UnregisterFromDebug()
{
  TGuard Guard(DebugSection.get());
  FileSystems.erase(this);
}

bool TWebDAVFileSystem::GetActive()
{
  return FActive;
}

void TWebDAVFileSystem::CollectUsage()
{
  if (!FTlsVersionStr.IsEmpty())
  {
    FTerminal->CollectTlsUsage(FTlsVersionStr);
  }

  if (!FTerminal->SessionData->TlsCertificateFile.IsEmpty())
  {
    Configuration->Usage->Inc(L"OpenedSessionsWebDAVSCertificate");
  }

  UnicodeString RemoteSystem = FFileSystemInfo.RemoteSystem;
  if (ContainsText(RemoteSystem, L"Microsoft-IIS"))
  {
    FTerminal->Configuration->Usage->Inc(L"OpenedSessionsWebDAVIIS");
  }
  else if (ContainsText(RemoteSystem, L"IT Hit WebDAV Server"))
  {
    FTerminal->Configuration->Usage->Inc(L"OpenedSessionsWebDAVITHit");
  }
  // e.g. brickftp.com
  else if (ContainsText(RemoteSystem, L"nginx"))
  {
    FTerminal->Configuration->Usage->Inc(L"OpenedSessionsWebDAVNginx");
  }
  else
  {
    // We also know OpenDrive, Yandex, iFiles (iOS), Swapper (iOS), SafeSync
    FTerminal->Configuration->Usage->Inc(L"OpenedSessionsWebDAVOther");
  }
}

const TSessionInfo & TWebDAVFileSystem::GetSessionInfo() const
{
  return FSessionInfo;
}

const TFileSystemInfo & TWebDAVFileSystem::GetFileSystemInfo(bool /*Retrieve*/)
{
  return FFileSystemInfo;
}

bool TWebDAVFileSystem::TemporaryTransferFile(const UnicodeString & /*AFileName*/)
{
  return false;
}

bool TWebDAVFileSystem::GetStoredCredentialsTried() const
{
  return FStoredPasswordTried;
}

UnicodeString TWebDAVFileSystem::FSGetUserName() const
{
  return FUserName;
}

void TWebDAVFileSystem::Idle()
{
  // TODO: Keep session alive
  // noop
}

UnicodeString TWebDAVFileSystem::GetAbsolutePath(const UnicodeString & APath, bool /*Local*/) const
{
  bool AddTrailingBackslash;

  if (Path == L"/")
  {
    // does not really matter as path "/" is still "/" when absolute,
    // no slash needed
    AddTrailingBackslash = FHasTrailingSlash;
  }
  else
  {
    AddTrailingBackslash = (Path[Path.Length()] == L'/');
  }

  UnicodeString Result = core::AbsolutePath(GetCurrDirectory(), APath);
  // We must preserve trailing slash, because particularly for mod_dav,
  // it really matters if the slash in there or not
  if (AddTrailingBackslash)
  {
    Result = UnixIncludeTrailingBackslash(Result);
  }

  return Result;
}

UnicodeString TWebDAVFileSystem::GetAbsolutePath(const UnicodeString & APath, bool Local)
{
  return static_cast<const TWebDAVFileSystem *>(this)->GetAbsolutePath(APath, Local);
}

void TWebDAVFileSystem::GetSupportedChecksumAlgs(TStrings * /*Algs*/)
{
  // NOOP
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
    case fcRemoteCopy:
    case fcPreservingTimestampDirs:
      return false;

    case fcLocking:
      return FLAGSET(FCapabilities, NE_CAP_DAV_CLASS2);

    default:
      DebugFail();
      return false;
  }
}

void TWebDAVFileSystem::EnsureLocation()
{
  if (!FCachedDirectoryChange.IsEmpty())
  {
    FTerminal->LogEvent(FORMAT(L"Locating to cached directory \"%s\".",
      FCachedDirectoryChange.c_str()));
    UnicodeString Directory = FCachedDirectoryChange;
    FCachedDirectoryChange.Clear();
    try
    {
      ChangeDirectory(Directory);
    }
    catch (...)
    {
      // when location to cached directory fails, pretend again
      // location in cached directory
      // here used to be check (CurrentDirectory != Directory), but it is
      // false always (current directory is already set to cached directory),
      // making the condition below useless. check removed.
      if (FTerminal->GetActive())
      {
        FCachedDirectoryChange = Directory;
      }
      throw;
    }
  }
}

UnicodeString TWebDAVFileSystem::GetCurrDirectory() const
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
  FAuthenticationRequested = false;
  ne_set_error(FNeonSession, "");
}

UnicodeString TWebDAVFileSystem::GetNeonError() const
{
  return ::GetNeonError(FNeonSession);
}

void TWebDAVFileSystem::CheckStatus(int NeonStatus)
{
  if ((NeonStatus == NE_ERROR) && FCancelled)
  {
    FCancelled = false;
    Abort();
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

void TWebDAVFileSystem::AnnounceFileListOperation()
{
  // noop
}

UnicodeString __fastcall TWebDAVFileSystem::DirectoryPath(UnicodeString Path)
{
  if (FHasTrailingSlash)
  {
    Path = ::UnixIncludeTrailingBackslash(Path);
  }
  return Path;
}

UnicodeString TWebDAVFileSystem::FilePath(const TRemoteFile * AFile)
{
  UnicodeString Result = File->FullFileName;
  if (File->IsDirectory)
  {
    Result = DirectoryPath(Result);
  }
  return Result;
}

void TWebDAVFileSystem::TryOpenDirectory(const UnicodeString & Directory)
{
  UnicodeString Directory2 = DirectoryPath(Directory);
  FTerminal->LogEvent(FORMAT(L"Trying to open directory \"%s\".", Directory2.c_str());
  TRemoteFile * File;
  ReadFile(Directory2, File);
  delete File;
}

void TWebDAVFileSystem::AnnounceFileListOperation()
{
  // noop
}

void TWebDAVFileSystem::DoChangeDirectory(const UnicodeString & /*Directory*/)
{
}

void TWebDAVFileSystem::ChangeDirectory(const UnicodeString & ADirectory)
{
  UnicodeString Path = AbsolutePath(ADirectory, false);

  // to verify existence of directory try to open it
  TryOpenDirectory(Path);

  // if open dir did not fail, directory exists -> success.
  FCachedDirectoryChange = Path;
}

void TWebDAVFileSystem::CachedChangeDirectory(const UnicodeString & Directory)
{
  FCachedDirectoryChange = core::UnixExcludeTrailingBackslash(Directory);
}

struct TReadFileData
{
  TWebDAVFileSystem * FileSystem;
  TRemoteFile * File;
  TRemoteFileList * FileList;
};

int TWebDAVFileSystem::ReadDirectoryInternal(
  const UnicodeString & Path, TRemoteFileList * FileList)
{
  TReadFileData Data;
  Data.FileSystem = this;
  Data.File = NULL;
  Data.FileList = FileList;
  ClearNeonError();
  ne_propfind_handler * PropFindHandler = ne_propfind_create(FNeonSession, PathToNeon(Path), NE_DEPTH_ONE);
  void * DiscoveryContext = ne_lock_register_discovery(PropFindHandler);
  int Result;
  try
  {
    Result = ne_propfind_allprop(PropFindHandler, NeonPropsResult, &Data);
  }
  __finally
  {
    ne_lock_discovery_free(DiscoveryContext);
    ne_propfind_destroy(PropFindHandler);
  }
  return Result;
}

bool TWebDAVFileSystem::IsValidRedirect(int NeonStatus, UnicodeString & Path)
{
  bool Result = (NeonStatus == NE_REDIRECT);
  if (Result)
  {
    // What PathToNeon does
    UnicodeString OriginalPath = AbsolutePath(Path, false);
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
      Path = RedirectPath;
    }
  }

  return Result;
}

void TWebDAVFileSystem::DoReadDirectory(TRemoteFileList * FileList)
{
  FileList->Reset();
  // add parent directory
  FileList->AddFile(new TRemoteParentDirectory(FTerminal));

  FLastReadDirectoryProgress = 0;

  TWebDAVFileListHelper Helper(this, FileList, false);

  // always specify path to list, do not attempt to
  // list "current" dir as:
  // 1) List() lists again the last listed directory, not the current working directory
  // 2) we handle this way the cached directory change
  UnicodeString Directory = GetAbsolutePath(FileList->GetDirectory(), false);
  if (FHasTrailingSlash)
    Directory = core::UnixIncludeTrailingBackslash(Directory);
  WebDAVGetList(Directory);
}

void TWebDAVFileSystem::ReadDirectory(TRemoteFileList * FileList)
{
  /*DebugAssert(FileList);
  webdav::cancelled = 0;
  bool Repeat = false;

  do
  {
    Repeat = false;
    try
    {
      DoReadDirectory(FileList);
    }
    catch (Exception &)
    {
      if (!FTerminal->GetActive())
      {
        FTerminal->Reopen(ropNoReadDirectory);
        Repeat = true;
      }
      else
      {
        throw;
      }
    }
  }
  while (Repeat);*/
  UnicodeString Path = DirectoryPath(FileList->Directory);
  TOperationVisualizer Visualizer(FTerminal->UseBusyCursor);

  int NeonStatus = ReadDirectoryInternal(Path, FileList);
  if (IsValidRedirect(NeonStatus, Path))
  {
    NeonStatus = ReadDirectoryInternal(Path, FileList);
  }
  CheckStatus(NeonStatus);
}

void TWebDAVFileSystem::ReadSymlink(TRemoteFile * SymlinkFile,
  TRemoteFile *& File)
{
  // CustomReadFile(SymlinkFile->GetLinkTo(), File, SymlinkFile);
  // we never set SymLink flag, so we should never get here
  DebugFail();
}

void TWebDAVFileSystem::ReadFile(const UnicodeString & AFileName,
  TRemoteFile *& File)
{
  CustomReadFile(AFileName, File, nullptr);
}

void TWebDAVFileSystem::NeonPropsResult(
  void * UserData, const ne_uri * Uri, const ne_prop_result_set * Results)
{
  UnicodeString Path = StrFromNeon(PathUnescape(Uri->path).c_str());

  TReadFileData & Data = *static_cast<TReadFileData *>(UserData);
  if (Data.FileList != NULL)
  {
    UnicodeString FileListPath = Data.FileSystem->AbsolutePath(Data.FileList->Directory, false);
    if (UnixSamePath(Path, FileListPath))
    {
      Path = UnixIncludeTrailingBackslash(UnixIncludeTrailingBackslash(Path) + L"..");
    }
    std::unique_ptr<TRemoteFile> File(new TRemoteFile(NULL));
    File->Terminal = Data.FileSystem->FTerminal;
    Data.FileSystem->ParsePropResultSet(File.get(), Path, Results);
    Data.FileList->AddFile(File.release());
  }
  else
  {
    Data.FileSystem->ParsePropResultSet(Data.File, Path, Results);
  }
}
//---------------------------------------------------------------------------
const char * __fastcall TWebDAVFileSystem::GetProp(
  const ne_prop_result_set * Results, const char * Name, const char * NameSpace)
{
  ne_propname Prop;
  Prop.nspace = (NameSpace == NULL) ? DAV_PROP_NAMESPACE : NameSpace;
  Prop.name = Name;
  return ne_propset_value(Results, &Prop);
}

void TWebDAVFileSystem::ParsePropResultSet(TRemoteFile * File,
  const UnicodeString & Path, const ne_prop_result_set * Results)
{
  File->FullFileName = UnixExcludeTrailingBackslash(Path);
  // Some servers do not use DAV:collection tag, but indicate the folder by trailing slash only.
  // It seems that all servers actually use the trailing slash, including IIS, mod_Dav, IT Hit, OpenDrive, etc.
  bool Collection = (File->FullFileName != Path);
  File->FileName = UnixExtractFileName(File->FullFileName);
  const char * ContentLength = GetProp(Results, PROP_CONTENT_LENGTH);
  // some servers, for example iFiles, do not provide "getcontentlength" for folders
  if (ContentLength != NULL)
  {
    File->Size = StrToInt64Def(ContentLength, 0);
  }
  const char * LastModified = GetProp(Results, PROP_LAST_MODIFIED);
  if (DebugAlwaysTrue(LastModified != NULL))
  {
    char WeekDay[4] = { L'\0' };
    int Year = 0;
    char MonthStr[4] = { L'\0' };
    int Day = 0;
    int Hour = 0;
    int Min = 0;
    int Sec = 0;
    #define RFC1123_FORMAT "%3s, %02d %3s %4d %02d:%02d:%02d GMT"
    int Filled =
      sscanf(LastModified, RFC1123_FORMAT, WeekDay, &Day, MonthStr, &Year, &Hour, &Min, &Sec);
    // we need at least a complete date
    if (Filled >= 4)
    {
      int Month = ParseShortEngMonthName(MonthStr);
      if (Month >= 1)
      {
        TDateTime Modification =
          EncodeDateVerbose((unsigned short)Year, (unsigned short)Month, (unsigned short)Day) +
          EncodeTimeVerbose((unsigned short)Hour, (unsigned short)Min, (unsigned short)Sec, 0);
        File->Modification = ConvertTimestampFromUTC(Modification);
        File->ModificationFmt = mfFull;
      }
    }
  }

  // optimization
  if (!Collection)
  {
    // This is possibly redundant code as all servers we know (see a comment above)
    // indicate the folder by trailing slash too
    const char * ResourceType = GetProp(Results, PROP_RESOURCE_TYPE);
    if (ResourceType != NULL)
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

  File->Type = Collection ? FILETYPE_DIRECTORY : FILETYPE_DEFAULT;
  // this is MS extension (draft-hopmann-collection-props-00)
  const char * IsHidden = GetProp(Results, PROP_HIDDEN);
  if (IsHidden != NULL)
  {
    File->IsHidden = (StrToIntDef(IsHidden, 0) != 0);
  }

  const char * Owner = GetProp(Results, PROP_OWNER);
  if (Owner != NULL)
  {
    File->Owner.Name = Owner;
  }

  const UnicodeString RightsDelimiter(L", ");
  UnicodeString HumanRights;

  // Proprietary property of mod_dav
  // http://www.webdav.org/mod_dav/#imp
  const char * Executable = GetProp(Results, PROP_EXECUTABLE, MODDAV_PROP_NAMESPACE);
  if (Executable != NULL)
  {
    if (strcmp(Executable, "T") == NULL)
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

  struct ne_lock * Lock = static_cast<struct ne_lock *>(ne_propset_private(Results));
  if ((Lock != NULL) && (Lock->token != NULL))
  {
    UnicodeString Owner;
    if (Lock->owner != NULL)
    {
      Owner = StrFromNeon(Lock->owner).Trim();
    }
    UnicodeString LockRights;
    if (IsWin8())
    {
      // The "lock" character is supported since Windows 8
      LockRights = L"\uD83D\uDD12" + Owner;
    }
    else
    {
      LockRights = LoadStr(LOCKED);
      if (!Owner.IsEmpty())
      {
        LockRights = FORMAT(L"%s (%s)", (LockRights, Owner));
      }
    }

    AddToList(HumanRights, LockRights, RightsDelimiter);
  }

  File->HumanRights = HumanRights;
}

int TWebDAVFileSystem::CustomReadFileInternal(const UnicodeString & AFileName,
  TRemoteFile *& AFile, TRemoteFile * ALinkedByFile)
{
  std::unique_ptr<TRemoteFile> File(new TRemoteFile(ALinkedByFile));
  TReadFileData Data;
  Data.FileSystem = this;
  Data.File = File.get();
  Data.FileList = NULL;
  ClearNeonError();
  int Result =
    ne_simple_propfind(FNeonSession, PathToNeon(FileName), NE_DEPTH_ZERO, NULL,
      NeonPropsResult, &Data);
  if (Result == NE_OK)
  {
    AFile = File.release();
  }
  return Result;
}

void TWebDAVFileSystem::CustomReadFile(const UnicodeString & AFileName,
  TRemoteFile *& AFile, TRemoteFile * ALinkedByFile)
{
  /*File = nullptr;
  int IsDir = 0;
  bool isExist = WebDAVCheckExisting(AFileName.c_str(), IsDir);
  if (isExist)
  {
    File = new TRemoteFile();
    if (IsDir)
      File->SetType(FILETYPE_DIRECTORY);
  }*/
  TOperationVisualizer Visualizer(FTerminal->UseBusyCursor);

  int NeonStatus = CustomReadFileInternal(FileName, File, ALinkedByFile);
  if (IsValidRedirect(NeonStatus, FileName))
  {
    NeonStatus = CustomReadFileInternal(FileName, File, ALinkedByFile);
  }
  CheckStatus(NeonStatus);
}

void TWebDAVFileSystem::RemoteDeleteFile(const UnicodeString & /*AFileName*/,
  const TRemoteFile * AFile, intptr_t Params, TRmSessionAction & /*Action*/)
{
  /*DebugUsedParam(AFile);
  DebugUsedParam(Params);
  UnicodeString FullFileName = AFile->GetFullFileName();
  bool res = WebDAVDeleteFile(FullFileName.c_str());
  if (!res)
  {
    ThrowSkipFileNull();
  }*/
  Action.Recursive();
  ClearNeonError();
  TOperationVisualizer Visualizer(FTerminal->UseBusyCursor);
  RawByteString Path = PathToNeon(FilePath(File));
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

int __fastcall TWebDAVFileSystem::RenameFileInternal(const UnicodeString & FileName,
  const UnicodeString & NewName)
{
  // 0 = no overwrite
  return ne_move(FNeonSession, 0, PathToNeon(FileName), PathToNeon(NewName));
}

void TWebDAVFileSystem::RemoteRenameFile(const UnicodeString & AFileName,
  const UnicodeString & ANewName)
{
  /*UnicodeString FullFileName = core::UnixIncludeTrailingBackslash(FCurrentDirectory) + AFileName;
  bool res = WebDAVRenameFile(FullFileName.c_str(), ANewName.c_str());
  if (!res)
  {
    ThrowSkipFileNull();
  }*/
  ClearNeonError();
  TOperationVisualizer Visualizer(FTerminal->UseBusyCursor);

  UnicodeString Path = FileName;
  int NeonStatus = RenameFileInternal(Path, NewName);
  if (IsValidRedirect(NeonStatus, Path))
  {
    NeonStatus = RenameFileInternal(Path, NewName);
  }
  CheckStatus(NeonStatus);
  // See a comment in DeleteFile
  DiscardLock(PathToNeon(Path));
}

void TWebDAVFileSystem::RemoteCopyFile(const UnicodeString & /*AFileName*/,
  const UnicodeString & /*ANewName*/)
{
  DebugFail();
  Error(SNotImplemented, 1012);
}

void TWebDAVFileSystem::RemoteCreateDirectory(const UnicodeString & ADirName)
{
  /*UnicodeString FullDirName = GetAbsolutePath(ADirName, true);
  bool res = WebDAVMakeDirectory(FullDirName.c_str());
  if (!res)
  {
    TStringList Strings;
    Strings.SetDelimiter(L'/');
    Strings.SetDelimitedText(ADirName);
    UnicodeString CurDir;
    for (intptr_t Index = 0; Index < Strings.GetCount(); ++Index)
    {
      if (Strings.GetString(Index).IsEmpty())
      {
        continue;
      }
      CurDir += L"/" + Strings.GetString(Index);
      res = WebDAVMakeDirectory(CurDir.c_str());
    }
    if (!res)
    {
      ThrowSkipFile(nullptr, L"");
    }
  }*/
  ClearNeonError();
  TOperationVisualizer Visualizer(FTerminal->UseBusyCursor);
  CheckStatus(ne_mkcol(FNeonSession, PathToNeon(DirName)));
}

void TWebDAVFileSystem::CreateLink(const UnicodeString & /*AFileName*/,
  const UnicodeString & /*PointTo*/, bool /*Symbolic*/)
{
  DebugFail();
  Error(SNotImplemented, 1014);
}

void TWebDAVFileSystem::ChangeFileProperties(const UnicodeString & /*AFileName*/,
  const TRemoteFile * /*AFile*/, const TRemoteProperties * /*Properties*/,
  TChmodSessionAction & /*Action*/)
{
  DebugFail();
  Error(SNotImplemented, 1006);
}

bool TWebDAVFileSystem::LoadFilesProperties(TStrings * /*FileList*/)
{
  DebugFail();
  return false;
}

void TWebDAVFileSystem::CalculateFilesChecksum(const UnicodeString & /*Alg*/,
  TStrings * /*FileList*/, TStrings * /*Checksums*/,
  TCalculatedChecksumEvent /*OnCalculatedChecksum*/)
{
  DebugFail();
}

bool TWebDAVFileSystem::ConfirmOverwrite(
  const UnicodeString & ASourceFullFileName, UnicodeString & ADestFileName,
  TFileOperationProgressType * OperationProgress,
  const TOverwriteFileParams * FileParams,
  const TCopyParamType * CopyParam, intptr_t Params,
  bool AutoResume,
  OUT TOverwriteMode & OverwriteMode,
  OUT uintptr_t & Answer)
{
  /*bool CanAutoResume = FLAGSET(Params, cpNoConfirmation) && AutoResume;
  bool CanResume = false; // disable resume

  Answer = 0;
  if (CanAutoResume && CanResume)
  {
    Answer = qaRetry;
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
    TQueryButtonAlias Aliases[3];
    Aliases[0].Button = qaRetry;
    Aliases[0].Alias = LoadStr(RESUME_BUTTON);
    Aliases[1].Button = qaAll;
    Aliases[1].Alias = LoadStr(YES_TO_NEWER_BUTTON);
    Aliases[2].Button = qaIgnore;
    Aliases[2].Alias = LoadStr(RENAME_BUTTON);
    TQueryParams QueryParams(qpNeverAskAgainCheck);
    QueryParams.Aliases = Aliases;
    QueryParams.AliasesCount = _countof(Aliases);

    {
      TSuspendFileOperationProgress Suspend(OperationProgress);
      Answer = FTerminal->ConfirmFileOverwrite(
         ASourceFullFileName, ADestFileName, FileParams,
         Answers, &QueryParams,
         OperationProgress->Side == osLocal ? osRemote : osLocal,
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
      FFileTransferResumed = FileParams->DestSize;
      break;

      // rename
    case qaIgnore:
      if (FTerminal->PromptUser(FTerminal->GetSessionData(), pkFileName,
            LoadStr(RENAME_TITLE), L"", LoadStr(RENAME_PROMPT2), true, 0, ADestFileName))
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

    case qaNo:
      FFileTransferAbort = ftaSkip;
      Result = false;
      break;

    case qaCancel:
      if (!OperationProgress->Cancel)
      {
        OperationProgress->Cancel = csCancel;
      }
      FFileTransferAbort = ftaCancel;
      Result = false;
      break;

    default:
      DebugAssert(false);
      Result = false;
      break;
  }
  return Result;*/
  // all = "yes to newer"
  int Answers = qaYes | qaNo | qaCancel | qaYesToAll | qaNoToAll | qaAll;
  TQueryButtonAlias Aliases[3];
  Aliases[0].Button = qaAll;
  Aliases[0].Alias = LoadStr(YES_TO_NEWER_BUTTON);
  Aliases[0].GroupWith = qaYes;
  Aliases[0].GrouppedShiftState = TShiftState() << ssCtrl;
  Aliases[1].Button = qaYesToAll;
  Aliases[1].GroupWith = qaYes;
  Aliases[1].GrouppedShiftState = TShiftState() << ssShift;
  Aliases[2].Button = qaNoToAll;
  Aliases[2].GroupWith = qaNo;
  Aliases[2].GrouppedShiftState = TShiftState() << ssShift;
  TQueryParams QueryParams(qpNeverAskAgainCheck);
  QueryParams.Aliases = Aliases;
  QueryParams.AliasesCount = LENOF(Aliases);

  unsigned int Answer;

  {
    TSuspendFileOperationProgress Suspend(OperationProgress);
    Answer =
      FTerminal->ConfirmFileOverwrite(
        SourceFullFileName, TargetFileName, FileParams, Answers, &QueryParams,
        (OperationProgress->Side == osLocal) ? osRemote : osLocal,
        CopyParam, Params, OperationProgress);
  }

  switch (Answer)
  {
    case qaYes:
      // noop
      break;

    case qaNo:
      THROW_SKIP_FILE_NULL;

    default:
      DebugFail();
    case qaCancel:
      if (!OperationProgress->Cancel)
      {
        OperationProgress->Cancel = csCancel;
      }
      Abort();
      break;
  }
}

void TWebDAVFileSystem::CustomCommandOnFile(const UnicodeString & AFileName,
  const TRemoteFile * AFile, const UnicodeString & Command, intptr_t Params, TCaptureOutputEvent OutputEvent)
{
  /*DebugAssert(AFile);
  bool Dir = AFile->GetIsDirectory() && !AFile->GetIsSymLink();
  if (Dir && (Params & ccRecursive))
  {
    TCustomCommandParams AParams;
    AParams.Command = Command;
    AParams.Params = Params;
    AParams.OutputEvent = OutputEvent;
    FTerminal->ProcessDirectory(AFileName, MAKE_CALLBACK(TTerminal::CustomCommandOnFile, FTerminal), &AParams);
  }

  if (!Dir || (Params & ccApplyToDirectories))
  {
    TCustomCommandData Data(FTerminal);
    UnicodeString Cmd = TRemoteCustomCommand(
      Data, FTerminal->GetCurrDirectory(), AFileName, L"").
      Complete(Command, true);
  }*/
  DebugFail();
}

void TWebDAVFileSystem::AnyCommand(const UnicodeString & /*Command*/,
  TCaptureOutputEvent /*OutputEvent*/)
{
  DebugFail();
  Error(SNotImplemented, 1008);
}

TStrings * TWebDAVFileSystem::GetFixedPaths()
{
  return nullptr;
}

void TWebDAVFileSystem::NeonQuotaResult(
  void * UserData, const ne_uri * /*Uri*/, const ne_prop_result_set * Results)
{
  TSpaceAvailable & SpaceAvailable = *static_cast<TSpaceAvailable *>(UserData);

  const char * Value = GetProp(Results, PROP_QUOTA_AVAILABLE);
  if (Value != NULL)
  {
    SpaceAvailable.UnusedBytesAvailableToUser = StrToInt64(StrFromNeon(Value));

    const char * Value = GetProp(Results, PROP_QUOTA_USED);
    if (Value != NULL)
    {
      SpaceAvailable.BytesAvailableToUser =
        StrToInt64(StrFromNeon(Value)) + SpaceAvailable.UnusedBytesAvailableToUser;
    }
  }
}

void TWebDAVFileSystem::SpaceAvailable(const UnicodeString & /*APath*/,
  TSpaceAvailable & /*ASpaceAvailable*/)
{
  // RFC4331: http://tools.ietf.org/html/rfc4331

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

  UnicodeString APath = DirectoryPath(Path);

  ne_propname QuotaProps[3];
  memset(QuotaProps, 0, sizeof(QuotaProps));
  QuotaProps[0].nspace = DAV_PROP_NAMESPACE;
  QuotaProps[0].name = PROP_QUOTA_AVAILABLE;
  QuotaProps[1].nspace = DAV_PROP_NAMESPACE;
  QuotaProps[1].name = PROP_QUOTA_USED;
  QuotaProps[2].nspace = NULL;
  QuotaProps[2].name = NULL;

  TOperationVisualizer Visualizer(FTerminal->UseBusyCursor);

  CheckStatus(
    ne_simple_propfind(FNeonSession, PathToNeon(APath), NE_DEPTH_ZERO, QuotaProps,
      NeonQuotaResult, &ASpaceAvailable));
}

void TWebDAVFileSystem::CopyToRemote(const TStrings * AFilesToCopy,
  const UnicodeString & ATargetDir, const TCopyParamType * CopyParam,
  intptr_t Params, TFileOperationProgressType * OperationProgress,
  TOnceDoneOperation & OnceDoneOperation)
{
  DebugAssert((AFilesToCopy != nullptr) && (OperationProgress != nullptr));

  Params &= ~cpAppend;
  UnicodeString FileName, FileNameOnly;
  UnicodeString TargetDir = GetAbsolutePath(ATargetDir, false);
  UnicodeString FullTargetDir = core::UnixIncludeTrailingBackslash(TargetDir);
  intptr_t Index = 0;
  while ((Index < AFilesToCopy->GetCount()) && !OperationProgress->Cancel)
  {
    FileName = AFilesToCopy->GetString(Index);
    TRemoteFile * File = NB_STATIC_DOWNCAST(TRemoteFile, AFilesToCopy->GetObj(Index));
    UnicodeString RealFileName = File ? File->GetFileName() : FileName;
    FileNameOnly = base::ExtractFileName(RealFileName, false);

    // try
    {
      bool Success = false;
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
        WebDAVSourceRobust(FileName, File, FullTargetDir, CopyParam, Params, OperationProgress,
          tfFirstLevel);
        Success = true;
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
    __finally
    {
      OperationProgress->Finish(FileName, Success, OnceDoneOperation);
    }
    ++Index;
  }
}

void TWebDAVFileSystem::WebDAVSourceRobust(const UnicodeString & AFileName,
  const TRemoteFile * AFile,
  const UnicodeString & TargetDir, const TCopyParamType * CopyParam, intptr_t Params,
  TFileOperationProgressType * OperationProgress, uintptr_t Flags)
{
  bool Retry = false;

  TUploadSessionAction Action(FTerminal->GetActionLog());

  do
  {
    Retry = false;
    try
    {
      WebDAVSource(AFileName, AFile, TargetDir, CopyParam, Params, OperationProgress,
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

void TWebDAVFileSystem::WebDAVSource(const UnicodeString & AFileName,
  const TRemoteFile * AFile,
  const UnicodeString & TargetDir, const TCopyParamType * CopyParam, intptr_t Params,
  TFileOperationProgressType * OperationProgress, uintptr_t Flags,
  TUploadSessionAction & Action)
{
  UnicodeString RealFileName = AFile ? AFile->GetFileName() : AFileName;
  Action.SetFileName(::ExpandUNCFileName(RealFileName));

  OperationProgress->SetFile(RealFileName, false);

  if (!FTerminal->AllowLocalFileTransfer(AFileName, CopyParam, OperationProgress))
  {
    FTerminal->LogEvent(FORMAT(L"File \"%s\" excluded from transfer", RealFileName.c_str()));
    ThrowSkipFileNull();
  }

  int64_t Size = 0;
  uintptr_t LocalFileAttrs = 0;

  FTerminal->OpenLocalFile(AFileName, GENERIC_READ,
    nullptr, &LocalFileAttrs, nullptr, nullptr, nullptr, &Size);

  OperationProgress->SetFileInProgress();

  bool Dir = FLAGSET(LocalFileAttrs, faDirectory);
  if (Dir)
  {
    Action.Cancel();
    WebDAVDirectorySource(::IncludeTrailingBackslash(AFileName), TargetDir,
      LocalFileAttrs, CopyParam, Params, OperationProgress, Flags);
  }
  else
  {
    UnicodeString DestFileName = CopyParam->ChangeFileName(base::ExtractFileName(RealFileName, false),
      osLocal, FLAGSET(Flags, tfFirstLevel));

    FTerminal->LogEvent(FORMAT(L"Copying \"%s\" to remote directory started.", RealFileName.c_str()));

    OperationProgress->SetLocalSize(Size);

    // Suppose same data size to transfer as to read
    // (not true with ASCII transfer)
    OperationProgress->SetTransferSize(OperationProgress->LocalSize);
    OperationProgress->TransferingFile = false;

    // Will we use ASCII of BINARY file transfer?
    TFileMasks::TParams MaskParams;
    MaskParams.Size = Size;

    ResetFileTransfer();

    TFileTransferData UserData;

    {
      uint32_t TransferType = 2; // OperationProgress->AsciiTransfer = false
      // ignore file list
      TWebDAVFileListHelper Helper(this, nullptr, true);

      FFileTransferCPSLimit = OperationProgress->CPSLimit;
      // not used for uploads anyway
      FFileTransferPreserveTime = CopyParam->GetPreserveTime();
      // not used for uploads, but we get new name (if any) back in this field
      UserData.FileName = DestFileName;
      UserData.Params = Params;
      UserData.AutoResume = FLAGSET(Flags, tfAutoResume);
      UserData.CopyParam = CopyParam;
      FileTransfer(RealFileName, AFileName, DestFileName,
        TargetDir, false, Size, TransferType, UserData, OperationProgress);
    }

    UnicodeString DestFullName = TargetDir + UserData.FileName;
    // only now, we know the final destination
    Action.Destination(DestFullName);
  }

  // TODO : Delete also read-only files.
  if (FLAGSET(Params, cpDelete))
  {
    if (!Dir)
    {
      FileOperationLoopCustom(FTerminal, OperationProgress, True, FMTLOAD(CORE_DELETE_LOCAL_FILE_ERROR, AFileName.c_str()), "",
      [&]()
      {
        THROWOSIFFALSE(::RemoveFile(AFileName));
      });
    }
  }
  else if (CopyParam->GetClearArchive() && FLAGSET(LocalFileAttrs, faArchive))
  {
    FileOperationLoopCustom(FTerminal, OperationProgress, True, FMTLOAD(CANT_SET_ATTRS, AFileName.c_str()), "",
    [&]()
    {
      THROWOSIFFALSE(FTerminal->SetLocalFileAttributes(ApiPath(AFileName), LocalFileAttrs & ~faArchive) == 0);
    });
  }
}

void TWebDAVFileSystem::WebDAVDirectorySource(const UnicodeString & DirectoryName,
  const UnicodeString & TargetDir, uintptr_t Attrs, const TCopyParamType * CopyParam,
  intptr_t Params, TFileOperationProgressType * OperationProgress, uintptr_t Flags)
{
  UnicodeString DestDirectoryName = CopyParam->ChangeFileName(
    base::ExtractFileName(::ExcludeTrailingBackslash(DirectoryName), false), osLocal,
    FLAGSET(Flags, tfFirstLevel));
  UnicodeString DestFullName = core::UnixIncludeTrailingBackslash(TargetDir + DestDirectoryName);
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

  WIN32_FIND_DATA SearchRec;
  bool FindOK = false;
  HANDLE FindHandle = INVALID_HANDLE_VALUE;

  UnicodeString FindPath = DirectoryName + L"*.*";

  FileOperationLoopCustom(FTerminal, OperationProgress, True, FMTLOAD(LIST_DIR_ERROR, DirectoryName.c_str()), "",
  [&]()
  {
    FindHandle = ::FindFirstFile(ApiPath(FindPath).c_str(), &SearchRec);
    FindOK = FindHandle != INVALID_HANDLE_VALUE;
    if (!FindOK)
    {
      FindCheck(::GetLastError(), FindPath);
    }
  });

  bool CreateDir = true;

  {
    SCOPE_EXIT
    {
      ::FindClose(FindHandle);
    };
    while (FindOK && !OperationProgress->Cancel)
    {
      UnicodeString FileName = DirectoryName + SearchRec.cFileName;
      try
      {
        if ((wcscmp(SearchRec.cFileName, THISDIRECTORY) != 0) && (wcscmp(SearchRec.cFileName, PARENTDIRECTORY) != 0))
        {
          WebDAVSourceRobust(FileName, nullptr, DestFullName, CopyParam, Params, OperationProgress,
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

      FileOperationLoopCustom(FTerminal, OperationProgress, True, FMTLOAD(LIST_DIR_ERROR, DirectoryName.c_str()), "",
      [&]()
      {
        FindOK = ::FindNextFile(FindHandle, &SearchRec) != FALSE;
        if (!FindOK)
        {
          ::FindCheck(::GetLastError(), FindPath);
        }
      });
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
      {
        SCOPE_EXIT
        {
          FTerminal->SetExceptionOnFail(false);
        };
        FTerminal->RemoteCreateDirectory(DestFullName, &Properties);
      }
    }
    catch (...)
    {
      TRemoteFile * File = nullptr;
      // ignore non-fatal error when the directory already exists
      UnicodeString fn = core::UnixExcludeTrailingBackslash(DestFullName);
      if (fn.IsEmpty())
      {
        fn = ROOTDIRECTORY;
      }
      bool Rethrow =
        !FTerminal->GetActive() ||
        !FTerminal->FileExists(fn, &File) ||
        (File && !File->GetIsDirectory());
      SAFE_DESTROY(File);
      if (Rethrow)
      {
        throw;
      }
    }
  }

  // TODO : Delete also read-only directories.
  // TODO : Show error message on failure.
  if (!OperationProgress->Cancel)
  {
    if (FLAGSET(Params, cpDelete))
    {
      FTerminal->RemoveLocalDirectory(ApiPath(DirectoryName));
    }
    else if (CopyParam->GetClearArchive() && FLAGSET(Attrs, faArchive))
    {
      FileOperationLoopCustom(FTerminal, OperationProgress, True, FMTLOAD(CANT_SET_ATTRS, DirectoryName.c_str()), "",
      [&]()
      {
        THROWOSIFFALSE(FTerminal->SetLocalFileAttributes(ApiPath(DirectoryName), Attrs & ~faArchive) == 0);
      });
    }
  }
}

void TWebDAVFileSystem::CopyToLocal(const TStrings * AFilesToCopy,
  const UnicodeString & TargetDir, const TCopyParamType * CopyParam,
  intptr_t Params, TFileOperationProgressType * OperationProgress,
  TOnceDoneOperation & OnceDoneOperation)
{
  Params &= ~cpAppend;
  UnicodeString FullTargetDir = ::IncludeTrailingBackslash(TargetDir);

  intptr_t Index = 0;
  while (Index < AFilesToCopy->GetCount() && !OperationProgress->Cancel)
  {
    UnicodeString FileName = AFilesToCopy->GetString(Index);
    const TRemoteFile * File = NB_STATIC_DOWNCAST_CONST(TRemoteFile, AFilesToCopy->GetObj(Index));
    FTerminal->SetExceptionOnFail(true);
    {
      bool Success = false;
      SCOPE_EXIT
      {
        OperationProgress->Finish(FileName, Success, OnceDoneOperation);
        FTerminal->SetExceptionOnFail(false);
      };
      UnicodeString AbsoluteFilePath = GetAbsolutePath(FileName, false);
      UnicodeString TargetDirectory = CreateTargetDirectory(File->GetFileName(), FullTargetDir, CopyParam);
      try
      {
        SinkRobust(AbsoluteFilePath, File, TargetDirectory, CopyParam, Params,
          OperationProgress, tfFirstLevel);
        Success = true;
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

void TWebDAVFileSystem::SinkRobust(const UnicodeString & AFileName,
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
      Sink(AFileName, AFile, TargetDir, CopyParam, Params, OperationProgress,
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
      DebugAssert(AFile != nullptr);
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

void TWebDAVFileSystem::Sink(const UnicodeString & AFileName,
  const TRemoteFile * AFile, const UnicodeString & TargetDir,
  const TCopyParamType * CopyParam, intptr_t Params,
  TFileOperationProgressType * OperationProgress, uintptr_t Flags,
  TDownloadSessionAction & Action)
{
  UnicodeString FileNameOnly = base::UnixExtractFileName(AFileName);

  Action.SetFileName(AFileName);

  DebugAssert(AFile);
  TFileMasks::TParams MaskParams;
  MaskParams.Size = AFile->GetSize();

  if (!CopyParam->AllowTransfer(AFileName, osRemote, AFile->GetIsDirectory(), MaskParams))
  {
    FTerminal->LogEvent(FORMAT(L"File \"%s\" excluded from transfer", AFileName.c_str()));
    ThrowSkipFileNull();
  }

  FTerminal->LogFileDetails(AFileName, TDateTime(), AFile->GetSize());

  OperationProgress->SetFile(FileNameOnly);

  UnicodeString DestFileName = CopyParam->ChangeFileName(base::UnixExtractFileName(AFile->GetFileName()),
    osRemote, FLAGSET(Flags, tfFirstLevel));
  UnicodeString DestFullName = TargetDir + DestFileName;

  if (AFile->GetIsDirectory())
  {
    bool CanProceed = true;
    if (::DirectoryExists(DestFullName))
    {
      uintptr_t Answer = 0;
      UnicodeString Message = FMTLOAD(DIRECTORY_OVERWRITE, FileNameOnly.c_str());
      TQueryParams QueryParams(qpNeverAskAgainCheck);

      {
        TSuspendFileOperationProgress Suspend(OperationProgress);
        Answer = FTerminal->ConfirmFileOverwrite(
          AFileName, DestFileName, nullptr,
          qaYes | qaNo | qaCancel | qaYesToAll | qaNoToAll,
          &QueryParams, osRemote, CopyParam, Params, OperationProgress, Message);
      }
      switch (Answer)
      {
        case qaCancel:
          OperationProgress->Cancel = csCancel; // continue on next case
          // FALLTHROUGH
        case qaNo:
          CanProceed = false;
        default:
          break;
      }
    }
    if (CanProceed)
    {
      Action.Cancel();
      if (!AFile->GetIsSymLink())
      {
        FileOperationLoopCustom(FTerminal, OperationProgress, True, FMTLOAD(NOT_DIRECTORY_ERROR, DestFullName.c_str()), "",
        [&]()
        {
          DWORD LocalFileAttrs = FTerminal->GetLocalFileAttributes(ApiPath(DestFullName));
          if (FLAGCLEAR(LocalFileAttrs, faDirectory))
          {
            ThrowExtException();
          }
        });

        FileOperationLoopCustom(FTerminal, OperationProgress, True, FMTLOAD(CREATE_DIR_ERROR, DestFullName.c_str()), "",
        [&]()
        {
          THROWOSIFFALSE(::ForceDirectories(ApiPath(DestFullName)));
        });

        TSinkFileParams SinkFileParams;
        SinkFileParams.TargetDir = ApiPath(::IncludeTrailingBackslash(DestFullName));
        SinkFileParams.CopyParam = CopyParam;
        SinkFileParams.Params = Params;
        SinkFileParams.OperationProgress = OperationProgress;
        SinkFileParams.Skipped = false;
        SinkFileParams.Flags = Flags & ~(tfFirstLevel | tfAutoResume);

        FTerminal->ProcessDirectory(AFileName, MAKE_CALLBACK(TWebDAVFileSystem::SinkFile, this), &SinkFileParams);

        // Do not delete directory if some of its files were skipped.
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
  }
  else
  {
    FTerminal->LogEvent(FORMAT(L"Copying \"%s\" to local directory started.", AFileName.c_str()));
    bool CanProceed = true;
    if (::FileExists(ApiPath(DestFullName)))
    {
      int64_t Size = 0;
      int64_t MTime = 0;
      FTerminal->OpenLocalFile(DestFullName, GENERIC_READ,
        nullptr, nullptr, nullptr, &MTime, nullptr, &Size);
      TOverwriteFileParams FileParams;

      FileParams.SourceSize = AFile->GetSize();
      FileParams.SourceTimestamp = AFile->GetModification();
      FileParams.DestSize = Size;
      FileParams.DestTimestamp = ::UnixToDateTime(MTime,
        FTerminal->GetSessionData()->GetDSTMode());

      uintptr_t Answer = 0;
      TOverwriteMode OverwriteMode = omOverwrite;
      bool AutoResume = false;
      ConfirmOverwrite(AFileName, DestFullName, OperationProgress,
          &FileParams, CopyParam, Params, AutoResume,
          OverwriteMode, Answer);
      switch (Answer)
      {
        case qaCancel:
          OperationProgress->Cancel = csCancel; // continue on next case
          // FALLTHROUGH
        case qaNo:
          CanProceed = false;
        default:
          break;
      }
    }
    if (CanProceed)
    {
      // Suppose same data size to transfer as to write
      OperationProgress->SetTransferSize(AFile->GetSize());
      OperationProgress->SetLocalSize(OperationProgress->TransferSize);

      DWORD LocalFileAttrs = INVALID_FILE_ATTRIBUTES;
      FileOperationLoopCustom(FTerminal, OperationProgress, True, FMTLOAD(NOT_FILE_ERROR, DestFullName.c_str()), "",
      [&]()
      {
        LocalFileAttrs = FTerminal->GetLocalFileAttributes(ApiPath(DestFullName));
        if ((LocalFileAttrs != INVALID_FILE_ATTRIBUTES) && FLAGSET(LocalFileAttrs, faDirectory))
        {
          ThrowExtException();
        }
      });

      OperationProgress->TransferingFile = false; // not set with FTP protocol

      ResetFileTransfer();

      TFileTransferData UserData;

      UnicodeString FilePath = core::UnixExtractFilePath(AFileName);
      if (FilePath.IsEmpty())
      {
        FilePath = ROOTDIRECTORY;
      }

      {
        int TransferType = 2; // OperationProgress->AsciiTransfer = false
        // ignore file list
        TWebDAVFileListHelper Helper(this, nullptr, true);

        FFileTransferCPSLimit = OperationProgress->CPSLimit;
        FFileTransferPreserveTime = CopyParam->GetPreserveTime();
        UserData.FileName = DestFileName;
        UserData.Params = Params;
        UserData.AutoResume = FLAGSET(Flags, tfAutoResume);
        UserData.CopyParam = CopyParam;
        FileTransfer(AFileName, DestFullName, FileNameOnly,
          FilePath, true, AFile->GetSize(), TransferType, UserData, OperationProgress);
      }

      // in case dest filename is changed from overwrite dialog
      if (DestFileName != UserData.FileName)
      {
        DestFullName = TargetDir + UserData.FileName;
        LocalFileAttrs = FTerminal->GetLocalFileAttributes(ApiPath(DestFullName));
      }

      Action.Destination(::ExpandUNCFileName(DestFullName));

      if (LocalFileAttrs == INVALID_FILE_ATTRIBUTES)
      {
        LocalFileAttrs = faArchive;
      }
      DWORD NewAttrs = CopyParam->LocalFileAttrs(*AFile->GetRights());
      if ((NewAttrs & LocalFileAttrs) != NewAttrs)
      {
        FileOperationLoopCustom(FTerminal, OperationProgress, True, FMTLOAD(CANT_SET_ATTRS, DestFullName.c_str()), "",
        [&]()
        {
          THROWOSIFFALSE(FTerminal->SetLocalFileAttributes(ApiPath(DestFullName), LocalFileAttrs | NewAttrs) == 0);
        });
      }
      // set time
      FTerminal->SetLocalFileTime(DestFullName, AFile->GetModification());
    }
  }

  if (FLAGSET(Params, cpDelete))
  {
    // If file is directory, do not delete it recursively, because it should be
    // empty already. If not, it should not be deleted (some files were
    // skipped or some new files were copied to it, while we were downloading)
    intptr_t Params = dfNoRecursive;
    FTerminal->RemoteDeleteFile(AFileName, AFile, &Params);
  }
}

void TWebDAVFileSystem::SinkFile(const UnicodeString & AFileName,
  const TRemoteFile * AFile, void * Param)
{
  TSinkFileParams * Params = NB_STATIC_DOWNCAST(TSinkFileParams, Param);
  DebugAssert(Params->OperationProgress);
  try
  {
    SinkRobust(AFileName, AFile, Params->TargetDir, Params->CopyParam,
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

bool TWebDAVFileSystem::HandleListData(const wchar_t * Path,
  const TListDataEntry * Entries, intptr_t Count)
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
  else
  {
    DebugAssert(FFileList != nullptr);
    // this can actually fail in real life,
    // when connected to server with case insensitive paths
    UnicodeString AbsolutePath = GetAbsolutePath(FFileList->GetDirectory(), false);
    DebugAssert(core::UnixSamePath(AbsolutePath, Path));
    DebugUsedParam(Path);

    for (intptr_t Index = 0; Index < Count; ++Index)
    {
      const TListDataEntry * Entry = &Entries[Index];
      std::unique_ptr<TRemoteFile> File(new TRemoteFile());
      try
      {
        File->SetTerminal(FTerminal);

        File->SetFileName(UnicodeString(Entry->Name));
        if (wcslen(Entry->Permissions) >= 10)
        {
          try
          {
            File->GetRights()->SetText(Entry->Permissions + 1);
          }
          catch (...)
          {
            // ignore permissions errors with WebDAV
          }
        }
        // FIXME
        UnicodeString Own = Entry->OwnerGroup;
        const wchar_t * Space = wcschr(Own.c_str(), ' ');
        if (Space != nullptr)
        {
          File->GetFileOwner().SetName(UnicodeString(Own.c_str(), (intptr_t)(Space - Own.c_str())));
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

        // ModificationFmt must be set after Modification
        if (Entry->Time.HasDate)
        {
          // should be the same as ConvertRemoteTimestamp
          TDateTime Modification =
            EncodeDateVerbose(static_cast<uint16_t>(Entry->Time.Year), static_cast<uint16_t>(Entry->Time.Month),
              static_cast<uint16_t>(Entry->Time.Day));
          if (Entry->Time.HasTime)
          {
            uint16_t seconds = 0;
            if (Entry->Time.HasSeconds)
              seconds = static_cast<uint16_t>(Entry->Time.Second);
            File->SetModification(Modification +
              EncodeTimeVerbose(static_cast<uint16_t>(Entry->Time.Hour),
                static_cast<uint16_t>(Entry->Time.Minute),
                seconds, 0));
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
          // We estimate date to be today, if we have at least time
          File->SetModification(TDateTime(0.0));
          File->SetModificationFmt(mfNone);
        }
        File->SetLastAccess(File->GetModification());

        File->SetLinkTo(Entry->LinkTarget);

        File->Complete();
      }
      catch (Exception & E)
      {
        UnicodeString EntryData =
          FORMAT(L"%s/%s/%s/%lld/%d/%d/%d/%d/%d/%d/%d/%d/%d",
                 Entry->Name,
                 Entry->Permissions,
                 Entry->OwnerGroup,
                 Entry->Size,
                 int(Entry->Dir), int(Entry->Link), Entry->Time.Year, Entry->Time.Month, Entry->Time.Day,
                 Entry->Time.Hour, Entry->Time.Minute, int(Entry->Time.HasTime), int(Entry->Time.HasDate));
        throw ETerminal(&E, FMTLOAD(LIST_LINE_ERROR, EntryData.c_str()), HELP_LIST_LINE_ERROR);
      }

      FFileList->AddFile(File.release());
    }
    return true;
  }
}

void TWebDAVFileSystem::ResetFileTransfer()
{
  FFileTransferAbort = ftaNone;
  FFileTransferCancelled = false;
  FFileTransferResumed = 0;
  webdav::cancelled = 0;
}

void TWebDAVFileSystem::ReadDirectoryProgress(int64_t Bytes)
{
  // with WebDAV we do not know exactly how many entries we have received,
  // instead we know number of bytes received only.
  // so we report approximation based on average size of entry.
  int Progress = static_cast<int>(Bytes / 80);
  if (Progress - FLastReadDirectoryProgress >= 10)
  {
    bool Cancel = false;
    FLastReadDirectoryProgress = Progress;
    FTerminal->DoReadDirectoryProgress(Progress, 0, Cancel);
    if (Cancel)
    {
      FTerminal->DoReadDirectoryProgress(-2, 0, Cancel);
    }
  }
}

void TWebDAVFileSystem::DoFileTransferProgress(int64_t TransferSize,
  int64_t Bytes)
{
  TFileOperationProgressType * OperationProgress = FTerminal->GetOperationProgress();
  if (!OperationProgress)
  {
    return;
  }

  OperationProgress->SetTransferSize(TransferSize);

  if (FFileTransferResumed > 0)
  {
    OperationProgress->AddResumed(FFileTransferResumed);
    FFileTransferResumed = 0;
  }

  int64_t Diff = Bytes - OperationProgress->TransferedSize;
  if (Diff >= 0)
  {
    OperationProgress->AddTransfered(Diff);
  }

  if (!FFileTransferCancelled && OperationProgress->Cancel == csCancel)
  {
    FFileTransferCancelled = true;
    FFileTransferAbort = ftaCancel;
    webdav::cancelled = 1;
  }

  if (FFileTransferCPSLimit != OperationProgress->CPSLimit)
  {
    FFileTransferCPSLimit = OperationProgress->CPSLimit;
  }
}

void TWebDAVFileSystem::FileTransferProgress(int64_t TransferSize,
  int64_t Bytes)
{
  TGuard Guard(FTransferStatusCriticalSection);

  DoFileTransferProgress(TransferSize, Bytes);
}

void TWebDAVFileSystem::FileTransfer(const UnicodeString & AFileName,
  const UnicodeString & LocalFile, const UnicodeString & RemoteFile,
  const UnicodeString & RemotePath, bool Get, int64_t Size, int /*Type*/,
  TFileTransferData & /*UserData*/, TFileOperationProgressType * OperationProgress)
{
  FCurrentOperationProgress = OperationProgress;
  FileOperationLoopCustom(FTerminal, OperationProgress, True, FMTLOAD(TRANSFER_ERROR, AFileName.c_str()), "",
  [&]()
  {
    UnicodeString FullRemoteFileName = RemotePath + RemoteFile;
    bool Result;
    if (Get)
    {
      Result = WebDAVGetFile(FullRemoteFileName.c_str(), LocalFile.c_str());
    }
    else
    {
      Result = WebDAVPutFile(FullRemoteFileName.c_str(), LocalFile.c_str(), Size);
    }
    // if (!Result)
      // ThrowExtException();
  });

  switch (FFileTransferAbort)
  {
    case ftaSkip:
      ThrowSkipFile(nullptr, L"");

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

bool TWebDAVFileSystem::SendPropFindRequest(const wchar_t * Path, int & ResponseCode)
{
  DebugAssert(Path);

  DebugAssert(FSession);
  apr_pool_t * pool = webdav_pool_create(webdav_pool);
  webdav::error_t err = WEBDAV_NO_ERROR;
  const char * remote_path = nullptr;
  err = webdav::path_cstring_to_utf8(&remote_path, StrToNeon(Path), pool);
  if (err)
    return false;
  err = webdav::client_send_propfind_request(
    FSession,
    remote_path,
    &ResponseCode,
    pool);

  webdav_pool_destroy(pool);
  return err == WEBDAV_NO_ERROR;
}

bool TWebDAVFileSystem::WebDAVCheckExisting(const wchar_t * Path, int & IsDir)
{
  DebugAssert(Path);
  IsDir = 0;
  DebugAssert(FSession);
  apr_pool_t * pool = webdav_pool_create(webdav_pool);
  webdav::error_t err = WEBDAV_NO_ERROR;
  webdav::node_kind_t kind = webdav::node_none;
  const char * remote_path = nullptr;
  err = webdav::path_cstring_to_utf8(&remote_path, StrToNeon(Path), pool);
  if (err)
    return false;
  err = webdav::client_check_path(
    FSession,
    remote_path,
    &kind,
    pool);

  if (kind != webdav::node_none)
    IsDir = kind == webdav::node_dir;
  webdav_pool_destroy(pool);
  return (err == WEBDAV_NO_ERROR) && (kind != webdav::node_none);
}

bool TWebDAVFileSystem::WebDAVMakeDirectory(const wchar_t * Path)
{
  DebugAssert(Path);

  DebugAssert(FSession);
  apr_pool_t * pool = webdav_pool_create(webdav_pool);
  webdav::error_t err = WEBDAV_NO_ERROR;
  const char * remote_path = nullptr;
  err = webdav::path_cstring_to_utf8(&remote_path, StrToNeon(Path), pool);
  if (err)
    return false;
  err = webdav::client_make_directory(
    FSession,
    remote_path,
    nullptr,
    pool);
  webdav_pool_destroy(pool);
  return err == WEBDAV_NO_ERROR;
}

bool TWebDAVFileSystem::WebDAVGetList(const UnicodeString & Directory)
{
  webdav::listdataentry_vector_t Entries;

  DebugAssert(FSession);
  webdav::list_func_baton_t baton = {0};
  baton.verbose = true;
  baton.entries = &Entries;
  baton.session = FSession;
  baton.pool = webdav_pool_create(webdav_pool);
  webdav::error_t err = WEBDAV_NO_ERROR;
  const char * remote_path = nullptr;
  err = webdav::path_cstring_to_utf8(&remote_path, StrToNeon(Directory), baton.pool);
  if (err)
    return false;
  err = webdav::client_list(
    FSession,
    remote_path,
    webdav::depth_immediates,
    WEBDAV_DIRENT_ALL,
    webdav::list_func,
    &baton,
    baton.pool);

  TListDataEntry * pEntries = !Entries.empty() ? &Entries[0] : nullptr;
  HandleListData(Directory.c_str(), pEntries, Entries.size());
  webdav_pool_destroy(baton.pool);
  return err == WEBDAV_NO_ERROR;
}

bool TWebDAVFileSystem::WebDAVGetFile(
  const wchar_t * RemotePath, const wchar_t * LocalPath)
{
  DebugAssert(RemotePath && *RemotePath);
  DebugAssert(FSession);

  bool Result = false;
  HANDLE LocalFileHandle = FTerminal->CreateLocalFile(LocalPath,
    GENERIC_WRITE, 0, CREATE_ALWAYS, 0);
  if (LocalFileHandle != INVALID_HANDLE_VALUE)
  try
  {
    apr_pool_t * pool = webdav_pool_create(webdav_pool);
    webdav::error_t err = WEBDAV_NO_ERROR;
    const char * remote_path = nullptr;
    err = webdav::path_cstring_to_utf8(&remote_path, StrToNeon(RemotePath), pool);
    if (err)
    {
      ::CloseHandle(LocalFileHandle);
      return false;
    }
    err = webdav::client_get_file(
      FSession,
      remote_path,
      &LocalFileHandle,
      pool);

    webdav_pool_destroy(pool);
    Result = err == WEBDAV_NO_ERROR;
    if (!Result)
    {
      ::CloseHandle(LocalFileHandle);
      LocalFileHandle = INVALID_HANDLE_VALUE;
    }
  }
  catch (...)
  {
    if (LocalFileHandle != INVALID_HANDLE_VALUE)
    {
      ::CloseHandle(LocalFileHandle);
      LocalFileHandle = INVALID_HANDLE_VALUE;
    }
    Result = false;
    throw;
  }

  return Result;
}

bool TWebDAVFileSystem::WebDAVPutFile(const wchar_t * RemotePath,
  const wchar_t * LocalPath, const uint64_t /*FileSize*/)
{
  DebugAssert(RemotePath && *RemotePath);
  DebugAssert(LocalPath && *LocalPath);

  DebugAssert(FSession);
  apr_pool_t * pool = webdav_pool_create(webdav_pool);
  webdav::error_t err = WEBDAV_NO_ERROR;
  const char * remote_path = nullptr;
  const char * local_path = nullptr;
  err = webdav::path_cstring_to_utf8(&remote_path, StrToNeon(RemotePath), pool);
  if (err)
    return false;
  err = webdav::path_cstring_to_utf8(&local_path, StrToNeon(ApiPath(LocalPath)), pool);
  if (err)
    return false;
  try
  {
    err = webdav::client_put_file(
      FSession,
      remote_path,
      local_path,
      pool);
  }
  catch (ESkipFile &)
  {
    err = WEBDAV_ERR_CANCELLED;
  }

  webdav_pool_destroy(pool);
  if (err == WEBDAV_ERR_CANCELLED)
  {
    FFileTransferCancelled = true;
    FFileTransferAbort = ftaCancel;
  }
  return err == WEBDAV_NO_ERROR;
}

bool TWebDAVFileSystem::WebDAVRenameFile(const wchar_t * SrcPath, const wchar_t * DstPath)
{
  DebugAssert(SrcPath && *SrcPath);
  DebugAssert(DstPath && *DstPath);

  DebugAssert(FSession);
  apr_pool_t * pool = webdav_pool_create(webdav_pool);
  webdav::error_t err = WEBDAV_NO_ERROR;
  const char * src_path = nullptr;
  const char * dst_path = nullptr;
  err = webdav::path_cstring_to_utf8(&src_path, StrToNeon(SrcPath), pool);
  if (err)
    return false;
  err = webdav::path_cstring_to_utf8(&dst_path, StrToNeon(DstPath), pool);
  if (err)
    return false;
  err = webdav::client_move_file_or_directory(
    FSession,
    src_path,
    dst_path,
    nullptr,
    pool);

  webdav_pool_destroy(pool);
  return err == WEBDAV_NO_ERROR;
}

bool TWebDAVFileSystem::WebDAVDeleteFile(const wchar_t * Path)
{
  DebugAssert(Path);

  DebugAssert(FSession);
  apr_pool_t * pool = webdav_pool_create(webdav_pool);
  webdav::error_t err = WEBDAV_NO_ERROR;
  const char * remote_path = nullptr;
  err = webdav::path_cstring_to_utf8(&remote_path, StrToNeon(Path), pool);
  if (err)
    return false;
  err = webdav::client_delete_file(
    FSession,
    remote_path,
    nullptr,
    pool);

  webdav_pool_destroy(pool);
  return err == WEBDAV_NO_ERROR;
}

webdav::error_t TWebDAVFileSystem::OpenURL(
  const UnicodeString & SessionURL,
  apr_pool_t * pool)
{
  webdav::client_ctx_t * ctx = nullptr;
  WEBDAV_ERR(client_create_context(&ctx, pool));

  const char * auth_username = nullptr;
  const char * auth_password = nullptr;
  WEBDAV_ERR(webdav::utf_cstring_to_utf8(&auth_username,
    StrToNeon(FTerminal->GetSessionData()->GetUserNameExpanded()), pool));
  WEBDAV_ERR(webdav::utf_cstring_to_utf8(&auth_password,
    StrToNeon(FTerminal->GetSessionData()->GetPassword()), pool));
  webdav::auth_baton_t * ab = nullptr;
  webdav::auth_baton_create(&ab, pool);
  webdav::auth_baton_init(
    ab,
    false, // non_interactive
    auth_username,
    auth_password,
    false, // no_auth_cache
    true, // trust_server_cert
    this,
    webdav::check_cancel, ab,
    pool);
  ctx->auth_baton = ab;

  // Set up our cancellation support.
  ctx->cancel_func = webdav::check_cancel;
  ctx->cancel_baton = ab;

  ctx->progress_func = webdav::progress_func;
  ctx->progress_baton = ctx;

  webdav::session_t * session_p = nullptr;
  const char * corrected_url = nullptr;
  UTF8String base_url(SessionURL);
  const char * base_url_encoded = webdav::path_uri_encode(base_url.c_str(), pool);
  WEBDAV_ERR(webdav::client_open_session_internal(
    &session_p,
    &corrected_url,
    base_url_encoded,
    ctx,
    pool));

  const char * url = nullptr;
  if (corrected_url)
  {
    url = apr_pstrdup(pool, corrected_url);
  }
  else
  {
    url = apr_pstrdup(pool, base_url_encoded);
  }
  ne_uri * uri = nullptr;
  if (WEBDAV_NO_ERROR == webdav::parse_ne_uri(&uri, url, pool))
  {
    FCurrentDirectory = uri->path;
    FHasTrailingSlash = (FCurrentDirectory.Length() > 0) && (FCurrentDirectory[FCurrentDirectory.Length()] == L'/');
  }
  FSession = session_p;
  return WEBDAV_NO_ERROR;
}

webdav::error_t TWebDAVFileSystem::GetServerSettings(
  int * proxy_method,
  const char ** proxy_host,
  uint32_t * proxy_port,
  const char ** proxy_username,
  const char ** proxy_password,
  int * timeout_seconds,
  int * neon_debug,
  const char ** neon_debug_file_name,
  bool * compression,
  const char ** pk11_provider,
  const char ** ssl_authority_file,
  apr_pool_t * pool)
{
  // If we find nothing, default to nulls.
  *proxy_method = 0;
  *proxy_host = nullptr;
  *proxy_port = static_cast<uint32_t>(-1);
  *proxy_username = nullptr;
  *proxy_password = nullptr;
  *pk11_provider = nullptr;
  *ssl_authority_file = nullptr;

  TSessionData * Data = FTerminal->GetSessionData();
  TConfiguration * Configuration = FTerminal->GetConfiguration();
  {
    TProxyMethod ProxyMethod = Data->GetProxyMethod();
    *proxy_method = static_cast<int>(ProxyMethod);
    if (ProxyMethod != ::pmNone)
    {
      WEBDAV_ERR(webdav::path_cstring_to_utf8(proxy_host, StrToNeon(Data->GetProxyHost()), pool));
      WEBDAV_ERR(webdav::path_cstring_to_utf8(proxy_username, StrToNeon(Data->GetProxyUsername()), pool));
      WEBDAV_ERR(webdav::path_cstring_to_utf8(proxy_password, StrToNeon(Data->GetProxyPassword()), pool));
    }
  }

  // Apply non-proxy-specific settings regardless of exceptions:
  if (compression)
    *compression = Data->GetCompression();

  int l_debug = Configuration->GetActualLogProtocol() >= 1 ? 1 : 0;
  *pk11_provider = "";

  *ssl_authority_file = apr_pstrdup(pool, StrToNeon(Data->GetPublicKeyFile()));

  {
    intptr_t l_proxy_port = Data->GetProxyPort();
    if (l_proxy_port < 0)
    {
      return webdav::error_create(WEBDAV_ERR_ILLEGAL_URL, nullptr,
        "Invalid URL: negative proxy port number");
    }
    if (l_proxy_port > 65535)
    {
      return webdav::error_create(WEBDAV_ERR_ILLEGAL_URL, nullptr,
        "Invalid URL: proxy port number greater "
        "than maximum TCP port number 65535");
    }
    *proxy_port = static_cast<uint32_t>(l_proxy_port);
  }

  {
    intptr_t l_timeout = Data->GetTimeout();
    if (l_timeout < 0)
      return webdav::error_create(WEBDAV_ERR_BAD_CONFIG_VALUE, nullptr,
        "Invalid config: negative timeout value");
    *timeout_seconds = static_cast<int>(l_timeout);
  }

  if (l_debug)
  {
    *neon_debug = l_debug;
    if (Configuration->GetLogToFile())
    {
      WEBDAV_ERR(webdav::path_cstring_to_utf8(neon_debug_file_name,
        StrToNeon(GetExpandedLogFileName(Configuration->GetLogFileName(), Data)), pool));
    }
    else
    {
      *neon_debug_file_name = nullptr;
    }
  }
  else
  {
    *neon_debug = 0;
    *neon_debug_file_name = nullptr;
  }

  return WEBDAV_NO_ERROR;
}

webdav::error_t TWebDAVFileSystem::VerifyCertificate(
  const char * Prompt, const char * fingerprint,
  uintptr_t & RequestResult)
{
  RequestResult = 0;
  TClipboardHandler ClipboardHandler;
  ClipboardHandler.Text = fingerprint;

  TQueryButtonAlias Aliases[1];
  Aliases[0].Button = qaRetry;
  Aliases[0].Alias = LoadStr(COPY_KEY_BUTTON);
  Aliases[0].OnClick = MAKE_CALLBACK(TClipboardHandler::Copy, &ClipboardHandler);

  TQueryParams Params;
  Params.HelpKeyword = HELP_VERIFY_CERTIFICATE;
  Params.NoBatchAnswers = qaYes | qaRetry;
  Params.Aliases = Aliases;
  Params.AliasesCount = _countof(Aliases);
  uintptr_t Answer = FTerminal->QueryUser(
    FMTLOAD(VERIFY_CERT_PROMPT3, UnicodeString(Prompt).c_str()),
    nullptr, qaYes | qaNo | qaCancel | qaRetry, &Params, qtWarning);
  RequestResult = Answer;
  switch (RequestResult)
  {
    case qaCancel:
      // FTerminal->Configuration->Usage->Inc(L"HostNotVerified");
      FFileTransferCancelled = true;
      FFileTransferAbort = ftaCancel;
      break;
  }
  return WEBDAV_NO_ERROR;
}

webdav::error_t TWebDAVFileSystem::AskForClientCertificateFilename(
  const char ** cert_file, uintptr_t & RequestResult,
  apr_pool_t * pool)
{
  RequestResult = 0;
#if 0
  TSessionData * Data = FTerminal->GetSessionData();
  UnicodeString FileName;
  if (!FTerminal->PromptUser(Data, pkFileName, LoadStr(CERT_FILENAME_PROMPT_TITLE), L"",
    LoadStr(CERT_FILENAME_PROMPT), true, 0, FileName))
  {
    FFileTransferCancelled = true;
    FFileTransferAbort = ftaCancel;
    return WEBDAV_ERR_CANCELLED;
  }
  WEBDAV_ERR(webdav::path_cstring_to_utf8(cert_file, StrToNeon(FileName), pool));
  RequestResult = qaOK;
#endif
  return WEBDAV_NO_ERROR;
}

webdav::error_t TWebDAVFileSystem::NeonRequestAuth(
  const char ** user_name,
  const char ** password,
  uintptr_t & RequestResult,
  apr_pool_t * pool)
{
  bool Result = true;
  RequestResult = 0;
  TSessionData * SessionData = FTerminal->GetSessionData();
  UnicodeString UserName = SessionData->GetUserNameExpanded();
  // will ask for username only once
  if (this->FUserName.IsEmpty())
  {
    if (!UserName.IsEmpty())
    {
      this->FUserName = UserName;
    }
    else
    {
      if (!FTerminal->PromptUser(SessionData, pkUserName, LoadStr(USERNAME_TITLE), L"",
        LoadStr(USERNAME_PROMPT2), true, NE_ABUFSIZ, this->FUserName))
      {
        // note that we never get here actually
        Result = false;
      }
    }
  }

  //if (!FTerminal->PromptUser(Data, pkUserName, LoadStr(USERNAME_TITLE), L"",
  //  LoadStr(USERNAME_PROMPT2), true, 0, UserName))
  //{
  //  FFileTransferCancelled = true;
  //  FFileTransferAbort = ftaCancel;
  //  return WEBDAV_ERR_CANCELLED;
  //}
  //WEBDAV_ERR(webdav::path_cstring_to_utf8(user_name, AnsiString(UserName).c_str(), pool));
  //RequestResult = qaOK;

  UnicodeString Password;
  if (Result)
  {
    // Some servers (Gallery2 on https://g2.pixi.me/w/webdav/)
    // return authentication error (401) on PROPFIND request for
    // non-existing files.
    // When we already tried password before, do not try anymore.
    // When we did not try password before (possible only when
    // server does not require authentication for any previous request,
    // such as when read access is not authenticated), try it now,
    // but use special flag for the try, because when it fails
    // we still want to try password for future requests (such as PUT).

    if (!this->FPassword.IsEmpty() && !this->FStoredPasswordTried)
    {
      if (this->FIgnoreAuthenticationFailure == iafPasswordFailed)
      {
        // Fail PROPFIND /nonexisting request...
        Result = false;
      }
      else
      {
        Password = FTerminal->DecryptPassword(this->FPassword);
      }
    }
    else
    {
      if (!SessionData->GetPassword().IsEmpty() && !this->FStoredPasswordTried)
      {
        Password = SessionData->GetPassword();
        this->FStoredPasswordTried = true;
      }
      else
      {
        // Asking for password (or using configured password) the first time,
        // and asking for password.
        // Note that we never get false here actually
        Result =
          FTerminal->PromptUser(
          SessionData, pkPassword, LoadStr(PASSWORD_TITLE), L"",
            LoadStr(PASSWORD_PROMPT), false, NE_ABUFSIZ, Password);
      }

      if (Result)
      {
        // While neon remembers the password on its own,
        // we need to keep a copy in case neon store gets reset by
        // 401 response to PROPFIND /nonexisting on G2, see above.
        // Possibly we can do this for G2 servers only.
        this->FPassword = FTerminal->EncryptPassword(Password);
      }
    }
  }

  if (Result)
  {
    WEBDAV_ERR(webdav::path_cstring_to_utf8(user_name, StrToNeon(this->FUserName), pool));
    WEBDAV_ERR(webdav::path_cstring_to_utf8(password, StrToNeon(Password), pool));
    RequestResult = qaOK;
  }

  return Result ? WEBDAV_NO_ERROR : WEBDAV_ERROR_AUTH;
}

webdav::error_t TWebDAVFileSystem::AskForUsername(
  const char ** user_name, uintptr_t & RequestResult,
  apr_pool_t * pool)
{
  RequestResult = 0;
  TSessionData * Data = FTerminal->GetSessionData();
  UnicodeString UserName = Data->GetUserNameExpanded();
  if (!FTerminal->PromptUser(Data, pkUserName, LoadStr(USERNAME_TITLE), L"",
    LoadStr(USERNAME_PROMPT2), true, 0, UserName))
  {
    FFileTransferCancelled = true;
    FFileTransferAbort = ftaCancel;
    return WEBDAV_ERR_CANCELLED;
  }
  WEBDAV_ERR(webdav::path_cstring_to_utf8(user_name, StrToNeon(UserName), pool));
  RequestResult = qaOK;
  return WEBDAV_NO_ERROR;
}

webdav::error_t TWebDAVFileSystem::AskForUserPassword(
  const char ** password,
  uintptr_t & RequestResult,
  apr_pool_t * pool)
{
  RequestResult = 0;
  TSessionData * Data = FTerminal->GetSessionData();
  UnicodeString Password = Data->GetPassword();
  if (!FTerminal->PromptUser(Data, pkPassword, LoadStr(PASSWORD_TITLE), L"",
    LoadStr(PASSWORD_PROMPT), false, 0, Password))
  {
    FFileTransferCancelled = true;
    FFileTransferAbort = ftaCancel;
    return WEBDAV_ERR_CANCELLED;
  }
  WEBDAV_ERR(webdav::path_cstring_to_utf8(password, StrToNeon(Password), pool));
  RequestResult = qaOK;
  return WEBDAV_NO_ERROR;
}

webdav::error_t TWebDAVFileSystem::AskForPassphrase(
  const char ** passphrase,
  const char * realm,
  uintptr_t & RequestResult,
  apr_pool_t * pool)
{
  RequestResult = 0;
  TSessionData * Data = FTerminal->GetSessionData();
  UnicodeString Passphrase = Data->GetUserNameExpanded();
  UnicodeString Prompt = FORMAT(LoadStr(PROMPT_KEY_PASSPHRASE).c_str(), UnicodeString(realm).c_str());
  if (!FTerminal->PromptUser(Data, pkPassphrase, LoadStr(PASSPHRASE_TITLE), L"",
    Prompt, false, 0, Passphrase))
  {
    FFileTransferCancelled = true;
    FFileTransferAbort = ftaCancel;
    return WEBDAV_ERR_CANCELLED;
  }
  WEBDAV_ERR(webdav::path_cstring_to_utf8(passphrase, StrToNeon(Passphrase), pool));
  RequestResult = qaOK;
  return WEBDAV_NO_ERROR;
}

webdav::error_t TWebDAVFileSystem::SimplePrompt(
  const char * prompt_text,
  const char * prompt_string,
  uintptr_t & RequestResult)
{
  RequestResult = 0;
  std::unique_ptr<TStrings> MoreMessages(new TStringList());
  MoreMessages->Add(UnicodeString(prompt_string));
  uintptr_t Answer = FTerminal->QueryUser(
    UnicodeString(prompt_text),
    MoreMessages.get(), qaYes | qaNo | qaCancel, nullptr, qtConfirmation);
  RequestResult = Answer;
  return RequestResult == qaCancel ? WEBDAV_ERR_CANCELLED : WEBDAV_NO_ERROR;
}

webdav::error_t TWebDAVFileSystem::CreateStorage(
  THierarchicalStorage *& Storage)
{
  Storage =
    FTerminal->GetConfiguration()->CreateConfigStorage();
  return WEBDAV_NO_ERROR;
}

uintptr_t TWebDAVFileSystem::AdjustToCPSLimit(uintptr_t Len)
{
  return FCurrentOperationProgress ? (uintptr_t)FCurrentOperationProgress->AdjustToCPSLimit(Len) : Len;
}

NB_IMPLEMENT_CLASS(TWebDAVFileSystem, NB_GET_CLASS_INFO(TCustomFileSystem), nullptr)

