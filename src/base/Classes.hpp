#pragma once

#include <nbsystem.h>

#include <stdexcept>
#include <limits>
#include <stdarg.h>
#include <math.h>
#include <fstream>

#include <FastDelegate.h>
#include <FastDelegateBind.h>

#pragma warning(push, 1)

#include <rtlconsts.h>
#include <UnicodeString.hpp>
#include <rtti.hpp>
#include <Property.hpp>

#pragma warning(pop)

namespace nb {
  using fastdelegate::bind;
  using fastdelegate::FastDelegate0;
  using fastdelegate::FastDelegate1;
  using fastdelegate::FastDelegate2;
  using fastdelegate::FastDelegate3;
  using fastdelegate::FastDelegate4;
  using fastdelegate::FastDelegate5;
  using fastdelegate::FastDelegate6;
  using fastdelegate::FastDelegate7;
  using fastdelegate::FastDelegate8;
} // namespace nb

using THandle = HANDLE;
using TThreadID = DWORD;

using TSize = size_t;
using Word = uint16_t;
using Cardinal = uint32_t;

constexpr const int32_t MonthsPerYear = 12;
constexpr const int32_t DaysPerWeek = 7;
constexpr const int32_t MinsPerHour = 60;
constexpr const int32_t SecsPerMin = 60;
constexpr const int32_t HoursPerDay = 24;
constexpr const int32_t MSecsPerSec = 1000;
constexpr const int32_t SecsPerHour = MinsPerHour * SecsPerMin;
constexpr const int32_t MinsPerDay = HoursPerDay * MinsPerHour;
constexpr const int32_t SecsPerDay = MinsPerDay * SecsPerMin;
constexpr const int32_t MSecsPerDay = SecsPerDay * MSecsPerSec;
constexpr const int32_t OneSecond = MSecsPerSec;
// Days between 1/1/0001 and 12/31/1899
constexpr const int32_t DateDelta = 693594;
constexpr const int32_t UnixDateDelta = 25569;

class TObject;
class Exception;

using TThreadMethod = nb::FastDelegate0<void>;
using TNotifyEvent = nb::FastDelegate1<void, TObject * /*Sender*/>;

enum class TObjectClassId {};
#define NB_DEFINE_CLASS_ID(CLASS_ID) static constexpr const TObjectClassId OBJECT_CLASS_ ## CLASS_ID = static_cast<TObjectClassId>(nb::counter_id())

NB_DEFINE_CLASS_ID(TObject);
class NB_CORE_EXPORT TObject
{
  CUSTOM_MEM_ALLOCATION_IMPL
public:
  static bool classof(const TObject * /*Obj*/) { return true; }
  virtual bool is(TObjectClassId Kind) const { return Kind == FKind; }
public:
  TObject() noexcept : FKind(OBJECT_CLASS_TObject) {}
  explicit TObject(TObjectClassId Kind) noexcept : FKind(Kind) {}
  virtual ~TObject() = default;
  virtual void Changed() {}
private:
  TObjectClassId FKind{};
};

template<class O, class T>
inline O as_object(T p) { return static_cast<O>(p); }
inline TObject * as_object(void * p) { return as_object<TObject *, void *>(p); }
inline const TObject * as_object(const void * p) { return as_object<const TObject *, const void *>(p); }
template<class O, class T>
inline O * cast_to(T p) { return dyn_cast<O>(as_object(p)); }
template<class O> inline O * cast_to(void * p) { return cast_to<O, void *>(p); }
template<class O> inline const O * cast_to(const void * p) { return cast_to<const O, const void *>(p); }
template <class T>
inline TObject * ToObj(const T & a) { return reinterpret_cast<TObject *>(nb::ToSizeT(a)); }

struct TPoint
{
  int32_t x{0};
  int32_t y{0};
  TPoint() = default;
  explicit TPoint(int32_t ax, int32_t ay) noexcept : x(ax), y(ay) {}
};

struct TRect
{
  int32_t Left{0};
  int32_t Top{0};
  int32_t Right{0};
  int32_t Bottom{0};
  int32_t Width() const { return Right - Left; }
  int32_t Height() const { return Bottom - Top; }
  TRect() = default;
  explicit TRect(int32_t left, int32_t top, int32_t right, int32_t bottom) noexcept :
    Left(left),
    Top(top),
    Right(right),
    Bottom(bottom)
  {}
  bool operator==(const TRect &other) const
  {
    return
      Left == other.Left &&
      Top == other.Top &&
      Right == other.Right &&
      Bottom == other.Bottom;
  }
  bool operator!=(const TRect &other) const
  {
    return !(operator==(other));
  }
  bool operator==(const RECT &other) const
  {
    return
      Left == other.left &&
      Top == other.top &&
      Right == other.right &&
      Bottom == other.bottom;
  }
  bool operator!=(const RECT &other) const
  {
    return !(operator==(other));
  }
};

NB_DEFINE_CLASS_ID(TPersistent);
class NB_CORE_EXPORT TPersistent : public TObject
{
public:
  static bool classof(const TObject * Obj) { return Obj->is(OBJECT_CLASS_TPersistent); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TPersistent) || TObject::is(Kind); }
public:
  TPersistent() noexcept : TObject(OBJECT_CLASS_TPersistent) {}
  explicit TPersistent(TObjectClassId Kind);
  virtual ~TPersistent() override = default;
  virtual void Assign(const TPersistent * Source);
  virtual TPersistent * GetOwner();
protected:
  virtual void AssignTo(TPersistent * Dest) const;
private:
  void AssignError(const TPersistent * Source);
};

enum TListNotification
{
  lnAdded,
  lnExtracted,
  lnDeleted,
};

using CompareFunc = int32_t (const void * Item1, const void * Item2);

NB_DEFINE_CLASS_ID(TList);
class NB_CORE_EXPORT TList : public TPersistent
{
public:
  static bool classof(const TObject * Obj) { return Obj->is(OBJECT_CLASS_TList); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TList) || TPersistent::is(Kind); }
public:
  TList();
  explicit TList(TObjectClassId Kind);
  virtual ~TList() override;

  template<class T>
  T * GetAs(int32_t Index) const { return cast_to<T>(GetItem(Index)); }
  void * operator[](int32_t Index) const;
  virtual void * GetItem(int32_t Index) const { return FList[Index]; }
  virtual void * GetItem(int32_t Index) { return FList[Index]; }
  void SetItem(int32_t Index, void * Item);
  int32_t Add(void * Value);
  void * Extract(void * Item);
  int32_t Remove(void * Item);
  virtual void Move(int32_t CurIndex, int32_t NewIndex);
  virtual void Delete(int32_t Index);
  void Insert(int32_t Index, void * Item);
  int32_t IndexOf(const void * Value) const;
  virtual void Clear();
  void Sort(CompareFunc Func);
  virtual void Notify(void * Ptr, TListNotification Action);
  virtual void Sort();

  int32_t GetCount() const;
  void SetCount(int32_t NewCount);

  ROProperty<int32_t> Count{nb::bind(&TList::GetCount, this)};
  ROIndexedProperty<void *> Items{nb::bind(&TList::GetItems, this)};

private:
  nb::vector_t<void *> FList;
  void * GetItems(int32_t Index) const { return FList[Index]; }
};

NB_DEFINE_CLASS_ID(TObjectList);
class NB_CORE_EXPORT TObjectList : public TList
{
public:
  static bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TObjectList); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TObjectList) || TList::is(Kind); }
public:
  TObjectList();
  explicit TObjectList(TObjectClassId Kind);
  virtual ~TObjectList() override;

  RWProperty2<bool> OwnsObjects{&FOwnsObjects};

  template<class T>
  T * GetAs(int32_t Index) const { return dyn_cast<T>(GetObj(Index)); }
  TObject * operator[](int32_t Index) const;
  TObject * GetObj(int32_t Index) const;
  bool GetOwnsObjects() const { return FOwnsObjects; }
  void SetOwnsObjects(bool Value) { FOwnsObjects = Value; }
  virtual void Notify(void * Ptr, TListNotification Action) override;

private:
  bool FOwnsObjects{true};
};

enum TDuplicatesEnum
{
  dupAccept,
  dupError,
  dupIgnore
};

class TStream;

NB_DEFINE_CLASS_ID(TStrings);
class NB_CORE_EXPORT TStrings : public TObjectList
{
public:
  static bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TStrings); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TStrings) || TObjectList::is(Kind); }
public:
  TStrings() noexcept;
  explicit TStrings(TObjectClassId Kind) noexcept;
  virtual ~TStrings() = default;
  int32_t Add(const UnicodeString & S, const TObject * AObject = nullptr);
  virtual UnicodeString GetTextStr() const;
  virtual void SetTextStr(const UnicodeString & Text);
  virtual void BeginUpdate();
  virtual void EndUpdate();
  virtual void SetUpdateState(bool Updating);
  virtual int32_t AddObject(const UnicodeString & S, const TObject * AObject);
  virtual void InsertObject(int32_t Index, const UnicodeString & Key, TObject * AObject);
  virtual int32_t CompareStrings(const UnicodeString & S1, const UnicodeString & S2) const;
  virtual int32_t GetCount() const = 0;
  virtual void Insert(int32_t Index, const UnicodeString & AString, const TObject * AObject = nullptr) = 0;
  bool Equals(const TStrings * Value) const;
  virtual void Move(int32_t CurIndex, int32_t NewIndex) override;
  virtual int32_t IndexOf(const UnicodeString & S) const;
  virtual int32_t IndexOfName(const UnicodeString & Name) const;
  UnicodeString ExtractName(const UnicodeString & S) const;
  void AddStrings(const TStrings * Strings);
  void Append(const UnicodeString & Value);
  void SaveToStream(TStream * Stream) const;
  wchar_t GetDelimiter() const { return FDelimiter; }
  void SetDelimiter(wchar_t Value) { FDelimiter = Value; }
  void SetStrictDelimiter(bool Value) { FStrictDelimiter = Value; }
  wchar_t GetQuoteChar() const { return FQuoteChar; }
  void SetQuoteChar(wchar_t Value) { FQuoteChar = Value; }
  UnicodeString GetDelimitedText() const;
  void SetDelimitedText(const UnicodeString & Value);
  int32_t GetUpdateCount() const { return FUpdateCount; }
  virtual void Assign(const TPersistent *Source) override;

public:
  virtual void SetObj(int32_t Index, TObject * AObject) = 0;
  virtual bool GetSorted() const = 0;
  virtual void SetSorted(bool Value) = 0;
  virtual bool GetCaseSensitive() const = 0;
  virtual void SetCaseSensitive(bool Value) = 0;
  void SetDuplicates(TDuplicatesEnum Value);
  wchar_t GetNameValueSeparator() const { return FDelimiter; }
  UnicodeString GetCommaText() const;
  void SetCommaText(const UnicodeString & Value);
  virtual UnicodeString GetText() const;
  virtual void SetText(const UnicodeString & Text);
  virtual const UnicodeString & GetStringRef(int32_t Index) const = 0;
  virtual const UnicodeString & GetString(int32_t Index) const = 0;
  virtual UnicodeString GetString(int32_t Index) = 0;
  virtual void SetString(int32_t Index, const UnicodeString & S) = 0;
  UnicodeString GetName(int32_t Index) const;
  void SetName(int32_t Index, const UnicodeString & Value);
  UnicodeString GetValue(const UnicodeString & Name) const;
  void SetValue(const UnicodeString & Name, const UnicodeString & Value);
  UnicodeString GetValueFromIndex(int32_t Index) const;

  ROProperty<UnicodeString> Text{nb::bind(&TStrings::GetText, this)};

public:
  // TODO: ROIndexedProperty<TObject *> Objects{nb::bind(&TStrings::GetObj, this)};
  // TODO: ROIndexedProperty<UnicodeString> Names{nb::bind(&TStrings::GetName, this)};
  ROIndexedProperty<UnicodeString> Strings{nb::bind(&TStrings::GetStrings, this)};

protected:
  UnicodeString GetStrings(int32_t Index) const { return GetString(Index); }

protected:
  TDuplicatesEnum FDuplicates{dupAccept};
  mutable wchar_t FDelimiter{};
  bool FStrictDelimiter{false};
  mutable wchar_t FQuoteChar{};
  int32_t FUpdateCount{0};
};

class TStringList;
typedef int32_t (TStringListSortCompare)(TStringList *List, int32_t Index1, int32_t Index2);

NB_DEFINE_CLASS_ID(TStringList);
class NB_CORE_EXPORT TStringList : public TStrings
{
  friend int32_t StringListCompareStrings(TStringList *List, int32_t Index1, int32_t Index2);
public:
  static bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TStringList); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TStringList) || TStrings::is(Kind); }
public:
  explicit TStringList(TObjectClassId Kind = OBJECT_CLASS_TStringList) noexcept;
  virtual ~TStringList() override = default;

  int32_t Add(const UnicodeString & S);
  virtual int32_t AddObject(const UnicodeString & S, const TObject * AObject) override;
  void LoadFromFile(const UnicodeString & AFileName);
  TNotifyEvent GetOnChange() const { return FOnChange; }
  void SetOnChange(TNotifyEvent OnChange) { FOnChange = OnChange; }
  TNotifyEvent GetOnChanging() const { return FOnChanging; }
  void SetOnChanging(TNotifyEvent OnChanging) { FOnChanging = OnChanging; }
  void InsertItem(int32_t Index, const UnicodeString & S, const TObject * AObject);
  void QuickSort(int32_t L, int32_t R, TStringListSortCompare SCompare);

  virtual void Assign(const TPersistent *Source) override;
  virtual bool Find(const UnicodeString & S, int32_t &Index) const;
  virtual int32_t IndexOf(const UnicodeString & S) const override;
  virtual void Delete(int32_t Index) override;
  virtual void InsertObject(int32_t Index, const UnicodeString & Key, TObject * AObject) override;
  virtual void Sort() override;
  virtual void CustomSort(TStringListSortCompare ACompareFunc);

  virtual void SetUpdateState(bool Updating) override;
  virtual void Changing();
  virtual void Changed() override;
  virtual void Insert(int32_t Index, const UnicodeString & S, const TObject * AObject = nullptr) override;
  virtual int32_t CompareStrings(const UnicodeString & S1, const UnicodeString & S2) const override;
  virtual int32_t GetCount() const override;

public:
  virtual void SetObj(int32_t Index, TObject * AObject) override;
  virtual bool GetSorted() const override { return FSorted; }
  virtual void SetSorted(bool Value) override;
  virtual bool GetCaseSensitive() const override { return FCaseSensitive; }
  virtual void SetCaseSensitive(bool Value) override;
  virtual const UnicodeString & GetStringRef(int32_t Index) const override;
  virtual const UnicodeString & GetString(int32_t Index) const override;
  virtual UnicodeString GetString(int32_t Index) override;
  virtual void SetString(int32_t Index, const UnicodeString & S) override;

private:
  TNotifyEvent FOnChange;
  TNotifyEvent FOnChanging;
  nb::vector_t<UnicodeString> FStrings;
  bool FSorted{false};
  bool FCaseSensitive{false};

private:
  void ExchangeItems(int32_t Index1, int32_t Index2);

private:
  TStringList(const TStringList &) = delete;
  TStringList &operator=(const TStringList &) = delete;
};

/// TDateTime: number of days since 12/30/1899
class NB_CORE_EXPORT TDateTime : public TObject
{
public:
  TDateTime() noexcept = default;
  explicit TDateTime(double Value) noexcept : FValue(Value) {}
  explicit TDateTime(uint16_t Hour,
    uint16_t Min, uint16_t Sec, uint16_t MSec = 0);
  TDateTime(const TDateTime &rhs) noexcept : FValue(rhs.FValue) {}
  double GetValue() const { return operator double(); }
  TDateTime &operator=(const TDateTime &rhs)
  {
    FValue = rhs.FValue;
    return *this;
  }
  operator double() const
  {
    return FValue;
  }
  TDateTime &operator+(const TDateTime &rhs)
  {
    FValue += rhs.FValue;
    return *this;
  }
  TDateTime &operator+=(const TDateTime &rhs)
  {
    FValue += rhs.FValue;
    return *this;
  }
  TDateTime &operator+=(double val)
  {
    FValue += val;
    return *this;
  }
  TDateTime &operator-(const TDateTime &rhs)
  {
    FValue -= rhs.FValue;
    return *this;
  }
  TDateTime &operator-=(const TDateTime &rhs)
  {
    FValue -= rhs.FValue;
    return *this;
  }
  TDateTime &operator-=(double val)
  {
    FValue -= val;
    return *this;
  }
  TDateTime &operator=(double Value)
  {
    FValue = Value;
    return *this;
  }
  bool operator==(const TDateTime &rhs) const;
  bool operator!=(const TDateTime &rhs) const
  {
    return !(operator==(rhs));
  }
  UnicodeString GetDateString() const;
  UnicodeString GetTimeString(bool Short) const;
  UnicodeString FormatString(const wchar_t *fmt) const;
  void DecodeDate(uint16_t &Y, uint16_t &M, uint16_t &D) const;
  void DecodeTime(uint16_t &H, uint16_t &N, uint16_t &S, uint16_t &MS) const;
private:
  double FValue{0.0};
};

//MinDateTime: TDateTime =  -693593.0;     { 01/01/0001 12:00:00.000 AM }
//MaxDateTime: TDateTime =  2958465.99999; { 12/31/9999 11:59:59.999 PM }
//#define MinDateTime TDateTime(-657434.0)
constexpr const double MinDateTime = -657434.0;
constexpr const double MaxDateTime = 2958465.99999;

// TODO: move to DateUtils.hpp
NB_CORE_EXPORT TDateTime Now();
NB_CORE_EXPORT TDateTime SpanOfNowAndThen(const TDateTime &ANow, const TDateTime &AThen);
NB_CORE_EXPORT double MilliSecondSpan(const TDateTime &ANow, const TDateTime &AThen);

class TTimeSpan
{
private:
  int64_t GetTicks() const { return FTicks; }
  int32_t GetDays() const { return FTicks / TicksPerDay; }
  int32_t GetHours() const { return (FTicks / TicksPerHour) % HoursPerDay; }
  int32_t GetMinutes() const { return (FTicks / TicksPerMinute) % MinsPerHour; }
  int32_t GetSeconds() const { return (FTicks / TicksPerSecond) % SecsPerMin; }
  int32_t GetMilliseconds() const { return (FTicks / TicksPerMillisecond) % MillisPerSecond; }
  double GetTotalDays() const { return (double)FTicks / TicksPerDay; }
  double GetTotalHours() const { return (double)FTicks / TicksPerHour; }
  double GetTotalMinutes() const { return (double)FTicks / TicksPerMinute; }
  double GetTotalSeconds() const { return (double)FTicks / TicksPerSecond; }
  double GetTotalMilliseconds() const { return (double)FTicks / TicksPerMillisecond; }
  static TTimeSpan GetScaledInterval(double Value, int32_t Scale);
private:
  static const int64_t FMinValue = -9223372036854775808;
  static const int64_t FMaxValue = 0x7FFFFFFFFFFFFFFF;
  static const int64_t FZero = 0;
private:
  double MillisecondsPerTick = 0.0001;
  double SecondsPerTick = 1e-07;
  double MinutesPerTick = 1.6666666666666667E-09;
  double HoursPerTick = 2.7777777777777777E-11;
  double DaysPerTick = 1.1574074074074074E-12;
  int32_t MillisPerSecond = 1000;
  int32_t MillisPerMinute = 60 * MillisPerSecond;
  int32_t MillisPerHour = 60 * MillisPerMinute;
  int32_t MillisPerDay = 24 * MillisPerHour;
  int64_t MaxSeconds = 922337203685;
  int64_t MinSeconds = -922337203685;
  int64_t MaxMilliseconds = 922337203685477;
  int64_t MinMilliseconds = -922337203685477;
public:
  static const int32_t TicksPerMillisecond = 10000;
  static const int64_t TicksPerSecond = 1000 * int64_t(TicksPerMillisecond);
  static const int64_t TicksPerMinute = 60 * int64_t(TicksPerSecond);
  static const int64_t TicksPerHour = 60 * int64_t(TicksPerMinute);
  static const int64_t TicksPerDay = 24 * TicksPerHour;
public:
  explicit TTimeSpan(int64_t ATicks);
  explicit TTimeSpan(int32_t Hours, int32_t Minutes, int32_t Seconds);
  explicit TTimeSpan(int32_t Days, int32_t Hours, int32_t Minutes, int32_t Seconds);
  explicit TTimeSpan(int32_t Days, int32_t Hours, int32_t Minutes, int32_t Seconds, int32_t Milliseconds);
  TTimeSpan Add(const TTimeSpan TS) const;
  TTimeSpan Duration() const;
  TTimeSpan Negate() const;
  TTimeSpan Subtract(const TTimeSpan TS);
  UnicodeString ToString() const;

  static TTimeSpan FromDays(double Value);
  static TTimeSpan FromHours(double Value);
  static TTimeSpan FromMinutes(double Value);
  static TTimeSpan FromSeconds(double Value);
  static TTimeSpan FromMilliseconds(double Value);
  static TTimeSpan FromTicks(int64_t Value);
  static TTimeSpan Subtract(const TDateTime D1, const TDateTime D2);
  static TTimeSpan Parse(const UnicodeString & S);
  static bool TryParse(const UnicodeString & S, const TTimeSpan & Value);
  static TTimeSpan Add(const TTimeSpan Left, const TTimeSpan Right);
  static TDateTime Add(const TTimeSpan Left, const TDateTime Right);
  static TDateTime Add(const TDateTime Left, const TTimeSpan Right);
  static TTimeSpan Subtract(const TTimeSpan Left, const TTimeSpan Right);
  static TDateTime Subtract(const TDateTime Left, const TTimeSpan Right);
  static bool Equal(const TTimeSpan Left, const TTimeSpan Right);
  static bool NotEqual(const TTimeSpan Left, const TTimeSpan Right);
  static bool GreaterThan(const TTimeSpan Left, const TTimeSpan Right);
  static bool GreaterThanOrEqual(const TTimeSpan Left, const TTimeSpan Right);
  static bool LessThan(const TTimeSpan Left, const TTimeSpan Right);
  static bool LessThanOrEqual(const TTimeSpan Left, const TTimeSpan Right) { return Left.FTicks <= Right.FTicks; }
  bool operator==(const TTimeSpan &rhs) const { return TTimeSpan::Equal(*this, rhs); }
  bool operator!=(const TTimeSpan &rhs) const { return TTimeSpan::NotEqual(*this, rhs); }
  bool operator>(const TTimeSpan &rhs) const { return TTimeSpan::GreaterThan(*this, rhs); }
  bool operator>=(const TTimeSpan &rhs) const { return TTimeSpan::GreaterThanOrEqual(*this, rhs); }
  bool operator<(const TTimeSpan &rhs) const { return TTimeSpan::LessThan(*this, rhs); }
  bool operator<=(const TTimeSpan &rhs) const { return TTimeSpan::LessThanOrEqual(*this, rhs); }
  static TTimeSpan Negative(const TTimeSpan Value);
  static TTimeSpan Positive(const TTimeSpan Value);
  static UnicodeString Implicit(const TTimeSpan Value);
  static UnicodeString Explicit(const TTimeSpan Value);
  ROProperty<int64_t> Ticks{nb::bind(&TTimeSpan::GetTicks, this)};
  ROProperty<int32_t> Days{nb::bind(&TTimeSpan::GetDays, this)};
  ROProperty<int32_t> Hours{nb::bind(&TTimeSpan::GetHours, this)};
  ROProperty<int32_t> Minutes{nb::bind(&TTimeSpan::GetMinutes, this)};
  ROProperty<int32_t> Seconds{nb::bind(&TTimeSpan::GetSeconds, this)};
  ROProperty<int32_t> Milliseconds{nb::bind(&TTimeSpan::GetMilliseconds, this)};
  ROProperty<double> TotalDays{nb::bind(&TTimeSpan::GetTotalDays, this)};
  ROProperty<double> TotalHours{nb::bind(&TTimeSpan::GetTotalHours, this)};
  ROProperty<double> TotalMinutes{nb::bind(&TTimeSpan::GetTotalMinutes, this)};
  ROProperty<double> TotalSeconds{nb::bind(&TTimeSpan::GetTotalSeconds, this)};
  ROProperty<double> TotalMilliseconds{nb::bind(&TTimeSpan::GetTotalMilliseconds, this)};
  static TTimeSpan GetMinValue() { return TTimeSpan(FMinValue); }
  static TTimeSpan GetMaxValue() { return TTimeSpan(FMaxValue); }
  static TTimeSpan GetZero() { return TTimeSpan(FZero); }
private:
  int64_t FTicks{0};
};

#if 0
class NB_CORE_EXPORT TSHFileInfo : public TObject
{
  typedef DWORD_PTR (WINAPI *TGetFileInfo)(
    _In_ LPCTSTR pszPath,
    DWORD dwFileAttributes,
    _Inout_ SHFILEINFO *psfi,
    UINT cbFileInfo,
    UINT uFlags);

public:
  TSHFileInfo() noexcept;
  virtual ~TSHFileInfo() noexcept;

  //get the image's index in the system's image list
  int GetFileIconIndex(const UnicodeString & StrFileName, BOOL bSmallIcon) const;
  int GetDirIconIndex(BOOL bSmallIcon);

  //get file type
  UnicodeString GetFileType(const UnicodeString & StrFileName);

private:
  TGetFileInfo FGetFileInfo;
  TLibraryLoader FSHFileInfoLoader;
};
#endif // #if 0

constexpr const uint32_t soFromBeginning = 0;
constexpr const uint32_t soFromCurrent = 1;
constexpr const uint32_t soFromEnd = 2;

enum class TSeekOrigin { soBeginning = soFromBeginning, soCurrent = soFromCurrent, soEnd = soFromEnd };

// TFileStream create mode
constexpr const uint16_t fmCreate = 0xFFFF;
constexpr const uint16_t fmOpenRead = 0x0;
constexpr const uint16_t fmOpenWrite = 0x1;
constexpr const uint16_t fmOpenReadWrite = 0x2;
// Share modes
constexpr const uint16_t fmShareCompat    = 0x0000;
constexpr const uint16_t fmShareExclusive = 0x0010;
constexpr const uint16_t fmShareDenyWrite = 0x0020;
constexpr const uint16_t fmShareDenyRead  = 0x0030;
constexpr const uint16_t fmShareDenyNone  = 0x0040;

class NB_CORE_EXPORT TStream : public TObject
{
public:
  TStream() = default;
  virtual ~TStream() = default;
  virtual int64_t Read(void *Buffer, int64_t Count) = 0;
  virtual int64_t Write(const void *Buffer, int64_t Count) = 0;
  virtual int64_t Seek(const int64_t Offset, TSeekOrigin Origin) const = 0;
  void ReadBuffer(void *Buffer, int64_t Count);
  void WriteBuffer(const void * Buffer, int64_t Count);
  int64_t CopyFrom(TStream * Source, int64_t Count);

public:
  int64_t GetPosition() const;
  virtual int64_t GetSize() const;
  virtual void SetSize(int64_t NewSize) = 0;
  void SetPosition(int64_t Pos);

  RWProperty3<int64_t> Size{nb::bind(&TStream::GetSize, this), nb::bind(&TStream::SetSize, this)};
  RWProperty3<int64_t> Position{nb::bind(&TStream::GetPosition, this), nb::bind(&TStream::SetPosition, this)};
};

class NB_CORE_EXPORT THandleStream : public TStream
{
  NB_DISABLE_COPY(THandleStream)
public:
  using TStream::TStream;
  explicit THandleStream(HANDLE AHandle) noexcept;
  virtual ~THandleStream() = default;
  virtual int64_t Read(void *Buffer, int64_t Count) override;
  virtual int64_t Write(const void *Buffer, int64_t Count) override;
  virtual int64_t Seek(const int64_t Offset, TSeekOrigin Origin) const override;
  ROProperty<HANDLE> Handle{nb::bind(&THandleStream::GetHandle, this)};

  HANDLE GetHandle() const { return FHandle; }

protected:
  virtual void SetSize(int64_t NewSize) override;

protected:
  HANDLE FHandle{};
};

class TFileStream : public THandleStream
{
public:
  explicit TFileStream(const UnicodeString & AFileName, uint16_t Mode);
  //explicit TFileStream(const UnicodeString & AFileName, uint16_t Mode, uint32_t Rights);
  ~TFileStream();
  UnicodeString GetFileName() const { return FFileName; }
private:
  UnicodeString FFileName;
};

class NB_CORE_EXPORT TSafeHandleStream : public THandleStream
{
public:
  explicit TSafeHandleStream(THandle AHandle) noexcept;
  TSafeHandleStream(THandleStream * Source, bool Own);
  static TSafeHandleStream * CreateFromFile(const UnicodeString & FileName, uint16_t Mode);
  virtual ~TSafeHandleStream();
  virtual int64_t Read(void * Buffer, int64_t Count) override;
  virtual int64_t Write(const void * Buffer, int64_t Count) override;
private:
  THandleStream * FSource{nullptr};
  bool FOwned{false};
};

class NB_CORE_EXPORT EReadError : public std::runtime_error
{
public:
  explicit EReadError(const char *Msg) noexcept : std::runtime_error(Msg) {}
};

class NB_CORE_EXPORT EWriteError : public std::runtime_error
{
public:
  explicit EWriteError(const char *Msg) noexcept : std::runtime_error(Msg) {}
};

class NB_CORE_EXPORT TMemoryStream : public TStream
{
  NB_DISABLE_COPY(TMemoryStream)
public:
  TMemoryStream() noexcept;
  virtual ~TMemoryStream() noexcept;
  virtual int64_t Read(void * Buffer, int64_t Count) override;
  virtual int64_t Seek(const int64_t Offset, TSeekOrigin Origin) const override;
  void SaveToStream(TStream * Stream);
  void SaveToFile(const UnicodeString & AFileName);

  void Clear();
  void LoadFromStream(TStream * Stream);
  __removed void LoadFromFile(const UnicodeString & AFileName);
  virtual int64_t GetSize() const override { return FSize; }
  virtual void SetSize(int64_t NewSize) override;
  virtual int64_t Write(const void * Buffer, int64_t Count) override;

  void * GetMemory() const { return FMemory; }

protected:
  void SetPointer(void * Ptr, int64_t ASize);
  virtual void *Realloc(int64_t & NewCapacity);
  int64_t GetCapacity() const { return FCapacity; }

private:
  void SetCapacity(int64_t NewCapacity);

private:
  void * FMemory{nullptr};
  int64_t FSize{0};
  mutable int64_t FPosition{0};
  int64_t FCapacity{0};
};

struct TRegKeyInfo
{
  DWORD NumSubKeys{0};
  DWORD MaxSubKeyLen{0};
  DWORD NumValues{0};
  DWORD MaxValueLen{0};
  DWORD MaxDataLen{0};
  FILETIME FileTime{};
};

enum TRegDataType
{
  rdUnknown, rdString, rdExpandString, rdInteger, rdIntPtr, rdBinary
};

struct TRegDataInfo
{
  TRegDataType RegData{};
  DWORD DataSize{0};
};

class NB_CORE_EXPORT TRegistry final : public TObject
{
  NB_DISABLE_COPY(TRegistry)
public:
  TRegistry() noexcept;
  ~TRegistry() noexcept;
  void GetValueNames(TStrings *Names) const;
  void GetKeyNames(TStrings *Names) const;
  void CloseKey();
  bool OpenKey(const UnicodeString & AKey, bool CanCreate);
  bool DeleteKey(const UnicodeString & AKey);
  bool DeleteValue(const UnicodeString & Value) const;
  bool KeyExists(const UnicodeString & SubKey) const;
  bool ValueExists(const UnicodeString & Value) const;
  bool GetDataInfo(const UnicodeString & ValueName, TRegDataInfo & Value) const;
  TRegDataType GetDataType(const UnicodeString & ValueName) const;
  DWORD GetDataSize(const UnicodeString & ValueName) const;
  bool ReadBool(const UnicodeString & Name) const;
  TDateTime ReadDateTime(const UnicodeString & Name) const;
  double ReadFloat(const UnicodeString & Name) const;
  intptr_t ReadIntPtr(const UnicodeString & Name) const;
  int32_t ReadInteger(const UnicodeString & Name) const;
  int64_t ReadInt64(const UnicodeString & Name) const;
  UnicodeString ReadString(const UnicodeString & Name) const;
  UnicodeString ReadStringRaw(const UnicodeString & Name) const;
  size_t ReadBinaryData(const UnicodeString & Name,
    void *Buffer, size_t BufSize) const;

  void WriteBool(const UnicodeString & Name, bool Value);
  void WriteDateTime(const UnicodeString & Name, const TDateTime &Value);
  void WriteFloat(const UnicodeString & Name, double Value);
  void WriteString(const UnicodeString & Name, const UnicodeString & Value);
  void WriteStringRaw(const UnicodeString & Name, const UnicodeString & Value);
  void WriteIntPtr(const UnicodeString & Name, intptr_t Value);
  void WriteInteger(const UnicodeString & Name, int32_t Value);
  void WriteInt64(const UnicodeString & Name, int64_t Value);
  void WriteBinaryData(const UnicodeString & Name,
    const void *Buffer, size_t BufSize);
private:
  void ChangeKey(HKEY Value, const UnicodeString & APath);
  HKEY GetBaseKey(bool Relative) const;
  HKEY GetKey(const UnicodeString & AKey) const;
  void SetCurrentKey(HKEY Value) { FCurrentKey = Value; }
  bool GetKeyInfo(TRegKeyInfo & Value) const;
  int GetData(const UnicodeString & Name, void * Buffer,
    int32_t ABufSize, TRegDataType & RegData) const;
  void PutData(const UnicodeString & Name, const void *Buffer,
    int32_t ABufSize, TRegDataType RegData);

public:
  void SetAccess(uint32_t Value);
  HKEY GetCurrentKey() const;
  HKEY GetRootKey() const;
  void SetRootKey(HKEY ARootKey);
  UnicodeString GetCurrentPath() const { return FCurrentPath; }

  HKEY& RootKey{FRootKey};
  uint32_t& Access{FAccess};

private:
  HKEY FCurrentKey{};
  HKEY FRootKey{};
  // bool FLazyWrite;
  UnicodeString FCurrentPath;
  bool FCloseRootKey{false};
  mutable uint32_t FAccess{KEY_ALL_ACCESS};
};

struct TTimeStamp
{
  int32_t Time{0}; // Number of milliseconds since midnight
  int32_t Date{0}; // One plus number of days since 1/1/0001
};

template <class PropType>
class NB_CORE_EXPORT TValidProperties // : public TObject
{
  CUSTOM_MEM_ALLOCATION_IMPL
public:
  TValidProperties() = default;
  void Clear()
  {
    FValue = 0;
  }
  bool Contains(PropType Value) const
  {
    return (FValue & Value) != 0;
  }
  bool operator==(const TValidProperties &rhs) const
  {
    return FValue == rhs.FValue;
  }
  bool operator!=(const TValidProperties &rhs) const
  {
    return !(operator==(rhs));
  }
  TValidProperties &operator<<(const PropType Value)
  {
    FValue |= Value;
    return *this;
  }
  TValidProperties &operator>>(const PropType Value)
  {
    FValue &= ~(nb::ToInt64(Value));
    return *this;
  }
  bool Empty() const
  {
    return FValue == 0;
  }

private:
  int64_t FValue{0};
};

// FIXME
class NB_CORE_EXPORT TShortCut : public TObject
{
public:
  explicit TShortCut() = default;
  explicit TShortCut(int32_t Value) noexcept;
  operator int32_t() const;
  bool operator<(const TShortCut &rhs) const;
  int32_t Compare(const TShortCut &rhs) const { return FValue - rhs.FValue; }

private:
  int32_t FValue{0};
};

enum TReplaceFlag
{
  rfReplaceAll,
  rfIgnoreCase
};

enum TShiftStateFlag
{
  ssShift, ssAlt, ssCtrl, ssLeft, ssRight, ssMiddle, ssDouble, ssTouch, ssPen
};

inline double Trunc(double Value) { double intpart; modf(Value, &intpart); return intpart; }
inline double Frac(double Value) { double intpart; return modf(Value, &intpart); }
inline double Abs(double Value) { return fabs(Value); }

// forms\InputDlg.cpp
struct TInputDialogData
{
//  TCustomEdit * Edit;
  void *Edit{nullptr};
};

using TInputDialogInitializeEvent = nb::FastDelegate2<void,
  TObject * /*Sender*/, TInputDialogData * /*Data*/>;

enum TQueryType
{
  qtConfirmation,
  qtWarning,
  qtError,
  qtInformation,
};

struct TMessageParams;

class NB_CORE_EXPORT TGlobalsIntf
{
public:
  virtual ~TGlobalsIntf() = default;

  virtual HINSTANCE GetInstanceHandle() const = 0;
  virtual UnicodeString GetMsg(int32_t Id) const = 0;
  virtual UnicodeString GetCurrDirectory() const = 0;
  virtual UnicodeString GetStrVersionNumber() const = 0;
  virtual void SetupDbgHandles(const UnicodeString & DbgFileName) = 0;
  virtual bool InputDialog(const UnicodeString & ACaption,
    const UnicodeString & APrompt, UnicodeString & Value, const UnicodeString & HelpKeyword,
    TStrings * History, bool PathInput,
    TInputDialogInitializeEvent OnInitialize, bool Echo) = 0;
  virtual uint32_t MoreMessageDialog(const UnicodeString & AMessage,
    TStrings * MoreMessages, TQueryType Type, uint32_t Answers,
    const TMessageParams * Params) = 0;
};

class NB_CORE_EXPORT TGlobals : public TGlobalsIntf, public TObject
{
public:
  TGlobals() noexcept;
  virtual ~TGlobals() = default;

  virtual void SetupDbgHandles(const UnicodeString & DbgFileName) override;

public:
  wchar_t Win32CSDVersion[128]{};
  int32_t Win32Platform{0};
  int32_t Win32MajorVersion{0};
  int32_t Win32MinorVersion{0};
  int32_t Win32BuildNumber{0};

private:
  void InitPlatformId();
private:
  std::ofstream dbgstream_;
};

NB_CORE_EXPORT TGlobals *GetGlobals();
NB_CORE_EXPORT void SetGlobals(TGlobals *Value);

template<typename T>
class TGlobalsIntfInitializer
{
public:
  TGlobalsIntfInitializer()
  {
    ::SetGlobals(new T());
  }

  ~TGlobalsIntfInitializer()
  {
    TGlobalsIntf *Intf = GetGlobals();
    delete Intf;
    ::SetGlobals(nullptr);
  }
};
