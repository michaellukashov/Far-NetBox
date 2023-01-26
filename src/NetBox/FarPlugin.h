#pragma once

#pragma warning(push, 1)
#include <vcl.h>
#include <Sysutils.hpp>
#include <System.SyncObjs.hpp>
#include <plugin.hpp>
#pragma warning(pop)

#include <Common.h>

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

constexpr int MaxMessageWidth = 64;

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
  hfProcessEvent
};

#if 0
typedef void (__closure *TFarInputBoxValidateEvent)
(AnsiString &Text);
#endif // #if 0
using TFarInputBoxValidateEvent = nb::FastDelegate1<void, UnicodeString & /*Text*/>;
#if 0
typedef void (__closure *TFarMessageTimerEvent)(unsigned int &Result);
#endif // #if 0
using TFarMessageTimerEvent = nb::FastDelegate1<void, uint32_t & /*Result*/>;
#if 0
typedef void (__closure *TFarMessageClickEvent)(void *Token, int Result, bool &Close);
#endif // #if 0
using TFarMessageClickEvent = nb::FastDelegate3<void, void * /*Token*/,
        uint32_t /*Result*/, bool & /*Close*/>;

struct TFarMessageParams : public TObject
{
  NB_DISABLE_COPY(TFarMessageParams)
public:
  TFarMessageParams() = default;

  TStrings *MoreMessages{nullptr};
  UnicodeString CheckBoxLabel;
  bool CheckBox{false};
  uint32_t Timer{0};
  uint32_t TimerAnswer{0};
  TFarMessageTimerEvent TimerEvent;
  uint32_t Timeout{0};
  uint32_t TimeoutButton{0};
  uint32_t DefaultButton{0};
  UnicodeString TimeoutStr;
  TFarMessageClickEvent ClickEvent;
  void *Token{nullptr};
};

NB_DEFINE_CLASS_ID(TCustomFarPlugin);
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
  static bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TCustomFarPlugin); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TCustomFarPlugin) || TObject::is(Kind); }
public:
  TCustomFarPlugin() = delete;
  explicit TCustomFarPlugin(TObjectClassId Kind, HINSTANCE HInst) noexcept;
  virtual ~TCustomFarPlugin() noexcept;
  virtual void Initialize();
  virtual void Finalize();

  virtual int32_t GetMinFarVersion() const;
  virtual void SetStartupInfo(const struct PluginStartupInfo *Info);
  virtual const struct PluginStartupInfo *GetPluginStartupInfo() const { return &FStartupInfo; }
  virtual void ExitFAR();
  virtual void GetPluginInfo(struct PluginInfo *Info);
  virtual int32_t Configure(int32_t Item);
  virtual void *OpenPlugin(int OpenFrom, int32_t Item);
  virtual void ClosePlugin(void *Plugin);
  virtual void GetOpenPluginInfo(HANDLE Plugin, struct OpenPluginInfo *Info);
  virtual int32_t GetFindData(HANDLE Plugin,
    struct PluginPanelItem **PanelItem, int *ItemsNumber, int OpMode);
  virtual void FreeFindData(HANDLE Plugin, struct PluginPanelItem *PanelItem,
    int32_t ItemsNumber);
  virtual int32_t ProcessHostFile(HANDLE Plugin,
    struct PluginPanelItem *PanelItem, int32_t ItemsNumber, int OpMode);
  virtual int32_t ProcessKey(HANDLE Plugin, int Key, DWORD ControlState);
  virtual int32_t ProcessEvent(HANDLE Plugin, int Event, void *Param);
  virtual int32_t SetDirectory(HANDLE Plugin, const wchar_t *Dir, int OpMode);
  virtual int32_t MakeDirectory(HANDLE Plugin, const wchar_t **Name, int OpMode);
  virtual int32_t DeleteFiles(HANDLE Plugin, struct PluginPanelItem *PanelItem,
    int32_t ItemsNumber, int OpMode);
  virtual int32_t GetFiles(HANDLE Plugin, struct PluginPanelItem *PanelItem,
    int32_t ItemsNumber, int Move, const wchar_t **DestPath, int OpMode);
  virtual int32_t PutFiles(HANDLE Plugin, struct PluginPanelItem *PanelItem,
    int32_t ItemsNumber, int Move, const wchar_t *srcPath, int OpMode);
  virtual int32_t ProcessEditorEvent(int Event, void *Param);
  virtual int32_t ProcessEditorInput(const INPUT_RECORD *Rec);

  virtual void HandleException(Exception *E, int OpMode = 0);
  virtual UnicodeString GetMsg(int32_t MsgId) const;
  virtual UnicodeString GetModuleName() const;

  static wchar_t *DuplicateStr(const UnicodeString Str, bool AllowEmpty = false);
  uint32_t Message(DWORD Flags, const UnicodeString Title,
    const UnicodeString Message, TStrings *Buttons = nullptr,
    TFarMessageParams *Params = nullptr);
  int32_t MaxMessageLines() const;
  int32_t MaxMenuItemLength() const;
  int32_t Menu(DWORD Flags, const UnicodeString Title,
    UnicodeString Bottom, TStrings *Items, const int *BreakKeys,
    int &BreakCode);
  int32_t Menu(DWORD Flags, const UnicodeString Title,
    UnicodeString Bottom, TStrings *Items);
  int32_t Menu(DWORD Flags, const UnicodeString Title,
    UnicodeString Bottom, const FarMenuItem *Items, int32_t Count,
    const int *BreakKeys, int &BreakCode);
  bool InputBox(const UnicodeString Title, const UnicodeString Prompt,
    UnicodeString &Text, DWORD Flags, const UnicodeString HistoryName = UnicodeString(),
    int32_t MaxLen = 255, TFarInputBoxValidateEvent OnValidate = nullptr);
  void SaveScreen(HANDLE &Screen);
  void RestoreScreen(HANDLE &Screen);
  bool CheckForEsc() const;
  bool Viewer(UnicodeString AFileName, UnicodeString Title, DWORD Flags);
  bool Editor(UnicodeString AFileName, UnicodeString Title, DWORD Flags);

  int32_t FarControl(uint32_t Command, int32_t Param1, int32_t Param2, HANDLE Plugin = INVALID_HANDLE_VALUE);
  int32_t FarAdvControl(uint32_t Command, void *Param = nullptr) const;
  int32_t FarEditorControl(uint32_t Command, void *Param);
  int32_t GetFarSystemSettings() const;
  void Text(int X, int Y, int Color, const UnicodeString Str);
  void FlushText();
  void FarWriteConsole(const UnicodeString Str);
  void FarCopyToClipboard(const UnicodeString Str);
  void FarCopyToClipboard(const TStrings *Strings);
  int32_t GetFarVersion() const;
  UnicodeString FormatFarVersion(int32_t Version) const;
  UnicodeString GetTemporaryDir() const;
  int32_t InputRecordToKey(const INPUT_RECORD *Rec) const;
  TFarEditorInfo *EditorInfo();

  void ShowConsoleTitle(const UnicodeString Title);
  void ClearConsoleTitle();
  void UpdateConsoleTitle(const UnicodeString Title);
  void UpdateConsoleTitleProgress(short Progress);
  void ShowTerminalScreen();
  void SaveTerminalScreen();
  void ScrollTerminalScreen(int Rows);
  TPoint TerminalInfo(TPoint *Size = nullptr, TPoint *Cursor = nullptr) const;
  uint32_t ConsoleWindowState() const;
  void ToggleVideoMode();

  TCustomFarFileSystem *GetPanelFileSystem(bool Another = false,
    HANDLE Plugin = INVALID_HANDLE_VALUE);

  TFarDialog *GetTopDialog() const { return FTopDialog; }
  HINSTANCE GetHandle() const { return FHandle; }
  uint32_t GetFarThreadId() const { return FFarThreadId; }
  const FarStandardFunctions &GetFarStandardFunctions() const { return FFarStandardFunctions; }

protected:
  PluginStartupInfo FStartupInfo{};
  FarStandardFunctions FFarStandardFunctions{};
  HINSTANCE FHandle{};
  std::unique_ptr<TList> FOpenedPlugins;
  TFarDialog *FTopDialog{nullptr};
  HANDLE FConsoleInput{};
  HANDLE FConsoleOutput{};
  mutable int32_t FFarVersion{0};
  bool FTerminalScreenShowing{false};
  TCriticalSection FCriticalSection;
  uint32_t FFarThreadId{0};
  mutable bool FValidFarSystemSettings{false};
  mutable int32_t FFarSystemSettings{0};
  TPoint FNormalConsoleSize;

  virtual bool HandlesFunction(THandlesFunction Function) const;
  virtual void GetPluginInfoEx(DWORD &Flags,
    TStrings *DiskMenuStrings, TStrings *PluginMenuStrings,
    TStrings *PluginConfigStrings, TStrings *CommandPrefixes) = 0;
  virtual TCustomFarFileSystem *OpenPluginEx(int32_t OpenFrom, int32_t Item) = 0;
  virtual bool ConfigureEx(int32_t Item) = 0;
  virtual int32_t ProcessEditorEventEx(int32_t Event, void *Param) = 0;
  virtual int32_t ProcessEditorInputEx(const INPUT_RECORD *Rec) = 0;
  virtual void HandleFileSystemException(TCustomFarFileSystem *FarFileSystem,
    Exception *E, int OpMode = 0);
  void ResetCachedInfo();
  int32_t MaxLength(TStrings *Strings) const;
  uint32_t FarMessage(DWORD Flags,
    const UnicodeString Title, const UnicodeString Message, TStrings *Buttons,
    TFarMessageParams *Params);
  uint32_t DialogMessage(DWORD Flags,
    const UnicodeString Title, const UnicodeString Message, TStrings *Buttons,
    TFarMessageParams *Params);
  void InvalidateOpenPluginInfo();

  const TCriticalSection &GetCriticalSection() const { return FCriticalSection; }

#ifdef NETBOX_DEBUG
public:
  void RunTests();
#endif
private:
  void UpdateProgress(int32_t State, int32_t Progress) const;

private:
  PluginInfo FPluginInfo{};
  std::unique_ptr<TStringList> FSavedTitles;
  UnicodeString FCurrentTitle;
  short FCurrentProgress{0};

  void ClearPluginInfo(PluginInfo &Info) const;
  void UpdateCurrentConsoleTitle();
  UnicodeString FormatConsoleTitle() const;
  HWND GetConsoleWindow() const;
  RECT GetPanelBounds(HANDLE PanelHandle);
  bool CompareRects(const RECT &lhs, const RECT &rhs) const
  {
    return
      lhs.left == rhs.left &&
      lhs.top == rhs.top &&
      lhs.right == rhs.right &&
      lhs.bottom == rhs.bottom;
  }
  void CloseFileSystem(TCustomFarFileSystem *FileSystem);
};

NB_DEFINE_CLASS_ID(TCustomFarFileSystem);
class TCustomFarFileSystem : public TObject
{
  friend class TFarPanelInfo;
  friend class TCustomFarPlugin;
  NB_DISABLE_COPY(TCustomFarFileSystem)
public:
  static bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TCustomFarFileSystem); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TCustomFarFileSystem) || TObject::is(Kind); }
public:
  explicit TCustomFarFileSystem(TObjectClassId Kind, TCustomFarPlugin *APlugin) noexcept;
  virtual ~TCustomFarFileSystem() noexcept;
  void Init();

  void GetOpenPluginInfo(struct OpenPluginInfo *Info);
  int32_t GetFindData(struct PluginPanelItem **PanelItem,
    int *ItemsNumber, int OpMode);
  void FreeFindData(struct PluginPanelItem *PanelItem, int32_t ItemsNumber);
  int32_t ProcessHostFile(struct PluginPanelItem *PanelItem,
    int32_t ItemsNumber, int OpMode);
  int32_t ProcessKey(int32_t Key, uint32_t ControlState);
  int32_t ProcessEvent(int32_t Event, void *Param);
  int32_t SetDirectory(const wchar_t *Dir, int OpMode);
  int32_t MakeDirectory(const wchar_t **Name, int OpMode);
  int32_t DeleteFiles(struct PluginPanelItem *PanelItem,
    int32_t ItemsNumber, int OpMode);
  int32_t GetFiles(struct PluginPanelItem *PanelItem,
    int32_t ItemsNumber, int Move, const wchar_t **DestPath, int OpMode);
  int32_t PutFiles(struct PluginPanelItem *PanelItem,
    int32_t ItemsNumber, int Move, const wchar_t *srcPath, int OpMode);
  virtual void Close();

protected:
  virtual UnicodeString GetCurrDirectory() const = 0;

protected:
  TCustomFarPlugin *FPlugin{nullptr};
  bool FClosed{false};

  virtual void GetOpenPluginInfoEx(DWORD &Flags,
    UnicodeString &HostFile, UnicodeString &CurDir, UnicodeString &Format,
    UnicodeString &PanelTitle, TFarPanelModes *PanelModes, int &StartPanelMode,
    int &StartSortMode, bool &StartSortOrder, TFarKeyBarTitles *KeyBarTitles,
    UnicodeString &ShortcutData) = 0;
  virtual bool GetFindDataEx(TObjectList *PanelItems, int OpMode) = 0;
  virtual bool ProcessHostFileEx(TObjectList *PanelItems, int OpMode);
  virtual bool ProcessKeyEx(int32_t Key, uint32_t ControlState);
  virtual bool ProcessEventEx(int32_t Event, void *Param);
  virtual bool SetDirectoryEx(UnicodeString Dir, int OpMode);
  virtual int32_t MakeDirectoryEx(UnicodeString &Name, int OpMode);
  virtual bool DeleteFilesEx(TObjectList *PanelItems, int OpMode);
  virtual int32_t GetFilesEx(TObjectList *PanelItems, bool Move,
    UnicodeString &DestPath, int OpMode);
  virtual int32_t PutFilesEx(TObjectList *PanelItems, bool Move, int OpMode);

  void ResetCachedInfo();
  int32_t FarControl(uint32_t Command, int32_t Param1, int32_t Param2);
  int32_t FarControl(uint32_t Command, int32_t Param1, int32_t Param2, HANDLE Plugin);
  bool UpdatePanel(bool ClearSelection = false, bool Another = false);
  void RedrawPanel(bool Another = false);
  void ClosePlugin();
  UnicodeString GetMsg(int32_t MsgId) const;
  TCustomFarFileSystem *GetOppositeFileSystem();
  bool IsActiveFileSystem() const;
  bool IsLeft() const;
  bool IsRight() const;

  virtual void HandleException(Exception *E, int OpMode = 0);

  TFarPanelInfo *const *GetPanelInfo() const { return GetPanelInfo(0); }
  TFarPanelInfo **GetPanelInfo() { return GetPanelInfo(0); }
  TFarPanelInfo *const *GetAnotherPanelInfo() const { return GetPanelInfo(1); }
  TFarPanelInfo **GetAnotherPanelInfo() { return GetPanelInfo(1); }
  const TCriticalSection &GetCriticalSection() const { return FCriticalSection; }
  TCriticalSection &GetCriticalSection() { return FCriticalSection; }
  bool GetOpenPluginInfoValid() const { return FOpenPluginInfoValid; }

protected:
  TCriticalSection FCriticalSection;
  void InvalidateOpenPluginInfo();
  TCustomFarFileSystem *GetOwnerFileSystem() { return FOwnerFileSystem; }
  void SetOwnerFileSystem(TCustomFarFileSystem *Value) { FOwnerFileSystem = Value; }

private:
  UnicodeString FNameStr;
  UnicodeString FDestPathStr;
  OpenPluginInfo FOpenPluginInfo{};
  TCustomFarFileSystem *FOwnerFileSystem{nullptr};
  bool FOpenPluginInfoValid{false};
  TFarPanelInfo *FPanelInfo[2]{};
  static uint32_t FInstances;

  void ClearOpenPluginInfo(OpenPluginInfo &Info);
  TObjectList *CreatePanelItemList(struct PluginPanelItem *PanelItem, int32_t ItemsNumber);
  TFarPanelInfo *const *GetPanelInfo(int Another) const;
  TFarPanelInfo **GetPanelInfo(int Another);
};

constexpr int32_t PANEL_MODES_COUNT = 10;
class TFarPanelModes final : public TObject
{
  friend class TCustomFarFileSystem;
public:
  TFarPanelModes() noexcept;
  virtual ~TFarPanelModes() noexcept;

  void SetPanelMode(size_t Mode, const UnicodeString ColumnTypes = UnicodeString(),
    const UnicodeString ColumnWidths = UnicodeString(), TStrings *ColumnTitles = nullptr,
    bool FullScreen = false, bool DetailedStatus = true, bool AlignExtensions = true,
    bool CaseConversion = true, const UnicodeString StatusColumnTypes = UnicodeString(),
    const UnicodeString StatusColumnWidths = UnicodeString());

private:
  PanelMode FPanelModes[PANEL_MODES_COUNT]{};
  bool FReferenced{false};

  void FillOpenPluginInfo(struct OpenPluginInfo *Info);
  static void ClearPanelMode(PanelMode &Mode);
  static int32_t CommaCount(const UnicodeString ColumnTypes);
};

class TFarKeyBarTitles final : public TObject
{
  friend class TCustomFarFileSystem;
public:
  TFarKeyBarTitles() noexcept;
  virtual ~TFarKeyBarTitles() noexcept;

  void ClearFileKeyBarTitles();
  void ClearKeyBarTitle(TFarShiftStatus ShiftStatus,
    int32_t FunctionKeyStart, int32_t FunctionKeyEnd = 0);
  void SetKeyBarTitle(TFarShiftStatus ShiftStatus, int32_t FunctionKey,
    UnicodeString Title);

private:
  KeyBarTitles FKeyBarTitles{};
  bool FReferenced{false};

  void FillOpenPluginInfo(struct OpenPluginInfo *Info);
  static void ClearKeyBarTitles(KeyBarTitles &Titles);
};

NB_DEFINE_CLASS_ID(TCustomFarPanelItem);
class TCustomFarPanelItem : public TObject
{
  friend class TCustomFarFileSystem;
public:
  static bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TCustomFarPanelItem); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TCustomFarPanelItem) || TObject::is(Kind); }
protected:
  TCustomFarPanelItem() = delete;
  explicit TCustomFarPanelItem(TObjectClassId Kind) noexcept : TObject(Kind) {}
  virtual ~TCustomFarPanelItem() = default;
  virtual void GetData(
    DWORD &Flags, UnicodeString &AFileName, int64_t &Size,
    DWORD &FileAttributes,
    TDateTime &LastWriteTime, TDateTime &LastAccess,
    DWORD &NumberOfLinks, UnicodeString &Description,
    UnicodeString &Owner, void *&UserData, int &CustomColumnNumber) = 0;
  virtual UnicodeString GetCustomColumnData(size_t Column);

  void FillPanelItem(struct PluginPanelItem *PanelItem);
};

NB_DEFINE_CLASS_ID(TFarPanelItem);
class TFarPanelItem final : public TCustomFarPanelItem
{
  NB_DISABLE_COPY(TFarPanelItem)
public:
  static bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TFarPanelItem); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TFarPanelItem) || TCustomFarPanelItem::is(Kind); }
public:
  TFarPanelItem() = delete;
  explicit TFarPanelItem(PluginPanelItem *APanelItem, bool OwnsItem) noexcept;
  virtual ~TFarPanelItem() noexcept;

  uint32_t GetFlags() const;
  uint32_t GetFileAttrs() const;
  UnicodeString GetFileName() const;
  void *GetUserData() const;
  bool GetSelected() const;
  void SetSelected(bool Value);
  bool GetIsParentDirectory() const;
  bool GetIsFile() const;

protected:
  PluginPanelItem *FPanelItem{nullptr};
  bool FOwnsItem{false};

  void GetData(
    DWORD &Flags, UnicodeString &AFileName, int64_t &Size,
    DWORD &FileAttributes,
    TDateTime &LastWriteTime, TDateTime &LastAccess,
    DWORD &NumberOfLinks, UnicodeString &Description,
    UnicodeString &Owner, void *&UserData, int &CustomColumnNumber) override;
  UnicodeString GetCustomColumnData(size_t Column) override;
};

NB_DEFINE_CLASS_ID(THintPanelItem);
class THintPanelItem final : public TCustomFarPanelItem
{
public:
  explicit THintPanelItem(const UnicodeString AHint) noexcept;
  virtual ~THintPanelItem() = default;

protected:
  void GetData(
    DWORD &Flags, UnicodeString &AFileName, int64_t &Size,
    DWORD &FileAttributes,
    TDateTime &LastWriteTime, TDateTime &LastAccess,
    DWORD &NumberOfLinks, UnicodeString &Description,
    UnicodeString &Owner, void *&UserData, int &CustomColumnNumber) override;

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
  TFarPanelInfo() = delete;
  explicit TFarPanelInfo(PanelInfo *APanelInfo, TCustomFarFileSystem *AOwner) noexcept;
  virtual ~TFarPanelInfo() noexcept;

  const TObjectList *GetItems() const { return const_cast<TFarPanelInfo *>(this)->GetItems(); }
  TObjectList *GetItems();
  int32_t GetItemCount() const;
  TFarPanelItem *GetFocusedItem() const;
  void SetFocusedItem(const TFarPanelItem *Value);
  int32_t GetFocusedIndex() const;
  void SetFocusedIndex(int32_t Value);
  int32_t GetSelectedCount(bool CountCurrentItem = false) const;
  TRect GetBounds() const;
  TFarPanelType GetType() const;
  bool GetIsPlugin() const;
  UnicodeString GetCurrDirectory() const;

  void ApplySelection();
  TFarPanelItem *FindFileName(const UnicodeString AFileName) const;
  const TFarPanelItem *FindUserData(const void *UserData) const;
  TFarPanelItem *FindUserData(const void *UserData);

private:
  gsl::owner<PanelInfo *> FPanelInfo{nullptr};
  TObjectList *FItems{nullptr};
  TCustomFarFileSystem *FOwner{nullptr};
};

NB_DEFINE_CLASS_ID(TFarMenuItems);
class TFarMenuItems : public TStringList
{
public:
  static bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TFarMenuItems); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TFarMenuItems) || TStringList::is(Kind); }
public:
  explicit TFarMenuItems() noexcept;
  virtual ~TFarMenuItems() = default;
  void AddSeparator(bool Visible = true);
  virtual int32_t Add(UnicodeString Text, bool Visible = true);

  void Clear() override;
  void Delete(int32_t Index) override;

  int32_t GetItemFocused() const { return FItemFocused; }
  void SetItemFocused(int32_t Value);
  bool GetDisabled(int32_t Index) const { return GetFlag(Index, MIF_DISABLE); }
  void SetDisabled(int32_t Index, bool Value) { SetFlag(Index, MIF_DISABLE, Value); }
  bool GetChecked(int32_t Index) const { return GetFlag(Index, MIF_CHECKED); }
  void SetChecked(int32_t Index, bool Value) { SetFlag(Index, MIF_CHECKED, Value); }

  void SetFlag(int32_t Index, uint32_t Flag, bool Value);
  bool GetFlag(int32_t Index, uint32_t Flag) const;

protected:
  void SetObj(int32_t Index, TObject *AObject) override;

private:
  int32_t FItemFocused{nb::NPOS};
};

class TFarEditorInfo : public TObject
{
  NB_DISABLE_COPY(TFarEditorInfo)
public:
  TFarEditorInfo() = delete;
  explicit TFarEditorInfo(EditorInfo *Info) noexcept;
  ~TFarEditorInfo() noexcept;

  int32_t GetEditorID() const;
  static UnicodeString GetFileName();

private:
  gsl::owner<EditorInfo*> FEditorInfo{nullptr};
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

extern TCustomFarPlugin *FarPlugin;

class TGlobalFunctions : public TGlobals
{
public:
  HINSTANCE GetInstanceHandle() const override;
  UnicodeString GetMsg(int32_t Id) const override;
  UnicodeString GetCurrDirectory() const override;
  UnicodeString GetStrVersionNumber() const override;
  bool InputDialog(const UnicodeString ACaption,
    const UnicodeString APrompt, UnicodeString &Value, const UnicodeString HelpKeyword,
    TStrings *History, bool PathInput,
    TInputDialogInitializeEvent OnInitialize, bool Echo) override;
  uint32_t MoreMessageDialog(const UnicodeString AMessage,
    TStrings *MoreMessages, TQueryType Type, uint32_t Answers,
    const TMessageParams *Params) override;
};

