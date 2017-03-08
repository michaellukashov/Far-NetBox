#pragma once

#pragma warning(push, 1)
#include <vcl.h>
#include <Sysutils.hpp>
#include <plugin.hpp>
#pragma warning(pop)

#include <Common.h>
#include "guid.h"

#define RMASK (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED | RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED | SHIFT_PRESSED)
#define ALTMASK (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED)
#define CTRLMASK (RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED)
#define SHIFTMASK (SHIFT_PRESSED)

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

const int MaxMessageWidth = 64;

enum TFarShiftStatus
{
  fsNone,
  fsCtrl,
  fsAlt,
  fsShift,
  fsCtrlShift,
  fsAltShift,
  fsCtrlAlt
};
enum THandlesFunction
{
  hfProcessKey,
  hfProcessHostFile,
  hfProcessPanelEvent
};

typedef nb::FastDelegate1<void, UnicodeString & /*Text*/> TFarInputBoxValidateEvent;

typedef nb::FastDelegate1<void, intptr_t & /*Result*/> TFarMessageTimerEvent;
typedef nb::FastDelegate3<void, void * /*Token*/,
  uintptr_t /*Result*/, bool & /*Close*/> TFarMessageClickEvent;

struct TFarMessageParams : public TObject
{
NB_DISABLE_COPY(TFarMessageParams)
public:
  TFarMessageParams();

  TStrings * MoreMessages;
  UnicodeString CheckBoxLabel;
  bool CheckBox;
  uintptr_t Timer;
  intptr_t TimerAnswer;
  TFarMessageTimerEvent TimerEvent;
  uintptr_t Timeout;
  uintptr_t TimeoutButton;
  uintptr_t DefaultButton;
  UnicodeString TimeoutStr;
  TFarMessageClickEvent ClickEvent;
  void * Token;
};

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

class TCustomFarPlugin : public TObject
{
friend class TCustomFarFileSystem;
friend class TFarDialog;
friend class TWinSCPFileSystem;
friend class TFarDialogItem;
friend class TFarMessageDialog;
friend class TFarPluginGuard;
NB_DISABLE_COPY(TCustomFarPlugin)
public:
  static inline bool classof(const TObject * Obj)
  {
    return
      Obj->GetKind() == OBJECT_CLASS_TCustomFarPlugin ||
      Obj->GetKind() == OBJECT_CLASS_TWinSCPPlugin;
  }
public:
  explicit TCustomFarPlugin(TObjectClassId Kind, HINSTANCE HInst);
  virtual ~TCustomFarPlugin();
  virtual VersionInfo GetMinFarVersion() const;
  virtual void SetStartupInfo(const struct PluginStartupInfo * Info);
  virtual const struct PluginStartupInfo * GetPluginStartupInfo() const { return &FStartupInfo; }
  virtual void ExitFAR();
  virtual void GetPluginInfo(struct PluginInfo * Info);
  virtual intptr_t Configure(const struct ConfigureInfo * Info);
  virtual void * OpenPlugin(const struct OpenInfo * Info);
  virtual void ClosePanel(void *Plugin);
  virtual void GetOpenPanelInfo(struct OpenPanelInfo * Info);
  virtual intptr_t GetFindData(struct GetFindDataInfo * Info);
  virtual void FreeFindData(const struct FreeFindDataInfo * Info);
  virtual intptr_t ProcessHostFile(const struct ProcessHostFileInfo * Info);
  virtual intptr_t ProcessPanelInput(const struct ProcessPanelInputInfo * Info);
  virtual intptr_t ProcessPanelEvent(const struct ProcessPanelEventInfo * Info);
  virtual intptr_t SetDirectory(const struct SetDirectoryInfo * Info);
  virtual intptr_t MakeDirectory(struct MakeDirectoryInfo * Info);
  virtual intptr_t DeleteFiles(const struct DeleteFilesInfo * Info);
  virtual intptr_t GetFiles(struct GetFilesInfo * Info);
  virtual intptr_t PutFiles(const struct PutFilesInfo * Info);
  virtual intptr_t ProcessEditorEvent(const struct ProcessEditorEventInfo * Info);
  virtual intptr_t ProcessEditorInput(const struct ProcessEditorInputInfo * Info);
  virtual void HandleException(Exception * E, OPERATION_MODES OpMode = 0);

  static wchar_t * DuplicateStr(const UnicodeString & Str, bool AllowEmpty = false);
  intptr_t Message(uintptr_t Flags, const UnicodeString & Title,
    const UnicodeString & Message, TStrings * Buttons = nullptr,
    TFarMessageParams * Params = nullptr);
  intptr_t MaxMessageLines();
  intptr_t MaxMenuItemLength();
  intptr_t Menu(FARMENUFLAGS Flags, const UnicodeString & Title,
    const UnicodeString & Bottom, TStrings * Items,
    const FarKey * BreakKeys,
    intptr_t & BreakCode);
  intptr_t Menu(FARMENUFLAGS Flags, const UnicodeString & Title,
    const UnicodeString & Bottom, TStrings * Items);
  intptr_t Menu(FARMENUFLAGS Flags, const UnicodeString & Title,
    const UnicodeString & Bottom, const FarMenuItem * Items, intptr_t Count,
    const FarKey * BreakKeys, intptr_t & BreakCode);
  bool InputBox(const UnicodeString & Title, const UnicodeString & Prompt,
    UnicodeString & Text, PLUGINPANELITEMFLAGS Flags, const UnicodeString & HistoryName = L"",
    intptr_t MaxLen = 255, TFarInputBoxValidateEvent OnValidate = nullptr);
  UnicodeString GetMsg(intptr_t MsgId) const;
  void SaveScreen(HANDLE & Screen);
  void RestoreScreen(HANDLE & Screen);
  bool CheckForEsc();
  bool Viewer(const UnicodeString & AFileName, const UnicodeString & Title, VIEWER_FLAGS Flags);
  bool Editor(const UnicodeString & AFileName, const UnicodeString & Title, EDITOR_FLAGS Flags);
  intptr_t FarControl(FILE_CONTROL_COMMANDS Command, intptr_t Param1, void * Param2, HANDLE Plugin = INVALID_HANDLE_VALUE);
  __int64 FarAdvControl(ADVANCED_CONTROL_COMMANDS Command, intptr_t Param1, void *Param2 = nullptr) const;
  intptr_t FarEditorControl(EDITOR_CONTROL_COMMANDS Command, intptr_t Param1, void * Param2) const;
  __int64 GetFarSystemSettings() const;
  void Text(int X, int Y, int Color, const UnicodeString & Str);
  void FlushText();
  void FarWriteConsole(const UnicodeString & Str);
  void FarCopyToClipboard(const UnicodeString & Str);
  void FarCopyToClipboard(const TStrings * Strings);
  intptr_t GetFarVersion() const;
  UnicodeString FormatFarVersion(VersionInfo & Info) const;
  UnicodeString GetTemporaryDir() const;
  int InputRecordToKey(const INPUT_RECORD * Rec);
  TFarEditorInfo * EditorInfo();

  void ShowConsoleTitle(const UnicodeString & Title);
  void ClearConsoleTitle();
  void UpdateConsoleTitle(const UnicodeString & Title);
  void UpdateConsoleTitleProgress(short Progress);
  void ShowTerminalScreen();
  void SaveTerminalScreen();
  void ScrollTerminalScreen(int Rows);
  TPoint TerminalInfo(TPoint * Size = nullptr, TPoint * Cursor = nullptr) const;
  uintptr_t ConsoleWindowState() const;
  void ToggleVideoMode();

  TCustomFarFileSystem * GetPanelFileSystem(bool Another = false,
    HANDLE Plugin = INVALID_HANDLE_VALUE);

  UnicodeString GetModuleName() const;
  TFarDialog * GetTopDialog() const { return FTopDialog; }
  HINSTANCE GetHandle() const { return FHandle; }
  uintptr_t GetFarThreadId() const { return FFarThreadId; }
  FarStandardFunctions & GetFarStandardFunctions() { return FFarStandardFunctions; }
  const struct PluginStartupInfo * GetStartupInfo() const { return &FStartupInfo; }

protected:
  PluginStartupInfo FStartupInfo;
  FarStandardFunctions FFarStandardFunctions;
  HINSTANCE FHandle;
  TObjectList * FOpenedPlugins;
  TFarDialog * FTopDialog;
  HANDLE FConsoleInput;
  HANDLE FConsoleOutput;
  mutable intptr_t FFarVersion;
  bool FTerminalScreenShowing;
  TCriticalSection FCriticalSection;
  uintptr_t FFarThreadId;
  mutable bool FValidFarSystemSettings;
  mutable intptr_t FFarSystemSettings;
  TPoint FNormalConsoleSize;

  virtual bool HandlesFunction(THandlesFunction Function) const;
  virtual void GetPluginInfoEx(PLUGIN_FLAGS & Flags,
    TStrings * DiskMenuStrings, TStrings * PluginMenuStrings,
    TStrings * PluginConfigStrings, TStrings * CommandPrefixes) = 0;
  virtual TCustomFarFileSystem * OpenPluginEx(OPENFROM OpenFrom, intptr_t Item) = 0;
  virtual bool ConfigureEx(const GUID * Guid) = 0;
  virtual intptr_t ProcessEditorEventEx(const struct ProcessEditorEventInfo * Info) = 0;
  virtual intptr_t ProcessEditorInputEx(const INPUT_RECORD * Rec) = 0;
  virtual void HandleFileSystemException(TCustomFarFileSystem * FarFileSystem,
    Exception * E, OPERATION_MODES OpMode = 0);
  void ResetCachedInfo();
  intptr_t MaxLength(TStrings * Strings);
  intptr_t FarMessage(uintptr_t Flags,
    const UnicodeString & Title, const UnicodeString & Message, TStrings * Buttons,
    TFarMessageParams * Params);
  intptr_t DialogMessage(uintptr_t Flags,
    const UnicodeString & Title, const UnicodeString & Message, TStrings * Buttons,
    TFarMessageParams * Params);
  void InvalidateOpenPanelInfo();

  const TCriticalSection & GetCriticalSection() const { return FCriticalSection; }

#ifdef NETBOX_DEBUG
public:
  void RunTests();
#endif
private:
  void UpdateProgress(intptr_t State, intptr_t Progress);
  __int64 GetSystemSetting(HANDLE & Settings, const wchar_t * Name) const;

private:
  PluginInfo FPluginInfo;
  TStringList * FSavedTitles;
  UnicodeString FCurrentTitle;
  short FCurrentProgress;

  void ClearPluginInfo(PluginInfo & Info);
  void UpdateCurrentConsoleTitle();
  UnicodeString FormatConsoleTitle();
  HWND GetConsoleWindow() const;
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

class TCustomFarFileSystem : public TObject
{
friend class TFarPanelInfo;
friend class TCustomFarPlugin;
NB_DISABLE_COPY(TCustomFarFileSystem)
public:
  static inline bool classof(const TObject * Obj)
  {
    return
      Obj->GetKind() == OBJECT_CLASS_TCustomFarFileSystem ||
      Obj->GetKind() == OBJECT_CLASS_TWinSCPFileSystem;
  }
public:
  explicit TCustomFarFileSystem(TObjectClassId Kind, TCustomFarPlugin * APlugin);
  void Init();
  virtual ~TCustomFarFileSystem();

  void GetOpenPanelInfo(struct OpenPanelInfo * Info);
  intptr_t GetFindData(struct GetFindDataInfo * Info);
  void FreeFindData(const struct FreeFindDataInfo * Info);
  intptr_t ProcessHostFile(const struct ProcessHostFileInfo * Info);
  intptr_t ProcessPanelInput(const struct ProcessPanelInputInfo * Info);
  intptr_t ProcessPanelEvent(intptr_t Event, void *Param);
  intptr_t SetDirectory(const struct SetDirectoryInfo * Info);
  intptr_t MakeDirectory(struct MakeDirectoryInfo * Info);
  intptr_t DeleteFiles(const struct DeleteFilesInfo * Info);
  intptr_t GetFiles(struct GetFilesInfo * Info);
  intptr_t PutFiles(const struct PutFilesInfo * Info);
  virtual void Close();

protected:
  virtual UnicodeString GetCurrDirectory() const = 0;

protected:
  TCustomFarPlugin * FPlugin;
  bool FClosed;

  virtual void GetOpenPanelInfoEx(OPENPANELINFO_FLAGS & Flags,
    UnicodeString & HostFile, UnicodeString & CurDir, UnicodeString & Format,
    UnicodeString & PanelTitle, TFarPanelModes * PanelModes, intptr_t & StartPanelMode,
    OPENPANELINFO_SORTMODES & StartSortMode, bool & StartSortOrder, TFarKeyBarTitles * KeyBarTitles,
    UnicodeString & ShortcutData) = 0;
  virtual bool GetFindDataEx(TObjectList * PanelItems, OPERATION_MODES OpMode) = 0;
  virtual bool ProcessHostFileEx(TObjectList * PanelItems, OPERATION_MODES OpMode);
  virtual bool ProcessKeyEx(intptr_t Key, uintptr_t ControlState);
  virtual bool ProcessPanelEventEx(intptr_t Event, void *Param);
  virtual bool SetDirectoryEx(const UnicodeString & Dir, OPERATION_MODES OpMode);
  virtual intptr_t MakeDirectoryEx(UnicodeString & Name, OPERATION_MODES OpMode);
  virtual bool DeleteFilesEx(TObjectList * PanelItems, OPERATION_MODES OpMode);
  virtual intptr_t GetFilesEx(TObjectList * PanelItems, bool Move,
    UnicodeString & DestPath, OPERATION_MODES OpMode);
  virtual intptr_t PutFilesEx(TObjectList * PanelItems, bool Move, OPERATION_MODES OpMode);

  void ResetCachedInfo();
  intptr_t FarControl(FILE_CONTROL_COMMANDS Command, intptr_t Param1, void * Param2);
  intptr_t FarControl(FILE_CONTROL_COMMANDS Command, intptr_t Param1, void * Param2, HANDLE Plugin);
  bool UpdatePanel(bool ClearSelection = false, bool Another = false);
  void RedrawPanel(bool Another = false);
  void ClosePanel();
  UnicodeString GetMsg(intptr_t MsgId) const;
  TCustomFarFileSystem * GetOppositeFileSystem();
  bool IsActiveFileSystem() const;
  bool IsLeft() const;
  bool IsRight() const;

  virtual void HandleException(Exception * E, OPERATION_MODES OpMode = 0);

  TFarPanelInfo * const * GetPanelInfo() const { return GetPanelInfo(0); }
  TFarPanelInfo ** GetPanelInfo() { return GetPanelInfo(0); }
  TFarPanelInfo * const * GetAnotherPanelInfo() const { return GetPanelInfo(1); }
  TFarPanelInfo ** GetAnotherPanelInfo() { return GetPanelInfo(1); }
  const TCriticalSection & GetCriticalSection() const { return FCriticalSection; }
  TCriticalSection & GetCriticalSection() { return FCriticalSection; }
  bool GetOpenPanelInfoValid() const { return FOpenPanelInfoValid; }

protected:
  TCriticalSection FCriticalSection;
  void InvalidateOpenPanelInfo();

private:
  UnicodeString FNameStr;
  UnicodeString FDestPathStr;
  OpenPanelInfo FOpenPanelInfo;
  bool FOpenPanelInfoValid;
  TFarPanelInfo * FPanelInfo[2];
  static uintptr_t FInstances;

  void ClearOpenPanelInfo(OpenPanelInfo & Info);
  TObjectList * CreatePanelItemList(struct PluginPanelItem * PanelItem,
    size_t ItemsNumber);
  TFarPanelInfo * const * GetPanelInfo(int Another) const;
  TFarPanelInfo ** GetPanelInfo(int Another);
};

#define PANEL_MODES_COUNT 10
class TFarPanelModes : public TObject
{
friend class TCustomFarFileSystem;
public:
  TFarPanelModes();
  virtual ~TFarPanelModes();

  void SetPanelMode(size_t Mode, const UnicodeString & ColumnTypes = UnicodeString(),
    const UnicodeString & ColumnWidths = UnicodeString(), TStrings * ColumnTitles = nullptr,
    bool FullScreen = false, bool DetailedStatus = true, bool AlignExtensions = true,
    bool CaseConversion = true, const UnicodeString & StatusColumnTypes = UnicodeString(),
    const UnicodeString & StatusColumnWidths = UnicodeString());

private:
  PanelMode FPanelModes[PANEL_MODES_COUNT];
  bool FReferenced;

  void FillOpenPanelInfo(struct OpenPanelInfo * Info);
  void SetFlag(PANELMODE_FLAGS & Flags, bool Value, PANELMODE_FLAGS Flag);
  static void ClearPanelMode(PanelMode & Mode);
  static intptr_t CommaCount(const UnicodeString & ColumnTypes);
};

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

  void FillOpenPanelInfo(struct OpenPanelInfo * Info);
  static void ClearKeyBarTitles(KeyBarTitles & Titles);
};

class TCustomFarPanelItem : public TObject
{
friend class TCustomFarFileSystem;
public:
  static inline bool classof(const TObject * Obj)
  {
    return
      Obj->GetKind() == OBJECT_CLASS_TCustomFarPanelItem ||
      Obj->GetKind() == OBJECT_CLASS_TFarPanelItem ||
      Obj->GetKind() == OBJECT_CLASS_THintPanelItem ||
      Obj->GetKind() == OBJECT_CLASS_TSessionPanelItem ||
      Obj->GetKind() == OBJECT_CLASS_TSessionFolderPanelItem ||
      Obj->GetKind() == OBJECT_CLASS_TRemoteFilePanelItem;
  }
protected:
  explicit TCustomFarPanelItem(TObjectClassId Kind) : TObject(Kind) {}
  virtual ~TCustomFarPanelItem()
  {}
  virtual void GetData(
    PLUGINPANELITEMFLAGS & Flags, UnicodeString & AFileName, int64_t & Size,
    uintptr_t & FileAttributes,
    TDateTime & LastWriteTime, TDateTime & LastAccess,
    uintptr_t & NumberOfLinks, UnicodeString & Description,
    UnicodeString & Owner, void *& UserData, size_t & CustomColumnNumber) = 0;
  virtual UnicodeString GetCustomColumnData(size_t Column);

  void FillPanelItem(struct PluginPanelItem * PanelItem);
};

class TFarPanelItem : public TCustomFarPanelItem
{
NB_DISABLE_COPY(TFarPanelItem)
public:
  static inline bool classof(const TObject * Obj)
  {
    return
      Obj->GetKind() == OBJECT_CLASS_TFarPanelItem;
  }
public:
  explicit TFarPanelItem(PluginPanelItem * APanelItem, bool OwnsItem);
  virtual ~TFarPanelItem();

  PLUGINPANELITEMFLAGS GetFlags() const;
  uintptr_t GetFileAttrs() const;
  UnicodeString GetFileName() const;
  void * GetUserData() const;
  bool GetSelected() const;
  void SetSelected(bool Value);
  bool GetIsParentDirectory() const;
  bool GetIsFile() const;

protected:
  PluginPanelItem * FPanelItem;
  bool FOwnsItem;

  virtual void GetData(
    PLUGINPANELITEMFLAGS & Flags, UnicodeString & AFileName, int64_t & Size,
    uintptr_t & FileAttributes,
    TDateTime & LastWriteTime, TDateTime & LastAccess,
    uintptr_t & NumberOfLinks, UnicodeString & Description,
    UnicodeString & Owner, void *& UserData, size_t & CustomColumnNumber);
  virtual UnicodeString GetCustomColumnData(size_t Column);
};

class THintPanelItem : public TCustomFarPanelItem
{
public:
  explicit THintPanelItem(const UnicodeString & AHint);
  virtual ~THintPanelItem() {}

protected:
  virtual void GetData(
    PLUGINPANELITEMFLAGS & Flags, UnicodeString & AFileName, int64_t & Size,
    uintptr_t & FileAttributes,
    TDateTime & LastWriteTime, TDateTime & LastAccess,
    uintptr_t & NumberOfLinks, UnicodeString & Description,
    UnicodeString & Owner, void *& UserData, size_t & CustomColumnNumber);

private:
  UnicodeString FHint;
};

enum TFarPanelType
{
  ptFile,
  ptTree,
  ptQuickView,
  ptInfo
};

class TFarPanelInfo : public TObject
{
NB_DISABLE_COPY(TFarPanelInfo)
public:
  explicit TFarPanelInfo(PanelInfo * APanelInfo, TCustomFarFileSystem * AOwner);
  virtual ~TFarPanelInfo();

  const TObjectList * GetItems() const { return const_cast<TFarPanelInfo *>(this)->GetItems(); }
  TObjectList * GetItems();
  intptr_t GetItemCount() const;
  TFarPanelItem * GetFocusedItem() const;
  void SetFocusedItem(const TFarPanelItem * Value);
  intptr_t GetFocusedIndex() const;
  void SetFocusedIndex(intptr_t Value);
  intptr_t GetSelectedCount(bool CountCurrentItem = false) const;
  TRect GetBounds() const;
  TFarPanelType GetType() const;
  bool GetIsPlugin() const;
  UnicodeString GetCurrDirectory() const;

  void ApplySelection();
  TFarPanelItem * FindFileName(const UnicodeString & AFileName) const;
  const TFarPanelItem * FindUserData(const void * UserData) const;
  TFarPanelItem * FindUserData(const void * UserData);

private:
  PanelInfo * FPanelInfo;
  TObjectList * FItems;
  TCustomFarFileSystem * FOwner;
};

class TFarMenuItems : public TStringList
{
public:
  static inline bool classof(const TObject * Obj)
  {
    return
      Obj->GetKind() == OBJECT_CLASS_TFarMenuItems;
  }
public:
  explicit TFarMenuItems();
  virtual ~TFarMenuItems() {}
  void AddSeparator(bool Visible = true);
  virtual intptr_t Add(const UnicodeString & Text, bool Visible = true);

  virtual void Clear();
  virtual void Delete(intptr_t Index);

  intptr_t GetItemFocused() const { return FItemFocused; }
  void SetItemFocused(intptr_t Value);
  bool GetDisabled(intptr_t Index) const { return GetFlag(Index, MIF_DISABLE); }
  void SetDisabled(intptr_t Index, bool Value) { SetFlag(Index, MIF_DISABLE, Value); }
  bool GetChecked(intptr_t Index) const { return GetFlag(Index, MIF_CHECKED); }
  void SetChecked(intptr_t Index, bool Value) { SetFlag(Index, MIF_CHECKED, Value); }

  void SetFlag(intptr_t Index, uintptr_t Flag, bool Value);
  bool GetFlag(intptr_t Index, uintptr_t Flag) const;

protected:
  virtual void SetObj(intptr_t Index, TObject * AObject);

private:
  intptr_t FItemFocused;
};

class TFarEditorInfo : public TObject
{
NB_DISABLE_COPY(TFarEditorInfo)
public:
  explicit TFarEditorInfo(EditorInfo * Info);
  ~TFarEditorInfo();

  intptr_t GetEditorID() const;
  static UnicodeString GetFileName();

private:
  EditorInfo * FEditorInfo;
};

class TFarEnvGuard // : public TObject
{
CUSTOM_MEM_ALLOCATION_IMPL
NB_DISABLE_COPY(TFarEnvGuard)
public:
  TFarEnvGuard();
  ~TFarEnvGuard();
};

class TFarPluginEnvGuard // : public TObject
{
CUSTOM_MEM_ALLOCATION_IMPL
NB_DISABLE_COPY(TFarPluginEnvGuard)
public:
  TFarPluginEnvGuard();
  ~TFarPluginEnvGuard();
};

void FarWrapText(const UnicodeString & Text, TStrings * Result, intptr_t MaxWidth);

extern TCustomFarPlugin * FarPlugin;

class TGlobalFunctions : public TGlobalFunctionsIntf, public TObject
{
public:
  virtual HINSTANCE GetInstanceHandle() const;
  virtual UnicodeString GetMsg(intptr_t Id) const;
  virtual UnicodeString GetCurrDirectory() const;
  virtual UnicodeString GetStrVersionNumber() const;
  virtual bool InputDialog(const UnicodeString & ACaption,
    const UnicodeString & APrompt, UnicodeString & Value, const UnicodeString & HelpKeyword,
    TStrings * History, bool PathInput,
    TInputDialogInitializeEvent OnInitialize, bool Echo);
  virtual uintptr_t MoreMessageDialog(const UnicodeString & Message,
    TStrings * MoreMessages, TQueryType Type, uintptr_t Answers,
      const TMessageParams * Params);
};

