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
    std::wstring FActionsLogFileName;
    std::wstring FPermanentActionsLogFileName;
    bool FConfirmOverwriting;
    bool FConfirmResume;
    bool FAutoReadDirectoryAfterOp;
    int FSessionReopenAuto;
    int FSessionReopenAutoMaximumNumberOfRetries;
    int FSessionReopenBackground;
    size_t FSessionReopenTimeout;
    int FSessionReopenAutoStall;
    std::wstring FIniFileStorageName;
    size_t FTunnelLocalPortNumberLow;
    size_t FTunnelLocalPortNumberHigh;
    int FCacheDirectoryChangesMaxSize;
    bool FShowFtpWelcomeMessage;
    std::wstring FDefaultRandomSeedFile;
    std::wstring FRandomSeedFile;
    std::wstring FPuttyRegistryStorageKey;
    std::wstring FExternalIpAddress;

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
    void CleanupRegistry(const std::wstring CleanupSubKey);
    std::wstring BannerHash(const std::wstring Banner);

    virtual std::wstring __fastcall ModuleFileName();

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
    virtual void __fastcall Default();
    virtual void __fastcall Load();
    virtual void __fastcall Save(bool All, bool Explicit);
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
    virtual THierarchicalStorage * __fastcall CreateStorage();
    void TemporaryLogging(const std::wstring ALogFileName);
    virtual std::wstring __fastcall EncryptPassword(const std::wstring Password, const std::wstring Key);
    virtual std::wstring __fastcall DecryptPassword(const std::wstring Password, const std::wstring Key);
    virtual std::wstring __fastcall StronglyRecryptPassword(const std::wstring Password, const std::wstring Key);

    std::wstring GetPuttyRegistryStorageKey() { return FPuttyRegistryStorageKey; }
    bool GetLogging() { return FLogging; }
    std::wstring GetLogFileName() { return FLogFileName; }
    void SetLogToFile(bool value);
    bool GetLogFileAppend() { return FLogFileAppend; }
    size_t GetLogProtocol() { return FLogProtocol; }
    void SetLogProtocol(size_t value);
    size_t GetActualLogProtocol() { return FActualLogProtocol; }
    bool GetLogActions() { return FLogActions; }
    int GetLogWindowLines() { return FLogWindowLines; }
    const nb::notify_signal_type &GetOnChange() const { return FOnChange; }
    void SetOnChange(const nb::notify_slot_type &value) { FOnChange.connect(value); }
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

    std::wstring GetTimeFormat();

    bool GetDisablePasswordStoring() { return FDisablePasswordStoring; }
    bool GetForceBanners() { return FForceBanners; }
    bool GetDisableAcceptingHostKeys() { return FDisableAcceptingHostKeys; }
    std::wstring __fastcall GetStoredSessionsSubKey();
    virtual bool __fastcall GetRememberPassword();
    virtual bool __fastcall GetConfirmOverwriting();
    virtual void __fastcall SetConfirmOverwriting(bool value);
    bool __fastcall GetAutoReadDirectoryAfterOp();
    void __fastcall SetAutoReadDirectoryAfterOp(bool value);
    virtual std::wstring __fastcall GetDefaultKeyFile();
    void __fastcall SetLogging(bool value);
    void __fastcall SetLogFileName(std::wstring value);
    std::wstring __fastcall GetOSVersionStr();
    VS_FIXEDFILEINFO GetFixedApplicationInfo();
    void * __fastcall GetApplicationInfo();
    virtual std::wstring __fastcall GetVersionStr();
    virtual std::wstring __fastcall GetVersion();
    std::wstring __fastcall GetProductVersion();
    std::wstring __fastcall GetProductName();
    std::wstring __fastcall GetCompanyName();
    std::wstring __fastcall TrimVersion(std::wstring Version);
    std::wstring __fastcall GetPuttySessionsKey();
    std::wstring __fastcall GetRandomSeedFile() const { return FRandomSeedFile; }
    void __fastcall SetRandomSeedFile(std::wstring value);
    std::wstring __fastcall GetRandomSeedFileName();
    void __fastcall SetPuttyRegistryStorageKey(std::wstring value);
    std::wstring __fastcall GetSshHostKeysSubKey();
    std::wstring __fastcall GetRootKeyStr();
    std::wstring __fastcall GetConfigurationSubKey();
    TEOLType __fastcall GetLocalEOLType();
    bool __fastcall GetLogToFile();
    void __fastcall SetLogWindowLines(int value);
    void __fastcall SetLogWindowComplete(bool value);
    bool __fastcall GetLogWindowComplete();
    void __fastcall SetLogFileAppend(bool value);
    void __fastcall SetLogActions(bool value);
    std::wstring GetActionsLogFileName() const { return FActionsLogFileName; }
    void __fastcall SetActionsLogFileName(std::wstring value);
    std::wstring __fastcall GetDefaultLogFileName();
    void __fastcall SetStorage(TStorage value);
    std::wstring __fastcall GetRegistryStorageKey();
    std::wstring __fastcall GetIniFileStorageName();
    void __fastcall SetIniFileStorageName(std::wstring value);
    std::wstring __fastcall GetPartialExt() const;
    std::wstring __fastcall GetFileInfoString(const std::wstring Key);
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
    std::wstring  __fastcall GetExternalIpAddress() const { return FExternalIpAddress; }
    void __fastcall SetExternalIpAddress(std::wstring value);
    bool __fastcall GetConfirmResume();
    void __fastcall SetConfirmResume(bool value);
    virtual TStorage __fastcall GetStorage();
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
