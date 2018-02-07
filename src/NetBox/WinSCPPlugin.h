#pragma once

#include <Interface.h>
#include <WinInterface.h>
#include <Option.h>
#include "FarPlugin.h"

class TWinSCPFileSystem;
class TCopyParamType;

class TWinSCPPlugin : public TCustomFarPlugin
{
  friend class TWinSCPFileSystem;
public:
  static inline bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TWinSCPPlugin); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TWinSCPPlugin) || TCustomFarPlugin::is(Kind); }
public:
  explicit TWinSCPPlugin(HINSTANCE HInst);
  virtual ~TWinSCPPlugin();
  virtual intptr_t GetMinFarVersion() const override;

  virtual void HandleException(Exception *E, int OpMode = 0) override;
  uint32_t MoreMessageDialog(const UnicodeString Str, TStrings *MoreMessages,
    TQueryType Type, uint32_t Answers, const TMessageParams *Params = nullptr);
  void ShowExtendedException(Exception *E);
  bool CopyParamCustomDialog(TCopyParamType &CopyParam,
    intptr_t CopyParamAttrs);
  virtual void SetStartupInfo(const struct PluginStartupInfo *Info) override;

protected:
  virtual bool HandlesFunction(THandlesFunction Function) const override;
  virtual void GetPluginInfoEx(DWORD &Flags, TStrings *DiskMenuStrings,
    TStrings *PluginMenuStrings, TStrings *PluginConfigStrings,
    TStrings *CommandPrefixes) override;
  virtual TCustomFarFileSystem *OpenPluginEx(intptr_t OpenFrom, intptr_t Item) override;
  virtual bool ConfigureEx(intptr_t Item) override;
  virtual intptr_t ProcessEditorEventEx(intptr_t Event, void *Param) override;
  virtual intptr_t ProcessEditorInputEx(const INPUT_RECORD *Rec) override;
  bool CopyParamDialog(const UnicodeString Caption, TCopyParamType &CopyParam,
    intptr_t CopyParamAttrs);
  void MessageClick(void *Token, uintptr_t Result, bool &Close);

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
  void CoreInitializeOnce();
  void ParseCommandLine(UnicodeString &CommandLine, TOptions *Options);

private:
  bool FInitialized;
};

