#pragma once

#include <Interface.h>
#include <WinInterface.h>
#include <Option.h>
#include "FarPlugin.h"

class TWinSCPFileSystem;
class TCopyParamType;

class TWinSCPPlugin final : public TCustomFarPlugin
{
  friend class TWinSCPFileSystem;
public:
  static bool classof(const TObject * Obj) { return Obj->is(OBJECT_CLASS_TWinSCPPlugin); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TWinSCPPlugin) || TCustomFarPlugin::is(Kind); }
  TWinSCPPlugin() = delete;
public:
  explicit TWinSCPPlugin(HINSTANCE HInst) noexcept;
  virtual ~TWinSCPPlugin() noexcept override;

  virtual void HandleException(Exception * E, OPERATION_MODES OpMode = 0) override;
  uint32_t MoreMessageDialog(const UnicodeString & Str, TStrings * MoreMessages,
    TQueryType Type, uint32_t Answers, const TMessageParams * Params = nullptr);
  void ShowExtendedException(Exception * E);
  bool CopyParamCustomDialog(TCopyParamType & CopyParam,
    uint32_t CopyParamAttrs);
  virtual void SetStartupInfo(const struct PluginStartupInfo * Info) override;

protected:
  virtual bool HandlesFunction(THandlesFunction Function) const override;
  virtual void GetPluginInfoEx(PLUGIN_FLAGS & Flags, TStrings * DiskMenuStrings,
    TStrings * PluginMenuStrings, TStrings * PluginConfigStrings,
    TStrings * CommandPrefixes) override;
  virtual TCustomFarFileSystem * OpenPluginEx(OPENFROM OpenFrom, intptr_t Item) override;
  virtual bool ConfigureEx(const GUID * Guid) override;
  virtual int32_t ProcessEditorEventEx(const struct ProcessEditorEventInfo * Info) override;
  virtual int32_t ProcessEditorInputEx(const INPUT_RECORD * Rec) override;
  bool CopyParamDialog(const UnicodeString & Caption, TCopyParamType & CopyParam,
    uint32_t CopyParamAttrs);
  void MessageClick(void * Token, int32_t Result, bool & Close);

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
  void DeleteLocalFile(const UnicodeString & LocalFileName);
  HANDLE CreateLocalFile(const UnicodeString & LocalFileName,
    DWORD DesiredAccess, DWORD ShareMode, DWORD CreationDisposition, DWORD FlagsAndAttributes);
  DWORD GetLocalFileAttributes(const UnicodeString & LocalFileName) const;
  bool SetLocalFileAttributes(const UnicodeString & LocalFileName, DWORD FileAttributes);
  bool MoveLocalFile(const UnicodeString & LocalFileName, const UnicodeString & NewLocalFileName, DWORD Flags);
  bool RemoveLocalDirectory(const UnicodeString & LocalDirName);
  bool CreateLocalDirectory(const UnicodeString & LocalDirName, LPSECURITY_ATTRIBUTES SecurityAttributes);

private:
  void CleanupConfiguration();
  void CoreInitializeOnce();
  void ParseCommandLine(UnicodeString & CommandLine, const TOptions * Options);

private:
  bool FInitialized{false};
};

