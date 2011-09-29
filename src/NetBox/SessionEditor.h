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

#include "FarDialog.h"
#include "FarUtil.h"

class CSession;


/**
 * Base session editor dialog
 */
class CSessionEditor : protected CFarDialog
{
protected:
    explicit CSessionEditor(CSession *session, const int width, const int height);

public:
    /**
     * Show session editor dialog
     * \return false if edition was canceled by user
     */
    bool EditSession();

protected:
    /**
     * Create Codepage control
     * \param topPos top position in dialog
     * \param current current cp value
     * \param idCPText created text item's id
     * \param idCP created item's id
     */
    void CreateCodePageControl(const int topPos, const UINT current,
        int &idCPText, int &idCP);

    //From CFarDialog
    virtual LONG_PTR DialogMessageProc(int msg, int param1, LONG_PTR param2);

    /**
     * Prepare dialog
     */
    virtual void OnPrepareDialog() {}

    /**
     * Check fill data for correct form
     * \return false if edited data has incorrect format
     */
    virtual bool OnValidate() const
    {
        return true;
    }

    /**
     * Save settings from dialog
     */
    virtual void OnSave() {}

    virtual void ShowSessionDlgItems(bool visible);
    virtual void ShowProxyDlgItems(const ProxySettingsDialogParams &params, bool visible);

private:
    /**
     * Check fill data for correct form
     * \return false if edited data has incorrect format
     */
    bool Validate() const;

protected:
    int m_IdBtnSession;
    int m_IdBtnProxy;
    int m_IdPagesSeparator;
    int m_IdTextEditName;
    int m_IdEditName;
    int m_IdTextEditURL;
    int m_IdEditURL;
    int m_IdAuthSeparator;
    int m_IdTextEditUser;
    int m_IdEditUser;
    int m_IdTextEditPswShow;
    int m_IdEditPswShow;
    int m_IdEditPswHide;
    int m_IdChBxPromtpPsw;
    int m_IdChBxShowPsw;
    int m_IdBtnOK;
    int m_IdBtnCancel;

    bool m_EditMode;
    std::wstring m_Title;

    CSession *m_Session;

    ProxySettingsDialogParams m_params;
};

typedef std::auto_ptr<CSessionEditor> PSessionEditor;
