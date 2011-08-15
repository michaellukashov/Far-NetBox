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

  // TVSFixedFileInfo *GetFixedApplicationInfo();
  wstring TrimVersion(wstring Version);
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
  void CleanupRegistry(wstring CleanupSubKey);
  wstring BannerHash(const wstring & Banner);

  virtual wstring ModuleFileName();

  wstring GetFileFileInfoString(const wstring Key,
    const wstring FileName);
  void * GetFileApplicationInfo(const wstring FileName);
  wstring GetFileProductVersion(const wstring FileName);
  wstring GetFileProductName(const wstring FileName);
  wstring GetFileCompanyName(const wstring FileName);

  // __property bool PermanentLogging  = { read=FPermanentLogging, write=SetLogging };
  bool GetPermanentLogging() { return FPermanentLogging; }
  void SetLogging(bool value);
    // __property wstring PermanentLogFileName  = { read=FPermanentLogFileName, write=SetLogFileName };
  wstring GetPermanentLogFileName() { return FPermanentLogFileName; }
  // __property bool PermanentLogActions  = { read=FPermanentLogActions, write=SetLogActions };
  bool GetPermanentLogActions() { return FPermanentLogActions; }

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

  // __property TVSFixedFileInfo *FixedApplicationInfo  = { read=GetFixedApplicationInfo };
  // __property void * ApplicationInfo  = { read=GetApplicationInfo };
  void * GetApplicationInfo();
  // __property wstring StoredSessionsSubKey = {read=GetStoredSessionsSubKey};
  wstring GetStoredSessionsSubKey();
  // __property wstring PuttyRegistryStorageKey  = { read=FPuttyRegistryStorageKey, write=SetPuttyRegistryStorageKey };
  wstring GetPuttyRegistryStorageKey() { return FPuttyRegistryStorageKey; }
  void SetPuttyRegistryStorageKey(wstring value);
  // __property wstring PuttySessionsKey  = { read=GetPuttySessionsKey };
  wstring GetPuttySessionsKey();
  // __property wstring RandomSeedFile  = { read=FRandomSeedFile, write=SetRandomSeedFile };
  wstring GetRandomSeedFile() { return FRandomSeedFile; }
  void SetRandomSeedFile(wstring value);
  // __property wstring RandomSeedFileName  = { read=GetRandomSeedFileName };
  wstring GetRandomSeedFileName();
  // __property wstring SshHostKeysSubKey  = { read=GetSshHostKeysSubKey };
  wstring GetSshHostKeysSubKey();
  // __property wstring RootKeyStr  = { read=GetRootKeyStr };
  wstring GetRootKeyStr();
  // __property wstring ConfigurationSubKey  = { read=GetConfigurationSubKey };
  wstring GetConfigurationSubKey();
  // __property TEOLType LocalEOLType = { read = GetLocalEOLType };
  TEOLType GetLocalEOLType();
  // __property wstring VersionStr = { read=GetVersionStr };
  virtual wstring GetVersionStr();
  // __property wstring Version = { read=GetVersion };
  virtual wstring GetVersion();
  // __property int CompoundVersion = { read=GetCompoundVersion };
  int GetCompoundVersion();
  // __property wstring ProductVersion = { read=GetProductVersion };
  wstring GetProductVersion();
  // __property wstring ProductName = { read=GetProductName };
  wstring GetProductName();
  // __property wstring CompanyName = { read=GetCompanyName };
  wstring GetCompanyName();
  // __property wstring FileInfoString[wstring Key] = { read = GetFileInfoString };
  wstring GetFileInfoString(const wstring Key);
  // __property wstring OSVersionStr = { read = GetOSVersionStr };
  wstring GetOSVersionStr();
  // __property bool Logging  = { read=FLogging, write=SetLogging };
  bool GetLogging() { return FLogging; }
  // __property wstring LogFileName  = { read=FLogFileName, write=SetLogFileName };
  wstring GetLogFileName() { return FLogFileName; }
  void SetLogFileName(wstring value);
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
  // __property wstring DefaultLogFileName  = { read=GetDefaultLogFileName };
  wstring GetDefaultLogFileName();
  // __property TNotifyEvent OnChange = { read = FOnChange, write = FOnChange };
  TNotifyEvent GetOnChange() { return FOnChange; }
  void SetOnChange(TNotifyEvent value) { FOnChange = value; }
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
  // __property wstring PartialExt = {read=GetPartialExt};
  wstring GetPartialExt() const;
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
  int GetTunnelLocalPortNumberLow() { return FTunnelLocalPortNumberLow; }
  void SetTunnelLocalPortNumberLow(int value);
  // __property int TunnelLocalPortNumberHigh = { read = FTunnelLocalPortNumberHigh, write = SetTunnelLocalPortNumberHigh };
  int GetTunnelLocalPortNumberHigh() { return FTunnelLocalPortNumberHigh; }
  void SetTunnelLocalPortNumberHigh(int value);
  // __property int CacheDirectoryChangesMaxSize = { read = FCacheDirectoryChangesMaxSize, write = SetCacheDirectoryChangesMaxSize };
  int GetCacheDirectoryChangesMaxSize() { return FCacheDirectoryChangesMaxSize; }
  void SetCacheDirectoryChangesMaxSize(int value);
  // __property bool ShowFtpWelcomeMessage = { read = FShowFtpWelcomeMessage, write = SetShowFtpWelcomeMessage };
  bool GetShowFtpWelcomeMessage() { return FShowFtpWelcomeMessage; }
  void SetShowFtpWelcomeMessage(bool value);

  // __property wstring TimeFormat = { read = GetTimeFormat };
  wstring GetTimeFormat();
  // __property TStorage Storage  = { read=GetStorage, write=SetStorage };
  virtual TStorage GetStorage();
  void SetStorage(TStorage value);
  // __property wstring RegistryStorageKey  = { read=GetRegistryStorageKey };
  wstring GetRegistryStorageKey();
  // __property wstring IniFileStorageName  = { read=GetIniFileStorageName, write=SetIniFileStorageName };
  wstring GetIniFileStorageName();
  void SetIniFileStorageName(wstring value);
  // __property wstring DefaultKeyFile = { read = GetDefaultKeyFile };
  virtual wstring GetDefaultKeyFile();

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
// FIXME
class TShortCut
{
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
