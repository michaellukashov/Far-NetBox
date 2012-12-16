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
  virtual VersionInfo GetMinFarVersion();

  virtual void HandleException(Exception * E, int OpMode = 0);
  intptr_t MoreMessageDialog(UnicodeString Str, TStrings * MoreMessages,
    TQueryType Type, int Answers, const TMessageParams * Params = NULL);
  void ShowExtendedException(Exception * E);
  bool CopyParamCustomDialog(TCopyParamType & CopyParam,
    int CopyParamAttrs);
  virtual void SetStartupInfo(const struct PluginStartupInfo * Info);

protected:
  virtual bool HandlesFunction(THandlesFunction Function);
  virtual void GetPluginInfoEx(PLUGIN_FLAGS &Flags, TStrings * DiskMenuStrings,
    TStrings * PluginMenuStrings, TStrings * PluginConfigStrings,
    TStrings * CommandPrefixes);
  virtual TCustomFarFileSystem * OpenPluginEx(OPENFROM OpenFrom, intptr_t Item);
  virtual bool ConfigureEx(const GUID * Guid);
  virtual intptr_t ProcessEditorEventEx(const struct ProcessEditorEventInfo *Info);
  virtual intptr_t ProcessEditorInputEx(const INPUT_RECORD * Rec);
  bool CopyParamDialog(const UnicodeString & Caption, TCopyParamType & CopyParam,
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

protected:
  ArclitePrivateInfo * __fastcall GetSystemFunctions() { return static_cast<ArclitePrivateInfo *>(FStartupInfo.Private); }
  void __fastcall DeleteLocalFile(const UnicodeString & LocalFileName);
  HANDLE __fastcall CreateLocalFile(const UnicodeString & LocalFileName,
    DWORD DesiredAccess, DWORD ShareMode, DWORD CreationDisposition, DWORD FlagsAndAttributes);
  DWORD __fastcall GetLocalFileAttributes(const UnicodeString & LocalFileName);
  BOOL __fastcall SetLocalFileAttributes(const UnicodeString & LocalFileName, DWORD FileAttributes);
  BOOL __fastcall MoveLocalFile(const UnicodeString & LocalFileName, const UnicodeString & NewLocalFileName, DWORD Flags);
  BOOL __fastcall RemoveLocalDirectory(const UnicodeString & LocalDirName);
  BOOL __fastcall CreateLocalDirectory(const UnicodeString & LocalDirName, LPSECURITY_ATTRIBUTES SecurityAttributes);

private:
  void CleanupConfiguration();

private:
  bool FInitialized;
};
//---------------------------------------------------------------------------
#endif
