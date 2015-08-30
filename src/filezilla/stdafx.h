#pragma once

//---------------------------------------------------------------------------
#define _int64 __int64
//---------------------------------------------------------------------------

#ifndef MPEXT
#define MPEXT
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif

#define MPEXT_NO_ZLIB
#define MPEXT_NO_GSS
#define MPEXT_NO_SFTP
#define MPEXT_NO_IDENT
#define MPEXT_NO_CACHE
#define MPEXT_NO_SPEED_LIM_RULES
#define _AFX_ENABLE_INLINES
#define _AFX_NOFORCE_LIBS
#define _MPT(T) _T(T)
#define _MPAT(T) T
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
#define _ATL_MIN_CRT
#define _ATL_NO_DEFAULT_LIBS

#include "afxpriv.h"
#include "afxole.h"
#include "../src/mfc/oleimpl2.h"
#include "../src/mfc/afximpl.h"

#include <afx.h>
#include "wtypes.h"
#include <afxmt.h>

//STL includes
#include <rdestl/list.h>
#include <rdestl/map.h>
#include <rdestl/vector.h>
#include <rdestl/set.h>
#include <algorithm>
//---------------------------------------------------------------------------
class CFileFix;
#define CFile CFileFix
//---------------------------------------------------------------------------
#include <FzApiStructures.h>
//---------------------------------------------------------------------------
#include <oleauto.h>
#include <afxdisp.h>
#include <afxconv.h>
//---------------------------------------------------------------------------
#define _strlwr strlwr
#ifndef USEDPARAM
#define USEDPARAM(p) (void)(p)
#endif
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
const int FILEEXISTS_ASK = -1;
const int FILEEXISTS_OVERWRITE = 0;
const int FILEEXISTS_OVERWRITEIFNEWER = 1;
const int FILEEXISTS_RESUME = 2;
const int FILEEXISTS_RENAME = 3;
const int FILEEXISTS_SKIP = 4;
const int FILEEXISTS_RESUME_ASKONFAIL = 5; // Used by queue for automatic resuming. If APPE failes, ask what to do instead.
const int FILEEXISTS_COMPLETE = 6;
//---------------------------------------------------------------------------
class t_ffam_statusmessage
{
CUSTOM_MEM_ALLOCATION_IMPL
public:
  CString status;
  int type;
  BOOL post;
};
//---------------------------------------------------------------------------
struct t_ffam_transferstatus
{
CUSTOM_MEM_ALLOCATION_IMPL
  __int64 bytes;
#ifdef MPEXT
  __int64 transfersize;
#endif
  int percent;
  int timeelapsed;
  int timeleft;
  int transferrate;
  BOOL bFileTransfer;
};
//---------------------------------------------------------------------------
#undef CFile
//---------------------------------------------------------------------------
class CFileFix : public CFile
{
CUSTOM_MEM_ALLOCATION_IMPL
public:
  explicit CFileFix() : CFile() {}
  explicit CFileFix(HANDLE hFile) : CFile(hFile) {}
  void SetCloseOnDelete(BOOL Value) { m_bCloseOnDelete = Value; }
  // MFC CFile::Read does not include file name into error message
  UINT Read(void * lpBuf, UINT nCount)
  {
    ASSERT_VALID(this);
    ASSERT(m_hFile != hFileNull);

    if (nCount == 0)
    {
      return 0;   // avoid Win32 "null-read"
    }

    ASSERT(lpBuf != NULL);
    ASSERT(AfxIsValidAddress(lpBuf, nCount));

    DWORD dwRead;
    if (!::ReadFile(m_hFile, lpBuf, nCount, &dwRead, NULL))
    {
      // The only change from MFC CFile::Read is m_strFileName
      CFileException::ThrowOsError(static_cast<LONG>(::GetLastError()), m_strFileName);
    }

    return static_cast<UINT>(dwRead);
  }

  // MFC allocates CObject (ancestor of CFile) with new, but deallocates with free,
  // what codeguard dislikes, this is fix, not sure if it is necessary for
  // release version, but probably causes no harm
  /*void operator delete(void* p)
  {
    delete p;
  }*/
};
//---------------------------------------------------------------------------
#define CFile CFileFix
//---------------------------------------------------------------------------
