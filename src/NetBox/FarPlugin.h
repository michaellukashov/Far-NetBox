#pragma once

#include "boostdefines.hpp"

#pragma warning(push, 1)
#include "Classes.h"
#include "SysUtils.h"
#pragma warning(pop)
#include "Common.h"
#include "guid.h"
#include "plugin.hpp"




//---------------------------------------------------------------------------
#define RMASK (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED | RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED | SHIFT_PRESSED)
#define ALTMASK (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED)
#define CTRLMASK (RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED)
#define SHIFTMASK (SHIFT_PRESSED)
//---------------------------------------------------------------------------
class TCustomFarFileSystem;
class TFarPanelModes;
class TFarKeyBarTitles;
class TFarPanelInfo;
class TFarDialog;
class TWinSCPFileSystem;
class TFarDialogItem;
class TCriticalSection;
class TFarMessageDialog;
class TFarEditorInfo;
class TFarPluginGuard;
//---------------------------------------------------------------------------
const int MaxMessageWidth = 64;
//---------------------------------------------------------------------------
enum TFarShiftStatus { fsNone, fsCtrl, fsAlt, fsShift, fsCtrlShift,
  fsAltShift, fsCtrlAlt };
enum THandlesFunction { hfProcessKey, hfProcessHostFile, hfProcessPanelEvent };
#ifndef _MSC_VER
typedef void __fastcall (__closure * TFarInputBoxValidateEvent)
  (AnsiString & Text);
#else
typedef fastdelegate::FastDelegate1<void,
  UnicodeString &> TFarInputBoxValidateEvent;
#endif
//---------------------------------------------------------------------------
#ifndef _MSC_VER
typedef void __fastcall (__closure *TFarMessageTimerEvent)(unsigned int & Result);
typedef void __fastcall (__closure *TFarMessageClickEvent)(void * Token, int Result, bool & Close);
#else
typedef fastdelegate::FastDelegate1<void, unsigned int &> TFarMessageTimerEvent;
typedef fastdelegate::FastDelegate3<void, void *, int, bool &> TFarMessageClickEvent;
#endif

//---------------------------------------------------------------------------
struct TFarMessageParams
{
  TFarMessageParams();

  TStrings * MoreMessages;
  UnicodeString CheckBoxLabel;
  bool CheckBox;
  unsigned int Timer;
  unsigned int TimerAnswer;
  TFarMessageTimerEvent TimerEvent;
  unsigned int Timeout;
  unsigned int TimeoutButton;
  UnicodeString TimeoutStr;
  TFarMessageClickEvent ClickEvent;
  void * Token;
};
//---------------------------------------------------------------------------
enum NetBoxSystemSettings
{
    NBSS_DELETETORECYCLEBIN             = 0x00000002,
    // NBSS_USESYSTEMCOPYROUTINE           = 0x00000004,
    // NBSS_COPYFILESOPENEDFORWRITING      = 0x00000008,
    // NBSS_CREATEFOLDERSINUPPERCASE       = 0x00000010,
    // NBSS_SAVECOMMANDSHISTORY            = 0x00000020,
    // NBSS_SAVEFOLDERSHISTORY             = 0x00000040,
    // NBSS_SAVEVIEWANDEDITHISTORY         = 0x00000080,
    // NBSS_USEWINDOWSREGISTEREDTYPES      = 0x00000100,
    // NBSS_AUTOSAVESETUP                  = 0x00000200,
    // NBSS_SCANSYMLINK                    = 0x00000400,
};
//---------------------------------------------------------------------------
class TCustomFarPlugin : public TObject
{
  friend TCustomFarFileSystem;
  friend TFarDialog;
  friend TWinSCPFileSystem;
  friend TFarDialogItem;
  friend TFarMessageDialog;
  friend TFarPluginGuard;
public:
  explicit /* __fastcall */ TCustomFarPlugin(HINSTANCE HInst);
  virtual /* __fastcall */ ~TCustomFarPlugin();
  virtual VersionInfo __fastcall GetMinFarVersion();
  virtual void __fastcall SetStartupInfo(const struct PluginStartupInfo * Info);
  virtual struct PluginStartupInfo * GetStartupInfo() { return &FStartupInfo; }
  virtual void __fastcall ExitFAR();
  virtual void __fastcall GetPluginInfo(struct PluginInfo * Info);
  virtual int __fastcall Configure(const struct ConfigureInfo *Info);
  virtual void * __fastcall OpenPlugin(const struct OpenInfo *Info);
  virtual void __fastcall ClosePanel(void *Plugin);
  virtual void __fastcall GetOpenPanelInfo(struct OpenPanelInfo *Info);
  virtual int __fastcall GetFindData(struct GetFindDataInfo *Info);

  virtual void __fastcall FreeFindData(const struct FreeFindDataInfo *Info);

  virtual int __fastcall ProcessHostFile(const struct ProcessHostFileInfo *Info);


  virtual int __fastcall ProcessPanelInput(const struct ProcessPanelInputInfo *Info);

  virtual int __fastcall ProcessPanelEvent(const struct ProcessPanelEventInfo *Info);


  virtual int __fastcall SetDirectory(const struct SetDirectoryInfo *Info);
  virtual int __fastcall MakeDirectory(struct MakeDirectoryInfo *Info);
  virtual int __fastcall DeleteFiles(const struct DeleteFilesInfo *Info);
  virtual int __fastcall GetFiles(struct GetFilesInfo *Info);

  virtual int __fastcall PutFiles(const struct PutFilesInfo *Info);

  virtual int __fastcall ProcessEditorEvent(const struct ProcessEditorEventInfo *Info);

  virtual int __fastcall ProcessEditorInput(const struct ProcessEditorInputInfo *Info);

  virtual void __fastcall HandleException(Exception * E, int OpMode = 0);

  static wchar_t * DuplicateStr(const UnicodeString Str, bool AllowEmpty = false);
  int __fastcall Message(unsigned int Flags, const UnicodeString Title,
    const UnicodeString Message, TStrings * Buttons = NULL,
    TFarMessageParams * Params = NULL);
  int __fastcall MaxMessageLines();
  int __fastcall MaxMenuItemLength();
  int __fastcall Menu(unsigned int Flags, const UnicodeString Title,
    const UnicodeString Bottom, TStrings * Items, const FarKey * BreakKeys,
    int & BreakCode);
  int __fastcall Menu(unsigned int Flags, const UnicodeString Title,
    const UnicodeString Bottom, TStrings * Items);
  int __fastcall Menu(unsigned int Flags, const UnicodeString Title,
    const UnicodeString Bottom, const FarMenuItem * Items, int Count,
    const FarKey * BreakKeys, int & BreakCode);
  bool __fastcall InputBox(const UnicodeString Title, const UnicodeString Prompt,
    UnicodeString & Text, unsigned long Flags, const UnicodeString HistoryName = L"",
    size_t MaxLen = 255, TFarInputBoxValidateEvent OnValidate = NULL);
  UnicodeString __fastcall GetMsg(int MsgId);
  void __fastcall SaveScreen(HANDLE & Screen);
  void __fastcall RestoreScreen(HANDLE & Screen);
  bool __fastcall CheckForEsc();
  bool __fastcall Viewer(const UnicodeString FileName, unsigned int Flags,
    UnicodeString Title = L"");
  bool __fastcall Editor(const UnicodeString FileName, unsigned int Flags,
    UnicodeString Title = L"");
  DWORD __fastcall FarControl(FILE_CONTROL_COMMANDS Command, int Param1, void * Param2, HANDLE Plugin = INVALID_HANDLE_VALUE);
  __int64 __fastcall FarAdvControl(ADVANCED_CONTROL_COMMANDS Command, int Param1, void *Param2 = NULL);
  int __fastcall FarEditorControl(EDITOR_CONTROL_COMMANDS Command, void * Param);
  __int64 __fastcall FarSystemSettings();
  void __fastcall Text(int X, int Y, int Color, const UnicodeString Str);
  void __fastcall FlushText();
  void __fastcall WriteConsole(const UnicodeString Str);
  void __fastcall FarCopyToClipboard(const UnicodeString Str);
  void __fastcall FarCopyToClipboard(TStrings * Strings);
  int __fastcall FarVersion();
  UnicodeString __fastcall FormatFarVersion(VersionInfo & Info);
  UnicodeString __fastcall TemporaryDir();
  int __fastcall InputRecordToKey(const INPUT_RECORD * Rec);
  TFarEditorInfo * __fastcall EditorInfo();

  void __fastcall ShowConsoleTitle(const UnicodeString Title);
  void __fastcall ClearConsoleTitle();
  void __fastcall UpdateConsoleTitle(const UnicodeString Title);
  void __fastcall UpdateConsoleTitleProgress(short Progress);
  void __fastcall ShowTerminalScreen();
  void __fastcall SaveTerminalScreen();
  void __fastcall ScrollTerminalScreen(int Rows);
  TPoint __fastcall TerminalInfo(TPoint * Size = NULL, TPoint * Cursor = NULL);
  unsigned int ConsoleWindowState();
  void __fastcall ToggleVideoMode();

  TCustomFarFileSystem * __fastcall GetPanelFileSystem(bool Another = false,
    HANDLE Plugin = INVALID_HANDLE_VALUE);

#ifndef _MSC_VER
  __property AnsiString ModuleName = { read = GetModuleName };
  __property TFarDialog * TopDialog = { read = FTopDialog };
  __property HWND Handle = { read = FHandle };
  __property bool ANSIApis = { read = FANSIApis };
  __property unsigned int FarThread = { read = FFarThread };
#else
  UnicodeString __fastcall GetModuleName();
  TFarDialog * __fastcall GetTopDialog() const { return FTopDialog; }
  HINSTANCE GetHandle() const { return FHandle; };
  bool GetANSIApis() const { return FANSIApis; };
  unsigned int GetFarThread() const { return FFarThread; };
  FarStandardFunctions & GetFarStandardFunctions() { return FFarStandardFunctions; }
  const struct PluginStartupInfo * __fastcall GetStartupInfo() const { return &FStartupInfo; }
#endif
protected:
  PluginStartupInfo FStartupInfo;
  FarStandardFunctions FFarStandardFunctions;
  HINSTANCE FHandle;
  bool FANSIApis;
  TObjectList * FOpenedPlugins;
  TFarDialog * FTopDialog;
  HANDLE FConsoleInput;
  HANDLE FConsoleOutput;
  int FFarVersion;
  bool FTerminalScreenShowing;
  TCriticalSection * FCriticalSection;
  unsigned int FFarThread;
  // bool FOldFar;
  bool FValidFarSystemSettings;
  INT_PTR FFarSystemSettings;
  TPoint FNormalConsoleSize;
  TCustomFarPlugin * Self;

  virtual bool __fastcall HandlesFunction(THandlesFunction Function);
  virtual void __fastcall GetPluginInfoEx(PLUGIN_FLAGS & Flags,
    TStrings * DiskMenuStrings, TStrings * PluginMenuStrings,
    TStrings * PluginConfigStrings, TStrings * CommandPrefixes) = 0;
  virtual TCustomFarFileSystem * __fastcall OpenPluginEx(OPENFROM OpenFrom, INT_PTR Item) = 0;
  virtual bool __fastcall ImportSessions() = 0;
  virtual bool __fastcall ConfigureEx(int Item) = 0;
  virtual int __fastcall ProcessEditorEventEx(const struct ProcessEditorEventInfo *Info) = 0;
  virtual int __fastcall ProcessEditorInputEx(const INPUT_RECORD * Rec) = 0;
  virtual void __fastcall HandleFileSystemException(TCustomFarFileSystem * FileSystem,
    Exception * E, int OpMode = 0);
  // virtual bool __fastcall IsOldFar();
  // virtual void __fastcall OldFar();
  void __fastcall ResetCachedInfo();
  int __fastcall MaxLength(TStrings * Strings);
  int __fastcall FarMessage(unsigned int Flags,
    const UnicodeString Title, const UnicodeString Message, TStrings * Buttons,
    TFarMessageParams * Params);
  int __fastcall DialogMessage(unsigned int Flags,
    const UnicodeString Title, const UnicodeString Message, TStrings * Buttons,
    TFarMessageParams * Params);
  void __fastcall InvalidateOpenPanelInfo();

#ifndef _MSC_VER
  __property TCriticalSection * CriticalSection = { read = FCriticalSection };
#else
  TCriticalSection * __fastcall GetCriticalSection() const { return FCriticalSection; }
#endif

#ifdef NETBOX_DEBUG
public:
  void RunTests();
#endif
private:
  void __fastcall UpdateProgress(int state, int progress);
  __int64 __fastcall GetSystemSetting(HANDLE & Settings, const wchar_t * Name);

private:
  PluginInfo FPluginInfo;
  TStringList * FSavedTitles;
  UnicodeString FCurrentTitle;
  short FCurrentProgress;

  void __fastcall ClearPluginInfo(PluginInfo & Info);
  // UnicodeString __fastcall GetModuleName();
  void __fastcall UpdateConsoleTitle();
  UnicodeString __fastcall FormatConsoleTitle();
  HWND __fastcall GetConsoleWindow();
  RECT __fastcall GetPanelBounds(HANDLE PanelHandle);
  bool CompareRects(const RECT & lhs, const RECT & rhs) const
  {
    return
      lhs.left == rhs.left &&
      lhs.top == rhs.top &&
      lhs.right == rhs.right &&
      lhs.bottom == rhs.bottom;
  }
};
//---------------------------------------------------------------------------
class TCustomFarFileSystem : public TObject
{
friend TFarPanelInfo;
friend TCustomFarPlugin;
public:
  /* __fastcall */ TCustomFarFileSystem(TCustomFarPlugin * APlugin);
  virtual void __fastcall Init();
  virtual /* __fastcall */ ~TCustomFarFileSystem();

  void __fastcall GetOpenPanelInfo(struct OpenPanelInfo *Info);
  int __fastcall GetFindData(struct GetFindDataInfo *Info);
  void __fastcall FreeFindData(const struct FreeFindDataInfo *Info);
  int __fastcall ProcessHostFile(const struct ProcessHostFileInfo *Info);
  int __fastcall ProcessPanelInput(const struct ProcessPanelInputInfo *Info);
  int __fastcall  ProcessPanelEvent(int Event, void *Param);
  int __fastcall SetDirectory(const struct SetDirectoryInfo *Info);
  int __fastcall MakeDirectory(struct MakeDirectoryInfo *Info);
  int __fastcall DeleteFiles(const struct DeleteFilesInfo *Info);
  int __fastcall GetFiles(struct GetFilesInfo *Info);
  int __fastcall PutFiles(const struct PutFilesInfo *Info);
  virtual void __fastcall Close();

protected:
  TCustomFarPlugin * FPlugin;
  bool FClosed;

  virtual void __fastcall GetOpenPanelInfoEx(OPENPANELINFO_FLAGS & Flags,
    UnicodeString & HostFile, UnicodeString & CurDir, UnicodeString & Format,
    UnicodeString & PanelTitle, TFarPanelModes * PanelModes, int & StartPanelMode,
    OPENPANELINFO_SORTMODES & StartSortMode, bool & StartSortOrder, TFarKeyBarTitles * KeyBarTitles,
    UnicodeString & ShortcutData) = 0;
  virtual bool __fastcall GetFindDataEx(TObjectList * PanelItems, int OpMode) = 0;
  virtual bool __fastcall ProcessHostFileEx(TObjectList * PanelItems, int OpMode);
  virtual bool __fastcall ProcessKeyEx(WORD Key, DWORD ControlState);
  virtual bool __fastcall ProcessPanelEventEx(int Event, void *Param);
  virtual bool __fastcall SetDirectoryEx(const UnicodeString Dir, int OpMode);
  virtual int __fastcall MakeDirectoryEx(UnicodeString & Name, int OpMode);
  virtual bool __fastcall DeleteFilesEx(TObjectList * PanelItems, int OpMode);
  virtual int __fastcall GetFilesEx(TObjectList * PanelItems, bool Move,
    UnicodeString & DestPath, int OpMode);
  virtual int __fastcall PutFilesEx(TObjectList * PanelItems, bool Move, int OpMode);

  void __fastcall ResetCachedInfo();
  DWORD __fastcall FarControl(FILE_CONTROL_COMMANDS Command, int Param1, void * Param2);
  DWORD __fastcall FarControl(FILE_CONTROL_COMMANDS Command, int Param1, void * Param2, HANDLE Plugin);
  bool __fastcall UpdatePanel(bool ClearSelection = false, bool Another = false);
  void __fastcall RedrawPanel(bool Another = false);
  void __fastcall ClosePanel();
  UnicodeString __fastcall GetMsg(int MsgId);
  TCustomFarFileSystem * __fastcall GetOppositeFileSystem();
  bool __fastcall IsActiveFileSystem();
  bool __fastcall IsLeft();
  bool __fastcall IsRight();

  virtual void __fastcall HandleException(Exception * E, int OpMode = 0);

#ifndef _MSC_VER
  __property TFarPanelInfo * PanelInfo = { read = GetPanelInfo, index = 0 };
  __property TFarPanelInfo * AnotherPanelInfo = { read = GetPanelInfo, index = 1 };
  __property TCriticalSection * CriticalSection = { read = FCriticalSection };
#else
  TFarPanelInfo * GetPanelInfo() { return GetPanelInfo(0); };
  TFarPanelInfo * GetAnotherPanelInfo() { return GetPanelInfo(1); };
  TCriticalSection * GetCriticalSection() { return FCriticalSection; };
#endif

protected:
  TCriticalSection * FCriticalSection;
  void __fastcall InvalidateOpenPanelInfo();

private:
  OpenPanelInfo FOpenPanelInfo;
  bool FOpenPanelInfoValid;
  TFarPanelInfo * FPanelInfo[2];
  static unsigned int FInstances;

  void __fastcall ClearOpenPanelInfo(OpenPanelInfo & Info);
  TObjectList * __fastcall CreatePanelItemList(struct PluginPanelItem * PanelItem,
    int ItemsNumber);
  TFarPanelInfo * __fastcall GetPanelInfo(int Another);
};
//---------------------------------------------------------------------------
#define PANEL_MODES_COUNT 10
class TFarPanelModes : public TObject
{
  friend class TCustomFarFileSystem;
public:
  void __fastcall SetPanelMode(size_t Mode, const UnicodeString ColumnTypes = L"",
    const UnicodeString ColumnWidths = L"", TStrings * ColumnTitles = NULL,
    bool FullScreen = false, bool DetailedStatus = true, bool AlignExtensions = true,
    bool CaseConversion = true, const UnicodeString StatusColumnTypes = L"",
    const UnicodeString StatusColumnWidths = L"");

private:
  PanelMode FPanelModes[PANEL_MODES_COUNT];
  bool FReferenced;

  /* __fastcall */ TFarPanelModes();
  virtual /* __fastcall */ ~TFarPanelModes();

  void __fastcall FillOpenPanelInfo(struct OpenPanelInfo *Info);
  void __fastcall SetFlag(PANELMODE_FLAGS & Flags, bool value, PANELMODE_FLAGS Flag);
  static void __fastcall ClearPanelMode(PanelMode & Mode);
  static int __fastcall CommaCount(const UnicodeString ColumnTypes);
};
//---------------------------------------------------------------------------
class TFarKeyBarTitles : public TObject
{
friend class TCustomFarFileSystem;
public:
  void __fastcall ClearFileKeyBarTitles();
  void __fastcall ClearKeyBarTitle(TFarShiftStatus ShiftStatus,
    int FunctionKeyStart, int FunctionKeyEnd = 0);
  void __fastcall SetKeyBarTitle(TFarShiftStatus ShiftStatus, int FunctionKey,
    const UnicodeString Title);

private:
  KeyBarTitles FKeyBarTitles;
  bool FReferenced;

  /* __fastcall */ TFarKeyBarTitles();
  virtual /* __fastcall */ ~TFarKeyBarTitles();

  void __fastcall FillOpenPanelInfo(struct OpenPanelInfo *Info);
  static void __fastcall ClearKeyBarTitles(KeyBarTitles & Titles);
};
//---------------------------------------------------------------------------
class TCustomFarPanelItem : public TObject
{
  friend class TCustomFarFileSystem;
public:

protected:
  virtual ~TCustomFarPanelItem()
  {}
  virtual void __fastcall GetData(
    unsigned __int64 & Flags, UnicodeString & FileName, __int64 & Size,
    unsigned long & FileAttributes,
    TDateTime & LastWriteTime, TDateTime & LastAccess,
    unsigned long & NumberOfLinks, UnicodeString & Description,
    UnicodeString & Owner, void *& UserData, size_t & CustomColumnNumber) = 0;
  virtual UnicodeString __fastcall GetCustomColumnData(int Column);

  void __fastcall FillPanelItem(struct PluginPanelItem * PanelItem);
};
//---------------------------------------------------------------------------
class TFarPanelItem : public TCustomFarPanelItem
{
public:
  explicit /* __fastcall */ TFarPanelItem(PluginPanelItem * APanelItem);
  virtual /* __fastcall */ ~TFarPanelItem();
#ifndef _MSC_VER
  __property unsigned long Flags = { read = GetFlags };
  __property unsigned long FileAttributes = { read = GetFileAttributes };
  __property AnsiString FileName = { read = GetFileName };
  __property void * UserData = { read = GetUserData };
  __property bool Selected = { read = GetSelected, write = SetSelected };
  __property bool IsParentDirectory = { read = GetIsParentDirectory };
  __property bool IsFile = { read = GetIsFile };
#endif

protected:
  PluginPanelItem * FPanelItem;

  virtual void __fastcall GetData(
    unsigned __int64 & Flags, UnicodeString & FileName, __int64 & Size,
    unsigned long & FileAttributes,
    TDateTime & LastWriteTime, TDateTime & LastAccess,
    unsigned long & NumberOfLinks, UnicodeString & Description,
    UnicodeString & Owner, void *& UserData, size_t & CustomColumnNumber);
  virtual UnicodeString __fastcall GetCustomColumnData(int Column);

public:
  unsigned long __fastcall GetFlags();
  UnicodeString __fastcall GetFileName();
  void * __fastcall GetUserData();
  bool __fastcall GetSelected();
  void __fastcall SetSelected(bool value);
  bool __fastcall GetIsParentDirectory();
  bool __fastcall GetIsFile();
  unsigned long __fastcall GetFileAttributes();
};
//---------------------------------------------------------------------------
class THintPanelItem : public TCustomFarPanelItem
{
public:
  explicit /* __fastcall */ THintPanelItem(const UnicodeString AHint);
  virtual /* __fastcall */ ~THintPanelItem() {}

protected:
  virtual void __fastcall GetData(
    unsigned __int64 & Flags, UnicodeString & FileName, __int64 & Size,
    unsigned long & FileAttributes,
    TDateTime & LastWriteTime, TDateTime & LastAccess,
    unsigned long & NumberOfLinks, UnicodeString & Description,
    UnicodeString & Owner, void *& UserData, size_t & CustomColumnNumber);

private:
  UnicodeString FHint;
};
//---------------------------------------------------------------------------
enum TFarPanelType { ptFile, ptTree, ptQuickView, ptInfo };
//---------------------------------------------------------------------------
class TFarPanelInfo : public TObject
{
public:
  explicit /* __fastcall */ TFarPanelInfo(PanelInfo * APanelInfo, TCustomFarFileSystem * AOwner);
  virtual /* __fastcall */ ~TFarPanelInfo();

#ifndef _MSC_VER
  __property TList * Items = { read = GetItems };
  __property int ItemCount = { read = GetItemCount };
  __property int SelectedCount = { read = GetSelectedCount };
  __property TFarPanelItem * FocusedItem = { read = GetFocusedItem, write = SetFocusedItem };
  __property int FocusedIndex = { read = GetFocusedIndex, write = SetFocusedIndex };
  __property TRect Bounds = { read = GetBounds };
  __property TFarPanelType Type = { read = GetType };
  __property bool IsPlugin = { read = GetIsPlugin };
  __property AnsiString CurrentDirectory = { read = GetCurrentDirectory };
#endif

  void __fastcall ApplySelection();
  TFarPanelItem * __fastcall FindFileName(const UnicodeString FileName);
  TFarPanelItem * __fastcall FindUserData(void * UserData);

private:
  PanelInfo * FPanelInfo;
  TObjectList * FItems;
  TCustomFarFileSystem * FOwner;

public:
  TObjectList * __fastcall GetItems();
  TFarPanelItem * __fastcall GetFocusedItem();
  void __fastcall SetFocusedItem(TFarPanelItem * value);
  int __fastcall GetFocusedIndex();
  void __fastcall SetFocusedIndex(int value);
  int __fastcall GetItemCount();
  int __fastcall GetSelectedCount();
  TRect __fastcall GetBounds();
  TFarPanelType __fastcall GetType();
  bool __fastcall GetIsPlugin();
  UnicodeString __fastcall GetCurrentDirectory();
};
//---------------------------------------------------------------------------
enum MENUITEMFLAGS_EX
{
  // FIXME MIF_HIDDEN = 0x40000000UL,
};
//---------------------------------------------------------------------------
class TFarMenuItems : public TStringList
{
public:
  explicit /* __fastcall */ TFarMenuItems();
  virtual /* __fastcall */ ~TFarMenuItems() {}
  void __fastcall AddSeparator(bool Visible = true);
  virtual int __fastcall Add(UnicodeString Text, bool Visible = true);

  virtual void __fastcall Clear();
  virtual void __fastcall Delete(int Index);

#ifndef _MSC_VER
  __property int ItemFocused = { read = FItemFocused, write = SetItemFocused };

  __property bool Disabled[int Index] = { read = GetFlag, write = SetFlag, index = MIF_DISABLE };
  __property bool Checked[int Index] = { read = GetFlag, write = SetFlag, index = MIF_CHECKED };
#else
  int __fastcall GetItemFocused() { return FItemFocused; }

  bool __fastcall GetDisabled(size_t Index) { return GetFlag(Index, MIF_DISABLE); }
  void __fastcall SetDisabled(size_t Index, bool value) { SetFlag(Index, MIF_DISABLE, value); }
  bool __fastcall GetChecked(size_t Index) { return GetFlag(Index, MIF_CHECKED); }
  void __fastcall SetChecked(size_t Index, bool value) { SetFlag(Index, MIF_CHECKED, value); }
#endif

protected:
  virtual void __fastcall PutObject(int Index, TObject * AObject);

private:
  int FItemFocused;

public:
  void __fastcall SetItemFocused(int value);
  void __fastcall SetFlag(size_t Index, size_t Flag, bool Value);
  bool __fastcall GetFlag(size_t Index, size_t Flag);
};
//---------------------------------------------------------------------------
class TFarEditorInfo
{
public:
  explicit /* __fastcall */ TFarEditorInfo(EditorInfo * Info);
  /* __fastcall */ ~TFarEditorInfo();

#ifndef _MSC_VER
  __property int EditorID = { read = GetEditorID };
  __property AnsiString FileName = { read = GetFileName };
#endif

private:
  EditorInfo * FEditorInfo;

public:
  int __fastcall GetEditorID();
  UnicodeString __fastcall GetFileName();
};
//---------------------------------------------------------------------------
class TFarEnvGuard
{
public:
  inline /* __fastcall */ TFarEnvGuard();
  inline /* __fastcall */ ~TFarEnvGuard();
};
//---------------------------------------------------------------------------
class TFarPluginEnvGuard
{
public:
  /* __fastcall */ TFarPluginEnvGuard();
  /* __fastcall */ ~TFarPluginEnvGuard();

private:
  bool FANSIApis;
};
//---------------------------------------------------------------------------
void __fastcall FarWrapText(UnicodeString Text, TStrings * Result, size_t MaxWidth);
//---------------------------------------------------------------------------
extern TCustomFarPlugin * FarPlugin;
