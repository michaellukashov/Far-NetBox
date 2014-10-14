// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

/////////////////////////////////////////////////////////////////////////////
// AFXCTL.H - MFC OLE Control support

#ifndef __AFXCTL_H__
#define __AFXCTL_H__

#pragma once

// make sure afxole.h is included first
#ifndef __AFXOLE_H__
	#include <afxole.h>
#endif

#ifdef _AFX_MINREBUILD
#pragma component(minrebuild, off)
#endif

/////////////////////////////////////////////////////////////////////////////
// Classes declared in this file

//CWinApp
//	class COleControlModule;        // Module housekeeping for an .OCX

//class CPictureHolder;               // For manipulating picture objects

//CWnd
	// class COleControl;              // OLE Control

//CDialog
	// class COlePropertyPage;         // OLE Property page

// class CPropExchange;                // Abstract base for property exchange

//CAsyncMonikerFile
	// class CDataPathProperty;        // Asynchronous properties for OLE Controls
		// class CCachedDataPathProperty;  // Cached asynchronous properties for OLE Controls

/////////////////////////////////////////////////////////////////////////////
// Set structure packing

#ifdef _AFX_PACKING
#pragma pack(push, _AFX_PACKING)
#endif

/////////////////////////////////////////////////////////////////////////////
// MFC data definition for data exported from the runtime DLL

#undef AFX_DATA
#define AFX_DATA AFX_OLE_DATA

/////////////////////////////////////////////////////////////////////////////
// COleControlModule - base class for .OCX module
//  This object is statically linked into the control.

/////////////////////////////////////////////////////////////////////////////
//  Module state macro

#define AfxGetControlModuleContext  AfxGetStaticModuleState
#define _afxModuleAddrThis AfxGetStaticModuleState()

/////////////////////////////////////////////////////////////////////////////
// Connection helper functions

BOOL AFXAPI AfxConnectionAdvise(LPUNKNOWN pUnkSrc, REFIID iid,
	LPUNKNOWN pUnkSink, BOOL bRefCount, DWORD* pdwCookie);

BOOL AFXAPI AfxConnectionUnadvise(LPUNKNOWN pUnkSrc, REFIID iid,
	LPUNKNOWN pUnkSink, BOOL bRefCount, DWORD dwCookie);

/////////////////////////////////////////////////////////////////////////////
// Event maps

enum AFX_EVENTMAP_FLAGS
{
	afxEventCustom = 0,
	afxEventStock = 1,
};

struct AFX_EVENTMAP_ENTRY
{
	AFX_EVENTMAP_FLAGS flags;
	DISPID dispid;
	LPCTSTR pszName;
	LPCSTR lpszParams;
};

struct AFX_EVENTMAP
{
#ifdef _AFXDLL
	const AFX_EVENTMAP* (PASCAL* pfnGetBaseMap)();
#else
	const AFX_EVENTMAP* lpBaseEventMap;
#endif
	const AFX_EVENTMAP_ENTRY* lpEntries;
	DWORD* lpStockEventMask;
};

#ifdef _AFXDLL
#define DECLARE_EVENT_MAP() \
private: \
	static const AFX_EVENTMAP_ENTRY _eventEntries[]; \
	static DWORD _dwStockEventMask; \
protected: \
	static const AFX_EVENTMAP eventMap; \
	static const AFX_EVENTMAP* PASCAL GetThisEventMap(); \
	virtual const AFX_EVENTMAP* GetEventMap() const; \

#define BEGIN_EVENT_MAP(theClass, baseClass) \
	const AFX_EVENTMAP* PASCAL theClass::GetThisEventMap() \
		{ return &theClass::eventMap; } \
	const AFX_EVENTMAP* theClass::GetEventMap() const \
		{ return &eventMap; } \
	AFX_COMDAT const AFX_EVENTMAP theClass::eventMap = \
		{ &(baseClass::GetThisEventMap), theClass::_eventEntries, \
			&theClass::_dwStockEventMask }; \
	AFX_COMDAT DWORD theClass::_dwStockEventMask = (DWORD)-1; \
	AFX_COMDAT const AFX_EVENTMAP_ENTRY theClass::_eventEntries[] = \
	{
#else
#define DECLARE_EVENT_MAP() \
private: \
	static const AFX_EVENTMAP_ENTRY _eventEntries[]; \
	static DWORD _dwStockEventMask; \
protected: \
	static const AFX_EVENTMAP eventMap; \
	virtual const AFX_EVENTMAP* GetEventMap() const;

#define BEGIN_EVENT_MAP(theClass, baseClass) \
	const AFX_EVENTMAP* theClass::GetEventMap() const \
		{ return &eventMap; } \
	AFX_COMDAT const AFX_EVENTMAP theClass::eventMap = \
		{ &(baseClass::eventMap), theClass::_eventEntries, \
			&theClass::_dwStockEventMask }; \
	AFX_COMDAT DWORD theClass::_dwStockEventMask = (DWORD)-1; \
	AFX_COMDAT const AFX_EVENTMAP_ENTRY theClass::_eventEntries[] = \
	{
#endif

#define END_EVENT_MAP() \
		{ afxEventCustom, DISPID_UNKNOWN, NULL, NULL }, \
	};

#define EVENT_CUSTOM(pszName, pfnFire, vtsParams) \
	{ afxEventCustom, DISPID_UNKNOWN, _T(pszName), vtsParams },

#define EVENT_CUSTOM_ID(pszName, dispid, pfnFire, vtsParams) \
	{ afxEventCustom, dispid, _T(pszName), vtsParams },

#define EVENT_PARAM(vtsParams) (BYTE*)(vtsParams)

/////////////////////////////////////////////////////////////////////////////
// Stock events

#define EVENT_STOCK_CLICK() \
	{ afxEventStock, DISPID_CLICK, _T("Click"), VTS_NONE },

#define EVENT_STOCK_DBLCLICK() \
	{ afxEventStock, DISPID_DBLCLICK, _T("DblClick"), VTS_NONE },

#define EVENT_STOCK_KEYDOWN() \
	{ afxEventStock, DISPID_KEYDOWN, _T("KeyDown"), VTS_PI2 VTS_I2 },

#define EVENT_STOCK_KEYPRESS() \
	{ afxEventStock, DISPID_KEYPRESS, _T("KeyPress"), VTS_PI2 },

#define EVENT_STOCK_KEYUP() \
	{ afxEventStock, DISPID_KEYUP, _T("KeyUp"), VTS_PI2 VTS_I2 },

#define EVENT_STOCK_MOUSEDOWN() \
	{ afxEventStock, DISPID_MOUSEDOWN, _T("MouseDown"), \
		VTS_I2 VTS_I2 VTS_XPOS_PIXELS VTS_YPOS_PIXELS },

#define EVENT_STOCK_MOUSEMOVE() \
	{ afxEventStock, DISPID_MOUSEMOVE, _T("MouseMove"), \
		VTS_I2 VTS_I2 VTS_XPOS_PIXELS VTS_YPOS_PIXELS },

#define EVENT_STOCK_MOUSEUP() \
	{ afxEventStock, DISPID_MOUSEUP, _T("MouseUp"), \
		VTS_I2 VTS_I2 VTS_XPOS_PIXELS VTS_YPOS_PIXELS },

#define EVENT_STOCK_ERROREVENT() \
	{ afxEventStock, DISPID_ERROREVENT, _T("Error"), \
		VTS_I2 VTS_PBSTR VTS_SCODE VTS_BSTR VTS_BSTR VTS_I4 VTS_PBOOL },

#define EVENT_STOCK_READYSTATECHANGE() \
	{ afxEventStock, DISPID_READYSTATECHANGE, _T("ReadyStateChange"), \
		VTS_I4 },

// Shift state values for mouse and keyboard events
#define SHIFT_MASK      0x01
#define CTRL_MASK       0x02
#define ALT_MASK        0x04

// Button values for mouse events
#define LEFT_BUTTON     0x01
#define RIGHT_BUTTON    0x02
#define MIDDLE_BUTTON   0x04

/////////////////////////////////////////////////////////////////////////////
// Stock properties

/////////////////////////////////////////////////////////////////////////////
// Stock methods

/////////////////////////////////////////////////////////////////////////////
// Macros for object factory and class ID

#define BEGIN_OLEFACTORY(class_name) \
protected: \
	class class_name##Factory : public COleObjectFactoryEx \
	{ \
	public: \
		class_name##Factory(REFCLSID clsid, CRuntimeClass* pRuntimeClass, \
			BOOL bMultiInstance, LPCTSTR lpszProgID) : \
				COleObjectFactoryEx(clsid, pRuntimeClass, bMultiInstance, \
				lpszProgID) {} \
		virtual BOOL UpdateRegistry(BOOL);

#define END_OLEFACTORY(class_name) \
	}; \
	friend class class_name##Factory; \
	static class_name##Factory factory; \
public: \
	static const GUID guid; \
	virtual HRESULT GetClassID(LPCLSID pclsid);

#define DECLARE_OLECREATE_EX(class_name) \
	BEGIN_OLEFACTORY(class_name) \
	END_OLEFACTORY(class_name)

#define IMPLEMENT_OLECREATE_EX(class_name, external_name, \
			l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
	const TCHAR _szProgID_##class_name[] = _T(external_name); \
	class_name::class_name##Factory class_name::factory( \
		class_name::guid, RUNTIME_CLASS(class_name), FALSE, \
		_szProgID_##class_name); \
	const GUID class_name::guid = \
		{ l, w1, w2, { b1, b2, b3, b4, b5, b6, b7, b8 } }; \
	HRESULT class_name::GetClassID(LPCLSID pclsid) \
		{ *pclsid = guid; return NOERROR; }

/////////////////////////////////////////////////////////////////////////////
// Macros for type name and misc status

#define DECLARE_OLECTLTYPE(class_name) \
	virtual UINT GetUserTypeNameID(); \
	virtual DWORD GetMiscStatus();

#define IMPLEMENT_OLECTLTYPE(class_name, idsUserTypeName, dwOleMisc) \
	UINT class_name::GetUserTypeNameID() { return idsUserTypeName; } \
	DWORD class_name::GetMiscStatus() { return dwOleMisc; }

/////////////////////////////////////////////////////////////////////////////
// Macros for property page IDs

#define DECLARE_PROPPAGEIDS(class_name) \
	protected: \
		virtual LPCLSID GetPropPageIDs(ULONG& cPropPages);

#define BEGIN_PROPPAGEIDS(class_name, count) \
	static CLSID _rgPropPageIDs_##class_name[count]; \
	AFX_COMDAT ULONG _cPropPages_##class_name = (ULONG)-1; \
	LPCLSID class_name::GetPropPageIDs(ULONG& cPropPages) { \
		if (_cPropPages_##class_name == (ULONG)-1) { \
			_cPropPages_##class_name = count; \
			LPCLSID pIDs = _rgPropPageIDs_##class_name; \
			ULONG iPageMax = count; \
			ULONG iPage = 0;

#define PROPPAGEID(clsid) \
			ASSERT(iPage < iPageMax); \
			if (iPage < iPageMax) \
				pIDs[iPage++] = clsid;

#define END_PROPPAGEIDS(class_name) \
			ASSERT(iPage == iPageMax); \
		} \
		cPropPages = _cPropPages_##class_name; \
		return _rgPropPageIDs_##class_name; }

/////////////////////////////////////////////////////////////////////////////
// Registry functions

BOOL AFXAPI AfxOleRegisterControlClass(HINSTANCE hInstance, REFCLSID clsid,
	LPCTSTR pszProgID, UINT idTypeName, UINT idBitmap, int nRegFlags,
	DWORD dwMiscStatus, REFGUID tlid, WORD wVerMajor, WORD wVerMinor);

BOOL AFXAPI AfxOleUnregisterClass(REFCLSID clsid, LPCTSTR pszProgID);

BOOL AFXAPI AfxOleRegisterPropertyPageClass(HINSTANCE hInstance,
	REFCLSID clsid, UINT idTypeName);

BOOL AFXAPI AfxOleRegisterPropertyPageClass(HINSTANCE hInstance,
	REFCLSID clsid, UINT idTypeName, int nRegFlags);

/////////////////////////////////////////////////////////////////////////////
// Licensing functions

BOOL AFXAPI AfxVerifyLicFile(HINSTANCE hInstance, LPCTSTR pszLicFileName,
	LPCOLESTR pszLicFileContents, UINT cch=-1);

////////////////////////////////////////////////////////////////////////////
// AfxOleTypeMatchGuid - Tests whether a given TYPEDESC matches a type with a
// given GUID, when all aliases have been expanded.

BOOL AFXAPI AfxOleTypeMatchGuid(LPTYPEINFO pTypeInfo,
	TYPEDESC* pTypeDesc, REFGUID guidType, ULONG cIndirectionLevels);

/////////////////////////////////////////////////////////////////////////////
// Inline function declarations

#ifdef _AFX_PACKING
#pragma pack(pop)
#endif

#ifdef _AFX_ENABLE_INLINES
#define _AFXCTL_INLINE AFX_INLINE
#include <afxctl.inl>
#endif

#undef AFX_DATA
#define AFX_DATA

#ifdef _AFX_MINREBUILD
#pragma component(minrebuild, on)
#endif

#endif // __AFXCTL_H__

/////////////////////////////////////////////////////////////////////////////
