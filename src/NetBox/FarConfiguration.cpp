//---------------------------------------------------------------------------
#ifndef _MSC_VER
#include <vcl.h>
#pragma hdrstop
#else
#include "nbafx.h"

#include "boostdefines.hpp"
#include <boost/scope_exit.hpp>
#endif

#include "Common.h"
#include "Bookmarks.h"
#include "FarConfiguration.h"
#include "Far3Storage.h"
#include "FarPlugin.h"
//---------------------------------------------------------------------------
#ifndef _MSC_VER
#pragma package(smart_init)
#endif
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
/* __fastcall */ TFarConfiguration::TFarConfiguration(TCustomFarPlugin * APlugin) :
  TGUIConfiguration()
{
  Self = this;
  FFarConfirmations = -1;
  FFarPlugin = APlugin;
  FBookmarks = new TBookmarks();
  Default();
}
//---------------------------------------------------------------------------
/* __fastcall */ TFarConfiguration::~TFarConfiguration()
{
  delete FBookmarks;
}
//---------------------------------------------------------------------------
void __fastcall TFarConfiguration::Default()
{
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
}
//---------------------------------------------------------------------------
THierarchicalStorage * TFarConfiguration::CreateScpStorage(bool SessionList)
{
  // if (GetStorage() == stFar3Storage)
  {
    assert(FFarPlugin);
    return FFarPlugin ? new TFar3Storage(GetRegistryStorageKey(), MainGuid, FFarPlugin->GetStartupInfo()->SettingsControl) : NULL;
  }
  return TGUIConfiguration::CreateScpStorage(SessionList);
}
//---------------------------------------------------------------------------
void __fastcall TFarConfiguration::Saved()
{
  TGUIConfiguration::Saved();
  FBookmarks->ModifyAll(false);
}
//---------------------------------------------------------------------------
// duplicated from core\configuration.cpp
#define LASTELEM(ELEM) \
  ELEM.SubString(ELEM.LastDelimiter(L".>")+1, ELEM.Length() - ELEM.LastDelimiter(L".>"))
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
    KEY(Bool,     DisksMenu); \
    KEY(Integer,  DisksMenuHotKey); \
    KEY(Bool,     PluginsMenu); \
    KEY(Bool,     PluginsMenuCommands); \
    KEY(String,   CommandPrefixes); \
    KEY(Bool,     CustomPanelModeDetailed); \
    KEY(Bool,     FullScreenDetailed); \
    KEY(String,   ColumnTypesDetailed); \
    KEY(String,   ColumnWidthsDetailed); \
    KEY(String,   StatusColumnTypesDetailed); \
    KEY(String,   StatusColumnWidthsDetailed); \
    KEY(Bool,     HostNameInTitle); \
    KEY(Bool,     ConfirmOverwritingOverride); \
    KEY(Bool,     EditorDownloadDefaultMode); \
    KEY(Bool,     EditorUploadSameOptions); \
    KEY(Bool,     EditorUploadOnSave); \
    KEY(Bool,     EditorMultiple); \
    KEY(Bool,     QueueBeep); \
    KEY(String,   PuttygenPath); \
    KEY(String,   PageantPath); \
    KEY(String,   ApplyCommandCommand); \
    KEY(Integer,  ApplyCommandParams); \
    KEY(Bool,     ConfirmSynchronizedBrowsing); \
  );
//---------------------------------------------------------------------------
void __fastcall TFarConfiguration::SaveData(THierarchicalStorage * Storage,
  bool All)
{
  TGUIConfiguration::SaveData(Storage, All);

  // duplicated from core\configuration.cpp
  #define KEY(TYPE, VAR) Storage->Write ## TYPE(LASTELEM(MB2W(#VAR)), Get##VAR())
  REGCONFIG(true);
  #undef KEY

  if (Storage->OpenSubKey(L"Bookmarks", true))
  {
    FBookmarks->Save(Storage, All);

    Storage->CloseSubKey();
  }
}
//---------------------------------------------------------------------------
void __fastcall TFarConfiguration::LoadData(THierarchicalStorage * Storage)
{
  TGUIConfiguration::LoadData(Storage);

  // duplicated from core\configuration.cpp
  #define KEY(TYPE, VAR) Set##VAR(Storage->Read ## TYPE(LASTELEM(MB2W(#VAR)), Get##VAR()))
  // #pragma warn -eas
  REGCONFIG(false);
  // #pragma warn +eas
  #undef KEY

  if (Storage->OpenSubKey(L"Bookmarks", false))
  {
    FBookmarks->Load(Storage);
    Storage->CloseSubKey();
  }
}
//---------------------------------------------------------------------------
void __fastcall TFarConfiguration::Load()
{
  FForceInheritance = true;
  // try
  {
    BOOST_SCOPE_EXIT ( (&Self) )
    {
      Self->FForceInheritance = false;
    } BOOST_SCOPE_EXIT_END
    TGUIConfiguration::Load();
  }
#ifndef _MSC_VER
  __finally
  {
    FForceInheritance = false;
  }
#endif
}
//---------------------------------------------------------------------------
void __fastcall TFarConfiguration::Save(bool All, bool Explicit)
{
  FForceInheritance = true;
  // try
  {
    BOOST_SCOPE_EXIT ( (&Self) )
    {
      Self->FForceInheritance = false;
    } BOOST_SCOPE_EXIT_END
    TGUIConfiguration::Save(All, Explicit);
  }
#ifndef _MSC_VER
  __finally
  {
    FForceInheritance = false;
  }
#endif
}
//---------------------------------------------------------------------------
void __fastcall TFarConfiguration::SetPlugin(TCustomFarPlugin * value)
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
void __fastcall TFarConfiguration::CacheFarSettings()
{
  FFarConfirmations = GetConfirmationsSettings();
}
//---------------------------------------------------------------------------
__int64 __fastcall TFarConfiguration::FarConfirmations()
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
bool __fastcall TFarConfiguration::GetConfirmOverwriting()
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
void __fastcall TFarConfiguration::SetConfirmOverwriting(bool value)
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
bool __fastcall TFarConfiguration::GetConfirmDeleting()
{
  assert(GetPlugin());
  return (FarConfirmations() & NBCS_DELETE) != 0;
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TFarConfiguration::ModuleFileName()
{
  assert(GetPlugin());
  return GetPlugin()->GetModuleName();
}
//---------------------------------------------------------------------------
void __fastcall TFarConfiguration::SetBookmarks(UnicodeString Key,
  TBookmarkList * value)
{
  FBookmarks->SetBookmarks(Key, value);
  Changed();
}
//---------------------------------------------------------------------------
TBookmarkList * __fastcall TFarConfiguration::GetBookmarks(UnicodeString Key)
{
  return FBookmarks->GetBookmarks(Key);
}
