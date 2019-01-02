#pragma once

#include "GUIConfiguration.h"

class TCustomFarPlugin;
class TBookmarks;
class TBookmarkList;

NB_DEFINE_CLASS_ID(TFarConfiguration);
class TFarConfiguration : public TGUIConfiguration
{
  NB_DISABLE_COPY(TFarConfiguration)
public:
  static inline bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TFarConfiguration); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TFarConfiguration) || TGUIConfiguration::is(Kind); }
public:
  explicit TFarConfiguration(TCustomFarPlugin *APlugin);
  virtual ~TFarConfiguration();

  virtual void Load();
  virtual void Save(bool All, bool Explicit);
  virtual void Default() override;
  virtual THierarchicalStorage *CreateStorage(bool &SessionList) override;
  void CacheFarSettings();

  const TCustomFarPlugin *GetPlugin() const { return FFarPlugin; }
  TCustomFarPlugin *GetPlugin() { return FFarPlugin; }
  void SetPlugin(TCustomFarPlugin *Value);
  bool GetConfirmOverwritingOverride() const { return FConfirmOverwritingOverride; }
  void SetConfirmOverwritingOverride(bool Value) { FConfirmOverwritingOverride = Value; }
  bool GetConfirmDeleting() const;
  bool GetConfirmSynchronizedBrowsing() const { return FConfirmSynchronizedBrowsing; }
  void SetConfirmSynchronizedBrowsing(bool Value) { FConfirmSynchronizedBrowsing = Value; }
  bool GetDisksMenu() const { return FDisksMenu; }
  void SetDisksMenu(bool Value) { FDisksMenu = Value; }
  intptr_t GetDisksMenuHotKey() const { return FDisksMenuHotKey; }
  void SetDisksMenuHotKey(intptr_t Value) { FDisksMenuHotKey = Value; }
  bool GetPluginsMenu() const { return FPluginsMenu; }
  void SetPluginsMenu(bool Value) { FPluginsMenu = Value; }
  bool GetPluginsMenuCommands() const { return FPluginsMenuCommands; }
  void SetPluginsMenuCommands(bool Value) { FPluginsMenuCommands = Value; }
  UnicodeString GetCommandPrefixes() const { return FCommandPrefixes; }
  void SetCommandPrefixes(UnicodeString Value) { FCommandPrefixes = Value; }
  bool GetSessionNameInTitle() const { return FSessionNameInTitle; }
  void SetSessionNameInTitle(bool Value) { FSessionNameInTitle = Value; }

  bool GetCustomPanelModeDetailed() const { return FCustomPanelModeDetailed; }
  void SetCustomPanelModeDetailed(bool Value) { FCustomPanelModeDetailed = Value; }
  bool GetFullScreenDetailed() const { return FFullScreenDetailed; }
  void SetFullScreenDetailed(bool Value) { FFullScreenDetailed = Value; }
  UnicodeString GetColumnTypesDetailed() const { return FColumnTypesDetailed; }
  void SetColumnTypesDetailed(UnicodeString Value) { FColumnTypesDetailed = Value; }
  UnicodeString GetColumnWidthsDetailed() const { return FColumnWidthsDetailed; }
  void SetColumnWidthsDetailed(UnicodeString Value) { FColumnWidthsDetailed = Value; }
  UnicodeString GetStatusColumnTypesDetailed() const { return FStatusColumnTypesDetailed; }
  void SetStatusColumnTypesDetailed(UnicodeString Value) { FStatusColumnTypesDetailed = Value; }
  UnicodeString GetStatusColumnWidthsDetailed() const { return FStatusColumnWidthsDetailed; }
  void SetStatusColumnWidthsDetailed(UnicodeString Value) { FStatusColumnWidthsDetailed = Value; }
  bool GetEditorDownloadDefaultMode() const { return FEditorDownloadDefaultMode; }
  void SetEditorDownloadDefaultMode(bool Value) { FEditorDownloadDefaultMode = Value; }
  bool GetEditorUploadSameOptions() const { return FEditorUploadSameOptions; }
  void SetEditorUploadSameOptions(bool Value) { FEditorUploadSameOptions = Value; }
  bool GetEditorUploadOnSave() const { return FEditorUploadOnSave; }
  void SetEditorUploadOnSave(bool Value) { FEditorUploadOnSave = Value; }
  bool GetEditorMultiple() const { return FEditorMultiple; }
  void SetEditorMultiple(bool Value) { FEditorMultiple = Value; }
  bool GetQueueBeep() const { return FQueueBeep; }
  void SetQueueBeep(bool Value) { FQueueBeep = Value; }

  UnicodeString GetApplyCommandCommand() const { return FApplyCommandCommand; }
  void SetApplyCommandCommand(UnicodeString Value) { FApplyCommandCommand = Value; }
  intptr_t GetApplyCommandParams() const { return FApplyCommandParams; }
  void SetApplyCommandParams(intptr_t Value) { FApplyCommandParams = Value; }

  UnicodeString GetPageantPath() const { return FPageantPath; }
  void SetPageantPath(UnicodeString Value) { FPageantPath = Value; }
  UnicodeString GetPuttygenPath() const { return FPuttygenPath; }
  void SetPuttygenPath(UnicodeString Value) { FPuttygenPath = Value; }
  TBookmarkList *GetBookmarks(const UnicodeString Key);
  void SetBookmarks(const UnicodeString Key, TBookmarkList *Value);

protected:
  bool GetConfirmOverwriting() const override;
  void SetConfirmOverwriting(bool Value) override;

  void SaveData(THierarchicalStorage *Storage, bool All) override;
  void LoadData(THierarchicalStorage *Storage) override;

  UnicodeString ModuleFileName() const override;
  void Saved() override;

private:
  TCustomFarPlugin *FFarPlugin{nullptr};
  std::unique_ptr<TBookmarks> FBookmarks;
  intptr_t FFarConfirmations{0};
  bool FConfirmOverwritingOverride{false};
  bool FConfirmSynchronizedBrowsing{false};
  bool FForceInheritance{false};
  bool FDisksMenu{false};
  intptr_t FDisksMenuHotKey{0};
  bool FPluginsMenu{false};
  bool FPluginsMenuCommands{false};
  UnicodeString FCommandPrefixes;
  bool FSessionNameInTitle{false};
  bool FEditorDownloadDefaultMode{false};
  bool FEditorUploadSameOptions{false};
  bool FEditorUploadOnSave{false};
  bool FEditorMultiple{false};
  bool FQueueBeep{false};
  UnicodeString FPageantPath;
  UnicodeString FPuttygenPath;
  UnicodeString FApplyCommandCommand;
  intptr_t FApplyCommandParams{0};

  bool FCustomPanelModeDetailed{false};
  bool FFullScreenDetailed{false};
  UnicodeString FColumnTypesDetailed;
  UnicodeString FColumnWidthsDetailed;
  UnicodeString FStatusColumnTypesDetailed;
  UnicodeString FStatusColumnWidthsDetailed;

private:
  intptr_t FarConfirmations() const;
};

TFarConfiguration *GetFarConfiguration();

