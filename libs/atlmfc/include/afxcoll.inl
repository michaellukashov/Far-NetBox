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

_AFXCOLL_INLINE INT_PTR CPtrArray::GetSize() const
	{ return m_nSize; }
_AFXCOLL_INLINE INT_PTR CPtrArray::GetCount() const
	{ return m_nSize; }
_AFXCOLL_INLINE BOOL CPtrArray::IsEmpty() const
	{ return m_nSize == 0; }
_AFXCOLL_INLINE INT_PTR CPtrArray::GetUpperBound() const
	{ return m_nSize-1; }
_AFXCOLL_INLINE void CPtrArray::RemoveAll()
	{ SetSize(0); }
_AFXCOLL_INLINE void* CPtrArray::GetAt(INT_PTR nIndex) const
	{ ASSERT(nIndex >= 0 && nIndex < m_nSize);
		if( nIndex < 0 || nIndex >= m_nSize )
			AfxThrowInvalidArgException();
		return m_pData[nIndex]; }
_AFXCOLL_INLINE void CPtrArray::SetAt(INT_PTR nIndex, void* newElement)
	{ ASSERT(nIndex >= 0 && nIndex < m_nSize);
		if( nIndex < 0 || nIndex >= m_nSize )
			AfxThrowInvalidArgException();
		m_pData[nIndex] = newElement; }

_AFXCOLL_INLINE void*& CPtrArray::ElementAt(INT_PTR nIndex)
	{ ASSERT(nIndex >= 0 && nIndex < m_nSize);
		if( nIndex < 0 || nIndex >= m_nSize )
			AfxThrowInvalidArgException();
		return m_pData[nIndex]; }
_AFXCOLL_INLINE const void** CPtrArray::GetData() const
	{ return (const void**)m_pData; }
_AFXCOLL_INLINE void** CPtrArray::GetData()
	{ return (void**)m_pData; }
_AFXCOLL_INLINE INT_PTR CPtrArray::Add(void* newElement)
	{ INT_PTR nIndex = m_nSize;
		SetAtGrow(nIndex, newElement);
		return nIndex; }

_AFXCOLL_INLINE void* CPtrArray::operator[](INT_PTR nIndex) const
	{ return GetAt(nIndex); }
_AFXCOLL_INLINE void*& CPtrArray::operator[](INT_PTR nIndex)
	{ return ElementAt(nIndex); }


////////////////////////////////////////////////////////////////////////////

_AFXCOLL_INLINE INT_PTR CPtrList::GetCount() const
	{ return m_nCount; }
_AFXCOLL_INLINE INT_PTR CPtrList::GetSize() const
	{ return m_nCount; }
_AFXCOLL_INLINE BOOL CPtrList::IsEmpty() const
	{ return m_nCount == 0; }
_AFXCOLL_INLINE void*& CPtrList::GetHead()
	{ ASSERT(m_pNodeHead != NULL);
		return m_pNodeHead->data; }
_AFXCOLL_INLINE const void* CPtrList::GetHead() const
	{ ASSERT(m_pNodeHead != NULL);
		return m_pNodeHead->data; }
_AFXCOLL_INLINE void*& CPtrList::GetTail()
	{ ASSERT(m_pNodeTail != NULL);
		return m_pNodeTail->data; }
_AFXCOLL_INLINE const void* CPtrList::GetTail() const
	{ ASSERT(m_pNodeTail != NULL);
		return m_pNodeTail->data; }
_AFXCOLL_INLINE POSITION CPtrList::GetHeadPosition() const
	{ return (POSITION) m_pNodeHead; }
_AFXCOLL_INLINE POSITION CPtrList::GetTailPosition() const
	{ return (POSITION) m_pNodeTail; }
_AFXCOLL_INLINE void*& CPtrList::GetNext(POSITION& rPosition) // return *Position++
	{ CNode* pNode = (CNode*) rPosition;
		ASSERT(AfxIsValidAddress(pNode, sizeof(CNode)));
		if( pNode == NULL )
			AfxThrowInvalidArgException();
		rPosition = (POSITION) pNode->pNext;
		return pNode->data; }
_AFXCOLL_INLINE const void* CPtrList::GetNext(POSITION& rPosition) const // return *Position++
	{ CNode* pNode = (CNode*) rPosition;
		ASSERT(AfxIsValidAddress(pNode, sizeof(CNode)));
		if( pNode == NULL )
			AfxThrowInvalidArgException();
		rPosition = (POSITION) pNode->pNext;
		return pNode->data; }
_AFXCOLL_INLINE void*& CPtrList::GetPrev(POSITION& rPosition) // return *Position--
	{ CNode* pNode = (CNode*) rPosition;
		ASSERT(AfxIsValidAddress(pNode, sizeof(CNode)));
		if( pNode == NULL )
			AfxThrowInvalidArgException();
		rPosition = (POSITION) pNode->pPrev;
		return pNode->data; }
_AFXCOLL_INLINE const void* CPtrList::GetPrev(POSITION& rPosition) const // return *Position--
	{ CNode* pNode = (CNode*) rPosition;
		ASSERT(AfxIsValidAddress(pNode, sizeof(CNode)));
		if( pNode == NULL )
			AfxThrowInvalidArgException();
		rPosition = (POSITION) pNode->pPrev;
		return pNode->data; }
_AFXCOLL_INLINE void*& CPtrList::GetAt(POSITION position)
	{ CNode* pNode = (CNode*) position;
		ASSERT(AfxIsValidAddress(pNode, sizeof(CNode)));
		if( pNode == NULL )
			AfxThrowInvalidArgException();
		return pNode->data; }
_AFXCOLL_INLINE const void* CPtrList::GetAt(POSITION position) const
	{ CNode* pNode = (CNode*) position;
		ASSERT(AfxIsValidAddress(pNode, sizeof(CNode)));
		if( pNode == NULL )
			AfxThrowInvalidArgException();
		return pNode->data; }
_AFXCOLL_INLINE void CPtrList::SetAt(POSITION pos, void* newElement)
	{ CNode* pNode = (CNode*) pos;
		ASSERT(AfxIsValidAddress(pNode, sizeof(CNode)));
		if( pNode == NULL )
			AfxThrowInvalidArgException();
		pNode->data = newElement; }



////////////////////////////////////////////////////////////////////////////

_AFXCOLL_INLINE INT_PTR CObList::GetCount() const
	{ return m_nCount; }
_AFXCOLL_INLINE INT_PTR CObList::GetSize() const
	{ return m_nCount; }
_AFXCOLL_INLINE BOOL CObList::IsEmpty() const
	{ return m_nCount == 0; }
_AFXCOLL_INLINE CObject*& CObList::GetHead()
	{ ASSERT(m_pNodeHead != NULL);
		return m_pNodeHead->data; }
_AFXCOLL_INLINE const CObject* CObList::GetHead() const
	{ ASSERT(m_pNodeHead != NULL);
		return m_pNodeHead->data; }
_AFXCOLL_INLINE CObject*& CObList::GetTail()
	{ ASSERT(m_pNodeTail != NULL);
		return m_pNodeTail->data; }
_AFXCOLL_INLINE const CObject* CObList::GetTail() const
	{ ASSERT(m_pNodeTail != NULL);
		return m_pNodeTail->data; }
_AFXCOLL_INLINE POSITION CObList::GetHeadPosition() const
	{ return (POSITION) m_pNodeHead; }
_AFXCOLL_INLINE POSITION CObList::GetTailPosition() const
	{ return (POSITION) m_pNodeTail; }
_AFXCOLL_INLINE CObject*& CObList::GetNext(POSITION& rPosition) // return *Position++
	{ CNode* pNode = (CNode*) rPosition;
		ASSERT(AfxIsValidAddress(pNode, sizeof(CNode)));
		if( pNode == NULL )
			AfxThrowInvalidArgException();
		rPosition = (POSITION) pNode->pNext;
		return pNode->data; }
_AFXCOLL_INLINE const CObject* CObList::GetNext(POSITION& rPosition) const // return *Position++
	{ CNode* pNode = (CNode*) rPosition;
		ASSERT(AfxIsValidAddress(pNode, sizeof(CNode)));
		if( pNode == NULL )
			AfxThrowInvalidArgException();
		rPosition = (POSITION) pNode->pNext;
		return pNode->data; }
_AFXCOLL_INLINE CObject*& CObList::GetPrev(POSITION& rPosition) // return *Position--
	{ CNode* pNode = (CNode*) rPosition;
		ASSERT(AfxIsValidAddress(pNode, sizeof(CNode)));
		if( pNode == NULL )
			AfxThrowInvalidArgException();
		rPosition = (POSITION) pNode->pPrev;
		return pNode->data; }
_AFXCOLL_INLINE const CObject* CObList::GetPrev(POSITION& rPosition) const // return *Position--
	{ CNode* pNode = (CNode*) rPosition;
		ASSERT(AfxIsValidAddress(pNode, sizeof(CNode)));
		if( pNode == NULL )
			AfxThrowInvalidArgException();
		rPosition = (POSITION) pNode->pPrev;
		return pNode->data; }
_AFXCOLL_INLINE CObject*& CObList::GetAt(POSITION position)
	{ CNode* pNode = (CNode*) position;
		ASSERT(AfxIsValidAddress(pNode, sizeof(CNode)));
		if( pNode == NULL )
			AfxThrowInvalidArgException();
		return pNode->data; }
_AFXCOLL_INLINE const CObject* CObList::GetAt(POSITION position) const
	{ CNode* pNode = (CNode*) position;
		ASSERT(AfxIsValidAddress(pNode, sizeof(CNode)));
		if( pNode == NULL )
			AfxThrowInvalidArgException();
		return pNode->data; }
_AFXCOLL_INLINE void CObList::SetAt(POSITION pos, CObject* newElement)
	{ CNode* pNode = (CNode*) pos;
		ASSERT(AfxIsValidAddress(pNode, sizeof(CNode)));
		if( pNode == NULL )
			AfxThrowInvalidArgException();
		pNode->data = newElement; }



////////////////////////////////////////////////////////////////////////////

//_AFXCOLL_INLINE INT_PTR CStringList::GetCount() const
//	{ return m_nCount; }
//_AFXCOLL_INLINE INT_PTR CStringList::GetSize() const
//	{ return m_nCount; }
//_AFXCOLL_INLINE BOOL CStringList::IsEmpty() const
//	{ return m_nCount == 0; }
//_AFXCOLL_INLINE CString& CStringList::GetHead()
//	{ ASSERT(m_pNodeHead != NULL);
//		return m_pNodeHead->data; }
//_AFXCOLL_INLINE const CString& CStringList::GetHead() const
//	{ ASSERT(m_pNodeHead != NULL);
//		return m_pNodeHead->data; }
//_AFXCOLL_INLINE CString& CStringList::GetTail()
//	{ ASSERT(m_pNodeTail != NULL);
//		return m_pNodeTail->data; }
//_AFXCOLL_INLINE const CString& CStringList::GetTail() const
//	{ ASSERT(m_pNodeTail != NULL);
//		return m_pNodeTail->data; }
//_AFXCOLL_INLINE POSITION CStringList::GetHeadPosition() const
//	{ return (POSITION) m_pNodeHead; }
//_AFXCOLL_INLINE POSITION CStringList::GetTailPosition() const
//	{ return (POSITION) m_pNodeTail; }
//_AFXCOLL_INLINE CString& CStringList::GetNext(POSITION& rPosition) // return *Position++
//	{ CNode* pNode = (CNode*) rPosition;
//		ASSERT(AfxIsValidAddress(pNode, sizeof(CNode)));
//		if( pNode == NULL )
//			AfxThrowInvalidArgException();
//		rPosition = (POSITION) pNode->pNext;
//		return pNode->data; }
//_AFXCOLL_INLINE const CString& CStringList::GetNext(POSITION& rPosition) const // return *Position++
//	{ CNode* pNode = (CNode*) rPosition;
//		ASSERT(AfxIsValidAddress(pNode, sizeof(CNode)));
//		if( pNode == NULL )
//			AfxThrowInvalidArgException();
//		rPosition = (POSITION) pNode->pNext;
//		return pNode->data; }
//_AFXCOLL_INLINE CString& CStringList::GetPrev(POSITION& rPosition) // return *Position--
//	{ CNode* pNode = (CNode*) rPosition;
//		ASSERT(AfxIsValidAddress(pNode, sizeof(CNode)));
//		if( pNode == NULL )
//			AfxThrowInvalidArgException();
//		rPosition = (POSITION) pNode->pPrev;
//		return pNode->data; }
//_AFXCOLL_INLINE const CString& CStringList::GetPrev(POSITION& rPosition) const // return *Position--
//	{ CNode* pNode = (CNode*) rPosition;
//		ASSERT(AfxIsValidAddress(pNode, sizeof(CNode)));
//		if( pNode == NULL )
//			AfxThrowInvalidArgException();
//		rPosition = (POSITION) pNode->pPrev;
//		return pNode->data; }
//_AFXCOLL_INLINE CString& CStringList::GetAt(POSITION position)
//	{ CNode* pNode = (CNode*) position;
//		ASSERT(AfxIsValidAddress(pNode, sizeof(CNode)));
//		if( pNode == NULL )
//			AfxThrowInvalidArgException();
//		return pNode->data; }
//_AFXCOLL_INLINE const CString& CStringList::GetAt(POSITION position) const
//	{ CNode* pNode = (CNode*) position;
//		ASSERT(AfxIsValidAddress(pNode, sizeof(CNode)));
//		if( pNode == NULL )
//			AfxThrowInvalidArgException();
//		return pNode->data; }
//_AFXCOLL_INLINE void CStringList::SetAt(POSITION pos, LPCTSTR newElement)
//	{ CNode* pNode = (CNode*) pos;
//		ASSERT(AfxIsValidAddress(pNode, sizeof(CNode)));
//		if( pNode == NULL )
//			AfxThrowInvalidArgException();
//		pNode->data = newElement; }

//_AFXCOLL_INLINE void CStringList::SetAt(POSITION pos, const CString& newElement)
//	{ CNode* pNode = (CNode*) pos;
//		ASSERT(AfxIsValidAddress(pNode, sizeof(CNode)));
//		if( pNode == NULL )
//			AfxThrowInvalidArgException();
//		pNode->data = newElement; }



////////////////////////////////////////////////////////////////////////////

//_AFXCOLL_INLINE INT_PTR CMapPtrToPtr::GetCount() const
//	{ return m_nCount; }
//_AFXCOLL_INLINE INT_PTR CMapPtrToPtr::GetSize() const
//	{ return m_nCount; }
//_AFXCOLL_INLINE BOOL CMapPtrToPtr::IsEmpty() const
//	{ return m_nCount == 0; }
//_AFXCOLL_INLINE void CMapPtrToPtr::SetAt(void* key, void* newValue)
//	{ (*this)[key] = newValue; }
//_AFXCOLL_INLINE POSITION CMapPtrToPtr::GetStartPosition() const
//	{ return (m_nCount == 0) ? NULL : BEFORE_START_POSITION; }
//_AFXCOLL_INLINE UINT CMapPtrToPtr::GetHashTableSize() const
//	{ return m_nHashTableSize; }


/////////////////////////////////////////////////////////////////////////////

#endif //_AFXCOLL_INLINE
