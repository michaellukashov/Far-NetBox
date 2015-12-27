#pragma once

class CCrypt  
{
  static const char *m_key;
public:
  static CString decrypt(CString str);
  static CString encrypt(CString str);
};

