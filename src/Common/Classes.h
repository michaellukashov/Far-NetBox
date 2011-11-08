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
    TPersistent();
    virtual ~TPersistent();
    virtual void Assign(TPersistent *Source);
protected:
    virtual void AssignTo(TPersistent *Dest);
    virtual TPersistent *GetOwner();
private:
    void AssignError(TPersistent *Source);
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
    TList();
    virtual ~TList();
    size_t GetCount() const;
    void SetCount(size_t value);
    void *operator [](size_t Index) const;
    void *GetItem(size_t Index) const;
    void SetItem(size_t Index, void *Item);
    size_t Add(void *value);
    void *Extract(void *item);
    int Remove(void *item);
    void Move(size_t CurIndex, size_t NewIndex);
    void Delete(size_t Index);
    virtual void Insert(size_t Index, void *Item);
    int IndexOf(void *value) const;
    virtual void Clear();
    virtual void Sort(CompareFunc func);
    virtual void Notify(void *Ptr, int Action);
    virtual void Sort();
private:
    std::vector<void *> FList;
};

class TObjectList : public TList
{
    typedef TList parent;
public:
    TObjectList();
    virtual ~TObjectList();
    TObject *operator [](size_t Index) const;
    TObject * GetItem(size_t Index) const;
    void SetItem(size_t Index, TObject *Value);
    size_t Add(TObject *value);
    int Remove(TObject *value);
    void Extract(TObject *value);
    void Move(size_t Index, size_t To);
    void Delete(size_t Index);
    virtual void Insert(size_t Index, TObject *value);
    int IndexOf(TObject *value) const;
    virtual void Clear();
    bool GetOwnsObjects();
    void SetOwnsObjects(bool value);
    virtual void Sort(CompareFunc func);
    virtual void Notify(void *Ptr, int Action);
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
    std::wstring GetCommaText();
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
    wchar_t FDelimiter;
    wchar_t FQuoteChar;
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
    TStringList();
    virtual ~TStringList();
    virtual void Assign(TPersistent *Source);
    virtual size_t GetCount() const;
    virtual void Clear();
    size_t Add(std::wstring S);
    int AddObject(std::wstring S, TObject *AObject);
    virtual bool Find(const std::wstring S, int &Index);
    int IndexOf(const std::wstring S);
    virtual void PutString(int Index, std::wstring S);
    virtual void Delete(size_t Index);
    virtual TObject *GetObject(int Index);
    virtual void InsertObject(int Index, std::wstring Key, TObject *AObject);
    void InsertItem(int Index, const std::wstring S, TObject *AObject);
    virtual std::wstring GetString(size_t Index) const;
    bool GetCaseSensitive() const;
    void SetCaseSensitive(bool value);
    bool GetSorted() const;
    void SetSorted(bool value);
    virtual void Sort();
    virtual void CustomSort(TStringListSortCompare CompareFunc);
    void QuickSort(int L, int R, TStringListSortCompare SCompare);

    void LoadFromFile(const std::wstring &FileName);
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

    virtual void PutObject(int Index, TObject *AObject);
    virtual void SetUpdateState(bool Updating);
    virtual void Changing();
    virtual void Changed();
    virtual void Insert(int Index, const std::wstring S);
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

TDateTime Now();

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

enum TSeekOrigin
{
    soFromBeginning = 0,
    soFromCurrent = 1,
    soFromEnd = 2
};

//---------------------------------------------------------------------------
class TStream
{
public:
    TStream();
    virtual ~TStream();
    virtual __int64 Read(void *Buffer, __int64 Count) = 0;
    virtual __int64 Write(const void *Buffer, __int64 Count) = 0;
    virtual __int64 Seek(__int64 Offset, __int64 Origin) = 0;
    virtual __int64 Seek(const __int64 Offset, TSeekOrigin Origin) = 0;
    void ReadBuffer(void *Buffer, __int64 Count);
    void WriteBuffer(const void *Buffer, __int64 Count);
    __int64 CopyFrom(TStream *Source, __int64 Count);
public:
    __int64 GetPosition() { return Seek(0, soFromCurrent); }
    __int64 GetSize()
    {
      __int64 Pos = Seek(0, soFromCurrent);
      __int64 Result = Seek(0, soFromEnd);
      Seek(Pos, soFromBeginning);
      return Result;
    }
    // void SetSize64(const __int64 NewSize);
public:
    virtual void SetSize(const __int64 NewSize) = 0;
    void SetPosition(const __int64 Pos)
    {
        Seek(Pos, soFromBeginning);
    }
};

//---------------------------------------------------------------------------

class THandleStream : public TStream
{
public:
  THandleStream(HANDLE AHandle);
  virtual ~THandleStream();
  virtual __int64 Read(void *Buffer, __int64 Count);
  virtual __int64 Write(const void *Buffer, __int64 Count);
  virtual __int64 Seek(__int64 Offset, __int64 Origin);
  virtual __int64 Seek(const __int64 Offset, TSeekOrigin Origin);

  // property Handle: Integer read FHandle;
  HANDLE GetHandle() { return FHandle; }
protected:
    virtual void SetSize(const __int64 NewSize);
protected:
  HANDLE FHandle;
};

//---------------------------------------------------------------------------
class EReadError : public std::exception
{
public:
    EReadError(const char *Msg) :
        std::exception(Msg)
    {}
};
 
class EWriteError : public std::exception
{
public:
    EWriteError(const char *Msg) :
        std::exception(Msg)
    {}
};

//---------------------------------------------------------------------------

class TMemoryStream : public TStream
{
public:
    TMemoryStream();
    virtual  ~TMemoryStream();
    virtual __int64 Read(void *Buffer, __int64 Count);
    virtual __int64 Seek(__int64 Offset, __int64 Origin);
    virtual __int64 Seek(const __int64 Offset, TSeekOrigin Origin);
    void SaveToStream(TStream *Stream);
    void SaveToFile(const std::wstring FileName);

    void Clear();
    void LoadFromStream(TStream *Stream);
    void LoadFromFile(const std::wstring FileName);
    virtual void SetSize(const __int64 NewSize);
    virtual __int64 Write(const void *Buffer, __int64 Count);

    // property Memory: Pointer read FMemory;
    void *GetMemory() { return FMemory; }
protected:
    void SetPointer(void *Ptr, __int64 Size);
    virtual void *Realloc(__int64 &NewCapacity);
    // property Capacity: Longint read FCapacity write SetCapacity;
    __int64 GetCapacity() { return FCapacity; }
private:
    void SetCapacity(__int64 NewCapacity);
private:
    void *FMemory;
    __int64 FSize;
    __int64 FPosition;
    __int64 FCapacity;
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
    TMemIniFile(const std::wstring AFileName);
    void GetStrings(TStrings *Strings);
    void ReadSections(TStrings *Strings);
    void ReadSection(std::wstring Section, TStrings *Strings);
    void EraseSection(std::wstring Section);
    bool SectionExists(std::wstring Section);
    bool ValueExists(std::wstring Section, std::wstring Name);
    bool DeleteKey(std::wstring Section, std::wstring Name);
    bool Readbool(const std::wstring Section, const std::wstring Name, const bool Default);
    int Readint(const std::wstring Section, const std::wstring Name, int Default);
    std::wstring ReadString(const std::wstring Section, const std::wstring Name, const std::wstring Default);
    void Writebool(const std::wstring Section, const std::wstring Name, bool Value);
    void WriteDateTime(const std::wstring Section, const std::wstring Name, TDateTime Value);
    void WriteFloat(const std::wstring Section, const std::wstring Name, double Value);
    void WriteString(const std::wstring Section, const std::wstring Name, const std::wstring Value);
    void WriteStringRaw(const std::wstring Section, const std::wstring Name, const std::wstring Value);
    void Writeint(const std::wstring Section, const std::wstring Name, int Value);
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
    explicit TShortCut();
    explicit TShortCut(int value);
    operator int() const;
    bool operator < (const TShortCut &rhs) const;
};
