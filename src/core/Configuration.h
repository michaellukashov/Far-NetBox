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
//---------------------------------------------------------------------------
class TConfiguration : public nb::TObject
{
private:
  bool FDontSave;
  bool FChanged;
  int FUpdating;
  nb::notify_signal_type FOnChange;

  void * FApplicationInfo;
  bool FLogging;
  bool FPermanentLogging;
  std::wstring FLogFileName;
  std::wstring FPermanentLogFileName;
  int FLogWindowLines;
  bool FLogFileAppend;
  int FLogProtocol;
  int FActualLogProtocol;
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
  TCriticalSection * FCriticalSection;

  virtual void Changed();
  virtual void SaveData(THierarchicalStorage * Storage, bool All);
  virtual void LoadData(THierarchicalStorage * Storage);
  virtual void CopyData(THierarchicalStorage * Source, THierarchicalStorage * Target);
  virtual void LoadAdmin(THierarchicalStorage * Storage);
  virtual void Saved();
  void CleanupRegistry(const std::wstring CleanupSubKey);
  std::wstring BannerHash(const std::wstring Banner);

  virtual std::wstring ModuleFileName();

  std::wstring GetFileFileInfoString(const std::wstring Key,
    const std::wstring FileName);
  void * GetFileApplicationInfo(const std::wstring FileName);
  std::wstring GetFileProductVersion(const std::wstring FileName);
  std::wstring GetFileProductName(const std::wstring FileName);
  std::wstring GetFileCompanyName(const std::wstring FileName);

  // __property bool PermanentLogging  = { read=FPermanentLogging, write=SetLogging };
  bool GetPermanentLogging() { return FPermanentLogging; }
  void SetPermanentLogging(bool value) { FPermanentLogging = value; }
    // __property std::wstring PermanentLogFileName  = { read=FPermanentLogFileName, write=SetLogFileName };
  std::wstring GetPermanentLogFileName() { return FPermanentLogFileName; }
  void SetPermanentLogFileName(const std::wstring value) { FPermanentLogFileName = value; }
  // __property bool PermanentLogActions  = { read=FPermanentLogActions, write=SetLogActions };
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
    TRemoteDirectoryChangesCache * DirectoryChangesCache);
  void SaveDirectoryChangesCache(const std::wstring SessionKey,
    TRemoteDirectoryChangesCache * DirectoryChangesCache);
  bool ShowBanner(const std::wstring SessionKey, const std::wstring Banner);
  void NeverShowBanner(const std::wstring SessionKey, const std::wstring Banner);
  virtual THierarchicalStorage * CreateStorage();
  void TemporaryLogging(const std::wstring ALogFileName);
  virtual std::wstring EncryptPassword(const std::wstring Password, const std::wstring Key);
  virtual std::wstring DecryptPassword(const std::wstring Password, const std::wstring Key);
  virtual std::wstring StronglyRecryptPassword(const std::wstring Password, const std::wstring Key);

  // __property TVSFixedFileInfo *FixedApplicationInfo  = { read=GetFixedApplicationInfo };
  VS_FIXEDFILEINFO GetFixedApplicationInfo();
  // __property void * ApplicationInfo  = { read=GetApplicationInfo };
  void *GetApplicationInfo();
  // __property std::wstring StoredSessionsSubKey = {read=GetStoredSessionsSubKey};
  std::wstring GetStoredSessionsSubKey();
  // __property std::wstring PuttyRegistryStorageKey  = { read=FPuttyRegistryStorageKey, write=SetPuttyRegistryStorageKey };
  std::wstring GetPuttyRegistryStorageKey() { return FPuttyRegistryStorageKey; }
  void SetPuttyRegistryStorageKey(const std::wstring value);
  // __property std::wstring PuttySessionsKey  = { read=GetPuttySessionsKey };
  std::wstring GetPuttySessionsKey();
  // __property std::wstring RandomSeedFile  = { read=FRandomSeedFile, write=SetRandomSeedFile };
  std::wstring GetRandomSeedFile() { return FRandomSeedFile; }
  void SetRandomSeedFile(const std::wstring value);
  // __property std::wstring RandomSeedFileName  = { read=GetRandomSeedFileName };
  std::wstring GetRandomSeedFileName();
  // __property std::wstring SshHostKeysSubKey  = { read=GetSshHostKeysSubKey };
  std::wstring GetSshHostKeysSubKey();
  // __property std::wstring RootKeyStr  = { read=GetRootKeyStr };
  std::wstring GetRootKeyStr();
  // __property std::wstring ConfigurationSubKey  = { read=GetConfigurationSubKey };
  std::wstring GetConfigurationSubKey();
  // __property TEOLType LocalEOLType = { read = GetLocalEOLType };
  TEOLType GetLocalEOLType();
  // __property std::wstring VersionStr = { read=GetVersionStr };
  virtual std::wstring GetVersionStr();
  // __property std::wstring Version = { read=GetVersion };
  virtual std::wstring GetVersion();
  // __property int CompoundVersion = { read=GetCompoundVersion };
  int GetCompoundVersion();
  // __property std::wstring ProductVersion = { read=GetProductVersion };
  std::wstring GetProductVersion();
  // __property std::wstring ProductName = { read=GetProductName };
  std::wstring GetProductName();
  // __property std::wstring CompanyName = { read=GetCompanyName };
  std::wstring GetCompanyName();
  // __property std::wstring FileInfoString[std::wstring Key] = { read = GetFileInfoString };
  std::wstring GetFileInfoString(const std::wstring Key);
  // __property std::wstring OSVersionStr = { read = GetOSVersionStr };
  std::wstring GetOSVersionStr();
  // __property bool Logging  = { read=FLogging, write=SetLogging };
  bool GetLogging() { return FLogging; }
  void SetLogging(bool value);
  // __property std::wstring LogFileName  = { read=FLogFileName, write=SetLogFileName };
  std::wstring GetLogFileName() { return FLogFileName; }
  void SetLogFileName(const std::wstring value);
  // __property bool LogToFile  = { read=GetLogToFile, write=SetLogToFile };
  bool GetLogToFile();
  void SetLogToFile(bool value);
  // __property bool LogFileAppend  = { read=FLogFileAppend, write=SetLogFileAppend };
  bool GetLogFileAppend() { return FLogFileAppend; }
  void SetLogFileAppend(bool value);
  // __property int LogProtocol  = { read=FLogProtocol, write=SetLogProtocol };
  int GetLogProtocol() { return FLogProtocol; }
  void SetLogProtocol(int value);
  // __property int ActualLogProtocol  = { read=FActualLogProtocol };
  int GetActualLogProtocol() { return FActualLogProtocol; }
  // __property bool LogActions  = { read=FLogActions, write=SetLogActions };
  bool GetLogActions() { return FLogActions; }
  void SetLogActions(bool value);
  // __property int LogWindowLines  = { read=FLogWindowLines, write=SetLogWindowLines };
  int GetLogWindowLines() { return FLogWindowLines; }
  void SetLogWindowLines(int value);
  // __property bool LogWindowComplete  = { read=GetLogWindowComplete, write=SetLogWindowComplete };
  bool GetLogWindowComplete();
  void SetLogWindowComplete(bool value);
  // __property std::wstring DefaultLogFileName  = { read=GetDefaultLogFileName };
  std::wstring GetDefaultLogFileName();
  // __property TNotifyEvent OnChange = { read = FOnChange, write = FOnChange };
  const nb::notify_signal_type &GetOnChange() const { return FOnChange; }
  void SetOnChange(const nb::notify_slot_type &value) { FOnChange.connect(value); }
  // __property bool ConfirmOverwriting = { read = GetConfirmOverwriting, write = SetConfirmOverwriting};
  virtual bool GetConfirmOverwriting();
  virtual void SetConfirmOverwriting(bool value);
  // __property bool ConfirmResume = { read = GetConfirmResume, write = SetConfirmResume};
  bool GetConfirmResume();
  void SetConfirmResume(bool value);
  // __property bool AutoReadDirectoryAfterOp = { read = GetAutoReadDirectoryAfterOp, write = SetAutoReadDirectoryAfterOp};
  bool GetAutoReadDirectoryAfterOp();
  void SetAutoReadDirectoryAfterOp(bool value);
  // __property bool RememberPassword = { read = GetRememberPassword };
  virtual bool GetRememberPassword();
  // __property std::wstring PartialExt = {read=GetPartialExt};
  std::wstring GetPartialExt() const;
  // __property int SessionReopenAuto = { read = FSessionReopenAuto, write = SetSessionReopenAuto };
  int GetSessionReopenAuto() { return FSessionReopenAuto; }
  void SetSessionReopenAuto(int value);
  // __property int SessionReopenBackground = { read = FSessionReopenBackground, write = SetSessionReopenBackground };
  int GetSessionReopenBackground() { return FSessionReopenBackground; }
  void SetSessionReopenBackground(int value);
  // __property int SessionReopenTimeout = { read = FSessionReopenTimeout, write = SetSessionReopenTimeout };
  int GetSessionReopenTimeout() { return FSessionReopenTimeout; }
  void SetSessionReopenTimeout(int value);
  // __property int TunnelLocalPortNumberLow = { read = FTunnelLocalPortNumberLow, write = SetTunnelLocalPortNumberLow };
  size_t GetTunnelLocalPortNumberLow() { return FTunnelLocalPortNumberLow; }
  void SetTunnelLocalPortNumberLow(size_t value);
  // __property int TunnelLocalPortNumberHigh = { read = FTunnelLocalPortNumberHigh, write = SetTunnelLocalPortNumberHigh };
  size_t GetTunnelLocalPortNumberHigh() { return FTunnelLocalPortNumberHigh; }
  void SetTunnelLocalPortNumberHigh(size_t value);
  // __property int CacheDirectoryChangesMaxSize = { read = FCacheDirectoryChangesMaxSize, write = SetCacheDirectoryChangesMaxSize };
  int GetCacheDirectoryChangesMaxSize() { return FCacheDirectoryChangesMaxSize; }
  void SetCacheDirectoryChangesMaxSize(int value);
  // __property bool ShowFtpWelcomeMessage = { read = FShowFtpWelcomeMessage, write = SetShowFtpWelcomeMessage };
  bool GetShowFtpWelcomeMessage() { return FShowFtpWelcomeMessage; }
  void SetShowFtpWelcomeMessage(bool value);

  // __property std::wstring TimeFormat = { read = GetTimeFormat };
  std::wstring GetTimeFormat();
  // __property TStorage Storage  = { read=GetStorage, write=SetStorage };
  virtual TStorage GetStorage();
  void SetStorage(TStorage value);
  // __property std::wstring RegistryStorageKey  = { read=GetRegistryStorageKey };
  std::wstring GetRegistryStorageKey();
  // __property std::wstring IniFileStorageName  = { read=GetIniFileStorageName, write=SetIniFileStorageName };
  std::wstring GetIniFileStorageName();
  void SetIniFileStorageName(const std::wstring value);
  // __property std::wstring DefaultKeyFile = { read = GetDefaultKeyFile };
  virtual std::wstring GetDefaultKeyFile();

  // __property bool DisablePasswordStoring = { read = FDisablePasswordStoring };
  bool GetDisablePasswordStoring() { return FDisablePasswordStoring; }
  // __property bool ForceBanners = { read = FForceBanners };
  bool GetForceBanners() { return FForceBanners; }
  // __property bool DisableAcceptingHostKeys = { read = FDisableAcceptingHostKeys };
  bool GetDisableAcceptingHostKeys() { return FDisableAcceptingHostKeys; }
  // __property bool GSSAPIInstalled = { read = GetGSSAPIInstalled };
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
