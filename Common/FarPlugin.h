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

#ifndef _export
#define _export __declspec(dllexport)
#endif  //_export

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif  //_WIN32_WINNT

#ifndef _WIN32_IE
#define _WIN32_IE 0x0501
#endif  //_WIN32_IE

#pragma warning(push, 1)
#include <plugin.hpp>
#include <farkeys.hpp>
#include <farcolor.hpp>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <iterator>
#include <algorithm>
#include <assert.h>
#pragma warning(pop)

using namespace std;

#define NETBOX_DEBUG

inline int __cdecl debug_printf(const wchar_t *format, ...)
{
    int len = 0;
#ifdef NETBOX_DEBUG
    va_list args;
    va_start(args, format);
    len = _vscwprintf(format, args) + 1;
    if (len <= 1)
    {
        return 0;
    }
    wstring buf(len, 0);
    vswprintf_s(&buf[0], buf.size(), format, args);
    va_end(args);
    buf.erase(buf.size() - 1); // Trim last NULL
    OutputDebugStringW(buf.c_str());
    // OutputDebugStringW(L"debug_printf: 1");
    // wstring buf2(len + 10 + strlen(funcname), 0);
    // swprintf_s(&buf2[0], buf2.size(), L"%s: %s\n", (wchar_t *)funcname, buf.c_str());
    // OutputDebugStringW(L"debug_printf: 2");
    // buf2.erase(buf2.size() - 1); // Trim last NULL
    // OutputDebugStringW(L"debug_printf: 3");
    // OutputDebugStringW(buf2.c_str());
#endif
    return len;
}

#ifdef NETBOX_DEBUG
    #define DEBUG_PRINTF(format, ...) debug_printf(format, __VA_ARGS__);
#else
    #define DEBUG_PRINTF(format, ...)
#endif

class CFarPlugin
{
public:
    /**
     * Initialize plugin
     * \param psi plugin startup info pointer
     */
    static void Initialize(const PluginStartupInfo *psi)
    {
        assert(psi);

        PluginStartupInfo *psiStatic = AccessPSI(psi);
        static FarStandardFunctions fsfStatic = *psi->FSF;
        psiStatic->FSF = &fsfStatic;
    }

    /**
     * Far message box
     * \param title message title
     * \param text message text
     * \param flags message box flags
     * \return message box exit status
     */
    static int MessageBox(const wchar_t *title, const wchar_t *text, const int flags)
    {
        assert(title);
        assert(text);

        wstring content(title);
        content += L'\n';
        content += text;
        return GetPSI()->Message(GetPSI()->ModuleNumber, FMSG_ALLINONE | flags, NULL, reinterpret_cast<const wchar_t* const *>(content.c_str()), 0, 0);
    }

    /**
     * Get Far plugin string resource
     * \param id string resource Id
     * \return string
     */
    static const wchar_t *GetString(const int id)
    {
        return GetPSI()->GetMsg(GetPSI()->ModuleNumber, id);
    }

    /**
     * Get Far plugin formatted string
     * \param id string resource Id
     * \param ... additional params
     * \return formatted string
     */
    static wstring GetFormattedString(const int id, ...)
    {
        const wchar_t *errFmt = GetString(id);
        assert(errFmt);
        va_list args;
        va_start(args, id);
        const int len = _vscwprintf(errFmt, args) + 1 /* last NULL */;
        if (len == 1)
        {
            return wstring();
        }
        wstring ret(len, 0);
        vswprintf_s(&ret[0], ret.size(), errFmt, args);
        ret.erase(ret.length() - 1);        ///Trim last NULL
        return ret;
    }

    /**
     * Get plugin library path
     * \return plugin library path
     */
    static const wchar_t *GetPluginPath()
    {
        static wstring pluginDir;
        if (pluginDir.empty())
        {
            pluginDir = GetPSI()->ModuleName;
            pluginDir.resize(pluginDir.find_last_of(L'\\') + 1);
        }
        return pluginDir.c_str();
    }

    /**
     * Get processing file name
     * \param openFrom open from type (see Far help)
     * \param item item pointer (see Far help)
     * \param fileName processing file name
     * \return false if error
     */
    static bool GetProcessingFileName(int openFrom, INT_PTR item, wstring &fileName)
    {
        fileName.clear();

        //Determine file name
        if (openFrom == OPEN_COMMANDLINE)
        {
            wchar_t *cmdString = reinterpret_cast<wchar_t *>(item);
            GetPSI()->FSF->Unquote(cmdString);
            GetPSI()->FSF->Trim(cmdString);
            const int fileNameLen = GetPSI()->FSF->ConvertPath(CPM_FULL, cmdString, NULL, 0);
            if (fileNameLen)
            {
                fileName.resize(fileNameLen);
                GetPSI()->FSF->ConvertPath(CPM_FULL, cmdString, &fileName[0], fileNameLen);
                fileName.erase(fileName.length() - 1);  //last NULL
            }
        }
        else if (openFrom == OPEN_PLUGINSMENU)
        {
            PanelInfo pi;
            if (!GetPSI()->Control(PANEL_ACTIVE, FCTL_GETPANELINFO, sizeof(pi), reinterpret_cast<LONG_PTR>(&pi)))
            {
                return false;
            }

            const size_t ppiBufferLength = GetPSI()->Control(PANEL_ACTIVE, FCTL_GETPANELITEM, pi.CurrentItem, static_cast<LONG_PTR>(NULL));
            if (ppiBufferLength == 0)
            {
                return false;
            }
            vector<unsigned char> ppiBuffer(ppiBufferLength);
            PluginPanelItem *ppi = reinterpret_cast<PluginPanelItem *>(&ppiBuffer.front());
            if (!GetPSI()->Control(PANEL_ACTIVE, FCTL_GETPANELITEM, pi.CurrentItem, reinterpret_cast<LONG_PTR>(ppi)))
            {
                return false;
            }
            const int fileNameLen = GetPSI()->FSF->ConvertPath(CPM_FULL, ppi->FindData.lpwszFileName, NULL, 0);
            if (fileNameLen)
            {
                fileName.resize(fileNameLen);
                GetPSI()->FSF->ConvertPath(CPM_FULL, ppi->FindData.lpwszFileName, &fileName[0], fileNameLen);
                fileName.erase(fileName.length() - 1);  //last NULL
            }
        }
        else if (openFrom == OPEN_VIEWER)
        {
            ViewerInfo vi;
            ZeroMemory(&vi, sizeof(vi));
            vi.StructSize = sizeof(vi);
            GetPSI()->ViewerControl(VCTL_GETINFO, &vi);
            if (vi.FileName)
            {
                fileName = vi.FileName;
            }
        }
        else if (openFrom == OPEN_EDITOR)
        {
            const int buffLen = GetPSI()->EditorControl(ECTL_GETFILENAME, NULL);
            if (buffLen)
            {
                fileName.resize(buffLen + 1, 0);
                GetPSI()->EditorControl(ECTL_GETFILENAME, &fileName[0]);
            }
        }

        return !fileName.empty();
    }

    /**
     * Encoding multibyte to wide string
     * \param src source string
     * \param cp code page
     * \return wide string
     */
    static wstring MB2W(const char *src, const UINT cp = CP_ACP)
    {
        assert(src);

        wstring wide;
        const int reqLength = MultiByteToWideChar(cp, 0, src, -1, NULL, 0);
        if (reqLength)
        {
            wide.resize(static_cast<size_t>(reqLength));
            MultiByteToWideChar(cp, 0, src, -1, &wide[0], reqLength);
            wide.erase(wide.length() - 1);  //remove NULL character
        }
        return wide;
    }

    /**
     * Encoding wide to multibyte string
     * \param src source string
     * \param cp code page
     * \return multibyte string
     */
    static string W2MB(const wchar_t *src, const UINT cp = CP_ACP)
    {
        assert(src);

        string mb;
        const int reqLength = WideCharToMultiByte(cp, 0, src, -1, 0, 0, NULL, NULL);
        if (reqLength)
        {
            mb.resize(static_cast<size_t>(reqLength));
            WideCharToMultiByte(cp, 0, src, -1, &mb[0], reqLength, NULL, NULL);
            mb.erase(mb.length() - 1);  //remove NULL character
        }
        return mb;
    }

public:
    /**
     * Call AdvControl function
     * \param command control command
     * \param param command parameter
     * \return call retcode
     */
    static INT_PTR AdvControl(const int command, void *param = NULL)
    {
        return GetPSI()->AdvControl(GetPSI()->ModuleNumber, command, param);
    }

    /**
     * Get plugin startup info pointer
     * \return plugin startup info pointer
     */
    static PluginStartupInfo *GetPSI()
    {
        assert(AccessPSI(NULL));
        return AccessPSI(NULL);
    }

    static void CheckAbortEvent(HANDLE *AbortEvent)
    {
        //Very-very bad architecture... TODO!
        static HANDLE stdIn = GetStdHandle(STD_INPUT_HANDLE);
        INPUT_RECORD rec;
        DWORD readCount = 0;
        while (*AbortEvent && PeekConsoleInput(stdIn, &rec, 1, &readCount) && readCount != 0)
        {
            ReadConsoleInput(stdIn, &rec, 1, &readCount);
            if (rec.EventType == KEY_EVENT && rec.Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE && rec.Event.KeyEvent.bKeyDown)
            {
                SetEvent(*AbortEvent);
            }
        }
    }

private:
    /**
     * Access to internal static plugin startup info
     * \param psi new plugin startup info (NULL to save privious)
     * \return plugin startup info pointer
     */
    static PluginStartupInfo *AccessPSI(const PluginStartupInfo *psi)
    {
        static PluginStartupInfo psiStatic;
        if (psi)
        {
            psiStatic = *psi;
        }
        return &psiStatic;
    }
};
