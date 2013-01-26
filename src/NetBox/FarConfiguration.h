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

  TCustomFarPlugin * GetPlugin() { return FFarPlugin; }
  void SetPlugin(TCustomFarPlugin * Value);
  bool GetConfirmOverwritingOverride() { return FConfirmOverwritingOverride; }
  void SetConfirmOverwritingOverride(bool Value) { FConfirmOverwritingOverride = Value; }
  bool GetConfirmDeleting();
  bool GetConfirmSynchronizedBrowsing() { return FConfirmSynchronizedBrowsing; }
  void SetConfirmSynchronizedBrowsing(bool Value) { FConfirmSynchronizedBrowsing = Value; }
  bool GetDisksMenu() { return FDisksMenu; }
  void SetDisksMenu(bool Value) { FDisksMenu = Value; }
  int GetDisksMenuHotKey() { return FDisksMenuHotKey; }
  void SetDisksMenuHotKey(int Value) { FDisksMenuHotKey = Value; }
  bool GetPluginsMenu() { return FPluginsMenu; }
  void SetPluginsMenu(bool Value) { FPluginsMenu = Value; }
  bool GetPluginsMenuCommands() { return FPluginsMenuCommands; }
  void SetPluginsMenuCommands(bool Value) { FPluginsMenuCommands = Value; }
  UnicodeString GetCommandPrefixes() { return FCommandPrefixes; }
  void SetCommandPrefixes(const UnicodeString & Value) { FCommandPrefixes = Value; }
  bool GetHostNameInTitle() { return FHostNameInTitle; }
  void SetHostNameInTitle(bool Value) { FHostNameInTitle = Value; }

  bool GetCustomPanelModeDetailed() { return FCustomPanelModeDetailed; }
  void SetCustomPanelModeDetailed(bool Value) { FCustomPanelModeDetailed = Value; }
  bool GetFullScreenDetailed() { return FFullScreenDetailed; }
  void SetFullScreenDetailed(bool Value) { FFullScreenDetailed = Value; }
  UnicodeString GetColumnTypesDetailed() { return FColumnTypesDetailed; }
  void SetColumnTypesDetailed(const UnicodeString & Value) { FColumnTypesDetailed = Value; }
  UnicodeString GetColumnWidthsDetailed() { return FColumnWidthsDetailed; }
  void SetColumnWidthsDetailed(const UnicodeString & Value) { FColumnWidthsDetailed = Value; }
  UnicodeString GetStatusColumnTypesDetailed() { return FStatusColumnTypesDetailed; }
  void SetStatusColumnTypesDetailed(const UnicodeString & Value) { FStatusColumnTypesDetailed = Value; }
  UnicodeString GetStatusColumnWidthsDetailed() { return FStatusColumnWidthsDetailed; }
  void SetStatusColumnWidthsDetailed(const UnicodeString & Value) { FStatusColumnWidthsDetailed = Value; }
  bool GetEditorDownloadDefaultMode() { return FEditorDownloadDefaultMode; }
  void SetEditorDownloadDefaultMode(bool Value) { FEditorDownloadDefaultMode = Value; }
  bool GetEditorUploadSameOptions() { return FEditorUploadSameOptions; }
  void SetEditorUploadSameOptions(bool Value) { FEditorUploadSameOptions = Value; }
  bool GetEditorUploadOnSave() { return FEditorUploadOnSave; }
  void SetEditorUploadOnSave(bool Value) { FEditorUploadOnSave = Value; }
  bool GetEditorMultiple() { return FEditorMultiple; }
  void SetEditorMultiple(bool Value) { FEditorMultiple = Value; }
  bool GetQueueBeep() { return FQueueBeep; }
  void SetQueueBeep(bool Value) { FQueueBeep = Value; }

  UnicodeString GetApplyCommandCommand() { return FApplyCommandCommand; }
  void SetApplyCommandCommand(const UnicodeString & Value) { FApplyCommandCommand = Value; }
  int GetApplyCommandParams() { return FApplyCommandParams; }
  void SetApplyCommandParams(int Value) { FApplyCommandParams = Value; }

  UnicodeString GetPageantPath() const { return FPageantPath; }
  void SetPageantPath(const UnicodeString & Value) { FPageantPath = Value; }
  UnicodeString GetPuttygenPath() const { return FPuttygenPath; }
  void SetPuttygenPath(const UnicodeString & Value) { FPuttygenPath = Value; }
  TBookmarkList * GetBookmarks(const UnicodeString & Key);
  void SetBookmarks(const UnicodeString & Key, TBookmarkList * Value);

  virtual void Load();
  virtual void Save(bool All, bool Explicit);
  virtual void Default();
  virtual THierarchicalStorage * CreateScpStorage(bool SessionList);
  void CacheFarSettings();

protected:
  virtual bool GetConfirmOverwriting();
  virtual void SetConfirmOverwriting(bool Value);

  virtual void SaveData(THierarchicalStorage * Storage, bool All);
  virtual void LoadData(THierarchicalStorage * Storage);

  virtual UnicodeString ModuleFileName();
  virtual void Saved();

private:
  TCustomFarPlugin * FFarPlugin;
  intptr_t FFarConfirmations;
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

  intptr_t FarConfirmations();
};
//---------------------------------------------------------------------------
#define FarConfiguration (static_cast<TFarConfiguration *>(Configuration))
//---------------------------------------------------------------------------
#endif
