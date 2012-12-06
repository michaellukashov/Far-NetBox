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
  UnicodeString __fastcall GetOSVersionStr();
  TVSFixedFileInfo *__fastcall GetFixedApplicationInfo();
  void * __fastcall GetApplicationInfo();
  virtual UnicodeString __fastcall GetVersionStr();
  virtual UnicodeString __fastcall GetVersion();
  UnicodeString __fastcall GetProductVersion();
  UnicodeString __fastcall GetProductName();
  UnicodeString __fastcall GetCompanyName();
  UnicodeString __fastcall TrimVersion(UnicodeString Version);
  UnicodeString __fastcall GetStoredSessionsSubKey();
  UnicodeString __fastcall GetPuttySessionsKey();
  void __fastcall SetRandomSeedFile(UnicodeString Value);
  UnicodeString __fastcall GetRandomSeedFileName();
  void __fastcall SetPuttyRegistryStorageKey(UnicodeString Value);
  UnicodeString __fastcall GetSshHostKeysSubKey();
  UnicodeString __fastcall GetRootKeyStr();
  UnicodeString __fastcall GetConfigurationSubKey();
  TEOLType __fastcall GetLocalEOLType();
  void __fastcall SetLogging(bool Value);
  void __fastcall SetLogFileName(UnicodeString Value);
  bool __fastcall GetLogToFile();
  void __fastcall SetLogWindowLines(int Value);
  void __fastcall SetLogWindowComplete(bool Value);
  bool __fastcall GetLogWindowComplete();
  void __fastcall SetLogFileAppend(bool Value);
  void __fastcall SetLogProtocol(int Value);
  void __fastcall SetLogActions(bool Value);
  void __fastcall SetActionsLogFileName(UnicodeString Value);
  UnicodeString __fastcall GetDefaultLogFileName();
  UnicodeString __fastcall GetTimeFormat();
  void __fastcall SetStorage(TStorage Value);
  UnicodeString __fastcall GetRegistryStorageKey();
  UnicodeString __fastcall GetIniFileStorageName();
  void __fastcall SetIniFileStorageName(UnicodeString Value);
  UnicodeString __fastcall GetPartialExt() const;
  UnicodeString __fastcall GetFileInfoString(const UnicodeString Key);
  bool __fastcall GetGSSAPIInstalled();
  void __fastcall SetSessionReopenAuto(int Value);
  void __fastcall SetSessionReopenBackground(int Value);
  void __fastcall SetSessionReopenTimeout(int Value);
  void __fastcall SetSessionReopenAutoStall(int Value);
  void __fastcall SetTunnelLocalPortNumberLow(int Value);
  void __fastcall SetTunnelLocalPortNumberHigh(int Value);
  void __fastcall SetCacheDirectoryChangesMaxSize(int Value);
  void __fastcall SetShowFtpWelcomeMessage(bool Value);
  int __fastcall GetCompoundVersion();
  void __fastcall UpdateActualLogProtocol();
  void __fastcall SetExternalIpAddress(UnicodeString Value);
  bool __fastcall GetCollectUsage();
  void __fastcall SetCollectUsage(bool Value);

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
  virtual void __fastcall SetConfirmOverwriting(bool Value);
  bool __fastcall GetConfirmResume();
  void __fastcall SetConfirmResume(bool Value);
  bool __fastcall GetAutoReadDirectoryAfterOp();
  void __fastcall SetAutoReadDirectoryAfterOp(bool Value);
  virtual bool __fastcall GetRememberPassword();

  virtual UnicodeString __fastcall ModuleFileName();

  UnicodeString __fastcall GetFileFileInfoString(const UnicodeString Key,
    const UnicodeString FileName);
  void * __fastcall GetFileApplicationInfo(const UnicodeString FileName);
  UnicodeString __fastcall GetFileProductVersion(const UnicodeString FileName);
  UnicodeString __fastcall GetFileProductName(const UnicodeString FileName);
  UnicodeString __fastcall GetFileCompanyName(const UnicodeString FileName);

  bool __fastcall GetPermanentLogging() { return FPermanentLogging; }
  void __fastcall SetPermanentLogging(bool Value) { FPermanentLogging = Value; }
  UnicodeString __fastcall GetPermanentLogFileName();
  void __fastcall SetPermanentLogFileName(const UnicodeString Value);
  bool __fastcall GetPermanentLogActions() { return FPermanentLogActions; }
  void __fastcall SetPermanentLogActions(bool Value) { FPermanentLogActions = Value; }
  UnicodeString __fastcall GetPermanentActionsLogFileName();
  void __fastcall SetPermanentActionsLogFileName(const UnicodeString Value);

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
  virtual RawByteString __fastcall EncryptPassword(UnicodeString Password, UnicodeString Key);
  virtual UnicodeString __fastcall DecryptPassword(RawByteString Password, UnicodeString Key);
  virtual RawByteString __fastcall StronglyRecryptPassword(RawByteString Password, UnicodeString Key);

  // TUsage * __fastcall GetUsage() { return FUsage; };
  UnicodeString __fastcall GetPuttyRegistryStorageKey() { return FPuttyRegistryStorageKey; }
  UnicodeString __fastcall GetRandomSeedFile() const { return FRandomSeedFile; }
  bool __fastcall GetLogging() { return FLogging; }
  UnicodeString __fastcall GetLogFileName() { return FLogFileName; }
  bool __fastcall GetLogFileAppend() { return FLogFileAppend; }
  int __fastcall GetLogProtocol() { return FLogProtocol; }
  int __fastcall GetActualLogProtocol() { return FActualLogProtocol; }
  bool __fastcall GetLogActions() { return FLogActions; }
  UnicodeString __fastcall GetActionsLogFileName() const { return FActionsLogFileName; }
  int __fastcall GetLogWindowLines() { return FLogWindowLines; }
  TNotifyEvent & __fastcall GetOnChange() { return FOnChange; }
  void __fastcall SetOnChange(TNotifyEvent Value) { FOnChange = Value; }
  int __fastcall GetSessionReopenAuto() { return FSessionReopenAuto; }
  int GetSessionReopenBackground() { return FSessionReopenBackground; }
  int __fastcall GetSessionReopenTimeout() { return FSessionReopenTimeout; }
  int __fastcall GetSessionReopenAutoStall() { return FSessionReopenAutoStall; }
  int __fastcall GetTunnelLocalPortNumberLow() { return FTunnelLocalPortNumberLow; }
  int __fastcall GetTunnelLocalPortNumberHigh() { return FTunnelLocalPortNumberHigh; }
  int __fastcall GetCacheDirectoryChangesMaxSize() { return FCacheDirectoryChangesMaxSize; }
  bool __fastcall GetShowFtpWelcomeMessage() { return FShowFtpWelcomeMessage; }
  UnicodeString __fastcall GetExternalIpAddress() const { return FExternalIpAddress; }
  bool __fastcall GetDisablePasswordStoring() { return FDisablePasswordStoring; }
  bool __fastcall GetForceBanners() { return FForceBanners; }
  bool __fastcall GetDisableAcceptingHostKeys() { return FDisableAcceptingHostKeys; }
  void __fastcall SetLogToFile(bool Value);
  int __fastcall GetSessionReopenAutoMaximumNumberOfRetries() const { return FSessionReopenAutoMaximumNumberOfRetries; }
  void __fastcall SetSessionReopenAutoMaximumNumberOfRetries(int Value);
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
