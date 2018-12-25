//---------------------------------------------------------------------------
#ifndef GUIConfigurationH
#define GUIConfigurationH
//---------------------------------------------------------------------------
#include "Configuration.h"
#include "CopyParam.h"
//---------------------------------------------------------------------------
class TGUIConfiguration;
class TStoredSessionList;
enum TInterface { ifCommander, ifExplorer };
//---------------------------------------------------------------------------
extern const int ccLocal;
extern const int ccShowResults;
extern const int ccCopyResults;
extern const int ccSet;
extern const int ccRemoteFiles;
extern const int ccShowResultsInMsgBox;
//---------------------------------------------------------------------------
const int soRecurse =         0x01;
const int soSynchronize =     0x02;
const int soSynchronizeAsk =  0x04;
const int soContinueOnError = 0x08;
//---------------------------------------------------------------------------
class TGUICopyParamType : public TCopyParamType
{
public:
  TGUICopyParamType();
  TGUICopyParamType(const TCopyParamType & Source);
  TGUICopyParamType(const TGUICopyParamType & Source);

  virtual void Load(THierarchicalStorage * Storage);
  virtual void Save(THierarchicalStorage * Storage, const TCopyParamType * Defaults = nullptr) const;

  virtual void Default();
  virtual void Assign(const TCopyParamType * Source);
  TGUICopyParamType & operator =(const TGUICopyParamType & rhp);
  TGUICopyParamType & operator =(const TCopyParamType & rhp);

  __property bool Queue = { read = FQueue, write = FQueue };
  __property bool QueueNoConfirmation = { read = FQueueNoConfirmation, write = FQueueNoConfirmation };
  __property bool QueueParallel = { read = FQueueParallel, write = FQueueParallel };

protected:
  void GUIDefault();
  void GUIAssign(const TGUICopyParamType * Source);

private:
  bool FQueue;
  bool FQueueNoConfirmation;
  bool FQueueParallel;
};
//---------------------------------------------------------------------------
struct TCopyParamRuleData
{
  UnicodeString HostName;
  UnicodeString UserName;
  UnicodeString RemoteDirectory;
  UnicodeString LocalDirectory;

  void Default();
};
//---------------------------------------------------------------------------
class TCopyParamRule
{
public:
  TCopyParamRule();
  TCopyParamRule(const TCopyParamRuleData & Data);
  TCopyParamRule(const TCopyParamRule & Source);

  bool Matches(const TCopyParamRuleData & Value) const;
  void Load(THierarchicalStorage * Storage);
  void Save(THierarchicalStorage * Storage) const;

  UnicodeString GetInfoStr(UnicodeString Separator) const;

  bool operator ==(const TCopyParamRule & rhp) const;

  __property TCopyParamRuleData Data = { read = FData, write = FData };
  __property bool IsEmpty = { read = GetEmpty };

private:
  TCopyParamRuleData FData;

  inline bool Match(const UnicodeString & Mask,
    const UnicodeString & Value, bool Path, bool Local, int ForceDirectoryMasks) const;
  bool GetEmpty() const;
};
//---------------------------------------------------------------------------
class TLocaleInfo : public TObject
{
public:
  LCID Locale;
  UnicodeString Name;
  int Completeness;
};
//---------------------------------------------------------------------------
class TCopyParamList
{
friend class TGUIConfiguration;
public:
  TCopyParamList();
  virtual ~TCopyParamList();
  int Find(const TCopyParamRuleData & Value) const;

  void Load(THierarchicalStorage * Storage, int Count);
  void Save(THierarchicalStorage * Storage) const;

  static void ValidateName(const UnicodeString Name);

  TCopyParamList & operator=(const TCopyParamList & rhl);
  bool operator==(const TCopyParamList & rhl) const;

  void Clear();
  void Add(const UnicodeString Name,
    TCopyParamType * CopyParam, TCopyParamRule * Rule);
  void Insert(int Index, const UnicodeString Name,
    TCopyParamType * CopyParam, TCopyParamRule * Rule);
  void Change(int Index, const UnicodeString Name,
    TCopyParamType * CopyParam, TCopyParamRule * Rule);
  void Move(int CurIndex, int NewIndex);
  void Delete(int Index);
  int IndexOfName(const UnicodeString Name) const;

  __property int Count = { read = GetCount };
  __property UnicodeString Names[int Index] = { read = GetName };
  __property const TCopyParamRule * Rules[int Index] = { read = GetRule };
  __property const TCopyParamType * CopyParams[int Index] = { read = GetCopyParam };
  __property bool Modified = { read = FModified };
  __property TStrings * NameList = { read = GetNameList };
  __property bool AnyRule = { read = GetAnyRule };

private:
  static UnicodeString FInvalidChars;
  TList * FRules;
  TList * FCopyParams;
  TStrings * FNames;
  mutable TStrings * FNameList;
  bool FModified;

  int GetCount() const;
  const TCopyParamRule * GetRule(int Index) const;
  const TCopyParamType * GetCopyParam(int Index) const;
  UnicodeString GetName(int Index) const;
  TStrings * GetNameList() const;
  bool GetAnyRule() const;

  void Init();
  void Reset();
  void Modify();
  bool CompareItem(int Index, const TCopyParamType * CopyParam,
    const TCopyParamRule * Rule) const;
};
//---------------------------------------------------------------------------
class TGUIConfiguration : public TConfiguration
{
private:
  std::unique_ptr<TObjectList> FLocales;
  UnicodeString FLastLocalesExts;
  bool FContinueOnError{false};
  bool FConfirmCommandSession{false};
  UnicodeString FPuttyPath;
  bool FPuttyPassword{false};
  bool FTelnetForFtpInPutty{false};
  UnicodeString FPuttySession;
  int FSynchronizeParams{0};
  int FSynchronizeOptions{0};
  int FSynchronizeModeAuto{0};
  int FSynchronizeMode{0};
  int FMaxWatchDirectories{0};
  TDateTime FIgnoreCancelBeforeFinish;
  bool FQueueAutoPopup{false};
  bool FSessionRememberPassword{false};
  int FQueueTransfersLimit{0};
  bool FQueueBootstrap{false};
  bool FQueueKeepDoneItems{false};
  int FQueueKeepDoneItemsFor{0};
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
  int FKeepUpToDateChangeDelay{0};
  UnicodeString FChecksumAlg;
  int FSessionReopenAutoIdle{0};
  LCID FAppliedLocale;
  // Corresponds to FAppliedLocale
  UnicodeString FLocaleModuleName;

protected:
  LCID FLocale{};

  virtual void SaveData(THierarchicalStorage * Storage, bool All);
  virtual void LoadData(THierarchicalStorage * Storage);
  LCID GetLocale();
  void SetLocale(LCID value);
  void SetLocaleSafe(LCID value);
  UnicodeString GetAppliedLocaleHex();
  virtual HINSTANCE LoadNewResourceModule(LCID Locale,
    UnicodeString & FileName);
  HANDLE GetResourceModule();
  void SetResourceModule(HINSTANCE Instance);
  TObjectList * GetLocales();
  void AddLocale(LCID Locale, const UnicodeString & Name);
  void FreeResourceModule(HANDLE Instance);
  void SetDefaultCopyParam(const TGUICopyParamType & value);
  virtual bool GetRememberPassword();
  const TCopyParamList * GetCopyParamList();
  void SetCopyParamList(const TCopyParamList * value);
  virtual void DefaultLocalized();
  int GetCopyParamIndex();
  TGUICopyParamType GetCurrentCopyParam();
  TGUICopyParamType GetCopyParamPreset(UnicodeString Name);
  bool GetHasCopyParamPreset(UnicodeString Name);
  void SetCopyParamIndex(int value);
  void SetCopyParamCurrent(UnicodeString value);
  void SetNewDirectoryProperties(const TRemoteProperties & value);
  virtual void Saved();
  void SetQueueTransfersLimit(int value);
  void SetQueueBootstrap(bool value);
  void SetQueueKeepDoneItems(bool value);
  void SetQueueKeepDoneItemsFor(int value);
  void SetLocaleInternal(LCID value, bool Safe, bool CompleteOnly);
  void SetAppliedLocale(LCID AppliedLocale, const UnicodeString & LocaleModuleName);
  bool GetCanApplyLocaleImmediately();
  UnicodeString GetTranslationModule(const UnicodeString & Path);
  UnicodeString AddTranslationsSubFolder(const UnicodeString & Path);
  void FindLocales(const UnicodeString & LocalesMask, TStrings * Exts, UnicodeString & LocalesExts);
  virtual int GetResourceModuleCompleteness(HINSTANCE Module);
  virtual bool IsTranslationComplete(HINSTANCE Module);
  static int LocalesCompare(void * Item1, void * Item2);
  bool DoSaveCopyParam(THierarchicalStorage * Storage, const TCopyParamType * CopyParam, const TCopyParamType * Defaults);

public:
  TGUIConfiguration() noexcept;
  virtual ~TGUIConfiguration() noexcept;
  virtual void Default();
  virtual void UpdateStaticUsage();
  bool LoadCopyParam(THierarchicalStorage * Storage, TCopyParamType * CopyParam);
  void LoadDefaultCopyParam(THierarchicalStorage * Storage);

  HANDLE ChangeToDefaultResourceModule();
  HANDLE ChangeResourceModule(HANDLE Instance);
  LCID InternalLocale();
  UnicodeString AppliedLocaleCopyright();
  UnicodeString AppliedLocaleVersion();
  TStoredSessionList * SelectPuttySessionsForImport(TStoredSessionList * Sessions, UnicodeString & Error);
  bool AnyPuttySessionForImport(TStoredSessionList * Sessions);

  __property bool ContinueOnError = { read = FContinueOnError, write = FContinueOnError };
  __property bool ConfirmCommandSession = { read = FConfirmCommandSession, write = FConfirmCommandSession };
  __property int SynchronizeParams = { read = FSynchronizeParams, write = FSynchronizeParams };
  __property int SynchronizeOptions = { read = FSynchronizeOptions, write = FSynchronizeOptions };
  __property int SynchronizeModeAuto = { read = FSynchronizeModeAuto, write = FSynchronizeModeAuto };
  __property int SynchronizeMode = { read = FSynchronizeMode, write = FSynchronizeMode };
  __property int MaxWatchDirectories = { read = FMaxWatchDirectories, write = FMaxWatchDirectories };
  __property int QueueTransfersLimit = { read = FQueueTransfersLimit, write = SetQueueTransfersLimit };
  __property bool QueueBootstrap = { read = FQueueBootstrap, write = SetQueueBootstrap };
  __property bool QueueKeepDoneItems = { read = FQueueKeepDoneItems, write = SetQueueKeepDoneItems };
  __property int QueueKeepDoneItemsFor = { read = FQueueKeepDoneItemsFor, write = SetQueueKeepDoneItemsFor };
  __property bool QueueAutoPopup = { read = FQueueAutoPopup, write = FQueueAutoPopup };
  __property bool SessionRememberPassword = { read = FSessionRememberPassword, write = FSessionRememberPassword };
  __property LCID Locale = { read = GetLocale, write = SetLocale };
  __property LCID LocaleSafe = { read = GetLocale, write = SetLocaleSafe };
  __property UnicodeString AppliedLocaleHex = { read = GetAppliedLocaleHex };
  __property TObjectList * Locales = { read = GetLocales };
  ROProperty<TObjectList *> Locales{nb::bind(&TGUIConfiguration::GetLocales, this)};
  __property UnicodeString PuttyPath = { read = FPuttyPath, write = FPuttyPath };
  UnicodeString& PuttyPath{FPuttyPath};
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
  __property bool HasCopyParamPreset[UnicodeString Name] = { read = GetHasCopyParamPreset };
  __property TRemoteProperties NewDirectoryProperties = { read = FNewDirectoryProperties, write = SetNewDirectoryProperties };
  __property int KeepUpToDateChangeDelay = { read = FKeepUpToDateChangeDelay, write = FKeepUpToDateChangeDelay };
  __property UnicodeString ChecksumAlg = { read = FChecksumAlg, write = FChecksumAlg };
  __property int SessionReopenAutoIdle = { read = FSessionReopenAutoIdle, write = FSessionReopenAutoIdle };
  __property bool CanApplyLocaleImmediately = { read = GetCanApplyLocaleImmediately };
  __property LCID AppliedLocale = { read = FAppliedLocale };
};
//---------------------------------------------------------------------------
extern TGUIConfiguration * GUIConfiguration;
//---------------------------------------------------------------------------
#endif
