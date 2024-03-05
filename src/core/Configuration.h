#pragma once

// #include <set>
// #include <list>
#include "RemoteFiles.h"
#include "FileBuffer.h"
#include "HierarchicalStorage.h"
#include "Usage.h"

#define SET_CONFIG_PROPERTY_EX(PROPERTY, APPLY) \
  do { if (Get ## PROPERTY() != Value) { F ## PROPERTY = Value; Changed(); APPLY; } } while(0)
#define SET_CONFIG_PROPERTY_EX2(PROPERTY, APPLY) \
  do { if (F ## PROPERTY != Value) { F ## PROPERTY = Value; Changed(); APPLY; } } while(0)
#define SET_CONFIG_PROPERTY(PROPERTY) \
  SET_CONFIG_PROPERTY_EX(PROPERTY, )
#define SET_CONFIG_PROPERTY2(PROPERTY) \
  SET_CONFIG_PROPERTY_EX2(PROPERTY, )

constexpr const int32_t CONST_DEFAULT_NUMBER_OF_RETRIES = 2;

constexpr const wchar_t * AutoSwitchNames = L"On;Off;Auto";
constexpr const wchar_t * NotAutoSwitchNames = L"Off;On;Auto";
enum TAutoSwitch { asOn, asOff, asAuto }; // Has to match PuTTY FORCE_ON, FORCE_OFF, AUTO

class TStoredSessionList;
class TCopyParamType;

class TSshHostCA final
{
public:
  explicit TSshHostCA() noexcept;
  void Save(THierarchicalStorage * Storage) const;
  bool Load(THierarchicalStorage * Storage);

  UnicodeString Name;
  RawByteString PublicKey;
  UnicodeString ValidityExpression;
  bool PermitRsaSha1{false};
  bool PermitRsaSha256{false};
  bool PermitRsaSha512{false};

  using TList = nb::vector_t<TSshHostCA>;
};

class TSshHostCAList final
{
public:
  TSshHostCAList() noexcept = default;
  TSshHostCAList(const TSshHostCAList & Other) { operator =(Other); }
  explicit TSshHostCAList(const TSshHostCA::TList & List);
  TSshHostCAList & operator =(const TSshHostCAList & Other);
  void Default();
  const TSshHostCA::TList & GetList() const;
  int32_t GetCount() const;
  const TSshHostCA * Get(int32_t Index) const;
  const TSshHostCA * Find(const UnicodeString & Name) const;

  void Save(THierarchicalStorage * Storage);
  void Load(THierarchicalStorage * Storage);

private:
  TSshHostCA::TList FList;
};

class TTerminal;
class TSessionData;
class TSessionLog;
class TFTPFileSystem;
class TSTSFTPFileSystemFTPFileSystem;

class NB_CORE_EXPORT TConfiguration : public TObject
{
  NB_DISABLE_COPY(TConfiguration)
  friend class TTerminal;
  friend class TSessionData;
  friend class TSessionLog;
  friend class TFTPFileSystem;
  friend class TSFTPFileSystem;
public:
  static bool classof(const TObject * Obj) { return Obj->is(OBJECT_CLASS_TConfiguration); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TConfiguration) || TObject::is(Kind); }
private:
  bool FDontSave{false};
  bool FForceSave{false};
  bool FChanged{false};
  int32_t FUpdating{0};
  TNotifyEvent FOnChange;

  mutable void * FApplicationInfo{nullptr};
  std::unique_ptr<TUsage> FUsage;
  bool FLogging{false};
  bool FPermanentLogging{false};
  UnicodeString FLogFileName;
  UnicodeString FPermanentLogFileName;
  int32_t FLogWindowLines{0};
  bool FLogFileAppend{false};
  bool FLogSensitive{false};
  bool FPermanentLogSensitive{false};
  int64_t FLogMaxSize{0};
  int64_t FPermanentLogMaxSize{0};
  int32_t FLogMaxCount{0};
  int32_t FPermanentLogMaxCount{0};
  int32_t FLogProtocol{0};
  int32_t FPermanentLogProtocol{0};
  int32_t FActualLogProtocol{0};
  bool FLogActions{false};
  bool FPermanentLogActions{false};
  bool FLogActionsRequired{false};
  UnicodeString FActionsLogFileName;
  UnicodeString FPermanentActionsLogFileName;
  bool FConfirmOverwriting{false};
  bool FConfirmResume{false};
  bool FAutoReadDirectoryAfterOp{false};
  int32_t FSessionReopenAuto{0};
  int32_t FSessionReopenBackground{0};
  int32_t FSessionReopenTimeout{0};
  int32_t FSessionReopenAutoStall{10 * MSecsPerSec};
  UnicodeString FCustomIniFileStorageName;
  UnicodeString FIniFileStorageName;
  mutable UnicodeString FVirtualIniFileStorageName;
  std::unique_ptr<TStrings> FOptionsStorage;
  mutable int32_t FProgramIniPathWritable{0};
  int32_t FTunnelLocalPortNumberLow{0};
  int32_t FTunnelLocalPortNumberHigh{0};
  int32_t FCacheDirectoryChangesMaxSize{0};
  bool FShowFtpWelcomeMessage{false};
  UnicodeString FDefaultRandomSeedFile;
  UnicodeString FRandomSeedFile;
  UnicodeString FPuttyRegistryStorageKey;
  UnicodeString FExternalIpAddress;
  int32_t FLocalPortNumberMin{0};
  int32_t FLocalPortNumberMax{0};
  bool FTryFtpWhenSshFails{false};
  int32_t FParallelDurationThreshold{0};
  bool FScripting{false};
  UnicodeString FMimeTypes;
  int32_t FDontReloadMoreThanSessions{0};
  int32_t FScriptProgressFileNameLimit{0};
  int32_t FSessionReopenAutoMaximumNumberOfRetries{0};
  int32_t FKeyVersion{0};
  int32_t FQueueTransfersLimit{0};
  int32_t FParallelTransferThreshold{0};
  UnicodeString FCertificateStorage;
  UnicodeString FAWSMetadataService;
  UnicodeString FChecksumCommands;
  std::unique_ptr<TSshHostCAList> FSshHostCAList;
  std::unique_ptr<TSshHostCAList> FPuttySshHostCAList;
  bool FSshHostCAsFromPuTTY{false};
  int32_t FHttpsCertificateValidation{0};
  UnicodeString FSynchronizationChecksumAlgs;

  bool FDisablePasswordStoring{false};
  bool FForceBanners{false};
  bool FDisableAcceptingHostKeys{false};
  bool FDefaultCollectUsage{false};

public:
  TVSFixedFileInfo * GetFixedApplicationInfo() const;
  void * GetApplicationInfo() const;
  virtual UnicodeString GetVersionStr() const;
  virtual UnicodeString GetVersion() const;
  UnicodeString GetFileProductVersion() const;
  UnicodeString GetProductName() const;
  UnicodeString GetCompanyName() const;
  UnicodeString GetFileVersion(const TVSFixedFileInfo * Info) const;
  UnicodeString GetStoredSessionsSubKey() const;
  UnicodeString DoGetPuttySessionsKey() const;
  UnicodeString GetPuttySessionsSubKey() const;
  void SetRandomSeedFile(const UnicodeString & Value);
  UnicodeString GetRandomSeedFileName() const;
  void SetPuttyRegistryStorageKey(const UnicodeString & Value);
  UnicodeString GetSshHostKeysSubKey() const;
  UnicodeString GetRootKeyStr() const;
  UnicodeString GetConfigurationSubKey() const;
  TEOLType GetLocalEOLType() const;
  void SetLogging(bool Value);
  bool GetLogging() const;
  void SetLogFileName(const UnicodeString & Value);
  UnicodeString GetLogFileName() const;
  bool GetLogToFile() const;
  void SetLogFileAppend(bool Value);
  void SetLogSensitive(bool Value);
  void SetLogMaxSize(int64_t Value);
  int64_t GetLogMaxSize() const;
  void SetLogMaxCount(int32_t Value);
  int32_t GetLogMaxCount() const;
  void SetLogProtocol(int32_t Value);
  void SetLogActions(bool Value);
  bool GetLogActions() const;
  void SetActionsLogFileName(const UnicodeString & Value);
  UnicodeString GetPermanentActionsLogFileName() const;
  UnicodeString GetActionsLogFileName() const;
  UnicodeString GetDefaultLogFileName() const;
  UnicodeString GetConfigurationTimeFormat() const;
  UnicodeString GetRegistryStorageKey() const;
  UnicodeString GetIniFileStorageNameForReadingWriting() const;
  UnicodeString GetIniFileStorageNameForReading();
  UnicodeString GetIniFileStorageName(bool ReadingOnly) const;
  void SetOptionsStorage(const TStrings * Value);
  const TStrings * GetOptionsStorage() const;
  UnicodeString GetFileInfoString(const UnicodeString & Key) const;
  void SetSessionReopenAuto(int32_t Value);
  void SetSessionReopenBackground(int32_t Value);
  void SetSessionReopenTimeout(int32_t Value);
  void SetSessionReopenAutoStall(int32_t Value);
  void SetTunnelLocalPortNumberLow(int32_t Value);
  void SetTunnelLocalPortNumberHigh(int32_t Value);
  void SetCacheDirectoryChangesMaxSize(int32_t Value);
  void SetShowFtpWelcomeMessage(bool Value);
  int32_t GetCompoundVersion() const;
  void UpdateActualLogProtocol();
  void SetExternalIpAddress(const UnicodeString & Value);
  void SetTryFtpWhenSshFails(bool Value);
  void SetParallelDurationThreshold(int32_t Value);
  void SetMimeTypes(const UnicodeString & Value);
  void SetCertificateStorage(const UnicodeString & value);
  UnicodeString GetCertificateStorageExpanded() const;
  void SetAWSMetadataService(const UnicodeString & Value);
  bool GetCollectUsage() const;
  void SetCollectUsage(bool Value);
  bool GetIsUnofficial() const;
  bool GetPersistent() const;
  void SetLocalPortNumberMin(int32_t Value);
  void SetLocalPortNumberMax(int32_t Value);
  void SetQueueTransfersLimit(int32_t Value);
  const TSshHostCAList * GetSshHostCAList() const;
  void SetSshHostCAList(const TSshHostCAList * Value);
  const TSshHostCAList * GetPuttySshHostCAList();
  const TSshHostCAList * GetActiveSshHostCAList();

  virtual UnicodeString GetProductVersion() const;
  virtual UnicodeString GetProductVersionStr() const;
protected:
  mutable TStorage FStorage{stDetect};
  mutable TCriticalSection FCriticalSection;

public:
  virtual TStorage GetStorage() const;
  virtual void Changed() override;
  virtual void SaveData(THierarchicalStorage * AStorage, bool All);
  virtual void LoadData(THierarchicalStorage * AStorage);
  virtual void LoadFrom(THierarchicalStorage * AStorage);
  virtual void CopyData(THierarchicalStorage * Source, THierarchicalStorage * Target);
  virtual void LoadAdmin(THierarchicalStorage * Storage);
  void LoadSshHostCAList(TSshHostCAList * SshHostCAList, THierarchicalStorage * AStorage);
  virtual UnicodeString GetDefaultKeyFile() const;
  virtual void Saved();
  void CleanupRegistry(const UnicodeString & RegistryPath);
  void CopyAllStringsInSubKey(
    THierarchicalStorage * Source, THierarchicalStorage * Target, const UnicodeString & Name);
  bool CopySubKey(THierarchicalStorage * Source, THierarchicalStorage * Target, const UnicodeString & Name);
  UnicodeString BannerHash(const UnicodeString & Banner) const;
  void SetBannerData(const UnicodeString & ASessionKey, const UnicodeString & ABannerHash, uint32_t AParams);
  void GetBannerData(const UnicodeString & ASessionKey, UnicodeString & ABannerHash, uint32_t & AParams);
  static UnicodeString PropertyToKey(const UnicodeString & Property);
  void DoSave(THierarchicalStorage * AStorage, bool All);
  virtual void DoSave(bool All, bool Explicit);
  UnicodeString FormatFingerprintKey(const UnicodeString & ASiteKey, const UnicodeString & AFingerprintType) const;
  THierarchicalStorage * OpenDirectoryStatisticsCache(bool CanCreate);
  UnicodeString GetDirectoryStatisticsCacheKey(
    const UnicodeString & SessionKey, const UnicodeString & Path, const TCopyParamType & CopyParam);

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
  TStrings * GetCaches() const;
  UnicodeString GetFullVersion() const;

  virtual UnicodeString ModuleFileName() const;

  UnicodeString GetFileFileInfoString(const UnicodeString & AKey,
    const UnicodeString & AFileName, bool AllowEmpty = false) const;
  void * GetFileApplicationInfo(const UnicodeString & AFileName) const;
  UnicodeString GetFileProductVersion(const UnicodeString & AFileName) const;
  UnicodeString GetFileProductName(const UnicodeString & AFileName) const;
  UnicodeString GetFileCompanyName(const UnicodeString & AFileName) const;

  __property bool PermanentLogging  = { read=GetLogging, write=SetLogging };
  __property UnicodeString PermanentLogFileName  = { read=GetLogFileName, write=SetLogFileName };
  __property bool PermanentLogActions  = { read=GetLogActions, write=SetLogActions };
  __property UnicodeString PermanentActionsLogFileName  = { read=GetPermanentActionsLogFileName, write=SetActionsLogFileName };
  __property int32_t PermanentLogProtocol  = { read=FPermanentLogProtocol, write=SetLogProtocol };
  __property bool PermanentLogSensitive  = { read=FPermanentLogSensitive, write=SetLogSensitive };
  __property int64_t PermanentLogMaxSize  = { read=GetLogMaxSize, write=SetLogMaxSize };
  __property int32_t PermanentLogMaxCount  = { read=GetLogMaxCount, write=SetLogMaxCount };

public:
  TConfiguration() = delete;
  explicit TConfiguration(TObjectClassId Kind = OBJECT_CLASS_TConfiguration) noexcept;
  virtual ~TConfiguration() noexcept;
  virtual void ConfigurationInit();
  virtual void Default();
  virtual void UpdateStaticUsage();
  void Load(THierarchicalStorage * Storage);
  void Save();
  void SaveExplicit();
  void MoveStorage(TStorage AStorage, const UnicodeString & ACustomIniFileStorageName);
  void ScheduleCustomIniFileStorageUse(const UnicodeString & ACustomIniFileStorageName);
  void SetExplicitIniFileStorageName(const UnicodeString & FileName);
  void SetNulStorage();
  UnicodeString GetAutomaticIniFileStorageName(bool ReadingOnly) const;
  UnicodeString GetDefaultIniFileExportPath() const;
  UnicodeString GetIniFileParamValue() const;
  void Export(const UnicodeString & AFileName);
  void Import(const UnicodeString & AFileName);
  void CleanupConfiguration();
  void CleanupIniFile();
  void CleanupCaches();
  bool HasAnyCache() const;
  void CleanupRandomSeedFile();
  void BeginUpdate();
  void EndUpdate();
  void DontSave();
  void LoadDirectoryChangesCache(const UnicodeString & SessionKey,
    TRemoteDirectoryChangesCache * DirectoryChangesCache);
  void SaveDirectoryChangesCache(const UnicodeString & SessionKey,
    const TRemoteDirectoryChangesCache * DirectoryChangesCache);
  TStrings * LoadDirectoryStatisticsCache(
    const UnicodeString & SessionKey, const UnicodeString & Path, const TCopyParamType & CopyParam);
  void SaveDirectoryStatisticsCache(
    const UnicodeString & SessionKey, const UnicodeString & Path, const TCopyParamType & CopyParam, const TStrings * DataList);
  bool ShowBanner(const UnicodeString & ASessionKey, const UnicodeString & ABanner, uint32_t & AParams);
  void NeverShowBanner(const UnicodeString & ASessionKey, const UnicodeString & ABanner);
  void SetBannerParams(const UnicodeString & ASessionKey, uint32_t AParams);
  void RememberLastFingerprint(const UnicodeString & ASiteKey, const UnicodeString & AFingerprintType, const UnicodeString & AFingerprint);
  UnicodeString GetLastFingerprint(const UnicodeString & SiteKey, const UnicodeString & FingerprintType);
  THierarchicalStorage * CreateConfigStorage();
  THierarchicalStorage * CreateConfigRegistryStorage() const;
  virtual THierarchicalStorage * CreateScpStorage(bool & SessionList);
  void TemporaryLogging(const UnicodeString & ALogFileName);
  void TemporaryActionsLogging(const UnicodeString & ALogFileName);
  void TemporaryLogProtocol(int32_t ALogProtocol);
  void TemporaryLogSensitive(bool ALogSensitive);
  void TemporaryLogMaxSize(int64_t ALogMaxSize);
  void TemporaryLogMaxCount(int32_t ALogMaxCount);
  virtual RawByteString EncryptPassword(const UnicodeString & APassword, const UnicodeString & AKey);
  virtual UnicodeString DecryptPassword(const RawByteString & APassword, const UnicodeString & AKey);
  virtual RawByteString StronglyRecryptPassword(const RawByteString & APassword, const UnicodeString & Key);
  UnicodeString GetFileDescription(const UnicodeString & AFileName) const;
  UnicodeString GetFileVersion(const UnicodeString & AFileName) const;
  UnicodeString GetFileMimeType(const UnicodeString & AFileName) const;
  bool RegistryPathExists(const UnicodeString & RegistryPath) const;
  bool HasLocalPortNumberLimits() const;
  virtual UnicodeString TemporaryDir(bool Mask = false) const = 0;
  UnicodeString GetVersionStrHuman();

  TStoredSessionList * SelectFilezillaSessionsForImport(
    TStoredSessionList * Sessions, UnicodeString & Error);
  bool AnyFilezillaSessionForImport(TStoredSessionList * Sessions);
  TStoredSessionList * SelectKnownHostsSessionsForImport(
    TStoredSessionList * Sessions, UnicodeString & Error);
  TStoredSessionList * SelectKnownHostsSessionsForImport(
    TStrings * Lines, TStoredSessionList * Sessions, UnicodeString & Error);
  TStoredSessionList * SelectOpensshSessionsForImport(TStoredSessionList * Sessions, UnicodeString & Error);
  UnicodeString GetPuttySessionsKey(const UnicodeString & RootKey) const;
  void RefreshPuttySshHostCAList();

  __property TVSFixedFileInfo * FixedApplicationInfo  = { read = GetFixedApplicationInfo };
  __property void * ApplicationInfo  = { read=GetApplicationInfo };
  const ROProperty<void *> ApplicationInfo{nb::bind(&TConfiguration::GetApplicationInfo, this)};
  __property TUsage * Usage = { read = FUsage };
  const ROProperty<TUsage*> Usage{nb::bind(&TConfiguration::GetUsage, this)};
  __property bool CollectUsage = { read = GetCollectUsage, write = SetCollectUsage };
  RWProperty<bool> CollectUsage{nb::bind(&TConfiguration::GetCollectUsage, this), nb::bind(&TConfiguration::SetCollectUsage, this)};
  __property UnicodeString StoredSessionsSubKey = {read=GetStoredSessionsSubKey};
  const ROProperty<UnicodeString> StoredSessionsSubKey{nb::bind(&TConfiguration::GetStoredSessionsSubKey, this)};
  __property UnicodeString PuttyRegistryStorageKey  = { read=FPuttyRegistryStorageKey, write=SetPuttyRegistryStorageKey };
  RWProperty<UnicodeString> PuttyRegistryStorageKey{nb::bind(&TConfiguration::GetPuttyRegistryStorageKey, this), nb::bind(&TConfiguration::SetPuttyRegistryStorageKey, this)};
  __property UnicodeString PuttySessionsKey  = { read=DoGetPuttySessionsKey };
  const ROProperty<UnicodeString> PuttySessionsKey{nb::bind(&TConfiguration::DoGetPuttySessionsKey, this)};
  __property UnicodeString PuttySessionsSubKey  = { read=GetPuttySessionsSubKey };
  __property UnicodeString RandomSeedFile  = { read=FRandomSeedFile, write=SetRandomSeedFile };
  __property UnicodeString RandomSeedFileName  = { read=GetRandomSeedFileName };
  __property UnicodeString SshHostKeysSubKey  = { read=GetSshHostKeysSubKey };
  const ROProperty<UnicodeString> SshHostKeysSubKey{nb::bind(&TConfiguration::GetSshHostKeysSubKey, this)};
  __property UnicodeString RootKeyStr  = { read=GetRootKeyStr };
  const ROProperty<UnicodeString> RootKeyStr{nb::bind(&TConfiguration::GetRootKeyStr, this)};
  __property UnicodeString ConfigurationSubKey  = { read=GetConfigurationSubKey };
  const ROProperty<UnicodeString> ConfigurationSubKey{nb::bind(&TConfiguration::GetConfigurationSubKey, this)};
  __property TEOLType LocalEOLType = { read = GetLocalEOLType };
  const ROProperty<TEOLType> LocalEOLType{nb::bind(&TConfiguration::GetLocalEOLType, this)};
  __property UnicodeString VersionStr = { read=GetVersionStr };
  const ROProperty<UnicodeString> VersionStr{nb::bind(&TConfiguration::GetVersionStr, this)};
  __property UnicodeString Version = { read=GetVersion };
  const ROProperty<UnicodeString> Version {nb::bind(&TConfiguration::GetVersion, this)};
  __property int CompoundVersion = { read=GetCompoundVersion };
  const ROProperty<int32_t> CompoundVersion{nb::bind(&TConfiguration::GetCompoundVersion, this)};
  __property UnicodeString ProductVersion = { read=GetProductVersion };
  const ROProperty<UnicodeString> ProductVersion{nb::bind(&TConfiguration::GetProductVersionStr, this)};
  __property UnicodeString ProductName = { read=GetProductName };
  __property UnicodeString CompanyName = { read=GetCompanyName };
  __property bool IsUnofficial = { read = GetIsUnofficial };
  __property bool Logging  = { read=FLogging, write=SetLogging };
  RWProperty<bool> Logging{nb::bind(&TConfiguration::GetLogging, this), nb::bind(&TConfiguration::SetLogging, this)};
  __property UnicodeString LogFileName  = { read=FLogFileName, write=SetLogFileName };
  __property bool LogToFile  = { read=GetLogToFile };
  const ROProperty<bool> LogToFile{nb::bind(&TConfiguration::GetLogToFile, this)};
  __property bool LogFileAppend  = { read=FLogFileAppend, write=SetLogFileAppend };
  __property bool LogSensitive  = { read=FLogSensitive, write=SetLogSensitive };
  __property __int64 LogMaxSize  = { read=FLogMaxSize, write=SetLogMaxSize };
  __property int LogMaxCount  = { read=FLogMaxCount, write=SetLogMaxCount };
  __property int LogProtocol  = { read=FLogProtocol, write=SetLogProtocol };
  RWProperty<int32_t> LogProtocol{nb::bind(&TConfiguration::GetLogProtocol, this), nb::bind(&TConfiguration::SetLogProtocol, this)};
  __property int ActualLogProtocol  = { read=FActualLogProtocol };
  const ROProperty<int32_t> ActualLogProtocol{nb::bind(&TConfiguration::GetActualLogProtocol, this)};
  __property bool LogActions  = { read=FLogActions, write=SetLogActions };
  __property bool LogActionsRequired  = { read=FLogActionsRequired, write=FLogActionsRequired };
  __property UnicodeString ActionsLogFileName  = { read=GetActionsLogFileName, write=SetActionsLogFileName };
  __property UnicodeString DefaultLogFileName  = { read=GetDefaultLogFileName };
  __property TNotifyEvent OnChange = { read = FOnChange, write = FOnChange };
  __property bool ConfirmOverwriting = { read = GetConfirmOverwriting, write = SetConfirmOverwriting};
  __property bool ConfirmResume = { read = GetConfirmResume, write = SetConfirmResume};
  __property bool AutoReadDirectoryAfterOp = { read = GetAutoReadDirectoryAfterOp, write = SetAutoReadDirectoryAfterOp};
  __property bool RememberPassword = { read = GetRememberPassword };
  __property int32_t SessionReopenAuto = { read = FSessionReopenAuto, write = SetSessionReopenAuto };
  __property int32_t SessionReopenBackground = { read = FSessionReopenBackground, write = SetSessionReopenBackground };
  __property int32_t SessionReopenTimeout = { read = FSessionReopenTimeout, write = SetSessionReopenTimeout };
  __property int32_t SessionReopenAutoStall = { read = FSessionReopenAutoStall, write = SetSessionReopenAutoStall };
  __property int32_t TunnelLocalPortNumberLow = { read = FTunnelLocalPortNumberLow, write = SetTunnelLocalPortNumberLow };
  __property int32_t TunnelLocalPortNumberHigh = { read = FTunnelLocalPortNumberHigh, write = SetTunnelLocalPortNumberHigh };
  __property int32_t CacheDirectoryChangesMaxSize = { read = FCacheDirectoryChangesMaxSize, write = SetCacheDirectoryChangesMaxSize };
  __property bool ShowFtpWelcomeMessage = { read = FShowFtpWelcomeMessage, write = SetShowFtpWelcomeMessage };
  __property UnicodeString ExternalIpAddress = { read = FExternalIpAddress, write = SetExternalIpAddress };
  __property UnicodeString CertificateStorage = { read = FCertificateStorage, write = SetCertificateStorage };
  __property UnicodeString CertificateStorageExpanded = { read = GetCertificateStorageExpanded };
  const ROProperty<UnicodeString> CertificateStorageExpanded{nb::bind(&TConfiguration::GetCertificateStorageExpanded, this)};
  __property UnicodeString AWSMetadataService = { read = FAWSMetadataService, write = SetAWSMetadataService };
  __property UnicodeString ChecksumCommands = { read = FChecksumCommands };
  __property int32_t LocalPortNumberMin = { read = FLocalPortNumberMin, write = SetLocalPortNumberMin };
  __property int32_t LocalPortNumberMax = { read = FLocalPortNumberMax, write = SetLocalPortNumberMax };
  __property bool TryFtpWhenSshFails = { read = FTryFtpWhenSshFails, write = SetTryFtpWhenSshFails };
  __property int32_t ParallelDurationThreshold = { read = FParallelDurationThreshold, write = SetParallelDurationThreshold };
  __property UnicodeString MimeTypes = { read = FMimeTypes, write = SetMimeTypes };
  __property int DontReloadMoreThanSessions = { read = FDontReloadMoreThanSessions, write = FDontReloadMoreThanSessions };
  int32_t& DontReloadMoreThanSessions{FDontReloadMoreThanSessions};
  __property int ScriptProgressFileNameLimit = { read = FScriptProgressFileNameLimit, write = FScriptProgressFileNameLimit };
  __property int QueueTransfersLimit = { read = FQueueTransfersLimit, write = SetQueueTransfersLimit };
  RWPropertySimple<int32_t> QueueTransfersLimit{&FQueueTransfersLimit, nb::bind(&TConfiguration::SetQueueTransfersLimit, this)};
  __property int ParallelTransferThreshold = { read = FParallelTransferThreshold, write = FParallelTransferThreshold };
  int32_t& ParallelTransferThreshold{FParallelTransferThreshold};
  __property int KeyVersion = { read = FKeyVersion, write = FKeyVersion };
  int32_t& KeyVersion{FKeyVersion};
  __property TSshHostCAList * SshHostCAList = { read = GetSshHostCAList, write = SetSshHostCAList };
  __property TSshHostCAList * PuttySshHostCAList = { read = GetPuttySshHostCAList };
//  const ROProperty<TSshHostCAList *> PuttySshHostCAList{nb::bind(&TConfiguration::GetPuttySshHostCAList, this)};
  __property TSshHostCAList * ActiveSshHostCAList = { read = GetActiveSshHostCAList };
//  const ROProperty<TSshHostCAList *> ActiveSshHostCAList{nb::bind(&TConfiguration::GetActiveSshHostCAList, this)};
  __property bool SshHostCAsFromPuTTY = { read = FSshHostCAsFromPuTTY, write = FSshHostCAsFromPuTTY };
  __property int32_t HttpsCertificateValidation = { read = FHttpsCertificateValidation, write = FHttpsCertificateValidation };
  int32_t& HttpsCertificateValidation{FHttpsCertificateValidation};
  __property UnicodeString SynchronizationChecksumAlgs = { read = FSynchronizationChecksumAlgs, write = FSynchronizationChecksumAlgs };
  UnicodeString& SynchronizationChecksumAlgs{FSynchronizationChecksumAlgs};

  __property UnicodeString TimeFormat = { read = GetTimeFormat };
  const ROProperty<UnicodeString> TimeFormat{nb::bind(&TConfiguration::GetConfigurationTimeFormat, this)};
  __property TStorage Storage  = { read=GetStorage };
  const ROProperty<TStorage> Storage{nb::bind(&TConfiguration::GetStorage, this)};
  __property UnicodeString RegistryStorageKey  = { read=GetRegistryStorageKey };
  const ROProperty<UnicodeString> RegistryStorageKey{nb::bind(&TConfiguration::GetRegistryStorageKey, this)};
  __property UnicodeString CustomIniFileStorageName  = { read=FCustomIniFileStorageName };
  __property UnicodeString ExplicitIniFileStorageName  = { read=FIniFileStorageName };
  __property UnicodeString IniFileStorageName  = { read=GetIniFileStorageNameForReadingWriting };
  __property UnicodeString IniFileStorageNameForReading  = { read=GetIniFileStorageNameForReading };
  __property TStrings * OptionsStorage = { read = GetOptionsStorage, write = SetOptionsStorage };
  RWProperty<const TStrings *> OptionsStorage{nb::bind(&TConfiguration::GetOptionsStorage, this), nb::bind(&TConfiguration::SetOptionsStorage, this)};
  __property bool Persistent = { read = GetPersistent };
  const ROProperty<bool> Persistent{nb::bind(&TConfiguration::GetPersistent, this)};
  __property bool ForceSave = { read = FForceSave, write = FForceSave };
  bool& ForceSave{FForceSave};
  __property bool Scripting = { read = FScripting, write = FScripting };
  bool& Scripting{FScripting};

  __property UnicodeString DefaultKeyFile = { read = GetDefaultKeyFile };
  const ROProperty<UnicodeString> DefaultKeyFile{nb::bind(&TConfiguration::GetDefaultKeyFile, this)};

  __property bool DisablePasswordStoring = { read = FDisablePasswordStoring };
  const bool& DisablePasswordStoring{FDisablePasswordStoring};
  __property bool ForceBanners = { read = FForceBanners };
  const bool& ForceBanners{FForceBanners};
  __property bool DisableAcceptingHostKeys = { read = FDisableAcceptingHostKeys };
  const bool& DisableAcceptingHostKeys{FDisableAcceptingHostKeys};

  bool GetScripting() const { return FScripting; }
  void SetScripting(bool Value) { FScripting = Value; }
  void SetStorage(TStorage Value);

  TUsage * GetUsage() { return FUsage.get(); }
  UnicodeString GetPuttyRegistryStorageKey() const { return FPuttyRegistryStorageKey; }
  UnicodeString GetRandomSeedFile() const { return FRandomSeedFile; }
  bool GetLogFileAppend() const { return FLogFileAppend; }
  bool GetLogSensitive() const { return FLogSensitive; }
  int32_t GetLogProtocol() const { return FLogProtocol; }
  int32_t GetActualLogProtocol() const { return FActualLogProtocol; }
  bool GetLogActionsRequired() const { return FLogActionsRequired; }
  int32_t GetLogWindowLines() const { return FLogWindowLines; }
  TNotifyEvent & GetOnChange() { return FOnChange; }
  void SetOnChange(TNotifyEvent && Value) { FOnChange = std::move(Value); }
  int32_t GetSessionReopenAuto() const { return FSessionReopenAuto; }
  int32_t GetSessionReopenBackground() const { return FSessionReopenBackground; }
  int32_t GetSessionReopenTimeout() const { return FSessionReopenTimeout; }
  int32_t GetSessionReopenAutoStall() const { return FSessionReopenAutoStall; }
  int32_t GetTunnelLocalPortNumberLow() const { return FTunnelLocalPortNumberLow; }
  int32_t GetTunnelLocalPortNumberHigh() const { return FTunnelLocalPortNumberHigh; }
  int32_t GetCacheDirectoryChangesMaxSize() const { return FCacheDirectoryChangesMaxSize; }
  bool GetShowFtpWelcomeMessage() const { return FShowFtpWelcomeMessage; }
  UnicodeString GetExternalIpAddress() const { return FExternalIpAddress; }
  bool GetTryFtpWhenSshFails() const { return FTryFtpWhenSshFails; }
  int32_t GetParallelDurationThreshold() const { return FParallelDurationThreshold; }
  UnicodeString GetMimeTypes() const { return FMimeTypes; }
  bool GetDisablePasswordStoring() const { return FDisablePasswordStoring; }
  bool GetForceBanners() const { return FForceBanners; }
  bool GetDisableAcceptingHostKeys() const { return FDisableAcceptingHostKeys; }
  int32_t GetSessionReopenAutoMaximumNumberOfRetries() const { return FSessionReopenAutoMaximumNumberOfRetries; }
  void SetSessionReopenAutoMaximumNumberOfRetries(int32_t Value);
};

class NB_CORE_EXPORT TShortCuts final : public TObject
{
public:
  TShortCuts() = default;
  void Add(const TShortCut & ShortCut);
  bool Has(const TShortCut & ShortCut) const;

private:
  nb::vector_t<TShortCut> FShortCuts;
};

extern "C"
{
#include <windows/platform.h>
//#include <winstuff.h>
}

constexpr const char * OriginalPuttyRegistryStorageKey = PUTTY_REG_POS;
constexpr const char * KittyRegistryStorageKey = "Software\\9bis.com\\KiTTY";
constexpr const char * OriginalPuttyExecutable = "putty.exe";
constexpr const char * KittyExecutable = "kitty.exe";
constexpr const char * PuttyKeyExt = "ppk";

constexpr const char * Sha1ChecksumAlg = "sha-1";
constexpr const char * Sha224ChecksumAlg = "sha-224";
constexpr const char * Sha256ChecksumAlg = "sha-256";
constexpr const char * Sha384ChecksumAlg = "sha-384";
constexpr const char * Sha512ChecksumAlg = "sha-512";
constexpr const char * Md5ChecksumAlg = "md5";
// Not defined by IANA
constexpr const char * Crc32ChecksumAlg = "crc32";

constexpr const char * SshFingerprintType = "ssh";
constexpr const char * TlsFingerprintType = "tls";

constexpr const char * FtpsCertificateStorageKey = "FtpsCertificates";
constexpr const char * HttpsCertificateStorageKey = "HttpsCertificates";

constexpr const int32_t BelowNormalLogLevels = 1;

constexpr const char * OpensshFolderName = ".ssh";
constexpr const char * OpensshAuthorizedKeysFileName = "authorized_keys";

constexpr const char * LastFingerprintsStorageKey = "LastFingerprints";
constexpr const char * DirectoryStatisticsCacheKey = "DirectoryStatisticsCache";
constexpr const char * SshHostCAsKey = "SshHostCAs";
constexpr const char * CDCacheKey = "CDCache";
constexpr const char * BannersKey = "Banners";
