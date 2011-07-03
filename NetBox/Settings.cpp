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
#include "Settings.h"
#include "Session.h"
#include "SessionManager.h"
#include "Strings.h"

//Registry settings names
static const wchar_t *RegPluginName =   L"NetBox";
static const wchar_t *RegAddToPM =      L"PanelMenu";
static const wchar_t *RegAddToDM =      L"DiskMenu";
static const wchar_t *RegUseOwnKey =    L"UseOwnKey";
static const wchar_t *RegPrefix =       L"Prefix";
static const wchar_t *RegAltPrefix =    L"AltPrefix";
static const wchar_t *RegTimeout =      L"Timeout";
static const wchar_t *RegSessionsPath = L"SessionsPath";

CSettings _Settings;


CSettings::CSettings()
    : _AddToPanelMenu(false),
      _AddToDiskMenu(true),
      _CmdPrefix(L"NetBox"),
      _AltPrefix(true),
      _UseOwnKey(false),
      _Timeout(60),
      _SessionPath()
{
}


void CSettings::Load()
{
    CFarSettings settings;
    if (settings.Open(RegPluginName, true))
    {
        DWORD regVal;
        if (settings.GetNumber(RegAddToPM, regVal))
        {
            _AddToPanelMenu = (regVal != 0);
        }
        if (settings.GetNumber(RegAddToDM, regVal))
        {
            _AddToDiskMenu = (regVal != 0);
        }
        settings.GetString(RegPrefix, _CmdPrefix);
        if (settings.GetNumber(RegAltPrefix, regVal))
        {
            _AltPrefix = (regVal != 0);
        }
        if (settings.GetNumber(RegUseOwnKey, regVal))
        {
            _UseOwnKey = (regVal != 0);
        }
        if (settings.GetNumber(RegTimeout, regVal))
        {
            _Timeout = regVal;
        }
        settings.GetString(RegSessionsPath, _SessionPath);
    }
}


void CSettings::Save() const
{
    CFarSettings settings;
    if (settings.Open(RegPluginName, false))
    {
        settings.SetNumber(RegAddToPM, _AddToPanelMenu ? 1 : 0);
        settings.SetNumber(RegAddToDM, _AddToDiskMenu ? 1 : 0);
        settings.SetNumber(RegUseOwnKey, _UseOwnKey ? 1 : 0);
        if (!_CmdPrefix.empty())
        {
            settings.SetString(RegPrefix, _CmdPrefix.c_str());
        }
        settings.SetNumber(RegAltPrefix, _AltPrefix ? 1 : 0);
        settings.SetNumber(RegTimeout, _Timeout);
        settings.SetString(RegSessionsPath, _SessionPath.c_str());
    }
}


void CSettings::Configure()
{
    CFarDialog dlg(54, 17, CFarPlugin::GetString(StringTitle));
    int topPos = dlg.GetTop();

    const int idAddPM = dlg.CreateCheckBox(dlg.GetLeft(), topPos, CFarPlugin::GetString(StringCfgAddToPM), _AddToPanelMenu);
    const int idAddDM = dlg.CreateCheckBox(dlg.GetLeft(), ++topPos, CFarPlugin::GetString(StringCfgAddToDM), _AddToDiskMenu);

    dlg.CreateText(dlg.GetLeft(), ++topPos, CFarPlugin::GetString(StringCfgPrefix));

    const int idPrefix = dlg.CreateEdit(dlg.GetLeft() + static_cast<int>(wcslen(CFarPlugin::GetString(StringCfgPrefix))) + 1, topPos, MAX_SIZE, _CmdPrefix.c_str());
    const int idAltPrefix = dlg.CreateCheckBox(dlg.GetLeft(), ++topPos, CFarPlugin::GetString(StringCfgAltPrefixes), _AltPrefix);

    dlg.CreateSeparator(++topPos);

    const int idUseOwnKey = dlg.CreateCheckBox(dlg.GetLeft(), ++topPos, CFarPlugin::GetString(StringCfgUseOwnKey), _UseOwnKey);

    dlg.CreateSeparator(++topPos);

    dlg.CreateText(dlg.GetLeft(), ++topPos, CFarPlugin::GetString(StringCfgTimeout));
    wchar_t toText[16];
    _itow_s(_Timeout, toText, 10);
    FarDialogItem *itemEdit;
    const int idTimeout = dlg.CreateDlgItem(DI_FIXEDIT, dlg.GetLeft() + static_cast<int>(wcslen(CFarPlugin::GetString(StringCfgTimeout))) + 1, dlg.GetWidth(), topPos, topPos, toText, DIF_MASKEDIT, &itemEdit);
    itemEdit->Mask = L"99999999";

    dlg.CreateSeparator(++topPos);
    dlg.CreateText(dlg.GetLeft(), ++topPos, CFarPlugin::GetString(StringCfgSessionsPath));
    const int idSessPath = dlg.CreateEdit(dlg.GetLeft(), ++topPos, MAX_SIZE, _SessionPath.c_str());

    dlg.CreateSeparator(dlg.GetHeight() - 2);
    FarDialogItem *itemFocusBtn;
    dlg.CreateButton(0, dlg.GetHeight() - 1, CFarPlugin::GetString(StringOK), DIF_CENTERGROUP, &itemFocusBtn);
    itemFocusBtn->Focus = 1;
    const int idBtnCancel = dlg.CreateButton(0, dlg.GetHeight() - 1, CFarPlugin::GetString(StringCancel), DIF_CENTERGROUP);

    const int retCode = dlg.DoModal();
    if (retCode >= 0 && retCode != idBtnCancel)
    {
        _AddToPanelMenu = dlg.GetCheckState(idAddPM);
        _AddToDiskMenu = dlg.GetCheckState(idAddDM);
        _UseOwnKey = dlg.GetCheckState(idUseOwnKey);
        _CmdPrefix = dlg.GetText(idPrefix);
        _AltPrefix = dlg.GetCheckState(idAltPrefix);
        _Timeout = static_cast<unsigned long>(_wtoi(dlg.GetText(idTimeout).c_str()));
        _SessionPath = dlg.GetText(idSessPath);
        if (!_SessionPath.empty() && _SessionPath[_SessionPath.length() - 1] != L'/' && _SessionPath[_SessionPath.length() - 1] != L'\\')
        {
            _SessionPath += L'\\';
        }
        Save();
    }
}


wstring CSettings::GetSessionPath() const
{
    wstring path = SessionPath();
    if (path.empty())
    {
        path = CFarPlugin::GetPluginPath();
        path += L"Sessions\\";
    }
    return path;
}
