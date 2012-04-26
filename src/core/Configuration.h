//---------------------------------------------------------------------------
#pragma once

#include <set>

#include "Classes.h"
#include "RemoteFiles.h"
#include "FileBuffer.h"
#include "HierarchicalStorage.h"
//---------------------------------------------------------------------------
#define SET_CONFIG_PROPERTY_EX(PROPERTY, APPLY) \
  if (Get##PROPERTY() != value) { F ## PROPERTY = value; Changed(); APPLY; }
#define SET_CONFIG_PROPERTY(PROPERTY) \
  SET_CONFIG_PROPERTY_EX(PROPERTY, )
//---------------------------------------------------------------------------
#define CONST_DEFAULT_NUMBER_OF_RETRIES 2
//---------------------------------------------------------------------------
class TCriticalSection;
enum TAutoSwitch { asOn, asOff, asAuto };
enum TFtpEncryptionSwitch { fesPlainFTP, fesExplicitSSL, fesImplicit, fesExplicitTLS };
//---------------------------------------------------------------------------
class TConfiguration : public System::TObject
{
private:
    bool FDontSave;
    bool FChanged;
    int FUpdating;
    System::notify_signal_type FOnChange;

    void *FApplicationInfo;
    bool FLogging;
    bool FPermanentLogging;
    UnicodeString FLogFileName;
    UnicodeString FPermanentLogFileName;
    int FLogWindowLines;
    bool FLogFileAppend;
    size_t FLogProtocol;
    size_t FActualLogProtocol;
    bool FLogActions;
    bool FPermanentLogActions;
    UnicodeString FActionsLogFileName;
    UnicodeString FPermanentActionsLogFileName;
    bool FConfirmOverwriting;
    bool FConfirmResume;
    bool FAutoReadDirectoryAfterOp;
    int FSessionReopenAuto;
    int FSessionReopenAutoMaximumNumberOfRetries;
    int FSessionReopenBackground;
    size_t FSessionReopenTimeout;
    int FSessionReopenAutoStall;
    UnicodeString FIniFileStorageName;
    size_t FTunnelLocalPortNumberLow;
    size_t FTunnelLocalPortNumberHigh;
    int FCacheDirectoryChangesMaxSize;
    bool FShowFtpWelcomeMessage;
    UnicodeString FDefaultRandomSeedFile;
    UnicodeString FRandomSeedFile;
    UnicodeString FPuttyRegistryStorageKey;
    UnicodeString FExternalIpAddress;

    bool FDisablePasswordStoring;
    bool FForceBanners;
    bool FDisableAcceptingHostKeys;

protected:
    TStorage FStorage;
    TCriticalSection *FCriticalSection;

    virtual void __fastcall Changed();
    virtual void __fastcall SaveData(THierarchicalStorage *Storage, bool All);
    virtual void __fastcall LoadData(THierarchicalStorage *Storage);
    virtual void __fastcall CopyData(THierarchicalStorage *Source, THierarchicalStorage *Target);
    virtual void __fastcall LoadAdmin(THierarchicalStorage *Storage);
    virtual void __fastcall Saved();
    void CleanupRegistry(const UnicodeString CleanupSubKey);
    UnicodeString BannerHash(const UnicodeString Banner);

    virtual UnicodeString __fastcall ModuleFileName();

    UnicodeString GetFileFileInfoString(const UnicodeString Key,
                                       const UnicodeString FileName);
    void *GetFileApplicationInfo(const UnicodeString FileName);
    UnicodeString GetFileProductVersion(const UnicodeString FileName);
    UnicodeString GetFileProductName(const UnicodeString FileName);
    UnicodeString GetFileCompanyName(const UnicodeString FileName);

    bool GetPermanentLogging() { return FPermanentLogging; }
    void SetPermanentLogging(bool value) { FPermanentLogging = value; }
    UnicodeString GetPermanentLogFileName() { return FPermanentLogFileName; }
    void SetPermanentLogFileName(const UnicodeString value) { FPermanentLogFileName = value; }
    bool GetPermanentLogActions() { return FPermanentLogActions; }
    void SetPermanentLogActions(bool value) { FPermanentLogActions = value; }

public:
    TConfiguration();
    virtual ~TConfiguration();
    virtual void __fastcall Default();
    virtual void __fastcall Load();
    virtual void __fastcall Save(bool All, bool Explicit);
    void Export(const UnicodeString FileName);
    void CleanupConfiguration();
    void CleanupIniFile();
    void CleanupHostKeys();
    void CleanupRandomSeedFile();
    void BeginUpdate();
    void EndUpdate();
    void LoadDirectoryChangesCache(const UnicodeString SessionKey,
                                   TRemoteDirectoryChangesCache *DirectoryChangesCache);
    void SaveDirectoryChangesCache(const UnicodeString SessionKey,
                                   TRemoteDirectoryChangesCache *DirectoryChangesCache);
    bool ShowBanner(const UnicodeString SessionKey, const UnicodeString Banner);
    void NeverShowBanner(const UnicodeString SessionKey, const UnicodeString Banner);
    virtual THierarchicalStorage * __fastcall CreateStorage();
    void TemporaryLogging(const UnicodeString ALogFileName);
    virtual UnicodeString __fastcall EncryptPassword(const UnicodeString Password, const UnicodeString Key);
    virtual UnicodeString __fastcall DecryptPassword(const UnicodeString Password, const UnicodeString Key);
    virtual UnicodeString __fastcall StronglyRecryptPassword(const UnicodeString Password, const UnicodeString Key);

    UnicodeString GetPuttyRegistryStorageKey() { return FPuttyRegistryStorageKey; }
    bool GetLogging() { return FLogging; }
    UnicodeString GetLogFileName() { return FLogFileName; }
    void SetLogToFile(bool value);
    bool GetLogFileAppend() { return FLogFileAppend; }
    size_t GetLogProtocol() { return FLogProtocol; }
    void SetLogProtocol(size_t value);
    size_t GetActualLogProtocol() { return FActualLogProtocol; }
    bool GetLogActions() { return FLogActions; }
    int GetLogWindowLines() { return FLogWindowLines; }
    const System::notify_signal_type &GetOnChange() const { return FOnChange; }
    void SetOnChange(const TNotifyEvent &value) { FOnChange.connect(value); }
    int GetSessionReopenAuto() { return FSessionReopenAuto; }
    int GetSessionReopenAutoMaximumNumberOfRetries() { return FSessionReopenAutoMaximumNumberOfRetries; }
    void SetSessionReopenAutoMaximumNumberOfRetries(int value);
    int GetSessionReopenBackground() { return FSessionReopenBackground; }
    size_t GetSessionReopenTimeout() { return FSessionReopenTimeout; }
    size_t GetTunnelLocalPortNumberLow() { return FTunnelLocalPortNumberLow; }
    size_t GetTunnelLocalPortNumberHigh() { return FTunnelLocalPortNumberHigh; }
    int GetCacheDirectoryChangesMaxSize() { return FCacheDirectoryChangesMaxSize; }
    void SetCacheDirectoryChangesMaxSize(int value);
    bool GetShowFtpWelcomeMessage() { return FShowFtpWelcomeMessage; }

    UnicodeString GetTimeFormat();

    bool GetDisablePasswordStoring() { return FDisablePasswordStoring; }
    bool GetForceBanners() { return FForceBanners; }
    bool GetDisableAcceptingHostKeys() { return FDisableAcceptingHostKeys; }
    UnicodeString __fastcall GetStoredSessionsSubKey();
    virtual bool __fastcall GetRememberPassword();
    virtual bool __fastcall GetConfirmOverwriting();
    virtual void __fastcall SetConfirmOverwriting(bool value);
    bool __fastcall GetAutoReadDirectoryAfterOp();
    void __fastcall SetAutoReadDirectoryAfterOp(bool value);
    virtual UnicodeString __fastcall GetDefaultKeyFile();
    void __fastcall SetLogging(bool value);
    void __fastcall SetLogFileName(UnicodeString value);
    UnicodeString __fastcall GetOSVersionStr();
    VS_FIXEDFILEINFO GetFixedApplicationInfo();
    void * __fastcall GetApplicationInfo();
    virtual UnicodeString __fastcall GetVersionStr();
    virtual UnicodeString __fastcall GetVersion();
    UnicodeString __fastcall GetProductVersion();
    UnicodeString __fastcall GetProductName();
    UnicodeString __fastcall GetCompanyName();
    UnicodeString __fastcall TrimVersion(UnicodeString Version);
    UnicodeString __fastcall GetPuttySessionsKey();
    UnicodeString __fastcall GetRandomSeedFile() const { return FRandomSeedFile; }
    void __fastcall SetRandomSeedFile(UnicodeString value);
    UnicodeString __fastcall GetRandomSeedFileName();
    void __fastcall SetPuttyRegistryStorageKey(UnicodeString value);
    UnicodeString __fastcall GetSshHostKeysSubKey();
    UnicodeString __fastcall GetRootKeyStr();
    UnicodeString __fastcall GetConfigurationSubKey();
    TEOLType __fastcall GetLocalEOLType();
    bool __fastcall GetLogToFile();
    void __fastcall SetLogWindowLines(int value);
    void __fastcall SetLogWindowComplete(bool value);
    bool __fastcall GetLogWindowComplete();
    void __fastcall SetLogFileAppend(bool value);
    void __fastcall SetLogActions(bool value);
    UnicodeString GetActionsLogFileName() const { return FActionsLogFileName; }
    void __fastcall SetActionsLogFileName(UnicodeString value);
    UnicodeString __fastcall GetDefaultLogFileName();
    void __fastcall SetStorage(TStorage value);
    UnicodeString __fastcall GetRegistryStorageKey();
    UnicodeString __fastcall GetIniFileStorageName();
    void __fastcall SetIniFileStorageName(UnicodeString value);
    UnicodeString __fastcall GetPartialExt() const;
    UnicodeString __fastcall GetFileInfoString(const UnicodeString Key);
    bool __fastcall GetGSSAPIInstalled();
    void __fastcall SetSessionReopenAuto(size_t value);
    void __fastcall SetSessionReopenBackground(size_t value);
    void __fastcall SetSessionReopenTimeout(size_t value);
    void __fastcall SetSessionReopenAutoStall(size_t value);
    void __fastcall SetTunnelLocalPortNumberLow(size_t value);
    void __fastcall SetTunnelLocalPortNumberHigh(size_t value);
    void __fastcall SetCacheDirectoryChangesMaxSize(size_t value);
    void __fastcall SetShowFtpWelcomeMessage(bool value);
    int __fastcall GetCompoundVersion();
    void __fastcall UpdateActualLogProtocol();
    UnicodeString  __fastcall GetExternalIpAddress() const { return FExternalIpAddress; }
    void __fastcall SetExternalIpAddress(UnicodeString value);
    bool __fastcall GetConfirmResume();
    void __fastcall SetConfirmResume(bool value);
    virtual TStorage __fastcall GetStorage();
};

//---------------------------------------------------------------------------
class TShortCuts
{
public:
    void Add(System::TShortCut ShortCut);
    bool Has(System::TShortCut ShortCut) const;

private:
    std::set<System::TShortCut> FShortCuts;
};
//---------------------------------------------------------------------------
