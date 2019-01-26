#pragma once

#include <nbsystem.h>

#include <stdexcept>
#include <limits>
#include <stdarg.h>
#include <math.h>
#include <rdestl/vector.h>
#include <rdestl/pair.h>

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

class Exception;

constexpr intptr_t MonthsPerYear = 12;
constexpr intptr_t DaysPerWeek = 7;
constexpr intptr_t MinsPerHour = 60;
constexpr intptr_t SecsPerMin = 60;
constexpr intptr_t HoursPerDay = 24;
constexpr intptr_t MSecsPerSec = 1000;
constexpr intptr_t SecsPerHour = MinsPerHour * SecsPerMin;
constexpr intptr_t MinsPerDay = HoursPerDay * MinsPerHour;
constexpr intptr_t SecsPerDay = MinsPerDay * SecsPerMin;
constexpr intptr_t MSecsPerDay = SecsPerDay * MSecsPerSec;
constexpr intptr_t OneSecond = MSecsPerSec;
// Days between 1/1/0001 and 12/31/1899
constexpr intptr_t DateDelta = 693594;
constexpr intptr_t UnixDateDelta = 25569;

class TObject;

using TThreadMethod = nb::FastDelegate0<void>;
using TNotifyEvent = nb::FastDelegate1<void, TObject * /*Sender*/>;

NB_CORE_EXPORT void Abort();
NB_CORE_EXPORT void Error(intptr_t Id, intptr_t ErrorId);
NB_CORE_EXPORT void ThrowNotImplemented(intptr_t ErrorId);

enum class TObjectClassId {};
#define NB_DEFINE_CLASS_ID(CLASS_ID) static constexpr TObjectClassId OBJECT_CLASS_ ## CLASS_ID = (TObjectClassId)nb::counter_id()

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
  virtual ~TObject() noexcept = default;
  virtual void Changed() {}
private:
  TObjectClassId FKind{};
};

template<class O, class T>
inline O as_object(T p) { return static_cast<O>(p); }
inline TObject *as_object(void *p) { return as_object<TObject *, void *>(p); }
inline const TObject *as_object(const void *p) { return as_object<const TObject *, const void *>(p); }
template<class O, class T>
inline O *get_as(T p) { return dyn_cast<O>(as_object(p)); }
template<class O> inline O *get_as(void *p) { return get_as<O, void *>(p); }
template<class O> inline const O *get_as(const void *p) { return get_as<const O, const void *>(p); }
template <class T>
inline TObject *ToObj(const T &a) { return reinterpret_cast<TObject *>(nb::ToSizeT(a)); }

struct TPoint
{
  int x{0};
  int y{0};
  TPoint() noexcept = default;
  TPoint(int ax, int ay) noexcept : x(ax), y(ay) {}
};

struct TRect
{
  int Left{0};
  int Top{0};
  int Right{0};
  int Bottom{0};
  int Width() const { return Right - Left; }
  int Height() const { return Bottom - Top; }
  TRect() noexcept = default;
  TRect(int left, int top, int right, int bottom) noexcept :
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
  static bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TPersistent); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TPersistent) || TObject::is(Kind); }
public:
  TPersistent() noexcept : TObject(OBJECT_CLASS_TPersistent) {}
  explicit TPersistent(TObjectClassId Kind) noexcept;
  virtual ~TPersistent() noexcept = default;
  virtual void Assign(const TPersistent *Source);
  virtual TPersistent *GetOwner();
protected:
  virtual void AssignTo(TPersistent *Dest) const;
private:
  void AssignError(const TPersistent *Source);
};

enum TListNotification
{
  lnAdded,
  lnExtracted,
  lnDeleted,
};

typedef intptr_t (CompareFunc)(const void *Item1, const void *Item2);

NB_DEFINE_CLASS_ID(TList);
class NB_CORE_EXPORT TList : public TPersistent
{
public:
  static bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TList); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TList) || TPersistent::is(Kind); }
public:
  TList() noexcept;
  explicit TList(TObjectClassId Kind) noexcept;
  virtual ~TList() noexcept;

  template<class T>
  T *GetAs(intptr_t Index) const { return get_as<T>(GetItem(Index)); }
  void *operator[](intptr_t Index) const;
  virtual void *GetItem(intptr_t Index) const { return FList[Index]; }
  virtual void *GetItem(intptr_t Index) { return FList[Index]; }
  void SetItem(intptr_t Index, void *Item);
  intptr_t Add(void *Value);
  void *Extract(void *Item);
  intptr_t Remove(void *Item);
  virtual void Move(intptr_t CurIndex, intptr_t NewIndex);
  virtual void Delete(intptr_t Index);
  void Insert(intptr_t Index, void *Item);
  intptr_t IndexOf(const void *Value) const;
  virtual void Clear();
  void Sort(CompareFunc Func);
  virtual void Notify(void *Ptr, TListNotification Action);
  virtual void Sort();

  intptr_t GetCount() const;
  void SetCount(intptr_t NewCount);

  ROProperty<intptr_t> Count{nb::bind(&TList::GetCount, this)};

private:
  rde::vector<void *> FList;
};

NB_DEFINE_CLASS_ID(TObjectList);
class NB_CORE_EXPORT TObjectList : public TList
{
public:
  static bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TObjectList); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TObjectList) || TList::is(Kind); }
public:
  TObjectList() noexcept;
  explicit TObjectList(TObjectClassId Kind) noexcept;
  virtual ~TObjectList() noexcept;

  template<class T>
  T *GetAs(intptr_t Index) const { return dyn_cast<T>(GetObj(Index)); }
  TObject *operator[](intptr_t Index) const;
  TObject *GetObj(intptr_t Index) const;
  bool GetOwnsObjects() const { return FOwnsObjects; }
  void SetOwnsObjects(bool Value) { FOwnsObjects = Value; }
  void Notify(void *Ptr, TListNotification Action) override;

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
  virtual ~TStrings() noexcept = default;
  intptr_t Add(UnicodeString S, TObject *AObject = nullptr);
  virtual UnicodeString GetTextStr() const;
  virtual void SetTextStr(UnicodeString Text);
  virtual void BeginUpdate();
  virtual void EndUpdate();
  virtual void SetUpdateState(bool Updating);
  virtual intptr_t AddObject(UnicodeString S, TObject *AObject);
  virtual void InsertObject(intptr_t Index, UnicodeString Key, TObject *AObject);
  virtual intptr_t CompareStrings(UnicodeString S1, UnicodeString S2) const;
  virtual intptr_t GetCount() const = 0;
  virtual void Insert(intptr_t Index, UnicodeString AString, TObject *AObject = nullptr) = 0;
  bool Equals(const TStrings *Value) const;
  void Move(intptr_t CurIndex, intptr_t NewIndex) override;
  virtual intptr_t IndexOf(UnicodeString S) const;
  virtual intptr_t IndexOfName(UnicodeString Name) const;
  UnicodeString ExtractName(UnicodeString S) const;
  void AddStrings(const TStrings *Strings);
  void Append(UnicodeString Value);
  void SaveToStream(TStream *Stream) const;
  wchar_t GetDelimiter() const { return FDelimiter; }
  void SetDelimiter(wchar_t Value) { FDelimiter = Value; }
  void SetStrictDelimiter(bool Value) { FStrictDelimiter = Value; }
  wchar_t GetQuoteChar() const { return FQuoteChar; }
  void SetQuoteChar(wchar_t Value) { FQuoteChar = Value; }
  UnicodeString GetDelimitedText() const;
  void SetDelimitedText(UnicodeString Value);
  intptr_t GetUpdateCount() const { return FUpdateCount; }
  void Assign(const TPersistent *Source) override;

public:
  virtual void SetObj(intptr_t Index, TObject *AObject) = 0;
  virtual bool GetSorted() const = 0;
  virtual void SetSorted(bool Value) = 0;
  virtual bool GetCaseSensitive() const = 0;
  virtual void SetCaseSensitive(bool Value) = 0;
  void SetDuplicates(TDuplicatesEnum Value);
  UnicodeString GetCommaText() const;
  void SetCommaText(UnicodeString Value);
  virtual UnicodeString GetText() const;
  virtual void SetText(UnicodeString Text);
  virtual const UnicodeString &GetStringRef(intptr_t Index) const = 0;
  virtual const UnicodeString &GetString(intptr_t Index) const = 0;
  virtual UnicodeString GetString(intptr_t Index) = 0;
  virtual void SetString(intptr_t Index, UnicodeString S) = 0;
  UnicodeString GetName(intptr_t Index) const;
  void SetName(intptr_t Index, UnicodeString Value);
  UnicodeString GetValue(UnicodeString Name) const;
  void SetValue(UnicodeString Name, UnicodeString Value);
  UnicodeString GetValueFromIndex(intptr_t Index) const;

protected:
  TDuplicatesEnum FDuplicates{dupAccept};
  mutable wchar_t FDelimiter{};
  bool FStrictDelimiter{false};
  mutable wchar_t FQuoteChar{};
  intptr_t FUpdateCount{0};
};

class TStringList;
typedef intptr_t (TStringListSortCompare)(TStringList *List, intptr_t Index1, intptr_t Index2);

NB_DEFINE_CLASS_ID(TStringList);
class NB_CORE_EXPORT TStringList : public TStrings
{
  friend intptr_t StringListCompareStrings(TStringList *List, intptr_t Index1, intptr_t Index2);
public:
  static bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TStringList); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TStringList) || TStrings::is(Kind); }
public:
  explicit TStringList(TObjectClassId Kind = OBJECT_CLASS_TStringList) noexcept;
  virtual ~TStringList() noexcept = default;

  intptr_t Add(UnicodeString S);
  intptr_t AddObject(UnicodeString S, TObject *AObject) override;
  void LoadFromFile(UnicodeString AFileName);
  TNotifyEvent GetOnChange() const { return FOnChange; }
  void SetOnChange(TNotifyEvent OnChange) { FOnChange = OnChange; }
  TNotifyEvent GetOnChanging() const { return FOnChanging; }
  void SetOnChanging(TNotifyEvent OnChanging) { FOnChanging = OnChanging; }
  void InsertItem(intptr_t Index, UnicodeString S, TObject *AObject);
  void QuickSort(intptr_t L, intptr_t R, TStringListSortCompare SCompare);

  void Assign(const TPersistent *Source) override;
  virtual bool Find(UnicodeString S, intptr_t &Index) const;
  intptr_t IndexOf(UnicodeString S) const override;
  void Delete(intptr_t Index) override;
  void InsertObject(intptr_t Index, UnicodeString Key, TObject *AObject) override;
  void Sort() override;
  virtual void CustomSort(TStringListSortCompare ACompareFunc);

  void SetUpdateState(bool Updating) override;
  virtual void Changing();
  void Changed() override;
  void Insert(intptr_t Index, UnicodeString S, TObject *AObject = nullptr) override;
  intptr_t CompareStrings(UnicodeString S1, UnicodeString S2) const override;
  intptr_t GetCount() const override;

public:
  void SetObj(intptr_t Index, TObject *AObject) override;
  bool GetSorted() const override { return FSorted; }
  void SetSorted(bool Value) override;
  bool GetCaseSensitive() const override { return FCaseSensitive; }
  void SetCaseSensitive(bool Value) override;
  const UnicodeString &GetStringRef(intptr_t Index) const override;
  const UnicodeString &GetString(intptr_t Index) const override;
  UnicodeString GetString(intptr_t Index) override;
  void SetString(intptr_t Index, UnicodeString S) override;

private:
  TNotifyEvent FOnChange;
  TNotifyEvent FOnChanging;
  rde::vector<UnicodeString> FStrings;
  bool FSorted{false};
  bool FCaseSensitive{false};

private:
  void ExchangeItems(intptr_t Index1, intptr_t Index2);

private:
  TStringList(const TStringList &) = delete;
  TStringList &operator=(const TStringList &) = delete;
};

/// TDateTime: number of days since 12/30/1899
class NB_CORE_EXPORT TDateTime : public TObject
{
public:
  TDateTime() noexcept : FValue(0.0) {}
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

#define MinDateTime TDateTime(-657434.0)

NB_CORE_EXPORT TDateTime Now();
NB_CORE_EXPORT TDateTime SpanOfNowAndThen(const TDateTime &ANow, const TDateTime &AThen);
NB_CORE_EXPORT double MilliSecondSpan(const TDateTime &ANow, const TDateTime &AThen);
NB_CORE_EXPORT int64_t MilliSecondsBetween(const TDateTime &ANow, const TDateTime &AThen);
NB_CORE_EXPORT int64_t SecondsBetween(const TDateTime &ANow, const TDateTime &AThen);

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
  int GetFileIconIndex(UnicodeString StrFileName, BOOL bSmallIcon) const;
  int GetDirIconIndex(BOOL bSmallIcon);

  //get file type
  UnicodeString GetFileType(UnicodeString StrFileName);

private:
  TGetFileInfo FGetFileInfo;
  TLibraryLoader FSHFileInfoLoader;
};
#endif // #if 0

enum TSeekOrigin
{
  soFromBeginning = 0,
  soFromCurrent = 1,
  soFromEnd = 2
};

class NB_CORE_EXPORT TStream : public TObject
{
public:
  TStream() noexcept = default;
  virtual ~TStream() noexcept = default;
  virtual int64_t Read(void *Buffer, int64_t Count) = 0;
  virtual int64_t Write(const void *Buffer, int64_t Count) = 0;
  virtual int64_t Seek(int64_t Offset, int Origin) const = 0;
  virtual int64_t Seek(const int64_t Offset, TSeekOrigin Origin) const = 0;
  void ReadBuffer(void *Buffer, int64_t Count);
  void WriteBuffer(const void *Buffer, int64_t Count);
  int64_t CopyFrom(TStream *Source, int64_t Count);

public:
  int64_t GetPosition() const;
  virtual int64_t GetSize() const;
  virtual void SetSize(const int64_t NewSize) = 0;
  void SetPosition(const int64_t Pos);

  RWProperty<int64_t> Position{nb::bind(&TStream::GetPosition, this), nb::bind(&TStream::SetPosition, this)};
  RWProperty<int64_t> Size{nb::bind(&TStream::GetSize, this), nb::bind(&TStream::SetSize, this)};
};

class NB_CORE_EXPORT THandleStream : public TStream
{
  NB_DISABLE_COPY(THandleStream)
public:
  explicit THandleStream(HANDLE AHandle) noexcept;
  virtual ~THandleStream() noexcept = default;
  int64_t Read(void *Buffer, int64_t Count) override;
  int64_t Write(const void *Buffer, int64_t Count) override;
  int64_t Seek(int64_t Offset, int Origin) const override;
  int64_t Seek(const int64_t Offset, TSeekOrigin Origin) const override;

  HANDLE GetHandle() const { return FHandle; }

protected:
  void SetSize(const int64_t NewSize) override;

protected:
  HANDLE FHandle{};
};

class NB_CORE_EXPORT TSafeHandleStream : public THandleStream
{
public:
  explicit TSafeHandleStream(THandle AHandle) noexcept;
  virtual ~TSafeHandleStream() noexcept = default;
  int64_t Read(void *Buffer, int64_t Count) override;
  int64_t Write(const void *Buffer, int64_t Count) override;
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
  int64_t Read(void *Buffer, int64_t Count) override;
  int64_t Seek(int64_t Offset, int Origin) const override;
  int64_t Seek(const int64_t Offset, TSeekOrigin Origin) const override;
  void SaveToStream(TStream *Stream);
  void SaveToFile(UnicodeString AFileName);

  void Clear();
  void LoadFromStream(TStream *Stream);
  __removed void LoadFromFile(UnicodeString AFileName);
  int64_t GetSize() const override { return FSize; }
  void SetSize(const int64_t NewSize) override;
  int64_t Write(const void *Buffer, int64_t Count) override;

  void *GetMemory() const { return FMemory; }

protected:
  void SetPointer(void *Ptr, int64_t ASize);
  virtual void *Realloc(int64_t &NewCapacity);
  int64_t GetCapacity() const { return FCapacity; }

private:
  void SetCapacity(int64_t NewCapacity);

private:
  void *FMemory{nullptr};
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
  bool OpenKey(UnicodeString AKey, bool CanCreate);
  bool DeleteKey(UnicodeString AKey);
  bool DeleteValue(UnicodeString Value) const;
  bool KeyExists(UnicodeString SubKey) const;
  bool ValueExists(UnicodeString Value) const;
  bool GetDataInfo(UnicodeString ValueName, TRegDataInfo &Value) const;
  TRegDataType GetDataType(UnicodeString ValueName) const;
  DWORD GetDataSize(UnicodeString ValueName) const;
  bool ReadBool(UnicodeString Name) const;
  TDateTime ReadDateTime(UnicodeString Name) const;
  double ReadFloat(UnicodeString Name) const;
  intptr_t ReadIntPtr(UnicodeString Name) const;
  int ReadInteger(UnicodeString Name) const;
  int64_t ReadInt64(UnicodeString Name) const;
  UnicodeString ReadString(UnicodeString Name) const;
  UnicodeString ReadStringRaw(UnicodeString Name) const;
  size_t ReadBinaryData(UnicodeString Name,
    void *Buffer, size_t BufSize) const;

  void WriteBool(UnicodeString Name, bool Value);
  void WriteDateTime(UnicodeString Name, const TDateTime &Value);
  void WriteFloat(UnicodeString Name, double Value);
  void WriteString(UnicodeString Name, UnicodeString Value);
  void WriteStringRaw(UnicodeString Name, UnicodeString Value);
  void WriteIntPtr(UnicodeString Name, intptr_t Value);
  void WriteInteger(UnicodeString Name, int Value);
  void WriteInt64(UnicodeString Name, int64_t Value);
  void WriteBinaryData(UnicodeString Name,
    const void *Buffer, size_t BufSize);
private:
  void ChangeKey(HKEY Value, UnicodeString APath);
  HKEY GetBaseKey(bool Relative) const;
  HKEY GetKey(UnicodeString AKey) const;
  void SetCurrentKey(HKEY Value) { FCurrentKey = Value; }
  bool GetKeyInfo(TRegKeyInfo &Value) const;
  int GetData(UnicodeString Name, void *Buffer,
    intptr_t ABufSize, TRegDataType &RegData) const;
  void PutData(UnicodeString Name, const void *Buffer,
    intptr_t ABufSize, TRegDataType RegData);

public:
  void SetAccess(uint32_t Value);
  HKEY GetCurrentKey() const;
  HKEY GetRootKey() const;
  void SetRootKey(HKEY ARootKey);
  UnicodeString GetCurrentPath() const { return FCurrentPath; }

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
  int Time{0}; // Number of milliseconds since midnight
  int Date{0}; // One plus number of days since 1/1/0001
};

// FIXME
class NB_CORE_EXPORT TShortCut : public TObject
{
public:
  explicit TShortCut() noexcept = default;
  explicit TShortCut(intptr_t Value) noexcept;
  operator intptr_t() const;
  bool operator<(const TShortCut &rhs) const;
  intptr_t Compare(const TShortCut &rhs) const { return FValue - rhs.FValue; }

private:
  intptr_t FValue{0};
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

#if 0
typedef void (__closure *TInputDialogInitialize)
  (TObject *Sender, TInputDialogData *Data);
#endif // #if 0
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
  virtual ~TGlobalsIntf() noexcept = default;

  virtual HINSTANCE GetInstanceHandle() const = 0;
  virtual UnicodeString GetMsg(intptr_t Id) const = 0;
  virtual UnicodeString GetCurrDirectory() const = 0;
  virtual UnicodeString GetStrVersionNumber() const = 0;
  virtual bool InputDialog(UnicodeString ACaption,
    UnicodeString APrompt, UnicodeString &Value, UnicodeString HelpKeyword,
    TStrings *History, bool PathInput,
    TInputDialogInitializeEvent OnInitialize, bool Echo) = 0;
  virtual uint32_t MoreMessageDialog(UnicodeString AMessage,
    TStrings *MoreMessages, TQueryType Type, uint32_t Answers,
    const TMessageParams *Params) = 0;
};

class NB_CORE_EXPORT TGlobals : public TGlobalsIntf, public TObject
{
public:
  TGlobals() noexcept;
  virtual ~TGlobals() noexcept = default;

public:
  wchar_t Win32CSDVersion[128]{};
  int Win32Platform{0};
  int Win32MajorVersion{0};
  int Win32MinorVersion{0};
  int Win32BuildNumber{0};

private:
  void InitPlatformId();
};

NB_CORE_EXPORT TGlobals *GetGlobals();
NB_CORE_EXPORT void SetGlobals(TGlobals *Value);
