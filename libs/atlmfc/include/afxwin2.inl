// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

// Inlines for AFXWIN.H (part 2)

#pragma once

#ifdef _AFXWIN_INLINE

// CWnd
_AFXWIN_INLINE CWnd::operator HWND() const
	{ return this == NULL ? NULL : m_hWnd; }
_AFXWIN_INLINE BOOL CWnd::operator==(const CWnd& wnd) const
	{ return ((HWND) wnd) == m_hWnd; }
_AFXWIN_INLINE BOOL CWnd::operator!=(const CWnd& wnd) const
	{ return ((HWND) wnd) != m_hWnd; }
_AFXWIN_INLINE HWND CWnd::GetSafeHwnd() const
	{ return this == NULL ? NULL : m_hWnd; }
_AFXWIN_INLINE CWnd* CWnd::GetOwner() const
	{ return m_hWndOwner != NULL ? CWnd::FromHandle(m_hWndOwner) : GetParent(); }
_AFXWIN_INLINE void CWnd::SetOwner(CWnd* pOwnerWnd)
	{ m_hWndOwner = pOwnerWnd != NULL ? pOwnerWnd->m_hWnd : NULL; }

_AFXWIN_INLINE LRESULT CWnd::_AFX_FUNCNAME(SendMessage)(UINT message, WPARAM wParam, LPARAM lParam) const
	{ ASSERT(::IsWindow(m_hWnd)); return ::SendMessage(m_hWnd, message, wParam, lParam); }
#pragma push_macro("SendMessage")
#undef SendMessage
_AFXWIN_INLINE LRESULT CWnd::SendMessage(UINT message, WPARAM wParam, LPARAM lParam) const
	{ return _AFX_FUNCNAME(SendMessage)(message, wParam, lParam); }
#pragma pop_macro("SendMessage")
_AFXWIN_INLINE BOOL CWnd::PostMessage(UINT message, WPARAM wParam, LPARAM lParam)
	{ ASSERT(::IsWindow(m_hWnd)); return ::PostMessage(m_hWnd, message, wParam, lParam); }
_AFXWIN_INLINE BOOL CWnd::DragDetect(POINT pt) const
	{ ASSERT(::IsWindow(m_hWnd)); return ::DragDetect(m_hWnd, pt); }
//#ifdef _AFX_NO_OCC_SUPPORT
//_AFXWIN_INLINE void CWnd::SetWindowText(LPCTSTR lpszString)
//	{ ASSERT(::IsWindow(m_hWnd)); ::SetWindowText(m_hWnd, lpszString); }
//_AFXWIN_INLINE int CWnd::GetWindowText(_Out_z_cap_post_count_(nMaxCount, return + 1) LPTSTR lpszString, _In_ int nMaxCount) const
//	{ ASSERT(::IsWindow(m_hWnd)); return ::GetWindowText(m_hWnd, lpszString, nMaxCount); }
//_AFXWIN_INLINE int CWnd::GetWindowTextLength() const
//	{ ASSERT(::IsWindow(m_hWnd)); return ::GetWindowTextLength(m_hWnd); }
//#endif //_AFX_NO_OCC_SUPPORT
//_AFXWIN_INLINE void CWnd::DragAcceptFiles(BOOL bAccept)
//	{ ASSERT(::IsWindow(m_hWnd)); ::DragAcceptFiles(m_hWnd, bAccept); }
//#ifdef _AFX_NO_OCC_SUPPORT
//_AFXWIN_INLINE int CWnd::GetDlgCtrlID() const
//	{ ASSERT(::IsWindow(m_hWnd)); return ::GetDlgCtrlID(m_hWnd); }
//_AFXWIN_INLINE int CWnd::SetDlgCtrlID(int nID)
//	{ ASSERT(::IsWindow(m_hWnd)); return (int)::SetWindowLong(m_hWnd, GWL_ID, nID); }
//#endif //_AFX_NO_OCC_SUPPORT
_AFXWIN_INLINE CWnd* CWnd::EnsureTopLevelParent() const
{
    CWnd *pWnd=GetTopLevelParent();
    ENSURE_VALID(pWnd);
    return pWnd;
}
_AFXWIN_INLINE void CWnd::MapWindowPoints(CWnd* pwndTo, LPPOINT lpPoint, UINT nCount) const
	{ ASSERT(::IsWindow(m_hWnd)); ::MapWindowPoints(m_hWnd, pwndTo->GetSafeHwnd(), lpPoint, nCount); }
_AFXWIN_INLINE void CWnd::MapWindowPoints(CWnd* pwndTo, LPRECT lpRect) const
	{ ASSERT(::IsWindow(m_hWnd)); ::MapWindowPoints(m_hWnd, pwndTo->GetSafeHwnd(), (LPPOINT)lpRect, 2); }
_AFXWIN_INLINE void CWnd::ClientToScreen(LPPOINT lpPoint) const
	{ ASSERT(::IsWindow(m_hWnd)); ::ClientToScreen(m_hWnd, lpPoint); }
_AFXWIN_INLINE void CWnd::ScreenToClient(LPPOINT lpPoint) const
	{ ASSERT(::IsWindow(m_hWnd)); ::ScreenToClient(m_hWnd, lpPoint); }
// _AFXWIN_INLINE CDC* CWnd::GetDC()
	// { ASSERT(::IsWindow(m_hWnd)); return CDC::FromHandle(::GetDC(m_hWnd)); }
// _AFXWIN_INLINE CDC* CWnd::GetWindowDC()
	// { ASSERT(::IsWindow(m_hWnd)); return CDC::FromHandle(::GetWindowDC(m_hWnd)); }
//_AFXWIN_INLINE int CWnd::ReleaseDC(CDC* pDC)
//	{ ASSERT(::IsWindow(m_hWnd)); return ::ReleaseDC(m_hWnd, pDC->m_hDC); }
_AFXWIN_INLINE void CWnd::UpdateWindow()
	{ ASSERT(::IsWindow(m_hWnd)); ::UpdateWindow(m_hWnd); }
_AFXWIN_INLINE void CWnd::SetRedraw(BOOL bRedraw)
	{ ASSERT(::IsWindow(m_hWnd)); ::SendMessage(m_hWnd, WM_SETREDRAW, bRedraw, 0); }
_AFXWIN_INLINE BOOL CWnd::GetUpdateRect(LPRECT lpRect, BOOL bErase)
	{ ASSERT(::IsWindow(m_hWnd)); return ::GetUpdateRect(m_hWnd, lpRect, bErase); }
_AFXWIN_INLINE void CWnd::Invalidate(BOOL bErase)
	{ ASSERT(::IsWindow(m_hWnd)); ::InvalidateRect(m_hWnd, NULL, bErase); }
#ifdef _AFX_NO_OCC_SUPPORT
_AFXWIN_INLINE BOOL CWnd::ShowWindow(int nCmdShow)
	{ ASSERT(::IsWindow(m_hWnd)); return ::ShowWindow(m_hWnd, nCmdShow); }
#endif //_AFX_NO_OCC_SUPPORT
_AFXWIN_INLINE BOOL CWnd::IsWindowVisible() const
	{ ASSERT(::IsWindow(m_hWnd)); return ::IsWindowVisible(m_hWnd); }
_AFXWIN_INLINE void CWnd::ShowOwnedPopups(BOOL bShow)
	{ ASSERT(::IsWindow(m_hWnd)); ::ShowOwnedPopups(m_hWnd, bShow); }
_AFXWIN_INLINE void CWnd::SendMessageToDescendants(
	UINT message, WPARAM wParam, LPARAM lParam, BOOL bDeep, BOOL bOnlyPerm)
	{ ASSERT(::IsWindow(m_hWnd)); CWnd::SendMessageToDescendants(m_hWnd, message, wParam, lParam, bDeep,
		bOnlyPerm); }
_AFXWIN_INLINE CWnd* CWnd::GetDescendantWindow(int nID, BOOL bOnlyPerm) const
	{ ASSERT(::IsWindow(m_hWnd)); return CWnd::GetDescendantWindow(m_hWnd, nID, bOnlyPerm); }

//#ifdef _AFX_NO_OCC_SUPPORT
//_AFXWIN_INLINE BOOL CWnd::IsDialogMessage(LPMSG lpMsg)
//	{ ASSERT(::IsWindow(m_hWnd)); return ::IsDialogMessage(m_hWnd, lpMsg); }
//#endif

//_AFXWIN_INLINE BOOL CWnd::LockWindowUpdate()
//	{ ASSERT(::IsWindow(m_hWnd)); return ::LockWindowUpdate(m_hWnd); }
//_AFXWIN_INLINE void CWnd::UnlockWindowUpdate()
//	{ ASSERT(::IsWindow(m_hWnd)); ::LockWindowUpdate(NULL); }
//_AFXWIN_INLINE BOOL CWnd::EnableScrollBar(int nSBFlags, UINT nArrowFlags)
//	{ ASSERT(::IsWindow(m_hWnd)); return (BOOL)::EnableScrollBar(m_hWnd, nSBFlags, nArrowFlags); }
//_AFXWIN_INLINE BOOL CWnd::DrawAnimatedRects(int idAni, CONST RECT *lprcFrom, CONST RECT *lprcTo)
//	{ ASSERT(::IsWindow(m_hWnd)); return (BOOL)::DrawAnimatedRects(m_hWnd, idAni, lprcFrom, lprcTo); }
//_AFXWIN_INLINE BOOL CWnd::DrawCaption(CDC* pDC, LPCRECT lprc, UINT uFlags)
//	{ ASSERT(::IsWindow(m_hWnd)); return (BOOL)::DrawCaption(m_hWnd, pDC->GetSafeHdc(), lprc, uFlags); }

_AFXWIN_INLINE UINT_PTR CWnd::SetTimer(UINT_PTR nIDEvent, UINT nElapse,
		void (CALLBACK* lpfnTimer)(HWND, UINT, UINT_PTR, DWORD))
	{ ASSERT(::IsWindow(m_hWnd)); return ::SetTimer(m_hWnd, nIDEvent, nElapse,
		lpfnTimer); }
_AFXWIN_INLINE BOOL CWnd::KillTimer(UINT_PTR nIDEvent)
	{ ASSERT(::IsWindow(m_hWnd)); return ::KillTimer(m_hWnd, nIDEvent); }
#ifdef _AFX_NO_OCC_SUPPORT
_AFXWIN_INLINE BOOL CWnd::IsWindowEnabled() const
	{ ASSERT(::IsWindow(m_hWnd)); return ::IsWindowEnabled(m_hWnd); }
_AFXWIN_INLINE BOOL CWnd::EnableWindow(BOOL bEnable)
	{ ASSERT(::IsWindow(m_hWnd)); return ::EnableWindow(m_hWnd, bEnable); }
#endif //_AFX_NO_OCC_SUPPORT
_AFXWIN_INLINE CWnd* PASCAL CWnd::GetActiveWindow()
	{ return CWnd::FromHandle(::GetActiveWindow()); }
_AFXWIN_INLINE CWnd* CWnd::SetActiveWindow()
	{ ASSERT(::IsWindow(m_hWnd)); return CWnd::FromHandle(::SetActiveWindow(m_hWnd)); }
_AFXWIN_INLINE CWnd* PASCAL CWnd::GetCapture()
	{ return CWnd::FromHandle(::GetCapture()); }
_AFXWIN_INLINE CWnd* CWnd::SetCapture()
	{ ASSERT(::IsWindow(m_hWnd)); return CWnd::FromHandle(::SetCapture(m_hWnd)); }
_AFXWIN_INLINE CWnd* PASCAL CWnd::GetFocus()
	{ return CWnd::FromHandle(::GetFocus()); }

_AFXWIN_INLINE CWnd* CWnd::GetWindow(UINT nCmd) const
	{ ASSERT(::IsWindow(m_hWnd)); return CWnd::FromHandle(::GetWindow(m_hWnd, nCmd)); }
_AFXWIN_INLINE CWnd* CWnd::GetLastActivePopup() const
	{ ASSERT(::IsWindow(m_hWnd)); return CWnd::FromHandle(::GetLastActivePopup(m_hWnd)); }
_AFXWIN_INLINE BOOL CWnd::IsChild(const CWnd* pWnd) const
	{ ASSERT(::IsWindow(m_hWnd)); return ::IsChild(m_hWnd, pWnd->GetSafeHwnd()); }
_AFXWIN_INLINE CWnd* CWnd::GetParent() const
	{ ASSERT(::IsWindow(m_hWnd)); return CWnd::FromHandle(::GetParent(m_hWnd)); }
_AFXWIN_INLINE CWnd* CWnd::SetParent(CWnd* pWndNewParent)
	{ ASSERT(::IsWindow(m_hWnd)); return CWnd::FromHandle(::SetParent(m_hWnd,
			pWndNewParent->GetSafeHwnd())); }
_AFXWIN_INLINE CWnd* PASCAL CWnd::WindowFromPoint(POINT point)
	{ return CWnd::FromHandle(::WindowFromPoint(point)); }
//#pragma push_macro("MessageBox")
//#undef MessageBox
//_AFXWIN_INLINE int CWnd::MessageBox(LPCTSTR lpszText, LPCTSTR lpszCaption, UINT nType)
//	{ return _AFX_FUNCNAME(MessageBox)(lpszText, lpszCaption, nType); }
//#pragma pop_macro("MessageBox")
//_AFXWIN_INLINE BOOL CWnd::FlashWindow(BOOL bInvert)
//	{ ASSERT(::IsWindow(m_hWnd)); return ::FlashWindow(m_hWnd, bInvert); }
//_AFXWIN_INLINE BOOL CWnd::ChangeClipboardChain(HWND hWndNext)
//	{ ASSERT(::IsWindow(m_hWnd)); return ::ChangeClipboardChain(m_hWnd, hWndNext); }
//_AFXWIN_INLINE HWND CWnd::SetClipboardViewer()
//	{ ASSERT(::IsWindow(m_hWnd)); return ::SetClipboardViewer(m_hWnd); }
//_AFXWIN_INLINE BOOL CWnd::OpenClipboard()
//	{ ASSERT(::IsWindow(m_hWnd)); return ::OpenClipboard(m_hWnd); }
//_AFXWIN_INLINE CWnd* PASCAL CWnd::GetOpenClipboardWindow()
//	{ return CWnd::FromHandle(::GetOpenClipboardWindow()); }
//_AFXWIN_INLINE CWnd* PASCAL CWnd::GetClipboardOwner()
//	{ return CWnd::FromHandle(::GetClipboardOwner()); }
//_AFXWIN_INLINE CWnd* PASCAL CWnd::GetClipboardViewer()
//	{ return CWnd::FromHandle(::GetClipboardViewer()); }
_AFXWIN_INLINE void CWnd::CreateSolidCaret(int nWidth, int nHeight)
	{ ASSERT(::IsWindow(m_hWnd)); ::CreateCaret(m_hWnd, (HBITMAP)0, nWidth, nHeight); }
_AFXWIN_INLINE void CWnd::CreateGrayCaret(int nWidth, int nHeight)
	{ ASSERT(::IsWindow(m_hWnd)); ::CreateCaret(m_hWnd, (HBITMAP)1, nWidth, nHeight); }
_AFXWIN_INLINE void PASCAL CWnd::SetCaretPos(POINT point)
	{ ::SetCaretPos(point.x, point.y); }
_AFXWIN_INLINE void CWnd::HideCaret()
	{ ASSERT(::IsWindow(m_hWnd)); ::HideCaret(m_hWnd); }
_AFXWIN_INLINE void CWnd::ShowCaret()
	{ ASSERT(::IsWindow(m_hWnd)); ::ShowCaret(m_hWnd); }
_AFXWIN_INLINE BOOL CWnd::SetForegroundWindow()
	{ ASSERT(::IsWindow(m_hWnd)); return ::SetForegroundWindow(m_hWnd); }
_AFXWIN_INLINE CWnd* PASCAL CWnd::GetForegroundWindow()
	{ return CWnd::FromHandle(::GetForegroundWindow()); }

_AFXWIN_INLINE BOOL CWnd::SendNotifyMessage(UINT message, WPARAM wParam, LPARAM lParam)
	{ ASSERT(::IsWindow(m_hWnd)); return ::SendNotifyMessage(m_hWnd, message, wParam, lParam); }

// Win4
_AFXWIN_INLINE HICON CWnd::SetIcon(HICON hIcon, BOOL bBigIcon)
	{ ASSERT(::IsWindow(m_hWnd)); return (HICON)::SendMessage(m_hWnd, WM_SETICON, bBigIcon, (LPARAM)hIcon); }
_AFXWIN_INLINE HICON CWnd::GetIcon(BOOL bBigIcon) const
	{ ASSERT(::IsWindow(m_hWnd)); return (HICON)::SendMessage(m_hWnd, WM_GETICON, bBigIcon, 0); }
//_AFXWIN_INLINE void CWnd::Print(CDC* pDC, DWORD dwFlags) const
//	{ ASSERT(::IsWindow(m_hWnd)); ::SendMessage(m_hWnd, WM_PRINT, (WPARAM)pDC->GetSafeHdc(), dwFlags); }
//_AFXWIN_INLINE void CWnd::PrintClient(CDC* pDC, DWORD dwFlags) const
//	{ ASSERT(::IsWindow(m_hWnd)); ::SendMessage(m_hWnd, WM_PRINTCLIENT, (WPARAM)pDC->GetSafeHdc(), dwFlags); }
_AFXWIN_INLINE BOOL CWnd::SetWindowContextHelpId(DWORD dwContextHelpId)
	{ ASSERT(::IsWindow(m_hWnd)); return ::SetWindowContextHelpId(m_hWnd, dwContextHelpId); }
_AFXWIN_INLINE DWORD CWnd::GetWindowContextHelpId() const
	{ ASSERT(::IsWindow(m_hWnd)); return ::GetWindowContextHelpId(m_hWnd); }

// ActiveAccessibility
_AFXWIN_INLINE void CWnd::EnableActiveAccessibility()
	{ m_bEnableActiveAccessibility = true; }
_AFXWIN_INLINE void CWnd::NotifyWinEvent(DWORD event, LONG idObjectType, LONG idObject)
	{ ASSERT(::IsWindow(m_hWnd)); ::NotifyWinEvent(event, m_hWnd, idObjectType, idObject); }

// Default message map implementations
_AFXWIN_INLINE void CWnd::OnActivateApp(BOOL, DWORD)
	{ Default(); }
_AFXWIN_INLINE void CWnd::OnActivate(UINT, CWnd*, BOOL)
	{ Default(); }
_AFXWIN_INLINE void CWnd::OnCancelMode()
	{ Default(); }
_AFXWIN_INLINE void CWnd::OnChildActivate()
	{ Default(); }
_AFXWIN_INLINE void CWnd::OnClose()
	{ Default(); }
_AFXWIN_INLINE int CWnd::OnCopyData(CWnd*, COPYDATASTRUCT*)
	{ return (int)Default(); }
_AFXWIN_INLINE int CWnd::OnCreate(LPCREATESTRUCT)
	{ return (int)Default(); }
_AFXWIN_INLINE void CWnd::OnEnable(BOOL)
	{ Default(); }
_AFXWIN_INLINE void CWnd::OnEndSession(BOOL)
	{ Default(); }
//_AFXWIN_INLINE BOOL CWnd::OnEraseBkgnd(CDC*)
//	{ return (BOOL)Default(); }
//_AFXWIN_INLINE void CWnd::OnGetMinMaxInfo(MINMAXINFO*)
//	{ Default(); }
//_AFXWIN_INLINE void CWnd::OnIconEraseBkgnd(CDC*)
//	{ Default(); }
_AFXWIN_INLINE void CWnd::OnKillFocus(CWnd*)
	{ Default(); }
_AFXWIN_INLINE void CWnd::OnMove(int, int)
	{ Default(); }
_AFXWIN_INLINE void CWnd::OnSyncPaint()
	{ Default(); }
_AFXWIN_INLINE HCURSOR CWnd::OnQueryDragIcon()
	{ return (HCURSOR)Default(); }
_AFXWIN_INLINE BOOL CWnd::OnQueryEndSession()
	{ return (BOOL)Default(); }
_AFXWIN_INLINE BOOL CWnd::OnQueryNewPalette()
	{ return (BOOL)Default(); }
_AFXWIN_INLINE BOOL CWnd::OnQueryOpen()
	{ return (BOOL)Default(); }
_AFXWIN_INLINE BOOL CWnd::OnSetCursor(CWnd*, UINT, UINT)
	{ return (BOOL)Default(); }
_AFXWIN_INLINE void CWnd::OnShowWindow(BOOL, UINT)
	{ Default(); }
_AFXWIN_INLINE void CWnd::OnSize(UINT, int, int)
	{ Default(); }
_AFXWIN_INLINE void CWnd::OnTCard(UINT, DWORD)
	{ Default(); }
_AFXWIN_INLINE void CWnd::OnWindowPosChanging(WINDOWPOS*)
	{ Default(); }
_AFXWIN_INLINE void CWnd::OnWindowPosChanged(WINDOWPOS*)
	{ Default(); }
_AFXWIN_INLINE void CWnd::OnSessionChange(UINT, UINT)
	{ Default(); }
//_AFXWIN_INLINE void CWnd::OnDropFiles(HDROP)
//	{ Default(); }
//_AFXWIN_INLINE void CWnd::OnPaletteIsChanging(CWnd*)
//	{ Default(); }
_AFXWIN_INLINE BOOL CWnd::OnNcActivate(BOOL)
	{ return (BOOL)Default(); }
_AFXWIN_INLINE void CWnd::OnNcCalcSize(BOOL, NCCALCSIZE_PARAMS*)
	{ Default(); }
_AFXWIN_INLINE BOOL CWnd::OnNcCreate(LPCREATESTRUCT)
	{ return (BOOL)Default(); }
_AFXWIN_INLINE void CWnd::OnSysChar(UINT, UINT, UINT)
	{ Default(); }
_AFXWIN_INLINE void CWnd::OnSysCommand(UINT, LPARAM)
	{ Default(); }
_AFXWIN_INLINE void CWnd::OnSysDeadChar(UINT, UINT, UINT)
	{ Default(); }
_AFXWIN_INLINE void CWnd::OnSysKeyDown(UINT, UINT, UINT)
	{ Default(); }
_AFXWIN_INLINE void CWnd::OnSysKeyUp(UINT, UINT, UINT)
	{ Default(); }
_AFXWIN_INLINE BOOL CWnd::OnAppCommand(CWnd*, UINT, UINT, UINT)
	{ return (BOOL)Default(); }
#if(_WIN32_WINNT >= 0x0501)
_AFXWIN_INLINE void CWnd::OnRawInput(UINT, HRAWINPUT)
	{ Default(); }
#endif
_AFXWIN_INLINE void CWnd::OnCompacting(UINT)
	{ Default(); }
_AFXWIN_INLINE void CWnd::OnFontChange()
	{ Default(); }
_AFXWIN_INLINE void CWnd::OnPaletteChanged(CWnd*)
	{ Default(); }
_AFXWIN_INLINE void CWnd::OnSpoolerStatus(UINT, UINT)
	{ Default(); }
_AFXWIN_INLINE void CWnd::OnTimeChange()
	{ Default(); }
_AFXWIN_INLINE UINT CWnd::OnPowerBroadcast(UINT, UINT)
	{ return (UINT) Default(); }
_AFXWIN_INLINE void CWnd::OnUserChanged()
	{ Default(); }
_AFXWIN_INLINE void CWnd::OnInputLangChange(UINT, UINT)
	{ Default(); }
_AFXWIN_INLINE void CWnd::OnInputLangChangeRequest(UINT, UINT)
	{ Default(); }
_AFXWIN_INLINE void CWnd::OnInputDeviceChange(unsigned short, HANDLE)
	{ Default(); }
_AFXWIN_INLINE void CWnd::OnChar(UINT, UINT, UINT)
	{ Default(); }
_AFXWIN_INLINE void CWnd::OnDeadChar(UINT, UINT, UINT)
	{ Default(); }
_AFXWIN_INLINE void CWnd::OnUniChar(UINT, UINT, UINT)
	{ Default(); }
_AFXWIN_INLINE void CWnd::OnKeyDown(UINT, UINT, UINT)
	{ Default(); }
_AFXWIN_INLINE void CWnd::OnKeyUp(UINT, UINT, UINT)
	{ Default(); }
_AFXWIN_INLINE void CWnd::OnHotKey(UINT, UINT, UINT)
	{ Default(); }
_AFXWIN_INLINE void CWnd::OnAskCbFormatName(_In_ UINT nMaxCount, _Out_z_cap_(nMaxCount) LPTSTR pszName)
{
	(void)(nMaxCount);
	if(nMaxCount>0)
	{
		/* defwindow proc should do this for us, but to be safe, we'll do it here too */
		pszName[0]=_T('\0');
	}
	Default();
}
_AFXWIN_INLINE void CWnd::OnChangeCbChain(HWND, HWND)
	{ Default(); }
_AFXWIN_INLINE void CWnd::OnDestroyClipboard()
	{ Default(); }
_AFXWIN_INLINE void CWnd::OnDrawClipboard()
	{ Default(); }
_AFXWIN_INLINE void CWnd::OnPaintClipboard(CWnd*, HGLOBAL)
	{ Default(); }
_AFXWIN_INLINE void CWnd::OnRenderAllFormats()
	{ Default(); }
_AFXWIN_INLINE void CWnd::OnRenderFormat(UINT)
	{ Default(); }
_AFXWIN_INLINE void CWnd::OnSizeClipboard(CWnd*, HGLOBAL)
	{ Default(); }
_AFXWIN_INLINE void CWnd::OnVScrollClipboard(CWnd*, UINT, UINT)
	{ Default(); }
_AFXWIN_INLINE void CWnd::OnClipboardUpdate()
	{ Default(); }
_AFXWIN_INLINE UINT CWnd::OnGetDlgCode()
	{ return (UINT)Default(); }
_AFXWIN_INLINE void CWnd::OnMDIActivate(BOOL, CWnd*, CWnd*)
	{ Default(); }
_AFXWIN_INLINE UINT CWnd::OnNotifyFormat(CWnd*, UINT)
	{ return (UINT) Default(); }
// Win4 support
_AFXWIN_INLINE void CWnd::OnStyleChanged(int, LPSTYLESTRUCT)
	{ Default(); }
_AFXWIN_INLINE void CWnd::OnStyleChanging(int, LPSTYLESTRUCT)
	{ Default(); }
_AFXWIN_INLINE void CWnd::OnSizing(UINT, LPRECT)
	{ Default(); }
_AFXWIN_INLINE void CWnd::OnMoving(UINT, LPRECT)
	{ Default(); }
_AFXWIN_INLINE void CWnd::OnEnterSizeMove()
	{ Default(); }
_AFXWIN_INLINE void CWnd::OnExitSizeMove()
	{ Default(); }
_AFXWIN_INLINE void CWnd::OnCaptureChanged(CWnd*)
	{ Default(); }
_AFXWIN_INLINE BOOL CWnd::OnDeviceChange(UINT, DWORD_PTR)
	{ return (BOOL)Default(); }
_AFXWIN_INLINE void CWnd::OnWinIniChange(LPCTSTR)
	{ Default(); }
_AFXWIN_INLINE void CWnd::OnChangeUIState(UINT, UINT)
	{ Default(); }
_AFXWIN_INLINE void CWnd::OnUpdateUIState(UINT, UINT)
	{ Default(); }
_AFXWIN_INLINE UINT CWnd::OnQueryUIState()
	{ return (UINT)Default(); }

// Desktop Windows Manager support
_AFXWIN_INLINE void CWnd::OnCompositionChanged()
	{ Default(); }
_AFXWIN_INLINE void CWnd::OnNcRenderingChanged(BOOL)
	{ Default(); }
_AFXWIN_INLINE void CWnd::OnColorizationColorChanged(DWORD, BOOL)
	{ Default(); }
_AFXWIN_INLINE void CWnd::OnWindowMaximizedChange(BOOL)
	{ Default(); }

// CWnd dialog data support
_AFXWIN_INLINE void CWnd::DoDataExchange(CDataExchange*)
	{ } // default does nothing

// CWnd modality support
_AFXWIN_INLINE void CWnd::BeginModalState()
	{ ::EnableWindow(m_hWnd, FALSE); }
_AFXWIN_INLINE void CWnd::EndModalState()
	{ ::EnableWindow(m_hWnd, TRUE); }

// CWinThread
_AFXWIN_INLINE CWinThread::operator HANDLE() const
	{ return this == NULL ? NULL : m_hThread; }
_AFXWIN_INLINE BOOL CWinThread::SetThreadPriority(int nPriority)
	{ ASSERT(m_hThread != NULL); return ::SetThreadPriority(m_hThread, nPriority); }
_AFXWIN_INLINE int CWinThread::GetThreadPriority()
	{ ASSERT(m_hThread != NULL); return ::GetThreadPriority(m_hThread); }
_AFXWIN_INLINE DWORD CWinThread::ResumeThread()
	{ ASSERT(m_hThread != NULL); return ::ResumeThread(m_hThread); }
_AFXWIN_INLINE DWORD CWinThread::SuspendThread()
	{ ASSERT(m_hThread != NULL); return ::SuspendThread(m_hThread); }
_AFXWIN_INLINE BOOL CWinThread::PostThreadMessage(UINT message, WPARAM wParam, LPARAM lParam)
	{ ASSERT(m_hThread != NULL); return ::PostThreadMessage(m_nThreadID, message, wParam, lParam); }

/////////////////////////////////////////////////////////////////////////////

#endif //_AFXWIN_INLINE
