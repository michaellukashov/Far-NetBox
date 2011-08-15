//---------------------------------------------------------------------------
#pragma once

#include <set>
#include "RemoteFiles.h"
#include "FileBuffer.h"
#include "HierarchicalStorage.h"
//---------------------------------------------------------------------------
#define SET_CONFIG_PROPERTY_EX(PROPERTY, APPLY) \
  if (PROPERTY != value) { F ## PROPERTY = value; Changed(); APPLY; }
#define SET_CONFIG_PROPERTY(PROPERTY) \
  SET_CONFIG_PROPERTY_EX(PROPERTY, )
//---------------------------------------------------------------------------
class TCriticalSection;
enum TAutoSwitch { asOn, asOff, asAuto };
//---------------------------------------------------------------------------
class TConfiguration : public TObject
{
private:
  bool FDontSave;
  bool FChanged;
  int FUpdating;
  TNotifyEvent FOnChange;

  void * FApplicationInfo;
  bool FLogging;
  bool FPermanentLogging;
  wstring FLogFileName;
  wstring FPermanentLogFileName;
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
  wstring FIniFileStorageName;
  int FTunnelLocalPortNumberLow;
  int FTunnelLocalPortNumberHigh;
  int FCacheDirectoryChangesMaxSize;
  bool FShowFtpWelcomeMessage;
  wstring FDefaultRandomSeedFile;
  wstring FRandomSeedFile;
  wstring FPuttyRegistryStorageKey;

  bool FDisablePasswordStoring;
  bool FForceBanners;
  bool FDisableAcceptingHostKeys;

  wstring GetOSVersionStr();
  TVSFixedFileInfo *GetFixedApplicationInfo();
  void * GetApplicationInfo();
  virtual wstring GetVersionStr();
  virtual wstring GetVersion();
  wstring GetProductVersion();
  wstring GetProductName();
  wstring GetCompanyName();
  wstring TrimVersion(wstring Version);
  wstring GetStoredSessionsSubKey();
  wstring GetPuttySessionsKey();
  void SetRandomSeedFile(wstring value);
  wstring GetRandomSeedFileName();
  void SetPuttyRegistryStorageKey(wstring value);
  wstring GetSshHostKeysSubKey();
  wstring GetRootKeyStr();
  wstring GetConfigurationSubKey();
  TEOLType GetLocalEOLType();
  void SetLogging(bool value);
  void SetLogFileName(wstring value);
  void SetLogToFile(bool value);
  bool GetLogToFile();
  void SetLogWindowLines(int value);
  void SetLogWindowComplete(bool value);
  bool GetLogWindowComplete();
  void SetLogFileAppend(bool value);
  void SetLogProtocol(int value);
  void SetLogActions(bool value);
  wstring GetDefaultLogFileName();
  wstring GetTimeFormat();
  void SetStorage(TStorage value);
  wstring GetRegistryStorageKey();
  wstring GetIniFileStorageName();
  void SetIniFileStorageName(wstring value);
  wstring GetPartialExt() const;
  wstring GetFileInfoString(const wstring Key);
  bool GetGSSAPIInstalled();
  void SetSessionReopenAuto(int value);
  void SetSessionReopenBackground(int value);
  void SetSessionReopenTimeout(int value);
  void SetTunnelLocalPortNumberLow(int value);
  void SetTunnelLocalPortNumberHigh(int value);
  void SetCacheDirectoryChangesMaxSize(int value);
  void SetShowFtpWelcomeMessage(bool value);
  int GetCompoundVersion();
  void UpdateActualLogProtocol();

protected:
  TStorage FStorage;
  TCriticalSection * FCriticalSection;

  virtual TStorage GetStorage();
  virtual void Changed();
  virtual void SaveData(THierarchicalStorage * Storage, bool All);
  virtual void LoadData(THierarchicalStorage * Storage);
  virtual void CopyData(THierarchicalStorage * Source, THierarchicalStorage * Target);
  virtual void LoadAdmin(THierarchicalStorage * Storage);
  virtual wstring GetDefaultKeyFile();
  virtual void Saved();
  void CleanupRegistry(wstring CleanupSubKey);
  wstring BannerHash(const wstring & Banner);

  virtual bool GetConfirmOverwriting();
  virtual void SetConfirmOverwriting(bool value);
  bool GetConfirmResume();
  void SetConfirmResume(bool value);
  bool GetAutoReadDirectoryAfterOp();
  void SetAutoReadDirectoryAfterOp(bool value);
  virtual bool GetRememberPassword();

  virtual wstring ModuleFileName();

  wstring GetFileFileInfoString(const wstring Key,
    const wstring FileName);
  void * GetFileApplicationInfo(const wstring FileName);
  wstring GetFileProductVersion(const wstring FileName);
  wstring GetFileProductName(const wstring FileName);
  wstring GetFileCompanyName(const wstring FileName);

  __property bool PermanentLogging  = { read=FPermanentLogging, write=SetLogging };
  __property wstring PermanentLogFileName  = { read=FPermanentLogFileName, write=SetLogFileName };
  __property bool PermanentLogActions  = { read=FPermanentLogActions, write=SetLogActions };

public:
  TConfiguration();
  virtual ~TConfiguration();
  virtual void Default();
  virtual void Load();
  virtual void Save(bool All, bool Explicit);
  void Export(const wstring FileName);
  void CleanupConfiguration();
  void CleanupIniFile();
  void CleanupHostKeys();
  void CleanupRandomSeedFile();
  void BeginUpdate();
  void EndUpdate();
  void LoadDirectoryChangesCache(const wstring SessionKey,
    TRemoteDirectoryChangesCache * DirectoryChangesCache);
  void SaveDirectoryChangesCache(const wstring SessionKey,
    TRemoteDirectoryChangesCache * DirectoryChangesCache);
  bool ShowBanner(const wstring SessionKey, const wstring & Banner);
  void NeverShowBanner(const wstring SessionKey, const wstring & Banner);
  virtual THierarchicalStorage * CreateScpStorage(bool SessionList);
  void TemporaryLogging(const wstring ALogFileName);
  virtual wstring EncryptPassword(wstring Password, wstring Key);
  virtual wstring DecryptPassword(wstring Password, wstring Key);
  virtual wstring StronglyRecryptPassword(wstring Password, wstring Key);

  __property TVSFixedFileInfo *FixedApplicationInfo  = { read=GetFixedApplicationInfo };
  __property void * ApplicationInfo  = { read=GetApplicationInfo };
  __property wstring StoredSessionsSubKey = {read=GetStoredSessionsSubKey};
  __property wstring PuttyRegistryStorageKey  = { read=FPuttyRegistryStorageKey, write=SetPuttyRegistryStorageKey };
  __property wstring PuttySessionsKey  = { read=GetPuttySessionsKey };
  __property wstring RandomSeedFile  = { read=FRandomSeedFile, write=SetRandomSeedFile };
  __property wstring RandomSeedFileName  = { read=GetRandomSeedFileName };
  __property wstring SshHostKeysSubKey  = { read=GetSshHostKeysSubKey };
  __property wstring RootKeyStr  = { read=GetRootKeyStr };
  __property wstring ConfigurationSubKey  = { read=GetConfigurationSubKey };
  __property TEOLType LocalEOLType = { read = GetLocalEOLType };
  __property wstring VersionStr = { read=GetVersionStr };
  __property wstring Version = { read=GetVersion };
  __property int CompoundVersion = { read=GetCompoundVersion };
  __property wstring ProductVersion = { read=GetProductVersion };
  __property wstring ProductName = { read=GetProductName };
  __property wstring CompanyName = { read=GetCompanyName };
  __property wstring FileInfoString[wstring Key] = { read = GetFileInfoString };
  __property wstring OSVersionStr = { read = GetOSVersionStr };
  __property bool Logging  = { read=FLogging, write=SetLogging };
  __property wstring LogFileName  = { read=FLogFileName, write=SetLogFileName };
  __property bool LogToFile  = { read=GetLogToFile, write=SetLogToFile };
  __property bool LogFileAppend  = { read=FLogFileAppend, write=SetLogFileAppend };
  __property int LogProtocol  = { read=FLogProtocol, write=SetLogProtocol };
  __property int ActualLogProtocol  = { read=FActualLogProtocol };
  __property bool LogActions  = { read=FLogActions, write=SetLogActions };
  __property int LogWindowLines  = { read=FLogWindowLines, write=SetLogWindowLines };
  __property bool LogWindowComplete  = { read=GetLogWindowComplete, write=SetLogWindowComplete };
  __property wstring DefaultLogFileName  = { read=GetDefaultLogFileName };
  __property TNotifyEvent OnChange = { read = FOnChange, write = FOnChange };
  __property bool ConfirmOverwriting = { read = GetConfirmOverwriting, write = SetConfirmOverwriting};
  __property bool ConfirmResume = { read = GetConfirmResume, write = SetConfirmResume};
  __property bool AutoReadDirectoryAfterOp = { read = GetAutoReadDirectoryAfterOp, write = SetAutoReadDirectoryAfterOp};
  __property bool RememberPassword = { read = GetRememberPassword };
  __property wstring PartialExt = {read=GetPartialExt};
  __property int SessionReopenAuto = { read = FSessionReopenAuto, write = SetSessionReopenAuto };
  __property int SessionReopenBackground = { read = FSessionReopenBackground, write = SetSessionReopenBackground };
  __property int SessionReopenTimeout = { read = FSessionReopenTimeout, write = SetSessionReopenTimeout };
  __property int TunnelLocalPortNumberLow = { read = FTunnelLocalPortNumberLow, write = SetTunnelLocalPortNumberLow };
  __property int TunnelLocalPortNumberHigh = { read = FTunnelLocalPortNumberHigh, write = SetTunnelLocalPortNumberHigh };
  __property int CacheDirectoryChangesMaxSize = { read = FCacheDirectoryChangesMaxSize, write = SetCacheDirectoryChangesMaxSize };
  __property bool ShowFtpWelcomeMessage = { read = FShowFtpWelcomeMessage, write = SetShowFtpWelcomeMessage };

  __property wstring TimeFormat = { read = GetTimeFormat };
  __property TStorage Storage  = { read=GetStorage, write=SetStorage };
  __property wstring RegistryStorageKey  = { read=GetRegistryStorageKey };
  __property wstring IniFileStorageName  = { read=GetIniFileStorageName, write=SetIniFileStorageName };
  __property wstring DefaultKeyFile = { read = GetDefaultKeyFile };

  __property bool DisablePasswordStoring = { read = FDisablePasswordStoring };
  __property bool ForceBanners = { read = FForceBanners };
  __property bool DisableAcceptingHostKeys = { read = FDisableAcceptingHostKeys };
  __property bool GSSAPIInstalled = { read = GetGSSAPIInstalled };
};
//---------------------------------------------------------------------------
class TShortCuts
{
public:
  void Add(TShortCut ShortCut);
  bool Has(TShortCut ShortCut) const;

private:
  std::set<TShortCut> FShortCuts;
};
//---------------------------------------------------------------------------
