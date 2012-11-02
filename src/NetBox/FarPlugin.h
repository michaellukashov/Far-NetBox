#pragma once

#pragma warning(push, 1)
#include <vcl.h>
#include <Sysutils.hpp>
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
class TFarMessageDialog;
class TFarEditorInfo;
class TFarPluginGuard;
//---------------------------------------------------------------------------
const int MaxMessageWidth = 64;
//---------------------------------------------------------------------------
enum TFarShiftStatus { fsNone, fsCtrl, fsAlt, fsShift, fsCtrlShift,
  fsAltShift, fsCtrlAlt };
enum THandlesFunction { hfProcessKey, hfProcessHostFile, hfProcessPanelEvent };
DEFINE_CALLBACK_TYPE1(TFarInputBoxValidateEvent, void, UnicodeString & /* Text */);
//---------------------------------------------------------------------------
DEFINE_CALLBACK_TYPE1(TFarMessageTimerEvent, void, unsigned int & /* Result */);
DEFINE_CALLBACK_TYPE3(TFarMessageClickEvent, void, void * /* Token */, int /* Result */, bool & /* Close */);
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
  friend class TCustomFarFileSystem;
  friend class TFarDialog;
  friend class TWinSCPFileSystem;
  friend class TFarDialogItem;
  friend class TFarMessageDialog;
  friend class TFarPluginGuard;
public:
  explicit TCustomFarPlugin(HINSTANCE HInst);
  virtual ~TCustomFarPlugin();
  virtual VersionInfo __fastcall GetMinFarVersion();
  virtual void __fastcall SetStartupInfo(const struct PluginStartupInfo * Info);
  virtual struct PluginStartupInfo * GetStartupInfo() { return &FStartupInfo; }
  virtual void __fastcall ExitFAR();
  virtual void __fastcall GetPluginInfo(struct PluginInfo * Info);
  virtual intptr_t __fastcall Configure(const struct ConfigureInfo *Info);
  virtual void * __fastcall OpenPlugin(const struct OpenInfo *Info);
  virtual void __fastcall ClosePanel(void *Plugin);
  virtual void __fastcall GetOpenPanelInfo(struct OpenPanelInfo *Info);
  virtual intptr_t __fastcall GetFindData(struct GetFindDataInfo *Info);

  virtual void __fastcall FreeFindData(const struct FreeFindDataInfo *Info);

  virtual intptr_t __fastcall ProcessHostFile(const struct ProcessHostFileInfo *Info);


  virtual intptr_t __fastcall ProcessPanelInput(const struct ProcessPanelInputInfo *Info);

  virtual intptr_t __fastcall ProcessPanelEvent(const struct ProcessPanelEventInfo *Info);


  virtual intptr_t __fastcall SetDirectory(const struct SetDirectoryInfo *Info);
  virtual intptr_t __fastcall MakeDirectory(struct MakeDirectoryInfo *Info);
  virtual intptr_t __fastcall DeleteFiles(const struct DeleteFilesInfo *Info);
  virtual intptr_t __fastcall GetFiles(struct GetFilesInfo *Info);

  virtual intptr_t __fastcall PutFiles(const struct PutFilesInfo *Info);

  virtual intptr_t __fastcall ProcessEditorEvent(const struct ProcessEditorEventInfo *Info);

  virtual intptr_t __fastcall ProcessEditorInput(const struct ProcessEditorInputInfo *Info);

  virtual void __fastcall HandleException(Exception * E, int OpMode = 0);

  static wchar_t * DuplicateStr(const UnicodeString Str, bool AllowEmpty = false);
  int __fastcall Message(unsigned int Flags, const UnicodeString Title,
    const UnicodeString Message, TStrings * Buttons = NULL,
    TFarMessageParams * Params = NULL);
  int __fastcall MaxMessageLines();
  int __fastcall MaxMenuItemLength();
  intptr_t __fastcall Menu(unsigned int Flags, const UnicodeString Title,
    const UnicodeString Bottom, TStrings * Items, const FarKey * BreakKeys,
    intptr_t & BreakCode);
  intptr_t __fastcall Menu(unsigned int Flags, const UnicodeString Title,
    const UnicodeString Bottom, TStrings * Items);
  intptr_t __fastcall Menu(unsigned int Flags, const UnicodeString Title,
    const UnicodeString Bottom, const FarMenuItem * Items, int Count,
    const FarKey * BreakKeys, intptr_t & BreakCode);
  bool __fastcall InputBox(const UnicodeString Title, const UnicodeString Prompt,
    UnicodeString & Text, PLUGINPANELITEMFLAGS Flags, const UnicodeString HistoryName = L"",
    size_t MaxLen = 255, TFarInputBoxValidateEvent OnValidate = NULL);
  UnicodeString __fastcall GetMsg(int MsgId);
  void __fastcall SaveScreen(HANDLE & Screen);
  void __fastcall RestoreScreen(HANDLE & Screen);
  bool __fastcall CheckForEsc();
  bool __fastcall Viewer(const UnicodeString FileName, const UnicodeString Title,
    unsigned int Flags);
  bool __fastcall Editor(const UnicodeString FileName, const UnicodeString Title,
    unsigned int Flags);
  intptr_t __fastcall FarControl(FILE_CONTROL_COMMANDS Command, intptr_t Param1, void * Param2, HANDLE Plugin = INVALID_HANDLE_VALUE);
  __int64 __fastcall FarAdvControl(ADVANCED_CONTROL_COMMANDS Command, intptr_t Param1, void *Param2 = NULL);
  intptr_t __fastcall FarEditorControl(EDITOR_CONTROL_COMMANDS Command, intptr_t Param1, void * Param2);
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

  UnicodeString __fastcall GetModuleName();
  TFarDialog * __fastcall GetTopDialog() const { return FTopDialog; }
  HINSTANCE GetHandle() const { return FHandle; };
  unsigned int GetFarThread() const { return FFarThread; };
  FarStandardFunctions & GetFarStandardFunctions() { return FFarStandardFunctions; }
  const struct PluginStartupInfo * __fastcall GetStartupInfo() const { return &FStartupInfo; }

protected:
  PluginStartupInfo FStartupInfo;
  FarStandardFunctions FFarStandardFunctions;
  HINSTANCE FHandle;
  TObjectList * FOpenedPlugins;
  TFarDialog * FTopDialog;
  HANDLE FConsoleInput;
  HANDLE FConsoleOutput;
  intptr_t FFarVersion;
  bool FTerminalScreenShowing;
  TCriticalSection * FCriticalSection;
  unsigned int FFarThread;
  bool FValidFarSystemSettings;
  intptr_t FFarSystemSettings;
  TPoint FNormalConsoleSize;

  virtual bool __fastcall HandlesFunction(THandlesFunction Function);
  virtual void __fastcall GetPluginInfoEx(PLUGIN_FLAGS & Flags,
    TStrings * DiskMenuStrings, TStrings * PluginMenuStrings,
    TStrings * PluginConfigStrings, TStrings * CommandPrefixes) = 0;
  virtual TCustomFarFileSystem * __fastcall OpenPluginEx(OPENFROM OpenFrom, intptr_t Item) = 0;
  virtual bool __fastcall ConfigureEx(const GUID * Guid) = 0;
  virtual intptr_t __fastcall ProcessEditorEventEx(const struct ProcessEditorEventInfo *Info) = 0;
  virtual intptr_t __fastcall ProcessEditorInputEx(const INPUT_RECORD * Rec) = 0;
  virtual void __fastcall HandleFileSystemException(TCustomFarFileSystem * FileSystem,
    Exception * E, int OpMode = 0);
  void __fastcall ResetCachedInfo();
  int __fastcall MaxLength(TStrings * Strings);
  int __fastcall FarMessage(unsigned int Flags,
    const UnicodeString Title, const UnicodeString Message, TStrings * Buttons,
    TFarMessageParams * Params);
  int __fastcall DialogMessage(unsigned int Flags,
    const UnicodeString Title, const UnicodeString Message, TStrings * Buttons,
    TFarMessageParams * Params);
  void __fastcall InvalidateOpenPanelInfo();

  TCriticalSection * __fastcall GetCriticalSection() const { return FCriticalSection; }

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
  UnicodeString FTemporaryDir;
  short FCurrentProgress;

  void __fastcall ClearPluginInfo(PluginInfo & Info);
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
friend class TFarPanelInfo;
friend class TCustomFarPlugin;
public:
  TCustomFarFileSystem(TCustomFarPlugin * APlugin);
  virtual void __fastcall Init();
  virtual ~TCustomFarFileSystem();

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
  virtual UnicodeString GetCurrentDirectory() = 0;

protected:
  TCustomFarPlugin * FPlugin;
  bool FClosed;

  virtual void __fastcall GetOpenPanelInfoEx(OPENPANELINFO_FLAGS & Flags,
    UnicodeString & HostFile, UnicodeString & CurDir, UnicodeString & Format,
    UnicodeString & PanelTitle, TFarPanelModes * PanelModes, intptr_t & StartPanelMode,
    OPENPANELINFO_SORTMODES & StartSortMode, bool & StartSortOrder, TFarKeyBarTitles * KeyBarTitles,
    UnicodeString & ShortcutData) = 0;
  virtual bool __fastcall GetFindDataEx(TObjectList * PanelItems, int OpMode) = 0;
  virtual bool __fastcall ProcessHostFileEx(TObjectList * PanelItems, int OpMode);
  virtual bool __fastcall ProcessKeyEx(intptr_t Key, uintptr_t ControlState);
  virtual bool __fastcall ProcessPanelEventEx(int Event, void *Param);
  virtual bool __fastcall SetDirectoryEx(const UnicodeString Dir, int OpMode);
  virtual int __fastcall MakeDirectoryEx(UnicodeString & Name, int OpMode);
  virtual bool __fastcall DeleteFilesEx(TObjectList * PanelItems, int OpMode);
  virtual int __fastcall GetFilesEx(TObjectList * PanelItems, bool Move,
    UnicodeString & DestPath, int OpMode);
  virtual int __fastcall PutFilesEx(TObjectList * PanelItems, bool Move, int OpMode);

  void __fastcall ResetCachedInfo();
  intptr_t __fastcall FarControl(FILE_CONTROL_COMMANDS Command, intptr_t Param1, void * Param2);
  intptr_t __fastcall FarControl(FILE_CONTROL_COMMANDS Command, intptr_t Param1, void * Param2, HANDLE Plugin);
  bool __fastcall UpdatePanel(bool ClearSelection = false, bool Another = false);
  void __fastcall RedrawPanel(bool Another = false);
  void __fastcall ClosePanel();
  UnicodeString __fastcall GetMsg(int MsgId);
  TCustomFarFileSystem * __fastcall GetOppositeFileSystem();
  bool __fastcall IsActiveFileSystem();
  bool __fastcall IsLeft();
  bool __fastcall IsRight();

  virtual void __fastcall HandleException(Exception * E, int OpMode = 0);

  TFarPanelInfo * GetPanelInfo() { return GetPanelInfo(0); };
  TFarPanelInfo * GetAnotherPanelInfo() { return GetPanelInfo(1); };
  TCriticalSection * GetCriticalSection() { return FCriticalSection; };

protected:
  TCriticalSection * FCriticalSection;
  void __fastcall InvalidateOpenPanelInfo();

private:
  UnicodeString FNameStr;
  UnicodeString FDestPathStr;
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
  TFarPanelModes();
  virtual ~TFarPanelModes();

  void __fastcall SetPanelMode(size_t Mode, const UnicodeString ColumnTypes = L"",
    const UnicodeString ColumnWidths = L"", TStrings * ColumnTitles = NULL,
    bool FullScreen = false, bool DetailedStatus = true, bool AlignExtensions = true,
    bool CaseConversion = true, const UnicodeString StatusColumnTypes = L"",
    const UnicodeString StatusColumnWidths = L"");

private:
  PanelMode FPanelModes[PANEL_MODES_COUNT];
  bool FReferenced;

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
  TFarKeyBarTitles();
  virtual ~TFarKeyBarTitles();

  void __fastcall ClearFileKeyBarTitles();
  void __fastcall ClearKeyBarTitle(TFarShiftStatus ShiftStatus,
    int FunctionKeyStart, int FunctionKeyEnd = 0);
  void __fastcall SetKeyBarTitle(TFarShiftStatus ShiftStatus, int FunctionKey,
    const UnicodeString Title);

private:
  KeyBarTitles FKeyBarTitles;
  bool FReferenced;

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
    PLUGINPANELITEMFLAGS & Flags, UnicodeString & FileName, __int64 & Size,
    uintptr_t & FileAttributes,
    TDateTime & LastWriteTime, TDateTime & LastAccess,
    uintptr_t & NumberOfLinks, UnicodeString & Description,
    UnicodeString & Owner, void *& UserData, size_t & CustomColumnNumber) = 0;
  virtual UnicodeString __fastcall GetCustomColumnData(int Column);

  void __fastcall FillPanelItem(struct PluginPanelItem * PanelItem);
};
//---------------------------------------------------------------------------
class TFarPanelItem : public TCustomFarPanelItem
{
public:
  explicit TFarPanelItem(PluginPanelItem * APanelItem, bool OwnsItem);
  virtual ~TFarPanelItem();

  PLUGINPANELITEMFLAGS __fastcall GetFlags();
  uintptr_t __fastcall GetFileAttributes();
  UnicodeString __fastcall GetFileName();
  void * __fastcall GetUserData();
  bool __fastcall GetSelected();
  void __fastcall SetSelected(bool value);
  bool __fastcall GetIsParentDirectory();
  bool __fastcall GetIsFile();

protected:
  PluginPanelItem * FPanelItem;
  bool FOwnsItem;

  virtual void __fastcall GetData(
    PLUGINPANELITEMFLAGS & Flags, UnicodeString & FileName, __int64 & Size,
    uintptr_t & FileAttributes,
    TDateTime & LastWriteTime, TDateTime & LastAccess,
    uintptr_t & NumberOfLinks, UnicodeString & Description,
    UnicodeString & Owner, void *& UserData, size_t & CustomColumnNumber);
  virtual UnicodeString __fastcall GetCustomColumnData(int Column);
};
//---------------------------------------------------------------------------
class THintPanelItem : public TCustomFarPanelItem
{
public:
  explicit THintPanelItem(const UnicodeString AHint);
  virtual ~THintPanelItem() {}

protected:
  virtual void __fastcall GetData(
    PLUGINPANELITEMFLAGS & Flags, UnicodeString & FileName, __int64 & Size,
    uintptr_t & FileAttributes,
    TDateTime & LastWriteTime, TDateTime & LastAccess,
    uintptr_t & NumberOfLinks, UnicodeString & Description,
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
  explicit TFarPanelInfo(PanelInfo * APanelInfo, TCustomFarFileSystem * AOwner);
  virtual ~TFarPanelInfo();

  TObjectList * __fastcall GetItems();
  int __fastcall GetItemCount();
  TFarPanelItem * __fastcall GetFocusedItem();
  void __fastcall SetFocusedItem(TFarPanelItem * value);
  int __fastcall GetFocusedIndex();
  void __fastcall SetFocusedIndex(int value);
  int __fastcall GetSelectedCount();
  TRect __fastcall GetBounds();
  TFarPanelType __fastcall GetType();
  bool __fastcall GetIsPlugin();
  UnicodeString __fastcall GetCurrentDirectory();

  void __fastcall ApplySelection();
  TFarPanelItem * __fastcall FindFileName(const UnicodeString FileName);
  TFarPanelItem * __fastcall FindUserData(void * UserData);

private:
  PanelInfo * FPanelInfo;
  TObjectList * FItems;
  TCustomFarFileSystem * FOwner;
};
//---------------------------------------------------------------------------
class TFarMenuItems : public TStringList
{
public:
  explicit TFarMenuItems();
  virtual ~TFarMenuItems() {}
  void __fastcall AddSeparator(bool Visible = true);
  virtual int __fastcall Add(UnicodeString Text, bool Visible = true);

  virtual void __fastcall Clear();
  virtual void __fastcall Delete(intptr_t Index);

  intptr_t __fastcall GetItemFocused() { return FItemFocused; }
  void __fastcall SetItemFocused(intptr_t Value);
  bool __fastcall GetDisabled(intptr_t Index) { return GetFlag(Index, MIF_DISABLE); }
  void __fastcall SetDisabled(intptr_t Index, bool value) { SetFlag(Index, MIF_DISABLE, value); }
  bool __fastcall GetChecked(intptr_t Index) { return GetFlag(Index, MIF_CHECKED); }
  void __fastcall SetChecked(intptr_t Index, bool value) { SetFlag(Index, MIF_CHECKED, value); }

  void __fastcall SetFlag(intptr_t Index, uintptr_t Flag, bool Value);
  bool __fastcall GetFlag(intptr_t Index, uintptr_t Flag);

protected:
  virtual void __fastcall PutObject(intptr_t Index, TObject * AObject);

private:
  intptr_t FItemFocused;
};
//---------------------------------------------------------------------------
class TFarEditorInfo
{
public:
  explicit TFarEditorInfo(EditorInfo * Info);
  ~TFarEditorInfo();

  int __fastcall GetEditorID() const;
  static UnicodeString __fastcall GetFileName();

private:
  EditorInfo * FEditorInfo;
};
//---------------------------------------------------------------------------
class TFarEnvGuard
{
public:
  inline TFarEnvGuard();
  inline ~TFarEnvGuard();
};
//---------------------------------------------------------------------------
class TFarPluginEnvGuard
{
public:
  inline TFarPluginEnvGuard();
  inline ~TFarPluginEnvGuard();
};
//---------------------------------------------------------------------------
void __fastcall FarWrapText(UnicodeString Text, TStrings * Result, intptr_t MaxWidth);
//---------------------------------------------------------------------------
extern TCustomFarPlugin * FarPlugin;
