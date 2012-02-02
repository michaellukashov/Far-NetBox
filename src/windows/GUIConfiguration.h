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
    TGUICopyParamType(const TCopyParamType &Source);
    TGUICopyParamType(const TGUICopyParamType &Source);
    ~TGUICopyParamType();

    void Load(THierarchicalStorage *Storage);
    void Save(THierarchicalStorage *Storage);

    virtual void Default();
    virtual void Assign(const TCopyParamType *Source);
    TGUICopyParamType &operator =(const TGUICopyParamType &rhp);
    TGUICopyParamType &operator =(const TCopyParamType &rhp);

    // __property bool Queue = { read = FQueue, write = FQueue };
    bool GetQueue() const { return FQueue; }
    void SetQueue(bool value) { FQueue = value; }
    // __property bool QueueNoConfirmation = { read = FQueueNoConfirmation, write = FQueueNoConfirmation };
    bool GetQueueNoConfirmation() const { return FQueueNoConfirmation; }
    void SetQueueNoConfirmation(bool value) { FQueueNoConfirmation = value; }
    // __property bool QueueIndividually = { read = FQueueIndividually, write = FQueueIndividually };
    bool GetQueueIndividually() const { return FQueueIndividually; }
    void SetQueueIndividually(bool value) { FQueueIndividually = value; }
    // __property bool NewerOnly = { read = FNewerOnly, write = FNewerOnly };
    bool GetNewerOnly() const { return FNewerOnly; }
    void SetNewerOnly(bool value) { FNewerOnly = value; }

protected:
    void GUIDefault();
    void GUIAssign(const TGUICopyParamType *Source);

private:
    bool FQueue;
    bool FQueueNoConfirmation;
    bool FQueueIndividually;
    bool FNewerOnly;
};
//---------------------------------------------------------------------------
struct TCopyParamRuleData
{
    std::wstring HostName;
    std::wstring UserName;
    std::wstring RemoteDirectory;
    std::wstring LocalDirectory;

    void Default();
};
//---------------------------------------------------------------------------
class TCopyParamRule
{
public:
    TCopyParamRule();
    TCopyParamRule(const TCopyParamRuleData &Data);
    TCopyParamRule(const TCopyParamRule &Source);

    bool Matches(const TCopyParamRuleData &Value) const;
    void Load(THierarchicalStorage *Storage);
    void Save(THierarchicalStorage *Storage) const;

    std::wstring GetInfoStr(const std::wstring Separator) const;

    bool operator ==(const TCopyParamRule &rhp) const;

    // __property TCopyParamRuleData Data = { read = FData, write = FData };
    TCopyParamRuleData GetData() { return FData; }
    void SetData(TCopyParamRuleData value) { FData = value; }
    // __property bool IsEmpty = { read = GetEmpty };
    bool GetEmpty() const;

private:
    TCopyParamRuleData FData;

    inline bool Match(const std::wstring Mask,
                      const std::wstring Value, bool Path, bool Local = true) const;
};
//---------------------------------------------------------------------------
class TCopyParamList
{
    friend class TGUIConfiguration;
public:
    TCopyParamList();
    virtual ~TCopyParamList();
    int Find(const TCopyParamRuleData &Value) const;

    void Load(THierarchicalStorage *Storage, int Count);
    void Save(THierarchicalStorage *Storage) const;

    static void ValidateName(const std::wstring Name);

    void operator=(const TCopyParamList &rhl);
    bool operator==(const TCopyParamList &rhl) const;

    void Clear();
    void Add(const std::wstring Name,
             TCopyParamType *CopyParam, TCopyParamRule *Rule);
    void Insert(size_t Index, const std::wstring Name,
                TCopyParamType *CopyParam, TCopyParamRule *Rule);
    void Change(size_t Index, const std::wstring Name,
                TCopyParamType *CopyParam, TCopyParamRule *Rule);
    void Move(size_t CurIndex, size_t NewIndex);
    void Delete(size_t Index);
    size_t IndexOfName(const std::wstring Name) const;

    // __property int Count = { read = GetCount };
    size_t GetCount() const;
    // __property std::wstring Names[int Index] = { read = GetName };
    std::wstring GetName(size_t Index) const;
    // __property const TCopyParamRule * Rules[int Index] = { read = GetRule };
    const TCopyParamRule *GetRule(size_t Index) const;
    // __property const TCopyParamType * CopyParams[int Index] = { read = GetCopyParam };
    const TCopyParamType *GetCopyParam(size_t Index) const;
    // __property bool Modified = { read = FModified };
    bool GetModified() { return FModified; }
    // __property nb::TStrings * NameList = { read = GetNameList };
    nb::TStrings *GetNameList() const;
    // __property bool AnyRule = { read = GetAnyRule };
    bool GetAnyRule() const;

private:
    static std::wstring FInvalidChars;
    nb::TObjectList *FRules;
    nb::TObjectList *FCopyParams;
    nb::TStrings *FNames;
    mutable nb::TStrings *FNameList;
    bool FModified;

    void Init();
    void Reset();
    void Modify();
    bool CompareItem(size_t Index, const TCopyParamType *CopyParam,
                     const TCopyParamRule *Rule) const;
};
//---------------------------------------------------------------------------
class TGUIConfiguration : public TConfiguration
{
private:
    nb::TStrings *FLocales;
    std::wstring FLastLocalesExts;
    bool FContinueOnError;
    bool FConfirmCommandSession;
    std::wstring FPuttyPath;
    std::wstring FPSftpPath;
    bool FPuttyPassword;
    bool FTelnetForFtpInPutty;
    std::wstring FPuttySession;
    int FSynchronizeParams;
    int FSynchronizeOptions;
    int FSynchronizeModeAuto;
    int FSynchronizeMode;
    int FMaxWatchDirectories;
    nb::TDateTime FIgnoreCancelBeforeFinish;
    bool FQueueAutoPopup;
    bool FQueueRememberPassword;
    size_t FQueueTransfersLimit;
    TGUICopyParamType FDefaultCopyParam;
    bool FBeepOnFinish;
    nb::TDateTime FBeepOnFinishAfter;
    std::wstring FDefaultPuttyPathOnly;
    std::wstring FDefaultPuttyPath;
    bool FSynchronizeBrowsing;
    TCopyParamList *FCopyParamList;
    bool FCopyParamListDefaults;
    std::wstring FCopyParamCurrent;
    TRemoteProperties FNewDirectoryProperties;
    int FKeepUpToDateChangeDelay;
    std::wstring FChecksumAlg;
    int FSessionReopenAutoIdle;

protected:
    LCID FLocale;

    virtual void SaveData(THierarchicalStorage *Storage, bool All);
    virtual void LoadData(THierarchicalStorage *Storage);
    virtual HANDLE LoadNewResourceModule(LCID Locale,
                                         std::wstring *FileName = NULL);
    HANDLE GetResourceModule();
    virtual void SetResourceModule(HANDLE Instance);
    LCID InternalLocale();
    void FreeResourceModule(HANDLE Instance);
    virtual bool GetRememberPassword();
    static std::wstring PropertyToKey(const std::wstring Property);
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
    size_t GetQueueTransfersLimit() { return FQueueTransfersLimit; }
    void SetQueueTransfersLimit(size_t value) { FQueueTransfersLimit = value; }
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
    // __property nb::TStrings * Locales = { read = GetLocales };
    nb::TStrings *GetLocales();
    // __property std::wstring PuttyPath = { read = FPuttyPath, write = FPuttyPath };
    std::wstring GetPuttyPath() { return FPuttyPath; }
    void SetPuttyPath(const std::wstring value) { FPuttyPath = value; }
    // __property std::wstring DefaultPuttyPath = { read = FDefaultPuttyPath };
    std::wstring GetDefaultPuttyPath() { return FDefaultPuttyPath; }
    // __property std::wstring PSftpPath = { read = FPSftpPath, write = FPSftpPath };
    std::wstring GetPSftpPath() { return FPSftpPath; }
    void SetPSftpPath(const std::wstring value) { FPSftpPath = value; }
    // __property bool PuttyPassword = { read = FPuttyPassword, write = FPuttyPassword };
    bool GetPuttyPassword() { return FPuttyPassword; }
    void SetPuttyPassword(bool value) { FPuttyPassword = value; }
    // __property bool TelnetForFtpInPutty = { read = FTelnetForFtpInPutty, write = FTelnetForFtpInPutty };
    bool GetTelnetForFtpInPutty() { return FTelnetForFtpInPutty; }
    void SetTelnetForFtpInPutty(bool value) { FTelnetForFtpInPutty = value; }
    // __property std::wstring PuttySession = { read = FPuttySession, write = FPuttySession };
    std::wstring GetPuttySession() { return FPuttySession; }
    void SetPuttySession(std::wstring value) { FPuttySession = value; }
    // __property nb::TDateTime IgnoreCancelBeforeFinish = { read = FIgnoreCancelBeforeFinish, write = FIgnoreCancelBeforeFinish };
    nb::TDateTime GetIgnoreCancelBeforeFinish() { return FIgnoreCancelBeforeFinish; }
    void SetIgnoreCancelBeforeFinish(nb::TDateTime value) { FIgnoreCancelBeforeFinish = value; }
    // __property TGUICopyParamType DefaultCopyParam = { read = FDefaultCopyParam, write = SetDefaultCopyParam };
    TGUICopyParamType &GetDefaultCopyParam() { return FDefaultCopyParam; }
    void SetDefaultCopyParam(const TGUICopyParamType &value);
    // __property bool BeepOnFinish = { read = FBeepOnFinish, write = FBeepOnFinish };
    bool GetBeepOnFinish() { return FBeepOnFinish; }
    void SetBeepOnFinish(bool value) { FBeepOnFinish = value; }
    // __property bool SynchronizeBrowsing = { read = FSynchronizeBrowsing, write = FSynchronizeBrowsing };
    bool GetSynchronizeBrowsing() { return FSynchronizeBrowsing; }
    void SetSynchronizeBrowsing(bool value) { FSynchronizeBrowsing = value; }
    // __property nb::TDateTime BeepOnFinishAfter = { read = FBeepOnFinishAfter, write = FBeepOnFinishAfter };
    nb::TDateTime GetBeepOnFinishAfter() { return FBeepOnFinishAfter; }
    void SetBeepOnFinishAfter(nb::TDateTime value) { FBeepOnFinishAfter = value; }
    // __property const TCopyParamList * CopyParamList = { read = GetCopyParamList, write = SetCopyParamList };
    const TCopyParamList *GetCopyParamList();
    void SetCopyParamList(const TCopyParamList *value);
    // __property std::wstring CopyParamCurrent = { read = FCopyParamCurrent, write = SetCopyParamCurrent };
    std::wstring GetCopyParamCurrent() { return FCopyParamCurrent; }
    void SetCopyParamCurrent(const std::wstring value);
    // __property int CopyParamIndex = { read = GetCopyParamIndex, write = SetCopyParamIndex };
    size_t GetCopyParamIndex();
    void SetCopyParamIndex(size_t value);
    // __property TGUICopyParamType CurrentCopyParam = { read = GetCurrentCopyParam };
    TGUICopyParamType GetCurrentCopyParam();
    // __property TGUICopyParamType CopyParamPreset[std::wstring Name] = { read = GetCopyParamPreset };
    TGUICopyParamType GetCopyParamPreset(const std::wstring Name);
    // __property TRemoteProperties NewDirectoryProperties = { read = FNewDirectoryProperties, write = SetNewDirectoryProperties };
    TRemoteProperties GetNewDirectoryProperties() { return FNewDirectoryProperties; }
    void SetNewDirectoryProperties(const TRemoteProperties &value);
    // __property int KeepUpToDateChangeDelay = { read = FKeepUpToDateChangeDelay, write = FKeepUpToDateChangeDelay };
    int GetKeepUpToDateChangeDelay() { return FKeepUpToDateChangeDelay; }
    void SetKeepUpToDateChangeDelay(int value) { FKeepUpToDateChangeDelay = value; }
    // __property std::wstring ChecksumAlg = { read = FChecksumAlg, write = FChecksumAlg };
    std::wstring GetChecksumAlg() { return FChecksumAlg; }
    void SetChecksumAlg(const std::wstring value) { FChecksumAlg = value; }
    // __property int SessionReopenAutoIdle = { read = FSessionReopenAutoIdle, write = FSessionReopenAutoIdle };
    int GetSessionReopenAutoIdle() { return FSessionReopenAutoIdle; }
    void SetSessionReopenAutoIdle(int value) { FSessionReopenAutoIdle = value; }
};
//---------------------------------------------------------------------------
#define GUIConfiguration (dynamic_cast<TGUIConfiguration *>(Configuration))
//---------------------------------------------------------------------------
#endif
