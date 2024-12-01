#pragma once

#include <memory>
#pragma warning(push, 1)
#include <farcolor.hpp>
#pragma warning(pop)
#include "FarPlugin.h"

class TFarDialogContainer;
class TFarDialogItem;
class TFarButton;
class TFarSeparator;
class TFarBox;
class TFarList;
struct FarDialogItem;

enum TItemPosition
{
  ipSame,
  ipNewLine,
  ipBelow,
  ipRight
};

using TFarKeyEvent = nb::FastDelegate4<void,
  TFarDialog * /*Sender*/, TFarDialogItem * /*Item*/, int32_t /*KeyCode*/, bool & /*Handled*/>;
using TFarMouseClickEvent = nb::FastDelegate2<void,
  TFarDialogItem * /*Item*/, const MOUSE_EVENT_RECORD * /*Event*/>;
using TFarProcessGroupEvent = nb::FastDelegate2<void,
  TFarDialogItem * /*Item*/, void * /*Arg*/>;

class TFarDialogIdleThread;

class TFarDialog : public TObject
{
  friend class TFarDialogItem;
  friend class TFarDialogContainer;
  friend class TFarButton;
  friend class TFarList;
  friend class TFarListBox;
  friend class TFarDialogIdleThread;
  NB_DISABLE_COPY(TFarDialog)
public:
  static bool classof(const TObject * Obj) { return Obj->is(OBJECT_CLASS_TFarDialog); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TFarDialog) || TObject::is(Kind); }
public:
  TFarDialog() = delete;
  explicit TFarDialog(gsl::not_null<TCustomFarPlugin *> AFarPlugin) noexcept;
  virtual ~TFarDialog() noexcept override;
  void InitDialog();

  int32_t ShowModal();
  void ShowGroup(int32_t Group, bool Show);
  void EnableGroup(int32_t Group, bool Enable);

  TRect GetBounds() const { return FBounds; }
  TRect GetClientRect() const;
  UnicodeString GetHelpTopic() const { return FHelpTopic; }
  void SetHelpTopic(const UnicodeString & Value);
  FARDIALOGITEMFLAGS GetFlags() const { return FFlags; }
  void SetFlags(FARDIALOGITEMFLAGS Value);
  bool GetCentered() const;
  void SetCentered(bool Value);
  TPoint GetSize() const;
  void SetSize(TPoint Value);
  TPoint GetClientSize() const;
  int32_t GetWidth() const;
  void SetWidth(int32_t Value);
  int32_t GetHeight() const;
  void SetHeight(int32_t Value);
  UnicodeString GetCaption() const;
  void SetCaption(const UnicodeString & Value);
  HANDLE GetHandle() const { return FHandle; }
  TFarButton * GetDefaultButton() const { return FDefaultButton; }
  TFarBox * GetBorderBox() const { return FBorderBox; }
  // int32_t GetType(TFarDialogItem * Item) const;
  int32_t GetItemIdx(const TFarDialogItem * Item) const;
  TFarDialogItem * GetItem(int32_t Index) const;
  TFarDialogItem * GetControl(int32_t Index) const { return GetItem(Index); }
  int32_t GetItemCount() const;
  int32_t GetControlCount() const { return GetItemCount(); }
  TItemPosition GetNextItemPosition() const { return FNextItemPosition; }
  void SetNextItemPosition(const TItemPosition Value) { FNextItemPosition = Value; }
  int32_t GetDefaultGroup() const { return FDefaultGroup; }
  void SetDefaultGroup(int32_t Value) { FDefaultGroup = Value; }
  int32_t GetTag() const { return FTag; }
  void SetTag(int32_t Value) { FTag = Value; }
  TFarDialogItem * GetItemFocused() const { return FItemFocused; }
  void SetItemFocused(TFarDialogItem * Value);
  int32_t GetResult() const { return FResult; }
  TPoint GetMaxSize() const;

  TFarKeyEvent & GetOnKey() { return FOnKey; }
  void SetOnKey(TFarKeyEvent && Value) { FOnKey = std::move(Value); }

  void Redraw();
  void LockChanges();
  void UnlockChanges();
  FarColor GetSystemColor(PaletteColors colorId);
  bool HotKey(uint32_t Key, uint32_t ControlState) const;
  void SetDialogGuid(const UUID * Guid) { FGuid = Guid; };

protected:
  TCustomFarPlugin * GetFarPlugin() const { return FFarPlugin; }
  TCustomFarPlugin * GetFarPlugin() { return FFarPlugin; }
  TObjectList * GetItems() const { return FItems.get(); }
  TObjectList * GetItems() { return FItems.get(); }
  void Add(TFarDialogItem * DialogItem);
  void Add(TFarDialogContainer * Container);
  intptr_t SendDlgMessage(intptr_t Msg, intptr_t Param1, void * Param2);
  virtual const UUID * GetDialogGuid() const { return FGuid; }
  virtual intptr_t DialogProc(intptr_t Msg, intptr_t Param1, void * Param2);
  virtual intptr_t FailDialogProc(intptr_t Msg, intptr_t Param1, void * Param2);
  intptr_t DefaultDialogProc(intptr_t Msg, intptr_t Param1, void * Param2);
  virtual bool MouseEvent(MOUSE_EVENT_RECORD * Event);
  virtual bool Key(TFarDialogItem * Item, intptr_t KeyCode);
  virtual void Change();
  virtual void Init();
  virtual bool CloseQuery();
  UnicodeString GetMsg(intptr_t MsgId) const;
  void GetNextItemPosition(int32_t & Left, int32_t & Top);
  void RefreshBounds();
  virtual void Idle();
  void BreakSynchronize();
  void Synchronize(TThreadMethod Method);
  void Close(const TFarButton * Button);
  void ProcessGroup(int32_t Group, TFarProcessGroupEvent && Callback, void * Arg);
  void ShowItem(TFarDialogItem * Item, void * Arg);
  void EnableItem(TFarDialogItem * Item, void * Arg);
  bool ChangesLocked() const;
  TFarDialogItem * ItemAt(int32_t X, int32_t Y) const;

  static intptr_t WINAPI DialogProcGeneral(HANDLE Handle, intptr_t Msg, intptr_t Param1, void * Param2);

  void SetBounds(const TRect & Value);

private:
  mutable gsl::not_null<TCustomFarPlugin *> FFarPlugin;
  TRect FBounds{-1, -1, 40, 10};
  FARDIALOGITEMFLAGS FFlags{0};
  UnicodeString FHelpTopic;
  bool FVisible{false};
  std::unique_ptr<TObjectList> FItems{std::make_unique<TObjectList>()};
  std::unique_ptr<TObjectList> FContainers{std::make_unique<TObjectList>()};
  HANDLE FHandle{nullptr};
  TFarButton * FDefaultButton{nullptr};
  TFarBox * FBorderBox{nullptr};
  TItemPosition FNextItemPosition{ipNewLine};
  int32_t FDefaultGroup{0};
  int32_t FTag{0};
  TFarDialogItem * FItemFocused{nullptr};
  TFarKeyEvent FOnKey;
  gsl::owner<FarDialogItem *> FDialogItems{nullptr};
  std::unique_ptr<TFarDialogIdleThread> FTIdleThread;
  int32_t FDialogItemsCapacity{0};
  int32_t FChangesLocked{0};
  bool FChangesPending{false};
  int32_t FResult{0};
  bool FNeedsSynchronize{false};
  HANDLE FSynchronizeObjects[2]{};
  TThreadMethod FSynchronizeMethod;
  const UUID * FGuid{&DialogGuid};
};

class TFarDialogContainer : public TObject
{
  friend class TFarDialog;
  friend class TFarDialogItem;
  NB_DISABLE_COPY(TFarDialogContainer)
public:
  static bool classof(const TObject * Obj) { return Obj->is(OBJECT_CLASS_TFarDialogContainer); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TFarDialogContainer) || TObject::is(Kind); }
public:
  int32_t GetLeft() const { return FLeft; }
  void SetLeft(int32_t Value) { SetPosition(0, Value); }
  int32_t GetTop() const { return FTop; }
  void SetTop(int32_t Value) { SetPosition(1, Value); }
  bool GetEnabled() const { return FEnabled; }
  void SetEnabled(bool Value);
  void SetPosition(int32_t AIndex, int32_t Value);
  int32_t GetItemCount() const;

protected:
  TFarDialogContainer() = delete;
  explicit TFarDialogContainer(TObjectClassId Kind, TFarDialog * ADialog) noexcept;
  virtual ~TFarDialogContainer() noexcept override;

  TFarDialog * GetDialog() const { return FDialog; }
  TFarDialog * GetDialog() { return FDialog; }

  void Add(TFarDialogItem * Item);
  void Remove(TFarDialogItem * Item);
  virtual void Changed() override;
  UnicodeString GetMsg(intptr_t MsgId) const;

private:
  int32_t FLeft{0};
  int32_t FTop{0};
  std::unique_ptr<TObjectList> FItems{std::make_unique<TObjectList>()};
  TFarDialog * FDialog{nullptr};
  bool FEnabled{true};
};

constexpr const int32_t DIF_INVERSE = 0x00000001UL;

class TFarDialogItem : public TObject
{
  friend class TFarDialog;
  friend class TFarMessageDialog;
  friend class TFarDialogContainer;
  friend class TFarList;
  NB_DISABLE_COPY(TFarDialogItem)
public:
  static bool classof(const TObject * Obj) { return Obj->is(OBJECT_CLASS_TFarDialogItem); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TFarDialogItem) || TObject::is(Kind); }
public:
  TRect GetBounds() const { return FBounds; }
  TRect GetActualBounds() const;
  int32_t GetLeft() const { return GetCoordinate(0); }
  void SetLeft(int32_t Value) { SetCoordinate(0, Value); }
  int32_t GetTop() const { return GetCoordinate(1); }
  void SetTop(int32_t Value) { SetCoordinate(1, Value); }
  int32_t GetRight() const { return GetCoordinate(2); }
  void SetRight(int32_t Value) { SetCoordinate(2, Value); }
  int32_t GetBottom() const { return GetCoordinate(3); }
  void SetBottom(int32_t Value) { SetCoordinate(3, Value); }
  int32_t GetWidth() const;
  void SetWidth(int32_t Value);
  int32_t GetHeight() const;
  void SetHeight(int32_t Value);
  bool GetEnabled() const { return FEnabled; }
  void SetEnabled(bool Value);
  bool GetIsEnabled() const { return FIsEnabled; }
  TFarDialogItem * GetEnabledFollow() const { return FEnabledFollow; }
  void SetEnabledFollow(TFarDialogItem * Value);
  TFarDialogItem * GetEnabledDependency() const { return FEnabledDependency; }
  void SetEnabledDependency(TFarDialogItem * Value);
  TFarDialogItem * GetEnabledDependencyNegative() const { return FEnabledDependencyNegative; }
  void SetEnabledDependencyNegative(TFarDialogItem * Value);
  virtual bool GetIsEmpty() const;
  int32_t GetGroup() const { return FGroup; }
  void SetGroup(int32_t Value) { FGroup = Value; }
  bool GetVisible() const { return GetFlag(DIF_HIDDEN | DIF_INVERSE); }
  void SetVisible(bool Value) { SetFlag(DIF_HIDDEN | DIF_INVERSE, Value); }
  bool GetTabStop() const { return GetFlag(DIF_NOFOCUS | DIF_INVERSE); }
  void SetTabStop(bool Value) { SetFlag(DIF_NOFOCUS | DIF_INVERSE, Value); }
  int32_t GetTag() const { return FTag; }
  void SetTag(int32_t Value) { FTag = Value; }
  TFarDialog * GetDialog() const { return FDialog; }
  TFarDialog * GetDialog() { return FDialog; }
  const TFarDialog * GetOwner() const { return FDialog; }
  TFarDialog * GetOwner() { return FDialog; }

  TNotifyEvent & GetOnExit() { return FOnExit; }
  void SetOnExit(TNotifyEvent && Value) { FOnExit = std::move(Value); }
  TFarMouseClickEvent & GetOnMouseClick() { return FOnMouseClick; }
  void SetOnMouseClick(TFarMouseClickEvent && Value) { FOnMouseClick = std::move(Value); }
  bool GetFocused() const;
  void SetFocused(bool Value);

  void Move(int32_t DeltaX, int32_t DeltaY);
  void MoveAt(int32_t X, int32_t Y);
  virtual bool CanFocus() const;
  bool Focused() const;
  void SetFocus();
  void SetItemIdx(int32_t Value) { FItemIdx = Value; }

public:
  virtual void SetDataInternal(const UnicodeString & Value);
  void UpdateData(const UnicodeString & Value);
  void UpdateSelected(intptr_t Value);

  bool GetFlag(FARDIALOGITEMFLAGS Index) const;
  void SetFlag(FARDIALOGITEMFLAGS Index, bool Value);

  virtual void DoFocus();
  virtual void DoExit();

  int8_t GetColor(int32_t Index) const;
  void SetColor(int32_t Index, int8_t Value);

protected:
  FARDIALOGITEMTYPES FDefaultType;
  int32_t FGroup{0};
  int32_t FTag{0};
  TNotifyEvent FOnExit;
  TFarMouseClickEvent FOnMouseClick;

  explicit TFarDialogItem(TObjectClassId Kind, TFarDialog * ADialog, FARDIALOGITEMTYPES AType);
  virtual ~TFarDialogItem() noexcept override;

  const FarDialogItem * GetDialogItem() const;
  FarDialogItem * GetDialogItem();
  virtual bool GetCenterGroup() const { return GetFlag(DIF_CENTERGROUP); }
  virtual void SetCenterGroup(bool Value) { SetFlag(DIF_CENTERGROUP, Value); }
  virtual UnicodeString GetData() const;
  virtual UnicodeString GetData();
  virtual void SetData(const UnicodeString & Value);
  FARDIALOGITEMTYPES GetType() const;
  void SetType(FARDIALOGITEMTYPES Value);
  int32_t GetItemIdx() const { return FItemIdx; }
  virtual intptr_t GetSelected() const;
  virtual void SetSelected(int32_t Value);
  TFarDialogContainer * GetContainer() const { return FContainer; }
  void SetContainer(TFarDialogContainer * Value);
  virtual bool GetChecked() const;
  virtual void SetChecked(bool Value);
  void SetBounds(const TRect & Value);
  FARDIALOGITEMFLAGS GetFlags() const;
  void SetFlags(FARDIALOGITEMFLAGS Value);
  void UpdateFlags(FARDIALOGITEMFLAGS Value);
  int32_t GetCoordinate(int32_t Index) const;
  void SetCoordinate(int32_t Index, int32_t Value);
  // TFarDialogItem * GetPrevItem() const;
  void UpdateFocused(bool Value);
  void UpdateEnabled();

  virtual void Detach();
  void DialogResized();
  intptr_t SendDialogMessage(intptr_t Msg, void * Param);
  intptr_t SendDialogMessage(intptr_t Msg, intptr_t Param1, void * Param2);
  virtual intptr_t ItemProc(intptr_t Msg, void * Param);
  intptr_t DefaultItemProc(intptr_t Msg, void * Param);
  intptr_t DefaultDialogProc(intptr_t Msg, intptr_t Param1, void * Param2);
  virtual intptr_t FailItemProc(intptr_t Msg, void * Param);
  virtual void Change();
  void DialogChange();
  bool GetAlterType(FARDIALOGITEMTYPES Index) const;
  void SetAlterType(FARDIALOGITEMTYPES Index, bool Value);
  virtual void UpdateBounds();
  virtual void ResetBounds();
  virtual void Init();
  virtual bool CloseQuery();
  virtual bool MouseMove(int32_t X, int32_t Y, MOUSE_EVENT_RECORD * Event);
  virtual bool MouseClick(MOUSE_EVENT_RECORD * Event);
  TPoint MouseClientPosition(const MOUSE_EVENT_RECORD * Event);
  void Text(int32_t X, int32_t Y, const FarColor & Color, const UnicodeString & Str);
  void Redraw();
  virtual bool HotKey(char HotKey);

private:
  const struct PluginStartupInfo * GetPluginStartupInfo() const;

private:
  TFarDialog * FDialog{nullptr};
  TRect FBounds{};
  TFarDialogItem * FEnabledFollow{nullptr};
  TFarDialogItem * FEnabledDependency{nullptr};
  TFarDialogItem * FEnabledDependencyNegative{nullptr};
  TFarDialogContainer * FContainer{nullptr};
  int32_t FItemIdx{0};
  uint32_t FColors{0};
  uint32_t FColorMask{0};
  bool FEnabled{false};
  bool FIsEnabled{false};
};

class TFarBox : public TFarDialogItem
{
public:
  explicit TFarBox(TFarDialog * ADialog) noexcept;

  virtual UnicodeString GetCaption() const { return GetData(); }
  virtual void SetCaption(const UnicodeString & Value) { SetData(Value); }
  virtual bool GetDouble() const { return GetAlterType(DI_DOUBLEBOX); }
  virtual void SetDouble(bool Value) { SetAlterType(DI_DOUBLEBOX, Value); }
};

using TFarButtonClickEvent = nb::FastDelegate2<void,
  TFarButton * /*Sender*/, bool & /*Close*/>;

enum TFarButtonBrackets
{
  brNone,
  brTight,
  brSpace,
  brNormal
};

class TFarButton : public TFarDialogItem
{
public:
  static bool classof(const TObject * Obj) { return Obj->is(OBJECT_CLASS_TFarButton); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TFarButton) || TFarDialogItem::is(Kind); }
public:
  explicit TFarButton(TFarDialog * ADialog) noexcept;
  explicit TFarButton(TObjectClassId Kind, TFarDialog * ADialog) noexcept;
  virtual ~TFarButton() noexcept override = default;

  virtual UnicodeString GetCaption() const { return GetData(); }
  virtual void SetCaption(const UnicodeString & Value) { SetData(Value); }
  virtual int32_t GetModalResult() const { return FResult; }
  virtual int32_t GetResult() const { return FResult; }
  virtual void SetResult(int32_t Value) { FResult = Value; }
  virtual UnicodeString GetData() const override;
  virtual UnicodeString GetData() override { return static_cast<const TFarButton *>(this)->GetData(); }
  bool GetDefault() const;
  void SetDefault(bool Value);
  TFarButtonBrackets GetBrackets() const { return FBrackets; }
  void SetBrackets(TFarButtonBrackets Value);
  virtual bool GetCenterGroup() const override { return TFarDialogItem::GetCenterGroup(); }
  virtual void SetCenterGroup(bool Value) override { TFarDialogItem::SetCenterGroup(Value); }
  virtual const TFarButtonClickEvent & GetOnClick() { return FOnClick; }
  virtual void SetOnClick(TFarButtonClickEvent && Value) { FOnClick = std::move(Value); }

protected:
  virtual void SetDataInternal(const UnicodeString & AValue) override;
  virtual intptr_t ItemProc(intptr_t Msg, void * Param) override;
  virtual bool HotKey(char HotKey) override;

private:
  int32_t FResult{0};
  TFarButtonClickEvent FOnClick;
  TFarButtonBrackets FBrackets;
};

using TFarAllowChangeEvent = nb::FastDelegate3<void,
  TFarDialogItem * /*Sender*/, void * /*NewState*/, bool & /*AllowChange*/>;

class TFarCheckBox : public TFarDialogItem
{
  NB_DISABLE_COPY(TFarCheckBox)
public:
  static bool classof(const TObject * Obj) { return Obj->is(OBJECT_CLASS_TFarCheckBox); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TFarCheckBox) || TFarDialogItem::is(Kind); }
public:
  explicit TFarCheckBox(TFarDialog * ADialog) noexcept;

  virtual UnicodeString GetCaption() const { return GetData(); }
  virtual void SetCaption(const UnicodeString & Value) { SetData(Value); }
  bool GetAllowGrayed() const { return GetFlag(DIF_3STATE); }
  void SetAllowGrayed(bool Value) { SetFlag(DIF_3STATE, Value); }
  virtual TFarAllowChangeEvent & GetOnAllowChange() { return FOnAllowChange; }
  virtual void SetOnAllowChange(TFarAllowChangeEvent && Value) { FOnAllowChange = std::move(Value); }
  virtual bool GetChecked() const override { return TFarDialogItem::GetChecked(); }
  virtual void SetChecked(bool Value) override { TFarDialogItem::SetChecked(Value); }
  virtual intptr_t GetSelected() const override { return TFarDialogItem::GetSelected(); }
  virtual void SetSelected(int32_t Value) override { TFarDialogItem::SetSelected(Value); }

protected:
  TFarAllowChangeEvent FOnAllowChange;
  virtual intptr_t ItemProc(intptr_t Msg, void * Param) override;
  virtual bool GetIsEmpty() const override;
  virtual void SetData(const UnicodeString & Value) override;
};

class TFarRadioButton : public TFarDialogItem
{
public:
  explicit TFarRadioButton(TFarDialog * ADialog) noexcept;

  virtual bool GetChecked() const override { return TFarDialogItem::GetChecked(); }
  virtual void SetChecked(bool Value) override { TFarDialogItem::SetChecked(Value); }
  virtual UnicodeString GetCaption() const { return GetData(); }
  virtual void SetCaption(const UnicodeString & Value) { SetData(Value); }
  virtual TFarAllowChangeEvent & GetOnAllowChange() { return FOnAllowChange; }
  virtual void SetOnAllowChange(TFarAllowChangeEvent && Value) { FOnAllowChange = std::move(Value); }

protected:
  TFarAllowChangeEvent FOnAllowChange;
  virtual intptr_t ItemProc(intptr_t Msg, void * Param) override;
  virtual bool GetIsEmpty() const override;
  virtual void SetData(const UnicodeString & Value) override;
};

class TFarEdit : public TFarDialogItem
{
public:
  static bool classof(const TObject * Obj) { return Obj->is(OBJECT_CLASS_TFarEdit); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TFarEdit) || TFarDialogItem::is(Kind); }
public:
  explicit TFarEdit(TFarDialog * ADialog) noexcept;

  virtual UnicodeString GetText() const { return GetData(); }
  virtual void SetText(const UnicodeString & Value) { SetData(Value); }
  int32_t GetAsInteger() const;
  void SetAsInteger(int32_t Value);
  virtual bool GetPassword() const { return GetAlterType(DI_PSWEDIT); }
  virtual void SetPassword(bool Value) { SetAlterType(DI_PSWEDIT, Value); }
  virtual bool GetFixed() const { return GetAlterType(DI_FIXEDIT); }
  virtual void SetFixed(bool Value) { SetAlterType(DI_FIXEDIT, Value); }
  virtual UnicodeString GetMask() const { return GetHistoryMask(1); }
  virtual void SetMask(const UnicodeString & Value) { SetHistoryMask(1, Value); }
  virtual UnicodeString GetHistory() const { return GetHistoryMask(0); }
  virtual void SetHistory(const UnicodeString & Value) { SetHistoryMask(0, Value); }
  bool GetExpandEnvVars() const { return GetFlag(DIF_EDITEXPAND); }
  void SetExpandEnvVars(bool Value) { SetFlag(DIF_EDITEXPAND, Value); }
  bool GetAutoSelect() const { return GetFlag(DIF_SELECTONENTRY); }
  void SetAutoSelect(bool Value) { SetFlag(DIF_SELECTONENTRY, Value); }
  bool GetReadOnly() const { return GetFlag(DIF_READONLY); }
  void SetReadOnly(bool Value) { SetFlag(DIF_READONLY, Value); }

protected:
  virtual intptr_t ItemProc(intptr_t Msg, void * Param) override;
  virtual void Detach() override;

private:
  UnicodeString GetHistoryMask(size_t Index) const;
  void SetHistoryMask(size_t Index, const UnicodeString & Value);
};

class TFarSeparator : public TFarDialogItem
{
public:
  explicit TFarSeparator(TFarDialog * ADialog) noexcept;

  bool GetDouble() const;
  void SetDouble(bool Value);
  virtual UnicodeString GetCaption() { return GetData(); }
  virtual void SetCaption(const UnicodeString & Value) { SetData(Value); }
  void SetPosition(int32_t Value);
  int32_t GetPosition() const;

protected:
  virtual void ResetBounds() override;
};

class TFarText final : public TFarDialogItem
{
public:
  static bool classof(const TObject * Obj) { return Obj->is(OBJECT_CLASS_TFarText); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TFarText) || TFarDialogItem::is(Kind); }
public:
  explicit TFarText(TFarDialog * ADialog) noexcept;

  UnicodeString GetCaption() const { return GetData(); }
  void SetCaption(const UnicodeString & Value) { SetData(Value); }
  virtual bool GetCenterGroup() const override { return TFarDialogItem::GetCenterGroup(); }
  virtual void SetCenterGroup(bool Value) override { TFarDialogItem::SetCenterGroup(Value); }

protected:
  virtual void SetData(const UnicodeString & Value) override;
};

class TFarListBox;
class TFarComboBox;
class TFarLister;

class TFarList final : public TStringList
{
  friend class TFarListBox;
  friend class TFarLister;
  friend class TFarComboBox;
  NB_DISABLE_COPY(TFarList)
public:
  static bool classof(const TObject * Obj) { return Obj->is(OBJECT_CLASS_TFarList); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TFarList) || TStringList::is(Kind); }
public:
  explicit TFarList(TFarDialogItem * ADialogItem = nullptr) noexcept;
  virtual ~TFarList() noexcept override;

  virtual void Assign(const TPersistent * Source) override;

  int32_t GetSelected() const;
  int32_t GetLastPosChange() const { return FLastPosChange; }
  void SetSelected(int32_t Value);
  int32_t GetTopIndex() const;
  void SetTopIndex(int32_t Value);
  inline int32_t GetSelectedInt(bool Init) const;
  bool GetFlag(int32_t Index, LISTITEMFLAGS Flag) const;
  void SetFlag(int32_t Index, LISTITEMFLAGS Flag, bool Value);
  LISTITEMFLAGS GetFlags(int32_t Index) const;
  void SetFlags(int32_t Index, LISTITEMFLAGS Value);
  int32_t GetMaxLength() const;
  int32_t GetVisibleCount() const;
  bool GetDisabled(int32_t Index) const { return GetFlag(Index, LIF_DISABLE); }
  void SetDisabled(int32_t Index, bool Value) { SetFlag(Index, LIF_DISABLE, Value); }
  bool GetChecked(int32_t Index) const { return GetFlag(Index, LIF_CHECKED); }
  void SetChecked(int32_t Index, bool Value) { SetFlag(Index, LIF_CHECKED, Value); }

protected:
  virtual void Changed() override;
  int32_t ItemProc(intptr_t Msg, void * Param);
  void Init();
  void UpdatePosition(int32_t Position);
  int32_t GetPosition() const;
  void Put(int32_t Index, const UnicodeString & Str);
  void SetCurPos(int32_t Position, int32_t TopIndex);
  void UpdateItem(int32_t Index);

  FarList * GetListItems() const { return FListItems; }
  FarList * GetListItems() { return FListItems; }
  TFarDialogItem * GetDialogItem() const { return FDialogItem; }
  TFarDialogItem * GetDialogItem() { return FDialogItem; }

private:
  FarList * FListItems{nullptr};
  TFarDialogItem * FDialogItem{nullptr};
  bool FNoDialogUpdate{false};
  int32_t FLastPosChange{0};
};

enum TFarListBoxAutoSelect
{
  asOnlyFocus,
  asAlways,
  asNever
};

class TFarListBox final : public TFarDialogItem
{
  NB_DISABLE_COPY(TFarListBox)
public:
  static bool classof(const TObject * Obj) { return Obj->is(OBJECT_CLASS_TFarListBox); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TFarListBox) || TFarDialogItem::is(Kind); }
public:
  TFarListBox() = delete;
  explicit TFarListBox(TFarDialog * ADialog) noexcept;
  virtual ~TFarListBox() noexcept override = default;

  void SetItems(const TStrings * Value, bool OwnItems = true);

  bool GetNoAmpersand() const { return GetFlag(DIF_LISTNOAMPERSAND); }
  void SetNoAmpersand(bool Value) { SetFlag(DIF_LISTNOAMPERSAND, Value); }
  bool GetAutoHighlight() const { return GetFlag(DIF_LISTAUTOHIGHLIGHT); }
  void SetAutoHighlight(bool Value) { SetFlag(DIF_LISTAUTOHIGHLIGHT, Value); }
  bool GetNoBox() const { return GetFlag(DIF_LISTNOBOX); }
  void SetNoBox(bool Value) { SetFlag(DIF_LISTNOBOX, Value); }
  bool GetWrapMode() const { return GetFlag(DIF_LISTWRAPMODE); }
  void SetWrapMode(bool Value) { SetFlag(DIF_LISTWRAPMODE, Value); }
  TFarList * GetItems() const { return FList.get(); }
  TFarList * GetItems() { return FList.get(); }
  void SetList(const TFarList * Value, bool OwnItems = true);
  TFarListBoxAutoSelect GetAutoSelect() const { return FAutoSelect; }
  void SetAutoSelect(TFarListBoxAutoSelect Value);

protected:
  virtual intptr_t ItemProc(intptr_t Msg, void * Param) override;
  virtual void Init() override;
  virtual bool CloseQuery() override;

private:
  void UpdateMouseReaction();

private:
  std::unique_ptr<TFarList> FList{std::make_unique<TFarList>(this)};
  TFarListBoxAutoSelect FAutoSelect;
  bool FDenyClose{false};
};

class TFarComboBox final : public TFarDialogItem
{
  NB_DISABLE_COPY(TFarComboBox)
public:
  TFarComboBox() = delete;
  explicit TFarComboBox(TFarDialog * ADialog) noexcept;
  virtual ~TFarComboBox() noexcept override = default;

  void ResizeToFitContent();

  bool GetNoAmpersand() const { return GetFlag(DIF_LISTNOAMPERSAND); }
  void SetNoAmpersand(bool Value) { SetFlag(DIF_LISTNOAMPERSAND, Value); }
  bool GetAutoHighlight() const { return GetFlag(DIF_LISTAUTOHIGHLIGHT); }
  void SetAutoHighlight(bool Value) { SetFlag(DIF_LISTAUTOHIGHLIGHT, Value); }
  bool GetWrapMode() const { return GetFlag(DIF_LISTWRAPMODE); }
  void SetWrapMode(bool Value) { SetFlag(DIF_LISTWRAPMODE, Value); }
  TFarList * GetItems() const { return FList.get(); }
  UnicodeString GetText() const { return GetData(); }
  void SetText(const UnicodeString & Value) { SetData(Value); }
  bool GetAutoSelect() const { return GetFlag(DIF_SELECTONENTRY); }
  void SetAutoSelect(bool Value) { SetFlag(DIF_SELECTONENTRY, Value); }
  bool GetDropDownList() const { return GetFlag(DIF_DROPDOWNLIST); }
  void SetDropDownList(bool Value) { SetFlag(DIF_DROPDOWNLIST, Value); }
  int32_t GetItemIndex() const { return FList->GetSelected(); }
  void SetItemIndex(int32_t Index) { FList->SetSelected(Index); }
  bool GetSetChanged(bool Value) { const bool OldValue = FItemChanged; FItemChanged = Value; return OldValue; }

protected:
  virtual intptr_t ItemProc(intptr_t Msg, void * Param) override;
  virtual void Init() override;

private:
  std::unique_ptr<TFarList> FList{std::make_unique<TFarList>(this)};
  bool FItemChanged{false};
};

class TFarLister final : public TFarDialogItem
{
  NB_DISABLE_COPY(TFarLister)
public:
  TFarLister() = delete;
  explicit TFarLister(TFarDialog * ADialog) noexcept;
  virtual ~TFarLister() noexcept override;

  TStrings * GetItems() const;
  void SetItems(const TStrings * Value);
  int32_t GetTopIndex() const { return FTopIndex; }
  void SetTopIndex(int32_t Value);
  bool GetScrollBar() const;

protected:
  virtual intptr_t ItemProc(intptr_t Msg, void * Param) override;
  virtual void DoFocus() override;

private:
  void ItemsChange(TObject * Sender);

private:
  std::unique_ptr<TStringList> FItems{std::make_unique<TStringList>()};
  int32_t FTopIndex{0};
};

inline TRect Rect(int32_t Left, int32_t Top, int32_t Right, int32_t Bottom);

template<typename ObjectType, typename OwnerType>
ObjectType * MakeOwnedObject(OwnerType * Owner)
{
  std::unique_ptr<ObjectType> Object(std::make_unique<ObjectType>(Owner));
  return Object.release();
}
