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
// Fusion: Dlls that have WinSxS assemblies and several versions can be loaded
// into the same process, and called by mfc, need special wrappers to allow the
// same mfc dll to call the correct version, stored in the module state of the
// user exe/dll.

#ifndef __AFXCOMCTL32_H__
#define __AFXCOMCTL32_H__

#pragma once

#if _WIN32_WINNT < 0x0500
#error This file requires _WIN32_WINNT to be #defined at least to 0x0500. Value 0x0501 or higher is recommended.
#endif


#pragma warning(disable: 4127)  // conditional expression constant

/////////////////////////////////////////////////////////////////////////////
// (WinSxS/Manifest) API.



enum eActCtxResult { ActCtxFailed, ActCtxSucceeded, ActCtxNoFusion };

BOOL AFXAPI AfxGetAmbientActCtx();
void AFXAPI AfxSetAmbientActCtx(BOOL bSet);
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Base class for all dll wrappers
//
class CDllIsolationWrapperBase : public CNoTrackObject
{
public:
	HMODULE m_hModule;
	bool m_bFreeLib;
protected:
	CString m_strModuleName;
public:
	HMODULE GetModuleHandle()
	{
		if (m_hModule == NULL)
		{
			m_hModule = ::GetModuleHandle(m_strModuleName.GetString());
			if (m_hModule == NULL)
			{
				m_hModule = ::LoadLibrary(m_strModuleName.GetString());
				m_bFreeLib = m_hModule != NULL;
			}
		}
		return m_hModule;
	}

public:
	CDllIsolationWrapperBase()
	{
		CommonConstruct();
	}
	CDllIsolationWrapperBase(const CString& strModuleName) 
	: m_strModuleName(strModuleName)
	{
		CommonConstruct();
	}

	void CommonConstruct()
	{
		m_hModule  = NULL;
		m_bFreeLib = false;
	}
	virtual ~CDllIsolationWrapperBase()
	{ 
		m_bFreeLib && ::FreeLibrary(m_hModule);
	}

};

class CComCtlWrapper : public CDllIsolationWrapperBase
{
public:
	CComCtlWrapper() 
	: CDllIsolationWrapperBase(_T("comctl32.dll"))
	{
	}

public:
};
/////////////////////////////////////////////////////////////////////////////

#ifdef _UNICODE
#define AfxCreateStatusWindow AfxCreateStatusWindowW
#define AfxDrawStatusText AfxDrawStatusTextW
#define AfxImageList_LoadImage AfxImageList_LoadImageW
#define AfxCreatePropertySheetPage AfxCreatePropertySheetPageW
#define AfxPropertySheet AfxPropertySheetW
#else
#define AfxCreateStatusWindow AfxCreateStatusWindowA
#define AfxDrawStatusText AfxDrawStatusTextA
#define AfxImageList_LoadImage AfxImageList_LoadImageA
#define AfxCreatePropertySheetPage AfxCreatePropertySheetPageA
#define AfxPropertySheet AfxPropertySheetA
#endif

////////////////////// commdlg.h //////////////////////////////////////////
#ifdef _UNICODE
#define AfxCtxGetOpenFileName AfxCtxGetOpenFileNameW
#define AfxCtxGetSaveFileName AfxCtxGetSaveFileNameW
//#define AfxCtxGetFileTitle	  AfxCtxGetFileTitleW
#define AfxCtxChooseColor	  AfxCtxChooseColorW
#define AfxCtxFindText		  AfxCtxFindTextW
#define AfxCtxReplaceText	  AfxCtxReplaceTextW
#define AfxCtxChooseFont	  AfxCtxChooseFontW
#define AfxCtxPrintDlg		  AfxCtxPrintDlgW
#define AfxCtxCommDlgExtendedError AfxCtxCommDlgExtendedErrorW
#define AfxCtxPageSetupDlg	  AfxCtxPageSetupDlgW
#define AfxCtxPrintDlgEx	  AfxCtxPrintDlgExW
#else // ANSI
#define AfxCtxGetOpenFileName AfxCtxGetOpenFileNameA
#define AfxCtxGetSaveFileName AfxCtxGetSaveFileNameA
//#define AfxCtxGetFileTitle	  AfxCtxGetFileTitleA
#define AfxCtxChooseColor	  AfxCtxChooseColorA
#define AfxCtxFindText		  AfxCtxFindTextA
#define AfxCtxReplaceText	  AfxCtxReplaceTextA
#define AfxCtxChooseFont	  AfxCtxChooseFontA
#define AfxCtxPrintDlg		  AfxCtxPrintDlgA
#define AfxCtxCommDlgExtendedError AfxCtxCommDlgExtendedErrorA
#define AfxCtxPageSetupDlg	  AfxCtxPageSetupDlgA
#define AfxCtxPrintDlgEx	  AfxCtxPrintDlgExA
#endif


////////////////////// WinUser.inl //////////////////////////////////////////
#ifdef _UNICODE
#define AfxCtxRegisterClass   AfxCtxRegisterClassW
#define AfxCtxUnregisterClass AfxCtxUnregisterClassW
#define AfxCtxGetClassInfo   AfxCtxGetClassInfoW
#define AfxCtxRegisterClassEx AfxCtxRegisterClassExW
#define AfxCtxGetClassInfoEx AfxCtxGetClassInfoExW
#define AfxCtxCreateWindowEx AfxCtxCreateWindowExW
#define AfxCtxCreateDialogParam AfxCtxCreateDialogParamW
#define AfxCtxCreateDialogIndirectParam AfxCtxCreateDialogIndirectParamW
#define AfxCtxDialogBoxParam AfxCtxDialogBoxParamW
#define AfxCtxDialogBoxIndirectParam AfxCtxDialogBoxIndirectParamW
#define AfxCtxMessageBox AfxCtxMessageBoxW
#define AfxCtxMessageBoxEx AfxCtxMessageBoxExW
#define AfxCtxMessageBoxIndirect AfxCtxMessageBoxIndirectW
#else // ANSI
#define AfxCtxRegisterClass   AfxCtxRegisterClassA
#define AfxCtxUnregisterClass AfxCtxUnregisterClassA
#define AfxCtxGetClassInfo   AfxCtxGetClassInfoA
#define AfxCtxRegisterClassEx AfxCtxRegisterClassExA
#define AfxCtxGetClassInfoEx AfxCtxGetClassInfoExA
#define AfxCtxCreateWindowEx AfxCtxCreateWindowExA
#define AfxCtxCreateDialogParam AfxCtxCreateDialogParamA
#define AfxCtxCreateDialogIndirectParam AfxCtxCreateDialogIndirectParamA
#define AfxCtxDialogBoxParam AfxCtxDialogBoxParamA
#define AfxCtxDialogBoxIndirectParam AfxCtxDialogBoxIndirectParamA
#define AfxCtxMessageBox AfxCtxMessageBoxA
#define AfxCtxMessageBoxEx AfxCtxMessageBoxExA
#define AfxCtxMessageBoxIndirect AfxCtxMessageBoxIndirectA
#endif

////////////////////// WinBase.inl //////////////////////////////////////////
//Only the funcs that actually change in winbase.inl context are in this list.
// AFX_ISOLATIONAWARE_STATICLINK_FUNC(HMODULE,LoadLibraryA,(LPCSTR lpLibFileName),(lpLibFileName),NULL)
// AFX_ISOLATIONAWARE_STATICLINK_FUNC(HMODULE,LoadLibraryW,(LPCWSTR lpLibFileName),(lpLibFileName),NULL)
// AFX_ISOLATIONAWARE_STATICLINK_FUNC(HMODULE,LoadLibraryExA,(LPCSTR lpLibFileName,HANDLE hFile,DWORD dwFlags),(lpLibFileName,hFile,dwFlags),NULL)
// AFX_ISOLATIONAWARE_STATICLINK_FUNC(HMODULE,LoadLibraryExW,(LPCWSTR lpLibFileName,HANDLE hFile,DWORD dwFlags),(lpLibFileName,hFile,dwFlags),NULL)

#ifdef _UNICODE
#define AfxCtxLoadLibrary AfxCtxLoadLibraryW
#define AfxCtxLoadLibraryEx AfxCtxLoadLibraryExW
#else // ANSI
#define AfxCtxLoadLibrary AfxCtxLoadLibraryA
#define AfxCtxLoadLibraryEx AfxCtxLoadLibraryExA
#endif
///////////////////////// ShellApi.h ////////////////////////////////////////

class CShellWrapper : public CDllIsolationWrapperBase
{
public:
	CShellWrapper() 
	: CDllIsolationWrapperBase(_T("shell32.dll"))
	{
	}
public:
	// AFX_ISOLATIONAWARE_FUNC(BOOL,InitNetworkAddressControl, (void), (),FALSE)
};

/////////////////////////////////////////////////////////////////////////////

// #pragma pop_macro("AFX_ISOLATIONAWARE_FUNC")
// #pragma pop_macro("AFX_ISOLATIONAWARE_PROC")
// #pragma pop_macro("AFX_ISOLATIONAWARE_STATICLINK_FUNC")
// #pragma pop_macro("AFX_ISOLATIONAWARE_STATICLINK_PROC")
// #pragma pop_macro("AFX_ISOLATIONAWARE_FUNC_DEACTIVATE")
// #pragma pop_macro("AFX_ISOLATIONAWARE_FUNC_ACTIVATE")
// #pragma pop_macro("AFX_ISOLATIONAWARE_COMMON_ACTIVATE")

/////////////////////////////////////////////////////////////////////////////

#endif // __AFXCOMCTL32_H__
