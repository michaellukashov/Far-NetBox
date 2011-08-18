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
  __fastcall TFarConfiguration(TCustomFarPlugin * APlugin);
  virtual __fastcall ~TFarConfiguration();

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

  virtual void __fastcall Load();
  virtual void __fastcall Save(bool All, bool Explicit);
  virtual void __fastcall Default();
  void __fastcall CacheFarSettings();

protected:
  virtual bool __fastcall GetConfirmOverwriting();
  virtual void __fastcall SetConfirmOverwriting(bool value);

  virtual void __fastcall SaveData(THierarchicalStorage * Storage, bool All);
  virtual void __fastcall LoadData(THierarchicalStorage * Storage);

  virtual AnsiString __fastcall ModuleFileName();
  virtual void __fastcall Saved();

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
  AnsiString FCommandPrefixes;
  bool FHostNameInTitle;
  bool FEditorDownloadDefaultMode;
  bool FEditorUploadSameOptions;
  bool FEditorUploadOnSave;
  bool FEditorMultiple;
  bool FQueueBeep;
  AnsiString FPageantPath;
  AnsiString FPuttygenPath;
  AnsiString FApplyCommandCommand;
  int FApplyCommandParams;

  bool FCustomPanelModeDetailed;
  bool FFullScreenDetailed;
  AnsiString FColumnTypesDetailed;
  AnsiString FColumnWidthsDetailed;
  AnsiString FStatusColumnTypesDetailed;
  AnsiString FStatusColumnWidthsDetailed;

  TBookmarks * FBookmarks;

  void __fastcall SetPlugin(TCustomFarPlugin * value);
  bool __fastcall GetConfirmDeleting();
  void __fastcall SetBookmarks(AnsiString Key, TBookmarkList * value);
  TBookmarkList * __fastcall GetBookmarks(AnsiString Key);
  int __fastcall FarConfirmations();

  AnsiString __fastcall GetPageantPath();
  AnsiString __fastcall GetPuttygenPath();
};
//---------------------------------------------------------------------------
#define FarConfiguration ((TFarConfiguration *) Configuration)
//---------------------------------------------------------------------------
#endif
