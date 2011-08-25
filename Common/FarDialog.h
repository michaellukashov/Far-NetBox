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
        m_Dlg(INVALID_HANDLE_VALUE), _Width(width), _Height(height), _UseFrame(false)
    {
    }

    /**
     * Constructor
     * \param width dialog width
     * \param height dialog height
     * \param title dialog title
     */
    explicit CFarDialog(const int width, const int height, const wchar_t *title) :
        m_Dlg(INVALID_HANDLE_VALUE), _Width(width), _Height(height), _UseFrame(true)
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
        assert(_DlgItems.empty());  //Must be first call!!!

        _UseFrame = true;
        CreateDlgItem(DI_DOUBLEBOX, 3, _Width - 4, 1, _Height - 2, title);
    }

    /**
     * Get top dialog coordinate
     * \return top dialog coordinate
     */
    inline int GetTop() const
    {
        return _UseFrame ? 2 : 0;
    }

    /**
     * Get left dialog coordinate
     * \return left dialog coordinate
     */
    inline int GetLeft() const
    {
        return _UseFrame ? 5 : 0;
    }

    /**
     * Get dialogs width
     * \return width
     */
    inline int GetWidth() const
    {
        return _Width - (_UseFrame ? 6 : 0);
    }

    /**
     * Get dialogs height
     * \return height
     */
    inline int GetHeight() const
    {
        return _Height - (_UseFrame ? 2 : 0);
    }

    /**
     * Resize dialog
     * \param width dialog width
     * \param height dialog height
     */
    inline void ResizeDialog(const int width, const int height)
    {
        assert(m_Dlg != INVALID_HANDLE_VALUE);
        _Width = width;
        _Height = height;
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
            assert(!_DlgItems.empty());
            m_Dlg = CFarPlugin::GetPSI()->DialogInit(CFarPlugin::GetPSI()->ModuleNumber, -1, -1, _Width, _Height, NULL, &_DlgItems.front(), static_cast<unsigned int>(_DlgItems.size()), 0, 0, &CFarDialog::InternalDialogMessageProc, 0);
            _DlgItems.clear();  //Non actual for now
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

        _DlgItems.push_back(item);
        if (dlgItem)
        {
            *dlgItem = &_DlgItems.back();
        }
        return static_cast<int>(_DlgItems.size()) - 1;
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
        const int c = _Width / 2 - l / 2;
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
     * Access to dialog instances map
     * \param dlg Far dialog handle
     * \param inst dialog class instance
     * \param removeVal true to remove value
     * \return dialog class instance for handle
     */
    static CFarDialog *AccessDlgInstances(HANDLE dlg, CFarDialog *inst, const bool removeVal = false)
    {
        static map<HANDLE, CFarDialog *> dlgInstances;
        if (dlg && inst && !removeVal)
        {
            dlgInstances.insert(make_pair(dlg, inst));
            return NULL;
        }
        else if (removeVal)
        {
            dlgInstances.erase(dlg);
            return NULL;
        }

        map<HANDLE, CFarDialog *>::iterator it = dlgInstances.find(dlg);
        assert(it != dlgInstances.end());
        return it->second;
    }

protected:
    HANDLE                  m_Dlg;       ///< Dialog descriptor
    int                     _Width;     ///< Dialog width
    int                     _Height;    ///< Dialog height
    vector<FarDialogItem>   _DlgItems;  ///< Dialog items array
    bool                    _UseFrame;  ///< Dialog frame flag
};
