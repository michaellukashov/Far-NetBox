// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

// Inlines for AFXOLE.H

// more than one _INLINE type; can't use #pragma once

/////////////////////////////////////////////////////////////////////////////
// General OLE inlines (CDocItem, COleDocument)

#ifdef _AFXOLE_INLINE

#endif //_AFXOLE_INLINE

/////////////////////////////////////////////////////////////////////////////
// OLE Moniker inlines

#ifdef _AFXOLEMONIKER_INLINE

// CMonikerFile
_AFXOLEMONIKER_INLINE CMonikerFile::CMonikerFile() { }
_AFXOLEMONIKER_INLINE IMoniker* CMonikerFile::GetMoniker() const
	{ ASSERT_VALID(this); return m_Moniker; }

// CAsyncMonikerFile
_AFXOLEMONIKER_INLINE IBinding* CAsyncMonikerFile::GetBinding() const
	{ ASSERT_VALID(this); return m_Binding; }
_AFXOLEMONIKER_INLINE void CAsyncMonikerFile::SetBinding(IBinding* pBinding)
	{ ASSERT_VALID(this); m_Binding=pBinding; }
_AFXOLEMONIKER_INLINE void CAsyncMonikerFile::SetFormatEtc(FORMATETC* pFormatEtc)
	{ ASSERT_VALID(this); m_pFormatEtc=pFormatEtc; }
_AFXOLEMONIKER_INLINE FORMATETC* CAsyncMonikerFile::GetFormatEtc() const
	{ ASSERT_VALID(this); return m_pFormatEtc; }

#endif //_AFXOLEMONIKER_INLINE

/////////////////////////////////////////////////////////////////////////////
// OLE automation inlines

#ifdef _AFXDISP_INLINE

// COleException
_AFXDISP_INLINE COleException::COleException()
	{ m_sc = S_OK; }
_AFXDISP_INLINE COleException::~COleException()
	{ }

// CCmdTarget
_AFXDISP_INLINE DWORD CCmdTarget::InternalAddRef()
	{ ASSERT(GetInterfaceMap() != NULL); return InterlockedIncrement(&m_dwRef); }

// CObjectFactory
_AFXDISP_INLINE BOOL COleObjectFactory::IsRegistered() const
	{ ASSERT_VALID(this); return m_dwRegister != 0; }
_AFXDISP_INLINE REFCLSID COleObjectFactory::GetClassID() const
	{ ASSERT_VALID(this); return m_clsid; }

// COleDispatchDriver
_AFXDISP_INLINE COleDispatchDriver::~COleDispatchDriver()
	{ ReleaseDispatch(); }
_AFXDISP_INLINE COleDispatchDriver::operator LPDISPATCH()
	{ return m_lpDispatch; }

// COleVariant
_AFXDISP_INLINE COleVariant::COleVariant()
	{ AfxVariantInit(this); }
_AFXDISP_INLINE COleVariant::~COleVariant()
	{ VERIFY(::VariantClear(this) == NOERROR); }
_AFXDISP_INLINE void COleVariant::Clear()
	{ VERIFY(::VariantClear(this) == NOERROR); }
_AFXDISP_INLINE COleVariant::COleVariant(LPCTSTR lpszSrc)
	{ vt = VT_EMPTY; *this = lpszSrc; }
_AFXDISP_INLINE COleVariant::COleVariant(CString& strSrc)
	{ vt = VT_EMPTY; *this = strSrc; }
_AFXDISP_INLINE COleVariant::COleVariant(BYTE nSrc)
	{ vt = VT_UI1; bVal = nSrc; }
_AFXDISP_INLINE COleVariant::COleVariant(const COleCurrency& curSrc)
	{ vt = VT_CY; cyVal = curSrc.m_cur; }
#if (_WIN32_WINNT >= 0x0501) || defined(_ATL_SUPPORT_VT_I8)
_AFXDISP_INLINE COleVariant::COleVariant(LONGLONG nSrc)
	{ vt = VT_I8; llVal = nSrc; }
_AFXDISP_INLINE COleVariant::COleVariant(ULONGLONG nSrc)
	{ vt = VT_UI8; ullVal = nSrc; }
#endif
_AFXDISP_INLINE COleVariant::COleVariant(float fltSrc)
	{ vt = VT_R4; fltVal = fltSrc; }
_AFXDISP_INLINE COleVariant::COleVariant(double dblSrc)
	{ vt = VT_R8; dblVal = dblSrc; }
_AFXDISP_INLINE COleVariant::COleVariant(const COleDateTime& dateSrc)
	{ vt = VT_DATE; date = dateSrc; }
_AFXDISP_INLINE COleVariant::COleVariant(const CByteArray& arrSrc)
	{ vt = VT_EMPTY; *this = arrSrc; }
_AFXDISP_INLINE BOOL COleVariant::operator==(LPCVARIANT pSrc) const
	{ return *this == *pSrc; }
_AFXDISP_INLINE COleVariant::operator LPVARIANT()
	{ return this; }
_AFXDISP_INLINE COleVariant::operator LPCVARIANT() const
	{ return this; }

// COleCurrency
_AFXDISP_INLINE COleCurrency::COleCurrency()
	{ m_cur.Hi = 0; m_cur.Lo = 0; SetStatus(valid); }
_AFXDISP_INLINE COleCurrency::COleCurrency(CURRENCY cySrc)
	{ m_cur = cySrc; SetStatus(valid); }
_AFXDISP_INLINE COleCurrency::COleCurrency(const COleCurrency& curSrc)
	{ m_cur = curSrc.m_cur; m_status = curSrc.m_status; }
_AFXDISP_INLINE COleCurrency::COleCurrency(const VARIANT& varSrc)
	{ *this = varSrc; }
_AFXDISP_INLINE COleCurrency::CurrencyStatus COleCurrency::GetStatus() const
	{ return m_status; }
_AFXDISP_INLINE void COleCurrency::SetStatus(CurrencyStatus status)
	{ m_status = status; }
_AFXDISP_INLINE const COleCurrency& COleCurrency::operator+=(const COleCurrency& cur)
	{ *this = *this + cur; return *this; }
_AFXDISP_INLINE const COleCurrency& COleCurrency::operator-=(const COleCurrency& cur)
	{ *this = *this - cur; return *this; }
_AFXDISP_INLINE const COleCurrency& COleCurrency::operator*=(long nOperand)
	{ *this = *this * nOperand; return *this; }
_AFXDISP_INLINE const COleCurrency& COleCurrency::operator/=(long nOperand)
	{ *this = *this / nOperand; return *this; }
_AFXDISP_INLINE BOOL COleCurrency::operator==(const COleCurrency& cur) const
	{ return(m_status == cur.m_status && m_cur.Hi == cur.m_cur.Hi &&
		m_cur.Lo == cur.m_cur.Lo); }
_AFXDISP_INLINE BOOL COleCurrency::operator!=(const COleCurrency& cur) const
	{ return(m_status != cur.m_status || m_cur.Hi != cur.m_cur.Hi ||
		m_cur.Lo != cur.m_cur.Lo); }
_AFXDISP_INLINE COleCurrency::operator CURRENCY() const
	{ return m_cur; }

// COleSafeArray
_AFXDISP_INLINE COleSafeArray::COleSafeArray()
	{ AfxSafeArrayInit(this);
		vt = VT_EMPTY; }
_AFXDISP_INLINE COleSafeArray::~COleSafeArray()
	{ Clear(); }
_AFXDISP_INLINE void COleSafeArray::Clear()
	{ VERIFY(::VariantClear(this) == NOERROR); }
_AFXDISP_INLINE COleSafeArray::operator LPVARIANT()
	{ return this; }
_AFXDISP_INLINE COleSafeArray::operator LPCVARIANT() const
	{ return this; }
_AFXDISP_INLINE DWORD COleSafeArray::GetDim()
	{ return ::SafeArrayGetDim(parray); }
_AFXDISP_INLINE DWORD COleSafeArray::GetElemSize()
	{ return ::SafeArrayGetElemsize(parray); }

#endif //_AFXDISP_INLINE

/////////////////////////////////////////////////////////////////////////////
// OLE Container inlines

#ifdef _AFXOLECLI_INLINE

#endif //_AFXOLECLI_INLINE

#ifdef _AFXOLEDOBJ_INLINE

// COleDataObject
_AFXOLEDOBJ_INLINE COleDataObject::~COleDataObject()
	{ Release(); }

#endif //_AFXOLECTL_INLINE

/////////////////////////////////////////////////////////////////////////////
// OLE dialog inlines

#ifdef _AFXODLGS_INLINE

#endif //_AFXODLGS_INLINE

/////////////////////////////////////////////////////////////////////////////
// OLE Server inlines

#ifdef _AFXOLESVR_INLINE

#endif //_AFXOLESVR_INLINE

/////////////////////////////////////////////////////////////////////////////
