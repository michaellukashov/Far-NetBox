//---------------------------------------------------------------------------
#ifndef ConfigurationH
#define ConfigurationH

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
class TConfiguration : public TObject
{
private:
  bool FDontSave;
  bool FChanged;
  int FUpdating;
  notify_signal_type FOnChange;

  void * FApplicationInfo;
  bool FLogging;
  bool FPermanentLogging;
  UnicodeString FLogFileName;
  UnicodeString FPermanentLogFileName;
  int FLogWindowLines;
  bool FLogFileAppend;
  int FLogProtocol;
  int FActualLogProtocol;
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
  int FSessionReopenTimeout;
  int FSessionReopenAutoStall;
  UnicodeString FIniFileStorageName;
  int FTunnelLocalPortNumberLow;
  int FTunnelLocalPortNumberHigh;
  int FCacheDirectoryChangesMaxSize;
  bool FShowFtpWelcomeMessage;
  UnicodeString FDefaultRandomSeedFile;
  UnicodeString FRandomSeedFile;
  UnicodeString FPuttyRegistryStorageKey;
  UnicodeString FExternalIpAddress;

  bool FDisablePasswordStoring;
  bool FForceBanners;
  bool FDisableAcceptingHostKeys;

public:
  UnicodeString __fastcall GetOSVersionStr();
  VS_FIXEDFILEINFO __fastcall GetFixedApplicationInfo();
  void * __fastcall GetApplicationInfo();
  virtual UnicodeString __fastcall GetVersionStr();
  virtual UnicodeString __fastcall GetVersion();
  UnicodeString __fastcall GetProductVersion();
  UnicodeString __fastcall GetProductName();
  UnicodeString __fastcall GetCompanyName();
  UnicodeString __fastcall TrimVersion(UnicodeString Version);
  UnicodeString __fastcall GetStoredSessionsSubKey();
  UnicodeString __fastcall GetPuttySessionsKey();
  void __fastcall SetRandomSeedFile(UnicodeString value);
  UnicodeString __fastcall GetRandomSeedFileName();
  void __fastcall SetPuttyRegistryStorageKey(UnicodeString value);
  UnicodeString __fastcall GetSshHostKeysSubKey();
  UnicodeString __fastcall GetRootKeyStr();
  UnicodeString __fastcall GetConfigurationSubKey();
  TEOLType __fastcall GetLocalEOLType();
  void __fastcall SetLogging(bool value);
  void __fastcall SetLogFileName(UnicodeString value);
  bool __fastcall GetLogToFile();
  void __fastcall SetLogWindowLines(int value);
  void __fastcall SetLogWindowComplete(bool value);
  bool __fastcall GetLogWindowComplete();
  void __fastcall SetLogFileAppend(bool value);
  void __fastcall SetLogProtocol(int value);
  void __fastcall SetLogActions(bool value);
  void __fastcall SetActionsLogFileName(UnicodeString value);
  UnicodeString __fastcall GetDefaultLogFileName();
  UnicodeString __fastcall GetTimeFormat();
  void __fastcall SetStorage(TStorage value);
  UnicodeString __fastcall GetRegistryStorageKey();
  UnicodeString __fastcall GetIniFileStorageName();
  void __fastcall SetIniFileStorageName(UnicodeString value);
  UnicodeString __fastcall GetPartialExt() const;
  UnicodeString __fastcall GetFileInfoString(const UnicodeString Key);
  bool __fastcall GetGSSAPIInstalled();
  void __fastcall SetSessionReopenAuto(int value);
  void __fastcall SetSessionReopenBackground(int value);
  void __fastcall SetSessionReopenTimeout(int value);
  void __fastcall SetSessionReopenAutoStall(int value);
  void __fastcall SetTunnelLocalPortNumberLow(int value);
  void __fastcall SetTunnelLocalPortNumberHigh(int value);
  void __fastcall SetCacheDirectoryChangesMaxSize(int value);
  void __fastcall SetShowFtpWelcomeMessage(bool value);
  int __fastcall GetCompoundVersion();
  void __fastcall UpdateActualLogProtocol();
  void __fastcall SetExternalIpAddress(UnicodeString value);

protected:
  TStorage FStorage;
  TCriticalSection * FCriticalSection;

public:
  virtual TStorage __fastcall GetStorage();
  virtual void __fastcall Changed();
  virtual void __fastcall SaveData(THierarchicalStorage * Storage, bool All);
  virtual void __fastcall LoadData(THierarchicalStorage * Storage);
  virtual void __fastcall CopyData(THierarchicalStorage * Source, THierarchicalStorage * Target);
  virtual void __fastcall LoadAdmin(THierarchicalStorage * Storage);
  virtual UnicodeString __fastcall GetDefaultKeyFile();
  virtual void __fastcall Saved();
  void __fastcall CleanupRegistry(UnicodeString CleanupSubKey);
  UnicodeString __fastcall BannerHash(const UnicodeString & Banner);

  virtual bool __fastcall GetConfirmOverwriting();
  virtual void __fastcall SetConfirmOverwriting(bool value);
  bool __fastcall GetConfirmResume();
  void __fastcall SetConfirmResume(bool value);
  bool __fastcall GetAutoReadDirectoryAfterOp();
  void __fastcall SetAutoReadDirectoryAfterOp(bool value);
  virtual bool __fastcall GetRememberPassword();

  virtual UnicodeString __fastcall ModuleFileName();

  UnicodeString __fastcall GetFileFileInfoString(const UnicodeString Key,
    const UnicodeString FileName);
  void * __fastcall GetFileApplicationInfo(const UnicodeString FileName);
  UnicodeString __fastcall GetFileProductVersion(const UnicodeString FileName);
  UnicodeString __fastcall GetFileProductName(const UnicodeString FileName);
  UnicodeString __fastcall GetFileCompanyName(const UnicodeString FileName);

#ifndef _MSC_VER
  __property bool PermanentLogging  = { read=FPermanentLogging, write=SetLogging };
  __property UnicodeString PermanentLogFileName  = { read=FPermanentLogFileName, write=SetLogFileName };
  __property bool PermanentLogActions  = { read=FPermanentLogActions, write=SetLogActions };
  __property UnicodeString PermanentActionsLogFileName  = { read=FPermanentActionsLogFileName, write=SetActionsLogFileName };
#else
  bool __fastcall GetPermanentLogging() { return FPermanentLogging; }
  void __fastcall SetPermanentLogging(bool value) { FPermanentLogging = value; }
  bool __fastcall GetPermanentLogActions() { return FPermanentLogActions; }
  void __fastcall SetPermanentLogActions(bool value) { FPermanentLogActions = value; }
  UnicodeString __fastcall GetPermanentLogFileName() { return FPermanentLogFileName; }
  void __fastcall SetPermanentLogFileName(const UnicodeString value) { FPermanentLogFileName = value; }
  UnicodeString __fastcall GetPermanentActionsLogFileName() { return FPermanentActionsLogFileName; }
  void __fastcall SetPermanentActionsLogFileName(const UnicodeString value) { FPermanentActionsLogFileName = value; }
#endif

public:
  /* __fastcall */ TConfiguration();
  virtual /* __fastcall */ ~TConfiguration();

  virtual void __fastcall Default();
  virtual void __fastcall Load();
  virtual void __fastcall Save(bool All, bool Explicit);
  void __fastcall SetNulStorage();
  void __fastcall SetDefaultStorage();
  void __fastcall Export(const UnicodeString FileName);
  void __fastcall CleanupConfiguration();
  void __fastcall CleanupIniFile();
  void __fastcall CleanupHostKeys();
  void __fastcall CleanupRandomSeedFile();
  void __fastcall BeginUpdate();
  void __fastcall EndUpdate();
  void __fastcall LoadDirectoryChangesCache(const UnicodeString SessionKey,
    TRemoteDirectoryChangesCache * DirectoryChangesCache);
  void __fastcall SaveDirectoryChangesCache(const UnicodeString SessionKey,
    TRemoteDirectoryChangesCache * DirectoryChangesCache);
  bool __fastcall ShowBanner(const UnicodeString SessionKey, const UnicodeString & Banner);
  void __fastcall NeverShowBanner(const UnicodeString SessionKey, const UnicodeString & Banner);
  virtual THierarchicalStorage * __fastcall CreateScpStorage(bool SessionList);
  void __fastcall TemporaryLogging(const UnicodeString ALogFileName);
  void __fastcall TemporaryActionsLogging(const UnicodeString ALogFileName);
  virtual RawByteString __fastcall EncryptPassword(const UnicodeString & Password, const UnicodeString & Key);
  virtual UnicodeString __fastcall DecryptPassword(RawByteString Password, UnicodeString Key);
  virtual RawByteString __fastcall StronglyRecryptPassword(RawByteString Password, UnicodeString Key);

#ifndef _MSC_VER
  __property TVSFixedFileInfo *FixedApplicationInfo  = { read=GetFixedApplicationInfo };
  __property void * ApplicationInfo  = { read=GetApplicationInfo };
  __property UnicodeString StoredSessionsSubKey = {read=GetStoredSessionsSubKey};
  __property UnicodeString PuttyRegistryStorageKey  = { read=FPuttyRegistryStorageKey, write=SetPuttyRegistryStorageKey };
  __property UnicodeString PuttySessionsKey  = { read=GetPuttySessionsKey };
  __property UnicodeString RandomSeedFile  = { read=FRandomSeedFile, write=SetRandomSeedFile };
  __property UnicodeString RandomSeedFileName  = { read=GetRandomSeedFileName };
  __property UnicodeString SshHostKeysSubKey  = { read=GetSshHostKeysSubKey };
  __property UnicodeString RootKeyStr  = { read=GetRootKeyStr };
  __property UnicodeString ConfigurationSubKey  = { read=GetConfigurationSubKey };
  __property TEOLType LocalEOLType = { read = GetLocalEOLType };
  __property UnicodeString VersionStr = { read=GetVersionStr };
  __property UnicodeString Version = { read=GetVersion };
  __property int CompoundVersion = { read=GetCompoundVersion };
  __property UnicodeString ProductVersion = { read=GetProductVersion };
  __property UnicodeString ProductName = { read=GetProductName };
  __property UnicodeString CompanyName = { read=GetCompanyName };
  __property UnicodeString FileInfoString[UnicodeString Key] = { read = GetFileInfoString };
  __property UnicodeString OSVersionStr = { read = GetOSVersionStr };
  __property bool Logging  = { read=FLogging, write=SetLogging };
  __property UnicodeString LogFileName  = { read=FLogFileName, write=SetLogFileName };
  __property bool LogToFile  = { read=GetLogToFile };
  __property bool LogFileAppend  = { read=FLogFileAppend, write=SetLogFileAppend };
  __property int LogProtocol  = { read=FLogProtocol, write=SetLogProtocol };
  __property int ActualLogProtocol  = { read=FActualLogProtocol };
  __property bool LogActions  = { read=FLogActions, write=SetLogActions };
  __property UnicodeString ActionsLogFileName  = { read=FActionsLogFileName, write=SetActionsLogFileName };
  __property int LogWindowLines  = { read=FLogWindowLines, write=SetLogWindowLines };
  __property bool LogWindowComplete  = { read=GetLogWindowComplete, write=SetLogWindowComplete };
  __property UnicodeString DefaultLogFileName  = { read=GetDefaultLogFileName };
  __property TNotifyEvent OnChange = { read = FOnChange, write = FOnChange };
  __property bool ConfirmOverwriting = { read = GetConfirmOverwriting, write = SetConfirmOverwriting};
  __property bool ConfirmResume = { read = GetConfirmResume, write = SetConfirmResume};
  __property bool AutoReadDirectoryAfterOp = { read = GetAutoReadDirectoryAfterOp, write = SetAutoReadDirectoryAfterOp};
  __property bool RememberPassword = { read = GetRememberPassword };
  __property UnicodeString PartialExt = {read=GetPartialExt};
  __property int SessionReopenAuto = { read = FSessionReopenAuto, write = SetSessionReopenAuto };
  __property int SessionReopenBackground = { read = FSessionReopenBackground, write = SetSessionReopenBackground };
  __property int SessionReopenTimeout = { read = FSessionReopenTimeout, write = SetSessionReopenTimeout };
  __property int SessionReopenAutoStall = { read = FSessionReopenAutoStall, write = SetSessionReopenAutoStall };
  __property int TunnelLocalPortNumberLow = { read = FTunnelLocalPortNumberLow, write = SetTunnelLocalPortNumberLow };
  __property int TunnelLocalPortNumberHigh = { read = FTunnelLocalPortNumberHigh, write = SetTunnelLocalPortNumberHigh };
  __property int CacheDirectoryChangesMaxSize = { read = FCacheDirectoryChangesMaxSize, write = SetCacheDirectoryChangesMaxSize };
  __property bool ShowFtpWelcomeMessage = { read = FShowFtpWelcomeMessage, write = SetShowFtpWelcomeMessage };
  __property UnicodeString ExternalIpAddress = { read = FExternalIpAddress, write = SetExternalIpAddress };

  __property UnicodeString TimeFormat = { read = GetTimeFormat };
  __property TStorage Storage  = { read=GetStorage, write=SetStorage };
  __property UnicodeString RegistryStorageKey  = { read=GetRegistryStorageKey };
  __property UnicodeString IniFileStorageName  = { read=GetIniFileStorageName, write=SetIniFileStorageName };
  __property UnicodeString DefaultKeyFile = { read = GetDefaultKeyFile };

  __property bool DisablePasswordStoring = { read = FDisablePasswordStoring };
  __property bool ForceBanners = { read = FForceBanners };
  __property bool DisableAcceptingHostKeys = { read = FDisableAcceptingHostKeys };
  __property bool GSSAPIInstalled = { read = GetGSSAPIInstalled };
#else
  UnicodeString __fastcall GetActionsLogFileName() const { return FActionsLogFileName; }
  UnicodeString __fastcall GetRandomSeedFile() const { return FRandomSeedFile; }
  int __fastcall GetSessionReopenAutoStall() { return FSessionReopenAutoStall; }

  UnicodeString GetPuttyRegistryStorageKey() { return FPuttyRegistryStorageKey; }
  bool GetLogging() { return FLogging; }
  UnicodeString __fastcall GetLogFileName() { return FLogFileName; }
  void __fastcall SetLogToFile(bool value);
  bool GetLogFileAppend() { return FLogFileAppend; }
  size_t GetLogProtocol() { return FLogProtocol; }
  size_t GetActualLogProtocol() { return FActualLogProtocol; }
  bool GetLogActions() { return FLogActions; }
  int GetLogWindowLines() { return FLogWindowLines; }
  notify_signal_type & GetOnChange() { return FOnChange; }
  void SetOnChange(const TNotifyEvent & value) { FOnChange.connect(value); }
  int GetSessionReopenAuto() { return FSessionReopenAuto; }
  int __fastcall GetSessionReopenAutoMaximumNumberOfRetries() const { return FSessionReopenAutoMaximumNumberOfRetries; }
  void __fastcall SetSessionReopenAutoMaximumNumberOfRetries(int value);
  int GetSessionReopenBackground() { return FSessionReopenBackground; }
  size_t GetSessionReopenTimeout() { return FSessionReopenTimeout; }
  size_t GetTunnelLocalPortNumberLow() { return FTunnelLocalPortNumberLow; }
  size_t GetTunnelLocalPortNumberHigh() { return FTunnelLocalPortNumberHigh; }
  int GetCacheDirectoryChangesMaxSize() { return FCacheDirectoryChangesMaxSize; }
  bool GetShowFtpWelcomeMessage() { return FShowFtpWelcomeMessage; }
  UnicodeString GetExternalIpAddress() const { return FExternalIpAddress; }

  bool GetDisablePasswordStoring() { return FDisablePasswordStoring; }
  bool GetForceBanners() { return FForceBanners; }
  bool GetDisableAcceptingHostKeys() { return FDisableAcceptingHostKeys; }
#endif
};
//---------------------------------------------------------------------------
class TShortCuts
{
public:
  void __fastcall Add(TShortCut ShortCut);
  bool __fastcall Has(TShortCut ShortCut) const;

private:
  std::set<TShortCut> FShortCuts;
};
//---------------------------------------------------------------------------
#endif
