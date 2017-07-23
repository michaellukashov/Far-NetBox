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
#include "comdef.h"
#include "afxglobals.h"

/////////////////////////////////////////////////////////////////////////////
// Cached system metrics, etc
AFX_GLOBAL_DATA afxGlobalData;

// Initialization code
AFX_GLOBAL_DATA::AFX_GLOBAL_DATA()
{
	// Detect the kind of OS:
	OSVERSIONINFO osvi;
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

	::GetVersionEx(&osvi);

//	bIsRemoteSession = GetSystemMetrics(SM_REMOTESESSION);

	bIsWindowsVista = (osvi.dwMajorVersion >= 6);
	bIsWindows7 = ((osvi.dwMajorVersion == 6) && (osvi.dwMinorVersion >= 1)) || (osvi.dwMajorVersion > 6);
	bDisableAero = FALSE;

	// Cached system values(updated in CWnd::OnSysColorChange)
	hbrBtnShadow = NULL;
	hbrBtnHilite = NULL;
	hbrWindow = NULL;

	// m_hcurStretch = NULL;
	// m_hcurStretchVert = NULL;
	// m_hcurHand = NULL;
	// m_hcurSizeAll = NULL;
	// m_hiconTool = NULL;
	// m_hiconLink = NULL;
	// m_hiconColors = NULL;
	// m_hcurMoveTab = NULL;
	// m_hcurNoMoveTab = NULL;

	m_bUseSystemFont = FALSE;
	m_bInSettingChange = FALSE;

	m_bIsRTL = FALSE;

	m_nDragFrameThicknessFloat = 4;  // pixels
	m_nDragFrameThicknessDock = 3;   // pixels

	m_nAutoHideToolBarSpacing = 14; // pixels
	m_nAutoHideToolBarMargin  = 4;  // pixels

	m_nCoveredMainWndClientAreaPercent = 50; // percents

	m_nMaxToolTipWidth = -1;
	m_bIsBlackHighContrast = FALSE;
	m_bIsWhiteHighContrast = FALSE;

	m_bUseBuiltIn32BitIcons = TRUE;

	// EnableAccessibilitySupport();
}

AFX_GLOBAL_DATA::~AFX_GLOBAL_DATA()
{
	CleanUp();
}

void AFX_GLOBAL_DATA::UpdateTextMetrics()
{
	/*CWindowDC dc(NULL);

	TEXTMETRIC tm;
	dc.GetTextMetrics(&tm);

	int nExtra = tm.tmHeight < 15 ? 2 : 5;

	m_nTextHeightHorz = tm.tmHeight + nExtra;
	m_nTextWidthHorz = tm.tmMaxCharWidth + nExtra;

	dc.GetTextMetrics(&tm);

	nExtra = tm.tmHeight < 15 ? 2 : 5;

	m_nTextHeightVert = tm.tmHeight + nExtra;
	m_nTextWidthVert = tm.tmMaxCharWidth + nExtra;
*/
}

void AFX_GLOBAL_DATA::CleanUp()
{
	// if (brLight.GetSafeHandle())
	// {
		// brLight.DeleteObject();
	// }

	m_bEnableAccessibility = FALSE;
}

void ControlBarCleanUp()
{
	afxGlobalData.CleanUp();
}

/*COLORREF AFX_GLOBAL_DATA::GetColor(int nColor)
{
	switch(nColor)
	{
		case COLOR_BTNFACE:             return clrBtnFace;
		case COLOR_BTNSHADOW:           return clrBtnShadow;
		case COLOR_3DDKSHADOW:          return clrBtnDkShadow;
		case COLOR_3DLIGHT:             return clrBtnLight;
		case COLOR_BTNHIGHLIGHT:        return clrBtnHilite;
		case COLOR_BTNTEXT:             return clrBtnText;
		case COLOR_GRAYTEXT:            return clrGrayedText;
		case COLOR_WINDOWFRAME:         return clrWindowFrame;

		case COLOR_HIGHLIGHT:           return clrHilite;
		case COLOR_HIGHLIGHTTEXT:       return clrTextHilite;

		case COLOR_WINDOW:              return clrWindow;
		case COLOR_WINDOWTEXT:          return clrWindowText;

		case COLOR_CAPTIONTEXT:         return clrCaptionText;
		case COLOR_MENUTEXT:            return clrMenuText;

		case COLOR_ACTIVECAPTION:       return clrActiveCaption;
		case COLOR_INACTIVECAPTION:     return clrInactiveCaption;

		case COLOR_ACTIVEBORDER:        return clrActiveBorder;
		case COLOR_INACTIVEBORDER:      return clrInactiveBorder;

		case COLOR_INACTIVECAPTIONTEXT: return clrInactiveCaptionText;
	}

	return ::GetSysColor(nColor);
}*/

BOOL AFX_GLOBAL_DATA::SetLayeredAttrib(HWND hwnd, COLORREF crKey, BYTE bAlpha, DWORD dwFlags)
{
	return(::SetLayeredWindowAttributes(hwnd, crKey, bAlpha, dwFlags));
}

// void AFX_GLOBAL_DATA::EnableAccessibilitySupport(BOOL bEnable/* = TRUE*/)
// {
	// m_bEnableAccessibility = bEnable;
// }

CString AFX_GLOBAL_DATA::RegisterWindowClass(LPCTSTR lpszClassNamePrefix)
{
	ENSURE(lpszClassNamePrefix != NULL);

	// Register a new window class:
	HINSTANCE hInst = AfxGetInstanceHandle();
//	UINT uiClassStyle = CS_DBLCLKS;
//	HCURSOR hCursor = ::LoadCursor(NULL, IDC_ARROW);
//	HBRUSH hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);

	CString strClassName;
//	strClassName.Format(_T("%s:%x:%x:%x:%x"), lpszClassNamePrefix, (UINT_PTR)hInst, uiClassStyle, (UINT_PTR)hCursor, (UINT_PTR)hbrBackground);

	// See if the class already exists:
	WNDCLASS wndcls;
	if (::GetClassInfo(hInst, strClassName, &wndcls))
	{
		// Already registered, assert everything is good:
//		ASSERT(wndcls.style == uiClassStyle);
	}
	else
	{
		// Otherwise we need to register a new class:
//		wndcls.style = uiClassStyle;
		wndcls.lpfnWndProc = ::DefWindowProc;
		wndcls.cbClsExtra = wndcls.cbWndExtra = 0;
		wndcls.hInstance = hInst;
		wndcls.hIcon = NULL;
//		wndcls.hCursor = hCursor;
//		wndcls.hbrBackground = hbrBackground;
		wndcls.lpszMenuName = NULL;
		wndcls.lpszClassName = strClassName;

		if (!AfxRegisterClass(&wndcls))
		{
			// AfxThrowResourceException();
		}
	}

	return strClassName;
}

BOOL AFX_GLOBAL_DATA::Resume()
{
	if (m_bEnableAccessibility)
	{
		// EnableAccessibilitySupport();
	}

	return TRUE;
}

BOOL AFX_GLOBAL_DATA::GetNonClientMetrics (NONCLIENTMETRICS& info)
{
	struct AFX_OLDNONCLIENTMETRICS
	{
		UINT    cbSize;
		int     iBorderWidth;
		int     iScrollWidth;
		int     iScrollHeight;
		int     iCaptionWidth;
		int     iCaptionHeight;
		LOGFONT lfCaptionFont;
		int     iSmCaptionWidth;
		int     iSmCaptionHeight;
		LOGFONT lfSmCaptionFont;
		int     iMenuWidth;
		int     iMenuHeight;
		LOGFONT lfMenuFont;
		LOGFONT lfStatusFont;
		LOGFONT lfMessageFont;
	};

	info.cbSize = 0;

	return 0;
}


BOOL AFXAPI AfxIsExtendedFrameClass(CWnd* pWnd)
{
	ENSURE( pWnd );
	return FALSE;
}


BOOL AFXAPI AfxIsMFCToolBar(CWnd* pWnd)
{
	ENSURE( pWnd );
	return FALSE;
}

