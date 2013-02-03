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
  if (F##Property != Value) { F##Property = Value; Modify(); }
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
  UnicodeString FTunnelHostKey;
  bool FFtpPasvMode;
  TAutoSwitch FFtpForcePasvIp;
  TAutoSwitch FFtpUseMlsd;
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
  void SetHostName(const UnicodeString & Value);
  UnicodeString GetHostNameExpanded();
  void SetPortNumber(int Value);
  void SetUserName(const UnicodeString & Value);
  UnicodeString GetUserNameExpanded();
  void SetPassword(const UnicodeString & Value);
  UnicodeString GetPassword() const;
  void SetPasswordless(bool Value);
  void SetPingInterval(int Value);
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

  void SetProtocolStr(const UnicodeString & Value);
  UnicodeString GetProtocolStr() const;
  bool GetCanLogin();
  void SetPingIntervalDT(TDateTime Value);
  TDateTime GetPingIntervalDT() const;
  TDateTime GetFtpPingIntervalDT();
  void SetTimeDifference(TDateTime Value);
  void SetPingType(TPingType Value);
  UnicodeString GetSessionName();
  bool HasSessionName();
  UnicodeString GetDefaultSessionName();
  UnicodeString GetSessionUrl();
  void SetProtocol(TProtocol Value);
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
  UnicodeString GetInfoTip();
  bool GetDefaultShell();
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
  void SetTimeout(int Value);
  void SetUnsetNationalVars(bool Value);
  void SetIgnoreLsWarnings(bool Value);
  void SetTcpNoDelay(bool Value);
  void SetSendBuf(int Value);
  void SetSshSimple(bool Value);
  UnicodeString GetSshProtStr();
  bool GetUsesSsh();
  void SetCipherList(const UnicodeString & Value);
  UnicodeString GetCipherList() const;
  void SetKexList(const UnicodeString & Value);
  UnicodeString GetKexList() const;
  void SetProxyMethod(TProxyMethod Value);
  void SetProxyHost(const UnicodeString & Value);
  void SetProxyPort(int Value);
  void SetProxyUsername(const UnicodeString & Value);
  void SetProxyPassword(const UnicodeString & Value);
  void SetProxyTelnetCommand(const UnicodeString & Value);
  void SetProxyLocalCommand(const UnicodeString & Value);
  void SetProxyDNS(TAutoSwitch Value);
  void SetProxyLocalhost(bool Value);
  UnicodeString GetProxyPassword() const;
  void SetFtpProxyLogonType(int Value);
  void SetBug(TSshBug Bug, TAutoSwitch Value);
  TAutoSwitch GetBug(TSshBug Bug) const;
  UnicodeString GetSessionKey();
  void SetCustomParam1(const UnicodeString & Value);
  void SetCustomParam2(const UnicodeString & Value);
  void SetResolveSymlinks(bool Value);
  void SetSFTPDownloadQueue(int Value);
  void SetSFTPUploadQueue(int Value);
  void SetSFTPListingQueue(int Value);
  void SetSFTPMaxVersion(int Value);
  void SetSFTPMinPacketSize(unsigned long Value);
  void SetSFTPMaxPacketSize(unsigned long Value);
  void SetSFTPBug(TSftpBug Bug, TAutoSwitch Value);
  TAutoSwitch GetSFTPBug(TSftpBug Bug) const;
  void SetSCPLsFullTime(TAutoSwitch Value);
  void SetFtpListAll(TAutoSwitch Value);
  void SetSslSessionReuse(bool Value);
  UnicodeString GetStorageKey();
  UnicodeString GetInternalStorageKey();
  void SetDSTMode(TDSTMode Value);
  void SetDeleteToRecycleBin(bool Value);
  void SetOverwrittenToRecycleBin(bool Value);
  void SetRecycleBinPath(const UnicodeString & Value);
  void SetPostLoginCommands(const UnicodeString & Value);
  void SetAddressFamily(TAddressFamily Value);
  void SetRekeyData(const UnicodeString & Value);
  void SetRekeyTime(unsigned int Value);
  void SetColor(int Value);
  void SetTunnel(bool Value);
  void SetTunnelHostName(const UnicodeString & Value);
  void SetTunnelPortNumber(int Value);
  void SetTunnelUserName(const UnicodeString & Value);
  void SetTunnelPassword(const UnicodeString & Value);
  UnicodeString GetTunnelPassword() const;
  void SetTunnelPublicKeyFile(const UnicodeString & Value);
  void SetTunnelPortFwd(const UnicodeString & Value);
  void SetTunnelLocalPortNumber(int Value);
  bool GetTunnelAutoassignLocalPortNumber();
  void SetTunnelHostKey(const UnicodeString & Value);
  void SetFtpPasvMode(bool Value);
  void SetFtpForcePasvIp(TAutoSwitch Value);
  void SetFtpUseMlsd(TAutoSwitch Value);
  void SetFtpAccount(const UnicodeString & Value);
  void SetFtpPingInterval(int Value);
  void SetFtpPingType(TPingType Value);
  void SetFtps(TFtps Value);
  void SetNotUtf(TAutoSwitch Value);
  void SetHostKey(const UnicodeString & Value);
  TDateTime GetTimeoutDT();
  void SavePasswords(THierarchicalStorage * Storage, bool PuttyExport);
  UnicodeString GetLocalName();
  void Modify();
  UnicodeString GetSource();
  void DoLoad(THierarchicalStorage * Storage, bool & RewritePassword);
  static RawByteString EncryptPassword(const UnicodeString & Password, const UnicodeString & Key);
  static UnicodeString DecryptPassword(const RawByteString & Password, const UnicodeString & Key);
  static RawByteString StronglyRecryptPassword(const RawByteString & Password, const UnicodeString & Key);

public:
  explicit TSessionData(const UnicodeString & AName);
  virtual ~TSessionData();
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
  bool ParseUrl(const UnicodeString & Url, TOptions * Options,
    TStoredSessionList * StoredSessions, bool & DefaultsOnly,
    UnicodeString * FileName, bool * AProtocolDefined, UnicodeString * MaskedUrl);
  bool ParseOptions(TOptions * Options);
  void ConfigureTunnel(int PortNumber);
  void RollbackTunnel();
  void ExpandEnvironmentVariables();
  bool IsSame(const TSessionData * Default, bool AdvancedOnly);
  static void ValidatePath(const UnicodeString & Path);
  static void ValidateName(const UnicodeString & Name);
  UnicodeString GetHostName() const { return FHostName; }
  int GetPortNumber() const { return FPortNumber; }
  TLoginType GetLoginType() const;
  void SetLoginType(TLoginType Value);
  UnicodeString GetUserName() const { return FUserName; }
  int GetPingInterval() const { return FPingInterval; }
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
  int GetTimeout() const { return FTimeout; }
  bool GetUnsetNationalVars() const { return FUnsetNationalVars; }
  bool GetIgnoreLsWarnings() const { return FIgnoreLsWarnings; }
  bool GetTcpNoDelay() const { return FTcpNoDelay; }
  int GetSendBuf() const { return FSendBuf; }
  bool GetSshSimple() const { return FSshSimple; }
  TProxyMethod GetProxyMethod() const { return FProxyMethod; }
  TProxyMethod GetActualProxyMethod() const
  {
    return GetProxyMethod() == pmSystem ? GetSystemProxyMethod() : GetProxyMethod();
  }
  UnicodeString GetProxyHost() const;
  int GetProxyPort() const;
  UnicodeString GetProxyUsername() const;
  UnicodeString GetProxyTelnetCommand() const { return FProxyTelnetCommand; }
  UnicodeString GetProxyLocalCommand() const { return FProxyLocalCommand; }
  TAutoSwitch GetProxyDNS() const { return FProxyDNS; }
  bool GetProxyLocalhost() const { return FProxyLocalhost; }
  int GetFtpProxyLogonType() const { return FFtpProxyLogonType; }
  UnicodeString GetCustomParam1() const { return FCustomParam1; }
  UnicodeString GetCustomParam2() const { return FCustomParam2; }
  bool GetResolveSymlinks() const { return FResolveSymlinks; }
  int GetSFTPDownloadQueue() const { return FSFTPDownloadQueue; }
  int GetSFTPUploadQueue() const { return FSFTPUploadQueue; }
  int GetSFTPListingQueue() const { return FSFTPListingQueue; }
  int GetSFTPMaxVersion() const { return FSFTPMaxVersion; }
  unsigned long GetSFTPMinPacketSize() const { return FSFTPMinPacketSize; }
  unsigned long GetSFTPMaxPacketSize() const { return FSFTPMaxPacketSize; }
  TAutoSwitch GetSCPLsFullTime() const { return FSCPLsFullTime; }
  TAutoSwitch GetFtpListAll() const { return FFtpListAll; }
  bool GetSslSessionReuse() const { return FSslSessionReuse; }
  TDSTMode GetDSTMode() const { return FDSTMode; }
  bool GetDeleteToRecycleBin() const { return FDeleteToRecycleBin; }
  bool GetOverwrittenToRecycleBin() const { return FOverwrittenToRecycleBin; }
  UnicodeString GetRecycleBinPath() const { return FRecycleBinPath; }
  UnicodeString GetPostLoginCommands() const { return FPostLoginCommands; }
  TAddressFamily GetAddressFamily() const { return FAddressFamily; }
  UnicodeString GetCodePage() const { return FCodePage; }
  void SetCodePage(const UnicodeString & Value);
  unsigned int GetCodePageAsNumber() const;
  UnicodeString GetRekeyData() const { return FRekeyData; }
  unsigned int GetRekeyTime() const { return FRekeyTime; }
  int GetColor() const { return FColor; }
  bool GetTunnel() const { return FTunnel; }
  UnicodeString GetTunnelHostName() const { return FTunnelHostName; }
  int GetTunnelPortNumber() const { return FTunnelPortNumber; }
  UnicodeString GetTunnelUserName() const { return FTunnelUserName; }
  UnicodeString GetTunnelPublicKeyFile() const { return FTunnelPublicKeyFile; }
  int GetTunnelLocalPortNumber() const { return FTunnelLocalPortNumber; }
  UnicodeString GetTunnelPortFwd() const { return FTunnelPortFwd; }
  UnicodeString GetTunnelHostKey() const { return FTunnelHostKey; }
  bool GetFtpPasvMode() const { return FFtpPasvMode; }
  bool GetFtpAllowEmptyPassword() const { return FFtpAllowEmptyPassword; }
  void SetFtpAllowEmptyPassword(bool Value);
  TAutoSwitch GetFtpForcePasvIp() const { return FFtpForcePasvIp; }
  TAutoSwitch GetFtpUseMlsd() const { return FFtpUseMlsd; }
  UnicodeString GetFtpAccount() const { return FFtpAccount; }
  int GetFtpPingInterval() const { return FFtpPingInterval; }
  TPingType GetFtpPingType() const { return FFtpPingType; }
  TFtps GetFtps() const { return FFtps; }
  TAutoSwitch GetNotUtf() const { return FNotUtf; }
  UnicodeString GetHostKey() const { return FHostKey; }
  UnicodeString GetOrigHostName() const { return FOrigHostName; }
  int GetOrigPortNumber() const { return FOrigPortNumber; }

  int GetNumberOfRetries() const { return FNumberOfRetries; }
  void SetNumberOfRetries(int Value) { FNumberOfRetries = Value; }
  uintptr_t GetSessionVersion() const { return FSessionVersion; }
  void SetSessionVersion(uintptr_t Value) { FSessionVersion = Value; }

private:
  uintptr_t GetDefaultVersion() { return ::GetVersionNumber219(); }
  TFSProtocol TranslateFSProtocolNumber(int FSProtocol);
  TFSProtocol TranslateFSProtocol(const UnicodeString & ProtocolID);
  TFtps TranslateFtpEncryptionNumber(int FtpEncryption);

private:
  mutable TIEProxyConfig * FIEProxyConfig;

private:
  TProxyMethod GetSystemProxyMethod() const;
  void  PrepareProxyData() const;
  void ParseIEProxyConfig() const;
  void FromURI(const UnicodeString & ProxyURI,
    UnicodeString & ProxyUrl, int & ProxyPort, TProxyMethod & ProxyMethod) const;
  void AdjustHostName(UnicodeString & HostName, const UnicodeString & Prefix);
  void RemoveProtocolPrefix(UnicodeString & HostName);
};
//---------------------------------------------------------------------------
class TStoredSessionList : public TNamedObjectList
{
public:
  explicit TStoredSessionList(bool aReadOnly = false);
  virtual ~TStoredSessionList();
  void Load(const UnicodeString & AKey, bool UseDefaults);
  void Load();
  void Save(bool All, bool Explicit);
  void Saved();
  void Export(const UnicodeString & FileName);
  void Load(THierarchicalStorage * Storage, bool AsModified = false,
    bool UseDefaults = false);
  void Save(THierarchicalStorage * Storage, bool All = false);
  void SelectAll(bool Select);
  void Import(TStoredSessionList * From, bool OnlySelected);
  void RecryptPasswords();
  TSessionData * AtSession(intptr_t Index)
    { return static_cast<TSessionData *>(AtObject(Index)); }
  void SelectSessionsToImport(TStoredSessionList * Dest, bool SSHOnly);
  void Cleanup();
  void UpdateStaticUsage();
  intptr_t IndexOf(TSessionData * Data);
  TSessionData * FindSame(TSessionData * Data);
  TSessionData * NewSession(UnicodeString SessionName, TSessionData * Session);
  TSessionData * ParseUrl(const UnicodeString & Url, TOptions * Options, bool & DefaultsOnly,
    UnicodeString * FileName = NULL, bool * ProtocolDefined = NULL, UnicodeString * MaskedUrl = NULL);
  TSessionData * GetSession(intptr_t Index) { return static_cast<TSessionData *>(AtObject(Index)); }
  TSessionData * GetDefaultSettings() const { return FDefaultSettings; }
  void SetDefaultSettings(TSessionData * Value);
  TSessionData * GetSessionByName(const UnicodeString & SessionName);

  static void ImportHostKeys(const UnicodeString & TargetKey,
    const UnicodeString & SourceKey, TStoredSessionList * Sessions,
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
bool GetCodePageInfo(UINT CodePage, CPINFOEX & CodePageInfoEx);
unsigned int GetCodePageAsNumber(const UnicodeString & CodePage);
UnicodeString GetCodePageAsString(unsigned int cp);
//---------------------------------------------------------------------------
UnicodeString GetExpandedLogFileName(UnicodeString LogFileName, TSessionData * SessionData);
//---------------------------------------------------------------------------
#endif
