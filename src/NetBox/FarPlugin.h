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
DEFINE_CALLBACK_TYPE3(TFarMessageClickEvent, void, void * /* Token */, intptr_t /* Result */, bool & /* Close */);
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
  friend class TCustomFarFileSystem;
  friend class TFarDialog;
  friend class TWinSCPFileSystem;
  friend class TFarDialogItem;
  friend class TFarMessageDialog;
  friend class TFarPluginGuard;
public:
  explicit TCustomFarPlugin(HINSTANCE HInst);
  virtual ~TCustomFarPlugin();
  virtual intptr_t GetMinFarVersion();
  virtual void SetStartupInfo(const struct PluginStartupInfo * Info);
  virtual struct PluginStartupInfo * GetStartupInfo() { return &FStartupInfo; }
  virtual void ExitFAR();
  virtual void GetPluginInfo(struct PluginInfo * Info);
  virtual intptr_t Configure(intptr_t Item);
  virtual void * OpenPlugin(int OpenFrom, intptr_t Item);
  virtual void ClosePlugin(void * Plugin);
  virtual void GetOpenPluginInfo(HANDLE Plugin, struct OpenPluginInfo * Info);
  virtual intptr_t GetFindData(HANDLE Plugin,
    struct PluginPanelItem ** PanelItem, int * ItemsNumber, int OpMode);
  virtual void FreeFindData(HANDLE Plugin, struct PluginPanelItem * PanelItem,
    int ItemsNumber);
  virtual intptr_t ProcessHostFile(HANDLE Plugin,
    struct PluginPanelItem * PanelItem, int ItemsNumber, int OpMode);
  virtual intptr_t ProcessKey(HANDLE Plugin, int Key, unsigned int ControlState);
  virtual intptr_t ProcessEvent(HANDLE Plugin, int Event, void * Param);
  virtual intptr_t SetDirectory(HANDLE Plugin, const wchar_t * Dir, int OpMode);
  virtual intptr_t MakeDirectory(HANDLE Plugin, const wchar_t ** Name, int OpMode);
  virtual intptr_t DeleteFiles(HANDLE Plugin, struct PluginPanelItem * PanelItem,
    int ItemsNumber, int OpMode);
  virtual intptr_t GetFiles(HANDLE Plugin, struct PluginPanelItem * PanelItem,
    int ItemsNumber, int Move, const wchar_t ** DestPath, int OpMode);
  virtual intptr_t PutFiles(HANDLE Plugin, struct PluginPanelItem * PanelItem,
    int ItemsNumber, int Move, const wchar_t * srcPath, int OpMode);
  virtual intptr_t ProcessEditorEvent(int Event, void * Param);
  virtual intptr_t ProcessEditorInput(const INPUT_RECORD * Rec);

  virtual void HandleException(Exception * E, int OpMode = 0);

  static wchar_t * DuplicateStr(const UnicodeString & Str, bool AllowEmpty = false);
  intptr_t Message(DWORD Flags, const UnicodeString & Title,
    const UnicodeString & Message, TStrings * Buttons = NULL,
    TFarMessageParams * Params = NULL);
  intptr_t MaxMessageLines();
  intptr_t MaxMenuItemLength();
  intptr_t Menu(DWORD Flags, const UnicodeString & Title,
    const UnicodeString & Bottom, TStrings * Items, const int * BreakKeys,
    int & BreakCode);
  intptr_t Menu(DWORD Flags, const UnicodeString & Title,
    const UnicodeString & Bottom, TStrings * Items);
  intptr_t Menu(DWORD Flags, const UnicodeString & Title,
    const UnicodeString & Bottom, const FarMenuItem * Items, intptr_t Count,
    const int * BreakKeys, int & BreakCode);
  bool InputBox(const UnicodeString & Title, const UnicodeString & Prompt,
    UnicodeString & Text, DWORD Flags, const UnicodeString & HistoryName = UnicodeString(),
    intptr_t MaxLen = 255, TFarInputBoxValidateEvent OnValidate = NULL);
  UnicodeString GetMsg(int MsgId);
  void SaveScreen(HANDLE & Screen);
  void RestoreScreen(HANDLE & Screen);
  bool CheckForEsc();
  bool Viewer(const UnicodeString & FileName, const UnicodeString & Title, DWORD Flags);
  bool Editor(const UnicodeString & FileName, const UnicodeString & Title, DWORD Flags);

  intptr_t FarControl(uintptr_t Command, intptr_t Param1, intptr_t Param2, HANDLE Plugin = INVALID_HANDLE_VALUE);
  intptr_t FarAdvControl(uintptr_t Command, void * Param = NULL);
  intptr_t FarEditorControl(uintptr_t Command, void * Param);
  intptr_t FarSystemSettings();
  void Text(int X, int Y, int Color, const UnicodeString & Str);
  void FlushText();
  void WriteConsole(const UnicodeString & Str);
  void FarCopyToClipboard(const UnicodeString & Str);
  void FarCopyToClipboard(TStrings * Strings);
  intptr_t FarVersion();
  UnicodeString FormatFarVersion(intptr_t Version);
  UnicodeString TemporaryDir();
  intptr_t InputRecordToKey(const INPUT_RECORD * Rec);
  TFarEditorInfo * EditorInfo();

  void ShowConsoleTitle(const UnicodeString & Title);
  void ClearConsoleTitle();
  void UpdateConsoleTitle(const UnicodeString & Title);
  void UpdateConsoleTitleProgress(short Progress);
  void ShowTerminalScreen();
  void SaveTerminalScreen();
  void ScrollTerminalScreen(int Rows);
  TPoint TerminalInfo(TPoint * Size = NULL, TPoint * Cursor = NULL);
  unsigned int ConsoleWindowState();
  void ToggleVideoMode();

  TCustomFarFileSystem * GetPanelFileSystem(bool Another = false,
    HANDLE Plugin = INVALID_HANDLE_VALUE);

  UnicodeString GetModuleName();
  TFarDialog * GetTopDialog() const { return FTopDialog; }
  HINSTANCE GetHandle() const { return FHandle; };
  unsigned int GetFarThread() const { return FFarThread; };
  FarStandardFunctions & GetFarStandardFunctions() { return FFarStandardFunctions; }

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

  virtual bool HandlesFunction(THandlesFunction Function);
  virtual void GetPluginInfoEx(DWORD & Flags,
    TStrings * DiskMenuStrings, TStrings * PluginMenuStrings,
    TStrings * PluginConfigStrings, TStrings * CommandPrefixes) = 0;
  virtual TCustomFarFileSystem * OpenPluginEx(intptr_t OpenFrom, intptr_t Item) = 0;
  virtual bool ConfigureEx(intptr_t Item) = 0;
  virtual intptr_t ProcessEditorEventEx(intptr_t Event, void * Param) = 0;
  virtual intptr_t ProcessEditorInputEx(const INPUT_RECORD * Rec) = 0;
  virtual void HandleFileSystemException(TCustomFarFileSystem * FileSystem,
    Exception * E, int OpMode = 0);
  void ResetCachedInfo();
  intptr_t MaxLength(TStrings * Strings);
  intptr_t FarMessage(DWORD Flags,
    const UnicodeString & Title, const UnicodeString & Message, TStrings * Buttons,
    TFarMessageParams * Params);
  intptr_t DialogMessage(DWORD Flags,
    const UnicodeString & Title, const UnicodeString & Message, TStrings * Buttons,
    TFarMessageParams * Params);
  void InvalidateOpenPluginInfo();

  TCriticalSection * GetCriticalSection() const { return FCriticalSection; }

#ifdef NETBOX_DEBUG
public:
  void RunTests();
#endif
private:
  void UpdateProgress(intptr_t State, intptr_t Progress);

private:
  PluginInfo FPluginInfo;
  TStringList * FSavedTitles;
  UnicodeString FCurrentTitle;
  UnicodeString FTemporaryDir;
  short FCurrentProgress;

  void ClearPluginInfo(PluginInfo & Info);
  void UpdateConsoleTitle();
  UnicodeString FormatConsoleTitle();
  HWND GetConsoleWindow();
  RECT GetPanelBounds(HANDLE PanelHandle);
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
  void Init();
  virtual ~TCustomFarFileSystem();

  void GetOpenPluginInfo(struct OpenPluginInfo * Info);
  intptr_t GetFindData(struct PluginPanelItem ** PanelItem,
    int * ItemsNumber, int OpMode);
  void FreeFindData(struct PluginPanelItem * PanelItem, int ItemsNumber);
  intptr_t ProcessHostFile(struct PluginPanelItem * PanelItem,
    int ItemsNumber, int OpMode);
  intptr_t ProcessKey(intptr_t Key, uintptr_t ControlState);
  intptr_t ProcessEvent(intptr_t Event, void * Param);
  intptr_t SetDirectory(const wchar_t * Dir, int OpMode);
  intptr_t MakeDirectory(const wchar_t ** Name, int OpMode);
  intptr_t DeleteFiles(struct PluginPanelItem * PanelItem,
    int ItemsNumber, int OpMode);
  intptr_t GetFiles(struct PluginPanelItem * PanelItem,
    int ItemsNumber, int Move, const wchar_t ** DestPath, int OpMode);
  intptr_t PutFiles(struct PluginPanelItem * PanelItem,
    int ItemsNumber, int Move, const wchar_t * srcPath, int OpMode);
  virtual void Close();

protected:
  virtual UnicodeString GetCurrentDirectory() = 0;

protected:
  TCustomFarPlugin * FPlugin;
  bool FClosed;

  virtual void GetOpenPluginInfoEx(DWORD & Flags,
    UnicodeString & HostFile, UnicodeString & CurDir, UnicodeString & Format,
    UnicodeString & PanelTitle, TFarPanelModes * PanelModes, int & StartPanelMode,
    int & StartSortMode, bool & StartSortOrder, TFarKeyBarTitles * KeyBarTitles,
    UnicodeString & ShortcutData) = 0;
  virtual bool GetFindDataEx(TObjectList * PanelItems, int OpMode) = 0;
  virtual bool ProcessHostFileEx(TObjectList * PanelItems, int OpMode);
  virtual bool ProcessKeyEx(intptr_t Key, uintptr_t ControlState);
  virtual bool ProcessEventEx(intptr_t Event, void * Param);
  virtual bool SetDirectoryEx(const UnicodeString & Dir, int OpMode);
  virtual intptr_t MakeDirectoryEx(UnicodeString & Name, int OpMode);
  virtual bool DeleteFilesEx(TObjectList * PanelItems, int OpMode);
  virtual intptr_t GetFilesEx(TObjectList * PanelItems, bool Move,
    UnicodeString & DestPath, int OpMode);
  virtual intptr_t PutFilesEx(TObjectList * PanelItems, bool Move, int OpMode);

  void ResetCachedInfo();
  intptr_t FarControl(uintptr_t Command, intptr_t Param1, intptr_t Param2);
  intptr_t FarControl(uintptr_t Command, intptr_t Param1, intptr_t Param2, HANDLE Plugin);
  bool UpdatePanel(bool ClearSelection = false, bool Another = false);
  void RedrawPanel(bool Another = false);
  void ClosePlugin();
  UnicodeString GetMsg(int MsgId);
  TCustomFarFileSystem * GetOppositeFileSystem();
  bool IsActiveFileSystem();
  bool IsLeft();
  bool IsRight();

  virtual void HandleException(Exception * E, int OpMode = 0);

  TFarPanelInfo * GetPanelInfo() { return GetPanelInfo(0); };
  TFarPanelInfo * GetAnotherPanelInfo() { return GetPanelInfo(1); };
  TCriticalSection * GetCriticalSection() { return FCriticalSection; };

protected:
  TCriticalSection * FCriticalSection;
  void InvalidateOpenPluginInfo();

private:
  UnicodeString FNameStr;
  UnicodeString FDestPathStr;
  OpenPluginInfo FOpenPluginInfo;
  bool FOpenPluginInfoValid;
  TFarPanelInfo * FPanelInfo[2];
  static unsigned int FInstances;

  void ClearOpenPluginInfo(OpenPluginInfo & Info);
  TObjectList * CreatePanelItemList(struct PluginPanelItem * PanelItem,
    int ItemsNumber);
  TFarPanelInfo * GetPanelInfo(int Another);
};
//---------------------------------------------------------------------------
#define PANEL_MODES_COUNT 10
class TFarPanelModes : public TObject
{
  friend class TCustomFarFileSystem;
public:
  TFarPanelModes();
  virtual ~TFarPanelModes();

  void SetPanelMode(size_t Mode, const UnicodeString & ColumnTypes = UnicodeString(),
    const UnicodeString & ColumnWidths = UnicodeString(), TStrings * ColumnTitles = NULL,
    bool FullScreen = false, bool DetailedStatus = true, bool AlignExtensions = true,
    bool CaseConversion = true, const UnicodeString & StatusColumnTypes = UnicodeString(),
    const UnicodeString & StatusColumnWidths = UnicodeString());

private:
  PanelMode FPanelModes[PANEL_MODES_COUNT];
  bool FReferenced;

  void FillOpenPluginInfo(struct OpenPluginInfo * Info);
  static void ClearPanelMode(PanelMode & Mode);
  static intptr_t CommaCount(const UnicodeString & ColumnTypes);
};
//---------------------------------------------------------------------------
class TFarKeyBarTitles : public TObject
{
friend class TCustomFarFileSystem;
public:
  TFarKeyBarTitles();
  virtual ~TFarKeyBarTitles();

  void ClearFileKeyBarTitles();
  void ClearKeyBarTitle(TFarShiftStatus ShiftStatus,
    intptr_t FunctionKeyStart, intptr_t FunctionKeyEnd = 0);
  void SetKeyBarTitle(TFarShiftStatus ShiftStatus, intptr_t FunctionKey,
    const UnicodeString & Title);

private:
  KeyBarTitles FKeyBarTitles;
  bool FReferenced;

  void FillOpenPluginInfo(struct OpenPluginInfo * Info);
  static void ClearKeyBarTitles(KeyBarTitles & Titles);
};
//---------------------------------------------------------------------------
class TCustomFarPanelItem : public TObject
{
  friend class TCustomFarFileSystem;
public:

protected:
  virtual ~TCustomFarPanelItem()
  {}
  virtual void GetData(
    DWORD & Flags, UnicodeString & FileName, __int64 & Size,
    DWORD & FileAttributes,
    TDateTime & LastWriteTime, TDateTime & LastAccess,
    DWORD & NumberOfLinks, UnicodeString & Description,
    UnicodeString & Owner, void *& UserData, int & CustomColumnNumber) = 0;
  virtual UnicodeString GetCustomColumnData(size_t Column);

  void FillPanelItem(struct PluginPanelItem * PanelItem);
};
//---------------------------------------------------------------------------
class TFarPanelItem : public TCustomFarPanelItem
{
public:
  explicit TFarPanelItem(PluginPanelItem * APanelItem, bool OwnsItem);
  virtual ~TFarPanelItem();

  uintptr_t GetFlags();
  uintptr_t GetFileAttributes();
  UnicodeString GetFileName();
  void * GetUserData();
  bool GetSelected();
  void SetSelected(bool Value);
  bool GetIsParentDirectory();
  bool GetIsFile();

protected:
  PluginPanelItem * FPanelItem;
  bool FOwnsItem;

  virtual void GetData(
    DWORD & Flags, UnicodeString & FileName, __int64 & Size,
    DWORD & FileAttributes,
    TDateTime & LastWriteTime, TDateTime & LastAccess,
    DWORD & NumberOfLinks, UnicodeString & Description,
    UnicodeString & Owner, void *& UserData, int & CustomColumnNumber);
  virtual UnicodeString GetCustomColumnData(size_t Column);
};
//---------------------------------------------------------------------------
class THintPanelItem : public TCustomFarPanelItem
{
public:
  explicit THintPanelItem(const UnicodeString & AHint);
  virtual ~THintPanelItem() {}

protected:
  virtual void GetData(
    DWORD & Flags, UnicodeString & FileName, __int64 & Size,
    DWORD & FileAttributes,
    TDateTime & LastWriteTime, TDateTime & LastAccess,
    DWORD & NumberOfLinks, UnicodeString & Description,
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
  explicit TFarPanelInfo(PanelInfo * APanelInfo, TCustomFarFileSystem * AOwner);
  virtual ~TFarPanelInfo();

  TObjectList * GetItems();
  intptr_t GetItemCount();
  TFarPanelItem * GetFocusedItem();
  void SetFocusedItem(TFarPanelItem * Value);
  intptr_t GetFocusedIndex();
  void SetFocusedIndex(intptr_t Value);
  intptr_t GetSelectedCount();
  TRect GetBounds();
  TFarPanelType GetType();
  bool GetIsPlugin();
  UnicodeString GetCurrentDirectory();

  void ApplySelection();
  TFarPanelItem * FindFileName(const UnicodeString & FileName);
  TFarPanelItem * FindUserData(void * UserData);

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
  void AddSeparator(bool Visible = true);
  virtual intptr_t Add(const UnicodeString & Text, bool Visible = true);

  virtual void Clear();
  virtual void Delete(intptr_t Index);

  intptr_t GetItemFocused() { return FItemFocused; }
  void SetItemFocused(intptr_t Value);
  bool GetDisabled(intptr_t Index) { return GetFlag(Index, MIF_DISABLE); }
  void SetDisabled(intptr_t Index, bool Value) { SetFlag(Index, MIF_DISABLE, Value); }
  bool GetChecked(intptr_t Index) { return GetFlag(Index, MIF_CHECKED); }
  void SetChecked(intptr_t Index, bool Value) { SetFlag(Index, MIF_CHECKED, Value); }

  void SetFlag(intptr_t Index, uintptr_t Flag, bool Value);
  bool GetFlag(intptr_t Index, uintptr_t Flag);

protected:
  virtual void PutObject(intptr_t Index, TObject * AObject);

private:
  intptr_t FItemFocused;
};
//---------------------------------------------------------------------------
class TFarEditorInfo : public TObject
{
public:
  explicit TFarEditorInfo(EditorInfo * Info);
  ~TFarEditorInfo();

  intptr_t GetEditorID() const;
  static UnicodeString GetFileName();

private:
  EditorInfo * FEditorInfo;
};
//---------------------------------------------------------------------------
class TFarEnvGuard : public TObject
{
public:
  inline TFarEnvGuard();
  inline ~TFarEnvGuard();
};
//---------------------------------------------------------------------------
class TFarPluginEnvGuard : public TObject
{
public:
  inline TFarPluginEnvGuard();
  inline ~TFarPluginEnvGuard();
};
//---------------------------------------------------------------------------
void FarWrapText(const UnicodeString & Text, TStrings * Result, intptr_t MaxWidth);
//---------------------------------------------------------------------------
extern TCustomFarPlugin * FarPlugin;
