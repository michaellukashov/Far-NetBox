//---------------------------------------------------------------------------
#ifndef WinSCPPluginH
#define WinSCPPluginH
//---------------------------------------------------------------------------
#include <Interface.h>
#include <Option.h>
#include "FarPlugin.h"
//---------------------------------------------------------------------------
class TWinSCPFileSystem;
class TCopyParamType;
//---------------------------------------------------------------------------
struct TMessageParams : public TObject
{
NB_DISABLE_COPY(TMessageParams)
public:
  TMessageParams();
  void Assign(const TMessageParams * AParams)
  {
    Aliases = AParams->Aliases;
    AliasesCount = AParams->AliasesCount;
    Flags = AParams->Flags;
    Params = AParams->Params;
    Timer = AParams->Timer;
    TimerEvent = AParams->TimerEvent;
    TimerMessage = AParams->TimerMessage;
    TimerAnswers = AParams->TimerAnswers;
    Timeout = AParams->Timeout;
    TimeoutAnswer = AParams->TimeoutAnswer;
  }

  const TQueryButtonAlias * Aliases;
  uintptr_t AliasesCount;
  uintptr_t Flags;
  uintptr_t Params;
  uintptr_t Timer;
  TQueryParamsTimerEvent TimerEvent;
  UnicodeString TimerMessage;
  uintptr_t TimerAnswers;
  uintptr_t Timeout;
  uintptr_t TimeoutAnswer;
};
//---------------------------------------------------------------------------
class TWinSCPPlugin : public TCustomFarPlugin
{
  friend TWinSCPFileSystem;
  NB_DECLARE_CLASS(TWinSCPPlugin)
public:
  explicit TWinSCPPlugin(HINSTANCE HInst);
  virtual ~TWinSCPPlugin();
  virtual intptr_t GetMinFarVersion();

  virtual void HandleException(Exception * E, int OpMode = 0);
  uintptr_t MoreMessageDialog(const UnicodeString & Str, TStrings * MoreMessages,
    TQueryType Type, uintptr_t Answers, const TMessageParams * Params = nullptr);
  void ShowExtendedException(Exception * E);
  bool CopyParamCustomDialog(TCopyParamType & CopyParam,
    intptr_t CopyParamAttrs);
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
    intptr_t CopyParamAttrs);
  void MessageClick(void * Token, uintptr_t Result, bool & Close);

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
  void ParseCommandLine(UnicodeString & CommandLine,
    TOptions * Options);

private:
  bool FInitialized;
};
//---------------------------------------------------------------------------
#endif
