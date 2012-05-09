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

#ifndef _MSC_VER
  __property TCustomFarPlugin * Plugin = { read = FPlugin, write = SetPlugin };
  __property bool ConfirmOverwritingOverride = { read = FConfirmOverwritingOverride, write = FConfirmOverwritingOverride };
  __property bool ConfirmDeleting = { read = GetConfirmDeleting };
  __property bool ConfirmSynchronizedBrowsing = { read = FConfirmSynchronizedBrowsing, write = FConfirmSynchronizedBrowsing };
  __property bool DisksMenu = { read = FDisksMenu, write = FDisksMenu };
  __property int DisksMenuHotKey = { read = FDisksMenuHotKey, write = FDisksMenuHotKey };
  __property bool PluginsMenu = { read = FPluginsMenu, write = FPluginsMenu };
  __property bool PluginsMenuCommands = { read = FPluginsMenuCommands, write = FPluginsMenuCommands };
  __property AnsiString CommandPrefixes = { read = FCommandPrefixes, write = FCommandPrefixes };
  __property bool HostNameInTitle = { read = FHostNameInTitle, write = FHostNameInTitle };

  __property bool CustomPanelModeDetailed = { read = FCustomPanelModeDetailed, write = FCustomPanelModeDetailed };
  __property bool FullScreenDetailed = { read = FFullScreenDetailed, write = FFullScreenDetailed };
  __property AnsiString ColumnTypesDetailed = { read = FColumnTypesDetailed, write = FColumnTypesDetailed };
  __property AnsiString ColumnWidthsDetailed = { read = FColumnWidthsDetailed, write = FColumnWidthsDetailed };
  __property AnsiString StatusColumnTypesDetailed = { read = FStatusColumnTypesDetailed, write = FStatusColumnTypesDetailed };
  __property AnsiString StatusColumnWidthsDetailed = { read = FStatusColumnWidthsDetailed, write = FStatusColumnWidthsDetailed };
  __property bool EditorDownloadDefaultMode = { read = FEditorDownloadDefaultMode, write = FEditorDownloadDefaultMode };
  __property bool EditorUploadSameOptions = { read = FEditorUploadSameOptions, write = FEditorUploadSameOptions };
  __property bool EditorUploadOnSave = { read = FEditorUploadOnSave, write = FEditorUploadOnSave };
  __property bool EditorMultiple = { read = FEditorMultiple, write = FEditorMultiple };
  __property bool QueueBeep = { read = FQueueBeep, write = FQueueBeep };

  __property AnsiString ApplyCommandCommand = { read = FApplyCommandCommand, write = FApplyCommandCommand };
  __property int ApplyCommandParams = { read = FApplyCommandParams, write = FApplyCommandParams };

  __property AnsiString PageantPath = { read = FPageantPath, write = FPageantPath };
  __property AnsiString PuttygenPath = { read = FPuttygenPath, write = FPuttygenPath };
  __property TBookmarkList * Bookmarks[AnsiString Key] = { read = GetBookmarks, write = SetBookmarks };
#else
  TCustomFarPlugin * GetPlugin() { return FFarPlugin; }
  bool GetConfirmOverwritingOverride() { return FConfirmOverwritingOverride; }
  void SetConfirmOverwritingOverride(bool value) { FConfirmOverwritingOverride = value; }
  bool GetConfirmSynchronizedBrowsing() { return FConfirmSynchronizedBrowsing; }
  void SetConfirmSynchronizedBrowsing(bool value) { FConfirmSynchronizedBrowsing = value; }
  bool GetDisksMenu() { return FDisksMenu; }
  void SetDisksMenu(bool value) { FDisksMenu = value; }
  int GetDisksMenuHotKey() { return FDisksMenuHotKey; }
  void SetDisksMenuHotKey(int value) { FDisksMenuHotKey = value; }
  bool GetPluginsMenu() { return FPluginsMenu; }
  void SetPluginsMenu(bool value) { FPluginsMenu = value; }
  bool GetPluginsMenuCommands() { return FPluginsMenuCommands; }
  void SetPluginsMenuCommands(bool value) { FPluginsMenuCommands = value; }
  UnicodeString GetCommandPrefixes() { return FCommandPrefixes; }
  void SetCommandPrefixes(const UnicodeString value) { FCommandPrefixes = value; }
  bool GetHostNameInTitle() { return FHostNameInTitle; }
  void SetHostNameInTitle(bool value) { FHostNameInTitle = value; }

  bool GetCustomPanelModeDetailed() { return FCustomPanelModeDetailed; }
  void SetCustomPanelModeDetailed(bool value) { FCustomPanelModeDetailed = value; }
  bool GetFullScreenDetailed() { return FFullScreenDetailed; }
  void SetFullScreenDetailed(bool value) { FFullScreenDetailed = value; }
  UnicodeString GetColumnTypesDetailed() { return FColumnTypesDetailed; }
  void SetColumnTypesDetailed(const UnicodeString value) { FColumnTypesDetailed = value; }
  UnicodeString GetColumnWidthsDetailed() { return FColumnWidthsDetailed; }
  void SetColumnWidthsDetailed(const UnicodeString value) { FColumnWidthsDetailed = value; }
  UnicodeString GetStatusColumnTypesDetailed() { return FStatusColumnTypesDetailed; }
  void SetStatusColumnTypesDetailed(const UnicodeString value) { FStatusColumnTypesDetailed = value; }
  UnicodeString GetStatusColumnWidthsDetailed() { return FStatusColumnWidthsDetailed; }
  void SetStatusColumnWidthsDetailed(const UnicodeString value) { FStatusColumnWidthsDetailed = value; }
  bool GetEditorDownloadDefaultMode() { return FEditorDownloadDefaultMode; }
  void SetEditorDownloadDefaultMode(bool value) { FEditorDownloadDefaultMode = value; }
  bool GetEditorUploadSameOptions() { return FEditorUploadSameOptions; }
  void SetEditorUploadSameOptions(bool value) { FEditorUploadSameOptions = value; }
  bool GetEditorUploadOnSave() { return FEditorUploadOnSave; }
  void SetEditorUploadOnSave(bool value) { FEditorUploadOnSave = value; }
  bool GetEditorMultiple() { return FEditorMultiple; }
  void SetEditorMultiple(bool value) { FEditorMultiple = value; }
  bool GetQueueBeep() { return FQueueBeep; }
  void SetQueueBeep(bool value) { FQueueBeep = value; }

  UnicodeString GetApplyCommandCommand() { return FApplyCommandCommand; }
  void SetApplyCommandCommand(const UnicodeString value) { FApplyCommandCommand = value; }
  int GetApplyCommandParams() { return FApplyCommandParams; }
  void SetApplyCommandParams(int value) { FApplyCommandParams = value; }

  void SetPageantPath(const UnicodeString value) { FPageantPath = value; }
  void SetPuttygenPath(const UnicodeString value) { FPuttygenPath = value; }
#endif

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

public:
  void __fastcall SetPlugin(TCustomFarPlugin * value);
  bool __fastcall GetConfirmDeleting();
  void __fastcall SetBookmarks(UnicodeString Key, TBookmarkList * value);
  TBookmarkList * __fastcall GetBookmarks(UnicodeString Key);
  int __fastcall FarConfirmations();

  UnicodeString __fastcall GetPageantPath() const { return FPageantPath; }
  UnicodeString __fastcall GetPuttygenPath() const { return FPuttygenPath; }
};
//---------------------------------------------------------------------------
#define FarConfiguration ((TFarConfiguration *) Configuration)
//---------------------------------------------------------------------------
#endif
