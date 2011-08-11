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

CSettings m_Settings;

CSettings::CSettings() :
    m_SettingsMenuIdx(0),
    m_AddToDiskMenu(true),
    m_AddToPanelMenu(false),
    m_CmdPrefix(L"NetBox"),
    m_AltPrefix(true),
    m_UseOwnKey(false),
    m_Timeout(10),
    m_SessionPath(),
    m_EnableLogging(false),
    m_LoggingLevel(0),
    m_LogToFile(false),
    m_LogFileName(L"C:\\NetBox.log")
{
    // m_LogFileName = CFarPlugin::GetString(StringLogFileName);
    m_proxySettings.proxyType = PROXY_NONE;
    m_proxySettings.proxyHost = L"127.0.0.1";
    m_proxySettings.proxyPort = 1080;
    m_proxySettings.proxyLogin = L"";
    m_proxySettings.proxyPassword = L"";
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
            m_AddToDiskMenu = (regVal != 0);
        }
        if (settings.GetNumber(RegAddToPM, regVal))
        {
            m_AddToPanelMenu = (regVal != 0);
        }
        settings.GetString(RegPrefix, m_CmdPrefix);
        if (settings.GetNumber(RegAltPrefix, regVal))
        {
            m_AltPrefix = (regVal != 0);
        }
        if (settings.GetNumber(RegUseOwnKey, regVal))
        {
            m_UseOwnKey = (regVal != 0);
        }
        if (settings.GetNumber(RegTimeout, regVal))
        {
            m_Timeout = regVal;
        }
        settings.GetString(RegSessionsPath, m_SessionPath);

        // Прокси
        if (settings.GetNumber(RegProxyType, regVal))
        {
            m_proxySettings.proxyType = regVal;
        }
        if (settings.GetString(RegProxyHost, regStrVal))
        {
            m_proxySettings.proxyHost = regStrVal;
        }
        if (settings.GetNumber(RegProxyPort, regVal))
        {
            m_proxySettings.proxyPort = regVal;
        }
        if (settings.GetString(RegProxyLogin, regStrVal))
        {
            m_proxySettings.proxyLogin = regStrVal;
        }
        if (settings.GetString(RegProxyPassword, regStrVal))
        {
            m_proxySettings.proxyPassword = regStrVal;
        }
        // DEBUG_PRINTF(L"NetBox: Load: m_ProxyType = %d, m_ProxyPort = %u, m_ProxyPassword = %s", m_ProxyType, m_ProxyPort, m_ProxyPassword.c_str());

        // Логирование
        if (settings.GetNumber(RegEnableLogging, regVal))
        {
            m_EnableLogging = (regVal != 0);
        }
        if (settings.GetNumber(RegLoggingLevel, regVal))
        {
            m_LoggingLevel = (regVal != 0);
        }
        if (settings.GetNumber(RegLogToFile, regVal))
        {
            m_LogToFile = (regVal != 0);
        }
        if (settings.GetString(RegLogFileName, regStrVal))
        {
            m_LogFileName = regStrVal;
        }
    }
}


void CSettings::Save() const
{
    CFarSettings settings;
    if (settings.Open(RegPluginName, false))
    {
        settings.SetNumber(RegAddToDM, m_AddToDiskMenu ? 1 : 0);
        settings.SetNumber(RegAddToPM, m_AddToPanelMenu ? 1 : 0);
        settings.SetNumber(RegUseOwnKey, m_UseOwnKey ? 1 : 0);
        if (!m_CmdPrefix.empty())
        {
            settings.SetString(RegPrefix, m_CmdPrefix.c_str());
        }
        settings.SetNumber(RegAltPrefix, m_AltPrefix ? 1 : 0);
        settings.SetNumber(RegTimeout, m_Timeout);
        settings.SetString(RegSessionsPath, m_SessionPath.c_str());
        // Настройки прокси
        settings.SetNumber(RegProxyType, m_proxySettings.proxyType);
        settings.SetString(RegProxyHost, m_proxySettings.proxyHost.c_str());
        settings.SetNumber(RegProxyPort, m_proxySettings.proxyPort);
        settings.SetString(RegProxyLogin, m_proxySettings.proxyLogin.c_str());
        settings.SetString(RegProxyPassword, m_proxySettings.proxyPassword.c_str());
        // DEBUG_PRINTF(L"NetBox: Save: m_ProxyType = %d, m_ProxyPort = %u, m_ProxyPassword = %s", m_proxySettings.proxyType, m_proxySettings.proxyPort, m_proxySettings.proxyPassword.c_str());
        // Настройки логирования
        settings.SetNumber(RegEnableLogging, m_EnableLogging ? 1 : 0);
        settings.SetNumber(RegLoggingLevel, m_LoggingLevel);
        settings.SetNumber(RegLogToFile, m_LogToFile ? 1 : 0);
        settings.SetString(RegLogFileName, m_LogFileName.c_str());
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
        items[m_SettingsMenuIdx].Flags |= MIF_SELECTED;

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
        m_SettingsMenuIdx = menuIdx;
        // DEBUG_PRINTF(L"new m_SettingsMenuIdx = %d", m_SettingsMenuIdx);
    }
}

void CSettings::MainConfigure()
{
    CFarDialog dlg(54, 17, CFarPlugin::GetString(StringTitle));
    int topPos = dlg.GetTop();

    const int idAddDM = dlg.CreateCheckBox(dlg.GetLeft(), topPos, CFarPlugin::GetString(StringCfgAddToDM), m_AddToDiskMenu);
    const int idAddPM = dlg.CreateCheckBox(dlg.GetLeft(), ++topPos, CFarPlugin::GetString(StringCfgAddToPM), m_AddToPanelMenu);

    dlg.CreateText(dlg.GetLeft(), ++topPos, CFarPlugin::GetString(StringCfgPrefix));

    const int idPrefix = dlg.CreateEdit(dlg.GetLeft() + static_cast<int>(wcslen(CFarPlugin::GetString(StringCfgPrefix))) + 1, topPos, MAX_SIZE, m_CmdPrefix.c_str());
    const int idAltPrefix = dlg.CreateCheckBox(dlg.GetLeft(), ++topPos, CFarPlugin::GetString(StringCfgAltPrefixes), m_AltPrefix);

    dlg.CreateSeparator(++topPos);

    const int idUseOwnKey = dlg.CreateCheckBox(dlg.GetLeft(), ++topPos, CFarPlugin::GetString(StringCfgUseOwnKey), m_UseOwnKey);

    dlg.CreateSeparator(++topPos);

    dlg.CreateText(dlg.GetLeft(), ++topPos, CFarPlugin::GetString(StringCfgTimeout));
    wstring timeoutStr = ::NumberToWString(m_Timeout);
    FarDialogItem *itemEdit;
    const int idTimeout = dlg.CreateDlgItem(DI_FIXEDIT, dlg.GetLeft() + static_cast<int>(wcslen(CFarPlugin::GetString(StringCfgTimeout))) + 1,
        dlg.GetWidth(), topPos, topPos, timeoutStr.c_str(), DIF_MASKEDIT, &itemEdit);
    itemEdit->Mask = L"99999999";

    dlg.CreateSeparator(++topPos);
    dlg.CreateText(dlg.GetLeft(), ++topPos, CFarPlugin::GetString(StringCfgSessionsPath));
    const int idSessPath = dlg.CreateEdit(dlg.GetLeft(), ++topPos, MAX_SIZE, m_SessionPath.c_str());

    dlg.CreateSeparator(dlg.GetHeight() - 2);
    FarDialogItem *itemFocusBtn;
    dlg.CreateButton(0, dlg.GetHeight() - 1, CFarPlugin::GetString(StringOK), DIF_CENTERGROUP, &itemFocusBtn);
    itemFocusBtn->Focus = 1;
    const int idBtnCancel = dlg.CreateButton(0, dlg.GetHeight() - 1, CFarPlugin::GetString(StringCancel), DIF_CENTERGROUP);

    const int itemIdx = dlg.DoModal();
    if (itemIdx >= 0 && itemIdx != idBtnCancel)
    {
        m_AddToDiskMenu = dlg.GetCheckState(idAddDM);
        m_AddToPanelMenu = dlg.GetCheckState(idAddPM);
        m_UseOwnKey = dlg.GetCheckState(idUseOwnKey);
        m_CmdPrefix = dlg.GetText(idPrefix);
        m_AltPrefix = dlg.GetCheckState(idAltPrefix);
        m_Timeout = TextToNumber(dlg.GetText(idTimeout));
        m_SessionPath = dlg.GetText(idSessPath);
        ::AppendPathDelimiterW(m_SessionPath);
        Save();
    }
}

void CSettings::ProxyConfigure()
{
    CFarDialog dlg(54, 14, CFarPlugin::GetString(StringProxySettingsDialogTitle));
    int topPos = dlg.GetTop();
    ProxySettingsDialogParams params;
    ::InitProxySettingsDialog(dlg, topPos,
        m_proxySettings,
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
        ::GetProxySettings(dlg, params, m_proxySettings);
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
        m_EnableLogging, 0L, &dlgItemEnableLogging);
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
        if (m_LoggingLevel == i)
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
        CFarPlugin::GetString(StringLoggingDialogLogToFile), m_LogToFile);
    const int idLogFileName = dlg.CreateEdit(dlg.GetLeft() + 4,
        ++topPos, MAX_SIZE, m_LogFileName.c_str());

    // Кнопки OK Cancel
    dlg.CreateSeparator(dlg.GetHeight() - 2);
    FarDialogItem *itemFocusBtn;
    dlg.CreateButton(0, dlg.GetHeight() - 1, CFarPlugin::GetString(StringOK), DIF_CENTERGROUP, &itemFocusBtn);
    const int idBtnCancel = dlg.CreateButton(0, dlg.GetHeight() - 1, CFarPlugin::GetString(StringCancel), DIF_CENTERGROUP);

    // Установка состояния элементов диалога
    // DEBUG_PRINTF(L"NetBox: idEnableLogging = %d, m_EnableLogging = %d", idEnableLogging, idBtnCancel, m_EnableLogging);
    dlgItemEnableLogging->Focus = 1;

    // Показываем диалог
    const int itemIdx = dlg.DoModal();
    if (itemIdx >= 0 && itemIdx != idBtnCancel)
    {
        // Сохраняем опции
        m_EnableLogging = dlg.GetCheckState(idEnableLogging);
        m_LoggingLevel = dlg.GetSelectonIndex(idLevelComboBox);
        // DEBUG_PRINTF(L"m_LoggingLevel = %d", m_LoggingLevel);
        m_LogToFile = dlg.GetCheckState(idLogToFile);
        m_LogFileName = dlg.GetText(idLogFileName);
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
    wstring path = ::ExpandEnvVars(SessionPath());
    DEBUG_PRINTF(L"NetBox: path = %s", path.c_str());
    if (path.empty())
    {
        path = ::ExpandEnvVars(CFarPlugin::GetPluginPath());
        path += L"Sessions\\";
    }
    return path;
}
