//---------------------------------------------------------------------------
#ifndef GUIConfigurationH
#define GUIConfigurationH
//---------------------------------------------------------------------------
#include "Configuration.h"
#include "CopyParam.h"
//---------------------------------------------------------------------------
struct TPasLibModule;
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

  void Load(THierarchicalStorage * Storage);
  void Save(THierarchicalStorage * Storage);

  virtual void Default();
  virtual void Assign(const TCopyParamType * Source);
  TGUICopyParamType & operator =(const TGUICopyParamType & rhp);
  TGUICopyParamType & operator =(const TCopyParamType & rhp);

  // __property bool Queue = { read = FQueue, write = FQueue };
  bool GetQueue() { return FQueue; }
  void SetQueue(bool value) { FQueue = value; }
  // __property bool QueueNoConfirmation = { read = FQueueNoConfirmation, write = FQueueNoConfirmation };
  bool GetQueueNoConfirmation() { return FQueueNoConfirmation; }
  void SetQueueNoConfirmation(bool value) { FQueueNoConfirmation = value; }
  // __property bool QueueIndividually = { read = FQueueIndividually, write = FQueueIndividually };
  bool GetQueueIndividually() { return FQueueIndividually; }
  void SetQueueIndividually(bool value) { FQueueIndividually = value; }
  // __property bool NewerOnly = { read = FNewerOnly, write = FNewerOnly };
  bool GetNewerOnly() { return FNewerOnly; }
  void SetNewerOnly(bool value) { FNewerOnly = value; }

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
  wstring HostName;
  wstring UserName;
  wstring RemoteDirectory;
  wstring LocalDirectory;

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

  wstring GetInfoStr(wstring Separator) const;

  bool operator ==(const TCopyParamRule & rhp) const;

  // __property TCopyParamRuleData Data = { read = FData, write = FData };
  TCopyParamRuleData GetData() { return FData; }
  void SetData(TCopyParamRuleData value) { FData = value; }
  // __property bool IsEmpty = { read = GetEmpty };
  bool GetEmpty() const;

private:
  TCopyParamRuleData FData;

  inline bool Match(const wstring & Mask,
    const wstring & Value, bool Path, bool Local = true) const;
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

  static void ValidateName(const wstring Name);

  void operator=(const TCopyParamList & rhl);
  bool operator==(const TCopyParamList & rhl) const;

  void Clear();
  void Add(const wstring Name,
    TCopyParamType * CopyParam, TCopyParamRule * Rule);
  void Insert(int Index, const wstring Name,
    TCopyParamType * CopyParam, TCopyParamRule * Rule);
  void Change(int Index, const wstring Name,
    TCopyParamType * CopyParam, TCopyParamRule * Rule);
  void Move(int CurIndex, int NewIndex);
  void Delete(int Index);
  int IndexOfName(const wstring Name) const;

  // __property int Count = { read = GetCount };
  int GetCount() const;
  // __property wstring Names[int Index] = { read = GetName };
  wstring GetName(int Index) const;
  // __property const TCopyParamRule * Rules[int Index] = { read = GetRule };
  const TCopyParamRule * GetRule(int Index) const;
  // __property const TCopyParamType * CopyParams[int Index] = { read = GetCopyParam };
  const TCopyParamType * GetCopyParam(int Index) const;
  //  __property bool Modified = { read = FModified };
  bool GetModified() { return FModified; }
  // __property TStrings * NameList = { read = GetNameList };
  TStrings * GetNameList() const;
  // __property bool AnyRule = { read = GetAnyRule };
  bool GetAnyRule() const;

private:
  static wstring FInvalidChars;
  TList * FRules;
  TList * FCopyParams;
  TStrings * FNames;
  mutable TStrings * FNameList;
  bool FModified;

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
  TStrings * FLocales;
  wstring FLastLocalesExts;
  bool FContinueOnError;
  bool FConfirmCommandSession;
  wstring FPuttyPath;
  wstring FPSftpPath;
  bool FPuttyPassword;
  bool FTelnetForFtpInPutty;
  wstring FPuttySession;
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
  wstring FDefaultPuttyPathOnly;
  wstring FDefaultPuttyPath;
  bool FSynchronizeBrowsing;
  TCopyParamList * FCopyParamList;
  bool FCopyParamListDefaults;
  wstring FCopyParamCurrent;
  TRemoteProperties FNewDirectoryProperties;
  int FKeepUpToDateChangeDelay;
  wstring FChecksumAlg;
  int FSessionReopenAutoIdle;

protected:
  LCID FLocale;

  virtual void SaveData(THierarchicalStorage * Storage, bool All);
  virtual void LoadData(THierarchicalStorage * Storage);
  virtual HANDLE LoadNewResourceModule(LCID Locale,
    wstring * FileName = NULL);
  HANDLE GetResourceModule();
  virtual void SetResourceModule(HANDLE Instance);
  LCID InternalLocale();
  void FreeResourceModule(HANDLE Instance);
  virtual bool GetRememberPassword();
  static wstring PropertyToKey(const wstring Property);
  virtual void DefaultLocalized();
  virtual void Saved();

public:
  TGUIConfiguration();
  virtual ~TGUIConfiguration();
  virtual void Default();

  HANDLE ChangeResourceModule(HANDLE Instance);

  // __property bool ContinueOnError = { read = FContinueOnError, write = FContinueOnError };
  bool GetContinueOnError() { return FContinueOnError; }
  void SetContinueOnError(bool value) { FContinueOnError = value; }
  // __property bool ConfirmCommandSession = { read = FConfirmCommandSession, write = FConfirmCommandSession };
  bool GetConfirmCommandSession() { return FConfirmCommandSession; }
  void SetConfirmCommandSession(bool value) { FConfirmCommandSession = value; }
  // __property int SynchronizeParams = { read = FSynchronizeParams, write = FSynchronizeParams };
  int GetSynchronizeParams() { return FSynchronizeParams; }
  void SetSynchronizeParams(int value) { FSynchronizeParams = value; }
  // __property int SynchronizeOptions = { read = FSynchronizeOptions, write = FSynchronizeOptions };
  int GetSynchronizeOptions() { return FSynchronizeOptions; }
  void SetSynchronizeOptions(int value) { FSynchronizeOptions = value; }
  // __property int SynchronizeModeAuto = { read = FSynchronizeModeAuto, write = FSynchronizeModeAuto };
  int GetSynchronizeModeAuto() { return FSynchronizeModeAuto; }
  void SetSynchronizeModeAuto(int value) { FSynchronizeModeAuto = value; }
  // __property int SynchronizeMode = { read = FSynchronizeMode, write = FSynchronizeMode };
  int GetSynchronizeMode() { return FSynchronizeMode; }
  void SetSynchronizeMode(int value) { FSynchronizeMode = value; }
  // __property int MaxWatchDirectories = { read = FMaxWatchDirectories, write = FMaxWatchDirectories };
  int GetMaxWatchDirectories() { return FMaxWatchDirectories; }
  void SetMaxWatchDirectories(int value) { FMaxWatchDirectories = value; }
  // __property int QueueTransfersLimit = { read = FQueueTransfersLimit, write = FQueueTransfersLimit };
  int GetQueueTransfersLimit() { return FQueueTransfersLimit; }
  void SetQueueTransfersLimit(int value) { FQueueTransfersLimit = value; }
  // __property bool QueueAutoPopup = { read = FQueueAutoPopup, write = FQueueAutoPopup };
  bool GetQueueAutoPopup() { return FQueueAutoPopup; }
  void SetQueueAutoPopup(bool value) { FQueueAutoPopup = value; }
  // __property bool QueueRememberPassword = { read = FQueueRememberPassword, write = FQueueRememberPassword };
  bool GetQueueRememberPassword() { return FQueueRememberPassword; }
  void SetQueueRememberPassword(bool value) { FQueueRememberPassword = value; }
  // __property LCID Locale = { read = GetLocale, write = SetLocale };
  virtual LCID GetLocale();
  void SetLocale(LCID value);
  // __property LCID LocaleSafe = { read = GetLocale, write = SetLocaleSafe };
  void SetLocaleSafe(LCID value);
  // __property TStrings * Locales = { read = GetLocales };
  TStrings * GetLocales();
  // __property wstring PuttyPath = { read = FPuttyPath, write = FPuttyPath };
  wstring GetPuttyPath() { return FPuttyPath; }
  void SetPuttyPath(wstring value) { FPuttyPath = value; }
  // __property wstring DefaultPuttyPath = { read = FDefaultPuttyPath };
  wstring GetDefaultPuttyPath() { return FDefaultPuttyPath; }
  // __property wstring PSftpPath = { read = FPSftpPath, write = FPSftpPath };
  wstring GetPSftpPath() { return FPSftpPath; }
  void SetPSftpPath(wstring value) { FPSftpPath = value; }
  // __property bool PuttyPassword = { read = FPuttyPassword, write = FPuttyPassword };
  bool GetPuttyPassword() { return FPuttyPassword; }
  void SetPuttyPassword(bool value) { FPuttyPassword = value; }
  // __property bool TelnetForFtpInPutty = { read = FTelnetForFtpInPutty, write = FTelnetForFtpInPutty };
  bool GetTelnetForFtpInPutty() { return FTelnetForFtpInPutty; }
  void SetTelnetForFtpInPutty(bool value) { FTelnetForFtpInPutty = value; }
  // __property wstring PuttySession = { read = FPuttySession, write = FPuttySession };
  wstring GetPuttySession() { return FPuttySession; }
  void SetPuttySession(wstring value) { FPuttySession = value; }
  // __property TDateTime IgnoreCancelBeforeFinish = { read = FIgnoreCancelBeforeFinish, write = FIgnoreCancelBeforeFinish };
  TDateTime GetIgnoreCancelBeforeFinish() { return FIgnoreCancelBeforeFinish; }
  void SetIgnoreCancelBeforeFinish(TDateTime value) { FIgnoreCancelBeforeFinish = value; }
  //  __property TGUICopyParamType DefaultCopyParam = { read = FDefaultCopyParam, write = SetDefaultCopyParam };
  TGUICopyParamType GetDefaultCopyParam() { return FDefaultCopyParam; }
  void SetDefaultCopyParam(const TGUICopyParamType & value);
  // __property bool BeepOnFinish = { read = FBeepOnFinish, write = FBeepOnFinish };
  bool GetBeepOnFinish() { return FBeepOnFinish; }
  void SetBeepOnFinish(bool value) { FBeepOnFinish = value; }
  // __property bool SynchronizeBrowsing = { read = FSynchronizeBrowsing, write = FSynchronizeBrowsing };
  bool GetSynchronizeBrowsing() { return FSynchronizeBrowsing; }
  void SetSynchronizeBrowsing(bool value) { FSynchronizeBrowsing = value; }
  // __property TDateTime BeepOnFinishAfter = { read = FBeepOnFinishAfter, write = FBeepOnFinishAfter };
  TDateTime GetBeepOnFinishAfter() { return FBeepOnFinishAfter; }
  void SetBeepOnFinishAfter(TDateTime value) { FBeepOnFinishAfter = value; }
  // __property const TCopyParamList * CopyParamList = { read = GetCopyParamList, write = SetCopyParamList };
  const TCopyParamList * GetCopyParamList();
  void SetCopyParamList(const TCopyParamList * value);
  // __property wstring CopyParamCurrent = { read = FCopyParamCurrent, write = SetCopyParamCurrent };
  wstring GetCopyParamCurrent() { return FCopyParamCurrent; }
  void SetCopyParamCurrent(wstring value);
  // __property int CopyParamIndex = { read = GetCopyParamIndex, write = SetCopyParamIndex };
  int GetCopyParamIndex();
  void SetCopyParamIndex(int value);
  // __property TGUICopyParamType CurrentCopyParam = { read = GetCurrentCopyParam };
  TGUICopyParamType GetCurrentCopyParam();
  // __property TGUICopyParamType CopyParamPreset[wstring Name] = { read = GetCopyParamPreset };
  TGUICopyParamType GetCopyParamPreset(wstring Name);
  // __property TRemoteProperties NewDirectoryProperties = { read = FNewDirectoryProperties, write = SetNewDirectoryProperties };
  TRemoteProperties GetNewDirectoryProperties() { return FNewDirectoryProperties; }
  void SetNewDirectoryProperties(const TRemoteProperties & value);
  // __property int KeepUpToDateChangeDelay = { read = FKeepUpToDateChangeDelay, write = FKeepUpToDateChangeDelay };
  int GetKeepUpToDateChangeDelay() { return FKeepUpToDateChangeDelay; }
  void SetKeepUpToDateChangeDelay(int value) { FKeepUpToDateChangeDelay = value; }
  // __property wstring ChecksumAlg = { read = FChecksumAlg, write = FChecksumAlg };
  wstring GetChecksumAlg() { return FChecksumAlg; }
  void SetChecksumAlg(wstring value) { FChecksumAlg = value; }
  // __property int SessionReopenAutoIdle = { read = FSessionReopenAutoIdle, write = FSessionReopenAutoIdle };
  int GetSessionReopenAutoIdle() { return FSessionReopenAutoIdle; }
  void SetSessionReopenAutoIdle(int value) { FSessionReopenAutoIdle = value; }
};
//---------------------------------------------------------------------------
#define GUIConfiguration (dynamic_cast<TGUIConfiguration *>(Configuration))
//---------------------------------------------------------------------------
#endif
