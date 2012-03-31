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
    explicit TFarConfiguration(TCustomFarPlugin *APlugin);
    virtual ~TFarConfiguration();

    TCustomFarPlugin *GetPlugin() { return FFarPlugin; }
    void SetPlugin(TCustomFarPlugin *value);
    bool GetConfirmOverwritingOverride() { return FConfirmOverwritingOverride; }
    void SetConfirmOverwritingOverride(bool value) { FConfirmOverwritingOverride = value; }
    bool GetConfirmDeleting();
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
    std::wstring GetCommandPrefixes() { return FCommandPrefixes; }
    void SetCommandPrefixes(const std::wstring value) { FCommandPrefixes = value; }
    bool GetHostNameInTitle() { return FHostNameInTitle; }
    void SetHostNameInTitle(bool value) { FHostNameInTitle = value; }

    bool GetCustomPanelModeDetailed() { return FCustomPanelModeDetailed; }
    void SetCustomPanelModeDetailed(bool value) { FCustomPanelModeDetailed = value; }
    bool GetFullScreenDetailed() { return FFullScreenDetailed; }
    void SetFullScreenDetailed(bool value) { FFullScreenDetailed = value; }
    std::wstring GetColumnTypesDetailed() { return FColumnTypesDetailed; }
    void SetColumnTypesDetailed(const std::wstring value) { FColumnTypesDetailed = value; }
    std::wstring GetColumnWidthsDetailed() { return FColumnWidthsDetailed; }
    void SetColumnWidthsDetailed(const std::wstring value) { FColumnWidthsDetailed = value; }
    std::wstring GetStatusColumnTypesDetailed() { return FStatusColumnTypesDetailed; }
    void SetStatusColumnTypesDetailed(const std::wstring value) { FStatusColumnTypesDetailed = value; }
    std::wstring GetStatusColumnWidthsDetailed() { return FStatusColumnWidthsDetailed; }
    void SetStatusColumnWidthsDetailed(const std::wstring value) { FStatusColumnWidthsDetailed = value; }
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

    std::wstring GetApplyCommandCommand() { return FApplyCommandCommand; }
    void SetApplyCommandCommand(const std::wstring value) { FApplyCommandCommand = value; }
    int GetApplyCommandParams() { return FApplyCommandParams; }
    void SetApplyCommandParams(int value) { FApplyCommandParams = value; }

    std::wstring GetPageantPath() const { return FPageantPath; }
    void SetPageantPath(const std::wstring value) { FPageantPath = value; }
    std::wstring GetPuttygenPath() const { return FPuttygenPath; }
    void SetPuttygenPath(const std::wstring value) { FPuttygenPath = value; }
    TBookmarkList *GetBookmark(const std::wstring Key);
    void SetBookmark(const std::wstring Key, TBookmarkList *value);

    virtual void __fastcall Load();
    virtual void __fastcall Save(bool All, bool Explicit);
    virtual void __fastcall Default();
    virtual THierarchicalStorage * __fastcall CreateStorage();
    void CacheFarSettings();

protected:
    virtual bool __fastcall GetConfirmOverwriting();
    virtual void __fastcall SetConfirmOverwriting(bool value);

    virtual void __fastcall SaveData(THierarchicalStorage *Storage, bool All);
    virtual void __fastcall LoadData(THierarchicalStorage *Storage);

    virtual std::wstring __fastcall ModuleFileName();
    virtual void __fastcall Saved();

private:
    TCustomFarPlugin *FFarPlugin;
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

    TBookmarks *FBookmarks;
    TFarConfiguration *Self;

    int FarConfirmations();
};
//---------------------------------------------------------------------------
#define FarConfiguration ((TFarConfiguration *) Configuration)
//---------------------------------------------------------------------------
#endif
