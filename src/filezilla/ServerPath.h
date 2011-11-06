// FileZilla - a Windows ftp client

// Copyright (C) 2002-2004 - Tim Kosse <tim.kosse@gmx.de>

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

// ServerPath.h: Schnittstelle für die Klasse CServerPath.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SERVERPATH_H__DF62E6B4_541A_4425_BA73_22B09A12DFE1__INCLUDED_)
#define AFX_SERVERPATH_H__DF62E6B4_541A_4425_BA73_22B09A12DFE1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CServerPath  
{
public:
	BOOL AddSubdir(std::wstring subdir);
	BOOL AddSubdirs(std::wstring subdirs);
	std::wstring GetSubdirsOf(const CServerPath &path) const;
	std::wstring GetSafePath() const;
	BOOL SetSafePath(std::wstring path);
	const BOOL IsEmpty() const;
	BOOL IsParentOf(const CServerPath &path, BOOL bCompareNoCase = FALSE) const;
	BOOL IsSubdirOf(const CServerPath &path, BOOL bCompareNoCase = FALSE) const;
	CServerPath GetParent() const;
	BOOL HasParent() const;
	std::wstring GetLastSegment() const;
	CServerPath();
	CServerPath(int nServerType);
	CServerPath(std::wstring path);
	CServerPath(std::wstring path, int nServerType);
	CServerPath(std::wstring subdir, const CServerPath &parent); //If subdir is absolute, parent is ignored
	CServerPath(const CServerPath &path);

	virtual ~CServerPath();

	void SetServer(const t_server &server);
	BOOL SetPath(std::wstring &newpath, BOOL bIsFile);
	BOOL SetPath(std::wstring newpath);
	BOOL ChangePath(std::wstring &subdir, BOOL bIsFile = FALSE);
	const std::wstring GetPath() const;

	const bool MatchNoCase(const CServerPath &op) const;

	CServerPath& operator=(const CServerPath &op);

	const bool operator == (const CServerPath &op) const;
	const bool operator != (const CServerPath &op) const;

	std::wstring FormatFilename(std::wstring fn, bool omitPath = false) const;
	
protected:
	BOOL m_bEmpty;
	std::list<std::wstring> m_Segments;
	typedef std::list<std::wstring>::iterator tIter;
		typedef std::list<std::wstring>::const_iterator tConstIter;
	typedef std::list<std::wstring>::const_iterator tConstIter;
	std::wstring m_Prefix;
	int m_nServerType;
};

const BOOL operator == (const CServerPath &a, const std::wstring &b);

#endif // !defined(AFX_SERVERPATH_H__DF62E6B4_541A_4425_BA73_22B09A12DFE1__INCLUDED_)
