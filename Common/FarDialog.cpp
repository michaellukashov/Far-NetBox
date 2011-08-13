//---------------------------------------------------------------------------
#include <map>
#include <farkeys.hpp>
#include <farcolor.hpp>
#include <math.h>

#include "stdafx.h"
#include "FarDialog.h"

// #include <Common.h>

//---------------------------------------------------------------------------
string StripHotKey(string Text)
{
    size_t Len = Text.length();
    int Pos = 1;
    while (Pos <= Len)
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
    FBounds(-1, -1, 40, 10)
{
    assert(AFarPlugin);
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
    FBorderBox->Bounds = TRect(3, 1, -4, -2);
    FBorderBox->Double = true;

    // Инициализируем property
    Bounds = property<self, TRect>(this, &self::GetBounds, &self::SetBounds);
    ClientRect = property_ro<self, TRect>(this, &self::GetClientRect);
    HelpTopic = property<self, wstring>(this, &self::GetHelpTopic, &self::SetHelpTopic);
    Flags = property<self, unsigned int>(this, &self::GetFlags, &self::SetFlags);
    Centered = property<self, bool>(this, &self::GetCentered, &self::SetCentered);
    Size = property<self, TPoint>(this, &self::GetSize, &self::SetSize);
    ClientSize = property_ro<self, TPoint>(this, &self::GetClientSize);
    Width = property<self, int>(this, &self::GetWidth, &self::SetWidth);
    Height = property<self, int>(this, &self::GetHeight, &self::SetHeight);
    Caption = property<self, string>(this, &self::GetCaption, &self::SetCaption);
    Handle = property_ro<self, HANDLE>(this, &self::GetHandle);
    DefaultButton = property_ro<self, TFarButton *>(this, &self::GetDefaultButton);
    BorderBox = property_ro<self, TFarBox *>(this, &self::GetBorderBox);
    Item = property_idx<self, TFarDialogItem *>(this, &self::GetItem);
    ItemCount = property_ro<self, size_t>(this, &self::GetItemCount);
    NextItemPosition = property<self, TItemPosition>(this, &self::GetNextItemPosition, &self::SetNextItemPosition);
    DefaultGroup = property<self, int>(this, &self::GetDefaultGroup, &self::SetDefaultGroup);
    Tag = property<self, int>(this, &self::GetTag, &self::SetTag);
    ItemFocused = property<self, TFarDialogItem *>(this, &self::GetItemFocused, &self::SetItemFocused);
    Result = property_ro<self, int>(this, &self::GetResult);
    MaxSize = property_ro<self, TPoint>(this, &self::GetMaxSize);
    
    OnKey = property<self, TFarKeyEvent>(this, &self::GetOnKey, &self::SetOnKey);
}
//---------------------------------------------------------------------------
TFarDialog::~TFarDialog()
{
    for (int i = 0; i < ItemCount; i++)
    {
        Item[i]->Detach();
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
    if (Bounds != value)
    {
        LockChanges();
        try
        {
            FBounds = value;
            if (Handle)
            {
                COORD Coord;
                Coord.X = (short int)Size.get().x;
                Coord.Y = (short int)Size.get().y;
                SendMessage(DM_RESIZEDIALOG, 0, (int)&Coord);
                Coord.X = (short int)FBounds.Left;
                Coord.Y = (short int)FBounds.Top;
                SendMessage(DM_MOVEDIALOG, true, (int)&Coord);
            }
            for (int i = 0; i < ItemCount; i++)
            {
                Item[i]->DialogResized();
            }
        }
        catch (...)
        {
        }
        UnlockChanges();
    }
}
//---------------------------------------------------------------------------
TRect TFarDialog::GetClientRect() const 
{
    TRect R;
    if (FBorderBox)
    {
        R = FBorderBox->Bounds;
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
        TRect R = FBorderBox->ActualBounds;
        S.x = R.Width() + 1;
        S.y = R.Height() + 1;
        S.x -= S.x > 4 ? 4 : S.x;
        S.y -= S.y > 2 ? 2 : S.y;
    }
    else
    {
        S = Size;
    }
    return S;
}
//---------------------------------------------------------------------------
TPoint TFarDialog::GetMaxSize() const 
{
    TPoint P = FarPlugin.get()->TerminalInfo();
    P.x -= 2;
    P.y -= 3;
    return P;
}
//---------------------------------------------------------------------------
void TFarDialog::SetHelpTopic(const wstring &value)
{
    if (HelpTopic != value)
    {
        assert(!Handle);
        FHelpTopic = value;
    }
}
//---------------------------------------------------------------------------
void TFarDialog::SetFlags(const unsigned int &value)
{
    if (Flags != value)
    {
        assert(!Handle);
        FFlags = value;
    }
}
//---------------------------------------------------------------------------
void TFarDialog::SetCentered(const bool &value)
{
    if (Centered != value)
    {
        assert(!Handle);
        TRect B = Bounds;
        B.Left = value ? -1 : 0;
        B.Top = value ? -1 : 0;
        Bounds = B;
    }
}
//---------------------------------------------------------------------------
bool TFarDialog::GetCentered() const
{
    return (Bounds.get().Left < 0) && (Bounds.get().Top < 0);
}
//---------------------------------------------------------------------------
TPoint TFarDialog::GetSize() const
{
    if (Centered)
    {
        return TPoint(Bounds.get().Right, Bounds.get().Bottom);
    }
    else
    {
        return TPoint(Bounds.get().Width() + 1, Bounds.get().Height() + 1);
    }
}
//---------------------------------------------------------------------------
void TFarDialog::SetSize(const TPoint &value)
{
    TRect B = Bounds;
    if (Centered)
    {
        B.Right = value.x;
        B.Bottom = value.y;
    }
    else
    {
        B.Right = FBounds.Left + value.x - 1;
        B.Bottom = FBounds.Top + value.y - 1;
    }
    Bounds = B;
}
//---------------------------------------------------------------------------
void TFarDialog::SetWidth(const int &value)
{
    Size = TPoint(value, Height);
}
//---------------------------------------------------------------------------
int TFarDialog::GetWidth() const
{
    return Size.get().x;
}
//---------------------------------------------------------------------------
void TFarDialog::SetHeight(const int &value)
{
    Size = TPoint(Width, value);
}
//---------------------------------------------------------------------------
int TFarDialog::GetHeight() const
{
    return Size.get().y;
}
//---------------------------------------------------------------------------
void TFarDialog::SetCaption(const string &value)
{
    if (Caption != value)
    {
        FBorderBox->Caption = value;
    }
}
//---------------------------------------------------------------------------
string TFarDialog::GetCaption() const
{
    return FBorderBox->Caption;
}
//---------------------------------------------------------------------------
size_t TFarDialog::GetItemCount() const
{
    return FItems->Count();
}
//---------------------------------------------------------------------------
TFarDialogItem *TFarDialog::GetItem(size_t Index) const
{
    TFarDialogItem *DialogItem;
    if (GetItemCount())
    {
        assert(Index >= 0 && Index < FItems->Count);
        DialogItem = dynamic_cast<TFarDialogItem *>((*Items.get())[Index]);
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
    TRect R = ClientRect;
    int Left, Top;
    GetNextItemPosition(Left, Top);
    R.Left = Left;
    R.Top = Top;

    if (FDialogItemsCapacity == Items.get()->Count())
    {
        int DialogItemsDelta = 10;
        FarDialogItem *NewDialogItems;
        NewDialogItems = new FarDialogItem[Items.get()->Count() + DialogItemsDelta];
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
    DialogItem->FItem = Items.get()->Add(DialogItem);

    R.Bottom = R.Top;
    DialogItem->Bounds = R;
    DialogItem->Group = DefaultGroup;
}
//---------------------------------------------------------------------------
void TFarDialog::Add(TFarDialogContainer *Container)
{
    FContainers->Add(Container);
}
//---------------------------------------------------------------------------
void TFarDialog::GetNextItemPosition(int &Left, int &Top)
{
    TRect R = ClientRect;
    Left = R.Left;
    Top = R.Top;

    TFarDialogItem *LastI = Item[ItemCount-1];
    LastI = LastI == FBorderBox ? NULL : LastI;

    if (LastI)
    {
        switch (NextItemPosition)
        {
        case ipNewLine:
            Top = LastI->Bottom + 1;
            break;

        case ipBelow:
            Top = LastI->Bottom + 1;
            Left = LastI->Left;
            break;

        case ipRight:
            Top = LastI->Top;
            Left = LastI->Right + 3;
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
    long Result;
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
            catch(...)
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
                TFarDialogItem *I = Item[Param1];
                try
                {
                    Result = I->ItemProc(Msg, Param2);
                }
                catch(exception &E)
                {
                    Handled = true;
                    FarPlugin.get()->HandleException(&E);
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
                     ((Param1 >= 0) && (dynamic_cast<TFarButton *>(Item[Param1]) == NULL))) &&
                    DefaultButton.get()->Enabled &&
                    (DefaultButton.get()->OnClick != NULL))
            {
                bool Close = (DefaultButton.get()->Result != 0);
                DefaultButton.get()->OnClick(DefaultButton, Close);
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
                    TFarButton *Button = dynamic_cast<TFarButton *>(Item[Param1]);
                    // FAR WORKAROUND
                    // FAR 1.70 alpha 6 calls DN_CLOSE even for non-button dialog items
                    // (list boxes in particular), while FAR 1.70 beta 5 used ID of
                    // default button in such case.
                    // Particularly for listbox, we can prevent closing dialog using
                    // flag DIF_LISTNOCLOSE.
                    if (Button == NULL)
                    {
                        assert(FarPlugin->FarVersion() >= FAR170ALPHA6);
                        assert(dynamic_cast<TFarListBox *>(Item[Param1]) != NULL);
                        Result = false;
                    }
                    else
                    {
                        FResult = Button->Result;
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
    catch(exception &E)
    {
        FarPlugin.get()->HandleException(&E);
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
    return FarPlugin.get()->FStartupInfo.DefDlgProc(Handle, Msg, Param1, Param2);
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
        int X = Event->dwMousePosition.X - Bounds.get().Left;
        int Y = Event->dwMousePosition.Y - Bounds.get().Top;
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
    if (OnKey)
    {
        ((*this).*OnKey.get())(this, Item, KeyCode, Result);
    }
    return Result;
}
//---------------------------------------------------------------------------
bool TFarDialog::HotKey(unsigned long Key)
{
    bool Result = false;
    char HotKey;
    if ((KEY_ALTA <= Key) && (Key <= KEY_ALTZ))
    {
        Result = true;
        HotKey = char('a' + char(Key - KEY_ALTA));
    }

    if (Result)
    {
        Result = false;
        for (int i = 0; i < ItemCount; i++)
        {
            if (Item[i]->HotKey(HotKey))
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
    for (int i = 0; i < ItemCount; i++)
    {
        TRect Bounds = Item[i]->ActualBounds;
        if ((Bounds.Left <= X) && (X <= Bounds.Right) &&
                (Bounds.Top <= Y) && (Y <= Bounds.Bottom))
        {
            Result = Item[i];
        }
    }
    return Result;
}
//---------------------------------------------------------------------------
bool TFarDialog::CloseQuery()
{
    bool Result = true;
    for (int i = 0; i < ItemCount && Result; i++)
    {
        if (!Item[i]->CloseQuery())
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
    for (int i = 0; i < ItemCount; i++)
    {
        Item[i]->Init();
    }

    RefreshBounds();

    Change();
}
//---------------------------------------------------------------------------
int TFarDialog::ShowModal()
{
    FResult = -1;

    int BResult;

    TFarDialog *PrevTopDialog = FarPlugin.get()->FTopDialog;
    FarPlugin.get()->FTopDialog = this;
    try
    {
        assert(DefaultButton);
        assert(DefaultButton->Default);

        wstring AHelpTopic = HelpTopic;

        {
            TFarEnvGuard Guard;
            HANDLE dlg = FarPlugin.get()->FStartupInfo.DialogInit(
                FarPlugin.get()->FStartupInfo.ModuleNumber,
                Bounds.get().Left, Bounds.get().Top, Bounds.get().Right, Bounds.get().Bottom,
                StrToFar(AHelpTopic), FDialogItems, ItemCount, 0, Flags,
                DialogProcGeneral, (long)this);
            BResult = FarPlugin.get()->FStartupInfo.DialogRun(dlg);
        }

        if (BResult >= 0)
        {
            TFarButton *Button = dynamic_cast<TFarButton *>(Item[BResult]);
            assert(Button);
            // correct result should be already set by TFarButton
            assert(FResult == Button->Result);
            FResult = Button->Result;
        }
        else
        {
            // allow only one negative value = -1
            FResult = -1;
        }
    }
    catch (...)
    {
    }
    FarPlugin.get()->FTopDialog = PrevTopDialog;

    return FResult;
}
//---------------------------------------------------------------------------
void TFarDialog::BreakSynchronize()
{
    SetEvent(FSynchronizeObjects[1]);
}
//---------------------------------------------------------------------------
void TFarDialog::Synchronize(TThreadMethod Method)
{
    if (FSynchronizeObjects[0] == INVALID_HANDLE_VALUE)
    {
        FSynchronizeObjects[0] = CreateSemaphore(NULL, 0, 2, NULL);
        FSynchronizeObjects[1] = CreateEvent(NULL, false, false, NULL);
    }
    FSynchronizeMethod = Method;
    FNeedsSynchronize = true;
    WaitForMultipleObjects(LENOF(FSynchronizeObjects),
                           reinterpret_cast<HANDLE *>(&FSynchronizeObjects), false, INFINITE);
}
//---------------------------------------------------------------------------
void TFarDialog::Close(TFarButton *Button)
{
    assert(Button != NULL);
    SendMessage(DM_CLOSE, Button->Item, 0);
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
        try
        {
            TFarDialogItem *DItem;
            for (int i = 0; i < Items->Count; i++)
            {
                DItem = Item[i];
                DItem->Change();
                if (DItem->Container && NotifiedContainers->IndexOf(DItem->Container) < 0)
                {
                    NotifiedContainers->Add(DItem->Container);
                }
            }

            for (int Index = 0; Index < NotifiedContainers->Count; Index++)
            {
                ((TFarDialogContainer *)NotifiedContainers->Items[Index])->Change();
            }
        }
        catch (...)
        {
        }
        delete NotifiedContainers;
    }
}
//---------------------------------------------------------------------------
long TFarDialog::SendMessage(int Msg, int Param1, int Param2)
{
    assert(Handle);
    TFarEnvGuard Guard;
    return FarPlugin->FStartupInfo.SendDlgMessage(Handle, Msg, Param1, Param2);
}
//---------------------------------------------------------------------------
char TFarDialog::GetSystemColor(unsigned int Index) const
{
    return static_cast<char>(FarPlugin->FarAdvControl(ACTL_GETCOLOR, Index));
}
//---------------------------------------------------------------------------
void TFarDialog::Redraw()
{
    SendMessage(DM_REDRAW, 0, 0);
}
//---------------------------------------------------------------------------
void TFarDialog::ShowGroup(int Group, bool Show)
{
    ProcessGroup(Group, ShowItem, &Show);
}
//---------------------------------------------------------------------------
void TFarDialog::EnableGroup(int Group, bool Enable)
{
    ProcessGroup(Group, EnableItem, &Enable);
}
//---------------------------------------------------------------------------
void TFarDialog::ProcessGroup(int Group, TFarProcessGroupEvent Callback,
        void *Arg)
{
    LockChanges();
    try
    {
        for (int i = 0; i < Items->Count; i++)
        {
            TFarDialogItem *I = Item[i];
            if (I->Group == Group)
            {
                Callback(I, Arg);
            }
        }
    }
    catch (...)
    {
    }
    UnlockChanges();
}
//---------------------------------------------------------------------------
void TFarDialog::ShowItem(TFarDialogItem *Item, void *Arg)
{
    Item->Visible = *static_cast<bool *>(Arg);
}
//---------------------------------------------------------------------------
void TFarDialog::EnableItem(TFarDialogItem *Item, void *Arg)
{
    Item->Enabled = *static_cast<bool *>(Arg);
}
//---------------------------------------------------------------------------
void TFarDialog::SetItemFocused(TFarDialogItem * const &value)
{
    if (value != ItemFocused)
    {
        assert(value);
        value->SetFocus();
    }
}
//---------------------------------------------------------------------------
string TFarDialog::GetMsg(int MsgId)
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
        if (Handle)
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
        try
        {
            if (FChangesPending)
            {
                FChangesPending = false;
                Change();
            }
        }
        catch (...)
        {
        }
        if (Handle)
        {
            SendMessage(DM_ENABLEREDRAW, true, 0);
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
    TObject()
{
    assert(ADialog);

    FItems = new TObjectList();
    FItems->OwnsObjects = false;
    FDialog = ADialog;
    FEnabled = true;

    Dialog->Add(this);
    Dialog->GetNextItemPosition(FLeft, FTop);
}
//---------------------------------------------------------------------------
TFarDialogContainer::~TFarDialogContainer()
{
    delete FItems;
}
//---------------------------------------------------------------------------
string TFarDialogContainer::GetMsg(int MsgId)
{
    return Dialog->GetMsg(MsgId);
}
//---------------------------------------------------------------------------
void TFarDialogContainer::Add(TFarDialogItem *Item)
{
    assert(FItems->IndexOf(Item) < 0);
    Item->Container = this;
    FItems->Add(Item);
}
//---------------------------------------------------------------------------
void TFarDialogContainer::Remove(TFarDialogItem *Item)
{
    assert(FItems->IndexOf(Item) >= 0);
    Item->Container = NULL;
    FItems->Remove(Item);
    if (FItems->Count == 0)
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
        for (int Index = 0; Index < FItems->Count; Index++)
        {
            dynamic_cast<TFarDialogItem *>(FItems->Items[Index])->DialogResized();
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
    if (Enabled != value)
    {
        FEnabled = true;
        for (int Index = 0; Index < FItems->Count; Index++)
        {
            dynamic_cast<TFarDialogItem *>(FItems->Items[Index])->UpdateEnabled();
        }
    }
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TFarDialogItem::TFarDialogItem(TFarDialog *ADialog, int AType) :
    TObject()
{
    assert(ADialog);
    FDialog = ADialog;
    FDefaultType = AType;
    FTag = 0;
    FOem = false;
    FGroup = 0;
    FEnabled = true;
    FIsEnabled = true;
    FOnExit = NULL;
    FOnMouseClick = NULL;
    FColors = 0;
    FColorMask = 0;

    Dialog->Add(this);

    DialogItem->Type = AType;
}
//---------------------------------------------------------------------------
TFarDialogItem::~TFarDialogItem()
{
    assert(!Dialog);
}
//---------------------------------------------------------------------------
FarDialogItem *TFarDialogItem::GetDialogItem()
{
    assert(Dialog);
    return &Dialog->FDialogItems[Item];
}
//---------------------------------------------------------------------------
void TFarDialogItem::SetBounds(TRect value)
{
    if (Bounds != value)
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
    TRect B = Bounds;
    FarDialogItem *DItem = DialogItem;
#define BOUND(DIB, BB, DB, CB) DItem->DIB = B.BB >= 0 ? \
    (Container ? Container->CB : 0 ) + B.BB : Dialog->Size.DB + B.BB
    BOUND(X1, Left, x, Left);
    BOUND(Y1, Top, y, Top);
    BOUND(X2, Right, x, Left);
    BOUND(Y2, Bottom, y, Top);
#undef BOUND
}
//---------------------------------------------------------------------------
void TFarDialogItem::UpdateBounds()
{
    ResetBounds();

    if (Dialog->Handle)
    {
        TRect B = ActualBounds;
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
    if (Flags != value)
    {
        assert(!Dialog->Handle);
        UpdateFlags(value);
    }
}
//---------------------------------------------------------------------------
void TFarDialogItem::UpdateFlags(unsigned int value)
{
    if (Flags != value)
    {
        DialogItem->Flags = value;
        DialogChange();
    }
}
//---------------------------------------------------------------------------
TRect TFarDialogItem::GetActualBounds()
{
    return TRect(DialogItem->X1, DialogItem->Y1,
                 DialogItem->X2, DialogItem->Y2);
}
//---------------------------------------------------------------------------
unsigned int TFarDialogItem::GetFlags()
{
    return DialogItem->Flags;
}
//---------------------------------------------------------------------------
void TFarDialogItem::SetDataInternal(const string value)
{
    string FarData = value.SubString(1, sizeof(DialogItem->Data) - 1);
    if (!Oem)
    {
        StrToFar(FarData);
    }
    if (Dialog->Handle)
    {
        SendMessage(DM_SETTEXTPTR, (int)FarData.c_str());
    }
    strcpy(DialogItem->Data, FarData.c_str());
    DialogChange();
}
//---------------------------------------------------------------------------
void TFarDialogItem::SetData(const string value)
{
    if (Data != value)
    {
        SetDataInternal(value);
    }
}
//---------------------------------------------------------------------------
void TFarDialogItem::UpdateData(const string value)
{
    string FarData = value.SubString(1, sizeof(DialogItem->Data) - 1);
    if (!Oem)
    {
        StrToFar(FarData);
    }
    strcpy(DialogItem->Data, FarData.c_str());
}
//---------------------------------------------------------------------------
string TFarDialogItem::GetData()
{
    string Result = DialogItem->Data;
    if (!Oem)
    {
        StrFromFar(Result);
    }
    return Result;
}
//---------------------------------------------------------------------------
void TFarDialogItem::SetType(int value)
{
    if (Type != value)
    {
        assert(!Dialog->Handle);
        DialogItem->Type = value;
    }
}
//---------------------------------------------------------------------------
int TFarDialogItem::GetType()
{
    return DialogItem->Type;
}
//---------------------------------------------------------------------------
void TFarDialogItem::SetAlterType(int Index, bool value)
{
    if (GetAlterType(Index) != value)
    {
        Type = value ? Index : FDefaultType;
    }
}
//---------------------------------------------------------------------------
bool TFarDialogItem::GetAlterType(int Index)
{
    return (Type == Index);
}
//---------------------------------------------------------------------------
bool TFarDialogItem::GetFlag(int Index)
{
    bool Result = (Flags & (Index & 0xFFFFFF00UL)) != 0;
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

        unsigned long F = Flags;
        unsigned long Flag = Index & 0xFFFFFF00UL;
        bool ToHandle = true;

        switch (Flag)
        {
        case DIF_DISABLE:
            if (Dialog->Handle)
            {
                SendMessage(DM_ENABLE, !value);
            }
            break;

        case DIF_HIDDEN:
            if (Dialog->Handle)
            {
                SendMessage(DM_SHOWITEM, !value);
            }
            break;

        case DIF_3STATE:
            if (Dialog->Handle)
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
    if (EnabledFollow != value)
    {
        FEnabledFollow = value;
        Change();
    }
}
//---------------------------------------------------------------------------
void TFarDialogItem::SetEnabledDependency(TFarDialogItem *value)
{
    if (EnabledDependency != value)
    {
        FEnabledDependency = value;
        Change();
    }
}
//---------------------------------------------------------------------------
void TFarDialogItem::SetEnabledDependencyNegative(TFarDialogItem *value)
{
    if (EnabledDependencyNegative != value)
    {
        FEnabledDependencyNegative = value;
        Change();
    }
}
//---------------------------------------------------------------------------
bool TFarDialogItem::GetIsEmpty()
{
    return Data.Trim().IsEmpty();
}
//---------------------------------------------------------------------------
long TFarDialogItem::FailItemProc(int Msg, long Param)
{
    long Result;
    switch (Msg)
    {
    case DN_KILLFOCUS:
        Result = Item;
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
    if (OnExit)
    {
        OnExit(this);
    }
}
//---------------------------------------------------------------------------
long TFarDialogItem::DefaultItemProc(int Msg, int Param)
{
    TFarEnvGuard Guard;
    return Dialog->FarPlugin->FStartupInfo.DefDlgProc(Dialog->Handle, Msg, Item, Param);
}
//---------------------------------------------------------------------------
long TFarDialogItem::DefaultDialogProc(int Msg, int Param1, int Param2)
{
    TFarEnvGuard Guard;
    return Dialog->FarPlugin->FStartupInfo.DefDlgProc(Dialog->Handle, Msg, Param1, Param2);
}
//---------------------------------------------------------------------------
void TFarDialogItem::Change()
{
    if (EnabledFollow || EnabledDependency || EnabledDependencyNegative)
    {
        UpdateEnabled();
    }
}
//---------------------------------------------------------------------------
void TFarDialogItem::SetEnabled(bool value)
{
    if (Enabled != value)
    {
        FEnabled = value;
        UpdateEnabled();
    }
}
//---------------------------------------------------------------------------
void TFarDialogItem::UpdateEnabled()
{
    bool Value =
        Enabled &&
        (!EnabledFollow || EnabledFollow->IsEnabled) &&
        (!EnabledDependency ||
         (!EnabledDependency->IsEmpty && EnabledDependency->IsEnabled)) &&
        (!EnabledDependencyNegative ||
         (EnabledDependencyNegative->IsEmpty || !EnabledDependencyNegative->IsEnabled)) &&
        (!Container || Container->Enabled);

    if (Value != IsEnabled)
    {
        FIsEnabled = Value;
        SetFlag(DIF_DISABLE | DIF_INVERSE, IsEnabled);
    }
}
//---------------------------------------------------------------------------
void TFarDialogItem::DialogChange()
{
    assert(Dialog);
    Dialog->Change();
}
//---------------------------------------------------------------------------
long TFarDialogItem::SendDialogMessage(int Msg, int Param1, int Param2)
{
    return Dialog->SendMessage(Msg, Param1, Param2);
}
//---------------------------------------------------------------------------
long TFarDialogItem::SendMessage(int Msg, int Param)
{
    return Dialog->SendMessage(Msg, Item, Param);
}
//---------------------------------------------------------------------------
void TFarDialogItem::SetSelected(int value)
{
    if (Selected != value)
    {
        if (Dialog->Handle)
        {
            SendMessage(DM_SETCHECK, value);
        }
        UpdateSelected(value);
    }
}
//---------------------------------------------------------------------------
void TFarDialogItem::UpdateSelected(int value)
{
    if (Selected != value)
    {
        DialogItem->Selected = value;
        DialogChange();
    }
}
//---------------------------------------------------------------------------
int TFarDialogItem::GetSelected()
{
    return DialogItem->Selected;
}
//---------------------------------------------------------------------------
void TFarDialogItem::SetChecked(bool value)
{
    Selected = value ? BSTATE_CHECKED : BSTATE_UNCHECKED;
}
//---------------------------------------------------------------------------
bool _fastcall TFarDialogItem::GetChecked()
{
    return Selected == BSTATE_CHECKED;
}
//---------------------------------------------------------------------------
void TFarDialogItem::Move(int DeltaX, int DeltaY)
{
    TRect R = Bounds;

    R.Left += DeltaX;
    R.Right += DeltaX;
    R.Top += DeltaY;
    R.Bottom += DeltaY;

    Bounds = R;
}
//---------------------------------------------------------------------------
void TFarDialogItem::MoveAt(int X, int Y)
{
    Move(X - Left, Y - Top);
}
//---------------------------------------------------------------------------
void TFarDialogItem::SetCoordinate(int Index, int value)
{
    assert(sizeof(TRect) == sizeof(long) * 4);
    TRect R = Bounds;
    long *D = (long *)&R;
    D += Index;
    *D = value;
    Bounds = R;
}
//---------------------------------------------------------------------------
int TFarDialogItem::GetCoordinate(int Index)
{
    assert(sizeof(TRect) == sizeof(long) * 4);
    TRect R = Bounds;
    long *D = (long *)&R;
    D += Index;
    return *D;
}
//---------------------------------------------------------------------------
void TFarDialogItem::SetWidth(int value)
{
    TRect R = Bounds;
    if (R.Left >= 0)
    {
        R.Right = R.Left + value - 1;
    }
    else
    {
        assert(R.Right < 0);
        R.Left = R.Right - value + 1;
    }
    Bounds = R;
}
//---------------------------------------------------------------------------
int TFarDialogItem::GetWidth()
{
    return ActualBounds.Width() + 1;
}
//---------------------------------------------------------------------------
void TFarDialogItem::SetHeight(int value)
{
    TRect R = Bounds;
    if (R.Top >= 0)
    {
        R.Bottom = R.Top + value - 1;
    }
    else
    {
        assert(R.Bottom < 0);
        R.Top = R.Bottom - value + 1;
    }
    Bounds = R;
}
//---------------------------------------------------------------------------
int TFarDialogItem::GetHeight()
{
    return ActualBounds.Height() + 1;
}
//---------------------------------------------------------------------------
bool TFarDialogItem::CanFocus()
{
    return Visible && Enabled && TabStop &&
           (Type == DI_EDIT || Type == DI_PSWEDIT || Type == DI_FIXEDIT ||
            Type == DI_BUTTON || Type == DI_CHECKBOX || Type == DI_RADIOBUTTON ||
            Type == DI_COMBOBOX || Type == DI_LISTBOX || Type == DI_USERCONTROL);
}
//---------------------------------------------------------------------------
bool TFarDialogItem::Focused()
{
    return DialogItem->Focus != 0;
}
//---------------------------------------------------------------------------
void TFarDialogItem::UpdateFocused(bool value)
{
    DialogItem->Focus = value;
    assert(Dialog);
    Dialog->FItemFocused = value ? this : NULL;
}
//---------------------------------------------------------------------------
void TFarDialogItem::SetFocus()
{
    assert(CanFocus());
    if (!Focused())
    {
        if (Dialog->Handle)
        {
            SendMessage(DM_SETFOCUS, 0);
        }
        else
        {
            if (Dialog->ItemFocused)
            {
                assert(Dialog->ItemFocused != this);
                Dialog->ItemFocused->UpdateFocused(false);
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

        TRect B = Bounds;
        B.Left = Rect.Left;
        B.Right = Rect.Right;
        Bounds = B;
    }
}
//---------------------------------------------------------------------------
bool TFarDialogItem::CloseQuery()
{
    if (Focused() && (Dialog->Result >= 0))
    {
        DoExit();
    }
    return true;
}
//---------------------------------------------------------------------------
TPoint TFarDialogItem::MouseClientPosition(MOUSE_EVENT_RECORD *Event)
{
    TPoint Result;
    if (Type == DI_USERCONTROL)
    {
        Result = TPoint(Event->dwMousePosition.X, Event->dwMousePosition.Y);
    }
    else
    {
        Result = TPoint(
                     Event->dwMousePosition.X - Dialog->Bounds.Left - Left,
                     Event->dwMousePosition.Y - Dialog->Bounds.Top - Top);
    }
    return Result;
}
//---------------------------------------------------------------------------
bool TFarDialogItem::MouseClick(MOUSE_EVENT_RECORD *Event)
{
    if (OnMouseClick)
    {
        OnMouseClick(this, Event);
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
void TFarDialogItem::Text(int X, int Y, int Color, string Str, bool AOem)
{
    if (!AOem && !Oem)
    {
        StrToFar(Str);
    }
    TFarEnvGuard Guard;
    Dialog->FarPlugin->FStartupInfo.Text(
        Dialog->Bounds.Left + Left + X, Dialog->Bounds.Top + Top + Y,
        Color, Str.c_str());
}
//---------------------------------------------------------------------------
void TFarDialogItem::Redraw()
{
    // do not know how to force redraw of the item only
    Dialog->Redraw();
}
//---------------------------------------------------------------------------
void TFarDialogItem::SetContainer(TFarDialogContainer *value)
{
    if (Container != value)
    {
        TFarDialogContainer *PrevContainer = Container;
        FContainer = value;
        if (PrevContainer)
        {
            PrevContainer->Remove(this);
        }
        if (Container)
        {
            Container->Add(this);
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
void TFarButton::SetDataInternal(const string value)
{
    string AValue;
    switch (FBrackets)
    {
    case brTight:
        AValue = "[" + value + "]";
        break;

    case brSpace:
        AValue = " " + value + " ";
        break;

    default:
        AValue = value;
        break;
    }

    TFarDialogItem::SetDataInternal(AValue);

    if ((Left >= 0) || (Right >= 0))
    {
        int Margin;
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
        Width = Margin + StripHotKey(AValue).Length() + Margin;
    }
}
//---------------------------------------------------------------------------
string TFarButton::GetData()
{
    string Result = TFarDialogItem::GetData();
    if ((FBrackets == brTight) || (FBrackets == brSpace))
    {
        bool HasBrackets = (Result.Length() >= 2) &&
                           (Result[1] == ((FBrackets == brSpace) ? ' ' : '[')) &&
                           (Result[Result.Length()] == ((FBrackets == brSpace) ? ' ' : ']'));
        assert(HasBrackets);
        if (HasBrackets)
        {
            Result = Result.SubString(2, Result.Length() - 2);
        }
    }
    return Result;
}
//---------------------------------------------------------------------------
void TFarButton::SetDefault(bool value)
{
    if (Default != value)
    {
        assert(!Dialog->Handle);
        DialogItem->DefaultButton = value;
        if (value)
        {
            if (Dialog->DefaultButton && (Dialog->DefaultButton != this))
            {
                Dialog->DefaultButton->Default = false;
            }
            Dialog->FDefaultButton = this;
        }
        else if (Dialog->DefaultButton == this)
        {
            Dialog->FDefaultButton = NULL;
        }
        DialogChange();
    }
}
//---------------------------------------------------------------------------
bool TFarButton::GetDefault()
{
    return DialogItem->DefaultButton;
}
//---------------------------------------------------------------------------
void TFarButton::SetBrackets(TFarButtonBrackets value)
{
    if (FBrackets != value)
    {
        string AData = Data;
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
        if (!Enabled)
        {
            return true;
        }
        else
        {
            bool Close = (Result != 0);
            if (OnClick)
            {
                OnClick(this, Close);
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
    int P = Caption.Pos("&");
    bool Result =
        Visible && Enabled &&
        (P > 0) && (P < Caption.Length()) &&
        SameText(Caption[P + 1], HotKey);
    if (Result)
    {
        bool Close = (Result != 0);
        if (OnClick)
        {
            OnClick(this, Close);
        }

        if (Close)
        {
            Dialog->Close(this);
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
        if (OnAllowChange)
        {
            OnAllowChange(this, Param, Allow);
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
    return Checked != BSTATE_CHECKED;
}
//---------------------------------------------------------------------------
void TFarCheckBox::SetData(const string value)
{
    TFarDialogItem::SetData(value);
    if (Left >= 0 || Right >= 0)
    {
        Width = 4 + StripHotKey(value).Length();
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
        if (OnAllowChange)
        {
            OnAllowChange(this, Param, Allow);
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
    return !Checked;
}
//---------------------------------------------------------------------------
void TFarRadioButton::SetData(const string value)
{
    TFarDialogItem::SetData(value);
    if (Left >= 0 || Right >= 0)
    {
        Width = 4 + StripHotKey(value).Length();
    }
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TFarEdit::TFarEdit(TFarDialog *ADialog) :
    TFarDialogItem(ADialog, DI_EDIT)
{
    AutoSelect = false;
}
//---------------------------------------------------------------------------
void TFarEdit::Detach()
{
    delete[] DialogItem->Mask;
    TFarDialogItem::Detach();
}
//---------------------------------------------------------------------------
long TFarEdit::ItemProc(int Msg, long Param)
{
    if (Msg == DN_EDITCHANGE)
    {
        strcpy(DialogItem->Data, ((FarDialogItem *)Param)->Data);
    }
    return TFarDialogItem::ItemProc(Msg, Param);
}
//---------------------------------------------------------------------------
string TFarEdit::GetHistoryMask(int Index)
{
    string Result =
        ((Index == 0) && (Flags & DIF_HISTORY)) ||
        ((Index == 1) && (Flags & DIF_MASKEDIT)) ? DialogItem->Mask : "";
    if (!Oem)
    {
        StrFromFar(Result);
    }
    return Result;
}
//---------------------------------------------------------------------------
void TFarEdit::SetHistoryMask(int Index, string value)
{
    if (GetHistoryMask(Index) != value)
    {
        assert(!Dialog->Handle);
        assert(&DialogItem->Mask == &DialogItem->History);

        delete[] DialogItem->Mask;
        if (value.IsEmpty())
        {
            DialogItem->Mask = NULL;
        }
        else
        {
            DialogItem->Mask = new char[value.Length()+1];
            strcpy((char *)DialogItem->Mask, value.c_str());
            if (!Oem)
            {
                StrToFar((char *)DialogItem->Mask);
            }
        }
        bool PrevHistory = !History.IsEmpty();
        SetFlag(DIF_HISTORY, (Index == 0) && !value.IsEmpty());
        bool Masked = (Index == 1) && !value.IsEmpty();
        SetFlag(DIF_MASKEDIT, Masked);
        if (Masked)
        {
            Fixed = true;
        }
        bool CurrHistory = !History.IsEmpty();
        if (PrevHistory != CurrHistory)
        {
            // add/remove space for history arrow
            Width = Width + (CurrHistory ? -1 : 1);
        }
        DialogChange();
    }
}
//---------------------------------------------------------------------------
void TFarEdit::SetAsInteger(int value)
{
    int Int;
    if (!TryStrToInt(Text.Trim(), Int) || (AsInteger != value))
    {
        Text = IntToStr(value);
        DialogChange();
    }
}
//---------------------------------------------------------------------------
int TFarEdit::GetAsInteger()
{
    return StrToIntDef(Text.Trim(), 0);
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TFarSeparator::TFarSeparator(TFarDialog *ADialog) :
    TFarDialogItem(ADialog, DI_TEXT)
{
    Left = -1;
    SetFlag(DIF_SEPARATOR, true);
}
//---------------------------------------------------------------------------
void TFarSeparator::ResetBounds()
{
    TFarDialogItem::ResetBounds();
    if (Bounds.Left < 0)
    {
        DialogItem->X1 = -1;
    }
}
//---------------------------------------------------------------------------
void TFarSeparator::SetDouble(bool value)
{
    if (Double != value)
    {
        assert(!Dialog->Handle);
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
    TRect R = Bounds;
    R.Top = value;
    R.Bottom = value;
    Bounds = R;
}
//---------------------------------------------------------------------------
int TFarSeparator::GetPosition()
{
    return Bounds.Top;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TFarText::TFarText(TFarDialog *ADialog) :
    TFarDialogItem(ADialog, DI_TEXT)
{
}
//---------------------------------------------------------------------------
void TFarText::SetData(const string value)
{
    TFarDialogItem::SetData(value);
    if (Left >= 0 || Right >= 0)
    {
        Width = StripHotKey(value).Length();
    }
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TFarList::TFarList(TFarDialogItem *ADialogItem) :
    TStringList()
{
    assert((ADialogItem == NULL) ||
           (ADialogItem->Type == DI_COMBOBOX) || (ADialogItem->Type == DI_LISTBOX));

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
        for (int Index = 0; Index < FarList->Count; Index++)
        {
            Flags[Index] = FarList->Flags[Index];
        }
    }
}
//---------------------------------------------------------------------------
void TFarList::UpdateItem(int Index)
{
    FarListItem *ListItem = &FListItems->Items[Index];
    strcpy(ListItem->Text,
           Strings[Index].SubString(1, sizeof(ListItem->Text) - 1).c_str());
    if (!DialogItem->Oem)
    {
        StrToFar(ListItem->Text);
    }

    FarListUpdate ListUpdate;
    memset(&ListUpdate, 0, sizeof(ListUpdate));
    ListUpdate.Index = Index;
    ListUpdate.Item = *ListItem;
    DialogItem->SendMessage(DM_LISTUPDATE, (int)&ListUpdate);
}
//---------------------------------------------------------------------------
void TFarList::Put(int Index, const string S)
{
    if ((DialogItem != NULL) && DialogItem->Dialog->Handle)
    {
        FNoDialogUpdate = true;
        try
        {
            TStringList::Put(Index, S);
            if (UpdateCount == 0)
            {
                UpdateItem(Index);
            }
        }
        catch (...)
        {
        }
        FNoDialogUpdate = false;
    }
    else
    {
        TStringList::Put(Index, S);
    }
}
//---------------------------------------------------------------------------
void TFarList::Changed()
{
    TStringList::Changed();

    if ((UpdateCount == 0) && !FNoDialogUpdate)
    {
        int PrevSelected;
        int PrevTopIndex;
        if ((DialogItem != NULL) && DialogItem->Dialog->Handle)
        {
            PrevSelected = Selected;
            PrevTopIndex = TopIndex;
        }
        if (FListItems->ItemsNumber != Count)
        {
            FarListItem *Items = FListItems->Items;
            if (Count)
            {
                FListItems->Items = new FarListItem[Count];
                for (int Index = 0; Index < Count; Index++)
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
            FListItems->ItemsNumber = Count;
        }
        for (int i = 0; i < Count; i++)
        {
            strcpy(FListItems->Items[i].Text,
                   Strings[i].SubString(1, sizeof(FListItems->Items[i].Text) - 1).c_str());
            if ((DialogItem != NULL) && !DialogItem->Oem)
            {
                StrToFar(FListItems->Items[i].Text);
            }
        }
        if ((DialogItem != NULL) && DialogItem->Dialog->Handle)
        {
            DialogItem->Dialog->LockChanges();
            try
            {
                DialogItem->SendMessage(DM_LISTSET, (int)FListItems);
                if (PrevTopIndex + DialogItem->Height > Count)
                {
                    PrevTopIndex = Count > DialogItem->Height ? Count - DialogItem->Height : 0;
                }
                SetCurPos((PrevSelected >= Count) ? (Count - 1) : PrevSelected,
                          PrevTopIndex);
            }
            catch (...)
            {
            }
            DialogItem->Dialog->UnlockChanges();
        }
    }
}
//---------------------------------------------------------------------------
void TFarList::SetSelected(int value)
{
    assert(DialogItem != NULL);
    if (GetSelectedInt(false) != value)
    {
        if (DialogItem->Dialog->Handle)
        {
            UpdatePosition(value);
        }
        else
        {
            DialogItem->Data = Strings[value];
        }
    }
}
//---------------------------------------------------------------------------
void TFarList::UpdatePosition(int Position)
{
    if (Position >= 0)
    {
        int ATopIndex = TopIndex;
        // even if new position is visible already, FAR will scroll the view so
        // that the new selected item is the last one, following prevents the scroll
        if ((ATopIndex <= Position) && (Position < ATopIndex + VisibleCount))
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
    assert(DialogItem != NULL);
    assert(DialogItem->Dialog->Handle);
    FarListPos ListPos;
    ListPos.SelectPos = Position;
    ListPos.TopPos = TopIndex;
    DialogItem->SendMessage(DM_LISTSETCURPOS, (int)&ListPos);
}
//---------------------------------------------------------------------------
void TFarList::SetTopIndex(int value)
{
    if (value != TopIndex)
    {
        SetCurPos(-1, value);
    }
}
//---------------------------------------------------------------------------
int TFarList::GetPosition()
{
    assert(DialogItem != NULL);
    return DialogItem->SendMessage(DM_LISTGETCURPOS, NULL);
}
//---------------------------------------------------------------------------
int TFarList::GetTopIndex()
{
    int Result;
    if (Count == 0)
    {
        Result = -1;
    }
    else
    {
        FarListPos ListPos;
        assert(DialogItem != NULL);
        DialogItem->SendMessage(DM_LISTGETCURPOS, reinterpret_cast<int>(&ListPos));
        Result = ListPos.TopPos;
    }
    return Result;
}
//---------------------------------------------------------------------------
int TFarList::GetMaxLength()
{
    int Result = 0;
    for (int i = 0; i < Count; i++)
    {
        if (Result < Strings[i].Length())
        {
            Result = Strings[i].Length();
        }
    }
    return Result;
}
//---------------------------------------------------------------------------
int TFarList::GetVisibleCount()
{
    assert(DialogItem != NULL);
    return FDialogItem->Height - (FDialogItem->GetFlag(DIF_LISTNOBOX) ? 0 : 2);
}
//---------------------------------------------------------------------------
int TFarList::GetSelectedInt(bool Init)
{
    int Result;
    assert(DialogItem != NULL);
    if (Count == 0)
    {
        Result = -1;
    }
    else if (DialogItem->Dialog->Handle && !Init)
    {
        Result = GetPosition();
    }
    else
    {
        Result = IndexOf(DialogItem->DialogItem->Data);
    }

    return Result;
}
//---------------------------------------------------------------------------
int TFarList::GetSelected()
{
    int Result = GetSelectedInt(false);

    if ((Result < 0) && (Count > 0))
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
        if ((DialogItem != NULL) && DialogItem->Dialog->Handle && (UpdateCount == 0))
        {
            UpdateItem(Index);
        }
    }
}
//---------------------------------------------------------------------------
bool TFarList::GetFlag(int Index, int Flag)
{
    return FLAGSET(Flags[Index], static_cast<unsigned int>(Flag));
}
//---------------------------------------------------------------------------
void TFarList::SetFlag(int Index, int Flag, bool value)
{
    Flags[Index] = (Flags[Index] & ~Flag) | FLAGMASK(value, Flag);
}
//---------------------------------------------------------------------------
void TFarList::Init()
{
    UpdatePosition(GetSelectedInt(true));
}
//---------------------------------------------------------------------------
long TFarList::ItemProc(int Msg, long Param)
{
    assert(DialogItem != NULL);
    if (Msg == DN_LISTCHANGE)
    {
        if ((Param < 0) || ((Param == 0) && (Count == 0)))
        {
            DialogItem->UpdateData("");
        }
        else
        {
            assert(Param >= 0 && Param < Count);
            DialogItem->UpdateData(Strings[Param]);
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
    DialogItem->ListItems = FList->ListItems;
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
    if ((Msg == DN_KEY) && (FarPlugin->FarVersion() >= FAR170) && Dialog->HotKey(Param))
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
    if ((Msg == DN_MOUSECLICK) && (Items->Count > 0) &&
            (FarPlugin->FarVersion() < FAR170ALPHA6))
    {
        FDenyClose = true;
    }

    return Result;
}
//---------------------------------------------------------------------------
void TFarListBox::Init()
{
    TFarDialogItem::Init();
    Items->Init();
    UpdateMouseReaction();
}
//---------------------------------------------------------------------------
void TFarListBox::SetAutoSelect(TFarListBoxAutoSelect value)
{
    if (AutoSelect != value)
    {
        FAutoSelect = value;
        if (Dialog->Handle)
        {
            UpdateMouseReaction();
        }
    }
}
//---------------------------------------------------------------------------
void TFarListBox::UpdateMouseReaction()
{
    SendMessage(DM_LISTSETMOUSEREACTION, AutoSelect);
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
        assert(FarPlugin->FarVersion() < FAR170ALPHA6);
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
    DialogItem->ListItems = FList->ListItems;
    AutoSelect = false;
}
//---------------------------------------------------------------------------
TFarComboBox::~TFarComboBox()
{
    SAFE_DESTROY(FList);
}
//---------------------------------------------------------------------------
void TFarComboBox::ResizeToFitContent()
{
    Width = FList->MaxLength;
}
//---------------------------------------------------------------------------
long TFarComboBox::ItemProc(int Msg, long Param)
{
    if (Msg == DN_EDITCHANGE)
    {
        strcpy(DialogItem->Data, ((FarDialogItem *)Param)->Data);
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
    Items->Init();
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TFarLister::TFarLister(TFarDialog *ADialog) :
    TFarDialogItem(ADialog, DI_USERCONTROL),
    FItems(new TStringList()),
    FTopIndex(0)
{
    FItems->OnChange = ItemsChange;
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
    if (Dialog->Handle)
    {
        Redraw();
    }
}
//---------------------------------------------------------------------------
bool TFarLister::GetScrollBar()
{
    return (Items->Count > Height);
}
//---------------------------------------------------------------------------
void TFarLister::SetTopIndex(int value)
{
    if (TopIndex != value)
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
    long Result;

    if (Msg == DN_DRAWDLGITEM)
    {
        bool AScrollBar = ScrollBar;
        int ScrollBarPos = 0;
        if (Items->Count - Height > 0)
        {
            ScrollBarPos = float(Height - 3) * (float(FTopIndex) / (Items->Count - Height)) + 1;
        }
        int DisplayWidth = Width - (AScrollBar ? 1 : 0);
        int Color = Dialog->GetSystemColor(
                        FLAGSET(Dialog->Flags, FDLG_WARNING) ? COL_WARNDIALOGLISTTEXT : COL_DIALOGLISTTEXT);
        string Buf;
        for (int Row = 0; Row < Height; Row++)
        {
            int Index = TopIndex + Row;
            Buf = " ";
            if (Index < Items->Count)
            {
                Buf += Items->Strings[Index].SubString(1, DisplayWidth - 1);
            }
            Buf += string::StringOfChar(' ', DisplayWidth - Buf.Length());
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
                else if (Row == Height - 1)
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

        int NewTopIndex = TopIndex;
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
            if (NewTopIndex < Items->Count - Height)
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
            if (NewTopIndex > Height - 1)
            {
                NewTopIndex -= Height - 1;
            }
            else
            {
                NewTopIndex = 0;
            }
        }
        else if (Param == KEY_PGDN)
        {
            if (NewTopIndex < Items->Count - Height - Height + 1)
            {
                NewTopIndex += Height - 1;
            }
            else
            {
                NewTopIndex = Items->Count - Height;
            }
        }
        else if (Param == KEY_HOME)
        {
            NewTopIndex = 0;
        }
        else if (Param == KEY_END)
        {
            NewTopIndex = Items->Count - Height;
        }
        else
        {
            Result = TFarDialogItem::ItemProc(Msg, Param);
        }

        TopIndex = NewTopIndex;
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
                (P.x < Width - 1))
        {
            Result = TFarDialogItem::ItemProc(Msg, Param);
        }
        else
        {
            int NewTopIndex = TopIndex;

            if (((P.x == Width - 1) && (P.y == 0)) ||
                    ((P.x < Width - 1) && (P.y < Height / 2)))
            {
                if (NewTopIndex > 0)
                {
                    NewTopIndex--;
                }
            }
            else if (((P.x == Width - 1) && (P.y == Height - 1)) ||
                     ((P.x < Width - 1) && (P.y >= Height / 2)))
            {
                if (NewTopIndex < Items->Count - Height)
                {
                    NewTopIndex++;
                }
            }
            else
            {
                assert(P.x == Width - 1);
                assert((P.y > 0) && (P.y < Height - 1));
                NewTopIndex = ceil(float(P.y - 1) / (Height - 2) * (Items->Count - Height + 1));
            }

            Result = true;

            TopIndex = NewTopIndex;
        }
    }
    else
    {
        Result = TFarDialogItem::ItemProc(Msg, Param);
    }

    return Result;
}
