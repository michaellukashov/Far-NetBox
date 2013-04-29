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



#define new DEBUG_NEW

/////////////////////////////////////////////////////////////////////////////
// CCmdTarget::EnableTypeLib - locks the typelib cache for this class

void CCmdTarget::EnableTypeLib()
{
	AfxLockGlobals(CRIT_TYPELIBCACHE);

	AfxUnlockGlobals(CRIT_TYPELIBCACHE);
}

/////////////////////////////////////////////////////////////////////////////
// CCmdTarget::GetTypeInfoOfGuid - Returns typeinfo

HRESULT CCmdTarget::GetTypeInfoOfGuid(LCID lcid, REFGUID guid,
	LPTYPEINFO* ppTypeInfo)
{
	HRESULT hr = TYPE_E_CANTLOADLIBRARY;
	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// AfxGetTypeLibCache

CTypeLibCache* AFXAPI AfxGetTypeLibCache(const GUID* pTypeLibID)
{
	return NULL;
}

