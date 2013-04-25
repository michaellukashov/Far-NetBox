// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

// Inlines for AFX.H

#ifndef AFX_H_TEMPLATE_INCLUDE_GUARD
#define AFX_H_TEMPLATE_INCLUDE_GUARD

#endif //AFX_H_TEMPLATE_INCLUDE_GUARD

#ifndef AFX_INL_MANAGED_INIT_INCLUDE_GUARD
#define AFX_INL_MANAGED_INIT_INCLUDE_GUARD
#if !defined(_AFX_MFCS) && !defined(_AFX_ISAPI)
__declspec( noinline ) inline int __cdecl  _AfxInitManaged()
{
	return 0;
}
#endif // !_AFX_MFCS && !_AFX_ISAPI
extern "C"
{
__declspec(selectany) void* _pIncludeInitManaged = (void*)_AfxInitManaged;
}
#endif  // AFX_INL_MANAGED_INIT_INCLUDE_GUARD

#ifdef _AFX_INLINE
#ifndef AFX_INL_INCLUDE_GUARD
#define AFX_INL_INCLUDE_GUARD

// CObject
_AFX_INLINE CObject::CObject()
	{ }
_AFX_INLINE CObject::~CObject()
	{ }
_AFX_INLINE void* PASCAL CObject::operator new(size_t, void* p)
	{ return p; }
#ifndef _DEBUG
// _DEBUG versions in afxmem.cpp
_AFX_INLINE void PASCAL CObject::operator delete(void* p)
	{ ::operator delete(p); }
_AFX_INLINE void PASCAL CObject::operator delete(void* p, void*)
	{ ::operator delete(p); }
_AFX_INLINE void* PASCAL CObject::operator new(size_t nSize)
	{ return ::operator new(nSize); }
// _DEBUG versions in objcore.cpp
#endif //!_DEBUG
_AFX_INLINE const CObject* AFX_CDECL AfxDynamicDownCast(CRuntimeClass* pClass, const CObject* pObject)
	{ return (const CObject*)AfxDynamicDownCast(pClass, (CObject*)pObject); }
#ifdef _DEBUG
_AFX_INLINE const CObject* AFX_CDECL AfxStaticDownCast(CRuntimeClass* pClass, const CObject* pObject)
	{ return (const CObject*)AfxStaticDownCast(pClass, (CObject*)pObject); }
#endif

// exceptions
_AFX_INLINE CException::~CException()
	{ }
_AFX_INLINE CSimpleException::CSimpleException()
	{ m_bInitialized = FALSE; m_bLoaded = FALSE; }
_AFX_INLINE CSimpleException::CSimpleException(BOOL bAutoDelete)
	: CException(bAutoDelete) { m_bInitialized = FALSE; m_bLoaded = FALSE; }
_AFX_INLINE CSimpleException::~CSimpleException()
	{ }

_AFX_INLINE CMemoryException::CMemoryException()
	: CSimpleException() { }
_AFX_INLINE CMemoryException::CMemoryException(BOOL bAutoDelete, UINT nResourceID)
	: CSimpleException(bAutoDelete) { m_nResourceID = nResourceID; }
_AFX_INLINE CMemoryException::~CMemoryException()
	{ }
_AFX_INLINE CNotSupportedException::CNotSupportedException()
	: CSimpleException() { }
_AFX_INLINE CNotSupportedException::CNotSupportedException(BOOL bAutoDelete, UINT nResourceID)
	: CSimpleException(bAutoDelete) { m_nResourceID = nResourceID; }
_AFX_INLINE CNotSupportedException::~CNotSupportedException()
	{ }
_AFX_INLINE CInvalidArgException::CInvalidArgException()
	: CSimpleException() { }
_AFX_INLINE CInvalidArgException::CInvalidArgException(BOOL bAutoDelete, UINT nResourceID)
	: CSimpleException(bAutoDelete) { m_nResourceID = nResourceID; }
_AFX_INLINE CInvalidArgException::~CInvalidArgException()
	{ }

_AFX_INLINE CFileException::CFileException(int cause, LONG lOsError,
	LPCTSTR pstrFileName /* = NULL */)
	{ m_cause = cause; m_lOsError = lOsError; m_strFileName = pstrFileName; }
_AFX_INLINE CFileException::~CFileException()
	{ }

// CFile
_AFX_INLINE CFile::operator HANDLE() const
	{ return m_hFile; }
_AFX_INLINE ULONGLONG CFile::SeekToEnd()
	{ return Seek(0, CFile::end); }
_AFX_INLINE void CFile::SeekToBegin()
	{ Seek(0, CFile::begin); }
_AFX_INLINE void CFile::SetFilePath(LPCTSTR lpszNewName)
{
	ASSERT_VALID(this);
	ASSERT(AfxIsValidString(lpszNewName));
	if(lpszNewName != NULL)
		m_strFileName = lpszNewName;
	else
		AfxThrowInvalidArgException();  

}

// CFileFind
_AFX_INLINE BOOL CFileFind::IsReadOnly() const
	{ return MatchesMask(FILE_ATTRIBUTE_READONLY); }
_AFX_INLINE BOOL CFileFind::IsDirectory() const
	{ return MatchesMask(FILE_ATTRIBUTE_DIRECTORY); }
_AFX_INLINE BOOL CFileFind::IsCompressed() const
	{ return MatchesMask(FILE_ATTRIBUTE_COMPRESSED); }
_AFX_INLINE BOOL CFileFind::IsSystem() const
	{ return MatchesMask(FILE_ATTRIBUTE_SYSTEM); }
_AFX_INLINE BOOL CFileFind::IsHidden() const
	{ return MatchesMask(FILE_ATTRIBUTE_HIDDEN); }
_AFX_INLINE BOOL CFileFind::IsTemporary() const
	{ return MatchesMask(FILE_ATTRIBUTE_TEMPORARY); }
_AFX_INLINE BOOL CFileFind::IsNormal() const
	{ return MatchesMask(FILE_ATTRIBUTE_NORMAL); }
_AFX_INLINE BOOL CFileFind::IsArchived() const
	{ return MatchesMask(FILE_ATTRIBUTE_ARCHIVE); }

/////////////////////////////////////////////////////////////////////////////
#endif //AFX_INL_INCLUDE_GUARD
#endif //_AFX_INLINE
