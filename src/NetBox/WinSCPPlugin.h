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

  const TQueryButtonAlias * Aliases;
  int AliasesCount;
  unsigned int Flags;
  unsigned int Params;
  unsigned int Timer;
  TQueryParamsTimerEvent TimerEvent;
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
  virtual intptr_t GetMinFarVersion();

  virtual void HandleException(Exception * E, int OpMode = 0);
  intptr_t MoreMessageDialog(UnicodeString Str, TStrings * MoreMessages,
    TQueryType Type, int Answers, const TMessageParams * Params = NULL);
  void ShowExtendedException(Exception * E);
  bool CopyParamCustomDialog(TCopyParamType & CopyParam,
    int CopyParamAttrs);
  virtual void SetStartupInfo(const struct PluginStartupInfo * Info);

protected:
  virtual bool HandlesFunction(THandlesFunction Function);
  virtual void GetPluginInfoEx(DWORD & Flags, TStrings * DiskMenuStrings,
    TStrings * PluginMenuStrings, TStrings * PluginConfigStrings,
    TStrings * CommandPrefixes);
  virtual TCustomFarFileSystem * OpenPluginEx(intptr_t OpenFrom, intptr_t Item);
  virtual bool ConfigureEx(intptr_t Item);
  virtual intptr_t ProcessEditorEventEx(intptr_t Event, void * Param);
  virtual intptr_t ProcessEditorInputEx(const INPUT_RECORD * Rec);
  bool CopyParamDialog(const UnicodeString & Caption, TCopyParamType & CopyParam,
    int CopyParamAttrs);
  void MessageClick(void * Token, intptr_t Result, bool & Close);

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
  void CleanupConfiguration();

private:
  bool FInitialized;
};
//---------------------------------------------------------------------------
#endif
