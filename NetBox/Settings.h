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

enum ProxyTypes
{
    PROXY_NONE = 0,
    PROXY_SOCKS4,
    PROXY_SOCKS5,
    PROXY_HTTP,
};

//! Plugin settings
class CSettings
{
public:
    CSettings();

    /**
     * Load settings
     */
    void Load();

    /**
     * Configure settings
     */
    void Configure();

    //Accessors
    bool AddToDiskMenu() const
    {
        return m_AddToDiskMenu;
    }
    bool AddToPanelMenu() const
    {
        return m_AddToPanelMenu;
    }
    const wchar_t *CmdPrefix() const
    {
        return m_CmdPrefix.c_str();
    }
    bool AltPrefix() const
    {
        return m_AltPrefix;
    }
    bool UseOwnKey() const
    {
        return m_UseOwnKey;
    }
    unsigned long Timeout() const
    {
        return m_Timeout;
    }
    const wchar_t *SessionPath() const
    {
        return m_SessionPath.c_str();
    }
    /**
     * Get session path
     * \return session path
     */
    wstring GetSessionPath() const;

    //
    // Proxy
    //
    int ProxyType() const
    {
        return m_proxySettings.proxyType;
    }
    wstring ProxyHost() const
    {
        return m_proxySettings.proxyHost;
    }
    unsigned long ProxyPort() const
    {
        return m_proxySettings.proxyPort;
    }
    wstring ProxyLogin() const
    {
        return m_proxySettings.proxyLogin;
    }
    wstring ProxyPassword() const
    {
        return m_proxySettings.proxyPassword;
    }

    //
    // Logging
    //
    bool EnableLogging() const
    {
        return m_EnableLogging;
    }
    int LoggingLevel() const
    {
        return m_LoggingLevel;
    }
    bool LogToFile() const
    {
        return m_LogToFile;
    }
    const wchar_t *LogFileName() const
    {
        return m_LogFileName.c_str();
    }

private:
    void AddMenuItem(vector<FarMenuItemEx> &items, DWORD flags, int titleId);

    void MainConfigure();
    void ProxyConfigure();
    void LoggingConfigure();
    void ShowAbout();

    void Save() const;

private:
    size_t m_SettingsMenuIdx;
    //Settings variables
    bool m_AddToDiskMenu; ///< Add plugin to disk menu flag
    bool m_AddToPanelMenu; ///< Add plugin to panel plugin menu flag
    wstring m_CmdPrefix; ///< Plugin command prefix
    bool m_AltPrefix; ///< Hande additional prefix flag (ftp, sftp etc)
    bool m_UseOwnKey; ///< Use own encryption key flag
    unsigned long m_Timeout; ///< Default timeout in seconds
    wstring m_SessionPath; ///< Session folder path
    bool m_EnableLogging; ///< Enable logging flag
    int m_LoggingLevel;
    bool m_LogToFile;
    wstring m_LogFileName;
    
    ProxySettings m_proxySettings;
};

extern CSettings m_Settings;
