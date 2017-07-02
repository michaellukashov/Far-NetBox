
#pragma once
#include "stdafx.h"

class CServerPath
{
CUSTOM_MEM_ALLOCATION_IMPL
public:
  BOOL AddSubdir(CString subdir);
  const BOOL IsEmpty() const;
  CServerPath GetParent() const;
  BOOL HasParent() const;
  CString GetLastSegment() const;
  CServerPath();
  explicit CServerPath(CString path, bool trim = true);
  CServerPath(CString path, int nServerType, bool trim = true);
  CServerPath(const CServerPath &path);

  virtual ~CServerPath();

  CServerPath & operator=(CString path) { SetPath(path); return *this; }
  void SetServer(const t_server & server);
  BOOL SetPath(CString & newpath, BOOL bIsFile);
  BOOL SetPath(CString newpath);
  const CString GetPath() const;
  const CString GetPathUnterminated() const;

  CServerPath & operator=(const CServerPath & op);

  const bool operator == (const CServerPath & op) const;
  const bool operator != (const CServerPath & op) const;

  CString FormatFilename(CString fn, bool omitPath = false) const;

protected:
  BOOL m_bEmpty;
  rde::list<CString> m_Segments;
  typedef rde::list<CString>::iterator tIter;
  typedef rde::list<CString>::const_iterator tConstIter;
  CString m_Prefix;
  int m_nServerType;

private:
  const CString DoGetPath(bool unterminated) const;
};

const BOOL operator == (const CServerPath & a, const CString & b);


