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
    TGUICopyParamType(const TCopyParamType &Source);
    TGUICopyParamType(const TGUICopyParamType &Source);
    ~TGUICopyParamType();

    void Load(THierarchicalStorage *Storage);
    void Save(THierarchicalStorage *Storage);

    virtual void Default();
    virtual void Assign(const TCopyParamType *Source);
    TGUICopyParamType &operator =(const TGUICopyParamType &rhp);
    TGUICopyParamType &operator =(const TCopyParamType &rhp);

    bool GetQueue() const { return FQueue; }
    void SetQueue(bool value) { FQueue = value; }
    bool GetQueueNoConfirmation() const { return FQueueNoConfirmation; }
    void SetQueueNoConfirmation(bool value) { FQueueNoConfirmation = value; }
    bool GetQueueIndividually() const { return FQueueIndividually; }
    void SetQueueIndividually(bool value) { FQueueIndividually = value; }
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

    TCopyParamRuleData GetData() { return FData; }
    void SetData(TCopyParamRuleData value) { FData = value; }
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

    size_t GetCount() const;
    std::wstring GetName(size_t Index) const;
    const TCopyParamRule *GetRule(size_t Index) const;
    const TCopyParamType *GetCopyParam(size_t Index) const;
    bool GetModified() { return FModified; }
    nb::TStrings *GetNameList() const;
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
    LCID InternalLocale();
    virtual bool GetRememberPassword();
    static std::wstring PropertyToKey(const std::wstring Property);
    virtual void DefaultLocalized();
    virtual void Saved();

public:
    TGUIConfiguration();
    virtual ~TGUIConfiguration();
    virtual void Default();

    bool GetContinueOnError() { return FContinueOnError; }
    void SetContinueOnError(bool value) { FContinueOnError = value; }
    bool GetConfirmCommandSession() { return FConfirmCommandSession; }
    void SetConfirmCommandSession(bool value) { FConfirmCommandSession = value; }
    int GetSynchronizeParams() { return FSynchronizeParams; }
    void SetSynchronizeParams(int value) { FSynchronizeParams = value; }
    int GetSynchronizeOptions() { return FSynchronizeOptions; }
    void SetSynchronizeOptions(int value) { FSynchronizeOptions = value; }
    int GetSynchronizeModeAuto() { return FSynchronizeModeAuto; }
    void SetSynchronizeModeAuto(int value) { FSynchronizeModeAuto = value; }
    int GetSynchronizeMode() { return FSynchronizeMode; }
    void SetSynchronizeMode(int value) { FSynchronizeMode = value; }
    int GetMaxWatchDirectories() { return FMaxWatchDirectories; }
    void SetMaxWatchDirectories(int value) { FMaxWatchDirectories = value; }
    size_t GetQueueTransfersLimit() { return FQueueTransfersLimit; }
    void SetQueueTransfersLimit(size_t value) { FQueueTransfersLimit = value; }
    bool GetQueueAutoPopup() { return FQueueAutoPopup; }
    void SetQueueAutoPopup(bool value) { FQueueAutoPopup = value; }
    bool GetQueueRememberPassword() { return FQueueRememberPassword; }
    void SetQueueRememberPassword(bool value) { FQueueRememberPassword = value; }
    virtual LCID GetLocale();
    void SetLocale(LCID value);
    void SetLocaleSafe(LCID value);
    nb::TStrings *GetLocales();
    std::wstring GetPuttyPath() { return FPuttyPath; }
    void SetPuttyPath(const std::wstring value) { FPuttyPath = value; }
    std::wstring GetDefaultPuttyPath() { return FDefaultPuttyPath; }
    std::wstring GetPSftpPath() { return FPSftpPath; }
    void SetPSftpPath(const std::wstring value) { FPSftpPath = value; }
    bool GetPuttyPassword() { return FPuttyPassword; }
    void SetPuttyPassword(bool value) { FPuttyPassword = value; }
    bool GetTelnetForFtpInPutty() { return FTelnetForFtpInPutty; }
    void SetTelnetForFtpInPutty(bool value) { FTelnetForFtpInPutty = value; }
    std::wstring GetPuttySession() { return FPuttySession; }
    void SetPuttySession(std::wstring value) { FPuttySession = value; }
    nb::TDateTime GetIgnoreCancelBeforeFinish() { return FIgnoreCancelBeforeFinish; }
    void SetIgnoreCancelBeforeFinish(nb::TDateTime value) { FIgnoreCancelBeforeFinish = value; }
    TGUICopyParamType &GetDefaultCopyParam() { return FDefaultCopyParam; }
    void SetDefaultCopyParam(const TGUICopyParamType &value);
    bool GetBeepOnFinish() { return FBeepOnFinish; }
    void SetBeepOnFinish(bool value) { FBeepOnFinish = value; }
    bool GetSynchronizeBrowsing() { return FSynchronizeBrowsing; }
    void SetSynchronizeBrowsing(bool value) { FSynchronizeBrowsing = value; }
    nb::TDateTime GetBeepOnFinishAfter() { return FBeepOnFinishAfter; }
    void SetBeepOnFinishAfter(nb::TDateTime value) { FBeepOnFinishAfter = value; }
    const TCopyParamList *GetCopyParamList();
    void SetCopyParamList(const TCopyParamList *value);
    std::wstring GetCopyParamCurrent() { return FCopyParamCurrent; }
    void SetCopyParamCurrent(const std::wstring value);
    size_t GetCopyParamIndex();
    void SetCopyParamIndex(size_t value);
    TGUICopyParamType GetCurrentCopyParam();
    TGUICopyParamType GetCopyParamPreset(const std::wstring Name);
    TRemoteProperties GetNewDirectoryProperties() { return FNewDirectoryProperties; }
    void SetNewDirectoryProperties(const TRemoteProperties &value);
    int GetKeepUpToDateChangeDelay() { return FKeepUpToDateChangeDelay; }
    void SetKeepUpToDateChangeDelay(int value) { FKeepUpToDateChangeDelay = value; }
    std::wstring GetChecksumAlg() { return FChecksumAlg; }
    void SetChecksumAlg(const std::wstring value) { FChecksumAlg = value; }
    int GetSessionReopenAutoIdle() { return FSessionReopenAutoIdle; }
    void SetSessionReopenAutoIdle(int value) { FSessionReopenAutoIdle = value; }
};
//---------------------------------------------------------------------------
#define GUIConfiguration (dynamic_cast<TGUIConfiguration *>(Configuration))
//---------------------------------------------------------------------------
#endif
