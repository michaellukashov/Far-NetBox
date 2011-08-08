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

#include "Settings.h"
#include "Protocol.h"
#include "Strings.h"
#include "SessionManager.h"

#pragma once


class CPanel
{
public:
    explicit CPanel(const bool exitToSessionMgr);
    ~CPanel();

    /**
     * Open connection
     * \param protoImpl client's protocol implementation
     * \return false if error
     */
    bool OpenConnection(IProtocol *protoImpl);

    /**
     * Close connection
     */
    void CloseConnection();

    /**
     * Get open plugin (panel) info
     * \param pluginInfo pointer to info structure
     */
    void GetOpenPluginInfo(OpenPluginInfo *pluginInfo);

    /**
     * Process key press event
     * \param key key code
     * \param controlState keyboard control state
     * \return FAR's return code for ProcessKeyW
     */
    int ProcessKey(const int key, const unsigned int controlState);

    /**
     * Change directory
     * \param dir new directory
     * \param opMode operation mode
     * \return FAR's return code for SetDirectoryW
     */
    int ChangeDirectory(const wchar_t *dir, const int opMode);

    /**
     * Make directory
     * \param name initial directory name
     * \param opMode operation mode
     * \return FAR's return code for MakeDirectoryW
     */
    int MakeDirectory(const wchar_t **name, const int opMode);

    /**
     * Get FAR panel item list
     * \param panelItem pointer to panel items
     * \param itemsNumber pinter to items counter
     * \param opMode operation mode
     * \return FAR's return code for GetFindDataW
     */
    int GetItemList(PluginPanelItem **panelItem, int *itemsNumber, const int opMode);

    /**
     * Free FAR panel item list
     * \param panelItem pointer to panel items
     * \param itemsNumber items counter
     */
    void FreeItemList(PluginPanelItem *panelItem, int itemsNumber);

    /**
     * Get files
     * \param panelItem pointer to panel items
     * \param itemsNumber items counter
     * \param dstPath destination path to file save
     * \param deleteSource delete source files flag (move mode)
     * \param opMode operation mode
     * \return FAR's return code for GetFilesW
     */
    int GetFiles(PluginPanelItem *panelItem, const int itemsNumber, const wchar_t **destPath, const bool deleteSource, const int opMode);

    /**
     * Put files
     * \param sourcePath source path
     * \param panelItem pointer to panel items
     * \param itemsNumber items counter
     * \param deleteSource delete source files flag (move mode)
     * \param opMode operation mode
     * \return FAR's return code for PutFilesW
     */
    int PutFiles(const wchar_t *sourcePath, PluginPanelItem *panelItem, const int itemsNumber, const bool deleteSource, const int opMode);

    /**
     * Delete files
     * \param panelItem pointer to panel items
     * \param itemsNumber items counter
     * \param opMode operation mode
     * \return FAR's return code for DeleteFilesW
     */
    int DeleteFiles(PluginPanelItem *panelItem, int itemsNumber, const int opMode);

    /**
     * Get panel title
     * \return panel title
     */
    const wchar_t *GetTitle() const
    {
        return m_Title.c_str();
    };

private:
    /**
     * Update panel's title
     */
    void UpdateTitle();

    /**
     * Check for session manager panel active
     * \return true if session manager panel active
     */
    inline bool IsSessionManager() const
    {
        return m_ProtoClient && (typeid(CSessionManager) == typeid(*m_ProtoClient));
    }

    /**
     * Show dialog with error information
     * \param errCode system error code
     * \param title error title
     * \param info additional info
     */
    void ShowErrorDialog(const DWORD errCode, const wstring &title, const wchar_t *info = NULL) const;

    void ResetAbortTask();
private:
    IProtocol *m_ProtoClient; ///< Client's protocol implementation
    wstring m_LastDirName; ///< Last created/copyed directory name (used as buffer)
    wstring m_Title; ///< Panel title

    bool m_ExitToSessionMgr;  ///< True to exit from top folder to session manager, false to close plugin

    HANDLE m_AbortTask; ///< Abort task event
};
