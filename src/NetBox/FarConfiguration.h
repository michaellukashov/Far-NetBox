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
  explicit /* __fastcall */ TFarConfiguration(TCustomFarPlugin * APlugin);
  virtual /* __fastcall */ ~TFarConfiguration();

  TCustomFarPlugin * __fastcall GetPlugin() { return FFarPlugin; }
  void __fastcall SetPlugin(TCustomFarPlugin * value);
  bool __fastcall GetConfirmOverwritingOverride() { return FConfirmOverwritingOverride; }
  void __fastcall SetConfirmOverwritingOverride(bool value) { FConfirmOverwritingOverride = value; }
  bool __fastcall GetConfirmDeleting();
  bool __fastcall GetConfirmSynchronizedBrowsing() { return FConfirmSynchronizedBrowsing; }
  void __fastcall SetConfirmSynchronizedBrowsing(bool value) { FConfirmSynchronizedBrowsing = value; }
  bool GetDisksMenu() { return FDisksMenu; }
  void SetDisksMenu(bool value) { FDisksMenu = value; }
  int __fastcall GetDisksMenuHotKey() { return FDisksMenuHotKey; }
  void __fastcall SetDisksMenuHotKey(int value) { FDisksMenuHotKey = value; }
  bool __fastcall GetPluginsMenu() { return FPluginsMenu; }
  void __fastcall SetPluginsMenu(bool value) { FPluginsMenu = value; }
  bool __fastcall GetPluginsMenuCommands() { return FPluginsMenuCommands; }
  void __fastcall SetPluginsMenuCommands(bool value) { FPluginsMenuCommands = value; }
  UnicodeString __fastcall GetCommandPrefixes() { return FCommandPrefixes; }
  void __fastcall SetCommandPrefixes(const UnicodeString value) { FCommandPrefixes = value; }
  bool __fastcall GetHostNameInTitle() { return FHostNameInTitle; }
  void __fastcall SetHostNameInTitle(bool value) { FHostNameInTitle = value; }

  bool __fastcall GetCustomPanelModeDetailed() { return FCustomPanelModeDetailed; }
  void __fastcall SetCustomPanelModeDetailed(bool value) { FCustomPanelModeDetailed = value; }
  bool __fastcall GetFullScreenDetailed() { return FFullScreenDetailed; }
  void __fastcall SetFullScreenDetailed(bool value) { FFullScreenDetailed = value; }
  UnicodeString __fastcall GetColumnTypesDetailed() { return FColumnTypesDetailed; }
  void __fastcall SetColumnTypesDetailed(const UnicodeString value) { FColumnTypesDetailed = value; }
  UnicodeString __fastcall GetColumnWidthsDetailed() { return FColumnWidthsDetailed; }
  void __fastcall SetColumnWidthsDetailed(const UnicodeString value) { FColumnWidthsDetailed = value; }
  UnicodeString __fastcall GetStatusColumnTypesDetailed() { return FStatusColumnTypesDetailed; }
  void __fastcall SetStatusColumnTypesDetailed(const UnicodeString value) { FStatusColumnTypesDetailed = value; }
  UnicodeString __fastcall GetStatusColumnWidthsDetailed() { return FStatusColumnWidthsDetailed; }
  void __fastcall SetStatusColumnWidthsDetailed(const UnicodeString value) { FStatusColumnWidthsDetailed = value; }
  bool __fastcall GetEditorDownloadDefaultMode() { return FEditorDownloadDefaultMode; }
  void __fastcall SetEditorDownloadDefaultMode(bool value) { FEditorDownloadDefaultMode = value; }
  bool __fastcall GetEditorUploadSameOptions() { return FEditorUploadSameOptions; }
  void __fastcall SetEditorUploadSameOptions(bool value) { FEditorUploadSameOptions = value; }
  bool __fastcall GetEditorUploadOnSave() { return FEditorUploadOnSave; }
  void __fastcall SetEditorUploadOnSave(bool value) { FEditorUploadOnSave = value; }
  bool __fastcall GetEditorMultiple() { return FEditorMultiple; }
  void __fastcall SetEditorMultiple(bool value) { FEditorMultiple = value; }
  bool __fastcall GetQueueBeep() { return FQueueBeep; }
  void __fastcall SetQueueBeep(bool value) { FQueueBeep = value; }

  UnicodeString __fastcall GetApplyCommandCommand() { return FApplyCommandCommand; }
  void __fastcall SetApplyCommandCommand(const UnicodeString value) { FApplyCommandCommand = value; }
  int __fastcall GetApplyCommandParams() { return FApplyCommandParams; }
  void __fastcall SetApplyCommandParams(int value) { FApplyCommandParams = value; }

  UnicodeString __fastcall GetPageantPath() const { return FPageantPath; }
  void __fastcall SetPageantPath(const UnicodeString value) { FPageantPath = value; }
  UnicodeString __fastcall GetPuttygenPath() const { return FPuttygenPath; }
  void __fastcall SetPuttygenPath(const UnicodeString value) { FPuttygenPath = value; }
  TBookmarkList * __fastcall GetBookmarks(UnicodeString Key);
  void __fastcall SetBookmarks(UnicodeString Key, TBookmarkList * value);

  virtual void __fastcall Load();
  virtual void __fastcall Save(bool All, bool Explicit);
  virtual void __fastcall Default();
  virtual THierarchicalStorage * __fastcall CreateScpStorage(bool SessionList);
  void __fastcall CacheFarSettings();

protected:
  virtual bool __fastcall GetConfirmOverwriting();
  virtual void __fastcall SetConfirmOverwriting(bool value);

  virtual void __fastcall SaveData(THierarchicalStorage * Storage, bool All);
  virtual void __fastcall LoadData(THierarchicalStorage * Storage);

  virtual UnicodeString __fastcall ModuleFileName();
  virtual void __fastcall Saved();

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

  int __fastcall FarConfirmations();
};
//---------------------------------------------------------------------------
#define FarConfiguration ((TFarConfiguration *) Configuration)
//---------------------------------------------------------------------------
#endif
