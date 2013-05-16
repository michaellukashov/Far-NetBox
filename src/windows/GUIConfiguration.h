//---------------------------------------------------------------------------
#ifndef GUIConfigurationH
#define GUIConfigurationH
//---------------------------------------------------------------------------
#include "Configuration.h"
#include "CopyParam.h"
//---------------------------------------------------------------------------
class TGUIConfiguration;
class TStoredSessionList;
enum TLogView { lvNone, lvWindow, pvPanel };
enum TInterface { ifCommander, ifExplorer };
//---------------------------------------------------------------------------
extern const intptr_t ccLocal;
extern const intptr_t ccShowResults;
extern const intptr_t ccCopyResults;
extern const intptr_t ccSet;
//---------------------------------------------------------------------------
const int soRecurse =        0x01;
const int soSynchronize =    0x02;
const int soSynchronizeAsk = 0x04;
//---------------------------------------------------------------------------
class TGUICopyParamType : public TCopyParamType
{
public:
  TGUICopyParamType();
  TGUICopyParamType(const TCopyParamType & Source);
  TGUICopyParamType(const TGUICopyParamType & Source);
  virtual ~TGUICopyParamType() {}

  void Load(THierarchicalStorage * Storage);
  void Save(THierarchicalStorage * Storage);

  virtual void Default();
  virtual void Assign(const TCopyParamType * Source);
  TGUICopyParamType & operator =(const TGUICopyParamType & rhp);
  TGUICopyParamType & operator =(const TCopyParamType & rhp);

  bool GetQueue() const { return FQueue; }
  void SetQueue(bool Value) { FQueue = Value; }
  bool GetQueueNoConfirmation() const { return FQueueNoConfirmation; }
  void SetQueueNoConfirmation(bool Value) { FQueueNoConfirmation = Value; }
  bool GetQueueIndividually() const { return FQueueIndividually; }
  void SetQueueIndividually(bool Value) { FQueueIndividually = Value; }

protected:
  void GUIDefault();
  void GUIAssign(const TGUICopyParamType * Source);

private:
  bool FQueue;
  bool FQueueNoConfirmation;
  bool FQueueIndividually;
};
//---------------------------------------------------------------------------
struct TCopyParamRuleData : public TObject
{
  UnicodeString HostName;
  UnicodeString UserName;
  UnicodeString RemoteDirectory;
  UnicodeString LocalDirectory;

  void Default();
};
//---------------------------------------------------------------------------
class TCopyParamRule : public TObject
{
public:
  explicit TCopyParamRule();
  explicit TCopyParamRule(const TCopyParamRuleData & Data);
  explicit TCopyParamRule(const TCopyParamRule & Source);

  bool Matches(const TCopyParamRuleData & Value) const;
  void Load(THierarchicalStorage * Storage);
  void Save(THierarchicalStorage * Storage) const;

  UnicodeString GetInfoStr(const UnicodeString & Separator) const;

  bool operator ==(const TCopyParamRule & rhp) const;

  TCopyParamRuleData GetData() const { return FData; }
  void SetData(const TCopyParamRuleData & Value);
  bool GetEmpty() const;

private:
  TCopyParamRuleData FData;

  inline bool Match(const UnicodeString & Mask,
    const UnicodeString & Value, bool Path, bool Local = true) const;
};
//---------------------------------------------------------------------------
class TCopyParamList : public TObject
{
friend class TGUIConfiguration;
public:
  explicit TCopyParamList();
  virtual ~TCopyParamList();
  intptr_t Find(const TCopyParamRuleData & Value) const;

  void Load(THierarchicalStorage * Storage, intptr_t Count);
  void Save(THierarchicalStorage * Storage) const;

  static void ValidateName(const UnicodeString & Name);

  TCopyParamList & operator=(const TCopyParamList & rhl);
  bool operator==(const TCopyParamList & rhl) const;

  void Clear();
  void Add(const UnicodeString & Name,
    TCopyParamType * CopyParam, TCopyParamRule * Rule);
  void Insert(intptr_t Index, const UnicodeString & Name,
    TCopyParamType * CopyParam, TCopyParamRule * Rule);
  void Change(intptr_t Index, const UnicodeString & Name,
    TCopyParamType * CopyParam, TCopyParamRule * Rule);
  void Move(intptr_t CurIndex, intptr_t NewIndex);
  void Delete(intptr_t Index);
  intptr_t IndexOfName(const UnicodeString & Name) const;

  intptr_t GetCount() const { return FCopyParams ? FCopyParams->GetCount() : 0; }
  UnicodeString GetName(intptr_t Index) const;
  const TCopyParamRule * GetRule(intptr_t Index) const;
  const TCopyParamType * GetCopyParam(intptr_t Index) const;
  bool GetModified() const { return FModified; }
  TStrings * GetNameList() const;
  bool GetAnyRule() const;

private:
  static UnicodeString FInvalidChars;
  TList * FRules;
  TList * FCopyParams;
  TStrings * FNames;
  mutable TStrings * FNameList;
  bool FModified;

  void Init();
  void Reset();
  void Modify();
  bool CompareItem(intptr_t Index, const TCopyParamType * CopyParam,
    const TCopyParamRule * Rule) const;
};
//---------------------------------------------------------------------------
class TGUIConfiguration : public TConfiguration
{
private:
  TStrings * FLocales;
  UnicodeString FLastLocalesExts;
  bool FContinueOnError;
  bool FConfirmCommandSession;
  UnicodeString FPuttyPath;
  UnicodeString FPSftpPath;
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
  UnicodeString FDefaultPuttyPathOnly;
  UnicodeString FDefaultPuttyPath;
  TCopyParamList * FCopyParamList;
  bool FCopyParamListDefaults;
  UnicodeString FCopyParamCurrent;
  TRemoteProperties FNewDirectoryProperties;
  intptr_t FKeepUpToDateChangeDelay;
  UnicodeString FChecksumAlg;
  intptr_t FSessionReopenAutoIdle;

protected:
  LCID FLocale;

public:
  virtual void SaveData(THierarchicalStorage * Storage, bool All);
  virtual void LoadData(THierarchicalStorage * Storage);
  virtual LCID GetLocale();
  LCID GetLocaleSafe() { return GetLocale(); }
  void SetLocale(LCID Value);
  void SetLocaleSafe(LCID Value);
  virtual HINSTANCE LoadNewResourceModule(LCID Locale,
    UnicodeString * FileName = NULL);
  HANDLE GetResourceModule();
  // virtual void SetResourceModule(HINSTANCE Instance);
  TStrings * GetLocales();
  LCID InternalLocale();
  void FreeResourceModule(HANDLE Instance);
  void SetDefaultCopyParam(const TGUICopyParamType & Value);
  virtual bool GetRememberPassword();
  const TCopyParamList * GetCopyParamList();
  void SetCopyParamList(const TCopyParamList * Value);
  static UnicodeString PropertyToKey(const UnicodeString & Property);
  virtual void DefaultLocalized();
  intptr_t GetCopyParamIndex();
  TGUICopyParamType GetCurrentCopyParam();
  TGUICopyParamType GetCopyParamPreset(const UnicodeString & Name);
  bool GetHasCopyParamPreset(const UnicodeString & Name);
  void SetCopyParamIndex(intptr_t Value);
  void SetCopyParamCurrent(const UnicodeString & Value);
  void SetNewDirectoryProperties(const TRemoteProperties & Value);
  virtual void Saved();
  void SetQueueTransfersLimit(intptr_t Value);
  void SetQueueKeepDoneItems(bool Value);
  void SetQueueKeepDoneItemsFor(intptr_t Value);

public:
  explicit TGUIConfiguration();
  virtual ~TGUIConfiguration();
  virtual void Default();
  virtual void UpdateStaticUsage();

  HANDLE ChangeResourceModule(HANDLE Instance);
  TStoredSessionList * SelectPuttySessionsForImport(TStoredSessionList * Sessions);
  bool AnyPuttySessionForImport(TStoredSessionList * Sessions);
  TStoredSessionList * SelectFilezillaSessionsForImport(TStoredSessionList * Sessions);
  bool AnyFilezillaSessionForImport(TStoredSessionList * Sessions);

  bool GetContinueOnError() { return FContinueOnError; }
  void SetContinueOnError(bool Value) { FContinueOnError = Value; }
  bool GetConfirmCommandSession() { return FConfirmCommandSession; }
  void SetConfirmCommandSession(bool Value) { FConfirmCommandSession = Value; }
  intptr_t GetSynchronizeParams() { return FSynchronizeParams; }
  void SetSynchronizeParams(intptr_t Value) { FSynchronizeParams = Value; }
  intptr_t GetSynchronizeOptions() { return FSynchronizeOptions; }
  void SetSynchronizeOptions(intptr_t Value) { FSynchronizeOptions = Value; }
  intptr_t GetSynchronizeModeAuto() { return FSynchronizeModeAuto; }
  void SetSynchronizeModeAuto(intptr_t Value) { FSynchronizeModeAuto = Value; }
  intptr_t GetSynchronizeMode() { return FSynchronizeMode; }
  void SetSynchronizeMode(intptr_t Value) { FSynchronizeMode = Value; }
  intptr_t GetMaxWatchDirectories() { return FMaxWatchDirectories; }
  void SetMaxWatchDirectories(intptr_t Value) { FMaxWatchDirectories = Value; }
  intptr_t GetQueueTransfersLimit() { return FQueueTransfersLimit; }
  bool GetQueueKeepDoneItems() { return FQueueKeepDoneItems; }
  intptr_t GetQueueKeepDoneItemsFor() { return FQueueKeepDoneItemsFor; }
  bool GetQueueAutoPopup() { return FQueueAutoPopup; }
  void SetQueueAutoPopup(bool Value) { FQueueAutoPopup = Value; }
  bool GetSessionRememberPassword() { return FSessionRememberPassword; }
  void SetSessionRememberPassword(bool Value) { FSessionRememberPassword = Value; }
  UnicodeString GetPuttyPath();
  void SetPuttyPath(const UnicodeString & Value);
  UnicodeString GetDefaultPuttyPath();
  UnicodeString GetPSftpPath();
  void SetPSftpPath(const UnicodeString & Value);
  bool GetPuttyPassword() { return FPuttyPassword; }
  void SetPuttyPassword(bool Value) { FPuttyPassword = Value; }
  bool GetTelnetForFtpInPutty() { return FTelnetForFtpInPutty; }
  void SetTelnetForFtpInPutty(bool Value) { FTelnetForFtpInPutty = Value; }
  UnicodeString GetPuttySession();
  void SetPuttySession(const UnicodeString & Value);
  TDateTime GetIgnoreCancelBeforeFinish() { return FIgnoreCancelBeforeFinish; }
  void SetIgnoreCancelBeforeFinish(TDateTime Value) { FIgnoreCancelBeforeFinish = Value; }
  TGUICopyParamType & GetDefaultCopyParam() { return FDefaultCopyParam; }
  bool GetBeepOnFinish() { return FBeepOnFinish; }
  void SetBeepOnFinish(bool Value) { FBeepOnFinish = Value; }
  TDateTime GetBeepOnFinishAfter() { return FBeepOnFinishAfter; }
  void SetBeepOnFinishAfter(TDateTime Value) { FBeepOnFinishAfter = Value; }
  UnicodeString GetCopyParamCurrent();
  TRemoteProperties GetNewDirectoryProperties() { return FNewDirectoryProperties; }
  intptr_t GetKeepUpToDateChangeDelay() { return FKeepUpToDateChangeDelay; }
  void SetKeepUpToDateChangeDelay(intptr_t Value) { FKeepUpToDateChangeDelay = Value; }
  UnicodeString GetChecksumAlg();
  void SetChecksumAlg(const UnicodeString & Value);
  intptr_t GetSessionReopenAutoIdle() { return FSessionReopenAutoIdle; }
  void SetSessionReopenAutoIdle(intptr_t Value) { FSessionReopenAutoIdle = Value; }
};
//---------------------------------------------------------------------------
#define GUIConfiguration (dynamic_cast<TGUIConfiguration *>(GetConfiguration()))
//---------------------------------------------------------------------------
#endif
