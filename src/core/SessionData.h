
#pragma once

#include <Common.h>
#include <FileBuffer.h>

#include "Option.h"
#include "NamedObjs.h"
#include "HierarchicalStorage.h"
#include "Configuration.h"
#include <Xml.XMLIntf.hpp>

enum TCipher { cipWarn, cip3DES, cipBlowfish, cipAES, cipDES, cipArcfour, cipChaCha20, cipAESGCM, cipCount };
#define CIPHER_COUNT (cipCount)
// explicit values to skip obsoleted fsExternalSSH, fsExternalSFTP
enum TFSProtocol { fsSCPonly = 0, fsSFTP = 1, fsSFTPonly = 2, fsFTP = 5, fsWebDAV = 6, fsS3 = 7, };
#define FSPROTOCOL_COUNT (fsS3+1)
extern const wchar_t * ProxyMethodNames;
enum TProxyMethod { pmNone, pmSocks4, pmSocks5, pmHTTP, pmTelnet, pmCmd };
enum TKex { kexWarn, kexDHGroup1, kexDHGroup14, kexDHGroup15, kexDHGroup16, kexDHGroup17, kexDHGroup18, kexDHGEx, kexRSA, kexECDH, kexNTRUHybrid, kexCount };
#define KEX_COUNT (kexCount)
enum THostKey { hkWarn, hkRSA, hkDSA, hkECDSA, hkED25519, hkED448, hkCount };
#define HOSTKEY_COUNT (hkCount)
enum TGssLib { gssGssApi32, gssSspi, gssCustom };
#define GSSLIB_COUNT (gssCustom+1)
// names have to match PuTTY registry entries (see settings.c)
enum TSshBug { sbHMAC2, sbDeriveKey2, sbRSAPad2,
  sbPKSessID2, sbRekey2, sbMaxPkt2, sbIgnore2, sbOldGex2, sbWinAdj, sbChanReq };
#define BUG_COUNT (sbChanReq+1)
enum TSftpBug { sbSymlink, sbSignedTS };
#define SFTP_BUG_COUNT (sbSignedTS+1)
extern const wchar_t * PingTypeNames;
enum TPingType { ptOff, ptNullPacket, ptDummyCommand };
enum TAddressFamily { afAuto, afIPv4, afIPv6 };
enum TFtps { ftpsNone, ftpsImplicit, ftpsExplicitSsl, ftpsExplicitTls };
// ssl2 and ssh3 are equivalent of tls10 now
enum TTlsVersion { ssl2 = 2, ssl3 = 3, tls10 = 10, tls11 = 11, tls12 = 12, tls13 = 13 };
// has to match libs3 S3UriStyle
enum TS3UrlStyle { s3usVirtualHost, s3usPath };
enum TSessionSource { ssNone, ssStored, ssStoredModified };
enum TSessionUrlFlags
{
  sufSpecific = 0x01,
  sufUserName = 0x02,
  sufPassword = 0x04,
  sufHostKey = 0x08,
  sufRawSettings = 0x10,
  sufHttpForWebDAV = 0x20,
  sufSession = sufUserName | sufPassword | sufHostKey,
  sufComplete = sufSession | sufRawSettings,
  sufOpen = sufUserName | sufPassword,
};
enum TParseUrlFlags
{
  pufAllowStoredSiteWithProtocol = 0x01,
  pufUnsafe = 0x02,
  pufPreferProtocol = 0x04,
};

enum TFSProtocol_219
{
  fsFTPS_219 = 6,
  fsHTTP_219 = 7,
  fsHTTPS_219 = 8,
};


enum TLoginType
{
  ltAnonymous = 0,
  ltNormal = 1,
};

NB_CORE_EXPORT extern const UnicodeString CipherNames[CIPHER_COUNT];
NB_CORE_EXPORT extern const UnicodeString KexNames[KEX_COUNT];
NB_CORE_EXPORT extern const UnicodeString HostKeyNames[HOSTKEY_COUNT];
NB_CORE_EXPORT extern const UnicodeString GssLibNames[GSSLIB_COUNT];
NB_CORE_EXPORT extern const wchar_t SshProtList[][10];
NB_CORE_EXPORT extern const TCipher DefaultCipherList[CIPHER_COUNT];
NB_CORE_EXPORT extern const TKex DefaultKexList[KEX_COUNT];
NB_CORE_EXPORT extern const THostKey DefaultHostKeyList[HOSTKEY_COUNT];
NB_CORE_EXPORT extern const TGssLib DefaultGssLibList[GSSLIB_COUNT];
NB_CORE_EXPORT extern const wchar_t FSProtocolNames[FSPROTOCOL_COUNT][16];
NB_CORE_EXPORT extern const int32_t DefaultSendBuf;
NB_CORE_EXPORT extern const UnicodeString AnonymousUserName;
NB_CORE_EXPORT extern const UnicodeString AnonymousPassword;
NB_CORE_EXPORT extern const int32_t SshPortNumber;
NB_CORE_EXPORT extern const int32_t FtpPortNumber;
NB_CORE_EXPORT extern const int32_t FtpsImplicitPortNumber;
NB_CORE_EXPORT extern const int32_t HTTPPortNumber;
NB_CORE_EXPORT extern const int32_t HTTPSPortNumber;
NB_CORE_EXPORT extern const int32_t TelnetPortNumber;
NB_CORE_EXPORT extern const int32_t ProxyPortNumber;
NB_CORE_EXPORT extern const UnicodeString PuttySshProtocol;
NB_CORE_EXPORT extern const UnicodeString PuttyTelnetProtocol;
NB_CORE_EXPORT extern const UnicodeString SftpProtocol;
NB_CORE_EXPORT extern const UnicodeString ScpProtocol;
NB_CORE_EXPORT extern const UnicodeString FtpProtocol;
NB_CORE_EXPORT extern const UnicodeString FtpsProtocol;
NB_CORE_EXPORT extern const UnicodeString FtpesProtocol;
NB_CORE_EXPORT extern const UnicodeString WebDAVProtocol;
NB_CORE_EXPORT extern const UnicodeString WebDAVSProtocol;
NB_CORE_EXPORT extern const UnicodeString S3Protocol;
NB_CORE_EXPORT extern const UnicodeString SshProtocol;
NB_CORE_EXPORT extern const UnicodeString WinSCPProtocolPrefix;
NB_CORE_EXPORT extern const wchar_t UrlParamSeparator;
NB_CORE_EXPORT extern const wchar_t UrlParamValueSeparator;
NB_CORE_EXPORT extern const UnicodeString UrlHostKeyParamName;
NB_CORE_EXPORT extern const UnicodeString UrlSaveParamName;
NB_CORE_EXPORT extern const UnicodeString PassphraseOption;
NB_CORE_EXPORT extern const UnicodeString S3HostName;

constexpr int32_t SFTPMinVersion = 0;
constexpr int32_t SFTPMaxVersion = 6;

struct NB_CORE_EXPORT TIEProxyConfig : public TObject
{
  TIEProxyConfig() = default;
  bool AutoDetect{false}; // not used
  UnicodeString AutoConfigUrl; // not used
  UnicodeString Proxy; //< string in format "http=host:80;https=host:443;ftp=ftpproxy:20;socks=socksproxy:1080"
  UnicodeString ProxyBypass; //< string in format "*.local, foo.com, google.com"
  UnicodeString ProxyHost;
  int32_t ProxyPort{0};
  TProxyMethod ProxyMethod{pmNone};
};

class TStoredSessionList;
class TSecondaryTerminal;
class TFileZillaImpl;
class TFTPFileSystem;
class TSFTPFileSystem;
class TWebDAVFileSystem;
class TS3FileSystem;
class TSecureShell;
class TSessionLog;

NB_DEFINE_CLASS_ID(TSessionData);
class NB_CORE_EXPORT TSessionData : public TNamedObject
{
  friend class TStoredSessionList;
  friend class TSecondaryTerminal;
  friend class TFileZillaImpl;
  friend class TFTPFileSystem;
  friend class TSFTPFileSystem;
  friend class TWebDAVFileSystem;
  friend class TS3FileSystem;
  NB_DISABLE_COPY(TSessionData)
public:
  static bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TSessionData); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TSessionData) || TNamedObject::is(Kind); }
  friend class TTerminal;
  friend class TSecureShell;
  friend class TSessionLog;
  void SessionSetUserName(UnicodeString value);
private:
  UnicodeString FHostName;
  int32_t FPortNumber{0};
  UnicodeString FUserName;
  RawByteString FPassword;
  //RawByteString FNewPassword;
  UnicodeString FNewPassword;
  bool FChangePassword{false};
  int32_t FPingInterval{0};
  TPingType FPingType{ptOff};
  bool FTryAgent{false};
  bool FAgentFwd{false};
  UnicodeString FListingCommand;
  bool FAuthKI{false};
  bool FAuthKIPassword{false};
  bool FAuthGSSAPI{false};
  bool FAuthGSSAPIKEX{false};
  bool FGSSAPIFwdTGT{false};
  bool FChangeUsername{false};
  bool FCompression{false};
  bool FSsh2DES{false};
  bool FSshNoUserAuth{false};
  TCipher FCiphers[CIPHER_COUNT]{};
  TKex FKex[KEX_COUNT]{};
  THostKey FHostKeys[HOSTKEY_COUNT]{};
  TGssLib FGssLib[GSSLIB_COUNT]{};
  UnicodeString FGssLibCustom;
  bool FClearAliases{false};
  TEOLType FEOLType{};
  bool FTrimVMSVersions{false};
  bool FVMSAllRevisions{false};
  UnicodeString FPublicKeyFile;
  UnicodeString FPassphrase;
  UnicodeString FDetachedCertificate;
  UnicodeString FPuttyProtocol;
  TFSProtocol FFSProtocol{};
  bool FModified{false};
  UnicodeString FLocalDirectory;
  UnicodeString FRemoteDirectory;
  UnicodeString FOtherLocalDirectory;
  bool FSpecial{false};
  bool FSynchronizeBrowsing{false};
  bool FUpdateDirectories{false};
  bool FRequireDirectories{false};
  bool FCacheDirectories{false};
  bool FCacheDirectoryChanges{false};
  bool FPreserveDirectoryChanges{false};
  bool FSelected{false};
  TAutoSwitch FLookupUserGroups{asOn};
  UnicodeString FReturnVar;
  bool FExitCode1IsError{false};
  bool FScp1Compatibility{false};
  UnicodeString FShell;
  UnicodeString FSftpServer;
  int32_t FTimeout{0};
  bool FUnsetNationalVars{false};
  bool FIgnoreLsWarnings{false};
  bool FTcpNoDelay{false};
  int32_t FSendBuf{0};
  UnicodeString FSourceAddress;
  UnicodeString FProtocolFeatures;
  bool FSshSimple{false};
  TProxyMethod FProxyMethod{pmNone};
  UnicodeString FProxyHost;
  int32_t FProxyPort{0};
  UnicodeString FProxyUsername;
  RawByteString FProxyPassword;
  UnicodeString FProxyTelnetCommand;
  UnicodeString FProxyLocalCommand;
  TAutoSwitch FProxyDNS{};
  bool FProxyLocalhost{false};
  int32_t FFtpProxyLogonType{0};
  TAutoSwitch FBugs[BUG_COUNT]{};
  UnicodeString FPuttySettings;
  UnicodeString FCustomParam1;
  UnicodeString FCustomParam2;
  bool FResolveSymlinks{false};
  bool FFollowDirectorySymlinks{false};
  TDateTime FTimeDifference;
  bool FTimeDifferenceAuto{false};
  int32_t FSFTPDownloadQueue{0};
  int32_t FSFTPUploadQueue{0};
  int32_t FSFTPListingQueue{0};
  int32_t FSFTPMaxVersion{0};
  uint32_t FSFTPMaxPacketSize{0};
  TAutoSwitch FSFTPRealPath;
  TDSTMode FDSTMode{dstmKeep};
  TAutoSwitch FSFTPBugs[SFTP_BUG_COUNT]{};
  bool FDeleteToRecycleBin{false};
  bool FOverwrittenToRecycleBin{false};
  UnicodeString FRecycleBinPath;
  UnicodeString FPostLoginCommands;
  TAutoSwitch FSCPLsFullTime{};
  TAutoSwitch FFtpListAll{};
  TAutoSwitch FFtpHost{};
  TAutoSwitch FFtpWorkFromCwd;
  bool FFtpAnyCodeForPwd{false};
  bool FSslSessionReuse{false};
  UnicodeString FTlsCertificateFile;
  TAddressFamily FAddressFamily{afAuto};
  UnicodeString FRekeyData;
  uint32_t FRekeyTime{0};
  int32_t FColor{0};
  bool FTunnel{false};
  UnicodeString FTunnelHostName;
  int32_t FTunnelPortNumber{0};
  UnicodeString FTunnelUserName;
  RawByteString FTunnelPassword;
  UnicodeString FTunnelPublicKeyFile;
  RawByteString FTunnelPassphrase;
  int32_t FTunnelLocalPortNumber;
  UnicodeString FTunnelPortFwd;
  UnicodeString FTunnelHostKey;
  bool FFtpPasvMode{false};
  TAutoSwitch FFtpForcePasvIp{};
  TAutoSwitch FFtpUseMlsd{};
  UnicodeString FFtpAccount;
  int32_t FFtpPingInterval{0};
  TPingType FFtpPingType{};
  TAutoSwitch FFtpTransferActiveImmediately{};
  TFtps FFtps{};
  TTlsVersion FMinTlsVersion{};
  TTlsVersion FMaxTlsVersion{};
  TAutoSwitch FNotUtf{};
  int32_t FInternalEditorEncoding{0};
  UnicodeString FS3DefaultRegion;
  UnicodeString FS3SessionToken;
  UnicodeString FS3Profile;
  TS3UrlStyle FS3UrlStyle;
  TAutoSwitch FS3MaxKeys;
  bool FS3CredentialsEnv{false};
  bool FIsWorkspace{false};
  UnicodeString FLink;
  UnicodeString FNameOverride;
  UnicodeString FHostKey;
  bool FFingerprintScan{false};
  bool FOverrideCachedHostKey{false};
  UnicodeString FNote;
  UnicodeString FWinTitle;
  RawByteString FEncryptKey;
  bool FWebDavLiberalEscaping{false};

  UnicodeString FOrigHostName;
  int32_t FOrigPortNumber{0};
  TProxyMethod FOrigProxyMethod{pmNone};
  TSessionSource FSource{ssNone};
  bool FSaveOnly{false};
  UnicodeString FLogicalHostName;

public:
  void SetHostName(UnicodeString value);
  UnicodeString GetHostNameExpanded() const;
  UnicodeString GetHostNameSource() const;
  void SetPortNumber(int32_t value);
  void SetUserName(UnicodeString value);
  UnicodeString GetUserNameExpanded() const;
  UnicodeString GetUserNameSource() const;
  void SetPassword(UnicodeString value);
  UnicodeString GetPassword() const;
  void SetNewPassword(UnicodeString value);
  UnicodeString GetNewPassword() const;
  void SetChangePassword(bool value);
  void SetPingInterval(int32_t value);
  void SetTryAgent(bool value);
  void SetAgentFwd(bool value);
  void SetAuthKI(bool value);
  void SetAuthKIPassword(bool value);
  void SetAuthGSSAPI(bool value);
  void SetAuthGSSAPIKEX(bool value);
  void SetGSSAPIFwdTGT(bool value);
  void SetChangeUsername(bool value);
  void SetCompression(bool value);
  void SetSsh2DES(bool value);
  void SetSshNoUserAuth(bool value);
  void SetCipher(int32_t Index, TCipher value);
  TCipher GetCipher(int32_t Index) const;
  void SetKex(int32_t Index, TKex value);
  TKex GetKex(int32_t Index) const;
  void SetHostKeys(int32_t Index, THostKey value);
  THostKey GetHostKeys(int32_t Index) const;
  void SetGssLib(int32_t Index, TGssLib value);
  TGssLib GetGssLib(int32_t Index) const;
  void SetGssLibCustom(UnicodeString value);
  void SetPublicKeyFile(UnicodeString value);
  UnicodeString GetPassphrase() const;
  void SetPassphrase(UnicodeString value);
  void SetDetachedCertificate(UnicodeString value);

  void SetPuttyProtocol(UnicodeString value);
  bool GetCanLogin() const;
  bool GetCanOpen() const;
  bool GetIsLocalBrowser() const;
  void SetPingIntervalDT(TDateTime value);
  TDateTime GetPingIntervalDT() const;
  TDateTime GetFtpPingIntervalDT() const;
  void SetTimeDifference(TDateTime value);
  void SetTimeDifferenceAuto(bool value);
  void SetPingType(TPingType value);
  UnicodeString GetSessionName() const;
  UnicodeString GetDefaultSessionName() const;
  UnicodeString GetProtocolUrl(bool HttpForWebDAV) const;
  void SetFSProtocol(TFSProtocol value);
  UnicodeString GetFSProtocolStr() const;
  void SetLocalDirectory(UnicodeString value);
  void SetOtherLocalDirectory(UnicodeString value);
  UnicodeString GetLocalDirectoryExpanded() const;
  void SetRemoteDirectory(UnicodeString value);
  void SetSynchronizeBrowsing(bool value);
  void SetUpdateDirectories(bool value);
  void SetCacheDirectories(bool value);
  void SetCacheDirectoryChanges(bool value);
  void SetPreserveDirectoryChanges(bool value);
  void SetSpecial(bool value);
  UnicodeString GetInfoTip() const;
  bool GetDefaultShell() const;
  void SetDetectReturnVar(bool value);
  bool GetDetectReturnVar() const;
  void SetListingCommand(UnicodeString value);
  void SetClearAliases(bool value);
  void SetDefaultShell(bool value);
  void SetEOLType(TEOLType value);
  void SetTrimVMSVersions(bool value);
  void SetVMSAllRevisions(bool value);
  void SetLookupUserGroups(TAutoSwitch value);
  void SetReturnVar(UnicodeString value);
  void SetExitCode1IsError(bool value);
  void SetScp1Compatibility(bool value);
  void SetShell(UnicodeString value);
  void SetSftpServer(UnicodeString value);
  void SetTimeout(int32_t value);
  void SetUnsetNationalVars(bool value);
  void SetIgnoreLsWarnings(bool value);
  void SetTcpNoDelay(bool value);
  void SetSendBuf(int32_t value);
  void SetSourceAddress(const UnicodeString & value);
  void SetProtocolFeatures(const UnicodeString & value);
  void SetSshSimple(bool value);
  bool GetUsesSsh() const;
  void SetCipherList(UnicodeString value);
  UnicodeString GetCipherList() const;
  void SetKexList(UnicodeString value);
  UnicodeString GetKexList() const;
  void SetHostKeyList(UnicodeString value);
  UnicodeString GetHostKeyList() const;
  void SetGssLibList(UnicodeString value);
  UnicodeString GetGssLibList() const;
  void SetProxyMethod(TProxyMethod value);
  void SetProxyHost(UnicodeString value);
  void SetProxyPort(int32_t value);
  void SetProxyUsername(UnicodeString value);
  void SetProxyPassword(UnicodeString value);
  void SetProxyTelnetCommand(UnicodeString value);
  void SetProxyLocalCommand(UnicodeString value);
  void SetProxyDNS(TAutoSwitch value);
  void SetProxyLocalhost(bool value);
  UnicodeString GetProxyPassword() const;
  void SetFtpProxyLogonType(int32_t value);
  void SetBug(TSshBug Bug, TAutoSwitch value);
  TAutoSwitch GetBug(TSshBug Bug) const;
  UnicodeString GetSessionKey() const;
  void SetPuttySettings(UnicodeString value);
  void SetCustomParam1(UnicodeString value);
  void SetCustomParam2(UnicodeString value);
  void SetResolveSymlinks(bool value);
  void SetFollowDirectorySymlinks(bool value);
  void SetSFTPDownloadQueue(int32_t value);
  void SetSFTPUploadQueue(int32_t value);
  void SetSFTPListingQueue(int32_t value);
  void SetSFTPMaxVersion(int32_t value);
  void SetSFTPMaxPacketSize(uint32_t value);
  void SetSFTPRealPath(TAutoSwitch value);
  void SetSFTPBug(TSftpBug Bug, TAutoSwitch value);
  TAutoSwitch GetSFTPBug(TSftpBug Bug) const;
  void SetSCPLsFullTime(TAutoSwitch value);
  void SetFtpListAll(TAutoSwitch value);
  void SetFtpHost(TAutoSwitch value);
  void SetFtpWorkFromCwd(TAutoSwitch value);
  void SetFtpAnyCodeForPwd(bool value);
  void SetSslSessionReuse(bool value);
  void SetTlsCertificateFile(UnicodeString value);
  UnicodeString GetStorageKey() const;
  UnicodeString GetInternalStorageKey() const;
  UnicodeString GetSiteKey() const;
  void SetDSTMode(TDSTMode value);
  void SetDeleteToRecycleBin(bool value);
  void SetOverwrittenToRecycleBin(bool value);
  void SetRecycleBinPath(UnicodeString value);
  void SetPostLoginCommands(UnicodeString value);
  void SetAddressFamily(TAddressFamily value);
  void SetRekeyData(UnicodeString value);
  void SetRekeyTime(uint32_t value);
  void SetColor(int32_t value);
  void SetTunnel(bool value);
  void SetTunnelHostName(UnicodeString value);
  void SetTunnelPortNumber(int32_t value);
  void SetTunnelUserName(UnicodeString value);
  void SetTunnelPassword(UnicodeString value);
  UnicodeString GetTunnelPassword() const;
  void SetTunnelPublicKeyFile(UnicodeString value);
  void SetTunnelPassphrase(UnicodeString value);
  UnicodeString GetTunnelPassphrase() const;
  void SetTunnelPortFwd(UnicodeString value);
  void SetTunnelLocalPortNumber(int32_t value);
  bool GetTunnelAutoassignLocalPortNumber() const;
  void SetTunnelHostKey(UnicodeString value);
  void SetFtpPasvMode(bool value);
  void SetFtpForcePasvIp(TAutoSwitch value);
  void SetFtpUseMlsd(TAutoSwitch value);
  void SetFtpAccount(UnicodeString value);
  void SetFtpPingInterval(int32_t value);
  void SetFtpPingType(TPingType value);
  void SetFtpTransferActiveImmediately(TAutoSwitch value);
  void SetFtps(TFtps value);
  void SetMinTlsVersion(TTlsVersion value);
  void SetMaxTlsVersion(TTlsVersion value);
  void SetNotUtf(TAutoSwitch value);
  void SetInternalEditorEncoding(int32_t value);
  void SetS3DefaultRegion(UnicodeString value);
  void SetS3SessionToken(UnicodeString value);
  void SetS3Profile(UnicodeString value);
  void SetS3UrlStyle(TS3UrlStyle value);
  void SetS3MaxKeys(TAutoSwitch value);
  void SetS3CredentialsEnv(bool value);
  void SetLogicalHostName(UnicodeString value);
  void SetIsWorkspace(bool value);
  void SetLink(UnicodeString value);
  void SetNameOverride(UnicodeString value);
  void SetHostKey(UnicodeString value);
  void SetNote(UnicodeString value);
  void SetWinTitle(UnicodeString value);
  UnicodeString GetEncryptKey() const;
  void SetEncryptKey(UnicodeString value);
  void SetWebDavLiberalEscaping(bool value);

  TDateTime GetTimeoutDT() const;
  void SavePasswords(THierarchicalStorage * Storage, bool PuttyExport, bool DoNotEncryptPasswords, bool SaveAll);
  UnicodeString GetLocalName() const;
  UnicodeString GetFolderName() const;
  void Modify();
  UnicodeString GetSource() const;
  void DoLoad(THierarchicalStorage * Storage, bool PuttyImport, bool & RewritePassword, bool Unsafe, bool RespectDisablePasswordStoring);
  void DoSave(THierarchicalStorage * Storage,
    bool PuttyExport, const TSessionData *Default, bool DoNotEncryptPasswords);
#if 0
  UnicodeString ReadXmlNode(_di_IXMLNode Node, const UnicodeString Name, const UnicodeString Default);
  int ReadXmlNode(_di_IXMLNode Node, const UnicodeString Name, int Default);
  _di_IXMLNode FindSettingsNode(_di_IXMLNode Node, const UnicodeString Name);
  UnicodeString ReadSettingsNode(_di_IXMLNode Node, const UnicodeString Name, const UnicodeString Default);
  int ReadSettingsNode(_di_IXMLNode Node, const UnicodeString Name, int Default);
#endif // #if 0
  bool IsSame(const TSessionData * Default, bool AdvancedOnly, TStrings * DifferentProperties, bool Decrypted) const;
  UnicodeString GetNameWithoutHiddenPrefix() const;
  bool HasStateData() const;
  void CopyStateData(TSessionData * SourceData);
  void CopyNonCoreData(TSessionData * SourceData);
  UnicodeString GetNormalizedPuttyProtocol() const;
  void ReadPasswordsFromFiles();
  static RawByteString EncryptPassword(const UnicodeString Password, UnicodeString Key);
  static UnicodeString DecryptPassword(const RawByteString Password, UnicodeString Key);
  static RawByteString StronglyRecryptPassword(const RawByteString Password, UnicodeString Key);
  static bool DoIsProtocolUrl(const UnicodeString AUrl, const UnicodeString AProtocol, int32_t & ProtocolLen);
  static bool IsProtocolUrl(const UnicodeString AUrl, const UnicodeString Protocol, int32_t & ProtocolLen);
  static void AddSwitch(UnicodeString & Result, const UnicodeString Name, bool Rtf);
  static void AddSwitch(
    UnicodeString &Result, const UnicodeString Name, const UnicodeString Value, bool Rtf);
  static void AddSwitch(UnicodeString &Result, const UnicodeString AName, int32_t Value, bool Rtf);
#if 0
  static void AddAssemblyProperty(
    UnicodeString &Result, TAssemblyLanguage Language,
    const UnicodeString &Name, const UnicodeString &Value);
  static void AddAssemblyProperty(
    UnicodeString &Result, TAssemblyLanguage Language,
    const UnicodeString &Name, const UnicodeString &Type,
    const UnicodeString &Member);
  static void AddAssemblyProperty(
    UnicodeString &Result, TAssemblyLanguage Language,
    const UnicodeString &Name, int Value);
  void AddAssemblyProperty(
    UnicodeString &Result, TAssemblyLanguage Language,
    const UnicodeString &Name, bool Value);
#endif // #if 0
  TStrings * SaveToOptions(const TSessionData * Default);
  TStrings * GetRawSettingsForUrl();
  void DoCopyData(const TSessionData * SourceData, bool NoRecrypt);
  bool HasS3AutoCredentials() const;
  template<class AlgoT>
  void SetAlgoList(AlgoT * List, const AlgoT * DefaultList, const UnicodeString * Names,
    int32_t Count, AlgoT WarnAlgo, UnicodeString AValue);
  static void Remove(THierarchicalStorage * Storage, const UnicodeString Name);

  __property UnicodeString InternalStorageKey = { read = GetInternalStorageKey };

public:
  TSessionData() = delete;
  explicit TSessionData(UnicodeString aName) noexcept;
  virtual ~TSessionData() noexcept;
  TSessionData * Clone() const;
  void Default();
  void DefaultSettings();
  void NonPersistent();
  void Load(THierarchicalStorage * Storage, bool PuttyImport);
  void ApplyRawSettings(TStrings * RawSettings, bool Unsafe);
  void ApplyRawSettings(THierarchicalStorage * Storage, bool Unsafe, bool RespectDisablePasswordStoring);
  __removed void ImportFromFilezilla(_di_IXMLNode Node, const UnicodeString Path, _di_IXMLNode SettingsNode);
  void ImportFromOpenssh(TStrings * Lines);
  void Save(THierarchicalStorage * Storage, bool PuttyExport,
    const TSessionData *Default = nullptr);
  void SaveRecryptedPasswords(THierarchicalStorage * Storage);
  void RecryptPasswords();
  bool HasPassword() const;
  bool HasAnySessionPassword() const;
  bool HasAnyPassword() const;
  void ClearSessionPasswords();
  void MaskPasswords();
  void Remove();
  void CacheHostKeyIfNotCached();
  virtual void Assign(const TPersistent * Source);
  virtual int32_t Compare(const TNamedObject * Other) const override;
  void CopyData(const TSessionData * Source);
  void CopyDataNoRecrypt(const TSessionData * SourceData);
  void CopyDirectoriesStateData(TSessionData * SourceData);
  bool ParseUrl(UnicodeString Url, TOptions * Options,
    TStoredSessionList * AStoredSessions, bool &DefaultsOnly,
    UnicodeString * AFileName, bool * AProtocolDefined, UnicodeString *MaskedUrl, int32_t Flags);
  TStrings * SaveToOptions(const TSessionData * Default, bool SaveName, bool PuttyExport);
  void ConfigureTunnel(int32_t PortNumber);
  void RollbackTunnel();
  TSessionData * CreateTunnelData(int32_t TunnelLocalPortNumber);
  void ExpandEnvironmentVariables();
  void DisableAuthenticationsExceptPassword();
  bool IsSame(const TSessionData * Default, bool AdvancedOnly) const;
  bool IsSameDecrypted(const TSessionData * Default) const;
  bool IsSameSite(const TSessionData * Default) const;
  bool IsInFolderOrWorkspace(UnicodeString Name) const;
  UnicodeString GenerateSessionUrl(uint32_t Flags) const;
  bool HasRawSettingsForUrl();
  bool HasSessionName() const;
  bool HasAutoCredentials() const;
  int32_t GetDefaultPort() const;
  UnicodeString ResolvePublicKeyFile();

  UnicodeString GenerateOpenCommandArgs(bool Rtf) const;
  __removed void GenerateAssemblyCode(TAssemblyLanguage Language, UnicodeString & Head, UnicodeString & Tail, int & Indent);
  void LookupLastFingerprint();
  bool GetIsSecure() const;
  static void ValidatePath(const UnicodeString Path);
  static void ValidateName(const UnicodeString Name);
  static UnicodeString MakeValidName(const UnicodeString Name);
  static UnicodeString ExtractLocalName(const UnicodeString Name);
  static UnicodeString ExtractFolderName(const UnicodeString Name);
  static UnicodeString ComposePath(const UnicodeString Path, const UnicodeString Name);
  static bool IsSensitiveOption(const UnicodeString & Option, const UnicodeString & Value);
  static bool IsOptionWithParameters(const UnicodeString Option);
  static bool MaskPasswordInOptionParameter(const UnicodeString Option, UnicodeString & Param);
  static UnicodeString FormatSiteKey(const UnicodeString HostName, int32_t PortNumber);
  static TStrings * GetAllOptionNames(bool PuttyExport);

  __property UnicodeString HostName  = { read = FHostName, write = SetHostName };
  RWProperty<UnicodeString> HostName{nb::bind(&TSessionData::GetHostName, this), nb::bind(&TSessionData::SetHostName, this)};
  __property UnicodeString HostNameExpanded  = { read = GetHostNameExpanded };
  ROProperty<UnicodeString> HostNameExpanded{nb::bind(&TSessionData::GetHostNameExpanded, this)};
  __property UnicodeString HostNameSource = { read=GetHostNameSource };
  ROProperty<UnicodeString> HostNameSource{nb::bind(&TSessionData::GetHostNameSource, this)};
  __property int PortNumber = { read = FPortNumber, write = SetPortNumber };
  RWProperty<int32_t> PortNumber{nb::bind(&TSessionData::GetPortNumber, this), nb::bind(&TSessionData::SetPortNumber, this)};
  __property UnicodeString UserName = { read = FUserName, write = SessionSetUserName };
  RWProperty<UnicodeString> UserName{nb::bind(&TSessionData::SessionGetUserName, this), nb::bind(&TSessionData::SessionSetUserName, this)};
  __property UnicodeString UserNameExpanded  = { read = GetUserNameExpanded };
  __property UnicodeString UserNameSource  = { read=GetUserNameSource };
  __property UnicodeString Password  = { read = GetPassword, write = SetPassword };
  RWProperty<UnicodeString> Password{nb::bind(&TSessionData::GetPassword, this), nb::bind(&TSessionData::SetPassword, this)};
  __property UnicodeString NewPassword  = { read = GetNewPassword, write = SetNewPassword };
  RWPropertySimple<UnicodeString> NewPassword{&FNewPassword, nb::bind(&TSessionData::SetNewPassword, this)};
  __property bool ChangePassword  = { read = FChangePassword, write = SetChangePassword };
  RWPropertySimple<bool> ChangePassword{&FChangePassword, nb::bind(&TSessionData::SetChangePassword, this)};
  __property int PingInterval  = { read = FPingInterval, write = SetPingInterval };
  RWPropertySimple<int32_t> PingInterval{&FPingInterval, nb::bind(&TSessionData::SetPingInterval, this)};
  __property bool TryAgent  = { read = FTryAgent, write = SetTryAgent };
  RWProperty<bool> TryAgent{nb::bind(&TSessionData::GetTryAgent, this), nb::bind(&TSessionData::SetTryAgent, this)};
  __property bool AgentFwd  = { read = FAgentFwd, write = SetAgentFwd };
  RWProperty<bool> AgentFwd{nb::bind(&TSessionData::GetAgentFwd, this), nb::bind(&TSessionData::SetAgentFwd, this)};
  __property UnicodeString ListingCommand = { read = FListingCommand, write = SetListingCommand };
  __property bool AuthKI  = { read = FAuthKI, write = SetAuthKI };
  __property bool AuthKIPassword  = { read = FAuthKIPassword, write = SetAuthKIPassword };
  __property bool AuthGSSAPI  = { read = FAuthGSSAPI, write = SetAuthGSSAPI };
  __property bool AuthGSSAPIKEX  = { read=FAuthGSSAPIKEX, write=SetAuthGSSAPIKEX };
  __property bool GSSAPIFwdTGT = { read = FGSSAPIFwdTGT, write = SetGSSAPIFwdTGT };
  __property bool ChangeUsername  = { read = FChangeUsername, write = SetChangeUsername };
  RWPropertySimple<bool> ChangeUsername{&FChangeUsername, nb::bind(&TSessionData::SetChangeUsername, this)};
  __property bool Compression  = { read = FCompression, write = SetCompression };
  RWProperty<bool> Compression{nb::bind(&TSessionData::GetCompression, this), nb::bind(&TSessionData::SetCompression, this)};
  __property bool UsesSsh = { read = GetUsesSsh };
  __property bool Ssh2DES  = { read = FSsh2DES, write = SetSsh2DES };
  RWPropertySimple<bool> Ssh2DES{&FSsh2DES, nb::bind(&TSessionData::SetSsh2DES, this)};
  __property bool SshNoUserAuth  = { read = FSshNoUserAuth, write = SetSshNoUserAuth };
  RWPropertySimple<bool> SshNoUserAuth{&FSshNoUserAuth, nb::bind(&TSessionData::SetSshNoUserAuth, this)};
  __property TCipher Cipher[int Index] = { read = GetCipher, write = SetCipher };
  __property TKex Kex[int Index] = { read = GetKex, write = SetKex };
  __property THostKey HostKeys[int Index] = { read = GetHostKeys, write = SetHostKeys };
  __property TGssLib GssLib[int Index] = { read = GetGssLib, write = SetGssLib };
  __property UnicodeString GssLibCustom = { read = FGssLibCustom, write = SetGssLibCustom };
  RWPropertySimple<UnicodeString> GssLibCustom{&FGssLibCustom, nb::bind(&TSessionData::SetGssLibCustom, this)};
  __property UnicodeString PublicKeyFile  = { read = FPublicKeyFile, write = SetPublicKeyFile };
  RWProperty<UnicodeString> PublicKeyFile{nb::bind(&TSessionData::GetPublicKeyFile, this), nb::bind(&TSessionData::SetPublicKeyFile, this)};
  __property UnicodeString Passphrase  = { read = GetPassphrase, write = SetPassphrase };
  RWProperty<UnicodeString> Passphrase{nb::bind(&TSessionData::GetPassphrase, this), nb::bind(&TSessionData::SetPassphrase, this)};
  __property UnicodeString DetachedCertificate  = { read=FDetachedCertificate, write=SetDetachedCertificate };
  RWPropertySimple<UnicodeString> DetachedCertificate{&FDetachedCertificate, nb::bind(&TSessionData::SetDetachedCertificate, this)};
  __property UnicodeString PuttyProtocol  = { read = FPuttyProtocol, write = SetPuttyProtocol };
  RWProperty<UnicodeString> PuttyProtoco{nb::bind(&TSessionData::GetPuttyProtocol, this), nb::bind(&TSessionData::SetPuttyProtocol, this)};
  __property TFSProtocol FSProtocol = { read = FFSProtocol, write = SetFSProtocol  };
  RWProperty<TFSProtocol> FSProtocol{nb::bind(&TSessionData::GetFSProtocol, this), nb::bind(&TSessionData::SetFSProtocol, this)};
  __property UnicodeString FSProtocolStr = { read = GetFSProtocolStr };
  ROProperty<UnicodeString> FSProtocolStr{nb::bind(&TSessionData::GetFSProtocolStr, this)};
  __property bool Modified  = { read = FModified, write = FModified };
  __property bool CanLogin  = { read = GetCanLogin };
  __property bool CanOpen = { read=GetCanOpen };
  ROProperty<bool> CanOpen{nb::bind(&TSessionData::GetCanOpen, this)};
  __property bool IsLocalBrowser = { read=GetIsLocalBrowser };
  ROProperty<bool> IsLocalBrowser{nb::bind(&TSessionData::GetIsLocalBrowser, this)};
  __property bool ClearAliases = { read = FClearAliases, write = SetClearAliases };
  RWPropertySimple<bool> ClearAliases{&FClearAliases, nb::bind(&TSessionData::SetClearAliases, this)};
  __property TDateTime PingIntervalDT = { read = GetPingIntervalDT, write = SetPingIntervalDT };
  __property TDateTime TimeDifference = { read = FTimeDifference, write = SetTimeDifference };
  __property bool TimeDifferenceAuto = { read = FTimeDifferenceAuto, write = SetTimeDifferenceAuto };
  __property TPingType PingType = { read = FPingType, write = SetPingType };
  RWPropertySimple<TPingType> PingType{&FPingType, nb::bind(&TSessionData::SetPingType, this)};
  __property UnicodeString SessionName  = { read = GetSessionName };
  ROProperty<UnicodeString> SessionName{nb::bind(&TSessionData::GetSessionName, this)};
  __property UnicodeString DefaultSessionName  = { read = GetDefaultSessionName };
  ROProperty<UnicodeString> DefaultSessionName{nb::bind(&TSessionData::GetDefaultSessionName, this)};
  __property UnicodeString LocalDirectory  = { read = FLocalDirectory, write = SetLocalDirectory };
  RWPropertySimple<UnicodeString> LocalDirectory{&FLocalDirectory, nb::bind(&TSessionData::SetLocalDirectory, this) };
  __property UnicodeString LocalDirectoryExpanded = { read = GetLocalDirectoryExpanded };
  __property UnicodeString OtherLocalDirectory = { read=FOtherLocalDirectory, write=SetOtherLocalDirectory };
  RWPropertySimple<UnicodeString> OtherLocalDirectory{&FOtherLocalDirectory, nb::bind(&TSessionData::SetOtherLocalDirectory, this) };
  __property UnicodeString RemoteDirectory  = { read = FRemoteDirectory, write = SetRemoteDirectory };
  RWProperty<UnicodeString> RemoteDirectory{nb::bind(&TSessionData::GetRemoteDirectory, this), nb::bind(&TSessionData::SetRemoteDirectory, this)};
  __property bool SynchronizeBrowsing = { read = FSynchronizeBrowsing, write = SetSynchronizeBrowsing };
  __property bool UpdateDirectories = { read = FUpdateDirectories, write = SetUpdateDirectories };
  RWProperty<bool> UpdateDirectories{nb::bind(&TSessionData::GetUpdateDirectories, this), nb::bind(&TSessionData::SetUpdateDirectories, this)};
  __property bool RequireDirectories = { read=FRequireDirectories, write=FRequireDirectories };
  RWProperty2<bool> RequireDirectories{&FRequireDirectories};
  __property bool CacheDirectories = { read = FCacheDirectories, write = SetCacheDirectories };
  RWPropertySimple<bool> CacheDirectories{&FCacheDirectories, nb::bind(&TSessionData::SetCacheDirectories, this)};
  __property bool CacheDirectoryChanges = { read = FCacheDirectoryChanges, write = SetCacheDirectoryChanges };
  RWPropertySimple<bool> CacheDirectoryChanges{&FCacheDirectoryChanges, nb::bind(&TSessionData::SetCacheDirectoryChanges, this)};
  __property bool PreserveDirectoryChanges = { read = FPreserveDirectoryChanges, write = SetPreserveDirectoryChanges };
  RWPropertySimple<bool> PreserveDirectoryChanges{&FPreserveDirectoryChanges, nb::bind(&TSessionData::SetPreserveDirectoryChanges, this)};
  __property bool Special = { read = FSpecial, write = SetSpecial };
  RWPropertySimple<bool> Special{&FSpecial, nb::bind(&TSessionData::SetSpecial, this)};
  __property bool Selected  = { read = FSelected, write = FSelected };
  RWProperty2<bool> Selected{&FSelected};
  __property UnicodeString InfoTip  = { read = GetInfoTip };
  __property bool DefaultShell = { read = GetDefaultShell, write = SetDefaultShell };
  __property bool DetectReturnVar = { read = GetDetectReturnVar, write = SetDetectReturnVar };
  __property TEOLType EOLType = { read = FEOLType, write = SetEOLType };
  RWPropertySimple<TEOLType> EOLType{&FEOLType, nb::bind(&TSessionData::SetEOLType, this)};
  __property bool TrimVMSVersions = { read = FTrimVMSVersions, write = SetTrimVMSVersions };
  RWPropertySimple<bool> TrimVMSVersions{&FTrimVMSVersions, nb::bind(&TSessionData::SetTrimVMSVersions, this) };
  __property bool VMSAllRevisions = { read = FVMSAllRevisions, write = SetVMSAllRevisions };
  RWPropertySimple<bool> VMSAllRevisions{&FVMSAllRevisions, nb::bind(&TSessionData::SetVMSAllRevisions, this) };
  __property TAutoSwitch LookupUserGroups = { read = FLookupUserGroups, write = SetLookupUserGroups };
  RWPropertySimple<TAutoSwitch> LookupUserGroups{&FLookupUserGroups, nb::bind(&TSessionData::SetLookupUserGroups, this)};
  __property UnicodeString ReturnVar = { read = FReturnVar, write = SetReturnVar };
  RWPropertySimple<UnicodeString> ReturnVar{&FReturnVar, nb::bind(&TSessionData::SetReturnVar, this)};
  __property bool ExitCode1IsError = { read = FExitCode1IsError, write = SetExitCode1IsError };
  RWPropertySimple<bool> ExitCode1IsError{&FExitCode1IsError, nb::bind(&TSessionData::SetExitCode1IsError, this)};
  __property bool Scp1Compatibility = { read = FScp1Compatibility, write = SetScp1Compatibility };
  RWPropertySimple<bool> Scp1Compatibility {&FScp1Compatibility, nb::bind(&TSessionData::SetScp1Compatibility, this)};
  __property UnicodeString Shell = { read = FShell, write = SetShell };
  RWPropertySimple<UnicodeString> Shell{&FShell, nb::bind(&TSessionData::SetShell, this)};
  __property UnicodeString SftpServer = { read = FSftpServer, write = SetSftpServer };
  __property int Timeout = { read = FTimeout, write = SetTimeout };
  RWPropertySimple<int32_t> Timeout{&FTimeout, nb::bind(&TSessionData::SetTimeout, this)};
  __property TDateTime TimeoutDT = { read = GetTimeoutDT };
  __property bool UnsetNationalVars = { read = FUnsetNationalVars, write = SetUnsetNationalVars };
  __property bool IgnoreLsWarnings = { read = FIgnoreLsWarnings, write = SetIgnoreLsWarnings };
  __property bool TcpNoDelay  = { read = FTcpNoDelay, write = SetTcpNoDelay };
  __property int SendBuf  = { read = FSendBuf, write = SetSendBuf };
  __property UnicodeString SourceAddress = { read=FSourceAddress, write=SetSourceAddress };
  __property UnicodeString ProtocolFeatures = { read=FProtocolFeatures, write=SetProtocolFeatures };
  __property bool SshSimple = { read = FSshSimple, write = SetSshSimple };
  __property UnicodeString CipherList = { read = GetCipherList, write = SetCipherList };
  RWProperty<UnicodeString> CipherList{nb::bind(&TSessionData::GetCipherList, this), nb::bind(&TSessionData::SetCipherList, this)};
  __property UnicodeString KexList  = { read = GetKexList, write = SetKexList };
  RWProperty<UnicodeString> KexList{nb::bind(&TSessionData::GetKexList, this), nb::bind(&TSessionData::SetKexList, this)};
  __property UnicodeString HostKeyList  = { read = GetHostKeyList, write = SetHostKeyList };
  RWProperty<UnicodeString> HostKeyList{nb::bind(&TSessionData::GetHostKeyList, this), nb::bind(&TSessionData::SetHostKeyList, this)};
  __property UnicodeString GssLibList  = { read = GetGssLibList, write = SetGssLibList };
  RWProperty<UnicodeString> GssLibList{nb::bind(&TSessionData::GetGssLibList, this), nb::bind(&TSessionData::SetGssLibList, this)};
  __property TProxyMethod ProxyMethod  = { read = FProxyMethod, write = SetProxyMethod };
  __property UnicodeString ProxyHost  = { read = FProxyHost, write = SetProxyHost };
  __property int ProxyPort  = { read = FProxyPort, write = SetProxyPort };
  __property UnicodeString ProxyUsername  = { read = FProxyUsername, write = SetProxyUsername };
  __property UnicodeString ProxyPassword  = { read = GetProxyPassword, write = SetProxyPassword };
  __property UnicodeString ProxyTelnetCommand  = { read = FProxyTelnetCommand, write = SetProxyTelnetCommand };
  __property UnicodeString ProxyLocalCommand  = { read = FProxyLocalCommand, write = SetProxyLocalCommand };
  __property TAutoSwitch ProxyDNS  = { read = FProxyDNS, write = SetProxyDNS };
  __property bool ProxyLocalhost  = { read = FProxyLocalhost, write = SetProxyLocalhost };
  __property int FtpProxyLogonType  = { read = FFtpProxyLogonType, write = SetFtpProxyLogonType };
  __property TAutoSwitch Bug[TSshBug Bug]  = { read = GetBug, write = SetBug };
  __property UnicodeString PuttySettings = { read = FPuttySettings, write = SetPuttySettings };
  __property UnicodeString CustomParam1 = { read = FCustomParam1, write = SetCustomParam1 };
  __property UnicodeString CustomParam2 = { read = FCustomParam2, write = SetCustomParam2 };
  __property UnicodeString SessionKey = { read = GetSessionKey };
  __property bool ResolveSymlinks = { read = FResolveSymlinks, write = SetResolveSymlinks };
  RWPropertySimple<bool> ResolveSymlinks{&FResolveSymlinks, nb::bind(&TSessionData::SetResolveSymlinks, this)};
  __property bool FollowDirectorySymlinks = { read = FFollowDirectorySymlinks, write = SetFollowDirectorySymlinks };
  RWPropertySimple<bool> FollowDirectorySymlinks{&FFollowDirectorySymlinks, nb::bind(&TSessionData::SetFollowDirectorySymlinks, this)};
  __property int SFTPDownloadQueue = { read = FSFTPDownloadQueue, write = SetSFTPDownloadQueue };
  __property int SFTPUploadQueue = { read = FSFTPUploadQueue, write = SetSFTPUploadQueue };
  __property int SFTPListingQueue = { read = FSFTPListingQueue, write = SetSFTPListingQueue };
  __property int SFTPMaxVersion = { read = FSFTPMaxVersion, write = SetSFTPMaxVersion };
  __property uint32_t SFTPMaxPacketSize = { read = FSFTPMaxPacketSize, write = SetSFTPMaxPacketSize };
  __property TAutoSwitch SFTPRealPath = { read = FSFTPRealPath, write = SetSFTPRealPath };
  __property TAutoSwitch SFTPBug[TSftpBug Bug]  = { read = GetSFTPBug, write = SetSFTPBug };
  __property TAutoSwitch SCPLsFullTime = { read = FSCPLsFullTime, write = SetSCPLsFullTime };
  __property TAutoSwitch FtpListAll = { read = FFtpListAll, write = SetFtpListAll };
  __property TAutoSwitch FtpHost = { read = FFtpHost, write = SetFtpHost };
  __property TAutoSwitch FtpWorkFromCwd = { read = FFtpWorkFromCwd, write = SetFtpWorkFromCwd };
  __property bool FtpAnyCodeForPwd = { read = FFtpAnyCodeForPwd, write = SetFtpAnyCodeForPwd };
  __property bool SslSessionReuse = { read = FSslSessionReuse, write = SetSslSessionReuse };
  __property UnicodeString TlsCertificateFile = { read = FTlsCertificateFile, write = SetTlsCertificateFile };
  RWProperty<UnicodeString> TlsCertificateFile{nb::bind(&TSessionData::GetTlsCertificateFile, this), nb::bind(&TSessionData::SetTlsCertificateFile, this)};
  __property TDSTMode DSTMode = { read = FDSTMode, write = SetDSTMode };
  RWPropertySimple<TDSTMode> DSTMode{&FDSTMode, nb::bind(&TSessionData::SetDSTMode, this)};
  __property bool DeleteToRecycleBin = { read = FDeleteToRecycleBin, write = SetDeleteToRecycleBin };
  __property bool OverwrittenToRecycleBin = { read = FOverwrittenToRecycleBin, write = SetOverwrittenToRecycleBin };
  __property UnicodeString RecycleBinPath = { read = FRecycleBinPath, write = SetRecycleBinPath };
  __property UnicodeString PostLoginCommands = { read = FPostLoginCommands, write = SetPostLoginCommands };
  __property TAddressFamily AddressFamily = { read = FAddressFamily, write = SetAddressFamily };
  RWProperty<TAddressFamily> AddressFamily{nb::bind(&TSessionData::GetAddressFamily, this), nb::bind(&TSessionData::SetAddressFamily, this)};
  __property UnicodeString RekeyData = { read = FRekeyData, write = SetRekeyData };
  RWPropertySimple<UnicodeString> RekeyData{&FRekeyData, nb::bind(&TSessionData::SetRekeyData, this)};
  __property uint32_t RekeyTime = { read = FRekeyTime, write = SetRekeyTime };
  RWPropertySimple<uint32_t> RekeyTime{&FRekeyTime, nb::bind(&TSessionData::SetRekeyTime, this)};
  __property int Color = { read = FColor, write = SetColor };
  __property bool Tunnel = { read = FTunnel, write = SetTunnel };
  __property UnicodeString TunnelHostName = { read = FTunnelHostName, write = SetTunnelHostName };
  __property int TunnelPortNumber = { read = FTunnelPortNumber, write = SetTunnelPortNumber };
  __property UnicodeString TunnelUserName = { read = FTunnelUserName, write = SetTunnelUserName };
  __property UnicodeString TunnelPassword = { read = GetTunnelPassword, write = SetTunnelPassword };
  __property UnicodeString TunnelPublicKeyFile = { read = FTunnelPublicKeyFile, write = SetTunnelPublicKeyFile };
  __property UnicodeString TunnelPassphrase = { read = GetTunnelPassphrase, write = SetTunnelPassphrase };
  __property bool TunnelAutoassignLocalPortNumber = { read = GetTunnelAutoassignLocalPortNumber };
  __property int TunnelLocalPortNumber = { read = FTunnelLocalPortNumber, write = SetTunnelLocalPortNumber };
  __property UnicodeString TunnelPortFwd = { read = FTunnelPortFwd, write = SetTunnelPortFwd };
  __property UnicodeString TunnelHostKey = { read = FTunnelHostKey, write = SetTunnelHostKey };
  __property bool FtpPasvMode = { read = FFtpPasvMode, write = SetFtpPasvMode };
  __property TAutoSwitch FtpForcePasvIp = { read = FFtpForcePasvIp, write = SetFtpForcePasvIp };
  __property TAutoSwitch FtpUseMlsd = { read = FFtpUseMlsd, write = SetFtpUseMlsd };
  __property UnicodeString FtpAccount = { read = FFtpAccount, write = SetFtpAccount };
  __property int FtpPingInterval  = { read = FFtpPingInterval, write = SetFtpPingInterval };
  __property TDateTime FtpPingIntervalDT  = { read = GetFtpPingIntervalDT };
  __property TPingType FtpPingType = { read = FFtpPingType, write = SetFtpPingType };
  __property TAutoSwitch FtpTransferActiveImmediately = { read = FFtpTransferActiveImmediately, write = SetFtpTransferActiveImmediately };
  __property TFtps Ftps = { read = FFtps, write = SetFtps };
  RWProperty<TFtps> Ftps{nb::bind(&TSessionData::GetFtps, this), nb::bind(&TSessionData::SetFtps, this)};
  __property TTlsVersion MinTlsVersion = { read = FMinTlsVersion, write = SetMinTlsVersion };
  __property TTlsVersion MaxTlsVersion = { read = FMaxTlsVersion, write = SetMaxTlsVersion };
  __property UnicodeString LogicalHostName = { read = FLogicalHostName, write = SetLogicalHostName };
  RWProperty<UnicodeString> LogicalHostName{nb::bind(&TSessionData::GetLogicalHostName, this), nb::bind(&TSessionData::SetLogicalHostName, this)};
  __property TAutoSwitch NotUtf = { read = FNotUtf, write = SetNotUtf };
  __property int InternalEditorEncoding = { read = FInternalEditorEncoding, write = SetInternalEditorEncoding };
  RWPropertySimple<int32_t> InternalEditorEncoding{&FInternalEditorEncoding, nb::bind(&TSessionData::SetInternalEditorEncoding, this)};
  __property UnicodeString S3DefaultRegion = { read = FS3DefaultRegion, write = SetS3DefaultRegion };
  RWProperty<UnicodeString> S3DefaultRegion{nb::bind(&TSessionData::GetS3DefaultRegion, this), nb::bind(&TSessionData::SetS3DefaultRegion, this)};
  __property UnicodeString S3SessionToken = { read = FS3SessionToken, write = SetS3SessionToken };
  RWPropertySimple<UnicodeString> S3SessionToken{&FS3SessionToken, nb::bind(&TSessionData::SetS3SessionToken, this)};
  __property UnicodeString S3Profile = { read = FS3Profile, write = SetS3Profile };
  RWPropertySimple<UnicodeString> S3Profile{&FS3Profile, nb::bind(&TSessionData::SetS3Profile, this) };
  __property TS3UrlStyle S3UrlStyle = { read = FS3UrlStyle, write = SetS3UrlStyle };
  __property TAutoSwitch S3MaxKeys = { read = FS3MaxKeys, write = SetS3MaxKeys };
  __property bool S3CredentialsEnv = { read = FS3CredentialsEnv, write = SetS3CredentialsEnv };
  __property bool IsWorkspace = { read = FIsWorkspace, write = SetIsWorkspace };
  __property UnicodeString Link = { read = FLink, write = SetLink };
  __property UnicodeString NameOverride = { read = FNameOverride, write = SetNameOverride };
  RWProperty<UnicodeString> NameOverride{nb::bind(&TSessionData::GetNameOverride, this), nb::bind(&TSessionData::SetNameOverride, this)};
  __property UnicodeString HostKey = { read = FHostKey, write = SetHostKey };
  RWProperty<UnicodeString> HostKey{nb::bind(&TSessionData::GetHostKey, this), nb::bind(&TSessionData::SetHostKey, this)};
  __property bool FingerprintScan = { read = FFingerprintScan, write = FFingerprintScan };
  RWProperty2<bool> FingerprintScan{&FFingerprintScan};
  __property bool OverrideCachedHostKey = { read = FOverrideCachedHostKey };
  __property UnicodeString Note = { read = FNote, write = SetNote };
  __property UnicodeString WinTitle = { read = FWinTitle, write = SetWinTitle };
  __property UnicodeString EncryptKey = { read = GetEncryptKey, write = SetEncryptKey };
  RWProperty<UnicodeString> EncryptKey{nb::bind(&TSessionData::GetEncryptKey, this), nb::bind(&TSessionData::SetEncryptKey, this)};
  __property bool WebDavLiberalEscaping = { read = FWebDavLiberalEscaping, write = SetWebDavLiberalEscaping };

  __property UnicodeString StorageKey = { read = GetStorageKey };
  ROProperty<UnicodeString> StorageKey{nb::bind(&TSessionData::GetStorageKey, this)};
  __property UnicodeString SiteKey = { read = GetSiteKey };
  ROProperty<UnicodeString> SiteKey{nb::bind(&TSessionData::GetSiteKey, this)};
  __property UnicodeString OrigHostName = { read = FOrigHostName };
  const UnicodeString& OrigHostName{FOrigHostName};
  __property int OrigPortNumber = { read = FOrigPortNumber };
  const int32_t& OrigPortNumber{FOrigPortNumber};
  __property UnicodeString LocalName = { read = GetLocalName };
  ROProperty<UnicodeString> LocalName{nb::bind(&TSessionData::GetLocalName, this)};
  __property UnicodeString FolderName = { read = GetFolderName };
  ROProperty<UnicodeString> FolderName{nb::bind(&TSessionData::GetFolderName, this)};
  __property UnicodeString Source = { read = GetSource };
  ROProperty<UnicodeString> Source{nb::bind(&TSessionData::GetSource, this)};
  __property bool SaveOnly = { read = FSaveOnly };
  const bool& SaveOnly{FSaveOnly};

public:
  void SetSFTPMinPacketSize(int32_t Value);
  void SetFingerprintScan(bool Value) { FFingerprintScan = Value; }
  bool GetSaveOnly() const { return FSaveOnly; }
  bool GetChangePassword() const { return FChangePassword; }
  UnicodeString GetGssLibCustom() const { return FGssLibCustom; }
  void SetFtpDupFF(bool Value);
  void SetFtpUndupFF(bool Value);
  UnicodeString GetWinTitle() const { return FWinTitle; }

  bool GetTimeDifferenceAuto() const { return FTimeDifferenceAuto; }
  UnicodeString GetNote() const { return FNote; }
  UnicodeString GetProtocolStr() const;
  void SetProtocolStr(UnicodeString Value);

  UnicodeString GetHostName() const { return FHostName; }
  int32_t GetPortNumber() const { return FPortNumber; }
  TLoginType GetLoginType() const;
  void SetLoginType(TLoginType Value);
  UnicodeString SessionGetUserName() const { return FUserName; }
  int32_t GetPingInterval() const { return FPingInterval; }
  bool GetTryAgent() const { return FTryAgent; }
  bool GetAgentFwd() const { return FAgentFwd; }
  UnicodeString GetListingCommand() const { return FListingCommand; }
  bool GetAuthKI() const { return FAuthKI; }
  bool GetAuthKIPassword() const { return FAuthKIPassword; }
  bool GetAuthGSSAPI() const { return FAuthGSSAPI; }
  bool GetGSSAPIFwdTGT() const { return FGSSAPIFwdTGT; }
  bool GetChangeUsername() const { return FChangeUsername; }
  bool GetCompression() const { return FCompression; }
  bool GetSsh2DES() const { return FSsh2DES; }
  bool GetSshNoUserAuth() const { return FSshNoUserAuth; }
  UnicodeString GetPublicKeyFile() const { return FPublicKeyFile; }
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
  bool GetSpecial() const { return FSpecial; }
  bool GetSelected() const { return FSelected; }
  void SetSelected(bool Value) { FSelected = Value; }
  TEOLType GetEOLType() const { return FEOLType; }
  bool GetTrimVMSVersions() const { return FTrimVMSVersions; }
  TAutoSwitch GetLookupUserGroups() const { return FLookupUserGroups; }
  UnicodeString GetReturnVar() const { return FReturnVar; }
  bool GetExitCode1IsError() const { return FExitCode1IsError; }
  bool GetScp1Compatibility() const { return FScp1Compatibility; }
  UnicodeString GetShell() const { return FShell; }
  UnicodeString GetSftpServer() const { return FSftpServer; }
  int32_t GetTimeout() const { return FTimeout; }
  bool GetUnsetNationalVars() const { return FUnsetNationalVars; }
  bool GetIgnoreLsWarnings() const { return FIgnoreLsWarnings; }
  bool GetTcpNoDelay() const { return FTcpNoDelay; }
  int32_t GetSendBuf() const { return FSendBuf; }
  bool GetSshSimple() const { return FSshSimple; }
  TProxyMethod GetProxyMethod() const { return FProxyMethod; }
  TProxyMethod GetActualProxyMethod() const;
  UnicodeString GetProxyHost() const;
  int32_t GetProxyPort() const;
  UnicodeString GetProxyUsername() const;
  UnicodeString GetProxyTelnetCommand() const { return FProxyTelnetCommand; }
  UnicodeString GetProxyLocalCommand() const { return FProxyLocalCommand; }
  TAutoSwitch GetProxyDNS() const { return FProxyDNS; }
  bool GetProxyLocalhost() const { return FProxyLocalhost; }
  int32_t GetFtpProxyLogonType() const { return FFtpProxyLogonType; }
  UnicodeString GetCustomParam1() const { return FCustomParam1; }
  UnicodeString GetCustomParam2() const { return FCustomParam2; }
  bool GetResolveSymlinks() const { return FResolveSymlinks; }
  bool GetFollowDirectorySymlinks() const { return FFollowDirectorySymlinks; }
  int32_t GetSFTPDownloadQueue() const { return FSFTPDownloadQueue; }
  int32_t GetSFTPUploadQueue() const { return FSFTPUploadQueue; }
  int32_t GetSFTPListingQueue() const { return FSFTPListingQueue; }
  int32_t GetSFTPMaxVersion() const { return FSFTPMaxVersion; }
  int32_t GetSFTPMinPacketSize() const { return FSFTPMinPacketSize; }
  int32_t GetSFTPMaxPacketSize() const { return FSFTPMaxPacketSize; }
  TAutoSwitch GetSCPLsFullTime() const { return FSCPLsFullTime; }
  TAutoSwitch GetFtpListAll() const { return FFtpListAll; }
  TAutoSwitch GetFtpHost() const { return FFtpHost; }
  bool GetFtpDupFF() const { return FFtpDupFF; }
  bool GetFtpUndupFF() const { return FFtpUndupFF; }
  bool GetSslSessionReuse() const { return FSslSessionReuse; }
  UnicodeString GetTlsCertificateFile() const { return FTlsCertificateFile; }
  TDSTMode GetDSTMode() const { return FDSTMode; }
  bool GetDeleteToRecycleBin() const { return FDeleteToRecycleBin; }
  bool GetOverwrittenToRecycleBin() const { return FOverwrittenToRecycleBin; }
  UnicodeString GetRecycleBinPath() const { return FRecycleBinPath; }
  UnicodeString GetPostLoginCommands() const { return FPostLoginCommands; }
  TAddressFamily GetAddressFamily() const { return FAddressFamily; }
  UnicodeString GetCodePage() const { return FCodePage; }
  void SetCodePage(UnicodeString Value);
  uint32_t GetCodePageAsNumber() const;
  UnicodeString GetRekeyData() const { return FRekeyData; }
  uint32_t GetRekeyTime() const { return FRekeyTime; }
  int32_t GetColor() const { return FColor; }
  bool GetTunnel() const { return FTunnel; }
  UnicodeString GetTunnelHostName() const { return FTunnelHostName; }
  int32_t GetTunnelPortNumber() const { return FTunnelPortNumber; }
  UnicodeString GetTunnelUserName() const { return FTunnelUserName; }
  UnicodeString GetTunnelPublicKeyFile() const { return FTunnelPublicKeyFile; }
  int32_t GetTunnelLocalPortNumber() const { return FTunnelLocalPortNumber; }
  UnicodeString GetTunnelPortFwd() const { return FTunnelPortFwd; }
  UnicodeString GetTunnelHostKey() const { return FTunnelHostKey; }
  bool GetFtpPasvMode() const { return FFtpPasvMode; }
  bool GetFtpAllowEmptyPassword() const { return FFtpAllowEmptyPassword; }
  void SetFtpAllowEmptyPassword(bool Value);
  TAutoSwitch GetFtpForcePasvIp() const { return FFtpForcePasvIp; }
  TAutoSwitch GetFtpUseMlsd() const { return FFtpUseMlsd; }
  UnicodeString GetFtpAccount() const { return FFtpAccount; }
  int32_t GetFtpPingInterval() const { return FFtpPingInterval; }
  TPingType GetFtpPingType() const { return FFtpPingType; }
  TAutoSwitch GetFtpTransferActiveImmediately() const { return FFtpTransferActiveImmediately; }
  TFtps GetFtps() const { return FFtps; }
  TTlsVersion GetMinTlsVersion() const { return FMinTlsVersion; }
  TTlsVersion GetMaxTlsVersion() const { return FMaxTlsVersion; }
  TAutoSwitch GetNotUtf() const { return FNotUtf; }
  int32_t GetInternalEditorEncoding() const { return FInternalEditorEncoding; }
  UnicodeString GetS3DefaultRegion() const { return FS3DefaultRegion; }
  bool GetIsWorkspace() const { return FIsWorkspace; }
  UnicodeString GetLink() const { return FLink; }
  UnicodeString GetNameOverride() const { return FNameOverride; }
  UnicodeString GetHostKey() const { return FHostKey; }
  bool GetFingerprintScan() const { return FFingerprintScan; }
  bool GetOverrideCachedHostKey() const { return FOverrideCachedHostKey; }
  UnicodeString GetOrigHostName() const { return FOrigHostName; }
  UnicodeString GetLogicalHostName() const { return FLogicalHostName; }
  int32_t GetOrigPortNumber() const { return FOrigPortNumber; }
  void SetPasswordless(bool Value);

  int32_t GetNumberOfRetries() const { return FNumberOfRetries; }
  void SetNumberOfRetries(int32_t Value) { FNumberOfRetries = Value; }
  uint32_t GetSessionVersion() const { return FSessionVersion; }
  void SetSessionVersion(uint32_t Value) { FSessionVersion = Value; }
  void RemoveProtocolPrefix(UnicodeString &HostName) const;
  static void AddSwitchValue(UnicodeString &Result, UnicodeString Name, UnicodeString Value);

private:
  uint32_t GetDefaultVersion() const { return ::GetCurrentVersionNumber(); }
  TFSProtocol TranslateFSProtocolNumber(int32_t FSProtocol);
  TFSProtocol TranslateFSProtocol(UnicodeString ProtocolID) const;
  TFtps TranslateFtpEncryptionNumber(int32_t FtpEncryption) const;

  TProxyMethod GetSystemProxyMethod() const;
  void PrepareProxyData() const;
  void ParseIEProxyConfig() const;
  void FromURI(UnicodeString ProxyURI,
    UnicodeString &ProxyUrl, int32_t &ProxyPort, TProxyMethod &ProxyMethod) const;
  void AdjustHostName(UnicodeString &HostName, UnicodeString Prefix) const;

private:
  int32_t FSFTPMinPacketSize{0};
  bool FFtpDupFF{false};
  bool FFtpUndupFF{false};
  bool FTunnelConfigured{false};
  UnicodeString FCodePage;
  mutable uint32_t FCodePageAsNumber{0};
  bool FFtpAllowEmptyPassword{false};
  TLoginType FLoginType{};
  int32_t FNumberOfRetries{0};
  uint32_t FSessionVersion{0};

  mutable TIEProxyConfig *FIEProxyConfig{nullptr};
};

NB_DEFINE_CLASS_ID(TStoredSessionList);
class NB_CORE_EXPORT TStoredSessionList : public TNamedObjectList
{
  NB_DISABLE_COPY(TStoredSessionList)
public:
  TStoredSessionList() noexcept;
  explicit TStoredSessionList(bool aReadOnly) noexcept;
  void Reload();
  void Save(bool All, bool Explicit);
  void Saved();
  void ImportFromFilezilla(const UnicodeString FileName, const UnicodeString ConfigurationFileName);
  void ImportFromKnownHosts(TStrings * Lines);
  void ImportFromOpenssh(TStrings * Lines);
  void Export(const UnicodeString FileName);
  void Load(THierarchicalStorage * Storage, bool AsModified = false,
    bool UseDefaults = false, bool PuttyImport = false);
  void Save(THierarchicalStorage * Storage, bool All = false);
  void SelectAll(bool Select);
  void Import(TStoredSessionList * From, bool OnlySelected, TList * Imported);
  void RecryptPasswords(TStrings * RecryptPasswordErrors);
  TSessionData * AtSession(int Index)
    { return static_cast<TSessionData *>(AtObject(Index)); }
  void SelectSessionsToImport(TStoredSessionList * Dest, bool SSHOnly);
  void Cleanup();
  void UpdateStaticUsage();
  int32_t IndexOf(TSessionData * Data) const;
  const TSessionData * FindSame(TSessionData * Data);
  TSessionData * NewSession(UnicodeString SessionName, TSessionData * Session);
  void NewWorkspace(const UnicodeString Name, TList * DataList);
  bool GetIsFolder(const UnicodeString Name) const;
  bool GetIsWorkspace(const UnicodeString Name) const;
  TSessionData * ParseUrl(UnicodeString Url, TOptions * Options, bool & DefaultsOnly,
    UnicodeString *AFileName = nullptr, bool *AProtocolDefined = nullptr, UnicodeString *MaskedUrl = nullptr, int32_t Flags = 0);
  bool IsUrl(UnicodeString Url);
  bool CanOpen(TSessionData * Data);
  void GetFolderOrWorkspace(const UnicodeString Name, TList * List);
  TStrings * GetFolderOrWorkspaceList(const UnicodeString Name);
  TStrings * GetWorkspaces() const;
  bool HasAnyWorkspace() const;
  TSessionData * SaveWorkspaceData(TSessionData * Data, int Index);
  virtual ~TStoredSessionList() noexcept;
  __property TSessionData *Sessions[int Index]  = { read = AtSession };
  __property TSessionData *DefaultSettings  = { read = FDefaultSettings, write = SetDefaultSettings };
  RWProperty<const TSessionData *> DefaultSettings{nb::bind(&TStoredSessionList::GetDefaultSettingsConst, this), nb::bind(&TStoredSessionList::SetDefaultSettings, this)};

  static int32_t ImportHostKeys(
    THierarchicalStorage * SourceStorage, THierarchicalStorage * TargetStorage, TStoredSessionList * Sessions, bool OnlySelected);
  static void ImportHostKeys(
    const UnicodeString & SourceKey, TStoredSessionList * Sessions, bool OnlySelected);
  static void ImportSelectedKnownHosts(TStoredSessionList * Sessions);
  static bool OpenHostKeysSubKey(THierarchicalStorage * Storage, bool CanCreate);
  static void SelectKnownHostsForSelectedSessions(TStoredSessionList * KnownHosts, TStoredSessionList * Sessions);

  const TSessionData *GetSession(int32_t Index) const { return dyn_cast<TSessionData>(AtObject(Index)); }
  TSessionData *GetSession(int32_t Index) { return dyn_cast<TSessionData>(AtObject(Index)); }
  const TSessionData *GetDefaultSettingsConst() const { return FDefaultSettings.get(); }
  TSessionData *GetDefaultSettings() { return FDefaultSettings.get(); }
  void SetDefaultSettings(const TSessionData *Value);
  const TSessionData *GetSessionByName(UnicodeString SessionName) const;
private:
  std::unique_ptr<TSessionData> FDefaultSettings;
  bool FReadOnly{false};
  std::unique_ptr<TStrings> FPendingRemovals;
  __removed void SetDefaultSettings(TSessionData * value);
  void DoSave(THierarchicalStorage * Storage, bool All,
    bool RecryptPasswordOnly, TStrings *RecryptPasswordErrors);
  void DoSave(bool All, bool Explicit, bool RecryptPasswordOnly,
    TStrings *RecryptPasswordErrors);
  void DoSave(THierarchicalStorage * Storage,
    TSessionData *Data, bool All, bool RecryptPasswordOnly,
    TSessionData *FactoryDefaults);
  TSessionData * ResolveWorkspaceData(TSessionData * Data);
  bool IsFolderOrWorkspace(const UnicodeString Name, bool Workspace) const;
  TSessionData * CheckIsInFolderOrWorkspaceAndResolve(
    TSessionData *Data, const UnicodeString Name);
  __removed void ImportLevelFromFilezilla(_di_IXMLNode Node, const UnicodeString Path, _di_IXMLNode SettingsNode);
  void DoGetFolderOrWorkspace(const UnicodeString & Name, TList * List, bool NoRecrypt);
  static THierarchicalStorage * CreateHostKeysStorageForWriting();
};

NB_CORE_EXPORT UnicodeString GetExpandedLogFileName(UnicodeString LogFileName, TDateTime Started, TSessionData *SessionData);
NB_CORE_EXPORT bool GetIsSshProtocol(TFSProtocol FSProtocol);
NB_CORE_EXPORT int32_t DefaultPort(TFSProtocol FSProtocol, TFtps Ftps);
NB_CORE_EXPORT bool IsIPv6Literal(const UnicodeString HostName);
NB_CORE_EXPORT UnicodeString EscapeIPv6Literal(const UnicodeString IP);
NB_CORE_EXPORT TFSProtocol NormalizeFSProtocol(TFSProtocol FSProtocol);

NB_CORE_EXPORT bool GetCodePageInfo(UINT CodePage, CPINFOEX &CodePageInfoEx);
NB_CORE_EXPORT uint32_t GetCodePageAsNumber(UnicodeString CodePage);
NB_CORE_EXPORT UnicodeString GetCodePageAsString(uint32_t CodePage);

//template<int s> struct CheckSizeT;
//CheckSizeT<sizeof(TSessionData)> checkSize;
