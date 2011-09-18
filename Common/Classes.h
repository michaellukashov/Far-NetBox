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

//---------------------------------------------------------------------------
extern const std::wstring sLineBreak;
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
        FDestroyed(false)
    {}
    virtual ~TObject()
    {
        Free();
    }

    virtual void Change()
    {}
    void Free()
    {
        if (this && !FDestroyed)
        {
            Destroy();
        }
    }
    virtual void Destroy()
    {
        FDestroyed = true;
    }

private:
    bool FDestroyed;
};

//---------------------------------------------------------------------------

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

//---------------------------------------------------------------------------

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

//---------------------------------------------------------------------------

class TPersistent : public TObject
{
public:
    TPersistent()
    {}
    virtual ~TPersistent()
    {}
    virtual void Assign(TPersistent *Source)
    {}
};

//---------------------------------------------------------------------------

enum TListNotification
{
  lnAdded,
  lnExtracted,
  lnDeleted,
};

typedef int (CompareFunc)(void * Item1, void * Item2);

class TList : public TObject
{
public:
    TList()
    {}
    virtual ~TList()
    {}
    size_t GetCount() const { return FList.size(); }
    void SetCount(size_t value)
    {
        FList.resize(value);
    }

    void *operator [](size_t Index) const
    {
        return FList[Index];
    }
    void *GetItem(size_t Index) const
    {
        return FList[Index];
    }
    void SetItem(size_t Index, void *Item)
    {
        if ((Index < 0) || (Index > FList.size()))
        {
          ::Error(SListIndexError, Index);
        }
        FList.insert(FList.begin() + Index, Item);
    }

    size_t Add(void *value)
    {
        size_t Result = FList.size();
        FList.push_back(value);
        return Result;
    }
    void *Extract(void *item)
    {
        if (Remove(item) >= 0)
            return item;
        else
            return NULL;
    }
    int Remove(void *item)
    {
        size_t Result = IndexOf(item);
        if (Result >= 0)
        {
            Delete(Result);
        }
        return Result;
    }
    void Move(size_t CurIndex, size_t NewIndex)
    {
      if (CurIndex != NewIndex)
      {
        if ((NewIndex < 0) || (NewIndex >= FList.size()))
        {
          ::Error(SListIndexError, NewIndex);
        }
        void *Item = GetItem(CurIndex);
        FList[CurIndex] = NULL;
        Delete(CurIndex);
        Insert(NewIndex, NULL);
        FList[NewIndex] = Item;
      }
    }
    void Delete(size_t Index)
    {
        if ((Index < 0) || (Index >= FList.size()))
        {
          ::Error(SListIndexError, Index);
        }
        FList.erase(FList.begin() + Index);
    }
    virtual void Insert(size_t Index, void *Item)
    {
        if ((Index < 0) || (Index > FList.size()))
        {
          ::Error(SListIndexError, Index);
        }
        // if (FCount == FCapacity)
          // Grow();
        if (Index <= FList.size())
        {
            FList.insert(FList.begin() + Index, Item);
        }
        if (Item != NULL)
          Notify(Item, lnAdded);
    }
    size_t IndexOf(void *value) const
    {
        size_t Result = 0;
        while ((Result < FList.size()) && (FList[Result] != value))
          Result++;
        if (Result == FList.size())
          Result = (size_t)-1;
        return Result;
    }
    virtual void Clear()
    {
        FList.clear();
    }

    virtual void Sort(CompareFunc func)
    {
        ::Error(SNotImplemented, 0);
    }
    virtual void Notify(void *Ptr, int Action)
    {
    }
    virtual void Sort()
    {
      // if (FList.size() > 1)
        // QuickSort(FList, 0, GetCount() - 1, Compare);
    }
private:
    std::vector<void *> FList;
};

class TObjectList : public TList
{
    typedef TList parent;
public:
    TObjectList() :
        FOwnsObjects(false)
    {
    }
    virtual ~TObjectList()
    {
    }

    TObject *operator [](size_t Index) const
    {
        return (TObject *)parent::operator[](Index);
    }
    TObject * GetItem(size_t Index) const
    {
        return (TObject *)parent::GetItem(Index);
    }
    void SetItem(size_t Index, TObject *Value)
    {
        parent::SetItem(Index, Value);
    }

    size_t Add(TObject *value)
    {
        return parent::Add(value);
    }
    int Remove(TObject *value)
    {
        return parent::Remove(value);
    }
    void Extract(TObject *value)
    {
        parent::Extract(value);
    }
    void Move(size_t Index, size_t To)
    {
        parent::Move(Index, To);
    }
    void Delete(size_t Index)
    {
        parent::Delete(Index);
    }
    virtual void Insert(size_t Index, TObject *value)
    {
        parent::Insert(Index, value);
    }
    size_t IndexOf(TObject *value) const
    {
        return parent::IndexOf(value);
    }
    virtual void Clear()
    {
        parent::Clear();
    }
    bool GetOwnsObjects() { return FOwnsObjects; }
    void SetOwnsObjects(bool value) { FOwnsObjects = value; }

    virtual void Sort(CompareFunc func)
    {
        parent::Sort(func);
    }
    virtual void Notify(void *Ptr, int Action)
    {
      if (GetOwnsObjects())
      {
        if (Action == lnDeleted)
        {
          ((TObject *)Ptr)->Free();
        }
      }
        parent::Notify(Ptr, Action);
    }
private:
    bool FOwnsObjects;
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
    TStrings() :
        FDelimiter(L','),
        FQuoteChar(L'"'),
        FUpdateCount(0)
    {
    }
    virtual ~TStrings()
    {}
    size_t Add(std::wstring S)
    {
        int Result = GetCount();
        Insert(Result, S);
        return Result;
    }
    virtual size_t GetCount() const = 0;
    virtual void Delete(size_t Index) = 0;
    virtual std::wstring GetString(int Index) const = 0;
    virtual std::wstring GetText()
    {
        return GetTextStr();
    }
    virtual std::wstring GetTextStr()
    {
        std::wstring Result;
        int I, L, Size, Count;
        wchar_t *P;
        std::wstring S, LB;

        Count = GetCount();
        // DEBUG_PRINTF(L"Count = %d", Count);
        Size = 0;
        LB = sLineBreak;
        for (I = 0; I < Count; I++)
        {
            Size += GetString(I).size() + LB.size();
        }
        Result.resize(Size);
        P = (wchar_t *)Result.c_str();
        for (I = 0; I < Count; I++)
        {
          S = GetString(I);
          // DEBUG_PRINTF(L"  S = %s", S.c_str());
          L = S.size() * sizeof(wchar_t);
          if (L != 0)
          {
            memcpy(P, S.c_str(), L);
            P += S.size();
          };
          L = LB.size() * sizeof(wchar_t);
          if (L != 0)
          {
            memcpy(P, LB.c_str(), L);
            P += LB.size();
          };
        }
        return Result;
    }
    virtual void SetText(const std::wstring Text)
    {
        SetTextStr(Text);
    }
    virtual void SetTextStr(const std::wstring Text);
    void SetCommaText(std::wstring Value)
    {
        SetDelimiter(L',');
        SetQuoteChar(L'"');
        SetDelimitedText(Value);
    }
    virtual void BeginUpdate()
    {
        if (FUpdateCount == 0) 
          SetUpdateState(true);
        FUpdateCount++;
    }
    virtual void EndUpdate()
    {
        FUpdateCount--;
        if (FUpdateCount == 0)
            SetUpdateState(false);
    }
    virtual void SetUpdateState(bool Updating)
    {
    }
    virtual TObject *GetObject(int Index)
    {
        return NULL;
    }
    int AddObject(std::wstring S, TObject *AObject)
    {
        int Result = Add(S);
        PutObject(Result, AObject);
        return Result;
    }
    virtual void InsertObject(int Index, std::wstring Key, TObject *AObject)
    {
        Insert(Index, Key);
        PutObject(Index, AObject);
    }

    bool Equals(TStrings *value)
    {
        ::Error(SNotImplemented, 0);
        return false;
    }
    virtual void Clear() = 0;
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
        ::Error(SNotImplemented, 0);
    }
    void Move(int Index, int To)
    {
        ::Error(SNotImplemented, 0);
    }
    size_t IndexOf(const wchar_t *value)
    {
        ::Error(SNotImplemented, 0);
        return -1;
    }
    size_t IndexOfName(const wchar_t *value)
    {
        ::Error(SNotImplemented, 0);
        return -1;
    }
    const std::wstring GetName(int Index)
    {
        ::Error(SNotImplemented, 0);
        return L"";
    }
    const std::wstring GetValue(const std::wstring Name)
    {
        ::Error(SNotImplemented, 0);
        return L"";
    }
    void SetValue(const std::wstring Name, const std::wstring Value)
    {
        ::Error(SNotImplemented, 0);
    }
    std::wstring GetCommaText() const;
    void AddStrings(TStrings *value)
    {
        ::Error(SNotImplemented, 0);
    }
    void Append(const std::wstring &value)
    {
        Insert(GetCount(), value);
    }
    bool Find(const std::wstring Value, int &Index)
    {
        ::Error(SNotImplemented, 0);
        return false;
    }
    virtual void Insert(int Index, const std::wstring AString) = 0;
    void SaveToStream(TStream *Stream)
    {
        ::Error(SNotImplemented, 0);
    }
    wchar_t GetDelimiter() const { return FDelimiter; }
    void SetDelimiter(wchar_t value)
    {
        FDelimiter = value;
    }
    wchar_t GetQuoteChar() const { return FQuoteChar; }
    void SetQuoteChar(wchar_t value)
    {
        FQuoteChar = value;
    }
    std::wstring GetDelimitedText() const;
    void SetDelimitedText(const std::wstring Value);
    virtual int CompareStrings(const std::wstring &S1, const std::wstring &S2);
    int GetUpdateCount() const { return FUpdateCount; }
private:
    mutable wchar_t FDelimiter;
    mutable wchar_t FQuoteChar;
    int FUpdateCount;
};

struct TStringItem
{
    std::wstring FString;
    TObject *FObject;
};

class TStringList;
typedef std::vector<TStringItem> TStringItemList;
typedef int (TStringListSortCompare)(TStringList *List, int Index1, int Index2);

class TStringList : public TStrings
{
    friend int StringListCompareStrings(TStringList *List, int Index1, int Index2);
public:
    TStringList() :
        FOnChange(NULL),
        FOnChanging(NULL),
        FSorted(false),
        FCaseSensitive(false)
    {
    }
    virtual ~TStringList()
    {}
    virtual void Assign(TPersistent *Source)
    {
        ::Error(SNotImplemented, 0);
    }
    virtual size_t GetCount() const
    {
        return FList.size();
    }
    virtual void Clear()
    {
        FList.clear();
          // SetCount(0);
          // SetCapacity(0);
    }
    virtual void PutString(int Index, std::wstring S)
    {
        if (GetSorted())
        {
          ::Error(SSortedListError, 0);
        }
        if ((Index < 0) || (Index > FList.size()))
        {
          ::Error(SListIndexError, Index);
        }
        Changing();
        // DEBUG_PRINTF(L"Index = %d, size = %d", Index, FList.size());
        if (Index < FList.size())
        {
          TStringItem item;
          item.FString = S;
          item.FObject = NULL;
          FList[Index] = item;
        }
        else
        {
            Insert(Index, S);
        }
        Changed();
    }
    virtual void Delete(size_t Index)
    {
      if ((Index < 0) || (Index >= FList.size()))
      {
        ::Error(SListIndexError, Index);
      }
      Changing();
      FList.erase(FList.begin() + Index);
      Changed();
    }
    virtual TObject *GetObject(int Index)
    {
        if ((Index < 0) || (Index >= FList.size()))
        {
            ::Error(SListIndexError, Index);
        }
        return FList[Index].FObject;
    }
    virtual void InsertObject(int Index, std::wstring Key, TObject *AObject)
    {
        if (GetSorted())
        {
            ::Error(SSortedListError, 0);
        }
        if ((Index < 0) || (Index > GetCount()))
        {
            ::Error(SListIndexError, Index);
        }
        InsertItem(Index, Key, AObject);
    }
    void InsertItem(int Index, const std::wstring S, TObject *AObject)
    {
        if ((Index < 0) || (Index > GetCount()))
        {
            ::Error(SListIndexError, Index);
        }
        Changing();
        // if (FCount == FCapacity) Grow();
        TStringItem item;
        item.FString = S;
        item.FObject = AObject;
        FList.insert(FList.begin() + Index, item);
        Changed();
    }
    virtual std::wstring GetString(int Index) const
    {
        // DEBUG_PRINTF(L"Index = %d, FList.size = %d", Index, FList.size());
        if ((Index < 0) || (Index >= FList.size()))
        {
            ::Error(SListIndexError, Index);
        }
        std::wstring Result = FList[Index].FString;
        return Result;
    }
    bool GetCaseSensitive() const
    {
        return FCaseSensitive;
    }
    void SetCaseSensitive(bool value)
    {
        if (value != FCaseSensitive)
        {
            FCaseSensitive = value;
            if (GetSorted())
            {
                Sort();
            }
        }
    }
    bool GetSorted() const
    {
        return FSorted;
    }
    void SetSorted(bool value)
    {
        if (value != FSorted)
        {
            if (value)
            {
                Sort();
            }
            FSorted = value;
        }
    }
    virtual void Sort();
    virtual void CustomSort(TStringListSortCompare CompareFunc);
    void QuickSort(int L, int R, TStringListSortCompare SCompare);

    void LoadFromFile(const std::wstring &FileName)
    {
        ::Error(SNotImplemented, 0);
    }
    TNotifyEvent GetOnChange() { return FOnChange; }
    void SetOnChange(TNotifyEvent Event) { FOnChange = Event; }

    void SetOnChange(const notify_slot_type &onChange)
    {
        m_OnChange.connect(onChange);
    }
    void SetOnChanging(const notify_slot_type &onChanging)
    {
        m_OnChanging.connect(onChanging);
    }

    virtual void PutObject(int Index, TObject *AObject)
    {
        if ((Index < 0) || (Index >= FList.size()))
        {
          ::Error(SListIndexError, Index);
        }
        Changing();
        TStringItem item;
        item.FString = FList[Index].FString;
        item.FObject = AObject;
        FList[Index] = item;
        Changed();
    }
    virtual void SetUpdateState(bool Updating)
    {
        if (Updating) 
            Changing();
        else
            Changed();
    }
    virtual void Changing()
    {
      if (GetUpdateCount() == 0)
        m_OnChanging(this);
    }
    virtual void Changed()
    {
        if (GetUpdateCount() == 0)
            m_OnChange(this);
    }
    virtual void Insert(int Index, const std::wstring S)
    {
      if ((Index < 0) || (Index > FList.size()))
      {
        ::Error(SListIndexError, Index);
      }
      TStringItem item;
      item.FString = S;
      item.FObject = NULL;
      FList.insert(FList.begin() + Index, item);
      Changed();
    }
    virtual int CompareStrings(const std::wstring &S1, const std::wstring &S2);
private:
    void ExchangeItems(int Index1, int Index2);
private:
    TNotifyEvent FOnChange;
    TNotifyEvent FOnChanging;
    notify_signal_type m_OnChange;
    notify_signal_type m_OnChanging;
    TStringItemList FList;
    bool FSorted;
    bool FCaseSensitive;
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
