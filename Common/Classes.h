#pragma once

#include "stdafx.h"

#include <WinDef.h>
#include <CommCtrl.h>

#pragma warning(push, 1)
#include <string>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <iterator>
#include <algorithm>
#include <assert.h>

#include "boostdefines.hpp"
#include <boost/signals/signal1.hpp>
#include <boost/bind.hpp>

#include <rtlconsts.h>

#pragma warning(pop)

//TODO: remove
using namespace std;

//---------------------------------------------------------------------------
std::wstring MB2W(const char *src, const UINT cp = CP_ACP);
std::string W2MB(const wchar_t *src, const UINT cp = CP_ACP);
//---------------------------------------------------------------------------
inline int __cdecl debug_printf(const wchar_t *format, ...)
{
    (void)format;
    int len = 0;
#ifdef NETBOX_DEBUG
    va_list args;
    va_start(args, format);
    len = _vscwprintf(format, args);
    std::wstring buf(len + sizeof(wchar_t), 0);
    vswprintf_s(&buf[0], buf.size(), format, args);

    va_end(args);
    OutputDebugStringW(buf.c_str());
#endif
    return len;
}

#ifdef NETBOX_DEBUG
// #define DEBUG_PRINTF(format, ...) debug_printf(L"NetBox: %s:%d %s: "format, ::MB2W(__FILE__).c_str(), __LINE__, ::MB2W(__FUNCTION__).c_str(), __VA_ARGS__);
#define DEBUG_PRINTF(format, ...) debug_printf(L"NetBox: %s: "format, ::MB2W(__FUNCTION__).c_str(), __VA_ARGS__);
#else
#define DEBUG_PRINTF(format, ...)
#endif

//---------------------------------------------------------------------------
class TObject;
typedef void (TObject::*TThreadMethod)();
typedef void (TObject::*TNotifyEvent)(TObject *);

typedef boost::signal1<void, TObject *> notify_signal_type;
typedef notify_signal_type::slot_type notify_slot_type;
//---------------------------------------------------------------------------
void Error(int ErrorID, int data);
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
    virtual void Insert(int Index, TObject *value)
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
    dupError,
    dupIgnore
};

class TStream;

class TStrings : public TPersistent
{
public:
    size_t Add(std::wstring S)
    {
        int Result = GetCount();
        Insert(Result, S);
        return Result;
    }
    virtual size_t GetCount() = 0;
    virtual void Delete(int Index) = 0;
    virtual std::wstring GetString(int Index) = 0;
    virtual std::wstring GetText()
    {
        return GetTextStr();
    }
    virtual std::wstring GetTextStr()
    {
        std::wstring Result;
        return Result;
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
    int AddObject(std::wstring S, TObject *AObject)
    {
          int Result = Add(S);
          PutObject(Result, AObject);
          return Result;
    }
    void InsertObject(int Index, std::wstring Key, TObject *obj)
    {
    }

    bool Equals(TStrings *value)
    {
        return false;
    }
    virtual void Clear()
    {
    }
    virtual TObject *GetObject(int Index)
    {
        return NULL;
    }
    virtual void PutObject(int Index, TObject *AObject)
    {
    }
    virtual void PutString(int Index, std::wstring S)
    {
        TObject *TempObject = GetObject(Index);
        Delete(Index);
        InsertObject(Index, S, TempObject);
    }
    void SetDuplicates(TDuplicatesEnum value)
    {
    }
    void Sort()
    {
    }
    void Move(int Index, int To)
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
    virtual void Insert(int Index, const std::wstring AString) = 0;
    void SaveToStream(TStream *Stream)
    {}
};

struct TStringItem
{
    std::wstring FString;
    TObject *FObject;
};

typedef std::vector<TStringItem> TStringItemList;

class TStringList : public TStrings
{
public:
    TStringList() :
        FOnChange(NULL),
        FOnChanging(NULL),
        FSorted(false)
    {
    }
    virtual void Assign(TPersistent *Source)
    {}
    virtual size_t GetCount()
    {
        return FList.size();
    }
    virtual void PutString(int Index, std::wstring S)
    {
          if (GetSorted())
            ::Error(SSortedListError, 0);
          if ((Index < 0) || (Index >= FList.size()))
            ::Error(SListIndexError, Index);
          Changing();
          FList[Index].FString = S;
          Changed();
    }
    virtual void Delete(int Index)
    {
      if ((Index < 0) || (Index >= FList.size()))
        ::Error(SListIndexError, Index);
      Changing();
      // Finalize(FList^[Index]);
      // Dec(FCount);
      // if Index < FCount then
        // System.Move(FList^[Index + 1], FList^[Index],
          // (FCount - Index) * SizeOf(TStringItem));
      FList.erase(FList.begin() + Index);
      Changed();
    }
    virtual std::wstring GetString(int Index)
    {
        DEBUG_PRINTF(L"NetBox: GetString: Index = %d", Index);
        if ((Index < 0) || (Index >= FList.size()))
            ::Error(SListIndexError, Index);
        std::wstring Result = FList[Index].FString;
        return Result;
    }
    int GetUpdateCount()
    {
        return 0;
    }
    void SetCaseSensitive(bool value)
    {
    }
    bool GetSorted() const
    {
        return FSorted;
    }
    void SetSorted(bool value)
    {
    }
    void LoadFromFile(const std::wstring &FileName)
    {
    }
    TNotifyEvent GetOnChange() { return FOnChange; }
    void SetOnChange(TNotifyEvent Event) { FOnChange = Event; }

    void SetOnChange(const notify_slot_type &onChange)
    {
        m_OnChange.connect(onChange);
    }

    virtual void PutObject(int Index, TObject *AObject)
    {
          if ((Index < 0) || (Index >= FList.size()))
            ::Error(SListIndexError, Index);
          Changing();
          FList[Index].FObject = AObject;
          Changed();
    }
    virtual void Changing()
    {
    }
    virtual void Changed()
    {
        /*
        if (FOnChange)
        {
            ((*this).*FOnChange)(this);
        }
        */
        m_OnChange(this);
    }
    virtual void Insert(int Index, const std::wstring AString)
    {
          // FList[Index].FString = AString;
          // FList[Index].FObject = AObject;
        Changed();
    }
private:
    TNotifyEvent FOnChange;
    TNotifyEvent FOnChanging;
    notify_signal_type m_OnChange;
    // int FCount;
    TStringItemList FList;
    bool FSorted;
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
public:
    TSHFileInfo();
    virtual ~TSHFileInfo();

    //get the system's image list
    HIMAGELIST GetSystemImageListHandle(BOOL bSmallIcon);

    //get the image's index in the system's image list
    int GetFileIconIndex( std::wstring strFileName, BOOL bSmallIcon);
    int GetDirIconIndex(BOOL bSmallIcon);

    //get a handle to the icon
    HICON GetFileIconHandle(std::wstring strFileName, BOOL bSmallIcon);
    HICON GetFolderIconHandle(BOOL bSmallIcon );

    //get file type
    std::wstring GetFileType(std::wstring strFileName);
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
public:
    TMemIniFile(const std::wstring AFileName)
    {}
    void GetStrings(TStrings *Strings)
    {}
    void ReadSections(TStrings *Strings)
    {}
    void ReadSection(std::wstring Section, TStrings *Strings)
    {}
    void EraseSection(std::wstring Section)
    {}
    bool SectionExists(std::wstring Section) { return false; }
    bool ValueExists(std::wstring Section, std::wstring Name) { return false; }
    bool DeleteKey(std::wstring Section, std::wstring Name)
    { return false; }
    bool Readbool(const std::wstring Section, const std::wstring Name, const bool Default)
    { return false; }
    int Readint(const std::wstring Section, const std::wstring Name, int Default)
    { return 0; }
    std::wstring ReadString(const std::wstring Section, const std::wstring Name, const std::wstring Default)
    { return L""; }

    void Writebool(const std::wstring Section, const std::wstring Name, bool Value)
    {}
    void WriteDateTime(const std::wstring Section, const std::wstring Name, TDateTime Value)
    {}
    void WriteFloat(const std::wstring Section, const std::wstring Name, double Value)
    {}
    void WriteString(const std::wstring Section, const std::wstring Name, const std::wstring Value)
    {}
    void WriteStringRaw(const std::wstring Section, const std::wstring Name, const std::wstring Value)
    {}
    void Writeint(const std::wstring Section, const std::wstring Name, int Value)
    {}
};

//---------------------------------------------------------------------------
struct TTimeStamp
{
    int Time;
    int Date;
};

//---------------------------------------------------------------------------
// FIXME
class TShortCut
{
public:
    explicit TShortCut()
    {}
    explicit TShortCut(int value)
    {}
    operator int() const { return 0; }
    inline bool operator < (const TShortCut &rhs) const
    {
        return false;
    }
};
