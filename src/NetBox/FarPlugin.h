#pragma once

#pragma warning(push, 1)
#include <vcl.h>
#include <Sysutils.hpp>
#pragma warning(pop)
#include "Common.h"
#include "plugin.hpp"

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
enum THandlesFunction { hfProcessKey, hfProcessHostFile, hfProcessEvent };
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
  virtual int __fastcall GetMinFarVersion();
  virtual void __fastcall SetStartupInfo(const struct PluginStartupInfo * Info);
  virtual struct PluginStartupInfo * GetStartupInfo() { return &FStartupInfo; }
  virtual void __fastcall ExitFAR();
  virtual void __fastcall GetPluginInfo(struct PluginInfo * Info);
  virtual int __fastcall Configure(int Item);
  virtual void * __fastcall OpenPlugin(int OpenFrom, intptr_t Item);
  virtual void __fastcall ClosePlugin(void * Plugin);
  virtual void __fastcall GetOpenPluginInfo(HANDLE Plugin, struct OpenPluginInfo * Info);
  virtual int __fastcall GetFindData(HANDLE Plugin,
    struct PluginPanelItem ** PanelItem, int * ItemsNumber, int OpMode);
  virtual void __fastcall FreeFindData(HANDLE Plugin, struct PluginPanelItem * PanelItem,
    int ItemsNumber);
  virtual int __fastcall ProcessHostFile(HANDLE Plugin,
    struct PluginPanelItem * PanelItem, int ItemsNumber, int OpMode);
  virtual int __fastcall ProcessKey(HANDLE Plugin, int Key, unsigned int ControlState);
  virtual int __fastcall ProcessEvent(HANDLE Plugin, int Event, void * Param);
  virtual int __fastcall SetDirectory(HANDLE Plugin, const wchar_t * Dir, int OpMode);
  virtual int __fastcall MakeDirectory(HANDLE Plugin, const wchar_t ** Name, int OpMode);
  virtual int __fastcall DeleteFiles(HANDLE Plugin, struct PluginPanelItem * PanelItem,
    int ItemsNumber, int OpMode);
  virtual int __fastcall GetFiles(HANDLE Plugin, struct PluginPanelItem * PanelItem,
    int ItemsNumber, int Move, const wchar_t ** DestPath, int OpMode);
  virtual int __fastcall PutFiles(HANDLE Plugin, struct PluginPanelItem * PanelItem,
    int ItemsNumber, int Move, const wchar_t * srcPath, int OpMode);
  virtual int __fastcall ProcessEditorEvent(int Event, void * Param);
  virtual int __fastcall ProcessEditorInput(const INPUT_RECORD * Rec);

  virtual void __fastcall HandleException(Exception * E, int OpMode = 0);

  static wchar_t * DuplicateStr(const UnicodeString Str, bool AllowEmpty = false);
  int __fastcall Message(unsigned int Flags, const UnicodeString Title,
    const UnicodeString Message, TStrings * Buttons = NULL,
    TFarMessageParams * Params = NULL);
  int __fastcall MaxMessageLines();
  int __fastcall MaxMenuItemLength();
  int __fastcall Menu(unsigned int Flags, const UnicodeString Title,
    const UnicodeString Bottom, TStrings * Items, const int * BreakKeys,
    int & BreakCode);
  int __fastcall Menu(unsigned int Flags, const UnicodeString Title,
    const UnicodeString Bottom, TStrings * Items);
  int __fastcall Menu(unsigned int Flags, const UnicodeString Title,
    const UnicodeString Bottom, const FarMenuItem * Items, int Count,
    const int * BreakKeys, int & BreakCode);
  bool __fastcall InputBox(const UnicodeString Title, const UnicodeString Prompt,
    UnicodeString & Text, unsigned long Flags, const UnicodeString HistoryName = L"",
    size_t MaxLen = 255, TFarInputBoxValidateEvent OnValidate = NULL);
  UnicodeString __fastcall GetMsg(int MsgId);
  void __fastcall SaveScreen(HANDLE & Screen);
  void __fastcall RestoreScreen(HANDLE & Screen);
  bool __fastcall CheckForEsc();
  bool __fastcall Viewer(const UnicodeString FileName, const UnicodeString Title,
    unsigned int Flags);
  bool __fastcall Editor(const UnicodeString FileName, const UnicodeString Title,
    unsigned int Flags);

  intptr_t __fastcall FarAdvControl(int Command, void * Param = NULL);
  DWORD __fastcall FarControl(int Command, int Param1, intptr_t Param2, HANDLE Plugin = INVALID_HANDLE_VALUE);
  int __fastcall FarEditorControl(int Command, void * Param);
  intptr_t __fastcall FarSystemSettings();
  void __fastcall Text(int X, int Y, int Color, const UnicodeString Str);
  void __fastcall FlushText();
  void __fastcall WriteConsole(const UnicodeString Str);
  void __fastcall FarCopyToClipboard(const UnicodeString Str);
  void __fastcall FarCopyToClipboard(TStrings * Strings);
  int __fastcall FarVersion();
  UnicodeString __fastcall FormatFarVersion(int Version);
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
  bool GetANSIApis() const { return FANSIApis; };
  unsigned int GetFarThread() const { return FFarThread; };
  FarStandardFunctions & GetFarStandardFunctions() { return FFarStandardFunctions; }

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
  intptr_t FFarSystemSettings;
  TPoint FNormalConsoleSize;
  TCustomFarPlugin * Self;

  virtual bool __fastcall HandlesFunction(THandlesFunction Function);
  virtual void __fastcall GetPluginInfoEx(long unsigned & Flags,
    TStrings * DiskMenuStrings, TStrings * PluginMenuStrings,
    TStrings * PluginConfigStrings, TStrings * CommandPrefixes) = 0;
  virtual TCustomFarFileSystem * __fastcall OpenPluginEx(int OpenFrom, intptr_t Item) = 0;
  virtual bool __fastcall ConfigureEx(int Item) = 0;
  virtual int __fastcall ProcessEditorEventEx(int Event, void * Param) = 0;
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
  void __fastcall InvalidateOpenPluginInfo();

  TCriticalSection * __fastcall GetCriticalSection() const { return FCriticalSection; }

#ifdef NETBOX_DEBUG
public:
  void RunTests();
#endif
private:
  void __fastcall UpdateProgress(int state, int progress);

private:
  PluginInfo FPluginInfo;
  TStringList * FSavedTitles;
  UnicodeString FCurrentTitle;
  UnicodeString FTemporaryDir;
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

  void __fastcall GetOpenPluginInfo(struct OpenPluginInfo * Info);
  int __fastcall GetFindData(struct PluginPanelItem ** PanelItem,
    int * ItemsNumber, int OpMode);
  void __fastcall FreeFindData(struct PluginPanelItem * PanelItem, int ItemsNumber);
  int __fastcall ProcessHostFile(struct PluginPanelItem * PanelItem,
    int ItemsNumber, int OpMode);
  int __fastcall ProcessKey(int Key, unsigned int ControlState);
  int __fastcall ProcessEvent(int Event, void * Param);
  int __fastcall SetDirectory(const wchar_t * Dir, int OpMode);
  int __fastcall MakeDirectory(const wchar_t ** Name, int OpMode);
  int __fastcall DeleteFiles(struct PluginPanelItem * PanelItem,
    int ItemsNumber, int OpMode);
  int __fastcall GetFiles(struct PluginPanelItem * PanelItem,
    int ItemsNumber, int Move, const wchar_t ** DestPath, int OpMode);
  int __fastcall PutFiles(struct PluginPanelItem * PanelItem,
    int ItemsNumber, int Move, const wchar_t * srcPath, int OpMode);
  virtual void __fastcall Close();

protected:
  TCustomFarPlugin * FPlugin;
  bool FClosed;

  virtual void __fastcall GetOpenPluginInfoEx(long unsigned & Flags,
    UnicodeString & HostFile, UnicodeString & CurDir, UnicodeString & Format,
    UnicodeString & PanelTitle, TFarPanelModes * PanelModes, int & StartPanelMode,
    int & StartSortMode, bool & StartSortOrder, TFarKeyBarTitles * KeyBarTitles,
   UnicodeString & ShortcutData) = 0;
  virtual bool __fastcall GetFindDataEx(TObjectList * PanelItems, int OpMode) = 0;
  virtual bool __fastcall ProcessHostFileEx(TObjectList * PanelItems, int OpMode);
  virtual bool __fastcall ProcessKeyEx(int Key, unsigned int ControlState);
  virtual bool __fastcall ProcessEventEx(int Event, void * Param);
  virtual bool __fastcall SetDirectoryEx(const UnicodeString Dir, int OpMode);
  virtual int __fastcall MakeDirectoryEx(UnicodeString & Name, int OpMode);
  virtual bool __fastcall DeleteFilesEx(TObjectList * PanelItems, int OpMode);
  virtual int __fastcall GetFilesEx(TObjectList * PanelItems, bool Move,
    UnicodeString & DestPath, int OpMode);
  virtual int __fastcall PutFilesEx(TObjectList * PanelItems, bool Move, int OpMode);

  void __fastcall ResetCachedInfo();
  DWORD __fastcall FarControl(int Command, int Param1, intptr_t Param2);
  DWORD __fastcall FarControl(int Command, int Param1, intptr_t Param2, HANDLE Plugin);
  bool __fastcall UpdatePanel(bool ClearSelection = false, bool Another = false);
  void __fastcall RedrawPanel(bool Another = false);
  void __fastcall ClosePlugin();
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
  void __fastcall InvalidateOpenPluginInfo();

private:
  OpenPluginInfo FOpenPluginInfo;
  bool FOpenPluginInfoValid;
  TFarPanelInfo * FPanelInfo[2];
  static unsigned int FInstances;

  void __fastcall ClearOpenPluginInfo(OpenPluginInfo & Info);
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
  /* __fastcall */ TFarPanelModes();
  virtual /* __fastcall */ ~TFarPanelModes();

  void __fastcall SetPanelMode(size_t Mode, const UnicodeString ColumnTypes = L"",
    const UnicodeString ColumnWidths = L"", TStrings * ColumnTitles = NULL,
    bool FullScreen = false, bool DetailedStatus = true, bool AlignExtensions = true,
    bool CaseConversion = true, const UnicodeString StatusColumnTypes = L"",
    const UnicodeString StatusColumnWidths = L"");

private:
  PanelMode FPanelModes[PANEL_MODES_COUNT];
  bool FReferenced;

  void __fastcall FillOpenPluginInfo(struct OpenPluginInfo * Info);
  static void __fastcall ClearPanelMode(PanelMode & Mode);
  static int __fastcall CommaCount(const UnicodeString ColumnTypes);
};
//---------------------------------------------------------------------------
class TFarKeyBarTitles : public TObject
{
friend class TCustomFarFileSystem;
public:
  /* __fastcall */ TFarKeyBarTitles();
  virtual /* __fastcall */ ~TFarKeyBarTitles();

  void __fastcall ClearFileKeyBarTitles();
  void __fastcall ClearKeyBarTitle(TFarShiftStatus ShiftStatus,
    int FunctionKeyStart, int FunctionKeyEnd = 0);
  void __fastcall SetKeyBarTitle(TFarShiftStatus ShiftStatus, int FunctionKey,
    const UnicodeString Title);

private:
  KeyBarTitles FKeyBarTitles;
  bool FReferenced;

  void __fastcall FillOpenPluginInfo(struct OpenPluginInfo * Info);
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
    unsigned long & Flags, UnicodeString & FileName, __int64 & Size,
    unsigned long & FileAttributes,
    TDateTime & LastWriteTime, TDateTime & LastAccess,
    unsigned long & NumberOfLinks, UnicodeString & Description,
    UnicodeString & Owner, void *& UserData, int & CustomColumnNumber) = 0;
  virtual UnicodeString __fastcall GetCustomColumnData(int Column);

  void __fastcall FillPanelItem(struct PluginPanelItem * PanelItem);
};
//---------------------------------------------------------------------------
class TFarPanelItem : public TCustomFarPanelItem
{
public:
  explicit /* __fastcall */ TFarPanelItem(PluginPanelItem * APanelItem);
  virtual /* __fastcall */ ~TFarPanelItem();

  unsigned long __fastcall GetFlags();
  unsigned long __fastcall GetFileAttributes();
  UnicodeString __fastcall GetFileName();
  void * __fastcall GetUserData();
  bool __fastcall GetSelected();
  void __fastcall SetSelected(bool value);
  bool __fastcall GetIsParentDirectory();
  bool __fastcall GetIsFile();

protected:
  PluginPanelItem * FPanelItem;

  virtual void __fastcall GetData(
    unsigned long & Flags, UnicodeString & FileName, __int64 & Size,
    unsigned long & FileAttributes,
    TDateTime & LastWriteTime, TDateTime & LastAccess,
    unsigned long & NumberOfLinks, UnicodeString & Description,
    UnicodeString & Owner, void *& UserData, int & CustomColumnNumber);
  virtual UnicodeString __fastcall GetCustomColumnData(int Column);
};
//---------------------------------------------------------------------------
class THintPanelItem : public TCustomFarPanelItem
{
public:
  explicit /* __fastcall */ THintPanelItem(const UnicodeString AHint);
  virtual /* __fastcall */ ~THintPanelItem() {}

protected:
  virtual void __fastcall GetData(
    unsigned long & Flags, UnicodeString & FileName, __int64 & Size,
    unsigned long & FileAttributes,
    TDateTime & LastWriteTime, TDateTime & LastAccess,
    unsigned long & NumberOfLinks, UnicodeString & Description,
    UnicodeString & Owner, void *& UserData, int & CustomColumnNumber);

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

  int __fastcall GetItemFocused() { return FItemFocused; }
  void __fastcall SetItemFocused(int value);
  bool __fastcall GetDisabled(size_t Index) { return GetFlag(Index, MIF_DISABLE); }
  void __fastcall SetDisabled(size_t Index, bool value) { SetFlag(Index, MIF_DISABLE, value); }
  bool __fastcall GetChecked(size_t Index) { return GetFlag(Index, MIF_CHECKED); }
  void __fastcall SetChecked(size_t Index, bool value) { SetFlag(Index, MIF_CHECKED, value); }

  void __fastcall SetFlag(size_t Index, size_t Flag, bool Value);
  bool __fastcall GetFlag(size_t Index, size_t Flag);

protected:
  virtual void __fastcall PutObject(int Index, TObject * AObject);

private:
  int FItemFocused;
};
//---------------------------------------------------------------------------
class TFarEditorInfo
{
public:
  explicit /* __fastcall */ TFarEditorInfo(EditorInfo * Info);
  /* __fastcall */ ~TFarEditorInfo();

  int __fastcall GetEditorID() const;
  static UnicodeString __fastcall GetFileName();

private:
  EditorInfo * FEditorInfo;
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
