// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#ifndef __AFXCOLL_H__
#define __AFXCOLL_H__

#ifndef __AFX_H__
	#include <afx.h>
#endif

#pragma once

#ifdef _AFX_MINREBUILD
//#pragma component(minrebuild, off)
#endif 

#ifdef _AFX_PACKING
#pragma pack(push, _AFX_PACKING)
#endif

/////////////////////////////////////////////////////////////////////////////
// Classes declared in this file

//CObject
	// Arrays
	class CByteArray;           // array of BYTE
	class CWordArray;           // array of WORD
	class CDWordArray;          // array of DWORD
	class CUIntArray;           // array of UINT
	class CPtrArray;            // array of void*

	// Lists
	class CPtrList;             // list of void*
	class CObList;              // list of CObject*

	// Maps (aka Dictionaries)
	// class CMapWordToPtr;        // map from WORD to void*
    // class CMapPtrToWord;        // map from void* to WORD
//    class CMapPtrToPtr;         // map from void* to void*

	// Special String variants
//	class CStringArray;         // array of CStrings
//	class CStringList;          // list of CStrings
	// class CMapStringToPtr;      // map from CString to void*
	// class CMapStringToOb;       // map from CString to CObject*
	// class CMapStringToString;   // map from CString to CString

/////////////////////////////////////////////////////////////////////////////

#undef AFX_DATA
#define AFX_DATA AFX_CORE_DATA

////////////////////////////////////////////////////////////////////////////

class CByteArray : public CObject
{

	DECLARE_SERIAL(CByteArray)
public:

// Construction
	CByteArray();

// Attributes
	INT_PTR GetSize() const;
	INT_PTR GetCount() const;
	BOOL IsEmpty() const;
	INT_PTR GetUpperBound() const;
	void SetSize(INT_PTR nNewSize, INT_PTR nGrowBy = -1);

// Operations
	// Clean up
	void FreeExtra();
	void RemoveAll();

	// Accessing elements
	BYTE GetAt(INT_PTR nIndex) const;
	void SetAt(INT_PTR nIndex, BYTE newElement);

	BYTE& ElementAt(INT_PTR nIndex);

	// Direct Access to the element data (may return NULL)
	const BYTE* GetData() const;
	BYTE* GetData();

	// Potentially growing the array
	void SetAtGrow(INT_PTR nIndex, BYTE newElement);

	INT_PTR Add(BYTE newElement);

	INT_PTR Append(const CByteArray& src);
	void Copy(const CByteArray& src);

	// overloaded operator helpers
	BYTE operator[](INT_PTR nIndex) const;
	BYTE& operator[](INT_PTR nIndex);

	// Operations that move elements around
	void InsertAt(INT_PTR nIndex, BYTE newElement, INT_PTR nCount = 1);

	void RemoveAt(INT_PTR nIndex, INT_PTR nCount = 1);
	void InsertAt(INT_PTR nStartIndex, CByteArray* pNewArray);

// Implementation
protected:
	BYTE* m_pData;   // the actual array of data
	INT_PTR m_nSize;     // # of elements (upperBound - 1)
	INT_PTR m_nMaxSize;  // max allocated
	INT_PTR m_nGrowBy;   // grow amount


public:
	~CByteArray();

protected:
	// local typedefs for class templates
	typedef BYTE BASE_TYPE;
	typedef BYTE BASE_ARG_TYPE;
};


////////////////////////////////////////////////////////////////////////////

class CWordArray : public CObject
{

	DECLARE_SERIAL(CWordArray)
public:

// Construction
	CWordArray();

// Attributes
	INT_PTR GetSize() const;
	INT_PTR GetCount() const;
	BOOL IsEmpty() const;
	INT_PTR GetUpperBound() const;
	void SetSize(INT_PTR nNewSize, INT_PTR nGrowBy = -1);

// Operations
	// Clean up
	void FreeExtra();
	void RemoveAll();

	// Accessing elements
	WORD GetAt(INT_PTR nIndex) const;
	void SetAt(INT_PTR nIndex, WORD newElement);

	WORD& ElementAt(INT_PTR nIndex);

	// Direct Access to the element data (may return NULL)
	const WORD* GetData() const;
	WORD* GetData();

	// Potentially growing the array
	void SetAtGrow(INT_PTR nIndex, WORD newElement);

	INT_PTR Add(WORD newElement);

	INT_PTR Append(const CWordArray& src);
	void Copy(const CWordArray& src);

	// overloaded operator helpers
	WORD operator[](INT_PTR nIndex) const;
	WORD& operator[](INT_PTR nIndex);

	// Operations that move elements around
	void InsertAt(INT_PTR nIndex, WORD newElement, INT_PTR nCount = 1);

	void RemoveAt(INT_PTR nIndex, INT_PTR nCount = 1);
	void InsertAt(INT_PTR nStartIndex, CWordArray* pNewArray);

// Implementation
protected:
	WORD* m_pData;   // the actual array of data
	INT_PTR m_nSize;     // # of elements (upperBound - 1)
	INT_PTR m_nMaxSize;  // max allocated
	INT_PTR m_nGrowBy;   // grow amount


public:
	~CWordArray();

protected:
	// local typedefs for class templates
	typedef WORD BASE_TYPE;
	typedef WORD BASE_ARG_TYPE;
};



#ifdef _AFX_PACKING
#pragma pack(pop)
#endif

#ifndef __AFXSTATE_H__
	#include <afxstat_.h>   // for MFC private state structures
#endif

/////////////////////////////////////////////////////////////////////////////
// Inline function declarations

#ifdef _AFX_ENABLE_INLINES
#define _AFXCOLL_INLINE AFX_INLINE
#include <afxcoll.inl>
#endif

#undef AFX_DATA
#define AFX_DATA

#ifdef _AFX_MINREBUILD
//#pragma component(minrebuild, on)
#endif

#endif //!__AFXCOLL_H__

/////////////////////////////////////////////////////////////////////////////
