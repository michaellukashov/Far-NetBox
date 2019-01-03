#include <vcl.h>
#pragma hdrstop

#include <Common.h>

#include "Bookmarks.h"
#include "FarConfiguration.h"
#include "FarPlugin.h"
#include "CoreMain.h"

TFarConfiguration::TFarConfiguration(TCustomFarPlugin *APlugin) noexcept :
  TGUIConfiguration(OBJECT_CLASS_TFarConfiguration),
  FFarPlugin(APlugin),
  FBookmarks(std::make_unique<TBookmarks>()),
  FFarConfirmations(-1)
{
//  TFarConfiguration::Default();
  CacheFarSettings();
}

TFarConfiguration::~TFarConfiguration() noexcept
{
//  SAFE_DESTROY(FBookmarks);
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

  SetApplyCommandCommand("");
  SetApplyCommandParams(0);

  SetPuttygenPath(FormatCommand(::ExtractFilePath(ModuleFileName()) + "putty\\puttygen.exe", ""));
  SetPageantPath(FormatCommand(::ExtractFilePath(ModuleFileName()) + "putty\\pageant.exe", ""));

  FBookmarks->Clear();
}

THierarchicalStorage *TFarConfiguration::CreateStorage(bool &SessionList)
{
  return TGUIConfiguration::CreateStorage(SessionList);
}

void TFarConfiguration::Saved()
{
  TGUIConfiguration::Saved();
  FBookmarks->ModifyAll(false);
}

// duplicated from core\configuration.cpp
#undef LASTELEM
#define LASTELEM(ELEM) \
  ELEM.SubString(ELEM.LastDelimiter(L".>")+1, ELEM.Length() - ELEM.LastDelimiter(L".>"))
#define BLOCK(KEY, CANCREATE, BLOCK) \
  if (Storage->OpenSubKey(KEY, CANCREATE, true)) \
  { \
    SCOPE_EXIT { Storage->CloseSubKey(); }; \
    BLOCK \
  }
#define REGCONFIG(CANCREATE) \
  BLOCK("Far", CANCREATE, \
    KEY(Bool,     DisksMenu); \
    KEY2(Integer,  DisksMenuHotKey); \
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
    KEY2(Integer,  ApplyCommandParams); \
    KEY(Bool,     ConfirmSynchronizedBrowsing); \
  )

void TFarConfiguration::SaveData(THierarchicalStorage *Storage, bool All)
{
  TGUIConfiguration::SaveData(Storage, All);

  // duplicated from core\configuration.cpp
#undef KEY
#undef KEY2
#define KEY(TYPE, VAR) Storage->Write ## TYPE(LASTELEM(MB2W(#VAR)), Get##VAR())
#define KEY2(TYPE, VAR) Storage->Write ## TYPE(LASTELEM(MB2W(#VAR)), nb::ToInt(Get##VAR()))
  REGCONFIG(true);
#undef KEY2
#undef KEY

  if (Storage->OpenSubKey("Bookmarks", /*CanCreate=*/true))
  {
    FBookmarks->Save(Storage, All);

    Storage->CloseSubKey();
  }
}

void TFarConfiguration::LoadData(THierarchicalStorage *Storage)
{
  TGUIConfiguration::LoadData(Storage);

  // duplicated from core\configuration.cpp
#undef KEY2
#undef KEY
#define KEY(TYPE, VAR) Set##VAR(Storage->Read ## TYPE(LASTELEM(MB2W(#VAR)), Get##VAR()))
#define KEY2(TYPE, VAR) Set##VAR(Storage->Read ## TYPE(LASTELEM(MB2W(#VAR)), nb::ToInt(Get##VAR())))
  REGCONFIG(false);
#undef KEY2
#undef KEY

  if (Storage->OpenSubKey("Bookmarks", false))
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

void TFarConfiguration::CacheFarSettings()
{
  if (GetPlugin())
  {
    FFarConfirmations = GetPlugin()->FarAdvControl(ACTL_GETCONFIRMATIONS);
  }
}

intptr_t TFarConfiguration::FarConfirmations() const
{
  if (GetPlugin() && (GetCurrentThreadId() == GetPlugin()->GetFarThreadId()))
  {
    return GetPlugin()->FarAdvControl(ACTL_GETCONFIRMATIONS);
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
  return (FarConfirmations() & FCS_COPYOVERWRITE) != 0;
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
  return (FarConfirmations() & FCS_DELETE) != 0;
}

UnicodeString TFarConfiguration::ModuleFileName() const
{
  DebugAssert(GetPlugin());
  return GetPlugin()->GetModuleName();
}

void TFarConfiguration::SetBookmarks(const UnicodeString Key,
  TBookmarkList *Value)
{
  FBookmarks->SetBookmarks(Key, Value);
  Changed();
}

TBookmarkList *TFarConfiguration::GetBookmarks(const UnicodeString Key)
{
  return FBookmarks->GetBookmarks(Key);
}

TFarConfiguration *GetFarConfiguration()
{
  return dyn_cast<TFarConfiguration>(GetConfiguration());
}

