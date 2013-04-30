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
#include <afxtempl.h>
#include <math.h>


/////////////////////////////////////////////////////////////////////////////
// helpers
typedef CFixedStringT<CStringW, 256> CTempStringW;

void AFXAPI AfxCheckError(SCODE sc)
{
	if (FAILED(sc))
	{
		if (sc == E_OUTOFMEMORY)
			AfxThrowMemoryException();
		else
			AfxThrowOleException(sc);
	}
}

AFX_STATIC BOOL AFXAPI _AfxCompareSafeArrays(SAFEARRAY* parray1, SAFEARRAY* parray2)
{
	BOOL bCompare = FALSE;

	// If one is NULL they must both be NULL to compare
	if (parray1 == NULL || parray2 == NULL)
	{
		return parray1 == parray2;
	}

	// Dimension must match and if 0, then arrays compare
	DWORD dwDim1 = ::SafeArrayGetDim(parray1);
	DWORD dwDim2 = ::SafeArrayGetDim(parray2);
	if (dwDim1 != dwDim2)
		return FALSE;
	else if (dwDim1 == 0)
		return TRUE;

	// Element size must match
	DWORD dwSize1 = ::SafeArrayGetElemsize(parray1);
	DWORD dwSize2 = ::SafeArrayGetElemsize(parray2);
	if (dwSize1 != dwSize2)
		return FALSE;

	long* pLBound1 = NULL;
	long* pLBound2 = NULL;
	long* pUBound1 = NULL;
	long* pUBound2 = NULL;

	void* pData1 = NULL;
	void* pData2 = NULL;

	TRY
	{
		// Bounds must match
		pLBound1 = new long[dwDim1];
		pLBound2 = new long[dwDim2];
		pUBound1 = new long[dwDim1];
		pUBound2 = new long[dwDim2];

		size_t nTotalElements = 1;

		// Get and compare bounds
		for (DWORD dwIndex = 0; dwIndex < dwDim1; dwIndex++)
		{
			AfxCheckError(::SafeArrayGetLBound(
				parray1, dwIndex+1, &pLBound1[dwIndex]));
			AfxCheckError(::SafeArrayGetLBound(
				parray2, dwIndex+1, &pLBound2[dwIndex]));
			AfxCheckError(::SafeArrayGetUBound(
				parray1, dwIndex+1, &pUBound1[dwIndex]));
			AfxCheckError(::SafeArrayGetUBound(
				parray2, dwIndex+1, &pUBound2[dwIndex]));

			// Check the magnitude of each bound
			if (pUBound1[dwIndex] - pLBound1[dwIndex] !=
				pUBound2[dwIndex] - pLBound2[dwIndex])
			{
				delete[] pLBound1;
				delete[] pLBound2;
				delete[] pUBound1;
				delete[] pUBound2;

				return FALSE;
			}

			// Increment the element count
			nTotalElements *= pUBound1[dwIndex] - pLBound1[dwIndex] + 1;
		}

		// Access the data
		AfxCheckError(::SafeArrayAccessData(parray1, &pData1));
		AfxCheckError(::SafeArrayAccessData(parray2, &pData2));

		// Calculate the number of bytes of data and compare
		size_t nSize = nTotalElements * dwSize1;
		int nOffset = memcmp(pData1, pData2, nSize);
		bCompare = nOffset == 0;

		// Release the array locks
		AfxCheckError(::SafeArrayUnaccessData(parray1));
		AfxCheckError(::SafeArrayUnaccessData(parray2));
	}
	CATCH_ALL(e)
	{
		// Clean up bounds arrays
		delete[] pLBound1;
		delete[] pLBound2;
		delete[] pUBound1;
		delete[] pUBound2;

		// Release the array locks
		if (pData1 != NULL)
			AfxCheckError(::SafeArrayUnaccessData(parray1));
		if (pData2 != NULL)
			AfxCheckError(::SafeArrayUnaccessData(parray2));

		THROW_LAST();
	}
	END_CATCH_ALL

	// Clean up bounds arrays
	delete[] pLBound1;
	delete[] pLBound2;
	delete[] pUBound1;
	delete[] pUBound2;

	return bCompare;
}

AFX_STATIC void AFXAPI _AfxCreateOneDimArray(VARIANT& varSrc, DWORD dwSize)
{
	UINT nDim;

	// Clear VARIANT and re-create SafeArray if necessary
	if (varSrc.vt != (VT_UI1 | VT_ARRAY) ||
		(nDim = ::SafeArrayGetDim(varSrc.parray)) != 1)
	{
		VERIFY(::VariantClear(&varSrc) == NOERROR);
		varSrc.vt = VT_UI1 | VT_ARRAY;

		SAFEARRAYBOUND bound;
		bound.cElements = dwSize;
		bound.lLbound = 0;
		varSrc.parray = ::SafeArrayCreate(VT_UI1, 1, &bound);
		if (varSrc.parray == NULL)
			AfxThrowMemoryException();
	}
	else
	{
		// Must redimension array if necessary
		long lLower, lUpper;
		AfxCheckError(::SafeArrayGetLBound(varSrc.parray, 1, &lLower));
		AfxCheckError(::SafeArrayGetUBound(varSrc.parray, 1, &lUpper));

		// Upper bound should always be greater than lower bound
		long lSize = lUpper - lLower;
		if (lSize < 0)
		{
			ASSERT(FALSE);
			lSize = 0;

		}

		if ((DWORD)lSize != dwSize)
		{
			SAFEARRAYBOUND bound;
			bound.cElements = dwSize;
			bound.lLbound = lLower;
			AfxCheckError(::SafeArrayRedim(varSrc.parray, &bound));
		}
	}
}

AFX_STATIC void AFXAPI _AfxCopyBinaryData(SAFEARRAY* parray, const void* pvSrc, DWORD dwSize)
{
	// Access the data, copy it and unaccess it.
	void* pDest;
	AfxCheckError(::SafeArrayAccessData(parray, &pDest));
	Checked::memcpy_s(pDest, dwSize, pvSrc, dwSize);
	AfxCheckError(::SafeArrayUnaccessData(parray));
}

/////////////////////////////////////////////////////////////////////////////
// Diagnostics

//////////////////////////////////////////////////////////////////////////////
// CComBSTR support
/////////////////////////////////////////////////////////////////////////////
