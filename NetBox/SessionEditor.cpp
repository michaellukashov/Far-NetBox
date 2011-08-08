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
#include "SessionEditor.h"
#include "Session.h"
#include "Strings.h"

CSessionEditor::CSessionEditor(CSession *session, const int width, const int height) :
    CFarDialog(width, height),
    m_IdBtnSession(0),
    m_IdBtnProxy(0),
    m_IdPagesSeparator(0),
    m_IdTextEditName(0),
    m_IdEditName(0),
    m_IdTextEditURL(0),
    m_IdEditURL(0),
    m_IdAuthSeparator(0),
    m_IdTextEditUser(0),
    m_IdEditUser(0),
    m_IdEditPswHide(0),
    m_IdTextEditPswShow(0),
    m_IdEditPswShow(0),
    m_IdChBxPromtpPsw(0),
    m_IdChBxShowPsw(0),
    m_IdBtnOK(0),
    m_IdBtnCancel(0),
    m_EditMode(false),
    m_Session(session)
{
    assert(m_Session);
}


bool CSessionEditor::EditSession()
{
    m_EditMode = (wcslen(m_Session->GetSessionName()) > 0);
    m_Title = CFarPlugin::GetString(m_EditMode ? StringEdEdtTitle : StringEdCrtTitle);
    m_Title += L" (";
    m_Title += CSession::GetProtocolName(m_Session->GetProtocolId());
    m_Title += L')';
    SetTitle(m_Title.c_str());

    wstring url = m_Session->GetURL();
    if (!m_EditMode)
    {
        url = CSession::GetDefaultScheme(m_Session->GetProtocolId());
        url += L"://";
    }

    int top = GetTop() + 2;
    // Создаем кнопку для перехода на настройки сессии
    m_IdBtnSession = CreateButton(GetLeft(), top - 2, CFarPlugin::GetString(StringSession), DIF_NOBRACKETS);
    // Создаем кнопку для перехода на настройки прокси
    m_IdBtnProxy = CreateButton(GetLeft() + wcslen(CFarPlugin::GetString(StringSession)) + 1,
        top - 2, CFarPlugin::GetString(StringProxy), DIF_NOBRACKETS);
    m_IdPagesSeparator = CreateSeparator(top - 1, CFarPlugin::GetString(StringSession));

    m_IdTextEditName = CreateText(GetLeft(), top + 0, CFarPlugin::GetString(StringEdName));
    FarDialogItem *editNameItem;
    m_IdEditName = CreateEdit(GetLeft(), top + 1, MAX_SIZE, m_Session->GetSessionName(), NULL, 0, &editNameItem);
    m_IdTextEditURL = CreateText(GetLeft(), top + 2, CFarPlugin::GetString(StringEdURL));
    m_IdEditURL = CreateEdit(GetLeft(), top + 3, MAX_SIZE, url.c_str());

    m_IdAuthSeparator = CreateSeparator(top + 4, CFarPlugin::GetString(StringEdAuth));
    m_IdTextEditUser = CreateText(GetLeft(), top + 5, CFarPlugin::GetString(StringEdAuthUser));
    m_IdEditUser = CreateEdit(GetLeft(), top + 6, MAX_SIZE, m_Session->GetUserName());
    m_IdTextEditPswShow = CreateText(GetLeft(), top + 7, CFarPlugin::GetString(StringEdAuthPsw));
    m_IdEditPswShow = CreateEdit(GetLeft(), top + 8, MAX_SIZE, m_Session->GetPassword());
    m_IdEditPswHide = CreateDlgItem(DI_PSWEDIT, GetLeft(), GetWidth(), top + 8, top + 8, m_Session->GetPassword());

    m_IdChBxPromtpPsw = CreateCheckBox(GetLeft(), top + 9, CFarPlugin::GetString(StringEdAuthPromtpPsw), m_Session->GetPromptPwd());
    m_IdChBxShowPsw = CreateCheckBox(GetLeft(), top + 10, CFarPlugin::GetString(StringEdAuthShowPsw), false);

    // Инициализируем настройки прокси
    ProxySettings proxySettings = m_Session->GetProxySettings();
    int topPos = GetTop() + 2;
    ::InitProxySettingsDialog(*this, topPos,
        proxySettings,
        m_params,
        false
    );

    OnPrepareDialog();

    CreateSeparator(GetHeight() - 2);
    FarDialogItem *itemOKBtn;
    m_IdBtnOK = CreateButton(0, GetHeight() - 1, CFarPlugin::GetString(StringOK), DIF_CENTERGROUP, &itemOKBtn);
    itemOKBtn->Focus = 1;
    m_IdBtnCancel = CreateButton(0, GetHeight() - 1, CFarPlugin::GetString(StringCancel), DIF_CENTERGROUP);
    const int ret = DoModal();
    if (ret < 0 || ret == m_IdBtnCancel)
    {
        return false;
    }

    // Парсим строку URL
    wstring scheme;
    wstring hostName;
    unsigned short port;
    wstring path;
    wstring query;
    wstring userName;
    wstring password;
    ::ParseURL(GetText(m_IdEditURL).c_str(), &scheme, &hostName, &port, &path, &query, &userName, &password);

    wstring sessionName = GetText(m_IdEditName);
    if (sessionName.empty())
    {
        sessionName = hostName;
    }
    m_Session->SetSessionName(sessionName.c_str());

    wstring sessionUserName = GetText(m_IdEditUser);
    if (sessionUserName.empty())
    {
        sessionUserName = userName;
    }
    m_Session->SetUserName(sessionUserName.c_str());

    wstring sessionPassword = GetText(m_IdEditPswHide);
    if (sessionPassword.empty())
    {
        sessionPassword = password;
    }
    m_Session->SetPassword(sessionPassword.c_str());

    wstring sessionURL = GetText(m_IdEditURL);
    if (!sessionUserName.empty() && !sessionPassword.empty())
    {
        sessionURL = scheme + L"://" + hostName + L":" + ::NumberToWString(port) + path + query;
    }
    DEBUG_PRINTF(L"NetBox: sessionURL = %s", sessionURL.c_str());
    m_Session->SetURL(sessionURL.c_str());

    m_Session->SetPromptPwd(GetCheckState(m_IdChBxPromtpPsw));
    ::GetProxySettings(*this, m_params, proxySettings);
    // DEBUG_PRINTF(L"NetBox: proxySettings.proxyType = %u, host = %s", proxySettings.proxyType, proxySettings.proxyHost.c_str());
    m_Session->SetProxySettings(proxySettings);

    OnSave();
    return true;
}


void CSessionEditor::CreateCodePageControl(const int topPos, const UINT current,
    int &idCPText, int &idCP)
{
    idCPText = CreateText(GetLeft(), topPos, CFarPlugin::GetString(StringEdCP));

    //Avialable codepages
    static vector<wstring> codePages;
    static vector<FarListItem> farListItems;
    static FarList farList;
    if (codePages.empty())
    {
        CPINFOEX cpInfoEx;
        if (GetCPInfoEx(CP_UTF8, 0, &cpInfoEx))
        {
            codePages.push_back(cpInfoEx.CodePageName);
        }
        if (GetCPInfoEx(CP_OEMCP, 0, &cpInfoEx))
        {
            codePages.push_back(cpInfoEx.CodePageName);
        }
        if (GetCPInfoEx(CP_ACP, 0, &cpInfoEx))
        {
            codePages.push_back(cpInfoEx.CodePageName);
        }
        if (GetCPInfoEx(20866, 0, &cpInfoEx))   //KOI8-r
        {
            codePages.push_back(cpInfoEx.CodePageName);
        }

        const size_t avialCP = codePages.size();
        farListItems.resize(avialCP);
        ZeroMemory(&farListItems[0], avialCP * sizeof(FarListItem));
        for (size_t i = 0; i < avialCP; ++i)
        {
            farListItems[i].Text = codePages[i].c_str();
        }
        farList.Items = &farListItems.front();
        farList.ItemsNumber = static_cast<int>(farListItems.size());
    }

    bool cpFound = false;
    for (size_t i = 0; i < farListItems.size(); ++i)
    {
        if (static_cast<UINT>(_wtoi(farListItems[i].Text)) != current)
        {
            farListItems[i].Flags = 0;
        }
        else
        {
            cpFound = true;
            farListItems[i].Flags = LIF_SELECTED;
        }
    }

    FarDialogItem *dlgItemidCPList;
    idCP = CreateDlgItem(DI_COMBOBOX, GetLeft(), GetWidth() - 1, topPos + 1, topPos + 1, NULL, DIF_LISTWRAPMODE, &dlgItemidCPList);
    dlgItemidCPList->ListItems = &farList;

    if (!cpFound)
    {
        // dlgItemidCPList->PtrData = ::NumberToWString(current).c_str();
        static wchar_t num[32];
        _itow_s(current, num, 10);
        dlgItemidCPList->PtrData = num;
    }
}


LONG_PTR CSessionEditor::DialogMessageProc(int msg, int param1, LONG_PTR param2)
{
    if (msg == DN_INITDIALOG)
    {
        ShowDlgItem(m_IdEditPswShow, false);
        ShowDlgItem(m_IdEditPswHide, true);
        DlgItem_SetFocus((*CFarPlugin::GetPSI()), m_Dlg, m_IdEditName);
    }
    else if (msg == DN_CLOSE && param1 >= 0 && param1 != m_IdBtnCancel && !Validate())
    {
        return FALSE;
    }
    else if (msg == DN_BTNCLICK && param1 == m_IdChBxShowPsw)
    {
        const bool showPwd = (param2 == 1);
        ShowDlgItem(m_IdEditPswShow, showPwd);
        ShowDlgItem(m_IdEditPswHide, !showPwd);
    }
    else if (msg == DN_BTNCLICK && param1 == m_IdBtnSession)
    {
        ShowSessionDlgItems(true);
        ShowProxyDlgItems(m_params, false);
        DlgItem_SetFocus((*CFarPlugin::GetPSI()), m_Dlg, m_IdEditName);
        SetText(m_IdPagesSeparator, CFarPlugin::GetString(StringSession));
        CFarPlugin::GetPSI()->SendDlgMessage(m_Dlg, DM_REDRAW, 0, 0);
        return TRUE;
    }
    else if (msg == DN_BTNCLICK && param1 == m_IdBtnProxy)
    {
        // DEBUG_PRINTF(L"NetBox: DN_BTNCLICK: param1 = %u, param2 = %u", param1, param2);
        // Прячем элементы диалога
        ShowSessionDlgItems(false);
        // Показываем элементы настроек прокси
        ShowProxyDlgItems(m_params, true);
        DlgItem_SetFocus((*CFarPlugin::GetPSI()), m_Dlg, m_params.idProxyTypeComboBox);
        SetText(m_IdPagesSeparator, CFarPlugin::GetString(StringProxy));
        CFarPlugin::GetPSI()->SendDlgMessage(m_Dlg, DM_REDRAW, 0, 0);
        return TRUE;
    }
    else if (msg == DN_EDITCHANGE && (param1 == m_IdEditPswHide || param1 == m_IdEditPswShow))
    {
        COORD coord;
        CFarPlugin::GetPSI()->SendDlgMessage(m_Dlg, DM_GETCURSORPOS, param1, reinterpret_cast<LONG_PTR>(&coord));
        if (param1 == m_IdEditPswHide)
        {
            SetText(m_IdEditPswShow, GetText(m_IdEditPswHide).c_str());
        }
        else
        {
            SetText(m_IdEditPswHide, GetText(m_IdEditPswShow).c_str());
        }
        CFarPlugin::GetPSI()->SendDlgMessage(m_Dlg, DM_SETCURSORPOS, param1, reinterpret_cast<LONG_PTR>(&coord));
    }

    return CFarDialog::DialogMessageProc(msg, param1, param2);
}

void CSessionEditor::ShowSessionDlgItems(bool visible)
{
    ShowDlgItem(m_IdTextEditName, visible);
    ShowDlgItem(m_IdEditName, visible);
    ShowDlgItem(m_IdTextEditURL, visible);
    ShowDlgItem(m_IdEditURL, visible);
    ShowDlgItem(m_IdAuthSeparator, visible);
    ShowDlgItem(m_IdTextEditUser, visible);
    ShowDlgItem(m_IdEditUser, visible);
    ShowDlgItem(m_IdTextEditPswShow, visible);
    ShowDlgItem(m_IdEditPswShow, visible);
    ShowDlgItem(m_IdEditPswHide, visible);
    ShowDlgItem(m_IdChBxPromtpPsw, visible);
    ShowDlgItem(m_IdChBxShowPsw, visible);
}

void CSessionEditor::ShowProxyDlgItems(const ProxySettingsDialogParams &params, bool visible)
{
    ShowDlgItem(params.idProxyTypeText, visible);
    ShowDlgItem(params.idProxyTypeComboBox, visible);
    ShowDlgItem(params.idSeparatorItem, visible);
    ShowDlgItem(params.idProxyHostText, visible);
    ShowDlgItem(params.idProxyHost, visible);
    ShowDlgItem(params.idProxyPortText, visible);
    ShowDlgItem(params.idProxyPort, visible);
    ShowDlgItem(params.idProxyLoginText, visible);
    ShowDlgItem(params.idProxyLogin, visible);
    ShowDlgItem(params.idProxyPasswordText, visible);
    ShowDlgItem(params.idProxyPassword, visible);
}

bool CSessionEditor::Validate() const
{
    wstring url = GetText(m_IdEditURL);
    if (url.empty())
    {
        CFarPlugin::MessageBox(m_Title.c_str(), CFarPlugin::GetString(StringEdErrURLEmpty), FMSG_MB_OK | FMSG_WARNING);
        return false;
    }

    //Check URL for valid form
    wstring schemeName, hostName;
    ParseURL(url.c_str(), &schemeName, NULL, NULL, NULL, NULL, NULL, NULL);
    if (schemeName.empty())
    {
        url = CSession::GetDefaultScheme(m_Session->GetProtocolId()) + url;
        SetText(m_IdEditURL, url.c_str());
    }
    ParseURL(url.c_str(), NULL, &hostName, NULL, NULL, NULL, NULL, NULL);
    if (hostName.empty())
    {
        CFarPlugin::MessageBox(m_Title.c_str(), CFarPlugin::GetString(StringEdErrURLInvalid), FMSG_MB_OK | FMSG_WARNING);
        return false;
    }

    wstring name = GetText(m_IdEditName);
    if (name.empty())
    {
        if (!m_EditMode)
        {
            name = hostName;
            SetText(m_IdEditName, name.c_str());
        }
        else
        {
            CFarPlugin::MessageBox(m_Title.c_str(), CFarPlugin::GetString(StringEdErrNameEmpty), FMSG_MB_OK | FMSG_WARNING);
            return false;
        }
    }
    else
    {
        static const wchar_t *restrictedSymbols = L"<>:\"/\\|?*";
        if (name.find_first_of(restrictedSymbols) != string::npos)
        {
            CFarPlugin::MessageBox(m_Title.c_str(), CFarPlugin::GetString(StringEdErrNameInvalid), FMSG_MB_OK | FMSG_WARNING);
            return false;
        }
    }

    return OnValidate();
}
