#pragma once

#pragma warning(push, 1)
#include <farcolor.hpp>
#pragma warning(pop)
#include "FarPlugin.h"

constexpr const int32_t MAX_SIZE = -1;

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
  TFarDialog * /*Sender*/, TFarDialogItem * /*Item*/, long /*KeyCode*/, bool & /*Handled*/>;
using TFarMouseClickEvent = nb::FastDelegate2<void,
  TFarDialogItem * /*Item*/, MOUSE_EVENT_RECORD * /*Event*/>;
using TFarProcessGroupEvent = nb::FastDelegate2<void,
  TFarDialogItem * /*Item*/, void * /*Arg*/>;

NB_DEFINE_CLASS_ID(TFarDialog);
class TFarDialog : public TObject
{
  friend class TFarDialogItem;
  friend class TFarDialogContainer;
  friend class TFarButton;
  friend class TFarList;
  friend class TFarListBox;
  NB_DISABLE_COPY(TFarDialog)
public:
  static bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TFarDialog); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TFarDialog) || TObject::is(Kind); }
public:
  explicit TFarDialog(TCustomFarPlugin * AFarPlugin) noexcept;
  virtual ~TFarDialog() noexcept;

  int32_t ShowModal();
  void ShowGroup(int32_t Group, bool Show);
  void EnableGroup(int32_t Group, bool Enable);

  TRect GetBounds() const { return FBounds; }
  TRect GetClientRect() const;
  UnicodeString GetHelpTopic() const { return FHelpTopic; }
  void SetHelpTopic(const UnicodeString & Value);
  FARDIALOGITEMFLAGS GetFlags() const { return FFlags; }
  void SetFlags(const FARDIALOGITEMFLAGS Value);
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
  int32_t GetType(TFarDialogItem * Item) const;
  int32_t GetItem(TFarDialogItem * Item) const;
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
  void SetItemFocused(TFarDialogItem *Value);
  int32_t GetResult() const { return FResult; }
  TPoint GetMaxSize() const;

  TFarKeyEvent GetOnKey() const { return FOnKey; }
  void SetOnKey(TFarKeyEvent Value) { FOnKey = Value; }

  void Redraw();
  void LockChanges();
  void UnlockChanges();
  FarColor GetSystemColor(PaletteColors colorId);
  bool HotKey(uint32_t Key, uint32_t ControlState) const;

protected:
  TCustomFarPlugin * GetFarPlugin() const { return FFarPlugin; }
  TCustomFarPlugin * GetFarPlugin() { return FFarPlugin; }
  TObjectList * GetItems() const { return FItems.get(); }
  TObjectList * GetItems() { return FItems.get(); }
  void Add(TFarDialogItem * DialogItem);
  void Add(TFarDialogContainer * Container);
  intptr_t SendDlgMessage(int32_t Msg, int32_t Param1, void * Param2);
  virtual intptr_t DialogProc(intptr_t Msg, intptr_t Param1, void * Param2);
  virtual intptr_t FailDialogProc(intptr_t Msg, intptr_t Param1, void * Param2);
  intptr_t DefaultDialogProc(int32_t Msg, int32_t Param1, void * Param2);
  virtual bool MouseEvent(MOUSE_EVENT_RECORD * Event);
  virtual bool Key(TFarDialogItem * Item, LONG_PTR KeyCode);
  virtual void Change();
  virtual void Init();
  virtual bool CloseQuery();
  UnicodeString GetMsg(int32_t MsgId) const;
  void GetNextItemPosition(int32_t & Left, int32_t & Top);
  void RefreshBounds();
  virtual void Idle();
  void BreakSynchronize();
  void Synchronize(TThreadMethod Method);
  void Close(TFarButton * Button);
  void ProcessGroup(int32_t Group, TFarProcessGroupEvent Callback, void * Arg);
  void ShowItem(TFarDialogItem * Item, void * Arg);
  void EnableItem(TFarDialogItem * Item, void * Arg);
  bool ChangesLocked() const;
  TFarDialogItem * ItemAt(int32_t X, int32_t Y);

  static intptr_t WINAPI DialogProcGeneral(HANDLE Handle, intptr_t Msg, intptr_t Param1, void * Param2);

  virtual void SetBounds(const TRect & Value);

private:
  mutable TCustomFarPlugin * FFarPlugin{nullptr};
  TRect FBounds{};
  FARDIALOGITEMFLAGS FFlags;
  UnicodeString FHelpTopic;
  bool FVisible{false};
  std::unique_ptr<TObjectList> FItems;
  std::unique_ptr<TObjectList> FContainers;
  HANDLE FHandle{nullptr};
  TFarButton * FDefaultButton{nullptr};
  TFarBox * FBorderBox{nullptr};
  TItemPosition FNextItemPosition{};
  int32_t FDefaultGroup{0};
  int32_t FTag{0};
  TFarDialogItem * FItemFocused{nullptr};
  TFarKeyEvent FOnKey;
  FarDialogItem * FDialogItems{nullptr};
  int32_t FDialogItemsCapacity{0};
  int32_t FChangesLocked{0};
  bool FChangesPending{false};
  int32_t FResult{0};
  bool FNeedsSynchronize{false};
  HANDLE FSynchronizeObjects[2]{};
  TThreadMethod FSynchronizeMethod;
};

NB_DEFINE_CLASS_ID(TFarDialogContainer);
class TFarDialogContainer : public TObject
{
  friend class TFarDialog;
  friend class TFarDialogItem;
  NB_DISABLE_COPY(TFarDialogContainer)
public:
  static bool classof(const TObject * Obj) { return Obj->is(OBJECT_CLASS_TFarDialogContainer); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TFarDialogContainer) || TObject::is(Kind); }
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
  virtual ~TFarDialogContainer() noexcept;

  TFarDialog * GetDialog() const { return FDialog; }
  TFarDialog * GetDialog() { return FDialog; }

  void Add(TFarDialogItem * Item);
  void Remove(TFarDialogItem * Item);
  virtual void Change();
  UnicodeString GetMsg(int32_t MsgId) const;

private:
  int32_t FLeft{0};
  int32_t FTop{0};
  std::unique_ptr<TObjectList> FItems;
  TFarDialog * FDialog{nullptr};
  bool FEnabled{false};
};

constexpr int32_t DIF_INVERSE = 0x00000001UL;

NB_DEFINE_CLASS_ID(TFarDialogItem);
class TFarDialogItem : public TObject
{
  friend class TFarDialog;
  friend class TFarMessageDialog;
  friend class TFarDialogContainer;
  friend class TFarList;
  NB_DISABLE_COPY(TFarDialogItem)
public:
  static bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TFarDialogItem); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TFarDialogItem) || TObject::is(Kind); }
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

  TNotifyEvent GetOnExit() const { return FOnExit; }
  void SetOnExit(TNotifyEvent Value) { FOnExit = Value; }
  TFarMouseClickEvent GetOnMouseClick() const { return FOnMouseClick; }
  void SetOnMouseClick(TFarMouseClickEvent Value) { FOnMouseClick = Value; }
  bool GetFocused() const;
  void SetFocused(bool Value);

  void Move(int32_t DeltaX, int32_t DeltaY);
  void MoveAt(int32_t X, int32_t Y);
  virtual bool CanFocus() const;
  bool Focused() const;
  void SetFocus();
  void SetItem(int32_t Value) { FItem = Value; }

public:
  virtual void SetDataInternal(const UnicodeString & Value);
  void UpdateData(const UnicodeString & Value);
  void UpdateSelected(int32_t Value);

  bool GetFlag(FARDIALOGITEMFLAGS Index) const;
  void SetFlag(FARDIALOGITEMFLAGS Index, bool Value);

  virtual void DoFocus();
  virtual void DoExit();

  char GetColor(int32_t Index) const;
  void SetColor(int32_t Index, char Value);

protected:
  FARDIALOGITEMTYPES FDefaultType;
  int32_t FGroup{0};
  int32_t FTag{0};
  TNotifyEvent FOnExit;
  TFarMouseClickEvent FOnMouseClick;

  explicit TFarDialogItem(TObjectClassId Kind, TFarDialog *ADialog, FARDIALOGITEMTYPES AType);
  explicit TFarDialogItem(TObjectClassId Kind, TFarDialog *ADialog, uint32_t AType) noexcept;
  virtual ~TFarDialogItem() noexcept;

  const FarDialogItem *GetDialogItem() const;
  FarDialogItem *GetDialogItem();
  bool GetCenterGroup() const { return GetFlag(DIF_CENTERGROUP); }
  void SetCenterGroup(bool Value) { SetFlag(DIF_CENTERGROUP, Value); }
  virtual UnicodeString GetData() const;
  virtual UnicodeString GetData();
  virtual void SetData(const UnicodeString & Value);
  FARDIALOGITEMTYPES GetType() const;
  void SetType(FARDIALOGITEMTYPES Value);
  int32_t GetItem() const { return FItem; }
  int32_t GetSelected() const;
  void SetSelected(int32_t Value);
  TFarDialogContainer * GetContainer() const { return FContainer; }
  void SetContainer(TFarDialogContainer *Value);
  bool GetChecked() const;
  void SetChecked(bool Value);
  void SetBounds(const TRect & Value);
  FARDIALOGITEMFLAGS GetFlags() const;
  void SetFlags(FARDIALOGITEMFLAGS Value);
  void UpdateFlags(FARDIALOGITEMFLAGS Value);
  int32_t GetCoordinate(int32_t Index) const;
  void SetCoordinate(int32_t Index, int32_t Value);
  TFarDialogItem *GetPrevItem() const;
  void UpdateFocused(bool Value);
  void UpdateEnabled();

  virtual void Detach();
  void DialogResized();
  intptr_t SendDialogMessage(int32_t Msg, void * Param);
  intptr_t SendDialogMessage(int32_t Msg, int32_t Param1, void * Param2);
  virtual intptr_t ItemProc(int32_t Msg, void * Param);
  intptr_t DefaultItemProc(int32_t Msg, void * Param);
  intptr_t DefaultDialogProc(int32_t Msg, int32_t Param1, void * Param2);
  virtual intptr_t FailItemProc(int32_t Msg, void * Param);
  virtual void Change();
  void DialogChange();
  bool GetAlterType(FARDIALOGITEMTYPES Index) const;
  bool GetAlterType(FARDIALOGITEMTYPES Index);
  void SetAlterType(FARDIALOGITEMTYPES Index, bool Value);
  virtual void UpdateBounds();
  virtual void ResetBounds();
  virtual void Init();
  virtual bool CloseQuery();
  virtual bool MouseMove(int32_t X, int32_t Y, MOUSE_EVENT_RECORD *Event);
  virtual bool MouseClick(MOUSE_EVENT_RECORD *Event);
  TPoint MouseClientPosition(MOUSE_EVENT_RECORD *Event);
  void Text(int32_t X, int32_t Y, const FarColor &Color, UnicodeString Str);
  void Redraw();
  virtual bool HotKey(char HotKey);

private:
  const struct PluginStartupInfo *GetPluginStartupInfo() const;


private:
  TFarDialog *FDialog{nullptr};
  TRect FBounds{};
  TFarDialogItem *FEnabledFollow{nullptr};
  TFarDialogItem *FEnabledDependency{nullptr};
  TFarDialogItem *FEnabledDependencyNegative{nullptr};
  TFarDialogContainer *FContainer{nullptr};
  int32_t FItem{0};
  uint32_t FColors{0};
  uint32_t FColorMask{0};
  bool FEnabled{false};
  bool FIsEnabled{false};
};

NB_DEFINE_CLASS_ID(TFarBox);
class TFarBox : public TFarDialogItem
{
public:
  explicit TFarBox(TFarDialog *ADialog) noexcept;

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

NB_DEFINE_CLASS_ID(TFarButton);
class TFarButton : public TFarDialogItem
{
public:
  static bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TFarButton); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TFarButton) || TFarDialogItem::is(Kind); }
public:
  explicit TFarButton(TFarDialog *ADialog) noexcept;
  explicit TFarButton(TObjectClassId Kind, TFarDialog *ADialog) noexcept;
  virtual ~TFarButton() = default;

  virtual UnicodeString GetCaption() const { return GetData(); }
  virtual void SetCaption(const UnicodeString & Value) { SetData(Value); }
  virtual int32_t GetModalResult() const { return FResult; }
  virtual int32_t GetResult() const { return FResult; }
  virtual void SetResult(int32_t Value) { FResult = Value; }
  virtual UnicodeString GetData() const override { return const_cast<TFarButton *>(this)->GetData(); }
  virtual UnicodeString GetData() override;
  bool GetDefault() const;
  void SetDefault(bool Value);
  TFarButtonBrackets GetBrackets() const { return FBrackets; }
  void SetBrackets(TFarButtonBrackets Value);
  bool GetCenterGroup() const { return TFarDialogItem::GetCenterGroup(); }
  void SetCenterGroup(bool Value) { TFarDialogItem::SetCenterGroup(Value); }
  virtual TFarButtonClickEvent GetOnClick() const { return FOnClick; }
  virtual void SetOnClick(TFarButtonClickEvent Value) { FOnClick = Value; }

protected:
  virtual void SetDataInternal(const UnicodeString & AValue) override;
  virtual intptr_t ItemProc(int32_t Msg, void * Param) override;
  virtual bool HotKey(char HotKey) override;

private:
  int32_t FResult{0};
  TFarButtonClickEvent FOnClick;
  TFarButtonBrackets FBrackets;
};

using TFarAllowChangeEvent = nb::FastDelegate3<void,
  TFarDialogItem * /*Sender*/, void * /*NewState*/, bool & /*AllowChange*/>;

NB_DEFINE_CLASS_ID(TFarCheckBox);
class TFarCheckBox : public TFarDialogItem
{
  NB_DISABLE_COPY(TFarCheckBox)
public:
  static bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TFarCheckBox); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TFarCheckBox) || TFarDialogItem::is(Kind); }
public:
  explicit TFarCheckBox(TFarDialog * ADialog) noexcept;

  virtual UnicodeString GetCaption() const { return GetData(); }
  virtual void SetCaption(const UnicodeString & Value) { SetData(Value); }
  bool GetAllowGrayed() const { return GetFlag(DIF_3STATE); }
  void SetAllowGrayed(bool Value) { SetFlag(DIF_3STATE, Value); }
  virtual TFarAllowChangeEvent GetOnAllowChange() const { return FOnAllowChange; }
  virtual void SetOnAllowChange(TFarAllowChangeEvent Value) { FOnAllowChange = Value; }
  bool GetChecked() const { return TFarDialogItem::GetChecked(); }
  void SetChecked(bool Value) { TFarDialogItem::SetChecked(Value); }
  int32_t GetSelected() const { return TFarDialogItem::GetSelected(); }
  void SetSelected(int32_t Value) { TFarDialogItem::SetSelected(Value); }

protected:
  TFarAllowChangeEvent FOnAllowChange;
  virtual intptr_t ItemProc(int32_t Msg, void *Param) override;
  virtual bool GetIsEmpty() const override;
  virtual void SetData(const UnicodeString & Value) override;
};

NB_DEFINE_CLASS_ID(TFarRadioButton);
class TFarRadioButton : public TFarDialogItem
{
public:
  explicit TFarRadioButton(TFarDialog * ADialog) noexcept;

  bool GetChecked() const { return TFarDialogItem::GetChecked(); }
  void SetChecked(bool Value) { TFarDialogItem::SetChecked(Value); }
  virtual UnicodeString GetCaption() const { return GetData(); }
  virtual void SetCaption(const UnicodeString & Value) { SetData(Value); }
  virtual TFarAllowChangeEvent GetOnAllowChange() const { return FOnAllowChange; }
  virtual void SetOnAllowChange(TFarAllowChangeEvent Value) { FOnAllowChange = Value; }

protected:
  TFarAllowChangeEvent FOnAllowChange;
  virtual intptr_t ItemProc(int32_t Msg, void *Param) override;
  virtual bool GetIsEmpty() const override;
  virtual void SetData(const UnicodeString & Value) override;
};

NB_DEFINE_CLASS_ID(TFarEdit);
class TFarEdit : public TFarDialogItem
{
public:
  static bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TFarEdit); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TFarEdit) || TFarDialogItem::is(Kind); }
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
  virtual intptr_t ItemProc(int32_t Msg, void *Param) override;
  virtual void Detach() override;

private:
  UnicodeString GetHistoryMask(size_t Index) const;
  void SetHistoryMask(size_t Index, const UnicodeString & Value);
};

NB_DEFINE_CLASS_ID(TFarSeparator);
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

NB_DEFINE_CLASS_ID(TFarText);
class TFarText : public TFarDialogItem
{
public:
  static bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TFarText); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TFarText) || TFarDialogItem::is(Kind); }
public:
  explicit TFarText(TFarDialog * ADialog) noexcept;

  virtual UnicodeString GetCaption() const { return GetData(); }
  virtual void SetCaption(const UnicodeString & Value) { SetData(Value); }
  bool GetCenterGroup() const { return TFarDialogItem::GetCenterGroup(); }
  void SetCenterGroup(bool Value) { TFarDialogItem::SetCenterGroup(Value); }
  char GetColor() const { return TFarDialogItem::GetColor(0); }
  void SetColor(char Value) { TFarDialogItem::SetColor(0, Value); }

protected:
  virtual void SetData(const UnicodeString & Value) override;
};

class TFarListBox;
class TFarComboBox;
class TFarLister;

NB_DEFINE_CLASS_ID(TFarList);
class TFarList : public TStringList
{
  friend class TFarListBox;
  friend class TFarLister;
  friend class TFarComboBox;
  NB_DISABLE_COPY(TFarList)
public:
  static bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TFarList); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TFarList) || TStringList::is(Kind); }
public:
  explicit TFarList(TFarDialogItem * ADialogItem = nullptr) noexcept;
  virtual ~TFarList() noexcept;

  virtual void Assign(const TPersistent * Source) override;

  int32_t GetSelected() const;
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
  virtual int32_t ItemProc(int32_t Msg, void * Param);
  virtual void Init();
  void UpdatePosition(int32_t Position);
  int32_t GetPosition() const;
  virtual void Put(int32_t Index, const UnicodeString & Str);
  void SetCurPos(int32_t Position, int32_t TopIndex);
  void UpdateItem(int32_t Index);

  FarList * GetListItems() const { return FListItems; }
  FarList * GetListItems() { return FListItems; }
  TFarDialogItem * GetDialogItem() const { return FDialogItem; }
  TFarDialogItem * GetDialogItem() { return FDialogItem; }

private:
  FarList *FListItems{nullptr};
  TFarDialogItem *FDialogItem{nullptr};
  bool FNoDialogUpdate{false};
};

enum TFarListBoxAutoSelect
{
  asOnlyFocus,
  asAlways,
  asNever
};

NB_DEFINE_CLASS_ID(TFarListBox);
class TFarListBox : public TFarDialogItem
{
  NB_DISABLE_COPY(TFarListBox)
public:
  static bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TFarListBox); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TFarListBox) || TFarDialogItem::is(Kind); }
public:
  explicit TFarListBox(TFarDialog *ADialog) noexcept;
  virtual ~TFarListBox() noexcept;

  void SetItems(TStrings *Value);

  bool GetNoAmpersand() const { return GetFlag(DIF_LISTNOAMPERSAND); }
  void SetNoAmpersand(bool Value) { SetFlag(DIF_LISTNOAMPERSAND, Value); }
  bool GetAutoHighlight() const { return GetFlag(DIF_LISTAUTOHIGHLIGHT); }
  void SetAutoHighlight(bool Value) { SetFlag(DIF_LISTAUTOHIGHLIGHT, Value); }
  bool GetNoBox() const { return GetFlag(DIF_LISTNOBOX); }
  void SetNoBox(bool Value) { SetFlag(DIF_LISTNOBOX, Value); }
  bool GetWrapMode() const { return GetFlag(DIF_LISTWRAPMODE); }
  void SetWrapMode(bool Value) { SetFlag(DIF_LISTWRAPMODE, Value); }
  TFarList *GetItems() const { return FList.get(); }
  TFarList *GetItems() { return FList.get(); }
  void SetList(TFarList *Value);
  TFarListBoxAutoSelect GetAutoSelect() { return FAutoSelect; }
  void SetAutoSelect(TFarListBoxAutoSelect Value);

protected:
  virtual intptr_t ItemProc(int32_t Msg, void *Param) override;
  virtual void Init() override;
  virtual bool CloseQuery() override;

private:
  void UpdateMouseReaction();

private:
  std::unique_ptr<TFarList> FList;
  TFarListBoxAutoSelect FAutoSelect;
  bool FDenyClose{false};
};

NB_DEFINE_CLASS_ID(TFarComboBox);
class TFarComboBox : public TFarDialogItem
{
  NB_DISABLE_COPY(TFarComboBox)
public:
  explicit TFarComboBox(TFarDialog * ADialog) noexcept;
  virtual ~TFarComboBox() noexcept;

  void ResizeToFitContent();

  bool GetNoAmpersand() const { return GetFlag(DIF_LISTNOAMPERSAND); }
  void SetNoAmpersand(bool Value) { SetFlag(DIF_LISTNOAMPERSAND, Value); }
  bool GetAutoHighlight() const { return GetFlag(DIF_LISTAUTOHIGHLIGHT); }
  void SetAutoHighlight(bool Value) { SetFlag(DIF_LISTAUTOHIGHLIGHT, Value); }
  bool GetWrapMode() const { return GetFlag(DIF_LISTWRAPMODE); }
  void SetWrapMode(bool Value) { SetFlag(DIF_LISTWRAPMODE, Value); }
  TFarList *GetItems() const { return FList.get(); }
  virtual UnicodeString GetText() const { return GetData(); }
  virtual void SetText(const UnicodeString & Value) { SetData(Value); }
  bool GetAutoSelect() const { return GetFlag(DIF_SELECTONENTRY); }
  void SetAutoSelect(bool Value) { SetFlag(DIF_SELECTONENTRY, Value); }
  bool GetDropDownList() const { return GetFlag(DIF_DROPDOWNLIST); }
  void SetDropDownList(bool Value) { SetFlag(DIF_DROPDOWNLIST, Value); }
  int32_t GetItemIndex() const { return FList->GetSelected(); }
  void SetItemIndex(int32_t Index) { FList->SetSelected(Index); }

protected:
  virtual intptr_t ItemProc(int32_t Msg, void *Param) override;
  virtual void Init() override;

private:
  std::unique_ptr<TFarList> FList;
};

NB_DEFINE_CLASS_ID(TFarLister);
class TFarLister : public TFarDialogItem
{
  NB_DISABLE_COPY(TFarLister)
public:
  TFarLister() = delete;
  explicit TFarLister(TFarDialog * ADialog) noexcept;
  virtual ~TFarLister() noexcept;

  TStrings * GetItems() const;
  void SetItems(const TStrings * Value);
  int32_t GetTopIndex() const { return FTopIndex; }
  void SetTopIndex(int32_t Value);
  bool GetScrollBar() const;

protected:
  virtual intptr_t ItemProc(int32_t Msg, void * Param) override;
  virtual void DoFocus() override;

private:
  void ItemsChange(TObject * Sender);

private:
  std::unique_ptr<TStringList> FItems;
  int32_t FTopIndex{0};
};

inline TRect Rect(int32_t Left, int32_t Top, int32_t Right, int32_t Bottom);

