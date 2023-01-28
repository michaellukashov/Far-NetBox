#pragma once

#include <Interface.h>
#include <WinInterface.h>
#include <Option.h>
#include "FarPlugin.h"

class TWinSCPFileSystem;
class TCopyParamType;

NB_DEFINE_CLASS_ID(TWinSCPPlugin);
class TWinSCPPlugin : public TCustomFarPlugin
{
  friend class TWinSCPFileSystem;
public:
  static bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TWinSCPPlugin); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TWinSCPPlugin) || TCustomFarPlugin::is(Kind); }
public:
  TWinSCPPlugin() = delete;
  explicit TWinSCPPlugin(HINSTANCE HInst) noexcept;
  virtual ~TWinSCPPlugin() noexcept;
  int32_t GetMinFarVersion() const override;

  void HandleException(Exception *E, int OpMode = 0) override;
  uint32_t MoreMessageDialog(const UnicodeString Str, TStrings *MoreMessages,
    TQueryType Type, uint32_t Answers, const TMessageParams *Params = nullptr);
  void ShowExtendedException(Exception *E);
  bool CopyParamCustomDialog(TCopyParamType &CopyParam,
    int32_t CopyParamAttrs);
  void SetStartupInfo(const struct PluginStartupInfo *Info) override;

protected:
  bool HandlesFunction(THandlesFunction Function) const override;
  void GetPluginInfoEx(DWORD &Flags, TStrings *DiskMenuStrings,
    TStrings *PluginMenuStrings, TStrings *PluginConfigStrings,
    TStrings *CommandPrefixes) override;
  TCustomFarFileSystem *OpenPluginEx(int32_t OpenFrom, int32_t Item) override;
  bool ConfigureEx(int32_t Item) override;
  int32_t ProcessEditorEventEx(int32_t Event, void *Param) override;
  int32_t ProcessEditorInputEx(const INPUT_RECORD *Rec) override;

  bool CopyParamDialog(const UnicodeString Caption, TCopyParamType &CopyParam,
    int32_t CopyParamAttrs);
  void MessageClick(void *Token, uint32_t Result, bool &Close);
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

protected:
  const NetBoxPrivateInfo *GetSystemFunctions() const { return static_cast<const NetBoxPrivateInfo *>(FStartupInfo.Private); }
  NetBoxPrivateInfo *GetSystemFunctions() { return static_cast<NetBoxPrivateInfo *>(FStartupInfo.Private); }
  void DeleteLocalFile(UnicodeString LocalFileName);
  HANDLE CreateLocalFile(UnicodeString LocalFileName,
    DWORD DesiredAccess, DWORD ShareMode, DWORD CreationDisposition, DWORD FlagsAndAttributes);
  DWORD GetLocalFileAttributes(UnicodeString LocalFileName) const;
  bool SetLocalFileAttributes(UnicodeString LocalFileName, DWORD FileAttributes);
  bool MoveLocalFile(UnicodeString LocalFileName, UnicodeString NewLocalFileName, DWORD Flags);
  bool RemoveLocalDirectory(UnicodeString LocalDirName);
  bool CreateLocalDirectory(UnicodeString LocalDirName, LPSECURITY_ATTRIBUTES SecurityAttributes);

private:
  void CleanupConfiguration();
  void CoreInitializeOnce();
  void ParseCommandLine(UnicodeString &CommandLine, TOptions *Options);

private:
  bool FInitialized{false};
};

