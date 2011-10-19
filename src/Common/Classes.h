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
#include <boost/signals/signal0.hpp>
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
#define DEBUG_PRINTF(format, ...) debug_printf(L"NetBox: [%s:%d] %s: "format L"\n", ExtractFilename(::MB2W(__FILE__).c_str(), L'\\').c_str(), __LINE__, ::MB2W(__FUNCTION__).c_str(), __VA_ARGS__);
#else
#define DEBUG_PRINTF(format, ...)
#endif

//---------------------------------------------------------------------------
class TObject;
// typedef void (TObject::*TThreadMethod)();
typedef boost::signal0<void> threadmethod_signal_type;
typedef threadmethod_signal_type::slot_type threadmethod_slot_type;

// typedef void (TObject::*TNotifyEvent)(TObject *);
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
        int Result = IndexOf(item);
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
    int IndexOf(void *value) const
    {
        int Result = 0;
        while ((Result < FList.size()) && (FList[Result] != value))
          Result++;
        if (Result == FList.size())
          Result = -1;
        return Result;
    }
    virtual void Clear()
    {
        FList.clear();
    }

    virtual void Sort(CompareFunc func)
    {
        ::Error(SNotImplemented, 1);
    }
    virtual void Notify(void *Ptr, int Action)
    {
    }
    virtual void Sort()
    {
      // if (FList.size() > 1)
        // QuickSort(FList, 0, GetCount() - 1, Compare);
      ::Error(SNotImplemented, 15);
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
    int IndexOf(TObject *value) const
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
        FUpdateCount(0),
        FDuplicates(dupAccept)
    {
    }
    virtual ~TStrings()
    {}
    size_t Add(std::wstring S);
    virtual size_t GetCount() const = 0;
    virtual void Delete(size_t Index) = 0;
    virtual std::wstring GetString(size_t Index) const = 0;
    virtual std::wstring GetText();
    virtual std::wstring GetTextStr();
    virtual void SetText(const std::wstring Text);
    virtual void SetTextStr(const std::wstring Text);
    void SetCommaText(std::wstring Value);
    virtual void BeginUpdate();
    virtual void EndUpdate();
    virtual void SetUpdateState(bool Updating);
    virtual TObject *GetObject(int Index);
    int AddObject(std::wstring S, TObject *AObject);
    virtual void InsertObject(int Index, std::wstring Key, TObject *AObject);
    bool Equals(TStrings *value);
    virtual void Clear() = 0;
    virtual void PutObject(int Index, TObject *AObject);
    virtual void PutString(int Index, std::wstring S);
    void SetDuplicates(TDuplicatesEnum value);
    void Move(int CurIndex, int NewIndex);
    int IndexOf(const std::wstring S);
    virtual int IndexOfName(const std::wstring &Name);
    const std::wstring GetName(int Index);
    std::wstring ExtractName(const std::wstring &S);
    const std::wstring GetValue(const std::wstring Name);
    void SetValue(const std::wstring Name, const std::wstring Value);
    std::wstring GetCommaText() const;
    void AddStrings(TStrings *Strings);
    void Append(const std::wstring &value);
    virtual void Insert(int Index, const std::wstring AString) = 0;
    void SaveToStream(TStream *Stream);
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
    void Assign(TPersistent *Source);
protected:
    TDuplicatesEnum FDuplicates;
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
    typedef TStrings parent;
    friend int StringListCompareStrings(TStringList *List, int Index1, int Index2);
public:
    TStringList() :
        FSorted(false),
        FCaseSensitive(false)
    {
    }
    virtual ~TStringList()
    {}
    virtual void Assign(TPersistent *Source)
    {
        // ::Error(SNotImplemented, 13);
        parent::Assign(Source);
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
    size_t Add(std::wstring S)
    {
        return AddObject(S, NULL);
    }
    int AddObject(std::wstring S, TObject *AObject)
    {
      // DEBUG_PRINTF(L"S = %s, Duplicates = %d", S.c_str(), FDuplicates);
      int Result = 0;
      if (!GetSorted())
      {
        Result = GetCount();
      }
      else
      {
        if (Find(S, Result))
        {
          switch (FDuplicates)
          {
            case dupIgnore:
                return Result;
                break;
            case dupError:
                ::Error(SDuplicateString, 2);
                break;
          }
        }
      }
      InsertItem(Result, S, AObject);
      return Result;
    }
    virtual bool Find(const std::wstring S, int &Index)
    {
      bool Result = false;
      int L = 0;
      int H = GetCount() - 1;
      while (L <= H)
      {
        int I = (L + H) >> 1;
        int C = CompareStrings(FList[I].FString, S);
        if (C < 0)
        {
            L = I + 1;
        }
        else
        {
          H = I - 1;
          if (C == 0)
          {
            Result = true;
            if (FDuplicates != dupAccept)
            {
                L = I;
            }
          }
        }
      }
      Index = L;
      return Result;
    }
    int IndexOf(const std::wstring S)
    {
      // DEBUG_PRINTF(L"begin");
      int Result = -1;
      if (!GetSorted())
      {
        Result = parent::IndexOf(S);
      }
      else
      {
        if (!Find(S, Result))
        {
          Result = -1;
        }
      }
      // DEBUG_PRINTF(L"end");
      return Result;
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
    virtual std::wstring GetString(size_t Index) const
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
        ::Error(SNotImplemented, 14);
    }
    const notify_signal_type &GetOnChange() const { return FOnChange; }
    void SetOnChange(const notify_slot_type &onChange)
    {
        FOnChange.connect(onChange);
    }
    const notify_signal_type &GetOnChanging() const { return FOnChanging; }
    void SetOnChanging(const notify_slot_type &onChanging)
    {
        FOnChanging.connect(onChanging);
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
        FOnChanging(this);
    }
    virtual void Changed()
    {
        if (GetUpdateCount() == 0)
            FOnChange(this);
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
    notify_signal_type FOnChange;
    notify_signal_type FOnChanging;
    TStringItemList FList;
    bool FSorted;
    bool FCaseSensitive;
};

/// TDateTime: number of days since 12/30/1899
class TDateTime
{
public:
    TDateTime() :
        FValue(0.0)
    {}
    explicit TDateTime(double value)
    {
        FValue = value;
    }
    explicit TDateTime(unsigned int Hour,
        unsigned int Min, unsigned int Sec, unsigned int MSec);
    TDateTime(const TDateTime &rhs)
    {
        FValue = rhs.FValue;
    }
    TDateTime &operator = (const TDateTime &rhs)
    {
        FValue = rhs.FValue;
        return *this;
    }
    operator double() const
    {
        return FValue;
    }
    TDateTime &operator + (const TDateTime &rhs)
    {
        FValue += rhs.FValue;
        return *this;
    }
    TDateTime &operator - (const TDateTime &rhs)
    {
        FValue -= rhs.FValue;
        return *this;
    }
    void operator = (double value)
    {
        FValue = value;
    }
    bool operator == (const TDateTime &rhs)
    {
        return abs(FValue - rhs.FValue) < 0.000001;
    }
    bool operator != (const TDateTime &rhs)
    {
        return !(operator == (rhs));
    }
    std::wstring TimeString() const
    {
        return std::wstring();
    }
    void DecodeDate(unsigned int &Y,
        unsigned int &M, unsigned int &D);
    void DecodeTime(unsigned int &H,
        unsigned int &N, unsigned int &S, unsigned int &MS);
private:
    double FValue;
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

struct TRegKeyInfo
{
    DWORD NumSubKeys;
    DWORD MaxSubKeyLen;
    DWORD NumValues;
    DWORD MaxValueLen;
    DWORD MaxDataLen;
    FILETIME FileTime;
};

//---------------------------------------------------------------------------
enum TRegDataType 
{
    rdUnknown, rdString, rdExpandString, rdInteger, rdBinary
};

struct TRegDataInfo
{
    TRegDataType RegData;
    DWORD DataSize;
};

//---------------------------------------------------------------------------

class TRegistry
{
public:
    TRegistry();
    ~TRegistry();
    void SetAccess(int access);
    void SetRootKey(HKEY ARootKey);
    void GetValueNames(TStrings * Names);
    void GetKeyNames(TStrings * Names);
    HKEY GetCurrentKey() const;
    HKEY GetRootKey() const;
    void CloseKey();
    bool OpenKey(const std::wstring &key, bool CanCreate);
    bool DeleteKey(const std::wstring &key);
    bool DeleteValue(const std::wstring &value);
    bool KeyExists(const std::wstring SubKey);
    bool ValueExists(const std::wstring Value);
    bool GetDataInfo(const std::wstring &ValueName, TRegDataInfo &Value);
    TRegDataType GetDataType(const std::wstring &ValueName);
    int GetDataSize(const std::wstring Name);
    bool Readbool(const std::wstring Name);
    TDateTime ReadDateTime(const std::wstring Name);
    double ReadFloat(const std::wstring Name);
    int Readint(const std::wstring Name);
    __int64 ReadInt64(const std::wstring Name);
    std::wstring ReadString(const std::wstring Name);
    std::wstring ReadStringRaw(const std::wstring Name);
    int ReadBinaryData(const std::wstring Name,
      void * Buffer, int Size);

  void Writebool(const std::wstring Name, bool Value);
  void WriteDateTime(const std::wstring Name, TDateTime Value);
  void WriteFloat(const std::wstring Name, double Value);
  void WriteString(const std::wstring Name, const std::wstring Value);
  void WriteStringRaw(const std::wstring Name, const std::wstring Value);
  void Writeint(const std::wstring Name, int Value);
  void WriteInt64(const std::wstring Name, __int64 Value);
  void WriteBinaryData(const std::wstring Name,
      const void * Buffer, int Size);
private:
    void ChangeKey(HKEY Value, const std::wstring &Path);
    HKEY GetBaseKey(bool Relative);
    HKEY GetKey(const std::wstring &Key);
    void SetCurrentKey(HKEY Value) { FCurrentKey = Value; }
    bool GetKeyInfo(TRegKeyInfo &Value);
    int GetData(const std::wstring &Name, void *Buffer,
      DWORD BufSize, TRegDataType &RegData);
    void PutData(const std::wstring &Name, const void *Buffer,
      int BufSize, TRegDataType RegData);
private:
    HKEY FCurrentKey;
	HKEY FRootKey;
	// bool FLazyWrite;
	std::wstring FCurrentPath;
	bool FCloseRootKey;
	unsigned FAccess;
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
    int Time; // Number of milliseconds since midnight
    int Date; // One plus number of days since 1/1/0001
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
