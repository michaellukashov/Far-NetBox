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
               sbRekey2, sbPKSessID2, sbMaxPkt2
             };
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
    int FFtpPingInterval;
    TPingType FFtpPingType;
    TFtps FFtps;
    TAutoSwitch FNotUtf;
    std::wstring FHostKey;

    std::wstring FOrigHostName;
    int FOrigPortNumber;
    TProxyMethod FOrigProxyMethod;
    TSessionSource FSource;

    void SavePasswords(THierarchicalStorage *Storage, bool PuttyExport);
    void Modify();
    static std::wstring EncryptPassword(const std::wstring Password, const std::wstring Key);
    static std::wstring DecryptPassword(const std::wstring Password, const std::wstring Key);
    static std::wstring StronglyRecryptPassword(const std::wstring Password, const std::wstring Key);

    std::wstring GetInternalStorageKey();

public:
    explicit TSessionData(const std::wstring aName);
    void Default();
    void NonPersistant();
    void Load(THierarchicalStorage *Storage);
    void Save(THierarchicalStorage *Storage, bool PuttyExport,
              const TSessionData *Default = NULL);
    void SaveRecryptedPasswords(THierarchicalStorage *Storage);
    void RecryptPasswords();
    bool HasAnyPassword();
    void Remove();
    virtual void Assign(nb::TPersistent *Source);
    bool ParseUrl(const std::wstring Url, TOptions *Options,
                  TStoredSessionList *StoredSessions, bool &DefaultsOnly,
                  std::wstring *FileName, bool *AProtocolDefined);
    bool ParseOptions(TOptions *Options);
    void ConfigureTunnel(size_t PortNumber);
    void RollbackTunnel();
    void ExpandEnvironmentVariables();
    static void ValidatePath(const std::wstring Path);
    static void ValidateName(const std::wstring Name);

    std::wstring GetHostName() const { return FHostName; }
    size_t GetPortNumber() const { return FPortNumber; }
    TLoginType GetLoginType() const;
    void SetLoginType(TLoginType value);
    std::wstring GetUserName() const { return FUserName; }
    void SetPassword(const std::wstring value);
    std::wstring GetPassword() const;
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
    bool GetUsesSsh();
    bool GetSsh2DES() const { return FSsh2DES; }
    bool GetSshNoUserAuth() const { return FSshNoUserAuth; }
    void SetCipher(size_t Index, TCipher value);
    TCipher GetCipher(size_t Index) const;
    void SetKex(size_t Index, TKex value);
    TKex GetKex(size_t Index) const;
    const std::wstring GetPublicKeyFile() const { return FPublicKeyFile; }
    TProtocol GetProtocol() const { return FProtocol; }
    void SetProtocolStr(const std::wstring value);
    std::wstring GetProtocolStr() const;
    TFSProtocol GetFSProtocol() const { return FFSProtocol; }
    std::wstring GetFSProtocolStr();
    bool GetModified() const { return FModified; }
    void SetModified(bool value) { FModified = value; }
    bool GetCanLogin();
    bool GetClearAliases() const { return FClearAliases; }
    void SetPingIntervalDT(nb::TDateTime value);
    nb::TDateTime GetPingIntervalDT() const;
    nb::TDateTime GetTimeDifference() const { return FTimeDifference; }
    TPingType GetPingType() const { return FPingType; }
    std::wstring GetSessionName();
    bool HasSessionName();
    std::wstring GetDefaultSessionName();
    std::wstring GetSessionUrl();
    std::wstring GetLocalDirectory() const { return FLocalDirectory; }
    std::wstring GetRemoteDirectory() const { return FRemoteDirectory; }
    void SetRemoteDirectory(const std::wstring value);
    bool GetUpdateDirectories() const { return FUpdateDirectories; }
    bool GetCacheDirectories() const { return FCacheDirectories; }
    bool GetCacheDirectoryChanges() const { return FCacheDirectoryChanges; }
    bool GetPreserveDirectoryChanges() const { return FPreserveDirectoryChanges; }
    bool GetLockInHome() const { return FLockInHome; }
    bool GetSpecial() const { return FSpecial; }
    bool GetSelected() const { return FSelected; }
    void SetSelected(bool value) { FSelected = value; }
    std::wstring GetInfoTip();
    bool GetDefaultShell();
    void SetDefaultShell(bool value);
    void SetDetectReturnVar(bool value);
    bool GetDetectReturnVar();
    TEOLType GetEOLType() const { return FEOLType; }
    bool GetLookupUserGroups() const { return FLookupUserGroups; }
    std::wstring GetReturnVar() const { return FReturnVar; }
    bool GetScp1Compatibility() const { return FScp1Compatibility; }
    std::wstring GetShell() const { return FShell; }
    std::wstring GetSftpServer() const { return FSftpServer; }
    int GetTimeout() const { return FTimeout; }
    nb::TDateTime GetTimeoutDT();
    bool GetUnsetNationalVars() const { return FUnsetNationalVars; }
    bool GetIgnoreLsWarnings() const { return FIgnoreLsWarnings; }
    bool GetTcpNoDelay() const { return FTcpNoDelay; }
    std::wstring GetSshProtStr();
    void SetCipherList(const std::wstring value);
    std::wstring GetCipherList() const;
    void SetKexList(const std::wstring value);
    std::wstring GetKexList() const;
    TProxyMethod GetProxyMethod() const { return FProxyMethod; }
    std::wstring GetProxyHost() const { return FProxyHost; }
    int GetProxyPort() const { return FProxyPort; }
    std::wstring GetProxyUsername() const { return FProxyUsername; }
    std::wstring GetProxyPassword() const;
    void SetProxyPassword(const std::wstring value);
    std::wstring GetProxyTelnetCommand() const { return FProxyTelnetCommand; }
    std::wstring GetProxyLocalCommand() const { return FProxyLocalCommand; }
    TAutoSwitch GetProxyDNS() const { return FProxyDNS; }
    bool GetProxyLocalhost() const { return FProxyLocalhost; }
    size_t GetFtpProxyLogonType() const { return FFtpProxyLogonType; }
    void SetBug(TSshBug Bug, TAutoSwitch value);
    TAutoSwitch GetBug(TSshBug Bug) const;
    std::wstring GetCustomParam1() const { return FCustomParam1; }
    std::wstring GetCustomParam2() const { return FCustomParam2; }
    std::wstring GetSessionKey();
    bool GetResolveSymlinks() const { return FResolveSymlinks; }
    size_t GetSFTPDownloadQueue() const { return FSFTPDownloadQueue; }
    void SetSFTPDownloadQueue(size_t value);
    size_t GetSFTPUploadQueue() const { return FSFTPUploadQueue; }
    void SetSFTPUploadQueue(size_t value);
    size_t GetSFTPListingQueue() const { return FSFTPListingQueue; }
    void SetSFTPListingQueue(size_t value);
    size_t GetSFTPMaxVersion() const { return FSFTPMaxVersion; }
    size_t GetSFTPMinPacketSize() const { return FSFTPMinPacketSize; }
    size_t GetSFTPMaxPacketSize() const { return FSFTPMaxPacketSize; }
    void SetSFTPBug(TSftpBug Bug, TAutoSwitch value);
    TAutoSwitch GetSFTPBug(TSftpBug Bug) const;
    TAutoSwitch GetSCPLsFullTime() const { return FSCPLsFullTime; }
    TAutoSwitch GetFtpListAll() const { return FFtpListAll; }
    TDSTMode GetDSTMode() const { return FDSTMode; }
    bool GetDeleteToRecycleBin() const { return FDeleteToRecycleBin; }
    bool GetOverwrittenToRecycleBin() const { return FOverwrittenToRecycleBin; }
    std::wstring GetRecycleBinPath() const { return FRecycleBinPath; }
    std::wstring GetPostLoginCommands() const { return FPostLoginCommands; }
    TAddressFamily GetAddressFamily() const { return FAddressFamily; }
    std::wstring GetRekeyData() const { return FRekeyData; }
    unsigned int GetRekeyTime() const { return FRekeyTime; }
    int GetColor() const { return FColor; }
    bool GetTunnel() const { return FTunnel; }
    void SetTunnel(bool value);
    std::wstring GetTunnelHostName() const { return FTunnelHostName; }
    size_t GetTunnelPortNumber() const { return FTunnelPortNumber; }
    std::wstring GetTunnelUserName() const { return FTunnelUserName; }
    void SetTunnelPassword(const std::wstring value);
    std::wstring GetTunnelPassword();
    std::wstring GetTunnelPublicKeyFile() const { return FTunnelPublicKeyFile; }
    bool GetTunnelAutoassignLocalPortNumber();
    size_t GetTunnelLocalPortNumber() const { return FTunnelLocalPortNumber; }
    std::wstring GetTunnelPortFwd() const { return FTunnelPortFwd; }
    void SetTunnelPortFwd(const std::wstring value);
    bool GetFtpPasvMode() const { return FFtpPasvMode; }
    bool GetFtpAllowEmptyPassword() const { return FFtpAllowEmptyPassword; }
    void SetFtpAllowEmptyPassword(bool value);
    TFtpEncryptionSwitch GetFtpEncryption() const { return FFtpEncryption; }
    void SetFtpEncryption(TFtpEncryptionSwitch value);
    bool GetFtpForcePasvIp() const { return FFtpForcePasvIp; }
    std::wstring GetFtpAccount() const { return FFtpAccount; }
    int GetFtpPingInterval() const { return FFtpPingInterval; }
    nb::TDateTime GetFtpPingIntervalDT();
    TPingType GetFtpPingType() const { return FFtpPingType; }
    TFtps GetFtps() const { return FFtps; }
    TAutoSwitch GetNotUtf() const { return FNotUtf; }
    std::wstring GetHostKey() const { return FHostKey; }
    std::wstring GetStorageKey();
    std::wstring GetOrigHostName() const { return FOrigHostName; }
    int GetOrigPortNumber() const { return FOrigPortNumber; }
    std::wstring GetLocalName();
    std::wstring GetSource();

    void SetHostName(const std::wstring value);
    void SetPortNumber(size_t value);
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
    void SetFtpProxyLogonType(size_t value);
    void SetCustomParam1(const std::wstring value);
    void SetCustomParam2(const std::wstring value);
    void SetResolveSymlinks(bool value);
    void SetSFTPMaxVersion(size_t value);
    void SetSFTPMinPacketSize(size_t value);
    void SetSFTPMaxPacketSize(size_t value);
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
    void SetTunnelPortNumber(size_t value);
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
    void AdjustHostName(std::wstring &hostName, const std::wstring prefix);
    void RemoveProtocolPrefix(std::wstring &hostName);
};
//---------------------------------------------------------------------------
class TStoredSessionList : public TNamedObjectList
{
public:
    explicit TStoredSessionList(bool aReadOnly = false);
    virtual ~TStoredSessionList();
    void Load(const std::wstring aKey, bool UseDefaults);
    void Load();
    void Save(bool All, bool Explicit);
    void Saved();
    void Export(const std::wstring FileName);
    void Load(THierarchicalStorage *Storage, bool AsModified = false,
              bool UseDefaults = false);
    void Save(THierarchicalStorage *Storage, bool All = false);
    void SelectAll(bool Select);
    void Import(TStoredSessionList *From, bool OnlySelected);
    void RecryptPasswords();
    void SelectSessionsToImport(TStoredSessionList *Dest, bool SSHOnly);
    void Cleanup();
    size_t IndexOf(TSessionData *Data);
    TSessionData *NewSession(const std::wstring SessionName, TSessionData *Session);
    TSessionData *ParseUrl(const std::wstring Url, TOptions *Options, bool &DefaultsOnly,
                           std::wstring *FileName = NULL, bool *ProtocolDefined = NULL);
    TSessionData *GetSession(size_t Index) { return static_cast<TSessionData *>(AtObject(Index)); }
    TSessionData *GetDefaultSettings() const { return FDefaultSettings; }
    void SetDefaultSettings(TSessionData *value);

    static void ImportHostKeys(const std::wstring TargetKey,
                               const std::wstring SourceKey, TStoredSessionList *Sessions,
                               bool OnlySelected);

private:
    TSessionData *FDefaultSettings;
    bool FReadOnly;
    void DoSave(THierarchicalStorage *Storage, bool All, bool RecryptPasswordOnly);
    void DoSave(bool All, bool Explicit, bool RecryptPasswordOnly);
    void DoSave(THierarchicalStorage *Storage,
                TSessionData *Data, bool All, bool RecryptPasswordOnly,
                TSessionData *FactoryDefaults);
};
//---------------------------------------------------------------------------
#endif
