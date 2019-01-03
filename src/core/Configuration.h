#pragma once

#include <FileBuffer.h>

#include "RemoteFiles.h"
#include "HierarchicalStorage.h"
#include "Usage.h"
//---------------------------------------------------------------------------
#define SET_CONFIG_PROPERTY_EX(PROPERTY, APPLY) \
  if (Get ## PROPERTY() != Value) { F ## PROPERTY = Value; Changed(); APPLY; }
#define SET_CONFIG_PROPERTY(PROPERTY) \
  SET_CONFIG_PROPERTY_EX(PROPERTY, )
//---------------------------------------------------------------------------
#define CONST_DEFAULT_NUMBER_OF_RETRIES 2

extern const wchar_t *AutoSwitchNames;
extern const wchar_t *NotAutoSwitchNames;
enum TAutoSwitch { asOn, asOff, asAuto }; // Has to match PuTTY FORCE_ON, FORCE_OFF, AUTO

enum TFtpEncryptionSwitch_219
{
  fesPlainFTP,
  fesExplicitSSL,
  fesImplicit,
  fesExplicitTLS,
};
//---------------------------------------------------------------------------
class TStoredSessionList;
//---------------------------------------------------------------------------
NB_DEFINE_CLASS_ID(TConfiguration);
class NB_CORE_EXPORT TConfiguration : public TObject
{
  NB_DISABLE_COPY(TConfiguration)
public:
  static inline bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TConfiguration); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TConfiguration) || TObject::is(Kind); }
private:
  bool FDontSave{false};
  bool FForceSave{false};
  bool FChanged{false};
  intptr_t FUpdating{0};
  TNotifyEvent FOnChange;

  mutable void *FApplicationInfo{nullptr};
  std::unique_ptr<TUsage> FUsage;
  bool FLogging{false};
  bool FPermanentLogging{false};
  UnicodeString FLogFileName;
  UnicodeString FPermanentLogFileName;
  intptr_t FLogWindowLines{0};
  bool FLogFileAppend{false};
  bool FLogSensitive{false};
  bool FPermanentLogSensitive{false};
  int64_t FLogMaxSize{0};
  int64_t FPermanentLogMaxSize{0};
  intptr_t FLogMaxCount{0};
  intptr_t FPermanentLogMaxCount{0};
  intptr_t FLogProtocol{0};
  intptr_t FPermanentLogProtocol{0};
  intptr_t FActualLogProtocol{0};
  bool FLogActions{false};
  bool FPermanentLogActions{false};
  bool FLogActionsRequired{false};
  UnicodeString FActionsLogFileName;
  UnicodeString FPermanentActionsLogFileName;
  bool FConfirmOverwriting{false};
  bool FConfirmResume{false};
  bool FAutoReadDirectoryAfterOp{false};
  intptr_t FSessionReopenAuto{0};
  intptr_t FSessionReopenBackground{0};
  intptr_t FSessionReopenTimeout{0};
  intptr_t FSessionReopenAutoStall{0};
  UnicodeString FCustomIniFileStorageName;
  UnicodeString FIniFileStorageName;
  UnicodeString FVirtualIniFileStorageName;
  std::unique_ptr<TStrings> FOptionsStorage;
  intptr_t FProgramIniPathWritable{0};
  intptr_t FTunnelLocalPortNumberLow{0};
  intptr_t FTunnelLocalPortNumberHigh{0};
  intptr_t FCacheDirectoryChangesMaxSize{0};
  bool FShowFtpWelcomeMessage{false};
  UnicodeString FDefaultRandomSeedFile;
  UnicodeString FRandomSeedFile;
  UnicodeString FPuttyRegistryStorageKey;
  UnicodeString FExternalIpAddress;
  bool FTryFtpWhenSshFails{false};
  intptr_t FParallelDurationThreshold;
  bool FScripting{false};
  UnicodeString FMimeTypes;
  intptr_t FDontReloadMoreThanSessions{0};
  intptr_t FScriptProgressFileNameLimit{0};
  intptr_t FSessionReopenAutoMaximumNumberOfRetries{0};

  bool FDisablePasswordStoring{false};
  bool FForceBanners{false};
  bool FDisableAcceptingHostKeys{false};
  bool FDefaultCollectUsage{false};

public:
  TVSFixedFileInfo *GetFixedApplicationInfo() const;
  void *GetApplicationInfo() const;
  virtual UnicodeString GetVersionStr() const;
  virtual UnicodeString GetVersion() const;
  virtual UnicodeString GetProductVersion() const;
  virtual UnicodeString GetProductVersionStr() const;
  UnicodeString GetFileProductVersion() const;
  UnicodeString GetProductName() const;
  UnicodeString GetCompanyName() const;
  UnicodeString GetFileVersion(TVSFixedFileInfo *Info) const;
  UnicodeString GetStoredSessionsSubKey() const;
  UnicodeString GetPuttySessionsKey() const;
  void SetRandomSeedFile(UnicodeString Value);
  UnicodeString GetRandomSeedFileName() const;
  void SetPuttyRegistryStorageKey(UnicodeString Value);
  UnicodeString GetSshHostKeysSubKey() const;
  UnicodeString GetRootKeyStr() const;
  UnicodeString GetConfigurationSubKey() const;
  TEOLType GetLocalEOLType() const;
  void SetLogging(bool Value);
  bool GetLogging() const;
  void SetLogFileName(UnicodeString Value);
  UnicodeString GetLogFileName() const;
  bool GetLogToFile() const;
  void SetLogFileAppend(bool Value);
  void SetLogSensitive(bool Value);
  void SetLogMaxSize(int64_t Value);
  int64_t GetLogMaxSize() const;
  void SetLogMaxCount(intptr_t Value);
  intptr_t GetLogMaxCount() const;
  void SetLogProtocol(intptr_t Value);
  void SetLogActions(bool Value);
  bool GetLogActions() const;
  void SetActionsLogFileName(UnicodeString Value);
  UnicodeString GetPermanentActionsLogFileName() const;
  UnicodeString GetActionsLogFileName() const;
  UnicodeString GetDefaultLogFileName() const;
  UnicodeString GetTimeFormat() const;
  UnicodeString GetRegistryStorageKey() const;
#if 0
  UnicodeString GetIniFileStorageNameForReadingWriting() const;
  UnicodeString GetIniFileStorageNameForReading();
  UnicodeString GetIniFileStorageName(bool ReadingOnly);
#endif // #if 0
  void SetIniFileStorageName(UnicodeString Value);
  void SetOptionsStorage(TStrings *Value);
  TStrings *GetOptionsStorage();
  UnicodeString GetPartialExt() const;
  UnicodeString GetFileInfoString(const UnicodeString Key) const;
  void SetSessionReopenAuto(intptr_t Value);
  void SetSessionReopenBackground(intptr_t Value);
  void SetSessionReopenTimeout(intptr_t Value);
  void SetSessionReopenAutoStall(intptr_t Value);
  void SetTunnelLocalPortNumberLow(intptr_t Value);
  void SetTunnelLocalPortNumberHigh(intptr_t Value);
  void SetCacheDirectoryChangesMaxSize(intptr_t Value);
  void SetShowFtpWelcomeMessage(bool Value);
  intptr_t GetCompoundVersion() const;
  void UpdateActualLogProtocol();
  void SetExternalIpAddress(UnicodeString Value);
  void SetTryFtpWhenSshFails(bool Value);
  void SetParallelDurationThreshold(intptr_t Value);
  void SetMimeTypes(UnicodeString Value);
  bool GetCollectUsage() const;
  void SetCollectUsage(bool Value);
  bool GetIsUnofficial() const;
  bool GetPersistent() const;

  bool GetScripting() const { return FScripting; }
  void SetScripting(bool Value) { FScripting = Value; }
  void SetStorage(TStorage Value);

protected:
  mutable TStorage FStorage{};
  mutable TCriticalSection FCriticalSection;

public:
  virtual TStorage GetStorage() const;
  virtual void Changed() override;
  virtual void SaveData(THierarchicalStorage *Storage, bool All);
  virtual void LoadData(THierarchicalStorage *Storage);
  virtual void LoadFrom(THierarchicalStorage *Storage);
  virtual void CopyData(THierarchicalStorage *Source, THierarchicalStorage *Target);
  virtual void LoadAdmin(THierarchicalStorage *Storage);
  virtual UnicodeString GetDefaultKeyFile() const;
  virtual void Saved();
  void CleanupRegistry(const UnicodeString CleanupSubKey);
  UnicodeString BannerHash(const UnicodeString Banner) const;
  void SetBannerData(const UnicodeString ASessionKey, const UnicodeString ABannerHash, uintptr_t AParams);
  void GetBannerData(const UnicodeString ASessionKey, UnicodeString &ABannerHash, uintptr_t &AParams);
  static UnicodeString PropertyToKey(const UnicodeString Property);
  virtual void DoSave(bool All, bool Explicit);
  UnicodeString FormatFingerprintKey(const UnicodeString ASiteKey, const UnicodeString AFingerprintType) const;

  virtual bool GetConfirmOverwriting() const;
  virtual void SetConfirmOverwriting(bool Value);
  bool GetConfirmResume() const;
  void SetConfirmResume(bool Value);
  bool GetAutoReadDirectoryAfterOp() const;
  void SetAutoReadDirectoryAfterOp(bool Value);
  virtual bool GetRememberPassword() const;
  UnicodeString GetReleaseType() const;
  UnicodeString LoadCustomIniFileStorageName();
  void SaveCustomIniFileStorageName();
  UnicodeString GetRegistryStorageOverrideKey() const;

  virtual UnicodeString ModuleFileName() const;

  UnicodeString GetFileFileInfoString(const UnicodeString AKey,
    const UnicodeString AFileName, bool AllowEmpty = false) const;
  void *GetFileApplicationInfo(const UnicodeString AFileName) const;
  UnicodeString GetFileProductVersion(const UnicodeString AFileName) const;
  UnicodeString GetFileProductName(const UnicodeString AFileName) const;
  UnicodeString GetFileCompanyName(const UnicodeString AFileName) const;

  __property bool PermanentLogging  = { read = GetLogging, write = SetLogging };
  __property UnicodeString PermanentLogFileName  = { read = GetLogFileName, write = SetLogFileName };
  __property bool PermanentLogActions  = { read = GetLogActions, write = SetLogActions };
  __property UnicodeString PermanentActionsLogFileName  = { read = GetPermanentActionsLogFileName, write = SetActionsLogFileName };
  __property int PermanentLogProtocol  = { read = FPermanentLogProtocol, write = SetLogProtocol };
  __property bool PermanentLogSensitive  = { read = FPermanentLogSensitive, write = SetLogSensitive };
  __property int64_t PermanentLogMaxSize  = { read = GetLogMaxSize, write = SetLogMaxSize };
  __property int PermanentLogMaxCount  = { read = GetLogMaxCount, write = SetLogMaxCount };

public:
  explicit TConfiguration(TObjectClassId Kind = OBJECT_CLASS_TConfiguration) noexcept;
  virtual ~TConfiguration() noexcept;
  virtual void ConfigurationInit();
  virtual void Default();
  virtual void UpdateStaticUsage();
  void Load(THierarchicalStorage *Storage);
  void Save();
  void SaveExplicit();
  void MoveStorage(TStorage AStorage, const UnicodeString ACustomIniFileStorageName);
  void ScheduleCustomIniFileStorageUse(const UnicodeString ACustomIniFileStorageName);
  void SetNulStorage();
  void SetDefaultStorage();
  UnicodeString GetAutomaticIniFileStorageName(bool ReadingOnly);
  UnicodeString GetDefaultIniFileExportPath();
  void Export(const UnicodeString AFileName);
  void Import(const UnicodeString AFileName);
  void CleanupConfiguration();
  void CleanupIniFile();
  void CleanupHostKeys();
  void CleanupRandomSeedFile();
  void BeginUpdate();
  void EndUpdate();
  void DontSave();
  void LoadDirectoryChangesCache(const UnicodeString SessionKey,
    TRemoteDirectoryChangesCache *DirectoryChangesCache);
  void SaveDirectoryChangesCache(const UnicodeString SessionKey,
    TRemoteDirectoryChangesCache *DirectoryChangesCache);
  bool ShowBanner(const UnicodeString ASessionKey, const UnicodeString ABanner, uintptr_t &AParams);
  void NeverShowBanner(const UnicodeString ASessionKey, const UnicodeString ABanner);
  void SetBannerParams(const UnicodeString ASessionKey, uintptr_t AParams);
  void RememberLastFingerprint(const UnicodeString ASiteKey, const UnicodeString AFingerprintType, const UnicodeString AFingerprint);
  UnicodeString GetLastFingerprint(const UnicodeString SiteKey, const UnicodeString FingerprintType);
  virtual THierarchicalStorage *CreateConfigStorage();
  virtual THierarchicalStorage *CreateStorage(bool &SessionList);
  void TemporaryLogging(const UnicodeString ALogFileName);
  void TemporaryActionsLogging(const UnicodeString ALogFileName);
  void TemporaryLogProtocol(intptr_t ALogProtocol);
  void TemporaryLogSensitive(bool ALogSensitive);
  void TemporaryLogMaxSize(int64_t ALogMaxSize);
  void TemporaryLogMaxCount(intptr_t ALogMaxCount);
  virtual RawByteString EncryptPassword(const UnicodeString Password, const UnicodeString Key);
  virtual UnicodeString DecryptPassword(const RawByteString Password, const UnicodeString Key);
  virtual RawByteString StronglyRecryptPassword(const RawByteString Password, const UnicodeString Key);
  UnicodeString GetFileDescription(const UnicodeString AFileName) const;
  UnicodeString GetFileVersion(const UnicodeString AFileName) const;
  UnicodeString GetFileMimeType(const UnicodeString AFileName) const;

  TStoredSessionList *SelectFilezillaSessionsForImport(
    TStoredSessionList *Sessions, UnicodeString &Error);
  bool AnyFilezillaSessionForImport(TStoredSessionList *Sessions);
  TStoredSessionList *SelectKnownHostsSessionsForImport(
    TStoredSessionList *Sessions, UnicodeString &Error);
  TStoredSessionList *SelectKnownHostsSessionsForImport(
    TStrings *Lines, TStoredSessionList *Sessions, UnicodeString &Error);

  __property TVSFixedFileInfo *FixedApplicationInfo  = { read = GetFixedApplicationInfo };
  __property void *ApplicationInfo  = { read = GetApplicationInfo };
  ROProperty<void *> ApplicationInfo{nb::bind(&TConfiguration::GetApplicationInfo, this)};
  __property TUsage *Usage = { read = FUsage };
  ROProperty<TUsage*> Usage{nb::bind(&TConfiguration::GetUsage, this)};
  __property bool CollectUsage = { read = GetCollectUsage, write = SetCollectUsage };
  RWProperty<bool> CollectUsage{nb::bind(&TConfiguration::GetCollectUsage, this), nb::bind(&TConfiguration::SetCollectUsage, this)};
  __property UnicodeString StoredSessionsSubKey = {read = GetStoredSessionsSubKey};
  ROProperty<UnicodeString> StoredSessionsSubKey{nb::bind(&TConfiguration::GetStoredSessionsSubKey, this)};
  __property UnicodeString PuttyRegistryStorageKey  = { read = FPuttyRegistryStorageKey, write = SetPuttyRegistryStorageKey };
  RWProperty<UnicodeString> PuttyRegistryStorageKey{nb::bind(&TConfiguration::GetPuttyRegistryStorageKey, this), nb::bind(&TConfiguration::SetPuttyRegistryStorageKey, this)};
  __property UnicodeString PuttySessionsKey  = { read = GetPuttySessionsKey };
  ROProperty<UnicodeString> PuttySessionsKey{nb::bind(&TConfiguration::GetPuttySessionsKey, this)};
  __property UnicodeString RandomSeedFile  = { read = FRandomSeedFile, write = SetRandomSeedFile };
  __property UnicodeString RandomSeedFileName  = { read = GetRandomSeedFileName };
  __property UnicodeString SshHostKeysSubKey  = { read = GetSshHostKeysSubKey };
  ROProperty<UnicodeString> SshHostKeysSubKey{nb::bind(&TConfiguration::GetSshHostKeysSubKey, this)};
  __property UnicodeString RootKeyStr  = { read = GetRootKeyStr };
  ROProperty<UnicodeString> RootKeyStr{nb::bind(&TConfiguration::GetRootKeyStr, this)};
  __property UnicodeString ConfigurationSubKey  = { read = GetConfigurationSubKey };
  ROProperty<UnicodeString> ConfigurationSubKey{nb::bind(&TConfiguration::GetConfigurationSubKey, this)};
  __property TEOLType LocalEOLType = { read = GetLocalEOLType };
  ROProperty<TEOLType> LocalEOLType{nb::bind(&TConfiguration::GetLocalEOLType, this)};
  __property UnicodeString VersionStr = { read = GetVersionStr };
  ROProperty<UnicodeString> VersionStr{nb::bind(&TConfiguration::GetVersionStr, this)};
  __property UnicodeString Version = { read = GetVersion };
  __property int CompoundVersion = { read = GetCompoundVersion };
  ROProperty<intptr_t> CompoundVersion{nb::bind(&TConfiguration::GetCompoundVersion, this)};
  __property UnicodeString ProductVersion = { read = GetProductVersion };
  ROProperty<UnicodeString> ProductVersion{nb::bind(&TConfiguration::GetProductVersionStr, this)};
  __property UnicodeString ProductName = { read = GetProductName };
  __property UnicodeString CompanyName = { read = GetCompanyName };
  __property bool IsUnofficial = { read = GetIsUnofficial };
  __property bool Logging  = { read = FLogging, write = SetLogging };
  RWProperty<bool> Logging{nb::bind(&TConfiguration::GetLogging, this), nb::bind(&TConfiguration::SetLogging, this)};
  __property UnicodeString LogFileName  = { read = FLogFileName, write = SetLogFileName };
  __property bool LogToFile  = { read = GetLogToFile };
  ROProperty<bool> LogToFile{nb::bind(&TConfiguration::GetLogToFile, this)};
  __property bool LogFileAppend  = { read = FLogFileAppend, write = SetLogFileAppend };
  __property bool LogSensitive  = { read = FLogSensitive, write = SetLogSensitive };
  __property int64_t LogMaxSize  = { read = FLogMaxSize, write = SetLogMaxSize };
  __property int LogMaxCount  = { read = FLogMaxCount, write = SetLogMaxCount };
  __property int LogProtocol  = { read = FLogProtocol, write = SetLogProtocol };
  RWProperty<intptr_t> LogProtocol{nb::bind(&TConfiguration::GetLogProtocol, this), nb::bind(&TConfiguration::SetLogProtocol, this)};
  __property int ActualLogProtocol  = { read = FActualLogProtocol };
  ROProperty<intptr_t> ActualLogProtocol{nb::bind(&TConfiguration::GetActualLogProtocol, this)};
  __property bool LogActions  = { read = FLogActions, write = SetLogActions };
  __property bool LogActionsRequired  = { read = FLogActionsRequired, write = FLogActionsRequired };
  __property UnicodeString ActionsLogFileName  = { read = GetActionsLogFileName, write = SetActionsLogFileName };
  __property UnicodeString DefaultLogFileName  = { read = GetDefaultLogFileName };
  __property TNotifyEvent OnChange = { read = FOnChange, write = FOnChange };
  __property bool ConfirmOverwriting = { read = GetConfirmOverwriting, write = SetConfirmOverwriting};
  __property bool ConfirmResume = { read = GetConfirmResume, write = SetConfirmResume};
  __property bool AutoReadDirectoryAfterOp = { read = GetAutoReadDirectoryAfterOp, write = SetAutoReadDirectoryAfterOp};
  __property bool RememberPassword = { read = GetRememberPassword };
  __property UnicodeString PartialExt = {read = GetPartialExt};
  __property int SessionReopenAuto = { read = FSessionReopenAuto, write = SetSessionReopenAuto };
  __property int SessionReopenBackground = { read = FSessionReopenBackground, write = SetSessionReopenBackground };
  __property int SessionReopenTimeout = { read = FSessionReopenTimeout, write = SetSessionReopenTimeout };
  __property int SessionReopenAutoStall = { read = FSessionReopenAutoStall, write = SetSessionReopenAutoStall };
  __property int TunnelLocalPortNumberLow = { read = FTunnelLocalPortNumberLow, write = SetTunnelLocalPortNumberLow };
  __property int TunnelLocalPortNumberHigh = { read = FTunnelLocalPortNumberHigh, write = SetTunnelLocalPortNumberHigh };
  __property int CacheDirectoryChangesMaxSize = { read = FCacheDirectoryChangesMaxSize, write = SetCacheDirectoryChangesMaxSize };
  __property bool ShowFtpWelcomeMessage = { read = FShowFtpWelcomeMessage, write = SetShowFtpWelcomeMessage };
  __property UnicodeString ExternalIpAddress = { read = FExternalIpAddress, write = SetExternalIpAddress };
  __property bool TryFtpWhenSshFails = { read = FTryFtpWhenSshFails, write = SetTryFtpWhenSshFails };
  __property int ParallelDurationThreshold = { read = FParallelDurationThreshold, write = SetParallelDurationThreshold };
  __property UnicodeString MimeTypes = { read = FMimeTypes, write = SetMimeTypes };
  __property int DontReloadMoreThanSessions = { read = FDontReloadMoreThanSessions, write = FDontReloadMoreThanSessions };
  intptr_t& DontReloadMoreThanSessions{FDontReloadMoreThanSessions};
  __property int ScriptProgressFileNameLimit = { read = FScriptProgressFileNameLimit, write = FScriptProgressFileNameLimit };
  intptr_t& ScriptProgressFileNameLimit{FScriptProgressFileNameLimit};

  __property UnicodeString TimeFormat = { read = GetTimeFormat };
  ROProperty<UnicodeString> TimeFormat{nb::bind(&TConfiguration::GetTimeFormat, this)};
  __property TStorage Storage  = { read = GetStorage };
  ROProperty<TStorage> Storage{nb::bind(&TConfiguration::GetStorage, this)};
  __property UnicodeString RegistryStorageKey  = { read = GetRegistryStorageKey };
  ROProperty<UnicodeString> RegistryStorageKey{nb::bind(&TConfiguration::GetRegistryStorageKey, this)};
  __property UnicodeString CustomIniFileStorageName  = { read = FCustomIniFileStorageName };
  __property UnicodeString IniFileStorageName  = { read = GetIniFileStorageNameForReadingWriting, write = SetIniFileStorageName };
  __property UnicodeString IniFileStorageNameForReading  = { read = GetIniFileStorageNameForReading };
  __property TStrings *OptionsStorage = { read = GetOptionsStorage, write = SetOptionsStorage };
  RWProperty<TStrings*> OptionsStorage{nb::bind(&TConfiguration::GetOptionsStorage, this), nb::bind(&TConfiguration::SetOptionsStorage, this)};
  __property bool Persistent = { read = GetPersistent };
  ROProperty<bool> Persistent{nb::bind(&TConfiguration::GetPersistent, this)};
  __property bool ForceSave = { read = FForceSave, write = FForceSave };
  bool& ForceSave{FForceSave};
  __property bool Scripting = { read = FScripting, write = FScripting };
  bool& Scripting{FScripting};

  __property UnicodeString DefaultKeyFile = { read = GetDefaultKeyFile };
  ROProperty<UnicodeString> DefaultKeyFile{nb::bind(&TConfiguration::GetDefaultKeyFile, this)};

  __property bool DisablePasswordStoring = { read = FDisablePasswordStoring };
  const bool& DisablePasswordStoring{FDisablePasswordStoring};
  __property bool ForceBanners = { read = FForceBanners };
  const bool& ForceBanners{FForceBanners};
  __property bool DisableAcceptingHostKeys = { read = FDisableAcceptingHostKeys };
  const bool& DisableAcceptingHostKeys{FDisableAcceptingHostKeys};

  TUsage * GetUsage() { return FUsage.get(); }
  UnicodeString GetPuttyRegistryStorageKey() const { return FPuttyRegistryStorageKey; }
  UnicodeString GetRandomSeedFile() const { return FRandomSeedFile; }
  bool GetLogFileAppend() const { return FLogFileAppend; }
  bool GetLogSensitive() const { return FLogSensitive; }
  intptr_t GetLogProtocol() const { return FLogProtocol; }
  intptr_t GetActualLogProtocol() const { return FActualLogProtocol; }
  bool GetLogActionsRequired() const { return FLogActionsRequired; }
  intptr_t GetLogWindowLines() const { return FLogWindowLines; }
  TNotifyEvent GetOnChange() const { return FOnChange; }
  void SetOnChange(TNotifyEvent Value) { FOnChange = Value; }
  intptr_t GetSessionReopenAuto() const { return FSessionReopenAuto; }
  intptr_t GetSessionReopenBackground() const { return FSessionReopenBackground; }
  intptr_t GetSessionReopenTimeout() const { return FSessionReopenTimeout; }
  intptr_t GetSessionReopenAutoStall() const { return FSessionReopenAutoStall; }
  intptr_t GetTunnelLocalPortNumberLow() const { return FTunnelLocalPortNumberLow; }
  intptr_t GetTunnelLocalPortNumberHigh() const { return FTunnelLocalPortNumberHigh; }
  intptr_t GetCacheDirectoryChangesMaxSize() const { return FCacheDirectoryChangesMaxSize; }
  bool GetShowFtpWelcomeMessage() const { return FShowFtpWelcomeMessage; }
  UnicodeString GetExternalIpAddress() const { return FExternalIpAddress; }
  bool GetTryFtpWhenSshFails() const { return FTryFtpWhenSshFails; }
  intptr_t GetParallelDurationThreshold() const { return FParallelDurationThreshold; }
  UnicodeString GetMimeTypes() const { return FMimeTypes; }
  bool GetDisablePasswordStoring() const { return FDisablePasswordStoring; }
  bool GetForceBanners() const { return FForceBanners; }
  bool GetDisableAcceptingHostKeys() const { return FDisableAcceptingHostKeys; }
  intptr_t GetSessionReopenAutoMaximumNumberOfRetries() const { return FSessionReopenAutoMaximumNumberOfRetries; }
  void SetSessionReopenAutoMaximumNumberOfRetries(intptr_t Value);
};
//---------------------------------------------------------------------------
class NB_CORE_EXPORT TShortCuts : public TObject
{
public:
  void Add(const TShortCut &ShortCut);
  bool Has(const TShortCut &ShortCut) const;

private:
  rde::vector<TShortCut> FShortCuts;
};
//---------------------------------------------------------------------------
NB_CORE_EXPORT extern const UnicodeString OriginalPuttyRegistryStorageKey;
NB_CORE_EXPORT extern const UnicodeString KittyRegistryStorageKey;
NB_CORE_EXPORT extern const UnicodeString OriginalPuttyExecutable;
NB_CORE_EXPORT extern const UnicodeString KittyExecutable;
//---------------------------------------------------------------------------
NB_CORE_EXPORT extern const UnicodeString Sha1ChecksumAlg;
NB_CORE_EXPORT extern const UnicodeString Sha224ChecksumAlg;
NB_CORE_EXPORT extern const UnicodeString Sha256ChecksumAlg;
NB_CORE_EXPORT extern const UnicodeString Sha384ChecksumAlg;
NB_CORE_EXPORT extern const UnicodeString Sha512ChecksumAlg;
NB_CORE_EXPORT extern const UnicodeString Md5ChecksumAlg;
NB_CORE_EXPORT extern const UnicodeString Crc32ChecksumAlg;
//---------------------------------------------------------------------------
NB_CORE_EXPORT extern const UnicodeString SshFingerprintType;
NB_CORE_EXPORT extern const UnicodeString TlsFingerprintType;
//---------------------------------------------------------------------------
NB_CORE_EXPORT extern const UnicodeString HttpsCertificateStorageKey;
//---------------------------------------------------------------------------
