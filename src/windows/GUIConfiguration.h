
#pragma once

#include "Configuration.h"
#include "CopyParam.h"

class TGUIConfiguration;
class TStoredSessionList;
enum TInterface { ifCommander, ifExplorer };

extern const int32_t ccLocal;
extern const int32_t ccShowResults;
extern const int32_t ccCopyResults;
extern const int32_t ccSet;
extern const int32_t ccRemoteFiles;
extern const int32_t ccShowResultsInMsgBox;

constexpr int32_t soRecurse =         0x01;
constexpr int32_t soSynchronize =     0x02;
constexpr int32_t soSynchronizeAsk =  0x04;
constexpr int32_t soContinueOnError = 0x08;

NB_DEFINE_CLASS_ID(TGUICopyParamType);
class NB_CORE_EXPORT TGUICopyParamType : public TCopyParamType
{
public:
  static bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TGUICopyParamType); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TGUICopyParamType) || TCopyParamType::is(Kind); }
  virtual ~TGUICopyParamType() = default;
public:
  TGUICopyParamType() noexcept;
  TGUICopyParamType(const TCopyParamType & Source) noexcept;
  explicit TGUICopyParamType(const TGUICopyParamType &Source) noexcept;

  virtual void Load(THierarchicalStorage * Storage);
  virtual void Save(THierarchicalStorage * Storage, const TCopyParamType * Defaults = nullptr) const;

  virtual void Default() override;
  virtual void Assign(const TCopyParamType * Source) override;
  TGUICopyParamType & operator =(const TGUICopyParamType & rhp);
  TGUICopyParamType & operator =(const TCopyParamType & rhp);

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
  void GUIAssign(const TGUICopyParamType * Source);

private:
  bool FQueue{false};
  bool FQueueNoConfirmation{false};
  bool FQueueParallel{false};
};

struct NB_CORE_EXPORT TCopyParamRuleData : public TObject
{
  UnicodeString HostName;
  UnicodeString UserName;
  UnicodeString RemoteDirectory;
  UnicodeString LocalDirectory;

  void Default();
};

NB_DEFINE_CLASS_ID(TCopyParamRule);
class NB_CORE_EXPORT TCopyParamRule : public TObject
{
public:
  static bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TCopyParamRule); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TCopyParamRule) || TObject::is(Kind); }
public:
  explicit TCopyParamRule() noexcept;
  explicit TCopyParamRule(const TCopyParamRuleData &Data) noexcept;
  explicit TCopyParamRule(const TCopyParamRule &Source) noexcept;

  bool Matches(const TCopyParamRuleData & Value) const;
  void Load(THierarchicalStorage * Storage);
  void Save(THierarchicalStorage * Storage) const;

  UnicodeString GetInfoStr(const UnicodeString & Separator) const;

  bool operator ==(const TCopyParamRule & rhp) const;

  __property TCopyParamRuleData Data = { read = FData, write = FData };
  __property bool IsEmpty = { read = GetEmpty };
  TCopyParamRuleData GetData() const { return FData; }
  void SetData(const TCopyParamRuleData &Value) { FData = Value; }

public:
  TCopyParamRule &operator=(const TCopyParamRule &other);

private:
  TCopyParamRuleData FData;

  bool Match(const UnicodeString & Mask,
    const UnicodeString & Value, bool Path, bool Local, int32_t ForceDirectoryMasks) const;
public:
  bool GetEmpty() const;
};

class NB_CORE_EXPORT TLocaleInfo : public TObject
{
public:
  LCID Locale{};
  UnicodeString Name;
  int32_t Completeness{0};
};

class NB_CORE_EXPORT TCopyParamList : public TObject
{
friend class TGUIConfiguration;
public:
  explicit TCopyParamList() noexcept;
  explicit TCopyParamList(const TCopyParamList & other) noexcept;

  virtual ~TCopyParamList() noexcept;
  int32_t Find(const TCopyParamRuleData & Value) const;

  void Load(THierarchicalStorage * Storage, int32_t Count);
  void Save(THierarchicalStorage * Storage) const;

  static void ValidateName(const UnicodeString & Name);

  TCopyParamList & operator=(const TCopyParamList & rhl);
  bool operator==(const TCopyParamList & rhl) const;

  void Clear();
  void Add(const UnicodeString & Name,
    TCopyParamType * CopyParam, TCopyParamRule * Rule);
  void Insert(int32_t Index, const UnicodeString & Name,
    TCopyParamType * CopyParam, TCopyParamRule * Rule);
  void Change(int32_t Index, const UnicodeString & Name,
    TCopyParamType * CopyParam, TCopyParamRule * Rule);
  void Move(int32_t CurIndex, int32_t NewIndex);
  void Delete(int32_t Index);
  int32_t IndexOfName(const UnicodeString & Name) const;

  __property int Count = { read = GetCount };
  ROProperty<int32_t> Count{nb::bind(&TCopyParamList::GetCount, this)};
//  __property UnicodeString Names[int Index] = { read = GetName };
//  __property const TCopyParamRule * Rules[int Index] = { read = GetRule };
//  __property const TCopyParamType * CopyParams[int Index] = { read = GetCopyParam };
  __property bool Modified = { read = FModified };
  __property TStrings * NameList = { read = GetNameList };
  __property bool AnyRule = { read = GetAnyRule };

private:
  static UnicodeString FInvalidChars;
  std::unique_ptr<TList> FRules;
  std::unique_ptr<TList> FCopyParams;
  std::unique_ptr<TStrings> FNames;
  mutable std::unique_ptr<TStrings> FNameList;
  bool FModified{false};

public:
  int32_t GetCount() const;
  const TCopyParamRule * GetRule(int32_t Index) const;
  const TCopyParamType * GetCopyParam(int32_t Index) const;
  UnicodeString GetName(int32_t Index) const;
  TStrings *GetNameList() const;
  bool GetAnyRule() const;

  bool GetModified() const { return FModified; }
private:
  void Init();
  void Reset();
  void Modify();
  bool CompareItem(int32_t Index, const TCopyParamType * CopyParam,
    const TCopyParamRule * Rule) const;
};

NB_DEFINE_CLASS_ID(TGUIConfiguration);
class NB_CORE_EXPORT TGUIConfiguration : public TConfiguration
{
  NB_DISABLE_COPY(TGUIConfiguration)
public:
  static bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TGUIConfiguration); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TGUIConfiguration) || TConfiguration::is(Kind); }
private:
  std::unique_ptr<TObjectList> FLocales;
  UnicodeString FLastLocalesExts;
  bool FContinueOnError{false};
  bool FConfirmCommandSession{false};
  UnicodeString FPuttyPath;
  TAutoSwitch FUsePuttyPwFile{asAuto};
  bool FPuttyPassword{false};
  bool FTelnetForFtpInPutty{false};
  UnicodeString FPuttySession;
  int32_t FSynchronizeParams{0};
  int32_t FSynchronizeOptions{0};
  int32_t FSynchronizeModeAuto{0};
  int32_t FSynchronizeMode{0};
  int32_t FMaxWatchDirectories{0};
  TDateTime FIgnoreCancelBeforeFinish;
  bool FQueueAutoPopup{false};
  bool FSessionRememberPassword{false};
  bool FQueueBootstrap{false};
  bool FQueueKeepDoneItems{false};
  int32_t FQueueKeepDoneItemsFor{0};
  TGUICopyParamType FDefaultCopyParam;
  bool FBeepOnFinish{false};
  TDateTime FBeepOnFinishAfter;
  UnicodeString FBeepSound;
  UnicodeString FDefaultPuttyPathOnly;
  UnicodeString FDefaultPuttyPath;
  std::unique_ptr<TCopyParamList> FCopyParamList;
  bool FCopyParamListDefaults{false};
  UnicodeString FCopyParamCurrent;
  TRemoteProperties FNewDirectoryProperties;
  int32_t FKeepUpToDateChangeDelay{0};
  UnicodeString FChecksumAlg;
  int32_t FSessionReopenAutoIdle{0};
  LCID FAppliedLocale{0};
  // Corresponds to FAppliedLocale
  UnicodeString FLocaleModuleName;

protected:
  LCID FLocale{0};

public:
  virtual void SaveData(THierarchicalStorage * Storage, bool All) override;
  virtual void LoadData(THierarchicalStorage * Storage) override;
  virtual LCID GetLocale() const;
  void SetLocale(LCID Value);
  void SetLocaleSafe(LCID Value);
  UnicodeString GetAppliedLocaleHex() const;
  virtual HINSTANCE LoadNewResourceModule(LCID Locale,
    UnicodeString & FileName);
  HANDLE GetResourceModule();
  void SetResourceModule(HINSTANCE Instance);
  TObjectList * GetLocales();
  void AddLocale(LCID Locale, const UnicodeString & Name);
  void FreeResourceModule(HANDLE Instance);
  void SetDefaultCopyParam(const TGUICopyParamType & Value);
  virtual bool GetRememberPassword() const override;
  const TCopyParamList * GetCopyParamList() const;
  void SetCopyParamList(const TCopyParamList * Value);
  virtual void DefaultLocalized();
  int32_t GetCopyParamIndex() const;
  TGUICopyParamType GetCurrentCopyParam() const;
  TGUICopyParamType GetCopyParamPreset(const UnicodeString & Name) const;
  bool GetHasCopyParamPreset(const UnicodeString & Name) const;
  void SetCopyParamIndex(int32_t Value);
  void SetCopyParamCurrent(const UnicodeString & Value);
  void SetNewDirectoryProperties(const TRemoteProperties & Value);
  virtual void Saved() override;
  void SetQueueBootstrap(bool Value);
  void SetQueueKeepDoneItems(bool Value);
  void SetQueueKeepDoneItemsFor(int32_t Value);
  void SetLocaleInternal(LCID Value, bool Safe, bool CompleteOnly);
  void SetAppliedLocale(LCID AppliedLocale, const UnicodeString & LocaleModuleName);
  bool GetCanApplyLocaleImmediately() const;
  UnicodeString GetTranslationModule(const UnicodeString & Path) const;
  UnicodeString AddTranslationsSubFolder(const UnicodeString & Path) const;
  void FindLocales(const UnicodeString & LocalesMask, TStrings * Exts, UnicodeString & LocalesExts);
  virtual int32_t GetResourceModuleCompleteness(HINSTANCE Module);
  virtual bool IsTranslationComplete(HINSTANCE Module);
  static int32_t LocalesCompare(void * Item1, void * Item2);
  LCID InternalLocale();
  bool DoSaveCopyParam(THierarchicalStorage * Storage, const TCopyParamType * CopyParam, const TCopyParamType * Defaults);

  LCID GetLocaleSafe() const { return GetLocale(); }
  void SetInitialLocale(LCID Value);
  void ConfigurationInit() override;
public:
  TGUIConfiguration() = delete;
  explicit TGUIConfiguration(TObjectClassId Kind) noexcept;
  virtual ~TGUIConfiguration() noexcept;
  virtual void Default() override;
  virtual void UpdateStaticUsage() override;
  bool LoadCopyParam(THierarchicalStorage * Storage, TCopyParamType * CopyParam);
  void LoadDefaultCopyParam(THierarchicalStorage * Storage);

  HANDLE ChangeToDefaultResourceModule();
  HANDLE ChangeResourceModule(HANDLE Instance);
  bool UsingInternalTranslation() const;
  UnicodeString AppliedLocaleCopyright() const;
  UnicodeString AppliedLocaleVersion();
  TStoredSessionList * SelectPuttySessionsForImport(
    const UnicodeString & RootKey, const UnicodeString & Source, TStoredSessionList * Sessions, UnicodeString & AError);
  bool AnyPuttySessionForImport(TStoredSessionList * ASessions);

  __property bool ContinueOnError = { read = FContinueOnError, write = FContinueOnError };
  __property bool ConfirmCommandSession = { read = FConfirmCommandSession, write = FConfirmCommandSession };
  __property int SynchronizeParams = { read = FSynchronizeParams, write = FSynchronizeParams };
  __property int SynchronizeOptions = { read = FSynchronizeOptions, write = FSynchronizeOptions };
  __property int SynchronizeModeAuto = { read = FSynchronizeModeAuto, write = FSynchronizeModeAuto };
  __property int SynchronizeMode = { read = FSynchronizeMode, write = FSynchronizeMode };
  __property int MaxWatchDirectories = { read = FMaxWatchDirectories, write = FMaxWatchDirectories };
  __property bool QueueBootstrap = { read = FQueueBootstrap, write = SetQueueBootstrap };
  __property bool QueueKeepDoneItems = { read = FQueueKeepDoneItems, write = SetQueueKeepDoneItems };
  __property int QueueKeepDoneItemsFor = { read = FQueueKeepDoneItemsFor, write = SetQueueKeepDoneItemsFor };
  __property bool QueueAutoPopup = { read = FQueueAutoPopup, write = FQueueAutoPopup };
  __property bool SessionRememberPassword = { read = FSessionRememberPassword, write = FSessionRememberPassword };
  bool& SessionRememberPassword{FSessionRememberPassword};
  __property LCID Locale = { read = GetLocale, write = SetLocale };
  __property LCID LocaleSafe = { read = GetLocale, write = SetLocaleSafe };
  __property UnicodeString AppliedLocaleHex = { read = GetAppliedLocaleHex };
  __property TObjectList * Locales = { read = GetLocales };
  ROProperty<TObjectList *> Locales{nb::bind(&TGUIConfiguration::GetLocales, this)};
  __property UnicodeString PuttyPath = { read = FPuttyPath, write = FPuttyPath };
  UnicodeString& PuttyPath{FPuttyPath};
  __property TAutoSwitch UsePuttyPwFile = { read = FUsePuttyPwFile, write = FUsePuttyPwFile };
  TAutoSwitch& UsePuttyPwFile{FUsePuttyPwFile};
  __property UnicodeString DefaultPuttyPath = { read = FDefaultPuttyPath };
  const UnicodeString& DefaultPuttyPath{FDefaultPuttyPath};
  __property bool PuttyPassword = { read = FPuttyPassword, write = FPuttyPassword };
  bool& PuttyPassword{FPuttyPassword};
  __property bool TelnetForFtpInPutty = { read = FTelnetForFtpInPutty, write = FTelnetForFtpInPutty };
  bool& TelnetForFtpInPutty{FTelnetForFtpInPutty};
  __property UnicodeString PuttySession = { read = FPuttySession, write = FPuttySession };
  UnicodeString& PuttySession{FPuttySession};
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
  // __property bool HasCopyParamPreset[UnicodeString Name] = { read = GetHasCopyParamPreset };
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
  int32_t GetSynchronizeParams() const { return FSynchronizeParams; }
  void SetSynchronizeParams(int32_t Value) { FSynchronizeParams = Value; }
  int32_t GetSynchronizeOptions() const { return FSynchronizeOptions; }
  void SetSynchronizeOptions(int32_t Value) { FSynchronizeOptions = Value; }
  int32_t GetSynchronizeModeAuto() const { return FSynchronizeModeAuto; }
  void SetSynchronizeModeAuto(int32_t Value) { FSynchronizeModeAuto = Value; }
  int32_t GetSynchronizeMode() const { return FSynchronizeMode; }
  void SetSynchronizeMode(int32_t Value) { FSynchronizeMode = Value; }
  int32_t GetMaxWatchDirectories() const { return FMaxWatchDirectories; }
  void SetMaxWatchDirectories(int32_t Value) { FMaxWatchDirectories = Value; }
  bool GetQueueBootstrap() const { return FQueueBootstrap; }
  bool GetQueueKeepDoneItems() const { return FQueueKeepDoneItems; }
  int32_t GetQueueKeepDoneItemsFor() const { return FQueueKeepDoneItemsFor; }
  bool GetQueueAutoPopup() const { return FQueueAutoPopup; }
  void SetQueueAutoPopup(bool Value) { FQueueAutoPopup = Value; }
  bool GetSessionRememberPassword() const { return FSessionRememberPassword; }
  void SetSessionRememberPassword(bool Value) { FSessionRememberPassword = Value; }
  UnicodeString GetPuttyPath() const;
  void SetPuttyPath(const UnicodeString & Value);
  UnicodeString GetDefaultPuttyPath() const;
  UnicodeString GetPSftpPath() const;
  void SetPSftpPath(const UnicodeString & Value);
  bool GetPuttyPassword() const { return FPuttyPassword; }
  void SetPuttyPassword(bool Value) { FPuttyPassword = Value; }
  bool GetTelnetForFtpInPutty() const { return FTelnetForFtpInPutty; }
  void SetTelnetForFtpInPutty(bool Value) { FTelnetForFtpInPutty = Value; }
  UnicodeString GetPuttySession() const;
  void SetPuttySession(const UnicodeString & Value);
  TDateTime GetIgnoreCancelBeforeFinish() const { return FIgnoreCancelBeforeFinish; }
  void SetIgnoreCancelBeforeFinish(const TDateTime &Value) { FIgnoreCancelBeforeFinish = Value; }
  TGUICopyParamType &GetDefaultCopyParam() { return FDefaultCopyParam; }
  bool GetBeepOnFinish() const { return FBeepOnFinish; }
  void SetBeepOnFinish(bool Value) { FBeepOnFinish = Value; }
  TDateTime GetBeepOnFinishAfter() const { return FBeepOnFinishAfter; }
  void SetBeepOnFinishAfter(const TDateTime &Value) { FBeepOnFinishAfter = Value; }
  UnicodeString GetCopyParamCurrent() const;
  const TRemoteProperties &GetNewDirectoryProperties() const { return FNewDirectoryProperties; }
  int32_t GetKeepUpToDateChangeDelay() const { return FKeepUpToDateChangeDelay; }
  void SetKeepUpToDateChangeDelay(int32_t Value) { FKeepUpToDateChangeDelay = Value; }
  UnicodeString GetChecksumAlg() const;
  void SetChecksumAlg(const UnicodeString & Value);
  int32_t GetSessionReopenAutoIdle() const { return FSessionReopenAutoIdle; }
  void SetSessionReopenAutoIdle(int32_t Value) { FSessionReopenAutoIdle = Value; }
  LCID GetAppliedLocale() const { return FAppliedLocale; }
};

NB_CORE_EXPORT TGUIConfiguration * GetGUIConfiguration();


