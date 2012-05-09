#pragma once

#include "boostdefines.hpp"
#include <boost/signals/signal1.hpp>
#include <boost/signals/signal2.hpp>
#include <boost/signals/signal3.hpp>
#include <boost/signals/signal4.hpp>

#include "FarPlugin.h"
#include "nbafx.h"

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
typedef boost::signal4<void, TFarDialog *, TFarDialogItem *, long, bool &> key_signal_type;
typedef key_signal_type::slot_type key_slot_type;

typedef boost::signal2<void, TFarDialogItem *, MOUSE_EVENT_RECORD *> mouse_click_signal_type;
typedef mouse_click_signal_type::slot_type mouse_click_slot_type;

typedef boost::signal2<void, TFarDialogItem *, void *> processgroupevent_signal_type;
typedef processgroupevent_signal_type::slot_type processgroupevent_slot_type;
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

  int ShowModal();
  void ShowGroup(int Group, bool Show);
  void EnableGroup(int Group, bool Enable);

  TRect GetBounds() const { return FBounds; }
  void SetBounds(const TRect & value);
  TRect GetClientRect() const;
  UnicodeString GetHelpTopic() const { return FHelpTopic; }
  void SetHelpTopic(const UnicodeString value);
  size_t GetFlags() const { return FFlags; }
  void SetFlags(size_t value);
  bool GetCentered() const;
  void SetCentered(const bool & value);
  TPoint GetSize() const;
  void SetSize(const TPoint & value);
  TPoint GetClientSize() const;
  int GetWidth() const;
  void SetWidth(const int & value);
  size_t GetHeight() const;
  void SetHeight(const size_t & value);
  UnicodeString GetCaption() const;
  void SetCaption(const UnicodeString value);
  HANDLE GetHandle() const { return FHandle; }
  TFarButton * GetDefaultButton() const { return FDefaultButton; }
  TFarBox * GetBorderBox() const { return FBorderBox; }
  TFarDialogItem * GetItem(size_t Index);
  size_t GetItemCount() const;
  TItemPosition GetNextItemPosition() const { return FNextItemPosition; }
  void SetNextItemPosition(const TItemPosition & value) { FNextItemPosition = value; }
  void GetNextItemPosition(int & Left, int & Top);
  int GetDefaultGroup() const { return FDefaultGroup; }
  void SetDefaultGroup(const int & value) { FDefaultGroup = value; }
  size_t GetTag() const { return FTag; }
  void SetTag(size_t value) { FTag = value; }
  TFarDialogItem * GetItemFocused() const { return FItemFocused; }
  void SetItemFocused(TFarDialogItem * const & value);
  int GetResult() const { return FResult; }
  TPoint GetMaxSize();

  const key_signal_type & GetOnKey() const { return FOnKey; }
  void SetOnKey(const key_slot_type & value) { FOnKey.connect(value); }

  void Redraw();
  void LockChanges();
  void UnlockChanges();
  int GetSystemColor(size_t Index);
  bool HotKey(unsigned long Key);

  TCustomFarPlugin * GetFarPlugin() { return FFarPlugin; }

protected:
  TObjectList * GetItems() { return FItems; }

  void Add(TFarDialogItem * Item);
  void Add(TFarDialogContainer * Container);
  LONG_PTR SendMessage(int Msg, int Param1, LONG_PTR Param2);
  virtual LONG_PTR DialogProc(int Msg, int Param1, LONG_PTR Param2);
  virtual LONG_PTR FailDialogProc(int Msg, int Param1, LONG_PTR Param2);
  LONG_PTR DefaultDialogProc(int Msg, int Param1, LONG_PTR Param2);
  virtual bool __fastcall MouseEvent(MOUSE_EVENT_RECORD * Event);
  virtual bool __fastcall Key(TFarDialogItem * Item, long KeyCode);
  virtual void Change();
  virtual void __fastcall Init();
  virtual bool __fastcall CloseQuery();
  UnicodeString __fastcall GetMsg(int MsgId);
  void __fastcall RefreshBounds();
  virtual void __fastcall Idle();
  void __fastcall BreakSynchronize();
  void __fastcall Synchronize(const TThreadMethod & slot);
  void __fastcall Close(TFarButton * Button);
  void __fastcall ProcessGroup(int Group, const processgroupevent_slot_type & Callback, void * Arg);
  void ShowItem(TFarDialogItem * Item, void * Arg);
  void /* __fastcall */ EnableItem(TFarDialogItem * Item, void * Arg);
  bool __fastcall ChangesLocked();
  TFarDialogItem * __fastcall ItemAt(int X, int Y);

  static LONG_PTR WINAPI DialogProcGeneral(HANDLE Handle, int Msg, int Param1, LONG_PTR Param2);

private:
  TCustomFarPlugin * FFarPlugin;
  TRect FBounds;
  size_t FFlags;
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
  key_signal_type FOnKey;
  FarDialogItem * FDialogItems;
  size_t FDialogItemsCapacity;
  int FChangesLocked;
  bool FChangesPending;
  int FResult;
  bool FNeedsSynchronize;
  HANDLE FSynchronizeObjects[2];
  threadmethod_signal_type FSynchronizeMethod;
  TFarDialog * Self;
};
//---------------------------------------------------------------------------
class TFarDialogContainer : public TObject
{
  friend TFarDialog;
  friend TFarDialogItem;
  typedef TFarDialogContainer self;
public:
  int GetLeft() { return FLeft; }
  void SetLeft(int value) { SetPosition(0, value); }
  int GetTop() { return FTop; }
  void SetTop(int value) { SetPosition(1, value); }
  size_t GetItemCount() const;
  bool GetEnabled() { return FEnabled; }
  void SetEnabled(bool value);

protected:
  explicit TFarDialogContainer(TFarDialog * ADialog);
  virtual ~TFarDialogContainer();

  TFarDialog * GetDialog() { return FDialog; }

  void Add(TFarDialogItem * Item);
  void Remove(TFarDialogItem * Item);
  virtual void Change();
  UnicodeString GetMsg(int MsgId);

private:
  int FLeft;
  int FTop;
  TObjectList * FItems;
  TFarDialog * FDialog;
  bool FEnabled;

  void SetPosition(size_t Index, int value);
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
  TRect GetBounds() const { return FBounds; }
  void SetBounds(TRect value);
  TRect GetActualBounds();
  int GetLeft() { return GetCoordinate(0); }
  void SetLeft(int value) { SetCoordinate(0, value); }
  int GetTop() { return GetCoordinate(1); }
  void SetTop(int value) { SetCoordinate(1, value); }
  int GetRight() { return GetCoordinate(2); }
  void SetRight(int value) { SetCoordinate(2, value); }
  int GetBottom() { return GetCoordinate(3); }
  void SetBottom(int value) { SetCoordinate(3, value); }
  int GetWidth();
  void SetWidth(int value);
  size_t GetHeight();
  void SetHeight(int value);
  size_t GetFlags();
  void SetFlags(size_t value);
  bool GetEnabled() { return FEnabled; }
  void SetEnabled(bool value);
  bool GetIsEnabled() { return FIsEnabled; }
  TFarDialogItem * GetEnabledFollow() { return FEnabledFollow; }
  void SetEnabledFollow(TFarDialogItem * value);
  TFarDialogItem * GetEnabledDependency() { return FEnabledDependency; }
  void SetEnabledDependency(TFarDialogItem * value);
  TFarDialogItem * GetEnabledDependencyNegative() { return FEnabledDependencyNegative; }
  void SetEnabledDependencyNegative(TFarDialogItem * value);
  virtual bool GetIsEmpty();
  int GetGroup() { return FGroup; }
  void SetGroup(int value) { FGroup = value; }
  bool GetVisible() { return GetFlag(DIF_HIDDEN | DIF_INVERSE); }
  void SetVisible(bool value) { SetFlag(DIF_HIDDEN | DIF_INVERSE, value); }
  bool GetTabStop() { return GetFlag(DIF_NOFOCUS | DIF_INVERSE); }
  void SetTabStop(bool value) { SetFlag(DIF_NOFOCUS | DIF_INVERSE, value); }
  size_t GetTag() { return FTag; }
  void SetTag(size_t value) { FTag = value; }
  TFarDialog * GetDialog() { return FDialog; }

  const notify_signal_type & GetOnExit() const { return FOnExit; }
  void SetOnExit(const TNotifyEvent & value) { FOnExit.connect(value); }
  const mouse_click_signal_type & GetOnMouseClick() const { return FOnMouseClick; }
  void SetOnMouseClick(const mouse_click_slot_type & value) { FOnMouseClick.connect(value); }

  void Move(int DeltaX, int DeltaY);
  void MoveAt(int X, int Y);
  virtual bool CanFocus();
  bool Focused();
  void SetFocus();
  size_t GetItem() { return FItem; }
  void SetItem(size_t value) { FItem = value; }

protected:
  int FDefaultType;
  int FGroup;
  size_t FTag;
  notify_signal_type FOnExit;
  mouse_click_signal_type FOnMouseClick;

  explicit TFarDialogItem(TFarDialog * ADialog, int AType);
  virtual ~TFarDialogItem();

  FarDialogItem * GetDialogItem();
  bool GetCenterGroup() { return GetFlag(DIF_CENTERGROUP); }
  void SetCenterGroup(bool value) { SetFlag(DIF_CENTERGROUP, value); }
  virtual UnicodeString GetData();
  virtual void SetData(const UnicodeString value);
  size_t GetType();
  void SetType(size_t value);
  size_t GetSelected();
  void SetSelected(size_t value);
  TFarDialogContainer * GetContainer() { return FContainer; }
  void SetContainer(TFarDialogContainer * value);
  bool GetFocused();
  void SetFocused(bool value);
  bool GetChecked();
  void SetChecked(bool value);

  virtual void Detach();
  void DialogResized();
  LONG_PTR SendMessage(int Msg, LONG_PTR Param);
  LONG_PTR SendDialogMessage(int Msg, int Param1, LONG_PTR Param2);
  virtual LONG_PTR ItemProc(int Msg, LONG_PTR Param);
  LONG_PTR DefaultItemProc(int Msg, LONG_PTR Param);
  LONG_PTR DefaultDialogProc(int Msg, int Param1, LONG_PTR Param2);
  virtual LONG_PTR FailItemProc(int Msg, LONG_PTR Param);
  virtual void Change();
  void DialogChange();
  void SetAlterType(size_t Index, bool value);
  bool GetAlterType(size_t Index);
  virtual void UpdateBounds();
  virtual void ResetBounds();
  virtual void Init();
  virtual bool __fastcall CloseQuery();
  virtual bool MouseMove(int X, int Y, MOUSE_EVENT_RECORD * Event);
  virtual bool MouseClick(MOUSE_EVENT_RECORD * Event);
  TPoint MouseClientPosition(MOUSE_EVENT_RECORD * Event);
  void Text(int X, int Y, int Color, const UnicodeString Str);
  void Redraw();
  virtual bool HotKey(char HotKey);

  virtual void SetDataInternal(const UnicodeString value);
  void UpdateData(const UnicodeString value);
  void UpdateSelected(size_t value);

  bool GetFlag(size_t Index);
  void SetFlag(size_t Index, bool value);

  virtual void DoFocus();
  virtual void DoExit();

  char GetColor(size_t Index);
  void SetColor(size_t Index, char value);

private:
  TFarDialog * FDialog;
  TRect FBounds;
  TFarDialogItem * FEnabledFollow;
  TFarDialogItem * FEnabledDependency;
  TFarDialogItem * FEnabledDependencyNegative;
  TFarDialogContainer * FContainer;
  size_t FItem;
  bool FEnabled;
  bool FIsEnabled;
  unsigned long FColors;
  unsigned long FColorMask;

  void UpdateFlags(size_t value);
  void SetCoordinate(size_t Index, int value);
  int GetCoordinate(size_t Index);
  TFarDialogItem * GetPrevItem();
  void UpdateFocused(bool value);
  void UpdateEnabled();
};
//---------------------------------------------------------------------------
class TFarBox : public TFarDialogItem
{
public:
  TFarBox(TFarDialog * ADialog);

  virtual UnicodeString GetCaption() { return GetData(); }
  virtual void SetCaption(const UnicodeString value) { SetData(value); }
  virtual bool GetDouble() { return GetAlterType(DI_DOUBLEBOX); }
  virtual void SetDouble(bool value) { SetAlterType(DI_DOUBLEBOX, value); }
};
//---------------------------------------------------------------------------
typedef boost::signal2<void, TFarButton *, bool &> button_click_signal_type;
typedef button_click_signal_type::slot_type button_click_slot_type;

enum TFarButtonBrackets { brNone, brTight, brSpace, brNormal };
//---------------------------------------------------------------------------
class TFarButton : public TFarDialogItem
{
public:
  TFarButton(TFarDialog * ADialog);

  virtual UnicodeString GetCaption() { return GetData(); }
  virtual void SetCaption(const UnicodeString value) { SetData(value); }
  virtual int GetResult() { return FResult; }
  virtual void SetResult(int value) { FResult = value; }
  bool GetDefault();
  void SetDefault(bool value);
  TFarButtonBrackets GetBrackets() { return FBrackets; }
  void SetBrackets(TFarButtonBrackets value);
  bool GetCenterGroup() { return TFarDialogItem::GetCenterGroup(); }
  void SetCenterGroup(bool value) { TFarDialogItem::SetCenterGroup(value); }
  virtual const button_click_signal_type & GetOnClick() const { return FOnClick; }
  virtual void SetOnClick(const button_click_slot_type & value) { FOnClick.connect(value); }

protected:
  virtual void SetDataInternal(const UnicodeString value);
  virtual UnicodeString GetData();
  virtual LONG_PTR ItemProc(int Msg, LONG_PTR Param);
  virtual bool HotKey(char HotKey);

private:
  int FResult;
  button_click_signal_type FOnClick;
  TFarButtonBrackets FBrackets;
};
//---------------------------------------------------------------------------
typedef boost::signal3<void, TFarDialogItem *, long, bool &> farallowchange_signal_type;
typedef farallowchange_signal_type::slot_type farallowchange_slot_type;
//---------------------------------------------------------------------------
class TFarCheckBox : public TFarDialogItem
{
public:
  TFarCheckBox(TFarDialog * ADialog);

  virtual UnicodeString GetCaption() { return GetData(); }
  virtual void SetCaption(const UnicodeString value) { SetData(value); }
  bool GetAllowGrayed() { return GetFlag(DIF_3STATE); }
  void SetAllowGrayed(bool value) { SetFlag(DIF_3STATE, value); }
  virtual farallowchange_signal_type & GetOnAllowChange() { return FOnAllowChange; }
  virtual void SetOnAllowChange(const farallowchange_slot_type & value) { FOnAllowChange.connect(value); }
  bool GetChecked() { return TFarDialogItem::GetChecked(); }
  void SetChecked(bool value) { TFarDialogItem::SetChecked(value); }
  size_t GetSelected() { return TFarDialogItem::GetSelected(); }
  void SetSelected(size_t value) { TFarDialogItem::SetSelected(value); }

protected:
  farallowchange_signal_type FOnAllowChange;
  virtual LONG_PTR ItemProc(int Msg, LONG_PTR Param);
  virtual bool GetIsEmpty();
  virtual void SetData(const UnicodeString value);
};
//---------------------------------------------------------------------------
class TFarRadioButton : public TFarDialogItem
{
public:
  TFarRadioButton(TFarDialog * ADialog);

  bool GetChecked() { return TFarDialogItem::GetChecked(); }
  void SetChecked(bool value) { TFarDialogItem::SetChecked(value); }
  virtual UnicodeString GetCaption() { return GetData(); }
  virtual void SetCaption(const UnicodeString value) { SetData(value); }
  virtual farallowchange_signal_type & GetOnAllowChange() { return FOnAllowChange; }
  virtual void SetOnAllowChange(const farallowchange_slot_type & value) { FOnAllowChange.connect(value); }

protected:
  farallowchange_signal_type FOnAllowChange;
  virtual LONG_PTR ItemProc(int Msg, LONG_PTR Param);
  virtual bool GetIsEmpty();
  virtual void SetData(const UnicodeString value);
};
//---------------------------------------------------------------------------
class TFarEdit : public TFarDialogItem
{
public:
  TFarEdit(TFarDialog * ADialog);

  virtual UnicodeString GetText() { return GetData(); }
  virtual void SetText(const UnicodeString value) { SetData(value); }
  int GetAsInteger();
  void SetAsInteger(int value);
  virtual bool GetPassword() { return GetAlterType(DI_PSWEDIT); }
  virtual void SetPassword(bool value) { SetAlterType(DI_PSWEDIT, value); }
  virtual bool GetFixed() { return GetAlterType(DI_FIXEDIT); }
  virtual void SetFixed(bool value) { SetAlterType(DI_FIXEDIT, value); }

  virtual UnicodeString GetMask() { return GetHistoryMask(1); }
  virtual void SetMask(const UnicodeString value) { SetHistoryMask(1, value); }
  virtual UnicodeString GetHistory() { return GetHistoryMask(0); }
  virtual void SetHistory(const UnicodeString value) { SetHistoryMask(0, value); }
  bool GetExpandEnvVars() { return GetFlag(DIF_EDITEXPAND); }
  void SetExpandEnvVars(bool value) { SetFlag(DIF_EDITEXPAND, value); }
  bool GetAutoSelect() { return GetFlag(DIF_SELECTONENTRY); }
  void SetAutoSelect(bool value) { SetFlag(DIF_SELECTONENTRY, value); }
  bool GetReadOnly() { return GetFlag(DIF_READONLY); }
  void SetReadOnly(bool value) { SetFlag(DIF_READONLY, value); }

protected:
  virtual LONG_PTR ItemProc(int Msg, LONG_PTR Param);
  virtual void Detach();

private:
  UnicodeString GetHistoryMask(size_t Index);
  void SetHistoryMask(size_t Index, const UnicodeString value);
};
//---------------------------------------------------------------------------
class TFarSeparator : public TFarDialogItem
{
public:
  TFarSeparator(TFarDialog * ADialog);

  bool GetDouble();
  void SetDouble(bool value);
  virtual UnicodeString GetCaption() { return GetData(); }
  virtual void SetCaption(const UnicodeString value) { SetData(value); }
  int GetPosition();
  void SetPosition(int value);

protected:
  virtual void ResetBounds();

private:
};
//---------------------------------------------------------------------------
class TFarText : public TFarDialogItem
{
public:
  TFarText(TFarDialog * ADialog);

  virtual UnicodeString GetCaption() { return GetData(); }
  virtual void SetCaption(const UnicodeString value) { SetData(value); }
  bool GetCenterGroup() { return TFarDialogItem::GetCenterGroup(); }
  void SetCenterGroup(bool value) { TFarDialogItem::SetCenterGroup(value); }
  bool GetColor() { return TFarDialogItem::GetColor(0) != 0; }
  void SetColor(bool value) { TFarDialogItem::SetColor(0, value); }

protected:
  virtual void SetData(const UnicodeString value);
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
  typedef TFarList self;
public:
  explicit TFarList(TFarDialogItem * ADialogItem = NULL);
  virtual ~TFarList();

  virtual void __fastcall Assign(TPersistent * Source);

  size_t GetSelected();
  void SetSelected(size_t value);
  int GetTopIndex();
  void SetTopIndex(size_t value);
  size_t GetMaxLength();
  int GetVisibleCount();
  size_t GetFlags(size_t Index);
  void SetFlags(size_t Index, size_t value);
  bool GetDisabled(size_t Index) { return GetFlag(Index, LIF_DISABLE); }
  void SetDisabled(size_t Index, bool value) { SetFlag(Index, LIF_DISABLE, value); }
  bool GetChecked(size_t Index) { return GetFlag(Index, LIF_CHECKED); }
  void SetChecked(size_t Index, bool value) { SetFlag(Index, LIF_CHECKED, value); }

protected:
  virtual void __fastcall Changed();
  virtual LONG_PTR __fastcall ItemProc(int Msg, LONG_PTR Param);
  virtual void __fastcall Init();
  void UpdatePosition(int Position);
  int GetPosition();
  virtual void __fastcall Put(size_t Index, const UnicodeString S);
  void SetCurPos(size_t Position, size_t TopIndex);
  void UpdateItem(size_t Index);

  FarList * GetListItems() { return FListItems; }
  // void SetListItems(FarList *value) { FListItems = value; }
  TFarDialogItem * GetDialogItem() { return FDialogItem; }

private:
  FarList * FListItems;
  TFarDialogItem * FDialogItem;
  bool FNoDialogUpdate;
  TFarList * Self;

  inline size_t GetSelectedInt(bool Init);
  bool GetFlag(size_t Index, size_t Flag);
  void SetFlag(size_t Index, size_t Flag, bool value);
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

  void SetItems(TStrings * value);

  bool GetNoAmpersand() { return GetFlag(DIF_LISTNOAMPERSAND); }
  void SetNoAmpersand(bool value) { SetFlag(DIF_LISTNOAMPERSAND, value); }
  bool GetAutoHighlight() { return GetFlag(DIF_LISTAUTOHIGHLIGHT); }
  void SetAutoHighlight(bool value) { SetFlag(DIF_LISTAUTOHIGHLIGHT, value); }
  bool GetNoBox() { return GetFlag(DIF_LISTNOBOX); }
  void SetNoBox(bool value) { SetFlag(DIF_LISTNOBOX, value); }
  bool GetWrapMode() { return GetFlag(DIF_LISTWRAPMODE); }
  void SetWrapMode(bool value) { SetFlag(DIF_LISTWRAPMODE, value); }
  TFarList * GetItems() { return FList; }
  void SetList(TFarList * value);
  TFarListBoxAutoSelect GetAutoSelect() { return FAutoSelect; }
  void SetAutoSelect(TFarListBoxAutoSelect value);

protected:
  virtual LONG_PTR ItemProc(int Msg, LONG_PTR Param);
  virtual void Init();
  virtual bool __fastcall CloseQuery();

private:
  TFarList * FList;
  TFarListBoxAutoSelect FAutoSelect;

  void UpdateMouseReaction();
};
//---------------------------------------------------------------------------
class TFarComboBox : public TFarDialogItem
{
  typedef TFarComboBox self;
public:
  explicit TFarComboBox(TFarDialog * ADialog);
  virtual ~TFarComboBox();

  void ResizeToFitContent();

  bool GetNoAmpersand() { return GetFlag(DIF_LISTNOAMPERSAND); }
  void SetNoAmpersand(bool value) { SetFlag(DIF_LISTNOAMPERSAND, value); }
  bool GetAutoHighlight() { return GetFlag(DIF_LISTAUTOHIGHLIGHT); }
  void SetAutoHighlight(bool value) { SetFlag(DIF_LISTAUTOHIGHLIGHT, value); }
  bool GetWrapMode() { return GetFlag(DIF_LISTWRAPMODE); }
  void SetWrapMode(bool value) { SetFlag(DIF_LISTWRAPMODE, value); }
  TFarList * GetItems() { return FList; }
  virtual UnicodeString GetText() { return GetData(); }
  virtual void SetText(const UnicodeString value) { SetData(value); }
  bool GetAutoSelect() { return GetFlag(DIF_SELECTONENTRY); }
  void SetAutoSelect(bool value) { SetFlag(DIF_SELECTONENTRY, value); }
  bool GetDropDownList() { return GetFlag(DIF_DROPDOWNLIST); }
  void SetDropDownList(bool value) { SetFlag(DIF_DROPDOWNLIST, value); }

protected:
  virtual LONG_PTR ItemProc(int Msg, LONG_PTR Param);
  virtual void Init();

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

  TStrings * GetItems();
  void SetItems(TStrings * value);
  size_t GetTopIndex() { return FTopIndex; }
  void SetTopIndex(size_t value);
  bool GetScrollBar();

protected:
  virtual LONG_PTR ItemProc(int Msg, LONG_PTR Param);
  virtual void DoFocus();

private:
  TStringList * FItems;
  size_t FTopIndex;

  void ItemsChange(TObject * Sender);
};
//---------------------------------------------------------------------------
UnicodeString StripHotKey(const UnicodeString Text);
TRect Rect(int Left, int Top, int Right, int Bottom);
//---------------------------------------------------------------------------
