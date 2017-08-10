#include <vcl.h>
#pragma hdrstop

#include <Common.h>

#include "Bookmarks.h"
#include "FarConfiguration.h"
#include "Far3Storage.h"
#include "FarPlugin.h"
#include "CoreMain.h"

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

TFarConfiguration::TFarConfiguration(TCustomFarPlugin *APlugin) :
  TGUIConfiguration(OBJECT_CLASS_TFarConfiguration),
  FFarPlugin(APlugin),
  FBookmarks(new TBookmarks()),
  FFarConfirmations(-1)
{
  TFarConfiguration::Default();
  CacheFarSettings();
}

TFarConfiguration::~TFarConfiguration()
{
  SAFE_DESTROY(FBookmarks);
}

void TFarConfiguration::Default()
{
  TGUIConfiguration::Default();

  FForceInheritance = false;
  FConfirmOverwritingOverride = false;
  FConfirmSynchronizedBrowsing = true;

  SetDisksMenu(true);
  SetDisksMenuHotKey(0);
  SetPluginsMenu(true);
  SetPluginsMenuCommands(true);
  SetCommandPrefixes("netbox,ftp,scp,sftp,ftps,http,https,webdav");
  SetSessionNameInTitle(true);
  SetEditorDownloadDefaultMode(true);
  SetEditorUploadSameOptions(true);
  FEditorUploadOnSave = true;
  FEditorMultiple = false;
  FQueueBeep = true;

  SetCustomPanelModeDetailed(true);
  SetFullScreenDetailed(true);
  SetColumnTypesDetailed("N,S,DM,O,G,R");
  SetColumnWidthsDetailed("0,8,14,0,0,9");
  SetStatusColumnTypesDetailed("NR");
  SetStatusColumnWidthsDetailed("0");

  SetApplyCommandCommand(L"");
  SetApplyCommandParams(0);

  SetPuttygenPath(FormatCommand(::ExtractFilePath(ModuleFileName()) + "putty\\puttygen.exe", L""));
  SetPageantPath(FormatCommand(::ExtractFilePath(ModuleFileName()) + "putty\\pageant.exe", L""));

  FBookmarks->Clear();
}

THierarchicalStorage *TFarConfiguration::CreateStorage(bool & /*SessionList*/)
{
  DebugAssert(FFarPlugin);
  return FFarPlugin ? new TFar3Storage(GetRegistryStorageKey(), MainGuid, FFarPlugin->GetStartupInfo()->SettingsControl) : nullptr;
}

void TFarConfiguration::Saved()
{
  TGUIConfiguration::Saved();
  FBookmarks->ModifyAll(false);
}

// duplicated from core\configuration.cpp
#define LASTELEM(ELEM) \
  ELEM.SubString(ELEM.LastDelimiter(L".>")+1, ELEM.Length() - ELEM.LastDelimiter(L".>"))
#define BLOCK(KEY, CANCREATE, BLOCK) \
  if (Storage->OpenSubKey(KEY, CANCREATE, true)) \
  { \
    SCOPE_EXIT { Storage->CloseSubKey(); }; \
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
    KEY(Bool,     SessionNameInTitle); \
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
  )

void TFarConfiguration::SaveData(THierarchicalStorage *Storage, bool All)
{
  TGUIConfiguration::SaveData(Storage, All);

  // duplicated from core\configuration.cpp
#define KEY(TYPE, VAR) Storage->Write ## TYPE(LASTELEM(MB2W(#VAR)), Get##VAR())
  REGCONFIG(true);
#undef KEY

  if (Storage->OpenSubKey(L"Bookmarks", /*CanCreate=*/true))
  {
    FBookmarks->Save(Storage, All);

    Storage->CloseSubKey();
  }
}

void TFarConfiguration::LoadData(THierarchicalStorage *Storage)
{
  TGUIConfiguration::LoadData(Storage);

  // duplicated from core\configuration.cpp
#define KEY(TYPE, VAR) Set##VAR(Storage->Read ## TYPE(LASTELEM(MB2W(#VAR)), Get##VAR()))
  REGCONFIG(false);
#undef KEY

  if (Storage->OpenSubKey(L"Bookmarks", false))
  {
    FBookmarks->Load(Storage);
    Storage->CloseSubKey();
  }
}

void TFarConfiguration::Load()
{
  std::unique_ptr<THierarchicalStorage> Storage(CreateConfigStorage());
  FForceInheritance = true;
  SCOPE_EXIT
  {
    FForceInheritance = false;
  };
  TGUIConfiguration::Load(Storage.get());
}

void TFarConfiguration::Save(bool All, bool Explicit)
{
  FForceInheritance = true;
  SCOPE_EXIT
  {
    FForceInheritance = false;
  };
  TGUIConfiguration::DoSave(All, Explicit);
}

void TFarConfiguration::SetPlugin(TCustomFarPlugin *Value)
{
  if (GetPlugin() != Value)
  {
    DebugAssert(!GetPlugin() || !Value);
    FFarPlugin = Value;
  }
}

int64_t TFarConfiguration::GetSetting(FARSETTINGS_SUBFOLDERS Root, const wchar_t * Name) const
{
  int64_t Result = 0;
  FarSettingsCreate settings = {sizeof(FarSettingsCreate), FarGuid, INVALID_HANDLE_VALUE};
  HANDLE Settings = FFarPlugin->GetStartupInfo()->SettingsControl(INVALID_HANDLE_VALUE, SCTL_CREATE, 0, &settings) ? settings.Handle : 0;
  if (Settings)
  {
    FarSettingsItem item = {sizeof(FarSettingsItem), (size_t)Root, Name, FST_UNKNOWN, {0} };
    if (FFarPlugin->GetStartupInfo()->SettingsControl(Settings, SCTL_GET, 0, &item) && FST_QWORD == item.Type)
    {
        Result = item.Number;
    }
    FFarPlugin->GetStartupInfo()->SettingsControl(Settings, SCTL_FREE, 0, 0);
  }
  return Result;
}

int64_t TFarConfiguration::GetConfirmationsSetting(HANDLE & Settings, const wchar_t * Name) const
{
  FarSettingsItem item = {sizeof(FarSettingsItem), FSSF_CONFIRMATIONS, Name, FST_UNKNOWN, {0} };
  if (FFarPlugin->GetStartupInfo()->SettingsControl(Settings, SCTL_GET, 0, &item) && FST_QWORD == item.Type)
  {
    return item.Number;
  }
  return 0;
}


int64_t TFarConfiguration::GetConfirmationsSettings() const
{
  int64_t Result = 0;
  FarSettingsCreate settings = {sizeof(FarSettingsCreate), FarGuid, INVALID_HANDLE_VALUE};
  HANDLE Settings = FFarPlugin->GetStartupInfo()->SettingsControl(INVALID_HANDLE_VALUE, SCTL_CREATE, 0, &settings) ? settings.Handle : 0;
  if (Settings)
  {
    if (GetConfirmationsSetting(Settings, L"Copy"))
      Result |= NBCS_COPYOVERWRITE;
    if (GetConfirmationsSetting(Settings, L"Move"))
      Result |= NBCS_MOVEOVERWRITE;
    // if (GetConfirmationsSetting(Settings, L"RO"))
        // result |= NBCS_MOVEOVERWRITE;
    if (GetConfirmationsSetting(Settings, L"Drag"))
      Result |= NBCS_DRAGANDDROP;
    if (GetConfirmationsSetting(Settings, L"Delete"))
      Result |= NBCS_DELETE;
    if (GetConfirmationsSetting(Settings, L"DeleteFolder"))
      Result |= NBCS_DELETENONEMPTYFOLDERS;
    if (GetConfirmationsSetting(Settings, L"Esc"))
      Result |= NBCS_INTERRUPTOPERATION;
    if (GetConfirmationsSetting(Settings, L"AllowReedit"))
      Result |= NBCS_RELOADEDITEDFILE;
    if (GetConfirmationsSetting(Settings, L"HistoryClear"))
      Result |= NBCS_CLEARHISTORYLIST;
    if (GetConfirmationsSetting(Settings, L"Exit"))
      Result |= NBCS_EXIT;
    FFarPlugin->GetStartupInfo()->SettingsControl(Settings, SCTL_FREE, 0, 0);
  }
  return Result;
}

void TFarConfiguration::CacheFarSettings()
{
  FFarConfirmations = GetConfirmationsSettings();
}

intptr_t TFarConfiguration::FarConfirmations() const
{
  if (GetPlugin() && (GetCurrentThreadId() == GetPlugin()->GetFarThreadId()))
  {
    return GetConfirmationsSettings();
  }
  DebugAssert(FFarConfirmations >= 0);
  return FFarConfirmations;
}

bool TFarConfiguration::GetConfirmOverwriting() const
{
  if (FForceInheritance || FConfirmOverwritingOverride)
  {
    return TGUIConfiguration::GetConfirmOverwriting();
  }
  // DebugAssert(GetPlugin());
  return (FarConfirmations() & NBCS_COPYOVERWRITE) != 0;
}

void TFarConfiguration::SetConfirmOverwriting(bool Value)
{
  if (FForceInheritance)
  {
    TGUIConfiguration::SetConfirmOverwriting(Value);
  }
  else
  {
    if (GetConfirmOverwriting() != Value)
    {
      FConfirmOverwritingOverride = true;
      TGUIConfiguration::SetConfirmOverwriting(Value);
    }
  }
}

bool TFarConfiguration::GetConfirmDeleting() const
{
  DebugAssert(GetPlugin());
  return (FarConfirmations() & NBCS_DELETE) != 0;
}

UnicodeString TFarConfiguration::ModuleFileName() const
{
  DebugAssert(GetPlugin());
  return GetPlugin()->GetModuleName();
}

void TFarConfiguration::SetBookmarks(UnicodeString Key,
  TBookmarkList *Value)
{
  FBookmarks->SetBookmarks(Key, Value);
  Changed();
}

TBookmarkList *TFarConfiguration::GetBookmarks(UnicodeString Key)
{
  return FBookmarks->GetBookmarks(Key);
}

TFarConfiguration *GetFarConfiguration()
{
  return dyn_cast<TFarConfiguration>(GetConfiguration());
}

