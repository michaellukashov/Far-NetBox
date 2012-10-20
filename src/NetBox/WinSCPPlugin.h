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
  virtual VersionInfo __fastcall GetMinFarVersion();

  virtual void __fastcall HandleException(Exception * E, int OpMode = 0);
  int __fastcall MoreMessageDialog(UnicodeString Str, TStrings * MoreMessages,
    TQueryType Type, int Answers, const TMessageParams * Params = NULL);
  void __fastcall ShowExtendedException(Exception * E);
  bool __fastcall CopyParamCustomDialog(TCopyParamType & CopyParam,
    int CopyParamAttrs);
  virtual void __fastcall SetStartupInfo(const struct PluginStartupInfo * Info);

protected:
  virtual bool __fastcall HandlesFunction(THandlesFunction Function);
  virtual void __fastcall GetPluginInfoEx(PLUGIN_FLAGS &Flags, TStrings * DiskMenuStrings,
    TStrings * PluginMenuStrings, TStrings * PluginConfigStrings,
    TStrings * CommandPrefixes);
  virtual TCustomFarFileSystem * __fastcall OpenPluginEx(OPENFROM OpenFrom, intptr_t Item);
  virtual bool __fastcall ConfigureEx(const GUID * Guid);
  virtual intptr_t __fastcall ProcessEditorEventEx(const struct ProcessEditorEventInfo *Info);
  virtual intptr_t __fastcall ProcessEditorInputEx(const INPUT_RECORD * Rec);
  bool __fastcall CopyParamDialog(UnicodeString Caption, TCopyParamType & CopyParam,
    int CopyParamAttrs);
  void MessageClick(void * Token, int Result, bool & Close);

  void __fastcall CommandsMenu(bool FromFileSystem);
  bool __fastcall ConfigurationDialog();
  bool __fastcall PanelConfigurationDialog();
  bool __fastcall TransferConfigurationDialog();
  bool __fastcall QueueConfigurationDialog();
  bool __fastcall EnduranceConfigurationDialog();
  bool __fastcall TransferEditorConfigurationDialog();
  bool __fastcall LoggingConfigurationDialog();
  bool __fastcall ConfirmationsConfigurationDialog();
  bool __fastcall IntegrationConfigurationDialog();
  void __fastcall AboutDialog();

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
  void __fastcall CleanupConfiguration();

private:
  bool FInitialized;
  TWinSCPPlugin * Self;
};
//---------------------------------------------------------------------------
#endif
