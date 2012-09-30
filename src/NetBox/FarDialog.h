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
  explicit /* __fastcall */ TFarDialog(TCustomFarPlugin * AFarPlugin);
  virtual /* __fastcall */ ~TFarDialog();

  int __fastcall ShowModal();
  void __fastcall ShowGroup(int Group, bool Show);
  void __fastcall EnableGroup(int Group, bool Enable);

  TRect GetBounds() { return FBounds; }
  FARDIALOGITEMFLAGS __fastcall GetFlags() const { return FFlags; }
  void __fastcall SetFlags(const FARDIALOGITEMFLAGS value);
  TRect __fastcall GetClientRect();
  UnicodeString GetHelpTopic() { return FHelpTopic; }
  void __fastcall SetHelpTopic(UnicodeString value);
  bool __fastcall GetCentered();
  void __fastcall SetCentered(bool value);
  TPoint __fastcall GetSize();
  void __fastcall SetSize(TPoint value);
  TPoint __fastcall GetClientSize();
  int __fastcall GetWidth();
  void __fastcall SetWidth(int value);
  int __fastcall GetHeight();
  void __fastcall SetHeight(int value);
  UnicodeString __fastcall GetCaption();
  void __fastcall SetCaption(UnicodeString value);
  HANDLE __fastcall GetHandle() { return FHandle; }
  TFarButton * __fastcall GetDefaultButton() const { return FDefaultButton; }
  TFarBox * __fastcall GetBorderBox() const { return FBorderBox; }
  TFarDialogItem * __fastcall GetItem(int Index);
  int __fastcall GetItemCount();
  TItemPosition __fastcall GetNextItemPosition() { return FNextItemPosition; }
  void __fastcall SetNextItemPosition(const TItemPosition & value) { FNextItemPosition = value; }
  int __fastcall GetDefaultGroup() const { return FDefaultGroup; }
  void __fastcall SetDefaultGroup(const int & value) { FDefaultGroup = value; }
  size_t GetTag() const { return FTag; }
  void SetTag(int value) { FTag = value; }
  TFarDialogItem * __fastcall GetItemFocused() { return FItemFocused; }
  void __fastcall SetItemFocused(TFarDialogItem * value);
  int __fastcall GetResult() { return FResult; }
  TPoint __fastcall GetMaxSize();

  TFarKeyEvent & GetOnKey() { return FOnKey; }
  void SetOnKey(TFarKeyEvent Value) { FOnKey = Value; }

  void __fastcall Redraw();
  void __fastcall LockChanges();
  void __fastcall UnlockChanges();
  FarColor __fastcall GetSystemColor(PaletteColors colorId);
  bool __fastcall HotKey(WORD Key, DWORD ControlState);


protected:
  TCustomFarPlugin * __fastcall GetFarPlugin() { return FFarPlugin; }
  TObjectList * __fastcall GetItems() { return FItems; }
  void __fastcall Add(TFarDialogItem * Item);
  void __fastcall Add(TFarDialogContainer * Container);
  intptr_t __fastcall SendMessage(int Msg, int Param1, void * Param2);
  virtual intptr_t __fastcall DialogProc(int Msg, int Param1, void * Param2);
  virtual intptr_t __fastcall FailDialogProc(int Msg, int Param1, void * Param2);
  intptr_t __fastcall DefaultDialogProc(int Msg, int Param1, void * Param2);
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
  void /* __fastcall */ ShowItem(TFarDialogItem * Item, void * Arg);
  void /* __fastcall */ EnableItem(TFarDialogItem * Item, void * Arg);
  bool __fastcall ChangesLocked();
  TFarDialogItem * __fastcall ItemAt(int X, int Y);

  static INT_PTR WINAPI DialogProcGeneral(HANDLE Handle, int Msg, int Param1, void * Param2);

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
  size_t FTag;
  TFarDialogItem * FItemFocused;
  TFarKeyEvent FOnKey;
  FarDialogItem * FDialogItems;
  int FDialogItemsCapacity;
  int FChangesLocked;
  bool FChangesPending;
  int FResult;
  bool FNeedsSynchronize;
  HANDLE FSynchronizeObjects[2];
  TThreadMethod FSynchronizeMethod;
  TFarDialog * Self;

public:
  void __fastcall SetBounds(TRect value);
};
//---------------------------------------------------------------------------
class TFarDialogContainer : public TObject
{
friend TFarDialog;
friend TFarDialogItem;
typedef TFarDialogContainer self;
public:
  int __fastcall GetLeft() { return FLeft; }
  void __fastcall SetLeft(int value) { SetPosition(0, value); }
  int __fastcall GetTop() { return FTop; }
  void __fastcall SetTop(int value) { SetPosition(1, value); }
  bool __fastcall GetEnabled() { return FEnabled; }
  void __fastcall SetEnabled(bool value);
  void __fastcall SetPosition(int Index, int value);
  int __fastcall GetItemCount();

protected:
  explicit /* __fastcall */ TFarDialogContainer(TFarDialog * ADialog);
  virtual /* __fastcall */ ~TFarDialogContainer();

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
  void __fastcall SetLeft(int value) { SetCoordinate(0, value); }
  int __fastcall GetTop() { return GetCoordinate(1); }
  void __fastcall SetTop(int value) { SetCoordinate(1, value); }
  int __fastcall GetRight() { return GetCoordinate(2); }
  void __fastcall SetRight(int value) { SetCoordinate(2, value); }
  int __fastcall GetBottom() { return GetCoordinate(3); }
  void __fastcall SetBottom(int value) { SetCoordinate(3, value); }
  int __fastcall GetWidth();
  void __fastcall SetWidth(int value);
  int __fastcall GetHeight();
  void __fastcall SetHeight(int value);
  void __fastcall SetFlags(unsigned int value);
  bool __fastcall GetEnabled() { return FEnabled; }
  void __fastcall SetEnabled(bool value);
  bool __fastcall GetIsEnabled() { return FIsEnabled; }
  TFarDialogItem * __fastcall GetEnabledFollow() { return FEnabledFollow; }
  void __fastcall SetEnabledFollow(TFarDialogItem * value);
  TFarDialogItem * __fastcall GetEnabledDependency() { return FEnabledDependency; }
  void __fastcall SetEnabledDependency(TFarDialogItem * value);
  TFarDialogItem * __fastcall GetEnabledDependencyNegative() { return FEnabledDependencyNegative; }
  void __fastcall SetEnabledDependencyNegative(TFarDialogItem * value);
  virtual bool __fastcall GetIsEmpty();
  int __fastcall GetGroup() { return FGroup; }
  void __fastcall SetGroup(int value) { FGroup = value; }
  bool __fastcall GetVisible() { return GetFlag(DIF_HIDDEN | DIF_INVERSE); }
  void __fastcall SetVisible(bool value) { SetFlag(DIF_HIDDEN | DIF_INVERSE, value); }
  bool __fastcall GetTabStop() { return GetFlag(DIF_NOFOCUS | DIF_INVERSE); }
  void __fastcall SetTabStop(bool value) { SetFlag(DIF_NOFOCUS | DIF_INVERSE, value); }
  int __fastcall GetTag() { return FTag; }
  void SetTag(int value) { FTag = value; }
  TFarDialog * GetDialog() { return FDialog; }

  TNotifyEvent & GetOnExit() { return FOnExit; }
  void SetOnExit(TNotifyEvent Value) { FOnExit = Value; }
  TFarMouseClickEvent & GetOnMouseClick() { return FOnMouseClick; }
  void SetOnMouseClick(TFarMouseClickEvent value) { FOnMouseClick = value; }
  bool __fastcall GetFocused();
  void __fastcall SetFocused(bool Value);

  void __fastcall Move(int DeltaX, int DeltaY);
  void __fastcall MoveAt(int X, int Y);
  virtual bool __fastcall CanFocus();
  bool __fastcall Focused();
  void __fastcall SetFocus();
  void SetItem(int value) { FItem = value; }

protected:
  FARDIALOGITEMTYPES FDefaultType;
  int FGroup;
  int FTag;
  TNotifyEvent FOnExit;
  TFarMouseClickEvent FOnMouseClick;

  explicit /* __fastcall */ TFarDialogItem(TFarDialog *ADialog, FARDIALOGITEMTYPES AType);
  virtual /* __fastcall */ ~TFarDialogItem();

  FarDialogItem * __fastcall GetDialogItem();
  bool __fastcall GetCenterGroup() { return GetFlag(DIF_CENTERGROUP); }
  void __fastcall SetCenterGroup(bool value) { SetFlag(DIF_CENTERGROUP, value); }
  virtual UnicodeString __fastcall GetData();
  virtual void __fastcall SetData(const UnicodeString value);
  FARDIALOGITEMTYPES __fastcall GetType();
  void __fastcall SetType(FARDIALOGITEMTYPES value);
  int __fastcall  GetItem() { return  FItem; }
  int __fastcall GetSelected();
  void __fastcall SetSelected(int value);
  TFarDialogContainer * __fastcall GetContainer() { return FContainer; }
  void __fastcall SetContainer(TFarDialogContainer * value);
  bool __fastcall GetChecked();
  void __fastcall SetChecked(bool value);
  void __fastcall SetBounds(TRect value);
  FARDIALOGITEMFLAGS __fastcall GetFlags();
  void __fastcall SetFlags(FARDIALOGITEMFLAGS value);
  void __fastcall UpdateFlags(FARDIALOGITEMFLAGS value);
  int __fastcall GetCoordinate(int Index);
  void __fastcall SetCoordinate(int Index, int value);
  TFarDialogItem * __fastcall GetPrevItem();
  void __fastcall UpdateFocused(bool value);
  void __fastcall UpdateEnabled();

  virtual void __fastcall Detach();
  void __fastcall DialogResized();
  intptr_t __fastcall SendMessage(int Msg, void * Param);
  intptr_t __fastcall SendDialogMessage(int Msg, int Param1, void * Param2);
  virtual intptr_t __fastcall ItemProc(int Msg, void * Param);
  intptr_t __fastcall DefaultItemProc(int Msg, void * Param);
  intptr_t __fastcall DefaultDialogProc(int Msg, int Param1, void * Param2);
  virtual intptr_t __fastcall FailItemProc(int Msg, void * Param);
  virtual void __fastcall Change();
  void __fastcall DialogChange();
  void __fastcall SetAlterType(FARDIALOGITEMTYPES Index, bool value);
  bool __fastcall GetAlterType(FARDIALOGITEMTYPES Index);
  virtual void __fastcall UpdateBounds();
  virtual void __fastcall ResetBounds();
  virtual void __fastcall Init();
  virtual bool __fastcall CloseQuery();
  virtual bool /* __fastcall */ MouseMove(int X, int Y, MOUSE_EVENT_RECORD * Event);
  virtual bool /* __fastcall */ MouseClick(MOUSE_EVENT_RECORD * Event);
  TPoint __fastcall MouseClientPosition(MOUSE_EVENT_RECORD * Event);
  void __fastcall Text(int X, int Y, const FarColor & Color, const UnicodeString Str);
  void __fastcall Redraw();
  virtual bool __fastcall HotKey(char HotKey);

public:
  virtual void __fastcall SetDataInternal(const UnicodeString value);
  void __fastcall UpdateData(const UnicodeString value);
  void __fastcall UpdateSelected(int Param);

  bool __fastcall GetFlag(FARDIALOGITEMFLAGS Index);
  void __fastcall SetFlag(FARDIALOGITEMFLAGS Index, bool value);

  virtual void __fastcall DoFocus();
  virtual void __fastcall DoExit();

  char __fastcall GetColor(int Index);
  void __fastcall SetColor(int Index, char value);

private:
  TFarDialog * FDialog;
  TRect FBounds;
  TFarDialogItem * FEnabledFollow;
  TFarDialogItem * FEnabledDependency;
  TFarDialogItem * FEnabledDependencyNegative;
  TFarDialogContainer * FContainer;
  int FItem;
  bool FEnabled;
  bool FIsEnabled;
  unsigned long FColors;
  unsigned long FColorMask;
};
//---------------------------------------------------------------------------
class TFarBox : public TFarDialogItem
{
public:
  explicit /* __fastcall */ TFarBox(TFarDialog * ADialog);

  virtual UnicodeString GetCaption() { return GetData(); }
  virtual void SetCaption(const UnicodeString value) { SetData(value); }
  virtual bool GetDouble() { return GetAlterType(DI_DOUBLEBOX); }
  virtual void SetDouble(bool value) { SetAlterType(DI_DOUBLEBOX, value); }
};
//---------------------------------------------------------------------------
DEFINE_CALLBACK_TYPE2(TFarButtonClickEvent, void,
  TFarButton * /* Sender */, bool & /* Close */);
enum TFarButtonBrackets { brNone, brTight, brSpace, brNormal };
//---------------------------------------------------------------------------
class TFarButton : public TFarDialogItem
{
public:
  explicit /* __fastcall */ TFarButton(TFarDialog * ADialog);

  virtual UnicodeString __fastcall GetCaption() { return GetData(); }
  virtual void __fastcall SetCaption(const UnicodeString value) { SetData(value); }
  virtual int __fastcall GetResult() { return FResult; }
  virtual void __fastcall SetResult(int value) { FResult = value; }
  virtual UnicodeString __fastcall GetData();
  void __fastcall SetDefault(bool value);
  TFarButtonBrackets GetBrackets() { return FBrackets; }
  void __fastcall SetBrackets(TFarButtonBrackets value);
  bool __fastcall GetCenterGroup() { return TFarDialogItem::GetCenterGroup(); }
  void __fastcall SetCenterGroup(bool value) { TFarDialogItem::SetCenterGroup(value); }
  virtual TFarButtonClickEvent & GetOnClick() { return FOnClick; }
  virtual void SetOnClick(TFarButtonClickEvent value) { FOnClick = value; }

protected:
  virtual void __fastcall SetDataInternal(const UnicodeString value);
  virtual intptr_t __fastcall ItemProc(int Msg, void * Param);
  virtual bool __fastcall HotKey(char HotKey);

private:
  int FResult;
  TFarButtonClickEvent FOnClick;
  TFarButtonBrackets FBrackets;

public:
  bool __fastcall GetDefault();
};
//---------------------------------------------------------------------------
DEFINE_CALLBACK_TYPE3(TFarAllowChangeEvent, void,
  TFarDialogItem * /* Sender */, void * /* NewState */, bool & /* AllowChange */);
//---------------------------------------------------------------------------
class TFarCheckBox : public TFarDialogItem
{
public:
  explicit /* __fastcall */ TFarCheckBox(TFarDialog * ADialog);

  virtual UnicodeString __fastcall GetCaption() { return GetData(); }
  virtual void __fastcall SetCaption(const UnicodeString value) { SetData(value); }
  bool GetAllowGrayed() { return GetFlag(DIF_3STATE); }
  void SetAllowGrayed(bool value) { SetFlag(DIF_3STATE, value); }
  virtual TFarAllowChangeEvent & GetOnAllowChange() { return FOnAllowChange; }
  virtual void SetOnAllowChange(TFarAllowChangeEvent value) { FOnAllowChange = value; }
  bool GetChecked() { return TFarDialogItem::GetChecked(); }
  void SetChecked(bool value) { TFarDialogItem::SetChecked(value); }
  int GetSelected() { return TFarDialogItem::GetSelected(); }
  void SetSelected(size_t value) { TFarDialogItem::SetSelected(value); }

protected:
  TFarAllowChangeEvent FOnAllowChange;
  virtual intptr_t __fastcall ItemProc(int Msg, void * Param);
  virtual bool __fastcall GetIsEmpty();
  virtual void __fastcall SetData(const UnicodeString value);
};
//---------------------------------------------------------------------------
class TFarRadioButton : public TFarDialogItem
{
public:
  explicit /* __fastcall */ TFarRadioButton(TFarDialog * ADialog);

  bool __fastcall GetChecked() { return TFarDialogItem::GetChecked(); }
  void __fastcall SetChecked(bool value) { TFarDialogItem::SetChecked(value); }
  virtual UnicodeString __fastcall GetCaption() { return GetData(); }
  virtual void __fastcall SetCaption(const UnicodeString value) { SetData(value); }
  virtual TFarAllowChangeEvent & GetOnAllowChange() { return FOnAllowChange; }
  virtual void SetOnAllowChange(TFarAllowChangeEvent value) { FOnAllowChange = value; }

protected:
  TFarAllowChangeEvent FOnAllowChange;
  virtual intptr_t __fastcall ItemProc(int Msg, void * Param);
  virtual bool __fastcall GetIsEmpty();
  virtual void __fastcall SetData(const UnicodeString value);
};
//---------------------------------------------------------------------------
class TFarEdit : public TFarDialogItem
{
public:
  explicit /* __fastcall */ TFarEdit(TFarDialog * ADialog);

  virtual UnicodeString __fastcall GetText() { return GetData(); }
  virtual void __fastcall SetText(const UnicodeString value) { SetData(value); }
  int __fastcall GetAsInteger();
  void __fastcall SetAsInteger(int value);
  virtual bool __fastcall GetPassword() { return GetAlterType(DI_PSWEDIT); }
  virtual void __fastcall SetPassword(bool value) { SetAlterType(DI_PSWEDIT, value); }
  virtual bool __fastcall GetFixed() { return GetAlterType(DI_FIXEDIT); }
  virtual void __fastcall SetFixed(bool value) { SetAlterType(DI_FIXEDIT, value); }
  virtual UnicodeString __fastcall GetMask() { return GetHistoryMask(1); }
  virtual void __fastcall SetMask(const UnicodeString value) { SetHistoryMask(1, value); }
  virtual UnicodeString __fastcall GetHistory() { return GetHistoryMask(0); }
  virtual void __fastcall SetHistory(const UnicodeString value) { SetHistoryMask(0, value); }
  bool __fastcall GetExpandEnvVars() { return GetFlag(DIF_EDITEXPAND); }
  void __fastcall SetExpandEnvVars(bool value) { SetFlag(DIF_EDITEXPAND, value); }
  bool __fastcall GetAutoSelect() { return GetFlag(DIF_SELECTONENTRY); }
  void __fastcall SetAutoSelect(bool value) { SetFlag(DIF_SELECTONENTRY, value); }
  bool __fastcall GetReadOnly() { return GetFlag(DIF_READONLY); }
  void __fastcall SetReadOnly(bool value) { SetFlag(DIF_READONLY, value); }

protected:
  virtual intptr_t __fastcall ItemProc(int Msg, void * Param);
  virtual void __fastcall Detach();

private:
  UnicodeString __fastcall GetHistoryMask(size_t Index);
  void __fastcall SetHistoryMask(size_t Index, const UnicodeString value);
};
//---------------------------------------------------------------------------
class TFarSeparator : public TFarDialogItem
{
public:
  explicit /* __fastcall */ TFarSeparator(TFarDialog * ADialog);

  bool __fastcall GetDouble();
  void __fastcall SetDouble(bool value);
  virtual UnicodeString __fastcall GetCaption() { return GetData(); }
  virtual void __fastcall SetCaption(const UnicodeString value) { SetData(value); }
  void __fastcall SetPosition(int value);
  int __fastcall GetPosition();

protected:
  virtual void __fastcall ResetBounds();
};
//---------------------------------------------------------------------------
class TFarText : public TFarDialogItem
{
public:
  explicit  /* __fastcall */ TFarText(TFarDialog * ADialog);

  virtual UnicodeString __fastcall GetCaption() { return GetData(); }
  virtual void __fastcall SetCaption(const UnicodeString value) { SetData(value); }
  bool __fastcall GetCenterGroup() { return TFarDialogItem::GetCenterGroup(); }
  void __fastcall SetCenterGroup(bool value) { TFarDialogItem::SetCenterGroup(value); }
  char GetColor() { return TFarDialogItem::GetColor(0); }
  void SetColor(char value) { TFarDialogItem::SetColor(0, value); }

protected:
  virtual void __fastcall SetData(const UnicodeString value);
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
  explicit /* __fastcall */ TFarList(TFarDialogItem * ADialogItem = NULL);
  virtual /* __fastcall */ ~TFarList();

  virtual void __fastcall Assign(TPersistent * Source);

  int __fastcall GetSelected();
  void __fastcall SetSelected(int value);
  int __fastcall GetTopIndex();
  void __fastcall SetTopIndex(int value);
  inline int __fastcall GetSelectedInt(bool Init);
  FARDIALOGITEMFLAGS __fastcall GetFlags(int Index);
  void __fastcall SetFlags(int Index, FARDIALOGITEMFLAGS value);
  int __fastcall GetMaxLength();
  int __fastcall GetVisibleCount();
  bool __fastcall GetDisabled(int Index) { return GetFlag(Index, LIF_DISABLE); }
  void __fastcall SetDisabled(int Index, bool value) { SetFlag(Index, LIF_DISABLE, value); }
  bool __fastcall GetChecked(int Index) { return GetFlag(Index, LIF_CHECKED); }
  void __fastcall SetChecked(int Index, bool value) { SetFlag(Index, LIF_CHECKED, value); }

protected:
  virtual void __fastcall Changed();
  virtual intptr_t __fastcall ItemProc(int Msg, void * Param);
  virtual void __fastcall Init();
  void __fastcall UpdatePosition(int Position);
  int __fastcall GetPosition();
  virtual void __fastcall Put(int Index, const UnicodeString S);
  void __fastcall SetCurPos(int Position, int TopIndex);
  void __fastcall UpdateItem(int Index);

  FarList * __fastcall GetListItems() { return FListItems; }
  TFarDialogItem * __fastcall GetDialogItem() { return FDialogItem; }

private:
  FarList * FListItems;
  TFarDialogItem * FDialogItem;
  bool FNoDialogUpdate;
  TFarList * Self;
  bool __fastcall GetFlag(int Index, FARDIALOGITEMFLAGS Flag);
  void __fastcall SetFlag(int Index, FARDIALOGITEMFLAGS Flag, bool value);
};
//---------------------------------------------------------------------------
enum TFarListBoxAutoSelect { asOnlyFocus, asAlways, asNever };
//---------------------------------------------------------------------------
class TFarListBox : public TFarDialogItem
{
  typedef TFarListBox self;
public:
  explicit /* __fastcall */ TFarListBox(TFarDialog * ADialog);
  virtual /* __fastcall */ ~TFarListBox();

  void __fastcall SetItems(TStrings * value);

  bool __fastcall GetNoAmpersand() { return GetFlag(DIF_LISTNOAMPERSAND); }
  void __fastcall SetNoAmpersand(bool value) { SetFlag(DIF_LISTNOAMPERSAND, value); }
  bool __fastcall GetAutoHighlight() { return GetFlag(DIF_LISTAUTOHIGHLIGHT); }
  void __fastcall SetAutoHighlight(bool value) { SetFlag(DIF_LISTAUTOHIGHLIGHT, value); }
  bool __fastcall GetNoBox() { return GetFlag(DIF_LISTNOBOX); }
  void __fastcall SetNoBox(bool value) { SetFlag(DIF_LISTNOBOX, value); }
  bool __fastcall GetWrapMode() { return GetFlag(DIF_LISTWRAPMODE); }
  void __fastcall SetWrapMode(bool value) { SetFlag(DIF_LISTWRAPMODE, value); }
  TFarList * __fastcall GetItems() { return FList; }
  void __fastcall SetList(TFarList * value);
  TFarListBoxAutoSelect GetAutoSelect() { return FAutoSelect; }
  void __fastcall SetAutoSelect(TFarListBoxAutoSelect value);

protected:
  virtual intptr_t __fastcall ItemProc(int Msg, void * Param);
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
  explicit /* __fastcall */ TFarComboBox(TFarDialog * ADialog);
  virtual /* __fastcall */ ~TFarComboBox();

  void __fastcall ResizeToFitContent();

  bool __fastcall GetNoAmpersand() { return GetFlag(DIF_LISTNOAMPERSAND); }
  void __fastcall SetNoAmpersand(bool value) { SetFlag(DIF_LISTNOAMPERSAND, value); }
  bool __fastcall GetAutoHighlight() { return GetFlag(DIF_LISTAUTOHIGHLIGHT); }
  void __fastcall SetAutoHighlight(bool value) { SetFlag(DIF_LISTAUTOHIGHLIGHT, value); }
  bool __fastcall GetWrapMode() { return GetFlag(DIF_LISTWRAPMODE); }
  void __fastcall SetWrapMode(bool value) { SetFlag(DIF_LISTWRAPMODE, value); }
  TFarList * __fastcall GetItems() { return FList; }
  virtual UnicodeString __fastcall GetText() { return GetData(); }
  virtual void __fastcall SetText(const UnicodeString value) { SetData(value); }
  bool __fastcall GetAutoSelect() { return GetFlag(DIF_SELECTONENTRY); }
  void __fastcall SetAutoSelect(bool value) { SetFlag(DIF_SELECTONENTRY, value); }
  bool __fastcall GetDropDownList() { return GetFlag(DIF_DROPDOWNLIST); }
  void __fastcall SetDropDownList(bool value) { SetFlag(DIF_DROPDOWNLIST, value); }
  int __fastcall GetItemIndex() const { return FList->GetSelected(); }
  void __fastcall SetItemIndex(int Index) { FList->SetSelected(Index); }

protected:
  virtual intptr_t __fastcall ItemProc(int Msg, void * Param);
  virtual void __fastcall Init();

private:
  TFarList * FList;
};
//---------------------------------------------------------------------------
class TFarLister : public TFarDialogItem
{
  typedef TFarLister self;
public:
  explicit /* __fastcall */ TFarLister(TFarDialog * ADialog);
  virtual /* __fastcall */ ~TFarLister();

  TStrings * __fastcall GetItems();
  void __fastcall SetItems(TStrings * value);
  int GetTopIndex() { return FTopIndex; }
  void __fastcall SetTopIndex(int value);
  bool __fastcall GetScrollBar();

protected:
  virtual intptr_t __fastcall ItemProc(int Msg, void * Param);
  virtual void __fastcall DoFocus();

private:
  TStringList * FItems;
  int FTopIndex;

  void /* __fastcall */ ItemsChange(TObject * Sender);
};
//---------------------------------------------------------------------------
UnicodeString __fastcall StripHotKey(const UnicodeString Text);
TRect __fastcall Rect(int Left, int Top, int Right, int Bottom);
//---------------------------------------------------------------------------
#endif
