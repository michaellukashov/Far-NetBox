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
class TCriticalSection;
enum TAutoSwitch { asOn, asOff, asAuto };
enum TFtpEncryptionSwitch { fesPlainFTP, fesExplicitSSL, fesImplicit, fesExplicitTLS };
//---------------------------------------------------------------------------
class TConfiguration : public nb::TObject
{
private:
    bool FDontSave;
    bool FChanged;
    int FUpdating;
    nb::notify_signal_type FOnChange;

    void *FApplicationInfo;
    bool FLogging;
    bool FPermanentLogging;
    std::wstring FLogFileName;
    std::wstring FPermanentLogFileName;
    int FLogWindowLines;
    bool FLogFileAppend;
    size_t FLogProtocol;
    size_t FActualLogProtocol;
    bool FLogActions;
    bool FPermanentLogActions;
    bool FConfirmOverwriting;
    bool FConfirmResume;
    bool FAutoReadDirectoryAfterOp;
    int FSessionReopenAuto;
    int FSessionReopenBackground;
    int FSessionReopenTimeout;
    std::wstring FIniFileStorageName;
    size_t FTunnelLocalPortNumberLow;
    size_t FTunnelLocalPortNumberHigh;
    int FCacheDirectoryChangesMaxSize;
    bool FShowFtpWelcomeMessage;
    std::wstring FDefaultRandomSeedFile;
    std::wstring FRandomSeedFile;
    std::wstring FPuttyRegistryStorageKey;

    bool FDisablePasswordStoring;
    bool FForceBanners;
    bool FDisableAcceptingHostKeys;

    std::wstring TrimVersion(const std::wstring Version);
    void UpdateActualLogProtocol();

protected:
    TStorage FStorage;
    TCriticalSection *FCriticalSection;

    virtual void Changed();
    virtual void SaveData(THierarchicalStorage *Storage, bool All);
    virtual void LoadData(THierarchicalStorage *Storage);
    virtual void CopyData(THierarchicalStorage *Source, THierarchicalStorage *Target);
    virtual void LoadAdmin(THierarchicalStorage *Storage);
    virtual void Saved();
    void CleanupRegistry(const std::wstring CleanupSubKey);
    std::wstring BannerHash(const std::wstring Banner);

    virtual std::wstring ModuleFileName();

    std::wstring GetFileFileInfoString(const std::wstring Key,
                                       const std::wstring FileName);
    void *GetFileApplicationInfo(const std::wstring FileName);
    std::wstring GetFileProductVersion(const std::wstring FileName);
    std::wstring GetFileProductName(const std::wstring FileName);
    std::wstring GetFileCompanyName(const std::wstring FileName);

    bool GetPermanentLogging() { return FPermanentLogging; }
    void SetPermanentLogging(bool value) { FPermanentLogging = value; }
    std::wstring GetPermanentLogFileName() { return FPermanentLogFileName; }
    void SetPermanentLogFileName(const std::wstring value) { FPermanentLogFileName = value; }
    bool GetPermanentLogActions() { return FPermanentLogActions; }
    void SetPermanentLogActions(bool value) { FPermanentLogActions = value; }

public:
    TConfiguration();
    virtual ~TConfiguration();
    virtual void Default();
    virtual void Load();
    virtual void Save(bool All, bool Explicit);
    void Export(const std::wstring FileName);
    void CleanupConfiguration();
    void CleanupIniFile();
    void CleanupHostKeys();
    void CleanupRandomSeedFile();
    void BeginUpdate();
    void EndUpdate();
    void LoadDirectoryChangesCache(const std::wstring SessionKey,
                                   TRemoteDirectoryChangesCache *DirectoryChangesCache);
    void SaveDirectoryChangesCache(const std::wstring SessionKey,
                                   TRemoteDirectoryChangesCache *DirectoryChangesCache);
    bool ShowBanner(const std::wstring SessionKey, const std::wstring Banner);
    void NeverShowBanner(const std::wstring SessionKey, const std::wstring Banner);
    virtual THierarchicalStorage *CreateStorage();
    void TemporaryLogging(const std::wstring ALogFileName);
    virtual std::wstring EncryptPassword(const std::wstring Password, const std::wstring Key);
    virtual std::wstring DecryptPassword(const std::wstring Password, const std::wstring Key);
    virtual std::wstring StronglyRecryptPassword(const std::wstring Password, const std::wstring Key);

    VS_FIXEDFILEINFO GetFixedApplicationInfo();
    void *GetApplicationInfo();
    std::wstring GetStoredSessionsSubKey();
    std::wstring GetPuttyRegistryStorageKey() { return FPuttyRegistryStorageKey; }
    void SetPuttyRegistryStorageKey(const std::wstring value);
    std::wstring GetPuttySessionsKey();
    std::wstring GetRandomSeedFile() { return FRandomSeedFile; }
    void SetRandomSeedFile(const std::wstring value);
    std::wstring GetRandomSeedFileName();
    std::wstring GetSshHostKeysSubKey();
    std::wstring GetRootKeyStr();
    std::wstring GetConfigurationSubKey();
    TEOLType GetLocalEOLType();
    virtual std::wstring GetVersionStr();
    virtual std::wstring GetVersion();
    int GetCompoundVersion();
    std::wstring GetProductVersion();
    std::wstring GetProductName();
    std::wstring GetCompanyName();
    std::wstring GetFileInfoString(const std::wstring Key);
    std::wstring GetOSVersionStr();
    bool GetLogging() { return FLogging; }
    void SetLogging(bool value);
    std::wstring GetLogFileName() { return FLogFileName; }
    void SetLogFileName(const std::wstring value);
    bool GetLogToFile();
    void SetLogToFile(bool value);
    bool GetLogFileAppend() { return FLogFileAppend; }
    void SetLogFileAppend(bool value);
    size_t GetLogProtocol() { return FLogProtocol; }
    void SetLogProtocol(size_t value);
    size_t GetActualLogProtocol() { return FActualLogProtocol; }
    bool GetLogActions() { return FLogActions; }
    void SetLogActions(bool value);
    int GetLogWindowLines() { return FLogWindowLines; }
    void SetLogWindowLines(int value);
    bool GetLogWindowComplete();
    void SetLogWindowComplete(bool value);
    std::wstring GetDefaultLogFileName();
    const nb::notify_signal_type &GetOnChange() const { return FOnChange; }
    void SetOnChange(const nb::notify_slot_type &value) { FOnChange.connect(value); }
    virtual bool GetConfirmOverwriting();
    virtual void SetConfirmOverwriting(bool value);
    bool GetConfirmResume();
    void SetConfirmResume(bool value);
    bool GetAutoReadDirectoryAfterOp();
    void SetAutoReadDirectoryAfterOp(bool value);
    virtual bool GetRememberPassword();
    std::wstring GetPartialExt() const;
    int GetSessionReopenAuto() { return FSessionReopenAuto; }
    void SetSessionReopenAuto(int value);
    int GetSessionReopenBackground() { return FSessionReopenBackground; }
    void SetSessionReopenBackground(int value);
    int GetSessionReopenTimeout() { return FSessionReopenTimeout; }
    void SetSessionReopenTimeout(int value);
    size_t GetTunnelLocalPortNumberLow() { return FTunnelLocalPortNumberLow; }
    void SetTunnelLocalPortNumberLow(size_t value);
    size_t GetTunnelLocalPortNumberHigh() { return FTunnelLocalPortNumberHigh; }
    void SetTunnelLocalPortNumberHigh(size_t value);
    int GetCacheDirectoryChangesMaxSize() { return FCacheDirectoryChangesMaxSize; }
    void SetCacheDirectoryChangesMaxSize(int value);
    bool GetShowFtpWelcomeMessage() { return FShowFtpWelcomeMessage; }
    void SetShowFtpWelcomeMessage(bool value);

    std::wstring GetTimeFormat();
    virtual TStorage GetStorage();
    void SetStorage(TStorage value);
    std::wstring GetRegistryStorageKey();
    std::wstring GetIniFileStorageName();
    void SetIniFileStorageName(const std::wstring value);
    virtual std::wstring GetDefaultKeyFile();

    bool GetDisablePasswordStoring() { return FDisablePasswordStoring; }
    bool GetForceBanners() { return FForceBanners; }
    bool GetDisableAcceptingHostKeys() { return FDisableAcceptingHostKeys; }
    bool GetGSSAPIInstalled();
};

//---------------------------------------------------------------------------
class TShortCuts
{
public:
    void Add(nb::TShortCut ShortCut);
    bool Has(nb::TShortCut ShortCut) const;

private:
    std::set<nb::TShortCut> FShortCuts;
};
//---------------------------------------------------------------------------
