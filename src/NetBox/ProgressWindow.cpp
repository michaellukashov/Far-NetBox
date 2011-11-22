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

#include "stdafx.h"
#include "ProgressWindow.h"
#include "FarTexts.h"

#define WND_WIDTH 60

CProgressWindow::CProgressWindow(HANDLE abortEvent, const OperationType oper,
    const DirectionType direction, const size_t num, IProtocol *impl) :
    CFarDialog(WND_WIDTH + 10, num > 1 ? 11 : 11, CFarPlugin::GetString(oper == Copy ? StringCopyTitle : StringMoveTitle)),
    m_AbortEvent(abortEvent), m_WndThread(NULL), m_Operation(oper), m_Direction(direction), m_FileCount(num), m_ProtoImpl(impl),
    m_IdSrcFileName(0), m_IdDstFileName(0), m_IdTotalProgress(0), m_IdCurrentProgress(0), m_IdBtnCancel(0),
    m_Show(false)
{
    assert(m_AbortEvent);
    assert(m_FileCount > 0);
    assert(m_ProtoImpl);
}


CProgressWindow::~CProgressWindow()
{
    Destroy();
    ResetEvent(m_AbortEvent);
}


void CProgressWindow::Show()
{
    ResetEvent(m_AbortEvent);
    int topPos = GetTop();

    CreateText(GetLeft(), topPos, CFarPlugin::GetString(m_Direction == Send ? StringPrgSendFile : StringPrgRcvFile));
    m_IdSrcFileName = CreateText(GetLeft(), ++topPos, L"");
    CreateText(GetLeft(), ++topPos, CFarPlugin::GetString(StringPrgTo));
    m_IdDstFileName = CreateText(GetLeft(), ++topPos, L"");

    std::wstring prgBar(WND_WIDTH - 5, L'\x2591');
    prgBar += L"   0%";
    m_IdCurrentProgress = CreateText(GetLeft(), ++topPos, prgBar.c_str());

    CreateSeparator(GetHeight() - 2);
    m_IdBtnCancel = CreateButton(0, GetHeight() - 1, CFarPlugin::GetString(StringCancel), DIF_CENTERGROUP);

    m_WndThread = CreateThread(NULL, 0, WindowThread, this, 0, NULL);
    assert(m_WndThread);

    //Wait for dialog initilization complete
    int tryCount = 100;
    while (--tryCount && m_Dlg == INVALID_HANDLE_VALUE)
    {
        Sleep(100);
    }
    if (!tryCount)
    {
        assert(NULL);
        return; //Fatal error
    }

    CFarPlugin::AdvControl(ACTL_SETPROGRESSSTATE, reinterpret_cast<void *>(PS_INDETERMINATE));
    m_Show = true;
}


void CProgressWindow::SetFileNames(const wchar_t *srcFileName, const wchar_t *dstFileName)
{
    assert(srcFileName);
    assert(dstFileName);

    std::wstring trimmedSrcFileName(srcFileName);
    if (trimmedSrcFileName.length() > WND_WIDTH)
    {
        trimmedSrcFileName.replace(3, 3, 3, L'.');
        trimmedSrcFileName.erase(6, trimmedSrcFileName.length() - WND_WIDTH);
    }
    trimmedSrcFileName.resize(WND_WIDTH, L' ');

    std::wstring trimmedDstFileName(dstFileName);
    if (trimmedDstFileName.length() > WND_WIDTH)
    {
        trimmedDstFileName.replace(3, 3, 3, L'.');
        trimmedDstFileName.erase(6, trimmedDstFileName.length() - WND_WIDTH);
    }
    trimmedDstFileName.resize(WND_WIDTH, L' ');

    SetText(m_IdSrcFileName, trimmedSrcFileName.c_str());
    SetText(m_IdDstFileName, trimmedDstFileName.c_str());
}


void CProgressWindow::Destroy()
{
    if (!m_Show)
    {
        return;
    }
    CFarPlugin::GetPSI()->SendDlgMessage(m_Dlg, DM_CLOSE, 0, 0);

    if (m_WndThread && WaitForSingleObject(m_WndThread, 3000) == WAIT_TIMEOUT)
    {
        DWORD exitCode = 0;
        GetExitCodeThread(m_WndThread, &exitCode);
        if (exitCode == STILL_ACTIVE)
        {
            TerminateThread(m_WndThread, exitCode);
        }
        CloseHandle(m_WndThread);
    }

    CFarPlugin::AdvControl(ACTL_PROGRESSNOTIFY, 0);
    CFarPlugin::AdvControl(ACTL_SETPROGRESSSTATE, reinterpret_cast<void *>(PS_NOPROGRESS));
}


LONG_PTR CProgressWindow::DialogMessageProc(int msg, int param1, LONG_PTR param2)
{
    if (msg == DN_CLOSE)
    {
        SetEvent(m_AbortEvent);
    }
    else if (msg == DN_ENTERIDLE)
    {
        const int percent = m_ProtoImpl->GetProgress();
        if (percent >= 0 && percent <= 100)
        {
            std::wstring prgBar(WND_WIDTH - 5, L'\x2591');
            const size_t fillLength = percent * prgBar.length() / 100;
            prgBar.replace(0, fillLength, fillLength, L'\x2588');

            wchar_t procTxt[5];
            swprintf_s(procTxt, L"%d%%", percent);
            if (percent < 10)
            {
                prgBar += L' ';
            }
            if (percent < 100)
            {
                prgBar += L' ';
            }
            prgBar += L' ';
            prgBar += procTxt;

            SetText(m_IdCurrentProgress, prgBar.c_str());

            PROGRESSVALUE pv;
            pv.Completed = percent;
            pv.Total = 100;
            CFarPlugin::AdvControl(ACTL_SETPROGRESSVALUE, &pv);
        }
    }
    return CFarDialog::DialogMessageProc(msg, param1, param2);
}


DWORD WINAPI CProgressWindow::WindowThread(LPVOID param)
{
    CProgressWindow *wnd = static_cast<CProgressWindow *>(param);
    assert(wnd);
    return wnd->DoModal();
}


CNotificationWindow::CNotificationWindow(const wchar_t *title, const wchar_t *text)
    : m_Title(title), m_Text(text)
{
}


CNotificationWindow::~CNotificationWindow()
{
    Hide();
}


void CNotificationWindow::Show() const
{
    CFarPlugin::AdvControl(ACTL_SETPROGRESSSTATE, reinterpret_cast<void *>(PS_INDETERMINATE));
    const wchar_t *wndText[] = { m_Title.c_str(), m_Text.c_str() };
    CFarPlugin::GetPSI()->Message(CFarPlugin::GetPSI()->ModuleNumber, 0, NULL, wndText, sizeof(wndText) / sizeof(wndText[0]), 0);
}


void CNotificationWindow::Hide() const
{
    CFarPlugin::AdvControl(ACTL_PROGRESSNOTIFY, 0);
    CFarPlugin::AdvControl(ACTL_SETPROGRESSSTATE, reinterpret_cast<void *>(PS_NOPROGRESS));
    CFarPlugin::AdvControl(ACTL_REDRAWALL);
}
