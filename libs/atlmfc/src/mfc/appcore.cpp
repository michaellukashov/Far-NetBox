// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#include "stdafx.h"
#include <malloc.h>
#include "sal.h"

#include "afxglobals.h"

AFX_STATIC_DATA const TCHAR _afxFileSection[] = _T("Recent File List");
AFX_STATIC_DATA const TCHAR _afxFileEntry[] = _T("File%d");
AFX_STATIC_DATA const TCHAR _afxPreviewSection[] = _T("Settings");
AFX_STATIC_DATA const TCHAR _afxPreviewEntry[] = _T("PreviewPages");

/////////////////////////////////////////////////////////////////////////////
// globals (internal library use)

/////////////////////////////////////////////////////////////////////////////
// _AFX_WIN_STATE implementation


static HINSTANCE _AfxLoadLangDLL(LPCTSTR pszFormat, LPCTSTR pszPath, LCID lcid)
{
	TCHAR szLangDLL[_MAX_PATH+14];
	TCHAR szLangCode[4];
	HINSTANCE hInstance;

	if (lcid == LOCALE_SYSTEM_DEFAULT)
	{
		Checked::tcscpy_s(szLangCode, _countof(szLangCode), _T("LOC"));
	}
	else
	{
		int nResult;

		nResult = ::GetLocaleInfo(lcid, LOCALE_SABBREVLANGNAME, szLangCode, 4);
		if (nResult == 0)
			return NULL;
		ASSERT( nResult == 4 );
	}

	int ret;
	ATL_CRT_ERRORCHECK_SPRINTF(ret = _sntprintf_s(szLangDLL,_countof(szLangDLL),_countof(szLangDLL)-1,pszFormat,pszPath,szLangCode));
	if(ret == -1 || ret >= _countof(szLangDLL))
	{
		ASSERT(FALSE);
		return NULL;
	}

	hInstance = ::LoadLibrary(szLangDLL);

	return hInstance;
}

static BOOL CALLBACK _AfxEnumResLangProc(HMODULE /*hModule*/, LPCTSTR /*pszType*/,
	LPCTSTR /*pszName*/, WORD langid, LONG_PTR lParam)
{
	if(lParam == NULL)
		return FALSE;

	LANGID* plangid = reinterpret_cast< LANGID* >( lParam );
	*plangid = langid;

	return TRUE;
}

#if 0
class CActivationContext
{
protected :
	HANDLE m_hCtxt;
	ULONG_PTR m_uCookie;

public:
	CActivationContext(HANDLE hCtxt = INVALID_HANDLE_VALUE) : m_hCtxt( hCtxt ), m_uCookie( 0 )
	{
	};

	~CActivationContext()
	{
		Release();
	}

	bool Create( PCACTCTX pactctx )
	{
		ASSERT( pactctx != NULL );
		if ( pactctx == NULL )
		{
			return false;
		}

		ASSERT( m_hCtxt == INVALID_HANDLE_VALUE );
		if ( m_hCtxt != INVALID_HANDLE_VALUE )
		{
			return false;
		}

		return ( ( m_hCtxt = CreateActCtx( pactctx ) ) != INVALID_HANDLE_VALUE );
	}

	void Release()
	{
		if ( m_hCtxt != INVALID_HANDLE_VALUE )
		{
			Deactivate();
			ReleaseActCtx( m_hCtxt );
		}
	}

	bool Activate()
	{
		ASSERT( m_hCtxt != INVALID_HANDLE_VALUE );
		if ( m_hCtxt == INVALID_HANDLE_VALUE )
		{
			return false;
		}

		ASSERT( m_uCookie == 0 );
		if ( m_uCookie != 0 )
		{
			return false;
		}

		return ( ActivateActCtx( m_hCtxt, &m_uCookie) == TRUE );
	}

	bool Deactivate()
	{
		if ( m_uCookie != 0 )
		{
			ULONG_PTR uCookie = m_uCookie;
			m_uCookie = 0;
			return ( DeactivateActCtx(0, uCookie) == TRUE );
		}
		return true;
	}
};
#endif


// HINSTANCE of the module
extern "C" IMAGE_DOS_HEADER __ImageBase;

typedef BOOL (WINAPI *PFNGETPREFERREDUILANGS)(DWORD, PULONG, PZZWSTR, PULONG);
const int nMaxExpectedPreferredLangs = 20;

HINSTANCE AFXAPI AfxLoadLangResourceDLL(LPCTSTR pszFormat, LPCTSTR pszPath)
{
	// load language specific DLL
	LANGID langid = 0;
	int nPrimaryLang = 0;
	int nSubLang = 0;
	LCID lcid = 0;
	LCID alcidSearch[nMaxExpectedPreferredLangs + 5];
	int nLocales;

	nLocales = 0;

	// First, get the thread preferred UI languages (if supported)
	HMODULE hKernel = NULL; // AfxCtxLoadLibrary(_T("KERNEL32.DLL"));
	if (hKernel != NULL)
	{
		PFNGETPREFERREDUILANGS pfnGetThreadPreferredUILanguages = (PFNGETPREFERREDUILANGS)GetProcAddress(hKernel, "GetThreadPreferredUILanguages");
		if (pfnGetThreadPreferredUILanguages != NULL)
		{
			BOOL bGotPreferredLangs = FALSE;
			ULONG nLanguages = 0;
			WCHAR wszLanguages[(5 * nMaxExpectedPreferredLangs) + 1] = {0}; // each lang has four chars plus NULL, plus terminating NULL
			ULONG cchLanguagesBuffer = _countof(wszLanguages);
			// bGotPreferredLangs = pfnGetThreadPreferredUILanguages(MUI_LANGUAGE_ID | MUI_UI_FALLBACK, &nLanguages, wszLanguages, &cchLanguagesBuffer);
			if (bGotPreferredLangs)
			{
				WCHAR *pwz = wszLanguages;
				while ((*pwz != 0) && (nLocales < nMaxExpectedPreferredLangs))
				{
					ULONG ulLangID = wcstoul(pwz, NULL, 16);
					if ((ulLangID != 0) && (errno != ERANGE))
					{
						alcidSearch[nLocales] = (LCID)ulLangID;
						nLocales++;
					}
					pwz += wcslen(pwz) + 1;  // move to next ID in list
				}
			}
		}
	}

	// Next, try the user's UI language
	langid = GetUserDefaultUILanguage();
	nPrimaryLang = PRIMARYLANGID(langid);
	nSubLang = SUBLANGID(langid);

	lcid = MAKELCID(MAKELANGID(nPrimaryLang, nSubLang), SORT_DEFAULT);
	alcidSearch[nLocales] = ::ConvertDefaultLocale(lcid);
	nLocales++;

	lcid = MAKELCID(MAKELANGID(nPrimaryLang, SUBLANG_NEUTRAL), SORT_DEFAULT);
	alcidSearch[nLocales] = ::ConvertDefaultLocale(lcid);
	nLocales++;

	// Then, try the system's default UI language
	langid = GetSystemDefaultUILanguage();
	nPrimaryLang = PRIMARYLANGID(langid);
	nSubLang = SUBLANGID(langid);

	lcid = MAKELCID(MAKELANGID(nPrimaryLang, nSubLang), SORT_DEFAULT);
	alcidSearch[nLocales] = ::ConvertDefaultLocale(lcid);
	nLocales++;

	lcid = MAKELCID(MAKELANGID(nPrimaryLang, SUBLANG_NEUTRAL), SORT_DEFAULT);
	alcidSearch[nLocales] = ::ConvertDefaultLocale(lcid);
	nLocales++;

	alcidSearch[nLocales] = LOCALE_SYSTEM_DEFAULT;
	nLocales++;

	// get path for our module
	TCHAR rgchFullModulePath[MAX_PATH + 2];
	rgchFullModulePath[_countof(rgchFullModulePath) - 1] = 0;
	rgchFullModulePath[_countof(rgchFullModulePath) - 2] = 0;
	DWORD dw = GetModuleFileName(reinterpret_cast<HMODULE>(&__ImageBase), rgchFullModulePath, _countof(rgchFullModulePath)-1);
	if (dw == 0)
	{
		return NULL;
	}

	for(int iLocale = 0; iLocale < nLocales; iLocale++)
	{
		HINSTANCE hLangDLL;

		hLangDLL = _AfxLoadLangDLL(pszFormat, pszPath, alcidSearch[iLocale]);
		if(hLangDLL != NULL)
			return hLangDLL;
	}

	return NULL;
}

HINSTANCE AFXAPI AfxLoadLangResourceDLL(LPCTSTR pszFormat)
{
	TCHAR pszNewFormat[MAX_PATH + 2 + 1] = _T("%s"); // have space for %s and string terminator
	ENSURE(_tcslen(pszFormat) <= MAX_PATH);
	_tcscat_s (pszNewFormat, _countof(pszNewFormat), pszFormat);
	return AfxLoadLangResourceDLL(pszNewFormat, _T(""));
}

/////////////////////////////////////////////////////////////////////////////
// CCommandLineInfo implementation

#define RESTART_COMMAND_LINE_ARG "RestartByRestartManager"  // command-line argument used when restarting the application
#define RESTART_IDENTIFIER_LEN   36                         // length of restart identifier (GUID) used when restarting

CCommandLineInfo::CCommandLineInfo()
{
	m_bShowSplash = TRUE;
	m_bRunEmbedded = FALSE;
	m_bRunAutomated = FALSE;
	m_bRegisterPerUser = FALSE;
	m_nShellCommand = FileNew;
}

CCommandLineInfo::~CCommandLineInfo()
{
}

void CCommandLineInfo::ParseParam(const TCHAR* pszParam,BOOL bFlag,BOOL bLast)
{
	if (bFlag)
	{
		const CStringA strParam(pszParam);
		ParseParamFlag(strParam.GetString());
	}
	else
		ParseParamNotFlag(pszParam);

	ParseLast(bLast);
}

#ifdef UNICODE
void CCommandLineInfo::ParseParam(const char* pszParam, BOOL bFlag, BOOL bLast)
{
	if (bFlag)
		ParseParamFlag(pszParam);
	else
		ParseParamNotFlag(pszParam);

	ParseLast(bLast);
}
#endif // UNICODE

void CCommandLineInfo::ParseParamFlag(const char* pszParam)
{
	// OLE command switches are case insensitive, while
	// shell command switches are case sensitive

	if (lstrcmpA(pszParam, "pt") == 0)
		m_nShellCommand = FilePrintTo;
	else if (lstrcmpA(pszParam, "p") == 0)
		m_nShellCommand = FilePrint;
	else if (::AfxInvariantStrICmp(pszParam, "Register") == 0 ||
		::AfxInvariantStrICmp(pszParam, "Regserver") == 0)
		m_nShellCommand = AppRegister;
	else if (::AfxInvariantStrICmp(pszParam, "RegisterPerUser") == 0 ||
		::AfxInvariantStrICmp(pszParam, "RegserverPerUser") == 0)
	{
		m_nShellCommand = AppRegister;
		m_bRegisterPerUser = TRUE;
	}
	else if (::AfxInvariantStrICmp(pszParam, "Unregister") == 0 ||
		::AfxInvariantStrICmp(pszParam, "Unregserver") == 0)
		m_nShellCommand = AppUnregister;
	else if (::AfxInvariantStrICmp(pszParam, "UnregisterPerUser") == 0 ||
		::AfxInvariantStrICmp(pszParam, "UnregserverPerUser") == 0)
	{
		m_nShellCommand = AppUnregister;
		m_bRegisterPerUser = TRUE;
	}
	else if (_strnicmp(pszParam, RESTART_COMMAND_LINE_ARG, _countof(RESTART_COMMAND_LINE_ARG) - 1) == 0)
	{
		CString strParam = pszParam;
		if (strParam.GetLength() == _countof(RESTART_COMMAND_LINE_ARG) + RESTART_IDENTIFIER_LEN)
		{
			m_nShellCommand = RestartByRestartManager;
			m_strRestartIdentifier = strParam.Right(RESTART_IDENTIFIER_LEN);
		}
	}
	else if (lstrcmpA(pszParam, "ddenoshow") == 0)
	{
		// AfxOleSetUserCtrl(FALSE);
		m_nShellCommand = FileDDENoShow;
	}
	else if (lstrcmpA(pszParam, "dde") == 0)
	{
		// AfxOleSetUserCtrl(FALSE);
		m_nShellCommand = FileDDE;
	}
	else if (::AfxInvariantStrICmp(pszParam, "Embedding") == 0)
	{
		// AfxOleSetUserCtrl(FALSE);
		m_bRunEmbedded = TRUE;
		m_bShowSplash = FALSE;
	}
	else if (::AfxInvariantStrICmp(pszParam, "Automation") == 0)
	{
		// AfxOleSetUserCtrl(FALSE);
		m_bRunAutomated = TRUE;
		m_bShowSplash = FALSE;
	}
}

void CCommandLineInfo::ParseParamNotFlag(const TCHAR* pszParam)
{
	if (m_strFileName.IsEmpty())
		m_strFileName = pszParam;
	else if (m_nShellCommand == FilePrintTo && m_strPrinterName.IsEmpty())
		m_strPrinterName = pszParam;
	else if (m_nShellCommand == FilePrintTo && m_strDriverName.IsEmpty())
		m_strDriverName = pszParam;
	else if (m_nShellCommand == FilePrintTo && m_strPortName.IsEmpty())
		m_strPortName = pszParam;
}

#ifdef UNICODE
void CCommandLineInfo::ParseParamNotFlag(const char* pszParam)
{
	if (m_strFileName.IsEmpty())
		m_strFileName = pszParam;
	else if (m_nShellCommand == FilePrintTo && m_strPrinterName.IsEmpty())
		m_strPrinterName = pszParam;
	else if (m_nShellCommand == FilePrintTo && m_strDriverName.IsEmpty())
		m_strDriverName = pszParam;
	else if (m_nShellCommand == FilePrintTo && m_strPortName.IsEmpty())
		m_strPortName = pszParam;
}
#endif

void CCommandLineInfo::ParseLast(BOOL bLast)
{
	if (bLast)
	{
		if (m_nShellCommand == FileNew && !m_strFileName.IsEmpty())
			m_nShellCommand = FileOpen;
		m_bShowSplash = !m_bRunEmbedded && !m_bRunAutomated;
	}
}

/////////////////////////////////////////////////////////////////////////////
// App termination

void AFXAPI AfxPostQuitMessage(int nExitCode)
{
	// cleanup OLE libraries
	CWinThread* pThread = AfxGetThread();
	if (pThread != NULL && pThread->m_lpfnOleTermOrFreeLib != NULL)
		(*pThread->m_lpfnOleTermOrFreeLib)(TRUE, TRUE);

	::PostQuitMessage(nExitCode);
}

/////////////////////////////////////////////////////////////////////////////
// Restart Manager support

// Restart Manager is only supported on Vista+, so we dynamically load the exports we need
typedef HRESULT (WINAPI *PFNREGISTERAPPLICATIONRESTART)(PCWSTR, DWORD);
typedef HRESULT (WINAPI *PFNREGISTERAPPLICATIONRECOVERYCALLBACK)(APPLICATION_RECOVERY_CALLBACK, PVOID, DWORD, DWORD);
typedef HRESULT (WINAPI *PFNAPPLICATIONRECOVERYINPROGRESS)(PBOOL);
typedef VOID    (WINAPI *PFNAPPLICATIONRECOVERYFINISHED)(BOOL);

/////////////////////////////////////////////////////////////////////////////
// Special exception handling

/////////////////////////////////////////////////////////////////////////////
// CWinApp idle processing

/////////////////////////////////////////////////////////////////////////////
// CWinApp idle processing

#pragma warning(disable: 4074)
#pragma init_seg(lib)

#if !defined(__MINGW32__)
PROCESS_LOCAL(_AFX_WIN_STATE, _afxWinState)
#else
CProcessLocal<_AFX_WIN_STATE> _afxWinState;
#endif

/////////////////////////////////////////////////////////////////////////////
///////
///////
