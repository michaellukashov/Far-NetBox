// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

// Inlines for AFXCOLL.H

#pragma once

#ifdef _AFXCOLL_INLINE

////////////////////////////////////////////////////////////////////////////

_AFXCOLL_INLINE INT_PTR CByteArray::GetSize() const
	{ return m_nSize; }
_AFXCOLL_INLINE INT_PTR CByteArray::GetCount() const
	{ return m_nSize; }
_AFXCOLL_INLINE BOOL CByteArray::IsEmpty() const
	{ return m_nSize == 0; }
_AFXCOLL_INLINE INT_PTR CByteArray::GetUpperBound() const
	{ return m_nSize-1; }
_AFXCOLL_INLINE void CByteArray::RemoveAll()
	{ SetSize(0); }
_AFXCOLL_INLINE BYTE CByteArray::GetAt(INT_PTR nIndex) const
	{ ASSERT(nIndex >= 0 && nIndex < m_nSize);
		if( nIndex < 0 || nIndex >= m_nSize )
			AfxThrowInvalidArgException();
		return m_pData[nIndex]; }
_AFXCOLL_INLINE void CByteArray::SetAt(INT_PTR nIndex, BYTE newElement)
	{ ASSERT(nIndex >= 0 && nIndex < m_nSize);
		if( nIndex < 0 || nIndex >= m_nSize )
			AfxThrowInvalidArgException();
		m_pData[nIndex] = newElement; }

_AFXCOLL_INLINE BYTE& CByteArray::ElementAt(INT_PTR nIndex)
	{ ASSERT(nIndex >= 0 && nIndex < m_nSize);
		if( nIndex < 0 || nIndex >= m_nSize )
			AfxThrowInvalidArgException();
		return m_pData[nIndex]; }
_AFXCOLL_INLINE const BYTE* CByteArray::GetData() const
	{ return (const BYTE*)m_pData; }
_AFXCOLL_INLINE BYTE* CByteArray::GetData()
	{ return (BYTE*)m_pData; }
_AFXCOLL_INLINE INT_PTR CByteArray::Add(BYTE newElement)
	{ INT_PTR nIndex = m_nSize;
		SetAtGrow(nIndex, newElement);
		return nIndex; }

_AFXCOLL_INLINE BYTE CByteArray::operator[](INT_PTR nIndex) const
	{ return GetAt(nIndex); }
_AFXCOLL_INLINE BYTE& CByteArray::operator[](INT_PTR nIndex)
	{ return ElementAt(nIndex); }


////////////////////////////////////////////////////////////////////////////

_AFXCOLL_INLINE INT_PTR CWordArray::GetSize() const
	{ return m_nSize; }
_AFXCOLL_INLINE INT_PTR CWordArray::GetCount() const
	{ return m_nSize; }
_AFXCOLL_INLINE BOOL CWordArray::IsEmpty() const
	{ return m_nSize == 0; }
_AFXCOLL_INLINE INT_PTR CWordArray::GetUpperBound() const
	{ return m_nSize-1; }
_AFXCOLL_INLINE void CWordArray::RemoveAll()
	{ SetSize(0); }
_AFXCOLL_INLINE WORD CWordArray::GetAt(INT_PTR nIndex) const
	{ ASSERT(nIndex >= 0 && nIndex < m_nSize);
		if( nIndex < 0 || nIndex >= m_nSize )
			AfxThrowInvalidArgException();
		return m_pData[nIndex]; }
_AFXCOLL_INLINE void CWordArray::SetAt(INT_PTR nIndex, WORD newElement)
	{ ASSERT(nIndex >= 0 && nIndex < m_nSize);
		if( nIndex < 0 || nIndex >= m_nSize )
			AfxThrowInvalidArgException();
		m_pData[nIndex] = newElement; }

_AFXCOLL_INLINE WORD& CWordArray::ElementAt(INT_PTR nIndex)
	{ ASSERT(nIndex >= 0 && nIndex < m_nSize);
		if( nIndex < 0 || nIndex >= m_nSize )
			AfxThrowInvalidArgException();
		return m_pData[nIndex]; }
_AFXCOLL_INLINE const WORD* CWordArray::GetData() const
	{ return (const WORD*)m_pData; }
_AFXCOLL_INLINE WORD* CWordArray::GetData()
	{ return (WORD*)m_pData; }
_AFXCOLL_INLINE INT_PTR CWordArray::Add(WORD newElement)
	{ INT_PTR nIndex = m_nSize;
		SetAtGrow(nIndex, newElement);
		return nIndex; }

_AFXCOLL_INLINE WORD CWordArray::operator[](INT_PTR nIndex) const
	{ return GetAt(nIndex); }
_AFXCOLL_INLINE WORD& CWordArray::operator[](INT_PTR nIndex)
	{ return ElementAt(nIndex); }


////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

#endif //_AFXCOLL_INLINE
