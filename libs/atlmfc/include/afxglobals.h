// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.
//
#pragma once

#if (NTDDI_VERSION >= NTDDI_WIN7)
// #include <shobjidl.h>
#endif

#ifdef _AFX_PACKING
#pragma pack(push, _AFX_PACKING)
#endif

#ifdef _AFX_MINREBUILD
#pragma component(minrebuild, off)
#endif

/////////////////////////////////////////////////////////////////////////////
// Auxiliary System/Screen metrics

enum AFX_DOCK_TYPE
{
	DT_UNDEFINED = 0,    // inherit from application
	DT_IMMEDIATE = 1,    // control bar torn off immediately and follows the mouse
	DT_STANDARD  = 2,    // user drags a frame
	DT_SMART     = 0x80  // smart docking style
};

// autohide sliding modes
static const UINT AFX_AHSM_MOVE    = 1;
static const UINT AFX_AHSM_STRETCH = 2;

#define AFX_AUTOHIDE_LEFT   0x0001
#define AFX_AUTOHIDE_RIGHT  0x0002
#define AFX_AUTOHIDE_TOP    0x0004
#define AFX_AUTOHIDE_BOTTOM 0x0008

typedef HANDLE AFX_HPAINTBUFFER;  // handle to a buffered paint context

typedef HRESULT (__stdcall * DRAWTHEMEPARENTBACKGROUND)(HWND hWnd, HDC hdc,const RECT *pRec);

typedef enum _AFX_BP_BUFFERFORMAT
{
	AFX_BPBF_COMPATIBLEBITMAP,    // Compatible bitmap
	AFX_BPBF_DIB,                 // Device-independent bitmap
	AFX_BPBF_TOPDOWNDIB,          // Top-down device-independent bitmap
	AFX_BPBF_TOPDOWNMONODIB       // Top-down monochrome device-independent bitmap
} AFX_BP_BUFFERFORMAT;

typedef struct _AFX_BP_PAINTPARAMS
{
	DWORD                       cbSize;
	DWORD                       dwFlags; // BPPF_ flags
	const RECT *                prcExclude;
	const BLENDFUNCTION *       pBlendFunction;
} AFX_BP_PAINTPARAMS;

typedef HRESULT (__stdcall * BUFFEREDPAINTINIT)(VOID);
typedef HRESULT (__stdcall * BUFFEREDPAINTUNINIT)(VOID);
typedef AFX_HPAINTBUFFER (__stdcall * BEGINBUFFEREDPAINT)(HDC hdcTarget, const RECT* rcTarget, AFX_BP_BUFFERFORMAT dwFormat, AFX_BP_PAINTPARAMS *pPaintParams, HDC *phdc);
typedef HRESULT (__stdcall * ENDBUFFEREDPAINT)(AFX_HPAINTBUFFER hBufferedPaint, BOOL fUpdateTarget);

typedef struct _AFX_MARGINS {
	int cxLeftWidth;
	int cxRightWidth;
	int cyTopHeight;
	int cyBottomHeight;
} AFX_MARGINS;

typedef int (WINAPI *AFX_DTT_CALLBACK_PROC)(HDC hdc, LPWSTR pszText, int cchText, LPRECT prc, UINT dwFlags, LPARAM lParam);

typedef struct _AFX_DTTOPTS {
	DWORD dwSize;
	DWORD dwFlags;
	COLORREF crText;
	COLORREF crBorder;
	COLORREF crShadow;
	int iTextShadowType;
	POINT ptShadowOffset;
	int nBorderSize;
	int iFontPropId;
	int iColorPropId;
	int iStateId;
	BOOL fApplyOverlay;
	int iGlowSize;
	AFX_DTT_CALLBACK_PROC pfnDrawTextCallback;
	LPARAM lParam;
} AFX_DTTOPTS;

//typedef HRESULT (__stdcall * DRAWTHEMETEXTEX)(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, LPCWSTR pszText, int iCharCount, DWORD dwFlags, LPRECT pRect, const AFX_DTTOPTS *pOptions);

class CMFCToolBarImages;

struct AFX_GLOBAL_DATA
{
	BOOL m_bUseSystemFont;	// Use system font for menu/toolbar/ribbons
	BOOL m_bInSettingChange;

	// solid brushes with convenient gray colors and system colors
	HBRUSH hbrBtnHilite, hbrBtnShadow;
	HBRUSH hbrWindow;

	// color values of system colors used for CToolBar
	// COLORREF clrBtnFace, clrBtnShadow, clrBtnHilite;
	// COLORREF clrBtnText, clrWindowFrame;
	// COLORREF clrBtnDkShadow, clrBtnLight;
	// COLORREF clrGrayedText;
	// COLORREF clrHilite;
	// COLORREF clrTextHilite;
	// COLORREF clrHotLinkNormalText;
	// COLORREF clrHotLinkHoveredText;
	// COLORREF clrHotLinkVisitedText;

	// COLORREF clrBarWindow;
	// COLORREF clrBarFace;
	// COLORREF clrBarShadow, clrBarHilite;
	// COLORREF clrBarDkShadow, clrBarLight;
	// COLORREF clrBarText;

	// COLORREF clrWindow;
	// COLORREF clrWindowText;

	// COLORREF clrCaptionText;
	// COLORREF clrMenuText;
	// COLORREF clrActiveCaption;
	// COLORREF clrInactiveCaption;
	// COLORREF clrInactiveCaptionText;
	///<summary>
	/// Specifies gradient color of active caption. Generally used for docking panes. </summary>
	// COLORREF clrActiveCaptionGradient;
	///<summary>
	/// Specifies gradient color of inactive active caption. Generally used for docking panes. </summary>
	// COLORREF clrInactiveCaptionGradient;

	// COLORREF clrActiveBorder;
	// COLORREF clrInactiveBorder;

	// Library cursors:
	// HCURSOR m_hcurStretch;
	// HCURSOR m_hcurStretchVert;
	// HCURSOR m_hcurHand;
	// HCURSOR m_hcurSizeAll;
	// HCURSOR m_hcurMoveTab;
	// HCURSOR m_hcurNoMoveTab;

	// HCURSOR GetHandCursor();

	// HICON m_hiconTool;
	// HICON m_hiconLink;
	// HICON m_hiconColors;

	// Shell icon sizes:
	CSize m_sizeSmallIcon;

	CRect m_rectVirtual;

	BOOL  bIsWindowsVista;
	///<summary>
	/// Indicates whether the application is being executed under Windows 7 OS or higher</summary>
	BOOL  bIsWindows7;
	BOOL  bDisableAero;
	BOOL  bIsRemoteSession;

	BOOL  m_bIsBlackHighContrast;
	BOOL  m_bIsWhiteHighContrast;
	BOOL  m_bUseBuiltIn32BitIcons;
	BOOL  m_bMenuAnimation;
	BOOL  m_bMenuFadeEffect;
	BOOL  m_bIsRTL;
	BOOL  m_bEnableAccessibility;

	BOOL m_bUnderlineKeyboardShortcuts;
	BOOL m_bSysUnderlineKeyboardShortcuts;

	BOOL m_bRefreshAutohideBars;

	int   m_nBitsPerPixel;
	int   m_nDragFrameThicknessFloat;
	int   m_nDragFrameThicknessDock;
	int   m_nAutoHideToolBarSpacing;
	int   m_nAutoHideToolBarMargin;
	int   m_nCoveredMainWndClientAreaPercent;
	int   m_nMaxToolTipWidth;
	int   m_nShellAutohideBars;

// Implementation
	AFX_GLOBAL_DATA();
	~AFX_GLOBAL_DATA();

	void UpdateSysColors();

	BOOL SetMenuFont(LPLOGFONT lpLogFont, BOOL bHorz);

	int GetTextHeight(BOOL bHorz = TRUE)
	{
		return bHorz ? m_nTextHeightHorz : m_nTextHeightVert;
	}

	int GetTextWidth(BOOL bHorz = TRUE)
	{
		return bHorz ? m_nTextWidthHorz : m_nTextWidthVert;
	}

	void CleanUp();

	// COLORREF GetColor(int nColor);

	BOOL SetLayeredAttrib(HWND hwnd, COLORREF crKey, BYTE bAlpha, DWORD dwFlags);
	BOOL IsWindowsLayerSupportAvailable() const
	{
		return TRUE;
	}

	BOOL Is32BitIcons() const
	{
		return m_bUseBuiltIn32BitIcons && m_nBitsPerPixel >= 16 && !m_bIsBlackHighContrast && !m_bIsWhiteHighContrast;
	}

	BOOL IsHighContrastMode() const
	{
		return m_bIsWhiteHighContrast || m_bIsBlackHighContrast;
	}

	BOOL IsAccessibilitySupport() const
	{
		return m_bEnableAccessibility;
	}

	/// <summary>
	/// Determines positions of Shell auto hide bars.</summary>
	/// <returns> An integer value with encoded flags that specify positions of auto hide bars.
	/// It may combine the following values: AFX_AUTOHIDE_BOTTOM, AFX_AUTOHIDE_TOP, AFX_AUTOHIDE_LEFT,
	/// AFX_AUTOHIDE_RIGHT.</returns>
	/*int GetShellAutohideBars()
	{
		if (m_bRefreshAutohideBars)
		{
			m_bRefreshAutohideBars = FALSE;

			APPBARDATA abd;
			ZeroMemory(&abd, sizeof(APPBARDATA));
			abd.cbSize = sizeof(APPBARDATA);

			abd.uEdge = ABE_BOTTOM;
			if (SHAppBarMessage(ABM_GETAUTOHIDEBAR, &abd))
			{
				m_nShellAutohideBars |= AFX_AUTOHIDE_BOTTOM;
			}

			abd.uEdge = ABE_TOP;
			if (SHAppBarMessage(ABM_GETAUTOHIDEBAR, &abd))
			{
				m_nShellAutohideBars |= AFX_AUTOHIDE_TOP;
			}

			abd.uEdge = ABE_LEFT;
			if (SHAppBarMessage(ABM_GETAUTOHIDEBAR, &abd))
			{
				m_nShellAutohideBars |= AFX_AUTOHIDE_LEFT;
			}

			abd.uEdge = ABE_RIGHT;
			if (SHAppBarMessage(ABM_GETAUTOHIDEBAR, &abd))
			{
				m_nShellAutohideBars |= AFX_AUTOHIDE_RIGHT;
			}
		}

		return m_nShellAutohideBars;
	}*/

	// void EnableAccessibilitySupport(BOOL bEnable = TRUE);

	CString RegisterWindowClass(LPCTSTR lpszClassNamePrefix);

	BOOL Resume();
	BOOL GetNonClientMetrics (NONCLIENTMETRICS& info);

protected:

	void UpdateTextMetrics();
	// HBITMAP CreateDitherBitmap(HDC hDC);

	int m_nTextHeightHorz;
	int m_nTextHeightVert;
	int m_nTextWidthHorz;
	int m_nTextWidthVert;

};

AFX_IMPORT_DATA extern AFX_GLOBAL_DATA afxGlobalData;

#define AFX_IMAGE_MARGIN 4

// MFC Control bar compatibility
#define AFX_CX_BORDER   1
#define AFX_CY_BORDER   1

#define AFX_CX_GRIPPER  3
#define AFX_CY_GRIPPER  3
#define AFX_CX_BORDER_GRIPPER 2
#define AFX_CY_BORDER_GRIPPER 2

#ifdef _AFX_MINREBUILD
#pragma component(minrebuild, on)
#endif

#ifdef _AFX_PACKING
#pragma pack(pop)
#endif
