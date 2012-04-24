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
// #ifdef _MSC_VER
// #include <dstring.h>
// #include <wstring.h>
// #include <ustring.h>
// #endif

#pragma warning(pop)

#define NPOS static_cast<size_t>(-1)

namespace System {

//---------------------------------------------------------------------------
extern const std::wstring sLineBreak;
extern const int MinsPerHour;
//---------------------------------------------------------------------------
std::wstring MB2W(const char *src, const UINT cp = CP_ACP);
std::string W2MB(const wchar_t *src, const UINT cp = CP_ACP);
//---------------------------------------------------------------------------
size_t __cdecl debug_printf(const wchar_t *format, ...);
size_t __cdecl debug_printf2(const char *format, ...);

#ifdef NETBOX_DEBUG
#define DEBUG_PRINTF(format, ...) System::debug_printf(L"NetBox: [%s:%d] %s: "format L"\n", ExtractFilename(System::MB2W(__FILE__).c_str(), L'\\').c_str(), __LINE__, System::MB2W(__FUNCTION__).c_str(), __VA_ARGS__);
#define DEBUG_PRINTF2(format, ...) System::debug_printf2("NetBox: [%s:%d] %s: "format "\n", System::W2MB(ExtractFilename(System::MB2W(__FILE__).c_str(), '\\').c_str()).c_str(), __LINE__, __FUNCTION__, __VA_ARGS__);
#else
#define DEBUG_PRINTF(format, ...)
#define DEBUG_PRINTF2(format, ...)
#endif

//---------------------------------------------------------------------------
class TObject;
typedef boost::signal0<void> threadmethod_signal_type;
typedef threadmethod_signal_type::slot_type threadmethod_slot_type;

typedef boost::signal1<void, TObject *> notify_signal_type;
typedef notify_signal_type::slot_type notify_slot_type;
//---------------------------------------------------------------------------
void Abort();
void Error(int ErrorID, size_t data);
//---------------------------------------------------------------------------
class TObject
{
public:
    TObject()
    {}
    virtual ~TObject()
    {}
    virtual void Change()
    {}
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
    virtual void __fastcall Assign(TPersistent *Source);
protected:
    virtual void __fastcall AssignTo(TPersistent *Dest);
    virtual TPersistent * __fastcall GetOwner();
private:
    void __fastcall AssignError(TPersistent *Source);
};

//---------------------------------------------------------------------------

enum TListNotification
{
    lnAdded,
    lnExtracted,
    lnDeleted,
};

typedef int (CompareFunc)(void *Item1, void *Item2);

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
    size_t Remove(void *item);
    void Move(size_t CurIndex, size_t NewIndex);
    void Delete(size_t Index);
    virtual void Insert(size_t Index, void *Item);
    size_t IndexOf(void *value) const;
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
    TObject *GetItem(size_t Index) const;
    void SetItem(size_t Index, TObject *Value);
    size_t Add(TObject *value);
    size_t Remove(TObject *value);
    void Extract(TObject *value);
    void Move(size_t Index, size_t To);
    void Delete(size_t Index);
    virtual void Insert(size_t Index, TObject *value);
    size_t IndexOf(TObject *value) const;
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
    size_t __fastcall Add(const std::wstring S);
    virtual size_t __fastcall GetCount() const = 0;
    virtual void __fastcall Delete(size_t Index) = 0;
    virtual std::wstring __fastcall GetString(size_t Index) const = 0;
    virtual std::wstring __fastcall GetText();
    virtual std::wstring __fastcall GetTextStr();
    virtual void __fastcall SetText(const std::wstring Text);
    virtual void __fastcall SetTextStr(const std::wstring Text);
    void __fastcall SetCommaText(const std::wstring Value);
    virtual void __fastcall BeginUpdate();
    virtual void __fastcall EndUpdate();
    virtual void __fastcall SetUpdateState(bool Updating);
    virtual TObject * __fastcall GetObject(size_t Index);
    size_t __fastcall AddObject(const std::wstring S, TObject *AObject);
    virtual void __fastcall InsertObject(size_t Index, const std::wstring Key, TObject *AObject);
    bool __fastcall Equals(TStrings *value);
    virtual void __fastcall Clear() = 0;
    virtual void __fastcall PutObject(size_t Index, TObject *AObject);
    virtual void __fastcall PutString(size_t Index, const std::wstring S);
    void __fastcall SetDuplicates(TDuplicatesEnum value);
    void __fastcall Move(size_t CurIndex, size_t NewIndex);
    size_t __fastcall IndexOf(const std::wstring S);
    virtual size_t __fastcall IndexOfName(const std::wstring Name);
    const std::wstring __fastcall GetName(size_t Index);
    std::wstring __fastcall ExtractName(const std::wstring S);
    const std::wstring __fastcall GetValue(const std::wstring Name);
    void __fastcall SetValue(const std::wstring Name, const std::wstring Value);
    std::wstring __fastcall GetCommaText();
    void __fastcall AddStrings(TStrings *Strings);
    void __fastcall Append(const std::wstring value);
    virtual void __fastcall Insert(size_t Index, const std::wstring AString) = 0;
    void __fastcall SaveToStream(TStream *Stream);
    wchar_t __fastcall GetDelimiter() const { return FDelimiter; }
    void __fastcall SetDelimiter(wchar_t value)
    {
        FDelimiter = value;
    }
    wchar_t __fastcall GetQuoteChar() const { return FQuoteChar; }
    void __fastcall SetQuoteChar(wchar_t value)
    {
        FQuoteChar = value;
    }
    std::wstring __fastcall GetDelimitedText() const;
    void __fastcall SetDelimitedText(const std::wstring Value);
    virtual int __fastcall CompareStrings(const std::wstring S1, const std::wstring S2);
    int __fastcall GetUpdateCount() const { return FUpdateCount; }
    virtual void __fastcall Assign(TPersistent *Source);
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
typedef int (TStringListSortCompare)(TStringList *List, size_t Index1, size_t Index2);

class TStringList : public TStrings
{
    typedef TStrings parent;
    friend int StringListCompareStrings(TStringList *List, size_t Index1, size_t Index2);
public:
    TStringList();
    virtual ~TStringList();
    virtual void __fastcall Assign(TPersistent *Source);
    virtual size_t __fastcall GetCount() const;
    virtual void __fastcall Clear();
    size_t __fastcall Add(const std::wstring S);
    size_t __fastcall AddObject(const std::wstring S, TObject *AObject);
    virtual bool __fastcall Find(const std::wstring S, size_t &Index);
    virtual size_t __fastcall IndexOf(const std::wstring S);
    virtual void __fastcall PutString(size_t Index, const std::wstring S);
    virtual void __fastcall Delete(size_t Index);
    virtual TObject * __fastcall GetObject(size_t Index);
    virtual void __fastcall InsertObject(size_t Index, const std::wstring Key, TObject *AObject);
    void __fastcall InsertItem(size_t Index, const std::wstring S, TObject *AObject);
    virtual std::wstring __fastcall GetString(size_t Index) const;
    bool __fastcall GetCaseSensitive() const;
    void __fastcall SetCaseSensitive(bool value);
    bool __fastcall GetSorted() const;
    void __fastcall SetSorted(bool value);
    virtual void __fastcall Sort();
    virtual void __fastcall CustomSort(TStringListSortCompare CompareFunc);
    void __fastcall QuickSort(size_t L, size_t R, TStringListSortCompare SCompare);

    void __fastcall LoadFromFile(const std::wstring FileName);
    const System::notify_signal_type & __fastcall GetOnChange() const { return FOnChange; }
    void __fastcall SetOnChange(const System::notify_slot_type &onChange)
    {
        FOnChange.connect(onChange);
    }
    const System::notify_signal_type & __fastcall GetOnChanging() const { return FOnChanging; }
    void __fastcall SetOnChanging(const System::notify_slot_type &onChanging)
    {
        FOnChanging.connect(onChanging);
    }

    virtual void __fastcall PutObject(size_t Index, TObject *AObject);
    virtual void __fastcall SetUpdateState(bool Updating);
    virtual void __fastcall Changing();
    virtual void __fastcall Changed();
    virtual void __fastcall Insert(size_t Index, const std::wstring S);
    virtual int __fastcall CompareStrings(const std::wstring S1, const std::wstring S2);

private:
    void __fastcall ExchangeItems(size_t Index1, size_t Index2);

private:
    System::notify_signal_type FOnChange;
    System::notify_signal_type FOnChanging;
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
        return fabs(FValue - rhs.FValue) < std::numeric_limits<double>::epsilon();
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

    //get the image's index in the system's image list
    int GetFileIconIndex( std::wstring strFileName, BOOL bSmallIcon);
    int GetDirIconIndex(BOOL bSmallIcon);

    //get file type
    std::wstring GetFileType(const std::wstring strFileName);
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
    virtual __int64 __fastcall Read(void *Buffer, __int64 Count) = 0;
    virtual __int64 __fastcall Write(const void *Buffer, __int64 Count) = 0;
    virtual __int64 __fastcall Seek(__int64 Offset, __int64 Origin) = 0;
    virtual __int64 __fastcall Seek(const __int64 Offset, TSeekOrigin Origin) = 0;
    void __fastcall ReadBuffer(void *Buffer, __int64 Count);
    void __fastcall WriteBuffer(const void *Buffer, __int64 Count);
    __int64 __fastcall CopyFrom(TStream *Source, __int64 Count);
public:
    __int64 __fastcall GetPosition() { return Seek(0, System::soFromCurrent); }
    __int64 __fastcall GetSize()
    {
        __int64 Pos = Seek(0, System::soFromCurrent);
        __int64 Result = Seek(0, System::soFromEnd);
        Seek(Pos, System::soFromBeginning);
        return Result;
    }
public:
    virtual void __fastcall SetSize(const __int64 NewSize) = 0;
    void __fastcall SetPosition(const __int64 Pos)
    {
        Seek(Pos, System::soFromBeginning);
    }
};

//---------------------------------------------------------------------------

class THandleStream : public TStream
{
public:
    explicit THandleStream(HANDLE AHandle);
    virtual ~THandleStream();
    virtual __int64 __fastcall Read(void *Buffer, __int64 Count);
    virtual __int64 __fastcall Write(const void *Buffer, __int64 Count);
    virtual __int64 __fastcall Seek(__int64 Offset, __int64 Origin);
    virtual __int64 __fastcall Seek(const __int64 Offset, TSeekOrigin Origin);

    HANDLE __fastcall GetHandle() { return FHandle; }
protected:
    virtual void __fastcall SetSize(const __int64 NewSize);
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
    virtual __int64 __fastcall Read(void *Buffer, __int64 Count);
    virtual __int64 __fastcall Seek(__int64 Offset, __int64 Origin);
    virtual __int64 __fastcall Seek(const __int64 Offset, TSeekOrigin Origin);
    void __fastcall SaveToStream(TStream *Stream);
    void __fastcall SaveToFile(const std::wstring FileName);

    void __fastcall Clear();
    void __fastcall LoadFromStream(TStream *Stream);
    void __fastcall LoadFromFile(const std::wstring FileName);
    virtual void __fastcall SetSize(const __int64 NewSize);
    virtual __int64 __fastcall Write(const void *Buffer, __int64 Count);

    void * __fastcall GetMemory() { return FMemory; }
protected:
    void __fastcall SetPointer(void *Ptr, __int64 Size);
    virtual void * __fastcall Realloc(__int64 &NewCapacity);
    __int64 __fastcall GetCapacity() { return FCapacity; }
private:
    void __fastcall SetCapacity(__int64 NewCapacity);
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
    void GetValueNames(TStrings *Names);
    void GetKeyNames(TStrings *Names);
    HKEY GetCurrentKey() const;
    HKEY GetRootKey() const;
    void CloseKey();
    bool OpenKey(const std::wstring key, bool CanCreate);
    bool DeleteKey(const std::wstring key);
    bool DeleteValue(const std::wstring value);
    bool KeyExists(const std::wstring SubKey);
    bool ValueExists(const std::wstring Value);
    bool GetDataInfo(const std::wstring ValueName, TRegDataInfo &Value);
    TRegDataType GetDataType(const std::wstring ValueName);
    int GetDataSize(const std::wstring Name);
    bool Readbool(const std::wstring Name);
    TDateTime ReadDateTime(const std::wstring Name);
    double ReadFloat(const std::wstring Name);
    int Readint(const std::wstring Name);
    __int64 ReadInt64(const std::wstring Name);
    std::wstring ReadString(const std::wstring Name);
    std::wstring ReadStringRaw(const std::wstring Name);
    size_t ReadBinaryData(const std::wstring Name,
                       void *Buffer, size_t Size);

    void Writebool(const std::wstring Name, bool Value);
    void WriteDateTime(const std::wstring Name, TDateTime &Value);
    void WriteFloat(const std::wstring Name, double Value);
    void WriteString(const std::wstring Name, const std::wstring Value);
    void WriteStringRaw(const std::wstring Name, const std::wstring Value);
    void Writeint(const std::wstring Name, int Value);
    void WriteInt64(const std::wstring Name, __int64 Value);
    void WriteBinaryData(const std::wstring Name,
                         const void *Buffer, size_t Size);
private:
    void ChangeKey(HKEY Value, const std::wstring Path);
    HKEY GetBaseKey(bool Relative);
    HKEY GetKey(const std::wstring Key);
    void SetCurrentKey(HKEY Value) { FCurrentKey = Value; }
    bool GetKeyInfo(TRegKeyInfo &Value);
    size_t GetData(const std::wstring Name, void *Buffer,
                   DWORD BufSize, TRegDataType &RegData);
    void PutData(const std::wstring Name, const void *Buffer,
                 size_t BufSize, TRegDataType RegData);
private:
    HKEY FCurrentKey;
    HKEY FRootKey;
    // bool FLazyWrite;
    std::wstring FCurrentPath;
    bool FCloseRootKey;
    unsigned FAccess;
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

//---------------------------------------------------------------------------

// From SysUtils.hpp
struct TFormatSettings
{
private:
    // typedef StaticArray<System::UnicodeString, 12> _TFormatSettings__1;

    // typedef StaticArray<System::UnicodeString, 12> _TFormatSettings__2;

    // typedef StaticArray<System::UnicodeString, 7> _TFormatSettings__3;

    // typedef StaticArray<System::UnicodeString, 7> _TFormatSettings__4;
public:
    unsigned char CurrencyFormat;
    unsigned char NegCurrFormat;
    wchar_t ThousandSeparator;
    wchar_t DecimalSeparator;
    unsigned char CurrencyDecimals;
    wchar_t DateSeparator;
    wchar_t TimeSeparator;
    wchar_t ListSeparator;
    std::wstring CurrencyString;
    std::wstring ShortDateFormat;
    std::wstring LongDateFormat;
    std::wstring TimeAMString;
    std::wstring TimePMString;
    std::wstring ShortTimeFormat;
    std::wstring LongTimeFormat;
    // _TFormatSettings__1 ShortMonthNames;
    // _TFormatSettings__2 LongMonthNames;
    // _TFormatSettings__3 ShortDayNames;
    // _TFormatSettings__4 LongDayNames;
    unsigned short TwoDigitYearCenturyWindow;
};

void __fastcall GetLocaleFormatSettings(int LCID, TFormatSettings &FormatSettings);
int __fastcall GetDefaultLCID();

//---------------------------------------------------------------------------

} // namespace System
