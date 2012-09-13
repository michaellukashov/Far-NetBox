#pragma once

#include "stdafx.h"
#include <coredefines.hpp>

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
#include <boost/noncopyable.hpp>
#include <boost/scope_exit.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include <boost/bind.hpp>

#include <rtlconsts.h>
#include <headers.hpp>
#include <CppProperties.h>

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
UnicodeString MB2W(const char * src, const UINT cp = CP_ACP);
std::string W2MB(const wchar_t * src, const UINT cp = CP_ACP);
//---------------------------------------------------------------------------
int __cdecl debug_printf(const wchar_t * format, ...);
int __cdecl debug_printf2(const char * format, ...);

#ifdef NETBOX_DEBUG
#define DEBUG_PRINTF(format, ...) debug_printf(L"NetBox: [%s:%d] %s: "format L"\n", Sysutils::ExtractFilename(MB2W(__FILE__).c_str(), L'\\').c_str(), __LINE__, MB2W(__FUNCTION__).c_str(), __VA_ARGS__);
#define DEBUG_PRINTF2(format, ...) debug_printf2("NetBox: [%s:%d] %s: "format "\n", W2MB(Sysutils::ExtractFilename(MB2W(__FILE__).c_str(), '\\').c_str()).c_str(), __LINE__, __FUNCTION__, __VA_ARGS__);
#else
#define DEBUG_PRINTF(format, ...)
#define DEBUG_PRINTF2(format, ...)
#endif

//---------------------------------------------------------------------------
class TObject;
DEFINE_CALLBACK_TYPE0(TThreadMethod, void);

DEFINE_CALLBACK_TYPE1(TNotifyEvent, void, TObject * /* Sender */);
//---------------------------------------------------------------------------
void Abort();
void Error(int ErrorID, int data);
//---------------------------------------------------------------------------
class TObject
{
public:
  TObject() {}
  virtual ~TObject() {}
  virtual void __fastcall Change() {}
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
  bool operator == (const TRect & other) const
  {
    return
      Left == other.Left &&
      Top == other.Top &&
      Right == other.Right &&
      Bottom == other.Bottom;
  }
  bool operator != (const TRect & other) const
  {
    return !(operator == (other));
  }
  bool operator == (const RECT & other) const
  {
    return
      Left == other.left &&
      Top == other.top &&
      Right == other.right &&
      Bottom == other.bottom;
  }
  bool operator != (const RECT & other) const
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
  virtual void __fastcall Assign(TPersistent * Source);
protected:
  virtual void __fastcall AssignTo(TPersistent * Dest);
  virtual TPersistent * __fastcall GetOwner();
private:
  void __fastcall AssignError(TPersistent * Source);
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
  void * operator [](int Index) const;
  void * GetItem(int Index) const;
  void SetItem(int Index, void * Item);
  int Add(void * value);
  void * Extract(void * item);
  int Remove(void * item);
  void Move(int CurIndex, int NewIndex);
  void Delete(int Index);
  virtual void Insert(int Index, void * Item);
  int IndexOf(void * value) const;
  virtual void __fastcall Clear();
  virtual void __fastcall Sort(CompareFunc func);
  virtual void __fastcall Notify(void * Ptr, int Action);
  virtual void __fastcall Sort();

protected:
  int GetCount() const;
  void SetCount(int value);

private:
  int PropertyGetCount() { return GetCount(); }
  void PropertySetCount(int Value) { SetCount(Value); }
  void * PropertyGetItem(int Index)
  {
    return GetItem(Index);
  }
  void PropertySetItem(int Index, void * Value)
  {
    SetItem(Index, Value);
  }

public:
  RWProperty<int, TList, &TList::PropertyGetCount, &TList::PropertySetCount> Count;
  IndexedProperty2<int, TList, &TList::PropertyGetItem, &TList::PropertySetItem> Items;

private:
  std::vector<void *> FList;
};

class TObjectList : public TList
{
  typedef TList parent;
public:
  TObjectList();
  virtual ~TObjectList();
  TObject * operator [](int Index) const;
  TObject * GetItem(int Index) const;
  void SetItem(int Index, TObject * Value);
  int Add(TObject * value);
  int Remove(TObject * value);
  void Extract(TObject * value);
  void Move(int Index, int To);
  void Delete(int Index);
  virtual void __fastcall Insert(int Index, TObject * value);
  int IndexOf(TObject * value) const;
  virtual void __fastcall Clear();
  bool GetOwnsObjects() const;
  void SetOwnsObjects(bool value);
  virtual void __fastcall Sort(CompareFunc func);
  virtual void __fastcall Notify(void * Ptr, int Action);

private:
  TObject * PropertyGetItem(int Index)
  {
    return GetItem(Index);
  }
  void PropertySetItem(int Index, TObject * Value)
  {
    Insert(Index, Value);
  }

public:
  IndexedProperty<int, TObject *, TObjectList, &TObjectList::PropertyGetItem, &TObjectList::PropertySetItem > Items;
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
  TStrings();
  virtual ~TStrings();
  int __fastcall Add(const UnicodeString S);
  virtual void __fastcall Delete(int Index) = 0;
  virtual UnicodeString __fastcall GetTextStr();
  virtual void __fastcall SetTextStr(const UnicodeString Text);
  virtual void __fastcall BeginUpdate();
  virtual void __fastcall EndUpdate();
  virtual void __fastcall SetUpdateState(bool Updating);
  int __fastcall AddObject(const UnicodeString S, TObject * AObject);
  virtual void __fastcall InsertObject(int Index, const UnicodeString Key, TObject * AObject);
  bool __fastcall Equals(TStrings * value) const;
  virtual void __fastcall Clear() = 0;
  void __fastcall Move(int CurIndex, int NewIndex);
  int __fastcall IndexOf(const UnicodeString S);
  virtual int __fastcall IndexOfName(const UnicodeString Name);
  UnicodeString __fastcall ExtractName(const UnicodeString S) const;
  void __fastcall SetValue(const UnicodeString Name, const UnicodeString Value);
  void __fastcall AddStrings(TStrings * Strings);
  void __fastcall Append(const UnicodeString value);
  virtual void __fastcall Insert(int Index, const UnicodeString AString) = 0;
  void __fastcall SaveToStream(TStream * Stream) const;
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
  virtual void __fastcall Assign(TPersistent * Source);

protected:
  virtual UnicodeString __fastcall GetText();
  virtual void __fastcall SetText(const UnicodeString Text);
  UnicodeString __fastcall GetCommaText();
  void __fastcall SetCommaText(const UnicodeString Value);
  virtual bool __fastcall GetCaseSensitive() const = 0;
  virtual void __fastcall SetCaseSensitive(bool value) = 0;
  virtual bool __fastcall GetSorted() const = 0;
  virtual void __fastcall SetSorted(bool value) = 0;
  void __fastcall SetDuplicates(TDuplicatesEnum value);
  virtual int __fastcall GetCount() const = 0;
  virtual UnicodeString __fastcall GetStrings(int Index) const = 0;
  virtual void __fastcall PutString(int Index, const UnicodeString S) = 0;
  virtual TObject * __fastcall GetObjects(int Index);
  virtual void __fastcall PutObject(int Index, TObject * AObject);
  const UnicodeString __fastcall GetName(int Index) const;
  const UnicodeString __fastcall GetValue(const UnicodeString Name);
  
private:
  int PropertyGetCount() { return GetCount(); }
  UnicodeString PropertyGetText() { return GetText(); }
  void PropertySetText(UnicodeString Value) { SetText(Value); }
  UnicodeString PropertyGetCommaText() { return GetCommaText(); }
  void PropertySetCommaText(UnicodeString Value) { SetCommaText(Value); }
  bool PropertyGetCaseSensitive() { return GetCaseSensitive(); }
  void PropertySetCaseSensitive(bool Value) { SetCaseSensitive(Value); }
  bool PropertyGetSorted() { return GetSorted(); }
  void PropertySetSorted(bool Value) { SetSorted(Value); }
  void PropertySetDuplicates(TDuplicatesEnum Value) { SetDuplicates(Value); }
  UnicodeString PropertyGetString(int Index)
  {
    return GetStrings(Index);
  }
  void PropertySetString(int Index, UnicodeString Value)
  {
    PutString(Index, Value);
  }
  TObject * PropertyGetObject(int Index)
  {
    return GetObjects(Index);
  }
  void PropertySetObject(int Index, TObject * Value)
  {
    PutObject(Index, Value);
  }
  UnicodeString PropertyGetName(int Index)
  {
    return GetName(Index);
  }
  void PropertySetName(int Index, UnicodeString Value)
  {
    // SetName(Index, Value);
    Classes::Error(SNotImplemented, 2012);
  }
  UnicodeString PropertyGetValue(UnicodeString Index)
  {
    return GetValue(Index);
  }
  void PropertySetValue(UnicodeString Index, UnicodeString Value)
  {
    // SetValue(Index, Value);
    Classes::Error(SNotImplemented, 2011);
  }

public:
  ROProperty<int, TStrings, &TStrings::PropertyGetCount> Count;
  RWProperty<UnicodeString, TStrings, &TStrings::PropertyGetText, &TStrings::PropertySetText> Text;
  RWProperty<UnicodeString, TStrings, &TStrings::PropertyGetCommaText, &TStrings::PropertySetCommaText> CommaText;
  RWProperty<bool, TStrings, &TStrings::PropertyGetCaseSensitive, &TStrings::PropertySetCaseSensitive> CaseSensitive;
  RWProperty<bool, TStrings, &TStrings::PropertyGetSorted, &TStrings::PropertySetSorted> Sorted;
  WOProperty<TDuplicatesEnum, TStrings, &TStrings::PropertySetDuplicates> Duplicates;
  IndexedProperty<int, UnicodeString, TStrings, &TStrings::PropertyGetString, &TStrings::PropertySetString> Strings;
  IndexedProperty<int, TObject *, TStrings, &TStrings::PropertyGetObject, &TStrings::PropertySetObject> Objects;
  IndexedProperty<int, UnicodeString, TStrings, &TStrings::PropertyGetName, &TStrings::PropertySetName> Names;
  IndexedProperty<UnicodeString, UnicodeString, TStrings, &TStrings::PropertyGetValue, &TStrings::PropertySetValue> Values;

protected:
  TDuplicatesEnum FDuplicates;
  wchar_t FDelimiter;
  wchar_t FQuoteChar;
  int FUpdateCount;
};

struct TStringItem
{
  TStringItem() : FString(), FObject(NULL) {}
  UnicodeString FString;
  TObject * FObject;
};

class TStringList;
typedef std::vector<TStringItem> TStringItemList;
typedef int (TStringListSortCompare)(TStringList * List, int Index1, int Index2);

class TStringList : public TStrings // , private boost::noncopyable
{
  typedef TStrings parent;
  friend int StringListCompareStrings(TStringList * List, int Index1, int Index2);

public:
  /* __fastcall */ TStringList();
  virtual /* __fastcall */ ~TStringList();
  virtual void __fastcall Assign(TPersistent * Source);
  virtual void __fastcall Clear();
  int __fastcall Add(const UnicodeString S);
  int __fastcall AddObject(const UnicodeString S, TObject * AObject);
  virtual bool __fastcall Find(const UnicodeString S, int & Index);
  virtual int __fastcall IndexOf(const UnicodeString S);
  virtual void __fastcall Delete(int Index);
  virtual void __fastcall InsertObject(int Index, const UnicodeString Key, TObject * AObject);
  void __fastcall InsertItem(int Index, const UnicodeString S, TObject * AObject);
  virtual void __fastcall Sort();
  virtual void __fastcall CustomSort(TStringListSortCompare ACompareFunc);
  void __fastcall QuickSort(int L, int R, TStringListSortCompare SCompare);

  void __fastcall LoadFromFile(const UnicodeString FileName);
  TNotifyEvent & __fastcall GetOnChange() { return FOnChange; }
  void __fastcall SetOnChange(TNotifyEvent onChange) { FOnChange = onChange; }
  TNotifyEvent & __fastcall GetOnChanging() { return FOnChanging; }
  void __fastcall SetOnChanging(TNotifyEvent onChanging) { FOnChanging = onChanging; }

  virtual void __fastcall SetUpdateState(bool Updating);
  virtual void __fastcall Changing();
  virtual void __fastcall Changed();
  virtual void __fastcall Insert(int Index, const UnicodeString S);
  virtual int __fastcall CompareStrings(const UnicodeString S1, const UnicodeString S2);

protected:
  virtual bool __fastcall GetCaseSensitive() const;
  virtual void __fastcall SetCaseSensitive(bool value);
  virtual bool __fastcall GetSorted() const;
  virtual void __fastcall SetSorted(bool value);
  virtual int __fastcall GetCount() const;
  virtual UnicodeString __fastcall GetStrings(int Index) const;
  virtual void __fastcall PutString(int Index, const UnicodeString S);
  virtual TObject * __fastcall GetObjects(int Index);
  virtual void __fastcall PutObject(int Index, TObject * AObject);

private:
  TNotifyEvent FOnChange;
  TNotifyEvent FOnChanging;
  TStringItemList FList;
  bool FSorted;
  bool FCaseSensitive;
private:
  void __fastcall ExchangeItems(int Index1, int Index2);
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
  TDateTime(const TDateTime & rhs)
  {
    FValue = rhs.FValue;
  }
  TDateTime & operator = (const TDateTime & rhs)
  {
    FValue = rhs.FValue;
    return *this;
  }
  operator double() const
  {
    return FValue;
  }
  TDateTime & operator + (const TDateTime & rhs)
  {
    FValue += rhs.FValue;
    return *this;
  }
  TDateTime & operator += (const TDateTime & rhs)
  {
    FValue += rhs.FValue;
    return *this;
  }
  TDateTime & operator += (double val)
  {
    FValue += val;
    return *this;
  }
  TDateTime & operator - (const TDateTime & rhs)
  {
    FValue -= rhs.FValue;
    return *this;
  }
  TDateTime & operator -= (const TDateTime & rhs)
  {
    FValue -= rhs.FValue;
    return *this;
  }
  TDateTime & operator -= (double val)
  {
    FValue -= val;
    return *this;
  }
  TDateTime & operator = (double value)
  {
    FValue = value;
    return *this;
  }
  bool operator == (const TDateTime & rhs)
  {
    return fabs(FValue - rhs.FValue) < std::numeric_limits<double>::epsilon();
  }
  bool operator != (const TDateTime & rhs)
  {
    return !(operator == (rhs));
  }
  UnicodeString TimeString() const
  {
    return UnicodeString();
  }
  void DecodeDate(unsigned short & Y,
                  unsigned short & M, unsigned short & D) const;
  void DecodeTime(unsigned short & H,
                  unsigned short & N, unsigned short & S, unsigned short & MS) const;
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
  int GetFileIconIndex( UnicodeString strFileName, BOOL bSmallIcon) const;
  int GetDirIconIndex(BOOL bSmallIcon);

  //get file type
  UnicodeString GetFileType(const UnicodeString strFileName);
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
  virtual __int64 __fastcall Read(void * Buffer, __int64 Count) = 0;
  virtual __int64 __fastcall Write(const void * Buffer, __int64 Count) = 0;
  virtual __int64 __fastcall Seek(__int64 Offset, int Origin) = 0;
  virtual __int64 __fastcall Seek(const __int64 Offset, TSeekOrigin Origin) = 0;
  void __fastcall ReadBuffer(void * Buffer, __int64 Count);
  void __fastcall WriteBuffer(const void * Buffer, __int64 Count);
  __int64 __fastcall CopyFrom(TStream * Source, __int64 Count);

protected:
  __int64 __fastcall GetPosition() { return Seek(0, soFromCurrent); }
  __int64 __fastcall GetSize()
  {
    __int64 Pos = Seek(0, soFromCurrent);
    __int64 Result = Seek(0, soFromEnd);
    Seek(Pos, soFromBeginning);
    return Result;
  }
  virtual void __fastcall SetSize(const __int64 NewSize) = 0;
  void __fastcall SetPosition(const __int64 Pos)
  {
    Seek(Pos, soFromBeginning);
  }

private:
  __int64 PropertyGetPosition() { return GetPosition(); }
  void PropertySetPosition(__int64 Value) { SetPosition(Value); }
  __int64 PropertyGetSize() { return GetSize(); }
  void PropertySetSize(__int64 Value) { SetSize(Value); }

public:
  RWProperty<__int64, TStream, &TStream::PropertyGetPosition, &TStream::PropertySetPosition> Position;
  RWProperty<__int64, TStream, &TStream::PropertyGetSize, &TStream::PropertySetSize> Size;
};

//---------------------------------------------------------------------------

class THandleStream : public TStream
{
public:
  explicit THandleStream(HANDLE AHandle);
  virtual ~THandleStream();
  virtual __int64 __fastcall Read(void * Buffer, __int64 Count);
  virtual __int64 __fastcall Write(const void * Buffer, __int64 Count);
  virtual __int64 __fastcall Seek(__int64 Offset, int Origin);
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
  EReadError(const char * Msg) :
    std::exception(Msg)
  {}
};

class EWriteError : public std::exception
{
public:
  EWriteError(const char * Msg) :
    std::exception(Msg)
  {}
};

//---------------------------------------------------------------------------

class TMemoryStream : public TStream
{
public:
  TMemoryStream();
  virtual  ~TMemoryStream();
  virtual __int64 __fastcall Read(void * Buffer, __int64 Count);
  virtual __int64 __fastcall Seek(__int64 Offset, int Origin);
  virtual __int64 __fastcall Seek(const __int64 Offset, TSeekOrigin Origin);
  void __fastcall SaveToStream(TStream * Stream);
  void __fastcall SaveToFile(const UnicodeString FileName);

  void __fastcall Clear();
  void __fastcall LoadFromStream(TStream * Stream);
  void __fastcall LoadFromFile(const UnicodeString FileName);
  virtual void __fastcall SetSize(const __int64 NewSize);
  virtual __int64 __fastcall Write(const void * Buffer, __int64 Count);

  void * __fastcall GetMemory() { return FMemory; }
protected:
  void __fastcall SetPointer(void * Ptr, __int64 Size);
  virtual void * __fastcall Realloc(__int64 & NewCapacity);
  __int64 __fastcall GetCapacity() const { return FCapacity; }
private:
  void __fastcall SetCapacity(__int64 NewCapacity);
private:
  void * FMemory;
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
  void GetValueNames(TStrings * Names) const;
  void GetKeyNames(TStrings * Names) const;
  void CloseKey();
  bool OpenKey(const UnicodeString key, bool CanCreate);
  bool DeleteKey(const UnicodeString key);
  bool DeleteValue(const UnicodeString value) const;
  bool KeyExists(const UnicodeString SubKey);
  bool ValueExists(const UnicodeString Value) const;
  bool GetDataInfo(const UnicodeString ValueName, TRegDataInfo & Value) const;
  TRegDataType GetDataType(const UnicodeString ValueName) const;
  int GetDataSize(const UnicodeString Name) const;
  bool ReadBool(const UnicodeString Name);
  TDateTime ReadDateTime(const UnicodeString Name);
  double ReadFloat(const UnicodeString Name) const;
  int ReadInteger(const UnicodeString Name) const;
  __int64 ReadInt64(const UnicodeString Name);
  UnicodeString ReadString(const UnicodeString Name);
  UnicodeString ReadStringRaw(const UnicodeString Name);
  int ReadBinaryData(const UnicodeString Name,
                     void * Buffer, int Size) const;

  void WriteBool(const UnicodeString Name, bool Value);
  void WriteDateTime(const UnicodeString Name, TDateTime & Value);
  void WriteFloat(const UnicodeString Name, double Value);
  void WriteString(const UnicodeString Name, const UnicodeString Value);
  void WriteStringRaw(const UnicodeString Name, const UnicodeString Value);
  void WriteInteger(const UnicodeString Name, int Value);
  void WriteInt64(const UnicodeString Name, __int64 Value);
  void WriteBinaryData(const UnicodeString Name,
                       const void * Buffer, int Size);
private:
  void ChangeKey(HKEY Value, const UnicodeString Path);
  HKEY GetBaseKey(bool Relative);
  HKEY GetKey(const UnicodeString Key);
  void SetCurrentKey(HKEY Value) { FCurrentKey = Value; }
  bool GetKeyInfo(TRegKeyInfo & Value) const;
  int GetData(const UnicodeString Name, void * Buffer,
              DWORD BufSize, TRegDataType & RegData) const;
  void PutData(const UnicodeString Name, const void * Buffer,
               int BufSize, TRegDataType RegData);
protected:
  void SetAccess(int Value);
  HKEY GetCurrentKey() const;
  HKEY GetRootKey() const;
  void SetRootKey(HKEY ARootKey);

private:
  void PropertySetAccess(int Value) { SetAccess(Value); }
  HKEY PropertyGetCurrentKey() { return GetCurrentKey(); }
  HKEY PropertyGetRootKey() { return GetRootKey(); }
  void PropertySetRootKey(HKEY Value) { SetRootKey(Value); }

public:
  WOProperty<int, TRegistry, &TRegistry::PropertySetAccess> Access;
  ROProperty<HKEY, TRegistry, &TRegistry::PropertyGetCurrentKey> CurrentKey;
  RWProperty<HKEY, TRegistry, &TRegistry::PropertyGetRootKey, &TRegistry::PropertySetRootKey> RootKey;

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
  bool operator < (const TShortCut & rhs) const;
};

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

  bool IsEmpty() const
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
typedef DWORD TThreadID;

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
