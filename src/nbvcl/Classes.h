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
#include <boost/noncopyable.hpp>

#include <rtlconsts.h>
// #ifdef _MSC_VER
// #include <dstring.h>
// #include <wstring.h>
// #include <ustring.h>
// #endif
#include "UnicodeString.hpp"

#pragma warning(pop)

#define NPOS static_cast<int>(-1)

namespace Classes {

//---------------------------------------------------------------------------
extern const UnicodeString sLineBreak;
extern const int MonthsPerYear;
extern const int DaysPerWeek;
extern const int MinsPerHour;
extern const int MinsPerDay;
extern const int SecsPerMin;
extern const int SecsPerHour;
extern const int HoursPerDay;
extern const int SecsPerDay;
extern const int MSecsPerDay;
extern const int MSecsPerSec;
extern const int DateDelta;
extern const int UnixDateDelta;
extern const UnicodeString kernel32;
//---------------------------------------------------------------------------
UnicodeString MB2W(const char *src, const UINT cp = CP_ACP);
std::string W2MB(const wchar_t *src, const UINT cp = CP_ACP);
//---------------------------------------------------------------------------
int __cdecl debug_printf(const wchar_t *format, ...);
int __cdecl debug_printf2(const char *format, ...);

#ifdef NETBOX_DEBUG
#define DEBUG_PRINTF(format, ...) debug_printf(L"NetBox: [%s:%d] %s: "format L"\n", Sysutils::ExtractFilename(MB2W(__FILE__).c_str(), L'\\').c_str(), __LINE__, MB2W(__FUNCTION__).c_str(), __VA_ARGS__);
#define DEBUG_PRINTF2(format, ...) debug_printf2("NetBox: [%s:%d] %s: "format "\n", W2MB(Sysutils::ExtractFilename(MB2W(__FILE__).c_str(), '\\').c_str()).c_str(), __LINE__, __FUNCTION__, __VA_ARGS__);
#else
#define DEBUG_PRINTF(format, ...)
#define DEBUG_PRINTF2(format, ...)
#endif

//---------------------------------------------------------------------------
class TObject;
typedef boost::signal0<void> threadmethod_signal_type;
typedef threadmethod_signal_type::slot_type TThreadMethod;

typedef boost::signal1<void, TObject * /* Sender */> notify_signal_type;
typedef notify_signal_type::slot_type TNotifyEvent;
//---------------------------------------------------------------------------
void Abort();
void Error(int ErrorID, int data);
//---------------------------------------------------------------------------
class TObject
{
public:
    TObject() {}
    virtual ~TObject() {}
    virtual void Change() {}
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
    int GetCount() const;
    void SetCount(int value);
    void *operator [](int Index) const;
    void *GetItem(int Index) const;
    void SetItem(int Index, void *Item);
    int Add(void *value);
    void *Extract(void *item);
    int Remove(void *item);
    void Move(int CurIndex, int NewIndex);
    void Delete(int Index);
    virtual void Insert(int Index, void *Item);
    int IndexOf(void *value) const;
    virtual void __fastcall Clear();
    virtual void __fastcall Sort(CompareFunc func);
    virtual void __fastcall Notify(void *Ptr, int Action);
    virtual void __fastcall Sort();
private:
    std::vector<void *> FList;
};

class TObjectList : public TList
{
    typedef TList parent;
public:
    TObjectList();
    virtual ~TObjectList();
    TObject *operator [](int Index) const;
    TObject *GetItem(int Index) const;
    void SetItem(int Index, TObject *Value);
    int Add(TObject *value);
    int Remove(TObject *value);
    void Extract(TObject *value);
    void Move(int Index, int To);
    void Delete(int Index);
    virtual void __fastcall Insert(int Index, TObject *value);
    int IndexOf(TObject *value) const;
    virtual void __fastcall Clear();
    bool GetOwnsObjects();
    void SetOwnsObjects(bool value);
    virtual void __fastcall Sort(CompareFunc func);
    virtual void __fastcall Notify(void *Ptr, int Action);
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
    int __fastcall Add(const UnicodeString S);
    virtual int __fastcall GetCount() const = 0;
    virtual void __fastcall Delete(int Index) = 0;
    virtual UnicodeString __fastcall GetStrings(int Index) const = 0;
    virtual UnicodeString __fastcall GetText();
    virtual UnicodeString __fastcall GetTextStr();
    virtual void __fastcall SetText(const UnicodeString Text);
    virtual void __fastcall SetTextStr(const UnicodeString Text);
    void __fastcall SetCommaText(const UnicodeString Value);
    virtual void __fastcall BeginUpdate();
    virtual void __fastcall EndUpdate();
    virtual void __fastcall SetUpdateState(bool Updating);
    virtual TObject * __fastcall GetObjects(int Index);
    int __fastcall AddObject(const UnicodeString S, TObject *AObject);
    virtual void __fastcall InsertObject(int Index, const UnicodeString Key, TObject *AObject);
    bool __fastcall Equals(TStrings *value);
    virtual void __fastcall Clear() = 0;
    virtual void __fastcall PutObject(int Index, TObject *AObject);
    virtual void __fastcall PutString(int Index, const UnicodeString S);
    void __fastcall SetDuplicates(TDuplicatesEnum value);
    void __fastcall Move(int CurIndex, int NewIndex);
    int __fastcall IndexOf(const UnicodeString S);
    virtual int __fastcall IndexOfName(const UnicodeString Name);
    const UnicodeString __fastcall GetName(int Index);
    UnicodeString __fastcall ExtractName(const UnicodeString S);
    const UnicodeString __fastcall GetValue(const UnicodeString Name);
    void __fastcall SetValue(const UnicodeString Name, const UnicodeString Value);
    UnicodeString __fastcall GetCommaText();
    void __fastcall AddStrings(TStrings *Strings);
    void __fastcall Append(const UnicodeString value);
    virtual void __fastcall Insert(int Index, const UnicodeString AString) = 0;
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
    UnicodeString __fastcall GetDelimitedText() const;
    void __fastcall SetDelimitedText(const UnicodeString Value);
    virtual int __fastcall CompareStrings(const UnicodeString S1, const UnicodeString S2);
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
    UnicodeString FString;
    TObject *FObject;
};

class TStringList;
typedef std::vector<TStringItem> TStringItemList;
typedef int (TStringListSortCompare)(TStringList *List, int Index1, int Index2);

class TStringList : public TStrings // , private boost::noncopyable
{
    typedef TStrings parent;
    friend int StringListCompareStrings(TStringList *List, int Index1, int Index2);
public:
    /* __fastcall */ TStringList();
    virtual /* __fastcall */ ~TStringList();
    virtual void __fastcall Assign(TPersistent *Source);
    virtual int __fastcall GetCount() const;
    virtual void __fastcall Clear();
    int __fastcall Add(const UnicodeString S);
    int __fastcall AddObject(const UnicodeString S, TObject *AObject);
    virtual bool __fastcall Find(const UnicodeString S, int &Index);
    virtual int __fastcall IndexOf(const UnicodeString S);
    virtual void __fastcall PutString(int Index, const UnicodeString S);
    virtual void __fastcall Delete(int Index);
    virtual TObject * __fastcall GetObjects(int Index);
    virtual void __fastcall InsertObject(int Index, const UnicodeString Key, TObject *AObject);
    void __fastcall InsertItem(int Index, const UnicodeString S, TObject *AObject);
    virtual UnicodeString __fastcall GetStrings(int Index) const;
    bool __fastcall GetCaseSensitive() const;
    void __fastcall SetCaseSensitive(bool value);
    bool __fastcall GetSorted() const;
    void __fastcall SetSorted(bool value);
    virtual void __fastcall Sort();
    virtual void __fastcall CustomSort(TStringListSortCompare CompareFunc);
    void __fastcall QuickSort(int L, int R, TStringListSortCompare SCompare);

    void __fastcall LoadFromFile(const UnicodeString FileName);
    const notify_signal_type & __fastcall GetOnChange() const { return FOnChange; }
    void __fastcall SetOnChange(const TNotifyEvent &onChange)
    {
        FOnChange.connect(onChange);
    }
    const notify_signal_type & __fastcall GetOnChanging() const { return FOnChanging; }
    void __fastcall SetOnChanging(const TNotifyEvent &onChanging)
    {
        FOnChanging.connect(onChanging);
    }

    virtual void __fastcall PutObject(int Index, TObject *AObject);
    virtual void __fastcall SetUpdateState(bool Updating);
    virtual void __fastcall Changing();
    virtual void __fastcall Changed();
    virtual void __fastcall Insert(int Index, const UnicodeString S);
    virtual int __fastcall CompareStrings(const UnicodeString S1, const UnicodeString S2);

private:
    void __fastcall ExchangeItems(int Index1, int Index2);

private:
    notify_signal_type FOnChange;
    notify_signal_type FOnChanging;
    TStringItemList FList;
    bool FSorted;
    bool FCaseSensitive;
private:
  TStringList(const TStringList &);
  void operator=(const TStringList &);
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
    explicit TDateTime(unsigned short Hour,
      unsigned short Min, unsigned short Sec, unsigned short MSec);
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
    UnicodeString TimeString() const
    {
      return UnicodeString();
    }
    void DecodeDate(unsigned short &Y,
      unsigned short &M, unsigned short &D) const;
    void DecodeTime(unsigned short &H,
      unsigned short &N, unsigned short &S, unsigned short &MS) const;
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
    int GetFileIconIndex( UnicodeString strFileName, BOOL bSmallIcon);
    int GetDirIconIndex(BOOL bSmallIcon);

    //get file type
    UnicodeString GetFileType(const UnicodeString strFileName);
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

class EFileNotFoundError : public std::exception
{
public:
    EFileNotFoundError() : std::exception("")
    {
    }
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
    __int64 __fastcall GetPosition() { return Seek(0, soFromCurrent); }
    __int64 __fastcall GetSize()
    {
        __int64 Pos = Seek(0, soFromCurrent);
        __int64 Result = Seek(0, soFromEnd);
        Seek(Pos, soFromBeginning);
        return Result;
    }
public:
    virtual void __fastcall SetSize(const __int64 NewSize) = 0;
    void __fastcall SetPosition(const __int64 Pos)
    {
        Seek(Pos, soFromBeginning);
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
    void __fastcall SaveToFile(const UnicodeString FileName);

    void __fastcall Clear();
    void __fastcall LoadFromStream(TStream *Stream);
    void __fastcall LoadFromFile(const UnicodeString FileName);
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
    bool OpenKey(const UnicodeString key, bool CanCreate);
    bool DeleteKey(const UnicodeString key);
    bool DeleteValue(const UnicodeString value);
    bool KeyExists(const UnicodeString SubKey);
    bool ValueExists(const UnicodeString Value);
    bool GetDataInfo(const UnicodeString ValueName, TRegDataInfo &Value);
    TRegDataType GetDataType(const UnicodeString ValueName);
    int GetDataSize(const UnicodeString Name);
    bool ReadBool(const UnicodeString Name);
    TDateTime ReadDateTime(const UnicodeString Name);
    double ReadFloat(const UnicodeString Name);
    int ReadInteger(const UnicodeString Name);
    __int64 ReadInt64(const UnicodeString Name);
    UnicodeString ReadString(const UnicodeString Name);
    UnicodeString ReadStringRaw(const UnicodeString Name);
    int ReadBinaryData(const UnicodeString Name,
                       void *Buffer, int Size);

    void WriteBool(const UnicodeString Name, bool Value);
    void WriteDateTime(const UnicodeString Name, TDateTime &Value);
    void WriteFloat(const UnicodeString Name, double Value);
    void WriteString(const UnicodeString Name, const UnicodeString Value);
    void WriteStringRaw(const UnicodeString Name, const UnicodeString Value);
    void WriteInteger(const UnicodeString Name, int Value);
    void WriteInt64(const UnicodeString Name, __int64 Value);
    void WriteBinaryData(const UnicodeString Name,
                         const void *Buffer, int Size);
private:
    void ChangeKey(HKEY Value, const UnicodeString Path);
    HKEY GetBaseKey(bool Relative);
    HKEY GetKey(const UnicodeString Key);
    void SetCurrentKey(HKEY Value) { FCurrentKey = Value; }
    bool GetKeyInfo(TRegKeyInfo &Value);
    int GetData(const UnicodeString Name, void *Buffer,
                   DWORD BufSize, TRegDataType &RegData);
    void PutData(const UnicodeString Name, const void *Buffer,
                 int BufSize, TRegDataType RegData);
private:
    HKEY FCurrentKey;
    HKEY FRootKey;
    // bool FLazyWrite;
    UnicodeString FCurrentPath;
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
    // typedef StaticArray<UnicodeString, 12> _TFormatSettings__1;

    // typedef StaticArray<UnicodeString, 12> _TFormatSettings__2;

    // typedef StaticArray<UnicodeString, 7> _TFormatSettings__3;

    // typedef StaticArray<UnicodeString, 7> _TFormatSettings__4;
public:
    unsigned char CurrencyFormat;
    unsigned char NegCurrFormat;
    wchar_t ThousandSeparator;
    wchar_t DecimalSeparator;
    unsigned char CurrencyDecimals;
    wchar_t DateSeparator;
    wchar_t TimeSeparator;
    wchar_t ListSeparator;
    UnicodeString CurrencyString;
    UnicodeString ShortDateFormat;
    UnicodeString LongDateFormat;
    UnicodeString TimeAMString;
    UnicodeString TimePMString;
    UnicodeString ShortTimeFormat;
    UnicodeString LongTimeFormat;
    // _TFormatSettings__1 ShortMonthNames;
    // _TFormatSettings__2 LongMonthNames;
    // _TFormatSettings__3 ShortDayNames;
    // _TFormatSettings__4 LongDayNames;
    unsigned short TwoDigitYearCenturyWindow;
};

void __fastcall GetLocaleFormatSettings(int LCID, TFormatSettings &FormatSettings);
// int __fastcall GetDefaultLCID();

//---------------------------------------------------------------------------
// from wxvcl\sysset.h

template <class T>
class DelphiSet
{
  private:
    std::set<T> FSet;
  public:
    DelphiSet()
    {
    }

    DelphiSet ( T StartValue, T EndValue )
    {
      int Value = EndValue - StartValue;
      if ( StartValue > EndValue )
        throw 1;//ERangeError::CreateFmt ( wxT("Start Value %d is greater than End Value %d"), StartValue, EndValue );
      this->AddRange ( StartValue, Value );
    }

    DelphiSet ( T StartValue, T EndValue , const int Count)
    {
      if ( StartValue > EndValue )
        throw 1;//ERangeError::CreateFmt ( wxT("Start Value %d is greater than End Value %d"), StartValue, EndValue );
      this->AddRange(StartValue,Count);
    }

    DelphiSet ( const DelphiSet<T>& src )
    {
      FSet = src.FSet;
    }

    DelphiSet<T>& operator = ( const DelphiSet<T>& rhs )
    {
      if ( this != &rhs )
      {
        FSet.clear();
        FSet.insert(rhs.FSet.begin(),rhs.FSet.end());
      }
      return *this;
    }

    DelphiSet<T>& operator += ( const DelphiSet<T>& rhs )
    {
      FSet.insert(rhs.FSet.begin(),rhs.FSet.end());
      return *this;
    }
    
    DelphiSet<T>& operator -= ( const DelphiSet<T>& rhs )
    {
      FSet.erase(rhs.FSet.begin(),rhs.FSet.end());
      return *this;
    }

//commenting becos this does not work with GCC
    DelphiSet<T>& operator *= ( const DelphiSet<T>& rhs )
    {
      typename std::set<T>::const_iterator itr;
      for ( itr = rhs.FSet.begin(); itr != rhs.FSet.end(); itr++)
      {
        if ( FSet.find ( *itr ) ==  FSet.end() )
          continue;
        FSet.erase ( *itr );
      }
      return *this;
    }

    DelphiSet<T> operator + ( const DelphiSet<T>& rhs ) const
    {
      DelphiSet<T> S = *this;
      S.FSet.insert(rhs.FSet.begin(),rhs.FSet.end());
      return S;
    }

    DelphiSet<T>& Add ( const T Value )
    {
      FSet.insert ( Value );
      return *this;
    }

    DelphiSet<T>& AddRange ( const T RangeStartValue, const int Count )
    {
      T RangeStartForAdd = RangeStartValue;
      for ( int i = 0 ; i < Count; ++i )
        this->Add ( RangeStartForAdd++ );
      return *this;
    }

    DelphiSet<T>& Add ( const T RangeStartValue, const T RangeEndValue )
    {
      if ( RangeEndValue < RangeStartValue )
        throw 1;//ERangeError::CreateFmt ( wxT("Start Value %d is greater than End Value %d"), RangeStartValue, RangeEndValue );
      int Range = RangeEndValue - RangeStartValue;
      T RangeStartForAdd = RangeStartValue;
      for ( int i = 0 ; i < Range; ++i )
        this->Add ( RangeStartForAdd++ );
      return *this;
    }

    DelphiSet<T>& Remove ( T Value )
    {
      FSet.erase ( Value );
      return *this;
    }

    DelphiSet<T>& Remove ( T RangeStartValue, T RangeEndValue )
    {
      if ( RangeEndValue < RangeStartValue )
        throw 1;//ERangeError::CreateFmt ( wxT("Start Value %d is greater than End Value %d"), RangeStartValue, RangeEndValue );
      for ( T i = RangeStartValue ; i <= RangeEndValue; ++i )
        this->Remove ( i );
      return *this;
    }

    bool Contains ( const T Value ) const
    {
      if ( FSet.find ( Value ) == FSet.end() )
        return false;
      else
        return true;
    }

    bool In ( const T Value ) const
    {
      return Contains ( Value );
    }

    bool Has ( const T Value ) const
    {
      return Contains ( Value );
    }

    void Clear()
    {
      FSet.clear();
    }

    void Empty() const
    {
      FSet.Clear();
    }

    bool operator == ( const DelphiSet<T>& rhs ) const
    {
      if ( FSet.size() != rhs.FSet.size() )
        return false;

      std::set<T> setDifference;
      set_symmetric_difference(FSet.begin(),FSet.end(),rhs.FSet.begin(), rhs.FSet.end(),back_inserter(setDifference));
      return (setDifference.size() == 0 );

    }
    bool operator != ( const DelphiSet<T>& rhs ) const
    {
      return !operator== ( rhs );
    }

    DelphiSet<T>& AddItems ( T FirstItem, ... )
    {
      va_list argList;
      this->Add ( FirstItem );
      va_start ( argList, FirstItem );
      T NewItem;
      while ( ( NewItem = (T)va_arg ( argList, int ) ) != 0 )
      {
        this->Add ( NewItem );
      }
      va_end ( argList );
      return *this;
    }


    static DelphiSet<T>& Init ( T FirstItem, ... )
    {
      DelphiSet<T> *NewOne = new DelphiSet<T>();
      va_list argList;
      NewOne->Add ( FirstItem );
      va_start ( argList, FirstItem );
      T NewItem;
      while ( ( NewItem = (T)va_arg ( argList, int ) ) != 0 )
      {
        NewOne->Add ( NewItem );
      }
      va_end ( argList );
      return *NewOne;
    }

    static DelphiSet<T>& InitRange ( T FirstItem, T LastItem )
    {
      DelphiSet<T> *NewOne = new DelphiSet<T>();
      NewOne->Add ( FirstItem, LastItem );
      return *NewOne;
    }

    static DelphiSet<T>& InitRange ( T FirstItem, T LastItem , const int Count )
    {
      DelphiSet<T> *NewOne = new DelphiSet<T>();
      NewOne->AddRange ( FirstItem, Count);
      return *NewOne;
    }

    bool IsEmpty ( void )
    {
      return ( FSet.size() == 0 );
    }
/*
    wxString ToString ( void )
    {
      wxString Result;
      Result.Alloc ( FSet.size() );
      typename std::set<T>::const_iterator itr;
      for ( itr = FSet.begin(); itr != FSet.end(); itr++)
      {
        Result += ( wxChar ) *itr;
      }

      return Result;
    }
*/
};

//---------------------------------------------------------------------------

enum TReplaceFlag { rfReplaceAll, rfIgnoreCase };
typedef DelphiSet<TReplaceFlag> TReplaceFlags;

//---------------------------------------------------------------------------
typedef HANDLE THandle;
typedef WIN32_FIND_DATA TWin32FindData;
typedef UnicodeString TFileName;

struct TSystemTime
{
    Word wYear;
    Word wMonth;
    Word wDayOfWeek;
    Word wDay;
    Word wHour;
    Word wMinute;
    Word wSecond;
    Word wMilliseconds;
};
struct TFileTime
{
  Integer LowTime;
  Integer HighTime;
};

struct TSearchRec
{
    Integer Time;
    Int64 Size;
    Integer Attr;
    TFileName Name;
    Integer ExcludeAttr;
    THandle FindHandle;
    TWin32FindData FindData;
};

//---------------------------------------------------------------------------

int FindFirst(const UnicodeString FileName, int FindAttrs, TSearchRec & Rec);
int FindNext(TSearchRec & Rec);
int FindClose(TSearchRec & Rec);

//---------------------------------------------------------------------------
TDateTime IncYear(const TDateTime AValue, const Int64 ANumberOfYears = 1);
TDateTime IncMonth(const TDateTime AValue, const Int64 NumberOfMonths = 1);
TDateTime IncWeek(const TDateTime AValue, const Int64 ANumberOfWeeks = 1);
TDateTime IncDay(const TDateTime AValue, const Int64 ANumberOfDays = 1);
TDateTime IncHour(const TDateTime AValue, const Int64 ANumberOfHours = 1);
TDateTime IncMinute(const TDateTime AValue, const Int64 ANumberOfMinutes = 1);
TDateTime IncSecond(const TDateTime AValue, const Int64 ANumberOfSeconds = 1);
TDateTime IncMilliSecond(const TDateTime AValue, const Int64 ANumberOfMilliSeconds = 1);

Boolean IsLeapYear(Word Year);
//---------------------------------------------------------------------------
inline double Trunc(double value) { double intpart; modf(value, &intpart); return intpart; }
inline double Frac(double value) { double intpart; return modf(value, &intpart); }
inline double Abs(double value) { return fabs(value); }
//---------------------------------------------------------------------------
class TCustomIniFile
{
public:
  TCustomIniFile() {}
  virtual ~TCustomIniFile() {}
};

//---------------------------------------------------------------------------
} // namespace Classes

using namespace Classes;
