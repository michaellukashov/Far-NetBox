// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#ifndef __AFXOLE_H__
#define __AFXOLE_H__

#ifdef _AFX_NO_OLE_SUPPORT
	#error OLE classes not supported in this library variant.
#endif

#ifndef __AFXEXT_H__
	#include <afxext.h>
#endif

#ifndef __AFXDISP_H__
	#include <afxdisp.h>
#endif

// include OLE Compound Document headers
#ifndef _OLE2_H_
	#include <ole2.h>
#endif

// ActiveX Document support
#ifndef __docobj_h__
	#include <docobj.h>
#endif

// URL Monikers support
#ifndef __urlmon_h__
	#include <urlmon.h>
#endif

#ifndef __AFXCOM_H__
#include <afxcom_.h>
#endif

#ifdef _AFX_MINREBUILD
#pragma component(minrebuild, off)
#endif 

#ifdef _AFX_PACKING
#pragma pack(push, _AFX_PACKING)
#endif


#ifndef _AFX_NOFORCE_LIBS

#pragma comment(lib, "urlmon.lib")

#endif // !_AFX_NOFORCE_LIBS

/////////////////////////////////////////////////////////////////////////////
// AFXOLE.H - MFC OLE support

// Classes declared in this file

//CControlBar
	class COleResizeBar;            // implements in-place resizing

//CFile
	class COleStreamFile;           // CFile wrapper for IStream interface
		class CMonikerFile;         // bound to via IMoniker
			class CAsyncMonikerFile;// asynchronous IMoniker

class COleDataObject;               // wrapper for IDataObject interface

/////////////////////////////////////////////////////////////////////////////

// AFXDLL support
#undef AFX_DATA
#define AFX_DATA AFX_OLE_DATA

/////////////////////////////////////////////////////////////////////////////
// backward compatibility

// COleClientDoc is now obsolete -- use COleDocument instead
#define COleClientDoc COleDocument

// COleServer has been replaced by the more general COleObjectFactory
#define COleServer  COleObjectFactory

/////////////////////////////////////////////////////////////////////////////
// Useful OLE specific types (some from OLE 1.0 headers)

// Codes for CallBack events
enum OLE_NOTIFICATION
{
	OLE_CHANGED,        // representation of a draw aspect has changed
	OLE_SAVED,          // the item has committed its storage
	OLE_CLOSED,         // the item has closed
	OLE_RENAMED,        // the item has changed its moniker
	OLE_CHANGED_STATE,  // the item state (open, active, etc.) has changed
	OLE_CHANGED_ASPECT, // the item draw aspect has changed
};

// Object types
enum OLE_OBJTYPE
{
	OT_UNKNOWN = 0,

	// These are OLE 1.0 types and OLE 2.0 types as returned from GetType().
	OT_LINK = 1,
	OT_EMBEDDED = 2,
	OT_STATIC = 3,

	// All OLE2 objects are written with this tag when serialized.  This
	//  differentiates them from OLE 1.0 objects written with MFC 2.0.
	//  This value will never be returned from GetType().
	OT_OLE2 = 256,
};

/////////////////////////////////////////////////////////////////////////////
// COleDataObject -- simple wrapper for IDataObject

class COleDataObject
{
// Constructors
public:
	COleDataObject();

// Operations
	void Attach(LPDATAOBJECT lpDataObject, BOOL bAutoRelease = TRUE);
	LPDATAOBJECT Detach();  // detach and get ownership of m_lpDataObject
	void Release(); // detach and Release ownership of m_lpDataObject
	BOOL AttachClipboard(); // attach to current clipboard object

// Attributes
	void BeginEnumFormats();
	BOOL GetNextFormat(LPFORMATETC lpFormatEtc);
	CFile* GetFileData(CLIPFORMAT cfFormat, LPFORMATETC lpFormatEtc = NULL);
	HGLOBAL GetGlobalData(CLIPFORMAT cfFormat, LPFORMATETC lpFormatEtc = NULL);
	BOOL GetData(CLIPFORMAT cfFormat, LPSTGMEDIUM lpStgMedium,
		LPFORMATETC lpFormatEtc = NULL);
	BOOL IsDataAvailable(CLIPFORMAT cfFormat, LPFORMATETC lpFormatEtc = NULL);

// Implementation
public:
	LPDATAOBJECT m_lpDataObject;
	LPENUMFORMATETC m_lpEnumerator;
	~COleDataObject();

	// advanced use and implementation
	LPDATAOBJECT GetIDataObject(BOOL bAddRef);
	void EnsureClipboardObject();
	BOOL m_bClipboard;      // TRUE if represents the Win32 clipboard

protected:
	BOOL m_bAutoRelease;    // TRUE if destructor should call Release

private:
	// Disable the copy constructor and assignment by default so you will get
	//   compiler errors instead of unexpected behaviour if you pass objects
	//   by value or assign objects.
	COleDataObject(const COleDataObject&);  // no implementation
	void operator=(const COleDataObject&);  // no implementation
};

/////////////////////////////////////////////////////////////////////////////
// COleDataSource -- wrapper for implementing IDataObject
//  (works similar to how data is provided on the clipboard)

struct AFX_DATACACHE_ENTRY;
class COleDropSource;

class COleDataSource : public CCmdTarget
{
// Constructors
public:
	COleDataSource();

// Operations
	void Empty();   // empty cache (similar to ::EmptyClipboard)

	// CacheData & DelayRenderData operations similar to ::SetClipboardData
	void CacheGlobalData(CLIPFORMAT cfFormat, HGLOBAL hGlobal,
		LPFORMATETC lpFormatEtc = NULL);    // for HGLOBAL based data
	void DelayRenderFileData(CLIPFORMAT cfFormat,
		LPFORMATETC lpFormatEtc = NULL);    // for CFile* based delayed render

	void SetClipboard();
	static void PASCAL FlushClipboard();
	static COleDataSource* PASCAL GetClipboardOwner();

	// Advanced: STGMEDIUM based cached data
	void CacheData(CLIPFORMAT cfFormat, LPSTGMEDIUM lpStgMedium,
		LPFORMATETC lpFormatEtc = NULL);    // for LPSTGMEDIUM based data
	// Advanced: STGMEDIUM or HGLOBAL based delayed render
	void DelayRenderData(CLIPFORMAT cfFormat, LPFORMATETC lpFormatEtc = NULL);

	// Advanced: support for SetData in COleServerItem
	//  (not generally useful for clipboard or drag/drop operations)
	void DelaySetData(CLIPFORMAT cfFormat, LPFORMATETC lpFormatEtc = NULL);

// Overidables
	virtual BOOL OnRenderGlobalData(LPFORMATETC lpFormatEtc, HGLOBAL* phGlobal);
	virtual BOOL OnRenderFileData(LPFORMATETC lpFormatEtc, CFile* pFile);
	virtual BOOL OnRenderData(LPFORMATETC lpFormatEtc, LPSTGMEDIUM lpStgMedium);
		// OnRenderFileData and OnRenderGlobalData are called by
		//  the default implementation of OnRenderData.

	virtual BOOL OnSetData(LPFORMATETC lpFormatEtc, LPSTGMEDIUM lpStgMedium,
		BOOL bRelease);
		// used only in COleServerItem implementation

// Implementation
public:
	virtual ~COleDataSource();

protected:
	AFX_DATACACHE_ENTRY* m_pDataCache;  // data cache itself
	UINT m_nMaxSize;    // current allocated size
	UINT m_nSize;       // current size of the cache
	UINT m_nGrowBy;     // number of cache elements to grow by for new allocs

	AFX_DATACACHE_ENTRY* Lookup(
		LPFORMATETC lpFormatEtc, DATADIR nDataDir) const;
	AFX_DATACACHE_ENTRY* GetCacheEntry(
		LPFORMATETC lpFormatEtc, DATADIR nDataDir);

// Interface Maps
public:
	BEGIN_INTERFACE_PART(DataObject, IDataObject)
		INIT_INTERFACE_PART(COleDataSource, DataObject)
		STDMETHOD(GetData)(LPFORMATETC, LPSTGMEDIUM);
		STDMETHOD(GetDataHere)(LPFORMATETC, LPSTGMEDIUM);
		STDMETHOD(QueryGetData)(LPFORMATETC);
		STDMETHOD(GetCanonicalFormatEtc)(LPFORMATETC, LPFORMATETC);
		STDMETHOD(SetData)(LPFORMATETC, LPSTGMEDIUM, BOOL);
		STDMETHOD(EnumFormatEtc)(DWORD, LPENUMFORMATETC*);
		STDMETHOD(DAdvise)(LPFORMATETC, DWORD, LPADVISESINK, LPDWORD);
		STDMETHOD(DUnadvise)(DWORD);
		STDMETHOD(EnumDAdvise)(LPENUMSTATDATA*);
	END_INTERFACE_PART(DataObject)

	DECLARE_INTERFACE_MAP()

	friend class COleServerItem;
};

class COleStreamFile : public CFile
{
	DECLARE_DYNAMIC(COleStreamFile)

public:
   using CFile::Open;

// Constructors and Destructors
public:
	/* explicit */ COleStreamFile(LPSTREAM lpStream = NULL);

// Operations
	// Note: OpenStream and CreateStream can accept eith STGM_ flags or
	//  CFile::OpenFlags bits since common values are guaranteed to have
	//  the same semantics.
	BOOL OpenStream(LPSTORAGE lpStorage, LPCTSTR lpszStreamName,
		DWORD nOpenFlags = modeReadWrite|shareExclusive,
		CFileException* pError = NULL);
	BOOL CreateStream(LPSTORAGE lpStorage, LPCTSTR lpszStreamName,
		DWORD nOpenFlags = modeReadWrite|shareExclusive|modeCreate,
		CFileException* pError = NULL);

	BOOL CreateMemoryStream(CFileException* pError = NULL);

	// attach & detach can be used when Open/Create functions aren't adequate
	void Attach(LPSTREAM lpStream);
	LPSTREAM Detach();

	IStream* GetStream() const;
	// Returns the current stream

// Implementation
public:
	LPSTREAM m_lpStream;
	virtual ~COleStreamFile();

	// attributes for implementation
	BOOL GetStatus(CFileStatus& rStatus) const;
	virtual ULONGLONG GetPosition() const;

	virtual const CString GetStorageName() const;

	// overrides for implementation
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

protected:
	CString m_strStorageName;
};

/////////////////////////////////////////////////////////////////////////////
// CMonikerFile - implementation of COleStreamFile that uses an IMoniker to
//                get the IStream

class CMonikerFile: public COleStreamFile
{
	DECLARE_DYNAMIC(CMonikerFile)

public:
   using COleStreamFile::Open;

public:
	CMonikerFile();

	virtual BOOL Open(LPCTSTR lpszURL, CFileException* pError=NULL);
	// Uses synchronous URLMonikers to create a moniker.
	// Opens the URL specified.
	// If provided, pError will be set in case of error.
	// Return value: TRUE if successful, FALSE otherwise.

	virtual BOOL Open(IMoniker* pMoniker, CFileException* pError=NULL);
	// Binds to the provided moniker to obtain a stream.
	// If provided, pError will be set in case of error.
	// Return value: TRUE if successful, FALSE otherwise.

	virtual void Close();
	// Detaches the stream, Release()s it, and the moniker.  Close may be
	// called on unopened, or already closed streams.

	BOOL Detach(CFileException* pError = NULL);
	// Closes the stream.  If there is an error when closing, then the
	// error code will be placed in pError and the function will return FALSE.

	IMoniker* GetMoniker() const;
	// Returns the current moniker.  The moniker returned is not AddRef()'ed.

protected:
// Overidables
	IBindCtx* CreateBindContext(CFileException* pError);
	// A hook so users can provide a particular IBindCtx, potentially one
	// on which the user has registered one or more objects.

// Implementation
protected:
	virtual BOOL Open(LPCTSTR lpszUrl, IBindHost* pBindHost,
		IBindStatusCallback* pBSC, IBindCtx* pBindCtx, CFileException* pError);
	BOOL Attach(LPCTSTR lpszUrl, IBindHost* pBindHost,
		IBindStatusCallback* pBSC, IBindCtx* pBindCtx, CFileException* pError);
	virtual BOOL Open(IMoniker* pMoniker, IBindHost* pBindHost,
		IBindStatusCallback* pBSC, IBindCtx* pBindCtx, CFileException* pError);

	BOOL Attach(IMoniker* pMoniker, IBindHost* pBindHost,
		IBindStatusCallback* pBSC, IBindCtx* pBindCtx, CFileException* pError);

	virtual BOOL PostBindToStream(CFileException* pError);

	static IBindHost* CreateBindHost();
public:
	virtual ~CMonikerFile();
	// Closes the stream, and releases the moniker if needed.

	virtual void Flush();

protected:
	IPTR(IMoniker) m_Moniker;
	// The moniker provided or created to which this class is bound.

	CMonikerFile(const CMonikerFile&);
	// Prevents copying.
};

/////////////////////////////////////////////////////////////////////////////
// CAsyncMonikerFile - implementation of COleStreamFile that uses an
//                     asynchronous IMoniker to get the IStream

class _AfxBindStatusCallback; // Forward declaration

class CAsyncMonikerFile: public CMonikerFile
{
	DECLARE_DYNAMIC(CAsyncMonikerFile)

public:
   using CMonikerFile::Open;

public:
	CAsyncMonikerFile();
	// Creates the IBindStatusCallback used internally to provide asynchronous
	// operation.

	//All Open overloads call one of these two.
	virtual BOOL Open(LPCTSTR lpszURL, IBindHost* pBindHost,
		CFileException* pError=NULL);
	virtual BOOL Open(IMoniker* pMoniker, IBindHost* pBindHost,
		CFileException* pError=NULL);

	//Open overloads that take monikers
	virtual BOOL Open(IMoniker* pMoniker, CFileException* pError=NULL);
	virtual BOOL Open(IMoniker* pMoniker, IServiceProvider* pServiceProvider,
		CFileException* pError=NULL);
	virtual BOOL Open(IMoniker* pMoniker, IUnknown* pUnknown,
		CFileException* pError=NULL);

	//Open overloads that take strings
	virtual BOOL Open(LPCTSTR lpszURL, CFileException* pError=NULL);
	virtual BOOL Open(LPCTSTR lpszURL, IServiceProvider* pServiceProvider,
		CFileException* pError=NULL);
	virtual BOOL Open(LPCTSTR lpszURL, IUnknown* pUnknown,
		CFileException* pError=NULL);

	virtual void Close();

	IBinding* GetBinding() const;
	// Returns the binding provided when the asychronous transfer begins.
	// With the IBinding*, the user may abort, or pause the transfer.
	// NULL may be returned if for any reason the transfer could not be
	// made asynchronous, or if the IBinding* has not yet been provided by
	// the system.

	FORMATETC* GetFormatEtc() const;
	// Returns the FORMATETC for the currently opened stream.  NULL will be
	// returned if this is called from outside the context of OnDataAvailable.
	// If you want to keep the FORMATETC beyond this call, make a copy of it.
	// The FORMATETC indicates the format of the data in the stream.

protected:
// Overidables
	virtual IUnknown* CreateBindStatusCallback(IUnknown* pUnkControlling);

	virtual DWORD GetBindInfo() const;
	// Returns the settings returned by IBindStatusCallback::GetBindInfo.
	// The default values returned should work for most cases and should not
	// be changed lightly.

	virtual LONG GetPriority() const;
	// Returns the priority at which the asynchronous transfer will take
	// place.  The value is one of the standard thread priority flags.
	// By default THREAD_PRIORITY_NORMAL is returned.

	virtual void OnDataAvailable(DWORD dwSize, DWORD bscfFlag);
	// Called when there is data available to be read.  dwSize indicates
	// the cumulative number of bytes which can be read.  The bscfFlag may be used
	// to identify first, last, and intermediate blocks of data.

	virtual void OnLowResource();
	// This is called when resources are low.

	virtual void OnStartBinding();
	// Called when the binding is starting up.

	virtual void OnProgress(ULONG ulProgress, ULONG ulProgressMax,
		ULONG ulStatusCode, LPCTSTR szStatusText);

	virtual void OnStopBinding(HRESULT hresult, LPCTSTR szError);
	// Called when the transfer is stopped.  This function releases the
	// IBinding and should nearly always be call when overidden.

// Implementation
public:
	virtual ~CAsyncMonikerFile();
	virtual UINT Read(void* lpBuf, UINT nCount);

protected:
	friend class _AfxBindStatusCallback;
	_AfxBindStatusCallback* m_pAfxBSCCurrent;
	BOOL m_bStopBindingReceived;
	void EndCallbacks();

	IPTR(IBinding) m_Binding;
	FORMATETC* m_pFormatEtc;

	void SetBinding(IBinding* pBinding);
	// Sets and AddRefs m_Binding

	void SetFormatEtc(FORMATETC* pFormatEtc);
	// Sets the FORMATETC for the current stream.

	virtual BOOL PostBindToStream(CFileException* pError);
};

/////////////////////////////////////////////////////////////////////////////
// COleDropSource (advanced drop source support)

class COleDropSource : public CCmdTarget
{
// Constructors
public:
	COleDropSource();

// Overridables
	virtual SCODE QueryContinueDrag(BOOL bEscapePressed, DWORD dwKeyState);
	virtual BOOL OnBeginDrag(CWnd* pWnd);

// Implementation
public:
public:
	BEGIN_INTERFACE_PART(DropSource, IDropSource)
		INIT_INTERFACE_PART(COleDropSource, DropSource)
		STDMETHOD(QueryContinueDrag)(BOOL, DWORD);
		STDMETHOD(GiveFeedback)(DWORD);
	END_INTERFACE_PART(DropSource)

	DECLARE_INTERFACE_MAP()

	CRect m_rectStartDrag;  // when mouse leaves this rect, drag drop starts
	BOOL m_bDragStarted;    // has drag really started yet?
	DWORD m_dwButtonCancel; // which button will cancel (going down)
	DWORD m_dwButtonDrop;   // which button will confirm (going up)

	// metrics for drag start determination
	static AFX_DATA UINT nDragMinDist;  // min. amount mouse must move for drag
	static AFX_DATA UINT nDragDelay;    // delay before drag starts

	friend class COleDataSource;
};

/////////////////////////////////////////////////////////////////////////////
// message map entries for OLE verbs

#define ON_STDOLEVERB(iVerb, memberFxn) \
	{ 0xC002, 0, (UINT)iVerb, (UINT)iVerb, (UINT)-1, \
		(AFX_PMSG)(BOOL (AFX_MSG_CALL CCmdTarget::*)(LPMSG, HWND, LPCRECT))&memberFxn },

#define ON_OLEVERB(idsVerbName, memberFxn) \
	{ 0xC002, 0, 1, 1, idsVerbName, \
		(AFX_PMSG)(BOOL (AFX_MSG_CALL CCmdTarget::*)(LPMSG, HWND, LPCRECT))&memberFxn },

#ifdef _DEBUG
// Mapping SCODEs to readable text
LPCTSTR AFXAPI AfxGetFullScodeString(SCODE sc);
LPCTSTR AFXAPI AfxGetScodeString(SCODE sc);
LPCTSTR AFXAPI AfxGetScodeRangeString(SCODE sc);
LPCTSTR AFXAPI AfxGetSeverityString(SCODE sc);
LPCTSTR AFXAPI AfxGetFacilityString(SCODE sc);

// Mapping IIDs to readable text
CString AFXAPI AfxGetIIDString(REFIID iid);
#endif

/////////////////////////////////////////////////////////////////////////////
// Inline function declarations

#ifdef _AFX_PACKING
#pragma pack(pop)
#endif

#ifdef _AFX_ENABLE_INLINES
#define _AFXOLE_INLINE AFX_INLINE
#define _AFXOLECLI_INLINE AFX_INLINE
#define _AFXOLESVR_INLINE AFX_INLINE
#define _AFXOLEDOBJ_INLINE AFX_INLINE
#define _AFXOLEMONIKER_INLINE AFX_INLINE
#include <afxole.inl>
#undef _AFXOLE_INLINE
#undef _AFXOLECLI_INLINE
#undef _AFXOLESVR_INLINE
#undef _AFXOLEDOBJ_INLINE
#undef _AFXOLEMONIKER_INLINE
#endif

#undef AFX_DATA
#define AFX_DATA

#ifdef _AFX_MINREBUILD
#pragma component(minrebuild, on)
#endif

#ifdef _M_CEE
    #include <afxpriv.h>
#endif

#endif //__AFXOLE_H__

/////////////////////////////////////////////////////////////////////////////x
