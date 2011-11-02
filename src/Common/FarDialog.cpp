//---------------------------------------------------------------------------
#include "stdafx.h"

#include "boostdefines.hpp"
#include <boost/scope_exit.hpp>
#include <boost/bind.hpp>

#include <map>

#include "FarDialog.h"
#include "Common.h"

//---------------------------------------------------------------------------
std::wstring StripHotKey(std::wstring Text)
{
    size_t Len = Text.length();
    int Pos = 0;
    while (Pos < Len)
    {
        if (Text[Pos] == '&')
        {
            Text.erase(Pos, 1);
            Len--;
        }
        else
        {
            Pos++;
        }
    }
    return Text;
}

//---------------------------------------------------------------------------
TRect Rect(int Left, int Top, int Right, int Bottom)
{
    TRect result = TRect(Left, Top, Right, Bottom);
    return result;
}

//---------------------------------------------------------------------------
TFarDialog::TFarDialog(TCustomFarPlugin *AFarPlugin) :
    TObject(),
    FFarPlugin(NULL),
    FBounds(-1, -1, 40, 10),
    FFlags(0),
    FHelpTopic(),
    FVisible(false),
    FItems(NULL),
    FContainers(NULL),
    FHandle(0),
    FDefaultButton(NULL),
    FBorderBox(NULL),
    // FNextItemPosition(0),
    FDefaultGroup(0),
    FTag(0),
    FItemFocused(NULL),
    // FOnKey(NULL),
    FDialogItems(NULL),
    FDialogItemsCapacity(0),
    FChangesLocked(0),
    FChangesPending(false),
    FResult(0),
    FNeedsSynchronize(false),
    // FSynchronizeMethod(NULL),
    Self(NULL)
{
    assert(AFarPlugin);
    Self = this;
    FItems = new TObjectList();
    FContainers = new TObjectList();
    FFarPlugin = AFarPlugin;
    FFlags = 0;
    FHandle = 0;
    FDefaultGroup = 0;
    FDefaultButton = NULL;
    FNextItemPosition = ipNewLine;
    FDialogItems = NULL;
    FDialogItemsCapacity = 0;
    FChangesPending = false;
    FChangesLocked = 0;
    FResult = -1;
    FNeedsSynchronize = false;
    FSynchronizeObjects[0] = INVALID_HANDLE_VALUE;
    FSynchronizeObjects[1] = INVALID_HANDLE_VALUE;

    FBorderBox = new TFarBox(this);
    FBorderBox->SetBounds(TRect(3, 1, -4, -2));
    FBorderBox->SetDouble(true);
}
//---------------------------------------------------------------------------
TFarDialog::~TFarDialog()
{
    for (int i = 0; i < GetItemCount(); i++)
    {
        GetItem(i)->Detach();
    }
    delete FItems;
    delete[] FDialogItems;
    FDialogItemsCapacity = 0;
    delete FContainers;
    if (FSynchronizeObjects[0] != INVALID_HANDLE_VALUE)
    {
        CloseHandle(FSynchronizeObjects[0]);
    }
    if (FSynchronizeObjects[1] != INVALID_HANDLE_VALUE)
    {
        CloseHandle(FSynchronizeObjects[1]);
    }
}
//---------------------------------------------------------------------------
void TFarDialog::SetBounds(const TRect &value)
{
    if (GetBounds() != value)
    {
        LockChanges();
        {
            BOOST_SCOPE_EXIT ( (&Self) )
            {
                Self->UnlockChanges();
            } BOOST_SCOPE_EXIT_END
            FBounds = value;
            if (GetHandle())
            {
                COORD Coord;
                Coord.X = (short int)GetSize().x;
                Coord.Y = (short int)GetSize().y;
                SendMessage(DM_RESIZEDIALOG, 0, (int)&Coord);
                Coord.X = (short int)FBounds.Left;
                Coord.Y = (short int)FBounds.Top;
                SendMessage(DM_MOVEDIALOG, true, (int)&Coord);
            }
            for (int i = 0; i < GetItemCount(); i++)
            {
                GetItem(i)->DialogResized();
            }
        }
    }
}
//---------------------------------------------------------------------------
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
//---------------------------------------------------------------------------
TPoint TFarDialog::GetClientSize() const
{
    TPoint S;
    if (FBorderBox)
    {
        TRect R = FBorderBox->GetActualBounds();
        S.x = R.Width() + 1;
        S.y = R.Height() + 1;
        S.x -= S.x > 4 ? 4 : S.x;
        S.y -= S.y > 2 ? 2 : S.y;
    }
    else
    {
        S = GetSize();
    }
    return S;
}
//---------------------------------------------------------------------------
TPoint TFarDialog::GetMaxSize()
{
    TPoint P = GetFarPlugin()->TerminalInfo();
    P.x -= 2;
    P.y -= 3;
    return P;
}
//---------------------------------------------------------------------------
void TFarDialog::SetHelpTopic(const std::wstring &value)
{
    if (FHelpTopic != value)
    {
        assert(!GetHandle());
        FHelpTopic = value;
    }
}
//---------------------------------------------------------------------------
void TFarDialog::SetFlags(const unsigned int &value)
{
    if (GetFlags() != value)
    {
        assert(!GetHandle());
        FFlags = value;
    }
}
//---------------------------------------------------------------------------
void TFarDialog::SetCentered(const bool &value)
{
    if (GetCentered() != value)
    {
        assert(!GetHandle());
        TRect B = GetBounds();
        B.Left = value ? -1 : 0;
        B.Top = value ? -1 : 0;
        SetBounds(B);
    }
}
//---------------------------------------------------------------------------
bool TFarDialog::GetCentered() const
{
    return (GetBounds().Left < 0) && (GetBounds().Top < 0);
}
//---------------------------------------------------------------------------
TPoint TFarDialog::GetSize() const
{
    if (GetCentered())
    {
        return TPoint(GetBounds().Right, GetBounds().Bottom);
    }
    else
    {
        return TPoint(GetBounds().Width() + 1, GetBounds().Height() + 1);
    }
}
//---------------------------------------------------------------------------
void TFarDialog::SetSize(const TPoint &value)
{
    TRect B = GetBounds();
    if (GetCentered())
    {
        B.Right = value.x;
        B.Bottom = value.y;
    }
    else
    {
        B.Right = FBounds.Left + value.x - 1;
        B.Bottom = FBounds.Top + value.y - 1;
    }
    SetBounds(B);
}
//---------------------------------------------------------------------------
void TFarDialog::SetWidth(const int &value)
{
    SetSize(TPoint(value, GetHeight()));
}
//---------------------------------------------------------------------------
int TFarDialog::GetWidth() const
{
    return GetSize().x;
}
//---------------------------------------------------------------------------
void TFarDialog::SetHeight(const int &value)
{
    SetSize(TPoint(GetWidth(), value));
}
//---------------------------------------------------------------------------
int TFarDialog::GetHeight() const
{
    return GetSize().y;
}
//---------------------------------------------------------------------------
void TFarDialog::SetCaption(const std::wstring &value)
{
    if (GetCaption() != value)
    {
        FBorderBox->SetCaption(value);
    }
}
//---------------------------------------------------------------------------
std::wstring TFarDialog::GetCaption() const
{
    return FBorderBox->GetCaption();
}
//---------------------------------------------------------------------------
size_t TFarDialog::GetItemCount() const
{
    return FItems->GetCount();
}
//---------------------------------------------------------------------------
TFarDialogItem *TFarDialog::GetItem(size_t Index)
{
    TFarDialogItem *DialogItem;
    if (GetItemCount())
    {
        assert(Index >= 0 && Index < FItems->GetCount());
        DialogItem = dynamic_cast<TFarDialogItem *>((*GetItems())[Index]);
        assert(DialogItem);
    }
    else
    {
        DialogItem = NULL;
    }
    return DialogItem;
}
//---------------------------------------------------------------------------
void TFarDialog::Add(TFarDialogItem *DialogItem)
{
    TRect R = GetClientRect();
    int Left, Top;
    GetNextItemPosition(Left, Top);
    R.Left = Left;
    R.Top = Top;

    if (FDialogItemsCapacity == GetItems()->GetCount())
    {
        int DialogItemsDelta = 10;
        FarDialogItem *NewDialogItems;
        NewDialogItems = new FarDialogItem[GetItems()->GetCount() + DialogItemsDelta];
        if (FDialogItems)
        {
            memcpy(NewDialogItems, FDialogItems, FDialogItemsCapacity * sizeof(FarDialogItem));
            delete[] FDialogItems;
        }
        memset(NewDialogItems + FDialogItemsCapacity, 0,
               DialogItemsDelta * sizeof(FarDialogItem));
        FDialogItems = NewDialogItems;
        FDialogItemsCapacity += DialogItemsDelta;
    }

    assert(DialogItem);
    DialogItem->SetItem(GetItems()->Add(DialogItem));

    R.Bottom = R.Top;
    DialogItem->SetBounds(R);
    DialogItem->SetGroup(GetDefaultGroup());
}
//---------------------------------------------------------------------------
void TFarDialog::Add(TFarDialogContainer *Container)
{
    FContainers->Add(Container);
}
//---------------------------------------------------------------------------
void TFarDialog::GetNextItemPosition(int &Left, int &Top)
{
    TRect R = GetClientRect();
    Left = R.Left;
    Top = R.Top;

    TFarDialogItem *LastI = GetItem(GetItemCount() - 1);
    LastI = LastI == FBorderBox ? NULL : LastI;

    if (LastI)
    {
        switch (GetNextItemPosition())
        {
        case ipNewLine:
            Top = LastI->GetBottom() + 1;
            break;

        case ipBelow:
            Top = LastI->GetBottom() + 1;
            Left = LastI->GetLeft();
            break;

        case ipRight:
            Top = LastI->GetTop();
            Left = LastI->GetRight() + 3;
            break;
        }
    }
}
//---------------------------------------------------------------------------
LONG_PTR WINAPI TFarDialog::DialogProcGeneral(HANDLE Handle, int Msg, int Param1, LONG_PTR Param2)
{
    TFarPluginEnvGuard Guard;

    static std::map<HANDLE, long> Dialogs;
    TFarDialog *Dialog = NULL;
    LONG_PTR Result = 0;
    if (Msg == DN_INITDIALOG)
    {
        assert(Dialogs.find(Handle) == Dialogs.end());
        Dialogs[Handle] = Param2;
        Dialog = (TFarDialog *)Param2;
        Dialog->FHandle = Handle;
    }
    else
    {
        if (Dialogs.find(Handle) == Dialogs.end())
        {
            // DM_CLOSE is sent after DN_CLOSE, if the dialog was closed programatically
            // by SendMessage(DM_CLOSE, ...)
            assert(Msg == DM_CLOSE);
            Result = false;
        }
        else
        {
            Dialog = (TFarDialog *)Dialogs[Handle];
        }
    }

    if (Dialog != NULL)
    {
        Result = Dialog->DialogProc(Msg, Param1, Param2);
    }

    if ((Msg == DN_CLOSE) && Result)
    {
        Dialog->FHandle = 0;
        Dialogs.erase(Handle);
    }
    return Result;
}
//---------------------------------------------------------------------------
long TFarDialog::DialogProc(int Msg, int Param1, long Param2)
{
    long Result = 0;
    bool Handled = false;

    try
    {
        if (FNeedsSynchronize)
        {
            try
            {
                FNeedsSynchronize = false;
                FSynchronizeMethod();
                ReleaseSemaphore(FSynchronizeObjects[0], 1, NULL);
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

        case DN_MOUSECLICK:
        case DN_CTLCOLORDLGITEM:
        case DN_CTLCOLORDLGLIST:
        case DN_DRAWDLGITEM:
        case DN_HOTKEY:
        case DN_KEY:
            if (Param1 >= 0)
            {
                TFarDialogItem *I = GetItem(Param1);
                try
                {
                    Result = I->ItemProc(Msg, Param2);
                }
                catch (const std::exception &E)
                {
                    Handled = true;
                    DEBUG_PRINTF(L"before GetFarPlugin()->HandleException");
                    GetFarPlugin()->HandleException(&E);
                    Result = I->FailItemProc(Msg, Param2);
                }

                if (!Result && (Msg == DN_KEY))
                {
                    Result = Key(I, Param2);
                }
                Handled = true;
            }

            // FAR WORKAROUND
            // When pressing Enter FAR forces dialog to close without calling
            // DN_BTNCLICK on default button. This fixes the scenario.
            // (first check if focused dialog item is not another button)
            if (!Result && (Msg == DN_KEY) &&
                    (Param2 == KEY_ENTER) &&
                    ((Param1 < 0) ||
                     ((Param1 >= 0) && (dynamic_cast<TFarButton *>(GetItem(Param1)) == NULL))) &&
                    GetDefaultButton()->GetEnabled() &&
                    (!GetDefaultButton()->GetOnClick().empty()))
            {
                bool Close = (GetDefaultButton()->GetResult() != 0);
                GetDefaultButton()->GetOnClick()(GetDefaultButton(), Close);
                Handled = true;
                if (!Close)
                {
                    Result = true;
                }
            }
            break;

        case DN_MOUSEEVENT:
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
                Result = true;
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
                Result = true;
                if (Param1 >= 0)
                {
                    TFarButton *Button = dynamic_cast<TFarButton *>(GetItem(Param1));
                    // FAR WORKAROUND
                    // FAR 1.70 alpha 6 calls DN_CLOSE even for non-button dialog items
                    // (list boxes in particular), while FAR 1.70 beta 5 used ID of
                    // default button in such case.
                    // Particularly for listbox, we can prevent closing dialog using
                    // flag DIF_LISTNOCLOSE.
                    if (Button == NULL)
                    {
                        assert((short int)FarPlugin->FarVersion() >= (short int)FAR170ALPHA6);
                        assert(dynamic_cast<TFarListBox *>(GetItem(Param1)) != NULL);
                        Result = false;
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
            };

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
    catch (const std::exception &E)
    {
        DEBUG_PRINTF(L"before GetFarPlugin()->HandleException");
        GetFarPlugin()->HandleException(&E);
        if (!Handled)
        {
            Result = FailDialogProc(Msg, Param1, Param2);
        }
    }
    return Result;
}
//---------------------------------------------------------------------------
long TFarDialog::DefaultDialogProc(int Msg, int Param1, long Param2)
{
    TFarEnvGuard Guard;
    return GetFarPlugin()->FStartupInfo.DefDlgProc(GetHandle(), Msg, Param1, Param2);
}
//---------------------------------------------------------------------------
long TFarDialog::FailDialogProc(int Msg, int Param1, long Param2)
{
    long Result;
    switch (Msg)
    {
    case DN_CLOSE:
        Result = false;
        break;

    default:
        Result = DefaultDialogProc(Msg, Param1, Param2);
        break;
    }
    return Result;
}
//---------------------------------------------------------------------------
void TFarDialog::Idle()
{
    // nothing
}
//---------------------------------------------------------------------------
bool TFarDialog::MouseEvent(MOUSE_EVENT_RECORD *Event)
{
    bool Result = true;
    bool Handled = false;
    if (FLAGSET(Event->dwEventFlags, MOUSE_MOVED))
    {
        int X = Event->dwMousePosition.X - GetBounds().Left;
        int Y = Event->dwMousePosition.Y - GetBounds().Top;
        TFarDialogItem *Item = ItemAt(X, Y);
        if (Item != NULL)
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
        Result = DefaultDialogProc(DN_MOUSEEVENT, 0, reinterpret_cast<long>(Event));
    }

    return Result;
}
//---------------------------------------------------------------------------
bool TFarDialog::Key(TFarDialogItem *Item, long KeyCode)
{
    bool Result = false;
    if (!FOnKey.empty())
    {
        FOnKey(this, Item, KeyCode, Result);
    }
    return Result;
}
//---------------------------------------------------------------------------
bool TFarDialog::HotKey(unsigned long Key)
{
    bool Result = false;
    char HotKey = 0;
    if ((KEY_ALTA <= Key) && (Key <= KEY_ALTZ))
    {
        Result = true;
        HotKey = char('a' + char(Key - KEY_ALTA));
    }

    if (Result)
    {
        Result = false;
        for (int i = 0; i < GetItemCount(); i++)
        {
            if (GetItem(i)->HotKey(HotKey))
            {
                Result = true;
            }
        }
    }

    return Result;
}
//---------------------------------------------------------------------------
TFarDialogItem *TFarDialog::ItemAt(int X, int Y)
{
    TFarDialogItem *Result = NULL;
    for (int i = 0; i < GetItemCount(); i++)
    {
        TRect Bounds = GetItem(i)->GetActualBounds();
        if ((Bounds.Left <= X) && (X <= Bounds.Right) &&
                (Bounds.Top <= Y) && (Y <= Bounds.Bottom))
        {
            Result = GetItem(i);
        }
    }
    return Result;
}
//---------------------------------------------------------------------------
bool TFarDialog::CloseQuery()
{
    bool Result = true;
    for (int i = 0; i < GetItemCount() && Result; i++)
    {
        if (!GetItem(i)->CloseQuery())
        {
            Result = false;
        }
    }
    return Result;
}
//---------------------------------------------------------------------------
void TFarDialog::RefreshBounds()
{
    SMALL_RECT Rect;
    SendMessage(DM_GETDLGRECT, 0, (int)&Rect);
    FBounds.Left = Rect.Left;
    FBounds.Top = Rect.Top;
    FBounds.Right = Rect.Right;
    FBounds.Bottom = Rect.Bottom;
}
//---------------------------------------------------------------------------
void TFarDialog::Init()
{
    for (int i = 0; i < GetItemCount(); i++)
    {
        GetItem(i)->Init();
    }

    RefreshBounds();

    Change();
}
//---------------------------------------------------------------------------
int TFarDialog::ShowModal()
{
    FResult = -1;

    int BResult;

    TFarDialog *PrevTopDialog = GetFarPlugin()->FTopDialog;
    GetFarPlugin()->FTopDialog = this;
    {
        BOOST_SCOPE_EXIT ( (&Self) (&PrevTopDialog) )
        {
            Self->GetFarPlugin()->FTopDialog = PrevTopDialog;
        } BOOST_SCOPE_EXIT_END
        assert(GetDefaultButton());
        assert(GetDefaultButton()->GetDefault());

        std::wstring AHelpTopic = GetHelpTopic();

        {
            TFarEnvGuard Guard;
            TRect Bounds = GetBounds();
            HANDLE dlg = GetFarPlugin()->FStartupInfo.DialogInit(
                GetFarPlugin()->FStartupInfo.ModuleNumber,
                Bounds.Left, Bounds.Top, Bounds.Right, Bounds.Bottom,
                StrToFar(AHelpTopic), FDialogItems, GetItemCount(), 0, GetFlags(),
                DialogProcGeneral, (long)this);
            BResult = GetFarPlugin()->FStartupInfo.DialogRun(dlg);
        }

        if (BResult >= 0)
        {
            TFarButton *Button = dynamic_cast<TFarButton *>(GetItem(BResult));
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
//---------------------------------------------------------------------------
void TFarDialog::BreakSynchronize()
{
    SetEvent(FSynchronizeObjects[1]);
}
//---------------------------------------------------------------------------
void TFarDialog::Synchronize(const threadmethod_slot_type &slot)
{
    if (FSynchronizeObjects[0] == INVALID_HANDLE_VALUE)
    {
        FSynchronizeObjects[0] = CreateSemaphore(NULL, 0, 2, NULL);
        FSynchronizeObjects[1] = CreateEvent(NULL, false, false, NULL);
    }
    FSynchronizeMethod.disconnect_all_slots();
    FSynchronizeMethod.connect(slot);
    FNeedsSynchronize = true;
    WaitForMultipleObjects(LENOF(FSynchronizeObjects),
                           reinterpret_cast<HANDLE *>(&FSynchronizeObjects), false, INFINITE);
}
//---------------------------------------------------------------------------
void TFarDialog::Close(TFarButton *Button)
{
    assert(Button != NULL);
    SendMessage(DM_CLOSE, Button->GetItem(), 0);
}
//---------------------------------------------------------------------------
void TFarDialog::Change()
{
    if (FChangesLocked > 0)
    {
        FChangesPending = true;
    }
    else
    {
        TList *NotifiedContainers = new TList();
        {
            BOOST_SCOPE_EXIT ( (&NotifiedContainers) )
            {
                delete NotifiedContainers;
            } BOOST_SCOPE_EXIT_END
            TFarDialogItem *DItem;
            for (int i = 0; i < GetItemCount(); i++)
            {
                DItem = GetItem(i);
                DItem->Change();
                if (DItem->GetContainer() && NotifiedContainers->IndexOf(DItem->GetContainer()) < 0)
                {
                    NotifiedContainers->Add(DItem->GetContainer());
                }
            }

            for (int Index = 0; Index < NotifiedContainers->GetCount(); Index++)
            {
                ((TFarDialogContainer *)(*NotifiedContainers)[Index])->Change();
            }
        }
    }
}
//---------------------------------------------------------------------------
long TFarDialog::SendMessage(int Msg, int Param1, int Param2)
{
    assert(GetHandle());
    TFarEnvGuard Guard;
    return GetFarPlugin()->FStartupInfo.SendDlgMessage(GetHandle(), Msg, Param1, Param2);
}
//---------------------------------------------------------------------------
int TFarDialog::GetSystemColor(unsigned int Index)
{
    return static_cast<int>(GetFarPlugin()->FarAdvControl(ACTL_GETCOLOR, Index));
}
//---------------------------------------------------------------------------
void TFarDialog::Redraw()
{
    SendMessage(DM_REDRAW, 0, 0);
}
//---------------------------------------------------------------------------
void TFarDialog::ShowGroup(int Group, bool Show)
{
    ProcessGroup(Group, boost::bind(&TFarDialog::ShowItem, this, _1, _2), &Show);
}
//---------------------------------------------------------------------------
void TFarDialog::EnableGroup(int Group, bool Enable)
{
    ProcessGroup(Group, boost::bind(&TFarDialog::EnableItem, this, _1, _2), &Enable);
}
//---------------------------------------------------------------------------
void TFarDialog::ProcessGroup(int Group, const processgroupevent_slot_type &Callback,
        void *Arg)
{
    LockChanges();
    {
        BOOST_SCOPE_EXIT ( (&Self) )
        {
            Self->UnlockChanges();
        } BOOST_SCOPE_EXIT_END
        processgroupevent_signal_type processgroupevent;
        processgroupevent.connect(Callback);
        for (int i = 0; i < GetItemCount(); i++)
        {
            TFarDialogItem *I = GetItem(i);
            if (I->GetGroup() == Group)
            {
                processgroupevent(I, Arg);
            }
        }
    }
}
//---------------------------------------------------------------------------
void TFarDialog::ShowItem(TFarDialogItem *Item, void *Arg)
{
    Item->SetVisible(*static_cast<bool *>(Arg));
}
//---------------------------------------------------------------------------
void TFarDialog::EnableItem(TFarDialogItem *Item, void *Arg)
{
    Item->SetEnabled(*static_cast<bool *>(Arg));
}
//---------------------------------------------------------------------------
void TFarDialog::SetItemFocused(TFarDialogItem * const &value)
{
    if (value != GetItemFocused())
    {
        assert(value);
        value->SetFocus();
    }
}
//---------------------------------------------------------------------------
std::wstring TFarDialog::GetMsg(int MsgId)
{
    return FFarPlugin->GetMsg(MsgId);
}
//---------------------------------------------------------------------------
void TFarDialog::LockChanges()
{
    assert(FChangesLocked < 10);
    FChangesLocked++;
    if (FChangesLocked == 1)
    {
        assert(!FChangesPending);
        if (GetHandle())
        {
            SendMessage(DM_ENABLEREDRAW, false, 0);
        }
    }
}
//---------------------------------------------------------------------------
void TFarDialog::UnlockChanges()
{
    assert(FChangesLocked > 0);
    FChangesLocked--;
    if (FChangesLocked == 0)
    {
        BOOST_SCOPE_EXIT ( (&Self) )
        {
            if (Self->GetHandle())
            {
                Self->SendMessage(DM_ENABLEREDRAW, true, 0);
            }
        } BOOST_SCOPE_EXIT_END
        if (FChangesPending)
        {
            FChangesPending = false;
            Change();
        }
    }
}
//---------------------------------------------------------------------------
bool TFarDialog::ChangesLocked()
{
    return (FChangesLocked > 0);
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TFarDialogContainer::TFarDialogContainer(TFarDialog *ADialog) :
    TObject(),
    FLeft(0),
    FTop(0),
    FItems(NULL),
    FDialog(NULL),
    FEnabled(false)
{
    assert(ADialog);

    FItems = new TObjectList();
    FItems->SetOwnsObjects(false);
    FDialog = ADialog;
    FEnabled = true;

    GetDialog()->Add(this);
    GetDialog()->GetNextItemPosition(FLeft, FTop);
}
//---------------------------------------------------------------------------
TFarDialogContainer::~TFarDialogContainer()
{
    delete FItems;
}
//---------------------------------------------------------------------------
std::wstring TFarDialogContainer::GetMsg(int MsgId)
{
    return GetDialog()->GetMsg(MsgId);
}
//---------------------------------------------------------------------------
void TFarDialogContainer::Add(TFarDialogItem *Item)
{
    assert(FItems->IndexOf(Item) < 0);
    Item->SetContainer(this);
    FItems->Add(Item);
}
//---------------------------------------------------------------------------
void TFarDialogContainer::Remove(TFarDialogItem *Item)
{
    assert(FItems->IndexOf(Item) >= 0);
    Item->SetContainer(NULL);
    FItems->Remove(Item);
    if (FItems->GetCount() == 0)
    {
        delete this;
    }
}
//---------------------------------------------------------------------------
void TFarDialogContainer::SetPosition(int Index, int value)
{
    int &Position = Index ? FTop : FLeft;
    if (Position != value)
    {
        Position = value;
        for (int Index = 0; Index < GetItemCount(); Index++)
        {
            dynamic_cast<TFarDialogItem *>((*FItems)[Index])->DialogResized();
        }
    }
}
//---------------------------------------------------------------------------
void TFarDialogContainer::Change()
{
}
//---------------------------------------------------------------------------
void TFarDialogContainer::SetEnabled(bool value)
{
    if (FEnabled != value)
    {
        FEnabled = true;
        for (int Index = 0; Index < GetItemCount(); Index++)
        {
            dynamic_cast<TFarDialogItem *>((*FItems)[Index])->UpdateEnabled();
        }
    }
}
//---------------------------------------------------------------------------
size_t TFarDialogContainer::GetItemCount() const
{
    return FItems->GetCount();
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TFarDialogItem::TFarDialogItem(TFarDialog *ADialog, int AType) :
    TObject(),
    FDefaultType(0),
    FGroup(0),
    FTag(0),
    FDialog(NULL),
    FEnabledFollow(NULL),
    FEnabledDependency(NULL),
    FEnabledDependencyNegative(NULL),
    FContainer(NULL),
    FItem(-1),
    FEnabled(false),
    FIsEnabled(false),
    FColors(0),
    FColorMask(0),
    FOem(false)
{
    assert(ADialog);
    FDialog = ADialog;
    FDefaultType = AType;
    FTag = 0;
    FOem = false;
    FGroup = 0;
    FEnabled = true;
    FIsEnabled = true;
    // FOnMouseClick = NULL;
    FColors = 0;
    FColorMask = 0;

    GetDialog()->Add(this);

    GetDialogItem()->Type = AType;
}
//---------------------------------------------------------------------------
TFarDialogItem::~TFarDialogItem()
{
    assert(!GetDialog());
}
//---------------------------------------------------------------------------
FarDialogItem *TFarDialogItem::GetDialogItem()
{
    assert(GetDialog());
    return &GetDialog()->FDialogItems[GetItem()];
}
//---------------------------------------------------------------------------
void TFarDialogItem::SetBounds(TRect value)
{
    if (FBounds != value)
    {
        FBounds = value;
        UpdateBounds();
    }
}
//---------------------------------------------------------------------------
void TFarDialogItem::Detach()
{
    FDialog = NULL;
}
//---------------------------------------------------------------------------
void TFarDialogItem::DialogResized()
{
    UpdateBounds();
}
//---------------------------------------------------------------------------
void TFarDialogItem::ResetBounds()
{
    TRect B = FBounds;
    FarDialogItem *DItem = GetDialogItem();
    // DEBUG_PRINTF(L"this = %x, DItem = %x, GetContainer = %x", this, DItem, GetContainer());
#define BOUND(DIB, BB, DB, CB) DItem->DIB = B.BB >= 0 ? \
    (GetContainer() ? GetContainer()->CB : 0) + B.BB : GetDialog()->GetSize().DB + B.BB
    BOUND(X1, Left, x, GetLeft());
    BOUND(Y1, Top, y, GetTop());
    BOUND(X2, Right, x, GetLeft());
    BOUND(Y2, Bottom, y, GetTop());
#undef BOUND
}
//---------------------------------------------------------------------------
void TFarDialogItem::UpdateBounds()
{
    ResetBounds();

    if (GetDialog()->GetHandle())
    {
        TRect B = GetActualBounds();
        SMALL_RECT Rect;
        Rect.Left = (short int)B.Left;
        Rect.Top = (short int)B.Top;
        Rect.Right = (short int)B.Right;
        Rect.Bottom = (short int)B.Bottom;
        SendMessage(DM_SETITEMPOSITION, (int)&Rect);
    }
}
//---------------------------------------------------------------------------
char TFarDialogItem::GetColor(int Index)
{
    return *(((char *)&FColors) + Index);
}
//---------------------------------------------------------------------------
void TFarDialogItem::SetColor(int Index, char value)
{
    if (GetColor(Index) != value)
    {
        *(((char *)&FColors) + Index) = value;
        FColorMask |= (0xFF << (Index * 8));
    }
}
//---------------------------------------------------------------------------
void TFarDialogItem::SetFlags(unsigned int value)
{
    if (GetFlags() != value)
    {
        assert(!GetDialog()->GetHandle());
        UpdateFlags(value);
    }
}
//---------------------------------------------------------------------------
void TFarDialogItem::UpdateFlags(unsigned int value)
{
    if (GetFlags() != value)
    {
        GetDialogItem()->Flags = value;
        DialogChange();
    }
}
//---------------------------------------------------------------------------
TRect TFarDialogItem::GetActualBounds()
{
    return TRect(GetDialogItem()->X1, GetDialogItem()->Y1,
                 GetDialogItem()->X2, GetDialogItem()->Y2);
}
//---------------------------------------------------------------------------
unsigned int TFarDialogItem::GetFlags()
{
    return GetDialogItem()->Flags;
}
//---------------------------------------------------------------------------
void TFarDialogItem::SetDataInternal(const std::wstring value)
{
    // DEBUG_PRINTF(L"value = %s", value.c_str());
    // DEBUG_PRINTF(L"GetDialogItem()->PtrData = %s", GetDialogItem()->PtrData);
    std::wstring FarData = value.c_str();
    // DEBUG_PRINTF(L"FarData = %s, GetOem = %d", FarData.c_str(), GetOem());
    if (!GetOem())
    {
        StrToFar(FarData);
    }
    if (GetDialog()->GetHandle())
    {
        // DEBUG_PRINTF(L"DM_SETTEXTPTR");
        SendMessage(DM_SETTEXTPTR, (int)FarData.c_str());
    }
    GetDialogItem()->PtrData = TCustomFarPlugin::DuplicateStr(FarData, true);
    // GetDialogItem()->MaxLen = FarData.size();
    // DEBUG_PRINTF(L"GetDialogItem()->PtrData = %s", GetDialogItem()->PtrData);
    DialogChange();
}
//---------------------------------------------------------------------------
void TFarDialogItem::SetData(const std::wstring value)
{
    if (GetData() != value)
    {
        SetDataInternal(value);
    }
}
//---------------------------------------------------------------------------
void TFarDialogItem::UpdateData(const std::wstring value)
{
    std::wstring FarData = value.c_str();
    if (!GetOem())
    {
        StrToFar(FarData);
    }
    GetDialogItem()->PtrData = TCustomFarPlugin::DuplicateStr(FarData, true);
    // GetDialogItem()->MaxLen = FarData.size();
}
//---------------------------------------------------------------------------
std::wstring TFarDialogItem::GetData()
{
    // DEBUG_PRINTF(L"GetItem = %d", GetItem());
    // DEBUG_PRINTF(L"GetDialogItem = %x", GetDialogItem());
    std::wstring Result;
    if (GetDialogItem()->PtrData)
        Result = GetDialogItem()->PtrData;
    if (!GetOem())
    {
        StrFromFar(Result);
    }
    return Result;
}
//---------------------------------------------------------------------------
void TFarDialogItem::SetType(int value)
{
    if (GetType() != value)
    {
        assert(!GetDialog()->GetHandle());
        GetDialogItem()->Type = value;
    }
}
//---------------------------------------------------------------------------
int TFarDialogItem::GetType()
{
    return GetDialogItem()->Type;
}
//---------------------------------------------------------------------------
void TFarDialogItem::SetAlterType(int Index, bool value)
{
    if (GetAlterType(Index) != value)
    {
        SetType(value ? Index : FDefaultType);
    }
}
//---------------------------------------------------------------------------
bool TFarDialogItem::GetAlterType(int Index)
{
    return (GetType() == Index);
}
//---------------------------------------------------------------------------
bool TFarDialogItem::GetFlag(int Index)
{
    bool Result = (GetFlags() & (Index & 0xFFFFFF00UL)) != 0;
    if (Index & 0x000000FFUL)
    {
        Result = !Result;
    }
    return Result;
}
//---------------------------------------------------------------------------
void TFarDialogItem::SetFlag(int Index, bool value)
{
    if (GetFlag(Index) != value)
    {
        if (Index & DIF_INVERSE)
        {
            value = !value;
        }

        unsigned long F = GetFlags();
        unsigned long Flag = Index & 0xFFFFFF00UL;
        bool ToHandle = true;

        switch (Flag)
        {
        case DIF_DISABLE:
            if (GetDialog()->GetHandle())
            {
                SendMessage(DM_ENABLE, !value);
            }
            break;

        case DIF_HIDDEN:
            if (GetDialog()->GetHandle())
            {
                SendMessage(DM_SHOWITEM, !value);
            }
            break;

        case DIF_3STATE:
            if (GetDialog()->GetHandle())
            {
                SendMessage(DM_SET3STATE, value);
            }
            break;
        }

        if (ToHandle)
        {
            if (value)
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
//---------------------------------------------------------------------------
void TFarDialogItem::SetEnabledFollow(TFarDialogItem *value)
{
    if (GetEnabledFollow() != value)
    {
        FEnabledFollow = value;
        Change();
    }
}
//---------------------------------------------------------------------------
void TFarDialogItem::SetEnabledDependency(TFarDialogItem *value)
{
    if (GetEnabledDependency() != value)
    {
        FEnabledDependency = value;
        Change();
    }
}
//---------------------------------------------------------------------------
void TFarDialogItem::SetEnabledDependencyNegative(TFarDialogItem *value)
{
    if (GetEnabledDependencyNegative() != value)
    {
        FEnabledDependencyNegative = value;
        Change();
    }
}
//---------------------------------------------------------------------------
bool TFarDialogItem::GetIsEmpty()
{
    return GetData().empty();
}
//---------------------------------------------------------------------------
long TFarDialogItem::FailItemProc(int Msg, long Param)
{
    long Result;
    switch (Msg)
    {
    case DN_KILLFOCUS:
        Result = GetItem();
        break;

    default:
        Result = DefaultItemProc(Msg, Param);
        break;
    }
    return Result;
}
//---------------------------------------------------------------------------
long TFarDialogItem::ItemProc(int Msg, long Param)
{
    long Result;
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
    else if (Msg == DN_MOUSECLICK)
    {
        MOUSE_EVENT_RECORD *Event = reinterpret_cast<MOUSE_EVENT_RECORD *>(Param);
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
        Result &= ~FColorMask;
        Result |= (FColors & FColorMask);
    }
    return Result;
}
//---------------------------------------------------------------------------
void TFarDialogItem::DoFocus()
{
}
//---------------------------------------------------------------------------
void TFarDialogItem::DoExit()
{
    if (!FOnExit.empty())
    {
        FOnExit(this);
    }
}
//---------------------------------------------------------------------------
long TFarDialogItem::DefaultItemProc(int Msg, int Param)
{
    TFarEnvGuard Guard;
    return GetDialog()->GetFarPlugin()->FStartupInfo.DefDlgProc(GetDialog()->GetHandle(), Msg, GetItem(), Param);
}
//---------------------------------------------------------------------------
long TFarDialogItem::DefaultDialogProc(int Msg, int Param1, int Param2)
{
    TFarEnvGuard Guard;
    return GetDialog()->GetFarPlugin()->FStartupInfo.DefDlgProc(GetDialog()->GetHandle(), Msg, Param1, Param2);
}
//---------------------------------------------------------------------------
void TFarDialogItem::Change()
{
    if (GetEnabledFollow() || GetEnabledDependency() || GetEnabledDependencyNegative())
    {
        UpdateEnabled();
    }
}
//---------------------------------------------------------------------------
void TFarDialogItem::SetEnabled(bool value)
{
    if (GetEnabled() != value)
    {
        FEnabled = value;
        UpdateEnabled();
    }
}
//---------------------------------------------------------------------------
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
//---------------------------------------------------------------------------
void TFarDialogItem::DialogChange()
{
    assert(GetDialog());
    GetDialog()->Change();
}
//---------------------------------------------------------------------------
long TFarDialogItem::SendDialogMessage(int Msg, int Param1, int Param2)
{
    return GetDialog()->SendMessage(Msg, Param1, Param2);
}
//---------------------------------------------------------------------------
long TFarDialogItem::SendMessage(int Msg, int Param)
{
    return GetDialog()->SendMessage(Msg, GetItem(), Param);
}
//---------------------------------------------------------------------------
void TFarDialogItem::SetSelected(int value)
{
    if (GetSelected() != value)
    {
        if (GetDialog()->GetHandle())
        {
            SendMessage(DM_SETCHECK, value);
        }
        UpdateSelected(value);
    }
}
//---------------------------------------------------------------------------
void TFarDialogItem::UpdateSelected(int value)
{
    if (GetSelected() != value)
    {
        GetDialogItem()->Selected = value;
        DialogChange();
    }
}
//---------------------------------------------------------------------------
int TFarDialogItem::GetSelected()
{
    return GetDialogItem()->Selected;
}
//---------------------------------------------------------------------------
void TFarDialogItem::SetChecked(bool value)
{
    SetSelected(value ? BSTATE_CHECKED : BSTATE_UNCHECKED);
}
//---------------------------------------------------------------------------
bool TFarDialogItem::GetChecked()
{
    return GetSelected() == BSTATE_CHECKED;
}
//---------------------------------------------------------------------------
void TFarDialogItem::Move(int DeltaX, int DeltaY)
{
    TRect R = GetBounds();

    R.Left += DeltaX;
    R.Right += DeltaX;
    R.Top += DeltaY;
    R.Bottom += DeltaY;

    SetBounds(R);
}
//---------------------------------------------------------------------------
void TFarDialogItem::MoveAt(int X, int Y)
{
    Move(X - GetLeft(), Y - GetTop());
}
//---------------------------------------------------------------------------
void TFarDialogItem::SetCoordinate(int Index, int value)
{
    assert(sizeof(TRect) == sizeof(long) * 4);
    TRect R = GetBounds();
    long *D = (long *)&R;
    D += Index;
    *D = value;
    SetBounds(R);
}
//---------------------------------------------------------------------------
int TFarDialogItem::GetCoordinate(int Index)
{
    assert(sizeof(TRect) == sizeof(long) * 4);
    TRect R = GetBounds();
    long *D = (long *)&R;
    D += Index;
    return *D;
}
//---------------------------------------------------------------------------
void TFarDialogItem::SetWidth(int value)
{
    TRect R = GetBounds();
    if (R.Left >= 0)
    {
        R.Right = R.Left + value - 1;
    }
    else
    {
        assert(R.Right < 0);
        R.Left = R.Right - value + 1;
    }
    SetBounds(R);
}
//---------------------------------------------------------------------------
int TFarDialogItem::GetWidth()
{
    return GetActualBounds().Width() + 1;
}
//---------------------------------------------------------------------------
void TFarDialogItem::SetHeight(int value)
{
    TRect R = GetBounds();
    if (R.Top >= 0)
    {
        R.Bottom = R.Top + value - 1;
    }
    else
    {
        assert(R.Bottom < 0);
        R.Top = R.Bottom - value + 1;
    }
    SetBounds(R);
}
//---------------------------------------------------------------------------
int TFarDialogItem::GetHeight()
{
    return GetActualBounds().Height() + 1;
}
//---------------------------------------------------------------------------
bool TFarDialogItem::CanFocus()
{
    int Type = GetType();
    return GetVisible() && GetEnabled() && GetTabStop() &&
           (Type == DI_EDIT || Type == DI_PSWEDIT || Type == DI_FIXEDIT ||
            Type == DI_BUTTON || Type == DI_CHECKBOX || Type == DI_RADIOBUTTON ||
            Type == DI_COMBOBOX || Type == DI_LISTBOX || Type == DI_USERCONTROL);
}
//---------------------------------------------------------------------------
bool TFarDialogItem::Focused()
{
    return GetDialogItem()->Focus != 0;
}
//---------------------------------------------------------------------------
void TFarDialogItem::UpdateFocused(bool value)
{
    GetDialogItem()->Focus = value;
    assert(GetDialog());
    GetDialog()->SetItemFocused(value ? this : NULL);
}
//---------------------------------------------------------------------------
void TFarDialogItem::SetFocus()
{
    assert(CanFocus());
    if (!Focused())
    {
        if (GetDialog()->GetHandle())
        {
            SendMessage(DM_SETFOCUS, 0);
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
//---------------------------------------------------------------------------
void TFarDialogItem::Init()
{
    if (GetFlag(DIF_CENTERGROUP))
    {
        SMALL_RECT Rect;

        // at least for "text" item, returned item size is not correct (on 1.70 final)
        SendMessage(DM_GETITEMPOSITION, (int)&Rect);

        TRect B = GetBounds();
        B.Left = Rect.Left;
        B.Right = Rect.Right;
        SetBounds(B);
    }
}
//---------------------------------------------------------------------------
bool TFarDialogItem::CloseQuery()
{
    if (Focused() && (GetDialog()->GetResult() >= 0))
    {
        DoExit();
    }
    return true;
}
//---------------------------------------------------------------------------
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
                     Event->dwMousePosition.X - GetDialog()->GetBounds().Left - GetLeft(),
                     Event->dwMousePosition.Y - GetDialog()->GetBounds().Top - GetTop());
    }
    return Result;
}
//---------------------------------------------------------------------------
bool TFarDialogItem::MouseClick(MOUSE_EVENT_RECORD *Event)
{
    if (!FOnMouseClick.empty())
    {
        FOnMouseClick(this, Event);
    }
    return DefaultItemProc(DN_MOUSECLICK, reinterpret_cast<long>(Event));
}
//---------------------------------------------------------------------------
bool TFarDialogItem::MouseMove(int /*X*/, int /*Y*/,
        MOUSE_EVENT_RECORD *Event)
{
    return DefaultDialogProc(DN_MOUSEEVENT, 0, reinterpret_cast<long>(Event));
}
//---------------------------------------------------------------------------
void TFarDialogItem::Text(int X, int Y, int Color, std::wstring Str, bool AOem)
{
    if (!AOem && !GetOem())
    {
        StrToFar(Str);
    }
    TFarEnvGuard Guard;
    GetDialog()->GetFarPlugin()->FStartupInfo.Text(
        GetDialog()->GetBounds().Left + GetLeft() + X, GetDialog()->GetBounds().Top + GetTop() + Y,
        Color, Str.c_str());
}
//---------------------------------------------------------------------------
void TFarDialogItem::Redraw()
{
    // do not know how to force redraw of the item only
    GetDialog()->Redraw();
}
//---------------------------------------------------------------------------
void TFarDialogItem::SetContainer(TFarDialogContainer *value)
{
    if (GetContainer() != value)
    {
        TFarDialogContainer *PrevContainer = GetContainer();
        FContainer = value;
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
//---------------------------------------------------------------------------
bool TFarDialogItem::HotKey(char /*HotKey*/)
{
    return false;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TFarBox::TFarBox(TFarDialog *ADialog) :
    TFarDialogItem(ADialog, DI_SINGLEBOX)
{
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TFarButton::TFarButton(TFarDialog *ADialog) :
    TFarDialogItem(ADialog, DI_BUTTON)
{
    FResult = 0;
    FBrackets = brNormal;
}
//---------------------------------------------------------------------------
void TFarButton::SetDataInternal(const std::wstring value)
{
    std::wstring AValue;
    switch (FBrackets)
    {
    case brTight:
        AValue = L"[" + value + L"]";
        break;

    case brSpace:
        AValue = L" " + value + L" ";
        break;

    default:
        AValue = value;
        break;
    }

    TFarDialogItem::SetDataInternal(AValue);

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
        SetWidth(Margin + StripHotKey(AValue).size() + Margin);
    }
}
//---------------------------------------------------------------------------
std::wstring TFarButton::GetData()
{
    std::wstring Result = TFarDialogItem::GetData();
    if ((FBrackets == brTight) || (FBrackets == brSpace))
    {
        bool HasBrackets = (Result.size() >= 2) &&
            (Result[0] == ((FBrackets == brSpace) ? L' ' : L'[')) &&
            (Result[Result.size() - 1] == ((FBrackets == brSpace) ? L' ' : L']'));
        assert(HasBrackets);
        if (HasBrackets)
        {
            Result = Result.substr(1, Result.size() - 2);
        }
    }
    return Result;
}
//---------------------------------------------------------------------------
void TFarButton::SetDefault(bool value)
{
    if (GetDefault() != value)
    {
        assert(!GetDialog()->GetHandle());
        GetDialogItem()->DefaultButton = value;
        if (value)
        {
            if (GetDialog()->GetDefaultButton() && (GetDialog()->GetDefaultButton() != this))
            {
                GetDialog()->GetDefaultButton()->SetDefault(false);
            }
            GetDialog()->FDefaultButton = this;
        }
        else if (GetDialog()->GetDefaultButton() == this)
        {
            GetDialog()->FDefaultButton = NULL;
        }
        DialogChange();
    }
}
//---------------------------------------------------------------------------
bool TFarButton::GetDefault()
{
    return GetDialogItem()->DefaultButton;
}
//---------------------------------------------------------------------------
void TFarButton::SetBrackets(TFarButtonBrackets value)
{
    if (FBrackets != value)
    {
        std::wstring AData = GetData();
        SetFlag(DIF_NOBRACKETS, (value != brNormal));
        FBrackets = value;
        SetDataInternal(AData);
    }
}
//---------------------------------------------------------------------------
long TFarButton::ItemProc(int Msg, long Param)
{
    if (Msg == DN_BTNCLICK)
    {
        if (!GetEnabled())
        {
            return true;
        }
        else
        {
            bool Close = (GetResult() != 0);
            if (!FOnClick.empty())
            {
                FOnClick(this, Close);
            }
            if (!Close)
            {
                return true;
            }
        }
    }
    return TFarDialogItem::ItemProc(Msg, Param);
}
//---------------------------------------------------------------------------
bool TFarButton::HotKey(char HotKey)
{
    int P = GetCaption().find_first_of(L"&");
    bool Result =
        GetVisible() && GetEnabled() &&
        (P > 0) && (P < GetCaption().size()) &&
        (GetCaption()[P + 1] == HotKey);
    if (Result)
    {
        bool Close = (GetResult() != 0);
        if (!FOnClick.empty())
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
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TFarCheckBox::TFarCheckBox(TFarDialog *ADialog) :
    TFarDialogItem(ADialog, DI_CHECKBOX)
{
}
//---------------------------------------------------------------------------
long TFarCheckBox::ItemProc(int Msg, long Param)
{
    if (Msg == DN_BTNCLICK)
    {
        bool Allow = true;
        if (!FOnAllowChange.empty())
        {
            FOnAllowChange(this, Param, Allow);
        }
        if (Allow)
        {
            UpdateSelected(Param);
        }
        return Allow;
    }
    else
    {
        return TFarDialogItem::ItemProc(Msg, Param);
    }
}
//---------------------------------------------------------------------------
bool TFarCheckBox::GetIsEmpty()
{
    return GetChecked() != BSTATE_CHECKED;
}
//---------------------------------------------------------------------------
void TFarCheckBox::SetData(const std::wstring value)
{
    TFarDialogItem::SetData(value);
    if (GetLeft() >= 0 || GetRight() >= 0)
    {
        SetWidth(4 + StripHotKey(value).size());
    }
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TFarRadioButton::TFarRadioButton(TFarDialog *ADialog) :
    TFarDialogItem(ADialog, DI_RADIOBUTTON)
{
}
//---------------------------------------------------------------------------
long TFarRadioButton::ItemProc(int Msg, long Param)
{
    if (Msg == DN_BTNCLICK)
    {
        bool Allow = true;
        if (!FOnAllowChange.empty())
        {
            FOnAllowChange(this, Param, Allow);
        }
        if (Allow)
        {
            // FAR WORKAROUND
            // This does not correspond to FAR API Manual, but it works so.
            // Manual says that Param should contain ID of previously selected dialog item
            UpdateSelected(Param);
        }
        return Allow;
    }
    else
    {
        return TFarDialogItem::ItemProc(Msg, Param);
    }
}
//---------------------------------------------------------------------------
bool TFarRadioButton::GetIsEmpty()
{
    return !GetChecked();
}
//---------------------------------------------------------------------------
void TFarRadioButton::SetData(const std::wstring value)
{
    TFarDialogItem::SetData(value);
    if (GetLeft() >= 0 || GetRight() >= 0)
    {
        SetWidth(4 + StripHotKey(value).size());
    }
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TFarEdit::TFarEdit(TFarDialog *ADialog) :
    TFarDialogItem(ADialog, DI_EDIT)
{
    SetAutoSelect(false);
}
//---------------------------------------------------------------------------
void TFarEdit::Detach()
{
    delete[] GetDialogItem()->Mask;
    TFarDialogItem::Detach();
}
//---------------------------------------------------------------------------
long TFarEdit::ItemProc(int Msg, long Param)
{
    if (Msg == DN_EDITCHANGE)
    {
        std::wstring Data = ((FarDialogItem *)Param)->PtrData;
        GetDialogItem()->PtrData = TCustomFarPlugin::DuplicateStr(Data, true);
        // GetDialogItem()->MaxLen = Data.size();
    }
    return TFarDialogItem::ItemProc(Msg, Param);
}
//---------------------------------------------------------------------------
std::wstring TFarEdit::GetHistoryMask(int Index)
{
    std::wstring Result =
        ((Index == 0) && (GetFlags() & DIF_HISTORY)) ||
        ((Index == 1) && (GetFlags() & DIF_MASKEDIT)) ? GetDialogItem()->Mask : L"";
    if (!GetOem())
    {
        StrFromFar(Result);
    }
    return Result;
}
//---------------------------------------------------------------------------
void TFarEdit::SetHistoryMask(int Index, std::wstring value)
{
    if (GetHistoryMask(Index) != value)
    {
        assert(!GetDialog()->GetHandle());
        assert(&GetDialogItem()->Mask == &GetDialogItem()->History);

        delete[] GetDialogItem()->Mask;
        if (value.empty())
        {
            GetDialogItem()->Mask = NULL;
        }
        else
        {
            GetDialogItem()->Mask = TCustomFarPlugin::DuplicateStr(value);
            if (!GetOem())
            {
                StrToFar((wchar_t *)GetDialogItem()->Mask);
            }
        }
        bool PrevHistory = !GetHistory().empty();
        SetFlag(DIF_HISTORY, (Index == 0) && !value.empty());
        bool Masked = (Index == 1) && !value.empty();
        SetFlag(DIF_MASKEDIT, Masked);
        if (Masked)
        {
            SetFixed(true);
        }
        bool CurrHistory = !GetHistory().empty();
        if (PrevHistory != CurrHistory)
        {
            // add/remove space for history arrow
            SetWidth(GetWidth() + (CurrHistory ? -1 : 1));
        }
        DialogChange();
    }
}
//---------------------------------------------------------------------------
void TFarEdit::SetAsInteger(int value)
{
    // DEBUG_PRINTF(L"GetText = %s, value = %d", GetText().c_str(), value);
    int Int = ::StrToIntDef(::Trim(GetText()), 0);
    if (!Int || (GetAsInteger() != value))
    {
        SetText(::IntToStr(value));
        DialogChange();
    }
}
//---------------------------------------------------------------------------
int TFarEdit::GetAsInteger()
{
    // DEBUG_PRINTF(L"GetText = %s, StrToIntDef = %d", GetText().c_str(), ::StrToIntDef(GetText(), 0));
    return ::StrToIntDef(::Trim(GetText()), 0);
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TFarSeparator::TFarSeparator(TFarDialog *ADialog) :
    TFarDialogItem(ADialog, DI_TEXT)
{
    SetLeft(-1);
    SetFlag(DIF_SEPARATOR, true);
}
//---------------------------------------------------------------------------
void TFarSeparator::ResetBounds()
{
    TFarDialogItem::ResetBounds();
    if (GetBounds().Left < 0)
    {
        GetDialogItem()->X1 = -1;
    }
}
//---------------------------------------------------------------------------
void TFarSeparator::SetDouble(bool value)
{
    if (GetDouble() != value)
    {
        assert(!GetDialog()->GetHandle());
        SetFlag(DIF_SEPARATOR, !value);
        SetFlag(DIF_SEPARATOR2, value);
    }
}
//---------------------------------------------------------------------------
bool TFarSeparator::GetDouble()
{
    return GetFlag(DIF_SEPARATOR2);
}
//---------------------------------------------------------------------------
void TFarSeparator::SetPosition(int value)
{
    TRect R = GetBounds();
    R.Top = value;
    R.Bottom = value;
    SetBounds(R);
}
//---------------------------------------------------------------------------
int TFarSeparator::GetPosition()
{
    return GetBounds().Top;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TFarText::TFarText(TFarDialog *ADialog) :
    TFarDialogItem(ADialog, DI_TEXT)
{
}
//---------------------------------------------------------------------------
void TFarText::SetData(const std::wstring value)
{
    TFarDialogItem::SetData(value);
    if (GetLeft() >= 0 || GetRight() >= 0)
    {
        SetWidth(StripHotKey(value).size());
    }
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TFarList::TFarList(TFarDialogItem *ADialogItem) :
    TStringList()
{
    assert((ADialogItem == NULL) ||
           (ADialogItem->GetType() == DI_COMBOBOX) || (ADialogItem->GetType() == DI_LISTBOX));
    Self = this;
    FDialogItem = ADialogItem;
    FListItems = new FarList;
    memset(FListItems, 0, sizeof(*FListItems));
    FNoDialogUpdate = false;
}
//---------------------------------------------------------------------------
TFarList::~TFarList()
{
    delete[] FListItems->Items;
    delete FListItems;
}
//---------------------------------------------------------------------------
void TFarList::Assign(TPersistent *Source)
{
    TStringList::Assign(Source);

    TFarList *FarList = dynamic_cast<TFarList *>(Source);
    if (FarList != NULL)
    {
        for (int Index = 0; Index < FarList->GetCount(); Index++)
        {
            SetFlags(Index, FarList->GetFlags(Index));
        }
    }
}
//---------------------------------------------------------------------------
void TFarList::UpdateItem(int Index)
{
    FarListItem *ListItem = &FListItems->Items[Index];
    std::wstring value = GetString(Index).c_str();
    ListItem->Text = TCustomFarPlugin::DuplicateStr(value, true);
    if (!GetDialogItem()->GetOem())
    {
        StrToFar(std::wstring(ListItem->Text));
    }

    FarListUpdate ListUpdate;
    memset(&ListUpdate, 0, sizeof(ListUpdate));
    ListUpdate.Index = Index;
    ListUpdate.Item = *ListItem;
    GetDialogItem()->SendMessage(DM_LISTUPDATE, (int)&ListUpdate);
}
//---------------------------------------------------------------------------
void TFarList::Put(int Index, const std::wstring S)
{
    if ((GetDialogItem() != NULL) && GetDialogItem()->GetDialog()->GetHandle())
    {
        FNoDialogUpdate = true;
        {
            BOOST_SCOPE_EXIT ( (&Self) )
            {
                Self->FNoDialogUpdate = false;
            } BOOST_SCOPE_EXIT_END
            TStringList::PutString(Index, S);
            if (GetUpdateCount() == 0)
            {
                UpdateItem(Index);
            }
        }
    }
    else
    {
        TStringList::PutString(Index, S);
    }
}
//---------------------------------------------------------------------------
void TFarList::Changed()
{
    TStringList::Changed();

    if ((GetUpdateCount() == 0) && !FNoDialogUpdate)
    {
        int PrevSelected = 0;
        int PrevTopIndex = 0;
        if ((GetDialogItem() != NULL) && GetDialogItem()->GetDialog()->GetHandle())
        {
            PrevSelected = GetSelected();
            PrevTopIndex = GetTopIndex();
        }
        if (FListItems->ItemsNumber != GetCount())
        {
            FarListItem *Items = FListItems->Items;
            if (GetCount())
            {
                FListItems->Items = new FarListItem[GetCount()];
                for (int Index = 0; Index < GetCount(); Index++)
                {
                    memset(&FListItems->Items[Index], 0, sizeof(FListItems->Items[Index]));
                    if (Index < FListItems->ItemsNumber)
                    {
                        FListItems->Items[Index].Flags = Items[Index].Flags;
                    }
                }
            }
            else
            {
                FListItems->Items = NULL;
            }
            delete[] Items;
            FListItems->ItemsNumber = GetCount();
        }
        for (int i = 0; i < GetCount(); i++)
        {
            std::wstring value = GetString(i);
            delete[] FListItems->Items[i].Text;
            FListItems->Items[i].Text = TCustomFarPlugin::DuplicateStr(value);
            if ((GetDialogItem() != NULL) && !GetDialogItem()->GetOem())
            {
                StrToFar(FListItems->Items[i].Text);
            }
        }
        if ((GetDialogItem() != NULL) && GetDialogItem()->GetDialog()->GetHandle())
        {
            GetDialogItem()->GetDialog()->LockChanges();
            {
                BOOST_SCOPE_EXIT ( (&Self) )
                {
                    Self->GetDialogItem()->GetDialog()->UnlockChanges();
                } BOOST_SCOPE_EXIT_END
                GetDialogItem()->SendMessage(DM_LISTSET, (int)FListItems);
                if (PrevTopIndex + GetDialogItem()->GetHeight() > GetCount())
                {
                    PrevTopIndex = GetCount() > GetDialogItem()->GetHeight() ? GetCount() - GetDialogItem()->GetHeight() : 0;
                }
                SetCurPos((PrevSelected >= GetCount()) ? (GetCount() - 1) : PrevSelected,
                          PrevTopIndex);
            }
        }
    }
}
//---------------------------------------------------------------------------
void TFarList::SetSelected(int value)
{
    assert(GetDialogItem() != NULL);
    // DEBUG_PRINTF(L"value = %d", value);
    if (GetSelectedInt(false) != value)
    {
        if (GetDialogItem()->GetDialog()->GetHandle())
        {
            UpdatePosition(value);
        }
        else
        {
            GetDialogItem()->SetData(GetString(value));
        }
    }
}
//---------------------------------------------------------------------------
void TFarList::UpdatePosition(int Position)
{
    if (Position >= 0)
    {
        int ATopIndex = GetTopIndex();
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
//---------------------------------------------------------------------------
void TFarList::SetCurPos(int Position, int TopIndex)
{
    assert(GetDialogItem() != NULL);
    assert(GetDialogItem()->GetDialog()->GetHandle());
    FarListPos ListPos;
    ListPos.SelectPos = Position;
    ListPos.TopPos = TopIndex;
    GetDialogItem()->SendMessage(DM_LISTSETCURPOS, (int)&ListPos);
}
//---------------------------------------------------------------------------
void TFarList::SetTopIndex(int value)
{
    if (value != GetTopIndex())
    {
        SetCurPos(-1, value);
    }
}
//---------------------------------------------------------------------------
int TFarList::GetPosition()
{
    assert(GetDialogItem() != NULL);
    return GetDialogItem()->SendMessage(DM_LISTGETCURPOS, NULL);
}
//---------------------------------------------------------------------------
int TFarList::GetTopIndex()
{
    int Result;
    if (GetCount() == 0)
    {
        Result = -1;
    }
    else
    {
        FarListPos ListPos;
        assert(GetDialogItem() != NULL);
        GetDialogItem()->SendMessage(DM_LISTGETCURPOS, reinterpret_cast<int>(&ListPos));
        Result = ListPos.TopPos;
    }
    return Result;
}
//---------------------------------------------------------------------------
int TFarList::GetMaxLength()
{
    int Result = 0;
    for (int i = 0; i < GetCount(); i++)
    {
        if (Result < GetString(i).size())
        {
            Result = GetString(i).size();
        }
    }
    return Result;
}
//---------------------------------------------------------------------------
int TFarList::GetVisibleCount()
{
    assert(GetDialogItem() != NULL);
    return GetDialogItem()->GetHeight() - (GetDialogItem()->GetFlag(DIF_LISTNOBOX) ? 0 : 2);
}
//---------------------------------------------------------------------------
int TFarList::GetSelectedInt(bool Init)
{
    int Result = -1;
    assert(GetDialogItem() != NULL);
    // DEBUG_PRINTF(L"GetCount = %d, Init = %d", GetCount(), Init);
    if (GetCount() == 0)
    {
        Result = -1;
    }
    else if (GetDialogItem()->GetDialog()->GetHandle() && !Init)
    {
        Result = GetPosition();
    }
    else
    {
        const wchar_t *PtrData = GetDialogItem()->GetDialogItem()->PtrData;
        if (PtrData)
            Result = IndexOf(PtrData);
    }

    return Result;
}
//---------------------------------------------------------------------------
int TFarList::GetSelected()
{
    // DEBUG_PRINTF(L"begin");
    int Result = GetSelectedInt(false);
    // DEBUG_PRINTF(L"Result = %d", Result);

    if ((Result < 0) && (GetCount() > 0))
    {
        Result = 0;
    }

    return Result;
}
//---------------------------------------------------------------------------
unsigned int TFarList::GetFlags(int Index)
{
    return FListItems->Items[Index].Flags;
}
//---------------------------------------------------------------------------
void TFarList::SetFlags(int Index, unsigned int value)
{
    if (FListItems->Items[Index].Flags != value)
    {
        FListItems->Items[Index].Flags = value;
        if ((GetDialogItem() != NULL) && GetDialogItem()->GetDialog()->GetHandle() && (GetUpdateCount() == 0))
        {
            UpdateItem(Index);
        }
    }
}
//---------------------------------------------------------------------------
bool TFarList::GetFlag(int Index, int Flag)
{
    return FLAGSET(GetFlags(Index), static_cast<unsigned int>(Flag));
}
//---------------------------------------------------------------------------
void TFarList::SetFlag(int Index, int Flag, bool value)
{
    SetFlags(Index, (GetFlags(Index) & ~Flag) | FLAGMASK(value, Flag));
}
//---------------------------------------------------------------------------
void TFarList::Init()
{
    // DEBUG_PRINTF(L"begin");
    UpdatePosition(GetSelectedInt(true));
    // DEBUG_PRINTF(L"end");
}
//---------------------------------------------------------------------------
long TFarList::ItemProc(int Msg, long Param)
{
    assert(GetDialogItem() != NULL);
    if (Msg == DN_LISTCHANGE)
    {
        if ((Param < 0) || ((Param == 0) && (GetCount() == 0)))
        {
            GetDialogItem()->UpdateData(L"");
        }
        else
        {
            assert(Param >= 0 && Param < GetCount());
            GetDialogItem()->UpdateData(GetString(Param));
        }
    }
    return false;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TFarListBox::TFarListBox(TFarDialog *ADialog) :
    TFarDialogItem(ADialog, DI_LISTBOX)
{
    FList = new TFarList(this);
    GetDialogItem()->ListItems = FList->GetListItems();
    FAutoSelect = asOnlyFocus;
    // FAR WORKAROUND
    FDenyClose = false;
}
//---------------------------------------------------------------------------
TFarListBox::~TFarListBox()
{
    SAFE_DESTROY(FList);
}
//---------------------------------------------------------------------------
long TFarListBox::ItemProc(int Msg, long Param)
{
    bool Result;
    // FAR WORKAROUND
    // Since 1.70 final, hotkeys do not work when list box has focus.
    if ((Msg == DN_KEY) && ((short int)GetDialog()->GetFarPlugin()->FarVersion() >= (short int)FAR170) && GetDialog()->HotKey(Param))
    {
        Result = true;
    }
    else if (FList->ItemProc(Msg, Param))
    {
        Result = true;
    }
    else
    {
        Result = TFarDialogItem::ItemProc(Msg, Param);
    }

    // FAR WORKAROUND
    if ((Msg == DN_MOUSECLICK) && (GetDialog()->GetItemCount() > 0) &&
            ((short int)GetDialog()->GetFarPlugin()->FarVersion() < (short int)FAR170ALPHA6))
    {
        FDenyClose = true;
    }

    return Result;
}
//---------------------------------------------------------------------------
void TFarListBox::Init()
{
    TFarDialogItem::Init();
    GetItems()->Init();
    UpdateMouseReaction();
}
//---------------------------------------------------------------------------
void TFarListBox::SetAutoSelect(TFarListBoxAutoSelect value)
{
    if (GetAutoSelect() != value)
    {
        FAutoSelect = value;
        if (GetDialog()->GetHandle())
        {
            UpdateMouseReaction();
        }
    }
}
//---------------------------------------------------------------------------
void TFarListBox::UpdateMouseReaction()
{
    SendMessage(DM_LISTSETMOUSEREACTION, GetAutoSelect());
}
//---------------------------------------------------------------------------
void TFarListBox::SetItems(TStrings *value)
{
    FList->Assign(value);
}
//---------------------------------------------------------------------------
void TFarListBox::SetList(TFarList *value)
{
    SetItems(value);
}
//---------------------------------------------------------------------------
bool TFarListBox::CloseQuery()
{
    // FAR WORKAROUND
    if (FDenyClose)
    {
        assert((short int)FarPlugin->FarVersion() < (short int)FAR170ALPHA6);
        FDenyClose = false;
        return false;
    }
    else
    {
        return true;
    }
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TFarComboBox::TFarComboBox(TFarDialog *ADialog) :
    TFarDialogItem(ADialog, DI_COMBOBOX)
{
    FList = new TFarList(this);
    GetDialogItem()->ListItems = FList->GetListItems();
    SetAutoSelect(false);
}
//---------------------------------------------------------------------------
TFarComboBox::~TFarComboBox()
{
    SAFE_DESTROY(FList);
}
//---------------------------------------------------------------------------
void TFarComboBox::ResizeToFitContent()
{
    SetWidth(FList->GetMaxLength());
}
//---------------------------------------------------------------------------
long TFarComboBox::ItemProc(int Msg, long Param)
{
    if (Msg == DN_EDITCHANGE)
    {
        std::wstring Data = ((FarDialogItem *)Param)->PtrData;
        GetDialogItem()->PtrData = TCustomFarPlugin::DuplicateStr(Data, true);
        // GetDialogItem()->MaxLen = Data.size();
    }

    if (FList->ItemProc(Msg, Param))
    {
        return true;
    }
    else
    {
        return TFarDialogItem::ItemProc(Msg, Param);
    }
}
//---------------------------------------------------------------------------
void TFarComboBox::Init()
{
    TFarDialogItem::Init();
    GetItems()->Init();
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TFarLister::TFarLister(TFarDialog *ADialog) :
    TFarDialogItem(ADialog, DI_USERCONTROL),
    FItems(new TStringList()),
    FTopIndex(0)
{
    FItems->SetOnChange(boost::bind(&TFarLister::ItemsChange, this, _1));
}
//---------------------------------------------------------------------------
TFarLister::~TFarLister()
{
    delete FItems;
}
//---------------------------------------------------------------------------
void TFarLister::ItemsChange(TObject * /*Sender*/)
{
    FTopIndex = 0;
    if (GetDialog()->GetHandle())
    {
        Redraw();
    }
}
//---------------------------------------------------------------------------
bool TFarLister::GetScrollBar()
{
    return (GetItems()->GetCount() > GetHeight());
}
//---------------------------------------------------------------------------
void TFarLister::SetTopIndex(int value)
{
    if (GetTopIndex() != value)
    {
        FTopIndex = value;
        Redraw();
    }
}
//---------------------------------------------------------------------------
TStrings *TFarLister::GetItems()
{
    return FItems;
}
//---------------------------------------------------------------------------
void TFarLister::SetItems(TStrings *value)
{
    if (!FItems->Equals(value))
    {
        FItems->Assign(value);
    }
}
//---------------------------------------------------------------------------
void TFarLister::DoFocus()
{
    TFarDialogItem::DoFocus();
    // TODO: hide cursor
}
//---------------------------------------------------------------------------
long TFarLister::ItemProc(int Msg, long Param)
{
    long Result = 0;

    if (Msg == DN_DRAWDLGITEM)
    {
        bool AScrollBar = GetScrollBar();
        int ScrollBarPos = 0;
        if (GetItems()->GetCount() - GetHeight() > 0)
        {
            ScrollBarPos = float(GetHeight() - 3) * (float(FTopIndex) / (GetItems()->GetCount() - GetHeight())) + 1;
        }
        int DisplayWidth = GetWidth() - (AScrollBar ? 1 : 0);
        int Color = GetDialog()->GetSystemColor(
                        FLAGSET(GetDialog()->GetFlags(), FDLG_WARNING) ? COL_WARNDIALOGLISTTEXT : COL_DIALOGLISTTEXT);
        std::wstring Buf;
        for (int Row = 0; Row < GetHeight(); Row++)
        {
            int Index = GetTopIndex() + Row;
            Buf = L" ";
            if (Index < GetItems()->GetCount())
            {
                std::wstring value = GetItems()->GetString(Index).substr(0, DisplayWidth - 1);
                Buf += value;
            }
            std::wstring value = ::StringOfChar(' ', DisplayWidth - Buf.size());
            value.resize(DisplayWidth - Buf.size());
            Buf += value;
            StrToFar(Buf);
            if (AScrollBar)
            {
                // OEM character set (Ansi does not have the ascii art we need)
                if (Row == 0)
                {
                    Buf += '\x1E';
                }
                else if (Row == ScrollBarPos)
                {
                    Buf += '\xB2';
                }
                else if (Row == GetHeight() - 1)
                {
                    Buf += '\x1F';
                }
                else
                {
                    Buf += '\xB0';
                }
            }
            Text(0, Row, Color, Buf, true);
        }
    }
    else if (Msg == DN_KEY)
    {
        Result = true;

        int NewTopIndex = GetTopIndex();
        if ((Param == KEY_UP) || (Param == KEY_LEFT))
        {
            if (NewTopIndex > 0)
            {
                NewTopIndex--;
            }
            else
            {
                long ShiftTab = KEY_SHIFTTAB;
                SendDialogMessage(DM_KEY, 1, reinterpret_cast<int>(&ShiftTab));
            }
        }
        else if ((Param == KEY_DOWN) || (Param == KEY_RIGHT))
        {
            if (NewTopIndex < GetItems()->GetCount() - GetHeight())
            {
                NewTopIndex++;
            }
            else
            {
                long Tab = KEY_TAB;
                SendDialogMessage(DM_KEY, 1, reinterpret_cast<int>(&Tab));
            }
        }
        else if (Param == KEY_PGUP)
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
        else if (Param == KEY_PGDN)
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
        else if (Param == KEY_HOME)
        {
            NewTopIndex = 0;
        }
        else if (Param == KEY_END)
        {
            NewTopIndex = GetItems()->GetCount() - GetHeight();
        }
        else
        {
            Result = TFarDialogItem::ItemProc(Msg, Param);
        }

        SetTopIndex(NewTopIndex);
    }
    else if (Msg == DN_MOUSECLICK)
    {
        if (!Focused() && CanFocus())
        {
            SetFocus();
        }

        MOUSE_EVENT_RECORD *Event = reinterpret_cast<MOUSE_EVENT_RECORD *>(Param);
        TPoint P = MouseClientPosition(Event);

        if (FLAGSET(Event->dwEventFlags, DOUBLE_CLICK) &&
                (P.x < GetWidth() - 1))
        {
            Result = TFarDialogItem::ItemProc(Msg, Param);
        }
        else
        {
            int NewTopIndex = GetTopIndex();

            if (((P.x == GetWidth() - 1) && (P.y == 0)) ||
                    ((P.x < GetWidth() - 1) && (P.y < GetHeight() / 2)))
            {
                if (NewTopIndex > 0)
                {
                    NewTopIndex--;
                }
            }
            else if (((P.x == GetWidth() - 1) && (P.y == GetHeight() - 1)) ||
                     ((P.x < GetWidth() - 1) && (P.y >= GetHeight() / 2)))
            {
                if (NewTopIndex < GetItems()->GetCount() - GetHeight())
                {
                    NewTopIndex++;
                }
            }
            else
            {
                assert(P.x == GetWidth() - 1);
                assert((P.y > 0) && (P.y < GetHeight() - 1));
                NewTopIndex = ceil(float(P.y - 1) / (GetHeight() - 2) * (GetItems()->GetCount() - GetHeight() + 1));
            }

            Result = true;

            SetTopIndex(NewTopIndex);
        }
    }
    else
    {
        Result = TFarDialogItem::ItemProc(Msg, Param);
    }

    return Result;
}
