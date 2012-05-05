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
#include "FarPlugin.h"
//---------------------------------------------------------------------------
#ifndef _MSC_VER
#pragma package(smart_init)
#endif
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
void __fastcall TFarConfiguration::CacheFarSettings()
{
  FFarConfirmations = GetPlugin()->FarAdvControl(ACTL_GETCONFIRMATIONS);
}
//---------------------------------------------------------------------------
int __fastcall TFarConfiguration::FarConfirmations()
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
bool __fastcall TFarConfiguration::GetConfirmOverwriting()
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
  return (FarConfirmations() & FCS_DELETE) != 0;
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
