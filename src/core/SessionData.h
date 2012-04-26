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
               sbPKSessID2, sbRekey2, sbMaxPkt2, sbIgnore2
             };
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
extern const wchar_t FSProtocolNames[FSPROTOCOL_COUNT][15];
extern const UnicodeString CONST_LOGIN_ANONYMOUS;
extern const int DefaultSendBuf;
extern const UnicodeString AnonymousUserName;
extern const UnicodeString AnonymousPassword;
//---------------------------------------------------------------------------
class TStoredSessionList;
//---------------------------------------------------------------------------
class TSessionData : public TNamedObject
{
    friend class TStoredSessionList;

private:
    UnicodeString FHostName;
    size_t FPortNumber;
    UnicodeString FUserName;
    UnicodeString FPassword;
    bool FPasswordless;
    size_t FPingInterval;
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
    UnicodeString FProxyPassword;
    UnicodeString FProxyTelnetCommand;
    UnicodeString FProxyLocalCommand;
    TAutoSwitch FProxyDNS;
    bool FProxyLocalhost;
    size_t FFtpProxyLogonType;
    TAutoSwitch FBugs[BUG_COUNT];
    UnicodeString FCustomParam1;
    UnicodeString FCustomParam2;
    bool FResolveSymlinks;
    System::TDateTime FTimeDifference;
    size_t FSFTPDownloadQueue;
    size_t FSFTPUploadQueue;
    size_t FSFTPListingQueue;
    size_t FSFTPMaxVersion;
    size_t FSFTPMinPacketSize;
    size_t FSFTPMaxPacketSize;
    TDSTMode FDSTMode;
    TAutoSwitch FSFTPBugs[SFTP_BUG_COUNT];
    bool FDeleteToRecycleBin;
    bool FOverwrittenToRecycleBin;
    UnicodeString FRecycleBinPath;
    UnicodeString FPostLoginCommands;
    TAutoSwitch FSCPLsFullTime;
    TAutoSwitch FFtpListAll;
    TAddressFamily FAddressFamily;
    UnicodeString FCodePage;
    UnicodeString FRekeyData;
    unsigned int FRekeyTime;
    int FColor;
    bool FTunnel;
    UnicodeString FTunnelHostName;
    size_t FTunnelPortNumber;
    UnicodeString FTunnelUserName;
    UnicodeString FTunnelPassword;
    UnicodeString FTunnelPublicKeyFile;
    size_t FTunnelLocalPortNumber;
    UnicodeString FTunnelPortFwd;
    bool FFtpPasvMode;
    bool FFtpForcePasvIp;
    bool FFtpAllowEmptyPassword;
    TFtpEncryptionSwitch FFtpEncryption;
    TLoginType FLoginType;
    UnicodeString FFtpAccount;
    size_t FFtpPingInterval;
    TPingType FFtpPingType;
    TFtps FFtps;
    TAutoSwitch FNotUtf;
    UnicodeString FHostKey;

    UnicodeString FOrigHostName;
    int FOrigPortNumber;
    TProxyMethod FOrigProxyMethod;
    TSessionSource FSource;
    int FNumberOfRetries;
    bool FSslSessionReuse;

    void __fastcall SavePasswords(THierarchicalStorage *Storage, bool PuttyExport);
    void __fastcall Modify();
    void __fastcall DoLoad(THierarchicalStorage * Storage, bool & RewritePassword);
    static UnicodeString __fastcall EncryptPassword(const UnicodeString Password, const UnicodeString Key);
    static UnicodeString __fastcall DecryptPassword(const UnicodeString Password, const UnicodeString Key);
    static UnicodeString __fastcall StronglyRecryptPassword(const UnicodeString Password, const UnicodeString Key);

    UnicodeString __fastcall GetInternalStorageKey();

public:
    explicit TSessionData(const UnicodeString aName);
    void __fastcall Default();
    void __fastcall NonPersistant();
    void __fastcall Load(THierarchicalStorage *Storage);
    void __fastcall Save(THierarchicalStorage *Storage, bool PuttyExport,
              const TSessionData *Default = NULL);
    void __fastcall SaveRecryptedPasswords(THierarchicalStorage *Storage);
    void __fastcall RecryptPasswords();
    bool __fastcall HasAnyPassword();
    void __fastcall Remove();
    virtual void __fastcall Assign(System::TPersistent *Source);
    bool __fastcall ParseUrl(const UnicodeString Url, TOptions *Options,
                  TStoredSessionList *StoredSessions, bool &DefaultsOnly,
                  UnicodeString *FileName, bool *AProtocolDefined);
    bool __fastcall ParseOptions(TOptions *Options);
    void __fastcall ConfigureTunnel(size_t PortNumber);
    void __fastcall RollbackTunnel();
    void __fastcall ExpandEnvironmentVariables();
    static void __fastcall ValidatePath(const UnicodeString Path);
    static void __fastcall ValidateName(const UnicodeString Name);

    UnicodeString GetHostName() const { return FHostName; }
    size_t GetPortNumber() const { return FPortNumber; }
    TLoginType __fastcall GetLoginType() const;
    void __fastcall SetLoginType(TLoginType value);
    UnicodeString GetUserName() const { return FUserName; }
    UnicodeString __fastcall GetUserNameExpanded();
    void __fastcall SetPassword(const UnicodeString value);
    UnicodeString __fastcall GetPassword() const;
    bool GetPasswordless() const { return FPasswordless; }
    size_t GetPingInterval() const { return FPingInterval; }
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
    bool __fastcall GetUsesSsh();
    bool GetSsh2DES() const { return FSsh2DES; }
    bool GetSshNoUserAuth() const { return FSshNoUserAuth; }
    void __fastcall SetCipher(size_t Index, TCipher value);
    TCipher __fastcall GetCipher(size_t Index) const;
    void __fastcall SetKex(size_t Index, TKex value);
    TKex __fastcall GetKex(size_t Index) const;
    const UnicodeString GetPublicKeyFile() const { return FPublicKeyFile; }
    TProtocol GetProtocol() const { return FProtocol; }
    void __fastcall SetProtocolStr(const UnicodeString value);
    UnicodeString __fastcall GetProtocolStr() const;
    TFSProtocol GetFSProtocol() const { return FFSProtocol; }
    UnicodeString __fastcall GetFSProtocolStr();
    bool GetModified() const { return FModified; }
    void __fastcall SetModified(bool value) { FModified = value; }
    bool __fastcall GetCanLogin();
    bool GetClearAliases() const { return FClearAliases; }
    void __fastcall SetPingIntervalDT(System::TDateTime value);
    System::TDateTime __fastcall GetPingIntervalDT() const;
    System::TDateTime __fastcall GetTimeDifference() const { return FTimeDifference; }
    TPingType GetPingType() const { return FPingType; }
    UnicodeString __fastcall GetSessionName();
    bool __fastcall HasSessionName();
    UnicodeString __fastcall GetDefaultSessionName();
    UnicodeString __fastcall GetSessionUrl();
    UnicodeString __fastcall GetLocalDirectory() const { return FLocalDirectory; }
    UnicodeString __fastcall GetRemoteDirectory() const { return FRemoteDirectory; }
    void __fastcall SetRemoteDirectory(const UnicodeString value);
	bool __fastcall GetSynchronizeBrowsing() const { return FSynchronizeBrowsing; }
    void __fastcall SetSynchronizeBrowsing(bool value);
    bool GetUpdateDirectories() const { return FUpdateDirectories; }
    bool GetCacheDirectories() const { return FCacheDirectories; }
    bool GetCacheDirectoryChanges() const { return FCacheDirectoryChanges; }
    bool GetPreserveDirectoryChanges() const { return FPreserveDirectoryChanges; }
    bool GetLockInHome() const { return FLockInHome; }
    bool GetSpecial() const { return FSpecial; }
    bool GetSelected() const { return FSelected; }
    void __fastcall SetSelected(bool value) { FSelected = value; }
    UnicodeString __fastcall GetInfoTip();
    bool __fastcall GetDefaultShell();
    void __fastcall SetDefaultShell(bool value);
    void __fastcall SetDetectReturnVar(bool value);
    bool __fastcall GetDetectReturnVar();
    TEOLType GetEOLType() const { return FEOLType; }
    TAutoSwitch GetLookupUserGroups() const { return FLookupUserGroups; }
    UnicodeString GetReturnVar() const { return FReturnVar; }
    bool GetScp1Compatibility() const { return FScp1Compatibility; }
    UnicodeString GetShell() const { return FShell; }
    UnicodeString GetSftpServer() const { return FSftpServer; }
    int GetTimeout() const { return FTimeout; }
    System::TDateTime __fastcall GetTimeoutDT();
    bool GetUnsetNationalVars() const { return FUnsetNationalVars; }
    bool GetIgnoreLsWarnings() const { return FIgnoreLsWarnings; }
    bool GetTcpNoDelay() const { return FTcpNoDelay; }
    int __fastcall GetSendBuf() const { return FSendBuf; }
    void __fastcall SetSendBuf(int value);
    bool __fastcall GetSshSimple() const { return FSshSimple; }
    void __fastcall SetSshSimple(bool value);
    UnicodeString __fastcall GetSshProtStr();
    void __fastcall SetCipherList(const UnicodeString value);
    UnicodeString __fastcall GetCipherList() const;
    void __fastcall SetKexList(const UnicodeString value);
    UnicodeString __fastcall GetKexList() const;
    TProxyMethod __fastcall GetProxyMethod() const { return FProxyMethod; }
    UnicodeString __fastcall GetProxyHost() const { return FProxyHost; }
    int __fastcall GetProxyPort() const { return FProxyPort; }
    UnicodeString __fastcall GetProxyUsername() const { return FProxyUsername; }
    UnicodeString __fastcall GetProxyPassword() const;
    void __fastcall __fastcall SetProxyPassword(const UnicodeString value);
    UnicodeString __fastcall GetProxyTelnetCommand() const { return FProxyTelnetCommand; }
    UnicodeString __fastcall GetProxyLocalCommand() const { return FProxyLocalCommand; }
    TAutoSwitch __fastcall GetProxyDNS() const { return FProxyDNS; }
    bool __fastcall GetProxyLocalhost() const { return FProxyLocalhost; }
    size_t __fastcall GetFtpProxyLogonType() const { return FFtpProxyLogonType; }
    void __fastcall SetBug(TSshBug Bug, TAutoSwitch value);
    TAutoSwitch __fastcall GetBug(TSshBug Bug) const;
    UnicodeString __fastcall GetCustomParam1() const { return FCustomParam1; }
    UnicodeString __fastcall GetCustomParam2() const { return FCustomParam2; }
    UnicodeString __fastcall GetSessionKey();
    bool __fastcall GetResolveSymlinks() const { return FResolveSymlinks; }
    size_t __fastcall GetSFTPDownloadQueue() const { return FSFTPDownloadQueue; }
    void __fastcall SetSFTPDownloadQueue(size_t value);
    size_t __fastcall GetSFTPUploadQueue() const { return FSFTPUploadQueue; }
    void __fastcall SetSFTPUploadQueue(size_t value);
    size_t __fastcall GetSFTPListingQueue() const { return FSFTPListingQueue; }
    void __fastcall SetSFTPListingQueue(size_t value);
    size_t __fastcall GetSFTPMaxVersion() const { return FSFTPMaxVersion; }
    size_t __fastcall GetSFTPMinPacketSize() const { return FSFTPMinPacketSize; }
    size_t __fastcall GetSFTPMaxPacketSize() const { return FSFTPMaxPacketSize; }
    void __fastcall SetSFTPBug(TSftpBug Bug, TAutoSwitch value);
    TAutoSwitch __fastcall GetSFTPBug(TSftpBug Bug) const;
    TAutoSwitch __fastcall GetSCPLsFullTime() const { return FSCPLsFullTime; }
    TAutoSwitch __fastcall GetFtpListAll() const { return FFtpListAll; }
    TDSTMode __fastcall GetDSTMode() const { return FDSTMode; }
    bool __fastcall GetDeleteToRecycleBin() const { return FDeleteToRecycleBin; }
    bool __fastcall GetOverwrittenToRecycleBin() const { return FOverwrittenToRecycleBin; }
    UnicodeString __fastcall GetRecycleBinPath() const { return FRecycleBinPath; }
    UnicodeString __fastcall GetPostLoginCommands() const { return FPostLoginCommands; }
    TAddressFamily __fastcall GetAddressFamily() const { return FAddressFamily; }
    UnicodeString __fastcall GetCodePage() const { return FCodePage; }
    unsigned int __fastcall GetCodePageAsNumber() const;
    UnicodeString __fastcall GetRekeyData() const { return FRekeyData; }
    unsigned int __fastcall GetRekeyTime() const { return FRekeyTime; }
    int __fastcall GetColor() const { return FColor; }
    bool __fastcall GetTunnel() const { return FTunnel; }
    void __fastcall SetTunnel(bool value);
    UnicodeString __fastcall GetTunnelHostName() const { return FTunnelHostName; }
    size_t __fastcall GetTunnelPortNumber() const { return FTunnelPortNumber; }
    UnicodeString __fastcall GetTunnelUserName() const { return FTunnelUserName; }
    void __fastcall SetTunnelPassword(const UnicodeString value);
    UnicodeString __fastcall GetTunnelPassword();
    UnicodeString __fastcall GetTunnelPublicKeyFile() const { return FTunnelPublicKeyFile; }
    bool __fastcall GetTunnelAutoassignLocalPortNumber();
    size_t __fastcall GetTunnelLocalPortNumber() const { return FTunnelLocalPortNumber; }
    UnicodeString __fastcall GetTunnelPortFwd() const { return FTunnelPortFwd; }
    void __fastcall SetTunnelPortFwd(const UnicodeString value);
    bool __fastcall GetFtpPasvMode() const { return FFtpPasvMode; }
    bool __fastcall GetFtpAllowEmptyPassword() const { return FFtpAllowEmptyPassword; }
    void __fastcall SetFtpAllowEmptyPassword(bool value);
    TFtpEncryptionSwitch __fastcall GetFtpEncryption() const { return FFtpEncryption; }
    void __fastcall SetFtpEncryption(TFtpEncryptionSwitch value);
    bool __fastcall GetFtpForcePasvIp() const { return FFtpForcePasvIp; }
    UnicodeString __fastcall GetFtpAccount() const { return FFtpAccount; }
    size_t __fastcall GetFtpPingInterval() const { return FFtpPingInterval; }
    System::TDateTime __fastcall GetFtpPingIntervalDT();
    TPingType __fastcall GetFtpPingType() const { return FFtpPingType; }
    TFtps __fastcall GetFtps() const { return FFtps; }
    TAutoSwitch __fastcall GetNotUtf() const { return FNotUtf; }
    UnicodeString __fastcall GetHostKey() const { return FHostKey; }
    UnicodeString __fastcall GetStorageKey();
    UnicodeString __fastcall GetOrigHostName() const { return FOrigHostName; }
    int __fastcall GetOrigPortNumber() const { return FOrigPortNumber; }
    UnicodeString __fastcall GetLocalName();
    UnicodeString __fastcall GetSource();

    void __fastcall SetHostName(const UnicodeString value);
    UnicodeString __fastcall GetHostNameExpanded();
    void __fastcall SetPortNumber(size_t value);
    void __fastcall SetUserName(const UnicodeString value);
    void __fastcall SetPasswordless(bool value);
    void __fastcall SetPingInterval(size_t value);
    void __fastcall SetTryAgent(bool value);
    void __fastcall SetAgentFwd(bool value);
    void __fastcall SetAuthTIS(bool value);
    void __fastcall SetAuthKI(bool value);
    void __fastcall SetAuthKIPassword(bool value);
    void __fastcall SetAuthGSSAPI(bool value);
    void __fastcall SetGSSAPIFwdTGT(bool value);
    void __fastcall SetGSSAPIServerRealm(const UnicodeString value);
    void __fastcall SetChangeUsername(bool value);
    void __fastcall SetCompression(bool value);
    void __fastcall SetSshProt(TSshProt value);
    void __fastcall SetSsh2DES(bool value);
    void __fastcall SetSshNoUserAuth(bool value);
    void __fastcall SetPublicKeyFile(const UnicodeString value);

    void __fastcall SetTimeDifference(System::TDateTime value);
    void __fastcall SetPingType(TPingType value);
    void __fastcall SetProtocol(TProtocol value);
    void __fastcall SetFSProtocol(TFSProtocol value);
    void __fastcall SetLocalDirectory(const UnicodeString value);
    void __fastcall SetUpdateDirectories(bool value);
    void __fastcall SetCacheDirectories(bool value);
    void __fastcall SetCacheDirectoryChanges(bool value);
    void __fastcall SetPreserveDirectoryChanges(bool value);
    void __fastcall SetLockInHome(bool value);
    void __fastcall SetSpecial(bool value);
    void __fastcall SetListingCommand(const UnicodeString value);
    void __fastcall SetClearAliases(bool value);
    void __fastcall SetEOLType(TEOLType value);
    void __fastcall SetLookupUserGroups(TAutoSwitch value);
    void __fastcall SetReturnVar(const UnicodeString value);
    void __fastcall SetScp1Compatibility(bool value);
    void __fastcall SetShell(const UnicodeString value);
    void __fastcall SetSftpServer(const UnicodeString value);
    void __fastcall SetTimeout(int value);
    void __fastcall SetUnsetNationalVars(bool value);
    void __fastcall SetIgnoreLsWarnings(bool value);
    void __fastcall SetTcpNoDelay(bool value);
    void __fastcall SetProxyMethod(TProxyMethod value);
    void __fastcall SetProxyHost(const UnicodeString value);
    void __fastcall SetProxyPort(int value);
    void __fastcall SetProxyUsername(const UnicodeString value);
    void __fastcall SetProxyTelnetCommand(const UnicodeString value);
    void __fastcall SetProxyLocalCommand(const UnicodeString value);
    void __fastcall SetProxyDNS(TAutoSwitch value);
    void __fastcall SetProxyLocalhost(bool value);
    void __fastcall SetFtpProxyLogonType(size_t value);
    void __fastcall SetCustomParam1(const UnicodeString value);
    void __fastcall SetCustomParam2(const UnicodeString value);
    void __fastcall SetResolveSymlinks(bool value);
    void __fastcall SetSFTPMaxVersion(size_t value);
    void __fastcall SetSFTPMinPacketSize(size_t value);
    void __fastcall SetSFTPMaxPacketSize(size_t value);
    void __fastcall SetSCPLsFullTime(TAutoSwitch value);
    void __fastcall SetFtpListAll(TAutoSwitch value);
    void __fastcall SetDSTMode(TDSTMode value);
    void __fastcall SetDeleteToRecycleBin(bool value);
    void __fastcall SetOverwrittenToRecycleBin(bool value);
    void __fastcall SetRecycleBinPath(const UnicodeString value);
    void __fastcall SetPostLoginCommands(const UnicodeString value);
    void __fastcall SetAddressFamily(TAddressFamily value);
    void __fastcall SetCodePage(const UnicodeString value);
    void __fastcall SetRekeyData(const UnicodeString value);
    void __fastcall SetRekeyTime(unsigned int value);
    void __fastcall SetColor(int value);
    void __fastcall SetTunnelHostName(const UnicodeString value);
    void __fastcall SetTunnelPortNumber(size_t value);
    void __fastcall SetTunnelUserName(const UnicodeString value);
    void __fastcall SetTunnelPublicKeyFile(const UnicodeString value);
    void __fastcall SetTunnelLocalPortNumber(size_t value);
    void __fastcall SetFtpPasvMode(bool value);
    void __fastcall SetFtpForcePasvIp(bool value);
    void __fastcall SetFtpAccount(const UnicodeString value);
    void __fastcall SetFtpPingInterval(size_t value);
    void __fastcall SetFtpPingType(TPingType value);
    void __fastcall SetFtps(TFtps value);
    void __fastcall SetNotUtf(TAutoSwitch value);
    void __fastcall SetHostKey(const UnicodeString value);

    int __fastcall GetNumberOfRetries() const { return FNumberOfRetries; }
    void __fastcall SetNumberOfRetries(int value) { FNumberOfRetries = value; }
    bool __fastcall GetSslSessionReuse() const { return FSslSessionReuse; }
    void __fastcall SetSslSessionReuse(bool value);
private:
    void __fastcall AdjustHostName(UnicodeString &hostName, const UnicodeString prefix);
    void __fastcall RemoveProtocolPrefix(UnicodeString &hostName);
};
//---------------------------------------------------------------------------
class TStoredSessionList : public TNamedObjectList
{
public:
    explicit TStoredSessionList(bool aReadOnly = false);
    virtual ~TStoredSessionList();
    void __fastcall Load(const UnicodeString aKey, bool UseDefaults);
    void __fastcall Load();
    void __fastcall Save(bool All, bool Explicit);
    void __fastcall Saved();
    void __fastcall Export(const UnicodeString FileName);
    void __fastcall Load(THierarchicalStorage *Storage, bool AsModified = false,
              bool UseDefaults = false);
    void __fastcall Save(THierarchicalStorage *Storage, bool All = false);
    void __fastcall SelectAll(bool Select);
    void __fastcall Import(TStoredSessionList *From, bool OnlySelected);
    void __fastcall RecryptPasswords();
    void __fastcall SelectSessionsToImport(TStoredSessionList *Dest, bool SSHOnly);
    void __fastcall Cleanup();
    size_t __fastcall IndexOf(TSessionData *Data);
    TSessionData * __fastcall NewSession(const UnicodeString SessionName, TSessionData *Session);
    TSessionData *__fastcall ParseUrl(const UnicodeString Url, TOptions *Options, bool &DefaultsOnly,
                           UnicodeString *FileName = NULL, bool *ProtocolDefined = NULL);
    TSessionData * __fastcall GetSession(size_t Index) { return static_cast<TSessionData *>(AtObject(Index)); }
    TSessionData * __fastcall GetDefaultSettings() const { return FDefaultSettings; }
    TSessionData * __fastcall GetSessionByName(const UnicodeString SessionName);
    void __fastcall SetDefaultSettings(TSessionData *value);

    static void __fastcall ImportHostKeys(const UnicodeString TargetKey,
                               const UnicodeString SourceKey, TStoredSessionList *Sessions,
                               bool OnlySelected);

private:
    TSessionData *FDefaultSettings;
    bool FReadOnly;
    void __fastcall DoSave(THierarchicalStorage *Storage, bool All, bool RecryptPasswordOnly);
    void __fastcall DoSave(bool All, bool Explicit, bool RecryptPasswordOnly);
    void __fastcall DoSave(THierarchicalStorage *Storage,
                TSessionData *Data, bool All, bool RecryptPasswordOnly,
                TSessionData *FactoryDefaults);
};
//---------------------------------------------------------------------------
bool GetCodePageInfo(UINT CodePage, CPINFOEX &CodePageInfoEx);
unsigned int GetCodePageAsNumber(const UnicodeString CodePage);
UnicodeString GetCodePageAsString(unsigned int cp);
//---------------------------------------------------------------------------
#endif
