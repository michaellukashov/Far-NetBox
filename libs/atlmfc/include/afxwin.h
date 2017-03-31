// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#ifndef __AFXWIN_H__
#ifndef RC_INVOKED
#define __AFXWIN_H__

/////////////////////////////////////////////////////////////////////////////
// Make sure 'afx.h' is included first

#ifndef __AFX_H__
	#include <afx.h>
#endif

// Note: WINDOWS.H already included from AFXV_W32.H
#ifndef NTDDI_LONGHORN
#define NTDDI_LONGHORN 0x06000000
#if (WINVER >= 0x0600) || (_WIN32_WINNT >= 0x0600)
#error Your version of the Windows SDK is earlier than 6.0. Try setting the 'WINVER' and '_WIN32_WINNT' definitions in your project to less than 0x0600.
#endif
#endif

#ifndef _INC_SHELLAPI
//	#include <shellapi.h>
#endif

#ifndef __AFXRES_H__
	#include <afxres.h>     // standard resource IDs
#endif

#ifndef __AFXCOLL_H__
	#include <afxcoll.h>    // standard collections
#endif

#ifndef _OBJBASE_H_
//	#include <objbase.h> //needed for commdlg.h (STDMETHOD)
#endif

#ifndef _INC_COMMDLG
//	#include <commdlg.h>    // common dialog APIs
#endif

// #include <afxctrlcontainer.h>

#if WINVER >= 0x0600
#ifndef _WIN32_IE
#define _WIN32_IE 0x0700
#else
#undef _WIN32_IE
#define _WIN32_IE 0x0700
#endif

#ifndef __shobjidl_h__
	// #include <shobjidl.h>    // for IPreviewHandler, IPreviewHandlerVisuals
#endif

// #ifndef __IThumbnailProvider_INTERFACE_DEFINED__
	// #include <thumbcache.h>  // for IThumbnailProvider
// #endif
#endif

#ifndef _AFX_NO_AFXCMN_SUPPORT
#ifdef _AFX_PACKING
#pragma pack(push, _AFX_PACKING)
#endif
//#include <afxcomctl32.h>
#ifdef _AFX_PACKING
#pragma pack(pop)
#endif
#endif

#if (_WIN32_WINNT >= 0x501)

#if ((NTDDI_VERSION >= NTDDI_LONGHORN || defined(__VSSYM32_H__)) && !defined(SCHEMA_VERIFY_VSSYM32))
#include <vssym32.h>
#else
#endif
#endif	// (_WIN32_WINNT >= 0x501)

#if (_WIN32_WINNT >= 0x600)
#ifndef _WINSOCK2API_
#ifdef _WINSOCKAPI_
	#error MFC requires use of Winsock2.h
#endif
	#include <winsock2.h>
#endif

#ifndef _WS2IPDEF_
	#include <ws2ipdef.h>
#endif

#ifndef _WINDNS_INCLUDED_
	#include <windns.h>
#endif

#ifndef __IPHLPAPI_H__
	// #include <iphlpapi.h>
#endif
#endif	// (_WIN32_WINNT >= 0x600)

#ifdef _AFX_MINREBUILD
//#pragma component(minrebuild, off)
#endif

#ifndef _AFX_NOFORCE_LIBS
#pragma comment(lib, "uuid.lib")
#endif

#ifdef _INC_WINDOWSX
// The following names from WINDOWSX.H collide with names in this header
#undef SubclassWindow
#undef CopyRgn
#endif

#include <htmlhelp.h>

#ifdef _AFX_PACKING
#pragma pack(push, _AFX_PACKING)
#endif

#pragma warning( push )
#pragma warning( disable: 4121 )

/////////////////////////////////////////////////////////////////////////////
// Classes declared in this file

//CObject
	//CException
		//CSimpleException
			class CResourceException;// Win resource failure exception
			// class CUserException;    // Message Box alert and stop operation

//	class CDC;                   // a Display Context / HDC wrapper
//		class CClientDC;         // CDC for client of window
//		class CWindowDC;         // CDC for entire window

//	class CImageList;            // an image list / HIMAGELIST wrapper

	class CCmdTarget;            // a target for user commands
		class CWnd;                 // a window / HWND wrapper
			// class CDialog;          // a dialog

			// standard windows controls
			class CStatic;          // Static control

		class CWinThread;           // thread base class
			class CWinApp;          // application base class

		class CDocTemplate;         // template for document creation
			class CSingleDocTemplate;// SDI support
			class CMultiDocTemplate; // MDI support

		class CMFCFilterChunkValueImpl; // search/organize/preview/thumbnail support - filter chunk value implementation


// Helper classes
class CCmdUI;           // Menu/button enabling
class CDataExchange;    // Data exchange and validation context
class CCommandLineInfo; // CommandLine parsing helper

struct COleControlSiteOrWnd; // ActiveX dialog control helper


class CControlCreationInfo; //Used in CWnd::CreateControl overloads.

class CVariantBoolConverter;

/////////////////////////////////////////////////////////////////////////////

enum AFX_HELP_TYPE
{
	afxWinHelp = 0,
	afxHTMLHelp = 1
};

// Type modifier for message handlers
#ifndef afx_msg
#define afx_msg         // intentional placeholder
#endif

#undef AFX_DATA
#define AFX_DATA AFX_CORE_DATA


#ifndef __ATLTYPES_H__
#include <atltypes.h>
#endif

// macro to be used in ATL search/organize/preview/thumbnail handlers with MFC document support
#define DECLARE_DOCUMENT(classDocument)\
protected:\
	virtual IDocument* CreateDocument()\
	{\
		CRuntimeClass* pDocRTC = RUNTIME_CLASS(classDocument);\
		if (pDocRTC == NULL)\
		{\
			TRACE("Document class does not support dynamic creation."); \
			return NULL;\
		}\
		classDocument* pDoc = DYNAMIC_DOWNCAST(classDocument, pDocRTC->CreateObject());\
		ASSERT_VALID(pDoc);\
		return pDoc->GetAdapter();\
	}



/////////////////////////////////////////////////////////////////////////////
// Standard exceptions

class CResourceException : public CSimpleException    // resource failure
{
	DECLARE_DYNAMIC(CResourceException)
public:
	CResourceException();

// Implementation
public:
	explicit CResourceException(BOOL bAutoDelete);
	CResourceException(BOOL bAutoDelete, UINT nResourceID);
	virtual ~CResourceException();
};

class CUserException : public CSimpleException   // general user visible alert
{
	DECLARE_DYNAMIC(CUserException)
public:
	CUserException();

// Implementation
public:
	explicit CUserException(BOOL bAutoDelete);
	CUserException(BOOL bAutoDelete, UINT nResourceID);
	virtual ~CUserException();
};

void AFXAPI AfxThrowResourceException();
void AFXAPI AfxThrowUserException();

/////////////////////////////////////////////////////////////////////////////
// Window message map handling

struct AFX_MSGMAP_ENTRY;       // declared below after CWnd

struct AFX_MSGMAP
{
	const AFX_MSGMAP* (PASCAL* pfnGetBaseMap)();
	const AFX_MSGMAP_ENTRY* lpEntries;
};


#define DECLARE_MESSAGE_MAP() \
protected: \
	static const AFX_MSGMAP* PASCAL GetThisMessageMap(); \
	virtual const AFX_MSGMAP* GetMessageMap() const; \

#define BEGIN_TEMPLATE_MESSAGE_MAP(theClass, type_name, baseClass)			\
	PTM_WARNING_DISABLE														\
	template < typename type_name >											\
	const AFX_MSGMAP* theClass< type_name >::GetMessageMap() const			\
		{ return GetThisMessageMap(); }										\
	template < typename type_name >											\
	const AFX_MSGMAP* PASCAL theClass< type_name >::GetThisMessageMap()		\
	{																		\
		typedef theClass< type_name > ThisClass;							\
		typedef baseClass TheBaseClass;										\
		static const AFX_MSGMAP_ENTRY _messageEntries[] =					\
		{

#define BEGIN_MESSAGE_MAP(theClass, baseClass) \
	PTM_WARNING_DISABLE \
	const AFX_MSGMAP* theClass::GetMessageMap() const \
		{ return GetThisMessageMap(); } \
	const AFX_MSGMAP* PASCAL theClass::GetThisMessageMap() \
	{ \
		typedef theClass ThisClass;						   \
		typedef baseClass TheBaseClass;					   \
		static const AFX_MSGMAP_ENTRY _messageEntries[] =  \
		{

#define END_MESSAGE_MAP() \
		{0, 0, 0, 0, AfxSig_end, (AFX_PMSG)0 } \
	}; \
		static const AFX_MSGMAP messageMap = \
		{ &TheBaseClass::GetThisMessageMap, &_messageEntries[0] }; \
		return &messageMap; \
	}								  \
	PTM_WARNING_RESTORE


// Message map signature values and macros in separate header
#include <afxmsg_.h>

/////////////////////////////////////////////////////////////////////////////
// Dialog data exchange (DDX_) and validation (DDV_)

class COleControlSite;

// CDataExchange - for data exchange and validation
class CDataExchange
{
// Attributes
public:
	BOOL m_bSaveAndValidate;   // TRUE => save and validate data
	CWnd* m_pDlgWnd;           // container usually a dialog

// Operations (for implementors of DDX and DDV procs)
	HWND PrepareCtrl(int nIDC);
	HWND PrepareEditCtrl(int nIDC);
	void Fail();                    // will throw exception

	CDataExchange(CWnd* pDlgWnd, BOOL bSaveAndValidate);

#ifndef _AFX_NO_OCC_SUPPORT
	COleControlSite* PrepareOleCtrl(int nIDC); // for OLE controls in dialog
#endif

// Implementation
	 UINT m_idLastControl;      // last control used (for validation)
	BOOL m_bEditLastControl;   // last control was an edit item
};

#include <afxdd_.h>     // standard DDX_ and DDV_ routines

/////////////////////////////////////////////////////////////////////////////
// OLE types

//typedef LONG HRESULT;

struct IUnknown;
typedef IUnknown* LPUNKNOWN;

struct IDispatch;
typedef IDispatch* LPDISPATCH;

struct IConnectionPoint;
typedef IConnectionPoint* LPCONNECTIONPOINT;

struct IEnumOLEVERB;
typedef IEnumOLEVERB* LPENUMOLEVERB;

typedef struct _GUID GUID;
typedef GUID IID;
typedef GUID CLSID;
#ifndef _REFCLSID_DEFINED
#define REFCLSID const CLSID &
#endif

typedef long DISPID;
typedef unsigned short VARTYPE;
typedef long SCODE;

typedef WCHAR OLECHAR;
typedef OLECHAR* BSTR;

struct tagDISPPARAMS;
typedef tagDISPPARAMS DISPPARAMS;

struct tagVARIANT;
typedef tagVARIANT VARIANT;

struct ITypeInfo;
typedef ITypeInfo* LPTYPEINFO;

struct ITypeLib;
typedef ITypeLib* LPTYPELIB;

struct IAccessible;
struct IAccessibleServer;
struct IEnumVARIANT;

struct tagEXCEPINFO;
typedef tagEXCEPINFO EXCEPINFO;


/////////////////////////////////////////////////////////////////////////////
// CCmdTarget

// private structures
struct AFX_CMDHANDLERINFO;  // info about where the command is handled
struct AFX_EVENT;           // info about an event
class CTypeLibCache;        // cache for OLE type libraries

/////////////////////////////////////////////////////////////////////////////
// OLE interface map handling (more in AFXDISP.H)

#ifndef _AFX_NO_OLE_SUPPORT

struct AFX_INTERFACEMAP_ENTRY
{
	const void* piid;       // the interface id (IID) (NULL for aggregate)
	size_t nOffset;         // offset of the interface vtable from m_unknown
};

struct AFX_INTERFACEMAP
{
#ifdef _AFXDLL
	const AFX_INTERFACEMAP* (PASCAL* pfnGetBaseMap)(); // NULL is root class
#else
	const AFX_INTERFACEMAP* pBaseMap;
#endif
	const AFX_INTERFACEMAP_ENTRY* pEntry; // map for this class
};


#ifdef _AFXDLL
#define DECLARE_INTERFACE_MAP() \
private: \
	static const AFX_INTERFACEMAP_ENTRY _interfaceEntries[]; \
protected: \
	static const AFX_INTERFACEMAP interfaceMap; \
	static const AFX_INTERFACEMAP* PASCAL GetThisInterfaceMap(); \
	virtual const AFX_INTERFACEMAP* GetInterfaceMap() const; \

#else
#define DECLARE_INTERFACE_MAP() \
private: \
	static const AFX_INTERFACEMAP_ENTRY _interfaceEntries[]; \
protected: \
	static const AFX_INTERFACEMAP interfaceMap; \
	virtual const AFX_INTERFACEMAP* GetInterfaceMap() const; \

#endif

/////////////////////////////////////////////////////////////////////////////
// OLE COM (Component Object Model) implementation infrastructure
//      - data driven QueryInterface
//      - standard implementation of aggregate AddRef and Release
// (see CCmdTarget in AFXWIN.H for more information)

#define METHOD_PROLOGUE(theClass, localClass) \
	theClass* pThis = \
		((theClass*)((BYTE*)this - offsetof(theClass, m_x##localClass))); \
	AFX_MANAGE_STATE(pThis->m_pModuleState) \
	pThis; // avoid warning from compiler \

#define METHOD_PROLOGUE_(theClass, localClass) \
	theClass* pThis = \
		((theClass*)((BYTE*)this - offsetof(theClass, m_x##localClass))); \
	pThis; // avoid warning from compiler \

#ifndef _AFX_NO_NESTED_DERIVATION
#define METHOD_PROLOGUE_EX(theClass, localClass) \
	theClass* pThis = ((theClass*)((BYTE*)this - m_nOffset)); \
	AFX_MANAGE_STATE(pThis->m_pModuleState) \
	pThis; // avoid warning from compiler \

#define METHOD_PROLOGUE_EX_(theClass, localClass) \
	theClass* pThis = ((theClass*)((BYTE*)this - m_nOffset)); \
	pThis; // avoid warning from compiler \

#else
#define METHOD_PROLOGUE_EX(theClass, localClass) \
	METHOD_PROLOGUE(theClass, localClass) \

#define METHOD_PROLOGUE_EX_(theClass, localClass) \
	METHOD_PROLOGUE_(theClass, localClass) \

#endif

// Provided only for compatibility with CDK 1.x
#define METHOD_MANAGE_STATE(theClass, localClass) \
	METHOD_PROLOGUE_EX(theClass, localClass) \

#define BEGIN_INTERFACE_PART(localClass, baseClass) \
	class X##localClass : public baseClass \
	{ \
	public: \
		STDMETHOD_(ULONG, AddRef)(); \
		STDMETHOD_(ULONG, Release)(); \
		STDMETHOD(QueryInterface)(REFIID iid, LPVOID* ppvObj); \

#ifndef _AFX_NO_NESTED_DERIVATION
#define BEGIN_INTERFACE_PART_DERIVE(localClass, baseClass) \
	class X##localClass : public baseClass \
	{ \
	public: \

#else
#define BEGIN_INTERFACE_PART_DERIVE(localClass, baseClass) \
	BEGIN_INTERFACE_PART(localClass, baseClass) \

#endif

#ifndef _AFX_NO_NESTED_DERIVATION
#define INIT_INTERFACE_PART(theClass, localClass) \
		size_t m_nOffset; \
		INIT_INTERFACE_PART_DERIVE(theClass, localClass) \

#define INIT_INTERFACE_PART_DERIVE(theClass, localClass) \
		X##localClass() \
			{ m_nOffset = offsetof(theClass, m_x##localClass); } \

#else
#define INIT_INTERFACE_PART(theClass, localClass)
#define INIT_INTERFACE_PART_DERIVE(theClass, localClass)

#endif

// Note: Inserts the rest of OLE functionality between these two macros,
//  depending upon the interface that is being implemented.  It is not
//  necessary to include AddRef, Release, and QueryInterface since those
//  member functions are declared by the macro.

#define END_INTERFACE_PART(localClass) \
	} m_x##localClass; \
	friend class X##localClass; \

struct CInterfacePlaceHolder
{
	DWORD_PTR m_vtbl;   // filled in with USE_INTERFACE_PART
	CInterfacePlaceHolder() { m_vtbl = 0; }
};

#define END_INTERFACE_PART_OPTIONAL(localClass) \
	}; \
	CInterfacePlaceHolder m_x##localClass; \
	friend class X##localClass; \

#ifdef _AFXDLL
#define END_INTERFACE_PART_STATIC END_INTERFACE_PART
#else
#define END_INTERFACE_PART_STATIC END_INTERFACE_PART
#endif

#define USE_INTERFACE_PART(localClass) \
	m_x##localClass.m_vtbl = *(DWORD_PTR*)&X##localClass(); \

// To avoid C4238.
#define USE_INTERFACE_PART_STD(localClass) \
	X##localClass tmp##localClass; \
	m_x##localClass.m_vtbl = *(DWORD_PTR*)&tmp##localClass;

#ifdef _AFXDLL
#define BEGIN_INTERFACE_MAP(theClass, theBase) \
	const AFX_INTERFACEMAP* PASCAL theClass::GetThisInterfaceMap() \
		{ return &theClass::interfaceMap; } \
	const AFX_INTERFACEMAP* theClass::GetInterfaceMap() const \
		{ return &theClass::interfaceMap; } \
	AFX_COMDAT const AFX_INTERFACEMAP theClass::interfaceMap = \
		{ &theBase::GetThisInterfaceMap, &theClass::_interfaceEntries[0], }; \
	AFX_COMDAT const AFX_INTERFACEMAP_ENTRY theClass::_interfaceEntries[] = \
	{ \

#else
#define BEGIN_INTERFACE_MAP(theClass, theBase) \
	const AFX_INTERFACEMAP* theClass::GetInterfaceMap() const \
		{ return &theClass::interfaceMap; } \
	AFX_COMDAT const AFX_INTERFACEMAP theClass::interfaceMap = \
		{ &theBase::interfaceMap, &theClass::_interfaceEntries[0], }; \
	AFX_COMDAT const AFX_INTERFACEMAP_ENTRY theClass::_interfaceEntries[] = \
	{ \

#endif

#define INTERFACE_PART(theClass, iid, localClass) \
		{ &iid, offsetof(theClass, m_x##localClass) }, \

#define INTERFACE_AGGREGATE(theClass, theAggr) \
		{ NULL, offsetof(theClass, theAggr) }, \

#define END_INTERFACE_MAP() \
		{ NULL, (size_t)-1 } \
	}; \


#endif //!_AFX_NO_OLE_SUPPORT

/////////////////////////////////////////////////////////////////////////////
// OLE dispatch map handling (more in AFXDISP.H)

#ifndef _AFX_NO_OLE_SUPPORT

struct AFX_DISPMAP_ENTRY;

struct AFX_DISPMAP
{
#ifdef _AFXDLL
	const AFX_DISPMAP* (PASCAL* pfnGetBaseMap)();
#else
	const AFX_DISPMAP* pBaseMap;
#endif
	const AFX_DISPMAP_ENTRY* lpEntries;
	UINT* lpEntryCount;
	DWORD* lpStockPropMask;
};

#ifdef _AFXDLL
#define DECLARE_DISPATCH_MAP() \
private: \
	static const AFX_DISPMAP_ENTRY _dispatchEntries[]; \
	static UINT _dispatchEntryCount; \
	static DWORD _dwStockPropMask; \
protected: \
	static const AFX_DISPMAP dispatchMap; \
	static const AFX_DISPMAP* PASCAL GetThisDispatchMap(); \
	virtual const AFX_DISPMAP* GetDispatchMap() const; \

#else
#define DECLARE_DISPATCH_MAP() \
private: \
	static const AFX_DISPMAP_ENTRY _dispatchEntries[]; \
	static UINT _dispatchEntryCount; \
	static DWORD _dwStockPropMask; \
protected: \
	static const AFX_DISPMAP dispatchMap; \
	virtual const AFX_DISPMAP* GetDispatchMap() const; \

#endif

#endif //!_AFX_NO_OLE_SUPPORT

/////////////////////////////////////////////////////////////////////////////
// OLE Document Object command target handling

#ifndef _AFX_NO_DOCOBJECT_SUPPORT

struct AFX_OLECMDMAP_ENTRY
{
   const GUID* pguid;   // id of the command group
   ULONG       cmdID;   // OLECMD ID
   UINT        nID;     // corresponding WM_COMMAND message ID
};

struct AFX_OLECMDMAP
{
#ifdef _AFXDLL
	const AFX_OLECMDMAP* (PASCAL* pfnGetBaseMap)();
#else
	const AFX_OLECMDMAP* pBaseMap;
#endif
	const AFX_OLECMDMAP_ENTRY* lpEntries;
};

#ifdef _AFXDLL
#define DECLARE_OLECMD_MAP() \
private: \
	static const AFX_OLECMDMAP_ENTRY _commandEntries[]; \
protected: \
	static const AFX_OLECMDMAP commandMap; \
	static const AFX_OLECMDMAP* PASCAL GetThisCommandMap(); \
	virtual const AFX_OLECMDMAP* GetCommandMap() const; \

#else
#define DECLARE_OLECMD_MAP() \
private: \
	static const AFX_OLECMDMAP_ENTRY _commandEntries[]; \
protected: \
	static const AFX_OLECMDMAP commandMap; \
	virtual const AFX_OLECMDMAP* GetCommandMap() const; \

#endif

#ifdef _AFXDLL
#define BEGIN_OLECMD_MAP(theClass, baseClass) \
	const AFX_OLECMDMAP* PASCAL theClass::GetThisCommandMap() \
		{ return &theClass::commandMap; } \
	const AFX_OLECMDMAP* theClass::GetCommandMap() const \
		{ return &theClass::commandMap; } \
	AFX_COMDAT const AFX_OLECMDMAP theClass::commandMap = \
	{ &baseClass::GetThisCommandMap, &theClass::_commandEntries[0] }; \
	AFX_COMDAT const AFX_OLECMDMAP_ENTRY theClass::_commandEntries[] = \
	{ \

#else
#define BEGIN_OLECMD_MAP(theClass, baseClass) \
	const AFX_OLECMDMAP* theClass::GetCommandMap() const \
		{ return &theClass::commandMap; } \
	AFX_COMDAT const AFX_OLECMDMAP theClass::commandMap = \
	{ &baseClass::commandMap, &theClass::_commandEntries[0] }; \
	AFX_COMDAT const AFX_OLECMDMAP_ENTRY theClass::_commandEntries[] = \
	{ \

#endif

#define END_OLECMD_MAP() \
		{NULL, 0, 0} \
	}; \

#endif //!_AFX_NO_DOCOBJECT_SUPPORT

/////////////////////////////////////////////////////////////////////////////
// OLE event sink map handling (more in AFXDISP.H)

#ifndef _AFX_NO_OCC_SUPPORT

struct AFX_EVENTSINKMAP_ENTRY;

struct AFX_EVENTSINKMAP
{
#ifdef _AFXDLL
	const AFX_EVENTSINKMAP* (PASCAL* pfnGetBaseMap)();
#else
	const AFX_EVENTSINKMAP* pBaseMap;
#endif
	const AFX_EVENTSINKMAP_ENTRY* lpEntries;
	UINT* lpEntryCount;
};

#ifdef _AFXDLL
#define DECLARE_EVENTSINK_MAP() \
private: \
	static const AFX_EVENTSINKMAP_ENTRY _eventsinkEntries[]; \
	static UINT _eventsinkEntryCount; \
protected: \
	static const AFX_EVENTSINKMAP eventsinkMap; \
	static const AFX_EVENTSINKMAP* PASCAL GetThisEventSinkMap(); \
	virtual const AFX_EVENTSINKMAP* GetEventSinkMap() const; \

#else
#define DECLARE_EVENTSINK_MAP() \
private: \
	static const AFX_EVENTSINKMAP_ENTRY _eventsinkEntries[]; \
	static UINT _eventsinkEntryCount; \
protected: \
	static const AFX_EVENTSINKMAP eventsinkMap; \
	virtual const AFX_EVENTSINKMAP* GetEventSinkMap() const; \

#endif

#endif //!_AFX_NO_OCC_SUPPORT

/////////////////////////////////////////////////////////////////////////////
// OLE connection map handling (more in AFXDISP.H)

#ifndef _AFX_NO_OLE_SUPPORT

struct AFX_CONNECTIONMAP_ENTRY
{
	const void* piid;   // the interface id (IID)
	size_t nOffset;         // offset of the interface vtable from m_unknown
};

struct AFX_CONNECTIONMAP
{
#ifdef _AFXDLL
	const AFX_CONNECTIONMAP* (PASCAL* pfnGetBaseMap)(); // NULL is root class
#else
	const AFX_CONNECTIONMAP* pBaseMap;
#endif
	const AFX_CONNECTIONMAP_ENTRY* pEntry; // map for this class
};

#ifdef _AFXDLL
#define DECLARE_CONNECTION_MAP() \
private: \
	static const AFX_CONNECTIONMAP_ENTRY _connectionEntries[]; \
protected: \
	static const AFX_CONNECTIONMAP connectionMap; \
	static const AFX_CONNECTIONMAP* PASCAL GetThisConnectionMap(); \
	virtual const AFX_CONNECTIONMAP* GetConnectionMap() const; \

#else
#define DECLARE_CONNECTION_MAP() \
private: \
	static const AFX_CONNECTIONMAP_ENTRY _connectionEntries[]; \
protected: \
	static const AFX_CONNECTIONMAP connectionMap; \
	virtual const AFX_CONNECTIONMAP* GetConnectionMap() const; \

#endif

#endif //!_AFX_NO_OLE_SUPPORT

/////////////////////////////////////////////////////////////////////////////
// CCmdTarget proper

#ifndef _AFX_NO_OCC_SUPPORT
class COccManager;      // forward reference (see ..\src\occimpl.h)
#endif

class AFX_NOVTABLE CCmdTarget : public CObject
{
	DECLARE_DYNAMIC(CCmdTarget)
protected:

public:
// Constructors
	CCmdTarget();

// Attributes
	LPDISPATCH GetIDispatch(BOOL bAddRef);
		// retrieve IDispatch part of CCmdTarget
	static CCmdTarget* PASCAL FromIDispatch(LPDISPATCH lpDispatch);
		// map LPDISPATCH back to CCmdTarget* (inverse of GetIDispatch)
	BOOL IsResultExpected();
		// returns TRUE if automation function should return a value

// Operations
	void EnableAutomation();
		// call in constructor to wire up IDispatch
	void EnableConnections();
		// call in constructor to wire up IConnectionPointContainer

	void BeginWaitCursor();
	void EndWaitCursor();
	void RestoreWaitCursor();       // call after messagebox

#ifndef _AFX_NO_OLE_SUPPORT
	// dispatch OLE verbs through the message map
	BOOL EnumOleVerbs(LPENUMOLEVERB* ppenumOleVerb);
	BOOL DoOleVerb(LONG iVerb, LPMSG lpMsg, HWND hWndParent, LPCRECT lpRect);
#endif

// Overridables
	// route and dispatch standard command message types
	//   (more sophisticated than OnCommand)
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra,
		AFX_CMDHANDLERINFO* pHandlerInfo);

#ifndef _AFX_NO_OLE_SUPPORT
	// called when last OLE reference is released
	virtual void OnFinalRelease();
#endif

#ifndef _AFX_NO_OLE_SUPPORT
	// called before dispatching to an automation handler function
	virtual BOOL IsInvokeAllowed(DISPID dispid);
#endif

#ifndef _AFX_NO_OLE_SUPPORT
	// support for OLE type libraries
	void EnableTypeLib();
	HRESULT GetTypeInfoOfGuid(LCID lcid, const GUID& guid,
		LPTYPEINFO* ppTypeInfo);
	virtual BOOL GetDispatchIID(IID* pIID);
	virtual UINT GetTypeInfoCount();
	virtual CTypeLibCache* GetTypeLibCache();
	virtual HRESULT GetTypeLib(LCID lcid, LPTYPELIB* ppTypeLib);
#endif

// Implementation
public:
	virtual ~CCmdTarget() = 0;
#ifndef _AFX_NO_OLE_SUPPORT
	void GetNotSupported();
	void SetNotSupported();
#endif

protected:
	friend class CView;

	// CView* GetRoutingView();
	// static CView* PASCAL GetRoutingView_();
	DECLARE_MESSAGE_MAP()       // base class - no {{ }} macros

#ifndef _AFX_NO_DOCOBJECT_SUPPORT
	DECLARE_OLECMD_MAP()
#endif

#ifndef _AFX_NO_OLE_SUPPORT
	DECLARE_DISPATCH_MAP()
	DECLARE_CONNECTION_MAP()
	DECLARE_INTERFACE_MAP()

#ifndef _AFX_NO_OCC_SUPPORT
	DECLARE_EVENTSINK_MAP()
#endif // !_AFX_NO_OCC_SUPPORT

	// OLE interface map implementation
public:
	// data used when CCmdTarget is made OLE aware
	long m_dwRef;
	LPUNKNOWN m_pOuterUnknown;  // external controlling unknown if != NULL
	DWORD_PTR m_xInnerUnknown;  // place-holder for inner controlling unknown

public:
	// advanced operations
	void EnableAggregation();       // call to enable aggregation
	void ExternalDisconnect();      // forcibly disconnect
	LPUNKNOWN GetControllingUnknown();
		// get controlling IUnknown for aggregate creation

	// these versions do not delegate to m_pOuterUnknown
	DWORD InternalQueryInterface(const void*, LPVOID* ppvObj);
	DWORD InternalAddRef();
	DWORD InternalRelease();
	// these versions delegate to m_pOuterUnknown
	DWORD ExternalQueryInterface(const void*, LPVOID* ppvObj);
	DWORD ExternalAddRef();
	DWORD ExternalRelease();

	// implementation helpers
	LPUNKNOWN GetInterface(const void*);
	LPUNKNOWN QueryAggregates(const void*);

	// advanced overrideables for implementation
	virtual BOOL OnCreateAggregates();
	virtual LPUNKNOWN GetInterfaceHook(const void*);

	// OLE automation implementation
protected:
	struct XDispatch
	{
		DWORD_PTR m_vtbl;   // place-holder for IDispatch vtable
#ifndef _AFX_NO_NESTED_DERIVATION
		size_t m_nOffset;
#endif
	} m_xDispatch;
	BOOL m_bResultExpected;

	// member variable-based properties
	void GetStandardProp(const AFX_DISPMAP_ENTRY* pEntry,
		VARIANT* pvarResult, UINT* puArgErr);
	SCODE SetStandardProp(const AFX_DISPMAP_ENTRY* pEntry,
		DISPPARAMS* pDispParams, UINT* puArgErr);

	// DISPID to dispatch map lookup
	static UINT PASCAL GetEntryCount(const AFX_DISPMAP* pDispMap);
	const AFX_DISPMAP_ENTRY* PASCAL GetDispEntry(LONG memid);
	static LONG PASCAL MemberIDFromName(const AFX_DISPMAP* pDispMap, LPCTSTR lpszName);

	// helpers for member function calling implementation
	static UINT PASCAL GetStackSize(const BYTE* pbParams, VARTYPE vtResult);
#ifdef _SHADOW_DOUBLES
	SCODE PushStackArgs(BYTE* pStack, const BYTE* pbParams,
		void* pResult, VARTYPE vtResult, DISPPARAMS* pDispParams,
		UINT* puArgErr, VARIANT* rgTempVars, UINT nSizeArgs,CVariantBoolConverter* pTempStackArgs = NULL);
#else
	SCODE PushStackArgs(BYTE* pStack, const BYTE* pbParams,
		void* pResult, VARTYPE vtResult, DISPPARAMS* pDispParams,
		UINT* puArgErr, VARIANT* rgTempVars,CVariantBoolConverter* pTempStackArgs = NULL);
#endif

	friend class COleDispatchImpl;

#ifndef _AFX_NO_OCC_SUPPORT
public:
	// OLE event sink implementation
	BOOL OnEvent(UINT idCtrl, AFX_EVENT* pEvent,
		AFX_CMDHANDLERINFO* pHandlerInfo);
protected:
	const AFX_EVENTSINKMAP_ENTRY* PASCAL GetEventSinkEntry(UINT idCtrl,
		AFX_EVENT* pEvent);
#endif // !_AFX_NO_OCC_SUPPORT

	// OLE connection implementation
	struct XConnPtContainer
	{
		DWORD_PTR m_vtbl;   // place-holder for IConnectionPointContainer vtable
#ifndef _AFX_NO_NESTED_DERIVATION
		size_t m_nOffset;
#endif
	} m_xConnPtContainer;

	AFX_MODULE_STATE* m_pModuleState;
	friend class CInnerUnknown;
	friend UINT APIENTRY _AfxThreadEntry(void* pParam);

	virtual BOOL GetExtraConnectionPoints(CPtrArray* pConnPoints);
	virtual LPCONNECTIONPOINT GetConnectionHook(const IID& iid);

	friend class COleConnPtContainer;

#endif //!_AFX_NO_OLE_SUPPORT
};

class CCmdUI        // simple helper class
{
public:
// Attributes
	UINT m_nID;
	UINT m_nIndex;          // menu item or other index

	// if from some other window
	CWnd* m_pOther;         // NULL if a menu or not a CWnd

// Operations to do in ON_UPDATE_COMMAND_UI
	virtual void Enable(BOOL bOn = TRUE);
	virtual void SetCheck(int nCheck = 1);   // 0, 1 or 2 (indeterminate)
	virtual void SetRadio(BOOL bOn = TRUE);
	virtual void SetText(LPCTSTR lpszText);

// Advanced operation
	void ContinueRouting();

// Implementation
	CCmdUI();
	BOOL m_bEnableChanged;
	BOOL m_bContinueRouting;
	UINT m_nIndexMax;       // last + 1 for iterating m_nIndex

	BOOL DoUpdate(CCmdTarget* pTarget, BOOL bDisableIfNoHndler);
};

// special CCmdUI derived classes are used for other UI paradigms
//  like toolbar buttons and status indicators

// pointer to afx_msg member function
#ifndef AFX_MSG_CALL
#define AFX_MSG_CALL
#endif
typedef void (AFX_MSG_CALL CCmdTarget::*AFX_PMSG)(void);

enum AFX_DISPMAP_FLAGS
{
	afxDispCustom = 0,
	afxDispStock = 1
};

#pragma warning( disable: 4121 )
struct AFX_DISPMAP_ENTRY
{
	LPCTSTR lpszName;       // member/property name
	long lDispID;           // DISPID (may be DISPID_UNKNOWN)
	LPCSTR lpszParams;      // member parameter description
	WORD vt;                // return value type / or type of property
	AFX_PMSG pfn;           // normal member On<membercall> or, OnGet<property>
	AFX_PMSG pfnSet;        // special member for OnSet<property>
	size_t nPropOffset;     // property offset
	AFX_DISPMAP_FLAGS flags;// flags (e.g. stock/custom)
};
#pragma warning( default: 4121 )

struct AFX_EVENTSINKMAP_ENTRY
{
	AFX_DISPMAP_ENTRY dispEntry;
	UINT nCtrlIDFirst;
	UINT nCtrlIDLast;
};

// DSC Sink state/reason codes passed to MFC user event handlers
enum DSCSTATE
{
	dscNoState = 0,
	dscOKToDo,
	dscCancelled,
	dscSyncBefore,
	dscAboutToDo,
	dscFailedToDo,
	dscSyncAfter,
	dscDidEvent
};

enum DSCREASON
{
	dscNoReason = 0,
	dscClose,
	dscCommit,
	dscDelete,
	dscEdit,
	dscInsert,
	dscModify,
	dscMove
};

/////////////////////////////////////////////////////////////////////////////
// CWnd implementation

// structures (see afxext.h)
struct CCreateContext;      // context for creating things
struct CPrintInfo;          // print preview customization info

struct AFX_MSGMAP_ENTRY
{
	UINT nMessage;   // windows message
	UINT nCode;      // control code or WM_NOTIFY code
	UINT nID;        // control ID (or 0 for windows messages)
	UINT nLastID;    // used for entries specifying a range of control id's
	UINT_PTR nSig;       // signature type (action) or pointer to message #
	AFX_PMSG pfn;    // routine to call (or special value)
};

/////////////////////////////////////////////////////////////////////////////
// CWnd - a Microsoft Windows application window

class COleDropTarget;   // for more information see AFXOLE.H

// CWnd::m_nFlags (generic to CWnd)
#define WF_TOOLTIPS         0x0001  // window is enabled for tooltips
#define WF_TEMPHIDE         0x0002  // window is temporarily hidden
#define WF_STAYDISABLED     0x0004  // window should stay disabled
#define WF_MODALLOOP        0x0008  // currently in modal loop
#define WF_CONTINUEMODAL    0x0010  // modal loop should continue running
#define WF_OLECTLCONTAINER  0x0100  // some descendant is an OLE control
#define WF_TRACKINGTOOLTIPS 0x0400  // window is enabled for tracking tooltips

#define WF_NOWIN32ISDIALOGMSG   0x0800
#define WF_ISWINFORMSVIEWWND    0x1000

// flags for CWnd::RunModalLoop
#define MLF_NOIDLEMSG       0x0001  // don't send WM_ENTERIDLE messages
#define MLF_NOKICKIDLE      0x0002  // don't send WM_KICKIDLE messages
#define MLF_SHOWONIDLE      0x0004  // show window if not visible at idle time

// extra MFC defined TTF_ flags for TOOLINFO::uFlags
#define TTF_NOTBUTTON       0x80000000L // no status help on buttondown
#define TTF_ALWAYSTIP       0x40000000L // always show the tip even if not active

class CWnd : public CCmdTarget
{
	DECLARE_DYNCREATE(CWnd)
protected:
	static const MSG* PASCAL GetCurrentMessage();

// Attributes
public:
	HWND m_hWnd;            // must be first data member
	operator HWND() const;
	BOOL operator==(const CWnd& wnd) const;
	BOOL operator!=(const CWnd& wnd) const;

	HWND GetSafeHwnd() const;
	DWORD GetStyle() const;
	DWORD GetExStyle() const;
	BOOL ModifyStyle(DWORD dwRemove, DWORD dwAdd, UINT nFlags = 0);
	BOOL ModifyStyleEx(DWORD dwRemove, DWORD dwAdd, UINT nFlags = 0);

	CWnd* GetOwner() const;
	void SetOwner(CWnd* pOwnerWnd);


#if(WINVER >= 0x0500)

	BOOL GetWindowInfo(PWINDOWINFO pwi) const;
	BOOL GetTitleBarInfo(PTITLEBARINFO pti) const;

#endif	// WINVER >= 0x0500

// Constructors and other creation
	CWnd();

	static CWnd* PASCAL FromHandle(HWND hWnd);
	static CWnd* PASCAL FromHandlePermanent(HWND hWnd);
	static void PASCAL DeleteTempMap();
	BOOL Attach(HWND hWndNew);
	HWND Detach();

	// subclassing/unsubclassing functions
	virtual void PreSubclassWindow();
	BOOL SubclassWindow(HWND hWnd);
	BOOL SubclassDlgItem(UINT nID, CWnd* pParent);
	HWND UnsubclassWindow();

public:
	// for child windows, views, panes etc
	virtual BOOL Create(LPCTSTR lpszClassName,
		LPCTSTR lpszWindowName, DWORD dwStyle,
		const RECT& rect,
		CWnd* pParentWnd, UINT nID,
		CCreateContext* pContext = NULL);

	// advanced creation (allows access to extended styles)
	virtual BOOL CreateEx(DWORD dwExStyle, LPCTSTR lpszClassName,
		LPCTSTR lpszWindowName, DWORD dwStyle,
		int x, int y, int nWidth, int nHeight,
		HWND hWndParent, HMENU nIDorHMenu, LPVOID lpParam = NULL);

	virtual BOOL CreateEx(DWORD dwExStyle, LPCTSTR lpszClassName,
		LPCTSTR lpszWindowName, DWORD dwStyle,
		const RECT& rect,
		CWnd* pParentWnd, UINT nID,
		LPVOID lpParam = NULL);

#ifndef _AFX_NO_OCC_SUPPORT
  // for wrapping OLE controls

   //Overload for special controls (WinForms), that require more than CLSID.
   BOOL CreateControl(const CControlCreationInfo& creationInfo, DWORD dwStyle,
  const POINT* ppt, const SIZE* psize, CWnd* pParentWnd, UINT nID);

  LPUNKNOWN GetControlUnknown();
//	BOOL PaintWindowlessControls(CDC *pDC);
#endif

	virtual BOOL DestroyWindow();

	// special pre-creation and window rect adjustment hooks
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

	// Advanced: virtual AdjustWindowRect
	enum AdjustType { adjustBorder = 0, adjustOutside = 1 };
	virtual void CalcWindowRect(LPRECT lpClientRect,
		UINT nAdjustType = adjustBorder);

// Window tree access
	int GetDlgCtrlID() const;
	int SetDlgCtrlID(int nID);
		// get and set window ID, for child windows only
	CWnd* GetDlgItem(int nID) const;
		// get immediate child with given ID
	void GetDlgItem(int nID, HWND* phWnd) const;
		// as above, but returns HWND
	CWnd* GetDescendantWindow(int nID, BOOL bOnlyPerm = FALSE) const;
		// like GetDlgItem but recursive
	void SendMessageToDescendants(UINT message, WPARAM wParam = 0,
		LPARAM lParam = 0, BOOL bDeep = TRUE, BOOL bOnlyPerm = FALSE);
	CWnd* GetTopLevelParent() const;
	CWnd* EnsureTopLevelParent() const;
	CWnd* GetTopLevelOwner() const;
	CWnd* GetParentOwner() const;
	static CWnd* PASCAL GetSafeOwner(CWnd* pParent = NULL, HWND* pWndTop = NULL);

#if(WINVER >= 0x0500)

	CWnd* GetAncestor(UINT gaFlags) const;

#endif	// WINVER >= 0x0500

// Message Functions
#pragma push_macro("SendMessage")
#undef SendMessage
	LRESULT _AFX_FUNCNAME(SendMessage)(UINT message, WPARAM wParam = 0, LPARAM lParam = 0) const;
	LRESULT SendMessage(UINT message, WPARAM wParam = 0, LPARAM lParam = 0) const;
#pragma pop_macro("SendMessage")
	BOOL PostMessage(UINT message, WPARAM wParam = 0, LPARAM lParam = 0);

	BOOL SendNotifyMessage(UINT message, WPARAM wParam, LPARAM lParam);
	BOOL SendChildNotifyLastMsg(LRESULT* pResult = NULL);

	BOOL DragDetect(POINT pt) const;


// Message processing for modeless dialog-like windows
//	BOOL IsDialogMessage(LPMSG lpMsg);

// Window Text Functions
	void SetWindowText(LPCTSTR lpszString);
	int GetWindowText(_Out_z_cap_post_count_(nMaxCount, return + 1) LPTSTR lpszStringBuf, _In_ int nMaxCount) const;
	void GetWindowText(CString& rString) const;
	int GetWindowTextLength() const;

	BOOL GetWindowPlacement(WINDOWPLACEMENT* lpwndpl) const;
	BOOL SetWindowPlacement(const WINDOWPLACEMENT* lpwndpl);

// Coordinate Mapping Functions
	void ClientToScreen(LPPOINT lpPoint) const;
	void ClientToScreen(LPRECT lpRect) const;
	void ScreenToClient(LPPOINT lpPoint) const;
	void ScreenToClient(LPRECT lpRect) const;
	void MapWindowPoints(CWnd* pwndTo, LPPOINT lpPoint, UINT nCount) const;
	void MapWindowPoints(CWnd* pwndTo, LPRECT lpRect) const;

// Update/Painting Functions
//	CDC* GetDC();
//	CDC* GetWindowDC();
//	int ReleaseDC(CDC* pDC);
//	void Print(CDC* pDC, DWORD dwFlags) const;
//	void PrintClient(CDC* pDC, DWORD dwFlags) const;

	void UpdateWindow();
	void SetRedraw(BOOL bRedraw = TRUE);
	BOOL GetUpdateRect(LPRECT lpRect, BOOL bErase = FALSE);
	// int GetUpdateRgn(CRgn* pRgn, BOOL bErase = FALSE);
	void Invalidate(BOOL bErase = TRUE);
	void InvalidateRect(LPCRECT lpRect, BOOL bErase = TRUE);
	// void InvalidateRgn(CRgn* pRgn, BOOL bErase = TRUE);
	void ValidateRect(LPCRECT lpRect);
	// void ValidateRgn(CRgn* pRgn);
	BOOL ShowWindow(int nCmdShow);
	BOOL IsWindowVisible() const;
	void ShowOwnedPopups(BOOL bShow = TRUE);

	// CDC* GetDCEx(CRgn* prgnClip, DWORD flags);
	BOOL LockWindowUpdate();
	void UnlockWindowUpdate();
	// BOOL RedrawWindow(LPCRECT lpRectUpdate = NULL,
		// CRgn* prgnUpdate = NULL,
		// UINT flags = RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
//	BOOL EnableScrollBar(int nSBFlags, UINT nArrowFlags = ESB_ENABLE_BOTH);

//	BOOL DrawAnimatedRects(int idAni, CONST RECT *lprcFrom, CONST RECT *lprcTo);
//	BOOL DrawCaption(CDC* pDC, LPCRECT lprc, UINT uFlags);

#if(WINVER >= 0x0500)

//	BOOL AnimateWindow(DWORD dwTime, DWORD dwFlags);

#endif	// WINVER >= 0x0500

#if(_WIN32_WINNT >= 0x0501)

//	BOOL PrintWindow(CDC* pDC, UINT nFlags) const;

#endif	// _WIN32_WINNT >= 0x0501

// Layered Window

#if(_WIN32_WINNT >= 0x0500)

	BOOL SetLayeredWindowAttributes(COLORREF crKey, BYTE bAlpha, DWORD dwFlags);
//	BOOL UpdateLayeredWindow(CDC* pDCDst, POINT *pptDst, SIZE *psize,
//		CDC* pDCSrc, POINT *pptSrc, COLORREF crKey, BLENDFUNCTION *pblend, DWORD dwFlags);

#endif	// _WIN32_WINNT >= 0x0500

#if(_WIN32_WINNT >= 0x0501)

	BOOL GetLayeredWindowAttributes(COLORREF *pcrKey, BYTE *pbAlpha, DWORD *pdwFlags) const;

#endif	// _WIN32_WINNT >= 0x0501


// Timer Functions
	UINT_PTR SetTimer(UINT_PTR nIDEvent, UINT nElapse,
		void (CALLBACK* lpfnTimer)(HWND, UINT, UINT_PTR, DWORD));
	BOOL KillTimer(UINT_PTR nIDEvent);

// ToolTip Functions
	BOOL EnableToolTips(BOOL bEnable = TRUE);
	BOOL EnableTrackingToolTips(BOOL bEnable = TRUE);
	static void PASCAL CancelToolTips(BOOL bKeys = FALSE);
	void FilterToolTipMessage(MSG* pMsg);

// Window State Functions
	BOOL IsWindowEnabled() const;
	BOOL EnableWindow(BOOL bEnable = TRUE);

	// the active window applies only to top-level (frame windows)
	static CWnd* PASCAL GetActiveWindow();
	CWnd* SetActiveWindow();

	// the foreground window applies only to top-level windows (frame windows)
	BOOL SetForegroundWindow();
	static CWnd* PASCAL GetForegroundWindow();

	// capture and focus apply to all windows
	static CWnd* PASCAL GetCapture();
	CWnd* SetCapture();
	static CWnd* PASCAL GetFocus();
	CWnd* SetFocus();

	static CWnd* PASCAL GetDesktopWindow();

// Obsolete and non-portable APIs - not recommended for new code
	void CloseWindow();
	BOOL OpenIcon();

// Window Access Functions
	CWnd* ChildWindowFromPoint(POINT point) const;
	CWnd* ChildWindowFromPoint(POINT point, UINT nFlags) const;
	static CWnd* PASCAL FindWindow(LPCTSTR lpszClassName, LPCTSTR lpszWindowName);
	static CWnd* FindWindowEx(HWND hwndParent, HWND hwndChildAfter, LPCTSTR lpszClass, LPCTSTR lpszWindow);

	CWnd* GetWindow(UINT nCmd) const;
	CWnd* GetLastActivePopup() const;

	BOOL IsChild(const CWnd* pWnd) const;
	CWnd* GetParent() const;
	CWnd* SetParent(CWnd* pWndNewParent);
	static CWnd* PASCAL WindowFromPoint(POINT point);

// Alert Functions
//	BOOL FlashWindow(BOOL bInvert);
//#pragma push_macro("MessageBox")
//#undef MessageBox
//	int _AFX_FUNCNAME(MessageBox)(LPCTSTR lpszText, LPCTSTR lpszCaption = NULL,
//			UINT nType = MB_OK);
//	int MessageBox(LPCTSTR lpszText, LPCTSTR lpszCaption = NULL,
//			UINT nType = MB_OK);
//#pragma pop_macro("MessageBox")

#if(WINVER >= 0x0500)

//	BOOL FlashWindowEx(DWORD dwFlags, UINT  uCount, DWORD dwTimeout);

#endif	// WINVER >= 0x0500

// Clipboard Functions
	BOOL ChangeClipboardChain(HWND hWndNext);
	HWND SetClipboardViewer();
	BOOL OpenClipboard();
	static CWnd* PASCAL GetClipboardOwner();
	static CWnd* PASCAL GetClipboardViewer();
	static CWnd* PASCAL GetOpenClipboardWindow();

// Caret Functions
	void CreateSolidCaret(int nWidth, int nHeight);
	void CreateGrayCaret(int nWidth, int nHeight);
	static void PASCAL SetCaretPos(POINT point);
	void HideCaret();
	void ShowCaret();

// Shell Interaction Functions
	void DragAcceptFiles(BOOL bAccept = TRUE);

// Icon Functions
	HICON SetIcon(HICON hIcon, BOOL bBigIcon);
	HICON GetIcon(BOOL bBigIcon) const;

// Context Help Functions
	BOOL SetWindowContextHelpId(DWORD dwContextHelpId);
	DWORD GetWindowContextHelpId() const;

// Dialog Data support
public:
	BOOL UpdateData(BOOL bSaveAndValidate = TRUE);
			// data wnd must be same type as this

// Help Command Handlers
	afx_msg void OnHelp();          // F1 (uses current context)
	afx_msg void OnHelpIndex();     // ID_HELP_INDEX
	afx_msg void OnHelpFinder();    // ID_HELP_FINDER, ID_DEFAULT_HELP
	afx_msg void OnHelpUsing();     // ID_HELP_USING

// Layout and other functions
public:
	enum RepositionFlags
		{ reposDefault = 0, reposQuery = 1, reposExtra = 2, reposNoPosLeftOver=0x8000 };
	void RepositionBars(UINT nIDFirst, UINT nIDLast, UINT nIDLeftOver,
		UINT nFlag = reposDefault, LPRECT lpRectParam = NULL,
		LPCRECT lpRectClient = NULL, BOOL bStretch = TRUE);

	// dialog support
	void UpdateDialogControls(CCmdTarget* pTarget, BOOL bDisableIfNoHndler);
	// void CenterWindow(CWnd* pAlternateOwner = NULL);
	int RunModalLoop(DWORD dwFlags = 0);
	virtual BOOL ContinueModal();
	virtual void EndModalLoop(int nResult);

#ifndef _AFX_NO_OCC_SUPPORT
// OLE control wrapper functions
	 COleControlSite* GetOleControlSite(UINT idControl) const;
	void AFX_CDECL InvokeHelper(DISPID dwDispID, WORD wFlags,
		VARTYPE vtRet, void* pvRet, const BYTE* pbParamInfo, ...);
	void AFX_CDECL SetProperty(DISPID dwDispID, VARTYPE vtProp, ...);
	void GetProperty(DISPID dwDispID, VARTYPE vtProp, void* pvProp) const;
	IUnknown* GetDSCCursor();
	void BindDefaultProperty(DISPID dwDispID, VARTYPE vtProp, LPCTSTR szFieldName, CWnd* pDSCWnd);
	void BindProperty(DISPID dwDispId, CWnd* pWndDSC);
#endif

// Accessibility Support
public :
	void EnableActiveAccessibility();
	void NotifyWinEvent(DWORD event, LONG idObjectType, LONG idObject);

public :
	// Windows 7 taskbar Tabs support

	/// <summary>
	/// Called by the framework when it needs to obtain a bitmap to be displayed on Windows 7 tab thumbnail,
	/// or on the client for application peek. </summary>
	/// <description>
	/// Override this method in a derived class and draw on the specified device context in order to customize thumbnail and peek.
	/// If bThumbnail is TRUE, szRequiredThumbnailSize can be ignored. In this case you should be aware
	/// that you draw full sized bitmap (e.g. a bitmap that cover the whole client area). The device context (dc) comes with selected 32 bits bitmap.
	/// The default implementation sends WM_PRINT to this window with PRF_CLIENT, PRF_CHILDREN and PRF_NONCLIENT flags.</description>
	/// <param name="dc"> Specifies the device context.</param>
	/// <param name="rect"> Specifies the bounding rectangle of area to render.</param>
	/// <param name="szRequiredThumbnailSize"> Specifies the size of target thumbnail. Should be ignored if bIsThumbnail is FALSE.</param>
	/// <param name="bIsThumbnail"> Specifies whether this method is called for iconic thumbnail or live preview (peek).</param>
	/// <param name="bAlphaChannelSet"> Output parameter. Set it to TRUE if your implementation initializes alpha channel of a bitmap
	/// selected in dc.</param>
	// virtual void OnDrawIconicThumbnailOrLivePreview(CDC& dc, CRect rect, CSize szRequiredThumbnailSize, BOOL bIsThumbnail, BOOL& bAlphaChannelSet);

protected :
	bool m_bEnableActiveAccessibility;
	friend BOOL AFXAPI AfxWinInit(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
		_In_z_ LPTSTR lpCmdLine, _In_ int nCmdShow);

protected:
	afx_msg LRESULT OnGetObject(WPARAM, LPARAM);

public :
	// Helpers for windows that contain windowless controls
	long GetWindowLessChildCount();
	long GetWindowedChildCount();
	long GetAccessibleChildCount();
	HRESULT GetAccessibleChild(VARIANT varChild, IDispatch** ppdispChild);
	HRESULT GetAccessibleName(VARIANT varChild, BSTR* pszName);
	HRESULT GetAccessibilityLocation(VARIANT varChild, long *pxLeft, long *pyTop, long *pcxWidth, long *pcyHeight);
	HRESULT GetAccessibilityHitTest(long xLeft, long yTop, VARIANT *pvarChild);


// Window-Management message handler member functions
protected:
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);

	afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
	afx_msg void OnActivateApp(BOOL bActive, DWORD dwThreadID);
	afx_msg LRESULT OnActivateTopLevel(WPARAM, LPARAM);
	afx_msg void OnCancelMode();
	afx_msg void OnChildActivate();
	afx_msg void OnClose();
	afx_msg BOOL OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);

	afx_msg void OnDestroy();
	afx_msg void OnEnable(BOOL bEnable);
	afx_msg void OnEndSession(BOOL bEnding);
	afx_msg void OnEnterIdle(UINT nWhy, CWnd* pWho);
//	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
	afx_msg BOOL OnHelpInfo(HELPINFO* lpHelpInfo);
//	afx_msg void OnIconEraseBkgnd(CDC* pDC);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnMove(int x, int y);
	afx_msg void OnPaint();
	afx_msg void OnSyncPaint();
	afx_msg void OnParentNotify(UINT message, LPARAM lParam);
	afx_msg UINT OnNotifyFormat(CWnd* pWnd, UINT nCommand);
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg BOOL OnQueryEndSession();
	afx_msg BOOL OnQueryNewPalette();
	afx_msg BOOL OnQueryOpen();
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnTCard(UINT idAction, DWORD dwActionData);
	afx_msg void OnWindowPosChanging(WINDOWPOS* lpwndpos);
	afx_msg void OnWindowPosChanged(WINDOWPOS* lpwndpos);
	afx_msg void OnSessionChange(UINT nSessionState, UINT nId);

	afx_msg void OnChangeUIState(UINT nAction, UINT nUIElement);
	afx_msg void OnUpdateUIState(UINT nAction, UINT nUIElement);
	afx_msg UINT OnQueryUIState();

// Nonclient-Area message handler member functions
	afx_msg BOOL OnNcActivate(BOOL bActive);
	afx_msg void OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp);
	afx_msg BOOL OnNcCreate(LPCREATESTRUCT lpCreateStruct);
	// afx_msg void OnNcDestroy();
	afx_msg void OnNcPaint();

// System message handler member functions
//	afx_msg void OnDropFiles(HDROP hDropInfo);
//	afx_msg void OnPaletteIsChanging(CWnd* pRealizeWnd);
	afx_msg void OnSysChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnSysDeadChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnSysKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg BOOL OnAppCommand(CWnd* pWnd, UINT nCmd, UINT nDevice, UINT nKey);
#if(_WIN32_WINNT >= 0x0501)
	afx_msg void OnRawInput(UINT nInputCode, HRAWINPUT hRawInput);
#endif
	afx_msg void OnCompacting(UINT nCpuTime);
	afx_msg void OnFontChange();
	afx_msg void OnPaletteChanged(CWnd* pFocusWnd);
	afx_msg void OnSpoolerStatus(UINT nStatus, UINT nJobs);
	afx_msg void OnSysColorChange();
	afx_msg void OnTimeChange();
	afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
	afx_msg void OnWinIniChange(LPCTSTR lpszSection);
	afx_msg UINT OnPowerBroadcast(UINT nPowerEvent, UINT nEventData);
	afx_msg void OnUserChanged();
	afx_msg void OnInputLangChange(UINT nCharSet, UINT nLocaleId);
	afx_msg void OnInputLangChangeRequest(UINT nFlags, UINT nLocaleId);
	afx_msg void OnInputDeviceChange(unsigned short nFlags, HANDLE hDevice);

// Input message handler member functions
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnDeadChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnUniChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnHotKey(UINT nHotKeyId, UINT nKey1, UINT nKey2);
	afx_msg void OnMouseLeave();
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnTimer(UINT_PTR nIDEvent);

// Clipboard message handler member functions
	afx_msg void OnAskCbFormatName(_In_ UINT nMaxCount, _Out_z_cap_(nMaxCount) LPTSTR lpszString);
	afx_msg void OnChangeCbChain(HWND hWndRemove, HWND hWndAfter);
	afx_msg void OnDestroyClipboard();
	afx_msg void OnDrawClipboard();
	afx_msg void OnPaintClipboard(CWnd* pClipAppWnd, HGLOBAL hPaintStruct);
	afx_msg void OnRenderAllFormats();
	afx_msg void OnRenderFormat(UINT nFormat);
	afx_msg void OnSizeClipboard(CWnd* pClipAppWnd, HGLOBAL hRect);
	afx_msg void OnVScrollClipboard(CWnd* pClipAppWnd, UINT nSBCode, UINT nPos);
	afx_msg void OnClipboardUpdate();

// Control message handler member functions
	afx_msg int OnCompareItem(int nIDCtl, LPCOMPAREITEMSTRUCT lpCompareItemStruct);
	afx_msg void OnDeleteItem(int nIDCtl, LPDELETEITEMSTRUCT lpDeleteItemStruct);
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	afx_msg UINT OnGetDlgCode();
	afx_msg void OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMeasureItemStruct);

// MDI message handler member functions
	afx_msg void OnMDIActivate(BOOL bActivate,
		CWnd* pActivateWnd, CWnd* pDeactivateWnd);

// Menu loop notification messages
	afx_msg void OnEnterMenuLoop(BOOL bIsTrackPopupMenu);
	afx_msg void OnExitMenuLoop(BOOL bIsTrackPopupMenu);
// Win4 messages
	afx_msg void OnStyleChanged(int nStyleType, LPSTYLESTRUCT lpStyleStruct);
	afx_msg void OnStyleChanging(int nStyleType, LPSTYLESTRUCT lpStyleStruct);
	afx_msg void OnSizing(UINT nSide, LPRECT lpRect);
	afx_msg void OnMoving(UINT nSide, LPRECT lpRect);
	afx_msg void OnEnterSizeMove();
	afx_msg void OnExitSizeMove();
	afx_msg void OnCaptureChanged(CWnd* pWnd);
	afx_msg BOOL OnDeviceChange(UINT nEventType, DWORD_PTR dwData);

// Desktop Windows Manager messages
	afx_msg void OnCompositionChanged();
	afx_msg void OnNcRenderingChanged(BOOL bIsRendering);
	afx_msg void OnColorizationColorChanged(DWORD dwColorizationColor, BOOL bOpacity);
	afx_msg void OnWindowMaximizedChange(BOOL bIsMaximized);

// Overridables and other helpers (for implementation of derived classes)
protected:
	// for deriving from a standard control
	virtual WNDPROC* GetSuperWndProcAddr();

	// for dialog data exchange and validation
	virtual void DoDataExchange(CDataExchange* pDX);

public:
	// for modality
	virtual void BeginModalState();
	virtual void EndModalState();

	// for translating Windows messages in main message pump
	virtual BOOL PreTranslateMessage(MSG* pMsg);

#ifndef _AFX_NO_OCC_SUPPORT
	// for ambient properties exposed to contained OLE controls
	virtual BOOL OnAmbientProperty(COleControlSite* pSite, DISPID dispid,
		VARIANT* pvar);
#endif

	// for touch:

	/// <summary>
	/// Register/Unregister window Windows touch support</summary>
	/// <returns>
	/// TRUE if succeeds; otherwise FALSE.</returns>
	/// <param name="bRegister">TRUE - register Windows touch support; FALSE - otherwise.</param>
	/// <param name="ulFlags">A set of bit flags that specify optional modifications. This field may contain 0 or one of the following values: TWF_FINETOUCH; TWF_WANTPALM</param>
	BOOL RegisterTouchWindow(BOOL bRegister = TRUE, ULONG ulFlags = 0);

	/// <summary>
	/// Specifies whether CWnd has touch support</summary>
	/// <returns>
	/// TRUE if CWnd has touch support; otherwise FALSE.</returns>
	BOOL IsTouchWindow() const;

protected:
	// for processing Windows messages
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	virtual BOOL OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult);

	// for handling default processing
	LRESULT Default();
	virtual LRESULT DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam);

	// for custom cleanup after WM_NCDESTROY
	virtual void PostNcDestroy();

	// for notifications from parent
	virtual BOOL OnChildNotify(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult);
		// return TRUE if parent should not process this message
	BOOL ReflectChildNotify(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	// static BOOL PASCAL ReflectLastMsg(HWND hWndChild, LRESULT* pResult = NULL);

	// for touch:
	BOOL m_bIsTouchWindowRegistered;

// Implementation
public:
	virtual ~CWnd();
	virtual BOOL CheckAutoCenter();
	static BOOL PASCAL GrayCtlColor(HDC hDC, HWND hWnd, UINT nCtlColor,
		HBRUSH hbrGray, COLORREF clrText);

	// helper routines for implementation
	BOOL HandleFloatingSysCommand(UINT nID, LPARAM lParam);
	BOOL IsTopParentActive() const;
	void ActivateTopParent();
	static BOOL PASCAL WalkPreTranslateTree(HWND hWndStop, MSG* pMsg);
	static CWnd* PASCAL GetDescendantWindow(HWND hWnd, int nID,
		BOOL bOnlyPerm);
	static void PASCAL SendMessageToDescendants(HWND hWnd, UINT message,
		WPARAM wParam, LPARAM lParam, BOOL bDeep, BOOL bOnlyPerm);
	virtual void OnFinalRelease();
	BOOL PreTranslateInput(LPMSG lpMsg);
	static BOOL PASCAL ModifyStyle(HWND hWnd, DWORD dwRemove, DWORD dwAdd,
		UINT nFlags);
	static BOOL PASCAL ModifyStyleEx(HWND hWnd, DWORD dwRemove, DWORD dwAdd,
		UINT nFlags);
	static void PASCAL _FilterToolTipMessage(MSG* pMsg, CWnd* pWnd);
	BOOL _EnableToolTips(BOOL bEnable, UINT nFlag);
	static HWND PASCAL GetSafeOwner_(HWND hWnd, HWND* pWndTop);
	void PrepareForHelp();

public:
	HWND m_hWndOwner;   // implementation of SetOwner and GetOwner
	UINT m_nFlags;      // see WF_ flags above

protected:
	WNDPROC m_pfnSuper; // for subclassing of controls
//	static const UINT m_nMsgDragList;
	int m_nModalResult; // for return values from CWnd::RunModalLoop

	// COleDropTarget* m_pDropTarget;  // for automatic cleanup of drop target
	// friend class COleDropTarget;

	// for creating dialogs and dialog-like windows
//	BOOL CreateDlg(LPCTSTR lpszTemplateName, CWnd* pParentWnd);
//	BOOL CreateDlgIndirect(LPCDLGTEMPLATE lpDialogTemplate, CWnd* pParentWnd,
//		HINSTANCE hInst);

#ifndef _AFX_NO_OCC_SUPPORT
	friend class COccManager;
	virtual BOOL SetOccDialogInfo(struct _AFX_OCC_DIALOG_INFO* pOccDialogInfo);
	virtual _AFX_OCC_DIALOG_INFO* GetOccDialogInfo();
	void AttachControlSite(CHandleMap* pMap);
public:
	void AttachControlSite(CWnd* pWndParent, UINT nIDC = 0);
#endif

public:

protected:
	// implementation of message dispatch/hooking
	friend LRESULT CALLBACK _AfxSendMsgHook(int, WPARAM, LPARAM);
	friend void AFXAPI _AfxStandardSubclass(HWND);
	friend LRESULT CALLBACK _AfxCbtFilterHook(int, WPARAM, LPARAM);
	friend LRESULT AFXAPI AfxCallWndProc(CWnd*, HWND, UINT, WPARAM, LPARAM);

	// standard message implementation
	afx_msg LRESULT OnNTCtlColor(WPARAM wParam, LPARAM lParam);
//	afx_msg LRESULT OnDisplayChange(WPARAM, LPARAM);
	afx_msg LRESULT OnDragList(WPARAM, LPARAM);

	// Helper functions for retrieving Text from windows messsage / structure
	// -----------------------------------------------------------------------
	// errCode  - the errCode for the window message, uMsg
	// pszText  - buffer to grow and retrieve the text (do not allocate when calling, the function will allocate)
	// cch      - size of the buffer in TCHAR to be pass to the windows message, uMsg
	// cchBegin - initial size to allocate
	// cchEnd   - maximum size to allocate
	// uMsg     - window message
	// lParam   - the LPARAM of the message.  This is pass by reference because it could potentially be alias of pszText/cch for some messages.
	// wParam   - the WPARAM of the message.  This is pass by reference because it could potentially be alias of pszText/cch for some messages.
	// strOut   - the CString containing the received text

	template <class TReturnType, class TCchType >
	TReturnType EnlargeBufferGetText(_In_ TReturnType errCode, LPTSTR& pszText, TCchType& cch, TCchType cchBegin, TCchType cchEnd, UINT uMsg, WPARAM& wParam, LPARAM& lParam, CString& strOut) const throw()
	{
		ENSURE(::IsWindow(m_hWnd));
		ENSURE(cchBegin < cchEnd);
		ENSURE(cchEnd <= INT_MAX); // CString only support up to INT_MAX
		TReturnType retCode = errCode;
		strOut = CString();
		cch = cchBegin;
		do
		{
			pszText = strOut.GetBufferSetLength(cch);
			retCode = static_cast<TReturnType>(this->SendMessage(uMsg, wParam, lParam));
			strOut.ReleaseBuffer();
			pszText = NULL;

			if (retCode == errCode)
			{
				// error clear the string and return error
				strOut = CString();
				cch=0;
				break;
			}
			if (static_cast<TCchType>(strOut.GetLength()) < cch-1)
			{
				cch = strOut.GetLength();
				break;
			}
		}
		while( (::ATL::AtlMultiply(&cch, cch, 2) == S_OK) && (cch < cchEnd));
		return retCode;
	}


	template <class TReturnType>
	inline TReturnType EnlargeBufferGetText(TReturnType errCode, LPTSTR& pszText, int& pcch, UINT uMsg, WPARAM& wParam, LPARAM& lParam, CString& strOut) const throw()
	{
		return EnlargeBufferGetText<TReturnType, int>(errCode, pszText, pcch, 256, INT_MAX, uMsg, wParam, lParam, strOut);
	}

	template <class TReturnType>
	inline TReturnType EnlargeBufferGetText(TReturnType errCode, LPTSTR& pszText, UINT& pcch, UINT uMsg, WPARAM& wParam, LPARAM& lParam, CString& strOut) const throw()
	{
		// using INT_MAX instead of UINT_MAX here because CString has a INT_MAX limit
		return EnlargeBufferGetText<TReturnType, UINT>(errCode, pszText, pcch, 256, INT_MAX, uMsg, wParam, lParam, strOut);
	}

	//{{AFX_MSG(CWnd)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	CWnd(HWND hWnd);    // just for special initialization
};

// helpers for registering your own WNDCLASSes
LPCTSTR AFXAPI AfxRegisterWndClass(UINT nClassStyle,
	HCURSOR hCursor = 0, HBRUSH hbrBackground = 0, HICON hIcon = 0);

BOOL AFXAPI AfxRegisterClass(WNDCLASS* lpWndClass);

// helper to initialize rich edit 1.0 control
BOOL AFXAPI AfxInitRichEdit();
// helper to initialize rich edit 2.0 control
BOOL AFXAPI AfxInitRichEdit2();

// Implementation
LRESULT CALLBACK AfxWndProc(HWND, UINT, WPARAM, LPARAM);

WNDPROC AFXAPI AfxGetAfxWndProc();
#define AfxWndProc (*AfxGetAfxWndProc())

typedef void (AFX_MSG_CALL CWnd::*AFX_PMSGW)(void);
	// like 'AFX_PMSG' but for CWnd derived classes only

typedef void (AFX_MSG_CALL CWinThread::*AFX_PMSGT)(void);
	// like 'AFX_PMSG' but for CWinThread-derived classes only

#pragma warning( pop )

/////////////////////////////////////////////////////////////////////////////
// CWinThread

typedef UINT (AFX_CDECL *AFX_THREADPROC)(LPVOID);

class COleMessageFilter;        // forward reference (see afxole.h)

BOOL AFXAPI AfxPumpMessage();
LRESULT AFXAPI AfxProcessWndProcException(CException*, const MSG* pMsg);
BOOL __cdecl AfxPreTranslateMessage(MSG* pMsg);
BOOL __cdecl AfxIsIdleMessage(MSG* pMsg);

class CWinThread : public CCmdTarget
{
	DECLARE_DYNAMIC(CWinThread)

	friend BOOL AfxInternalPreTranslateMessage(MSG* pMsg);

public:
// Constructors
	CWinThread();
	BOOL CreateThread(DWORD dwCreateFlags = 0, UINT nStackSize = 0,
		LPSECURITY_ATTRIBUTES lpSecurityAttrs = NULL);

// Attributes
	CWnd* m_pMainWnd;       // main window (usually same AfxGetApp()->m_pMainWnd)
	CWnd* m_pActiveWnd;     // active main window (may not be m_pMainWnd)
	BOOL m_bAutoDelete;     // enables 'delete this' after thread termination

	// only valid while running
	HANDLE m_hThread;       // this thread's HANDLE
	operator HANDLE() const;
	DWORD m_nThreadID;      // this thread's ID

	int GetThreadPriority();
	BOOL SetThreadPriority(int nPriority);

// Operations
	DWORD SuspendThread();
	DWORD ResumeThread();
	BOOL PostThreadMessage(UINT message, WPARAM wParam, LPARAM lParam);

// Overridables
	// thread initialization
	virtual BOOL InitInstance();

	// running and idle processing
	virtual int Run();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL PumpMessage();     // low level message pump
	virtual BOOL OnIdle(LONG lCount); // return TRUE if more idle processing
	virtual BOOL IsIdleMessage(MSG* pMsg);  // checks for special messages

	// thread termination
	virtual int ExitInstance(); // default will 'delete this'

	// Advanced: exception handling
	virtual LRESULT ProcessWndProcException(CException* e, const MSG* pMsg);

	// Advanced: handling messages sent to message filter hook
	virtual BOOL ProcessMessageFilter(int code, LPMSG lpMsg);

	// Advanced: virtual access to m_pMainWnd
	virtual CWnd* GetMainWnd();

// Implementation
public:
	virtual ~CWinThread();
	void CommonConstruct();
	virtual void Delete();
		// 'delete this' only if m_bAutoDelete == TRUE

public:
	// constructor used by implementation of AfxBeginThread
	CWinThread(AFX_THREADPROC pfnThreadProc, LPVOID pParam);

	// valid after construction
	LPVOID m_pThreadParams; // generic parameters passed to starting function
	AFX_THREADPROC m_pfnThreadProc;

	// set after OLE is initialized
	void (AFXAPI* m_lpfnOleTermOrFreeLib)(BOOL, BOOL);
	COleMessageFilter* m_pMessageFilter;

protected:
	BOOL DispatchThreadMessageEx(MSG* msg);  // helper
	void DispatchThreadMessage(MSG* msg);  // obsolete
};

// global helpers for threads

CWinThread* AFXAPI AfxBeginThread(AFX_THREADPROC pfnThreadProc, LPVOID pParam,
	int nPriority = THREAD_PRIORITY_NORMAL, UINT nStackSize = 0,
	DWORD dwCreateFlags = 0, LPSECURITY_ATTRIBUTES lpSecurityAttrs = NULL);
CWinThread* AFXAPI AfxBeginThread(CRuntimeClass* pThreadClass,
	int nPriority = THREAD_PRIORITY_NORMAL, UINT nStackSize = 0,
	DWORD dwCreateFlags = 0, LPSECURITY_ATTRIBUTES lpSecurityAttrs = NULL);

CWinThread* AFXAPI AfxGetThread();
MSG* AFXAPI AfxGetCurrentMessage();

void AFXAPI AfxEndThread(UINT nExitCode, BOOL bDelete = TRUE);

void AFXAPI AfxInitThread();
void AFXAPI AfxTermThread(HINSTANCE hInstTerm = NULL);

/////////////////////////////////////////////////////////////////////////////
// Global functions for access to the one and only CWinApp

#define afxCurrentWinApp    AfxGetModuleState()->m_pCurrentWinApp
#define afxCurrentInstanceHandle    AfxGetModuleState()->m_hCurrentInstanceHandle
#define afxCurrentResourceHandle    AfxGetModuleState()->m_hCurrentResourceHandle
#define afxCurrentAppName   AfxGetModuleState()->m_lpszCurrentAppName
#define afxContextIsDLL     AfxGetModuleState()->m_bDLL
#define afxRegisteredClasses    AfxGetModuleState()->m_fRegisteredClasses
#define afxAmbientActCtx    AfxGetModuleState()->m_bSetAmbientActCtx

#ifndef _AFX_NO_OCC_SUPPORT
#define afxOccManager   AfxGetModuleState()->m_pOccManager
#endif

//Fusion: Access macros for WinSxS dynamic wrappers.
//#ifndef _AFX_NO_AFXCMN_SUPPORT
//#define _AFX_COMCTL32_ISOLATION_WRAPPER_INDEX 0
//#define afxComCtlWrapper static_cast<CComCtlWrapper*>(AfxGetModuleState()->m_pDllIsolationWrappers[_AFX_COMCTL32_ISOLATION_WRAPPER_INDEX])
//#endif

//#define _AFX_COMMDLG_ISOLATION_WRAPPER_INDEX 1
//#define afxCommDlgWrapper static_cast<CCommDlgWrapper*>(AfxGetModuleState()->m_pDllIsolationWrappers[_AFX_COMMDLG_ISOLATION_WRAPPER_INDEX])

//#define _AFX_SHELL_ISOLATION_WRAPPER_INDEX 2
//#define afxShellWrapper static_cast<CShellWrapper*>(AfxGetModuleState()->m_pDllIsolationWrappers[_AFX_SHELL_ISOLATION_WRAPPER_INDEX])

//#define _AFX_ISOLATION_WRAPPER_ARRAY_SIZE 3

// Advanced initialization: for overriding default WinMain
BOOL AFXAPI AfxWinInit(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
	_In_z_ LPTSTR lpCmdLine, _In_ int nCmdShow);
void AFXAPI AfxWinTerm();

// Global Windows state data helper functions (inlines)
#ifdef _AFXDLL
ULONG AFXAPI AfxGetDllVersion();
#endif

CWnd* AFXAPI AfxGetMainWnd();
HINSTANCE AFXAPI AfxGetInstanceHandle();
HINSTANCE AFXAPI AfxGetResourceHandle();
void AFXAPI AfxSetResourceHandle(HINSTANCE hInstResource);
LPCTSTR AFXAPI AfxGetAppName();
AFX_DEPRECATED("AfxLoadLangResourceDLL(LPCTSTR pszFormat) has been deprecated, use AfxLoadLangResourceDLL(LPCTSTR pszFormat, LPCTSTR pszPath) instead")
	HINSTANCE AFXAPI AfxLoadLangResourceDLL(LPCTSTR pszFormat);
HINSTANCE AFXAPI AfxLoadLangResourceDLL(LPCTSTR pszFormat, LPCTSTR pszPath);

// Use instead of PostQuitMessage in OLE server applications
void AFXAPI AfxPostQuitMessage(int nExitCode);

// Use AfxFindResourceHandle to find resource in chain of extension DLLs
#ifndef _AFXDLL
#define AfxFindResourceHandle(lpszResource, lpszType) AfxGetResourceHandle()
#else
HINSTANCE AFXAPI AfxFindResourceHandle(LPCTSTR lpszName, LPCTSTR lpszType);
#endif

/// <summary>
/// Deletes the subkeys and values of the specified key recursively.</summary>
/// <returns>
/// If the function succeeds, the return value is ERROR_SUCCESS. If the function fails, the return value is a nonzero error code defined in Winerror.h</returns>
/// <param name="hKey">A handle to an open registry key.</param>
/// <param name="lpSubKey">The name of the key to be deleted.</param>
LONG AFXAPI AfxDelRegTreeHelper(HKEY hParentKey, const CString& strKeyName);

class CRecentFileList;          // forward reference (see afxadv.h)

// access to message filter in CWinApp
COleMessageFilter* AFXAPI AfxOleGetMessageFilter();

/////////////////////////////////////////////////////////////////////////////
// CCommandLineInfo

class CCommandLineInfo : public CObject
{
public:
	// Sets default values
	CCommandLineInfo();

	// plain char* version on UNICODE for source-code backwards compatibility
	virtual void ParseParam(const TCHAR* pszParam, BOOL bFlag, BOOL bLast);
#ifdef _UNICODE
	virtual void ParseParam(const char* pszParam, BOOL bFlag, BOOL bLast);
#endif

	BOOL m_bShowSplash;
	BOOL m_bRunEmbedded;
	BOOL m_bRunAutomated;
	BOOL m_bRegisterPerUser;
	enum { FileNew, FileOpen, FilePrint, FilePrintTo, FileDDE, FileDDENoShow, AppRegister,
		AppUnregister, RestartByRestartManager, FileNothing = -1 } m_nShellCommand;

	// not valid for FileNew
	CString m_strFileName;

	// valid only for FilePrintTo
	CString m_strPrinterName;
	CString m_strDriverName;
	CString m_strPortName;

	// valid only for RestartByRestartManager
	CString m_strRestartIdentifier;

	~CCommandLineInfo();
// Implementation
protected:
	void ParseParamFlag(const char* pszParam);
	void ParseParamNotFlag(const TCHAR* pszParam);
#ifdef _UNICODE
	void ParseParamNotFlag(const char* pszParam);
#endif
	void ParseLast(BOOL bLast);
};

/////////////////////////////////////////////////////////////////////////////
// CWinApp - the root of all Windows applications

#define _AFX_MRU_COUNT   4      // default support for 4 entries in file MRU
#define _AFX_MRU_MAX_COUNT 16   // currently allocated id range supports 16

#define _AFX_SYSPOLICY_NOTINITIALIZED			0
#define _AFX_SYSPOLICY_NORUN					1
#define _AFX_SYSPOLICY_NODRIVES					2
#define _AFX_SYSPOLICY_RESTRICTRUN				4
#define _AFX_SYSPOLICY_NONETCONNECTDISCONNECTD	8
#define _AFX_SYSPOLICY_NOENTIRENETWORK			16
#define _AFX_SYSPOLICY_NORECENTDOCHISTORY		32
#define _AFX_SYSPOLICY_NOCLOSE					64
#define _AFX_SYSPOLICY_NOPLACESBAR				128
#define _AFX_SYSPOLICY_NOBACKBUTTON				256
#define _AFX_SYSPOLICY_NOFILEMRU				512

struct _AfxSysPolicyData
{
	LPCTSTR szPolicyName;
	DWORD dwID;
};

struct _AfxSysPolicies
{
	LPCTSTR szPolicyKey;
	_AfxSysPolicyData *pData;
};

// Restart Manager support flags
#define AFX_RESTART_MANAGER_SUPPORT_RESTART				0x01  // restart support, means application is registered via RegisterApplicationRestart
#define AFX_RESTART_MANAGER_SUPPORT_RECOVERY			0x02  // recovery support, means application is registered via RegisterApplicationRecoveryCallback
#define AFX_RESTART_MANAGER_AUTOSAVE_AT_RESTART			0x04  // auto-save support is enabled, documents will be autosaved at restart by restart manager
#define AFX_RESTART_MANAGER_AUTOSAVE_AT_INTERVAL		0x08  // auto-save support is enabled, documents will be autosaved periodically for crash recovery
#define AFX_RESTART_MANAGER_REOPEN_PREVIOUS_FILES		0x10  // reopen of previously opened documents is enabled, on restart all previous documents will be opened
#define AFX_RESTART_MANAGER_RESTORE_AUTOSAVED_FILES		0x20  // restoration of auto-saved documents is enabled, on restart user will be prompted to open auto-saved documents intead of last saved
#define AFX_RESTART_MANAGER_SUPPORT_NO_AUTOSAVE			AFX_RESTART_MANAGER_SUPPORT_RESTART | AFX_RESTART_MANAGER_SUPPORT_RECOVERY | AFX_RESTART_MANAGER_REOPEN_PREVIOUS_FILES
#define AFX_RESTART_MANAGER_SUPPORT_ALL_ASPECTS			AFX_RESTART_MANAGER_SUPPORT_NO_AUTOSAVE | AFX_RESTART_MANAGER_AUTOSAVE_AT_RESTART | AFX_RESTART_MANAGER_AUTOSAVE_AT_INTERVAL | AFX_RESTART_MANAGER_RESTORE_AUTOSAVED_FILES
#define AFX_RESTART_MANAGER_SUPPORT_RESTART_ASPECTS		AFX_RESTART_MANAGER_SUPPORT_RESTART | AFX_RESTART_MANAGER_AUTOSAVE_AT_RESTART | AFX_RESTART_MANAGER_REOPEN_PREVIOUS_FILES | AFX_RESTART_MANAGER_RESTORE_AUTOSAVED_FILES
#define AFX_RESTART_MANAGER_SUPPORT_RECOVERY_ASPECTS	AFX_RESTART_MANAGER_SUPPORT_RECOVERY | AFX_RESTART_MANAGER_AUTOSAVE_AT_INTERVAL | AFX_RESTART_MANAGER_REOPEN_PREVIOUS_FILES | AFX_RESTART_MANAGER_RESTORE_AUTOSAVED_FILES

/////////////////////////////////////////////////////////////////////////////
// Extra diagnostic tracing options

#ifdef _DEBUG
extern AFX_DATA UINT afxTraceFlags;
#endif // _DEBUG

//////////////////////////////////////////////////////////////////////////////
// MessageBox helpers

void AFXAPI AfxFormatString1(CString& rString, UINT nIDS, LPCTSTR lpsz1);
void AFXAPI AfxFormatString2(CString& rString, UINT nIDS,
				LPCTSTR lpsz1, LPCTSTR lpsz2);
//int AFXAPI AfxMessageBox(LPCTSTR lpszText, UINT nType = MB_OK,
//				UINT nIDHelp = 0);
//int AFXAPI AfxMessageBox(UINT nIDPrompt, UINT nType = MB_OK,
//				UINT nIDHelp = (UINT)-1);

// Implementation string helpers
void AFXAPI AfxFormatStrings(CString& rString, UINT nIDS,
				LPCTSTR const* rglpsz, int nString);
void AFXAPI AfxFormatStrings(CString& rString, LPCTSTR lpszFormat,
				LPCTSTR const* rglpsz, int nString);
BOOL AFXAPI AfxExtractSubString(CString& rString, LPCTSTR lpszFullString,
				int iSubString, TCHAR chSep = '\n');

/////////////////////////////////////////////////////////////////////////////
// Special target variant APIs

#ifdef _AFXDLL
	#include <afxdll_.h>
#endif

// Windows Version compatibility (obsolete)
#define AfxEnableWin30Compatibility()
#define AfxEnableWin31Compatibility()
#define AfxEnableWin40Compatibility()

// Temporary map management (locks temp map on current thread)
void AFXAPI AfxLockTempMaps();
BOOL AFXAPI AfxUnlockTempMaps(BOOL bDeleteTemps = TRUE);

/////////////////////////////////////////////////////////////////////////////
// Special OLE related functions (see OLELOCK.CPP)

void AFXAPI AfxOleOnReleaseAllObjects();
BOOL AFXAPI AfxOleCanExitApp();
void AFXAPI AfxOleLockApp();
void AFXAPI AfxOleUnlockApp();

// void AFXAPI AfxOleSetUserCtrl(BOOL bUserCtrl);
// BOOL AFXAPI AfxOleGetUserCtrl();

#ifndef _AFX_NO_OCC_SUPPORT
BOOL AFXAPI AfxOleLockControl(REFCLSID clsid);
BOOL AFXAPI AfxOleUnlockControl(REFCLSID clsid);
BOOL AFXAPI AfxOleLockControl(LPCTSTR lpszProgID);
BOOL AFXAPI AfxOleUnlockControl(LPCTSTR lpszProgID);
void AFXAPI AfxOleUnlockAllControls();
#endif

/////////////////////////////////////////////////////////////////////////////
// Use version 1.0 of the RichEdit control

#define _RICHEDIT_VER 0x0210

/////////////////////////////////////////////////////////////////////////////
// Inline function declarations

#ifndef GET_X_LPARAM
#define GET_X_LPARAM(lp)                        ((int)(short)LOWORD(lp))
#endif
#ifndef GET_Y_LPARAM
#define GET_Y_LPARAM(lp)                        ((int)(short)HIWORD(lp))
#endif

#ifdef _AFX_PACKING
#pragma pack(pop)
#endif

#include <afxcomctl32.inl>

#ifdef _AFX_ENABLE_INLINES
#define _AFXWIN_INLINE AFX_INLINE
#include <afxwin1.inl>
#include <afxwin2.inl>
#include <afxwin3.inl>
#endif

#include <afxwin4.inl>

#undef AFX_DATA
#define AFX_DATA

#ifdef _AFX_MINREBUILD
//#pragma component(minrebuild, on)
#endif

/////////////////////////////////////////////////////////////////////////////

#else //RC_INVOKED
#include <afxres.h>     // standard resource IDs
#endif //RC_INVOKED


#ifdef _M_CEE
    #include <atliface.h>
    #include <afxole.h>
#endif

#pragma warning( pop )

#endif //__AFXWIN_H__

/////////////////////////////////////////////////////////////////////////////


