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
    queryparamstimer_slot_type *TimerEvent;
    std::wstring TimerMessage;
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
    virtual VersionInfo GetMinFarVersion();

    virtual void HandleException(const std::exception *E, int OpMode = 0);
    int MoreMessageDialog(const std::wstring Str, nb::TStrings *MoreMessages,
                          TQueryType Type, int Answers, const TMessageParams *Params = NULL);
    void ShowExtendedException(const std::exception *E);
    bool CopyParamCustomDialog(TCopyParamType &CopyParam,
                               int CopyParamAttrs);
    virtual void SetStartupInfo(const struct PluginStartupInfo *Info);

protected:
    virtual bool HandlesFunction(THandlesFunction Function);
    virtual void GetPluginInfoEx(PLUGIN_FLAGS &Flags, nb::TStrings * DiskMenuStrings,
                                 nb::TStrings *PluginMenuStrings, nb::TStrings *PluginConfigStrings,
                                 nb::TStrings *CommandPrefixes);
    virtual TCustomFarFileSystem *OpenPluginEx(OPENFROM OpenFrom, INT_PTR Item);
    virtual bool ImportSessions();
    virtual bool ConfigureEx(int Item);
    virtual int ProcessEditorEventEx(const struct ProcessEditorEventInfo *Info);
    virtual int ProcessEditorInputEx(const INPUT_RECORD *Rec);
    bool CopyParamDialog(const std::wstring Caption, TCopyParamType &CopyParam,
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
    bool ImportSessions(const std::wstring RegistryStorageKey, int &imported);

private:
    bool FInitialized;
    TWinSCPPlugin *Self;
};
//---------------------------------------------------------------------------
#endif
