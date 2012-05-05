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

    UnicodeString GetPageantPath() const { return FPageantPath; }
    void SetPageantPath(const UnicodeString value) { FPageantPath = value; }
    UnicodeString GetPuttygenPath() const { return FPuttygenPath; }
    void SetPuttygenPath(const UnicodeString value) { FPuttygenPath = value; }
    TBookmarkList *GetBookmark(const UnicodeString Key);
    void SetBookmark(const UnicodeString Key, TBookmarkList *value);

    virtual void __fastcall Load();
    virtual void __fastcall Save(bool All, bool Explicit);
    virtual void __fastcall Default();
    virtual THierarchicalStorage * __fastcall CreateScpStorage(bool SessionList);
    void CacheFarSettings();

protected:
    virtual bool __fastcall GetConfirmOverwriting();
    virtual void __fastcall SetConfirmOverwriting(bool value);

    virtual void __fastcall SaveData(THierarchicalStorage *Storage, bool All);
    virtual void __fastcall LoadData(THierarchicalStorage *Storage);

    virtual UnicodeString __fastcall ModuleFileName();
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

    TBookmarks *FBookmarks;
    TFarConfiguration *Self;

    int FarConfirmations();
};
//---------------------------------------------------------------------------
#define FarConfiguration ((TFarConfiguration *) Configuration)
//---------------------------------------------------------------------------
#endif
