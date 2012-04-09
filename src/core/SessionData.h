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
extern const std::wstring CONST_LOGIN_ANONYMOUS;
extern const int DefaultSendBuf;
extern const std::wstring AnonymousUserName;
extern const std::wstring AnonymousPassword;
//---------------------------------------------------------------------------
class TStoredSessionList;
//---------------------------------------------------------------------------
class TSessionData : public TNamedObject
{
    friend class TStoredSessionList;

private:
    std::wstring FHostName;
    size_t FPortNumber;
    std::wstring FUserName;
    std::wstring FPassword;
    bool FPasswordless;
    size_t FPingInterval;
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
    bool FSynchronizeBrowsing;
    bool FUpdateDirectories;
    bool FCacheDirectories;
    bool FCacheDirectoryChanges;
    bool FPreserveDirectoryChanges;
    bool FSelected;
    TAutoSwitch FLookupUserGroups;
    std::wstring FReturnVar;
    bool FScp1Compatibility;
    std::wstring FShell;
    std::wstring FSftpServer;
    int FTimeout;
    bool FUnsetNationalVars;
    bool FIgnoreLsWarnings;
    bool FTcpNoDelay;
    int FSendBuf;
    bool FSshSimple;
    TProxyMethod FProxyMethod;
    std::wstring FProxyHost;
    int FProxyPort;
    std::wstring FProxyUsername;
    std::wstring FProxyPassword;
    std::wstring FProxyTelnetCommand;
    std::wstring FProxyLocalCommand;
    TAutoSwitch FProxyDNS;
    bool FProxyLocalhost;
    size_t FFtpProxyLogonType;
    TAutoSwitch FBugs[BUG_COUNT];
    std::wstring FCustomParam1;
    std::wstring FCustomParam2;
    bool FResolveSymlinks;
    nb::TDateTime FTimeDifference;
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
    std::wstring FRecycleBinPath;
    std::wstring FPostLoginCommands;
    TAutoSwitch FSCPLsFullTime;
    TAutoSwitch FFtpListAll;
    TAddressFamily FAddressFamily;
    std::wstring FCodePage;
    std::wstring FRekeyData;
    unsigned int FRekeyTime;
    int FColor;
    bool FTunnel;
    std::wstring FTunnelHostName;
    size_t FTunnelPortNumber;
    std::wstring FTunnelUserName;
    std::wstring FTunnelPassword;
    std::wstring FTunnelPublicKeyFile;
    size_t FTunnelLocalPortNumber;
    std::wstring FTunnelPortFwd;
    bool FFtpPasvMode;
    bool FFtpForcePasvIp;
    bool FFtpAllowEmptyPassword;
    TFtpEncryptionSwitch FFtpEncryption;
    TLoginType FLoginType;
    std::wstring FFtpAccount;
    size_t FFtpPingInterval;
    TPingType FFtpPingType;
    TFtps FFtps;
    TAutoSwitch FNotUtf;
    std::wstring FHostKey;

    std::wstring FOrigHostName;
    int FOrigPortNumber;
    TProxyMethod FOrigProxyMethod;
    TSessionSource FSource;
    int FNumberOfRetries;
    bool FSslSessionReuse;

    void __fastcall SavePasswords(THierarchicalStorage *Storage, bool PuttyExport);
    void __fastcall Modify();
    void __fastcall DoLoad(THierarchicalStorage * Storage, bool & RewritePassword);
    static std::wstring __fastcall EncryptPassword(const std::wstring Password, const std::wstring Key);
    static std::wstring __fastcall DecryptPassword(const std::wstring Password, const std::wstring Key);
    static std::wstring __fastcall StronglyRecryptPassword(const std::wstring Password, const std::wstring Key);

    std::wstring __fastcall GetInternalStorageKey();

public:
    explicit TSessionData(const std::wstring aName);
    void __fastcall Default();
    void __fastcall NonPersistant();
    void __fastcall Load(THierarchicalStorage *Storage);
    void __fastcall Save(THierarchicalStorage *Storage, bool PuttyExport,
              const TSessionData *Default = NULL);
    void __fastcall SaveRecryptedPasswords(THierarchicalStorage *Storage);
    void __fastcall RecryptPasswords();
    bool __fastcall HasAnyPassword();
    void __fastcall Remove();
    virtual void __fastcall Assign(nb::TPersistent *Source);
    bool __fastcall ParseUrl(const std::wstring Url, TOptions *Options,
                  TStoredSessionList *StoredSessions, bool &DefaultsOnly,
                  std::wstring *FileName, bool *AProtocolDefined);
    bool __fastcall ParseOptions(TOptions *Options);
    void __fastcall ConfigureTunnel(size_t PortNumber);
    void __fastcall RollbackTunnel();
    void __fastcall ExpandEnvironmentVariables();
    static void __fastcall ValidatePath(const std::wstring Path);
    static void __fastcall ValidateName(const std::wstring Name);

    std::wstring GetHostName() const { return FHostName; }
    size_t GetPortNumber() const { return FPortNumber; }
    TLoginType __fastcall GetLoginType() const;
    void __fastcall SetLoginType(TLoginType value);
    std::wstring GetUserName() const { return FUserName; }
    std::wstring __fastcall GetUserNameExpanded();
    void __fastcall SetPassword(const std::wstring value);
    std::wstring __fastcall GetPassword() const;
    bool GetPasswordless() const { return FPasswordless; }
    size_t GetPingInterval() const { return FPingInterval; }
    bool GetTryAgent() const { return FTryAgent; }
    bool GetAgentFwd() const { return FAgentFwd; }
    const std::wstring GetListingCommand() const { return FListingCommand; }
    bool GetAuthTIS() const { return FAuthTIS; }
    bool GetAuthKI() const { return FAuthKI; }
    bool GetAuthKIPassword() const { return FAuthKIPassword; }
    bool GetAuthGSSAPI() const { return FAuthGSSAPI; }
    bool GetGSSAPIFwdTGT() const { return FGSSAPIFwdTGT; }
    const std::wstring GetGSSAPIServerRealm() const { return FGSSAPIServerRealm; }
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
    const std::wstring GetPublicKeyFile() const { return FPublicKeyFile; }
    TProtocol GetProtocol() const { return FProtocol; }
    void __fastcall SetProtocolStr(const std::wstring value);
    std::wstring __fastcall GetProtocolStr() const;
    TFSProtocol GetFSProtocol() const { return FFSProtocol; }
    std::wstring __fastcall GetFSProtocolStr();
    bool GetModified() const { return FModified; }
    void __fastcall SetModified(bool value) { FModified = value; }
    bool __fastcall GetCanLogin();
    bool GetClearAliases() const { return FClearAliases; }
    void __fastcall SetPingIntervalDT(nb::TDateTime value);
    nb::TDateTime __fastcall GetPingIntervalDT() const;
    nb::TDateTime __fastcall GetTimeDifference() const { return FTimeDifference; }
    TPingType GetPingType() const { return FPingType; }
    std::wstring __fastcall GetSessionName();
    bool __fastcall HasSessionName();
    std::wstring __fastcall GetDefaultSessionName();
    std::wstring __fastcall GetSessionUrl();
    std::wstring __fastcall GetLocalDirectory() const { return FLocalDirectory; }
    std::wstring __fastcall GetRemoteDirectory() const { return FRemoteDirectory; }
    void __fastcall SetRemoteDirectory(const std::wstring value);
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
    std::wstring __fastcall GetInfoTip();
    bool __fastcall GetDefaultShell();
    void __fastcall SetDefaultShell(bool value);
    void __fastcall SetDetectReturnVar(bool value);
    bool __fastcall GetDetectReturnVar();
    TEOLType GetEOLType() const { return FEOLType; }
    TAutoSwitch GetLookupUserGroups() const { return FLookupUserGroups; }
    std::wstring GetReturnVar() const { return FReturnVar; }
    bool GetScp1Compatibility() const { return FScp1Compatibility; }
    std::wstring GetShell() const { return FShell; }
    std::wstring GetSftpServer() const { return FSftpServer; }
    int GetTimeout() const { return FTimeout; }
    nb::TDateTime __fastcall GetTimeoutDT();
    bool GetUnsetNationalVars() const { return FUnsetNationalVars; }
    bool GetIgnoreLsWarnings() const { return FIgnoreLsWarnings; }
    bool GetTcpNoDelay() const { return FTcpNoDelay; }
    int __fastcall GetSendBuf() const { return FSendBuf; }
    void __fastcall SetSendBuf(int value);
    bool __fastcall GetSshSimple() const { return FSshSimple; }
    void __fastcall SetSshSimple(bool value);
    std::wstring __fastcall GetSshProtStr();
    void __fastcall SetCipherList(const std::wstring value);
    std::wstring __fastcall GetCipherList() const;
    void __fastcall SetKexList(const std::wstring value);
    std::wstring __fastcall GetKexList() const;
    TProxyMethod __fastcall GetProxyMethod() const { return FProxyMethod; }
    std::wstring __fastcall GetProxyHost() const { return FProxyHost; }
    int __fastcall GetProxyPort() const { return FProxyPort; }
    std::wstring __fastcall GetProxyUsername() const { return FProxyUsername; }
    std::wstring __fastcall GetProxyPassword() const;
    void __fastcall __fastcall SetProxyPassword(const std::wstring value);
    std::wstring __fastcall GetProxyTelnetCommand() const { return FProxyTelnetCommand; }
    std::wstring __fastcall GetProxyLocalCommand() const { return FProxyLocalCommand; }
    TAutoSwitch __fastcall GetProxyDNS() const { return FProxyDNS; }
    bool __fastcall GetProxyLocalhost() const { return FProxyLocalhost; }
    size_t __fastcall GetFtpProxyLogonType() const { return FFtpProxyLogonType; }
    void __fastcall SetBug(TSshBug Bug, TAutoSwitch value);
    TAutoSwitch __fastcall GetBug(TSshBug Bug) const;
    std::wstring __fastcall GetCustomParam1() const { return FCustomParam1; }
    std::wstring __fastcall GetCustomParam2() const { return FCustomParam2; }
    std::wstring __fastcall GetSessionKey();
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
    std::wstring __fastcall GetRecycleBinPath() const { return FRecycleBinPath; }
    std::wstring __fastcall GetPostLoginCommands() const { return FPostLoginCommands; }
    TAddressFamily __fastcall GetAddressFamily() const { return FAddressFamily; }
    std::wstring __fastcall GetCodePage() const { return FCodePage; }
    unsigned int __fastcall GetCodePageAsNumber() const;
    std::wstring __fastcall GetRekeyData() const { return FRekeyData; }
    unsigned int __fastcall GetRekeyTime() const { return FRekeyTime; }
    int __fastcall GetColor() const { return FColor; }
    bool __fastcall GetTunnel() const { return FTunnel; }
    void __fastcall SetTunnel(bool value);
    std::wstring __fastcall GetTunnelHostName() const { return FTunnelHostName; }
    size_t __fastcall GetTunnelPortNumber() const { return FTunnelPortNumber; }
    std::wstring __fastcall GetTunnelUserName() const { return FTunnelUserName; }
    void __fastcall SetTunnelPassword(const std::wstring value);
    std::wstring __fastcall GetTunnelPassword();
    std::wstring __fastcall GetTunnelPublicKeyFile() const { return FTunnelPublicKeyFile; }
    bool __fastcall GetTunnelAutoassignLocalPortNumber();
    size_t __fastcall GetTunnelLocalPortNumber() const { return FTunnelLocalPortNumber; }
    std::wstring __fastcall GetTunnelPortFwd() const { return FTunnelPortFwd; }
    void __fastcall SetTunnelPortFwd(const std::wstring value);
    bool __fastcall GetFtpPasvMode() const { return FFtpPasvMode; }
    bool __fastcall GetFtpAllowEmptyPassword() const { return FFtpAllowEmptyPassword; }
    void __fastcall SetFtpAllowEmptyPassword(bool value);
    TFtpEncryptionSwitch __fastcall GetFtpEncryption() const { return FFtpEncryption; }
    void __fastcall SetFtpEncryption(TFtpEncryptionSwitch value);
    bool __fastcall GetFtpForcePasvIp() const { return FFtpForcePasvIp; }
    std::wstring __fastcall GetFtpAccount() const { return FFtpAccount; }
    size_t __fastcall GetFtpPingInterval() const { return FFtpPingInterval; }
    nb::TDateTime __fastcall GetFtpPingIntervalDT();
    TPingType __fastcall GetFtpPingType() const { return FFtpPingType; }
    TFtps __fastcall GetFtps() const { return FFtps; }
    TAutoSwitch __fastcall GetNotUtf() const { return FNotUtf; }
    std::wstring __fastcall GetHostKey() const { return FHostKey; }
    std::wstring __fastcall GetStorageKey();
    std::wstring __fastcall GetOrigHostName() const { return FOrigHostName; }
    int __fastcall GetOrigPortNumber() const { return FOrigPortNumber; }
    std::wstring __fastcall GetLocalName();
    std::wstring __fastcall GetSource();

    void __fastcall SetHostName(const std::wstring value);
    std::wstring __fastcall GetHostNameExpanded();
    void __fastcall SetPortNumber(size_t value);
    void __fastcall SetUserName(const std::wstring value);
    void __fastcall SetPasswordless(bool value);
    void __fastcall SetPingInterval(size_t value);
    void __fastcall SetTryAgent(bool value);
    void __fastcall SetAgentFwd(bool value);
    void __fastcall SetAuthTIS(bool value);
    void __fastcall SetAuthKI(bool value);
    void __fastcall SetAuthKIPassword(bool value);
    void __fastcall SetAuthGSSAPI(bool value);
    void __fastcall SetGSSAPIFwdTGT(bool value);
    void __fastcall SetGSSAPIServerRealm(const std::wstring value);
    void __fastcall SetChangeUsername(bool value);
    void __fastcall SetCompression(bool value);
    void __fastcall SetSshProt(TSshProt value);
    void __fastcall SetSsh2DES(bool value);
    void __fastcall SetSshNoUserAuth(bool value);
    void __fastcall SetPublicKeyFile(const std::wstring value);

    void __fastcall SetTimeDifference(nb::TDateTime value);
    void __fastcall SetPingType(TPingType value);
    void __fastcall SetProtocol(TProtocol value);
    void __fastcall SetFSProtocol(TFSProtocol value);
    void __fastcall SetLocalDirectory(const std::wstring value);
    void __fastcall SetUpdateDirectories(bool value);
    void __fastcall SetCacheDirectories(bool value);
    void __fastcall SetCacheDirectoryChanges(bool value);
    void __fastcall SetPreserveDirectoryChanges(bool value);
    void __fastcall SetLockInHome(bool value);
    void __fastcall SetSpecial(bool value);
    void __fastcall SetListingCommand(const std::wstring value);
    void __fastcall SetClearAliases(bool value);
    void __fastcall SetEOLType(TEOLType value);
    void __fastcall SetLookupUserGroups(TAutoSwitch value);
    void __fastcall SetReturnVar(const std::wstring value);
    void __fastcall SetScp1Compatibility(bool value);
    void __fastcall SetShell(const std::wstring value);
    void __fastcall SetSftpServer(const std::wstring value);
    void __fastcall SetTimeout(int value);
    void __fastcall SetUnsetNationalVars(bool value);
    void __fastcall SetIgnoreLsWarnings(bool value);
    void __fastcall SetTcpNoDelay(bool value);
    void __fastcall SetProxyMethod(TProxyMethod value);
    void __fastcall SetProxyHost(const std::wstring value);
    void __fastcall SetProxyPort(int value);
    void __fastcall SetProxyUsername(const std::wstring value);
    void __fastcall SetProxyTelnetCommand(const std::wstring value);
    void __fastcall SetProxyLocalCommand(const std::wstring value);
    void __fastcall SetProxyDNS(TAutoSwitch value);
    void __fastcall SetProxyLocalhost(bool value);
    void __fastcall SetFtpProxyLogonType(size_t value);
    void __fastcall SetCustomParam1(const std::wstring value);
    void __fastcall SetCustomParam2(const std::wstring value);
    void __fastcall SetResolveSymlinks(bool value);
    void __fastcall SetSFTPMaxVersion(size_t value);
    void __fastcall SetSFTPMinPacketSize(size_t value);
    void __fastcall SetSFTPMaxPacketSize(size_t value);
    void __fastcall SetSCPLsFullTime(TAutoSwitch value);
    void __fastcall SetFtpListAll(TAutoSwitch value);
    void __fastcall SetDSTMode(TDSTMode value);
    void __fastcall SetDeleteToRecycleBin(bool value);
    void __fastcall SetOverwrittenToRecycleBin(bool value);
    void __fastcall SetRecycleBinPath(const std::wstring value);
    void __fastcall SetPostLoginCommands(const std::wstring value);
    void __fastcall SetAddressFamily(TAddressFamily value);
    void __fastcall SetCodePage(const std::wstring value);
    void __fastcall SetRekeyData(const std::wstring value);
    void __fastcall SetRekeyTime(unsigned int value);
    void __fastcall SetColor(int value);
    void __fastcall SetTunnelHostName(const std::wstring value);
    void __fastcall SetTunnelPortNumber(size_t value);
    void __fastcall SetTunnelUserName(const std::wstring value);
    void __fastcall SetTunnelPublicKeyFile(const std::wstring value);
    void __fastcall SetTunnelLocalPortNumber(size_t value);
    void __fastcall SetFtpPasvMode(bool value);
    void __fastcall SetFtpForcePasvIp(bool value);
    void __fastcall SetFtpAccount(const std::wstring value);
    void __fastcall SetFtpPingInterval(size_t value);
    void __fastcall SetFtpPingType(TPingType value);
    void __fastcall SetFtps(TFtps value);
    void __fastcall SetNotUtf(TAutoSwitch value);
    void __fastcall SetHostKey(const std::wstring value);

    int __fastcall GetNumberOfRetries() const { return FNumberOfRetries; }
    void __fastcall SetNumberOfRetries(int value) { FNumberOfRetries = value; }
    bool __fastcall GetSslSessionReuse() const { return FSslSessionReuse; }
    void __fastcall SetSslSessionReuse(bool value);
private:
    void __fastcall AdjustHostName(std::wstring &hostName, const std::wstring prefix);
    void __fastcall RemoveProtocolPrefix(std::wstring &hostName);
};
//---------------------------------------------------------------------------
class TStoredSessionList : public TNamedObjectList
{
public:
    explicit TStoredSessionList(bool aReadOnly = false);
    virtual ~TStoredSessionList();
    void __fastcall Load(const std::wstring aKey, bool UseDefaults);
    void __fastcall Load();
    void __fastcall Save(bool All, bool Explicit);
    void __fastcall Saved();
    void __fastcall Export(const std::wstring FileName);
    void __fastcall Load(THierarchicalStorage *Storage, bool AsModified = false,
              bool UseDefaults = false);
    void __fastcall Save(THierarchicalStorage *Storage, bool All = false);
    void __fastcall SelectAll(bool Select);
    void __fastcall Import(TStoredSessionList *From, bool OnlySelected);
    void __fastcall RecryptPasswords();
    void __fastcall SelectSessionsToImport(TStoredSessionList *Dest, bool SSHOnly);
    void __fastcall Cleanup();
    size_t __fastcall IndexOf(TSessionData *Data);
    TSessionData * __fastcall NewSession(const std::wstring SessionName, TSessionData *Session);
    TSessionData *__fastcall ParseUrl(const std::wstring Url, TOptions *Options, bool &DefaultsOnly,
                           std::wstring *FileName = NULL, bool *ProtocolDefined = NULL);
    TSessionData * __fastcall GetSession(size_t Index) { return static_cast<TSessionData *>(AtObject(Index)); }
    TSessionData * __fastcall GetDefaultSettings() const { return FDefaultSettings; }
    TSessionData * __fastcall GetSessionByName(const std::wstring SessionName);
    void __fastcall SetDefaultSettings(TSessionData *value);

    static void __fastcall ImportHostKeys(const std::wstring TargetKey,
                               const std::wstring SourceKey, TStoredSessionList *Sessions,
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
unsigned int GetCodePageAsNumber(const std::wstring CodePage);
std::wstring GetCodePageAsString(unsigned int cp);
//---------------------------------------------------------------------------
#endif
