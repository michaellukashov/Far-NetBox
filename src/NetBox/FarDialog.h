//---------------------------------------------------------------------------
#ifndef FarDialogH
#define FarDialogH
//---------------------------------------------------------------------------
#pragma once

#include "FarPlugin.h"

#define MAX_SIZE -1


//---------------------------------------------------------------------------

class TFarDialogContainer;
class TFarDialogItem;
class TFarButton;
class TFarSeparator;
class TFarBox;
class TFarList;
struct FarDialogItem;
enum TItemPosition { ipNewLine, ipBelow, ipRight };
//---------------------------------------------------------------------------
DEFINE_CALLBACK_TYPE4(TFarKeyEvent, void,
  TFarDialog * /* Sender */, TFarDialogItem * /* Item */, long /* KeyCode */, bool & /* Handled */);
DEFINE_CALLBACK_TYPE2(TFarMouseClickEvent, void,
  TFarDialogItem * /* Item */, MOUSE_EVENT_RECORD * /* Event */);
DEFINE_CALLBACK_TYPE2(TFarProcessGroupEvent, void,
  TFarDialogItem * /* Item */, void * /* Arg */);
//---------------------------------------------------------------------------
class TFarDialog : public TObject
{
friend TFarDialogItem;
friend TFarDialogContainer;
friend TFarButton;
friend TFarList;
friend class TFarListBox;
public:
  explicit TFarDialog(TCustomFarPlugin * AFarPlugin);
  virtual ~TFarDialog();

  intptr_t ShowModal();
  void ShowGroup(int Group, bool Show);
  void EnableGroup(int Group, bool Enable);

  TRect GetBounds() const { return FBounds; }
  TRect GetClientRect() const;
  UnicodeString GetHelpTopic() const { return FHelpTopic; }
  void SetHelpTopic(UnicodeString Value);
  DWORD GetFlags() const { return FFlags; }
  void SetFlags(DWORD Value);
  bool GetCentered();
  void SetCentered(bool Value);
  TPoint GetSize();
  void SetSize(TPoint Value);
  TPoint GetClientSize();
  int GetWidth();
  void SetWidth(int Value);
  int GetHeight();
  void SetHeight(int Value);
  UnicodeString GetCaption();
  void SetCaption(UnicodeString Value);
  HANDLE GetHandle() { return FHandle; }
  TFarButton * GetDefaultButton() const { return FDefaultButton; }
  TFarBox * GetBorderBox() const { return FBorderBox; }
  intptr_t GetType(TFarDialogItem * Item) const;
  intptr_t GetItem(TFarDialogItem * Item) const;
  TFarDialogItem * GetItem(intptr_t Index);
  intptr_t GetItemCount();
  TItemPosition GetNextItemPosition() { return FNextItemPosition; }
  void SetNextItemPosition(const TItemPosition Value) { FNextItemPosition = Value; }
  intptr_t GetDefaultGroup() const { return FDefaultGroup; }
  void SetDefaultGroup(const int Value) { FDefaultGroup = Value; }
  int GetTag() const { return FTag; }
  void SetTag(int Value) { FTag = Value; }
  TFarDialogItem * GetItemFocused() { return FItemFocused; }
  void SetItemFocused(TFarDialogItem * Value);
  intptr_t GetResult() { return FResult; }
  TPoint GetMaxSize();

  TFarKeyEvent & GetOnKey() { return FOnKey; }
  void SetOnKey(TFarKeyEvent Value) { FOnKey = Value; }

  void Redraw();
  void LockChanges();
  void UnlockChanges();
  uintptr_t GetSystemColor(int Index);
  bool HotKey(uintptr_t Key);

protected:
  TCustomFarPlugin * GetFarPlugin() { return FFarPlugin; }
  TObjectList * GetItems() { return FItems; }
  void Add(TFarDialogItem * Item);
  void Add(TFarDialogContainer * Container);
  LONG_PTR SendMessage(int Msg, intptr_t Param1, LONG_PTR Param2);
  virtual LONG_PTR DialogProc(int Msg, intptr_t Param1, LONG_PTR Param2);
  virtual LONG_PTR FailDialogProc(int Msg, intptr_t Param1, LONG_PTR Param2);
  LONG_PTR DefaultDialogProc(int Msg, intptr_t Param1, LONG_PTR Param2);
  virtual bool MouseEvent(MOUSE_EVENT_RECORD * Event);
  virtual bool Key(TFarDialogItem * Item, LONG_PTR KeyCode);
  virtual void __fastcall Change();
  virtual void Init();
  virtual bool CloseQuery();
  UnicodeString GetMsg(int MsgId);
  void GetNextItemPosition(int & Left, int & Top);
  void RefreshBounds();
  virtual void Idle();
  void BreakSynchronize();
  void Synchronize(TThreadMethod Method);
  void Close(TFarButton * Button);
  void ProcessGroup(int Group, TFarProcessGroupEvent Callback, void * Arg);
  void ShowItem(TFarDialogItem * Item, void * Arg);
  void EnableItem(TFarDialogItem * Item, void * Arg);
  bool ChangesLocked();
  TFarDialogItem * ItemAt(int X, int Y);

  static LONG_PTR WINAPI DialogProcGeneral(HANDLE Handle, int Msg, int Param1, LONG_PTR Param2);

  virtual void SetBounds(TRect Value);

private:
  TCustomFarPlugin * FFarPlugin;
  TRect FBounds;
  DWORD FFlags;
  UnicodeString FHelpTopic;
  bool FVisible;
  TObjectList * FItems;
  TObjectList * FContainers;
  HANDLE FHandle;
  TFarButton * FDefaultButton;
  TFarBox * FBorderBox;
  TItemPosition FNextItemPosition;
  int FDefaultGroup;
  int FTag;
  TFarDialogItem * FItemFocused;
  TFarKeyEvent FOnKey;
  FarDialogItem * FDialogItems;
  int FDialogItemsCapacity;
  int FChangesLocked;
  bool FChangesPending;
  intptr_t FResult;
  bool FNeedsSynchronize;
  HANDLE FSynchronizeObjects[2];
  TThreadMethod FSynchronizeMethod;
};
//---------------------------------------------------------------------------
class TFarDialogContainer : public TObject
{
friend TFarDialog;
friend TFarDialogItem;
public:
  int GetLeft() { return FLeft; }
  void SetLeft(int Value) { SetPosition(0, Value); }
  int GetTop() { return FTop; }
  void SetTop(int Value) { SetPosition(1, Value); }
  bool GetEnabled() { return FEnabled; }
  void SetEnabled(bool Value);
  void SetPosition(int Index, int Value);
  intptr_t GetItemCount() const;

protected:
  explicit TFarDialogContainer(TFarDialog * ADialog);
  virtual ~TFarDialogContainer();

  TFarDialog * GetDialog() { return FDialog; }

  void Add(TFarDialogItem * Item);
  void Remove(TFarDialogItem * Item);
  virtual void __fastcall Change();
  UnicodeString GetMsg(int MsgId);

private:
  int FLeft;
  int FTop;
  TObjectList * FItems;
  TFarDialog * FDialog;
  bool FEnabled;
};
//---------------------------------------------------------------------------
#define DIF_INVERSE 0x00000001UL
//---------------------------------------------------------------------------
class TFarDialogItem : public TObject
{
  friend TFarDialog;
  friend TFarDialogContainer;
  friend TFarList;
public:
  TRect GetBounds() { return FBounds; }
  TRect GetActualBounds();
  int GetLeft() { return GetCoordinate(0); }
  void SetLeft(int Value) { SetCoordinate(0, Value); }
  int GetTop() { return GetCoordinate(1); }
  void SetTop(int Value) { SetCoordinate(1, Value); }
  int GetRight() { return GetCoordinate(2); }
  void SetRight(int Value) { SetCoordinate(2, Value); }
  int GetBottom() { return GetCoordinate(3); }
  void SetBottom(int Value) { SetCoordinate(3, Value); }
  int GetWidth();
  void SetWidth(intptr_t Value);
  intptr_t GetHeight();
  void SetHeight(int Value);
  bool GetEnabled() { return FEnabled; }
  void SetEnabled(bool Value);
  bool GetIsEnabled() { return FIsEnabled; }
  TFarDialogItem * GetEnabledFollow() { return FEnabledFollow; }
  void SetEnabledFollow(TFarDialogItem * Value);
  TFarDialogItem * GetEnabledDependency() { return FEnabledDependency; }
  void SetEnabledDependency(TFarDialogItem * Value);
  TFarDialogItem * GetEnabledDependencyNegative() { return FEnabledDependencyNegative; }
  void SetEnabledDependencyNegative(TFarDialogItem * Value);
  virtual bool GetIsEmpty();
  intptr_t GetGroup() { return FGroup; }
  void SetGroup(intptr_t Value) { FGroup = Value; }
  bool GetVisible() { return GetFlag(DIF_HIDDEN | DIF_INVERSE); }
  void SetVisible(bool Value) { SetFlag(DIF_HIDDEN | DIF_INVERSE, Value); }
  bool GetTabStop() { return GetFlag(DIF_NOFOCUS | DIF_INVERSE); }
  void SetTabStop(bool Value) { SetFlag(DIF_NOFOCUS | DIF_INVERSE, Value); }
  int GetTag() { return FTag; }
  void SetTag(int Value) { FTag = Value; }
  TFarDialog * GetDialog() { return FDialog; }

  TNotifyEvent & GetOnExit() { return FOnExit; }
  void SetOnExit(TNotifyEvent Value) { FOnExit = Value; }
  TFarMouseClickEvent & GetOnMouseClick() { return FOnMouseClick; }
  void SetOnMouseClick(TFarMouseClickEvent Value) { FOnMouseClick = Value; }
  bool GetFocused();
  void SetFocused(bool Value);

  void Move(int DeltaX, int DeltaY);
  void MoveAt(int X, int Y);
  virtual bool CanFocus();
  bool Focused();
  void SetFocus();
  void SetItem(intptr_t Value) { FItem = Value; }

protected:
  uintptr_t FDefaultType;
  intptr_t FGroup;
  int FTag;
  TNotifyEvent FOnExit;
  TFarMouseClickEvent FOnMouseClick;

  explicit TFarDialogItem(TFarDialog * ADialog, uintptr_t AType);
  virtual ~TFarDialogItem();

  FarDialogItem * GetDialogItem();
  bool GetCenterGroup() { return GetFlag(DIF_CENTERGROUP); }
  void SetCenterGroup(bool Value) { SetFlag(DIF_CENTERGROUP, Value); }
  virtual UnicodeString GetData();
  virtual void SetData(const UnicodeString Value);
  uintptr_t GetType();
  void SetType(uintptr_t Value);
  intptr_t  GetItem() { return FItem; }
  intptr_t GetSelected();
  void SetSelected(intptr_t Value);
  TFarDialogContainer * GetContainer() { return FContainer; }
  void SetContainer(TFarDialogContainer * Value);
  bool GetChecked();
  void SetChecked(bool Value);
  void SetBounds(TRect Value);
  DWORD GetFlags();
  void SetFlags(DWORD Value);
  void UpdateFlags(DWORD Value);
  int GetCoordinate(int Index);
  void SetCoordinate(int Index, int Value);
  TFarDialogItem * GetPrevItem();
  void UpdateFocused(bool Value);
  void UpdateEnabled();

  virtual void Detach();
  void DialogResized();
  LONG_PTR SendMessage(int Msg, LONG_PTR Param);
  LONG_PTR SendDialogMessage(int Msg, intptr_t Param1, LONG_PTR Param2);
  virtual LONG_PTR ItemProc(int Msg, LONG_PTR Param);
  LONG_PTR DefaultItemProc(int Msg, LONG_PTR Param);
  LONG_PTR DefaultDialogProc(int Msg, intptr_t Param1, LONG_PTR Param2);
  virtual LONG_PTR FailItemProc(int Msg, LONG_PTR Param);
  virtual void __fastcall Change();
  void DialogChange();
  void SetAlterType(int Index, bool Value);
  bool GetAlterType(int Index);
  virtual void UpdateBounds();
  virtual void ResetBounds();
  virtual void Init();
  virtual bool CloseQuery();
  virtual bool MouseMove(int X, int Y, MOUSE_EVENT_RECORD * Event);
  virtual bool MouseClick(MOUSE_EVENT_RECORD * Event);
  TPoint MouseClientPosition(MOUSE_EVENT_RECORD * Event);
  void Text(int X, int Y, uintptr_t Color, const UnicodeString Str);
  void Redraw();
  virtual bool HotKey(char HotKey);

public:
  virtual void SetDataInternal(const UnicodeString Value);
  void UpdateData(const UnicodeString Value);
  void UpdateSelected(intptr_t Value);

  bool GetFlag(int Index);
  void SetFlag(int Index, bool Value);

  virtual void DoFocus();
  virtual void DoExit();

  char GetColor(int Index);
  void SetColor(int Index, char Value);

private:
  TFarDialog * FDialog;
  TRect FBounds;
  TFarDialogItem * FEnabledFollow;
  TFarDialogItem * FEnabledDependency;
  TFarDialogItem * FEnabledDependencyNegative;
  TFarDialogContainer * FContainer;
  intptr_t FItem;
  bool FEnabled;
  bool FIsEnabled;
  unsigned long FColors;
  unsigned long FColorMask;
};
//---------------------------------------------------------------------------
class TFarBox : public TFarDialogItem
{
public:
  explicit TFarBox(TFarDialog * ADialog);

  virtual UnicodeString GetCaption() { return GetData(); }
  virtual void SetCaption(const UnicodeString Value) { SetData(Value); }
  virtual bool GetDouble() { return GetAlterType(DI_DOUBLEBOX); }
  virtual void SetDouble(bool Value) { SetAlterType(DI_DOUBLEBOX, Value); }
};
//---------------------------------------------------------------------------
DEFINE_CALLBACK_TYPE2(TFarButtonClickEvent, void,
  TFarButton * /* Sender */, bool & /* Close */);
enum TFarButtonBrackets { brNone, brTight, brSpace, brNormal };
//---------------------------------------------------------------------------
class TFarButton : public TFarDialogItem
{
public:
  explicit TFarButton(TFarDialog * ADialog);
  virtual ~TFarButton() {}

  virtual UnicodeString GetCaption() { return GetData(); }
  virtual void SetCaption(const UnicodeString Value) { SetData(Value); }
  virtual int GetResult() { return FResult; }
  virtual void SetResult(int Value) { FResult = Value; }
  virtual UnicodeString GetData();
  bool GetDefault();
  void SetDefault(bool Value);
  TFarButtonBrackets GetBrackets() { return FBrackets; }
  void SetBrackets(TFarButtonBrackets Value);
  bool GetCenterGroup() { return TFarDialogItem::GetCenterGroup(); }
  void SetCenterGroup(bool Value) { TFarDialogItem::SetCenterGroup(Value); }
  virtual TFarButtonClickEvent & GetOnClick() { return FOnClick; }
  virtual void SetOnClick(TFarButtonClickEvent Value) { FOnClick = Value; }

protected:
  virtual void SetDataInternal(const UnicodeString Value);
  virtual LONG_PTR ItemProc(int Msg, LONG_PTR Param);
  virtual bool HotKey(char HotKey);

private:
  int FResult;
  TFarButtonClickEvent FOnClick;
  TFarButtonBrackets FBrackets;
};
//---------------------------------------------------------------------------
DEFINE_CALLBACK_TYPE3(TFarAllowChangeEvent, void,
  TFarDialogItem * /* Sender */, intptr_t /* NewState */, bool & /* AllowChange */);
//---------------------------------------------------------------------------
class TFarCheckBox : public TFarDialogItem
{
public:
  explicit TFarCheckBox(TFarDialog * ADialog);

  virtual UnicodeString GetCaption() { return GetData(); }
  virtual void SetCaption(const UnicodeString Value) { SetData(Value); }
  bool GetAllowGrayed() { return GetFlag(DIF_3STATE); }
  void SetAllowGrayed(bool Value) { SetFlag(DIF_3STATE, Value); }
  virtual TFarAllowChangeEvent & GetOnAllowChange() { return FOnAllowChange; }
  virtual void SetOnAllowChange(TFarAllowChangeEvent Value) { FOnAllowChange = Value; }
  bool GetChecked() { return TFarDialogItem::GetChecked(); }
  void SetChecked(bool Value) { TFarDialogItem::SetChecked(Value); }
  intptr_t GetSelected() { return TFarDialogItem::GetSelected(); }
  void SetSelected(intptr_t Value) { TFarDialogItem::SetSelected(Value); }

protected:
  TFarAllowChangeEvent FOnAllowChange;
  virtual LONG_PTR ItemProc(int Msg, LONG_PTR Param);
  virtual bool GetIsEmpty();
  virtual void SetData(const UnicodeString Value);
};
//---------------------------------------------------------------------------
class TFarRadioButton : public TFarDialogItem
{
public:
  explicit TFarRadioButton(TFarDialog * ADialog);

  bool GetChecked() { return TFarDialogItem::GetChecked(); }
  void SetChecked(bool Value) { TFarDialogItem::SetChecked(Value); }
  virtual UnicodeString GetCaption() { return GetData(); }
  virtual void SetCaption(const UnicodeString Value) { SetData(Value); }
  virtual TFarAllowChangeEvent & GetOnAllowChange() { return FOnAllowChange; }
  virtual void SetOnAllowChange(TFarAllowChangeEvent Value) { FOnAllowChange = Value; }

protected:
  TFarAllowChangeEvent FOnAllowChange;
  virtual LONG_PTR ItemProc(int Msg, LONG_PTR Param);
  virtual bool GetIsEmpty();
  virtual void SetData(const UnicodeString Value);
};
//---------------------------------------------------------------------------
class TFarEdit : public TFarDialogItem
{
public:
  explicit TFarEdit(TFarDialog * ADialog);

  virtual UnicodeString GetText() { return GetData(); }
  virtual void SetText(const UnicodeString Value) { SetData(Value); }
  int GetAsInteger();
  void SetAsInteger(int Value);
  virtual bool GetPassword() { return GetAlterType(DI_PSWEDIT); }
  virtual void SetPassword(bool Value) { SetAlterType(DI_PSWEDIT, Value); }
  virtual bool GetFixed() { return GetAlterType(DI_FIXEDIT); }
  virtual void SetFixed(bool Value) { SetAlterType(DI_FIXEDIT, Value); }
  virtual UnicodeString GetMask() { return GetHistoryMask(1); }
  virtual void SetMask(const UnicodeString Value) { SetHistoryMask(1, Value); }
  virtual UnicodeString GetHistory() { return GetHistoryMask(0); }
  virtual void SetHistory(const UnicodeString Value) { SetHistoryMask(0, Value); }
  bool GetExpandEnvVars() { return GetFlag(DIF_EDITEXPAND); }
  void SetExpandEnvVars(bool Value) { SetFlag(DIF_EDITEXPAND, Value); }
  bool GetAutoSelect() { return GetFlag(DIF_SELECTONENTRY); }
  void SetAutoSelect(bool Value) { SetFlag(DIF_SELECTONENTRY, Value); }
  bool GetReadOnly() { return GetFlag(DIF_READONLY); }
  void SetReadOnly(bool Value) { SetFlag(DIF_READONLY, Value); }

protected:
  virtual LONG_PTR ItemProc(int Msg, LONG_PTR Param);
  virtual void Detach();

private:
  UnicodeString GetHistoryMask(size_t Index);
  void SetHistoryMask(size_t Index, const UnicodeString Value);
};
//---------------------------------------------------------------------------
class TFarSeparator : public TFarDialogItem
{
public:
  explicit TFarSeparator(TFarDialog * ADialog);

  bool GetDouble();
  void SetDouble(bool Value);
  virtual UnicodeString GetCaption() { return GetData(); }
  virtual void SetCaption(const UnicodeString Value) { SetData(Value); }
  void SetPosition(int Value);
  int GetPosition();

protected:
  virtual void ResetBounds();
};
//---------------------------------------------------------------------------
class TFarText : public TFarDialogItem
{
public:
  explicit  TFarText(TFarDialog * ADialog);

  virtual UnicodeString GetCaption() { return GetData(); }
  virtual void SetCaption(const UnicodeString Value) { SetData(Value); }
  bool GetCenterGroup() { return TFarDialogItem::GetCenterGroup(); }
  void SetCenterGroup(bool Value) { TFarDialogItem::SetCenterGroup(Value); }
  char GetColor() { return TFarDialogItem::GetColor(0); }
  void SetColor(char Value) { TFarDialogItem::SetColor(0, Value); }

protected:
  virtual void SetData(const UnicodeString Value);
};
//---------------------------------------------------------------------------
class TFarListBox;
class TFarComboBox;
class TFarLister;
//---------------------------------------------------------------------------
class TFarList : public TStringList
{
  friend TFarListBox;
  friend TFarLister;
  friend TFarComboBox;
public:
  explicit TFarList(TFarDialogItem * ADialogItem = NULL);
  virtual ~TFarList();

  virtual void __fastcall Assign(TPersistent * Source);

  intptr_t GetSelected();
  void SetSelected(intptr_t Value);
  intptr_t GetTopIndex();
  void SetTopIndex(intptr_t Value);
  inline intptr_t GetSelectedInt(bool Init);
  bool GetFlag(intptr_t Index, DWORD Flag);
  void SetFlag(intptr_t Index, DWORD Flag, bool Value);
  DWORD GetFlags(intptr_t Index);
  void SetFlags(intptr_t Index, DWORD Value);
  intptr_t GetMaxLength();
  intptr_t GetVisibleCount();
  bool GetDisabled(intptr_t Index) { return GetFlag(Index, LIF_DISABLE); }
  void SetDisabled(intptr_t Index, bool Value) { SetFlag(Index, LIF_DISABLE, Value); }
  bool GetChecked(intptr_t Index) { return GetFlag(Index, LIF_CHECKED); }
  void SetChecked(intptr_t Index, bool Value) { SetFlag(Index, LIF_CHECKED, Value); }

protected:
  virtual void __fastcall Changed();
  virtual LONG_PTR ItemProc(int Msg, LONG_PTR Param);
  virtual void Init();
  void UpdatePosition(intptr_t Position);
  intptr_t GetPosition();
  virtual void Put(int Index, const UnicodeString S);
  void SetCurPos(intptr_t Position, intptr_t TopIndex);
  void UpdateItem(intptr_t Index);

  FarList * GetListItems() { return FListItems; }
  TFarDialogItem * GetDialogItem() { return FDialogItem; }

private:
  FarList * FListItems;
  TFarDialogItem * FDialogItem;
  bool FNoDialogUpdate;
};
//---------------------------------------------------------------------------
enum TFarListBoxAutoSelect { asOnlyFocus, asAlways, asNever };
//---------------------------------------------------------------------------
class TFarListBox : public TFarDialogItem
{
public:
  explicit TFarListBox(TFarDialog * ADialog);
  virtual ~TFarListBox();

  void SetItems(TStrings * Value);

  bool GetNoAmpersand() { return GetFlag(DIF_LISTNOAMPERSAND); }
  void SetNoAmpersand(bool Value) { SetFlag(DIF_LISTNOAMPERSAND, Value); }
  bool GetAutoHighlight() { return GetFlag(DIF_LISTAUTOHIGHLIGHT); }
  void SetAutoHighlight(bool Value) { SetFlag(DIF_LISTAUTOHIGHLIGHT, Value); }
  bool GetNoBox() { return GetFlag(DIF_LISTNOBOX); }
  void SetNoBox(bool Value) { SetFlag(DIF_LISTNOBOX, Value); }
  bool GetWrapMode() { return GetFlag(DIF_LISTWRAPMODE); }
  void SetWrapMode(bool Value) { SetFlag(DIF_LISTWRAPMODE, Value); }
  TFarList * GetItems() { return FList; }
  void SetList(TFarList * Value);
  TFarListBoxAutoSelect GetAutoSelect() { return FAutoSelect; }
  void SetAutoSelect(TFarListBoxAutoSelect Value);

protected:
  virtual LONG_PTR ItemProc(int Msg, LONG_PTR Param);
  virtual void Init();
  virtual bool CloseQuery();

private:
  TFarList * FList;
  TFarListBoxAutoSelect FAutoSelect;
  bool FDenyClose;

  void UpdateMouseReaction();
};
//---------------------------------------------------------------------------
class TFarComboBox : public TFarDialogItem
{
public:
  explicit TFarComboBox(TFarDialog * ADialog);
  virtual ~TFarComboBox();

  void ResizeToFitContent();

  bool GetNoAmpersand() { return GetFlag(DIF_LISTNOAMPERSAND); }
  void SetNoAmpersand(bool Value) { SetFlag(DIF_LISTNOAMPERSAND, Value); }
  bool GetAutoHighlight() { return GetFlag(DIF_LISTAUTOHIGHLIGHT); }
  void SetAutoHighlight(bool Value) { SetFlag(DIF_LISTAUTOHIGHLIGHT, Value); }
  bool GetWrapMode() { return GetFlag(DIF_LISTWRAPMODE); }
  void SetWrapMode(bool Value) { SetFlag(DIF_LISTWRAPMODE, Value); }
  TFarList * GetItems() { return FList; }
  virtual UnicodeString GetText() { return GetData(); }
  virtual void SetText(const UnicodeString Value) { SetData(Value); }
  bool GetAutoSelect() { return GetFlag(DIF_SELECTONENTRY); }
  void SetAutoSelect(bool Value) { SetFlag(DIF_SELECTONENTRY, Value); }
  bool GetDropDownList() { return GetFlag(DIF_DROPDOWNLIST); }
  void SetDropDownList(bool Value) { SetFlag(DIF_DROPDOWNLIST, Value); }
  intptr_t GetItemIndex() const { return FList->GetSelected(); }
  void SetItemIndex(intptr_t Index) { FList->SetSelected(Index); }

protected:
  virtual LONG_PTR ItemProc(int Msg, LONG_PTR Param);
  virtual void Init();

private:
  TFarList * FList;
};
//---------------------------------------------------------------------------
class TFarLister : public TFarDialogItem
{
public:
  explicit TFarLister(TFarDialog * ADialog);
  virtual ~TFarLister();

  TStrings * GetItems();
  void SetItems(TStrings * Value);
  intptr_t GetTopIndex() const { return FTopIndex; }
  void SetTopIndex(intptr_t Value);
  bool GetScrollBar();

protected:
  virtual LONG_PTR ItemProc(int Msg, LONG_PTR Param);
  virtual void DoFocus();

private:
  TStringList * FItems;
  intptr_t FTopIndex;

  void ItemsChange(TObject * Sender);
};
//---------------------------------------------------------------------------
UnicodeString StripHotKey(const UnicodeString Text);
TRect Rect(int Left, int Top, int Right, int Bottom);
//---------------------------------------------------------------------------
#endif
