//---------------------------------------------------------------------------
#ifndef WinSCPPluginH
#define WinSCPPluginH
//---------------------------------------------------------------------------
#include <Interface.h>
#include "FarPlugin.h"
//---------------------------------------------------------------------------
class TWinSCPFileSystem;
class TCopyParamType;
//---------------------------------------------------------------------------
struct TMessageParams
{
    TMessageParams();

    const TQueryButtonAlias *Aliases;
    size_t AliasesCount;
    unsigned int Flags;
    unsigned int Params;
    unsigned int Timer;
    TQueryParamsTimerEvent *TimerEvent;
    UnicodeString TimerMessage;
    unsigned int TimerAnswers;
    unsigned int Timeout;
    unsigned int TimeoutAnswer;
};
//---------------------------------------------------------------------------
class TWinSCPPlugin : public TCustomFarPlugin
{
    friend TWinSCPFileSystem;
public:
    explicit TWinSCPPlugin(HINSTANCE HInst);
    virtual ~TWinSCPPlugin();
    virtual int GetMinFarVersion();

    virtual void HandleException(const std::exception *E, int OpMode = 0);
    int MoreMessageDialog(const UnicodeString Str, System::TStrings *MoreMessages,
                          TQueryType Type, int Answers, const TMessageParams *Params = NULL);
    void ShowExtendedException(const std::exception *E);
    bool CopyParamCustomDialog(TCopyParamType &CopyParam,
                               int CopyParamAttrs);
    virtual void SetStartupInfo(const struct PluginStartupInfo *Info);

protected:
    virtual bool HandlesFunction(THandlesFunction Function);
    virtual void GetPluginInfoEx(long unsigned &Flags, System::TStrings *DiskMenuStrings,
                                 System::TStrings *PluginMenuStrings, System::TStrings *PluginConfigStrings,
                                 System::TStrings *CommandPrefixes);
    virtual TCustomFarFileSystem *OpenPluginEx(int OpenFrom, LONG_PTR Item);
    virtual bool ImportSessions();
    virtual bool ConfigureEx(int Item);
    virtual int ProcessEditorEventEx(int Event, void *Param);
    virtual int ProcessEditorInputEx(const INPUT_RECORD *Rec);
    bool CopyParamDialog(const UnicodeString Caption, TCopyParamType &CopyParam,
                         int CopyParamAttrs);
    void MessageClick(void *Token, int Result, bool &Close);

    void CommandsMenu(bool FromFileSystem);
    bool ConfigurationDialog();
    bool PanelConfigurationDialog();
    bool TransferConfigurationDialog();
    bool QueueConfigurationDialog();
    bool EnduranceConfigurationDialog();
    bool TransferEditorConfigurationDialog();
    bool LoggingConfigurationDialog();
    bool ConfirmationsConfigurationDialog();
    bool IntegrationConfigurationDialog();
    void AboutDialog();

private:
    bool ImportSessions(const UnicodeString RegistryStorageKey, int &imported);

private:
    bool FInitialized;
    TWinSCPPlugin *Self;
};
//---------------------------------------------------------------------------
#endif
