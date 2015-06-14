// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

// Note: This header file contains useful classes that are documented only
//  in the MFC Technical Notes.  These classes may change from version to
//  version, so be prepared to change your code accordingly if you utilize
//  this header.  In the future, commonly used portions of this header
//  may be moved and officially documented.

#ifndef __AFXADV_H__
#define __AFXADV_H__

#ifndef __AFXWIN_H__
	#include <afxwin.h>
#endif

#ifndef __AFXTEMPL_H__
	#include <afxtempl.h>
#endif

#ifndef __objectarray_h__
	#include <ObjectArray.h>
#endif

#ifndef __shobjidl_h__
	// #include <shobjidl.h>
#endif

#pragma once

#ifdef _AFX_MINREBUILD
//#pragma component(minrebuild, off)
#endif 

#ifdef _AFX_PACKING
#pragma pack(push, _AFX_PACKING)
#endif

/////////////////////////////////////////////////////////////////////////////
// AFXADV - MFC Advanced Classes

// Classes declared in this file

//CObject
	//CFile
		//CMemFile
			class CSharedFile;          // Shared memory file

	class CRecentFileList;              // used in CWinApp for MRU list
	class CDockState;                   // state of docking toolbars
	class CJumpList;                    // Windows7 custom jump list implementation
	class CAppDestinations;             // Windows7 IApplicationDestinations implementation
/////////////////////////////////////////////////////////////////////////////

#undef AFX_DATA
#define AFX_DATA AFX_CORE_DATA

/////////////////////////////////////////////////////////////////////////////

#ifdef _AFX_PACKING
#pragma pack(pop)
#endif

#undef AFX_DATA
#define AFX_DATA

#ifdef _AFX_MINREBUILD
//#pragma component(minrebuild, on)
#endif

#endif // __AFXADV_H__

/////////////////////////////////////////////////////////////////////////////
