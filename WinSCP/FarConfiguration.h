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
  TFarConfiguration(TCustomFarPlugin * APlugin);
  virtual ~TFarConfiguration();

  // __property TCustomFarPlugin * Plugin = { read = FPlugin, write = SetPlugin };
  TCustomFarPlugin * GetPlugin() { return FPlugin; }
  void SetPlugin(TCustomFarPlugin * value);
  // __property bool ConfirmOverwritingOverride = { read = FConfirmOverwritingOverride, write = FConfirmOverwritingOverride };
  bool GetConfirmOverwritingOverride() { return FConfirmOverwritingOverride; }
  void SetConfirmOverwritingOverride(bool value) { FConfirmOverwritingOverride = value; }
  // __property bool ConfirmDeleting = { read = GetConfirmDeleting };
  bool GetConfirmDeleting();
  // __property bool ConfirmSynchronizedBrowsing = { read = FConfirmSynchronizedBrowsing, write = FConfirmSynchronizedBrowsing };
  bool GetConfirmSynchronizedBrowsing() { return FConfirmSynchronizedBrowsing; }
  void SetConfirmSynchronizedBrowsing(bool value) { FConfirmSynchronizedBrowsing = value; }
  // __property bool DisksMenu = { read = FDisksMenu, write = FDisksMenu };
  bool GetDisksMenu() { return FDisksMenu; }
  void SetDisksMenu(bool value) { FDisksMenu = value; }
  // __property int DisksMenuHotKey = { read = FDisksMenuHotKey, write = FDisksMenuHotKey };
  int GetDisksMenuHotKey() { return FDisksMenuHotKey; }
  void SetDisksMenuHotKey(int value) { FDisksMenuHotKey = value; }
  // __property bool PluginsMenu = { read = FPluginsMenu, write = FPluginsMenu };
  bool GetPluginsMenu() { return FPluginsMenu; }
  void SetPluginsMenu(bool value) { FPluginsMenu = value; }
  // __property bool PluginsMenuCommands = { read = FPluginsMenuCommands, write = FPluginsMenuCommands };
  bool GetPluginsMenuCommands() { return FPluginsMenuCommands; }
  void SetPluginsMenuCommands(bool value) { FPluginsMenuCommands = value; }
  // __property std::wstring CommandPrefixes = { read = FCommandPrefixes, write = FCommandPrefixes };
  std::wstring GetCommandPrefixes() { return FCommandPrefixes; }
  void SetCommandPrefixes(std::wstring value) { FCommandPrefixes = value; }
  // __property bool HostNameInTitle = { read = FHostNameInTitle, write = FHostNameInTitle };
  bool GetHostNameInTitle() { return FHostNameInTitle; }
  void SetHostNameInTitle(bool value) { FHostNameInTitle = value; }

  // __property bool CustomPanelModeDetailed = { read = FCustomPanelModeDetailed, write = FCustomPanelModeDetailed };
  bool GetCustomPanelModeDetailed() { return FCustomPanelModeDetailed; }
  void SetCustomPanelModeDetailed(bool value) { FCustomPanelModeDetailed = value; }
  // __property bool FullScreenDetailed = { read = FFullScreenDetailed, write = FFullScreenDetailed };
  bool GetFullScreenDetailed() { return FFullScreenDetailed; }
  void SetFullScreenDetailed(bool value) { FFullScreenDetailed = value; }
  // __property std::wstring ColumnTypesDetailed = { read = FColumnTypesDetailed, write = FColumnTypesDetailed };
  std::wstring GetColumnTypesDetailed() { return FColumnTypesDetailed; }
  void SetColumnTypesDetailed(std::wstring value) { FColumnTypesDetailed = value; }
  // __property std::wstring ColumnWidthsDetailed = { read = FColumnWidthsDetailed, write = FColumnWidthsDetailed };
  std::wstring GetColumnWidthsDetailed() { return FColumnWidthsDetailed; }
  void SetColumnWidthsDetailed(std::wstring value) { FColumnWidthsDetailed = value; }
  // __property std::wstring StatusColumnTypesDetailed = { read = FStatusColumnTypesDetailed, write = FStatusColumnTypesDetailed };
  std::wstring GetStatusColumnTypesDetailed() { return FStatusColumnTypesDetailed; }
  void SetStatusColumnTypesDetailed(std::wstring value) { FStatusColumnTypesDetailed = value; }
  // __property std::wstring StatusColumnWidthsDetailed = { read = FStatusColumnWidthsDetailed, write = FStatusColumnWidthsDetailed };
  std::wstring GetStatusColumnWidthsDetailed() { return FStatusColumnWidthsDetailed; }
  void SetStatusColumnWidthsDetailed(std::wstring value) { FStatusColumnWidthsDetailed = value; }
  // __property bool EditorDownloadDefaultMode = { read = FEditorDownloadDefaultMode, write = FEditorDownloadDefaultMode };
  bool GetEditorDownloadDefaultMode() { return FEditorDownloadDefaultMode; }
  void SetEditorDownloadDefaultMode(bool value) { FEditorDownloadDefaultMode = value; }
  // __property bool EditorUploadSameOptions = { read = FEditorUploadSameOptions, write = FEditorUploadSameOptions };
  bool GetEditorUploadSameOptions() { return FEditorUploadSameOptions; }
  void SetEditorUploadSameOptions(bool value) { FEditorUploadSameOptions = value; }
  // __property bool EditorUploadOnSave = { read = FEditorUploadOnSave, write = FEditorUploadOnSave };
  bool GetEditorUploadOnSave() { return FEditorUploadOnSave; }
  void SetEditorUploadOnSave(bool value) { FEditorUploadOnSave = value; }
  // __property bool EditorMultiple = { read = FEditorMultiple, write = FEditorMultiple };
  bool GetEditorMultiple() { return FEditorMultiple; }
  void SetEditorMultiple(bool value) { FEditorMultiple = value; }
  // __property bool QueueBeep = { read = FQueueBeep, write = FQueueBeep };
  bool GetQueueBeep() { return FQueueBeep; }
  void SetQueueBeep(bool value) { FQueueBeep = value; }

  // __property std::wstring ApplyCommandCommand = { read = FApplyCommandCommand, write = FApplyCommandCommand };
  std::wstring GetApplyCommandCommand() { return FApplyCommandCommand; }
  void SetApplyCommandCommand(std::wstring value) { FApplyCommandCommand = value; }
  // __property int ApplyCommandParams = { read = FApplyCommandParams, write = FApplyCommandParams };
  int GetApplyCommandParams() { return FApplyCommandParams; }
  void SetApplyCommandParams(int value) { FApplyCommandParams = value; }

  // __property std::wstring PageantPath = { read = FPageantPath, write = FPageantPath };
  std::wstring GetPageantPath() const { return FPageantPath; }
  void SetPageantPath(std::wstring value) { FPageantPath = value; }
  // __property std::wstring PuttygenPath = { read = FPuttygenPath, write = FPuttygenPath };
  std::wstring GetPuttygenPath() const { return FPuttygenPath; }
  void SetPuttygenPath(std::wstring value) { FPuttygenPath = value; }
  // __property TBookmarkList * Bookmarks[wstring Key] = { read = GetBookmarks, write = SetBookmarks };
  TBookmarkList * GetBookmark(std::wstring Key);
  void SetBookmark(std::wstring Key, TBookmarkList * value);

  virtual void Load();
  virtual void Save(bool All, bool Explicit);
  virtual void Default();
  void CacheFarSettings();

protected:
  virtual bool GetConfirmOverwriting();
  virtual void SetConfirmOverwriting(bool value);

  virtual void SaveData(THierarchicalStorage * Storage, bool All);
  virtual void LoadData(THierarchicalStorage * Storage);

  virtual std::wstring ModuleFileName();
  virtual void Saved();

private:
  TCustomFarPlugin * FPlugin;
  int FFarConfirmations;
  bool FConfirmOverwritingOverride;
  bool FConfirmSynchronizedBrowsing;
  bool FForceInheritance;
  bool FDisksMenu;
  int FDisksMenuHotKey;
  bool FPluginsMenu;
  bool FPluginsMenuCommands;
  std::wstring FCommandPrefixes;
  bool FHostNameInTitle;
  bool FEditorDownloadDefaultMode;
  bool FEditorUploadSameOptions;
  bool FEditorUploadOnSave;
  bool FEditorMultiple;
  bool FQueueBeep;
  std::wstring FPageantPath;
  std::wstring FPuttygenPath;
  std::wstring FApplyCommandCommand;
  int FApplyCommandParams;

  bool FCustomPanelModeDetailed;
  bool FFullScreenDetailed;
  std::wstring FColumnTypesDetailed;
  std::wstring FColumnWidthsDetailed;
  std::wstring FStatusColumnTypesDetailed;
  std::wstring FStatusColumnWidthsDetailed;

  TBookmarks * FBookmarks;

  int FarConfirmations();
};
//---------------------------------------------------------------------------
#define FarConfiguration ((TFarConfiguration *) Configuration)
//---------------------------------------------------------------------------
#endif
