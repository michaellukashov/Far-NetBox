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
  AnsiString FLogFileName;
  AnsiString FPermanentLogFileName;
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
  AnsiString FIniFileStorageName;
  int FTunnelLocalPortNumberLow;
  int FTunnelLocalPortNumberHigh;
  int FCacheDirectoryChangesMaxSize;
  bool FShowFtpWelcomeMessage;
  AnsiString FDefaultRandomSeedFile;
  AnsiString FRandomSeedFile;
  AnsiString FPuttyRegistryStorageKey;

  bool FDisablePasswordStoring;
  bool FForceBanners;
  bool FDisableAcceptingHostKeys;

  AnsiString GetOSVersionStr();
  TVSFixedFileInfo *GetFixedApplicationInfo();
  void * GetApplicationInfo();
  virtual AnsiString GetVersionStr();
  virtual AnsiString GetVersion();
  AnsiString GetProductVersion();
  AnsiString GetProductName();
  AnsiString GetCompanyName();
  AnsiString TrimVersion(AnsiString Version);
  AnsiString GetStoredSessionsSubKey();
  AnsiString GetPuttySessionsKey();
  void SetRandomSeedFile(AnsiString value);
  AnsiString GetRandomSeedFileName();
  void SetPuttyRegistryStorageKey(AnsiString value);
  AnsiString GetSshHostKeysSubKey();
  AnsiString GetRootKeyStr();
  AnsiString GetConfigurationSubKey();
  TEOLType GetLocalEOLType();
  void SetLogging(bool value);
  void SetLogFileName(AnsiString value);
  void SetLogToFile(bool value);
  bool GetLogToFile();
  void SetLogWindowLines(int value);
  void SetLogWindowComplete(bool value);
  bool GetLogWindowComplete();
  void SetLogFileAppend(bool value);
  void SetLogProtocol(int value);
  void SetLogActions(bool value);
  AnsiString GetDefaultLogFileName();
  AnsiString GetTimeFormat();
  void SetStorage(TStorage value);
  AnsiString GetRegistryStorageKey();
  AnsiString GetIniFileStorageName();
  void SetIniFileStorageName(AnsiString value);
  AnsiString GetPartialExt() const;
  AnsiString GetFileInfoString(const AnsiString Key);
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
  virtual AnsiString GetDefaultKeyFile();
  virtual void Saved();
  void CleanupRegistry(AnsiString CleanupSubKey);
  AnsiString BannerHash(const AnsiString & Banner);

  virtual bool GetConfirmOverwriting();
  virtual void SetConfirmOverwriting(bool value);
  bool GetConfirmResume();
  void SetConfirmResume(bool value);
  bool GetAutoReadDirectoryAfterOp();
  void SetAutoReadDirectoryAfterOp(bool value);
  virtual bool GetRememberPassword();

  virtual AnsiString ModuleFileName();

  AnsiString GetFileFileInfoString(const AnsiString Key,
    const AnsiString FileName);
  void * GetFileApplicationInfo(const AnsiString FileName);
  AnsiString GetFileProductVersion(const AnsiString FileName);
  AnsiString GetFileProductName(const AnsiString FileName);
  AnsiString GetFileCompanyName(const AnsiString FileName);

  __property bool PermanentLogging  = { read=FPermanentLogging, write=SetLogging };
  __property AnsiString PermanentLogFileName  = { read=FPermanentLogFileName, write=SetLogFileName };
  __property bool PermanentLogActions  = { read=FPermanentLogActions, write=SetLogActions };

public:
  TConfiguration();
  virtual ~TConfiguration();
  virtual void Default();
  virtual void Load();
  virtual void Save(bool All, bool Explicit);
  void Export(const AnsiString FileName);
  void CleanupConfiguration();
  void CleanupIniFile();
  void CleanupHostKeys();
  void CleanupRandomSeedFile();
  void BeginUpdate();
  void EndUpdate();
  void LoadDirectoryChangesCache(const AnsiString SessionKey,
    TRemoteDirectoryChangesCache * DirectoryChangesCache);
  void SaveDirectoryChangesCache(const AnsiString SessionKey,
    TRemoteDirectoryChangesCache * DirectoryChangesCache);
  bool ShowBanner(const AnsiString SessionKey, const AnsiString & Banner);
  void NeverShowBanner(const AnsiString SessionKey, const AnsiString & Banner);
  virtual THierarchicalStorage * CreateScpStorage(bool SessionList);
  void TemporaryLogging(const AnsiString ALogFileName);
  virtual AnsiString EncryptPassword(AnsiString Password, AnsiString Key);
  virtual AnsiString DecryptPassword(AnsiString Password, AnsiString Key);
  virtual AnsiString StronglyRecryptPassword(AnsiString Password, AnsiString Key);

  __property TVSFixedFileInfo *FixedApplicationInfo  = { read=GetFixedApplicationInfo };
  __property void * ApplicationInfo  = { read=GetApplicationInfo };
  __property AnsiString StoredSessionsSubKey = {read=GetStoredSessionsSubKey};
  __property AnsiString PuttyRegistryStorageKey  = { read=FPuttyRegistryStorageKey, write=SetPuttyRegistryStorageKey };
  __property AnsiString PuttySessionsKey  = { read=GetPuttySessionsKey };
  __property AnsiString RandomSeedFile  = { read=FRandomSeedFile, write=SetRandomSeedFile };
  __property AnsiString RandomSeedFileName  = { read=GetRandomSeedFileName };
  __property AnsiString SshHostKeysSubKey  = { read=GetSshHostKeysSubKey };
  __property AnsiString RootKeyStr  = { read=GetRootKeyStr };
  __property AnsiString ConfigurationSubKey  = { read=GetConfigurationSubKey };
  __property TEOLType LocalEOLType = { read = GetLocalEOLType };
  __property AnsiString VersionStr = { read=GetVersionStr };
  __property AnsiString Version = { read=GetVersion };
  __property int CompoundVersion = { read=GetCompoundVersion };
  __property AnsiString ProductVersion = { read=GetProductVersion };
  __property AnsiString ProductName = { read=GetProductName };
  __property AnsiString CompanyName = { read=GetCompanyName };
  __property AnsiString FileInfoString[AnsiString Key] = { read = GetFileInfoString };
  __property AnsiString OSVersionStr = { read = GetOSVersionStr };
  __property bool Logging  = { read=FLogging, write=SetLogging };
  __property AnsiString LogFileName  = { read=FLogFileName, write=SetLogFileName };
  __property bool LogToFile  = { read=GetLogToFile, write=SetLogToFile };
  __property bool LogFileAppend  = { read=FLogFileAppend, write=SetLogFileAppend };
  __property int LogProtocol  = { read=FLogProtocol, write=SetLogProtocol };
  __property int ActualLogProtocol  = { read=FActualLogProtocol };
  __property bool LogActions  = { read=FLogActions, write=SetLogActions };
  __property int LogWindowLines  = { read=FLogWindowLines, write=SetLogWindowLines };
  __property bool LogWindowComplete  = { read=GetLogWindowComplete, write=SetLogWindowComplete };
  __property AnsiString DefaultLogFileName  = { read=GetDefaultLogFileName };
  __property TNotifyEvent OnChange = { read = FOnChange, write = FOnChange };
  __property bool ConfirmOverwriting = { read = GetConfirmOverwriting, write = SetConfirmOverwriting};
  __property bool ConfirmResume = { read = GetConfirmResume, write = SetConfirmResume};
  __property bool AutoReadDirectoryAfterOp = { read = GetAutoReadDirectoryAfterOp, write = SetAutoReadDirectoryAfterOp};
  __property bool RememberPassword = { read = GetRememberPassword };
  __property AnsiString PartialExt = {read=GetPartialExt};
  __property int SessionReopenAuto = { read = FSessionReopenAuto, write = SetSessionReopenAuto };
  __property int SessionReopenBackground = { read = FSessionReopenBackground, write = SetSessionReopenBackground };
  __property int SessionReopenTimeout = { read = FSessionReopenTimeout, write = SetSessionReopenTimeout };
  __property int TunnelLocalPortNumberLow = { read = FTunnelLocalPortNumberLow, write = SetTunnelLocalPortNumberLow };
  __property int TunnelLocalPortNumberHigh = { read = FTunnelLocalPortNumberHigh, write = SetTunnelLocalPortNumberHigh };
  __property int CacheDirectoryChangesMaxSize = { read = FCacheDirectoryChangesMaxSize, write = SetCacheDirectoryChangesMaxSize };
  __property bool ShowFtpWelcomeMessage = { read = FShowFtpWelcomeMessage, write = SetShowFtpWelcomeMessage };

  __property AnsiString TimeFormat = { read = GetTimeFormat };
  __property TStorage Storage  = { read=GetStorage, write=SetStorage };
  __property AnsiString RegistryStorageKey  = { read=GetRegistryStorageKey };
  __property AnsiString IniFileStorageName  = { read=GetIniFileStorageName, write=SetIniFileStorageName };
  __property AnsiString DefaultKeyFile = { read = GetDefaultKeyFile };

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
