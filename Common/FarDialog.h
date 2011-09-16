/**************************************************************************
 *  Far plugin helper (for FAR 2.0) (http://code.google.com/p/farplugs)   *
 *  Copyright (C) 2000-2011 by Artem Senichev <artemsen@gmail.com>        *
 *                                                                        *
 *  This program is free software: you can redistribute it and/or modify  *
 *  it under the terms of the GNU General Public License as published by  *
 *  the Free Software Foundation, either version 3 of the License, or     *
 *  (at your option) any later version.                                   *
 *                                                                        *
 *  This program is distributed in the hope that it will be useful,       *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *  GNU General Public License for more details.                          *
 *                                                                        *
 *  You should have received a copy of the GNU General Public License     *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 **************************************************************************/

#pragma once

#include "FarPlugin.h"
#include "stdafx.h"

#define MAX_SIZE -1


/**
 * Far dialog wrapper
 */
class CFarDialog
{
public:
    /**
     * Constructor
     * \param width dialog width
     * \param height dialog height
     */
    explicit CFarDialog(const int width, const int height) :
        m_Dlg(INVALID_HANDLE_VALUE), m_Width(width), m_Height(height), m_UseFrame(false)
    {
    }

    /**
     * Constructor
     * \param width dialog width
     * \param height dialog height
     * \param title dialog title
     */
    explicit CFarDialog(const int width, const int height, const wchar_t *title) :
        m_Dlg(INVALID_HANDLE_VALUE), m_Width(width), m_Height(height), m_UseFrame(true)
    {
        SetTitle(title);
    }

    /**
     * Destructor
     */
    virtual ~CFarDialog()
    {
        if (m_Dlg != INVALID_HANDLE_VALUE)
        {
            CFarPlugin::GetPSI()->DialogFree(m_Dlg);
        }
    }

    /**
     * Get maximum dialogs width (FAR main window width)
     * \return maximum width
     */
    static int GetMaxWidth()
    {
        int wndWidth;

        SMALL_RECT rcFarWnd;
        if (CFarPlugin::AdvControl(ACTL_GETFARRECT, &rcFarWnd))
        {
            wndWidth = rcFarWnd.Right;
        }
        else
        {
            HANDLE stdOut = GetStdHandle(STD_OUTPUT_HANDLE);
            CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
            wndWidth = (GetConsoleScreenBufferInfo(stdOut, &consoleInfo) ? consoleInfo.srWindow.Right : 79);
        }
        return wndWidth + 1;
    }

    /**
     * Get maximum dialogs height (FAR main window height)
     * \return maximum height
     */
    static int GetMaxHeight()
    {
        int wndHeight;

        SMALL_RECT rcFarWnd;
        if (CFarPlugin::AdvControl(ACTL_GETFARRECT, &rcFarWnd))
        {
            wndHeight = rcFarWnd.Bottom - rcFarWnd.Top;
        }
        else
        {
            HANDLE stdOut = GetStdHandle(STD_OUTPUT_HANDLE);
            CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
            wndHeight = (GetConsoleScreenBufferInfo(stdOut, &consoleInfo) ? consoleInfo.srWindow.Bottom : 24);
        }
        return wndHeight + 1;
    }

    /**
     * Set dialog title and change window model
     * \param title dialog's title
     */
    inline void SetTitle(const wchar_t *title)
    {
        assert(title);
        assert(m_DlgItems.empty());  //Must be first call!!!

        m_UseFrame = true;
        CreateDlgItem(DI_DOUBLEBOX, 3, m_Width - 4, 1, m_Height - 2, title);
    }

    /**
     * Get top dialog coordinate
     * \return top dialog coordinate
     */
    inline int GetTop() const
    {
        return m_UseFrame ? 2 : 0;
    }

    /**
     * Get left dialog coordinate
     * \return left dialog coordinate
     */
    inline int GetLeft() const
    {
        return m_UseFrame ? 5 : 0;
    }

    /**
     * Get dialogs width
     * \return width
     */
    inline int GetWidth() const
    {
        return m_Width - (m_UseFrame ? 6 : 0);
    }

    /**
     * Get dialogs height
     * \return height
     */
    inline int GetHeight() const
    {
        return m_Height - (m_UseFrame ? 2 : 0);
    }

    /**
     * Resize dialog
     * \param width dialog width
     * \param height dialog height
     */
    inline void ResizeDialog(const int width, const int height)
    {
        assert(m_Dlg != INVALID_HANDLE_VALUE);
        m_Width = width;
        m_Height = height;
        COORD newSize;
        newSize.X = static_cast<SHORT>(width);
        newSize.Y = static_cast<SHORT>(height);
        CFarPlugin::GetPSI()->SendDlgMessage(m_Dlg, DM_RESIZEDIALOG, 0, reinterpret_cast<LONG_PTR>(&newSize));
    }

    /**
     * Show dialogs
     * \return -2 if error else dialog return code
     */
    inline int DoModal()
    {
        if (m_Dlg == INVALID_HANDLE_VALUE)
        {
            assert(!m_DlgItems.empty());
            m_Dlg = CFarPlugin::GetPSI()->DialogInit(CFarPlugin::GetPSI()->ModuleNumber, -1, -1, m_Width, m_Height, NULL, &m_DlgItems.front(), static_cast<unsigned int>(m_DlgItems.size()), 0, 0, &CFarDialog::InternalDialogMessageProc, 0);
            m_DlgItems.clear();  //Non actual for now
            if (m_Dlg == INVALID_HANDLE_VALUE)
            {
                return -2;
            }
        }

        AccessDlgInstances(m_Dlg, this);
        const int retCode = CFarPlugin::GetPSI()->DialogRun(m_Dlg);
        AccessDlgInstances(m_Dlg, this, true);

        return retCode;
    }

    /**
     * Redraw dialog
     */
    inline void Redraw()
    {
        assert(m_Dlg != INVALID_HANDLE_VALUE);
        CFarPlugin::GetPSI()->SendDlgMessage(m_Dlg, DM_REDRAW, 0, 0L);
    }

    /**
     * Move cursor position
     * \param dlgItemId item id
     * \param col horizontal coordinates (column)
     * \param row vertical coordinates (row)
     */
    inline void MoveCursor(const int dlgItemId, const int col, const int row)
    {
        assert(m_Dlg != INVALID_HANDLE_VALUE);
        COORD coord;
        coord.Y = static_cast<SHORT>(row);
        coord.X = static_cast<SHORT>(col);
        CFarPlugin::GetPSI()->SendDlgMessage(m_Dlg, DM_SETCURSORPOS, dlgItemId, reinterpret_cast<LONG_PTR>(&coord));
    }

    /**
     * Create dialog item
     * \param type dialog item type
     * \param colBegin column begin coordinate
     * \param colEnd column end coordinate
     * \param rowBegin row begin coordinate
     * \param rowEnd row end coordinate
     * \param ptrData data pointer
     * \param flags dialog item flags
     * \param dlgItem registered dialog item
     * \return dialog created dialogs item id
     */
    inline int CreateDlgItem(const int type, const int colBegin, const int colEnd,
                             const int rowBegin, const int rowEnd, const wchar_t *ptrData = NULL,
                             const DWORD flags = 0, FarDialogItem **dlgItem = NULL)
    {
        FarDialogItem item;
        ZeroMemory(&item, sizeof(item));

        item.Type = type;
        item.Flags = flags;
        item.X1 = colBegin;
        item.X2 = colEnd;
        item.Y1 = rowBegin;
        item.Y2 = rowEnd;
        item.PtrData = ptrData;

        m_DlgItems.push_back(item);
        if (dlgItem)
        {
            *dlgItem = &m_DlgItems.back();
        }
        return static_cast<int>(m_DlgItems.size()) - 1;
    }

    /**
     * Create separator dialog item
     * \param row vertical coordinates (row)
     * \param title separator title
     * \return dialog created dialogs item id
     */
    inline int CreateSeparator(const int row, const wchar_t *title = NULL, const DWORD flags = 0, FarDialogItem **dlgItem = NULL)
    {
        return CreateDlgItem(DI_TEXT, 0, GetWidth(), row, row, title, DIF_SEPARATOR | flags, dlgItem);
    }

    /**
     * Create text dialog item
     * \param col horizontal coordinates (column)
     * \param row vertical coordinates (row)
     * \param text dialog item text
     * \return dialog created dialogs item id
     */
    inline int CreateText(const int col, const int row, const wchar_t *text, const DWORD flags = 0, FarDialogItem **dlgItem = NULL)
    {
        return CreateDlgItem(DI_TEXT, col, col + lstrlen(text) - 1, row, row, text, flags, dlgItem);
    }

    /**
     * Create text dialog item (centered on dialog)
     * \param row vertical coordinates (row)
     * \param text dialog item text
     * \return dialog created dialogs item id
     */
    inline int CreateText(const int row, const wchar_t *text)
    {
        const int l = lstrlen(text);
        const int c = m_Width / 2 - l / 2;
        return CreateDlgItem(DI_TEXT, c, c + l - 1, row, row, text);
    }

    /**
     * Create edit dialog item
     * \param col horizontal coordinates (column)
     * \param row vertical coordinates (row)
     * \param width item width (MAX_SIZE to full width)
     * \param text dialog item default text
     * \param history history path (NULL to ignore)
     * \param dlgItem registered dialog item
     * \return dialog created dialogs item id
     */
    inline int CreateEdit(const int col, const int row, const int width,
                          const wchar_t *text = NULL, const wchar_t *history = NULL,
                          const DWORD flags = 0, FarDialogItem **dlgItem = NULL)
    {
        FarDialogItem *dlgItemEdit;
        const int itemId = CreateDlgItem(DI_EDIT, col, width == MAX_SIZE ? GetWidth() : col + width,
                                         row, row, text, (history ? DIF_HISTORY : 0) | flags, &dlgItemEdit);
        if (history)
        {
            dlgItemEdit->History = history;
        }
        if (dlgItem)
        {
            *dlgItem = dlgItemEdit;
        }
        return itemId;
    }

    /**
     * Create button dialog item
     * \param col horizontal coordinates (column)
     * \param row vertical coordinates (row)
     * \param width button width
     * \param title button text
     * \param flags button flags
     * \param dlgItem registered dialog item
     * \return dialog created dialogs item id
     */
    inline int CreateButton(const int col, const int row, const wchar_t *title, const int flags = 0L, FarDialogItem **dlgItem = NULL)
    {
        return CreateDlgItem(DI_BUTTON, col, col, row, row, title, flags, dlgItem);
    }

    /**
     * Create radio button dialog item
     * \param col horizontal coordinates (column)
     * \param row vertical coordinates (row)
     * \param title button text
     * \param flags button flags
     * \param dlgItem registered dialog item
     * \return dialog created dialogs item id
     */
    inline int CreateRadioButton(const int col, const int row, const wchar_t *title = NULL, const int flags = 0L, FarDialogItem **dlgItem = NULL)
    {
        return CreateDlgItem(DI_RADIOBUTTON, col, 0, row, row, title, flags, dlgItem);
    }

    /**
     * Create checkbox dialog item
     * \param col horizontal coordinates (column)
     * \param row vertical coordinates (row)
     * \param title button text
     * \param initState initial state
     * \param flags button flags
     * \param dlgItem registered dialog item
     * \return dialog created dialogs item id
     */
    inline int CreateCheckBox(const int col, const int row, const wchar_t *title, const bool initState = false, const int flags = 0L, FarDialogItem **dlgItem = NULL)
    {
        FarDialogItem *dlgItemCB;
        const int itemId = CreateDlgItem(DI_CHECKBOX, col, 0, row, row, title, flags, &dlgItemCB);
        dlgItemCB->Selected = initState ? TRUE : FALSE;
        if (dlgItem)
        {
            *dlgItem = dlgItemCB;
        }
        return itemId;
    }

    /**
     * Get Far dialog item description
     * \param dlgItemId dialog item id
     * \return dialog item description. Must be deleted after using.
     */
    inline FarDialogItem *GetDlgItem(const int dlgItemId)
    {
        assert(m_Dlg != INVALID_HANDLE_VALUE);

        const size_t sz = CFarPlugin::GetPSI()->SendDlgMessage(m_Dlg, DM_GETDLGITEM, dlgItemId, 0L);
        assert(sz);
        FarDialogItem *item = reinterpret_cast<FarDialogItem *>(new unsigned char[sz]);
        CFarPlugin::GetPSI()->SendDlgMessage(m_Dlg, DM_GETDLGITEM, dlgItemId, reinterpret_cast<LONG_PTR>(item));
        return item;
    }

    /**
     * Set Far dialog item description
     * \param dlgItemId dialog item id
     * \param dlgItem dialog item description
     */
    inline void SetDlgItem(const int dlgItemId, const FarDialogItem &dlgItem)
    {
        assert(m_Dlg != INVALID_HANDLE_VALUE);
        CFarPlugin::GetPSI()->SendDlgMessage(m_Dlg, DM_SETDLGITEM, dlgItemId, reinterpret_cast<LONG_PTR>(&dlgItem));
    }

    /**
     * Show/hide dialog item
     * \param dlgItemId dialog item id
     * \param show true to show, false to hide
     */
    inline void ShowDlgItem(const int dlgItemId, const bool show)
    {
        assert(m_Dlg != INVALID_HANDLE_VALUE);
        CFarPlugin::GetPSI()->SendDlgMessage(m_Dlg, DM_SHOWITEM, dlgItemId, show ? 1 : 0);
    }

    /**
     * Get list/combobox selection index
     * \param dlgItemId item id
     * \return list/combobox selection index
     */
    inline int GetSelectonIndex(const int dlgItemId) const
    {
        assert(m_Dlg != INVALID_HANDLE_VALUE);
        return static_cast<int>(CFarPlugin::GetPSI()->SendDlgMessage(m_Dlg, DM_LISTGETCURPOS, dlgItemId, 0));
    }

    /**
     * Get checkbox/radiobutton state
     * \param dlgItemId item id
     * \return checkbox/radiobutton state
     */
    bool GetCheckState(const int dlgItemId) const
    {
        assert(m_Dlg != INVALID_HANDLE_VALUE);
        return CFarPlugin::GetPSI()->SendDlgMessage(m_Dlg, DM_GETCHECK, dlgItemId, 0) == BSTATE_CHECKED;
    }


    /**
     * Set checkbox/radiobutton state
     * \param dlgItemId item id
     * \param check checkbox/radiobutton state
     */
    void SetCheckState(const int dlgItemId, const bool check) const
    {
        assert(m_Dlg != INVALID_HANDLE_VALUE);
        CFarPlugin::GetPSI()->SendDlgMessage(m_Dlg, DM_SETCHECK, dlgItemId, check ? BSTATE_CHECKED : BSTATE_UNCHECKED);
    }

    /**
     * Get dialog item text
     * \param dlgItemId item id
     * \return item text
     */
    inline std::wstring GetText(const int dlgItemId) const
    {
        assert(m_Dlg != INVALID_HANDLE_VALUE);

        std::wstring itemText;

        const LONG_PTR itemTextLen = CFarPlugin::GetPSI()->SendDlgMessage(m_Dlg, DM_GETTEXTLENGTH, dlgItemId, 0);
        if (itemTextLen != 0)
        {
            itemText.resize(itemTextLen);
            FarDialogItemData item;
            item.PtrLength = itemTextLen;
            item.PtrData = &itemText[0];
            CFarPlugin::GetPSI()->SendDlgMessage(m_Dlg, DM_GETTEXT, dlgItemId, reinterpret_cast<LONG_PTR>(&item));
        }

        return itemText;
    }

    /**
     * Set dialog item text
     * \param dlgItemId item id
     * \param text item text
     */
    inline void SetText(const int dlgItemId, const wchar_t *text) const
    {
        assert(m_Dlg != INVALID_HANDLE_VALUE);

        FarDialogItemData item;
        item.PtrLength = wcslen(text);
        item.PtrData = const_cast<wchar_t *>(text);

        CFarPlugin::GetPSI()->SendDlgMessage(m_Dlg, DM_SETTEXT, dlgItemId, reinterpret_cast<LONG_PTR>(&item));
    }

protected:
    /**
     * Dialog message processing procedure
     * \param msg message id
     * \param param1 first parameter
     * \param param2 second parameter
     * \return code depending on message id
     */
    virtual LONG_PTR DialogMessageProc(int msg, int param1, LONG_PTR param2)
    {
        // DEBUG_PRINTF(L"NetBox: msg = %u, DN_BTNCLICK = %u, param1 = %u", msg, DN_BTNCLICK, param1);
        return CFarPlugin::GetPSI()->DefDlgProc(m_Dlg, msg, param1, param2);
    }

private:
    //! Dialog message handler procedure (see Far help)
    static LONG_PTR WINAPI InternalDialogMessageProc(HANDLE dlg, int msg, int param1, LONG_PTR param2)
    {
        CFarDialog *dlgInst = AccessDlgInstances(dlg, NULL);
        assert(dlgInst);
        return dlgInst->DialogMessageProc(msg, param1, param2);
    }

    /**
     * Access to dialog instances std::map
     * \param dlg Far dialog handle
     * \param inst dialog class instance
     * \param removeVal true to remove value
     * \return dialog class instance for handle
     */
    static CFarDialog *AccessDlgInstances(HANDLE dlg, CFarDialog *inst, const bool removeVal = false)
    {
        static std::map<HANDLE, CFarDialog *> dlgInstances;
        if (dlg && inst && !removeVal)
        {
            dlgInstances.insert(std::make_pair(dlg, inst));
            return NULL;
        }
        else if (removeVal)
        {
            dlgInstances.erase(dlg);
            return NULL;
        }

        std::map<HANDLE, CFarDialog *>::iterator it = dlgInstances.find(dlg);
        assert(it != dlgInstances.end());
        return it->second;
    }

protected:
    HANDLE m_Dlg; ///< Dialog descriptor
    int m_Width; ///< Dialog width
    int m_Height; ///< Dialog height
    std::vector<FarDialogItem> m_DlgItems; ///< Dialog items array
    bool m_UseFrame; ///< Dialog frame flag
};

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
typedef void (TObject::*TFarKeyEvent)
    (TFarDialog *Sender, TFarDialogItem *Item, long KeyCode, bool &Handled);
typedef void (TObject::*TFarMouseClickEvent) 
    (TFarDialogItem *Item, MOUSE_EVENT_RECORD *Event);
typedef void (TObject::*TFarProcessGroupEvent)(TFarDialogItem *Item, void *Arg);
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
    TFarDialog(TCustomFarPlugin *AFarPlugin);
    ~TFarDialog();

    int ShowModal();
    void ShowGroup(int Group, bool Show);
    void EnableGroup(int Group, bool Enable);

    TRect GetBounds() const { return FBounds; }
    void SetBounds(const TRect &value);
    TRect GetClientRect() const;
    wstring GetHelpTopic() const { return FHelpTopic; }
    void SetHelpTopic(const wstring &value);
    unsigned int GetFlags() const { return FFlags; }
    void SetFlags(const unsigned int &value);
    bool GetCentered() const;
    void SetCentered(const bool &value);
    TPoint GetSize() const;
    void SetSize(const TPoint &value);
    TPoint GetClientSize() const;
    int GetWidth() const;
    void SetWidth(const int &value);
    int GetHeight() const;
    void SetHeight(const int &value);
    wstring GetCaption() const;
    void SetCaption(const wstring &value);
    HANDLE GetHandle() const { return FHandle; }
    TFarButton *GetDefaultButton() const { return FDefaultButton; }
    TFarBox *GetBorderBox() const { return FBorderBox; }
    TFarDialogItem *GetItem(size_t Index);
    size_t GetItemCount() const;
    TItemPosition GetNextItemPosition() const { return FNextItemPosition; }
    void SetNextItemPosition(const TItemPosition &value) { FNextItemPosition = value; }
    void GetNextItemPosition(int &Left, int &Top);
    int GetDefaultGroup() const { return FDefaultGroup; }
    void SetDefaultGroup(const int &value) { FDefaultGroup = value; }
    int GetTag() const { return FTag; }
    void SetTag(const int &value) { FTag = value; }
    TFarDialogItem *GetItemFocused() const { return FItemFocused; }
    void SetItemFocused(TFarDialogItem * const &value);
    int GetResult() const { return FResult; }
    TPoint GetMaxSize();
    
    TFarKeyEvent GetOnKey() const { return FOnKey; }
    void SetOnKey(const TFarKeyEvent &value) { FOnKey = value; }

    void Redraw();
    void LockChanges();
    void UnlockChanges();
    char GetSystemColor(unsigned int Index);
    bool HotKey(unsigned long Key);

    TCustomFarPlugin *GetFarPlugin() { return FFarPlugin; }

protected:
    TObjectList *GetItems() { return FItems; }

    void Add(TFarDialogItem *Item);
    void Add(TFarDialogContainer *Container);
    long SendMessage(int Msg, int Param1, int Param2);
    virtual long DialogProc(int Msg, int Param1, long Param2);
    virtual long FailDialogProc(int Msg, int Param1, long Param2);
    long DefaultDialogProc(int Msg, int Param1, long Param2);
    virtual bool MouseEvent(MOUSE_EVENT_RECORD *Event);
    virtual bool Key(TFarDialogItem *Item, long KeyCode);
    virtual void Change();
    virtual void Init();
    virtual bool CloseQuery();
    wstring GetMsg(int MsgId);
    void RefreshBounds();
    virtual void Idle();
    void BreakSynchronize();
    void Synchronize(TThreadMethod Method);
    void Close(TFarButton *Button);
    void ProcessGroup(int Group, TFarProcessGroupEvent Callback, void *Arg);
    void ShowItem(TFarDialogItem *Item, void *Arg);
    void EnableItem(TFarDialogItem *Item, void *Arg);
    bool ChangesLocked();
    TFarDialogItem *ItemAt(int X, int Y);

    static LONG_PTR WINAPI DialogProcGeneral(HANDLE Handle, int Msg, int Param1, LONG_PTR Param2);

private:
    TCustomFarPlugin *FFarPlugin;
    TRect FBounds;
    unsigned int FFlags;
    wstring FHelpTopic;
    bool FVisible;
    TObjectList *FItems;
    TObjectList *FContainers;
    HANDLE FHandle;
    TFarButton *FDefaultButton;
    TFarBox *FBorderBox;
    TItemPosition FNextItemPosition;
    int FDefaultGroup;
    int FTag;
    TFarDialogItem *FItemFocused;
    TFarKeyEvent FOnKey;
    FarDialogItem *FDialogItems;
    int FDialogItemsCapacity;
    int FChangesLocked;
    bool FChangesPending;
    int FResult;
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
    int GetLeft() { return FLeft; }
    void SetLeft(int value) { SetPosition(0, value); }
    int GetTop() { return FTop; }
    void SetTop(int value) { SetPosition(1, value); }
    size_t GetItemCount() const;
    bool GetEnabled() { return FEnabled; }
    void SetEnabled(bool value);

protected:
    TFarDialogContainer(TFarDialog *ADialog);
    ~TFarDialogContainer();

    TFarDialog *GetDialog() { return FDialog; }

    void Add(TFarDialogItem *Item);
    void Remove(TFarDialogItem *Item);
    virtual void Change();
    wstring GetMsg(int MsgId);

private:
    int FLeft;
    int FTop;
    TObjectList *FItems;
    TFarDialog *FDialog;
    bool FEnabled;

    void SetPosition(int Index, int value);
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
    int GetHeight();
    void SetHeight(int value);
    unsigned int GetFlags();
    void SetFlags(unsigned int value);
    bool GetEnabled() { return FEnabled; }
    void SetEnabled(bool value);
    bool GetIsEnabled() { return FIsEnabled; }
    TFarDialogItem *GetEnabledFollow() { return FEnabledFollow; }
    void SetEnabledFollow(TFarDialogItem *value);
    TFarDialogItem *GetEnabledDependency() { return FEnabledDependency; }
    void SetEnabledDependency(TFarDialogItem *value);
    TFarDialogItem *GetEnabledDependencyNegative() { return FEnabledDependencyNegative; }
    void SetEnabledDependencyNegative(TFarDialogItem *value);
    virtual bool GetIsEmpty();
    int GetGroup() { return FGroup; }
    void SetGroup(int value) { FGroup = value; }
    bool GetVisible() { return GetFlag(DIF_HIDDEN | DIF_INVERSE); }
    void SetVisible(bool value) { SetFlag(DIF_HIDDEN | DIF_INVERSE, value); }
    bool GetTabStop() { return GetFlag(DIF_NOFOCUS | DIF_INVERSE); }
    void SetTabStop(bool value) { SetFlag(DIF_NOFOCUS | DIF_INVERSE, value); }
    bool GetOem() { return FOem; }
    void SetOem(bool value) { FOem = value; }
    int GetTag() { return FTag; }
    void SetTag(int value) { FTag = value; }
    TFarDialog *GetDialog() { return FDialog; }

    TNotifyEvent GetOnExit() { return FOnExit; }
    void SetOnExit(TNotifyEvent value) { FOnExit = value; }
    TFarMouseClickEvent GetOnMouseClick() { return FOnMouseClick; }
    void SetOnMouseClick(TFarMouseClickEvent value) { FOnMouseClick = value; }

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
    int FTag;
    TNotifyEvent FOnExit;
    TFarMouseClickEvent FOnMouseClick;

    TFarDialogItem(TFarDialog *ADialog, int AType);
    ~TFarDialogItem();

    FarDialogItem *GetDialogItem();
    bool GetCenterGroup() { return GetFlag(DIF_CENTERGROUP); }
    void SetCenterGroup(bool value) { SetFlag(DIF_CENTERGROUP, value); }
    virtual wstring GetData();
    virtual void SetData(const wstring value);
    int GetType();
    void SetType(int value);
    int GetSelected();
    void SetSelected(int value);
    TFarDialogContainer *GetContainer() { return FContainer; }
    void SetContainer(TFarDialogContainer *value);
    bool GetChecked();
    void SetChecked(bool value);

    virtual void Detach();
    void DialogResized();
    long SendMessage(int Msg, int Param);
    long SendDialogMessage(int Msg, int Param1, int Param2);
    virtual long ItemProc(int Msg, long Param);
    long DefaultItemProc(int Msg, int Param);
    long DefaultDialogProc(int Msg, int Param1, int Param2);
    virtual long FailItemProc(int Msg, long Param);
    virtual void Change();
    void DialogChange();
    void SetAlterType(int Index, bool value);
    bool GetAlterType(int Index);
    virtual void UpdateBounds();
    virtual void ResetBounds();
    virtual void Init();
    virtual bool CloseQuery();
    virtual bool MouseMove(int X, int Y, MOUSE_EVENT_RECORD *Event);
    virtual bool MouseClick(MOUSE_EVENT_RECORD *Event);
    TPoint MouseClientPosition(MOUSE_EVENT_RECORD *Event);
    void Text(int X, int Y, int Color, wstring Str, bool Oem = false);
    void Redraw();
    virtual bool HotKey(char HotKey);

    virtual void SetDataInternal(const wstring value);
    void UpdateData(const wstring value);
    void UpdateSelected(int value);

    bool GetFlag(int Index);
    void SetFlag(int Index, bool value);

    virtual void DoFocus();
    virtual void DoExit();

    char GetColor(int Index);
    void SetColor(int Index, char value);

private:
    TFarDialog *FDialog;
    TRect FBounds;
    TFarDialogItem *FEnabledFollow;
    TFarDialogItem *FEnabledDependency;
    TFarDialogItem *FEnabledDependencyNegative;
    TFarDialogContainer *FContainer;
    size_t FItem;
    bool FEnabled;
    bool FIsEnabled;
    unsigned long FColors;
    unsigned long FColorMask;
    bool FOem;

    void UpdateFlags(unsigned int value);
    void SetCoordinate(int Index, int value);
    int GetCoordinate(int Index);
    TFarDialogItem *GetPrevItem();
    void UpdateFocused(bool value);
    void UpdateEnabled();
};
//---------------------------------------------------------------------------
class TFarBox : public TFarDialogItem
{
public:
    TFarBox(TFarDialog *ADialog);

    virtual wstring GetCaption() { return GetData(); }
    virtual void SetCaption(wstring value) { SetData(value); }
    virtual bool GetDouble() { return GetAlterType(DI_DOUBLEBOX); }
    virtual void SetDouble(bool value) { SetAlterType(DI_DOUBLEBOX, value); }
};
//---------------------------------------------------------------------------
typedef void (TObject::*TFarButtonClick)(TFarButton *Sender, bool &Close);
enum TFarButtonBrackets { brNone, brTight, brSpace, brNormal };
//---------------------------------------------------------------------------
class TFarButton : public TFarDialogItem
{
public:
    TFarButton(TFarDialog *ADialog);

    virtual wstring GetCaption() { return GetData(); }
    virtual void SetCaption(wstring value) { SetData(value); }
    virtual int GetResult() { return FResult; }
    virtual void SetResult(int value) { FResult = value; }
    bool GetDefault();
    void SetDefault(bool value);
    TFarButtonBrackets GetBrackets() { return FBrackets; }
    void SetBrackets(TFarButtonBrackets value);
    bool GetCenterGroup() { return TFarDialogItem::GetCenterGroup(); }
    void SetCenterGroup(bool value) { TFarDialogItem::SetCenterGroup(value); }
    virtual TFarButtonClick GetOnClick() { return FOnClick; }
    virtual void SetOnClick(TFarButtonClick value) { FOnClick = value; }

protected:
    virtual void SetDataInternal(const wstring value);
    virtual wstring GetData();
    virtual long ItemProc(int Msg, long Param);
    virtual bool HotKey(char HotKey);

private:
    int FResult;
    TFarButtonClick FOnClick;
    TFarButtonBrackets FBrackets;
};
//---------------------------------------------------------------------------
typedef void (*TFarAllowChange)(TFarDialogItem *Sender,
        long NewState, bool &AllowChange);
//---------------------------------------------------------------------------
class TFarCheckBox : public TFarDialogItem
{
public:
    TFarCheckBox(TFarDialog *ADialog);

    virtual wstring GetCaption() { return GetData(); }
    virtual void SetCaption(wstring value) { SetData(value); }
    bool GetAllowGrayed() { return GetFlag(DIF_3STATE); }
    void SetAllowGrayed(bool value) { SetFlag(DIF_3STATE, value); }
    virtual TFarAllowChange GetOnAllowChange() { return FOnAllowChange; }
    virtual void SetOnAllowChange(TFarAllowChange value) { FOnAllowChange = value; }
    bool GetChecked() { return TFarDialogItem::GetChecked(); }
    void SetChecked(bool value) { TFarDialogItem::SetChecked(value); }
    int GetSelected() { return TFarDialogItem::GetSelected(); }
    void SetSelected(int value) { TFarDialogItem::SetSelected(value); }

protected:
    TFarAllowChange FOnAllowChange;
    virtual long ItemProc(int Msg, long Param);
    virtual bool GetIsEmpty();
    virtual void SetData(const wstring value);
};
//---------------------------------------------------------------------------
class TFarRadioButton : public TFarDialogItem
{
public:
    TFarRadioButton(TFarDialog *ADialog);

    bool GetChecked() { return TFarDialogItem::GetChecked(); }
    void SetChecked(bool value) { TFarDialogItem::SetChecked(value); }
    virtual wstring GetCaption() { return GetData(); }
    virtual void SetCaption(wstring value) { SetData(value); }
    virtual TFarAllowChange GetOnAllowChange() { return FOnAllowChange; }
    virtual void SetOnAllowChange(TFarAllowChange value) { FOnAllowChange = value; }

protected:
    TFarAllowChange FOnAllowChange;
    virtual long ItemProc(int Msg, long Param);
    virtual bool GetIsEmpty();
    virtual void SetData(const wstring value);
};
//---------------------------------------------------------------------------
class TFarEdit : public TFarDialogItem
{
public:
    TFarEdit(TFarDialog *ADialog);

    virtual wstring GetText() { return GetData(); }
    virtual void SetText(wstring value) { SetData(value); }
    int GetAsInteger();
    void SetAsInteger(int value);
    virtual bool GetPassword() { return GetAlterType(DI_PSWEDIT); }
    virtual void SetPassword(bool value) { SetAlterType(DI_PSWEDIT, value); }
    virtual bool GetFixed() { return GetAlterType(DI_FIXEDIT); }
    virtual void SetFixed(bool value) { SetAlterType(DI_FIXEDIT, value); }

    virtual wstring GetMask() { return GetHistoryMask(1); }
    virtual void SetMask(wstring value) { SetHistoryMask(1, value); }
    virtual wstring GetHistory() { return GetHistoryMask(0); }
    virtual void SetHistory(wstring value) { SetHistoryMask(0, value); }
    bool GetExpandEnvVars() { return GetFlag(DIF_EDITEXPAND); }
    void SetExpandEnvVars(bool value) { SetFlag(DIF_EDITEXPAND, value); }
    bool GetAutoSelect() { return GetFlag(DIF_SELECTONENTRY); }
    void SetAutoSelect(bool value) { SetFlag(DIF_SELECTONENTRY, value); }
    bool GetReadOnly() { return GetFlag(DIF_READONLY); }
    void SetReadOnly(bool value) { SetFlag(DIF_READONLY, value); }

protected:
    virtual long ItemProc(int Msg, long Param);
    virtual void Detach();

private:
    wstring GetHistoryMask(int Index);
    void SetHistoryMask(int Index, wstring value);
};
//---------------------------------------------------------------------------
class TFarSeparator : public TFarDialogItem
{
public:
    TFarSeparator(TFarDialog *ADialog);

    bool GetDouble();
    void SetDouble(bool value);
    virtual wstring GetCaption() { return GetData(); }
    virtual void SetCaption(wstring value) { SetData(value); }
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
    TFarText(TFarDialog *ADialog);

    virtual wstring GetCaption() { return GetData(); }
    virtual void SetCaption(wstring value) { SetData(value); }
    bool GetCenterGroup() { return TFarDialogItem::GetCenterGroup(); }
    void SetCenterGroup(bool value) { TFarDialogItem::SetCenterGroup(value); }
    bool GetColor() { return TFarDialogItem::GetColor(0); }
    void SetColor(bool value) { TFarDialogItem::SetColor(0, value); }

protected:
    virtual void SetData(const wstring value);
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
    TFarList(TFarDialogItem *ADialogItem = NULL);
    virtual ~TFarList();

    virtual void Assign(TPersistent *Source);

    int GetSelected();
    void SetSelected(int value);
    int GetTopIndex();
    void SetTopIndex(int value);
    int GetMaxLength();
    int GetVisibleCount();
    unsigned int GetFlags(int Index);
    void SetFlags(int Index, unsigned int value);
    bool GetDisabled(int Index) { return GetFlag(Index, LIF_DISABLE); }
    void SetDisabled(int Index, bool value) { SetFlag(Index, LIF_DISABLE, value); }
    bool GetChecked(int Index) { return GetFlag(Index, LIF_CHECKED); }
    void SetChecked(int Index, bool value) { SetFlag(Index, LIF_CHECKED, value); }

protected:
    virtual void Changed();
    virtual long ItemProc(int Msg, long Param);
    virtual void Init();
    void UpdatePosition(int Position);
    int GetPosition();
    virtual void Put(int Index, const wstring S);
    void SetCurPos(int Position, int TopIndex);
    void UpdateItem(int Index);

    FarList *GetListItems() { return FListItems; }
    // void SetListItems(FarList *value) { FListItems = value; }
    TFarDialogItem *GetDialogItem() { return FDialogItem; }

private:
    FarList *FListItems;
    TFarDialogItem *FDialogItem;
    bool FNoDialogUpdate;

    inline int GetSelectedInt(bool Init);
    bool GetFlag(int Index, int Flag);
    void SetFlag(int Index, int Flag, bool value);
};
//---------------------------------------------------------------------------
enum TFarListBoxAutoSelect { asOnlyFocus, asAlways, asNever };
//---------------------------------------------------------------------------
class TFarListBox : public TFarDialogItem
{
    typedef TFarListBox self;
public:
    TFarListBox(TFarDialog *ADialog);
    ~TFarListBox();

    void SetItems(TStrings *value);

    bool GetNoAmpersand() { return GetFlag(DIF_LISTNOAMPERSAND); }
    void SetNoAmpersand(bool value) { SetFlag(DIF_LISTNOAMPERSAND, value); }
    bool GetAutoHighlight() { return GetFlag(DIF_LISTAUTOHIGHLIGHT); }
    void SetAutoHighlight(bool value) { SetFlag(DIF_LISTAUTOHIGHLIGHT, value); }
    bool GetNoBox() { return GetFlag(DIF_LISTNOBOX); }
    void SetNoBox(bool value) { SetFlag(DIF_LISTNOBOX, value); }
    bool GetWrapMode() { return GetFlag(DIF_LISTWRAPMODE); }
    void SetWrapMode(bool value) { SetFlag(DIF_LISTWRAPMODE, value); }
    TFarList *GetItems() { return FList; }
    void SetList(TFarList *value);
    TFarListBoxAutoSelect GetAutoSelect() { return FAutoSelect; }
    void SetAutoSelect(TFarListBoxAutoSelect value);

protected:
    virtual long ItemProc(int Msg, long Param);
    virtual void Init();
    virtual bool CloseQuery();

private:
    TFarList *FList;
    TFarListBoxAutoSelect FAutoSelect;
    bool FDenyClose;

    void UpdateMouseReaction();
};
//---------------------------------------------------------------------------
class TFarComboBox : public TFarDialogItem
{
    typedef TFarComboBox self;
public:
    TFarComboBox(TFarDialog *ADialog);
    ~TFarComboBox();

    void ResizeToFitContent();

    bool GetNoAmpersand() { return GetFlag(DIF_LISTNOAMPERSAND); }
    void SetNoAmpersand(bool value) { SetFlag(DIF_LISTNOAMPERSAND, value); }
    bool GetAutoHighlight() { return GetFlag(DIF_LISTAUTOHIGHLIGHT); }
    void SetAutoHighlight(bool value) { SetFlag(DIF_LISTAUTOHIGHLIGHT, value); }
    bool GetWrapMode() { return GetFlag(DIF_LISTWRAPMODE); }
    void SetWrapMode(bool value) { SetFlag(DIF_LISTWRAPMODE, value); }
    TFarList *GetItems() { return FList; }
    virtual wstring GetText() { return GetData(); }
    virtual void SetText(wstring value) { SetData(value); }
    bool GetAutoSelect() { return GetFlag(DIF_SELECTONENTRY); }
    void SetAutoSelect(bool value) { SetFlag(DIF_SELECTONENTRY, value); }
    bool GetDropDownList() { return GetFlag(DIF_DROPDOWNLIST); }
    void SetDropDownList(bool value) { SetFlag(DIF_DROPDOWNLIST, value); }

protected:
    virtual long ItemProc(int Msg, long Param);
    virtual void Init();

private:
    TFarList *FList;
};
//---------------------------------------------------------------------------
class TFarLister : public TFarDialogItem
{
    typedef TFarLister self;
public:
    TFarLister(TFarDialog *ADialog);
    virtual ~TFarLister();

    TStrings *GetItems();
    void SetItems(TStrings *value);
    int GetTopIndex() { return FTopIndex; }
    void SetTopIndex(int value);
    bool GetScrollBar();

protected:
    virtual long ItemProc(int Msg, long Param);
    virtual void DoFocus();

private:
    TStringList *FItems;
    int FTopIndex;

    void ItemsChange(TObject *Sender);
};
//---------------------------------------------------------------------------
wstring StripHotKey(wstring Text);
TRect Rect(int Left, int Top, int Right, int Bottom);
//---------------------------------------------------------------------------
