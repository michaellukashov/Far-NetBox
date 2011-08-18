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
  unsigned int AliasesCount;
  unsigned int Flags;
  unsigned int Params;
  unsigned int Timer;
  TQueryParamsTimerEvent TimerEvent;
  wstring TimerMessage;
  unsigned int TimerAnswers;
  unsigned int Timeout;
  unsigned int TimeoutAnswer;
};
//---------------------------------------------------------------------------
class TWinSCPPlugin : public TCustomFarPlugin
{
friend TWinSCPFileSystem;
public:
  TWinSCPPlugin(HWND AHandle);
  virtual ~TWinSCPPlugin();
  virtual int GetMinFarVersion();

  virtual void HandleException(Exception * E, int OpMode = 0);
  int MoreMessageDialog(wstring Str, TStrings * MoreMessages,
    TQueryType Type, int Answers, const TMessageParams * Params = NULL);
  void ShowExtendedException(Exception * E);
  bool CopyParamCustomDialog(TCopyParamType & CopyParam,
    int CopyParamAttrs);
  virtual void SetStartupInfo(const struct PluginStartupInfo * Info);

protected:
  virtual bool HandlesFunction(THandlesFunction Function);
  virtual void GetPluginInfoEx(long unsigned & Flags, TStrings * DiskMenuStrings,
    TStrings * PluginMenuStrings, TStrings * PluginConfigStrings,
    TStrings * CommandPrefixes);
  virtual TCustomFarFileSystem * OpenPluginEx(int OpenFrom, int Item);
  virtual bool ConfigureEx(int Item);
  virtual int ProcessEditorEventEx(int Event, void * Param);
  virtual int ProcessEditorInputEx(const INPUT_RECORD * Rec);
  virtual void OldFar();
  bool CopyParamDialog(wstring Caption, TCopyParamType & CopyParam,
    int CopyParamAttrs);
  void MessageClick(void * Token, int Result, bool & Close);

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
  bool FInitialized;
};
//---------------------------------------------------------------------------
#endif
