#pragma once

/*
UnicodeString.hpp

Unicode строка
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

#include "local.hpp"

const size_t __US_DELTA = 20;

class UnicodeStringData
{
	private:
		size_t m_nLength;
		size_t m_nSize;
		size_t m_nDelta;
		int m_nRefCount;
		wchar_t *m_pData;
		wchar_t StackBuffer[__US_DELTA];

		wchar_t *AllocData(size_t nSize, size_t& nNewSize)
		{
			if (nSize <= m_nDelta)
				nNewSize = m_nDelta;
			else if (nSize%m_nDelta > 0)
				nNewSize = (nSize/m_nDelta + 1) * m_nDelta;
			else
				nNewSize = nSize;

			return nNewSize > __US_DELTA? new wchar_t[nNewSize] : StackBuffer;
		}

		void FreeData(wchar_t *pData)
		{
			if (pData != StackBuffer)
			{
				delete [] pData;
			}
		}

	public:
		UnicodeStringData(size_t nSize=0, size_t nDelta=0)
		{
			m_nDelta = nDelta? nDelta : __US_DELTA;
			m_nLength = 0;
			m_nRefCount = 1;
			m_pData = AllocData(nSize, m_nSize);
			//“ак как ни где выше в коде мы не готовы на случай что пам€ти не хватит
			//то уж лучше и здесь не провер€ть а сразу падать
			*m_pData = 0;
		}

		size_t SetLength(size_t nLength)
		{
			//if (nLength<m_nSize) //Ёту проверку делает верхний класс, так что скажем что это оптимизаци€
			{
				m_nLength = nLength;
				m_pData[m_nLength] = 0;
			}
			return m_nLength;
		}

		void Inflate(size_t nSize)
		{
			if (nSize <= m_nSize)
				return;

			if (nSize >= m_nDelta << 3)
				nSize = nSize << 1;
			else
				nSize = (nSize/m_nDelta + 1) * m_nDelta;

			wchar_t *pOldData = m_pData;
			m_pData = AllocData(nSize, m_nSize);
			//“ак как ни где выше в коде мы не готовы на случай что пам€ти не хватит
			//то уж лучше и здесь не провер€ть а сразу падать
			wmemcpy(m_pData,pOldData,m_nLength);
			m_pData[m_nLength] = 0;
			FreeData(pOldData);
		}

		wchar_t *GetData() const { return m_pData; }
		size_t GetLength() const { return m_nLength; }
		size_t GetSize() const { return m_nSize; }

		int GetRef() const { return m_nRefCount; }
		void AddRef() { m_nRefCount++; }
		void DecRef()
		{
			m_nRefCount--;

			if (!m_nRefCount)
				delete this;
		}

		~UnicodeStringData() { FreeData(m_pData); }
};

typedef class UnicodeString
{
	private:
		UnicodeStringData *m_pData;

		void SetEUS();

	public:

		UnicodeString() { SetEUS(); }
		UnicodeString(const UnicodeString &strCopy) { SetEUS(); Copy(strCopy); }
		UnicodeString(const wchar_t *lpwszData) { SetEUS(); Copy(lpwszData); }
		UnicodeString(const wchar_t *lpwszData, size_t nLength) { SetEUS(); Copy(lpwszData, nLength); }
		UnicodeString(const char *lpszData, UINT CodePage=CP_OEMCP) { SetEUS(); Copy(lpszData, CodePage); }
		explicit UnicodeString(size_t nSize, size_t nDelta=0) { m_pData = new UnicodeStringData(nSize, nDelta); }
		UnicodeString(const std::wstring &strCopy) { SetEUS(); Copy(strCopy.c_str(), strCopy.size()); }

		~UnicodeString() { /*if (m_pData) он не должен быть nullptr*/ m_pData->DecRef(); }

		void Inflate(size_t nSize);
		wchar_t *GetBuffer(size_t nSize = (size_t)-1);
		const wchar_t *c_str() const { return m_pData->GetData(); }
		void ReleaseBuffer(size_t nLength = (size_t)-1);

        size_t  __fastcall Length() const { return GetLength(); }
		size_t GetLength() const { return m_pData->GetLength(); }
		size_t SetLength(size_t nLength);

		size_t GetSize() const { return m_pData->GetSize(); }

		wchar_t At(size_t nIndex) const { return m_pData->GetData()[nIndex]; }
		wchar_t First() const { return m_pData->GetData()[0]; }
		wchar_t Last() const { return m_pData->GetData()[m_pData->GetLength()?m_pData->GetLength()-1:0]; }

		bool IsEmpty() const { return !(m_pData->GetLength() && *m_pData->GetData()); }

		size_t GetCharString(char *lpszStr, size_t nSize, UINT CodePage=CP_OEMCP) const;

		int CDECL Format(const wchar_t * format, ...);

		UnicodeString& Replace(size_t Pos, size_t Len, const wchar_t* Data, size_t DataLen);
		UnicodeString& Replace(size_t Pos, size_t Len, const UnicodeString& Str) { return Replace(Pos, Len, Str.CPtr(), Str.GetLength()); }
		UnicodeString& Replace(size_t Pos, size_t Len, const wchar_t* Str) { return Replace(Pos, Len, Str, StrLength(NullToEmpty(Str))); }
		UnicodeString& Replace(size_t Pos, size_t Len, wchar_t Ch) { return Replace(Pos, Len, &Ch, 1); }
		UnicodeString& Replace(size_t Pos, wchar_t Ch) { return Replace(Pos, 1, &Ch, 1); }

		UnicodeString& Append(const wchar_t* Str, size_t StrLen) { return Replace(GetLength(), 0, Str, StrLen); }
		UnicodeString& Append(const UnicodeString& Str) { return Append(Str.CPtr(), Str.GetLength()); }
		UnicodeString& Append(const wchar_t* Str) { return Append(Str, StrLength(NullToEmpty(Str))); }
		UnicodeString& Append(wchar_t Ch) { return Append(&Ch, 1); }
		UnicodeString& Append(const char *lpszAdd, UINT CodePage=CP_OEMCP);

		UnicodeString& Insert(size_t Pos, const wchar_t* Str, size_t StrLen) { return Replace(Pos, 0, Str, StrLen); }
		UnicodeString& Insert(size_t Pos, const UnicodeString& Str) { return Insert(Pos, Str.CPtr(), Str.GetLength()); }
		UnicodeString& Insert(size_t Pos, const wchar_t* Str) { return Insert(Pos, Str, StrLength(NullToEmpty(Str))); }
		UnicodeString& Insert(size_t Pos, wchar_t Ch) { return Insert(Pos, &Ch, 1); }

		UnicodeString& Insert(const wchar_t *Str, size_t Pos) { return Insert(Pos, Str); }

		UnicodeString& Copy(const wchar_t *Str, size_t StrLen) { return Replace(0, GetLength(), Str, StrLen); }
		UnicodeString& Copy(const wchar_t *Str) { return Copy(Str, StrLength(NullToEmpty(Str))); }
		UnicodeString& Copy(wchar_t Ch) { return Copy(&Ch, 1); }
		UnicodeString& Copy(const UnicodeString &Str);
		UnicodeString& Copy(const char *lpszData, UINT CodePage=CP_OEMCP);

		UnicodeString& Remove(size_t Pos, size_t Len = 1) { return Replace(Pos, Len, nullptr, 0); }
        UnicodeString& Delete(size_t index, size_t count) { return Remove(index, count); }
		UnicodeString& LShift(size_t nShiftCount, size_t nStartPos=0) { return Remove(nStartPos, nShiftCount); }

		UnicodeString& Clear();

		UnicodeString& Unlink() {Inflate(m_pData->GetLength()+1); return *this;}

		const wchar_t *CPtr() const { return m_pData->GetData(); }
		operator const wchar_t *() const { return m_pData->GetData(); }

		UnicodeString SubStr(size_t Pos, size_t Len = -1) const;

		const UnicodeString& operator=(const UnicodeString &strCopy) { return Copy(strCopy); }
		const UnicodeString& operator=(const char *lpszData) { return Copy(lpszData); }
		const UnicodeString& operator=(const wchar_t *lpwszData) { return Copy(lpwszData); }
		const UnicodeString& operator=(wchar_t chData) { return Copy(chData); }

		const UnicodeString& operator+=(const UnicodeString &strAdd) { return Append(strAdd); }
		const UnicodeString& operator+=(const char *lpszAdd) { return Append(lpszAdd); }
		const UnicodeString& operator+=(const wchar_t *lpwszAdd) { return Append(lpwszAdd); }
		const UnicodeString& operator+=(wchar_t chAdd) { return Append(chAdd); }

		friend const UnicodeString operator+(const UnicodeString &strSrc1, const UnicodeString &strSrc2);
		friend const UnicodeString operator+(const UnicodeString &strSrc1, const char *lpszSrc2);
		friend const UnicodeString operator+(const UnicodeString &strSrc1, const wchar_t *lpwszSrc2);
		friend const UnicodeString operator+(const UnicodeString &strSrc1, const std::wstring &strSrc2);

		bool IsSubStrAt(size_t Pos, size_t Len, const wchar_t* Data, size_t DataLen) const;
		bool IsSubStrAt(size_t Pos, const wchar_t* Str, size_t StrLen) const { return IsSubStrAt(Pos, StrLen, Str, StrLen); }
		bool IsSubStrAt(size_t Pos, const wchar_t* Str) const { return IsSubStrAt(Pos, StrLength(Str), Str, StrLength(Str)); }
		bool IsSubStrAt(size_t Pos, const UnicodeString& Str) const { return IsSubStrAt(Pos, Str.GetLength(), Str.CPtr(), Str.GetLength()); }
		bool IsSubStrAt(size_t Pos, wchar_t Ch) const { return IsSubStrAt(Pos, 1, &Ch, 1); }
		bool operator==(const UnicodeString& Str) const { return IsSubStrAt(0, GetLength(), Str.CPtr(), Str.GetLength()); }
		bool operator==(const wchar_t* Str) const { return IsSubStrAt(0, GetLength(), Str, StrLength(Str)); }
		bool operator==(wchar_t Ch) const { return IsSubStrAt(0, GetLength(), &Ch, 1); }
		bool operator!=(const UnicodeString& Str) const { return !IsSubStrAt(0, GetLength(), Str.CPtr(), Str.GetLength()); }
		bool operator!=(const wchar_t* Str) const { return !IsSubStrAt(0, GetLength(), Str, StrLength(Str)); }
		bool operator!=(wchar_t Ch) const { return !IsSubStrAt(0, GetLength(), &Ch, 1); }

		UnicodeString& Lower(size_t nStartPos=0, size_t nLength=(size_t)-1);
		UnicodeString& Upper(size_t nStartPos=0, size_t nLength=(size_t)-1);

		bool Pos(size_t &nPos, wchar_t Ch, size_t nStartPos=0) const;
		bool Pos(size_t &nPos, const wchar_t *lpwszFind, size_t nStartPos=0) const;
		bool PosI(size_t &nPos, const wchar_t *lpwszFind, size_t nStartPos=0) const;
		bool RPos(size_t &nPos, wchar_t Ch, size_t nStartPos=0) const;

		bool Contains(wchar_t Ch, size_t nStartPos=0) const { return wcschr(m_pData->GetData()+nStartPos,Ch) != nullptr; }
		bool ContainsAny(const wchar_t *Chars, size_t nStartPos=0) const { return wcspbrk(m_pData->GetData()+nStartPos,Chars) != nullptr; }
		bool Contains(const wchar_t *lpwszFind, size_t nStartPos=0) const { return wcsstr(m_pData->GetData()+nStartPos,lpwszFind) != nullptr; }

        wchar_t __fastcall operator [](const int idx) const
        {
          ThrowIfOutOfRange(idx);   // Should Range-checking be optional to avoid overhead ??
          return m_pData->GetData()[idx-1];
        }

        wchar_t& __fastcall operator [](const int idx)
        {
          ThrowIfOutOfRange(idx);   // Should Range-checking be optional to avoid overhead ??
          // Unique();                 // Ensure we're not ref-counted (and Unicode)
          return m_pData->GetData()[idx-1];
        }
protected:
    void  __cdecl ThrowIfOutOfRange(int idx) const;
} string;

class UTF8String
{
public:
	UTF8String(const wchar_t* Str)
	{
		Init(Str, StrLength(Str));
	}

	UTF8String(const string& Str)
	{
		Init(Str, Str.GetLength());
	}

	~UTF8String()
	{
		delete[] Data;
	}

	operator const char*() const { return Data; }
	size_t size() const { return Size; }
    const char *c_str() const { return Data; }
    size_t Length() const { return Size; }


public:
    UTF8String __fastcall operator +(const UTF8String &rhs) const;
    UTF8String __fastcall operator +(const std::wstring &rhs) const;

private:
	void Init(const wchar_t *Str, size_t Length)
	{
		Size = WideCharToMultiByte(CP_UTF8, 0, Str, static_cast<int>(Length), nullptr, 0, nullptr, nullptr) + 1;
		Data = new char[Size];
		WideCharToMultiByte(CP_UTF8, 0, Str, static_cast<int>(Length), Data, static_cast<int>(Size-1), nullptr, nullptr);
		Data[Size-1] = 0;
	}

	char* Data;
	size_t Size;
};
