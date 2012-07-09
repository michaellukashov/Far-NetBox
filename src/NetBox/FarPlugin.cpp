//---------------------------------------------------------------------------
#ifndef _MSC_VER
#include <vcl.h>
#pragma hdrstop
#else
#include "nbafx.h"

#include "boostdefines.hpp"
#include <boost/scope_exit.hpp>
#include <boost/bind.hpp>
#endif

#include "FarPlugin.h"
#include "FarDialog.h"
#include <Common.h>
// FAR WORKAROUND
#include "Exceptions.h"
#include "TextsCore.h"
#include "FileMasks.h"
#include "RemoteFiles.h"
#include "puttyexp.h"

//---------------------------------------------------------------------------
TCustomFarPlugin * FarPlugin = NULL;
#define FAR_TITLE_SUFFIX L" - Far"
//---------------------------------------------------------------------------
#ifndef _MSC_VER
#pragma package(smart_init)
#endif
//---------------------------------------------------------------------------
TFarMessageParams::TFarMessageParams()
{
  MoreMessages = NULL;
  CheckBox = false;
  Timer = 0;
  TimerAnswer = 0;
  TimerEvent = NULL;
  Timeout = 0;
  TimeoutButton = 0;
  ClickEvent = NULL;
}
//---------------------------------------------------------------------------
/* __fastcall */ TCustomFarPlugin::TCustomFarPlugin(HINSTANCE HInst) :
  TObject()
{
  // DEBUG_PRINTF(L"TCustomFarPlugin: begin");
  InitPlatformId();
  Self = this;
  FFarThread = GetCurrentThreadId();
  FCriticalSection = new TCriticalSection;
  FHandle = HInst;
  FANSIApis = AreFileApisANSI() > 0;
  FFarVersion = 0;
  FTerminalScreenShowing = false;

  FOpenedPlugins = new TObjectList();
  FOpenedPlugins->SetOwnsObjects(false);
  FSavedTitles = new TStringList();
  FTopDialog = NULL;
  // FOldFar = true;
  FValidFarSystemSettings = false;

  memset(&FPluginInfo, 0, sizeof(FPluginInfo));
  ClearPluginInfo(FPluginInfo);

  // far\Examples\Compare\compare.cpp
  FConsoleInput = CreateFile(L"CONIN$", GENERIC_READ, FILE_SHARE_READ, NULL,
    OPEN_EXISTING, 0, NULL);
  FConsoleOutput = CreateFile(L"CONOUT$", GENERIC_READ | GENERIC_WRITE,
    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
  if (ConsoleWindowState() == SW_SHOWNORMAL)
  {
    FNormalConsoleSize = TerminalInfo();
  }
  else
  {
    FNormalConsoleSize = TPoint(-1, -1);
  }
  // DEBUG_PRINTF(L"TCustomFarPlugin: end");
}
//---------------------------------------------------------------------------
/* __fastcall */ TCustomFarPlugin::~TCustomFarPlugin()
{
  assert(FTopDialog == NULL);

  ResetCachedInfo();
  CloseHandle(FConsoleInput);
  FConsoleInput = INVALID_HANDLE_VALUE;
  CloseHandle(FConsoleOutput);
  FConsoleOutput = INVALID_HANDLE_VALUE;

  ClearPluginInfo(FPluginInfo);
  assert(FOpenedPlugins->GetCount() == 0);
  delete FOpenedPlugins;
  delete FSavedTitles;
  delete FCriticalSection;
}
//---------------------------------------------------------------------------
bool __fastcall TCustomFarPlugin::HandlesFunction(THandlesFunction /*Function*/)
{
  return false;
}
//---------------------------------------------------------------------------
int __fastcall TCustomFarPlugin::GetMinFarVersion()
{
  return 0;
}
//---------------------------------------------------------------------------
void __fastcall TCustomFarPlugin::SetStartupInfo(const struct PluginStartupInfo * Info)
{
  try
  {
    ResetCachedInfo();
    // Info->StructSize = 336 for FAR 1.65
    memset(&FStartupInfo, 0, sizeof(FStartupInfo));
    memmove(&FStartupInfo, Info,
            Info->StructSize >= sizeof(FStartupInfo) ?
            sizeof(FStartupInfo) : static_cast<size_t>(Info->StructSize));
    // the minimum we really need
    assert(FStartupInfo.GetMsg != NULL);
    assert(FStartupInfo.Message != NULL);

    memset(&FFarStandardFunctions, 0, sizeof(FFarStandardFunctions));
    size_t FSFOffset = (static_cast<const char *>(reinterpret_cast<const void *>(&Info->FSF)) -
                        static_cast<const char *>(reinterpret_cast<const void *>(Info)));
    if (static_cast<size_t>(Info->StructSize) > FSFOffset)
    {
      memmove(&FFarStandardFunctions, Info->FSF,
              static_cast<size_t>(Info->FSF->StructSize) >= sizeof(FFarStandardFunctions) ?
              sizeof(FFarStandardFunctions) : Info->FSF->StructSize);
    }
  }
  catch (Exception & E)
  {
    DEBUG_PRINTF(L"before HandleException");
    HandleException(&E);
  }
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::ExitFAR()
{
}
//---------------------------------------------------------------------------
void __fastcall TCustomFarPlugin::GetPluginInfo(struct PluginInfo * Info)
{
  try
  {
    ResetCachedInfo();
    Info->StructSize = sizeof(PluginInfo);
    TStringList DiskMenuStrings;
    TStringList PluginMenuStrings;
    TStringList PluginConfigStrings;
    TStringList CommandPrefixes;

    ClearPluginInfo(FPluginInfo);

    GetPluginInfoEx(FPluginInfo.Flags, &DiskMenuStrings, &PluginMenuStrings,
                    &PluginConfigStrings, &CommandPrefixes);

    #define COMPOSESTRINGARRAY(NAME) \
        if (NAME.GetCount()) \
        { \
          wchar_t ** StringArray = new wchar_t *[NAME.GetCount()]; \
          FPluginInfo.NAME = StringArray; \
          FPluginInfo.NAME ## Number = static_cast<int>(NAME.GetCount()); \
          for (int Index = 0; Index < NAME.GetCount(); Index++) \
          { \
            StringArray[Index] = DuplicateStr(NAME.GetStrings(Index)); \
          } \
        }

    COMPOSESTRINGARRAY(DiskMenuStrings);
    COMPOSESTRINGARRAY(PluginMenuStrings);
    COMPOSESTRINGARRAY(PluginConfigStrings);

    #undef COMPOSESTRINGARRAY
    // FIXME
    /*
    if (DiskMenuStrings.GetCount())
    {
        wchar_t *NumberArray = new wchar_t[DiskMenuStrings.GetCount()];
        FPluginInfo.DiskMenuNumbers = &NumberArray;
        for (int Index = 0; Index < DiskMenuStrings.GetCount(); Index++)
        {
            NumberArray[Index] = (int)DiskMenuStrings.GetObject(Index);
        }
    }
    */

    UnicodeString CommandPrefix;
    for (int Index = 0; Index < CommandPrefixes.GetCount(); Index++)
    {
      CommandPrefix = CommandPrefix + (CommandPrefix.IsEmpty() ? L"" : L":") +
                      CommandPrefixes.GetStrings(Index);
    }
    // DEBUG_PRINTF(L"CommandPrefix = %s", CommandPrefix.c_str());
    FPluginInfo.CommandPrefix = DuplicateStr(CommandPrefix);

    memmove(Info, &FPluginInfo, sizeof(FPluginInfo));
  }
  catch (Exception & E)
  {
    DEBUG_PRINTF(L"before HandleException");
    HandleException(&E);
  }
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TCustomFarPlugin::GetModuleName()
{
  return FStartupInfo.ModuleName;
}
//---------------------------------------------------------------------------
void __fastcall TCustomFarPlugin::ClearPluginInfo(PluginInfo & Info)
{
  if (Info.StructSize)
  {
    #define FREESTRINGARRAY(NAME) \
      for (int Index = 0; Index < Info.NAME ## Number; Index++) \
      { \
        delete[] Info.NAME[Index]; \
      } \
      delete[] Info.NAME; \
      Info.NAME = NULL;

    FREESTRINGARRAY(DiskMenuStrings);
    FREESTRINGARRAY(PluginMenuStrings);
    FREESTRINGARRAY(PluginConfigStrings);

    #undef FREESTRINGARRAY

    // FIXME delete[] Info.DiskMenuNumbers;
    delete[] Info.CommandPrefix;
  }
  memset(&Info, 0, sizeof(Info));
  Info.StructSize = sizeof(Info);
}
//---------------------------------------------------------------------------
wchar_t * TCustomFarPlugin::DuplicateStr(const UnicodeString Str, bool AllowEmpty)
{
  if (Str.IsEmpty() && !AllowEmpty)
  {
    return NULL;
  }
  else
  {
    // DEBUG_PRINTF(L"Str = %s", Str.c_str());
    const size_t sz = Str.Length() + 1;
    wchar_t * Result = new wchar_t[sz];
    wcscpy_s(Result, sz, Str.c_str());
    return Result;
  }
}
//---------------------------------------------------------------------------
RECT __fastcall TCustomFarPlugin::GetPanelBounds(HANDLE PanelHandle)
{
  PanelInfo Info;
  FarControl(FCTL_GETPANELINFO, 0, reinterpret_cast<intptr_t>(&Info),
             PanelHandle);
  RECT Bounds;
  memset(&Bounds, -1, sizeof(Bounds));
  if (Info.Plugin)
  {
    Bounds = Info.PanelRect;
  }
  return Bounds;
}

//---------------------------------------------------------------------------
TCustomFarFileSystem * __fastcall TCustomFarPlugin::GetPanelFileSystem(bool Another,
    HANDLE /* Plugin */)
{
  // DEBUG_PRINTF(L"begin");
  TCustomFarFileSystem * Result = NULL;
  RECT ActivePanelBounds = GetPanelBounds(PANEL_ACTIVE);
  RECT PassivePanelBounds = GetPanelBounds(PANEL_PASSIVE);

  TCustomFarFileSystem * FileSystem = NULL;
  int Index = 0;
  while (!Result && (Index < FOpenedPlugins->GetCount()))
  {
    FileSystem = dynamic_cast<TCustomFarFileSystem *>(FOpenedPlugins->GetItem(Index));
    assert(FileSystem);
    RECT Bounds = GetPanelBounds(FileSystem);
    if (Another && CompareRects(Bounds, PassivePanelBounds))
    {
      Result = FileSystem;
    }
    else if (!Another && CompareRects(Bounds, ActivePanelBounds))
    {
      Result = FileSystem;
    }
    Index++;
  }
  // DEBUG_PRINTF(L"end");
  return Result;
}
//---------------------------------------------------------------------------
void __fastcall TCustomFarPlugin::InvalidateOpenPluginInfo()
{
  for (int Index = 0; Index < FOpenedPlugins->GetCount(); Index++)
  {
    TCustomFarFileSystem * FileSystem =
      dynamic_cast<TCustomFarFileSystem *>(FOpenedPlugins->GetItem(Index));
    FileSystem->InvalidateOpenPluginInfo();
  }
}
//---------------------------------------------------------------------------
int __fastcall TCustomFarPlugin::Configure(int Item)
{
  try
  {
    ResetCachedInfo();
    int Result = ConfigureEx(Item);
    InvalidateOpenPluginInfo();

    return Result;
  }
  catch (Exception & E)
  {
    DEBUG_PRINTF(L"before HandleException");
    HandleException(&E);
    return (int)false;
  }
}
//---------------------------------------------------------------------------
void * __fastcall TCustomFarPlugin::OpenPlugin(int OpenFrom, intptr_t Item)
{
  try
  {
    ResetCachedInfo();

    UnicodeString Buf;
    if ((OpenFrom == OPEN_SHORTCUT) || (OpenFrom == OPEN_COMMANDLINE))
    {
      Buf = reinterpret_cast<wchar_t *>(Item);
      Item = reinterpret_cast<intptr_t>(Buf.c_str());
    }

    TCustomFarFileSystem * Result = OpenPluginEx(OpenFrom, Item);

    if (Result)
    {
      FOpenedPlugins->Add(Result);
    }
    else
    {
      Result = (TCustomFarFileSystem *)INVALID_HANDLE_VALUE;
    }

    return Result;
  }
  catch(Exception & E)
  {
    DEBUG_PRINTF(L"before HandleException");
    HandleException(&E);
    return INVALID_HANDLE_VALUE;
  }
}
//---------------------------------------------------------------------------
void __fastcall TCustomFarPlugin::ClosePlugin(void * Plugin)
{
  try
  {
    ResetCachedInfo();
    TCustomFarFileSystem * FileSystem = static_cast<TCustomFarFileSystem *>(Plugin);
    assert(FOpenedPlugins->IndexOf(FileSystem) != NPOS);
    // try
    {
      BOOST_SCOPE_EXIT ( (&Self) (&FileSystem) )
      {
        Self->FOpenedPlugins->Remove(FileSystem);
      } BOOST_SCOPE_EXIT_END
      {
        TGuard Guard(FileSystem->GetCriticalSection());
        FileSystem->Close();
      }
      delete FileSystem;
    }
#ifndef _MSC_VER
    __finally
    {
      FOpenedPlugins->Remove(FileSystem);
    }
#endif
  }
  catch(Exception & E)
  {
    DEBUG_PRINTF(L"before HandleException");
    HandleException(&E);
  }
}
//---------------------------------------------------------------------------
void __fastcall TCustomFarPlugin::HandleFileSystemException(
  TCustomFarFileSystem * FileSystem, Exception * E, int OpMode)
{
  // This method is called as last-resort exception handler before
  // leaving plugin API. Especially for API fuctions that must update
  // panel contents on themselves (like ProcessKey), the instance of filesystem
  // may not exists anymore.
  // Check against object pointer is stupid, but no other idea so far.
  if (FOpenedPlugins->IndexOf(FileSystem) != NPOS)
  {
    DEBUG_PRINTF(L"before FileSystem->HandleException");
    FileSystem->HandleException(E, OpMode);
  }
  else
  {
    DEBUG_PRINTF(L"before HandleException");
    HandleException(E, OpMode);
  }
}
//---------------------------------------------------------------------------
void __fastcall TCustomFarPlugin::GetOpenPluginInfo(HANDLE Plugin,
  struct OpenPluginInfo * Info)
{
  TCustomFarFileSystem * FileSystem = static_cast<TCustomFarFileSystem *>(Plugin);
  try
  {
    ResetCachedInfo();
    assert(FOpenedPlugins->IndexOf(FileSystem) != NPOS);

    {
      TGuard Guard(FileSystem->GetCriticalSection());
      FileSystem->GetOpenPluginInfo(Info);
    }
  }
  catch(Exception & E)
  {
    DEBUG_PRINTF(L"before HandleFileSystemException");
    HandleFileSystemException(FileSystem, &E);
  }
}
//---------------------------------------------------------------------------
int __fastcall TCustomFarPlugin::GetFindData(HANDLE Plugin,
  struct PluginPanelItem ** PanelItem, int * ItemsNumber, int OpMode)
{
  TCustomFarFileSystem * FileSystem = static_cast<TCustomFarFileSystem *>(Plugin);
  try
  {
    ResetCachedInfo();
    assert(FOpenedPlugins->IndexOf(FileSystem) != NPOS);

    {
      TGuard Guard(FileSystem->GetCriticalSection());
      return FileSystem->GetFindData(PanelItem, ItemsNumber, OpMode);
    }
  }
  catch (Exception & E)
  {
    DEBUG_PRINTF(L"before HandleFileSystemException");
    HandleFileSystemException(FileSystem, &E, OpMode);
    return 0;
  }
}
//---------------------------------------------------------------------------
void __fastcall TCustomFarPlugin::FreeFindData(HANDLE Plugin,
  struct PluginPanelItem * PanelItem, int ItemsNumber)
{
  TCustomFarFileSystem * FileSystem = static_cast<TCustomFarFileSystem *>(Plugin);
  try
  {
    ResetCachedInfo();
    assert(FOpenedPlugins->IndexOf(FileSystem) != NPOS);

    {
      TGuard Guard(FileSystem->GetCriticalSection());
      FileSystem->FreeFindData(PanelItem, ItemsNumber);
    }
  }
  catch(Exception & E)
  {
    DEBUG_PRINTF(L"before HandleFileSystemException");
    HandleFileSystemException(FileSystem, &E);
  }
}
//---------------------------------------------------------------------------
int __fastcall TCustomFarPlugin::ProcessHostFile(HANDLE Plugin,
  struct PluginPanelItem * PanelItem, int ItemsNumber, int OpMode)
{
  TCustomFarFileSystem * FileSystem = static_cast<TCustomFarFileSystem *>(Plugin);
  try
  {
    ResetCachedInfo();
    if (HandlesFunction(hfProcessHostFile))
    {
      assert(FOpenedPlugins->IndexOf(FileSystem) != NPOS);

      {
        TGuard Guard(FileSystem->GetCriticalSection());
        return FileSystem->ProcessHostFile(PanelItem, ItemsNumber, OpMode);
      }
    }
    else
    {
      return 0;
    }
  }
  catch(Exception & E)
  {
    DEBUG_PRINTF(L"before HandleFileSystemException");
    HandleFileSystemException(FileSystem, &E, OpMode);
    return 0;
  }
}
//---------------------------------------------------------------------------
int __fastcall TCustomFarPlugin::ProcessKey(HANDLE Plugin, int Key,
  unsigned int ControlState)
{
  TCustomFarFileSystem * FileSystem = static_cast<TCustomFarFileSystem *>(Plugin);
  try
  {
    ResetCachedInfo();
    if (HandlesFunction(hfProcessKey))
    {
      assert(FOpenedPlugins->IndexOf(FileSystem) != NPOS);

      {
        TGuard Guard(FileSystem->GetCriticalSection());
        return FileSystem->ProcessKey(Key, ControlState);
      }
    }
    else
    {
      return 0;
    }
  }
  catch(Exception & E)
  {
    DEBUG_PRINTF(L"before HandleFileSystemException");
    HandleFileSystemException(FileSystem, &E);
    // when error occurs, assume that key can be handled by plugin and
    // should not be processed by FAR
    return 1;
  }
}
//---------------------------------------------------------------------------
int __fastcall TCustomFarPlugin::ProcessEvent(HANDLE Plugin, int Event, void * Param)
{
  TCustomFarFileSystem * FileSystem = static_cast<TCustomFarFileSystem *>(Plugin);
  try
  {
    ResetCachedInfo();
    if (HandlesFunction(hfProcessEvent))
    {
      assert(FOpenedPlugins->IndexOf(FileSystem) != NPOS);

      UnicodeString Buf;
      if ((Event == FE_CHANGEVIEWMODE) || (Event == FE_COMMAND))
      {
        Buf = static_cast<wchar_t *>(Param);
        Param = const_cast<void *>(reinterpret_cast<const void *>(Buf.c_str()));
      }

      {
        TGuard Guard(FileSystem->GetCriticalSection());
        return FileSystem->ProcessEvent(Event, Param);
      }
    }
    else
    {
      return (int)false;
    }
  }
  catch(Exception & E)
  {
    DEBUG_PRINTF(L"before HandleFileSystemException");
    HandleFileSystemException(FileSystem, &E);
    return Event == FE_COMMAND ? true : false;
  }
}
//---------------------------------------------------------------------------
int __fastcall TCustomFarPlugin::SetDirectory(HANDLE Plugin, const wchar_t * Dir, int OpMode)
{
  TCustomFarFileSystem * FileSystem = static_cast<TCustomFarFileSystem *>(Plugin);
  try
  {
    ResetCachedInfo();
    assert(FOpenedPlugins->IndexOf(FileSystem) != NPOS);

    {
      TGuard Guard(FileSystem->GetCriticalSection());
      return FileSystem->SetDirectory(Dir, OpMode);
    }
  }
  catch(Exception & E)
  {
    DEBUG_PRINTF(L"before HandleFileSystemException");
    HandleFileSystemException(FileSystem, &E, OpMode);
    return 0;
  }
}
//---------------------------------------------------------------------------
int __fastcall TCustomFarPlugin::MakeDirectory(HANDLE Plugin, const wchar_t ** Name, int OpMode)
{
  TCustomFarFileSystem * FileSystem = static_cast<TCustomFarFileSystem *>(Plugin);
  try
  {
    ResetCachedInfo();
    assert(FOpenedPlugins->IndexOf(FileSystem) != NPOS);

    {
      TGuard Guard(FileSystem->GetCriticalSection());
      return FileSystem->MakeDirectory(Name, OpMode);
    }
  }
  catch(Exception & E)
  {
    DEBUG_PRINTF(L"before HandleFileSystemException");
    HandleFileSystemException(FileSystem, &E, OpMode);
    return 0;
  }
}
//---------------------------------------------------------------------------
int __fastcall TCustomFarPlugin::DeleteFiles(HANDLE Plugin,
  struct PluginPanelItem * PanelItem, int ItemsNumber, int OpMode)
{
  TCustomFarFileSystem * FileSystem = static_cast<TCustomFarFileSystem *>(Plugin);
  try
  {
    ResetCachedInfo();
    assert(FOpenedPlugins->IndexOf(FileSystem) != NPOS);

    {
      TGuard Guard(FileSystem->GetCriticalSection());
      return FileSystem->DeleteFiles(PanelItem, ItemsNumber, OpMode);
    }
  }
  catch (Exception & E)
  {
    DEBUG_PRINTF(L"before HandleFileSystemException");
    HandleFileSystemException(FileSystem, &E, OpMode);
    return 0;
  }
}
//---------------------------------------------------------------------------
int __fastcall TCustomFarPlugin::GetFiles(HANDLE Plugin,
  struct PluginPanelItem * PanelItem, int ItemsNumber, int Move,
  const wchar_t ** DestPath, int OpMode)
{
  TCustomFarFileSystem * FileSystem = static_cast<TCustomFarFileSystem *>(Plugin);
  try
  {
    ResetCachedInfo();
    assert(FOpenedPlugins->IndexOf(FileSystem) != NPOS);

    {
      TGuard Guard(FileSystem->GetCriticalSection());
      return FileSystem->GetFiles(PanelItem, ItemsNumber, Move, DestPath, OpMode);
    }
  }
  catch(Exception & E)
  {
    DEBUG_PRINTF(L"before HandleFileSystemException");
    // display error even for OPM_FIND
    HandleFileSystemException(FileSystem, &E, OpMode & ~OPM_FIND);
    return 0;
  }
}
//---------------------------------------------------------------------------
int __fastcall TCustomFarPlugin::PutFiles(HANDLE Plugin,
  struct PluginPanelItem * PanelItem, int ItemsNumber, int Move, int OpMode)
{
  TCustomFarFileSystem * FileSystem = static_cast<TCustomFarFileSystem *>(Plugin);
  try
  {
    ResetCachedInfo();
    assert(FOpenedPlugins->IndexOf(FileSystem) != NPOS);

    {
      TGuard Guard(FileSystem->GetCriticalSection());
      return FileSystem->PutFiles(PanelItem, ItemsNumber, Move, OpMode);
    }
  }
  catch(Exception & E)
  {
    DEBUG_PRINTF(L"before HandleFileSystemException");
    HandleFileSystemException(FileSystem, &E, OpMode);
    return 0;
  }
}
//---------------------------------------------------------------------------
int __fastcall TCustomFarPlugin::ProcessEditorEvent(int Event, void * Param)
{
  try
  {
    ResetCachedInfo();

    return ProcessEditorEventEx(Event, Param);
  }
  catch(Exception & E)
  {
    DEBUG_PRINTF(L"before HandleException");
    HandleException(&E);
    return 0;
  }
}
//---------------------------------------------------------------------------
int __fastcall TCustomFarPlugin::ProcessEditorInput(const INPUT_RECORD * Rec)
{
  try
  {
    ResetCachedInfo();

    return ProcessEditorInputEx(Rec);
  }
  catch(Exception & E)
  {
    DEBUG_PRINTF(L"before HandleException");
    HandleException(&E);
    // when error occurs, assume that input event can be handled by plugin and
    // should not be processed by FAR
    return 1;
  }
}
//---------------------------------------------------------------------------
int __fastcall TCustomFarPlugin::MaxMessageLines()
{
  return TerminalInfo().y - 5;
}
//---------------------------------------------------------------------------
int __fastcall TCustomFarPlugin::MaxMenuItemLength()
{
  // got from maximal length of path in FAR's folders history
  return TerminalInfo().x - 13;
}
//---------------------------------------------------------------------------
int __fastcall TCustomFarPlugin::MaxLength(TStrings * Strings)
{
  int Result = 0;
  for (int Index = 0; Index < Strings->GetCount(); Index++)
  {
    if (Result < Strings->GetStrings(Index).Length())
    {
      Result = Strings->GetStrings(Index).Length();
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
class TFarMessageDialog : public TFarDialog
{
public:
  /* __fastcall */ TFarMessageDialog(TCustomFarPlugin * Plugin,
    TFarMessageParams * Params);
  void __fastcall Init(unsigned int AFlags,
    const UnicodeString Title, const UnicodeString Message, TStrings * Buttons);

  int __fastcall Execute(bool & ACheckBox);

protected:
  virtual void __fastcall Change();
  virtual void __fastcall Idle();

private:
  void /* __fastcall */ ButtonClick(TFarButton * Sender, bool & Close);

private:
  bool FCheckBoxChecked;
  TFarMessageParams * FParams;
  TDateTime FStartTime;
  TDateTime FLastTimerTime;
  TFarButton * FTimeoutButton;
  UnicodeString FTimeoutButtonCaption;
  TFarCheckBox * FCheckBox;
};
//---------------------------------------------------------------------------
/* __fastcall */ TFarMessageDialog::TFarMessageDialog(TCustomFarPlugin * Plugin,
  TFarMessageParams * Params) :
  TFarDialog(Plugin),
  FCheckBoxChecked(false),
  FParams(Params),
  FTimeoutButton(NULL),
  FCheckBox(NULL)
{
  assert(Params != NULL);
}
//---------------------------------------------------------------------------
void __fastcall TFarMessageDialog::Init(unsigned int AFlags,
  const UnicodeString Title, const UnicodeString Message, TStrings * Buttons)
{
  assert(FLAGCLEAR(AFlags, FMSG_ERRORTYPE));
  assert(FLAGCLEAR(AFlags, FMSG_KEEPBACKGROUND));
  // FIXME assert(FLAGCLEAR(AFlags, FMSG_DOWN));
  assert(FLAGCLEAR(AFlags, FMSG_ALLINONE));

  TStrings * MessageLines = new TStringList();
  TStrings * MoreMessageLines = NULL;
  // try
  {
    BOOST_SCOPE_EXIT ( (&MessageLines) (&MoreMessageLines) )
    {
      delete MessageLines;
      delete MoreMessageLines;
    } BOOST_SCOPE_EXIT_END
    FarWrapText(Message, MessageLines, MaxMessageWidth);
    int MaxLen = GetFarPlugin()->MaxLength(MessageLines);
    // DEBUG_PRINTF(L"MaxLen = %d, FParams->MoreMessages = %x", MaxLen, FParams->MoreMessages);
    if (FParams->MoreMessages != NULL)
    {
      MoreMessageLines = new TStringList();
      UnicodeString MoreMessages = FParams->MoreMessages->GetText();
      while (MoreMessages[MoreMessages.Length()] == L'\n' ||
             MoreMessages[MoreMessages.Length()] == L'\r')
      {
        MoreMessages.SetLength(MoreMessages.Length() - 1);
      }
      FarWrapText(MoreMessages, MoreMessageLines, MaxMessageWidth);
      int MoreMaxLen = GetFarPlugin()->MaxLength(MoreMessageLines);
      if (MaxLen < MoreMaxLen)
      {
        MaxLen = MoreMaxLen;
      }
    }

    // temporary
    // DEBUG_PRINTF(L"MaxMessageWidth = %d, Title = %s", MaxMessageWidth, Title.c_str());
    SetSize(TPoint(MaxMessageWidth, 10));
    SetCaption(Title);
    SetFlags(GetFlags() |
             FLAGMASK(FLAGSET(AFlags, FMSG_WARNING), FDLG_WARNING));

    for (int Index = 0; Index < MessageLines->GetCount(); Index++)
    {
      TFarText * Text = new TFarText(this);
      Text->SetCaption(MessageLines->GetStrings(Index));
    }

    TFarLister * MoreMessagesLister = NULL;
    TFarSeparator * MoreMessagesSeparator = NULL;

    if (FParams->MoreMessages != NULL)
    {
      new TFarSeparator(this);

      MoreMessagesLister = new TFarLister(this);
      MoreMessagesLister->GetItems()->Assign(MoreMessageLines);
      MoreMessagesLister->SetLeft(GetBorderBox()->GetLeft() + 1);

      MoreMessagesSeparator = new TFarSeparator(this);
    }

    int ButtonOffset = (FParams->CheckBoxLabel.IsEmpty() ? -1 : -2);
    int ButtonLines = 1;
    TFarButton * Button = NULL;
    FTimeoutButton = NULL;
    for (int Index = 0; Index < Buttons->GetCount(); Index++)
    {
      TFarButton * PrevButton = Button;
      Button = new TFarButton(this);
      Button->SetDefault(Index == 0);
      Button->SetBrackets(brNone);
      Button->SetOnClick(fastdelegate::bind(&TFarMessageDialog::ButtonClick, this, _1, _2));
      UnicodeString Caption = Buttons->GetStrings(Index);
      if ((FParams->Timeout > 0) &&
          (FParams->TimeoutButton == Index))
      {
        FTimeoutButtonCaption = Caption;
        Caption = FORMAT(FParams->TimeoutStr.c_str(), Caption.c_str(), static_cast<int>(FParams->Timeout / 1000));
        FTimeoutButton = Button;
      }
      Button->SetCaption(FORMAT(L" %s ", Caption.c_str()));
      Button->SetTop(GetBorderBox()->GetBottom() + ButtonOffset);
      Button->SetBottom(Button->GetTop());
      Button->SetResult(Index + 1);
      Button->SetCenterGroup(true);
      Button->SetTag(reinterpret_cast<size_t>(Buttons->GetObjects(Index)));
      if (PrevButton != NULL)
      {
        Button->Move(PrevButton->GetRight() - Button->GetLeft() + 1, 0);
      }

      if (MaxMessageWidth < Button->GetRight() - GetBorderBox()->GetLeft())
      {
        for (int PIndex = 0; PIndex < GetItemCount(); PIndex++)
        {
          TFarButton * PrevButton = dynamic_cast<TFarButton *>(GetItem(PIndex));
          if ((PrevButton != NULL) && (PrevButton != Button))
          {
            PrevButton->Move(0, -1);
          }
        }
        Button->Move(-(Button->GetLeft() - GetBorderBox()->GetLeft()), 0);
        ButtonLines++;
      }

      // DEBUG_PRINTF(L"Button->GetLeft = %d, Button->GetRight = %d, GetBorderBox()->GetLeft = %d", Button->GetLeft(), Button->GetRight(), GetBorderBox()->GetLeft());
      if (MaxLen < Button->GetRight() - GetBorderBox()->GetLeft())
      {
        MaxLen = Button->GetRight() - GetBorderBox()->GetLeft();
      }
      // DEBUG_PRINTF(L"MaxLen = %d", MaxLen);

      SetNextItemPosition(ipRight);
    }

    // DEBUG_PRINTF(L"FParams->CheckBoxLabel = %s", FParams->CheckBoxLabel.c_str());
    if (!FParams->CheckBoxLabel.IsEmpty())
    {
      SetNextItemPosition(ipNewLine);
      FCheckBox = new TFarCheckBox(this);
      FCheckBox->SetCaption(FParams->CheckBoxLabel);

      if (MaxLen < FCheckBox->GetRight() - GetBorderBox()->GetLeft())
      {
        MaxLen = FCheckBox->GetRight() - GetBorderBox()->GetLeft();
      }
    }
    else
    {
      FCheckBox = NULL;
    }

    TRect rect = GetClientRect();
    // DEBUG_PRINTF(L"rect.Left = %d, MaxLen = %d, rect.Right = %d", rect.Left, MaxLen, rect.Right);
    TPoint S(
      // rect.Left + MaxLen + (-(rect.Right + 1)),
      rect.Left + MaxLen - rect.Right,
      rect.Top + MessageLines->GetCount() +
      (FParams->MoreMessages != NULL ? 1 : 0) + ButtonLines +
      (!FParams->CheckBoxLabel.IsEmpty() ? 1 : 0) +
      (-(rect.Bottom + 1)));

    if (FParams->MoreMessages != NULL)
    {
      int MoreMessageHeight = GetFarPlugin()->TerminalInfo().y - S.y - 1;
      if (MoreMessageHeight > MoreMessagesLister->GetItems()->GetCount())
      {
        MoreMessageHeight = MoreMessagesLister->GetItems()->GetCount();
      }
      assert(MoreMessagesLister != NULL);
      MoreMessagesLister->SetHeight(MoreMessageHeight);
      MoreMessagesLister->SetRight(
        GetBorderBox()->GetRight() - (MoreMessagesLister->GetScrollBar() ? 0 : 1));
      MoreMessagesLister->SetTabStop(MoreMessagesLister->GetScrollBar());
      assert(MoreMessagesSeparator != NULL);
      MoreMessagesSeparator->SetPosition(
        MoreMessagesLister->GetTop() + MoreMessagesLister->GetHeight());
      S.y += static_cast<int>(MoreMessagesLister->GetHeight()) + 1;
    }
    // DEBUG_PRINTF(L"S.x = %d, S.y = %d", S.x, S.y);
    SetSize(S);
  }
#ifndef _MSC_VER
  __finally
  {
    delete MessageLines;
    delete MoreMessageLines;
  }
#endif
}

//---------------------------------------------------------------------------
void __fastcall TFarMessageDialog::Idle()
{
  TFarDialog::Idle();

  if (FParams->Timer > 0)
  {
    size_t SinceLastTimer = static_cast<size_t>((static_cast<double>(Now()) - static_cast<double>(FLastTimerTime)) * 24*60*60*1000);
    if (SinceLastTimer >= FParams->Timeout)
    {
      assert(!FParams->TimerEvent.empty());
      if (!FParams->TimerEvent.empty())
      {
        FParams->TimerAnswer = 0;
        FParams->TimerEvent(FParams->TimerAnswer);
        if (FParams->TimerAnswer != 0)
        {
          Close(GetDefaultButton());
        }
        FLastTimerTime = Now();
      }
    }
  }

  if (FParams->Timeout > 0)
  {
    size_t Running = static_cast<size_t>((static_cast<double>(Now()) - static_cast<double>(FStartTime)) * 24*60*60*1000);
    if (Running >= FParams->Timeout)
    {
      assert(FTimeoutButton != NULL);
      Close(FTimeoutButton);
    }
    else
    {
      UnicodeString Caption =
        FORMAT(L" %s ", FORMAT(FParams->TimeoutStr.c_str(),
                               FTimeoutButtonCaption.c_str(), static_cast<int>((FParams->Timeout - Running) / 1000)).c_str()).c_str();
      size_t sz = FTimeoutButton->GetCaption().Length() > Caption.Length() ? FTimeoutButton->GetCaption().Length() - Caption.Length() : 0;
      Caption += ::StringOfChar(L' ', sz);
      FTimeoutButton->SetCaption(Caption);
    }
  }
}
//---------------------------------------------------------------------------
void __fastcall TFarMessageDialog::Change()
{
  TFarDialog::Change();

  if (GetHandle() != NULL)
  {
    if ((FCheckBox != NULL) && (FCheckBoxChecked != FCheckBox->GetChecked()))
    {
      for (int Index = 0; Index < GetItemCount(); Index++)
      {
        TFarButton * Button = dynamic_cast<TFarButton *>(GetItem(Index));
        if ((Button != NULL) && (Button->GetTag() == 0))
        {
          Button->SetEnabled(!FCheckBox->GetChecked());
        }
      }
      FCheckBoxChecked = FCheckBox->GetChecked();
    }
  }
}
//---------------------------------------------------------------------------
int __fastcall TFarMessageDialog::Execute(bool & ACheckBox)
{
  FStartTime = Now();
  FLastTimerTime = FStartTime;
  FCheckBoxChecked = !ACheckBox;
  if (FCheckBox != NULL)
  {
    FCheckBox->SetChecked(ACheckBox);
  }

  int Result = ShowModal();
  assert(Result != 0);
  if (Result > 0)
  {
    if (FCheckBox != NULL)
    {
      ACheckBox = FCheckBox->GetChecked();
    }
    Result--;
  }
  return Result;
}
//---------------------------------------------------------------------------
void /* __fastcall */ TFarMessageDialog::ButtonClick(TFarButton * Sender, bool & Close)
{
  if (!FParams->ClickEvent.empty())
  {
    FParams->ClickEvent(FParams->Token, Sender->GetResult() - 1, Close);
  }
}
//---------------------------------------------------------------------------
int __fastcall TCustomFarPlugin::DialogMessage(unsigned int Flags,
  const UnicodeString Title, const UnicodeString Message, TStrings * Buttons,
  TFarMessageParams * Params)
{
  int Result;
  TFarMessageDialog * Dialog =
    new TFarMessageDialog(this, Params);
  // try
  {
    BOOST_SCOPE_EXIT ( (&Dialog) )
    {
      delete Dialog;
    } BOOST_SCOPE_EXIT_END
    Dialog->Init(Flags, Title, Message, Buttons);
    Result = Dialog->Execute(Params->CheckBox);
  }
#ifndef _MSC_VER
  __finally
  {
    delete Dialog;
  }
#endif
  return Result;
}
//---------------------------------------------------------------------------
int __fastcall TCustomFarPlugin::FarMessage(unsigned int Flags,
  const UnicodeString Title, const UnicodeString Message, TStrings * Buttons,
  TFarMessageParams * Params)
{
  assert(Params != NULL);

  int Result;
  TStringList * MessageLines = NULL;
  wchar_t ** Items = NULL;
  // try
  {
    BOOST_SCOPE_EXIT ( (&MessageLines) (&Items) )
    {
      delete MessageLines;
      delete[] Items;
    } BOOST_SCOPE_EXIT_END
    UnicodeString FullMessage = Message;
    if (Params->MoreMessages != NULL)
    {
      FullMessage += UnicodeString(L"\n\x01\n") + Params->MoreMessages->GetText();
      while (FullMessage[FullMessage.Length()] == L'\n' ||
             FullMessage[FullMessage.Length()] == L'\r')
      {
        FullMessage.SetLength(FullMessage.Length() - 1);
      }
      FullMessage += L"\n\x01\n";
    }

    MessageLines = new TStringList();
    MessageLines->Add(Title);
    FarWrapText(FullMessage, MessageLines, MaxMessageWidth);

    // FAR WORKAROUND
    // When there is too many lines to fit on screen, far uses not-shown
    // lines as button captions instead of real captions at the end of the list
    int MaxLines = MaxMessageLines();
    while (MessageLines->GetCount() > MaxLines)
    {
      MessageLines->Delete(MessageLines->GetCount() - 1);
    }

    for (int Index = 0; Index < Buttons->GetCount(); Index++)
    {
      MessageLines->Add(Buttons->GetStrings(Index));
    }

    Items = new wchar_t *[MessageLines->GetCount()];
    for (int Index = 0; Index < MessageLines->GetCount(); Index++)
    {
      UnicodeString S = MessageLines->GetStrings(Index);
      MessageLines->PutString(Index, UnicodeString(S));
      Items[Index] = const_cast<wchar_t *>(MessageLines->GetStrings(Index).c_str());
    }

    TFarEnvGuard Guard;
    Result = FStartupInfo.Message(FStartupInfo.ModuleNumber,
      Flags | FMSG_LEFTALIGN, NULL, Items, static_cast<int>(MessageLines->GetCount()),
      static_cast<int>(Buttons->GetCount()));
  }
#ifndef _MSC_VER
  __finally
  {
    delete Items;
    delete MessageLines;
  }
#endif

  return Result;
}
//---------------------------------------------------------------------------
int __fastcall TCustomFarPlugin::Message(unsigned int Flags,
  const UnicodeString Title, const UnicodeString Message, TStrings * Buttons,
  TFarMessageParams * Params)
{
  // DEBUG_PRINTF(L"Message = %s", Message.c_str());
  // when message is shown while some "custom" output is on screen,
  // make the output actually background of FAR screen
  if (FTerminalScreenShowing)
  {
    FarControl(FCTL_SETUSERSCREEN, 0, NULL);
  }

  int Result;
  if (Buttons != NULL)
  {
    TFarMessageParams DefaultParams;
    TFarMessageParams * AParams = (Params == NULL ? &DefaultParams : Params);
    Result = DialogMessage(Flags, Title, Message, Buttons, AParams);
  }
  else
  {
    assert(Params == NULL);
    UnicodeString Items = Title + L"\n" + Message;
    TFarEnvGuard Guard;
    Result = FStartupInfo.Message(FStartupInfo.ModuleNumber,
      Flags | FMSG_ALLINONE | FMSG_LEFTALIGN,
      NULL,
      static_cast<const wchar_t * const *>(static_cast<const void *>(Items.c_str())), 0, 0);
  }
  return Result;
}
//---------------------------------------------------------------------------
int __fastcall TCustomFarPlugin::Menu(unsigned int Flags, UnicodeString Title,
  UnicodeString Bottom, const FarMenuItem * Items, int Count,
  const int * BreakKeys, int & BreakCode)
{
  assert(Items);

  UnicodeString ATitle = Title;
  UnicodeString ABottom = Bottom;
  TFarEnvGuard Guard;
  return static_cast<size_t>(FStartupInfo.Menu(FStartupInfo.ModuleNumber, -1, -1, 0,
    Flags, ATitle.c_str(), ABottom.c_str(), NULL, BreakKeys,
    &BreakCode, Items, Count));
}
//---------------------------------------------------------------------------
int __fastcall TCustomFarPlugin::Menu(unsigned int Flags, const UnicodeString Title,
  const UnicodeString Bottom, TStrings * Items, const int * BreakKeys,
  int & BreakCode)
{
  assert(Items && Items->GetCount());
  int Result = 0;
  FarMenuItemEx * MenuItems = new FarMenuItemEx[Items->GetCount()];
  {
    BOOST_SCOPE_EXIT ( (&MenuItems) )
    {
      delete[] MenuItems;
    } BOOST_SCOPE_EXIT_END
    int Selected = NPOS;
    int Count = 0;
    for (int i = 0; i < Items->GetCount(); i++)
    {
      size_t flags = reinterpret_cast<size_t>(Items->GetObjects(i));
      if (FLAGCLEAR(Flags, MIF_HIDDEN))
      {
        memset(&MenuItems[Count], 0, sizeof(MenuItems[Count]));
        UnicodeString Text = Items->GetStrings(i).c_str();
        MenuItems[Count].Flags = flags;
        if (MenuItems[Count].Flags & MIF_SELECTED)
        {
          assert(Selected == NPOS);
          Selected = i;
        }
        UnicodeString Str = Text;
        MenuItems[Count].Text = TCustomFarPlugin::DuplicateStr(Str);
        MenuItems[Count].UserData = i;
        Count++;
      }
    }

    int ResultItem = Menu(Flags | FMENU_USEEXT, Title, Bottom,
      reinterpret_cast<const FarMenuItem *>(MenuItems), Count, BreakKeys, BreakCode);

    if (ResultItem >= 0)
    {
      Result = MenuItems[ResultItem].UserData;
      if (Selected >= 0)
      {
        Items->PutObject(Selected, (TObject *)(reinterpret_cast<size_t>(Items->GetObjects(Selected)) & ~MIF_SELECTED));
      }
      Items->PutObject(Result, (TObject *)(reinterpret_cast<size_t>(Items->GetObjects(Result)) | MIF_SELECTED));
    }
    else
    {
      Result = ResultItem;
    }
  }
#ifndef _MSC_VER
  __finally
  {
    delete[] MenuItems;
  }
#endif
  return Result;
}
//---------------------------------------------------------------------------
int __fastcall TCustomFarPlugin::Menu(unsigned int Flags, const UnicodeString Title,
  const UnicodeString Bottom, TStrings * Items)
{
  int BreakCode;
  return Menu(Flags, Title, Bottom, Items, NULL, BreakCode);
}
//---------------------------------------------------------------------------
bool __fastcall TCustomFarPlugin::InputBox(const UnicodeString Title,
  const UnicodeString Prompt, UnicodeString & Text, unsigned long Flags,
  const UnicodeString HistoryName, size_t MaxLen, TFarInputBoxValidateEvent OnValidate)
{
  bool Repeat = false;
  int Result = 0;
  do
  {
    UnicodeString DestText;
    DestText.SetLength(MaxLen + 1);
    HANDLE ScreenHandle = 0;
    SaveScreen(ScreenHandle);
    UnicodeString AText = Text;
    {
      TFarEnvGuard Guard;
      Result = FStartupInfo.InputBox(
        Title.c_str(),
        Prompt.c_str(),
        HistoryName.c_str(),
        AText.c_str(),
        const_cast<wchar_t *>(DestText.c_str()),
        MaxLen,
        NULL,
        FIB_ENABLEEMPTY | FIB_BUTTONS | Flags);
    }
    RestoreScreen(ScreenHandle);
    Repeat = false;
    if (Result)
    {
      Text = DestText.c_str();
      if (OnValidate)
      {
        try
        {
          OnValidate(Text);
        }
        catch (Exception & E)
        {
          DEBUG_PRINTF(L"before HandleException");
          HandleException(&E);
          Repeat = true;
        }
      }
    }
  }
  while (Repeat);

  return (Result != 0);
}
//---------------------------------------------------------------------------
void __fastcall TCustomFarPlugin::Text(int X, int Y, int Color, const UnicodeString Str)
{
  TFarEnvGuard Guard;
  FStartupInfo.Text(X, Y, Color, Str.c_str());
}
//---------------------------------------------------------------------------
void __fastcall TCustomFarPlugin::FlushText()
{
  TFarEnvGuard Guard;
  FStartupInfo.Text(0, 0, 0, NULL);
}
//---------------------------------------------------------------------------
void __fastcall TCustomFarPlugin::WriteConsole(const UnicodeString Str)
{
  unsigned long Written;
  ::WriteConsole(FConsoleOutput, Str.c_str(), static_cast<DWORD>(Str.Length()), &Written, NULL);
}
//---------------------------------------------------------------------------
void __fastcall TCustomFarPlugin::FarCopyToClipboard(const UnicodeString Str)
{
  TFarEnvGuard Guard;
  FFarStandardFunctions.CopyToClipboard(Str.c_str());
}
//---------------------------------------------------------------------------
void __fastcall TCustomFarPlugin::FarCopyToClipboard(TStrings * Strings)
{
  if (Strings->GetCount() > 0)
  {
    if (Strings->GetCount() == 1)
    {
      FarCopyToClipboard(Strings->GetStrings(0));
    }
    else
    {
      FarCopyToClipboard(Strings->GetText());
    }
  }
}
//---------------------------------------------------------------------------
TPoint __fastcall TCustomFarPlugin::TerminalInfo(TPoint * Size, TPoint * Cursor)
{
  // DEBUG_PRINTF(L"begin");
  CONSOLE_SCREEN_BUFFER_INFO BufferInfo;
  GetConsoleScreenBufferInfo(FConsoleOutput, &BufferInfo);

  TPoint Result(BufferInfo.dwSize.X, BufferInfo.dwSize.Y);

  if (Size != NULL)
  {
    *Size = Result;
  }

  if (Cursor != NULL)
  {
    Cursor->x = BufferInfo.dwCursorPosition.X;
    Cursor->y = BufferInfo.dwCursorPosition.Y;
  }
  // DEBUG_PRINTF(L"end");
  return Result;
}
//---------------------------------------------------------------------------
HWND __fastcall TCustomFarPlugin::GetConsoleWindow()
{
  wchar_t Title[1024];
  GetConsoleTitle(Title, sizeof(Title) - 1);
  HWND Result = FindWindow(NULL, Title);
  return Result;
}
//---------------------------------------------------------------------------
unsigned int TCustomFarPlugin::ConsoleWindowState()
{
  unsigned int Result;
  HWND Window = GetConsoleWindow();
  if (Window != NULL)
  {
    WINDOWPLACEMENT WindowPlacement;
    WindowPlacement.length = sizeof(WindowPlacement);
    Win32Check(GetWindowPlacement(Window, &WindowPlacement) > 0);
    Result = WindowPlacement.showCmd;
  }
  else
  {
    Result = SW_SHOWNORMAL;
  }
  return Result;
}
//---------------------------------------------------------------------------
void __fastcall TCustomFarPlugin::ToggleVideoMode()
{
  HWND Window = GetConsoleWindow();
  if (Window != NULL)
  {
    if (ConsoleWindowState() == SW_SHOWMAXIMIZED)
    {
      if (FNormalConsoleSize.x >= 0)
      {
        COORD Size = { static_cast<short>(FNormalConsoleSize.x), static_cast<short>(FNormalConsoleSize.y) };

        Win32Check(ShowWindow(Window, SW_RESTORE) > 0);

        SMALL_RECT WindowSize;
        WindowSize.Left = 0;
        WindowSize.Top = 0;
        WindowSize.Right = static_cast<short>(Size.X - 1);
        WindowSize.Bottom = static_cast<short>(Size.Y - 1);
        Win32Check(SetConsoleWindowInfo(FConsoleOutput, true, &WindowSize) > 0);

        Win32Check(SetConsoleScreenBufferSize(FConsoleOutput, Size) > 0);
      }
    }
    else
    {
      COORD Size = GetLargestConsoleWindowSize(FConsoleOutput);
      Win32Check((Size.X != 0) || (Size.Y != 0));

      FNormalConsoleSize = TerminalInfo();

      Win32Check(ShowWindow(Window, SW_MAXIMIZE) > 0);

      Win32Check(SetConsoleScreenBufferSize(FConsoleOutput, Size) > 0);

      CONSOLE_SCREEN_BUFFER_INFO BufferInfo;
      Win32Check(GetConsoleScreenBufferInfo(FConsoleOutput, &BufferInfo) > 0);

      SMALL_RECT WindowSize;
      WindowSize.Left = 0;
      WindowSize.Top = 0;
      WindowSize.Right = static_cast<short>(BufferInfo.dwMaximumWindowSize.X - 1);
      WindowSize.Bottom = static_cast<short>(BufferInfo.dwMaximumWindowSize.Y - 1);
      Win32Check(SetConsoleWindowInfo(FConsoleOutput, true, &WindowSize) > 0);
    }
  }
}
//---------------------------------------------------------------------------
void __fastcall TCustomFarPlugin::ScrollTerminalScreen(int Rows)
{
  TPoint Size = TerminalInfo();

  SMALL_RECT Source;
  COORD Dest;
  CHAR_INFO Fill;
  Source.Left = 0;
  Source.Top = static_cast<char>(Rows);
  Source.Right = static_cast<SHORT>(Size.x);
  Source.Bottom = static_cast<SHORT>(Size.y);
  Dest.X = 0;
  Dest.Y = 0;
  Fill.Char.UnicodeChar = L' ';
  Fill.Attributes = 7;
  ScrollConsoleScreenBuffer(FConsoleOutput, &Source, NULL, Dest, &Fill);
}
//---------------------------------------------------------------------------
void __fastcall TCustomFarPlugin::ShowTerminalScreen()
{
  assert(!FTerminalScreenShowing);
  TPoint Size, Cursor;
  TerminalInfo(&Size, &Cursor);

  UnicodeString Blank = ::StringOfChar(L' ', Size.x);
  // Blank.SetLength(static_cast<size_t>(Size.x));
  for (int Y = 0; Y < Size.y; Y++)
  {
    Text(0, Y, 7/* LIGHTGRAY */, Blank);
  }
  FlushText();

  COORD Coord;
  Coord.X = 0;
  Coord.Y = static_cast<SHORT>(Cursor.y);
  SetConsoleCursorPosition(FConsoleOutput, Coord);
  FTerminalScreenShowing = true;
}
//---------------------------------------------------------------------------
void __fastcall TCustomFarPlugin::SaveTerminalScreen()
{
  FTerminalScreenShowing = false;
  FarControl(FCTL_SETUSERSCREEN, 0, NULL);
}
//---------------------------------------------------------------------------
struct TConsoleTitleParam
{
  short Progress;
  short Own;
};
//---------------------------------------------------------------------------
void __fastcall TCustomFarPlugin::ShowConsoleTitle(const UnicodeString Title)
{
  wchar_t SaveTitle[1024];
  GetConsoleTitle(SaveTitle, sizeof(SaveTitle));
  TConsoleTitleParam Param;
  Param.Progress = FCurrentProgress;
  Param.Own = !FCurrentTitle.IsEmpty() && (FormatConsoleTitle() == SaveTitle);
  assert(sizeof(Param) == sizeof(TObject *));
  if (Param.Own)
  {
    FSavedTitles->AddObject(FCurrentTitle, *reinterpret_cast<TObject **>(&Param));
  }
  else
  {
    FSavedTitles->AddObject(SaveTitle, *reinterpret_cast<TObject **>(&Param));
  }
  FCurrentTitle = Title;
  FCurrentProgress = -1;
  UpdateConsoleTitle();
}
//---------------------------------------------------------------------------
void __fastcall TCustomFarPlugin::ClearConsoleTitle()
{
  assert(FSavedTitles->GetCount() > 0);
  UnicodeString Title = FSavedTitles->GetStrings(FSavedTitles->GetCount()-1);
  TObject * Object = static_cast<TObject *>(FSavedTitles->GetObjects(FSavedTitles->GetCount()-1));
  TConsoleTitleParam Param = *reinterpret_cast<TConsoleTitleParam *>(&Object);
  if (Param.Own)
  {
    FCurrentTitle = Title;
    FCurrentProgress = Param.Progress;
    UpdateConsoleTitle();
  }
  else
  {
    FCurrentTitle = L"";
    FCurrentProgress = -1;
    SetConsoleTitle(Title.c_str());
    UpdateProgress(PS_NOPROGRESS, 0);
  }
  FSavedTitles->Delete(FSavedTitles->GetCount() - 1);
}
//---------------------------------------------------------------------------
void __fastcall TCustomFarPlugin::UpdateConsoleTitle(const UnicodeString Title)
{
  // assert(!FCurrentTitle.IsEmpty());
  FCurrentTitle = Title;
  UpdateConsoleTitle();
}
//---------------------------------------------------------------------------
void __fastcall TCustomFarPlugin::UpdateConsoleTitleProgress(short Progress)
{
  // assert(!FCurrentTitle.IsEmpty());
  FCurrentProgress = Progress;
  UpdateConsoleTitle();
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TCustomFarPlugin::FormatConsoleTitle()
{
  UnicodeString Title;
  if (FCurrentProgress >= 0)
  {
    Title = FORMAT(L"{%d%%} %s", FCurrentProgress, FCurrentTitle.c_str());
  }
  else
  {
    Title = FCurrentTitle;
  }
  Title += FAR_TITLE_SUFFIX;
  return Title;
}
//---------------------------------------------------------------------------
void __fastcall TCustomFarPlugin::UpdateProgress(int state, int progress)
{
  FarAdvControl(ACTL_SETPROGRESSSTATE, (void *)state);
  if (state == PS_NORMAL)
  {
    PROGRESSVALUE pv;
    pv.Completed = progress < 0 ? 0 : progress > 100 ? 100 : progress;
    pv.Total = 100;
    FarAdvControl(ACTL_SETPROGRESSVALUE, (void *)&pv);
  }
}
//---------------------------------------------------------------------------
void __fastcall TCustomFarPlugin::UpdateConsoleTitle()
{
  UnicodeString Title = FormatConsoleTitle();
  SetConsoleTitle(Title.c_str());
  short progress = FCurrentProgress != -1 ? FCurrentProgress : 0;
  UpdateProgress(progress != 0 ? PS_NORMAL : PS_NOPROGRESS, progress);
}
//---------------------------------------------------------------------------
void __fastcall TCustomFarPlugin::SaveScreen(HANDLE & Screen)
{
  assert(!Screen);
  TFarEnvGuard Guard;
  Screen = static_cast<HANDLE>(FStartupInfo.SaveScreen(0, 0, -1, -1));
  assert(Screen);
}
//---------------------------------------------------------------------------
void __fastcall TCustomFarPlugin::RestoreScreen(HANDLE & Screen)
{
  assert(Screen);
  TFarEnvGuard Guard;
  FStartupInfo.RestoreScreen(static_cast<HANDLE>(Screen));
  Screen = 0;
}
//---------------------------------------------------------------------------
void __fastcall TCustomFarPlugin::HandleException(Exception * E, int /*OpMode*/)
{
  assert(E);
  Message(FMSG_WARNING | FMSG_MB_OK, L"", MB2W(E->what()));
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TCustomFarPlugin::GetMsg(int MsgId)
{
  TFarEnvGuard Guard;
  UnicodeString Result = FStartupInfo.GetMsg(FStartupInfo.ModuleNumber, MsgId);
  return Result;
}
//---------------------------------------------------------------------------
bool __fastcall TCustomFarPlugin::CheckForEsc()
{
  INPUT_RECORD Rec;
  unsigned long ReadCount;
  while (PeekConsoleInput(FConsoleInput, &Rec, 1, &ReadCount) && ReadCount)
  {
    ReadConsoleInput(FConsoleInput, &Rec, 1, &ReadCount);
    if (Rec.EventType == KEY_EVENT &&
        Rec.Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE &&
        Rec.Event.KeyEvent.bKeyDown)
    {
      return true;
    }
  }
  return false;
}
//---------------------------------------------------------------------------
bool __fastcall TCustomFarPlugin::Viewer(UnicodeString FileName,
  unsigned int Flags, UnicodeString Title)
{
  TFarEnvGuard Guard;
  int Result = FStartupInfo.Viewer(
    FileName.c_str(),
    Title.c_str(), 0, 0, -1, -1, Flags,
    CP_AUTODETECT);
  return Result > 0;
}
//---------------------------------------------------------------------------
bool __fastcall TCustomFarPlugin::Editor(const UnicodeString FileName,
  unsigned int Flags, UnicodeString Title)
{
  TFarEnvGuard Guard;
  int Result = FStartupInfo.Editor(
                 FileName.c_str(),
                 Title.c_str(), 0, 0, -1, -1, Flags, -1, -1,
                 CP_AUTODETECT);
  return (Result == EEC_MODIFIED) || (Result == EEC_NOT_MODIFIED);
}
//---------------------------------------------------------------------------
void __fastcall TCustomFarPlugin::ResetCachedInfo()
{
  FValidFarSystemSettings = false;
}
//---------------------------------------------------------------------------
intptr_t __fastcall TCustomFarPlugin::FarSystemSettings()
{
  if (!FValidFarSystemSettings)
  {
    FFarSystemSettings = FarAdvControl(ACTL_GETSYSTEMSETTINGS);
    FValidFarSystemSettings = true;
  }
  return FFarSystemSettings;
}
//---------------------------------------------------------------------------
DWORD __fastcall TCustomFarPlugin::FarControl(int Command, int Param1, intptr_t Param2, HANDLE Plugin)
{
  switch (Command)
  {
  case FCTL_CLOSEPLUGIN:
  case FCTL_SETPANELDIR:
  case FCTL_SETCMDLINE:
  case FCTL_INSERTCMDLINE:
    break;

  case FCTL_GETCMDLINE:
  case FCTL_GETCMDLINESELECTEDTEXT:
    // ANSI/OEM translation not implemented yet
    assert(false);
    break;
  }

  TFarEnvGuard Guard;
  return FStartupInfo.Control(Plugin, Command, Param1, Param2);
}
//---------------------------------------------------------------------------
intptr_t __fastcall TCustomFarPlugin::FarAdvControl(int Command, void * Param)
{
  TFarEnvGuard Guard;
  return FStartupInfo.AdvControl(FStartupInfo.ModuleNumber, Command, Param);
}
//---------------------------------------------------------------------------
int __fastcall TCustomFarPlugin::FarEditorControl(int Command, void * Param)
{
  UnicodeString Buf;
  switch (Command)
  {
  case ECTL_GETINFO:
  case ECTL_SETPARAM:
  case ECTL_GETFILENAME:
    // noop
    break;

  case ECTL_SETTITLE:
    // Buf = static_cast<wchar_t *>(Param);
    // Param = Buf.c_str();
    break;

  default:
    // for other commands, OEM/ANSI conversion to be verified
    assert(false);
    break;
  }

  TFarEnvGuard Guard;
  return FStartupInfo.EditorControl(Command, Param);
}
//---------------------------------------------------------------------------
TFarEditorInfo * __fastcall TCustomFarPlugin::EditorInfo()
{
  TFarEditorInfo * Result;
  ::EditorInfo * Info = new ::EditorInfo;
  try
  {
    if (FarEditorControl(ECTL_GETINFO, Info))
    {
      Result = new TFarEditorInfo(Info);
    }
    else
    {
      delete Info;
      Result = NULL;
    }
  }
  catch (...)
  {
    delete Info;
    throw;
  }
  return Result;
}
//---------------------------------------------------------------------------
int __fastcall TCustomFarPlugin::FarVersion()
{
  if (FFarVersion == 0)
  {
    FFarVersion = FarAdvControl(ACTL_GETFARVERSION);
  }
  return FFarVersion;
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TCustomFarPlugin::FormatFarVersion(int Version)
{
  return FORMAT(L"%d.%d.%d", (Version >> 8) & 0xFF, Version & 0xFF, Version >> 16);
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TCustomFarPlugin::TemporaryDir()
{
  UnicodeString Result;
  Result.SetLength(MAX_PATH);
  TFarEnvGuard Guard;
  FFarStandardFunctions.MkTemp(const_cast<wchar_t *>(Result.c_str()), Result.Length(), NULL);
  PackStr(Result);
  return Result;
}
//---------------------------------------------------------------------------
int __fastcall TCustomFarPlugin::InputRecordToKey(const INPUT_RECORD * Rec)
{
  int Result;
  if (FFarStandardFunctions.FarInputRecordToKey != NULL)
  {
    TFarEnvGuard Guard;
    Result = FFarStandardFunctions.FarInputRecordToKey(Rec);
  }
  else
  {
    Result = 0;
  }
  return Result;
}
//---------------------------------------------------------------------------
#ifdef NETBOX_DEBUG
void TCustomFarPlugin::RunTests()
{
  DEBUG_PRINTF(L"begin");
  {
    TFileMasks m(L"*.txt;*.log");
    bool res = m.Matches(L"test.exe");
    DEBUG_PRINTF(L"res = %d", res);
  }
  {
    random_ref();
    random_unref();
  }
  DEBUG_PRINTF(L"end");
}
#endif
//---------------------------------------------------------------------------
unsigned int TCustomFarFileSystem::FInstances = 0;
//---------------------------------------------------------------------------
/* __fastcall */ TCustomFarFileSystem::TCustomFarFileSystem(TCustomFarPlugin * APlugin) :
    TObject(),
    FPlugin(APlugin)
{
};

void __fastcall TCustomFarFileSystem::Init()
{
  FCriticalSection = new TCriticalSection;
  FPanelInfo[0] = NULL;
  FPanelInfo[1] = NULL;
  FClosed = false;

  memset(&FOpenPluginInfo, 0, sizeof(FOpenPluginInfo));
  ClearOpenPluginInfo(FOpenPluginInfo);
  FInstances++;
  // DEBUG_PRINTF(L"FInstances = %d", FInstances);
}

//---------------------------------------------------------------------------
/* __fastcall */ TCustomFarFileSystem::~TCustomFarFileSystem()
{
  FInstances--;
  // DEBUG_PRINTF(L"FInstances = %d", FInstances);
  ResetCachedInfo();
  ClearOpenPluginInfo(FOpenPluginInfo);
  delete FCriticalSection;
}
//---------------------------------------------------------------------------
void __fastcall TCustomFarFileSystem::HandleException(Exception * E, int OpMode)
{
  DEBUG_PRINTF(L"before FPlugin->HandleException");
  FPlugin->HandleException(E, OpMode);
}
//---------------------------------------------------------------------------
void __fastcall TCustomFarFileSystem::Close()
{
  FClosed = true;
}
//---------------------------------------------------------------------------
void __fastcall TCustomFarFileSystem::InvalidateOpenPluginInfo()
{
  FOpenPluginInfoValid = false;
}
//---------------------------------------------------------------------------
void __fastcall TCustomFarFileSystem::ClearOpenPluginInfo(OpenPluginInfo & Info)
{
  if (Info.StructSize)
  {
    delete[] Info.HostFile;
    delete[] Info.CurDir;
    delete[] Info.Format;
    delete[] Info.PanelTitle;
    assert(!Info.InfoLines);
    assert(!Info.InfoLinesNumber);
    assert(!Info.DescrFiles);
    assert(!Info.DescrFilesNumber);
    assert(Info.PanelModesNumber == 0 || Info.PanelModesNumber == PANEL_MODES_COUNT);
    for (int Index = 0; Index < Info.PanelModesNumber; Index++)
    {
      assert(Info.PanelModesArray);
      TFarPanelModes::ClearPanelMode(
        const_cast<PanelMode &>(Info.PanelModesArray[Index]));
    }
    delete[] Info.PanelModesArray;
    if (Info.KeyBar)
    {
      TFarKeyBarTitles::ClearKeyBarTitles(const_cast<KeyBarTitles &>(*Info.KeyBar));
      delete Info.KeyBar;
    }
    delete[] Info.ShortcutData;
  }
  memset(&Info, 0, sizeof(Info));
  Info.StructSize = sizeof(Info);
  InvalidateOpenPluginInfo();
}
//---------------------------------------------------------------------------
void __fastcall TCustomFarFileSystem::GetOpenPluginInfo(struct OpenPluginInfo * Info)
{
  ResetCachedInfo();
  if (FClosed)
  {
    // FAR WORKAROUND
    // if plugin is closed from ProcessEvent(FE_IDLE), is does not close,
    // so we close it here on the very next opportunity
    ClosePlugin();
  }
  else
  {
    if (!FOpenPluginInfoValid)
    {
      ClearOpenPluginInfo(FOpenPluginInfo);
      UnicodeString HostFile, CurDir, Format, PanelTitle, ShortcutData;
      bool StartSortOrder;
      TFarPanelModes * PanelModes = NULL;
      TFarKeyBarTitles * KeyBarTitles = NULL;
      // try
      {
        BOOST_SCOPE_EXIT ( (&PanelModes) (&KeyBarTitles) )
        {
          delete PanelModes;
          delete KeyBarTitles;
        } BOOST_SCOPE_EXIT_END
        PanelModes = new TFarPanelModes();
        KeyBarTitles = new TFarKeyBarTitles();
        StartSortOrder = false;

        GetOpenPluginInfoEx(FOpenPluginInfo.Flags, HostFile, CurDir, Format,
          PanelTitle, PanelModes, FOpenPluginInfo.StartPanelMode,
          FOpenPluginInfo.StartSortMode, StartSortOrder, KeyBarTitles, ShortcutData);

        FOpenPluginInfo.HostFile = TCustomFarPlugin::DuplicateStr(HostFile);
        // FOpenPluginInfo.CurDir = TCustomFarPlugin::DuplicateStr(::StringReplace(CurDir, L"/", L"\\", TReplaceFlags::Init(rfReplaceAll)));
        FOpenPluginInfo.CurDir = TCustomFarPlugin::DuplicateStr(::StringReplace(CurDir, L"\\", L"/", TReplaceFlags::Init(rfReplaceAll)));
        FOpenPluginInfo.Format = TCustomFarPlugin::DuplicateStr(Format);
        FOpenPluginInfo.PanelTitle = TCustomFarPlugin::DuplicateStr(PanelTitle);
        PanelModes->FillOpenPluginInfo(&FOpenPluginInfo);
        FOpenPluginInfo.StartSortOrder = StartSortOrder;
        KeyBarTitles->FillOpenPluginInfo(&FOpenPluginInfo);
        FOpenPluginInfo.ShortcutData = TCustomFarPlugin::DuplicateStr(ShortcutData);
      }
#ifndef _MSC_VER
      __finally
      {
        delete PanelModes;
        delete KeyBarTitles;
      }
#endif

      FOpenPluginInfoValid = true;
    }

    memmove(Info, &FOpenPluginInfo, sizeof(FOpenPluginInfo));
  }
}
//---------------------------------------------------------------------------
int __fastcall TCustomFarFileSystem::GetFindData(
  struct PluginPanelItem ** PanelItem, int * ItemsNumber, int OpMode)
{
  // DEBUG_PRINTF(L"begin");
  ResetCachedInfo();
  TObjectList * PanelItems = new TObjectList();
  bool Result;
  // try
  {
    BOOST_SCOPE_EXIT ( (&PanelItems) )
    {
      delete PanelItems;
    } BOOST_SCOPE_EXIT_END
    Result = !FClosed && GetFindDataEx(PanelItems, OpMode);
    // DEBUG_PRINTF(L"Result = %d, PanelItems->GetCount = %d", Result, PanelItems->GetCount());
    if (Result && PanelItems->GetCount())
    {
      *PanelItem = new PluginPanelItem[PanelItems->GetCount()];
      memset(*PanelItem, 0, PanelItems->GetCount() * sizeof(PluginPanelItem));
      *ItemsNumber = static_cast<int>(PanelItems->GetCount());
      for (int Index = 0; Index < PanelItems->GetCount(); Index++)
      {
        static_cast<TCustomFarPanelItem *>(PanelItems->GetItem(Index))->FillPanelItem(
          &((*PanelItem)[Index]));
      }
    }
    else
    {
      *PanelItem = NULL;
      *ItemsNumber = 0;
    }
  }
#ifndef _MSC_VER
  __finally
  {
    delete PanelItems;
  }
#endif
  // DEBUG_PRINTF(L"end: Result = %d", Result);
  return Result;
}
//---------------------------------------------------------------------------
void __fastcall TCustomFarFileSystem::FreeFindData(
  struct PluginPanelItem * PanelItem, int ItemsNumber)
{
  ResetCachedInfo();
  if (PanelItem)
  {
    assert(ItemsNumber > 0);
    for (int Index = 0; Index < ItemsNumber; Index++)
    {
      delete[] PanelItem[Index].FindData.lpwszFileName;
      delete[] PanelItem[Index].Description;
      delete[] PanelItem[Index].Owner;
      for (int CustomIndex = 0; CustomIndex < PanelItem[Index].CustomColumnNumber; CustomIndex++)
      {
        delete[] PanelItem[Index].CustomColumnData[CustomIndex];
      }
      delete[] PanelItem[Index].CustomColumnData;
    }
    delete[] PanelItem;
  }
}
//---------------------------------------------------------------------------
int __fastcall TCustomFarFileSystem::ProcessHostFile(struct PluginPanelItem * PanelItem,
  int ItemsNumber, int OpMode)
{
  ResetCachedInfo();
  TObjectList * PanelItems = CreatePanelItemList(PanelItem, ItemsNumber);
  bool Result;
  // try
  {
    BOOST_SCOPE_EXIT ( (&PanelItems) )
    {
      delete PanelItems;
    } BOOST_SCOPE_EXIT_END
    Result = ProcessHostFileEx(PanelItems, OpMode);
  }
#ifndef _MSC_VER
  __finally
  {
    delete PanelItems;
  }
#endif

  return Result;
}
//---------------------------------------------------------------------------
int __fastcall TCustomFarFileSystem::ProcessKey(int Key, unsigned int ControlState)
{
  ResetCachedInfo();
  return ProcessKeyEx(Key, ControlState);
}
//---------------------------------------------------------------------------
int __fastcall TCustomFarFileSystem::ProcessEvent(int Event, void * Param)
{
  ResetCachedInfo();
  return ProcessEventEx(Event, Param);
}
//---------------------------------------------------------------------------
int __fastcall TCustomFarFileSystem::SetDirectory(const wchar_t * Dir, int OpMode)
{
  ResetCachedInfo();
  InvalidateOpenPluginInfo();
  int Result = SetDirectoryEx(Dir, OpMode);
  InvalidateOpenPluginInfo();
  return Result;
}
//---------------------------------------------------------------------------
int __fastcall TCustomFarFileSystem::MakeDirectory(const wchar_t ** Name, int OpMode)
{
  ResetCachedInfo();
  UnicodeString NameStr = *Name;
  int Result = 0;
  // try
  {
    BOOST_SCOPE_EXIT ( (&NameStr) (&Name) )
    {
      if (NameStr != *Name)
      {
        // wcscpy_s(*Name, NameStr.size(), NameStr.c_str());
        *Name = TCustomFarPlugin::DuplicateStr(NameStr, true);
      }
    } BOOST_SCOPE_EXIT_END
    Result = MakeDirectoryEx(NameStr, OpMode);
  }
#ifndef _MSC_VER
  __finally
  {
    StrToFar(NameStr);
    if (NameStr != Name)
    {
      strcpy(Name, NameStr.c_str());
    }
  }
#endif
  return Result;
}
//---------------------------------------------------------------------------
int __fastcall TCustomFarFileSystem::DeleteFiles(struct PluginPanelItem * PanelItem,
    int ItemsNumber, int OpMode)
{
  ResetCachedInfo();
  TObjectList * PanelItems = CreatePanelItemList(PanelItem, ItemsNumber);
  bool Result;
  // try
  {
    BOOST_SCOPE_EXIT ( (&PanelItems) )
    {
      delete PanelItems;
    } BOOST_SCOPE_EXIT_END
    Result = DeleteFilesEx(PanelItems, OpMode);
  }
#ifndef _MSC_VER
  __finally
  {
    delete PanelItems;
  }
#endif

  return Result;
}
//---------------------------------------------------------------------------
int __fastcall TCustomFarFileSystem::GetFiles(struct PluginPanelItem * PanelItem,
  int ItemsNumber, int Move, const wchar_t ** DestPath, int OpMode)
{
  ResetCachedInfo();
  TObjectList * PanelItems = CreatePanelItemList(PanelItem, ItemsNumber);
  int Result;
  UnicodeString DestPathStr = *DestPath;
  // try
  {
    BOOST_SCOPE_EXIT ( (&DestPathStr) (&DestPath) (&PanelItems) )
    {
      if (DestPathStr != *DestPath)
      {
        // wcscpy_s(*DestPath, DestPathStr.size(), DestPathStr.c_str());
        *DestPath = TCustomFarPlugin::DuplicateStr(DestPathStr, true);
      }
      delete PanelItems;
    } BOOST_SCOPE_EXIT_END
    Result = GetFilesEx(PanelItems, Move > 0, DestPathStr, OpMode);
  }
#ifndef _MSC_VER
  __finally
  {
    StrToFar(DestPathStr);
    if (DestPathStr != DestPath)
    {
      strcpy(DestPath, DestPathStr.c_str());
    }
    delete PanelItems;
  }
#endif

  return Result;
}
//---------------------------------------------------------------------------
int __fastcall TCustomFarFileSystem::PutFiles(struct PluginPanelItem * PanelItem,
  int ItemsNumber, int Move, int OpMode)
{
  ResetCachedInfo();
  TObjectList * PanelItems = CreatePanelItemList(PanelItem, ItemsNumber);
  int Result;
  // try
  {
    BOOST_SCOPE_EXIT ( (&PanelItems) )
    {
      delete PanelItems;
    } BOOST_SCOPE_EXIT_END
    Result = PutFilesEx(PanelItems, Move > 0, OpMode);
  }
#ifndef _MSC_VER
  __finally
  {
    delete PanelItems;
  }
#endif

  return Result;
}
//---------------------------------------------------------------------------
void __fastcall TCustomFarFileSystem::ResetCachedInfo()
{
  if (FPanelInfo[0])
  {
    SAFE_DESTROY(FPanelInfo[false]);
  }
  if (FPanelInfo[1])
  {
    SAFE_DESTROY(FPanelInfo[true]);
  }
}
//---------------------------------------------------------------------------
TFarPanelInfo * __fastcall TCustomFarFileSystem::GetPanelInfo(int Another)
{
  // DEBUG_PRINTF(L"Another = %d", Another);
  bool another = Another != 0;
  if (FPanelInfo[another] == NULL)
  {
    PanelInfo * Info = new PanelInfo;
    // Info->StructSize = sizeof(PanelInfo);
    bool res = (FPlugin->FarControl(FCTL_GETPANELINFO, 0, reinterpret_cast<intptr_t>(Info),
                                    !another ? PANEL_ACTIVE : PANEL_PASSIVE) > 0);
    if (!res)
    {
      memset(Info, 0, sizeof(*Info));
      assert(false);
    }
    // DEBUG_PRINTF(L"Info = %x", Info);
    FPanelInfo[another] = new TFarPanelInfo(Info, !another ? this : NULL);
  }
  return FPanelInfo[another];
}
//---------------------------------------------------------------------------
DWORD __fastcall TCustomFarFileSystem::FarControl(int Command, int Param1, intptr_t Param2)
{
  return FPlugin->FarControl(Command, Param1, Param2, this);
}
//---------------------------------------------------------------------------
DWORD __fastcall TCustomFarFileSystem::FarControl(int Command, int Param1, intptr_t Param2, HANDLE Plugin)
{
  return FPlugin->FarControl(Command, Param1, Param2, Plugin);
}
//---------------------------------------------------------------------------
bool __fastcall TCustomFarFileSystem::UpdatePanel(bool ClearSelection, bool Another)
{
  unsigned int PrevInstances = FInstances;
  InvalidateOpenPluginInfo();
  FPlugin->FarControl(FCTL_UPDATEPANEL, !ClearSelection, NULL, Another ? PANEL_PASSIVE : PANEL_ACTIVE);
  return (FInstances >= PrevInstances);
}
//---------------------------------------------------------------------------
void __fastcall TCustomFarFileSystem::RedrawPanel(bool Another)
{
  FPlugin->FarControl(FCTL_REDRAWPANEL, 0, reinterpret_cast<intptr_t>(static_cast<void *>(0)), Another ? PANEL_PASSIVE : PANEL_ACTIVE);
}
//---------------------------------------------------------------------------
void __fastcall TCustomFarFileSystem::ClosePlugin()
{
  FClosed = true;
  FarControl(FCTL_CLOSEPLUGIN, 0, NULL);
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TCustomFarFileSystem::GetMsg(int MsgId)
{
  return FPlugin->GetMsg(MsgId);
}
//---------------------------------------------------------------------------
TCustomFarFileSystem * __fastcall TCustomFarFileSystem::GetOppositeFileSystem()
{
  return FPlugin->GetPanelFileSystem(true, this);
}
//---------------------------------------------------------------------------
bool __fastcall TCustomFarFileSystem::IsActiveFileSystem()
{
  // Cannot use PanelInfo::Focus as it occasionally does not work from editor;
  return (this == FPlugin->GetPanelFileSystem());
}
//---------------------------------------------------------------------------
bool __fastcall TCustomFarFileSystem::IsLeft()
{
  DEBUG_PRINTF(L"IsLeft");
  return (GetPanelInfo(0)->GetBounds().Left <= 0);
}
//---------------------------------------------------------------------------
bool __fastcall TCustomFarFileSystem::IsRight()
{
  return !IsLeft();
}
//---------------------------------------------------------------------------
bool __fastcall TCustomFarFileSystem::ProcessHostFileEx(TObjectList * /*PanelItems*/, int /*OpMode*/)
{
  return false;
}
//---------------------------------------------------------------------------
bool __fastcall TCustomFarFileSystem::ProcessKeyEx(int /*Key*/, unsigned int /*ControlState*/)
{
  return false;
}
//---------------------------------------------------------------------------
bool __fastcall TCustomFarFileSystem::ProcessEventEx(int /*Event*/, void * /*Param*/)
{
  return false;
}
//---------------------------------------------------------------------------
bool __fastcall TCustomFarFileSystem::SetDirectoryEx(const UnicodeString /*Dir*/, int /*OpMode*/)
{
  return false;
}
//---------------------------------------------------------------------------
int __fastcall TCustomFarFileSystem::MakeDirectoryEx(UnicodeString & /*Name*/, int /*OpMode*/)
{
  return -1;
}
//---------------------------------------------------------------------------
bool __fastcall TCustomFarFileSystem::DeleteFilesEx(TObjectList * /*PanelItems*/, int /*OpMode*/)
{
  return false;
}
//---------------------------------------------------------------------------
int __fastcall TCustomFarFileSystem::GetFilesEx(TObjectList * /*PanelItems*/, bool /*Move*/,
  UnicodeString & /*DestPath*/, int /*OpMode*/)
{
  return 0;
}
//---------------------------------------------------------------------------
int __fastcall TCustomFarFileSystem::PutFilesEx(TObjectList * /*PanelItems*/,
  bool /*Move*/, int /*OpMode*/)
{
  return 0;
}
//---------------------------------------------------------------------------
TObjectList * __fastcall TCustomFarFileSystem::CreatePanelItemList(
  struct PluginPanelItem * PanelItem, int ItemsNumber)
{
  // DEBUG_PRINTF(L"ItemsNumber = %d", ItemsNumber);
  TObjectList * PanelItems = new TObjectList();
  PanelItems->SetOwnsObjects(false);
  try
  {
    for (int Index = 0; Index < ItemsNumber; Index++)
    {
      PanelItems->Add(new TFarPanelItem(&PanelItem[Index]));
    }
  }
  catch(...)
  {
    delete PanelItems;
    throw;
  }
  return PanelItems;
}
//---------------------------------------------------------------------------
/* __fastcall */ TFarPanelModes::TFarPanelModes() : TObject()
{
  memset(&FPanelModes, 0, sizeof(FPanelModes));
  FReferenced = false;
}
//---------------------------------------------------------------------------
/* __fastcall */ TFarPanelModes::~TFarPanelModes()
{
  if (!FReferenced)
  {
    for (int Index = 0; Index < LENOF(FPanelModes); Index++)
    {
      ClearPanelMode(FPanelModes[Index]);
    }
  }
}
//---------------------------------------------------------------------------
void __fastcall TFarPanelModes::SetPanelMode(size_t Mode, const UnicodeString ColumnTypes,
  const UnicodeString ColumnWidths, TStrings * ColumnTitles,
  bool FullScreen, bool DetailedStatus, bool AlignExtensions,
  bool CaseConversion, const UnicodeString StatusColumnTypes,
  const UnicodeString StatusColumnWidths)
{
  int ColumnTypesCount = !ColumnTypes.IsEmpty() ? CommaCount(ColumnTypes) + 1 : 0;
  assert(Mode != NPOS && Mode < LENOF(FPanelModes));
  assert(!ColumnTitles || (ColumnTitles->GetCount() == ColumnTypesCount));

  ClearPanelMode(FPanelModes[Mode]);
  static wchar_t * Titles[PANEL_MODES_COUNT];
  FPanelModes[Mode].ColumnTypes = TCustomFarPlugin::DuplicateStr(ColumnTypes);
  FPanelModes[Mode].ColumnWidths = TCustomFarPlugin::DuplicateStr(ColumnWidths);
  if (ColumnTitles)
  {
    FPanelModes[Mode].ColumnTitles = new wchar_t *[ColumnTypesCount];
    for (int Index = 0; Index < ColumnTypesCount; Index++)
    {
      // FPanelModes[Mode].ColumnTitles[Index] =
      //    TCustomFarPlugin::DuplicateStr(ColumnTitles->GetStrings(Index));
      Titles[Index] = TCustomFarPlugin::DuplicateStr(ColumnTitles->GetStrings(Index));
    }
    FPanelModes[Mode].ColumnTitles = Titles;
  }
  else
  {
    FPanelModes[Mode].ColumnTitles = NULL;
  }
  FPanelModes[Mode].FullScreen = FullScreen;
  FPanelModes[Mode].DetailedStatus = DetailedStatus;
  FPanelModes[Mode].AlignExtensions = AlignExtensions;
  FPanelModes[Mode].CaseConversion = CaseConversion;

  FPanelModes[Mode].StatusColumnTypes = TCustomFarPlugin::DuplicateStr(StatusColumnTypes);
  FPanelModes[Mode].StatusColumnWidths = TCustomFarPlugin::DuplicateStr(StatusColumnWidths);
}
//---------------------------------------------------------------------------
void __fastcall TFarPanelModes::ClearPanelMode(PanelMode & Mode)
{
  if (Mode.ColumnTypes)
  {
    int ColumnTypesCount = Mode.ColumnTypes ?
                              CommaCount(UnicodeString(Mode.ColumnTypes)) + 1 : 0;

    delete[] Mode.ColumnTypes;
    delete[] Mode.ColumnWidths;
    if (Mode.ColumnTitles)
    {
      for (int Index = 0; Index < ColumnTypesCount; Index++)
      {
        // delete[] Mode.ColumnTitles[Index];
      }
      // delete[] Mode.ColumnTitles;
    }
    delete[] Mode.StatusColumnTypes;
    delete[] Mode.StatusColumnWidths;
    memset(&Mode, 0, sizeof(Mode));
  }
}
//---------------------------------------------------------------------------
void __fastcall TFarPanelModes::FillOpenPluginInfo(struct OpenPluginInfo * Info)
{
  assert(Info);
  Info->PanelModesNumber = LENOF(FPanelModes);
  PanelMode * PanelModesArray = new PanelMode[LENOF(FPanelModes)];
  memmove(PanelModesArray, &FPanelModes, sizeof(FPanelModes));
  Info->PanelModesArray = PanelModesArray;
  FReferenced = true;
}
//---------------------------------------------------------------------------
int __fastcall TFarPanelModes::CommaCount(const UnicodeString ColumnTypes)
{
  int Count = 0;
  for (int Index = 1; Index <= ColumnTypes.Length(); Index++)
  {
    if (ColumnTypes[Index] == ',')
    {
      Count++;
    }
  }
  return Count;
}
//---------------------------------------------------------------------------
/* __fastcall */ TFarKeyBarTitles::TFarKeyBarTitles()
{
  memset(&FKeyBarTitles, 0, sizeof(FKeyBarTitles));
  FReferenced = false;
}
//---------------------------------------------------------------------------
/* __fastcall */ TFarKeyBarTitles::~TFarKeyBarTitles()
{
  if (!FReferenced)
  {
    ClearKeyBarTitles(FKeyBarTitles);
  }
}
//---------------------------------------------------------------------------
void __fastcall TFarKeyBarTitles::ClearFileKeyBarTitles()
{
  ClearKeyBarTitle(fsNone, 3, 8);
  ClearKeyBarTitle(fsCtrl, 4, 11);
  ClearKeyBarTitle(fsAlt, 3, 7);
  ClearKeyBarTitle(fsShift, 1, 8);
  ClearKeyBarTitle(fsCtrlShift, 3, 4);
}
//---------------------------------------------------------------------------
void __fastcall TFarKeyBarTitles::ClearKeyBarTitle(TFarShiftStatus ShiftStatus,
  int FunctionKeyStart, int FunctionKeyEnd)
{
  if (!FunctionKeyEnd)
  {
    FunctionKeyEnd = FunctionKeyStart;
  }
  for (int Index = FunctionKeyStart; Index <= FunctionKeyEnd; Index++)
  {
    SetKeyBarTitle(ShiftStatus, Index, L"");
  }
}
//---------------------------------------------------------------------------
void __fastcall TFarKeyBarTitles::SetKeyBarTitle(TFarShiftStatus ShiftStatus,
  int FunctionKey, const UnicodeString Title)
{
  assert(FunctionKey >= 1 && FunctionKey <= LENOF(FKeyBarTitles.Titles));
  wchar_t ** Titles = NULL;
  switch (ShiftStatus)
  {
  case fsNone:
    Titles = FKeyBarTitles.Titles;
    break;
  case fsCtrl:
    Titles = FKeyBarTitles.CtrlTitles;
    break;
  case fsAlt:
    Titles = FKeyBarTitles.AltTitles;
    break;
  case fsShift:
    Titles = FKeyBarTitles.ShiftTitles;
    break;
  case fsCtrlShift:
    Titles = FKeyBarTitles.CtrlShiftTitles;
    break;
  case fsAltShift:
    Titles = FKeyBarTitles.AltShiftTitles;
    break;
  case fsCtrlAlt:
    Titles = FKeyBarTitles.CtrlAltTitles;
    break;
  default:
    assert(false);
  }
  if (Titles[FunctionKey-1])
  {
    delete[] Titles[FunctionKey-1];
  }
  Titles[FunctionKey-1] = TCustomFarPlugin::DuplicateStr(Title, true);
}
//---------------------------------------------------------------------------
void __fastcall TFarKeyBarTitles::ClearKeyBarTitles(KeyBarTitles & Titles)
{
  for (int Index = 0; Index < LENOF(Titles.Titles); Index++)
  {
    delete[] Titles.Titles[Index];
    delete[] Titles.CtrlTitles[Index];
    delete[] Titles.AltTitles[Index];
    delete[] Titles.ShiftTitles[Index];
    delete[] Titles.CtrlShiftTitles[Index];
    delete[] Titles.AltShiftTitles[Index];
    delete[] Titles.CtrlAltTitles[Index];
  }
}
//---------------------------------------------------------------------------
void __fastcall TFarKeyBarTitles::FillOpenPluginInfo(struct OpenPluginInfo * Info)
{
  assert(Info);
  KeyBarTitles * KeyBar = new KeyBarTitles;
  Info->KeyBar = KeyBar;
  memmove(KeyBar, &FKeyBarTitles, sizeof(FKeyBarTitles));
  FReferenced = true;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
UnicodeString __fastcall TCustomFarPanelItem::GetCustomColumnData(int /*Column*/)
{
  assert(false);
  return L"";
}
//---------------------------------------------------------------------------
void __fastcall TCustomFarPanelItem::FillPanelItem(struct PluginPanelItem * PanelItem)
{
  // DEBUG_PRINTF(L"begin");
  assert(PanelItem);

  UnicodeString FileName;
  __int64 Size = 0;
  TDateTime LastWriteTime;
  TDateTime LastAccess;
  UnicodeString Description;
  UnicodeString Owner;

  void * UserData = reinterpret_cast<void *>(PanelItem->UserData);
  GetData(PanelItem->Flags, FileName, Size, PanelItem->FindData.dwFileAttributes,
          LastWriteTime, LastAccess, PanelItem->NumberOfLinks, Description, Owner,
          static_cast<void *>(UserData), PanelItem->CustomColumnNumber);
  PanelItem->UserData = reinterpret_cast<uintptr_t>(UserData);
  // DEBUG_PRINTF(L"LastWriteTime = %f, LastAccess = %f", LastWriteTime, LastAccess);
  FILETIME FileTime = DateTimeToFileTime(LastWriteTime, dstmWin);
  FILETIME FileTimeA = DateTimeToFileTime(LastAccess, dstmWin);
  PanelItem->FindData.ftCreationTime = FileTime;
  PanelItem->FindData.ftLastAccessTime = FileTimeA;
  PanelItem->FindData.ftLastWriteTime = FileTime;
  PanelItem->FindData.nFileSize = Size;
  // PanelItem->PackSize = (long int)Size;

  // ASCOPY(PanelItem->FindData.lpwszFileName, FileName);
  PanelItem->FindData.lpwszFileName = TCustomFarPlugin::DuplicateStr(FileName);
  // DEBUG_PRINTF(L"PanelItem->FindData.lpwszFileName = %s", PanelItem->FindData.lpwszFileName);
  PanelItem->Description = TCustomFarPlugin::DuplicateStr(Description);
  PanelItem->Owner = TCustomFarPlugin::DuplicateStr(Owner);
  // PanelItem->CustomColumnData = new wchar_t *[PanelItem->CustomColumnNumber];
  wchar_t ** CustomColumnData = new wchar_t *[PanelItem->CustomColumnNumber];
  for (int Index = 0; Index < PanelItem->CustomColumnNumber; Index++)
  {
    CustomColumnData[Index] =
      TCustomFarPlugin::DuplicateStr(GetCustomColumnData(Index));
  }
  PanelItem->CustomColumnData = CustomColumnData;
  // DEBUG_PRINTF(L"end");
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
/* __fastcall */ TFarPanelItem::TFarPanelItem(PluginPanelItem * APanelItem):
  TCustomFarPanelItem()
{
  assert(APanelItem);
  FPanelItem = APanelItem;
}
/* __fastcall */ TFarPanelItem::~TFarPanelItem()
{
  delete FPanelItem;
  FPanelItem = NULL;
}

//---------------------------------------------------------------------------
void __fastcall TFarPanelItem::GetData(
  unsigned long & /*Flags*/, UnicodeString & /*FileName*/, __int64 & /*Size*/,
  unsigned long & /*FileAttributes*/,
  TDateTime & /*LastWriteTime*/, TDateTime & /*LastAccess*/,
  unsigned long & /*NumberOfLinks*/, UnicodeString & /*Description*/,
  UnicodeString & /*Owner*/, void *& /*UserData*/, int & /*CustomColumnNumber*/)
{
  assert(false);
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TFarPanelItem::GetCustomColumnData(int /*Column*/)
{
  assert(false);
  return L"";
}
//---------------------------------------------------------------------------
unsigned long __fastcall TFarPanelItem::GetFlags()
{
  return FPanelItem->Flags;
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TFarPanelItem::GetFileName()
{
  UnicodeString Result = FPanelItem->FindData.lpwszFileName;
  return Result;
}
//---------------------------------------------------------------------------
void * __fastcall TFarPanelItem::GetUserData()
{
  return reinterpret_cast<void *>(FPanelItem->UserData);
}
//---------------------------------------------------------------------------
bool __fastcall TFarPanelItem::GetSelected()
{
  return (FPanelItem->Flags & PPIF_SELECTED) != 0;
}
//---------------------------------------------------------------------------
void __fastcall TFarPanelItem::SetSelected(bool value)
{
  if (value)
  {
    FPanelItem->Flags |= PPIF_SELECTED;
  }
  else
  {
    FPanelItem->Flags &= ~PPIF_SELECTED;
  }
}
//---------------------------------------------------------------------------
unsigned long __fastcall TFarPanelItem::GetFileAttributes()
{
  return FPanelItem->FindData.dwFileAttributes;
}
//---------------------------------------------------------------------------
bool __fastcall TFarPanelItem::GetIsParentDirectory()
{
  return (GetFileName() == PARENTDIRECTORY);
}
//---------------------------------------------------------------------------
bool __fastcall TFarPanelItem::GetIsFile()
{
  return (GetFileAttributes() & FILE_ATTRIBUTE_DIRECTORY) == 0;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
/* __fastcall */ THintPanelItem::THintPanelItem(const UnicodeString AHint) :
  TCustomFarPanelItem()
{
  FHint = AHint;
}
//---------------------------------------------------------------------------
void __fastcall THintPanelItem::GetData(
  unsigned long & /*Flags*/, UnicodeString & FileName, __int64 & /*Size*/,
  unsigned long & /*FileAttributes*/,
  TDateTime & /*LastWriteTime*/, TDateTime & /*LastAccess*/,
  unsigned long & /*NumberOfLinks*/, UnicodeString & /*Description*/,
  UnicodeString & /*Owner*/, void *& /*UserData*/, int & /*CustomColumnNumber*/)
{
  FileName = FHint;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
/* __fastcall */ TFarPanelInfo::TFarPanelInfo(PanelInfo * APanelInfo, TCustomFarFileSystem * AOwner):
  TObject(),
  FPanelInfo(NULL),
  FItems(NULL),
  FOwner(NULL)
{
  // if (!APanelInfo) throw ExtException(L"");
  assert(APanelInfo);
  FPanelInfo = APanelInfo;
  FOwner = AOwner;
  FItems = NULL;
}
//---------------------------------------------------------------------------
/* __fastcall */ TFarPanelInfo::~TFarPanelInfo()
{
  delete FPanelInfo;
  delete FItems;
}
//---------------------------------------------------------------------------
int __fastcall TFarPanelInfo::GetItemCount()
{
  return FPanelInfo->ItemsNumber;
}
//---------------------------------------------------------------------------
TRect __fastcall TFarPanelInfo::GetBounds()
{
  RECT rect = FPanelInfo->PanelRect;
  return TRect(rect.left, rect.top, rect.right, rect.bottom);
}
//---------------------------------------------------------------------------
int __fastcall TFarPanelInfo::GetSelectedCount()
{
  int Count = FPanelInfo->SelectedItemsNumber;

  if (Count == 1)
  {
    size_t size = FOwner->FarControl(FCTL_GETSELECTEDPANELITEM, 0, NULL);
    // DEBUG_PRINTF(L"size1 = %d, sizeof(PluginPanelItem) = %d", size, sizeof(PluginPanelItem));
    PluginPanelItem * ppi = (PluginPanelItem *)malloc(size);
    memset(ppi, 0, size);
    FOwner->FarControl(FCTL_GETSELECTEDPANELITEM, 0, reinterpret_cast<intptr_t>(ppi));
    if ((ppi->Flags & PPIF_SELECTED) == 0)
    {
      // DEBUG_PRINTF(L"ppi->Flags = %x", ppi->Flags);
      Count = 0;
    }
    free(ppi);
  }

  return Count;
}
//---------------------------------------------------------------------------
TObjectList * __fastcall TFarPanelInfo::GetItems()
{
  if (!FItems)
  {
    FItems = new TObjectList();
  }
  // DEBUG_PRINTF(L"FPanelInfo->ItemsNumber = %d", FPanelInfo->ItemsNumber);
  for (int Index = 0; Index < FPanelInfo->ItemsNumber; Index++)
  {
    // DEBUG_PRINTF(L"Index = %d", Index);
    // TODO: move to common function
    size_t size = FOwner->FarControl(FCTL_GETPANELITEM, Index, NULL);
    PluginPanelItem * ppi = (PluginPanelItem *)malloc(size);
    memset(ppi, 0, size);
    FOwner->FarControl(FCTL_GETPANELITEM, Index, reinterpret_cast<intptr_t>(ppi));
    // DEBUG_PRINTF(L"ppi.FileName = %s", ppi->FindData.lpwszFileName);
    FItems->Add(static_cast<TObject *>(new TFarPanelItem(ppi)));
  }
  return FItems;
}
//---------------------------------------------------------------------------
TFarPanelItem * __fastcall TFarPanelInfo::FindFileName(const UnicodeString FileName)
{
  TObjectList * AItems = GetItems();
  TFarPanelItem * PanelItem;
  for (int Index = 0; Index < AItems->GetCount(); Index++)
  {
    PanelItem = static_cast<TFarPanelItem *>(AItems->GetItem(Index));
    if (PanelItem->GetFileName() == FileName)
    {
      return PanelItem;
    }
  }
  return NULL;
}
//---------------------------------------------------------------------------
TFarPanelItem * __fastcall TFarPanelInfo::FindUserData(void * UserData)
{
  TObjectList * AItems = GetItems();
  TFarPanelItem * PanelItem;
  for (int Index = 0; Index < AItems->GetCount(); Index++)
  {
    PanelItem = static_cast<TFarPanelItem *>(AItems->GetItem(Index));
    if (PanelItem->GetUserData() == UserData)
    {
      return PanelItem;
    }
  }
  return NULL;
}
//---------------------------------------------------------------------------
void __fastcall TFarPanelInfo::ApplySelection()
{
  // for "another panel info", there's no owner
  assert(FOwner != NULL);
  FOwner->FarControl(FCTL_SETSELECTION, 0, reinterpret_cast<intptr_t>(FPanelInfo));
}
//---------------------------------------------------------------------------
TFarPanelItem * __fastcall TFarPanelInfo::GetFocusedItem()
{
  int Index = GetFocusedIndex();
  TObjectList * Items = GetItems();
  // DEBUG_PRINTF(L"Index = %d, Items = %x, Items->GetCount = %d", Index, Items, Items->GetCount());
  if (Items->GetCount() > 0)
  {
    assert(Index < Items->GetCount());
    return static_cast<TFarPanelItem *>(Items->GetItem(Index));
  }
  else
  {
    return NULL;
  }
}
//---------------------------------------------------------------------------
void __fastcall TFarPanelInfo::SetFocusedItem(TFarPanelItem * value)
{
  TObjectList * Items = GetItems();
  int Index = Items->IndexOf(static_cast<TObject *>(value));
  assert(Index != NPOS);
  SetFocusedIndex(Index);
  // delete Items;
}
//---------------------------------------------------------------------------
int __fastcall TFarPanelInfo::GetFocusedIndex()
{
  return FPanelInfo->CurrentItem;
}
//---------------------------------------------------------------------------
void __fastcall TFarPanelInfo::SetFocusedIndex(int value)
{
  // for "another panel info", there's no owner
  assert(FOwner != NULL);
  // DEBUG_PRINTF(L"GetFocusedIndex = %d, value = %d", GetFocusedIndex(), value);
  if (GetFocusedIndex() != value)
  {
    assert(value != NPOS && value < FPanelInfo->ItemsNumber);
    FPanelInfo->CurrentItem = value;
    PanelRedrawInfo PanelInfo;
    PanelInfo.CurrentItem = FPanelInfo->CurrentItem;
    PanelInfo.TopPanelItem = FPanelInfo->TopPanelItem;
    FOwner->FarControl(FCTL_REDRAWPANEL, 0, reinterpret_cast<intptr_t>(&PanelInfo));
  }
}
//---------------------------------------------------------------------------
TFarPanelType __fastcall TFarPanelInfo::GetType()
{
  switch (FPanelInfo->PanelType)
  {
  case PTYPE_FILEPANEL:
    return ptFile;

  case PTYPE_TREEPANEL:
    return ptTree;

  case PTYPE_QVIEWPANEL:
    return ptQuickView;

  case PTYPE_INFOPANEL:
    return ptInfo;

  default:
    assert(false);
    return ptFile;
  }
}
//---------------------------------------------------------------------------
bool __fastcall TFarPanelInfo::GetIsPlugin()
{
  return (FPanelInfo->Plugin != 0);
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TFarPanelInfo::GetCurrentDirectory()
{
  UnicodeString Result = L"";
  /*
  size_t Size = FarPlugin->GetFarStandardFunctions().GetCurrentDirectory(0, NULL);
  if (Size)
  {
      Result.SetLength(Size);
      FarPlugin->GetFarStandardFunctions().GetCurrentDirectory(Size,
          const_cast<wchar_t *>(Result.c_str()));
  }
  */
  // FarControl(FCTL_GETPANELINFO, 0, reinterpret_cast<intptr_t>(&Info), Another ? PANEL_PASSIVE : PANEL_ACTIVE);
  size_t Size = FarPlugin->FarControl(FCTL_GETPANELDIR,
                                      0,
                                      NULL,
                                      FOwner != NULL ? PANEL_ACTIVE : PANEL_PASSIVE);
  if (Size)
  {
    Result.SetLength(Size);
    FarPlugin->FarControl(FCTL_GETPANELDIR,
                          static_cast<int>(Size),
                          reinterpret_cast<intptr_t>(Result.c_str()),
                          FOwner != NULL ? PANEL_ACTIVE : PANEL_PASSIVE);
  }
  return Result.c_str();
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
/* __fastcall */ TFarMenuItems::TFarMenuItems() :
  TStringList()
{
  FItemFocused = NPOS;
}
//---------------------------------------------------------------------------
void __fastcall TFarMenuItems::Clear()
{
  FItemFocused = NPOS;
  TStringList::Clear();
}
//---------------------------------------------------------------------------
void __fastcall TFarMenuItems::Delete(int Index)
{
  if (Index == FItemFocused)
  {
    FItemFocused = NPOS;
  }
  TStringList::Delete(Index);
}
//---------------------------------------------------------------------------
void __fastcall TFarMenuItems::PutObject(int Index, TObject * AObject)
{
  TStringList::PutObject(Index, AObject);
  bool Focused = (reinterpret_cast<size_t>(AObject) & MIF_SEPARATOR) != 0;
  if ((Index == GetItemFocused()) && !Focused)
  {
    FItemFocused = NPOS;
  }
  if (Focused)
  {
    if (GetItemFocused() != NPOS)
    {
      SetFlag(GetItemFocused(), MIF_SELECTED, false);
    }
    FItemFocused = Index;
  }
}
//---------------------------------------------------------------------------
int __fastcall TFarMenuItems::Add(const UnicodeString Text, bool Visible)
{
  int Result = TStringList::Add(Text);
  if (!Visible)
  {
    SetFlag(GetCount() - 1, MIF_HIDDEN, true);
  }
  return Result;
}
//---------------------------------------------------------------------------
void __fastcall TFarMenuItems::AddSeparator(bool Visible)
{
  Add(L"");
  SetFlag(GetCount() - 1, MIF_SEPARATOR, true);
  if (!Visible)
  {
    SetFlag(GetCount() - 1, MIF_HIDDEN, true);
  }
}
//---------------------------------------------------------------------------
void __fastcall TFarMenuItems::SetItemFocused(int value)
{
  if (GetItemFocused() != value)
  {
    if (GetItemFocused() != NPOS)
    {
      SetFlag(GetItemFocused(), MIF_SELECTED, false);
    }
    FItemFocused = value;
    SetFlag(GetItemFocused(), MIF_SELECTED, true);
  }
}
//---------------------------------------------------------------------------
void __fastcall TFarMenuItems::SetFlag(size_t Index, size_t Flag, bool Value)
{
  if (GetFlag(Index, Flag) != Value)
  {
    int F = reinterpret_cast<size_t>(GetObjects(Index));
    if (Value)
    {
      F |= Flag;
    }
    else
    {
      F &= ~Flag;
    }
    PutObject(Index, reinterpret_cast<TObject *>(F));
  }
}
//---------------------------------------------------------------------------
bool __fastcall TFarMenuItems::GetFlag(size_t Index, size_t Flag)
{
  return (reinterpret_cast<size_t>(GetObjects(Index)) & Flag) > 0;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
/* __fastcall */ TFarEditorInfo::TFarEditorInfo(EditorInfo * Info) :
  FEditorInfo(Info)
{
}
//---------------------------------------------------------------------------
/* __fastcall */ TFarEditorInfo::~TFarEditorInfo()
{
  delete FEditorInfo;
}
//---------------------------------------------------------------------------
int __fastcall TFarEditorInfo::GetEditorID()
{
  return FEditorInfo->EditorID;
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TFarEditorInfo::GetFileName()
{
  UnicodeString Result = L"";
  size_t buffLen = FarPlugin->FarEditorControl(ECTL_GETFILENAME, NULL);
  if (buffLen)
  {
    Result.SetLength(buffLen + 1);
    FarPlugin->FarEditorControl(ECTL_GETFILENAME, (wchar_t *)Result.c_str());
  }
  return Result.c_str();
};
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
/* __fastcall */ TFarEnvGuard::TFarEnvGuard()
{
  assert(FarPlugin != NULL);
}
//---------------------------------------------------------------------------
/* __fastcall */ TFarEnvGuard::~TFarEnvGuard()
{
  assert(FarPlugin != NULL);
  /*
  if (!FarPlugin->GetANSIApis())
  {
      assert(!AreFileApisANSI());
      SetFileApisToANSI();
  }
  else
  {
      assert(AreFileApisANSI());
  }
  */
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
/* __fastcall */ TFarPluginEnvGuard::TFarPluginEnvGuard()
{
  assert(FarPlugin != NULL);

  // keep the assertion, but be robust, in case we are called from incorrectly
  // programmed plugin (e.g. EMenu)
  /*
  FANSIApis = AreFileApisANSI() > 0;
  assert(FANSIApis == FarPlugin->GetANSIApis());

  if (!FANSIApis)
  {
      SetFileApisToANSI();
  }
  */
}
//---------------------------------------------------------------------------
/* __fastcall */ TFarPluginEnvGuard::~TFarPluginEnvGuard()
{
  assert(FarPlugin != NULL);
}
//---------------------------------------------------------------------------
void __fastcall FarWrapText(const UnicodeString Text, TStrings * Result, size_t MaxWidth)
{
  size_t TabSize = 8;
  TStringList Lines;
  Lines.SetText(Text);
  TStringList WrappedLines;
  for (int Index = 0; Index < Lines.GetCount(); Index++)
  {
    UnicodeString WrappedLine = Lines.GetStrings(Index);
    if (!WrappedLine.IsEmpty())
    {
      WrappedLine = ::ReplaceChar(WrappedLine, '\'', '\3');
      WrappedLine = ::ReplaceChar(WrappedLine, '\"', '\4');
      // FIXME WrappedLine = ::WrapText(WrappedLine, MaxWidth);
      WrappedLine = ::ReplaceChar(WrappedLine, '\3', '\'');
      WrappedLine = ::ReplaceChar(WrappedLine, '\4', '\"');
      WrappedLines.SetText(WrappedLine);
      for (int WrappedIndex = 0; WrappedIndex < WrappedLines.GetCount(); WrappedIndex++)
      {
        UnicodeString FullLine = WrappedLines.GetStrings(WrappedIndex);
        do
        {
            // WrapText does not wrap when not possible, enforce it
            // (it also does not wrap when the line is longer than maximum only
            // because of trailing dot or similar)
            UnicodeString Line = FullLine.SubString(1, MaxWidth);
            FullLine.Delete(1, MaxWidth);

            int P = 0;
            while ((P = Line.Pos(L'\t')) > 0)
            {
              Line.Delete(P, 1);
              Line.Insert(P, ::StringOfChar(' ',
                  ((P / TabSize) + ((P % TabSize) > 0 ? 1 : 0)) * TabSize - P + 1));
              // UnicodeString s;
              // s.SetLength(((P / TabSize) + ((P % TabSize) > 0 ? 1 : 0)) * TabSize - P + 1);
              // Line.append(s.c_str(), P);
          }
          Result->Add(Line);
        }
        while (!FullLine.IsEmpty());
      }
    }
    else
    {
      Result->Add(L"");
    }
  }
}
