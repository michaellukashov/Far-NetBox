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
  virtual VersionInfo GetMinFarVersion() const override;

  virtual void HandleException(Exception *E, OPERATION_MODES OpMode = 0) override;
  uintptr_t MoreMessageDialog(UnicodeString Str, TStrings *MoreMessages,
    TQueryType Type, uintptr_t Answers, const TMessageParams *Params = nullptr);
  void ShowExtendedException(Exception *E);
  bool CopyParamCustomDialog(TCopyParamType &CopyParam,
    intptr_t CopyParamAttrs);
  virtual void SetStartupInfo(const struct PluginStartupInfo *Info) override;

protected:
  virtual bool HandlesFunction(THandlesFunction Function) const override;
  virtual void GetPluginInfoEx(PLUGIN_FLAGS &Flags, TStrings *DiskMenuStrings,
    TStrings *PluginMenuStrings, TStrings *PluginConfigStrings,
    TStrings *CommandPrefixes) override;
  virtual TCustomFarFileSystem *OpenPluginEx(OPENFROM OpenFrom, intptr_t Item) override;
  virtual bool ConfigureEx(const GUID *Guid) override;
  virtual intptr_t ProcessEditorEventEx(const struct ProcessEditorEventInfo *Info) override;
  virtual intptr_t ProcessEditorInputEx(const INPUT_RECORD *Rec) override;
  bool CopyParamDialog(UnicodeString Caption, TCopyParamType &CopyParam,
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

protected:
  const NetBoxPrivateInfo * GetSystemFunctions() const { return static_cast<const NetBoxPrivateInfo *>(FStartupInfo.Private); }
  NetBoxPrivateInfo * GetSystemFunctions() { return static_cast<NetBoxPrivateInfo *>(FStartupInfo.Private); }
  void DeleteLocalFile(UnicodeString LocalFileName);
  HANDLE CreateLocalFile(UnicodeString LocalFileName,
    DWORD DesiredAccess, DWORD ShareMode, DWORD CreationDisposition, DWORD FlagsAndAttributes);
  DWORD GetLocalFileAttributes(const UnicodeString & LocalFileName) const;
  BOOL SetLocalFileAttributes(const UnicodeString & LocalFileName, DWORD FileAttributes);
  BOOL MoveLocalFile(const UnicodeString & LocalFileName, const UnicodeString & NewLocalFileName, DWORD Flags);
  BOOL RemoveLocalDirectory(const UnicodeString & LocalDirName);
  BOOL CreateLocalDirectory(const UnicodeString & LocalDirName, LPSECURITY_ATTRIBUTES SecurityAttributes);

private:
  void CleanupConfiguration();
  void CoreInitializeOnce();
  void ParseCommandLine(UnicodeString &CommandLine,
    TOptions *Options);

private:
  bool FInitialized;
};

