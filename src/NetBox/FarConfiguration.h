#pragma once

#include <plugin.hpp>
#include "GUIConfiguration.h"

class TCustomFarPlugin;
class TBookmarks;
class TBookmarkList;

// DefaultCommandPrefixes("netbox,ftp,scp,sftp,ftps,http,https,webdav,s3");
constexpr const wchar_t * DefaultCommandPrefixes = L"netbox,ftp,scp,sftp,ftps,webdav,s3";

class TFarConfiguration final : public TGUIConfiguration
{
  NB_DISABLE_COPY(TFarConfiguration)
public:
  static bool classof(const TObject * Obj) { return Obj->is(OBJECT_CLASS_TFarConfiguration); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TFarConfiguration) || TGUIConfiguration::is(Kind); }
public:
  TFarConfiguration() = delete;
  explicit TFarConfiguration(gsl::not_null<TCustomFarPlugin *> APlugin) noexcept;
  virtual ~TFarConfiguration() noexcept override;

  void LoadFarConfiguration();
  void SaveFarConfiguration(bool All, bool Explicit);
  virtual void Default() override;
  virtual THierarchicalStorage * CreateScpStorage(bool & SessionList) override;
  void CacheFarSettings();

  const TCustomFarPlugin * GetPlugin() const { return FFarPlugin; }
  TCustomFarPlugin * GetPlugin() { return FFarPlugin; }
  void SetPlugin(TCustomFarPlugin * Value);
  bool GetConfirmOverwritingOverride() const { return FConfirmOverwritingOverride; }
  void SetConfirmOverwritingOverride(bool Value) { FConfirmOverwritingOverride = Value; }
  bool GetConfirmDeleting() const;
  bool GetConfirmSynchronizedBrowsing() const { return FConfirmSynchronizedBrowsing; }
  void SetConfirmSynchronizedBrowsing(bool Value) { FConfirmSynchronizedBrowsing = Value; }
  bool GetDisksMenu() const { return FDisksMenu; }
  void SetDisksMenu(bool Value) { FDisksMenu = Value; }
  int32_t GetDisksMenuHotKey() const { return FDisksMenuHotKey; }
  void SetDisksMenuHotKey(int32_t Value) { FDisksMenuHotKey = Value; }
  bool GetPluginsMenu() const { return FPluginsMenu; }
  void SetPluginsMenu(bool Value) { FPluginsMenu = Value; }
  bool GetPluginsMenuCommands() const { return FPluginsMenuCommands; }
  void SetPluginsMenuCommands(bool Value) { FPluginsMenuCommands = Value; }
  UnicodeString GetCommandPrefixes() const { return FCommandPrefixes; }
  void SetCommandPrefixes(const UnicodeString & Value) { FCommandPrefixes = Value; }
  bool GetSessionNameInTitle() const { return FSessionNameInTitle; }
  void SetSessionNameInTitle(bool Value) { FSessionNameInTitle = Value; }

  bool GetCustomPanelModeDetailed() const { return FCustomPanelModeDetailed; }
  void SetCustomPanelModeDetailed(bool Value) { FCustomPanelModeDetailed = Value; }
  bool GetFullScreenDetailed() const { return FFullScreenDetailed; }
  void SetFullScreenDetailed(bool Value) { FFullScreenDetailed = Value; }
  UnicodeString GetColumnTypesDetailed() const { return FColumnTypesDetailed; }
  void SetColumnTypesDetailed(const UnicodeString & Value) { FColumnTypesDetailed = Value; }
  UnicodeString GetColumnWidthsDetailed() const { return FColumnWidthsDetailed; }
  void SetColumnWidthsDetailed(const UnicodeString & Value) { FColumnWidthsDetailed = Value; }
  UnicodeString GetStatusColumnTypesDetailed() const { return FStatusColumnTypesDetailed; }
  void SetStatusColumnTypesDetailed(const UnicodeString & Value) { FStatusColumnTypesDetailed = Value; }
  UnicodeString GetStatusColumnWidthsDetailed() const { return FStatusColumnWidthsDetailed; }
  void SetStatusColumnWidthsDetailed(const UnicodeString & Value) { FStatusColumnWidthsDetailed = Value; }
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
  void SetApplyCommandCommand(const UnicodeString & Value) { FApplyCommandCommand = Value; }
  int32_t GetApplyCommandParams() const { return FApplyCommandParams; }
  void SetApplyCommandParams(int32_t Value) { FApplyCommandParams = Value; }

  UnicodeString GetPageantPath() const { return FPageantPath; }
  void SetPageantPath(const UnicodeString & Value) { FPageantPath = Value; }
  UnicodeString GetPuttygenPath() const { return FPuttygenPath; }
  void SetPuttygenPath(const UnicodeString & Value) { FPuttygenPath = Value; }
  TBookmarkList * GetBookmarks(const UnicodeString & Key);
  void SetBookmarks(const UnicodeString & Key, const TBookmarkList * Value);

public:
  virtual UnicodeString TemporaryDir(bool Mask = false) const override { return ""; }

protected:
  virtual bool GetConfirmOverwriting() const override;
  virtual void SetConfirmOverwriting(bool Value) override;

  virtual void SaveData(THierarchicalStorage * Storage, bool All) override;
  virtual void LoadData(THierarchicalStorage * Storage) override;

  virtual UnicodeString ModuleFileName() const override;
  virtual void Saved() override;
  void ConfigurationInit() override;

private:
  intptr_t GetSetting(FARSETTINGS_SUBFOLDERS Root, const wchar_t * Name) const;
  intptr_t GetConfirmationsSetting(HANDLE & Settings, const wchar_t * Name) const;
  int32_t GetConfirmationsSettings() const;

private:
  gsl::not_null<TCustomFarPlugin *> FFarPlugin;
  std::unique_ptr<TBookmarks> FBookmarks{std::make_unique<TBookmarks>()};
  int32_t FFarConfirmations{-1};
  bool FConfirmOverwritingOverride{false};
  bool FConfirmSynchronizedBrowsing{false};
  bool FForceInheritance{false};
  bool FDisksMenu{false};
  int32_t FDisksMenuHotKey{0};
  bool FPluginsMenu{false};
  bool FPluginsMenuCommands{false};
  UnicodeString FCommandPrefixes;
  bool FSessionNameInTitle{false};
  bool FEditorDownloadDefaultMode{false};
  bool FEditorUploadSameOptions{false};
  bool FEditorUploadOnSave{false};
  bool FEditorMultiple{false};
  bool FQueueBeep{true};
  UnicodeString FPageantPath;
  UnicodeString FPuttygenPath;
  UnicodeString FApplyCommandCommand;
  int32_t FApplyCommandParams{0};

  bool FCustomPanelModeDetailed{false};
  bool FFullScreenDetailed{false};
  UnicodeString FColumnTypesDetailed;
  UnicodeString FColumnWidthsDetailed;
  UnicodeString FStatusColumnTypesDetailed;
  UnicodeString FStatusColumnWidthsDetailed;

private:
  int32_t FarConfirmations() const;
};

NB_CORE_EXPORT TFarConfiguration * GetFarConfiguration();
