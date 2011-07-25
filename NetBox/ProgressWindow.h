/**************************************************************************
 *  NetBox plugin for FAR 2.0 (http://code.google.com/p/farplugs)         *
 *  Copyright (C) 2011 by Artem Senichev <artemsen@gmail.com>             *
 *  Copyright (C) 2011 by Michael Lukashov <michael.lukashov@gmail.com>   *
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

#include "Protocol.h"


class CProgressWindow : public CFarDialog
{
public:
    //! Operation types
    enum OperationType
    {
        Copy,
        Move
    };

    //! Derection types
    enum DerectionType
    {
        Send,
        Receive
    };

    CProgressWindow(HANDLE abortEvent, const OperationType oper, const DerectionType direction, const size_t num, IProtocol *impl);
    ~CProgressWindow();

    /**
     * Show progress window
     */
    void Show();

    /**
     * Hide and destroy window
     */
    void Destroy();

    /**
     * Update window info
     * \param srcFileName source file name
     * \param dstFileName destination file name
     * \return
     */
    void SetFileNames(const wchar_t *srcFileName, const wchar_t *dstFileName);

private:
    //From CFarDialog
    LONG_PTR DialogMessageProc(int msg, int param1, LONG_PTR param2);

    //! Window processing thread entry
    static DWORD WINAPI WindowThread(LPVOID param);

private:
    HANDLE          m_AbortEvent;
    HANDLE          m_WndThread;
    OperationType   m_Operation;
    DerectionType   m_Direction;
    size_t          m_FileCount;
    IProtocol      *m_ProtoImpl;

    int     m_IdSrcFileName;
    int     m_IdDstFileName;
    int     m_IdTotalProgress;
    int     m_IdCurrentProgress;
    int     m_IdBtnCancel;
};


class CNotificationWindow
{
public:
    CNotificationWindow(const wchar_t *title, const wchar_t *text);
    virtual ~CNotificationWindow();

    /**
     * Show progress window
     */
    virtual void Show() const;

    /**
     * Hide progress window
     */
    virtual void Hide() const;

protected:
    wstring m_Title;
    wstring m_Text;
};
