
#pragma once

#include <Common.h>
#include <FileBuffer.h>

#include "Option.h"
#include "NamedObjs.h"
#include "HierarchicalStorage.h"
#include "Configuration.h"
#include <Xml.XMLIntf.hpp>

enum TCipher { cipWarn, cip3DES, cipBlowfish, cipAES, cipDES, cipArcfour, cipChaCha20, cipAESGCM, cipCount };
constexpr const int32_t CIPHER_COUNT = cipCount;
// explicit values to skip obsoleted fsExternalSSH, fsExternalSFTP
enum TFSProtocol { fsSCPonly = 0, fsSFTP = 1, fsSFTPonly = 2, fsFTP = 5, fsWebDAV = 6, fsS3 = 7 };
constexpr const int32_t FSPROTOCOL_COUNT = fsS3 + 1;
constexpr const wchar_t * ProxyMethodNames = L"None;SOCKS4;SOCKS5;HTTP;Telnet;Cmd";
enum TProxyMethod { pmNone, pmSocks4, pmSocks5, pmHTTP, pmTelnet, pmCmd };
enum TKex { kexWarn, kexDHGroup1, kexDHGroup14, kexDHGroup15, kexDHGroup16, kexDHGroup17, kexDHGroup18, kexDHGEx, kexRSA, kexECDH, kexNTRUHybrid, kexCount };
constexpr const int32_t KEX_COUNT = kexCount;
enum THostKey { hkWarn, hkRSA, hkDSA, hkECDSA, hkED25519, hkED448, hkCount };
constexpr const int32_t HOSTKEY_COUNT = hkCount;
enum TGssLib { gssGssApi32, gssSspi, gssCustom };
constexpr const int32_t GSSLIB_COUNT = gssCustom + 1;
// names have to match PuTTY registry entries (see settings.c)
enum TSshBug { sbHMAC2, sbDeriveKey2, sbRSAPad2,
  sbPKSessID2, sbRekey2, sbMaxPkt2, sbIgnore2, sbOldGex2, sbWinAdj, sbChanReq };
constexpr const int32_t BUG_COUNT = sbChanReq + 1;
enum TSftpBug { sbSymlink, sbSignedTS };
constexpr const int32_t SFTP_BUG_COUNT = sbSignedTS + 1;
constexpr const wchar_t * PingTypeNames = L"Off;Null;Dummy";
enum TPingType { ptOff, ptNullPacket, ptDummyCommand };
constexpr const wchar_t * FtpPingTypeNames = L"Off;Dummy;Dummy;List";
enum TFtpPingType { fptOff, fptDummyCommand0, fptDummyCommand, fptDirectoryListing };
enum TAddressFamily { afAuto, afIPv4, afIPv6 };
enum TFtps { ftpsNone, ftpsImplicit, ftpsExplicitSsl, ftpsExplicitTls };
// ssl2 and ssh3 are equivalent of tls10 now
enum TTlsVersion { ssl2 = 2, ssl3 = 3, tls10 = 10, tls11 = 11, tls12 = 12, tls13 = 13, tlsMin = tls10, tlsDefaultMin = tls12, tlsMax = tls13 };
// has to match libs3 S3UriStyle
enum TS3UrlStyle { s3usVirtualHost, s3usPath };
enum TSessionSource { ssNone, ssStored, ssStoredModified };
constexpr const int32_t SFTPMaxVersionAuto = -1;
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
  sufOpen = sufUserName | sufPassword
};
enum TParseUrlFlags
{
  pufAllowStoredSiteWithProtocol = 0x01,
  pufUnsafe = 0x02,
  pufPreferProtocol = 0x04,
  pufParseOnly = 0x08,
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

constexpr const wchar_t * CipherNames[CIPHER_COUNT] = {L"WARN", L"3des", L"blowfish", L"aes", L"des", L"arcfour", L"chacha20", L"aesgcm"};
constexpr const wchar_t * KexNames[KEX_COUNT] = {L"WARN", L"dh-group1-sha1", L"dh-group14-sha1", L"dh-group15-sha512", L"dh-group16-sha512", L"dh-group17-sha512", L"dh-group18-sha512", L"dh-gex-sha1", L"rsa", L"ecdh", L"ntru-curve25519"};
constexpr const wchar_t * HostKeyNames[HOSTKEY_COUNT] = {L"WARN", L"rsa", L"dsa", L"ecdsa", L"ed25519", L"ed448"};
constexpr const wchar_t * GssLibNames[GSSLIB_COUNT] = {L"gssapi32", L"sspi", L"custom"};
constexpr const TCipher DefaultCipherList[CIPHER_COUNT] =
  { cipAES, cipChaCha20, cipAESGCM, cip3DES, cipWarn, cipDES, cipBlowfish, cipArcfour };
// Update also order in SshKexList()
constexpr const TKex DefaultKexList[KEX_COUNT] =
  { kexNTRUHybrid, kexECDH, kexDHGEx, kexDHGroup18, kexDHGroup17, kexDHGroup16, kexDHGroup15, kexDHGroup14, kexRSA, kexWarn, kexDHGroup1 };
constexpr const THostKey DefaultHostKeyList[HOSTKEY_COUNT] =
  { hkED448, hkED25519, hkECDSA, hkRSA, hkDSA, hkWarn };
constexpr const TGssLib DefaultGssLibList[GSSLIB_COUNT] =
  { gssGssApi32, gssSspi, gssCustom };
constexpr const wchar_t * FSProtocolNames[FSPROTOCOL_COUNT] =
  { L"SCP", L"SFTP (SCP)", L"SFTP", L"", L"", L"FTP", L"WebDAV", L"S3" };
constexpr const wchar_t * AnonymousUserName = L"anonymous";
constexpr const wchar_t * AnonymousPassword = L"anonymous@example.com";
constexpr const int32_t SshPortNumber = 22;
constexpr const int32_t FtpPortNumber = 21;
constexpr const int32_t FtpsImplicitPortNumber = 990;
constexpr const int32_t HTTPPortNumber = 80;
constexpr const int32_t HTTPSPortNumber = 443;
constexpr const int32_t TelnetPortNumber = 23;
constexpr const int32_t DefaultSendBuf = 256 * 1024;
constexpr const int32_t ProxyPortNumber = 80;
constexpr const wchar_t * PuttySshProtocol = L"ssh";
constexpr const wchar_t * PuttyTelnetProtocol = L"telnet";
constexpr const wchar_t * SftpProtocol = L"sftp";
constexpr const wchar_t * ScpProtocol = L"scp";
constexpr const wchar_t * FtpProtocol = L"ftp";
constexpr const wchar_t * FtpsProtocol = L"ftps";
constexpr const wchar_t * FtpesProtocol = L"ftpes";
constexpr const wchar_t * WebDAVProtocol = L"dav";
constexpr const wchar_t * WebDAVSProtocol = L"davs";
constexpr const wchar_t * WebDAVAltProtocol = L"webdav";
constexpr const wchar_t * S3Protocol = L"s3";
constexpr const wchar_t * S3PlainProtocol = L"s3plain";
constexpr const wchar_t * SshProtocol = L"ssh";
constexpr const wchar_t * WinSCPProtocolPrefix = L"winscp-";
constexpr const wchar_t UrlParamSeparator = L';';
constexpr const wchar_t UrlParamValueSeparator = L'=';
constexpr const wchar_t * UrlHostKeyParamName = L"fingerprint";
constexpr const wchar_t * UrlSaveParamName = L"save";
constexpr const wchar_t * UrlRawSettingsParamNamePrefix = L"x-";
constexpr const wchar_t * PassphraseOption = L"passphrase";
constexpr const wchar_t * RawSettingsOption = L"rawsettings";
constexpr const wchar_t * S3HostName = L"s3.amazonaws.com"; //S3_DEFAULT_HOSTNAME;
constexpr const wchar_t * S3GoogleCloudHostName = L"storage.googleapis.com";
constexpr const wchar_t * OpensshHostDirective = L"Host";
constexpr const uint32_t CONST_DEFAULT_CODEPAGE = CP_UTF8;
constexpr const TFSProtocol CONST_DEFAULT_PROTOCOL = fsSFTP;
class TStoredSessionList;
class TSecondaryTerminal;
class TFileZillaImpl;
class TFTPFileSystem;
class TSFTPFileSystem;
class TWebDAVFileSystem;
class TS3FileSystem;
class TSecureShell;
class TSessionLog;
struct TIEProxyConfig;

constexpr const int32_t SFTPMinVersion = 0;
constexpr const int32_t SFTPMaxVersion = 6;

class NB_CORE_EXPORT TSessionData final : public TNamedObject
{
  friend class TStoredSessionList;
  friend class TSecondaryTerminal;
  friend class TFileZillaImpl;
  friend class TFTPFileSystem;
  friend class TSFTPFileSystem;
  friend class TWebDAVFileSystem;
  friend class TS3FileSystem;
  friend class TTerminal;
  friend class TSecureShell;
  friend class TSessionLog;
  NB_DISABLE_COPY(TSessionData)
public:
  static bool classof(const TObject * Obj) { return Obj->is(OBJECT_CLASS_TSessionData); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TSessionData) || TNamedObject::is(Kind); }
private:
  UnicodeString FHostName;
  int32_t FPortNumber{0};
  UnicodeString FUserName;
  RawByteString FPassword;
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
  TEOLType FEOLType{eolLF};
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
  uint32_t FSFTPMaxVersion{SFTPMaxVersion};
  int32_t FSFTPMaxPacketSize{0};
  TAutoSwitch FSFTPRealPath{asAuto};
  bool FUsePosixRename{false};
  TDSTMode FDSTMode{dstmKeep};
  TAutoSwitch FSFTPBugs[SFTP_BUG_COUNT]{};
  bool FDeleteToRecycleBin{false};
  bool FOverwrittenToRecycleBin{false};
  UnicodeString FRecycleBinPath;
  UnicodeString FPostLoginCommands;
  TAutoSwitch FSCPLsFullTime{asAuto};
  TAutoSwitch FFtpListAll{asAuto};
  TAutoSwitch FFtpHost{asAuto};
  TAutoSwitch FFtpWorkFromCwd{asAuto};
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
  int32_t FTunnelLocalPortNumber{0};
  UnicodeString FTunnelPortFwd;
  UnicodeString FTunnelHostKey;
  bool FFtpPasvMode{false};
  TAutoSwitch FFtpForcePasvIp{};
  TAutoSwitch FFtpUseMlsd{asAuto};
  UnicodeString FFtpAccount;
  int32_t FFtpPingInterval{0};
  TFtpPingType FFtpPingType{fptOff};
  TAutoSwitch FFtpTransferActiveImmediately{};
  TFtps FFtps{ftpsNone};
  TTlsVersion FMinTlsVersion{};
  TTlsVersion FMaxTlsVersion{};
  TAutoSwitch FCompleteTlsShutdown{};
  TAutoSwitch FNotUtf{};
  int32_t FInternalEditorEncoding{0};
  UnicodeString FS3DefaultRegion;
  UnicodeString FS3SessionToken;
  UnicodeString FS3Profile;
  TS3UrlStyle FS3UrlStyle;
  TAutoSwitch FS3MaxKeys;
  bool FS3CredentialsEnv{false};
  bool FS3RequesterPays{false};
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
  bool FWebDavAuthLegacy{false};

  UnicodeString FOrigHostName;
  int32_t FOrigPortNumber{0};
  TProxyMethod FOrigProxyMethod{pmNone};
  TSessionSource FSource{ssNone};
  bool FSaveOnly{false};
  UnicodeString FLogicalHostName;

public:
  void SetHostName(const UnicodeString & AValue);
  UnicodeString GetHostNameExpanded() const;
  UnicodeString GetHostNameSource() const;
  void SetPortNumber(int32_t AValue);
  void SetUserName(const UnicodeString & AValue);
  UnicodeString GetUserNameExpanded() const;
  UnicodeString GetUserNameSource() const;
  void SetPassword(const UnicodeString & AValue);
  UnicodeString GetPassword() const;
  void SetNewPassword(const UnicodeString & AValue);
  UnicodeString GetNewPassword() const;
  void SetChangePassword(bool AValue);
  void SetPingInterval(int32_t AValue);
  void SetTryAgent(bool AValue);
  void SetAgentFwd(bool AValue);
  void SetAuthKI(bool AValue);
  void SetAuthKIPassword(bool AValue);
  void SetAuthGSSAPI(bool AValue);
  void SetAuthGSSAPIKEX(bool AValue);
  void SetGSSAPIFwdTGT(bool AValue);
  void SetChangeUsername(bool AValue);
  void SetCompression(bool AValue);
  void SetSsh2DES(bool AValue);
  void SetSshNoUserAuth(bool AValue);
  void SetCipher(int32_t Index, TCipher AValue);
  TCipher GetCipher(int32_t Index) const;
  void SetKex(int32_t Index, TKex AValue);
  TKex GetKex(int32_t Index) const;
  void SetHostKeys(int32_t Index, THostKey AValue);
  THostKey GetHostKeys(int32_t Index) const;
  void SetGssLib(int32_t Index, TGssLib AValue);
  TGssLib GetGssLib(int32_t Index) const;
  void SetGssLibCustom(const UnicodeString & AValue);
  void SetPublicKeyFile(const UnicodeString & AValue);
  UnicodeString GetPassphrase() const;
  void SetPassphrase(const UnicodeString & AValue);
  void SetDetachedCertificate(const UnicodeString & AValue);

  void SetPuttyProtocol(const UnicodeString & AValue);
  bool GetCanLogin() const;
  bool GetCanOpen() const;
  bool GetIsLocalBrowser() const;
  void SetPingIntervalDT(const TDateTime & AValue);
  TDateTime GetPingIntervalDT() const;
  TDateTime GetFtpPingIntervalDT() const;
  void SetTimeDifference(const TDateTime & AValue);
  void SetTimeDifferenceAuto(bool AValue);
  void SetPingType(TPingType AValue);
  UnicodeString GetSessionName() const;
  UnicodeString GetDefaultSessionName() const;
  UnicodeString GetProtocolUrl(bool HttpForWebDAV) const;
  void SetFSProtocol(TFSProtocol AValue);
  UnicodeString GetFSProtocolStr() const;
  void SetLocalDirectory(const UnicodeString & AValue);
  void SetOtherLocalDirectory(const UnicodeString & AValue);
  UnicodeString GetLocalDirectoryExpanded() const;
  void SetRemoteDirectory(const UnicodeString & AValue);
  void SetSynchronizeBrowsing(bool AValue);
  void SetUpdateDirectories(bool AValue);
  void SetCacheDirectories(bool AValue);
  void SetCacheDirectoryChanges(bool AValue);
  void SetPreserveDirectoryChanges(bool AValue);
  void SetSpecial(bool AValue);
  UnicodeString GetInfoTip() const;
  bool GetDefaultShell() const;
  void SetDetectReturnVar(bool AValue);
  bool GetDetectReturnVar() const;
  void SetListingCommand(const UnicodeString & AValue);
  void SetClearAliases(bool AValue);
  void SetDefaultShell(bool AValue);
  void SetEOLType(TEOLType AValue);
  void SetTrimVMSVersions(bool AValue);
  void SetVMSAllRevisions(bool AValue);
  void SetLookupUserGroups(TAutoSwitch AValue);
  void SetReturnVar(const UnicodeString & AValue);
  void SetExitCode1IsError(bool AValue);
  void SetScp1Compatibility(bool AValue);
  void SetShell(const UnicodeString & AValue);
  void SetSftpServer(const UnicodeString & AValue);
  void SetTimeout(int32_t AValue);
  void SetUnsetNationalVars(bool AValue);
  void SetIgnoreLsWarnings(bool AValue);
  void SetTcpNoDelay(bool AValue);
  void SetSendBuf(int32_t AValue);
  void SetSourceAddress(const UnicodeString & AValue);
  void SetProtocolFeatures(const UnicodeString & AValue);
  void SetSshSimple(bool AValue);
  bool GetUsesSsh() const;
  void SetCipherList(const UnicodeString & AValue);
  UnicodeString GetCipherList() const;
  void SetKexList(const UnicodeString & AValue);
  UnicodeString GetKexList() const;
  void SetHostKeyList(const UnicodeString & AValue);
  UnicodeString GetHostKeyList() const;
  void SetGssLibList(const UnicodeString & AValue);
  UnicodeString GetGssLibList() const;
  void SetProxyMethod(TProxyMethod AValue);
  void SetProxyHost(const UnicodeString & AValue);
  void SetProxyPort(int32_t AValue);
  void SetProxyUsername(const UnicodeString & AValue);
  void SetProxyPassword(const UnicodeString & AValue);
  void SetProxyTelnetCommand(const UnicodeString & AValue);
  void SetProxyLocalCommand(const UnicodeString & AValue);
  void SetProxyDNS(TAutoSwitch AValue);
  void SetProxyLocalhost(bool AValue);
  UnicodeString GetProxyPassword() const;
  void SetFtpProxyLogonType(int32_t AValue);
  void SetBug(TSshBug Bug, TAutoSwitch AValue);
  TAutoSwitch GetBug(TSshBug Bug) const;
  UnicodeString GetSessionKey() const;
  void SetPuttySettings(const UnicodeString & AValue);
  void SetCustomParam1(const UnicodeString & AValue);
  void SetCustomParam2(const UnicodeString & AValue);
  void SetResolveSymlinks(bool AValue);
  void SetFollowDirectorySymlinks(bool AValue);
  void SetSFTPDownloadQueue(int32_t AValue);
  void SetSFTPUploadQueue(int32_t AValue);
  void SetSFTPListingQueue(int32_t AValue);
  void SetSFTPMaxVersion(int32_t AValue);
  void SetSFTPMaxPacketSize(uint32_t AValue);
  void SetSFTPRealPath(TAutoSwitch AValue);
  void SetUsePosixRename(bool AValue);
  void SetSFTPBug(TSftpBug Bug, TAutoSwitch AValue);
  TAutoSwitch GetSFTPBug(TSftpBug Bug) const;
  void SetSCPLsFullTime(TAutoSwitch AValue);
  void SetFtpListAll(TAutoSwitch AValue);
  void SetFtpHost(TAutoSwitch AValue);
  void SetFtpWorkFromCwd(TAutoSwitch AValue);
  void SetFtpAnyCodeForPwd(bool AValue);
  void SetSslSessionReuse(bool AValue);
  void SetTlsCertificateFile(const UnicodeString & AValue);
  UnicodeString GetStorageKey() const;
  UnicodeString GetInternalStorageKey() const;
  UnicodeString GetSiteKey() const;
  void SetDSTMode(TDSTMode AValue);
  void SetDeleteToRecycleBin(bool AValue);
  void SetOverwrittenToRecycleBin(bool AValue);
  void SetRecycleBinPath(const UnicodeString & AValue);
  void SetPostLoginCommands(const UnicodeString & AValue);
  void SetAddressFamily(TAddressFamily AValue);
  void SetRekeyData(const UnicodeString & AValue);
  void SetRekeyTime(uint32_t AValue);
  void SetColor(int32_t AValue);
  void SetTunnel(bool AValue);
  void SetTunnelHostName(const UnicodeString & AValue);
  void SetTunnelPortNumber(int32_t AValue);
  void SetTunnelUserName(const UnicodeString & AValue);
  void SetTunnelPassword(const UnicodeString & AValue);
  UnicodeString GetTunnelPassword() const;
  void SetTunnelPublicKeyFile(const UnicodeString & AValue);
  void SetTunnelPassphrase(const UnicodeString & AValue);
  UnicodeString GetTunnelPassphrase() const;
  void SetTunnelPortFwd(const UnicodeString & AValue);
  void SetTunnelLocalPortNumber(int32_t AValue);
  bool GetTunnelAutoassignLocalPortNumber() const;
  void SetTunnelHostKey(const UnicodeString & AValue);
  void SetFtpPasvMode(bool AValue);
  void SetFtpForcePasvIp(TAutoSwitch AValue);
  void SetFtpUseMlsd(TAutoSwitch AValue);
  void SetFtpAccount(const UnicodeString & AValue);
  void SetFtpPingInterval(int32_t AValue);
  void SetFtpPingType(TFtpPingType AValue);
  void SetFtpTransferActiveImmediately(TAutoSwitch AValue);
  void SetFtps(TFtps AValue);
  void SetMinTlsVersion(TTlsVersion AValue);
  void SetMaxTlsVersion(TTlsVersion AValue);
  void SetCompleteTlsShutdown(TAutoSwitch AValue);
  void SetNotUtf(TAutoSwitch AValue);
  void SetInternalEditorEncoding(int32_t AValue);
  void SetS3DefaultRegion(const UnicodeString & AValue);
  void SetS3SessionToken(const UnicodeString & AValue);
  void SetS3Profile(const UnicodeString & AValue);
  void SetS3UrlStyle(TS3UrlStyle AValue);
  void SetS3MaxKeys(TAutoSwitch AValue);
  void SetS3CredentialsEnv(bool AValue);
  void SetS3RequesterPays(bool AValue);
  void SetLogicalHostName(const UnicodeString & AValue);
  void SetIsWorkspace(bool AValue);
  void SetLink(const UnicodeString & AValue);
  void SetNameOverride(const UnicodeString & AValue);
  void SetHostKey(const UnicodeString & AValue);
  void SetNote(const UnicodeString & AValue);
  void SetWinTitle(const UnicodeString & AValue);
  UnicodeString GetEncryptKey() const;
  void SetEncryptKey(const UnicodeString & AValue);
  void SetWebDavLiberalEscaping(bool AValue);
  void SetWebDavAuthLegacy(bool AValue);

  TDateTime GetTimeoutDT() const;
  void SavePasswords(THierarchicalStorage * Storage, bool PuttyExport, bool DoNotEncryptPasswords, bool SaveAll);
  UnicodeString GetLocalName() const;
  UnicodeString GetFolderName() const;
  void Modify();
  UnicodeString GetSourceName() const;
  void DoLoad(THierarchicalStorage * Storage, bool PuttyImport, bool & RewritePassword, bool Unsafe, bool RespectDisablePasswordStoring);
  void DoSave(THierarchicalStorage * Storage,
    bool PuttyExport, const TSessionData * Default, bool DoNotEncryptPasswords);
#if defined(__BORLANDC__)
  UnicodeString ReadXmlNode(_di_IXMLNode Node, const UnicodeString & Name, const UnicodeString & Default);
  int32_t ReadXmlNode(_di_IXMLNode Node, const UnicodeString & Name, int32_t Default);
  _di_IXMLNode FindSettingsNode(_di_IXMLNode Node, const UnicodeString & Name);
  UnicodeString ReadSettingsNode(_di_IXMLNode Node, const UnicodeString & Name, const UnicodeString & Default);
  int32_t ReadSettingsNode(_di_IXMLNode Node, const UnicodeString & Name, int32_t Default);
#endif // defined(__BORLANDC__)
  bool IsSame(const TSessionData * Default, bool AdvancedOnly, TStrings * DifferentProperties, bool Decrypted) const;
  UnicodeString GetNameWithoutHiddenPrefix() const;
  bool HasStateData() const;
  void CopyStateData(TSessionData * SourceData);
  void CopyNonCoreData(TSessionData * SourceData);
  UnicodeString GetNormalizedPuttyProtocol() const;
  void ReadPasswordsFromFiles();
  static RawByteString EncryptPassword(const UnicodeString & Password, const UnicodeString & Key);
  static UnicodeString DecryptPassword(const RawByteString & Password, const UnicodeString & Key);
  static RawByteString StronglyRecryptPassword(const RawByteString & Password, const UnicodeString & Key);
  static bool DoIsProtocolUrl(const UnicodeString & AUrl, const UnicodeString & AProtocol, int32_t & ProtocolLen);
  static bool IsProtocolUrl(const UnicodeString & AUrl, const UnicodeString & Protocol, int32_t & ProtocolLen);
  static void AddSwitch(UnicodeString & Result, const UnicodeString & Name, bool Rtf);
  static void AddSwitch(
    UnicodeString & Result, const UnicodeString & Name, const UnicodeString & Value, bool Rtf);
  static void AddSwitch(UnicodeString & Result, const UnicodeString & AName, int32_t Value, bool Rtf);
#if defined(__BORLANDC__)
  static void AddAssemblyProperty(
    UnicodeString & Result, TAssemblyLanguage Language,
    const UnicodeString & Name, const UnicodeString & Value);
  static void AddAssemblyProperty(
    UnicodeString & Result, TAssemblyLanguage Language,
    const UnicodeString & Name, const UnicodeString & Type,
    const UnicodeString & Member);
  static void AddAssemblyProperty(
    UnicodeString & Result, TAssemblyLanguage Language,
    const UnicodeString & Name, int32_t Value);
  void AddAssemblyProperty(
    UnicodeString & Result, TAssemblyLanguage Language,
    const UnicodeString & Name, bool Value);
#endif // defined(__BORLANDC__)
  TStrings * GetRawSettingsForUrl();
  void DoCopyData(const TSessionData * SourceData, bool NoRecrypt);
  bool HasS3AutoCredentials() const;
  template<class AlgoT>
  void SetAlgoList(AlgoT * List, const AlgoT * DefaultList, const wchar_t * const * Names,
    int32_t Count, AlgoT WarnAlgo, const UnicodeString & AValue);
  static void Remove(THierarchicalStorage * Storage, const UnicodeString & Name);

  __property UnicodeString InternalStorageKey = { read = GetInternalStorageKey };

public:
  TSessionData() = delete;
  explicit TSessionData(const UnicodeString & AName) noexcept;
  virtual ~TSessionData() noexcept override;
  TSessionData * Clone() const;
  void Default();
  void DefaultSettings();
  void NonPersistent();
  void Load(THierarchicalStorage * Storage, bool PuttyImport);
  void ApplyRawSettings(TStrings * RawSettings, bool Unsafe);
  void ApplyRawSettings(THierarchicalStorage * Storage, bool Unsafe, bool RespectDisablePasswordStoring);
#if defined(__BORLANDC__)
  void ImportFromFilezilla(_di_IXMLNode Node, const UnicodeString & Path, _di_IXMLNode SettingsNode);
#endif // defined(__BORLANDC__)
  void ImportFromOpenssh(TStrings * Lines);
  void Save(THierarchicalStorage * Storage, bool PuttyExport,
    const TSessionData * Default = nullptr);
  void SaveRecryptedPasswords(THierarchicalStorage * Storage);
  void RecryptPasswords();
  bool HasPassword() const;
  bool HasAnySessionPassword() const;
  bool HasAnyPassword() const;
  void ClearSessionPasswords();
  void MaskPasswords();
  void Remove();
  void CacheHostKeyIfNotCached();
  virtual void Assign(const TPersistent * Source) override;
  virtual int32_t Compare(const TNamedObject * Other) const override;
  void CopyData(const TSessionData * Source);
  void CopyDataNoRecrypt(const TSessionData * SourceData);
  void CopyDirectoriesStateData(const TSessionData * SourceData);
  bool ParseUrl(const UnicodeString & Url, TOptions * Options,
    TStoredSessionList * AStoredSessions, bool & DefaultsOnly,
    UnicodeString * AFileName, bool * AProtocolDefined, UnicodeString * MaskedUrl, int32_t Flags);
  TStrings * SaveToOptions(const TSessionData * Default, bool SaveName, bool PuttyExport);
  void ConfigureTunnel(int32_t PortNumber);
  void RollbackTunnel();
  TSessionData * CreateTunnelData(int32_t TunnelLocalPortNumber);
  void ExpandEnvironmentVariables();
  void DisableAuthenticationsExceptPassword();
  bool IsSame(const TSessionData * Default, bool AdvancedOnly) const;
  bool IsSameDecrypted(const TSessionData * Default) const;
  bool IsSameSite(const TSessionData * Default) const;
  bool IsInFolderOrWorkspace(const UnicodeString & Name) const;
  UnicodeString GenerateSessionUrl(uint32_t Flags) const;
  bool HasRawSettingsForUrl();
  bool HasSessionName() const;
  bool HasAutoCredentials() const;
  int32_t GetDefaultPort() const;
  UnicodeString ResolvePublicKeyFile();
  UnicodeString GetSessionPasswordEncryptionKey() const;

  UnicodeString GenerateOpenCommandArgs(bool Rtf) const;
#if defined(__BORLANDC__)
  void GenerateAssemblyCode(TAssemblyLanguage Language, UnicodeString & Head, UnicodeString & Tail, int & Indent);
#endif // defined(__BORLANDC__)
  void LookupLastFingerprint();
  bool GetIsSecure() const;
  static void ValidatePath(const UnicodeString & Path);
  static void ValidateName(const UnicodeString & AName);
  static UnicodeString MakeValidName(const UnicodeString & AName);
  static UnicodeString ExtractLocalName(const UnicodeString & Name);
  static UnicodeString ExtractFolderName(const UnicodeString & Name);
  static UnicodeString ComposePath(const UnicodeString & Path, const UnicodeString & Name);
  static bool IsSensitiveOption(const UnicodeString & Option, const UnicodeString & Value);
  static bool IsOptionWithParameters(const UnicodeString & Option);
  static bool MaskPasswordInOptionParameter(const UnicodeString & Option, UnicodeString & Param);
  static UnicodeString FormatSiteKey(const UnicodeString & HostName, int32_t PortNumber);
  static TStrings * GetAllOptionNames(bool PuttyExport);

  __property UnicodeString HostName  = { read=FHostName, write=SetHostName };
  RWProperty<UnicodeString> HostName{nb::bind(&TSessionData::GetHostName, this), nb::bind(&TSessionData::SetHostName, this)};
  __property UnicodeString HostNameExpanded  = { read=GetHostNameExpanded };
  const ROProperty<UnicodeString> HostNameExpanded{nb::bind(&TSessionData::GetHostNameExpanded, this)};
  __property UnicodeString HostNameSource = { read=GetHostNameSource };
  const ROProperty<UnicodeString> HostNameSource{nb::bind(&TSessionData::GetHostNameSource, this)};
  __property int32_t PortNumber  = { read=FPortNumber, write=SetPortNumber };
  RWProperty<int32_t> PortNumber{nb::bind(&TSessionData::GetPortNumber, this), nb::bind(&TSessionData::SetPortNumber, this)};
  __property UnicodeString UserName  = { read=FUserName, write=SetUserName };
  RWProperty<UnicodeString> UserName{nb::bind(&TSessionData::GetUserName, this), nb::bind(&TSessionData::SetUserName, this)};
  __property UnicodeString UserNameExpanded  = { read=GetUserNameExpanded };
  __property UnicodeString UserNameSource  = { read=GetUserNameSource };
  __property UnicodeString Password  = { read=GetPassword, write=SetPassword };
  RWProperty<UnicodeString> Password{nb::bind(&TSessionData::GetPassword, this), nb::bind(&TSessionData::SetPassword, this)};
  __property UnicodeString NewPassword  = { read=GetNewPassword, write=SetNewPassword };
  RWPropertySimple<UnicodeString> NewPassword{&FNewPassword, nb::bind(&TSessionData::SetNewPassword, this)};
  __property bool ChangePassword  = { read=FChangePassword, write=SetChangePassword };
  RWPropertySimple<bool> ChangePassword{&FChangePassword, nb::bind(&TSessionData::SetChangePassword, this)};
  __property int32_t PingInterval  = { read=FPingInterval, write=SetPingInterval };
  RWPropertySimple<int32_t> PingInterval{&FPingInterval, nb::bind(&TSessionData::SetPingInterval, this)};
  __property bool TryAgent  = { read=FTryAgent, write=SetTryAgent };
  RWProperty<bool> TryAgent{nb::bind(&TSessionData::GetTryAgent, this), nb::bind(&TSessionData::SetTryAgent, this)};
  __property bool AgentFwd  = { read=FAgentFwd, write=SetAgentFwd };
  RWProperty<bool> AgentFwd{nb::bind(&TSessionData::GetAgentFwd, this), nb::bind(&TSessionData::SetAgentFwd, this)};
  __property UnicodeString ListingCommand = { read = FListingCommand, write = SetListingCommand };
  __property bool AuthKI  = { read = FAuthKI, write = SetAuthKI };
  RWPropertySimple<bool> AuthKI{&FAuthKI, nb::bind(&TSessionData::SetAuthKI, this)};
  __property bool AuthKIPassword  = { read=FAuthKIPassword, write=SetAuthKIPassword };
  __property bool AuthGSSAPI  = { read=FAuthGSSAPI, write=SetAuthGSSAPI };
  RWPropertySimple<bool> AuthGSSAPI{&FAuthGSSAPI, nb::bind(&TSessionData::SetAuthGSSAPI, this)};
  __property bool AuthGSSAPIKEX  = { read=FAuthGSSAPIKEX, write=SetAuthGSSAPIKEX };
  RWPropertySimple<bool> AuthGSSAPIKEX{&FAuthGSSAPIKEX, nb::bind(&TSessionData::SetAuthGSSAPIKEX, this)};
  __property bool GSSAPIFwdTGT = { read=FGSSAPIFwdTGT, write=SetGSSAPIFwdTGT };
  __property bool ChangeUsername  = { read=FChangeUsername, write=SetChangeUsername };
  RWPropertySimple<bool> ChangeUsername{&FChangeUsername, nb::bind(&TSessionData::SetChangeUsername, this)};
  __property bool Compression  = { read=FCompression, write=SetCompression };
  RWProperty<bool> Compression{nb::bind(&TSessionData::GetCompression, this), nb::bind(&TSessionData::SetCompression, this)};
  __property bool UsesSsh = { read = GetUsesSsh };
  __property bool Ssh2DES  = { read = FSsh2DES, write = SetSsh2DES };
  RWPropertySimple<bool> Ssh2DES{&FSsh2DES, nb::bind(&TSessionData::SetSsh2DES, this)};
  __property bool SshNoUserAuth  = { read=FSshNoUserAuth, write=SetSshNoUserAuth };
  RWPropertySimple<bool> SshNoUserAuth{&FSshNoUserAuth, nb::bind(&TSessionData::SetSshNoUserAuth, this)};
#if defined(__BORLANDC__)
  __property TCipher Cipher[int32_t Index] = { read=GetCipher, write=SetCipher };
  __property TKex Kex[int32_t Index] = { read=GetKex, write=SetKex };
  __property THostKey HostKeys[int32_t Index] = { read=GetHostKeys, write=SetHostKeys };
  __property TGssLib GssLib[int32_t Index] = { read=GetGssLib, write=SetGssLib };
#endif // defined(__BORLANDC__)
  __property UnicodeString GssLibCustom = { read=FGssLibCustom, write=SetGssLibCustom };
  RWPropertySimple<UnicodeString> GssLibCustom{&FGssLibCustom, nb::bind(&TSessionData::SetGssLibCustom, this)};
  __property UnicodeString PublicKeyFile  = { read=FPublicKeyFile, write=SetPublicKeyFile };
  RWProperty<UnicodeString> PublicKeyFile{nb::bind(&TSessionData::GetPublicKeyFile, this), nb::bind(&TSessionData::SetPublicKeyFile, this)};
  __property UnicodeString Passphrase  = { read=GetPassphrase, write=SetPassphrase };
  RWProperty<UnicodeString> Passphrase{nb::bind(&TSessionData::GetPassphrase, this), nb::bind(&TSessionData::SetPassphrase, this)};
  __property UnicodeString DetachedCertificate  = { read=FDetachedCertificate, write=SetDetachedCertificate };
  RWPropertySimple<UnicodeString> DetachedCertificate{&FDetachedCertificate, nb::bind(&TSessionData::SetDetachedCertificate, this)};
  __property UnicodeString PuttyProtocol  = { read=FPuttyProtocol, write=SetPuttyProtocol };
  RWProperty<UnicodeString> PuttyProtocol{nb::bind(&TSessionData::GetPuttyProtocol, this), nb::bind(&TSessionData::SetPuttyProtocol, this)};
  __property TFSProtocol FSProtocol  = { read=FFSProtocol, write=SetFSProtocol  };
  RWProperty<TFSProtocol> FSProtocol{nb::bind(&TSessionData::GetFSProtocol, this), nb::bind(&TSessionData::SetFSProtocol, this)};
  __property UnicodeString FSProtocolStr  = { read=GetFSProtocolStr };
  const ROProperty<UnicodeString> FSProtocolStr{nb::bind(&TSessionData::GetFSProtocolStr, this)};
  __property bool Modified  = { read=FModified, write=FModified };
  __property bool CanLogin  = { read=GetCanLogin };
  __property bool CanOpen = { read=GetCanOpen };
  const ROProperty<bool> CanOpen{nb::bind(&TSessionData::GetCanOpen, this)};
  __property bool IsLocalBrowser = { read=GetIsLocalBrowser };
  const ROProperty<bool> IsLocalBrowser{nb::bind(&TSessionData::GetIsLocalBrowser, this)};
  __property bool ClearAliases = { read = FClearAliases, write = SetClearAliases };
  RWPropertySimple<bool> ClearAliases{&FClearAliases, nb::bind(&TSessionData::SetClearAliases, this)};
  __property TDateTime PingIntervalDT = { read = GetPingIntervalDT, write = SetPingIntervalDT };
  __property TDateTime TimeDifference = { read = FTimeDifference, write = SetTimeDifference };
  __property bool TimeDifferenceAuto = { read = FTimeDifferenceAuto, write = SetTimeDifferenceAuto };
  __property TPingType PingType = { read = FPingType, write = SetPingType };
  RWPropertySimple<TPingType> PingType{&FPingType, nb::bind(&TSessionData::SetPingType, this)};
  __property UnicodeString SessionName  = { read=GetSessionName };
  const ROProperty<UnicodeString> SessionName{nb::bind(&TSessionData::GetSessionName, this)};
  __property UnicodeString DefaultSessionName  = { read=GetDefaultSessionName };
  const ROProperty<UnicodeString> DefaultSessionName{nb::bind(&TSessionData::GetDefaultSessionName, this)};
  __property UnicodeString LocalDirectory  = { read=FLocalDirectory, write=SetLocalDirectory };
  RWPropertySimple<UnicodeString> LocalDirectory{&FLocalDirectory, nb::bind(&TSessionData::SetLocalDirectory, this) };
  __property UnicodeString LocalDirectoryExpanded = { read = GetLocalDirectoryExpanded };
  __property UnicodeString OtherLocalDirectory = { read=FOtherLocalDirectory, write=SetOtherLocalDirectory };
  RWPropertySimple<UnicodeString> OtherLocalDirectory{&FOtherLocalDirectory, nb::bind(&TSessionData::SetOtherLocalDirectory, this) };
  __property UnicodeString RemoteDirectory  = { read=FRemoteDirectory, write=SetRemoteDirectory };
  RWProperty<UnicodeString> RemoteDirectory{nb::bind(&TSessionData::GetRemoteDirectory, this), nb::bind(&TSessionData::SetRemoteDirectory, this)};
  __property bool SynchronizeBrowsing = { read=FSynchronizeBrowsing, write=SetSynchronizeBrowsing };
  __property bool UpdateDirectories = { read=FUpdateDirectories, write=SetUpdateDirectories };
  RWProperty<bool> UpdateDirectories{nb::bind(&TSessionData::GetUpdateDirectories, this), nb::bind(&TSessionData::SetUpdateDirectories, this)};
  __property bool RequireDirectories = { read=FRequireDirectories, write=FRequireDirectories };
  RWProperty2<bool> RequireDirectories{&FRequireDirectories};
  __property bool CacheDirectories = { read=FCacheDirectories, write=SetCacheDirectories };
  RWPropertySimple<bool> CacheDirectories{&FCacheDirectories, nb::bind(&TSessionData::SetCacheDirectories, this)};
  __property bool CacheDirectoryChanges = { read=FCacheDirectoryChanges, write=SetCacheDirectoryChanges };
  RWPropertySimple<bool> CacheDirectoryChanges{&FCacheDirectoryChanges, nb::bind(&TSessionData::SetCacheDirectoryChanges, this)};
  __property bool PreserveDirectoryChanges = { read=FPreserveDirectoryChanges, write=SetPreserveDirectoryChanges };
  RWPropertySimple<bool> PreserveDirectoryChanges{&FPreserveDirectoryChanges, nb::bind(&TSessionData::SetPreserveDirectoryChanges, this)};
  __property bool Special = { read=FSpecial, write=SetSpecial };
  RWPropertySimple<bool> Special{&FSpecial, nb::bind(&TSessionData::SetSpecial, this)};
  __property bool Selected  = { read=FSelected, write=FSelected };
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
  __property int32_t Timeout = { read = FTimeout, write = SetTimeout };
  RWPropertySimple<int32_t> Timeout{&FTimeout, nb::bind(&TSessionData::SetTimeout, this)};
  __property TDateTime TimeoutDT = { read = GetTimeoutDT };
  __property bool UnsetNationalVars = { read = FUnsetNationalVars, write = SetUnsetNationalVars };
  __property bool IgnoreLsWarnings  = { read = FIgnoreLsWarnings, write = SetIgnoreLsWarnings };
  __property bool TcpNoDelay  = { read = FTcpNoDelay, write = SetTcpNoDelay };
  __property int32_t SendBuf  = { read = FSendBuf, write = SetSendBuf };
  __property UnicodeString SourceAddress = { read=FSourceAddress, write=SetSourceAddress };
  RWPropertySimple<UnicodeString> SourceAddress{&FSourceAddress, nb::bind(&TSessionData::SetSourceAddress, this)};
  __property UnicodeString ProtocolFeatures = { read=FProtocolFeatures, write=SetProtocolFeatures };
  RWPropertySimple<UnicodeString> ProtocolFeatures{&FProtocolFeatures, nb::bind(&TSessionData::SetProtocolFeatures, this)};
  __property bool SshSimple  = { read=FSshSimple, write=SetSshSimple };
  __property UnicodeString CipherList  = { read=GetCipherList, write=SetCipherList };
  RWProperty<UnicodeString> CipherList{nb::bind(&TSessionData::GetCipherList, this), nb::bind(&TSessionData::SetCipherList, this)};
  __property UnicodeString KexList  = { read=GetKexList, write=SetKexList };
  RWProperty<UnicodeString> KexList{nb::bind(&TSessionData::GetKexList, this), nb::bind(&TSessionData::SetKexList, this)};
  __property UnicodeString HostKeyList  = { read=GetHostKeyList, write=SetHostKeyList };
  RWProperty<UnicodeString> HostKeyList{nb::bind(&TSessionData::GetHostKeyList, this), nb::bind(&TSessionData::SetHostKeyList, this)};
  __property UnicodeString GssLibList  = { read=GetGssLibList, write=SetGssLibList };
  RWProperty<UnicodeString> GssLibList{nb::bind(&TSessionData::GetGssLibList, this), nb::bind(&TSessionData::SetGssLibList, this)};
  __property TProxyMethod ProxyMethod  = { read=FProxyMethod, write=SetProxyMethod };
  __property UnicodeString ProxyHost  = { read=FProxyHost, write=SetProxyHost };
  __property int32_t ProxyPort  = { read=FProxyPort, write=SetProxyPort };
  __property UnicodeString ProxyUsername  = { read=FProxyUsername, write=SetProxyUsername };
  __property UnicodeString ProxyPassword  = { read=GetProxyPassword, write=SetProxyPassword };
  RWProperty<UnicodeString> ProxyPassword{nb::bind(&TSessionData::GetProxyPassword, this), nb::bind(&TSessionData::SetProxyPassword, this)};
  __property UnicodeString ProxyTelnetCommand  = { read=FProxyTelnetCommand, write=SetProxyTelnetCommand };
  __property UnicodeString ProxyLocalCommand  = { read=FProxyLocalCommand, write=SetProxyLocalCommand };
  __property TAutoSwitch ProxyDNS  = { read=FProxyDNS, write=SetProxyDNS };
  __property bool ProxyLocalhost  = { read=FProxyLocalhost, write=SetProxyLocalhost };
  __property int32_t FtpProxyLogonType  = { read=FFtpProxyLogonType, write=SetFtpProxyLogonType };
#if defined(__BORLANDC__)
  __property TAutoSwitch Bug[TSshBug Bug]  = { read=GetBug, write=SetBug };
#endif // defined(__BORLANDC__)
  __property UnicodeString PuttySettings = { read = FPuttySettings, write = SetPuttySettings };
  __property UnicodeString CustomParam1 = { read = FCustomParam1, write = SetCustomParam1 };
  __property UnicodeString CustomParam2 = { read = FCustomParam2, write = SetCustomParam2 };
  __property UnicodeString SessionKey = { read = GetSessionKey };
  __property bool ResolveSymlinks = { read = FResolveSymlinks, write = SetResolveSymlinks };
  RWPropertySimple<bool> ResolveSymlinks{&FResolveSymlinks, nb::bind(&TSessionData::SetResolveSymlinks, this)};
  __property bool FollowDirectorySymlinks = { read = FFollowDirectorySymlinks, write = SetFollowDirectorySymlinks };
  RWPropertySimple<bool> FollowDirectorySymlinks{&FFollowDirectorySymlinks, nb::bind(&TSessionData::SetFollowDirectorySymlinks, this)};
  __property int32_t SFTPDownloadQueue = { read = FSFTPDownloadQueue, write = SetSFTPDownloadQueue };
  __property int32_t SFTPUploadQueue = { read = FSFTPUploadQueue, write = SetSFTPUploadQueue };
  __property int32_t SFTPListingQueue = { read = FSFTPListingQueue, write = SetSFTPListingQueue };
  __property int32_t SFTPMaxVersion = { read = FSFTPMaxVersion, write = SetSFTPMaxVersion };
  __property uint32_t SFTPMaxPacketSize = { read = FSFTPMaxPacketSize, write = SetSFTPMaxPacketSize };
  __property TAutoSwitch SFTPRealPath = { read = FSFTPRealPath, write = SetSFTPRealPath };
  __property bool UsePosixRename = { read = FUsePosixRename, write = SetUsePosixRename };
  RWPropertySimple<bool> UsePosixRename{&FUsePosixRename, nb::bind(&TSessionData::SetUsePosixRename, this)};
  // __property TAutoSwitch SFTPBug[TSftpBug Bug]  = { read=GetSFTPBug, write=SetSFTPBug };
  __property TAutoSwitch SCPLsFullTime = { read = FSCPLsFullTime, write = SetSCPLsFullTime };
  __property TAutoSwitch FtpListAll = { read = FFtpListAll, write = SetFtpListAll };
  __property TAutoSwitch FtpHost = { read = FFtpHost, write = SetFtpHost };
  __property TAutoSwitch FtpWorkFromCwd = { read = FFtpWorkFromCwd, write = SetFtpWorkFromCwd };
  __property bool FtpAnyCodeForPwd = { read = FFtpAnyCodeForPwd, write = SetFtpAnyCodeForPwd };
  __property bool SslSessionReuse = { read = FSslSessionReuse, write = SetSslSessionReuse };
  __property UnicodeString TlsCertificateFile = { read=FTlsCertificateFile, write=SetTlsCertificateFile };
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
  __property int32_t Color = { read = FColor, write = SetColor };
  __property bool Tunnel = { read = FTunnel, write = SetTunnel };
  RWPropertySimple<bool> Tunnel{&FTunnel, nb::bind(&TSessionData::SetTunnel, this)};
  __property UnicodeString TunnelHostName = { read = FTunnelHostName, write = SetTunnelHostName };
  RWPropertySimple<UnicodeString> TunnelHostName{&FTunnelHostName, nb::bind(&TSessionData::SetTunnelHostName, this)};
  __property int32_t TunnelPortNumber = { read = FTunnelPortNumber, write = SetTunnelPortNumber };
  RWPropertySimple<int32_t> TunnelPortNumber{&FTunnelPortNumber, nb::bind(&TSessionData::SetTunnelPortNumber, this)};
  __property UnicodeString TunnelUserName = { read = FTunnelUserName, write = SetTunnelUserName };
  RWPropertySimple<UnicodeString> TunnelUserName{&FTunnelUserName, nb::bind(&TSessionData::SetTunnelUserName, this)};
  __property UnicodeString TunnelPassword = { read = GetTunnelPassword, write = SetTunnelPassword };
  RWProperty<UnicodeString> TunnelPassword{nb::bind(&TSessionData::GetTunnelPassword, this), nb::bind(&TSessionData::SetTunnelPassword, this)};
  __property UnicodeString TunnelPublicKeyFile = { read = FTunnelPublicKeyFile, write = SetTunnelPublicKeyFile };
  RWPropertySimple<UnicodeString> TunnelPublicKeyFile{&FTunnelPublicKeyFile, nb::bind(&TSessionData::SetTunnelPublicKeyFile, this)};
  __property UnicodeString TunnelPassphrase = { read = GetTunnelPassphrase, write = SetTunnelPassphrase };
  RWProperty<UnicodeString> TunnelPassphrase{nb::bind(&TSessionData::GetTunnelPassphrase, this), nb::bind(&TSessionData::SetTunnelPassphrase, this)};
  __property bool TunnelAutoassignLocalPortNumber = { read = GetTunnelAutoassignLocalPortNumber };
  __property int32_t TunnelLocalPortNumber = { read = FTunnelLocalPortNumber, write = SetTunnelLocalPortNumber };
  __property UnicodeString TunnelPortFwd = { read = FTunnelPortFwd, write = SetTunnelPortFwd };
  __property UnicodeString TunnelHostKey = { read = FTunnelHostKey, write = SetTunnelHostKey };
  __property bool FtpPasvMode = { read = FFtpPasvMode, write = SetFtpPasvMode };
  __property TAutoSwitch FtpForcePasvIp = { read = FFtpForcePasvIp, write = SetFtpForcePasvIp };
  __property TAutoSwitch FtpUseMlsd = { read = FFtpUseMlsd, write = SetFtpUseMlsd };
  __property UnicodeString FtpAccount = { read = FFtpAccount, write = SetFtpAccount };
  __property int32_t FtpPingInterval  = { read=FFtpPingInterval, write=SetFtpPingInterval };
  __property TDateTime FtpPingIntervalDT  = { read=GetFtpPingIntervalDT };
  __property TFtpPingType FtpPingType = { read = FFtpPingType, write = SetFtpPingType };
  __property TAutoSwitch FtpTransferActiveImmediately = { read = FFtpTransferActiveImmediately, write = SetFtpTransferActiveImmediately };
  __property TFtps Ftps = { read = FFtps, write = SetFtps };
  RWProperty<TFtps> Ftps{nb::bind(&TSessionData::GetFtps, this), nb::bind(&TSessionData::SetFtps, this)};
  __property TTlsVersion MinTlsVersion = { read = FMinTlsVersion, write = SetMinTlsVersion };
  RWPropertySimple<TTlsVersion> MinTlsVersion{&FMinTlsVersion, nb::bind(&TSessionData::SetMinTlsVersion, this)};
  __property TTlsVersion MaxTlsVersion = { read = FMaxTlsVersion, write = SetMaxTlsVersion };
  RWPropertySimple<TTlsVersion> MaxTlsVersion{&FMaxTlsVersion, nb::bind(&TSessionData::SetMaxTlsVersion, this)};
  __property TAutoSwitch CompleteTlsShutdown = { read = FCompleteTlsShutdown, write = SetCompleteTlsShutdown };
  RWPropertySimple<TAutoSwitch> CompleteTlsShutdown{&FCompleteTlsShutdown, nb::bind(&TSessionData::SetCompleteTlsShutdown, this)};
  __property UnicodeString LogicalHostName = { read = FLogicalHostName, write = SetLogicalHostName };
  RWProperty<UnicodeString> LogicalHostName{nb::bind(&TSessionData::GetLogicalHostName, this), nb::bind(&TSessionData::SetLogicalHostName, this)};
  __property TAutoSwitch NotUtf = { read = FNotUtf, write = SetNotUtf };
  __property int32_t InternalEditorEncoding = { read = FInternalEditorEncoding, write = SetInternalEditorEncoding };
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
  RWPropertySimple<bool> S3CredentialsEnv{&FS3CredentialsEnv, nb::bind(&TSessionData::SetS3CredentialsEnv, this) };
  __property bool S3RequesterPays = { read = FS3RequesterPays, write = SetS3RequesterPays };
  __property bool IsWorkspace = { read = FIsWorkspace, write = SetIsWorkspace };
  __property UnicodeString Link = { read = FLink, write = SetLink };
  __property UnicodeString NameOverride = { read = FNameOverride, write = SetNameOverride };
  RWProperty<UnicodeString> NameOverride{nb::bind(&TSessionData::GetNameOverride, this), nb::bind(&TSessionData::SetNameOverride, this)};
  __property UnicodeString HostKey = { read = FHostKey, write = SetHostKey };
  RWProperty<UnicodeString> HostKey{nb::bind(&TSessionData::GetHostKey, this), nb::bind(&TSessionData::SetHostKey, this)};
  __property bool FingerprintScan = { read = FFingerprintScan, write = FFingerprintScan };
  RWProperty2<bool> FingerprintScan{&FFingerprintScan};
  __property bool OverrideCachedHostKey = { read = FOverrideCachedHostKey };
  const ROProperty2<bool> OverrideCachedHostKey{&FOverrideCachedHostKey};
  __property UnicodeString Note = { read = FNote, write = SetNote };
  __property UnicodeString WinTitle = { read = FWinTitle, write = SetWinTitle };
  __property UnicodeString EncryptKey = { read = GetEncryptKey, write = SetEncryptKey };
  RWProperty<UnicodeString> EncryptKey{nb::bind(&TSessionData::GetEncryptKey, this), nb::bind(&TSessionData::SetEncryptKey, this)};
  __property bool WebDavLiberalEscaping = { read = FWebDavLiberalEscaping, write = SetWebDavLiberalEscaping };
  RWPropertySimple<bool> WebDavLiberalEscaping{&FWebDavLiberalEscaping, nb::bind(&TSessionData::SetWebDavLiberalEscaping, this) };
  __property bool WebDavAuthLegacy = { read = FWebDavAuthLegacy, write = SetWebDavAuthLegacy };
  RWPropertySimple<bool> WebDavAuthLegacy{&FWebDavAuthLegacy, nb::bind(&TSessionData::SetWebDavAuthLegacy, this) };

  __property UnicodeString StorageKey = { read = GetStorageKey };
  const ROProperty<UnicodeString> StorageKey{nb::bind(&TSessionData::GetStorageKey, this)};
  __property UnicodeString SiteKey = { read = GetSiteKey };
  const ROProperty<UnicodeString> SiteKey{nb::bind(&TSessionData::GetSiteKey, this)};
  __property UnicodeString OrigHostName = { read = FOrigHostName };
  const UnicodeString& OrigHostName{FOrigHostName};
  __property int32_t OrigPortNumber = { read = FOrigPortNumber };
  const int32_t& OrigPortNumber{FOrigPortNumber};
  __property UnicodeString LocalName = { read = GetLocalName };
  const ROProperty<UnicodeString> LocalName{nb::bind(&TSessionData::GetLocalName, this)};
  __property UnicodeString FolderName = { read = GetFolderName };
  const ROProperty<UnicodeString> FolderName{nb::bind(&TSessionData::GetFolderName, this)};
  __property TSessionSource Source = { read = FSource };
  const TSessionSource& Source{FSource};
  __property UnicodeString SourceName = { read = GetSourceName };
  const ROProperty<UnicodeString> SourceName{nb::bind(&TSessionData::GetSourceName, this)};
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

  UnicodeString GetHostName() const { return FHostName; }
  int32_t GetPortNumber() const { return FPortNumber; }
  TLoginType GetLoginType() const;
  void SetLoginType(TLoginType Value);
  UnicodeString GetUserName() const { return FUserName; }
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
  uint32_t GetSFTPMaxVersion() const { return FSFTPMaxVersion; }
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
  void SetCodePage(const UnicodeString & Value);
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
  TFtpPingType GetFtpPingType() const { return FFtpPingType; }
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
  void RemoveProtocolPrefix(UnicodeString & HostName) const;

private:
  uint32_t GetDefaultVersion() const { return ::GetCurrentVersionNumber(); }
  TFSProtocol TranslateFSProtocolNumber(int32_t FSProtocol);
  TFSProtocol TranslateFSProtocol(const UnicodeString & ProtocolID) const;
  TFtps TranslateFtpEncryptionNumber(int32_t FtpEncryption) const;

  TProxyMethod GetSystemProxyMethod() const;
  void PrepareProxyData() const;
  void ParseIEProxyConfig() const;
  void FromURI(const UnicodeString & ProxyURI,
    UnicodeString & ProxyUrl, int32_t & ProxyPort, TProxyMethod & ProxyMethod) const;
  void AdjustHostName(UnicodeString & HostName, const UnicodeString & Prefix) const;

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

  mutable gsl::owner<TIEProxyConfig *> FIEProxyConfig{nullptr};
};

class NB_CORE_EXPORT TStoredSessionList final : public TNamedObjectList
{
  NB_DISABLE_COPY(TStoredSessionList)
public:
  TStoredSessionList() noexcept;
  explicit TStoredSessionList(bool aReadOnly) noexcept;
  void Reload();
  void Save(bool All, bool Explicit);
  void Saved();
  void ImportFromFilezilla(const UnicodeString & FileName, const UnicodeString & ConfigurationFileName);
  void ImportFromKnownHosts(TStrings * Lines);
  void ImportFromOpenssh(TStrings * Lines);
  void Export(const UnicodeString & FileName);
  void Load(THierarchicalStorage * Storage, bool AsModified = false,
    bool UseDefaults = false, bool PuttyImport = false);
  void Save(THierarchicalStorage * Storage, bool All = false);
  void SelectAll(bool Select);
  bool Import(TStoredSessionList * From, bool OnlySelected, TList * Imported);
  void RecryptPasswords(TStrings * RecryptPasswordErrors);
  TSessionData * AtSession(int32_t Index)
  { return static_cast<TSessionData *>(AtObject(Index)); }
  void SelectSessionsToImport(TStoredSessionList * Dest, bool SSHOnly);
  void Cleanup();
  void UpdateStaticUsage();
  int32_t IndexOf(TSessionData * Data) const;
  const TSessionData * FindSame(TSessionData * Data);
  TSessionData * NewSession(const UnicodeString & SessionName, TSessionData * Session);
  void NewWorkspace(const UnicodeString & Name, TList * DataList);
#if defined(__BORLANDC__)
  bool IsFolder(const UnicodeString & Name) const;
  bool IsWorkspace(const UnicodeString & Name) const;
#endif // defined(__BORLANDC__)
  bool IsFolderOrWorkspace(const UnicodeString & Name) const;
  TSessionData * ParseUrl(const UnicodeString & AUrl, TOptions * Options, bool & DefaultsOnly,
    UnicodeString * AFileName = nullptr, bool * AProtocolDefined = nullptr, UnicodeString * MaskedUrl = nullptr, int32_t Flags = 0);
  bool IsUrl(const UnicodeString & AUrl);
  bool CanOpen(TSessionData * Data);
  void GetFolderOrWorkspace(const UnicodeString & Name, TList * List);
  TStrings * GetFolderOrWorkspaceList(const UnicodeString & Name);
  TStrings * GetWorkspaces() const;
  bool HasAnyWorkspace() const;
  TSessionData * SaveWorkspaceData(TSessionData * Data, int32_t Index);
  virtual ~TStoredSessionList() noexcept override;
#if defined(__BORLANDC__)
  __property TSessionData * Sessions[int32_t Index]  = { read=AtSession };
#endif // defined(__BORLANDC__)
  __property TSessionData * DefaultSettings  = { read=FDefaultSettings, write=SetDefaultSettings };
  RWProperty<const TSessionData *> DefaultSettings{nb::bind(&TStoredSessionList::GetDefaultSettingsConst, this), nb::bind(&TStoredSessionList::SetDefaultSettings, this)};

  static int32_t ImportHostKeys(
    THierarchicalStorage * SourceStorage, THierarchicalStorage * TargetStorage, TStoredSessionList * Sessions, bool OnlySelected);
  static void ImportHostKeys(
    const UnicodeString & SourceKey, TStoredSessionList * Sessions, bool OnlySelected);
  static void ImportSelectedKnownHosts(TStoredSessionList * Sessions);
  static bool OpenHostKeysSubKey(THierarchicalStorage * Storage, bool CanCreate);
  static void SelectKnownHostsForSelectedSessions(TStoredSessionList * KnownHosts, TStoredSessionList * Sessions);

  const TSessionData * GetSession(int32_t Index) const { return rtti::dyn_cast_or_null<TSessionData>(AtObject(Index)); }
  TSessionData * GetSession(int32_t Index) { return rtti::dyn_cast_or_null<TSessionData>(AtObject(Index)); }
  const TSessionData * GetDefaultSettingsConst() const { return FDefaultSettings.get(); }
  TSessionData * GetDefaultSettings() { return FDefaultSettings.get(); }
  void SetDefaultSettings(const TSessionData * Value);
  const TSessionData * GetSessionByName(const UnicodeString & SessionName) const;

private:
  std::unique_ptr<TSessionData> FDefaultSettings;
  bool FReadOnly{false};
  std::unique_ptr<TStrings> FPendingRemovals;
#if defined(__BORLANDC__)
  void SetDefaultSettings(TSessionData * AValue);
#endif // defined(__BORLANDC__)
  void DoSave(THierarchicalStorage * Storage, bool All,
    bool RecryptPasswordOnly, TStrings * RecryptPasswordErrors);
  void DoSave(bool All, bool Explicit, bool RecryptPasswordOnly,
    TStrings * RecryptPasswordErrors);
  void DoSave(THierarchicalStorage * Storage,
    TSessionData * Data, bool All, bool RecryptPasswordOnly,
    TSessionData * FactoryDefaults);
  TSessionData * ResolveWorkspaceData(TSessionData * Data);
  const TSessionData * GetFirstFolderOrWorkspaceSession(const UnicodeString & Name) const;
  TSessionData * CheckIsInFolderOrWorkspaceAndResolve(
    TSessionData * Data, const UnicodeString & Name);
#if defined(__BORLANDC__)
  void ImportLevelFromFilezilla(_di_IXMLNode Node, const UnicodeString & Path, _di_IXMLNode SettingsNode);
#endif // defined(__BORLANDC__)
  void DoGetFolderOrWorkspace(const UnicodeString & Name, TList * List, bool NoRecrypt);
  static THierarchicalStorage * CreateHostKeysStorageForWriting();
};
//---------------------------------------------------------------------------
UnicodeString GetExpandedLogFileName(const UnicodeString & LogFileName, const TDateTime & Started, TSessionData * SessionData);
bool GetIsSshProtocol(TFSProtocol FSProtocol);
int32_t DefaultPort(TFSProtocol FSProtocol, TFtps Ftps);
bool HasIP6LiteralBrackets(const UnicodeString & HostName);
UnicodeString StripIP6LiteralBrackets(const UnicodeString & HostName);
bool IsIPv6Literal(const UnicodeString & HostName);
UnicodeString EscapeIPv6Literal(const UnicodeString & IP);
TFSProtocol NormalizeFSProtocol(TFSProtocol FSProtocol);
bool ParseOpensshDirective(const UnicodeString & ALine, UnicodeString & Directive, UnicodeString & Value);
UnicodeString CutOpensshToken(UnicodeString & S);
UnicodeString ConvertPathFromOpenssh(const UnicodeString & Path);
UnicodeString GetTlsVersionName(TTlsVersion TlsVersion);
//---------------------------------------------------------------------------
struct NB_CORE_EXPORT TIEProxyConfig final : public TObject
{
  TIEProxyConfig() = default;
  bool AutoDetect{false}; // not used
  UnicodeString AutoConfigUrl; // not used
  UnicodeString Proxy; ///< string in format "http=host:80;https=host:443;ftp=ftpproxy:20;socks=socksproxy:1080"
  UnicodeString ProxyBypass; ///< string in format "*.local, foo.com, google.com"
  UnicodeString ProxyHost;
  int32_t ProxyPort{0};
  TProxyMethod ProxyMethod{pmNone};
};

NB_CORE_EXPORT bool GetCodePageInfo(UINT CodePage, CPINFOEX & CodePageInfoEx);
NB_CORE_EXPORT uint32_t GetCodePageAsNumber(const UnicodeString & CodePage);
NB_CORE_EXPORT UnicodeString GetCodePageAsString(uint32_t CodePage);

//template<int s> struct CheckSizeT;
//CheckSizeT<sizeof(TSessionData)> checkSize;

