
#include <vcl.h>
#pragma hdrstop

#include <rdestl/vector.h>
#include <winhttp.h>

#include <Common.h>
#include <Exceptions.h>
#include <FileBuffer.h>
#include <StrUtils.hpp>
#include <LibraryLoader.hpp>
#include <nbutils.h>
#include <System.ShlObj.hpp>

#include "SessionData.h"
#include "CoreMain.h"
#include "TextsCore.h"
#include "PuttyIntf.h"
#include "RemoteFiles.h"
#include "SftpFileSystem.h"
#include "S3FileSystem.h"
#include "FileMasks.h"

__removed #pragma package(smart_init)

#define SET_SESSION_PROPERTY_FROM(PROPERTY, FROM) \
  if (F##PROPERTY != FROM) { F##PROPERTY = FROM; Modify(); }

#define SET_SESSION_PROPERTY(PROPERTY) \
  SET_SESSION_PROPERTY_FROM(PROPERTY, value)

const wchar_t * PingTypeNames = L"Off;Null;Dummy";
const wchar_t * ProxyMethodNames = L"None;SOCKS4;SOCKS5;HTTP;Telnet;Cmd";
TIntMapping ProxyMethodMapping = CreateIntMappingFromEnumNames(LowerCase(ProxyMethodNames));
const wchar_t * DefaultName = L"Default Settings";
const UnicodeString CipherNames[CIPHER_COUNT] = {L"WARN", L"3des", L"blowfish", L"aes", L"des", L"arcfour", L"chacha20", "aesgcm"};
const UnicodeString KexNames[KEX_COUNT] = {L"WARN", L"dh-group1-sha1", L"dh-group14-sha1", L"dh-group15-sha512", L"dh-group16-sha512", L"dh-group17-sha512", L"dh-group18-sha512", L"dh-gex-sha1", L"rsa", L"ecdh", L"ntru-curve25519"};
const UnicodeString HostKeyNames[HOSTKEY_COUNT] = {L"WARN", L"rsa", L"dsa", L"ecdsa", L"ed25519", L"ed448"};
const UnicodeString GssLibNames[GSSLIB_COUNT] = {L"gssapi32", L"sspi", L"custom"};
// Update also order in SshCipherList()
const TCipher DefaultCipherList[CIPHER_COUNT] =
  { cipAES, cipChaCha20, cipAESGCM, cip3DES, cipWarn, cipDES, cipBlowfish, cipArcfour };
// Update also order in SshKexList()
const TKex DefaultKexList[KEX_COUNT] =
  { kexNTRUHybrid, kexECDH, kexDHGEx, kexDHGroup18, kexDHGroup17, kexDHGroup16, kexDHGroup15, kexDHGroup14, kexRSA, kexWarn, kexDHGroup1 };
const THostKey DefaultHostKeyList[HOSTKEY_COUNT] =
  { hkED448, hkED25519, hkECDSA, hkRSA, hkDSA, hkWarn };
const TGssLib DefaultGssLibList[GSSLIB_COUNT] =
  { gssGssApi32, gssSspi, gssCustom };
const wchar_t FSProtocolNames[FSPROTOCOL_COUNT][16] = { L"SCP", L"SFTP (SCP)", L"SFTP", L"", L"", L"FTP", L"WebDAV", L"S3" };
const int32_t SshPortNumber = 22;
const int32_t FtpPortNumber = 21;
const int32_t FtpsImplicitPortNumber = 990;
const int32_t HTTPPortNumber = 80;
const int32_t HTTPSPortNumber = 443;
const int32_t TelnetPortNumber = 23;
const int32_t DefaultSendBuf = 256 * 1024;
const int32_t ProxyPortNumber = 80;
const UnicodeString AnonymousUserName("anonymous");
const UnicodeString AnonymousPassword("anonymous@example.com");
const UnicodeString PuttySshProtocol("ssh");
const UnicodeString PuttyTelnetProtocol("telnet");
const UnicodeString SftpProtocol("sftp");
const UnicodeString ScpProtocol("scp");
const UnicodeString FtpProtocol("ftp");
const UnicodeString FtpsProtocol("ftps");
const UnicodeString FtpesProtocol("ftpes");
const UnicodeString WebDAVProtocol("dav");
const UnicodeString WebDAVSProtocol("davs");
const UnicodeString S3Protocol("s3");
const UnicodeString S3PlainProtocol(L"s3plain");
const UnicodeString SshProtocol("ssh");
const UnicodeString WinSCPProtocolPrefix("winscp-");
const wchar_t UrlParamSeparator = L';';
const wchar_t UrlParamValueSeparator = L'=';
const UnicodeString UrlHostKeyParamName("fingerprint");
const UnicodeString UrlSaveParamName("save");
const UnicodeString UrlRawSettingsParamNamePrefix("x-");
const UnicodeString PassphraseOption("passphrase");
const UnicodeString RawSettingsOption("rawsettings");
const UnicodeString S3HostName(S3LibDefaultHostName());
const UnicodeString OpensshHostDirective(L"Host");
const uint32_t CONST_DEFAULT_CODEPAGE = CP_UTF8;
const TFSProtocol CONST_DEFAULT_PROTOCOL = fsSFTP;

static TDateTime SecToDateTime(int32_t Sec)
{
  return TDateTime(double(Sec) / SecsPerDay);
}

static bool IsValidOpensshLine(const UnicodeString & Line)
{
  return !Line.IsEmpty() && (Line[1] != L'#');
}

static bool ParseOpensshDirective(const UnicodeString & ALine, UnicodeString & Directive, UnicodeString & Value)
{
  bool Result = IsValidOpensshLine(ALine);
  if (Result)
  {
    UnicodeString Line = Trim(ALine);
    if (Line.SubString(1, 1) == "\"")
    {
      Line.Delete(1, 1);
      int32_t P = Line.Pos("\"");
      Result = (P > 0);
      if (Result)
      {
        Directive = Line.SubString(1, P - 1);
        Line = MidStr(Line, P + 1).Trim();
      }
    }
    else
    {
      wchar_t Equal = L'=';
      UnicodeString Whitespaces(L" \t");
      UnicodeString Delimiters(Whitespaces + Equal);
      int32_t P = FindDelimiter(Delimiters, Line);
      Result = (P > 0);
      if (Result)
      {
        Directive = Line.SubString(1, P - 1);
        Line.Delete(1, P - 1);
        UnicodeString TrimChars = Delimiters;
        while (!Line.IsEmpty() && Line.IsDelimiter(TrimChars, 1))
        {
          if (Line[1] == Equal)
          {
            TrimChars = Whitespaces;
          }
          Line.Delete(1, 1);
        }
      }
    }

    Value = Line;
    Result = !Value.IsEmpty();
  }
  return Result;
}
//--- TSessionData ----------------------------------------------------
TSessionData::TSessionData(const UnicodeString & AName) noexcept :
  TNamedObject(OBJECT_CLASS_TSessionData, AName)
{
  Default();
  FModified = true;
}

TSessionData::~TSessionData() noexcept
{
  if (nullptr != FIEProxyConfig)
  {
    SAFE_DESTROY(FIEProxyConfig);
  }
}

int32_t TSessionData::Compare(const TNamedObject * Other) const
{
  int32_t Result;
  // To avoid using CompareLogicalText on hex names of sessions in workspace.
  // The session 000A would be sorted before 0001.
#if 0
  if (IsWorkspace && DebugNotNull(dynamic_cast<TSessionData *>(Other))->IsWorkspace)
  {
    Result = CompareText(Name, Other->GetName());
  }
  else
#endif // #if 0
  {
    Result = TNamedObject::Compare(Other);
  }
  return Result;
}

TSessionData * TSessionData::Clone() const
{
  std::unique_ptr<TSessionData> Data(std::make_unique<TSessionData>(""));
  Data->Assign(this);
  return Data.release();
}

void TSessionData::DefaultSettings()
{
  SetHostName("");
  SetPortNumber(SshPortNumber);
  SessionSetUserName("");
  SetPassword("");
  SetNewPassword("");
  SetChangePassword(false);
  SetPingInterval(30);
  SetPingType(ptOff);
  SetTimeout(15);
  SetTryAgent(true);
  SetAgentFwd(false);
  FAuthKI = true;
  SetAuthKI(true);
  SetAuthKIPassword(true);
  SetAuthGSSAPI(false);
  FAuthGSSAPIKEX = false;
  SetGSSAPIFwdTGT(false);
  SetLogicalHostName("");
  SetChangeUsername(false);
  SetCompression(false);
  SetSsh2DES(false);
  SetSshNoUserAuth(false);
  for (int32_t Index = 0; Index < CIPHER_COUNT; ++Index)
  {
    SetCipher(Index, DefaultCipherList[Index]);
  }
  for (int32_t Index = 0; Index < KEX_COUNT; ++Index)
  {
    SetKex(Index, DefaultKexList[Index]);
  }
  for (int32_t Index = 0; Index < HOSTKEY_COUNT; Index++)
  {
    SetHostKeys(Index, DefaultHostKeyList[Index]);
  }
  for (int32_t Index = 0; Index < GSSLIB_COUNT; ++Index)
  {
    SetGssLib(Index, DefaultGssLibList[Index]);
  }
  SetGssLibCustom("");
  PublicKeyFile = EmptyStr;
  DetachedCertificate = EmptyStr;
  SetPassphrase("");
  SetPuttyProtocol("");
  SetTcpNoDelay(true);
  SetSendBuf(DefaultSendBuf);
  FSourceAddress = L"";
  FProtocolFeatures = L"";
  SetSshSimple(true);
  FNotUtf = asAuto;
  FIsWorkspace = false;
  SetHostKey("");
  SetFingerprintScan(false);
  FOverrideCachedHostKey = true;
  SetNote("");
  SetWinTitle("");
  FOrigHostName.Clear();
  SetInternalEditorEncoding(-1);
  FOrigPortNumber = 0;
  FOrigProxyMethod = pmNone;
  FTunnelConfigured = false;

  EncryptKey = UnicodeString();

  WebDavLiberalEscaping = false;
  WebDavAuthLegacy = false;

  FProxyMethod = ::pmNone;
  FProxyHost = L"proxy";
  SetProxyPort(ProxyPortNumber);
  SetProxyUsername("");
  SetProxyPassword("");
  SetProxyTelnetCommand("connect %host %port\\n");
  SetProxyLocalCommand("");
  SetProxyDNS(asAuto);
  SetProxyLocalhost(false);

  for (int32_t Index = 0; Index < nb::ToIntPtr(_countof(FBugs)); ++Index)
  {
    SetBug(static_cast<TSshBug>(Index), asAuto);
  }

  SetSpecial(false);
  SetFSProtocol(CONST_DEFAULT_PROTOCOL);
  SetAddressFamily(afAuto);
  SetRekeyData("1G");
  SetRekeyTime(MinsPerHour);

  // FS common
  SetLocalDirectory("");
  OtherLocalDirectory = L"";
  SetRemoteDirectory("");
  SetSynchronizeBrowsing(false);
  SetUpdateDirectories(true);
  FRequireDirectories = false;
  SetCacheDirectories(false);
  SetCacheDirectoryChanges(false);
  SetPreserveDirectoryChanges(false);
  SetResolveSymlinks(true);
  SetFollowDirectorySymlinks(true);
  SetDSTMode(dstmUnix);
  SetDeleteToRecycleBin(false);
  SetOverwrittenToRecycleBin(false);
  SetRecycleBinPath("");
  SetColor(0);
  SetPostLoginCommands("");

  // SCP
  SetLookupUserGroups(asOn);
  SetEOLType(eolLF);
  SetTrimVMSVersions(false);
  VMSAllRevisions = false;
  SetShell(""); //default shell
  SetReturnVar("");
  SetExitCode1IsError(false);
  FClearAliases = true;
  SetUnsetNationalVars(true);
  SetListingCommand("ls -la");
  SetIgnoreLsWarnings(true);
  SetScp1Compatibility(false);
  SetTimeDifference(TDateTime(0.0));
  SetTimeDifferenceAuto(true);
  SetSCPLsFullTime(asAuto);
  SetNotUtf(asOn); // asAuto

  // S3
  S3DefaultRegion = EmptyStr;
  S3SessionToken = EmptyStr;
  S3Profile = EmptyStr;
  FS3UrlStyle = s3usVirtualHost;
  FS3MaxKeys = asAuto;
  FS3CredentialsEnv = false;

  // SFTP
  SetSftpServer("");
  SetSFTPDownloadQueue(32);
  SetSFTPUploadQueue(32);
  SetSFTPListingQueue(2);
  SetSFTPMaxVersion(::SFTPMaxVersion);
  SetSFTPMaxPacketSize(0);
  SetSFTPMinPacketSize(0);
  FSFTPRealPath = asAuto;

  for (int32_t Index = 0; Index < nb::ToIntPtr(_countof(FSFTPBugs)); ++Index)
  {
    SetSFTPBug(static_cast<TSftpBug>(Index), asAuto);
  }

  SetTunnel(false);
  SetTunnelHostName("");
  SetTunnelPortNumber(SshPortNumber);
  SetTunnelUserName("");
  SetTunnelPassword("");
  SetTunnelPublicKeyFile("");
  FTunnelPassphrase = L"";
  SetTunnelLocalPortNumber(0);
  SetTunnelPortFwd("");
  SetTunnelHostKey("");

  // FTP
  SetFtpPasvMode(true);
  SetFtpForcePasvIp(asAuto);
  SetFtpUseMlsd(asAuto);
  SetFtpAccount("");
  SetFtpPingInterval(30);
  SetFtpPingType(ptDummyCommand);
  SetFtpTransferActiveImmediately(asAuto);
  SetFtps(ftpsNone);
  MinTlsVersion = tlsDefaultMin;
  MaxTlsVersion = tlsMax;
  SetFtpListAll(asAuto);
  SetFtpHost(asAuto);
  SetFtpDupFF(false);
  SetFtpUndupFF(false);
  FFtpWorkFromCwd = asAuto;
  FFtpAnyCodeForPwd = false;
  SetSslSessionReuse(true);
  SetTlsCertificateFile("");

  SetFtpProxyLogonType(0); // none

  FPuttySettings = UnicodeString();

  SetCustomParam1("");
  SetCustomParam2("");

#if 0
  IsWorkspace = false;
  Link = L"";
  NameOverride = L"";
#endif // #if 0

  SetSelected(false);
  FModified = false;
  FSource = ::ssNone;
  FSaveOnly = false;

  SetCodePage(::GetCodePageAsString(CONST_DEFAULT_CODEPAGE));
  SetLoginType(ltAnonymous);
  SetFtpAllowEmptyPassword(false);

  FNumberOfRetries = 0;
  FSessionVersion = ::StrToVersionNumber(GetGlobals()->GetStrVersionNumber());
  // add also to TSessionLog::AddStartupInfo()
}

void TSessionData::Default()
{
  DefaultSettings();

  FIsWorkspace = false;
  FLink = L"";
  FNameOverride = L"";

  FSelected = false;
  FModified = false;
  FSource = ::ssNone;
  FSaveOnly = false;

  // add also to TSessionLog::AddStartupInfo()
}

void TSessionData::NonPersistent()
{
  SetUpdateDirectories(false);
  SetPreserveDirectoryChanges(false);
}

  //PROPERTY(UserName); \

#define PROPERTY(P) PROPERTY_HANDLER(P, )
#define BASE_PROPERTIES \
  PROPERTY(HostName); \
  PROPERTY(PortNumber); \
  PROPERTY_HANDLER(Password, F); \
  PROPERTY(PublicKeyFile); \
  PROPERTY2(DetachedCertificate); \
  PROPERTY_HANDLER(Passphrase, F); \
  PROPERTY(FSProtocol); \
  PROPERTY(Ftps); \
  PROPERTY(LocalDirectory); \
  PROPERTY2(OtherLocalDirectory); \
  PROPERTY(RemoteDirectory); \
  PROPERTY2(RequireDirectories);
#if 0
 PROPERTY(UserName); \
  PROPERTY(Color); \
  PROPERTY(SynchronizeBrowsing); \
  PROPERTY(Note);
#endif // #if 0

#define ADVANCED_PROPERTIES \
  PROPERTY_HANDLER(NewPassword, F); \
  PROPERTY(ChangePassword); \
  PROPERTY(PingInterval); \
  PROPERTY(PingType); \
  PROPERTY(Timeout); \
  PROPERTY(TryAgent); \
  PROPERTY(AgentFwd); \
  PROPERTY(LogicalHostName); \
  PROPERTY(ChangeUsername); \
  PROPERTY(Compression); \
  PROPERTY(Ssh2DES); \
  PROPERTY(SshNoUserAuth); \
  PROPERTY(CipherList); \
  PROPERTY(KexList); \
  PROPERTY(HostKeyList); \
  PROPERTY(GssLibList); \
  PROPERTY(GssLibCustom); \
  PROPERTY(AddressFamily); \
  PROPERTY(RekeyData); \
  PROPERTY(RekeyTime); \
  PROPERTY(HostKey); \
  PROPERTY(FingerprintScan); \
  PROPERTY(InternalEditorEncoding); \
  \
  PROPERTY(UpdateDirectories); \
  PROPERTY(CacheDirectories); \
  PROPERTY(CacheDirectoryChanges); \
  PROPERTY(PreserveDirectoryChanges); \
  \
  PROPERTY(ResolveSymlinks); \
  PROPERTY(FollowDirectorySymlinks); \
  PROPERTY(DSTMode); \
  PROPERTY(Special); \
  PROPERTY(Selected); \
  PROPERTY(ReturnVar); \
  PROPERTY(ExitCode1IsError); \
  PROPERTY(LookupUserGroups); \
  PROPERTY(EOLType); \
  PROPERTY(TrimVMSVersions); \
  PROPERTY2(VMSAllRevisions); \
  PROPERTY(Shell); \
  PROPERTY(ClearAliases); \
  PROPERTY(Scp1Compatibility); \
  PROPERTY(UnsetNationalVars); \
  PROPERTY(ListingCommand); \
  PROPERTY(IgnoreLsWarnings); \
  PROPERTY(SCPLsFullTime); \
  \
  PROPERTY(TimeDifference); \
  PROPERTY(TimeDifferenceAuto); \
  PROPERTY(TcpNoDelay); \
  PROPERTY(SendBuf); \
  PROPERTY2(SourceAddress); \
  PROPERTY2(ProtocolFeatures); \
  PROPERTY(SshSimple); \
  PROPERTY(AuthKI); \
  PROPERTY(AuthKIPassword); \
  PROPERTY(AuthGSSAPI); \
  PROPERTY2(AuthGSSAPIKEX); \
  PROPERTY(GSSAPIFwdTGT); \
  PROPERTY(DeleteToRecycleBin); \
  PROPERTY(OverwrittenToRecycleBin); \
  PROPERTY(RecycleBinPath); \
  PROPERTY(NotUtf); \
  PROPERTY(PostLoginCommands); \
  \
  PROPERTY2(S3DefaultRegion); \
  PROPERTY2(S3SessionToken); \
  PROPERTY2(S3Profile); \
  PROPERTY2(S3UrlStyle); \
  PROPERTY2(S3MaxKeys); \
  PROPERTY2(S3CredentialsEnv); \
  \
  PROPERTY(ProxyMethod); \
  PROPERTY(ProxyHost); \
  PROPERTY(ProxyPort); \
  PROPERTY(ProxyUsername); \
  PROPERTY_HANDLER(ProxyPassword, F); \
  PROPERTY(ProxyTelnetCommand); \
  PROPERTY(ProxyLocalCommand); \
  PROPERTY(ProxyDNS); \
  PROPERTY(ProxyLocalhost); \
  \
  PROPERTY(SftpServer); \
  PROPERTY(SFTPDownloadQueue); \
  PROPERTY(SFTPUploadQueue); \
  PROPERTY(SFTPListingQueue); \
  PROPERTY(SFTPMaxVersion); \
  PROPERTY(SFTPMaxPacketSize); \
  PROPERTY2(SFTPRealPath); \
  \
  PROPERTY(Tunnel); \
  PROPERTY(TunnelHostName); \
  PROPERTY(TunnelPortNumber); \
  PROPERTY(TunnelUserName); \
  PROPERTY_HANDLER(TunnelPassword, F); \
  PROPERTY(TunnelPublicKeyFile); \
  PROPERTY_HANDLER(TunnelPassphrase, F); \
  PROPERTY(TunnelLocalPortNumber); \
  PROPERTY(TunnelPortFwd); \
  PROPERTY(TunnelHostKey); \
  \
  PROPERTY(FtpPasvMode); \
  PROPERTY(FtpForcePasvIp); \
  PROPERTY(FtpUseMlsd); \
  PROPERTY(FtpAccount); \
  PROPERTY(FtpPingInterval); \
  PROPERTY(FtpPingType); \
  PROPERTY(FtpTransferActiveImmediately); \
  PROPERTY(FtpListAll); \
  PROPERTY(FtpHost); \
  PROPERTY2(FtpWorkFromCwd); \
  PROPERTY2(FtpAnyCodeForPwd); \
  PROPERTY(SslSessionReuse); \
  PROPERTY(TlsCertificateFile); \
  \
  PROPERTY(FtpProxyLogonType); \
  \
  PROPERTY(MinTlsVersion); \
  PROPERTY(MaxTlsVersion); \
  \
  PROPERTY(WinTitle); \
  \
  PROPERTY_HANDLER(EncryptKey, F); \
  \
  PROPERTY2(WebDavLiberalEscaping); \
  PROPERTY2(WebDavAuthLegacy); \
  \
  PROPERTY2(PuttySettings); \
  \
  PROPERTY(CustomParam1); \
  PROPERTY(CustomParam2);
#define META_PROPERTIES \
  PROPERTY(IsWorkspace); \
  PROPERTY(Link); \
  PROPERTY(NameOverride);

void TSessionData::Assign(const TPersistent * Source)
{
  if (Source && isa<TSessionData>(Source))
  {
    TSessionData * SourceData = dyn_cast<TSessionData>(const_cast<TPersistent *>(Source));
    // Master password prompt shows implicitly here, when cloning the session data for a new terminal
    CopyData(SourceData);
    FSource = SourceData->FSource;
  }
  else
  {
    TNamedObject::Assign(Source);
  }
}

void TSessionData::DoCopyData(const TSessionData * SourceData, bool NoRecrypt)
{
/*
  #define PROPERTY_HANDLER(P, F) \
    if (NoRecrypt) \
    { \
      F##P = SourceData->F##P; \
    } \
    else \
    { \
      P = SourceData->P; \
    }
*/
  #undef PROPERTY
  #undef PROPERTY2
  #undef PROPERTY_HANDLER
  #define PROPERTY(P) Set ## P(SourceData->Get ## P())
  #define PROPERTY2(P) F##P = SourceData->F##P;
  #define PROPERTY_HANDLER(P, F) F##P = SourceData->F##P;

  PROPERTY(Name);
  BASE_PROPERTIES;
  ADVANCED_PROPERTIES;
  META_PROPERTIES;
  #undef PROPERTY_HANDLER
  FOverrideCachedHostKey = SourceData->FOverrideCachedHostKey;
  FModified = SourceData->FModified;
  FSaveOnly = SourceData->FSaveOnly;

  SessionSetUserName(SourceData->SessionGetUserName());
  for (int32_t Index = 0; Index < nb::ToIntPtr(_countof(FBugs)); ++Index)
  {
    // PROPERTY(Bug[(TSshBug)Index]);
    SetBug(static_cast<TSshBug>(Index),
      SourceData->GetBug(static_cast<TSshBug>(Index)));
  }
  for (int32_t Index = 0; Index < nb::ToIntPtr(_countof(FSFTPBugs)); ++Index)
  {
    // PROPERTY(SFTPBug[(TSftpBug)Index]);
    SetSFTPBug(static_cast<TSftpBug>(Index),
      SourceData->GetSFTPBug(static_cast<TSftpBug>(Index)));
  }
  // Restore default kex list
  for (int32_t Index = 0; Index < KEX_COUNT; ++Index)
  {
    SetKex(Index, DefaultKexList[Index]);
  }

  FOverrideCachedHostKey = SourceData->GetOverrideCachedHostKey();
  FModified = SourceData->GetModified();
  FSaveOnly = SourceData->GetSaveOnly();

  FSource = SourceData->FSource;
  FNumberOfRetries = SourceData->FNumberOfRetries;
}

void TSessionData::CopyData(const TSessionData * SourceData)
{
  DoCopyData(SourceData, false);
}

void TSessionData::CopyDataNoRecrypt(const TSessionData * SourceData)
{
  DoCopyData(SourceData, true);
}

void TSessionData::CopyDirectoriesStateData(TSessionData * SourceData)
{
  SetRemoteDirectory(SourceData->GetRemoteDirectory());
  SetLocalDirectory(SourceData->GetLocalDirectory());
  OtherLocalDirectory = SourceData->OtherLocalDirectory;
  SetSynchronizeBrowsing(SourceData->GetSynchronizeBrowsing());
}

bool TSessionData::HasStateData() const
{
  return
    !RemoteDirectory().IsEmpty() ||
    !LocalDirectory().IsEmpty() ||
    !OtherLocalDirectory().IsEmpty() ||
    (GetColor() != 0);
}

void TSessionData::CopyStateData(TSessionData * SourceData)
{
  // Keep in sync with TCustomScpExplorerForm::UpdateSessionData.
  CopyDirectoriesStateData(SourceData);
  SetColor(SourceData->GetColor());
}

void TSessionData::CopyNonCoreData(TSessionData * SourceData)
{
  CopyStateData(SourceData);
  SetUpdateDirectories(SourceData->GetUpdateDirectories());
  SetNote(SourceData->GetNote());
}

bool TSessionData::IsSame(
  const TSessionData * Default, bool AdvancedOnly, TStrings * DifferentProperties, bool Decrypted) const
{
  bool Result = true;
  #undef PROPERTY
  #undef PROPERTY2
  #undef PROPERTY_HANDLER
  #define PROPERTY(P) if (Get ## P() != Default->Get ## P()) { if (DifferentProperties != nullptr) { DifferentProperties->Add(# P); Result = false; } }
  #define PROPERTY2(P) if (F##P != Default->F##P) { if (DifferentProperties != nullptr) { DifferentProperties->Add(# P); Result = false; } }
  #define PROPERTY_HANDLER(P, F) \
    if (Get ## P() != Default->Get ## P()) \
    { \
      if (DifferentProperties != nullptr) \
      { \
        DifferentProperties->Add(# P); \
      } \
      Result = false; \
    }

#if 0
  #define PROPERTY_HANDLER(P, F) \
    if ((Decrypted && (P != Default->P)) || \
        (!Decrypted && (F##P != Default->F##P))) \
    { \
      Result = false; \
      if (DifferentProperties != nullptr) \
      { \
        DifferentProperties->Add(#P); \
      } \
      else \
      { \
        return Result; \
      } \
      Result = false; \
    }
#endif // #if 0

  if (!AdvancedOnly)
  {
    BASE_PROPERTIES;
    // META_PROPERTIES;
  }
  ADVANCED_PROPERTIES;
  #undef PROPERTY_HANDLER
  #undef PROPERTY

  for (int32_t Index = 0; Index < nb::ToIntPtr(_countof(FBugs)); ++Index)
  {
    // PROPERTY(Bug[(TSshBug)Index]);
    if (GetBug(static_cast<TSshBug>(Index)) != Default->GetBug(static_cast<TSshBug>(Index)))
      return false;
  }
  for (int32_t Index = 0; Index < nb::ToIntPtr(_countof(FSFTPBugs)); ++Index)
  {
    // PROPERTY(SFTPBug[(TSftpBug)Index]);
    if (GetSFTPBug(static_cast<TSftpBug>(Index)) != Default->GetSFTPBug(static_cast<TSftpBug>(Index)))
      return false;
  }

  return Result;
}

bool TSessionData::IsSame(const TSessionData * Default, bool AdvancedOnly) const
{
  return IsSame(Default, AdvancedOnly, nullptr, false);
}

bool TSessionData::IsSameDecrypted(const TSessionData * Default) const
{
  return IsSame(Default, false, nullptr, true);
}

TFSProtocol NormalizeFSProtocol(TFSProtocol FSProtocol)
{
  if (FSProtocol == fsSFTPonly)
  {
    FSProtocol = fsSFTP;
  }
  return FSProtocol;
}

bool TSessionData::IsSameSite(const TSessionData * Other) const
{
  return
    // Particularly when handling /refresh,
    // fsSFTPonly sites when compared against sftp:// URLs (fsSFTP) have to match.
    // But similarly also falled back SCP sites.
    (NormalizeFSProtocol(GetFSProtocol()) == NormalizeFSProtocol(Other->GetFSProtocol())) &&
    (GetHostName() == Other->GetHostName()) &&
    (GetPortNumber() == Other->GetPortNumber()) &&
    (SessionGetUserName() == Other->SessionGetUserName());
}

bool TSessionData::IsInFolderOrWorkspace(const UnicodeString & AFolder) const
{
  return ::StartsText(base::UnixIncludeTrailingBackslash(AFolder), GetName());
}

void TSessionData::DoLoad(THierarchicalStorage * Storage, bool PuttyImport, bool & RewritePassword, bool Unsafe, bool RespectDisablePasswordStoring)
{
  SetSessionVersion(::StrToVersionNumber(Storage->ReadString("Version", "")));
  // Make sure we only ever use methods supported by TOptionsStorage
  // (implemented by TOptionsIniFile)

  SetPortNumber(Storage->ReadInteger("PortNumber", nb::ToInt32(GetPortNumber())));
  SessionSetUserName(Storage->ReadString("UserName", SessionGetUserName()));
  // must be loaded after UserName, because HostName may be in format user@host
  SetHostName(Storage->ReadString("HostName", GetHostName()));

  #define LOAD_PASSWORD_EX(PROP, PLAIN_NAME, ENC_NAME, ONPLAIN) \
    if (Storage->ValueExists(PLAIN_NAME)) \
    { \
      Set ## PROP(Storage->ReadString(PLAIN_NAME, F ## PROP)); \
      ONPLAIN \
    } \
    else \
    { \
      RawByteString A##PROP = Storage->ReadStringAsBinaryData(ENC_NAME, F##PROP); \
      SET_SESSION_PROPERTY_FROM(PROP, A##PROP); \
    }
  #define LOAD_PASSWORD(PROP, PLAIN_NAME) LOAD_PASSWORD_EX(PROP, PLAIN_NAME, TEXT(#PROP), RewritePassword = true;)
  bool LoadPasswords = !GetConfiguration()->GetDisablePasswordStoring() || !RespectDisablePasswordStoring;
  if (LoadPasswords)
  {
    LOAD_PASSWORD(Password, L"PasswordPlain");
  }
  SetHostKey(Storage->ReadString("SshHostKey", GetHostKey())); // probably never used
  SetNote(Storage->ReadString("Note", GetNote()));
  // Putty uses PingIntervalSecs
  int32_t PingIntervalSecs = Storage->ReadInteger("PingIntervalSecs", -1);
  if (PingIntervalSecs < 0)
  {
    PingIntervalSecs = Storage->ReadInteger("PingIntervalSec", GetPingInterval() % SecsPerMin);
  }
  SetPingInterval(
    Storage->ReadInteger("PingInterval", nb::ToInt32(GetPingInterval()) / SecsPerMin) * SecsPerMin +
    PingIntervalSecs);
  if (GetPingInterval() == 0)
  {
    SetPingInterval(30);
  }
  SetPingType(static_cast<TPingType>(Storage->ReadInteger("PingType", GetPingType())));
  SetTimeout(Storage->ReadInteger("Timeout", nb::ToInt32(GetTimeout())));
  SetTryAgent(Storage->ReadBool("TryAgent", GetTryAgent()));
  SetAgentFwd(Storage->ReadBool("AgentFwd", GetAgentFwd()));
  SetAuthKI(Storage->ReadBool("AuthKI", GetAuthKI()));
  SetAuthKIPassword(Storage->ReadBool("AuthKIPassword", GetAuthKIPassword()));
  // Continue to use setting keys of previous kerberos implementation (vaclav tomec),
  // but fallback to keys of other implementations (official putty and vintela quest putty),
  // to allow imports from all putty versions.
  // Both vaclav tomec and official putty use AuthGSSAPI
  SetAuthGSSAPI(Storage->ReadBool("AuthGSSAPI", Storage->ReadBool("AuthSSPI", GetAuthGSSAPI())));
  FAuthGSSAPIKEX = Storage->ReadBool(L"AuthGSSAPIKEX", FAuthGSSAPIKEX);
  SetGSSAPIFwdTGT(Storage->ReadBool("GSSAPIFwdTGT", Storage->ReadBool("GssapiFwd", Storage->ReadBool("SSPIFwdTGT", GetGSSAPIFwdTGT()))));
  // KerbPrincipal was used by Quest PuTTY
  // GSSAPIServerRealm was used by Vaclav Tomec
  SetLogicalHostName(Storage->ReadString("LogicalHostName", Storage->ReadString("GSSAPIServerRealm", Storage->ReadString("KerbPrincipal", GetLogicalHostName()))));
  SetChangeUsername(Storage->ReadBool("ChangeUsername", GetChangeUsername()));
  SetCompression(Storage->ReadBool("Compression", GetCompression()));
  SetSsh2DES(Storage->ReadBool("Ssh2DES", GetSsh2DES()));
  SetSshNoUserAuth(Storage->ReadBool("SshNoUserAuth", GetSshNoUserAuth()));
  SetCipherList(Storage->ReadString("Cipher", GetCipherList()));
  SetKexList(Storage->ReadString("KEX", GetKexList()));
  SetHostKeyList(Storage->ReadString("HostKey", GetHostKeyList()));
  if (!Unsafe)
  {
    GssLibList = Storage->ReadString(L"GSSLibs", GssLibList);
  }
  SetGssLibList(Storage->ReadString("GSSLibs", GetGssLibList()));
  SetGssLibCustom(Storage->ReadString("GSSCustom", GetGssLibCustom()));
  SetPublicKeyFile(Storage->ReadString("PublicKeyFile", GetPublicKeyFile()));
  DetachedCertificate = Storage->ReadString(L"DetachedCertificate", DetachedCertificate);
  SetAddressFamily(static_cast<TAddressFamily>
    (Storage->ReadInteger("AddressFamily", GetAddressFamily())));
  SetRekeyData(Storage->ReadString("RekeyBytes", GetRekeyData()));
  SetRekeyTime(Storage->ReadInteger("RekeyTime", nb::ToInt32(GetRekeyTime())));

  FSProtocol = (TFSProtocol)Storage->ReadInteger(L"FSProtocol", FSProtocol);
  LocalDirectory = Storage->ReadString(L"LocalDirectory", LocalDirectory);
  OtherLocalDirectory = Storage->ReadString(L"OtherLocalDirectory", OtherLocalDirectory);
  RemoteDirectory = Storage->ReadString(L"RemoteDirectory", RemoteDirectory);
  // SynchronizeBrowsing = Storage->ReadBool(L"SynchronizeBrowsing", SynchronizeBrowsing);
  UpdateDirectories = Storage->ReadBool(L"UpdateDirectories", UpdateDirectories);
  CacheDirectories = Storage->ReadBool(L"CacheDirectories", CacheDirectories);
  CacheDirectoryChanges = Storage->ReadBool(L"CacheDirectoryChanges", CacheDirectoryChanges);
  PreserveDirectoryChanges = Storage->ReadBool(L"PreserveDirectoryChanges", PreserveDirectoryChanges);

  ResolveSymlinks = Storage->ReadBool(L"ResolveSymlinks", ResolveSymlinks);
  FollowDirectorySymlinks = Storage->ReadBool(L"FollowDirectorySymlinks", FollowDirectorySymlinks);
  DSTMode = (TDSTMode)Storage->ReadInteger(L"ConsiderDST", DSTMode);
  Special = Storage->ReadBool(L"Special", Special);
  if (!Unsafe)
  {
    Shell = Storage->ReadString(L"Shell", Shell);
  }
  ClearAliases = Storage->ReadBool(L"ClearAliases", ClearAliases);
  FUnsetNationalVars = Storage->ReadBool(L"UnsetNationalVars", FUnsetNationalVars);
  if (!Unsafe)
  {
    FListingCommand = Storage->ReadString(L"ListingCommand",
      Storage->ReadBool(L"AliasGroupList", false) ? UnicodeString(L"ls -gla") : FListingCommand);
  }
  FIgnoreLsWarnings = Storage->ReadBool(L"IgnoreLsWarnings", FIgnoreLsWarnings);
  FSCPLsFullTime = Storage->ReadEnum(L"SCPLsFullTime", FSCPLsFullTime, AutoSwitchMapping);
  FScp1Compatibility = Storage->ReadBool(L"Scp1Compatibility", FScp1Compatibility);
  FTimeDifference = Storage->ReadFloat(L"TimeDifference", FTimeDifference);
  FTimeDifferenceAuto = Storage->ReadBool(L"TimeDifferenceAuto", (FTimeDifference == TDateTime()));
  if (!Unsafe)
  {
    FDeleteToRecycleBin = Storage->ReadBool(L"DeleteToRecycleBin", FDeleteToRecycleBin);
    FOverwrittenToRecycleBin = Storage->ReadBool(L"OverwrittenToRecycleBin", FOverwrittenToRecycleBin);
    FRecycleBinPath = Storage->ReadString(L"RecycleBinPath", FRecycleBinPath);
    FPostLoginCommands = Storage->ReadString(L"PostLoginCommands", FPostLoginCommands);
    FReturnVar = Storage->ReadString(L"ReturnVar", FReturnVar);
  }

  FExitCode1IsError = Storage->ReadBool(L"ExitCode1IsError", FExitCode1IsError);
  FLookupUserGroups = Storage->ReadEnum(L"LookupUserGroups2", FLookupUserGroups, AutoSwitchMapping);
  FEOLType = (TEOLType)Storage->ReadInteger(L"EOLType", FEOLType);
  FTrimVMSVersions = Storage->ReadBool(L"TrimVMSVersions", FTrimVMSVersions);
  FVMSAllRevisions = Storage->ReadBool(L"VMSAllRevisions", FVMSAllRevisions);
  FNotUtf = Storage->ReadEnum(L"Utf", Storage->ReadEnum(L"SFTPUtfBug", FNotUtf), AutoSwitchReversedMapping);
  FInternalEditorEncoding = Storage->ReadInteger(L"InternalEditorEncoding", FInternalEditorEncoding);

  FS3DefaultRegion = Storage->ReadString(L"S3DefaultRegion", FS3DefaultRegion);
  FS3SessionToken = Storage->ReadString(L"S3SessionToken", FS3SessionToken);
  S3Profile = Storage->ReadString(L"S3Profile", S3Profile);
  FS3UrlStyle = (TS3UrlStyle)Storage->ReadInteger(L"S3UrlStyle", FS3UrlStyle);
  FS3MaxKeys = Storage->ReadEnum(L"S3MaxKeys", FS3MaxKeys, AutoSwitchMapping);
  FS3CredentialsEnv = Storage->ReadBool(L"S3CredentialsEnv", FS3CredentialsEnv);

  // PuTTY defaults to TcpNoDelay, but the psftp/pscp ignores this preference, and always set this to off (what is our default too)
  if (!PuttyImport)
  {
    SetTcpNoDelay(Storage->ReadBool("TcpNoDelay", GetTcpNoDelay()));
  }
  SetSendBuf(Storage->ReadInteger("SendBuf", Storage->ReadInteger("SshSendBuf", nb::ToInt32(GetSendBuf()))));
  FSourceAddress = Storage->ReadString(L"SourceAddress", FSourceAddress);
  FProtocolFeatures = Storage->ReadString(L"ProtocolFeatures", FProtocolFeatures);
  SetSshSimple(Storage->ReadBool("SshSimple", GetSshSimple()));

  SetProxyMethod(Storage->ReadEnum<TProxyMethod>("ProxyMethod", GetProxyMethod(), ProxyMethodMapping));
  FProxyHost = Storage->ReadString(L"ProxyHost", FProxyHost);
  FProxyPort = Storage->ReadInteger(L"ProxyPort", FProxyPort);
  FProxyUsername = Storage->ReadString(L"ProxyUsername", FProxyUsername);
  // proxy password is not rewritten
  LOAD_PASSWORD_EX(ProxyPassword, L"ProxyPassword", L"ProxyPasswordEnc", );
  if (!Unsafe)
  {
    if (FProxyMethod == pmCmd)
    {
      FProxyLocalCommand = Storage->ReadStringRaw(L"ProxyTelnetCommand", FProxyLocalCommand);
    }
    else
    {
      FProxyTelnetCommand = Storage->ReadStringRaw(L"ProxyTelnetCommand", FProxyTelnetCommand);
    }
  }
  SetProxyDNS(static_cast<TAutoSwitch>((Storage->ReadInteger("ProxyDNS", (GetProxyDNS() + 2) % 3) + 1) % 3));
  SetProxyLocalhost(Storage->ReadBool("ProxyLocalhost", GetProxyLocalhost()));

  #define READ_BUG(BUG) \
    SetBug(sb##BUG, TAutoSwitch(2 - Storage->ReadInteger(MB_TEXT("Bug"#BUG), \
      2 - GetBug(sb##BUG))));
  READ_BUG(HMAC2);
  READ_BUG(DeriveKey2);
  READ_BUG(RSAPad2);
  READ_BUG(PKSessID2);
  READ_BUG(Rekey2);
  READ_BUG(MaxPkt2);
  READ_BUG(Ignore2);
  READ_BUG(OldGex2);
  READ_BUG(WinAdj);
  READ_BUG(ChanReq);
  #undef READ_BUG

  if ((GetBug(sbHMAC2) == asAuto) &&
      Storage->ReadBool("BuggyMAC", false))
  {
      SetBug(sbHMAC2, asOn);
  }

  if (!Unsafe)
  {
    SetSftpServer(Storage->ReadString("SftpServer", GetSftpServer()));
  }
#define READ_SFTP_BUG(BUG) \
    SetSFTPBug(sb##BUG, TAutoSwitch(Storage->ReadEnum(MB_TEXT("SFTP" #BUG "Bug"), GetSFTPBug(sb##BUG), AutoSwitchMapping)));
  READ_SFTP_BUG(Symlink);
  READ_SFTP_BUG(SignedTS);
  #undef READ_SFTP_BUG

  SetSFTPMaxVersion(Storage->ReadInteger("SFTPMaxVersion", nb::ToInt32(GetSFTPMaxVersion())));
  SetSFTPMinPacketSize(Storage->ReadInteger("SFTPMinPacketSize", nb::ToInt32(GetSFTPMinPacketSize())));
  SetSFTPMaxPacketSize(Storage->ReadInteger("SFTPMaxPacketSize", nb::ToInt32(GetSFTPMaxPacketSize())));
  SetSFTPDownloadQueue(Storage->ReadInteger("SFTPDownloadQueue", nb::ToInt32(GetSFTPDownloadQueue())));
  SetSFTPUploadQueue(Storage->ReadInteger("SFTPUploadQueue", nb::ToInt32(GetSFTPUploadQueue())));
  SetSFTPListingQueue(Storage->ReadInteger("SFTPListingQueue", nb::ToInt32(GetSFTPListingQueue())));
  FSFTPRealPath = Storage->ReadEnum(L"SFTPRealPath", FSFTPRealPath, AutoSwitchMapping);

  SetColor(Storage->ReadInteger("Color", nb::ToInt32(GetColor())));

  SetPuttyProtocol(Storage->ReadString("Protocol", GetPuttyProtocol()));

  SetTunnel(Storage->ReadBool("Tunnel", GetTunnel()));
  SetTunnelPortNumber(Storage->ReadInteger("TunnelPortNumber", nb::ToInt32(GetTunnelPortNumber())));
  SetTunnelUserName(Storage->ReadString("TunnelUserName", GetTunnelUserName()));
  // must be loaded after TunnelUserName,
  // because TunnelHostName may be in format user@host
  SetTunnelHostName(Storage->ReadString("TunnelHostName", GetTunnelHostName()));
  if (LoadPasswords)
  {
    LOAD_PASSWORD(TunnelPassword, L"TunnelPasswordPlain");
  }
  FTunnelPublicKeyFile = Storage->ReadString(L"TunnelPublicKeyFile", FTunnelPublicKeyFile);
  // Contrary to main session passphrase (which has -passphrase switch in scripting),
  // we are loading tunnel passphrase, as there's no other way to provide it in scripting
  if (LoadPasswords)
  {
    LOAD_PASSWORD(TunnelPassphrase, L"TunnelPassphrasePlain");
  }
  SetTunnelLocalPortNumber(Storage->ReadInteger("TunnelLocalPortNumber", nb::ToInt32(GetTunnelLocalPortNumber())));
  SetTunnelHostKey(Storage->ReadString("TunnelHostKey", GetTunnelHostKey()));

  // Ftp prefix
  SetFtpPasvMode(Storage->ReadBool("FtpPasvMode", GetFtpPasvMode()));
  SetFtpForcePasvIp(static_cast<TAutoSwitch>(Storage->ReadInteger("FtpForcePasvIp2", GetFtpForcePasvIp())));
  SetFtpUseMlsd(static_cast<TAutoSwitch>(Storage->ReadInteger("FtpUseMlsd", GetFtpUseMlsd())));
  FFtpForcePasvIp = Storage->ReadEnum(L"FtpForcePasvIp2", FFtpForcePasvIp, AutoSwitchMapping);
  FFtpUseMlsd = Storage->ReadEnum(L"FtpUseMlsd", FFtpUseMlsd, AutoSwitchMapping);
  SetFtpAccount(Storage->ReadString("FtpAccount", GetFtpAccount()));
  SetFtpPingInterval(Storage->ReadInteger("FtpPingInterval", nb::ToInt32(GetFtpPingInterval())));
  SetFtpPingType(static_cast<TPingType>(Storage->ReadInteger("FtpPingType", GetFtpPingType())));
  SetFtpTransferActiveImmediately(static_cast<TAutoSwitch>(Storage->ReadInteger("FtpTransferActiveImmediately2", GetFtpTransferActiveImmediately())));
  FFtpTransferActiveImmediately = Storage->ReadEnum(L"FtpTransferActiveImmediately2", FFtpTransferActiveImmediately, AutoSwitchMapping);
  SetFtps(static_cast<TFtps>(Storage->ReadInteger("Ftps", GetFtps())));
  FFtpListAll = Storage->ReadEnum(L"FtpListAll", FFtpListAll, AutoSwitchMapping);
  FFtpHost = Storage->ReadEnum(L"FtpHost", FFtpHost, AutoSwitchMapping);
  FFtpWorkFromCwd = Storage->ReadEnum(L"FtpWorkFromCwd", Storage->ReadEnum(L"FtpDeleteFromCwd", FFtpWorkFromCwd), AutoSwitchMapping);
  FFtpAnyCodeForPwd = Storage->ReadBool(L"FtpAnyCodeForPwd", FFtpAnyCodeForPwd);
  SetSslSessionReuse(Storage->ReadBool("SslSessionReuse", GetSslSessionReuse()));
  SetTlsCertificateFile(Storage->ReadString("TlsCertificateFile", GetTlsCertificateFile()));

  SetFtpProxyLogonType(Storage->ReadInteger("FtpProxyLogonType", nb::ToInt32(GetFtpProxyLogonType())));

  SetMinTlsVersion(static_cast<TTlsVersion>(Storage->ReadInteger("MinTlsVersion", GetMinTlsVersion())));
  SetMaxTlsVersion(static_cast<TTlsVersion>(Storage->ReadInteger("MaxTlsVersion", GetMaxTlsVersion())));

  LOAD_PASSWORD(EncryptKey, L"EncryptKeyPlain");

  FWebDavLiberalEscaping = Storage->ReadBool(L"WebDavLiberalEscaping", FWebDavLiberalEscaping);
  WebDavAuthLegacy = Storage->ReadBool(L"WebDavAuthLegacy", WebDavAuthLegacy);

#if 0
  IsWorkspace = Storage->ReadBool(L"IsWorkspace", IsWorkspace);
  Link = Storage->ReadString(L"Link", Link);
  NameOverride = Storage->ReadString(L"NameOverride", NameOverride);
#endif // #if 0

  FPuttySettings = Storage->ReadString(L"PuttySettings", FPuttySettings);

  FCustomParam1 = Storage->ReadString(L"CustomParam1", FCustomParam1);
  FCustomParam2 = Storage->ReadString(L"CustomParam2", FCustomParam2);
  SetCustomParam1(Storage->ReadString("CustomParam1", GetCustomParam1()));
  SetCustomParam2(Storage->ReadString("CustomParam2", GetCustomParam2()));
  SetNameOverride(Storage->ReadString("NameOverride", GetNameOverride()));

  SetCodePage(Storage->ReadString("CodePage", GetCodePage()));
  SetLoginType(static_cast<TLoginType>(Storage->ReadInteger("LoginType", GetLoginType())));
  SetFtpAllowEmptyPassword(Storage->ReadBool("FtpAllowEmptyPassword", GetFtpAllowEmptyPassword()));
  if (GetSessionVersion() < ::GetVersionNumber2110())
  {
    SetFtps(TranslateFtpEncryptionNumber(Storage->ReadInteger("FtpEncryption", -1)));
  }

  #undef LOAD_PASSWORD
}

void TSessionData::Load(THierarchicalStorage * Storage, bool PuttyImport)
{
  bool RewritePassword = false;
  if (Storage->OpenSubKey(GetInternalStorageKey(), False))
  {
    // In case we are re-loading, reset passwords, to avoid pointless
    // re-cryption, while loading username/hostname. And moreover, when
    // the password is wrongly encrypted (using a different master password),
    // this breaks sites reload and consequently an overall operation,
    // such as opening Sites menu
    ClearSessionPasswords();
    SetProxyPassword("");

    DoLoad(Storage, PuttyImport, RewritePassword, false, true);

    Storage->CloseSubKey();
  }

  if (RewritePassword)
  {
    TStorageAccessMode AccessMode = Storage->GetAccessMode();
    Storage->SetAccessMode(smReadWrite);

    try
    {
      if (Storage->OpenSubKey(GetInternalStorageKey(), true))
      {
        #define REWRITE_PASSWORD(PROP, PLAIN_NAME) \
          Storage->DeleteValue(PLAIN_NAME); \
          if (!F ## PROP.IsEmpty()) \
          { \
            Storage->WriteBinaryDataAsString(TEXT(#PROP), F##PROP); \
          }
        REWRITE_PASSWORD(Password, L"PasswordPlain");
        REWRITE_PASSWORD(TunnelPassword, L"TunnelPasswordPlain");
        REWRITE_PASSWORD(EncryptKey, L"EncryptKeyPlain");
        REWRITE_PASSWORD(TunnelPassphrase, L"TunnelPassphrasePlain");
        #undef REWRITE_PASSWORD
        Storage->CloseSubKey();
      }
    }
    catch (...)
    {
      // ignore errors (like read-only INI file)
    }

    Storage->SetAccessMode(AccessMode);
  }

  FNumberOfRetries = 0;
  FModified = false;
  FSource = ssStored;
}

void TSessionData::DoSave(THierarchicalStorage * Storage,
  bool PuttyExport, const TSessionData * Default, bool DoNotEncryptPasswords)
{
  nb::used(Default);
  // Same as in TCopyParamType::Save
#undef WRITE_DATA_EX
#define WRITE_DATA_EX(TYPE, NAME, PROPERTY, CONV) \
    if ((Default != nullptr) && (CONV(Default->PROPERTY) == CONV(PROPERTY))) \
    { \
      Storage->DeleteValue(NAME); \
    } \
    else \
    { \
      Storage->Write ## TYPE(NAME, CONV(PROPERTY)); \
    }
#undef WRITE_DATA_EX2
#define WRITE_DATA_EX2(TYPE, NAME, PROPERTY, CONV) \
    { \
      Storage->Write ## TYPE(NAME, CONV(PROPERTY)); \
    }
#undef WRITE_DATA_CONV
#define WRITE_DATA_CONV(TYPE, NAME, PROPERTY) WRITE_DATA_EX(TYPE, NAME, PROPERTY, WRITE_DATA_CONV_FUNC)
#undef WRITE_DATA
#undef WRITE_DATA2
#define WRITE_DATA(TYPE, PROPERTY) WRITE_DATA_EX(TYPE, MB_TEXT(#PROPERTY), Get ## PROPERTY(), )
#define WRITE_DATA2(TYPE, PROPERTY) WRITE_DATA_EX2(TYPE, MB_TEXT(#PROPERTY), Get ## PROPERTY(), nb::ToInt)
#define WRITE_DATA3(TYPE, PROPERTY) WRITE_DATA_EX2(TYPE, MB_TEXT(#PROPERTY), F ## PROPERTY, nb::ToInt)
#define WRITE_DATA4(TYPE, PROPERTY) WRITE_DATA_EX(TYPE, MB_TEXT(#PROPERTY), F ## PROPERTY, )

  Storage->WriteString("Version", ::VersionNumberToStr(::GetCurrentVersionNumber()));
  WRITE_DATA(String, HostName);
  WRITE_DATA2(Integer, PortNumber);
  if ((PingType == ptOff) && PuttyExport)
  {
    // Deleting would do too
    Storage->WriteInteger(L"PingInterval", 0);
    Storage->WriteInteger(L"PingIntervalSecs", 0);
  }
  else
  {
    WRITE_DATA_EX(Integer, "PingInterval", GetPingInterval() / SecsPerMin, nb::ToInt);
    WRITE_DATA_EX(Integer, "PingIntervalSecs", GetPingInterval() % SecsPerMin, );
  }
  Storage->DeleteValue("PingIntervalSec"); // obsolete
  WRITE_DATA(Integer, PingType);
  WRITE_DATA2(Integer, Timeout);
  WRITE_DATA(Bool, TryAgent);
  WRITE_DATA(Bool, AgentFwd);
  WRITE_DATA(Bool, AuthKI);
  WRITE_DATA(Bool, AuthKIPassword);
  WRITE_DATA_EX(String, L"SshHostKey", HostKey, );
  WRITE_DATA(String, Note);

  WRITE_DATA2(Bool, AuthGSSAPI);
  WRITE_DATA3(Bool, AuthGSSAPIKEX);
  WRITE_DATA2(Bool, GSSAPIFwdTGT);
  Storage->DeleteValue("TryGSSKEX");
  Storage->DeleteValue("UserNameFromEnvironment");
  Storage->DeleteValue("GSSAPIServerChoosesUserName");
  Storage->DeleteValue("GSSAPITrustDNS");
  WRITE_DATA(String, LogicalHostName);
  if (PuttyExport)
  {
    // duplicate kerberos setting with keys of the vintela quest putty
    WRITE_DATA_EX(Bool, "AuthSSPI", GetAuthGSSAPI(), );
    WRITE_DATA_EX(Bool, "SSPIFwdTGT", GetGSSAPIFwdTGT(), );
    WRITE_DATA_EX(String, "KerbPrincipal", GetLogicalHostName(), );
    // duplicate kerberos setting with keys of the official putty
    WRITE_DATA_EX(Bool, "GssapiFwd", GetGSSAPIFwdTGT(), );
  }

  WRITE_DATA(Bool, ChangeUsername);
  WRITE_DATA(Bool, Compression);
  WRITE_DATA(Bool, Ssh2DES);
  WRITE_DATA(Bool, SshNoUserAuth);
  WRITE_DATA_EX(String, "Cipher", GetCipherList(), );
  WRITE_DATA_EX(String, "KEX", GetKexList(), );
  WRITE_DATA_EX(String, "HostKey", GetHostKeyList(), );
  WRITE_DATA_EX(String, "GSSLibs", GetGssLibList(), );
  WRITE_DATA_EX(String, "GSSCustom", GetGssLibCustom(), );
  WRITE_DATA(Integer, AddressFamily);
  WRITE_DATA_EX(String, "RekeyBytes", GetRekeyData(), );
  WRITE_DATA2(Integer, RekeyTime);

  WRITE_DATA(Bool, TcpNoDelay);

  if (PuttyExport)
  {
    WRITE_DATA_EX(StringRaw, "UserName", SessionGetUserName(), );
    // PuTTY is started in its binary directory to allow relative paths when opening PuTTY's own stored session.
    // To allow relative paths in our sessions, we have to expand them for PuTTY.
    WRITE_DATA_EX(StringRaw, "PublicKeyFile", GetPublicKeyFile(), ExpandFileName);
    WRITE_DATA_EX(StringRaw, L"DetachedCertificate", DetachedCertificate, ExpandFileName);
  }
  else
  {
    WRITE_DATA_EX(String, "UserName", SessionGetUserName(), );
    WRITE_DATA(String, PublicKeyFile);
    WRITE_DATA_EX(String, "DetachedCertificate", FDetachedCertificate, );
    WRITE_DATA_EX2(String, "FSProtocol", GetFSProtocolStr(), );
    WRITE_DATA(String, LocalDirectory);
    WRITE_DATA_EX(String, "OtherLocalDirectory", FOtherLocalDirectory, );
    WRITE_DATA(String, RemoteDirectory);
    WRITE_DATA(Bool, SynchronizeBrowsing);
    WRITE_DATA(Bool, UpdateDirectories);
    WRITE_DATA(Bool, CacheDirectories);
    WRITE_DATA(Bool, CacheDirectoryChanges);
    WRITE_DATA(Bool, PreserveDirectoryChanges);

    WRITE_DATA(Bool, ResolveSymlinks);
    WRITE_DATA(Bool, FollowDirectorySymlinks);
    WRITE_DATA_EX(Integer, "ConsiderDST", GetDSTMode(), );
    // Special is never stored (if it would, login dialog must be modified not to
    // duplicate Special parameter when Special session is loaded and then stored
    // under different name)
    // WRITE_DATA(Bool, Special);
    WRITE_DATA(String, Shell);
    WRITE_DATA(Bool, ClearAliases);
    WRITE_DATA(Bool, UnsetNationalVars);
    WRITE_DATA(String, ListingCommand);
    WRITE_DATA(Bool, IgnoreLsWarnings);
    WRITE_DATA(Integer, SCPLsFullTime);
    WRITE_DATA(Bool, Scp1Compatibility);
    // TimeDifferenceAuto is valid for FTP protocol only.
    // For other protocols it's typically true (default value),
    // but ignored so TimeDifference is still taken into account (SCP only actually)
    if (FTimeDifferenceAuto && (GetFSProtocol() == fsFTP))
    {
      // Have to delete it as TimeDifferenceAuto is not saved when enabled,
      // but the default is derived from value of TimeDifference.
      Storage->DeleteValue("TimeDifference");
    }
    else
    {
      WRITE_DATA(Float, TimeDifference);
    }
    WRITE_DATA(Bool, TimeDifferenceAuto);
    WRITE_DATA(Bool, DeleteToRecycleBin);
    WRITE_DATA(Bool, OverwrittenToRecycleBin);
    WRITE_DATA(String, RecycleBinPath);
    WRITE_DATA(String, PostLoginCommands);

    WRITE_DATA(String, ReturnVar);
    WRITE_DATA(Bool, ExitCode1IsError);
    WRITE_DATA_EX(Integer, L"LookupUserGroups2", LookupUserGroups, );
    WRITE_DATA(Integer, EOLType);
    WRITE_DATA(Bool, TrimVMSVersions);
    WRITE_DATA3(Bool, VMSAllRevisions);
    Storage->DeleteValue("SFTPUtfBug");
    WRITE_DATA_EX(Integer, "Utf", GetNotUtf(), );
    WRITE_DATA2(Integer, InternalEditorEncoding);
    WRITE_DATA(String, S3DefaultRegion);
    WRITE_DATA4(String, S3SessionToken);
    WRITE_DATA4(String, S3Profile);
    WRITE_DATA3(Integer, S3UrlStyle);
    WRITE_DATA3(Integer, S3MaxKeys);
    WRITE_DATA3(Bool, S3CredentialsEnv);
    WRITE_DATA(Integer, SendBuf);
    WRITE_DATA4(String, SourceAddress);
    WRITE_DATA4(String, ProtocolFeatures);
    WRITE_DATA(Bool, SshSimple);
  }

  WRITE_DATA(Integer, ProxyMethod);
  WRITE_DATA(String, ProxyHost);
  WRITE_DATA2(Integer, ProxyPort);
  WRITE_DATA(String, ProxyUsername);
  if (GetProxyMethod() == pmCmd)
  {
    WRITE_DATA_EX(StringRaw, "ProxyTelnetCommand", GetProxyLocalCommand(), );
  }
  else
  {
    WRITE_DATA_EX(StringRaw, "ProxyTelnetCommand", GetProxyTelnetCommand(), );
  }
  #undef WRITE_DATA_CONV_FUNC
  #define WRITE_DATA_CONV_FUNC(X) (((X) + 2) % 3)
  WRITE_DATA_CONV(Integer, "ProxyDNS", GetProxyDNS());
#undef WRITE_DATA_CONV_FUNC
  WRITE_DATA_EX(Bool, "ProxyLocalhost", GetProxyLocalhost(), );

  #undef WRITE_DATA_CONV_FUNC
  #define WRITE_DATA_CONV_FUNC(X) (2 - (X))
  #define WRITE_BUG(BUG) WRITE_DATA_CONV(Integer, MB_TEXT("Bug" #BUG), GetBug(sb##BUG));
  WRITE_BUG(HMAC2);
  WRITE_BUG(DeriveKey2);
  WRITE_BUG(RSAPad2);
  WRITE_BUG(PKSessID2);
  WRITE_BUG(Rekey2);
  WRITE_BUG(MaxPkt2);
  WRITE_BUG(Ignore2);
  WRITE_BUG(OldGex2);
  WRITE_BUG(WinAdj);
  WRITE_BUG(ChanReq);
  #undef WRITE_BUG
  #undef WRITE_DATA_CONV_FUNC

  Storage->DeleteValue("BuggyMAC");
  Storage->DeleteValue("AliasGroupList");

  if (PuttyExport)
  {
    WRITE_DATA_EX(String, "Protocol", GetNormalizedPuttyProtocol(), );
    WRITE_DATA(String, WinTitle);
  }

  if (!PuttyExport)
  {
    WRITE_DATA(String, SftpServer);

  #define WRITE_SFTP_BUG(BUG) WRITE_DATA_EX(Integer, MB_TEXT("SFTP" #BUG "Bug"), GetSFTPBug(sb##BUG), );
    WRITE_SFTP_BUG(Symlink);
    WRITE_SFTP_BUG(SignedTS);
  #undef WRITE_SFTP_BUG

    WRITE_DATA2(Integer, SFTPMaxVersion);
    WRITE_DATA2(Integer, SFTPMaxPacketSize);
    WRITE_DATA2(Integer, SFTPMinPacketSize);
    WRITE_DATA2(Integer, SFTPDownloadQueue);
    WRITE_DATA2(Integer, SFTPUploadQueue);
    WRITE_DATA2(Integer, SFTPListingQueue);
    WRITE_DATA3(Integer, SFTPRealPath);

    WRITE_DATA2(Integer, Color);

    WRITE_DATA(Bool, Tunnel);
    WRITE_DATA(String, TunnelHostName);
    WRITE_DATA2(Integer, TunnelPortNumber);
    WRITE_DATA4(String, TunnelUserName);
    WRITE_DATA4(String, TunnelPublicKeyFile);
    WRITE_DATA2(Integer, TunnelLocalPortNumber);
    WRITE_DATA4(String, TunnelHostKey);

    WRITE_DATA(Bool, FtpPasvMode);
    WRITE_DATA_EX(Integer, "FtpForcePasvIp2", GetFtpForcePasvIp(), );
    WRITE_DATA(Integer, FtpUseMlsd);
    WRITE_DATA(String, FtpAccount);
    WRITE_DATA2(Integer, FtpPingInterval);
    WRITE_DATA(Integer, FtpPingType);
    WRITE_DATA_EX(Integer, "FtpTransferActiveImmediately2", GetFtpTransferActiveImmediately(), );
    WRITE_DATA(Integer, Ftps);
    WRITE_DATA(Integer, FtpListAll);
    WRITE_DATA(Integer, FtpHost);
    WRITE_DATA(Bool, FtpDupFF);
    WRITE_DATA(Bool, FtpUndupFF);
    WRITE_DATA3(Integer, FtpWorkFromCwd);
    WRITE_DATA3(Bool, FtpAnyCodeForPwd);
    WRITE_DATA(Bool, SslSessionReuse);
    WRITE_DATA(String, TlsCertificateFile);

    WRITE_DATA2(Integer, FtpProxyLogonType);

    WRITE_DATA(Integer, MinTlsVersion);
    WRITE_DATA(Integer, MaxTlsVersion);

    WRITE_DATA3(Bool, WebDavLiberalEscaping);
    WRITE_DATA3(Bool, WebDavAuthLegacy);

#if 0
    WRITE_DATA(Bool, IsWorkspace);
    WRITE_DATA(String, Link);
    WRITE_DATA(String, NameOverride);
#endif // #if 0

    WRITE_DATA4(String, PuttySettings);

    WRITE_DATA(String, CustomParam1);
    WRITE_DATA(String, CustomParam2);

    if (!GetCodePage().IsEmpty())
    {
      WRITE_DATA_EX(String, "CodePage", GetCodePage(), );
    }
    WRITE_DATA_EX(Integer, "LoginType", GetLoginType(), );
    WRITE_DATA_EX(Bool, "FtpAllowEmptyPassword", GetFtpAllowEmptyPassword(), );
  }

  // This is for collecting all keys for TSiteRawDialog::AddButtonClick.
  // It should be enough to test for (Default == nullptr),
  // the DoNotEncryptPasswords and PuttyExport were added to limit a possible unintended impact.
  bool SaveAll = (Default == nullptr) && DoNotEncryptPasswords && !PuttyExport;

  SavePasswords(Storage, PuttyExport, DoNotEncryptPasswords, SaveAll);

  if (PuttyExport)
  {
    WritePuttySettings(Storage, FPuttySettings);
  }
}

TStrings * TSessionData::SaveToOptions(const TSessionData * Default, bool SaveName, bool PuttyExport)
{
  TODO("implement");
  ThrowNotImplemented(3041);
#if 0
  std::unique_ptr<TStringList> Options(std::make_unique<TStringList>());
  std::unique_ptr<TOptionsStorage> OptionsStorage(std::make_unique<TOptionsStorage>(Options.get(), true, false));
  if (SaveName)
  {
    OptionsStorage->WriteString(L"Name", Name);
  }
  DoSave(OptionsStorage.get(), PuttyExport, Default, true);
  return Options.release();
#endif
  return nullptr;
}

void TSessionData::Save(THierarchicalStorage * Storage,
  bool PuttyExport, const TSessionData * Default)
{
  if (Storage->OpenSubKey(GetInternalStorageKey(), true))
  {
    DoSave(Storage, PuttyExport, Default, false);

    Storage->CloseSubKey();
  }
}

#if 0
UnicodeString TSessionData::ReadXmlNode(_di_IXMLNode Node, const UnicodeString & Name, const UnicodeString & Default)
{
  _di_IXMLNode TheNode = Node->ChildNodes->FindNode(Name);
  UnicodeString Result;
  if (TheNode != nullptr)
  {
    Result = TheNode->Text.Trim();
  }

  if (Result.IsEmpty())
  {
    Result = Default;
  }

  return Result;
}

int32_t TSessionData::ReadXmlNode(_di_IXMLNode Node, const UnicodeString & Name, int Default)
{
  _di_IXMLNode TheNode = Node->ChildNodes->FindNode(Name);
  int32_t Result;
  if (TheNode != nullptr)
  {
    Result = StrToIntDef(TheNode->Text.Trim(), Default);
  }
  else
  {
    Result = Default;
  }

  return Result;
}

_di_IXMLNode TSessionData::FindSettingsNode(_di_IXMLNode Node, const UnicodeString & Name)
{
  for (int32_t Index = 0; Index < Node->ChildNodes->Count; Index++)
  {
    _di_IXMLNode ChildNode = Node->ChildNodes->Get(Index);
    if (ChildNode->NodeName == L"Setting")
    {
       OleVariant SettingName = ChildNode->GetAttribute(L"name");
       if (SettingName == Name)
       {
         return ChildNode;
       }
    }
  }

  return nullptr;
}

UnicodeString TSessionData::ReadSettingsNode(_di_IXMLNode Node, const UnicodeString & Name, UnicodeString & Default)
{
  _di_IXMLNode TheNode = FindSettingsNode(Node, Name);
  UnicodeString Result;
  if (TheNode != nullptr)
  {
    Result = TheNode->Text.Trim();
  }

  if (Result.IsEmpty())
  {
    Result = Default;
  }

  return Result;
}

int TSessionData::ReadSettingsNode(_di_IXMLNode Node, const UnicodeString & Name, int Default)
{
  _di_IXMLNode TheNode = FindSettingsNode(Node, Name);
  int Result;
  if (TheNode != nullptr)
  {
    Result = StrToIntDef(TheNode->Text.Trim(), Default);
  }
  else
  {
    Result = Default;
  }

  return Result;
}

void TSessionData::ImportFromFilezilla(
  _di_IXMLNode Node, const UnicodeString Path, _di_IXMLNode SettingsNode)
{
  Name = UnixIncludeTrailingBackslash(Path) + MakeValidName(ReadXmlNode(Node, L"Name", Name));
  HostName = ReadXmlNode(Node, L"Host", HostName);
  PortNumber = ReadXmlNode(Node, L"Port", PortNumber);

  int32_t AProtocol = ReadXmlNode(Node, L"Protocol", 0);
  // ServerProtocol enum
  switch (AProtocol)
  {
  case 0: // FTP
  default: // UNKNOWN, HTTP, HTTPS, INSECURE_FTP
    FSProtocol = fsFTP;
    break;

  case 1: // SFTP
    FSProtocol = fsSFTP;
    break;

  case 3: // FTPS
    FSProtocol = fsFTP;
    Ftps = ftpsImplicit;
    break;

  case 4: // FTPES
    FSProtocol = fsFTP;
    Ftps = ftpsExplicitTls;
    break;
  }

  // LogonType enum
  int32_t LogonType = ReadXmlNode(Node, L"Logontype", 0);
  if (LogonType == 0) // ANONYMOUS
  {
    UserName = AnonymousUserName;
    Password = AnonymousPassword;
  }
  else
  {
    UserName = ReadXmlNode(Node, L"User", UserName);
    FtpAccount = ReadXmlNode(Node, L"Account", FtpAccount);

    _di_IXMLNode PassNode = Node->ChildNodes->FindNode(L"Pass");
    if (PassNode != nullptr)
    {
      UnicodeString APassword = PassNode->Text.Trim();
      OleVariant EncodingValue = PassNode->GetAttribute(L"encoding");
      if (!EncodingValue.IsNull())
      {
        UnicodeString EncodingValueStr = EncodingValue;
        if (SameText(EncodingValueStr, L"base64"))
        {
          TBytes Bytes = DecodeBase64(APassword);
          APassword = TEncoding::UTF8->GetString(Bytes);
        }
      }
      Password = APassword;
    }
  }

  PublicKeyFile = ReadXmlNode(Node, L"Keyfile", PublicKeyFile);

  int32_t DefaultTimeDifference = TimeToSeconds(TimeDifference);
  TimeDifference =
    (double(ReadXmlNode(Node, L"TimezoneOffset", DefaultTimeDifference) / SecsPerDay));
  TimeDifferenceAuto = (TimeDifference == TDateTime());

  UnicodeString PasvMode = ReadXmlNode(Node, L"PasvMode", L"");
  if (SameText(PasvMode, L"MODE_PASSIVE"))
  {
    FtpPasvMode = true;
  }
  else if (SameText(PasvMode, L"MODE_ACTIVE"))
  {
    FtpPasvMode = false;
  }
  else if (SettingsNode != nullptr)
  {
    int PasvMode = ReadSettingsNode(SettingsNode, L"Use Pasv mode", 1);
    FtpPasvMode = (PasvMode != 0);
  }

  UnicodeString EncodingType = ReadXmlNode(Node, L"EncodingType", L"");
  if (SameText(EncodingType, L"Auto"))
  {
    NotUtf = asAuto;
  }
  else if (SameText(EncodingType, L"UTF-8"))
  {
    NotUtf = asOff;
  }

  // todo PostLoginCommands

  Note = ReadXmlNode(Node, L"Comments", Note);

  LocalDirectory = ReadXmlNode(Node, L"LocalDir", LocalDirectory);

  UnicodeString RemoteDir = ReadXmlNode(Node, L"RemoteDir", L"");
  if (!RemoteDir.IsEmpty())
  {
    CutToChar(RemoteDir, L' ', false); // type
    int32_t PrefixSize = StrToIntDef(CutToChar(RemoteDir, L' ', false), 0); // prefix size
    if (PrefixSize > 0)
    {
      RemoteDir.Delete(1, PrefixSize);
    }
    RemoteDirectory = L"/";
    while (!RemoteDir.IsEmpty())
    {
      int32_t SegmentSize = StrToIntDef(CutToChar(RemoteDir, L' ', false), 0);
      UnicodeString Segment = RemoteDir.SubString(1, SegmentSize);
      RemoteDirectory = UnixIncludeTrailingBackslash(RemoteDirectory) + Segment;
      RemoteDir.Delete(1, SegmentSize + 1);
    }
  }

  SynchronizeBrowsing = (ReadXmlNode(Node, L"SyncBrowsing", SynchronizeBrowsing ? 1 : 0) != 0);

  if (SettingsNode != nullptr)
  {
    if (UsesSsh && PublicKeyFile.IsEmpty())
    {
      UnicodeString KeyFiles = ReadSettingsNode(SettingsNode, L"SFTP keyfiles", UnicodeString());
      UnicodeString KeyFile = CutToChar(KeyFiles, L'\n', true).Trim();
      KeyFiles = KeyFiles.Trim();
      // If there are more keys, ignore them, as we do not know which one to use
      if (!KeyFile.IsEmpty() && KeyFiles.IsEmpty())
      {
        PublicKeyFile = KeyFile;
      }
    }

    bool BypassProxy = (ReadXmlNode(Node, L"BypassProxy", 0) != 0);
    if (!BypassProxy)
    {
      int32_t FtpProxyType = ReadSettingsNode(SettingsNode, L"FTP Proxy type", -1);
      if (FtpProxyType > 0)
      {
        switch (FtpProxyType)
        {
        case 1:
          FtpProxyLogonType = 2;
          break;
        case 2:
          FtpProxyLogonType = 1;
          break;
        case 3:
          FtpProxyLogonType = 3;
          break;
        case 4:
          // custom
          // TODO: map known sequences to our enumeration
          FtpProxyLogonType = 0;
          break;
        default:
          DebugFail();
          FtpProxyLogonType = 0;
          break;
        }

        ProxyHost = ReadSettingsNode(SettingsNode, L"FTP Proxy host", ProxyHost);
        ProxyUsername = ReadSettingsNode(SettingsNode, L"FTP Proxy user", ProxyUsername);
        ProxyPassword = ReadSettingsNode(SettingsNode, L"FTP Proxy password", ProxyPassword);
        // ProxyPort is not used with FtpProxyLogonType
      }
      else
      {
        int32_t ProxyType = ReadSettingsNode(SettingsNode, L"Proxy type", -1);
        if (ProxyType >= 0)
        {
          switch (ProxyType)
          {
          case 0:
            ProxyMethod = ::pmNone;
            break;

          case 1:
            ProxyMethod = pmHTTP;
            break;

          case 2:
            ProxyMethod = pmSocks5;
            break;

          case 3:
            ProxyMethod = pmSocks4;
            break;

          default:
            DebugFail();
            ProxyMethod = ::pmNone;
            break;
          }

          ProxyHost = ReadSettingsNode(SettingsNode, L"Proxy host", ProxyHost);
          ProxyPort = ReadSettingsNode(SettingsNode, L"Proxy port", ProxyPort);
          ProxyUsername = ReadSettingsNode(SettingsNode, L"Proxy user", ProxyUsername);
          ProxyPassword = ReadSettingsNode(SettingsNode, L"Proxy password", ProxyPassword);
        }
      }
    }
  }

}
#endif // #if 0

bool OpensshBoolValue(const UnicodeString & Value)
{
  return SameText(Value, L"yes");
}

UnicodeString CutOpensshToken(UnicodeString & S)
{
  const wchar_t NoQuote = L'\0';
  wchar_t Quote = NoQuote;
  UnicodeString Result;
  int32_t P = 1;
  while (P <= S.Length())
  {
    wchar_t C = S[P];
    if ((C == L'\\') &&
        (P < S.Length()) &&
        ((S[P + 1] == L'\'') ||
         (S[P + 1] == L'\"') ||
         (S[P + 1] == L'\\') ||
         ((Quote == NoQuote) && S[P + 1] == L' ')))
    {
      Result += S[P + 1];
      P++;
    }
    else if ((Quote == NoQuote) && ((C == L' ') || (C == L'\t')))
    {
      break;
    }
    else if ((Quote == NoQuote) && ((C == L'\'') || (C == L'"')))
    {
      Quote = C;
    }
    else if ((Quote != NoQuote) && (Quote == C))
    {
      Quote = NoQuote;
    }
    else
    {
      Result += C;
    }
    P++;
  }

  S = MidStr(S, P).Trim();

  return Result;
}

static UnicodeString ConvertPathFromOpenssh(const UnicodeString & Path)
{
  // It's likely there would be forward slashes in OpenSSH config file and our load/save dialogs
  // (e.g. when converting keys) work suboptimally when working with forward slashes.
  UnicodeString Result = GetNormalizedPath(Path);
  const UnicodeString HomePathPrefix = L"~";
  if (StartsStr(HomePathPrefix, Result + L"\\"))
  {
    Result =
      GetShellFolderPath(CSIDL_PROFILE) +
      Result.SubString(HomePathPrefix.Length() + 1, Result.Length() - HomePathPrefix.Length());
  }
  return Result;
}

void TSessionData::ImportFromOpenssh(TStrings * Lines)
{
  bool SkippingSection = false;
  std::unique_ptr<TStrings> UsedDirectives(CreateSortedStringList());
  for (int32_t Index = 0; Index < Lines->Count; Index++)
  {
    UnicodeString Line = Lines->GetString(Index);
    UnicodeString Directive, Args;
    if (ParseOpensshDirective(Line, Directive, Args))
    {
      if (SameText(Directive, OpensshHostDirective))
      {
        SkippingSection = true;
        while (!Args.IsEmpty())
        {
          UnicodeString M = CutOpensshToken(Args);
          bool Negated = DebugAlwaysTrue(!M.IsEmpty()) && (M[1] == L'!');
          if (Negated)
          {
            M.Delete(1, 1);
          }
          TFileMasks Mask;
          Mask.SetMask(M);
          // This does way more than OpenSSH, but on the other hand, the special characters of our file masks,
          // should not be present in hostnames.
          if (Mask.MatchesFileName(Name))
          {
            if (Negated)
            {
              // Skip even if matched by other possitive patterns
              SkippingSection = true;
              break;
            }
            else
            {
              // Keep looking, in case if negated
              SkippingSection = false;
            }
          }
        }
      }
      else if (SameText(Directive, L"Match"))
      {
        SkippingSection = true;
      }
      else if (!SkippingSection && (UsedDirectives->IndexOf(Directive) < 0))
      {
        UnicodeString Value = CutOpensshToken(Args);
        // All the directives we support allow only one token
        if (Args.IsEmpty())
        {
          if (SameText(Directive, L"AddressFamily"))
          {
            if (SameText(Value, L"inet"))
            {
              AddressFamily = afIPv4;
            }
            else if (SameText(Value, L"inet6"))
            {
              AddressFamily = afIPv6;
            }
            else
            {
              AddressFamily = afAuto;
            }
          }
          else if (SameText(Directive, L"BindAddress"))
          {
            SourceAddress = Value;
          }
          else if (SameText(Directive, L"Compression"))
          {
            Compression = OpensshBoolValue(Value);
          }
          else if (SameText(Directive, L"ForwardAgent"))
          {
            AgentFwd = OpensshBoolValue(Value);
          }
          else if (SameText(Directive, L"GSSAPIAuthentication"))
          {
            AuthGSSAPI = OpensshBoolValue(Value);
          }
          else if (SameText(Directive, L"GSSAPIDelegateCredentials"))
          {
            AuthGSSAPIKEX = OpensshBoolValue(Value);
          }
          else if (SameText(Directive, L"Hostname"))
          {
            HostName = Value;
          }
          else if (SameText(Directive, L"IdentityFile"))
          {
            PublicKeyFile = ConvertPathFromOpenssh(Value);
          }
          else if (SameText(Directive, L"CertificateFile"))
          {
            DetachedCertificate = ConvertPathFromOpenssh(Value);
          }
          else if (SameText(Directive, L"KbdInteractiveAuthentication"))
          {
            AuthKI = OpensshBoolValue(Value);
          }
          else if (SameText(Directive, L"Port"))
          {
            PortNumber = ::StrToIntDef(Value, 0);
          }
          else if (SameText(Directive, L"User"))
          {
            UserName = Value;
          }
          else if (SameText(Directive, L"ProxyJump"))
          {
            UnicodeString Jump = Value;
            // multiple jumps are not supported
            if (Jump.Pos(L",") == 0)
            {
              std::unique_ptr<TSessionData> JumpData(std::make_unique<TSessionData>(EmptyStr));
              bool DefaultsOnly;
              if ((JumpData->ParseUrl(Jump, nullptr, nullptr, DefaultsOnly, nullptr, nullptr, nullptr, 0)) &&
                  !JumpData->HostName().IsEmpty())
              {
                JumpData->Name = JumpData->HostName;
                JumpData->ImportFromOpenssh(Lines);

                Tunnel = true;
                TunnelHostName = JumpData->HostName;
                TunnelPortNumber = JumpData->PortNumber;
                TunnelUserName = JumpData->UserName;
                TunnelPassword = JumpData->Password;
                TunnelPublicKeyFile = JumpData->PublicKeyFile;
              }
            }
          }
          UsedDirectives->Add(Directive);
        }
      }
    }
  }
}

void TSessionData::SavePasswords(THierarchicalStorage * Storage, bool PuttyExport, bool DoNotEncryptPasswords, bool SaveAll)
{
  // It's probably safe to replace this with if (!PuttyExport) { SAVE_PASSWORD(...) }
  if (!GetConfiguration()->DisablePasswordStoring && !PuttyExport && (!FPassword.IsEmpty() || SaveAll))
  {
    if (DoNotEncryptPasswords)
    {
      Storage->WriteString(L"PasswordPlain", Password);
      Storage->DeleteValue(L"Password");
    }
    else
    {
      Storage->WriteBinaryDataAsString(L"Password", StronglyRecryptPassword(FPassword, FUserName + FHostName));
      Storage->DeleteValue(L"PasswordPlain");
    }
  }
  else
  {
    Storage->DeleteValue(L"Password");
    Storage->DeleteValue(L"PasswordPlain");
  }

  if (PuttyExport)
  {
    // save password unencrypted
    Storage->WriteString(L"ProxyPassword", FProxyPassword);
  }
  else
  {
    #define SAVE_PASSWORD_EX(PROP, PLAIN_NAME, ENC_NAME, ENC_KEY, COND) \
      if (DoNotEncryptPasswords) \
      { \
        if (!F##PROP.IsEmpty() || SaveAll) \
        { \
          Storage->WriteString(PLAIN_NAME, F ## PROP); \
        } \
        else \
        { \
          Storage->DeleteValue(PLAIN_NAME); \
        } \
        Storage->DeleteValue(ENC_NAME); \
      } \
      else \
      { \
        if (COND && (!F##PROP.IsEmpty() || SaveAll)) \
        { \
          Storage->WriteBinaryDataAsString(ENC_NAME, StronglyRecryptPassword(F##PROP, ENC_KEY)); \
        } \
        else \
        { \
          Storage->DeleteValue(ENC_NAME); \
        } \
        Storage->DeleteValue(PLAIN_NAME); \
      }
    #define SAVE_PASSWORD(PROP, PLAIN_NAME, ENC_KEY) SAVE_PASSWORD_EX(PROP, PLAIN_NAME, TEXT(#PROP), ENC_KEY, !GetConfiguration()->DisablePasswordStoring)

    SAVE_PASSWORD_EX(ProxyPassword, L"ProxyPassword", L"ProxyPasswordEnc", FProxyUsername + FProxyHost, true);
    SAVE_PASSWORD(TunnelPassword, L"TunnelPasswordPlain", FTunnelUserName + FTunnelHostName);
    SAVE_PASSWORD_EX(EncryptKey, L"EncryptKeyPlain", L"EncryptKey", FUserName + FHostName, true);
  }
}

void TSessionData::RecryptPasswords()
{
  SetPassword(GetPassword());
  SetNewPassword(GetNewPassword());
  SetProxyPassword(GetProxyPassword());
  FTunnelPassword = FTunnelPassword; // TODO: SetTunnelPassword(GetTunnelPassword)
  SetTunnelPassword(GetTunnelPassword());
  SetPassphrase(GetPassphrase());
  SetEncryptKey(GetEncryptKey());
}

// Caching read password files, particularly when the file is actually a named pipe
// that does not support repeated reading.
static std::unique_ptr<TCriticalSection> PasswordFilesCacheSection(TraceInitPtr(std::make_unique<TCriticalSection>()));
typedef rde::map<UnicodeString, UnicodeString> TPasswordFilesCache;
static TPasswordFilesCache PasswordFilesCache;

static UnicodeString ReadPasswordFromFile(const UnicodeString & FileName)
{
  UnicodeString Result;
  if (!FileName.IsEmpty())
  {
    TGuard Guard(*PasswordFilesCacheSection.get());
    TPasswordFilesCache::iterator I = PasswordFilesCache.find(FileName);
    if (I != PasswordFilesCache.end())
    {
      Result = I->second;
    }
    else
    {
      std::unique_ptr<TStrings> Lines(std::make_unique<TStringList>());
#if 0
      LoadScriptFromFile(FileName, Lines.get());
#endif //if 0
      if (Lines->Count > 0)
      {
        Result = Lines->Strings[0];
      }
      PasswordFilesCache[FileName] = Result;
    }
  }
  return Result;
}

void TSessionData::ReadPasswordsFromFiles()
{
  Password = ReadPasswordFromFile(Password);
  NewPassword = ReadPasswordFromFile(NewPassword);
  ProxyPassword = ReadPasswordFromFile(ProxyPassword);
  TunnelPassword = ReadPasswordFromFile(TunnelPassword);
  TunnelPassphrase = ReadPasswordFromFile(TunnelPassphrase);
  Passphrase = ReadPasswordFromFile(Passphrase);
  EncryptKey = ReadPasswordFromFile(EncryptKey);
}

bool TSessionData::HasPassword() const
{
  return !FPassword.IsEmpty();
}

bool TSessionData::HasAnySessionPassword() const
{
  // Keep in sync with ClearSessionPasswords
  return
    HasPassword() ||
    !FTunnelPassword.IsEmpty() ||
    // will probably be never used
    !FNewPassword.IsEmpty();
}

bool TSessionData::HasAnyPassword() const
{
  // Keep in sync with MaskPasswords
  return
    HasAnySessionPassword() ||
    !FProxyPassword.IsEmpty() ||
    !FEncryptKey.IsEmpty() ||
    !FPassphrase.IsEmpty() ||
    !FTunnelPassphrase.IsEmpty();
}

void TSessionData::ClearSessionPasswords()
{
  // Keep in sync with HasAnySessionPassword
  FPassword = L"";
  FNewPassword = L"";
  FTunnelPassword = L"";
}

void TSessionData::Modify()
{
  FModified = true;
  if (FSource == ssStored)
  {
    FSource = ssStoredModified;
  }
}

UnicodeString TSessionData::GetSource() const
{
  switch (FSource)
  {
  case ::ssNone:
    return L"Ad-Hoc site";

  case ssStored:
    return L"Site";

  case ssStoredModified:
    return L"Modified site";

  default:
    DebugFail();
    return L"";
  }
}

void TSessionData::SaveRecryptedPasswords(THierarchicalStorage * Storage)
{
  if (Storage->OpenSubKey(GetInternalStorageKey(), true))
  {
    try__finally
    {
      RecryptPasswords();

      SavePasswords(Storage, false, false, false);
    },
    __finally
    {
      Storage->CloseSubKey();
    } end_try__finally
  }
}

void TSessionData::Remove(THierarchicalStorage * Storage, const UnicodeString & Name)
{
  Storage->RecursiveDeleteSubKey(Name);
}

void TSessionData::Remove()
{
  bool SessionList = true;
  std::unique_ptr<THierarchicalStorage> Storage(GetConfiguration()->CreateScpStorage(SessionList));
  try__finally
  {
    Storage->SetExplicit(true);
    if (Storage->OpenSubKey(GetConfiguration()->GetStoredSessionsSubKey(), false))
    {
      Remove(Storage.get(), GetInternalStorageKey());
    }
  },
  __finally__removed
  ({
    delete Storage;
  }) end_try__finally
}

void TSessionData::CacheHostKeyIfNotCached()
{
  UnicodeString KeyType = KeyTypeFromFingerprint(GetHostKey());

  // Should allow importing to INI file as ImportHostKeys
  UnicodeString TargetKey = GetConfiguration()->GetRegistryStorageKey() + L"\\" + GetConfiguration()->GetSshHostKeysSubKey();
  std::unique_ptr<TRegistryStorage> Storage(std::make_unique<TRegistryStorage>(TargetKey));
  Storage->Init();
  Storage->SetAccessMode(smReadWrite);
  if (Storage->OpenRootKey(true))
  {
    UnicodeString HostKeyName = PuttyMungeStr(FORMAT("%s@%d:%s", KeyType, GetPortNumber(), GetHostName()));
    if (!Storage->ValueExists(HostKeyName))
    {
      // fingerprint is a checksum of a host key, so it cannot be translated back to host key,
      // so we store fingerprint and TSecureShell::VerifyHostKey was
      // modified to accept also fingerprint
      Storage->WriteString(HostKeyName, GetHostKey());
    }
  }
}

inline void MoveStr(UnicodeString & Source, UnicodeString * Dest, int32_t Count)
{
  if (Dest != nullptr)
  {
    (*Dest) += Source.SubString(1, Count);
  }

  Source.Delete(1, Count);
}

bool TSessionData::DoIsProtocolUrl(
  const UnicodeString & Url, const UnicodeString & Protocol, int32_t & ProtocolLen)
{
  bool Result = ::SameText(Url.SubString(1, Protocol.Length() + 1), Protocol + L":");
  if (Result)
  {
    ProtocolLen = Protocol.Length() + 1;
  }
  return Result;
}

bool TSessionData::IsProtocolUrl(
  const UnicodeString & Url, const UnicodeString & Protocol, int32_t & ProtocolLen)
{
  return
    DoIsProtocolUrl(Url, Protocol, ProtocolLen) ||
    DoIsProtocolUrl(Url, WinSCPProtocolPrefix + Protocol, ProtocolLen);
}

bool TSessionData::IsSensitiveOption(const UnicodeString & Option, const UnicodeString & Value)
{
  bool Result;
  if (SameText(Option, PassphraseOption) ||
      SameText(Option, PASSWORD_SWITCH) ||
      SameText(Option, NEWPASSWORD_SWITCH))
  {
    Result = true;
  }
  else if (SameText(Option, PRIVATEKEY_SWITCH))
  {
    Filename * AFilename = filename_from_str(UTF8String(Value).c_str());
    Result = (in_memory_key_data(AFilename) != nullptr);
    filename_free(AFilename);
  }
  else
  {
    Result = false;
  }
  return Result;
}

bool TSessionData::IsOptionWithParameters(const UnicodeString & Option)
{
  return SameText(Option, RawSettingsOption);
}

bool TSessionData::MaskPasswordInOptionParameter(const UnicodeString & AOption, UnicodeString & AParam)
{
  bool Result = false;
  if (SameText(AOption, RawSettingsOption))
  {
    int32_t P = AParam.Pos(L"=");
    if (P > 0)
    {
      // TStrings.IndexOfName does not trim
      UnicodeString Key = AParam.SubString(1, P - 1);

      if (SameText(Key, L"ProxyPassword") ||
          SameText(Key, L"ProxyPasswordEnc") ||
          SameText(Key, L"TunnelPassword") ||
          SameText(Key, L"TunnelPasswordPlain") ||
          SameText(Key, L"TunnelPassphrase") ||
          SameText(Key, L"TunnelPassphrasePlain") ||
          SameText(Key, L"EncryptKey") ||
          SameText(Key, L"EncryptKeyPlain"))
      {
        AParam = Key + L"=" + PasswordMask;
        Result = true;
      }
    }
  }
  return Result;
}

void TSessionData::MaskPasswords()
{
  // Keep in sync with HasAnyPassword
  if (!FPassword.IsEmpty())
  {
    FPassword = PasswordMask;
  }
  if (!GetPassword().IsEmpty())
  {
    SetPassword(PasswordMask);
  }
  if (!GetNewPassword().IsEmpty())
  {
    SetNewPassword(PasswordMask);
  }
  if (!GetProxyPassword().IsEmpty())
  {
    SetProxyPassword(PasswordMask);
  }
  if (!GetTunnelPassword().IsEmpty())
  {
    SetTunnelPassword(PasswordMask);
  }
  if (!EncryptKey().IsEmpty())
  {
    EncryptKey = PasswordMask;
  }
  {
    SetPassphrase(PasswordMask);
  }
}

static bool IsDomainOrSubdomain(const UnicodeString & FullDomain, const UnicodeString & Domain)
{
  return
    SameText(FullDomain, Domain) ||
    EndsText(L"." + Domain, FullDomain);
}

bool TSessionData::ParseUrl(const UnicodeString & AUrl, TOptions * Options,
  TStoredSessionList * AStoredSessions, bool & DefaultsOnly, UnicodeString * AFileName,
  bool * AProtocolDefined, UnicodeString * MaskedUrl, int32_t Flags)
{
  UnicodeString Url = AUrl;
  bool ProtocolDefined = true;
  bool PortNumberDefined = false;
  TFSProtocol AFSProtocol = fsSCPonly;
  int32_t DefaultProtocolPortNumber = 0;
  TFtps AFtps = ftpsNone;
  int32_t ProtocolLen = 0;
  bool HttpForWebdav = FLAGCLEAR(Flags, pufPreferProtocol) || (FSProtocol != fsS3);
  if (IsProtocolUrl(Url, ScpProtocol, ProtocolLen))
  {
    AFSProtocol = fsSCPonly;
    DefaultProtocolPortNumber = SshPortNumber;
  }
  else if (IsProtocolUrl(Url, SftpProtocol, ProtocolLen))
  {
    AFSProtocol = fsSFTPonly;
    DefaultProtocolPortNumber = SshPortNumber;
  }
  else if (IsProtocolUrl(Url, FtpProtocol, ProtocolLen))
  {
    AFSProtocol = fsFTP;
    Ftps = ftpsNone;
    DefaultProtocolPortNumber = FtpPortNumber;
  }
  else if (IsProtocolUrl(Url, FtpsProtocol, ProtocolLen))
  {
    AFSProtocol = fsFTP;
    AFtps = ftpsImplicit;
    DefaultProtocolPortNumber = FtpsImplicitPortNumber;
  }
  else if (IsProtocolUrl(Url, FtpesProtocol, ProtocolLen))
  {
    AFSProtocol = fsFTP;
    AFtps = ftpsExplicitTls;
    DefaultProtocolPortNumber = FtpPortNumber;
  }
  else if (IsProtocolUrl(Url, WebDAVProtocol, ProtocolLen) ||
           (HttpForWebdav && IsProtocolUrl(Url, HttpProtocol, ProtocolLen)))
  {
    AFSProtocol = fsWebDAV;
    AFtps = ftpsNone;
    DefaultProtocolPortNumber = HTTPPortNumber;
  }
  else if (IsProtocolUrl(Url, WebDAVSProtocol, ProtocolLen) ||
           (HttpForWebdav && IsProtocolUrl(Url, HttpsProtocol, ProtocolLen)))
  {
    AFSProtocol = fsWebDAV;
    AFtps = ftpsImplicit;
    DefaultProtocolPortNumber = HTTPSPortNumber;
  }
  else if (IsProtocolUrl(Url, S3PlainProtocol, ProtocolLen) ||
           IsProtocolUrl(Url, HttpProtocol, ProtocolLen))
  {
    AFSProtocol = fsS3;
    AFtps = ftpsNone;
    DefaultProtocolPortNumber = HTTPPortNumber;
  }
  else if (IsProtocolUrl(Url, S3Protocol, ProtocolLen) ||
           IsProtocolUrl(Url, HttpsProtocol, ProtocolLen))
  {
    AFSProtocol = fsS3;
    AFtps = ftpsImplicit;
    DefaultProtocolPortNumber = HTTPSPortNumber;
  }
  else if (IsProtocolUrl(Url, SshProtocol, ProtocolLen))
  {
    // For most uses, handling ssh:// the same way as sftp://
    // The only place where a difference is made is GetLoginData() in WinMain.cpp
    AFSProtocol = fsSFTPonly;
    FPuttyProtocol = PuttySshProtocol;
    DefaultProtocolPortNumber = SshPortNumber;
  }
  else
  {
    ProtocolDefined = false;
  }

  if (ProtocolDefined)
  {
    MoveStr(Url, MaskedUrl, ProtocolLen);
  }

  if (ProtocolDefined && (Url.SubString(1, 2) == L"//"))
  {
    MoveStr(Url, MaskedUrl, 2);
  }

  if (AProtocolDefined != nullptr)
  {
    *AProtocolDefined = ProtocolDefined;
  }

  bool Unsafe = FLAGSET(Flags, pufUnsafe);
  if (!Url.IsEmpty())
  {
    UnicodeString DecodedUrl = DecodeUrlChars(Url);
    // lookup stored session even if protocol was defined
    // (this allows setting for example default username for host
    // by creating stored session named by host)
    TSessionData * Data = nullptr;
    // When using to paste URL on Login dialog, we do not want to lookup the stored sites
    if ((StoredSessions != nullptr) &&
        (!ProtocolDefined || FLAGSET(Flags, pufAllowStoredSiteWithProtocol)))
    {
      // this can be optimized as the list is sorted
      for (Integer Index = 0; Index < AStoredSessions->GetCountIncludingHidden(); ++Index)
      {
        TSessionData * AData = AStoredSessions->GetAs<TSessionData>(Index);
        if (!AData->GetIsWorkspace())
        {
          bool Match = false;
          // Comparison optimizations as this is called many times
          // e.g. when updating jumplist
          UnicodeString Name = AData->GetName();
          if ((Name.Length() == DecodedUrl.Length()) &&
               SameText(Name, DecodedUrl))
          {
            Match = true;
          }
          else if ((Name.Length() < DecodedUrl.Length()) &&
                   // (DecodedUrl[Name.Length() + 1] == L'/') &&
                   // StrLIComp is an equivalent of SameText
                   (nb::StrLIComp(Name.c_str(), DecodedUrl.c_str(), Name.Length()) == 0))
          {
            Match = true;
          }

          if (Match)
          {
            Data = AData;
            break;
          }
        }
      }
    }

    UnicodeString ARemoteDirectory;

    bool ParseOnly = FLAGSET(Flags, pufParseOnly);
    if (Data != nullptr)
    {
      DoCopyData(Data, ParseOnly);
      FSource = Data->FSource;
      int32_t P = 1;
      while (!AnsiSameText(DecodeUrlChars(Url.SubString(1, P)), Data->Name))
      {
        P++;
        DebugAssert(P <= Url.Length());
      }
      ARemoteDirectory = Url.SubString(P + 1, Url.Length() - P);

      if (Data->Hidden && !ParseOnly)
      {
        Data->Remove();
        StoredSessions->Remove(Data);
        // only modified, implicit
        StoredSessions->Save(false, false);
      }

      if (MaskedUrl != nullptr)
      {
        (*MaskedUrl) += Url;
      }
    }
    else
    {
      // When ad-hoc URL is used, always display error when the directories are not valid,
      // no matter if they are part of the URL or raw settings.
      RequireDirectories = true;

      // This happens when pasting URL on Login dialog
      if (StoredSessions != nullptr)
      {
        DoCopyData(StoredSessions->DefaultSettings, ParseOnly);
      }
      Name = L"";

      int32_t PSlash = Url.Pos(L"/");
      if (PSlash == 0)
      {
        PSlash = Url.Length() + 1;
      }

      UnicodeString ConnectInfo = Url.SubString(1, PSlash - 1);

      int32_t P = ConnectInfo.LastDelimiter(L"@");

      UnicodeString UserInfo;
      UnicodeString HostInfo;

      if (P > 0)
      {
        UserInfo = ConnectInfo.SubString(1, P - 1);
        HostInfo = ConnectInfo.SubString(P + 1, ConnectInfo.Length() - P);
      }
      else
      {
        HostInfo = ConnectInfo;
      }

      UnicodeString OrigHostInfo = HostInfo;
      if ((HostInfo.Length() >= 2) && (HostInfo[1] == L'[') && ((P = HostInfo.Pos(L"]")) > 0))
      {
        SetHostName(HostInfo.SubString(2, P - 2));
        HostInfo.Delete(1, P);
        if (!HostInfo.IsEmpty() && (HostInfo[1] == L':'))
        {
          HostInfo.Delete(1, 1);
        }
      }
      else
      {
        SetHostName(DecodeUrlChars(CutToChar(HostInfo, L':', true)));
      }

      // expanded from ?: operator, as it caused strange "access violation" errors
      if (!HostInfo.IsEmpty())
      {
        int APortNumber = ::StrToIntDef(DecodeUrlChars(HostInfo), -1);
        if ((APortNumber > 0) && (APortNumber <= 65535))
        {
          PortNumber = APortNumber;
          PortNumberDefined = true;
        }
      }
      else if (ProtocolDefined)
      {
        if ((AFSProtocol == fsWebDAV) &&
            (IsDomainOrSubdomain(HostName, S3LibDefaultHostName()) ||
             IsDomainOrSubdomain(HostName, L"digitaloceanspaces.com") ||
             IsDomainOrSubdomain(HostName, L"storage.googleapis.com") ||
             IsDomainOrSubdomain(HostName, L"r2.cloudflarestorage.com")))
        {
          AFSProtocol = fsS3;
        }
        PortNumber = DefaultProtocolPortNumber;
      }

      if (ProtocolDefined)
      {
        SetFtps(AFtps);
      }

      UnicodeString UserInfoWithoutConnectionParams = CutToChar(UserInfo, UrlParamSeparator, false);
      UnicodeString ConnectionParams = UserInfo;
      UserInfo = UserInfoWithoutConnectionParams;

      std::unique_ptr<TStrings> RawSettings(std::make_unique<TStringList>());

      while (!ConnectionParams.IsEmpty())
      {
        UnicodeString ConnectionParam = CutToChar(ConnectionParams, UrlParamSeparator, false);
        UnicodeString ConnectionParamName = CutToChar(ConnectionParam, UrlParamValueSeparator, false);
        if (::SameText(ConnectionParamName, UrlHostKeyParamName))
        {
          SetHostKey(DecodeUrlChars(ConnectionParam));
          FOverrideCachedHostKey = false;
        }
        else if (StartsText(UrlRawSettingsParamNamePrefix, ConnectionParamName))
        {
          UnicodeString AName = RightStr(ConnectionParamName, ConnectionParamName.Length() - UrlRawSettingsParamNamePrefix.Length());
          AName = DecodeUrlChars(AName);
          UnicodeString Value = DecodeUrlChars(ConnectionParam);
          if (SameText(AName, L"Name"))
          {
            Name = Value;
          }
          else
          {
            RawSettings->SetValue(AName, Value);
          }
        }
      }

      if (RawSettings->Count > 0) // optimization
      {
        ApplyRawSettings(RawSettings.get(), Unsafe);
      }

      bool HasPassword = (UserInfo.Pos(L':') > 0);
      UnicodeString RawUserName = CutToChar(UserInfo, L':', false);
      UserName = DecodeUrlChars(RawUserName);

      SetPassword(DecodeUrlChars(UserInfo));
      if (HasPassword && Password().IsEmpty())
      {
        Password = EmptyString;
      }

      UnicodeString RemoteDirectoryWithSessionParams = Url.SubString(PSlash, Url.Length() - PSlash + 1);
      RemoteDirectory = CutToChar(RemoteDirectoryWithSessionParams, UrlParamSeparator, false);
      UnicodeString SessionParams = RemoteDirectoryWithSessionParams;

      // We should handle session params in "stored session" branch too.
      // And particularly if there's a "save" param, we should actually not try to match the
      // URL against site names
      while (!SessionParams.IsEmpty())
      {
        UnicodeString SessionParam = CutToChar(SessionParams, UrlParamSeparator, false);
        UnicodeString SessionParamName = CutToChar(SessionParam, UrlParamValueSeparator, false);
        if (::SameText(SessionParamName, UrlSaveParamName))
        {
          FSaveOnly = (::StrToIntDef(SessionParam, 1) != 0);
        }
      }

      if (MaskedUrl != nullptr)
      {
        (*MaskedUrl) += RawUserName;
        if (HasPassword)
        {
          (*MaskedUrl) += L":" + UnicodeString(PASSWORD_MASK);
        }
        if (!RawUserName.IsEmpty() || HasPassword)
        {
          (*MaskedUrl) += L"@";
        }
        (*MaskedUrl) += OrigHostInfo + RemoteDirectory;
      }

      if (PSlash <= Url.Length())
      {
        RemoteDirectory = Url.SubString(PSlash, Url.Length() - PSlash + 1);
      }
    }

    if (!RemoteDirectory().IsEmpty() && (RemoteDirectory() != ROOTDIRECTORY))
    {
      if ((RemoteDirectory()[RemoteDirectory().Length()] != L'/') &&
          (AFileName != nullptr))
      {
        *AFileName = DecodeUrlChars(base::UnixExtractFileName(RemoteDirectory));
        RemoteDirectory = base::UnixExtractFilePath(RemoteDirectory);
      }
      SetRemoteDirectory(DecodeUrlChars(RemoteDirectory));
      // Is already true for ad-hoc URL, but we want to error even for "storedsite/path/"-style URL.
      RequireDirectories = true;
    }

    DefaultsOnly = false;
  }
  else
  {
    // This happens when pasting URL on Login dialog
    if (AStoredSessions != nullptr)
    {
      CopyData(AStoredSessions->GetDefaultSettings());
    }

    DefaultsOnly = true;
  }

  if (ProtocolDefined)
  {
    SetFSProtocol(AFSProtocol);
  }

  if (Options != nullptr)
  {
    // we deliberately do keep defaultonly to false, in presence of any option,
    // as the option should not make session "connectable"

    UnicodeString Value;
    if (Options->FindSwitch(USERNAME_SWITCH, Value))
    {
      UserName = Value;
    }
    if (Options->FindSwitch(PASSWORD_SWITCH, Value))
    {
      Password = Value;
    }
    if (Options->FindSwitch(SESSIONNAME_SWITCH, Value))
    {
      Name = Value;
    }
    if (Options->FindSwitch(NEWPASSWORD_SWITCH, Value))
    {
      SetChangePassword(true);
      SetNewPassword(Value);
    }
    if (Options->FindSwitch(PRIVATEKEY_SWITCH, Value))
    {
      SetPublicKeyFile(Value);
    }
    if (Options->FindSwitch(L"clientcert", Value))
    {
      SetTlsCertificateFile(Value);
    }
    if (Options->FindSwitch(PassphraseOption, Value))
    {
      SetPassphrase(Value);
    }
    if (Options->FindSwitch("timeout", Value))
    {
      SetTimeout(nb::ToIntPtr(::StrToInt64(Value)));
    }
    if (Options->FindSwitch("hostkey", Value) ||
        Options->FindSwitch("certificate", Value))
    {
      SetHostKey(Value);
      FOverrideCachedHostKey = true;
    }
    SetFtpPasvMode(Options->SwitchValue("passive", GetFtpPasvMode()));
    if (Options->FindSwitch("implicit"))
    {
      bool Enabled = Options->SwitchValue("implicit", true);
      SetFtps(Enabled ? ftpsImplicit : ftpsNone);
      if (!PortNumberDefined && Enabled)
      {
        SetPortNumber(FtpsImplicitPortNumber);
      }
    }
    // BACKWARD COMPATIBILITY with 5.5.x
    if (Options->FindSwitch("explicitssl", Value))
    {
      bool Enabled = Options->SwitchValue("explicitssl", true);
      SetFtps(Enabled ? ftpsExplicitSsl : ftpsNone);
      if (!PortNumberDefined && Enabled)
      {
        SetPortNumber(FtpPortNumber);
      }
    }
    if (Options->FindSwitch("explicit", Value) ||
        // BACKWARD COMPATIBILITY with 5.5.x
        Options->FindSwitch("explicittls", Value))
    {
      UnicodeString SwitchName =
        Options->FindSwitch(L"explicit", Value) ? "explicit" : "explicittls";
      bool Enabled = Options->SwitchValue(SwitchName, true);
      SetFtps(Enabled ? ftpsExplicitTls : ftpsNone);
      if (!PortNumberDefined && Enabled)
      {
        SetPortNumber(FtpPortNumber);
      }
    }
    if (Options->FindSwitch(RawSettingsOption))
    {
      std::unique_ptr<TStrings> RawSettings(std::make_unique<TStringList>());
      if (Options->FindSwitch(RawSettingsOption, RawSettings.get()))
      {
        ApplyRawSettings(RawSettings.get(), Unsafe);
      }
    }
    if (Options->FindSwitch(PASSWORDSFROMFILES_SWITCH))
    {
      ReadPasswordsFromFiles();
    }
  }

  return true;
}

void TSessionData::ApplyRawSettings(TStrings * RawSettings, bool Unsafe)
{
#if 0
  std::unique_ptr<TOptionsStorage> OptionsStorage(std::make_unique<TOptionsStorage>(RawSettings, false));
  ApplyRawSettings(OptionsStorage.get(), Unsafe, false);
#endif // #if 0
}

void TSessionData::ApplyRawSettings(THierarchicalStorage * Storage, bool Unsafe, bool RespectDisablePasswordStoring)
{
  bool Dummy;
  DoLoad(Storage, false, Dummy, Unsafe, RespectDisablePasswordStoring);
}

void TSessionData::ConfigureTunnel(int32_t APortNumber)
{
  FOrigHostName = GetHostName();
  FOrigPortNumber = GetPortNumber();
  FOrigProxyMethod = GetProxyMethod();

  SetHostName("127.0.0.1");
  SetPortNumber(APortNumber);
  // proxy settings is used for tunnel
  SetProxyMethod(::pmNone);
  FTunnelConfigured = true;
  FLogicalHostName = FOrigHostName;
}

void TSessionData::RollbackTunnel()
{
  if (FTunnelConfigured)
  {
    SetHostName(FOrigHostName);
    SetPortNumber(FOrigPortNumber);
    SetProxyMethod(FOrigProxyMethod);
    SetLogicalHostName(L"");
    FTunnelConfigured = false;
  }
}

TSessionData * TSessionData::CreateTunnelData(int32_t TunnelLocalPortNumber)
{
  std::unique_ptr<TSessionData> TunnelData(std::make_unique<TSessionData>(EmptyStr));
  TunnelData->Assign(StoredSessions->DefaultSettings);
  TunnelData->Name = FMTLOAD(TUNNEL_SESSION_NAME, SessionName);
  TunnelData->FTunnel = false;
  TunnelData->FHostName = FTunnelHostName;
  TunnelData->FPortNumber = FTunnelPortNumber;
  TunnelData->FUserName = FTunnelUserName;
  TunnelData->FPassword = FTunnelPassword;
  TunnelData->FPublicKeyFile = FTunnelPublicKeyFile;
  TunnelData->DetachedCertificate = EmptyStr;
  TunnelData->FPassphrase = FTunnelPassphrase;
  UnicodeString AHostName = HostNameExpanded;
  if (IsIPv6Literal(AHostName))
  {
    AHostName = EscapeIPv6Literal(AHostName);
  }
  TunnelData->FTunnelPortFwd = FORMAT(L"L%d\t%s:%d",
    TunnelLocalPortNumber, AHostName, PortNumber);
  TunnelData->FHostKey = FTunnelHostKey;

  // inherit proxy options on the main session
  TunnelData->FProxyMethod = FProxyMethod;
  TunnelData->FProxyHost = FProxyHost;
  TunnelData->FProxyPort = FProxyPort;
  TunnelData->FProxyUsername = FProxyUsername;
  TunnelData->FProxyPassword = FProxyPassword;
  TunnelData->FProxyTelnetCommand = FProxyTelnetCommand;
  TunnelData->FProxyLocalCommand = FProxyLocalCommand;
  TunnelData->FProxyDNS = FProxyDNS;
  TunnelData->FProxyLocalhost = FProxyLocalhost;

  // inherit most SSH options of the main session (except for private key and bugs)
  TunnelData->Compression = Compression;
  TunnelData->CipherList = CipherList;
  TunnelData->Ssh2DES = Ssh2DES;

  TunnelData->KexList = KexList;
  TunnelData->RekeyData = RekeyData;
  TunnelData->RekeyTime = RekeyTime;

  TunnelData->FSshNoUserAuth = FSshNoUserAuth;
  TunnelData->FAuthGSSAPI = FAuthGSSAPI;
  TunnelData->FAuthGSSAPIKEX = FAuthGSSAPIKEX;
  TunnelData->FGSSAPIFwdTGT = FGSSAPIFwdTGT;
  TunnelData->FTryAgent = FTryAgent;
  TunnelData->FAgentFwd = FAgentFwd;
  TunnelData->FAuthKI = FAuthKI;
  TunnelData->FAuthKIPassword = FAuthKIPassword;
  return TunnelData.release();
}

void TSessionData::ExpandEnvironmentVariables()
{
  SetHostName(GetHostNameExpanded());
  SessionSetUserName(GetUserNameExpanded());
  SetPublicKeyFile(::ExpandEnvironmentVariables(GetPublicKeyFile()));
  DetachedCertificate = ::ExpandEnvironmentVariables(DetachedCertificate);
}

void TSessionData::ValidatePath(const UnicodeString & /*APath*/)
{
  // noop
}

void TSessionData::ValidateName(const UnicodeString & Name)
{
  // keep consistent with MakeValidName
  if (Name.LastDelimiter(L"/") > 0)
  {
    throw Exception(FMTLOAD(ITEM_NAME_INVALID, Name, L"/"));
  }
}

UnicodeString TSessionData::MakeValidName(const UnicodeString & Name)
{
  // keep consistent with ValidateName
  return ReplaceStr(Name, L"/", L"\\");
}

RawByteString TSessionData::EncryptPassword(const UnicodeString & Password, const UnicodeString & Key)
{
  return GetConfiguration()->EncryptPassword(Password, Key);
}

RawByteString TSessionData::StronglyRecryptPassword(const RawByteString & APassword, const UnicodeString & AKey)
{
  return GetConfiguration()->StronglyRecryptPassword(APassword, AKey);
}

UnicodeString TSessionData::DecryptPassword(const RawByteString & APassword, const UnicodeString & AKey)
{
  UnicodeString Result;
  try
  {
    Result = GetConfiguration()->DecryptPassword(APassword, AKey);
  }
  catch (EAbort &)
  {
    // silently ignore aborted prompts for master password and return empty password
  }
  return Result;
}

UnicodeString TSessionData::GetSessionPasswordEncryptionKey() const
{
  return UserName() + HostName();
}

bool TSessionData::GetCanLogin() const
{
  return !FHostName.IsEmpty();
}

bool TSessionData::GetIsLocalBrowser() const
{
  return !LocalDirectory().IsEmpty() && !OtherLocalDirectory().IsEmpty();
}

bool TSessionData::GetCanOpen() const
{
  return GetCanLogin() || IsLocalBrowser();
}

int32_t TSessionData::GetDefaultPort() const
{
  return DefaultPort(FSProtocol, Ftps);
}

UnicodeString TSessionData::GetSessionKey() const
{
  UnicodeString Result = FORMAT("%s@%s", SessionGetUserName(), GetHostName());
  if (GetPortNumber() != GetDefaultPort())
  {
    Result += FORMAT(":%d", GetPortNumber());
  }
  return Result;
}

UnicodeString TSessionData::GetInternalStorageKey() const
{
  // This is probably useless remnant of previous use of this method from OpenSessionInPutty
  // that needs the method to return something even for ad-hoc sessions
  if (GetName().IsEmpty())
  {
    return GetSessionKey();
  }
  return GetName();
}

UnicodeString TSessionData::GetStorageKey() const
{
  return GetSessionName();
}

UnicodeString TSessionData::FormatSiteKey(const UnicodeString & HostName, int32_t PortNumber)
{
  return FORMAT("%s:%d", HostName, nb::ToInt32(PortNumber));
}

UnicodeString TSessionData::GetSiteKey() const
{
  return FormatSiteKey(GetHostNameExpanded(), GetPortNumber());
}

void TSessionData::SetHostName(const UnicodeString & AValue)
{
  if (FHostName != AValue)
  {
    // HostName is key for password encryption
    UnicodeString XPassword = Password;
    UnicodeString XNewPassword = NewPassword;
    UnicodeString XEncryptKey = EncryptKey;

    // This is now hardly used as hostname is parsed directly on login dialog.
    // But can be used when importing sites from PuTTY, as it allows same format too.
    UnicodeString Value = AValue;
    int32_t P = Value.LastDelimiter(L"@");
    if (P > 0)
    {
      SessionSetUserName(Value.SubString(1, P - 1));
      Value = Value.SubString(P + 1, Value.Length() - P);
    }
    FHostName = Value;
    Modify();

    SetPassword(XPassword);
    FNewPassword = XNewPassword;
    EncryptKey = XEncryptKey;
    Shred(XPassword);
    Shred(XNewPassword);
    Shred(XEncryptKey);
  }
}

UnicodeString TSessionData::GetHostNameExpanded() const
{
  return ::ExpandEnvironmentVariables(GetHostName());
}

UnicodeString TSessionData::GetHostNameSource() const
{
  UnicodeString Result;
  if (HostName() != HostNameExpanded())
  {
    Result = HostName;
  }
  return Result;
}

void TSessionData::SetPortNumber(int32_t value)
{
  SET_SESSION_PROPERTY(PortNumber);
}

void TSessionData::SetShell(const UnicodeString & value)
{
  SET_SESSION_PROPERTY(Shell);
}

void TSessionData::SetSftpServer(const UnicodeString & value)
{
  SET_SESSION_PROPERTY(SftpServer);
}

void TSessionData::SetClearAliases(bool value)
{
  SET_SESSION_PROPERTY(ClearAliases);
}

void TSessionData::SetListingCommand(const UnicodeString & value)
{
  SET_SESSION_PROPERTY(ListingCommand);
}

void TSessionData::SetIgnoreLsWarnings(bool value)
{
  SET_SESSION_PROPERTY(IgnoreLsWarnings);
}

void TSessionData::SetUnsetNationalVars(bool value)
{
  SET_SESSION_PROPERTY(UnsetNationalVars);
}

void TSessionData::SessionSetUserName(const UnicodeString & value)
{
  // Avoid password recryption (what may popup master password prompt)
  if (FUserName != value)
  {
    // UserName is key for password encryption
    UnicodeString XPassword = GetPassword();
    UnicodeString XNewPassword = FNewPassword;
    UnicodeString XEncryptKey = EncryptKey;
    SET_SESSION_PROPERTY(UserName);
    SetPassword(XPassword);
    FNewPassword = XNewPassword;
    EncryptKey = XEncryptKey;
    Shred(XPassword);
    Shred(XNewPassword);
    Shred(XEncryptKey);
  }
}

UnicodeString TSessionData::GetUserNameExpanded() const
{
  UnicodeString Result = ::ExpandEnvironmentVariables(UserName);
  if (Result.IsEmpty() && HasS3AutoCredentials())
  {
#if 0
    Result = S3EnvUserName(S3Profile);
#endif //if 0
  }
  return Result;
}

UnicodeString TSessionData::GetUserNameSource() const
{
  UnicodeString Result;
  if (UserName().IsEmpty() && HasS3AutoCredentials())
  {
#if 0
    S3EnvUserName(S3Profile, &Result);
#endif //if 0
  }
  if (Result.IsEmpty() && (UserName() != GetUserNameExpanded()))
  {
    Result = UserName();
  }
  return Result;
}

void TSessionData::SetPassword(const UnicodeString & AValue)
{
  RawByteString value = EncryptPassword(AValue, GetSessionPasswordEncryptionKey());
  SET_SESSION_PROPERTY(Password);
}

UnicodeString TSessionData::GetPassword() const
{
  return DecryptPassword(FPassword, GetSessionPasswordEncryptionKey());
}

void TSessionData::SetNewPassword(const UnicodeString & AValue)
{
  RawByteString value = EncryptPassword(AValue, GetSessionPasswordEncryptionKey());
  SET_SESSION_PROPERTY(NewPassword);
}

UnicodeString TSessionData::GetNewPassword() const
{
  return DecryptPassword(FNewPassword, GetSessionPasswordEncryptionKey());
}

void TSessionData::SetChangePassword(bool value)
{
  SET_SESSION_PROPERTY(ChangePassword);
}

void TSessionData::SetPingInterval(int32_t value)
{
  SET_SESSION_PROPERTY(PingInterval);
}

void TSessionData::SetTryAgent(bool value)
{
  SET_SESSION_PROPERTY(TryAgent);
}

void TSessionData::SetAgentFwd(bool value)
{
  SET_SESSION_PROPERTY(AgentFwd);
}

void TSessionData::SetAuthKI(bool value)
{
  SET_SESSION_PROPERTY(AuthKI);
}

void TSessionData::SetAuthKIPassword(bool value)
{
  SET_SESSION_PROPERTY(AuthKIPassword);
}

void TSessionData::SetAuthGSSAPI(bool value)
{
  SET_SESSION_PROPERTY(AuthGSSAPI);
}

void TSessionData::SetAuthGSSAPIKEX(bool value)
{
  SET_SESSION_PROPERTY(AuthGSSAPIKEX);
}

void TSessionData::SetGSSAPIFwdTGT(bool value)
{
  SET_SESSION_PROPERTY(GSSAPIFwdTGT);
}

void TSessionData::SetChangeUsername(bool value)
{
  SET_SESSION_PROPERTY(ChangeUsername);
}

void TSessionData::SetCompression(bool value)
{
  SET_SESSION_PROPERTY(Compression);
}

void TSessionData::SetSsh2DES(bool value)
{
  SET_SESSION_PROPERTY(Ssh2DES);
}

void TSessionData::SetSshNoUserAuth(bool value)
{
  SET_SESSION_PROPERTY(SshNoUserAuth);
}

bool TSessionData::GetUsesSsh() const
{
  return GetIsSshProtocol(GetFSProtocol());
}

void TSessionData::SetCipher(int32_t Index, TCipher value)
{
  DebugAssert(Index >= 0 && Index < CIPHER_COUNT);
  SET_SESSION_PROPERTY(Ciphers[Index]);
}

TCipher TSessionData::GetCipher(int32_t Index) const
{
  DebugAssert(Index >= 0 && Index < CIPHER_COUNT);
  return FCiphers[Index];
}

template <class AlgoT>
void TSessionData::SetAlgoList(AlgoT * List, const AlgoT * DefaultList, const UnicodeString * Names,
  int32_t Count, AlgoT WarnAlgo, const UnicodeString & AValue)
{
  UnicodeString Value = AValue;

  nb::vector_t<bool> Used(Count); // initialized to false
  nb::vector_t<AlgoT> NewList(Count);

  bool HasWarnAlgo = (WarnAlgo >= AlgoT());
  const AlgoT * WarnPtr;
  int32_t WarnDefaultIndex;
  if (!HasWarnAlgo)
  {
    WarnPtr = nullptr;
    WarnDefaultIndex = -1;
  }
  else
  {
    WarnPtr = std::find(DefaultList, DefaultList + Count, WarnAlgo);
    DebugAssert(WarnPtr != nullptr);
    WarnDefaultIndex = (WarnPtr - DefaultList);
  }

  int32_t Index = 0;
  while (!Value.IsEmpty())
  {
    UnicodeString AlgoStr = CutToChar(Value, L',', true);
    for (int32_t Algo = 0; Algo < Count; ++Algo)
    {
      if (!AlgoStr.CompareIC(Names[Algo]) &&
          !Used[Algo] && DebugAlwaysTrue(Index < Count))
      {
        NewList[Index] = static_cast<AlgoT>(Algo);
        Used[Algo] = true;
        ++Index;
        break;
      }
    }
  }

  if (HasWarnAlgo && !Used[WarnAlgo] && DebugAlwaysTrue(Index < Count))
  {
    NewList[Index] = WarnAlgo;
    Used[WarnAlgo] = true;
    ++Index;
  }

  int32_t WarnIndex = -1;
  if (HasWarnAlgo)
  {
    WarnIndex = std::find(NewList.begin(), NewList.end(), WarnAlgo) - NewList.begin();
  }

  bool Priority = true;
  for (int32_t DefaultIndex = 0; (DefaultIndex < Count); ++DefaultIndex)
  {
    AlgoT DefaultAlgo = DefaultList[DefaultIndex];
    if (!Used[DefaultAlgo] && DebugAlwaysTrue(Index < Count))
    {
      int32_t TargetIndex;
      // Unused algs that are prioritized in the default list,
      // should be merged before the existing custom list
      if (Priority)
      {
        TargetIndex = DefaultIndex;
      }
      else
      {
        if (HasWarnAlgo && (DefaultIndex < WarnDefaultIndex))
        {
          TargetIndex = WarnIndex;
        }
        else
        {
          TargetIndex = Index;
        }
      }

      NewList.insert(NewList.begin() + TargetIndex, DefaultAlgo);
      DebugAssert(NewList.back() == AlgoT());
      NewList.pop_back();

      if (HasWarnAlgo && (TargetIndex <= WarnIndex))
      {
        ++WarnIndex;
      }

      ++Index;
    }
    else
    {
      Priority = false;
    }
  }

  if (!std::equal(NewList.begin(), NewList.end(), List))
  {
    std::copy(NewList.begin(), NewList.end(), List);
    Modify();
  }
}

void TSessionData::SetCipherList(const UnicodeString & Value)
{
  SetAlgoList(FCiphers, DefaultCipherList, CipherNames, CIPHER_COUNT, cipWarn, Value);
}

UnicodeString TSessionData::GetCipherList() const
{
  UnicodeString Result;
  for (int32_t Index = 0; Index < CIPHER_COUNT; ++Index)
  {
    Result += UnicodeString(Index ? "," : "") + CipherNames[GetCipher(Index)];
  }
  return Result;
}

void TSessionData::SetKex(int32_t Index, TKex value)
{
  DebugAssert(Index >= 0 && Index < KEX_COUNT);
  SET_SESSION_PROPERTY(Kex[Index]);
}

TKex TSessionData::GetKex(int32_t Index) const
{
  DebugAssert(Index >= 0 && Index < KEX_COUNT);
  return FKex[Index];
}

void TSessionData::SetKexList(const UnicodeString & Value)
{
  SetAlgoList(FKex, DefaultKexList, KexNames, KEX_COUNT, kexWarn, Value);
}

UnicodeString TSessionData::GetKexList() const
{
  UnicodeString Result;
  for (int32_t Index = 0; Index < KEX_COUNT; ++Index)
  {
    Result += UnicodeString(Index ? "," : "") + KexNames[GetKex(Index)];
  }
  return Result;
}

void TSessionData::SetHostKeys(int32_t Index, THostKey value)
{
  DebugAssert(Index >= 0 && Index < HOSTKEY_COUNT);
  SET_SESSION_PROPERTY(HostKeys[Index]);
}

THostKey TSessionData::GetHostKeys(int32_t Index) const
{
  DebugAssert(Index >= 0 && Index < HOSTKEY_COUNT);
  return FHostKeys[Index];
}

void TSessionData::SetHostKeyList(const UnicodeString & Value)
{
  SetAlgoList(FHostKeys, DefaultHostKeyList, HostKeyNames, HOSTKEY_COUNT, hkWarn, Value);
}

UnicodeString TSessionData::GetHostKeyList() const
{
  UnicodeString Result;
  for (int32_t Index = 0; Index < HOSTKEY_COUNT; Index++)
  {
    Result += UnicodeString(Index ? "," : "") + HostKeyNames[FHostKeys[Index]];
  }
  return Result;
}

void TSessionData::SetGssLib(int32_t Index, TGssLib Value)
{
  DebugAssert(Index >= 0 && Index < GSSLIB_COUNT);
  // SET_SESSION_PROPERTY(FGssLib[Index]);
  if (FGssLib[Index] != Value)
  {
    FGssLib[Index] = Value;
  }
  Modify();
}

TGssLib TSessionData::GetGssLib(int32_t Index) const
{
  DebugAssert(Index >= 0 && Index < GSSLIB_COUNT);
  return FGssLib[Index];
}

void TSessionData::SetGssLibList(const UnicodeString & Value)
{
  SetAlgoList(FGssLib, DefaultGssLibList, GssLibNames, GSSLIB_COUNT, TGssLib(-1), Value);
}

UnicodeString TSessionData::GetGssLibList() const
{
  UnicodeString Result;
  for (int32_t Index = 0; Index < GSSLIB_COUNT; Index++)
  {
    Result += UnicodeString(Index ? "," : "") + GssLibNames[GetGssLib(Index)];
  }
  return Result;
}

void TSessionData::SetGssLibCustom(const UnicodeString & value)
{
  SET_SESSION_PROPERTY(GssLibCustom);
}

void TSessionData::SetPublicKeyFile(const UnicodeString & Value)
{
  if (FPublicKeyFile != Value)
  {
    // PublicKeyFile is key for Passphrase encryption
    UnicodeString XPassphrase = GetPassphrase();

    // StripPathQuotes should not be needed as we do not feed quotes anymore
    FPublicKeyFile = StripPathQuotes(Value);
    Modify();

    SetPassphrase(XPassphrase);
    Shred(XPassphrase);
  }
}

void TSessionData::SetDetachedCertificate(const UnicodeString & value)
{
  SET_SESSION_PROPERTY(DetachedCertificate);
}

UnicodeString TSessionData::ResolvePublicKeyFile()
{
  UnicodeString Result = PublicKeyFile;
  if (Result.IsEmpty())
  {
    Result = GetConfiguration()->DefaultKeyFile;
  }
  // StripPathQuotes should not be needed as we do not feed quotes anymore
  Result = StripPathQuotes(::ExpandEnvironmentVariables(Result));
  return Result;
}

void TSessionData::SetPassphrase(const UnicodeString & AValue)
{
  RawByteString value = EncryptPassword(AValue, GetPublicKeyFile());
  SET_SESSION_PROPERTY(Passphrase);
}

UnicodeString TSessionData::GetPassphrase() const
{
  return DecryptPassword(FPassphrase, GetPublicKeyFile());
}

void TSessionData::SetReturnVar(const UnicodeString & value)
{
  SET_SESSION_PROPERTY(ReturnVar);
}

void TSessionData::SetExitCode1IsError(bool value)
{
  SET_SESSION_PROPERTY(ExitCode1IsError);
}

void TSessionData::SetLookupUserGroups(TAutoSwitch value)
{
  SET_SESSION_PROPERTY(LookupUserGroups);
}

void TSessionData::SetEOLType(TEOLType value)
{
  SET_SESSION_PROPERTY(EOLType);
}

void TSessionData::SetTrimVMSVersions(bool value)
{
  SET_SESSION_PROPERTY(TrimVMSVersions);
}

void TSessionData::SetVMSAllRevisions(bool value)
{
  SET_SESSION_PROPERTY(VMSAllRevisions);
}

TDateTime TSessionData::GetTimeoutDT() const
{
  return SecToDateTime(GetTimeout());
}

void TSessionData::SetTimeout(int32_t value)
{
  SET_SESSION_PROPERTY(Timeout);
}

void TSessionData::SetFSProtocol(const TFSProtocol & value)
{
  SET_SESSION_PROPERTY(FSProtocol);
}

UnicodeString TSessionData::GetFSProtocolStr() const
{
  UnicodeString Result;
  DebugAssert(GetFSProtocol() >= 0);
  if (GetFSProtocol() < FSPROTOCOL_COUNT)
  {
    Result = FSProtocolNames[GetFSProtocol()];
  }
  // DebugAssert(!Result.IsEmpty());
  if (Result.IsEmpty())
    Result = FSProtocolNames[CONST_DEFAULT_PROTOCOL];
  return Result;
}

void TSessionData::SetDetectReturnVar(bool value)
{
  if (value != GetDetectReturnVar())
  {
    SetReturnVar(value ? "" : "$?");
  }
}

bool TSessionData::GetDetectReturnVar() const
{
  return GetReturnVar().IsEmpty();
}

void TSessionData::SetDefaultShell(bool Value)
{
  if (Value != GetDefaultShell())
  {
    SetShell(Value ? "" : "/bin/bash");
  }
}

bool TSessionData::GetDefaultShell() const
{
  return GetShell().IsEmpty();
}

void TSessionData::SetPuttyProtocol(const UnicodeString & value)
{
  SET_SESSION_PROPERTY(PuttyProtocol);
}

UnicodeString TSessionData::GetNormalizedPuttyProtocol() const
{
  return DefaultStr(GetPuttyProtocol(), PuttySshProtocol);
}

void TSessionData::SetPingIntervalDT(TDateTime Value)
{
  uint16_t hour, min, sec, msec;

  Value.DecodeTime(hour, min, sec, msec);
  SetPingInterval(hour * SecsPerHour + min * SecsPerMin + sec);
}

TDateTime TSessionData::GetPingIntervalDT() const
{
  return SecToDateTime(GetPingInterval());
}

void TSessionData::SetPingType(TPingType value)
{
  SET_SESSION_PROPERTY(PingType);
}

void TSessionData::SetAddressFamily(const TAddressFamily & value)
{
  SET_SESSION_PROPERTY(AddressFamily);
}

void TSessionData::SetRekeyData(const UnicodeString & value)
{
  SET_SESSION_PROPERTY(RekeyData);
}

void TSessionData::SetRekeyTime(uint32_t value)
{
  SET_SESSION_PROPERTY(RekeyTime);
}

UnicodeString TSessionData::GetDefaultSessionName() const
{
  UnicodeString Result;
  UnicodeString HostName = ::TrimLeft(GetHostName());
  UnicodeString UserName = SessionGetUserName();
  if (IsLocalBrowser)
  {
    // See also TScpCommanderForm::GetLocalBrowserSessionTitle
    UnicodeString Path1 = base::ExtractShortName(LocalDirectory, false);
    UnicodeString Path2 = base::ExtractShortName(OtherLocalDirectory, false);
    Result = Path1 + TitleSeparator + Path2;
  }
  else if (!HostName.IsEmpty() && !UserName.IsEmpty())
  {
    // If we ever choose to include port number,
    // we have to escape IPv6 literals in HostName
    Result = FORMAT("%s@%s", UserName, HostName);
  }
  else if (!HostName.IsEmpty())
  {
    Result = HostName;
  }
  else
  {
    Result = "session";
  }
  Result = MakeValidName(Result);
  return Result;
}

UnicodeString TSessionData::GetNameWithoutHiddenPrefix() const
{
  UnicodeString Result = GetName();
  if (GetHidden())
  {
    Result = Result.SubString(TNamedObjectList::HiddenPrefix.Length() + 1, Result.Length() - TNamedObjectList::HiddenPrefix.Length());
  }
  return Result;
}

bool TSessionData::HasSessionName() const
{
  return (!GetNameWithoutHiddenPrefix().IsEmpty() && (Name != DefaultName));
}

UnicodeString TSessionData::GetSessionName() const
{
  UnicodeString Result;
  if (HasSessionName())
  {
    Result = GetNameWithoutHiddenPrefix();
  }
  else
  {
    Result = GetDefaultSessionName();
  }
  return Result;
}

bool TSessionData::GetIsSecure() const
{
  bool Result = false;
  switch (GetFSProtocol())
  {
    case fsSCPonly:
    case fsSFTP:
    case fsSFTPonly:
      Result = true;
      break;

    case fsFTP:
    case fsWebDAV:
    case fsS3:
      Result = (GetFtps() != ftpsNone);
      break;

    default:
      DebugFail();
      break;
  }
  return Result;
}

UnicodeString TSessionData::GetProtocolUrl(bool HttpForWebDAV) const
{
  UnicodeString Url;
  switch (GetFSProtocol())
  {
    case fsSCPonly:
      Url = ScpProtocol;
      break;

    default:
      DebugFail();
      // fallback
    case fsSFTP:
    case fsSFTPonly:
      Url = SftpProtocol;
      break;

    case fsFTP:
      if (GetFtps() == ftpsImplicit)
      {
        Url = FtpsProtocol;
      }
      else if ((GetFtps() == ftpsExplicitTls) || (GetFtps() == ftpsExplicitSsl))
      {
        Url = FtpesProtocol;
      }
      else
      {
      Url = FtpProtocol;
      }
      break;

    case fsWebDAV:
      if (HttpForWebDAV)
      {
        if (Ftps == ftpsImplicit)
        {
          Url = HttpsProtocol;
        }
        else
        {
          Url = HttpProtocol;
        }
      }
      else
      {
        if (Ftps == ftpsImplicit)
        {
          Url = WebDAVSProtocol;
        }
        else
        {
          Url = WebDAVProtocol;
        }
      }
      break;

    case fsS3:
      if (Ftps == ftpsImplicit)
      {
        Url = S3Protocol;
      }
      else
      {
        Url = S3PlainProtocol;
      }
      break;
  }

  Url += ProtocolSeparator;

  return Url;
}

static bool HasIP6LiteralBrackets(const UnicodeString & HostName)
{
  return
    (HostName.Length() >= 2) &&
    (HostName[1] == L'[') &&
    (HostName[HostName.Length()] == L']');
}

static UnicodeString StripIP6LiteralBrackets(const UnicodeString & HostName)
{
  UnicodeString Result = HostName;
  if (DebugAlwaysTrue(HasIP6LiteralBrackets(Result)))
  {
    Result = Result.SubString(2, Result.Length() - 2);
  }
  return Result;
}

bool IsIPv6Literal(const UnicodeString & HostName)
{
  UnicodeString Buf = HostName;
  if (HasIP6LiteralBrackets(Buf))
  {
    Buf = StripIP6LiteralBrackets(Buf);
  }
  int32_t Colons = 0;
  bool Result = true;
  for (int32_t Index = 1; Result && (Index <= Buf.Length()); Index++)
  {
    wchar_t C = Buf[Index];
    if (C == L'%')
    {
      break;
    }
    else if (C == L':')
    {
      Colons++;
    }
    else
    {
      Result = IsHex(C);
    }
  }
  Result = Result && (Colons >= 2);
  return Result;
}

UnicodeString EscapeIPv6Literal(const UnicodeString & IP)
{
  UnicodeString Result = IP;
  if (!HasIP6LiteralBrackets(Result))
  {
    Result = L"[" + IP + L"]";
  }
  return Result;
}

TStrings * TSessionData::GetRawSettingsForUrl()
{
  std::unique_ptr<TSessionData> FactoryDefaults(std::make_unique<TSessionData>(""));
  std::unique_ptr<TSessionData> SessionData(Clone());
  SessionData->FSProtocol = FactoryDefaults->FSProtocol;
  SessionData->HostName = FactoryDefaults->HostName;
  SessionData->PortNumber = FactoryDefaults->PortNumber;
  SessionData->UserName = FactoryDefaults->UserName;
  SessionData->Password = FactoryDefaults->Password;
  SessionData->Ftps = FactoryDefaults->Ftps;
  SessionData->HostKey = FactoryDefaults->HostKey;
  SessionData->CopyNonCoreData(FactoryDefaults.get());
  // Cannot be decided in SaveToOptions as it does not have HostName and UserName, so it cannot calculate DefaultSessionName.
  bool SaveName = HasSessionName() && (Name != DefaultSessionName);
  return SessionData->SaveToOptions(FactoryDefaults.get(), SaveName, false);
}

bool TSessionData::HasRawSettingsForUrl()
{
  std::unique_ptr<TStrings> RawSettings(GetRawSettingsForUrl());
  return RawSettings.get() && (RawSettings->Count > 0);
}

UnicodeString TSessionData::GenerateSessionUrl(uint32_t Flags) const
{
  UnicodeString Url;

  if (FLAGSET(Flags, sufSpecific))
  {
    Url += WinSCPProtocolPrefix;
  }

  Url += GetProtocolUrl(FLAGSET(Flags, sufHttpForWebDAV));

  // Add username only if it was somehow explicitly specified (so not with S3CredentialsEnv), but if it was, add it in the expanded form.
  // For scripting, we might use unexpanded form (keeping the environment variables),
  // but for consistency with code generation (where explicit expansion code would need to be added), we do not.
  if (FLAGSET(Flags, sufUserName) && !GetUserNameExpanded().IsEmpty())
  {
    Url += EncodeUrlString(GetUserNameExpanded());

    if (FLAGSET(Flags, sufPassword) && !GetPassword().IsEmpty())
    {
      Url += L":" + EncodeUrlString(NormalizeString(Password));
    }

    if (FLAGSET(Flags, sufHostKey) && !GetHostKey().IsEmpty())
    {
      UnicodeString KeyName;
      UnicodeString Fingerprint = HostKey;
      NormalizeFingerprint(Fingerprint, KeyName);
      UnicodeString S = Fingerprint;
      if (!KeyName.IsEmpty())
      {
        S = KeyName + NormalizedFingerprintSeparator + S;
      }
      S = Base64ToUrlSafe(S); // Noop for MD5 (both in SSH host keys and TLS/SSL)
      S = MD5ToUrlSafe(S); // TLS/SSL fingerprints
      UnicodeString S2 = EncodeUrlString(S);
      DebugAssert(S2 == S2); // There should be nothing left for encoding

      Url +=
        UnicodeString(UrlParamSeparator) + UrlHostKeyParamName +
        UnicodeString(UrlParamValueSeparator) + S2;
    }

#if 0
    if (FLAGSET(Flags, sufRawSettings))
    {
      std::unique_ptr<TStrings> RawSettings(GetRawSettingsForUrl());
      for (int32_t Index = 0; Index < RawSettings->Count; Index++)
      {
        Url +=
          UnicodeString(UrlParamSeparator) +
          UrlRawSettingsParamNamePrefix + EncodeUrlString(LowerCase(RawSettings->Names[Index])) +
          UnicodeString(UrlParamValueSeparator) + EncodeUrlString(RawSettings->ValueFromIndex[Index]);
      }
    }
#endif // #if 0

    Url += "@";
  }

  UnicodeString HostNameExpanded = GetHostNameExpanded();
  DebugAssert(!HostNameExpanded.IsEmpty());
  if (IsIPv6Literal(HostNameExpanded))
  {
    Url += EscapeIPv6Literal(HostNameExpanded);
  }
  else
  {
    Url += EncodeUrlString(HostNameExpanded);
  }

  if (PortNumber != GetDefaultPort())
  {
    Url += L":" + IntToStr(PortNumber);
  }
  Url += L"/";

  return Url;
}

__removed UnicodeString ScriptCommandOpenLink = ScriptCommandLink("open");

void TSessionData::AddSwitch(
  UnicodeString & Result, const UnicodeString & Name, bool Rtf)
{
#if 0
  Result += RtfSwitch(Name, ScriptCommandOpenLink, Rtf);
#endif // #if 0
}

void TSessionData::AddSwitch(
  UnicodeString & Result, const UnicodeString & AName, const UnicodeString & Value, bool Rtf)
{
#if 0
  Result += RtfSwitch(Name, ScriptCommandOpenLink, Value, Rtf);
#endif // #if 0
}

void TSessionData::AddSwitch(
  UnicodeString & Result, const UnicodeString & AName, int32_t Value, bool Rtf)
{
#if 0
  Result += RtfSwitch(Name, ScriptCommandOpenLink, Value, Rtf);
#endif // #if 0
}

void TSessionData::LookupLastFingerprint()
{
  UnicodeString FingerprintType;
  if (GetIsSshProtocol(GetFSProtocol()))
  {
    FingerprintType = SshFingerprintType;
  }
  else if (GetFtps() != ftpsNone)
  {
    FingerprintType = TlsFingerprintType;
  }

  if (!FingerprintType.IsEmpty())
  {
    SetHostKey(GetConfiguration()->GetLastFingerprint(GetSiteKey(), FingerprintType));
  }

  if (FTunnel)
  {
    // not used anyway
    int32_t TunnelPortNumber = std::max(FTunnelLocalPortNumber, GetConfiguration()->FTunnelLocalPortNumberLow);
    std::unique_ptr<TSessionData> TunnelData(CreateTunnelData(TunnelPortNumber));
    FTunnelHostKey = GetConfiguration()->GetLastFingerprint(TunnelData->GetSiteKey(), SshFingerprintType);
  }
}

UnicodeString TSessionData::GenerateOpenCommandArgs(bool /*Rtf*/) const
{
  UnicodeString Result;
#if 0
  std::unique_ptr<TSessionData> FactoryDefaults(std::make_unique<TSessionData>(""));
  std::unique_ptr<TSessionData> SessionData(std::make_unique<TSessionData>(""));

  SessionData->Assign(this);

  UnicodeString Result = SessionData->GenerateSessionUrl(sufOpen);

  // Before we reset the FSProtocol
  bool AUsesSsh = SessionData->GetUsesSsh();
  // SFTP-only is not reflected by the protocol prefix, we have to use rawsettings for that
  if (SessionData->GetFSProtocol() != fsSFTPonly)
  {
    SessionData->SetFSProtocol(FactoryDefaults->GetFSProtocol());
  }
  SessionData->SetHostName(FactoryDefaults->GetHostName());
  SessionData->SetPortNumber(FactoryDefaults->GetPortNumber());
  SessionData->SessionSetUserName(FactoryDefaults->SessionGetUserName());
  SessionData->SetPassword(FactoryDefaults->GetPassword());
  SessionData->CopyNonCoreData(FactoryDefaults.get());
  SessionData->SetFtps(FactoryDefaults->GetFtps());

  if (SessionData->GetHostKey() != FactoryDefaults->GetHostKey())
  {
    UnicodeString SwitchName = AUsesSsh ? L"hostkey" : L"certificate";
    AddSwitch(Result, SwitchName, SessionData->GetHostKey(), Rtf);
    SessionData->SetHostKey(FactoryDefaults->GetHostKey());
  }
  if (SessionData->GetPublicKeyFile() != FactoryDefaults->GetPublicKeyFile())
  {
    AddSwitch(Result, PRIVATEKEY_SWITCH, SessionData->GetPublicKeyFile(), Rtf);
    SessionData->SetPublicKeyFile(FactoryDefaults->GetPublicKeyFile());
  }
  if (SessionData->GetTlsCertificateFile() != FactoryDefaults->GetTlsCertificateFile())
  {
    AddSwitch(Result, L"clientcert", SessionData->GetTlsCertificateFile(), Rtf);
    SessionData->SetTlsCertificateFile(FactoryDefaults->GetTlsCertificateFile());
  }
  if (SessionData->GetPassphrase() != FactoryDefaults->GetPassphrase())
  {
    AddSwitch(Result, PassphraseOption, SessionData->GetPassphrase(), Rtf);
    SessionData->SetPassphrase(FactoryDefaults->GetPassphrase());
  }
  if (SessionData->GetFtpPasvMode() != FactoryDefaults->GetFtpPasvMode())
  {
    AddSwitch(Result, L"passive", SessionData->GetFtpPasvMode() ? 1 : 0, Rtf);
    SessionData->SetFtpPasvMode(FactoryDefaults->GetFtpPasvMode());
  }
  if (SessionData->GetTimeout() != FactoryDefaults->GetTimeout())
  {
    AddSwitch(Result, L"timeout", SessionData->GetTimeout(), Rtf);
    SessionData->SetTimeout(FactoryDefaults->GetTimeout());
  }

  std::unique_ptr<TStrings> RawSettings(SessionData->SaveToOptions(FactoryDefaults.get(), false, false));

  if (RawSettings->GetCount() > 0)
  {
    TODO("implement");
#if 0
    AddSwitch(Result, RawSettingsOption, FRtf);

    Result += StringsToParams(RawSettings.get());
#endif
  }
#endif // #if 0

  return Result;
}
#if 0

UnicodeString SessionOptionsClassName(L"SessionOptions");

void TSessionData::AddAssemblyProperty(
  UnicodeString & Result, TAssemblyLanguage Language,
  const UnicodeString & Name, const UnicodeString & Type,
  const UnicodeString & Member)
{
  Result += AssemblyProperty(Language, SessionOptionsClassName, Name, Type, Member, false);
}

void TSessionData::AddAssemblyProperty(
  UnicodeString & Result, TAssemblyLanguage Language,
  const UnicodeString & Name, const UnicodeString & Value)
{
  Result += AssemblyProperty(Language, SessionOptionsClassName, Name, Value, false);
}

void TSessionData::AddAssemblyProperty(
  UnicodeString & Result, TAssemblyLanguage Language,
  const UnicodeString & Name, int Value)
{
  Result += AssemblyProperty(Language, SessionOptionsClassName, Name, Value, false);
}

void TSessionData::AddAssemblyProperty(
  UnicodeString & Result, TAssemblyLanguage Language,
  const UnicodeString & Name, bool Value)
{
  Result += AssemblyProperty(Language, SessionOptionsClassName, Name, Value, false);
}

void TSessionData::GenerateAssemblyCode(
  TAssemblyLanguage Language, UnicodeString & Head, UnicodeString & Tail, int & Indent)
{
  std::unique_ptr<TSessionData> FactoryDefaults(std::make_unique<TSessionData>(L""));
  std::unique_ptr<TSessionData> SessionData(Clone());

  switch (Language)
  {
    case alCSharp:
    case alVBNET:
      // noop
      break;

    case alPowerShell:
      Head +=
        AssemblyCommentLine(Language, LoadStr(CODE_PS_ADD_TYPE)) +
        RtfKeyword(L"Add-Type") + RtfText(" -Path ") + AssemblyString(Language, "WinSCPnet.dll") + RtfPara +
        RtfPara;
      break;

    default:
      DebugFail();
      break;
  }

  Head +=
    AssemblyCommentLine(Language, LoadStr(CODE_SESSION_OPTIONS)) +
    AssemblyNewClassInstanceStart(Language, SessionOptionsClassName, false);

  UnicodeString ProtocolMember;
  switch (SessionData->FSProtocol)
  {
    case fsSCPonly:
      ProtocolMember = "Scp";
      break;

    default:
      DebugFail();
      // fallback
    case fsSFTP:
    case fsSFTPonly:
      ProtocolMember = "Sftp";
      break;

    case fsFTP:
      ProtocolMember = "Ftp";
      break;

    case fsWebDAV:
      ProtocolMember = "Webdav";
      break;

    case fsS3:
      ProtocolMember = "S3";
      break;
  }

  // Before we reset the FSProtocol
  bool AUsesSsh = SessionData->UsesSsh;

  // Protocol is set unconditionally, we want even the default SFTP
  AddAssemblyProperty(Head, Language, L"Protocol", L"Protocol", ProtocolMember);
  // SFTP-only is not reflected by the protocol prefix, we have to use rawsettings for that
  if (SessionData->FSProtocol != fsSFTPonly)
  {
    SessionData->FSProtocol = FactoryDefaults->FSProtocol;
  }
  if (SessionData->HostName != FactoryDefaults->HostName)
  {
    AddAssemblyProperty(Head, Language, L"HostName", HostName);
    SessionData->HostName = FactoryDefaults->HostName;
  }
  int ADefaultPort = DefaultPort(FSProtocol, Ftps);
  if (SessionData->PortNumber != ADefaultPort)
  {
    AddAssemblyProperty(Head, Language, L"PortNumber", PortNumber);
  }
  SessionData->PortNumber = FactoryDefaults->PortNumber;
  if (SessionData->UserName != FactoryDefaults->UserName)
  {
    AddAssemblyProperty(Head, Language, L"UserName", UserName);
    SessionData->UserName = FactoryDefaults->UserName;
  }
  if (SessionData->Password != FactoryDefaults->Password)
  {
    AddAssemblyProperty(Head, Language, L"Password", NormalizeString(Password));
    SessionData->Password = FactoryDefaults->Password;
  }

  SessionData->CopyNonCoreData(FactoryDefaults.get());

  TFtps DefaultFtps = FactoryDefaults->Ftps;
  if (FSProtocol == fsS3)
  {
    DefaultFtps = ftpsImplicit;
  }

  if (SessionData->Ftps != DefaultFtps)
  {
    // SessionData->FSProtocol is reset already
    switch (FSProtocol)
    {
      case fsFTP:
        {
          UnicodeString FtpSecureMember;
          switch (SessionData->Ftps)
          {
            case ftpsNone:
              DebugFail();
              FtpSecureMember = L"None";
              break;

            case ftpsImplicit:
              FtpSecureMember = L"Implicit";
              break;

            case ftpsExplicitTls:
            case ftpsExplicitSsl:
              FtpSecureMember = L"Explicit";
              break;

            default:
              DebugFail();
              break;
          }
          AddAssemblyProperty(Head, Language, L"FtpSecure", L"FtpSecure", FtpSecureMember);
        }
        break;

      case fsWebDAV:
      case fsS3:
        AddAssemblyProperty(Head, Language, L"Secure", (SessionData->Ftps != ftpsNone));
        break;

      default:
        DebugFail();
        break;
    }
  }
  SessionData->Ftps = FactoryDefaults->Ftps;

  if (SessionData->HostKey != FactoryDefaults->HostKey)
  {
    UnicodeString PropertyName = AUsesSsh ? L"SshHostKeyFingerprint" : L"TlsHostCertificateFingerprint";
    AddAssemblyProperty(Head, Language, PropertyName, SessionData->HostKey);
    SessionData->HostKey = FactoryDefaults->HostKey;
  }
  if (SessionData->PublicKeyFile != FactoryDefaults->PublicKeyFile)
  {
    AddAssemblyProperty(Head, Language, L"SshPrivateKeyPath", SessionData->PublicKeyFile);
    SessionData->PublicKeyFile = FactoryDefaults->PublicKeyFile;
  }
  if (SessionData->TlsCertificateFile != FactoryDefaults->TlsCertificateFile)
  {
    AddAssemblyProperty(Head, Language, L"TlsClientCertificatePath", SessionData->TlsCertificateFile);
    SessionData->TlsCertificateFile = FactoryDefaults->TlsCertificateFile;
  }
  if (SessionData->Passphrase != FactoryDefaults->Passphrase)
  {
    AddAssemblyProperty(Head, Language, L"PrivateKeyPassphrase", SessionData->Passphrase);
    SessionData->Passphrase = FactoryDefaults->Passphrase;
  }
  if (SessionData->FtpPasvMode != FactoryDefaults->FtpPasvMode)
  {
    AddAssemblyProperty(Head, Language, L"FtpMode", L"FtpMode", (SessionData->FtpPasvMode ? L"Passive" : L"Active"));
    SessionData->FtpPasvMode = FactoryDefaults->FtpPasvMode;
  }
  if (SessionData->Timeout != FactoryDefaults->Timeout)
  {
    AddAssemblyProperty(Head, Language, L"TimeoutInMilliseconds", SessionData->Timeout * 1000);
    SessionData->Timeout = FactoryDefaults->Timeout;
  }

  Head += AssemblyNewClassInstanceEnd(Language, false);

  std::unique_ptr<TStrings> RawSettings(SessionData->SaveToOptions(FactoryDefaults.get(), false, false));

  UnicodeString SessionOptionsVariableName = AssemblyVariableName(Language, SessionOptionsClassName);

  if (RawSettings->Count > 0)
  {
    Head +=
      RtfPara +
      AssemblyAddRawSettings(Language, RawSettings.get(), SessionOptionsClassName, L"AddRawSettings");
  }

  Head += RtfPara;

  UnicodeString Indentation = L"    ";
  UnicodeString SessionVariableName = AssemblyVariableName(Language, SessionClassName);
  UnicodeString RtfSessionClass = RtfLibraryClass(SessionClassName);
  UnicodeString RtfSessionOpenMethod = RtfLibraryMethod(SessionClassName, L"Open", false);

  UnicodeString NewSessionInstance = AssemblyNewClassInstance(Language, SessionClassName, false);
  UnicodeString OpenCall =
    Indentation + AssemblyCommentLine(Language, LoadStr(CODE_CONNECT)) +
    Indentation + RtfText(SessionVariableName + L".") + RtfSessionOpenMethod + RtfText(L"(" + SessionOptionsVariableName + L")") +
      AssemblyStatementSeparator(Language) + RtfPara;

  switch (Language)
  {
    case alCSharp:
      Head +=
        RtfKeyword(L"using") + RtfText(" (") + NewSessionInstance + RtfText(L"())") + RtfPara +
        RtfText(L"{") + RtfPara +
        OpenCall;

      Tail =
        RtfText(L"}") + RtfPara;
      break;

    case alVBNET:
      Head +=
        RtfKeyword(L"Using") + RtfText(L" ") + NewSessionInstance + RtfPara +
        OpenCall;

      Tail =
        RtfKeyword(L"End Using") + RtfPara;
      break;

    case alPowerShell:
      Head +=
        NewSessionInstance + RtfPara +
        RtfPara +
        RtfKeyword(L"try") + RtfPara +
        RtfText(L"{") + RtfPara +
        OpenCall;

      Tail =
        RtfText(L"}") + RtfPara +
        RtfKeyword(L"finally") + RtfPara +
        RtfText(L"{") + RtfPara +
        RtfText(Indentation + SessionVariableName + L".") +
          RtfLibraryMethod(SessionClassName, L"Dispose", false) + RtfText(L"()") + RtfPara +
        RtfText(L"}") + RtfPara;
      break;
  }

  Head += RtfPara;

  Indent = 4; // the same for all languages so far
}
#endif // #if 0

void TSessionData::SetTimeDifference(TDateTime value)
{
  SET_SESSION_PROPERTY(TimeDifference);
}

void TSessionData::SetTimeDifferenceAuto(bool value)
{
  SET_SESSION_PROPERTY(TimeDifferenceAuto);
}

void TSessionData::SetLocalDirectory(const UnicodeString & value)
{
  SET_SESSION_PROPERTY(LocalDirectory);
}

void TSessionData::SetOtherLocalDirectory(const UnicodeString & value)
{
  SET_SESSION_PROPERTY(OtherLocalDirectory);
}

UnicodeString TSessionData::GetLocalDirectoryExpanded() const
{
  return ExpandFileName(::ExpandEnvironmentVariables(LocalDirectory()));
}

void TSessionData::SetRemoteDirectory(const UnicodeString & value)
{
  SET_SESSION_PROPERTY(RemoteDirectory);
}

void TSessionData::SetSynchronizeBrowsing(bool value)
{
  SET_SESSION_PROPERTY(SynchronizeBrowsing);
}

void TSessionData::SetUpdateDirectories(bool value)
{
  SET_SESSION_PROPERTY(UpdateDirectories);
}

void TSessionData::SetCacheDirectories(bool value)
{
  SET_SESSION_PROPERTY(CacheDirectories);
}

void TSessionData::SetCacheDirectoryChanges(bool value)
{
  SET_SESSION_PROPERTY(CacheDirectoryChanges);
}

void TSessionData::SetPreserveDirectoryChanges(bool value)
{
  SET_SESSION_PROPERTY(PreserveDirectoryChanges);
}

void TSessionData::SetResolveSymlinks(bool value)
{
  SET_SESSION_PROPERTY(ResolveSymlinks);
}

void TSessionData::SetFollowDirectorySymlinks(bool value)
{
  SET_SESSION_PROPERTY(FollowDirectorySymlinks);
}

void TSessionData::SetDSTMode(TDSTMode value)
{
  SET_SESSION_PROPERTY(DSTMode);
}

void TSessionData::SetDeleteToRecycleBin(bool value)
{
  SET_SESSION_PROPERTY(DeleteToRecycleBin);
}

void TSessionData::SetOverwrittenToRecycleBin(bool value)
{
  SET_SESSION_PROPERTY(OverwrittenToRecycleBin);
}

void TSessionData::SetRecycleBinPath(const UnicodeString & value)
{
  SET_SESSION_PROPERTY(RecycleBinPath);
}

void TSessionData::SetPostLoginCommands(const UnicodeString & value)
{
  SET_SESSION_PROPERTY(PostLoginCommands);
}

void TSessionData::SetSpecial(bool value)
{
  SET_SESSION_PROPERTY(Special);
}

void TSessionData::SetScp1Compatibility(bool value)
{
  SET_SESSION_PROPERTY(Scp1Compatibility);
}

void TSessionData::SetTcpNoDelay(bool value)
{
  SET_SESSION_PROPERTY(TcpNoDelay);
}

void TSessionData::SetSendBuf(int value)
{
  SET_SESSION_PROPERTY(SendBuf);
}

void TSessionData::SetSourceAddress(const UnicodeString & value)
{
  SET_SESSION_PROPERTY(SourceAddress);
}

void TSessionData::SetProtocolFeatures(const UnicodeString & value)
{
  SET_SESSION_PROPERTY(ProtocolFeatures);
}

void TSessionData::SetSshSimple(bool value)
{
  SET_SESSION_PROPERTY(SshSimple);
}

void TSessionData::SetProxyMethod(TProxyMethod value)
{
//  TProxyMethod Value = value == pmSystemOld ? pmSystem : value;
//  nb::used(Value);
  SET_SESSION_PROPERTY(ProxyMethod);
}

void TSessionData::SetProxyHost(const UnicodeString & value)
{
  SET_SESSION_PROPERTY(ProxyHost);
}

void TSessionData::SetProxyPort(int32_t value)
{
  SET_SESSION_PROPERTY(ProxyPort);
}

void TSessionData::SetProxyUsername(const UnicodeString & value)
{
  SET_SESSION_PROPERTY(ProxyUsername);
}

void TSessionData::SetProxyPassword(const UnicodeString & AValue)
{
  RawByteString value = EncryptPassword(AValue, GetProxyUsername() + GetProxyHost());
  SET_SESSION_PROPERTY(ProxyPassword);
}

TProxyMethod TSessionData::GetSystemProxyMethod() const
{
  PrepareProxyData();
//  if ((GetProxyMethod() == pmSystem) && (nullptr != FIEProxyConfig))
//    return FIEProxyConfig->ProxyMethod;
  return pmNone;
}

TProxyMethod TSessionData::GetActualProxyMethod() const
{
  return GetProxyMethod(); // == pmSystem ? GetSystemProxyMethod() : GetProxyMethod();
}

UnicodeString TSessionData::GetProxyHost() const
{
  PrepareProxyData();
//  if ((GetProxyMethod() == pmSystem) && (nullptr != FIEProxyConfig))
//    return FIEProxyConfig->ProxyHost;
  return FProxyHost;
}
int32_t TSessionData::GetProxyPort() const
{
  PrepareProxyData();
//  if ((GetProxyMethod() == pmSystem) && (nullptr != FIEProxyConfig))
//    return FIEProxyConfig->ProxyPort;
  return FProxyPort;
}

UnicodeString TSessionData::GetProxyUsername() const
{
  return FProxyUsername;
}

UnicodeString TSessionData::GetProxyPassword() const
{
  return DecryptPassword(FProxyPassword, GetProxyUsername() + GetProxyHost());
}

static void FreeIEProxyConfig(WINHTTP_CURRENT_USER_IE_PROXY_CONFIG * IEProxyConfig)
{
  DebugAssert(IEProxyConfig);
  if (IEProxyConfig->lpszAutoConfigUrl)
    GlobalFree(IEProxyConfig->lpszAutoConfigUrl);
  if (IEProxyConfig->lpszProxy)
    GlobalFree(IEProxyConfig->lpszProxy);
  if (IEProxyConfig->lpszProxyBypass)
    GlobalFree(IEProxyConfig->lpszProxyBypass);
}

void TSessionData::PrepareProxyData() const
{
//  if ((GetProxyMethod() == pmSystem) && (nullptr == FIEProxyConfig))
  {
    FIEProxyConfig = new TIEProxyConfig;
    WINHTTP_CURRENT_USER_IE_PROXY_CONFIG IEProxyConfig;
    nb::ClearStruct(IEProxyConfig);
    TLibraryLoader LibraryLoader("winhttp.dll", true);
    if (LibraryLoader.Loaded())
    {
      typedef BOOL (WINAPI * FWinHttpGetIEProxyConfigForCurrentUser)(WINHTTP_CURRENT_USER_IE_PROXY_CONFIG *);
      FWinHttpGetIEProxyConfigForCurrentUser GetIEProxyConfig = reinterpret_cast<FWinHttpGetIEProxyConfigForCurrentUser>(
          LibraryLoader.GetProcAddress("WinHttpGetIEProxyConfigForCurrentUser"));
      if (!GetIEProxyConfig)
        return;
      if (!GetIEProxyConfig(&IEProxyConfig))
      {
        DWORD Err = ::GetLastError();
        DEBUG_PRINTF("Error reading system proxy configuration, code: %x", Err);
        DebugUsedParam(Err);
      }
      else
      {
        FIEProxyConfig->AutoDetect = !!IEProxyConfig.fAutoDetect;
        if (nullptr != IEProxyConfig.lpszAutoConfigUrl)
        {
          FIEProxyConfig->AutoConfigUrl = IEProxyConfig.lpszAutoConfigUrl;
        }
        if (nullptr != IEProxyConfig.lpszProxy)
        {
          FIEProxyConfig->Proxy = IEProxyConfig.lpszProxy;
        }
        if (nullptr != IEProxyConfig.lpszProxyBypass)
        {
          FIEProxyConfig->ProxyBypass = IEProxyConfig.lpszProxyBypass;
        }
        FreeIEProxyConfig(&IEProxyConfig);
        ParseIEProxyConfig();
      }
    }
  }
}

void TSessionData::ParseIEProxyConfig() const
{
  DebugAssert(FIEProxyConfig);
  TStringList ProxyServerList;
  ProxyServerList.SetDelimiter(L';');
  ProxyServerList.SetDelimitedText(FIEProxyConfig->Proxy);
  UnicodeString ProxyUrl;
  int32_t ProxyPort = 0;
  TProxyMethod ProxyMethod = pmNone;
  UnicodeString ProxyUrlTmp;
  int32_t ProxyPortTmp = 0;
  TProxyMethod ProxyMethodTmp = pmNone;
  for (int32_t Index = 0; Index < ProxyServerList.GetCount(); ++Index)
  {
    UnicodeString ProxyServer = ProxyServerList.GetString(Index).Trim();
    TStringList ProxyServerForScheme;
    ProxyServerForScheme.SetDelimiter(L'=');
    ProxyServerForScheme.SetDelimitedText(ProxyServer);
    UnicodeString ProxyScheme;
    UnicodeString ProxyURI;
    if (ProxyServerForScheme.GetCount() == 2)
    {
      ProxyScheme = ProxyServerList.GetString(0).Trim();
      ProxyURI = ProxyServerList.GetString(1).Trim();
    }
    else
    {
      if (ProxyServerForScheme.GetCount() == 1)
      {
        ProxyScheme = L"http";
        ProxyURI = ProxyServerList.GetString(0).Trim();
        ProxyMethodTmp = pmHTTP;
      }
    }
    if (ProxyUrlTmp.IsEmpty() && (ProxyPortTmp == 0))
    {
      FromURI(ProxyURI, ProxyUrlTmp, ProxyPortTmp, ProxyMethodTmp);
    }
    switch (GetFSProtocol())
    {
    // case fsSCPonly:
    // case fsSFTP:
    // case fsSFTPonly:
    // case fsFTP:
    // case fsFTPS:
    // break;
    case fsWebDAV:
      if ((ProxyScheme == L"http") || (ProxyScheme == L"https"))
      {
        FromURI(ProxyURI, ProxyUrl, ProxyPort, ProxyMethod);
      }
      break;
    default:
      break;
    }
  }
  if (ProxyUrl.IsEmpty() && (ProxyPort == 0) && (ProxyMethod == pmNone))
  {
    ProxyUrl = ProxyUrlTmp;
    ProxyPort = ProxyPortTmp;
    ProxyMethod = ProxyMethodTmp;
  }
  FIEProxyConfig->ProxyHost = ProxyUrl;
  FIEProxyConfig->ProxyPort = ProxyPort;
  FIEProxyConfig->ProxyMethod = ProxyMethod;
}

void TSessionData::FromURI(const UnicodeString & ProxyURI,
  UnicodeString & ProxyUrl, int32_t & ProxyPort, TProxyMethod & ProxyMethod) const
{
  ProxyUrl.Clear();
  ProxyPort = 0;
  ProxyMethod = pmNone;
  int32_t Pos = ProxyURI.RPos(L':');
  if (Pos > 0)
  {
    ProxyUrl = ProxyURI.SubString(1, Pos - 1).Trim();
    ProxyPort = ProxyURI.SubString(Pos + 1).Trim().ToIntPtr();
  }
  // remove scheme from Url e.g. "socks5://" "https://"
  Pos = ProxyUrl.Pos(L"://");
  if (Pos > 0)
  {
    UnicodeString ProxyScheme = ProxyUrl.SubString(1, Pos - 1);
    ProxyUrl = ProxyUrl.SubString(Pos + 3);
    if (ProxyScheme == L"socks4")
    {
      ProxyMethod = pmSocks4;
    }
    else if (ProxyScheme == L"socks5")
    {
      ProxyMethod = pmSocks5;
    }
    else if (ProxyScheme == L"socks")
    {
      ProxyMethod = pmSocks5;
    }
    else if (ProxyScheme == L"http")
    {
      ProxyMethod = pmHTTP;
    }
    else if (ProxyScheme == L"https")
    {
      TODO("pmHTTPS");
      ProxyMethod = pmHTTP;
    }
  }
  if (ProxyMethod == pmNone)
    ProxyMethod = pmHTTP; // default Value
}

void TSessionData::SetProxyTelnetCommand(const UnicodeString & value)
{
  SET_SESSION_PROPERTY(ProxyTelnetCommand);
}

void TSessionData::SetProxyLocalCommand(const UnicodeString & value)
{
  SET_SESSION_PROPERTY(ProxyLocalCommand);
}

void TSessionData::SetProxyDNS(TAutoSwitch value)
{
  SET_SESSION_PROPERTY(ProxyDNS);
}

void TSessionData::SetProxyLocalhost(bool value)
{
  SET_SESSION_PROPERTY(ProxyLocalhost);
}

void TSessionData::SetFtpProxyLogonType(int32_t value)
{
  SET_SESSION_PROPERTY(FtpProxyLogonType);
}

void TSessionData::SetBug(TSshBug Bug, TAutoSwitch value)
{
  DebugAssert(Bug >= 0 && nb::ToUInt32(Bug) < _countof(FBugs));
  SET_SESSION_PROPERTY(Bugs[Bug]);
}

TAutoSwitch TSessionData::GetBug(TSshBug Bug) const
{
  DebugAssert(Bug >= 0 && nb::ToUInt32(Bug) < _countof(FBugs));
  return FBugs[Bug];
}

void TSessionData::SetPuttySettings(const UnicodeString & value)
{
  SET_SESSION_PROPERTY(PuttySettings);
}

void TSessionData::SetCustomParam1(const UnicodeString & value)
{
  SET_SESSION_PROPERTY(CustomParam1);
}

void TSessionData::SetCustomParam2(const UnicodeString & value)
{
  SET_SESSION_PROPERTY(CustomParam2);
}

void TSessionData::SetSFTPDownloadQueue(int32_t value)
{
  SET_SESSION_PROPERTY(SFTPDownloadQueue);
}

void TSessionData::SetSFTPUploadQueue(int32_t value)
{
  SET_SESSION_PROPERTY(SFTPUploadQueue);
}

void TSessionData::SetSFTPListingQueue(int32_t value)
{
  SET_SESSION_PROPERTY(SFTPListingQueue);
}

void TSessionData::SetSFTPMaxVersion(int32_t value)
{
  SET_SESSION_PROPERTY(SFTPMaxVersion);
}

void TSessionData::SetSFTPMaxPacketSize(uint32_t value)
{
  SET_SESSION_PROPERTY(SFTPMaxPacketSize);
}

void TSessionData::SetSFTPRealPath(TAutoSwitch value)
{
  SET_SESSION_PROPERTY(SFTPRealPath);
}

void TSessionData::SetSFTPBug(TSftpBug Bug, TAutoSwitch value)
{
  DebugAssert(Bug >= 0 && nb::ToUInt32(Bug) < _countof(FSFTPBugs));
  SET_SESSION_PROPERTY(SFTPBugs[Bug]);
}

TAutoSwitch TSessionData::GetSFTPBug(TSftpBug Bug) const
{
  DebugAssert(Bug >= 0 && nb::ToUInt32(Bug) < _countof(FSFTPBugs));
  return FSFTPBugs[Bug];
}

void TSessionData::SetSCPLsFullTime(TAutoSwitch value)
{
  SET_SESSION_PROPERTY(SCPLsFullTime);
}

void TSessionData::SetColor(int32_t value)
{
  SET_SESSION_PROPERTY(Color);
}

void TSessionData::SetTunnel(bool value)
{
  SET_SESSION_PROPERTY(Tunnel);
}

void TSessionData::SetTunnelHostName(const UnicodeString & value)
{
  if (FTunnelHostName != value)
  {
    // HostName is key for password encryption
    UnicodeString XTunnelPassword = GetTunnelPassword();

    UnicodeString Value2 = value;
    int32_t P = Value2.LastDelimiter(L"@");
    if (P > 0)
    {
      SetTunnelUserName(Value2.SubString(1, P - 1));
      Value2 = Value2.SubString(P + 1, Value2.Length() - P);
    }
    FTunnelHostName = Value2;
    Modify();

    SetTunnelPassword(XTunnelPassword);
    Shred(XTunnelPassword);
  }
}

void TSessionData::SetTunnelPortNumber(int32_t value)
{
  SET_SESSION_PROPERTY(TunnelPortNumber);
}

void TSessionData::SetTunnelUserName(const UnicodeString & value)
{
  // Avoid password recryption (what may popup master password prompt)
  if (FTunnelUserName != value)
  {
    // TunnelUserName is key for password encryption
    UnicodeString XTunnelPassword = GetTunnelPassword();
    SET_SESSION_PROPERTY(TunnelUserName);
    SetTunnelPassword(XTunnelPassword);
    Shred(XTunnelPassword);
  }
}

void TSessionData::SetTunnelPassword(const UnicodeString & AValue)
{
  RawByteString value = EncryptPassword(AValue, GetTunnelUserName() + GetTunnelHostName());
  SET_SESSION_PROPERTY(TunnelPassword);
}

UnicodeString TSessionData::GetTunnelPassword() const
{
  return DecryptPassword(FTunnelPassword, GetTunnelUserName() + GetTunnelHostName());
}

void TSessionData::SetTunnelPassphrase(const UnicodeString & AValue)
{
  RawByteString value = EncryptPassword(AValue, FTunnelPublicKeyFile);
  SET_SESSION_PROPERTY(TunnelPassphrase);
}

UnicodeString TSessionData::GetTunnelPassphrase() const
{
  return DecryptPassword(FTunnelPassphrase, FTunnelPublicKeyFile);
}

void TSessionData::SetTunnelPublicKeyFile(const UnicodeString & value)
{
  if (FTunnelPublicKeyFile != value)
  {
    // TunnelPublicKeyFile is key for TunnelPassphrase encryption
    UnicodeString XTunnelPassphrase = FTunnelPassphrase;

    // StripPathQuotes should not be needed as we do not feed quotes anymore
    FTunnelPublicKeyFile = StripPathQuotes(value);
    Modify();

    FTunnelPassphrase = XTunnelPassphrase;
    Shred(XTunnelPassphrase);
  }
}

void TSessionData::SetTunnelLocalPortNumber(int32_t value)
{
  SET_SESSION_PROPERTY(TunnelLocalPortNumber);
}

bool TSessionData::GetTunnelAutoassignLocalPortNumber() const
{
  return (FTunnelLocalPortNumber <= 0);
}

void TSessionData::SetTunnelPortFwd(const UnicodeString & value)
{
  SET_SESSION_PROPERTY(TunnelPortFwd);
}

void TSessionData::SetTunnelHostKey(const UnicodeString & value)
{
  SET_SESSION_PROPERTY(TunnelHostKey);
}

void TSessionData::SetFtpPasvMode(bool value)
{
  SET_SESSION_PROPERTY(FtpPasvMode);
}

void TSessionData::SetFtpForcePasvIp(TAutoSwitch value)
{
  SET_SESSION_PROPERTY(FtpForcePasvIp);
}

void TSessionData::SetFtpUseMlsd(TAutoSwitch value)
{
  SET_SESSION_PROPERTY(FtpUseMlsd);
}

void TSessionData::SetFtpAccount(const UnicodeString & value)
{
  SET_SESSION_PROPERTY(FtpAccount);
}

void TSessionData::SetFtpPingInterval(int32_t value)
{
  SET_SESSION_PROPERTY(FtpPingInterval);
}

TDateTime TSessionData::GetFtpPingIntervalDT() const
{
  return SecToDateTime(GetFtpPingInterval());
}

void TSessionData::SetFtpPingType(TPingType value)
{
  SET_SESSION_PROPERTY(FtpPingType);
}

void TSessionData::SetFtpTransferActiveImmediately(TAutoSwitch value)
{
  SET_SESSION_PROPERTY(FtpTransferActiveImmediately);
}

void TSessionData::SetFtps(TFtps value)
{
  SET_SESSION_PROPERTY(Ftps);
}

void TSessionData::SetMinTlsVersion(TTlsVersion value)
{
  SET_SESSION_PROPERTY(MinTlsVersion);
}

void TSessionData::SetMaxTlsVersion(TTlsVersion value)
{
  SET_SESSION_PROPERTY(MaxTlsVersion);
}

void TSessionData::SetLogicalHostName(const UnicodeString & value)
{
  SET_SESSION_PROPERTY(LogicalHostName);
}

void TSessionData::SetFtpListAll(TAutoSwitch value)
{
  SET_SESSION_PROPERTY(FtpListAll);
}

void TSessionData::SetFtpHost(TAutoSwitch value)
{
  SET_SESSION_PROPERTY(FtpHost);
}

void TSessionData::SetFtpWorkFromCwd(TAutoSwitch value)
{
  SET_SESSION_PROPERTY(FtpWorkFromCwd);
}

void TSessionData::SetFtpAnyCodeForPwd(bool value)
{
  SET_SESSION_PROPERTY(FtpAnyCodeForPwd);
}

void TSessionData::SetSslSessionReuse(bool value)
{
  SET_SESSION_PROPERTY(SslSessionReuse);
}

void TSessionData::SetTlsCertificateFile(const UnicodeString & value)
{
  SET_SESSION_PROPERTY(TlsCertificateFile);
}

void TSessionData::SetNotUtf(TAutoSwitch value)
{
  SET_SESSION_PROPERTY(NotUtf);
}

void TSessionData::SetInternalEditorEncoding(int32_t value)
{
  SET_SESSION_PROPERTY(InternalEditorEncoding);
}

void TSessionData::SetS3DefaultRegion(const UnicodeString & value)
{
  SET_SESSION_PROPERTY(S3DefaultRegion);
}

void TSessionData::SetS3SessionToken(const UnicodeString & value)
{
  SET_SESSION_PROPERTY(S3SessionToken);
}

void TSessionData::SetS3Profile(const UnicodeString & value)
{
  SET_SESSION_PROPERTY(S3Profile);
}

void TSessionData::SetS3UrlStyle(TS3UrlStyle value)
{
  SET_SESSION_PROPERTY(S3UrlStyle);
}

void TSessionData::SetS3MaxKeys(TAutoSwitch value)
{
  SET_SESSION_PROPERTY(S3MaxKeys);
}

void TSessionData::SetS3CredentialsEnv(bool value)
{
  SET_SESSION_PROPERTY(S3CredentialsEnv);
}

void TSessionData::SetIsWorkspace(bool value)
{
  SET_SESSION_PROPERTY(IsWorkspace);
}

void TSessionData::SetLink(const UnicodeString & value)
{
  SET_SESSION_PROPERTY(Link);
}

void TSessionData::SetNameOverride(const UnicodeString & value)
{
  SET_SESSION_PROPERTY(NameOverride);
}

void TSessionData::SetHostKey(const UnicodeString & value)
{
  SET_SESSION_PROPERTY(HostKey);
}

void TSessionData::SetNote(const UnicodeString & value)
{
  SET_SESSION_PROPERTY(Note);
}

void TSessionData::SetWinTitle(const UnicodeString & value)
{
  SET_SESSION_PROPERTY(WinTitle);
}

void TSessionData::SetSFTPMinPacketSize(int32_t value)
{
  SET_SESSION_PROPERTY(SFTPMinPacketSize);
}

void TSessionData::SetFtpAllowEmptyPassword(bool value)
{
  SET_SESSION_PROPERTY(FtpAllowEmptyPassword);
}

void TSessionData::SetFtpDupFF(bool value)
{
  SET_SESSION_PROPERTY(FtpDupFF);
}

void TSessionData::SetFtpUndupFF(bool value)
{
  SET_SESSION_PROPERTY(FtpUndupFF);
}

UnicodeString TSessionData::GetEncryptKey() const
{
  return DecryptPassword(FEncryptKey, GetSessionPasswordEncryptionKey());
}

void TSessionData::SetEncryptKey(const UnicodeString & AValue)
{
  RawByteString value = EncryptPassword(AValue, GetSessionPasswordEncryptionKey());
  SET_SESSION_PROPERTY(EncryptKey);
}

void TSessionData::SetWebDavLiberalEscaping(bool value)
{
  SET_SESSION_PROPERTY(WebDavLiberalEscaping);
}

void TSessionData::SetWebDavAuthLegacy(bool value)
{
  SET_SESSION_PROPERTY(WebDavAuthLegacy);
}

UnicodeString TSessionData::GetInfoTip() const
{
  if (GetUsesSsh())
  {
    return FMTLOAD(SESSION_INFO_TIP2,
       GetHostName(), SessionGetUserName(),
       (GetPublicKeyFile().IsEmpty() ? LoadStr(NO_STR) : LoadStr(YES_STR)),
       GetFSProtocolStr());
  }
  else
  {
    return FMTLOAD(SESSION_INFO_TIP_NO_SSH,
      GetHostName(), SessionGetUserName(), GetFSProtocolStr());
  }
}

UnicodeString TSessionData::ExtractLocalName(const UnicodeString & Name)
{
  UnicodeString Result = Name;
  int32_t P = Result.LastDelimiter(L"/");
  if (P > 0)
  {
    Result.Delete(1, P);
  }
  return Result;
}

UnicodeString TSessionData::GetLocalName() const
{
  UnicodeString Result;
  if (HasSessionName())
  {
    Result = ExtractLocalName(GetName());
  }
  else
  {
    Result = GetDefaultSessionName();
  }
  return Result;
}

UnicodeString TSessionData::ExtractFolderName(const UnicodeString & Name)
{
  UnicodeString Result;
  int32_t P = Name.LastDelimiter(L"/");
  if (P > 0)
  {
    Result = Name.SubString(1, P - 1);
  }
  return Result;
}

UnicodeString TSessionData::GetFolderName() const
{
  UnicodeString Result;
  if (HasSessionName() || GetIsWorkspace())
  {
    Result = ExtractFolderName(GetName());
  }
  return Result;
}

UnicodeString TSessionData::ComposePath(
  const UnicodeString & APath, const UnicodeString & Name)
{
  return base::UnixIncludeTrailingBackslash(APath) + Name;
}

void TSessionData::DisableAuthenticationsExceptPassword()
{
  FSshNoUserAuth = false;
  FAuthKI = false;
  FAuthKIPassword = false;
  FAuthGSSAPI = false;
  FAuthGSSAPIKEX = false;
  PublicKeyFile = EmptyStr;
  DetachedCertificate = EmptyStr;
  TlsCertificateFile = EmptyStr;
  Passphrase = EmptyStr;
  FTryAgent = false;
}

TStrings * TSessionData::GetAllOptionNames(bool PuttyExport)
{
  std::unique_ptr<TSessionData> FactoryDefaults(new TSessionData(L""));
  return FactoryDefaults->SaveToOptions(nullptr, false, PuttyExport);
}

bool TSessionData::HasS3AutoCredentials() const
{
  return (FFSProtocol == fsS3) && FS3CredentialsEnv;
}

bool TSessionData::HasAutoCredentials() const
{
  return HasS3AutoCredentials();
}

TLoginType TSessionData::GetLoginType() const
{
  return (SessionGetUserName() == AnonymousUserName) && GetPassword().IsEmpty() ?
    ltAnonymous : ltNormal;
}

void TSessionData::SetLoginType(TLoginType value)
{
  SET_SESSION_PROPERTY(LoginType);
  if (GetLoginType() == ltAnonymous)
  {
    SetPassword(L"");
    SessionSetUserName(AnonymousUserName);
  }
}

uint32_t TSessionData::GetCodePageAsNumber() const
{
  if (FCodePageAsNumber == 0)
    FCodePageAsNumber = ::GetCodePageAsNumber(GetCodePage());
  return FCodePageAsNumber;
}

void TSessionData::SetCodePage(const UnicodeString & value)
{
  SET_SESSION_PROPERTY(CodePage);
  FCodePageAsNumber = 0;
}

void TSessionData::AdjustHostName(UnicodeString & HostName, const UnicodeString & Prefix) const
{
  UnicodeString FullPrefix = Prefix + ProtocolSeparator;
  if (::LowerCase(HostName.SubString(1, FullPrefix.Length())) == FullPrefix)
  {
    HostName.Delete(1, FullPrefix.Length());
  }
}

void TSessionData::RemoveProtocolPrefix(UnicodeString & HostName) const
{
  AdjustHostName(HostName, ScpProtocol);
  AdjustHostName(HostName, SftpProtocol);
  AdjustHostName(HostName, FtpProtocol);
  AdjustHostName(HostName, FtpsProtocol);
  AdjustHostName(HostName, WebDAVProtocol);
  AdjustHostName(HostName, WebDAVSProtocol);
}

TFSProtocol TSessionData::TranslateFSProtocolNumber(int32_t AFSProtocol)
{
  TFSProtocol Result = static_cast<TFSProtocol>(-1);
  if (GetSessionVersion() >= ::GetVersionNumber2110())
  {
    Result = static_cast<TFSProtocol>(AFSProtocol);
  }
  else
  {
    if (AFSProtocol < fsFTPS_219)
    {
      Result = static_cast<TFSProtocol>(AFSProtocol);
    }
    switch (AFSProtocol)
    {
    case fsFTPS_219:
      SetFtps(ftpsExplicitSsl);
      Result = fsFTP;
      break;
    case fsHTTP_219:
      SetFtps(ftpsNone);
      Result = fsWebDAV;
      break;
    case fsHTTPS_219:
      SetFtps(ftpsImplicit);
      Result = fsWebDAV;
      break;
    default:
      break;
    }
  }
  // DebugAssert(Result != -1);
  return Result;
}

TFSProtocol TSessionData::TranslateFSProtocol(const UnicodeString & ProtocolID) const
{
  // Find protocol by string id
  TFSProtocol Result = static_cast<TFSProtocol>(-1);
  for (int32_t Index = 0; Index < FSPROTOCOL_COUNT; ++Index)
  {
    if (FSProtocolNames[Index] == ProtocolID)
    {
      Result = static_cast<TFSProtocol>(Index);
      break;
    }
  }
  if (Result == static_cast<TFSProtocol>(-1))
    Result = CONST_DEFAULT_PROTOCOL;
  DebugAssert(Result != static_cast<TFSProtocol>(-1));
  return Result;
}

TFtps TSessionData::TranslateFtpEncryptionNumber(int32_t FtpEncryption) const
{
  TFtps Result = GetFtps();
  if ((GetSessionVersion() < ::GetVersionNumber2110()) &&
    (GetFSProtocol() == fsFTP) && (GetFtps() != ftpsNone))
  {
    switch (FtpEncryption)
    {
    case fesPlainFTP:
      Result = ftpsNone;
      break;
    case fesExplicitSSL:
      Result = ftpsExplicitSsl;
      break;
    case fesImplicit:
      Result = ftpsImplicit;
      break;
    case fesExplicitTLS:
      Result = ftpsExplicitTls;
      break;
    default:
      break;
    }
  }
  DebugAssert(Result != static_cast<TFtps>(-1));
  return Result;
}

//=== TStoredSessionList ----------------------------------------------
TStoredSessionList::TStoredSessionList() noexcept :
  TNamedObjectList(OBJECT_CLASS_TStoredSessionList), FReadOnly(false)
{
}

TStoredSessionList::TStoredSessionList(bool AReadOnly) noexcept :
  TNamedObjectList(OBJECT_CLASS_TStoredSessionList),
  FDefaultSettings(std::make_unique<TSessionData>(DefaultName)),
  FPendingRemovals(std::make_unique<TStringList>()),
  FReadOnly(AReadOnly)
{
  DebugAssert(GetConfiguration());
  SetOwnsObjects(true);
}

TStoredSessionList::~TStoredSessionList() noexcept
{
//  SAFE_DESTROY(FDefaultSettings);
  for (int32_t Index = 0; Index < GetCount(); ++Index)
  {
    TObject * Obj = TNamedObjectList::AtObject(Index);
    SAFE_DESTROY(Obj);
    SetItem(Index, nullptr);
  }
}

void TStoredSessionList::Load(THierarchicalStorage * Storage,
  bool AsModified, bool UseDefaults, bool PuttyImport)
{
  std::unique_ptr<TStringList> SubKeys(std::make_unique<TStringList>());
  std::unique_ptr<TList> Loaded(std::make_unique<TList>());
  try__finally
  {
    DebugAssert(FAutoSort);
    FAutoSort = false;
    bool WasEmpty = (GetCount() == 0);

    Storage->GetSubKeyNames(SubKeys.get());

    for (int32_t Index = 0; Index < SubKeys->GetCount(); ++Index)
    {
      UnicodeString SessionName = SubKeys->GetString(Index);

      bool ValidName = true;
      try
      {
        TSessionData::ValidatePath(SessionName);
      }
      catch (...)
      {
        ValidName = false;
      }

      if (ValidName)
      {
        TSessionData * SessionData = nullptr;
        if (SessionName == FDefaultSettings->GetName())
        {
          SessionData = FDefaultSettings.get();
        }
        else
        {
          // if the list was empty before loading, do not waste time trying to
          // find existing sites to overwrite (we rely on underlying storage
          // to secure uniqueness of the key names)
          if (WasEmpty)
          {
            SessionData = nullptr;
          }
          else
          {
            SessionData = dyn_cast<TSessionData>(FindByName(SessionName));
          }
        }

        if ((SessionData != FDefaultSettings.get()) || !UseDefaults)
        {
          if (!SessionData)
          {
            SessionData = new TSessionData(L"");
            if (UseDefaults)
            {
              SessionData->CopyData(GetDefaultSettings());
            }
            SessionData->SetName(SessionName);
            Add(SessionData);
          }
          Loaded->Add(SessionData);
          SessionData->Load(Storage, PuttyImport);
          if (AsModified)
          {
            SessionData->SetModified(true);
          }
        }
      }
    }

    if (!AsModified)
    {
      for (int32_t Index = 0; Index < TObjectList::GetCount(); ++Index)
      {
        if (Loaded->IndexOf(GetObj(Index)) < 0)
        {
          Delete(Index);
          Index--;
        }
      }
    }
  },
  __finally
  {
    FAutoSort = true;
    AlphaSort();
    __removed delete SubKeys;
    __removed delete Loaded;
  } end_try__finally
}

void TStoredSessionList::Reload()
{
  if (Count <= GetConfiguration()->DontReloadMoreThanSessions)
  {
    bool SessionList = true;
    std::unique_ptr<THierarchicalStorage> Storage(GetConfiguration()->CreateScpStorage(SessionList));
    if (Storage->OpenSubKey(GetConfiguration()->StoredSessionsSubKey, False))
    {
      Load(Storage.get());
    }
  }
}

void TStoredSessionList::DoSave(THierarchicalStorage * Storage,
  TSessionData * Data, bool All, bool RecryptPasswordOnly,
  TSessionData * FactoryDefaults)
{
  if (All || Data->GetModified())
  {
    if (RecryptPasswordOnly)
    {
      Data->SaveRecryptedPasswords(Storage);
    }
    else
    {
      Data->Save(Storage, false, FactoryDefaults);
    }
  }
}

void TStoredSessionList::DoSave(THierarchicalStorage * Storage,
  bool All, bool RecryptPasswordOnly, TStrings * RecryptPasswordErrors)
{
  std::unique_ptr<TSessionData> FactoryDefaults(std::make_unique<TSessionData>(""));
  try__finally
  {
    while (FPendingRemovals->Count > 0)
    {
      TSessionData::Remove(Storage, FPendingRemovals->GetString(0));
      FPendingRemovals->Delete(0);
    }

    DoSave(Storage, FDefaultSettings.get(), All, RecryptPasswordOnly, FactoryDefaults.get());
    for (int32_t Index = 0; Index < GetCountIncludingHidden(); Index++)
    {
      TSessionData * SessionData = GetAs<TSessionData>(Index);
      try
      {
        DoSave(Storage, SessionData, All, RecryptPasswordOnly, FactoryDefaults.get());
      }
      catch (Exception &E)
      {
        UnicodeString Message;
        if (RecryptPasswordOnly && DebugAlwaysTrue(RecryptPasswordErrors != nullptr) &&
          ExceptionMessage(&E, Message))
        {
          RecryptPasswordErrors->Add(FORMAT("%s: %s", SessionData->GetSessionName(), Message));
        }
        else
        {
          throw;
        }
      }
    }
  },
  __finally__removed
  ({
    delete FactoryDefaults;
  }) end_try__finally
}

void TStoredSessionList::Save(THierarchicalStorage * Storage, bool All)
{
  DoSave(Storage, All, false, nullptr);
}

void TStoredSessionList::DoSave(bool All, bool Explicit,
  bool RecryptPasswordOnly, TStrings * RecryptPasswordErrors)
{
  bool SessionList = true;
  std::unique_ptr<THierarchicalStorage> Storage(GetConfiguration()->CreateScpStorage(SessionList));
  try__finally
  {
    Storage->SetAccessMode(smReadWrite);
    Storage->SetExplicit(Explicit);
    if (Storage->OpenSubKey(GetConfiguration()->GetStoredSessionsSubKey(), true))
    {
      DoSave(Storage.get(), All, RecryptPasswordOnly, RecryptPasswordErrors);
    }
  },
  __finally__removed
  ({
    delete Storage;
  }) end_try__finally

  Saved();
}

void TStoredSessionList::Save(bool All, bool Explicit)
{
  DoSave(All, Explicit, false, nullptr);
}

void TStoredSessionList::RecryptPasswords(TStrings * RecryptPasswordErrors)
{
  DoSave(true, true, true, RecryptPasswordErrors);
}

void TStoredSessionList::Saved()
{
  FDefaultSettings->SetModified(false);
  for (int32_t Index = 0; Index < GetCountIncludingHidden(); ++Index)
  {
    GetAs<TSessionData>(Index)->SetModified(false);
  }
}

#if 0
void TStoredSessionList::ImportLevelFromFilezilla(
  _di_IXMLNode Node, const UnicodeString & Path, _di_IXMLNode SettingsNode)
{
  for (int Index = 0; Index < Node->ChildNodes->Count; Index++)
  {
    _di_IXMLNode ChildNode = Node->ChildNodes->Get(Index);
    if (ChildNode->NodeName == L"Server")
    {
      std::unique_ptr<TSessionData> SessionData(std::make_unique<TSessionData>(L""));
      SessionData->CopyData(DefaultSettings);
      SessionData->ImportFromFilezilla(ChildNode, Path, SettingsNode);
      Add(SessionData.release());
    }
    else if (ChildNode->NodeName == L"Folder")
    {
      UnicodeString Name;

      for (int Index = 0; Index < ChildNode->ChildNodes->Count; Index++)
      {
        _di_IXMLNode PossibleTextMode = ChildNode->ChildNodes->Get(Index);
        if (PossibleTextMode->NodeType == ntText)
        {
          UnicodeString NodeValue = PossibleTextMode->NodeValue;
          AddToList(Name, NodeValue.Trim(), L" ");
        }
      }

      Name = TSessionData::MakeValidName(Name).Trim();

      ImportLevelFromFilezilla(ChildNode, TSessionData::ComposePath(Path, Name), SettingsNode);
    }
  }
}
#endif // #if 0

void TStoredSessionList::ImportFromFilezilla(
  const UnicodeString & /*FileName*/, const UnicodeString & /*ConfigurationFileName*/)
{
  ThrowNotImplemented(3004);
#if 0
  // not sure if the document must exists if we want to use its node
  _di_IXMLDocument ConfigurationDocument;
  _di_IXMLNode SettingsNode;

  if (FileExists(ApiPath(ConfigurationFileName)))
  {
    ConfigurationDocument = interface_cast<Xmlintf::IXMLDocument>(new TXMLDocument(nullptr));
    ConfigurationDocument->LoadFromFile(ConfigurationFileName);
    _di_IXMLNode FileZilla3Node = ConfigurationDocument->ChildNodes->FindNode(L"FileZilla3");
    if (FileZilla3Node != nullptr)
    {
      SettingsNode = FileZilla3Node->ChildNodes->FindNode(L"Settings");
    }
  }

  const _di_IXMLDocument Document = interface_cast<Xmlintf::IXMLDocument>(new TXMLDocument(nullptr));
  Document->LoadFromFile(FileName);
  _di_IXMLNode FileZilla3Node = Document->ChildNodes->FindNode(L"FileZilla3");
  if (FileZilla3Node != nullptr)
  {
    _di_IXMLNode ServersNode = FileZilla3Node->ChildNodes->FindNode(L"Servers");
    if (ServersNode != nullptr)
    {
      ImportLevelFromFilezilla(ServersNode, L"", SettingsNode);
    }
  }
#endif // #if 0
}

UnicodeString FormatKnownHostName(const UnicodeString & HostName, int PortNumber)
{
  return FORMAT(L"%s:%d", HostName, PortNumber);
}

void TStoredSessionList::ImportFromKnownHosts(TStrings * Lines)
{
  bool SessionList = false;
  std::unique_ptr<THierarchicalStorage> HostKeyStorage(GetConfiguration()->CreateScpStorage(SessionList));
  std::unique_ptr<TStrings> KeyList(std::make_unique<TStringList>());
  if (OpenHostKeysSubKey(HostKeyStorage.get(), false))
  {
    HostKeyStorage->GetValueNames(KeyList.get());
  }
  HostKeyStorage.reset(nullptr);

  UnicodeString FirstError;
  for (int32_t Index = 0; Index < Lines->GetCount(); ++Index)
  {
    try
    {
      UnicodeString Line = Lines->GetString(Index);
      Line = Trim(Line);
      if (IsValidOpensshLine(Line))
      {
        int32_t P = Pos(L' ', Line);
        if (P > 0)
        {
          UnicodeString HostNameStr = Line.SubString(1, P - 1);
          Line = Line.SubString(P + 1, Line.Length() - P);

          P = Pos(L',', HostNameStr);
          if (P > 0)
          {
            HostNameStr.SetLength(P - 1);
          }
          P = Pos(L':', HostNameStr);
          int PortNumber = -1;
          if (P > 0)
          {
            UnicodeString PortNumberStr = HostNameStr.SubString(P + 1, HostNameStr.Length() - P);
            PortNumber = ::StrToInt64(PortNumberStr);
            HostNameStr.SetLength(P - 1);
          }
          if (HasIP6LiteralBrackets(HostNameStr))
          {
            HostNameStr = StripIP6LiteralBrackets(HostNameStr);
          }

          UnicodeString NameStr = HostNameStr;
          if (PortNumber >= 0)
          {
            NameStr = FormatKnownHostName(NameStr, PortNumber);
          }

          std::unique_ptr<TSessionData> SessionDataOwner;
          TSessionData * SessionData = dynamic_cast<TSessionData *>(FindByName(NameStr));
          if (SessionData == nullptr)
          {
            SessionData = new TSessionData(L"");
            SessionDataOwner.reset(SessionData);
            SessionData->CopyData(DefaultSettings);
            SessionData->Name = NameStr;
            SessionData->HostName = HostNameStr;
            if (PortNumber >= 0)
            {
              SessionData->PortNumber = PortNumber;
            }
          }

          const struct ssh_keyalg * Algorithm;
          UnicodeString Key = ParseOpenSshPubLine(Line, Algorithm);
          UnicodeString KeyKey =
            FORMAT(L"%s@%d:%s", Algorithm->cache_id, SessionData->PortNumber, HostNameStr);
          UnicodeString HostKey =
            FORMAT(L"%s:%s=%s", Algorithm->ssh_id, KeyKey, Key);
          UnicodeString HostKeyList = SessionData->HostKey;
          AddToList(HostKeyList, HostKey, L";");
          SessionData->HostKey = HostKeyList;
          // If there's at least one unknown key type for this host, select it
          if (KeyList->IndexOf(KeyKey) < 0)
          {
            SessionData->Selected = true;
          }

          if (SessionDataOwner.get() != nullptr)
          {
            Add(SessionDataOwner.release());
          }
        }
      }
    }
    catch (Exception & E)
    {
      if (FirstError.IsEmpty())
      {
        FirstError = E.Message;
      }
    }
  }

  if (GetCount() == 0)
  {
    UnicodeString Message = LoadStr(KNOWN_HOSTS_NO_SITES);
    if (!FirstError.IsEmpty())
    {
      Message = FORMAT("%s\n(%s)", Message, FirstError);
    }

    throw Exception(Message);
  }
}

void TStoredSessionList::ImportFromOpenssh(TStrings * Lines)
{
  std::unique_ptr<TStrings> Hosts(CreateSortedStringList());
  for (int32_t Index = 0; Index < Lines->Count; Index++)
  {
    UnicodeString Line = Lines->GetString(Index);
    UnicodeString Directive, Value;
    if (ParseOpensshDirective(Line, Directive, Value) &&
        SameText(Directive, OpensshHostDirective))
    {
      while (!Value.IsEmpty())
      {
        UnicodeString Name = CutOpensshToken(Value);
        if ((Hosts->IndexOf(Name) < 0) && (Name.LastDelimiter(L"*?") == 0))
        {
          std::unique_ptr<TSessionData> Data(new TSessionData(EmptyStr));
          Data->CopyData(DefaultSettings);
          Data->Name = Name;
          Data->HostName = Name;
          Data->ImportFromOpenssh(Lines);
          Add(Data.release());
          Hosts->Add(Name);
        }
      }
    }
  }
}

void TStoredSessionList::Export(const UnicodeString & /*AFileName*/)
{
  ThrowNotImplemented(3003);
#if 0
  THierarchicalStorage * Storage = TIniFileStorage::CreateFromPath(FileName);
  try__finally
  {
    std::unique_ptr<THierarchicalStorage> Storage(std::make_unique<TIniFileStorage>(FileName));
    Storage->SetAccessMode(smReadWrite);
    if (Storage->OpenSubKey(GetConfiguration()->GetStoredSessionsSubKey(), true))
    {
      Save(Storage, true);
    }
  }
  __finally
  {
    delete Storage;
  };
#endif // #if 0
}

void TStoredSessionList::SelectAll(bool Select)
{
  for (int32_t Index = 0; Index < GetCount(); ++Index)
  {
    GetSession(Index)->SetSelected(Select);
  }
}

void TStoredSessionList::Import(TStoredSessionList * From,
  bool OnlySelected, TList * Imported)
{
  for (int32_t Index = 0; Index < From->GetCount(); ++Index)
  {
    if (!OnlySelected || From->GetSession(Index)->GetSelected())
    {
      TSessionData * Session = new TSessionData("");
      Session->Assign(From->GetSession(Index));
      Session->SetModified(true);
      Session->MakeUniqueIn(this);
      Add(Session);
      if (Imported != nullptr)
      {
        Imported->Add(Session);
      }
    }
  }
  // only modified, explicit
  Save(false, true);
}

void TStoredSessionList::SelectSessionsToImport(
  TStoredSessionList * Dest, bool SSHOnly)
{
  for (int32_t Index = 0; Index < GetCount(); ++Index)
  {
    GetSession(Index)->SetSelected(
      (!SSHOnly || (GetSession(Index)->GetNormalizedPuttyProtocol() == PuttySshProtocol)) &&
      !Dest->FindByName(GetSession(Index)->GetName()));
  }
}

void TStoredSessionList::Cleanup()
{
  try
  {
    if (GetConfiguration()->GetStorage() == stRegistry)
    {
      Clear();
    }
    std::unique_ptr<TRegistryStorage> Storage(std::make_unique<TRegistryStorage>(GetConfiguration()->GetRegistryStorageKey()));
    Storage->Init();
    try__finally
    {
      Storage->SetAccessMode(smReadWrite);
      if (Storage->OpenRootKey(False))
      {
        Storage->RecursiveDeleteSubKey(GetConfiguration()->GetStoredSessionsSubKey());
      }
    },
    __finally__removed
    ({
      delete Storage;
    }) end_try__finally
  }
  catch (Exception &E)
  {
    throw ExtException(&E, LoadStr(CLEANUP_SESSIONS_ERROR));
  }
}

void TStoredSessionList::UpdateStaticUsage()
{
#if 0
  int32_t SCP = 0;
  int32_t SFTP = 0;
  int32_t FTP = 0;
  int32_t FTPS = 0;
  int32_t WebDAV = 0;
  int S3 = 0;
  int32_t Password = 0;
  int32_t Advanced = 0;
  int32_t Color = 0;
  int32_t Note = 0;
  int32_t Tunnel = 0;
  bool Folders = false;
  bool Workspaces = false;
  std::unique_ptr<TSessionData> FactoryDefaults(std::make_unique<TSessionData>(L""));
  std::unique_ptr<TStringList> DifferentAdvancedProperties(CreateSortedStringList());
  for (int32_t Index = 0; Index < Count; Index++)
  {
    TSessionData *Data = Sessions[Index];
    if (Data->IsWorkspace)
    {
      Workspaces = true;
    }
    else
    {
      switch (Data->FSProtocol)
      {
        case fsSCPonly:
          SCP++;
          break;

        case fsSFTP:
        case fsSFTPonly:
          SFTP++;
          break;

        case fsFTP:
          if (Data->Ftps == ftpsNone)
          {
            FTP++;
          }
          else
          {
            FTPS++;
          }
          break;

        case fsWebDAV:
          if (Data->Ftps == ftpsNone)
          {
            WebDAV++;
          }
          else
          {
            WebDAVS++;
          }
          break;

        case fsS3:
          S3++;
          break;
      }

      if (Data->HasAnySessionPassword())
      {
        Password++;
      }

      if (Data->Color != 0)
      {
        Color++;
      }

      if (!Data->Note.IsEmpty())
      {
        Note++;
      }

      // This would not work for passwords, as they are compared in their encrypted form.
      // But there are no passwords set in factory defaults anyway.
      if (!Data->IsSame(FactoryDefaults.get(), true, DifferentAdvancedProperties.get(), false))
      {
        Advanced++;
      }

      if (Data->Tunnel)
      {
        Tunnel++;
      }

      if (!Data->FolderName.IsEmpty())
      {
        Folders = true;
      }
    }
  }

  GetConfiguration()->Usage->Set(L"StoredSessionsCountSCP", SCP);
  GetConfiguration()->Usage->Set(L"StoredSessionsCountSFTP", SFTP);
  GetConfiguration()->Usage->Set(L"StoredSessionsCountFTP", FTP);
  GetConfiguration()->Usage->Set(L"StoredSessionsCountFTPS", FTPS);
  GetConfiguration()->Usage->Set(L"StoredSessionsCountWebDAV", WebDAV);
  GetConfiguration()->Usage->Set(L"StoredSessionsCountWebDAVS", WebDAVS);
  GetConfiguration()->Usage->Set(L"StoredSessionsCountS3", S3);
  GetConfiguration()->Usage->Set(L"StoredSessionsCountPassword", Password);
  GetConfiguration()->Usage->Set(L"StoredSessionsCountColor", Color);
  GetConfiguration()->Usage->Set(L"StoredSessionsCountNote", Note);
  GetConfiguration()->Usage->Set(L"StoredSessionsCountAdvanced", Advanced);
  DifferentAdvancedProperties->Delimiter = L',';
  GetConfiguration()->Usage->Set(L"StoredSessionsAdvancedSettings", DifferentAdvancedProperties->DelimitedText);
  GetConfiguration()->Usage->Set(L"StoredSessionsCountTunnel", Tunnel);

  // actually default might be true, see below for when the default is actually used
  bool CustomDefaultStoredSession = false;
  try
  {
    // this can throw, when the default session settings have password set
    // (and no other basic property, like hostname/username),
    // and master password is enabled as we are called before master password
    // handler is set
    CustomDefaultStoredSession = !FDefaultSettings->IsSame(FactoryDefaults.get(), false);
  }
  catch (...)
  {
  }
  GetConfiguration()->Usage->Set(L"UsingDefaultStoredSession", CustomDefaultStoredSession);

  GetConfiguration()->Usage->Set(L"UsingStoredSessionsFolders", Folders);
  GetConfiguration()->Usage->Set(L"UsingWorkspaces", Workspaces);
#endif // #if 0
}

const TSessionData * TStoredSessionList::FindSame(TSessionData * Data)
{
  const TSessionData * Result = nullptr;
  if (!(Data->GetHidden() || Data->GetName().IsEmpty())) // || Data->GetIsWorkspace())
  {
    const TNamedObject * Obj = FindByName(Data->GetName());
    Result = dyn_cast<TSessionData>(Obj);
  }
  return Result;
}

int32_t TStoredSessionList::IndexOf(TSessionData * Data) const
{
  for (int32_t Index = 0; Index < GetCount(); ++Index)
  {
    if (Data == GetSession(Index))
    {
      return Index;
    }
  }
  return -1;
}

TSessionData * TStoredSessionList::NewSession(
  const UnicodeString & SessionName, TSessionData * Session)
{
  TSessionData * DuplicateSession = dyn_cast<TSessionData>(FindByName(SessionName));
  if (!DuplicateSession)
  {
    std::unique_ptr<TSessionData> DuplicateSession = std::make_unique<TSessionData>("");
    DuplicateSession->Assign(Session);
    DuplicateSession->SetName(SessionName);
    // make sure, that new stored session is saved to registry
    DuplicateSession->SetModified(true);
    Add(DuplicateSession.release());
  }
  else
  {
    DuplicateSession->Assign(Session);
    DuplicateSession->SetName(SessionName);
    DuplicateSession->SetModified(true);
  }
  // list was saved here before to default storage, but it would not allow
  // to work with special lists (export/import) not using default storage
  return DuplicateSession;
}

void TStoredSessionList::SetDefaultSettings(const TSessionData * Value)
{
  DebugAssert(FDefaultSettings);
  if (FDefaultSettings.get() != Value)
  {
    FDefaultSettings->Assign(Value);
    // make sure default settings are saved
    FDefaultSettings->SetModified(true);
    FDefaultSettings->SetName(DefaultName);
    if (!FReadOnly)
    {
      // only modified, explicit
      Save(false, true);
    }
  }
}

bool TStoredSessionList::OpenHostKeysSubKey(THierarchicalStorage * Storage, bool CanCreate)
{
  return
    Storage->OpenRootKey(CanCreate) &&
    Storage->OpenSubKey(GetConfiguration()->GetSshHostKeysSubKey(), CanCreate);
}

THierarchicalStorage * TStoredSessionList::CreateHostKeysStorageForWriting()
{
  bool SessionList = false;
  std::unique_ptr<THierarchicalStorage> Storage(GetConfiguration()->CreateScpStorage(SessionList));
  Storage->SetExplicit(true);
  Storage->SetAccessMode(smReadWrite);
  return Storage.release();
}

int32_t TStoredSessionList::ImportHostKeys(
  THierarchicalStorage * SourceStorage, THierarchicalStorage * TargetStorage, TStoredSessionList * Sessions, bool OnlySelected)
{
  int32_t Result = 0;
  if (OpenHostKeysSubKey(SourceStorage, false) &&
      OpenHostKeysSubKey(TargetStorage, true))
  {
    std::unique_ptr<TStringList> KeyList(std::make_unique<TStringList>());
    SourceStorage->GetValueNames(KeyList.get());

    DebugAssert(Sessions != nullptr);
    for (int32_t Index = 0; Index < Sessions->Count; Index++)
    {
      TSessionData * Session = Sessions->GetSession(Index);
      if (!OnlySelected || Session->Selected)
      {
        UnicodeString HostKeyName = PuttyMungeStr(FORMAT("@%d:%s", Session->PortNumber, Session->HostNameExpanded));
        for (int32_t KeyIndex = 0; KeyIndex < KeyList->Count; KeyIndex++)
        {
          UnicodeString KeyName = KeyList->GetString(KeyIndex);
          if (EndsText(HostKeyName, KeyName))
          {
            TargetStorage->WriteStringRaw(KeyName, SourceStorage->ReadStringRaw(KeyName, ""));
            Result++;
          }
        }
      }
    }
  }
  return Result;
}

void TStoredSessionList::ImportHostKeys(
  const UnicodeString & SourceKey, TStoredSessionList * Sessions, bool OnlySelected)
{
  std::unique_ptr<THierarchicalStorage> TargetStorage(CreateHostKeysStorageForWriting());
  TargetStorage->Init();
  std::unique_ptr<THierarchicalStorage> SourceStorage(std::make_unique<TRegistryStorage>(SourceKey));
  SourceStorage->Init();

  ImportHostKeys(SourceStorage.get(), TargetStorage.get(), Sessions, OnlySelected);
}

void TStoredSessionList::ImportSelectedKnownHosts(TStoredSessionList * Sessions)
{
  std::unique_ptr<THierarchicalStorage> Storage(CreateHostKeysStorageForWriting());
  if (OpenHostKeysSubKey(Storage.get(), true))
  {
    for (int32_t Index = 0; Index < Sessions->GetCount(); ++Index)
    {
      TSessionData * Session = Sessions->GetSession(Index);
      if (Session->GetSelected())
      {
        UnicodeString Algs;
        UnicodeString HostKeys = Session->GetHostKey();
        while (!HostKeys.IsEmpty())
        {
          UnicodeString HostKey = CutToChar(HostKeys, L';', true);
          // skip alg
          CutToChar(HostKey, L':', true);
          UnicodeString Key = CutToChar(HostKey, L'=', true);
          Storage->WriteStringRaw(Key, HostKey);
        }
      }
    }
  }
}

void TStoredSessionList::SelectKnownHostsForSelectedSessions(
  TStoredSessionList * KnownHosts, TStoredSessionList * Sessions)
{
  for (int32_t SessionIndex = 0; SessionIndex < Sessions->Count; SessionIndex++)
  {
    TSessionData * Session = Sessions->GetSession(SessionIndex);
    if (Session->FSelected)
    {
      UnicodeString Key = Session->FHostName;
      if (Session->PortNumber != Session->GetDefaultPort())
      {
        Key = FormatKnownHostName(Key, Session->FPortNumber);
      }
      TSessionData * KnownHost = dyn_cast<TSessionData>(KnownHosts->FindByName(Key));
      if (KnownHost != nullptr)
      {
        KnownHost->Selected = true;
      }
    }
  }
}

const TSessionData * TStoredSessionList::GetFirstFolderOrWorkspaceSession(const UnicodeString & Name) const
{
  const TSessionData * Result = nullptr;
  if (!Name.IsEmpty())
  {
    UnicodeString NameWithSlash = base::UnixIncludeTrailingBackslash(Name); // optimization
    for (int32_t Index = 0; (Result == nullptr) && (Index < Count); Index++)
    {
      if (GetSession(Index)->IsInFolderOrWorkspace(NameWithSlash))
      {
        Result = GetSession(Index);
      }
    }
  }

  return Result;
}

bool TStoredSessionList::IsFolderOrWorkspace(const UnicodeString & Name) const
{
  return (GetFirstFolderOrWorkspaceSession(Name) != nullptr);
}

bool TStoredSessionList::GetIsFolder(const UnicodeString & Name) const
{
  const TSessionData * SessionData = GetFirstFolderOrWorkspaceSession(Name);
  return (SessionData != nullptr) && !SessionData->GetIsWorkspace();
}

bool TStoredSessionList::GetIsWorkspace(const UnicodeString & Name) const
{
  const TSessionData * SessionData = GetFirstFolderOrWorkspaceSession(Name);
  return (SessionData != nullptr) && SessionData->GetIsWorkspace();
}

TSessionData * TStoredSessionList::CheckIsInFolderOrWorkspaceAndResolve(
  TSessionData * Data, const UnicodeString & Name)
{
  if (Data->IsInFolderOrWorkspace(Name))
  {
    Data = ResolveWorkspaceData(Data);

    if ((Data != nullptr) && Data->CanOpen &&
        DebugAlwaysTrue(Data->GetLink().IsEmpty()))
    {
      return Data;
    }
  }
  return nullptr;
}

void TStoredSessionList::GetFolderOrWorkspace(const UnicodeString & Name, TList * List)
{
  DoGetFolderOrWorkspace(Name, List, false);
}

void TStoredSessionList::DoGetFolderOrWorkspace(const UnicodeString & Name, TList * List, bool NoRecrypt)
{
  for (int32_t Index = 0; (Index < Count); Index++)
  {
    TSessionData * RawData = GetSession(Index);
    TSessionData * Data =
      CheckIsInFolderOrWorkspaceAndResolve(RawData, Name);

    if (Data != nullptr)
    {
      std::unique_ptr<TSessionData>Data2(std::make_unique<TSessionData>(L""));
      if (NoRecrypt)
      {
        Data2->CopyDataNoRecrypt(Data);
      }
      else
      {
        Data2->Assign(Data);
      }

      if (!RawData->GetLink().IsEmpty() && (DebugAlwaysTrue(Data != RawData)) &&
          // BACKWARD COMPATIBILITY
          // When loading pre-5.6.4 workspace, that does not have state saved,
          // do not overwrite the site "state" defaults
          // with (empty) workspace state
          RawData->HasStateData())
      {
        Data2->CopyStateData(RawData);
      }

      if (!RawData->NameOverride().IsEmpty())
      {
        Data2->Name = RawData->NameOverride();
      }
      else if (RawData->GetLink().IsEmpty() && RawData->GetIsWorkspace())
      {
        // Newly opened ad-hoc session has no name, so restore the workspace that way too.
        // Otherwise we would persist the generated internal workspace name as a real name.
        Data2->Name = UnicodeString();
      }

      List->Add(Data2.release());
    }
  }
}

TStrings * TStoredSessionList::GetFolderOrWorkspaceList(
  const UnicodeString & Name)
{
  std::unique_ptr<TObjectList> DataList(new TObjectList());
  DoGetFolderOrWorkspace(Name, DataList.get(), true);

  std::unique_ptr<TStringList> Result(std::make_unique<TStringList>());
  for (int32_t Index = 0; (Index < DataList->Count); Index++)
  {
    Result->Add(dyn_cast<TSessionData>(DataList->GetObj(Index))->GetSessionName());
  }

  return Result.release();
}

TStrings * TStoredSessionList::GetWorkspaces() const
{
  std::unique_ptr<TStringList> Result(CreateSortedStringList());

  for (int32_t Index = 0; Index < GetCount(); ++Index)
  {
    const TSessionData * Data = GetSession(Index);
    if (Data->GetIsWorkspace())
    {
      Result->Add(Data->GetFolderName());
    }
  }

  return Result.release();
}

void TStoredSessionList::NewWorkspace(
  const UnicodeString & Name, TList * DataList)
{
  for (int32_t Index = 0; Index < GetCount(); ++Index)
  {
    TSessionData * Data = GetSession(Index);
    if (Data->IsInFolderOrWorkspace(Name))
    {
      FPendingRemovals->Add(Data->Name);
      Remove(Data);
      Index--;
    }
  }

  for (int32_t Index = 0; Index < DataList->GetCount(); ++Index)
  {
    TSessionData * Data = DataList->GetAs<TSessionData>(Index);

    std::unique_ptr<TSessionData> Data2 = std::make_unique<TSessionData>("");
    Data2->Assign(Data);
    Data2->SetName(TSessionData::ComposePath(Name, Data->GetName()));
    // make sure, that new stored session is saved to registry
    Data2->SetModified(true);
    Add(Data2.release());
  }
}

bool TStoredSessionList::HasAnyWorkspace() const
{
  bool Result = false;
  for (int32_t Index = 0; !Result && (Index < GetCount()); ++Index)
  {
    const TSessionData * Data = GetSession(Index);
    Result = Data->GetIsWorkspace();
  }
  return Result;
}

TSessionData * TStoredSessionList::ParseUrl(const UnicodeString & Url,
  TOptions * Options, bool & DefaultsOnly, UnicodeString * AFileName,
  bool * AProtocolDefined, UnicodeString * MaskedUrl, int32_t Flags)
{
  std::unique_ptr<TSessionData> Data(std::make_unique<TSessionData>(""));
  try__catch
  {
    Data->ParseUrl(Url, Options, this, DefaultsOnly, AFileName, AProtocolDefined, MaskedUrl, Flags);
  }
  catch__removed
  ({
    delete Data;
    throw;
  })
  return Data.release();
}

bool TStoredSessionList::IsUrl(const UnicodeString & Url)
{
  bool DefaultsOnly;
  bool ProtocolDefined = false;
  std::unique_ptr<TSessionData> ParsedData(ParseUrl(Url, nullptr, DefaultsOnly, nullptr, &ProtocolDefined));
  bool Result = ProtocolDefined;
  return Result;
}

TSessionData * TStoredSessionList::ResolveWorkspaceData(TSessionData * Data)
{
  if (!Data->GetLink().IsEmpty())
  {
    Data = dyn_cast<TSessionData>(FindByName(Data->GetLink()));
    if (Data != nullptr)
    {
      Data = ResolveWorkspaceData(Data);
    }
  }
  return Data;
}

TSessionData * TStoredSessionList::SaveWorkspaceData(TSessionData * Data, int Index)
{
  std::unique_ptr<TSessionData> Result(std::make_unique<TSessionData>(""));

  const TSessionData * SameData = StoredSessions->FindSame(Data);
  if (SameData != nullptr)
  {
    Result->CopyStateData(Data);
    Result->SetLink(Data->GetName());
  }
  else
  {
    Result->Assign(Data);
    Result->NameOverride = Data->Name;
  }

  Result->SetIsWorkspace(true);
  Result->Name = IntToHex(Index, 4); // See HasSessionName()

  return Result.release();
}

bool TStoredSessionList::CanOpen(TSessionData * Data)
{
  Data = ResolveWorkspaceData(Data);
  return (Data != nullptr) && Data->CanOpen;
}

const TSessionData * TStoredSessionList::GetSessionByName(const UnicodeString & SessionName) const
{
  for (int32_t Index = 0; Index < GetCount(); ++Index)
  {
    const TSessionData * SessionData = GetSession(Index);
    if (SessionData->GetName() == SessionName)
    {
      return SessionData;
    }
  }
  return nullptr;
}
#if 0
void TStoredSessionList::Load(const UnicodeString & AKey, bool UseDefaults)
{
  std::unique_ptr<TRegistryStorage> Storage(std::make_unique<TRegistryStorage>(AKey));
  Storage->Init();
  if (Storage->OpenRootKey(false))
  {
    Load(Storage.get(), false, UseDefaults);
  }
}
#endif // if 0
//===========================================================================
UnicodeString GetExpandedLogFileName(const UnicodeString & LogFileName, TDateTime Started, TSessionData * SessionData)
{
  // StripPathQuotes should not be needed as we do not feed quotes anymore
  UnicodeString Result = StripPathQuotes(::ExpandEnvironmentVariables(LogFileName));
  for (int32_t Index = 1; Index < Result.Length(); ++Index)
  {
    if ((Index < Result.Length()) && (Result[Index] == L'&'))
    {
      UnicodeString Replacement;
      // keep consistent with TFileCustomCommand::PatternReplacement
      uint16_t Y, M, D, H, NN, S, MS;
      TDateTime DateTime = Started;
      DateTime.DecodeDate(Y, M, D);
      DateTime.DecodeTime(H, NN, S, MS);
      switch (::LowCase(Result[Index + 1]))
      {
        case L'y':
          // Replacement = FormatDateTime(L"yyyy", N);
          Replacement = FORMAT("%04d", Y);
          break;

        case L'm':
          // Replacement = FormatDateTime(L"mm", N);
          Replacement = FORMAT("%02d", M);
          break;

        case L'd':
          // Replacement = FormatDateTime(L"dd", N);
          Replacement = FORMAT("%02d", D);
          break;

        case L't':
          // Replacement = FormatDateTime("hhnnss", N);
          Replacement = FORMAT("%02d%02d%02d", H, NN, S);
          break;

        case 'p':
          Replacement = ::Int64ToStr(nb::ToInt32(::GetCurrentProcessId()));
          break;

        case L'@':
          if (SessionData != nullptr)
          {
            Replacement = MakeValidFileName(SessionData->GetHostNameExpanded());
          }
          else
          {
            Replacement = "nohost";
          }
          break;

        case L's':
          if (SessionData != nullptr)
          {
            Replacement = MakeValidFileName(SessionData->GetSessionName());
          }
          else
          {
            Replacement = "nosession";
          }
          break;

        case L'&':
          Replacement = "&";
          break;

        case L'!':
          Replacement = "!";
          break;

        default:
          Replacement = UnicodeString("&") + Result[Index + 1];
          break;
      }
      Result.Delete(Index, 2);
      Result.Insert(Replacement, Index);
      Index += Replacement.Length() - 1;
    }
  }
  return Result;
}

bool GetIsSshProtocol(TFSProtocol FSProtocol)
{
  return
    (FSProtocol == fsSFTPonly) || (FSProtocol == fsSFTP) ||
    (FSProtocol == fsSCPonly);
}

int32_t DefaultPort(TFSProtocol FSProtocol, TFtps Ftps)
{
  int32_t Result;
  switch (FSProtocol)
  {
    case fsFTP:
      if (Ftps == ftpsImplicit)
      {
        Result = FtpsImplicitPortNumber;
      }
      else
      {
        Result = FtpPortNumber;
      }
      break;

    case fsWebDAV:
    case fsS3:
      if (Ftps == ftpsNone)
      {
        Result = HTTPPortNumber;
      }
      else
      {
        Result = HTTPSPortNumber;
      }
      break;

    default:
      if (GetIsSshProtocol(FSProtocol))
      {
        Result = SshPortNumber;
      }
      else
      {
        DebugFail();
        Result = -1;
      }
      break;
  }
  return Result;
}

bool GetCodePageInfo(UINT CodePage, CPINFOEX & CodePageInfoEx)
{
  if (!GetCPInfoEx(CodePage, 0, &CodePageInfoEx))
  {
    CPINFO CodePageInfo;

    if (!GetCPInfo(CodePage, &CodePageInfo))
      return false;

    CodePageInfoEx.MaxCharSize = CodePageInfo.MaxCharSize;
    CodePageInfoEx.CodePageName[0] = L'\0';
  }

  //if (CodePageInfoEx.MaxCharSize != 1)
  //  return false;

  return true;
}

uint32_t GetCodePageAsNumber(const UnicodeString & CodePage)
{
  uint32_t codePage = _wtoi(CodePage.c_str());
  return nb::ToUIntPtr(codePage == 0 ? CONST_DEFAULT_CODEPAGE : codePage);
}

UnicodeString GetCodePageAsString(uint32_t CodePage)
{
  CPINFOEX cpInfoEx;
  if (::GetCodePageInfo(static_cast<UINT>(CodePage), cpInfoEx))
  {
    return UnicodeString(cpInfoEx.CodePageName);
  }
  return ::IntToStr(CONST_DEFAULT_CODEPAGE);
}

