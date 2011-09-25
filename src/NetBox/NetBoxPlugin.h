//---------------------------------------------------------------------------
#pragma once

//---------------------------------------------------------------------------
#include <Interface.h>
#include "FarPlugin.h"
#include "WinSCPPlugin.h"
//---------------------------------------------------------------------------
class TWinSCPFileSystem;
class TCopyParamType;

//---------------------------------------------------------------------------
class TNetBoxPlugin : public TWinSCPPlugin // TCustomFarPlugin
{
  typedef TWinSCPPlugin parent;
  friend TWinSCPFileSystem;
public:
  explicit TNetBoxPlugin(HINSTANCE HInst);
  virtual ~TNetBoxPlugin();
  virtual int GetMinFarVersion();

  virtual void HandleException(const std::exception * E, int OpMode = 0);
  int MoreMessageDialog(std::wstring Str, TStrings * MoreMessages,
    TQueryType Type, int Answers, const TMessageParams * Params = NULL);
  void ShowExtendedException(const std::exception * E);
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
  bool CopyParamDialog(std::wstring Caption, TCopyParamType & CopyParam,
    int CopyParamAttrs);
  void MessageClick(void * Token, int Result, bool & Close);

  void CommandsMenu(bool FromFileSystem);
  // bool ConfigurationDialog();
  // bool PanelConfigurationDialog();
  // bool TransferConfigurationDialog();
  // bool QueueConfigurationDialog();
  // bool EnduranceConfigurationDialog();
  // bool TransferEditorConfigurationDialog();
  // bool LoggingConfigurationDialog();
  // bool ConfirmationsConfigurationDialog();
  // bool IntegrationConfigurationDialog();
  // void AboutDialog();

private:
  bool FInitialized;
  TNetBoxPlugin *Self;
};
//---------------------------------------------------------------------------
