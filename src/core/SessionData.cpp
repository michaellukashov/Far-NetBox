//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include <Winhttp.h>

#include <StrUtils.hpp>
#include "SessionData.h"

#include "Common.h"
#include "Exceptions.h"
#include "FileBuffer.h"
#include "CoreMain.h"
#include "TextsCore.h"
#include "PuttyIntf.h"
#include "RemoteFiles.h"

//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------
enum TProxyType { pxNone, pxHTTP, pxSocks, pxTelnet }; // 0.53b and older
const wchar_t * DefaultName = L"Default Settings";
const wchar_t CipherNames[CIPHER_COUNT][10] = {L"WARN", L"3des", L"blowfish", L"aes", L"des", L"arcfour"};
const wchar_t KexNames[KEX_COUNT][20] = {L"WARN", L"dh-group1-sha1", L"dh-group14-sha1", L"dh-gex-sha1", L"rsa" };
const wchar_t ProtocolNames[PROTOCOL_COUNT][10] = { L"raw", L"telnet", L"rlogin", L"ssh" };
const wchar_t SshProtList[][10] = {L"1 only", L"1", L"2", L"2 only"};
const wchar_t ProxyMethodList[][10] = {L"none", L"SOCKS4", L"SOCKS5", L"HTTP", L"Telnet", L"Cmd", L"System" };
const TCipher DefaultCipherList[CIPHER_COUNT] =
  { cipAES, cipBlowfish, cip3DES, cipWarn, cipArcfour, cipDES };
const TKex DefaultKexList[KEX_COUNT] =
  { kexDHGEx, kexDHGroup14, kexDHGroup1, kexRSA, kexWarn };
const wchar_t FSProtocolNames[FSPROTOCOL_COUNT][11] = { L"SCP", L"SFTP (SCP)", L"SFTP", L"", L"", L"FTP", L"WebDAV" };
const intptr_t SshPortNumber = 22;
const intptr_t FtpPortNumber = 21;
const intptr_t FtpsImplicitPortNumber = 990;
const intptr_t HTTPPortNumber = 80;
const intptr_t HTTPSPortNumber = 443;
const intptr_t TelnetPortNumber = 23;
const intptr_t DefaultSendBuf = 256 * 1024;
const UnicodeString AnonymousUserName(L"anonymous");
const UnicodeString AnonymousPassword(L"");
const UnicodeString PuttySshProtocol(L"ssh");
const UnicodeString PuttyTelnetProtocol(L"telnet");

const uintptr_t CONST_DEFAULT_CODEPAGE = CP_ACP;
const TFSProtocol CONST_DEFAULT_PROTOCOL = fsSFTP;

//---------------------------------------------------------------------
static TDateTime SecToDateTime(intptr_t Sec)
{
  return TDateTime(static_cast<unsigned short>(Sec/SecsPerHour),
    static_cast<unsigned short>(Sec/SecsPerMin%MinsPerHour), static_cast<unsigned short>(Sec%SecsPerMin), 0);
}
//--- TSessionData ----------------------------------------------------
TSessionData::TSessionData(const UnicodeString & AName) :
  TNamedObject(AName),
  FIEProxyConfig(nullptr)
{
  Default();
  FModified = true;
}
//---------------------------------------------------------------------
TSessionData::~TSessionData()
{
  if (nullptr != FIEProxyConfig)
  {
    delete FIEProxyConfig;
    FIEProxyConfig = nullptr;
  }
}
//---------------------------------------------------------------------
void TSessionData::Default()
{
  SetHostName(L"");
  SetPortNumber(SshPortNumber);
  SetUserName(AnonymousUserName);
  SetPassword(AnonymousPassword);
  SetPingInterval(30);
  // when changing default, update load/save logic
  SetPingType(ptOff);
  SetTimeout(15);
  SetTryAgent(true);
  SetAgentFwd(false);
  SetAuthTIS(false);
  SetAuthKI(true);
  SetAuthKIPassword(true);
  SetAuthGSSAPI(false);
  SetGSSAPIFwdTGT(false);
  SetGSSAPIServerRealm(L"");
  SetChangeUsername(false);
  SetCompression(false);
  SetSshProt(ssh2);
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
  SetPublicKeyFile(L"");
  FProtocol = ptSSH;
  SetPuttyProtocol(PuttySshProtocol);
  SetTcpNoDelay(true);
  SetSendBuf(DefaultSendBuf);
  SetSshSimple(true);
  SetHostKey(L"");

  SetProxyMethod(::pmNone);
  SetProxyHost(L"proxy");
  SetProxyPort(80);
  SetProxyUsername(L"");
  SetProxyPassword(L"");
  SetProxyTelnetCommand(L"connect %host %port\\n");
  SetProxyLocalCommand(L"");
  SetProxyDNS(asAuto);
  SetProxyLocalhost(false);

  for (intptr_t Index = 0; Index < static_cast<intptr_t>(LENOF(FBugs)); ++Index)
  {
    SetBug(static_cast<TSshBug>(Index), asAuto);
  }

  SetSpecial(false);
  SetFSProtocol(CONST_DEFAULT_PROTOCOL);
  SetAddressFamily(afAuto);
  SetRekeyData(L"1G");
  SetRekeyTime(MinsPerHour);

  // FS common
  SetLocalDirectory(L"");
  SetRemoteDirectory(L"");
  SetSynchronizeBrowsing(false);
  SetUpdateDirectories(true);
  SetCacheDirectories(true);
  SetCacheDirectoryChanges(true);
  SetPreserveDirectoryChanges(true);
  SetLockInHome(false);
  SetResolveSymlinks(true);
  SetDSTMode(dstmUnix);
  SetDeleteToRecycleBin(false);
  SetOverwrittenToRecycleBin(false);
  SetRecycleBinPath(L"");
  SetColor(0);
  SetPostLoginCommands(L"");

  // SCP
  SetReturnVar(L"");
  SetLookupUserGroups(asAuto);
  SetEOLType(eolLF);
  SetShell(L""); //default shell
  SetReturnVar(L"");
  SetClearAliases(true);
  SetUnsetNationalVars(true);
  SetListingCommand(L"ls -la");
  SetIgnoreLsWarnings(true);
  SetScp1Compatibility(false);
  SetTimeDifference(TDateTime(0));
  SetSCPLsFullTime(asAuto);
  SetNotUtf(asOn); // asAuto

  // SFTP
  SetSftpServer(L"");
  SetSFTPDownloadQueue(4);
  SetSFTPUploadQueue(4);
  SetSFTPListingQueue(2);
  SetSFTPMaxVersion(5);
  SetSFTPMaxPacketSize(0);
  SetSFTPMinPacketSize(0);

  for (intptr_t Index = 0; Index < static_cast<intptr_t>(LENOF(FSFTPBugs)); ++Index)
  {
    SetSFTPBug(static_cast<TSftpBug>(Index), asAuto);
  }

  SetTunnel(false);
  SetTunnelHostName(L"");
  SetTunnelPortNumber(SshPortNumber);
  SetTunnelUserName(L"");
  SetTunnelPassword(L"");
  SetTunnelPublicKeyFile(L"");
  SetTunnelLocalPortNumber(0);
  SetTunnelPortFwd(L"");
  SetTunnelHostKey(L"");

  // FTP
  SetFtpPasvMode(true);
  SetFtpForcePasvIp(asAuto);
  SetFtpUseMlsd(asAuto);
  SetFtpAccount(L"");
  SetFtpPingInterval(30);
  SetFtpPingType(ptDummyCommand);
  SetFtps(ftpsNone);
  SetMinTlsVersion(ssl2);
  SetMaxTlsVersion(tls12);
  SetFtpListAll(asAuto);
  SetFtpDupFF(false);
  SetFtpUndupFF(false);
  SetSslSessionReuse(true);

  SetFtpProxyLogonType(0); // none

  SetCustomParam1(L"");
  SetCustomParam2(L"");

  // SetIsWorkspace(false);
  // SetLink(L"");

  SetSelected(false);
  FModified = false;
  FSource = ::ssNone;

  SetCodePage(::GetCodePageAsString(CONST_DEFAULT_CODEPAGE));
  SetLoginType(ltAnonymous);
  SetFtpAllowEmptyPassword(false);

  FNumberOfRetries = 0;
  FSessionVersion = ::StrToVersionNumber(GetGlobalFunctions()->GetStrVersionNumber());
  // add also to TSessionLog::AddStartupInfo()
}
//---------------------------------------------------------------------
void TSessionData::NonPersistant()
{
  SetUpdateDirectories(false);
  SetPreserveDirectoryChanges(false);
}
//---------------------------------------------------------------------
#define BASE_PROPERTIES \
  PROPERTY(HostName); \
  PROPERTY(PortNumber); \
  PROPERTY(UserName); \
  PROPERTY(Password); \
  PROPERTY(PublicKeyFile); \
  PROPERTY(FSProtocol); \
  PROPERTY(Ftps); \
  PROPERTY(LocalDirectory); \
  PROPERTY(RemoteDirectory); \
//---------------------------------------------------------------------
#define ADVANCED_PROPERTIES \
  PROPERTY(PingInterval); \
  PROPERTY(PingType); \
  PROPERTY(Timeout); \
  PROPERTY(TryAgent); \
  PROPERTY(AgentFwd); \
  PROPERTY(AuthTIS); \
  PROPERTY(ChangeUsername); \
  PROPERTY(Compression); \
  PROPERTY(SshProt); \
  PROPERTY(Ssh2DES); \
  PROPERTY(SshNoUserAuth); \
  PROPERTY(CipherList); \
  PROPERTY(KexList); \
  PROPERTY(AddressFamily); \
  PROPERTY(RekeyData); \
  PROPERTY(RekeyTime); \
  PROPERTY(HostKey); \
  \
  PROPERTY(SynchronizeBrowsing); \
  PROPERTY(UpdateDirectories); \
  PROPERTY(CacheDirectories); \
  PROPERTY(CacheDirectoryChanges); \
  PROPERTY(PreserveDirectoryChanges); \
  \
  PROPERTY(ResolveSymlinks); \
  PROPERTY(DSTMode); \
  PROPERTY(LockInHome); \
  PROPERTY(Special); \
  PROPERTY(Selected); \
  PROPERTY(ReturnVar); \
  PROPERTY(LookupUserGroups); \
  PROPERTY(EOLType); \
  PROPERTY(Shell); \
  PROPERTY(ClearAliases); \
  PROPERTY(Scp1Compatibility); \
  PROPERTY(UnsetNationalVars); \
  PROPERTY(ListingCommand); \
  PROPERTY(IgnoreLsWarnings); \
  PROPERTY(SCPLsFullTime); \
  \
  PROPERTY(TimeDifference); \
  PROPERTY(TcpNoDelay); \
  PROPERTY(SendBuf); \
  PROPERTY(SshSimple); \
  PROPERTY(AuthKI); \
  PROPERTY(AuthKIPassword); \
  PROPERTY(AuthGSSAPI); \
  PROPERTY(GSSAPIFwdTGT); \
  PROPERTY(GSSAPIServerRealm); \
  PROPERTY(DeleteToRecycleBin); \
  PROPERTY(OverwrittenToRecycleBin); \
  PROPERTY(RecycleBinPath); \
  PROPERTY(NotUtf); \
  PROPERTY(PostLoginCommands); \
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
  PROPERTY(Color); \
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
  PROPERTY(FtpListAll); \
  PROPERTY(FtpDupFF); \
  PROPERTY(FtpUndupFF); \
  PROPERTY(SslSessionReuse); \
  \
  PROPERTY(FtpProxyLogonType); \
  \
  PROPERTY(MinTlsVersion); \
  PROPERTY(MaxTlsVersion); \
  \
  PROPERTY(CustomParam1); \
  PROPERTY(CustomParam2); \
  \
  PROPERTY(CodePage); \
  PROPERTY(LoginType); \
  PROPERTY(FtpAllowEmptyPassword);

  //PROPERTY(IsWorkspace); \
  //PROPERTY(Link); \
//---------------------------------------------------------------------
void TSessionData::Assign(const TPersistent * Source)
{
  if (Source && (dynamic_cast<const TSessionData *>(Source) != nullptr))
  {
    TSessionData * Data = dynamic_cast<TSessionData *>(const_cast<TPersistent *>(Source));
    #define PROPERTY(P) Set ## P(Data->Get ## P())
    PROPERTY(Name);
    BASE_PROPERTIES;
    ADVANCED_PROPERTIES;
    #undef PROPERTY

    for (intptr_t Index = 0; Index < static_cast<intptr_t>(LENOF(FBugs)); ++Index)
    {
      // PROPERTY(Bug[(TSshBug)Index]);
      SetBug(static_cast<TSshBug>(Index),
          Data->GetBug(static_cast<TSshBug>(Index)));
    }
    for (intptr_t Index = 0; Index < static_cast<intptr_t>(LENOF(FSFTPBugs)); ++Index)
    {
      // PROPERTY(SFTPBug[(TSftpBug)Index]);
      SetSFTPBug(static_cast<TSftpBug>(Index),
          Data->GetSFTPBug(static_cast<TSftpBug>(Index)));
    }

    FModified = Data->GetModified();
    FSource = Data->FSource;

    FNumberOfRetries = Data->FNumberOfRetries;
  }
  else
  {
    TNamedObject::Assign(Source);
  }
}
//---------------------------------------------------------------------
bool TSessionData::IsSame(const TSessionData * Default, bool AdvancedOnly) const
{
  #define PROPERTY(P) if (Get ## P() != Default->Get ## P()) return false;
  if (!AdvancedOnly)
  {
    BASE_PROPERTIES;
  }
  ADVANCED_PROPERTIES;
  #undef PROPERTY

  for (intptr_t Index = 0; Index < static_cast<intptr_t>(LENOF(FBugs)); ++Index)
  {
    // PROPERTY(Bug[(TSshBug)Index]);
    if (GetBug(static_cast<TSshBug>(Index)) != Default->GetBug(static_cast<TSshBug>(Index))) return false;
  }
  for (intptr_t Index = 0; Index < static_cast<intptr_t>(LENOF(FSFTPBugs)); ++Index)
  {
    // PROPERTY(SFTPBug[(TSftpBug)Index]);
    if (GetSFTPBug(static_cast<TSftpBug>(Index)) != Default->GetSFTPBug(static_cast<TSftpBug>(Index))) return false;
  }

  return true;
}
//---------------------------------------------------------------------
bool TSessionData::IsInFolderOrWorkspace(const UnicodeString & AFolder) const
{
  return StartsText(UnixIncludeTrailingBackslash(AFolder), GetName());
}
//---------------------------------------------------------------------
void TSessionData::DoLoad(THierarchicalStorage * Storage, bool & RewritePassword)
{
  SetSessionVersion(::StrToVersionNumber(Storage->ReadString(L"Version", L"")));
  SetPortNumber(Storage->ReadInteger(L"PortNumber", GetPortNumber()));
  SetUserName(Storage->ReadString(L"UserName", GetUserName()));
  // must be loaded after UserName, because HostName may be in format user@host
  SetHostName(Storage->ReadString(L"HostName", GetHostName()));

  if (!GetConfiguration()->GetDisablePasswordStoring())
  {
    if (Storage->ValueExists(L"PasswordPlain"))
    {
      SetPassword(Storage->ReadString(L"PasswordPlain", GetPassword()));
      RewritePassword = true;
    }
    else
    {
      FPassword = Storage->ReadStringAsBinaryData(L"Password", FPassword);
    }
  }
  SetHostKey(Storage->ReadString(L"HostKey", GetHostKey()));
  // Putty uses PingIntervalSecs
  intptr_t PingIntervalSecs = Storage->ReadInteger(L"PingIntervalSecs", -1);
  if (PingIntervalSecs < 0)
  {
    PingIntervalSecs = Storage->ReadInteger(L"PingIntervalSec", GetPingInterval()%SecsPerMin);
  }
  SetPingInterval(
    Storage->ReadInteger(L"PingInterval", GetPingInterval()/SecsPerMin)*SecsPerMin +
    PingIntervalSecs);
  if (GetPingInterval() == 0)
  {
    SetPingInterval(30);
  }
  // PingType has not existed before 3.5, where PingInterval > 0 meant today's ptNullPacket
  // Since 3.5, until 4.1 PingType was stored unconditionally.
  // Since 4.1 PingType is stored when it is not ptOff (default) or
  // when PingInterval is stored.
  if (!Storage->ValueExists(L"PingType"))
  {
    if (Storage->ReadInteger(L"PingInterval", 0) > 0)
    {
      SetPingType(ptNullPacket);
    }
  }
  else
  {
    SetPingType(static_cast<TPingType>(Storage->ReadInteger(L"PingType", ptOff)));
  }
  SetTimeout(Storage->ReadInteger(L"Timeout", GetTimeout()));
  SetTryAgent(Storage->ReadBool(L"TryAgent", GetTryAgent()));
  SetAgentFwd(Storage->ReadBool(L"AgentFwd", GetAgentFwd()));
  SetAuthTIS(Storage->ReadBool(L"AuthTIS", GetAuthTIS()));
  SetAuthKI(Storage->ReadBool(L"AuthKI", GetAuthKI()));
  SetAuthKIPassword(Storage->ReadBool(L"AuthKIPassword", GetAuthKIPassword()));
  // Continue to use setting keys of previous kerberos implementation (vaclav tomec),
  // but fallback to keys of other implementations (official putty and vintela quest putty),
  // to allow imports from all putty versions.
  // Both vaclav tomec and official putty use AuthGSSAPI
  SetAuthGSSAPI(Storage->ReadBool(L"AuthGSSAPI", Storage->ReadBool(L"AuthSSPI", GetAuthGSSAPI())));
  SetGSSAPIFwdTGT(Storage->ReadBool(L"GSSAPIFwdTGT", Storage->ReadBool(L"GssapiFwd", Storage->ReadBool(L"SSPIFwdTGT", GetGSSAPIFwdTGT()))));
  SetGSSAPIServerRealm(Storage->ReadString(L"GSSAPIServerRealm", Storage->ReadString(L"KerbPrincipal", GetGSSAPIServerRealm())));
  SetChangeUsername(Storage->ReadBool(L"ChangeUsername", GetChangeUsername()));
  SetCompression(Storage->ReadBool(L"Compression", GetCompression()));
  SetSshProt(static_cast<TSshProt>(Storage->ReadInteger(L"SshProt", GetSshProt())));
  SetSsh2DES(Storage->ReadBool(L"Ssh2DES", GetSsh2DES()));
  SetSshNoUserAuth(Storage->ReadBool(L"SshNoUserAuth", GetSshNoUserAuth()));
  SetCipherList(Storage->ReadString(L"Cipher", GetCipherList()));
  SetKexList(Storage->ReadString(L"KEX", GetKexList()));
  SetPublicKeyFile(Storage->ReadString(L"PublicKeyFile", GetPublicKeyFile()));
  SetAddressFamily(static_cast<TAddressFamily>
    (Storage->ReadInteger(L"AddressFamily", GetAddressFamily())));
  SetRekeyData(Storage->ReadString(L"RekeyBytes", GetRekeyData()));
  SetRekeyTime(Storage->ReadInteger(L"RekeyTime", GetRekeyTime()));

  if (GetSessionVersion() < GetVersionNumber2121())
  {
    SetFSProtocol(TranslateFSProtocolNumber(Storage->ReadInteger(L"FSProtocol", GetFSProtocol())));
  }
  else
  {
    SetFSProtocol(TranslateFSProtocol(Storage->ReadString(L"FSProtocol", GetFSProtocolStr())));
  }
  SetLocalDirectory(Storage->ReadString(L"LocalDirectory", GetLocalDirectory()));
  SetRemoteDirectory(Storage->ReadString(L"RemoteDirectory", GetRemoteDirectory()));
  SetSynchronizeBrowsing(Storage->ReadBool(L"SynchronizeBrowsing", GetSynchronizeBrowsing()));
  SetUpdateDirectories(Storage->ReadBool(L"UpdateDirectories", GetUpdateDirectories()));
  SetCacheDirectories(Storage->ReadBool(L"CacheDirectories", GetCacheDirectories()));
  SetCacheDirectoryChanges(Storage->ReadBool(L"CacheDirectoryChanges", GetCacheDirectoryChanges()));
  SetPreserveDirectoryChanges(Storage->ReadBool(L"PreserveDirectoryChanges", GetPreserveDirectoryChanges()));

  SetResolveSymlinks(Storage->ReadBool(L"ResolveSymlinks", GetResolveSymlinks()));
  SetDSTMode(static_cast<TDSTMode>(Storage->ReadInteger(L"ConsiderDST", GetDSTMode())));
  SetLockInHome(Storage->ReadBool(L"LockInHome", GetLockInHome()));
  SetSpecial(Storage->ReadBool(L"Special", GetSpecial()));
  SetShell(Storage->ReadString(L"Shell", GetShell()));
  SetClearAliases(Storage->ReadBool(L"ClearAliases", GetClearAliases()));
  SetUnsetNationalVars(Storage->ReadBool(L"UnsetNationalVars", GetUnsetNationalVars()));
  SetListingCommand(Storage->ReadString(L"ListingCommand",
    Storage->ReadBool(L"AliasGroupList", false) ? UnicodeString(L"ls -gla") : GetListingCommand()));
  SetIgnoreLsWarnings(Storage->ReadBool(L"IgnoreLsWarnings", GetIgnoreLsWarnings()));
  SetSCPLsFullTime(static_cast<TAutoSwitch>(Storage->ReadInteger(L"SCPLsFullTime", GetSCPLsFullTime())));
  SetScp1Compatibility(Storage->ReadBool(L"Scp1Compatibility", GetScp1Compatibility()));
  SetTimeDifference(TDateTime(Storage->ReadFloat(L"TimeDifference", GetTimeDifference())));
  SetDeleteToRecycleBin(Storage->ReadBool(L"DeleteToRecycleBin", GetDeleteToRecycleBin()));
  SetOverwrittenToRecycleBin(Storage->ReadBool(L"OverwrittenToRecycleBin", GetOverwrittenToRecycleBin()));
  SetRecycleBinPath(Storage->ReadString(L"RecycleBinPath", GetRecycleBinPath()));
  SetPostLoginCommands(Storage->ReadString(L"PostLoginCommands", GetPostLoginCommands()));

  SetReturnVar(Storage->ReadString(L"ReturnVar", GetReturnVar()));
  SetLookupUserGroups(TAutoSwitch(Storage->ReadInteger(L"LookupUserGroups", GetLookupUserGroups())));
  SetEOLType(static_cast<TEOLType>(Storage->ReadInteger(L"EOLType", GetEOLType())));
  SetNotUtf(static_cast<TAutoSwitch>(Storage->ReadInteger(L"Utf", Storage->ReadInteger(L"SFTPUtfBug", GetNotUtf()))));

  SetTcpNoDelay(Storage->ReadBool(L"TcpNoDelay", GetTcpNoDelay()));
  SetSendBuf(Storage->ReadInteger(L"SendBuf", Storage->ReadInteger(L"SshSendBuf", GetSendBuf())));
  SetSshSimple(Storage->ReadBool(L"SshSimple", GetSshSimple()));

  SetProxyMethod(static_cast<TProxyMethod>(Storage->ReadInteger(L"ProxyMethod", -1)));
  if (GetProxyMethod() < 0)
  {
    intptr_t ProxyType = Storage->ReadInteger(L"ProxyType", pxNone);
    intptr_t ProxySOCKSVersion = 0;
    switch (ProxyType)
    {
      case pxHTTP:
        SetProxyMethod(pmHTTP);
        break;
      case pxTelnet:
        SetProxyMethod(pmTelnet);
        break;
      case pxSocks:
        ProxySOCKSVersion = Storage->ReadInteger(L"ProxySOCKSVersion", 5);
        SetProxyMethod(ProxySOCKSVersion == 5 ? pmSocks5 : pmSocks4);
        break;
      default:
      case pxNone:
        SetProxyMethod(::pmNone);
        break;
    }
  }
  if (GetProxyMethod() != pmSystem)
  {
    SetProxyHost(Storage->ReadString(L"ProxyHost", GetProxyHost()));
    SetProxyPort(Storage->ReadInteger(L"ProxyPort", GetProxyPort()));
  }
  SetProxyUsername(Storage->ReadString(L"ProxyUsername", GetProxyUsername()));
  if (Storage->ValueExists(L"ProxyPassword"))
  {
    // encrypt unencrypted password
    SetProxyPassword(Storage->ReadString(L"ProxyPassword", L""));
  }
  else
  {
    // load encrypted password
    FProxyPassword = Storage->ReadStringAsBinaryData(L"ProxyPasswordEnc", FProxyPassword);
  }
  if (GetProxyMethod() == pmCmd)
  {
    SetProxyLocalCommand(Storage->ReadStringRaw(L"ProxyTelnetCommand", GetProxyLocalCommand()));
  }
  else
  {
    SetProxyTelnetCommand(Storage->ReadStringRaw(L"ProxyTelnetCommand", GetProxyTelnetCommand()));
  }
  SetProxyDNS(static_cast<TAutoSwitch>((Storage->ReadInteger(L"ProxyDNS", (GetProxyDNS() + 2) % 3) + 1) % 3));
  SetProxyLocalhost(Storage->ReadBool(L"ProxyLocalhost", GetProxyLocalhost()));

  #define READ_BUG(BUG) \
    SetBug(sb##BUG, TAutoSwitch(2 - Storage->ReadInteger(TEXT("Bug"#BUG), \
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
  #undef READ_BUG

  if ((GetBug(sbHMAC2) == asAuto) &&
      Storage->ReadBool(L"BuggyMAC", false))
  {
    SetBug(sbHMAC2, asOn);
  }

  SetSftpServer(Storage->ReadString(L"SftpServer", GetSftpServer()));
  #define READ_SFTP_BUG(BUG) \
    SetSFTPBug(sb##BUG, TAutoSwitch(Storage->ReadInteger(TEXT("SFTP" #BUG "Bug"), GetSFTPBug(sb##BUG))));
  READ_SFTP_BUG(Symlink);
  READ_SFTP_BUG(SignedTS);
  #undef READ_SFTP_BUG

  SetSFTPMaxVersion(Storage->ReadInteger(L"SFTPMaxVersion", GetSFTPMaxVersion()));
  SetSFTPMinPacketSize(Storage->ReadInteger(L"SFTPMinPacketSize", GetSFTPMinPacketSize()));
  SetSFTPMaxPacketSize(Storage->ReadInteger(L"SFTPMaxPacketSize", GetSFTPMaxPacketSize()));

  SetColor(Storage->ReadInteger(L"Color", GetColor()));

  // ???
  // SetProtocolStr(Storage->ReadString(L"Protocol", GetProtocolStr()));

  SetPuttyProtocol(Storage->ReadString(L"Protocol", GetPuttyProtocol()));

  SetTunnel(Storage->ReadBool(L"Tunnel", GetTunnel()));
  SetTunnelPortNumber(Storage->ReadInteger(L"TunnelPortNumber", GetTunnelPortNumber()));
  SetTunnelUserName(Storage->ReadString(L"TunnelUserName", GetTunnelUserName()));
  // must be loaded after TunnelUserName,
  // because TunnelHostName may be in format user@host
  SetTunnelHostName(Storage->ReadString(L"TunnelHostName", GetTunnelHostName()));
  if (!GetConfiguration()->GetDisablePasswordStoring())
  {
    if (Storage->ValueExists(L"TunnelPasswordPlain"))
    {
      SetTunnelPassword(Storage->ReadString(L"TunnelPasswordPlain", GetTunnelPassword()));
      RewritePassword = true;
    }
    else
    {
      FTunnelPassword = Storage->ReadStringAsBinaryData(L"TunnelPassword", FTunnelPassword);
    }
  }
  SetTunnelPublicKeyFile(Storage->ReadString(L"TunnelPublicKeyFile", GetTunnelPublicKeyFile()));
  SetTunnelLocalPortNumber(Storage->ReadInteger(L"TunnelLocalPortNumber", GetTunnelLocalPortNumber()));
  SetTunnelHostKey(Storage->ReadString(L"TunnelHostKey", GetTunnelHostKey()));

  // Ftp prefix
  SetFtpPasvMode(Storage->ReadBool(L"FtpPasvMode", GetFtpPasvMode()));
  SetFtpForcePasvIp(TAutoSwitch(Storage->ReadInteger(L"FtpForcePasvIp2", GetFtpForcePasvIp())));
  SetFtpUseMlsd(TAutoSwitch(Storage->ReadInteger(L"FtpUseMlsd", GetFtpUseMlsd())));
  SetFtpAccount(Storage->ReadString(L"FtpAccount", GetFtpAccount()));
  SetFtpPingInterval(Storage->ReadInteger(L"FtpPingInterval", GetFtpPingInterval()));
  SetFtpPingType(static_cast<TPingType>(Storage->ReadInteger(L"FtpPingType", GetFtpPingType())));
  SetFtps(static_cast<TFtps>(Storage->ReadInteger(L"Ftps", GetFtps())));
  SetFtpListAll(static_cast<TAutoSwitch>(Storage->ReadInteger(L"FtpListAll", GetFtpListAll())));
  SetFtpDupFF(Storage->ReadBool(L"FtpDupFF", GetFtpDupFF()));
  SetFtpUndupFF(Storage->ReadBool(L"FtpUndupFF", GetFtpUndupFF()));
  SetSslSessionReuse(Storage->ReadBool(L"SslSessionReuse", GetSslSessionReuse()));

  SetFtpProxyLogonType(Storage->ReadInteger(L"FtpProxyLogonType", GetFtpProxyLogonType()));

  SetMinTlsVersion(static_cast<TTlsVersion>(Storage->ReadInteger(L"MinTlsVersion", GetMinTlsVersion())));
  SetMaxTlsVersion(static_cast<TTlsVersion>(Storage->ReadInteger(L"MaxTlsVersion", GetMaxTlsVersion())));

  // SetIsWorkspace(Storage->ReadBool(L"IsWorkspace", GetIsWorkspace()));
  // SetLink(Storage->ReadString(L"Link", GetLink()));

  SetCustomParam1(Storage->ReadString(L"CustomParam1", GetCustomParam1()));
  SetCustomParam2(Storage->ReadString(L"CustomParam2", GetCustomParam2()));

  SetCodePage(Storage->ReadString(L"CodePage", GetCodePage()));
  SetLoginType(static_cast<TLoginType>(Storage->ReadInteger(L"LoginType", GetLoginType())));
  SetFtpAllowEmptyPassword(Storage->ReadBool(L"FtpAllowEmptyPassword", GetFtpAllowEmptyPassword()));
  if (GetSessionVersion() < GetVersionNumber2110())
  {
    SetFtps(TranslateFtpEncryptionNumber(Storage->ReadInteger(L"FtpEncryption", -1)));
  }
}
//---------------------------------------------------------------------
void TSessionData::Load(THierarchicalStorage * Storage)
{
  bool RewritePassword = false;
  if (Storage->OpenSubKey(GetInternalStorageKey(), False))
  {
    DoLoad(Storage, RewritePassword);

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
        Storage->DeleteValue(L"PasswordPlain");
        if (!GetPassword().IsEmpty())
        {
          Storage->WriteBinaryDataAsString(L"Password", FPassword);
        }
        Storage->DeleteValue(L"TunnelPasswordPlain");
        if (!GetTunnelPassword().IsEmpty())
        {
          Storage->WriteBinaryDataAsString(L"TunnelPassword", FTunnelPassword);
        }
        Storage->CloseSubKey();
      }
    }
    catch(...)
    {
      // ignore errors (like read-only INI file)
    }

    Storage->SetAccessMode(AccessMode);
  }

  FNumberOfRetries = 0;
  FModified = false;
  FSource = ssStored;
}
//---------------------------------------------------------------------
void TSessionData::Save(THierarchicalStorage * Storage,
  bool PuttyExport, const TSessionData * Default)
{
  if (Storage->OpenSubKey(GetInternalStorageKey(), true))
  {
    #define WRITE_DATA_EX(TYPE, NAME, PROPERTY, CONV) \
      if ((Default != nullptr) && (CONV(Default->PROPERTY) == CONV(PROPERTY))) \
      { \
        Storage->DeleteValue(NAME); \
      } \
      else \
      { \
        Storage->Write ## TYPE(NAME, CONV(PROPERTY)); \
      }
    #define WRITE_DATA_EX2(TYPE, NAME, PROPERTY, CONV) \
      { \
        Storage->Write ## TYPE(NAME, CONV(PROPERTY)); \
      }
    #define WRITE_DATA_CONV(TYPE, NAME, PROPERTY) WRITE_DATA_EX(TYPE, NAME, PROPERTY, WRITE_DATA_CONV_FUNC)
    #define WRITE_DATA(TYPE, PROPERTY) WRITE_DATA_EX(TYPE, TEXT(#PROPERTY), Get ## PROPERTY(), )

    Storage->WriteString(L"Version", ::VersionNumberToStr(::GetCurrentVersionNumber()));
    WRITE_DATA(String, HostName);
    WRITE_DATA(Integer, PortNumber);
    WRITE_DATA_EX(Integer, L"PingInterval", GetPingInterval() / SecsPerMin, );
    WRITE_DATA_EX(Integer, L"PingIntervalSecs", GetPingInterval() % SecsPerMin, );
    Storage->DeleteValue(L"PingIntervalSec"); // obsolete
    // when PingInterval is stored always store PingType not to attempt to
    // deduce PingType from PingInterval (backward compatibility with pre 3.5)
    if (((Default != nullptr) && (GetPingType() != Default->GetPingType())) ||
        Storage->ValueExists(L"PingInterval"))
    {
      Storage->WriteInteger(L"PingType", GetPingType());
    }
    else
    {
      Storage->DeleteValue(L"PingType");
    }
    WRITE_DATA(Integer, Timeout);
    WRITE_DATA(Bool, TryAgent);
    WRITE_DATA(Bool, AgentFwd);
    WRITE_DATA(Bool, AuthTIS);
    WRITE_DATA(Bool, AuthKI);
    WRITE_DATA(Bool, AuthKIPassword);

    WRITE_DATA(Bool, AuthGSSAPI);
    WRITE_DATA(Bool, GSSAPIFwdTGT);
    WRITE_DATA(String, GSSAPIServerRealm);
    Storage->DeleteValue(L"TryGSSKEX");
    Storage->DeleteValue(L"UserNameFromEnvironment");
    Storage->DeleteValue(L"GSSAPIServerChoosesUserName");
    Storage->DeleteValue(L"GSSAPITrustDNS");
    if (PuttyExport)
    {
      // duplicate kerberos setting with keys of the vintela quest putty
      WRITE_DATA_EX(Bool, L"AuthSSPI", GetAuthGSSAPI(), );
      WRITE_DATA_EX(Bool, L"SSPIFwdTGT", GetGSSAPIFwdTGT(), );
      WRITE_DATA_EX(String, L"KerbPrincipal", GetGSSAPIServerRealm(), );
      // duplicate kerberos setting with keys of the official putty
      WRITE_DATA_EX(Bool, L"GssapiFwd", GetGSSAPIFwdTGT(), );
    }

    WRITE_DATA(Bool, ChangeUsername);
    WRITE_DATA(Bool, Compression);
    WRITE_DATA(Integer, SshProt);
    WRITE_DATA(Bool, Ssh2DES);
    WRITE_DATA(Bool, SshNoUserAuth);
    WRITE_DATA_EX(String, L"Cipher", GetCipherList(), );
    WRITE_DATA_EX(String, L"KEX", GetKexList(), );
    WRITE_DATA(Integer, AddressFamily);
    WRITE_DATA_EX(String, L"RekeyBytes", GetRekeyData(), );
    WRITE_DATA(Integer, RekeyTime);

    WRITE_DATA(Bool, TcpNoDelay);

    if (PuttyExport)
    {
      WRITE_DATA(StringRaw, UserName);
      WRITE_DATA(StringRaw, PublicKeyFile);
    }
    else
    {
      WRITE_DATA(String, UserName);
      WRITE_DATA(String, PublicKeyFile);
      WRITE_DATA_EX2(String, L"FSProtocol", GetFSProtocolStr(), );
      WRITE_DATA(String, LocalDirectory);
      WRITE_DATA(String, RemoteDirectory);
      WRITE_DATA(Bool, SynchronizeBrowsing);
      WRITE_DATA(Bool, UpdateDirectories);
      WRITE_DATA(Bool, CacheDirectories);
      WRITE_DATA(Bool, CacheDirectoryChanges);
      WRITE_DATA(Bool, PreserveDirectoryChanges);

      WRITE_DATA(Bool, ResolveSymlinks);
      WRITE_DATA_EX(Integer, L"ConsiderDST", GetDSTMode(), );
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
      WRITE_DATA(Float, TimeDifference);
      WRITE_DATA(Bool, DeleteToRecycleBin);
      WRITE_DATA(Bool, OverwrittenToRecycleBin);
      WRITE_DATA(String, RecycleBinPath);
      WRITE_DATA(String, PostLoginCommands);

      WRITE_DATA(String, ReturnVar);
      WRITE_DATA_EX(Integer, L"LookupUserGroups2", GetLookupUserGroups(), );
      WRITE_DATA(Integer, EOLType);
      Storage->DeleteValue(L"SFTPUtfBug");
      WRITE_DATA_EX(Integer, L"Utf", GetNotUtf(), );
      WRITE_DATA(Integer, SendBuf);
      WRITE_DATA(Bool, SshSimple);
    }

    WRITE_DATA(Integer, ProxyMethod);
    if (PuttyExport)
    {
      // support for Putty 0.53b and older
      int ProxyType;
      int ProxySOCKSVersion = 5;
      switch (GetProxyMethod())
      {
        case pmHTTP:
          ProxyType = pxHTTP;
          break;
        case pmTelnet:
          ProxyType = pxTelnet;
          break;
        case pmSocks5:
          ProxyType = pxSocks;
          ProxySOCKSVersion = 5;
          break;
        case pmSocks4:
          ProxyType = pxSocks;
          ProxySOCKSVersion = 4;
          break;
        default:
        case ::pmNone:
          ProxyType = pxNone;
          break;
      }
      Storage->WriteInteger(L"ProxyType", ProxyType);
      Storage->WriteInteger(L"ProxySOCKSVersion", ProxySOCKSVersion);
    }
    else
    {
      Storage->DeleteValue(L"ProxyType");
      Storage->DeleteValue(L"ProxySOCKSVersion");
    }
    if (GetProxyMethod() != pmSystem)
    {
      WRITE_DATA(String, ProxyHost);
      WRITE_DATA(Integer, ProxyPort);
    }
    WRITE_DATA(String, ProxyUsername);
    if (GetProxyMethod() == pmCmd)
    {
      WRITE_DATA_EX(StringRaw, L"ProxyTelnetCommand", GetProxyLocalCommand(), );
    }
    else
    {
      WRITE_DATA_EX(StringRaw, L"ProxyTelnetCommand", GetProxyTelnetCommand(), );
    }
    #define WRITE_DATA_CONV_FUNC(X) (((X) + 2) % 3)
    WRITE_DATA_CONV(Integer, L"ProxyDNS", GetProxyDNS());
    #undef WRITE_DATA_CONV_FUNC
    WRITE_DATA_EX(Bool, L"ProxyLocalhost", GetProxyLocalhost(), );

    #define WRITE_DATA_CONV_FUNC(X) (2 - (X))
    #define WRITE_BUG(BUG) WRITE_DATA_CONV(Integer, TEXT("Bug" #BUG), GetBug(sb##BUG));
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
    #undef WRITE_BUG
    #undef WRITE_DATA_CONV_FUNC

    Storage->DeleteValue(L"BuggyMAC");
    Storage->DeleteValue(L"AliasGroupList");

    if (PuttyExport)
    {
      WRITE_DATA_EX(String, L"Protocol", GetPuttyProtocol(), );
    }

    if (!PuttyExport)
    {
      WRITE_DATA(String, SftpServer);

      #define WRITE_SFTP_BUG(BUG) WRITE_DATA_EX(Integer, TEXT("SFTP" #BUG "Bug"), GetSFTPBug(sb##BUG), );
      WRITE_SFTP_BUG(Symlink);
      WRITE_SFTP_BUG(SignedTS);
      #undef WRITE_SFTP_BUG

      WRITE_DATA(Integer, SFTPMaxVersion);
      WRITE_DATA(Integer, SFTPMaxPacketSize);
      WRITE_DATA(Integer, SFTPMinPacketSize);

      WRITE_DATA(Integer, Color);

      WRITE_DATA(Bool, Tunnel);
      WRITE_DATA(String, TunnelHostName);
      WRITE_DATA(Integer, TunnelPortNumber);
      WRITE_DATA(String, TunnelUserName);
      WRITE_DATA(String, TunnelPublicKeyFile);
      WRITE_DATA(Integer, TunnelLocalPortNumber);

      WRITE_DATA(Bool, FtpPasvMode);
      WRITE_DATA_EX(Integer, L"FtpForcePasvIp2", GetFtpForcePasvIp(), );
      WRITE_DATA(Integer, FtpUseMlsd);
      WRITE_DATA(String, FtpAccount);
      WRITE_DATA(Integer, FtpPingInterval);
      WRITE_DATA(Integer, FtpPingType);
      WRITE_DATA(Integer, Ftps);
      WRITE_DATA(Integer, FtpListAll);
      WRITE_DATA(Bool, FtpDupFF);
      WRITE_DATA(Bool, FtpUndupFF);
      WRITE_DATA(Bool, SslSessionReuse);

      WRITE_DATA(Integer, FtpProxyLogonType);

      WRITE_DATA(Integer, MinTlsVersion);
      WRITE_DATA(Integer, MaxTlsVersion);

      // WRITE_DATA(Bool, IsWorkspace);
      // WRITE_DATA(String, Link);

      WRITE_DATA(String, CustomParam1);
      WRITE_DATA(String, CustomParam2);

      WRITE_DATA_EX(String, L"CodePage", GetCodePage(), );
      WRITE_DATA_EX(Integer, L"LoginType", GetLoginType(), );
      WRITE_DATA_EX(Bool, L"FtpAllowEmptyPassword", GetFtpAllowEmptyPassword(), );
    }

    SavePasswords(Storage, PuttyExport);

    Storage->CloseSubKey();
  }
}
//---------------------------------------------------------------------
/*UnicodeString TSessionData::ReadXmlNode(_di_IXMLNode Node, const UnicodeString & Name, const UnicodeString & Default)
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
int TSessionData::ReadXmlNode(_di_IXMLNode Node, const UnicodeString & Name, int Default)
{
  _di_IXMLNode TheNode = Node->ChildNodes->FindNode(Name);
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
void TSessionData::ImportFromFilezilla(_di_IXMLNode Node, const UnicodeString & Path)
{
  Name = UnixIncludeTrailingBackslash(Path) + ReadXmlNode(Node, L"Name", Name);
  HostName = ReadXmlNode(Node, L"Host", HostName);
  PortNumber = ReadXmlNode(Node, L"Port", PortNumber);

  int AProtocol = ReadXmlNode(Node, L"Protocol", 0);
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
      Ftps = ftpsExplicitSsl;
      break;
  }

  // LogonType enum
  int LogonType = ReadXmlNode(Node, L"Logontype", 0);
  if (LogonType == 0) // ANONYMOUS
  {
    UserName = AnonymousUserName;
    Password = AnonymousPassword;
  }
  else
  {
    UserName = ReadXmlNode(Node, L"User", UserName);
    Password = ReadXmlNode(Node, L"Pass", Password);
    FtpAccount = ReadXmlNode(Node, L"Account", FtpAccount);
  }

  int DefaultTimeDifference = TimeToMSec(TimeDifference) / MSecsPerSec;
  TimeDifference =
    (double(ReadXmlNode(Node, L"TimezoneOffset", DefaultTimeDifference) / SecsPerDay));

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

  LocalDirectory = ReadXmlNode(Node, L"LocalDir", LocalDirectory);

  UnicodeString RemoteDir = ReadXmlNode(Node, L"RemoteDir", L"");
  if (!RemoteDir.IsEmpty())
  {
    CutToChar(RemoteDir, L' ', false); // type
    int PrefixSize = StrToIntDef(CutToChar(RemoteDir, L' ', false), 0); // prefix size
    if (PrefixSize > 0)
    {
      RemoteDir.Delete(1, PrefixSize);
    }
    RemoteDirectory = L"/";
    while (!RemoteDir.IsEmpty())
    {
      int SegmentSize = StrToIntDef(CutToChar(RemoteDir, L' ', false), 0);
      UnicodeString Segment = RemoteDir.SubString(1, SegmentSize);
      RemoteDirectory = UnixIncludeTrailingBackslash(RemoteDirectory) + Segment;
      RemoteDir.Delete(1, SegmentSize + 1);
    }
  }

  SynchronizeBrowsing = (ReadXmlNode(Node, L"SyncBrowsing", SynchronizeBrowsing ? 1 : 0) != 0);

}*/
//---------------------------------------------------------------------
void TSessionData::SavePasswords(THierarchicalStorage * Storage, bool PuttyExport)
{
  if (!GetConfiguration()->GetDisablePasswordStoring() && !PuttyExport && !FPassword.IsEmpty())
  {
    Storage->WriteBinaryDataAsString(L"Password", StronglyRecryptPassword(FPassword, GetUserName() + GetHostName()));
  }
  else
  {
    Storage->DeleteValue(L"Password");
  }
  Storage->DeleteValue(L"PasswordPlain");

  if (PuttyExport)
  {
    // save password unencrypted
    Storage->WriteString(L"ProxyPassword", GetProxyPassword());
  }
  else
  {
    // save password encrypted
    if (!FProxyPassword.IsEmpty())
    {
      Storage->WriteBinaryDataAsString(L"ProxyPasswordEnc", StronglyRecryptPassword(FProxyPassword, GetProxyUsername() + GetProxyHost()));
    }
    else
    {
      Storage->DeleteValue(L"ProxyPasswordEnc");
    }
    Storage->DeleteValue(L"ProxyPassword");

    if (!GetConfiguration()->GetDisablePasswordStoring() && !FTunnelPassword.IsEmpty())
    {
      Storage->WriteBinaryDataAsString(L"TunnelPassword", StronglyRecryptPassword(FTunnelPassword, GetTunnelUserName() + GetTunnelHostName()));
    }
    else
    {
      Storage->DeleteValue(L"TunnelPassword");
    }
  }
}
//---------------------------------------------------------------------
void TSessionData::RecryptPasswords()
{
  SetPassword(GetPassword());
  SetProxyPassword(GetProxyPassword());
  SetTunnelPassword(GetTunnelPassword());
}
//---------------------------------------------------------------------
bool TSessionData::HasPassword() const
{
  return !FPassword.IsEmpty();
}
//---------------------------------------------------------------------
bool TSessionData::HasAnyPassword() const
{
  return HasPassword() || !FProxyPassword.IsEmpty() || !FTunnelPassword.IsEmpty();
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
      assert(false);
      return L"";
  }
}
//---------------------------------------------------------------------
void TSessionData::SaveRecryptedPasswords(THierarchicalStorage * Storage)
{
  if (Storage->OpenSubKey(GetInternalStorageKey(), true))
  {
    auto cleanup = finally([&]()
    {
      Storage->CloseSubKey();
    });
    {
      RecryptPasswords();

      SavePasswords(Storage, false);
    }
  }
}
//---------------------------------------------------------------------
void TSessionData::Remove()
{
  std::auto_ptr<THierarchicalStorage> Storage(GetConfiguration()->CreateStorage(true));
  Storage->SetExplicit(true);
  if (Storage->OpenSubKey(GetConfiguration()->GetStoredSessionsSubKey(), false))
  {
    Storage->RecursiveDeleteSubKey(GetInternalStorageKey());
  }
}
//---------------------------------------------------------------------
inline void MoveStr(UnicodeString & Source, UnicodeString * Dest, intptr_t Count)
{
  if (Dest != nullptr)
  {
    (*Dest) += Source.SubString(1, Count);
  }

  Source.Delete(1, Count);
}
//---------------------------------------------------------------------
bool TSessionData::ParseUrl(const UnicodeString & Url, TOptions * Options,
  TStoredSessionList * StoredSessions, bool & DefaultsOnly, UnicodeString * FileName,
  bool * AProtocolDefined, UnicodeString * MaskedUrl)
{
  UnicodeString url = Url;
  bool ProtocolDefined = false;
  bool PortNumberDefined = false;
  TFSProtocol AFSProtocol = fsSCPonly;
  intptr_t APortNumber = 0;
  TFtps AFtps = ftpsNone;
  if (url.SubString(1, 7).LowerCase() == L"netbox:")
  {
    // Remove "netbox:" prefix
    url.Delete(1, 7);
  }
  if (url.SubString(1, 7).LowerCase() == L"webdav:")
  {
    AFSProtocol = fsWebDAV;
    AFtps = ftpsNone;
    APortNumber = HTTPPortNumber;
    url.Delete(1, 7);
    ProtocolDefined = true;
  }
  if (url.SubString(1, 4).LowerCase() == L"scp:")
  {
    AFSProtocol = fsSCPonly;
    APortNumber = SshPortNumber;
    MoveStr(url, MaskedUrl, 4);
    ProtocolDefined = true;
  }
  else if (url.SubString(1, 5).LowerCase() == L"sftp:")
  {
    AFSProtocol = fsSFTPonly;
    APortNumber = SshPortNumber;
    MoveStr(url, MaskedUrl, 5);
    ProtocolDefined = true;
  }
  else if (url.SubString(1, 4).LowerCase() == L"ftp:")
  {
    AFSProtocol = fsFTP;
    SetFtps(ftpsNone);
    APortNumber = FtpPortNumber;
    MoveStr(url, MaskedUrl, 4);
    ProtocolDefined = true;
  }
  else if (url.SubString(1, 5).LowerCase() == L"ftps:")
  {
    AFSProtocol = fsFTP;
    AFtps = ftpsImplicit;
    APortNumber = FtpsImplicitPortNumber;
    MoveStr(url, MaskedUrl, 5);
    ProtocolDefined = true;
  }
  else if (url.SubString(1, 5).LowerCase() == L"http:")
  {
    AFSProtocol = fsWebDAV;
    AFtps = ftpsNone;
    APortNumber = HTTPPortNumber;
    MoveStr(url, MaskedUrl, 5);
    ProtocolDefined = true;
  }
  else if (url.SubString(1, 6).LowerCase() == L"https:")
  {
    AFSProtocol = fsWebDAV;
    AFtps = ftpsImplicit;
    APortNumber = HTTPSPortNumber;
    MoveStr(url, MaskedUrl, 6);
    ProtocolDefined = true;
  }

  if (ProtocolDefined && (url.SubString(1, 2) == L"//"))
  {
    MoveStr(url, MaskedUrl, 2);
  }

  if (AProtocolDefined != nullptr)
  {
    *AProtocolDefined = ProtocolDefined;
  }

  if (!url.IsEmpty())
  {
    UnicodeString DecodedUrl = DecodeUrlChars(url);
    // lookup stored session even if protocol was defined
    // (this allows setting for example default username for host
    // by creating stored session named by host)
    TSessionData * Data = nullptr;
    for (Integer Index = 0; Index < StoredSessions->GetCount() + StoredSessions->GetHiddenCount(); ++Index)
    {

      TSessionData * AData = static_cast<TSessionData *>(StoredSessions->GetItem(Index));
      if (// !AData->GetIsWorkspace() &&
          AnsiSameText(AData->GetName(), DecodedUrl) ||
          AnsiSameText(AData->GetName() + L"/", DecodedUrl.SubString(1, AData->GetName().Length() + 1)))
      {
        Data = AData;
        break;
      }
    }

    UnicodeString ARemoteDirectory;

    if (Data != nullptr)
    {
      Assign(Data);
      intptr_t P = 1;
      while (!AnsiSameText(DecodeUrlChars(url.SubString(1, P)), Data->GetName()))
      {
        P++;
        assert(P <= url.Length());
      }
      ARemoteDirectory = url.SubString(P + 1, url.Length() - P);

      if (Data->GetHidden())
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
      Assign(StoredSessions->GetDefaultSettings());
      SetName(L"");

      intptr_t PSlash = url.Pos(L"/");
      if (PSlash == 0)
      {
        PSlash = url.Length() + 1;
      }

      UnicodeString ConnectInfo = url.SubString(1, PSlash - 1);

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
        SetPortNumber(StrToIntDef(DecodeUrlChars(HostInfo), -1));
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

      UnicodeString RawUserName = CutToChar(UserInfo, L':', false);
      SetUserName(DecodeUrlChars(RawUserName));

      SetPassword(DecodeUrlChars(UserInfo));

      ARemoteDirectory = url.SubString(PSlash, Url.Length() - PSlash + 1);

      if (MaskedUrl != nullptr)
      {
        (*MaskedUrl) += RawUserName;
        if (!UserInfo.IsEmpty())
        {
          (*MaskedUrl) += L":***";
        }
        (*MaskedUrl) += L"@" + OrigHostInfo + ARemoteDirectory;
      }

      if (PSlash <= url.Length())
      {
        ARemoteDirectory = url.SubString(PSlash, url.Length() - PSlash + 1);
      }
    }

    if (!ARemoteDirectory.IsEmpty() && (ARemoteDirectory != L"/"))
    {
      if ((ARemoteDirectory[ARemoteDirectory.Length()] != L'/') &&
          (FileName != nullptr))
      {
        *FileName = DecodeUrlChars(UnixExtractFileName(ARemoteDirectory));
        ARemoteDirectory = ::UnixExtractFilePath(ARemoteDirectory);
      }
      SetRemoteDirectory(DecodeUrlChars(ARemoteDirectory));
    }

    DefaultsOnly = false;
  }
  else
  {
    Assign(StoredSessions->GetDefaultSettings());

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
    if (Options->FindSwitch(L"privatekey", Value))
    {
      SetPublicKeyFile(Value);
    }
    if (Options->FindSwitch(L"timeout", Value))
    {
      SetTimeout(Sysutils::StrToInt(Value));
    }
    if (Options->FindSwitch(L"hostkey", Value) ||
        Options->FindSwitch(L"certificate", Value))
    {
      SetHostKey(Value);
    }
    SetFtpPasvMode(Options->SwitchValue(L"passive", GetFtpPasvMode()));
    if (Options->FindSwitch(L"implicit"))
    {
      bool Enabled = Options->SwitchValue(L"implicit", true);
      SetFtps(Enabled ? ftpsImplicit : ftpsNone);
      if (!PortNumberDefined && Enabled)
      {
        SetPortNumber(FtpsImplicitPortNumber);
      }
    }
    if (Options->FindSwitch(L"explicitssl", Value))
    {
      bool Enabled = Options->SwitchValue(L"explicitssl", true);
      SetFtps(Enabled ? ftpsExplicitSsl : ftpsNone);
      if (!PortNumberDefined && Enabled)
      {
        SetPortNumber(FtpPortNumber);
      }
    }
    if (Options->FindSwitch(L"explicittls", Value))
    {
      bool Enabled = Options->SwitchValue(L"explicittls", true);
      SetFtps(Enabled ? ftpsExplicitTls : ftpsNone);
      if (!PortNumberDefined && Enabled)
      {
        SetPortNumber(FtpPortNumber);
      }
    }
    if (Options->FindSwitch(L"rawsettings"))
    {
      std::auto_ptr<TStrings> RawSettings(new TStringList());
      std::auto_ptr<TRegistryStorage> OptionsStorage(nullptr);
      if (Options->FindSwitch(L"rawsettings", RawSettings.get()))
      {
        OptionsStorage.reset(new TRegistryStorage(GetConfiguration()->GetRegistryStorageKey()));

        bool Dummy;
        DoLoad(OptionsStorage.get(), Dummy);
      }
    }
    if (Options->FindSwitch(L"allowemptypassword", Value))
    {
      SetFtpAllowEmptyPassword((StrToIntDef(Value, 0) != 0));
    }
    if (Options->FindSwitch(L"explicitssl", Value))
    {
      bool Enabled = (StrToIntDef(Value, 1) != 0);
      SetFtps(Enabled ? ftpsExplicitSsl : ftpsNone);
      if (!PortNumberDefined && Enabled)
      {
        SetPortNumber(FtpPortNumber);
      }
    }
    if (Options->FindSwitch(L"username", Value))
    {
      if (!Value.IsEmpty())
      {
        SetUserName(Value);
      }
    }
    if (Options->FindSwitch(L"password", Value))
    {
      SetPassword(Value);
    }
    if (Options->FindSwitch(L"codepage", Value))
    {
      intptr_t CodePage = StrToIntDef(Value, 0);
      if (CodePage != 0)
      {
        SetCodePage(GetCodePageAsString(CodePage));
      }
    }
  }

  return true;
}
//---------------------------------------------------------------------
void TSessionData::ConfigureTunnel(intptr_t APortNumber)
{
  FOrigHostName = GetHostName();
  FOrigPortNumber = GetPortNumber();
  FOrigProxyMethod = GetProxyMethod();

  SetHostName(L"127.0.0.1");
  SetPortNumber(APortNumber);
  // proxy settings is used for tunnel
  SetProxyMethod(::pmNone);
}
//---------------------------------------------------------------------
void TSessionData::RollbackTunnel()
{
  SetHostName(FOrigHostName);
  SetPortNumber(FOrigPortNumber);
  SetProxyMethod(FOrigProxyMethod);
}
//---------------------------------------------------------------------
void TSessionData::ExpandEnvironmentVariables()
{
  SetHostName(GetHostNameExpanded());
  SetUserName(GetUserNameExpanded());
  SetPublicKeyFile(::ExpandEnvironmentVariables(GetPublicKeyFile()));
}
//---------------------------------------------------------------------
void TSessionData::ValidatePath(const UnicodeString & /*Path*/)
{
  // noop
}
//---------------------------------------------------------------------
void TSessionData::ValidateName(const UnicodeString & Name)
{
  if (Name.LastDelimiter(L"/") > 0)
  {
    throw Exception(FMTLOAD(ITEM_NAME_INVALID, Name.c_str(), L"/"));
  }
}
//---------------------------------------------------------------------
RawByteString TSessionData::EncryptPassword(const UnicodeString & Password, const UnicodeString & Key)
{
  return GetConfiguration()->EncryptPassword(Password, Key);
}
//---------------------------------------------------------------------
RawByteString TSessionData::StronglyRecryptPassword(const RawByteString & Password, const UnicodeString & Key)
{
  return GetConfiguration()->StronglyRecryptPassword(Password, Key);
}
//---------------------------------------------------------------------
UnicodeString TSessionData::DecryptPassword(const RawByteString & Password, const UnicodeString & Key)
{
  UnicodeString Result;
  try
  {
    Result = GetConfiguration()->DecryptPassword(Password, Key);
  }
  catch(EAbort &)
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
//---------------------------------------------------------------------------
UnicodeString TSessionData::GetSessionKey() const
{
  return FORMAT(L"%s@%s", GetUserName().c_str(), GetHostName().c_str());
}
//---------------------------------------------------------------------
UnicodeString TSessionData::GetInternalStorageKey() const
{
  if (GetName().IsEmpty())
  {
    return GetSessionKey();
  }
  else
  {
    return GetName();
  }
}
//---------------------------------------------------------------------
UnicodeString TSessionData::GetStorageKey() const
{
  return GetSessionName();
}
//---------------------------------------------------------------------
void TSessionData::SetHostName(const UnicodeString & Value)
{
  if (FHostName != Value)
  {
    UnicodeString Value2 = Value;
    RemoveProtocolPrefix(Value2);
    // remove path
    {
      intptr_t Pos = 1;
      Value2 = CopyToChars(Value2, Pos, L"/", true, nullptr, false);
    }
    // HostName is key for password encryption
    UnicodeString XPassword = GetPassword();

    intptr_t P = Value2.LastDelimiter(L"@");
    if (P > 0)
    {
      SetUserName(Value2.SubString(1, P - 1));
      Value2 = Value2.SubString(P + 1, Value2.Length() - P);
    }
    FHostName = Value2;
    Modify();

    SetPassword(XPassword);
    Shred(XPassword);
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
//---------------------------------------------------------------------------
void TSessionData::SetShell(const UnicodeString & Value)
{
  SET_SESSION_PROPERTY(Shell);
}
//---------------------------------------------------------------------------
void TSessionData::SetSftpServer(const UnicodeString & Value)
{
  SET_SESSION_PROPERTY(SftpServer);
}
//---------------------------------------------------------------------------
void TSessionData::SetClearAliases(bool Value)
{
  SET_SESSION_PROPERTY(ClearAliases);
}
//---------------------------------------------------------------------------
void TSessionData::SetListingCommand(const UnicodeString & Value)
{
  SET_SESSION_PROPERTY(ListingCommand);
}
//---------------------------------------------------------------------------
void TSessionData::SetIgnoreLsWarnings(bool Value)
{
  SET_SESSION_PROPERTY(IgnoreLsWarnings);
}
//---------------------------------------------------------------------------
void TSessionData::SetUnsetNationalVars(bool Value)
{
  SET_SESSION_PROPERTY(UnsetNationalVars);
}
//---------------------------------------------------------------------
void TSessionData::SetUserName(const UnicodeString & Value)
{
  // UserName is key for password encryption
  UnicodeString XPassword = GetPassword();
  SET_SESSION_PROPERTY(UserName);
  SetPassword(XPassword);
  Shred(XPassword);
}
//---------------------------------------------------------------------
UnicodeString TSessionData::GetUserNameExpanded() const
{
  return ::ExpandEnvironmentVariables(GetUserName());
}
//---------------------------------------------------------------------
void TSessionData::SetPassword(const UnicodeString & AValue)
{
  RawByteString Value = EncryptPassword(AValue, GetUserName() + GetHostName());
  SET_SESSION_PROPERTY(Password);
}
//---------------------------------------------------------------------
UnicodeString TSessionData::GetPassword() const
{
  return DecryptPassword(FPassword, GetUserName() + GetHostName());
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
void TSessionData::SetGSSAPIServerRealm(const UnicodeString & Value)
{
  SET_SESSION_PROPERTY(GSSAPIServerRealm);
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
  return IsSshProtocol(GetFSProtocol());
}
//---------------------------------------------------------------------
void TSessionData::SetCipher(intptr_t Index, TCipher Value)
{
  assert(Index >= 0 && Index < CIPHER_COUNT);
  SET_SESSION_PROPERTY(Ciphers[Index]);
}
//---------------------------------------------------------------------
TCipher TSessionData::GetCipher(intptr_t Index) const
{
  assert(Index >= 0 && Index < CIPHER_COUNT);
  return FCiphers[Index];
}
//---------------------------------------------------------------------
void TSessionData::SetCipherList(const UnicodeString & Value)
{
  bool Used[CIPHER_COUNT];
  for (intptr_t C = 0; C < CIPHER_COUNT; C++)
  {
    Used[C] = false;
  }

  UnicodeString CipherStr;
  intptr_t Index = 0;
  UnicodeString Value2 = Value;
  while (!Value2.IsEmpty() && (Index < CIPHER_COUNT))
  {
    CipherStr = CutToChar(Value2, L',', true);
    for (int C = 0; C < CIPHER_COUNT; C++)
    {
      if (!CipherStr.CompareIC(CipherNames[C]))
      {
        SetCipher(Index, static_cast<TCipher>(C));
        Used[C] = true;
        ++Index;
        break;
      }
    }
  }

  for (intptr_t C = 0; C < CIPHER_COUNT && Index < CIPHER_COUNT; C++)
  {
    if (!Used[DefaultCipherList[C]])
    {
      SetCipher(++Index, DefaultCipherList[C]);
    }
  }
}
//---------------------------------------------------------------------
UnicodeString TSessionData::GetCipherList() const
{
  UnicodeString Result;
  for (intptr_t Index = 0; Index < CIPHER_COUNT; ++Index)
  {
    Result += UnicodeString(Index ? L"," : L"") + CipherNames[GetCipher(Index)];
  }
  return Result;
}
//---------------------------------------------------------------------
void TSessionData::SetKex(intptr_t Index, TKex Value)
{
  assert(Index >= 0 && Index < KEX_COUNT);
  SET_SESSION_PROPERTY(Kex[Index]);
}
//---------------------------------------------------------------------
TKex TSessionData::GetKex(intptr_t Index) const
{
  assert(Index >= 0 && Index < KEX_COUNT);
  return FKex[Index];
}
//---------------------------------------------------------------------
void TSessionData::SetKexList(const UnicodeString & Value)
{
  bool Used[KEX_COUNT];
  for (int K = 0; K < KEX_COUNT; K++)
  {
    Used[K] = false;
  }

  UnicodeString KexStr;
  intptr_t Index = 0;
  UnicodeString Value2 = Value;
  while (!Value2.IsEmpty() && (Index < KEX_COUNT))
  {
    KexStr = CutToChar(Value2, L',', true);
    for (int K = 0; K < KEX_COUNT; K++)
    {
      if (!KexStr.CompareIC(KexNames[K]))
      {
        SetKex(Index, static_cast<TKex>(K));
        Used[K] = true;
        ++Index;
        break;
      }
    }
  }

  for (intptr_t K = 0; K < KEX_COUNT && Index < KEX_COUNT; K++)
  {
    if (!Used[DefaultKexList[K]])
    {
      SetKex(++Index, DefaultKexList[K]);
    }
  }
}
//---------------------------------------------------------------------
UnicodeString TSessionData::GetKexList() const
{
  UnicodeString Result;
  for (intptr_t Index = 0; Index < KEX_COUNT; ++Index)
  {
    Result += UnicodeString(Index ? L"," : L"") + KexNames[GetKex(Index)];
  }
  return Result;
}
//---------------------------------------------------------------------
void TSessionData::SetPublicKeyFile(const UnicodeString & Value)
{
  if (FPublicKeyFile != Value)
  {
    FPublicKeyFile = StripPathQuotes(Value);
    Modify();
  }
}
//---------------------------------------------------------------------
void TSessionData::SetReturnVar(const UnicodeString & Value)
{
  SET_SESSION_PROPERTY(ReturnVar);
}
//---------------------------------------------------------------------------
void TSessionData::SetLookupUserGroups(TAutoSwitch Value)
{
  SET_SESSION_PROPERTY(LookupUserGroups);
}
//---------------------------------------------------------------------------
void TSessionData::SetEOLType(TEOLType Value)
{
  SET_SESSION_PROPERTY(EOLType);
}
//---------------------------------------------------------------------------
TDateTime TSessionData::GetTimeoutDT()
{
  return SecToDateTime(GetTimeout());
}
//---------------------------------------------------------------------------
void TSessionData::SetTimeout(intptr_t Value)
{
  SET_SESSION_PROPERTY(Timeout);
}
//---------------------------------------------------------------------------
//void TSessionData::SetProtocol(TProtocol Value)
//{
//  SET_SESSION_PROPERTY(Protocol);
//}
//---------------------------------------------------------------------------
void TSessionData::SetFSProtocol(TFSProtocol Value)
{
  SET_SESSION_PROPERTY(FSProtocol);
}
//---------------------------------------------------------------------
UnicodeString TSessionData::GetFSProtocolStr() const
{
  UnicodeString Result;
  assert(GetFSProtocol() >= 0);
  if (GetFSProtocol() < FSPROTOCOL_COUNT)
  {
    Result = FSProtocolNames[GetFSProtocol()];
  }
  // assert(!Result.IsEmpty());
  if (Result.IsEmpty())
    Result = FSProtocolNames[CONST_DEFAULT_PROTOCOL];
  return Result;
}
//---------------------------------------------------------------------------
void TSessionData::SetDetectReturnVar(bool Value)
{
  if (Value != GetDetectReturnVar())
  {
    SetReturnVar(Value ? L"" : L"$?");
  }
}
//---------------------------------------------------------------------------
bool TSessionData::GetDetectReturnVar() const
{
  return GetReturnVar().IsEmpty();
}
//---------------------------------------------------------------------------
void TSessionData::SetDefaultShell(bool Value)
{
  if (Value != GetDefaultShell())
  {
    SetShell(Value ? L"" : L"/bin/bash");
  }
}
//---------------------------------------------------------------------------
bool TSessionData::GetDefaultShell() const
{
  return GetShell().IsEmpty();
}
//---------------------------------------------------------------------------
void TSessionData::SetPuttyProtocol(const UnicodeString & Value)
{
  SET_SESSION_PROPERTY(PuttyProtocol);
}
//---------------------------------------------------------------------
void TSessionData::SetPingIntervalDT(TDateTime Value)
{
  unsigned short hour, min, sec, msec;

  Value.DecodeTime(hour, min, sec, msec);
  SetPingInterval(hour*SecsPerHour + min*SecsPerMin + sec);
}
//---------------------------------------------------------------------------
TDateTime TSessionData::GetPingIntervalDT() const
{
  return SecToDateTime(GetPingInterval());
}
//---------------------------------------------------------------------------
void TSessionData::SetPingType(TPingType Value)
{
  SET_SESSION_PROPERTY(PingType);
}
//---------------------------------------------------------------------------
void TSessionData::SetAddressFamily(TAddressFamily Value)
{
  SET_SESSION_PROPERTY(AddressFamily);
}
//---------------------------------------------------------------------------
void TSessionData::SetRekeyData(const UnicodeString & Value)
{
  SET_SESSION_PROPERTY(RekeyData);
}
//---------------------------------------------------------------------------
void TSessionData::SetRekeyTime(uintptr_t Value)
{
  SET_SESSION_PROPERTY(RekeyTime);
}
//---------------------------------------------------------------------
UnicodeString TSessionData::GetDefaultSessionName() const
{
  UnicodeString Result;
  UnicodeString HostName = TrimLeft(GetHostName());
  UnicodeString UserName = GetUserName();
  RemoveProtocolPrefix(HostName);
  // remove path
  {
    intptr_t Pos = 1;
    HostName = CopyToChars(HostName, Pos, L"/", true, nullptr, false);
  }
  if (!HostName.IsEmpty() && !UserName.IsEmpty())
  {
    Result = FORMAT(L"%s@%s", UserName.c_str(), HostName.c_str());
  }
  else if (!HostName.IsEmpty())
  {
    Result = HostName;
  }
  else
  {
    Result = L"session";
  }
  return Result;
}
//---------------------------------------------------------------------
bool TSessionData::HasSessionName() const
{
  return (!GetName().IsEmpty() && (GetName() != DefaultName));
}
//---------------------------------------------------------------------
UnicodeString TSessionData::GetSessionName() const
{
  UnicodeString Result;
  if (HasSessionName())
  {
    Result = GetName();
    if (GetHidden())
    {
      Result = Result.SubString(TNamedObjectList::HiddenPrefix.Length() + 1, Result.Length() - TNamedObjectList::HiddenPrefix.Length());
    }
  }
  else
  {
    Result = GetDefaultSessionName();
  }
  return Result;
}
//---------------------------------------------------------------------
UnicodeString TSessionData::GetSessionUrl() const
{
  UnicodeString Url;
  if (HasSessionName())
  {
    Url = GetName();
  }
  else
  {
    switch (GetFSProtocol())
    {
      case fsSCPonly:
        Url = L"scp://";
        break;

      default:
        assert(false);
        // fallback
      case fsSFTP:
      case fsSFTPonly:
        Url = L"sftp://";
        break;

      case fsFTP:
        if (GetFtps() == ftpsNone)
          Url = L"ftp://";
        else
          Url = L"ftps://";
        break;
      case fsWebDAV:
        if (GetFtps() == ftpsNone)
          Url = L"http://";
        else
          Url = L"https://";
        break;
    }

    if (!GetHostName().IsEmpty() && !GetUserName().IsEmpty())
    {
      Url += FORMAT(L"%s@%s", GetUserName().c_str(), GetHostName().c_str());
    }
    else if (!GetHostName().IsEmpty())
    {
      Url += GetHostName();
    }
    else
    {
      Url = L"";
    }
  }
  return Url;
}
//---------------------------------------------------------------------
void TSessionData::SetTimeDifference(TDateTime Value)
{
  SET_SESSION_PROPERTY(TimeDifference);
}
//---------------------------------------------------------------------
void TSessionData::SetLocalDirectory(const UnicodeString & Value)
{
  SET_SESSION_PROPERTY(LocalDirectory);
}
//---------------------------------------------------------------------
void TSessionData::SetRemoteDirectory(const UnicodeString & Value)
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
//---------------------------------------------------------------------------
void TSessionData::SetDSTMode(TDSTMode Value)
{
  SET_SESSION_PROPERTY(DSTMode);
}
//---------------------------------------------------------------------------
void TSessionData::SetDeleteToRecycleBin(bool Value)
{
  SET_SESSION_PROPERTY(DeleteToRecycleBin);
}
//---------------------------------------------------------------------------
void TSessionData::SetOverwrittenToRecycleBin(bool Value)
{
  SET_SESSION_PROPERTY(OverwrittenToRecycleBin);
}
//---------------------------------------------------------------------------
void TSessionData::SetRecycleBinPath(const UnicodeString & Value)
{
  SET_SESSION_PROPERTY(RecycleBinPath);
}
//---------------------------------------------------------------------------
void TSessionData::SetPostLoginCommands(const UnicodeString & Value)
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
//---------------------------------------------------------------------------
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
void TSessionData::SetProxyMethod(TProxyMethod Value)
{
  SET_SESSION_PROPERTY(ProxyMethod);
}
//---------------------------------------------------------------------
void TSessionData::SetProxyHost(const UnicodeString & Value)
{
  SET_SESSION_PROPERTY(ProxyHost);
}
//---------------------------------------------------------------------
void TSessionData::SetProxyPort(intptr_t Value)
{
  SET_SESSION_PROPERTY(ProxyPort);
}
//---------------------------------------------------------------------
void TSessionData::SetProxyUsername(const UnicodeString & Value)
{
  SET_SESSION_PROPERTY(ProxyUsername);
}
//---------------------------------------------------------------------
void TSessionData::SetProxyPassword(const UnicodeString & AValue)
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
//---------------------------------------------------------------------
UnicodeString TSessionData::GetProxyHost() const
{
  PrepareProxyData();
  if ((GetProxyMethod() == pmSystem) && (nullptr != FIEProxyConfig))
    return FIEProxyConfig->ProxyHost;
  return FProxyHost;
}
//---------------------------------------------------------------------
intptr_t TSessionData::GetProxyPort() const
{
  PrepareProxyData();
  if ((GetProxyMethod() == pmSystem) && (nullptr != FIEProxyConfig))
    return FIEProxyConfig->ProxyPort;
  return FProxyPort;
}
//---------------------------------------------------------------------
UnicodeString TSessionData::GetProxyUsername() const
{
  return FProxyUsername;
}
//---------------------------------------------------------------------
UnicodeString TSessionData::GetProxyPassword() const
{
  return DecryptPassword(FProxyPassword, GetProxyUsername() + GetProxyHost());
}
//---------------------------------------------------------------------
static void FreeIEProxyConfig(WINHTTP_CURRENT_USER_IE_PROXY_CONFIG * IEProxyConfig)
{
  assert(IEProxyConfig);
  if (IEProxyConfig->lpszAutoConfigUrl)
    GlobalFree(IEProxyConfig->lpszAutoConfigUrl);
  if (IEProxyConfig->lpszProxy)
    GlobalFree(IEProxyConfig->lpszProxy);
  if (IEProxyConfig->lpszProxyBypass)
    GlobalFree(IEProxyConfig->lpszProxyBypass);
}
//---------------------------------------------------------------------
void TSessionData::PrepareProxyData() const
{
  if ((GetProxyMethod() == pmSystem) && (nullptr == FIEProxyConfig))
  {
    FIEProxyConfig = new TIEProxyConfig;
    WINHTTP_CURRENT_USER_IE_PROXY_CONFIG IEProxyConfig;
    if (!WinHttpGetIEProxyConfigForCurrentUser(&IEProxyConfig))
    {
      DWORD Err = GetLastError();
      DEBUG_PRINTF(L"Error reading system proxy configuration, code: %x", Err);
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
//---------------------------------------------------------------------
void TSessionData::ParseIEProxyConfig() const
{
  assert(FIEProxyConfig);
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
//---------------------------------------------------------------------
void TSessionData::FromURI(const UnicodeString & ProxyURI,
  UnicodeString & ProxyUrl, intptr_t & ProxyPort, TProxyMethod & ProxyMethod) const
{
  ProxyUrl.Clear();
  ProxyPort = 0;
  ProxyMethod = pmNone;
  intptr_t Pos = ProxyURI.RPos(L':');
  if (Pos > 0)
  {
    ProxyUrl = ProxyURI.SubString(1, Pos - 1).Trim();
    ProxyPort = ProxyURI.SubString(Pos + 1, -1).Trim().ToInt();
  }
  // remove scheme from Url e.g. "socks5://" "https://"
  Pos = ProxyUrl.Pos(L"://");
  if (Pos > 0)
  {
    UnicodeString ProxyScheme = ProxyUrl.SubString(1, Pos - 1);
    ProxyUrl = ProxyUrl.SubString(Pos + 3, -1);
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
      ProxyMethod = pmHTTP; // TODO: pmHTTPS
    }
  }
  if (ProxyMethod == pmNone)
    ProxyMethod = pmHTTP; // default Value
}
//---------------------------------------------------------------------
void TSessionData::SetProxyTelnetCommand(const UnicodeString & Value)
{
  SET_SESSION_PROPERTY(ProxyTelnetCommand);
}
//---------------------------------------------------------------------
void TSessionData::SetProxyLocalCommand(const UnicodeString & Value)
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
  assert(Bug >= 0 && static_cast<unsigned int>(Bug) < LENOF(FBugs));
  SET_SESSION_PROPERTY(Bugs[Bug]);
}
//---------------------------------------------------------------------
TAutoSwitch TSessionData::GetBug(TSshBug Bug) const
{
  assert(Bug >= 0 && static_cast<unsigned int>(Bug) < LENOF(FBugs));
  return FBugs[Bug];
}
//---------------------------------------------------------------------
void TSessionData::SetCustomParam1(const UnicodeString & Value)
{
  SET_SESSION_PROPERTY(CustomParam1);
}
//---------------------------------------------------------------------
void TSessionData::SetCustomParam2(const UnicodeString & Value)
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
void TSessionData::SetSFTPMinPacketSize(intptr_t Value)
{
  SET_SESSION_PROPERTY(SFTPMinPacketSize);
}
//---------------------------------------------------------------------
void TSessionData::SetSFTPMaxPacketSize(intptr_t Value)
{
  SET_SESSION_PROPERTY(SFTPMaxPacketSize);
}
//---------------------------------------------------------------------
void TSessionData::SetSFTPBug(TSftpBug Bug, TAutoSwitch Value)
{
  assert(Bug >= 0 && static_cast<unsigned int>(Bug) < LENOF(FSFTPBugs));
  SET_SESSION_PROPERTY(SFTPBugs[Bug]);
}
//---------------------------------------------------------------------
TAutoSwitch TSessionData::GetSFTPBug(TSftpBug Bug) const
{
  assert(Bug >= 0 && static_cast<unsigned int>(Bug) < LENOF(FSFTPBugs));
  return FSFTPBugs[Bug];
}
//---------------------------------------------------------------------
void TSessionData::SetSCPLsFullTime(TAutoSwitch Value)
{
  SET_SESSION_PROPERTY(SCPLsFullTime);
}
//---------------------------------------------------------------------------
void TSessionData::SetColor(intptr_t Value)
{
  SET_SESSION_PROPERTY(Color);
}
//---------------------------------------------------------------------------
void TSessionData::SetTunnel(bool Value)
{
  SET_SESSION_PROPERTY(Tunnel);
}
//---------------------------------------------------------------------
void TSessionData::SetTunnelHostName(const UnicodeString & Value)
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
void TSessionData::SetTunnelUserName(const UnicodeString & Value)
{
  // TunnelUserName is key for password encryption
  UnicodeString XTunnelPassword = GetTunnelPassword();
  SET_SESSION_PROPERTY(TunnelUserName);
  SetTunnelPassword(XTunnelPassword);
  Shred(XTunnelPassword);
}
//---------------------------------------------------------------------
void TSessionData::SetTunnelPassword(const UnicodeString & AValue)
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
void TSessionData::SetTunnelPublicKeyFile(const UnicodeString & Value)
{
  if (FTunnelPublicKeyFile != Value)
  {
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
bool TSessionData::GetTunnelAutoassignLocalPortNumber()
{
  return (FTunnelLocalPortNumber <= 0);
}
//---------------------------------------------------------------------
void TSessionData::SetTunnelPortFwd(const UnicodeString & Value)
{
  SET_SESSION_PROPERTY(TunnelPortFwd);
}
//---------------------------------------------------------------------
void TSessionData::SetTunnelHostKey(const UnicodeString & Value)
{
  SET_SESSION_PROPERTY(TunnelHostKey);
}
//---------------------------------------------------------------------
void TSessionData::SetFtpPasvMode(bool Value)
{
  SET_SESSION_PROPERTY(FtpPasvMode);
}
//---------------------------------------------------------------------
void TSessionData::SetFtpAllowEmptyPassword(bool Value)
{
  SET_SESSION_PROPERTY(FtpAllowEmptyPassword);
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
void TSessionData::SetFtpAccount(const UnicodeString & Value)
{
  SET_SESSION_PROPERTY(FtpAccount);
}
//---------------------------------------------------------------------
void TSessionData::SetFtpPingInterval(intptr_t Value)
{
  SET_SESSION_PROPERTY(FtpPingInterval);
}
//---------------------------------------------------------------------------
TDateTime TSessionData::GetFtpPingIntervalDT() const
{
  return SecToDateTime(GetFtpPingInterval());
}
//---------------------------------------------------------------------------
void TSessionData::SetFtpPingType(TPingType Value)
{
  SET_SESSION_PROPERTY(FtpPingType);
}
//---------------------------------------------------------------------------
void TSessionData::SetFtps(TFtps Value)
{
  SET_SESSION_PROPERTY(Ftps);
}
//---------------------------------------------------------------------------
void TSessionData::SetMinTlsVersion(TTlsVersion Value)
{
  SET_SESSION_PROPERTY(MinTlsVersion);
}
//---------------------------------------------------------------------------
void TSessionData::SetMaxTlsVersion(TTlsVersion Value)
{
  SET_SESSION_PROPERTY(MaxTlsVersion);
}
//---------------------------------------------------------------------
void TSessionData::SetFtpListAll(TAutoSwitch Value)
{
  SET_SESSION_PROPERTY(FtpListAll);
}
//---------------------------------------------------------------------
void TSessionData::SetFtpDupFF(bool Value)
{
  SET_SESSION_PROPERTY(FtpDupFF);
}
//---------------------------------------------------------------------
void TSessionData::SetFtpUndupFF(bool Value)
{
  SET_SESSION_PROPERTY(FtpUndupFF);
}
//---------------------------------------------------------------------
void TSessionData::SetSslSessionReuse(bool Value)
{
  SET_SESSION_PROPERTY(SslSessionReuse);
}
//---------------------------------------------------------------------
void TSessionData::SetNotUtf(TAutoSwitch Value)
{
  SET_SESSION_PROPERTY(NotUtf);
}
//---------------------------------------------------------------------
/*void TSessionData::SetIsWorkspace(bool Value)
{
  SET_SESSION_PROPERTY(IsWorkspace);
}
//---------------------------------------------------------------------
void TSessionData::SetLink(UnicodeString Value)
{
  SET_SESSION_PROPERTY(Link);
}*/
//---------------------------------------------------------------------
void TSessionData::SetHostKey(const UnicodeString & Value)
{
  SET_SESSION_PROPERTY(HostKey);
}
//---------------------------------------------------------------------
UnicodeString TSessionData::GetInfoTip() const
{
  if (GetUsesSsh())
  {
    return FMTLOAD(SESSION_INFO_TIP2,
        GetHostName().c_str(), GetUserName().c_str(),
         (GetPublicKeyFile().IsEmpty() ? LoadStr(NO_STR).c_str() : LoadStr(YES_STR).c_str()),
         GetFSProtocolStr().c_str());
  }
  else
  {
    return FMTLOAD(SESSION_INFO_TIP_NO_SSH,
      GetHostName().c_str(), GetUserName().c_str(), GetFSProtocolStr().c_str());
  }
}
//---------------------------------------------------------------------
UnicodeString TSessionData::ExtractLocalName(const UnicodeString & Name)
{
  UnicodeString Result = Name;
  int P = Result.LastDelimiter(L"/");
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
UnicodeString TSessionData::ExtractFolderName(const UnicodeString & Name)
{
  UnicodeString Result;
  int P = Name.LastDelimiter(L"/");
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
  if (HasSessionName()) // || GetIsWorkspace())
  {
    Result = ExtractFolderName(GetName());
  }
  return Result;
}
//---------------------------------------------------------------------
UnicodeString TSessionData::ComposePath(
  const UnicodeString & Path, const UnicodeString & Name)
{
  return UnixIncludeTrailingBackslash(Path) + Name;
}
//---------------------------------------------------------------------
TLoginType TSessionData::GetLoginType() const
{
  return (GetUserName() == AnonymousUserName) && GetPassword().IsEmpty() ?
    ltAnonymous : ltNormal;
}
//---------------------------------------------------------------------
void TSessionData::SetLoginType(TLoginType Value)
{
  SET_SESSION_PROPERTY(LoginType);
  if (GetLoginType() == ltAnonymous)
  {
    SetPassword(L"");
    SetUserName(AnonymousUserName);
  }
}
//---------------------------------------------------------------------
uintptr_t TSessionData::GetCodePageAsNumber() const
{
  if (FCodePageAsNumber == 0)
    FCodePageAsNumber = ::GetCodePageAsNumber(GetCodePage());
  return FCodePageAsNumber;
}

//---------------------------------------------------------------------------
void TSessionData::SetCodePage(const UnicodeString & Value)
{
  SET_SESSION_PROPERTY(CodePage);
  FCodePageAsNumber = 0;
}
//---------------------------------------------------------------------
void TSessionData::AdjustHostName(UnicodeString & HostName, const UnicodeString & Prefix) const
{
  if (::LowerCase(HostName.SubString(1, Prefix.Length())) == Prefix)
  {
    HostName.Delete(1, Prefix.Length());
  }
}
//---------------------------------------------------------------------
void TSessionData::RemoveProtocolPrefix(UnicodeString & HostName) const
{
  AdjustHostName(HostName, L"scp://");
  AdjustHostName(HostName, L"sftp://");
  AdjustHostName(HostName, L"ftp://");
  AdjustHostName(HostName, L"ftps://");
  AdjustHostName(HostName, L"http://");
  AdjustHostName(HostName, L"https://");
}
//---------------------------------------------------------------------
TFSProtocol TSessionData::TranslateFSProtocolNumber(intptr_t FSProtocol)
{
  TFSProtocol Result = static_cast<TFSProtocol>(-1);
  if (GetSessionVersion() >= GetVersionNumber2110())
  {
    Result = static_cast<TFSProtocol>(FSProtocol);
  }
  else
  {
    if (FSProtocol < fsFTPS_219)
    {
      Result = static_cast<TFSProtocol>(FSProtocol);
    }
    switch (FSProtocol)
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
    }
  }
  // assert(Result != -1);
  return Result;
}
//---------------------------------------------------------------------
TFSProtocol TSessionData::TranslateFSProtocol(const UnicodeString & ProtocolID)
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
  if (Result == -1)
    Result = CONST_DEFAULT_PROTOCOL;
  assert(Result != -1);
  return Result;
}
//---------------------------------------------------------------------
TFtps TSessionData::TranslateFtpEncryptionNumber(intptr_t FtpEncryption)
{
  TFtps Result = GetFtps();
  if ((GetSessionVersion() < GetVersionNumber2110()) &&
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
  assert(Result != -1);
  return Result;
}
//---------------------------------------------------------------------
//=== TStoredSessionList ----------------------------------------------
TStoredSessionList::TStoredSessionList(bool AReadOnly) :
  TNamedObjectList(),
  FDefaultSettings(new TSessionData(DefaultName)),
  FReadOnly(AReadOnly)
{
  SetOwnsObjects(true);
}
//---------------------------------------------------------------------
TStoredSessionList::~TStoredSessionList()
{
  delete FDefaultSettings;
}
//---------------------------------------------------------------------
void TStoredSessionList::Load(THierarchicalStorage * Storage,
  bool AsModified, bool UseDefaults)
{
  std::auto_ptr<TStringList> SubKeys(new TStringList());
  std::auto_ptr<TList> Loaded(new TList());
  Storage->GetSubKeyNames(SubKeys.get());
  for (intptr_t Index = 0; Index < SubKeys->GetCount(); ++Index)
  {
    UnicodeString SessionName = SubKeys->GetString(Index);
    bool ValidName = true;
    try
    {
      TSessionData::ValidatePath(SessionName);
    }
    catch(...)
    {
      ValidName = false;
    }
    if (ValidName)
    {
      TSessionData * SessionData = nullptr;
      if (SessionName == FDefaultSettings->GetName())
      {
        SessionData = FDefaultSettings;
      }
      else
      {
        SessionData = static_cast<TSessionData *>(FindByName(SessionName));
      }

      if ((SessionData != FDefaultSettings) || !UseDefaults)
      {
        if (!SessionData)
        {
          SessionData = new TSessionData(L"");
          if (UseDefaults)
          {
            SessionData->Assign(GetDefaultSettings());
          }
          SessionData->SetName(SessionName);
          Add(SessionData);
        }
        Loaded->Add(SessionData);
        SessionData->Load(Storage);
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
      if (Loaded->IndexOf(GetItem(Index)) < 0)
      {
        Delete(Index);
        Index--;
      }
    }
  }
}
//---------------------------------------------------------------------
void TStoredSessionList::Load()
{
  std::auto_ptr<THierarchicalStorage> Storage(GetConfiguration()->CreateStorage(true));
  if (Storage->OpenSubKey(GetConfiguration()->GetStoredSessionsSubKey(), False))
  {
    Load(Storage.get());
  }
}
//---------------------------------------------------------------------
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
//---------------------------------------------------------------------
void TStoredSessionList::DoSave(THierarchicalStorage * Storage,
  bool All, bool RecryptPasswordOnly, TStrings * RecryptPasswordErrors)
{
  std::auto_ptr<TSessionData> FactoryDefaults(new TSessionData(L""));
  DoSave(Storage, FDefaultSettings, All, RecryptPasswordOnly, FactoryDefaults.get());
  for (intptr_t Index = 0; Index < GetCount() + GetHiddenCount(); ++Index)
  {
    TSessionData * SessionData = static_cast<TSessionData *>(GetItem(Index));
    try
    {
      DoSave(Storage, SessionData, All, RecryptPasswordOnly, FactoryDefaults.get());
    }
    catch (Exception & E)
    {
      UnicodeString Message;
      if (RecryptPasswordOnly && ALWAYS_TRUE(RecryptPasswordErrors != NULL) &&
          ExceptionMessage(&E, Message))
      {
        RecryptPasswordErrors->Add(FORMAT("%s: %s", SessionData->GetSessionName().c_str(), Message.c_str()));
      }
      else
      {
        throw;
      }
    }
  }
}
//---------------------------------------------------------------------
void TStoredSessionList::Save(THierarchicalStorage * Storage, bool All)
{
  DoSave(Storage, All, false, NULL);
}
//---------------------------------------------------------------------
void TStoredSessionList::DoSave(bool All, bool Explicit,
  bool RecryptPasswordOnly, TStrings * RecryptPasswordErrors)
{
  std::auto_ptr<THierarchicalStorage> Storage(GetConfiguration()->CreateStorage(true));
  Storage->SetAccessMode(smReadWrite);
  Storage->SetExplicit(Explicit);
  if (Storage->OpenSubKey(GetConfiguration()->GetStoredSessionsSubKey(), true))
  {
    DoSave(Storage.get(), All, RecryptPasswordOnly, RecryptPasswordErrors);
  }

  Saved();
}
//---------------------------------------------------------------------
void TStoredSessionList::Save(bool All, bool Explicit)
{
  DoSave(All, Explicit, false, nullptr);
}
//---------------------------------------------------------------------
void TStoredSessionList::RecryptPasswords(TStrings * RecryptPasswordErrors)
{
  DoSave(true, true, true, RecryptPasswordErrors);
}
//---------------------------------------------------------------------
void TStoredSessionList::Saved()
{
  FDefaultSettings->SetModified(false);
  for (intptr_t Index = 0; Index < GetCount() + GetHiddenCount(); ++Index)
  {
    (static_cast<TSessionData *>(GetItem(Index))->SetModified(false));
  }
}
//---------------------------------------------------------------------
/*void TStoredSessionList::ImportLevelFromFilezilla(_di_IXMLNode Node, const UnicodeString & Path)
{
  for (int Index = 0; Index < Node->ChildNodes->Count; Index++)
  {
    _di_IXMLNode ChildNode = Node->ChildNodes->Get(Index);
    if (ChildNode->NodeName == L"Server")
    {
      std::auto_ptr<TSessionData> SessionData(new TSessionData(L""));
      SessionData->Assign(DefaultSettings);
      SessionData->ImportFromFilezilla(ChildNode, Path);
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

      ImportLevelFromFilezilla(ChildNode, UnixIncludeTrailingBackslash(Path) + Name.Trim());
    }
  }
}*/
//---------------------------------------------------------------------
void TStoredSessionList::ImportFromFilezilla(const UnicodeString & /*FileName*/)
{
  Classes::Error(SNotImplemented, 3004);
/*
  const _di_IXMLDocument Document = interface_cast<Xmlintf::IXMLDocument>(new TXMLDocument(nullptr));
  Document->LoadFromFile(FileName);
  _di_IXMLNode FileZilla3Node = Document->ChildNodes->FindNode(L"FileZilla3");
  if (FileZilla3Node != nullptr)
  {
    _di_IXMLNode ServersNode = FileZilla3Node->ChildNodes->FindNode(L"Servers");
    if (ServersNode != nullptr)
    {
      ImportLevelFromFilezilla(ServersNode, L"");
    }
  }
*/
}
//---------------------------------------------------------------------
void TStoredSessionList::Export(const UnicodeString & FileName)
{
  Classes::Error(SNotImplemented, 3003);
/*
  std::auto_ptr<THierarchicalStorage> Storage(new TIniFileStorage(FileName));
  Storage->SetAccessMode(smReadWrite);
  if (Storage->OpenSubKey(GetConfiguration()->GetStoredSessionsSubKey(), true))
  {
    Save(Storage, true);
  }
*/
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
void TStoredSessionList::Import(TStoredSessionList * From,
  bool OnlySelected)
{
  for (intptr_t Index = 0; Index < From->GetCount(); ++Index)
  {
    if (!OnlySelected || From->GetSession(Index)->GetSelected())
    {
      TSessionData *Session = new TSessionData(L"");
      Session->Assign(From->GetSession(Index));
      Session->SetModified(true);
      Session->MakeUniqueIn(this);
      Add(Session);
    }
  }
  // only modified, explicit
  Save(false, true);
}
//---------------------------------------------------------------------
void TStoredSessionList::SelectSessionsToImport(TStoredSessionList * Dest, bool SSHOnly)
{
  for (intptr_t Index = 0; Index < GetCount(); ++Index)
  {
    GetSession(Index)->SetSelected(
      (!SSHOnly || (GetSession(Index)->GetPuttyProtocol() == PuttySshProtocol)) &&
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
    std::auto_ptr<TRegistryStorage> Storage(new TRegistryStorage(GetConfiguration()->GetRegistryStorageKey()));
    Storage->SetAccessMode(smReadWrite);
    if (Storage->OpenRootKey(False))
    {
      Storage->RecursiveDeleteSubKey(GetConfiguration()->GetStoredSessionsSubKey());
    }
  }
  catch (Exception &E)
  {
    throw ExtException(&E, LoadStr(CLEANUP_SESSIONS_ERROR));
  }
}
//---------------------------------------------------------------------------
void TStoredSessionList::UpdateStaticUsage()
{
/*
  int SCP = 0;
  int SFTP = 0;
  int FTP = 0;
  int FTPS = 0;
  int WebDAV = 0;
  int WebDAVS = 0;
  int Password = 0;
  int Advanced = 0;
  int Color = 0;
  bool Folders = false;
  bool Workspaces = false;
  std::auto_ptr<TSessionData> FactoryDefaults(new TSessionData(L""));
  for (intptr_t Index = 0; Index < Count; ++Index)
  {
    TSessionData * Data = GetSession(Index);
    switch (Data->GetFSProtocol())
    {
      case fsSCPonly:
        SCP++;
        break;

      case fsSFTP:
      case fsSFTPonly:
        SFTP++;
        break;

      case fsFTP:
        if (Data->GetFtps() == ftpsNone)
        {
          FTP++;
        }
        else
        {
          FTPS++;
        }
        break;

      case fsWebDAV:
        if (Data->GetFtps() == ftpsNone)
        {
          WebDAV++;
        }
        else
        {
          WebDAVS++;
        }
        break;
    }

    if (Data->HasAnyPassword())
    {
      Password++;
    }

    if (Data->GetColor() != 0)
    {
      Color++;
    }

    if (!Data->IsSame(FactoryDefaults.get(), true))
    {
      Advanced++;
    }

    if (Data->GetIsWorkspace())
    {
      Workspaces = true;
    }
    else if (!Data->GetFolderName().IsEmpty())
    {
      Folders = true;
    }
  }

  GetConfiguration()->GetUsage()->Set(L"StoredSessionsCountSCP", SCP);
  GetConfiguration()->GetUsage()->Set(L"StoredSessionsCountSFTP", SFTP);
  GetConfiguration()->GetUsage()->Set(L"StoredSessionsCountFTP", FTP);
  GetConfiguration()->GetUsage()->Set(L"StoredSessionsCountFTPS", FTPS);
  GetConfiguration()->GetUsage()->Set(L"StoredSessionsCountWebDAV", WebDAV);
  GetConfiguration()->GetUsage()->Set(L"StoredSessionsCountWebDAVS", WebDAVS);
  Configuration->Usage->Set(L"StoredSessionsCountPassword", Password);
  Configuration->Usage->Set(L"StoredSessionsCountColor", Color);
  GetConfiguration()->GetUsage()->Set(L"StoredSessionsCountAdvanced", Advanced);

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
*/
}
//---------------------------------------------------------------------------
TSessionData * TStoredSessionList::FindSame(TSessionData * Data)
{
  TSessionData * Result;
  if (Data->GetHidden() || Data->GetName().IsEmpty()) // || Data->GetIsWorkspace())
  {
    Result = nullptr;
  }
  else
  {
    Result = dynamic_cast<TSessionData *>(FindByName(Data->GetName()));
  }
  return Result;
}
//---------------------------------------------------------------------------
intptr_t TStoredSessionList::IndexOf(TSessionData * Data)
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
TSessionData * TStoredSessionList::NewSession(
  const UnicodeString & SessionName, TSessionData * Session)
{
  TSessionData * DuplicateSession = static_cast<TSessionData*>(FindByName(SessionName));
  if (!DuplicateSession)
  {
    DuplicateSession = new TSessionData(L"");
    DuplicateSession->Assign(Session);
    DuplicateSession->SetName(SessionName);
    // make sure, that new stored session is saved to registry
    DuplicateSession->SetModified(true);
    Add(DuplicateSession);
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
void TStoredSessionList::SetDefaultSettings(const TSessionData * Value)
{
  assert(FDefaultSettings);
  if (FDefaultSettings != Value)
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
//---------------------------------------------------------------------------
void TStoredSessionList::ImportHostKeys(const UnicodeString & TargetKey,
  const UnicodeString & SourceKey, TStoredSessionList * Sessions,
  bool OnlySelected)
{
  std::auto_ptr<TRegistryStorage> SourceStorage(new TRegistryStorage(SourceKey));
  std::auto_ptr<TRegistryStorage> TargetStorage(new TRegistryStorage(TargetKey));
  std::auto_ptr<TStringList> KeyList(new TStringList());
  TargetStorage->SetAccessMode(smReadWrite);

  if (SourceStorage->OpenRootKey(false) &&
      TargetStorage->OpenRootKey(true))
  {
    SourceStorage->GetValueNames(KeyList.get());

    UnicodeString HostKeyName;
    assert(Sessions != nullptr);
    for (intptr_t Index = 0; Index < Sessions->GetCount(); ++Index)
    {
      TSessionData * Session = Sessions->GetSession(Index);
      if (!OnlySelected || Session->GetSelected())
      {
        HostKeyName = PuttyMungeStr(FORMAT(L"@%d:%s", Session->GetPortNumber(), Session->GetHostName().c_str()));
        UnicodeString KeyName;
        for (intptr_t KeyIndex = 0; KeyIndex < KeyList->GetCount(); ++KeyIndex)
        {
          KeyName = KeyList->GetString(KeyIndex);
          intptr_t P = KeyName.Pos(HostKeyName);
          if ((P > 0) && (P == KeyName.Length() - HostKeyName.Length() + 1))
          {
            TargetStorage->WriteStringRaw(KeyName,
              SourceStorage->ReadStringRaw(KeyName, L""));
          }
        }
      }
    }
  }
}
//---------------------------------------------------------------------------
const TSessionData * TStoredSessionList::GetSessionByName(const UnicodeString & SessionName) const
{
  for (intptr_t I = 0; I < GetCount(); ++I)
  {
    const TSessionData * SessionData = GetSession(I);
    if (SessionData->GetName() == SessionName)
    {
      return SessionData;
    }
  }
  return nullptr;
}

//---------------------------------------------------------------------
void TStoredSessionList::Load(const UnicodeString & AKey, bool UseDefaults)
{
  std::auto_ptr<TRegistryStorage> Storage(new TRegistryStorage(AKey));
  if (Storage->OpenRootKey(false))
  {
    Load(Storage.get(), false, UseDefaults);
  }
}
//---------------------------------------------------------------------------
bool TStoredSessionList::IsFolderOrWorkspace(
  const UnicodeString & Name, bool Workspace)
{
  bool Result = false;
  TSessionData * FirstData = nullptr;
  if (!Name.IsEmpty())
  {
    for (intptr_t Index = 0; !Result && (Index < GetCount()); Index++)
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
    ALWAYS_TRUE(FirstData != nullptr) &&
    (FirstData->IsWorkspace() == Workspace);
}
//---------------------------------------------------------------------------
bool TStoredSessionList::IsFolder(const UnicodeString & Name)
{
  return IsFolderOrWorkspace(Name, false);
}
//---------------------------------------------------------------------------
bool TStoredSessionList::IsWorkspace(const UnicodeString & Name)
{
  return IsFolderOrWorkspace(Name, true);
}
//---------------------------------------------------------------------------
TSessionData * TStoredSessionList::CheckIsInFolderOrWorkspaceAndResolve(
  TSessionData * Data, const UnicodeString & Name)
{
  if (Data->IsInFolderOrWorkspace(Name))
  {
    Data = ResolveSessionData(Data);

    if ((Data != nullptr) && Data->GetCanLogin()) // &&
        // ALWAYS_TRUE(Data->Link.IsEmpty()))
    {
      return Data;
    }
  }
  return nullptr;
}
//---------------------------------------------------------------------------
void TStoredSessionList::GetFolderOrWorkspace(const UnicodeString & Name, TList * List)
{
  for (intptr_t Index = 0; (Index < GetCount()); Index++)
  {
    TSessionData * Data =
      CheckIsInFolderOrWorkspaceAndResolve(GetSession(Index), Name);

    if (Data != nullptr)
    {
      TSessionData * Data2 = new TSessionData(L"");
      Data2->Assign(Data);
      List->Add(Data2);
    }
  }
}
//---------------------------------------------------------------------------
TStrings * TStoredSessionList::GetFolderOrWorkspaceList(
  const UnicodeString & Name)
{
  std::auto_ptr<TStringList> Result(new TStringList());

  for (intptr_t Index = 0; Index < GetCount(); Index++)
  {
    TSessionData * Data =
      CheckIsInFolderOrWorkspaceAndResolve(GetSession(Index), Name);

    if (Data != nullptr)
    {
      Result->Add(Data->GetSessionName());
    }
  }

  return Result.release();
}
//---------------------------------------------------------------------------
TStrings * TStoredSessionList::GetWorkspaces()
{
  std::auto_ptr<TStringList> Result(new TStringList());
  Result->SetSorted(true);
  Result->SetDuplicates(dupIgnore);
  Result->SetCaseSensitive(false);

  for (intptr_t Index = 0; Index < GetCount(); Index++)
  {
    TSessionData * Data = GetSession(Index);
    if (Data->IsWorkspace())
    {
      Result->Add(Data->GetFolderName());
    }
  }

  return Result.release();
}
//---------------------------------------------------------------------------
void TStoredSessionList::NewWorkspace(
  const UnicodeString & Name, TList * DataList)
{
  for (intptr_t Index = 0; Index < GetCount(); Index++)
  {
    TSessionData * Data = GetSession(Index);
    if (Data->IsInFolderOrWorkspace(Name))
    {
      Data->Remove();
      Remove(Data);
      Index--;
    }
  }

  for (intptr_t Index = 0; Index < DataList->GetCount(); Index++)
  {
    TSessionData * Data = static_cast<TSessionData *>(DataList->GetItem(Index));

    TSessionData * Data2 = new TSessionData(L"");
    Data2->Assign(Data);
    Data2->SetName(TSessionData::ComposePath(Name, Data->GetName()));
    // make sure, that new stored session is saved to registry
    Data2->SetModified(true);
    Add(Data2);
  }
}
//---------------------------------------------------------------------------
bool TStoredSessionList::HasAnyWorkspace()
{
  bool Result = false;
  for (intptr_t Index = 0; !Result && (Index < GetCount()); Index++)
  {
    TSessionData * Data = GetSession(Index);
    Result = Data->IsWorkspace();
  }
  return Result;
}
//---------------------------------------------------------------------------
TSessionData * TStoredSessionList::ParseUrl(const UnicodeString & Url,
  TOptions * Options, bool & DefaultsOnly, UnicodeString * FileName,
  bool * AProtocolDefined, UnicodeString * MaskedUrl)
{
  std::auto_ptr<TSessionData> Data(new TSessionData(L""));
    Data->ParseUrl(Url, Options, this, DefaultsOnly, FileName, AProtocolDefined, MaskedUrl);

  return Data.release();
}
//---------------------------------------------------------------------
TSessionData * TStoredSessionList::ResolveSessionData(TSessionData * Data)
{
  /*if (!Data->GetLink().IsEmpty())
  {
    Data = dynamic_cast<TSessionData *>(FindByName(Data->GetLink()));
    if (Data != nullptr)
    {
      Data = ResolveSessionData(Data);
    }
  }*/
  return Data;
}
//---------------------------------------------------------------------
bool TStoredSessionList::CanLogin(TSessionData * Data)
{
  Data = ResolveSessionData(Data);
  return (Data != nullptr) && Data->GetCanLogin();
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
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
//---------------------------------------------------------------------
uintptr_t GetCodePageAsNumber(const UnicodeString & CodePage)
{
  uintptr_t codePage = _wtoi(CodePage.c_str());
  return static_cast<uintptr_t >(codePage == 0 ? CONST_DEFAULT_CODEPAGE : codePage);
}
//---------------------------------------------------------------------
UnicodeString GetCodePageAsString(uintptr_t CodePage)
{
  CPINFOEX cpInfoEx;
  if (::GetCodePageInfo(static_cast<UINT>(CodePage), cpInfoEx))
  {
    return UnicodeString(cpInfoEx.CodePageName);
  }
  return IntToStr(CONST_DEFAULT_CODEPAGE);
}

//---------------------------------------------------------------------
UnicodeString GetExpandedLogFileName(const UnicodeString & LogFileName, TSessionData * SessionData)
{
  UnicodeString ANewFileName = StripPathQuotes(ExpandEnvironmentVariables(LogFileName));
  TDateTime N = Now();
  for (intptr_t Index = 1; Index < ANewFileName.Length(); ++Index)
  {
    if (ANewFileName[Index] == L'&')
    {
      UnicodeString Replacement;
      // keep consistent with TFileCustomCommand::PatternReplacement
      unsigned short Y, M, D, H, NN, S, MS;
      TDateTime DateTime = N;
      DateTime.DecodeDate(Y, M, D);
      DateTime.DecodeTime(H, NN, S, MS);
      switch (tolower(ANewFileName[Index + 1]))
      {
        case L'y':
          // Replacement = FormatDateTime(L"yyyy", N);
          Replacement = FORMAT(L"%04d", Y);
          break;

        case L'm':
          // Replacement = FormatDateTime(L"mm", N);
          Replacement = FORMAT(L"%02d", M);
          break;

        case L'd':
          // Replacement = FormatDateTime(L"dd", N);
          Replacement = FORMAT(L"%02d", D);
          break;

        case L't':
          // Replacement = FormatDateTime(L"hhnnss", N);
          Replacement = FORMAT(L"%02d%02d%02d", H, NN, S);
          break;

        case L'@':
          Replacement = MakeValidFileName(SessionData->GetHostNameExpanded());
          break;

        case L's':
          Replacement = MakeValidFileName(SessionData->GetSessionName());
          break;

        case L'&':
          Replacement = L"&";
          break;

        default:
          Replacement = UnicodeString(L"&") + ANewFileName[Index + 1];
          break;
      }
      ANewFileName.Delete(Index, 2);
      ANewFileName.Insert(Replacement, Index);
      Index += Replacement.Length() - 1;
    }
  }
  return ANewFileName;
}
//---------------------------------------------------------------------
bool IsSshProtocol(TFSProtocol FSProtocol)
{
  return
    (FSProtocol == fsSFTPonly) || (FSProtocol == fsSFTP) ||
    (FSProtocol == fsSCPonly);
}
//---------------------------------------------------------------------
