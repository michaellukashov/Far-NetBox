#include <vcl.h>
#pragma hdrstop

#include <Common.h>

#include "FarDialog.h"
#include <plugin.hpp>
#pragma warning(push, 1)
#include <farcolor.hpp>
#pragma warning(pop)
#include <Queue.h> // TODO: move TSimpleThread to Sysutils

inline TRect Rect(int32_t Left, int32_t Top, int32_t Right, int32_t Bottom)
{
  return TRect(Left, Top, Right, Bottom);
}

class TFarDialogIdleThread final : public TSimpleThread
{
public:
  TFarDialogIdleThread() = delete;
  explicit TFarDialogIdleThread(gsl::not_null<TFarDialog *> Dialog, DWORD Millisecs) noexcept :
    TSimpleThread(OBJECT_CLASS_TDialogIdleThread),
    FDialog(Dialog),
    FMillisecs(Millisecs)
  {}

  virtual ~TFarDialogIdleThread() noexcept override = default;

  void InitIdleThread()
  {
    TSimpleThread::InitSimpleThread("NetBox Dialog Idle Thread");
    FEvent = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
    Start();
  }

  virtual void Execute() override
  {
    while (!IsFinished())
    {
      if (::WaitForSingleObject(FEvent, FMillisecs) != WAIT_FAILED)
      {
        if (!IsFinished() && FDialog && FDialog->GetHandle())
          FDialog->Idle();
      }
    }
    if (!IsFinished())
      SAFE_CLOSE_HANDLE(FEvent);
  }

  virtual void Terminate() override
  {
    FFinished = true;
    TriggerEvent();
  }

  virtual void Close() override
  {
    TSimpleThread::Close();
    SAFE_CLOSE_HANDLE(FEvent);
  }

private:
  void TriggerEvent()
  {
    if (CheckHandle(FEvent))
    {
      ::SetEvent(FEvent);
    }
  }

  gsl::not_null<TFarDialog *> FDialog;
  DWORD FMillisecs{0};
  HANDLE FEvent{INVALID_HANDLE_VALUE};
};

TFarDialog::TFarDialog(gsl::not_null<TCustomFarPlugin *> AFarPlugin) noexcept :
  TObject(OBJECT_CLASS_TFarDialog),
  FFarPlugin(AFarPlugin)
{
}

TFarDialog::~TFarDialog() noexcept
{
  FTIdleThread->Close();
  for (int32_t Index = 0; Index < GetItemCount(); ++Index)
  {
    TFarDialogItem * Item = GetItem(Index);
    Item->Detach();
    // TODO: SAFE_DESTROY(Item);
  }
//  SAFE_DESTROY(FItems);
  nb_free(FDialogItems);
  FDialogItemsCapacity = 0;
//  SAFE_DESTROY(FContainers);
  SAFE_CLOSE_HANDLE(FSynchronizeObjects[0]);
  SAFE_CLOSE_HANDLE(FSynchronizeObjects[1]);
  FHandle = nullptr;
}

void TFarDialog::InitDialog()
{
  FSynchronizeObjects[0] = INVALID_HANDLE_VALUE;
  FSynchronizeObjects[1] = INVALID_HANDLE_VALUE;

  FBorderBox = MakeOwnedObject<TFarBox>(this);
  FBorderBox->SetBounds(TRect(3, 1, -4, -2));
  FBorderBox->SetDouble(true);
  FTIdleThread = std::make_unique<TFarDialogIdleThread>(this, 400);
  FTIdleThread->InitIdleThread();
}

void TFarDialog::SetBounds(const TRect & Value)
{
  if (GetBounds() != Value)
  {
    LockChanges();
    try__finally
    {
      FBounds = Value;
      if (GetHandle())
      {
        COORD Coord{};
        Coord.X = static_cast<int16_t>(GetSize().x);
        Coord.Y = static_cast<int16_t>(GetSize().y);
        SendDlgMessage(DM_RESIZEDIALOG, 0, nb::ToPtr(&Coord));
        Coord.X = static_cast<int16_t>(FBounds.Left);
        Coord.Y = static_cast<int16_t>(FBounds.Top);
        SendDlgMessage(DM_MOVEDIALOG, nb::ToInt32(true), nb::ToPtr(&Coord));
      }
      for (int32_t Index = 0; Index < GetItemCount(); ++Index)
      {
        GetItem(Index)->DialogResized();
      }
    }
    __finally
    {
      UnlockChanges();
    } end_try__finally
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
    const TRect R = FBorderBox->GetActualBounds();
    S.x = nb::ToInt32(R.Width()) + 1;
    S.y = nb::ToInt32(R.Height()) + 1;
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
    DebugAssert(!GetHandle());
    FHelpTopic = Value;
  }
}

void TFarDialog::SetFlags(const FARDIALOGITEMFLAGS Value)
{
  if (GetFlags() != Value)
  {
    DebugAssert(!GetHandle());
    FFlags = Value;
  }
}

void TFarDialog::SetCentered(bool Value)
{
  if (GetCentered() != Value)
  {
    DebugAssert(!GetHandle());
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
    return TPoint(nb::ToInt32(GetBounds().Right), nb::ToInt32(GetBounds().Bottom));
  }
  return TPoint(nb::ToInt32(GetBounds().Width() + 1), nb::ToInt32(GetBounds().Height() + 1));
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
  SetSize(TPoint(nb::ToInt32(Value), nb::ToInt32(GetHeight())));
}

int32_t TFarDialog::GetWidth() const
{
  return GetSize().x;
}

void TFarDialog::SetHeight(int32_t Value)
{
  SetSize(TPoint(nb::ToInt32(GetWidth()), nb::ToInt32(Value)));
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

int32_t TFarDialog::GetItemIdx(const TFarDialogItem * Item) const
{
  if (!Item)
    return -1;
  return Item->GetItemIdx();
}

TFarDialogItem * TFarDialog::GetItem(int32_t Index) const
{
  TFarDialogItem * DialogItem = nullptr;
  if (GetItemCount())
  {
    DebugAssert(Index >= 0 && Index < FItems->GetCount());
    DialogItem = FItems->GetAs<TFarDialogItem>(Index);
    DebugAssert(DialogItem);
  }
  return DialogItem;
}

void TFarDialog::Add(TFarDialogItem * DialogItem)
{
  TRect R = GetClientRect();
  int32_t Left, Top;
  GetNextItemPosition(Left, Top);
  R.Left = nb::ToInt32(Left);
  R.Top = nb::ToInt32(Top);

  if (FDialogItemsCapacity == GetItems()->GetCount())
  {
    constexpr int32_t DialogItemsDelta = 10;
    FarDialogItem * NewDialogItems{nullptr};
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

  DebugAssert(DialogItem);
  DialogItem->SetItemIdx(GetItems()->Add(DialogItem));

  R.Bottom = R.Top;
  DialogItem->SetBounds(R);
  DialogItem->SetGroup(GetDefaultGroup());
}

void TFarDialog::Add(TFarDialogContainer * Container)
{
  FContainers->Add(Container);
}

void TFarDialog::GetNextItemPosition(int32_t & Left, int32_t & Top)
{
  const TRect R = GetClientRect();
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

intptr_t WINAPI TFarDialog::DialogProcGeneral(HANDLE Handle, intptr_t Msg, intptr_t Param1, void * Param2)
{
  const TFarPluginEnvGuard Guard;

  static nb::map_t<HANDLE, void *> Dialogs;
  TFarDialog * Dialog = nullptr;
  intptr_t Result = 0;
  if (Msg == DN_INITDIALOG)
  {
    DebugAssert(Dialogs.find(Handle) == Dialogs.end());
    Dialogs[Handle] = Param2;
    Dialog = static_cast<TFarDialog *>(Param2);
    Dialog->FHandle = Handle;
  }
  else
  {
    if (Dialogs.find(Handle) == Dialogs.end())
    {
      // DM_CLOSE is sent after DN_CLOSE, if the dialog was closed programmatically
      // by SendMessage(DM_CLOSE, ...)
      DebugAssert(Msg == DM_CLOSE);
      Result = static_cast<intptr_t>(0);
    }
    else
    {
      Dialog = static_cast<TFarDialog *>(Dialogs[Handle]);
    }
  }

  if (Dialog != nullptr)
  {
    Result = Dialog->DialogProc(Msg, Param1, Param2);
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

intptr_t TFarDialog::DialogProc(intptr_t Msg, intptr_t Param1, void * Param2)
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
        DebugAssert(false);
      }
    }

    bool Changed = false;

    // DEBUG_PRINTF("Msg: %d", Msg);
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
        TFarDialogItem * Item = GetItem(nb::ToInt32(Param1));
        try
        {
          Result = Item->ItemProc(Msg, Param2);
        }
        catch(Exception & E)
        {
          Handled = true;
          DEBUG_PRINTF("before GetFarPlugin()->HandleException");
          GetFarPlugin()->HandleException(&E);
          Result = Item->FailItemProc(Msg, Param2);
        }

        if (!Result && (Msg == DN_CONTROLINPUT))
        {
          INPUT_RECORD * Rec = static_cast<INPUT_RECORD *>(Param2);
          const KEY_EVENT_RECORD & Event = Rec->Event.KeyEvent;
          Result = Key(Item, static_cast<intptr_t>(Event.wVirtualKeyCode | (Event.dwControlKeyState << 16)));
        }
        Handled = true;
      }

      // FAR WORKAROUND
      // When pressing Enter FAR forces dialog to close without calling
      // DN_BTNCLICK on default button. This fixes the scenario.
      // (first check if focused dialog item is not another button)
      if (!Result && (Msg == DN_CONTROLINPUT) &&
        (nb::ToIntPtr(Param2) == VK_RETURN) &&
        ((Param1 < 0) ||
          !rtti::isa<TFarButton>(GetItem(nb::ToInt32(Param1)))) &&
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
      Result = MouseEvent(static_cast<MOUSE_EVENT_RECORD *>(Param2));
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
          TFarButton * Button = rtti::dyn_cast_or_null<TFarButton>(GetItem(nb::ToInt32(Param1)));
          // FAR WORKAROUND
          // FAR 1.70 alpha 6 calls DN_CLOSE even for non-button dialog items
          // (list boxes in particular), while FAR 1.70 beta 5 used ID of
          // default button in such case.
          // Particularly for listbox, we can prevent closing dialog using
          // flag DIF_LISTNOCLOSE.
          if (Button == nullptr)
          {
            DebugAssert(rtti::isa<TFarListBox>(GetItem(nb::ToInt32(Param1))));
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

      // case DN_ENTERIDLE:
      //   Idle();
      //   break;
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
  catch(Exception & E)
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

intptr_t TFarDialog::DefaultDialogProc(intptr_t Msg, intptr_t Param1, void * Param2)
{
  if (GetHandle())
  {
    const TFarEnvGuard Guard;
    return GetFarPlugin()->GetPluginStartupInfo()->DefDlgProc(GetHandle(), Msg, nb::ToInt32(Param1), Param2);
  }
  return 0;
}

intptr_t TFarDialog::FailDialogProc(intptr_t Msg, intptr_t Param1, void * Param2)
{
  intptr_t Result;
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

bool TFarDialog::MouseEvent(MOUSE_EVENT_RECORD * Event)
{
  bool Result = true;
  bool Handled = false;
  if (FLAGSET(Event->dwEventFlags, MOUSE_MOVED))
  {
    int32_t X = Event->dwMousePosition.X - GetBounds().Left;
    int32_t Y = Event->dwMousePosition.Y - GetBounds().Top;
    TFarDialogItem * Item = ItemAt(X, Y);
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
    INPUT_RECORD Rec{0};
    Rec.EventType = MOUSE_EVENT;
    memmove(&Rec.Event.MouseEvent, Event, sizeof(*Event));
    Result = DefaultDialogProc(DN_INPUT, 0, nb::ToPtr(&Rec)) != 0;
  }

  return Result;
}

bool TFarDialog::Key(TFarDialogItem * Item, intptr_t KeyCode)
{
  bool Result = false;
  if (FOnKey)
  {
    FOnKey(this, Item, nb::ToInt32(KeyCode), Result);
  }
  return Result;
}

bool TFarDialog::HotKey(uint32_t Key, uint32_t ControlState) const
{
  bool Result = false;
  char HotKey = 0;
  if (CheckControlMaskSet(ControlState, ALTMASK) &&
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

TFarDialogItem * TFarDialog::ItemAt(int32_t X, int32_t Y) const
{
  TFarDialogItem * Result = nullptr;
  for (int32_t Index = 0; Index < GetItemCount(); ++Index)
  {
    const TRect Bounds = GetItem(Index)->GetActualBounds();
    if ((Bounds.Left <= nb::ToInt32(X)) && (nb::ToInt32(X) <= Bounds.Right) &&
      (Bounds.Top <= nb::ToInt32(Y)) && (nb::ToInt32(Y) <= Bounds.Bottom))
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
  SMALL_RECT Rect{};
  SendDlgMessage(DM_GETDLGRECT, 0, nb::ToPtr(&Rect));
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
  Expects(GetFarPlugin());
  FResult = -1;
  TFarDialog * PrevTopDialog = GetFarPlugin()->FTopDialog;
  GetFarPlugin()->FTopDialog = this;
  HANDLE Handle = INVALID_HANDLE_VALUE;
  const PluginStartupInfo & Info = *GetFarPlugin()->GetPluginStartupInfo();
  try__finally
  {
    DebugAssert(GetDefaultButton());
    DebugAssert(GetDefaultButton()->GetDefault());

    const UnicodeString HelpTopic = GetHelpTopic();
    intptr_t BResult{0};

    {
      const TFarEnvGuard Guard;
      const TRect Bounds = GetBounds();
      Handle = Info.DialogInit(
        &NetBoxPluginGuid, GetDialogGuid(),
        Bounds.Left, Bounds.Top, Bounds.Right, Bounds.Bottom,
        HelpTopic.c_str(), FDialogItems,
        GetItemCount(), 0, GetFlags(),
        TFarDialog::DialogProcGeneral,
        nb::ToPtr(this));
      BResult = CheckHandle(Handle) ? Info.DialogRun(Handle) : -1;
    }

    if (BResult >= 0)
    {
      const TFarButton * Button = rtti::dyn_cast_or_null<TFarButton>(GetItem(nb::ToInt32(BResult)));
      DebugAssert(Button);
      if (Button)
      {
        // correct result should be already set by TFarButton
        DebugAssert(FResult == Button->GetResult());
        FResult = Button->GetResult();
      }
    }
    else
    {
      // allow only one negative value = -1
      FResult = -1;
    }
  }
  __finally
  {
    GetFarPlugin()->FTopDialog = PrevTopDialog;
    if (CheckHandle(Handle))
    {
      Info.DialogFree(Handle);
    }
  } end_try__finally

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

void TFarDialog::Close(const TFarButton * Button)
{
  DebugAssert(Button != nullptr);
  SendDlgMessage(DM_CLOSE, Button->GetItemIdx(), nullptr);
}

void TFarDialog::Change()
{
  if (FChangesLocked > 0)
  {
    FChangesPending = true;
  }
  else
  {
    std::unique_ptr<TListBase<TFarDialogContainer>> NotifiedContainers(std::make_unique<TListBase<TFarDialogContainer>>());
    for (int32_t Index = 0; Index < GetItemCount(); ++Index)
    {
      TFarDialogItem * DItem = GetItem(Index);
      DItem->Change();
      if (DItem->GetContainer() && NotifiedContainers->IndexOf(DItem->GetContainer()) == nb::NPOS)
      {
        NotifiedContainers->Add(DItem->GetContainer());
      }
    }

    for (int32_t Index = 0; Index < NotifiedContainers->GetCount(); ++Index)
    {
      NotifiedContainers->GetAs<TFarDialogContainer>(Index)->Changed();
    }
  }
}

intptr_t TFarDialog::SendDlgMessage(intptr_t Msg, intptr_t Param1, void * Param2)
{
  if (GetHandle())
  {
    const TFarEnvGuard Guard;
    return GetFarPlugin()->GetPluginStartupInfo()->SendDlgMessage(GetHandle(),
      Msg, Param1, Param2);
  }
  return 0;
}

FarColor TFarDialog::GetSystemColor(PaletteColors colorId)
{
  FarColor color{};
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

void TFarDialog::ProcessGroup(int32_t Group, TFarProcessGroupEvent && Callback,
  void *Arg)
{
  LockChanges();
  try__finally
  {
    for (int32_t Index = 0; Index < GetItemCount(); ++Index)
    {
      TFarDialogItem * Item = GetItem(Index);
      if (Item->GetGroup() == Group)
      {
        Callback(Item, Arg);
      }
    }
  }
  __finally
  {
    UnlockChanges();
  } end_try__finally
}

void TFarDialog::ShowItem(TFarDialogItem * Item, void * Arg)
{
  Item->SetVisible(*static_cast<bool *>(Arg));
}

void TFarDialog::EnableItem(TFarDialogItem * Item, void * Arg)
{
  Item->SetEnabled(*static_cast<bool *>(Arg));
}

void TFarDialog::SetItemFocused(TFarDialogItem * Value)
{
  if ((Value != GetItemFocused()) && Value)
  {
    DebugAssert(Value);
    Value->SetFocus();
  }
}

UnicodeString TFarDialog::GetMsg(intptr_t MsgId) const
{
  return FFarPlugin->GetMsg(MsgId);
}

void TFarDialog::LockChanges()
{
  DebugAssert(FChangesLocked < 10);
  FChangesLocked++;
  if (FChangesLocked == 1)
  {
    DebugAssert(!FChangesPending);
    if (GetHandle())
    {
      SendDlgMessage(DM_ENABLEREDRAW, 0, nullptr);
    }
  }
}

void TFarDialog::UnlockChanges()
{
  DebugAssert(FChangesLocked > 0);
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

TFarDialogContainer::TFarDialogContainer(TObjectClassId Kind, TFarDialog * ADialog) noexcept :
  TObject(Kind),
  FDialog(ADialog)
{
  DebugAssert(ADialog);
  FItems->SetOwnsObjects(false);
  GetDialog()->Add(this);
  GetDialog()->GetNextItemPosition(FLeft, FTop);
}

TFarDialogContainer::~TFarDialogContainer() noexcept = default;

UnicodeString TFarDialogContainer::GetMsg(intptr_t MsgId) const
{
  return GetDialog()->GetMsg(MsgId);
}

void TFarDialogContainer::Add(TFarDialogItem * Item)
{
  DebugAssert(FItems->IndexOf(Item) == nb::NPOS);
  Item->SetContainer(this);
  if (FItems->IndexOf(Item) == nb::NPOS)
    FItems->Add(Item);
}

void TFarDialogContainer::Remove(TFarDialogItem * Item)
{
  DebugAssert(FItems->IndexOf(Item) != nb::NPOS);
  Item->SetContainer(nullptr);
  FItems->Remove(Item);
  if (FItems->GetCount() == 0)
  {
    std::destroy_at(this);
  }
}

void TFarDialogContainer::SetPosition(int32_t AIndex, int32_t Value)
{
  int32_t & Position = AIndex ? FTop : FLeft;
  if (Position != Value)
  {
    Position = Value;
    for (int32_t Index = 0; Index < GetItemCount(); ++Index)
    {
      FItems->GetAs<TFarDialogItem>(Index)->DialogResized();
    }
  }
}

void TFarDialogContainer::Changed()
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

TFarDialogItem::TFarDialogItem(TObjectClassId Kind, TFarDialog * ADialog, FARDIALOGITEMTYPES AType) :
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
  FItemIdx(nb::NPOS),
  FColors(0),
  FColorMask(0),
  FEnabled(true),
  FIsEnabled(true)
{
  DebugAssert(ADialog);
  GetDialog()->Add(this);

  GetDialogItem()->Type = AType;
}

TFarDialogItem::~TFarDialogItem() noexcept
{
  TFarDialog * Dlg = GetDialog();
  DebugAssert(!Dlg);
  if (Dlg)
  {
    nb_free(GetDialogItem()->Data);
  }
}

const FarDialogItem * TFarDialogItem::GetDialogItem() const
{
  return const_cast<TFarDialogItem *>(this)->GetDialogItem();
}

FarDialogItem * TFarDialogItem::GetDialogItem()
{
  TFarDialog * Dlg = GetDialog();
  DebugAssert(Dlg);
  return &Dlg->FDialogItems[GetItemIdx()];
}

void TFarDialogItem::SetBounds(const TRect & Value)
{
  if (FBounds != Value)
  {
    FBounds = Value;
    TFarDialogItem::UpdateBounds();
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
  FarDialogItem * DItem = GetDialogItem();
  nb::used(B);
  nb::used(DItem);
#define BOUND(DIB, BB, DB, CB) ((DItem->DIB = B.BB >= 0 ? \
    (GetContainer() ? nb::ToInt32(GetContainer()->CB) : 0) + B.BB : GetDialog()->GetSize().DB + B.BB))
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
    const TRect B = GetActualBounds();
    SMALL_RECT Rect{};
    Rect.Left = static_cast<int16_t>(B.Left);
    Rect.Top = static_cast<int16_t>(B.Top);
    Rect.Right = static_cast<int16_t>(B.Right);
    Rect.Bottom = static_cast<int16_t>(B.Bottom);
    SendDialogMessage(DM_SETITEMPOSITION, nb::ToPtr(&Rect));
  }
}

int8_t TFarDialogItem::GetColor(int32_t Index) const
{
  return *((nb::ToInt8Ptr(&FColors)) + Index);
}

void TFarDialogItem::SetColor(int32_t Index, int8_t Value)
{
  if (GetColor(Index) != Value)
  {
    *((nb::ToInt8Ptr(&FColors)) + Index) = Value;
    FColorMask |= (0xFF << (Index * 8));
  }
}

const struct PluginStartupInfo * TFarDialogItem::GetPluginStartupInfo() const
{
  return GetDialog()->GetFarPlugin()->GetPluginStartupInfo();
}

void TFarDialogItem::SetFlags(FARDIALOGITEMFLAGS Value)
{
  if (GetFlags() != Value)
  {
    DebugAssert(!GetDialog()->GetHandle());
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
  return TRect(nb::ToInt32(GetDialogItem()->X1), nb::ToInt32(GetDialogItem()->Y1),
               nb::ToInt32(GetDialogItem()->X2), nb::ToInt32(GetDialogItem()->Y2));
}

FARDIALOGITEMFLAGS TFarDialogItem::GetFlags() const
{
  return GetDialogItem()->Flags;
}

void TFarDialogItem::SetDataInternal(const UnicodeString & Value)
{
  const UnicodeString FarData = Value.c_str();
  if (GetDialog()->GetHandle())
  {
    SendDialogMessage(DM_SETTEXTPTR, nb::ToPtr(ToWCharPtr(FarData)));
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
  const UnicodeString FarData = Value.c_str();
  nb_free(GetDialogItem()->Data);
  GetDialogItem()->Data = TCustomFarPlugin::DuplicateStr(FarData, /*AllowEmpty=*/true);
}

UnicodeString TFarDialogItem::GetData()
{
  return static_cast<const TFarDialogItem *>(this)->GetData();
}

UnicodeString TFarDialogItem::GetData() const
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
    DebugAssert(!GetDialog()->GetHandle());
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
    const FARDIALOGITEMFLAGS Flag = Index & 0xFFFFFFFFFFFFFF00ULL;
    const bool ToHandle = true; //Value;

    switch (Flag)
    {
    case DIF_DISABLE:
      if (GetDialog()->GetHandle())
      {
        SendDialogMessage(DM_ENABLE, nb::ToPtr(!Value));
      }
      break;

    case DIF_HIDDEN:
      if (GetDialog()->GetHandle())
      {
        SendDialogMessage(DM_SHOWITEM, nb::ToPtr(!Value));
      }
      break;

    case DIF_3STATE:
      if (GetDialog()->GetHandle())
      {
        SendDialogMessage(DM_SET3STATE, nb::ToPtr(Value));
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

void TFarDialogItem::SetEnabledFollow(TFarDialogItem * Value)
{
  if (GetEnabledFollow() != Value)
  {
    FEnabledFollow = Value;
    Change();
  }
}

void TFarDialogItem::SetEnabledDependency(TFarDialogItem * Value)
{
  if (GetEnabledDependency() != Value)
  {
    FEnabledDependency = Value;
    Change();
  }
}

void TFarDialogItem::SetEnabledDependencyNegative(TFarDialogItem * Value)
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

intptr_t TFarDialogItem::FailItemProc(intptr_t Msg, void * Param)
{
  intptr_t Result;
  switch (Msg)
  {
  case DN_KILLFOCUS:
    Result = nb::ToIntPtr(GetItemIdx());
    break;

  default:
    Result = DefaultItemProc(Msg, Param);
    break;
  }
  return Result;
}

intptr_t TFarDialogItem::ItemProc(intptr_t Msg, void * Param)
{
  intptr_t Result{0};
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
    INPUT_RECORD * Rec = static_cast<INPUT_RECORD *>(Param);
    MOUSE_EVENT_RECORD * Event = &Rec->Event.MouseEvent;
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
    Result &= ~static_cast<intptr_t>(FColorMask);
    Result |= (FColors & FColorMask);
  }
  return Result;
}

void TFarDialogItem::DoFocus()
{
}

void TFarDialogItem::DoExit()
{
  if (!FOnExit.empty())
  {
    FOnExit(this);
  }
}

intptr_t TFarDialogItem::DefaultItemProc(intptr_t Msg, void * Param)
{
  if (GetDialog() && GetDialog()->GetHandle())
  {
    const TFarEnvGuard Guard;
    return GetPluginStartupInfo()->DefDlgProc(GetDialog()->GetHandle(),
      Msg, nb::ToIntPtr(GetItemIdx()), Param);
  }
  return 0;
}

intptr_t TFarDialogItem::DefaultDialogProc(intptr_t Msg, intptr_t Param1, void * Param2)
{
  if (GetDialog() && GetDialog()->GetHandle())
  {
    const TFarEnvGuard Guard;
    return GetPluginStartupInfo()->DefDlgProc(GetDialog()->GetHandle(),
        Msg, Param1, Param2);
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
  const bool Value =
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
  TFarDialog * Dlg = GetDialog();
  DebugAssert(Dlg);
  Dlg->Change();
}

intptr_t TFarDialogItem::SendDialogMessage(intptr_t Msg, intptr_t Param1, void * Param2)
{
  return GetDialog()->SendDlgMessage(Msg, Param1, Param2);
}

intptr_t TFarDialogItem::SendDialogMessage(intptr_t Msg, void * Param)
{
  return GetDialog()->SendDlgMessage(Msg, GetItemIdx(), Param);
}

void TFarDialogItem::SetSelected(int32_t Value)
{
  if (GetSelected() != Value)
  {
    if (GetDialog()->GetHandle())
    {
      SendDialogMessage(DM_SETCHECK, nb::ToPtr(Value));
    }
    UpdateSelected(Value);
  }
}

void TFarDialogItem::UpdateSelected(intptr_t Value)
{
  if (GetSelected() != Value)
  {
    GetDialogItem()->Selected = Value;
    DialogChange();
  }
}

intptr_t TFarDialogItem::GetSelected() const
{
  return GetDialogItem()->Selected;
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

  R.Left += nb::ToInt32(DeltaX);
  R.Right += nb::ToInt32(DeltaX);
  R.Top += nb::ToInt32(DeltaY);
  R.Bottom += nb::ToInt32(DeltaY);

  SetBounds(R);
}

void TFarDialogItem::MoveAt(int32_t X, int32_t Y)
{
  Move(X - GetLeft(), Y - GetTop());
}

void TFarDialogItem::SetCoordinate(int32_t Index, int32_t Value)
{
  static_assert(sizeof(TRect) == sizeof(int32_t) * 4, "TRect");
  TRect R = GetBounds();
  int32_t * D = reinterpret_cast<int32_t *>(&R);
  D += Index;
  *D = nb::ToInt32(Value);
  SetBounds(R);
}

int32_t TFarDialogItem::GetCoordinate(int32_t Index) const
{
  static_assert(sizeof(TRect) == sizeof(int32_t) * 4, "TRect");
  TRect R = GetBounds();
  int32_t * D = reinterpret_cast<int32_t *>(&R);
  D += Index;
  return nb::ToInt32(*D);
}

void TFarDialogItem::SetWidth(int32_t Value)
{
  TRect R = GetBounds();
  if (R.Left >= 0)
  {
    R.Right = R.Left + Value - 1;
  }
  else
  {
    DebugAssert(R.Right < 0);
    R.Left = R.Right - nb::ToInt32(Value + 1);
  }
  SetBounds(R);
}

int32_t TFarDialogItem::GetWidth() const
{
  return nb::ToInt32(GetActualBounds().Width() + 1);
}

void TFarDialogItem::SetHeight(int32_t Value)
{
  TRect R = GetBounds();
  if (R.Top >= 0)
  {
    R.Bottom = nb::ToInt32(R.Top + Value - 1);
  }
  else
  {
    DebugAssert(R.Bottom < 0);
    R.Top = nb::ToInt32(R.Bottom - Value + 1);
  }
  SetBounds(R);
}

int32_t TFarDialogItem::GetHeight() const
{
  return nb::ToInt32(GetActualBounds().Height() + 1);
}

bool TFarDialogItem::CanFocus() const
{
  const FARDIALOGITEMTYPES Type = GetType();
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
  TFarDialog * Dlg = GetDialog();
  DebugAssert(Dlg);
  Dlg->SetItemFocused(Value ? this : nullptr);
}

void TFarDialogItem::SetFocus()
{
  DebugAssert(CanFocus());
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
        DebugAssert(GetDialog()->GetItemFocused() != this);
        GetDialog()->GetItemFocused()->UpdateFocused(false);
      }
      UpdateFocused(true);
    }
  }
}

void TFarDialogItem::Init()
{
  // GetDialog()->Add(this);
  if (GetFlag(DIF_CENTERGROUP))
  {
    SMALL_RECT Rect;
    nb::ClearStruct(Rect);

    // at least for "text" item, returned item size is not correct (on 1.70 final)
    SendDialogMessage(DM_GETITEMPOSITION, nb::ToPtr(&Rect));

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

TPoint TFarDialogItem::MouseClientPosition(const MOUSE_EVENT_RECORD * Event)
{
  TPoint Result;
  if (GetType() == DI_USERCONTROL)
  {
    Result = TPoint(Event->dwMousePosition.X, Event->dwMousePosition.Y);
  }
  else
  {
    Result = TPoint(
        nb::ToInt32(Event->dwMousePosition.X - GetDialog()->GetBounds().Left - GetLeft()),
        nb::ToInt32(Event->dwMousePosition.Y - GetDialog()->GetBounds().Top - GetTop()));
  }
  return Result;
}

bool TFarDialogItem::MouseClick(MOUSE_EVENT_RECORD * Event)
{
  if (FOnMouseClick)
  {
    FOnMouseClick(this, Event);
  }
  INPUT_RECORD Rec = {};
  Rec.EventType = MOUSE_EVENT;
  memmove(&Rec.Event.MouseEvent, Event, sizeof(*Event));
  return DefaultItemProc(DN_CONTROLINPUT, nb::ToPtr(&Rec)) != 0;
}

bool TFarDialogItem::MouseMove(int32_t /*X*/, int32_t /*Y*/,
  MOUSE_EVENT_RECORD * Event)
{
  INPUT_RECORD Rec = {};
  Rec.EventType = MOUSE_EVENT;
  memmove(&Rec.Event.MouseEvent, Event, sizeof(*Event));
  return DefaultDialogProc(DN_INPUT, 0, nb::ToPtr(&Rec)) != 0;
}

void TFarDialogItem::Text(int32_t X, int32_t Y, const FarColor &Color, const UnicodeString & Str)
{
  const TFarEnvGuard Guard;
  GetPluginStartupInfo()->Text(
    nb::ToInt32(GetDialog()->GetBounds().Left + GetLeft() + X),
    nb::ToInt32(GetDialog()->GetBounds().Top + GetTop() + Y),
    &Color, Str.c_str());
}

void TFarDialogItem::Redraw()
{
  // do not know how to force redraw of the item only
  GetDialog()->Redraw();
}

void TFarDialogItem::SetContainer(TFarDialogContainer * Value)
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

TFarBox::TFarBox(TFarDialog * ADialog) noexcept :
  TFarDialogItem(OBJECT_CLASS_TFarBox, ADialog, DI_SINGLEBOX)
{
}

TFarButton::TFarButton(TFarDialog * ADialog) noexcept : TFarButton(OBJECT_CLASS_TFarButton, ADialog)
{
}

TFarButton::TFarButton(TObjectClassId Kind, TFarDialog * ADialog) noexcept :
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
    int32_t Margin = 0;
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

UnicodeString TFarButton::GetData() const
{
  UnicodeString Result = TFarDialogItem::GetData();
  if ((FBrackets == brTight) || (FBrackets == brSpace))
  {
    const bool HasBrackets = (Result.Length() >= 2) &&
      (Result[1] == ((FBrackets == brSpace) ? L' ' : L'[')) &&
      (Result[Result.Length()] == ((FBrackets == brSpace) ? L' ' : L']'));
    DebugAssert(HasBrackets);
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
    DebugAssert(!GetDialog()->GetHandle());
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
    const UnicodeString Data = GetData();
    SetFlag(DIF_NOBRACKETS, (Value != brNormal));
    FBrackets = Value;
    SetDataInternal(Data);
  }
}

intptr_t TFarButton::ItemProc(intptr_t Msg, void * Param)
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
  const UnicodeString Caption = GetCaption();
  const int32_t P = Caption.Pos(L'&');
  const bool Result =
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

TFarCheckBox::TFarCheckBox(TFarDialog * ADialog) noexcept :
  TFarDialogItem(OBJECT_CLASS_TFarCheckBox, ADialog, DI_CHECKBOX),
  FOnAllowChange(nullptr)
{
}

intptr_t TFarCheckBox::ItemProc(intptr_t Msg, void * Param)
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
      UpdateSelected(nb::ToIntPtr(Param));
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

TFarRadioButton::TFarRadioButton(TFarDialog * ADialog) noexcept :
  TFarDialogItem(OBJECT_CLASS_TFarRadioButton, ADialog, DI_RADIOBUTTON),
  FOnAllowChange(nullptr)
{
}

intptr_t TFarRadioButton::ItemProc(intptr_t Msg, void * Param)
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
      UpdateSelected(nb::ToIntPtr(Param));
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

TFarEdit::TFarEdit(TFarDialog * ADialog) noexcept :
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

intptr_t TFarEdit::ItemProc(intptr_t Msg, void * Param)
{
  if (Msg == DN_EDITCHANGE)
  {
    const UnicodeString Data = (static_cast<FarDialogItem *>(Param))->Data;
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
    DebugAssert(!GetDialog()->GetHandle());
    FarDialogItem * Item = GetDialogItem();
    // DebugAssert(&GetDialogItem()->Mask == &GetDialogItem()->History);

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
    const bool PrevHistory = !GetHistory().IsEmpty();
    SetFlag(DIF_HISTORY, (Index == 0) && !Value.IsEmpty());
    const bool Masked = (Index == 1) && !Value.IsEmpty();
    SetFlag(DIF_MASKEDIT, Masked);
    if (Masked)
    {
      SetFixed(true);
    }
    const bool CurrHistory = !GetHistory().IsEmpty();
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
  const int32_t Int = GetAsInteger();
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

TFarSeparator::TFarSeparator(TFarDialog * ADialog) noexcept :
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
    DebugAssert(!GetDialog()->GetHandle());
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
  R.Top = nb::ToInt32(Value);
  R.Bottom = nb::ToInt32(Value);
  SetBounds(R);
}

int32_t TFarSeparator::GetPosition() const
{
  return GetBounds().Top;
}

TFarText::TFarText(TFarDialog * ADialog) noexcept :
  TFarDialogItem(OBJECT_CLASS_TFarText, ADialog, DI_TEXT)
{
}

void TFarText::SetData(const UnicodeString & Value)
{
  TFarDialogItem::SetData(Value);
  if (GetLeft() >= 0 || GetRight() >= 0)
  {
    // SetWidth(::StripHotkey(Value).Length());
    SetWidth(Value.Length());
  }
}

TFarList::TFarList(TFarDialogItem * ADialogItem) noexcept :
  TStringList(OBJECT_CLASS_TFarList),
  FDialogItem(ADialogItem),
  FNoDialogUpdate(false)
{
  DebugAssert((ADialogItem == nullptr) ||
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

void TFarList::Assign(const TPersistent * Source)
{
  TStringList::Assign(Source);

  const TFarList * FarList = rtti::dyn_cast_or_null<TFarList>(Source);
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

  FarListUpdate ListUpdate{};
  nb::ClearStruct(ListUpdate);
  ListUpdate.StructSize = sizeof(FarListUpdate);
  ListUpdate.Item = *ListItem;
  ListUpdate.Index = Index;
  GetDialogItem()->SendDialogMessage(DM_LISTUPDATE, nb::ToPtr(&ListUpdate));
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
    const size_t ItemsCount = nb::ToSizeT(GetCount());
    if (FListItems->ItemsNumber != ItemsCount)
    {
      const FarListItem * Items = FListItems->Items;
      const size_t ItemsNumber = FListItems->ItemsNumber;
      if (ItemsCount)
      {
        FListItems->Items = nb::calloc<FarListItem *>(ItemsCount, sizeof(FarListItem));
        for (size_t Index = 0; Index < ItemsCount; ++Index)
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
      for (size_t Index = 0; Index < ItemsNumber; ++Index)
      {
        nb_free(Items[Index].Text);
      }
      nb_free(Items);
      FListItems->ItemsNumber = nb::ToSizeT(GetCount());
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
      GetDialogItem()->SendDialogMessage(DM_LISTSET, nb::ToPtr(FListItems));
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
  TFarDialogItem * DialogItem = GetDialogItem();
  DebugAssert(DialogItem != nullptr);
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
    const int32_t ATopIndex = GetTopIndex();
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
  TFarDialogItem * DialogItem = GetDialogItem();
  DebugAssert(DialogItem != nullptr);
  const TFarDialog * Dlg = DialogItem->GetDialog();
  DebugAssert(Dlg);
  DebugAssert(Dlg->GetHandle());
  DebugUsedParam(Dlg);
  FarListPos ListPos{};
  ListPos.StructSize = sizeof(FarListPos);
  ListPos.SelectPos = Position;
  ListPos.TopPos = TopIndex;
  DialogItem->SendDialogMessage(DM_LISTSETCURPOS, nb::ToPtr(&ListPos));
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
  TFarDialogItem * DialogItem = GetDialogItem();
  DebugAssert(DialogItem != nullptr);
  return nb::ToInt32(DialogItem->SendDialogMessage(DM_LISTGETCURPOS, nullptr));
}

int32_t TFarList::GetTopIndex() const
{
  int32_t Result = -1;
  if (GetCount() != 0)
  {
    FarListPos ListPos{};
    nb::ClearStruct(ListPos);
    ListPos.StructSize = sizeof(FarListPos);
    TFarDialogItem * DialogItem = GetDialogItem();
    DebugAssert(DialogItem != nullptr);
    DialogItem->SendDialogMessage(DM_LISTGETCURPOS, nb::ToPtr(&ListPos));
    Result = nb::ToInt32(ListPos.TopPos);
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
  const TFarDialogItem * DialogItem = GetDialogItem();
  DebugAssert(DialogItem != nullptr);
  return DialogItem ? DialogItem->GetHeight() - (GetDialogItem()->GetFlag(DIF_LISTNOBOX) ? 0 : 2) : 0;
}

int32_t TFarList::GetSelectedInt(bool Init) const
{
  int32_t Result = nb::NPOS;
  TFarDialogItem * DialogItem = GetDialogItem();
  DebugAssert(DialogItem != nullptr);
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
    const wchar_t * Data = DialogItem->GetDialogItem()->Data;
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
  DebugAssert(Index >= 0 && Index < FListItems->ItemsNumber);
  if (GetDialogItem() == nullptr || !GetDialogItem()->GetDialog()->GetHandle())
  {
    return FListItems->Items[Index].Flags;
  }
  else
  {
    FarListGetItem List;
    List.StructSize = sizeof(FarListGetItem);
    List.ItemIndex = Index;
    bool Result = GetDialogItem()->SendDialogMessage(DM_LISTGETITEM, nb::ToPtr(&List));
    return Result ? List.Item.Flags : LIF_NONE;
  }
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

int32_t TFarList::ItemProc(intptr_t Msg, void * Param)
{
  TFarDialogItem * DialogItem = GetDialogItem();
  DebugAssert(DialogItem != nullptr);
  if (Msg == DN_LISTCHANGE)
  {
    if ((Param == nullptr) && (GetCount() == 0))
    {
      DialogItem->UpdateData(L"");
    }
    else
    {
      const int32_t Int32 = nb::ToInt32(nb::ToIntPtr(Param));
      DebugAssert(Int32 >= 0 && Int32 < GetCount());
      FLastPosChange = Int32;
      DialogItem->UpdateData(GetString(Int32));
    }
    return 1;
  }
  return 0;
}

TFarListBox::TFarListBox(TFarDialog * ADialog) noexcept :
  TFarDialogItem(OBJECT_CLASS_TFarListBox, ADialog, DI_LISTBOX),
  FAutoSelect(asOnlyFocus),
  FDenyClose(false)
{
  GetDialogItem()->ListItems = FList->GetListItems();
}

intptr_t TFarListBox::ItemProc(intptr_t Msg, void * Param)
{
  intptr_t Result = 0;
  if (Msg == DN_CONTROLINPUT)
  {
    const INPUT_RECORD * Rec = static_cast<const INPUT_RECORD *>(Param);
    const KEY_EVENT_RECORD & Event = Rec->Event.KeyEvent;
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
  SendDialogMessage(DIF_LISTTRACKMOUSE, nb::ToPtr(GetAutoSelect()));
}

void TFarListBox::SetItems(const TStrings * Value, bool OwnItems)
{
  FList->Assign(Value);
  FList->SetOwnsObjects(OwnItems);
}

void TFarListBox::SetList(const TFarList * Value, bool OwnItems)
{
  SetItems(Value, OwnItems);
}

bool TFarListBox::CloseQuery()
{
  return true;
}

TFarComboBox::TFarComboBox(TFarDialog * ADialog) noexcept :
  TFarDialogItem(OBJECT_CLASS_TFarComboBox, ADialog, DI_COMBOBOX)
{
  GetDialogItem()->ListItems = FList->GetListItems();
  SetAutoSelect(false);
}

void TFarComboBox::ResizeToFitContent()
{
  SetWidth(FList->GetMaxLength());
}

intptr_t TFarComboBox::ItemProc(intptr_t Msg, void * Param)
{
  if (Msg == DN_EDITCHANGE)
  {
    const UnicodeString Data = (static_cast<FarDialogItem *>(Param))->Data;
    nb_free(GetDialogItem()->Data);
    GetDialogItem()->Data = TCustomFarPlugin::DuplicateStr(Data, /*AllowEmpty=*/true);
    FItemChanged = true;
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

TFarLister::TFarLister(TFarDialog * ADialog) noexcept :
  TFarDialogItem(OBJECT_CLASS_TFarLister, ADialog, DI_USERCONTROL)
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

TStrings * TFarLister::GetItems() const
{
  return FItems.get();
}

void TFarLister::SetItems(const TStrings * Value)
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

intptr_t TFarLister::ItemProc(intptr_t Msg, void * Param)
{
  intptr_t Result = 0;

  if (Msg == DN_DRAWDLGITEM)
  {
    bool AScrollBar = GetScrollBar();
    int32_t ScrollBarPos = 0;
    if (GetItems()->GetCount() > GetHeight())
    {
      ScrollBarPos = nb::ToInt32((nb::ToDouble(GetHeight() - 3) * (nb::ToDouble(FTopIndex) / (GetItems()->GetCount() - GetHeight())))) + 1;
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
    const INPUT_RECORD * Rec = static_cast<const INPUT_RECORD *>(Param);
    if (Rec->EventType == KEY_EVENT)
    {
      const KEY_EVENT_RECORD * KeyEvent = &Rec->Event.KeyEvent;

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
          INPUT_RECORD Rec2 = {};
          Rec2.EventType = KEY_EVENT;
          Rec2.Event.KeyEvent.wVirtualKeyCode = VK_TAB;
          Rec2.Event.KeyEvent.dwControlKeyState = SHIFT_PRESSED;
          SendDialogMessage(DN_CONTROLINPUT, 1, nb::ToPtr(&Rec2));
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
          INPUT_RECORD Rec2 = {};
          Rec2.EventType = KEY_EVENT;
          Rec2.Event.KeyEvent.wVirtualKeyCode = VK_TAB;
          Rec2.Event.KeyEvent.dwControlKeyState = 0;
          SendDialogMessage(DN_CONTROLINPUT, 1, nb::ToPtr(&Rec2));
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

      const MOUSE_EVENT_RECORD * Event = &Rec->Event.MouseEvent;
      TPoint P = MouseClientPosition(Event);

      if (FLAGSET(Event->dwEventFlags, DOUBLE_CLICK) &&
        (P.x < GetWidth() - 1))
      {
        Result = TFarDialogItem::ItemProc(Msg, Param);
      }
      else if (FLAGSET(Event->dwEventFlags, MOUSE_WHEELED))
      {
        auto WheelDelta = (nb::ToInt(Event->dwButtonState) >> 16) / 120;
        int32_t NewTopIndex = GetTopIndex() - WheelDelta;
        if (NewTopIndex >= 0 && NewTopIndex <= GetItems()->GetCount() - GetHeight())
        {
          SetTopIndex(NewTopIndex);
        }
        Result = 1;
      }
      else if (Event->dwEventFlags == 0)
      {
        int32_t NewTopIndex = GetTopIndex();

        if (((P.x == nb::ToInt32(GetWidth()) - 1) && (P.y == 0)) ||
          ((P.x < nb::ToInt32(GetWidth() - 1)) && (P.y < nb::ToInt32(GetHeight() / 2))))
        {
          if (NewTopIndex > 0)
          {
            --NewTopIndex;
          }
        }
        else if (((P.x == GetWidth() - 1) && (P.y == nb::ToInt32(GetHeight() - 1))) ||
          ((P.x < GetWidth() - 1) && (P.y >= nb::ToInt32(GetHeight() / 2))))
        {
          if (NewTopIndex < GetItems()->GetCount() - GetHeight())
          {
            ++NewTopIndex;
          }
        }
        else
        {
          DebugAssert(P.x == GetWidth() - 1);
          DebugAssert((P.y > 0) && (P.y < nb::ToInt32(GetHeight() - 1)));
          NewTopIndex = nb::ToInt32(ceil(nb::ToDouble(P.y - 1) / (GetHeight() - 2) * (GetItems()->GetCount() - GetHeight() + 1)));
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
