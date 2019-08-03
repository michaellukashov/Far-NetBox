//---------------------------------------------------------------------------
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

#include "SessionData.h"
#include "CoreMain.h"
#include "TextsCore.h"
#include "PuttyIntf.h"
#include "RemoteFiles.h"
#include "SftpFileSystem.h"
#include "S3FileSystem.h"
//---------------------------------------------------------------------------
__removed #pragma package(smart_init)
//---------------------------------------------------------------------------
const wchar_t *PingTypeNames = L"Off;Null;Dummy";
const wchar_t *ProxyMethodNames = L"None;SOCKS4;SOCKS5;HTTP;Telnet;Cmd";
const wchar_t *DefaultName = L"Default Settings";
const UnicodeString CipherNames[CIPHER_COUNT] = { L"WARN", L"3des", L"blowfish", L"aes", L"des", L"arcfour", L"chacha20" };
const UnicodeString KexNames[KEX_COUNT] = { L"WARN", L"dh-group1-sha1", L"dh-group14-sha1", L"dh-gex-sha1", L"rsa", L"ecdh" };
const UnicodeString HostKeyNames[HOSTKEY_COUNT] = {L"WARN", L"rsa", L"dsa", L"ecdsa", L"ed25519"};
const UnicodeString GssLibNames[GSSLIB_COUNT] = {L"gssapi32", L"sspi", L"custom"};
const wchar_t SshProtList[][10] = {L"1", L"1>2", L"2>1", L"2"};
// Update also order in Ssh2CipherList()
const TCipher DefaultCipherList[CIPHER_COUNT] =
  { cipAES, cipChaCha20, cipBlowfish, cip3DES, cipWarn, cipArcfour, cipDES };
// Update also order in SshKexList()
const TKex DefaultKexList[KEX_COUNT] =
  { kexECDH, kexDHGEx, kexDHGroup14, kexRSA, kexWarn, kexDHGroup1 };
const THostKey DefaultHostKeyList[HOSTKEY_COUNT] =
  { hkED25519, hkECDSA, hkRSA, hkDSA, hkWarn };
const TGssLib DefaultGssLibList[GSSLIB_COUNT] =
  { gssGssApi32, gssSspi, gssCustom };
const wchar_t FSProtocolNames[FSPROTOCOL_COUNT][16] = { L"SCP", L"SFTP (SCP)", L"SFTP", L"", L"", L"FTP", L"WebDAV", L"S3" };
const intptr_t SshPortNumber = 22;
const intptr_t FtpPortNumber = 21;
const intptr_t FtpsImplicitPortNumber = 990;
const intptr_t HTTPPortNumber = 80;
const intptr_t HTTPSPortNumber = 443;
const intptr_t TelnetPortNumber = 23;
const intptr_t DefaultSendBuf = 256 * 1024;
const intptr_t ProxyPortNumber = 80;
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
const uintptr_t CONST_DEFAULT_CODEPAGE = CP_UTF8;
const TFSProtocol CONST_DEFAULT_PROTOCOL = fsSFTP;
//---------------------------------------------------------------------
static TDateTime SecToDateTime(intptr_t Sec)
{
  return TDateTime(double(Sec) / SecsPerDay);
}
//--- TSessionData ----------------------------------------------------
TSessionData::TSessionData(UnicodeString AName) noexcept :
  TNamedObject(OBJECT_CLASS_TSessionData, AName)
{
  Default();
  FModified = true;
}
//---------------------------------------------------------------------
TSessionData::~TSessionData() noexcept
{
  if (nullptr != FIEProxyConfig)
  {
    SAFE_DESTROY(FIEProxyConfig);
  }
}
//---------------------------------------------------------------------
intptr_t TSessionData::Compare(const TNamedObject *Other) const
{
  intptr_t Result;
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
//---------------------------------------------------------------------
TSessionData *TSessionData::Clone() const
{
  std::unique_ptr<TSessionData> Data(std::make_unique<TSessionData>(""));
  Data->Assign(this);
  return Data.release();
}
//---------------------------------------------------------------------
void TSessionData::Default()
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
  SetAuthTIS(false);
  SetAuthKI(true);
  SetAuthKIPassword(true);
  SetAuthGSSAPI(false);
  SetGSSAPIFwdTGT(false);
  SetLogicalHostName("");
  SetChangeUsername(false);
  SetCompression(false);
  SetSshProt(ssh2only);
  SetSsh2DES(false);
  SetSshNoUserAuth(false);
  for (intptr_t Index = 0; Index < CIPHER_COUNT; ++Index)
  {
    SetCipher(Index, DefaultCipherList[Index]);
  }
  for (intptr_t Index = 0; Index < KEX_COUNT; ++Index)
  {
    SetKex(Index, DefaultKexList[Index]);
  }
  for (intptr_t Index = 0; Index < HOSTKEY_COUNT; Index++)
  {
    SetHostKeys(Index, DefaultHostKeyList[Index]);
  }
  for (intptr_t Index = 0; Index < GSSLIB_COUNT; ++Index)
  {
    SetGssLib(Index, DefaultGssLibList[Index]);
  }
  SetGssLibCustom("");
  SetPublicKeyFile("");
  SetPassphrase("");
  SetPuttyProtocol("");
  SetTcpNoDelay(true);
  SetSendBuf(DefaultSendBuf);
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

  SetProxyPort(ProxyPortNumber);
  SetProxyUsername("");
  SetProxyPassword("");
  SetProxyTelnetCommand("connect %host %port\\n");
  SetProxyLocalCommand("");
  SetProxyDNS(asAuto);
  SetProxyLocalhost(false);

  for (intptr_t Index = 0; Index < nb::ToIntPtr(_countof(FBugs)); ++Index)
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
  SetRemoteDirectory("");
  SetSynchronizeBrowsing(false);
  SetUpdateDirectories(true);
  SetCacheDirectories(false);
  SetCacheDirectoryChanges(false);
  SetPreserveDirectoryChanges(false);
  SetLockInHome(false);
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
  SetShell(""); //default shell
  SetReturnVar("");
  SetExitCode1IsError(false);
  SetUnsetNationalVars(true);
  SetListingCommand("ls -la");
  SetIgnoreLsWarnings(true);
  SetScp1Compatibility(false);
  SetTimeDifference(TDateTime(0.0));
  SetTimeDifferenceAuto(true);
  SetSCPLsFullTime(asAuto);
  SetNotUtf(asOn); // asAuto

  S3DefaultRegion = "";

  // SFTP
  SetSftpServer("");
  SetSFTPDownloadQueue(32);
  SetSFTPUploadQueue(32);
  SetSFTPListingQueue(2);
  SetSFTPMaxVersion(::SFTPMaxVersion);
  SetSFTPMaxPacketSize(0);
  SetSFTPMinPacketSize(0);

  for (intptr_t Index = 0; Index < nb::ToIntPtr(_countof(FSFTPBugs)); ++Index)
  {
    SetSFTPBug(static_cast<TSftpBug>(Index), asAuto);
  }

  SetTunnel(false);
  SetTunnelHostName("");
  SetTunnelPortNumber(SshPortNumber);
  SetTunnelUserName("");
  SetTunnelPassword("");
  SetTunnelPublicKeyFile("");
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
  SetMinTlsVersion(tls10);
  SetMaxTlsVersion(tls12);
  SetFtpListAll(asAuto);
  SetFtpHost(asAuto);
  SetFtpDupFF(false);
  SetFtpUndupFF(false);
  SetFtpDeleteFromCwd(asAuto);
  SetSslSessionReuse(true);
  SetTlsCertificateFile("");

  SetFtpProxyLogonType(0); // none

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
//---------------------------------------------------------------------
void TSessionData::NonPersistent()
{
  SetUpdateDirectories(false);
  SetPreserveDirectoryChanges(false);
}
//---------------------------------------------------------------------
#define BASE_PROPERTIES \
  PROPERTY(HostName); \
  PROPERTY(PortNumber); \
  PROPERTY(Password); \
  PROPERTY(PublicKeyFile); \
  PROPERTY(Passphrase); \
  PROPERTY(FSProtocol); \
  PROPERTY(Ftps); \
  PROPERTY(LocalDirectory); \
  PROPERTY(RemoteDirectory); \
  PROPERTY(Color); \
  PROPERTY(SynchronizeBrowsing); \
  PROPERTY(Note);
//---------------------------------------------------------------------
#define ADVANCED_PROPERTIES \
  PROPERTY(NewPassword); \
  PROPERTY(ChangePassword); \
  PROPERTY(PingInterval); \
  PROPERTY(PingType); \
  PROPERTY(Timeout); \
  PROPERTY(TryAgent); \
  PROPERTY(AgentFwd); \
  PROPERTY(AuthTIS); \
  PROPERTY(LogicalHostName); \
  PROPERTY(ChangeUsername); \
  PROPERTY(Compression); \
  PROPERTY(SshProt); \
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
  PROPERTY(LockInHome); \
  PROPERTY(Special); \
  PROPERTY(Selected); \
  PROPERTY(ReturnVar); \
  PROPERTY(ExitCode1IsError); \
  PROPERTY(LookupUserGroups); \
  PROPERTY(EOLType); \
  PROPERTY(TrimVMSVersions); \
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
  PROPERTY(SshSimple); \
  PROPERTY(AuthKI); \
  PROPERTY(AuthKIPassword); \
  PROPERTY(AuthGSSAPI); \
  PROPERTY(GSSAPIFwdTGT); \
  PROPERTY(DeleteToRecycleBin); \
  PROPERTY(OverwrittenToRecycleBin); \
  PROPERTY(RecycleBinPath); \
  PROPERTY(NotUtf); \
  PROPERTY(PostLoginCommands); \
  \
  PROPERTY(S3DefaultRegion); \
  \
  PROPERTY(ProxyMethod); \
  PROPERTY(ProxyHost); \
  PROPERTY(ProxyPort); \
  PROPERTY(ProxyUsername); \
  PROPERTY(ProxyPassword); \
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
  \
  PROPERTY(Tunnel); \
  PROPERTY(TunnelHostName); \
  PROPERTY(TunnelPortNumber); \
  PROPERTY(TunnelUserName); \
  PROPERTY(TunnelPassword); \
  PROPERTY(TunnelPublicKeyFile); \
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
  PROPERTY(FtpDupFF); \
  PROPERTY(FtpUndupFF); \
  PROPERTY(FtpDeleteFromCwd); \
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
  PROPERTY(EncryptKey); \
  \
  PROPERTY(CustomParam1); \
  PROPERTY(CustomParam2); \
  \
  PROPERTY(CodePage); \
  PROPERTY(LoginType); \
  PROPERTY(FtpAllowEmptyPassword);

#define META_PROPERTIES \
  PROPERTY(IsWorkspace); \
  PROPERTY(Link); \
  PROPERTY(NameOverride);
//---------------------------------------------------------------------
void TSessionData::Assign(const TPersistent *Source)
{
  if (Source && isa<TSessionData>(Source))
  {
    TSessionData *SourceData = dyn_cast<TSessionData>(const_cast<TPersistent *>(Source));
    CopyData(SourceData);
    FSource = SourceData->FSource;
  }
  else
  {
    TNamedObject::Assign(Source);
  }
}
//---------------------------------------------------------------------
void TSessionData::CopyData(const TSessionData *SourceData)
{
  #define PROPERTY(P) Set ## P(SourceData->Get ## P())
  PROPERTY(Name);
  BASE_PROPERTIES;
  ADVANCED_PROPERTIES;
  //META_PROPERTIES;


  SessionSetUserName(SourceData->SessionGetUserName());
  for (intptr_t Index = 0; Index < nb::ToIntPtr(_countof(FBugs)); ++Index)
  {
    // PROPERTY(Bug[(TSshBug)Index]);
    SetBug(static_cast<TSshBug>(Index),
      SourceData->GetBug(static_cast<TSshBug>(Index)));
  }
  for (intptr_t Index = 0; Index < nb::ToIntPtr(_countof(FSFTPBugs)); ++Index)
  {
    // PROPERTY(SFTPBug[(TSftpBug)Index]);
    SetSFTPBug(static_cast<TSftpBug>(Index),
      SourceData->GetSFTPBug(static_cast<TSftpBug>(Index)));
  }
  // Restore default kex list
  for (intptr_t Index = 0; Index < KEX_COUNT; ++Index)
  {
    SetKex(Index, DefaultKexList[Index]);
  }

  #undef PROPERTY
  FOverrideCachedHostKey = SourceData->GetOverrideCachedHostKey();
  FModified = SourceData->GetModified();
  FSaveOnly = SourceData->GetSaveOnly();

  FSource = SourceData->FSource;
  FNumberOfRetries = SourceData->FNumberOfRetries;
}
//---------------------------------------------------------------------
void TSessionData::CopyDirectoriesStateData(TSessionData *SourceData)
{
  SetRemoteDirectory(SourceData->GetRemoteDirectory());
  SetLocalDirectory(SourceData->GetLocalDirectory());
  SetSynchronizeBrowsing(SourceData->GetSynchronizeBrowsing());
}
//---------------------------------------------------------------------
bool TSessionData::HasStateData() const
{
  return
    !GetRemoteDirectory().IsEmpty() ||
    !GetLocalDirectory().IsEmpty() ||
    (GetColor() != 0);
}
//---------------------------------------------------------------------
void TSessionData::CopyStateData(TSessionData *SourceData)
{
  // Keep in sync with TCustomScpExplorerForm::UpdateSessionData.
  CopyDirectoriesStateData(SourceData);
  SetColor(SourceData->GetColor());
}
//---------------------------------------------------------------------
void TSessionData::CopyNonCoreData(TSessionData *SourceData)
{
  CopyStateData(SourceData);
  SetUpdateDirectories(SourceData->GetUpdateDirectories());
  SetNote(SourceData->GetNote());
}
//---------------------------------------------------------------------
bool TSessionData::IsSame(const TSessionData *Default, bool AdvancedOnly, TStrings *DifferentProperties) const
{
  bool Result = true;
#define PROPERTY(P) \
    if (Get ## P() != Default->Get ## P()) \
    { \
      if (DifferentProperties != nullptr) \
      { \
        DifferentProperties->Add(# P); \
      } \
      Result = false; \
    }

  if (!AdvancedOnly)
  {
    BASE_PROPERTIES;
    //META_PROPERTIES;
  }
  ADVANCED_PROPERTIES;
#undef PROPERTY

  for (intptr_t Index = 0; Index < nb::ToIntPtr(_countof(FBugs)); ++Index)
  {
    // PROPERTY(Bug[(TSshBug)Index]);
    if (GetBug(static_cast<TSshBug>(Index)) != Default->GetBug(static_cast<TSshBug>(Index)))
      return false;
  }
  for (intptr_t Index = 0; Index < nb::ToIntPtr(_countof(FSFTPBugs)); ++Index)
  {
    // PROPERTY(SFTPBug[(TSftpBug)Index]);
    if (GetSFTPBug(static_cast<TSftpBug>(Index)) != Default->GetSFTPBug(static_cast<TSftpBug>(Index)))
      return false;
  }

  return Result;
}
//---------------------------------------------------------------------
bool TSessionData::IsSame(const TSessionData *Default, bool AdvancedOnly) const
{
  return IsSame(Default, AdvancedOnly, nullptr);
}
//---------------------------------------------------------------------------
TFSProtocol NormalizeFSProtocol(TFSProtocol FSProtocol)
{
  if ((FSProtocol == fsSCPonly) || (FSProtocol == fsSFTPonly))
  {
    FSProtocol = fsSFTP;
  }
  return FSProtocol;
}
//---------------------------------------------------------------------
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
//---------------------------------------------------------------------
bool TSessionData::IsInFolderOrWorkspace(UnicodeString AFolder) const
{
  return ::StartsText(base::UnixIncludeTrailingBackslash(AFolder), GetName());
}
//---------------------------------------------------------------------
void TSessionData::DoLoad(THierarchicalStorage *Storage, bool PuttyImport, bool &RewritePassword)
{
  SetSessionVersion(::StrToVersionNumber(Storage->ReadString("Version", "")));
  // Make sure we only ever use methods supported by TOptionsStorage
  // (implemented by TOptionsIniFile)

  SetPortNumber(Storage->ReadInteger("PortNumber", static_cast<int>(GetPortNumber())));
  SessionSetUserName(Storage->ReadString("UserName", SessionGetUserName()));
  // must be loaded after UserName, because HostName may be in format user@host
  SetHostName(Storage->ReadString("HostName", GetHostName()));

  if (!GetConfiguration()->GetDisablePasswordStoring())
  {
    if (Storage->ValueExists("PasswordPlain"))
    {
      SetPassword(Storage->ReadString("PasswordPlain", GetPassword()));
      RewritePassword = true;
    }
    else
    {
      FPassword = Storage->ReadStringAsBinaryData("Password", FPassword);
    }
  }
  SetHostKey(Storage->ReadString("SshHostKey", GetHostKey())); // probably never used
  SetNote(Storage->ReadString("Note", GetNote()));
  // Putty uses PingIntervalSecs
  intptr_t PingIntervalSecs = Storage->ReadInteger("PingIntervalSecs", -1);
  if (PingIntervalSecs < 0)
  {
    PingIntervalSecs = Storage->ReadInteger("PingIntervalSec", GetPingInterval() % SecsPerMin);
  }
  SetPingInterval(
    Storage->ReadInteger("PingInterval", static_cast<int>(GetPingInterval()) / SecsPerMin) * SecsPerMin +
    PingIntervalSecs);
  if (GetPingInterval() == 0)
  {
    SetPingInterval(30);
  }
  SetPingType(static_cast<TPingType>(Storage->ReadInteger("PingType", GetPingType())));
  SetTimeout(Storage->ReadInteger("Timeout", static_cast<int>(GetTimeout())));
  SetTryAgent(Storage->ReadBool("TryAgent", GetTryAgent()));
  SetAgentFwd(Storage->ReadBool("AgentFwd", GetAgentFwd()));
  SetAuthTIS(Storage->ReadBool("AuthTIS", GetAuthTIS()));
  SetAuthKI(Storage->ReadBool("AuthKI", GetAuthKI()));
  SetAuthKIPassword(Storage->ReadBool("AuthKIPassword", GetAuthKIPassword()));
  // Continue to use setting keys of previous kerberos implementation (vaclav tomec),
  // but fallback to keys of other implementations (official putty and vintela quest putty),
  // to allow imports from all putty versions.
  // Both vaclav tomec and official putty use AuthGSSAPI
  SetAuthGSSAPI(Storage->ReadBool("AuthGSSAPI", Storage->ReadBool("AuthSSPI", GetAuthGSSAPI())));
  SetGSSAPIFwdTGT(Storage->ReadBool("GSSAPIFwdTGT", Storage->ReadBool("GssapiFwd", Storage->ReadBool("SSPIFwdTGT", GetGSSAPIFwdTGT()))));
  // KerbPrincipal was used by Quest PuTTY
  // GSSAPIServerRealm was used by Vaclav Tomec
  SetLogicalHostName(Storage->ReadString("LogicalHostName", Storage->ReadString("GSSAPIServerRealm", Storage->ReadString("KerbPrincipal", GetLogicalHostName()))));
  SetChangeUsername(Storage->ReadBool("ChangeUsername", GetChangeUsername()));
  SetCompression(Storage->ReadBool("Compression", GetCompression()));
  TSshProt ASshProt = static_cast<TSshProt>(Storage->ReadInteger("SshProt", GetSshProt()));
  // Old sessions may contain the values correponding to the fallbacks we used to allow; migrate them
  if (ASshProt == ssh2deprecated)
  {
    ASshProt = ssh2only;
  }
  else if (ASshProt == ssh1deprecated)
  {
    ASshProt = ssh1only;
  }
  SetSshProt(ASshProt);
  SetSsh2DES(Storage->ReadBool("Ssh2DES", GetSsh2DES()));
  SetSshNoUserAuth(Storage->ReadBool("SshNoUserAuth", GetSshNoUserAuth()));
  SetCipherList(Storage->ReadString("Cipher", GetCipherList()));
  SetKexList(Storage->ReadString("KEX", GetKexList()));
  SetHostKeyList(Storage->ReadString("HostKey", GetHostKeyList()));
  SetGssLibList(Storage->ReadString("GSSLibs", GetGssLibList()));
  SetGssLibCustom(Storage->ReadString("GSSCustom", GetGssLibCustom()));
  SetPublicKeyFile(Storage->ReadString("PublicKeyFile", GetPublicKeyFile()));
  SetAddressFamily(static_cast<TAddressFamily>
    (Storage->ReadInteger("AddressFamily", GetAddressFamily())));
  SetRekeyData(Storage->ReadString("RekeyBytes", GetRekeyData()));
  SetRekeyTime(Storage->ReadInteger("RekeyTime", static_cast<int>(GetRekeyTime())));

  if (GetSessionVersion() < ::GetVersionNumber2121())
  {
    SetFSProtocol(TranslateFSProtocolNumber(Storage->ReadInteger("FSProtocol", GetFSProtocol())));
  }
  else
  {
    SetFSProtocol(TranslateFSProtocol(Storage->ReadString("FSProtocol", GetFSProtocolStr())));
  }
  SetLocalDirectory(Storage->ReadString("LocalDirectory", GetLocalDirectory()));
  SetRemoteDirectory(Storage->ReadString("RemoteDirectory", GetRemoteDirectory()));
  SetSynchronizeBrowsing(Storage->ReadBool("SynchronizeBrowsing", GetSynchronizeBrowsing()));
  SetUpdateDirectories(Storage->ReadBool("UpdateDirectories", GetUpdateDirectories()));
  SetCacheDirectories(Storage->ReadBool("CacheDirectories", GetCacheDirectories()));
  SetCacheDirectoryChanges(Storage->ReadBool("CacheDirectoryChanges", GetCacheDirectoryChanges()));
  SetPreserveDirectoryChanges(Storage->ReadBool("PreserveDirectoryChanges", GetPreserveDirectoryChanges()));

  SetResolveSymlinks(Storage->ReadBool("ResolveSymlinks", GetResolveSymlinks()));
  SetFollowDirectorySymlinks(Storage->ReadBool("FollowDirectorySymlinks", GetFollowDirectorySymlinks()));
  SetDSTMode(static_cast<TDSTMode>(Storage->ReadInteger("ConsiderDST", GetDSTMode())));
  SetLockInHome(Storage->ReadBool("LockInHome", GetLockInHome()));
  SetSpecial(Storage->ReadBool("Special", GetSpecial()));
  SetShell(Storage->ReadString("Shell", GetShell()));
  SetClearAliases(Storage->ReadBool("ClearAliases", GetClearAliases()));
  SetUnsetNationalVars(Storage->ReadBool("UnsetNationalVars", GetUnsetNationalVars()));
  SetListingCommand(Storage->ReadString("ListingCommand",
      Storage->ReadBool("AliasGroupList", false) ? UnicodeString("ls -gla") : GetListingCommand()));
  SetIgnoreLsWarnings(Storage->ReadBool("IgnoreLsWarnings", GetIgnoreLsWarnings()));
  SetSCPLsFullTime(static_cast<TAutoSwitch>(Storage->ReadInteger("SCPLsFullTime", GetSCPLsFullTime())));
  SetScp1Compatibility(Storage->ReadBool("Scp1Compatibility", GetScp1Compatibility()));
  SetTimeDifference(TDateTime(Storage->ReadFloat("TimeDifference", GetTimeDifference())));
  SetTimeDifferenceAuto(Storage->ReadBool("TimeDifferenceAuto", (GetTimeDifference() == TDateTime())));
  SetDeleteToRecycleBin(Storage->ReadBool("DeleteToRecycleBin", GetDeleteToRecycleBin()));
  SetOverwrittenToRecycleBin(Storage->ReadBool("OverwrittenToRecycleBin", GetOverwrittenToRecycleBin()));
  SetRecycleBinPath(Storage->ReadString("RecycleBinPath", GetRecycleBinPath()));
  SetPostLoginCommands(Storage->ReadString("PostLoginCommands", GetPostLoginCommands()));

  SetReturnVar(Storage->ReadString("ReturnVar", GetReturnVar()));
  SetExitCode1IsError(Storage->ReadBool("ExitCode1IsError", GetExitCode1IsError()));
  SetEOLType(static_cast<TEOLType>(Storage->ReadInteger("EOLType", GetEOLType())));
  SetTrimVMSVersions(Storage->ReadBool("TrimVMSVersions", GetTrimVMSVersions()));
  SetNotUtf(static_cast<TAutoSwitch>(Storage->ReadInteger("Utf", Storage->ReadInteger("SFTPUtfBug", GetNotUtf()))));
  SetInternalEditorEncoding(Storage->ReadInteger("InternalEditorEncoding", static_cast<int>(GetInternalEditorEncoding())));

  SetS3DefaultRegion(Storage->ReadString("S3DefaultRegion", GetS3DefaultRegion()));

  // PuTTY defaults to TcpNoDelay, but the psftp/pscp ignores this preference, and always set this to off (what is our default too)
  if (!PuttyImport)
  {
    SetTcpNoDelay(Storage->ReadBool("TcpNoDelay", GetTcpNoDelay()));
  }
  SetSendBuf(Storage->ReadInteger("SendBuf", Storage->ReadInteger("SshSendBuf", static_cast<int>(GetSendBuf()))));
  SetSshSimple(Storage->ReadBool("SshSimple", GetSshSimple()));

  SetProxyMethod(static_cast<TProxyMethod>(Storage->ReadInteger("ProxyMethod", ::pmNone)));
  if (GetProxyMethod() != pmSystem)
  {
    SetProxyHost(Storage->ReadString("ProxyHost", GetProxyHost()));
    SetProxyPort(Storage->ReadInteger("ProxyPort", static_cast<int>(GetProxyPort())));
  }
  SetProxyUsername(Storage->ReadString("ProxyUsername", GetProxyUsername()));
  if (Storage->ValueExists("ProxyPassword"))
  {
    // encrypt unencrypted password
    SetProxyPassword(Storage->ReadString("ProxyPassword", ""));
  }
  else
  {
    // load encrypted password
    FProxyPassword = Storage->ReadStringAsBinaryData("ProxyPasswordEnc", FProxyPassword);
  }
  if (GetProxyMethod() == pmCmd)
  {
    SetProxyLocalCommand(Storage->ReadStringRaw("ProxyTelnetCommand", GetProxyLocalCommand()));
  }
  else
  {
    SetProxyTelnetCommand(Storage->ReadStringRaw("ProxyTelnetCommand", GetProxyTelnetCommand()));
  }
  SetProxyDNS(static_cast<TAutoSwitch>((Storage->ReadInteger("ProxyDNS", (GetProxyDNS() + 2) % 3) + 1) % 3));
  SetProxyLocalhost(Storage->ReadBool("ProxyLocalhost", GetProxyLocalhost()));

#define READ_BUG(BUG) \
    SetBug(sb##BUG, TAutoSwitch(2 - Storage->ReadInteger(MB_TEXT("Bug"#BUG), \
      2 - GetBug(sb##BUG))));
  READ_BUG(Ignore1);
  READ_BUG(PlainPW1);
  READ_BUG(RSA1);
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

  SetSftpServer(Storage->ReadString("SftpServer", GetSftpServer()));
#define READ_SFTP_BUG(BUG) \
    SetSFTPBug(sb##BUG, TAutoSwitch(Storage->ReadInteger(MB_TEXT("SFTP" #BUG "Bug"), GetSFTPBug(sb##BUG))));
  READ_SFTP_BUG(Symlink);
  READ_SFTP_BUG(SignedTS);
#undef READ_SFTP_BUG

  SetSFTPMaxVersion(Storage->ReadInteger("SFTPMaxVersion", static_cast<int>(GetSFTPMaxVersion())));
  SetSFTPMinPacketSize(Storage->ReadInteger("SFTPMinPacketSize", static_cast<int>(GetSFTPMinPacketSize())));
  SetSFTPMaxPacketSize(Storage->ReadInteger("SFTPMaxPacketSize", static_cast<int>(GetSFTPMaxPacketSize())));
  SetSFTPDownloadQueue(Storage->ReadInteger("SFTPDownloadQueue", static_cast<int>(GetSFTPDownloadQueue())));
  SetSFTPUploadQueue(Storage->ReadInteger("SFTPUploadQueue", static_cast<int>(GetSFTPUploadQueue())));
  SetSFTPListingQueue(Storage->ReadInteger("SFTPListingQueue", static_cast<int>(GetSFTPListingQueue())));

  SetColor(Storage->ReadInteger("Color", static_cast<int>(GetColor())));

  SetPuttyProtocol(Storage->ReadString("Protocol", GetPuttyProtocol()));

  SetTunnel(Storage->ReadBool("Tunnel", GetTunnel()));
  SetTunnelPortNumber(Storage->ReadInteger("TunnelPortNumber", static_cast<int>(GetTunnelPortNumber())));
  SetTunnelUserName(Storage->ReadString("TunnelUserName", GetTunnelUserName()));
  // must be loaded after TunnelUserName,
  // because TunnelHostName may be in format user@host
  SetTunnelHostName(Storage->ReadString("TunnelHostName", GetTunnelHostName()));
  if (!GetConfiguration()->GetDisablePasswordStoring())
  {
    if (Storage->ValueExists("TunnelPasswordPlain"))
    {
      SetTunnelPassword(Storage->ReadString("TunnelPasswordPlain", GetTunnelPassword()));
      RewritePassword = true;
    }
    else
    {
      FTunnelPassword = Storage->ReadStringAsBinaryData("TunnelPassword", FTunnelPassword);
    }
  }
  SetTunnelPublicKeyFile(Storage->ReadString("TunnelPublicKeyFile", GetTunnelPublicKeyFile()));
  SetTunnelLocalPortNumber(Storage->ReadInteger("TunnelLocalPortNumber", static_cast<int>(GetTunnelLocalPortNumber())));
  SetTunnelHostKey(Storage->ReadString("TunnelHostKey", GetTunnelHostKey()));

  // Ftp prefix
  SetFtpPasvMode(Storage->ReadBool("FtpPasvMode", GetFtpPasvMode()));
  SetFtpForcePasvIp(static_cast<TAutoSwitch>(Storage->ReadInteger("FtpForcePasvIp2", GetFtpForcePasvIp())));
  SetFtpUseMlsd(static_cast<TAutoSwitch>(Storage->ReadInteger("FtpUseMlsd", GetFtpUseMlsd())));
  SetFtpAccount(Storage->ReadString("FtpAccount", GetFtpAccount()));
  SetFtpPingInterval(Storage->ReadInteger("FtpPingInterval", static_cast<int>(GetFtpPingInterval())));
  SetFtpPingType(static_cast<TPingType>(Storage->ReadInteger("FtpPingType", GetFtpPingType())));
  SetFtpTransferActiveImmediately(static_cast<TAutoSwitch>(Storage->ReadInteger("FtpTransferActiveImmediately2", GetFtpTransferActiveImmediately())));
  SetFtps(static_cast<TFtps>(Storage->ReadInteger("Ftps", GetFtps())));
  SetFtpListAll(static_cast<TAutoSwitch>(Storage->ReadInteger("FtpListAll", GetFtpListAll())));
  SetFtpHost(static_cast<TAutoSwitch>(Storage->ReadInteger("FtpHost", GetFtpHost())));
  SetFtpDeleteFromCwd(static_cast<TAutoSwitch>(Storage->ReadInteger("FtpDeleteFromCwd", GetFtpDeleteFromCwd())));
  SetSslSessionReuse(Storage->ReadBool("SslSessionReuse", GetSslSessionReuse()));
  SetTlsCertificateFile(Storage->ReadString("TlsCertificateFile", GetTlsCertificateFile()));

  SetFtpProxyLogonType(Storage->ReadInteger("FtpProxyLogonType", static_cast<int>(GetFtpProxyLogonType())));

  SetMinTlsVersion(static_cast<TTlsVersion>(Storage->ReadInteger("MinTlsVersion", GetMinTlsVersion())));
  SetMaxTlsVersion(static_cast<TTlsVersion>(Storage->ReadInteger("MaxTlsVersion", GetMaxTlsVersion())));

  if (Storage->ValueExists("EncryptKeyPlain"))
  {
    EncryptKey = Storage->ReadString("EncryptKeyPlain", EncryptKey);
    RewritePassword = true;
  }
  else
  {
    FEncryptKey = Storage->ReadStringAsBinaryData("EncryptKey", FEncryptKey);
  }

#if 0
  IsWorkspace = Storage->ReadBool(L"IsWorkspace", IsWorkspace);
  Link = Storage->ReadString(L"Link", Link);
#endif // #if 0

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

#ifdef TEST
#define KEX_TEST(VALUE, EXPECTED) KexList = VALUE; DebugAssert(KexList == EXPECTED);
#define KEX_DEFAULT L"ecdh,dh-gex-sha1,dh-group14-sha1,rsa,WARN,dh-group1-sha1"
  // Empty source should result in default list
  KEX_TEST(L"", KEX_DEFAULT);
  // Default of pre 5.8.1
  KEX_TEST(L"dh-gex-sha1,dh-group14-sha1,dh-group1-sha1,rsa,WARN", L"ecdh,dh-gex-sha1,dh-group14-sha1,dh-group1-sha1,rsa,WARN");
  // Missing first two priority algos, and last non-priority algo
  KEX_TEST(L"dh-group14-sha1,dh-group1-sha1,WARN", L"ecdh,dh-gex-sha1,dh-group14-sha1,dh-group1-sha1,rsa,WARN");
  // Missing first two priority algos, last non-priority algo and WARN
  KEX_TEST(L"dh-group14-sha1,dh-group1-sha1", L"ecdh,dh-gex-sha1,dh-group14-sha1,dh-group1-sha1,rsa,WARN");
  // Old algos, with all but the first below WARN
  KEX_TEST(L"dh-gex-sha1,WARN,dh-group14-sha1,dh-group1-sha1,rsa", L"ecdh,dh-gex-sha1,WARN,dh-group14-sha1,dh-group1-sha1,rsa");
  // Unknown algo at front
  KEX_TEST(L"unknown,ecdh,dh-gex-sha1,dh-group14-sha1,rsa,WARN,dh-group1-sha1", KEX_DEFAULT);
  // Unknown algo at back
  KEX_TEST(L"ecdh,dh-gex-sha1,dh-group14-sha1,rsa,WARN,dh-group1-sha1,unknown", KEX_DEFAULT);
  // Unknown algo in the middle
  KEX_TEST(L"ecdh,dh-gex-sha1,dh-group14-sha1,unknown,rsa,WARN,dh-group1-sha1", KEX_DEFAULT);
#undef KEX_DEFAULT
#undef KEX_TEST

#define CIPHER_TEST(VALUE, EXPECTED) CipherList = VALUE; DebugAssert(CipherList == EXPECTED);
#define CIPHER_DEFAULT L"aes,chacha20,blowfish,3des,WARN,arcfour,des"
  // Empty source should result in default list
  CIPHER_TEST(L"", CIPHER_DEFAULT);
  // Default of pre 5.8.1
  CIPHER_TEST(L"aes,blowfish,3des,WARN,arcfour,des", L"aes,blowfish,3des,chacha20,WARN,arcfour,des");
  // Missing priority algo
  CIPHER_TEST(L"chacha20,blowfish,3des,WARN,arcfour,des", CIPHER_DEFAULT);
  // Missing non-priority algo
  CIPHER_TEST(L"aes,chacha20,3des,WARN,arcfour,des", L"aes,chacha20,3des,blowfish,WARN,arcfour,des");
  // Missing last warn algo
  CIPHER_TEST(L"aes,blowfish,chacha20,3des,WARN,arcfour", L"aes,blowfish,chacha20,3des,WARN,arcfour,des");
  // Missing first warn algo
  CIPHER_TEST(L"aes,blowfish,chacha20,3des,WARN,des", L"aes,blowfish,chacha20,3des,WARN,des,arcfour");
#undef CIPHER_DEFAULT
#undef CIPHER_TEST

  #define HOSTKEY_TEST(VALUE, EXPECTED) HostKeyList = VALUE; DebugAssert(HostKeyList == EXPECTED);
  #define HOSTKEY_DEFAULT L"ed25519,ecdsa,rsa,dsa,WARN"
  // Empty source should result in default list
  HOSTKEY_TEST(L"", HOSTKEY_DEFAULT);
  // Missing priority algo
  HOSTKEY_TEST(L"ecdsa,rsa,dsa,WARN", HOSTKEY_DEFAULT);
  // Missing non-priority algo
  HOSTKEY_TEST(L"ed25519,ecdsa,dsa,WARN", L"ed25519,ecdsa,dsa,rsa,WARN");
  #undef HOSTKEY_DEFAULT
  #undef HOSTKEY_TEST
#endif
}
//---------------------------------------------------------------------
void TSessionData::Load(THierarchicalStorage *Storage, bool PuttyImport)
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

    DoLoad(Storage, PuttyImport, RewritePassword);

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
        Storage->DeleteValue("PasswordPlain");
        if (!GetPassword().IsEmpty())
        {
          Storage->WriteBinaryDataAsString("Password", FPassword);
        }
        Storage->DeleteValue("TunnelPasswordPlain");
        if (!GetTunnelPassword().IsEmpty())
        {
          Storage->WriteBinaryDataAsString("TunnelPassword", FTunnelPassword);
        }
        Storage->DeleteValue("EncryptKeyPlain");
        if (!EncryptKey().IsEmpty())
        {
          Storage->WriteBinaryDataAsString("EncryptKey", FEncryptKey);
        }
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
//---------------------------------------------------------------------------
void TSessionData::DoSave(THierarchicalStorage *Storage,
  bool PuttyExport, const TSessionData *Default, bool DoNotEncryptPasswords)
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

  Storage->WriteString("Version", ::VersionNumberToStr(::GetCurrentVersionNumber()));
  WRITE_DATA(String, HostName);
  WRITE_DATA2(Integer, PortNumber);
  WRITE_DATA_EX(Integer, "PingInterval", GetPingInterval() / SecsPerMin, nb::ToInt);
  WRITE_DATA_EX(Integer, "PingIntervalSecs", GetPingInterval() % SecsPerMin, );
  Storage->DeleteValue("PingIntervalSec"); // obsolete
  WRITE_DATA(Integer, PingType);
  WRITE_DATA2(Integer, Timeout);
  WRITE_DATA(Bool, TryAgent);
  WRITE_DATA(Bool, AgentFwd);
  WRITE_DATA(Bool, AuthTIS);
  WRITE_DATA(Bool, AuthKI);
  WRITE_DATA(Bool, AuthKIPassword);
  WRITE_DATA(String, Note);

  WRITE_DATA(Bool, AuthGSSAPI);
  WRITE_DATA(Bool, GSSAPIFwdTGT);
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
  WRITE_DATA(Integer, SshProt);
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
  }
  else
  {
    WRITE_DATA_EX(String, "UserName", SessionGetUserName(), );
    WRITE_DATA(String, PublicKeyFile);
    WRITE_DATA_EX2(String, "FSProtocol", GetFSProtocolStr(), );
    WRITE_DATA(String, LocalDirectory);
    WRITE_DATA(String, RemoteDirectory);
    WRITE_DATA(Bool, SynchronizeBrowsing);
    WRITE_DATA(Bool, UpdateDirectories);
    WRITE_DATA(Bool, CacheDirectories);
    WRITE_DATA(Bool, CacheDirectoryChanges);
    WRITE_DATA(Bool, PreserveDirectoryChanges);

    WRITE_DATA(Bool, ResolveSymlinks);
    WRITE_DATA(Bool, FollowDirectorySymlinks);
    WRITE_DATA_EX(Integer, "ConsiderDST", GetDSTMode(), );
    WRITE_DATA(Bool, LockInHome);
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
    WRITE_DATA(Integer, EOLType);
    WRITE_DATA(Bool, TrimVMSVersions);
    Storage->DeleteValue("SFTPUtfBug");
    WRITE_DATA_EX(Integer, "Utf", GetNotUtf(), );
    WRITE_DATA2(Integer, InternalEditorEncoding);
    WRITE_DATA(String, S3DefaultRegion);
    WRITE_DATA2(Integer, SendBuf);
    WRITE_DATA(Bool, SshSimple);
  }

  WRITE_DATA(Integer, ProxyMethod);
  if (GetProxyMethod() != pmSystem)
  {
    WRITE_DATA(String, ProxyHost);
    WRITE_DATA2(Integer, ProxyPort);
  }
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
  WRITE_BUG(Ignore1);
  WRITE_BUG(PlainPW1);
  WRITE_BUG(RSA1);
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

    WRITE_DATA2(Integer, Color);

    WRITE_DATA(Bool, Tunnel);
    WRITE_DATA(String, TunnelHostName);
    WRITE_DATA2(Integer, TunnelPortNumber);
    WRITE_DATA(String, TunnelUserName);
    WRITE_DATA(String, TunnelPublicKeyFile);
    WRITE_DATA2(Integer, TunnelLocalPortNumber);

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
    WRITE_DATA(Integer, FtpDeleteFromCwd);
    WRITE_DATA(Bool, SslSessionReuse);
    WRITE_DATA(String, TlsCertificateFile);

    WRITE_DATA2(Integer, FtpProxyLogonType);

    WRITE_DATA(Integer, MinTlsVersion);
    WRITE_DATA(Integer, MaxTlsVersion);

#if 0
    WRITE_DATA(Bool, IsWorkspace);
    WRITE_DATA(String, Link);
#endif // #if 0

    WRITE_DATA(String, CustomParam1);
    WRITE_DATA(String, CustomParam2);
    WRITE_DATA(String, NameOverride);

    if (!GetCodePage().IsEmpty())
    {
      WRITE_DATA_EX(String, "CodePage", GetCodePage(), );
    }
    WRITE_DATA_EX(Integer, "LoginType", GetLoginType(), );
    WRITE_DATA_EX(Bool, "FtpAllowEmptyPassword", GetFtpAllowEmptyPassword(), );
  }

  SavePasswords(Storage, PuttyExport, DoNotEncryptPasswords);
}
//---------------------------------------------------------------------
TStrings *TSessionData::SaveToOptions(const TSessionData * /*Default*/)
{
  TODO("implement");
#if 0
  std::unique_ptr<TStringList> Options(std::make_unique<TStringList>());
  std::unique_ptr<TOptionsStorage> OptionsStorage(std::make_unique<TOptionsStorage>(Options.get(), true, false));
  DoSave(OptionsStorage.get(), false, Default, true);
  return Options.release();
#endif
  return nullptr;
}
//---------------------------------------------------------------------
void TSessionData::Save(THierarchicalStorage *Storage,
  bool PuttyExport, const TSessionData *Default)
{
  if (Storage->OpenSubKey(GetInternalStorageKey(), true))
  {
    DoSave(Storage, PuttyExport, Default, false);

    Storage->CloseSubKey();
  }
}
//---------------------------------------------------------------------
#if 0
UnicodeString TSessionData::ReadXmlNode(_di_IXMLNode Node, UnicodeString Name, UnicodeString Default)
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
//---------------------------------------------------------------------
intptr_t TSessionData::ReadXmlNode(_di_IXMLNode Node, UnicodeString Name, int Default)
{
  _di_IXMLNode TheNode = Node->ChildNodes->FindNode(Name);
  intptr_t Result;
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
//---------------------------------------------------------------------
_di_IXMLNode TSessionData::FindSettingsNode(_di_IXMLNode Node, UnicodeString & Name)
{
  for (int Index = 0; Index < Node->ChildNodes->Count; Index++)
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
//---------------------------------------------------------------------
UnicodeString TSessionData::ReadSettingsNode(_di_IXMLNode Node, UnicodeString & Name, UnicodeString & Default)
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
//---------------------------------------------------------------------
int TSessionData::ReadSettingsNode(_di_IXMLNode Node, UnicodeString & Name, int Default)
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
//---------------------------------------------------------------------
void TSessionData::ImportFromFilezilla(
  _di_IXMLNode Node, UnicodeString Path, _di_IXMLNode SettingsNode)
{
  Name = UnixIncludeTrailingBackslash(Path) + MakeValidName(ReadXmlNode(Node, L"Name", Name));
  HostName = ReadXmlNode(Node, L"Host", HostName);
  PortNumber = ReadXmlNode(Node, L"Port", PortNumber);

  intptr_t AProtocol = ReadXmlNode(Node, L"Protocol", 0);
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
  intptr_t LogonType = ReadXmlNode(Node, L"Logontype", 0);
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

  intptr_t DefaultTimeDifference = TimeToSeconds(TimeDifference);
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
    intptr_t PrefixSize = StrToIntDef(CutToChar(RemoteDir, L' ', false), 0); // prefix size
    if (PrefixSize > 0)
    {
      RemoteDir.Delete(1, PrefixSize);
    }
    RemoteDirectory = L"/";
    while (!RemoteDir.IsEmpty())
    {
      intptr_t SegmentSize = StrToIntDef(CutToChar(RemoteDir, L' ', false), 0);
      UnicodeString Segment = RemoteDir.SubString(1, SegmentSize);
      RemoteDirectory = UnixIncludeTrailingBackslash(RemoteDirectory) + Segment;
      RemoteDir.Delete(1, SegmentSize + 1);
    }
  }

  SynchronizeBrowsing = (ReadXmlNode(Node, L"SyncBrowsing", SynchronizeBrowsing ? 1 : 0) != 0);

  if (SettingsNode != nullptr)
  {
    if (UsesSsh)
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
      intptr_t FtpProxyType = ReadSettingsNode(SettingsNode, L"FTP Proxy type", -1);
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
        intptr_t ProxyType = ReadSettingsNode(SettingsNode, L"Proxy type", -1);
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
//---------------------------------------------------------------------
void TSessionData::SavePasswords(THierarchicalStorage *Storage, bool PuttyExport, bool DoNotEncryptPasswords)
{
  if (!GetConfiguration()->GetDisablePasswordStoring() && !PuttyExport && !FPassword.IsEmpty())
  {
    // DoNotEncryptPasswords is set when called from GenerateOpenCommandArgs only
    // and it never saves session password
    DebugAssert(!DoNotEncryptPasswords);

    Storage->WriteBinaryDataAsString("Password", StronglyRecryptPassword(FPassword, SessionGetUserName() + GetHostName()));
  }
  else
  {
    Storage->DeleteValue("Password");
  }
  Storage->DeleteValue("PasswordPlain");

  if (PuttyExport)
  {
    // save password unencrypted
    Storage->WriteString("ProxyPassword", GetProxyPassword());
  }
  else
  {
    if (DoNotEncryptPasswords)
    {
      if (!FProxyPassword.IsEmpty())
      {
        Storage->WriteString("ProxyPassword", FProxyPassword);
      }
      else
      {
        Storage->DeleteValue("ProxyPassword");
      }
      Storage->DeleteValue("ProxyPasswordEnc");
    }
    else
    {
      // save password encrypted
      if (!FProxyPassword.IsEmpty())
      {
        Storage->WriteBinaryDataAsString("ProxyPasswordEnc", StronglyRecryptPassword(FProxyPassword, GetProxyUsername() + GetProxyHost()));
      }
      else
      {
        Storage->DeleteValue("ProxyPasswordEnc");
      }
      Storage->DeleteValue("ProxyPassword");
    }

    if (DoNotEncryptPasswords)
    {
      if (!FTunnelPassword.IsEmpty())
      {
        Storage->WriteString("TunnelPasswordPlain", GetTunnelPassword());
      }
      else
      {
        Storage->DeleteValue("TunnelPasswordPlain");
      }
    }
    else
    {
      if (!GetConfiguration()->GetDisablePasswordStoring() && !FTunnelPassword.IsEmpty())
      {
        Storage->WriteBinaryDataAsString("TunnelPassword", StronglyRecryptPassword(FTunnelPassword, GetTunnelUserName() + GetTunnelHostName()));
      }
      else
      {
        Storage->DeleteValue("TunnelPassword");
      }
    }

    if (DoNotEncryptPasswords)
    {
      if (!FEncryptKey.IsEmpty())
      {
        Storage->WriteString(L"EncryptKeyPlain", EncryptKey);
      }
      else
      {
        Storage->DeleteValue(L"EncryptKeyPlain");
      }
      Storage->DeleteValue(L"EncryptKey");
    }
    else
    {
      if (!FEncryptKey.IsEmpty())
      {
        Storage->WriteBinaryDataAsString(L"EncryptKey", StronglyRecryptPassword(FEncryptKey, UserName() + HostName()));
      }
      else
      {
        Storage->DeleteValue("EncryptKey");
      }
      Storage->DeleteValue("EncryptKeyPlain");
    }
  }
}
//---------------------------------------------------------------------
void TSessionData::RecryptPasswords()
{
  SetPassword(GetPassword());
  SetNewPassword(GetNewPassword());
  SetProxyPassword(GetProxyPassword());
  SetTunnelPassword(GetTunnelPassword());
  SetPassphrase(GetPassphrase());
  SetEncryptKey(GetEncryptKey());
}
//---------------------------------------------------------------------
bool TSessionData::HasPassword() const
{
  return !FPassword.IsEmpty();
}
//---------------------------------------------------------------------
bool TSessionData::HasAnySessionPassword() const
{
  return HasPassword() || !FTunnelPassword.IsEmpty();
}
//---------------------------------------------------------------------
bool TSessionData::HasAnyPassword() const
{
  return
    HasAnySessionPassword() ||
    !FProxyPassword.IsEmpty() ||
    // will probably be never used
    !FNewPassword.IsEmpty();
}
//---------------------------------------------------------------------
void TSessionData::ClearSessionPasswords()
{
  FPassword.Clear();
  FNewPassword.Clear();
  FTunnelPassword.Clear();
}
//---------------------------------------------------------------------
void TSessionData::Modify()
{
  FModified = true;
  if (FSource == ssStored)
  {
    FSource = ssStoredModified;
  }
}
//---------------------------------------------------------------------
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
//---------------------------------------------------------------------
void TSessionData::SaveRecryptedPasswords(THierarchicalStorage *Storage)
{
  if (Storage->OpenSubKey(GetInternalStorageKey(), true))
  {
    try__finally
    {
      RecryptPasswords();

      SavePasswords(Storage, false, false);
    },
    __finally
    {
      Storage->CloseSubKey();
    } end_try__finally
  }
}
//---------------------------------------------------------------------
void TSessionData::Remove(THierarchicalStorage * Storage, UnicodeString Name)
{
  Storage->RecursiveDeleteSubKey(Name);
}
//---------------------------------------------------------------------
void TSessionData::Remove()
{
  bool SessionList = true;
  std::unique_ptr<THierarchicalStorage> Storage(GetConfiguration()->CreateStorage(SessionList));
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
//---------------------------------------------------------------------
void TSessionData::CacheHostKeyIfNotCached()
{
  UnicodeString KeyType = GetKeyTypeFromFingerprint(GetHostKey());

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
      // fingerprint is MD5 of host key, so it cannot be translated back to host key,
      // so we store fingerprint and TSecureShell::VerifyHostKey was
      // modified to accept also fingerprint
      Storage->WriteString(HostKeyName, GetHostKey());
    }
  }
}
//---------------------------------------------------------------------
inline void MoveStr(UnicodeString &Source, UnicodeString *Dest, intptr_t Count)
{
  if (Dest != nullptr)
  {
    (*Dest) += Source.SubString(1, Count);
  }

  Source.Delete(1, Count);
}
//---------------------------------------------------------------------
bool TSessionData::DoIsProtocolUrl(
  UnicodeString Url, UnicodeString Protocol, intptr_t &ProtocolLen)
{
  bool Result = ::SameText(Url.SubString(1, Protocol.Length() + 1), Protocol + L":");
  if (Result)
  {
    ProtocolLen = Protocol.Length() + 1;
  }
  return Result;
}
//---------------------------------------------------------------------
bool TSessionData::IsProtocolUrl(
  UnicodeString Url, UnicodeString Protocol, intptr_t &ProtocolLen)
{
  return
    DoIsProtocolUrl(Url, Protocol, ProtocolLen) ||
    DoIsProtocolUrl(Url, WinSCPProtocolPrefix + Protocol, ProtocolLen);
}
//---------------------------------------------------------------------
bool TSessionData::IsSensitiveOption(UnicodeString Option)
{
  return
    ::SameText(Option, PassphraseOption) ||
    ::SameText(Option, NEWPASSWORD_SWITCH);
}
//---------------------------------------------------------------------
bool TSessionData::IsOptionWithParameters(UnicodeString AOption)
{
  return SameText(AOption, RawSettingsOption);
}
//---------------------------------------------------------------------
bool TSessionData::MaskPasswordInOptionParameter(UnicodeString AOption, UnicodeString &AParam)
{
  bool Result = false;
  if (SameText(AOption, RawSettingsOption))
  {
    intptr_t P = AParam.Pos(L"=");
    if (P > 0)
    {
      // TStrings.IndexOfName does not trim
      UnicodeString Key = AParam.SubString(1, P - 1);

      if (SameText(Key, L"ProxyPassword") ||
          SameText(Key, L"ProxyPasswordEnc") ||
          SameText(Key, L"TunnelPassword") ||
          SameText(Key, L"TunnelPasswordPlain") ||
          SameText(Key, L"EncryptKey"))
      {
        AParam = Key + L"=" + PasswordMask;
        Result = true;
      }
    }
  }
  return Result;
}
//---------------------------------------------------------------------
void TSessionData::MaskPasswords()
{
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
//---------------------------------------------------------------------
bool TSessionData::ParseUrl(UnicodeString Url, TOptions *Options,
  TStoredSessionList *AStoredSessions, bool &DefaultsOnly, UnicodeString *AFileName,
  bool * AProtocolDefined, UnicodeString * MaskedUrl, intptr_t Flags)
{
  bool ProtocolDefined = false;
  bool PortNumberDefined = false;
  TFSProtocol AFSProtocol = fsSCPonly;
  intptr_t APortNumber = 0;
  TFtps AFtps = ftpsNone;
  intptr_t ProtocolLen = 0;
  if (Url.SubString(1, 7).LowerCase() == "netbox:")
  {
    // Remove "netbox:" prefix
    Url.Delete(1, 7);
    if (Url.SubString(1, 2) == L"//")
    {
      // Remove "//"
      Url.Delete(1, 2);
    }
  }
  if (Url.SubString(1, 7).LowerCase() == "webdav:")
  {
    AFSProtocol = fsWebDAV;
    AFtps = ftpsNone;
    APortNumber = HTTPPortNumber;
    Url.Delete(1, 7);
    ProtocolDefined = true;
  }
  if (IsProtocolUrl(Url, ScpProtocol, ProtocolLen))
  {
    AFSProtocol = fsSCPonly;
    APortNumber = SshPortNumber;
    MoveStr(Url, MaskedUrl, ProtocolLen);
    ProtocolDefined = true;
  }
  else if (IsProtocolUrl(Url, SftpProtocol, ProtocolLen))
  {
    AFSProtocol = fsSFTPonly;
    APortNumber = SshPortNumber;
    MoveStr(Url, MaskedUrl, ProtocolLen);
    ProtocolDefined = true;
  }
  else if (IsProtocolUrl(Url, FtpProtocol, ProtocolLen))
  {
    AFSProtocol = fsFTP;
    SetFtps(ftpsNone);
    APortNumber = FtpPortNumber;
    MoveStr(Url, MaskedUrl, ProtocolLen);
    ProtocolDefined = true;
  }
  else if (IsProtocolUrl(Url, FtpsProtocol, ProtocolLen))
  {
    AFSProtocol = fsFTP;
    AFtps = ftpsImplicit;
    APortNumber = FtpsImplicitPortNumber;
    MoveStr(Url, MaskedUrl, ProtocolLen);
    ProtocolDefined = true;
  }
  else if (IsProtocolUrl(Url, FtpesProtocol, ProtocolLen))
  {
    AFSProtocol = fsFTP;
    AFtps = ftpsExplicitTls;
    APortNumber = FtpPortNumber;
    MoveStr(Url, MaskedUrl, ProtocolLen);
    ProtocolDefined = true;
  }
  else if (IsProtocolUrl(Url, WebDAVProtocol, ProtocolLen) ||
           IsProtocolUrl(Url, HttpProtocol, ProtocolLen))
  {
    AFSProtocol = fsWebDAV;
    AFtps = ftpsNone;
    APortNumber = HTTPPortNumber;
    MoveStr(Url, MaskedUrl, ProtocolLen);
    ProtocolDefined = true;
  }
  else if (IsProtocolUrl(Url, WebDAVSProtocol, ProtocolLen) ||
           IsProtocolUrl(Url, HttpsProtocol, ProtocolLen))
  {
    AFSProtocol = fsWebDAV;
    AFtps = ftpsImplicit;
    APortNumber = HTTPSPortNumber;
    MoveStr(Url, MaskedUrl, ProtocolLen);
    ProtocolDefined = true;
  }
  else if (IsProtocolUrl(Url, S3Protocol, ProtocolLen))
  {
    AFSProtocol = fsS3;
    AFtps = ftpsImplicit;
    APortNumber = HTTPSPortNumber;
    MoveStr(Url, MaskedUrl, ProtocolLen);
    ProtocolDefined = true;
  }
  else if (IsProtocolUrl(Url, SshProtocol, ProtocolLen))
  {
    // For most uses, handling ssh:// the same way as sftp://
    // The only place where a difference is made is GetLoginData() in WinMain.cpp
    AFSProtocol = fsSFTPonly;
    SetPuttyProtocol(PuttySshProtocol);
    APortNumber = SshPortNumber;
    MoveStr(Url, MaskedUrl, ProtocolLen);
    ProtocolDefined = true;
  }

  if (ProtocolDefined && (Url.SubString(1, 2) == "//"))
  {
    MoveStr(Url, MaskedUrl, 2);
  }

  if (AProtocolDefined != nullptr)
  {
    *AProtocolDefined = ProtocolDefined;
  }

  if (!Url.IsEmpty())
  {
    UnicodeString DecodedUrl = DecodeUrlChars(Url);
    // lookup stored session even if protocol was defined
    // (this allows setting for example default username for host
    // by creating stored session named by host)
    TSessionData *Data = nullptr;
    // When using to paste URL on Login dialog, we do not want to lookup the stored sites
    if ((StoredSessions != nullptr) &&
        (!ProtocolDefined || FLAGSET(Flags, pufAllowStoredSiteWithProtocol)))
    {
      // this can be optimized as the list is sorted
      for (Integer Index = 0; Index < AStoredSessions->GetCountIncludingHidden(); ++Index)
      {
        TSessionData *AData = AStoredSessions->GetAs<TSessionData>(Index);
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

    UnicodeString RemoteDirectory;

    if (Data != nullptr)
    {
      Assign(Data);
      RemoteDirectory = Url.SubString(Data->GetName().Length() + 1);
      if (RemoteDirectory.Length() > 0 && RemoteDirectory[1] == ':')
        RemoteDirectory.Delete(1, 1);

      if (Data->GetHidden())
      {
        Data->Remove();
        AStoredSessions->Remove(Data);
        // only modified, implicit
        AStoredSessions->Save(false, false);
      }

      if (MaskedUrl != nullptr)
      {
        (*MaskedUrl) += Url;
      }
    }
    else
    {
      // This happens when pasting URL on Login dialog
      if (AStoredSessions != nullptr)
      {
        CopyData(AStoredSessions->GetDefaultSettings());
#if 0
        SetUserName(ANONYMOUS_USER_NAME);
        SetLoginType(ltAnonymous);
#endif // #if 0
      }
      SetName(L"");

      intptr_t PSlash = Url.Pos(L"/");
      if (PSlash == 0)
      {
        PSlash = Url.Length() + 1;
      }

      UnicodeString ConnectInfo = Url.SubString(1, PSlash - 1);

      intptr_t P = ConnectInfo.LastDelimiter(L"@");

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
        SetPortNumber(::StrToIntDef(DecodeUrlChars(HostInfo), -1));
        PortNumberDefined = true;
      }
      else if (ProtocolDefined)
      {
        SetPortNumber(APortNumber);
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
          UnicodeString Name = RightStr(ConnectionParamName, ConnectionParamName.Length() - UrlRawSettingsParamNamePrefix.Length());
          Name = DecodeUrlChars(Name);
          RawSettings->SetValue(Name, DecodeUrlChars(ConnectionParam));
        }
      }

      if (RawSettings->Count > 0) // optimization
      {
        ApplyRawSettings(RawSettings.get());
      }

      bool HasPassword = (UserInfo.Pos(L':') > 0);
      UnicodeString RawUserName = CutToChar(UserInfo, L':', false);
      if (!RawUserName.IsEmpty())
        SessionSetUserName(DecodeUrlChars(RawUserName));

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

    if (!RemoteDirectory.IsEmpty() && (RemoteDirectory != ROOTDIRECTORY))
    {
      if ((RemoteDirectory[RemoteDirectory.Length()] != L'/') &&
          (AFileName != nullptr))
      {
        *AFileName = DecodeUrlChars(base::UnixExtractFileName(RemoteDirectory));
        RemoteDirectory = base::UnixExtractFilePath(RemoteDirectory);
      }
      SetRemoteDirectory(DecodeUrlChars(RemoteDirectory));
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
    if (Options->FindSwitch(SESSIONNAME_SWITCH, Value))
    {
      SetName(Value);
    }
    if (Options->FindSwitch(NEWPASSWORD_SWITCH, Value))
    {
      SetChangePassword(true);
      SetNewPassword(Value);
    }
    if (Options->FindSwitch(L"privatekey", Value))
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
        ApplyRawSettings(RawSettings.get());
    }
    if (Options->FindSwitch("allowemptypassword", Value))
    {
      SetFtpAllowEmptyPassword((::StrToIntDef(Value, 0) != 0));
    }
    if (Options->FindSwitch("explicitssl", Value))
    {
      bool Enabled = (::StrToIntDef(Value, 1) != 0);
      SetFtps(Enabled ? ftpsExplicitSsl : ftpsNone);
      if (!PortNumberDefined && Enabled)
      {
        SetPortNumber(FtpPortNumber);
      }
    }
    if (Options->FindSwitch("username", Value))
    {
      if (!Value.IsEmpty())
      {
        SessionSetUserName(Value);
      }
    }
    if (Options->FindSwitch("password", Value))
    {
      SetPassword(Value);
    }
    if (Options->FindSwitch("codepage", Value))
    {
      intptr_t CodePage = ::StrToIntDef(Value, 0);
      if (CodePage != 0)
      {
        SetCodePage(GetCodePageAsString(CodePage));
      }
    }
  }

  return true;
}
//---------------------------------------------------------------------
void TSessionData::ApplyRawSettings(TStrings * RawSettings)
{
#if 0
  std::unique_ptr<TOptionsStorage> OptionsStorage(std::make_unique<TOptionsStorage>(RawSettings, false));
  ApplyRawSettings(OptionsStorage.get());
#endif // #if 0
}
//---------------------------------------------------------------------
void TSessionData::ApplyRawSettings(THierarchicalStorage * Storage)
{
  bool Dummy;
  DoLoad(Storage, false, Dummy);
}
//---------------------------------------------------------------------
void TSessionData::ConfigureTunnel(intptr_t APortNumber)
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
//---------------------------------------------------------------------
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
//---------------------------------------------------------------------
void TSessionData::ExpandEnvironmentVariables()
{
  SetHostName(GetHostNameExpanded());
  SessionSetUserName(GetUserNameExpanded());
  SetPublicKeyFile(::ExpandEnvironmentVariables(GetPublicKeyFile()));
}
//---------------------------------------------------------------------
void TSessionData::ValidatePath(UnicodeString /*APath*/)
{
  // noop
}
//---------------------------------------------------------------------
void TSessionData::ValidateName(UnicodeString Name)
{
  // keep consistent with MakeValidName
  if (Name.LastDelimiter(L"/") > 0)
  {
    throw Exception(FMTLOAD(ITEM_NAME_INVALID, Name, L"/"));
  }
}
//---------------------------------------------------------------------
UnicodeString TSessionData::MakeValidName(UnicodeString Name)
{
  // keep consistent with ValidateName
  return ReplaceStr(Name, L"/", L"\\");
}
//---------------------------------------------------------------------
RawByteString TSessionData::EncryptPassword(UnicodeString Password, UnicodeString Key)
{
  return GetConfiguration()->EncryptPassword(Password, Key);
}
//---------------------------------------------------------------------
RawByteString TSessionData::StronglyRecryptPassword(RawByteString APassword, UnicodeString AKey)
{
  return GetConfiguration()->StronglyRecryptPassword(APassword, AKey);
}
//---------------------------------------------------------------------
UnicodeString TSessionData::DecryptPassword(RawByteString APassword, UnicodeString AKey)
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
//---------------------------------------------------------------------
bool TSessionData::GetCanLogin() const
{
  return !FHostName.IsEmpty();
}
//---------------------------------------------------------------------
UnicodeString TSessionData::GetSessionKey() const
{
  UnicodeString Result = FORMAT("%s@%s", SessionGetUserName(), GetHostName());
  if (GetPortNumber() != GetDefaultPort(GetFSProtocol(), GetFtps()))
  {
    Result += FORMAT(":%d", GetPortNumber());
  }
  return Result;
}
//---------------------------------------------------------------------
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
//---------------------------------------------------------------------
UnicodeString TSessionData::GetStorageKey() const
{
  return GetSessionName();
}
//---------------------------------------------------------------------
UnicodeString TSessionData::FormatSiteKey(UnicodeString HostName, intptr_t PortNumber)
{
  return FORMAT("%s:%d", HostName, nb::ToInt(PortNumber));
}
//---------------------------------------------------------------------
UnicodeString TSessionData::GetSiteKey() const
{
  return FormatSiteKey(GetHostNameExpanded(), GetPortNumber());
}
//---------------------------------------------------------------------
void TSessionData::SetHostName(UnicodeString AValue)
{
  if (FHostName != AValue)
  {
    UnicodeString Value = AValue;
    RemoveProtocolPrefix(Value);
    // remove path
    {
      intptr_t Pos = 1;
      Value = CopyToChars(Value, Pos, L"/", true, nullptr, false);
    }
    // HostName is key for password encryption
    UnicodeString XPassword = GetPassword();
    UnicodeString XNewPassword = FPassword;
    UnicodeString XEncryptKey = EncryptKey;

    // This is now hardly used as hostname is parsed directly on login dialog.
    // But can be used when importing sites from PuTTY, as it allows same format too.
    intptr_t P = Value.LastDelimiter(L"@");
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
//---------------------------------------------------------------------
UnicodeString TSessionData::GetHostNameExpanded() const
{
  return ::ExpandEnvironmentVariables(GetHostName());
}
//---------------------------------------------------------------------
void TSessionData::SetPortNumber(intptr_t Value)
{
  SET_SESSION_PROPERTY(PortNumber);
}
//---------------------------------------------------------------------
void TSessionData::SetShell(UnicodeString Value)
{
  SET_SESSION_PROPERTY(Shell);
}
//---------------------------------------------------------------------
void TSessionData::SetSftpServer(UnicodeString Value)
{
  SET_SESSION_PROPERTY(SftpServer);
}
//---------------------------------------------------------------------
void TSessionData::SetClearAliases(bool Value)
{
  SET_SESSION_PROPERTY(ClearAliases);
}
//---------------------------------------------------------------------
void TSessionData::SetListingCommand(UnicodeString Value)
{
  SET_SESSION_PROPERTY(ListingCommand);
}
//---------------------------------------------------------------------
void TSessionData::SetIgnoreLsWarnings(bool Value)
{
  SET_SESSION_PROPERTY(IgnoreLsWarnings);
}
//---------------------------------------------------------------------
void TSessionData::SetUnsetNationalVars(bool Value)
{
  SET_SESSION_PROPERTY(UnsetNationalVars);
}
//---------------------------------------------------------------------
void TSessionData::SessionSetUserName(UnicodeString Value)
{
  // Avoid password recryption (what may popup master password prompt)
  if (FUserName != Value)
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
//---------------------------------------------------------------------
UnicodeString TSessionData::GetUserNameExpanded() const
{
  return ::ExpandEnvironmentVariables(SessionGetUserName());
}
//---------------------------------------------------------------------
void TSessionData::SetPassword(UnicodeString AValue)
{
  RawByteString Value = EncryptPassword(AValue, SessionGetUserName() + GetHostName());
  SET_SESSION_PROPERTY(Password);
}
//---------------------------------------------------------------------
UnicodeString TSessionData::GetPassword() const
{
  return DecryptPassword(FPassword, SessionGetUserName() + GetHostName());
}
//---------------------------------------------------------------------
void TSessionData::SetNewPassword(UnicodeString AValue)
{
  RawByteString Value = EncryptPassword(AValue, SessionGetUserName() + GetHostName());
  SET_SESSION_PROPERTY(NewPassword);
}
//---------------------------------------------------------------------
UnicodeString TSessionData::GetNewPassword() const
{
  return DecryptPassword(FNewPassword, SessionGetUserName() + GetHostName());
}
//---------------------------------------------------------------------
void TSessionData::SetChangePassword(bool Value)
{
  SET_SESSION_PROPERTY(ChangePassword);
}
//---------------------------------------------------------------------
void TSessionData::SetPingInterval(intptr_t Value)
{
  SET_SESSION_PROPERTY(PingInterval);
}
//---------------------------------------------------------------------
void TSessionData::SetTryAgent(bool Value)
{
  SET_SESSION_PROPERTY(TryAgent);
}
//---------------------------------------------------------------------
void TSessionData::SetAgentFwd(bool Value)
{
  SET_SESSION_PROPERTY(AgentFwd);
}
//---------------------------------------------------------------------
void TSessionData::SetAuthTIS(bool Value)
{
  SET_SESSION_PROPERTY(AuthTIS);
}
//---------------------------------------------------------------------
void TSessionData::SetAuthKI(bool Value)
{
  SET_SESSION_PROPERTY(AuthKI);
}
//---------------------------------------------------------------------
void TSessionData::SetAuthKIPassword(bool Value)
{
  SET_SESSION_PROPERTY(AuthKIPassword);
}
//---------------------------------------------------------------------
void TSessionData::SetAuthGSSAPI(bool Value)
{
  SET_SESSION_PROPERTY(AuthGSSAPI);
}
//---------------------------------------------------------------------
void TSessionData::SetGSSAPIFwdTGT(bool Value)
{
  SET_SESSION_PROPERTY(GSSAPIFwdTGT);
}
//---------------------------------------------------------------------
void TSessionData::SetChangeUsername(bool Value)
{
  SET_SESSION_PROPERTY(ChangeUsername);
}
//---------------------------------------------------------------------
void TSessionData::SetCompression(bool Value)
{
  SET_SESSION_PROPERTY(Compression);
}
//---------------------------------------------------------------------
void TSessionData::SetSshProt(TSshProt Value)
{
  SET_SESSION_PROPERTY(SshProt);
}
//---------------------------------------------------------------------
void TSessionData::SetSsh2DES(bool Value)
{
  SET_SESSION_PROPERTY(Ssh2DES);
}
//---------------------------------------------------------------------
void TSessionData::SetSshNoUserAuth(bool Value)
{
  SET_SESSION_PROPERTY(SshNoUserAuth);
}
//---------------------------------------------------------------------
UnicodeString TSessionData::GetSshProtStr() const
{
  return SshProtList[FSshProt];
}
//---------------------------------------------------------------------
bool TSessionData::GetUsesSsh() const
{
  return GetIsSshProtocol(GetFSProtocol());
}
//---------------------------------------------------------------------
void TSessionData::SetCipher(intptr_t Index, TCipher Value)
{
  DebugAssert(Index >= 0 && Index < CIPHER_COUNT);
  SET_SESSION_PROPERTY(Ciphers[Index]);
}
//---------------------------------------------------------------------
TCipher TSessionData::GetCipher(intptr_t Index) const
{
  DebugAssert(Index >= 0 && Index < CIPHER_COUNT);
  return FCiphers[Index];
}
//---------------------------------------------------------------------
template <class AlgoT>
void TSessionData::SetAlgoList(AlgoT *List, const AlgoT *DefaultList, const UnicodeString *Names,
  intptr_t Count, AlgoT WarnAlgo, UnicodeString AValue)
{
  UnicodeString Value = AValue;

  rde::vector<bool> Used(Count); // initialized to false
  rde::vector<AlgoT> NewList(Count);

  bool HasWarnAlgo = (WarnAlgo >= AlgoT());
  intptr_t WarnDefaultIndex;
  if (!HasWarnAlgo)
  {
    WarnDefaultIndex = -1;
  }
  else
  {
    const AlgoT *WarnPtr = std::find(DefaultList, DefaultList + Count, WarnAlgo);
    DebugAssert(WarnPtr != nullptr);
    WarnDefaultIndex = (WarnPtr - DefaultList);
  }

  intptr_t Index = 0;
  while (!Value.IsEmpty())
  {
    UnicodeString AlgoStr = CutToChar(Value, L',', true);
    for (intptr_t Algo = 0; Algo < Count; ++Algo)
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

  intptr_t WarnIndex = -1;
  if (HasWarnAlgo)
  {
    WarnIndex = std::find(NewList.begin(), NewList.end(), WarnAlgo) - NewList.begin();
  }

  bool Priority = true;
  for (intptr_t DefaultIndex = 0; (DefaultIndex < Count); ++DefaultIndex)
  {
    AlgoT DefaultAlgo = DefaultList[DefaultIndex];
    if (!Used[DefaultAlgo] && DebugAlwaysTrue(Index < Count))
    {
      intptr_t TargetIndex;
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
//---------------------------------------------------------------------
void TSessionData::SetCipherList(UnicodeString Value)
{
  SetAlgoList(FCiphers, DefaultCipherList, CipherNames, CIPHER_COUNT, cipWarn, Value);
}
//---------------------------------------------------------------------
UnicodeString TSessionData::GetCipherList() const
{
  UnicodeString Result;
  for (intptr_t Index = 0; Index < CIPHER_COUNT; ++Index)
  {
    Result += UnicodeString(Index ? "," : "") + CipherNames[GetCipher(Index)];
  }
  return Result;
}
//---------------------------------------------------------------------
void TSessionData::SetKex(intptr_t Index, TKex Value)
{
  DebugAssert(Index >= 0 && Index < KEX_COUNT);
  SET_SESSION_PROPERTY(Kex[Index]);
}
//---------------------------------------------------------------------
TKex TSessionData::GetKex(intptr_t Index) const
{
  DebugAssert(Index >= 0 && Index < KEX_COUNT);
  return FKex[Index];
}
//---------------------------------------------------------------------
void TSessionData::SetKexList(UnicodeString Value)
{
  SetAlgoList(FKex, DefaultKexList, KexNames, KEX_COUNT, kexWarn, Value);
}
//---------------------------------------------------------------------
UnicodeString TSessionData::GetKexList() const
{
  UnicodeString Result;
  for (intptr_t Index = 0; Index < KEX_COUNT; ++Index)
  {
    Result += UnicodeString(Index ? "," : "") + KexNames[GetKex(Index)];
  }
  return Result;
}
//---------------------------------------------------------------------
void TSessionData::SetHostKeys(intptr_t Index, THostKey Value)
{
  DebugAssert(Index >= 0 && Index < HOSTKEY_COUNT);
  SET_SESSION_PROPERTY(HostKeys[Index]);
}
//---------------------------------------------------------------------
THostKey TSessionData::GetHostKeys(intptr_t Index) const
{
  DebugAssert(Index >= 0 && Index < HOSTKEY_COUNT);
  return FHostKeys[Index];
}
//---------------------------------------------------------------------
void TSessionData::SetHostKeyList(UnicodeString Value)
{
  SetAlgoList(FHostKeys, DefaultHostKeyList, HostKeyNames, HOSTKEY_COUNT, hkWarn, Value);
}
//---------------------------------------------------------------------
UnicodeString TSessionData::GetHostKeyList() const
{
  UnicodeString Result;
  for (intptr_t Index = 0; Index < HOSTKEY_COUNT; Index++)
  {
    Result += UnicodeString(Index ? "," : "") + HostKeyNames[FHostKeys[Index]];
  }
  return Result;
}
//---------------------------------------------------------------------
void TSessionData::SetGssLib(intptr_t Index, TGssLib Value)
{
  DebugAssert(Index >= 0 && Index < GSSLIB_COUNT);
  // SET_SESSION_PROPERTY(FGssLib[Index]);
  if (FGssLib[Index] != Value)
  {
    FGssLib[Index] = Value;
  }
  Modify();
}
//---------------------------------------------------------------------
TGssLib TSessionData::GetGssLib(intptr_t Index) const
{
  DebugAssert(Index >= 0 && Index < GSSLIB_COUNT);
  return FGssLib[Index];
}
//---------------------------------------------------------------------
void TSessionData::SetGssLibList(UnicodeString Value)
{
  SetAlgoList(FGssLib, DefaultGssLibList, GssLibNames, GSSLIB_COUNT, TGssLib(-1), Value);
}
//---------------------------------------------------------------------
UnicodeString TSessionData::GetGssLibList() const
{
  UnicodeString Result;
  for (intptr_t Index = 0; Index < GSSLIB_COUNT; Index++)
  {
    Result += UnicodeString(Index ? "," : "") + GssLibNames[GetGssLib(Index)];
  }
  return Result;
}
//---------------------------------------------------------------------
void TSessionData::SetGssLibCustom(UnicodeString Value)
{
  SET_SESSION_PROPERTY(GssLibCustom);
}
//---------------------------------------------------------------------
void TSessionData::SetPublicKeyFile(UnicodeString Value)
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
//---------------------------------------------------------------------
void TSessionData::SetPassphrase(UnicodeString AValue)
{
  RawByteString Value = EncryptPassword(AValue, GetPublicKeyFile());
  SET_SESSION_PROPERTY(Passphrase);
}
//---------------------------------------------------------------------
UnicodeString TSessionData::GetPassphrase() const
{
  return DecryptPassword(FPassphrase, GetPublicKeyFile());
}
//---------------------------------------------------------------------
void TSessionData::SetReturnVar(UnicodeString Value)
{
  SET_SESSION_PROPERTY(ReturnVar);
}
//---------------------------------------------------------------------
void TSessionData::SetExitCode1IsError(bool Value)
{
  SET_SESSION_PROPERTY(ExitCode1IsError);
}
//---------------------------------------------------------------------------
void TSessionData::SetLookupUserGroups(TAutoSwitch Value)
{
  SET_SESSION_PROPERTY(LookupUserGroups);
}
//---------------------------------------------------------------------
void TSessionData::SetEOLType(TEOLType Value)
{
  SET_SESSION_PROPERTY(EOLType);
}
//---------------------------------------------------------------------
void TSessionData::SetTrimVMSVersions(bool Value)
{
  SET_SESSION_PROPERTY(TrimVMSVersions);
}
//---------------------------------------------------------------------
TDateTime TSessionData::GetTimeoutDT() const
{
  return SecToDateTime(GetTimeout());
}
//---------------------------------------------------------------------
void TSessionData::SetTimeout(intptr_t Value)
{
  SET_SESSION_PROPERTY(Timeout);
}
//---------------------------------------------------------------------
void TSessionData::SetFSProtocol(TFSProtocol Value)
{
  SET_SESSION_PROPERTY(FSProtocol);
}
//---------------------------------------------------------------------
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
//---------------------------------------------------------------------
void TSessionData::SetDetectReturnVar(bool Value)
{
  if (Value != GetDetectReturnVar())
  {
    SetReturnVar(Value ? "" : "$?");
  }
}
//---------------------------------------------------------------------
bool TSessionData::GetDetectReturnVar() const
{
  return GetReturnVar().IsEmpty();
}
//---------------------------------------------------------------------
void TSessionData::SetDefaultShell(bool Value)
{
  if (Value != GetDefaultShell())
  {
    SetShell(Value ? "" : "/bin/bash");
  }
}
//---------------------------------------------------------------------
bool TSessionData::GetDefaultShell() const
{
  return GetShell().IsEmpty();
}
//---------------------------------------------------------------------
void TSessionData::SetPuttyProtocol(UnicodeString Value)
{
  SET_SESSION_PROPERTY(PuttyProtocol);
}
//---------------------------------------------------------------------
UnicodeString TSessionData::GetNormalizedPuttyProtocol() const
{
  return DefaultStr(GetPuttyProtocol(), PuttySshProtocol);
}
//---------------------------------------------------------------------
void TSessionData::SetPingIntervalDT(TDateTime Value)
{
  uint16_t hour, min, sec, msec;

  Value.DecodeTime(hour, min, sec, msec);
  SetPingInterval(hour * SecsPerHour + min * SecsPerMin + sec);
}
//---------------------------------------------------------------------
TDateTime TSessionData::GetPingIntervalDT() const
{
  return SecToDateTime(GetPingInterval());
}
//---------------------------------------------------------------------
void TSessionData::SetPingType(TPingType Value)
{
  SET_SESSION_PROPERTY(PingType);
}
//---------------------------------------------------------------------
void TSessionData::SetAddressFamily(TAddressFamily Value)
{
  SET_SESSION_PROPERTY(AddressFamily);
}
//---------------------------------------------------------------------
void TSessionData::SetRekeyData(UnicodeString Value)
{
  SET_SESSION_PROPERTY(RekeyData);
}
//---------------------------------------------------------------------
void TSessionData::SetRekeyTime(uintptr_t Value)
{
  SET_SESSION_PROPERTY(RekeyTime);
}
//---------------------------------------------------------------------
UnicodeString TSessionData::GetDefaultSessionName() const
{
  UnicodeString Result;
  UnicodeString HostName = ::TrimLeft(GetHostName());
  UnicodeString UserName = SessionGetUserName();
  RemoveProtocolPrefix(HostName);
  // remove path
  {
    intptr_t Pos = 1;
    HostName = CopyToChars(HostName, Pos, "/", true, nullptr, false);
  }
  if (!HostName.IsEmpty() && !UserName.IsEmpty())
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
  return Result;
}
//---------------------------------------------------------------------
UnicodeString TSessionData::GetNameWithoutHiddenPrefix() const
{
  UnicodeString Result = GetName();
  if (GetHidden())
  {
    Result = Result.SubString(TNamedObjectList::HiddenPrefix.Length() + 1, Result.Length() - TNamedObjectList::HiddenPrefix.Length());
  }
  return Result;
}
//---------------------------------------------------------------------
bool TSessionData::HasSessionName() const
{
  return (!GetNameWithoutHiddenPrefix().IsEmpty() && (Name != DefaultName));
}
//---------------------------------------------------------------------
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
//---------------------------------------------------------------------
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
//---------------------------------------------------------------------
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
      Url = S3Protocol;
      break;
  }

  Url += ProtocolSeparator;

  return Url;
}
//---------------------------------------------------------------------
bool IsIPv6Literal(UnicodeString AHostName)
{
  bool Result = (AHostName.Pos(":") > 0);
  if (Result)
  {
    for (intptr_t Index = 1; Result && (Index <= AHostName.Length()); Index++)
    {
      wchar_t C = AHostName[Index];
      Result = IsHex(C) || (C == L':');
    }
  }
  return Result;
}
//---------------------------------------------------------------------
UnicodeString EscapeIPv6Literal(UnicodeString IP)
{
  return L"[" + IP + L"]";
}
//---------------------------------------------------------------------
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
  return SessionData->SaveToOptions(FactoryDefaults.get());
}
//---------------------------------------------------------------------
bool TSessionData::HasRawSettingsForUrl()
{
  std::unique_ptr<TStrings> RawSettings(GetRawSettingsForUrl());
  return (RawSettings->Count > 0);
}
//---------------------------------------------------------------------
UnicodeString TSessionData::GenerateSessionUrl(uintptr_t Flags) const
{
  UnicodeString Url;

  if (FLAGSET(Flags, sufSpecific))
  {
    Url += WinSCPProtocolPrefix;
  }

  Url += GetProtocolUrl(FLAGSET(Flags, sufHttpForWebDAV));

  if (FLAGSET(Flags, sufUserName) && !GetUserNameExpanded().IsEmpty())
  {
    Url += EncodeUrlString(GetUserNameExpanded());

    if (FLAGSET(Flags, sufPassword) && !GetPassword().IsEmpty())
    {
      Url += L":" + EncodeUrlString(NormalizeString(Password));
    }

    if (FLAGSET(Flags, sufHostKey) && !GetHostKey().IsEmpty())
    {
      // Many SHA-256 fingerprints end with an equal sign and we do not really need it to be encoded, so avoid that.
      // Also colons in TLS/SSL fingerprint do not really need encoding.
      UnicodeString S = EncodeUrlString(NormalizeFingerprint(GetHostKey()), "=:");

      Url +=
        UnicodeString(UrlParamSeparator) + UrlHostKeyParamName +
        UnicodeString(UrlParamValueSeparator) + S;
    }

#if 0
    if (FLAGSET(Flags, sufRawSettings))
    {
      std::unique_ptr<TStrings> RawSettings(GetRawSettingsForUrl());
      for (int Index = 0; Index < RawSettings->Count; Index++)
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

  if (GetPortNumber() != GetDefaultPort(GetFSProtocol(), GetFtps()))
  {
    Url += UnicodeString(":") + ::Int64ToStr(GetPortNumber());
  }
  Url += "/";

  return Url;
}
//---------------------------------------------------------------------
__removed UnicodeString ScriptCommandOpenLink = ScriptCommandLink("open");

void TSessionData::AddSwitchValue(
  UnicodeString &Result, UnicodeString Name, UnicodeString Value)
{
  AddSwitch(Result, FORMAT("%s=%s", Name, Value));
}
//---------------------------------------------------------------------
void TSessionData::AddSwitch(
  UnicodeString &Result, UnicodeString Switch)
{
  Result += FORMAT(" -%s", Switch);
}
//---------------------------------------------------------------------
void TSessionData::AddSwitch(
  UnicodeString &Result, UnicodeString AName, UnicodeString Value)
{
  AddSwitchValue(Result, AName, FORMAT("\"%s\"", EscapeParam(Value)));
}
//---------------------------------------------------------------------
void TSessionData::AddSwitch(
  UnicodeString &Result, UnicodeString AName, intptr_t Value)
{
  AddSwitchValue(Result, AName, IntToStr(Value));
}
//---------------------------------------------------------------------
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
}
//---------------------------------------------------------------------
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
    AddSwitch(Result, SwitchName, SessionData->GetHostKey());
    SessionData->SetHostKey(FactoryDefaults->GetHostKey());
  }
  if (SessionData->GetPublicKeyFile() != FactoryDefaults->GetPublicKeyFile())
  {
    AddSwitch(Result, L"privatekey", SessionData->GetPublicKeyFile());
    SessionData->SetPublicKeyFile(FactoryDefaults->GetPublicKeyFile());
  }
  if (SessionData->GetTlsCertificateFile() != FactoryDefaults->GetTlsCertificateFile())
  {
    AddSwitch(Result, L"clientcert", SessionData->GetTlsCertificateFile());
    SessionData->SetTlsCertificateFile(FactoryDefaults->GetTlsCertificateFile());
  }
  if (SessionData->GetPassphrase() != FactoryDefaults->GetPassphrase())
  {
    AddSwitch(Result, PassphraseOption, SessionData->GetPassphrase());
    SessionData->SetPassphrase(FactoryDefaults->GetPassphrase());
  }
  if (SessionData->GetFtpPasvMode() != FactoryDefaults->GetFtpPasvMode())
  {
    AddSwitch(Result, L"passive", SessionData->GetFtpPasvMode() ? 1 : 0);
    SessionData->SetFtpPasvMode(FactoryDefaults->GetFtpPasvMode());
  }
  if (SessionData->GetTimeout() != FactoryDefaults->GetTimeout())
  {
    AddSwitch(Result, L"timeout", SessionData->GetTimeout());
    SessionData->SetTimeout(FactoryDefaults->GetTimeout());
  }

  std::unique_ptr<TStrings> RawSettings(SessionData->SaveToOptions(FactoryDefaults.get()));

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
//---------------------------------------------------------------------
UnicodeString SessionOptionsClassName(L"SessionOptions");
//---------------------------------------------------------------------
void TSessionData::AddAssemblyProperty(
  UnicodeString & Result, TAssemblyLanguage Language,
  UnicodeString & Name, UnicodeString & Type,
  UnicodeString & Member)
{
  Result += AssemblyProperty(Language, SessionOptionsClassName, Name, Type, Member, false);
}
//---------------------------------------------------------------------
void TSessionData::AddAssemblyProperty(
  UnicodeString & Result, TAssemblyLanguage Language,
  UnicodeString & Name, UnicodeString & Value)
{
  Result += AssemblyProperty(Language, SessionOptionsClassName, Name, Value, false);
}
//---------------------------------------------------------------------
void TSessionData::AddAssemblyProperty(
  UnicodeString & Result, TAssemblyLanguage Language,
  UnicodeString & Name, int Value)
{
  Result += AssemblyProperty(Language, SessionOptionsClassName, Name, Value, false);
}
//---------------------------------------------------------------------
void TSessionData::AddAssemblyProperty(
  UnicodeString & Result, TAssemblyLanguage Language,
  UnicodeString & Name, bool Value)
{
  Result += AssemblyProperty(Language, SessionOptionsClassName, Name, Value, false);
}
//---------------------------------------------------------------------
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

  if (SessionData->Ftps != FactoryDefaults->Ftps)
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
              // noop
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
        AddAssemblyProperty(Head, Language, L"WebdavSecure", (SessionData->Ftps != ftpsNone));
        break;

      case fsS3:
        // implicit
        break;

      default:
        DebugFail();
        break;
    }
    SessionData->Ftps = FactoryDefaults->Ftps;
  }

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

  std::unique_ptr<TStrings> RawSettings(SessionData->SaveToOptions(FactoryDefaults.get()));

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
//---------------------------------------------------------------------
void TSessionData::SetTimeDifference(TDateTime Value)
{
  SET_SESSION_PROPERTY(TimeDifference);
}
//---------------------------------------------------------------------
void TSessionData::SetTimeDifferenceAuto(bool Value)
{
  SET_SESSION_PROPERTY(TimeDifferenceAuto);
}
//---------------------------------------------------------------------
void TSessionData::SetLocalDirectory(UnicodeString Value)
{
  SET_SESSION_PROPERTY(LocalDirectory);
}
//---------------------------------------------------------------------
void TSessionData::SetRemoteDirectory(UnicodeString Value)
{
  SET_SESSION_PROPERTY(RemoteDirectory);
}
//---------------------------------------------------------------------
void TSessionData::SetSynchronizeBrowsing(bool Value)
{
  SET_SESSION_PROPERTY(SynchronizeBrowsing);
}
//---------------------------------------------------------------------
void TSessionData::SetUpdateDirectories(bool Value)
{
  SET_SESSION_PROPERTY(UpdateDirectories);
}
//---------------------------------------------------------------------
void TSessionData::SetCacheDirectories(bool Value)
{
  SET_SESSION_PROPERTY(CacheDirectories);
}
//---------------------------------------------------------------------
void TSessionData::SetCacheDirectoryChanges(bool Value)
{
  SET_SESSION_PROPERTY(CacheDirectoryChanges);
}
//---------------------------------------------------------------------
void TSessionData::SetPreserveDirectoryChanges(bool Value)
{
  SET_SESSION_PROPERTY(PreserveDirectoryChanges);
}
//---------------------------------------------------------------------
void TSessionData::SetResolveSymlinks(bool Value)
{
  SET_SESSION_PROPERTY(ResolveSymlinks);
}
//---------------------------------------------------------------------
void TSessionData::SetFollowDirectorySymlinks(bool Value)
{
  SET_SESSION_PROPERTY(FollowDirectorySymlinks);
}
//---------------------------------------------------------------------
void TSessionData::SetDSTMode(TDSTMode Value)
{
  SET_SESSION_PROPERTY(DSTMode);
}
//---------------------------------------------------------------------
void TSessionData::SetDeleteToRecycleBin(bool Value)
{
  SET_SESSION_PROPERTY(DeleteToRecycleBin);
}
//---------------------------------------------------------------------
void TSessionData::SetOverwrittenToRecycleBin(bool Value)
{
  SET_SESSION_PROPERTY(OverwrittenToRecycleBin);
}
//---------------------------------------------------------------------
void TSessionData::SetRecycleBinPath(UnicodeString Value)
{
  SET_SESSION_PROPERTY(RecycleBinPath);
}
//---------------------------------------------------------------------
void TSessionData::SetPostLoginCommands(UnicodeString Value)
{
  SET_SESSION_PROPERTY(PostLoginCommands);
}
//---------------------------------------------------------------------
void TSessionData::SetLockInHome(bool Value)
{
  SET_SESSION_PROPERTY(LockInHome);
}
//---------------------------------------------------------------------
void TSessionData::SetSpecial(bool Value)
{
  SET_SESSION_PROPERTY(Special);
}
//---------------------------------------------------------------------
void TSessionData::SetScp1Compatibility(bool Value)
{
  SET_SESSION_PROPERTY(Scp1Compatibility);
}
//---------------------------------------------------------------------
void TSessionData::SetTcpNoDelay(bool Value)
{
  SET_SESSION_PROPERTY(TcpNoDelay);
}
//---------------------------------------------------------------------
void TSessionData::SetSendBuf(intptr_t Value)
{
  SET_SESSION_PROPERTY(SendBuf);
}
//---------------------------------------------------------------------
void TSessionData::SetSshSimple(bool Value)
{
  SET_SESSION_PROPERTY(SshSimple);
}
//---------------------------------------------------------------------
void TSessionData::SetProxyMethod(TProxyMethod value)
{
  TProxyMethod Value = value == pmSystemOld ? pmSystem : value;
  nb::used(Value);
  SET_SESSION_PROPERTY(ProxyMethod);
}
//---------------------------------------------------------------------
void TSessionData::SetProxyHost(UnicodeString Value)
{
  SET_SESSION_PROPERTY(ProxyHost);
}
//---------------------------------------------------------------------
void TSessionData::SetProxyPort(intptr_t Value)
{
  SET_SESSION_PROPERTY(ProxyPort);
}
//---------------------------------------------------------------------
void TSessionData::SetProxyUsername(UnicodeString Value)
{
  SET_SESSION_PROPERTY(ProxyUsername);
}
//---------------------------------------------------------------------
void TSessionData::SetProxyPassword(UnicodeString AValue)
{
  RawByteString Value = EncryptPassword(AValue, GetProxyUsername() + GetProxyHost());
  SET_SESSION_PROPERTY(ProxyPassword);
}
//---------------------------------------------------------------------
TProxyMethod TSessionData::GetSystemProxyMethod() const
{
  PrepareProxyData();
  if ((GetProxyMethod() == pmSystem) && (nullptr != FIEProxyConfig))
    return FIEProxyConfig->ProxyMethod;
  return pmNone;
}

TProxyMethod TSessionData::GetActualProxyMethod() const
{
  return GetProxyMethod() == pmSystem ? GetSystemProxyMethod() : GetProxyMethod();
}

UnicodeString TSessionData::GetProxyHost() const
{
  PrepareProxyData();
  if ((GetProxyMethod() == pmSystem) && (nullptr != FIEProxyConfig))
    return FIEProxyConfig->ProxyHost;
  return FProxyHost;
}
intptr_t TSessionData::GetProxyPort() const
{
  PrepareProxyData();
  if ((GetProxyMethod() == pmSystem) && (nullptr != FIEProxyConfig))
    return FIEProxyConfig->ProxyPort;
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

static void FreeIEProxyConfig(WINHTTP_CURRENT_USER_IE_PROXY_CONFIG *IEProxyConfig)
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
  if ((GetProxyMethod() == pmSystem) && (nullptr == FIEProxyConfig))
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
  intptr_t ProxyPort = 0;
  TProxyMethod ProxyMethod = pmNone;
  UnicodeString ProxyUrlTmp;
  intptr_t ProxyPortTmp = 0;
  TProxyMethod ProxyMethodTmp = pmNone;
  for (intptr_t Index = 0; Index < ProxyServerList.GetCount(); ++Index)
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

void TSessionData::FromURI(UnicodeString ProxyURI,
  UnicodeString &ProxyUrl, intptr_t &ProxyPort, TProxyMethod &ProxyMethod) const
{
  ProxyUrl.Clear();
  ProxyPort = 0;
  ProxyMethod = pmNone;
  intptr_t Pos = ProxyURI.RPos(L':');
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
//---------------------------------------------------------------------
void TSessionData::SetProxyTelnetCommand(UnicodeString Value)
{
  SET_SESSION_PROPERTY(ProxyTelnetCommand);
}
//---------------------------------------------------------------------
void TSessionData::SetProxyLocalCommand(UnicodeString Value)
{
  SET_SESSION_PROPERTY(ProxyLocalCommand);
}
//---------------------------------------------------------------------
void TSessionData::SetProxyDNS(TAutoSwitch Value)
{
  SET_SESSION_PROPERTY(ProxyDNS);
}
//---------------------------------------------------------------------
void TSessionData::SetProxyLocalhost(bool Value)
{
  SET_SESSION_PROPERTY(ProxyLocalhost);
}
//---------------------------------------------------------------------
void TSessionData::SetFtpProxyLogonType(intptr_t Value)
{
  SET_SESSION_PROPERTY(FtpProxyLogonType);
}
//---------------------------------------------------------------------
void TSessionData::SetBug(TSshBug Bug, TAutoSwitch Value)
{
  DebugAssert(Bug >= 0 && nb::ToUInt32(Bug) < _countof(FBugs));
  SET_SESSION_PROPERTY(Bugs[Bug]);
}
//---------------------------------------------------------------------
TAutoSwitch TSessionData::GetBug(TSshBug Bug) const
{
  DebugAssert(Bug >= 0 && nb::ToUInt32(Bug) < _countof(FBugs));
  return FBugs[Bug];
}
//---------------------------------------------------------------------
void TSessionData::SetCustomParam1(UnicodeString Value)
{
  SET_SESSION_PROPERTY(CustomParam1);
}
//---------------------------------------------------------------------
void TSessionData::SetCustomParam2(UnicodeString Value)
{
  SET_SESSION_PROPERTY(CustomParam2);
}
//---------------------------------------------------------------------
void TSessionData::SetSFTPDownloadQueue(intptr_t Value)
{
  SET_SESSION_PROPERTY(SFTPDownloadQueue);
}
//---------------------------------------------------------------------
void TSessionData::SetSFTPUploadQueue(intptr_t Value)
{
  SET_SESSION_PROPERTY(SFTPUploadQueue);
}
//---------------------------------------------------------------------
void TSessionData::SetSFTPListingQueue(intptr_t Value)
{
  SET_SESSION_PROPERTY(SFTPListingQueue);
}
//---------------------------------------------------------------------
void TSessionData::SetSFTPMaxVersion(intptr_t Value)
{
  SET_SESSION_PROPERTY(SFTPMaxVersion);
}
//---------------------------------------------------------------------
void TSessionData::SetSFTPMaxPacketSize(intptr_t Value)
{
  SET_SESSION_PROPERTY(SFTPMaxPacketSize);
}
//---------------------------------------------------------------------
void TSessionData::SetSFTPBug(TSftpBug Bug, TAutoSwitch Value)
{
  DebugAssert(Bug >= 0 && nb::ToUInt32(Bug) < _countof(FSFTPBugs));
  SET_SESSION_PROPERTY(SFTPBugs[Bug]);
}
//---------------------------------------------------------------------
TAutoSwitch TSessionData::GetSFTPBug(TSftpBug Bug) const
{
  DebugAssert(Bug >= 0 && nb::ToUInt32(Bug) < _countof(FSFTPBugs));
  return FSFTPBugs[Bug];
}
//---------------------------------------------------------------------
void TSessionData::SetSCPLsFullTime(TAutoSwitch Value)
{
  SET_SESSION_PROPERTY(SCPLsFullTime);
}
//---------------------------------------------------------------------
void TSessionData::SetColor(intptr_t Value)
{
  SET_SESSION_PROPERTY(Color);
}
//---------------------------------------------------------------------
void TSessionData::SetTunnel(bool Value)
{
  SET_SESSION_PROPERTY(Tunnel);
}
//---------------------------------------------------------------------
void TSessionData::SetTunnelHostName(UnicodeString Value)
{
  if (FTunnelHostName != Value)
  {
    // HostName is key for password encryption
    UnicodeString XTunnelPassword = GetTunnelPassword();

    UnicodeString Value2 = Value;
    intptr_t P = Value2.LastDelimiter(L"@");
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
//---------------------------------------------------------------------
void TSessionData::SetTunnelPortNumber(intptr_t Value)
{
  SET_SESSION_PROPERTY(TunnelPortNumber);
}
//---------------------------------------------------------------------
void TSessionData::SetTunnelUserName(UnicodeString Value)
{
  // Avoid password recryption (what may popup master password prompt)
  if (FTunnelUserName != Value)
  {
    // TunnelUserName is key for password encryption
    UnicodeString XTunnelPassword = GetTunnelPassword();
    SET_SESSION_PROPERTY(TunnelUserName);
    SetTunnelPassword(XTunnelPassword);
    Shred(XTunnelPassword);
  }
}
//---------------------------------------------------------------------
void TSessionData::SetTunnelPassword(UnicodeString AValue)
{
  RawByteString Value = EncryptPassword(AValue, GetTunnelUserName() + GetTunnelHostName());
  SET_SESSION_PROPERTY(TunnelPassword);
}
//---------------------------------------------------------------------
UnicodeString TSessionData::GetTunnelPassword() const
{
  return DecryptPassword(FTunnelPassword, GetTunnelUserName() + GetTunnelHostName());
}
//---------------------------------------------------------------------
void TSessionData::SetTunnelPublicKeyFile(UnicodeString Value)
{
  if (FTunnelPublicKeyFile != Value)
  {
    // StripPathQuotes should not be needed as we do not feed quotes anymore
    FTunnelPublicKeyFile = StripPathQuotes(Value);
    Modify();
  }
}
//---------------------------------------------------------------------
void TSessionData::SetTunnelLocalPortNumber(intptr_t Value)
{
  SET_SESSION_PROPERTY(TunnelLocalPortNumber);
}
//---------------------------------------------------------------------
bool TSessionData::GetTunnelAutoassignLocalPortNumber() const
{
  return (FTunnelLocalPortNumber <= 0);
}
//---------------------------------------------------------------------
void TSessionData::SetTunnelPortFwd(UnicodeString Value)
{
  SET_SESSION_PROPERTY(TunnelPortFwd);
}
//---------------------------------------------------------------------
void TSessionData::SetTunnelHostKey(UnicodeString Value)
{
  SET_SESSION_PROPERTY(TunnelHostKey);
}
//---------------------------------------------------------------------
void TSessionData::SetFtpPasvMode(bool Value)
{
  SET_SESSION_PROPERTY(FtpPasvMode);
}
//---------------------------------------------------------------------
void TSessionData::SetFtpForcePasvIp(TAutoSwitch Value)
{
  SET_SESSION_PROPERTY(FtpForcePasvIp);
}
//---------------------------------------------------------------------
void TSessionData::SetFtpUseMlsd(TAutoSwitch Value)
{
  SET_SESSION_PROPERTY(FtpUseMlsd);
}
//---------------------------------------------------------------------
void TSessionData::SetFtpAccount(UnicodeString Value)
{
  SET_SESSION_PROPERTY(FtpAccount);
}
//---------------------------------------------------------------------
void TSessionData::SetFtpPingInterval(intptr_t Value)
{
  SET_SESSION_PROPERTY(FtpPingInterval);
}
//---------------------------------------------------------------------
TDateTime TSessionData::GetFtpPingIntervalDT() const
{
  return SecToDateTime(GetFtpPingInterval());
}
//---------------------------------------------------------------------
void TSessionData::SetFtpPingType(TPingType Value)
{
  SET_SESSION_PROPERTY(FtpPingType);
}
//---------------------------------------------------------------------
void TSessionData::SetFtpTransferActiveImmediately(TAutoSwitch Value)
{
  SET_SESSION_PROPERTY(FtpTransferActiveImmediately);
}
//---------------------------------------------------------------------
void TSessionData::SetFtps(TFtps Value)
{
  SET_SESSION_PROPERTY(Ftps);
}
//---------------------------------------------------------------------
void TSessionData::SetMinTlsVersion(TTlsVersion Value)
{
  SET_SESSION_PROPERTY(MinTlsVersion);
}
//---------------------------------------------------------------------
void TSessionData::SetMaxTlsVersion(TTlsVersion Value)
{
  SET_SESSION_PROPERTY(MaxTlsVersion);
}
//---------------------------------------------------------------------
void TSessionData::SetLogicalHostName(UnicodeString Value)
{
  SET_SESSION_PROPERTY(LogicalHostName);
}
//---------------------------------------------------------------------
void TSessionData::SetFtpListAll(TAutoSwitch Value)
{
  SET_SESSION_PROPERTY(FtpListAll);
}
//---------------------------------------------------------------------
void TSessionData::SetFtpHost(TAutoSwitch Value)
{
  SET_SESSION_PROPERTY(FtpHost);
}
//---------------------------------------------------------------------
void TSessionData::SetFtpDeleteFromCwd(TAutoSwitch Value)
{
  SET_SESSION_PROPERTY(FtpDeleteFromCwd);
}
//---------------------------------------------------------------------
void TSessionData::SetSslSessionReuse(bool Value)
{
  SET_SESSION_PROPERTY(SslSessionReuse);
}
//---------------------------------------------------------------------
void TSessionData::SetTlsCertificateFile(UnicodeString Value)
{
  SET_SESSION_PROPERTY(TlsCertificateFile);
}
//---------------------------------------------------------------------
void TSessionData::SetNotUtf(TAutoSwitch Value)
{
  SET_SESSION_PROPERTY(NotUtf);
}
//---------------------------------------------------------------------
void TSessionData::SetInternalEditorEncoding(intptr_t Value)
{
  SET_SESSION_PROPERTY(InternalEditorEncoding);
}
//---------------------------------------------------------------------
void TSessionData::SetS3DefaultRegion(UnicodeString Value)
{
  SET_SESSION_PROPERTY(S3DefaultRegion);
}
//---------------------------------------------------------------------
void TSessionData::SetIsWorkspace(bool Value)
{
  SET_SESSION_PROPERTY(IsWorkspace);
}
//---------------------------------------------------------------------
void TSessionData::SetLink(UnicodeString Value)
{
  SET_SESSION_PROPERTY(Link);
}
//---------------------------------------------------------------------
void TSessionData::SetNameOverride(UnicodeString Value)
{
  SET_SESSION_PROPERTY(NameOverride);
}
//---------------------------------------------------------------------
void TSessionData::SetHostKey(UnicodeString Value)
{
  SET_SESSION_PROPERTY(HostKey);
}
//---------------------------------------------------------------------
void TSessionData::SetNote(UnicodeString Value)
{
  SET_SESSION_PROPERTY(Note);
}
//---------------------------------------------------------------------
void TSessionData::SetWinTitle(UnicodeString Value)
{
  SET_SESSION_PROPERTY(WinTitle);
}

void TSessionData::SetSFTPMinPacketSize(intptr_t Value)
{
  SET_SESSION_PROPERTY(SFTPMinPacketSize);
}

void TSessionData::SetFtpAllowEmptyPassword(bool Value)
{
  SET_SESSION_PROPERTY(FtpAllowEmptyPassword);
}

void TSessionData::SetFtpDupFF(bool Value)
{
  SET_SESSION_PROPERTY(FtpDupFF);
}

void TSessionData::SetFtpUndupFF(bool Value)
{
  SET_SESSION_PROPERTY(FtpUndupFF);
}
//---------------------------------------------------------------------
UnicodeString TSessionData::GetEncryptKey() const
{
  return DecryptPassword(FEncryptKey, UserName() + HostName());
}
//---------------------------------------------------------------------
void TSessionData::SetEncryptKey(UnicodeString AValue)
{
  RawByteString Value = EncryptPassword(AValue, UserName() + HostName());
  SET_SESSION_PROPERTY(EncryptKey);
}
//---------------------------------------------------------------------
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
//---------------------------------------------------------------------
UnicodeString TSessionData::ExtractLocalName(UnicodeString Name)
{
  UnicodeString Result = Name;
  intptr_t P = Result.LastDelimiter(L"/");
  if (P > 0)
  {
    Result.Delete(1, P);
  }
  return Result;
}
//---------------------------------------------------------------------
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
//---------------------------------------------------------------------
UnicodeString TSessionData::ExtractFolderName(UnicodeString Name)
{
  UnicodeString Result;
  intptr_t P = Name.LastDelimiter(L"/");
  if (P > 0)
  {
    Result = Name.SubString(1, P - 1);
  }
  return Result;
}
//---------------------------------------------------------------------
UnicodeString TSessionData::GetFolderName() const
{
  UnicodeString Result;
  if (HasSessionName() || GetIsWorkspace())
  {
    Result = ExtractFolderName(GetName());
  }
  return Result;
}
//---------------------------------------------------------------------
UnicodeString TSessionData::ComposePath(
  UnicodeString APath, UnicodeString Name)
{
  return base::UnixIncludeTrailingBackslash(APath) + Name;
}
//---------------------------------------------------------------------
void TSessionData::DisableAuthenticationsExceptPassword()
{
  FSshNoUserAuth = false;
  FAuthTIS = false;
  FAuthKI = false;
  FAuthKIPassword = false;
  FAuthGSSAPI = false;
  FPublicKeyFile = L"";
  FTlsCertificateFile = L"";
  FPassphrase = L"";
  FTryAgent = false;
}

TLoginType TSessionData::GetLoginType() const
{
  return (SessionGetUserName() == AnonymousUserName) && GetPassword().IsEmpty() ?
    ltAnonymous : ltNormal;
}

void TSessionData::SetLoginType(TLoginType Value)
{
  SET_SESSION_PROPERTY(LoginType);
  if (GetLoginType() == ltAnonymous)
  {
    SetPassword(L"");
    SessionSetUserName(AnonymousUserName);
  }
}

uintptr_t TSessionData::GetCodePageAsNumber() const
{
  if (FCodePageAsNumber == 0)
    FCodePageAsNumber = ::GetCodePageAsNumber(GetCodePage());
  return FCodePageAsNumber;
}

void TSessionData::SetCodePage(UnicodeString Value)
{
  SET_SESSION_PROPERTY(CodePage);
  FCodePageAsNumber = 0;
}

void TSessionData::AdjustHostName(UnicodeString &HostName, UnicodeString Prefix) const
{
  UnicodeString FullPrefix = Prefix + ProtocolSeparator;
  if (::LowerCase(HostName.SubString(1, FullPrefix.Length())) == FullPrefix)
  {
    HostName.Delete(1, FullPrefix.Length());
  }
}

void TSessionData::RemoveProtocolPrefix(UnicodeString &HostName) const
{
  AdjustHostName(HostName, ScpProtocol);
  AdjustHostName(HostName, SftpProtocol);
  AdjustHostName(HostName, FtpProtocol);
  AdjustHostName(HostName, FtpsProtocol);
  AdjustHostName(HostName, WebDAVProtocol);
  AdjustHostName(HostName, WebDAVSProtocol);
}

TFSProtocol TSessionData::TranslateFSProtocolNumber(intptr_t AFSProtocol)
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

TFSProtocol TSessionData::TranslateFSProtocol(UnicodeString ProtocolID) const
{
  // Find protocol by string id
  TFSProtocol Result = static_cast<TFSProtocol>(-1);
  for (intptr_t Index = 0; Index < FSPROTOCOL_COUNT; ++Index)
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

TFtps TSessionData::TranslateFtpEncryptionNumber(intptr_t FtpEncryption) const
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
TStoredSessionList::TStoredSessionList() noexcept : TStoredSessionList::TStoredSessionList(false)
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
//---------------------------------------------------------------------
TStoredSessionList::~TStoredSessionList() noexcept
{
//  SAFE_DESTROY(FDefaultSettings);
  for (intptr_t Index = 0; Index < GetCount(); ++Index)
  {
    TObject* Obj = TNamedObjectList::AtObject(Index);
    SAFE_DESTROY(Obj);
    SetItem(Index, nullptr);
  }
}
//---------------------------------------------------------------------
void TStoredSessionList::Load(THierarchicalStorage *Storage,
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

    for (intptr_t Index = 0; Index < SubKeys->GetCount(); ++Index)
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
        TSessionData *SessionData = nullptr;
        if (SessionName == FDefaultSettings->GetName())
        {
          SessionData = FDefaultSettings.get();
        }
        else
        {
          // if the list was empty before loading, do not waste time trying to
          // find existing sites to overwrite (we rely on underlying storage
          // to secure uniqueness of the key names)
          if (!WasEmpty)
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
      for (intptr_t Index = 0; Index < TObjectList::GetCount(); ++Index)
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
//---------------------------------------------------------------------
void TStoredSessionList::Reload()
{
  if (Count <= GetConfiguration()->DontReloadMoreThanSessions)
  {
    bool SessionList = true;
    std::unique_ptr<THierarchicalStorage> Storage(GetConfiguration()->CreateStorage(SessionList));
    if (Storage->OpenSubKey(GetConfiguration()->StoredSessionsSubKey, False))
    {
      Load(Storage.get());
    }
  }
}
//---------------------------------------------------------------------
void TStoredSessionList::DoSave(THierarchicalStorage *Storage,
  TSessionData *Data, bool All, bool RecryptPasswordOnly,
  TSessionData *FactoryDefaults)
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
//---------------------------------------------------------------------
void TStoredSessionList::DoSave(THierarchicalStorage *Storage,
  bool All, bool RecryptPasswordOnly, TStrings *RecryptPasswordErrors)
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
    for (intptr_t Index = 0; Index < GetCountIncludingHidden(); Index++)
    {
      TSessionData *SessionData = GetAs<TSessionData>(Index);
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
//---------------------------------------------------------------------
void TStoredSessionList::Save(THierarchicalStorage *Storage, bool All)
{
  DoSave(Storage, All, false, nullptr);
}
//---------------------------------------------------------------------
void TStoredSessionList::DoSave(bool All, bool Explicit,
  bool RecryptPasswordOnly, TStrings *RecryptPasswordErrors)
{
  bool SessionList = true;
  std::unique_ptr<THierarchicalStorage> Storage(GetConfiguration()->CreateStorage(SessionList));
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
//---------------------------------------------------------------------
void TStoredSessionList::Save(bool All, bool Explicit)
{
  DoSave(All, Explicit, false, nullptr);
}
//---------------------------------------------------------------------
void TStoredSessionList::RecryptPasswords(TStrings *RecryptPasswordErrors)
{
  DoSave(true, true, true, RecryptPasswordErrors);
}
//---------------------------------------------------------------------
void TStoredSessionList::Saved()
{
  FDefaultSettings->SetModified(false);
  for (intptr_t Index = 0; Index < GetCountIncludingHidden(); ++Index)
  {
    GetAs<TSessionData>(Index)->SetModified(false);
  }
}
//---------------------------------------------------------------------
#if 0
void TStoredSessionList::ImportLevelFromFilezilla(
  _di_IXMLNode Node, UnicodeString & Path, _di_IXMLNode SettingsNode)
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
//---------------------------------------------------------------------
void TStoredSessionList::ImportFromFilezilla(
  UnicodeString /*FileName*/, UnicodeString /*ConfigurationFileName*/)
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
//---------------------------------------------------------------------
void TStoredSessionList::ImportFromKnownHosts(TStrings *Lines)
{
  bool SessionList = false;
  std::unique_ptr<THierarchicalStorage> HostKeyStorage(GetConfiguration()->CreateStorage(SessionList));
  std::unique_ptr<TStrings> KeyList(std::make_unique<TStringList>());
  if (OpenHostKeysSubKey(HostKeyStorage.get(), false))
  {
    HostKeyStorage->GetValueNames(KeyList.get());
  }
  HostKeyStorage.reset(nullptr);

  UnicodeString FirstError;
  for (intptr_t Index = 0; Index < Lines->GetCount(); ++Index)
  {
    try
    {
      UnicodeString Line = Lines->GetString(Index);
      Line = Trim(Line);
      if (!Line.IsEmpty() && (Line[1] != L';'))
      {
        intptr_t P = Pos(L' ', Line);
        if (P > 0)
        {
          UnicodeString HostNameStr = Line.SubString(1, P - 1);
          Line = Line.SubString(P + 1, Line.Length() - P);

          UTF8String UtfLine = UTF8String(Line);
          char *AlgorithmName = nullptr;
          int PubBlobLen = 0;
          char *CommentPtr = nullptr;
          const char *ErrorStr = nullptr;
          unsigned char *PubBlob = openssh_loadpub_line(UtfLine.data(), &AlgorithmName, &PubBlobLen, &CommentPtr, &ErrorStr);
          if (PubBlob == nullptr)
          {
            throw Exception(UnicodeString(ErrorStr));
          }
          else
          {
            try__finally
            {
              P = Pos(L',', HostNameStr);
              if (P > 0)
              {
                HostNameStr.SetLength(P - 1);
              }
              P = Pos(L':', HostNameStr);
              int64_t PortNumber = -1;
              if (P > 0)
              {
                UnicodeString PortNumberStr = HostNameStr.SubString(P + 1, HostNameStr.Length() - P);
                PortNumber = ::StrToInt64(PortNumberStr);
                HostNameStr.SetLength(P - 1);
              }
              if ((HostNameStr.Length() >= 2) &&
                  (HostNameStr[1] == L'[') && (HostNameStr[HostNameStr.Length()] == L']'))
              {
                HostNameStr = HostNameStr.SubString(2, HostNameStr.Length() - 2);
              }

              UnicodeString NameStr = HostNameStr;
              if (PortNumber >= 0)
              {
                NameStr = FORMAT("%s:%d", NameStr, nb::ToInt(PortNumber));
              }

              std::unique_ptr<TSessionData> SessionDataOwner;
              TSessionData * SessionData = dyn_cast<TSessionData>(FindByName(NameStr));
              if (SessionData == nullptr)
              {
                SessionData = new TSessionData("");
                SessionDataOwner.reset(SessionData);
                SessionData->CopyData(GetDefaultSettings());
                SessionData->SetName(NameStr);
                SessionData->SetHostName(HostNameStr);
                if (PortNumber >= 0)
                {
                  SessionData->SetPortNumber(nb::ToIntPtr(PortNumber));
                }
              }

              const struct ssh_signkey *Algorithm = find_pubkey_alg(AlgorithmName);
              if (Algorithm == nullptr)
              {
                throw Exception(FORMAT("Unknown public key algorithm \"%s\".", AlgorithmName));
              }

              void *Key = Algorithm->newkey(Algorithm, reinterpret_cast<const char *>(PubBlob), PubBlobLen);
              try__finally
              {
                if (Key == nullptr)
                {
                  throw Exception("Invalid public key.");
                }
                char *Fingerprint = Algorithm->fmtkey(Key);
                UnicodeString KeyKey =
                  FORMAT("%s@%d:%s", Algorithm->keytype, SessionData->GetPortNumber(), HostNameStr);
                UnicodeString HostKey =
                  FORMAT("%s:%s=%s", Algorithm->name, KeyKey, Fingerprint);
                sfree(Fingerprint);
                UnicodeString HostKeyList = SessionData->GetHostKey();
                AddToList(HostKeyList, HostKey, L";");
                SessionData->SetHostKey(HostKeyList);
                // If there's at least one unknown key type for this host, select it
                if (KeyList->IndexOf(KeyKey) < 0)
                {
                        SessionData->SetSelected(true);
                }
              },
              __finally
              {
                Algorithm->freekey(Key);
              } end_try__finally

              if (SessionDataOwner != nullptr)
              {
                Add(SessionDataOwner.release());
              }
            },
            __finally
            {
              sfree(PubBlob);
              sfree(AlgorithmName);
              sfree(CommentPtr);
            } end_try__finally
          }
        }
      }
    }
    catch (Exception &E)
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
//---------------------------------------------------------------------
void TStoredSessionList::Export(UnicodeString /*AFileName*/)
{
  ThrowNotImplemented(3003);
#if 0
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
//---------------------------------------------------------------------
void TStoredSessionList::SelectAll(bool Select)
{
  for (intptr_t Index = 0; Index < GetCount(); ++Index)
  {
    GetSession(Index)->SetSelected(Select);
  }
}
//---------------------------------------------------------------------
void TStoredSessionList::Import(TStoredSessionList *From,
  bool OnlySelected, TList *Imported)
{
  for (intptr_t Index = 0; Index < From->GetCount(); ++Index)
  {
    if (!OnlySelected || From->GetSession(Index)->GetSelected())
    {
      TSessionData *Session = new TSessionData("");
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
//---------------------------------------------------------------------
void TStoredSessionList::SelectSessionsToImport(
  TStoredSessionList *Dest, bool SSHOnly)
{
  for (intptr_t Index = 0; Index < GetCount(); ++Index)
  {
    GetSession(Index)->SetSelected(
      (!SSHOnly || (GetSession(Index)->GetNormalizedPuttyProtocol() == PuttySshProtocol)) &&
      !Dest->FindByName(GetSession(Index)->GetName()));
  }
}
//---------------------------------------------------------------------
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
//---------------------------------------------------------------------
void TStoredSessionList::UpdateStaticUsage()
{
#if 0
  intptr_t SCP = 0;
  intptr_t SFTP = 0;
  intptr_t FTP = 0;
  intptr_t FTPS = 0;
  intptr_t WebDAV = 0;
  int S3 = 0;
  intptr_t Password = 0;
  intptr_t Advanced = 0;
  intptr_t Color = 0;
  intptr_t Note = 0;
  intptr_t Tunnel = 0;
  bool Folders = false;
  bool Workspaces = false;
  std::unique_ptr<TSessionData> FactoryDefaults(std::make_unique<TSessionData>(L""));
  std::unique_ptr<TStringList> DifferentAdvancedProperties(CreateSortedStringList());
  for (intptr_t Index = 0; Index < Count; Index++)
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

      // this effectively does not take passwords (proxy + tunnel) into account,
      // when master password is set, as master password handler in not set up yet
      if (!Data->IsSame(FactoryDefaults.get(), true, DifferentAdvancedProperties.get()))
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

  Configuration->Usage->Set(L"StoredSessionsCountSCP", SCP);
  Configuration->Usage->Set(L"StoredSessionsCountSFTP", SFTP);
  Configuration->Usage->Set(L"StoredSessionsCountFTP", FTP);
  Configuration->Usage->Set(L"StoredSessionsCountFTPS", FTPS);
  Configuration->Usage->Set(L"StoredSessionsCountWebDAV", WebDAV);
  Configuration->Usage->Set(L"StoredSessionsCountWebDAVS", WebDAVS);
  Configuration->Usage->Set(L"StoredSessionsCountS3", S3);
  Configuration->Usage->Set(L"StoredSessionsCountPassword", Password);
  Configuration->Usage->Set(L"StoredSessionsCountColor", Color);
  Configuration->Usage->Set(L"StoredSessionsCountNote", Note);
  Configuration->Usage->Set(L"StoredSessionsCountAdvanced", Advanced);
  DifferentAdvancedProperties->Delimiter = L',';
  Configuration->Usage->Set(L"StoredSessionsAdvancedSettings", DifferentAdvancedProperties->DelimitedText);
  Configuration->Usage->Set(L"StoredSessionsCountTunnel", Tunnel);

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
  Configuration->Usage->Set(L"UsingDefaultStoredSession", CustomDefaultStoredSession);

  Configuration->Usage->Set(L"UsingStoredSessionsFolders", Folders);
  Configuration->Usage->Set(L"UsingWorkspaces", Workspaces);
#endif // #if 0
}
//---------------------------------------------------------------------
const TSessionData *TStoredSessionList::FindSame(TSessionData *Data)
{
  const TSessionData *Result = nullptr;
  if (!(Data->GetHidden() || Data->GetName().IsEmpty())) // || Data->GetIsWorkspace())
  {
    const TNamedObject *Obj = FindByName(Data->GetName());
    Result = dyn_cast<TSessionData>(Obj);
  }
  return Result;
}
//---------------------------------------------------------------------------
intptr_t TStoredSessionList::IndexOf(TSessionData *Data) const
{
  for (intptr_t Index = 0; Index < GetCount(); ++Index)
  {
    if (Data == GetSession(Index))
    {
      return Index;
    }
  }
  return -1;
}
//---------------------------------------------------------------------------
TSessionData *TStoredSessionList::NewSession(
  UnicodeString SessionName, TSessionData *Session)
{
  TSessionData *DuplicateSession = dyn_cast<TSessionData>(FindByName(SessionName));
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
//---------------------------------------------------------------------------
void TStoredSessionList::SetDefaultSettings(const TSessionData *Value)
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
//---------------------------------------------------------------------
bool TStoredSessionList::OpenHostKeysSubKey(THierarchicalStorage *Storage, bool CanCreate)
{
  return
    Storage->OpenRootKey(CanCreate) &&
    Storage->OpenSubKey(GetConfiguration()->GetSshHostKeysSubKey(), CanCreate);
}
//---------------------------------------------------------------------------
THierarchicalStorage *TStoredSessionList::CreateHostKeysStorageForWriting()
{
  bool SessionList = false;
  std::unique_ptr<THierarchicalStorage> Storage(GetConfiguration()->CreateStorage(SessionList));
  Storage->SetExplicit(true);
  Storage->SetAccessMode(smReadWrite);
  return Storage.release();
}
//---------------------------------------------------------------------------
void TStoredSessionList::ImportHostKeys(
  THierarchicalStorage * SourceStorage, THierarchicalStorage * TargetStorage, TStoredSessionList * Sessions, bool OnlySelected)
{
  if (OpenHostKeysSubKey(SourceStorage, false) &&
      OpenHostKeysSubKey(TargetStorage, true))
  {
    std::unique_ptr<TStringList> KeyList(std::make_unique<TStringList>());
    SourceStorage->GetValueNames(KeyList.get());

    DebugAssert(Sessions != nullptr);
    for (int Index = 0; Index < Sessions->Count; Index++)
    {
      TSessionData * Session = Sessions->GetSession(Index);
      if (!OnlySelected || Session->Selected)
      {
        UnicodeString HostKeyName = PuttyMungeStr(FORMAT("@%d:%s", Session->PortNumber, Session->HostNameExpanded));
        for (int KeyIndex = 0; KeyIndex < KeyList->Count; KeyIndex++)
        {
          UnicodeString KeyName = KeyList->GetString(KeyIndex);
          if (EndsText(HostKeyName, KeyName))
          {
            TargetStorage->WriteStringRaw(KeyName, SourceStorage->ReadStringRaw(KeyName, ""));
          }
        }
      }
    }
  }
}
//---------------------------------------------------------------------------
void TStoredSessionList::ImportHostKeys(
  UnicodeString & SourceKey, TStoredSessionList * Sessions, bool OnlySelected)
{
  std::unique_ptr<THierarchicalStorage> TargetStorage(CreateHostKeysStorageForWriting());
  TargetStorage->Init();
  std::unique_ptr<THierarchicalStorage> SourceStorage(std::make_unique<TRegistryStorage>(SourceKey));
  SourceStorage->Init();

  ImportHostKeys(SourceStorage.get(), TargetStorage.get(), Sessions, OnlySelected);
}
//---------------------------------------------------------------------------
void TStoredSessionList::ImportSelectedKnownHosts(TStoredSessionList *Sessions)
{
  std::unique_ptr<THierarchicalStorage> Storage(CreateHostKeysStorageForWriting());
  if (OpenHostKeysSubKey(Storage.get(), true))
  {
    for (intptr_t Index = 0; Index < Sessions->GetCount(); ++Index)
    {
      TSessionData *Session = Sessions->GetSession(Index);
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

const TSessionData *TStoredSessionList::GetSessionByName(UnicodeString SessionName) const
{
  for (intptr_t Index = 0; Index < GetCount(); ++Index)
  {
    const TSessionData *SessionData = GetSession(Index);
    if (SessionData->GetName() == SessionName)
    {
      return SessionData;
    }
  }
  return nullptr;
}

void TStoredSessionList::Load(UnicodeString AKey, bool UseDefaults)
{
  std::unique_ptr<TRegistryStorage> Storage(std::make_unique<TRegistryStorage>(AKey));
  Storage->Init();
  if (Storage->OpenRootKey(false))
  {
    Load(Storage.get(), false, UseDefaults);
  }
}
//---------------------------------------------------------------------
bool TStoredSessionList::IsFolderOrWorkspace(
  UnicodeString Name, bool Workspace) const
{
  bool Result = false;
  const TSessionData *FirstData = nullptr;
  if (!Name.IsEmpty())
  {
    for (intptr_t Index = 0; !Result && (Index < GetCount()); ++Index)
    {
      Result = GetSession(Index)->IsInFolderOrWorkspace(Name);
      if (Result)
      {
        FirstData = GetSession(Index);
      }
    }
  }

  return
    Result &&
    DebugAlwaysTrue(FirstData != nullptr) &&
    (FirstData->GetIsWorkspace() == Workspace);
}
//---------------------------------------------------------------------------
bool TStoredSessionList::GetIsFolder(UnicodeString Name) const
{
  return IsFolderOrWorkspace(Name, false);
}
//---------------------------------------------------------------------------
bool TStoredSessionList::GetIsWorkspace(UnicodeString Name) const
{
  return IsFolderOrWorkspace(Name, true);
}
//---------------------------------------------------------------------------
TSessionData *TStoredSessionList::CheckIsInFolderOrWorkspaceAndResolve(
  TSessionData *Data, UnicodeString Name)
{
  if (Data && Data->IsInFolderOrWorkspace(Name))
  {
    Data = ResolveWorkspaceData(Data);

    if ((Data != nullptr) && Data->GetCanLogin() &&
      DebugAlwaysTrue(Data->GetLink().IsEmpty()))
    {
      return Data;
    }
  }
  return nullptr;
}
//---------------------------------------------------------------------
void TStoredSessionList::GetFolderOrWorkspace(UnicodeString Name, TList *List)
{
  for (intptr_t Index = 0; (Index < GetCount()); ++Index)
  {
    TSessionData *RawData = GetSession(Index);
    TSessionData *Data =
      CheckIsInFolderOrWorkspaceAndResolve(RawData, Name);

    if (Data != nullptr)
    {
      std::unique_ptr<TSessionData> Data2 = std::make_unique<TSessionData>("");
      Data2->Assign(Data);

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
        Data2->Name = RawData->NameOverride;
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
//---------------------------------------------------------------------------
TStrings *TStoredSessionList::GetFolderOrWorkspaceList(
  UnicodeString Name)
{
  std::unique_ptr<TObjectList> DataList(new TObjectList());
  GetFolderOrWorkspace(Name, DataList.get());

  std::unique_ptr<TStringList> Result(new TStringList());
  for (int Index = 0; (Index < DataList->Count); Index++)
  {
    Result->Add(dyn_cast<TSessionData>(DataList->GetObj(Index))->GetSessionName());
  }

  return Result.release();
}
//---------------------------------------------------------------------------
TStrings *TStoredSessionList::GetWorkspaces() const
{
  std::unique_ptr<TStringList> Result(CreateSortedStringList());

  for (intptr_t Index = 0; Index < GetCount(); ++Index)
  {
    const TSessionData *Data = GetSession(Index);
    if (Data->GetIsWorkspace())
    {
      Result->Add(Data->GetFolderName());
    }
  }

  return Result.release();
}
//---------------------------------------------------------------------------
void TStoredSessionList::NewWorkspace(
  UnicodeString Name, TList *DataList)
{
  for (intptr_t Index = 0; Index < GetCount(); ++Index)
  {
    TSessionData *Data = GetSession(Index);
    if (Data->IsInFolderOrWorkspace(Name))
    {
      FPendingRemovals->Add(Data->Name);
      Remove(Data);
      Index--;
    }
  }

  for (intptr_t Index = 0; Index < DataList->GetCount(); ++Index)
  {
    TSessionData *Data = DataList->GetAs<TSessionData>(Index);

    std::unique_ptr<TSessionData> Data2 = std::make_unique<TSessionData>("");
    Data2->Assign(Data);
    Data2->SetName(TSessionData::ComposePath(Name, Data->GetName()));
    // make sure, that new stored session is saved to registry
    Data2->SetModified(true);
    Add(Data2.release());
  }
}
//---------------------------------------------------------------------------
bool TStoredSessionList::HasAnyWorkspace() const
{
  bool Result = false;
  for (intptr_t Index = 0; !Result && (Index < GetCount()); ++Index)
  {
    const TSessionData *Data = GetSession(Index);
    Result = Data->GetIsWorkspace();
  }
  return Result;
}
//---------------------------------------------------------------------------
TSessionData *TStoredSessionList::ParseUrl(UnicodeString Url,
  TOptions *Options, bool &DefaultsOnly, UnicodeString *AFileName,
  bool * AProtocolDefined, UnicodeString * MaskedUrl, intptr_t Flags)
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
//---------------------------------------------------------------------------
bool TStoredSessionList::IsUrl(UnicodeString Url)
{
  bool DefaultsOnly;
  bool ProtocolDefined = false;
  std::unique_ptr<TSessionData> ParsedData(ParseUrl(Url, nullptr, DefaultsOnly, nullptr, &ProtocolDefined));
  bool Result = ProtocolDefined;
  return Result;
}
//---------------------------------------------------------------------------
TSessionData *TStoredSessionList::ResolveWorkspaceData(TSessionData *Data)
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
//---------------------------------------------------------------------------
TSessionData * TStoredSessionList::SaveWorkspaceData(TSessionData * Data, int Index)
{
  std::unique_ptr<TSessionData> Result(std::make_unique<TSessionData>(""));

  const TSessionData *SameData = StoredSessions->FindSame(Data);
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
//---------------------------------------------------------------------------
bool TStoredSessionList::CanLogin(TSessionData *Data)
{
  Data = ResolveWorkspaceData(Data);
  return (Data != nullptr) && Data->GetCanLogin();
}
//---------------------------------------------------------------------------
UnicodeString GetExpandedLogFileName(UnicodeString LogFileName, TDateTime Started, TSessionData *SessionData)
{
  // StripPathQuotes should not be needed as we do not feed quotes anymore
  UnicodeString Result = StripPathQuotes(::ExpandEnvironmentVariables(LogFileName));
  for (intptr_t Index = 1; Index < Result.Length(); ++Index)
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
        Replacement = ::Int64ToStr(nb::ToInt(::GetCurrentProcessId()));
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
//---------------------------------------------------------------------------
bool GetIsSshProtocol(TFSProtocol FSProtocol)
{
  return
    (FSProtocol == fsSFTPonly) || (FSProtocol == fsSFTP) ||
    (FSProtocol == fsSCPonly);
}
//---------------------------------------------------------------------------
intptr_t GetDefaultPort(TFSProtocol FSProtocol, TFtps Ftps)
{
  intptr_t Result;
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
//---------------------------------------------------------------------
bool GetCodePageInfo(UINT CodePage, CPINFOEX &CodePageInfoEx)
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

uintptr_t GetCodePageAsNumber(UnicodeString CodePage)
{
  uintptr_t codePage = _wtoi(CodePage.c_str());
  return nb::ToUIntPtr(codePage == 0 ? CONST_DEFAULT_CODEPAGE : codePage);
}

UnicodeString GetCodePageAsString(uintptr_t CodePage)
{
  CPINFOEX cpInfoEx;
  if (::GetCodePageInfo(static_cast<UINT>(CodePage), cpInfoEx))
  {
    return UnicodeString(cpInfoEx.CodePageName);
  }
  return ::IntToStr(CONST_DEFAULT_CODEPAGE);
}
