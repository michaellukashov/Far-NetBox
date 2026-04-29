
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

#include "SessionData.h"
#include "Interface.h"
#include "Common.h"
#include "Exceptions.h"
#include "Terminal.h"
#include "TextsCore.h"
#include "HelpCore.h"
#include "NeonIntf.h"
#include <ne_request.h>
#include <StrUtils.hpp>
#include <limits>
#include "CoreMain.h"
#include "Http.h"
#include "Cryptography.h"
#include <System.JSON.hpp>
#include <System.DateUtils.hpp>
#include <System.IOUtils.hpp>
extern "C" {
#include <request.h>
} // extern "C"
#include <XMLDoc.hpp>

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

#if defined(__BORLANDC__)
#pragma package(smart_init)
#endif // defined(__BORLANDC__)

// Should be used with character pointer only
#define StrFromS3(S) StrFromNeon(S)
#define StrToS3(S) StrToNeon(S)

#undef FILE_OPERATION_LOOP_TERMINAL
#define FILE_OPERATION_LOOP_TERMINAL FTerminal

#define AWS_ACCESS_KEY_ID L"AWS_ACCESS_KEY_ID"
#define AWS_SECRET_ACCESS_KEY L"AWS_SECRET_ACCESS_KEY"
#define AWS_SESSION_TOKEN L"AWS_SESSION_TOKEN"
#define AWS_SHARED_CREDENTIALS_FILE L"AWS_SHARED_CREDENTIALS_FILE"
#define AWS_CONFIG_FILE L"AWS_CONFIG_FILE"
#define AWS_PROFILE L"AWS_PROFILE"
#define AWS_PROFILE_DEFAULT L"default"
#define AWS_CONFIG_PROFILE_PREFIX L"profile "
#define AWS_SOURCE_PROFILE_KEY L"source_profile"
#define AWS_CREDENTIAL_SOURCE_KEY L"credential_source"
#define AWS_CREDENTIAL_SOURCE_ENVIRONMENT L"Environment"
#define AWS_CREDENTIAL_SOURCE_METADATA L"Ec2InstanceMetadata"
#define AWS_ROLE_ARN L"AWS_ROLE_ARN"
#define AWS_ROLE_ARN_KEY L"role_arn"

static std::unique_ptr<TCriticalSection> LibS3Section(TraceInitPtr(std::make_unique<TCriticalSection>()));

constexpr const char * LibS3Delimiter = "/";

UnicodeString S3LibVersion()
{
  return FORMAT("%s.%s", LIBS3_VER_MAJOR, LIBS3_VER_MINOR);
}

UnicodeString S3LibDefaultHostName()
{
  return UnicodeString(S3_DEFAULT_HOSTNAME);
}

UnicodeString S3LibDefaultRegion()
{
  return StrFromS3(S3_DEFAULT_REGION);
}

bool IsAmazonS3SessionData(TSessionData * Data)
{
  return IsDomainOrSubdomain(Data->HostNameExpanded, S3HostName);
}

#if defined(__BORLANDC__)

static void NeedS3Config(
  UnicodeString & FileName, TDateTime & TimeStamp, std::unique_ptr<TCustomIniFile> & File,
  const UnicodeString & DefaultName, const UnicodeString & EnvName)
{
  if (FileName.IsEmpty())
  {
    FileName = GetEnvironmentVariable(EnvName);
    if (FileName.IsEmpty())
    {
      UnicodeString ProfilePath = GetShellFolderPath(CSIDL_PROFILE);
      UnicodeString DefaultFileName = IncludeTrailingBackslash(ProfilePath) + L".aws\\" + DefaultName;
      if (FileExists(DefaultFileName))
      {
        FileName = DefaultFileName;
      }
    }
  }

  TDateTime CurrentTimestamp;
  FileAge(FileName, CurrentTimestamp);
  if (TimeStamp != CurrentTimestamp)
  {
    TimeStamp = CurrentTimestamp;
    // TMemIniFile silently ignores empty paths or non-existing files
    AppLogFmt("Reading AWS '%s' file", FileName);
    File.reset(new TMemIniFile(FileName));
  }
}

#endif // defined(__BORLANDC__)

static UnicodeString S3CredentialsFileName;
static TDateTime S3CredentialsTimestamp;
#if defined(__BORLANDC__)
static std::unique_ptr<TCustomIniFile> S3CredentialsFile;
#endif // defined(__BORLANDC__)
static UnicodeString S3ConfigFileName;
static TDateTime S3ConfigTimestamp;
#if defined(__BORLANDC__)
static std::unique_ptr<TCustomIniFile> S3ConfigFile;
#endif // defined(__BORLANDC__)
static UnicodeString S3Profile;
static bool S3SecurityProfileChecked = false;
static TDateTime S3CredentialsExpiration;
static UnicodeString S3SessionToken;
static UnicodeString S3SecurityProfile;
// Thread-safety: LibS3Section protects S3Profile, S3Credentials, S3SessionToken,
// S3SecurityProfile, S3SecurityProfileChecked, and S3CredentialsExpiration.
// All accesses must be under LibS3Section.
#if defined(__BORLANDC__)
typedef std::map<UnicodeString, UnicodeString> TS3Credentials;
#endif // defined(__BORLANDC__)
using TS3Credentials = nb::map_t<UnicodeString, UnicodeString>;
static TS3Credentials S3Credentials;

#if defined(__BORLANDC__)

static void NeedS3Config()
{
  TGuard Guard(*LibS3Section.get());
  if (S3Profile.IsEmpty())
  {
    S3Profile = base::GetEnvironmentVariable(AWS_PROFILE);
    if (S3Profile.IsEmpty())
    {
      S3Profile = AWS_PROFILE_DEFAULT;
    }
  }

  NeedS3Config(S3CredentialsFileName, S3CredentialsTimestamp, S3CredentialsFile, L"credentials", AWS_SHARED_CREDENTIALS_FILE);
  NeedS3Config(S3ConfigFileName, S3ConfigTimestamp, S3ConfigFile, L"config", AWS_CONFIG_FILE);
}

void GetS3Profiles(TStrings * Profiles, TCustomIniFile * File, const UnicodeString & Prefix = EmptyStr)
{
  if (File != nullptr)
  {
    std::unique_ptr<TStrings> Sections(new TStringList());
    File->ReadSections(Sections.get());
    for (int32_t Index = 0; Index < Sections->Count; Index++)
    {
      UnicodeString Section = Sections->Strings[Index];
      if (Prefix.IsEmpty() || StartsText(Prefix, Section))
      {
        bool Supported = false;
        if (!File->ReadString(Section, AWS_ACCESS_KEY_ID, EmptyStr).IsEmpty() &&
            !File->ReadString(Section, AWS_SECRET_ACCESS_KEY, EmptyStr).IsEmpty())
        {
          Supported = true;
        }
        else if (!File->ReadString(Section, AWS_ROLE_ARN_KEY, EmptyStr).IsEmpty())
        {
          if (!File->ReadString(Section, AWS_SOURCE_PROFILE_KEY, EmptyStr).IsEmpty())
          {
            Supported = true;
          }
          else
          {
            UnicodeString CredentialSource = File->ReadString(Section, AWS_CREDENTIAL_SOURCE_KEY, EmptyStr);
            Supported =
              SameText(CredentialSource, AWS_CREDENTIAL_SOURCE_ENVIRONMENT) ||
              SameText(CredentialSource, AWS_CREDENTIAL_SOURCE_METADATA);
          }
        }

        if (Supported)
        {
          Profiles->Add(MidStr(Section, Prefix.Length() + 1));
        }
      }
    }
  }
}

TStrings * GetS3Profiles()
{
  NeedS3Config();
  // S3 allegedly treats the section case-sensitively, but our GetS3ConfigValue (ReadString) does not,
  // so consistently we return case-insensitive list.
  std::unique_ptr<TStrings> Result(CreateSortedStringList());
  GetS3Profiles(Result.get(), S3CredentialsFile.get());
  GetS3Profiles(Result.get(), S3ConfigFile.get(), AWS_CONFIG_PROFILE_PREFIX);
  return Result.release();
}

static THttp * CreateHttp(const UnicodeString & Url, int32_t ConnectTimeout)
{
  std::unique_ptr<THttp> Http(new THttp());
  Http->ResponseLimit = BasicHttpResponseLimit;
  Http->ConnectTimeout = ConnectTimeout;
  Http->URL = Url;
  return Http.release();
}

static UnicodeString ReadSecurityUrl(const UnicodeString & Url, int32_t ConnectTimeout = 0)
{
  std::unique_ptr<THttp> Http(CreateHttp(Url, ConnectTimeout));
  std::unique_ptr<TStrings> RequestHeaders(new TStringList());
  if (!S3SessionToken.IsEmpty())
  {
    RequestHeaders->Values[L"X-aws-ec2-metadata-token"] = S3SessionToken;
    Http->RequestHeaders = RequestHeaders.get();
  }
  Http->Get();
  return Http->Response.Trim();
}

static TDateTime ParseExpiration(const UnicodeString & S)
{
  return ISO8601ToDate(S, false);
}

static UnicodeString GetS3ConfigValue(
  TCustomIniFile * File, const UnicodeString & Profile, const UnicodeString & Name, const UnicodeString & Prefix, UnicodeString & Source)
{
  UnicodeString Result;
  if (File != nullptr)
  {
    UnicodeString ConfigSection = Profile;
    if (!SameText(ConfigSection, AWS_PROFILE_DEFAULT))
    {
      ConfigSection = Prefix + ConfigSection;
    }
    // This is not consistent with AWS CLI.
    // AWS CLI fails if one of aws_access_key_id or aws_secret_access_key is set and other is missing:
    // "Partial credentials found in shared-credentials-file, missing: aws_secret_access_key"
    Result = File->ReadString(ConfigSection, Name, EmptyStr);
    if (!Result.IsEmpty())
    {
      Source = FORMAT("%s/%s", ExtractFileName(File->FileName), Profile);
    }
  }
  return Result;
}

static UnicodeString GetS3ConfigValue(
  const UnicodeString & Profile, const UnicodeString & Name, UnicodeString & Source)
{
  UnicodeString Result = GetS3ConfigValue(S3CredentialsFile.get(), Profile, Name, EmptyStr, Source);
  if (Result.IsEmpty())
  {
    Result = GetS3ConfigValue(S3ConfigFile.get(), Profile, Name, AWS_CONFIG_PROFILE_PREFIX, Source);
  }
  return Result;
}

static UnicodeString GetS3ConfigValue(
  const UnicodeString & Profile, const UnicodeString & EnvName, const UnicodeString & AConfigName,
  const UnicodeString & CredentialsName, UnicodeString * Source, bool OnlyCached = false)
{
  UnicodeString Result;
  UnicodeString ASource;
  TGuard Guard(*LibS3Section.get());
  bool TryCredentials = true;

  try
  {
    if (Profile.IsEmpty())
    {
      Result = base::GetEnvironmentVariable(EnvName);
    }
    if (!Result.IsEmpty())
    {
      ASource = FORMAT("%%%s%%", EnvName);
    }
    else if (!AConfigName.IsEmpty())
    {
      NeedS3Config();

      UnicodeString AProfile = DefaultStr(Profile, S3Profile);
      UnicodeString ConfigName = DefaultStr(AConfigName, EnvName);
      Result = GetS3ConfigValue(AProfile, ConfigName, ASource);
      if (Result.IsEmpty())
      {
        UnicodeString SourceSource;
        UnicodeString SourceProfile = GetS3ConfigValue(AProfile, AWS_SOURCE_PROFILE_KEY, SourceSource);
        if (!SourceProfile.IsEmpty())
        {
          TryCredentials = false;
          Result = GetS3ConfigValue(SourceProfile, ConfigName, ASource);
        }
        else
        {
          UnicodeString CredentialSource = GetS3ConfigValue(AProfile, AWS_CREDENTIAL_SOURCE_KEY, SourceSource);
          if (!CredentialSource.IsEmpty())
          {
            TryCredentials = false;
            if (SameText(CredentialSource, AWS_CREDENTIAL_SOURCE_ENVIRONMENT))
            {
              Result = GetS3ConfigValue(EmptyStr, EnvName, EmptyStr, EmptyStr, &ASource);
            }
            else if (SameText(CredentialSource, AWS_CREDENTIAL_SOURCE_METADATA))
            {
              Result = GetS3ConfigValue(EmptyStr, EmptyStr, EmptyStr, CredentialsName, &ASource);
            }
          }
        }

        if (!Result.IsEmpty())
        {
          ASource = FORMAT("%s=>%s", SourceSource, ASource);
        }
      }
    }
  }
  catch (Exception & E)
  {
    throw ExtException(&E, MainInstructions(LoadStr(S3_CONFIG_ERROR)));
  }

  if (Result.IsEmpty() && TryCredentials && !CredentialsName.IsEmpty())
  {
    if (S3SecurityProfileChecked && (S3CredentialsExpiration != TDateTime()) && (IncHour(S3CredentialsExpiration, -1) < Now()))
    {
      AppLog("AWS session token or security credentials have expired or are close to expiration, will retrieve new");
      S3SecurityProfileChecked = false;
    }

    if (!S3SecurityProfileChecked && !OnlyCached)
    {
      S3Credentials.clear();
      S3SessionToken = EmptyStr;
      S3SecurityProfile = EmptyStr;
      S3SecurityProfileChecked = true;
      S3CredentialsExpiration = TDateTime();
      try
      {
        const UnicodeString AWSAPI = DefaultStr(Configuration->AWSAPI, L"http://169.254.169.254/latest/");

        int32_t ConnectTimeout = StrToIntDef(GetEnvironmentVariable("AWS_METADATA_SERVICE_TIMEOUT"), 1);
        UnicodeString TokenUrl = AWSAPI + L"api/token";

        AppLogFmt("Trying to create IMDSv2 session token via %s", TokenUrl);
        try
        {
          std::unique_ptr<THttp> Http(CreateHttp(TokenUrl, ConnectTimeout));
          int32_t TtlSeconds = 6 * 60 * 60; // max possible
          TDateTime TokenExpiration = IncSecond(Now(), TtlSeconds);
          std::unique_ptr<TStrings> RequestHeaders(new TStringList());
          RequestHeaders->Values[L"X-aws-ec2-metadata-token-ttl-seconds"] = IntToStr(TtlSeconds);
          Http->RequestHeaders = RequestHeaders.get();
          Http->Put(EmptyStr);
          S3SessionToken = Http->Response.Trim();
          S3CredentialsExpiration = TokenExpiration;
          AppLogFmt("Created IMDSv2 session token: %s, with expiration: %s", S3SessionToken, StandardTimestamp(TokenExpiration));
        }
        catch (Exception & E)
        {
          UnicodeString Message;
          ExceptionMessage(&E, Message);
          AppLogFmt("Error creating IMDSv2 session token: %s", Message);
        }

        const UnicodeString SecurityCredentialsUrl = AWSAPI + L"meta-data/iam/security-credentials/";
        AppLogFmt("Retrieving AWS security credentials from %s", SecurityCredentialsUrl);
        S3SecurityProfile = ReadSecurityUrl(SecurityCredentialsUrl, ConnectTimeout);

        if (S3SecurityProfile.IsEmpty())
        {
          AppLog("No AWS security credentials role detected");
        }
        else
        {
          UnicodeString SecurityProfileUrl = SecurityCredentialsUrl + EncodeUrlString(S3SecurityProfile);
          AppLogFmt("AWS security credentials role detected: %s, retrieving %s", S3SecurityProfile, SecurityProfileUrl);
          const UnicodeString ProfileDataStr = ReadSecurityUrl(SecurityProfileUrl);

          std::unique_ptr<TJSONValue> ProfileDataValue(TJSONObject::ParseJSONValue(ProfileDataStr));
          TJSONObject * ProfileData = dynamic_cast<TJSONObject *>(ProfileDataValue.get());
          if (ProfileData == nullptr)
          {
            throw new Exception(FORMAT("Unexpected response: %s", ProfileDataStr.SubString(1, 1000)));
          }
          TJSONValue * CodeValue = ProfileData->Values[L"Code"];
          if (CodeValue == nullptr)
          {
            throw new Exception("Missing \"Code\" value");
          }
          UnicodeString Code = CodeValue->Value();
          if (!SameText(Code, L"Success"))
          {
            throw new Exception(FORMAT("Received non-success code: %s", Code));
          }
          TJSONValue * ExpirationValue = ProfileData->Values[L"Expiration"];
          if (ExpirationValue == nullptr)
          {
            throw new Exception("Missing \"Expiration\" value");
          }
          UnicodeString ExpirationStr = ExpirationValue->Value();
          TDateTime CredentialsExpiration = ParseExpiration(ExpirationStr);
          AppLogFmt("Credentials expiration: %s", StandardTimestamp(CredentialsExpiration));
          if ((S3CredentialsExpiration == TDateTime()) ||
              (CredentialsExpiration < S3CredentialsExpiration))
          {
            S3CredentialsExpiration = CredentialsExpiration;
          }

          std::unique_ptr<TJSONObject::TEnumerator> Enumerator(ProfileData->GetEnumerator());
          UnicodeString Names;
          while (Enumerator->MoveNext())
          {
            TJSONPair * Pair = Enumerator->Current;
            UnicodeString Name = Pair->JsonString->Value();
            S3Credentials.insert(std::make_pair(Name, Pair->JsonValue->Value()));
            AddToList(Names, Name, L", ");
          }
          AppLogFmt("Response contains following values: %s", Names);
        }
      }
      catch (Exception & E)
      {
        UnicodeString Message;
        ExceptionMessage(&E, Message);
        AppLogFmt("Error retrieving AWS security credentials role: %s", Message);
      }
    }

    TS3Credentials::const_iterator I = S3Credentials.find(CredentialsName);
    if (I != S3Credentials.end())
    {
      Result = I->second;
      ASource = FORMAT("meta-data/%s", S3SecurityProfile);
    }
  }

  if (Source != nullptr)
  {
    *Source = ASource;
  }
  return Result;
}

UnicodeString S3EnvUserName(const UnicodeString & Profile, UnicodeString * Source)
{
  return GetS3ConfigValue(Profile, AWS_ACCESS_KEY_ID, L"AccessKeyId", Source);
}

UnicodeString S3EnvPassword(const UnicodeString & Profile, UnicodeString * Source)
{
  return GetS3ConfigValue(Profile, AWS_SECRET_ACCESS_KEY, L"SecretAccessKey", Source);
}

UnicodeString S3EnvSessionToken(const UnicodeString & Profile, UnicodeString * Source)
{
  return GetS3ConfigValue(Profile, AWS_SESSION_TOKEN, L"Token", Source);
}

UnicodeString S3EnvRoleArn(const UnicodeString & Profile, UnicodeString * Source, bool OnlyCached)
{
  return GetS3ConfigValue(Profile, AWS_ROLE_ARN, AWS_ROLE_ARN_KEY, EmptyStr, Source, OnlyCached);
}


constexpr const int32_t TS3FileSystem::S3MinMultiPartChunkSize = 5 * 1024 * 1024;
constexpr const int32_t TS3FileSystem::S3MaxMultiPartChunks = 10000;

#endif // defined(__BORLANDC__)

TS3FileSystem::TS3FileSystem(TTerminal * ATerminal) noexcept :
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

TS3FileSystem::~TS3FileSystem() noexcept
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

void TS3FileSystem::Open()
{

  FTlsVersionStr = L"";
  FNeonSession = nullptr;
  FCurrentDirectory = L"";
  FAuthRegion = DefaultStr(FTerminal->SessionData->S3DefaultRegion(), S3LibDefaultRegion());

  RequireNeon(FTerminal);

  FTerminal->Information(LoadStr(STATUS_CONNECT));

  TSessionData * Data = FTerminal->GetSessionData();

  FSessionInfo.LoginTime = Now();
  FSessionInfo.CertificateVerifiedManually = false;

  if (Data->Ftps != ftpsNone)
  {
    FLibS3Protocol = S3ProtocolHTTPS;
    RequireTls();
  }
  else
  {
    FLibS3Protocol = S3ProtocolHTTP;
  }

  UnicodeString S3Profile;
  if (Data->S3CredentialsEnv)
  {
    S3Profile = Data->S3Profile;
  }
  if (!S3Profile.IsEmpty() && !Data->FingerprintScan)
  {
#if defined(__BORLANDC__)
    std::unique_ptr<TStrings> S3Profiles(GetS3Profiles());
    if (S3Profiles->IndexOf(S3Profile) < 0)
    {
      throw Exception(MainInstructions(FMTLOAD(S3_PROFILE_NOT_EXIST, S3Profile)));
    }
#endif // defined(__BORLANDC__)
  }

  UnicodeString AccessKeyId = Data->UserNameExpanded;
  if (AccessKeyId.IsEmpty() && !Data->FingerprintScan)
  {
    if (!FTerminal->PromptUser(Data, pkUserName, LoadStr(S3_ACCESS_KEY_ID_TITLE), L"",
          LoadStr(S3_ACCESS_KEY_ID_PROMPT), true, 0, AccessKeyId))
    {
      FTerminal->FatalError(nullptr, LoadStr(CREDENTIALS_NOT_SPECIFIED));
    }
  }

  UnicodeString Password = Data->Password;
  if (Password.IsEmpty() && Data->FS3CredentialsEnv)
  {
#if defined(__BORLANDC__)
    UnicodeString PasswordSource;
    Password = S3EnvPassword(S3Profile, &PasswordSource);
    if (!Password.IsEmpty())
    {
      FTerminal->LogEvent(FORMAT("Password (secret access key) read from %s", PasswordSource));
    }
#endif // defined(__BORLANDC__)
  }
  UnicodeString SecretAccessKey = UTF8String(NormalizeString(Password)).data();
  if (SecretAccessKey.IsEmpty() && !Data->FingerprintScan)
  {
    if (!FTerminal->PromptUser(Data, pkPassword, LoadStr(S3_SECRET_ACCESS_KEY_TITLE), L"",
          LoadStr(S3_SECRET_ACCESS_KEY_PROMPT), false, 0, SecretAccessKey))
    {
      FTerminal->FatalError(nullptr, LoadStr(CREDENTIALS_NOT_SPECIFIED));
    }
  }

  UnicodeString SessionToken = Data->S3SessionToken;
  if (SessionToken.IsEmpty() && Data->S3CredentialsEnv)
  {
    UnicodeString SessionTokenSource;
#if defined(__BORLANDC__)
    SessionToken = S3EnvSessionToken(S3Profile, &SessionTokenSource);
#endif // defined(__BORLANDC__)
    if (!SessionToken.IsEmpty())
    {
      FTerminal->LogEvent(FORMAT("Session token read from %s", SessionTokenSource));
    }
  }

  SetCredentials(AccessKeyId, SecretAccessKey, SessionToken);

  FHostName = UTF8String(Data->HostNameExpanded);
  FPortSuffix = UTF8String();
  const int32_t ADefaultPort = Data->GetDefaultPort();
  DebugAssert((ADefaultPort == HTTPSPortNumber) || (ADefaultPort == HTTPPortNumber));
  if (Data->PortNumber != ADefaultPort)
  {
    FPortSuffix = UTF8String(FORMAT(":%d", Data->PortNumber));
  }
  FTimeout = Data->Timeout;

  RegisterForNeonDebug(FTerminal);
  UpdateNeonDebugMask();

  {
    const TGuard Guard(*LibS3Section.get());
    S3_initialize(nullptr, S3_INIT_ALL, nullptr);
  }

  if (IsGoogleCloud())
  {
    FTerminal->LogEvent("Google Cloud detected.");
  }

  S3_set_request_context_requester_pays(FRequestContext, (int)Data->S3RequesterPays());

  if (IsAmazonS3SessionData(Data))
  {
    UnicodeString RoleArn = Data->S3RoleArn;
    if (RoleArn.IsEmpty())
    {
#if defined(__BORLANDC__)
      UnicodeString RoleArnSource;
      RoleArn = S3EnvRoleArn(S3Profile, &RoleArnSource);
      if (!RoleArn.IsEmpty())
      {
        FTerminal->LogEvent(FORMAT("Role ARN read from %s", RoleArnSource));
      }
#endif // defined(__BORLANDC__)
    }

    if (!RoleArn.IsEmpty())
    {
      AssumeRole(RoleArn);
    }
  }

  FActive = false;
  try
  {
    UnicodeString Path = Data->RemoteDirectory;
    if (base::IsUnixRootPath(Path))
    {
      Path = ROOTDIRECTORY;
    }
    TryOpenDirectory(Path);
  }
  catch(Exception & E)
  {
    LibS3Deinitialize();
    FTerminal->Closed();
    FTerminal->FatalError(&E, LoadStr(CONNECTION_FAILED));
  }
  FActive = true;
}

void TS3FileSystem::SetCredentials(
  const UnicodeString & AccessKeyId, const UnicodeString & SecretAccessKey, const UnicodeString & SessionToken)
{
  FAccessKeyId = UTF8String(AccessKeyId);
  if (FAccessKeyId.Length() > S3_MAX_ACCESS_KEY_ID_LENGTH)
  {
    FAccessKeyId.SetLength(S3_MAX_ACCESS_KEY_ID_LENGTH);
  }
  FSecretAccessKey = UTF8String(SecretAccessKey);

  // without this check, error occurs:
  // There were headers present in the request which were not signed
  if (!SessionToken.IsEmpty())
  {
    FSecurityTokenBuf = UTF8String(SessionToken);
    FSecurityToken = FSecurityTokenBuf.data();
  }
}

struct TLibS3CallbackData
{
  CUSTOM_MEM_ALLOCATION_IMPL

  TLibS3CallbackData()
  {
    Status = static_cast<S3Status>(-1);
    FileSystem = nullptr;
  }

  TS3FileSystem * FileSystem{nullptr};
  S3Status Status{};
  UnicodeString RegionDetail;
  UnicodeString EndpointDetail;
  UnicodeString ErrorMessage;
  UnicodeString ErrorDetails;
};

TS3FileSystem * TS3FileSystem::GetFileSystem(void * CallbackData)
{
  return static_cast<TLibS3CallbackData *>(CallbackData)->FileSystem;
}

void TS3FileSystem::LibS3SessionCallback(ne_session_s * Session, void * CallbackData)
{
  TS3FileSystem * FileSystem = static_cast<TS3FileSystem *>(CallbackData);
  const TSessionData * Data = FileSystem->FTerminal->GetSessionData();

  InitNeonSession(
    Session, Data->GetProxyMethod(), Data->GetProxyHost(), Data->GetProxyPort(),
    Data->GetProxyUsername(), Data->GetProxyPassword(), FileSystem->FTerminal);

  SetNeonTlsInit(Session, TS3FileSystem::InitSslSession, FileSystem->FTerminal);

  ne_set_session_flag(Session, SE_SESSFLAG_SNDBUF, Data->FSendBuf);

  ne_set_session_private(Session, SESSION_FS_KEY, FileSystem);

  // Data->Timeout is propagated via timeoutMs parameter of functions like S3_list_service

  FileSystem->FNeonSession = Session;
}

void TS3FileSystem::InitSslSessionImpl(ssl_st * Ssl, void * /* ne_session */ Session)
{
  SetupSsl(Ssl, FTerminal->SessionData->MinTlsVersion, FTerminal->SessionData->MaxTlsVersion);

  UnicodeString CACert = FTerminal->SessionData->S3CACertificate;
  FTerminal->LogEvent(FORMAT(L"InitSslSessionImpl: S3CACertificate is %s", CACert.IsEmpty() ? L"(empty)" : L"(configured)"));

  if (!CACert.IsEmpty())
  {
    try
    {
      UnicodeString TempPath = SystemTemporaryDirectory();
      UnicodeString TempFileName = TempPath + L"NetBox_S3_CA_" + ::IntToStr(::GetCurrentProcessId()) + L"_" +
                                   ::IntToStr(reinterpret_cast<intptr_t>(this)) + L".pem";

      TFile::WriteAllText(TempFileName, CACert);

      FTerminal->LogEvent(L"InitSslSessionImpl: Wrote CA cert to temp file");
      FS3CACertificateTempFile = TempFileName;

      if (Session != nullptr)
      {
        ne_ssl_set_certificates_storage(static_cast<ne_session *>(Session), StrToNeon(FS3CACertificateTempFile));
        FTerminal->LogEvent(L"InitSslSessionImpl: Custom CA certificate applied to SSL context");
      }
      else
      {
        FTerminal->LogEvent(L"InitSslSessionImpl: Session is null, cannot apply CA certificate");
      }
    }
    catch(Exception & E)
    {
      FTerminal->LogEvent(FORMAT(L"InitSslSessionImpl: Failed to apply custom CA certificate: %s", E.Message));
    }
  }
}

int32_t TS3FileSystem::LibS3SslCallback(int32_t Failures, const ne_ssl_certificate_s * Certificate, void * CallbackData)
{
  TNeonCertificateData Data;
  RetrieveNeonCertificateData(Failures, Certificate, Data);
  TS3FileSystem * FileSystem = static_cast<TS3FileSystem *>(CallbackData);
  return FileSystem->VerifyCertificate(Data) ? NE_OK : NE_ERROR;
}

bool TS3FileSystem::VerifyCertificate(TNeonCertificateData & Data)
{
  const bool Result =
    FTerminal->VerifyOrConfirmHttpCertificate(
      FTerminal->SessionData->HostNameExpanded, FTerminal->SessionData->PortNumber, Data, true, FSessionInfo);

  if (Result)
  {
    CollectTLSSessionInfo();
  }

  return Result;
}

void TS3FileSystem::CollectTLSSessionInfo()
{
  // See also TFTPFileSystem::Open().
  // Have to cache the value as the connection (the neon HTTP session, not "our" session)
  // can be closed at the time we need it in CollectUsage().
  const UnicodeString Message = NeonTlsSessionInfo(FNeonSession, FSessionInfo, FTlsVersionStr);
  FTerminal->LogEvent(0, Message);
}

S3Status TS3FileSystem::LibS3ResponsePropertiesCallback(const S3ResponseProperties * /*Properties*/, void * /*CallbackData*/)
{

  // TODO
  return S3StatusOK;
}

void TS3FileSystem::LibS3ResponseDataCallback(const char * Data, size_t Size, void * CallbackData)
{
  TS3FileSystem * FileSystem = static_cast<TS3FileSystem *>(CallbackData);
  if (FileSystem->FTerminal->Log->Logging && !FileSystem->FResponseIgnore)
  {
    const UnicodeString Content = UnicodeString(UTF8String(Data, nb::ToInt32(Size))).Trim();
    FileSystem->FResponse += Content;
  }
}

void TS3FileSystem::LibS3ResponseCompleteCallback(S3Status Status, const S3ErrorDetails * Error, void * CallbackData)
{
  TLibS3CallbackData & Data = *static_cast<TLibS3CallbackData *>(CallbackData);

  const TS3FileSystem * FileSystem = Data.FileSystem;
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
      AddToList(ErrorDetails, FMTLOAD(S3_ERROR_RESOURCE, StrFromS3(Error->resource)), "\n");
    }
    if (Error->furtherDetails != nullptr)
    {
      AddToList(ErrorDetails, FMTLOAD(S3_ERROR_FURTHER_DETAILS, StrFromS3(Error->furtherDetails)), "\n");
    }
    if (Error->extraDetailsCount)
    {
      UnicodeString ExtraDetails;
      for (int32_t I = 0; I < Error->extraDetailsCount; I++)
      {
        UnicodeString DetailName = StrFromS3(Error->extraDetails[I].name);
        UnicodeString DetailValue = StrFromS3(Error->extraDetails[I].value);
        if (SameText(DetailName, "Region"))
        {
          Data.RegionDetail = DetailValue;
        }
        else if (SameText(DetailName, "Endpoint"))
        {
          Data.EndpointDetail = DetailValue;
        }
        AddToList(ExtraDetails, FORMAT("%s: %s", DetailName, DetailValue), ", ");
      }
      AddToList(ErrorDetails, LoadStr(S3_ERROR_EXTRA_DETAILS) + ExtraDetails, "\n");
    }

    if (!ErrorDetails.IsEmpty())
    {
      FileSystem->FTerminal->LogEvent(ErrorDetails);
      Data.ErrorDetails = ErrorDetails;
    }
  }

  if (!FileSystem->FResponse.IsEmpty() && (FileSystem->FTerminal->Configuration->ActualLogProtocol >= 0))
  {
    FileSystem->FTerminal->Log->Add(llOutput, FileSystem->FResponse);
  }
}

void TS3FileSystem::RequestInit(TLibS3CallbackData & Data)
{
  Data.FileSystem = this;
  FResponse = L"";
}

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
        Error = ReplaceStr(LoadStr(NET_TRANSL_HOST_NOT_EXIST2), "%HOST%", FTerminal->SessionData->HostNameExpanded);
        FatalCandidate = true;
        break;

      case S3StatusFailedToConnect:
        Error = LoadStr(CONNECTION_FAILED);
        FatalCandidate = true;
        break;

      case S3StatusConnectionFailed:
      case S3StatusErrorSlowDown:
        FatalCandidate = true;
        break;
    }

    if (!Error.IsEmpty())
    {
      Details = Data.ErrorMessage;
      AddToList(Details, Data.ErrorDetails, "\n");
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
      std::unique_ptr<Exception> E(std::make_unique<Exception>(Error));
      throw EFatal(E.get(), Details);
    }
    else
    {
      throw ExtException(Error, Details);
    }
  }
}

void TS3FileSystem::LibS3Deinitialize()
{
  const TGuard Guard(*LibS3Section.get());
  S3_deinitialize();
}

struct TLibS3XmlCallbackData : TLibS3CallbackData
{
  RawByteString Contents;
};

const UnicodeString AssumeRoleVersion(TraceInitStr("2011-06-15"));
const UnicodeString AssumeRoleNamespace(TraceInitStr(FORMAT("https://sts.amazonaws.com/doc/%s/", AssumeRoleVersion)));

#if defined(__BORLANDC__)

static _di_IXMLNode NeedNode(const _di_IXMLNodeList & NodeList, const UnicodeString & Name, const UnicodeString & Namespace)
{
  _di_IXMLNode Result = NodeList->FindNode(Name, Namespace);
  if (Result == nullptr)
  {
    throw Exception(FMTLOAD(S3_RESPONSE_ERROR, Name));
  }
  return Result;
}

static _di_IXMLNode AssumeRoleNeedNode(const _di_IXMLNodeList & NodeList, const UnicodeString & Name)
{
  return NeedNode(NodeList, Name, AssumeRoleNamespace);
}

static const _di_IXMLDocument CreateDocumentFromXML(const TLibS3XmlCallbackData & Data, TParseOptions ParseOptions)
{
  const _di_IXMLDocument Result = interface_cast<Xmlintf::IXMLDocument>(new TXMLDocument(nullptr));
  DebugAssert(Result->ParseOptions == TParseOptions());
  Result->ParseOptions = ParseOptions;
  Result->LoadFromXML(UTFToString(Data.Contents));
  return Result;
}

#endif // defined(__BORLANDC__)

void TS3FileSystem::AssumeRole(const UnicodeString & RoleArn)
{
  // According to AWS cli does, AWS_ROLE_SESSION_NAME does not apply here
  const UnicodeString RoleSessionName = DefaultStr(FTerminal->SessionData->S3RoleSessionName, GetAppNameString());

  try
  {
    TLibS3XmlCallbackData Data;
    RequestInit(Data);

    const UnicodeString QueryParams =
      FORMAT("Version=%s&Action=AssumeRole&RoleSessionName=%s&RoleArn=%s",
        AssumeRoleVersion, EncodeUrlString(RoleSessionName), EncodeUrlString(RoleArn));

    const UTF8String AuthRegionBuf = UTF8String(FAuthRegion);
    const UTF8String QueryParamsBuf = UTF8String(QueryParams);
    const UTF8String StsService = L"sts";
    const AnsiString StsHostName =
      AnsiString(ReplaceStr(S3LibDefaultHostName(), FORMAT("%s.", UnicodeString(S3_SERVICE)), FORMAT("%s.", UnicodeString(StsService))));
    DebugAssert(UnicodeString(StsHostName) != S3LibDefaultHostName());

    RequestParams AssumeRoleRequestParams =
    {
      HttpRequestTypeGET,
      {
        StsHostName.c_str(),
        nullptr,
        S3ProtocolHTTPS,
        S3UriStylePath, // Otherwise lib3s prefixes "(null)." (because of nullptr bucketName)
        FAccessKeyId.c_str(),
        FSecretAccessKey.c_str(),
        FSecurityToken,
        AuthRegionBuf.c_str(),
        StsService.c_str(),
      },
      nullptr,
      QueryParamsBuf.c_str(),
      nullptr, nullptr, nullptr, nullptr, 0, 0, nullptr, nullptr, nullptr, 0,
      LibS3XmlDataCallback,
      LibS3ResponseCompleteCallback,
      &Data,
      FTimeout
    };

    request_perform(&AssumeRoleRequestParams, FRequestContext);

    CheckLibS3Error(Data);

#if defined(__BORLANDC__)
    const _di_IXMLDocument Document = CreateDocumentFromXML(Data, TParseOptions());
    _di_IXMLNode ResponseNode = AssumeRoleNeedNode(Document->ChildNodes, "AssumeRoleResponse");
    _di_IXMLNode ResultNode = AssumeRoleNeedNode(ResponseNode->ChildNodes, "AssumeRoleResult");
    _di_IXMLNode CredentialsNode = AssumeRoleNeedNode(ResultNode->ChildNodes, "Credentials");
    UnicodeString AccessKeyId = AssumeRoleNeedNode(CredentialsNode->ChildNodes, "AccessKeyId")->Text;
    UnicodeString SecretAccessKey = AssumeRoleNeedNode(CredentialsNode->ChildNodes, "SecretAccessKey")->Text;
    UnicodeString SessionToken = AssumeRoleNeedNode(CredentialsNode->ChildNodes, "SessionToken")->Text;
    UnicodeString ExpirationStr = AssumeRoleNeedNode(CredentialsNode->ChildNodes, "Expiration")->Text;

    FTerminal->LogEvent(FORMAT("Assumed role \"%s\".", RoleArn));
    FTerminal->LogEvent(FORMAT("New acess key is: %s", AccessKeyId));
    if (Configuration->LogSensitive)
    {
      FTerminal->LogEvent(FORMAT("Secret access key: %s", SecretAccessKey));
      FTerminal->LogEvent(FORMAT("Session token: %s", SessionToken));
    }

    // Only logged for now
    TDateTime Expiration = ParseExpiration(ExpirationStr);
    FTerminal->LogEvent(FORMAT("Credentials expiration: %s", StandardTimestamp(Expiration)));

    SetCredentials(AccessKeyId, SecretAccessKey, SessionToken);
#endif // defined(__BORLANDC__)
  }
  catch (Exception & E)
  {
    throw ExtException(&E, MainInstructions(FMTLOAD(S3_ASSUME_ROLE_ERROR, RoleArn)));
  }
}

S3Status TS3FileSystem::LibS3XmlDataCallback(int32_t BufferSize, const char * Buffer, void * CallbackData)
{
  TLibS3XmlCallbackData & Data = *static_cast<TLibS3XmlCallbackData *>(CallbackData);
  if (Data.Contents.Length() + BufferSize > 1024 * 1024)
  {
    throw Exception("Too much data");
  }
  Data.Contents += RawByteString(Buffer, BufferSize);
  return S3StatusOK;
}

int32_t TS3FileSystem::LibS3XmlDataToCallback(int32_t BufferSize, char * Buffer, void * CallbackData)
{
  TLibS3XmlCallbackData & Data = *static_cast<TLibS3XmlCallbackData *>(CallbackData);
  const int32_t Len = std::min(Data.Contents.Length(), BufferSize);
  nbstr_memcpy(Buffer, Data.Contents.c_str(), Len);
  Data.Contents.Delete(1, Len);
  return Len;
}

UnicodeString TS3FileSystem::GetFolderKey(const UnicodeString & AKey) const
{
  return AKey + "/";
}

void TS3FileSystem::ParsePath(const UnicodeString & APath, UnicodeString & BucketName, UnicodeString & AKey)
{
  UnicodeString Path = APath;
  if (DebugAlwaysTrue(Path.SubString(1, 1) == SLASH))
  {
    Path.Delete(1, 1);
  }
  const int32_t P = Path.Pos("/");
  UnicodeString Result;
  nb::used(Result);
  if (P == 0)
  {
    BucketName = Path;
    AKey = "";
  }
  else
  {
    BucketName = Path.SubString(0, P - 1);
    AKey = Path.SubString(P + 1, Path.Length() - P);
    // Added logging for debugging path parsing
    FTerminal->LogEvent(FORMAT("ParsePath: input='%s', bucket='%s', key='%s'", APath, BucketName, AKey));
  }
}

struct TLibS3BucketContext : S3BucketContext
{
  CUSTOM_MEM_ALLOCATION_IMPL
  TLibS3BucketContext() = default;
  TLibS3BucketContext(const TLibS3BucketContext & rhs)
  {
    operator =(rhs);
  }
  TLibS3BucketContext & operator =(const TLibS3BucketContext & rhs)
  {
    if (this != &rhs)
    {
      HostNameBuf = rhs.HostNameBuf;
      BucketNameBuf = rhs.BucketNameBuf;
      AuthRegionBuf = rhs.AuthRegionBuf;

      hostName = HostNameBuf.c_str();
      bucketName = BucketNameBuf.c_str();
      protocol = rhs.protocol;
      uriStyle = rhs.uriStyle;
      accessKeyId = rhs.accessKeyId;
      secretAccessKey = rhs.secretAccessKey;
      securityToken = rhs.securityToken;
      authRegion = AuthRegionBuf.c_str();
    }
    return *this;
  }
  // These keep data that we point the native S3BucketContext fields to
  UTF8String HostNameBuf;
  UTF8String BucketNameBuf;
  UTF8String AuthRegionBuf;
};

struct TLibS3ListBucketCallbackData : TLibS3CallbackData
{
  CUSTOM_MEM_ALLOCATION_IMPL
  TRemoteFileList * FileList{nullptr};
  bool Any{false};
  int32_t KeyCount{0};
  UTF8String NextMarker;
  bool IsTruncated{false};
};

TLibS3BucketContext TS3FileSystem::GetBucketContext(const UnicodeString & ABucketName, const UnicodeString & Prefix)
{
  TLibS3BucketContext Result{};
  bool First = true;
  int32_t RetryCount = 0;
  const int32_t MaxRetries = 3;
  bool Retry = false;
  do
  {
      FTerminal->LogEvent(FORMAT("GetBucketContext: bucket='%s', retry=%d (count=%d)", ABucketName, Retry, RetryCount));
    TRegions::const_iterator I = FRegions.find(ABucketName);
    UnicodeString Region;
    if (I != FRegions.end())
    {
      Region = I->second;
    }
    else
    {
      Region = FAuthRegion;
      if (First)
      {
        FTerminal->LogEvent(FORMAT("Unknown bucket \"%s\", will detect its region (and service endpoint)", ABucketName));
        First = false;
      }
      Retry = true;
    }

    S3UriStyle UriStyle = static_cast<S3UriStyle>(FTerminal->SessionData->FS3UrlStyle);

    I = FHostNames.find(ABucketName);
    UnicodeString HostName;
    if (I != FHostNames.end())
    {
      HostName = I->second;
      if (SameText(HostName.SubString(1, ABucketName.Length() + 1), ABucketName + "."))
      {
        HostName.Delete(1, ABucketName.Length() + 1);
        // Even when using path-style URL Amazon seems to redirect us to bucket hostname and
        // we need to switch to virtual host style URL (without bucket name in the path)
        UriStyle = S3UriStyleVirtualHost;
      }
    }
    else
    {
      HostName = UnicodeString(FHostName);
    }

    Result.HostNameBuf = UTF8String(HostName + UnicodeString(FPortSuffix));
    Result.hostName = Result.HostNameBuf.c_str();
    Result.BucketNameBuf = UTF8String(ABucketName);
    Result.bucketName = Result.BucketNameBuf.c_str();
    Result.protocol = FLibS3Protocol;
    Result.uriStyle = UriStyle;
    Result.accessKeyId = FAccessKeyId.c_str();
    Result.secretAccessKey = FSecretAccessKey.c_str();
    Result.securityToken = FSecurityToken;
    Result.AuthRegionBuf = UTF8String(Region);
    if (Result.AuthRegionBuf.Length() > S3_MAX_REGION_LENGTH)
    {
      Result.AuthRegionBuf.SetLength(S3_MAX_REGION_LENGTH);
    }
    Result.authRegion = Result.AuthRegionBuf.c_str();
    Result.service = nullptr;

    if (Retry)
    {
      std::unique_ptr<TRemoteFileList> FileList(std::make_unique<TRemoteFileList>());
      TLibS3ListBucketCallbackData Data{};
      // Using prefix for which we need the bucket, as the account may have access to that prefix only (using "Condition" in policy)
      DoListBucket(Prefix, FileList.get(), 1, Result, Data);

      Retry = false;
      UnicodeString EndpointDetail = Data.EndpointDetail;
      if ((Data.Status == S3StatusErrorAuthorizationHeaderMalformed) &&
          (Region != Data.RegionDetail))
      {
        FTerminal->LogEvent(FORMAT("Will use region \"%s\" for bucket \"%s\" from now on.", Data.RegionDetail, ABucketName));
        FRegions.emplace(TRegions::value_type(ABucketName, Data.RegionDetail));

        Result.AuthRegionBuf = UTF8String(Data.RegionDetail);
        Result.authRegion = Result.AuthRegionBuf.c_str();
      }
      // happens with newly created buckets (and happens before the region redirect)
      else if (((Data.Status == S3StatusErrorTemporaryRedirect) || (Data.Status == S3StatusErrorPermanentRedirect)) &&
               !Data.EndpointDetail.IsEmpty())
      {
        UnicodeString Endpoint = Data.EndpointDetail;
        if (HostName != Endpoint)
        {
          FTerminal->LogEvent(FORMAT("Will use endpoint \"%s\" for bucket \"%s\" from now on.", Endpoint, ABucketName));
          FHostNames.emplace(TRegions::value_type(ABucketName, Endpoint));
          Retry = true;
        }
      }
      // Minio
      else if (Data.Status == S3StatusOK)
      {
        FTerminal->LogEvent(FORMAT("Will keep using region \"%s\" for bucket \"%s\" from now on.", FAuthRegion, ABucketName));
        FRegions[ABucketName] = FAuthRegion;
      }
    }
    // Increment retry counter and enforce limit
    RetryCount++;
    if (RetryCount >= MaxRetries)
    {
      FTerminal->LogEvent(FORMAT("Max retries (%d) reached for bucket '%s' (region='%s', endpoint='%s'), aborting", MaxRetries, ABucketName, Region, HostName));
      Retry = false;
    }
  }
  while (Retry);

  return Result;
}

#define CreateResponseHandlerCustom(PropertiesCallback) { &PropertiesCallback, &LibS3ResponseCompleteCallback }
#define CreateResponseHandler() CreateResponseHandlerCustom(LibS3ResponsePropertiesCallback)

void TS3FileSystem::Close()
{
  DebugAssert(FActive);
  LibS3Deinitialize();
  FTerminal->Closed();
  FActive = false;
  UnregisterFromNeonDebug(FTerminal);
  // Clean up temporary CA certificate file if created
  if (!FS3CACertificateTempFile.IsEmpty())
  {
    ::DeleteFileW(FS3CACertificateTempFile.c_str());
    FS3CACertificateTempFile.Clear();
  }
}

bool TS3FileSystem::GetActive() const
{
  return FActive;
}

void TS3FileSystem::CollectUsage()
{
  if (IsAmazonS3SessionData(FTerminal->SessionData))
  {
    FTerminal->Configuration->Usage->Inc("OpenedSessionsS3Amazon");
  }
  else
  {
    FTerminal->Configuration->Usage->Inc("OpenedSessionsS3Other");
  }
}

const TSessionInfo & TS3FileSystem::GetSessionInfo() const
{
  return FSessionInfo;
}

const TFileSystemInfo & TS3FileSystem::GetFileSystemInfo(bool /*Retrieve*/)
{
  return FFileSystemInfo;
}

bool TS3FileSystem::TemporaryTransferFile(const UnicodeString & /*AFileName*/)
{
  return false;
}

bool TS3FileSystem::GetStoredCredentialsTried() const
{
  // if we have one, we always try it
  return !FTerminal->SessionData->Password().IsEmpty();
}

UnicodeString TS3FileSystem::GetUserName() const
{
  return UnicodeString(FAccessKeyId);
}

void TS3FileSystem::Idle()
{
  // noop
}

UnicodeString TS3FileSystem::AbsolutePath(const UnicodeString & Path, bool /*Local*/) const
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

bool TS3FileSystem::IsCapable(int32_t Capability) const
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
    case fcSkipTransfer:
    case fcParallelTransfers:
    case fcLoadingAdditionalProperties:
    case fcAclChangingFiles:
    case fcMoveOverExistingFile:
    case fcTags:
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
    case fcTransferOut:
    case fcTransferIn:
    case fcParallelFileTransfers:
      return false;

    default:
      DebugFail();
      return false;
  }
}

UnicodeString TS3FileSystem::GetCurrentDirectory() const
{
  return FCurrentDirectory;
}

void TS3FileSystem::DoStartup()
{
  // Capabilities of S3 protocol are fixed
  FTerminal->SaveCapabilities(FFileSystemInfo);
  /* === NETBOX: gh-441 ===
  FTerminal->SetExceptionOnFail(true);
  // retrieve initialize working directory to save it as home directory
  ReadCurrentDirectory();
  FTerminal->SetExceptionOnFail(false);
  */
}

void TS3FileSystem::LookupUsersGroups()
{
  DebugFail();
}

void TS3FileSystem::ReadCurrentDirectory()
{
  if (FCachedDirectoryChange.IsEmpty())
  {
    FCurrentDirectory = FCurrentDirectory.IsEmpty() ? UnicodeString("/") : FCurrentDirectory;
  }
  else
  {
    FCurrentDirectory = FCachedDirectoryChange;
    FCachedDirectoryChange = "";
  }
}

void TS3FileSystem::HomeDirectory()
{
  ChangeDirectory("/");
}

void TS3FileSystem::AnnounceFileListOperation()
{
  // noop
}

void TS3FileSystem::TryOpenDirectory(const UnicodeString & ADirectory)
{
  FTerminal->LogEvent(FORMAT("Trying to open directory \"%s\".", ADirectory));
  std::unique_ptr<TRemoteFileList> FileList(std::make_unique<TRemoteFileList>());
  ReadDirectoryInternal(ADirectory, FileList.get(), -1, UnicodeString());
}

void TS3FileSystem::ChangeDirectory(const UnicodeString & ADirectory)
{
  const UnicodeString Path = AbsolutePath(ADirectory, false);

  // to verify existence of directory try to open it
  TryOpenDirectory(Path);

  // if open dir did not fail, directory exists -> success.
  FCachedDirectoryChange = Path;
}

void TS3FileSystem::CachedChangeDirectory(const UnicodeString & Directory)
{
  FCachedDirectoryChange = base::UnixExcludeTrailingBackslash(Directory);
}

TRemoteToken TS3FileSystem::MakeRemoteToken(const char * OwnerId, const char * OwnerDisplayName)
{
  TRemoteToken Result;
  Result.SetName(StrFromS3(OwnerDisplayName));
  if (Result.GetName().IsEmpty())
  {
    Result.SetName(StrFromS3(OwnerId));
  }
  return Result;
}

struct TLibS3ListServiceCallbackData : TLibS3CallbackData
{
  CUSTOM_MEM_ALLOCATION_IMPL

  TRemoteFileList * FileList{nullptr};
  UnicodeString FileName; // filter for buckets
};

S3Status TS3FileSystem::LibS3ListServiceCallback(
  const char * OwnerId, const char * OwnerDisplayName, const char * BucketName,
  int64_t /*CreationDate*/, void * CallbackData)
{
  TLibS3ListServiceCallbackData & Data = *static_cast<TLibS3ListServiceCallbackData *>(CallbackData);

  const UnicodeString FileName = StrFromS3(BucketName);
  if (Data.FileName.IsEmpty() || (Data.FileName == FileName))
  {
    std::unique_ptr<TRemoteFile> File(std::make_unique<TRemoteFile>(nullptr));
    const TTerminal * Terminal = Data.FileSystem->FTerminal;
    File->Terminal = Terminal;
    File->FileName = StrFromS3(BucketName);
    File->Type = FILETYPE_DIRECTORY;
    File->SetFileOwner(Data.FileSystem->MakeRemoteToken(OwnerId, OwnerDisplayName));
    File->ModificationFmt = mfNone;
    if (Terminal->IsValidFile(File.get()))
    {
      Data.FileList->AddFile(File.release());
    }
  }

  return S3StatusOK;
}

S3Status TS3FileSystem::LibS3ListBucketCallback(
  int32_t IsTruncated, const char * NextMarker, int32_t ContentsCount, const S3ListBucketContent * Contents,
  int32_t CommonPrefixesCount, const char ** CommonPrefixes, void * CallbackData)
{
  TLibS3ListBucketCallbackData & Data = *static_cast<TLibS3ListBucketCallbackData *>(CallbackData);

  Data.IsTruncated = IsTruncated;
  // This is being called in chunks, not once for all data in a response.
  Data.KeyCount += ContentsCount;
  Data.NextMarker = StrFromS3(NextMarker);
  TTerminal * Terminal = Data.FileSystem->FTerminal;

  Terminal->LogEvent(1, FORMAT("ListBucket callback: contents=%d, prefixes=%d", ContentsCount, CommonPrefixesCount));
  for (int32_t Index = 0; Index < ContentsCount; Index++)
  {
    Data.Any = true;
    const S3ListBucketContent * Content = &Contents[Index];
    UnicodeString FileName = base::UnixExtractFileName(StrFromS3(Content->key));
    if (!FileName.IsEmpty())
    {
      std::unique_ptr<TRemoteFile> File(std::make_unique<TRemoteFile>(nullptr));
      File->SetTerminal(Terminal);
      File->SetFileName(FileName);
      File->SetType(FILETYPE_DEFAULT);

      // ISO8601 timestamp parsing with 24:00 handling
      #define ISO8601_FORMAT "%04d-%02d-%02dT%02d:%02d:%02d"
      int32_t Year = 0;
      int32_t Month = 0;
      int32_t Day = 0;
      int32_t Hour = 0;
      int32_t Min = 0;
      int32_t Sec = 0;
      // The libs3's parseIso8601Time uses mktime, so returns a local time, which we would have to complicatedly restore,
      // Doing own parting instead as it's easier.
      // Might be replaced with ISO8601ToDate.
      // Keep is sync with WebDAV.
      // const int32_t Filled =
      //   sscanf(Content->lastModifiedStr, ISO8601_FORMAT, &Year, &Month, &Day, &Hour, &Min, &Sec);
      const int32_t Filled = sscanf(Content->lastModifiedStr, ISO8601_FORMAT, &Year, &Month, &Day, &Hour, &Min, &Sec);
      if (Filled == 6)
      {
        // TDateTime Modification =
        //   EncodeDateVerbose(static_cast<uint16_t>(Year), static_cast<uint16_t>(Month), static_cast<uint16_t>(Day)) +
        //   EncodeTimeVerbose(static_cast<uint16_t>(Hour), static_cast<uint16_t>(Min), static_cast<uint16_t>(Sec), 0);
        // File->Modification = ConvertTimestampFromUTC(Modification);
        // File->ModificationFmt = mfFull;
        // Normalize 24:00:00 to next day 00:00:00 if needed (ISO 8601 allows 24:00)
        if (Hour == 24)
        {
          Hour = 0;
          // Increment day, letting TDateTime handle month/year rollover
          TDateTime Date = EncodeDateVerbose(static_cast<uint16_t>(Year), static_cast<uint16_t>(Month), static_cast<uint16_t>(Day));
          Date = Date + 1; // add one day
          uint16_t NewYear, NewMonth, NewDay;
          DecodeDate(Date, NewYear, NewMonth, NewDay);
          Year = NewYear;
          Month = NewMonth;
          Day = NewDay;
        }
        // Validate hour range after normalization
        if (Hour >= 0 && Hour < 24)
        {
          TDateTime Modification = EncodeDateVerbose(static_cast<uint16_t>(Year), static_cast<uint16_t>(Month), static_cast<uint16_t>(Day)) +
            EncodeTimeVerbose(static_cast<uint16_t>(Hour), static_cast<uint16_t>(Min), static_cast<uint16_t>(Sec), 0);
          File->Modification = ConvertTimestampFromUTC(Modification);
          File->ModificationFmt = mfFull;
        }
        else
        {
          Terminal->LogEvent(FORMAT("Invalid hour value %d in timestamp for '%s'", Hour, FileName));
          File->ModificationFmt = mfNone;
        }
      }
      else
      {
        File->ModificationFmt = mfNone;
      }

      File->Size = static_cast<int64_t>(Content->size);
      File->SetFileOwner(Data.FileSystem->MakeRemoteToken(Content->ownerId, Content->ownerDisplayName));
      if (Terminal->IsValidFile(File.get()))
      {
        Data.FileList->AddFile(File.release());
      }
    }
  }

  for (int32_t Index = 0; Index < CommonPrefixesCount; Index++)
  {
    Data.Any = true;
    UnicodeString CommonPrefix = StrFromS3(CommonPrefixes[Index]);
    UnicodeString FileName = base::UnixExtractFileName(base::UnixExcludeTrailingBackslash(CommonPrefix));
    // Have seen prefixes like "/" or "path/subpath//"
    if (!FileName.IsEmpty())
    {
      std::unique_ptr<TRemoteFile> File(std::make_unique<TRemoteFile>(nullptr));
      File->Terminal = Data.FileSystem->FTerminal;
      File->FileName = FileName;
      File->Type = FILETYPE_DIRECTORY;
      File->ModificationFmt = mfNone;
      if (Terminal->IsValidFile(File.get()))
      {
        Data.FileList->AddFile(File.release());
      }
    }
  }

  return S3StatusOK;
}

void TS3FileSystem::DoListBucket(
  const UnicodeString & APrefix, TRemoteFileList * FileList, int32_t MaxKeys, const TLibS3BucketContext & BucketContext,
  TLibS3ListBucketCallbackData & Data)
{
  const S3ListBucketHandler ListBucketHandler = { CreateResponseHandler(), &LibS3ListBucketCallback };
  RequestInit(Data);
  Data.KeyCount = 0;
  Data.FileList = FileList;
  Data.IsTruncated = false;

  S3_list_bucket(
    static_cast<const S3BucketContext *>(&BucketContext), StrToS3(APrefix), StrToS3(Data.NextMarker),
    LibS3Delimiter, nb::ToInt32(MaxKeys), FRequestContext, FTimeout, &ListBucketHandler, &Data);
}

void TS3FileSystem::HandleNonBucketStatus(TLibS3CallbackData & Data, bool & Retry)
{
  if ((Data.Status == S3StatusErrorAuthorizationHeaderMalformed) &&
      (FAuthRegion != Data.RegionDetail))
  {
    FTerminal->LogEvent(FORMAT("Will use authentication region \"%s\" from now on.", Data.RegionDetail));
    FAuthRegion = Data.RegionDetail;
    Retry = true;
  }
}

bool TS3FileSystem::IsGoogleCloud() const
{
  return SameText(S3GoogleCloudHostName, FTerminal->SessionData->HostNameExpanded);
}

void TS3FileSystem::ReadDirectoryInternal(
  const UnicodeString & APath, TRemoteFileList * FileList, int32_t MaxKeys, const UnicodeString & AFileName)
{
  const UnicodeString Path = base::UnixExcludeTrailingBackslash(AbsolutePath(APath, false));
  int32_t AMaxKeys = (MaxKeys == -1) ? 1 : MaxKeys;
  if (base::IsUnixRootPath(Path))
  {
    DebugAssert(FileList != nullptr);

    TLibS3ListServiceCallbackData Data{};
    Data.FileList = FileList;
    Data.FileName = AFileName;

    bool Retry;
    do
    {
      RequestInit(Data);

      S3ListServiceHandler ListServiceHandler = { CreateResponseHandler(), &LibS3ListServiceCallback };

      Retry = false;

      if ((FTerminal->SessionData->FS3MaxKeys == asOff) ||
          ((FTerminal->SessionData->FS3MaxKeys == asAuto) && IsGoogleCloud()))
      {
        if (AMaxKeys != 0)
        {
          FTerminal->LogEvent(1, "Not limiting keys.");
          AMaxKeys = 0;
        }
      }

      S3_list_service(
        FLibS3Protocol, FAccessKeyId.c_str(), FSecretAccessKey.c_str(), FSecurityToken, (FHostName + FPortSuffix).c_str(),
        StrToS3(FAuthRegion), nb::ToInt32(AMaxKeys), FRequestContext, FTimeout, &ListServiceHandler, &Data);

      HandleNonBucketStatus(Data, Retry);
    }
    while (Retry);

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
    const TLibS3BucketContext BucketContext = GetBucketContext(BucketName, Prefix);

    TLibS3ListBucketCallbackData Data{};
    Data.Any = false;

    bool Continue;

    do
    {
      FTerminal->LogEvent(FORMAT("ReadDirectoryInternal: path='%s', bucket='%s', prefix='%s', maxKeys=%d", Path, BucketName, Prefix, AMaxKeys));
      DoListBucket(Prefix, FileList, AMaxKeys, BucketContext, Data);
      CheckLibS3Error(Data);

      Continue = false;

      if (Data.IsTruncated)
      {
        // We have report that with max-keys=1, server can return IsTruncated response with no keys,
        // so we would loop infinitely. For now, if we do GET request only to check for bucket/folder existence (MaxKeys == -1),
        // we are happy with a successful response and never loop, even if IsTruncated.
        if ((MaxKeys == 0) ||
            ((MaxKeys > 0) && (Data.KeyCount < MaxKeys)))
        {
          bool Cancel = false;
          FTerminal->DoReadDirectoryProgress(FileList->Count, false, Cancel);
          if (!Cancel)
          {
            Continue = true;
          }
        }
      }
    } while (Continue);
    FTerminal->LogEvent(FORMAT("ListBucket result: keyCount=%d, truncated=%d, any=%d", Data.KeyCount, Data.IsTruncated, Data.Any));

    // Listing bucket root directory will report an error if the bucket does not exist.
    // But there won't be any prefix/ entry, so if the bucket is empty, the Data.Any is false.
    // But when listing a prefix, we do not get any error, when the "prefix" does not exist.
    // But when the prefix does exist, there's at least the prefix/ entry. If there's none, it means that the path does not exist.
    // Even an empty-named entry/subprefix (which are ignored for other purposes) still indicate that the prefix exists.
    if (Prefix.IsEmpty() || Data.Any)
    {
      FileList->AddFile(new TRemoteParentDirectory(FTerminal));
    }
    else
    {
      // When called from DoReadFile (FileName is set), leaving error handling to the caller.
      if (AFileName.IsEmpty())
      {
        throw Exception(FMTLOAD(FILE_NOT_EXISTS, APath));
      }
    }
  }
}

void TS3FileSystem::ReadDirectory(TRemoteFileList * FileList)
{
  const TOperationVisualizer Visualizer(FTerminal->GetUseBusyCursor());
  ReadDirectoryInternal(FileList->Directory, FileList, 0, UnicodeString());
}

void TS3FileSystem::ReadSymlink(TRemoteFile * /*SymlinkFile*/,
  TRemoteFile *& /*File*/)
{
  // we never set SymLink flag, so we should never get here
  DebugFail();
}

void TS3FileSystem::DoReadFile(const UnicodeString & AFileName, TRemoteFile *& AFile)
{
  const UnicodeString FileNameOnly = base::UnixExtractFileName(AFileName);
  std::unique_ptr<TRemoteFileList> FileList(std::make_unique<TRemoteFileList>());
  ReadDirectoryInternal(base::UnixExtractFileDir(AFileName), FileList.get(), 1, FileNameOnly);
  const TRemoteFile * File = FileList->FindFile(FileNameOnly);
  if (File != nullptr)
  {
    AFile = File->Duplicate();
  }
  else
  {
    AFile = nullptr;
  }
}

void TS3FileSystem::ReadFile(const UnicodeString & AFileName,
  TRemoteFile *& File)
{
  const TOperationVisualizer Visualizer(FTerminal->GetUseBusyCursor());
  DoReadFile(AFileName, File);
  if (File == nullptr)
  {
    throw Exception(FMTLOAD(FILE_NOT_EXISTS, AFileName));
  }
}

void TS3FileSystem::DeleteFile(const UnicodeString & AFileName,
  const TRemoteFile * AFile, int32_t AParams, TRmSessionAction & Action)
{
  const UnicodeString FileName = AbsolutePath(AFileName, false);

  const bool Dir = FTerminal->DeleteContentsIfDirectory(FileName, AFile, AParams, Action);

  UnicodeString BucketName, Key;
  ParsePath(FileName, BucketName, Key);

  if (!Key.IsEmpty() && Dir)
  {
    Key = GetFolderKey(Key);
  }

  const TLibS3BucketContext BucketContext = GetBucketContext(BucketName, Key);

  const S3ResponseHandler ResponseHandler = CreateResponseHandler();

  TLibS3CallbackData Data{};
  RequestInit(Data);

  if (Key.IsEmpty())
  {
    S3_delete_bucket(
      BucketContext.protocol, BucketContext.uriStyle, BucketContext.accessKeyId, BucketContext.secretAccessKey,
      BucketContext.securityToken, BucketContext.hostName, BucketContext.bucketName, BucketContext.authRegion,
      FRequestContext, FTimeout, &ResponseHandler, &Data);
    CheckLibS3Error(Data);
  }
  else
  {
    S3_delete_object(&BucketContext, StrToS3(Key), FRequestContext, FTimeout, &ResponseHandler, &Data);
    try
    {
      CheckLibS3Error(Data);
    }
    catch (...)
    {
      if (FTerminal->Active && Dir && !FTerminal->FileExists(AFileName))
      {
        // Amazon silently ignores attempts to delete non existing folders,
        // But Google Cloud fails that.
        FTerminal->LogEvent("Folder does not exist anymore, it was probably only virtual");
      }
      else
      {
        throw;
      }
    }
  }
}

void TS3FileSystem::RenameFile(
  const UnicodeString & AFileName, const TRemoteFile * AFile, const UnicodeString & ANewName, bool Overwrite)
{
  if (DebugAlwaysTrue(AFile != nullptr) && AFile->GetIsDirectory())
  {
    NotSupported();
  }
  CopyFile(AFileName, AFile, ANewName, Overwrite);
  TRmSessionAction DummyAction(FTerminal->ActionLog, AFileName);
  DeleteFile(AFileName, AFile, dfForceDelete, DummyAction);
  DummyAction.Cancel();
}

void TS3FileSystem::CopyFile(
  const UnicodeString & AFileName, const TRemoteFile * AFile, const UnicodeString & ANewName, bool DebugUsedArg(Overwrite))
{
  if (DebugAlwaysTrue(AFile != nullptr) && AFile->GetIsDirectory())
  {
    throw Exception(LoadStr(DUPLICATE_FOLDER_NOT_SUPPORTED));
  }

  const UnicodeString FileName = AbsolutePath(AFileName, false);
  const UnicodeString NewName = AbsolutePath(ANewName, false);

  UnicodeString SourceBucketName, SourceKey;
  ParsePath(FileName, SourceBucketName, SourceKey);
  DebugAssert(!SourceKey.IsEmpty()); // it's not a folder, so it cannot be a bucket or root

  UnicodeString DestBucketName, DestKey;
  ParsePath(NewName, DestBucketName, DestKey);

  if (DestKey.IsEmpty())
  {
      FTerminal->LogEvent(FORMAT("Upload rejected: Key is empty. Bucket='%s', path='%s'", DestBucketName, NewName));
    throw Exception(LoadStr(MISSING_TARGET_BUCKET));
  }

  TLibS3BucketContext BucketContext = GetBucketContext(DestBucketName, DestKey);
  BucketContext.BucketNameBuf = UTF8String(SourceBucketName);
  BucketContext.bucketName = BucketContext.BucketNameBuf.c_str();

  const S3ResponseHandler ResponseHandler = CreateResponseHandler();

  TLibS3CallbackData Data{};
  RequestInit(Data);

  S3_copy_object(
    &BucketContext, StrToS3(SourceKey), StrToS3(DestBucketName), StrToS3(DestKey),
    nullptr, nullptr, 0, nullptr, FRequestContext, FTimeout, &ResponseHandler, &Data);

  CheckLibS3Error(Data);
}

void TS3FileSystem::CreateDirectory(const UnicodeString & ADirName, bool /*Encrypt*/)
{
  const TOperationVisualizer Visualizer(FTerminal->GetUseBusyCursor());
  const UnicodeString DirName = base::UnixExcludeTrailingBackslash(AbsolutePath(ADirName, false));

  UnicodeString BucketName, Key;
  ParsePath(DirName, BucketName, Key);

  if (Key.IsEmpty())
  {
    const S3ResponseHandler ResponseHandler = CreateResponseHandler();

    // Not using GetBucketContext here, as the bucket does not exist

    UTF8String RegionBuf;
    const char * Region = nullptr;
    if (!FTerminal->SessionData->S3DefaultRegion().IsEmpty() &&
        (FTerminal->SessionData->S3DefaultRegion() != S3LibDefaultRegion()))
    {
      RegionBuf = UTF8String(FTerminal->GetSessionData()->GetS3DefaultRegion());
      Region = RegionBuf.c_str();
    }

    TLibS3CallbackData Data{};

    bool Retry;
    do
    {
      RequestInit(Data);

      Retry = false;

      S3_create_bucket(
        FLibS3Protocol, FAccessKeyId.c_str(), FSecretAccessKey.c_str(), FSecurityToken,
        (FHostName + FPortSuffix).c_str(), StrToS3(BucketName),
        StrToS3(FAuthRegion), S3CannedAclPrivate, Region, FRequestContext, FTimeout, &ResponseHandler, &Data);

      HandleNonBucketStatus(Data, Retry);
    }
    while (Retry);

    CheckLibS3Error(Data);
  }
  else
  {
    Key = GetFolderKey(Key);

    const TLibS3BucketContext BucketContext = GetBucketContext(BucketName, Key);

    const S3PutObjectHandler PutObjectHandler = { CreateResponseHandler(), nullptr };

    TLibS3CallbackData Data{};
    RequestInit(Data);

    S3_put_object(&BucketContext, StrToS3(Key), 0, nullptr, FRequestContext, FTimeout, &PutObjectHandler, &Data);

    CheckLibS3Error(Data);
  }
}

void TS3FileSystem::CreateLink(const UnicodeString & FileName,
  const UnicodeString & PointTo, bool /*Symbolic*/)
{
  DebugUsedParam2(FileName, PointTo);
  DebugFail();
}

struct TS3FileProperties
{
  char OwnerId[S3_MAX_GRANTEE_USER_ID_SIZE]{};
  char OwnerDisplayName[S3_MAX_GRANTEE_DISPLAY_NAME_SIZE]{};
  int32_t AclGrantCount{0};
  S3AclGrant AclGrants[S3_MAX_ACL_GRANT_COUNT]{};
  UnicodeString Tags;
};

static TRights::TRightLevel S3PermissionToRightLevel(S3Permission Permission)
{
  TRights::TRightLevel Result;
  switch (Permission)
  {
    case S3PermissionRead: Result = TRights::rlS3Read; break;
    case S3PermissionWrite: Result = TRights::rlS3Write; break;
    case S3PermissionReadACP: Result = TRights::rlS3ReadACP; break;
    case S3PermissionWriteACP: Result = TRights::rlS3WriteACP; break;
    default: DebugFail(); Result = TRights::rlNone; break;
  }
  return Result;
}

bool TS3FileSystem::ParsePathForPropertiesRequests(
  const UnicodeString & Path, const TRemoteFile * File, UnicodeString & BucketName, UnicodeString & Key)
{
  const UnicodeString FileName = AbsolutePath(Path, false);

  ParsePath(FileName, BucketName, Key);

  const bool Result = !Key.IsEmpty();
  if (Result && File->IsDirectory)
  {
    Key = GetFolderKey(Key);
  }
  return Result;
}

const UnicodeString S3Version(TraceInitStr("2006-03-01"));
const UnicodeString S3Namespace(TraceInitStr(FORMAT("http://s3.amazonaws.com/doc/%s/", S3Version)));

#if defined(__BORLANDC__)
static _di_IXMLNode S3NeedNode(const _di_IXMLNodeList & NodeList, const UnicodeString & Name)
{
  return NeedNode(NodeList, Name, S3Namespace);
}
#endif // defined(__BORLANDC__)

#define COPY_BUCKET_CONTEXT(BucketContext) \
  { BucketContext.hostName, BucketContext.bucketName, BucketContext.protocol, BucketContext.uriStyle, \
    BucketContext.accessKeyId, BucketContext.secretAccessKey, BucketContext.securityToken, BucketContext.authRegion, \
    BucketContext.service }

bool TS3FileSystem::DoLoadFileProperties(
  const UnicodeString & AFileName, const TRemoteFile * File, TS3FileProperties & Properties, bool LoadTags)
{
  UnicodeString BucketName, Key;
  const bool Result = ParsePathForPropertiesRequests(AFileName, File, BucketName, Key);
  if (Result)
  {
    const TLibS3BucketContext BucketContext = GetBucketContext(BucketName, Key);

    const S3ResponseHandler ResponseHandler = CreateResponseHandler();

    TLibS3CallbackData AclData{};
    RequestInit(AclData);

    S3_get_acl(
      &BucketContext, StrToS3(Key), Properties.OwnerId, Properties.OwnerDisplayName,
      &Properties.AclGrantCount, Properties.AclGrants,
      FRequestContext, FTimeout, &ResponseHandler, &AclData);

    CheckLibS3Error(AclData);

    if (LoadTags)
    {
      TLibS3XmlCallbackData TagsData;
      RequestInit(TagsData);

      UTF8String KeyBuf = UTF8String(Key);
      RequestParams TaggingRequestParams =
      {
        HttpRequestTypeGET,
        COPY_BUCKET_CONTEXT(BucketContext),
        KeyBuf.c_str(),
        nullptr,
        "tagging",
        nullptr, nullptr, nullptr, 0, 0, nullptr,
        LibS3ResponsePropertiesCallback,
        nullptr, 0,
        LibS3XmlDataCallback,
        LibS3ResponseCompleteCallback,
        &TagsData,
        FTimeout
      };

      request_perform(&TaggingRequestParams, FRequestContext);

      if (TagsData.Status != S3StatusErrorAccessDenied)
      {
        CheckLibS3Error(TagsData);

#if defined(__BORLANDC__)
        const _di_IXMLDocument Document = CreateDocumentFromXML(TagsData, TParseOptions() << poPreserveWhiteSpace);
        _di_IXMLNode TaggingNode = S3NeedNode(Document->ChildNodes, "Tagging");
        _di_IXMLNode TagSetNode = S3NeedNode(TaggingNode->ChildNodes, "TagSet");
        _di_IXMLNodeList TagNodeList = TagSetNode->GetChildNodes();
        std::unique_ptr<TStrings> Tags(new TStringList());
        for (int32_t Index = 0; Index < TagNodeList->Count; Index++)
        {
          _di_IXMLNode TagNode = TagNodeList->Get(Index);
          UnicodeString Key = S3NeedNode(TagNode->ChildNodes, "Key")->Text;
          Tags->Add(Key);
          UnicodeString Value = S3NeedNode(TagNode->ChildNodes, "Value")->Text;
          Tags->Add(Value);
        }

        Properties.Tags = Tags->Text;
#endif // defined(__BORLANDC__)
      }
    }
  }
  return Result;
}

#if defined(__BORLANDC__)
typedef std::vector<S3AclGrant> TAclGrantsVector;
#endif // defined(__BORLANDC__)
using TAclGrantsVector = nb::vector_t<S3AclGrant>;
static void AddAclGrant(
  TRights::TRightGroup Group, uint16_t & Permissions, TAclGrantsVector & AclGrants,
  const S3AclGrant & AclGrantTemplate, S3Permission Permission)
{
  const TRights::TRightLevel Level = S3PermissionToRightLevel(Permission);
  const TRights::TFlag Flag = TRights::CalculateFlag(Group, Level);
  if (FLAGSET(Permissions, Flag))
  {
    S3AclGrant AclGrant(AclGrantTemplate);
    AclGrant.permission = Permission;
    AclGrants.push_back(AclGrant);
    Permissions -= static_cast<int16_t>(Flag);
  }
}

void TS3FileSystem::ChangeFileProperties(const UnicodeString & FileName,
  const TRemoteFile * File, const TRemoteProperties * Properties,
  TChmodSessionAction & /*Action*/)
{
  UnicodeString BucketName, Key;
  if (DebugAlwaysTrue(ParsePathForPropertiesRequests(FileName, File, BucketName, Key)))
  {
    TValidProperties ValidProperties = Properties->Valid;

    TLibS3BucketContext BucketContext = GetBucketContext(BucketName, Key);

    if (ValidProperties.Contains(vpRights))
    {
      ValidProperties >> vpRights;

      DebugAssert(!Properties->AddXToDirectories);

      TS3FileProperties FileProperties;
      if (DebugAlwaysTrue(!File->IsDirectory) &&
          DebugAlwaysTrue(DoLoadFileProperties(FileName, File, FileProperties, false)))
      {
        TAclGrantsVector NewAclGrants;

        uint16_t Permissions = File->Rights->Combine(Properties->Rights);
        for (int32_t GroupI = TRights::rgFirst; GroupI <= TRights::rgLast; GroupI++)
        {
          TRights::TRightGroup Group = static_cast<TRights::TRightGroup>(GroupI);
          S3AclGrant NewAclGrant;
          memset(&NewAclGrant, 0, sizeof(NewAclGrant));
          if (Group == TRights::rgUser)
          {
            NewAclGrant.granteeType = S3GranteeTypeCanonicalUser;
            DebugAssert(sizeof(NewAclGrant.grantee.canonicalUser.id) == sizeof(FileProperties.OwnerId));
            strcpy(NewAclGrant.grantee.canonicalUser.id, FileProperties.OwnerId);
          }
          else if (Group == TRights::rgS3AllAwsUsers)
          {
            NewAclGrant.granteeType = S3GranteeTypeAllAwsUsers;
          }
          else if (DebugAlwaysTrue(Group == TRights::rgS3AllUsers))
          {
            NewAclGrant.granteeType = S3GranteeTypeAllUsers;
          }
          uint16_t AllGroupPermissions =
            TRights::CalculatePermissions(Group, TRights::rlS3Read, TRights::rlS3ReadACP, TRights::rlS3WriteACP);
          if (FLAGSET(Permissions, AllGroupPermissions))
          {
            NewAclGrant.permission = S3PermissionFullControl;
            NewAclGrants.push_back(NewAclGrant);
            Permissions -= AllGroupPermissions;
          }
          else
          {
            #define ADD_ACL_GRANT(PERM) AddAclGrant(Group, Permissions, NewAclGrants, NewAclGrant, PERM)
            ADD_ACL_GRANT(S3PermissionRead);
            ADD_ACL_GRANT(S3PermissionWrite);
            ADD_ACL_GRANT(S3PermissionReadACP);
            ADD_ACL_GRANT(S3PermissionWriteACP);
          }
        }

        DebugAssert(Permissions == 0);

        // Preserve unrecognized permissions
        for (int32_t Index = 0; Index < FileProperties.AclGrantCount; Index++)
        {
          S3AclGrant & AclGrant = FileProperties.AclGrants[Index];
          uint16_t Permission = AclGrantToPermissions(AclGrant, FileProperties);
          if (Permission == 0)
          {
            NewAclGrants.push_back(AclGrant);
          }
        }

        S3ResponseHandler ResponseHandler = CreateResponseHandler();

        TLibS3CallbackData Data{};
        RequestInit(Data);

        S3_set_acl(
          &BucketContext, StrToS3(Key), FileProperties.OwnerId, FileProperties.OwnerDisplayName,
          nb::ToInt32(NewAclGrants.size()), &NewAclGrants[0],
          FRequestContext, FTimeout, &ResponseHandler, &Data);

        CheckLibS3Error(Data);
      }
    }

    if (ValidProperties.Contains(vpTags))
    {
      ValidProperties >> vpTags;

      const UnicodeString NewLine = "\n";
      const UnicodeString Indent = "  ";
      UnicodeString Xml =
        UnicodeString(XmlDeclaration) + NewLine +
        "<Tagging>" + NewLine +
        Indent + "<TagSet>" + NewLine;

      std::unique_ptr<TStrings> Tags(TextToStringList(Properties->Tags));
      for (int32_t Index = 0; Index < Tags->Count; Index += 2)
      {
        UnicodeString Key = Tags->Strings[Index];
        UnicodeString Value = Tags->Strings[Index + 1];
        Xml += Indent + Indent + FORMAT("<Tag><Key>%s</Key><Value>%s</Value></Tag>", XmlEscape(Key), XmlEscape(Value)) + NewLine;
      }

      Xml +=
        Indent + "</TagSet>" + NewLine +
        "</Tagging>" + NewLine;

      FTerminal->Log->Add(llOutput, Xml);

      TLibS3XmlCallbackData Data{};
      RequestInit(Data);

      Data.Contents = StrToS3(Xml);

      const UTF8String KeyBuf = UTF8String(Key);
      RequestParams TaggingRequestParams =
      {
        HttpRequestTypePUT,
        COPY_BUCKET_CONTEXT(BucketContext),
        KeyBuf.c_str(),
        nullptr,
        "tagging",
        nullptr, nullptr, nullptr, 0, 0, nullptr,
        LibS3ResponsePropertiesCallback,
        LibS3XmlDataToCallback,
        Data.Contents.Length(),
        nullptr,
        LibS3ResponseCompleteCallback,
        &Data,
        FTimeout
      };

      request_perform(&TaggingRequestParams, FRequestContext);

      CheckLibS3Error(Data);
    }

    DebugAssert(ValidProperties.Empty());
  }
}

uint16_t TS3FileSystem::AclGrantToPermissions(S3AclGrant & AclGrant, const TS3FileProperties & Properties)
{
  TRights::TRightGroup RightGroup = static_cast<TRights::TRightGroup>(-1);
  if (AclGrant.granteeType == S3GranteeTypeCanonicalUser)
  {
    if (strcmp(Properties.OwnerId, AclGrant.grantee.canonicalUser.id) == 0)
    {
      RightGroup = TRights::rgUser;
    }
    else
    {
      FTerminal->LogEvent(1, FORMAT("Unsupported permission for canonical user %s", StrFromS3(Properties.OwnerId)));
    }
  }
  else if (AclGrant.granteeType == S3GranteeTypeAllAwsUsers)
  {
    RightGroup = TRights::rgS3AllAwsUsers;
  }
  else if (AclGrant.granteeType == S3GranteeTypeAllUsers)
  {
    RightGroup = TRights::rgS3AllUsers;
  }
  uint16_t Result;
  if (RightGroup < 0)
  {
    Result = 0;
  }
  else
  {
    if (AclGrant.permission == S3PermissionFullControl)
    {
      Result = TRights::CalculatePermissions(RightGroup, TRights::rlS3Read, TRights::rlS3ReadACP, TRights::rlS3WriteACP);
    }
    else
    {
      DebugAssert(AclGrant.permission != S3PermissionWrite);
      const TRights::TRightLevel RightLevel = S3PermissionToRightLevel(AclGrant.permission);
      if (RightLevel == TRights::rlNone)
      {
        Result = 0;
      }
      else
      {
        Result = TRights::CalculateFlag(RightGroup, RightLevel);
      }
    }
  }
  return Result;
}

struct TLoadFilePropertiesData
{
  bool Result;
  bool LoadTags;
};

void TS3FileSystem::LoadFileProperties(const UnicodeString & AFileName, const TRemoteFile * AFile, void * Param)
{
  TRemoteFile * File = const_cast<TRemoteFile *>(AFile);
  TLoadFilePropertiesData & Data = *static_cast<TLoadFilePropertiesData *>(Param);
  TS3FileProperties Properties;
  Data.Result = DoLoadFileProperties(AFileName, File, Properties, Data.LoadTags);
  if (Data.Result)
  {
    bool AdditionalRights{false};
    uint16_t Permissions = 0;
    for (int32_t Index = 0; Index < Properties.AclGrantCount; Index++)
    {
      S3AclGrant & AclGrant = Properties.AclGrants[Index];
      const uint16_t Permission = AclGrantToPermissions(AclGrant, Properties);
      if (Permission == 0)
      {
        AdditionalRights = true;
      }
      else
      {
        Permissions |= Permission;
      }
    }

    const UnicodeString Delimiter(",");
    UnicodeString HumanRights;
    for (int32_t GroupI = TRights::rgFirst; GroupI <= TRights::rgLast; GroupI++)
    {
      const TRights::TRightGroup Group = static_cast<TRights::TRightGroup>(GroupI);
      #define RIGHT_LEVEL_SET(LEVEL) FLAGSET(Permissions, TRights::CalculateFlag(Group, TRights::LEVEL))
      const bool ReadRight = RIGHT_LEVEL_SET(rlS3Read);
      const bool WriteRight = DebugAlwaysFalse(RIGHT_LEVEL_SET(rlS3Write));
      const bool ReadACPRight = RIGHT_LEVEL_SET(rlS3ReadACP);
      const bool WriteACPRight = RIGHT_LEVEL_SET(rlS3WriteACP);
      UnicodeString Desc;
      if (ReadRight && ReadACPRight && WriteACPRight)
      {
        Desc = "F";
      }
      else if (ReadRight)
      {
        Desc = "R";
        if (ReadACPRight || WriteACPRight || WriteRight)
        {
          Desc += "+";
        }
      }

      if (!Desc.IsEmpty())
      {
        UnicodeString GroupDesc;
        switch (Group)
        {
          case TRights::rgUser: GroupDesc = "O"; break;
          case TRights::rgS3AllAwsUsers: GroupDesc = "U"; break;
          case TRights::rgS3AllUsers: GroupDesc = "E"; break;
          default: DebugFail(); break;
        }

        if (!GroupDesc.IsEmpty())
        {
          Desc = GroupDesc + ":" + Desc;
          AddToList(HumanRights, Desc, Delimiter);
        }
      }
    }

    if (AdditionalRights)
    {
      AddToList(HumanRights, "+", Delimiter);
    }

    File->Rights->Number = Permissions;
    File->Rights->SetTextOverride(HumanRights);
  }
}

bool TS3FileSystem::LoadFilesProperties(TStrings * FileList)
{
  TLoadFilePropertiesData Data{};
  Data.Result = false;
  Data.LoadTags = (FileList->Count == 1);
  FTerminal->BeginTransaction();
  try__finally
  {
    FTerminal->ProcessFiles(FileList, foGetProperties, nb::bind(&TS3FileSystem::LoadFileProperties, this), &Data);
  }
  __finally
  {
    FTerminal->EndTransaction();
  } end_try__finally
  return Data.Result;
}

void TS3FileSystem::CalculateFilesChecksum(
  const UnicodeString & DebugUsedArg(Alg), TStrings * DebugUsedArg(FileList), TCalculatedChecksumEvent &&,
  TFileOperationProgressType *, bool DebugUsedArg(FirstLevel))
{
  DebugFail();
}

void TS3FileSystem::CustomCommandOnFile(const UnicodeString & AFileName,
  const TRemoteFile * /*AFile*/, const UnicodeString & ACommand, int32_t /*AParams*/, TCaptureOutputEvent && /*OutputEvent*/)
{
  DebugUsedParam2(AFileName, ACommand);
  DebugFail();
}

void TS3FileSystem::AnyCommand(const UnicodeString & ACommand,
  TCaptureOutputEvent && /*OutputEvent*/)
{
  DebugUsedParam(ACommand);
  DebugFail();
}

TStrings * TS3FileSystem::GetFixedPaths() const
{
  return nullptr;
}

void TS3FileSystem::SpaceAvailable(const UnicodeString & APath,
  TSpaceAvailable & /*ASpaceAvailable*/)
{
  DebugUsedParam(APath);
  DebugFail();
}

void TS3FileSystem::CopyToRemote(
  TStrings * AFilesToCopy, const UnicodeString & ATargetDir, const TCopyParamType * CopyParam,
  int32_t AParams, TFileOperationProgressType * AOperationProgress, TOnceDoneOperation & OnceDoneOperation)
{
  AParams &= ~cpAppend;

  FTerminal->DoCopyToRemote(AFilesToCopy, ATargetDir, CopyParam, AParams, AOperationProgress, tfPreCreateDir, OnceDoneOperation);
}

void TS3FileSystem::ConfirmOverwrite(
  const UnicodeString & ASourceFullFileName, UnicodeString & ATargetFileName,
  TFileOperationProgressType * AOperationProgress, const TOverwriteFileParams * FileParams,
  const TCopyParamType * CopyParam, int32_t AParams)
{
  constexpr const uint32_t Answers = qaYes | qaNo | qaCancel | qaYesToAll | qaNoToAll;
  nb::vector_t<TQueryButtonAlias> Aliases(2);
  Aliases.emplace_back(TQueryButtonAlias::CreateYesToAllGroupedWithYes());
  Aliases.emplace_back(TQueryButtonAlias::CreateNoToAllGroupedWithNo());

  TQueryParams QueryParams(qpNeverAskAgainCheck);
  QueryParams.Aliases = &Aliases[0];
  QueryParams.AliasesCount = nb::ToUInt32(Aliases.size());

  uint32_t Answer;

  {
    const TSuspendFileOperationProgress Suspend(AOperationProgress);
    Answer =
      FTerminal->ConfirmFileOverwrite(
        ASourceFullFileName, ATargetFileName, FileParams, Answers, &QueryParams,
        ReverseOperationSide(AOperationProgress->GetSide()),
        CopyParam, AParams, AOperationProgress);
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
      AOperationProgress->SetCancelAtLeast(csCancel);
      Abort();
      break;
  }
}

struct TLibS3TransferObjectDataCallbackData : TLibS3CallbackData
{
  CUSTOM_MEM_ALLOCATION_IMPL

  UnicodeString FileName;
  TStream * Stream{nullptr};
  TFileOperationProgressType * OperationProgress{nullptr};
  std::unique_ptr<Exception> Exception;
};

struct TLibS3PutObjectDataCallbackData : TLibS3TransferObjectDataCallbackData
{
  CUSTOM_MEM_ALLOCATION_IMPL

  RawByteString ETag;
};

int32_t TS3FileSystem::LibS3PutObjectDataCallback(int32_t BufferSize, char * Buffer, void * CallbackData)
{
  TLibS3PutObjectDataCallbackData & Data = *static_cast<TLibS3PutObjectDataCallbackData *>(CallbackData);

  return nb::ToInt32(Data.FileSystem->PutObjectData(BufferSize, Buffer, Data));
}

bool TS3FileSystem::ShouldCancelTransfer(TLibS3TransferObjectDataCallbackData & Data)
{
  const bool Result = (Data.OperationProgress->GetCancel() != csContinue);
  if (Result)
  {
    if (Data.OperationProgress->ClearCancelFile())
    {
      Data.Exception = std::make_unique<ESkipFile>();
    }
    else
    {
      Data.Exception = std::make_unique<EAbort>("");
    }
  }
  return Result;
}

int64_t TS3FileSystem::PutObjectData(int32_t BufferSize, char * Buffer, TLibS3PutObjectDataCallbackData & Data)
{
  int64_t Result = -1;

  if (ShouldCancelTransfer(Data))
  {
    Result = -1;
  }
  else
  {
    TFileOperationProgressType * OperationProgress = Data.OperationProgress;
    try
    {
      FILE_OPERATION_LOOP_BEGIN
      {
        Result = Data.Stream->Read(Buffer, BufferSize);
      }
      FILE_OPERATION_LOOP_END(FMTLOAD(READ_ERROR, Data.FileName));

      OperationProgress->ThrottleToCPSLimit(Result);
      OperationProgress->AddTransferred(Result);
    }
    catch (Exception & E)
    {
      Data.Exception.reset(CloneException(&E));
      Result = -1;
    }
  }

  return Result;
}

struct TLibS3MultipartInitialCallbackData : TLibS3CallbackData
{
  CUSTOM_MEM_ALLOCATION_IMPL
  RawByteString UploadId;
};

S3Status TS3FileSystem::LibS3MultipartInitialCallback(const char * UploadId, void * CallbackData)
{
  TLibS3MultipartInitialCallbackData & Data = *static_cast<TLibS3MultipartInitialCallbackData *>(CallbackData);

  Data.UploadId = UploadId;

  return S3StatusOK;
}

struct TLibS3MultipartCommitPutObjectDataCallbackData : TLibS3CallbackData
{
  CUSTOM_MEM_ALLOCATION_IMPL
  RawByteString Message;
  int32_t Remaining{0};
};

S3Status TS3FileSystem::LibS3MultipartResponsePropertiesCallback(
  const S3ResponseProperties * Properties, void * CallbackData)
{
  const S3Status Result = LibS3ResponsePropertiesCallback(Properties, CallbackData);

  TLibS3PutObjectDataCallbackData & Data = *static_cast<TLibS3PutObjectDataCallbackData *>(CallbackData);

  Data.ETag = Properties->eTag;

  return Result;
}

int32_t TS3FileSystem::LibS3MultipartCommitPutObjectDataCallback(int32_t BufferSize, char * Buffer, void * CallbackData)
{
  TLibS3MultipartCommitPutObjectDataCallbackData & Data =
    *static_cast<TLibS3MultipartCommitPutObjectDataCallbackData *>(CallbackData);
  int32_t Result = 0;
  if (Data.Remaining > 0)
  {
    Result = std::min(BufferSize, Data.Remaining);
    nbstr_memcpy(Buffer, Data.Message.c_str() + Data.Message.Length() - Data.Remaining, Result);
    Data.Remaining -= Result;
  }
  return Result;
}

void TS3FileSystem::Source(
  TLocalFileHandle & AHandle, const UnicodeString & TargetDir, UnicodeString & ADestFileName,
  const TCopyParamType * CopyParam, int32_t Params,
  TFileOperationProgressType * OperationProgress, uint32_t /*Flags*/,
  TUploadSessionAction & Action, bool & /*ChildError*/)
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
    FileParams.DestSize = RemoteFile->Size();
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

  if (Key.IsEmpty())
  {
      FTerminal->LogEvent(FORMAT("Upload rejected: Key is empty. Bucket='%s', path='%s'", BucketName, DestFullName));
    throw Exception(LoadStr(MISSING_TARGET_BUCKET));
  }

  TLibS3BucketContext BucketContext = GetBucketContext(BucketName, Key);

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

  const int32_t Parts = std::min(S3MaxMultiPartChunks, std::max(1, nb::ToInt32((AHandle.Size + S3MinMultiPartChunkSize - 1) / S3MinMultiPartChunkSize)));
  const int32_t ChunkSize = std::max(S3MinMultiPartChunkSize, nb::ToInt32((AHandle.Size + Parts - 1) / Parts));
  DebugAssert((ChunkSize == S3MinMultiPartChunkSize) || (AHandle.Size > nb::ToInt64(S3MaxMultiPartChunks) * S3MinMultiPartChunkSize));

  bool Multipart = (Parts > 1);

  RawByteString MultipartUploadId;
  TLibS3MultipartCommitPutObjectDataCallbackData MultipartCommitPutObjectDataCallbackData;

  if (Multipart)
  {
    FTerminal->LogEvent(FORMAT("Initiating multipart upload (%d parts - chunk size %s)", Parts, IntToStr(ChunkSize)));

    FILE_OPERATION_LOOP_BEGIN
    {
      TLibS3MultipartInitialCallbackData Data;
      RequestInit(Data);

      S3MultipartInitialHandler Handler = { CreateResponseHandler(), &LibS3MultipartInitialCallback };

      S3_initiate_multipart(&BucketContext, StrToS3(Key), &PutProperties, &Handler, FRequestContext, FTimeout, &Data);

      CheckLibS3Error(Data, true);

      MultipartUploadId = Data.UploadId;
    }
    FILE_OPERATION_LOOP_END_EX(FMTLOAD(TRANSFER_ERROR, AHandle.FileName), (folAllowSkip | folRetryOnFatal));

    FTerminal->LogEvent(FORMAT("Initiated multipart upload (%s - %d parts)", UnicodeString(MultipartUploadId), Parts));

    MultipartCommitPutObjectDataCallbackData.Message += "<CompleteMultipartUpload>\n";
  }

  try
  {
    TLibS3PutObjectDataCallbackData Data{};

    int64_t Position = 0;

    std::unique_ptr<TStream> Stream(std::make_unique<TSafeHandleStream>(reinterpret_cast<THandle>(AHandle.Handle)));

    for (int32_t Part = 1; Part <= Parts; Part++)
    {
      FILE_OPERATION_LOOP_BEGIN
      {
        DebugAssert(Stream->Position() == OperationProgress->TransferredSize);

        // If not, it's chunk retry and we have to undo the unsuccessful chunk upload
        if (Position < Stream->Position())
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
          const int64_t Remaining = Stream->Size() - Stream->Position();
          const int32_t RemainingInt = nb::ToInt32(std::min(nb::ToInt64(std::numeric_limits<int32_t>::max()), Remaining));
          const int32_t PartLength = std::min(ChunkSize, RemainingInt);
          FTerminal->LogEvent(FORMAT("Uploading part %d [%s]", Part, IntToStr(PartLength)));
          S3_upload_part(
            &BucketContext, StrToS3(Key), &PutProperties, &UploadPartHandler, nb::ToInt32(Part), MultipartUploadId.c_str(),
            PartLength, FRequestContext, FTimeout, &Data);
        }
        else
        {
          const S3PutObjectHandler PutObjectHandler = { CreateResponseHandler(), LibS3PutObjectDataCallback };
          S3_put_object(&BucketContext, StrToS3(Key), nb::ToUInt64(AHandle.Size), &PutProperties, FRequestContext, FTimeout, &PutObjectHandler, &Data);
        }

        // The "exception" was already seen by the user, its presence mean an accepted abort of the operation.
        if (Data.Exception == nullptr)
        {
          CheckLibS3Error(Data, true);
        }

        Position = Stream->Position();

        if (Multipart)
        {
          const RawByteString PartCommitTag =
            FORMAT("  <Part><PartNumber>%d</PartNumber><ETag>%s</ETag></Part>\n", Part, Data.ETag.c_str());
          MultipartCommitPutObjectDataCallbackData.Message += PartCommitTag;
        }
      }
      FILE_OPERATION_LOOP_END_EX(FMTLOAD(TRANSFER_ERROR, AHandle.FileName), (folAllowSkip | folRetryOnFatal));

      if (Data.Exception != nullptr)
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

      FILE_OPERATION_LOOP_BEGIN
      {
        RequestInit(MultipartCommitPutObjectDataCallbackData);

        MultipartCommitPutObjectDataCallbackData.Remaining = nb::ToInt32(MultipartCommitPutObjectDataCallbackData.Message.Length());

        S3MultipartCommitHandler MultipartCommitHandler =
          { CreateResponseHandler(), &LibS3MultipartCommitPutObjectDataCallback, nullptr };

        S3_complete_multipart_upload(
          &BucketContext, StrToS3(Key), &MultipartCommitHandler, MultipartUploadId.c_str(),
          MultipartCommitPutObjectDataCallbackData.Remaining,
          FRequestContext, FTimeout, &MultipartCommitPutObjectDataCallbackData);

        CheckLibS3Error(MultipartCommitPutObjectDataCallbackData, true);
      }
      FILE_OPERATION_LOOP_END_EX(FMTLOAD(TRANSFER_ERROR, AHandle.FileName), (folAllowSkip | folRetryOnFatal));

      // to skip abort, in case we ever add any code before the catch, that can throw
      MultipartUploadId = RawByteString();
    }
  }
  catch(Exception &)
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

void TS3FileSystem::CopyToLocal(
  TStrings * FilesToCopy, const UnicodeString & ATargetDir, const TCopyParamType * CopyParam,
  int32_t AParams, TFileOperationProgressType * AOperationProgress, TOnceDoneOperation & OnceDoneOperation)
{
  AParams &= ~cpAppend;

  FTerminal->DoCopyToLocal(FilesToCopy, ATargetDir, CopyParam, AParams, AOperationProgress, tfNone, OnceDoneOperation);
}

struct TLibS3GetObjectDataCallbackData : TLibS3TransferObjectDataCallbackData
{
};

S3Status TS3FileSystem::LibS3GetObjectDataCallback(int32_t BufferSize, const char * Buffer, void * CallbackData)
{
  TLibS3GetObjectDataCallbackData & Data = *static_cast<TLibS3GetObjectDataCallbackData *>(CallbackData);

  return Data.FileSystem->GetObjectData(BufferSize, Buffer, Data);
}

S3Status TS3FileSystem::GetObjectData(int32_t BufferSize, const char * Buffer, TLibS3GetObjectDataCallbackData & Data)
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
      FILE_OPERATION_LOOP_BEGIN
      {
        Data.Stream->Write(Buffer, BufferSize);
      }
      FILE_OPERATION_LOOP_END(FMTLOAD(WRITE_ERROR, Data.FileName));

      OperationProgress->ThrottleToCPSLimit(BufferSize);
      OperationProgress->AddTransferred(BufferSize);
    }
    catch(Exception & E)
    {
      Data.Exception.reset(CloneException(&E));
      Result = S3StatusAbortedByCallback;
    }
  }

  return Result;
}

void TS3FileSystem::Sink(
  const UnicodeString & AFileName, const TRemoteFile * AFile,
  const UnicodeString & ATargetDir, UnicodeString & ADestFileName, int32_t Attrs,
  const TCopyParamType * CopyParam, int32_t Params, TFileOperationProgressType * OperationProgress,
  uint32_t /*AFlags*/, TDownloadSessionAction & Action)
{
  const UnicodeString DestFullName = ATargetDir + ADestFileName;
  if (base::FileExists(ApiPath(DestFullName)))
  {
    int64_t Size;
    int64_t MTime;
    FTerminal->OpenLocalFile(DestFullName, GENERIC_READ, nullptr, nullptr, nullptr, &MTime, nullptr, &Size);
    TOverwriteFileParams FileParams;

    FileParams.SourceSize = AFile->Size();
    FileParams.SourceTimestamp = AFile->Modification(); // noop
    FileParams.DestSize = Size;
    FileParams.DestTimestamp = UnixToDateTime(MTime, FTerminal->GetSessionData()->GetDSTMode());

    ConfirmOverwrite(AFileName, ADestFileName, OperationProgress, &FileParams, CopyParam, Params);
  }

  UnicodeString BucketName, Key;
  ParsePath(AFileName, BucketName, Key);

  const TLibS3BucketContext BucketContext = GetBucketContext(BucketName, Key);

  const UnicodeString ExpandedDestFullName = ExpandUNCFileName(DestFullName);
  Action.Destination(ExpandedDestFullName);

  FILE_OPERATION_LOOP_BEGIN
  {
    HANDLE LocalFileHandle;
    if (!FTerminal->CreateLocalFile(DestFullName, OperationProgress, &LocalFileHandle, FLAGSET(Params, cpNoConfirmation)))
    {
      throw ESkipFile();
    }

    std::unique_ptr<TStream> Stream(std::make_unique<TSafeHandleStream>(reinterpret_cast<THandle>(LocalFileHandle)));

    bool DeleteLocalFile = true;

    try__finally
    {
      TLibS3GetObjectDataCallbackData Data{};

      FILE_OPERATION_LOOP_BEGIN
      {
        RequestInit(Data);
        Data.FileName = AFileName;
        Data.Stream = Stream.get();
        Data.OperationProgress = OperationProgress;
        Data.Exception.reset(nullptr);

        const TAutoFlag ResponseIgnoreSwitch(FResponseIgnore);
        const S3GetObjectHandler GetObjectHandler = { CreateResponseHandler(), LibS3GetObjectDataCallback };
        S3_get_object(
          &BucketContext, StrToS3(Key), nullptr, nb::ToUInt64(Stream->Position()), 0, FRequestContext, FTimeout, &GetObjectHandler, &Data);

        // The "exception" was already seen by the user, its presence mean an accepted abort of the operation.
        if (Data.Exception == nullptr)
        {
          CheckLibS3Error(Data, true);
        }
      }
      FILE_OPERATION_LOOP_END_EX(FMTLOAD(TRANSFER_ERROR, AFileName), (folAllowSkip | folRetryOnFatal));

      if (Data.Exception != nullptr)
      {
        RethrowException(Data.Exception.get());
      }

      DeleteLocalFile = false;

      if (CopyParam->PreserveTime())
      {
        FTerminal->UpdateTargetTime(
          LocalFileHandle, AFile->Modification(), AFile->ModificationFmt, FTerminal->SessionData->GetDSTMode());
      }
    }
    __finally
    {
      SAFE_CLOSE_HANDLE(LocalFileHandle);

      if (DeleteLocalFile)
      {
        FTerminal->DoDeleteLocalFile(DestFullName);
      }
    } end_try__finally
  }
  FILE_OPERATION_LOOP_END(FMTLOAD(TRANSFER_ERROR, AFileName));

  FTerminal->UpdateTargetAttrs(DestFullName, AFile, CopyParam, Attrs);
}

void TS3FileSystem::GetSupportedChecksumAlgs(TStrings * /*Algs*/)
{
  // NOOP
}

void TS3FileSystem::LockFile(const UnicodeString & /*AFileName*/, const TRemoteFile * /*AFile*/)
{
  DebugFail();
}

void TS3FileSystem::UnlockFile(const UnicodeString & /*AFileName*/, const TRemoteFile * /*AFile*/)
{
  DebugFail();
}

void TS3FileSystem::UpdateFromMain(TCustomFileSystem * /*AMainFileSystem*/)
{
  // noop
}

void TS3FileSystem::ClearCaches()
{
  FRegions.clear();
  FHostNames.clear();
}

UnicodeString TS3FileSystem::AbsolutePath(const UnicodeString & APath, bool Local)
{
  return std::as_const(*this).AbsolutePath(APath, Local);
}

void TS3FileSystem::InitSslSession(ssl_st * Ssl, ne_session * Session)
{
  TS3FileSystem * FileSystem =
    static_cast<TS3FileSystem *>(ne_get_session_private(Session, SESSION_FS_KEY));
  DebugAssert(FileSystem);
  FileSystem->InitSslSessionImpl(Ssl, Session);
}
