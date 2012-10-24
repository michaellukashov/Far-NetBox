//---------------------------------------------------------------------------
#ifndef FarConfigurationH
#define FarConfigurationH
//---------------------------------------------------------------------------
#include "GUIConfiguration.h"
//---------------------------------------------------------------------------
class TCustomFarPlugin;
class TBookmarks;
class TBookmarkList;
//---------------------------------------------------------------------------
class TFarConfiguration : public TGUIConfiguration
{
public:
  explicit TFarConfiguration(TCustomFarPlugin * APlugin);
  virtual ~TFarConfiguration();

  TCustomFarPlugin * __fastcall GetPlugin() { return FFarPlugin; }
  void __fastcall SetPlugin(TCustomFarPlugin * Value);
  bool __fastcall GetConfirmOverwritingOverride() { return FConfirmOverwritingOverride; }
  void __fastcall SetConfirmOverwritingOverride(bool Value) { FConfirmOverwritingOverride = Value; }
  bool __fastcall GetConfirmDeleting();
  bool __fastcall GetConfirmSynchronizedBrowsing() { return FConfirmSynchronizedBrowsing; }
  void __fastcall SetConfirmSynchronizedBrowsing(bool Value) { FConfirmSynchronizedBrowsing = Value; }
  bool GetDisksMenu() { return FDisksMenu; }
  void SetDisksMenu(bool Value) { FDisksMenu = Value; }
  int __fastcall GetDisksMenuHotKey() { return FDisksMenuHotKey; }
  void __fastcall SetDisksMenuHotKey(int Value) { FDisksMenuHotKey = Value; }
  bool __fastcall GetPluginsMenu() { return FPluginsMenu; }
  void __fastcall SetPluginsMenu(bool Value) { FPluginsMenu = Value; }
  bool __fastcall GetPluginsMenuCommands() { return FPluginsMenuCommands; }
  void __fastcall SetPluginsMenuCommands(bool Value) { FPluginsMenuCommands = Value; }
  UnicodeString __fastcall GetCommandPrefixes() { return FCommandPrefixes; }
  void __fastcall SetCommandPrefixes(const UnicodeString Value) { FCommandPrefixes = Value; }
  bool __fastcall GetHostNameInTitle() { return FHostNameInTitle; }
  void __fastcall SetHostNameInTitle(bool Value) { FHostNameInTitle = Value; }

  bool __fastcall GetCustomPanelModeDetailed() { return FCustomPanelModeDetailed; }
  void __fastcall SetCustomPanelModeDetailed(bool Value) { FCustomPanelModeDetailed = Value; }
  bool __fastcall GetFullScreenDetailed() { return FFullScreenDetailed; }
  void __fastcall SetFullScreenDetailed(bool Value) { FFullScreenDetailed = Value; }
  UnicodeString __fastcall GetColumnTypesDetailed() { return FColumnTypesDetailed; }
  void __fastcall SetColumnTypesDetailed(const UnicodeString Value) { FColumnTypesDetailed = Value; }
  UnicodeString __fastcall GetColumnWidthsDetailed() { return FColumnWidthsDetailed; }
  void __fastcall SetColumnWidthsDetailed(const UnicodeString Value) { FColumnWidthsDetailed = Value; }
  UnicodeString __fastcall GetStatusColumnTypesDetailed() { return FStatusColumnTypesDetailed; }
  void __fastcall SetStatusColumnTypesDetailed(const UnicodeString Value) { FStatusColumnTypesDetailed = Value; }
  UnicodeString __fastcall GetStatusColumnWidthsDetailed() { return FStatusColumnWidthsDetailed; }
  void __fastcall SetStatusColumnWidthsDetailed(const UnicodeString Value) { FStatusColumnWidthsDetailed = Value; }
  bool __fastcall GetEditorDownloadDefaultMode() { return FEditorDownloadDefaultMode; }
  void __fastcall SetEditorDownloadDefaultMode(bool Value) { FEditorDownloadDefaultMode = Value; }
  bool __fastcall GetEditorUploadSameOptions() { return FEditorUploadSameOptions; }
  void __fastcall SetEditorUploadSameOptions(bool Value) { FEditorUploadSameOptions = Value; }
  bool __fastcall GetEditorUploadOnSave() { return FEditorUploadOnSave; }
  void __fastcall SetEditorUploadOnSave(bool Value) { FEditorUploadOnSave = Value; }
  bool __fastcall GetEditorMultiple() { return FEditorMultiple; }
  void __fastcall SetEditorMultiple(bool Value) { FEditorMultiple = Value; }
  bool __fastcall GetQueueBeep() { return FQueueBeep; }
  void __fastcall SetQueueBeep(bool Value) { FQueueBeep = Value; }

  UnicodeString __fastcall GetApplyCommandCommand() { return FApplyCommandCommand; }
  void __fastcall SetApplyCommandCommand(const UnicodeString Value) { FApplyCommandCommand = Value; }
  int __fastcall GetApplyCommandParams() { return FApplyCommandParams; }
  void __fastcall SetApplyCommandParams(int Value) { FApplyCommandParams = Value; }

  UnicodeString __fastcall GetPageantPath() const { return FPageantPath; }
  void __fastcall SetPageantPath(const UnicodeString Value) { FPageantPath = Value; }
  UnicodeString __fastcall GetPuttygenPath() const { return FPuttygenPath; }
  void __fastcall SetPuttygenPath(const UnicodeString Value) { FPuttygenPath = Value; }
  TBookmarkList * __fastcall GetBookmarks(UnicodeString Key);
  void __fastcall SetBookmarks(UnicodeString Key, TBookmarkList * Value);

  virtual void __fastcall Load();
  virtual void __fastcall Save(bool All, bool Explicit);
  virtual void __fastcall Default();
  virtual THierarchicalStorage * __fastcall CreateScpStorage(bool SessionList);
  void __fastcall CacheFarSettings();

protected:
  virtual bool __fastcall GetConfirmOverwriting();
  virtual void __fastcall SetConfirmOverwriting(bool Value);

  virtual void __fastcall SaveData(THierarchicalStorage * Storage, bool All);
  virtual void __fastcall LoadData(THierarchicalStorage * Storage);

  virtual UnicodeString __fastcall ModuleFileName();
  virtual void __fastcall Saved();

private:
    __int64 GetSetting(FARSETTINGS_SUBFOLDERS Root, const wchar_t *Name);
    __int64 GetConfirmationsSetting(HANDLE &Settings, const wchar_t *Name);
    __int64 GetConfirmationsSettings();

private:
  TCustomFarPlugin * FFarPlugin;
  int FFarConfirmations;
  bool FConfirmOverwritingOverride;
  bool FConfirmSynchronizedBrowsing;
  bool FForceInheritance;
  bool FDisksMenu;
  int FDisksMenuHotKey;
  bool FPluginsMenu;
  bool FPluginsMenuCommands;
  UnicodeString FCommandPrefixes;
  bool FHostNameInTitle;
  bool FEditorDownloadDefaultMode;
  bool FEditorUploadSameOptions;
  bool FEditorUploadOnSave;
  bool FEditorMultiple;
  bool FQueueBeep;
  UnicodeString FPageantPath;
  UnicodeString FPuttygenPath;
  UnicodeString FApplyCommandCommand;
  int FApplyCommandParams;

  bool FCustomPanelModeDetailed;
  bool FFullScreenDetailed;
  UnicodeString FColumnTypesDetailed;
  UnicodeString FColumnWidthsDetailed;
  UnicodeString FStatusColumnTypesDetailed;
  UnicodeString FStatusColumnWidthsDetailed;

  TBookmarks * FBookmarks;
  TFarConfiguration * Self;

  __int64 __fastcall FarConfirmations();
};
//---------------------------------------------------------------------------
#define FarConfiguration (static_cast<TFarConfiguration *>(Configuration))
//---------------------------------------------------------------------------
#endif
