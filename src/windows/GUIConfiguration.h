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
  /* __fastcall */ TGUICopyParamType();
  /* __fastcall */ TGUICopyParamType(const TCopyParamType & Source);
  /* __fastcall */ TGUICopyParamType(const TGUICopyParamType & Source);
  virtual /* __fastcall */ ~TGUICopyParamType();

  void __fastcall Load(THierarchicalStorage * Storage);
  void __fastcall Save(THierarchicalStorage * Storage);

  virtual void __fastcall Default();
  virtual void __fastcall Assign(const TCopyParamType * Source);
  TGUICopyParamType & __fastcall operator =(const TGUICopyParamType & rhp);
  TGUICopyParamType & __fastcall operator =(const TCopyParamType & rhp);

#ifndef _MSC_VER
  __property bool Queue = { read = FQueue, write = FQueue };
  __property bool QueueNoConfirmation = { read = FQueueNoConfirmation, write = FQueueNoConfirmation };
  __property bool QueueIndividually = { read = FQueueIndividually, write = FQueueIndividually };
  __property bool NewerOnly = { read = FNewerOnly, write = FNewerOnly };
#else
  bool GetQueue() const { return FQueue; }
  void SetQueue(bool value) { FQueue = value; }
  bool GetQueueNoConfirmation() const { return FQueueNoConfirmation; }
  void SetQueueNoConfirmation(bool value) { FQueueNoConfirmation = value; }
  bool GetQueueIndividually() const { return FQueueIndividually; }
  void SetQueueIndividually(bool value) { FQueueIndividually = value; }
  bool GetNewerOnly() const { return FNewerOnly; }
  void SetNewerOnly(bool value) { FNewerOnly = value; }
#endif

protected:
  void __fastcall GUIDefault();
  void __fastcall GUIAssign(const TGUICopyParamType * Source);

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

  void /* __fastcall */ Default();
};
//---------------------------------------------------------------------------
class TCopyParamRule
{
public:
  /* __fastcall */ TCopyParamRule();
  explicit /* __fastcall */ TCopyParamRule(const TCopyParamRuleData & Data);
  /* __fastcall */ TCopyParamRule(const TCopyParamRule & Source);

  bool __fastcall Matches(const TCopyParamRuleData & Value) const;
  void __fastcall Load(THierarchicalStorage * Storage);
  void __fastcall Save(THierarchicalStorage * Storage) const;

  UnicodeString __fastcall GetInfoStr(UnicodeString Separator) const;

  bool __fastcall operator ==(const TCopyParamRule & rhp) const;

#ifndef _MSC_VER
  __property TCopyParamRuleData Data = { read = FData, write = FData };
  __property bool IsEmpty = { read = GetEmpty };
#else
  TCopyParamRuleData __fastcall GetData() { return FData; }
  void __fastcall SetData(TCopyParamRuleData value) { FData = value; }
  bool __fastcall GetEmpty() const;
#endif

private:
  TCopyParamRuleData FData;

  inline bool __fastcall Match(const UnicodeString & Mask,
    const UnicodeString & Value, bool Path, bool Local = true) const;
};
//---------------------------------------------------------------------------
class TCopyParamList
{
friend class TGUIConfiguration;
public:
  /* __fastcall */ TCopyParamList();
  virtual /* __fastcall */ ~TCopyParamList();
  int __fastcall Find(const TCopyParamRuleData & Value) const;

  void __fastcall Load(THierarchicalStorage * Storage, int Count);
  void __fastcall Save(THierarchicalStorage * Storage) const;

  static void __fastcall ValidateName(const UnicodeString Name);

  void __fastcall operator=(const TCopyParamList & rhl);
  bool __fastcall operator==(const TCopyParamList & rhl) const;

  void __fastcall Clear();
  void __fastcall Add(const UnicodeString Name,
    TCopyParamType * CopyParam, TCopyParamRule * Rule);
  void __fastcall Insert(int Index, const UnicodeString Name,
    TCopyParamType * CopyParam, TCopyParamRule * Rule);
  void __fastcall Change(int Index, const UnicodeString Name,
    TCopyParamType * CopyParam, TCopyParamRule * Rule);
  void __fastcall Move(int CurIndex, int NewIndex);
  void __fastcall Delete(int Index);
  int __fastcall IndexOfName(const UnicodeString Name) const;

#ifndef _MSC_VER
  __property int Count = { read = GetCount };
  __property UnicodeString Names[int Index] = { read = GetName };
  __property const TCopyParamRule * Rules[int Index] = { read = GetRule };
  __property const TCopyParamType * CopyParams[int Index] = { read = GetCopyParam };
  __property bool Modified = { read = FModified };
  __property TStrings * NameList = { read = GetNameList };
  __property bool AnyRule = { read = GetAnyRule };
#else
  size_t __fastcall GetCount() const;
  UnicodeString __fastcall GetName(size_t Index) const;
  const TCopyParamRule * __fastcall GetRule(size_t Index) const;
  const TCopyParamType * __fastcall GetCopyParam(size_t Index) const;
  bool __fastcall GetModified() { return FModified; }
  TStrings * __fastcall GetNameList() const;
  bool __fastcall GetAnyRule() const;
#endif

private:
  static UnicodeString FInvalidChars;
  TObjectList * FRules;
  TObjectList * FCopyParams;
  TStrings * FNames;
  mutable TStrings * FNameList;
  bool FModified;

#ifndef _MSC_VER
  int __fastcall GetCount() const;
  const TCopyParamRule * __fastcall GetRule(int Index) const;
  const TCopyParamType * __fastcall GetCopyParam(int Index) const;
  UnicodeString __fastcall GetName(int Index) const;
  TStrings * __fastcall GetNameList() const;
  bool __fastcall GetAnyRule() const;
#endif

  void __fastcall Init();
  void __fastcall Reset();
  void __fastcall Modify();
  bool __fastcall CompareItem(int Index, const TCopyParamType * CopyParam,
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
  bool FSynchronizeBrowsing;
  TCopyParamList * FCopyParamList;
  bool FCopyParamListDefaults;
  UnicodeString FCopyParamCurrent;
  TRemoteProperties FNewDirectoryProperties;
  int FKeepUpToDateChangeDelay;
  UnicodeString FChecksumAlg;
  int FSessionReopenAutoIdle;

protected:
  LCID FLocale;

  virtual void __fastcall SaveData(THierarchicalStorage * Storage, bool All);
  virtual void __fastcall LoadData(THierarchicalStorage * Storage);
  virtual HANDLE __fastcall LoadNewResourceModule(LCID Locale,
    UnicodeString * FileName = NULL);
#ifndef _MSC_VER
  virtual LCID __fastcall GetLocale();
  void __fastcall SetLocale(LCID value);
  void __fastcall SetLocaleSafe(LCID value);
  virtual HINSTANCE __fastcall LoadNewResourceModule(LCID Locale,
    UnicodeString * FileName = NULL);
  HANDLE __fastcall GetResourceModule();
  virtual void __fastcall SetResourceModule(HINSTANCE Instance);
  TStrings * __fastcall GetLocales();
#endif
  LCID __fastcall InternalLocale();
  void __fastcall FreeResourceModule(HANDLE Instance);
  void __fastcall SetDefaultCopyParam(const TGUICopyParamType & value);
  virtual bool __fastcall GetRememberPassword();
  const TCopyParamList * __fastcall GetCopyParamList();
  void __fastcall SetCopyParamList(const TCopyParamList * value);
  static UnicodeString __fastcall PropertyToKey(const UnicodeString Property);
  virtual void __fastcall DefaultLocalized();
  int __fastcall GetCopyParamIndex();
  TGUICopyParamType __fastcall GetCurrentCopyParam();
  TGUICopyParamType __fastcall GetCopyParamPreset(UnicodeString Name);
  void __fastcall SetCopyParamIndex(int value);
  void __fastcall SetCopyParamCurrent(UnicodeString value);
  void __fastcall SetNewDirectoryProperties(const TRemoteProperties & value);
  virtual void __fastcall Saved();

public:
  /* __fastcall */ TGUIConfiguration();
  virtual /* __fastcall */ ~TGUIConfiguration();
  virtual void __fastcall Default();

#ifndef _MSC_VER
  HANDLE __fastcall ChangeResourceModule(HANDLE Instance);

  __property bool ContinueOnError = { read = FContinueOnError, write = FContinueOnError };
  __property bool ConfirmCommandSession = { read = FConfirmCommandSession, write = FConfirmCommandSession };
  __property int SynchronizeParams = { read = FSynchronizeParams, write = FSynchronizeParams };
  __property int SynchronizeOptions = { read = FSynchronizeOptions, write = FSynchronizeOptions };
  __property int SynchronizeModeAuto = { read = FSynchronizeModeAuto, write = FSynchronizeModeAuto };
  __property int SynchronizeMode = { read = FSynchronizeMode, write = FSynchronizeMode };
  __property int MaxWatchDirectories = { read = FMaxWatchDirectories, write = FMaxWatchDirectories };
  __property int QueueTransfersLimit = { read = FQueueTransfersLimit, write = FQueueTransfersLimit };
  __property bool QueueAutoPopup = { read = FQueueAutoPopup, write = FQueueAutoPopup };
  __property bool QueueRememberPassword = { read = FQueueRememberPassword, write = FQueueRememberPassword };
  __property LCID Locale = { read = GetLocale, write = SetLocale };
  __property LCID LocaleSafe = { read = GetLocale, write = SetLocaleSafe };
  __property TStrings * Locales = { read = GetLocales };
  __property UnicodeString PuttyPath = { read = FPuttyPath, write = FPuttyPath };
  __property UnicodeString DefaultPuttyPath = { read = FDefaultPuttyPath };
  __property UnicodeString PSftpPath = { read = FPSftpPath, write = FPSftpPath };
  __property bool PuttyPassword = { read = FPuttyPassword, write = FPuttyPassword };
  __property bool TelnetForFtpInPutty = { read = FTelnetForFtpInPutty, write = FTelnetForFtpInPutty };
  __property UnicodeString PuttySession = { read = FPuttySession, write = FPuttySession };
  __property TDateTime IgnoreCancelBeforeFinish = { read = FIgnoreCancelBeforeFinish, write = FIgnoreCancelBeforeFinish };
  __property TGUICopyParamType DefaultCopyParam = { read = FDefaultCopyParam, write = SetDefaultCopyParam };
  __property bool BeepOnFinish = { read = FBeepOnFinish, write = FBeepOnFinish };
  __property TDateTime BeepOnFinishAfter = { read = FBeepOnFinishAfter, write = FBeepOnFinishAfter };
  __property const TCopyParamList * CopyParamList = { read = GetCopyParamList, write = SetCopyParamList };
  __property UnicodeString CopyParamCurrent = { read = FCopyParamCurrent, write = SetCopyParamCurrent };
  __property int CopyParamIndex = { read = GetCopyParamIndex, write = SetCopyParamIndex };
  __property TGUICopyParamType CurrentCopyParam = { read = GetCurrentCopyParam };
  __property TGUICopyParamType CopyParamPreset[UnicodeString Name] = { read = GetCopyParamPreset };
  __property TRemoteProperties NewDirectoryProperties = { read = FNewDirectoryProperties, write = SetNewDirectoryProperties };
  __property int KeepUpToDateChangeDelay = { read = FKeepUpToDateChangeDelay, write = FKeepUpToDateChangeDelay };
  __property UnicodeString ChecksumAlg = { read = FChecksumAlg, write = FChecksumAlg };
  __property int SessionReopenAutoIdle = { read = FSessionReopenAutoIdle, write = FSessionReopenAutoIdle };
#else
  bool __fastcall GetContinueOnError() { return FContinueOnError; }
  void __fastcall SetContinueOnError(bool value) { FContinueOnError = value; }
  bool __fastcall GetConfirmCommandSession() { return FConfirmCommandSession; }
  void __fastcall SetConfirmCommandSession(bool value) { FConfirmCommandSession = value; }
  int __fastcall GetSynchronizeParams() { return FSynchronizeParams; }
  void __fastcall SetSynchronizeParams(int value) { FSynchronizeParams = value; }
  int __fastcall GetSynchronizeOptions() { return FSynchronizeOptions; }
  void __fastcall SetSynchronizeOptions(int value) { FSynchronizeOptions = value; }
  int __fastcall GetSynchronizeModeAuto() { return FSynchronizeModeAuto; }
  void __fastcall SetSynchronizeModeAuto(int value) { FSynchronizeModeAuto = value; }
  int __fastcall GetSynchronizeMode() { return FSynchronizeMode; }
  void __fastcall SetSynchronizeMode(int value) { FSynchronizeMode = value; }
  int __fastcall GetMaxWatchDirectories() { return FMaxWatchDirectories; }
  void __fastcall SetMaxWatchDirectories(int value) { FMaxWatchDirectories = value; }
  size_t __fastcall GetQueueTransfersLimit() { return FQueueTransfersLimit; }
  void __fastcall SetQueueTransfersLimit(size_t value) { FQueueTransfersLimit = value; }
  bool __fastcall GetQueueAutoPopup() { return FQueueAutoPopup; }
  void __fastcall SetQueueAutoPopup(bool value) { FQueueAutoPopup = value; }
  bool __fastcall GetQueueRememberPassword() { return FQueueRememberPassword; }
  void __fastcall SetQueueRememberPassword(bool value) { FQueueRememberPassword = value; }
  virtual LCID __fastcall GetLocale();
  void __fastcall SetLocale(LCID value);
  void __fastcall SetLocaleSafe(LCID value);
  TStrings * __fastcall GetLocales();
  UnicodeString __fastcall GetPuttyPath() { return FPuttyPath; }
  void __fastcall SetPuttyPath(const UnicodeString value) { FPuttyPath = value; }
  UnicodeString __fastcall GetDefaultPuttyPath() { return FDefaultPuttyPath; }
  UnicodeString __fastcall GetPSftpPath() { return FPSftpPath; }
  void __fastcall SetPSftpPath(const UnicodeString value) { FPSftpPath = value; }
  bool __fastcall GetPuttyPassword() { return FPuttyPassword; }
  void __fastcall SetPuttyPassword(bool value) { FPuttyPassword = value; }
  bool __fastcall GetTelnetForFtpInPutty() { return FTelnetForFtpInPutty; }
  void __fastcall SetTelnetForFtpInPutty(bool value) { FTelnetForFtpInPutty = value; }
  UnicodeString __fastcall GetPuttySession() { return FPuttySession; }
  void __fastcall SetPuttySession(UnicodeString value) { FPuttySession = value; }
  TDateTime __fastcall GetIgnoreCancelBeforeFinish() { return FIgnoreCancelBeforeFinish; }
  void __fastcall SetIgnoreCancelBeforeFinish(TDateTime value) { FIgnoreCancelBeforeFinish = value; }
  TGUICopyParamType & __fastcall GetDefaultCopyParam() { return FDefaultCopyParam; }
  void __fastcall SetDefaultCopyParam(const TGUICopyParamType & value);
  bool __fastcall GetBeepOnFinish() { return FBeepOnFinish; }
  void __fastcall SetBeepOnFinish(bool value) { FBeepOnFinish = value; }
  bool __fastcall GetSynchronizeBrowsing() { return FSynchronizeBrowsing; }
  void __fastcall SetSynchronizeBrowsing(bool value) { FSynchronizeBrowsing = value; }
  TDateTime __fastcall GetBeepOnFinishAfter() { return FBeepOnFinishAfter; }
  void __fastcall SetBeepOnFinishAfter(TDateTime value) { FBeepOnFinishAfter = value; }
  const TCopyParamList * __fastcall GetCopyParamList();
  void __fastcall SetCopyParamList(const TCopyParamList * value);
  UnicodeString __fastcall GetCopyParamCurrent() { return FCopyParamCurrent; }
  void __fastcall SetCopyParamCurrent(const UnicodeString value);
  size_t __fastcall GetCopyParamIndex();
  void __fastcall SetCopyParamIndex(size_t value);
  TGUICopyParamType __fastcall GetCurrentCopyParam();
  TGUICopyParamType __fastcall GetCopyParamPreset(const UnicodeString Name);
  TRemoteProperties __fastcall GetNewDirectoryProperties() { return FNewDirectoryProperties; }
  void __fastcall SetNewDirectoryProperties(const TRemoteProperties & value);
  int __fastcall GetKeepUpToDateChangeDelay() { return FKeepUpToDateChangeDelay; }
  void __fastcall SetKeepUpToDateChangeDelay(int value) { FKeepUpToDateChangeDelay = value; }
  UnicodeString __fastcall GetChecksumAlg() { return FChecksumAlg; }
  void __fastcall SetChecksumAlg(const UnicodeString value) { FChecksumAlg = value; }
  int __fastcall GetSessionReopenAutoIdle() { return FSessionReopenAutoIdle; }
  void __fastcall SetSessionReopenAutoIdle(int value) { FSessionReopenAutoIdle = value; }
#endif
};
//---------------------------------------------------------------------------
#define GUIConfiguration (dynamic_cast<TGUIConfiguration *>(Configuration))
//---------------------------------------------------------------------------
#endif
