#include "stdafx.h"
#include "Crypt.h"

//////////////////////////////////////////////////////////////////////
// Konstruktion/Destruktion
//////////////////////////////////////////////////////////////////////

const char* CCrypt::m_key = "FILEZILLA1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ";

CString CCrypt::encrypt(CString str)
{
	USES_CONVERSION;
	int pos=str.GetLength()%strlen(m_key);
	CString ret;
	LPCSTR lpszAscii=T2CA(str);
	for (unsigned int i=0;i<strlen(lpszAscii);i++)
	{
		CString tmp=ret;
		ret.Format(_T("%s%03d"),(LPCTSTR)tmp,(uint8_t)lpszAscii[i]^m_key[(i+pos)%strlen(m_key)]);
	}
	return ret;
}

CString CCrypt::decrypt(CString str)
{
	USES_CONVERSION;

	LPCSTR lpszAscii=T2CA(str);
	intptr_t pos=(strlen(lpszAscii)/3)%strlen(m_key);
	CString ret;
	TCHAR tmp[2];
	tmp[1] = 0;
	for (unsigned int i=0;i<strlen(lpszAscii)/3;i++)
	{
		int digit;
		int number = 0;
		digit = lpszAscii[i * 3];
		if (digit < '0' || digit > '9')
			return _T("");
		number += (digit - '0') * 100;
		digit = lpszAscii[i * 3 + 1];
		if (digit < '0' || digit > '9')
			return _T("");
		number += (digit - '0') * 10;
		digit = lpszAscii[i * 3 + 2];
		if (digit < '0' || digit > '9')
			return _T("");
		number += digit - '0';
		tmp[0] = (TCHAR)(number^m_key[(i+pos)%strlen(m_key)]);
		ret += tmp;
	}
	return ret;
}

