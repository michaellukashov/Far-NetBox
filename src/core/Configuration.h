//---------------------------------------------------------------------------
#ifndef ConfigurationH
#define ConfigurationH

#include <set>
#include "RemoteFiles.h"
#include "FileBuffer.h"
#include "HierarchicalStorage.h"
// #include "Usage.h"
//---------------------------------------------------------------------------
#define SET_CONFIG_PROPERTY_EX(PROPERTY, APPLY) \
  if (Get ## PROPERTY() != Value) { F ## PROPERTY = Value; Changed(); APPLY; }
#define SET_CONFIG_PROPERTY(PROPERTY) \
  SET_CONFIG_PROPERTY_EX(PROPERTY, )
//---------------------------------------------------------------------------
#define CONST_DEFAULT_NUMBER_OF_RETRIES 2
//---------------------------------------------------------------------------
enum TAutoSwitch { asOn, asOff, asAuto };
enum TFtpEncryptionSwitch_219 { fesPlainFTP, fesExplicitSSL, fesImplicit, fesExplicitTLS };
//---------------------------------------------------------------------------
class TConfiguration : public TObject
{
private:
  bool FDontSave;
  bool FChanged;
  int FUpdating;
  TNotifyEvent FOnChange;

  void * FApplicationInfo;
  // TUsage * FUsage;
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
  int FSessionReopenBackground;
  int FSessionReopenTimeout;
  int FSessionReopenAutoStall;
  UnicodeString FIniFileStorageName;
  UnicodeString FVirtualIniFileStorageName;
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
  bool FDefaultCollectUsage;
  int FSessionReopenAutoMaximumNumberOfRetries;

public:
  UnicodeString GetOSVersionStr();
  TVSFixedFileInfo *GetFixedApplicationInfo();
  void * GetApplicationInfo();
  virtual UnicodeString GetVersionStr();
  virtual UnicodeString GetVersion();
  UnicodeString GetProductVersion();
  UnicodeString GetProductName();
  UnicodeString GetCompanyName();
  UnicodeString TrimVersion(const UnicodeString & Version);
  UnicodeString GetStoredSessionsSubKey();
  UnicodeString GetPuttySessionsKey();
  void SetRandomSeedFile(const UnicodeString & Value);
  UnicodeString GetRandomSeedFileName();
  void SetPuttyRegistryStorageKey(const UnicodeString & Value);
  UnicodeString GetSshHostKeysSubKey();
  UnicodeString GetRootKeyStr();
  UnicodeString GetConfigurationSubKey();
  TEOLType GetLocalEOLType();
  void SetLogging(bool Value);
  void SetLogFileName(const UnicodeString & Value);
  bool GetLogToFile();
  void SetLogWindowLines(int Value);
  void SetLogWindowComplete(bool Value);
  bool GetLogWindowComplete();
  void SetLogFileAppend(bool Value);
  void SetLogProtocol(int Value);
  void SetLogActions(bool Value);
  void SetActionsLogFileName(const UnicodeString & Value);
  UnicodeString GetDefaultLogFileName();
  UnicodeString GetTimeFormat();
  void SetStorage(TStorage Value);
  UnicodeString GetRegistryStorageKey();
  UnicodeString GetIniFileStorageName();
  void SetIniFileStorageName(const UnicodeString & Value);
  UnicodeString GetPartialExt() const;
  UnicodeString GetFileInfoString(const UnicodeString & Key);
  bool GetGSSAPIInstalled();
  void SetSessionReopenAuto(int Value);
  void SetSessionReopenBackground(int Value);
  void SetSessionReopenTimeout(int Value);
  void SetSessionReopenAutoStall(int Value);
  void SetTunnelLocalPortNumberLow(int Value);
  void SetTunnelLocalPortNumberHigh(int Value);
  void SetCacheDirectoryChangesMaxSize(int Value);
  void SetShowFtpWelcomeMessage(bool Value);
  int GetCompoundVersion();
  void UpdateActualLogProtocol();
  void SetExternalIpAddress(const UnicodeString & Value);
  bool GetCollectUsage();
  void SetCollectUsage(bool Value);

protected:
  TStorage FStorage;
  TCriticalSection * FCriticalSection;

public:
  virtual TStorage GetStorage();
  virtual void Changed();
  virtual void SaveData(THierarchicalStorage * Storage, bool All);
  virtual void LoadData(THierarchicalStorage * Storage);
  virtual void CopyData(THierarchicalStorage * Source, THierarchicalStorage * Target);
  virtual void LoadAdmin(THierarchicalStorage * Storage);
  virtual UnicodeString GetDefaultKeyFile();
  virtual void Saved();
  void CleanupRegistry(const UnicodeString & CleanupSubKey);
  UnicodeString BannerHash(const UnicodeString & Banner);

  virtual bool GetConfirmOverwriting();
  virtual void SetConfirmOverwriting(bool Value);
  bool GetConfirmResume();
  void SetConfirmResume(bool Value);
  bool GetAutoReadDirectoryAfterOp();
  void SetAutoReadDirectoryAfterOp(bool Value);
  virtual bool GetRememberPassword();

  virtual UnicodeString ModuleFileName();

  UnicodeString GetFileFileInfoString(const UnicodeString & Key,
    const UnicodeString & FileName);
  void * GetFileApplicationInfo(const UnicodeString & FileName);
  UnicodeString GetFileProductVersion(const UnicodeString & FileName);
  UnicodeString GetFileProductName(const UnicodeString & FileName);
  UnicodeString GetFileCompanyName(const UnicodeString & FileName);

  bool GetPermanentLogging() { return FPermanentLogging; }
  void SetPermanentLogging(bool Value) { FPermanentLogging = Value; }
  UnicodeString GetPermanentLogFileName();
  void SetPermanentLogFileName(const UnicodeString & Value);
  bool GetPermanentLogActions() { return FPermanentLogActions; }
  void SetPermanentLogActions(bool Value) { FPermanentLogActions = Value; }
  UnicodeString GetPermanentActionsLogFileName();
  void SetPermanentActionsLogFileName(const UnicodeString & Value);

public:
  TConfiguration();
  virtual ~TConfiguration();

  virtual void Default();
  virtual void Load();
  virtual void Save(bool All, bool Explicit);
  void SetNulStorage();
  void SetDefaultStorage();
  void Export(const UnicodeString & FileName);
  void CleanupConfiguration();
  void CleanupIniFile();
  void CleanupHostKeys();
  void CleanupRandomSeedFile();
  void BeginUpdate();
  void EndUpdate();
  void LoadDirectoryChangesCache(const UnicodeString & SessionKey,
    TRemoteDirectoryChangesCache * DirectoryChangesCache);
  void SaveDirectoryChangesCache(const UnicodeString & SessionKey,
    TRemoteDirectoryChangesCache * DirectoryChangesCache);
  bool ShowBanner(const UnicodeString & SessionKey, const UnicodeString & Banner);
  void NeverShowBanner(const UnicodeString & SessionKey, const UnicodeString & Banner);
  virtual THierarchicalStorage * CreateScpStorage(bool SessionList);
  void TemporaryLogging(const UnicodeString & ALogFileName);
  void TemporaryActionsLogging(const UnicodeString & ALogFileName);
  virtual RawByteString EncryptPassword(const UnicodeString & Password, const UnicodeString & Key);
  virtual UnicodeString DecryptPassword(const RawByteString & Password, const UnicodeString & Key);
  virtual RawByteString StronglyRecryptPassword(const RawByteString & Password, const UnicodeString & Key);

  // TUsage * GetUsage() { return FUsage; };
  UnicodeString GetPuttyRegistryStorageKey() { return FPuttyRegistryStorageKey; }
  UnicodeString GetRandomSeedFile() const { return FRandomSeedFile; }
  bool GetLogging() { return FLogging; }
  UnicodeString GetLogFileName() { return FLogFileName; }
  bool GetLogFileAppend() { return FLogFileAppend; }
  int GetLogProtocol() { return FLogProtocol; }
  int GetActualLogProtocol() { return FActualLogProtocol; }
  bool GetLogActions() { return FLogActions; }
  UnicodeString GetActionsLogFileName() const { return FActionsLogFileName; }
  int GetLogWindowLines() { return FLogWindowLines; }
  TNotifyEvent & GetOnChange() { return FOnChange; }
  void SetOnChange(TNotifyEvent Value) { FOnChange = Value; }
  int GetSessionReopenAuto() { return FSessionReopenAuto; }
  int GetSessionReopenBackground() { return FSessionReopenBackground; }
  int GetSessionReopenTimeout() { return FSessionReopenTimeout; }
  int GetSessionReopenAutoStall() { return FSessionReopenAutoStall; }
  int GetTunnelLocalPortNumberLow() { return FTunnelLocalPortNumberLow; }
  int GetTunnelLocalPortNumberHigh() { return FTunnelLocalPortNumberHigh; }
  int GetCacheDirectoryChangesMaxSize() { return FCacheDirectoryChangesMaxSize; }
  bool GetShowFtpWelcomeMessage() { return FShowFtpWelcomeMessage; }
  UnicodeString GetExternalIpAddress() const { return FExternalIpAddress; }
  bool GetDisablePasswordStoring() { return FDisablePasswordStoring; }
  bool GetForceBanners() { return FForceBanners; }
  bool GetDisableAcceptingHostKeys() { return FDisableAcceptingHostKeys; }
  void SetLogToFile(bool Value);
  int GetSessionReopenAutoMaximumNumberOfRetries() const { return FSessionReopenAutoMaximumNumberOfRetries; }
  void SetSessionReopenAutoMaximumNumberOfRetries(int Value);
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
#endif
