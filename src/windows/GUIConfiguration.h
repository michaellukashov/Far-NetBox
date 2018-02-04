
#pragma once

#include "Configuration.h"
#include "CopyParam.h"

#define CONST_INVALID_CHARS L"/\\[]"

class TGUIConfiguration;
class TStoredSessionList;

enum TLogView
{
  lvNone,
  lvWindow,
  pvPanel
};

enum TInterface
{
  ifCommander,
  ifExplorer
};

extern const intptr_t ccLocal;
extern const intptr_t ccShowResults;
extern const intptr_t ccCopyResults;
extern const intptr_t ccSet;
extern const intptr_t ccRemoteFiles;
extern const intptr_t ccShowResultsInMsgBox;

const int soRecurse =        0x01;
const int soSynchronize =    0x02;
const int soSynchronizeAsk = 0x04;
const int soContinueOnError = 0x08;

class NB_CORE_EXPORT TGUICopyParamType : public TCopyParamType
{
public:
  static inline bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TGUICopyParamType); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TGUICopyParamType) || TCopyParamType::is(Kind); }
public:
  TGUICopyParamType();
  TGUICopyParamType(const TCopyParamType &Source);
  explicit TGUICopyParamType(const TGUICopyParamType &Source);
  virtual ~TGUICopyParamType()
  {
  }

  void Load(THierarchicalStorage *Storage);
  void Save(THierarchicalStorage *Storage);

  virtual void Default() override;
  virtual void Assign(const TCopyParamType *Source) override;
  TGUICopyParamType &operator=(const TGUICopyParamType &rhp);
  TGUICopyParamType &operator=(const TCopyParamType &rhp);

  __property bool Queue = { read = FQueue, write = FQueue };
  __property bool QueueNoConfirmation = { read = FQueueNoConfirmation, write = FQueueNoConfirmation };
  __property bool QueueParallel = { read = FQueueParallel, write = FQueueParallel };

  bool GetQueue() const { return FQueue; }
  void SetQueue(bool Value) { FQueue = Value; }
  bool GetQueueNoConfirmation() const { return FQueueNoConfirmation; }
  void SetQueueNoConfirmation(bool Value) { FQueueNoConfirmation = Value; }
  bool GetQueueParallel() const { return FQueueParallel; }
  void SetQueueParallel(bool Value) { FQueueParallel = Value; }

protected:
  void GUIDefault();
  void GUIAssign(const TGUICopyParamType *Source);

private:
  bool FQueue;
  bool FQueueNoConfirmation;
  bool FQueueParallel;
};

struct NB_CORE_EXPORT TCopyParamRuleData : public TObject
{
  UnicodeString HostName;
  UnicodeString UserName;
  UnicodeString RemoteDirectory;
  UnicodeString LocalDirectory;

  void Default();
};

class NB_CORE_EXPORT TCopyParamRule : public TObject
{
public:
  static inline bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TCopyParamRule); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TCopyParamRule) || TObject::is(Kind); }
public:
  explicit TCopyParamRule();
  explicit TCopyParamRule(const TCopyParamRuleData &Data);
  explicit TCopyParamRule(const TCopyParamRule &Source);

  bool Matches(const TCopyParamRuleData &Value) const;
  void Load(THierarchicalStorage *Storage);
  void Save(THierarchicalStorage *Storage) const;

  UnicodeString GetInfoStr(const UnicodeString Separator) const;

  bool operator==(const TCopyParamRule &rhp) const;

  __property TCopyParamRuleData Data = { read = FData, write = FData };
  __property bool IsEmpty = { read = GetEmpty };

  TCopyParamRuleData GetData() const { return FData; }
  void SetData(const TCopyParamRuleData &Value) { FData = Value; }

public:
  TCopyParamRule &operator=(const TCopyParamRule &other);

private:
  TCopyParamRuleData FData;

  bool Match(const UnicodeString Mask,
    UnicodeString Value, bool Path, bool Local, int ForceDirectoryMasks) const;

public:
  bool GetEmpty() const;
};

class NB_CORE_EXPORT TLocaleInfo : public TObject
{
public:
  LCID Locale;
  UnicodeString Name;
  int Completeness;
};

class NB_CORE_EXPORT TCopyParamList : public TObject
{
  friend class TGUIConfiguration;
public:
  explicit TCopyParamList();
  explicit TCopyParamList(const TCopyParamList &other);

  virtual ~TCopyParamList();
  intptr_t Find(const TCopyParamRuleData &Value) const;

  void Load(THierarchicalStorage *Storage, intptr_t ACount);
  void Save(THierarchicalStorage *Storage) const;

  static void ValidateName(const UnicodeString Name);

  TCopyParamList &operator=(const TCopyParamList &rhl);
  bool operator==(const TCopyParamList &rhl) const;

  void Clear();
  void Add(const UnicodeString Name,
    TCopyParamType *CopyParam, TCopyParamRule *Rule);
  void Insert(intptr_t Index, const UnicodeString Name,
    TCopyParamType *CopyParam, TCopyParamRule *Rule);
  void Change(intptr_t Index, const UnicodeString Name,
    TCopyParamType *CopyParam, TCopyParamRule *Rule);
  void Move(intptr_t CurIndex, intptr_t NewIndex);
  void Delete(intptr_t Index);
  intptr_t IndexOfName(const UnicodeString Name) const;

  __property int Count = { read = GetCount };
  __property UnicodeString Names[int Index] = { read = GetName };
  __property const TCopyParamRule * Rules[int Index] = { read = GetRule };
  __property const TCopyParamType * CopyParams[int Index] = { read = GetCopyParam };
  __property bool Modified = { read = FModified };
  __property TStrings * NameList = { read = GetNameList };
  __property bool AnyRule = { read = GetAnyRule };

private:
  static UnicodeString FInvalidChars;
  TList *FRules;
  TList *FCopyParams;
  TStrings *FNames;
  mutable TStrings *FNameList;
  bool FModified;

public:
  intptr_t GetCount() const;
  UnicodeString GetName(intptr_t Index) const;
  const TCopyParamRule *GetRule(intptr_t Index) const;
  const TCopyParamType *GetCopyParam(intptr_t Index) const;
  bool GetModified() const { return FModified; }
  TStrings *GetNameList() const;
  bool GetAnyRule() const;

private:
  void Reset();
  void Modify();
  bool CompareItem(intptr_t Index, const TCopyParamType *CopyParam,
    const TCopyParamRule *Rule) const;
};

class NB_CORE_EXPORT TGUIConfiguration : public TConfiguration
{
  NB_DISABLE_COPY(TGUIConfiguration)
public:
  static inline bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TGUIConfiguration); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TGUIConfiguration) || TConfiguration::is(Kind); }
private:
  TObjectList *FLocales;
  UnicodeString FLastLocalesExts;
  bool FContinueOnError;
  bool FConfirmCommandSession;
  UnicodeString FPuttyPath;
  bool FPuttyPassword;
  bool FTelnetForFtpInPutty;
  UnicodeString FPuttySession;
  intptr_t FSynchronizeParams;
  intptr_t FSynchronizeOptions;
  intptr_t FSynchronizeModeAuto;
  intptr_t FSynchronizeMode;
  intptr_t FMaxWatchDirectories;
  TDateTime FIgnoreCancelBeforeFinish;
  bool FQueueAutoPopup;
  bool FSessionRememberPassword;
  intptr_t FQueueTransfersLimit;
  bool FQueueKeepDoneItems;
  intptr_t FQueueKeepDoneItemsFor;
  TGUICopyParamType FDefaultCopyParam;
  bool FBeepOnFinish;
  TDateTime FBeepOnFinishAfter;
  UnicodeString FBeepSound;
  UnicodeString FDefaultPuttyPathOnly;
  UnicodeString FDefaultPuttyPath;
  TCopyParamList *FCopyParamList;
  bool FCopyParamListDefaults;
  UnicodeString FCopyParamCurrent;
  TRemoteProperties FNewDirectoryProperties;
  intptr_t FKeepUpToDateChangeDelay;
  UnicodeString FChecksumAlg;
  intptr_t FSessionReopenAutoIdle;
  LCID FAppliedLocale;
  // Corresponds to FAppliedLocale
  UnicodeString FLocaleModuleName;

protected:
  LCID FLocale;

public:
  virtual void SaveData(THierarchicalStorage *Storage, bool All) override;
  virtual void LoadData(THierarchicalStorage *Storage) override;
  virtual LCID GetLocale();
  void SetLocale(LCID Value);
  LCID GetLocaleSafe() { return GetLocale(); }
  void SetLocaleSafe(LCID Value);
  UnicodeString GetAppliedLocaleHex() const;
  virtual HINSTANCE LoadNewResourceModule(LCID ALocale,
    UnicodeString &AFileName);
  HANDLE GetResourceModule();
  void SetResourceModule(HINSTANCE Instance);
  TObjectList *GetLocales();
  void AddLocale(LCID Locale, const UnicodeString Name);
  void FreeResourceModule(HANDLE Instance);
  void SetDefaultCopyParam(const TGUICopyParamType &Value);
  virtual bool GetRememberPassword() const override;
  TCopyParamList *GetCopyParamList() const;
  void SetCopyParamList(const TCopyParamList *Value);
  virtual void DefaultLocalized();
  intptr_t GetCopyParamIndex() const;
  TGUICopyParamType GetCurrentCopyParam() const;
  TGUICopyParamType GetCopyParamPreset(const UnicodeString Name) const;
  bool GetHasCopyParamPreset(const UnicodeString Name) const;
  void SetCopyParamIndex(intptr_t Value);
  void SetCopyParamCurrent(const UnicodeString Value);
  void SetNewDirectoryProperties(const TRemoteProperties &Value);
  virtual void Saved() override;
  void SetQueueTransfersLimit(intptr_t Value);
  void SetQueueKeepDoneItems(bool Value);
  void SetQueueKeepDoneItemsFor(intptr_t Value);
  void SetLocaleInternal(LCID Value, bool Safe, bool CompleteOnly);
  void SetInitialLocale(LCID Value);
  void SetAppliedLocale(LCID AppliedLocale, const UnicodeString LocaleModuleName);
  bool GetCanApplyLocaleImmediately() const;
  UnicodeString GetTranslationModule(const UnicodeString Path) const;
  UnicodeString AddTranslationsSubFolder(const UnicodeString Path) const;
  void FindLocales(const UnicodeString LocalesMask, TStrings *Exts, UnicodeString &LocalesExts);
  virtual int GetResourceModuleCompleteness(HINSTANCE Module);
  virtual bool IsTranslationComplete(HINSTANCE Module);
  static intptr_t LocalesCompare(void *Item1, void *Item2);

public:
  TGUIConfiguration(TObjectClassId Kind);
  virtual ~TGUIConfiguration();
  virtual void Default() override;
  virtual void UpdateStaticUsage() override;

  HANDLE ChangeToDefaultResourceModule();
  HANDLE ChangeResourceModule(HANDLE Instance);
  LCID InternalLocale() const;
  UnicodeString AppliedLocaleCopyright() const;
  UnicodeString AppliedLocaleVersion();
  TStoredSessionList *SelectPuttySessionsForImport(TStoredSessionList *Sessions, UnicodeString &Error);
  bool AnyPuttySessionForImport(TStoredSessionList *Sessions);

  __property bool ContinueOnError = { read = FContinueOnError, write = FContinueOnError };
  __property bool ConfirmCommandSession = { read = FConfirmCommandSession, write = FConfirmCommandSession };
  __property intptr_t SynchronizeParams = { read = FSynchronizeParams, write = FSynchronizeParams };
  __property intptr_t SynchronizeOptions = { read = FSynchronizeOptions, write = FSynchronizeOptions };
  __property intptr_t SynchronizeModeAuto = { read = FSynchronizeModeAuto, write = FSynchronizeModeAuto };
  __property intptr_t SynchronizeMode = { read = FSynchronizeMode, write = FSynchronizeMode };
  __property intptr_t MaxWatchDirectories = { read = FMaxWatchDirectories, write = FMaxWatchDirectories };
  __property intptr_t QueueTransfersLimit = { read = FQueueTransfersLimit, write = SetQueueTransfersLimit };
  __property bool QueueKeepDoneItems = { read = FQueueKeepDoneItems, write = SetQueueKeepDoneItems };
  __property intptr_t QueueKeepDoneItemsFor = { read = FQueueKeepDoneItemsFor, write = SetQueueKeepDoneItemsFor };
  __property bool QueueAutoPopup = { read = FQueueAutoPopup, write = FQueueAutoPopup };
  __property bool SessionRememberPassword = { read = FSessionRememberPassword, write = FSessionRememberPassword };
  __property LCID Locale = { read = GetLocale, write = SetLocale };
  __property LCID LocaleSafe = { read = GetLocale, write = SetLocaleSafe };
  __property UnicodeString AppliedLocaleHex = { read = GetAppliedLocaleHex };
  __property TObjectList * Locales = { read = GetLocales };
  __property UnicodeString PuttyPath = { read = FPuttyPath, write = FPuttyPath };
  __property UnicodeString DefaultPuttyPath = { read = FDefaultPuttyPath };
  __property bool PuttyPassword = { read = FPuttyPassword, write = FPuttyPassword };
  __property bool TelnetForFtpInPutty = { read = FTelnetForFtpInPutty, write = FTelnetForFtpInPutty };
  __property UnicodeString PuttySession = { read = FPuttySession, write = FPuttySession };
  __property TDateTime IgnoreCancelBeforeFinish = { read = FIgnoreCancelBeforeFinish, write = FIgnoreCancelBeforeFinish };
  __property TGUICopyParamType DefaultCopyParam = { read = FDefaultCopyParam, write = SetDefaultCopyParam };
  __property bool BeepOnFinish = { read = FBeepOnFinish, write = FBeepOnFinish };
  __property TDateTime BeepOnFinishAfter = { read = FBeepOnFinishAfter, write = FBeepOnFinishAfter };
  __property UnicodeString BeepSound = { read = FBeepSound, write = FBeepSound };
  __property const TCopyParamList * CopyParamList = { read = GetCopyParamList, write = SetCopyParamList };
  __property UnicodeString CopyParamCurrent = { read = FCopyParamCurrent, write = SetCopyParamCurrent };
  __property int CopyParamIndex = { read = GetCopyParamIndex, write = SetCopyParamIndex };
  __property TGUICopyParamType CurrentCopyParam = { read = GetCurrentCopyParam };
  __property TGUICopyParamType CopyParamPreset[UnicodeString Name] = { read = GetCopyParamPreset };
  __property bool HasCopyParamPreset[UnicodeString Name] = { read = GetHasCopyParamPreset };
  __property TRemoteProperties NewDirectoryProperties = { read = FNewDirectoryProperties, write = SetNewDirectoryProperties };
  __property int KeepUpToDateChangeDelay = { read = FKeepUpToDateChangeDelay, write = FKeepUpToDateChangeDelay };
  __property UnicodeString ChecksumAlg = { read = FChecksumAlg, write = FChecksumAlg };
  __property int SessionReopenAutoIdle = { read = FSessionReopenAutoIdle, write = FSessionReopenAutoIdle };
  __property bool CanApplyLocaleImmediately = { read = GetCanApplyLocaleImmediately };
  __property LCID AppliedLocale = { read = FAppliedLocale };

public:

  bool GetContinueOnError() const { return FContinueOnError; }
  void SetContinueOnError(bool Value) { FContinueOnError = Value; }
  bool GetConfirmCommandSession() const { return FConfirmCommandSession; }
  void SetConfirmCommandSession(bool Value) { FConfirmCommandSession = Value; }
  intptr_t GetSynchronizeParams() const { return FSynchronizeParams; }
  void SetSynchronizeParams(intptr_t Value) { FSynchronizeParams = Value; }
  intptr_t GetSynchronizeOptions() const { return FSynchronizeOptions; }
  void SetSynchronizeOptions(intptr_t Value) { FSynchronizeOptions = Value; }
  intptr_t GetSynchronizeModeAuto() const { return FSynchronizeModeAuto; }
  void SetSynchronizeModeAuto(intptr_t Value) { FSynchronizeModeAuto = Value; }
  intptr_t GetSynchronizeMode() const { return FSynchronizeMode; }
  void SetSynchronizeMode(intptr_t Value) { FSynchronizeMode = Value; }
  intptr_t GetMaxWatchDirectories() const { return FMaxWatchDirectories; }
  void SetMaxWatchDirectories(intptr_t Value) { FMaxWatchDirectories = Value; }
  intptr_t GetQueueTransfersLimit() const { return FQueueTransfersLimit; }
  bool GetQueueKeepDoneItems() const { return FQueueKeepDoneItems; }
  intptr_t GetQueueKeepDoneItemsFor() const { return FQueueKeepDoneItemsFor; }
  bool GetQueueAutoPopup() const { return FQueueAutoPopup; }
  void SetQueueAutoPopup(bool Value) { FQueueAutoPopup = Value; }
  bool GetSessionRememberPassword() const { return FSessionRememberPassword; }
  void SetSessionRememberPassword(bool Value) { FSessionRememberPassword = Value; }
  UnicodeString GetPuttyPath() const;
  void SetPuttyPath(UnicodeString Value);
  UnicodeString GetDefaultPuttyPath() const;
  UnicodeString GetPSftpPath() const;
  void SetPSftpPath(UnicodeString Value);
  bool GetPuttyPassword() const { return FPuttyPassword; }
  void SetPuttyPassword(bool Value) { FPuttyPassword = Value; }
  bool GetTelnetForFtpInPutty() const { return FTelnetForFtpInPutty; }
  void SetTelnetForFtpInPutty(bool Value) { FTelnetForFtpInPutty = Value; }
  UnicodeString GetPuttySession() const;
  void SetPuttySession(UnicodeString Value);
  TDateTime GetIgnoreCancelBeforeFinish() const { return FIgnoreCancelBeforeFinish; }
  void SetIgnoreCancelBeforeFinish(const TDateTime &Value) { FIgnoreCancelBeforeFinish = Value; }
  TGUICopyParamType &GetDefaultCopyParam() { return FDefaultCopyParam; }
  bool GetBeepOnFinish() const { return FBeepOnFinish; }
  void SetBeepOnFinish(bool Value) { FBeepOnFinish = Value; }
  TDateTime GetBeepOnFinishAfter() const { return FBeepOnFinishAfter; }
  void SetBeepOnFinishAfter(const TDateTime &Value) { FBeepOnFinishAfter = Value; }
  UnicodeString GetCopyParamCurrent() const;
  const TRemoteProperties &GetNewDirectoryProperties() const { return FNewDirectoryProperties; }
  intptr_t GetKeepUpToDateChangeDelay() const { return FKeepUpToDateChangeDelay; }
  void SetKeepUpToDateChangeDelay(intptr_t Value) { FKeepUpToDateChangeDelay = Value; }
  UnicodeString GetChecksumAlg() const;
  void SetChecksumAlg(UnicodeString Value);
  intptr_t GetSessionReopenAutoIdle() const { return FSessionReopenAutoIdle; }
  void SetSessionReopenAutoIdle(intptr_t Value) { FSessionReopenAutoIdle = Value; }
  LCID GetAppliedLocale() const { return FAppliedLocale; }
};

NB_CORE_EXPORT TGUIConfiguration *GetGUIConfiguration();

