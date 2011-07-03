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
#include "Strings.h"

#define WND_WIDTH 60

CProgressWindow::CProgressWindow(HANDLE abortEvent, const OperationType oper, const DerectionType direction, const size_t num, IProtocol *impl)
    : CFarDialog(WND_WIDTH + 10, num > 1 ? 11 : 11, CFarPlugin::GetString(oper == Copy ? StringCopyTitle : StringMoveTitle)),
      _AbortEvent(abortEvent), _WndThread(NULL), _Operation(oper), _Direction(direction), _FileCount(num), _ProtoImpl(impl),
      _IdSrcFileName(0), _IdDstFileName(0), _IdTotalProgress(0), _IdCurrentProgress(0), _IdBtnCancel(0)
{
    assert(_AbortEvent);
    assert(_FileCount > 0);
    assert(_ProtoImpl);
}


CProgressWindow::~CProgressWindow()
{
    Destroy();
    ResetEvent(_AbortEvent);
}


void CProgressWindow::Show()
{
    ResetEvent(_AbortEvent);

    int topPos = GetTop();

    CreateText(GetLeft(), topPos, CFarPlugin::GetString(_Direction == Send ? StringPrgSendFile : StringPrgRcvFile));
    _IdSrcFileName = CreateText(GetLeft(), ++topPos, L"");
    CreateText(GetLeft(), ++topPos, CFarPlugin::GetString(StringPrgTo));
    _IdDstFileName = CreateText(GetLeft(), ++topPos, L"");

    wstring prgBar(WND_WIDTH - 5, L'\x2591');
    prgBar += L"   0%";
    _IdCurrentProgress = CreateText(GetLeft(), ++topPos, prgBar.c_str());

    CreateSeparator(GetHeight() - 2);
    _IdBtnCancel = CreateButton(0, GetHeight() - 1, CFarPlugin::GetString(StringCancel), DIF_CENTERGROUP);

    _WndThread = CreateThread(NULL, 0, WindowThread, this, 0, NULL);
    assert(_WndThread);

    //Wait for dialog initilization complete
    int tryCount = 100;
    while (--tryCount && _Dlg == INVALID_HANDLE_VALUE)
    {
        Sleep(100);
    }
    if (!tryCount)
    {
        assert(NULL);
        return; //Fatal error
    }

    CFarPlugin::AdvControl(ACTL_SETPROGRESSSTATE, reinterpret_cast<void *>(PS_INDETERMINATE));
}


void CProgressWindow::SetFileNames(const wchar_t *srcFileName, const wchar_t *dstFileName)
{
    assert(srcFileName);
    assert(dstFileName);

    wstring trimmedSrcFileName(srcFileName);
    if (trimmedSrcFileName.length() > WND_WIDTH)
    {
        trimmedSrcFileName.replace(3, 3, 3, L'.');
        trimmedSrcFileName.erase(6, trimmedSrcFileName.length() - WND_WIDTH);
    }
    trimmedSrcFileName.resize(WND_WIDTH, L' ');

    wstring trimmedDstFileName(dstFileName);
    if (trimmedDstFileName.length() > WND_WIDTH)
    {
        trimmedDstFileName.replace(3, 3, 3, L'.');
        trimmedDstFileName.erase(6, trimmedDstFileName.length() - WND_WIDTH);
    }
    trimmedDstFileName.resize(WND_WIDTH, L' ');

    SetText(_IdSrcFileName, trimmedSrcFileName.c_str());
    SetText(_IdDstFileName, trimmedDstFileName.c_str());
}


void CProgressWindow::Destroy()
{
    CFarPlugin::GetPSI()->SendDlgMessage(_Dlg, DM_CLOSE, 0, 0);

    if (_WndThread && WaitForSingleObject(_WndThread, 3000) == WAIT_TIMEOUT)
    {
        DWORD exitCode = 0;
        GetExitCodeThread(_WndThread, &exitCode);
        if (exitCode == STILL_ACTIVE)
        {
            TerminateThread(_WndThread, exitCode);
        }
        CloseHandle(_WndThread);
    }

    CFarPlugin::AdvControl(ACTL_PROGRESSNOTIFY, 0);
    CFarPlugin::AdvControl(ACTL_SETPROGRESSSTATE, reinterpret_cast<void *>(PS_NOPROGRESS));
}


LONG_PTR CProgressWindow::DialogMessageProc(int msg, int param1, LONG_PTR param2)
{
    if (msg == DN_CLOSE)
    {
        SetEvent(_AbortEvent);
    }
    else if (msg == DN_ENTERIDLE)
    {
        const int percent = _ProtoImpl->GetProgress();
        if (percent >= 0 && percent <= 100)
        {
            wstring prgBar(WND_WIDTH - 5, L'\x2591');
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

            SetText(_IdCurrentProgress, prgBar.c_str());

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
    : _Title(title), _Text(text)
{
}


CNotificationWindow::~CNotificationWindow()
{
    Hide();
}


void CNotificationWindow::Show() const
{
    CFarPlugin::AdvControl(ACTL_SETPROGRESSSTATE, reinterpret_cast<void *>(PS_INDETERMINATE));
    const wchar_t *wndText[] = { _Title.c_str(), _Text.c_str() };
    CFarPlugin::GetPSI()->Message(CFarPlugin::GetPSI()->ModuleNumber, 0, NULL, wndText, sizeof(wndText) / sizeof(wndText[0]), 0);
}


void CNotificationWindow::Hide() const
{
    CFarPlugin::AdvControl(ACTL_PROGRESSNOTIFY, 0);
    CFarPlugin::AdvControl(ACTL_SETPROGRESSSTATE, reinterpret_cast<void *>(PS_NOPROGRESS));
    CFarPlugin::AdvControl(ACTL_REDRAWALL);
}
