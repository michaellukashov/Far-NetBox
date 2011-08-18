//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include "SessionData.h"

#include "Common.h"
#include "Exceptions.h"
// #include "FileBuffer.h"
#include "CoreMain.h"
#include "TextsCore.h"
#include "PuttyIntf.h"
#include "RemoteFiles.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------
enum TProxyType { pxNone, pxHTTP, pxSocks, pxTelnet }; // 0.53b and older
const char * DefaultName = "Default Settings";
const char CipherNames[CIPHER_COUNT][10] = {"WARN", "3des", "blowfish", "aes", "des", "arcfour"};
const char KexNames[KEX_COUNT][20] = {"WARN", "dh-group1-sha1", "dh-group14-sha1", "dh-gex-sha1", "rsa" };
const char ProtocolNames[PROTOCOL_COUNT][10] = { "raw", "telnet", "rlogin", "ssh" };
const char SshProtList[][10] = {"1 only", "1", "2", "2 only"};
const char ProxyMethodList[][10] = {"none", "SOCKS4", "SOCKS5", "HTTP", "Telnet", "Cmd" };
const TCipher DefaultCipherList[CIPHER_COUNT] =
  { cipAES, cipBlowfish, cip3DES, cipWarn, cipArcfour, cipDES };
const TKex DefaultKexList[KEX_COUNT] =
  { kexDHGEx, kexDHGroup14, kexDHGroup1, kexRSA, kexWarn };
const char FSProtocolNames[FSPROTOCOL_COUNT][11] = { "SCP", "SFTP (SCP)", "SFTP", "", "", "FTP" };
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
TSessionData::TSessionData(wstring aName):
  TNamedObject(aName)
{
  Default();
  FModified = true;
}
//---------------------------------------------------------------------
void TSessionData::Default()
{
  HostName = "";
  PortNumber = SshPortNumber;
  UserName = "";
  Password = "";
  Passwordless = false;
  PingInterval = 30;
  // when changing default, update load/save logic
  PingType = ptOff;
  Timeout = 15;
  TryAgent = true;
  AgentFwd = false;
  AuthTIS = false;
  AuthKI = true;
  AuthKIPassword = true;
  AuthGSSAPI = false;
  GSSAPIFwdTGT = false;
  GSSAPIServerRealm = "";
  ChangeUsername = false;
  Compression = false;
  SshProt = ssh2;
  Ssh2DES = false;
  SshNoUserAuth = false;
  for (int Index = 0; Index < CIPHER_COUNT; Index++)
  {
    Cipher[Index] = DefaultCipherList[Index];
  }
  for (int Index = 0; Index < KEX_COUNT; Index++)
  {
    Kex[Index] = DefaultKexList[Index];
  }
  PublicKeyFile = "";
  FProtocol = ptSSH;
  TcpNoDelay = true;
  HostKey = "";

  ProxyMethod = pmNone;
  ProxyHost = "proxy";
  ProxyPort = 80;
  ProxyUsername = "";
  ProxyPassword = "";
  ProxyTelnetCommand = "connect %host %port\\n";
  ProxyLocalCommand = "";
  ProxyDNS = asAuto;
  ProxyLocalhost = false;

  for (int Index = 0; Index < LENOF(FBugs); Index++)
  {
    Bug[(TSshBug)Index] = asAuto;
  }

  Special = false;
  FSProtocol = fsSFTP;
  AddressFamily = afAuto;
  RekeyData = "1G";
  RekeyTime = 60;

  // FS common
  LocalDirectory = "";
  RemoteDirectory = "";
  UpdateDirectories = false;
  CacheDirectories = true;
  CacheDirectoryChanges = true;
  PreserveDirectoryChanges = true;
  LockInHome = false;
  ResolveSymlinks = true;
  DSTMode = dstmUnix;
  DeleteToRecycleBin = false;
  OverwrittenToRecycleBin = false;
  RecycleBinPath = "/tmp";
  Color = 0;
  PostLoginCommands = "";

  // SCP
  ReturnVar = "";
  LookupUserGroups = true;
  EOLType = eolLF;
  Shell = ""; //default shell
  ReturnVar = "";
  ClearAliases = true;
  UnsetNationalVars = true;
  ListingCommand = "ls -la";
  IgnoreLsWarnings = true;
  Scp1Compatibility = false;
  TimeDifference = 0;
  SCPLsFullTime = asAuto;
  NotUtf = asAuto;
  FtpListAll = asAuto;

  // SFTP
  SftpServer = "";
  SFTPDownloadQueue = 4;
  SFTPUploadQueue = 4;
  SFTPListingQueue = 2;
  SFTPMaxVersion = 5;
  SFTPMaxPacketSize = 0;

  for (int Index = 0; Index < LENOF(FSFTPBugs); Index++)
  {
    SFTPBug[(TSftpBug)Index] = asAuto;
  }

  Tunnel = false;
  TunnelHostName = "";
  TunnelPortNumber = SshPortNumber;
  TunnelUserName = "";
  TunnelPassword = "";
  TunnelPublicKeyFile = "";
  TunnelLocalPortNumber = 0;
  TunnelPortFwd = "";

  // FTP
  FtpPasvMode = true;
  FtpForcePasvIp = false;
  FtpAccount = "";
  FtpPingInterval = 30;
  FtpPingType = ptDummyCommand;
  Ftps = ftpsNone;

  FtpProxyLogonType = 0; // none

  CustomParam1 = "";
  CustomParam2 = "";

  Selected = false;
  FModified = false;
  FSource = ::ssNone;

  // add also to TSessionLog::AddStartupInfo()
}
//---------------------------------------------------------------------
void TSessionData::NonPersistant()
{
  UpdateDirectories = false;
  PreserveDirectoryChanges = false;
}
//---------------------------------------------------------------------
void TSessionData::Assign(TPersistent * Source)
{
  if (Source && Source->InheritsFrom(__classid(TSessionData)))
  {
    #define DUPL(P) P = ((TSessionData *)Source)->P
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
      DUPL(Bug[(TSshBug)Index]);
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
      DUPL(SFTPBug[(TSftpBug)Index]);
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
    FModified = ((TSessionData *)Source)->Modified;
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
  if (Storage->OpenSubKey(InternalStorageKey, false))
  {
    PortNumber = Storage->ReadInteger("PortNumber", PortNumber);
    UserName = Storage->ReadString("UserName", UserName);
    // must be loaded after UserName, because HostName may be in format user@host
    HostName = Storage->ReadString("HostName", HostName);

    if (!Configuration->DisablePasswordStoring)
    {
      if (Storage->ValueExists("PasswordPlain"))
      {
        Password = Storage->ReadString("PasswordPlain", Password);
        RewritePassword = true;
      }
      else
      {
        FPassword = Storage->ReadString("Password", FPassword);
      }
    }
    Passwordless = Storage->ReadBool("Passwordless", Passwordless);
    // Putty uses PingIntervalSecs
    int PingIntervalSecs = Storage->ReadInteger("PingIntervalSecs", -1);
    if (PingIntervalSecs < 0)
    {
      PingIntervalSecs = Storage->ReadInteger("PingIntervalSec", PingInterval%60);
    }
    PingInterval =
      Storage->ReadInteger("PingInterval", PingInterval/60)*60 +
      PingIntervalSecs;
    if (PingInterval == 0)
    {
      PingInterval = 30;
    }
    // PingType has not existed before 3.5, where PingInterval > 0 meant today's ptNullPacket
    // Since 3.5, until 4.1 PingType was stored unconditionally.
    // Since 4.1 PingType is stored when it is not ptOff (default) or
    // when PingInterval is stored.
    if (!Storage->ValueExists("PingType"))
    {
      if (Storage->ReadInteger("PingInterval", 0) > 0)
      {
        PingType = ptNullPacket;
      }
    }
    else
    {
      PingType = static_cast<TPingType>(Storage->ReadInteger("PingType", ptOff));
    }
    Timeout = Storage->ReadInteger("Timeout", Timeout);
    TryAgent = Storage->ReadBool("TryAgent", TryAgent);
    AgentFwd = Storage->ReadBool("AgentFwd", AgentFwd);
    AuthTIS = Storage->ReadBool("AuthTIS", AuthTIS);
    AuthKI = Storage->ReadBool("AuthKI", AuthKI);
    AuthKIPassword = Storage->ReadBool("AuthKIPassword", AuthKIPassword);
    // Continue to use setting keys of previous kerberos implementation (vaclav tomec),
    // but fallback to keys of other implementations (official putty and vintela quest putty),
    // to allow imports from all putty versions.
    // Both vaclav tomec and official putty use AuthGSSAPI
    AuthGSSAPI = Storage->ReadBool("AuthGSSAPI", Storage->ReadBool("AuthSSPI", AuthGSSAPI));
    GSSAPIFwdTGT = Storage->ReadBool("GSSAPIFwdTGT", Storage->ReadBool("GssapiFwd", Storage->ReadBool("SSPIFwdTGT", GSSAPIFwdTGT)));
    GSSAPIServerRealm = Storage->ReadString("GSSAPIServerRealm", Storage->ReadString("KerbPrincipal", GSSAPIServerRealm));
    ChangeUsername = Storage->ReadBool("ChangeUsername", ChangeUsername);
    Compression = Storage->ReadBool("Compression", Compression);
    SshProt = (TSshProt)Storage->ReadInteger("SshProt", SshProt);
    Ssh2DES = Storage->ReadBool("Ssh2DES", Ssh2DES);
    SshNoUserAuth = Storage->ReadBool("SshNoUserAuth", SshNoUserAuth);
    CipherList = Storage->ReadString("Cipher", CipherList);
    KexList = Storage->ReadString("KEX", KexList);
    PublicKeyFile = Storage->ReadString("PublicKeyFile", PublicKeyFile);
    AddressFamily = static_cast<TAddressFamily>
      (Storage->ReadInteger("AddressFamily", AddressFamily));
    RekeyData = Storage->ReadString("RekeyBytes", RekeyData);
    RekeyTime = Storage->ReadInteger("RekeyTime", RekeyTime);

    FSProtocol = (TFSProtocol)Storage->ReadInteger("FSProtocol", FSProtocol);
    LocalDirectory = Storage->ReadString("LocalDirectory", LocalDirectory);
    RemoteDirectory = Storage->ReadString("RemoteDirectory", RemoteDirectory);
    UpdateDirectories = Storage->ReadBool("UpdateDirectories", UpdateDirectories);
    CacheDirectories = Storage->ReadBool("CacheDirectories", CacheDirectories);
    CacheDirectoryChanges = Storage->ReadBool("CacheDirectoryChanges", CacheDirectoryChanges);
    PreserveDirectoryChanges = Storage->ReadBool("PreserveDirectoryChanges", PreserveDirectoryChanges);

    ResolveSymlinks = Storage->ReadBool("ResolveSymlinks", ResolveSymlinks);
    DSTMode = (TDSTMode)Storage->ReadInteger("ConsiderDST", DSTMode);
    LockInHome = Storage->ReadBool("LockInHome", LockInHome);
    Special = Storage->ReadBool("Special", Special);
    Shell = Storage->ReadString("Shell", Shell);
    ClearAliases = Storage->ReadBool("ClearAliases", ClearAliases);
    UnsetNationalVars = Storage->ReadBool("UnsetNationalVars", UnsetNationalVars);
    ListingCommand = Storage->ReadString("ListingCommand",
      Storage->ReadBool("AliasGroupList", false) ? wstring("ls -gla") : ListingCommand);
    IgnoreLsWarnings = Storage->ReadBool("IgnoreLsWarnings", IgnoreLsWarnings);
    SCPLsFullTime = TAutoSwitch(Storage->ReadInteger("SCPLsFullTime", SCPLsFullTime));
    FtpListAll = TAutoSwitch(Storage->ReadInteger("FtpListAll", FtpListAll));
    Scp1Compatibility = Storage->ReadBool("Scp1Compatibility", Scp1Compatibility);
    TimeDifference = Storage->ReadFloat("TimeDifference", TimeDifference);
    DeleteToRecycleBin = Storage->ReadBool("DeleteToRecycleBin", DeleteToRecycleBin);
    OverwrittenToRecycleBin = Storage->ReadBool("OverwrittenToRecycleBin", OverwrittenToRecycleBin);
    RecycleBinPath = Storage->ReadString("RecycleBinPath", RecycleBinPath);
    PostLoginCommands = Storage->ReadString("PostLoginCommands", PostLoginCommands);

    ReturnVar = Storage->ReadString("ReturnVar", ReturnVar);
    LookupUserGroups = Storage->ReadBool("LookupUserGroups", LookupUserGroups);
    EOLType = (TEOLType)Storage->ReadInteger("EOLType", EOLType);
    NotUtf = TAutoSwitch(Storage->ReadInteger("Utf", Storage->ReadInteger("SFTPUtfBug", NotUtf)));

    TcpNoDelay = Storage->ReadBool("TcpNoDelay", TcpNoDelay);

    ProxyMethod = (TProxyMethod)Storage->ReadInteger("ProxyMethod", -1);
    if (ProxyMethod < 0)
    {
      int ProxyType = Storage->ReadInteger("ProxyType", pxNone);
      int ProxySOCKSVersion;
      switch (ProxyType) {
        case pxHTTP:
          ProxyMethod = pmHTTP;
          break;
        case pxTelnet:
          ProxyMethod = pmTelnet;
          break;
        case pxSocks:
          ProxySOCKSVersion = Storage->ReadInteger("ProxySOCKSVersion", 5);
          ProxyMethod = ProxySOCKSVersion == 5 ? pmSocks5 : pmSocks4;
          break;
        default:
        case pxNone:
          ProxyMethod = pmNone;
          break;
      }
    }
    ProxyHost = Storage->ReadString("ProxyHost", ProxyHost);
    ProxyPort = Storage->ReadInteger("ProxyPort", ProxyPort);
    ProxyUsername = Storage->ReadString("ProxyUsername", ProxyUsername);
    if (Storage->ValueExists("ProxyPassword"))
    {
      // encrypt unencrypted password
      ProxyPassword = Storage->ReadString("ProxyPassword", "");
    }
    else
    {
      // load encrypted password
      FProxyPassword = Storage->ReadString("ProxyPasswordEnc", FProxyPassword);
    }
    if (ProxyMethod == pmCmd)
    {
      ProxyLocalCommand = Storage->ReadStringRaw("ProxyTelnetCommand", ProxyLocalCommand);
    }
    else
    {
      ProxyTelnetCommand = Storage->ReadStringRaw("ProxyTelnetCommand", ProxyTelnetCommand);
    }
    ProxyDNS = TAutoSwitch((Storage->ReadInteger("ProxyDNS", (ProxyDNS + 2) % 3) + 1) % 3);
    ProxyLocalhost = Storage->ReadBool("ProxyLocalhost", ProxyLocalhost);

    #define READ_BUG(BUG) \
      Bug[sb##BUG] = TAutoSwitch(2 - Storage->ReadInteger("Bug"#BUG, \
        2 - Bug[sb##BUG]));
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

    if ((Bug[sbHMAC2] == asAuto) &&
        Storage->ReadBool("BuggyMAC", false))
    {
        Bug[sbHMAC2] = asOn;
    }

    SftpServer = Storage->ReadString("SftpServer", SftpServer);
    #define READ_SFTP_BUG(BUG) \
      SFTPBug[sb##BUG] = TAutoSwitch(Storage->ReadInteger("SFTP" #BUG "Bug", SFTPBug[sb##BUG]));
    READ_SFTP_BUG(Symlink);
    READ_SFTP_BUG(SignedTS);
    #undef READ_SFTP_BUG

    SFTPMaxVersion = Storage->ReadInteger("SFTPMaxVersion", SFTPMaxVersion);
    SFTPMaxPacketSize = Storage->ReadInteger("SFTPMaxPacketSize", SFTPMaxPacketSize);

    Color = Storage->ReadInteger("Color", Color);

    ProtocolStr = Storage->ReadString("Protocol", ProtocolStr);

    Tunnel = Storage->ReadBool("Tunnel", Tunnel);
    TunnelPortNumber = Storage->ReadInteger("TunnelPortNumber", TunnelPortNumber);
    TunnelUserName = Storage->ReadString("TunnelUserName", TunnelUserName);
    // must be loaded after TunnelUserName,
    // because TunnelHostName may be in format user@host
    TunnelHostName = Storage->ReadString("TunnelHostName", TunnelHostName);
    if (!Configuration->DisablePasswordStoring)
    {
      if (Storage->ValueExists("TunnelPasswordPlain"))
      {
        TunnelPassword = Storage->ReadString("TunnelPasswordPlain", TunnelPassword);
        RewritePassword = true;
      }
      else
      {
        FTunnelPassword = Storage->ReadString("TunnelPassword", FTunnelPassword);
      }
    }
    TunnelPublicKeyFile = Storage->ReadString("TunnelPublicKeyFile", TunnelPublicKeyFile);
    TunnelLocalPortNumber = Storage->ReadInteger("TunnelLocalPortNumber", TunnelLocalPortNumber);

    // Ftp prefix
    FtpPasvMode = Storage->ReadBool("FtpPasvMode", FtpPasvMode);
    FtpForcePasvIp = Storage->ReadBool("FtpForcePasvIp", FtpForcePasvIp);
    FtpAccount = Storage->ReadString("FtpAccount", FtpAccount);
    FtpPingInterval = Storage->ReadInteger("FtpPingInterval", FtpPingInterval);
    FtpPingType = static_cast<TPingType>(Storage->ReadInteger("FtpPingType", FtpPingType));
    Ftps = static_cast<TFtps>(Storage->ReadInteger("Ftps", Ftps));

    FtpProxyLogonType = Storage->ReadInteger("FtpProxyLogonType", FtpProxyLogonType);

    CustomParam1 = Storage->ReadString("CustomParam1", CustomParam1);
    CustomParam2 = Storage->ReadString("CustomParam2", CustomParam2);

    Storage->CloseSubKey();
  };

  if (RewritePassword)
  {
    TStorageAccessMode AccessMode = Storage->AccessMode;
    Storage->AccessMode = smReadWrite;

    try
    {
      if (Storage->OpenSubKey(InternalStorageKey, true))
      {
        Storage->DeleteValue("PasswordPlain");
        if (!Password.empty())
        {
          Storage->WriteString("Password", FPassword);
        }
        Storage->DeleteValue("TunnelPasswordPlain");
        if (!TunnelPassword.empty())
        {
          Storage->WriteString("TunnelPassword", FTunnelPassword);
        }
        Storage->CloseSubKey();
      }
    }
    catch(...)
    {
      // ignore errors (like read-only INI file)
    }

    Storage->AccessMode = AccessMode;
  }

  FModified = false;
  FSource = ssStored;
}
//---------------------------------------------------------------------
void TSessionData::Save(THierarchicalStorage * Storage,
  bool PuttyExport, const TSessionData * Default)
{
  if (Storage->OpenSubKey(InternalStorageKey, true))
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

    WRITE_DATA(String, HostName);
    WRITE_DATA(int, PortNumber);
    WRITE_DATA(bool, Passwordless);
    WRITE_DATA_EX(int, "PingInterval", PingInterval / 60, );
    WRITE_DATA_EX(int, "PingIntervalSecs", PingInterval % 60, );
    Storage->DeleteValue("PingIntervalSec"); // obsolete
    // when PingInterval is stored always store PingType not to attempt to
    // deduce PingType from PingInterval (backward compatibility with pre 3.5)
    if (((Default != NULL) && (PingType != Default->PingType)) ||
        Storage->ValueExists("PingInterval"))
    {
      Storage->WriteInteger("PingType", PingType);
    }
    else
    {
      Storage->DeleteValue("PingType");
    }
    WRITE_DATA(int, Timeout);
    WRITE_DATA(bool, TryAgent);
    WRITE_DATA(bool, AgentFwd);
    WRITE_DATA(bool, AuthTIS);
    WRITE_DATA(bool, AuthKI);
    WRITE_DATA(bool, AuthKIPassword);

    WRITE_DATA(bool, AuthGSSAPI);
    WRITE_DATA(bool, GSSAPIFwdTGT);
    WRITE_DATA(String, GSSAPIServerRealm);
    Storage->DeleteValue("TryGSSKEX");
    Storage->DeleteValue("UserNameFromEnvironment");
    Storage->DeleteValue("GSSAPIServerChoosesUserName");
    Storage->DeleteValue("GSSAPITrustDNS");
    if (PuttyExport)
    {
      // duplicate kerberos setting with keys of the vintela quest putty
      WRITE_DATA_EX(bool, "AuthSSPI", AuthGSSAPI, );
      WRITE_DATA_EX(bool, "SSPIFwdTGT", GSSAPIFwdTGT, );
      WRITE_DATA_EX(String, "KerbPrincipal", GSSAPIServerRealm, );
      // duplicate kerberos setting with keys of the official putty
      WRITE_DATA_EX(bool, "GssapiFwd", GSSAPIFwdTGT, );
    }

    WRITE_DATA(bool, ChangeUsername);
    WRITE_DATA(bool, Compression);
    WRITE_DATA(int, SshProt);
    WRITE_DATA(bool, Ssh2DES);
    WRITE_DATA(bool, SshNoUserAuth);
    WRITE_DATA_EX(String, "Cipher", CipherList, );
    WRITE_DATA_EX(String, "KEX", KexList, );
    WRITE_DATA(int, AddressFamily);
    WRITE_DATA_EX(String, "RekeyBytes", RekeyData, );
    WRITE_DATA(int, RekeyTime);

    WRITE_DATA(bool, TcpNoDelay);

    if (PuttyExport)
    {
      WRITE_DATA(StringRaw, UserName);
      WRITE_DATA(StringRaw, PublicKeyFile);
    }
    else
    {
      WRITE_DATA(String, UserName);
      WRITE_DATA(String, PublicKeyFile);
      WRITE_DATA(int, FSProtocol);
      WRITE_DATA(String, LocalDirectory);
      WRITE_DATA(String, RemoteDirectory);
      WRITE_DATA(bool, UpdateDirectories);
      WRITE_DATA(bool, CacheDirectories);
      WRITE_DATA(bool, CacheDirectoryChanges);
      WRITE_DATA(bool, PreserveDirectoryChanges);

      WRITE_DATA(bool, ResolveSymlinks);
      WRITE_DATA_EX(int, "ConsiderDST", DSTMode, );
      WRITE_DATA(bool, LockInHome);
      // Special is never stored (if it would, login dialog must be modified not to
      // duplicate Special parameter when Special session is loaded and then stored
      // under different name)
      // WRITE_DATA(bool, Special);
      WRITE_DATA(String, Shell);
      WRITE_DATA(bool, ClearAliases);
      WRITE_DATA(bool, UnsetNationalVars);
      WRITE_DATA(String, ListingCommand);
      WRITE_DATA(bool, IgnoreLsWarnings);
      WRITE_DATA(int, SCPLsFullTime);
      WRITE_DATA(int, FtpListAll);
      WRITE_DATA(bool, Scp1Compatibility);
      WRITE_DATA(Float, TimeDifference);
      WRITE_DATA(bool, DeleteToRecycleBin);
      WRITE_DATA(bool, OverwrittenToRecycleBin);
      WRITE_DATA(String, RecycleBinPath);
      WRITE_DATA(String, PostLoginCommands);

      WRITE_DATA(String, ReturnVar);
      WRITE_DATA(bool, LookupUserGroups);
      WRITE_DATA(int, EOLType);
      Storage->DeleteValue("SFTPUtfBug");
      WRITE_DATA_EX(int, "Utf", NotUtf, );
    }

    WRITE_DATA(int, ProxyMethod);
    if (PuttyExport)
    {
      // support for Putty 0.53b and older
      int ProxyType;
      int ProxySOCKSVersion = 5;
      switch (ProxyMethod) {
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
      Storage->WriteInteger("ProxyType", ProxyType);
      Storage->WriteInteger("ProxySOCKSVersion", ProxySOCKSVersion);
    }
    else
    {
      Storage->DeleteValue("ProxyType");
      Storage->DeleteValue("ProxySOCKSVersion");
    }
    WRITE_DATA(String, ProxyHost);
    WRITE_DATA(int, ProxyPort);
    WRITE_DATA(String, ProxyUsername);
    if (ProxyMethod == pmCmd)
    {
      WRITE_DATA_EX(StringRaw, "ProxyTelnetCommand", ProxyLocalCommand, );
    }
    else
    {
      WRITE_DATA(StringRaw, ProxyTelnetCommand);
    }
    #define WRITE_DATA_CONV_FUNC(X) (((X) + 2) % 3)
    WRITE_DATA_CONV(int, "ProxyDNS", ProxyDNS);
    #undef WRITE_DATA_CONV_FUNC
    WRITE_DATA(bool, ProxyLocalhost);

    #define WRITE_DATA_CONV_FUNC(X) (2 - (X))
    #define WRITE_BUG(BUG) WRITE_DATA_CONV(int, "Bug" #BUG, Bug[sb##BUG]);
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

    Storage->DeleteValue("BuggyMAC");
    Storage->DeleteValue("AliasGroupList");

    if (PuttyExport)
    {
      WRITE_DATA_EX(String, "Protocol", ProtocolStr, );
    }

    if (!PuttyExport)
    {
      WRITE_DATA(String, SftpServer);

      #define WRITE_SFTP_BUG(BUG) WRITE_DATA_EX(int, "SFTP" #BUG "Bug", SFTPBug[sb##BUG], );
      WRITE_SFTP_BUG(Symlink);
      WRITE_SFTP_BUG(SignedTS);
      #undef WRITE_SFTP_BUG

      WRITE_DATA(int, SFTPMaxVersion);
      WRITE_DATA(int, SFTPMaxPacketSize);

      WRITE_DATA(int, Color);

      WRITE_DATA(bool, Tunnel);
      WRITE_DATA(String, TunnelHostName);
      WRITE_DATA(int, TunnelPortNumber);
      WRITE_DATA(String, TunnelUserName);
      WRITE_DATA(String, TunnelPublicKeyFile);
      WRITE_DATA(int, TunnelLocalPortNumber);

      WRITE_DATA(bool, FtpPasvMode);
      WRITE_DATA(bool, FtpForcePasvIp);
      WRITE_DATA(String, FtpAccount);
      WRITE_DATA(int, FtpPingInterval);
      WRITE_DATA(int, FtpPingType);
      WRITE_DATA(int, Ftps);

      WRITE_DATA(int, FtpProxyLogonType);

      WRITE_DATA(String, CustomParam1);
      WRITE_DATA(String, CustomParam2);
    }

    SavePasswords(Storage, PuttyExport);

    Storage->CloseSubKey();
  }
}
//---------------------------------------------------------------------
void TSessionData::SavePasswords(THierarchicalStorage * Storage, bool PuttyExport)
{
  if (!Configuration->DisablePasswordStoring && !PuttyExport && !FPassword.empty())
  {
    Storage->WriteString("Password", StronglyRecryptPassword(FPassword, UserName+HostName));
  }
  else
  {
    Storage->DeleteValue("Password");
  }
  Storage->DeleteValue("PasswordPlain");

  if (PuttyExport)
  {
    // save password unencrypted
    Storage->WriteString("ProxyPassword", ProxyPassword);
  }
  else
  {
    // save password encrypted
    if (!FProxyPassword.empty())
    {
      Storage->WriteString("ProxyPasswordEnc", StronglyRecryptPassword(FProxyPassword, ProxyUsername+ProxyHost));
    }
    else
    {
      Storage->DeleteValue("ProxyPasswordEnc");
    }
    Storage->DeleteValue("ProxyPassword");

    if (!Configuration->DisablePasswordStoring && !FTunnelPassword.empty())
    {
      Storage->WriteString("TunnelPassword", StronglyRecryptPassword(FTunnelPassword, TunnelUserName+TunnelHostName));
    }
    else
    {
      Storage->DeleteValue("TunnelPassword");
    }
  }
}
//---------------------------------------------------------------------
void TSessionData::RecryptPasswords()
{
  Password = Password;
  ProxyPassword = ProxyPassword;
  TunnelPassword = TunnelPassword;
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
wstring TSessionData::GetSource()
{
  switch (FSource)
  {
    case ::ssNone:
      return "Ad-Hoc session";

    case ssStored:
      return "Stored session";

    case ssStoredModified:
      return "Modified stored session";

    default:
      assert(false);
      return "";
  }
}
//---------------------------------------------------------------------
void TSessionData::SaveRecryptedPasswords(THierarchicalStorage * Storage)
{
  if (Storage->OpenSubKey(InternalStorageKey, true))
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
  try
  {
    Storage->Explicit = true;
    if (Storage->OpenSubKey(Configuration->StoredSessionsSubKey, false))
    {
      Storage->RecursiveDeleteSubKey(InternalStorageKey);
    }
  }
  catch(...)
  {
    delete Storage;
  }
}
//---------------------------------------------------------------------
bool TSessionData::ParseUrl(wstring Url, TOptions * Options,
  TStoredSessionList * StoredSessions, bool & DefaultsOnly, wstring * FileName,
  bool * AProtocolDefined)
{
  bool ProtocolDefined = false;
  bool PortNumberDefined = false;
  TFSProtocol AFSProtocol;
  int APortNumber;
  TFtps AFtps = ftpsNone;
  if (Url.SubString(1, 4).LowerCase() == "scp:")
  {
    AFSProtocol = fsSCPonly;
    APortNumber = SshPortNumber;
    Url.Delete(1, 4);
    ProtocolDefined = true;
  }
  else if (Url.SubString(1, 5).LowerCase() == "sftp:")
  {
    AFSProtocol = fsSFTPonly;
    APortNumber = SshPortNumber;
    Url.Delete(1, 5);
    ProtocolDefined = true;
  }
  else if (Url.SubString(1, 4).LowerCase() == "ftp:")
  {
    AFSProtocol = fsFTP;
    Ftps = ftpsNone;
    APortNumber = FtpPortNumber;
    Url.Delete(1, 4);
    ProtocolDefined = true;
  }
  else if (Url.SubString(1, 5).LowerCase() == "ftps:")
  {
    AFSProtocol = fsFTP;
    AFtps = ftpsImplicit;
    APortNumber = FtpsImplicitPortNumber;
    Url.Delete(1, 5);
    ProtocolDefined = true;
  }

  if (ProtocolDefined && (Url.SubString(1, 2) == "//"))
  {
    Url.Delete(1, 2);
  }

  if (AProtocolDefined != NULL)
  {
    *AProtocolDefined = ProtocolDefined;
  }

  if (!Url.empty())
  {
    wstring DecodedUrl = DecodeUrlChars(Url);
    // lookup stored session even if protocol was defined
    // (this allows setting for example default username for host
    // by creating stored session named by host)
    TSessionData * Data = NULL;
    for (int Index = 0; Index < StoredSessions->Count + StoredSessions->HiddenCount; Index++)
    {
      TSessionData * AData = (TSessionData *)StoredSessions->Items[Index];
      if (AnsiSameText(AData->Name, DecodedUrl) ||
          AnsiSameText(AData->Name + "/", DecodedUrl.SubString(1, AData->Name.Length() + 1)))
      {
        Data = AData;
        break;
      }
    }

    wstring ARemoteDirectory;

    if (Data != NULL)
    {
      DefaultsOnly = false;
      Assign(Data);
      int P = 1;
      while (!AnsiSameText(DecodeUrlChars(Url.SubString(1, P)), Data->Name))
      {
        P++;
        assert(P <= Url.Length());
      }
      ARemoteDirectory = Url.SubString(P + 1, Url.Length() - P);

      if (StoredSessions->GetIsHidden()(Data))
      {
        Data->Remove();
        StoredSessions->Remove(Data);
        // only modified, implicit
        StoredSessions->Save(false, false);
      }
    }
    else
    {
      Assign(StoredSessions->DefaultSettings);
      Name = "";

      int PSlash = Url.Pos("/");
      if (PSlash == 0)
      {
        PSlash = Url.Length() + 1;
      }

      wstring ConnectInfo = Url.SubString(1, PSlash - 1);

      int P = ConnectInfo.LastDelimiter("@");

      wstring UserInfo;
      wstring HostInfo;

      if (P > 0)
      {
        UserInfo = ConnectInfo.SubString(1, P - 1);
        HostInfo = ConnectInfo.SubString(P + 1, ConnectInfo.Length() - P);
      }
      else
      {
        HostInfo = ConnectInfo;
      }

      if ((HostInfo.Length() >= 2) && (HostInfo[1] == '[') && ((P = HostInfo.Pos("]")) > 0))
      {
        HostName = HostInfo.SubString(2, P - 2);
        HostInfo.Delete(1, P);
        if (!HostInfo.empty() && (HostInfo[1] == ':'))
        {
          HostInfo.Delete(1, 1);
        }
      }
      else
      {
        HostName = DecodeUrlChars(CutToChar(HostInfo, ':', true));
      }

      // expanded from ?: operator, as it caused strange "access violation" errors
      if (!HostInfo.empty())
      {
        PortNumber = StrToIntDef(DecodeUrlChars(HostInfo), -1);
        PortNumberDefined = true;
      }
      else if (ProtocolDefined)
      {
        PortNumber = APortNumber;
      }

      if (ProtocolDefined)
      {
        Ftps = AFtps;
      }

      bool PasswordSeparator = (UserInfo.Pos(':') != 0);
      UserName = DecodeUrlChars(CutToChar(UserInfo, ':', false));
      Password = DecodeUrlChars(UserInfo);
      Passwordless = Password.empty() && PasswordSeparator;

      ARemoteDirectory = Url.SubString(PSlash, Url.Length() - PSlash + 1);
    }

    if (!ARemoteDirectory.empty() && (ARemoteDirectory != "/"))
    {
      if ((ARemoteDirectory[ARemoteDirectory.Length()] != '/') &&
          (FileName != NULL))
      {
        *FileName = DecodeUrlChars(UnixExtractFileName(ARemoteDirectory));
        ARemoteDirectory = UnixExtractFilePath(ARemoteDirectory);
      }
      RemoteDirectory = DecodeUrlChars(ARemoteDirectory);
    }

    DefaultsOnly = false;
  }
  else
  {
    Assign(StoredSessions->DefaultSettings);

    DefaultsOnly = true;
  }

  if (ProtocolDefined)
  {
    FSProtocol = AFSProtocol;
  }

  if (Options != NULL)
  {
    // we deliberatelly do keep defaultonly to false, in presence of any option,
    // as the option should not make session "connectable"

    wstring Value;
    if (Options->FindSwitch("privatekey", Value))
    {
      PublicKeyFile = Value;
    }
    if (Options->FindSwitch("timeout", Value))
    {
      Timeout = StrToInt(Value);
    }
    if (Options->FindSwitch("hostkey", Value) ||
        Options->FindSwitch("certificate", Value))
    {
      HostKey = Value;
    }
    if (Options->FindSwitch("passive", Value))
    {
      FtpPasvMode = (StrToIntDef(Value, 1) != 0);
    }
    if (Options->FindSwitch("implicit", Value))
    {
      bool Enabled = (StrToIntDef(Value, 1) != 0);
      Ftps = Enabled ? ftpsImplicit : ftpsNone;
      if (!PortNumberDefined && Enabled)
      {
        PortNumber = FtpsImplicitPortNumber;
      }
    }
    if (Options->FindSwitch("explicitssl", Value))
    {
      bool Enabled = (StrToIntDef(Value, 1) != 0);
      Ftps = Enabled ? ftpsExplicitSsl : ftpsNone;
      if (!PortNumberDefined && Enabled)
      {
        PortNumber = FtpPortNumber;
      }
    }
    if (Options->FindSwitch("explicittls", Value))
    {
      bool Enabled = (StrToIntDef(Value, 1) != 0);
      Ftps = Enabled ? ftpsExplicitTls : ftpsNone;
      if (!PortNumberDefined && Enabled)
      {
        PortNumber = FtpPortNumber;
      }
    }
  }

  return true;
}
//---------------------------------------------------------------------
void TSessionData::ConfigureTunnel(int APortNumber)
{
  FOrigHostName = HostName;
  FOrigPortNumber = PortNumber;
  FOrigProxyMethod = ProxyMethod;

  HostName = "127.0.0.1";
  PortNumber = APortNumber;
  // proxy settings is used for tunnel
  ProxyMethod = pmNone;
}
//---------------------------------------------------------------------
void TSessionData::RollbackTunnel()
{
  HostName = FOrigHostName;
  PortNumber = FOrigPortNumber;
  ProxyMethod = FOrigProxyMethod;
}
//---------------------------------------------------------------------
void TSessionData::ExpandEnvironmentVariables()
{
  PublicKeyFile = ::ExpandEnvironmentVariables(PublicKeyFile);
}
//---------------------------------------------------------------------
void TSessionData::ValidatePath(const wstring Path)
{
  // noop
}
//---------------------------------------------------------------------
void TSessionData::ValidateName(const wstring Name)
{
  if (Name.LastDelimiter("/") > 0)
  {
    throw exception(FMTLOAD(ITEM_NAME_INVALID, (Name, "/")));
  }
}
//---------------------------------------------------------------------
wstring TSessionData::EncryptPassword(const wstring & Password, wstring Key)
{
  return Configuration->EncryptPassword(Password, Key);
}
//---------------------------------------------------------------------
wstring TSessionData::StronglyRecryptPassword(const wstring & Password, wstring Key)
{
  return Configuration->StronglyRecryptPassword(Password, Key);
}
//---------------------------------------------------------------------
wstring TSessionData::DecryptPassword(const wstring & Password, wstring Key)
{
  wstring Result;
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
wstring TSessionData::GetSessionKey()
{
  return FORMAT("%s@%s", (UserName, HostName));
}
//---------------------------------------------------------------------
wstring TSessionData::GetInternalStorageKey()
{
  if (Name.empty())
  {
    return SessionKey;
  }
  else
  {
    return Name;
  }
}
//---------------------------------------------------------------------
wstring TSessionData::GetStorageKey()
{
  return SessionName;
}
//---------------------------------------------------------------------
void TSessionData::SetHostName(wstring value)
{
  if (FHostName != value)
  {
    // HostName is key for password encryption
    wstring XPassword = Password;

    int P = value.LastDelimiter("@");
    if (P > 0)
    {
      UserName = value.SubString(1, P - 1);
      value = value.SubString(P + 1, value.Length() - P);
    }
    FHostName = value;
    Modify();

    Password = XPassword;
    if (!XPassword.empty())
    {
      XPassword.Unique();
      memset(XPassword.c_str(), 0, XPassword.Length());
    }
  }
}
//---------------------------------------------------------------------
void TSessionData::SetPortNumber(int value)
{
  SET_SESSION_PROPERTY(PortNumber);
}
//---------------------------------------------------------------------------
void TSessionData::SetShell(wstring value)
{
  SET_SESSION_PROPERTY(Shell);
}
//---------------------------------------------------------------------------
void TSessionData::SetSftpServer(wstring value)
{
  SET_SESSION_PROPERTY(SftpServer);
}
//---------------------------------------------------------------------------
void TSessionData::SetClearAliases(bool value)
{
  SET_SESSION_PROPERTY(ClearAliases);
}
//---------------------------------------------------------------------------
void TSessionData::SetListingCommand(wstring value)
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
void TSessionData::SetUserName(wstring value)
{
  // UserName is key for password encryption
  wstring XPassword = Password;
  SET_SESSION_PROPERTY(UserName);
  Password = XPassword;
  if (!XPassword.empty())
  {
    XPassword.Unique();
    memset(XPassword.c_str(), 0, XPassword.Length());
  }
}
//---------------------------------------------------------------------
void TSessionData::SetPassword(wstring value)
{
  if (!value.empty())
  {
    Passwordless = false;
  }
  value = EncryptPassword(value, UserName+HostName);
  SET_SESSION_PROPERTY(Password);
}
//---------------------------------------------------------------------
wstring TSessionData::GetPassword()
{
  return DecryptPassword(FPassword, UserName+HostName);
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
void TSessionData::SetGSSAPIServerRealm(wstring value)
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
wstring TSessionData::GetSshProtStr()
{
  return SshProtList[FSshProt];
}
//---------------------------------------------------------------------
bool TSessionData::GetUsesSsh()
{
  return (FSProtocol != fsFTP);
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
void TSessionData::SetCipherList(wstring value)
{
  bool Used[CIPHER_COUNT];
  for (int C = 0; C < CIPHER_COUNT; C++) Used[C] = false;

  wstring CipherStr;
  int Index = 0;
  while (!value.empty() && (Index < CIPHER_COUNT))
  {
    CipherStr = CutToChar(value, ',', true);
    for (int C = 0; C < CIPHER_COUNT; C++)
    {
      if (!CipherStr.AnsiCompareIC(CipherNames[C]))
      {
        Cipher[Index] = (TCipher)C;
        Used[C] = true;
        Index++;
        break;
      }
    }
  }

  for (int C = 0; C < CIPHER_COUNT && Index < CIPHER_COUNT; C++)
  {
    if (!Used[DefaultCipherList[C]]) Cipher[Index++] = DefaultCipherList[C];
  }
}
//---------------------------------------------------------------------
wstring TSessionData::GetCipherList() const
{
  wstring Result;
  for (int Index = 0; Index < CIPHER_COUNT; Index++)
  {
    Result += wstring(Index ? "," : "") + CipherNames[Cipher[Index]];
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
void TSessionData::SetKexList(wstring value)
{
  bool Used[KEX_COUNT];
  for (int K = 0; K < KEX_COUNT; K++) Used[K] = false;

  wstring KexStr;
  int Index = 0;
  while (!value.empty() && (Index < KEX_COUNT))
  {
    KexStr = CutToChar(value, ',', true);
    for (int K = 0; K < KEX_COUNT; K++)
    {
      if (!KexStr.AnsiCompareIC(KexNames[K]))
      {
        Kex[Index] = (TKex)K;
        Used[K] = true;
        Index++;
        break;
      }
    }
  }

  for (int K = 0; K < KEX_COUNT && Index < KEX_COUNT; K++)
  {
    if (!Used[DefaultKexList[K]]) Kex[Index++] = DefaultKexList[K];
  }
}
//---------------------------------------------------------------------
wstring TSessionData::GetKexList() const
{
  wstring Result;
  for (int Index = 0; Index < KEX_COUNT; Index++)
  {
    Result += wstring(Index ? "," : "") + KexNames[Kex[Index]];
  }
  return Result;
}
//---------------------------------------------------------------------
void TSessionData::SetPublicKeyFile(wstring value)
{
  if (FPublicKeyFile != value)
  {
    FPublicKeyFile = StripPathQuotes(value);
    Modify();
  }
}
//---------------------------------------------------------------------
void TSessionData::SetReturnVar(wstring value)
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
  return SecToDateTime(Timeout);
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
wstring TSessionData::GetFSProtocolStr()
{
  assert(FSProtocol >= 0 && FSProtocol < FSPROTOCOL_COUNT);
  return FSProtocolNames[FSProtocol];
}
//---------------------------------------------------------------------------
void TSessionData::SetDetectReturnVar(bool value)
{
  if (value != DetectReturnVar)
  {
    ReturnVar = value ? "" : "$?";
  }
}
//---------------------------------------------------------------------------
bool TSessionData::GetDetectReturnVar()
{
  return ReturnVar.empty();
}
//---------------------------------------------------------------------------
void TSessionData::SetDefaultShell(bool value)
{
  if (value != DefaultShell)
  {
    Shell = value ? "" : "/bin/bash";
  }
}
//---------------------------------------------------------------------------
bool TSessionData::GetDefaultShell()
{
  return Shell.empty();
}
//---------------------------------------------------------------------------
void TSessionData::SetProtocolStr(wstring value)
{
  FProtocol = ptRaw;
  for (int Index = 0; Index < PROTOCOL_COUNT; Index++)
  {
    if (value.AnsiCompareIC(ProtocolNames[Index]) == 0)
    {
      FProtocol = TProtocol(Index);
      break;
    }
  }
}
//---------------------------------------------------------------------
wstring TSessionData::GetProtocolStr() const
{
  return ProtocolNames[Protocol];
}
//---------------------------------------------------------------------
void TSessionData::SetPingIntervalDT(TDateTime value)
{
  unsigned short hour, min, sec, msec;

  value.DecodeTime(&hour, &min, &sec, &msec);
  PingInterval = ((int)hour)*60*60 + ((int)min)*60 + sec;
}
//---------------------------------------------------------------------------
TDateTime TSessionData::GetPingIntervalDT()
{
  return SecToDateTime(PingInterval);
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
void TSessionData::SetRekeyData(wstring value)
{
  SET_SESSION_PROPERTY(RekeyData);
}
//---------------------------------------------------------------------------
void TSessionData::SetRekeyTime(unsigned int value)
{
  SET_SESSION_PROPERTY(RekeyTime);
}
//---------------------------------------------------------------------
wstring TSessionData::GetDefaultSessionName()
{
  if (!HostName.empty() && !UserName.empty())
  {
    return FORMAT("%s@%s", (UserName, HostName));
  }
  else if (!HostName.empty())
  {
    return HostName;
  }
  else
  {
    return "session";
  }
}
//---------------------------------------------------------------------
wstring TSessionData::GetSessionName()
{
  if (!Name.empty() && !TNamedObjectList::IsHidden(this) &&
      (Name != DefaultName))
  {
    return Name;
  }
  else
  {
    return DefaultSessionName;
  }
}
//---------------------------------------------------------------------
wstring TSessionData::GetSessionUrl()
{
  wstring Url;
  if (!Name.empty() && !TNamedObjectList::IsHidden(this) &&
      (Name != DefaultName))
  {
    Url = Name;
  }
  else
  {
    switch (FSProtocol)
    {
      case fsSCPonly:
        Url = "scp://";
        break;

      default:
        assert(false);
        // fallback
      case fsSFTP:
      case fsSFTPonly:
        Url = "sftp://";
        break;

      case fsFTP:
        Url = "ftp://";
        break;
    }

    if (!HostName.empty() && !UserName.empty())
    {
      Url += FORMAT("%s@%s", (UserName, HostName));
    }
    else if (!HostName.empty())
    {
      Url += HostName;
    }
    else
    {
      Url = "";
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
void TSessionData::SetLocalDirectory(wstring value)
{
  SET_SESSION_PROPERTY(LocalDirectory);
}
//---------------------------------------------------------------------
void TSessionData::SetRemoteDirectory(wstring value)
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
void TSessionData::SetRecycleBinPath(wstring value)
{
  SET_SESSION_PROPERTY(RecycleBinPath);
}
//---------------------------------------------------------------------------
void TSessionData::SetPostLoginCommands(wstring value)
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
void TSessionData::SetProxyHost(wstring value)
{
  SET_SESSION_PROPERTY(ProxyHost);
}
//---------------------------------------------------------------------
void TSessionData::SetProxyPort(int value)
{
  SET_SESSION_PROPERTY(ProxyPort);
}
//---------------------------------------------------------------------
void TSessionData::SetProxyUsername(wstring value)
{
  SET_SESSION_PROPERTY(ProxyUsername);
}
//---------------------------------------------------------------------
void TSessionData::SetProxyPassword(wstring value)
{
  value = EncryptPassword(value, ProxyUsername+ProxyHost);
  SET_SESSION_PROPERTY(ProxyPassword);
}
//---------------------------------------------------------------------
wstring TSessionData::GetProxyPassword() const
{
  return DecryptPassword(FProxyPassword, ProxyUsername+ProxyHost);
}
//---------------------------------------------------------------------
void TSessionData::SetProxyTelnetCommand(wstring value)
{
  SET_SESSION_PROPERTY(ProxyTelnetCommand);
}
//---------------------------------------------------------------------
void TSessionData::SetProxyLocalCommand(wstring value)
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
void TSessionData::SetCustomParam1(wstring value)
{
  SET_SESSION_PROPERTY(CustomParam1);
}
//---------------------------------------------------------------------
void TSessionData::SetCustomParam2(wstring value)
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
void TSessionData::SetTunnelHostName(wstring value)
{
  if (FTunnelHostName != value)
  {
    // HostName is key for password encryption
    wstring XTunnelPassword = TunnelPassword;

    int P = value.LastDelimiter("@");
    if (P > 0)
    {
      TunnelUserName = value.SubString(1, P - 1);
      value = value.SubString(P + 1, value.Length() - P);
    }
    FTunnelHostName = value;
    Modify();

    TunnelPassword = XTunnelPassword;
    if (!XTunnelPassword.empty())
    {
      XTunnelPassword.Unique();
      memset(XTunnelPassword.c_str(), 0, XTunnelPassword.Length());
    }
  }
}
//---------------------------------------------------------------------
void TSessionData::SetTunnelPortNumber(int value)
{
  SET_SESSION_PROPERTY(TunnelPortNumber);
}
//---------------------------------------------------------------------
void TSessionData::SetTunnelUserName(wstring value)
{
  // TunnelUserName is key for password encryption
  wstring XTunnelPassword = TunnelPassword;
  SET_SESSION_PROPERTY(TunnelUserName);
  TunnelPassword = XTunnelPassword;
  if (!XTunnelPassword.empty())
  {
    XTunnelPassword.Unique();
    memset(XTunnelPassword.c_str(), 0, XTunnelPassword.Length());
  }
}
//---------------------------------------------------------------------
void TSessionData::SetTunnelPassword(wstring value)
{
  value = EncryptPassword(value, TunnelUserName+TunnelHostName);
  SET_SESSION_PROPERTY(TunnelPassword);
}
//---------------------------------------------------------------------
wstring TSessionData::GetTunnelPassword()
{
  return DecryptPassword(FTunnelPassword, TunnelUserName+TunnelHostName);
}
//---------------------------------------------------------------------
void TSessionData::SetTunnelPublicKeyFile(wstring value)
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
void TSessionData::SetTunnelPortFwd(wstring value)
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
void TSessionData::SetFtpAccount(wstring value)
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
  return SecToDateTime(FtpPingInterval);
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
void TSessionData::SetHostKey(wstring value)
{
  SET_SESSION_PROPERTY(HostKey);
}
//---------------------------------------------------------------------
wstring TSessionData::GetInfoTip()
{
  if (UsesSsh)
  {
    return FMTLOAD(SESSION_INFO_TIP,
        (HostName, UserName,
         (PublicKeyFile.empty() ? LoadStr(NO_STR) : LoadStr(YES_STR)),
         SshProtStr, FSProtocolStr));
  }
  else
  {
    return FMTLOAD(SESSION_INFO_TIP_NO_SSH,
      (HostName, UserName, FSProtocolStr));
  }
}
//---------------------------------------------------------------------
wstring TSessionData::GetLocalName()
{
  wstring Result = Name;
  int P = Result.LastDelimiter("/");
  if (P > 0)
  {
    Result.Delete(1, P);
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
  try
  {
    Storage->GetSubKeyNames(SubKeys);
    for (int Index = 0; Index < SubKeys->Count; Index++)
    {
      TSessionData *SessionData;
      wstring SessionName = SubKeys->Strings[Index];
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
            SessionData = new TSessionData("");
            if (UseDefaults)
            {
              SessionData->Assign(DefaultSettings);
            }
            SessionData->Name = SessionName;
            Add(SessionData);
          }
          Loaded->Add(SessionData);
          SessionData->Load(Storage);
          if (AsModified)
          {
            SessionData->Modified = true;
          }
        }
      }
    }

    if (!AsModified)
    {
      for (int Index = 0; Index < TObjectList::Count; Index++)
      {
        if (Loaded->IndexOf(Items[Index]) < 0)
        {
          Delete(Index);
          Index--;
        }
      }
    }
  }
  catch(...)
  {
    delete SubKeys;
    delete Loaded;
  }
}
//---------------------------------------------------------------------
void TStoredSessionList::Load(wstring aKey, bool UseDefaults)
{
  TRegistryStorage * Storage = new TRegistryStorage(aKey);
  try {
    if (Storage->OpenRootKey(false)) Load(Storage, false, UseDefaults);
  } catch(...) {
    delete Storage;
  }
}
//---------------------------------------------------------------------
void TStoredSessionList::Load()
{
  THierarchicalStorage * Storage = Configuration->CreateScpStorage(true);
  try {
    if (Storage->OpenSubKey(Configuration->StoredSessionsSubKey, false))
      Load(Storage);
  } catch(...) {
    delete Storage;
  }
}
//---------------------------------------------------------------------
void TStoredSessionList::DoSave(THierarchicalStorage * Storage,
  TSessionData * Data, bool All, bool RecryptPasswordOnly,
  TSessionData * FactoryDefaults)
{
  if (All || Data->Modified)
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
  bool All, bool RecryptPasswordOnly)
{
  TSessionData * FactoryDefaults = new TSessionData("");
  try
  {
    DoSave(Storage, FDefaultSettings, All, RecryptPasswordOnly, FactoryDefaults);
    for (int Index = 0; Index < Count+HiddenCount; Index++)
    {
      TSessionData * SessionData = (TSessionData *)Items[Index];
      DoSave(Storage, SessionData, All, RecryptPasswordOnly, FactoryDefaults);
    }
  }
  catch(...)
  {
    delete FactoryDefaults;
  }
}
//---------------------------------------------------------------------
void TStoredSessionList::Save(THierarchicalStorage * Storage, bool All)
{
  DoSave(Storage, All, false);
}
//---------------------------------------------------------------------
void TStoredSessionList::DoSave(bool All, bool Explicit, bool RecryptPasswordOnly)
{
  THierarchicalStorage * Storage = Configuration->CreateScpStorage(true);
  try
  {
    Storage->AccessMode = smReadWrite;
    Storage->Explicit = Explicit;
    if (Storage->OpenSubKey(Configuration->StoredSessionsSubKey, true))
    {
      DoSave(Storage, All, RecryptPasswordOnly);
    }
  }
  catch(...)
  {
    delete Storage;
  }

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
  FDefaultSettings->Modified = false;
  for (int Index = 0; Index < Count + HiddenCount; Index++)
  {
    ((TSessionData *)Items[Index])->Modified = false;
  }
}
//---------------------------------------------------------------------
void TStoredSessionList::Export(const wstring FileName)
{
  THierarchicalStorage * Storage = new TIniFileStorage(FileName);
  try
  {
    Storage->AccessMode = smReadWrite;
    if (Storage->OpenSubKey(Configuration->StoredSessionsSubKey, true))
    {
      Save(Storage, true);
    }
  }
  catch(...)
  {
    delete Storage;
  }
}
//---------------------------------------------------------------------
void TStoredSessionList::SelectAll(bool Select)
{
  for (int Index = 0; Index < Count; Index++)
    Sessions[Index]->Selected = Select;
}
//---------------------------------------------------------------------
void TStoredSessionList::Import(TStoredSessionList * From,
  bool OnlySelected)
{
  for (int Index = 0; Index < From->Count; Index++)
  {
    if (!OnlySelected || From->Sessions[Index]->Selected)
    {
      TSessionData *Session = new TSessionData("");
      Session->Assign(From->Sessions[Index]);
      Session->Modified = true;
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
  for (int Index = 0; Index < Count; Index++)
  {
    Sessions[Index]->Selected =
      (!SSHOnly || (Sessions[Index]->Protocol == ptSSH)) &&
      !Dest->FindByName(Sessions[Index]->Name);
  }
}
//---------------------------------------------------------------------
void TStoredSessionList::Cleanup()
{
  try {
    if (Configuration->Storage == stRegistry) Clear();
    TRegistryStorage * Storage = new TRegistryStorage(Configuration->RegistryStorageKey);
    try {
      Storage->AccessMode = smReadWrite;
      if (Storage->OpenRootKey(false))
        Storage->RecursiveDeleteSubKey(Configuration->StoredSessionsSubKey);
    } catch(...) {
      delete Storage;
    }
  } catch (exception &E) {
    throw ExtException(&E, CLEANUP_SESSIONS_ERROR);
  }
}
//---------------------------------------------------------------------------
int TStoredSessionList::IndexOf(TSessionData * Data)
{
  for (int Index = 0; Index < Count; Index++)
    if (Data == Sessions[Index]) return Index;
  return -1;
}
//---------------------------------------------------------------------------
TSessionData * TStoredSessionList::NewSession(
  wstring SessionName, TSessionData * Session)
{
  TSessionData * DuplicateSession = (TSessionData*)FindByName(SessionName);
  if (!DuplicateSession)
  {
    DuplicateSession = new TSessionData("");
    DuplicateSession->Assign(Session);
    DuplicateSession->Name = SessionName;
    // make sure, that new stored session is saved to registry
    DuplicateSession->Modified = true;
    Add(DuplicateSession);
  }
    else
  {
    DuplicateSession->Assign(Session);
    DuplicateSession->Name = SessionName;
    DuplicateSession->Modified = true;
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
void TStoredSessionList::ImportHostKeys(const wstring TargetKey,
  const wstring SourceKey, TStoredSessionList * Sessions,
  bool OnlySelected)
{
  TRegistryStorage * SourceStorage = NULL;
  TRegistryStorage * TargetStorage = NULL;
  TStringList * KeyList = NULL;
  try
  {
    SourceStorage = new TRegistryStorage(SourceKey);
    TargetStorage = new TRegistryStorage(TargetKey);
    TargetStorage->AccessMode = smReadWrite;
    KeyList = new TStringList();

    if (SourceStorage->OpenRootKey(false) &&
        TargetStorage->OpenRootKey(true))
    {
      SourceStorage->GetValueNames(KeyList);

      TSessionData * Session;
      wstring HostKeyName;
      assert(Sessions != NULL);
      for (int Index = 0; Index < Sessions->Count; Index++)
      {
        Session = Sessions->Sessions[Index];
        if (!OnlySelected || Session->Selected)
        {
          HostKeyName = PuttyMungeStr(FORMAT("@%d:%s", (Session->PortNumber, Session->HostName)));
          wstring KeyName;
          for (int KeyIndex = 0; KeyIndex < KeyList->Count; KeyIndex++)
          {
            KeyName = KeyList->Strings[KeyIndex];
            int P = KeyName.Pos(HostKeyName);
            if ((P > 0) && (P == KeyName.Length() - HostKeyName.Length() + 1))
            {
              TargetStorage->WriteStringRaw(KeyName,
                SourceStorage->ReadStringRaw(KeyName, ""));
            }
          }
        }
      }
    }
  }
  catch(...)
  {
    delete SourceStorage;
    delete TargetStorage;
    delete KeyList;
  }
}
//---------------------------------------------------------------------------
TSessionData * TStoredSessionList::ParseUrl(wstring Url,
  TOptions * Options, bool & DefaultsOnly, wstring * FileName,
  bool * AProtocolDefined)
{
  TSessionData * Data = new TSessionData("");
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
