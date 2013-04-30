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
// COleVariant class

COleVariant::COleVariant(const VARIANT& varSrc)
{
	AfxVariantInit(this);
	AfxCheckError(::VariantCopy(this, (LPVARIANT)&varSrc));
}

COleVariant::COleVariant(LPCVARIANT pSrc)
{
	AfxVariantInit(this);
	AfxCheckError(::VariantCopy(this, (LPVARIANT)pSrc));
}

COleVariant::COleVariant(const COleVariant& varSrc)
{
	AfxVariantInit(this);
	AfxCheckError(::VariantCopy(this, (LPVARIANT)&varSrc));
}

COleVariant::COleVariant(LPCTSTR lpszSrc, VARTYPE vtSrc)
{
#if defined (UNICODE)
	ASSERT(vtSrc == VT_BSTR);
#else
	ASSERT(vtSrc == VT_BSTR || vtSrc == VT_BSTRT);
#endif
	UNUSED(vtSrc);

	vt = VT_BSTR;
	bstrVal = NULL;

	if (lpszSrc != NULL)
	{
#ifndef _UNICODE
		if (vtSrc == VT_BSTRT)
		{
			int nLen = lstrlen(lpszSrc);
			bstrVal = ::SysAllocStringByteLen(lpszSrc, nLen);
			
			if (bstrVal == NULL)
				AfxThrowMemoryException();
		}
		else
#endif
		{
			bstrVal = CTempStringW(lpszSrc).AllocSysString();
		}
	}
}



void COleVariant::SetString(LPCTSTR lpszSrc, VARTYPE vtSrc)
{
#if defined (UNICODE)
	ASSERT(vtSrc == VT_BSTR);
#else
	ASSERT(vtSrc == VT_BSTR || vtSrc == VT_BSTRT);
#endif
	UNUSED(vtSrc);

	// Free up previous VARIANT
	Clear();

	vt = VT_BSTR;
	bstrVal = NULL;

	if (lpszSrc != NULL)
	{
#ifndef _UNICODE
		if (vtSrc == VT_BSTRT)
		{
			int nLen = lstrlen(lpszSrc);
			bstrVal = ::SysAllocStringByteLen(lpszSrc, nLen);

			if (bstrVal == NULL)
				AfxThrowMemoryException();
		}
		else
#endif
		{
			bstrVal = CTempStringW(lpszSrc).AllocSysString();
		}
	}
}

COleVariant::COleVariant(short nSrc, VARTYPE vtSrc)
{
	ASSERT(vtSrc == VT_I2 || vtSrc == VT_BOOL);

	if (vtSrc == VT_BOOL)
	{
		vt = VT_BOOL;
		if (!nSrc)
			V_BOOL(this) = AFX_OLE_FALSE;
		else
			V_BOOL(this) = AFX_OLE_TRUE;
	}
	else
	{
		vt = VT_I2;
		iVal = nSrc;
	}
}

COleVariant::COleVariant(long lSrc, VARTYPE vtSrc)
{
	ASSERT(vtSrc == VT_I4 || vtSrc == VT_ERROR || vtSrc == VT_BOOL
		|| vtSrc == VT_UINT || vtSrc == VT_INT || vtSrc == VT_UI4
		|| vtSrc == VT_HRESULT);

	if (vtSrc == VT_ERROR)
	{
		vt = VT_ERROR;
		scode = lSrc;
	}
	else if (vtSrc == VT_BOOL)
	{
		vt = VT_BOOL;
		if (!lSrc)
			V_BOOL(this) = AFX_OLE_FALSE;
		else
			V_BOOL(this) = AFX_OLE_TRUE;
	}
	else if (vtSrc == VT_INT)
	{
		vt = VT_INT;
	 	V_INT(this) = lSrc;
	}
	else if (vtSrc == VT_UINT)
	{
		vt = VT_UINT;
	 	V_UINT(this) = lSrc;
	}
	else if (vtSrc == VT_HRESULT)
	{
		vt = VT_HRESULT;
		V_ERROR(this) = lSrc;
	}
	else if (vtSrc == VT_UI4)
	{
		vt = VT_UI4;
		lVal = lSrc;
	}
	else
	{
		vt = VT_I4;
		lVal = lSrc;
	}
}

// Operations
void COleVariant::ChangeType(VARTYPE vartype, LPVARIANT pSrc)
{
	// If pSrc is NULL, convert type in place
	if (pSrc == NULL)
		pSrc = this;
	if (pSrc != this || vartype != vt)
		AfxCheckError(::VariantChangeType(this, pSrc, 0, vartype));
}

void COleVariant::Attach(VARIANT& varSrc)
{
	// Free up previous VARIANT
	Clear();

	// give control of data to COleVariant 
	Checked::memcpy_s(this, sizeof(VARIANT), &varSrc, sizeof(varSrc));
	varSrc.vt = VT_EMPTY;
}

VARIANT COleVariant::Detach()
{
	VARIANT varResult = *this;
	vt = VT_EMPTY;
	return varResult;
}

void COleVariant::GetByteArrayFromVariantArray(CByteArray& bytes)
{
	ASSERT( V_ISARRAY(this) );

	LPVOID pSrc;
	LPVOID pDest;
	HRESULT hResult;
	ULONG nDim;
	LONG iLowerBound;
	LONG iUpperBound;
	LONG nElements;
	ULONG nBytes;

	hResult = ::SafeArrayAccessData(V_ARRAY(this), &pSrc);
	AfxCheckError(hResult);
	
	nDim = ::SafeArrayGetDim(V_ARRAY(this));
	ASSERT( (nDim == 0) || (nDim == 1) );

	if (nDim == 1)
	{
		::SafeArrayGetLBound(V_ARRAY(this), 1, &iLowerBound);
		::SafeArrayGetUBound(V_ARRAY(this), 1, &iUpperBound);
		nElements = (iUpperBound-iLowerBound)+1;
		nBytes = nElements*::SafeArrayGetElemsize(V_ARRAY(this));
		bytes.SetSize( nBytes );
		pDest = bytes.GetData();
		Checked::memcpy_s(pDest, nBytes, pSrc, nBytes);
	}
	else
	{
		bytes.SetSize(0);
	}

	::SafeArrayUnaccessData(V_ARRAY(this));
}

// Literal comparison. Types and values must match.
BOOL COleVariant::operator==(const VARIANT& var) const
{
	if (&var == this)
		return TRUE;

	// Variants not equal if types don't match
	if (var.vt != vt)
		return FALSE;

	// Check type specific values
	switch (vt)
	{
	case VT_EMPTY:
	case VT_NULL:
		return TRUE;

	case VT_BOOL:
		return V_BOOL(&var) == V_BOOL(this);

	case VT_I1:
		return var.cVal == cVal;

	case VT_UI1:
		return var.bVal == bVal;

	case VT_I2:
		return var.iVal == iVal;

	case VT_UI2:
		return var.uiVal == uiVal;

	case VT_I4:
		return var.lVal == lVal;

	case VT_UI4:
		return var.ulVal == ulVal;

	case VT_I8:
		return var.llVal == llVal;

	case VT_UI8:
		return var.ullVal == ullVal;

	case VT_CY:
		return (var.cyVal.Hi == cyVal.Hi && var.cyVal.Lo == cyVal.Lo);

	case VT_R4:
		return var.fltVal == fltVal;

	case VT_R8:
		return var.dblVal == dblVal;

	case VT_DATE:
		return var.date == date;

	case VT_BSTR:
		return SysStringByteLen(var.bstrVal) == SysStringByteLen(bstrVal) &&
			memcmp(var.bstrVal, bstrVal, SysStringByteLen(bstrVal)) == 0;

	case VT_ERROR:
		return var.scode == scode;

	case VT_DISPATCH:
	case VT_UNKNOWN:
		return var.punkVal == punkVal;

	default:
		if (vt & VT_ARRAY && !(vt & VT_BYREF))
			return _AfxCompareSafeArrays(var.parray, parray);
		else
			ASSERT(FALSE);  // VT_BYREF not supported
		// fall through
	}

	return FALSE;
}

const COleVariant& COleVariant::operator=(const VARIANT& varSrc)
{
	if(static_cast<LPVARIANT>(this) != &varSrc)
	{
	AfxCheckError(::VariantCopy(this, (LPVARIANT)&varSrc));
	}

	return *this;
}

const COleVariant& COleVariant::operator=(LPCVARIANT pSrc)
{
	if(static_cast<LPCVARIANT>(this) != pSrc)
	{
	AfxCheckError(::VariantCopy(this, (LPVARIANT)pSrc));
	}

	return *this;
}

const COleVariant& COleVariant::operator=(const COleVariant& varSrc)
{
	if(this != &varSrc)
	{
	AfxCheckError(::VariantCopy(this, (LPVARIANT)&varSrc));
	}

	return *this;
}

const COleVariant& COleVariant::operator=(const LPCTSTR lpszSrc)
{
	// Free up previous VARIANT
	Clear();

	vt = VT_BSTR;
	if (lpszSrc == NULL)
		bstrVal = NULL;
	else
	{
		bstrVal = CTempStringW(lpszSrc).AllocSysString();
	}
	return *this;
}

const COleVariant& COleVariant::operator=(const CString& strSrc)
{
	// Free up previous VARIANT
	Clear();

	vt = VT_BSTR;
	bstrVal = strSrc.AllocSysString();

	return *this;
}

const COleVariant& COleVariant::operator=(BYTE nSrc)
{
	// Free up previous VARIANT if necessary
	if (vt != VT_UI1)
	{
		Clear();
		vt = VT_UI1;
	}

	bVal = nSrc;
	return *this;
}

const COleVariant& COleVariant::operator=(short nSrc)
{
	if (vt == VT_I2)
		iVal = nSrc;
	else if (vt == VT_BOOL)
	{
		if (!nSrc)
			V_BOOL(this) = AFX_OLE_FALSE;
		else
			V_BOOL(this) = AFX_OLE_TRUE;
	}
	else
	{
		// Free up previous VARIANT
		Clear();
		vt = VT_I2;
		iVal = nSrc;
	}

	return *this;
}

const COleVariant& COleVariant::operator=(long lSrc)
{
	if (vt == VT_I4)
		lVal = lSrc;
	else if (vt == VT_ERROR)
		scode = lSrc;
	else if (vt == VT_BOOL)
	{
		if (!lSrc)
			V_BOOL(this) = AFX_OLE_FALSE;
		else
			V_BOOL(this) = AFX_OLE_TRUE;
	}
	else
	{
		// Free up previous VARIANT
		Clear();
		vt = VT_I4;
		lVal = lSrc;
	}

	return *this;
}

#if (_WIN32_WINNT >= 0x0501) || defined(_ATL_SUPPORT_VT_I8)
const COleVariant& COleVariant::operator=(LONGLONG nSrc)
{
	if (vt != VT_I8)
	{
		Clear();
		vt = VT_I8;
	}

	llVal = nSrc;
	return *this;
}

const COleVariant& COleVariant::operator=(ULONGLONG nSrc)
{
	if (vt != VT_UI8)
	{
		Clear();
		vt = VT_UI8;
	}

	ullVal = nSrc;
	return *this;
}
#endif

const COleVariant& COleVariant::operator=(float fltSrc)
{
	// Free up previous VARIANT if necessary
	if (vt != VT_R4)
	{
		Clear();
		vt = VT_R4;
	}

	fltVal = fltSrc;
	return *this;
}

const COleVariant& COleVariant::operator=(double dblSrc)
{
	// Free up previous VARIANT if necessary
	if (vt != VT_R8)
	{
		Clear();
		vt = VT_R8;
	}

	dblVal = dblSrc;
	return *this;
}

const COleVariant& COleVariant::operator=(const CByteArray& arrSrc)
{
	INT_PTR nSize = arrSrc.GetSize();
	if( nSize > LONG_MAX )
	{
		AfxThrowMemoryException();
	}

	// Set the correct type and make sure SafeArray can hold data
	_AfxCreateOneDimArray(*this, (DWORD)nSize);

	// Copy the data into the SafeArray
	_AfxCopyBinaryData(parray, arrSrc.GetData(), (DWORD)nSize);

	return *this;
}

void AFXAPI AfxVariantInit(LPVARIANT pVar)
{
	memset(pVar, 0, sizeof(*pVar));
}

/////////////////////////////////////////////////////////////////////////////
// Diagnostics

//////////////////////////////////////////////////////////////////////////////
// CComBSTR support

/////////////////////////////////////////////////////////////////////////////
// COleVariant Helpers
template <> void AFXAPI CopyElements<COleVariant> (COleVariant* pDest, const COleVariant* pSrc, INT_PTR nCount)
{
	ENSURE_ARG(nCount == 0 || pDest != NULL && pSrc != NULL);
	ASSERT(nCount == 0 ||
		AfxIsValidAddress(pDest, (size_t)nCount * sizeof(COleVariant)));
	ASSERT(nCount == 0 ||
		AfxIsValidAddress(pSrc, (size_t)nCount * sizeof(COleVariant)));

	for (; nCount--; ++pDest, ++pSrc)
		*pDest = *pSrc;
}

template<> UINT AFXAPI HashKey<const struct tagVARIANT&> (const struct tagVARIANT& var)
{
	switch (var.vt)
	{
	case VT_EMPTY:
	case VT_NULL:
		return 0;
	case VT_I2:
		return HashKey<DWORD>((DWORD)var.iVal);
	case VT_I4:
		return HashKey<DWORD>((DWORD)var.lVal);
	case VT_R4:
		return (UINT)(var.fltVal / 16);
	case VT_R8:
	case VT_CY:
		return (UINT)(var.dblVal / 16);
	case VT_BOOL:
		return HashKey<DWORD>((DWORD)V_BOOL(&var));
	case VT_ERROR:
		return HashKey<DWORD>((DWORD)var.scode);
	case VT_DATE:
		return (UINT)(var.date / 16);
	case VT_BSTR:
		return HashKey<LPCOLESTR>(var.bstrVal);
	case VT_DISPATCH:
	case VT_UNKNOWN:
#ifdef _WIN64
		return HashKey<DWORD_PTR>((DWORD_PTR)var.punkVal);
#else
		return HashKey<DWORD>((DWORD)(DWORD_PTR)var.punkVal);
#endif
	default:
		// No support for VT_BYREF, VT_ARRAY, VT_VARIANT, VT_DECIMAL, & VT_UI1
		ASSERT(FALSE);

		// Fall through
	}

	return 0;
}


/////////////////////////////////////////////////////////////////////////////
