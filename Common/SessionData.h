//---------------------------------------------------------------------------
#ifndef SessionDataH
#define SessionDataH

#include "Common.h"
#include "Option.h"
#include "FileBuffer.h"
#include "NamedObjs.h"
#include "HierarchicalStorage.h"
#include "Configuration.h"
//---------------------------------------------------------------------------
#define SET_SESSION_PROPERTY(Property) \
  if (F##Property != value) { F##Property = value; Modify(); }
//---------------------------------------------------------------------------
enum TCipher { cipWarn, cip3DES, cipBlowfish, cipAES, cipDES, cipArcfour };
#define CIPHER_COUNT (cipArcfour+1)
enum TProtocol { ptRaw, ptTelnet, ptRLogin, ptSSH };
#define PROTOCOL_COUNT (ptSSH+1)
// explicit values to skip obsoleted fsExternalSSH, fsExternalSFTP
enum TFSProtocol { fsSCPonly = 0, fsSFTP = 1, fsSFTPonly = 2, fsFTP = 5 };
#define FSPROTOCOL_COUNT (fsFTP+1)
enum TProxyMethod { pmNone, pmSocks4, pmSocks5, pmHTTP, pmTelnet, pmCmd };
enum TSshProt { ssh1only, ssh1, ssh2, ssh2only };
enum TKex { kexWarn, kexDHGroup1, kexDHGroup14, kexDHGEx, kexRSA };
#define KEX_COUNT (kexRSA+1)
enum TSshBug { sbIgnore1, sbPlainPW1, sbRSA1, sbHMAC2, sbDeriveKey2, sbRSAPad2,
  sbRekey2, sbPKSessID2, sbMaxPkt2 };
#define BUG_COUNT (sbMaxPkt2+1)
enum TSftpBug { sbSymlink, sbSignedTS };
#define SFTP_BUG_COUNT (sbSignedTS+1)
enum TPingType { ptOff, ptNullPacket, ptDummyCommand };
enum TAddressFamily { afAuto, afIPv4, afIPv6 };
enum TFtps { ftpsNone, ftpsImplicit, ftpsExplicitSsl, ftpsExplicitTls };
enum TSessionSource { ssNone, ssStored, ssStoredModified };
//---------------------------------------------------------------------------
extern const char CipherNames[CIPHER_COUNT][10];
extern const char KexNames[KEX_COUNT][20];
extern const char ProtocolNames[PROTOCOL_COUNT][10];
extern const char SshProtList[][10];
extern const char ProxyMethodList[][10];
extern const TCipher DefaultCipherList[CIPHER_COUNT];
extern const TKex DefaultKexList[KEX_COUNT];
extern const char FSProtocolNames[FSPROTOCOL_COUNT][11];
//---------------------------------------------------------------------------
class TStoredSessionList;
//---------------------------------------------------------------------------
class TSessionData : public TNamedObject
{
friend class TStoredSessionList;

private:
  wstring FHostName;
  int FPortNumber;
  wstring FUserName;
  wstring FPassword;
  bool FPasswordless;
  int FPingInterval;
  TPingType FPingType;
  bool FTryAgent;
  bool FAgentFwd;
  wstring FListingCommand;
  bool FAuthTIS;
  bool FAuthKI;
  bool FAuthKIPassword;
  bool FAuthGSSAPI;
  bool FGSSAPIFwdTGT; // not supported anymore
  wstring FGSSAPIServerRealm; // not supported anymore
  bool FChangeUsername;
  bool FCompression;
  TSshProt FSshProt;
  bool FSsh2DES;
  bool FSshNoUserAuth;
  TCipher FCiphers[CIPHER_COUNT];
  TKex FKex[KEX_COUNT];
  bool FClearAliases;
  TEOLType FEOLType;
  wstring FPublicKeyFile;
  TProtocol FProtocol;
  TFSProtocol FFSProtocol;
  bool FModified;
  wstring FLocalDirectory;
  wstring FRemoteDirectory;
  bool FLockInHome;
  bool FSpecial;
  bool FUpdateDirectories;
  bool FCacheDirectories;
  bool FCacheDirectoryChanges;
  bool FPreserveDirectoryChanges;
  bool FSelected;
  bool FLookupUserGroups;
  wstring FReturnVar;
  bool FScp1Compatibility;
  wstring FShell;
  wstring FSftpServer;
  int FTimeout;
  bool FUnsetNationalVars;
  bool FIgnoreLsWarnings;
  bool FTcpNoDelay;
  TProxyMethod FProxyMethod;
  wstring FProxyHost;
  int FProxyPort;
  wstring FProxyUsername;
  wstring FProxyPassword;
  wstring FProxyTelnetCommand;
  wstring FProxyLocalCommand;
  TAutoSwitch FProxyDNS;
  bool FProxyLocalhost;
  int FFtpProxyLogonType;
  TAutoSwitch FBugs[BUG_COUNT];
  wstring FCustomParam1;
  wstring FCustomParam2;
  bool FResolveSymlinks;
  TDateTime FTimeDifference;
  int FSFTPDownloadQueue;
  int FSFTPUploadQueue;
  int FSFTPListingQueue;
  int FSFTPMaxVersion;
  unsigned long FSFTPMaxPacketSize;
  TDSTMode FDSTMode;
  TAutoSwitch FSFTPBugs[SFTP_BUG_COUNT];
  bool FDeleteToRecycleBin;
  bool FOverwrittenToRecycleBin;
  wstring FRecycleBinPath;
  wstring FPostLoginCommands;
  TAutoSwitch FSCPLsFullTime;
  TAutoSwitch FFtpListAll;
  TAddressFamily FAddressFamily;
  wstring FRekeyData;
  unsigned int FRekeyTime;
  int FColor;
  bool FTunnel;
  wstring FTunnelHostName;
  int FTunnelPortNumber;
  wstring FTunnelUserName;
  wstring FTunnelPassword;
  wstring FTunnelPublicKeyFile;
  int FTunnelLocalPortNumber;
  wstring FTunnelPortFwd;
  bool FFtpPasvMode;
  bool FFtpForcePasvIp;
  wstring FFtpAccount;
  int FFtpPingInterval;
  TPingType FFtpPingType;
  TFtps FFtps;
  TAutoSwitch FNotUtf;
  wstring FHostKey;

  wstring FOrigHostName;
  int FOrigPortNumber;
  TProxyMethod FOrigProxyMethod;
  TSessionSource FSource;

  void SetHostName(wstring value);
  void SetPortNumber(int value);
  void SetUserName(wstring value);
  void SetPassword(wstring value);
  wstring GetPassword();
  void SetPasswordless(bool value);
  void SetPingInterval(int value);
  void SetTryAgent(bool value);
  void SetAgentFwd(bool value);
  void SetAuthTIS(bool value);
  void SetAuthKI(bool value);
  void SetAuthKIPassword(bool value);
  void SetAuthGSSAPI(bool value);
  void SetGSSAPIFwdTGT(bool value);
  void SetGSSAPIServerRealm(wstring value);
  void SetChangeUsername(bool value);
  void SetCompression(bool value);
  void SetSshProt(TSshProt value);
  void SetSsh2DES(bool value);
  void SetSshNoUserAuth(bool value);
  void SetCipher(int Index, TCipher value);
  TCipher GetCipher(int Index) const;
  void SetKex(int Index, TKex value);
  TKex GetKex(int Index) const;
  void SetPublicKeyFile(wstring value);

  void SetProtocolStr(wstring value);
  wstring GetProtocolStr() const;
  bool GetCanLogin();
  void SetPingIntervalDT(TDateTime value);
  TDateTime GetPingIntervalDT();
  TDateTime GetFtpPingIntervalDT();
  void SetTimeDifference(TDateTime value);
  void SetPingType(TPingType value);
  wstring GetSessionName();
  wstring GetDefaultSessionName();
  wstring GetSessionUrl();
  void SetProtocol(TProtocol value);
  void SetFSProtocol(TFSProtocol value);
  wstring GetFSProtocolStr();
  void SetLocalDirectory(wstring value);
  void SetRemoteDirectory(wstring value);
  void SetUpdateDirectories(bool value);
  void SetCacheDirectories(bool value);
  void SetCacheDirectoryChanges(bool value);
  void SetPreserveDirectoryChanges(bool value);
  void SetLockInHome(bool value);
  void SetSpecial(bool value);
  wstring GetInfoTip();
  bool GetDefaultShell();
  void SetDetectReturnVar(bool value);
  bool GetDetectReturnVar();
  void SetListingCommand(wstring value);
  void SetClearAliases(bool value);
  void SetDefaultShell(bool value);
  void SetEOLType(TEOLType value);
  void SetLookupUserGroups(bool value);
  void SetReturnVar(wstring value);
  void SetScp1Compatibility(bool value);
  void SetShell(wstring value);
  void SetSftpServer(wstring value);
  void SetTimeout(int value);
  void SetUnsetNationalVars(bool value);
  void SetIgnoreLsWarnings(bool value);
  void SetTcpNoDelay(bool value);
  wstring GetSshProtStr();
  bool GetUsesSsh();
  void SetCipherList(wstring value);
  wstring GetCipherList() const;
  void SetKexList(wstring value);
  wstring GetKexList() const;
  void SetProxyMethod(TProxyMethod value);
  void SetProxyHost(wstring value);
  void SetProxyPort(int value);
  void SetProxyUsername(wstring value);
  void SetProxyPassword(wstring value);
  void SetProxyTelnetCommand(wstring value);
  void SetProxyLocalCommand(wstring value);
  void SetProxyDNS(TAutoSwitch value);
  void SetProxyLocalhost(bool value);
  wstring GetProxyPassword() const;
  void SetFtpProxyLogonType(int value);
  void SetBug(TSshBug Bug, TAutoSwitch value);
  TAutoSwitch GetBug(TSshBug Bug) const;
  wstring GetSessionKey();
  void SetCustomParam1(wstring value);
  void SetCustomParam2(wstring value);
  void SetResolveSymlinks(bool value);
  void SetSFTPDownloadQueue(int value);
  void SetSFTPUploadQueue(int value);
  void SetSFTPListingQueue(int value);
  void SetSFTPMaxVersion(int value);
  void SetSFTPMaxPacketSize(unsigned long value);
  void SetSFTPBug(TSftpBug Bug, TAutoSwitch value);
  TAutoSwitch GetSFTPBug(TSftpBug Bug) const;
  void SetSCPLsFullTime(TAutoSwitch value);
  void SetFtpListAll(TAutoSwitch value);
  wstring GetStorageKey();
  void SetDSTMode(TDSTMode value);
  void SetDeleteToRecycleBin(bool value);
  void SetOverwrittenToRecycleBin(bool value);
  void SetRecycleBinPath(wstring value);
  void SetPostLoginCommands(wstring value);
  void SetAddressFamily(TAddressFamily value);
  void SetRekeyData(wstring value);
  void SetRekeyTime(unsigned int value);
  void SetColor(int value);
  void SetTunnel(bool value);
  void SetTunnelHostName(wstring value);
  void SetTunnelPortNumber(int value);
  void SetTunnelUserName(wstring value);
  void SetTunnelPassword(wstring value);
  wstring GetTunnelPassword();
  void SetTunnelPublicKeyFile(wstring value);
  void SetTunnelPortFwd(wstring value);
  void SetTunnelLocalPortNumber(int value);
  bool GetTunnelAutoassignLocalPortNumber();
  void SetFtpPasvMode(bool value);
  void SetFtpForcePasvIp(bool value);
  void SetFtpAccount(wstring value);
  void SetFtpPingInterval(int value);
  void SetFtpPingType(TPingType value);
  void SetFtps(TFtps value);
  void SetNotUtf(TAutoSwitch value);
  void SetHostKey(wstring value);
  TDateTime GetTimeoutDT();
  void SavePasswords(THierarchicalStorage * Storage, bool PuttyExport);
  wstring GetLocalName();
  void Modify();
  wstring GetSource();
  static wstring EncryptPassword(const wstring & Password, wstring Key);
  static wstring DecryptPassword(const wstring & Password, wstring Key);
  static wstring StronglyRecryptPassword(const wstring & Password, wstring Key);

  // __property wstring InternalStorageKey = { read = GetInternalStorageKey };
  wstring GetInternalStorageKey();

public:
  TSessionData(wstring aName);
  void Default();
  void NonPersistant();
  void Load(THierarchicalStorage * Storage);
  void Save(THierarchicalStorage * Storage, bool PuttyExport,
    const TSessionData * Default = NULL);
  void SaveRecryptedPasswords(THierarchicalStorage * Storage);
  void RecryptPasswords();
  bool HasAnyPassword();
  void Remove();
  virtual void Assign(TPersistent * Source);
  bool ParseUrl(wstring Url, TOptions * Options,
    TStoredSessionList * StoredSessions, bool & DefaultsOnly,
    wstring * FileName, bool * AProtocolDefined);
  bool ParseOptions(TOptions * Options);
  void ConfigureTunnel(int PortNumber);
  void RollbackTunnel();
  void ExpandEnvironmentVariables();
  static void ValidatePath(const wstring Path);
  static void ValidateName(const wstring Name);

  // __property wstring HostName  = { read=FHostName, write=SetHostName };
  wstring GetHostName() { return FHostName; }
  // __property int PortNumber  = { read=FPortNumber, write=SetPortNumber };
  int GetPortNumber() { return FPortNumber; }
  // __property wstring UserName  = { read=FUserName, write=SetUserName };
  wstring GetUserName() { return FUserName; }
  __property wstring Password  = { read=GetPassword, write=SetPassword };
  // __property bool Passwordless = { read=FPasswordless, write=SetPasswordless };
  bool GetPasswordless() { return FPasswordless; }
  // __property int PingInterval  = { read=FPingInterval, write=SetPingInterval };
  int GetPingInterval() { return FPingInterval; }
  // __property bool TryAgent  = { read=FTryAgent, write=SetTryAgent };
  bool GetTryAgent() { return FTryAgent; }
  // __property bool AgentFwd  = { read=FAgentFwd, write=SetAgentFwd };
  bool GetAgentFwd() { return FAgentFwd; }
  // __property wstring ListingCommand = { read = FListingCommand, write = SetListingCommand };
  wstring GetListingCommand() { return FListingCommand; }
  // __property bool AuthTIS  = { read=FAuthTIS, write=SetAuthTIS };
  bool GetAuthTIS() { return FAuthTIS; }
  // __property bool AuthKI  = { read=FAuthKI, write=SetAuthKI };
  bool GetAuthKI() { return FAuthKI; }
  // __property bool AuthKIPassword  = { read=FAuthKIPassword, write=SetAuthKIPassword };
  bool GetAuthKIPassword() { return FAuthKIPassword; }
  // __property bool AuthGSSAPI  = { read=FAuthGSSAPI, write=SetAuthGSSAPI };
  bool GetAuthGSSAPI() { return FAuthGSSAPI; }
  // __property bool GSSAPIFwdTGT = { read=FGSSAPIFwdTGT, write=SetGSSAPIFwdTGT };
  bool GetGSSAPIFwdTGT() { return FGSSAPIFwdTGT; }
  // __property wstring GSSAPIServerRealm = { read=FGSSAPIServerRealm, write=SetGSSAPIServerRealm };
  wstring GetGSSAPIServerRealm() { return FGSSAPIServerRealm; }
  // __property bool ChangeUsername  = { read=FChangeUsername, write=SetChangeUsername };
  bool GetChangeUsername() { return FChangeUsername; }
  // __property bool Compression  = { read=FCompression, write=SetCompression };
  bool GetCompression() { return FCompression; }
  // __property TSshProt SshProt  = { read=FSshProt, write=SetSshProt };
  TSshProt GetSshProt() { return FSshProt; }
  __property bool UsesSsh = { read = GetUsesSsh };
  // __property bool Ssh2DES  = { read=FSsh2DES, write=SetSsh2DES };
  bool GetSsh2DES() { return FSsh2DES; }
  // __property bool SshNoUserAuth  = { read=FSshNoUserAuth, write=SetSshNoUserAuth };
  bool GetSshNoUserAuth() { return FSshNoUserAuth; }
  __property TCipher Cipher[int Index] = { read=GetCipher, write=SetCipher };
  __property TKex Kex[int Index] = { read=GetKex, write=SetKex };
  // __property wstring PublicKeyFile  = { read=FPublicKeyFile, write=SetPublicKeyFile };
  wstring GetPublicKeyFile() { return FPublicKeyFile; }
  // __property TProtocol Protocol  = { read=FProtocol, write=SetProtocol };
  TProtocol GetProtocol() { return FProtocol; }
  __property wstring ProtocolStr  = { read=GetProtocolStr, write=SetProtocolStr };
  // __property TFSProtocol FSProtocol  = { read=FFSProtocol, write=SetFSProtocol  };
  TFSProtocol GetFSProtocol() { return FFSProtocol; }
  __property wstring FSProtocolStr  = { read=GetFSProtocolStr };
  // __property bool Modified  = { read=FModified, write=FModified };
  bool GetModified() { return FModified; }
  __property bool CanLogin  = { read=GetCanLogin };
  // __property bool ClearAliases = { read = FClearAliases, write = SetClearAliases };
  bool GetClearAliases() { return FClearAliases; }
  __property TDateTime PingIntervalDT = { read = GetPingIntervalDT, write = SetPingIntervalDT };
  // __property TDateTime TimeDifference = { read = FTimeDifference, write = SetTimeDifference };
  TDateTime GetTimeDifference() { return FTimeDifference; }
  // __property TPingType PingType = { read = FPingType, write = SetPingType };
  TPingType GetPingType() { return FPingType; }
  __property wstring SessionName  = { read=GetSessionName };
  __property wstring DefaultSessionName  = { read=GetDefaultSessionName };
  __property wstring SessionUrl  = { read=GetSessionUrl };
  // __property wstring LocalDirectory  = { read=FLocalDirectory, write=SetLocalDirectory };
  wstring GetLocalDirectory() { return FLocalDirectory; }
  // __property wstring RemoteDirectory  = { read=FRemoteDirectory, write=SetRemoteDirectory };
  wstring GetRemoteDirectory() { return FRemoteDirectory; }
  // __property bool UpdateDirectories = { read=FUpdateDirectories, write=SetUpdateDirectories };
  bool GetUpdateDirectories() { return FUpdateDirectories; }
  // __property bool CacheDirectories = { read=FCacheDirectories, write=SetCacheDirectories };
  bool GetCacheDirectories() { return FCacheDirectories; }
  // __property bool CacheDirectoryChanges = { read=FCacheDirectoryChanges, write=SetCacheDirectoryChanges };
  bool GetCacheDirectoryChanges() { return FCacheDirectoryChanges; }
  // __property bool PreserveDirectoryChanges = { read=FPreserveDirectoryChanges, write=SetPreserveDirectoryChanges };
  bool GetPreserveDirectoryChanges() { return FPreserveDirectoryChanges; }
  // __property bool LockInHome = { read=FLockInHome, write=SetLockInHome };
  bool GetLockInHome() { return FLockInHome; }
  // __property bool Special = { read=FSpecial, write=SetSpecial };
  bool GetSpecial() { return FSpecial; }
  // __property bool Selected  = { read=FSelected, write=FSelected };
  bool GetSelected() { return FSelected; }
  __property wstring InfoTip  = { read=GetInfoTip };
  __property bool DefaultShell = { read = GetDefaultShell, write = SetDefaultShell };
  __property bool DetectReturnVar = { read = GetDetectReturnVar, write = SetDetectReturnVar };
  // __property TEOLType EOLType = { read = FEOLType, write = SetEOLType };
  TEOLType GetEOLType() { return FEOLType; }
  // __property bool LookupUserGroups = { read = FLookupUserGroups, write = SetLookupUserGroups };
  bool GetLookupUserGroups() { return FLookupUserGroups; }
  // __property wstring ReturnVar = { read = FReturnVar, write = SetReturnVar };
  wstring GetReturnVar() { return FReturnVar; }
  // __property bool Scp1Compatibility = { read = FScp1Compatibility, write = SetScp1Compatibility };
  bool GetScp1Compatibility() { return FScp1Compatibility; }
  // __property wstring Shell = { read = FShell, write = SetShell };
  wstring GetShell() { return FShell; }
  // __property wstring SftpServer = { read = FSftpServer, write = SetSftpServer };
  wstring GetSftpServer() { return FSftpServer; }
  // __property int Timeout = { read = FTimeout, write = SetTimeout };
  int GetTimeout() { return FTimeout; }
  __property TDateTime TimeoutDT = { read = GetTimeoutDT };
  // __property bool UnsetNationalVars = { read = FUnsetNationalVars, write = SetUnsetNationalVars };
  bool GetUnsetNationalVars() { return FUnsetNationalVars; }
  // __property bool IgnoreLsWarnings  = { read=FIgnoreLsWarnings, write=SetIgnoreLsWarnings };
  bool GetIgnoreLsWarnings() { return FIgnoreLsWarnings; }
  // __property bool TcpNoDelay  = { read=FTcpNoDelay, write=SetTcpNoDelay };
  bool GetTcpNoDelay() { return FTcpNoDelay; }
  __property wstring SshProtStr  = { read=GetSshProtStr };
  __property wstring CipherList  = { read=GetCipherList, write=SetCipherList };
  __property wstring KexList  = { read=GetKexList, write=SetKexList };
  // __property TProxyMethod ProxyMethod  = { read=FProxyMethod, write=SetProxyMethod };
  TProxyMethod GetProxyMethod() { return FProxyMethod; }
  // __property wstring ProxyHost  = { read=FProxyHost, write=SetProxyHost };
  wstring GetProxyHost() { return FProxyHost; }
  // __property int ProxyPort  = { read=FProxyPort, write=SetProxyPort };
  int GetProxyPort() { return FProxyPort; }
  // __property wstring ProxyUsername  = { read=FProxyUsername, write=SetProxyUsername };
  wstring GetProxyUsername() { return FProxyUsername; }
  __property wstring ProxyPassword  = { read=GetProxyPassword, write=SetProxyPassword };
  // __property wstring ProxyTelnetCommand  = { read=FProxyTelnetCommand, write=SetProxyTelnetCommand };
  wstring GetProxyTelnetCommand() { return FProxyTelnetCommand; }
  // __property wstring ProxyLocalCommand  = { read=FProxyLocalCommand, write=SetProxyLocalCommand };
  wstring GetProxyLocalCommand() { return FProxyLocalCommand; }
  // __property TAutoSwitch ProxyDNS  = { read=FProxyDNS, write=SetProxyDNS };
  TAutoSwitch GetProxyDNS() { return FProxyDNS; }
  // __property bool ProxyLocalhost  = { read=FProxyLocalhost, write=SetProxyLocalhost };
  bool GetProxyLocalhost() { return FProxyLocalhost; }
  // __property int FtpProxyLogonType  = { read=FFtpProxyLogonType, write=SetFtpProxyLogonType };
  int GetFtpProxyLogonType() { return FFtpProxyLogonType; }
  __property TAutoSwitch Bug[TSshBug Bug]  = { read=GetBug, write=SetBug };
  // __property wstring CustomParam1 = { read = FCustomParam1, write = SetCustomParam1 };
  wstring GetCustomParam1() { return FCustomParam1; }
  // __property wstring CustomParam2 = { read = FCustomParam2, write = SetCustomParam2 };
  wstring GetCustomParam2() { return FCustomParam2; }
  __property wstring SessionKey = { read = GetSessionKey };
  // __property bool ResolveSymlinks = { read = FResolveSymlinks, write = SetResolveSymlinks };
  bool GetResolveSymlinks() { return FResolveSymlinks; }
  // __property int SFTPDownloadQueue = { read = FSFTPDownloadQueue, write = SetSFTPDownloadQueue };
  int SFTGetPDownloadQueue = { read = FSFTPDownloadQueue, write = SetSFTPDownloadQueue };
  // __property int SFTPUploadQueue = { read = FSFTPUploadQueue, write = SetSFTPUploadQueue };
  int GetSFTPUploadQueue() { return FSFTPUploadQueue; }
  // __property int SFTPListingQueue = { read = FSFTPListingQueue, write = SetSFTPListingQueue };
  int GetSFTPListingQueue() { return FSFTPListingQueue; }
  // __property int SFTPMaxVersion = { read = FSFTPMaxVersion, write = SetSFTPMaxVersion };
  int GetSFTPMaxVersion() { return FSFTPMaxVersion; }
  // __property unsigned long SFTPMaxPacketSize = { read = FSFTPMaxPacketSize, write = SetSFTPMaxPacketSize };
  unsigned long GetSFTPMaxPacketSize() { return FSFTPMaxPacketSize; }
  __property TAutoSwitch SFTPBug[TSftpBug Bug]  = { read=GetSFTPBug, write=SetSFTPBug };
  // __property TAutoSwitch SCPLsFullTime = { read = FSCPLsFullTime, write = SetSCPLsFullTime };
  TAutoSwitch GetSCPLsFullTime() { return FSCPLsFullTime; }
  // __property TAutoSwitch FtpListAll = { read = FFtpListAll, write = SetFtpListAll };
  TAutoSwitch GetFtpListAll() { return FFtpListAll; }
  // __property TDSTMode DSTMode = { read = FDSTMode, write = SetDSTMode };
  TDSTMode GetDSTMode() { return FDSTMode; }
  // __property bool DeleteToRecycleBin = { read = FDeleteToRecycleBin, write = SetDeleteToRecycleBin };
  bool GetDeleteToRecycleBin() { return FDeleteToRecycleBin; }
  // __property bool OverwrittenToRecycleBin = { read = FOverwrittenToRecycleBin, write = SetOverwrittenToRecycleBin };
  bool GetOverwrittenToRecycleBin() { return FOverwrittenToRecycleBin; }
  // __property wstring RecycleBinPath = { read = FRecycleBinPath, write = SetRecycleBinPath };
  wstring GetRecycleBinPath() { return FRecycleBinPath; }
  // __property wstring PostLoginCommands = { read = FPostLoginCommands, write = SetPostLoginCommands };
  wstring GetPostLoginCommands() { return FPostLoginCommands; }
  // __property TAddressFamily AddressFamily = { read = FAddressFamily, write = SetAddressFamily };
  TAddressFamily GetAddressFamily() { return FAddressFamily; }
  // __property wstring RekeyData = { read = FRekeyData, write = SetRekeyData };
  wstring GetRekeyData() { return FRekeyData; }
  // __property unsigned int RekeyTime = { read = FRekeyTime, write = SetRekeyTime };
  unsigned int GetRekeyTime() { return FRekeyTime; }
  // __property int Color = { read = FColor, write = SetColor };
  int GetColor() { return FColor; }
  // __property bool Tunnel = { read = FTunnel, write = SetTunnel };
  bool GetTunnel() { return FTunnel; }
  // __property wstring TunnelHostName = { read = FTunnelHostName, write = SetTunnelHostName };
  wstring GetTunnelHostName() { return FTunnelHostName; }
  // __property int TunnelPortNumber = { read = FTunnelPortNumber, write = SetTunnelPortNumber };
  int GetTunnelPortNumber() { return FTunnelPortNumber; }
  // __property wstring TunnelUserName = { read = FTunnelUserName, write = SetTunnelUserName };
  wstring GetTunnelUserName() { return FTunnelUserName; }
  __property wstring TunnelPassword = { read = GetTunnelPassword, write = SetTunnelPassword };
  // __property wstring TunnelPublicKeyFile = { read = FTunnelPublicKeyFile, write = SetTunnelPublicKeyFile };
  wstring GetTunnelPublicKeyFile() { return FTunnelPublicKeyFile; }
  __property bool TunnelAutoassignLocalPortNumber = { read = GetTunnelAutoassignLocalPortNumber };
  // __property int TunnelLocalPortNumber = { read = FTunnelLocalPortNumber, write = SetTunnelLocalPortNumber };
  int GetTunnelLocalPortNumber() { return FTunnelLocalPortNumber; }
  // __property wstring TunnelPortFwd = { read = FTunnelPortFwd, write = SetTunnelPortFwd };
  Getwstring TunnelPortFwd = { read = FTunnelPortFwd, write = SetTunnelPortFwd };
  // __property bool FtpPasvMode = { read = FFtpPasvMode, write = SetFtpPasvMode };
  bool GetFtpPasvMode() { return FFtpPasvMode; }
  // __property bool FtpForcePasvIp = { read = FFtpForcePasvIp, write = SetFtpForcePasvIp };
  bool GetFtpForcePasvIp() { return FFtpForcePasvIp; }
  // __property wstring FtpAccount = { read = FFtpAccount, write = SetFtpAccount };
  wstring GetFtpAccount() { return FFtpAccount; }
  // __property int FtpPingInterval  = { read=FFtpPingInterval, write=SetFtpPingInterval };
  int GetFtpPingInterval() { return FFtpPingInterval; }
  __property TDateTime FtpPingIntervalDT  = { read=GetFtpPingIntervalDT };
  // __property TPingType FtpPingType = { read = FFtpPingType, write = SetFtpPingType };
  TPingType GetFtpPingType() { return FFtpPingType; }
  // __property TFtps Ftps = { read = FFtps, write = SetFtps };
  TFtps GetFtps() { return FFtps; }
  // __property TAutoSwitch NotUtf = { read = FNotUtf, write = SetNotUtf };
  TAutoSwitch GetNotUtf() { return FNotUtf; }
  // __property wstring HostKey = { read = FHostKey, write = SetHostKey };
  wstring GetHostKey() { return FHostKey; }
  __property wstring StorageKey = { read = GetStorageKey };
  // __property wstring OrigHostName = { read = FOrigHostName };
  wstring GetOrigHostName() { return FOrigHostName; }
  // __property int OrigPortNumber = { read = FOrigPortNumber };
  int GetOrigPortNumber() { return FOrigPortNumber; }
  __property wstring LocalName = { read = GetLocalName };
  __property wstring Source = { read = GetSource };
};
//---------------------------------------------------------------------------
class TStoredSessionList : public TNamedObjectList
{
public:
  TStoredSessionList(bool aReadOnly = false);
  void Load(wstring aKey, bool UseDefaults);
  void Load();
  void Save(bool All, bool Explicit);
  void Saved();
  void Export(const wstring FileName);
  void Load(THierarchicalStorage * Storage, bool AsModified = false,
    bool UseDefaults = false);
  void Save(THierarchicalStorage * Storage, bool All = false);
  void SelectAll(bool Select);
  void Import(TStoredSessionList * From, bool OnlySelected);
  void RecryptPasswords();
  TSessionData * AtSession(int Index)
    { return (TSessionData*)AtObject(Index); }
  void SelectSessionsToImport(TStoredSessionList * Dest, bool SSHOnly);
  void Cleanup();
  int IndexOf(TSessionData * Data);
  TSessionData * NewSession(wstring SessionName, TSessionData * Session);
  TSessionData * ParseUrl(wstring Url, TOptions * Options, bool & DefaultsOnly,
    wstring * FileName = NULL, bool * ProtocolDefined = NULL);
  virtual ~TStoredSessionList();
  __property TSessionData * Sessions[int Index]  = { read=AtSession };
  __property TSessionData * DefaultSettings  = { read=FDefaultSettings, write=SetDefaultSettings };

  static void ImportHostKeys(const wstring TargetKey,
    const wstring SourceKey, TStoredSessionList * Sessions,
    bool OnlySelected);

private:
  TSessionData * FDefaultSettings;
  bool FReadOnly;
  void SetDefaultSettings(TSessionData * value);
  void DoSave(THierarchicalStorage * Storage, bool All, bool RecryptPasswordOnly);
  void DoSave(bool All, bool Explicit, bool RecryptPasswordOnly);
  void DoSave(THierarchicalStorage * Storage,
    TSessionData * Data, bool All, bool RecryptPasswordOnly,
    TSessionData * FactoryDefaults);
};
//---------------------------------------------------------------------------
#endif
