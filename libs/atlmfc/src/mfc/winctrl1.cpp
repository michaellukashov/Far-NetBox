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
#ifndef _AFX_NO_OCC_SUPPORT
#include "occimpl.h"
#endif



#define new DEBUG_NEW

// Helper for radio buttons
int CWnd::GetCheckedRadioButton(int nIDFirstButton, int nIDLastButton) const
{
	for (int nID = nIDFirstButton; nID <= nIDLastButton; nID++)
	{
		if (IsDlgButtonChecked(nID))
			return nID; // id that matched
	}
	return 0; // invalid ID
}

/////////////////////////////////////////////////////////////////////////////
