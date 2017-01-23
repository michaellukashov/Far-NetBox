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
NB_DECLARE_CLASS(TWinSCPPlugin)
public:
  explicit TWinSCPPlugin(HINSTANCE HInst);
  virtual ~TWinSCPPlugin();
  virtual VersionInfo GetMinFarVersion() const;

  virtual void HandleException(Exception * E, OPERATION_MODES OpMode = 0);
  uintptr_t MoreMessageDialog(const UnicodeString & Str, TStrings * MoreMessages,
    TQueryType Type, uintptr_t Answers, const TMessageParams * Params = nullptr);
  void ShowExtendedException(Exception * E);
  bool CopyParamCustomDialog(TCopyParamType & CopyParam,
    intptr_t CopyParamAttrs);
  virtual void SetStartupInfo(const struct PluginStartupInfo * Info);

protected:
  virtual bool HandlesFunction(THandlesFunction Function) const;
  virtual void GetPluginInfoEx(PLUGIN_FLAGS & Flags, TStrings * DiskMenuStrings,
    TStrings * PluginMenuStrings, TStrings * PluginConfigStrings,
    TStrings * CommandPrefixes);
  virtual TCustomFarFileSystem * OpenPluginEx(OPENFROM OpenFrom, intptr_t Item);
  virtual bool ConfigureEx(const GUID * Guid);
  virtual intptr_t ProcessEditorEventEx(const struct ProcessEditorEventInfo * Info);
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

protected:
  NetBoxPrivateInfo * GetSystemFunctions() { return static_cast<NetBoxPrivateInfo *>(FStartupInfo.Private); }
  void DeleteLocalFile(const UnicodeString & LocalFileName);
  HANDLE CreateLocalFile(const UnicodeString & LocalFileName,
    DWORD DesiredAccess, DWORD ShareMode, DWORD CreationDisposition, DWORD FlagsAndAttributes);
  DWORD GetLocalFileAttributes(const UnicodeString & LocalFileName);
  BOOL SetLocalFileAttributes(const UnicodeString & LocalFileName, DWORD FileAttributes);
  BOOL MoveLocalFile(const UnicodeString & LocalFileName, const UnicodeString & NewLocalFileName, DWORD Flags);
  BOOL RemoveLocalDirectory(const UnicodeString & LocalDirName);
  BOOL CreateLocalDirectory(const UnicodeString & LocalDirName, LPSECURITY_ATTRIBUTES SecurityAttributes);

private:
  void CleanupConfiguration();
  void CoreInitializeOnce();
  void ParseCommandLine(UnicodeString & CommandLine,
    TOptions * Options);

private:
  bool FInitialized;
};

