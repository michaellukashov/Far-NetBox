//---------------------------------------------------------------------------
#include "stdafx.h"

#include "boostdefines.hpp"
#include <boost/scope_exit.hpp>

#include "Common.h"
#include "Bookmarks.h"
#include "FarConfiguration.h"
#include "FarPlugin.h"
//---------------------------------------------------------------------------
TFarConfiguration::TFarConfiguration(TCustomFarPlugin * APlugin) :
  TGUIConfiguration()
{
  // DEBUG_PRINTF(L"begin");
  Self = this;
  FFarConfirmations = -1;
  FPlugin = APlugin;
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
  SetCommandPrefixes(L"winscp,scp,sftp,ftps,http,https");
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
void TFarConfiguration::Saved()
{
  TGUIConfiguration::Saved();
  FBookmarks->ModifyAll(false);
}
//---------------------------------------------------------------------------
// duplicated from core\configuration.cpp
#define LASTELEM(ELEM) \
  ELEM.substr(LastDelimiter(ELEM, L".>")+1, ELEM.size() - LastDelimiter(ELEM, L".>"))
#define BLOCK(KEY, CANCREATE, BLOCK) \
  if (Storage->OpenSubKey(KEY, CANCREATE, true)) \
  { \
      BOOST_SCOPE_EXIT ( (Storage) ) \
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
void TFarConfiguration::SaveData(THierarchicalStorage * Storage,
  bool All)
{
  // DEBUG_PRINTF(L"begin");
  TGUIConfiguration::SaveData(Storage, All);

  // duplicated from core\configuration.cpp
  #define KEY(TYPE, VAR) Storage->Write ## TYPE(LASTELEM(::MB2W(#VAR)), Get##VAR())
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
void TFarConfiguration::LoadData(THierarchicalStorage * Storage)
{
  TGUIConfiguration::LoadData(Storage);

  // duplicated from core\configuration.cpp
  #define KEY(TYPE, VAR) Set##VAR(Storage->Read ## TYPE(LASTELEM(::MB2W(#VAR)), Get##VAR()))
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
void TFarConfiguration::SetPlugin(TCustomFarPlugin * value)
{
  if (GetPlugin() != value)
  {
    assert(!GetPlugin() || !value);
    FPlugin = value;
  }
}
//---------------------------------------------------------------------------
void TFarConfiguration::CacheFarSettings()
{
  FFarConfirmations = GetPlugin()->FarAdvControl(ACTL_GETCONFIRMATIONS);
}
//---------------------------------------------------------------------------
int TFarConfiguration::FarConfirmations()
{
  if (GetCurrentThreadId() == GetPlugin()->GetFarThread())
  {
    return GetPlugin()->FarAdvControl(ACTL_GETCONFIRMATIONS);
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
    return (FarConfirmations() & FCS_COPYOVERWRITE) != 0;
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
  return (FarConfirmations() & FCS_DELETE) != 0;
}
//---------------------------------------------------------------------------
std::wstring TFarConfiguration::ModuleFileName()
{
  assert(GetPlugin());
  return GetPlugin()->GetModuleName();
}
//---------------------------------------------------------------------------
void TFarConfiguration::SetBookmark(std::wstring Key,
  TBookmarkList * value)
{
  FBookmarks->SetBookmark(Key, value);
  Changed();
}
//---------------------------------------------------------------------------
TBookmarkList * TFarConfiguration::GetBookmark(std::wstring Key)
{
  return FBookmarks->GetBookmark(Key);
}
