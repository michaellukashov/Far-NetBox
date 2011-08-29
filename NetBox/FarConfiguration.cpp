//---------------------------------------------------------------------------
#include "stdafx.h"

#include "Common.h"
#include "Bookmarks.h"
#include "FarConfiguration.h"
#include "FarPlugin.h"
#include "FarUtil.h"
//---------------------------------------------------------------------------
TFarConfiguration::TFarConfiguration(TCustomFarPlugin * APlugin) :
  TGUIConfiguration()
{
  FFarConfirmations = -1;
  FPlugin = APlugin;
  FBookmarks = new TBookmarks();
  Default();
}
//---------------------------------------------------------------------------
TFarConfiguration::~TFarConfiguration()
{
  delete FBookmarks;
}
//---------------------------------------------------------------------------
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
  SetCommandPrefixes(L"winscp,scp,sftp");
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
  if (Storage->OpenSubKey(KEY, CANCREATE, true)) try { BLOCK } catch(...) { Storage->CloseSubKey(); }
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
void TFarConfiguration::SaveData(THierarchicalStorage * Storage,
  bool All)
{
  TGUIConfiguration::SaveData(Storage, All);

  // duplicated from core\configuration.cpp
  #define KEY(TYPE, VAR) Storage->Write ## TYPE(LASTELEM(std::wstring(::MB2W("##VAR"))), Get##VAR())
  REGCONFIG(true);
  #undef KEY

  if (Storage->OpenSubKey("Bookmarks", true))
  {
    FBookmarks->Save(Storage, All);

    Storage->CloseSubKey();
  }
}
//---------------------------------------------------------------------------
void TFarConfiguration::LoadData(THierarchicalStorage * Storage)
{
  TGUIConfiguration::LoadData(Storage);

  // duplicated from core\configuration.cpp
  #define KEY(TYPE, VAR) VAR = Storage->Read ## TYPE(LASTELEM(wstring(#VAR)), VAR)
  #pragma warn -eas
  REGCONFIG(false);
  #pragma warn +eas
  #undef KEY

  if (Storage->OpenSubKey("Bookmarks", false))
  {
    FBookmarks->Load(Storage);
    Storage->CloseSubKey();
  }
}
//---------------------------------------------------------------------------
void TFarConfiguration::Load()
{
  FForceInheritance = true;
  try
  {
    TGUIConfiguration::Load();
  }
  catch(...)
  {
    FForceInheritance = false;
  }
}
//---------------------------------------------------------------------------
void TFarConfiguration::Save(bool All, bool Explicit)
{
  FForceInheritance = true;
  try
  {
    TGUIConfiguration::Save(All, Explicit);
  }
  catch(...)
  {
    FForceInheritance = false;
  }
}
//---------------------------------------------------------------------------
void TFarConfiguration::SetPlugin(TCustomFarPlugin * value)
{
  if (Plugin != value)
  {
    assert(!Plugin || !value);
    FPlugin = value;
  }
}
//---------------------------------------------------------------------------
void TFarConfiguration::CacheFarSettings()
{
  FFarConfirmations = Plugin->FarAdvControl(ACTL_GETCONFIRMATIONS);
}
//---------------------------------------------------------------------------
int TFarConfiguration::FarConfirmations()
{
  if (GetCurrentThreadId() == Plugin->FarThread)
  {
    return Plugin->FarAdvControl(ACTL_GETCONFIRMATIONS);
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
    assert(Plugin);
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
    if (ConfirmOverwriting != value)
    {
      FConfirmOverwritingOverride = true;
      TGUIConfiguration::SetConfirmOverwriting(value);
    }
  }
}
//---------------------------------------------------------------------------
bool TFarConfiguration::GetConfirmDeleting()
{
  assert(Plugin);
  return (FarConfirmations() & FCS_DELETE) != 0;
}
//---------------------------------------------------------------------------
wstring TFarConfiguration::ModuleFileName()
{
  assert(Plugin);
  return Plugin->ModuleName;
}
//---------------------------------------------------------------------------
void TFarConfiguration::SetBookmark(wstring Key,
  TBookmarkList * value)
{
  FBookmarks->Bookmarks[Key] = value;
  Changed();
}
//---------------------------------------------------------------------------
TBookmarkList * TFarConfiguration::GetBookmark(wstring Key)
{
  return FBookmarks->Bookmarks[Key];
}
