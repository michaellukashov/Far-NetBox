// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#ifndef __AFXINET_H_
#define __AFXINET_H_

#pragma once

#ifndef __AFX_H__
	#include <afx.h>
#endif

#ifndef _WININET_
#include <wininet.h>
#endif

#ifdef _AFXDLL
#pragma comment(lib, "wininet.lib")
#endif

#ifdef _AFX_MINREBUILD
#pragma component(minrebuild, off)
#endif

/////////////////////////////////////////////////////////////////////////////
// classes that are declared in this file

class CInternetSession; // from CObject

class CGopherLocator;   // from CObject

class CInternetFile;    // from CStdioFile (FILETXT.CPP)
	class CHttpFile;
	class CGopherFile;

class CInternetConnection;
	class CFtpConnection;
	class CGopherConnection;
	class CHttpConnection;

class CFtpFileFind;     // from CFileFind (FILEFIND.CPP)
class CGopherFileFind;

class CInternetException;

/////////////////////////////////////////////////////////////////////////////

#undef AFX_DATA
#define AFX_DATA AFX_CORE_DATA

/////////////////////////////////////////////////////////////////////////////
// Global Functions

BOOL AFXAPI AfxParseURL(LPCTSTR pstrURL, DWORD& dwServiceType,
	CString& strServer, CString& strObject, INTERNET_PORT& nPort);
BOOL AFXAPI AfxParseURLEx(LPCTSTR pstrURL, DWORD& dwServiceType,
	CString& strServer, CString& strObject, INTERNET_PORT& nPort,
	CString& strUsername, CString& strPassword, DWORD dwFlags = 0);

DWORD AFXAPI AfxGetInternetHandleType(HINTERNET hQuery);

// see CInternetException at the bottom of this file

void AFXAPI AfxThrowInternetException(DWORD_PTR dwContext, DWORD dwError = 0);

// these are defined by WININET.H

#define AFX_INET_SERVICE_FTP        INTERNET_SERVICE_FTP
#define AFX_INET_SERVICE_HTTP       INTERNET_SERVICE_HTTP
#define AFX_INET_SERVICE_GOPHER     INTERNET_SERVICE_GOPHER

// these are types that MFC parsing functions understand

#define AFX_INET_SERVICE_UNK        0x1000
#define AFX_INET_SERVICE_FILE       (AFX_INET_SERVICE_UNK+1)
#define AFX_INET_SERVICE_MAILTO     (AFX_INET_SERVICE_UNK+2)
#define AFX_INET_SERVICE_MID        (AFX_INET_SERVICE_UNK+3)
#define AFX_INET_SERVICE_CID        (AFX_INET_SERVICE_UNK+4)
#define AFX_INET_SERVICE_NEWS       (AFX_INET_SERVICE_UNK+5)
#define AFX_INET_SERVICE_NNTP       (AFX_INET_SERVICE_UNK+6)
#define AFX_INET_SERVICE_PROSPERO   (AFX_INET_SERVICE_UNK+7)
#define AFX_INET_SERVICE_TELNET     (AFX_INET_SERVICE_UNK+8)
#define AFX_INET_SERVICE_WAIS       (AFX_INET_SERVICE_UNK+9)
#define AFX_INET_SERVICE_AFS        (AFX_INET_SERVICE_UNK+10)
#define AFX_INET_SERVICE_HTTPS      (AFX_INET_SERVICE_UNK+11)

/////////////////////////////////////////////////////////////////////////////
// Inline function declarations

#ifdef _AFX_ENABLE_INLINES
#define _AFXINET_INLINE AFX_INLINE
#include <afxinet.inl>
#endif

#undef AFX_DATA
#define AFX_DATA

#ifdef _AFX_MINREBUILD
#pragma component(minrebuild, on)
#endif

#ifndef _AFX_DISABLE_DEPRECATED
#pragma deprecated( CGopherLocator )
#pragma deprecated( CGopherFile )
#pragma deprecated( CGopherConnection )
#pragma deprecated( CGopherFileFind )
#endif

#endif // __AFXINET_H__
