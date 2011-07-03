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

#include "Protocol.h"
#include "Session.h"


/**
 * Base (common) client's protocol implementation with session support
 */
template<class T> class CProtocolBase : public IProtocol
{
public:
	CProtocolBase<T>(const CSession* session) : _Session(*static_cast<const T*>(session)), _ProgressPercent(-1) {}

	//From IProtocol
	virtual int GetProgress()
	{
		return _ProgressPercent;
	}

	//From IProtocol
	virtual bool ChangeDirectory(const wchar_t* name, wstring& errorInfo)
	{
		assert(name && *name);

		const bool moveUp = (wcscmp(L"..", name) == 0);
		const bool topDirectory = (_CurrentDirectory.compare(L"/") == 0);

		assert(!moveUp || !topDirectory);	//Must be handled in CPanel (exit from session)

		wstring newPath = _CurrentDirectory;
		if (moveUp) {
			const size_t lastSlash = newPath.rfind(L'/');
			assert(lastSlash != string::npos);
			if (lastSlash != string::npos && lastSlash != 0)
				newPath.erase(lastSlash);
			else
				newPath = L'/';
		}
		else {
			if (!topDirectory)
				newPath += L'/';
			newPath += name;
		}

		//Check path existing
		bool dirExist = false;
		if (!CheckExisting(newPath.c_str(), ItemDirectory, dirExist, errorInfo) || !dirExist)
			return false;
		_CurrentDirectory = newPath;
		return true;
	}

	//From IProtocol
	virtual const wchar_t* GetCurrentDirectory()
	{
		return _CurrentDirectory.c_str();
	}

	//From IProtocol
	virtual void FreeList(PluginPanelItem* items, int itemsNum)
	{
		if (itemsNum) {
			assert(items);
			for (int i = 0; i < itemsNum; ++i) {
				delete[] items[i].FindData.lpwszFileName;
				delete[] reinterpret_cast<void*>(items[i].UserData);
			}
			delete[] items;
		}
	}

	//From IProtocol
	wstring GetURL(const bool includeUser = false)
	{
		unsigned short port = 0;
		wstring schemeName;
		wstring hostName;
		wstring path;
		ParseURL(_Session.GetURL(), &schemeName, &hostName, &port, &path, NULL, NULL, NULL);

		wstring ret;
		ret += schemeName;
		ret += L"://";

		if (includeUser) {
			const wstring userName = _Session.GetUserName();
			const wstring password = _Session.GetPassword();
			if (!userName.empty() || !password.empty()) {
				ret += userName;
				ret += L':';
				ret += password;
				ret += L'@';
			}
		}

		ret += hostName;
		if (port) {
			wchar_t portTxt[8];
			_itow_s(port, portTxt, 10);
			ret += L':';
			ret += portTxt;
		}

		return ret;
	}

protected:
	/**
	 * Format error description
	 * \param errCode system error code
	 * \param info additional info
	 * \return error description
	 */
	wstring FormatErrorDescription(const DWORD errCode, const wchar_t* info = NULL) const
	{
		assert(errCode || info);

		wstring errDescr;
		if (info)
			errDescr = info;
		if (errCode) {
			if (!errDescr.empty())
				errDescr += L'\n';
			errDescr += GetSystemErrorMessage(errCode);
		}
		return errDescr;
	}

protected:
	T		_Session;			///< Session description
	int		_ProgressPercent;	///< Progress percent value
	wstring	_CurrentDirectory;	///< Current directory name
};
