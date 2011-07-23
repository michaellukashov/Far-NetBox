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
#include "Logging.h"
#include "resource.h"

//Registry settings names
static const wchar_t *RegPluginName =   L"NetBox";
static const wchar_t *RegAddToPM =      L"PanelMenu";
static const wchar_t *RegAddToDM =      L"DiskMenu";
static const wchar_t *RegUseOwnKey =    L"UseOwnKey";
static const wchar_t *RegPrefix =       L"Prefix";
static const wchar_t *RegAltPrefix =    L"AltPrefix";
static const wchar_t *RegTimeout =      L"Timeout";
static const wchar_t *RegSessionsPath = L"SessionsPath";

static const wchar_t *RegProxyType = L"ProxyType";
static const wchar_t *RegProxyHost = L"ProxyHost";
static const wchar_t *RegProxyPort = L"ProxyPort";
static const wchar_t *RegProxyLogin = L"ProxyLogin";
static const wchar_t *RegProxyPassword = L"ProxyPassword";

static const wchar_t *RegEnableLogging = L"EnableLogging";
static const wchar_t *RegLoggingLevel = L"LoggingLevel";
static const wchar_t *RegLogToFile = L"LogToFile";
static const wchar_t *RegLogFileName = L"LogFileName";

CSettings _Settings;

CSettings::CSettings() :
    _SettingsMenuIdx(0),
    _AddToDiskMenu(true),
    _AddToPanelMenu(false),
    _CmdPrefix(L"NetBox"),
    _AltPrefix(true),
    _UseOwnKey(false),
    _Timeout(10),
    _SessionPath(),
    _EnableLogging(false),
    _LoggingLevel(0),
    _LogToFile(false),
    _LogFileName(L"C:\\NetBox.log")
{
    // _LogFileName = CFarPlugin::GetString(StringLogFileName);
    _proxySettings.proxyType = PROXY_NONE;
    _proxySettings.proxyHost = L"127.0.0.1";
    _proxySettings.proxyPort = 1080;
    _proxySettings.proxyLogin = L"";
    _proxySettings.proxyPassword = L"";
}


void CSettings::Load()
{
    CFarSettings settings;
    if (settings.Open(RegPluginName, true))
    {
        DWORD regVal;
        wstring regStrVal;
        if (settings.GetNumber(RegAddToDM, regVal))
        {
            _AddToDiskMenu = (regVal != 0);
        }
        if (settings.GetNumber(RegAddToPM, regVal))
        {
            _AddToPanelMenu = (regVal != 0);
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

        // Прокси
        if (settings.GetNumber(RegProxyType, regVal))
        {
            _proxySettings.proxyType = regVal;
        }
        if (settings.GetString(RegProxyHost, regStrVal))
        {
            _proxySettings.proxyHost = regStrVal;
        }
        if (settings.GetNumber(RegProxyPort, regVal))
        {
            _proxySettings.proxyPort = regVal;
        }
        if (settings.GetString(RegProxyLogin, regStrVal))
        {
            _proxySettings.proxyLogin = regStrVal;
        }
        if (settings.GetString(RegProxyPassword, regStrVal))
        {
            _proxySettings.proxyPassword = regStrVal;
        }
        // DEBUG_PRINTF(L"NetBox: Load: _ProxyType = %d, _ProxyPort = %u, _ProxyPassword = %s", _ProxyType, _ProxyPort, _ProxyPassword.c_str());

        // Логирование
        if (settings.GetNumber(RegEnableLogging, regVal))
        {
            _EnableLogging = (regVal != 0);
        }
        if (settings.GetNumber(RegLoggingLevel, regVal))
        {
            _LoggingLevel = (regVal != 0);
        }
        if (settings.GetNumber(RegLogToFile, regVal))
        {
            _LogToFile = (regVal != 0);
        }
        if (settings.GetString(RegLogFileName, regStrVal))
        {
            _LogFileName = regStrVal;
        }
    }
}


void CSettings::Save() const
{
    CFarSettings settings;
    if (settings.Open(RegPluginName, false))
    {
        settings.SetNumber(RegAddToDM, _AddToDiskMenu ? 1 : 0);
        settings.SetNumber(RegAddToPM, _AddToPanelMenu ? 1 : 0);
        settings.SetNumber(RegUseOwnKey, _UseOwnKey ? 1 : 0);
        if (!_CmdPrefix.empty())
        {
            settings.SetString(RegPrefix, _CmdPrefix.c_str());
        }
        settings.SetNumber(RegAltPrefix, _AltPrefix ? 1 : 0);
        settings.SetNumber(RegTimeout, _Timeout);
        settings.SetString(RegSessionsPath, _SessionPath.c_str());
        // Настройки прокси
        settings.SetNumber(RegProxyType, _proxySettings.proxyType);
        settings.SetString(RegProxyHost, _proxySettings.proxyHost.c_str());
        settings.SetNumber(RegProxyPort, _proxySettings.proxyPort);
        settings.SetString(RegProxyLogin, _proxySettings.proxyLogin.c_str());
        settings.SetString(RegProxyPassword, _proxySettings.proxyPassword.c_str());
        // DEBUG_PRINTF(L"NetBox: Save: _ProxyType = %d, _ProxyPort = %u, _ProxyPassword = %s", _proxySettings.proxyType, _proxySettings.proxyPort, _proxySettings.proxyPassword.c_str());
        // Настройки логирования
        settings.SetNumber(RegEnableLogging, _EnableLogging ? 1 : 0);
        settings.SetNumber(RegLoggingLevel, _LoggingLevel);
        settings.SetNumber(RegLogToFile, _LogToFile ? 1 : 0);
        settings.SetString(RegLogFileName, _LogFileName.c_str());
    }
}

void CSettings::AddMenuItem(vector<FarMenuItemEx> &items, DWORD flags, int titleId)
{
    FarMenuItemEx item = {0};
    if (!(flags & MIF_SEPARATOR))
    {
        item.Text = CFarPlugin::GetString(titleId);
    }
    item.Flags = flags;
    items.push_back(item);
}

void CSettings::Configure()
{
    for (;;)
    {
        // Создаем меню с настройками
        vector<FarMenuItemEx> items;
        // Main settings
        size_t MainSettingsMenuIdx = items.size();
        AddMenuItem(items, 0, StringMainSettingsMenuTitle);
        // Proxy settings
        size_t ProxySettingsMenuIdx = items.size();
        AddMenuItem(items, 0, StringProxySettingsMenuTitle);
        // Logging settings
        size_t LoggingSettingsMenuIdx = items.size();
        AddMenuItem(items, 0, StringLoggingSettingsMenuTitle);
        //
        AddMenuItem(items, MIF_SEPARATOR, 0);
        // About
        size_t AboutMenuIdx = items.size();
        AddMenuItem(items, 0, StringAboutMenuTitle);
        items[_SettingsMenuIdx].Flags |= MIF_SELECTED;

        const size_t menuIdx = CFarPlugin::GetPSI()->Menu(CFarPlugin::GetPSI()->ModuleNumber,
            -1, -1, 0, FMENU_AUTOHIGHLIGHT | FMENU_WRAPMODE | FMENU_USEEXT,
            CFarPlugin::GetString(StringSettingsMenuTitle), NULL, NULL, NULL, NULL,
            reinterpret_cast<FarMenuItem *>(&items.front()),
            static_cast<int>(items.size()));
        if (menuIdx == MainSettingsMenuIdx)
        {
            MainConfigure();
        }
        else if (menuIdx == ProxySettingsMenuIdx)
        {
            ProxyConfigure();
        }
        else if (menuIdx == LoggingSettingsMenuIdx)
        {
            LoggingConfigure();
        }
        else if (menuIdx == AboutMenuIdx)
        {
            ShowAbout();
        }
        else
        {
            return;
        }
        // Сохраняем индекс выбранного элемента меню
        _SettingsMenuIdx = menuIdx;
        // DEBUG_PRINTF(L"new _SettingsMenuIdx = %d", _SettingsMenuIdx);
    }
}

void CSettings::MainConfigure()
{
    CFarDialog dlg(54, 17, CFarPlugin::GetString(StringTitle));
    int topPos = dlg.GetTop();

    const int idAddDM = dlg.CreateCheckBox(dlg.GetLeft(), topPos, CFarPlugin::GetString(StringCfgAddToDM), _AddToDiskMenu);
    const int idAddPM = dlg.CreateCheckBox(dlg.GetLeft(), ++topPos, CFarPlugin::GetString(StringCfgAddToPM), _AddToPanelMenu);

    dlg.CreateText(dlg.GetLeft(), ++topPos, CFarPlugin::GetString(StringCfgPrefix));

    const int idPrefix = dlg.CreateEdit(dlg.GetLeft() + static_cast<int>(wcslen(CFarPlugin::GetString(StringCfgPrefix))) + 1, topPos, MAX_SIZE, _CmdPrefix.c_str());
    const int idAltPrefix = dlg.CreateCheckBox(dlg.GetLeft(), ++topPos, CFarPlugin::GetString(StringCfgAltPrefixes), _AltPrefix);

    dlg.CreateSeparator(++topPos);

    const int idUseOwnKey = dlg.CreateCheckBox(dlg.GetLeft(), ++topPos, CFarPlugin::GetString(StringCfgUseOwnKey), _UseOwnKey);

    dlg.CreateSeparator(++topPos);

    dlg.CreateText(dlg.GetLeft(), ++topPos, CFarPlugin::GetString(StringCfgTimeout));
    wstring timeoutStr = ::NumberToWString(_Timeout);
    FarDialogItem *itemEdit;
    const int idTimeout = dlg.CreateDlgItem(DI_FIXEDIT, dlg.GetLeft() + static_cast<int>(wcslen(CFarPlugin::GetString(StringCfgTimeout))) + 1,
        dlg.GetWidth(), topPos, topPos, timeoutStr.c_str(), DIF_MASKEDIT, &itemEdit);
    itemEdit->Mask = L"99999999";

    dlg.CreateSeparator(++topPos);
    dlg.CreateText(dlg.GetLeft(), ++topPos, CFarPlugin::GetString(StringCfgSessionsPath));
    const int idSessPath = dlg.CreateEdit(dlg.GetLeft(), ++topPos, MAX_SIZE, _SessionPath.c_str());

    dlg.CreateSeparator(dlg.GetHeight() - 2);
    FarDialogItem *itemFocusBtn;
    dlg.CreateButton(0, dlg.GetHeight() - 1, CFarPlugin::GetString(StringOK), DIF_CENTERGROUP, &itemFocusBtn);
    itemFocusBtn->Focus = 1;
    const int idBtnCancel = dlg.CreateButton(0, dlg.GetHeight() - 1, CFarPlugin::GetString(StringCancel), DIF_CENTERGROUP);

    const int itemIdx = dlg.DoModal();
    if (itemIdx >= 0 && itemIdx != idBtnCancel)
    {
        _AddToDiskMenu = dlg.GetCheckState(idAddDM);
        _AddToPanelMenu = dlg.GetCheckState(idAddPM);
        _UseOwnKey = dlg.GetCheckState(idUseOwnKey);
        _CmdPrefix = dlg.GetText(idPrefix);
        _AltPrefix = dlg.GetCheckState(idAltPrefix);
        _Timeout = TextToNumber(dlg.GetText(idTimeout));
        _SessionPath = dlg.GetText(idSessPath);
        ::AppendPathDelimiterW(_SessionPath);
        Save();
    }
}

void CSettings::ProxyConfigure()
{
    CFarDialog dlg(54, 14, CFarPlugin::GetString(StringProxySettingsDialogTitle));
    int topPos = dlg.GetTop();
    ProxySettingsDialogParams params;
    ::InitProxySettingsDialog(dlg, topPos,
        _proxySettings,
        params,
        true
    );

    // Кнопки OK Cancel
    dlg.CreateSeparator(dlg.GetHeight() - 2);
    FarDialogItem *itemFocusBtn;
    dlg.CreateButton(0, dlg.GetHeight() - 1, CFarPlugin::GetString(StringOK), DIF_CENTERGROUP, &itemFocusBtn);
    const int idBtnCancel = dlg.CreateButton(0, dlg.GetHeight() - 1, CFarPlugin::GetString(StringCancel), DIF_CENTERGROUP);

    params.proxyTypeComboBoxItem->Focus = 1;
    const int itemIdx = dlg.DoModal();
    if (itemIdx >= 0 && itemIdx != idBtnCancel)
    {
        // Сохраняем опции
        ::GetProxySettings(dlg, params, _proxySettings);
        Save();
    }
}

void CSettings::LoggingConfigure()
{
    CFarDialog dlg(54, 12, CFarPlugin::GetString(StringLoggingDialogTitle));
    int topPos = dlg.GetTop();

    // Логирование включено/выключено
    FarDialogItem *dlgItemEnableLogging;
    const int idEnableLogging = dlg.CreateCheckBox(dlg.GetLeft(), topPos,
        CFarPlugin::GetString(StringLoggingDialogEnableLogging),
        _EnableLogging, 0L, &dlgItemEnableLogging);
    // Сепаратор
    dlg.CreateSeparator(++topPos, CFarPlugin::GetString(StringLoggingOptionsSeparatorTitle));
    // Уровень логирования
    dlg.CreateText(dlg.GetLeft(), ++topPos, CFarPlugin::GetString(StringLoggingOptionsLevel));

    FarDialogItem *itemLevelComboBox;
    const int idLevelComboBox = dlg.CreateDlgItem(DI_COMBOBOX, dlg.GetLeft() +
        static_cast<int>(wcslen(CFarPlugin::GetString(StringLoggingOptionsLevel))) + 1,
        dlg.GetWidth(), topPos, topPos, NULL, DIF_LISTWRAPMODE, &itemLevelComboBox);

    FarList levelsList;
    vector<FarListItem> levelsListItems;
    int levelsCount = 2;
    levelsListItems.resize(levelsCount);
    ZeroMemory(&levelsListItems[0], levelsCount * sizeof(FarListItem));
    for (int i = 0; i < levelsCount; ++i)
    {
        if (_LoggingLevel == i)
        {
            levelsListItems[i].Flags = LIF_SELECTED;
        }
        levelsListItems[i].Text = CFarPlugin::GetString(StringLoggingOptionsLevelItem1 + i);
    }
    levelsList.Items = &levelsListItems.front();
    levelsList.ItemsNumber = static_cast<int>(levelsListItems.size());
    itemLevelComboBox->ListItems = &levelsList;

    // Логирование в файл
    dlg.CreateSeparator(++topPos);
    const int idLogToFile = dlg.CreateCheckBox(dlg.GetLeft(), ++topPos,
        CFarPlugin::GetString(StringLoggingDialogLogToFile), _LogToFile);
    const int idLogFileName = dlg.CreateEdit(dlg.GetLeft() + 4,
        ++topPos, MAX_SIZE, _LogFileName.c_str());

    // Кнопки OK Cancel
    dlg.CreateSeparator(dlg.GetHeight() - 2);
    FarDialogItem *itemFocusBtn;
    dlg.CreateButton(0, dlg.GetHeight() - 1, CFarPlugin::GetString(StringOK), DIF_CENTERGROUP, &itemFocusBtn);
    const int idBtnCancel = dlg.CreateButton(0, dlg.GetHeight() - 1, CFarPlugin::GetString(StringCancel), DIF_CENTERGROUP);

    // Установка состояния элементов диалога
    // DEBUG_PRINTF(L"NetBox: idEnableLogging = %d, _EnableLogging = %d", idEnableLogging, idBtnCancel, _EnableLogging);
    dlgItemEnableLogging->Focus = 1;

    // Показываем диалог
    const int itemIdx = dlg.DoModal();
    if (itemIdx >= 0 && itemIdx != idBtnCancel)
    {
        // Сохраняем опции
        _EnableLogging = dlg.GetCheckState(idEnableLogging);
        _LoggingLevel = dlg.GetSelectonIndex(idLevelComboBox);
        // DEBUG_PRINTF(L"_LoggingLevel = %d", _LoggingLevel);
        _LogToFile = dlg.GetCheckState(idLogToFile);
        _LogFileName = dlg.GetText(idLogFileName);
        Save();
        CLogger::Initialize(EnableLogging(), LoggingLevel(), LogToFile(), LogFileName());
    }
}

void CSettings::ShowAbout()
{
    CFarDialog dlg(54, 12, CFarPlugin::GetString(StringAboutDialogTitle));
    int topPos = dlg.GetTop();
    dlg.CreateText(dlg.GetLeft() + 5, ++topPos, CFarPlugin::GetString(StringPluginDescriptionText));
    ++topPos;
    wstring ver = PLUGIN_VERSION_WTXT;
    wstring version = CFarPlugin::GetFormattedString(StringPluginVersion, ver.c_str());
    // DEBUG_PRINTF(L"NetBox: version = %s", version.c_str());
    dlg.CreateText(dlg.GetLeft() + 16, ++topPos, version.c_str());

    FarDialogItem *itemCloseBtn;
    dlg.CreateButton(0, dlg.GetHeight() - 1, CFarPlugin::GetString(StringPluginDescriptionClose), DIF_CENTERGROUP, &itemCloseBtn);
    itemCloseBtn->Focus = 1;

    dlg.DoModal();
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
