//---------------------------------------------------------------------------
#include "stdafx.h"

#include "boostdefines.hpp"
#include <boost/scope_exit.hpp>

#include "SessionData.h"

#include "Common.h"
#include "Exceptions.h"
// #include "FileBuffer.h"
#include "CoreMain.h"
#include "TextsCore.h"
#include "PuttyIntf.h"
#include "RemoteFiles.h"
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
const wchar_t FSProtocolNames[FSPROTOCOL_COUNT][11] = { L"SCP", L"SFTP (SCP)", L"SFTP", L"", L"", L"FTP" };
const int SshPortNumber = 22;
const int FtpPortNumber = 21;
const int FtpsImplicitPortNumber = 990;
//---------------------------------------------------------------------
TDateTime SecToDateTime(int Sec)
{
  return TDateTime((unsigned short)(Sec/60/60),
    (unsigned short)(Sec/60%60), (unsigned short)(Sec%60), 0);
}
//--- TSessionData ----------------------------------------------------
TSessionData::TSessionData(std::wstring aName):
  TNamedObject(aName)
{
  Default();
  FModified = true;
}
//---------------------------------------------------------------------
void TSessionData::Default()
{
  SetHostName(L"");
  SetPortNumber(SshPortNumber);
  SetUserName(L"");
  SetPassword(L"");
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
  SetHostKey(L"");

  SetProxyMethod(pmNone);
  SetProxyHost(L"proxy");
  SetProxyPort(80);
  SetProxyUsername(L"");
  SetProxyPassword(L"");
  SetProxyTelnetCommand(L"connect %host %port\\n");
  SetProxyLocalCommand(L"");
  SetProxyDNS(asAuto);
  SetProxyLocalhost(false);

  for (int Index = 0; Index < LENOF(FBugs); Index++)
  {
    SetBug((TSshBug)Index, asAuto);
  }

  SetSpecial(false);
  SetFSProtocol(fsSFTP);
  SetAddressFamily(afAuto);
  SetRekeyData(L"1G");
  SetRekeyTime(60);

  // FS common
  SetLocalDirectory(L"");
  SetRemoteDirectory(L"");
  SetUpdateDirectories(false);
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
  SetLookupUserGroups(true);
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
  SetNotUtf(asAuto);
  SetFtpListAll(asAuto);

  // SFTP
  SetSftpServer(L"");
  SetSFTPDownloadQueue(4);
  SetSFTPUploadQueue(4);
  SetSFTPListingQueue(2);
  SetSFTPMaxVersion(5);
  SetSFTPMaxPacketSize(0);

  for (int Index = 0; Index < LENOF(FSFTPBugs); Index++)
  {
    SetSFTPBug((TSftpBug)Index, asAuto);
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

  SetCustomParam1(L"");
  SetCustomParam2(L"");

  FSelected = false;
  FModified = false;
  FSource = ::ssNone;

  // add also to TSessionLog::AddStartupInfo()
}
//---------------------------------------------------------------------
void TSessionData::NonPersistant()
{
  SetUpdateDirectories(false);
  SetPreserveDirectoryChanges(false);
}
//---------------------------------------------------------------------
void TSessionData::Assign(TPersistent * Source)
{
  if (Source && ::InheritsFrom<TPersistent, TSessionData>(Source))
  {
    #define DUPL(P) Set##P(((TSessionData *)Source)->Get##P())
    DUPL(Name);
    DUPL(HostName);
    DUPL(PortNumber);
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
    DUPL(RekeyData);
    DUPL(RekeyTime);
    DUPL(HostKey);

    DUPL(FSProtocol);
    DUPL(LocalDirectory);
    DUPL(RemoteDirectory);
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

    for (int Index = 0; Index < LENOF(FBugs); Index++)
    {
      // DUPL(Bug[(TSshBug)Index]);
      ((TSessionData *)Source)->SetBug((TSshBug)Index, GetBug((TSshBug)Index));
    }

    // SFTP
    DUPL(SftpServer);
    DUPL(SFTPDownloadQueue);
    DUPL(SFTPUploadQueue);
    DUPL(SFTPListingQueue);
    DUPL(SFTPMaxVersion);
    DUPL(SFTPMaxPacketSize);

    for (int Index = 0; Index < LENOF(FSFTPBugs); Index++)
    {
      // DUPL(SFTPBug[(TSftpBug)Index]);
      ((TSessionData *)Source)->SetSFTPBug((TSftpBug)Index, GetSFTPBug((TSftpBug)Index));
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
    DUPL(FtpForcePasvIp);
    DUPL(FtpAccount);
    DUPL(FtpPingInterval);
    DUPL(FtpPingType);
    DUPL(Ftps);

    DUPL(FtpProxyLogonType);

    DUPL(CustomParam1);
    DUPL(CustomParam2);

    #undef DUPL
    FModified = ((TSessionData *)Source)->GetModified();
    FSource = ((TSessionData *)Source)->FSource;
  }
  else
  {
    TNamedObject::Assign(Source);
  }
}
//---------------------------------------------------------------------
void TSessionData::Load(THierarchicalStorage * Storage)
{
  bool RewritePassword = false;
  if (Storage->OpenSubKey(GetInternalStorageKey(), false))
  {
    SetPortNumber(Storage->Readint(L"PortNumber", GetPortNumber()));
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
        FPassword = Storage->ReadString(L"Password", FPassword);
      }
    }
    SetPasswordless(Storage->Readbool(L"Passwordless", GetPasswordless()));
    // Putty uses PingIntervalSecs
    int PingIntervalSecs = Storage->Readint(L"PingIntervalSecs", -1);
    if (PingIntervalSecs < 0)
    {
      PingIntervalSecs = Storage->Readint(L"PingIntervalSec", GetPingInterval()%60);
    }
    SetPingInterval(
      Storage->Readint(L"PingInterval", GetPingInterval()/60)*60 +
      PingIntervalSecs);
    if (GetPingInterval()== 0)
    {
      SetPingInterval(30);
    }
    // PingType has not existed before 3.5, where PingInterval > 0 meant today's ptNullPacket
    // Since 3.5, until 4.1 PingType was stored unconditionally.
    // Since 4.1 PingType is stored when it is not ptOff (default) or
    // when PingInterval is stored.
    if (!Storage->ValueExists(L"PingType"))
    {
      if (Storage->Readint(L"PingInterval", 0) > 0)
      {
        SetPingType(ptNullPacket);
      }
    }
    else
    {
      SetPingType(static_cast<TPingType>(Storage->Readint(L"PingType", ptOff)));
    }
    SetTimeout(Storage->Readint(L"Timeout", GetTimeout()));
    SetTryAgent(Storage->Readbool(L"TryAgent", GetTryAgent()));
    SetAgentFwd(Storage->Readbool(L"AgentFwd", GetAgentFwd()));
    SetAuthTIS(Storage->Readbool(L"AuthTIS", GetAuthTIS()));
    SetAuthKI(Storage->Readbool(L"AuthKI", GetAuthKI()));
    SetAuthKIPassword(Storage->Readbool(L"AuthKIPassword", GetAuthKIPassword()));
    // Continue to use setting keys of previous kerberos implementation (vaclav tomec),
    // but fallback to keys of other implementations (official putty and vintela quest putty),
    // to allow imports from all putty versions.
    // Both vaclav tomec and official putty use AuthGSSAPI
    SetAuthGSSAPI(Storage->Readbool(L"AuthGSSAPI", Storage->Readbool(L"AuthSSPI", GetAuthGSSAPI())));
    SetGSSAPIFwdTGT(Storage->Readbool(L"GSSAPIFwdTGT", Storage->Readbool(L"GssapiFwd", Storage->Readbool(L"SSPIFwdTGT", GetGSSAPIFwdTGT()))));
    SetGSSAPIServerRealm(Storage->ReadString(L"GSSAPIServerRealm", Storage->ReadString(L"KerbPrincipal", GetGSSAPIServerRealm())));
    SetChangeUsername(Storage->Readbool(L"ChangeUsername", GetChangeUsername()));
    SetCompression(Storage->Readbool(L"Compression", GetCompression()));
    SetSshProt((TSshProt)Storage->Readint(L"SshProt", GetSshProt()));
    SetSsh2DES(Storage->Readbool(L"Ssh2DES", GetSsh2DES()));
    SetSshNoUserAuth(Storage->Readbool(L"SshNoUserAuth", GetSshNoUserAuth()));
    SetCipherList(Storage->ReadString(L"Cipher", GetCipherList()));
    SetKexList(Storage->ReadString(L"KEX", GetKexList()));
    SetPublicKeyFile(Storage->ReadString(L"PublicKeyFile", GetPublicKeyFile()));
    SetAddressFamily(static_cast<TAddressFamily>
        (Storage->Readint(L"AddressFamily", GetAddressFamily())));
    SetRekeyData(Storage->ReadString(L"RekeyBytes", GetRekeyData()));
    SetRekeyTime(Storage->Readint(L"RekeyTime", GetRekeyTime()));

    SetFSProtocol((TFSProtocol)Storage->Readint(L"FSProtocol", GetFSProtocol()));
    SetLocalDirectory(Storage->ReadString(L"LocalDirectory", GetLocalDirectory()));
    SetRemoteDirectory(Storage->ReadString(L"RemoteDirectory", GetRemoteDirectory()));
    SetUpdateDirectories(Storage->Readbool(L"UpdateDirectories", GetUpdateDirectories()));
    SetCacheDirectories(Storage->Readbool(L"CacheDirectories", GetCacheDirectories()));
    SetCacheDirectoryChanges(Storage->Readbool(L"CacheDirectoryChanges", GetCacheDirectoryChanges()));
    SetPreserveDirectoryChanges(Storage->Readbool(L"PreserveDirectoryChanges", GetPreserveDirectoryChanges()));

    SetResolveSymlinks(Storage->Readbool(L"ResolveSymlinks", GetResolveSymlinks()));
    SetDSTMode((TDSTMode)Storage->Readint(L"ConsiderDST", GetDSTMode()));
    SetLockInHome(Storage->Readbool(L"LockInHome", GetLockInHome()));
    SetSpecial(Storage->Readbool(L"Special", GetSpecial()));
    SetShell(Storage->ReadString(L"Shell", GetShell()));
    SetClearAliases(Storage->Readbool(L"ClearAliases", GetClearAliases()));
    SetUnsetNationalVars(Storage->Readbool(L"UnsetNationalVars", GetUnsetNationalVars()));
    SetListingCommand(Storage->ReadString(L"ListingCommand",
        Storage->Readbool(L"AliasGroupList", false) ? std::wstring(L"ls -gla") : GetListingCommand()));
    SetIgnoreLsWarnings(Storage->Readbool(L"IgnoreLsWarnings", GetIgnoreLsWarnings()));
    SetSCPLsFullTime(TAutoSwitch(Storage->Readint(L"SCPLsFullTime", GetSCPLsFullTime())));
    SetFtpListAll(TAutoSwitch(Storage->Readint(L"FtpListAll", GetFtpListAll())));
    SetScp1Compatibility(Storage->Readbool(L"Scp1Compatibility", GetScp1Compatibility()));
    SetTimeDifference(TDateTime(Storage->ReadFloat(L"TimeDifference", GetTimeDifference())));
    SetDeleteToRecycleBin(Storage->Readbool(L"DeleteToRecycleBin", GetDeleteToRecycleBin()));
    SetOverwrittenToRecycleBin(Storage->Readbool(L"OverwrittenToRecycleBin", GetOverwrittenToRecycleBin()));
    SetRecycleBinPath(Storage->ReadString(L"RecycleBinPath", GetRecycleBinPath()));
    SetPostLoginCommands(Storage->ReadString(L"PostLoginCommands", GetPostLoginCommands()));

    SetReturnVar(Storage->ReadString(L"ReturnVar", GetReturnVar()));
    SetLookupUserGroups(Storage->Readbool(L"LookupUserGroups", GetLookupUserGroups()));
    SetEOLType((TEOLType)Storage->Readint(L"EOLType", GetEOLType()));
    SetNotUtf(TAutoSwitch(Storage->Readint(L"Utf", Storage->Readint(L"SFTPUtfBug", GetNotUtf()))));

    SetTcpNoDelay(Storage->Readbool(L"TcpNoDelay", GetTcpNoDelay()));

    SetProxyMethod((TProxyMethod)Storage->Readint(L"ProxyMethod", -1));
    if (GetProxyMethod() < 0)
    {
      int ProxyType = Storage->Readint(L"ProxyType", pxNone);
      int ProxySOCKSVersion;
      switch (ProxyType) {
        case pxHTTP:
          SetProxyMethod(pmHTTP);
          break;
        case pxTelnet:
          SetProxyMethod(pmTelnet);
          break;
        case pxSocks:
          ProxySOCKSVersion = Storage->Readint(L"ProxySOCKSVersion", 5);
          SetProxyMethod(ProxySOCKSVersion == 5 ? pmSocks5 : pmSocks4);
          break;
        default:
        case pxNone:
          SetProxyMethod(pmNone);
          break;
      }
    }
    SetProxyHost(Storage->ReadString(L"ProxyHost", GetProxyHost()));
    SetProxyPort(Storage->Readint(L"ProxyPort", GetProxyPort()));
    SetProxyUsername(Storage->ReadString(L"ProxyUsername", GetProxyUsername()));
    if (Storage->ValueExists(L"ProxyPassword"))
    {
      // encrypt unencrypted password
      SetProxyPassword(Storage->ReadString(L"ProxyPassword", L""));
    }
    else
    {
      // load encrypted password
      FProxyPassword = Storage->ReadString(L"ProxyPasswordEnc", FProxyPassword);
    }
    if (GetProxyMethod() == pmCmd)
    {
      SetProxyLocalCommand(Storage->ReadStringRaw(L"ProxyTelnetCommand", GetProxyLocalCommand()));
    }
    else
    {
      SetProxyTelnetCommand(Storage->ReadStringRaw(L"ProxyTelnetCommand", GetProxyTelnetCommand()));
    }
    SetProxyDNS(TAutoSwitch((Storage->Readint(L"ProxyDNS", (GetProxyDNS() + 2) % 3) + 1) % 3));
    SetProxyLocalhost(Storage->Readbool(L"ProxyLocalhost", GetProxyLocalhost()));

    #define READ_BUG(BUG) \
      SetBug(sb##BUG, TAutoSwitch(2 - Storage->Readint(L"Bug" + ::MB2W(#BUG), \
        2 - GetBug(sb##BUG))));
    READ_BUG(Ignore1);
    READ_BUG(PlainPW1);
    READ_BUG(RSA1);
    READ_BUG(HMAC2);
    READ_BUG(DeriveKey2);
    READ_BUG(RSAPad2);
    READ_BUG(Rekey2);
    READ_BUG(PKSessID2);
    READ_BUG(MaxPkt2);
    #undef READ_BUG

    if ((GetBug(sbHMAC2) == asAuto) &&
        Storage->Readbool(L"BuggyMAC", false))
    {
        SetBug(sbHMAC2, asOn);
    }

    SetSftpServer(Storage->ReadString(L"SftpServer", GetSftpServer()));
    #define READ_SFTP_BUG(BUG) \
      SetSFTPBug(sb##BUG, TAutoSwitch(Storage->Readint(L"SFTP" + ::MB2W(#BUG) + L"Bug", GetSFTPBug(sb##BUG))));
    READ_SFTP_BUG(Symlink);
    READ_SFTP_BUG(SignedTS);
    #undef READ_SFTP_BUG

    SetSFTPMaxVersion(Storage->Readint(L"SFTPMaxVersion", GetSFTPMaxVersion()));
    SetSFTPMaxPacketSize(Storage->Readint(L"SFTPMaxPacketSize", GetSFTPMaxPacketSize()));

    SetColor(Storage->Readint(L"Color", GetColor()));

    SetProtocolStr(Storage->ReadString(L"Protocol", GetProtocolStr()));

    SetTunnel(Storage->Readbool(L"Tunnel", GetTunnel()));
    SetTunnelPortNumber(Storage->Readint(L"TunnelPortNumber", GetTunnelPortNumber()));
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
        FTunnelPassword = Storage->ReadString(L"TunnelPassword", FTunnelPassword);
      }
    }
    SetTunnelPublicKeyFile(Storage->ReadString(L"TunnelPublicKeyFile", GetTunnelPublicKeyFile()));
    SetTunnelLocalPortNumber(Storage->Readint(L"TunnelLocalPortNumber", GetTunnelLocalPortNumber()));

    // Ftp prefix
    SetFtpPasvMode(Storage->Readbool(L"FtpPasvMode", GetFtpPasvMode()));
    SetFtpForcePasvIp(Storage->Readbool(L"FtpForcePasvIp", GetFtpForcePasvIp()));
    SetFtpAccount(Storage->ReadString(L"FtpAccount", GetFtpAccount()));
    SetFtpPingInterval(Storage->Readint(L"FtpPingInterval", GetFtpPingInterval()));
    SetFtpPingType(static_cast<TPingType>(Storage->Readint(L"FtpPingType", GetFtpPingType())));
    SetFtps(static_cast<TFtps>(Storage->Readint(L"Ftps", GetFtps())));

    SetFtpProxyLogonType(Storage->Readint(L"FtpProxyLogonType", GetFtpProxyLogonType()));

    SetCustomParam1(Storage->ReadString(L"CustomParam1", GetCustomParam1()));
    SetCustomParam2(Storage->ReadString(L"CustomParam2", GetCustomParam2()));

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
        if (!GetPassword().empty())
        {
          Storage->WriteString(L"Password", FPassword);
        }
        Storage->DeleteValue(L"TunnelPasswordPlain");
        if (!GetTunnelPassword().empty())
        {
          Storage->WriteString(L"TunnelPassword", FTunnelPassword);
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

  FModified = false;
  FSource = ssStored;
}
//---------------------------------------------------------------------
void TSessionData::Save(THierarchicalStorage * Storage,
  bool PuttyExport, const TSessionData * Default)
{
  DEBUG_PRINTF(L"begin");
  if (Storage->OpenSubKey(GetInternalStorageKey(), true))
  {
    #define WRITE_DATA_EX(TYPE, NAME, PROPERTY, CONV) \
      if ((Default != NULL) && (CONV(Default->PROPERTY) == CONV(PROPERTY))) \
      { \
        Storage->DeleteValue(std::wstring(::MB2W(NAME))); \
      } \
      else \
      { \
        Storage->Write##TYPE(std::wstring(::MB2W(NAME)), CONV(PROPERTY)); \
      }
    #define WRITE_DATA_CONV(TYPE, NAME, PROPERTY) WRITE_DATA_EX(TYPE, NAME, PROPERTY, WRITE_DATA_CONV_FUNC)
    #define WRITE_DATA(TYPE, PROPERTY) WRITE_DATA_EX(TYPE, #PROPERTY, PROPERTY, )

    WRITE_DATA(String, GetHostName());
    WRITE_DATA(int, GetPortNumber());
    WRITE_DATA(bool, GetPasswordless());
    WRITE_DATA_EX(int, "PingInterval", GetPingInterval() / 60, );
    WRITE_DATA_EX(int, "PingIntervalSecs", GetPingInterval() % 60, );
    Storage->DeleteValue(L"PingIntervalSec"); // obsolete
    // when PingInterval is stored always store PingType not to attempt to
    // deduce PingType from PingInterval (backward compatibility with pre 3.5)
    if (((Default != NULL) && (GetPingType() != Default->GetPingType())) ||
        Storage->ValueExists(L"PingInterval"))
    {
      Storage->Writeint(L"PingType", GetPingType());
    }
    else
    {
      Storage->DeleteValue(L"PingType");
    }
    WRITE_DATA(int, GetTimeout());
    WRITE_DATA(bool, GetTryAgent());
    WRITE_DATA(bool, GetAgentFwd());
    WRITE_DATA(bool, GetAuthTIS());
    WRITE_DATA(bool, GetAuthKI());
    WRITE_DATA(bool, GetAuthKIPassword());

    WRITE_DATA(bool, GetAuthGSSAPI());
    WRITE_DATA(bool, GetGSSAPIFwdTGT());
    WRITE_DATA(String, GetGSSAPIServerRealm());
    Storage->DeleteValue(L"TryGSSKEX");
    Storage->DeleteValue(L"UserNameFromEnvironment");
    Storage->DeleteValue(L"GSSAPIServerChoosesUserName");
    Storage->DeleteValue(L"GSSAPITrustDNS");
    if (PuttyExport)
    {
      // duplicate kerberos setting with keys of the vintela quest putty
      WRITE_DATA_EX(bool, "AuthSSPI", GetAuthGSSAPI(), );
      WRITE_DATA_EX(bool, "SSPIFwdTGT", GetGSSAPIFwdTGT(), );
      WRITE_DATA_EX(String, "KerbPrincipal", GetGSSAPIServerRealm(), );
      // duplicate kerberos setting with keys of the official putty
      WRITE_DATA_EX(bool, "GssapiFwd", GetGSSAPIFwdTGT(), );
    }

    WRITE_DATA(bool, GetChangeUsername());
    WRITE_DATA(bool, GetCompression());
    WRITE_DATA(int, GetSshProt());
    WRITE_DATA(bool, GetSsh2DES());
    WRITE_DATA(bool, GetSshNoUserAuth());
    WRITE_DATA_EX(String, "Cipher", GetCipherList(), );
    WRITE_DATA_EX(String, "KEX", GetKexList(), );
    WRITE_DATA(int, GetAddressFamily());
    WRITE_DATA_EX(String, "RekeyBytes", GetRekeyData(), );
    WRITE_DATA(int, GetRekeyTime());

    WRITE_DATA(bool, GetTcpNoDelay());

    if (PuttyExport)
    {
      WRITE_DATA(StringRaw, GetUserName());
      WRITE_DATA(StringRaw, GetPublicKeyFile());
    }
    else
    {
      WRITE_DATA(String, GetUserName());
      WRITE_DATA(String, GetPublicKeyFile());
      WRITE_DATA(int, GetFSProtocol());
      WRITE_DATA(String, GetLocalDirectory());
      WRITE_DATA(String, GetRemoteDirectory());
      WRITE_DATA(bool, GetUpdateDirectories());
      WRITE_DATA(bool, GetCacheDirectories());
      WRITE_DATA(bool, GetCacheDirectoryChanges());
      WRITE_DATA(bool, GetPreserveDirectoryChanges());

      WRITE_DATA(bool, GetResolveSymlinks());
      WRITE_DATA_EX(int, "ConsiderDST", GetDSTMode(), );
      WRITE_DATA(bool, GetLockInHome());
      // Special is never stored (if it would, login dialog must be modified not to
      // duplicate Special parameter when Special session is loaded and then stored
      // under different name)
      // WRITE_DATA(bool, GetSpecial());
      WRITE_DATA(String, GetShell());
      WRITE_DATA(bool, GetClearAliases());
      WRITE_DATA(bool, GetUnsetNationalVars());
      WRITE_DATA(String, GetListingCommand());
      WRITE_DATA(bool, GetIgnoreLsWarnings());
      WRITE_DATA(int, GetSCPLsFullTime());
      WRITE_DATA(int, GetFtpListAll());
      WRITE_DATA(bool, GetScp1Compatibility());
      WRITE_DATA(Float, GetTimeDifference());
      WRITE_DATA(bool, GetDeleteToRecycleBin());
      WRITE_DATA(bool, GetOverwrittenToRecycleBin());
      WRITE_DATA(String, GetRecycleBinPath());
      WRITE_DATA(String, GetPostLoginCommands());

      WRITE_DATA(String, GetReturnVar());
      WRITE_DATA(bool, GetLookupUserGroups());
      WRITE_DATA(int, GetEOLType());
      Storage->DeleteValue(L"SFTPUtfBug");
      WRITE_DATA_EX(int, "Utf", GetNotUtf(), );
    }

    WRITE_DATA(int, GetProxyMethod());
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
        case pmNone:
          ProxyType = pxNone;
          break;
      }
      Storage->Writeint(L"ProxyType", ProxyType);
      Storage->Writeint(L"ProxySOCKSVersion", ProxySOCKSVersion);
    }
    else
    {
      Storage->DeleteValue(L"ProxyType");
      Storage->DeleteValue(L"ProxySOCKSVersion");
    }
    WRITE_DATA(String, GetProxyHost());
    WRITE_DATA(int, GetProxyPort());
    WRITE_DATA(String, GetProxyUsername());
    if (GetProxyMethod() == pmCmd)
    {
      WRITE_DATA_EX(StringRaw, "ProxyTelnetCommand", GetProxyLocalCommand(), );
    }
    else
    {
      WRITE_DATA(StringRaw, GetProxyTelnetCommand());
    }
    #define WRITE_DATA_CONV_FUNC(X) (((X) + 2) % 3)
    WRITE_DATA_CONV(int, "ProxyDNS", GetProxyDNS());
    #undef WRITE_DATA_CONV_FUNC
    WRITE_DATA(bool, GetProxyLocalhost());

    #define WRITE_DATA_CONV_FUNC(X) (2 - (X))
    // #define WRITE_BUG(BUG) WRITE_DATA_CONV(int, L"Bug" + std::wstring(::MB2W("##BUG")), GetBug(sb##BUG));
    #define WRITE_BUG(BUG) WRITE_DATA_CONV(int, L"Bug" + ::MB2W(#BUG), GetBug(sb##BUG));
    WRITE_BUG(Ignore1);
    WRITE_BUG(PlainPW1);
    WRITE_BUG(RSA1);
    WRITE_BUG(HMAC2);
    WRITE_BUG(DeriveKey2);
    WRITE_BUG(RSAPad2);
    WRITE_BUG(Rekey2);
    WRITE_BUG(PKSessID2);
    WRITE_BUG(MaxPkt2);
    #undef WRITE_BUG
    #undef WRITE_DATA_CONV_FUNC

    Storage->DeleteValue(L"BuggyMAC");
    Storage->DeleteValue(L"AliasGroupList");

    if (PuttyExport)
    {
      WRITE_DATA_EX(String, "Protocol", GetProtocolStr(), );
    }

    if (!PuttyExport)
    {
      WRITE_DATA(String, GetSftpServer());

      #define WRITE_SFTP_BUG(BUG) WRITE_DATA_EX(int, "SFTP" #BUG "Bug", GetSFTPBug(sb##BUG), );
      WRITE_SFTP_BUG(Symlink);
      WRITE_SFTP_BUG(SignedTS);
      #undef WRITE_SFTP_BUG

      WRITE_DATA(int, GetSFTPMaxVersion());
      WRITE_DATA(int, GetSFTPMaxPacketSize());

      WRITE_DATA(int, GetColor());

      WRITE_DATA(bool, GetTunnel());
      WRITE_DATA(String, GetTunnelHostName());
      WRITE_DATA(int, GetTunnelPortNumber());
      WRITE_DATA(String, GetTunnelUserName());
      WRITE_DATA(String, GetTunnelPublicKeyFile());
      WRITE_DATA(int, GetTunnelLocalPortNumber());

      WRITE_DATA(bool, GetFtpPasvMode());
      WRITE_DATA(bool, GetFtpForcePasvIp());
      WRITE_DATA(String, GetFtpAccount());
      WRITE_DATA(int, GetFtpPingInterval());
      WRITE_DATA(int, GetFtpPingType());
      WRITE_DATA(int, GetFtps());

      WRITE_DATA(int, GetFtpProxyLogonType());

      WRITE_DATA(String, GetCustomParam1());
      WRITE_DATA(String, GetCustomParam2());
    }

    SavePasswords(Storage, PuttyExport);

    Storage->CloseSubKey();
  }
  DEBUG_PRINTF(L"end");
}
//---------------------------------------------------------------------
void TSessionData::SavePasswords(THierarchicalStorage * Storage, bool PuttyExport)
{
  if (!Configuration->GetDisablePasswordStoring() && !PuttyExport && !FPassword.empty())
  {
    Storage->WriteString(L"Password", StronglyRecryptPassword(FPassword, GetUserName() + GetHostName()));
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
    if (!FProxyPassword.empty())
    {
      Storage->WriteString(L"ProxyPasswordEnc", StronglyRecryptPassword(FProxyPassword, GetProxyUsername() + GetProxyHost()));
    }
    else
    {
      Storage->DeleteValue(L"ProxyPasswordEnc");
    }
    Storage->DeleteValue(L"ProxyPassword");

    if (!Configuration->GetDisablePasswordStoring() && !FTunnelPassword.empty())
    {
      Storage->WriteString(L"TunnelPassword", StronglyRecryptPassword(FTunnelPassword, GetTunnelUserName() + GetTunnelHostName()));
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
bool TSessionData::HasAnyPassword()
{
  return !FPassword.empty() || !FProxyPassword.empty() || !FTunnelPassword.empty();
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
std::wstring TSessionData::GetSource()
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
void TSessionData::SaveRecryptedPasswords(THierarchicalStorage * Storage)
{
  if (Storage->OpenSubKey(GetInternalStorageKey(), true))
  {
    RecryptPasswords();

    SavePasswords(Storage, false);

    Storage->CloseSubKey();
  }
}
//---------------------------------------------------------------------
void TSessionData::Remove()
{
  THierarchicalStorage * Storage = Configuration->CreateScpStorage(true);
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
}
//---------------------------------------------------------------------
bool TSessionData::ParseUrl(std::wstring Url, TOptions * Options,
  TStoredSessionList * StoredSessions, bool & DefaultsOnly, std::wstring * FileName,
  bool * AProtocolDefined)
{
  bool ProtocolDefined = false;
  bool PortNumberDefined = false;
  TFSProtocol AFSProtocol;
  int APortNumber;
  TFtps AFtps = ftpsNone;
  if (LowerCase(Url.substr(0, 4)) == L"scp:")
  {
    AFSProtocol = fsSCPonly;
    APortNumber = SshPortNumber;
    Url.erase(1, 4);
    ProtocolDefined = true;
  }
  else if (LowerCase(Url.substr(0, 5)) == L"sftp:")
  {
    AFSProtocol = fsSFTPonly;
    APortNumber = SshPortNumber;
    Url.erase(1, 5);
    ProtocolDefined = true;
  }
  else if (LowerCase(Url.substr(0, 4)) == L"ftp:")
  {
    AFSProtocol = fsFTP;
    SetFtps(ftpsNone);
    APortNumber = FtpPortNumber;
    Url.erase(1, 4);
    ProtocolDefined = true;
  }
  else if (LowerCase(Url.substr(0, 5)) == L"ftps:")
  {
    AFSProtocol = fsFTP;
    AFtps = ftpsImplicit;
    APortNumber = FtpsImplicitPortNumber;
    Url.erase(1, 5);
    ProtocolDefined = true;
  }

  if (ProtocolDefined && (Url.substr(0, 2) == L"//"))
  {
    Url.erase(1, 2);
  }

  if (AProtocolDefined != NULL)
  {
    *AProtocolDefined = ProtocolDefined;
  }

  if (!Url.empty())
  {
    std::wstring DecodedUrl = DecodeUrlChars(Url);
    // lookup stored session even if protocol was defined
    // (this allows setting for example default username for host
    // by creating stored session named by host)
    TSessionData * Data = NULL;
    for (int Index = 0; Index < StoredSessions->GetCount() + StoredSessions->GetHiddenCount(); Index++)
    {
      TSessionData * AData = (TSessionData *)StoredSessions->GetItem(Index);
      if (AnsiSameText(AData->Name, DecodedUrl) ||
          AnsiSameText(AData->Name + L"/", DecodedUrl.substr(0, AData->Name.size() + 1)))
      {
        Data = AData;
        break;
      }
    }

    std::wstring ARemoteDirectory;

    if (Data != NULL)
    {
      DefaultsOnly = false;
      Assign(Data);
      int P = 1;
      while (!AnsiSameText(DecodeUrlChars(Url.substr(0, P)), Data->Name))
      {
        P++;
        assert(P <= Url.size());
      }
      ARemoteDirectory = Url.substr(P + 1, Url.size() - P);

      if (StoredSessions->IsHidden(Data))
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
      Name = L"";

      int PSlash = Url.find_first_of(L"/");
      if (PSlash == 0)
      {
        PSlash = Url.size() + 1;
      }

      std::wstring ConnectInfo = Url.substr(0, PSlash - 1);

      int P = ::LastDelimiter(ConnectInfo, L"@");

      std::wstring UserInfo;
      std::wstring HostInfo;

      if (P > 0)
      {
        UserInfo = ConnectInfo.substr(0, P - 1);
        HostInfo = ConnectInfo.substr(P, ConnectInfo.size() - P);
      }
      else
      {
        HostInfo = ConnectInfo;
      }

      if ((HostInfo.size() >= 2) && (HostInfo[0] == '[') && ((P = HostInfo.find_first_of(L"]")) > 0))
      {
        SetHostName(HostInfo.substr(1, P - 2));
        HostInfo.erase(1, P);
        if (!HostInfo.empty() && (HostInfo[0] == ':'))
        {
          HostInfo.erase(0, 1);
        }
      }
      else
      {
        SetHostName(DecodeUrlChars(CutToChar(HostInfo, ':', true)));
      }

      // expanded from ?: operator, as it caused strange "access violation" errors
      if (!HostInfo.empty())
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

      bool PasswordSeparator = (UserInfo.find_first_of(':') != 0);
      SetUserName(DecodeUrlChars(CutToChar(UserInfo, ':', false)));
      SetPassword(DecodeUrlChars(UserInfo));
      SetPasswordless(GetPassword().empty() && PasswordSeparator);

      ARemoteDirectory = Url.substr(PSlash, Url.size() - PSlash + 1);
    }

    if (!ARemoteDirectory.empty() && (ARemoteDirectory != L"/"))
    {
      if ((ARemoteDirectory[ARemoteDirectory.size() - 1] != '/') &&
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
    // we deliberatelly do keep defaultonly to false, in presence of any option,
    // as the option should not make session "connectable"

    std::wstring Value;
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
    if (Options->FindSwitch(L"passive", Value))
    {
      SetFtpPasvMode((StrToIntDef(Value, 1) != 0));
    }
    if (Options->FindSwitch(L"implicit", Value))
    {
      bool Enabled = (StrToIntDef(Value, 1) != 0);
      SetFtps(Enabled ? ftpsImplicit : ftpsNone);
      if (!PortNumberDefined && Enabled)
      {
        SetPortNumber(FtpsImplicitPortNumber);
      }
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
    if (Options->FindSwitch(L"explicittls", Value))
    {
      bool Enabled = (StrToIntDef(Value, 1) != 0);
      SetFtps(Enabled ? ftpsExplicitTls : ftpsNone);
      if (!PortNumberDefined && Enabled)
      {
        SetPortNumber(FtpPortNumber);
      }
    }
  }

  return true;
}
//---------------------------------------------------------------------
void TSessionData::ConfigureTunnel(int APortNumber)
{
  FOrigHostName = GetHostName();
  FOrigPortNumber = GetPortNumber();
  FOrigProxyMethod = GetProxyMethod();

  SetHostName(L"127.0.0.1");
  SetPortNumber(APortNumber);
  // proxy settings is used for tunnel
  SetProxyMethod(pmNone);
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
  SetPublicKeyFile(::ExpandEnvironmentVariables(GetPublicKeyFile()));
}
//---------------------------------------------------------------------
void TSessionData::ValidatePath(const std::wstring Path)
{
  // noop
}
//---------------------------------------------------------------------
void TSessionData::ValidateName(const std::wstring Name)
{
  if (::LastDelimiter(Name, L"/") > 0)
  {
    throw ExtException(FMTLOAD(ITEM_NAME_INVALID, Name.c_str(), L"/"));
  }
}
//---------------------------------------------------------------------
std::wstring TSessionData::EncryptPassword(const std::wstring & Password, std::wstring Key)
{
  return Configuration->EncryptPassword(Password, Key);
}
//---------------------------------------------------------------------
std::wstring TSessionData::StronglyRecryptPassword(const std::wstring & Password, std::wstring Key)
{
  return Configuration->StronglyRecryptPassword(Password, Key);
}
//---------------------------------------------------------------------
std::wstring TSessionData::DecryptPassword(const std::wstring & Password, std::wstring Key)
{
  std::wstring Result;
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
bool TSessionData::GetCanLogin()
{
  return !FHostName.empty();
}
//---------------------------------------------------------------------------
std::wstring TSessionData::GetSessionKey()
{
  return FORMAT(L"%s@%s", GetUserName(), GetHostName());
}
//---------------------------------------------------------------------
std::wstring TSessionData::GetInternalStorageKey()
{
  if (Name.empty())
  {
    return GetSessionKey();
  }
  else
  {
    return Name;
  }
}
//---------------------------------------------------------------------
std::wstring TSessionData::GetStorageKey()
{
  return GetSessionName();
}
//---------------------------------------------------------------------
void TSessionData::SetHostName(std::wstring value)
{
  if (FHostName != value)
  {
    // HostName is key for password encryption
    std::wstring XPassword = GetPassword();

    int P = ::LastDelimiter(value, L"@");
    if (P > 0)
    {
      SetUserName(value.substr(0, P - 1));
      value = value.substr(P, value.size() - P);
    }
    FHostName = value;
    Modify();

    SetPassword(XPassword);
    if (!XPassword.empty())
    {
      ::Error(SNotImplemented, 239); 
      // FIXME ::Unique(XPassword());
      memset((void *)XPassword.c_str(), 0, XPassword.size());
    }
  }
}
//---------------------------------------------------------------------
void TSessionData::SetPortNumber(int value)
{
  SET_SESSION_PROPERTY(PortNumber);
}
//---------------------------------------------------------------------------
void TSessionData::SetShell(std::wstring value)
{
  SET_SESSION_PROPERTY(Shell);
}
//---------------------------------------------------------------------------
void TSessionData::SetSftpServer(std::wstring value)
{
  SET_SESSION_PROPERTY(SftpServer);
}
//---------------------------------------------------------------------------
void TSessionData::SetClearAliases(bool value)
{
  SET_SESSION_PROPERTY(ClearAliases);
}
//---------------------------------------------------------------------------
void TSessionData::SetListingCommand(std::wstring value)
{
  SET_SESSION_PROPERTY(ListingCommand);
}
//---------------------------------------------------------------------------
void TSessionData::SetIgnoreLsWarnings(bool value)
{
  SET_SESSION_PROPERTY(IgnoreLsWarnings);
}
//---------------------------------------------------------------------------
void TSessionData::SetUnsetNationalVars(bool value)
{
  SET_SESSION_PROPERTY(UnsetNationalVars);
}
//---------------------------------------------------------------------
void TSessionData::SetUserName(std::wstring value)
{
  // UserName is key for password encryption
  std::wstring XPassword = GetPassword();
  SET_SESSION_PROPERTY(UserName);
  SetPassword(XPassword);
  if (!XPassword.empty())
  {
    // FIXME ::Unique(XPassword);
    memset((void *)XPassword.c_str(), 0, XPassword.size());
  }
}
//---------------------------------------------------------------------
void TSessionData::SetPassword(std::wstring value)
{
  if (!value.empty())
  {
    SetPasswordless(false);
  }
  value = EncryptPassword(value, GetUserName() + GetHostName());
  SET_SESSION_PROPERTY(Password);
}
//---------------------------------------------------------------------
std::wstring TSessionData::GetPassword()
{
  return DecryptPassword(FPassword, GetUserName() + GetHostName());
}
//---------------------------------------------------------------------
void TSessionData::SetPasswordless(bool value)
{
  SET_SESSION_PROPERTY(Passwordless);
}
//---------------------------------------------------------------------
void TSessionData::SetPingInterval(int value)
{
  SET_SESSION_PROPERTY(PingInterval);
}
//---------------------------------------------------------------------
void TSessionData::SetTryAgent(bool value)
{
  SET_SESSION_PROPERTY(TryAgent);
}
//---------------------------------------------------------------------
void TSessionData::SetAgentFwd(bool value)
{
  SET_SESSION_PROPERTY(AgentFwd);
}
//---------------------------------------------------------------------
void TSessionData::SetAuthTIS(bool value)
{
  SET_SESSION_PROPERTY(AuthTIS);
}
//---------------------------------------------------------------------
void TSessionData::SetAuthKI(bool value)
{
  SET_SESSION_PROPERTY(AuthKI);
}
//---------------------------------------------------------------------
void TSessionData::SetAuthKIPassword(bool value)
{
  SET_SESSION_PROPERTY(AuthKIPassword);
}
//---------------------------------------------------------------------
void TSessionData::SetAuthGSSAPI(bool value)
{
  SET_SESSION_PROPERTY(AuthGSSAPI);
}
//---------------------------------------------------------------------
void TSessionData::SetGSSAPIFwdTGT(bool value)
{
  SET_SESSION_PROPERTY(GSSAPIFwdTGT);
}
//---------------------------------------------------------------------
void TSessionData::SetGSSAPIServerRealm(std::wstring value)
{
  SET_SESSION_PROPERTY(GSSAPIServerRealm);
}
//---------------------------------------------------------------------
void TSessionData::SetChangeUsername(bool value)
{
  SET_SESSION_PROPERTY(ChangeUsername);
}
//---------------------------------------------------------------------
void TSessionData::SetCompression(bool value)
{
  SET_SESSION_PROPERTY(Compression);
}
//---------------------------------------------------------------------
void TSessionData::SetSshProt(TSshProt value)
{
  SET_SESSION_PROPERTY(SshProt);
}
//---------------------------------------------------------------------
void TSessionData::SetSsh2DES(bool value)
{
  SET_SESSION_PROPERTY(Ssh2DES);
}
//---------------------------------------------------------------------
void TSessionData::SetSshNoUserAuth(bool value)
{
  SET_SESSION_PROPERTY(SshNoUserAuth);
}
//---------------------------------------------------------------------
std::wstring TSessionData::GetSshProtStr()
{
  return SshProtList[FSshProt];
}
//---------------------------------------------------------------------
bool TSessionData::GetUsesSsh()
{
  return (GetFSProtocol() != fsFTP);
}
//---------------------------------------------------------------------
void TSessionData::SetCipher(int Index, TCipher value)
{
  assert(Index >= 0 && Index < CIPHER_COUNT);
  SET_SESSION_PROPERTY(Ciphers[Index]);
}
//---------------------------------------------------------------------
TCipher TSessionData::GetCipher(int Index) const
{
  assert(Index >= 0 && Index < CIPHER_COUNT);
  return FCiphers[Index];
}
//---------------------------------------------------------------------
void TSessionData::SetCipherList(std::wstring value)
{
  bool Used[CIPHER_COUNT];
  for (int C = 0; C < CIPHER_COUNT; C++) Used[C] = false;

  std::wstring CipherStr;
  int Index = 0;
  while (!value.empty() && (Index < CIPHER_COUNT))
  {
    CipherStr = CutToChar(value, ',', true);
    for (int C = 0; C < CIPHER_COUNT; C++)
    {
      if (!::AnsiCompareIC(CipherStr, CipherNames[C]))
      {
        SetCipher(Index, (TCipher)C);
        Used[C] = true;
        Index++;
        break;
      }
    }
  }

  for (int C = 0; C < CIPHER_COUNT && Index < CIPHER_COUNT; C++)
  {
    if (!Used[DefaultCipherList[C]]) SetCipher(Index++, DefaultCipherList[C]);
  }
}
//---------------------------------------------------------------------
std::wstring TSessionData::GetCipherList() const
{
  std::wstring Result;
  for (int Index = 0; Index < CIPHER_COUNT; Index++)
  {
    Result += std::wstring(Index ? L"," : L"") + CipherNames[GetCipher(Index)];
  }
  return Result;
}
//---------------------------------------------------------------------
void TSessionData::SetKex(int Index, TKex value)
{
  assert(Index >= 0 && Index < KEX_COUNT);
  SET_SESSION_PROPERTY(Kex[Index]);
}
//---------------------------------------------------------------------
TKex TSessionData::GetKex(int Index) const
{
  assert(Index >= 0 && Index < KEX_COUNT);
  return FKex[Index];
}
//---------------------------------------------------------------------
void TSessionData::SetKexList(std::wstring value)
{
  bool Used[KEX_COUNT];
  for (int K = 0; K < KEX_COUNT; K++) Used[K] = false;

  std::wstring KexStr;
  int Index = 0;
  while (!value.empty() && (Index < KEX_COUNT))
  {
    KexStr = CutToChar(value, ',', true);
    for (int K = 0; K < KEX_COUNT; K++)
    {
      if (!::AnsiCompareIC(KexStr, KexNames[K]))
      {
        SetKex(Index, (TKex)K);
        Used[K] = true;
        Index++;
        break;
      }
    }
  }

  for (int K = 0; K < KEX_COUNT && Index < KEX_COUNT; K++)
  {
    if (!Used[DefaultKexList[K]]) SetKex(Index++, DefaultKexList[K]);
  }
}
//---------------------------------------------------------------------
std::wstring TSessionData::GetKexList() const
{
  std::wstring Result;
  for (int Index = 0; Index < KEX_COUNT; Index++)
  {
    Result += std::wstring(Index ? L"," : L"") + KexNames[GetKex(Index)];
  }
  return Result;
}
//---------------------------------------------------------------------
void TSessionData::SetPublicKeyFile(std::wstring value)
{
  if (FPublicKeyFile != value)
  {
    FPublicKeyFile = StripPathQuotes(value);
    Modify();
  }
}
//---------------------------------------------------------------------
void TSessionData::SetReturnVar(std::wstring value)
{
  SET_SESSION_PROPERTY(ReturnVar);
}
//---------------------------------------------------------------------------
void TSessionData::SetLookupUserGroups(bool value)
{
  SET_SESSION_PROPERTY(LookupUserGroups);
}
//---------------------------------------------------------------------------
void TSessionData::SetEOLType(TEOLType value)
{
  SET_SESSION_PROPERTY(EOLType);
}
//---------------------------------------------------------------------------
TDateTime TSessionData::GetTimeoutDT()
{
  return SecToDateTime(GetTimeout());
}
//---------------------------------------------------------------------------
void TSessionData::SetTimeout(int value)
{
  SET_SESSION_PROPERTY(Timeout);
}
//---------------------------------------------------------------------------
void TSessionData::SetProtocol(TProtocol value)
{
  SET_SESSION_PROPERTY(Protocol);
}
//---------------------------------------------------------------------------
void TSessionData::SetFSProtocol(TFSProtocol value)
{
  SET_SESSION_PROPERTY(FSProtocol);
}
//---------------------------------------------------------------------
std::wstring TSessionData::GetFSProtocolStr()
{
  assert(GetFSProtocol() >= 0 && GetFSProtocol() < FSPROTOCOL_COUNT);
  return FSProtocolNames[GetFSProtocol()];
}
//---------------------------------------------------------------------------
void TSessionData::SetDetectReturnVar(bool value)
{
  if (value != GetDetectReturnVar())
  {
    SetReturnVar(value ? L"" : L"$?");
  }
}
//---------------------------------------------------------------------------
bool TSessionData::GetDetectReturnVar()
{
  return GetReturnVar().empty();
}
//---------------------------------------------------------------------------
void TSessionData::SetDefaultShell(bool value)
{
  if (value != GetDefaultShell())
  {
    SetShell(value ? L"" : L"/bin/bash");
  }
}
//---------------------------------------------------------------------------
bool TSessionData::GetDefaultShell()
{
  return GetShell().empty();
}
//---------------------------------------------------------------------------
void TSessionData::SetProtocolStr(std::wstring value)
{
  FProtocol = ptRaw;
  for (int Index = 0; Index < PROTOCOL_COUNT; Index++)
  {
    if (::AnsiCompareIC(value, ProtocolNames[Index]) == 0)
    {
      FProtocol = TProtocol(Index);
      break;
    }
  }
}
//---------------------------------------------------------------------
std::wstring TSessionData::GetProtocolStr() const
{
  return ProtocolNames[GetProtocol()];
}
//---------------------------------------------------------------------
void TSessionData::SetPingIntervalDT(TDateTime value)
{
  unsigned int hour, min, sec, msec;

  value.DecodeTime(hour, min, sec, msec);
  SetPingInterval(((int)hour)*60*60 + ((int)min)*60 + sec);
}
//---------------------------------------------------------------------------
TDateTime TSessionData::GetPingIntervalDT()
{
  return SecToDateTime(GetPingInterval());
}
//---------------------------------------------------------------------------
void TSessionData::SetPingType(TPingType value)
{
  SET_SESSION_PROPERTY(PingType);
}
//---------------------------------------------------------------------------
void TSessionData::SetAddressFamily(TAddressFamily value)
{
  SET_SESSION_PROPERTY(AddressFamily);
}
//---------------------------------------------------------------------------
void TSessionData::SetRekeyData(std::wstring value)
{
  SET_SESSION_PROPERTY(RekeyData);
}
//---------------------------------------------------------------------------
void TSessionData::SetRekeyTime(unsigned int value)
{
  SET_SESSION_PROPERTY(RekeyTime);
}
//---------------------------------------------------------------------
std::wstring TSessionData::GetDefaultSessionName()
{
  if (!GetHostName().empty() && !GetUserName().empty())
  {
    return FORMAT(L"%s@%s", GetUserName().c_str(), GetHostName().c_str());
  }
  else if (!GetHostName().empty())
  {
    return GetHostName();
  }
  else
  {
    return L"session";
  }
}
//---------------------------------------------------------------------
std::wstring TSessionData::GetSessionName()
{
  if (!Name.empty() && !TNamedObjectList::IsHidden(this) &&
      (Name != DefaultName))
  {
    return Name;
  }
  else
  {
    return GetDefaultSessionName();
  }
}
//---------------------------------------------------------------------
std::wstring TSessionData::GetSessionUrl()
{
  std::wstring Url;
  if (!Name.empty() && !TNamedObjectList::IsHidden(this) &&
      (Name != DefaultName))
  {
    Url = Name;
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
    }

    if (!GetHostName().empty() && !GetUserName().empty())
    {
      Url += FORMAT(L"%s@%s", GetUserName().c_str(), GetHostName().c_str());
    }
    else if (!GetHostName().empty())
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
void TSessionData::SetTimeDifference(TDateTime value)
{
  SET_SESSION_PROPERTY(TimeDifference);
}
//---------------------------------------------------------------------
void TSessionData::SetLocalDirectory(std::wstring value)
{
  SET_SESSION_PROPERTY(LocalDirectory);
}
//---------------------------------------------------------------------
void TSessionData::SetRemoteDirectory(std::wstring value)
{
  SET_SESSION_PROPERTY(RemoteDirectory);
}
//---------------------------------------------------------------------
void TSessionData::SetUpdateDirectories(bool value)
{
  SET_SESSION_PROPERTY(UpdateDirectories);
}
//---------------------------------------------------------------------
void TSessionData::SetCacheDirectories(bool value)
{
  SET_SESSION_PROPERTY(CacheDirectories);
}
//---------------------------------------------------------------------
void TSessionData::SetCacheDirectoryChanges(bool value)
{
  SET_SESSION_PROPERTY(CacheDirectoryChanges);
}
//---------------------------------------------------------------------
void TSessionData::SetPreserveDirectoryChanges(bool value)
{
  SET_SESSION_PROPERTY(PreserveDirectoryChanges);
}
//---------------------------------------------------------------------
void TSessionData::SetResolveSymlinks(bool value)
{
  SET_SESSION_PROPERTY(ResolveSymlinks);
}
//---------------------------------------------------------------------------
void TSessionData::SetDSTMode(TDSTMode value)
{
  SET_SESSION_PROPERTY(DSTMode);
}
//---------------------------------------------------------------------------
void TSessionData::SetDeleteToRecycleBin(bool value)
{
  SET_SESSION_PROPERTY(DeleteToRecycleBin);
}
//---------------------------------------------------------------------------
void TSessionData::SetOverwrittenToRecycleBin(bool value)
{
  SET_SESSION_PROPERTY(OverwrittenToRecycleBin);
}
//---------------------------------------------------------------------------
void TSessionData::SetRecycleBinPath(std::wstring value)
{
  SET_SESSION_PROPERTY(RecycleBinPath);
}
//---------------------------------------------------------------------------
void TSessionData::SetPostLoginCommands(std::wstring value)
{
  SET_SESSION_PROPERTY(PostLoginCommands);
}
//---------------------------------------------------------------------
void TSessionData::SetLockInHome(bool value)
{
  SET_SESSION_PROPERTY(LockInHome);
}
//---------------------------------------------------------------------
void TSessionData::SetSpecial(bool value)
{
  SET_SESSION_PROPERTY(Special);
}
//---------------------------------------------------------------------------
void TSessionData::SetScp1Compatibility(bool value)
{
  SET_SESSION_PROPERTY(Scp1Compatibility);
}
//---------------------------------------------------------------------
void TSessionData::SetTcpNoDelay(bool value)
{
  SET_SESSION_PROPERTY(TcpNoDelay);
}
//---------------------------------------------------------------------
void TSessionData::SetProxyMethod(TProxyMethod value)
{
  SET_SESSION_PROPERTY(ProxyMethod);
}
//---------------------------------------------------------------------
void TSessionData::SetProxyHost(std::wstring value)
{
  SET_SESSION_PROPERTY(ProxyHost);
}
//---------------------------------------------------------------------
void TSessionData::SetProxyPort(int value)
{
  SET_SESSION_PROPERTY(ProxyPort);
}
//---------------------------------------------------------------------
void TSessionData::SetProxyUsername(std::wstring value)
{
  SET_SESSION_PROPERTY(ProxyUsername);
}
//---------------------------------------------------------------------
void TSessionData::SetProxyPassword(std::wstring value)
{
  value = EncryptPassword(value, GetProxyUsername() + GetProxyHost());
  SET_SESSION_PROPERTY(ProxyPassword);
}
//---------------------------------------------------------------------
std::wstring TSessionData::GetProxyPassword() const
{
  return DecryptPassword(FProxyPassword, GetProxyUsername() + GetProxyHost());
}
//---------------------------------------------------------------------
void TSessionData::SetProxyTelnetCommand(std::wstring value)
{
  SET_SESSION_PROPERTY(ProxyTelnetCommand);
}
//---------------------------------------------------------------------
void TSessionData::SetProxyLocalCommand(std::wstring value)
{
  SET_SESSION_PROPERTY(ProxyLocalCommand);
}
//---------------------------------------------------------------------
void TSessionData::SetProxyDNS(TAutoSwitch value)
{
  SET_SESSION_PROPERTY(ProxyDNS);
}
//---------------------------------------------------------------------
void TSessionData::SetProxyLocalhost(bool value)
{
  SET_SESSION_PROPERTY(ProxyLocalhost);
}
//---------------------------------------------------------------------
void TSessionData::SetFtpProxyLogonType(int value)
{
  SET_SESSION_PROPERTY(FtpProxyLogonType);
}
//---------------------------------------------------------------------
void TSessionData::SetBug(TSshBug Bug, TAutoSwitch value)
{
  assert(Bug >= 0 && Bug < LENOF(FBugs));
  SET_SESSION_PROPERTY(Bugs[Bug]);
}
//---------------------------------------------------------------------
TAutoSwitch TSessionData::GetBug(TSshBug Bug) const
{
  assert(Bug >= 0 && Bug < LENOF(FBugs));
  return FBugs[Bug];
}
//---------------------------------------------------------------------
void TSessionData::SetCustomParam1(std::wstring value)
{
  SET_SESSION_PROPERTY(CustomParam1);
}
//---------------------------------------------------------------------
void TSessionData::SetCustomParam2(std::wstring value)
{
  SET_SESSION_PROPERTY(CustomParam2);
}
//---------------------------------------------------------------------
void TSessionData::SetSFTPDownloadQueue(int value)
{
  SET_SESSION_PROPERTY(SFTPDownloadQueue);
}
//---------------------------------------------------------------------
void TSessionData::SetSFTPUploadQueue(int value)
{
  SET_SESSION_PROPERTY(SFTPUploadQueue);
}
//---------------------------------------------------------------------
void TSessionData::SetSFTPListingQueue(int value)
{
  SET_SESSION_PROPERTY(SFTPListingQueue);
}
//---------------------------------------------------------------------
void TSessionData::SetSFTPMaxVersion(int value)
{
  SET_SESSION_PROPERTY(SFTPMaxVersion);
}
//---------------------------------------------------------------------
void TSessionData::SetSFTPMaxPacketSize(unsigned long value)
{
  SET_SESSION_PROPERTY(SFTPMaxPacketSize);
}
//---------------------------------------------------------------------
void TSessionData::SetSFTPBug(TSftpBug Bug, TAutoSwitch value)
{
  assert(Bug >= 0 && Bug < LENOF(FSFTPBugs));
  SET_SESSION_PROPERTY(SFTPBugs[Bug]);
}
//---------------------------------------------------------------------
TAutoSwitch TSessionData::GetSFTPBug(TSftpBug Bug) const
{
  assert(Bug >= 0 && Bug < LENOF(FSFTPBugs));
  return FSFTPBugs[Bug];
}
//---------------------------------------------------------------------
void TSessionData::SetSCPLsFullTime(TAutoSwitch value)
{
  SET_SESSION_PROPERTY(SCPLsFullTime);
}
//---------------------------------------------------------------------
void TSessionData::SetFtpListAll(TAutoSwitch value)
{
  SET_SESSION_PROPERTY(FtpListAll);
}
//---------------------------------------------------------------------------
void TSessionData::SetColor(int value)
{
  SET_SESSION_PROPERTY(Color);
}
//---------------------------------------------------------------------------
void TSessionData::SetTunnel(bool value)
{
  SET_SESSION_PROPERTY(Tunnel);
}
//---------------------------------------------------------------------
void TSessionData::SetTunnelHostName(std::wstring value)
{
  if (FTunnelHostName != value)
  {
    // HostName is key for password encryption
    std::wstring XTunnelPassword = GetTunnelPassword();

    int P = ::LastDelimiter(value, L"@");
    if (P > 0)
    {
      SetTunnelUserName(value.substr(0, P - 1));
      value = value.substr(P, value.size() - P);
    }
    FTunnelHostName = value;
    Modify();

    SetTunnelPassword(XTunnelPassword);
    if (!XTunnelPassword.empty())
    {
      ::Error(SNotImplemented, 241); 
      // FIXME ::Unique(XTunnelPassword);
      memset((void *)XTunnelPassword.c_str(), 0, XTunnelPassword.size());
    }
  }
}
//---------------------------------------------------------------------
void TSessionData::SetTunnelPortNumber(int value)
{
  SET_SESSION_PROPERTY(TunnelPortNumber);
}
//---------------------------------------------------------------------
void TSessionData::SetTunnelUserName(std::wstring value)
{
  // TunnelUserName is key for password encryption
  std::wstring XTunnelPassword = GetTunnelPassword();
  SET_SESSION_PROPERTY(TunnelUserName);
  SetTunnelPassword(XTunnelPassword);
  if (!XTunnelPassword.empty())
  {
    ::Error(SNotImplemented, 242); 
    // FIXME ::Unique(XTunnelPassword);
    memset((void *)XTunnelPassword.c_str(), 0, XTunnelPassword.size());
  }
}
//---------------------------------------------------------------------
void TSessionData::SetTunnelPassword(std::wstring value)
{
  value = EncryptPassword(value, GetTunnelUserName() + GetTunnelHostName());
  SET_SESSION_PROPERTY(TunnelPassword);
}
//---------------------------------------------------------------------
std::wstring TSessionData::GetTunnelPassword()
{
  return DecryptPassword(FTunnelPassword, GetTunnelUserName() + GetTunnelHostName());
}
//---------------------------------------------------------------------
void TSessionData::SetTunnelPublicKeyFile(std::wstring value)
{
  if (FTunnelPublicKeyFile != value)
  {
    FTunnelPublicKeyFile = StripPathQuotes(value);
    Modify();
  }
}
//---------------------------------------------------------------------
void TSessionData::SetTunnelLocalPortNumber(int value)
{
  SET_SESSION_PROPERTY(TunnelLocalPortNumber);
}
//---------------------------------------------------------------------
bool TSessionData::GetTunnelAutoassignLocalPortNumber()
{
  return (FTunnelLocalPortNumber <= 0);
}
//---------------------------------------------------------------------
void TSessionData::SetTunnelPortFwd(std::wstring value)
{
  SET_SESSION_PROPERTY(TunnelPortFwd);
}
//---------------------------------------------------------------------
void TSessionData::SetFtpPasvMode(bool value)
{
  SET_SESSION_PROPERTY(FtpPasvMode);
}
//---------------------------------------------------------------------
void TSessionData::SetFtpForcePasvIp(bool value)
{
  SET_SESSION_PROPERTY(FtpForcePasvIp);
}
//---------------------------------------------------------------------
void TSessionData::SetFtpAccount(std::wstring value)
{
  SET_SESSION_PROPERTY(FtpAccount);
}
//---------------------------------------------------------------------
void TSessionData::SetFtpPingInterval(int value)
{
  SET_SESSION_PROPERTY(FtpPingInterval);
}
//---------------------------------------------------------------------------
TDateTime TSessionData::GetFtpPingIntervalDT()
{
  return SecToDateTime(GetFtpPingInterval());
}
//---------------------------------------------------------------------------
void TSessionData::SetFtpPingType(TPingType value)
{
  SET_SESSION_PROPERTY(FtpPingType);
}
//---------------------------------------------------------------------------
void TSessionData::SetFtps(TFtps value)
{
  SET_SESSION_PROPERTY(Ftps);
}
//---------------------------------------------------------------------
void TSessionData::SetNotUtf(TAutoSwitch value)
{
  SET_SESSION_PROPERTY(NotUtf);
}
//---------------------------------------------------------------------
void TSessionData::SetHostKey(std::wstring value)
{
  SET_SESSION_PROPERTY(HostKey);
}
//---------------------------------------------------------------------
std::wstring TSessionData::GetInfoTip()
{
  if (GetUsesSsh())
  {
    return FMTLOAD(SESSION_INFO_TIP,
        GetHostName().c_str(), GetUserName().c_str(),
        (GetPublicKeyFile().empty() ? LoadStr(NO_STR).c_str() : LoadStr(YES_STR).c_str()),
        GetSshProtStr().c_str(), GetFSProtocolStr().c_str());
  }
  else
  {
    return FMTLOAD(SESSION_INFO_TIP_NO_SSH,
      GetHostName().c_str(), GetUserName().c_str(), GetFSProtocolStr().c_str());
  }
}
//---------------------------------------------------------------------
std::wstring TSessionData::GetLocalName()
{
  std::wstring Result = Name;
  int P = ::LastDelimiter(Result, L"/");
  if (P > 0)
  {
    Result.erase(1, P);
  }
  return Result;
}
//=== TStoredSessionList ----------------------------------------------
TStoredSessionList::TStoredSessionList(bool aReadOnly):
  TNamedObjectList(), FReadOnly(aReadOnly)
{
  assert(Configuration);
  FDefaultSettings = new TSessionData(DefaultName);
}
//---------------------------------------------------------------------
TStoredSessionList::~TStoredSessionList()
{
  assert(Configuration);
  delete FDefaultSettings;
}
//---------------------------------------------------------------------
void TStoredSessionList::Load(THierarchicalStorage * Storage,
  bool AsModified, bool UseDefaults)
{
  TStringList *SubKeys = new TStringList();
  TList * Loaded = new TList;
  {
    BOOST_SCOPE_EXIT ( (&SubKeys) (&Loaded) )
    {
      delete SubKeys;
      delete Loaded;
    } BOOST_SCOPE_EXIT_END
    Storage->GetSubKeyNames(SubKeys);
    for (int Index = 0; Index < SubKeys->GetCount(); Index++)
    {
      TSessionData *SessionData;
      std::wstring SessionName = SubKeys->GetString(Index);
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
        if (SessionName == FDefaultSettings->Name) SessionData = FDefaultSettings;
          else SessionData = (TSessionData*)FindByName(SessionName);

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
}
//---------------------------------------------------------------------
void TStoredSessionList::Load(std::wstring aKey, bool UseDefaults)
{
  TRegistryStorage * Storage = new TRegistryStorage(aKey);
  {
    BOOST_SCOPE_EXIT ( (&Storage) )
    {
      delete Storage;
    } BOOST_SCOPE_EXIT_END
    if (Storage->OpenRootKey(false)) Load(Storage, false, UseDefaults);
  }
}
//---------------------------------------------------------------------
void TStoredSessionList::Load()
{
  THierarchicalStorage * Storage = Configuration->CreateScpStorage(true);
  {
    BOOST_SCOPE_EXIT ( (&Storage) )
    {
      delete Storage;
    } BOOST_SCOPE_EXIT_END
    if (Storage->OpenSubKey(Configuration->GetStoredSessionsSubKey(), false))
      Load(Storage);
  }
}
//---------------------------------------------------------------------
void TStoredSessionList::DoSave(THierarchicalStorage * Storage,
  TSessionData * Data, bool All, bool RecryptPasswordOnly,
  TSessionData * FactoryDefaults)
{
  DEBUG_PRINTF(L"begin: All = %d, Data->GetModified = %d", All, Data->GetModified());
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
  DEBUG_PRINTF(L"end");
}
//---------------------------------------------------------------------
void TStoredSessionList::DoSave(THierarchicalStorage * Storage,
  bool All, bool RecryptPasswordOnly)
{
  DEBUG_PRINTF(L"begin");
  TSessionData * FactoryDefaults = new TSessionData(L"");
  {
    BOOST_SCOPE_EXIT ( (&FactoryDefaults) )
    {
      delete FactoryDefaults;
    } BOOST_SCOPE_EXIT_END
    DoSave(Storage, FDefaultSettings, All, RecryptPasswordOnly, FactoryDefaults);
    for (int Index = 0; Index < GetCount() + GetHiddenCount(); Index++)
    {
      TSessionData * SessionData = (TSessionData *)GetItem(Index);
      DoSave(Storage, SessionData, All, RecryptPasswordOnly, FactoryDefaults);
    }
  }
  DEBUG_PRINTF(L"end");
}
//---------------------------------------------------------------------
void TStoredSessionList::Save(THierarchicalStorage * Storage, bool All)
{
  DoSave(Storage, All, false);
}
//---------------------------------------------------------------------
void TStoredSessionList::DoSave(bool All, bool Explicit, bool RecryptPasswordOnly)
{
  DEBUG_PRINTF(L"begin")
  THierarchicalStorage * Storage = Configuration->CreateScpStorage(true);
  {
    BOOST_SCOPE_EXIT ( (&Storage) )
    {
      delete Storage;
    } BOOST_SCOPE_EXIT_END
    Storage->SetAccessMode(smReadWrite);
    Storage->SetExplicit(Explicit);
    DEBUG_PRINTF(L"Configuration->GetStoredSessionsSubKey = %s", Configuration->GetStoredSessionsSubKey().c_str());
    if (Storage->OpenSubKey(Configuration->GetStoredSessionsSubKey(), true))
    {
      DoSave(Storage, All, RecryptPasswordOnly);
    }
  }
  DEBUG_PRINTF(L"end");
  Saved();
}
//---------------------------------------------------------------------
void TStoredSessionList::Save(bool All, bool Explicit)
{
  DoSave(All, Explicit, false);
}
//---------------------------------------------------------------------
void TStoredSessionList::RecryptPasswords()
{
  DoSave(true, true, true);
}
//---------------------------------------------------------------------
void TStoredSessionList::Saved()
{
  FDefaultSettings->SetModified(false);
  for (int Index = 0; Index < GetCount() + GetHiddenCount(); Index++)
  {
    ((TSessionData *)GetItem(Index))->SetModified(false);
  }
}
//---------------------------------------------------------------------
void TStoredSessionList::Export(const std::wstring FileName)
{
  THierarchicalStorage * Storage = new TIniFileStorage(FileName);
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
}
//---------------------------------------------------------------------
void TStoredSessionList::SelectAll(bool Select)
{
  for (int Index = 0; Index < GetCount(); Index++)
    GetSession(Index)->SetSelected(Select);
}
//---------------------------------------------------------------------
void TStoredSessionList::Import(TStoredSessionList * From,
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
void TStoredSessionList::SelectSessionsToImport
  (TStoredSessionList * Dest, bool SSHOnly)
{
  for (int Index = 0; Index < GetCount(); Index++)
  {
    GetSession(Index)->SetSelected(
      (!SSHOnly || (GetSession(Index)->GetProtocol() == ptSSH)) &&
      !Dest->FindByName(GetSession(Index)->Name));
  }
}
//---------------------------------------------------------------------
void TStoredSessionList::Cleanup()
{
  try
  {
    if (Configuration->GetStorage() == stRegistry) Clear();
    TRegistryStorage * Storage = new TRegistryStorage(Configuration->GetRegistryStorageKey());
    {
      BOOST_SCOPE_EXIT ( (&Storage) )
      {
        delete Storage;
      } BOOST_SCOPE_EXIT_END
      Storage->SetAccessMode(smReadWrite);
      if (Storage->OpenRootKey(false))
        Storage->RecursiveDeleteSubKey(Configuration->GetStoredSessionsSubKey());
    }
  } catch (const std::exception &E)
  {
    throw ExtException(&E, FMTLOAD(CLEANUP_SESSIONS_ERROR));
  }
}
//---------------------------------------------------------------------------
int TStoredSessionList::IndexOf(TSessionData * Data)
{
  for (int Index = 0; Index < GetCount(); Index++)
    if (Data == GetSession(Index)) return Index;
  return -1;
}
//---------------------------------------------------------------------------
TSessionData * TStoredSessionList::NewSession(
  std::wstring SessionName, TSessionData * Session)
{
  TSessionData * DuplicateSession = (TSessionData*)FindByName(SessionName);
  if (!DuplicateSession)
  {
    DuplicateSession = new TSessionData(L"");
    DuplicateSession->Assign(Session);
    DuplicateSession->Name = SessionName;
    // make sure, that new stored session is saved to registry
    DuplicateSession->SetModified(true);
    Add(DuplicateSession);
  }
    else
  {
    DuplicateSession->Assign(Session);
    DuplicateSession->Name = SessionName;
    DuplicateSession->SetModified(true);
  }
  // list was saved here before to default storage, but it would not allow
  // to work with special lists (export/import) not using default storage
  return DuplicateSession;
}
//---------------------------------------------------------------------------
void TStoredSessionList::SetDefaultSettings(TSessionData * value)
{
  assert(FDefaultSettings);
  if (FDefaultSettings != value)
  {
    FDefaultSettings->Assign(value);
    FDefaultSettings->Name = DefaultName;
    if (!FReadOnly)
    {
      // only modified, explicit
      Save(false, true);
    }
  }
}
//---------------------------------------------------------------------------
void TStoredSessionList::ImportHostKeys(const std::wstring TargetKey,
  const std::wstring SourceKey, TStoredSessionList * Sessions,
  bool OnlySelected)
{
  TRegistryStorage * SourceStorage = NULL;
  TRegistryStorage * TargetStorage = NULL;
  TStringList * KeyList = NULL;
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
      std::wstring HostKeyName;
      assert(Sessions != NULL);
      for (int Index = 0; Index < Sessions->GetCount(); Index++)
      {
        Session = Sessions->GetSession(Index);
        if (!OnlySelected || Session->GetSelected())
        {
          HostKeyName = PuttyMungeStr(FORMAT(L"@%d:%s", Session->GetPortNumber(), Session->GetHostName()));
          std::wstring KeyName;
          for (int KeyIndex = 0; KeyIndex < KeyList->GetCount(); KeyIndex++)
          {
            KeyName = KeyList->GetString(KeyIndex);
            int P = KeyName.find_first_of(HostKeyName);
            if ((P > 0) && (P == KeyName.size() - HostKeyName.size() + 1))
            {
              TargetStorage->WriteStringRaw(KeyName,
                SourceStorage->ReadStringRaw(KeyName, L""));
            }
          }
        }
      }
    }
  }
}
//---------------------------------------------------------------------------
TSessionData * TStoredSessionList::ParseUrl(std::wstring Url,
  TOptions * Options, bool & DefaultsOnly, std::wstring * FileName,
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
