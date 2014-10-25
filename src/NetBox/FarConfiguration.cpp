
#include <vcl.h>
#pragma hdrstop

#include "Common.h"
#include "Bookmarks.h"
#include "FarConfiguration.h"
#include "FarPlugin.h"
#include "CoreMain.h"

TFarConfiguration::TFarConfiguration(TCustomFarPlugin * APlugin) :
  TGUIConfiguration(),
  FFarPlugin(APlugin),
  FBookmarks(new TBookmarks()),
  FFarConfirmations(-1)
{
  Default();
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
  SetCommandPrefixes(L"netbox,ftp,scp,sftp,ftps,http,https,webdav");
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

  SetPuttygenPath(FormatCommand(::ExtractFilePath(ModuleFileName()) + L"putty\\puttygen.exe", L""));
  SetPageantPath(FormatCommand(::ExtractFilePath(ModuleFileName()) + L"putty\\pageant.exe", L""));

  FBookmarks->Clear();
}

THierarchicalStorage * TFarConfiguration::CreateStorage(bool SessionList)
{
  return TGUIConfiguration::CreateScpStorage(SessionList);
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

void TFarConfiguration::SaveData(THierarchicalStorage * Storage, bool All)
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

void TFarConfiguration::LoadData(THierarchicalStorage * Storage)
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
  FForceInheritance = true;
  SCOPE_EXIT
  {
    FForceInheritance = false;
  };
  TGUIConfiguration::Load();
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

void TFarConfiguration::SetPlugin(TCustomFarPlugin * Value)
{
  if (GetPlugin() != Value)
  {
    assert(!GetPlugin() || !Value);
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
  else
  {
    assert(FFarConfirmations >= 0);
    return FFarConfirmations;
  }
}

bool TFarConfiguration::GetConfirmOverwriting() const
{
  if (FForceInheritance || FConfirmOverwritingOverride)
  {
    return TGUIConfiguration::GetConfirmOverwriting();
  }
  else
  {
    // assert(GetPlugin());
    return (FarConfirmations() & FCS_COPYOVERWRITE) != 0;
  }
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
  assert(GetPlugin());
  return (FarConfirmations() & FCS_DELETE) != 0;
}

UnicodeString TFarConfiguration::ModuleFileName() const
{
  assert(GetPlugin());
  return GetPlugin()->GetModuleName();
}

void TFarConfiguration::SetBookmarks(const UnicodeString & Key,
  TBookmarkList * Value)
{
  FBookmarks->SetBookmarks(Key, Value);
  Changed();
}

TBookmarkList * TFarConfiguration::GetBookmarks(const UnicodeString & Key)
{
  return FBookmarks->GetBookmarks(Key);
}

TFarConfiguration * GetFarConfiguration()
{
  return NB_STATIC_DOWNCAST(TFarConfiguration, GetConfiguration());
}

NB_IMPLEMENT_CLASS(TFarConfiguration, NB_GET_CLASS_INFO(TGUIConfiguration), nullptr)

