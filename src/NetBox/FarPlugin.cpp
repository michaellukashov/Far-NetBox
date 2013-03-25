//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include "FarPlugin.h"
#include "FarDialog.h"
#include <Common.h>
#include "Exceptions.h"
#include "TextsCore.h"
#include "FileMasks.h"
#include "RemoteFiles.h"
#include "puttyexp.h"

//---------------------------------------------------------------------------
TCustomFarPlugin * FarPlugin = NULL;
#define FAR_TITLE_SUFFIX L" - Far"
//---------------------------------------------------------------------------
#pragma package(smart_init)
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
  Token = NULL;
}
//---------------------------------------------------------------------------
TCustomFarPlugin::TCustomFarPlugin(HINSTANCE HInst) :
  TObject()
{
  // DEBUG_PRINTF(L"TCustomFarPlugin: begin");
  InitPlatformId();
  FFarThread = GetCurrentThreadId();
  FCriticalSection = new TCriticalSection();
  FHandle = HInst;
  FFarVersion = 0;
  FTerminalScreenShowing = false;

  FOpenedPlugins = new TObjectList();
  FOpenedPlugins->SetOwnsObjects(false);
  FSavedTitles = new TStringList();
  FCurrentProgress = -1;
  FTopDialog = NULL;
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
TCustomFarPlugin::~TCustomFarPlugin()
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
  for (intptr_t I = 0; I < FSavedTitles->GetCount(); I++)
    delete FSavedTitles->Objects[I];
  delete FSavedTitles;
  delete FCriticalSection;
}
//---------------------------------------------------------------------------
bool TCustomFarPlugin::HandlesFunction(THandlesFunction /*Function*/)
{
  return false;
}
//---------------------------------------------------------------------------
intptr_t TCustomFarPlugin::GetMinFarVersion()
{
  return 0;
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::SetStartupInfo(const struct PluginStartupInfo * Info)
{
  try
  {
    ResetCachedInfo();
    memset(&FStartupInfo, 0, sizeof(FStartupInfo));
    memmove(&FStartupInfo, Info,
            Info->StructSize >= static_cast<intptr_t>(sizeof(FStartupInfo)) ?
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
void TCustomFarPlugin::GetPluginInfo(struct PluginInfo * Info)
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
          wchar_t ** StringArray = static_cast<wchar_t **>(nb_malloc(sizeof(wchar_t *) * NAME.GetCount())); \
          FPluginInfo.NAME = StringArray; \
          FPluginInfo.NAME ## Number = static_cast<int>(NAME.GetCount()); \
          for (intptr_t Index = 0; Index < NAME.GetCount(); ++Index) \
          { \
            StringArray[Index] = DuplicateStr(NAME.GetString(Index)); \
          } \
        }

    COMPOSESTRINGARRAY(DiskMenuStrings);
    COMPOSESTRINGARRAY(PluginMenuStrings);
    COMPOSESTRINGARRAY(PluginConfigStrings);

    #undef COMPOSESTRINGARRAY
    UnicodeString CommandPrefix;
    for (intptr_t Index = 0; Index < CommandPrefixes.GetCount(); ++Index)
    {
      CommandPrefix = CommandPrefix + (CommandPrefix.IsEmpty() ? L"" : L":") +
                      CommandPrefixes.GetString(Index);
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
UnicodeString TCustomFarPlugin::GetModuleName()
{
  return FStartupInfo.ModuleName;
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::ClearPluginInfo(PluginInfo & Info)
{
  if (Info.StructSize)
  {
    #define FREESTRINGARRAY(NAME) \
      for (intptr_t Index = 0; Index < Info.NAME ## Number; ++Index) \
      { \
        nb_free((void *)Info.NAME[Index]); \
      } \
      nb_free((void *)Info.NAME); \
      Info.NAME = NULL;

    FREESTRINGARRAY(DiskMenuStrings);
    FREESTRINGARRAY(PluginMenuStrings);
    FREESTRINGARRAY(PluginConfigStrings);

    #undef FREESTRINGARRAY

    // FIXME delete[] Info.DiskMenuNumbers;
    nb_free((void *)Info.CommandPrefix);
  }
  memset(&Info, 0, sizeof(Info));
  Info.StructSize = sizeof(Info);
}
//---------------------------------------------------------------------------
wchar_t * TCustomFarPlugin::DuplicateStr(const UnicodeString & Str, bool AllowEmpty)
{
  if (Str.IsEmpty() && !AllowEmpty)
  {
    return NULL;
  }
  else
  {
    // DEBUG_PRINTF(L"Str = %s", Str.c_str());
    const size_t sz = Str.Length() + 1;
    wchar_t * Result = static_cast<wchar_t *>(
      nb_malloc(sizeof(wchar_t) * sz));
    wcscpy_s(Result, sz, Str.c_str());
    return Result;
  }
}
//---------------------------------------------------------------------------
RECT TCustomFarPlugin::GetPanelBounds(HANDLE PanelHandle)
{
  PanelInfo Info = {0};
  FarControl(FCTL_GETPANELINFO, 0, reinterpret_cast<intptr_t>(&Info), PanelHandle);
  RECT Bounds;
  memset(&Bounds, -1, sizeof(Bounds));
  if (Info.Plugin)
  {
    Bounds = Info.PanelRect;
  }
  return Bounds;
}

//---------------------------------------------------------------------------
TCustomFarFileSystem * TCustomFarPlugin::GetPanelFileSystem(bool Another,
    HANDLE /* Plugin */)
{
  // DEBUG_PRINTF(L"begin");
  TCustomFarFileSystem * Result = NULL;
  RECT ActivePanelBounds = GetPanelBounds(PANEL_ACTIVE);
  RECT PassivePanelBounds = GetPanelBounds(PANEL_PASSIVE);

  TCustomFarFileSystem * FileSystem = NULL;
  intptr_t Index = 0;
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
    ++Index;
  }
  // DEBUG_PRINTF(L"end");
  return Result;
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::InvalidateOpenPluginInfo()
{
  for (intptr_t Index = 0; Index < FOpenedPlugins->GetCount(); ++Index)
  {
    TCustomFarFileSystem * FileSystem =
      dynamic_cast<TCustomFarFileSystem *>(FOpenedPlugins->GetItem(Index));
    FileSystem->InvalidateOpenPluginInfo();
  }
}
//---------------------------------------------------------------------------
intptr_t TCustomFarPlugin::Configure(intptr_t Item)
{
  try
  {
    ResetCachedInfo();
    intptr_t Result = ConfigureEx(Item);
    InvalidateOpenPluginInfo();

    return Result;
  }
  catch (Exception & E)
  {
    DEBUG_PRINTF(L"before HandleException");
    HandleException(&E);
    return static_cast<int>(false);
  }
}
//---------------------------------------------------------------------------
void * TCustomFarPlugin::OpenPlugin(int OpenFrom, intptr_t Item)
{
#ifdef USE_DLMALLOC
  // dlmallopt(M_GRANULARITY, 128 * 1024);
#endif

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
      Result = static_cast<TCustomFarFileSystem *>(INVALID_HANDLE_VALUE);
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
void TCustomFarPlugin::ClosePlugin(void * Plugin)
{
  try
  {
    ResetCachedInfo();
    TCustomFarFileSystem * FileSystem = static_cast<TCustomFarFileSystem *>(Plugin);
    assert(FOpenedPlugins->IndexOf(FileSystem) != NPOS);
    TRY_FINALLY (
    {
      {
        TGuard Guard(FileSystem->GetCriticalSection());
        FileSystem->Close();
      }
    }
    ,
    {
      FOpenedPlugins->Remove(FileSystem);
    }
    );
    delete FileSystem;
#ifdef USE_DLMALLOC
    // dlmalloc_trim(0); // 64 * 1024);
#endif
  }
  catch(Exception & E)
  {
    DEBUG_PRINTF(L"before HandleException");
    HandleException(&E);
  }
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::HandleFileSystemException(
  TCustomFarFileSystem * FileSystem, Exception * E, int OpMode)
{
  // This method is called as last-resort exception handler before
  // leaving plugin API. Especially for API functions that must update
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
void TCustomFarPlugin::GetOpenPluginInfo(HANDLE Plugin,
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
intptr_t TCustomFarPlugin::GetFindData(HANDLE Plugin,
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
void TCustomFarPlugin::FreeFindData(HANDLE Plugin,
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
intptr_t TCustomFarPlugin::ProcessHostFile(HANDLE Plugin,
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
intptr_t TCustomFarPlugin::ProcessKey(HANDLE Plugin, int Key,
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
intptr_t TCustomFarPlugin::ProcessEvent(HANDLE Plugin, int Event, void * Param)
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
      return static_cast<int>(false);
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
intptr_t TCustomFarPlugin::SetDirectory(HANDLE Plugin, const wchar_t * Dir, int OpMode)
{
  TCustomFarFileSystem * FileSystem = static_cast<TCustomFarFileSystem *>(Plugin);
  UnicodeString PrevCurrentDirectory = FileSystem->GetCurrentDirectory();
  // DEBUG_PRINTF(L"PrevCurrentDirectory = %s", PrevCurrentDirectory.c_str());
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
    if (!PrevCurrentDirectory.IsEmpty())
    {
      try
      {
        TGuard Guard(FileSystem->GetCriticalSection());
        return FileSystem->SetDirectory(PrevCurrentDirectory, OpMode);
      }
      catch(Exception & E)
      {
        (void)E;
        return 0;
      }
    }
    return 0;
  }
}
//---------------------------------------------------------------------------
intptr_t TCustomFarPlugin::MakeDirectory(HANDLE Plugin, const wchar_t ** Name, int OpMode)
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
intptr_t TCustomFarPlugin::DeleteFiles(HANDLE Plugin,
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
intptr_t TCustomFarPlugin::GetFiles(HANDLE Plugin,
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
intptr_t TCustomFarPlugin::PutFiles(HANDLE Plugin,
  struct PluginPanelItem * PanelItem, int ItemsNumber, int Move, const wchar_t * srcPath, int OpMode)
{
  TCustomFarFileSystem * FileSystem = static_cast<TCustomFarFileSystem *>(Plugin);
  try
  {
    ResetCachedInfo();
    assert(FOpenedPlugins->IndexOf(FileSystem) != NPOS);

    {
      TGuard Guard(FileSystem->GetCriticalSection());
      return FileSystem->PutFiles(PanelItem, ItemsNumber, Move, srcPath, OpMode);
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
intptr_t TCustomFarPlugin::ProcessEditorEvent(int Event, void * Param)
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
intptr_t TCustomFarPlugin::ProcessEditorInput(const INPUT_RECORD * Rec)
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
intptr_t TCustomFarPlugin::MaxMessageLines()
{
  return static_cast<intptr_t>(TerminalInfo().y - 5);
}
//---------------------------------------------------------------------------
intptr_t TCustomFarPlugin::MaxMenuItemLength()
{
  // got from maximal length of path in FAR's folders history
  return static_cast<intptr_t>(TerminalInfo().x - 13);
}
//---------------------------------------------------------------------------
intptr_t TCustomFarPlugin::MaxLength(TStrings * Strings)
{
  intptr_t Result = 0;
  for (intptr_t Index = 0; Index < Strings->GetCount(); ++Index)
  {
    if (Result < Strings->GetString(Index).Length())
    {
      Result = Strings->GetString(Index).Length();
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
class TFarMessageDialog : public TFarDialog
{
public:
  TFarMessageDialog(TCustomFarPlugin * Plugin,
    TFarMessageParams * Params);
  void Init(unsigned int AFlags,
    const UnicodeString & Title, const UnicodeString & Message, TStrings * Buttons);

  intptr_t Execute(bool & ACheckBox);

protected:
  virtual void Change();
  virtual void Idle();

private:
  void ButtonClick(TFarButton * Sender, bool & Close);

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
TFarMessageDialog::TFarMessageDialog(TCustomFarPlugin * Plugin,
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
void TFarMessageDialog::Init(unsigned int AFlags,
  const UnicodeString & Title, const UnicodeString & Message, TStrings * Buttons)
{
  assert(FLAGCLEAR(AFlags, FMSG_ERRORTYPE));
  assert(FLAGCLEAR(AFlags, FMSG_KEEPBACKGROUND));
  // FIXME assert(FLAGCLEAR(AFlags, FMSG_DOWN));
  assert(FLAGCLEAR(AFlags, FMSG_ALLINONE));

  TStrings * MessageLines = new TStringList();
  {
    std::auto_ptr<TStrings> MessageLinesPtr;
    MessageLinesPtr.reset(MessageLines);
    FarWrapText(Message, MessageLines, MaxMessageWidth);
    intptr_t MaxLen = GetFarPlugin()->MaxLength(MessageLines);
    // DEBUG_PRINTF(L"MaxLen = %d, FParams->MoreMessages = %x", MaxLen, FParams->MoreMessages);
    TStrings * MoreMessageLines = NULL;
    std::auto_ptr<TStrings> MoreMessageLinesPtr(NULL);
    if (FParams->MoreMessages != NULL)
    {
      MoreMessageLines = new TStringList();
      MoreMessageLinesPtr.reset(MoreMessageLines);
      UnicodeString MoreMessages = FParams->MoreMessages->Text;
      while (MoreMessages[MoreMessages.Length()] == L'\n' ||
             MoreMessages[MoreMessages.Length()] == L'\r')
      {
        MoreMessages.SetLength(MoreMessages.Length() - 1);
      }
      FarWrapText(MoreMessages, MoreMessageLines, MaxMessageWidth);
      intptr_t MoreMaxLen = GetFarPlugin()->MaxLength(MoreMessageLines);
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

    for (intptr_t Index = 0; Index < MessageLines->GetCount(); ++Index)
    {
      TFarText * Text = new TFarText(this);
      Text->SetCaption(MessageLines->GetString(Index));
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
    for (intptr_t Index = 0; Index < Buttons->GetCount(); ++Index)
    {
      TFarButton * PrevButton = Button;
      Button = new TFarButton(this);
      Button->SetDefault(Index == 0);
      Button->SetBrackets(brNone);
      Button->SetOnClick(MAKE_CALLBACK(TFarMessageDialog::ButtonClick, this));
      UnicodeString Caption = Buttons->GetString(Index);
      if ((FParams->Timeout > 0) &&
          (FParams->TimeoutButton == static_cast<size_t>(Index)))
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
      Button->SetTag(reinterpret_cast<intptr_t>(Buttons->Objects[Index]));
      if (PrevButton != NULL)
      {
        Button->Move(PrevButton->GetRight() - Button->GetLeft() + 1, 0);
      }

      if (MaxMessageWidth < Button->GetRight() - GetBorderBox()->GetLeft())
      {
        for (intptr_t PIndex = 0; PIndex < GetItemCount(); ++PIndex)
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
        MaxLen = static_cast<intptr_t>(Button->GetRight() - GetBorderBox()->GetLeft() + 2);
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
        MaxLen = static_cast<intptr_t>(FCheckBox->GetRight() - GetBorderBox()->GetLeft());
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
      static_cast<int>(rect.Left + MaxLen - rect.Right),
      static_cast<int>(rect.Top + MessageLines->GetCount() +
      (FParams->MoreMessages != NULL ? 1 : 0) + ButtonLines +
      (!FParams->CheckBoxLabel.IsEmpty() ? 1 : 0) +
      (-(rect.Bottom + 1))));

    if (FParams->MoreMessages != NULL)
    {
      intptr_t MoreMessageHeight = static_cast<intptr_t>(GetFarPlugin()->TerminalInfo().y - S.y - 1);
      assert(MoreMessagesLister != NULL);
      if (MoreMessageHeight > MoreMessagesLister->GetItems()->GetCount())
      {
        MoreMessageHeight = MoreMessagesLister->GetItems()->GetCount();
      }
      MoreMessagesLister->SetHeight(MoreMessageHeight);
      MoreMessagesLister->SetRight(
        GetBorderBox()->GetRight() - (MoreMessagesLister->GetScrollBar() ? 0 : 1));
      MoreMessagesLister->SetTabStop(MoreMessagesLister->GetScrollBar());
      assert(MoreMessagesSeparator != NULL);
      MoreMessagesSeparator->SetPosition(
        static_cast<int>(MoreMessagesLister->GetTop() + MoreMessagesLister->GetHeight()));
      S.y += static_cast<int>(MoreMessagesLister->GetHeight()) + 1;
    }
    // DEBUG_PRINTF(L"S.x = %d, S.y = %d", S.x, S.y);
    SetSize(S);
  }
}

//---------------------------------------------------------------------------
void TFarMessageDialog::Idle()
{
  TFarDialog::Idle();

  if (FParams->Timer > 0)
  {
    size_t SinceLastTimer = static_cast<size_t>((static_cast<double>(Now()) - static_cast<double>(FLastTimerTime)) * 24*60*60*1000);
    if (SinceLastTimer >= FParams->Timeout)
    {
      assert(FParams->TimerEvent);
      if (FParams->TimerEvent)
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
      intptr_t sz = FTimeoutButton->GetCaption().Length() > Caption.Length() ? FTimeoutButton->GetCaption().Length() - Caption.Length() : 0;
      Caption += ::StringOfChar(L' ', sz);
      FTimeoutButton->SetCaption(Caption);
    }
  }
}
//---------------------------------------------------------------------------
void TFarMessageDialog::Change()
{
  TFarDialog::Change();

  if (GetHandle() != NULL)
  {
    if ((FCheckBox != NULL) && (FCheckBoxChecked != FCheckBox->GetChecked()))
    {
      for (intptr_t Index = 0; Index < GetItemCount(); ++Index)
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
intptr_t TFarMessageDialog::Execute(bool & ACheckBox)
{
  FStartTime = Now();
  FLastTimerTime = FStartTime;
  FCheckBoxChecked = !ACheckBox;
  if (FCheckBox != NULL)
  {
    FCheckBox->SetChecked(ACheckBox);
  }

  intptr_t Result = ShowModal();
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
void TFarMessageDialog::ButtonClick(TFarButton * Sender, bool & Close)
{
  if (FParams->ClickEvent)
  {
    FParams->ClickEvent(FParams->Token, Sender->GetResult() - 1, Close);
  }
}
//---------------------------------------------------------------------------
intptr_t TCustomFarPlugin::DialogMessage(DWORD Flags,
  const UnicodeString & Title, const UnicodeString & Message, TStrings * Buttons,
  TFarMessageParams * Params)
{
  intptr_t Result;
  TFarMessageDialog * Dialog =
    new TFarMessageDialog(this, Params);
  {
    std::auto_ptr<TFarMessageDialog> DialogPtr;
    DialogPtr.reset(Dialog);
    Dialog->Init(Flags, Title, Message, Buttons);
    Result = Dialog->Execute(Params->CheckBox);
  }
  return Result;
}
//---------------------------------------------------------------------------
intptr_t TCustomFarPlugin::FarMessage(DWORD Flags,
  const UnicodeString & Title, const UnicodeString & Message, TStrings * Buttons,
  TFarMessageParams * Params)
{
  assert(Params != NULL);

  intptr_t Result;
  TStringList * MessageLines = NULL;
  std::auto_ptr<TStrings> MessageLinesPtr(NULL);
  wchar_t ** Items = NULL;
  TRY_FINALLY (
  {
    UnicodeString FullMessage = Message;
    if (Params->MoreMessages != NULL)
    {
      FullMessage += UnicodeString(L"\n\x01\n") + Params->MoreMessages->Text;
      while (FullMessage[FullMessage.Length()] == L'\n' ||
             FullMessage[FullMessage.Length()] == L'\r')
      {
        FullMessage.SetLength(FullMessage.Length() - 1);
      }
      FullMessage += L"\n\x01\n";
    }

    MessageLines = new TStringList();
    MessageLinesPtr.reset(MessageLines);
    MessageLines->Add(Title);
    FarWrapText(FullMessage, MessageLines, MaxMessageWidth);

    // FAR WORKAROUND
    // When there is too many lines to fit on screen, far uses not-shown
    // lines as button captions instead of real captions at the end of the list
    intptr_t MaxLines = MaxMessageLines();
    while (MessageLines->GetCount() > MaxLines)
    {
      MessageLines->Delete(MessageLines->GetCount() - 1);
    }

    for (intptr_t Index = 0; Index < Buttons->GetCount(); ++Index)
    {
      MessageLines->Add(Buttons->GetString(Index));
    }

    Items = static_cast<wchar_t **>(
      nb_malloc(sizeof(wchar_t *) * MessageLines->GetCount()));
    for (intptr_t Index = 0; Index < MessageLines->GetCount(); ++Index)
    {
      UnicodeString S = MessageLines->GetString(Index);
      MessageLines->SetString(Index, UnicodeString(S));
      Items[Index] = const_cast<wchar_t *>(MessageLines->GetString(Index).c_str());
    }

    TFarEnvGuard Guard;
    Result = static_cast<intptr_t>(FStartupInfo.Message(FStartupInfo.ModuleNumber,
      Flags | FMSG_LEFTALIGN, NULL, Items, static_cast<int>(MessageLines->GetCount()),
      static_cast<int>(Buttons->GetCount())));
  }
  ,
  {
    nb_free((void *)Items);
  }
  );

  return Result;
}
//---------------------------------------------------------------------------
intptr_t TCustomFarPlugin::Message(DWORD Flags,
  const UnicodeString & Title, const UnicodeString & Message, TStrings * Buttons,
  TFarMessageParams * Params)
{
  // DEBUG_PRINTF(L"Message = %s", Message.c_str());
  // when message is shown while some "custom" output is on screen,
  // make the output actually background of FAR screen
  if (FTerminalScreenShowing)
  {
    FarControl(FCTL_SETUSERSCREEN, 0, NULL);
  }

  intptr_t Result;
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
    Result = static_cast<intptr_t>(FStartupInfo.Message(FStartupInfo.ModuleNumber,
      Flags | FMSG_ALLINONE | FMSG_LEFTALIGN,
      NULL,
      static_cast<const wchar_t * const *>(static_cast<const void *>(Items.c_str())), 0, 0));
  }
  return Result;
}
//---------------------------------------------------------------------------
intptr_t TCustomFarPlugin::Menu(DWORD Flags, const UnicodeString & Title,
  const UnicodeString & Bottom, const FarMenuItem * Items, intptr_t Count,
  const int * BreakKeys, int & BreakCode)
{
  assert(Items);

  UnicodeString ATitle = Title;
  UnicodeString ABottom = Bottom;
  TFarEnvGuard Guard;
  return static_cast<intptr_t>(FStartupInfo.Menu(FStartupInfo.ModuleNumber, -1, -1, 0,
    Flags, ATitle.c_str(), ABottom.c_str(), NULL, BreakKeys,
    &BreakCode, Items, static_cast<int>(Count)));
}
//---------------------------------------------------------------------------
intptr_t TCustomFarPlugin::Menu(DWORD Flags, const UnicodeString & Title,
  const UnicodeString & Bottom, TStrings * Items, const int * BreakKeys,
  int & BreakCode)
{
  assert(Items && Items->GetCount());
  intptr_t Result = 0;
  FarMenuItemEx * MenuItems = static_cast<FarMenuItemEx *>(
    nb_malloc(sizeof(FarMenuItemEx) * Items->GetCount()));
  TRY_FINALLY (
  {
    intptr_t Selected = NPOS;
    intptr_t Count = 0;
    for (intptr_t I = 0; I < Items->GetCount(); ++I)
    {
      uintptr_t Flags = reinterpret_cast<uintptr_t>(Items->Objects[I]);
      if (FLAGCLEAR(Flags, MIF_HIDDEN))
      {
        memset(&MenuItems[Count], 0, sizeof(FarMenuItemEx));
        MenuItems[Count].Flags = static_cast<DWORD>(Flags);
        if (MenuItems[Count].Flags & MIF_SELECTED)
        {
          assert(Selected == NPOS);
          Selected = I;
        }
        MenuItems[Count].Text = Items->GetString(I).c_str();
        MenuItems[Count].UserData = I;
        Count++;
      }
    }

    intptr_t ResultItem = Menu((DWORD)(Flags | FMENU_USEEXT), Title, Bottom,
      reinterpret_cast<const FarMenuItem *>(MenuItems), Count, BreakKeys, BreakCode);

    if (ResultItem >= 0)
    {
      Result = MenuItems[ResultItem].UserData;
      if (Selected >= 0)
      {
        Items->Objects(Selected, reinterpret_cast<TObject *>(reinterpret_cast<size_t>(Items->Objects[Selected]) & ~MIF_SELECTED));
      }
      Items->Objects(Result, reinterpret_cast<TObject *>(reinterpret_cast<size_t>(Items->Objects[Result]) | MIF_SELECTED));
    }
    else
    {
      Result = ResultItem;
    }
  }
  ,
  {
    nb_free(MenuItems);
  }
  );
  return Result;
}
//---------------------------------------------------------------------------
intptr_t TCustomFarPlugin::Menu(DWORD Flags, const UnicodeString & Title,
  const UnicodeString & Bottom, TStrings * Items)
{
  int BreakCode;
  return Menu(Flags, Title, Bottom, Items, NULL, BreakCode);
}
//---------------------------------------------------------------------------
bool TCustomFarPlugin::InputBox(const UnicodeString & Title,
  const UnicodeString & Prompt, UnicodeString & Text, DWORD Flags,
  const UnicodeString & HistoryName, intptr_t MaxLen, TFarInputBoxValidateEvent OnValidate)
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
        static_cast<int>(MaxLen),
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
void TCustomFarPlugin::Text(int X, int Y, int Color, const UnicodeString & Str)
{
  TFarEnvGuard Guard;
  FStartupInfo.Text(X, Y, Color, Str.c_str());
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::FlushText()
{
  TFarEnvGuard Guard;
  FStartupInfo.Text(0, 0, 0, NULL);
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::WriteConsole(const UnicodeString & Str)
{
  DWORD Written;
  ::WriteConsole(FConsoleOutput, Str.c_str(), static_cast<DWORD>(Str.Length()), &Written, NULL);
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::FarCopyToClipboard(const UnicodeString & Str)
{
  TFarEnvGuard Guard;
  FFarStandardFunctions.CopyToClipboard(Str.c_str());
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::FarCopyToClipboard(TStrings * Strings)
{
  if (Strings->GetCount() > 0)
  {
    if (Strings->GetCount() == 1)
    {
      FarCopyToClipboard(Strings->GetString(0));
    }
    else
    {
      FarCopyToClipboard(Strings->Text);
    }
  }
}
//---------------------------------------------------------------------------
TPoint TCustomFarPlugin::TerminalInfo(TPoint * Size, TPoint * Cursor)
{
  // DEBUG_PRINTF(L"begin");
  CONSOLE_SCREEN_BUFFER_INFO BufferInfo;
  memset(&BufferInfo, 0, sizeof(BufferInfo));
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
HWND TCustomFarPlugin::GetConsoleWindow()
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
void TCustomFarPlugin::ToggleVideoMode()
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
void TCustomFarPlugin::ScrollTerminalScreen(int Rows)
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
void TCustomFarPlugin::ShowTerminalScreen()
{
  assert(!FTerminalScreenShowing);
  TPoint Size, Cursor;
  TerminalInfo(&Size, &Cursor);

  UnicodeString Blank = ::StringOfChar(L' ', static_cast<intptr_t>(Size.x));
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
void TCustomFarPlugin::SaveTerminalScreen()
{
  FTerminalScreenShowing = false;
  FarControl(FCTL_SETUSERSCREEN, 0, NULL);
}
//---------------------------------------------------------------------------
class TConsoleTitleParam : public TObject
{
public:
  TConsoleTitleParam() :
    Progress(0),
    Own(0)
  {}
  short Progress;
  short Own;
};
//---------------------------------------------------------------------------
void TCustomFarPlugin::ShowConsoleTitle(const UnicodeString & Title)
{
  wchar_t SaveTitle[1024];
  GetConsoleTitle(SaveTitle, sizeof(SaveTitle));
  TConsoleTitleParam * Param = new TConsoleTitleParam();
  Param->Progress = FCurrentProgress;
  Param->Own = !FCurrentTitle.IsEmpty() && (FormatConsoleTitle() == SaveTitle);
  if (Param->Own)
  {
    FSavedTitles->AddObject(FCurrentTitle, Param);
  }
  else
  {
    FSavedTitles->AddObject(SaveTitle, Param);
  }
  FCurrentTitle = Title;
  FCurrentProgress = -1;
  UpdateConsoleTitle();
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::ClearConsoleTitle()
{
  assert(FSavedTitles->GetCount() > 0);
  UnicodeString Title = FSavedTitles->GetString(FSavedTitles->GetCount() - 1);
  TObject * Object = FSavedTitles->Objects[FSavedTitles->GetCount()-1];
  TConsoleTitleParam * Param = dynamic_cast<TConsoleTitleParam *>(Object);
  if (Param->Own)
  {
    FCurrentTitle = Title;
    FCurrentProgress = Param->Progress;
    UpdateConsoleTitle();
  }
  else
  {
    FCurrentTitle = L"";
    FCurrentProgress = -1;
    SetConsoleTitle(Title.c_str());
    UpdateProgress(PS_NOPROGRESS, 0);
  }
  delete FSavedTitles->Objects(FSavedTitles->GetCount() - 1);
  FSavedTitles->Delete(FSavedTitles->GetCount() - 1);
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::UpdateConsoleTitle(const UnicodeString & Title)
{
  FCurrentTitle = Title;
  UpdateConsoleTitle();
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::UpdateConsoleTitleProgress(short Progress)
{
  FCurrentProgress = Progress;
  UpdateConsoleTitle();
}
//---------------------------------------------------------------------------
UnicodeString TCustomFarPlugin::FormatConsoleTitle()
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
void TCustomFarPlugin::UpdateProgress(intptr_t State, intptr_t Progress)
{
  FarAdvControl(ACTL_SETPROGRESSSTATE, reinterpret_cast<void *>(State));
  if (State == PS_NORMAL)
  {
    PROGRESSVALUE pv;
    pv.Completed = Progress < 0 ? 0 : Progress > 100 ? 100 : Progress;
    pv.Total = 100;
    FarAdvControl(ACTL_SETPROGRESSVALUE, static_cast<void *>(&pv));
  }
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::UpdateConsoleTitle()
{
  UnicodeString Title = FormatConsoleTitle();
  SetConsoleTitle(Title.c_str());
  short progress = FCurrentProgress != -1 ? FCurrentProgress : 0;
  UpdateProgress(progress != 0 ? PS_NORMAL : PS_NOPROGRESS, progress);
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::SaveScreen(HANDLE & Screen)
{
  assert(!Screen);
  TFarEnvGuard Guard;
  Screen = static_cast<HANDLE>(FStartupInfo.SaveScreen(0, 0, -1, -1));
  assert(Screen);
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::RestoreScreen(HANDLE & Screen)
{
  assert(Screen);
  TFarEnvGuard Guard;
  FStartupInfo.RestoreScreen(static_cast<HANDLE>(Screen));
  Screen = 0;
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::HandleException(Exception * E, int /*OpMode*/)
{
  assert(E);
  Message(FMSG_WARNING | FMSG_MB_OK, L"", MB2W(E->what()));
}
//---------------------------------------------------------------------------
UnicodeString TCustomFarPlugin::GetMsg(intptr_t MsgId)
{
  TFarEnvGuard Guard;
  UnicodeString Result = FStartupInfo.GetMsg(FStartupInfo.ModuleNumber, MsgId);
  return Result;
}
//---------------------------------------------------------------------------
bool TCustomFarPlugin::CheckForEsc()
{
  static unsigned long LastTicks;
  unsigned long Ticks = GetTickCount();
  if ((LastTicks == 0) || (Ticks - LastTicks > 500))
  {
    LastTicks = Ticks;

    INPUT_RECORD Rec;
    DWORD ReadCount;
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
  }
  return false;
}
//---------------------------------------------------------------------------
bool TCustomFarPlugin::Viewer(const UnicodeString & FileName,
  const UnicodeString & Title, DWORD Flags)
{
  TFarEnvGuard Guard;
  int Result = FStartupInfo.Viewer(
    FileName.c_str(),
    Title.c_str(), 0, 0, -1, -1, Flags,
    CP_AUTODETECT);
  return Result > 0;
}
//---------------------------------------------------------------------------
bool TCustomFarPlugin::Editor(const UnicodeString & FileName,
  const UnicodeString & Title, DWORD Flags)
{
  TFarEnvGuard Guard;
  int Result = FStartupInfo.Editor(
    FileName.c_str(),
    Title.c_str(), 0, 0, -1, -1, Flags, -1, -1,
    CP_AUTODETECT);
  return (Result == EEC_MODIFIED) || (Result == EEC_NOT_MODIFIED);
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::ResetCachedInfo()
{
  FValidFarSystemSettings = false;
}
//---------------------------------------------------------------------------
intptr_t TCustomFarPlugin::FarSystemSettings()
{
  if (!FValidFarSystemSettings)
  {
    FFarSystemSettings = FarAdvControl(ACTL_GETSYSTEMSETTINGS);
    FValidFarSystemSettings = true;
  }
  return FFarSystemSettings;
}
//---------------------------------------------------------------------------
intptr_t TCustomFarPlugin::FarControl(uintptr_t Command, intptr_t Param1, intptr_t Param2, HANDLE Plugin)
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
  return FStartupInfo.Control(Plugin, static_cast<int>(Command), static_cast<int>(Param1), Param2);
}
//---------------------------------------------------------------------------
intptr_t TCustomFarPlugin::FarAdvControl(uintptr_t Command, void * Param)
{
  TFarEnvGuard Guard;
  return FStartupInfo.AdvControl(FStartupInfo.ModuleNumber, static_cast<int>(Command), Param);
}
//---------------------------------------------------------------------------
intptr_t TCustomFarPlugin::FarEditorControl(uintptr_t Command, void * Param)
{
  switch (Command)
  {
  case ECTL_GETINFO:
  case ECTL_SETPARAM:
  case ECTL_GETFILENAME:
    // noop
    break;

  case ECTL_SETTITLE:
    break;

  default:
    // for other commands, OEM/ANSI conversion to be verified
    assert(false);
    break;
  }

  TFarEnvGuard Guard;
  return static_cast<intptr_t>(FStartupInfo.EditorControl(static_cast<int>(Command), Param));
}
//---------------------------------------------------------------------------
TFarEditorInfo * TCustomFarPlugin::EditorInfo()
{
  TFarEditorInfo * Result;
  ::EditorInfo * Info = static_cast<::EditorInfo *>(
    nb_malloc(sizeof(::EditorInfo)));
  try
  {
    if (FarEditorControl(ECTL_GETINFO, Info))
    {
      Result = new TFarEditorInfo(Info);
    }
    else
    {
      nb_free(Info);
      Result = NULL;
    }
  }
  catch (...)
  {
    nb_free(Info);
    throw;
  }
  return Result;
}
//---------------------------------------------------------------------------
intptr_t TCustomFarPlugin::FarVersion()
{
  if (FFarVersion == 0)
  {
    FFarVersion = FarAdvControl(ACTL_GETFARVERSION);
  }
  return FFarVersion;
}
//---------------------------------------------------------------------------
UnicodeString TCustomFarPlugin::FormatFarVersion(intptr_t Version)
{
  return FORMAT(L"%d.%d.%d", (Version >> 8) & 0xFF, Version & 0xFF, Version >> 16);
}
//---------------------------------------------------------------------------
UnicodeString TCustomFarPlugin::TemporaryDir()
{
  UnicodeString Result;
  if (FTemporaryDir.IsEmpty())
  {
    Result.SetLength(MAX_PATH);
    TFarEnvGuard Guard;
    FFarStandardFunctions.MkTemp(const_cast<wchar_t *>(Result.c_str()), (DWORD)Result.Length(), NULL);
    PackStr(Result);
    FTemporaryDir = Result;
  }
  else
  {
    Result = FTemporaryDir;
  }
  return Result;
}
//---------------------------------------------------------------------------
intptr_t TCustomFarPlugin::InputRecordToKey(const INPUT_RECORD * Rec)
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
  return static_cast<intptr_t>(Result);
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
TCustomFarFileSystem::TCustomFarFileSystem(TCustomFarPlugin * APlugin) :
  TObject(),
  FPlugin(APlugin),
  FClosed(false),
  FCriticalSection(NULL),
  FOpenPluginInfoValid(false)
{
  memset(FPanelInfo, 0, sizeof(FPanelInfo));
}

void TCustomFarFileSystem::Init()
{
  FCriticalSection = new TCriticalSection();
  FPanelInfo[0] = NULL;
  FPanelInfo[1] = NULL;
  FClosed = false;

  memset(&FOpenPluginInfo, 0, sizeof(FOpenPluginInfo));
  ClearOpenPluginInfo(FOpenPluginInfo);
  FInstances++;
  // DEBUG_PRINTF(L"FInstances = %d", FInstances);
}

//---------------------------------------------------------------------------
TCustomFarFileSystem::~TCustomFarFileSystem()
{
  FInstances--;
  // DEBUG_PRINTF(L"FInstances = %d", FInstances);
  ResetCachedInfo();
  ClearOpenPluginInfo(FOpenPluginInfo);
  delete FCriticalSection;
}
//---------------------------------------------------------------------------
void TCustomFarFileSystem::HandleException(Exception * E, int OpMode)
{
  DEBUG_PRINTF(L"before FPlugin->HandleException");
  FPlugin->HandleException(E, OpMode);
}
//---------------------------------------------------------------------------
void TCustomFarFileSystem::Close()
{
  FClosed = true;
}
//---------------------------------------------------------------------------
void TCustomFarFileSystem::InvalidateOpenPluginInfo()
{
  FOpenPluginInfoValid = false;
}
//---------------------------------------------------------------------------
void TCustomFarFileSystem::ClearOpenPluginInfo(OpenPluginInfo & Info)
{
  if (Info.StructSize)
  {
    nb_free((void *)Info.HostFile);
    nb_free((void *)Info.CurDir);
    nb_free((void *)Info.Format);
    nb_free((void *)Info.PanelTitle);
    assert(!Info.InfoLines);
    assert(!Info.InfoLinesNumber);
    assert(!Info.DescrFiles);
    assert(!Info.DescrFilesNumber);
    assert(Info.PanelModesNumber == 0 || Info.PanelModesNumber == PANEL_MODES_COUNT);
    for (intptr_t Index = 0; Index < Info.PanelModesNumber; ++Index)
    {
      assert(Info.PanelModesArray);
      TFarPanelModes::ClearPanelMode(
        const_cast<PanelMode &>(Info.PanelModesArray[Index]));
    }
    if (Info.KeyBar)
    {
      TFarKeyBarTitles::ClearKeyBarTitles(const_cast<KeyBarTitles &>(*Info.KeyBar));
      nb_free((void *)Info.KeyBar);
    }
    nb_free((void *)Info.ShortcutData);
  }
  memset(&Info, 0, sizeof(Info));
  Info.StructSize = sizeof(Info);
  InvalidateOpenPluginInfo();
}
//---------------------------------------------------------------------------
void TCustomFarFileSystem::GetOpenPluginInfo(struct OpenPluginInfo * Info)
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
      {
        std::auto_ptr<TFarPanelModes> PanelModesPtr(NULL);
        std::auto_ptr<TFarKeyBarTitles> KeyBarTitlesPtr(NULL);
        TFarPanelModes * PanelModes = new TFarPanelModes();
        PanelModesPtr.reset(PanelModes);
        TFarKeyBarTitles * KeyBarTitles = new TFarKeyBarTitles();
        KeyBarTitlesPtr.reset(KeyBarTitles);
        bool StartSortOrder = false;

        GetOpenPluginInfoEx(FOpenPluginInfo.Flags, HostFile, CurDir, Format,
          PanelTitle, PanelModes, FOpenPluginInfo.StartPanelMode,
          FOpenPluginInfo.StartSortMode, StartSortOrder, KeyBarTitles, ShortcutData);

        FOpenPluginInfo.HostFile = TCustomFarPlugin::DuplicateStr(HostFile);
        FOpenPluginInfo.CurDir = TCustomFarPlugin::DuplicateStr(::StringReplace(CurDir, L"\\", L"/", TReplaceFlags() << rfReplaceAll));
        FOpenPluginInfo.Format = TCustomFarPlugin::DuplicateStr(Format);
        FOpenPluginInfo.PanelTitle = TCustomFarPlugin::DuplicateStr(PanelTitle);
        PanelModes->FillOpenPluginInfo(&FOpenPluginInfo);
        FOpenPluginInfo.StartSortOrder = StartSortOrder;
        KeyBarTitles->FillOpenPluginInfo(&FOpenPluginInfo);
        FOpenPluginInfo.ShortcutData = TCustomFarPlugin::DuplicateStr(ShortcutData);
      }

      FOpenPluginInfoValid = true;
    }

    memmove(Info, &FOpenPluginInfo, sizeof(FOpenPluginInfo));
  }
}
//---------------------------------------------------------------------------
intptr_t TCustomFarFileSystem::GetFindData(
  struct PluginPanelItem ** PanelItem, int * ItemsNumber, int OpMode)
{
  // DEBUG_PRINTF(L"begin");
  ResetCachedInfo();
  bool Result = false;
  TObjectList * PanelItems = new TObjectList();
  {
    std::auto_ptr<TObjectList> PanelItemsPtr;
    PanelItemsPtr.reset(PanelItems);
    Result = !FClosed && GetFindDataEx(PanelItems, OpMode);
    // DEBUG_PRINTF(L"Result = %d, PanelItems->GetCount() = %d", Result, PanelItems->Count);
    if (Result && PanelItems->GetCount())
    {
      *PanelItem = static_cast<PluginPanelItem *>(
        nb_malloc(sizeof(PluginPanelItem) * PanelItems->GetCount()));
      memset(*PanelItem, 0, PanelItems->GetCount() * sizeof(PluginPanelItem));
      *ItemsNumber = static_cast<int>(PanelItems->GetCount());
      for (intptr_t Index = 0; Index < PanelItems->GetCount(); ++Index)
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
  // DEBUG_PRINTF(L"end: Result = %d", Result);
  return Result;
}
//---------------------------------------------------------------------------
void TCustomFarFileSystem::FreeFindData(
  struct PluginPanelItem * PanelItem, int ItemsNumber)
{
  ResetCachedInfo();
  if (PanelItem)
  {
    assert(ItemsNumber > 0);
    for (intptr_t Index = 0; Index < ItemsNumber; ++Index)
    {
      nb_free((void *)PanelItem[Index].FindData.lpwszFileName);
      nb_free((void *)PanelItem[Index].Description);
      nb_free((void *)PanelItem[Index].Owner);
      for (intptr_t CustomIndex = 0; CustomIndex < PanelItem[Index].CustomColumnNumber; ++CustomIndex)
      {
        nb_free((void *)PanelItem[Index].CustomColumnData[CustomIndex]);
      }
      nb_free((void *)PanelItem[Index].CustomColumnData);
    }
    nb_free((void *)PanelItem);
  }
}
//---------------------------------------------------------------------------
intptr_t TCustomFarFileSystem::ProcessHostFile(struct PluginPanelItem * PanelItem,
  int ItemsNumber, int OpMode)
{
  ResetCachedInfo();
  bool Result = false;
  TObjectList * PanelItems = CreatePanelItemList(PanelItem, ItemsNumber);
  {
    std::auto_ptr<TObjectList> PanelItemsPtr;
    PanelItemsPtr.reset(PanelItems);
    Result = ProcessHostFileEx(PanelItems, OpMode);
  }

  return Result;
}
//---------------------------------------------------------------------------
intptr_t TCustomFarFileSystem::ProcessKey(intptr_t Key, uintptr_t ControlState)
{
  ResetCachedInfo();
  return ProcessKeyEx(Key, ControlState);
}
//---------------------------------------------------------------------------
intptr_t TCustomFarFileSystem::ProcessEvent(intptr_t Event, void * Param)
{
  ResetCachedInfo();
  return ProcessEventEx(Event, Param);
}
//---------------------------------------------------------------------------
intptr_t TCustomFarFileSystem::SetDirectory(const wchar_t * Dir, int OpMode)
{
  ResetCachedInfo();
  InvalidateOpenPluginInfo();
  int Result = SetDirectoryEx(Dir, OpMode);
  InvalidateOpenPluginInfo();
  return Result;
}
//---------------------------------------------------------------------------
intptr_t TCustomFarFileSystem::MakeDirectory(const wchar_t ** Name, int OpMode)
{
  ResetCachedInfo();
  FNameStr = *Name;
  intptr_t Result = 0;
  TRY_FINALLY (
  {
    Result = MakeDirectoryEx(FNameStr, OpMode);
  }
  ,
  {
    if (FNameStr != *Name)
    {
      *Name = FNameStr.c_str();
    }
  }
  );
  return Result;
}
//---------------------------------------------------------------------------
intptr_t TCustomFarFileSystem::DeleteFiles(struct PluginPanelItem * PanelItem,
    int ItemsNumber, int OpMode)
{
  ResetCachedInfo();
  bool Result = false;
  TObjectList * PanelItems = CreatePanelItemList(PanelItem, ItemsNumber);
  {
    std::auto_ptr<TObjectList> PanelItemsPtr;
    PanelItemsPtr.reset(PanelItems);
    Result = DeleteFilesEx(PanelItems, OpMode);
  }

  return Result;
}
//---------------------------------------------------------------------------
intptr_t TCustomFarFileSystem::GetFiles(struct PluginPanelItem * PanelItem,
  int ItemsNumber, int Move, const wchar_t ** DestPath, int OpMode)
{
  ResetCachedInfo();
  TObjectList * PanelItems = CreatePanelItemList(PanelItem, ItemsNumber);
  intptr_t Result = 0;
  FDestPathStr = *DestPath;
  TRY_FINALLY (
  {
    Result = GetFilesEx(PanelItems, Move > 0, FDestPathStr, OpMode);
  }
  ,
  {
    if (FDestPathStr != *DestPath)
    {
      *DestPath = FDestPathStr.c_str();
    }
    delete PanelItems;
  }
  );

  return Result;
}
//---------------------------------------------------------------------------
intptr_t TCustomFarFileSystem::PutFiles(struct PluginPanelItem * PanelItem,
  int ItemsNumber, int Move, const wchar_t * srcPath, int OpMode)
{
  (void)srcPath;
  ResetCachedInfo();
  intptr_t Result = 0;
  TObjectList * PanelItems = CreatePanelItemList(PanelItem, ItemsNumber);
  {
    std::auto_ptr<TObjectList> PanelItemsPtr;
    PanelItemsPtr.reset(PanelItems);
    Result = PutFilesEx(PanelItems, Move > 0, OpMode);
  }

  return Result;
}
//---------------------------------------------------------------------------
void TCustomFarFileSystem::ResetCachedInfo()
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
TFarPanelInfo * TCustomFarFileSystem::GetPanelInfo(int Another)
{
  // DEBUG_PRINTF(L"Another = %d", Another);
  bool bAnother = Another != 0;
  if (FPanelInfo[bAnother] == NULL)
  {
    PanelInfo * Info = static_cast<PanelInfo *>(
      nb_malloc(sizeof(PanelInfo)));
    bool Res = (FPlugin->FarControl(FCTL_GETPANELINFO, 0, reinterpret_cast<intptr_t>(Info),
      !bAnother ? PANEL_ACTIVE : PANEL_PASSIVE) > 0);
    if (!Res)
    {
      memset(Info, 0, sizeof(*Info));
      assert(false);
    }
    // DEBUG_PRINTF(L"Info = %x", Info);
    FPanelInfo[bAnother] = new TFarPanelInfo(Info, !bAnother ? this : NULL);
  }
  return FPanelInfo[bAnother];
}
//---------------------------------------------------------------------------
intptr_t TCustomFarFileSystem::FarControl(uintptr_t Command, intptr_t Param1, intptr_t Param2)
{
  return FPlugin->FarControl(Command, Param1, Param2, this);
}
//---------------------------------------------------------------------------
intptr_t TCustomFarFileSystem::FarControl(uintptr_t Command, intptr_t Param1, intptr_t Param2, HANDLE Plugin)
{
  return FPlugin->FarControl(Command, Param1, Param2, Plugin);
}
//---------------------------------------------------------------------------
bool TCustomFarFileSystem::UpdatePanel(bool ClearSelection, bool Another)
{
  unsigned int PrevInstances = FInstances;
  InvalidateOpenPluginInfo();
  FPlugin->FarControl(FCTL_UPDATEPANEL, !ClearSelection, NULL, Another ? PANEL_PASSIVE : PANEL_ACTIVE);
  return (FInstances >= PrevInstances);
}
//---------------------------------------------------------------------------
void TCustomFarFileSystem::RedrawPanel(bool Another)
{
  FPlugin->FarControl(FCTL_REDRAWPANEL, 0, reinterpret_cast<intptr_t>(static_cast<void *>(0)), Another ? PANEL_PASSIVE : PANEL_ACTIVE);
}
//---------------------------------------------------------------------------
void TCustomFarFileSystem::ClosePlugin()
{
  FClosed = true;
  FarControl(FCTL_CLOSEPLUGIN, 0, NULL);
}
//---------------------------------------------------------------------------
UnicodeString TCustomFarFileSystem::GetMsg(int MsgId)
{
  return FPlugin->GetMsg(MsgId);
}
//---------------------------------------------------------------------------
TCustomFarFileSystem * TCustomFarFileSystem::GetOppositeFileSystem()
{
  return FPlugin->GetPanelFileSystem(true, this);
}
//---------------------------------------------------------------------------
bool TCustomFarFileSystem::IsActiveFileSystem()
{
  // Cannot use PanelInfo::Focus as it occasionally does not work from editor;
  return (this == FPlugin->GetPanelFileSystem());
}
//---------------------------------------------------------------------------
bool TCustomFarFileSystem::IsLeft()
{
  DEBUG_PRINTF(L"IsLeft");
  return (GetPanelInfo(0)->GetBounds().Left <= 0);
}
//---------------------------------------------------------------------------
bool TCustomFarFileSystem::IsRight()
{
  return !IsLeft();
}
//---------------------------------------------------------------------------
bool TCustomFarFileSystem::ProcessHostFileEx(TObjectList * /* PanelItems */, int /* OpMode */)
{
  return false;
}
//---------------------------------------------------------------------------
bool TCustomFarFileSystem::ProcessKeyEx(intptr_t /* Key */, uintptr_t /* ControlState */)
{
  return false;
}
//---------------------------------------------------------------------------
bool TCustomFarFileSystem::ProcessEventEx(intptr_t /* Event */, void * /* Param */)
{
  return false;
}
//---------------------------------------------------------------------------
bool TCustomFarFileSystem::SetDirectoryEx(const UnicodeString & /* Dir */, int /* OpMode */)
{
  return false;
}
//---------------------------------------------------------------------------
intptr_t TCustomFarFileSystem::MakeDirectoryEx(UnicodeString & /* Name */, int /* OpMode */)
{
  return -1;
}
//---------------------------------------------------------------------------
bool TCustomFarFileSystem::DeleteFilesEx(TObjectList * /* PanelItems */, int /* OpMode */)
{
  return false;
}
//---------------------------------------------------------------------------
intptr_t TCustomFarFileSystem::GetFilesEx(TObjectList * /* PanelItems */, bool /* Move */,
  UnicodeString & /* DestPath */, int /* OpMode */)
{
  return 0;
}
//---------------------------------------------------------------------------
intptr_t TCustomFarFileSystem::PutFilesEx(TObjectList * /*PanelItems*/,
  bool /* Move */, int /* OpMode */)
{
  return 0;
}
//---------------------------------------------------------------------------
TObjectList * TCustomFarFileSystem::CreatePanelItemList(
  struct PluginPanelItem * PanelItem, int ItemsNumber)
{
  // DEBUG_PRINTF(L"ItemsNumber = %d", ItemsNumber);
  TObjectList * PanelItems = new TObjectList();
  PanelItems->SetOwnsObjects(true);
  try
  {
    for (intptr_t Index = 0; Index < ItemsNumber; ++Index)
    {
      PanelItems->Add(new TFarPanelItem(&PanelItem[Index], false));
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
TFarPanelModes::TFarPanelModes() : TObject()
{
  memset(&FPanelModes, 0, sizeof(FPanelModes));
  FReferenced = false;
}
//---------------------------------------------------------------------------
TFarPanelModes::~TFarPanelModes()
{
  if (!FReferenced)
  {
    for (intptr_t Index = 0; Index < static_cast<intptr_t>(LENOF(FPanelModes)); ++Index)
    {
      ClearPanelMode(FPanelModes[Index]);
    }
  }
}
//---------------------------------------------------------------------------
void TFarPanelModes::SetPanelMode(size_t Mode, const UnicodeString & ColumnTypes,
  const UnicodeString & ColumnWidths, TStrings * ColumnTitles,
  bool FullScreen, bool DetailedStatus, bool AlignExtensions,
  bool CaseConversion, const UnicodeString & StatusColumnTypes,
  const UnicodeString & StatusColumnWidths)
{
  intptr_t ColumnTypesCount = !ColumnTypes.IsEmpty() ? CommaCount(ColumnTypes) + 1 : 0;
  assert(Mode != NPOS && Mode < LENOF(FPanelModes));
  assert(!ColumnTitles || (ColumnTitles->GetCount() == ColumnTypesCount));

  ClearPanelMode(FPanelModes[Mode]);
  wchar_t ** Titles = static_cast<wchar_t **>(
    nb_malloc(sizeof(wchar_t *) * ColumnTypesCount));
  FPanelModes[Mode].ColumnTypes = TCustomFarPlugin::DuplicateStr(ColumnTypes);
  FPanelModes[Mode].ColumnWidths = TCustomFarPlugin::DuplicateStr(ColumnWidths);
  if (ColumnTitles)
  {
    for (intptr_t Index = 0; Index < ColumnTypesCount; ++Index)
    {
      Titles[Index] = TCustomFarPlugin::DuplicateStr(ColumnTitles->GetString(Index));
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
void TFarPanelModes::ClearPanelMode(PanelMode & Mode)
{
  if (Mode.ColumnTypes)
  {
    intptr_t ColumnTypesCount = Mode.ColumnTypes ?
      CommaCount(UnicodeString(Mode.ColumnTypes)) + 1 : 0;

    nb_free((void *)Mode.ColumnTypes);
    nb_free((void *)Mode.ColumnWidths);
    if (Mode.ColumnTitles)
    {
      for (intptr_t Index = 0; Index < ColumnTypesCount; ++Index)
      {
        nb_free((void *)Mode.ColumnTitles[Index]);
      }
      nb_free((void *)Mode.ColumnTitles);
    }
    nb_free((void *)Mode.StatusColumnTypes);
    nb_free((void *)Mode.StatusColumnWidths);
    memset(&Mode, 0, sizeof(Mode));
  }
}
//---------------------------------------------------------------------------
void TFarPanelModes::FillOpenPluginInfo(struct OpenPluginInfo * Info)
{
  assert(Info);
  Info->PanelModesNumber = LENOF(FPanelModes);
  static PanelMode PanelModesArray[LENOF(FPanelModes)];
  memmove(PanelModesArray, &FPanelModes, sizeof(FPanelModes));
  Info->PanelModesArray = PanelModesArray;
  FReferenced = true;
}
//---------------------------------------------------------------------------
intptr_t TFarPanelModes::CommaCount(const UnicodeString & ColumnTypes)
{
  intptr_t Count = 0;
  for (intptr_t Index = 1; Index <= ColumnTypes.Length(); ++Index)
  {
    if (ColumnTypes[Index] == ',')
    {
      Count++;
    }
  }
  return Count;
}
//---------------------------------------------------------------------------
TFarKeyBarTitles::TFarKeyBarTitles()
{
  memset(&FKeyBarTitles, 0, sizeof(FKeyBarTitles));
  FReferenced = false;
}
//---------------------------------------------------------------------------
TFarKeyBarTitles::~TFarKeyBarTitles()
{
  if (!FReferenced)
  {
    ClearKeyBarTitles(FKeyBarTitles);
  }
}
//---------------------------------------------------------------------------
void TFarKeyBarTitles::ClearFileKeyBarTitles()
{
  ClearKeyBarTitle(fsNone, 3, 8);
  ClearKeyBarTitle(fsCtrl, 4, 11);
  ClearKeyBarTitle(fsAlt, 3, 7);
  ClearKeyBarTitle(fsShift, 1, 8);
  ClearKeyBarTitle(fsCtrlShift, 3, 4);
}
//---------------------------------------------------------------------------
void TFarKeyBarTitles::ClearKeyBarTitle(TFarShiftStatus ShiftStatus,
  intptr_t FunctionKeyStart, intptr_t FunctionKeyEnd)
{
  if (!FunctionKeyEnd)
  {
    FunctionKeyEnd = FunctionKeyStart;
  }
  for (intptr_t Index = FunctionKeyStart; Index <= FunctionKeyEnd; ++Index)
  {
    SetKeyBarTitle(ShiftStatus, Index, L"");
  }
}
//---------------------------------------------------------------------------
void TFarKeyBarTitles::SetKeyBarTitle(TFarShiftStatus ShiftStatus,
  intptr_t FunctionKey, const UnicodeString & Title)
{
  assert(FunctionKey >= 1 && FunctionKey <= static_cast<intptr_t>(LENOF(FKeyBarTitles.Titles)));
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
    nb_free(Titles[FunctionKey-1]);
  }
  Titles[FunctionKey-1] = TCustomFarPlugin::DuplicateStr(Title, true);
}
//---------------------------------------------------------------------------
void TFarKeyBarTitles::ClearKeyBarTitles(KeyBarTitles & Titles)
{
  for (intptr_t Index = 0; Index < static_cast<intptr_t>(LENOF(Titles.Titles)); ++Index)
  {
    nb_free(Titles.Titles[Index]);
    nb_free(Titles.CtrlTitles[Index]);
    nb_free(Titles.AltTitles[Index]);
    nb_free(Titles.ShiftTitles[Index]);
    nb_free(Titles.CtrlShiftTitles[Index]);
    nb_free(Titles.AltShiftTitles[Index]);
    nb_free(Titles.CtrlAltTitles[Index]);
  }
}
//---------------------------------------------------------------------------
void TFarKeyBarTitles::FillOpenPluginInfo(struct OpenPluginInfo * Info)
{
  assert(Info);
  KeyBarTitles * KeyBar = static_cast<KeyBarTitles *>(
    nb_malloc(sizeof(KeyBarTitles)));
  Info->KeyBar = KeyBar;
  memmove(KeyBar, &FKeyBarTitles, sizeof(FKeyBarTitles));
  FReferenced = true;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
UnicodeString TCustomFarPanelItem::GetCustomColumnData(size_t /*Column*/)
{
  assert(false);
  return L"";
}
//---------------------------------------------------------------------------
void TCustomFarPanelItem::FillPanelItem(struct PluginPanelItem * PanelItem)
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

  PanelItem->FindData.lpwszFileName = TCustomFarPlugin::DuplicateStr(FileName);
  // DEBUG_PRINTF(L"PanelItem->FindData.lpwszFileName = %s", PanelItem->FindData.lpwszFileName);
  PanelItem->Description = TCustomFarPlugin::DuplicateStr(Description);
  PanelItem->Owner = TCustomFarPlugin::DuplicateStr(Owner);
  wchar_t ** CustomColumnData = static_cast<wchar_t **>(
    nb_malloc(sizeof(wchar_t *) * PanelItem->CustomColumnNumber));
  for (intptr_t Index = 0; Index < PanelItem->CustomColumnNumber; ++Index)
  {
    CustomColumnData[Index] =
      TCustomFarPlugin::DuplicateStr(GetCustomColumnData(Index));
  }
  PanelItem->CustomColumnData = CustomColumnData;
  // DEBUG_PRINTF(L"end");
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TFarPanelItem::TFarPanelItem(PluginPanelItem * APanelItem, bool OwnsItem):
  TCustomFarPanelItem(),
  FPanelItem(NULL),
  FOwnsItem(false)
{
  assert(APanelItem);
  FPanelItem = APanelItem;
  FOwnsItem = OwnsItem;
}

TFarPanelItem::~TFarPanelItem()
{
  if (FOwnsItem)
    nb_free(FPanelItem);
  FPanelItem = NULL;
}

//---------------------------------------------------------------------------
void TFarPanelItem::GetData(
  DWORD & /*Flags*/, UnicodeString & /*FileName*/, __int64 & /*Size*/,
  DWORD & /*FileAttributes*/,
  TDateTime & /*LastWriteTime*/, TDateTime & /*LastAccess*/,
  DWORD & /*NumberOfLinks*/, UnicodeString & /*Description*/,
  UnicodeString & /*Owner*/, void *& /*UserData*/, int & /*CustomColumnNumber*/)
{
  assert(false);
}
//---------------------------------------------------------------------------
UnicodeString TFarPanelItem::GetCustomColumnData(size_t /*Column*/)
{
  assert(false);
  return L"";
}
//---------------------------------------------------------------------------
uintptr_t TFarPanelItem::GetFlags()
{
  return static_cast<uintptr_t>(FPanelItem->Flags);
}
//---------------------------------------------------------------------------
UnicodeString TFarPanelItem::GetFileName()
{
  UnicodeString Result = FPanelItem->FindData.lpwszFileName;
  return Result;
}
//---------------------------------------------------------------------------
void * TFarPanelItem::GetUserData()
{
  return reinterpret_cast<void *>(FPanelItem->UserData);
}
//---------------------------------------------------------------------------
bool TFarPanelItem::GetSelected()
{
  return (FPanelItem->Flags & PPIF_SELECTED) != 0;
}
//---------------------------------------------------------------------------
void TFarPanelItem::SetSelected(bool Value)
{
  if (Value)
  {
    FPanelItem->Flags |= PPIF_SELECTED;
  }
  else
  {
    FPanelItem->Flags &= ~PPIF_SELECTED;
  }
}
//---------------------------------------------------------------------------
uintptr_t TFarPanelItem::GetFileAttributes()
{
  return static_cast<uintptr_t>(FPanelItem->FindData.dwFileAttributes);
}
//---------------------------------------------------------------------------
bool TFarPanelItem::GetIsParentDirectory()
{
  return (GetFileName() == PARENTDIRECTORY);
}
//---------------------------------------------------------------------------
bool TFarPanelItem::GetIsFile()
{
  return (GetFileAttributes() & FILE_ATTRIBUTE_DIRECTORY) == 0;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
THintPanelItem::THintPanelItem(const UnicodeString & AHint) :
  TCustomFarPanelItem()
{
  FHint = AHint;
}
//---------------------------------------------------------------------------
void THintPanelItem::GetData(
  DWORD & /*Flags*/, UnicodeString & FileName, __int64 & /*Size*/,
  DWORD & /*FileAttributes*/,
  TDateTime & /*LastWriteTime*/, TDateTime & /*LastAccess*/,
  DWORD & /*NumberOfLinks*/, UnicodeString & /*Description*/,
  UnicodeString & /*Owner*/, void *& /*UserData*/, int & /*CustomColumnNumber*/)
{
  FileName = FHint;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TFarPanelInfo::TFarPanelInfo(PanelInfo * APanelInfo, TCustomFarFileSystem * AOwner):
  TObject(),
  FPanelInfo(APanelInfo),
  FItems(NULL),
  FOwner(AOwner)
{
  // if (!FPanelInfo) throw ExtException(L"");
  assert(FPanelInfo);
}
//---------------------------------------------------------------------------
TFarPanelInfo::~TFarPanelInfo()
{
  nb_free(FPanelInfo);
  delete FItems;
}
//---------------------------------------------------------------------------
intptr_t TFarPanelInfo::GetItemCount()
{
  return static_cast<intptr_t>(FPanelInfo->ItemsNumber);
}
//---------------------------------------------------------------------------
TRect TFarPanelInfo::GetBounds()
{
  RECT rect = FPanelInfo->PanelRect;
  return TRect(rect.left, rect.top, rect.right, rect.bottom);
}
//---------------------------------------------------------------------------
intptr_t TFarPanelInfo::GetSelectedCount()
{
  intptr_t Count = static_cast<intptr_t>(FPanelInfo->SelectedItemsNumber);

  if ((Count == 1) && FOwner)
  {
    intptr_t size = FOwner->FarControl(FCTL_GETSELECTEDPANELITEM, 0, NULL);
    // DEBUG_PRINTF(L"size1 = %d, sizeof(PluginPanelItem) = %d", size, sizeof(PluginPanelItem));
    PluginPanelItem * ppi = static_cast<PluginPanelItem *>(nb_malloc(size));
    memset(ppi, 0, size);
    FOwner->FarControl(FCTL_GETSELECTEDPANELITEM, 0, reinterpret_cast<intptr_t>(ppi));
    if ((ppi->Flags & PPIF_SELECTED) == 0)
    {
      // DEBUG_PRINTF(L"ppi->Flags = %x", ppi->Flags);
      Count = 0;
    }
    nb_free(ppi);
  }

  return Count;
}
//---------------------------------------------------------------------------
TObjectList * TFarPanelInfo::GetItems()
{
  if (!FItems)
  {
    FItems = new TObjectList();
  }
  // DEBUG_PRINTF(L"FPanelInfo->ItemsNumber = %d", FPanelInfo->ItemsNumber);
  if (FOwner)
  {
    for (intptr_t Index = 0; Index < FPanelInfo->ItemsNumber; ++Index)
    {
      // DEBUG_PRINTF(L"Index = %d", Index);
      // TODO: move to common function
      intptr_t size = FOwner->FarControl(FCTL_GETPANELITEM, Index, NULL);
      PluginPanelItem * ppi = static_cast<PluginPanelItem *>(nb_malloc(size));
      memset(ppi, 0, size);
      FOwner->FarControl(FCTL_GETPANELITEM, Index, reinterpret_cast<intptr_t>(ppi));
      // DEBUG_PRINTF(L"ppi.FileName = %s", ppi->FindData.lpwszFileName);
      FItems->Add(new TFarPanelItem(ppi, true));
    }
  }
  return FItems;
}
//---------------------------------------------------------------------------
TFarPanelItem * TFarPanelInfo::FindFileName(const UnicodeString & FileName)
{
  TObjectList * AItems = GetItems();
  TFarPanelItem * PanelItem;
  for (intptr_t Index = 0; Index < AItems->GetCount(); ++Index)
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
TFarPanelItem * TFarPanelInfo::FindUserData(void * UserData)
{
  TObjectList * AItems = GetItems();
  TFarPanelItem * PanelItem;
  for (intptr_t Index = 0; Index < AItems->GetCount(); ++Index)
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
void TFarPanelInfo::ApplySelection()
{
  // for "another panel info", there's no owner
  assert(FOwner != NULL);
  FOwner->FarControl(FCTL_SETSELECTION, 0, reinterpret_cast<intptr_t>(FPanelInfo));
}
//---------------------------------------------------------------------------
TFarPanelItem * TFarPanelInfo::GetFocusedItem()
{
  intptr_t Index = GetFocusedIndex();
  TObjectList * Items = GetItems();
  // DEBUG_PRINTF(L"Index = %d, Items = %x, Items->GetCount() = %d", Index, Items, Items->Count);
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
void TFarPanelInfo::SetFocusedItem(TFarPanelItem * Value)
{
  TObjectList * Items = GetItems();
  intptr_t Index = Items->IndexOf(static_cast<TObject *>(Value));
  assert(Index != NPOS);
  SetFocusedIndex(Index);
  // delete Items;
}
//---------------------------------------------------------------------------
intptr_t TFarPanelInfo::GetFocusedIndex()
{
  return static_cast<intptr_t>(FPanelInfo->CurrentItem);
}
//---------------------------------------------------------------------------
void TFarPanelInfo::SetFocusedIndex(intptr_t Value)
{
  // for "another panel info", there's no owner
  assert(FOwner != NULL);
  // DEBUG_PRINTF(L"GetFocusedIndex = %d, Value = %d", GetFocusedIndex(), Value);
  if (GetFocusedIndex() != Value)
  {
    assert(Value != NPOS && Value < FPanelInfo->ItemsNumber);
    FPanelInfo->CurrentItem = static_cast<int>(Value);
    PanelRedrawInfo PanelInfo;
    PanelInfo.CurrentItem = FPanelInfo->CurrentItem;
    PanelInfo.TopPanelItem = FPanelInfo->TopPanelItem;
    FOwner->FarControl(FCTL_REDRAWPANEL, 0, reinterpret_cast<intptr_t>(&PanelInfo));
  }
}
//---------------------------------------------------------------------------
TFarPanelType TFarPanelInfo::GetType()
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
bool TFarPanelInfo::GetIsPlugin()
{
  return (FPanelInfo->Plugin != 0);
}
//---------------------------------------------------------------------------
UnicodeString TFarPanelInfo::GetCurrentDirectory()
{
  UnicodeString Result = L"";
  intptr_t Size = FarPlugin->FarControl(FCTL_GETPANELDIR,
                                      0,
                                      NULL,
                                      FOwner != NULL ? PANEL_ACTIVE : PANEL_PASSIVE);
  if (Size)
  {
    Result.SetLength(Size);
    FarPlugin->FarControl(FCTL_GETPANELDIR,
      Size,
      reinterpret_cast<intptr_t>(Result.c_str()),
      FOwner != NULL ? PANEL_ACTIVE : PANEL_PASSIVE);
  }
  return Result.c_str();
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TFarMenuItems::TFarMenuItems() :
  TStringList()
{
  FItemFocused = NPOS;
}
//---------------------------------------------------------------------------
void TFarMenuItems::Clear()
{
  FItemFocused = NPOS;
  TStringList::Clear();
}
//---------------------------------------------------------------------------
void TFarMenuItems::Delete(intptr_t Index)
{
  if (Index == FItemFocused)
  {
    FItemFocused = NPOS;
  }
  TStringList::Delete(Index);
}
//---------------------------------------------------------------------------
void TFarMenuItems::PutObject(intptr_t Index, TObject * AObject)
{
  TStringList::PutObject(Index, AObject);
  bool Focused = (reinterpret_cast<uintptr_t>(AObject) & MIF_SEPARATOR) != 0;
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
intptr_t TFarMenuItems::Add(const UnicodeString & Text, bool Visible)
{
  intptr_t Result = TStringList::Add(Text);
  if (!Visible)
  {
    SetFlag(GetCount() - 1, MIF_HIDDEN, true);
  }
  return Result;
}
//---------------------------------------------------------------------------
void TFarMenuItems::AddSeparator(bool Visible)
{
  Add(L"");
  SetFlag(GetCount() - 1, MIF_SEPARATOR, true);
  if (!Visible)
  {
    SetFlag(GetCount() - 1, MIF_HIDDEN, true);
  }
}
//---------------------------------------------------------------------------
void TFarMenuItems::SetItemFocused(intptr_t Value)
{
  if (GetItemFocused() != Value)
  {
    if (GetItemFocused() != NPOS)
    {
      SetFlag(GetItemFocused(), MIF_SELECTED, false);
    }
    FItemFocused = Value;
    SetFlag(GetItemFocused(), MIF_SELECTED, true);
  }
}
//---------------------------------------------------------------------------
void TFarMenuItems::SetFlag(intptr_t Index, uintptr_t Flag, bool Value)
{
  if (GetFlag(Index, Flag) != Value)
  {
    uintptr_t F = reinterpret_cast<uintptr_t>(Objects[Index]);
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
bool TFarMenuItems::GetFlag(intptr_t Index, uintptr_t Flag)
{
  return (reinterpret_cast<uintptr_t>(Objects[Index]) & Flag) > 0;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TFarEditorInfo::TFarEditorInfo(EditorInfo * Info) :
  FEditorInfo(Info)
{
}
//---------------------------------------------------------------------------
TFarEditorInfo::~TFarEditorInfo()
{
  nb_free(FEditorInfo);
}
//---------------------------------------------------------------------------
intptr_t TFarEditorInfo::GetEditorID() const
{
  return static_cast<intptr_t>(FEditorInfo->EditorID);
}
//---------------------------------------------------------------------------
UnicodeString TFarEditorInfo::GetFileName()
{
  UnicodeString Result = L"";
  intptr_t buffLen = FarPlugin->FarEditorControl(ECTL_GETFILENAME, NULL);
  if (buffLen)
  {
    Result.SetLength(buffLen + 1);
    FarPlugin->FarEditorControl(ECTL_GETFILENAME, const_cast<wchar_t *>(Result.c_str()));
  }
  return Result.c_str();
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TFarEnvGuard::TFarEnvGuard()
{
  assert(FarPlugin != NULL);
}
//---------------------------------------------------------------------------
TFarEnvGuard::~TFarEnvGuard()
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
TFarPluginEnvGuard::TFarPluginEnvGuard()
{
  assert(FarPlugin != NULL);
}
//---------------------------------------------------------------------------
TFarPluginEnvGuard::~TFarPluginEnvGuard()
{
  assert(FarPlugin != NULL);
}
//---------------------------------------------------------------------------
void FarWrapText(const UnicodeString & Text, TStrings * Result, intptr_t MaxWidth)
{
  size_t TabSize = 8;
  TStringList Lines;
  Lines.Text = Text;
  TStringList WrappedLines;
  for (intptr_t Index = 0; Index < Lines.GetCount(); ++Index)
  {
    UnicodeString WrappedLine = Lines.GetString(Index);
    if (!WrappedLine.IsEmpty())
    {
      WrappedLine = ::ReplaceChar(WrappedLine, '\'', '\3');
      WrappedLine = ::ReplaceChar(WrappedLine, '\"', '\4');
      WrappedLine = Sysutils::WrapText(WrappedLine, MaxWidth);
      WrappedLine = ::ReplaceChar(WrappedLine, '\3', '\'');
      WrappedLine = ::ReplaceChar(WrappedLine, '\4', '\"');
      WrappedLines.Text = WrappedLine;
      for (intptr_t WrappedIndex = 0; WrappedIndex < WrappedLines.GetCount(); ++WrappedIndex)
      {
        UnicodeString FullLine = WrappedLines.GetString(WrappedIndex);
        do
        {
          // WrapText does not wrap when not possible, enforce it
          // (it also does not wrap when the line is longer than maximum only
          // because of trailing dot or similar)
          UnicodeString Line = FullLine.SubString(1, MaxWidth);
          FullLine.Delete(1, MaxWidth);

          intptr_t P = 0;
          while ((P = Line.Pos(L'\t')) > 0)
          {
            Line.Delete(P, 1);
            Line.Insert(::StringOfChar(' ',
                ((P / TabSize) + ((P % TabSize) > 0 ? 1 : 0)) * TabSize - P + 1),
              P);
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
