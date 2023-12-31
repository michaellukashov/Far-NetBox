// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#ifndef __AFX_H__
#define __AFX_H__

#ifndef __cplusplus
	#error MFC requires C++ compilation (use a .cpp suffix)
#endif

#pragma once

#include <new>
#include <exception>

/////////////////////////////////////////////////////////////////////////////

// Since MFC itself is built with wchar_t as a native type, it will not have
// the correct type info for types built with wchar_t typedef'd to unsigned
// short.  Make sure that the user's app builds this type info in this case.
#ifndef _NATIVE_WCHAR_T_DEFINED
#define _AFX_FULLTYPEINFO
#endif

#if defined(_MFC_DLL_BLD) && defined(_DEBUG)
#ifndef _CRTDBG_MAP_ALLOC
#define _CRTDBG_MAP_ALLOC
#endif
#endif

#define __ATLTRANSACTIONMANAGER_H__

#ifndef _INC_NEW
	#include <new.h>
#endif

#include <afxver_.h>        // Target version control

#ifdef _WIN64
#ifndef _AFX_NO_DAO_SUPPORT
#define _AFX_NO_DAO_SUPPORT
#endif
#endif

#ifndef _AFX_NOFORCE_LIBS

/////////////////////////////////////////////////////////////////////////////
// Win32 libraries

#ifndef _AFXDLL
	#ifndef _UNICODE
		#ifdef _DEBUG
			#pragma comment(lib, "nafxcwd.lib")
		#else
			#pragma comment(lib, "nafxcw.lib")
		#endif
	#else
		#ifdef _DEBUG
			#pragma comment(lib, "uafxcwd.lib")
		#else
			#pragma comment(lib, "uafxcw.lib")
		#endif
	#endif
#else
	#ifndef _UNICODE
		#ifdef _DEBUG
			#pragma comment(lib, "mfc" _MFC_FILENAME_VER "d.lib")
			#pragma comment(lib, "mfcs" _MFC_FILENAME_VER "d.lib")
		#else
			#pragma comment(lib, "mfc" _MFC_FILENAME_VER ".lib")
			#pragma comment(lib, "mfcs" _MFC_FILENAME_VER ".lib")
		#endif
	#else
		#ifdef _DEBUG
			#pragma comment(lib, "mfc" _MFC_FILENAME_VER "ud.lib")
			#pragma comment(lib, "mfcs" _MFC_FILENAME_VER "ud.lib")
		#else
			#pragma comment(lib, "mfc" _MFC_FILENAME_VER "u.lib")
			#pragma comment(lib, "mfcs" _MFC_FILENAME_VER "u.lib")
		#endif
	#endif
#endif

#ifdef _DLL
	#if !defined(_AFX_NO_DEBUG_CRT) && defined(_DEBUG)
		#pragma comment(lib, "msvcrtd.lib")
	#else
		#pragma comment(lib, "msvcrt.lib")
	#endif
#else
	#if !defined(_AFX_NO_DEBUG_CRT) && defined(_DEBUG)
		#pragma comment(lib, "libcmtd.lib")
	#else
		#pragma comment(lib, "libcmt.lib")
	#endif
#endif

#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "user32.lib")
//#pragma comment(lib, "gdi32.lib")
//#pragma comment(lib, "msimg32.lib")
//#pragma comment(lib, "comdlg32.lib")
//#pragma comment(lib, "winspool.lib")
//#pragma comment(lib, "advapi32.lib")
//#pragma comment(lib, "shell32.lib")
//#pragma comment(lib, "comctl32.lib")
//#pragma comment(lib, "shlwapi.lib")

// force inclusion of NOLIB.OBJ for /disallowlib directives
#pragma comment(linker, "/include:__afxForceEXCLUDE")

// force inclusion of DLLMODUL.OBJ for _USRDLL
#ifdef _USRDLL
#pragma comment(linker, "/include:__afxForceUSRDLL")
#endif

// force inclusion of STDAFX.OBJ for precompiled types
#ifdef _AFXDLL
#pragma comment(linker, "/include:__afxForceSTDAFX")
#endif

#endif //!_AFX_NOFORCE_LIBS

#ifdef _MANAGED

#ifndef AFX_NO_CLR_COINIT_STA
#pragma comment(linker, "/CLRTHREADATTRIBUTE:STA")
#endif 

#endif //_MANAGED
/////////////////////////////////////////////////////////////////////////////
// Classes declared in this file
//   in addition to standard primitive data types and various helper macros

struct CRuntimeClass;          // object type information

class CObject;                        // the root of all objects classes

	class CException;                 // the root of all exceptions
	class CFile;                      // raw binary file
		class CStdioFile;             // buffered stdio text/binary file
		class CMemFile;               // memory based file
//		class CFileException;         // file exception
//		class CSimpleException;
//			class CMemoryException;       // out-of-memory exception
//			class CNotSupportedException; // feature not supported exception
//			class CInvalidArgException;	  // one of the parameters to the function is invalid

// Non CObject classes
struct CFileStatus;                   // file status information
struct CMemoryState;                  // diagnostic memory support

/////////////////////////////////////////////////////////////////////////////
// Other includes from standard "C" runtimes

#ifndef _INC_STRING
	#include <string.h>
#endif
#ifndef _INC_STDIO
	#include <stdio.h>
#endif
#ifndef _INC_STDLIB
	#include <stdlib.h>
#endif
#ifndef _INC_TIME
	#include <time.h>
#endif
#ifndef _INC_LIMITS
	#include <limits.h>
#endif
#ifndef _INC_STDDEF
	#include <stddef.h>
#endif
#ifndef _INC_STDARG
	#include <stdarg.h>
#endif
#ifndef _INC_ERRNO 
#include <errno.h>
#endif

#include <malloc.h>

#ifndef _AFX_NO_DEBUG_CRT
#ifndef _INC_CRTDBG
	#include <crtdbg.h>
#endif
#endif // _AFX_NO_DEBUG_CRT

#ifdef _AFX_OLD_EXCEPTIONS
#error MFC no longer supports setjmp/longjmp exception handling.
#endif

#ifdef _AFX_PACKING
#pragma pack(push, _AFX_PACKING)
#endif

/////////////////////////////////////////////////////////////////////////////
// Basic types
// abstract iteration position
struct __POSITION {};
typedef __POSITION* POSITION;

// Standard constants
#undef FALSE
#undef TRUE
#undef NULL

#define FALSE   0
#define TRUE    1
#ifndef NULL
#define NULL    0
#endif

/////////////////////////////////////////////////////////////////////////////
// _AFX_FUNCNAME definition
#ifdef UNICODE
#define _AFX_FUNCNAME(_Name) _Name##W
#else
#define _AFX_FUNCNAME(_Name) _Name##A
#endif

/////////////////////////////////////////////////////////////////////////////
// Turn off warnings for /W4
// To resume any of these warning: #pragma warning(default: 4xxx)
// which should be placed after the AFX include files

#pragma warning(disable: 4505)  // unreferenced local function has been removed
#pragma warning(disable: 4511)  // private copy constructors are good to have
#pragma warning(disable: 4512)  // private operator= are good to have
#pragma warning(disable: 4514)  // unreferenced inlines are common
#pragma warning(disable: 4710)  // function not inlined
#pragma warning(disable: 4127)  // constant expression used in macros do while(0);

// warnings specific to _AFXDLL version
#ifdef _AFXDLL
#pragma warning(disable: 4275)  // deriving exported class from non-exported
#pragma warning(disable: 4251)  // using non-exported as public in exported
#endif

#ifdef _AFX_ALL_WARNINGS
#pragma warning( push )
#endif

// warnings generated with common MFC/Windows code
#pragma warning(disable: 4201)  // nameless unions are part of C++
#pragma warning(disable: 4191)  // pointer-to-function casting
// warnings caused by normal optimizations
#ifndef _DEBUG
#pragma warning(disable: 4701)  // local variable *may* be used without init
#pragma warning(disable: 4702)  // unreachable code caused by optimizations
#pragma warning(disable: 4189)  // initialized but unused variable
#pragma warning(disable: 4390)  // empty controlled statement
#endif
// warnings specific to _AFXDLL version
#ifdef _AFXDLL
#pragma warning(disable: 4204)  // non-constant aggregate initializer
#endif
#pragma warning(disable: 4263 4264)  // base class method is hidden

/////////////////////////////////////////////////////////////////////////////
// Diagnostic support

#ifdef _DEBUG

BOOL AFXAPI AfxAssertFailedLine(LPCSTR lpszFileName, int nLine);

void AFX_CDECL AfxTrace(LPCTSTR lpszFormat, ...);

#include <atltrace.h>

// extern ATL::CTrace TRACE;
#define TRACE ATLTRACE

#define THIS_FILE          __FILE__
#define VERIFY(f)          ASSERT(f)
#define DEBUG_ONLY(f)      (f)

// The following trace macros are provided for backward compatiblity
//  (they also take a fixed number of parameters which provides
//   some amount of extra error checking)
#define TRACE0(sz)              TRACE(_T("%s"), _T(sz))
#define TRACE1(sz, p1)          TRACE(_T(sz), p1)
#define TRACE2(sz, p1, p2)      TRACE(_T(sz), p1, p2)
#define TRACE3(sz, p1, p2, p3)  TRACE(_T(sz), p1, p2, p3)

// These AFX_DUMP macros also provided for backward compatibility
#define AFX_DUMP0(dc, sz)   dc << _T(sz)
#define AFX_DUMP1(dc, sz, p1) dc << _T(sz) << p1

#else   // _DEBUG

#define VERIFY(f)          ((void)(f))
#define DEBUG_ONLY(f)      ((void)0)
#pragma warning(push)
#pragma warning(disable : 4793)
inline void AFX_CDECL AfxTrace(...) { }
#pragma warning(pop)
#define TRACE              __noop
#define TRACE0(sz)
#define TRACE1(sz, p1)
#define TRACE2(sz, p1, p2)
#define TRACE3(sz, p1, p2, p3)

#endif // !_DEBUG

#define ASSERT(f)          DEBUG_ONLY((void) ((f) || !::AfxAssertFailedLine(THIS_FILE, __LINE__) || (AfxDebugBreak(), 0)))
/* see ATL headers for commentary on this */
/* We use the name AFXASSUME to avoid name clashes */

#if defined(_PREFAST_) || defined (_DEBUG)
#define AFXASSUME(cond)			do { bool __afx_condVal=!!(cond); ASSERT(__afx_condVal); __analysis_assume(__afx_condVal); } while(0) 
#else
#define AFXASSUME(cond)			((void)0)
#endif

#define ASSERT_VALID(pOb)  

// Debug ASSERTs then throws. Retail throws if condition not met
#define ENSURE_THROW(cond, exception)	\
	do { int __afx_condVal=!!(cond); ASSERT(__afx_condVal); if (!(__afx_condVal)){exception;} } while (false)
#define ENSURE(cond)		ENSURE_THROW(cond, ::AfxThrowInvalidArgException() )
#define ENSURE_ARG(cond)	ENSURE_THROW(cond, ::AfxThrowInvalidArgException() )

// Debug ASSERT_VALIDs then throws. Retail throws if pOb is NULL
#define ENSURE_VALID_THROW(pOb, exception)	\
	do { ASSERT_VALID(pOb); if (!(pOb)){exception;} } while (false)
#define ENSURE_VALID(pOb)	ENSURE_VALID_THROW(pOb, ::AfxThrowInvalidArgException() )

#define ASSERT_POINTER(p, type) \
	ASSERT(((p) != NULL) && AfxIsValidAddress((p), sizeof(type), FALSE))

#define ASSERT_NULL_OR_POINTER(p, type) \
	ASSERT(((p) == NULL) || AfxIsValidAddress((p), sizeof(type), FALSE))

#ifdef _DEBUG
#define UNUSED(x) (void)(x)
#else
#define UNUSED(x) UNREFERENCED_PARAMETER(x)
#endif
#define UNUSED_ALWAYS(x) UNREFERENCED_PARAMETER(x)

#ifdef _DEBUG
#define REPORT_EXCEPTION(pException, szMsg) \
	do { \
		ASSERT(FALSE); \
	} while (0)
#else
#define REPORT_EXCEPTION(pException, szMsg) \
	do { \
		CString strMsg; \
		TCHAR  szErrorMessage[512]; \
		if (pException->GetErrorMessage(szErrorMessage, sizeof(szErrorMessage)/sizeof(*szErrorMessage), 0)) \
			strMsg.Format(_T("%s (%s:%d)\n%s"), szMsg, _T(__FILE__), __LINE__, szErrorMessage); \
		else \
			strMsg.Format(_T("%s (%s:%d)"), szMsg, _T(__FILE__), __LINE__); \
	} while (0)
#endif

#define EXCEPTION_IN_DTOR(pException) \
	do { \
		REPORT_EXCEPTION((pException), _T("Exception thrown in destructor")); \
		delete pException; \
	} while (0)
	
#define AFX_BEGIN_DESTRUCTOR try {
#define AFX_END_DESTRUCTOR   } catch (CException *pException) { EXCEPTION_IN_DTOR(pException); }

/////////////////////////////////////////////////////////////////////////////
// Other implementation helpers

#define BEFORE_START_POSITION ((POSITION)-1L)

/////////////////////////////////////////////////////////////////////////////
// explicit initialization for general purpose classes

BOOL AFXAPI AfxInitialize(BOOL bDLL = FALSE, DWORD dwVersion = _MFC_VER);

#undef AFX_DATA
#define AFX_DATA AFX_CORE_DATA

/////////////////////////////////////////////////////////////////////////////
// Basic object model

// generate static object constructor for class registration
void AFXAPI AfxClassInit(CRuntimeClass* pNewClass);
struct AFX_CLASSINIT
	{ AFX_CLASSINIT(CRuntimeClass* pNewClass) { AfxClassInit(pNewClass); } };

struct CRuntimeClass
{
// Attributes
	LPCSTR m_lpszClassName;
	int m_nObjectSize;
	UINT m_wSchema; // schema number of the loaded class
	CObject* (PASCAL* m_pfnCreateObject)(); // NULL => abstract class
#ifdef _AFXDLL
	CRuntimeClass* (PASCAL* m_pfnGetBaseClass)();
#else
	CRuntimeClass* m_pBaseClass;
#endif

// Operations
	CObject* CreateObject();
	BOOL IsDerivedFrom(const CRuntimeClass* pBaseClass) const;

	// dynamic name lookup and creation
	static CRuntimeClass* PASCAL FromName(LPCSTR lpszClassName);
	static CRuntimeClass* PASCAL FromName(LPCWSTR lpszClassName);
	static CObject* PASCAL CreateObject(LPCSTR lpszClassName);
	static CObject* PASCAL CreateObject(LPCWSTR lpszClassName);

	// CRuntimeClass objects linked together in simple list
	CRuntimeClass* m_pNextClass;       // linked list of registered classes
	const AFX_CLASSINIT* m_pClassInit;
};

/////////////////////////////////////////////////////////////////////////////
// Standard exception throws

void __declspec(noreturn) AFXAPI AfxThrowMemoryException();
void __declspec(noreturn) AFXAPI AfxThrowNotSupportedException();
void __declspec(noreturn) AFXAPI AfxThrowInvalidArgException();
void __declspec(noreturn) AFXAPI AfxThrowArchiveException(int cause,
	LPCTSTR lpszArchiveName = NULL);
void __declspec(noreturn) AFXAPI AfxThrowFileException(int cause, LONG lOsError = -1,
	LPCTSTR lpszFileName = NULL);
void __declspec(noreturn) AFXAPI AfxThrowOleException(LONG sc);

/////////////////////////////////////////////////////////////////////////////
// CRT functions

inline errno_t AfxCrtErrorCheck(errno_t error)
{
	switch(error)
	{
	case ENOMEM:
		AfxThrowMemoryException();
		break;
	case EINVAL:
	case ERANGE:
		AfxThrowInvalidArgException();
		break;
	case STRUNCATE:
	case 0:
		break;
	default:
		AfxThrowInvalidArgException();
		break;
	}
	return error;
}

#define AFX_CRT_ERRORCHECK(expr) \
	AfxCrtErrorCheck(expr)

inline void __cdecl Afx_clearerr_s(FILE *stream)
{
	AFX_CRT_ERRORCHECK(::clearerr_s(stream));
}

/////////////////////////////////////////////////////////////////////////////
// Strings

#ifndef _OLEAUTO_H_
	typedef LPWSTR BSTR;// must (semantically) match typedef in oleauto.h
#endif

/////////////////////////////////////////////////////////////////////////////
// class CObject is the root of all compliant objects

class AFX_NOVTABLE CObject
{
public:

// Object model (types, destruction, allocation)
	virtual CRuntimeClass* GetRuntimeClass() const;
	virtual ~CObject() = 0;  // virtual destructors are necessary

	// Diagnostic allocations
	void* PASCAL operator new(size_t nSize);
	void* PASCAL operator new(size_t, void* p);
	void PASCAL operator delete(void* p);
	void PASCAL operator delete(void* p, void* pPlace);

#if defined(_DEBUG) && !defined(_AFX_NO_DEBUG_CRT)
	// for file name/line number tracking using DEBUG_NEW
	void* PASCAL operator new(size_t nSize, LPCSTR lpszFileName, int nLine);
	void PASCAL operator delete(void *p, LPCSTR lpszFileName, int nLine);
#endif

	// Disable the copy constructor and assignment by default so you will get
	//   compiler errors instead of unexpected behaviour if you pass objects
	//   by value or assign objects.
protected:
	CObject();
private:
	CObject(const CObject& objectSrc);              // no implementation
	void operator=(const CObject& objectSrc);       // no implementation

// Attributes
public:
	BOOL IsSerializable() const;
	BOOL IsKindOf(const CRuntimeClass* pClass) const;

// Implementation
public:
	static const CRuntimeClass classCObject;
#ifdef _AFXDLL
	static CRuntimeClass* PASCAL _GetBaseClass();
	static CRuntimeClass* PASCAL GetThisClass();
#endif
};

// Helper macros
#define _RUNTIME_CLASS(class_name) ((CRuntimeClass*)(&class_name::class##class_name))
#ifdef _AFXDLL
#define RUNTIME_CLASS(class_name) (class_name::GetThisClass())
#else
#define RUNTIME_CLASS(class_name) _RUNTIME_CLASS(class_name)
#endif
#define ASSERT_KINDOF(class_name, object) \
	ASSERT((object)->IsKindOf(RUNTIME_CLASS(class_name)))

// RTTI helper macros/functions
const CObject* AFX_CDECL AfxDynamicDownCast(CRuntimeClass* pClass, const CObject* pObject);
CObject* AFX_CDECL AfxDynamicDownCast(CRuntimeClass* pClass, CObject* pObject);
#define DYNAMIC_DOWNCAST(class_name, object) \
	(class_name*)AfxDynamicDownCast(RUNTIME_CLASS(class_name), object)

#ifdef _DEBUG
const CObject* AFX_CDECL AfxStaticDownCast(CRuntimeClass* pClass, const CObject* pObject);
CObject* AFX_CDECL AfxStaticDownCast(CRuntimeClass* pClass, CObject* pObject);
#define STATIC_DOWNCAST(class_name, object) \
	(static_cast<class_name*>(AfxStaticDownCast(RUNTIME_CLASS(class_name), object)))
#else
#define STATIC_DOWNCAST(class_name, object) (static_cast<class_name*>(object))
#endif

//////////////////////////////////////////////////////////////////////////////
// Helper macros for declaring CRuntimeClass compatible classes

#ifdef _AFXDLL
#define DECLARE_DYNAMIC(class_name) \
protected: \
	static CRuntimeClass* PASCAL _GetBaseClass(); \
public: \
	static const CRuntimeClass class##class_name; \
	static CRuntimeClass* PASCAL GetThisClass(); \
	virtual CRuntimeClass* GetRuntimeClass() const; \

#define _DECLARE_DYNAMIC(class_name) \
protected: \
	static CRuntimeClass* PASCAL _GetBaseClass(); \
public: \
	static CRuntimeClass class##class_name; \
	static CRuntimeClass* PASCAL GetThisClass(); \
	virtual CRuntimeClass* GetRuntimeClass() const; \

#else
#define DECLARE_DYNAMIC(class_name) \
public: \
	static const CRuntimeClass class##class_name; \
	virtual CRuntimeClass* GetRuntimeClass() const; \

#define _DECLARE_DYNAMIC(class_name) \
public: \
	static CRuntimeClass class##class_name; \
	virtual CRuntimeClass* GetRuntimeClass() const; \

#endif

// not serializable, but dynamically constructable
#define DECLARE_DYNCREATE(class_name) \
	DECLARE_DYNAMIC(class_name) \
	static CObject* PASCAL CreateObject();

#define _DECLARE_DYNCREATE(class_name) \
	_DECLARE_DYNAMIC(class_name) \
	static CObject* PASCAL CreateObject();

#define DECLARE_SERIAL(class_name) \
	_DECLARE_DYNCREATE(class_name)

#ifdef _AFXDLL
#define IMPLEMENT_RUNTIMECLASS(class_name, base_class_name, wSchema, pfnNew, class_init) \
	CRuntimeClass* PASCAL class_name::_GetBaseClass() \
		{ return RUNTIME_CLASS(base_class_name); } \
	AFX_COMDAT const CRuntimeClass class_name::class##class_name = { \
		#class_name, sizeof(class class_name), wSchema, pfnNew, \
			&class_name::_GetBaseClass, NULL, class_init }; \
	CRuntimeClass* PASCAL class_name::GetThisClass() \
		{ return _RUNTIME_CLASS(class_name); } \
	CRuntimeClass* class_name::GetRuntimeClass() const \
		{ return _RUNTIME_CLASS(class_name); }

#define _IMPLEMENT_RUNTIMECLASS(class_name, base_class_name, wSchema, pfnNew, class_init) \
	CRuntimeClass* PASCAL class_name::_GetBaseClass() \
		{ return RUNTIME_CLASS(base_class_name); } \
	AFX_COMDAT CRuntimeClass class_name::class##class_name = { \
		#class_name, sizeof(class class_name), wSchema, pfnNew, \
			&class_name::_GetBaseClass, NULL, class_init }; \
	CRuntimeClass* PASCAL class_name::GetThisClass() \
		{ return _RUNTIME_CLASS(class_name); } \
	CRuntimeClass* class_name::GetRuntimeClass() const \
		{ return _RUNTIME_CLASS(class_name); }

#else
#define IMPLEMENT_RUNTIMECLASS(class_name, base_class_name, wSchema, pfnNew, class_init) \
	AFX_COMDAT const CRuntimeClass class_name::class##class_name = { \
		#class_name, sizeof(class class_name), wSchema, pfnNew, \
			RUNTIME_CLASS(base_class_name), NULL, class_init }; \
	CRuntimeClass* class_name::GetRuntimeClass() const \
		{ return RUNTIME_CLASS(class_name); }

#define _IMPLEMENT_RUNTIMECLASS(class_name, base_class_name, wSchema, pfnNew, class_init) \
	AFX_COMDAT CRuntimeClass class_name::class##class_name = { \
		#class_name, sizeof(class class_name), wSchema, pfnNew, \
			RUNTIME_CLASS(base_class_name), NULL, class_init }; \
	CRuntimeClass* class_name::GetRuntimeClass() const \
		{ return RUNTIME_CLASS(class_name); }

#endif

#define IMPLEMENT_DYNAMIC(class_name, base_class_name) \
	IMPLEMENT_RUNTIMECLASS(class_name, base_class_name, 0xFFFF, NULL, NULL)

#define IMPLEMENT_DYNCREATE(class_name, base_class_name) \
	CObject* PASCAL class_name::CreateObject() \
		{ return new class_name; } \
	IMPLEMENT_RUNTIMECLASS(class_name, base_class_name, 0xFFFF, \
		class_name::CreateObject, NULL)

#define IMPLEMENT_SERIAL(class_name, base_class_name, wSchema) \
	CObject* PASCAL class_name::CreateObject() \
		{ return new class_name; } \
	extern AFX_CLASSINIT _init_##class_name; \
	_IMPLEMENT_RUNTIMECLASS(class_name, base_class_name, wSchema, \
		class_name::CreateObject, &_init_##class_name) \
	AFX_CLASSINIT _init_##class_name(RUNTIME_CLASS(class_name)); \

// optional bit for schema number that enables object versioning
#define VERSIONABLE_SCHEMA  (0x80000000)

/////////////////////////////////////////////////////////////////////////////
// Exceptions

class AFX_NOVTABLE CException : public CObject
{
	// abstract class for dynamic type checking
	DECLARE_DYNAMIC(CException)

public:
// Constructors
	CException();   // sets m_bAutoDelete = TRUE
	explicit CException(BOOL bAutoDelete);   // sets m_bAutoDelete = bAutoDelete

// Operations
	void Delete();  // use to delete exception in 'catch' block

	virtual BOOL GetErrorMessage(_Out_z_cap_(nMaxError) LPTSTR lpszError, _In_ UINT nMaxError,
		_Out_opt_ PUINT pnHelpContext = NULL) const ;
	virtual BOOL GetErrorMessage(_Out_z_cap_(nMaxError) LPTSTR lpszError, _In_ UINT nMaxError,
		_Out_opt_ PUINT pnHelpContext = NULL);
//	virtual int ReportError(UINT nType = MB_OK, UINT nMessageID = 0);

// Implementation (setting m_bAutoDelete to FALSE is advanced)
public:
	virtual ~CException() = 0;
	BOOL m_bAutoDelete;
#ifdef _DEBUG
	void PASCAL operator delete(void* pbData);
	void PASCAL operator delete(void* pbData, LPCSTR lpszFileName, int nLine);
protected:
	BOOL m_bReadyForDelete;
#endif
};

#include <afxstr.h>

// ATL Classes

class CSimpleException : public CException
{
	DECLARE_DYNAMIC(CSimpleException)
	
	// base class for resource-critical MFC exceptions
	// handles ownership and initialization of an error message

public:
// Constructors
	CSimpleException();
	explicit CSimpleException(BOOL bAutoDelete);

// Operations
	virtual BOOL GetErrorMessage(_Out_z_cap_(nMaxError) LPTSTR lpszError, _In_ UINT nMaxError,
		_Out_opt_ PUINT pnHelpContext = NULL) const;

// Implementation (setting m_bAutoDelete to FALSE is advanced)
public:
	virtual ~CSimpleException() = 0;
	BOOL m_bAutoDelete;

	void InitString();      // used during MFC initialization

protected:
	BOOL m_bInitialized;
	BOOL m_bLoaded;
	TCHAR m_szMessage[128];
	UINT m_nResourceID;

#ifdef _DEBUG
	BOOL m_bReadyForDelete;
#endif
};

// helper routines for non-C++ EH implementations
	// for THROW_LAST auto-delete backward compatiblity
	void AFXAPI AfxThrowLastCleanup();

// other out-of-line helper functions
void AFXAPI AfxTryCleanup();

#ifndef _AFX_JUMPBUF
// Use portable 'jmp_buf' defined by ANSI by default.
#define _AFX_JUMPBUF jmp_buf
#endif

// Placed on frame for EXCEPTION linkage, or CException cleanup
struct AFX_EXCEPTION_LINK
{
	AFX_EXCEPTION_LINK* m_pLinkPrev;    // previous top, next in handler chain
	CException* m_pException;   // current exception (NULL in TRY block)

	AFX_EXCEPTION_LINK();       // for initialization and linking
	~AFX_EXCEPTION_LINK()       // for cleanup and unlinking
		{ AfxTryCleanup(); };
};

// Exception global state - never access directly
struct AFX_EXCEPTION_CONTEXT
{
	AFX_EXCEPTION_LINK* m_pLinkTop;

	// Note: most of the exception context is now in the AFX_EXCEPTION_LINK
};

#ifndef _PNH_DEFINED
typedef int (__cdecl * _PNH)( size_t );
#define _PNH_DEFINED
#endif

_PNH AFXAPI AfxGetNewHandler();
_PNH AFXAPI AfxSetNewHandler(_PNH pfnNewHandler);
int AFX_CDECL AfxNewHandler(size_t nSize);

void AFXAPI AfxAbort();


/////////////////////////////////////////////////////////////////////////////
// Exception macros using try, catch and throw
//  (for backward compatibility to previous versions of MFC)

#define TRY { AFX_EXCEPTION_LINK _afxExceptionLink; try {

#define CATCH(class, e) } catch (class* e) \
	{ ASSERT(e->IsKindOf(RUNTIME_CLASS(class))); \
		_afxExceptionLink.m_pException = e;

#define AND_CATCH(class, e) } catch (class* e) \
	{ ASSERT(e->IsKindOf(RUNTIME_CLASS(class))); \
		_afxExceptionLink.m_pException = e;

#define END_CATCH } }

#define THROW(e) throw e
#define THROW_LAST() (AfxThrowLastCleanup(), throw)

// Advanced macros for smaller code
#define CATCH_ALL(e) } catch (CException* e) \
	{ { ASSERT(e->IsKindOf(RUNTIME_CLASS(CException))); \
		_afxExceptionLink.m_pException = e;

#define AND_CATCH_ALL(e) } catch (CException* e) \
	{ { ASSERT(e->IsKindOf(RUNTIME_CLASS(CException))); \
		_afxExceptionLink.m_pException = e;

#define END_CATCH_ALL } } }

#define END_TRY } catch (CException* e) \
	{ ASSERT(e->IsKindOf(RUNTIME_CLASS(CException))); \
		_afxExceptionLink.m_pException = e; } }


/////////////////////////////////////////////////////////////////////////////
// Standard Exception classes

class CMemoryException : public CSimpleException
{
	DECLARE_DYNAMIC(CMemoryException)
public:
	CMemoryException();

// Implementation
public:
	explicit CMemoryException(BOOL bAutoDelete);
	CMemoryException(BOOL bAutoDelete, UINT nResourceID);
	virtual ~CMemoryException();
};

class CNotSupportedException : public CSimpleException
{
	DECLARE_DYNAMIC(CNotSupportedException)
public:
	CNotSupportedException();

// Implementation
public:
	explicit CNotSupportedException(BOOL bAutoDelete);
	CNotSupportedException(BOOL bAutoDelete, UINT nResourceID);
	virtual ~CNotSupportedException();
};
class CInvalidArgException : public CSimpleException
{
	DECLARE_DYNAMIC(CInvalidArgException)
public:
	CInvalidArgException();

// Implementation
public:
	CInvalidArgException(BOOL bAutoDelete);
	CInvalidArgException(BOOL bAutoDelete, UINT nResourceID);
	virtual ~CInvalidArgException();
};

class CArchiveException : public CException
{
	DECLARE_DYNAMIC(CArchiveException)
public:
	enum {
		none,
		genericException,
		readOnly,
		endOfFile,
		writeOnly,
		badIndex,
		badClass,
		badSchema,
		bufferFull
	};

// Constructor
	/* explicit */ CArchiveException(int cause = CArchiveException::none,
		LPCTSTR lpszArchiveName = NULL);

// Attributes
	int m_cause;
	CString m_strFileName;

// Implementation
public:
	virtual ~CArchiveException();
	virtual BOOL GetErrorMessage(_Out_z_cap_(nMaxError) LPTSTR lpszError, _In_ UINT nMaxError,
		_Out_opt_ PUINT pnHelpContext = NULL) const;
};

class CFileException : public CException
{
	DECLARE_DYNAMIC(CFileException)

public:
	enum {
		none,
		genericException,
		fileNotFound,
		badPath,
		tooManyOpenFiles,
		accessDenied,
		invalidFile,
		removeCurrentDir,
		directoryFull,
		badSeek,
		hardIO,
		sharingViolation,
		lockViolation,
		diskFull,
		endOfFile
	};

// Constructor
	/* explicit */ CFileException(int cause = CFileException::none, LONG lOsError = -1,
		LPCTSTR lpszArchiveName = NULL);

// Attributes
	int     m_cause;
	LONG    m_lOsError;
	CString m_strFileName;

// Operations
	// convert a OS dependent error code to a Cause
	static int PASCAL OsErrorToException(LONG lOsError);
	static int PASCAL ErrnoToException(int nErrno);

	// helper functions to throw exception after converting to a Cause
	static void PASCAL ThrowOsError(LONG lOsError, LPCTSTR lpszFileName = NULL);
	static void PASCAL ThrowErrno(int nErrno, LPCTSTR lpszFileName = NULL);

// Implementation
public:
	virtual ~CFileException();
	virtual BOOL GetErrorMessage(_Out_z_cap_(nMaxError) LPTSTR lpszError, _In_ UINT nMaxError,
		_Out_opt_ PUINT pnHelpContext = NULL) const;
};

/////////////////////////////////////////////////////////////////////////////
// File - raw unbuffered disk file I/O

class CFile : public CObject
{
	DECLARE_DYNAMIC(CFile)

public:
// Flag values
	enum OpenFlags {
		modeRead =         (int) 0x00000,
		modeWrite =        (int) 0x00001,
		modeReadWrite =    (int) 0x00002,
		shareCompat =      (int) 0x00000,
		shareExclusive =   (int) 0x00010,
		shareDenyWrite =   (int) 0x00020,
		shareDenyRead =    (int) 0x00030,
		shareDenyNone =    (int) 0x00040,
		modeNoInherit =    (int) 0x00080,
		modeCreate =       (int) 0x01000,
		modeNoTruncate =   (int) 0x02000,
		typeText =         (int) 0x04000, // typeText and typeBinary are
		typeBinary =       (int) 0x08000, // used in derived classes only
		osNoBuffer =       (int) 0x10000,
		osWriteThrough =   (int) 0x20000,
		osRandomAccess =   (int) 0x40000,
		osSequentialScan = (int) 0x80000,
		};

	enum Attribute {
		normal =    0x00,
		readOnly =  0x01,
		hidden =    0x02,
		system =    0x04,
		volume =    0x08,
		directory = 0x10,
		archive =   0x20
		};

	enum SeekPosition { begin = 0x0, current = 0x1, end = 0x2 };

	static AFX_DATA const HANDLE hFileNull;

// Constructors
	CFile();

	CFile(HANDLE hFile);
	CFile(LPCTSTR lpszFileName, UINT nOpenFlags);

// Attributes
	HANDLE m_hFile;
	operator HANDLE() const;

	virtual ULONGLONG GetPosition() const;
	BOOL GetStatus(CFileStatus& rStatus) const;
	virtual CString GetFileName() const;
	virtual CString GetFilePath() const;
	virtual void SetFilePath(LPCTSTR lpszNewName);

// Operations
	virtual BOOL Open(LPCTSTR lpszFileName, UINT nOpenFlags, CFileException* pError = NULL);

	/// <summary>
	/// This static function renames the specified file.</summary>
	/// <param name="lpszOldName">The old path.</param>
	/// <param name="lpszNewName">The new path.</param>
	static void PASCAL Rename(LPCTSTR lpszOldName, LPCTSTR lpszNewName);

	/// <summary>
	/// This static function deletes the file specified by the path.</summary>
	/// <param name="lpszFileName">A string that is the path to the desired file. The path can be relative or absolute.</param>
	static void PASCAL Remove(LPCTSTR lpszFileName); 

	/// <summary>
	/// This method retrieves status information related to a given CFile object instance or a given file path.</summary>
	/// <returns> 
	/// TRUE if succeeds; otherwise FALSE.</returns>
	/// <param name="lpszFileName">A string that is the path to the desired file. The path can be relative or absolute.</param>
	/// <param name="rStatus">A reference to a user-supplied CFileStatus structure that will receive the status information.</param>
	static BOOL PASCAL GetStatus(LPCTSTR lpszFileName, CFileStatus& rStatus);

	/// <summary>
	/// Sets the status of the file associated with this file location.</summary>
	/// <param name="lpszFileName">A string that is the path to the desired file. The path can be relative or absolute.</param>
	/// <param name="rStatus">The buffer containing the new status information.</param>
	static void PASCAL SetStatus(LPCTSTR lpszFileName, const CFileStatus& status);

	ULONGLONG SeekToEnd();
	void SeekToBegin();

// Overridables
	virtual CFile* Duplicate() const;

	virtual ULONGLONG Seek(LONGLONG lOff, UINT nFrom);
	virtual void SetLength(ULONGLONG dwNewLen);
	virtual ULONGLONG GetLength() const;

	virtual UINT Read(void* lpBuf, UINT nCount);
	virtual void Write(const void* lpBuf, UINT nCount);

	virtual void LockRange(ULONGLONG dwPos, ULONGLONG dwCount);
	virtual void UnlockRange(ULONGLONG dwPos, ULONGLONG dwCount);

	virtual void Abort();
	virtual void Flush();
	virtual void Close();

// Implementation
public:
	virtual ~CFile();
	enum BufferCommand { bufferRead, bufferWrite, bufferCommit, bufferCheck };
	enum BufferFlags 
	{ 
		bufferDirect = 0x01,
		bufferBlocking = 0x02
	};
	virtual UINT GetBufferPtr(UINT nCommand, UINT nCount = 0,
		void** ppBufStart = NULL, void** ppBufMax = NULL);

protected:
	void CommonBaseInit(HANDLE hFile);
	void CommonInit(LPCTSTR lpszFileName, UINT nOpenFlags);

	BOOL m_bCloseOnDelete;
	CString m_strFileName;
};

/////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
// Local file searches

#include <atltime.h>
using ATL::CTime;
using ATL::CTimeSpan;

/////////////////////////////////////////////////////////////////////////////
// File status

struct CFileStatus
{
	CTime m_ctime;          // creation date/time of file
	CTime m_mtime;          // last modification date/time of file
	CTime m_atime;          // last access date/time of file
	ULONGLONG m_size;            // logical size of file in bytes
	BYTE m_attribute;       // logical OR of CFile::Attribute enum values
	BYTE _m_padding;        // pad the structure to a WORD
	TCHAR m_szFullName[_MAX_PATH]; // absolute path name

};

/////////////////////////////////////////////////////////////////////////////
// Diagnostic memory management routines

// Low level sanity checks for memory blocks
BOOL AFXAPI AfxIsValidAddress(const void* lp,
			UINT_PTR nBytes, BOOL bReadWrite = TRUE);
BOOL AFXAPI AfxIsValidString(LPCWSTR lpsz, int nLength = -1);
BOOL AFXAPI AfxIsValidString(LPCSTR lpsz, int nLength = -1);

// Sanity checks for ATOMs
BOOL AfxIsValidAtom(ATOM nAtom);
BOOL AfxIsValidAtom(LPCTSTR psz);

#pragma warning(push)
#pragma warning(disable: 4290) // C++ exception specification ignored except to indicate a function is not __declspec(nothrow)

#if defined(_DEBUG) && !defined(_AFX_NO_DEBUG_CRT)

// Memory tracking allocation
void* AFX_CDECL operator new(size_t nSize, LPCSTR lpszFileName, int nLine) noexcept(false);
#define DEBUG_NEW new(THIS_FILE, __LINE__)
void AFX_CDECL operator delete(void* p, LPCSTR lpszFileName, int nLine) throw();

#if !defined(__MINGW32__)
void* __cdecl operator new[](size_t nSize) noexcept(false);
#endif // #if !defined(__MINGW32__)
void* __cdecl operator new[](size_t nSize, LPCSTR lpszFileName, int nLine) noexcept(false);
void __cdecl operator delete[](void* p, LPCSTR lpszFileName, int nLine) throw();
#if defined(__MINGW32__)
void operator delete(void*) _GLIBCXX_USE_NOEXCEPT
  __attribute__((__externally_visible__));
#else
void __cdecl operator delete[](void * p) throw();
#endif

#pragma warning(pop)

void* AFXAPI AfxAllocMemoryDebug(size_t nSize, BOOL bIsObject,
	LPCSTR lpszFileName, int nLine);
void AFXAPI AfxFreeMemoryDebug(void* pbData, BOOL bIsObject);

// Dump any memory leaks since program started
BOOL AFXAPI AfxDumpMemoryLeaks();

// Return TRUE if valid memory block of nBytes
BOOL AFXAPI AfxIsMemoryBlock(const void* p, UINT nBytes,
	LONG* plRequestNumber = NULL);

// Return TRUE if memory is sane or print out what is wrong
BOOL AFXAPI AfxCheckMemory();

#define afxMemDF _crtDbgFlag

enum AfxMemDF // memory debug/diagnostic flags
{
	allocMemDF          = _CRTDBG_ALLOC_MEM_DF,         // turn on debugging allocator
	delayFreeMemDF      = _CRTDBG_DELAY_FREE_MEM_DF,         // delay freeing memory
	checkAlwaysMemDF    = _CRTDBG_CHECK_ALWAYS_DF,          // AfxCheckMemory on every alloc/free
	checkEvery16MemDF	= _CRTDBG_CHECK_EVERY_16_DF,
	checkEvery128MemDF	= _CRTDBG_CHECK_EVERY_128_DF,
	checkEvery1024MemDF	= _CRTDBG_CHECK_EVERY_1024_DF,
	checkDefaultMemDF	= _CRTDBG_CHECK_DEFAULT_DF
};

#define AfxOutputDebugString TRACE

// turn on/off tracking for a short while
BOOL AFXAPI AfxEnableMemoryTracking(BOOL bTrack);

// turn on/off memory leak dump in _AFX_DEBUG_STATE destructor
BOOL AFXAPI AfxEnableMemoryLeakDump(BOOL bDump);

// Turn on/off the global flag _afxMemoryLeakOverride. if bEnable is TRUE
// then further calls to AfxEnableMemoryTracking() wont change the current
// memory tracking state, until AfxEnableMemoryLeakOverride(BOOL bEnable)
// is called again with bEnable == FALSE.
BOOL AFXAPI AfxEnableMemoryLeakOverride(BOOL bEnable);

// Advanced initialization: for overriding default diagnostics
BOOL AFXAPI AfxDiagnosticInit(void);

// A failure hook returns whether to permit allocation
typedef BOOL (AFXAPI* AFX_ALLOC_HOOK)(size_t nSize, BOOL bObject, LONG lRequestNumber);

// Set new hook, return old (never NULL)
AFX_ALLOC_HOOK AFXAPI AfxSetAllocHook(AFX_ALLOC_HOOK pfnAllocHook);

// Debugger hook on specified allocation request - Obsolete
void AFXAPI AfxSetAllocStop(LONG lRequestNumber);

// Memory state for snapshots/leak detection
struct CMemoryState
{
// Attributes
	enum blockUsage
	{
		freeBlock,    // memory not used
		objectBlock,  // contains a CObject derived class object
		bitBlock,     // contains ::operator new data
		crtBlock,
		ignoredBlock,
		nBlockUseMax  // total number of usages
	};

	_CrtMemState m_memState;
	LONG_PTR m_lCounts[nBlockUseMax];
	LONG_PTR m_lSizes[nBlockUseMax];
	LONG_PTR m_lHighWaterCount;
	LONG_PTR m_lTotalCount;

	CMemoryState();

// Operations
	void Checkpoint();  // fill with current state
	BOOL Difference(const CMemoryState& oldState,
					const CMemoryState& newState);  // fill with difference
	void UpdateData();

	// Output to afxDump
	void DumpStatistics() const;
	void DumpAllObjectsSince() const;
};

// Enumerate allocated objects or runtime classes
void AFXAPI AfxDoForAllObjects(void (AFX_CDECL *pfn)(CObject* pObject, void* pContext),
	void* pContext);
void AFXAPI AfxDoForAllClasses(void (AFX_CDECL *pfn)(const CRuntimeClass* pClass,
	void* pContext), void* pContext);

#else

// non-_DEBUG_ALLOC version that assume everything is OK
#define DEBUG_NEW new
#define AfxCheckMemory() TRUE
#define AfxIsMemoryBlock(p, nBytes) TRUE
#define AfxEnableMemoryTracking(bTrack) FALSE
#define AfxEnableMemoryLeakOverride(bEnable) TRUE
#define AfxOutputDebugString(lpsz) ::OutputDebugString(lpsz)

// diagnostic initialization
#ifndef _DEBUG
#define AfxDiagnosticInit() TRUE
#else
BOOL AFXAPI AfxDiagnosticInit(void);
#endif

#endif // _DEBUG

/////////////////////////////////////////////////////////////////////////////
// Archives for serializing CObject data

// needed for implementation
template<class TYPE, class ARG_TYPE>
class CArray;
class CPtrArray;
class CMapPtrToPtr;

/////////////////////////////////////////////////////////////////////////////
// Diagnostic dumping

// Note: AfxDumpStack is available in release builds, although it is always
//      statically linked so as to not negatively affect the size of MFCXX.DLL.

#define AFX_STACK_DUMP_TARGET_TRACE                     0x0001
#define AFX_STACK_DUMP_TARGET_CLIPBOARD 0x0002
#define AFX_STACK_DUMP_TARGET_BOTH                      0x0003
#define AFX_STACK_DUMP_TARGET_ODS                       0x0004
#ifdef _DEBUG
#define AFX_STACK_DUMP_TARGET_DEFAULT           AFX_STACK_DUMP_TARGET_TRACE
#else
#define AFX_STACK_DUMP_TARGET_DEFAULT           AFX_STACK_DUMP_TARGET_CLIPBOARD
#endif

/////////////////////////////////////////////////////////////////////////////
int __cdecl _AfxInitManaged();

#ifdef _DEBUG
#define AFXDUMP( exp ) (void)(afxDump<<exp)
#else
#define AFXDUMP( exp )
#endif

/////////////////////////////////////////////////////////////////////////////

#ifdef _AFX_PACKING
#pragma pack(pop)
#endif

#ifndef __AFXCOLL_H__
	#include <afxcoll.h>
	#ifndef __AFXSTATE_H__
		#include <afxstat_.h> // for _AFX_APP_STATE and _AFX_THREAD_STATE
	#endif
#endif

/////////////////////////////////////////////////////////////////////////////
// Inline function declarations

#ifdef _AFX_ENABLE_INLINES
#define _AFX_INLINE AFX_INLINE

#if !defined(_AFX_CORE_IMPL) || !defined(_AFXDLL) || defined(_DEBUG)
#define _AFX_PUBLIC_INLINE AFX_INLINE
#else
#define _AFX_PUBLIC_INLINE
#endif

#endif

#include <afx.inl>


#undef AFX_DATA
#define AFX_DATA

#ifdef _AFX_MINREBUILD
//#pragma component(minrebuild, on)
#endif

#ifdef _AFX_ALL_WARNINGS
#pragma warning( pop )
#endif

#endif // __AFX_H__

/////////////////////////////////////////////////////////////////////////////
