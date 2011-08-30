#pragma once

#include "stdafx.h"

#include <WinDef.h>

#pragma warning(push, 1)
#include <string>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <iterator>
#include <algorithm>
#include <assert.h>

#pragma warning(pop)

//TODO: remove
using namespace std;

//---------------------------------------------------------------------------
class TObject;
typedef void (TObject::*TThreadMethod)();
typedef void (TObject::*TNotifyEvent)(TObject *);
//---------------------------------------------------------------------------
class TObject
{
public:
    TObject() :
        FOwnsObjects(false)
    {}
    virtual ~TObject()
    {}

    virtual void Change()
    {}
    void OwnsObjects(bool value) { FOwnsObjects = value; }

private:
    bool FOwnsObjects;
};

struct TPoint
{
    int x;
    int y;
    TPoint() :
        x(0),
        y(0)
    {}
    TPoint(int x, int y) :
        x(x),
        y(y)
    {}
};

struct TRect
{
    int Left;
    int Top;
    int Right;
    int Bottom;
    int Width() const { return Right - Left; }
    int Height() const { return Bottom - Top; }
    TRect() :
        Left(0),
        Top(0),
        Right(0),
        Bottom(0)
    {}
    TRect(int left, int top, int right, int bottom) :
        Left(left),
        Top(top),
        Right(right),
        Bottom(bottom)
    {}
    bool operator == (const TRect &other)
    {
        return
            Left == other.Left &&
            Top == other.Top &&
            Right == other.Right &&
            Bottom == other.Bottom;
    }
    bool operator != (const TRect &other)
    {
        return !(operator == (other));
    }
    bool operator == (const RECT &other)
    {
        return
            Left == other.left &&
            Top == other.top &&
            Right == other.right &&
            Bottom == other.bottom;
    }
    bool operator != (const RECT &other)
    {
        return !(operator == (other));
    }
};

class TPersistent : public TObject
{
public:
    virtual void Assign(TPersistent *Source)
    {}
};

typedef int (CompareFunc)(void * Item1, void * Item2);

class TObjectList : public TPersistent
{
public:
    size_t GetCount() const { return m_objects.size(); }
    void SetCount(size_t value)
    {}

    TObject * operator [](size_t Index) const
    {
        return m_objects[Index];
    }
    TObject * GetItem(size_t Index) const
    {
        return m_objects[Index];
    }
    void SetItem(size_t Index, TObject *Value)
    {
    }

    size_t Add(TObject *value)
    {
        m_objects.push_back(value);
        return m_objects.size() - 1;
    }
    int Remove(TObject *value)
    {
        return 0;
    }
    void Extract(TObject *value)
    {
    }
    void Move(int Index, int To)
    {
    }
    void Delete(int Index)
    {
    }
    size_t IndexOf(TObject *value) const
    {
        return -1;
    }
    void Clear()
    {
    }
    bool GetOwnsObjects() { return FOwnsObjects; }
    void SetOwnsObjects(bool value) { FOwnsObjects = value; }

    void Sort(CompareFunc func)
    {}
    void Notify(void *Ptr, int Action)
    {}
private:
    vector<TObject *> m_objects;
    bool FOwnsObjects;
};

class TList : public TObjectList
{
public:
};

enum TDuplicatesEnum
{
    dupAccept,
    dupError
};

class TStream;

class TStrings : public TPersistent
{
public:
    size_t Add(std::wstring value)
    {
        return -1;
    }
    virtual size_t GetCount()
    {
        return 0;
    }
    std::wstring GetString(int Index)
    {
        return L"";
    }
    std::wstring GetText()
    {
        return L"";
    }
    void SetText(std::wstring S)
    {
    }
    void SetCommaText(std::wstring S)
    {
    }
    void SetString(int Index, std::wstring S)
    {
    }
    void *GetObject(int Index)
    {
        return NULL;
    }
    void SetObject(int Index, TObject *obj)
    {
    }
    void AddObject(std::wstring str, TObject *obj)
    {
    }

    TNotifyEvent GetOnChange() { return FOnChange; }
    void SetOnChange(TNotifyEvent Event) { FOnChange = Event; }
    bool Equals(TStrings *value)
    {
        return false;
    }
    virtual void Clear()
    {
    }
    virtual void PutObject(int Index, TObject *AObject)
    {
    }
    void SetDuplicates(TDuplicatesEnum value)
    {
    }
    void Sort()
    {
    }
    size_t IndexOf(const wchar_t *value)
    {
        return -1;
    }
    size_t IndexOfName(const wchar_t *value)
    {
        return -1;
    }
    const std::wstring GetName(int Index)
    {
        return L"";
    }
    const std::wstring GetValue(const std::wstring Name)
    {
        return L"";
    }
    void SetValue(const std::wstring Name, const std::wstring Value)
    {

    }
    void Delete(size_t Index)
    {
    }
    std::wstring GetCommaText() const
    {
        return L"";
    }
    void AddStrings(TStrings *value)
    {
    }
    void Append(const std::wstring &value)
    {
    }
    bool Find(const std::wstring Value, int &Index)
    {
        return false;
    }
    void Insert(int Index, const std::wstring &value)
    {
    }
    void SaveToStream(TStream *Stream)
    {}
private:
    TNotifyEvent FOnChange;
};

class TStringList : public TStrings
{
public:
    virtual void Assign(TPersistent *Source)
    {}
    void Put(int Index, std::wstring value)
    {
    }
    int GetUpdateCount()
    {
        return 0;
    }
    void Changed()
    {
    }
	void SetCaseSensitive(bool value)
	{
	}
    void SetSorted(bool value)
    {
    }
      // SetDuplicates(dupError);
      // SetCaseSensitive(true);
    void LoadFromFile(const std::wstring &FileName)
    {
    }
};

class TDateTime
{
public:
    TDateTime()
    {}
    explicit TDateTime(double)
    {}
    explicit TDateTime(unsigned int Hour, 
        unsigned int Min, unsigned int Sec, unsigned int MSec)
    {}
    operator double() const
    {
        return 0.0;
    }
    TDateTime &operator - (const TDateTime &)
    {
        return *this;
    }
    void operator = (double value)
    {

    }
    bool operator == (const TDateTime &rhs)
    {
        return false;
    }
    bool operator != (const TDateTime &rhs)
    {
        return !(operator == (rhs));
    }
    std::wstring TimeString() const
    {
        return std::wstring();
    }
    void DecodeDate(unsigned short &Y,
        unsigned short &M, unsigned short &D);
    void DecodeTime(unsigned short &H,
        unsigned short &N, unsigned short &S, unsigned short &MS);
};

static TDateTime Now()
{
    TDateTime result(0.0);
    return result;
}

//---------------------------------------------------------------------------

class TSHFileInfo
{
};

class EAbort : public std::exception
{
public:
    EAbort(std::string what) : std::exception(what.c_str())
    {}
};

class EAccessViolation : public std::exception
{
public:
    EAccessViolation(std::string what) : std::exception(what.c_str())
    {}

};

//---------------------------------------------------------------------------

enum SeekEnum
{
    soFromBeginning,
    soFromCurrent
};

//---------------------------------------------------------------------------
class TStream
{
public:
    __int64 GetPosition() const { return FPosition; }
    void SetPosition(__int64 value) { FPosition = value; }
    __int64 GetSize() const { return FSize; }
    void SetSize(__int64 value) { FSize = value; }
    void ReadBuffer(void *Buffer, unsigned long int Count);
    unsigned long Read(void *Buffer, unsigned long int Count);
    void WriteBuffer(void *Buffer, unsigned long int Count);
    unsigned long Write(void *Buffer, unsigned long int Count);
private:
    __int64 FPosition;
    __int64 FSize;
};

//---------------------------------------------------------------------------

class THandleStream : public TStream
{
public:
  THandleStream(HANDLE AHandle)
  {}
protected:
  HANDLE FHandle;
};

//---------------------------------------------------------------------------

class TMemoryStream : public TStream
{
public:
    void Seek(int seek, int from)
    {}
    void Clear()
    {}
};

class EReadError : public std::exception
{
};

class EWriteError : public std::exception
{
};

//---------------------------------------------------------------------------
class TRegistry
{
public:
    void SetAccess(int access)
    {}
    void SetRootKey(HKEY ARootKey)
    {}
    void GetValueNames(TStrings * Names)
    {}
    void GetKeyNames(TStrings * Names)
    {}
    HKEY GetCurrentKey() const { return 0; }
    HKEY GetRootKey() const { return 0; }
    void CloseKey() {}
    bool OpenKey(const std::wstring &key, bool CanCreate) { return false; }
    bool DeleteKey(const std::wstring &key) { return false; }
    bool DeleteValue(const std::wstring &value) { return false; }
    bool KeyExists(const std::wstring SubKey) { return false; }
    bool ValueExists(const std::wstring Value) { return false; }
    int GetDataSize(const std::wstring Name) { return 0; }
    bool Readbool(const std::wstring Name) { return false; }
    TDateTime ReadDateTime(const std::wstring Name) { return TDateTime(); }
    double ReadFloat(const std::wstring Name)
    { return 0; }
    int Readint(const std::wstring Name)
    { return 0; }
    __int64 ReadInt64(const std::wstring Name)
    { return 0; }
    std::wstring ReadString(const std::wstring Name)
    { return L""; }
    std::wstring ReadStringRaw(const std::wstring Name)
    { return L""; }
    int ReadBinaryData(const std::wstring Name,
      void * Buffer, int Size)
    { return 0; }

  void Writebool(const std::wstring Name, bool Value)
  {}
  void WriteDateTime(const std::wstring Name, TDateTime Value)
  {}
  void WriteFloat(const std::wstring Name, double Value)
  {}
  void WriteString(const std::wstring Name, const std::wstring Value)
  {}
  void WriteStringRaw(const std::wstring Name, const std::wstring Value)
  {}
  void Writeint(const std::wstring Name, int Value)
  {}
  void WriteInt64(const std::wstring Name, __int64 Value)
  {}
  void WriteBinaryData(const std::wstring Name,
      const void * Buffer, int Size)
  {}
};

//---------------------------------------------------------------------------

class TMemIniFile
{
};

//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// FIXME
class TShortCut
{
public:
    inline bool operator < (const TShortCut &rhs) const
    {
        return false;
    }
};
