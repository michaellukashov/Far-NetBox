//---------------------------------------------------------------------------
#ifndef ConfigurationH
#define ConfigurationH

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
  intptr_t FUpdating;
  TNotifyEvent FOnChange;

  mutable void * FApplicationInfo;
  // TUsage * FUsage;
  bool FLogging;
  bool FPermanentLogging;
  UnicodeString FLogFileName;
  UnicodeString FPermanentLogFileName;
  intptr_t FLogWindowLines;
  bool FLogFileAppend;
  intptr_t FLogProtocol;
  intptr_t FActualLogProtocol;
  bool FLogActions;
  bool FPermanentLogActions;
  UnicodeString FActionsLogFileName;
  UnicodeString FPermanentActionsLogFileName;
  bool FConfirmOverwriting;
  bool FConfirmResume;
  bool FAutoReadDirectoryAfterOp;
  intptr_t FSessionReopenAuto;
  intptr_t FSessionReopenBackground;
  intptr_t FSessionReopenTimeout;
  intptr_t FSessionReopenAutoStall;
  UnicodeString FIniFileStorageName;
  UnicodeString FVirtualIniFileStorageName;
  intptr_t FProgramIniPathWrittable;
  intptr_t FTunnelLocalPortNumberLow;
  intptr_t FTunnelLocalPortNumberHigh;
  intptr_t FCacheDirectoryChangesMaxSize;
  bool FShowFtpWelcomeMessage;
  UnicodeString FDefaultRandomSeedFile;
  UnicodeString FRandomSeedFile;
  UnicodeString FPuttyRegistryStorageKey;
  UnicodeString FExternalIpAddress;
  bool FTryFtpWhenSshFails;

  bool FDisablePasswordStoring;
  bool FForceBanners;
  bool FDisableAcceptingHostKeys;
  bool FDefaultCollectUsage;
  intptr_t FSessionReopenAutoMaximumNumberOfRetries;

public:
  UnicodeString GetOSVersionStr() const;
  TVSFixedFileInfo *GetFixedApplicationInfo() const;
  void * GetApplicationInfo() const;
  virtual UnicodeString GetVersionStr() const;
  virtual UnicodeString GetVersion() const;
  UnicodeString GetProductVersion() const;
  UnicodeString GetProductName() const;
  UnicodeString GetCompanyName() const;
  UnicodeString TrimVersion(const UnicodeString & Version) const;
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
  void SetLogWindowLines(intptr_t Value);
  void SetLogWindowComplete(bool Value);
  bool GetLogWindowComplete();
  void SetLogFileAppend(bool Value);
  void SetLogProtocol(intptr_t Value);
  void SetLogActions(bool Value);
  void SetActionsLogFileName(const UnicodeString & Value);
  UnicodeString GetDefaultLogFileName();
  UnicodeString GetTimeFormat();
  void SetStorage(TStorage Value);
  UnicodeString GetRegistryStorageKey();
  UnicodeString GetIniFileStorageNameForReadingWritting();
  UnicodeString GetIniFileStorageNameForReading();
  UnicodeString GetIniFileStorageName(bool ReadingOnly);
  void SetIniFileStorageName(const UnicodeString & Value);
  UnicodeString GetPartialExt() const;
  UnicodeString GetFileInfoString(const UnicodeString & Key);
  bool GetGSSAPIInstalled();
  void SetSessionReopenAuto(intptr_t Value);
  void SetSessionReopenBackground(intptr_t Value);
  void SetSessionReopenTimeout(intptr_t Value);
  void SetSessionReopenAutoStall(intptr_t Value);
  void SetTunnelLocalPortNumberLow(intptr_t Value);
  void SetTunnelLocalPortNumberHigh(intptr_t Value);
  void SetCacheDirectoryChangesMaxSize(intptr_t Value);
  void SetShowFtpWelcomeMessage(bool Value);
  intptr_t GetCompoundVersion();
  void UpdateActualLogProtocol();
  void SetExternalIpAddress(const UnicodeString & Value);
  void SetTryFtpWhenSshFails(bool Value);
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
  virtual void LoadFrom(THierarchicalStorage * Storage);
  virtual void CopyData(THierarchicalStorage * Source, THierarchicalStorage * Target);
  virtual void LoadAdmin(THierarchicalStorage * Storage);
  virtual UnicodeString GetDefaultKeyFile();
  virtual void Saved();
  void CleanupRegistry(const UnicodeString & CleanupSubKey);
  UnicodeString BannerHash(const UnicodeString & Banner);
  static UnicodeString PropertyToKey(const UnicodeString & Property);

  virtual bool GetConfirmOverwriting();
  virtual void SetConfirmOverwriting(bool Value);
  bool GetConfirmResume();
  void SetConfirmResume(bool Value);
  bool GetAutoReadDirectoryAfterOp();
  void SetAutoReadDirectoryAfterOp(bool Value);
  virtual bool GetRememberPassword();

  virtual UnicodeString ModuleFileName() const;

  UnicodeString GetFileInfoString(const UnicodeString & Key,
    const UnicodeString & FileName) const;
  void * GetFileApplicationInfo(const UnicodeString & FileName) const;
  UnicodeString GetFileProductVersion(const UnicodeString & FileName) const;
  UnicodeString GetFileProductName(const UnicodeString & FileName) const;
  UnicodeString GetFileCompanyName(const UnicodeString & FileName) const;

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
  void Import(const UnicodeString & FileName);
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
  virtual THierarchicalStorage * CreateStorage(bool SessionList);
  void TemporaryLogging(const UnicodeString & ALogFileName);
  void TemporaryActionsLogging(const UnicodeString & ALogFileName);
  virtual RawByteString EncryptPassword(const UnicodeString & Password, const UnicodeString & Key);
  virtual UnicodeString DecryptPassword(const RawByteString & Password, const UnicodeString & Key);
  virtual RawByteString StronglyRecryptPassword(const RawByteString & Password, const UnicodeString & Key);
  UnicodeString GetFileDescription(const UnicodeString & FileName);

  // TUsage * GetUsage() { return FUsage; };
  UnicodeString GetPuttyRegistryStorageKey() { return FPuttyRegistryStorageKey; }
  UnicodeString GetRandomSeedFile() const { return FRandomSeedFile; }
  bool GetLogging() { return FLogging; }
  UnicodeString GetLogFileName() { return FLogFileName; }
  bool GetLogFileAppend() { return FLogFileAppend; }
  intptr_t GetLogProtocol() { return FLogProtocol; }
  intptr_t GetActualLogProtocol() { return FActualLogProtocol; }
  bool GetLogActions() { return FLogActions; }
  UnicodeString GetActionsLogFileName() const { return FActionsLogFileName; }
  intptr_t GetLogWindowLines() { return FLogWindowLines; }
  TNotifyEvent & GetOnChange() { return FOnChange; }
  void SetOnChange(TNotifyEvent Value) { FOnChange = Value; }
  intptr_t GetSessionReopenAuto() { return FSessionReopenAuto; }
  intptr_t GetSessionReopenBackground() { return FSessionReopenBackground; }
  intptr_t GetSessionReopenTimeout() { return FSessionReopenTimeout; }
  intptr_t GetSessionReopenAutoStall() { return FSessionReopenAutoStall; }
  intptr_t GetTunnelLocalPortNumberLow() { return FTunnelLocalPortNumberLow; }
  intptr_t GetTunnelLocalPortNumberHigh() { return FTunnelLocalPortNumberHigh; }
  intptr_t GetCacheDirectoryChangesMaxSize() { return FCacheDirectoryChangesMaxSize; }
  bool GetShowFtpWelcomeMessage() { return FShowFtpWelcomeMessage; }
  UnicodeString GetExternalIpAddress() const { return FExternalIpAddress; }
  bool GetTryFtpWhenSshFails() { return FTryFtpWhenSshFails; }
  bool GetDisablePasswordStoring() { return FDisablePasswordStoring; }
  bool GetForceBanners() { return FForceBanners; }
  bool GetDisableAcceptingHostKeys() { return FDisableAcceptingHostKeys; }
  void SetLogToFile(bool Value);
  intptr_t GetSessionReopenAutoMaximumNumberOfRetries() const { return FSessionReopenAutoMaximumNumberOfRetries; }
  void SetSessionReopenAutoMaximumNumberOfRetries(intptr_t Value);
};
//---------------------------------------------------------------------------
class TShortCuts : public TObject
{
public:
  void Add(const TShortCut & ShortCut);
  bool Has(const TShortCut & ShortCut) const;

private:
  rde::vector<TShortCut> FShortCuts;
};
//---------------------------------------------------------------------------
#endif
