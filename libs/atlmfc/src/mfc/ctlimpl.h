// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#ifndef __CTLIMPL_H__
#define __CTLIMPL_H__

#pragma once

// MFC data definition for data exported from the runtime DLL

#undef AFX_DATA
#define AFX_DATA AFX_OLE_DATA

/////////////////////////////////////////////////////////////////////////////
// Codes for COleControl::SendAdvise
//......Code.........................Method called
#define OBJECTCODE_SAVED          0  //IOleAdviseHolder::SendOnSave
#define OBJECTCODE_CLOSED         1  //IOleAdviseHolder::SendOnClose
#define OBJECTCODE_RENAMED        2  //IOleAdviseHolder::SendOnRename
#define OBJECTCODE_SAVEOBJECT     3  //IOleClientSite::SaveObject
#define OBJECTCODE_DATACHANGED    4  //IDataAdviseHolder::SendOnDataChange
#define OBJECTCODE_SHOWWINDOW     5  //IOleClientSite::OnShowWindow(TRUE)
#define OBJECTCODE_HIDEWINDOW     6  //IOleClientSite::OnShowWindow(FALSE)
#define OBJECTCODE_SHOWOBJECT     7  //IOleClientSite::ShowObject
#define OBJECTCODE_VIEWCHANGED    8  //IOleAdviseHolder::SendOnViewChange


/////////////////////////////////////////////////////////////////////////////
// Typedefs

typedef LPVOID* LPLPVOID;

/////////////////////////////////////////////////////////////////////////////
// Functions

CLIPFORMAT AFXAPI _AfxGetClipboardFormatConvertVBX();
CLIPFORMAT AFXAPI _AfxGetClipboardFormatPersistPropset();
BOOL AFXAPI _AfxOleMatchPropsetClipFormat(CLIPFORMAT cfFormat, LPCLSID lpFmtID);
BOOL AFXAPI _AfxCopyPropValue(VARTYPE vtProp, void* pvDest, const void * pvSrc);
BOOL AFXAPI _AfxPeekAtClassIDInStream(LPSTREAM pstm, LPCLSID lpClassID);
BOOL AFXAPI _AfxIsSamePropValue(VARTYPE vtProp, const void* pv1, const void* pv2);
BOOL AFXAPI _AfxIsSameFont(CFontHolder& font, const FONTDESC* pFontDesc,
  LPFONTDISP pFontDispAmbient);
BOOL AFXAPI _AfxIsSameUnknownObject(REFIID iid, LPUNKNOWN pUnk1, LPUNKNOWN pUnk2);
BOOL AFXAPI _AfxInitBlob(HGLOBAL* phDst, void* pvSrc);
BOOL AFXAPI _AfxCopyBlob(HGLOBAL* phDst, HGLOBAL hSrc);
LPFONT AFXAPI _AfxCreateFontFromStream(LPSTREAM);
BOOL AFXAPI _AfxTreatAsClass(REFCLSID clsidOld, REFCLSID clsidNew);
void AFXAPI _AfxXformSizeInPixelsToHimetric(HDC, LPSIZEL, LPSIZEL);
void AFXAPI _AfxXformSizeInHimetricToPixels(HDC, LPSIZEL, LPSIZEL);
void AFXAPI _AfxDrawBorders(CDC* pDC, CRect& rc, BOOL bBorder, BOOL bClientEdge);

/////////////////////////////////////////////////////////////////////////////
// Reset MFC data definitions

#undef AFX_DATA
#define AFX_DATA

#endif  //__CTLIMPL_H__
