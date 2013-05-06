#pragma once

#include <stdexcept>
#include <stdarg.h>
#include <vector.h>
#include <rdestl/pair.h>
#include "stdafx.h"
#include <CoreDefs.hpp>

#include <WinDef.h>

#pragma warning(push, 1)

#include <rtlconsts.h>
#include <headers.hpp>
#include <CppProperties.h>

#pragma warning(pop)

#define NPOS static_cast<intptr_t>(-1)

namespace Sysutils {
class Exception;
}

namespace Classes {

//---------------------------------------------------------------------------
extern const UnicodeString sLineBreak;
extern const intptr_t MonthsPerYear;
extern const intptr_t DaysPerWeek;
extern const intptr_t MinsPerHour;
extern const intptr_t MinsPerDay;
extern const intptr_t SecsPerMin;
extern const intptr_t SecsPerHour;
extern const intptr_t HoursPerDay;
extern const intptr_t SecsPerDay;
extern const intptr_t MSecsPerDay;
extern const intptr_t MSecsPerSec;
extern const intptr_t DateDelta;
extern const intptr_t UnixDateDelta;
extern const UnicodeString kernel32;
//---------------------------------------------------------------------------
UnicodeString MB2W(const char * src, const UINT cp = CP_ACP);
AnsiString W2MB(const wchar_t * src, const UINT cp = CP_ACP);
//---------------------------------------------------------------------------
intptr_t __cdecl debug_printf(const wchar_t * format, ...);
intptr_t __cdecl debug_printf2(const char * format, ...);

#ifdef NETBOX_DEBUG
#if defined(_MSC_VER)
#define DEBUG_PRINTF(format, ...) do { debug_printf(L"NetBox: [%s:%d] %s: "format L"\n", Sysutils::ExtractFilename(__FILEW__, L'\\').c_str(), __LINE__, MB2W(__FUNCTION__).c_str(), __VA_ARGS__); } while (0)
#define DEBUG_PRINTF2(format, ...) do { debug_printf2("NetBox: [%s:%d] %s: "format "\n", W2MB(Sysutils::ExtractFilename(__FILEW__, '\\').c_str()).c_str(), __LINE__, __FUNCTION__, __VA_ARGS__); } while (0)
#else
#define DEBUG_PRINTF(format, ...) do { debug_printf(L"NetBox: [%s:%d] %s: "format L"\n", Sysutils::ExtractFilename(MB2W(__FILE__).c_str(), L'\\').c_str(), __LINE__, MB2W(__FUNCTION__).c_str(), ##__VA_ARGS__); } while (0)
#define DEBUG_PRINTF2(format, ...) do { debug_printf2("NetBox: [%s:%d] %s: "format "\n", W2MB(Sysutils::ExtractFilename(MB2W(__FILE__).c_str(), '\\').c_str()).c_str(), __LINE__, __FUNCTION__, ##__VA_ARGS__); } while (0)
#endif
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
  CUSTOM_MEM_ALLOCATION_IMPL;
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
  virtual void Assign(TPersistent * Source);
protected:
  virtual void AssignTo(TPersistent * Dest);
  virtual TPersistent * GetOwner();
private:
  void AssignError(TPersistent * Source);
};

//---------------------------------------------------------------------------

enum TListNotification
{
  lnAdded,
  lnExtracted,
  lnDeleted,
};

typedef intptr_t (CompareFunc)(const void * Item1, const void * Item2);

class TList : public TObject
{
public:
  TList();
  virtual ~TList();
  void * operator [](intptr_t Index) const;
  void * GetItem(intptr_t Index);
  void SetItem(intptr_t Index, void * Item);
  intptr_t Add(void * Value);
  void * Extract(void * Item);
  intptr_t Remove(void * Item);
  void Move(intptr_t CurIndex, intptr_t NewIndex);
  void Delete(intptr_t Index);
  void Insert(intptr_t Index, void * Item);
  intptr_t IndexOf(void * Value) const;
  virtual void Clear();
  virtual void Sort(CompareFunc Func);
  virtual void Notify(void * Ptr, TListNotification Action);
  virtual void Sort();

  intptr_t GetCount() const;
  void SetCount(intptr_t Value);

private:
  rde::vector<void *> FList;
};

class TObjectList : public TList
{
public:
  TObjectList();
  virtual ~TObjectList();

  TObject * operator [](intptr_t Index) const;
  TObject * GetItem(intptr_t Index);
  void SetItem(intptr_t Index, TObject * Value);
  intptr_t Add(TObject * Value);
  intptr_t Remove(TObject * Value);
  void Extract(TObject * Value);
  void Move(intptr_t Index, intptr_t To);
  void Delete(intptr_t Index);
  void Insert(intptr_t Index, TObject * Value);
  intptr_t IndexOf(TObject * Value) const;
  virtual void Clear();
  bool GetOwnsObjects() const { return FOwnsObjects; }
  void SetOwnsObjects(bool Value) { FOwnsObjects = Value; }
  virtual void Sort(CompareFunc func);
  virtual void Notify(void * Ptr, TListNotification Action);

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
  intptr_t Add(const UnicodeString & S);
  virtual void Delete(intptr_t Index) = 0;
  virtual UnicodeString GetTextStr();
  virtual void SetTextStr(const UnicodeString & Text);
  virtual void BeginUpdate();
  virtual void EndUpdate();
  virtual void SetUpdateState(bool Updating);
  intptr_t AddObject(const UnicodeString & S, TObject * AObject);
  virtual void InsertObject(intptr_t Index, const UnicodeString & Key, TObject * AObject);
  bool Equals(TStrings * Value);
  virtual void Clear() = 0;
  void Move(intptr_t CurIndex, intptr_t NewIndex);
  intptr_t IndexOf(const UnicodeString & S);
  virtual intptr_t IndexOfName(const UnicodeString & Name);
  UnicodeString ExtractName(const UnicodeString & S) const;
  void AddStrings(TStrings * Strings);
  void Append(const UnicodeString & Value);
  virtual void Insert(intptr_t Index, const UnicodeString & AString) = 0;
  void SaveToStream(TStream * Stream) const;
  wchar_t GetDelimiter() const { return FDelimiter; }
  void SetDelimiter(wchar_t Value)
  {
    FDelimiter = Value;
  }
  wchar_t GetQuoteChar() const { return FQuoteChar; }
  void SetQuoteChar(wchar_t Value)
  {
    FQuoteChar = Value;
  }
  UnicodeString GetDelimitedText();
  void SetDelimitedText(const UnicodeString & Value);
  virtual intptr_t CompareStrings(const UnicodeString & S1, const UnicodeString & S2);
  intptr_t GetUpdateCount() const { return FUpdateCount; }
  virtual void Assign(TPersistent * Source);
  virtual intptr_t GetCount() const = 0;

protected:
  virtual bool GetCaseSensitive() const = 0;
  virtual void SetCaseSensitive(bool Value) = 0;
  virtual bool GetSorted() const = 0;
  virtual void SetSorted(bool Value) = 0;
  void SetDuplicates(TDuplicatesEnum Value);

public:
  virtual TObject * GetObject(intptr_t Index) = 0;
  virtual void SetObject(intptr_t Index, TObject * AObject) = 0;
  UnicodeString GetCommaText();
  void SetCommaText(const UnicodeString & Value);
  virtual UnicodeString GetText();
  virtual void SetText(const UnicodeString & Text);
  virtual UnicodeString & GetString(intptr_t Index) = 0;
  virtual void SetString(intptr_t Index, const UnicodeString & S) = 0;
  const UnicodeString GetName(intptr_t Index);
  void SetName(intptr_t Index, const UnicodeString & Value);
  const UnicodeString GetValue(const UnicodeString & Name);
  void SetValue(const UnicodeString & Name, const UnicodeString & Value);

private:
  bool PropertyGetCaseSensitive() { return GetCaseSensitive(); }
  void PropertySetCaseSensitive(bool Value) { SetCaseSensitive(Value); }
  bool PropertyGetSorted() { return GetSorted(); }
  void PropertySetSorted(bool Value) { SetSorted(Value); }
  void PropertySetDuplicates(TDuplicatesEnum Value) { SetDuplicates(Value); }

public:
  RWProperty<bool, TStrings, &TStrings::PropertyGetCaseSensitive, &TStrings::PropertySetCaseSensitive> CaseSensitive;
  RWProperty<bool, TStrings, &TStrings::PropertyGetSorted, &TStrings::PropertySetSorted> Sorted;
  WOProperty<TDuplicatesEnum, TStrings, &TStrings::PropertySetDuplicates> Duplicates;

protected:
  TDuplicatesEnum FDuplicates;
  wchar_t FDelimiter;
  wchar_t FQuoteChar;
  intptr_t FUpdateCount;
};

typedef rde::pair<UnicodeString, TObject *> TStringItem;

class TStringList;
typedef intptr_t (TStringListSortCompare)(TStringList * List, intptr_t Index1, intptr_t Index2);

class TStringList : public TStrings
{
  typedef rde::vector<TStringItem> TStringItemList;
  friend intptr_t StringListCompareStrings(TStringList * List, intptr_t Index1, intptr_t Index2);

public:
  TStringList();
  virtual ~TStringList();

  intptr_t Add(const UnicodeString & S);
  intptr_t AddObject(const UnicodeString & S, TObject * AObject);
  void LoadFromFile(const UnicodeString & FileName);
  TNotifyEvent & GetOnChange() { return FOnChange; }
  void SetOnChange(TNotifyEvent OnChange) { FOnChange = OnChange; }
  TNotifyEvent & GetOnChanging() { return FOnChanging; }
  void SetOnChanging(TNotifyEvent onChanging) { FOnChanging = onChanging; }
  void InsertItem(intptr_t Index, const UnicodeString & S, TObject * AObject);
  void QuickSort(intptr_t L, intptr_t R, TStringListSortCompare SCompare);

  virtual void Assign(TPersistent * Source);
  virtual void Clear();
  virtual bool Find(const UnicodeString & S, intptr_t & Index);
  virtual intptr_t IndexOf(const UnicodeString & S);
  virtual void Delete(intptr_t Index);
  virtual void InsertObject(intptr_t Index, const UnicodeString & Key, TObject * AObject);
  virtual void Sort();
  virtual void CustomSort(TStringListSortCompare ACompareFunc);

  virtual void SetUpdateState(bool Updating);
  virtual void Changing();
  virtual void Changed();
  virtual void Insert(intptr_t Index, const UnicodeString & S);
  virtual intptr_t CompareStrings(const UnicodeString & S1, const UnicodeString & S2);
  virtual intptr_t GetCount() const;

protected:
  virtual bool GetCaseSensitive() const;
  virtual void SetCaseSensitive(bool Value);
  virtual bool GetSorted() const;
  virtual void SetSorted(bool Value);

public:
  virtual TObject * GetObject(intptr_t Index);
  virtual void SetObject(intptr_t Index, TObject * AObject);
  virtual UnicodeString & GetString(intptr_t Index);
  virtual void SetString(intptr_t Index, const UnicodeString & S);

private:
  TNotifyEvent FOnChange;
  TNotifyEvent FOnChanging;
  TStringItemList FList;
  bool FSorted;
  bool FCaseSensitive;

private:
  void ExchangeItems(intptr_t Index1, intptr_t Index2);

private:
  TStringList(const TStringList &);
  TStringList & operator=(const TStringList &);
};

/// TDateTime: number of days since 12/30/1899
class TDateTime : public TObject
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

class TSHFileInfo : public TObject
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
class TStream : public TObject
{
public:
  TStream();
  virtual ~TStream();
  virtual __int64 Read(void * Buffer, __int64 Count) = 0;
  virtual __int64 Write(const void * Buffer, __int64 Count) = 0;
  virtual __int64 Seek(__int64 Offset, int Origin) = 0;
  virtual __int64 Seek(const __int64 Offset, TSeekOrigin Origin) = 0;
  void ReadBuffer(void * Buffer, __int64 Count);
  void WriteBuffer(const void * Buffer, __int64 Count);
  __int64 CopyFrom(TStream * Source, __int64 Count);

public:
  __int64 GetPosition() { return Seek(0, soFromCurrent); }
  __int64 GetSize()
  {
    __int64 Pos = Seek(0, soFromCurrent);
    __int64 Result = Seek(0, soFromEnd);
    Seek(Pos, soFromBeginning);
    return Result;
  }

protected:
  virtual void SetSize(const __int64 NewSize) = 0;
  void SetPosition(const __int64 Pos)
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
  virtual __int64 Read(void * Buffer, __int64 Count);
  virtual __int64 Write(const void * Buffer, __int64 Count);
  virtual __int64 Seek(__int64 Offset, int Origin);
  virtual __int64 Seek(const __int64 Offset, TSeekOrigin Origin);

  HANDLE GetHandle() { return FHandle; }
protected:
  virtual void SetSize(const __int64 NewSize);
protected:
  HANDLE FHandle;
};

//---------------------------------------------------------------------------
class EReadError : public std::runtime_error
{
public:
  EReadError(const char * Msg) :
    std::runtime_error(Msg)
  {}
};

class EWriteError : public std::runtime_error
{
public:
  EWriteError(const char * Msg) :
    std::runtime_error(Msg)
  {}
};

//---------------------------------------------------------------------------

class TMemoryStream : public TStream
{
public:
  TMemoryStream();
  virtual  ~TMemoryStream();
  virtual __int64 Read(void * Buffer, __int64 Count);
  virtual __int64 Seek(__int64 Offset, int Origin);
  virtual __int64 Seek(const __int64 Offset, TSeekOrigin Origin);
  void SaveToStream(TStream * Stream);
  void SaveToFile(const UnicodeString & FileName);

  void Clear();
  void LoadFromStream(TStream * Stream);
  void LoadFromFile(const UnicodeString & FileName);
  __int64 GetSize() const { return FSize; }
  virtual void SetSize(const __int64 NewSize);
  virtual __int64 Write(const void * Buffer, __int64 Count);

  void * GetMemory() { return FMemory; }

protected:
  void SetPointer(void * Ptr, __int64 Size);
  virtual void * Realloc(__int64 & NewCapacity);
  __int64 GetCapacity() const { return FCapacity; }

private:
  void SetCapacity(__int64 NewCapacity);
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

class TRegistry : public TObject
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
  intptr_t ReadInteger(const UnicodeString & Name) const;
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
  void WriteInteger(const UnicodeString & Name, intptr_t Value);
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
class TShortCut : public TObject
{
public:
  explicit TShortCut();
  explicit TShortCut(intptr_t Value);
  operator intptr_t() const;
  bool operator < (const TShortCut & rhs) const;
private:
  intptr_t FValue;
};

//---------------------------------------------------------------------------
// from wxvcl\sysset.h

template <class T>
class DelphiSet : public TObject
{
private:
  rde::vector<T> FSet;

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
      FSet.assign(&*rhs.FSet.begin(), &*rhs.FSet.end());
    }
    return *this;
  }

  DelphiSet<T>& operator += (const DelphiSet<T>& rhs)
  {
    FSet.insert(rhs.FSet.begin(), rhs.FSet.end());
    return *this;
  }

  DelphiSet<T>& operator -= (const DelphiSet<T>& rhs)
  {
    FSet.erase(rhs.FSet.begin(), rhs.FSet.end());
    return *this;
  }

  DelphiSet<T>& operator *= (const DelphiSet<T>& rhs)
  {
    typename rde::vector<T>::const_iterator itr;
    for (itr = rhs.FSet.begin(); itr != rhs.FSet.end(); ++itr)
    {
      if (FSet.find(*itr) == FSet.end())
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
    FSet.push_back(Value);
    return *this;
  }

  DelphiSet<T>& AddRange(const T RangeStartValue, const int Count)
  {
    T RangeStartForAdd = RangeStartValue;
    for (int I = 0; I < Count; ++I)
      this->Add(RangeStartForAdd++);
    return *this;
  }

  DelphiSet<T>& Add(const T RangeStartValue, const T RangeEndValue)
  {
    if (RangeEndValue < RangeStartValue)
      throw Sysutils::Exception(FORMAT("Start Value %d is greater than End Value %d", RangeStartValue, RangeEndValue));
    int Range = RangeEndValue - RangeStartValue;
    T RangeStartForAdd = RangeStartValue;
    for (int I = 0; I < Range; ++I)
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
      throw Sysutils::Exception(FORMAT("Start Value %d is greater than End Value %d", RangeStartValue, RangeEndValue));
    for (T I = RangeStartValue ; I <= RangeEndValue; ++I)
      this->Remove(I);
    return *this;
  }

  bool Contains (const T Value) const
  {
    if(FSet.find(Value) == FSet.end())
      return false;
    else
      return true;
  }

  bool Has(const T Value) const { return Contains(Value); }
  void Clear() { FSet.clear(); }
  void Empty() const { FSet.Clear(); }

  bool operator == (const DelphiSet<T>& rhs) const
  {
    if (FSet.size() != rhs.FSet.size())
      return false;

    rde::vector<T> setDifference;
    set_symmetric_difference(FSet.begin(), FSet.end(), rhs.FSet.begin(), rhs.FSet.end(), back_inserter(setDifference));
    return (setDifference.size() == 0);

  }
  bool operator != (const DelphiSet<T>& rhs) const
  {
    return !(operator == (rhs));
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

  static DelphiSet<T>& InitRange(T FirstItem, T LastItem, const int Count)
  {
    DelphiSet<T> *NewOne = new DelphiSet<T>();
    NewOne->AddRange(FirstItem, Count);
    return *NewOne;
  }

  bool IsEmpty() const
  {
    return (FSet.size() == 0);
  }
  UnicodeString ToString(void)
  {
    UnicodeString Result(FSet.size());
    typename std::set<T>::const_iterator itr;
    for(itr = FSet.begin(); itr != FSet.end(); itr++)
    {
      Result += (wchar_t)*itr;
    }

    return Result;
  }
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
class TCustomIniFile : public TObject
{
public:
  TCustomIniFile() {}
  virtual ~TCustomIniFile() {}
};

//---------------------------------------------------------------------------
} // namespace Classes

using namespace Classes;
