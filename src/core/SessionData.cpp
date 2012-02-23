//---------------------------------------------------------------------------
#include "stdafx.h"

#include "boostdefines.hpp"
#include <boost/scope_exit.hpp>

#include "SessionData.h"

#include "Common.h"
#include "Exceptions.h"
#include "CoreMain.h"
#include "TextsCore.h"
#include "PuttyIntf.h"
#include "RemoteFiles.h"
//---------------------------------------------------------------------------
enum TProxyType { pxNone, pxHTTP, pxSocks, pxTelnet }; // 0.53b and older
const wchar_t *DefaultName = L"Default Settings";
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
const std::wstring CONST_LOGIN_ANONYMOUS = L"anonymous";
const int SshPortNumber = 22;
const int FtpPortNumber = 21;
const int HTTPPortNumber = 80;
const int HTTPSPortNumber = 443;
const int FtpsImplicitPortNumber = 990;
const unsigned int CONST_DEFAULT_CODEPAGE = 65001;
const std::wstring CONST_DEFAULT_CODEPAGE_STRING = L"65001 (UTF-8)";
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

    if (CodePageInfoEx.MaxCharSize != 1)
        return false;

    return true;
}
//---------------------------------------------------------------------
unsigned int GetCodePageAsNumber(const std::wstring CodePage)
{
    unsigned int codePage = _wtoi(CodePage.c_str());
    return codePage == 0 ? CONST_DEFAULT_CODEPAGE : codePage;
}

//---------------------------------------------------------------------
nb::TDateTime SecToDateTime(int Sec)
{
    return nb::TDateTime(static_cast<unsigned short>(Sec/60/60),
                         static_cast<unsigned short>(Sec/60%60), static_cast<unsigned short>(Sec%60), 0);
}
//--- TSessionData ----------------------------------------------------
TSessionData::TSessionData(const std::wstring aName):
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
    for (size_t Index = 0; Index < CIPHER_COUNT; Index++)
    {
        SetCipher(Index, DefaultCipherList[Index]);
    }
    for (size_t Index = 0; Index < KEX_COUNT; Index++)
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

    for (size_t Index = 0; Index < LENOF(FBugs); Index++)
    {
        SetBug(static_cast<TSshBug>(Index), asAuto);
    }

    SetSpecial(false);
    SetFSProtocol(fsSFTP);
    SetAddressFamily(afAuto);
    SetCodePage(CONST_DEFAULT_CODEPAGE_STRING);
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
    SetTimeDifference(nb::TDateTime(0));
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

    for (size_t Index = 0; Index < LENOF(FSFTPBugs); Index++)
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
    SetFtpAllowEmptyPassword(false);
    SetFtpEncryption(fesPlainFTP);
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
void TSessionData::Assign(nb::TPersistent *Source)
{
    if (Source && ::InheritsFrom<nb::TPersistent, TSessionData>(Source))
    {
#define DUPL(P) Set##P(((TSessionData *)Source)->Get##P())
        DUPL(Name);
        DUPL(HostName);
        // DEBUG_PRINTF(L"HostName = %s, Source->HostName = %s", GetHostName().c_str(), ((TSessionData *)Source)->GetHostName().c_str());
        DUPL(PortNumber);
        DUPL(LoginType);
        DUPL(UserName);
        DUPL(Password);
        // SetPassword(Source->GetPassword());
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

        for (size_t Index = 0; Index < LENOF(FBugs); Index++)
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

        for (size_t Index = 0; Index < LENOF(FSFTPBugs); Index++)
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

#undef DUPL
        FModified = (static_cast<TSessionData *>(Source))->GetModified();
        FSource = (static_cast<TSessionData *>(Source))->FSource;
    }
    else
    {
        TNamedObject::Assign(Source);
    }
}
//---------------------------------------------------------------------
void TSessionData::Load(THierarchicalStorage *Storage)
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
        SetLoginType(static_cast<TLoginType>(Storage->Readint(L"LoginType", GetLoginType())));
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
        SetSshProt(static_cast<TSshProt>(Storage->Readint(L"SshProt", GetSshProt())));
        SetSsh2DES(Storage->Readbool(L"Ssh2DES", GetSsh2DES()));
        SetSshNoUserAuth(Storage->Readbool(L"SshNoUserAuth", GetSshNoUserAuth()));
        SetCipherList(Storage->ReadString(L"Cipher", GetCipherList()));
        SetKexList(Storage->ReadString(L"KEX", GetKexList()));
        SetPublicKeyFile(Storage->ReadString(L"PublicKeyFile", GetPublicKeyFile()));
        SetAddressFamily(static_cast<TAddressFamily>
                         (Storage->Readint(L"AddressFamily", GetAddressFamily())));
        SetCodePage(Storage->ReadString(L"CodePage", GetCodePage()));
        SetRekeyData(Storage->ReadString(L"RekeyBytes", GetRekeyData()));
        SetRekeyTime(Storage->Readint(L"RekeyTime", GetRekeyTime()));

        SetFSProtocol(static_cast<TFSProtocol>(Storage->Readint(L"FSProtocol", GetFSProtocol())));
        SetLocalDirectory(Storage->ReadString(L"LocalDirectory", GetLocalDirectory()));
        SetRemoteDirectory(Storage->ReadString(L"RemoteDirectory", GetRemoteDirectory()));
        SetUpdateDirectories(Storage->Readbool(L"UpdateDirectories", GetUpdateDirectories()));
        SetCacheDirectories(Storage->Readbool(L"CacheDirectories", GetCacheDirectories()));
        SetCacheDirectoryChanges(Storage->Readbool(L"CacheDirectoryChanges", GetCacheDirectoryChanges()));
        SetPreserveDirectoryChanges(Storage->Readbool(L"PreserveDirectoryChanges", GetPreserveDirectoryChanges()));

        SetResolveSymlinks(Storage->Readbool(L"ResolveSymlinks", GetResolveSymlinks()));
        SetDSTMode(static_cast<TDSTMode>(Storage->Readint(L"ConsiderDST", GetDSTMode())));
        SetLockInHome(Storage->Readbool(L"LockInHome", GetLockInHome()));
        SetSpecial(Storage->Readbool(L"Special", GetSpecial()));
        SetShell(Storage->ReadString(L"Shell", GetShell()));
        SetClearAliases(Storage->Readbool(L"ClearAliases", GetClearAliases()));
        SetUnsetNationalVars(Storage->Readbool(L"UnsetNationalVars", GetUnsetNationalVars()));
        SetListingCommand(Storage->ReadString(L"ListingCommand",
                                              Storage->Readbool(L"AliasGroupList", false) ? std::wstring(L"ls -gla") : GetListingCommand()));
        SetIgnoreLsWarnings(Storage->Readbool(L"IgnoreLsWarnings", GetIgnoreLsWarnings()));
        SetSCPLsFullTime(static_cast<TAutoSwitch>(Storage->Readint(L"SCPLsFullTime", GetSCPLsFullTime())));
        SetFtpListAll(static_cast<TAutoSwitch>(Storage->Readint(L"FtpListAll", GetFtpListAll())));
        SetScp1Compatibility(Storage->Readbool(L"Scp1Compatibility", GetScp1Compatibility()));
        SetTimeDifference(nb::TDateTime(Storage->ReadFloat(L"TimeDifference", GetTimeDifference())));
        SetDeleteToRecycleBin(Storage->Readbool(L"DeleteToRecycleBin", GetDeleteToRecycleBin()));
        SetOverwrittenToRecycleBin(Storage->Readbool(L"OverwrittenToRecycleBin", GetOverwrittenToRecycleBin()));
        SetRecycleBinPath(Storage->ReadString(L"RecycleBinPath", GetRecycleBinPath()));
        SetPostLoginCommands(Storage->ReadString(L"PostLoginCommands", GetPostLoginCommands()));

        SetReturnVar(Storage->ReadString(L"ReturnVar", GetReturnVar()));
        SetLookupUserGroups(Storage->Readbool(L"LookupUserGroups", GetLookupUserGroups()));
        SetEOLType(static_cast<TEOLType>(Storage->Readint(L"EOLType", GetEOLType())));
        SetNotUtf(static_cast<TAutoSwitch>(Storage->Readint(L"Utf", Storage->Readint(L"SFTPUtfBug", GetNotUtf()))));

        SetTcpNoDelay(Storage->Readbool(L"TcpNoDelay", GetTcpNoDelay()));

        SetProxyMethod(static_cast<TProxyMethod>(Storage->Readint(L"ProxyMethod", -1)));
        if (GetProxyMethod() < 0)
        {
            int ProxyType = Storage->Readint(L"ProxyType", pxNone);
            int ProxySOCKSVersion;
            switch (ProxyType)
            {
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
        SetProxyDNS(static_cast<TAutoSwitch>((Storage->Readint(L"ProxyDNS", (GetProxyDNS() + 2) % 3) + 1) % 3));
        SetProxyLocalhost(Storage->Readbool(L"ProxyLocalhost", GetProxyLocalhost()));

#define READ_BUG(BUG) \
      SetBug(sb##BUG, TAutoSwitch(2 - Storage->Readint(L"Bug" + nb::MB2W(#BUG), \
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
      SetSFTPBug(sb##BUG, TAutoSwitch(Storage->Readint(L"SFTP" + nb::MB2W(#BUG) + L"Bug", GetSFTPBug(sb##BUG))));
        READ_SFTP_BUG(Symlink);
        READ_SFTP_BUG(SignedTS);
#undef READ_SFTP_BUG

        SetSFTPMaxVersion(static_cast<size_t>(Storage->Readint(L"SFTPMaxVersion", GetSFTPMaxVersion())));
        SetSFTPMinPacketSize(static_cast<size_t>(Storage->Readint(L"SFTPMinPacketSize", GetSFTPMinPacketSize())));
        SetSFTPMaxPacketSize(static_cast<size_t>(Storage->Readint(L"SFTPMaxPacketSize", GetSFTPMaxPacketSize())));

        SetColor(Storage->Readint(L"Color", GetColor()));

        SetProtocolStr(Storage->ReadString(L"Protocol", GetProtocolStr()));

        SetTunnel(Storage->Readbool(L"Tunnel", GetTunnel()));
        SetTunnelPortNumber(static_cast<size_t>(Storage->Readint(L"TunnelPortNumber", GetTunnelPortNumber())));
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
        SetTunnelLocalPortNumber(static_cast<size_t>(Storage->Readint(L"TunnelLocalPortNumber", GetTunnelLocalPortNumber())));

        // Ftp prefix
        SetFtpPasvMode(Storage->Readbool(L"FtpPasvMode", GetFtpPasvMode()));
        SetFtpAllowEmptyPassword(Storage->Readbool(L"FtpAllowEmptyPassword", GetFtpAllowEmptyPassword()));
        SetFtpEncryption(static_cast<TFtpEncryptionSwitch>(Storage->Readint(L"FtpEncryption", GetFtpEncryption())));
        SetFtpForcePasvIp(Storage->Readbool(L"FtpForcePasvIp", GetFtpForcePasvIp()));
        SetFtpAccount(Storage->ReadString(L"FtpAccount", GetFtpAccount()));
        SetFtpPingInterval(Storage->Readint(L"FtpPingInterval", GetFtpPingInterval()));
        SetFtpPingType(static_cast<TPingType>(Storage->Readint(L"FtpPingType", GetFtpPingType())));
        SetFtps(static_cast<TFtps>(Storage->Readint(L"Ftps", GetFtps())));

        SetFtpProxyLogonType(static_cast<size_t>(Storage->Readint(L"FtpProxyLogonType", GetFtpProxyLogonType())));

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
        catch (...)
        {
            // ignore errors (like read-only INI file)
        }

        Storage->SetAccessMode(AccessMode);
    }

    FModified = false;
    FSource = ssStored;
}
//---------------------------------------------------------------------
void TSessionData::Save(THierarchicalStorage *Storage,
                        bool PuttyExport, const TSessionData *Default)
{
    // DEBUG_PRINTF(L"begin");
    if (Storage->OpenSubKey(GetInternalStorageKey(), true))
    {
#define WRITE_DATA_EX(TYPE, NAME, PROPERTY, CONV) \
      if ((Default != NULL) && (CONV(Default->PROPERTY) == CONV(PROPERTY))) \
      { \
        Storage->DeleteValue(std::wstring(NAME)); \
      } \
      else \
      { \
        Storage->Write##TYPE(std::wstring(NAME), CONV(PROPERTY)); \
      }
#define WRITE_DATA_CONV(TYPE, NAME, PROPERTY) WRITE_DATA_EX(TYPE, NAME, PROPERTY, WRITE_DATA_CONV_FUNC)
#define WRITE_DATA(TYPE, PROPERTY) WRITE_DATA_EX(TYPE, #PROPERTY, PROPERTY, )

        WRITE_DATA_EX(String, L"HostName", GetHostName(), );
        WRITE_DATA_EX(int, L"PortNumber", GetPortNumber(), );
        WRITE_DATA_EX(bool, L"Passwordless", GetPasswordless(), );
        WRITE_DATA_EX(int, L"PingInterval", GetPingInterval() / 60, );
        WRITE_DATA_EX(int, L"PingIntervalSecs", GetPingInterval() % 60, );
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
        WRITE_DATA_EX(int, L"Timeout", GetTimeout(), );
        WRITE_DATA_EX(bool, L"TryAgent", GetTryAgent(), );
        WRITE_DATA_EX(bool, L"AgentFwd", GetAgentFwd(), );
        WRITE_DATA_EX(bool, L"AuthTIS", GetAuthTIS(), );
        WRITE_DATA_EX(bool, L"AuthKI", GetAuthKI(), );
        WRITE_DATA_EX(bool, L"AuthKIPassword", GetAuthKIPassword(), );

        WRITE_DATA_EX(bool, L"AuthGSSAPI", GetAuthGSSAPI(), );
        WRITE_DATA_EX(bool, L"GSSAPIFwdTGT", GetGSSAPIFwdTGT(), );
        WRITE_DATA_EX(String, L"GSSAPIServerRealm", GetGSSAPIServerRealm(), );
        Storage->DeleteValue(L"TryGSSKEX");
        Storage->DeleteValue(L"UserNameFromEnvironment");
        Storage->DeleteValue(L"GSSAPIServerChoosesUserName");
        Storage->DeleteValue(L"GSSAPITrustDNS");
        if (PuttyExport)
        {
            // duplicate kerberos setting with keys of the vintela quest putty
            WRITE_DATA_EX(bool, L"AuthSSPI", GetAuthGSSAPI(), );
            WRITE_DATA_EX(bool, L"SSPIFwdTGT", GetGSSAPIFwdTGT(), );
            WRITE_DATA_EX(String, L"KerbPrincipal", GetGSSAPIServerRealm(), );
            // duplicate kerberos setting with keys of the official putty
            WRITE_DATA_EX(bool, L"GssapiFwd", GetGSSAPIFwdTGT(), );
        }

        WRITE_DATA_EX(bool, L"ChangeUsername", GetChangeUsername(), );
        WRITE_DATA_EX(bool, L"Compression", GetCompression(), );
        WRITE_DATA_EX(int, L"SshProt", GetSshProt(), );
        WRITE_DATA_EX(bool, L"Ssh2DES", GetSsh2DES(), );
        WRITE_DATA_EX(bool, L"SshNoUserAuth", GetSshNoUserAuth(), );
        WRITE_DATA_EX(String, L"Cipher", GetCipherList(), );
        WRITE_DATA_EX(String, L"KEX", GetKexList(), );
        WRITE_DATA_EX(int, L"AddressFamily", GetAddressFamily(), );
        WRITE_DATA_EX(String, L"CodePage", GetCodePage(), );
        WRITE_DATA_EX(String, L"RekeyBytes", GetRekeyData(), );
        WRITE_DATA_EX(int, L"RekeyTime", GetRekeyTime(), );

        WRITE_DATA_EX(bool, L"TcpNoDelay", GetTcpNoDelay(), );

        if (PuttyExport)
        {
            WRITE_DATA_EX(String, L"UserName", GetUserName(), );
            WRITE_DATA_EX(String, L"PublicKeyFile", GetPublicKeyFile(), );
        }
        else
        {
            WRITE_DATA_EX(int, L"LoginType", GetLoginType(), );
            WRITE_DATA_EX(String, L"UserName", GetUserName(), );
            WRITE_DATA_EX(int, L"LoginType", GetLoginType(), );
            WRITE_DATA_EX(String, L"PublicKeyFile", GetPublicKeyFile(), );
            WRITE_DATA_EX(int, L"FSProtocol", GetFSProtocol(), );
            WRITE_DATA_EX(String, L"LocalDirectory", GetLocalDirectory(), );
            WRITE_DATA_EX(String, L"RemoteDirectory", GetRemoteDirectory(), );
            WRITE_DATA_EX(bool, L"UpdateDirectories", GetUpdateDirectories(), );
            WRITE_DATA_EX(bool, L"CacheDirectories", GetCacheDirectories(), );
            WRITE_DATA_EX(bool, L"CacheDirectoryChanges", GetCacheDirectoryChanges(), );
            WRITE_DATA_EX(bool, L"PreserveDirectoryChanges", GetPreserveDirectoryChanges(), );

            WRITE_DATA_EX(bool, L"ResolveSymlinks", GetResolveSymlinks(), );
            WRITE_DATA_EX(int, L"ConsiderDST", GetDSTMode(), );
            WRITE_DATA_EX(bool, L"LockInHome", GetLockInHome(), );
            // Special is never stored (if it would, login dialog must be modified not to
            // duplicate Special parameter when Special session is loaded and then stored
            // under different name)
            // WRITE_DATA_EX(bool, L"Special", GetSpecial(), );
            WRITE_DATA_EX(String, L"Shell", GetShell(), );
            WRITE_DATA_EX(bool, L"ClearAliases", GetClearAliases(), );
            WRITE_DATA_EX(bool, L"UnsetNationalVars", GetUnsetNationalVars(), );
            WRITE_DATA_EX(String, L"ListingCommand", GetListingCommand(), );
            WRITE_DATA_EX(bool, L"IgnoreLsWarnings", GetIgnoreLsWarnings(), );
            WRITE_DATA_EX(int, L"SCPLsFullTime", GetSCPLsFullTime(), );
            WRITE_DATA_EX(int, L"FtpListAll", GetFtpListAll(), );
            WRITE_DATA_EX(bool, L"Scp1Compatibility", GetScp1Compatibility(), );
            WRITE_DATA_EX(Float, L"TimeDifference", GetTimeDifference(), );
            WRITE_DATA_EX(bool, L"DeleteToRecycleBin", GetDeleteToRecycleBin(), );
            WRITE_DATA_EX(bool, L"OverwrittenToRecycleBin", GetOverwrittenToRecycleBin(), );
            WRITE_DATA_EX(String, L"RecycleBinPath", GetRecycleBinPath(), );
            WRITE_DATA_EX(String, L"PostLoginCommands", GetPostLoginCommands(), );

            WRITE_DATA_EX(String, L"ReturnVar", GetReturnVar(), );
            WRITE_DATA_EX(bool, L"LookupUserGroups", GetLookupUserGroups(), );
            WRITE_DATA_EX(int, L"EOLType", GetEOLType(), );
            Storage->DeleteValue(L"SFTPUtfBug");
            WRITE_DATA_EX(int, L"Utf", GetNotUtf(), );
        }

        WRITE_DATA_EX(int, L"ProxyMethod", GetProxyMethod(), );
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
        WRITE_DATA_EX(String, L"ProxyHost", GetProxyHost(), );
        WRITE_DATA_EX(int, L"ProxyPort", GetProxyPort(), );
        WRITE_DATA_EX(String, L"ProxyUsername", GetProxyUsername(), );
        if (GetProxyMethod() == pmCmd)
        {
            WRITE_DATA_EX(String, L"ProxyTelnetCommand", GetProxyLocalCommand(), );
        }
        else
        {
            WRITE_DATA_EX(String, L"ProxyTelnetCommand", GetProxyTelnetCommand(), );
        }
#define WRITE_DATA_CONV_FUNC(X) (((X) + 2) % 3)
        WRITE_DATA_CONV(int, L"ProxyDNS", GetProxyDNS());
#undef WRITE_DATA_CONV_FUNC
        WRITE_DATA_EX(bool, L"ProxyLocalhost", GetProxyLocalhost(), );

#define WRITE_DATA_CONV_FUNC(X) (2 - (X))
#define WRITE_BUG(BUG) WRITE_DATA_CONV(int, nb::MB2W("Bug" #BUG), GetBug(sb##BUG));
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
            WRITE_DATA_EX(String, L"Protocol", GetProtocolStr(), );
        }

        if (!PuttyExport)
        {
            WRITE_DATA_EX(String, L"SftpServer", GetSftpServer(), );

#define WRITE_SFTP_BUG(BUG) WRITE_DATA_EX(int, nb::MB2W("SFTP" #BUG "Bug"), GetSFTPBug(sb##BUG), );
            WRITE_SFTP_BUG(Symlink);
            WRITE_SFTP_BUG(SignedTS);
#undef WRITE_SFTP_BUG

            WRITE_DATA_EX(int, L"SFTPMaxVersion", GetSFTPMaxVersion(), );
            WRITE_DATA_EX(int, L"SFTPMinPacketSize", GetSFTPMinPacketSize(), );
            WRITE_DATA_EX(int, L"SFTPMaxPacketSize", GetSFTPMaxPacketSize(), );

            WRITE_DATA_EX(int, L"Color", GetColor(), );

            WRITE_DATA_EX(bool, L"Tunnel", GetTunnel(), );
            WRITE_DATA_EX(String, L"TunnelHostName", GetTunnelHostName(), );
            WRITE_DATA_EX(int, L"TunnelPortNumber", GetTunnelPortNumber(), );
            WRITE_DATA_EX(String, L"TunnelUserName", GetTunnelUserName(), );
            WRITE_DATA_EX(String, L"TunnelPublicKeyFile", GetTunnelPublicKeyFile(), );
            WRITE_DATA_EX(int, L"TunnelLocalPortNumber", GetTunnelLocalPortNumber(), );

            WRITE_DATA_EX(bool, L"FtpPasvMode", GetFtpPasvMode(), );
            WRITE_DATA_EX(bool, L"FtpAllowEmptyPassword", GetFtpAllowEmptyPassword(), );
            WRITE_DATA_EX(int, L"FtpEncryption", GetFtpEncryption(), );
            WRITE_DATA_EX(bool, L"FtpForcePasvIp", GetFtpForcePasvIp(), );
            WRITE_DATA_EX(String, L"FtpAccount", GetFtpAccount(), );
            WRITE_DATA_EX(int, L"FtpPingInterval", GetFtpPingInterval(), );
            WRITE_DATA_EX(int, L"FtpPingType", GetFtpPingType(), );
            WRITE_DATA_EX(int, L"Ftps", GetFtps(), );

            WRITE_DATA_EX(int, L"FtpProxyLogonType", GetFtpProxyLogonType(), );

            WRITE_DATA_EX(String, L"CustomParam1", GetCustomParam1(), );
            WRITE_DATA_EX(String, L"CustomParam2", GetCustomParam2(), );
        }

        SavePasswords(Storage, PuttyExport);

        Storage->CloseSubKey();
    }
    // DEBUG_PRINTF(L"end");
}
//---------------------------------------------------------------------
void TSessionData::SavePasswords(THierarchicalStorage *Storage, bool PuttyExport)
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
void TSessionData::SaveRecryptedPasswords(THierarchicalStorage *Storage)
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
    THierarchicalStorage *Storage = Configuration->CreateStorage();
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
bool TSessionData::ParseUrl(const std::wstring Url, TOptions *Options,
                            TStoredSessionList *StoredSessions, bool &DefaultsOnly, std::wstring *FileName,
                            bool *AProtocolDefined)
{
    bool ProtocolDefined = false;
    bool PortNumberDefined = false;
    TFSProtocol AFSProtocol = fsSCPonly;
    int APortNumber = 0;
    TFtps AFtps = ftpsNone;
    std::wstring url = Url;
    if (::LowerCase(url.substr(0, 7)) == L"netbox:")
    {
        // Remove "netbox:" prefix
        url.erase(0, 7);
    }
    if (::LowerCase(url.substr(0, 4)) == L"scp:")
    {
        AFSProtocol = fsSCPonly;
        APortNumber = SshPortNumber;
        url.erase(0, 4);
        ProtocolDefined = true;
    }
    else if (::LowerCase(url.substr(0, 5)) == L"sftp:")
    {
        AFSProtocol = fsSFTPonly;
        APortNumber = SshPortNumber;
        url.erase(0, 5);
        ProtocolDefined = true;
    }
    else if (::LowerCase(url.substr(0, 4)) == L"ftp:")
    {
        AFSProtocol = fsFTP;
        SetFtps(ftpsNone);
        APortNumber = FtpPortNumber;
        url.erase(0, 4);
        ProtocolDefined = true;
    }
    else if (::LowerCase(url.substr(0, 5)) == L"ftps:")
    {
        AFSProtocol = fsFTP;
        AFtps = ftpsImplicit;
        APortNumber = FtpsImplicitPortNumber;
        url.erase(0, 5);
        ProtocolDefined = true;
    }
    else if (::LowerCase(url.substr(0, 5)) == L"http:")
    {
        AFSProtocol = fsHTTP;
        APortNumber = HTTPPortNumber;
        url.erase(0, 5);
        ProtocolDefined = true;
    }
    else if (::LowerCase(url.substr(0, 6)) == L"https:")
    {
        AFSProtocol = fsHTTPS;
        APortNumber = HTTPSPortNumber;
        url.erase(0, 6);
        ProtocolDefined = true;
    }

    if (ProtocolDefined && (url.substr(0, 2) == L"//"))
    {
        url.erase(0, 2);
    }

    if (AProtocolDefined != NULL)
    {
        *AProtocolDefined = ProtocolDefined;
    }

    if (!url.empty())
    {
        const std::wstring DecodedUrl = DecodeUrlChars(url);
        // lookup stored session even if protocol was defined
        // (this allows setting for example default username for host
        // by creating stored session named by host)
        TSessionData *Data = NULL;
        for (size_t Index = 0; Index < StoredSessions->GetCount() + StoredSessions->GetHiddenCount(); Index++)
        {
            TSessionData *AData = static_cast<TSessionData *>(StoredSessions->GetItem(Index));
            if (AnsiSameText(AData->GetName(), DecodedUrl) ||
                    AnsiSameText(AData->GetName() + L"/", DecodedUrl.substr(0, AData->GetName().size() + 1)))
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
            size_t P = 0;
            while (!AnsiSameText(DecodeUrlChars(url.substr(0, P)), Data->GetName()))
            {
                assert(P < url.size());
                P++;
            }
            ARemoteDirectory = url.substr(P, url.size() - P);

            if (Data->GetHidden())
            {
                // DEBUG_PRINTF(L"StoredSessions->IsHidden(Data) = %d", StoredSessions->IsHidden(Data));
                Data->Remove();
                StoredSessions->Remove(Data);
                // DEBUG_PRINTF(L"StoredSessions->Count = %d", StoredSessions->GetCount());
                // only modified, implicit
                StoredSessions->Save(false, false);
            }
        }
        else
        {
            Assign(StoredSessions->GetDefaultSettings());
            SetName(L"");

            size_t PSlash = url.find_first_of(L"/");
            if (PSlash == std::wstring::npos)
            {
                PSlash = url.size() + 1;
            }

            std::wstring ConnectInfo = url.substr(0, PSlash - 1);

            size_t P = ::LastDelimiter(ConnectInfo, L"@");

            std::wstring UserInfo;
            std::wstring HostInfo;

            if (P != std::wstring::npos)
            {
                UserInfo = ConnectInfo.substr(0, P);
                HostInfo = ConnectInfo.substr(P + 1, ConnectInfo.size() - P);
            }
            else
            {
                HostInfo = ConnectInfo;
            }
            DEBUG_PRINTF(L"UserInfo = %s, HostInfo = %s", UserInfo.c_str(), HostInfo.c_str());
            if ((HostInfo.size() >= 2) && (HostInfo[0] == '[') && ((P = HostInfo.find(L"]")) != std::wstring::npos))
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

            bool PasswordSeparator = (UserInfo.find_first_of(':') != std::wstring::npos);
            SetUserName(DecodeUrlChars(CutToChar(UserInfo, ':', false)));
            SetPassword(DecodeUrlChars(UserInfo));
            SetPasswordless(GetPassword().empty() && PasswordSeparator);

            if (PSlash < url.size())
            {
                ARemoteDirectory = url.substr(PSlash, url.size() - PSlash + 1);
            }
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
        if (Options->FindSwitch(L"allowemptypassword", Value))
        {
            SetFtpAllowEmptyPassword((StrToIntDef(Value, 0) != 0));
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
void TSessionData::ConfigureTunnel(size_t APortNumber)
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
    if (::LastDelimiter(Name, L"/") != std::wstring::npos)
    {
        throw ExtException(FMTLOAD(ITEM_NAME_INVALID, Name.c_str(), L"/"));
    }
}
//---------------------------------------------------------------------
std::wstring TSessionData::EncryptPassword(const std::wstring  Password, const std::wstring Key)
{
    return Configuration->EncryptPassword(Password, Key);
}
//---------------------------------------------------------------------
std::wstring TSessionData::StronglyRecryptPassword(const std::wstring Password, const std::wstring Key)
{
    return Configuration->StronglyRecryptPassword(Password, Key);
}
//---------------------------------------------------------------------
std::wstring TSessionData::DecryptPassword(const std::wstring Password, const std::wstring Key)
{
    std::wstring Result;
    try
    {
        Result = Configuration->DecryptPassword(Password, Key);
    }
    catch (nb::EAbort &)
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
    return FORMAT(L"%s@%s", GetUserName().c_str(), GetHostName().c_str());
}
//---------------------------------------------------------------------
std::wstring TSessionData::GetInternalStorageKey()
{
    if (GetName().empty())
    {
        return GetSessionKey();
    }
    else
    {
        return GetName();
    }
}
//---------------------------------------------------------------------
std::wstring TSessionData::GetStorageKey()
{
    return GetSessionName();
}
//---------------------------------------------------------------------
void TSessionData::SetHostName(const std::wstring value)
{
    std::wstring val = value;
    if (FHostName != val)
    {
        RemoveProtocolPrefix(val);
        // HostName is key for password encryption
        std::wstring XPassword = GetPassword();

        size_t P = ::LastDelimiter(val, L"@");
        if (P != std::wstring::npos)
        {
            SetUserName(val.substr(0, P));
            val = val.substr(P + 1, val.size() - P);
        }
        FHostName = val;
        Modify();

        SetPassword(XPassword);
        if (!XPassword.empty())
        {
            memset(const_cast<wchar_t *>(XPassword.c_str()), 0, XPassword.size() * sizeof(wchar_t));
        }
    }
}
//---------------------------------------------------------------------
void TSessionData::SetPortNumber(size_t value)
{
    SET_SESSION_PROPERTY(PortNumber);
}
//---------------------------------------------------------------------------
void TSessionData::SetShell(const std::wstring value)
{
    SET_SESSION_PROPERTY(Shell);
}
//---------------------------------------------------------------------------
void TSessionData::SetSftpServer(const std::wstring value)
{
    SET_SESSION_PROPERTY(SftpServer);
}
//---------------------------------------------------------------------------
void TSessionData::SetClearAliases(bool value)
{
    SET_SESSION_PROPERTY(ClearAliases);
}
//---------------------------------------------------------------------------
void TSessionData::SetListingCommand(const std::wstring value)
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
TLoginType TSessionData::GetLoginType() const
{
    return (GetUserName() == CONST_LOGIN_ANONYMOUS) && GetPassword().empty() ?
           ltAnonymous : ltNormal;
}
//---------------------------------------------------------------------
void TSessionData::SetLoginType(TLoginType value)
{
    SET_SESSION_PROPERTY(LoginType);
    if (GetLoginType() == ltAnonymous)
    {
        SetPassword(L"");
        SetUserName(CONST_LOGIN_ANONYMOUS);
    }
}
//---------------------------------------------------------------------
void TSessionData::SetUserName(const std::wstring value)
{
    // UserName is key for password encryption
    std::wstring XPassword = GetPassword();
    SET_SESSION_PROPERTY(UserName);
    SetPassword(XPassword);
    if (!XPassword.empty())
    {
        memset(const_cast<wchar_t *>(XPassword.c_str()), 0, XPassword.size() * sizeof(wchar_t));
    }
}
//---------------------------------------------------------------------
void TSessionData::SetPassword(const std::wstring val)
{
    if (!val.empty())
    {
        SetPasswordless(false);
    }
    std::wstring value = EncryptPassword(val, GetUserName() + GetHostName());
    SET_SESSION_PROPERTY(Password);
}
//---------------------------------------------------------------------
std::wstring TSessionData::GetPassword() const
{
    return DecryptPassword(FPassword, GetUserName() + GetHostName());
}
//---------------------------------------------------------------------
void TSessionData::SetPasswordless(bool value)
{
    SET_SESSION_PROPERTY(Passwordless);
}
//---------------------------------------------------------------------
void TSessionData::SetPingInterval(size_t value)
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
void TSessionData::SetGSSAPIServerRealm(const std::wstring value)
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
    return (GetFSProtocol() < fsFTP);
}
//---------------------------------------------------------------------
void TSessionData::SetCipher(size_t Index, TCipher value)
{
    assert(Index != -1 && Index < CIPHER_COUNT);
    SET_SESSION_PROPERTY(Ciphers[Index]);
}
//---------------------------------------------------------------------
TCipher TSessionData::GetCipher(size_t Index) const
{
    assert(Index != -1 && Index < CIPHER_COUNT);
    return FCiphers[Index];
}
//---------------------------------------------------------------------
void TSessionData::SetCipherList(const std::wstring value)
{
    bool Used[CIPHER_COUNT];
    for (size_t C = 0; C < CIPHER_COUNT; C++)
    {
        Used[C] = false;
    }

    std::wstring CipherStr;
    size_t Index = 0;
    std::wstring val = value;
    while (!val.empty() && (Index < CIPHER_COUNT))
    {
        CipherStr = CutToChar(val, ',', true);
        for (size_t C = 0; C < CIPHER_COUNT; C++)
        {
            if (!::AnsiCompareIC(CipherStr, CipherNames[C]))
            {
                SetCipher(Index, static_cast<TCipher>(C));
                Used[C] = true;
                Index++;
                break;
            }
        }
    }

    for (size_t C = 0; C < CIPHER_COUNT && Index < CIPHER_COUNT; C++)
    {
        if (!Used[DefaultCipherList[C]]) { SetCipher(Index++, DefaultCipherList[C]); }
    }
}
//---------------------------------------------------------------------
std::wstring TSessionData::GetCipherList() const
{
    std::wstring Result;
    for (size_t Index = 0; Index < CIPHER_COUNT; Index++)
    {
        Result += std::wstring(Index ? L"," : L"") + CipherNames[GetCipher(Index)];
    }
    return Result;
}
//---------------------------------------------------------------------
void TSessionData::SetKex(size_t Index, TKex value)
{
    assert(Index != -1 && Index < KEX_COUNT);
    SET_SESSION_PROPERTY(Kex[Index]);
}
//---------------------------------------------------------------------
TKex TSessionData::GetKex(size_t Index) const
{
    assert(Index != -1 && Index < KEX_COUNT);
    return FKex[Index];
}
//---------------------------------------------------------------------
void TSessionData::SetKexList(const std::wstring value)
{
    bool Used[KEX_COUNT];
    for (int K = 0; K < KEX_COUNT; K++) { Used[K] = false; }

    std::wstring KexStr;
    size_t Index = 0;
    std::wstring val = value;
    while (!val.empty() && (Index < KEX_COUNT))
    {
        KexStr = CutToChar(val, ',', true);
        for (int K = 0; K < KEX_COUNT; K++)
        {
            if (!::AnsiCompareIC(KexStr, KexNames[K]))
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
std::wstring TSessionData::GetKexList() const
{
    std::wstring Result;
    for (size_t Index = 0; Index < KEX_COUNT; Index++)
    {
        Result += std::wstring(Index ? L"," : L"") + KexNames[GetKex(Index)];
    }
    return Result;
}
//---------------------------------------------------------------------
void TSessionData::SetPublicKeyFile(const std::wstring value)
{
    if (FPublicKeyFile != value)
    {
        FPublicKeyFile = StripPathQuotes(value);
        Modify();
    }
}
//---------------------------------------------------------------------
void TSessionData::SetReturnVar(const std::wstring value)
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
nb::TDateTime TSessionData::GetTimeoutDT()
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
    assert(GetFSProtocol() != -1 && GetFSProtocol() < FSPROTOCOL_COUNT);
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
void TSessionData::SetProtocolStr(const std::wstring value)
{
    FProtocol = ptRaw;
    for (size_t Index = 0; Index < PROTOCOL_COUNT; Index++)
    {
        if (::AnsiCompareIC(value, ProtocolNames[Index]) == 0)
        {
            FProtocol = static_cast<TProtocol>(Index);
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
void TSessionData::SetPingIntervalDT(nb::TDateTime value)
{
    unsigned int hour, min, sec, msec;

    value.DecodeTime(hour, min, sec, msec);
    SetPingInterval((static_cast<size_t>(hour))*60*60 + (static_cast<int>(min))*60 + sec);
}
//---------------------------------------------------------------------------
nb::TDateTime TSessionData::GetPingIntervalDT() const
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
void TSessionData::SetCodePage(const std::wstring value)
{
    SET_SESSION_PROPERTY(CodePage);
}
//---------------------------------------------------------------------------
void TSessionData::SetRekeyData(const std::wstring value)
{
    SET_SESSION_PROPERTY(RekeyData);
}
//---------------------------------------------------------------------------
void TSessionData::SetRekeyTime(unsigned int value)
{
    SET_SESSION_PROPERTY(RekeyTime);
}
//---------------------------------------------------------------------
void TSessionData::AdjustHostName(std::wstring &hostName, const std::wstring prefix)
{
    if (::LowerCase(hostName.substr(0, prefix.size())) == prefix)
    {
        hostName.erase(0, prefix.size());
        hostName = ::ReplaceStrAll(hostName, L"/", L"_");
    }
}
//---------------------------------------------------------------------
void TSessionData::RemoveProtocolPrefix(std::wstring &hostName)
{
    AdjustHostName(hostName, L"scp://");
    AdjustHostName(hostName, L"sftp://");
    AdjustHostName(hostName, L"ftp://");
    AdjustHostName(hostName, L"ftps://");
    AdjustHostName(hostName, L"http://");
    AdjustHostName(hostName, L"https://");
}

//---------------------------------------------------------------------
std::wstring TSessionData::GetDefaultSessionName()
{
    std::wstring hostName = GetHostName();
    std::wstring userName = GetUserName();
    // DEBUG_PRINTF(L"hostName = %s", hostName.c_str());
    RemoveProtocolPrefix(hostName);
    if (!hostName.empty() && !userName.empty())
    {
        return FORMAT(L"%s@%s", userName.c_str(), hostName.c_str());
    }
    else if (!hostName.empty())
    {
        return hostName;
    }
    else
    {
        return L"session";
    }
}
//---------------------------------------------------------------------
bool TSessionData::HasSessionName()
{
    return (!GetName().empty() && (GetName() != DefaultName));
}
//---------------------------------------------------------------------
std::wstring TSessionData::GetSessionName()
{
    // DEBUG_PRINTF(L"Name = %s", GetName().c_str());
    std::wstring Result;
    if (HasSessionName())
    {
        Result = GetName();
        if (GetHidden())
        {
            Result = Result.substr(TNamedObjectList::HiddenPrefix.size(), Result.size() - TNamedObjectList::HiddenPrefix.size());
        }
    }
    else
    {
        Result = GetDefaultSessionName();
    }
    // DEBUG_PRINTF(L"Result = %s", Result.c_str());
    return Result;
}
//---------------------------------------------------------------------
std::wstring TSessionData::GetSessionUrl()
{
    std::wstring Url;
    // DEBUG_PRINTF(L"Name = %s", GetName().c_str());
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
void TSessionData::SetTimeDifference(nb::TDateTime value)
{
    SET_SESSION_PROPERTY(TimeDifference);
}
//---------------------------------------------------------------------
void TSessionData::SetLocalDirectory(const std::wstring value)
{
    SET_SESSION_PROPERTY(LocalDirectory);
}
//---------------------------------------------------------------------
void TSessionData::SetRemoteDirectory(const std::wstring value)
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
void TSessionData::SetRecycleBinPath(const std::wstring value)
{
    SET_SESSION_PROPERTY(RecycleBinPath);
}
//---------------------------------------------------------------------------
void TSessionData::SetPostLoginCommands(const std::wstring value)
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
void TSessionData::SetProxyHost(const std::wstring value)
{
    SET_SESSION_PROPERTY(ProxyHost);
}
//---------------------------------------------------------------------
void TSessionData::SetProxyPort(int value)
{
    SET_SESSION_PROPERTY(ProxyPort);
}
//---------------------------------------------------------------------
void TSessionData::SetProxyUsername(const std::wstring value)
{
    SET_SESSION_PROPERTY(ProxyUsername);
}
//---------------------------------------------------------------------
void TSessionData::SetProxyPassword(const std::wstring val)
{
    std::wstring value = val;
    value = EncryptPassword(value, GetProxyUsername() + GetProxyHost());
    SET_SESSION_PROPERTY(ProxyPassword);
}
//---------------------------------------------------------------------
std::wstring TSessionData::GetProxyPassword() const
{
    return DecryptPassword(FProxyPassword, GetProxyUsername() + GetProxyHost());
}
//---------------------------------------------------------------------
void TSessionData::SetProxyTelnetCommand(const std::wstring value)
{
    SET_SESSION_PROPERTY(ProxyTelnetCommand);
}
//---------------------------------------------------------------------
void TSessionData::SetProxyLocalCommand(const std::wstring value)
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
void TSessionData::SetFtpProxyLogonType(size_t value)
{
    SET_SESSION_PROPERTY(FtpProxyLogonType);
}
//---------------------------------------------------------------------
void TSessionData::SetBug(TSshBug Bug, TAutoSwitch value)
{
    assert(Bug != -1 && Bug < LENOF(FBugs));
    SET_SESSION_PROPERTY(Bugs[Bug]);
}
//---------------------------------------------------------------------
TAutoSwitch TSessionData::GetBug(TSshBug Bug) const
{
    assert(Bug != -1 && Bug < LENOF(FBugs));
    return FBugs[Bug];
}
//---------------------------------------------------------------------
void TSessionData::SetCustomParam1(const std::wstring value)
{
    SET_SESSION_PROPERTY(CustomParam1);
}
//---------------------------------------------------------------------
void TSessionData::SetCustomParam2(const std::wstring value)
{
    SET_SESSION_PROPERTY(CustomParam2);
}
//---------------------------------------------------------------------
void TSessionData::SetSFTPDownloadQueue(size_t value)
{
    SET_SESSION_PROPERTY(SFTPDownloadQueue);
}
//---------------------------------------------------------------------
void TSessionData::SetSFTPUploadQueue(size_t value)
{
    SET_SESSION_PROPERTY(SFTPUploadQueue);
}
//---------------------------------------------------------------------
void TSessionData::SetSFTPListingQueue(size_t value)
{
    SET_SESSION_PROPERTY(SFTPListingQueue);
}
//---------------------------------------------------------------------
void TSessionData::SetSFTPMaxVersion(size_t value)
{
    SET_SESSION_PROPERTY(SFTPMaxVersion);
}
//---------------------------------------------------------------------
void TSessionData::SetSFTPMinPacketSize(size_t value)
{
    SET_SESSION_PROPERTY(SFTPMinPacketSize);
}
//---------------------------------------------------------------------
void TSessionData::SetSFTPMaxPacketSize(size_t value)
{
    SET_SESSION_PROPERTY(SFTPMaxPacketSize);
}
//---------------------------------------------------------------------
void TSessionData::SetSFTPBug(TSftpBug Bug, TAutoSwitch value)
{
    assert(Bug != -1 && Bug < LENOF(FSFTPBugs));
    SET_SESSION_PROPERTY(SFTPBugs[Bug]);
}
//---------------------------------------------------------------------
TAutoSwitch TSessionData::GetSFTPBug(TSftpBug Bug) const
{
    assert(Bug != -1 && Bug < LENOF(FSFTPBugs));
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
void TSessionData::SetTunnelHostName(const std::wstring val)
{
    std::wstring value = val;
    if (FTunnelHostName != value)
    {
        // HostName is key for password encryption
        const std::wstring XTunnelPassword = GetTunnelPassword();

        size_t P = ::LastDelimiter(value, L"@");
        if (P != std::wstring::npos)
        {
            SetTunnelUserName(value.substr(0, P));
            value = value.substr(P + 1, value.size() - P);
        }
        FTunnelHostName = value;
        Modify();

        SetTunnelPassword(XTunnelPassword);
        if (!XTunnelPassword.empty())
        {
            memset(const_cast<wchar_t *>(XTunnelPassword.c_str()), 0, XTunnelPassword.size() * sizeof(wchar_t));
        }
    }
}
//---------------------------------------------------------------------
void TSessionData::SetTunnelPortNumber(size_t value)
{
    SET_SESSION_PROPERTY(TunnelPortNumber);
}
//---------------------------------------------------------------------
void TSessionData::SetTunnelUserName(const std::wstring val)
{
    // TunnelUserName is key for password encryption
    std::wstring value = val;
    std::wstring XTunnelPassword = GetTunnelPassword();
    SET_SESSION_PROPERTY(TunnelUserName);
    SetTunnelPassword(XTunnelPassword);
    if (!XTunnelPassword.empty())
    {
        memset(const_cast<wchar_t *>(XTunnelPassword.c_str()), 0, XTunnelPassword.size() * sizeof(wchar_t));
    }
}
//---------------------------------------------------------------------
void TSessionData::SetTunnelPassword(const std::wstring val)
{
    std::wstring value = val;
    value = EncryptPassword(value, GetTunnelUserName() + GetTunnelHostName());
    SET_SESSION_PROPERTY(TunnelPassword);
}
//---------------------------------------------------------------------
std::wstring TSessionData::GetTunnelPassword()
{
    return DecryptPassword(FTunnelPassword, GetTunnelUserName() + GetTunnelHostName());
}
//---------------------------------------------------------------------
void TSessionData::SetTunnelPublicKeyFile(const std::wstring val)
{
    std::wstring value = val;
    if (FTunnelPublicKeyFile != val)
    {
        FTunnelPublicKeyFile = StripPathQuotes(val);
        Modify();
    }
}
//---------------------------------------------------------------------
void TSessionData::SetTunnelLocalPortNumber(size_t value)
{
    SET_SESSION_PROPERTY(TunnelLocalPortNumber);
}
//---------------------------------------------------------------------
bool TSessionData::GetTunnelAutoassignLocalPortNumber()
{
    return (FTunnelLocalPortNumber <= 0);
}
//---------------------------------------------------------------------
void TSessionData::SetTunnelPortFwd(const std::wstring value)
{
    // DEBUG_PRINTF(L"value = %s", value.c_str());
    SET_SESSION_PROPERTY(TunnelPortFwd);
}
//---------------------------------------------------------------------
void TSessionData::SetFtpPasvMode(bool value)
{
    SET_SESSION_PROPERTY(FtpPasvMode);
}
//---------------------------------------------------------------------
void TSessionData::SetFtpAllowEmptyPassword(bool value)
{
    SET_SESSION_PROPERTY(FtpAllowEmptyPassword);
}
//---------------------------------------------------------------------
void TSessionData::SetFtpEncryption(TFtpEncryptionSwitch value)
{
    SET_SESSION_PROPERTY(FtpEncryption);
}
//---------------------------------------------------------------------
void TSessionData::SetFtpForcePasvIp(bool value)
{
    SET_SESSION_PROPERTY(FtpForcePasvIp);
}
//---------------------------------------------------------------------
void TSessionData::SetFtpAccount(const std::wstring value)
{
    SET_SESSION_PROPERTY(FtpAccount);
}
//---------------------------------------------------------------------
void TSessionData::SetFtpPingInterval(size_t value)
{
    SET_SESSION_PROPERTY(FtpPingInterval);
}
//---------------------------------------------------------------------------
nb::TDateTime TSessionData::GetFtpPingIntervalDT()
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
void TSessionData::SetHostKey(const std::wstring value)
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
    std::wstring Result;
    if (HasSessionName())
    {
        Result = GetName();
        size_t P = ::LastDelimiter(Result, L"/");
        if (P != std::wstring::npos)
        {
            Result.erase(0, P);
        }
    }
    else
    {
        Result = GetDefaultSessionName();
    }
    return Result;
}
//---------------------------------------------------------------------
unsigned int TSessionData::GetCodePageAsNumber() const
{
    return ::GetCodePageAsNumber(GetCodePage());
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
void TStoredSessionList::Load(THierarchicalStorage *Storage,
                              bool AsModified, bool UseDefaults)
{
    nb::TStringList *SubKeys = new nb::TStringList();
    nb::TList *Loaded = new nb::TList;
    {
        BOOST_SCOPE_EXIT ( (&SubKeys) (&Loaded) )
        {
            delete SubKeys;
            delete Loaded;
        } BOOST_SCOPE_EXIT_END
        Storage->GetSubKeyNames(SubKeys);
        // DEBUG_PRINTF(L"SubKeys->GetCount = %d", SubKeys->GetCount());
        for (size_t Index = 0; Index < SubKeys->GetCount(); Index++)
        {
            TSessionData *SessionData;
            const std::wstring SessionName = SubKeys->GetString(Index);
            // DEBUG_PRINTF(L"SessionName = %s", SessionName.c_str());
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
            for (size_t Index = 0; Index < nb::TObjectList::GetCount(); Index++)
            {
                if (Loaded->IndexOf(GetItem(Index)) == -1)
                {
                    Delete(Index);
                    Index--;
                }
            }
        }
    }
}
//---------------------------------------------------------------------
void TStoredSessionList::Load(const std::wstring aKey, bool UseDefaults)
{
    // DEBUG_PRINTF(L"aKey = %s", aKey.c_str());
    TRegistryStorage *Storage = new TRegistryStorage(aKey);
    {
        BOOST_SCOPE_EXIT ( (&Storage) )
        {
            delete Storage;
        } BOOST_SCOPE_EXIT_END
        if (Storage->OpenRootKey(false)) { Load(Storage, false, UseDefaults); }
    }
}
//---------------------------------------------------------------------
void TStoredSessionList::Load()
{
    THierarchicalStorage *Storage = Configuration->CreateStorage();
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
}
//---------------------------------------------------------------------
void TStoredSessionList::DoSave(THierarchicalStorage *Storage,
                                TSessionData *Data, bool All, bool RecryptPasswordOnly,
                                TSessionData *FactoryDefaults)
{
    // DEBUG_PRINTF(L"begin: All = %d, Data->GetModified = %d", All, Data->GetModified());
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
    // DEBUG_PRINTF(L"end");
}
//---------------------------------------------------------------------
void TStoredSessionList::DoSave(THierarchicalStorage *Storage,
                                bool All, bool RecryptPasswordOnly)
{
    // DEBUG_PRINTF(L"begin");
    TSessionData *FactoryDefaults = new TSessionData(L"");
    {
        BOOST_SCOPE_EXIT ( (&FactoryDefaults) )
        {
            delete FactoryDefaults;
        } BOOST_SCOPE_EXIT_END
        DoSave(Storage, FDefaultSettings, All, RecryptPasswordOnly, FactoryDefaults);
        for (size_t Index = 0; Index < GetCount() + GetHiddenCount(); Index++)
        {
            TSessionData *SessionData = static_cast<TSessionData *>(GetItem(Index));
            DoSave(Storage, SessionData, All, RecryptPasswordOnly, FactoryDefaults);
        }
    }
    // DEBUG_PRINTF(L"end");
}
//---------------------------------------------------------------------
void TStoredSessionList::Save(THierarchicalStorage *Storage, bool All)
{
    DoSave(Storage, All, false);
}
//---------------------------------------------------------------------
void TStoredSessionList::DoSave(bool All, bool Explicit, bool RecryptPasswordOnly)
{
    // DEBUG_PRINTF(L"begin")
    THierarchicalStorage *Storage = Configuration->CreateStorage();
    {
        BOOST_SCOPE_EXIT ( (&Storage) )
        {
            delete Storage;
        } BOOST_SCOPE_EXIT_END
        Storage->SetAccessMode(smReadWrite);
        Storage->SetExplicit(Explicit);
        // DEBUG_PRINTF(L"Configuration->GetStoredSessionsSubKey = %s", Configuration->GetStoredSessionsSubKey().c_str());
        if (Storage->OpenSubKey(Configuration->GetStoredSessionsSubKey(), true))
        {
            DoSave(Storage, All, RecryptPasswordOnly);
        }
    }
    // DEBUG_PRINTF(L"end");
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
    for (size_t Index = 0; Index < GetCount() + GetHiddenCount(); Index++)
    {
        (static_cast<TSessionData *>(GetItem(Index))->SetModified(false));
    }
}
//---------------------------------------------------------------------
void TStoredSessionList::Export(const std::wstring FileName)
{
    nb::Error(SNotImplemented, 3003);
    /*
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
    */
}
//---------------------------------------------------------------------
void TStoredSessionList::SelectAll(bool Select)
{
    for (size_t Index = 0; Index < GetCount(); Index++)
    {
        GetSession(Index)->SetSelected(Select);
    }
}
//---------------------------------------------------------------------
void TStoredSessionList::Import(TStoredSessionList *From,
                                bool OnlySelected)
{
    for (size_t Index = 0; Index < From->GetCount(); Index++)
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
(TStoredSessionList *Dest, bool SSHOnly)
{
    for (size_t Index = 0; Index < GetCount(); Index++)
    {
        GetSession(Index)->SetSelected(
            (!SSHOnly || (GetSession(Index)->GetProtocol() == ptSSH)) &&
            !Dest->FindByName(GetSession(Index)->GetName()));
    }
}
//---------------------------------------------------------------------
void TStoredSessionList::Cleanup()
{
    try
    {
        if (Configuration->GetStorage() == stRegistry) { Clear(); }
        TRegistryStorage *Storage = new TRegistryStorage(Configuration->GetRegistryStorageKey());
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
    }
    catch (const std::exception &E)
    {
        throw ExtException(FMTLOAD(CLEANUP_SESSIONS_ERROR), &E);
    }
}
//---------------------------------------------------------------------------
size_t TStoredSessionList::IndexOf(TSessionData *Data)
{
    for (size_t Index = 0; Index < GetCount(); Index++)
    {
        if (Data == GetSession(Index))
        {
            return Index;
        }
    }
    return NPOS;
}
//---------------------------------------------------------------------------
TSessionData *TStoredSessionList::NewSession(
    const std::wstring SessionName, TSessionData *Session)
{
    TSessionData *DuplicateSession = static_cast<TSessionData *>(FindByName(SessionName));
    // DEBUG_PRINTF(L"DuplicateSession = %x", DuplicateSession);
    if (!DuplicateSession)
    {
        DuplicateSession = new TSessionData(L"");
        DuplicateSession->Assign(Session);
        DuplicateSession->SetName(SessionName);
        // make sure, that new stored session is saved to registry
        DuplicateSession->SetModified(true);
        Add(DuplicateSession);
        // DEBUG_PRINTF(L"Count = %d", GetCount());
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
void TStoredSessionList::SetDefaultSettings(TSessionData *value)
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
void TStoredSessionList::ImportHostKeys(const std::wstring TargetKey,
                                        const std::wstring SourceKey, TStoredSessionList *Sessions,
                                        bool OnlySelected)
{
    TRegistryStorage *SourceStorage = NULL;
    TRegistryStorage *TargetStorage = NULL;
    nb::TStringList *KeyList = NULL;
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
        KeyList = new nb::TStringList();

        if (SourceStorage->OpenRootKey(false) &&
                TargetStorage->OpenRootKey(true))
        {
            SourceStorage->GetValueNames(KeyList);

            TSessionData *Session;
            std::wstring HostKeyName;
            assert(Sessions != NULL);
            for (size_t Index = 0; Index < Sessions->GetCount(); Index++)
            {
                Session = Sessions->GetSession(Index);
                if (!OnlySelected || Session->GetSelected())
                {
                    HostKeyName = PuttyMungeStr(FORMAT(L"@%d:%s", Session->GetPortNumber(), Session->GetHostName().c_str()));
                    std::wstring KeyName;
                    for (size_t KeyIndex = 0; KeyIndex < KeyList->GetCount(); KeyIndex++)
                    {
                        KeyName = KeyList->GetString(KeyIndex);
                        size_t P = KeyName.find(HostKeyName);
                        if ((P != std::wstring::npos) && (P == KeyName.size() - HostKeyName.size() + 1))
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
TSessionData *TStoredSessionList::ParseUrl(const std::wstring Url,
        TOptions *Options, bool &DefaultsOnly, std::wstring *FileName,
        bool *AProtocolDefined)
{
    TSessionData *Data = new TSessionData(L"");
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
