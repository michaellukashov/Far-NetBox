#pragma once

#pragma warning(push, 1)
#include <vcl.h>
#include <Sysutils.hpp>
#include <System.SyncObjs.hpp>
#include <plugin.hpp>
#pragma warning(pop)

#include <Common.h>
#include "guid.h"

#undef GetStartupInfo

constexpr const DWORD RMASK = (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED | RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED | SHIFT_PRESSED);
constexpr const DWORD ALTMASK = (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED);
constexpr const DWORD CTRLMASK = (RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED);
constexpr const DWORD SHIFTMASK = (SHIFT_PRESSED);
constexpr const wchar_t * SHORTCUT_DELIMITER = L"\1";

template<typename... TMasks>
bool CheckControlMaskSet(DWORD State, TMasks... Masks)
{
  const auto CtlState = State & RMASK;
  // 1) check all individual masks are present in the ctl state
  // 2) check all bits that are not in any mask are cleared 
  return ((CtlState & Masks) && ...) && !(CtlState & ~(Masks | ...));
}

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

constexpr const int32_t MaxMessageWidth = 64;

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

using TFarInputBoxValidateEvent = nb::FastDelegate1<void,
  UnicodeString & /*Text*/>;
using TFarMessageTimerEvent = nb::FastDelegate1<void,
  uint32_t & /*Result*/>;
using TFarMessageClickEvent = nb::FastDelegate3<void,
  void * /*Token*/, int32_t /*Result*/, bool & /*Close*/>;
using TSynchroEvent = nb::FastDelegate2<void,
  TObject * /*Sender*/, void * /*Data*/>;

struct TFarMessageParams : public TObject
{
  NB_DISABLE_COPY(TFarMessageParams)
public:
  TFarMessageParams() = default;

  TStrings * MoreMessages{nullptr};
  UnicodeString CheckBoxLabel;
  bool CheckBox{false};
  uint32_t Timer{0};
  uint32_t TimerAnswer{0};
  TFarMessageTimerEvent TimerEvent;
  uint32_t Timeout{0};
  int32_t TimeoutButton{0};
  int32_t DefaultButton{0};
  UnicodeString TimeoutStr;
  TFarMessageClickEvent ClickEvent;
  void * Token{nullptr};
};

struct TSynchroParams final : public TObject
{
  NB_DISABLE_COPY(TSynchroParams)
public:
  TSynchroParams() = default;
  TSynchroEvent SynchroEvent;
  TFarDialog * Sender{nullptr};
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

class TGlobalFunctions;
class TPluginIdleThread;

class TCustomFarPlugin : public TObject
{
  friend class TCustomFarFileSystem;
  friend class TFarDialog;
  friend class TWinSCPFileSystem;
  friend class TFarDialogItem;
  friend class TFarMessageDialog;
  friend class TQueueDialog;
  friend class TFarPluginGuard;
  NB_DISABLE_COPY(TCustomFarPlugin)
public:
  static bool classof(const TObject * Obj) { return Obj->is(OBJECT_CLASS_TCustomFarPlugin); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TCustomFarPlugin) || TObject::is(Kind); }
  TCustomFarPlugin() = delete;
  VersionInfo GetMinFarVersion() const;
public:
  explicit TCustomFarPlugin(TObjectClassId Kind, HINSTANCE HInst) noexcept;
  virtual ~TCustomFarPlugin() noexcept override;
  virtual void Initialize();
  virtual void Finalize();

  virtual void SetStartupInfo(const struct PluginStartupInfo * Info);
  virtual const struct PluginStartupInfo * GetPluginStartupInfo() const { return &FStartupInfo; }
  virtual void ExitFAR();
  virtual void GetPluginInfo(struct PluginInfo * Info);
  virtual intptr_t ProcessSynchroEvent(const struct ProcessSynchroEventInfo * Info);
  virtual int32_t Configure(const struct ConfigureInfo * Info);
  virtual void * OpenPlugin(const struct OpenInfo * Info);
  virtual void ClosePanel(void * Plugin);
  virtual void GetOpenPanelInfo(struct OpenPanelInfo * Info);
  virtual int32_t GetFindData(struct GetFindDataInfo * Info);
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
  int32_t Message(uint32_t Flags, const UnicodeString & Title,
    const UnicodeString & Message, TStrings * Buttons = nullptr,
    TFarMessageParams * Params = nullptr);
  int32_t MaxMessageLines() const;
  int32_t MaxMenuItemLength() const;
  intptr_t Menu(FARMENUFLAGS Flags, const UnicodeString & Title,
    const UnicodeString & Bottom, TStrings * Items);
  intptr_t Menu(FARMENUFLAGS Flags, const UnicodeString & Title,
    const UnicodeString & Bottom, TStrings * Items,
    const FarKey * BreakKeys, intptr_t & BreakCode);
  intptr_t Menu(FARMENUFLAGS Flags, const UnicodeString & Title,
    const UnicodeString & Bottom, const FarMenuItem * Items, size_t Count,
    const FarKey * BreakKeys, intptr_t & BreakCode);
  bool InputBox(const UnicodeString & Title, const UnicodeString & Prompt,
    UnicodeString & Text, PLUGINPANELITEMFLAGS Flags, const UnicodeString & HistoryName = L"",
    int32_t MaxLen = 255, TFarInputBoxValidateEvent && OnValidate = nullptr);
  virtual UnicodeString GetMsg(intptr_t MsgId) const;
  void SaveScreen(HANDLE & Screen);
  void RestoreScreen(HANDLE & Screen);
  bool CheckForEsc() const;
  bool Viewer(const UnicodeString & AFileName, const UnicodeString & Title, VIEWER_FLAGS Flags);
  bool Editor(const UnicodeString & AFileName, const UnicodeString & Title, EDITOR_FLAGS Flags);
  intptr_t FarControl(FILE_CONTROL_COMMANDS Command, intptr_t Param1, void * Param2, HANDLE Plugin = INVALID_HANDLE_VALUE);
  intptr_t FarAdvControl(ADVANCED_CONTROL_COMMANDS Command, intptr_t Param1, void * Param2 = nullptr) const;
  intptr_t FarEditorControl(EDITOR_CONTROL_COMMANDS Command, intptr_t Param1, void * Param2) const;
  intptr_t GetFarSystemSettings() const;
  void Text(int32_t X, int32_t Y, int32_t Color, const UnicodeString & Str);
  void FlushText();
  void FarWriteConsole(const UnicodeString & Str);
  void FarCopyToClipboard(const UnicodeString & Str);
  void FarCopyToClipboard(const TStrings * Strings);
  int32_t GetFarVersion() const;
  UnicodeString FormatFarVersion(const VersionInfo & Info) const;
  UnicodeString GetTemporaryDir() const;
  intptr_t InputRecordToKey(const INPUT_RECORD * Rec);
  TFarEditorInfo * EditorInfo();

  void ShowConsoleTitle(const UnicodeString & Title);
  void ClearConsoleTitle();
  void UpdateConsoleTitle(const UnicodeString & Title);
  void UpdateConsoleTitleProgress(int16_t Progress);
  void ShowTerminalScreen(const UnicodeString & Command);
  void SaveTerminalScreen();
  void ScrollTerminalScreen(int32_t Rows);
  TPoint TerminalInfo(TPoint * Size = nullptr, TPoint * Cursor = nullptr) const;
  uint32_t ConsoleWindowState() const;
  void ToggleVideoMode();

  TCustomFarFileSystem * GetPanelFileSystem(bool Another = false,
    HANDLE Plugin = INVALID_HANDLE_VALUE);

  virtual UnicodeString GetModuleName() const;
  TFarDialog * GetTopDialog() const { return FTopDialog; }
  HINSTANCE GetPluginHandle() const { return FPluginHandle; }
  uint32_t GetFarThreadId() const { return FFarThreadId; }
  const FarStandardFunctions & GetFarStandardFunctions() const { return FFarStandardFunctions; }
  const struct PluginStartupInfo * GetStartupInfo() const { return &FStartupInfo; }

protected:
  TGlobalsIntfInitializer<TGlobalFunctions> FGlobalsIntfInitializer;
  PluginStartupInfo FStartupInfo{};
  FarStandardFunctions FFarStandardFunctions{};
  HINSTANCE FPluginHandle{};
  std::unique_ptr<TList> FOpenedPlugins;
  gsl::owner<TFarDialog *> FTopDialog{nullptr};
  HANDLE FConsoleInput{};
  HANDLE FConsoleOutput{};
  mutable int32_t FFarVersion{0};
  bool FTerminalScreenShowing{false};
  TCriticalSection FCriticalSection;
  uint32_t FFarThreadId{0};
  mutable bool FValidFarSystemSettings{false};
  mutable int32_t FFarSystemSettings{0};
  TPoint FNormalConsoleSize;
  TSynchroParams FSynchroParams{};

  virtual bool HandlesFunction(THandlesFunction Function) const;
  virtual void GetPluginInfoEx(PLUGIN_FLAGS & Flags,
    TStrings * DiskMenuStrings, TStrings * PluginMenuStrings,
    TStrings * PluginConfigStrings, TStrings * CommandPrefixes) = 0;
  virtual TCustomFarFileSystem * OpenPluginEx(OPENFROM OpenFrom, intptr_t Item) = 0;
  virtual bool ConfigureEx(const GUID * Guid) = 0;
  virtual int32_t ProcessEditorEventEx(const struct ProcessEditorEventInfo * Info) = 0;
  virtual int32_t ProcessEditorInputEx(const INPUT_RECORD * Rec) = 0;
  virtual void HandleFileSystemException(TCustomFarFileSystem * FarFileSystem,
    Exception * E, OPERATION_MODES OpMode = 0);
  void ResetCachedInfo();
  int32_t MaxLength(TStrings * Strings) const;
  int32_t FarMessage(uint32_t Flags,
    const UnicodeString & Title, const UnicodeString & Message, TStrings * Buttons,
    TFarMessageParams * Params);
  int32_t DialogMessage(uint32_t Flags,
    const UnicodeString & Title, const UnicodeString & Message, TStrings * Buttons,
    gsl::not_null<TFarMessageParams *> Params);
  void InvalidateOpenPanelInfo();

  const TCriticalSection & GetCriticalSection() const { return FCriticalSection; }

private:
  void UpdateProgress(int32_t State, int32_t Progress) const;
  int64_t GetSystemSetting(HANDLE & Settings, const wchar_t * Name) const;

private:
  PluginInfo FPluginInfo{};
  std::unique_ptr<TPluginIdleThread> FTIdleThread;
  std::unique_ptr<TStringList> FSavedTitles;
  UnicodeString FCurrentTitle;
  int16_t FCurrentProgress{0};

  void ClearPluginInfo(PluginInfo & Info) const;
  void UpdateCurrentConsoleTitle();
  UnicodeString FormatConsoleTitle() const;
  HWND GetConsoleWindow() const;
  RECT GetPanelBounds(HANDLE PanelHandle);
  static bool CompareRects(const RECT & lhs, const RECT & rhs)
  {
    return
      lhs.left == rhs.left &&
      lhs.top == rhs.top &&
      lhs.right == rhs.right &&
      lhs.bottom == rhs.bottom;
  }
  void CloseFileSystem(TCustomFarFileSystem * FileSystem);
};

class TCustomFarFileSystem : public TObject
{
  friend class TFarPanelInfo;
  friend class TCustomFarPlugin;
  NB_DISABLE_COPY(TCustomFarFileSystem)
public:
  static bool classof(const TObject * Obj) { return Obj->is(OBJECT_CLASS_TCustomFarFileSystem); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TCustomFarFileSystem) || TObject::is(Kind); }
public:
  explicit TCustomFarFileSystem(TObjectClassId Kind, gsl::not_null<TCustomFarPlugin *> APlugin) noexcept;
  virtual ~TCustomFarFileSystem() noexcept override;
  void Init();

  void GetOpenPanelInfo(struct OpenPanelInfo * Info);
  int32_t GetFindData(struct GetFindDataInfo * Info);
  void FreeFindData(const struct FreeFindDataInfo * Info);
  int32_t ProcessHostFile(const struct ProcessHostFileInfo * Info);
  int32_t ProcessPanelInput(const struct ProcessPanelInputInfo * Info);
  bool ProcessPanelEvent(intptr_t Event, void * Param);
  int32_t SetDirectory(const struct SetDirectoryInfo * Info);
  int32_t MakeDirectory(struct MakeDirectoryInfo * Info);
  int32_t DeleteFiles(const struct DeleteFilesInfo * Info);
  int32_t GetFiles(struct GetFilesInfo * Info);
  int32_t PutFiles(const struct PutFilesInfo * Info);
  virtual void Close();

protected:
  virtual UnicodeString GetCurrentDirectory() const = 0;

protected:

  virtual void GetOpenPanelInfoEx(OPENPANELINFO_FLAGS & Flags,
    UnicodeString & HostFile, UnicodeString & CurDir, UnicodeString & Format,
    UnicodeString & PanelTitle, TFarPanelModes * PanelModes, intptr_t & StartPanelMode,
    OPENPANELINFO_SORTMODES & StartSortMode, bool & StartSortOrder, TFarKeyBarTitles * KeyBarTitles,
    UnicodeString & ShortcutData) = 0;
  virtual bool GetFindDataEx(TObjectList * PanelItems, OPERATION_MODES OpMode) = 0;
  virtual bool ProcessHostFileEx(TObjectList * PanelItems, OPERATION_MODES OpMode);
  virtual bool ProcessKeyEx(int32_t Key, uint32_t ControlState);
  virtual bool ProcessPanelEventEx(intptr_t Event, void * Param);
  virtual bool SetDirectoryEx(const UnicodeString & Dir, OPERATION_MODES OpMode);
  virtual int32_t MakeDirectoryEx(UnicodeString & Name, OPERATION_MODES OpMode);
  virtual bool DeleteFilesEx(TObjectList * PanelItems, OPERATION_MODES OpMode);
  virtual int32_t GetFilesEx(TObjectList * PanelItems, bool Move,
    UnicodeString & DestPath, OPERATION_MODES OpMode);
  virtual int32_t PutFilesEx(TObjectList * PanelItems, bool Move, OPERATION_MODES OpMode);

  void ResetCachedInfo();
  int32_t FarControl(FILE_CONTROL_COMMANDS Command, intptr_t Param1, void * Param2);
  int32_t FarControl(FILE_CONTROL_COMMANDS Command, intptr_t Param1, void * Param2, HANDLE Plugin);
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
  void InvalidateOpenPanelInfo();
  TCustomFarFileSystem * GetOwnerFileSystem() { return FOwnerFileSystem; }
  void SetOwnerFileSystem(TCustomFarFileSystem * Value) { FOwnerFileSystem = Value; }
  bool GetClosed() const { return FClosed; }
  TCustomFarPlugin * GetPlugin() const { return FPlugin; }
  bool IsConnectedDirectly() const { return FConnectedDirectly; };
  void SetConnectedDirectly(bool Value = true) { FConnectedDirectly = Value; }

private:
  TCriticalSection FCriticalSection;
  UnicodeString FNameStr;
  UnicodeString FDestPathStr;
  OpenPanelInfo FOpenPanelInfo{};
  bool FOpenPanelInfoValid{false};
  TCustomFarFileSystem * FOwnerFileSystem{nullptr};
  TFarPanelInfo * FPanelInfo[2]{};
  gsl::not_null<TCustomFarPlugin *> FPlugin;
  bool FClosed{false};
  static uint32_t FInstances;
  bool FConnectedDirectly{false};

  void ClearOpenPanelInfo(OpenPanelInfo & Info);
  TObjectList * CreatePanelItemList(struct PluginPanelItem * PanelItem, int32_t ItemsNumber);
  TFarPanelInfo * const * GetPanelInfo(int32_t Another) const;
  TFarPanelInfo ** GetPanelInfo(int32_t Another);
};

constexpr const int32_t PANEL_MODES_COUNT = 10;
class TFarPanelModes final : public TObject
{
  friend class TCustomFarFileSystem;
public:
  TFarPanelModes() noexcept;
  virtual ~TFarPanelModes() noexcept override;

  void SetPanelMode(size_t Mode, const UnicodeString & ColumnTypes = UnicodeString(),
    const UnicodeString & ColumnWidths = UnicodeString(), TStrings * ColumnTitles = nullptr,
    bool FullScreen = false, bool DetailedStatus = true, bool AlignExtensions = true,
    bool CaseConversion = true, const UnicodeString & StatusColumnTypes = UnicodeString(),
    const UnicodeString & StatusColumnWidths = UnicodeString());

private:
  PanelMode FPanelModes[PANEL_MODES_COUNT]{};
  bool FReferenced{false};

  void FillOpenPanelInfo(struct OpenPanelInfo * Info);
  void SetFlag(PANELMODE_FLAGS & Flags, bool Value, PANELMODE_FLAGS Flag);
  static void ClearPanelMode(PanelMode & Mode);
  static int32_t CommaCount(const UnicodeString & ColumnTypes);
};

class TFarKeyBarTitles final : public TObject
{
  friend class TCustomFarFileSystem;
public:
  TFarKeyBarTitles() noexcept;
  virtual ~TFarKeyBarTitles() noexcept override;

  void ClearFileKeyBarTitles();
  void ClearKeyBarTitle(TFarShiftStatus ShiftStatus,
    int32_t FunctionKeyStart, int32_t FunctionKeyEnd = 0);
  void SetKeyBarTitle(TFarShiftStatus ShiftStatus, int32_t FunctionKey,
    const UnicodeString & Title);

private:
  KeyBarTitles FKeyBarTitles{};
  bool FReferenced{false};

  void FillOpenPanelInfo(struct OpenPanelInfo * Info);
  static void ClearKeyBarTitles(KeyBarTitles & Titles);
};

class TCustomFarPanelItem : public TObject
{
  friend class TCustomFarFileSystem;
public:
  static bool classof(const TObject * Obj) { return Obj->is(OBJECT_CLASS_TCustomFarPanelItem); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TCustomFarPanelItem) || TObject::is(Kind); }
protected:
  TCustomFarPanelItem() = delete;
  explicit TCustomFarPanelItem(TObjectClassId Kind) noexcept : TObject(Kind) {}
  virtual ~TCustomFarPanelItem() override = default;
  virtual void GetData(
    PLUGINPANELITEMFLAGS & Flags, UnicodeString & AFileName, int64_t & Size,
    uintptr_t & FileAttributes,
    TDateTime & LastWriteTime, TDateTime & LastAccess,
    uintptr_t & NumberOfLinks, UnicodeString & Description,
    UnicodeString & Owner, void *& UserData, size_t & CustomColumnNumber) = 0;
  virtual UnicodeString GetCustomColumnData(size_t Column);

  void FillPanelItem(struct PluginPanelItem * PanelItem);
};

class TFarPanelItem final : public TCustomFarPanelItem
{
  NB_DISABLE_COPY(TFarPanelItem)
public:
  static bool classof(const TObject * Obj) { return Obj->is(OBJECT_CLASS_TFarPanelItem); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TFarPanelItem) || TCustomFarPanelItem::is(Kind); }
public:
  TFarPanelItem() = delete;
  explicit TFarPanelItem(PluginPanelItem * APanelItem, bool OwnsItem) noexcept;
  virtual ~TFarPanelItem() noexcept override;

  PLUGINPANELITEMFLAGS GetFlags() const;
  uint32_t GetFileAttrs() const;
  UnicodeString GetFileName() const;
  void * GetUserData() const;
  bool GetSelected() const;
  void SetSelected(bool Value);
  bool GetIsParentDirectory() const;
  bool GetIsFile() const;

protected:
  PluginPanelItem * FPanelItem{nullptr};
  bool FOwnsItem{false};

  virtual void GetData(
    PLUGINPANELITEMFLAGS & Flags, UnicodeString & AFileName, int64_t & Size,
    uintptr_t & FileAttributes,
    TDateTime & LastWriteTime, TDateTime & LastAccess,
    uintptr_t & NumberOfLinks, UnicodeString & Description,
    UnicodeString & Owner, void *& UserData, size_t & CustomColumnNumber) override;
  virtual UnicodeString GetCustomColumnData(size_t Column) override;
};

class THintPanelItem final : public TCustomFarPanelItem
{
public:
  THintPanelItem() = delete;
  explicit THintPanelItem(const UnicodeString & AHint) noexcept;
  virtual ~THintPanelItem() override = default;

protected:
  virtual void GetData(
    PLUGINPANELITEMFLAGS & Flags, UnicodeString & AFileName, int64_t & Size,
    uintptr_t & FileAttributes,
    TDateTime & LastWriteTime, TDateTime & LastAccess,
    uintptr_t & NumberOfLinks, UnicodeString & Description,
    UnicodeString & Owner, void *& UserData, size_t & CustomColumnNumber) override;

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

class TFarPanelInfo final : public TObject
{
  NB_DISABLE_COPY(TFarPanelInfo)
public:
  TFarPanelInfo() = delete;
  explicit TFarPanelInfo(PanelInfo * APanelInfo, TCustomFarFileSystem * AOwner) noexcept;
  virtual ~TFarPanelInfo() noexcept override;

  TObjectList * GetItems() const;
  TObjectList * GetItems() { return std::as_const(*this).GetItems(); }
  int32_t GetItemCount() const;
  const TFarPanelItem * GetFocusedItem() const;
  void SetFocusedItem(const TFarPanelItem * Value);
  int32_t GetFocusedIndex() const;
  void SetFocusedIndex(int32_t Value);
  int32_t GetSelectedCount(bool CountCurrentItem = false) const;
  TRect GetBounds() const;
  TFarPanelType GetType() const;
  bool GetIsPlugin() const;
  UnicodeString GetCurrentDirectory() const;

  void ApplySelection();
  const TFarPanelItem * FindFileName(const UnicodeString & AFileName) const;
  const TFarPanelItem * FindUserData(const void * UserData) const;
  TFarPanelItem * FindUserData(const void * UserData);

private:
  gsl::owner<PanelInfo *> FPanelInfo{nullptr};
  mutable TObjectList * FItems{nullptr};
  TCustomFarFileSystem * FOwner{nullptr};
};

class TFarMenuItems final : public TStringList
{
public:
  static bool classof(const TObject * Obj) { return Obj->is(OBJECT_CLASS_TFarMenuItems); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TFarMenuItems) || TStringList::is(Kind); }
public:
  explicit TFarMenuItems() noexcept;
  virtual ~TFarMenuItems() override = default;
  void AddSeparator(bool Visible = true);
  int32_t AddString(const UnicodeString & Text, bool Visible = true);

  virtual void Clear() override;
  virtual void Delete(int32_t Index) override;

  int32_t GetItemFocused() const { return FItemFocused; }
  void SetItemFocused(int32_t Value);
  bool GetDisabled(int32_t Index) const { return GetFlag(Index, MIF_DISABLE); }
  void SetDisabled(int32_t Index, bool Value) { SetFlag(Index, MIF_DISABLE, Value); }
  bool GetChecked(int32_t Index) const { return GetFlag(Index, MIF_CHECKED); }
  void SetChecked(int32_t Index, bool Value) { SetFlag(Index, MIF_CHECKED, Value); }

  void SetFlag(int32_t Index, uint32_t Flag, bool Value);
  bool GetFlag(int32_t Index, uint32_t Flag) const;

protected:
  virtual void SetObject(int32_t Index, TObject * AObject) override;

private:
  int32_t FItemFocused{nb::NPOS};
};

class TFarEditorInfo final : public TObject
{
  CUSTOM_MEM_ALLOCATION_IMPL
  NB_DISABLE_COPY(TFarEditorInfo)
public:
  TFarEditorInfo() = delete;
  explicit TFarEditorInfo(gsl::not_null<EditorInfo *> Info) noexcept;
  virtual ~TFarEditorInfo() noexcept override;

  int32_t GetEditorID() const;
  static UnicodeString GetFileName();

private:
  gsl::owner<EditorInfo *> FEditorInfo{nullptr};
};

class TFarEnvGuard // : public TObject
{
  CUSTOM_MEM_ALLOCATION_IMPL
  NB_DISABLE_COPY(TFarEnvGuard)
public:
  TFarEnvGuard() noexcept;
  ~TFarEnvGuard() noexcept;
};

class TFarPluginEnvGuard // : public TObject
{
  CUSTOM_MEM_ALLOCATION_IMPL
  NB_DISABLE_COPY(TFarPluginEnvGuard)
public:
  TFarPluginEnvGuard() noexcept;
  ~TFarPluginEnvGuard() noexcept;
};

extern TCustomFarPlugin * FarPlugin;

class TGlobalFunctions : public TGlobals
{
public:
  virtual HINSTANCE GetInstanceHandle() const override;
  virtual UnicodeString GetMsg(int32_t Id) const override;
  virtual UnicodeString GetCurrentDirectory() const override;
  virtual UnicodeString GetStrVersionNumber() const override;
  virtual bool InputDialog(const UnicodeString & ACaption,
    const UnicodeString & APrompt, UnicodeString & Value, const UnicodeString & HelpKeyword,
    TStrings * History, bool PathInput,
    TInputDialogInitializeEvent && OnInitialize, bool Echo) override;
  virtual uint32_t MoreMessageDialog(const UnicodeString & AMessage,
    TStrings * MoreMessages, TQueryType Type, uint32_t Answers,
    const TMessageParams * Params) override;
};

