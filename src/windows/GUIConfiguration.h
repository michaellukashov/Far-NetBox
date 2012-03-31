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

    void __fastcall Load(THierarchicalStorage *Storage);
    void __fastcall Save(THierarchicalStorage *Storage);

    virtual void __fastcall Default();
    virtual void __fastcall Assign(const TCopyParamType *Source);
    TGUICopyParamType & __fastcall operator =(const TGUICopyParamType &rhp);
    TGUICopyParamType & __fastcall operator =(const TCopyParamType &rhp);

    bool GetQueue() const { return FQueue; }
    void SetQueue(bool value) { FQueue = value; }
    bool GetQueueNoConfirmation() const { return FQueueNoConfirmation; }
    void SetQueueNoConfirmation(bool value) { FQueueNoConfirmation = value; }
    bool GetQueueIndividually() const { return FQueueIndividually; }
    void SetQueueIndividually(bool value) { FQueueIndividually = value; }
    bool GetNewerOnly() const { return FNewerOnly; }
    void SetNewerOnly(bool value) { FNewerOnly = value; }

protected:
    void __fastcall GUIDefault();
    void __fastcall GUIAssign(const TGUICopyParamType *Source);

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

    bool __fastcall Matches(const TCopyParamRuleData &Value) const;
    void __fastcall Load(THierarchicalStorage *Storage);
    void __fastcall Save(THierarchicalStorage *Storage) const;

    std::wstring __fastcall GetInfoStr(const std::wstring Separator) const;

    bool __fastcall operator ==(const TCopyParamRule &rhp) const;

    TCopyParamRuleData __fastcall GetData() { return FData; }
    void __fastcall SetData(TCopyParamRuleData value) { FData = value; }
    bool __fastcall GetEmpty() const;

private:
    TCopyParamRuleData FData;

    inline bool __fastcall Match(const std::wstring Mask,
                      const std::wstring Value, bool Path, bool Local = true) const;
};
//---------------------------------------------------------------------------
class TCopyParamList
{
    friend class TGUIConfiguration;
public:
    TCopyParamList();
    virtual ~TCopyParamList();
    int __fastcall Find(const TCopyParamRuleData &Value) const;

    void __fastcall Load(THierarchicalStorage *Storage, int Count);
    void __fastcall Save(THierarchicalStorage *Storage) const;

    static void __fastcall ValidateName(const std::wstring Name);

    void __fastcall operator=(const TCopyParamList &rhl);
    bool __fastcall operator==(const TCopyParamList &rhl) const;

    void __fastcall Clear();
    void __fastcall Add(const std::wstring Name,
             TCopyParamType *CopyParam, TCopyParamRule *Rule);
    void __fastcall Insert(size_t Index, const std::wstring Name,
                TCopyParamType *CopyParam, TCopyParamRule *Rule);
    void __fastcall Change(size_t Index, const std::wstring Name,
                TCopyParamType *CopyParam, TCopyParamRule *Rule);
    void __fastcall Move(size_t CurIndex, size_t NewIndex);
    void __fastcall Delete(size_t Index);
    size_t __fastcall IndexOfName(const std::wstring Name) const;

    size_t __fastcall GetCount() const;
    std::wstring __fastcall GetName(size_t Index) const;
    const TCopyParamRule * __fastcall GetRule(size_t Index) const;
    const TCopyParamType * __fastcall GetCopyParam(size_t Index) const;
    bool __fastcall GetModified() { return FModified; }
    nb::TStrings * __fastcall GetNameList() const;
    bool __fastcall GetAnyRule() const;

private:
    static std::wstring FInvalidChars;
    nb::TObjectList *FRules;
    nb::TObjectList *FCopyParams;
    nb::TStrings *FNames;
    mutable nb::TStrings *FNameList;
    bool FModified;

    void __fastcall Init();
    void __fastcall Reset();
    void __fastcall Modify();
    bool __fastcall CompareItem(size_t Index, const TCopyParamType *CopyParam,
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

    virtual void __fastcall SaveData(THierarchicalStorage *Storage, bool All);
    virtual void __fastcall LoadData(THierarchicalStorage *Storage);
    virtual HANDLE __fastcall LoadNewResourceModule(LCID Locale,
                                         std::wstring *FileName = NULL);
    LCID __fastcall InternalLocale();
    virtual bool __fastcall GetRememberPassword();
    static std::wstring __fastcall PropertyToKey(const std::wstring Property);
    virtual void __fastcall DefaultLocalized();
    virtual void __fastcall Saved();

public:
    TGUIConfiguration();
    virtual ~TGUIConfiguration();
    virtual void __fastcall Default();

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
    nb::TStrings * __fastcall GetLocales();
    std::wstring __fastcall GetPuttyPath() { return FPuttyPath; }
    void __fastcall SetPuttyPath(const std::wstring value) { FPuttyPath = value; }
    std::wstring __fastcall GetDefaultPuttyPath() { return FDefaultPuttyPath; }
    std::wstring __fastcall GetPSftpPath() { return FPSftpPath; }
    void __fastcall SetPSftpPath(const std::wstring value) { FPSftpPath = value; }
    bool __fastcall GetPuttyPassword() { return FPuttyPassword; }
    void __fastcall SetPuttyPassword(bool value) { FPuttyPassword = value; }
    bool __fastcall GetTelnetForFtpInPutty() { return FTelnetForFtpInPutty; }
    void __fastcall SetTelnetForFtpInPutty(bool value) { FTelnetForFtpInPutty = value; }
    std::wstring __fastcall GetPuttySession() { return FPuttySession; }
    void __fastcall SetPuttySession(std::wstring value) { FPuttySession = value; }
    nb::TDateTime __fastcall GetIgnoreCancelBeforeFinish() { return FIgnoreCancelBeforeFinish; }
    void __fastcall SetIgnoreCancelBeforeFinish(nb::TDateTime value) { FIgnoreCancelBeforeFinish = value; }
    TGUICopyParamType & __fastcall GetDefaultCopyParam() { return FDefaultCopyParam; }
    void __fastcall SetDefaultCopyParam(const TGUICopyParamType &value);
    bool __fastcall GetBeepOnFinish() { return FBeepOnFinish; }
    void __fastcall SetBeepOnFinish(bool value) { FBeepOnFinish = value; }
    bool __fastcall GetSynchronizeBrowsing() { return FSynchronizeBrowsing; }
    void __fastcall SetSynchronizeBrowsing(bool value) { FSynchronizeBrowsing = value; }
    nb::TDateTime __fastcall GetBeepOnFinishAfter() { return FBeepOnFinishAfter; }
    void __fastcall SetBeepOnFinishAfter(nb::TDateTime value) { FBeepOnFinishAfter = value; }
    const TCopyParamList * __fastcall GetCopyParamList();
    void __fastcall SetCopyParamList(const TCopyParamList *value);
    std::wstring __fastcall GetCopyParamCurrent() { return FCopyParamCurrent; }
    void __fastcall SetCopyParamCurrent(const std::wstring value);
    size_t __fastcall GetCopyParamIndex();
    void __fastcall SetCopyParamIndex(size_t value);
    TGUICopyParamType __fastcall GetCurrentCopyParam();
    TGUICopyParamType __fastcall GetCopyParamPreset(const std::wstring Name);
    TRemoteProperties __fastcall GetNewDirectoryProperties() { return FNewDirectoryProperties; }
    void __fastcall SetNewDirectoryProperties(const TRemoteProperties &value);
    int __fastcall GetKeepUpToDateChangeDelay() { return FKeepUpToDateChangeDelay; }
    void __fastcall SetKeepUpToDateChangeDelay(int value) { FKeepUpToDateChangeDelay = value; }
    std::wstring __fastcall GetChecksumAlg() { return FChecksumAlg; }
    void __fastcall SetChecksumAlg(const std::wstring value) { FChecksumAlg = value; }
    int __fastcall GetSessionReopenAutoIdle() { return FSessionReopenAutoIdle; }
    void __fastcall SetSessionReopenAutoIdle(int value) { FSessionReopenAutoIdle = value; }
};
//---------------------------------------------------------------------------
#define GUIConfiguration (dynamic_cast<TGUIConfiguration *>(Configuration))
//---------------------------------------------------------------------------
#endif
