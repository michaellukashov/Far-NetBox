//---------------------------------------------------------------------------
#include "Bookmarks.h"
#include "FarConfiguration.h"
#include "FarPlugin.h"
#include <Common.h>
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

  DisksMenu = true;
  DisksMenuHotKey = 0;
  PluginsMenu = true;
  PluginsMenuCommands = true;
  CommandPrefixes = "winscp,scp,sftp";
  HostNameInTitle = true;
  EditorDownloadDefaultMode = true;
  EditorUploadSameOptions = true;
  FEditorUploadOnSave = false;
  FEditorMultiple = false;
  FQueueBeep = true;

  CustomPanelModeDetailed = true;
  FullScreenDetailed = true;
  ColumnTypesDetailed = "N,S,DM,O,G,R";
  ColumnWidthsDetailed = "0,8,14,0,0,9";
  StatusColumnTypesDetailed = "NR";
  StatusColumnWidthsDetailed = "0";

  ApplyCommandCommand = "";
  ApplyCommandParams = 0;

  PuttygenPath = FormatCommand(ExtractFilePath(ModuleFileName()) + "putty\\puttygen.exe", "");
  PageantPath = FormatCommand(ExtractFilePath(ModuleFileName()) + "putty\\pageant.exe", "");

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
  ELEM.SubString(ELEM.LastDelimiter(".>")+1, ELEM.Length() - ELEM.LastDelimiter(".>"))
#define BLOCK(KEY, CANCREATE, BLOCK) \
  if (Storage->OpenSubKey(KEY, CANCREATE, true)) try { BLOCK } catch(...) { Storage->CloseSubKey(); }
#define REGCONFIG(CANCREATE) \
  BLOCK("Far", CANCREATE, \
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
  #define KEY(TYPE, VAR) Storage->Write ## TYPE(LASTELEM(wstring(#VAR)), VAR)
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
void TFarConfiguration::SetBookmarks(wstring Key,
  TBookmarkList * value)
{
  FBookmarks->Bookmarks[Key] = value;
  Changed();
}
//---------------------------------------------------------------------------
TBookmarkList * TFarConfiguration::GetBookmarks(wstring Key)
{
  return FBookmarks->Bookmarks[Key];
}
