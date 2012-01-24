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
enum TFSProtocol { fsSCPonly = 0, fsSFTP = 1, fsSFTPonly = 2, fsFTP = 5, fsFTPS = 6, fsHTTP = 7, fsHTTPS = 8 };
enum TLoginType { ltAnonymous = 0, ltNormal = 1 };
#define FSPROTOCOL_COUNT (fsHTTPS+1)
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
extern const wchar_t CipherNames[CIPHER_COUNT][10];
extern const wchar_t KexNames[KEX_COUNT][20];
extern const wchar_t ProtocolNames[PROTOCOL_COUNT][10];
extern const wchar_t SshProtList[][10];
extern const wchar_t ProxyMethodList[][10];
extern const TCipher DefaultCipherList[CIPHER_COUNT];
extern const TKex DefaultKexList[KEX_COUNT];
extern const wchar_t FSProtocolNames[FSPROTOCOL_COUNT][15];
extern const std::wstring CONST_LOGIN_ANONYMOUS;
//---------------------------------------------------------------------------
class TStoredSessionList;
//---------------------------------------------------------------------------
class TSessionData : public TNamedObject
{
friend class TStoredSessionList;

private:
  std::wstring FHostName;
  int FPortNumber;
  std::wstring FUserName;
  std::wstring FPassword;
  bool FPasswordless;
  int FPingInterval;
  TPingType FPingType;
  bool FTryAgent;
  bool FAgentFwd;
  std::wstring FListingCommand;
  bool FAuthTIS;
  bool FAuthKI;
  bool FAuthKIPassword;
  bool FAuthGSSAPI;
  bool FGSSAPIFwdTGT; // not supported anymore
  std::wstring FGSSAPIServerRealm; // not supported anymore
  bool FChangeUsername;
  bool FCompression;
  TSshProt FSshProt;
  bool FSsh2DES;
  bool FSshNoUserAuth;
  TCipher FCiphers[CIPHER_COUNT];
  TKex FKex[KEX_COUNT];
  bool FClearAliases;
  TEOLType FEOLType;
  std::wstring FPublicKeyFile;
  TProtocol FProtocol;
  TFSProtocol FFSProtocol;
  bool FModified;
  std::wstring FLocalDirectory;
  std::wstring FRemoteDirectory;
  bool FLockInHome;
  bool FSpecial;
  bool FUpdateDirectories;
  bool FCacheDirectories;
  bool FCacheDirectoryChanges;
  bool FPreserveDirectoryChanges;
  bool FSelected;
  bool FLookupUserGroups;
  std::wstring FReturnVar;
  bool FScp1Compatibility;
  std::wstring FShell;
  std::wstring FSftpServer;
  int FTimeout;
  bool FUnsetNationalVars;
  bool FIgnoreLsWarnings;
  bool FTcpNoDelay;
  TProxyMethod FProxyMethod;
  std::wstring FProxyHost;
  int FProxyPort;
  std::wstring FProxyUsername;
  std::wstring FProxyPassword;
  std::wstring FProxyTelnetCommand;
  std::wstring FProxyLocalCommand;
  TAutoSwitch FProxyDNS;
  bool FProxyLocalhost;
  int FFtpProxyLogonType;
  TAutoSwitch FBugs[BUG_COUNT];
  std::wstring FCustomParam1;
  std::wstring FCustomParam2;
  bool FResolveSymlinks;
  nb::TDateTime FTimeDifference;
  int FSFTPDownloadQueue;
  int FSFTPUploadQueue;
  int FSFTPListingQueue;
  int FSFTPMaxVersion;
  unsigned long FSFTPMinPacketSize;
  unsigned long FSFTPMaxPacketSize;
  TDSTMode FDSTMode;
  TAutoSwitch FSFTPBugs[SFTP_BUG_COUNT];
  bool FDeleteToRecycleBin;
  bool FOverwrittenToRecycleBin;
  std::wstring FRecycleBinPath;
  std::wstring FPostLoginCommands;
  TAutoSwitch FSCPLsFullTime;
  TAutoSwitch FFtpListAll;
  TAddressFamily FAddressFamily;
  std::wstring FRekeyData;
  unsigned int FRekeyTime;
  int FColor;
  bool FTunnel;
  std::wstring FTunnelHostName;
  int FTunnelPortNumber;
  std::wstring FTunnelUserName;
  std::wstring FTunnelPassword;
  std::wstring FTunnelPublicKeyFile;
  size_t FTunnelLocalPortNumber;
  std::wstring FTunnelPortFwd;
  bool FFtpPasvMode;
  bool FFtpForcePasvIp;
  bool FFtpAllowEmptyPassword;
  TLoginType FLoginType;
  std::wstring FFtpAccount;
  int FFtpPingInterval;
  TPingType FFtpPingType;
  TFtps FFtps;
  TAutoSwitch FNotUtf;
  std::wstring FHostKey;

  std::wstring FOrigHostName;
  int FOrigPortNumber;
  TProxyMethod FOrigProxyMethod;
  TSessionSource FSource;

  void SavePasswords(THierarchicalStorage * Storage, bool PuttyExport);
  void Modify();
  static std::wstring EncryptPassword(const std::wstring Password, const std::wstring Key);
  static std::wstring DecryptPassword(const std::wstring Password, const std::wstring Key);
  static std::wstring StronglyRecryptPassword(const std::wstring Password, const std::wstring Key);

  // __property std::wstring InternalStorageKey = { read = GetInternalStorageKey };
  std::wstring GetInternalStorageKey();

public:
  TSessionData(const std::wstring aName);
  void Default();
  void NonPersistant();
  void Load(THierarchicalStorage * Storage);
  void Save(THierarchicalStorage * Storage, bool PuttyExport,
    const TSessionData * Default = NULL);
  void SaveRecryptedPasswords(THierarchicalStorage * Storage);
  void RecryptPasswords();
  bool HasAnyPassword();
  void Remove();
  virtual void Assign(nb::TPersistent * Source);
  bool ParseUrl(const std::wstring Url, TOptions * Options,
    TStoredSessionList * StoredSessions, bool & DefaultsOnly,
    std::wstring *FileName, bool *AProtocolDefined);
  bool ParseOptions(TOptions * Options);
  void ConfigureTunnel(int PortNumber);
  void RollbackTunnel();
  void ExpandEnvironmentVariables();
  static void ValidatePath(const std::wstring Path);
  static void ValidateName(const std::wstring Name);

  // __property std::wstring HostName  = { read=FHostName, write=SetHostName };
  std::wstring GetHostName() const { return FHostName; }
  // __property int PortNumber  = { read=FPortNumber, write=SetPortNumber };
  int GetPortNumber() const { return FPortNumber; }
  TLoginType GetLoginType() const;
  void SetLoginType(TLoginType value);
  // __property std::wstring UserName  = { read=FUserName, write=SetUserName };
  std::wstring GetUserName() const { return FUserName; }
  // __property std::wstring Password  = { read=GetPassword, write=SetPassword };
  void SetPassword(const std::wstring value);
  std::wstring GetPassword() const;
  // __property bool Passwordless = { read=FPasswordless, write=SetPasswordless };
  bool GetPasswordless() const { return FPasswordless; }
  // __property int PingInterval  = { read=FPingInterval, write=SetPingInterval };
  int GetPingInterval() const { return FPingInterval; }
  // __property bool TryAgent  = { read=FTryAgent, write=SetTryAgent };
  bool GetTryAgent() const { return FTryAgent; }
  // __property bool AgentFwd  = { read=FAgentFwd, write=SetAgentFwd };
  bool GetAgentFwd() const { return FAgentFwd; }
  // __property std::wstring ListingCommand = { read = FListingCommand, write = SetListingCommand };
  const std::wstring GetListingCommand() const { return FListingCommand; }
  // __property bool AuthTIS  = { read=FAuthTIS, write=SetAuthTIS };
  bool GetAuthTIS() const { return FAuthTIS; }
  // __property bool AuthKI  = { read=FAuthKI, write=SetAuthKI };
  bool GetAuthKI() const { return FAuthKI; }
  // __property bool AuthKIPassword  = { read=FAuthKIPassword, write=SetAuthKIPassword };
  bool GetAuthKIPassword() const { return FAuthKIPassword; }
  // __property bool AuthGSSAPI  = { read=FAuthGSSAPI, write=SetAuthGSSAPI };
  bool GetAuthGSSAPI() const { return FAuthGSSAPI; }
  // __property bool GSSAPIFwdTGT = { read=FGSSAPIFwdTGT, write=SetGSSAPIFwdTGT };
  bool GetGSSAPIFwdTGT() const { return FGSSAPIFwdTGT; }
  // __property std::wstring GSSAPIServerRealm = { read=FGSSAPIServerRealm, write=SetGSSAPIServerRealm };
  const std::wstring GetGSSAPIServerRealm() const { return FGSSAPIServerRealm; }
  // __property bool ChangeUsername  = { read=FChangeUsername, write=SetChangeUsername };
  bool GetChangeUsername() const { return FChangeUsername; }
  // __property bool Compression  = { read=FCompression, write=SetCompression };
  bool GetCompression() const { return FCompression; }
  // __property TSshProt SshProt  = { read=FSshProt, write=SetSshProt };
  TSshProt GetSshProt() const { return FSshProt; }
  // __property bool UsesSsh = { read = GetUsesSsh };
  bool GetUsesSsh();
  // __property bool Ssh2DES  = { read=FSsh2DES, write=SetSsh2DES };
  bool GetSsh2DES() const { return FSsh2DES; }
  // __property bool SshNoUserAuth  = { read=FSshNoUserAuth, write=SetSshNoUserAuth };
  bool GetSshNoUserAuth() const { return FSshNoUserAuth; }
  // __property TCipher Cipher[int Index] = { read=GetCipher, write=SetCipher };
  void SetCipher(int Index, TCipher value);
  TCipher GetCipher(int Index) const;
  // __property TKex Kex[int Index] = { read=GetKex, write=SetKex };
  void SetKex(int Index, TKex value);
  TKex GetKex(int Index) const;
  // __property std::wstring PublicKeyFile  = { read=FPublicKeyFile, write=SetPublicKeyFile };
  const std::wstring GetPublicKeyFile() const { return FPublicKeyFile; }
  // __property TProtocol Protocol  = { read=FProtocol, write=SetProtocol };
  TProtocol GetProtocol() const { return FProtocol; }
  // __property std::wstring ProtocolStr  = { read=GetProtocolStr, write=SetProtocolStr };
  void SetProtocolStr(const std::wstring value);
  std::wstring GetProtocolStr() const;
  // __property TFSProtocol FSProtocol  = { read=FFSProtocol, write=SetFSProtocol  };
  TFSProtocol GetFSProtocol() const { return FFSProtocol; }
  // __property std::wstring FSProtocolStr  = { read=GetFSProtocolStr };
  std::wstring GetFSProtocolStr();
  // __property bool Modified  = { read=FModified, write=FModified };
  bool GetModified() const { return FModified; }
  void SetModified(bool value) { FModified = value; }
  // __property bool CanLogin  = { read=GetCanLogin };
  bool GetCanLogin();
  // __property bool ClearAliases = { read = FClearAliases, write = SetClearAliases };
  bool GetClearAliases() const { return FClearAliases; }
  // __property nb::TDateTime PingIntervalDT = { read = GetPingIntervalDT, write = SetPingIntervalDT };
  void SetPingIntervalDT(nb::TDateTime value);
  nb::TDateTime GetPingIntervalDT() const;
  // __property nb::TDateTime TimeDifference = { read = FTimeDifference, write = SetTimeDifference };
  nb::TDateTime GetTimeDifference() const { return FTimeDifference; }
  // __property TPingType PingType = { read = FPingType, write = SetPingType };
  TPingType GetPingType() const { return FPingType; }
  // __property std::wstring SessionName  = { read=GetSessionName };
  std::wstring GetSessionName();
  bool HasSessionName();
  // __property std::wstring DefaultSessionName  = { read=GetDefaultSessionName };
  std::wstring GetDefaultSessionName();
  // __property std::wstring SessionUrl  = { read=GetSessionUrl };
  std::wstring GetSessionUrl();
  // __property std::wstring LocalDirectory  = { read=FLocalDirectory, write=SetLocalDirectory };
  std::wstring GetLocalDirectory() const { return FLocalDirectory; }
  // __property std::wstring RemoteDirectory  = { read=FRemoteDirectory, write=SetRemoteDirectory };
  std::wstring GetRemoteDirectory() const { return FRemoteDirectory; }
  void SetRemoteDirectory(const std::wstring value);
    // __property bool UpdateDirectories = { read=FUpdateDirectories, write=SetUpdateDirectories };
  bool GetUpdateDirectories() const { return FUpdateDirectories; }
  // __property bool CacheDirectories = { read=FCacheDirectories, write=SetCacheDirectories };
  bool GetCacheDirectories() const { return FCacheDirectories; }
  // __property bool CacheDirectoryChanges = { read=FCacheDirectoryChanges, write=SetCacheDirectoryChanges };
  bool GetCacheDirectoryChanges() const { return FCacheDirectoryChanges; }
  // __property bool PreserveDirectoryChanges = { read=FPreserveDirectoryChanges, write=SetPreserveDirectoryChanges };
  bool GetPreserveDirectoryChanges() const { return FPreserveDirectoryChanges; }
  // __property bool LockInHome = { read=FLockInHome, write=SetLockInHome };
  bool GetLockInHome() const { return FLockInHome; }
  // __property bool Special = { read=FSpecial, write=SetSpecial };
  bool GetSpecial() const { return FSpecial; }
  // __property bool Selected  = { read=FSelected, write=FSelected };
  bool GetSelected() const { return FSelected; }
  void SetSelected(bool value) { FSelected = value; }
  // __property std::wstring InfoTip  = { read=GetInfoTip };
  std::wstring GetInfoTip();
  // __property bool DefaultShell = { read = GetDefaultShell, write = SetDefaultShell };
  bool GetDefaultShell();
  void SetDefaultShell(bool value);
  // __property bool DetectReturnVar = { read = GetDetectReturnVar, write = SetDetectReturnVar };
  void SetDetectReturnVar(bool value);
  bool GetDetectReturnVar();
  // __property TEOLType EOLType = { read = FEOLType, write = SetEOLType };
  TEOLType GetEOLType() const { return FEOLType; }
  // __property bool LookupUserGroups = { read = FLookupUserGroups, write = SetLookupUserGroups };
  bool GetLookupUserGroups() const { return FLookupUserGroups; }
  // __property std::wstring ReturnVar = { read = FReturnVar, write = SetReturnVar };
  std::wstring GetReturnVar() const { return FReturnVar; }
  // __property bool Scp1Compatibility = { read = FScp1Compatibility, write = SetScp1Compatibility };
  bool GetScp1Compatibility() const { return FScp1Compatibility; }
  // __property std::wstring Shell = { read = FShell, write = SetShell };
  std::wstring GetShell() const { return FShell; }
  // __property std::wstring SftpServer = { read = FSftpServer, write = SetSftpServer };
  std::wstring GetSftpServer() const { return FSftpServer; }
  // __property int Timeout = { read = FTimeout, write = SetTimeout };
  int GetTimeout() const { return FTimeout; }
  // __property nb::TDateTime TimeoutDT = { read = GetTimeoutDT };
  nb::TDateTime GetTimeoutDT();
  // __property bool UnsetNationalVars = { read = FUnsetNationalVars, write = SetUnsetNationalVars };
  bool GetUnsetNationalVars() const { return FUnsetNationalVars; }
  // __property bool IgnoreLsWarnings  = { read=FIgnoreLsWarnings, write=SetIgnoreLsWarnings };
  bool GetIgnoreLsWarnings() const { return FIgnoreLsWarnings; }
  // __property bool TcpNoDelay  = { read=FTcpNoDelay, write=SetTcpNoDelay };
  bool GetTcpNoDelay() const { return FTcpNoDelay; }
  // __property std::wstring SshProtStr  = { read=GetSshProtStr };
  std::wstring GetSshProtStr();
  // __property std::wstring CipherList  = { read=GetCipherList, write=SetCipherList };
  void SetCipherList(const std::wstring value);
  std::wstring GetCipherList() const;
  // __property std::wstring KexList  = { read=GetKexList, write=SetKexList };
  void SetKexList(const std::wstring value);
  std::wstring GetKexList() const;
  // __property TProxyMethod ProxyMethod  = { read=FProxyMethod, write=SetProxyMethod };
  TProxyMethod GetProxyMethod() const { return FProxyMethod; }
  // __property std::wstring ProxyHost  = { read=FProxyHost, write=SetProxyHost };
  std::wstring GetProxyHost() const { return FProxyHost; }
  // __property int ProxyPort  = { read=FProxyPort, write=SetProxyPort };
  int GetProxyPort() const { return FProxyPort; }
  // __property std::wstring ProxyUsername  = { read=FProxyUsername, write=SetProxyUsername };
  std::wstring GetProxyUsername() const { return FProxyUsername; }
  // __property std::wstring ProxyPassword  = { read=GetProxyPassword, write=SetProxyPassword };
  std::wstring GetProxyPassword() const;
  void SetProxyPassword(const std::wstring value);
  // __property std::wstring ProxyTelnetCommand  = { read=FProxyTelnetCommand, write=SetProxyTelnetCommand };
  std::wstring GetProxyTelnetCommand() const { return FProxyTelnetCommand; }
  // __property std::wstring ProxyLocalCommand  = { read=FProxyLocalCommand, write=SetProxyLocalCommand };
  std::wstring GetProxyLocalCommand() const { return FProxyLocalCommand; }
  // __property TAutoSwitch ProxyDNS  = { read=FProxyDNS, write=SetProxyDNS };
  TAutoSwitch GetProxyDNS() const { return FProxyDNS; }
  // __property bool ProxyLocalhost  = { read=FProxyLocalhost, write=SetProxyLocalhost };
  bool GetProxyLocalhost() const { return FProxyLocalhost; }
  // __property int FtpProxyLogonType  = { read=FFtpProxyLogonType, write=SetFtpProxyLogonType };
  int GetFtpProxyLogonType() const { return FFtpProxyLogonType; }
  // __property TAutoSwitch Bug[TSshBug Bug]  = { read=GetBug, write=SetBug };
  void SetBug(TSshBug Bug, TAutoSwitch value);
  TAutoSwitch GetBug(TSshBug Bug) const;
  // __property std::wstring CustomParam1 = { read = FCustomParam1, write = SetCustomParam1 };
  std::wstring GetCustomParam1() const { return FCustomParam1; }
  // __property std::wstring CustomParam2 = { read = FCustomParam2, write = SetCustomParam2 };
  std::wstring GetCustomParam2() const { return FCustomParam2; }
  // __property std::wstring SessionKey = { read = GetSessionKey };
  std::wstring GetSessionKey();
  // __property bool ResolveSymlinks = { read = FResolveSymlinks, write = SetResolveSymlinks };
  bool GetResolveSymlinks() const { return FResolveSymlinks; }
  // __property int SFTPDownloadQueue = { read = FSFTPDownloadQueue, write = SetSFTPDownloadQueue };
  int GetSFTPDownloadQueue() const { return FSFTPDownloadQueue; }
  void SetSFTPDownloadQueue(int value);
  // __property int SFTPUploadQueue = { read = FSFTPUploadQueue, write = SetSFTPUploadQueue };
  int GetSFTPUploadQueue() const { return FSFTPUploadQueue; }
  void SetSFTPUploadQueue(int value);
  // __property int SFTPListingQueue = { read = FSFTPListingQueue, write = SetSFTPListingQueue };
  int GetSFTPListingQueue() const { return FSFTPListingQueue; }
  void SetSFTPListingQueue(int value);
  // __property int SFTPMaxVersion = { read = FSFTPMaxVersion, write = SetSFTPMaxVersion };
  int GetSFTPMaxVersion() const { return FSFTPMaxVersion; }
  // __property unsigned long SFTPMaxPacketSize = { read = FSFTPMaxPacketSize, write = SetSFTPMaxPacketSize };
  unsigned long GetSFTPMinPacketSize() const { return FSFTPMinPacketSize; }
  unsigned long GetSFTPMaxPacketSize() const { return FSFTPMaxPacketSize; }
  // __property TAutoSwitch SFTPBug[TSftpBug Bug]  = { read=GetSFTPBug, write=SetSFTPBug };
  void SetSFTPBug(TSftpBug Bug, TAutoSwitch value);
  TAutoSwitch GetSFTPBug(TSftpBug Bug) const;
  // __property TAutoSwitch SCPLsFullTime = { read = FSCPLsFullTime, write = SetSCPLsFullTime };
  TAutoSwitch GetSCPLsFullTime() const { return FSCPLsFullTime; }
  // __property TAutoSwitch FtpListAll = { read = FFtpListAll, write = SetFtpListAll };
  TAutoSwitch GetFtpListAll() const { return FFtpListAll; }
  // __property TDSTMode DSTMode = { read = FDSTMode, write = SetDSTMode };
  TDSTMode GetDSTMode() const { return FDSTMode; }
  // __property bool DeleteToRecycleBin = { read = FDeleteToRecycleBin, write = SetDeleteToRecycleBin };
  bool GetDeleteToRecycleBin() const { return FDeleteToRecycleBin; }
  // __property bool OverwrittenToRecycleBin = { read = FOverwrittenToRecycleBin, write = SetOverwrittenToRecycleBin };
  bool GetOverwrittenToRecycleBin() const { return FOverwrittenToRecycleBin; }
  // __property std::wstring RecycleBinPath = { read = FRecycleBinPath, write = SetRecycleBinPath };
  std::wstring GetRecycleBinPath() const { return FRecycleBinPath; }
  // __property std::wstring PostLoginCommands = { read = FPostLoginCommands, write = SetPostLoginCommands };
  std::wstring GetPostLoginCommands() const { return FPostLoginCommands; }
  // __property TAddressFamily AddressFamily = { read = FAddressFamily, write = SetAddressFamily };
  TAddressFamily GetAddressFamily() const { return FAddressFamily; }
  // __property std::wstring RekeyData = { read = FRekeyData, write = SetRekeyData };
  std::wstring GetRekeyData() const { return FRekeyData; }
  // __property unsigned int RekeyTime = { read = FRekeyTime, write = SetRekeyTime };
  unsigned int GetRekeyTime() const { return FRekeyTime; }
  // __property int Color = { read = FColor, write = SetColor };
  int GetColor() const { return FColor; }
  // __property bool Tunnel = { read = FTunnel, write = SetTunnel };
  bool GetTunnel() const { return FTunnel; }
  void SetTunnel(bool value);
  // __property std::wstring TunnelHostName = { read = FTunnelHostName, write = SetTunnelHostName };
  std::wstring GetTunnelHostName() const { return FTunnelHostName; }
  // __property int TunnelPortNumber = { read = FTunnelPortNumber, write = SetTunnelPortNumber };
  int GetTunnelPortNumber() const { return FTunnelPortNumber; }
  // __property std::wstring TunnelUserName = { read = FTunnelUserName, write = SetTunnelUserName };
  std::wstring GetTunnelUserName() const { return FTunnelUserName; }
  // __property std::wstring TunnelPassword = { read = GetTunnelPassword, write = SetTunnelPassword };
  void SetTunnelPassword(const std::wstring value);
  std::wstring GetTunnelPassword();
  // __property std::wstring TunnelPublicKeyFile = { read = FTunnelPublicKeyFile, write = SetTunnelPublicKeyFile };
  std::wstring GetTunnelPublicKeyFile() const { return FTunnelPublicKeyFile; }
  // __property bool TunnelAutoassignLocalPortNumber = { read = GetTunnelAutoassignLocalPortNumber };
  bool GetTunnelAutoassignLocalPortNumber();
  // __property int TunnelLocalPortNumber = { read = FTunnelLocalPortNumber, write = SetTunnelLocalPortNumber };
  int GetTunnelLocalPortNumber() const { return FTunnelLocalPortNumber; }
  // __property std::wstring TunnelPortFwd = { read = FTunnelPortFwd, write = SetTunnelPortFwd };
  std::wstring GetTunnelPortFwd() const { return FTunnelPortFwd; }
  void SetTunnelPortFwd(const std::wstring value);
  // __property bool FtpPasvMode = { read = FFtpPasvMode, write = SetFtpPasvMode };
  bool GetFtpPasvMode() const { return FFtpPasvMode; }
  bool GetFtpAllowEmptyPassword() const { return FFtpAllowEmptyPassword; }
  void SetFtpAllowEmptyPassword(bool value);
  // __property bool FtpForcePasvIp = { read = FFtpForcePasvIp, write = SetFtpForcePasvIp };
  bool GetFtpForcePasvIp() const { return FFtpForcePasvIp; }
  // __property std::wstring FtpAccount = { read = FFtpAccount, write = SetFtpAccount };
  std::wstring GetFtpAccount() const { return FFtpAccount; }
  // __property int FtpPingInterval  = { read=FFtpPingInterval, write=SetFtpPingInterval };
  int GetFtpPingInterval() const { return FFtpPingInterval; }
  // __property nb::TDateTime FtpPingIntervalDT  = { read=GetFtpPingIntervalDT };
  nb::TDateTime GetFtpPingIntervalDT();
  // __property TPingType FtpPingType = { read = FFtpPingType, write = SetFtpPingType };
  TPingType GetFtpPingType() const { return FFtpPingType; }
  // __property TFtps Ftps = { read = FFtps, write = SetFtps };
  TFtps GetFtps() const { return FFtps; }
  // __property TAutoSwitch NotUtf = { read = FNotUtf, write = SetNotUtf };
  TAutoSwitch GetNotUtf() const { return FNotUtf; }
  // __property std::wstring HostKey = { read = FHostKey, write = SetHostKey };
  std::wstring GetHostKey() const { return FHostKey; }
  // __property std::wstring StorageKey = { read = GetStorageKey };
  std::wstring GetStorageKey();
  // __property std::wstring OrigHostName = { read = FOrigHostName };
  std::wstring GetOrigHostName() const { return FOrigHostName; }
  // __property int OrigPortNumber = { read = FOrigPortNumber };
  int GetOrigPortNumber() const { return FOrigPortNumber; }
  // __property std::wstring LocalName = { read = GetLocalName };
  std::wstring GetLocalName();
  // __property std::wstring Source = { read = GetSource };
  std::wstring GetSource();

  void SetHostName(const std::wstring value);
  void SetPortNumber(int value);
  void SetUserName(const std::wstring value);
  void SetPasswordless(bool value);
  void SetPingInterval(int value);
  void SetTryAgent(bool value);
  void SetAgentFwd(bool value);
  void SetAuthTIS(bool value);
  void SetAuthKI(bool value);
  void SetAuthKIPassword(bool value);
  void SetAuthGSSAPI(bool value);
  void SetGSSAPIFwdTGT(bool value);
  void SetGSSAPIServerRealm(const std::wstring value);
  void SetChangeUsername(bool value);
  void SetCompression(bool value);
  void SetSshProt(TSshProt value);
  void SetSsh2DES(bool value);
  void SetSshNoUserAuth(bool value);
  void SetPublicKeyFile(const std::wstring value);

  void SetTimeDifference(nb::TDateTime value);
  void SetPingType(TPingType value);
  void SetProtocol(TProtocol value);
  void SetFSProtocol(TFSProtocol value);
  void SetLocalDirectory(const std::wstring value);
  void SetUpdateDirectories(bool value);
  void SetCacheDirectories(bool value);
  void SetCacheDirectoryChanges(bool value);
  void SetPreserveDirectoryChanges(bool value);
  void SetLockInHome(bool value);
  void SetSpecial(bool value);
  void SetListingCommand(const std::wstring value);
  void SetClearAliases(bool value);
  void SetEOLType(TEOLType value);
  void SetLookupUserGroups(bool value);
  void SetReturnVar(const std::wstring value);
  void SetScp1Compatibility(bool value);
  void SetShell(const std::wstring value);
  void SetSftpServer(const std::wstring value);
  void SetTimeout(int value);
  void SetUnsetNationalVars(bool value);
  void SetIgnoreLsWarnings(bool value);
  void SetTcpNoDelay(bool value);
  void SetProxyMethod(TProxyMethod value);
  void SetProxyHost(const std::wstring value);
  void SetProxyPort(int value);
  void SetProxyUsername(const std::wstring value);
  void SetProxyTelnetCommand(const std::wstring value);
  void SetProxyLocalCommand(const std::wstring value);
  void SetProxyDNS(TAutoSwitch value);
  void SetProxyLocalhost(bool value);
  void SetFtpProxyLogonType(int value);
  void SetCustomParam1(const std::wstring value);
  void SetCustomParam2(const std::wstring value);
  void SetResolveSymlinks(bool value);
  void SetSFTPMaxVersion(int value);
  void SetSFTPMinPacketSize(unsigned long value);
  void SetSFTPMaxPacketSize(unsigned long value);
  void SetSCPLsFullTime(TAutoSwitch value);
  void SetFtpListAll(TAutoSwitch value);
  void SetDSTMode(TDSTMode value);
  void SetDeleteToRecycleBin(bool value);
  void SetOverwrittenToRecycleBin(bool value);
  void SetRecycleBinPath(const std::wstring value);
  void SetPostLoginCommands(const std::wstring value);
  void SetAddressFamily(TAddressFamily value);
  void SetRekeyData(const std::wstring value);
  void SetRekeyTime(unsigned int value);
  void SetColor(int value);
  void SetTunnelHostName(const std::wstring value);
  void SetTunnelPortNumber(int value);
  void SetTunnelUserName(const std::wstring value);
  void SetTunnelPublicKeyFile(const std::wstring value);
  void SetTunnelLocalPortNumber(size_t value);
  void SetFtpPasvMode(bool value);
  void SetFtpForcePasvIp(bool value);
  void SetFtpAccount(const std::wstring value);
  void SetFtpPingInterval(int value);
  void SetFtpPingType(TPingType value);
  void SetFtps(TFtps value);
  void SetNotUtf(TAutoSwitch value);
  void SetHostKey(const std::wstring value);
private:
  std::wstring AdjustHostName(const std::wstring hostName, const std::wstring prefix);
  void RemoveProtocolPrefix(std::wstring &hostName);
};
//---------------------------------------------------------------------------
class TStoredSessionList : public TNamedObjectList
{
public:
  TStoredSessionList(bool aReadOnly = false);
  virtual ~TStoredSessionList();
  void Load(const std::wstring aKey, bool UseDefaults);
  void Load();
  void Save(bool All, bool Explicit);
  void Saved();
  void Export(const std::wstring FileName);
  void Load(THierarchicalStorage * Storage, bool AsModified = false,
    bool UseDefaults = false);
  void Save(THierarchicalStorage * Storage, bool All = false);
  void SelectAll(bool Select);
  void Import(TStoredSessionList * From, bool OnlySelected);
  void RecryptPasswords();
  void SelectSessionsToImport(TStoredSessionList * Dest, bool SSHOnly);
  void Cleanup();
  int IndexOf(TSessionData * Data);
  TSessionData *NewSession(const std::wstring SessionName, TSessionData *Session);
  TSessionData *ParseUrl(const std::wstring Url, TOptions *Options, bool &DefaultsOnly,
    std::wstring *FileName = NULL, bool *ProtocolDefined = NULL);
  // __property TSessionData * Sessions[int Index]  = { read=AtSession };
  TSessionData *GetSession(int Index) { return static_cast<TSessionData *>(AtObject(Index)); }
  // __property TSessionData * DefaultSettings  = { read=FDefaultSettings, write=SetDefaultSettings };
  TSessionData *GetDefaultSettings() const { return FDefaultSettings; }
  void SetDefaultSettings(TSessionData * value);

  static void ImportHostKeys(const std::wstring TargetKey,
    const std::wstring SourceKey, TStoredSessionList * Sessions,
    bool OnlySelected);

private:
  TSessionData * FDefaultSettings;
  bool FReadOnly;
  void DoSave(THierarchicalStorage * Storage, bool All, bool RecryptPasswordOnly);
  void DoSave(bool All, bool Explicit, bool RecryptPasswordOnly);
  void DoSave(THierarchicalStorage * Storage,
    TSessionData * Data, bool All, bool RecryptPasswordOnly,
    TSessionData * FactoryDefaults);
};
//---------------------------------------------------------------------------
#endif
