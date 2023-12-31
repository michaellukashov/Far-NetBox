// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

/////////////////////////////////////////////////////////////////////////////
// AFXSTR.H - Framework-independent, templateable string class

#ifndef __AFXSTR_H__
#define __AFXSTR_H__

#pragma once

#ifdef _AFX_MINREBUILD
//#pragma component(minrebuild, off)
#endif

#ifndef _AFX
#error afxstr.h can only be used in MFC projects.  Use atlstr.h
#endif

#include <mbstring.h>

HINSTANCE AFXAPI AfxGetResourceHandle();
HINSTANCE AFXAPI AfxFindStringResourceHandle(UINT nID);

#include <atlcore.h>
#include <cstringt.h>

ATL::IAtlStringMgr* AFXAPI AfxGetStringManager();

template< typename _CharType = char, class StringIterator = ATL::ChTraitsCRT< _CharType > >
class StrTraitMFC : 
	public StringIterator
{
public:
	static HINSTANCE FindStringResourceInstance( UINT nID ) throw()
	{
		return( AfxFindStringResourceHandle( nID ) );
	}

	static ATL::IAtlStringMgr* GetDefaultManager() throw()
	{
		return( AfxGetStringManager() );
	}
};

// MFC-enabled compilation. Use MFC memory management and exceptions;
// also, use MFC module state.

// Don't import when MFC dll is being built
typedef ATL::CStringT< wchar_t, StrTraitMFC< wchar_t > > CStringW;
typedef ATL::CStringT< char, StrTraitMFC< char > > CStringA;
typedef ATL::CStringT< TCHAR, StrTraitMFC< TCHAR > > CString;

#ifdef _AFX_MINREBUILD
//#pragma component(minrebuild, on)
#endif

#endif	// __AFXSTR_H__ (whole file)
