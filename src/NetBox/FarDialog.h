//---------------------------------------------------------------------------
#ifndef FarDialogH
#define FarDialogH
//---------------------------------------------------------------------------
#pragma once

#include "boostdefines.hpp"

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
#ifndef _MSC_VER
typedef void __fastcall (__closure * TFarKeyEvent)
  (TFarDialog * Sender, TFarDialogItem * Item, long KeyCode, bool & Handled);
typedef void __fastcall (__closure * TFarMouseClickEvent)
  (TFarDialogItem * Item, MOUSE_EVENT_RECORD * Event);
typedef void __fastcall (__closure * TFarProcessGroupEvent)
  (TFarDialogItem * Item, void * Arg);
#else
typedef fastdelegate::FastDelegate4<void,
  TFarDialog *, TFarDialogItem *, long, bool &> TFarKeyEvent;
typedef fastdelegate::FastDelegate2<void,
  TFarDialogItem *, MOUSE_EVENT_RECORD *> TFarMouseClickEvent;
typedef fastdelegate::FastDelegate2<void,
  TFarDialogItem *, void *> TFarProcessGroupEvent;
#endif
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

#ifndef _MSC_VER
  __property TRect Bounds = { read = FBounds, write = SetBounds };
  __property TRect ClientRect = { read = GetClientRect };
  __property AnsiString HelpTopic = { read = FHelpTopic, write = SetHelpTopic };
  __property unsigned int Flags = { read = FFlags, write = SetFlags };
  __property bool Centered = { read = GetCentered, write = SetCentered };
  __property TPoint Size = { read = GetSize, write = SetSize };
  __property TPoint ClientSize = { read = GetClientSize };
  __property int Width = { read = GetWidth, write = SetWidth };
  __property int Height = { read = GetHeight, write = SetHeight };
  __property AnsiString Caption = { read = GetCaption, write = SetCaption };
  __property HANDLE Handle = { read = FHandle };
  __property TFarButton * DefaultButton = { read = FDefaultButton };
  __property TFarBox * BorderBox = { read = FBorderBox };
  __property TFarDialogItem * Item[int Index] = { read = GetItem };
  __property int ItemCount = { read = GetItemCount };
  __property TItemPosition NextItemPosition = { read = FNextItemPosition, write = FNextItemPosition };
  __property int DefaultGroup = { read = FDefaultGroup, write = FDefaultGroup };
  __property int Tag = { read = FTag, write = FTag };
  __property TFarDialogItem * ItemFocused = { read = FItemFocused, write = SetItemFocused };
  __property int Result = { read = FResult };
  __property TPoint MaxSize = { read = GetMaxSize };

  __property TFarKeyEvent OnKey = { read = FOnKey, write = FOnKey };
#else
  TRect GetBounds() { return FBounds; }
  unsigned int GetFlags() const { return FFlags; }
  UnicodeString GetHelpTopic() { return FHelpTopic; }
  HANDLE GetHandle() { return FHandle; }
  TFarButton * GetDefaultButton() const { return FDefaultButton; }
  TFarBox * GetBorderBox() const { return FBorderBox; }
  TItemPosition GetNextItemPosition() { return FNextItemPosition; }
  void SetNextItemPosition(const TItemPosition & value) { FNextItemPosition = value; }
  int GetDefaultGroup() const { return FDefaultGroup; }
  void SetDefaultGroup(const int & value) { FDefaultGroup = value; }
  size_t GetTag() const { return FTag; }
  void SetTag(int value) { FTag = value; }
  TFarDialogItem * GetItemFocused() { return FItemFocused; }
  int GetResult() { return FResult; }

  const TFarKeyEvent & GetOnKey() const { return FOnKey; }
  void SetOnKey(const TFarKeyEvent & value) { FOnKey = value; }
#endif

  void __fastcall Redraw();
  void __fastcall LockChanges();
  void __fastcall UnlockChanges();
  int __fastcall GetSystemColor(unsigned int Index);
  bool __fastcall HotKey(unsigned long Key);

  TCustomFarPlugin * GetFarPlugin() { return FFarPlugin; }

protected:
#ifndef _MSC_VER
  __property TCustomFarPlugin * FarPlugin = { read = FFarPlugin };
  __property TObjectList * Items = { read = FItems };
#else
  TObjectList * GetItems() { return FItems; }
#endif
  void __fastcall Add(TFarDialogItem * Item);
  void __fastcall Add(TFarDialogContainer * Container);
  LONG_PTR __fastcall SendMessage(int Msg, int Param1, LONG_PTR Param2);
  virtual LONG_PTR __fastcall DialogProc(int Msg, int Param1, LONG_PTR Param2);
  virtual LONG_PTR __fastcall FailDialogProc(int Msg, int Param1, LONG_PTR Param2);
  LONG_PTR __fastcall DefaultDialogProc(int Msg, int Param1, LONG_PTR Param2);
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
  void __fastcall Synchronize(const TThreadMethodEvent & Method);
  void __fastcall Close(TFarButton * Button);
  void __fastcall ProcessGroup(int Group, const TFarProcessGroupEvent & Callback, void * Arg);
  void /* __fastcall */ ShowItem(TFarDialogItem * Item, void * Arg);
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
  TFarKeyEvent FOnKey;
  FarDialogItem * FDialogItems;
  size_t FDialogItemsCapacity;
  int FChangesLocked;
  bool FChangesPending;
  int FResult;
  bool FNeedsSynchronize;
  HANDLE FSynchronizeObjects[2];
  TThreadMethodEvent FSynchronizeMethod;
  TFarDialog * Self;

public:
  void __fastcall SetBounds(TRect value);
  void __fastcall SetHelpTopic(UnicodeString value);
  void __fastcall SetFlags(unsigned int value);
  void __fastcall SetCentered(bool value);
  bool __fastcall GetCentered();
  TPoint __fastcall GetSize();
  void __fastcall SetSize(TPoint value);
  void __fastcall SetCaption(UnicodeString value);
  UnicodeString __fastcall GetCaption();
  TFarDialogItem * __fastcall GetItem(int Index);
  TRect __fastcall GetClientRect();
  int __fastcall GetItemCount();
  void __fastcall SetItemFocused(TFarDialogItem * value);
  TPoint __fastcall GetClientSize();
  TPoint __fastcall GetMaxSize();
  void __fastcall SetWidth(int value);
  int __fastcall GetWidth();
  void __fastcall SetHeight(int value);
  int __fastcall GetHeight();
};
//---------------------------------------------------------------------------
class TFarDialogContainer : public TObject
{
friend TFarDialog;
friend TFarDialogItem;
typedef TFarDialogContainer self;
public:
#ifndef _MSC_VER
  __property int Left = { read = FLeft, write = SetPosition, index = 0 };
  __property int Top = { read = FTop, write = SetPosition, index = 1 };
  __property bool Enabled = { read = FEnabled, write = SetEnabled };
#else
  int __fastcall GetLeft() { return FLeft; }
  void __fastcall SetLeft(int value) { SetPosition(0, value); }
  int __fastcall GetTop() { return FTop; }
  void __fastcall SetTop(int value) { SetPosition(1, value); }
  int __fastcall GetItemCount();
  bool __fastcall GetEnabled() { return FEnabled; }
#endif

protected:
  explicit /* __fastcall */ TFarDialogContainer(TFarDialog * ADialog);
  virtual /* __fastcall */ ~TFarDialogContainer();

#ifndef _MSC_VER
  __property TFarDialog * Dialog = { read = FDialog };
#else
  TFarDialog * GetDialog() { return FDialog; }
#endif

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

public:
  void __fastcall SetPosition(int Index, int value);
  void __fastcall SetEnabled(bool value);
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
#ifndef _MSC_VER
  __property TRect Bounds = { read = FBounds, write = SetBounds };
  __property TRect ActualBounds = { read = GetActualBounds };
  __property int Left = { read = GetCoordinate, write = SetCoordinate, index = 0 };
  __property int Top = { read = GetCoordinate, write = SetCoordinate, index = 1 };
  __property int Right = { read = GetCoordinate, write = SetCoordinate, index = 2 };
  __property int Bottom = { read = GetCoordinate, write = SetCoordinate, index = 3 };
  __property int Width = { read = GetWidth, write = SetWidth };
  __property int Height = { read = GetHeight, write = SetHeight };
  __property unsigned int Flags = { read = GetFlags, write = SetFlags };
  __property bool Enabled = { read = FEnabled, write = SetEnabled };
  __property bool IsEnabled = { read = FIsEnabled };
  __property TFarDialogItem * EnabledFollow = { read = FEnabledFollow, write = SetEnabledFollow };
  __property TFarDialogItem * EnabledDependency = { read = FEnabledDependency, write = SetEnabledDependency };
  __property TFarDialogItem * EnabledDependencyNegative = { read = FEnabledDependencyNegative, write = SetEnabledDependencyNegative };
  __property bool IsEmpty = { read = GetIsEmpty };
  __property int Group = { read = FGroup, write = FGroup };
  __property bool Visible = { read = GetFlag, write = SetFlag, index = DIF_HIDDEN | DIF_INVERSE };
  __property bool TabStop = { read = GetFlag, write = SetFlag, index = DIF_NOFOCUS | DIF_INVERSE };
  __property bool Oem = { read = FOem, write = FOem };
  __property int Tag = { read = FTag, write = FTag };
  __property TFarDialog * Dialog = { read = FDialog };

  __property TNotifyEvent OnExit = { read = FOnExit, write = FOnExit };
  __property TFarMouseClickEvent OnMouseClick = { read = FOnMouseClick, write = FOnMouseClick };
#else
  TRect __fastcall GetBounds() { return FBounds; }
  int __fastcall GetLeft() { return GetCoordinate(0); }
  void __fastcall SetLeft(int value) { SetCoordinate(0, value); }
  int __fastcall GetTop() { return GetCoordinate(1); }
  void __fastcall SetTop(int value) { SetCoordinate(1, value); }
  int __fastcall GetRight() { return GetCoordinate(2); }
  void __fastcall SetRight(int value) { SetCoordinate(2, value); }
  int __fastcall GetBottom() { return GetCoordinate(3); }
  void __fastcall SetBottom(int value) { SetCoordinate(3, value); }
  bool __fastcall GetEnabled() { return FEnabled; }
  bool __fastcall GetIsEnabled() { return FIsEnabled; }
  bool __fastcall GetFocused();
  void __fastcall SetFocused(bool value);
  TFarDialogItem * __fastcall GetEnabledFollow() { return FEnabledFollow; }
  TFarDialogItem * __fastcall GetEnabledDependency() { return FEnabledDependency; }
  TFarDialogItem * __fastcall GetEnabledDependencyNegative() { return FEnabledDependencyNegative; }
  int __fastcall GetGroup() { return FGroup; }
  void __fastcall SetGroup(int value) { FGroup = value; }
  bool __fastcall GetVisible() { return GetFlag(DIF_HIDDEN | DIF_INVERSE); }
  void __fastcall SetVisible(bool value) { SetFlag(DIF_HIDDEN | DIF_INVERSE, value); }
  bool __fastcall GetTabStop() { return GetFlag(DIF_NOFOCUS | DIF_INVERSE); }
  void __fastcall SetTabStop(bool value) { SetFlag(DIF_NOFOCUS | DIF_INVERSE, value); }
  int __fastcall GetTag() { return FTag; }
  void SetTag(int value) { FTag = value; }
  TFarDialog * GetDialog() { return FDialog; }

  const TNotifyEvent & GetOnExit() const { return FOnExit; }
  void SetOnExit(const TNotifyEvent & value) { FOnExit = value; }
  const TFarMouseClickEvent & GetOnMouseClick() const { return FOnMouseClick; }
  void SetOnMouseClick(const TFarMouseClickEvent & value) { FOnMouseClick = value; }
#endif

  void __fastcall Move(int DeltaX, int DeltaY);
  void __fastcall MoveAt(int X, int Y);
  virtual bool __fastcall CanFocus();
  bool __fastcall Focused();
  void __fastcall SetFocus();
  int GetItem() { return FItem; }
  void SetItem(int value) { FItem = value; }

protected:
  int FDefaultType;
  int FGroup;
  int FTag;
  TNotifyEvent FOnExit;
  TFarMouseClickEvent FOnMouseClick;

  explicit /* __fastcall */ TFarDialogItem(TFarDialog * ADialog, int AType);
  virtual /* __fastcall */ ~TFarDialogItem();

#ifndef _MSC_VER
  __property FarDialogItem * DialogItem = { read = GetDialogItem };
  __property bool CenterGroup = { read = GetFlag, write = SetFlag, index = DIF_CENTERGROUP };
  __property AnsiString Data = { read = GetData, write = SetData };
  __property int Type = { read = GetType, write = SetType };
  __property int Item = { read = FItem };
  __property int Selected = { read = GetSelected, write = SetSelected };
  __property TFarDialogContainer * Container = { read = FContainer, write = SetContainer };
  __property bool Checked = { read = GetChecked, write = SetChecked };
#else
  bool GetCenterGroup() { return GetFlag(DIF_CENTERGROUP); }
  void SetCenterGroup(bool value) { SetFlag(DIF_CENTERGROUP, value); }
  TFarDialogContainer * GetContainer() { return FContainer; }
#endif

  virtual void __fastcall Detach();
  void __fastcall DialogResized();
  LONG_PTR __fastcall SendMessage(int Msg, LONG_PTR Param);
  LONG_PTR __fastcall SendDialogMessage(int Msg, int Param1, LONG_PTR Param2);
  virtual LONG_PTR __fastcall ItemProc(int Msg, LONG_PTR Param);
  LONG_PTR __fastcall DefaultItemProc(int Msg, LONG_PTR Param);
  LONG_PTR __fastcall DefaultDialogProc(int Msg, int Param1, LONG_PTR Param2);
  virtual LONG_PTR __fastcall FailItemProc(int Msg, LONG_PTR Param);
  virtual void __fastcall Change();
  void __fastcall DialogChange();
  void __fastcall SetAlterType(size_t Index, bool value);
  bool __fastcall GetAlterType(int Index);
  virtual void __fastcall UpdateBounds();
  virtual void __fastcall ResetBounds();
  virtual void __fastcall Init();
  virtual bool __fastcall CloseQuery();
  virtual bool /* __fastcall */ MouseMove(int X, int Y, MOUSE_EVENT_RECORD * Event);
  virtual bool /* __fastcall */ MouseClick(MOUSE_EVENT_RECORD * Event);
  TPoint __fastcall MouseClientPosition(MOUSE_EVENT_RECORD * Event);
  void __fastcall Text(int X, int Y, int Color, const UnicodeString Str);
  void __fastcall Redraw();
  virtual bool __fastcall HotKey(char HotKey);

public:
  virtual bool __fastcall GetIsEmpty();
  virtual void __fastcall SetDataInternal(const UnicodeString value);
  virtual void __fastcall SetData(const UnicodeString value);
  virtual UnicodeString __fastcall GetData();
  void __fastcall UpdateData(const UnicodeString value);
  void __fastcall UpdateSelected(int value);

  bool __fastcall GetFlag(int Index);
  void __fastcall SetFlag(int Index, bool value);

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
  // bool FOem;

public:
  void __fastcall SetBounds(TRect value);
  void __fastcall SetFlags(unsigned int value);
  void __fastcall UpdateFlags(unsigned int value);
  TRect __fastcall GetActualBounds();
  size_t __fastcall GetFlags();
  void __fastcall SetType(int value);
  int __fastcall GetType();
  void __fastcall SetEnabledFollow(TFarDialogItem * value);
  void __fastcall SetEnabledDependency(TFarDialogItem * value);
  void __fastcall SetEnabledDependencyNegative(TFarDialogItem * value);
  void __fastcall SetSelected(int value);
  int __fastcall GetSelected();
  void __fastcall SetCoordinate(int Index, int value);
  int __fastcall GetCoordinate(int Index);
  void __fastcall SetWidth(int value);
  int __fastcall GetWidth();
  void __fastcall SetHeight(int value);
  int __fastcall GetHeight();
  TFarDialogItem * __fastcall GetPrevItem();
  void __fastcall UpdateFocused(bool value);
  void __fastcall SetContainer(TFarDialogContainer * value);
  void __fastcall SetEnabled(bool value);
  void __fastcall UpdateEnabled();
  void __fastcall SetChecked(bool value);
  bool __fastcall GetChecked();
  FarDialogItem * __fastcall GetDialogItem();
};
//---------------------------------------------------------------------------
class TFarBox : public TFarDialogItem
{
public:
  explicit /* __fastcall */ TFarBox(TFarDialog * ADialog);

#ifndef _MSC_VER
  __property AnsiString Caption = { read = Data, write = Data };
  __property bool Double = { read = GetAlterType, write = SetAlterType, index = DI_DOUBLEBOX };
#else
  virtual UnicodeString GetCaption() { return GetData(); }
  virtual void SetCaption(const UnicodeString value) { SetData(value); }
  virtual bool GetDouble() { return GetAlterType(DI_DOUBLEBOX); }
  virtual void SetDouble(bool value) { SetAlterType(DI_DOUBLEBOX, value); }
#endif
};
//---------------------------------------------------------------------------
#ifndef _MSC_VER
typedef void __fastcall (__closure * TFarButtonClickEvent)(TFarButton * Sender, bool & Close);
#else
typedef fastdelegate::FastDelegate2<void, TFarButton *, bool &> TFarButtonClickEvent;
#endif
enum TFarButtonBrackets { brNone, brTight, brSpace, brNormal };
//---------------------------------------------------------------------------
class TFarButton : public TFarDialogItem
{
public:
  explicit /* __fastcall */ TFarButton(TFarDialog * ADialog);

#ifndef _MSC_VER
  __property AnsiString Caption = { read = Data, write = Data };
  __property int Result = { read = FResult, write = FResult };
  __property bool Default = { read = GetDefault, write = SetDefault };
  __property TFarButtonBrackets Brackets = { read = FBrackets, write = SetBrackets };
  __property CenterGroup;
  __property TFarButtonClickEvent OnClick = { read = FOnClick, write = FOnClick };
#else
  virtual UnicodeString GetCaption() { return GetData(); }
  virtual void SetCaption(const UnicodeString value) { SetData(value); }
  virtual int GetResult() { return FResult; }
  virtual void SetResult(int value) { FResult = value; }
  TFarButtonBrackets GetBrackets() { return FBrackets; }
  bool GetCenterGroup() { return TFarDialogItem::GetCenterGroup(); }
  void SetCenterGroup(bool value) { TFarDialogItem::SetCenterGroup(value); }
  virtual const TFarButtonClickEvent & GetOnClick() const { return FOnClick; }
  virtual void SetOnClick(const TFarButtonClickEvent & value) { FOnClick = value; }
#endif

protected:
  virtual void __fastcall SetDataInternal(const UnicodeString value);
  virtual UnicodeString __fastcall GetData();
  virtual LONG_PTR __fastcall ItemProc(int Msg, LONG_PTR Param);
  virtual bool __fastcall HotKey(char HotKey);

private:
  int FResult;
  TFarButtonClickEvent FOnClick;
  TFarButtonBrackets FBrackets;

public:
  void __fastcall SetDefault(bool value);
  bool __fastcall GetDefault();
  void __fastcall SetBrackets(TFarButtonBrackets value);
};
//---------------------------------------------------------------------------
#ifndef _MSC_VER
typedef void __fastcall (__closure * TFarAllowChangeEvent)(TFarDialogItem * Sender,
  long NewState, bool & AllowChange);
#else
typedef fastdelegate::FastDelegate3<void, TFarDialogItem *,
  long, bool &> TFarAllowChangeEvent;
#endif
//---------------------------------------------------------------------------
class TFarCheckBox : public TFarDialogItem
{
public:
  explicit /* __fastcall */ TFarCheckBox(TFarDialog * ADialog);

#ifndef _MSC_VER
  __property AnsiString Caption = { read = Data, write = Data };
  __property bool AllowGrayed = { read = GetFlag, write = SetFlag, index = DIF_3STATE };
  __property TFarAllowChangeEvent OnAllowChange = { read = FOnAllowChange, write = FOnAllowChange };
  __property Checked;
  __property Selected;
#else
  virtual UnicodeString GetCaption() { return GetData(); }
  virtual void SetCaption(const UnicodeString value) { SetData(value); }
  bool GetAllowGrayed() { return GetFlag(DIF_3STATE); }
  void SetAllowGrayed(bool value) { SetFlag(DIF_3STATE, value); }
  virtual TFarAllowChangeEvent & GetOnAllowChange() { return FOnAllowChange; }
  virtual void SetOnAllowChange(const TFarAllowChangeEvent & value) { FOnAllowChange = value; }
  bool GetChecked() { return TFarDialogItem::GetChecked(); }
  void SetChecked(bool value) { TFarDialogItem::SetChecked(value); }
  int GetSelected() { return TFarDialogItem::GetSelected(); }
  void SetSelected(size_t value) { TFarDialogItem::SetSelected(value); }
#endif

protected:
  TFarAllowChangeEvent FOnAllowChange;
  virtual LONG_PTR __fastcall ItemProc(int Msg, LONG_PTR Param);
  virtual bool __fastcall GetIsEmpty();
  virtual void __fastcall SetData(const UnicodeString value);
};
//---------------------------------------------------------------------------
class TFarRadioButton : public TFarDialogItem
{
public:
  explicit /* __fastcall */ TFarRadioButton(TFarDialog * ADialog);

#ifndef _MSC_VER
  __property Checked;
  __property AnsiString Caption = { read = Data, write = Data };
  __property TFarAllowChangeEvent OnAllowChange = { read = FOnAllowChange, write = FOnAllowChange };
#else
  bool GetChecked() { return TFarDialogItem::GetChecked(); }
  void SetChecked(bool value) { TFarDialogItem::SetChecked(value); }
  virtual UnicodeString GetCaption() { return GetData(); }
  virtual void SetCaption(const UnicodeString value) { SetData(value); }
  virtual TFarAllowChangeEvent & GetOnAllowChange() { return FOnAllowChange; }
  virtual void SetOnAllowChange(const TFarAllowChangeEvent & value) { FOnAllowChange = value; }
#endif

protected:
  TFarAllowChangeEvent FOnAllowChange;
  virtual LONG_PTR __fastcall ItemProc(int Msg, LONG_PTR Param);
  virtual bool __fastcall GetIsEmpty();
  virtual void __fastcall SetData(const UnicodeString value);
};
//---------------------------------------------------------------------------
class TFarEdit : public TFarDialogItem
{
public:
  explicit /* __fastcall */ TFarEdit(TFarDialog * ADialog);

#ifndef _MSC_VER
  __property AnsiString Text = { read = Data, write = Data };
  __property int AsInteger = { read = GetAsInteger, write = SetAsInteger };
  __property bool Password = { read = GetAlterType, write = SetAlterType, index = DI_PSWEDIT };
  __property bool Fixed = { read = GetAlterType, write = SetAlterType, index = DI_FIXEDIT };
  __property AnsiString Mask = { read = GetHistoryMask, write = SetHistoryMask, index = 1 };
  __property AnsiString History = { read = GetHistoryMask, write = SetHistoryMask, index = 0 };
  __property bool ExpandEnvVars = { read = GetFlag, write = SetFlag, index = DIF_EDITEXPAND };
  __property bool AutoSelect = { read = GetFlag, write = SetFlag, index = DIF_SELECTONENTRY };
  __property bool ReadOnly = { read = GetFlag, write = SetFlag, index = DIF_READONLY };
#else
  virtual UnicodeString GetText() { return GetData(); }
  virtual void SetText(const UnicodeString value) { SetData(value); }
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
#endif

protected:
  virtual LONG_PTR __fastcall ItemProc(int Msg, LONG_PTR Param);
  virtual void __fastcall Detach();

private:
  UnicodeString __fastcall GetHistoryMask(size_t Index);
  void __fastcall SetHistoryMask(size_t Index, const UnicodeString value);

public:
  void __fastcall SetAsInteger(int value);
  int __fastcall GetAsInteger();
};
//---------------------------------------------------------------------------
class TFarSeparator : public TFarDialogItem
{
public:
  explicit /* __fastcall */ TFarSeparator(TFarDialog * ADialog);

#ifndef _MSC_VER
  __property bool Double = { read = GetDouble, write = SetDouble };
  __property AnsiString Caption = { read = Data, write = Data };
  __property int Position = { read = GetPosition, write = SetPosition };
#else
  virtual UnicodeString GetCaption() { return GetData(); }
  virtual void SetCaption(const UnicodeString value) { SetData(value); }
#endif

protected:
  virtual void __fastcall ResetBounds();

public:
  void __fastcall SetDouble(bool value);
  bool __fastcall GetDouble();
  void __fastcall SetPosition(int value);
  int __fastcall GetPosition();
};
//---------------------------------------------------------------------------
class TFarText : public TFarDialogItem
{
public:
  explicit  /* __fastcall */ TFarText(TFarDialog * ADialog);

#ifndef _MSC_VER
  __property AnsiString Caption = { read = Data, write = Data };
  __property CenterGroup;
  __property char Color = { read = GetColor, write = SetColor, index = 0 };
#else
  virtual UnicodeString GetCaption() { return GetData(); }
  virtual void SetCaption(const UnicodeString value) { SetData(value); }
  bool GetCenterGroup() { return TFarDialogItem::GetCenterGroup(); }
  void SetCenterGroup(bool value) { TFarDialogItem::SetCenterGroup(value); }
  bool GetColor() { return TFarDialogItem::GetColor(0) != 0; }
  void SetColor(bool value) { TFarDialogItem::SetColor(0, value); }
#endif

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

#ifndef _MSC_VER
  __property int Selected = { read = GetSelected, write = SetSelected };
  __property int TopIndex = { read = GetTopIndex, write = SetTopIndex };
  __property int MaxLength = { read = GetMaxLength };
  __property int VisibleCount = { read = GetVisibleCount };
  __property unsigned int Flags[int Index] = { read = GetFlags, write = SetFlags };
  __property bool Disabled[int Index] = { read = GetFlag, write = SetFlag, index = LIF_DISABLE };
  __property bool Checked[int Index] = { read = GetFlag, write = SetFlag, index = LIF_CHECKED };
#else
  bool GetDisabled(int Index) { return GetFlag(Index, LIF_DISABLE); }
  void SetDisabled(int Index, bool value) { SetFlag(Index, LIF_DISABLE, value); }
  bool GetChecked(int Index) { return GetFlag(Index, LIF_CHECKED); }
  void SetChecked(int Index, bool value) { SetFlag(Index, LIF_CHECKED, value); }
#endif

protected:
  virtual void __fastcall Changed();
  virtual LONG_PTR __fastcall ItemProc(int Msg, LONG_PTR Param);
  virtual void __fastcall Init();
  void __fastcall UpdatePosition(int Position);
  int __fastcall GetPosition();
  virtual void __fastcall Put(int Index, const UnicodeString S);
  void __fastcall SetCurPos(int Position, int TopIndex);
  void __fastcall UpdateItem(int Index);

#ifndef _MSC_VER
  __property FarList * ListItems = { read = FListItems };
  __property TFarDialogItem * DialogItem = { read = FDialogItem };
#else
  FarList * GetListItems() { return FListItems; }
  TFarDialogItem * GetDialogItem() { return FDialogItem; }
#endif

private:
  FarList * FListItems;
  TFarDialogItem * FDialogItem;
  bool FNoDialogUpdate;
  TFarList * Self;

public:
  inline int __fastcall GetSelectedInt(bool Init);
  int __fastcall GetSelected();
  void __fastcall SetSelected(int value);
  int __fastcall GetTopIndex();
  void __fastcall SetTopIndex(int value);
  int __fastcall GetMaxLength();
  int __fastcall GetVisibleCount();
  unsigned int __fastcall GetFlags(int Index);
  void __fastcall SetFlags(int Index, unsigned int value);
  bool __fastcall GetFlag(int Index, int Flag);
  void __fastcall SetFlag(int Index, int Flag, bool value);
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

#ifndef _MSC_VER
  __property bool NoAmpersand = { read = GetFlag, write = SetFlag, index = DIF_LISTNOAMPERSAND };
  __property bool AutoHighlight = { read = GetFlag, write = SetFlag, index = DIF_LISTAUTOHIGHLIGHT };
  __property bool NoBox = { read = GetFlag, write = SetFlag, index = DIF_LISTNOBOX };
  __property bool WrapMode = { read = GetFlag, write = SetFlag, index = DIF_LISTWRAPMODE };
  __property TFarList * Items = { read = FList, write = SetList };
  __property TFarListBoxAutoSelect AutoSelect = { read = FAutoSelect, write = SetAutoSelect };
#else
  bool GetNoAmpersand() { return GetFlag(DIF_LISTNOAMPERSAND); }
  void SetNoAmpersand(bool value) { SetFlag(DIF_LISTNOAMPERSAND, value); }
  bool GetAutoHighlight() { return GetFlag(DIF_LISTAUTOHIGHLIGHT); }
  void SetAutoHighlight(bool value) { SetFlag(DIF_LISTAUTOHIGHLIGHT, value); }
  bool GetNoBox() { return GetFlag(DIF_LISTNOBOX); }
  void SetNoBox(bool value) { SetFlag(DIF_LISTNOBOX, value); }
  bool GetWrapMode() { return GetFlag(DIF_LISTWRAPMODE); }
  void SetWrapMode(bool value) { SetFlag(DIF_LISTWRAPMODE, value); }
  TFarList * GetItems() { return FList; }
  TFarListBoxAutoSelect GetAutoSelect() { return FAutoSelect; }
#endif

protected:
  virtual LONG_PTR __fastcall ItemProc(int Msg, LONG_PTR Param);
  virtual void __fastcall Init();
  virtual bool __fastcall CloseQuery();

private:
  TFarList * FList;
  TFarListBoxAutoSelect FAutoSelect;
  bool FDenyClose;

public:
  void __fastcall SetAutoSelect(TFarListBoxAutoSelect value);
  void __fastcall UpdateMouseReaction();
  void __fastcall SetList(TFarList * value);
};
//---------------------------------------------------------------------------
class TFarComboBox : public TFarDialogItem
{
  typedef TFarComboBox self;
public:
  explicit /* __fastcall */ TFarComboBox(TFarDialog * ADialog);
  virtual /* __fastcall */ ~TFarComboBox();

  void __fastcall ResizeToFitContent();

#ifndef _MSC_VER
  __property bool NoAmpersand = { read = GetFlag, write = SetFlag, index = DIF_LISTNOAMPERSAND };
  __property bool AutoHighlight = { read = GetFlag, write = SetFlag, index = DIF_LISTAUTOHIGHLIGHT };
  __property bool WrapMode = { read = GetFlag, write = SetFlag, index = DIF_LISTWRAPMODE };
  __property TFarList * Items = { read = FList };
  __property AnsiString Text = { read = Data, write = Data };
  __property bool AutoSelect = { read = GetFlag, write = SetFlag, index = DIF_SELECTONENTRY };
  __property bool DropDownList = { read = GetFlag, write = SetFlag, index = DIF_DROPDOWNLIST };
#else
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
#endif

protected:
  virtual LONG_PTR __fastcall ItemProc(int Msg, LONG_PTR Param);
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

#ifndef _MSC_VER
  __property TStrings * Items = { read = GetItems, write = SetItems };
  __property int TopIndex = { read = FTopIndex, write = SetTopIndex };
  __property bool ScrollBar = { read = GetScrollBar };
#else
  int GetTopIndex() { return FTopIndex; }
#endif

protected:
  virtual LONG_PTR __fastcall ItemProc(int Msg, LONG_PTR Param);
  virtual void __fastcall DoFocus();

private:
  TStringList * FItems;
  int FTopIndex;

public:
  void /* __fastcall */ ItemsChange(TObject * Sender);
  bool __fastcall GetScrollBar();
  TStrings * __fastcall GetItems();
  void __fastcall SetItems(TStrings * value);
  void __fastcall SetTopIndex(int value);
};
//---------------------------------------------------------------------------
UnicodeString __fastcall StripHotKey(const UnicodeString Text);
TRect __fastcall Rect(int Left, int Top, int Right, int Bottom);
//---------------------------------------------------------------------------
#endif
