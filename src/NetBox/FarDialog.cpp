#include <vcl.h>
#pragma hdrstop

#include <rdestl/map.h>

#include <Common.h>

#include "FarDialog.h"
#include <plugin.hpp>
#pragma warning(push, 1)
#include <farcolor.hpp>
#pragma warning(pop)

inline TRect Rect(int Left, int Top, int Right, int Bottom)
{
  return TRect(Left, Top, Right, Bottom);
}

TFarDialog::TFarDialog(TCustomFarPlugin *AFarPlugin) noexcept :
  TObject(OBJECT_CLASS_TFarDialog),
  FFarPlugin(AFarPlugin),
  FBounds(-1, -1, 40, 10),
  FFlags(0),
  FHelpTopic(),
  FVisible(false),
  FItems(std::make_unique<TObjectList>()),
  FContainers(std::make_unique<TObjectList>()),
  FHandle(nullptr),
  FDefaultButton(nullptr),
  FBorderBox(nullptr),
  FNextItemPosition(ipNewLine),
  FDefaultGroup(0),
  FTag(0),
  FItemFocused(nullptr),
  FOnKey(nullptr),
  FDialogItems(nullptr),
  FDialogItemsCapacity(0),
  FChangesLocked(0),
  FChangesPending(false),
  FResult(-1),
  FNeedsSynchronize(false),
  FSynchronizeMethod(nullptr)
{
  assert(AFarPlugin);
  FSynchronizeObjects[0] = INVALID_HANDLE_VALUE;
  FSynchronizeObjects[1] = INVALID_HANDLE_VALUE;

  FBorderBox = new TFarBox(this);
  FBorderBox->SetBounds(TRect(3, 1, -4, -2));
  FBorderBox->SetDouble(true);
}

TFarDialog::~TFarDialog() noexcept
{
  for (int32_t Index = 0; Index < GetItemCount(); ++Index)
  {
    GetItem(Index)->Detach();
  }
//  SAFE_DESTROY(FItems);
  nb_free(FDialogItems);
  FDialogItemsCapacity = 0;
//  SAFE_DESTROY(FContainers);
  SAFE_CLOSE_HANDLE(FSynchronizeObjects[0]);
  SAFE_CLOSE_HANDLE(FSynchronizeObjects[1]);
}

void TFarDialog::SetBounds(const TRect &Value)
{
  if (GetBounds() != Value)
  {
    LockChanges();
    {
      SCOPE_EXIT
      {
        UnlockChanges();
      };
      FBounds = Value;
      if (GetHandle())
      {
        COORD Coord;
        Coord.X = static_cast<short int>(GetSize().x);
        Coord.Y = static_cast<short int>(GetSize().y);
        SendDlgMessage(DM_RESIZEDIALOG, 0, reinterpret_cast<void *>(&Coord));
        Coord.X = static_cast<short int>(FBounds.Left);
        Coord.Y = static_cast<short int>(FBounds.Top);
        SendDlgMessage(DM_MOVEDIALOG, nb::ToInt(true), reinterpret_cast<void *>(&Coord));
      }
      for (int32_t Index = 0; Index < GetItemCount(); ++Index)
      {
        GetItem(Index)->DialogResized();
      }
    }
  }
}

TRect TFarDialog::GetClientRect() const
{
  TRect R;
  if (FBorderBox)
  {
    R = FBorderBox->GetBounds();
    R.Left += 2;
    R.Right -= 2;
    R.Top++;
    R.Bottom--;
  }
  else
  {
    R.Left = 0;
    R.Top = 0;
    R.Bottom = 0;
    R.Right = 0;
  }
  return R;
}

TPoint TFarDialog::GetClientSize() const
{
  TPoint S;
  if (FBorderBox)
  {
    TRect R = FBorderBox->GetActualBounds();
    S.x = static_cast<int>(R.Width()) + 1;
    S.y = static_cast<int>(R.Height()) + 1;
    S.x -= S.x > 4 ? 4 : S.x;
    S.y -= S.y > 2 ? 2 : S.y;
  }
  else
  {
    S = GetSize();
  }
  return S;
}

TPoint TFarDialog::GetMaxSize() const
{
  TPoint P = GetFarPlugin()->TerminalInfo();
  P.x -= 2;
  P.y -= 3;
  return P;
}

void TFarDialog::SetHelpTopic(const UnicodeString & Value)
{
  if (FHelpTopic != Value)
  {
    assert(!GetHandle());
    FHelpTopic = Value;
  }
}

void TFarDialog::SetFlags(const FARDIALOGITEMFLAGS Value)
{
  if (GetFlags() != Value)
  {
    assert(!GetHandle());
    FFlags = Value;
  }
}

void TFarDialog::SetCentered(bool Value)
{
  if (GetCentered() != Value)
  {
    assert(!GetHandle());
    TRect B = GetBounds();
    B.Left = Value ? -1 : 0;
    B.Top = Value ? -1 : 0;
    SetBounds(B);
  }
}

bool TFarDialog::GetCentered() const
{
  return (GetBounds().Left < 0) && (GetBounds().Top < 0);
}

TPoint TFarDialog::GetSize() const
{
  if (GetCentered())
  {
    return TPoint(static_cast<int>(GetBounds().Right), static_cast<int>(GetBounds().Bottom));
  }
  return TPoint(nb::ToInt(GetBounds().Width() + 1), nb::ToInt(GetBounds().Height() + 1));
}

void TFarDialog::SetSize(TPoint Value)
{
  TRect B = GetBounds();
  if (GetCentered())
  {
    B.Right = Value.x;
    B.Bottom = Value.y;
  }
  else
  {
    B.Right = FBounds.Left + Value.x - 1;
    B.Bottom = FBounds.Top + Value.y - 1;
  }
  SetBounds(B);
}

void TFarDialog::SetWidth(int32_t Value)
{
  SetSize(TPoint(nb::ToInt(Value), nb::ToInt(GetHeight())));
}

int32_t TFarDialog::GetWidth() const
{
  return GetSize().x;
}

void TFarDialog::SetHeight(int32_t Value)
{
  SetSize(TPoint(nb::ToInt(GetWidth()), nb::ToInt(Value)));
}

int32_t TFarDialog::GetHeight() const
{
  return GetSize().y;
}

void TFarDialog::SetCaption(const UnicodeString & Value)
{
  if (GetCaption() != Value)
  {
    FBorderBox->SetCaption(Value);
  }
}

UnicodeString TFarDialog::GetCaption() const
{
  return FBorderBox->GetCaption();
}

int32_t TFarDialog::GetItemCount() const
{
  return FItems->GetCount();
}

int32_t TFarDialog::GetItem(TFarDialogItem *Item) const
{
  if (!Item)
    return -1;
  return Item->GetItem();
}

TFarDialogItem *TFarDialog::GetItem(int32_t Index) const
{
  TFarDialogItem *DialogItem = nullptr;
  if (GetItemCount())
  {
    assert(Index >= 0 && Index < FItems->GetCount());
    DialogItem = FItems->GetAs<TFarDialogItem>(Index);
    assert(DialogItem);
  }
  return DialogItem;
}

void TFarDialog::Add(TFarDialogItem *DialogItem)
{
  TRect R = GetClientRect();
  int32_t Left, Top;
  GetNextItemPosition(Left, Top);
  R.Left = nb::ToInt(Left);
  R.Top = nb::ToInt(Top);

  if (FDialogItemsCapacity == GetItems()->GetCount())
  {
    int DialogItemsDelta = 10;
    FarDialogItem *NewDialogItems;
    NewDialogItems = nb::calloc<FarDialogItem *>(GetItems()->GetCount() + DialogItemsDelta, sizeof(FarDialogItem));
    if (FDialogItems)
    {
      memmove(NewDialogItems, FDialogItems, FDialogItemsCapacity * sizeof(FarDialogItem));
      nb_free(FDialogItems);
    }
    ::ZeroMemory(NewDialogItems + FDialogItemsCapacity, DialogItemsDelta * sizeof(FarDialogItem));
    FDialogItems = NewDialogItems;
    FDialogItemsCapacity += DialogItemsDelta;
  }

  assert(DialogItem);
  DialogItem->SetItem(GetItems()->Add(DialogItem));

  R.Bottom = R.Top;
  DialogItem->SetBounds(R);
  DialogItem->SetGroup(GetDefaultGroup());
}

void TFarDialog::Add(TFarDialogContainer *Container)
{
  FContainers->Add(Container);
}

void TFarDialog::GetNextItemPosition(int32_t &Left, int32_t &Top)
{
  TRect R = GetClientRect();
  Left = R.Left;
  Top = R.Top;

  TFarDialogItem *LastItem = GetItem(GetItemCount() - 1);
  LastItem = LastItem == FBorderBox ? nullptr : LastItem;

  if (LastItem)
  {
    switch (GetNextItemPosition())
    {
    case ipSame:
      Top = LastItem->GetTop();
      Left = LastItem->GetLeft();
      break;

    case ipNewLine:
      Top = LastItem->GetBottom() + 1;
      break;

    case ipBelow:
      Top = LastItem->GetBottom() + 1;
      Left = LastItem->GetLeft();
      break;

    case ipRight:
      Top = LastItem->GetTop();
      Left = LastItem->GetRight() + 3;
      break;
    }
  }
}

intptr_t WINAPI TFarDialog::DialogProcGeneral(HANDLE Handle, intptr_t Msg, intptr_t Param1, void *Param2)
{
  TFarPluginEnvGuard Guard; nb::used(Guard);

  static rde::map<HANDLE, void *> Dialogs;
  TFarDialog *Dialog = nullptr;
  LONG_PTR Result = 0;
  if (Msg == DN_INITDIALOG)
  {
    assert(Dialogs.find(Handle) == Dialogs.end());
    Dialogs[Handle] = Param2;
    Dialog = dyn_cast<TFarDialog>(as_object(Param2));
    Dialog->FHandle = Handle;
  }
  else
  {
    if (Dialogs.find(Handle) == Dialogs.end())
    {
      // DM_CLOSE is sent after DN_CLOSE, if the dialog was closed programmatically
      // by SendMessage(DM_CLOSE, ...)
      assert(Msg == DM_CLOSE);
      Result = static_cast<LONG_PTR>(0);
    }
    else
    {
      Dialog = dyn_cast<TFarDialog>(as_object(Dialogs[Handle]));
    }
  }

  if (Dialog != nullptr)
  {
    Result = Dialog->DialogProc(Msg, nb::ToIntPtr(Param1), Param2);
  }

  if ((Msg == DN_CLOSE) && Result)
  {
    if (Dialog != nullptr)
    {
      Dialog->FHandle = nullptr;
    }
    Dialogs.erase(Handle);
  }
  return Result;
}

intptr_t TFarDialog::DialogProc(intptr_t Msg, intptr_t Param1, void *Param2)
{

  intptr_t Result = 0;
  bool Handled = false;

  try
  {
    if (FNeedsSynchronize)
    {
      try
      {
        FNeedsSynchronize = false;
        FSynchronizeMethod();
        ::ReleaseSemaphore(FSynchronizeObjects[0], 1, nullptr);
        BreakSynchronize();
      }
      catch (...)
      {
        assert(false);
      }
    }

    bool Changed = false;

    switch (Msg)
    {
    case DN_BTNCLICK:
    case DN_EDITCHANGE:
    case DN_GOTFOCUS:
    case DN_KILLFOCUS:
    case DN_LISTCHANGE:
      Changed = true;

    // case DN_MOUSECLICK:
    case DN_CTLCOLORDLGITEM:
    case DN_CTLCOLORDLGLIST:
    case DN_DRAWDLGITEM:
    case DN_HOTKEY:
    case DN_CONTROLINPUT:
      // case DN_KEY:
      if (Param1 >= 0)
      {
        TFarDialogItem *Item = GetItem(Param1);
        try
        {
          Result = Item->ItemProc(Msg, Param2);
        }
        catch (Exception &E)
        {
          Handled = true;
          DEBUG_PRINTF("before GetFarPlugin()->HandleException");
          GetFarPlugin()->HandleException(&E);
          Result = Item->FailItemProc(Msg, Param2);
        }

        if (!Result && (Msg == DN_CONTROLINPUT))
        {
          INPUT_RECORD *Rec = reinterpret_cast<INPUT_RECORD *>(Param2);
          const KEY_EVENT_RECORD &Event = Rec->Event.KeyEvent;
          Result = Key(Item, static_cast<long>(Event.wVirtualKeyCode | (Event.dwControlKeyState << 16)));
        }
        Handled = true;
      }

      // FAR WORKAROUND
      // When pressing Enter FAR forces dialog to close without calling
      // DN_BTNCLICK on default button. This fixes the scenario.
      // (first check if focused dialog item is not another button)
      if (!Result && (Msg == DN_CONTROLINPUT) &&
        (reinterpret_cast<intptr_t>(Param2) == VK_RETURN) &&
        ((Param1 < 0) ||
          !isa<TFarButton>(GetItem(Param1))) &&
        GetDefaultButton()->GetEnabled() &&
        (GetDefaultButton()->GetOnClick()))
      {
        bool Close = (GetDefaultButton()->GetResult() != 0);
        GetDefaultButton()->GetOnClick()(GetDefaultButton(), Close);
        Handled = true;
        if (!Close)
        {
          Result = 1;
        }
      }
      break;
    case DN_INPUT:
      // case DN_MOUSEEVENT:
      Result = MouseEvent(reinterpret_cast<MOUSE_EVENT_RECORD *>(Param2));
      Handled = true;
      break;
    }
    if (!Handled)
    {
      switch (Msg)
      {
      case DN_INITDIALOG:
        Init();
        Result = 1;
        break;

      case DN_DRAGGED:
        if (Param1 == 1)
        {
          RefreshBounds();
        }
        break;

      case DN_DRAWDIALOG:
        // before drawing the dialog, make sure we know correct coordinates
        // (especially while the dialog is being dragged)
        RefreshBounds();
        break;

      case DN_CLOSE:
        Result = 1;
        if (Param1 >= 0)
        {
          TFarButton *Button = dyn_cast<TFarButton>(GetItem(Param1));
          // FAR WORKAROUND
          // FAR 1.70 alpha 6 calls DN_CLOSE even for non-button dialog items
          // (list boxes in particular), while FAR 1.70 beta 5 used ID of
          // default button in such case.
          // Particularly for listbox, we can prevent closing dialog using
          // flag DIF_LISTNOCLOSE.
          if (Button == nullptr)
          {
            DebugAssert(isa<TFarListBox>(GetItem(Param1)));
            Result = nb::ToIntPtr(false);
          }
          else
          {
            FResult = Button->GetResult();
          }
        }
        else
        {
          FResult = -1;
        }
        if (Result)
        {
          Result = CloseQuery();
          if (!Result)
          {
            FResult = -1;
          }
        }
        Handled = true;
        break;

      case DN_ENTERIDLE:
        Idle();
        break;
      }

      if (!Handled)
      {
        Result = DefaultDialogProc(Msg, Param1, Param2);
      }
    }
    if (Changed)
    {
      Change();
    }
  }
  catch (Exception &E)
  {
    DEBUG_PRINTF("before GetFarPlugin()->HandleException");
    GetFarPlugin()->HandleException(&E);
    if (!Handled)
    {
      Result = FailDialogProc(Msg, Param1, Param2);
    }
  }
  return Result;
}

int32_t TFarDialog::DefaultDialogProc(int32_t Msg, int32_t Param1, void *Param2)
{
  if (GetHandle())
  {
    TFarEnvGuard Guard; nb::used(Guard);
    return GetFarPlugin()->GetPluginStartupInfo()->DefDlgProc(GetHandle(), Msg, nb::ToInt(Param1), Param2);
  }
  return 0;
}

intptr_t TFarDialog::FailDialogProc(intptr_t Msg, intptr_t Param1, void *Param2)
{
  int32_t Result;
  switch (Msg)
  {
  case DN_CLOSE:
    Result = 0;
    break;

  default:
    Result = DefaultDialogProc(Msg, Param1, Param2);
    break;
  }
  return Result;
}

void TFarDialog::Idle()
{
  // nothing
}

bool TFarDialog::MouseEvent(MOUSE_EVENT_RECORD *Event)
{
  bool Result = true;
  bool Handled = false;
  if (FLAGSET(Event->dwEventFlags, MOUSE_MOVED))
  {
    int32_t X = Event->dwMousePosition.X - GetBounds().Left;
    int32_t Y = Event->dwMousePosition.Y - GetBounds().Top;
    TFarDialogItem *Item = ItemAt(X, Y);
    if (Item != nullptr)
    {
      Result = Item->MouseMove(X, Y, Event);
      Handled = true;
    }
  }
  else
  {
    Handled = false;
  }

  if (!Handled)
  {
    INPUT_RECORD Rec = {0};
    Rec.EventType = MOUSE_EVENT;
    memmove(&Rec.Event.MouseEvent, Event, sizeof(*Event));
    Result = DefaultDialogProc(DN_INPUT, 0, nb::ToPtr(&Rec)) != 0;
  }

  return Result;
}

bool TFarDialog::Key(TFarDialogItem *Item, LONG_PTR KeyCode)
{
  bool Result = false;
  if (FOnKey)
  {
    FOnKey(this, Item, static_cast<long>(KeyCode), Result);
  }
  return Result;
}

bool TFarDialog::HotKey(uint32_t Key, uint32_t ControlState) const
{
  bool Result = false;
  char HotKey = 0;
  if ((ControlState & ALTMASK) &&
    ('A' <= Key) && (Key <= 'Z'))
  {
    Result = true;
    HotKey = static_cast<char>('a' + static_cast<char>(Key - 'A'));
  }

  if (Result)
  {
    Result = false;
    for (int32_t Index = 0; Index < GetItemCount(); ++Index)
    {
      if (GetItem(Index)->HotKey(HotKey))
      {
        Result = true;
      }
    }
  }

  return Result;
}

TFarDialogItem *TFarDialog::ItemAt(int32_t X, int32_t Y)
{
  TFarDialogItem *Result = nullptr;
  for (int32_t Index = 0; Index < GetItemCount(); ++Index)
  {
    TRect Bounds = GetItem(Index)->GetActualBounds();
    if ((Bounds.Left <= nb::ToInt(X)) && (nb::ToInt(X) <= Bounds.Right) &&
      (Bounds.Top <= nb::ToInt(Y)) && (nb::ToInt(Y) <= Bounds.Bottom))
    {
      Result = GetItem(Index);
    }
  }
  return Result;
}

bool TFarDialog::CloseQuery()
{
  bool Result = true;
  for (int32_t Index = 0; Index < GetItemCount() && Result; ++Index)
  {
    if (!GetItem(Index)->CloseQuery())
    {
      Result = false;
    }
  }
  return Result;
}

void TFarDialog::RefreshBounds()
{
  SMALL_RECT Rect = {0};
  SendDlgMessage(DM_GETDLGRECT, 0, reinterpret_cast<void *>(&Rect));
  FBounds.Left = Rect.Left;
  FBounds.Top = Rect.Top;
  FBounds.Right = Rect.Right;
  FBounds.Bottom = Rect.Bottom;
}

void TFarDialog::Init()
{
  for (int32_t Index = 0; Index < GetItemCount(); ++Index)
  {
    GetItem(Index)->Init();
  }

  RefreshBounds();

  Change();
}

int32_t TFarDialog::ShowModal()
{
  FResult = -1;

  TFarDialog * PrevTopDialog = GetFarPlugin()->FTopDialog;
  GetFarPlugin()->FTopDialog = this;
  HANDLE Handle = INVALID_HANDLE_VALUE;
  {
    SCOPE_EXIT
    {
      GetFarPlugin()->FTopDialog = PrevTopDialog;
      if (Handle != INVALID_HANDLE_VALUE)
      {
        GetFarPlugin()->GetPluginStartupInfo()->DialogFree(Handle);
      }
    };
    assert(GetDefaultButton());
    assert(GetDefaultButton()->GetDefault());

    UnicodeString HelpTopic = GetHelpTopic();
    int32_t BResult;

    {
      TFarEnvGuard Guard; nb::used(Guard);
      TRect Bounds = GetBounds();
      const PluginStartupInfo &Info = *GetFarPlugin()->GetPluginStartupInfo();
      Handle = Info.DialogInit(
          &MainGuid, &MainGuid,
          Bounds.Left, Bounds.Top, Bounds.Right, Bounds.Bottom,
          HelpTopic.c_str(), FDialogItems,
          GetItemCount(), 0, GetFlags(),
          DialogProcGeneral,
          reinterpret_cast<void *>(this));
      BResult = Info.DialogRun(Handle);
    }

    if (BResult >= 0)
    {
      TFarButton * Button = dyn_cast<TFarButton>(GetItem(BResult));
      assert(Button);
      // correct result should be already set by TFarButton
      assert(FResult == Button->GetResult());
      FResult = Button->GetResult();
    }
    else
    {
      // allow only one negative value = -1
      FResult = -1;
    }
  }

  return FResult;
}

void TFarDialog::BreakSynchronize()
{
  ::SetEvent(FSynchronizeObjects[1]);
}

void TFarDialog::Synchronize(TThreadMethod Method)
{
  if (FSynchronizeObjects[0] == INVALID_HANDLE_VALUE)
  {
    FSynchronizeObjects[0] = ::CreateSemaphore(nullptr, 0, 2, nullptr);
    FSynchronizeObjects[1] = ::CreateEvent(nullptr, false, false, nullptr);
  }
  FSynchronizeMethod = Method;
  FNeedsSynchronize = true;
  ::WaitForMultipleObjects(_countof(FSynchronizeObjects),
    reinterpret_cast<HANDLE *>(&FSynchronizeObjects), false, INFINITE);
}

void TFarDialog::Close(TFarButton *Button)
{
  assert(Button != nullptr);
  SendDlgMessage(DM_CLOSE, Button->GetItem(), nullptr);
}

void TFarDialog::Change()
{
  if (FChangesLocked > 0)
  {
    FChangesPending = true;
  }
  else
  {
    std::unique_ptr<TList> NotifiedContainers(std::make_unique<TList>());
    for (int32_t Index = 0; Index < GetItemCount(); ++Index)
    {
      TFarDialogItem *DItem = GetItem(Index);
      DItem->Change();
      if (DItem->GetContainer() && NotifiedContainers->IndexOf(DItem->GetContainer()) == nb::NPOS)
      {
        NotifiedContainers->Add(DItem->GetContainer());
      }
    }

    for (int32_t Index = 0; Index < NotifiedContainers->GetCount(); ++Index)
    {
      NotifiedContainers->GetAs<TFarDialogContainer>(Index)->Change();
    }
  }
}

int32_t TFarDialog::SendDlgMessage(int32_t Msg, int32_t Param1, void *Param2)
{
  if (GetHandle())
  {
    TFarEnvGuard Guard; nb::used(Guard);
    return GetFarPlugin()->GetPluginStartupInfo()->SendDlgMessage(GetHandle(),
        Msg, Param1, Param2);
  }
  return 0;
}

FarColor TFarDialog::GetSystemColor(PaletteColors colorId)
{
  FarColor color = {0};
  if (GetFarPlugin()->FarAdvControl(ACTL_GETCOLOR, colorId, &color) != 0)
  {
    // TODO: throw error
  }
  return color;
}

void TFarDialog::Redraw()
{
  SendDlgMessage(DM_REDRAW, 0, nullptr);
}

void TFarDialog::ShowGroup(int32_t Group, bool Show)
{
  ProcessGroup(Group, nb::bind(&TFarDialog::ShowItem, this), &Show);
}

void TFarDialog::EnableGroup(int32_t Group, bool Enable)
{
  ProcessGroup(Group, nb::bind(&TFarDialog::EnableItem, this), &Enable);
}

void TFarDialog::ProcessGroup(int32_t Group, TFarProcessGroupEvent Callback,
  void *Arg)
{
  LockChanges();
  {
    SCOPE_EXIT
    {
      UnlockChanges();
    };
    for (int32_t Index = 0; Index < GetItemCount(); ++Index)
    {
      TFarDialogItem *Item = GetItem(Index);
      if (Item->GetGroup() == Group)
      {
        Callback(Item, Arg);
      }
    }
  }
}

void TFarDialog::ShowItem(TFarDialogItem *Item, void *Arg)
{
  Item->SetVisible(*static_cast<bool *>(Arg));
}

void TFarDialog::EnableItem(TFarDialogItem *Item, void *Arg)
{
  Item->SetEnabled(*static_cast<bool *>(Arg));
}

void TFarDialog::SetItemFocused(TFarDialogItem *Value)
{
  if (Value != GetItemFocused())
  {
    assert(Value);
    Value->SetFocus();
  }
}

UnicodeString TFarDialog::GetMsg(int32_t MsgId) const
{
  return FFarPlugin->GetMsg(MsgId);
}

void TFarDialog::LockChanges()
{
  assert(FChangesLocked < 10);
  FChangesLocked++;
  if (FChangesLocked == 1)
  {
    assert(!FChangesPending);
    if (GetHandle())
    {
      SendDlgMessage(DM_ENABLEREDRAW, 0, nullptr);
    }
  }
}

void TFarDialog::UnlockChanges()
{
  assert(FChangesLocked > 0);
  FChangesLocked--;
  if (FChangesLocked == 0)
  {
    SCOPE_EXIT
    {
      if (GetHandle())
      {
        this->SendDlgMessage(DM_ENABLEREDRAW, TRUE, nullptr);
      }
    };
    if (FChangesPending)
    {
      FChangesPending = false;
      Change();
    }
  }
}

bool TFarDialog::ChangesLocked() const
{
  return (FChangesLocked > 0);
}

TFarDialogContainer::TFarDialogContainer(TObjectClassId Kind, TFarDialog *ADialog) noexcept :
  TObject(Kind),
  FLeft(0),
  FTop(0),
  FItems(std::make_unique<TObjectList>()),
  FDialog(ADialog),
  FEnabled(true)
{
  assert(ADialog);
  FItems->SetOwnsObjects(false);
  GetDialog()->Add(this);
  GetDialog()->GetNextItemPosition(FLeft, FTop);
}

TFarDialogContainer::~TFarDialogContainer() noexcept
{
//  SAFE_DESTROY(FItems);
}

UnicodeString TFarDialogContainer::GetMsg(int32_t MsgId) const
{
  return GetDialog()->GetMsg(MsgId);
}

void TFarDialogContainer::Add(TFarDialogItem *Item)
{
  assert(FItems->IndexOf(Item) == nb::NPOS);
  Item->SetContainer(this);
  if (FItems->IndexOf(Item) == nb::NPOS)
    FItems->Add(Item);
}

void TFarDialogContainer::Remove(TFarDialogItem *Item)
{
  assert(FItems->IndexOf(Item) != nb::NPOS);
  Item->SetContainer(nullptr);
  FItems->Remove(Item);
  if (FItems->GetCount() == 0)
  {
    delete this;
  }
}

void TFarDialogContainer::SetPosition(int32_t AIndex, int32_t Value)
{
  int32_t &Position = AIndex ? FTop : FLeft;
  if (Position != Value)
  {
    Position = Value;
    for (int32_t Index = 0; Index < GetItemCount(); ++Index)
    {
      dyn_cast<TFarDialogItem>((*FItems)[Index])->DialogResized();
    }
  }
}

void TFarDialogContainer::Change()
{
}

void TFarDialogContainer::SetEnabled(bool Value)
{
  if (FEnabled != Value)
  {
    FEnabled = true;
    for (int32_t Index = 0; Index < GetItemCount(); ++Index)
    {
      FItems->GetAs<TFarDialogItem>(Index)->UpdateEnabled();
    }
  }
}

int32_t TFarDialogContainer::GetItemCount() const
{
  return FItems->GetCount();
}

TFarDialogItem::TFarDialogItem(TObjectClassId Kind, TFarDialog *ADialog, FARDIALOGITEMTYPES AType) :
  TObject(Kind),
  FDefaultType(AType),
  FGroup(0),
  FTag(0),
  FOnExit(nullptr),
  FOnMouseClick(nullptr),
  FDialog(ADialog),
  FEnabledFollow(nullptr),
  FEnabledDependency(nullptr),
  FEnabledDependencyNegative(nullptr),
  FContainer(nullptr),
  FItem(nb::NPOS),
  FColors(0),
  FColorMask(0),
  FEnabled(true),
  FIsEnabled(true)
{
  assert(ADialog);
  GetDialog()->Add(this);

  GetDialogItem()->Type = AType;
}

TFarDialogItem::~TFarDialogItem() noexcept
{
  TFarDialog *Dlg = GetDialog();
  assert(!Dlg);
  if (Dlg)
  {
    nb_free(GetDialogItem()->Data);
  }
}

const FarDialogItem *TFarDialogItem::GetDialogItem() const
{
  return const_cast<TFarDialogItem *>(this)->GetDialogItem();
}

FarDialogItem *TFarDialogItem::GetDialogItem()
{
  TFarDialog *Dlg = GetDialog();
  assert(Dlg);
  return &Dlg->FDialogItems[GetItem()];
}

void TFarDialogItem::SetBounds(const TRect &Value)
{
  if (FBounds != Value)
  {
    FBounds = Value;
    UpdateBounds();
  }
}

void TFarDialogItem::Detach()
{
  nb_free(GetDialogItem()->Data);
  FDialog = nullptr;
}

void TFarDialogItem::DialogResized()
{
  UpdateBounds();
}

void TFarDialogItem::ResetBounds()
{
  TRect B = FBounds;
  FarDialogItem *DItem = GetDialogItem();
  nb::used(B);
  nb::used(DItem);
#define BOUND(DIB, BB, DB, CB) DItem->DIB = B.BB >= 0 ? \
    (GetContainer() ? nb::ToInt(GetContainer()->CB) : 0) + B.BB : GetDialog()->GetSize().DB + B.BB
  BOUND(X1, Left, x, GetLeft());
  BOUND(Y1, Top, y, GetTop());
  BOUND(X2, Right, x, GetLeft());
  BOUND(Y2, Bottom, y, GetTop());
#undef BOUND
}

void TFarDialogItem::UpdateBounds()
{
  ResetBounds();

  if (GetDialog()->GetHandle())
  {
    TRect B = GetActualBounds();
    SMALL_RECT Rect;
    Rect.Left = static_cast<short int>(B.Left);
    Rect.Top = static_cast<short int>(B.Top);
    Rect.Right = static_cast<short int>(B.Right);
    Rect.Bottom = static_cast<short int>(B.Bottom);
    SendDialogMessage(DM_SETITEMPOSITION, reinterpret_cast<void *>(&Rect));
  }
}

char TFarDialogItem::GetColor(int32_t Index) const
{
  return *((reinterpret_cast<const char *>(&FColors)) + Index);
}

void TFarDialogItem::SetColor(int32_t Index, char Value)
{
  if (GetColor(Index) != Value)
  {
    *((reinterpret_cast<char *>(&FColors)) + Index) = Value;
    FColorMask |= (0xFF << (Index * 8));
  }
}

const struct PluginStartupInfo *TFarDialogItem::GetPluginStartupInfo() const
{
  return GetDialog()->GetFarPlugin()->GetPluginStartupInfo();
}

void TFarDialogItem::SetFlags(FARDIALOGITEMFLAGS Value)
{
  if (GetFlags() != Value)
  {
    assert(!GetDialog()->GetHandle());
    UpdateFlags(Value);
  }
}

void TFarDialogItem::UpdateFlags(FARDIALOGITEMFLAGS Value)
{
  if (GetFlags() != Value)
  {
    GetDialogItem()->Flags = Value;
    DialogChange();
  }
}

TRect TFarDialogItem::GetActualBounds() const
{
  return TRect(GetDialogItem()->X1, GetDialogItem()->Y1,
      GetDialogItem()->X2, GetDialogItem()->Y2);
}

FARDIALOGITEMFLAGS TFarDialogItem::GetFlags() const
{
  return GetDialogItem()->Flags;
}

void TFarDialogItem::SetDataInternal(const UnicodeString & Value)
{
  UnicodeString FarData = Value.c_str();
  if (GetDialog()->GetHandle())
  {
    SendDialogMessage(DM_SETTEXTPTR, nb::ToPtr(ToWChar(FarData)));
  }
  nb_free(GetDialogItem()->Data);
  GetDialogItem()->Data = TCustomFarPlugin::DuplicateStr(FarData, /*AllowEmpty=*/true);

  DialogChange();
}

void TFarDialogItem::SetData(const UnicodeString & Value)
{
  if (GetData() != Value)
  {
    SetDataInternal(Value);
  }
}

void TFarDialogItem::UpdateData(const UnicodeString & Value)
{
  UnicodeString FarData = Value.c_str();
  nb_free(GetDialogItem()->Data);
  GetDialogItem()->Data = TCustomFarPlugin::DuplicateStr(FarData, /*AllowEmpty=*/true);
}

UnicodeString TFarDialogItem::GetData() const
{
  return const_cast<TFarDialogItem *>(this)->GetData();
}

UnicodeString TFarDialogItem::GetData()
{
  UnicodeString Result;
  if (GetDialogItem()->Data)
  {
    Result = GetDialogItem()->Data;
  }
  return Result;
}

void TFarDialogItem::SetType(FARDIALOGITEMTYPES Value)
{
  if (GetType() != Value)
  {
    assert(!GetDialog()->GetHandle());
    GetDialogItem()->Type = Value;
  }
}

FARDIALOGITEMTYPES TFarDialogItem::GetType() const
{
  return static_cast<FARDIALOGITEMTYPES>(GetDialogItem()->Type);
}

void TFarDialogItem::SetAlterType(FARDIALOGITEMTYPES Index, bool Value)
{
  if (GetAlterType(Index) != Value)
  {
    SetType(Value ? Index : FDefaultType);
  }
}

bool TFarDialogItem::GetAlterType(FARDIALOGITEMTYPES Index) const
{
  return const_cast<TFarDialogItem *>(this)->GetAlterType(Index);
}

bool TFarDialogItem::GetAlterType(FARDIALOGITEMTYPES Index)
{
  return (GetType() == Index);
}

bool TFarDialogItem::GetFlag(FARDIALOGITEMFLAGS Index) const
{
  bool Result = (GetFlags() & (Index & 0xFFFFFFFFFFFFFF00ULL)) != 0;
  if (Index & 0x000000FFUL)
  {
    Result = !Result;
  }
  // bool Result = (GetFlags() & Index) != 0;
  return Result;
}

void TFarDialogItem::SetFlag(FARDIALOGITEMFLAGS Index, bool Value)
{
  if (GetFlag(Index) != Value)
  {
    if (Index & DIF_INVERSE)
    {
      Value = !Value;
    }

    FARDIALOGITEMFLAGS F = GetFlags();
    FARDIALOGITEMFLAGS Flag = Index & 0xFFFFFFFFFFFFFF00ULL;
    bool ToHandle = true;

    switch (Flag)
    {
    case DIF_DISABLE:
      if (GetDialog()->GetHandle())
      {
        SendDialogMessage(DM_ENABLE, reinterpret_cast<void *>(!Value));
      }
      break;

    case DIF_HIDDEN:
      if (GetDialog()->GetHandle())
      {
        SendDialogMessage(DM_SHOWITEM, reinterpret_cast<void *>(!Value));
      }
      break;

    case DIF_3STATE:
      if (GetDialog()->GetHandle())
      {
        SendDialogMessage(DM_SET3STATE, reinterpret_cast<void *>(Value));
      }
      break;
    }

    if (ToHandle)
    {
      if (Value)
      {
        F |= Flag;
      }
      else
      {
        F &= ~Flag;
      }
      UpdateFlags(F);
    }
  }
}

void TFarDialogItem::SetEnabledFollow(TFarDialogItem *Value)
{
  if (GetEnabledFollow() != Value)
  {
    FEnabledFollow = Value;
    Change();
  }
}

void TFarDialogItem::SetEnabledDependency(TFarDialogItem *Value)
{
  if (GetEnabledDependency() != Value)
  {
    FEnabledDependency = Value;
    Change();
  }
}

void TFarDialogItem::SetEnabledDependencyNegative(TFarDialogItem *Value)
{
  if (GetEnabledDependencyNegative() != Value)
  {
    FEnabledDependencyNegative = Value;
    Change();
  }
}

bool TFarDialogItem::GetIsEmpty() const
{
  return GetData().IsEmpty();
}

int32_t TFarDialogItem::FailItemProc(int32_t Msg, void *Param)
{
  int32_t Result;
  switch (Msg)
  {
  case DN_KILLFOCUS:
    Result = nb::ToIntPtr(GetItem());
    break;

  default:
    Result = DefaultItemProc(Msg, Param);
    break;
  }
  return Result;
}

int32_t TFarDialogItem::ItemProc(int32_t Msg, void *Param)
{
  LONG_PTR Result = 0;
  bool Handled = false;

  if (Msg == DN_GOTFOCUS)
  {
    DoFocus();
    UpdateFocused(true);
  }
  else if (Msg == DN_KILLFOCUS)
  {
    DoExit();
    UpdateFocused(false);
  }
  else if (Msg == DN_CONTROLINPUT)
  {
    INPUT_RECORD *Rec = reinterpret_cast<INPUT_RECORD *>(Param);
    MOUSE_EVENT_RECORD *Event = &Rec->Event.MouseEvent;
    if (FLAGCLEAR(Event->dwEventFlags, MOUSE_MOVED))
    {
      Result = MouseClick(Event);
      Handled = true;
    }
  }

  if (!Handled)
  {
    Result = DefaultItemProc(Msg, Param);
  }

  if (Msg == DN_CTLCOLORDLGITEM && FColorMask)
  {
    Result &= ~static_cast<LONG_PTR>(FColorMask);
    Result |= (FColors & FColorMask);
  }
  return Result;
}

void TFarDialogItem::DoFocus()
{
}

void TFarDialogItem::DoExit()
{
  if (FOnExit)
  {
    FOnExit(this);
  }
}

int32_t TFarDialogItem::DefaultItemProc(int32_t Msg, void *Param)
{
  if (GetDialog() && GetDialog()->GetHandle())
  {
    TFarEnvGuard Guard; nb::used(Guard);
    return GetPluginStartupInfo()->DefDlgProc(GetDialog()->GetHandle(),
        Msg, nb::ToInt(GetItem()), Param);
  }
  return 0;
}

int32_t TFarDialogItem::DefaultDialogProc(int32_t Msg, int32_t Param1, void *Param2)
{
  if (GetDialog() && GetDialog()->GetHandle())
  {
    TFarEnvGuard Guard; nb::used(Guard);
    return GetPluginStartupInfo()->DefDlgProc(GetDialog()->GetHandle(),
        Msg, nb::ToInt(Param1), Param2);
  }
  return 0;
}

void TFarDialogItem::Change()
{
  if (GetEnabledFollow() || GetEnabledDependency() || GetEnabledDependencyNegative())
  {
    UpdateEnabled();
  }
}

void TFarDialogItem::SetEnabled(bool Value)
{
  if (GetEnabled() != Value)
  {
    FEnabled = Value;
    UpdateEnabled();
  }
}

void TFarDialogItem::UpdateEnabled()
{
  bool Value =
    GetEnabled() &&
    (!GetEnabledFollow() || GetEnabledFollow()->GetIsEnabled()) &&
    (!GetEnabledDependency() ||
      (!GetEnabledDependency()->GetIsEmpty() && GetEnabledDependency()->GetIsEnabled())) &&
    (!GetEnabledDependencyNegative() ||
      (GetEnabledDependencyNegative()->GetIsEmpty() || !GetEnabledDependencyNegative()->GetIsEnabled())) &&
    (!GetContainer() || GetContainer()->GetEnabled());

  if (Value != GetIsEnabled())
  {
    FIsEnabled = Value;
    SetFlag(DIF_DISABLE | DIF_INVERSE, GetIsEnabled());
  }
}

void TFarDialogItem::DialogChange()
{
  TFarDialog *Dlg = GetDialog();
  assert(Dlg);
  Dlg->Change();
}

int32_t TFarDialogItem::SendDialogMessage(int32_t Msg, int32_t Param1, void *Param2)
{
  return GetDialog()->SendDlgMessage(Msg, Param1, Param2);
}

int32_t TFarDialogItem::SendDialogMessage(int32_t Msg, void *Param)
{
  return GetDialog()->SendDlgMessage(Msg, GetItem(), Param);
}

void TFarDialogItem::SetSelected(int32_t Value)
{
  if (GetSelected() != Value)
  {
    if (GetDialog()->GetHandle())
    {
      SendDialogMessage(DM_SETCHECK, reinterpret_cast<void *>(Value));
    }
    UpdateSelected(Value);
  }
}

void TFarDialogItem::UpdateSelected(int32_t Value)
{
  if (GetSelected() != Value)
  {
    GetDialogItem()->Selected = nb::ToInt(Value);
    DialogChange();
  }
}

int32_t TFarDialogItem::GetSelected() const
{
  return nb::ToIntPtr(GetDialogItem()->Selected);
}

bool TFarDialogItem::GetFocused() const
{
  return GetFlag(DIF_FOCUS);
}

void TFarDialogItem::SetFocused(bool Value)
{
  SetFlag(DIF_FOCUS, Value);
}

bool TFarDialogItem::GetChecked() const
{
  return GetSelected() == BSTATE_CHECKED;
}

void TFarDialogItem::SetChecked(bool Value)
{
  SetSelected(Value ? BSTATE_CHECKED : BSTATE_UNCHECKED);
}

void TFarDialogItem::Move(int32_t DeltaX, int32_t DeltaY)
{
  TRect R = GetBounds();

  R.Left += nb::ToInt(DeltaX);
  R.Right += nb::ToInt(DeltaX);
  R.Top += nb::ToInt(DeltaY);
  R.Bottom += nb::ToInt(DeltaY);

  SetBounds(R);
}

void TFarDialogItem::MoveAt(int32_t X, int32_t Y)
{
  Move(X - GetLeft(), Y - GetTop());
}

void TFarDialogItem::SetCoordinate(int32_t Index, int32_t Value)
{
  static_assert(sizeof(TRect) == sizeof(int32_t) * 4);
  TRect R = GetBounds();
  int32_t *D = reinterpret_cast<int32_t *>(&R);
  D += Index;
  *D = nb::ToInt32(Value);
  SetBounds(R);
}

int32_t TFarDialogItem::GetCoordinate(int32_t Index) const
{
  static_assert(sizeof(TRect) == sizeof(int32_t) * 4);
  TRect R = GetBounds();
  int32_t *D = reinterpret_cast<int32_t *>(&R);
  D += Index;
  return nb::ToInt32(*D);
}

void TFarDialogItem::SetWidth(int32_t Value)
{
  TRect R = GetBounds();
  if (R.Left >= 0)
  {
    R.Right = R.Left + nb::ToInt(Value - 1);
  }
  else
  {
    assert(R.Right < 0);
    R.Left = R.Right - nb::ToInt(Value + 1);
  }
  SetBounds(R);
}

int32_t TFarDialogItem::GetWidth() const
{
  return nb::ToIntPtr(GetActualBounds().Width() + 1);
}

void TFarDialogItem::SetHeight(int32_t Value)
{
  TRect R = GetBounds();
  if (R.Top >= 0)
  {
    R.Bottom = nb::ToInt(R.Top + Value - 1);
  }
  else
  {
    assert(R.Bottom < 0);
    R.Top = nb::ToInt(R.Bottom - Value + 1);
  }
  SetBounds(R);
}

int32_t TFarDialogItem::GetHeight() const
{
  return nb::ToIntPtr(GetActualBounds().Height() + 1);
}

bool TFarDialogItem::CanFocus() const
{
  FARDIALOGITEMTYPES Type = GetType();
  return GetVisible() && GetEnabled() && GetTabStop() &&
    (Type == DI_EDIT || Type == DI_PSWEDIT || Type == DI_FIXEDIT ||
      Type == DI_BUTTON || Type == DI_CHECKBOX || Type == DI_RADIOBUTTON ||
      Type == DI_COMBOBOX || Type == DI_LISTBOX || Type == DI_USERCONTROL);
}

bool TFarDialogItem::Focused() const
{
  return GetFocused();
}

void TFarDialogItem::UpdateFocused(bool Value)
{
  SetFocused(Value);
  TFarDialog *Dlg = GetDialog();
  assert(Dlg);
  Dlg->SetItemFocused(Value ? this : nullptr);
}

void TFarDialogItem::SetFocus()
{
  assert(CanFocus());
  if (!Focused())
  {
    if (GetDialog()->GetHandle())
    {
      SendDialogMessage(DM_SETFOCUS, nullptr);
    }
    else
    {
      if (GetDialog()->GetItemFocused())
      {
        assert(GetDialog()->GetItemFocused() != this);
        GetDialog()->GetItemFocused()->UpdateFocused(false);
      }
      UpdateFocused(true);
    }
  }
}

void TFarDialogItem::Init()
{
  if (GetFlag(DIF_CENTERGROUP))
  {
    SMALL_RECT Rect;
    nb::ClearStruct(Rect);

    // at least for "text" item, returned item size is not correct (on 1.70 final)
    SendDialogMessage(DM_GETITEMPOSITION, reinterpret_cast<void *>(&Rect));

    TRect B = GetBounds();
    B.Left = Rect.Left;
    B.Right = Rect.Right;
    SetBounds(B);
  }
}

bool TFarDialogItem::CloseQuery()
{
  if (Focused() && (GetDialog()->GetResult() >= 0))
  {
    DoExit();
  }
  return true;
}

TPoint TFarDialogItem::MouseClientPosition(MOUSE_EVENT_RECORD *Event)
{
  TPoint Result;
  if (GetType() == DI_USERCONTROL)
  {
    Result = TPoint(Event->dwMousePosition.X, Event->dwMousePosition.Y);
  }
  else
  {
    Result = TPoint(
        nb::ToInt(Event->dwMousePosition.X - GetDialog()->GetBounds().Left - GetLeft()),
        nb::ToInt(Event->dwMousePosition.Y - GetDialog()->GetBounds().Top - GetTop()));
  }
  return Result;
}

bool TFarDialogItem::MouseClick(MOUSE_EVENT_RECORD *Event)
{
  if (FOnMouseClick)
  {
    FOnMouseClick(this, Event);
  }
  INPUT_RECORD Rec = {0};
  Rec.EventType = MOUSE_EVENT;
  memmove(&Rec.Event.MouseEvent, Event, sizeof(*Event));
  return DefaultItemProc(DN_CONTROLINPUT, nb::ToPtr(&Rec)) != 0;
}

bool TFarDialogItem::MouseMove(int32_t /*X*/, int32_t /*Y*/,
  MOUSE_EVENT_RECORD *Event)
{
  INPUT_RECORD Rec = {0};
  Rec.EventType = MOUSE_EVENT;
  memmove(&Rec.Event.MouseEvent, Event, sizeof(*Event));
  return DefaultDialogProc(DN_INPUT, 0, reinterpret_cast<void *>(&Rec)) != 0;
}

void TFarDialogItem::Text(int32_t X, int32_t Y, const FarColor &Color, UnicodeString Str)
{
  TFarEnvGuard Guard; nb::used(Guard);
  GetPluginStartupInfo()->Text(
    nb::ToInt(GetDialog()->GetBounds().Left + GetLeft() + X),
    nb::ToInt(GetDialog()->GetBounds().Top + GetTop() + Y),
    &Color, Str.c_str());
}

void TFarDialogItem::Redraw()
{
  // do not know how to force redraw of the item only
  GetDialog()->Redraw();
}

void TFarDialogItem::SetContainer(TFarDialogContainer *Value)
{
  if (GetContainer() != Value)
  {
    TFarDialogContainer *PrevContainer = GetContainer();
    FContainer = Value;
    if (PrevContainer)
    {
      PrevContainer->Remove(this);
    }
    if (GetContainer())
    {
      GetContainer()->Add(this);
    }
    UpdateBounds();
    UpdateEnabled();
  }
}

bool TFarDialogItem::HotKey(char /*HotKey*/)
{
  return false;
}

TFarBox::TFarBox(TFarDialog *ADialog) noexcept :
  TFarDialogItem(OBJECT_CLASS_TFarBox, ADialog, DI_SINGLEBOX)
{
}

TFarButton::TFarButton(TFarDialog *ADialog) noexcept :
  TFarDialogItem(OBJECT_CLASS_TFarButton, ADialog, DI_BUTTON),
  FResult(0),
  FOnClick(nullptr),
  FBrackets(brNormal)
{
}

TFarButton::TFarButton(TObjectClassId Kind, TFarDialog *ADialog) noexcept :
  TFarDialogItem(Kind, ADialog, DI_BUTTON),
  FResult(0),
  FOnClick(nullptr),
  FBrackets(brNormal)
{
}

void TFarButton::SetDataInternal(const UnicodeString & AValue)
{
  UnicodeString Value;
  switch (FBrackets)
  {
  case brTight:
    Value = L"[" + AValue + L"]";
    break;

  case brSpace:
    Value = L" " + AValue + L" ";
    break;

  default:
    Value = AValue;
    break;
  }

  TFarDialogItem::SetDataInternal(Value);

  if ((GetLeft() >= 0) || (GetRight() >= 0))
  {
    int Margin = 0;
    switch (FBrackets)
    {
    case brNone:
      Margin = 0;
      break;

    case brTight:
    case brSpace:
      Margin = 1;
      break;

    case brNormal:
      Margin = 2;
      break;
    }
    SetWidth(Margin + ::StripHotkey(Value).GetLength() + Margin);
  }
}

UnicodeString TFarButton::GetData()
{
  UnicodeString Result = TFarDialogItem::GetData();
  if ((FBrackets == brTight) || (FBrackets == brSpace))
  {
    bool HasBrackets = (Result.Length() >= 2) &&
      (Result[1] == ((FBrackets == brSpace) ? L' ' : L'[')) &&
      (Result[Result.Length()] == ((FBrackets == brSpace) ? L' ' : L']'));
    assert(HasBrackets);
    if (HasBrackets)
    {
      Result = Result.SubString(2, Result.Length() - 2);
    }
  }
  return Result;
}

void TFarButton::SetDefault(bool Value)
{
  if (GetDefault() != Value)
  {
    assert(!GetDialog()->GetHandle());
    SetFlag(DIF_DEFAULTBUTTON, Value);
    if (Value)
    {
      if (GetDialog()->GetDefaultButton() && (GetDialog()->GetDefaultButton() != this))
      {
        GetDialog()->GetDefaultButton()->SetDefault(false);
      }
      GetDialog()->FDefaultButton = this;
    }
    else if (GetDialog()->GetDefaultButton() == this)
    {
      GetDialog()->FDefaultButton = nullptr;
    }
    DialogChange();
  }
}

bool TFarButton::GetDefault() const
{
  return GetFlag(DIF_DEFAULTBUTTON);
}

void TFarButton::SetBrackets(TFarButtonBrackets Value)
{
  if (FBrackets != Value)
  {
    UnicodeString Data = GetData();
    SetFlag(DIF_NOBRACKETS, (Value != brNormal));
    FBrackets = Value;
    SetDataInternal(Data);
  }
}

int32_t TFarButton::ItemProc(int32_t Msg, void *Param)
{
  if (Msg == DN_BTNCLICK)
  {
    if (!GetEnabled())
    {
      return 1;
    }
    bool Close = (GetResult() != 0);
    if (FOnClick)
    {
      FOnClick(this, Close);
    }
    if (!Close)
    {
      return 1;
    }
  }
  return TFarDialogItem::ItemProc(Msg, Param);
}

bool TFarButton::HotKey(char HotKey)
{
  UnicodeString Caption = GetCaption();
  int32_t P = Caption.Pos(L'&');
  bool Result =
    GetVisible() && GetEnabled() &&
    (P > 0) && (P < Caption.Length()) &&
    (Caption[P + 1] == HotKey);
  if (Result)
  {
    bool Close = (GetResult() != 0);
    if (FOnClick)
    {
      FOnClick(this, Close);
    }

    if (Close)
    {
      GetDialog()->Close(this);
    }
  }
  return Result;
}

TFarCheckBox::TFarCheckBox(TFarDialog *ADialog) noexcept :
  TFarDialogItem(OBJECT_CLASS_TFarCheckBox, ADialog, DI_CHECKBOX),
  FOnAllowChange(nullptr)
{
}

int32_t TFarCheckBox::ItemProc(int32_t Msg, void *Param)
{
  if (Msg == DN_BTNCLICK)
  {
    bool Allow = true;
    if (FOnAllowChange)
    {
      FOnAllowChange(this, Param, Allow);
    }
    if (Allow)
    {
      UpdateSelected(reinterpret_cast<intptr_t>(Param));
    }
    return nb::ToIntPtr(Allow);
  }
  return TFarDialogItem::ItemProc(Msg, Param);
}

bool TFarCheckBox::GetIsEmpty() const
{
  return GetChecked() != BSTATE_CHECKED;
}

void TFarCheckBox::SetData(const UnicodeString & Value)
{
  TFarDialogItem::SetData(Value);
  if (GetLeft() >= 0 || GetRight() >= 0)
  {
    SetWidth(4 + ::StripHotkey(Value).Length());
  }
}

TFarRadioButton::TFarRadioButton(TFarDialog *ADialog) noexcept :
  TFarDialogItem(OBJECT_CLASS_TFarRadioButton, ADialog, DI_RADIOBUTTON),
  FOnAllowChange(nullptr)
{
}

int32_t TFarRadioButton::ItemProc(int32_t Msg, void *Param)
{
  if (Msg == DN_BTNCLICK)
  {
    bool Allow = true;
    if (FOnAllowChange)
    {
      FOnAllowChange(this, Param, Allow);
    }
    if (Allow)
    {
      // FAR WORKAROUND
      // This does not correspond to FAR API Manual, but it works so.
      // Manual says that Param should contain ID of previously selected dialog item
      UpdateSelected(reinterpret_cast<intptr_t>(Param));
    }
    return nb::ToIntPtr(Allow);
  }
  return TFarDialogItem::ItemProc(Msg, Param);
}

bool TFarRadioButton::GetIsEmpty() const
{
  return !GetChecked();
}

void TFarRadioButton::SetData(const UnicodeString & Value)
{
  TFarDialogItem::SetData(Value);
  if (GetLeft() >= 0 || GetRight() >= 0)
  {
    SetWidth(4 + ::StripHotkey(Value).Length());
  }
}

TFarEdit::TFarEdit(TFarDialog *ADialog) noexcept :
  TFarDialogItem(OBJECT_CLASS_TFarEdit, ADialog, DI_EDIT)
{
  SetAutoSelect(false);
}

void TFarEdit::Detach()
{
  nb_free(GetDialogItem()->Mask);
  nb_free(GetDialogItem()->History);
  TFarDialogItem::Detach();
}

int32_t TFarEdit::ItemProc(int32_t Msg, void *Param)
{
  if (Msg == DN_EDITCHANGE)
  {
    UnicodeString Data = (reinterpret_cast<FarDialogItem *>(Param))->Data;
    nb_free(GetDialogItem()->Data);
    GetDialogItem()->Data = TCustomFarPlugin::DuplicateStr(Data, /*AllowEmpty=*/true);
  }
  return TFarDialogItem::ItemProc(Msg, Param);
}

UnicodeString TFarEdit::GetHistoryMask(size_t Index) const
{
  UnicodeString Result =
    ((Index == 0) && (GetFlags() & DIF_HISTORY)) ||
    ((Index == 1) && (GetFlags() & DIF_MASKEDIT)) ? GetDialogItem()->Mask : L"";
  return Result;
}

void TFarEdit::SetHistoryMask(size_t Index, const UnicodeString & Value)
{
  if (GetHistoryMask(Index) != Value)
  {
    assert(!GetDialog()->GetHandle());
    FarDialogItem * Item = GetDialogItem();
    // assert(&GetDialogItem()->Mask == &GetDialogItem()->History);

    nb_free(Item->Mask);
    nb_free(Item->History);
    if (Value.IsEmpty())
    {
      Item->Mask = nullptr;
      Item->History = nullptr;
    }
    else
    {
      Item->Mask = TCustomFarPlugin::DuplicateStr(Value);
      Item->History = TCustomFarPlugin::DuplicateStr(Value);
    }
    bool PrevHistory = !GetHistory().IsEmpty();
    SetFlag(DIF_HISTORY, (Index == 0) && !Value.IsEmpty());
    bool Masked = (Index == 1) && !Value.IsEmpty();
    SetFlag(DIF_MASKEDIT, Masked);
    if (Masked)
    {
      SetFixed(true);
    }
    bool CurrHistory = !GetHistory().IsEmpty();
    if (PrevHistory != CurrHistory)
    {
      // add/remove space for history arrow
      SetWidth(GetWidth() + (CurrHistory ? -1 : 1));
    }
    DialogChange();
  }
}

void TFarEdit::SetAsInteger(int32_t Value)
{
  int32_t Int = GetAsInteger();
  if (!Int || (Int != Value))
  {
    SetText(::IntToStr(Value));
    DialogChange();
  }
}

int32_t TFarEdit::GetAsInteger() const
{
  return ::StrToIntDef(::Trim(GetText()), 0);
}

TFarSeparator::TFarSeparator(TFarDialog *ADialog) noexcept :
  TFarDialogItem(OBJECT_CLASS_TFarSeparator, ADialog, DI_TEXT)
{
  SetLeft(-1);
  SetFlag(DIF_SEPARATOR, true);
}

void TFarSeparator::ResetBounds()
{
  TFarDialogItem::ResetBounds();
  if (GetBounds().Left < 0)
  {
    GetDialogItem()->X1 = -1;
  }
}

void TFarSeparator::SetDouble(bool Value)
{
  if (GetDouble() != Value)
  {
    assert(!GetDialog()->GetHandle());
    SetFlag(DIF_SEPARATOR, !Value);
    SetFlag(DIF_SEPARATOR2, Value);
  }
}

bool TFarSeparator::GetDouble() const
{
  return GetFlag(DIF_SEPARATOR2);
}

void TFarSeparator::SetPosition(int32_t Value)
{
  TRect R = GetBounds();
  R.Top = nb::ToInt(Value);
  R.Bottom = nb::ToInt(Value);
  SetBounds(R);
}

int32_t TFarSeparator::GetPosition() const
{
  return GetBounds().Top;
}

TFarText::TFarText(TFarDialog *ADialog) noexcept :
  TFarDialogItem(OBJECT_CLASS_TFarText, ADialog, DI_TEXT)
{
}

void TFarText::SetData(const UnicodeString & Value)
{
  TFarDialogItem::SetData(Value);
  if (GetLeft() >= 0 || GetRight() >= 0)
  {
    SetWidth(::StripHotkey(Value).Length());
  }
}

TFarList::TFarList(TFarDialogItem *ADialogItem) noexcept :
  TStringList(OBJECT_CLASS_TFarList),
  FDialogItem(ADialogItem),
  FNoDialogUpdate(false)
{
  assert((ADialogItem == nullptr) ||
    (ADialogItem->GetType() == DI_COMBOBOX) || (ADialogItem->GetType() == DI_LISTBOX));
  FListItems = nb::calloc<FarList *>(1, sizeof(FarList));
  FListItems->StructSize = sizeof(FarList);
}

TFarList::~TFarList() noexcept
{
  for (size_t Index = 0; Index < FListItems->ItemsNumber; ++Index)
  {
    nb_free(FListItems->Items[Index].Text);
  }
  nb_free(FListItems->Items);
  nb_free(FListItems);
}

void TFarList::Assign(const TPersistent *Source)
{
  TStringList::Assign(Source);

  const TFarList *FarList = dyn_cast<TFarList>(Source);
  if (FarList != nullptr)
  {
    for (int32_t Index = 0; Index < FarList->GetCount(); ++Index)
    {
      SetFlags(Index, FarList->GetFlags(Index));
    }
  }
}

void TFarList::UpdateItem(int32_t Index)
{
  FarListItem *ListItem = &FListItems->Items[Index];
  nb_free(ListItem->Text);
  ListItem->Text = TCustomFarPlugin::DuplicateStr(GetString(Index), /*AllowEmpty=*/true);

  FarListUpdate ListUpdate;
  nb::ClearStruct(ListUpdate);
  ListUpdate.StructSize = sizeof(FarListUpdate);
  ListUpdate.Item = *ListItem;
  GetDialogItem()->SendDialogMessage(DM_LISTUPDATE, reinterpret_cast<void *>(&ListUpdate));
}

void TFarList::Put(int32_t Index, const UnicodeString & Str)
{
  if ((GetDialogItem() != nullptr) && GetDialogItem()->GetDialog()->GetHandle())
  {
    FNoDialogUpdate = true;
    SCOPE_EXIT
    {
      FNoDialogUpdate = false;
    };
    TStringList::SetString(Index, Str);
    if (GetUpdateCount() == 0)
    {
      UpdateItem(Index);
    }
  }
  else
  {
    TStringList::SetString(Index, Str);
  }
}

void TFarList::Changed()
{
  TStringList::Changed();

  if ((GetUpdateCount() == 0) && !FNoDialogUpdate)
  {
    int32_t PrevSelected = 0;
    int32_t PrevTopIndex = 0;
    if ((GetDialogItem() != nullptr) && GetDialogItem()->GetDialog()->GetHandle())
    {
      PrevSelected = GetSelected();
      PrevTopIndex = GetTopIndex();
    }
    size_t Count = GetCount();
    if (FListItems->ItemsNumber != Count)
    {
      FarListItem *Items = FListItems->Items;
      int32_t ItemsNumber = FListItems->ItemsNumber;
      if (Count)
      {
        FListItems->Items = nb::calloc<FarListItem *>(Count, sizeof(FarListItem));
        for (size_t Index = 0; Index < Count; ++Index)
        {
          if (Index < FListItems->ItemsNumber)
          {
            FListItems->Items[Index].Flags = Items[Index].Flags;
          }
        }
      }
      else
      {
        FListItems->Items = nullptr;
      }
      for (int32_t Index = 0; Index < ItemsNumber; ++Index)
      {
        nb_free(Items[Index].Text);
      }
      nb_free(Items);
      FListItems->ItemsNumber = nb::ToInt(GetCount());
    }
    for (int32_t Index = 0; Index < GetCount(); ++Index)
    {
      FListItems->Items[Index].Text = TCustomFarPlugin::DuplicateStr(GetString(Index), /*AllowEmpty=*/true);
    }
    if ((GetDialogItem() != nullptr) && GetDialogItem()->GetDialog()->GetHandle())
    {
      GetDialogItem()->GetDialog()->LockChanges();
      SCOPE_EXIT
      {
        GetDialogItem()->GetDialog()->UnlockChanges();
      };
      GetDialogItem()->SendDialogMessage(DM_LISTSET, reinterpret_cast<void *>(FListItems));
      if (PrevTopIndex + GetDialogItem()->GetHeight() > GetCount())
      {
        PrevTopIndex = GetCount() > GetDialogItem()->GetHeight() ? GetCount() - GetDialogItem()->GetHeight() : 0;
      }
      SetCurPos((PrevSelected >= GetCount()) ? (GetCount() - 1) : PrevSelected,
        PrevTopIndex);
    }
  }
}

void TFarList::SetSelected(int32_t Value)
{
  TFarDialogItem *DialogItem = GetDialogItem();
  assert(DialogItem != nullptr);
  if (GetSelectedInt(false) != Value)
  {
    if (DialogItem->GetDialog()->GetHandle())
    {
      UpdatePosition(Value);
    }
    else
    {
      DialogItem->SetData(GetString(Value));
    }
  }
}

void TFarList::UpdatePosition(int32_t Position)
{
  if (Position >= 0)
  {
    int32_t ATopIndex = GetTopIndex();
    // even if new position is visible already, FAR will scroll the view so
    // that the new selected item is the last one, following prevents the scroll
    if ((ATopIndex <= Position) && (Position < ATopIndex + GetVisibleCount()))
    {
      SetCurPos(Position, ATopIndex);
    }
    else
    {
      SetCurPos(Position, -1);
    }
  }
}

void TFarList::SetCurPos(int32_t Position, int32_t TopIndex)
{
  TFarDialogItem *DialogItem = GetDialogItem();
  assert(DialogItem != nullptr);
  TFarDialog *Dlg = DialogItem->GetDialog();
  assert(Dlg);
  assert(Dlg->GetHandle());
  DebugUsedParam(Dlg);
  FarListPos ListPos;
  ListPos.StructSize = sizeof(FarListPos);
  ListPos.SelectPos = Position;
  ListPos.TopPos = TopIndex;
  DialogItem->SendDialogMessage(DM_LISTSETCURPOS, reinterpret_cast<void *>(&ListPos));
}

void TFarList::SetTopIndex(int32_t Value)
{
  if (Value != GetTopIndex())
  {
    SetCurPos(nb::NPOS, Value);
  }
}

int32_t TFarList::GetPosition() const
{
  TFarDialogItem *DialogItem = GetDialogItem();
  assert(DialogItem != nullptr);
  return DialogItem->SendDialogMessage(DM_LISTGETCURPOS, nullptr);
}

int32_t TFarList::GetTopIndex() const
{
  int32_t Result = -1;
  if (GetCount() != 0)
  {
    FarListPos ListPos;
    nb::ClearStruct(ListPos);
    ListPos.StructSize = sizeof(FarListPos);
    TFarDialogItem *DialogItem = GetDialogItem();
    assert(DialogItem != nullptr);
    DialogItem->SendDialogMessage(DM_LISTGETCURPOS, reinterpret_cast<void *>(&ListPos));
    Result = nb::ToIntPtr(ListPos.TopPos);
  }
  return Result;
}

int32_t TFarList::GetMaxLength() const
{
  int32_t Result = 0;
  for (int32_t Index = 0; Index < GetCount(); ++Index)
  {
    if (Result < GetString(Index).Length())
    {
      Result = GetString(Index).Length();
    }
  }
  return Result;
}

int32_t TFarList::GetVisibleCount() const
{
  TFarDialogItem *DialogItem = GetDialogItem();
  assert(DialogItem != nullptr);
  return DialogItem ? DialogItem->GetHeight() - (GetDialogItem()->GetFlag(DIF_LISTNOBOX) ? 0 : 2) : 0;
}

int32_t TFarList::GetSelectedInt(bool Init) const
{
  int32_t Result = nb::NPOS;
  TFarDialogItem *DialogItem = GetDialogItem();
  assert(DialogItem != nullptr);
  if (GetCount() == 0)
  {
    Result = nb::NPOS;
  }
  else if (DialogItem->GetDialog()->GetHandle() && !Init)
  {
    Result = GetPosition();
  }
  else
  {
    const wchar_t *Data = DialogItem->GetDialogItem()->Data;
    if (Data)
    {
      Result = IndexOf(Data);
    }
  }

  return Result;
}

int32_t TFarList::GetSelected() const
{
  int32_t Result = GetSelectedInt(false);

  if ((Result == nb::NPOS) && (GetCount() > 0))
  {
    Result = 0;
  }

  return Result;
}

LISTITEMFLAGS TFarList::GetFlags(int32_t Index) const
{
  return FListItems->Items[Index].Flags;
}

void TFarList::SetFlags(int32_t Index, LISTITEMFLAGS Value)
{
  if (FListItems->Items[Index].Flags != Value)
  {
    FListItems->Items[Index].Flags = Value;
    if ((GetDialogItem() != nullptr) && GetDialogItem()->GetDialog()->GetHandle() && (GetUpdateCount() == 0))
    {
      UpdateItem(Index);
    }
  }
}

bool TFarList::GetFlag(int32_t Index, LISTITEMFLAGS Flag) const
{
  return FLAGSET(GetFlags(Index), Flag);
}

void TFarList::SetFlag(int32_t Index, LISTITEMFLAGS Flag, bool Value)
{
  SetFlags(Index, (GetFlags(Index) & ~Flag) | FLAGMASK(Value, Flag));
}

void TFarList::Init()
{
  UpdatePosition(GetSelectedInt(true));
}

int32_t TFarList::ItemProc(int32_t Msg, void *Param)
{
  TFarDialogItem *DialogItem = GetDialogItem();
  assert(DialogItem != nullptr);
  if (Msg == DN_LISTCHANGE)
  {
    if ((Param == nullptr) && (GetCount() == 0))
    {
      DialogItem->UpdateData(L"");
    }
    else
    {
      int32_t param = reinterpret_cast<intptr_t>(Param);
      assert(param >= 0 && param < GetCount());
      DialogItem->UpdateData(GetString(param));
    }
  }
  return 0;
}

TFarListBox::TFarListBox(TFarDialog *ADialog) noexcept :
  TFarDialogItem(OBJECT_CLASS_TFarListBox, ADialog, DI_LISTBOX),
  FAutoSelect(asOnlyFocus),
  FDenyClose(false)
{
  FList = std::make_unique<TFarList>(this);
  GetDialogItem()->ListItems = FList->GetListItems();
}

TFarListBox::~TFarListBox() noexcept
{
//  SAFE_DESTROY(FList);
}

int32_t TFarListBox::ItemProc(int32_t Msg, void *Param)
{
  int32_t Result = 0;
  if (Msg == DN_CONTROLINPUT)
  {
    const INPUT_RECORD *Rec = static_cast<const INPUT_RECORD *>(Param);
    const KEY_EVENT_RECORD &Event = Rec->Event.KeyEvent;
    if (GetDialog()->HotKey(Event.wVirtualKeyCode, Event.dwControlKeyState))
    {
      Result = 1;
    }
  }
  else if (FList->ItemProc(Msg, Param))
  {
    Result = 1;
  }
  else
  {
    Result = TFarDialogItem::ItemProc(Msg, Param);
  }
  return Result;
}

void TFarListBox::Init()
{
  TFarDialogItem::Init();
  GetItems()->Init();
  UpdateMouseReaction();
}

void TFarListBox::SetAutoSelect(TFarListBoxAutoSelect Value)
{
  if (GetAutoSelect() != Value)
  {
    FAutoSelect = Value;
    if (GetDialog()->GetHandle())
    {
      UpdateMouseReaction();
    }
  }
}

void TFarListBox::UpdateMouseReaction()
{
  SendDialogMessage(DIF_LISTTRACKMOUSE, reinterpret_cast<void *>(GetAutoSelect()));
}

void TFarListBox::SetItems(TStrings *Value)
{
  FList->Assign(Value);
}

void TFarListBox::SetList(TFarList *Value)
{
  SetItems(Value);
}

bool TFarListBox::CloseQuery()
{
  return true;
}

TFarComboBox::TFarComboBox(TFarDialog *ADialog) noexcept :
  TFarDialogItem(OBJECT_CLASS_TFarComboBox, ADialog, DI_COMBOBOX),
  FList(std::make_unique<TFarList>(this))
{
  GetDialogItem()->ListItems = FList->GetListItems();
  SetAutoSelect(false);
}

TFarComboBox::~TFarComboBox() noexcept
{
//  SAFE_DESTROY(FList);
}

void TFarComboBox::ResizeToFitContent()
{
  SetWidth(FList->GetMaxLength());
}

int32_t TFarComboBox::ItemProc(int32_t Msg, void *Param)
{
  if (Msg == DN_EDITCHANGE)
  {
    UnicodeString Data = (reinterpret_cast<FarDialogItem *>(Param))->Data;
    nb_free(GetDialogItem()->Data);
    GetDialogItem()->Data = TCustomFarPlugin::DuplicateStr(Data, /*AllowEmpty=*/true);
  }

  if (FList->ItemProc(Msg, Param))
  {
    return 1;
  }
  return TFarDialogItem::ItemProc(Msg, Param);
}

void TFarComboBox::Init()
{
  TFarDialogItem::Init();
  GetItems()->Init();
}

TFarLister::TFarLister(TFarDialog *ADialog) noexcept :
  TFarDialogItem(OBJECT_CLASS_TFarLister, ADialog, DI_USERCONTROL),
  FItems(std::make_unique<TStringList>()),
  FTopIndex(0)
{
  FItems->SetOnChange(nb::bind(&TFarLister::ItemsChange, this));
}

TFarLister::~TFarLister() noexcept
{
//  SAFE_DESTROY(FItems);
}

void TFarLister::ItemsChange(TObject * /*Sender*/)
{
  FTopIndex = 0;
  if (GetDialog()->GetHandle())
  {
    Redraw();
  }
}

bool TFarLister::GetScrollBar() const
{
  return (GetItems()->GetCount() > GetHeight());
}

void TFarLister::SetTopIndex(int32_t Value)
{
  if (GetTopIndex() != Value)
  {
    FTopIndex = Value;
    Redraw();
  }
}

TStrings *TFarLister::GetItems() const
{
  return FItems.get();
}

void TFarLister::SetItems(const TStrings *Value)
{
  if (!FItems->Equals(Value))
  {
    FItems->Assign(Value);
  }
}

void TFarLister::DoFocus()
{
  TFarDialogItem::DoFocus();
  TODO("hide cursor");
}

int32_t TFarLister::ItemProc(int32_t Msg, void *Param)
{
  int32_t Result = 0;

  if (Msg == DN_DRAWDLGITEM)
  {
    bool AScrollBar = GetScrollBar();
    int32_t ScrollBarPos = 0;
    if (GetItems()->GetCount() > GetHeight())
    {
      ScrollBarPos = nb::ToIntPtr((static_cast<float>(GetHeight() - 3) * (static_cast<float>(FTopIndex) / (GetItems()->GetCount() - GetHeight())))) + 1;
    }
    int32_t DisplayWidth = GetWidth() - (AScrollBar ? 1 : 0);
    FarColor Color = GetDialog()->GetSystemColor(
        FLAGSET(GetDialog()->GetFlags(), FDLG_WARNING) ? COL_WARNDIALOGLISTTEXT : COL_DIALOGLISTTEXT);
    for (int32_t Row = 0; Row < GetHeight(); Row++)
    {
      int32_t Index = GetTopIndex() + Row;
      UnicodeString Buf = L" ";
      if (Index < GetItems()->GetCount())
      {
        UnicodeString Value = GetItems()->GetString(Index).SubString(1, DisplayWidth - 1);
        Buf += Value;
      }
      UnicodeString Value = ::StringOfChar(' ', DisplayWidth - Buf.Length());
      Value.SetLength(DisplayWidth - Buf.Length());
      Buf += Value;
      if (AScrollBar)
      {
        if (Row == 0)
        {
          Buf += static_cast<wchar_t>(0x25B2); // ucUpScroll
        }
        else if (Row == ScrollBarPos)
        {
          Buf += static_cast<wchar_t>(0x2592); // ucBox50
        }
        else if (Row == GetHeight() - 1)
        {
          Buf += static_cast<wchar_t>(0x25BC); // ucDnScroll
        }
        else
        {
          Buf += static_cast<wchar_t>(0x2591); // ucBox25
        }
      }
      Text(0, Row, Color, Buf);
    }
  }
  else if (Msg == DN_CONTROLINPUT)
  {
    Result = 1;
    INPUT_RECORD *Rec = reinterpret_cast<INPUT_RECORD *>(Param);
    if (Rec->EventType == KEY_EVENT)
    {
      KEY_EVENT_RECORD *KeyEvent = &Rec->Event.KeyEvent;

      int32_t NewTopIndex = GetTopIndex();

      WORD Key = KeyEvent->wVirtualKeyCode;
      if ((Key == VK_UP) || (Key == VK_LEFT))
      {
        if (NewTopIndex > 0)
        {
          --NewTopIndex;
        }
        else
        {
          INPUT_RECORD Rec = {0};
          Rec.EventType = KEY_EVENT;
          Rec.Event.KeyEvent.wVirtualKeyCode = VK_TAB;
          Rec.Event.KeyEvent.dwControlKeyState = SHIFT_PRESSED;
          SendDialogMessage(DN_CONTROLINPUT, 1, nb::ToPtr(&Rec));
        }
      }
      else if ((Key == VK_DOWN) || (Key == VK_RIGHT))
      {
        if (NewTopIndex < GetItems()->GetCount() - GetHeight())
        {
          ++NewTopIndex;
        }
        else
        {
          INPUT_RECORD Rec = {0};
          Rec.EventType = KEY_EVENT;
          Rec.Event.KeyEvent.wVirtualKeyCode = VK_TAB;
          Rec.Event.KeyEvent.dwControlKeyState = 0;
          SendDialogMessage(DN_CONTROLINPUT, 1, nb::ToPtr(&Rec));
        }
      }
      else if (Key == VK_PRIOR)
      {
        if (NewTopIndex > GetHeight() - 1)
        {
          NewTopIndex -= GetHeight() - 1;
        }
        else
        {
          NewTopIndex = 0;
        }
      }
      else if (Key == VK_NEXT)
      {
        if (NewTopIndex < GetItems()->GetCount() - GetHeight() - GetHeight() + 1)
        {
          NewTopIndex += GetHeight() - 1;
        }
        else
        {
          NewTopIndex = GetItems()->GetCount() - GetHeight();
        }
      }
      else if (Key == VK_HOME)
      {
        NewTopIndex = 0;
      }
      else if (Key == VK_END)
      {
        NewTopIndex = GetItems()->GetCount() - GetHeight();
      }
      else
      {
        Result = TFarDialogItem::ItemProc(Msg, Param);
      }

      SetTopIndex(NewTopIndex);
    }
    else if (Rec->EventType == MOUSE_EVENT)
    {
      if (!Focused() && CanFocus())
      {
        SetFocus();
      }

      MOUSE_EVENT_RECORD *Event = &Rec->Event.MouseEvent;
      TPoint P = MouseClientPosition(Event);

      if (FLAGSET(Event->dwEventFlags, DOUBLE_CLICK) &&
        (P.x < GetWidth() - 1))
      {
        Result = TFarDialogItem::ItemProc(Msg, Param);
      }
      else
      {
        int32_t NewTopIndex = GetTopIndex();

        if (((P.x == nb::ToInt(GetWidth()) - 1) && (P.y == 0)) ||
          ((P.x < nb::ToInt(GetWidth() - 1)) && (P.y < nb::ToInt(GetHeight() / 2))))
        {
          if (NewTopIndex > 0)
          {
            --NewTopIndex;
          }
        }
        else if (((P.x == GetWidth() - 1) && (P.y == nb::ToInt(GetHeight() - 1))) ||
          ((P.x < GetWidth() - 1) && (P.y >= nb::ToInt(GetHeight() / 2))))
        {
          if (NewTopIndex < GetItems()->GetCount() - GetHeight())
          {
            ++NewTopIndex;
          }
        }
        else
        {
          assert(P.x == GetWidth() - 1);
          assert((P.y > 0) && (P.y < nb::ToInt(GetHeight() - 1)));
          NewTopIndex = nb::ToInt32(ceil(static_cast<float>(P.y - 1) / (GetHeight() - 2) * (GetItems()->GetCount() - GetHeight() + 1)));
        }

        Result = 1;

        SetTopIndex(NewTopIndex);
      }
    }
  }
  else
  {
    Result = TFarDialogItem::ItemProc(Msg, Param);
  }

  return Result;
}
