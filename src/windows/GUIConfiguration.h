//---------------------------------------------------------------------------
#ifndef GUIConfigurationH
#define GUIConfigurationH
//---------------------------------------------------------------------------
#include "Configuration.h"
#include "CopyParam.h"
//---------------------------------------------------------------------------
class TGUIConfiguration;
enum TLogView { lvNone, lvWindow, pvPanel };
enum TInterface { ifCommander, ifExplorer };
//---------------------------------------------------------------------------
extern const int ccLocal;
extern const int ccShowResults;
extern const int ccCopyResults;
extern const int ccSet;
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
  bool GetNewerOnly() const { return FNewerOnly; }
  void SetNewerOnly(bool Value) { FNewerOnly = Value; }

protected:
  void GUIDefault();
  void GUIAssign(const TGUICopyParamType * Source);

private:
  bool FQueue;
  bool FQueueNoConfirmation;
  bool FQueueIndividually;
  bool FNewerOnly;
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
  explicit TCopyParamRule();
  explicit TCopyParamRule(const TCopyParamRuleData & Data);
  explicit TCopyParamRule(const TCopyParamRule & Source);

  bool Matches(const TCopyParamRuleData & Value) const;
  void Load(THierarchicalStorage * Storage);
  void Save(THierarchicalStorage * Storage) const;

  UnicodeString GetInfoStr(UnicodeString Separator) const;

  bool operator ==(const TCopyParamRule & rhp) const;

  TCopyParamRuleData GetData() const { return FData; }
  void SetData(TCopyParamRuleData Value);
  bool GetEmpty() const;

private:
  TCopyParamRuleData FData;

  inline bool Match(const UnicodeString & Mask,
    const UnicodeString & Value, bool Path, bool Local = true) const;
};
//---------------------------------------------------------------------------
class TCopyParamList
{
friend class TGUIConfiguration;
public:
  explicit TCopyParamList();
  virtual ~TCopyParamList();
  int Find(const TCopyParamRuleData & Value) const;

  void Load(THierarchicalStorage * Storage, int Count);
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

  intptr_t GetCount() const;
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
  int FSynchronizeParams;
  int FSynchronizeOptions;
  int FSynchronizeModeAuto;
  int FSynchronizeMode;
  int FMaxWatchDirectories;
  TDateTime FIgnoreCancelBeforeFinish;
  bool FQueueAutoPopup;
  bool FQueueRememberPassword;
  int FQueueTransfersLimit;
  TGUICopyParamType FDefaultCopyParam;
  bool FBeepOnFinish;
  TDateTime FBeepOnFinishAfter;
  UnicodeString FDefaultPuttyPathOnly;
  UnicodeString FDefaultPuttyPath;
  TCopyParamList * FCopyParamList;
  bool FCopyParamListDefaults;
  UnicodeString FCopyParamCurrent;
  TRemoteProperties FNewDirectoryProperties;
  int FKeepUpToDateChangeDelay;
  UnicodeString FChecksumAlg;
  int FSessionReopenAutoIdle;

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
  TGUICopyParamType GetCopyParamPreset(UnicodeString Name);
  bool GetHasCopyParamPreset(UnicodeString Name);
  void SetCopyParamIndex(int Value);
  void SetCopyParamCurrent(UnicodeString Value);
  void SetNewDirectoryProperties(const TRemoteProperties & Value);
  virtual void Saved();

public:
  explicit TGUIConfiguration();
  virtual ~TGUIConfiguration();
  virtual void Default();
  virtual void UpdateStaticUsage();

  HANDLE ChangeResourceModule(HANDLE Instance);

  bool GetContinueOnError() { return FContinueOnError; }
  void SetContinueOnError(bool Value) { FContinueOnError = Value; }
  bool GetConfirmCommandSession() { return FConfirmCommandSession; }
  void SetConfirmCommandSession(bool Value) { FConfirmCommandSession = Value; }
  int GetSynchronizeParams() { return FSynchronizeParams; }
  void SetSynchronizeParams(int Value) { FSynchronizeParams = Value; }
  int GetSynchronizeOptions() { return FSynchronizeOptions; }
  void SetSynchronizeOptions(int Value) { FSynchronizeOptions = Value; }
  int GetSynchronizeModeAuto() { return FSynchronizeModeAuto; }
  void SetSynchronizeModeAuto(int Value) { FSynchronizeModeAuto = Value; }
  int GetSynchronizeMode() { return FSynchronizeMode; }
  void SetSynchronizeMode(int Value) { FSynchronizeMode = Value; }
  int GetMaxWatchDirectories() { return FMaxWatchDirectories; }
  void SetMaxWatchDirectories(int Value) { FMaxWatchDirectories = Value; }
  int GetQueueTransfersLimit() { return FQueueTransfersLimit; }
  void SetQueueTransfersLimit(int Value) { FQueueTransfersLimit = Value; }
  bool GetQueueAutoPopup() { return FQueueAutoPopup; }
  void SetQueueAutoPopup(bool Value) { FQueueAutoPopup = Value; }
  bool GetQueueRememberPassword() { return FQueueRememberPassword; }
  void SetQueueRememberPassword(bool Value) { FQueueRememberPassword = Value; }
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
  void SetPuttySession(UnicodeString Value);
  TDateTime GetIgnoreCancelBeforeFinish() { return FIgnoreCancelBeforeFinish; }
  void SetIgnoreCancelBeforeFinish(TDateTime Value) { FIgnoreCancelBeforeFinish = Value; }
  TGUICopyParamType & GetDefaultCopyParam() { return FDefaultCopyParam; }
  bool GetBeepOnFinish() { return FBeepOnFinish; }
  void SetBeepOnFinish(bool Value) { FBeepOnFinish = Value; }
  TDateTime GetBeepOnFinishAfter() { return FBeepOnFinishAfter; }
  void SetBeepOnFinishAfter(TDateTime Value) { FBeepOnFinishAfter = Value; }
  UnicodeString GetCopyParamCurrent();
  TRemoteProperties GetNewDirectoryProperties() { return FNewDirectoryProperties; }
  int GetKeepUpToDateChangeDelay() { return FKeepUpToDateChangeDelay; }
  void SetKeepUpToDateChangeDelay(int Value) { FKeepUpToDateChangeDelay = Value; }
  UnicodeString GetChecksumAlg();
  void SetChecksumAlg(const UnicodeString & Value);
  int GetSessionReopenAutoIdle() { return FSessionReopenAutoIdle; }
  void SetSessionReopenAutoIdle(int Value) { FSessionReopenAutoIdle = Value; }
};
//---------------------------------------------------------------------------
#define GUIConfiguration (dynamic_cast<TGUIConfiguration *>(Configuration))
//---------------------------------------------------------------------------
#endif
