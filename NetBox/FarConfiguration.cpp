//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include "Bookmarks.h"
#include "FarConfiguration.h"
#include "FarPlugin.h"
#include <Common.h>
//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------
__fastcall TFarConfiguration::TFarConfiguration(TCustomFarPlugin * APlugin) :
  TGUIConfiguration()
{
  FFarConfirmations = -1;
  FPlugin = APlugin;
  FBookmarks = new TBookmarks();
  Default();
}
//---------------------------------------------------------------------------
__fastcall TFarConfiguration::~TFarConfiguration()
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
void __fastcall TFarConfiguration::Saved()
{
  TGUIConfiguration::Saved();
  FBookmarks->ModifyAll(false);
}
//---------------------------------------------------------------------------
// duplicated from core\configuration.cpp
#define LASTELEM(ELEM) \
  ELEM.SubString(ELEM.LastDelimiter(".>")+1, ELEM.Length() - ELEM.LastDelimiter(".>"))
#define BLOCK(KEY, CANCREATE, BLOCK) \
  if (Storage->OpenSubKey(KEY, CANCREATE, true)) try { BLOCK } __finally { Storage->CloseSubKey(); }
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
void __fastcall TFarConfiguration::SaveData(THierarchicalStorage * Storage,
  bool All)
{
  TGUIConfiguration::SaveData(Storage, All);

  // duplicated from core\configuration.cpp
  #define KEY(TYPE, VAR) Storage->Write ## TYPE(LASTELEM(AnsiString(#VAR)), VAR)
  REGCONFIG(true);
  #undef KEY

  if (Storage->OpenSubKey("Bookmarks", true))
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
  #define KEY(TYPE, VAR) VAR = Storage->Read ## TYPE(LASTELEM(AnsiString(#VAR)), VAR)
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
void __fastcall TFarConfiguration::Load()
{
  FForceInheritance = true;
  try
  {
    TGUIConfiguration::Load();
  }
  __finally
  {
    FForceInheritance = false;
  }
}
//---------------------------------------------------------------------------
void __fastcall TFarConfiguration::Save(bool All, bool Explicit)
{
  FForceInheritance = true;
  try
  {
    TGUIConfiguration::Save(All, Explicit);
  }
  __finally
  {
    FForceInheritance = false;
  }
}
//---------------------------------------------------------------------------
void __fastcall TFarConfiguration::SetPlugin(TCustomFarPlugin * value)
{
  if (Plugin != value)
  {
    assert(!Plugin || !value);
    FPlugin = value;
  }
}
//---------------------------------------------------------------------------
void __fastcall TFarConfiguration::CacheFarSettings()
{
  FFarConfirmations = Plugin->FarAdvControl(ACTL_GETCONFIRMATIONS);
}
//---------------------------------------------------------------------------
int __fastcall TFarConfiguration::FarConfirmations()
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
bool __fastcall TFarConfiguration::GetConfirmOverwriting()
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
void __fastcall TFarConfiguration::SetConfirmOverwriting(bool value)
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
bool __fastcall TFarConfiguration::GetConfirmDeleting()
{
  assert(Plugin);
  return (FarConfirmations() & FCS_DELETE) != 0;
}
//---------------------------------------------------------------------------
AnsiString __fastcall TFarConfiguration::ModuleFileName()
{
  assert(Plugin);
  return Plugin->ModuleName;
}
//---------------------------------------------------------------------------
void __fastcall TFarConfiguration::SetBookmarks(AnsiString Key,
  TBookmarkList * value)
{
  FBookmarks->Bookmarks[Key] = value;
  Changed();
}
//---------------------------------------------------------------------------
TBookmarkList * __fastcall TFarConfiguration::GetBookmarks(AnsiString Key)
{
  return FBookmarks->Bookmarks[Key];
}
