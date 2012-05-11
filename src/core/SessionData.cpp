//---------------------------------------------------------------------------
#ifndef _MSC_VER
#include <vcl.h>
#pragma hdrstop
#endif
#include "stdafx.h"

#include "boostdefines.hpp"
#include <boost/scope_exit.hpp>

#include "SessionData.h"

#include "Common.h"
#include "Exceptions.h"
#include "FileBuffer.h"
#include "CoreMain.h"
#include "TextsCore.h"
#include "PuttyIntf.h"
#include "RemoteFiles.h"
//---------------------------------------------------------------------------
#ifndef _MSC_VER
#pragma package(smart_init)
#endif
//---------------------------------------------------------------------------
enum TProxyType { pxNone, pxHTTP, pxSocks, pxTelnet }; // 0.53b and older
const wchar_t * DefaultName = L"Default Settings";
const wchar_t CipherNames[CIPHER_COUNT][10] = {L"WARN", L"3des", L"blowfish", L"aes", L"des", L"arcfour"};
const wchar_t KexNames[KEX_COUNT][20] = {L"WARN", L"dh-group1-sha1", L"dh-group14-sha1", L"dh-gex-sha1", L"rsa" };
const wchar_t ProtocolNames[PROTOCOL_COUNT][10] = { L"raw", L"telnet", L"rlogin", L"ssh" };
const wchar_t SshProtList[][10] = {L"1 only", L"1", L"2", L"2 only"};
const wchar_t ProxyMethodList[][10] = {L"none", L"SOCKS4", L"SOCKS5", L"HTTP", L"Telnet", L"Cmd" };
const TCipher DefaultCipherList[CIPHER_COUNT] =
  { cipAES, cipBlowfish, cip3DES, cipWarn, cipArcfour, cipDES };
const TKex DefaultKexList[KEX_COUNT] =
  { kexDHGEx, kexDHGroup14, kexDHGroup1, kexRSA, kexWarn };
const wchar_t FSProtocolNames[FSPROTOCOL_COUNT][15] = { L"SCP", L"SFTP (SCP)", L"SFTP", L"", L"", L"FTP", L"FTPS", L"WebDAV - HTTP", L"WebDAV - HTTPS" };
const UnicodeString CONST_LOGIN_ANONYMOUS = L"anonymous";
const int SshPortNumber = 22;
const int FtpPortNumber = 21;
const int HTTPPortNumber = 80;
const int HTTPSPortNumber = 443;
const int FtpsImplicitPortNumber = 990;
const int DefaultSendBuf = 262144;
const UnicodeString AnonymousUserName(L"anonymous");
const UnicodeString AnonymousPassword(L"anonymous@example.com");
const unsigned int CONST_DEFAULT_CODEPAGE = CP_ACP;
//---------------------------------------------------------------------
TDateTime __fastcall SecToDateTime(int Sec)
{
  return TDateTime(static_cast<unsigned short>(Sec/SecsPerHour),
    static_cast<unsigned short>(Sec/SecsPerMin%MinsPerHour), static_cast<unsigned short>(Sec%SecsPerMin), 0);
}
//--- TSessionData ----------------------------------------------------
/* __fastcall */ TSessionData::TSessionData(UnicodeString aName):
  TNamedObject(aName)
{
  Default();
  FModified = true;
}
//---------------------------------------------------------------------
void __fastcall TSessionData::Default()
{
  SetHostName(L"");
  SetPortNumber(SshPortNumber);
  SetUserName(CONST_LOGIN_ANONYMOUS);
  SetPassword(L"");
  SetLoginType(ltAnonymous);
  SetPasswordless(false);
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
  for (int Index = 0; Index < CIPHER_COUNT; Index++)
  {
    SetCipher(Index, DefaultCipherList[Index]);
  }
  for (int Index = 0; Index < KEX_COUNT; Index++)
  {
    SetKex(Index, DefaultKexList[Index]);
  }
  SetPublicKeyFile(L"");
  FProtocol = ptSSH;
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

  for (unsigned int Index = 0; Index < LENOF(FBugs); Index++)
  {
    SetBug(static_cast<TSshBug>(Index), asAuto);
  }

  SetSpecial(false);
  SetFSProtocol(fsSFTP);
  SetAddressFamily(afAuto);
  SetCodePage(::GetCodePageAsString(CONST_DEFAULT_CODEPAGE));
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
  SetRecycleBinPath(L"/tmp");
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
  SetNotUtf(asOff);
  SetFtpListAll(asAuto);

  // SFTP
  SetSftpServer(L"");
  SetSFTPDownloadQueue(4);
  SetSFTPUploadQueue(4);
  SetSFTPListingQueue(2);
  SetSFTPMaxVersion(5);
  SetSFTPMinPacketSize(0);
  SetSFTPMaxPacketSize(0);

  for (unsigned int Index = 0; Index < LENOF(FSFTPBugs); Index++)
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

  // FTP
  SetFtpPasvMode(true);
  SetFtpForcePasvIp(false);
  SetFtpAccount(L"");
  SetFtpPingInterval(30);
  SetFtpPingType(ptDummyCommand);
  SetFtps(ftpsNone);

  SetFtpProxyLogonType(0); // none

  SetFtpAllowEmptyPassword(false);
  SetFtpEncryption(fesPlainFTP);

  SetSslSessionReuse(true);

  SetCustomParam1(L"");
  SetCustomParam2(L"");

  FNumberOfRetries = 0;

  FSelected = false;
  FModified = false;
  FSource = ::ssNone;

  // add also to TSessionLog::AddStartupInfo()
}
//---------------------------------------------------------------------
void __fastcall TSessionData::NonPersistant()
{
  SetUpdateDirectories(false);
  SetPreserveDirectoryChanges(false);
}
//---------------------------------------------------------------------
void __fastcall TSessionData::Assign(TPersistent * Source)
{
  if (Source && ::InheritsFrom<TPersistent, TSessionData>(Source))
  {
    #define DUPL(P) Set##P(((TSessionData *)Source)->Get##P())
    DUPL(Name);
    DUPL(HostName);
    DUPL(PortNumber);
    DUPL(LoginType);
    DUPL(UserName);
    DUPL(Password);
    DUPL(Passwordless);
    DUPL(PingInterval);
    DUPL(PingType);
    DUPL(Timeout);
    DUPL(TryAgent);
    DUPL(AgentFwd);
    DUPL(AuthTIS);
    DUPL(ChangeUsername);
    DUPL(Compression);
    DUPL(SshProt);
    DUPL(Ssh2DES);
    DUPL(SshNoUserAuth);
    DUPL(CipherList);
    DUPL(KexList);
    DUPL(PublicKeyFile);
    DUPL(AddressFamily);
    DUPL(CodePage);
    DUPL(RekeyData);
    DUPL(RekeyTime);
    DUPL(HostKey);

    DUPL(FSProtocol);
    DUPL(LocalDirectory);
    DUPL(RemoteDirectory);
    DUPL(SynchronizeBrowsing);
    DUPL(UpdateDirectories);
    DUPL(CacheDirectories);
    DUPL(CacheDirectoryChanges);
    DUPL(PreserveDirectoryChanges);

    DUPL(ResolveSymlinks);
    DUPL(DSTMode);
    DUPL(LockInHome);
    DUPL(Special);
    DUPL(Selected);
    DUPL(ReturnVar);
    DUPL(LookupUserGroups);
    DUPL(EOLType);
    DUPL(Shell);
    DUPL(ClearAliases);
    DUPL(Scp1Compatibility);
    DUPL(UnsetNationalVars);
    DUPL(ListingCommand);
    DUPL(IgnoreLsWarnings);
    DUPL(SCPLsFullTime);
    DUPL(FtpListAll);

    DUPL(TimeDifference);
    // new in 53b
    DUPL(TcpNoDelay);
    DUPL(SendBuf);
    DUPL(SshSimple);
    DUPL(AuthKI);
    DUPL(AuthKIPassword);
    DUPL(AuthGSSAPI);
    DUPL(GSSAPIFwdTGT);
    DUPL(GSSAPIServerRealm);
    DUPL(DeleteToRecycleBin);
    DUPL(OverwrittenToRecycleBin);
    DUPL(RecycleBinPath);
    DUPL(NotUtf);
    DUPL(PostLoginCommands);

    DUPL(ProxyMethod);
    DUPL(ProxyHost);
    DUPL(ProxyPort);
    DUPL(ProxyUsername);
    DUPL(ProxyPassword);
    DUPL(ProxyTelnetCommand);
    DUPL(ProxyLocalCommand);
    DUPL(ProxyDNS);
    DUPL(ProxyLocalhost);

    for (unsigned int Index = 0; Index < LENOF(FBugs); Index++)
    {
      // DUPL(Bug[(TSshBug)Index]);
      (static_cast<TSessionData *>(Source))->SetBug(static_cast<TSshBug>(Index),
          GetBug(static_cast<TSshBug>(Index)));
    }

    // SFTP
    DUPL(SftpServer);
    DUPL(SFTPDownloadQueue);
    DUPL(SFTPUploadQueue);
    DUPL(SFTPListingQueue);
    DUPL(SFTPMaxVersion);
    DUPL(SFTPMinPacketSize);
    DUPL(SFTPMaxPacketSize);

    for (unsigned int Index = 0; Index < LENOF(FSFTPBugs); Index++)
    {
      // DUPL(SFTPBug[(TSftpBug)Index]);
      (static_cast<TSessionData *>(Source))->SetSFTPBug(static_cast<TSftpBug>(Index),
          GetSFTPBug(static_cast<TSftpBug>(Index)));
    }

    DUPL(Color);

    DUPL(Tunnel);
    DUPL(TunnelHostName);
    DUPL(TunnelPortNumber);
    DUPL(TunnelUserName);
    DUPL(TunnelPassword);
    DUPL(TunnelPublicKeyFile);
    DUPL(TunnelLocalPortNumber);
    DUPL(TunnelPortFwd);

    DUPL(FtpPasvMode);
    DUPL(FtpAllowEmptyPassword);
    DUPL(FtpEncryption);
    DUPL(FtpForcePasvIp);
    DUPL(FtpAccount);
    DUPL(FtpPingInterval);
    DUPL(FtpPingType);
    DUPL(Ftps);

    DUPL(FtpProxyLogonType);

    DUPL(CustomParam1);
    DUPL(CustomParam2);
    DUPL(SslSessionReuse);

    #undef DUPL
    FModified = (static_cast<TSessionData *>(Source))->GetModified();
    FSource = (static_cast<TSessionData *>(Source))->FSource;

    FNumberOfRetries = (static_cast<TSessionData *>(Source))->GetNumberOfRetries();
  }
  else
  {
    TNamedObject::Assign(Source);
  }
}
//---------------------------------------------------------------------
void __fastcall TSessionData::DoLoad(THierarchicalStorage * Storage, bool & RewritePassword)
{
  SetPortNumber(Storage->ReadInteger(L"PortNumber", GetPortNumber()));
  SetUserName(Storage->ReadString(L"UserName", GetUserName()));
  // must be loaded after UserName, because HostName may be in format user@host
  SetHostName(Storage->ReadString(L"HostName", GetHostName()));

  if (!Configuration->GetDisablePasswordStoring())
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
  SetLoginType(static_cast<TLoginType>(Storage->ReadInteger(L"LoginType", GetLoginType())));
  SetPasswordless(Storage->ReadBool(L"Passwordless", GetPasswordless()));
  // Putty uses PingIntervalSecs
  int PingIntervalSecs = Storage->ReadInteger(L"PingIntervalSecs", -1);
  if (PingIntervalSecs < 0)
  {
    PingIntervalSecs = Storage->ReadInteger(L"PingIntervalSec", GetPingInterval()%60);
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

  SetCodePage(Storage->ReadString(L"CodePage", GetCodePage()));

  SetFSProtocol(static_cast<TFSProtocol>(Storage->ReadInteger(L"FSProtocol", GetFSProtocol())));
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
  SetFtpListAll(static_cast<TAutoSwitch>(Storage->ReadInteger(L"FtpListAll", GetFtpListAll())));
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
    int ProxyType = Storage->ReadInteger(L"ProxyType", pxNone);
    int ProxySOCKSVersion;
    switch (ProxyType) {
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
        SetProxyMethod(pmNone);
        break;
    }
  }
  SetProxyHost(Storage->ReadString(L"ProxyHost", GetProxyHost()));
  SetProxyPort(Storage->ReadInteger(L"ProxyPort", GetProxyPort()));
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
    SetBug(sb##BUG, TAutoSwitch(2 - Storage->ReadInteger(L"Bug" + MB2W(#BUG), \
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
    SetSFTPBug(sb##BUG, TAutoSwitch(Storage->ReadInteger(L"SFTP" + MB2W(#BUG) + L"Bug", GetSFTPBug(sb##BUG))));
  READ_SFTP_BUG(Symlink);
  READ_SFTP_BUG(SignedTS);
  #undef READ_SFTP_BUG

  SetSFTPMaxVersion(Storage->ReadInteger(L"SFTPMaxVersion", GetSFTPMaxVersion()));
  SetSFTPMinPacketSize(Storage->ReadInteger(L"SFTPMinPacketSize", GetSFTPMinPacketSize()));
  SetSFTPMaxPacketSize(Storage->ReadInteger(L"SFTPMaxPacketSize", GetSFTPMaxPacketSize()));

  SetColor(Storage->ReadInteger(L"Color", GetColor()));

  SetProtocolStr(Storage->ReadString(L"Protocol", GetProtocolStr()));

  SetTunnel(Storage->ReadBool(L"Tunnel", GetTunnel()));
  SetTunnelPortNumber(Storage->ReadInteger(L"TunnelPortNumber", GetTunnelPortNumber()));
  SetTunnelUserName(Storage->ReadString(L"TunnelUserName", GetTunnelUserName()));
  // must be loaded after TunnelUserName,
  // because TunnelHostName may be in format user@host
  SetTunnelHostName(Storage->ReadString(L"TunnelHostName", GetTunnelHostName()));
  if (!Configuration->GetDisablePasswordStoring())
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

  // Ftp prefix
  SetFtpPasvMode(Storage->ReadBool(L"FtpPasvMode", GetFtpPasvMode()));
  SetFtpForcePasvIp(Storage->ReadBool(L"FtpForcePasvIp", GetFtpForcePasvIp()));
  SetFtpAccount(Storage->ReadString(L"FtpAccount", GetFtpAccount()));
  SetFtpPingInterval(Storage->ReadInteger(L"FtpPingInterval", GetFtpPingInterval()));
  SetFtpPingType(static_cast<TPingType>(Storage->ReadInteger(L"FtpPingType", GetFtpPingType())));
  SetFtps(static_cast<TFtps>(Storage->ReadInteger(L"Ftps", GetFtps())));

  SetFtpProxyLogonType(Storage->ReadInteger(L"FtpProxyLogonType", GetFtpProxyLogonType()));

  SetFtpAllowEmptyPassword(Storage->ReadBool(L"FtpAllowEmptyPassword", GetFtpAllowEmptyPassword()));
  SetFtpEncryption(static_cast<TFtpEncryptionSwitch>(Storage->ReadInteger(L"FtpEncryption", GetFtpEncryption())));

  SetCustomParam1(Storage->ReadString(L"CustomParam1", GetCustomParam1()));
  SetCustomParam2(Storage->ReadString(L"CustomParam2", GetCustomParam2()));

  SetSslSessionReuse(Storage->ReadBool(L"SslSessionReuse", GetSslSessionReuse()));
}
//---------------------------------------------------------------------
void __fastcall TSessionData::Load(THierarchicalStorage * Storage)
{
  bool RewritePassword = false;
  if (Storage->OpenSubKey(GetInternalStorageKey(), False))
  {
    DoLoad(Storage, RewritePassword);

    Storage->CloseSubKey();
  };

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
void __fastcall TSessionData::Save(THierarchicalStorage * Storage,
  bool PuttyExport, const TSessionData * Default)
{
  if (Storage->OpenSubKey(GetInternalStorageKey(), true))
  {
    #define WRITE_DATA_EX(TYPE, NAME, PROPERTY, CONV) \
      if ((Default != NULL) && (CONV(Default->PROPERTY) == CONV(PROPERTY))) \
      { \
        Storage->DeleteValue(NAME); \
      } \
      else \
      { \
        Storage->Write ## TYPE(NAME, CONV(PROPERTY)); \
      }
    #define WRITE_DATA_CONV(TYPE, NAME, PROPERTY) WRITE_DATA_EX(TYPE, NAME, PROPERTY, WRITE_DATA_CONV_FUNC)
    #define WRITE_DATA(TYPE, PROPERTY) WRITE_DATA_EX(TYPE, #PROPERTY, PROPERTY, )

    WRITE_DATA_EX(String, L"HostName", GetHostName(), );
    WRITE_DATA_EX(Integer, L"PortNumber", GetPortNumber(), );
    WRITE_DATA_EX(Bool, L"Passwordless", GetPasswordless(), );
    WRITE_DATA_EX(Integer, L"PingInterval", GetPingInterval() / SecsPerMin, );
    WRITE_DATA_EX(Integer, L"PingIntervalSecs", GetPingInterval() % SecsPerMin, );
    Storage->DeleteValue(L"PingIntervalSec"); // obsolete
    // when PingInterval is stored always store PingType not to attempt to
    // deduce PingType from PingInterval (backward compatibility with pre 3.5)
    if (((Default != NULL) && (GetPingType() != Default->GetPingType())) ||
        Storage->ValueExists(L"PingInterval"))
    {
      Storage->WriteInteger(L"PingType", GetPingType());
    }
    else
    {
      Storage->DeleteValue(L"PingType");
    }
    WRITE_DATA_EX(Integer, L"Timeout", GetTimeout(), );
    WRITE_DATA_EX(Bool, L"TryAgent", GetTryAgent(), );
    WRITE_DATA_EX(Bool, L"AgentFwd", GetAgentFwd(), );
    WRITE_DATA_EX(Bool, L"AuthTIS", GetAuthTIS(), );
    WRITE_DATA_EX(Bool, L"AuthKI", GetAuthKI(), );
    WRITE_DATA_EX(Bool, L"AuthKIPassword", GetAuthKIPassword(), );

    WRITE_DATA_EX(Bool, L"AuthGSSAPI", GetAuthGSSAPI(), );
    WRITE_DATA_EX(Bool, L"GSSAPIFwdTGT", GetGSSAPIFwdTGT(), );
    WRITE_DATA_EX(String, L"GSSAPIServerRealm", GetGSSAPIServerRealm(), );
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

    WRITE_DATA_EX(Bool, L"ChangeUsername", GetChangeUsername(), );
    WRITE_DATA_EX(Bool, L"Compression", GetCompression(), );
    WRITE_DATA_EX(Integer, L"SshProt", GetSshProt(), );
    WRITE_DATA_EX(Bool, L"Ssh2DES", GetSsh2DES(), );
    WRITE_DATA_EX(Bool, L"SshNoUserAuth", GetSshNoUserAuth(), );
    WRITE_DATA_EX(String, L"Cipher", GetCipherList(), );
    WRITE_DATA_EX(String, L"KEX", GetKexList(), );
    WRITE_DATA_EX(Integer, L"AddressFamily", GetAddressFamily(), );
    WRITE_DATA_EX(String, L"RekeyBytes", GetRekeyData(), );
    WRITE_DATA_EX(Integer, L"RekeyTime", GetRekeyTime(), );

    WRITE_DATA_EX(Bool, L"TcpNoDelay", GetTcpNoDelay(), );

    WRITE_DATA_EX(String, L"CodePage", GetCodePage(), );

    if (PuttyExport)
    {
      WRITE_DATA_EX(StringRaw, L"UserName", GetUserName(), );
      WRITE_DATA_EX(StringRaw, L"PublicKeyFile", GetPublicKeyFile(), );
    }
    else
    {
      WRITE_DATA_EX(Integer, L"LoginType", GetLoginType(), );
      WRITE_DATA_EX(String, L"UserName", GetUserName(), );
      WRITE_DATA_EX(String, L"PublicKeyFile", GetPublicKeyFile(), );
      WRITE_DATA_EX(Integer, L"FSProtocol", GetFSProtocol(), );
      WRITE_DATA_EX(String, L"LocalDirectory", GetLocalDirectory(), );
      WRITE_DATA_EX(String, L"RemoteDirectory", GetRemoteDirectory(), );
      WRITE_DATA_EX(Bool, L"SynchronizeBrowsing", GetSynchronizeBrowsing(), );
      WRITE_DATA_EX(Bool, L"UpdateDirectories", GetUpdateDirectories(), );
      WRITE_DATA_EX(Bool, L"CacheDirectories", GetCacheDirectories(), );
      WRITE_DATA_EX(Bool, L"CacheDirectoryChanges", GetCacheDirectoryChanges(), );
      WRITE_DATA_EX(Bool, L"PreserveDirectoryChanges", GetPreserveDirectoryChanges(), );

      WRITE_DATA_EX(Bool, L"ResolveSymlinks", GetResolveSymlinks(), );
      WRITE_DATA_EX(Integer, L"ConsiderDST", GetDSTMode(), );
      WRITE_DATA_EX(Bool, L"LockInHome", GetLockInHome(), );
      // Special is never stored (if it would, login dialog must be modified not to
      // duplicate Special parameter when Special session is loaded and then stored
      // under different name)
      // WRITE_DATA_EX(Bool, L"Special", GetSpecial(), );
      WRITE_DATA_EX(String, L"Shell", GetShell(), );
      WRITE_DATA_EX(Bool, L"ClearAliases", GetClearAliases(), );
      WRITE_DATA_EX(Bool, L"UnsetNationalVars", GetUnsetNationalVars(), );
      WRITE_DATA_EX(String, L"ListingCommand", GetListingCommand(), );
      WRITE_DATA_EX(Bool, L"IgnoreLsWarnings", GetIgnoreLsWarnings(), );
      WRITE_DATA_EX(Integer, L"SCPLsFullTime", GetSCPLsFullTime(), );
      WRITE_DATA_EX(Integer, L"FtpListAll", GetFtpListAll(), );
      WRITE_DATA_EX(Bool, L"Scp1Compatibility", GetScp1Compatibility(), );
      WRITE_DATA_EX(Float, L"TimeDifference", GetTimeDifference(), );
      WRITE_DATA_EX(Bool, L"DeleteToRecycleBin", GetDeleteToRecycleBin(), );
      WRITE_DATA_EX(Bool, L"OverwrittenToRecycleBin", GetOverwrittenToRecycleBin(), );
      WRITE_DATA_EX(String, L"RecycleBinPath", GetRecycleBinPath(), );
      WRITE_DATA_EX(String, L"PostLoginCommands", GetPostLoginCommands(), );

      WRITE_DATA_EX(String, L"ReturnVar", GetReturnVar(), );
      WRITE_DATA_EX(Bool, L"LookupUserGroups", GetLookupUserGroups(), );
      WRITE_DATA_EX(Integer, L"EOLType", GetEOLType(), );
      Storage->DeleteValue(L"SFTPUtfBug");
      WRITE_DATA_EX(Integer, L"Utf", GetNotUtf(), );
      WRITE_DATA_EX(Integer, L"SendBuf", GetSendBuf(), );
      WRITE_DATA_EX(Bool, L"SshSimple", GetSshSimple(), );
    }

    WRITE_DATA_EX(Integer, L"ProxyMethod", GetProxyMethod(), );
    if (PuttyExport)
    {
      // support for Putty 0.53b and older
      int ProxyType;
      int ProxySOCKSVersion = 5;
      switch (GetProxyMethod()) {
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
    WRITE_DATA_EX(String, L"ProxyHost", GetProxyHost(), );
    WRITE_DATA_EX(Integer, L"ProxyPort", GetProxyPort(), );
    WRITE_DATA_EX(String, L"ProxyUsername", GetProxyUsername(), );
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
    #define WRITE_BUG(BUG) WRITE_DATA_CONV(Integer, MB2W("Bug" #BUG), GetBug(sb##BUG));
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
      WRITE_DATA_EX(String, L"Protocol", GetProtocolStr(), );
    }

    if (!PuttyExport)
    {
      WRITE_DATA_EX(String, L"SftpServer", GetSftpServer(), );

      #define WRITE_SFTP_BUG(BUG) WRITE_DATA_EX(Integer, MB2W("SFTP" #BUG "Bug"), GetSFTPBug(sb##BUG), );
      WRITE_SFTP_BUG(Symlink);
      WRITE_SFTP_BUG(SignedTS);
      #undef WRITE_SFTP_BUG

      WRITE_DATA_EX(Integer, L"SFTPMaxVersion", GetSFTPMaxVersion(), );
      WRITE_DATA_EX(Integer, L"SFTPMinPacketSize", GetSFTPMinPacketSize(), );
      WRITE_DATA_EX(Integer, L"SFTPMaxPacketSize", GetSFTPMaxPacketSize(), );

      WRITE_DATA_EX(Integer, L"Color", GetColor(), );

      WRITE_DATA_EX(Bool, L"Tunnel", GetTunnel(), );
      WRITE_DATA_EX(String, L"TunnelHostName", GetTunnelHostName(), );
      WRITE_DATA_EX(Integer, L"TunnelPortNumber", GetTunnelPortNumber(), );
      WRITE_DATA_EX(String, L"TunnelUserName", GetTunnelUserName(), );
      WRITE_DATA_EX(String, L"TunnelPublicKeyFile", GetTunnelPublicKeyFile(), );
      WRITE_DATA_EX(Integer, L"TunnelLocalPortNumber", GetTunnelLocalPortNumber(), );

      WRITE_DATA_EX(Bool, L"FtpPasvMode", GetFtpPasvMode(), );
      WRITE_DATA_EX(Bool, L"FtpForcePasvIp", GetFtpForcePasvIp(), );
      WRITE_DATA_EX(String, L"FtpAccount", GetFtpAccount(), );
      WRITE_DATA_EX(Integer, L"FtpPingInterval", GetFtpPingInterval(), );
      WRITE_DATA_EX(Integer, L"FtpPingType", GetFtpPingType(), );
      WRITE_DATA_EX(Integer, L"Ftps", GetFtps(), );

      WRITE_DATA_EX(Integer, L"FtpProxyLogonType", GetFtpProxyLogonType(), );

      WRITE_DATA_EX(String, L"CustomParam1", GetCustomParam1(), );
      WRITE_DATA_EX(String, L"CustomParam2", GetCustomParam2(), );

      WRITE_DATA_EX(Bool, L"FtpAllowEmptyPassword", GetFtpAllowEmptyPassword(), );
      WRITE_DATA_EX(Integer, L"FtpEncryption", GetFtpEncryption(), );
      WRITE_DATA_EX(Bool, L"SslSessionReuse", GetSslSessionReuse(), );
    }

    SavePasswords(Storage, PuttyExport);

    Storage->CloseSubKey();
  }
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SavePasswords(THierarchicalStorage * Storage, bool PuttyExport)
{
  if (!Configuration->GetDisablePasswordStoring() && !PuttyExport && !FPassword.IsEmpty())
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

    if (!Configuration->GetDisablePasswordStoring() && !FTunnelPassword.IsEmpty())
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
void __fastcall TSessionData::RecryptPasswords()
{
  SetPassword(GetPassword());
  SetProxyPassword(GetProxyPassword());
  SetTunnelPassword(GetTunnelPassword());
}
//---------------------------------------------------------------------
bool __fastcall TSessionData::HasAnyPassword()
{
  return !FPassword.IsEmpty() || !FProxyPassword.IsEmpty() || !FTunnelPassword.IsEmpty();
}
//---------------------------------------------------------------------
void __fastcall TSessionData::Modify()
{
  FModified = true;
  if (FSource == ssStored)
  {
    FSource = ssStoredModified;
  }
}
//---------------------------------------------------------------------
UnicodeString __fastcall TSessionData::GetSource()
{
  switch (FSource)
  {
    case ::ssNone:
      return L"Ad-Hoc session";

    case ssStored:
      return L"Stored session";

    case ssStoredModified:
      return L"Modified stored session";

    default:
      assert(false);
      return L"";
  }
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SaveRecryptedPasswords(THierarchicalStorage * Storage)
{
  if (Storage->OpenSubKey(GetInternalStorageKey(), true))
  {
    RecryptPasswords();

    SavePasswords(Storage, false);

    Storage->CloseSubKey();
  }
}
//---------------------------------------------------------------------
void __fastcall TSessionData::Remove()
{
  THierarchicalStorage * Storage = Configuration->CreateScpStorage(true);
  // try
  {
    BOOST_SCOPE_EXIT ( (&Storage) )
    {
      delete Storage;
    } BOOST_SCOPE_EXIT_END
    Storage->SetExplicit(true);
    if (Storage->OpenSubKey(Configuration->GetStoredSessionsSubKey(), false))
    {
      Storage->RecursiveDeleteSubKey(GetInternalStorageKey());
    }
  }
#ifndef _MSC_VER
  __finally
  {
    delete Storage;
  }
#endif
}
//---------------------------------------------------------------------
bool __fastcall TSessionData::ParseUrl(UnicodeString Url, TOptions * Options,
  TStoredSessionList * StoredSessions, bool & DefaultsOnly, UnicodeString * FileName,
  bool * AProtocolDefined)
{
  bool ProtocolDefined = false;
  bool PortNumberDefined = false;
  TFSProtocol AFSProtocol = fsSCPonly;
  int APortNumber = 0;
  TFtps AFtps = ftpsNone;
  if (Url.SubString(1, 7).LowerCase() == L"netbox:")
  {
    // Remove "netbox:" prefix
    Url.Delete(1, 7);
  }
  if (Url.SubString(1, 4).LowerCase() == L"scp:")
  {
    AFSProtocol = fsSCPonly;
    APortNumber = SshPortNumber;
    Url.Delete(1, 4);
    ProtocolDefined = true;
  }
  else if (Url.SubString(1, 5).LowerCase() == L"sftp:")
  {
    AFSProtocol = fsSFTPonly;
    APortNumber = SshPortNumber;
    Url.Delete(1, 5);
    ProtocolDefined = true;
  }
  else if (Url.SubString(1, 4).LowerCase() == L"ftp:")
  {
    AFSProtocol = fsFTP;
    SetFtps(ftpsNone);
    APortNumber = FtpPortNumber;
    Url.Delete(1, 4);
    ProtocolDefined = true;
  }
  else if (Url.SubString(1, 5).LowerCase() == L"ftps:")
  {
    AFSProtocol = fsFTP;
    AFtps = ftpsImplicit;
    APortNumber = FtpsImplicitPortNumber;
    Url.Delete(1, 5);
    ProtocolDefined = true;
  }
  else if (Url.SubString(1, 5).LowerCase() == L"http:")
  {
    AFSProtocol = fsHTTP;
    APortNumber = HTTPPortNumber;
    Url.Delete(1, 5);
    ProtocolDefined = true;
  }
  else if (Url.SubString(1, 6).LowerCase() == L"https:")
  {
    AFSProtocol = fsHTTPS;
    APortNumber = HTTPSPortNumber;
    Url.Delete(1, 6);
    ProtocolDefined = true;
  }

  if (ProtocolDefined && (Url.SubString(1, 2) == L"//"))
  {
    Url.Delete(1, 2);
  }

  if (AProtocolDefined != NULL)
  {
    *AProtocolDefined = ProtocolDefined;
  }

  if (!Url.IsEmpty())
  {
    UnicodeString DecodedUrl = DecodeUrlChars(Url);
    // lookup stored session even if protocol was defined
    // (this allows setting for example default username for host
    // by creating stored session named by host)
    TSessionData * Data = NULL;
    for (Integer Index = 0; Index < StoredSessions->GetCount() + StoredSessions->GetHiddenCount(); Index++)
    {
      TSessionData * AData = static_cast<TSessionData *>(StoredSessions->GetItem(Index));
      if (AnsiSameText(AData->GetName(), DecodedUrl) ||
          AnsiSameText(AData->GetName() + L"/", DecodedUrl.SubString(1, AData->GetName().Length() + 1)))
      {
        Data = AData;
        break;
      }
    }

    UnicodeString ARemoteDirectory;

    if (Data != NULL)
    {
      DefaultsOnly = false;
      Assign(Data);
      int P = 1;
      while (!AnsiSameText(DecodeUrlChars(Url.SubString(1, P)), Data->GetName()))
      {
        P++;
        assert(P <= Url.Length());
      }
      ARemoteDirectory = Url.SubString(P + 1, Url.Length() - P);

      if (Data->GetHidden())
      {
        Data->Remove();
        StoredSessions->Remove(Data);
        // only modified, implicit
        StoredSessions->Save(false, false);
      }
    }
    else
    {
      Assign(StoredSessions->GetDefaultSettings());
      SetName(L"");

      int PSlash = Url.Pos(L"/");
      if (PSlash == 0)
      {
        PSlash = Url.Length() + 1;
      }

      UnicodeString ConnectInfo = Url.SubString(1, PSlash - 1);

      int P = ConnectInfo.LastDelimiter(L"@");

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

      if ((HostInfo.Length() >= 2) && (HostInfo[1] == L'[') && ((P = HostInfo.Pos(L"]")) > 0))
      {
        SetHostName(HostInfo.SubString(2, P - 2));
        HostInfo.Delete(1, P);
        if (!HostInfo.IsEmpty() && (HostInfo[1] == ':'))
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

      bool PasswordSeparator = (UserInfo.Pos(L':') != 0);
      SetUserName(DecodeUrlChars(CutToChar(UserInfo, L':', false)));
      SetPassword(DecodeUrlChars(UserInfo));
      SetPasswordless(GetPassword().IsEmpty() && PasswordSeparator);

      if (PSlash <= Url.Length())
      {
        ARemoteDirectory = Url.SubString(PSlash, Url.Length() - PSlash + 1);
      }
    }

    if (!ARemoteDirectory.IsEmpty() && (ARemoteDirectory != L"/"))
    {
      if ((ARemoteDirectory[ARemoteDirectory.Length()] != L'/') &&
          (FileName != NULL))
      {
        *FileName = DecodeUrlChars(UnixExtractFileName(ARemoteDirectory));
        ARemoteDirectory = UnixExtractFilePath(ARemoteDirectory);
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

  if (Options != NULL)
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
      SetTimeout(StrToInt(Value));
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
      TStrings * RawSettings = NULL;
      // TOptionsStorage * OptionsStorage = NULL;
      TRegistryStorage * OptionsStorage = NULL;
      // try
      {
        BOOST_SCOPE_EXIT ( (&RawSettings) (&OptionsStorage) )
        {
          delete RawSettings;
          delete OptionsStorage;
        } BOOST_SCOPE_EXIT_END
        RawSettings = new TStringList();

        if (Options->FindSwitch(L"rawsettings", RawSettings))
        {
          // OptionsStorage = new TOptionsStorage(RawSettings);
          OptionsStorage = new TRegistryStorage(Configuration->GetRegistryStorageKey());

          bool Dummy;
          DoLoad(OptionsStorage, Dummy);
        }
      }
#ifndef _MSC_VER
      __finally
      {
        delete RawSettings;
        delete OptionsStorage;
      }
#endif
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
  }

  return true;
}
//---------------------------------------------------------------------
void __fastcall TSessionData::ConfigureTunnel(int APortNumber)
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
void __fastcall TSessionData::RollbackTunnel()
{
  SetHostName(FOrigHostName);
  SetPortNumber(FOrigPortNumber);
  SetProxyMethod(FOrigProxyMethod);
}
//---------------------------------------------------------------------
void __fastcall TSessionData::ExpandEnvironmentVariables()
{
  SetHostName(GetHostNameExpanded());
  SetUserName(GetUserNameExpanded());
  SetPublicKeyFile(::ExpandEnvironmentVariables(GetPublicKeyFile()));
}
//---------------------------------------------------------------------
void __fastcall TSessionData::ValidatePath(const UnicodeString Path)
{
  // noop
}
//---------------------------------------------------------------------
void __fastcall TSessionData::ValidateName(const UnicodeString Name)
{
  if (Name.LastDelimiter(L"/") > 0)
  {
    throw Exception(FMTLOAD(ITEM_NAME_INVALID, Name.c_str(), L"/"));
  }
}
//---------------------------------------------------------------------
RawByteString __fastcall TSessionData::EncryptPassword(const UnicodeString & Password, UnicodeString Key)
{
  return Configuration->EncryptPassword(Password, Key);
}
//---------------------------------------------------------------------
RawByteString __fastcall TSessionData::StronglyRecryptPassword(const RawByteString & Password, UnicodeString Key)
{
  return Configuration->StronglyRecryptPassword(Password, Key);
}
//---------------------------------------------------------------------
UnicodeString __fastcall TSessionData::DecryptPassword(const RawByteString & Password, UnicodeString Key)
{
  UnicodeString Result;
  try
  {
    Result = Configuration->DecryptPassword(Password, Key);
  }
  catch(EAbort &)
  {
    // silently ignore aborted prompts for master password and return empty password
  }
  return Result;
}
//---------------------------------------------------------------------
bool __fastcall TSessionData::GetCanLogin()
{
  return !FHostName.IsEmpty();
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TSessionData::GetSessionKey()
{
  return FORMAT(L"%s@%s", GetUserName().c_str(), GetHostName().c_str());
}
//---------------------------------------------------------------------
UnicodeString __fastcall TSessionData::GetInternalStorageKey()
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
UnicodeString __fastcall TSessionData::GetStorageKey()
{
  return GetSessionName();
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetHostName(UnicodeString value)
{
  if (FHostName != value)
  {
    RemoveProtocolPrefix(value);
    // HostName is key for password encryption
    UnicodeString XPassword = GetPassword();

    int P = value.LastDelimiter(L"@");
    if (P > 0)
    {
      SetUserName(value.SubString(1, P - 1));
      value = value.SubString(P + 1, value.Length() - P);
    }
    FHostName = value;
    Modify();

    SetPassword(XPassword);
    if (!XPassword.IsEmpty())
    {
      XPassword.Unique();
      memset((void *)XPassword.c_str(), 0, XPassword.Length() * sizeof(*XPassword.c_str()));
    }
  }
}
//---------------------------------------------------------------------
UnicodeString __fastcall TSessionData::GetHostNameExpanded()
{
  return ::ExpandEnvironmentVariables(GetHostName());
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetPortNumber(int value)
{
  SET_SESSION_PROPERTY(PortNumber);
}
//---------------------------------------------------------------------------
void __fastcall TSessionData::SetShell(UnicodeString value)
{
  SET_SESSION_PROPERTY(Shell);
}
//---------------------------------------------------------------------------
void __fastcall TSessionData::SetSftpServer(UnicodeString value)
{
  SET_SESSION_PROPERTY(SftpServer);
}
//---------------------------------------------------------------------------
void __fastcall TSessionData::SetClearAliases(bool value)
{
  SET_SESSION_PROPERTY(ClearAliases);
}
//---------------------------------------------------------------------------
void __fastcall TSessionData::SetListingCommand(UnicodeString value)
{
  SET_SESSION_PROPERTY(ListingCommand);
}
//---------------------------------------------------------------------------
void __fastcall TSessionData::SetIgnoreLsWarnings(bool value)
{
  SET_SESSION_PROPERTY(IgnoreLsWarnings);
}
//---------------------------------------------------------------------------
void __fastcall TSessionData::SetUnsetNationalVars(bool value)
{
  SET_SESSION_PROPERTY(UnsetNationalVars);
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetUserName(UnicodeString value)
{
  // UserName is key for password encryption
  UnicodeString XPassword = GetPassword();
  SET_SESSION_PROPERTY(UserName);
  SetPassword(XPassword);
  if (!XPassword.IsEmpty())
  {
    XPassword.Unique();
    memset((void *)XPassword.c_str(), 0, XPassword.Length() * sizeof(*XPassword.c_str()));
  }
}
//---------------------------------------------------------------------
UnicodeString __fastcall TSessionData::GetUserNameExpanded()
{
  return ::ExpandEnvironmentVariables(GetUserName());
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetPassword(UnicodeString avalue)
{
  if (!avalue.IsEmpty())
  {
    SetPasswordless(false);
  }
  RawByteString value = EncryptPassword(avalue, GetUserName() + GetHostName());
  SET_SESSION_PROPERTY(Password);
}
//---------------------------------------------------------------------
UnicodeString __fastcall TSessionData::GetPassword()
{
  return DecryptPassword(FPassword, GetUserName() + GetHostName());
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetPasswordless(bool value)
{
  SET_SESSION_PROPERTY(Passwordless);
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetPingInterval(int value)
{
  SET_SESSION_PROPERTY(PingInterval);
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetTryAgent(bool value)
{
  SET_SESSION_PROPERTY(TryAgent);
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetAgentFwd(bool value)
{
  SET_SESSION_PROPERTY(AgentFwd);
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetAuthTIS(bool value)
{
  SET_SESSION_PROPERTY(AuthTIS);
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetAuthKI(bool value)
{
  SET_SESSION_PROPERTY(AuthKI);
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetAuthKIPassword(bool value)
{
  SET_SESSION_PROPERTY(AuthKIPassword);
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetAuthGSSAPI(bool value)
{
  SET_SESSION_PROPERTY(AuthGSSAPI);
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetGSSAPIFwdTGT(bool value)
{
  SET_SESSION_PROPERTY(GSSAPIFwdTGT);
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetGSSAPIServerRealm(UnicodeString value)
{
  SET_SESSION_PROPERTY(GSSAPIServerRealm);
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetChangeUsername(bool value)
{
  SET_SESSION_PROPERTY(ChangeUsername);
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetCompression(bool value)
{
  SET_SESSION_PROPERTY(Compression);
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetSshProt(TSshProt value)
{
  SET_SESSION_PROPERTY(SshProt);
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetSsh2DES(bool value)
{
  SET_SESSION_PROPERTY(Ssh2DES);
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetSshNoUserAuth(bool value)
{
  SET_SESSION_PROPERTY(SshNoUserAuth);
}
//---------------------------------------------------------------------
UnicodeString __fastcall TSessionData::GetSshProtStr()
{
  return SshProtList[FSshProt];
}
//---------------------------------------------------------------------
bool __fastcall TSessionData::GetUsesSsh()
{
  return (GetFSProtocol() < fsFTP);
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetCipher(int Index, TCipher value)
{
  assert(Index >= 0 && Index < CIPHER_COUNT);
  SET_SESSION_PROPERTY(Ciphers[Index]);
}
//---------------------------------------------------------------------
TCipher __fastcall TSessionData::GetCipher(int Index) const
{
  assert(Index >= 0 && Index < CIPHER_COUNT);
  return FCiphers[Index];
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetCipherList(UnicodeString value)
{
  bool Used[CIPHER_COUNT];
  for (int C = 0; C < CIPHER_COUNT; C++) { Used[C] = false; }

  UnicodeString CipherStr;
  int Index = 0;
  while (!value.IsEmpty() && (Index < CIPHER_COUNT))
  {
    CipherStr = CutToChar(value, L',', true);
    for (int C = 0; C < CIPHER_COUNT; C++)
    {
      if (!CipherStr.CompareIC(CipherNames[C]))
      {
        SetCipher(Index, static_cast<TCipher>(C));
        Used[C] = true;
        Index++;
        break;
      }
    }
  }

  for (int C = 0; C < CIPHER_COUNT && Index < CIPHER_COUNT; C++)
  {
    if (!Used[DefaultCipherList[C]]) { SetCipher(Index++, DefaultCipherList[C]); }
  }
}
//---------------------------------------------------------------------
UnicodeString __fastcall TSessionData::GetCipherList() const
{
  UnicodeString Result;
  for (int Index = 0; Index < CIPHER_COUNT; Index++)
  {
    Result += UnicodeString(Index ? L"," : L"") + CipherNames[GetCipher(Index)];
  }
  return Result;
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetKex(int Index, TKex value)
{
  assert(Index >= 0 && Index < KEX_COUNT);
  SET_SESSION_PROPERTY(Kex[Index]);
}
//---------------------------------------------------------------------
TKex __fastcall TSessionData::GetKex(int Index) const
{
  assert(Index >= 0 && Index < KEX_COUNT);
  return FKex[Index];
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetKexList(UnicodeString value)
{
  bool Used[KEX_COUNT];
  for (int K = 0; K < KEX_COUNT; K++) { Used[K] = false; }

  UnicodeString KexStr;
  int Index = 0;
  while (!value.IsEmpty() && (Index < KEX_COUNT))
  {
    KexStr = CutToChar(value, L',', true);
    for (int K = 0; K < KEX_COUNT; K++)
    {
      if (!KexStr.CompareIC(KexNames[K]))
      {
        SetKex(Index, static_cast<TKex>(K));
        Used[K] = true;
        Index++;
        break;
      }
    }
  }

  for (int K = 0; K < KEX_COUNT && Index < KEX_COUNT; K++)
  {
    if (!Used[DefaultKexList[K]]) { SetKex(Index++, DefaultKexList[K]); }
  }
}
//---------------------------------------------------------------------
UnicodeString __fastcall TSessionData::GetKexList() const
{
  UnicodeString Result;
  for (int Index = 0; Index < KEX_COUNT; Index++)
  {
    Result += UnicodeString(Index ? L"," : L"") + KexNames[GetKex(Index)];
  }
  return Result;
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetPublicKeyFile(UnicodeString value)
{
  if (FPublicKeyFile != value)
  {
    FPublicKeyFile = StripPathQuotes(value);
    Modify();
  }
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetReturnVar(UnicodeString value)
{
  SET_SESSION_PROPERTY(ReturnVar);
}
//---------------------------------------------------------------------------
void __fastcall TSessionData::SetLookupUserGroups(TAutoSwitch value)
{
  SET_SESSION_PROPERTY(LookupUserGroups);
}
//---------------------------------------------------------------------------
void __fastcall TSessionData::SetEOLType(TEOLType value)
{
  SET_SESSION_PROPERTY(EOLType);
}
//---------------------------------------------------------------------------
TDateTime __fastcall TSessionData::GetTimeoutDT()
{
  return SecToDateTime(GetTimeout());
}
//---------------------------------------------------------------------------
void __fastcall TSessionData::SetTimeout(int value)
{
  SET_SESSION_PROPERTY(Timeout);
}
//---------------------------------------------------------------------------
void __fastcall TSessionData::SetProtocol(TProtocol value)
{
  SET_SESSION_PROPERTY(Protocol);
}
//---------------------------------------------------------------------------
void __fastcall TSessionData::SetFSProtocol(TFSProtocol value)
{
  SET_SESSION_PROPERTY(FSProtocol);
}
//---------------------------------------------------------------------
UnicodeString __fastcall TSessionData::GetFSProtocolStr()
{
  assert(GetFSProtocol() >= 0 && GetFSProtocol() < FSPROTOCOL_COUNT);
  return FSProtocolNames[GetFSProtocol()];
}
//---------------------------------------------------------------------------
void __fastcall TSessionData::SetDetectReturnVar(bool value)
{
  if (value != GetDetectReturnVar())
  {
    SetReturnVar(value ? L"" : L"$?");
  }
}
//---------------------------------------------------------------------------
bool __fastcall TSessionData::GetDetectReturnVar()
{
  return GetReturnVar().IsEmpty();
}
//---------------------------------------------------------------------------
void __fastcall TSessionData::SetDefaultShell(bool value)
{
  if (value != GetDefaultShell())
  {
    SetShell(value ? L"" : L"/bin/bash");
  }
}
//---------------------------------------------------------------------------
bool __fastcall TSessionData::GetDefaultShell()
{
  return GetShell().IsEmpty();
}
//---------------------------------------------------------------------------
void __fastcall TSessionData::SetProtocolStr(UnicodeString value)
{
  FProtocol = ptRaw;
  for (int Index = 0; Index < PROTOCOL_COUNT; Index++)
  {
    if (value.CompareIC(ProtocolNames[Index]) == 0)
    {
      FProtocol = static_cast<TProtocol>(Index);
      break;
    }
  }
}
//---------------------------------------------------------------------
UnicodeString __fastcall TSessionData::GetProtocolStr() const
{
  return ProtocolNames[GetProtocol()];
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetPingIntervalDT(TDateTime value)
{
  unsigned short hour, min, sec, msec;

  value.DecodeTime(hour, min, sec, msec);
  SetPingInterval((static_cast<int>(hour))*SecsPerHour + (static_cast<int>(min))*SecsPerMin + sec);
}
//---------------------------------------------------------------------------
TDateTime __fastcall TSessionData::GetPingIntervalDT() const
{
  return SecToDateTime(GetPingInterval());
}
//---------------------------------------------------------------------------
void __fastcall TSessionData::SetPingType(TPingType value)
{
  SET_SESSION_PROPERTY(PingType);
}
//---------------------------------------------------------------------------
void __fastcall TSessionData::SetAddressFamily(TAddressFamily value)
{
  SET_SESSION_PROPERTY(AddressFamily);
}
//---------------------------------------------------------------------------
void __fastcall TSessionData::SetRekeyData(UnicodeString value)
{
  SET_SESSION_PROPERTY(RekeyData);
}
//---------------------------------------------------------------------------
void __fastcall TSessionData::SetRekeyTime(unsigned int value)
{
  SET_SESSION_PROPERTY(RekeyTime);
}
//---------------------------------------------------------------------
UnicodeString __fastcall TSessionData::GetDefaultSessionName()
{
  UnicodeString HostName = GetHostName();
  UnicodeString UserName = GetUserName();
  RemoveProtocolPrefix(HostName);
  if (!HostName.IsEmpty() && !UserName.IsEmpty())
  {
    return FORMAT(L"%s@%s", UserName.c_str(), HostName.c_str());
  }
  else if (!HostName.IsEmpty())
  {
    return HostName;
  }
  else
  {
    return L"session";
  }
}
//---------------------------------------------------------------------
bool __fastcall TSessionData::HasSessionName()
{
  return (!GetName().IsEmpty() && (GetName() != DefaultName));
}
//---------------------------------------------------------------------
UnicodeString __fastcall TSessionData::GetSessionName()
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
UnicodeString __fastcall TSessionData::GetSessionUrl()
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
        Url = L"ftp://";
        break;
      case fsFTPS:
        Url = L"ftps://";
        break;
      case fsHTTP:
        Url = L"http://";
        break;
      case fsHTTPS:
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
void __fastcall TSessionData::SetTimeDifference(TDateTime value)
{
  SET_SESSION_PROPERTY(TimeDifference);
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetLocalDirectory(UnicodeString value)
{
  SET_SESSION_PROPERTY(LocalDirectory);
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetRemoteDirectory(UnicodeString value)
{
  SET_SESSION_PROPERTY(RemoteDirectory);
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetSynchronizeBrowsing(bool value)
{
  SET_SESSION_PROPERTY(SynchronizeBrowsing);
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetUpdateDirectories(bool value)
{
  SET_SESSION_PROPERTY(UpdateDirectories);
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetCacheDirectories(bool value)
{
  SET_SESSION_PROPERTY(CacheDirectories);
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetCacheDirectoryChanges(bool value)
{
  SET_SESSION_PROPERTY(CacheDirectoryChanges);
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetPreserveDirectoryChanges(bool value)
{
  SET_SESSION_PROPERTY(PreserveDirectoryChanges);
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetResolveSymlinks(bool value)
{
  SET_SESSION_PROPERTY(ResolveSymlinks);
}
//---------------------------------------------------------------------------
void __fastcall TSessionData::SetDSTMode(TDSTMode value)
{
  SET_SESSION_PROPERTY(DSTMode);
}
//---------------------------------------------------------------------------
void __fastcall TSessionData::SetDeleteToRecycleBin(bool value)
{
  SET_SESSION_PROPERTY(DeleteToRecycleBin);
}
//---------------------------------------------------------------------------
void __fastcall TSessionData::SetOverwrittenToRecycleBin(bool value)
{
  SET_SESSION_PROPERTY(OverwrittenToRecycleBin);
}
//---------------------------------------------------------------------------
void __fastcall TSessionData::SetRecycleBinPath(UnicodeString value)
{
  SET_SESSION_PROPERTY(RecycleBinPath);
}
//---------------------------------------------------------------------------
void __fastcall TSessionData::SetPostLoginCommands(UnicodeString value)
{
  SET_SESSION_PROPERTY(PostLoginCommands);
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetLockInHome(bool value)
{
  SET_SESSION_PROPERTY(LockInHome);
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetSpecial(bool value)
{
  SET_SESSION_PROPERTY(Special);
}
//---------------------------------------------------------------------------
void __fastcall TSessionData::SetScp1Compatibility(bool value)
{
  SET_SESSION_PROPERTY(Scp1Compatibility);
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetTcpNoDelay(bool value)
{
  SET_SESSION_PROPERTY(TcpNoDelay);
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetSendBuf(int value)
{
  SET_SESSION_PROPERTY(SendBuf);
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetSshSimple(bool value)
{
  SET_SESSION_PROPERTY(SshSimple);
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetProxyMethod(TProxyMethod value)
{
  SET_SESSION_PROPERTY(ProxyMethod);
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetProxyHost(UnicodeString value)
{
  SET_SESSION_PROPERTY(ProxyHost);
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetProxyPort(int value)
{
  SET_SESSION_PROPERTY(ProxyPort);
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetProxyUsername(UnicodeString value)
{
  SET_SESSION_PROPERTY(ProxyUsername);
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetProxyPassword(UnicodeString avalue)
{
  RawByteString value = EncryptPassword(avalue, GetProxyUsername() + GetProxyHost());
  SET_SESSION_PROPERTY(ProxyPassword);
}
//---------------------------------------------------------------------
UnicodeString __fastcall TSessionData::GetProxyPassword() const
{
  return DecryptPassword(FProxyPassword, GetProxyUsername() + GetProxyHost());
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetProxyTelnetCommand(UnicodeString value)
{
  SET_SESSION_PROPERTY(ProxyTelnetCommand);
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetProxyLocalCommand(UnicodeString value)
{
  SET_SESSION_PROPERTY(ProxyLocalCommand);
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetProxyDNS(TAutoSwitch value)
{
  SET_SESSION_PROPERTY(ProxyDNS);
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetProxyLocalhost(bool value)
{
  SET_SESSION_PROPERTY(ProxyLocalhost);
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetFtpProxyLogonType(int value)
{
  SET_SESSION_PROPERTY(FtpProxyLogonType);
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetBug(TSshBug Bug, TAutoSwitch value)
{
  assert(Bug >= 0 && static_cast<unsigned int>(Bug) < LENOF(FBugs));
  SET_SESSION_PROPERTY(Bugs[Bug]);
}
//---------------------------------------------------------------------
TAutoSwitch __fastcall TSessionData::GetBug(TSshBug Bug) const
{
  assert(Bug >= 0 && static_cast<unsigned int>(Bug) < LENOF(FBugs));
  return FBugs[Bug];
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetCustomParam1(UnicodeString value)
{
  SET_SESSION_PROPERTY(CustomParam1);
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetCustomParam2(UnicodeString value)
{
  SET_SESSION_PROPERTY(CustomParam2);
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetSFTPDownloadQueue(int value)
{
  SET_SESSION_PROPERTY(SFTPDownloadQueue);
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetSFTPUploadQueue(int value)
{
  SET_SESSION_PROPERTY(SFTPUploadQueue);
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetSFTPListingQueue(int value)
{
  SET_SESSION_PROPERTY(SFTPListingQueue);
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetSFTPMaxVersion(int value)
{
  SET_SESSION_PROPERTY(SFTPMaxVersion);
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetSFTPMinPacketSize(unsigned long value)
{
  SET_SESSION_PROPERTY(SFTPMinPacketSize);
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetSFTPMaxPacketSize(unsigned long value)
{
  SET_SESSION_PROPERTY(SFTPMaxPacketSize);
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetSFTPBug(TSftpBug Bug, TAutoSwitch value)
{
  assert(Bug >= 0 && static_cast<unsigned int>(Bug) < LENOF(FSFTPBugs));
  SET_SESSION_PROPERTY(SFTPBugs[Bug]);
}
//---------------------------------------------------------------------
TAutoSwitch __fastcall TSessionData::GetSFTPBug(TSftpBug Bug) const
{
  assert(Bug >= 0 && static_cast<unsigned int>(Bug) < LENOF(FSFTPBugs));
  return FSFTPBugs[Bug];
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetSCPLsFullTime(TAutoSwitch value)
{
  SET_SESSION_PROPERTY(SCPLsFullTime);
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetFtpListAll(TAutoSwitch value)
{
  SET_SESSION_PROPERTY(FtpListAll);
}
//---------------------------------------------------------------------------
void __fastcall TSessionData::SetColor(int value)
{
  SET_SESSION_PROPERTY(Color);
}
//---------------------------------------------------------------------------
void __fastcall TSessionData::SetTunnel(bool value)
{
  SET_SESSION_PROPERTY(Tunnel);
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetTunnelHostName(UnicodeString value)
{
  if (FTunnelHostName != value)
  {
    // HostName is key for password encryption
    UnicodeString XTunnelPassword = GetTunnelPassword();

    int P = value.LastDelimiter(L"@");
    if (P > 0)
    {
      SetTunnelUserName(value.SubString(1, P - 1));
      value = value.SubString(P + 1, value.Length() - P);
    }
    FTunnelHostName = value;
    Modify();

    SetTunnelPassword(XTunnelPassword);
    if (!XTunnelPassword.IsEmpty())
    {
      XTunnelPassword.Unique();
      memset(const_cast<wchar_t *>(XTunnelPassword.c_str()), 0, XTunnelPassword.Length() * sizeof(*XTunnelPassword.c_str()));
    }
  }
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetTunnelPortNumber(int value)
{
  SET_SESSION_PROPERTY(TunnelPortNumber);
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetTunnelUserName(UnicodeString value)
{
  // TunnelUserName is key for password encryption
  UnicodeString XTunnelPassword = GetTunnelPassword();
  SET_SESSION_PROPERTY(TunnelUserName);
  SetTunnelPassword(XTunnelPassword);
  if (!XTunnelPassword.IsEmpty())
  {
    XTunnelPassword.Unique();
    memset(const_cast<wchar_t *>(XTunnelPassword.c_str()), 0, XTunnelPassword.Length() * sizeof(*XTunnelPassword.c_str()));
  }
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetTunnelPassword(UnicodeString avalue)
{
  RawByteString value = EncryptPassword(avalue, GetTunnelUserName() + GetTunnelHostName());
  SET_SESSION_PROPERTY(TunnelPassword);
}
//---------------------------------------------------------------------
UnicodeString __fastcall TSessionData::GetTunnelPassword()
{
  return DecryptPassword(FTunnelPassword, GetTunnelUserName() + GetTunnelHostName());
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetTunnelPublicKeyFile(UnicodeString value)
{
  if (FTunnelPublicKeyFile != value)
  {
    FTunnelPublicKeyFile = StripPathQuotes(value);
    Modify();
  }
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetTunnelLocalPortNumber(int value)
{
  SET_SESSION_PROPERTY(TunnelLocalPortNumber);
}
//---------------------------------------------------------------------
bool __fastcall TSessionData::GetTunnelAutoassignLocalPortNumber()
{
  return (FTunnelLocalPortNumber <= 0);
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetTunnelPortFwd(UnicodeString value)
{
  SET_SESSION_PROPERTY(TunnelPortFwd);
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetFtpPasvMode(bool value)
{
  SET_SESSION_PROPERTY(FtpPasvMode);
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetFtpAllowEmptyPassword(bool value)
{
  SET_SESSION_PROPERTY(FtpAllowEmptyPassword);
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetFtpEncryption(TFtpEncryptionSwitch value)
{
  SET_SESSION_PROPERTY(FtpEncryption);
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetFtpForcePasvIp(bool value)
{
  SET_SESSION_PROPERTY(FtpForcePasvIp);
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetFtpAccount(UnicodeString value)
{
  SET_SESSION_PROPERTY(FtpAccount);
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetFtpPingInterval(int value)
{
  SET_SESSION_PROPERTY(FtpPingInterval);
}
//---------------------------------------------------------------------------
TDateTime __fastcall TSessionData::GetFtpPingIntervalDT()
{
  return SecToDateTime(GetFtpPingInterval());
}
//---------------------------------------------------------------------------
void __fastcall TSessionData::SetFtpPingType(TPingType value)
{
  SET_SESSION_PROPERTY(FtpPingType);
}
//---------------------------------------------------------------------------
void __fastcall TSessionData::SetFtps(TFtps value)
{
  SET_SESSION_PROPERTY(Ftps);
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetNotUtf(TAutoSwitch value)
{
  SET_SESSION_PROPERTY(NotUtf);
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetHostKey(UnicodeString value)
{
  SET_SESSION_PROPERTY(HostKey);
}
//---------------------------------------------------------------------
UnicodeString __fastcall TSessionData::GetInfoTip()
{
  if (GetUsesSsh())
  {
    return FMTLOAD(SESSION_INFO_TIP,
        GetHostName().c_str(), GetUserName().c_str(),
         (GetPublicKeyFile().IsEmpty() ? LoadStr(NO_STR).c_str() : LoadStr(YES_STR).c_str()),
         GetSshProtStr().c_str(), GetFSProtocolStr().c_str());
  }
  else
  {
    return FMTLOAD(SESSION_INFO_TIP_NO_SSH,
      GetHostName().c_str(), GetUserName().c_str(), GetFSProtocolStr().c_str());
  }
}
//---------------------------------------------------------------------
UnicodeString __fastcall TSessionData::GetLocalName()
{
  UnicodeString Result;
  if (HasSessionName())
  {
    Result = GetName();
    int P = Result.LastDelimiter(L"/");
    if (P > 0)
    {
      Result.Delete(1, P);
    }
  }
  else
  {
    Result = GetDefaultSessionName();
  }
  return Result;
}
//---------------------------------------------------------------------
TLoginType __fastcall TSessionData::GetLoginType() const
{
  return (GetUserName() == CONST_LOGIN_ANONYMOUS) && GetPassword().IsEmpty() ?
         ltAnonymous : ltNormal;
}
//---------------------------------------------------------------------
void __fastcall TSessionData::SetLoginType(TLoginType value)
{
  SET_SESSION_PROPERTY(LoginType);
  if (GetLoginType() == ltAnonymous)
  {
    SetPassword(L"");
    SetUserName(CONST_LOGIN_ANONYMOUS);
  }
}
//---------------------------------------------------------------------
unsigned int __fastcall TSessionData::GetCodePageAsNumber() const
{
  return ::GetCodePageAsNumber(GetCodePage());
}

//---------------------------------------------------------------------------
void __fastcall TSessionData::SetCodePage(const UnicodeString value)
{
  SET_SESSION_PROPERTY(CodePage);
}
//---------------------------------------------------------------------
void __fastcall TSessionData::AdjustHostName(UnicodeString & hostName, const UnicodeString prefix)
{
  if (::LowerCase(hostName.SubString(1, prefix.Length())) == prefix)
  {
    hostName.Delete(1, prefix.Length());
    hostName = ::ReplaceStrAll(hostName, L"/", L"_");
  }
}
//---------------------------------------------------------------------
void __fastcall TSessionData::RemoveProtocolPrefix(UnicodeString & hostName)
{
  AdjustHostName(hostName, L"scp://");
  AdjustHostName(hostName, L"sftp://");
  AdjustHostName(hostName, L"ftp://");
  AdjustHostName(hostName, L"ftps://");
  AdjustHostName(hostName, L"http://");
  AdjustHostName(hostName, L"https://");
}

//---------------------------------------------------------------------
void __fastcall TSessionData::SetSslSessionReuse(bool value)
{
  SET_SESSION_PROPERTY(SslSessionReuse);
}
//=== TStoredSessionList ----------------------------------------------
/* __fastcall */ TStoredSessionList::TStoredSessionList(bool aReadOnly):
  TNamedObjectList(), FReadOnly(aReadOnly)
{
  assert(Configuration);
  FDefaultSettings = new TSessionData(DefaultName);
}
//---------------------------------------------------------------------
/* __fastcall */ TStoredSessionList::~TStoredSessionList()
{
  assert(Configuration);
  delete FDefaultSettings;
}
//---------------------------------------------------------------------
void __fastcall TStoredSessionList::Load(THierarchicalStorage * Storage,
  bool AsModified, bool UseDefaults)
{
  TStringList *SubKeys = new TStringList();
  TList * Loaded = new TList;
  // try
  {
    BOOST_SCOPE_EXIT ( (&SubKeys) (&Loaded) )
    {
      delete SubKeys;
      delete Loaded;
    } BOOST_SCOPE_EXIT_END
    Storage->GetSubKeyNames(SubKeys);
    for (int Index = 0; Index < SubKeys->GetCount(); Index++)
    {
      TSessionData * SessionData = NULL;
      UnicodeString SessionName = SubKeys->GetStrings(Index);
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
        if (SessionName == FDefaultSettings->GetName()) { SessionData = FDefaultSettings; }
          else { SessionData = static_cast<TSessionData*>(FindByName(SessionName)); }

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
      for (int Index = 0; Index < TObjectList::GetCount(); Index++)
      {
        if (Loaded->IndexOf(GetItem(Index)) < 0)
        {
          Delete(Index);
          Index--;
        }
      }
    }
  }
#ifndef _MSC_VER
  __finally
  {
    delete SubKeys;
    delete Loaded;
  }
#endif
}
//---------------------------------------------------------------------
void __fastcall TStoredSessionList::Load()
{
  THierarchicalStorage * Storage = Configuration->CreateScpStorage(true);
  // try
  {
    BOOST_SCOPE_EXIT ( (&Storage) )
    {
      delete Storage;
    } BOOST_SCOPE_EXIT_END
    if (Storage->OpenSubKey(Configuration->GetStoredSessionsSubKey(), false))
    {
      Load(Storage);
    }
  }
#ifndef _MSC_VER
  __finally
  {
    delete Storage;
  }
#endif
}
//---------------------------------------------------------------------
void __fastcall TStoredSessionList::DoSave(THierarchicalStorage * Storage,
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
void __fastcall TStoredSessionList::DoSave(THierarchicalStorage * Storage,
  bool All, bool RecryptPasswordOnly)
{
  TSessionData * FactoryDefaults = new TSessionData(L"");
  // try
  {
    BOOST_SCOPE_EXIT ( (&FactoryDefaults) )
    {
      delete FactoryDefaults;
    } BOOST_SCOPE_EXIT_END
    DoSave(Storage, FDefaultSettings, All, RecryptPasswordOnly, FactoryDefaults);
    for (int Index = 0; Index < GetCount() + GetHiddenCount(); Index++)
    {
      TSessionData * SessionData = static_cast<TSessionData *>(GetItem(Index));
      DoSave(Storage, SessionData, All, RecryptPasswordOnly, FactoryDefaults);
    }
  }
#ifndef _MSC_VER
  __finally
  {
    delete FactoryDefaults;
  }
#endif
}
//---------------------------------------------------------------------
void __fastcall TStoredSessionList::Save(THierarchicalStorage * Storage, bool All)
{
  DoSave(Storage, All, false);
}
//---------------------------------------------------------------------
void __fastcall TStoredSessionList::DoSave(bool All, bool Explicit, bool RecryptPasswordOnly)
{
  THierarchicalStorage * Storage = Configuration->CreateScpStorage(true);
  // try
  {
    BOOST_SCOPE_EXIT ( (&Storage) )
    {
      delete Storage;
    } BOOST_SCOPE_EXIT_END
    Storage->SetAccessMode(smReadWrite);
    Storage->SetExplicit(Explicit);
    if (Storage->OpenSubKey(Configuration->GetStoredSessionsSubKey(), true))
    {
      DoSave(Storage, All, RecryptPasswordOnly);
    }
  }
#ifndef _MSC_VER
  __finally
  {
    delete Storage;
  }
#endif

  Saved();
}
//---------------------------------------------------------------------
void __fastcall TStoredSessionList::Save(bool All, bool Explicit)
{
  DoSave(All, Explicit, false);
}
//---------------------------------------------------------------------
void __fastcall TStoredSessionList::RecryptPasswords()
{
  DoSave(true, true, true);
}
//---------------------------------------------------------------------
void __fastcall TStoredSessionList::Saved()
{
  FDefaultSettings->SetModified(false);
  for (int Index = 0; Index < GetCount() + GetHiddenCount(); Index++)
  {
    (static_cast<TSessionData *>(GetItem(Index))->SetModified(false));
  }
}
//---------------------------------------------------------------------
void __fastcall TStoredSessionList::Export(const UnicodeString FileName)
{
  Error(SNotImplemented, 3003);
/*
  THierarchicalStorage * Storage = new TIniFileStorage(FileName);
  // try
  {
    BOOST_SCOPE_EXIT ( (&Storage) )
    {
      delete Storage;
    } BOOST_SCOPE_EXIT_END
    Storage->SetAccessMode(smReadWrite);
    if (Storage->OpenSubKey(Configuration->GetStoredSessionsSubKey(), true))
    {
      Save(Storage, true);
    }
  }
  __finally
  {
    delete Storage;
  }
*/
}
//---------------------------------------------------------------------
void __fastcall TStoredSessionList::SelectAll(bool Select)
{
  for (int Index = 0; Index < GetCount(); Index++)
  {
    GetSession(Index)->SetSelected(Select);
  }
}
//---------------------------------------------------------------------
void __fastcall TStoredSessionList::Import(TStoredSessionList * From,
  bool OnlySelected)
{
  for (int Index = 0; Index < From->GetCount(); Index++)
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
void __fastcall TStoredSessionList::SelectSessionsToImport
  (TStoredSessionList * Dest, bool SSHOnly)
{
  for (int Index = 0; Index < GetCount(); Index++)
  {
    GetSession(Index)->SetSelected(
      (!SSHOnly || (GetSession(Index)->GetProtocol() == ptSSH)) &&
      !Dest->FindByName(GetSession(Index)->GetName()));
  }
}
//---------------------------------------------------------------------
void __fastcall TStoredSessionList::Cleanup()
{
  try
  {
    if (Configuration->GetStorage() == stRegistry) { Clear(); }
    TRegistryStorage * Storage = new TRegistryStorage(Configuration->GetRegistryStorageKey());
    // try
    {
      BOOST_SCOPE_EXIT ( (&Storage) )
      {
        delete Storage;
      } BOOST_SCOPE_EXIT_END
      Storage->SetAccessMode(smReadWrite);
      if (Storage->OpenRootKey(false))
      {
        Storage->RecursiveDeleteSubKey(Configuration->GetStoredSessionsSubKey());
      }
    }
#ifndef _MSC_VER
    __finally
    {
      delete Storage;
    }
#endif
  }
  catch (Exception &E)
  {
    throw ExtException(&E, FMTLOAD(CLEANUP_SESSIONS_ERROR));
  }
}
//---------------------------------------------------------------------------
int __fastcall TStoredSessionList::IndexOf(TSessionData * Data)
{
  for (int Index = 0; Index < GetCount(); Index++)
  {
    if (Data == GetSession(Index))
    {
      return Index;
    }
  }
  return -1;
}
//---------------------------------------------------------------------------
TSessionData * __fastcall TStoredSessionList::NewSession(
  UnicodeString SessionName, TSessionData * Session)
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
void __fastcall TStoredSessionList::SetDefaultSettings(TSessionData * value)
{
  assert(FDefaultSettings);
  if (FDefaultSettings != value)
  {
    FDefaultSettings->Assign(value);
    FDefaultSettings->SetName(DefaultName);
    if (!FReadOnly)
    {
      // only modified, explicit
      Save(false, true);
    }
  }
}
//---------------------------------------------------------------------------
void __fastcall TStoredSessionList::ImportHostKeys(const UnicodeString TargetKey,
  const UnicodeString SourceKey, TStoredSessionList * Sessions,
  bool OnlySelected)
{
  TRegistryStorage * SourceStorage = NULL;
  TRegistryStorage * TargetStorage = NULL;
  TStringList * KeyList = NULL;
  // try
  {
    BOOST_SCOPE_EXIT ( (&SourceStorage) (&TargetStorage) (&KeyList) )
    {
      delete SourceStorage;
      delete TargetStorage;
      delete KeyList;
    } BOOST_SCOPE_EXIT_END
    SourceStorage = new TRegistryStorage(SourceKey);
    TargetStorage = new TRegistryStorage(TargetKey);
    TargetStorage->SetAccessMode(smReadWrite);
    KeyList = new TStringList();

    if (SourceStorage->OpenRootKey(false) &&
        TargetStorage->OpenRootKey(true))
    {
      SourceStorage->GetValueNames(KeyList);

      TSessionData * Session;
      UnicodeString HostKeyName;
      assert(Sessions != NULL);
      for (int Index = 0; Index < Sessions->GetCount(); Index++)
      {
        Session = Sessions->GetSession(Index);
        if (!OnlySelected || Session->GetSelected())
        {
          HostKeyName = PuttyMungeStr(FORMAT(L"@%d:%s", Session->GetPortNumber(), Session->GetHostName().c_str()));
          UnicodeString KeyName;
          for (int KeyIndex = 0; KeyIndex < KeyList->GetCount(); KeyIndex++)
          {
            KeyName = KeyList->GetStrings(KeyIndex);
            int P = KeyName.Pos(HostKeyName);
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
#ifndef _MSC_VER
  __finally
  {
    delete SourceStorage;
    delete TargetStorage;
    delete KeyList;
  }
#endif
}
//---------------------------------------------------------------------------
TSessionData * __fastcall TStoredSessionList::ParseUrl(UnicodeString Url,
  TOptions * Options, bool & DefaultsOnly, UnicodeString * FileName,
  bool * AProtocolDefined)
{
  TSessionData * Data = new TSessionData(L"");
  try
  {
    Data->ParseUrl(Url, Options, this, DefaultsOnly, FileName, AProtocolDefined);
  }
  catch(...)
  {
    delete Data;
    throw;
  }

  return Data;
}
//---------------------------------------------------------------------------
TSessionData * TStoredSessionList::GetSessionByName(const UnicodeString SessionName)
{
  for (int I = 0; I < GetCount(); I++)
  {
    TSessionData * SessionData = GetSession(I);
    if (SessionData->GetName() == SessionName)
    {
      return SessionData;
    }
  }
  return NULL;
}

//---------------------------------------------------------------------
void __fastcall TStoredSessionList::Load(const UnicodeString aKey, bool UseDefaults)
{
  TRegistryStorage * Storage = new TRegistryStorage(aKey);
  {
    BOOST_SCOPE_EXIT ( (&Storage) )
    {
      delete Storage;
    } BOOST_SCOPE_EXIT_END
    if (Storage->OpenRootKey(false)) { Load(Storage, false, UseDefaults); }
  }
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

  if (CodePageInfoEx.MaxCharSize != 1)
    return false;

  return true;
}
//---------------------------------------------------------------------
unsigned int GetCodePageAsNumber(const UnicodeString CodePage)
{
  unsigned int codePage = _wtoi(CodePage.c_str());
  return codePage == 0 ? CONST_DEFAULT_CODEPAGE : codePage;
}
//---------------------------------------------------------------------
UnicodeString GetCodePageAsString(unsigned int cp)
{
  CPINFOEX cpInfoEx;
  if (::GetCodePageInfo(cp, cpInfoEx))
  {
    return UnicodeString(cpInfoEx.CodePageName);
  }
  return IntToStr(CONST_DEFAULT_CODEPAGE);
}

//---------------------------------------------------------------------
