
#pragma once

#include "Common.h"
#include "Option.h"
#include "FileBuffer.h"
#include "NamedObjs.h"
#include "HierarchicalStorage.h"
#include "Configuration.h"

#define SET_SESSION_PROPERTY(Property) \
  if (F##Property != Value) { F##Property = Value; Modify(); }

enum TCipher
{
  cipWarn,
  cip3DES,
  cipBlowfish,
  cipAES,
  cipDES,
  cipArcfour
};
#define CIPHER_COUNT (cipArcfour+1)
enum TProtocol
{
  ptRaw,
  ptTelnet,
  ptRLogin,
  ptSSH
};
#define PROTOCOL_COUNT (ptSSH+1)
// explicit values to skip obsoleted fsExternalSSH, fsExternalSFTP
enum TFSProtocol_219
{
  fsFTPS_219 = 6,
  fsHTTP_219 = 7,
  fsHTTPS_219 = 8
};
enum TFSProtocol
{
  fsSCPonly = 0,
  fsSFTP = 1,
  fsSFTPonly = 2,
  fsFTP = 5,
  fsWebDAV = 6
};
enum TLoginType
{
  ltAnonymous = 0,
  ltNormal = 1
};
#define FSPROTOCOL_COUNT (fsWebDAV+1)
enum TProxyMethod
{
  pmNone, pmSocks4, pmSocks5, pmHTTP, pmTelnet, pmCmd, pmSystem
};
enum TSshProt
{
  ssh1only, ssh1, ssh2, ssh2only
};
enum TKex
{
  kexWarn, kexDHGroup1, kexDHGroup14, kexDHGEx, kexRSA
};
#define KEX_COUNT (kexRSA+1)
enum TSshBug
{
  sbIgnore1, sbPlainPW1, sbRSA1, sbHMAC2, sbDeriveKey2, sbRSAPad2,
  sbPKSessID2, sbRekey2, sbMaxPkt2, sbIgnore2, sbWinAdj
};
#define BUG_COUNT (sbWinAdj+1)
enum TSftpBug
{
  sbSymlink,
  sbSignedTS
};
#define SFTP_BUG_COUNT (sbSignedTS+1)
enum TPingType
{
  ptOff, ptNullPacket, ptDummyCommand
};
enum TAddressFamily
{
  afAuto, afIPv4, afIPv6
};
enum TFtps
{
  ftpsNone, ftpsImplicit, ftpsExplicitSsl, ftpsExplicitTls
};
// has to match SSL_VERSION_XXX constants in AsyncSslSocketLayer.h
enum TTlsVersion { ssl2 = 2, ssl3 = 3, tls10 = 10, tls11 = 11, tls12 = 12 };
enum TSessionSource { ssNone, ssStored, ssStoredModified };

extern const wchar_t CipherNames[CIPHER_COUNT][10];
extern const wchar_t KexNames[KEX_COUNT][20];
// extern const wchar_t ProtocolNames[PROTOCOL_COUNT][10];
extern const wchar_t SshProtList[][10];
extern const wchar_t ProxyMethodList[][10];
extern const TCipher DefaultCipherList[CIPHER_COUNT];
extern const TKex DefaultKexList[KEX_COUNT];
extern const intptr_t DefaultSendBuf;
#define ANONYMOUS_USER_NAME L"anonymous"
#define ANONYMOUS_PASSWORD L""
extern const intptr_t SshPortNumber;
extern const intptr_t FtpPortNumber;
extern const intptr_t FtpsImplicitPortNumber;
extern const intptr_t HTTPPortNumber;
extern const intptr_t HTTPSPortNumber;
extern const intptr_t TelnetPortNumber;
extern const intptr_t ProxyPortNumber;
extern const UnicodeString PuttySshProtocol;
extern const UnicodeString PuttyTelnetProtocol;
extern const UnicodeString SftpProtocolStr;
extern const UnicodeString ScpProtocolStr;
extern const UnicodeString FtpProtocolStr;
extern const UnicodeString FtpsProtocolStr;
extern const UnicodeString WebDAVProtocolStr;
extern const UnicodeString WebDAVSProtocolStr;
extern const UnicodeString ProtocolSeparator;
extern const UnicodeString WinSCPProtocolPrefix;
extern const wchar_t UrlParamSeparator;
extern const wchar_t UrlParamValueSeparator;
extern const UnicodeString UrlHostKeyParamName;
extern const UnicodeString UrlSaveParamName;

struct TIEProxyConfig : public TObject
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
  intptr_t ProxyPort;
  TProxyMethod ProxyMethod;
};

class TStoredSessionList;

class TSessionData : public TNamedObject
{
friend class TStoredSessionList;
NB_DISABLE_COPY(TSessionData)
NB_DECLARE_CLASS(TSessionData)
public:
  void SetHostName(const UnicodeString & Value);
  UnicodeString GetHostNameExpanded() const;
  void SetPortNumber(intptr_t Value);
  void SetUserName(const UnicodeString & Value);
  UnicodeString GetUserNameExpanded() const;
  void SetPassword(const UnicodeString & Value);
  UnicodeString GetPassword() const;
  void SetPasswordless(bool Value);
  void SetPingInterval(intptr_t Value);
  void SetTryAgent(bool Value);
  void SetAgentFwd(bool Value);
  void SetAuthTIS(bool Value);
  void SetAuthKI(bool Value);
  void SetAuthKIPassword(bool Value);
  void SetAuthGSSAPI(bool Value);
  void SetGSSAPIFwdTGT(bool Value);
  void SetGSSAPIServerRealm(const UnicodeString & Value);
  void SetChangeUsername(bool Value);
  void SetCompression(bool Value);
  void SetSshProt(TSshProt Value);
  void SetSsh2DES(bool Value);
  void SetSshNoUserAuth(bool Value);
  void SetCipher(intptr_t Index, TCipher Value);
  TCipher GetCipher(intptr_t Index) const;
  void SetKex(intptr_t Index, TKex Value);
  TKex GetKex(intptr_t Index) const;
  void SetPublicKeyFile(const UnicodeString & Value);
  void SetPuttyProtocol(const UnicodeString & Value);
  UnicodeString GetPassphrase() const;
  void SetPassphrase(const UnicodeString & Value);

  void SetProtocolStr(const UnicodeString & Value);
  UnicodeString GetProtocolStr() const;
  bool GetCanLogin() const;
  void SetPingIntervalDT(const TDateTime & Value);
  TDateTime GetPingIntervalDT() const;
  TDateTime GetFtpPingIntervalDT() const;
  void SetTimeDifference(const TDateTime & Value);
  void SetPingType(TPingType Value);
  UnicodeString GetSessionName() const;
  bool HasSessionName() const;
  UnicodeString GetDefaultSessionName() const;
  UnicodeString GetSessionUrl() const;
  UnicodeString GetProtocolUrl() const;
  // void SetProtocol(TProtocol Value);
  void SetFSProtocol(TFSProtocol Value);
  UnicodeString GetFSProtocolStr() const;
  void SetLocalDirectory(const UnicodeString & Value);
  void SetRemoteDirectory(const UnicodeString & Value);
  void SetSynchronizeBrowsing(bool Value);
  void SetUpdateDirectories(bool Value);
  void SetCacheDirectories(bool Value);
  void SetCacheDirectoryChanges(bool Value);
  void SetPreserveDirectoryChanges(bool Value);
  void SetLockInHome(bool Value);
  void SetSpecial(bool Value);
  UnicodeString GetInfoTip() const;
  bool GetDefaultShell() const;
  void SetDetectReturnVar(bool Value);
  bool GetDetectReturnVar() const;
  void SetListingCommand(const UnicodeString & Value);
  void SetClearAliases(bool Value);
  void SetDefaultShell(bool Value);
  void SetEOLType(TEOLType Value);
  void SetLookupUserGroups(TAutoSwitch Value);
  void SetReturnVar(const UnicodeString & Value);
  void SetScp1Compatibility(bool Value);
  void SetShell(const UnicodeString & Value);
  void SetSftpServer(const UnicodeString & Value);
  void SetTimeout(intptr_t Value);
  void SetUnsetNationalVars(bool Value);
  void SetIgnoreLsWarnings(bool Value);
  void SetTcpNoDelay(bool Value);
  void SetSendBuf(intptr_t Value);
  void SetSshSimple(bool Value);
  UnicodeString GetSshProtStr() const;
  bool GetUsesSsh() const;
  void SetCipherList(const UnicodeString & Value);
  UnicodeString GetCipherList() const;
  void SetKexList(const UnicodeString & Value);
  UnicodeString GetKexList() const;
  void SetProxyMethod(TProxyMethod Value);
  void SetProxyHost(const UnicodeString & Value);
  void SetProxyPort(intptr_t Value);
  void SetProxyUsername(const UnicodeString & Value);
  void SetProxyPassword(const UnicodeString & Value);
  void SetProxyTelnetCommand(const UnicodeString & Value);
  void SetProxyLocalCommand(const UnicodeString & Value);
  void SetProxyDNS(TAutoSwitch Value);
  void SetProxyLocalhost(bool Value);
  UnicodeString GetProxyPassword() const;
  void SetFtpProxyLogonType(intptr_t Value);
  void SetBug(TSshBug Bug, TAutoSwitch Value);
  TAutoSwitch GetBug(TSshBug Bug) const;
  UnicodeString GetSessionKey() const;
  void SetCustomParam1(const UnicodeString & Value);
  void SetCustomParam2(const UnicodeString & Value);
  void SetResolveSymlinks(bool Value);
  void SetSFTPDownloadQueue(intptr_t Value);
  void SetSFTPUploadQueue(intptr_t Value);
  void SetSFTPListingQueue(intptr_t Value);
  void SetSFTPMaxVersion(intptr_t Value);
  void SetSFTPMinPacketSize(intptr_t Value);
  void SetSFTPMaxPacketSize(intptr_t Value);
  void SetSFTPBug(TSftpBug Bug, TAutoSwitch Value);
  TAutoSwitch GetSFTPBug(TSftpBug Bug) const;
  void SetSCPLsFullTime(TAutoSwitch Value);
  void SetFtpListAll(TAutoSwitch Value);
  void SetFtpDupFF(bool Value);
  void SetFtpUndupFF(bool Value);
  void SetSslSessionReuse(bool Value);
  UnicodeString GetStorageKey() const;
  UnicodeString GetInternalStorageKey() const;
  UnicodeString GetSiteKey() const;
  void SetDSTMode(TDSTMode Value);
  void SetDeleteToRecycleBin(bool Value);
  void SetOverwrittenToRecycleBin(bool Value);
  void SetRecycleBinPath(const UnicodeString & Value);
  void SetPostLoginCommands(const UnicodeString & Value);
  void SetAddressFamily(TAddressFamily Value);
  void SetRekeyData(const UnicodeString & Value);
  void SetRekeyTime(uintptr_t Value);
  void SetColor(intptr_t Value);
  void SetTunnel(bool Value);
  void SetTunnelHostName(const UnicodeString & Value);
  void SetTunnelPortNumber(intptr_t Value);
  void SetTunnelUserName(const UnicodeString & Value);
  void SetTunnelPassword(const UnicodeString & Value);
  UnicodeString GetTunnelPassword() const;
  void SetTunnelPublicKeyFile(const UnicodeString & Value);
  void SetTunnelPortFwd(const UnicodeString & Value);
  void SetTunnelLocalPortNumber(intptr_t Value);
  bool GetTunnelAutoassignLocalPortNumber();
  void SetTunnelHostKey(const UnicodeString & Value);
  void SetFtpPasvMode(bool Value);
  void SetFtpForcePasvIp(TAutoSwitch Value);
  void SetFtpUseMlsd(TAutoSwitch Value);
  void SetFtpAccount(const UnicodeString & Value);
  void SetFtpPingInterval(intptr_t Value);
  void SetFtpPingType(TPingType Value);
  void SetFtpTransferActiveImmediately(bool Value);
  void SetFtps(TFtps Value);
  void SetMinTlsVersion(TTlsVersion Value);
  void SetMaxTlsVersion(TTlsVersion Value);
  void SetNotUtf(TAutoSwitch Value);
  bool IsWorkspace() { return false; }
  // void SetIsWorkspace(bool Value);
  // void SetLink(const UnicodeString & Value);
  void SetHostKey(const UnicodeString & Value);
  UnicodeString GetNote() const { return FNote; }
  void SetNote(const UnicodeString & Value);
  TDateTime GetTimeoutDT();
  void SavePasswords(THierarchicalStorage * Storage, bool PuttyExport);
  UnicodeString GetLocalName() const;
  UnicodeString GetFolderName() const;
  void Modify();
  UnicodeString GetSource() const;
  bool GetSaveOnly() const { return FSaveOnly; }
  void DoLoad(THierarchicalStorage * Storage, bool & RewritePassword);
  //UnicodeString ReadXmlNode(_di_IXMLNode Node, const UnicodeString & Name, const UnicodeString & Default);
  //int ReadXmlNode(_di_IXMLNode Node, const UnicodeString & Name, int Default);
  //bool IsSameSite(const TSessionData * Default);
  static RawByteString EncryptPassword(const UnicodeString & Password, const UnicodeString & Key);
  static UnicodeString DecryptPassword(const RawByteString & Password, const UnicodeString & Key);
  static RawByteString StronglyRecryptPassword(const RawByteString & Password, const UnicodeString & Key);
  static bool DoIsProtocolUrl(const UnicodeString & Url, const UnicodeString & Protocol, intptr_t & ProtocolLen);
  static bool IsProtocolUrl(const UnicodeString & Url, const UnicodeString & Protocol, intptr_t & ProtocolLen);

public:
  explicit TSessionData(const UnicodeString & AName);
  virtual ~TSessionData();
  void Default();
  void NonPersistant();
  void Load(THierarchicalStorage * Storage);
  // void ImportFromFilezilla(_di_IXMLNode Node, const UnicodeString & APath);
  void Save(THierarchicalStorage * Storage, bool PuttyExport,
    const TSessionData * Default = nullptr);
  void SaveRecryptedPasswords(THierarchicalStorage * Storage);
  void RecryptPasswords();
  bool HasPassword() const;
  bool HasAnySessionPassword() const;
  bool HasAnyPassword() const;
  void ClearSessionPasswords();
  void Remove();
  void CacheHostKeyIfNotCached();
  virtual void Assign(const TPersistent * Source);
  bool ParseUrl(const UnicodeString & Url, TOptions * Options,
    TStoredSessionList * StoredSessions, bool & DefaultsOnly,
    UnicodeString * AFileName, bool * AProtocolDefined, UnicodeString * MaskedUrl);
  bool ParseOptions(TOptions * Options);
  void ConfigureTunnel(intptr_t PortNumber);
  void RollbackTunnel();
  void ExpandEnvironmentVariables();
  bool IsSame(const TSessionData * Default, bool AdvancedOnly, TStrings * DifferentProperties) const;
  bool IsSame(const TSessionData * Default, bool AdvancedOnly) const;
  bool IsInFolderOrWorkspace(const UnicodeString & Name) const;
  static void ValidatePath(const UnicodeString & APath);
  static void ValidateName(const UnicodeString & Name);
  static UnicodeString MakeValidName(const UnicodeString & Name);
  static UnicodeString ExtractLocalName(const UnicodeString & Name);
  static UnicodeString ExtractFolderName(const UnicodeString & Name);
  static UnicodeString ComposePath(const UnicodeString & APath, const UnicodeString & Name);

  UnicodeString GetHostName() const { return FHostName; }
  intptr_t GetPortNumber() const { return FPortNumber; }
  TLoginType GetLoginType() const;
  void SetLoginType(TLoginType Value);
  UnicodeString SessionGetUserName() const { return FUserName; }
  intptr_t GetPingInterval() const { return FPingInterval; }
  bool GetTryAgent() const { return FTryAgent; }
  bool GetAgentFwd() const { return FAgentFwd; }
  const UnicodeString GetListingCommand() const { return FListingCommand; }
  bool GetAuthTIS() const { return FAuthTIS; }
  bool GetAuthKI() const { return FAuthKI; }
  bool GetAuthKIPassword() const { return FAuthKIPassword; }
  bool GetAuthGSSAPI() const { return FAuthGSSAPI; }
  bool GetGSSAPIFwdTGT() const { return FGSSAPIFwdTGT; }
  const UnicodeString GetGSSAPIServerRealm() const { return FGSSAPIServerRealm; }
  bool GetChangeUsername() const { return FChangeUsername; }
  bool GetCompression() const { return FCompression; }
  TSshProt GetSshProt() const { return FSshProt; }
  bool GetSsh2DES() const { return FSsh2DES; }
  bool GetSshNoUserAuth() const { return FSshNoUserAuth; }
  const UnicodeString GetPublicKeyFile() const { return FPublicKeyFile; }
  TProtocol GetProtocol() const { return FProtocol; }
  UnicodeString GetPuttyProtocol() const { return FPuttyProtocol; }
  TFSProtocol GetFSProtocol() const { return FFSProtocol; }
  bool GetModified() const { return FModified; }
  void SetModified(bool Value) { FModified = Value; }
  bool GetClearAliases() const { return FClearAliases; }
  TDateTime GetTimeDifference() const { return FTimeDifference; }
  TPingType GetPingType() const { return FPingType; }
  UnicodeString GetLocalDirectory() const { return FLocalDirectory; }
  UnicodeString GetRemoteDirectory() const { return FRemoteDirectory; }
  bool GetSynchronizeBrowsing() const { return FSynchronizeBrowsing; }
  bool GetUpdateDirectories() const { return FUpdateDirectories; }
  bool GetCacheDirectories() const { return FCacheDirectories; }
  bool GetCacheDirectoryChanges() const { return FCacheDirectoryChanges; }
  bool GetPreserveDirectoryChanges() const { return FPreserveDirectoryChanges; }
  bool GetLockInHome() const { return FLockInHome; }
  bool GetSpecial() const { return FSpecial; }
  bool GetSelected() const { return FSelected; }
  void SetSelected(bool Value) { FSelected = Value; }
  TEOLType GetEOLType() const { return FEOLType; }
  TAutoSwitch GetLookupUserGroups() const { return FLookupUserGroups; }
  UnicodeString GetReturnVar() const { return FReturnVar; }
  bool GetScp1Compatibility() const { return FScp1Compatibility; }
  UnicodeString GetShell() const { return FShell; }
  UnicodeString GetSftpServer() const { return FSftpServer; }
  intptr_t GetTimeout() const { return FTimeout; }
  bool GetUnsetNationalVars() const { return FUnsetNationalVars; }
  bool GetIgnoreLsWarnings() const { return FIgnoreLsWarnings; }
  bool GetTcpNoDelay() const { return FTcpNoDelay; }
  intptr_t GetSendBuf() const { return FSendBuf; }
  bool GetSshSimple() const { return FSshSimple; }
  TProxyMethod GetProxyMethod() const { return FProxyMethod; }
  TProxyMethod GetActualProxyMethod() const
  {
    return GetProxyMethod() == pmSystem ? GetSystemProxyMethod() : GetProxyMethod();
  }
  UnicodeString GetProxyHost() const;
  intptr_t GetProxyPort() const;
  UnicodeString GetProxyUsername() const;
  UnicodeString GetProxyTelnetCommand() const { return FProxyTelnetCommand; }
  UnicodeString GetProxyLocalCommand() const { return FProxyLocalCommand; }
  TAutoSwitch GetProxyDNS() const { return FProxyDNS; }
  bool GetProxyLocalhost() const { return FProxyLocalhost; }
  intptr_t GetFtpProxyLogonType() const { return FFtpProxyLogonType; }
  UnicodeString GetCustomParam1() const { return FCustomParam1; }
  UnicodeString GetCustomParam2() const { return FCustomParam2; }
  bool GetResolveSymlinks() const { return FResolveSymlinks; }
  intptr_t GetSFTPDownloadQueue() const { return FSFTPDownloadQueue; }
  intptr_t GetSFTPUploadQueue() const { return FSFTPUploadQueue; }
  intptr_t GetSFTPListingQueue() const { return FSFTPListingQueue; }
  intptr_t GetSFTPMaxVersion() const { return FSFTPMaxVersion; }
  intptr_t GetSFTPMinPacketSize() const { return FSFTPMinPacketSize; }
  intptr_t GetSFTPMaxPacketSize() const { return FSFTPMaxPacketSize; }
  TAutoSwitch GetSCPLsFullTime() const { return FSCPLsFullTime; }
  TAutoSwitch GetFtpListAll() const { return FFtpListAll; }
  bool GetFtpDupFF() const { return FFtpDupFF; }
  bool GetFtpUndupFF() const { return FFtpUndupFF; }
  bool GetSslSessionReuse() const { return FSslSessionReuse; }
  TDSTMode GetDSTMode() const { return FDSTMode; }
  bool GetDeleteToRecycleBin() const { return FDeleteToRecycleBin; }
  bool GetOverwrittenToRecycleBin() const { return FOverwrittenToRecycleBin; }
  UnicodeString GetRecycleBinPath() const { return FRecycleBinPath; }
  UnicodeString GetPostLoginCommands() const { return FPostLoginCommands; }
  TAddressFamily GetAddressFamily() const { return FAddressFamily; }
  UnicodeString GetCodePage() const { return FCodePage; }
  void SetCodePage(const UnicodeString & Value);
  uintptr_t GetCodePageAsNumber() const;
  UnicodeString GetRekeyData() const { return FRekeyData; }
  uintptr_t GetRekeyTime() const { return FRekeyTime; }
  intptr_t GetColor() const { return FColor; }
  bool GetTunnel() const { return FTunnel; }
  UnicodeString GetTunnelHostName() const { return FTunnelHostName; }
  intptr_t GetTunnelPortNumber() const { return FTunnelPortNumber; }
  UnicodeString GetTunnelUserName() const { return FTunnelUserName; }
  UnicodeString GetTunnelPublicKeyFile() const { return FTunnelPublicKeyFile; }
  intptr_t GetTunnelLocalPortNumber() const { return FTunnelLocalPortNumber; }
  UnicodeString GetTunnelPortFwd() const { return FTunnelPortFwd; }
  UnicodeString GetTunnelHostKey() const { return FTunnelHostKey; }
  bool GetFtpPasvMode() const { return FFtpPasvMode; }
  bool GetFtpAllowEmptyPassword() const { return FFtpAllowEmptyPassword; }
  void SetFtpAllowEmptyPassword(bool Value);
  TAutoSwitch GetFtpForcePasvIp() const { return FFtpForcePasvIp; }
  TAutoSwitch GetFtpUseMlsd() const { return FFtpUseMlsd; }
  UnicodeString GetFtpAccount() const { return FFtpAccount; }
  intptr_t GetFtpPingInterval() const { return FFtpPingInterval; }
  TPingType GetFtpPingType() const { return FFtpPingType; }
  bool GetFtpTransferActiveImmediately() const { return FFtpTransferActiveImmediately; }
  TFtps GetFtps() const { return FFtps; }
  TTlsVersion GetMinTlsVersion() const { return FMinTlsVersion; }
  TTlsVersion GetMaxTlsVersion() const { return FMaxTlsVersion; }
  TAutoSwitch GetNotUtf() const { return FNotUtf; }
  UnicodeString GetHostKey() const { return FHostKey; }
  bool GetOverrideCachedHostKey() const { return FOverrideCachedHostKey; }
  UnicodeString GetOrigHostName() const { return FOrigHostName; }
  intptr_t GetOrigPortNumber() const { return FOrigPortNumber; }

  intptr_t GetNumberOfRetries() const { return FNumberOfRetries; }
  void SetNumberOfRetries(intptr_t Value) { FNumberOfRetries = Value; }
  uintptr_t GetSessionVersion() const { return FSessionVersion; }
  void SetSessionVersion(uintptr_t Value) { FSessionVersion = Value; }
  void RemoveProtocolPrefix(UnicodeString & HostName) const;

private:
  uintptr_t GetDefaultVersion() const { return ::GetCurrentVersionNumber(); }
  TFSProtocol TranslateFSProtocolNumber(intptr_t FSProtocol);
  TFSProtocol TranslateFSProtocol(const UnicodeString & ProtocolID);
  TFtps TranslateFtpEncryptionNumber(intptr_t FtpEncryption);

private:
  TProxyMethod GetSystemProxyMethod() const;
  void PrepareProxyData() const;
  void ParseIEProxyConfig() const;
  void FromURI(const UnicodeString & ProxyURI,
    UnicodeString & ProxyUrl, intptr_t & ProxyPort, TProxyMethod & ProxyMethod) const;
  void AdjustHostName(UnicodeString & HostName, const UnicodeString & Prefix) const;

private:
  UnicodeString FHostName;
  intptr_t FPortNumber;
  UnicodeString FUserName;
  RawByteString FPassword;
  intptr_t FPingInterval;
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
  UnicodeString FPassphrase;
  TProtocol FProtocol;
  UnicodeString FPuttyProtocol;
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
  intptr_t FTimeout;
  bool FUnsetNationalVars;
  bool FIgnoreLsWarnings;
  bool FTcpNoDelay;
  intptr_t FSendBuf;
  bool FSshSimple;
  TProxyMethod FProxyMethod;
  UnicodeString FProxyHost;
  intptr_t FProxyPort;
  UnicodeString FProxyUsername;
  RawByteString FProxyPassword;
  UnicodeString FProxyTelnetCommand;
  UnicodeString FProxyLocalCommand;
  TAutoSwitch FProxyDNS;
  bool FProxyLocalhost;
  intptr_t FFtpProxyLogonType;
  TAutoSwitch FBugs[BUG_COUNT];
  UnicodeString FCustomParam1;
  UnicodeString FCustomParam2;
  bool FResolveSymlinks;
  TDateTime FTimeDifference;
  intptr_t FSFTPDownloadQueue;
  intptr_t FSFTPUploadQueue;
  intptr_t FSFTPListingQueue;
  intptr_t FSFTPMaxVersion;
  intptr_t FSFTPMinPacketSize;
  intptr_t FSFTPMaxPacketSize;
  TDSTMode FDSTMode;
  TAutoSwitch FSFTPBugs[SFTP_BUG_COUNT];
  bool FDeleteToRecycleBin;
  bool FOverwrittenToRecycleBin;
  UnicodeString FRecycleBinPath;
  UnicodeString FPostLoginCommands;
  TAutoSwitch FSCPLsFullTime;
  TAutoSwitch FFtpListAll;
  bool FFtpDupFF;
  bool FFtpUndupFF;
  bool FSslSessionReuse;
  TAddressFamily FAddressFamily;
  UnicodeString FRekeyData;
  uintptr_t FRekeyTime;
  intptr_t FColor;
  bool FTunnel;
  UnicodeString FTunnelHostName;
  intptr_t FTunnelPortNumber;
  UnicodeString FTunnelUserName;
  RawByteString FTunnelPassword;
  UnicodeString FTunnelPublicKeyFile;
  intptr_t FTunnelLocalPortNumber;
  UnicodeString FTunnelPortFwd;
  UnicodeString FTunnelHostKey;
  bool FFtpPasvMode;
  TAutoSwitch FFtpForcePasvIp;
  TAutoSwitch FFtpUseMlsd;
  UnicodeString FFtpAccount;
  intptr_t FFtpPingInterval;
  TPingType FFtpPingType;
  bool FFtpTransferActiveImmediately;
  TFtps FFtps;
  TTlsVersion FMinTlsVersion;
  TTlsVersion FMaxTlsVersion;
  TAutoSwitch FNotUtf;
  bool FIsWorkspace;
  UnicodeString FLink;
  UnicodeString FHostKey;
  bool FOverrideCachedHostKey;
  UnicodeString FNote;

  UnicodeString FOrigHostName;
  intptr_t FOrigPortNumber;
  TProxyMethod FOrigProxyMethod;
  bool FTunnelConfigured;
  TSessionSource FSource;
  bool FSaveOnly;
  UnicodeString FCodePage;
  mutable uintptr_t FCodePageAsNumber;
  bool FFtpAllowEmptyPassword;
  TLoginType FLoginType;
  intptr_t FNumberOfRetries;
  uintptr_t FSessionVersion;

  mutable TIEProxyConfig * FIEProxyConfig;
};

class TStoredSessionList : public TNamedObjectList
{
NB_DISABLE_COPY(TStoredSessionList)
public:
  explicit TStoredSessionList(bool AReadOnly = false);
  virtual ~TStoredSessionList();
  void Load(const UnicodeString & AKey, bool UseDefaults);
  void Load();
  void Save(bool All, bool Explicit);
  void Saved();
  void ImportFromFilezilla(const UnicodeString & AFileName);
  void Export(const UnicodeString & AFileName);
  void Load(THierarchicalStorage * Storage, bool AsModified = false,
    bool UseDefaults = false);
  void Save(THierarchicalStorage * Storage, bool All = false);
  void SelectAll(bool Select);
  void Import(TStoredSessionList * From, bool OnlySelected, TList * Imported);
  void RecryptPasswords(TStrings * RecryptPasswordErrors);
  void SelectSessionsToImport(TStoredSessionList * Dest, bool SSHOnly);
  void Cleanup();
  void UpdateStaticUsage();
  intptr_t IndexOf(TSessionData * Data);
  TSessionData * FindSame(TSessionData * Data);
  TSessionData * NewSession(const UnicodeString & SessionName, TSessionData * Session);
  void NewWorkspace(const UnicodeString & Name, TList * DataList);
  bool IsFolder(const UnicodeString & Name);
  bool IsWorkspace(const UnicodeString & Name);
  TSessionData * ParseUrl(const UnicodeString & Url, TOptions * Options, bool & DefaultsOnly,
    UnicodeString * AFileName = nullptr, bool * ProtocolDefined = nullptr, UnicodeString * MaskedUrl = nullptr);
  bool IsUrl(const UnicodeString & Url);
  bool CanLogin(TSessionData * Data);
  void GetFolderOrWorkspace(const UnicodeString & Name, TList * List);
  TStrings * GetFolderOrWorkspaceList(const UnicodeString & Name);
  TStrings * GetWorkspaces();
  bool HasAnyWorkspace();
  const TSessionData * GetSession(intptr_t Index) const { return NB_STATIC_DOWNCAST_CONST(TSessionData, AtObject(Index)); }
  TSessionData * GetSession(intptr_t Index) { return NB_STATIC_DOWNCAST(TSessionData, AtObject(Index)); }
  const TSessionData * GetDefaultSettings() const { return FDefaultSettings; }
  const TSessionData * GetSessionByName(const UnicodeString & SessionName) const;
  void SetDefaultSettings(const TSessionData * Value);

  static void ImportHostKeys(const UnicodeString & TargetKey,
    const UnicodeString & SourceKey, TStoredSessionList * Sessions,
    bool OnlySelected);

private:
  TSessionData * FDefaultSettings;
  bool FReadOnly;
  void SetDefaultSettings(TSessionData * Value);
  void DoSave(THierarchicalStorage * Storage, bool All,
    bool RecryptPasswordOnly, TStrings * RecryptPasswordErrors);
  void DoSave(bool All, bool Explicit, bool RecryptPasswordOnly,
    TStrings * RecryptPasswordErrors);
  void DoSave(THierarchicalStorage * Storage,
    TSessionData * Data, bool All, bool RecryptPasswordOnly,
    TSessionData * FactoryDefaults);
  TSessionData * ResolveSessionData(TSessionData * Data);
  bool IsFolderOrWorkspace(const UnicodeString & Name, bool Workspace);
  TSessionData * CheckIsInFolderOrWorkspaceAndResolve(
    TSessionData * Data, const UnicodeString & Name);
  // void ImportLevelFromFilezilla(_di_IXMLNode Node, const UnicodeString & APath);
};

bool GetCodePageInfo(UINT CodePage, CPINFOEX & CodePageInfoEx);
uintptr_t GetCodePageAsNumber(const UnicodeString & CodePage);
UnicodeString GetCodePageAsString(uintptr_t CodePage);

UnicodeString GetExpandedLogFileName(const UnicodeString & LogFileName, TSessionData * SessionData);
bool IsSshProtocol(TFSProtocol FSProtocol);
intptr_t DefaultPort(TFSProtocol FSProtocol, TFtps Ftps);

