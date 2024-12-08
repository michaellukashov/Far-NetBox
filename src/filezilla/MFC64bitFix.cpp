
#include "stdafx.h"
#include "MFC64bitFix.h"

int64_t GetLength64(CFile & file)
{
  DWORD low;
  DWORD high;
  low=::GetFileSize(nb::ToPtr(file.m_hFile), &high);
  _int64 size=((_int64)high<<32)+low;
  return size;
}

BOOL GetLength64(CString filename, _int64 & size)
{
  WIN32_FIND_DATA findFileData;
  HANDLE hFind = FindFirstFile(filename, &findFileData);
  if (hFind == INVALID_HANDLE_VALUE)
    return FALSE;
  DebugCheck(FindClose(hFind) != FALSE);

  size=((_int64)findFileData.nFileSizeHigh<<32)+findFileData.nFileSizeLow;

  return TRUE;
}

BOOL PASCAL GetStatus64(LPCTSTR lpszFileName, CFileStatus64& rStatus)
{
  WIN32_FIND_DATA findFileData;
  HANDLE hFind = FindFirstFile((LPTSTR)lpszFileName, &findFileData);
  if (hFind == INVALID_HANDLE_VALUE)
  {
    return FALSE;
  }
  DebugCheck(FindClose(hFind) != FALSE);

  // strip attribute of NORMAL bit, our API doesn't have a "normal" bit.
  rStatus.m_attribute = (BYTE)
    (findFileData.dwFileAttributes & ~FILE_ATTRIBUTE_NORMAL);

  rStatus.m_size = ((_int64)findFileData.nFileSizeHigh<<32)+findFileData.nFileSizeLow;

  // convert times as appropriate
  TRY
  {
    rStatus.m_ctime = CTime(findFileData.ftCreationTime);
    rStatus.m_has_ctime = true;
  }
  CATCH_ALL(e)
  {
    rStatus.m_has_ctime = false;
  }
  END_CATCH_ALL;

  TRY
  {
    rStatus.m_atime = CTime(findFileData.ftLastAccessTime);
    rStatus.m_has_atime = true;
  }
  CATCH_ALL(e)
  {
    rStatus.m_has_atime = false;
  }
  END_CATCH_ALL;

  TRY
  {
    rStatus.m_mtime = CTime(findFileData.ftLastWriteTime);
    rStatus.m_has_mtime = true;
  }
  CATCH_ALL(e)
  {
    rStatus.m_has_mtime = false;
  }
  END_CATCH_ALL;

  if (!rStatus.m_has_ctime || rStatus.m_ctime.GetTime() == 0)
  {
    if (rStatus.m_has_mtime)
    {
      rStatus.m_ctime = rStatus.m_mtime;
      rStatus.m_has_ctime = true;
    }
    else
      rStatus.m_has_ctime = false;
  }


  if (!rStatus.m_has_atime || rStatus.m_atime.GetTime() == 0)
  {
    if (rStatus.m_has_mtime)
    {
      rStatus.m_atime = rStatus.m_mtime;
      rStatus.m_has_atime = true;
    }
    else
      rStatus.m_has_atime = false;
  }

  if (!rStatus.m_has_mtime || rStatus.m_mtime.GetTime() == 0)
  {
    if (rStatus.m_has_ctime)
    {
      rStatus.m_mtime = rStatus.m_ctime;
      rStatus.m_has_mtime = true;
    }
    else
      rStatus.m_has_mtime = false;
  }

  return TRUE;
}
