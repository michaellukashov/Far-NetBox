#pragma once

#include "stdafx.h"
#include <CoreDefs.hpp>

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

#include <rtlconsts.h>
#include <headers.hpp>
#include <CppProperties.h>

#pragma warning(pop)

#define NPOS static_cast<intptr_t>(-1)

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
#define DEBUG_PRINTF(format, ...) do { debug_printf(L"NetBox: [%s:%d] %s: "format L"\n", Sysutils::ExtractFilename(MB2W(__FILE__).c_str(), L'\\').c_str(), __LINE__, MB2W(__FUNCTION__).c_str(), __VA_ARGS__); } while (0)
#define DEBUG_PRINTF2(format, ...) do { debug_printf2("NetBox: [%s:%d] %s: "format "\n", W2MB(Sysutils::ExtractFilename(MB2W(__FILE__).c_str(), '\\').c_str()).c_str(), __LINE__, __FUNCTION__, __VA_ARGS__); } while (0)
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
void Error(int ErrorID, intptr_t data);
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

typedef int (CompareFunc)(const void * Item1, const void * Item2);

class TList : public TObject
{
public:
  TList();
  virtual ~TList();
  void * operator [](intptr_t Index) const;
  void *& GetItem(intptr_t Index);
  void SetItem(intptr_t Index, void * Item);
  intptr_t Add(void * Value);
  void * Extract(void * Item);
  intptr_t Remove(void * Item);
  void Move(intptr_t CurIndex, intptr_t NewIndex);
  void Delete(intptr_t Index);
  void Insert(intptr_t Index, void * Item);
  intptr_t IndexOf(void * Value) const;
  virtual void __fastcall Clear();
  virtual void __fastcall Sort(CompareFunc Func);
  virtual void __fastcall Notify(void * Ptr, TListNotification Action);
  virtual void __fastcall Sort();

protected:
  intptr_t GetCount() const;
  void SetCount(intptr_t Value);

private:
  intptr_t PropertyGetCount() { return GetCount(); }
  void PropertySetCount(intptr_t Value) { SetCount(Value); }
  void *& PropertyGetItem(intptr_t Index)
  {
    return GetItem(Index);
  }
  void PropertySetItem(intptr_t Index, void * Value)
  {
    SetItem(Index, Value);
  }

public:
  RWProperty<intptr_t, TList, &TList::PropertyGetCount, &TList::PropertySetCount> Count;
  IndexedPropertyVoid<intptr_t, TList, &TList::PropertyGetItem, &TList::PropertySetItem> Items;

private:
  std::vector<void *> FList;
};

class TObjectList : public TList
{
  typedef TList parent;
public:
  TObjectList();
  virtual ~TObjectList();
  TObject * operator [](intptr_t Index) const;
  TObject *& GetItem(intptr_t Index);
  void SetItem(intptr_t Index, TObject * Value);
  intptr_t Add(TObject * Value);
  intptr_t Remove(TObject * Value);
  void Extract(TObject * Value);
  void Move(intptr_t Index, intptr_t To);
  void Delete(intptr_t Index);
  void __fastcall Insert(intptr_t Index, TObject * Value);
  intptr_t IndexOf(TObject * Value) const;
  virtual void __fastcall Clear();
  bool GetOwnsObjects() const;
  void SetOwnsObjects(bool Value);
  virtual void __fastcall Sort(CompareFunc func);
  virtual void __fastcall Notify(void * Ptr, TListNotification Action);

private:
  TObject *& PropertyGetItem(intptr_t Index)
  {
    return GetItem(Index);
  }
  void PropertySetItem(intptr_t Index, TObject * Value)
  {
    Insert(Index, Value);
  }

public:
  IndexedProperty2<intptr_t, TObject *, TObjectList, &TObjectList::PropertyGetItem, &TObjectList::PropertySetItem > Items;
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
  intptr_t __fastcall Add(const UnicodeString & S);
  virtual void __fastcall Delete(intptr_t Index) = 0;
  virtual UnicodeString __fastcall GetTextStr();
  virtual void __fastcall SetTextStr(const UnicodeString & Text);
  virtual void __fastcall BeginUpdate();
  virtual void __fastcall EndUpdate();
  virtual void __fastcall SetUpdateState(bool Updating);
  intptr_t __fastcall AddObject(const UnicodeString & S, TObject * AObject);
  virtual void __fastcall InsertObject(intptr_t Index, const UnicodeString & Key, TObject * AObject);
  bool __fastcall Equals(TStrings * Value) const;
  virtual void __fastcall Clear() = 0;
  void __fastcall Move(intptr_t CurIndex, intptr_t NewIndex);
  intptr_t __fastcall IndexOf(const UnicodeString & S);
  virtual intptr_t __fastcall IndexOfName(const UnicodeString & Name);
  UnicodeString __fastcall ExtractName(const UnicodeString & S) const;
  void __fastcall AddStrings(TStrings * Strings);
  void __fastcall Append(const UnicodeString & Value);
  virtual void __fastcall Insert(intptr_t Index, const UnicodeString & AString) = 0;
  void __fastcall SaveToStream(TStream * Stream) const;
  wchar_t __fastcall GetDelimiter() const { return FDelimiter; }
  void __fastcall SetDelimiter(wchar_t Value)
  {
    FDelimiter = Value;
  }
  wchar_t __fastcall GetQuoteChar() const { return FQuoteChar; }
  void __fastcall SetQuoteChar(wchar_t Value)
  {
    FQuoteChar = Value;
  }
  UnicodeString __fastcall GetDelimitedText() const;
  void __fastcall SetDelimitedText(const UnicodeString & Value);
  virtual intptr_t __fastcall CompareStrings(const UnicodeString & S1, const UnicodeString & S2);
  int __fastcall GetUpdateCount() const { return FUpdateCount; }
  virtual void __fastcall Assign(TPersistent * Source);

protected:
  virtual UnicodeString __fastcall GetText();
  virtual void __fastcall SetText(const UnicodeString & Text);
  UnicodeString __fastcall GetCommaText();
  void __fastcall SetCommaText(const UnicodeString & Value);
  virtual bool __fastcall GetCaseSensitive() const = 0;
  virtual void __fastcall SetCaseSensitive(bool Value) = 0;
  virtual bool __fastcall GetSorted() const = 0;
  virtual void __fastcall SetSorted(bool Value) = 0;
  void __fastcall SetDuplicates(TDuplicatesEnum Value);
  virtual intptr_t __fastcall GetCount() const = 0;
  virtual UnicodeString & __fastcall GetString(intptr_t Index) = 0;
  virtual UnicodeString __fastcall GetStrings(intptr_t Index) const = 0;
  virtual void __fastcall PutString(intptr_t Index, const UnicodeString & S) = 0;
  virtual TObject *& __fastcall GetObjects(intptr_t Index) = 0;
  virtual void __fastcall PutObject(intptr_t Index, TObject * AObject) = 0;
  const UnicodeString __fastcall GetName(intptr_t Index) const;
  const UnicodeString __fastcall GetValue(const UnicodeString & Name);
  void __fastcall SetValue(const UnicodeString & Name, const UnicodeString & Value);

private:
  intptr_t PropertyGetCount() { return GetCount(); }
  UnicodeString PropertyGetText() { return GetText(); }
  void PropertySetText(UnicodeString Value) { SetText(Value); }
  UnicodeString PropertyGetCommaText() { return GetCommaText(); }
  void PropertySetCommaText(UnicodeString Value) { SetCommaText(Value); }
  bool PropertyGetCaseSensitive() { return GetCaseSensitive(); }
  void PropertySetCaseSensitive(bool Value) { SetCaseSensitive(Value); }
  bool PropertyGetSorted() { return GetSorted(); }
  void PropertySetSorted(bool Value) { SetSorted(Value); }
  void PropertySetDuplicates(TDuplicatesEnum Value) { SetDuplicates(Value); }
  UnicodeString & PropertyGetString(intptr_t Index)
  {
    return GetString(Index);
  }
  void PropertySetString(intptr_t Index, UnicodeString Value)
  {
    PutString(Index, Value);
  }
  TObject *& PropertyGetObject(intptr_t Index)
  {
    return GetObjects(Index);
  }
  void PropertySetObject(intptr_t Index, TObject * Value)
  {
    PutObject(Index, Value);
  }
  UnicodeString PropertyGetName(intptr_t Index)
  {
    return GetName(Index);
  }
  void PropertySetName(intptr_t Index, UnicodeString Value)
  {
    (void)Index;
    // SetName(Index, Value);
    Classes::Error(SNotImplemented, 2012);
  }
  UnicodeString PropertyGetValue(UnicodeString Index)
  {
    return GetValue(Index);
  }
  void PropertySetValue(UnicodeString Index, UnicodeString Value)
  {
    SetValue(Index, Value);
  }

public:
  ROProperty<intptr_t, TStrings, &TStrings::PropertyGetCount> Count;
  RWProperty<UnicodeString, TStrings, &TStrings::PropertyGetText, &TStrings::PropertySetText> Text;
  RWProperty<UnicodeString, TStrings, &TStrings::PropertyGetCommaText, &TStrings::PropertySetCommaText> CommaText;
  RWProperty<bool, TStrings, &TStrings::PropertyGetCaseSensitive, &TStrings::PropertySetCaseSensitive> CaseSensitive;
  RWProperty<bool, TStrings, &TStrings::PropertyGetSorted, &TStrings::PropertySetSorted> Sorted;
  WOProperty<TDuplicatesEnum, TStrings, &TStrings::PropertySetDuplicates> Duplicates;
  IndexedProperty2<intptr_t, UnicodeString, TStrings, &TStrings::PropertyGetString, &TStrings::PropertySetString> Strings;
  IndexedProperty2<intptr_t, TObject *, TStrings, &TStrings::PropertyGetObject, &TStrings::PropertySetObject> Objects;
  IndexedProperty<intptr_t, UnicodeString, TStrings, &TStrings::PropertyGetName, &TStrings::PropertySetName> Names;
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
typedef intptr_t (TStringListSortCompare)(TStringList * List, intptr_t Index1, intptr_t Index2);

class TStringList : public TStrings
{
  typedef TStrings parent;
  friend intptr_t StringListCompareStrings(TStringList * List, intptr_t Index1, intptr_t Index2);

public:
  /* __fastcall */ TStringList();
  virtual /* __fastcall */ ~TStringList();
  virtual void __fastcall Assign(TPersistent * Source);
  virtual void __fastcall Clear();
  intptr_t __fastcall Add(const UnicodeString & S);
  intptr_t __fastcall AddObject(const UnicodeString & S, TObject * AObject);
  virtual bool __fastcall Find(const UnicodeString & S, intptr_t & Index);
  virtual intptr_t __fastcall IndexOf(const UnicodeString & S);
  virtual void __fastcall Delete(intptr_t Index);
  virtual void __fastcall InsertObject(intptr_t Index, const UnicodeString & Key, TObject * AObject);
  void __fastcall InsertItem(intptr_t Index, const UnicodeString & S, TObject * AObject);
  virtual void __fastcall Sort();
  virtual void __fastcall CustomSort(TStringListSortCompare ACompareFunc);
  void __fastcall QuickSort(intptr_t L, intptr_t R, TStringListSortCompare SCompare);

  void __fastcall LoadFromFile(const UnicodeString & FileName);
  TNotifyEvent & __fastcall GetOnChange() { return FOnChange; }
  void __fastcall SetOnChange(TNotifyEvent onChange) { FOnChange = onChange; }
  TNotifyEvent & __fastcall GetOnChanging() { return FOnChanging; }
  void __fastcall SetOnChanging(TNotifyEvent onChanging) { FOnChanging = onChanging; }

  virtual void __fastcall SetUpdateState(bool Updating);
  virtual void __fastcall Changing();
  virtual void __fastcall Changed();
  virtual void __fastcall Insert(intptr_t Index, const UnicodeString & S);
  virtual intptr_t __fastcall CompareStrings(const UnicodeString & S1, const UnicodeString & S2);

protected:
  virtual bool __fastcall GetCaseSensitive() const;
  virtual void __fastcall SetCaseSensitive(bool Value);
  virtual bool __fastcall GetSorted() const;
  virtual void __fastcall SetSorted(bool Value);
  virtual intptr_t __fastcall GetCount() const;
  virtual UnicodeString & __fastcall GetString(intptr_t Index);
  virtual UnicodeString __fastcall GetStrings(intptr_t Index) const;
  virtual void __fastcall PutString(intptr_t Index, const UnicodeString & S);
  virtual TObject *& __fastcall GetObjects(intptr_t Index);
  virtual void __fastcall PutObject(intptr_t Index, TObject * AObject);

private:
  TNotifyEvent FOnChange;
  TNotifyEvent FOnChanging;
  TStringItemList FList;
  bool FSorted;
  bool FCaseSensitive;
private:
  void __fastcall ExchangeItems(intptr_t Index1, intptr_t Index2);
private:
  TStringList(const TStringList &);
  TStringList & operator=(const TStringList &);
};

/// TDateTime: number of days since 12/30/1899
class TDateTime
{
public:
  TDateTime() :
    FValue(0.0)
  {}
  explicit TDateTime(double Value)
  {
    FValue = Value;
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
  TDateTime & operator = (double Value)
  {
    FValue = Value;
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
  UnicodeString DateString() const;
  UnicodeString TimeString() const;
  UnicodeString FormatString(wchar_t * fmt) const;
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
  int GetFileIconIndex(const UnicodeString & StrFileName, BOOL bSmallIcon) const;
  int GetDirIconIndex(BOOL bSmallIcon);

  //get file type
  UnicodeString GetFileType(const UnicodeString & StrFileName);
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

public:
  __int64 __fastcall GetPosition() { return Seek(0, soFromCurrent); }
  __int64 __fastcall GetSize()
  {
    __int64 Pos = Seek(0, soFromCurrent);
    __int64 Result = Seek(0, soFromEnd);
    Seek(Pos, soFromBeginning);
    return Result;
  }

protected:
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
  void __fastcall SaveToFile(const UnicodeString & FileName);

  void __fastcall Clear();
  void __fastcall LoadFromStream(TStream * Stream);
  void __fastcall LoadFromFile(const UnicodeString & FileName);
  __int64 __fastcall GetSize() const { return FSize; }
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
  bool OpenKey(const UnicodeString & Key, bool CanCreate);
  bool DeleteKey(const UnicodeString & Key);
  bool DeleteValue(const UnicodeString & Value) const;
  bool KeyExists(const UnicodeString & SubKey);
  bool ValueExists(const UnicodeString & Value) const;
  bool GetDataInfo(const UnicodeString & ValueName, TRegDataInfo & Value) const;
  TRegDataType GetDataType(const UnicodeString & ValueName) const;
  int GetDataSize(const UnicodeString & Name) const;
  bool ReadBool(const UnicodeString & Name);
  TDateTime ReadDateTime(const UnicodeString & Name);
  double ReadFloat(const UnicodeString & Name) const;
  int ReadInteger(const UnicodeString & Name) const;
  __int64 ReadInt64(const UnicodeString & Name);
  UnicodeString ReadString(const UnicodeString & Name);
  UnicodeString ReadStringRaw(const UnicodeString & Name);
  size_t ReadBinaryData(const UnicodeString & Name,
    void * Buffer, size_t Size) const;

  void WriteBool(const UnicodeString & Name, bool Value);
  void WriteDateTime(const UnicodeString & Name, TDateTime & Value);
  void WriteFloat(const UnicodeString & Name, double Value);
  void WriteString(const UnicodeString & Name, const UnicodeString & Value);
  void WriteStringRaw(const UnicodeString & Name, const UnicodeString & Value);
  void WriteInteger(const UnicodeString & Name, int Value);
  void WriteInt64(const UnicodeString & Name, __int64 Value);
  void WriteBinaryData(const UnicodeString & Name,
    const void * Buffer, size_t Size);
private:
  void ChangeKey(HKEY Value, const UnicodeString & Path);
  HKEY GetBaseKey(bool Relative);
  HKEY GetKey(const UnicodeString & Key);
  void SetCurrentKey(HKEY Value) { FCurrentKey = Value; }
  bool GetKeyInfo(TRegKeyInfo & Value) const;
  int GetData(const UnicodeString & Name, void * Buffer,
    intptr_t BufSize, TRegDataType & RegData) const;
  void PutData(const UnicodeString & Name, const void * Buffer,
    intptr_t BufSize, TRegDataType RegData);
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
  explicit TShortCut(int Value);
  operator int() const;
  bool operator < (const TShortCut & rhs) const;
private:
  int FValue;
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
  {}

  DelphiSet(T StartValue, T EndValue)
  {
    int Value = EndValue - StartValue;
    if (StartValue > EndValue)
      throw Sysutils::Exception(FORMAT("Start Value %d is greater than End Value %d", StartValue, EndValue));
    this->AddRange(StartValue, Value);
  }

  DelphiSet(T StartValue, T EndValue , const int Count)
  {
    if (StartValue > EndValue)
      throw Sysutils::Exception(FORMAT("Start Value %d is greater than End Value %d", StartValue, EndValue));
    this->AddRange(StartValue,Count);
  }

  DelphiSet(const DelphiSet<T>& src)
  {
    FSet = src.FSet;
  }

  DelphiSet<T>& operator = (const DelphiSet<T>& rhs)
  {
    if (this != &rhs)
    {
      FSet.clear();
      FSet.insert(rhs.FSet.begin(),rhs.FSet.end());
    }
    return *this;
  }

  DelphiSet<T>& operator += (const DelphiSet<T>& rhs)
  {
    FSet.insert(rhs.FSet.begin(),rhs.FSet.end());
    return *this;
  }

  DelphiSet<T>& operator -= (const DelphiSet<T>& rhs)
  {
    FSet.erase(rhs.FSet.begin(),rhs.FSet.end());
    return *this;
  }

  DelphiSet<T>& operator *= (const DelphiSet<T>& rhs)
  {
    typename std::set<T>::const_iterator itr;
    for (itr = rhs.FSet.begin(); itr != rhs.FSet.end(); ++itr)
    {
      if (FSet.find(*itr) ==  FSet.end())
        continue;
      FSet.erase(*itr);
    }
    return *this;
  }

  DelphiSet<T> operator + (const DelphiSet<T>& rhs) const
  {
    DelphiSet<T> S = *this;
    S.FSet.insert(rhs.FSet.begin(),rhs.FSet.end());
    return S;
  }

  DelphiSet<T>& operator << (T Item)
  {
    Init(Item);
    return *this;
  }

  DelphiSet<T>& Add(const T Value)
  {
    FSet.insert(Value);
    return *this;
  }

  DelphiSet<T>& AddRange(const T RangeStartValue, const int Count)
  {
    T RangeStartForAdd = RangeStartValue;
    for (int i = 0 ; i < Count; ++i)
      this->Add(RangeStartForAdd++);
    return *this;
  }

  DelphiSet<T>& Add(const T RangeStartValue, const T RangeEndValue)
  {
    if (RangeEndValue < RangeStartValue)
      throw Sysutils::Exception(FORMAT("Start Value %d is greater than End Value %d", StartValue, EndValue));
    int Range = RangeEndValue - RangeStartValue;
    T RangeStartForAdd = RangeStartValue;
    for (int i = 0 ; i < Range; ++i)
      this->Add(RangeStartForAdd++);
    return *this;
  }

  DelphiSet<T>& Remove(T Value)
  {
    FSet.erase(Value);
    return *this;
  }

  DelphiSet<T>& Remove(T RangeStartValue, T RangeEndValue)
  {
    if (RangeEndValue < RangeStartValue)
      throw Sysutils::Exception(FORMAT("Start Value %d is greater than End Value %d", StartValue, EndValue));
    for (T i = RangeStartValue ; i <= RangeEndValue; ++i)
      this->Remove(i);
    return *this;
  }

  bool Contains (const T Value) const
  {
    if(FSet.find(Value) == FSet.end())
      return false;
    else
      return true;
  }

  bool In(const T Value) const { return Contains(Value); }
  bool Has(const T Value) const { return Contains(Value); }
  void Clear() { FSet.clear(); }
  void Empty() const { FSet.Clear(); }

  bool operator == (const DelphiSet<T>& rhs) const
  {
    if (FSet.size() != rhs.FSet.size())
      return false;

    std::set<T> setDifference;
    set_symmetric_difference(FSet.begin(), FSet.end(), rhs.FSet.begin(), rhs.FSet.end(), back_inserter(setDifference));
    return (setDifference.size() == 0);

  }
  bool operator != (const DelphiSet<T>& rhs) const
  {
    return !operator == (rhs);
  }

  DelphiSet<T>& AddItems(T FirstItem, ...)
  {
    va_list argList;
    this->Add(FirstItem);
    va_start(argList, FirstItem);
    T NewItem;
    while ((NewItem = (T)va_arg(argList, int)) != 0)
    {
      this->Add(NewItem);
    }
    va_end(argList);
    return *this;
  }

  DelphiSet<T>& Init(T FirstItem, ...)
  {
    va_list argList;
    Add(FirstItem);
    va_start(argList, FirstItem);
    T NewItem;
    while ((NewItem = (T)va_arg(argList, int)) != 0)
    {
      Add(NewItem);
    }
    va_end(argList);
    return *this;
  }

  static DelphiSet<T>& InitRange(T FirstItem, T LastItem)
  {
    DelphiSet<T> *NewOne = new DelphiSet<T>();
    NewOne->Add(FirstItem, LastItem);
    return *NewOne;
  }

  static DelphiSet<T>& InitRange(T FirstItem, T LastItem , const int Count)
  {
    DelphiSet<T> *NewOne = new DelphiSet<T>();
    NewOne->AddRange(FirstItem, Count);
    return *NewOne;
  }

  bool IsEmpty() const
  {
    return (FSet.size() == 0);
  }
  /*
      wxString ToString(void)
      {
        wxString Result;
        Result.Alloc(FSet.size());
        typename std::set<T>::const_iterator itr;
        for(itr = FSet.begin(); itr != FSet.end(); itr++)
        {
          Result +=(wxChar) *itr;
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
inline double Trunc(double Value) { double intpart; modf(Value, &intpart); return intpart; }
inline double Frac(double Value) { double intpart; return modf(Value, &intpart); }
inline double Abs(double Value) { return fabs(Value); }
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
