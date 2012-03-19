//---------------------------------------------------------------------------
#include "stdafx.h"

#include "boostdefines.hpp"
#include <boost/scope_exit.hpp>

#include "Common.h"
#include "Bookmarks.h"
#include "FarConfiguration.h"
#include "Far3Storage.h"
#include "FarPlugin.h"
//---------------------------------------------------------------------------
enum NetBoxConfirmationsSettings
{
	NBCS_COPYOVERWRITE                  = 0x00000001,
	NBCS_MOVEOVERWRITE                  = 0x00000002,
	NBCS_DRAGANDDROP                    = 0x00000004,
	NBCS_DELETE                         = 0x00000008,
	NBCS_DELETENONEMPTYFOLDERS          = 0x00000010,
	NBCS_INTERRUPTOPERATION             = 0x00000020,
	// NBCS_DISCONNECTNETWORKDRIVE         = 0x00000040,
	NBCS_RELOADEDITEDFILE               = 0x00000080,
	NBCS_CLEARHISTORYLIST               = 0x00000100,
	NBCS_EXIT                           = 0x00000200,
	// NBCS_OVERWRITEDELETEROFILES         = 0x00000400,
};
//---------------------------------------------------------------------------
TFarConfiguration::TFarConfiguration(TCustomFarPlugin *APlugin) :
    TGUIConfiguration()
{
    // DEBUG_PRINTF(L"begin");
    Self = this;
    FFarConfirmations = -1;
    FFarPlugin = APlugin;
    FBookmarks = new TBookmarks();
    Default();
    // DEBUG_PRINTF(L"end");
}
//---------------------------------------------------------------------------
TFarConfiguration::~TFarConfiguration()
{
    delete FBookmarks;
}
//---------------------------------------------------------------------------
void TFarConfiguration::Default()
{
    // DEBUG_PRINTF(L"begin");
    TGUIConfiguration::Default();

    FForceInheritance = false;
    FConfirmOverwritingOverride = false;
    FConfirmSynchronizedBrowsing = true;

    SetDisksMenu(true);
    SetDisksMenuHotKey(0);
    SetPluginsMenu(true);
    SetPluginsMenuCommands(true);
    SetCommandPrefixes(L"netbox,scp,sftp,ftps,http,https");
    SetHostNameInTitle(true);
    SetEditorDownloadDefaultMode(true);
    SetEditorUploadSameOptions(true);
    FEditorUploadOnSave = false;
    FEditorMultiple = false;
    FQueueBeep = true;

    SetCustomPanelModeDetailed(true);
    SetFullScreenDetailed(true);
    SetColumnTypesDetailed(L"N,S,DM,O,G,R");
    SetColumnWidthsDetailed(L"0,8,14,0,0,9");
    SetStatusColumnTypesDetailed(L"NR");
    SetStatusColumnWidthsDetailed(L"0");

    SetApplyCommandCommand(L"");
    SetApplyCommandParams(0);

    SetPuttygenPath(FormatCommand(ExtractFilePath(ModuleFileName()) + L"putty\\puttygen.exe", L""));
    SetPageantPath(FormatCommand(ExtractFilePath(ModuleFileName()) + L"putty\\pageant.exe", L""));

    FBookmarks->Clear();
    // DEBUG_PRINTF(L"end");
}
//---------------------------------------------------------------------------
THierarchicalStorage *TFarConfiguration::CreateStorage()
{
    // DEBUG_PRINTF(L"GetStorage = %d", GetStorage());
  // if (GetStorage() == stFar3Storage)
  {
    assert(FFarPlugin);
    return new TFar3Storage(GetRegistryStorageKey(), MainGuid, FFarPlugin->GetStartupInfo()->SettingsControl);
  }
    return TGUIConfiguration::CreateStorage();
}
//---------------------------------------------------------------------------
void TFarConfiguration::Saved()
{
    TGUIConfiguration::Saved();
    FBookmarks->ModifyAll(false);
}
//---------------------------------------------------------------------------
// duplicated from core\configuration.cpp
#define LASTELEM(ELEM) \
  ELEM.substr(::LastDelimiter(ELEM, L".>") + 1, ELEM.size() - ::LastDelimiter(ELEM, L".>"))
#define BLOCK(KEY, CANCREATE, BLOCK) \
  if (Storage->OpenSubKey(KEY, CANCREATE, true)) \
  { \
      BOOST_SCOPE_EXIT ( (&Storage) ) \
      { \
        Storage->CloseSubKey(); \
      } BOOST_SCOPE_EXIT_END \
      BLOCK \
  }
#define REGCONFIG(CANCREATE) \
  BLOCK(L"Far", CANCREATE, \
    KEY(bool,     DisksMenu); \
    KEY(int,  DisksMenuHotKey); \
    KEY(bool,     PluginsMenu); \
    KEY(bool,     PluginsMenuCommands); \
    KEY(String,   CommandPrefixes); \
    KEY(bool,     CustomPanelModeDetailed); \
    KEY(bool,     FullScreenDetailed); \
    KEY(String,   ColumnTypesDetailed); \
    KEY(String,   ColumnWidthsDetailed); \
    KEY(String,   StatusColumnTypesDetailed); \
    KEY(String,   StatusColumnWidthsDetailed); \
    KEY(bool,     HostNameInTitle); \
    KEY(bool,     ConfirmOverwritingOverride); \
    KEY(bool,     EditorDownloadDefaultMode); \
    KEY(bool,     EditorUploadSameOptions); \
    KEY(bool,     EditorUploadOnSave); \
    KEY(bool,     EditorMultiple); \
    KEY(bool,     QueueBeep); \
    KEY(String,   PuttygenPath); \
    KEY(String,   PageantPath); \
    KEY(String,   ApplyCommandCommand); \
    KEY(int,  ApplyCommandParams); \
    KEY(bool,     ConfirmSynchronizedBrowsing); \
  );
//---------------------------------------------------------------------------
void TFarConfiguration::SaveData(THierarchicalStorage *Storage,
                                 bool All)
{
    // DEBUG_PRINTF(L"begin");
    TGUIConfiguration::SaveData(Storage, All);

    // duplicated from core\configuration.cpp
#define KEY(TYPE, VAR) Storage->Write ## TYPE(LASTELEM(nb::MB2W(#VAR)), Get##VAR())
    REGCONFIG(true);
#undef KEY

    if (Storage->OpenSubKey(L"Bookmarks", true))
    {
        FBookmarks->Save(Storage, All);

        Storage->CloseSubKey();
    }
    // DEBUG_PRINTF(L"end");
}
//---------------------------------------------------------------------------
void TFarConfiguration::LoadData(THierarchicalStorage *Storage)
{
    TGUIConfiguration::LoadData(Storage);

    // duplicated from core\configuration.cpp
#define KEY(TYPE, VAR) Set##VAR(Storage->Read ## TYPE(LASTELEM(nb::MB2W(#VAR)), Get##VAR()))
    REGCONFIG(false);
#undef KEY

    if (Storage->OpenSubKey(L"Bookmarks", false))
    {
        FBookmarks->Load(Storage);
        Storage->CloseSubKey();
    }
}
//---------------------------------------------------------------------------
void TFarConfiguration::Load()
{
    FForceInheritance = true;
    {
        BOOST_SCOPE_EXIT ( (&Self) )
        {
            Self->FForceInheritance = false;
        } BOOST_SCOPE_EXIT_END
        TGUIConfiguration::Load();
    }
}
//---------------------------------------------------------------------------
void TFarConfiguration::Save(bool All, bool Explicit)
{
    FForceInheritance = true;
    {
        BOOST_SCOPE_EXIT ( (&Self) )
        {
            Self->FForceInheritance = false;
        } BOOST_SCOPE_EXIT_END
        TGUIConfiguration::Save(All, Explicit);
    }
}
//---------------------------------------------------------------------------
void TFarConfiguration::SetPlugin(TCustomFarPlugin *value)
{
    if (GetPlugin() != value)
    {
        assert(!GetPlugin() || !value);
        FFarPlugin = value;
    }
}
//---------------------------------------------------------------------------
__int64 TFarConfiguration::GetSetting(FARSETTINGS_SUBFOLDERS Root, const wchar_t *Name)
{
    __int64 result = 0;
    FarSettingsCreate settings = {sizeof(FarSettingsCreate), FarGuid, INVALID_HANDLE_VALUE};
    HANDLE Settings = FFarPlugin->GetStartupInfo()->SettingsControl(INVALID_HANDLE_VALUE, SCTL_CREATE, 0, &settings) ? settings.Handle : 0;
    if (Settings)
    {
        FarSettingsItem item = {Root, Name, FST_UNKNOWN, {0} };
        if (FFarPlugin->GetStartupInfo()->SettingsControl(Settings, SCTL_GET, 0, &item) && FST_QWORD == item.Type)
        {
            result = item.Number;
        }
        FFarPlugin->GetStartupInfo()->SettingsControl(Settings, SCTL_FREE, 0, 0);
    }
    return result;
}
//---------------------------------------------------------------------------
__int64 TFarConfiguration::GetConfirmationsSetting(HANDLE &Settings, const wchar_t *Name)
{
    FarSettingsItem item = {FSSF_CONFIRMATIONS, Name, FST_UNKNOWN, {0} };
    if (FFarPlugin->GetStartupInfo()->SettingsControl(Settings, SCTL_GET, 0, &item) && FST_QWORD == item.Type)
    {
        return item.Number;
    }
    return 0;
}

//---------------------------------------------------------------------------
__int64 TFarConfiguration::GetConfirmationsSettings()
{
    __int64 result = 0;
    FarSettingsCreate settings = {sizeof(FarSettingsCreate), FarGuid, INVALID_HANDLE_VALUE};
    HANDLE Settings = FFarPlugin->GetStartupInfo()->SettingsControl(INVALID_HANDLE_VALUE, SCTL_CREATE, 0, &settings) ? settings.Handle : 0;
    if (Settings)
    {
        if (GetConfirmationsSetting(Settings, L"Copy"))
            result |= NBCS_COPYOVERWRITE;
        if (GetConfirmationsSetting(Settings, L"Move"))
            result |= NBCS_MOVEOVERWRITE;
        // if (GetConfirmationsSetting(Settings, L"RO"))
            // result |= NBCS_MOVEOVERWRITE;
        if (GetConfirmationsSetting(Settings, L"Drag"))
            result |= NBCS_DRAGANDDROP;
        if (GetConfirmationsSetting(Settings, L"Delete"))
            result |= NBCS_DELETE;
        if (GetConfirmationsSetting(Settings, L"DeleteFolder"))
            result |= NBCS_DELETENONEMPTYFOLDERS;
        if (GetConfirmationsSetting(Settings, L"Esc"))
            result |= NBCS_INTERRUPTOPERATION;
        if (GetConfirmationsSetting(Settings, L"AllowReedit"))
            result |= NBCS_RELOADEDITEDFILE;
        if (GetConfirmationsSetting(Settings, L"HistoryClear"))
            result |= NBCS_CLEARHISTORYLIST;
        if (GetConfirmationsSetting(Settings, L"Exit"))
            result |= NBCS_EXIT;
        FFarPlugin->GetStartupInfo()->SettingsControl(Settings, SCTL_FREE, 0, 0);
    }
    return result;
}
//---------------------------------------------------------------------------
void TFarConfiguration::CacheFarSettings()
{
    FFarConfirmations = GetConfirmationsSettings();
}
//---------------------------------------------------------------------------
__int64 TFarConfiguration::FarConfirmations()
{
    if (GetCurrentThreadId() == GetPlugin()->GetFarThread())
    {
        return GetConfirmationsSettings();
    }
    else
    {
        assert(FFarConfirmations >= 0);
        return FFarConfirmations;
    }
}
//---------------------------------------------------------------------------
bool TFarConfiguration::GetConfirmOverwriting()
{
    if (FForceInheritance || FConfirmOverwritingOverride)
    {
        return TGUIConfiguration::GetConfirmOverwriting();
    }
    else
    {
        assert(GetPlugin());
        return (FarConfirmations() & NBCS_COPYOVERWRITE) != 0;
    }
}
//---------------------------------------------------------------------------
void TFarConfiguration::SetConfirmOverwriting(bool value)
{
    if (FForceInheritance)
    {
        TGUIConfiguration::SetConfirmOverwriting(value);
    }
    else
    {
        if (GetConfirmOverwriting() != value)
        {
            FConfirmOverwritingOverride = true;
            TGUIConfiguration::SetConfirmOverwriting(value);
        }
    }
}
//---------------------------------------------------------------------------
bool TFarConfiguration::GetConfirmDeleting()
{
    assert(GetPlugin());
    return (FarConfirmations() & NBCS_DELETE) != 0;
}
//---------------------------------------------------------------------------
std::wstring TFarConfiguration::ModuleFileName()
{
    assert(GetPlugin());
    return GetPlugin()->GetModuleName();
}
//---------------------------------------------------------------------------
void TFarConfiguration::SetBookmark(const std::wstring Key,
                                    TBookmarkList *value)
{
    FBookmarks->SetBookmark(Key, value);
    Changed();
}
//---------------------------------------------------------------------------
TBookmarkList *TFarConfiguration::GetBookmark(const std::wstring Key)
{
    return FBookmarks->GetBookmark(Key);
}
