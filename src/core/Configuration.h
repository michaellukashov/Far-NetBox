
#pragma once

#include "RemoteFiles.h"
#include "FileBuffer.h"
#include "HierarchicalStorage.h"

#define SET_CONFIG_PROPERTY_EX(PROPERTY, APPLY) \
  if (Get ## PROPERTY() != Value) { F ## PROPERTY = Value; Changed(); APPLY; }
#define SET_CONFIG_PROPERTY(PROPERTY) \
  SET_CONFIG_PROPERTY_EX(PROPERTY, )

#define CONST_DEFAULT_NUMBER_OF_RETRIES 2

enum TAutoSwitch
{
  asOn,
  asOff,
  asAuto
};

enum TFtpEncryptionSwitch_219
{
  fesPlainFTP,
  fesExplicitSSL,
  fesImplicit,
  fesExplicitTLS
};

class TConfiguration : public TObject
{
NB_DECLARE_CLASS(TConfiguration)
public:
  UnicodeString GetOSVersionStr() const;
  TVSFixedFileInfo *GetFixedApplicationInfo() const;
  void * GetApplicationInfo() const;
  virtual UnicodeString GetVersionStr() const;
  virtual UnicodeString GetVersion() const;
  UnicodeString GetProductVersion() const;
  UnicodeString GetProductName() const;
  UnicodeString GetCompanyName() const;
  UnicodeString GetStoredSessionsSubKey() const;
  UnicodeString GetPuttySessionsKey() const;
  void SetRandomSeedFile(const UnicodeString & Value);
  UnicodeString GetRandomSeedFileName() const;
  void SetPuttyRegistryStorageKey(const UnicodeString & Value);
  UnicodeString GetSshHostKeysSubKey() const;
  UnicodeString GetRootKeyStr() const;
  UnicodeString GetConfigurationSubKey() const;
  TEOLType GetLocalEOLType() const;
  void SetLogging(bool Value);
  void SetLogFileName(const UnicodeString & Value);
  bool GetLogToFile() const;
  void SetLogWindowLines(intptr_t Value);
  void SetLogWindowComplete(bool Value);
  bool GetLogWindowComplete() const;
  void SetLogFileAppend(bool Value);
  void SetLogProtocol(intptr_t Value);
  void SetLogActions(bool Value);
  void SetActionsLogFileName(const UnicodeString & Value);
  UnicodeString GetDefaultLogFileName() const;
  UnicodeString GetTimeFormat() const;
  void SetStorage(TStorage Value);
  UnicodeString GetRegistryStorageKey() const;
//  UnicodeString GetIniFileStorageNameForReadingWriting() const;
//  UnicodeString GetIniFileStorageNameForReading();
//  UnicodeString GetIniFileStorageName(bool ReadingOnly);
  void SetIniFileStorageName(const UnicodeString & Value);
  UnicodeString GetPartialExt() const;
  UnicodeString GetFileInfoString(const UnicodeString & Key) const;
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
  void SetExternalIpAddress(const UnicodeString & Value);
  void SetTryFtpWhenSshFails(bool Value);
  bool GetCollectUsage() const;
  void SetCollectUsage(bool);
  bool GetIsUnofficial() const;

public:
  virtual TStorage GetStorage();
  virtual void Changed();
  virtual void SaveData(THierarchicalStorage * Storage, bool All);
  virtual void LoadData(THierarchicalStorage * Storage);
  virtual void LoadFrom(THierarchicalStorage * Storage);
  virtual void CopyData(THierarchicalStorage * Source, THierarchicalStorage * Target);
  virtual void LoadAdmin(THierarchicalStorage * Storage);
  virtual UnicodeString GetDefaultKeyFile() const;
  virtual void Saved();
  void CleanupRegistry(const UnicodeString & CleanupSubKey);
  UnicodeString BannerHash(const UnicodeString & Banner) const;
  static UnicodeString PropertyToKey(const UnicodeString & Property);
  virtual void DoSave(bool All, bool Explicit);

  virtual bool GetConfirmOverwriting() const;
  virtual void SetConfirmOverwriting(bool Value);
  bool GetConfirmResume() const;
  void SetConfirmResume(bool Value);
  bool GetAutoReadDirectoryAfterOp() const;
  void SetAutoReadDirectoryAfterOp(bool Value);
  virtual bool GetRememberPassword() const;

  virtual UnicodeString ModuleFileName() const;

  UnicodeString GetFileFileInfoString(const UnicodeString & AKey,
    const UnicodeString & AFileName, bool AllowEmpty = false) const;
  void * GetFileApplicationInfo(const UnicodeString & AFileName) const;
  UnicodeString GetFileProductVersion(const UnicodeString & AFileName) const;
  UnicodeString GetFileProductName(const UnicodeString & AFileName) const;
  UnicodeString GetFileCompanyName(const UnicodeString & AFileName) const;

  bool GetPermanentLogging() const { return FPermanentLogging; }
  void SetPermanentLogging(bool Value) { FPermanentLogging = Value; }
  UnicodeString GetPermanentLogFileName() const;
  void SetPermanentLogFileName(const UnicodeString & Value);
  bool GetPermanentLogActions() const { return FPermanentLogActions; }
  void SetPermanentLogActions(bool Value) { FPermanentLogActions = Value; }
  UnicodeString GetPermanentActionsLogFileName() const;
  void SetPermanentActionsLogFileName(const UnicodeString & Value);

public:
  TConfiguration();
  virtual ~TConfiguration();

  virtual void Default();
  virtual void UpdateStaticUsage();
  void Load();
  void Save();
  void SaveExplicit();
  void SetNulStorage();
  void SetDefaultStorage();
  void Export(const UnicodeString & AFileName);
  void Import(const UnicodeString & AFileName);
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
  UnicodeString GetFileDescription(const UnicodeString & AFileName);

  // TUsage * GetUsage() { return FUsage; }
  UnicodeString GetPuttyRegistryStorageKey() const { return FPuttyRegistryStorageKey; }
  UnicodeString GetRandomSeedFile() const { return FRandomSeedFile; }
  bool GetLogging() const { return FLogging; }
  UnicodeString GetLogFileName() const { return FLogFileName; }
  bool GetLogFileAppend() const { return FLogFileAppend; }
  intptr_t GetLogProtocol() const { return FLogProtocol; }
  intptr_t GetActualLogProtocol() const { return FActualLogProtocol; }
  bool GetLogActions() const { return FLogActions; }
  UnicodeString GetActionsLogFileName() const { return FActionsLogFileName; }
  intptr_t GetLogWindowLines() const { return FLogWindowLines; }
  TNotifyEvent & GetOnChange() { return FOnChange; }
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
  bool GetDisablePasswordStoring() const { return FDisablePasswordStoring; }
  bool GetForceBanners() const { return FForceBanners; }
  bool GetDisableAcceptingHostKeys() const { return FDisableAcceptingHostKeys; }
  void SetLogToFile(bool Value);
  intptr_t GetSessionReopenAutoMaximumNumberOfRetries() const { return FSessionReopenAutoMaximumNumberOfRetries; }
  void SetSessionReopenAutoMaximumNumberOfRetries(intptr_t Value);

protected:
  TStorage FStorage;
  TCriticalSection FCriticalSection;

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
  std::unique_ptr<TStrings> FOptionsStorage;
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
};

class TShortCuts : public TObject
{
public:
  void Add(const TShortCut & ShortCut);
  bool Has(const TShortCut & ShortCut) const;

private:
  rde::vector<TShortCut> FShortCuts;
};

