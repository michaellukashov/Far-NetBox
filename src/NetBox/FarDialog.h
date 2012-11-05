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
typedef TFarDialog self;
public:
  explicit TFarDialog(TCustomFarPlugin * AFarPlugin);
  virtual ~TFarDialog();

  intptr_t __fastcall ShowModal();
  void __fastcall ShowGroup(int Group, bool Show);
  void __fastcall EnableGroup(int Group, bool Enable);

  TRect GetBounds() const { return FBounds; }
  TRect __fastcall GetClientRect();
  UnicodeString GetHelpTopic() { return FHelpTopic; }
  void __fastcall SetHelpTopic(UnicodeString Value);
  FARDIALOGITEMFLAGS __fastcall GetFlags() const { return FFlags; }
  void __fastcall SetFlags(const FARDIALOGITEMFLAGS value);
  bool __fastcall GetCentered();
  void __fastcall SetCentered(bool Value);
  TPoint __fastcall GetSize();
  void __fastcall SetSize(TPoint Value);
  TPoint __fastcall GetClientSize();
  intptr_t __fastcall GetWidth();
  void __fastcall SetWidth(intptr_t Value);
  intptr_t __fastcall GetHeight();
  void __fastcall SetHeight(intptr_t Value);
  UnicodeString __fastcall GetCaption();
  void __fastcall SetCaption(UnicodeString Value);
  HANDLE __fastcall GetHandle() { return FHandle; }
  TFarButton * __fastcall GetDefaultButton() const { return FDefaultButton; }
  TFarBox * __fastcall GetBorderBox() const { return FBorderBox; }
  TFarDialogItem * __fastcall GetItem(int Index);
  int __fastcall GetItemCount();
  TItemPosition __fastcall GetNextItemPosition() { return FNextItemPosition; }
  void __fastcall SetNextItemPosition(const TItemPosition Value) { FNextItemPosition = Value; }
  int __fastcall GetDefaultGroup() const { return FDefaultGroup; }
  void __fastcall SetDefaultGroup(const int Value) { FDefaultGroup = Value; }
  int GetTag() const { return FTag; }
  void SetTag(int Value) { FTag = Value; }
  TFarDialogItem * __fastcall GetItemFocused() { return FItemFocused; }
  void __fastcall SetItemFocused(TFarDialogItem * Value);
  intptr_t __fastcall GetResult() { return FResult; }
  TPoint __fastcall GetMaxSize();

  TFarKeyEvent & GetOnKey() { return FOnKey; }
  void SetOnKey(TFarKeyEvent Value) { FOnKey = Value; }

  void __fastcall Redraw();
  void __fastcall LockChanges();
  void __fastcall UnlockChanges();
  FarColor __fastcall GetSystemColor(PaletteColors colorId);
  bool __fastcall HotKey(uintptr_t Key, uintptr_t ControlState);

protected:
  TCustomFarPlugin * __fastcall GetFarPlugin() { return FFarPlugin; }
  TObjectList * __fastcall GetItems() { return FItems; }
  void __fastcall Add(TFarDialogItem * Item);
  void __fastcall Add(TFarDialogContainer * Container);
  intptr_t __fastcall SendMessage(intptr_t Msg, intptr_t Param1, void * Param2);
  virtual intptr_t __fastcall DialogProc(intptr_t Msg, intptr_t Param1, void * Param2);
  virtual intptr_t __fastcall FailDialogProc(intptr_t Msg, intptr_t Param1, void * Param2);
  intptr_t __fastcall DefaultDialogProc(intptr_t Msg, intptr_t Param1, void * Param2);
  virtual bool __fastcall MouseEvent(MOUSE_EVENT_RECORD * Event);
  virtual bool __fastcall Key(TFarDialogItem * Item, long KeyCode);
  virtual void __fastcall Change();
  virtual void __fastcall Init();
  virtual bool __fastcall CloseQuery();
  UnicodeString __fastcall GetMsg(int MsgId);
  void __fastcall GetNextItemPosition(int & Left, int & Top);
  void __fastcall RefreshBounds();
  virtual void __fastcall Idle();
  void __fastcall BreakSynchronize();
  void __fastcall Synchronize(TThreadMethod Method);
  void __fastcall Close(TFarButton * Button);
  void __fastcall ProcessGroup(int Group, TFarProcessGroupEvent Callback, void * Arg);
  void ShowItem(TFarDialogItem * Item, void * Arg);
  void EnableItem(TFarDialogItem * Item, void * Arg);
  bool __fastcall ChangesLocked();
  TFarDialogItem * __fastcall ItemAt(int X, int Y);

  static intptr_t WINAPI DialogProcGeneral(HANDLE Handle, intptr_t Msg, intptr_t Param1, void * Param2);

  virtual void __fastcall SetBounds(TRect Value);

private:
  TCustomFarPlugin * FFarPlugin;
  TRect FBounds;
  FARDIALOGITEMFLAGS FFlags;
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
typedef TFarDialogContainer self;
public:
  int __fastcall GetLeft() { return FLeft; }
  void __fastcall SetLeft(int Value) { SetPosition(0, Value); }
  int __fastcall GetTop() { return FTop; }
  void __fastcall SetTop(int Value) { SetPosition(1, Value); }
  bool __fastcall GetEnabled() { return FEnabled; }
  void __fastcall SetEnabled(bool Value);
  void __fastcall SetPosition(int Index, int Value);
  int __fastcall GetItemCount();

protected:
  explicit TFarDialogContainer(TFarDialog * ADialog);
  virtual ~TFarDialogContainer();

  TFarDialog * __fastcall GetDialog() { return FDialog; }

  void __fastcall Add(TFarDialogItem * Item);
  void __fastcall Remove(TFarDialogItem * Item);
  virtual void __fastcall Change();
  UnicodeString __fastcall GetMsg(int MsgId);

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
  TRect __fastcall GetBounds() { return FBounds; }
  TRect __fastcall GetActualBounds();
  int __fastcall GetLeft() { return GetCoordinate(0); }
  void __fastcall SetLeft(int Value) { SetCoordinate(0, Value); }
  int __fastcall GetTop() { return GetCoordinate(1); }
  void __fastcall SetTop(int Value) { SetCoordinate(1, Value); }
  int __fastcall GetRight() { return GetCoordinate(2); }
  void __fastcall SetRight(int Value) { SetCoordinate(2, Value); }
  int __fastcall GetBottom() { return GetCoordinate(3); }
  void __fastcall SetBottom(int Value) { SetCoordinate(3, Value); }
  int __fastcall GetWidth();
  void __fastcall SetWidth(int Value);
  int __fastcall GetHeight();
  void __fastcall SetHeight(int Value);
  bool __fastcall GetEnabled() { return FEnabled; }
  void __fastcall SetEnabled(bool Value);
  bool __fastcall GetIsEnabled() { return FIsEnabled; }
  TFarDialogItem * __fastcall GetEnabledFollow() { return FEnabledFollow; }
  void __fastcall SetEnabledFollow(TFarDialogItem * Value);
  TFarDialogItem * __fastcall GetEnabledDependency() { return FEnabledDependency; }
  void __fastcall SetEnabledDependency(TFarDialogItem * Value);
  TFarDialogItem * __fastcall GetEnabledDependencyNegative() { return FEnabledDependencyNegative; }
  void __fastcall SetEnabledDependencyNegative(TFarDialogItem * Value);
  virtual bool __fastcall GetIsEmpty();
  int __fastcall GetGroup() { return FGroup; }
  void __fastcall SetGroup(int Value) { FGroup = Value; }
  bool __fastcall GetVisible() { return GetFlag(DIF_HIDDEN | DIF_INVERSE); }
  void __fastcall SetVisible(bool Value) { SetFlag(DIF_HIDDEN | DIF_INVERSE, Value); }
  bool __fastcall GetTabStop() { return GetFlag(DIF_NOFOCUS | DIF_INVERSE); }
  void __fastcall SetTabStop(bool Value) { SetFlag(DIF_NOFOCUS | DIF_INVERSE, Value); }
  int __fastcall GetTag() { return FTag; }
  void SetTag(int Value) { FTag = Value; }
  TFarDialog * GetDialog() { return FDialog; }

  TNotifyEvent & GetOnExit() { return FOnExit; }
  void SetOnExit(TNotifyEvent Value) { FOnExit = Value; }
  TFarMouseClickEvent & GetOnMouseClick() { return FOnMouseClick; }
  void SetOnMouseClick(TFarMouseClickEvent Value) { FOnMouseClick = Value; }
  bool __fastcall GetFocused();
  void __fastcall SetFocused(bool Value);

  void __fastcall Move(int DeltaX, int DeltaY);
  void __fastcall MoveAt(int X, int Y);
  virtual bool __fastcall CanFocus();
  bool __fastcall Focused();
  void __fastcall SetFocus();
  void SetItem(int Value) { FItem = Value; }

protected:
  FARDIALOGITEMTYPES FDefaultType;
  int FGroup;
  int FTag;
  TNotifyEvent FOnExit;
  TFarMouseClickEvent FOnMouseClick;

  explicit TFarDialogItem(TFarDialog * ADialog, FARDIALOGITEMTYPES AType);
  virtual ~TFarDialogItem();

  FarDialogItem * __fastcall GetDialogItem();
  bool __fastcall GetCenterGroup() { return GetFlag(DIF_CENTERGROUP); }
  void __fastcall SetCenterGroup(bool Value) { SetFlag(DIF_CENTERGROUP, Value); }
  virtual UnicodeString __fastcall GetData();
  virtual void __fastcall SetData(const UnicodeString Value);
  FARDIALOGITEMTYPES __fastcall GetType();
  void __fastcall SetType(FARDIALOGITEMTYPES Value);
  intptr_t __fastcall  GetItem() { return FItem; }
  intptr_t __fastcall GetSelected();
  void __fastcall SetSelected(intptr_t Value);
  TFarDialogContainer * __fastcall GetContainer() { return FContainer; }
  void __fastcall SetContainer(TFarDialogContainer * Value);
  bool __fastcall GetChecked();
  void __fastcall SetChecked(bool Value);
  void __fastcall SetBounds(TRect Value);
  FARDIALOGITEMFLAGS __fastcall GetFlags();
  void __fastcall SetFlags(FARDIALOGITEMFLAGS value);
  void __fastcall UpdateFlags(FARDIALOGITEMFLAGS value);
  int __fastcall GetCoordinate(int Index);
  void __fastcall SetCoordinate(int Index, int Value);
  TFarDialogItem * __fastcall GetPrevItem();
  void __fastcall UpdateFocused(bool Value);
  void __fastcall UpdateEnabled();

  virtual void __fastcall Detach();
  void __fastcall DialogResized();
  intptr_t __fastcall SendMessage(intptr_t Msg, void * Param);
  intptr_t __fastcall SendDialogMessage(intptr_t Msg, intptr_t Param1, void * Param2);
  virtual intptr_t __fastcall ItemProc(intptr_t Msg, void * Param);
  intptr_t __fastcall DefaultItemProc(intptr_t Msg, void * Param);
  intptr_t __fastcall DefaultDialogProc(intptr_t Msg, intptr_t Param1, void * Param2);
  virtual intptr_t __fastcall FailItemProc(intptr_t Msg, void * Param);
  virtual void __fastcall Change();
  void __fastcall DialogChange();
  void __fastcall SetAlterType(FARDIALOGITEMTYPES Index, bool Value);
  bool __fastcall GetAlterType(FARDIALOGITEMTYPES Index);
  virtual void __fastcall UpdateBounds();
  virtual void __fastcall ResetBounds();
  virtual void __fastcall Init();
  virtual bool __fastcall CloseQuery();
  virtual bool MouseMove(int X, int Y, MOUSE_EVENT_RECORD * Event);
  virtual bool MouseClick(MOUSE_EVENT_RECORD * Event);
  TPoint __fastcall MouseClientPosition(MOUSE_EVENT_RECORD * Event);
  void __fastcall Text(int X, int Y, const FarColor & Color, const UnicodeString Str);
  void __fastcall Redraw();
  virtual bool __fastcall HotKey(char HotKey);

public:
  virtual void __fastcall SetDataInternal(const UnicodeString Value);
  void __fastcall UpdateData(const UnicodeString Value);
  void __fastcall UpdateSelected(intptr_t Value);

  bool __fastcall GetFlag(FARDIALOGITEMFLAGS Index);
  void __fastcall SetFlag(FARDIALOGITEMFLAGS Index, bool Value);

  virtual void __fastcall DoFocus();
  virtual void __fastcall DoExit();

  char __fastcall GetColor(int Index);
  void __fastcall SetColor(int Index, char Value);

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

  virtual UnicodeString __fastcall GetCaption() { return GetData(); }
  virtual void __fastcall SetCaption(const UnicodeString Value) { SetData(Value); }
  virtual int __fastcall GetResult() { return FResult; }
  virtual void __fastcall SetResult(int Value) { FResult = Value; }
  virtual UnicodeString __fastcall GetData();
  bool __fastcall GetDefault();
  void __fastcall SetDefault(bool Value);
  TFarButtonBrackets GetBrackets() { return FBrackets; }
  void __fastcall SetBrackets(TFarButtonBrackets Value);
  bool __fastcall GetCenterGroup() { return TFarDialogItem::GetCenterGroup(); }
  void __fastcall SetCenterGroup(bool Value) { TFarDialogItem::SetCenterGroup(Value); }
  virtual TFarButtonClickEvent & GetOnClick() { return FOnClick; }
  virtual void SetOnClick(TFarButtonClickEvent Value) { FOnClick = Value; }

protected:
  virtual void __fastcall SetDataInternal(const UnicodeString Value);
  virtual intptr_t __fastcall ItemProc(intptr_t Msg, void * Param);
  virtual bool __fastcall HotKey(char HotKey);

private:
  int FResult;
  TFarButtonClickEvent FOnClick;
  TFarButtonBrackets FBrackets;
};
//---------------------------------------------------------------------------
DEFINE_CALLBACK_TYPE3(TFarAllowChangeEvent, void,
  TFarDialogItem * /* Sender */, void * /* NewState */, bool & /* AllowChange */);
//---------------------------------------------------------------------------
class TFarCheckBox : public TFarDialogItem
{
public:
  explicit TFarCheckBox(TFarDialog * ADialog);

  virtual UnicodeString __fastcall GetCaption() { return GetData(); }
  virtual void __fastcall SetCaption(const UnicodeString Value) { SetData(Value); }
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
  virtual intptr_t __fastcall ItemProc(intptr_t Msg, void * Param);
  virtual bool __fastcall GetIsEmpty();
  virtual void __fastcall SetData(const UnicodeString Value);
};
//---------------------------------------------------------------------------
class TFarRadioButton : public TFarDialogItem
{
public:
  explicit TFarRadioButton(TFarDialog * ADialog);

  bool __fastcall GetChecked() { return TFarDialogItem::GetChecked(); }
  void __fastcall SetChecked(bool Value) { TFarDialogItem::SetChecked(Value); }
  virtual UnicodeString __fastcall GetCaption() { return GetData(); }
  virtual void __fastcall SetCaption(const UnicodeString Value) { SetData(Value); }
  virtual TFarAllowChangeEvent & GetOnAllowChange() { return FOnAllowChange; }
  virtual void SetOnAllowChange(TFarAllowChangeEvent Value) { FOnAllowChange = Value; }

protected:
  TFarAllowChangeEvent FOnAllowChange;
  virtual intptr_t __fastcall ItemProc(intptr_t Msg, void * Param);
  virtual bool __fastcall GetIsEmpty();
  virtual void __fastcall SetData(const UnicodeString Value);
};
//---------------------------------------------------------------------------
class TFarEdit : public TFarDialogItem
{
public:
  explicit TFarEdit(TFarDialog * ADialog);

  virtual UnicodeString __fastcall GetText() { return GetData(); }
  virtual void __fastcall SetText(const UnicodeString Value) { SetData(Value); }
  int __fastcall GetAsInteger();
  void __fastcall SetAsInteger(int Value);
  virtual bool __fastcall GetPassword() { return GetAlterType(DI_PSWEDIT); }
  virtual void __fastcall SetPassword(bool Value) { SetAlterType(DI_PSWEDIT, Value); }
  virtual bool __fastcall GetFixed() { return GetAlterType(DI_FIXEDIT); }
  virtual void __fastcall SetFixed(bool Value) { SetAlterType(DI_FIXEDIT, Value); }
  virtual UnicodeString __fastcall GetMask() { return GetHistoryMask(1); }
  virtual void __fastcall SetMask(const UnicodeString Value) { SetHistoryMask(1, Value); }
  virtual UnicodeString __fastcall GetHistory() { return GetHistoryMask(0); }
  virtual void __fastcall SetHistory(const UnicodeString Value) { SetHistoryMask(0, Value); }
  bool __fastcall GetExpandEnvVars() { return GetFlag(DIF_EDITEXPAND); }
  void __fastcall SetExpandEnvVars(bool Value) { SetFlag(DIF_EDITEXPAND, Value); }
  bool __fastcall GetAutoSelect() { return GetFlag(DIF_SELECTONENTRY); }
  void __fastcall SetAutoSelect(bool Value) { SetFlag(DIF_SELECTONENTRY, Value); }
  bool __fastcall GetReadOnly() { return GetFlag(DIF_READONLY); }
  void __fastcall SetReadOnly(bool Value) { SetFlag(DIF_READONLY, Value); }

protected:
  virtual intptr_t __fastcall ItemProc(intptr_t Msg, void * Param);
  virtual void __fastcall Detach();

private:
  UnicodeString __fastcall GetHistoryMask(size_t Index);
  void __fastcall SetHistoryMask(size_t Index, const UnicodeString Value);
};
//---------------------------------------------------------------------------
class TFarSeparator : public TFarDialogItem
{
public:
  explicit TFarSeparator(TFarDialog * ADialog);

  bool __fastcall GetDouble();
  void __fastcall SetDouble(bool Value);
  virtual UnicodeString __fastcall GetCaption() { return GetData(); }
  virtual void __fastcall SetCaption(const UnicodeString Value) { SetData(Value); }
  void __fastcall SetPosition(int Value);
  int __fastcall GetPosition();

protected:
  virtual void __fastcall ResetBounds();
};
//---------------------------------------------------------------------------
class TFarText : public TFarDialogItem
{
public:
  explicit  TFarText(TFarDialog * ADialog);

  virtual UnicodeString __fastcall GetCaption() { return GetData(); }
  virtual void __fastcall SetCaption(const UnicodeString Value) { SetData(Value); }
  bool __fastcall GetCenterGroup() { return TFarDialogItem::GetCenterGroup(); }
  void __fastcall SetCenterGroup(bool Value) { TFarDialogItem::SetCenterGroup(Value); }
  char GetColor() { return TFarDialogItem::GetColor(0); }
  void SetColor(char Value) { TFarDialogItem::SetColor(0, Value); }

protected:
  virtual void __fastcall SetData(const UnicodeString Value);
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

  intptr_t __fastcall GetSelected();
  void __fastcall SetSelected(intptr_t Value);
  intptr_t __fastcall GetTopIndex();
  void __fastcall SetTopIndex(intptr_t Value);
  inline intptr_t __fastcall GetSelectedInt(bool Init);
  bool __fastcall GetFlag(intptr_t Index, LISTITEMFLAGS Flag);
  void __fastcall SetFlag(intptr_t Index, LISTITEMFLAGS Flag, bool Value);
  LISTITEMFLAGS __fastcall GetFlags(intptr_t Index);
  void __fastcall SetFlags(intptr_t Index, LISTITEMFLAGS Value);
  intptr_t __fastcall GetMaxLength();
  intptr_t __fastcall GetVisibleCount();
  bool GetDisabled(intptr_t Index) { return GetFlag(Index, LIF_DISABLE); }
  void SetDisabled(intptr_t Index, bool Value) { SetFlag(Index, LIF_DISABLE, Value); }
  bool GetChecked(intptr_t Index) { return GetFlag(Index, LIF_CHECKED); }
  void SetChecked(intptr_t Index, bool Value) { SetFlag(Index, LIF_CHECKED, Value); }

protected:
  virtual void __fastcall Changed();
  virtual intptr_t __fastcall ItemProc(intptr_t Msg, void * Param);
  virtual void __fastcall Init();
  void __fastcall UpdatePosition(intptr_t Position);
  intptr_t __fastcall GetPosition();
  virtual void __fastcall Put(intptr_t Index, const UnicodeString S);
  void __fastcall SetCurPos(intptr_t Position, intptr_t TopIndex);
  void __fastcall UpdateItem(intptr_t Index);

  FarList * __fastcall GetListItems() { return FListItems; }
  TFarDialogItem * __fastcall GetDialogItem() { return FDialogItem; }

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
  typedef TFarListBox self;
public:
  explicit TFarListBox(TFarDialog * ADialog);
  virtual ~TFarListBox();

  void __fastcall SetItems(TStrings * Value);

  bool __fastcall GetNoAmpersand() { return GetFlag(DIF_LISTNOAMPERSAND); }
  void __fastcall SetNoAmpersand(bool Value) { SetFlag(DIF_LISTNOAMPERSAND, Value); }
  bool __fastcall GetAutoHighlight() { return GetFlag(DIF_LISTAUTOHIGHLIGHT); }
  void __fastcall SetAutoHighlight(bool Value) { SetFlag(DIF_LISTAUTOHIGHLIGHT, Value); }
  bool __fastcall GetNoBox() { return GetFlag(DIF_LISTNOBOX); }
  void __fastcall SetNoBox(bool Value) { SetFlag(DIF_LISTNOBOX, Value); }
  bool __fastcall GetWrapMode() { return GetFlag(DIF_LISTWRAPMODE); }
  void __fastcall SetWrapMode(bool Value) { SetFlag(DIF_LISTWRAPMODE, Value); }
  TFarList * __fastcall GetItems() { return FList; }
  void __fastcall SetList(TFarList * Value);
  TFarListBoxAutoSelect GetAutoSelect() { return FAutoSelect; }
  void __fastcall SetAutoSelect(TFarListBoxAutoSelect Value);

protected:
  virtual intptr_t __fastcall ItemProc(intptr_t Msg, void * Param);
  virtual void __fastcall Init();
  virtual bool __fastcall CloseQuery();

private:
  TFarList * FList;
  TFarListBoxAutoSelect FAutoSelect;
  bool FDenyClose;

  void __fastcall UpdateMouseReaction();
};
//---------------------------------------------------------------------------
class TFarComboBox : public TFarDialogItem
{
  typedef TFarComboBox self;
public:
  explicit TFarComboBox(TFarDialog * ADialog);
  virtual ~TFarComboBox();

  void __fastcall ResizeToFitContent();

  bool __fastcall GetNoAmpersand() { return GetFlag(DIF_LISTNOAMPERSAND); }
  void __fastcall SetNoAmpersand(bool Value) { SetFlag(DIF_LISTNOAMPERSAND, Value); }
  bool __fastcall GetAutoHighlight() { return GetFlag(DIF_LISTAUTOHIGHLIGHT); }
  void __fastcall SetAutoHighlight(bool Value) { SetFlag(DIF_LISTAUTOHIGHLIGHT, Value); }
  bool __fastcall GetWrapMode() { return GetFlag(DIF_LISTWRAPMODE); }
  void __fastcall SetWrapMode(bool Value) { SetFlag(DIF_LISTWRAPMODE, Value); }
  TFarList * __fastcall GetItems() { return FList; }
  virtual UnicodeString __fastcall GetText() { return GetData(); }
  virtual void __fastcall SetText(const UnicodeString Value) { SetData(Value); }
  bool __fastcall GetAutoSelect() { return GetFlag(DIF_SELECTONENTRY); }
  void __fastcall SetAutoSelect(bool Value) { SetFlag(DIF_SELECTONENTRY, Value); }
  bool __fastcall GetDropDownList() { return GetFlag(DIF_DROPDOWNLIST); }
  void __fastcall SetDropDownList(bool Value) { SetFlag(DIF_DROPDOWNLIST, Value); }
  intptr_t __fastcall GetItemIndex() const { return FList->GetSelected(); }
  void __fastcall SetItemIndex(intptr_t Index) { FList->SetSelected(Index); }

protected:
  virtual intptr_t __fastcall ItemProc(intptr_t Msg, void * Param);
  virtual void __fastcall Init();

private:
  TFarList * FList;
};
//---------------------------------------------------------------------------
class TFarLister : public TFarDialogItem
{
  typedef TFarLister self;
public:
  explicit TFarLister(TFarDialog * ADialog);
  virtual ~TFarLister();

  TStrings * __fastcall GetItems();
  void __fastcall SetItems(TStrings * Value);
  int GetTopIndex() { return FTopIndex; }
  void __fastcall SetTopIndex(int Value);
  bool __fastcall GetScrollBar();

protected:
  virtual intptr_t __fastcall ItemProc(intptr_t Msg, void * Param);
  virtual void __fastcall DoFocus();

private:
  TStringList * FItems;
  int FTopIndex;

  void ItemsChange(TObject * Sender);
};
//---------------------------------------------------------------------------
UnicodeString __fastcall StripHotKey(const UnicodeString Text);
TRect __fastcall Rect(int Left, int Top, int Right, int Bottom);
//---------------------------------------------------------------------------
#endif
