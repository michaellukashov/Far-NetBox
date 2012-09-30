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
enum TFSProtocol_219 { fsFTPS_219 = 6, fsHTTP_219 = 7, fsHTTPS_219 = 8 };
enum TFSProtocol { fsSCPonly = 0, fsSFTP = 1, fsSFTPonly = 2, fsFTP = 5, fsWebDAV = 6 };
enum TLoginType { ltAnonymous = 0, ltNormal = 1 };
#define FSPROTOCOL_COUNT (fsWebDAV+1)
enum TProxyMethod { pmNone, pmSocks4, pmSocks5, pmHTTP, pmTelnet, pmCmd, pmSystem };
enum TSshProt { ssh1only, ssh1, ssh2, ssh2only };
enum TKex { kexWarn, kexDHGroup1, kexDHGroup14, kexDHGEx, kexRSA };
#define KEX_COUNT (kexRSA+1)
enum TSshBug { sbIgnore1, sbPlainPW1, sbRSA1, sbHMAC2, sbDeriveKey2, sbRSAPad2,
  sbPKSessID2, sbRekey2, sbMaxPkt2, sbIgnore2 };
#define BUG_COUNT (sbIgnore2+1)
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
extern const wchar_t FSProtocolNames[FSPROTOCOL_COUNT][11];
extern const int DefaultSendBuf;
extern const UnicodeString AnonymousUserName;
extern const UnicodeString AnonymousPassword;
extern const int SshPortNumber;
extern const int FtpPortNumber;
extern const int FtpsImplicitPortNumber;
extern const int HTTPPortNumber;
extern const int HTTPSPortNumber;
//---------------------------------------------------------------------------
struct TIEProxyConfig
{
  TIEProxyConfig() :
    AutoDetect(false),
    ProxyPort(0),
    ProxyMethod(pmNone)
  {}
  bool AutoDetect; // not used
  UnicodeString AutoConfigUrl; // not used
  UnicodeString Proxy; //< string in format "http=host:80;https=host:443;ftp=ftpproxy:20;socks=socksproxy:1080"
  UnicodeString ProxyBypass; //< string in format "*.local, foo.com, google.com"
  UnicodeString ProxyHost;
  int ProxyPort;
  TProxyMethod ProxyMethod;
};
//---------------------------------------------------------------------------
class TStoredSessionList;
//---------------------------------------------------------------------------
class TSessionData : public TNamedObject
{
friend class TStoredSessionList;

private:
  UnicodeString FHostName;
  int FPortNumber;
  UnicodeString FUserName;
  RawByteString FPassword;
  int FPingInterval;
  TPingType FPingType;
  bool FTryAgent;
  bool FAgentFwd;
  UnicodeString FListingCommand;
  bool FAuthTIS;
  bool FAuthKI;
  bool FAuthKIPassword;
  bool FAuthGSSAPI;
  bool FGSSAPIFwdTGT; // not supported anymore
  UnicodeString FGSSAPIServerRealm; // not supported anymore
  bool FChangeUsername;
  bool FCompression;
  TSshProt FSshProt;
  bool FSsh2DES;
  bool FSshNoUserAuth;
  TCipher FCiphers[CIPHER_COUNT];
  TKex FKex[KEX_COUNT];
  bool FClearAliases;
  TEOLType FEOLType;
  UnicodeString FPublicKeyFile;
  TProtocol FProtocol;
  TFSProtocol FFSProtocol;
  bool FModified;
  UnicodeString FLocalDirectory;
  UnicodeString FRemoteDirectory;
  bool FLockInHome;
  bool FSpecial;
  bool FSynchronizeBrowsing;
  bool FUpdateDirectories;
  bool FCacheDirectories;
  bool FCacheDirectoryChanges;
  bool FPreserveDirectoryChanges;
  bool FSelected;
  TAutoSwitch FLookupUserGroups;
  UnicodeString FReturnVar;
  bool FScp1Compatibility;
  UnicodeString FShell;
  UnicodeString FSftpServer;
  int FTimeout;
  bool FUnsetNationalVars;
  bool FIgnoreLsWarnings;
  bool FTcpNoDelay;
  int FSendBuf;
  bool FSshSimple;
  TProxyMethod FProxyMethod;
  UnicodeString FProxyHost;
  int FProxyPort;
  UnicodeString FProxyUsername;
  RawByteString FProxyPassword;
  UnicodeString FProxyTelnetCommand;
  UnicodeString FProxyLocalCommand;
  TAutoSwitch FProxyDNS;
  bool FProxyLocalhost;
  int FFtpProxyLogonType;
  TAutoSwitch FBugs[BUG_COUNT];
  UnicodeString FCustomParam1;
  UnicodeString FCustomParam2;
  bool FResolveSymlinks;
  TDateTime FTimeDifference;
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
  UnicodeString FRecycleBinPath;
  UnicodeString FPostLoginCommands;
  TAutoSwitch FSCPLsFullTime;
  TAutoSwitch FFtpListAll;
  bool FSslSessionReuse;
  TAddressFamily FAddressFamily;
  UnicodeString FRekeyData;
  unsigned int FRekeyTime;
  int FColor;
  bool FTunnel;
  UnicodeString FTunnelHostName;
  int FTunnelPortNumber;
  UnicodeString FTunnelUserName;
  RawByteString FTunnelPassword;
  UnicodeString FTunnelPublicKeyFile;
  int FTunnelLocalPortNumber;
  UnicodeString FTunnelPortFwd;
  bool FFtpPasvMode;
  TAutoSwitch FFtpForcePasvIp;
  UnicodeString FFtpAccount;
  int FFtpPingInterval;
  TPingType FFtpPingType;
  TFtps FFtps;
  TAutoSwitch FNotUtf;
  UnicodeString FHostKey;

  UnicodeString FOrigHostName;
  int FOrigPortNumber;
  TProxyMethod FOrigProxyMethod;
  TSessionSource FSource;
  UnicodeString FCodePage;
  bool FFtpAllowEmptyPassword;
  TLoginType FLoginType;
  int FNumberOfRetries;
  uintptr_t FSessionVersion;

public:
  void __fastcall SetHostName(UnicodeString value);
  UnicodeString __fastcall GetHostNameExpanded();
  void __fastcall SetPortNumber(int value);
  void __fastcall SetUserName(UnicodeString value);
  UnicodeString __fastcall GetUserNameExpanded();
  void __fastcall SetPassword(UnicodeString value);
  UnicodeString __fastcall GetPassword() const;
  void __fastcall SetPasswordless(bool value);
  void __fastcall SetPingInterval(int value);
  void __fastcall SetTryAgent(bool value);
  void __fastcall SetAgentFwd(bool value);
  void __fastcall SetAuthTIS(bool value);
  void __fastcall SetAuthKI(bool value);
  void __fastcall SetAuthKIPassword(bool value);
  void __fastcall SetAuthGSSAPI(bool value);
  void __fastcall SetGSSAPIFwdTGT(bool value);
  void __fastcall SetGSSAPIServerRealm(UnicodeString value);
  void __fastcall SetChangeUsername(bool value);
  void __fastcall SetCompression(bool value);
  void __fastcall SetSshProt(TSshProt value);
  void __fastcall SetSsh2DES(bool value);
  void __fastcall SetSshNoUserAuth(bool value);
  void __fastcall SetCipher(int Index, TCipher value);
  TCipher __fastcall GetCipher(int Index) const;
  void __fastcall SetKex(int Index, TKex value);
  TKex __fastcall GetKex(int Index) const;
  void __fastcall SetPublicKeyFile(UnicodeString value);

  void __fastcall SetProtocolStr(UnicodeString value);
  UnicodeString __fastcall GetProtocolStr() const;
  bool __fastcall GetCanLogin();
  void __fastcall SetPingIntervalDT(TDateTime value);
  TDateTime __fastcall GetPingIntervalDT() const;
  TDateTime __fastcall GetFtpPingIntervalDT();
  void __fastcall SetTimeDifference(TDateTime value);
  void __fastcall SetPingType(TPingType value);
  UnicodeString __fastcall GetSessionName();
  bool __fastcall HasSessionName();
  UnicodeString __fastcall GetDefaultSessionName();
  UnicodeString __fastcall GetSessionUrl();
  void __fastcall SetProtocol(TProtocol value);
  void __fastcall SetFSProtocol(TFSProtocol value);
  UnicodeString __fastcall GetFSProtocolStr();
  void __fastcall SetLocalDirectory(UnicodeString value);
  void __fastcall SetRemoteDirectory(UnicodeString value);
  void __fastcall SetSynchronizeBrowsing(bool value);
  void __fastcall SetUpdateDirectories(bool value);
  void __fastcall SetCacheDirectories(bool value);
  void __fastcall SetCacheDirectoryChanges(bool value);
  void __fastcall SetPreserveDirectoryChanges(bool value);
  void __fastcall SetLockInHome(bool value);
  void __fastcall SetSpecial(bool value);
  UnicodeString __fastcall GetInfoTip();
  bool __fastcall GetDefaultShell();
  void __fastcall SetDetectReturnVar(bool value);
  bool __fastcall GetDetectReturnVar();
  void __fastcall SetListingCommand(UnicodeString value);
  void __fastcall SetClearAliases(bool value);
  void __fastcall SetDefaultShell(bool value);
  void __fastcall SetEOLType(TEOLType value);
  void __fastcall SetLookupUserGroups(TAutoSwitch value);
  void __fastcall SetReturnVar(UnicodeString value);
  void __fastcall SetScp1Compatibility(bool value);
  void __fastcall SetShell(UnicodeString value);
  void __fastcall SetSftpServer(UnicodeString value);
  void __fastcall SetTimeout(int value);
  void __fastcall SetUnsetNationalVars(bool value);
  void __fastcall SetIgnoreLsWarnings(bool value);
  void __fastcall SetTcpNoDelay(bool value);
  void __fastcall SetSendBuf(int value);
  void __fastcall SetSshSimple(bool value);
  UnicodeString __fastcall GetSshProtStr();
  bool __fastcall GetUsesSsh();
  void __fastcall SetCipherList(UnicodeString value);
  UnicodeString __fastcall GetCipherList() const;
  void __fastcall SetKexList(UnicodeString value);
  UnicodeString __fastcall GetKexList() const;
  void __fastcall SetProxyMethod(TProxyMethod value);
  void __fastcall SetProxyHost(UnicodeString value);
  void __fastcall SetProxyPort(int value);
  void __fastcall SetProxyUsername(UnicodeString value);
  void __fastcall SetProxyPassword(UnicodeString value);
  void __fastcall SetProxyTelnetCommand(UnicodeString value);
  void __fastcall SetProxyLocalCommand(UnicodeString value);
  void __fastcall SetProxyDNS(TAutoSwitch value);
  void __fastcall SetProxyLocalhost(bool value);
  UnicodeString __fastcall GetProxyPassword() const;
  void __fastcall SetFtpProxyLogonType(int value);
  void __fastcall SetBug(TSshBug Bug, TAutoSwitch value);
  TAutoSwitch __fastcall GetBug(TSshBug Bug) const;
  UnicodeString __fastcall GetSessionKey();
  void __fastcall SetCustomParam1(UnicodeString value);
  void __fastcall SetCustomParam2(UnicodeString value);
  void __fastcall SetResolveSymlinks(bool value);
  void __fastcall SetSFTPDownloadQueue(int value);
  void __fastcall SetSFTPUploadQueue(int value);
  void __fastcall SetSFTPListingQueue(int value);
  void __fastcall SetSFTPMaxVersion(int value);
  void __fastcall SetSFTPMinPacketSize(unsigned long value);
  void __fastcall SetSFTPMaxPacketSize(unsigned long value);
  void __fastcall SetSFTPBug(TSftpBug Bug, TAutoSwitch value);
  TAutoSwitch __fastcall GetSFTPBug(TSftpBug Bug) const;
  void __fastcall SetSCPLsFullTime(TAutoSwitch value);
  void __fastcall SetFtpListAll(TAutoSwitch value);
  void __fastcall SetSslSessionReuse(bool value);
  UnicodeString __fastcall GetStorageKey();
  UnicodeString __fastcall GetInternalStorageKey();
  void __fastcall SetDSTMode(TDSTMode value);
  void __fastcall SetDeleteToRecycleBin(bool value);
  void __fastcall SetOverwrittenToRecycleBin(bool value);
  void __fastcall SetRecycleBinPath(UnicodeString value);
  void __fastcall SetPostLoginCommands(UnicodeString value);
  void __fastcall SetAddressFamily(TAddressFamily value);
  void __fastcall SetRekeyData(UnicodeString value);
  void __fastcall SetRekeyTime(unsigned int value);
  void __fastcall SetColor(int value);
  void __fastcall SetTunnel(bool value);
  void __fastcall SetTunnelHostName(UnicodeString value);
  void __fastcall SetTunnelPortNumber(int value);
  void __fastcall SetTunnelUserName(UnicodeString value);
  void __fastcall SetTunnelPassword(UnicodeString value);
  UnicodeString __fastcall GetTunnelPassword() const;
  void __fastcall SetTunnelPublicKeyFile(UnicodeString value);
  void __fastcall SetTunnelPortFwd(UnicodeString value);
  void __fastcall SetTunnelLocalPortNumber(int value);
  bool __fastcall GetTunnelAutoassignLocalPortNumber();
  void __fastcall SetFtpPasvMode(bool value);
  void __fastcall SetFtpForcePasvIp(TAutoSwitch value);
  void __fastcall SetFtpAccount(UnicodeString value);
  void __fastcall SetFtpPingInterval(int value);
  void __fastcall SetFtpPingType(TPingType value);
  void __fastcall SetFtps(TFtps value);
  void __fastcall SetNotUtf(TAutoSwitch value);
  void __fastcall SetHostKey(UnicodeString value);
  TDateTime __fastcall GetTimeoutDT();
  void __fastcall SavePasswords(THierarchicalStorage * Storage, bool PuttyExport);
  UnicodeString __fastcall GetLocalName();
  void __fastcall Modify();
  UnicodeString __fastcall GetSource();
  void __fastcall DoLoad(THierarchicalStorage * Storage, bool & RewritePassword);
  static RawByteString __fastcall EncryptPassword(const UnicodeString & Password, UnicodeString Key);
  static UnicodeString __fastcall DecryptPassword(const RawByteString & Password, UnicodeString Key);
  static RawByteString __fastcall StronglyRecryptPassword(const RawByteString & Password, UnicodeString Key);

public:
  explicit /* __fastcall */ TSessionData(UnicodeString aName);
  virtual /* __fastcall */ ~TSessionData();
  void __fastcall Default();
  void __fastcall NonPersistant();
  void __fastcall Load(THierarchicalStorage * Storage);
  void __fastcall Save(THierarchicalStorage * Storage, bool PuttyExport,
    const TSessionData * Default = NULL);
  void __fastcall SaveRecryptedPasswords(THierarchicalStorage * Storage);
  void __fastcall RecryptPasswords();
  bool __fastcall HasAnyPassword();
  void __fastcall Remove();
  virtual void __fastcall Assign(TPersistent * Source);
  bool __fastcall ParseUrl(UnicodeString Url, TOptions * Options,
    TStoredSessionList * StoredSessions, bool & DefaultsOnly,
    UnicodeString * FileName, bool * AProtocolDefined);
  bool __fastcall ParseOptions(TOptions * Options);
  void __fastcall ConfigureTunnel(int PortNumber);
  void __fastcall RollbackTunnel();
  void __fastcall ExpandEnvironmentVariables();
  bool __fastcall IsSame(const TSessionData * Default, bool AdvancedOnly);
  static void __fastcall ValidatePath(const UnicodeString Path);
  static void __fastcall ValidateName(const UnicodeString Name);
  UnicodeString __fastcall GetHostName() const { return FHostName; }
  int __fastcall GetPortNumber() const { return FPortNumber; }
  TLoginType __fastcall GetLoginType() const;
  void __fastcall SetLoginType(TLoginType value);
  UnicodeString __fastcall GetUserName() const { return FUserName; }
  int __fastcall GetPingInterval() const { return FPingInterval; }
  bool __fastcall GetTryAgent() const { return FTryAgent; }
  bool __fastcall GetAgentFwd() const { return FAgentFwd; }
  const UnicodeString __fastcall GetListingCommand() const { return FListingCommand; }
  bool __fastcall GetAuthTIS() const { return FAuthTIS; }
  bool __fastcall GetAuthKI() const { return FAuthKI; }
  bool __fastcall GetAuthKIPassword() const { return FAuthKIPassword; }
  bool __fastcall GetAuthGSSAPI() const { return FAuthGSSAPI; }
  bool __fastcall GetGSSAPIFwdTGT() const { return FGSSAPIFwdTGT; }
  const UnicodeString __fastcall GetGSSAPIServerRealm() const { return FGSSAPIServerRealm; }
  bool __fastcall GetChangeUsername() const { return FChangeUsername; }
  bool __fastcall GetCompression() const { return FCompression; }
  TSshProt __fastcall GetSshProt() const { return FSshProt; }
  bool __fastcall GetSsh2DES() const { return FSsh2DES; }
  bool __fastcall GetSshNoUserAuth() const { return FSshNoUserAuth; }
  const UnicodeString __fastcall GetPublicKeyFile() const { return FPublicKeyFile; }
  TProtocol __fastcall GetProtocol() const { return FProtocol; }
  TFSProtocol __fastcall GetFSProtocol() const { return FFSProtocol; }
  bool __fastcall GetModified() const { return FModified; }
  void __fastcall SetModified(bool value) { FModified = value; }
  bool __fastcall GetClearAliases() const { return FClearAliases; }
  TDateTime __fastcall GetTimeDifference() const { return FTimeDifference; }
  TPingType __fastcall GetPingType() const { return FPingType; }
  UnicodeString __fastcall GetLocalDirectory() const { return FLocalDirectory; }
  UnicodeString __fastcall GetRemoteDirectory() const { return FRemoteDirectory; }
  bool __fastcall GetSynchronizeBrowsing() const { return FSynchronizeBrowsing; }
  bool __fastcall GetUpdateDirectories() const { return FUpdateDirectories; }
  bool __fastcall GetCacheDirectories() const { return FCacheDirectories; }
  bool __fastcall GetCacheDirectoryChanges() const { return FCacheDirectoryChanges; }
  bool __fastcall GetPreserveDirectoryChanges() const { return FPreserveDirectoryChanges; }
  bool __fastcall GetLockInHome() const { return FLockInHome; }
  bool __fastcall GetSpecial() const { return FSpecial; }
  bool __fastcall GetSelected() const { return FSelected; }
  void __fastcall SetSelected(bool value) { FSelected = value; }
  TEOLType __fastcall GetEOLType() const { return FEOLType; }
  TAutoSwitch __fastcall GetLookupUserGroups() const { return FLookupUserGroups; }
  UnicodeString __fastcall GetReturnVar() const { return FReturnVar; }
  bool __fastcall GetScp1Compatibility() const { return FScp1Compatibility; }
  UnicodeString __fastcall GetShell() const { return FShell; }
  UnicodeString __fastcall GetSftpServer() const { return FSftpServer; }
  int __fastcall GetTimeout() const { return FTimeout; }
  bool __fastcall GetUnsetNationalVars() const { return FUnsetNationalVars; }
  bool __fastcall GetIgnoreLsWarnings() const { return FIgnoreLsWarnings; }
  bool __fastcall GetTcpNoDelay() const { return FTcpNoDelay; }
  int __fastcall GetSendBuf() const { return FSendBuf; }
  bool __fastcall GetSshSimple() const { return FSshSimple; }
  TProxyMethod __fastcall GetProxyMethod() const { return FProxyMethod; }
  TProxyMethod __fastcall GetActualProxyMethod() const
  {
    return GetProxyMethod() == pmSystem ? GetSystemProxyMethod() : GetProxyMethod();
  }
  UnicodeString __fastcall GetProxyHost() const;
  int __fastcall GetProxyPort() const;
  UnicodeString __fastcall GetProxyUsername() const;
  UnicodeString __fastcall GetProxyTelnetCommand() const { return FProxyTelnetCommand; }
  UnicodeString __fastcall GetProxyLocalCommand() const { return FProxyLocalCommand; }
  TAutoSwitch __fastcall GetProxyDNS() const { return FProxyDNS; }
  bool __fastcall GetProxyLocalhost() const { return FProxyLocalhost; }
  int __fastcall GetFtpProxyLogonType() const { return FFtpProxyLogonType; }
  UnicodeString __fastcall GetCustomParam1() const { return FCustomParam1; }
  UnicodeString __fastcall GetCustomParam2() const { return FCustomParam2; }
  bool __fastcall GetResolveSymlinks() const { return FResolveSymlinks; }
  int __fastcall GetSFTPDownloadQueue() const { return FSFTPDownloadQueue; }
  int __fastcall GetSFTPUploadQueue() const { return FSFTPUploadQueue; }
  int __fastcall GetSFTPListingQueue() const { return FSFTPListingQueue; }
  int __fastcall GetSFTPMaxVersion() const { return FSFTPMaxVersion; }
  unsigned long __fastcall GetSFTPMinPacketSize() const { return FSFTPMinPacketSize; }
  unsigned long __fastcall GetSFTPMaxPacketSize() const { return FSFTPMaxPacketSize; }
  TAutoSwitch __fastcall GetSCPLsFullTime() const { return FSCPLsFullTime; }
  TAutoSwitch __fastcall GetFtpListAll() const { return FFtpListAll; }
  bool __fastcall GetSslSessionReuse() const { return FSslSessionReuse; }
  TDSTMode __fastcall GetDSTMode() const { return FDSTMode; }
  bool __fastcall GetDeleteToRecycleBin() const { return FDeleteToRecycleBin; }
  bool __fastcall GetOverwrittenToRecycleBin() const { return FOverwrittenToRecycleBin; }
  UnicodeString __fastcall GetRecycleBinPath() const { return FRecycleBinPath; }
  UnicodeString __fastcall GetPostLoginCommands() const { return FPostLoginCommands; }
  TAddressFamily __fastcall GetAddressFamily() const { return FAddressFamily; }
  UnicodeString __fastcall GetCodePage() const { return FCodePage; }
  void __fastcall SetCodePage(const UnicodeString value);
  unsigned int __fastcall GetCodePageAsNumber() const;
  UnicodeString __fastcall GetRekeyData() const { return FRekeyData; }
  unsigned int __fastcall GetRekeyTime() const { return FRekeyTime; }
  int __fastcall GetColor() const { return FColor; }
  bool __fastcall GetTunnel() const { return FTunnel; }
  UnicodeString __fastcall GetTunnelHostName() const { return FTunnelHostName; }
  int __fastcall GetTunnelPortNumber() const { return FTunnelPortNumber; }
  UnicodeString __fastcall GetTunnelUserName() const { return FTunnelUserName; }
  UnicodeString __fastcall GetTunnelPublicKeyFile() const { return FTunnelPublicKeyFile; }
  int __fastcall GetTunnelLocalPortNumber() const { return FTunnelLocalPortNumber; }
  UnicodeString __fastcall GetTunnelPortFwd() const { return FTunnelPortFwd; }
  bool __fastcall GetFtpPasvMode() const { return FFtpPasvMode; }
  bool __fastcall GetFtpAllowEmptyPassword() const { return FFtpAllowEmptyPassword; }
  void __fastcall SetFtpAllowEmptyPassword(bool value);
  TAutoSwitch __fastcall GetFtpForcePasvIp() const { return FFtpForcePasvIp; }
  UnicodeString __fastcall GetFtpAccount() const { return FFtpAccount; }
  int __fastcall GetFtpPingInterval() const { return FFtpPingInterval; }
  TPingType __fastcall GetFtpPingType() const { return FFtpPingType; }
  TFtps __fastcall GetFtps() const { return FFtps; }
  TAutoSwitch __fastcall GetNotUtf() const { return FNotUtf; }
  UnicodeString __fastcall GetHostKey() const { return FHostKey; }
  UnicodeString __fastcall GetOrigHostName() const { return FOrigHostName; }
  int __fastcall GetOrigPortNumber() const { return FOrigPortNumber; }

  int __fastcall GetNumberOfRetries() const { return FNumberOfRetries; }
  void __fastcall SetNumberOfRetries(int value) { FNumberOfRetries = value; }
  DWORD __fastcall GetSessionVersion() const { return FSessionVersion; }
  void __fastcall SetSessionVersion(DWORD value) { FSessionVersion = value; }
protected:
  DWORD __fastcall GetDefaultVersion() { return ::GetVersionNumber219(); }
  TFSProtocol __fastcall TranslateFSProtocolNumber(int FSProtocol);
  TFtps __fastcall TranslateFtpEncryptionNumber(int FtpEncryption);
private:
  mutable TIEProxyConfig * FIEProxyConfig;
private:
  TProxyMethod __fastcall GetSystemProxyMethod() const;
  void  __fastcall PrepareProxyData() const;
  void __fastcall ParseIEProxyConfig() const;
  void __fastcall FromURI(const UnicodeString & ProxyURI,
    UnicodeString & ProxyUrl, int & ProxyPort, TProxyMethod & ProxyMethod) const;
  void __fastcall AdjustHostName(UnicodeString & hostName, const UnicodeString prefix);
  void __fastcall RemoveProtocolPrefix(UnicodeString & hostName);
};
//---------------------------------------------------------------------------
class TStoredSessionList : public TNamedObjectList
{
public:
  explicit /* __fastcall */ TStoredSessionList(bool aReadOnly = false);
  virtual /* __fastcall */ ~TStoredSessionList();
  void __fastcall Load(const UnicodeString aKey, bool UseDefaults);
  void __fastcall Load();
  void __fastcall Save(bool All, bool Explicit);
  void __fastcall Saved();
  void __fastcall Export(const UnicodeString FileName);
  void __fastcall Load(THierarchicalStorage * Storage, bool AsModified = false,
    bool UseDefaults = false);
  void __fastcall Save(THierarchicalStorage * Storage, bool All = false);
  void __fastcall SelectAll(bool Select);
  void __fastcall Import(TStoredSessionList * From, bool OnlySelected);
  void __fastcall RecryptPasswords();
  TSessionData * __fastcall AtSession(int Index)
    { return static_cast<TSessionData *>(AtObject(Index)); }
  void __fastcall SelectSessionsToImport(TStoredSessionList * Dest, bool SSHOnly);
  void __fastcall Cleanup();
  void __fastcall UpdateStaticUsage();
  int __fastcall IndexOf(TSessionData * Data);
  TSessionData * __fastcall FindSame(TSessionData * Data);
  TSessionData * __fastcall NewSession(UnicodeString SessionName, TSessionData * Session);
  TSessionData * __fastcall ParseUrl(UnicodeString Url, TOptions * Options, bool & DefaultsOnly,
    UnicodeString * FileName = NULL, bool * ProtocolDefined = NULL);
  TSessionData * __fastcall GetSession(int Index) { return static_cast<TSessionData *>(AtObject(Index)); }
  TSessionData * __fastcall GetDefaultSettings() const { return FDefaultSettings; }
  void __fastcall SetDefaultSettings(TSessionData * value);
  TSessionData * __fastcall GetSessionByName(const UnicodeString SessionName);

  static void __fastcall ImportHostKeys(const UnicodeString TargetKey,
    const UnicodeString SourceKey, TStoredSessionList * Sessions,
    bool OnlySelected);

private:
  TSessionData * FDefaultSettings;
  bool FReadOnly;
  void __fastcall DoSave(THierarchicalStorage * Storage, bool All, bool RecryptPasswordOnly);
  void __fastcall DoSave(bool All, bool Explicit, bool RecryptPasswordOnly);
  void __fastcall DoSave(THierarchicalStorage * Storage,
    TSessionData * Data, bool All, bool RecryptPasswordOnly,
    TSessionData * FactoryDefaults);
};
//---------------------------------------------------------------------------
bool GetCodePageInfo(UINT CodePage, CPINFOEX & CodePageInfoEx);
unsigned int GetCodePageAsNumber(const UnicodeString CodePage);
UnicodeString GetCodePageAsString(unsigned int cp);
//---------------------------------------------------------------------------
UnicodeString GetExpandedLogFileName(UnicodeString LogFileName, TSessionData * SessionData);
//---------------------------------------------------------------------------
#endif
