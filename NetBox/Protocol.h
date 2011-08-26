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

/**
 * Client's protocol interface
 */
class IProtocol
{
public:
    //! Item types
    enum ItemType
    {
        ItemDirectory,
        ItemFile,
    };

    /**
     * Connect to remote host
     * \param abortEvent abort task event
     * \param errorInfo error description
     * \return false if error
     */
    virtual bool Connect(HANDLE abortEvent, std::wstring &errorInfo) = 0;

    /**
     * Close connection
     */
    virtual void Close() = 0;

    /**
     * Check for file/directory existing
     * \param path checked path
     * \param type checked item type
     * \param isExist existing flag
     * \param errorInfo error description
     * \return false if error
     */
    virtual bool CheckExisting(const wchar_t *path, const ItemType type, bool &isExist, std::wstring &errorInfo) = 0;

    /**
     * Change directory directory
     * \param name directory name (relative by current directory)
     * \param errorInfo error description
     * \return false if error
     */
    virtual bool ChangeDirectory(const wchar_t *name, std::wstring &errorInfo) = 0;

    /**
     * Get current directory
     * \return current directory path
     */
    virtual const wchar_t *GetCurrentDirectory() = 0;

    /**
     * Make directory
     * \param path new directory path
     * \param errorInfo error description
     * \return false if error
     */
    virtual bool MakeDirectory(const wchar_t *path, std::wstring &errorInfo) = 0;

    /**
     * Get current directory listing
     * \param items items array
     * \param itemsNum items quantity
     * \param errorInfo error description
     * \return false if error
     */
    virtual bool GetList(PluginPanelItem **items, int *itemsNum, std::wstring &errorInfo) = 0;

    /**
     * Free directory listing
     * \param items items array
     * \param itemsNum items quantity
     */
    virtual void FreeList(PluginPanelItem *items, int itemsNum) = 0;

    /**
     * Copy remote to local file
     * \param param remote file path
     * \param param local file path
     * \param param remote file size
     * \param errorInfo error description
     * \return false if error
     */
    virtual bool GetFile(const wchar_t *remotePath, const wchar_t *localPath, const unsigned __int64 fileSize, std::wstring &errorInfo) = 0;

    /**
     * Copy local to remote file
     * \param param remote file path
     * \param param local file path
     * \param param remote file size
     * \param errorInfo error description
     * \return false if error
     */
    virtual bool PutFile(const wchar_t *remotePath, const wchar_t *localPath, const unsigned __int64 fileSize, std::wstring &errorInfo) = 0;

    /**
     * Rename remote file/directory
     * \param param task parameters
     * \return false if error
     */
    /**
     * Rename remote file/directory
     * \param srcPath source remote path
     * \param dstPath destination remote file path
     * \param type item type
     * \param errorInfo error description
     * \return false if error
     */
    virtual bool Rename(const wchar_t *srcPath, const wchar_t *dstPath, const ItemType type, std::wstring &errorInfo) = 0;

    /**
     * Delete remote file/directory
     * \param path remote path
     * \param type item type
     * \param errorInfo error description
     * \return false if error
     */
    virtual bool Delete(const wchar_t *path, const ItemType type, std::wstring &errorInfo) = 0;

    /**
     * Get current URL (without path)
     * \param includeUser include user's name/password info
     * \return current URL
     */
    virtual std::wstring GetURL(const bool includeUser = false) = 0;

    /**
     * Get progress in percent of currently processing operation
     * \return progress in percent or -1 if percent unknown
     */
    virtual int GetProgress() = 0;

    virtual bool TryToResolveConnectionProblem() = 0;
    virtual bool Aborted() const = 0;
};

typedef std::auto_ptr<IProtocol> PProtocol;
