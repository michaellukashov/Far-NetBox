// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#include "stdafx.h"



#define new DEBUG_NEW

////////////////////////////////////////////////////////////////////////////
// Archive support for polymorphic reading/writing of CObjects

// Note: Starting with MFC 4.0, the file format written/read has been
//  extended to eliminate the previous 32k limit.  Files previously written
//  can still be read by old versions (even 16-bit versions).  In addition,
//  new files, unless they are large enough to take advantage of 32-bit tags,
//  can be read by old versions.

// Pointer mapping constants
#define wNullTag        ((WORD)0)           // special tag indicating NULL ptrs
#define wNewClassTag    ((WORD)0xFFFF)      // special tag indicating new CRuntimeClass
#define wClassTag       ((WORD)0x8000)      // 0x8000 indicates class tag (OR'd)
#define dwBigClassTag   ((DWORD)0x80000000) // 0x8000000 indicates big class tag (OR'd)
#define wBigObjectTag   ((WORD)0x7FFF)      // 0x7FFF indicates DWORD object tag
#define nMaxMapCount    ((DWORD)0x3FFFFFFE) // 0x3FFFFFFE last valid mapCount

// Note: tag value 0x8000 could be used for something in the future, since
//  it is currently an invalid tag (0x8000 means zero wClassTag, but zero
//  index is always reserved for a NULL pointer, and a NULL runtime class
//  does not make any sense).

// This is how the tags have been allocated currently:
//
//  0x0000              represents NULL pointer
//  0x0001 - 0x7FFE     "small" object tags
//  0x7FFF              header for "big" object/class tag
//  0x8000              reserved for future use
//  0x8001 - 0xFFFE     "small" class tag
//  0xFFFF              header for class definition
//
// The special value of 0x7FFF indicates that a DWORD tag follows.  This
// two part "big" tag is used for 32-bit tag values 0x7FFF and above.
// The tag value of 0x7FFF was unused in MFC versions prior to MFC 4.0.

////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
