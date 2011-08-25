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

#include "FarPlugin.h"


/**
 * Registry operation wrapper (Far plugins settings)
 */
class CFarSettings
{
public:
    CFarSettings() : _RegKey(NULL)  {}
    ~CFarSettings()
    {
        Close();
    }

    /**
     * Open registry key
     * \param regPath registry path
     * \param readOnly read only flag
     * \return false if error
     */
    inline bool Open(const wchar_t *regPath, const bool readOnly)
    {
        Close();
        std::wstring keyName = CFarPlugin::GetPSI()->RootKey;
        keyName += L'\\';
        keyName += regPath;
        const DWORD status = readOnly ?
                             RegOpenKeyEx(HKEY_CURRENT_USER, keyName.c_str(), 0, KEY_READ, &_RegKey) :
                             RegCreateKeyEx(HKEY_CURRENT_USER, keyName.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, DELETE | KEY_READ | KEY_WRITE, NULL, &_RegKey, NULL);
        return (status == ERROR_SUCCESS);
    }

    /**
     * Close registry key
     */
    inline void Close()
    {
        if (_RegKey != NULL)
        {
            RegCloseKey(_RegKey);
            _RegKey = NULL;
        }
    }

    /**
     * Get registry numeric value
     * \param name key name
     * \param val key value
     * \return false if error
     */
    inline bool GetNumber(const wchar_t *name, DWORD &val) const
    {
        assert(_RegKey != NULL);

        DWORD dataLen = sizeof(DWORD);
        return (RegQueryValueEx(_RegKey, name, NULL, NULL, reinterpret_cast<LPBYTE>(&val), &dataLen) == ERROR_SUCCESS);
    }

    /**
     * Get registry string value
     * \param name key name
     * \param val key value
     * \return false if error
     */
    inline bool GetString(const wchar_t *name, std::wstring &val) const
    {
        assert(_RegKey != NULL);

        //Get value length
        DWORD dataLen = 0;
        if (RegQueryValueEx(_RegKey, name, NULL, NULL, NULL, &dataLen) != ERROR_SUCCESS)
        {
            return false;
        }

        if (dataLen == 0)
        {
            val.clear();
            return true;
        }

        //Get value
        val.resize(dataLen / sizeof(wchar_t));
        const bool status = RegQueryValueEx(_RegKey, name, NULL, NULL, reinterpret_cast<LPBYTE>(&val[0]), &dataLen) == ERROR_SUCCESS;
        if (!status)
        {
            val.clear();
        }
        else
        {
            while (!val.empty() && val[val.length() - 1] == 0)
            {
                val.erase(val.length() - 1);
            }
        }
        return status;
    }

    /**
     * Set registry string value
     * \param name key name
     * \param val key value
     */
    inline void SetNumber(const wchar_t *name, const DWORD val) const
    {
        RegSetValueEx(_RegKey, name, 0, REG_DWORD, reinterpret_cast<const BYTE *>(&val), sizeof(val));
    }

    /**
     * Set registry string value
     * \param name key name
     * \param val key value
     */
    inline void SetString(const wchar_t *name, const wchar_t *val) const
    {
        RegSetValueEx(_RegKey, name, 0, REG_SZ, reinterpret_cast<const BYTE *>(val), lstrlen(val) * sizeof(wchar_t));
    }

    /**
     * Delete registry value
     * \param name value name
     * \return false if error
     */
    inline bool DeleteValue(const wchar_t *name) const
    {
        return (RegDeleteValue(_RegKey, name) == ERROR_SUCCESS);
    }

    /**
     * Enum keys
     * \return key name list
     */
    inline std::vector<std::wstring> EnumKeys() const
    {
        DWORD subKeys = 0;
        RegQueryInfoKey(_RegKey, NULL, NULL, NULL, &subKeys, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

        std::vector<std::wstring> keys;
        for (DWORD i = 0; i < subKeys; ++i)
        {
            std::wstring name(256, 0);
            DWORD nameSz = static_cast<DWORD>(name.size());
            if (RegEnumKeyEx(_RegKey, i, &name[0], &nameSz, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
            {
                keys.push_back(name.substr(0, nameSz));
            }
        }

        return keys;
    }

    /**
     * Delete registry tree
     * \param regPath registry path
     * \return false if error
     */
    static bool DeleteTree(const wchar_t *regPath)
    {
        //Vista and higher: return (RegDeleteTree(_RegKey, NULL) == ERROR_SUCCESS);

        bool result = false;

        //Delete tree using Shell API
        std::wstring keyName = CFarPlugin::GetPSI()->RootKey;
        keyName += L'\\';
        keyName += regPath;

        typedef DWORD (__stdcall* SHDeleteKeyFx) (HKEY, LPCWSTR);
        SHDeleteKeyFx SHDeleteKeyFunction = NULL;
        HINSTANCE shlwapidll = ::LoadLibrary(L"shlwapi.dll");
        if (shlwapidll)
        {
            SHDeleteKeyFunction = (SHDeleteKeyFx)GetProcAddress(shlwapidll, "SHDeleteKeyW");
            if (SHDeleteKeyFunction)
            {
                result = (SHDeleteKeyFunction(HKEY_CURRENT_USER, keyName.c_str()) == ERROR_SUCCESS);
            }
            FreeLibrary(shlwapidll);
        }

        return result;
    }

private:
    HKEY _RegKey;   ///< Registry key
};
