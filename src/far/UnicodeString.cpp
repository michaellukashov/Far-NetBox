/*
UnicodeString.hpp

Unicode строки
*/
/*
Copyright © 1996 Eugene Roshal
Copyright © 2000 Far Group
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "headers.hpp"
#include "UnicodeString.hpp"
#include "Sysutils.h"
#pragma hdrstop

#if 0
UnicodeStringData * eus()
{
  //для оптимизации создания пустых UnicodeString
  static UnicodeStringData * EmptyUnicodeStringData = new UnicodeStringData(1,1);
  return EmptyUnicodeStringData;
}

void UnicodeString::SetEUS()
{
  m_pData = eus();
  m_pData->AddRef();
}

void UnicodeString::Inflate(size_t nSize)
{
  if (m_pData->GetRef() == 1)
  {
    m_pData->Inflate(nSize);
  }
  else
  {
    UnicodeStringData * pNewData = new UnicodeStringData(nSize);
    size_t nNewLength = Min(m_pData->GetLength(),nSize-1);
    wmemcpy(pNewData->GetData(),m_pData->GetData(),nNewLength);
    pNewData->SetLength(nNewLength);
    m_pData->DecRef();
    m_pData = pNewData;
  }
}

size_t UnicodeString::GetCharString(char * lpszStr, size_t nSize, UINT CodePage) const
{
  if (!lpszStr)
    return 0;

  size_t nCopyLength = (nSize <= m_pData->GetLength()+1 ? nSize-1 : m_pData->GetLength());
  WideCharToMultiByte(CodePage,0,m_pData->GetData(),(int)nCopyLength,lpszStr,(int)nCopyLength+1,nullptr,nullptr);
  lpszStr[nCopyLength] = 0;
  return nCopyLength+1;
}

UnicodeString & UnicodeString::Replace(size_t Pos, size_t Len, const wchar_t * Data, size_t DataLen)
{
  // Pos & Len must be valid
  assert(Pos <= m_pData->GetLength());
  assert(Len <= m_pData->GetLength());
  assert(Pos + Len <= m_pData->GetLength());
  // Data and *this must not intersect (but Data can be located entirely within *this)
  assert(!(Data < m_pData->GetData() && Data + DataLen > m_pData->GetData()));
  assert(!(Data < m_pData->GetData() + m_pData->GetLength() && Data + DataLen > m_pData->GetData() + m_pData->GetLength()));

  if (!Len && !DataLen)
    return *this;

  if (DataLen == static_cast<size_t>(-1))
  {
    DataLen = StrLength(Data);
  }
  size_t NewLength = m_pData->GetLength() + DataLen - Len;

  if (m_pData->GetRef() == 1 && NewLength + 1 <= m_pData->GetSize())
  {
    if (NewLength)
    {
      if (Data >= m_pData->GetData() && Data + DataLen <= m_pData->GetData() + m_pData->GetLength())
      {
        // copy data from self
        UnicodeString TmpStr(Data, DataLen);
        wmemmove(m_pData->GetData() + Pos + DataLen, m_pData->GetData() + Pos + Len, m_pData->GetLength() - Pos - Len);
        wmemcpy(m_pData->GetData() + Pos, TmpStr.CPtr(), TmpStr.GetLength());
      }
      else
      {
        wmemmove(m_pData->GetData() + Pos + DataLen, m_pData->GetData() + Pos + Len, m_pData->GetLength() - Pos - Len);
        wmemcpy(m_pData->GetData() + Pos, Data, DataLen);
      }
    }

    m_pData->SetLength(NewLength);
  }
  else
  {
    if (!NewLength)
    {
      m_pData->DecRef();
      SetEUS();
      return *this;
    }

    UnicodeStringData * NewData = new UnicodeStringData(NewLength + 1);
    wmemcpy(NewData->GetData(), m_pData->GetData(), Pos);
    wmemcpy(NewData->GetData() + Pos, Data, DataLen);
    wmemcpy(NewData->GetData() + Pos + DataLen, m_pData->GetData() + Pos + Len, m_pData->GetLength() - Pos - Len);
    NewData->SetLength(NewLength);
    m_pData->DecRef();
    m_pData = NewData;
  }

  return *this;
}

UnicodeString & UnicodeString::Append(const char * lpszAdd, UINT CodePage)
{
  if (lpszAdd && *lpszAdd)
  {
    size_t nAddSize = MultiByteToWideChar(CodePage,0,lpszAdd,-1,nullptr,0);
    size_t nNewLength = m_pData->GetLength() + nAddSize - 1;
    Inflate(nNewLength + 1);
    MultiByteToWideChar(CodePage,0,lpszAdd,(int)nAddSize,m_pData->GetData() + m_pData->GetLength(),(int)m_pData->GetSize());
    m_pData->SetLength(nNewLength);
  }

  return *this;
}

UnicodeString & UnicodeString::Copy(const UnicodeString & Str)
{
  if (Str.m_pData != m_pData)
  {
    m_pData->DecRef();
    m_pData = Str.m_pData;
    m_pData->AddRef();
  }

  return *this;
}

UnicodeString & UnicodeString::Copy(const char * lpszData, UINT CodePage)
{
  m_pData->DecRef();

  if (!lpszData || !*lpszData)
  {
    SetEUS();
  }
  else
  {
    size_t nSize = MultiByteToWideChar(CodePage,0,lpszData,-1,nullptr,0);
    m_pData = new UnicodeStringData(nSize);
    MultiByteToWideChar(CodePage,0,lpszData,-1,m_pData->GetData(),(int)m_pData->GetSize());
    m_pData->SetLength(nSize - 1);
  }

  return *this;
}

UnicodeString UnicodeString::SubStr(size_t Pos, size_t Len) const
{
  if (Pos >= GetLength())
    return UnicodeString();
  if (Len == static_cast<size_t>(-1) || Len > GetLength() || Pos + Len > GetLength())
    Len = GetLength() - Pos;
  return UnicodeString(m_pData->GetData() + Pos, Len);
}

bool UnicodeString::IsSubStrAt(size_t Pos, size_t Len, const wchar_t * Data, size_t DataLen) const
{
  if (Pos >= m_pData->GetLength())
    Len = 0;
  else if (Len >= m_pData->GetLength() || Pos + Len >= m_pData->GetLength())
    Len = m_pData->GetLength() - Pos;

  if (Len != DataLen)
    return false;

  return !wmemcmp(m_pData->GetData() + Pos, Data, Len);
}

const UnicodeString operator+(const UnicodeString & strSrc1, const UnicodeString & strSrc2)
{
  return UnicodeString(strSrc1).Append(strSrc2);
}

const UnicodeString operator+(const UnicodeString & strSrc1, const char * lpszSrc2)
{
  return UnicodeString(strSrc1).Append(lpszSrc2);
}

const UnicodeString operator+(const UnicodeString & strSrc1, const wchar_t * lpwszSrc2)
{
  return UnicodeString(strSrc1).Append(lpwszSrc2);
}

const UnicodeString operator+(const UnicodeString & strSrc1, const std::wstring & strSrc2)
{
  return UnicodeString(strSrc1).Append(strSrc2.c_str());
}

const UnicodeString operator+(const UnicodeString & strSrc1, wchar_t ch)
{
  return UnicodeString(strSrc1).Append(&ch, 1);
}

const UnicodeString operator+(wchar_t ch, const UnicodeString & strSrc1)
{
  return UnicodeString(strSrc1).Append(&ch, 1);
}

wchar_t * UnicodeString::GetBuffer(size_t nSize)
{
  Inflate(nSize == (size_t)-1?m_pData->GetSize():nSize);
  return m_pData->GetData();
}

void UnicodeString::ReleaseBuffer(size_t nLength)
{
  if (nLength == (size_t)-1)
    nLength = StrLength(m_pData->GetData());

  if (nLength >= m_pData->GetSize())
    nLength = m_pData->GetSize() - 1;

  m_pData->SetLength(nLength);
}

size_t UnicodeString::SetLength(size_t nLength)
{
  if (nLength < m_pData->GetLength())
  {
    if (!nLength && m_pData->GetRef() > 1)
    {
      m_pData->DecRef();
      SetEUS();
    }
    else
    {
      Inflate(nLength+1);
      return m_pData->SetLength(nLength);
    }
  }
  /*else
  {
    // size_t nNewSize = 0;
    Inflate(nLength+1); //, nNewSize);
    return m_pData->SetLength(nLength);
  }*/

  return m_pData->GetLength();
}

UnicodeString & UnicodeString::Clear()
{
  if (m_pData->GetRef() > 1)
  {
    m_pData->DecRef();
    SetEUS();
  }
  else
  {
    m_pData->SetLength(0);
  }

  return *this;
}

int CDECL UnicodeString::Format(const wchar_t * format, ...)
{
  wchar_t * buffer = nullptr;
  size_t Size = MAX_PATH;
  int retValue = -1;
  va_list argptr;
  va_start(argptr, format);

  do
  {
    Size <<= 1;
    wchar_t * tmpbuffer = (wchar_t *)xf_realloc_nomove(buffer, Size*sizeof(wchar_t));

    if (!tmpbuffer)
    {
      va_end(argptr);
      xf_free(buffer);
      return retValue;
    }

    buffer = tmpbuffer;
    //_vsnwprintf не всегда ставит '\0' вконце.
    //Поэтому надо обнулить и передать в _vsnwprintf размер-1.
    buffer[Size-1] = 0;
    retValue = _vsnwprintf(buffer, Size-1, format, argptr);
  }
  while (retValue == -1);

  va_end(argptr);
  Copy(buffer);
  xf_free(buffer);
  return retValue;
}

UnicodeString & UnicodeString::Lower(size_t nStartPos, size_t nLength)
{
  Inflate(m_pData->GetSize());
  CharLowerBuffW(m_pData->GetData()+nStartPos, nLength==(size_t)-1?(DWORD)(m_pData->GetLength()-nStartPos):(DWORD)nLength);
  return *this;
}

UnicodeString & UnicodeString::Upper(size_t nStartPos, size_t nLength)
{
  Inflate(m_pData->GetSize());
  CharUpperBuffW(m_pData->GetData()+nStartPos, nLength==(size_t)-1?(DWORD)(m_pData->GetLength()-nStartPos):(DWORD)nLength);
  return *this;
}

bool UnicodeString::Pos(size_t & nPos, wchar_t Ch, size_t nStartPos) const
{
  const wchar_t * lpwszStr = wcschr(m_pData->GetData()+nStartPos,Ch);

  if (lpwszStr)
  {
    nPos = lpwszStr - m_pData->GetData();
    return true;
  }

  return false;
}

bool UnicodeString::Pos(size_t & nPos, const wchar_t * lpwszFind, size_t nStartPos) const
{
  const wchar_t * lpwszStr = wcsstr(m_pData->GetData()+nStartPos,lpwszFind);

  if (lpwszStr)
  {
    nPos = lpwszStr - m_pData->GetData();
    return true;
  }

  return false;
}

bool UnicodeString::PosI(size_t & nPos, const wchar_t * lpwszFind, size_t nStartPos) const
{
  const wchar_t * lpwszStr = StrStrI(m_pData->GetData()+nStartPos,lpwszFind);

  if (lpwszStr)
  {
    nPos = lpwszStr - m_pData->GetData();
    return true;
  }

  return false;
}

bool UnicodeString::RPos(size_t & nPos, wchar_t Ch, size_t nStartPos) const
{
  const wchar_t * lpwszStrStart = m_pData->GetData()+nStartPos;
  const wchar_t * lpwszStrEnd = m_pData->GetData()+m_pData->GetLength();

  do
  {
    if (*lpwszStrEnd == Ch)
    {
      nPos = lpwszStrEnd - m_pData->GetData();
      return true;
    }

    lpwszStrEnd--;
  }
  while (lpwszStrEnd >= lpwszStrStart);

  return false;
}

void  __cdecl UnicodeString::ThrowIfOutOfRange(int idx) const
{
  if (idx < 1 || idx > Length())    // NOTE: UnicodeString is 1-based !!
    throw std::runtime_error("Index is out of range"); // ERangeError(Sysconst_SRangeError);
}

int UnicodeString::Pos(wchar_t Ch) const
{
  // bool Pos(size_t &nPos, wchar_t Ch, size_t nStartPos=0) const;
  size_t nPos = 0;
  if (Pos(nPos, Ch))
  {
    return nPos;
  }
  return 0;
}

int UnicodeString::Pos(const wchar_t * str) const
{
  size_t nPos = 0;
  if (Pos(nPos, str))
  {
    return nPos;
  }
  return 0;
}

int UnicodeString::RPos(wchar_t Ch) const
{
  size_t nPos = 0;
  if (RPos(nPos, Ch))
  {
    return nPos;
  }
  return 0;
}

int UnicodeString::CompareIC(const UnicodeString str) const
{
  return Sysutils::AnsiCompareIC(*this, str);
}

int UnicodeString::ToInt() const
{
  return Sysutils::StrToIntDef(*this, 0);
}

bool UnicodeString::IsDelimiter(UnicodeString Chars, int Pos) const
{
  return Sysutils::IsDelimiter(*this, Chars, Pos);
}

int UnicodeString::LastDelimiter(const UnicodeString & delimiters) const
{
  return Sysutils::LastDelimiter(*this, delimiters);
}

UnicodeString UnicodeString::Trim() const
{
  return Sysutils::Trim(*this);
}

UnicodeString UnicodeString::TrimLeft() const
{
  return Sysutils::TrimLeft(*this);
}

UnicodeString UnicodeString::TrimRight() const
{
  return Sysutils::TrimRight(*this);
}
#endif


//------------------------------------------------------------------------------

AnsiString::operator UnicodeString() const
{
  return UnicodeString(Data.c_str(), Data.size());
}

int AnsiString::Pos(wchar_t Ch) const
{
  AnsiString s(&Ch, 1);
  return Data.find(s.c_str(), 0, 1);
}

AnsiString & AnsiString::Insert(const char * Str, int Pos)
{
  Data.insert(Pos - 1, Str);
  return *this;
}

AnsiString AnsiString::SubString(int Pos, int Len) const
{
  std::string s = Data.substr(Pos - 1, Len);
  AnsiString Result(s.c_str(), s.size());
  // return AnsiString(reinterpret_cast<const char *>(s).c_str(), Len));
  return Result;
}

const AnsiString & AnsiString::operator=(const UnicodeString & strCopy)
{
  Init(strCopy.c_str(), strCopy.Length());
  return *this;
}

const AnsiString & AnsiString::operator=(const AnsiString & strCopy)
{
  Init(strCopy.c_str(), strCopy.Length());
  return *this;
}

const AnsiString & AnsiString::operator=(const UTF8String & strCopy)
{
  Init(strCopy.c_str(), strCopy.Length());
  return *this;
}

const AnsiString & AnsiString::operator=(const char * lpszData)
{
  Init(lpszData, strlen(lpszData));
  return *this;
}

const AnsiString & AnsiString::operator=(const wchar_t * lpszData)
{
  Init(lpszData, wcslen(lpszData));
  return *this;
}

AnsiString __fastcall AnsiString::operator +(const RawByteString & rhs) const
{
  std::string Result = Data + rhs.c_str();
  return AnsiString(Result.c_str(), Result.size());
}

const AnsiString & __fastcall AnsiString::operator +=(const RawByteString & rhs)
{
  Data.append(reinterpret_cast<const char *>(rhs.c_str()), rhs.size());
  return *this;
}

const AnsiString & __fastcall AnsiString::operator +=(const AnsiString & rhs)
{
  Data.append(rhs.c_str(), rhs.size());
  return *this;
}

const AnsiString & __fastcall AnsiString::operator +=(const UTF8String & rhs)
{
  Data.append(reinterpret_cast<const char *>(rhs.c_str()), rhs.size());
  return *this;
}

const AnsiString & __fastcall AnsiString::operator +=(const char Ch)
{
  Data.append(1, Ch);
  return *this;
}

//------------------------------------------------------------------------------

RawByteString::operator UnicodeString() const
{
  return UnicodeString(reinterpret_cast<const char *>(Data.c_str()), Data.size());
}

int RawByteString::Pos(wchar_t Ch) const
{
  // rawstring_t s(&Ch, 1);
  RawByteString s(&Ch, 1);
  return Data.find(reinterpret_cast<const unsigned char *>(s.c_str()), 0, 1);
}

RawByteString & RawByteString::Insert(const char * Str, int Pos)
{
  Data.insert(Pos - 1, reinterpret_cast<const unsigned char *>(Str));
  return *this;
}

RawByteString RawByteString::SubString(int Pos, int Len) const
{
  rawstring_t s = Data.substr(Pos - 1, Len);
  RawByteString Result(s.c_str(), s.size());
  // return RawByteString(reinterpret_cast<const char *>(s).c_str(), Len));
  return Result;
}

const RawByteString & RawByteString::operator=(const UnicodeString & strCopy)
{
  Init(strCopy.c_str(), strCopy.Length());
  return *this;
}

const RawByteString & RawByteString::operator=(const RawByteString & strCopy)
{
  Init(strCopy.c_str(), strCopy.Length());
  return *this;
}

const RawByteString & RawByteString::operator=(const AnsiString & strCopy)
{
  Init(strCopy.c_str(), strCopy.Length());
  return *this;
}

const RawByteString & RawByteString::operator=(const UTF8String & strCopy)
{
  Init(strCopy.c_str(), strCopy.Length());
  return *this;
}

const RawByteString & RawByteString::operator=(const std::wstring & strCopy)
{
  Init(strCopy.c_str(), strCopy.size());
  return *this;
}

const RawByteString & RawByteString::operator=(const char * lpszData)
{
  Init(lpszData, strlen(lpszData));
  return *this;
}

const RawByteString & RawByteString::operator=(const wchar_t * lpszData)
{
  Init(lpszData, wcslen(lpszData));
  return *this;
}

RawByteString __fastcall RawByteString::operator +(const RawByteString & rhs) const
{
  rawstring_t Result = Data + rhs.Data;
  return RawByteString(reinterpret_cast<const char *>(Result.c_str()), Result.size());
}

const RawByteString & __fastcall RawByteString::operator +=(const RawByteString & rhs)
{
  Data.append(reinterpret_cast<const unsigned char *>(rhs.c_str()), rhs.size());
  return *this;
}
const RawByteString & __fastcall RawByteString::operator +=(const UTF8String & rhs)
{
  Data.append(reinterpret_cast<const unsigned char *>(rhs.c_str()), rhs.size());
  return *this;
}

const RawByteString & __fastcall RawByteString::operator +=(const char Ch)
{
  unsigned char ch(static_cast<unsigned char>(Ch));
  Data.append(1, ch);
  return *this;
}

//------------------------------------------------------------------------------

UTF8String::UTF8String(const UnicodeString & Str)
{
  Init(Str.c_str(), Str.GetLength());
}

int UTF8String::Pos(wchar_t Ch) const
{
  wstring_t s(&Ch, 1);
  return Data.find(s.c_str(), 0, 1);
}

UTF8String & UTF8String::Insert(const wchar_t * Str, int Pos)
{
  Data.insert(Pos - 1, Str);
  return *this;
}

const UTF8String & UTF8String::operator=(const UnicodeString & strCopy)
{
  Init(strCopy.c_str(), strCopy.Length());
  return *this;
}

const UTF8String & UTF8String::operator=(const UTF8String & strCopy)
{
  Init(strCopy.c_str(), strCopy.Length());
  return *this;
}

const UTF8String & UTF8String::operator=(const RawByteString & strCopy)
{
  Init(strCopy.c_str(), strCopy.Length());
  return *this;
}

const UTF8String & UTF8String::operator=(const char * lpszData)
{
  Init(lpszData, strlen(lpszData));
  return *this;
}

const UTF8String & UTF8String::operator=(const wchar_t * lpszData)
{
  Init(lpszData, wcslen(lpszData));
  return *this;
}

UTF8String __fastcall UTF8String::operator +(const UTF8String & rhs) const
{
  wstring_t Result = Data + rhs.Data;
  return UTF8String(Result.c_str(), Result.size());
}

const UTF8String & __fastcall UTF8String::operator +=(const UTF8String & rhs)
{
  Data.append(rhs.Data.c_str(), rhs.size());
  return *this;
}
const UTF8String & __fastcall UTF8String::operator +=(const RawByteString & rhs)
{
  UTF8String s(rhs.c_str(), rhs.size());
  Data.append(s.Data.c_str(), s.size());
  return *this;
}

const UTF8String & __fastcall UTF8String::operator +=(const char Ch)
{
  unsigned char ch(static_cast<unsigned char>(Ch));
  Data.append(1, ch);
  return *this;
}

//------------------------------------------------------------------------------

UnicodeString & UnicodeString::Lower(int nStartPos, int nLength)
{
  // std::transform(Data.begin(), Data.end(), Data.begin(), ::toupper);
  Data = Sysutils::LowerCase(SubString(nStartPos, nLength)).c_str();
  return *this;
}

UnicodeString & UnicodeString::Upper(int nStartPos, int nLength)
{
  Data = Sysutils::UpperCase(SubString(nStartPos, nLength)).c_str();
  return *this;
}

int UnicodeString::CompareIC(const UnicodeString str) const
{
  return Sysutils::AnsiCompareIC(*this, str);
}

int UnicodeString::ToInt() const
{
  return Sysutils::StrToIntDef(*this, 0);
}

UnicodeString & UnicodeString::Replace(int Pos, int Len, const wchar_t * Str, int DataLen)
{
  Data.replace(Pos, Len, std::wstring(Str, DataLen));
  return *this;
}

UnicodeString & UnicodeString::Append(const char * lpszAdd, UINT CodePage)
{
  // Data.append();
  return *this;
}

UnicodeString & UnicodeString::Insert(int Pos, const wchar_t * Str, int StrLen)
{
  Data.insert(Pos - 1, Str, StrLen);
  return *this;
}
/*
UnicodeString & UnicodeString::Insert(int Pos, const wchar_t * Str, int StrLen)
{
  Data.insert(Pos - 1, Str, StrLen);
  return *this;
}
*/
bool UnicodeString::RPos(int & nPos, wchar_t Ch, int nStartPos) const
{
  int pos = (int)Data.find_last_of(Ch, nStartPos);
  nPos = pos + 1;
  return pos != std::wstring::npos;
}

UnicodeString UnicodeString::SubStr(int Pos, int Len) const
{
  std::wstring S(Data.substr(Pos - 1, Len));
  return UnicodeString(S);
}

bool UnicodeString::IsDelimiter(UnicodeString Chars, int Pos) const
{
  return Sysutils::IsDelimiter(*this, Chars, Pos);
}

int UnicodeString::LastDelimiter(const UnicodeString & delimiters) const
{
  return Sysutils::LastDelimiter(*this, delimiters);
}

UnicodeString UnicodeString::Trim() const
{
  return Sysutils::Trim(*this);
}

UnicodeString UnicodeString::TrimLeft() const
{
  return Sysutils::TrimLeft(*this);
}

UnicodeString UnicodeString::TrimRight() const
{
  return Sysutils::TrimRight(*this);
}

const UnicodeString & UnicodeString::operator=(const UnicodeString & strCopy)
{
  Init(strCopy.c_str(), strCopy.Length());
  return *this;
}

const UnicodeString & UnicodeString::operator=(const RawByteString & strCopy)
{
  Init(strCopy.c_str(), strCopy.Length());
  return *this;
}

const UnicodeString & UnicodeString::operator=(const AnsiString & strCopy)
{
  Init(strCopy.c_str(), strCopy.Length());
  return *this;
}

const UnicodeString & UnicodeString::operator=(const UTF8String & strCopy)
{
  Init(strCopy.c_str(), strCopy.Length());
  return *this;
}

const UnicodeString & UnicodeString::operator=(const std::wstring & strCopy)
{
  Init(strCopy.c_str(), strCopy.size());
  return *this;
}

const UnicodeString & UnicodeString::operator=(const wchar_t * Str)
{
  Init(Str, wcslen(Str));
  return *this;
}

const UnicodeString & UnicodeString::operator=(const wchar_t Ch)
{
  Init(&Ch, 1);
  return *this;
}

const UnicodeString & UnicodeString::operator=(const char * lpszData)
{
  Init(lpszData, strlen(lpszData));
  return *this;
}

UnicodeString __fastcall UnicodeString::operator +(const UnicodeString & rhs) const
{
  std::wstring Result = Data + rhs.Data;
  return UnicodeString(Result.c_str(), Result.size());
}

const UnicodeString & __fastcall UnicodeString::operator +=(const UnicodeString & rhs)
{
  Data.append(rhs.Data.c_str(), rhs.size());
  return *this;
}

const UnicodeString & __fastcall UnicodeString::operator +=(const wchar_t * rhs)
{
  Data.append(rhs);
  return *this;
}

const UnicodeString & __fastcall UnicodeString::operator +=(const RawByteString & rhs)
{
  UnicodeString s(rhs.c_str(), rhs.size());
  Data.append(s.Data.c_str(), s.size());
  return *this;
}

const UnicodeString & __fastcall UnicodeString::operator +=(const std::wstring & rhs)
{
  Data.append(rhs.c_str(), rhs.size());
  return *this;
}

const UnicodeString & __fastcall UnicodeString::operator +=(const char Ch)
{
  unsigned char ch(static_cast<unsigned char>(Ch));
  Data.append(1, ch);
  return *this;
}

void  __cdecl UnicodeString::ThrowIfOutOfRange(int idx) const
{
  if (idx < 1 || idx > Length()) // NOTE: UnicodeString is 1-based !!
    throw std::runtime_error("Index is out of range"); // ERangeError(Sysconst_SRangeError);
}

//------------------------------------------------------------------------------

UnicodeString __fastcall operator +(const wchar_t lhs, const UnicodeString & rhs)
{
  return UnicodeString(&lhs, 1) + rhs;
}

UnicodeString __fastcall operator +(const UnicodeString & lhs, const wchar_t rhs)
{
  return lhs + UnicodeString(&rhs);
}

UnicodeString __fastcall operator +(const wchar_t * lhs, const UnicodeString & rhs)
{
  return UnicodeString(lhs) + rhs;
}

UnicodeString __fastcall operator +(const UnicodeString & lhs, const wchar_t * rhs)
{
  return lhs + UnicodeString(rhs);
}

UnicodeString __fastcall operator +(const UnicodeString & lhs, const char * rhs)
{
  return lhs + UnicodeString(rhs);
}

//------------------------------------------------------------------------------
