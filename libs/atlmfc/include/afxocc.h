// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#ifndef __AFXOCC_H__
#define __AFXOCC_H__

#pragma once

#ifndef _AFX_NO_OCC_SUPPORT

#include <afxtempl.h>
#include <oledb.h>
#include "olebind.h"
#include "ocdbid.h"
#include "ocdb.h"

#ifdef _AFX_MINREBUILD
#pragma component(minrebuild, off)
#endif

class CDataSourceControl;
class CDataBoundProperty;

class COccManager;
struct _AFX_OCC_DIALOG_INFO;

#define DISPID_DATASOURCE   0x80010001
#define DISPID_DATAFIELD    0x80010002


class COleControlSiteFactory;
__declspec(selectany) extern const CLSID CLSID_WinFormsControl = 
{0xb7e7a666,0xd623,0x457f,{0xa3,0x0a,0x6a,0x49,0xa3,0xe5,0xb4,0x70}};
/////////////////////////////////////////////////////////////////////////////
// Control site factory interface - allow instantiation of different control sites

class IControlSiteFactory 
{
public:
		virtual ~IControlSiteFactory() {}
};

/////////////////////////////////////////////////////////////////////////////
// Control site factory collection manager class

class CControlSiteFactoryMgr : public CNoTrackObject {
public:
	CControlSiteFactoryMgr();
	virtual ~CControlSiteFactoryMgr();
	BOOL RegisterSiteFactory(IControlSiteFactory* pFactory);
	BOOL UnregisterSiteFactory(IControlSiteFactory* pFactory);
protected:
	CList <IControlSiteFactory*,IControlSiteFactory*> m_lstFactory;
	COleControlSiteFactory* m_pOleControlSiteDefaultFactory;
};

BOOL AFXAPI AfxRegisterSiteFactory(IControlSiteFactory* pFactory);
BOOL AFXAPI AfxUnregisterSiteFactory(IControlSiteFactory* pFactory);



class CControlCreationInfo {
public:
	CControlCreationInfo();
	enum HandleKind { ReflectionType,ControlInstance,NullHandle };
	BOOL IsManaged() const;
	HandleKind m_hk;
	intptr_t   m_nHandle;
	
	CLSID m_clsid;
};


/////////////////////////////////////////////////////////////////////////////
// Control containment helper functions

DLGTEMPLATE* _AfxSplitDialogTemplate(const DLGTEMPLATE* pTemplate,
	CMapWordToPtr* pOleItemMap);

void _AfxZOrderOleControls(CWnd* pWnd, CMapWordToPtr* pOleItemMap);

/////////////////////////////////////////////////////////////////////////////
// COleControlSiteOrWnd (helper)
struct COleControlSiteOrWnd
{
	COleControlSiteOrWnd();
	COleControlSiteOrWnd(COleControlSite *pSite);
	COleControlSiteOrWnd(HWND hWnd, BOOL bAutoRadioButton);
	~COleControlSiteOrWnd();

	DWORD GetStyle() const;

	HWND m_hWnd;
	COleControlSite *m_pSite;
	BOOL m_bAutoRadioButton;
};

COleControlSiteOrWnd* AFXAPI _AfxFindSiteOrWnd(CWnd *pWndDlg, CWnd *pWnd);

/////////////////////////////////////////////////////////////////////////////
// OLE control container manager

class COccManager : public CNoTrackObject
{
// Operations
public:
	// Event handling
	virtual BOOL OnEvent(CCmdTarget* pCmdTarget, UINT idCtrl, AFX_EVENT* pEvent,
		AFX_CMDHANDLERINFO* pHandlerInfo);

	// Dialog creation
	virtual const DLGTEMPLATE* PreCreateDialog(_AFX_OCC_DIALOG_INFO* pOccDialogInfo,
		const DLGTEMPLATE* pOrigTemplate);
	virtual void PostCreateDialog(_AFX_OCC_DIALOG_INFO* pOccDialogInfo);
	virtual DLGTEMPLATE* SplitDialogTemplate(const DLGTEMPLATE* pTemplate,
		DLGITEMTEMPLATE** ppOleDlgItems);
	virtual BOOL CreateDlgControls(CWnd* pWndParent, LPCTSTR lpszResourceName,
		_AFX_OCC_DIALOG_INFO* pOccDialogInfo);
	virtual BOOL CreateDlgControls(CWnd* pWndParent, void* lpResource,
		_AFX_OCC_DIALOG_INFO* pOccDialogInfo);

	// Dialog manager
	virtual BOOL IsDialogMessage(CWnd* pWndDlg, LPMSG lpMsg);
	static BOOL AFX_CDECL IsLabelControl(CWnd* pWnd);
	static BOOL AFX_CDECL IsLabelControl(COleControlSiteOrWnd* pWnd);
	static BOOL AFX_CDECL IsMatchingMnemonic(CWnd* pWnd, LPMSG lpMsg);
	static BOOL AFX_CDECL IsMatchingMnemonic(COleControlSiteOrWnd* pWnd, LPMSG lpMsg);
	static void AFX_CDECL SetDefaultButton(CWnd* pWnd, BOOL bDefault);
	static DWORD AFX_CDECL GetDefBtnCode(CWnd* pWnd);

// Implementation
protected:
	// Dialog creation
	BOOL CreateDlgControl(CWnd* pWndParent, HWND hwAfter, BOOL bDialogEx,
		LPDLGITEMTEMPLATE pDlgItem, WORD nMsg, BYTE* lpData, DWORD cb, HWND* phWnd);

	// Databinding
	void BindControls(CWnd* pWndParent);

	// Dialog manager
	static void AFX_CDECL UIActivateControl(CWnd* pWndNewFocus);
	static void AFX_CDECL UIDeactivateIfNecessary(CWnd* pWndOldFocus, CWnd* pWndNewFocus);
};

struct _AFX_OCC_DIALOG_INFO
{
	DLGTEMPLATE* m_pNewTemplate;
	DLGITEMTEMPLATE** m_ppOleDlgItems;

	unsigned m_cItems;
	struct ItemInfo
	{
		unsigned nId;
		BOOL bAutoRadioButton;
	};
	ItemInfo *m_pItemInfo;
};

#ifdef _AFX_MINREBUILD
#pragma component(minrebuild, on)
#endif

#endif // !_AFX_NO_OCC_SUPPORT
#endif // __AFXOCC_H__
