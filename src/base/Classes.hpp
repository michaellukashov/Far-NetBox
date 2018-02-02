#pragma once

#include <headers.hpp>

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
#include <ObjIDs.h>
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

typedef HANDLE THandle;
typedef DWORD TThreadID;

class Exception;

NB_CORE_EXPORT extern const intptr_t MonthsPerYear;
NB_CORE_EXPORT extern const intptr_t DaysPerWeek;
NB_CORE_EXPORT extern const intptr_t MinsPerHour;
NB_CORE_EXPORT extern const intptr_t MinsPerDay;
NB_CORE_EXPORT extern const intptr_t SecsPerMin;
NB_CORE_EXPORT extern const intptr_t SecsPerHour;
NB_CORE_EXPORT extern const intptr_t HoursPerDay;
NB_CORE_EXPORT extern const intptr_t SecsPerDay;
NB_CORE_EXPORT extern const intptr_t MSecsPerDay;
NB_CORE_EXPORT extern const intptr_t MSecsPerSec;
NB_CORE_EXPORT extern const intptr_t OneSecond;
NB_CORE_EXPORT extern const intptr_t DateDelta;
NB_CORE_EXPORT extern const intptr_t UnixDateDelta;

class TObject;

typedef nb::FastDelegate0<void> TThreadMethod;
typedef nb::FastDelegate1<void, TObject * /*Sender*/> TNotifyEvent;

NB_CORE_EXPORT void Abort();
NB_CORE_EXPORT void Error(intptr_t Id, intptr_t ErrorId);
NB_CORE_EXPORT void ThrowNotImplemented(intptr_t ErrorId);

class NB_CORE_EXPORT TObject
{
  CUSTOM_MEM_ALLOCATION_IMPL
public:
  static inline bool classof(const TObject * /*Obj*/) { return true; }
  virtual bool is(TObjectClassId Kind) const { return Kind == FKind; }
public:
  TObject() : FKind(OBJECT_CLASS_TObject) {}
  explicit TObject(TObjectClassId Kind) : FKind(Kind) {}
  virtual ~TObject() {}
  virtual void Changed() {}
private:
  TObjectClassId FKind;
};

inline TObject *as_object(void *p) { return static_cast<TObject *>(p); }
inline const TObject *as_object(const void *p) { return static_cast<const TObject *>(p); }
template<class T> inline T *get_as(void *p) { return dyn_cast<T>(as_object(p)); }
template<class T> inline const T *get_as(const void *p) { return dyn_cast<T>(as_object(p)); }
template <class T>
inline TObject *ToObj(const T &a) { return reinterpret_cast<TObject *>(static_cast<size_t>(a)); }

struct TPoint
{
  int x;
  int y;
  TPoint() :
    x(0),
    y(0)
  {
  }
  TPoint(int ax, int ay) :
    x(ax),
    y(ay)
  {
  }
};

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

class NB_CORE_EXPORT TPersistent : public TObject
{
public:
  static inline bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TPersistent); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TPersistent) || TObject::is(Kind); }
public:
  TPersistent() : TObject(OBJECT_CLASS_TPersistent) {}
  explicit TPersistent(TObjectClassId Kind);
  virtual ~TPersistent();
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

class NB_CORE_EXPORT TList : public TPersistent
{
public:
  static inline bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TList); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TList) || TPersistent::is(Kind); }
public:
  TList();
  explicit TList(TObjectClassId Kind);
  virtual ~TList();

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

private:
  rde::vector<void *> FList;
};

class NB_CORE_EXPORT TObjectList : public TList
{
public:
  static inline bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TObjectList); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TObjectList) || TList::is(Kind); }
public:
  explicit TObjectList(TObjectClassId Kind = OBJECT_CLASS_TObjectList);
  virtual ~TObjectList();

  template<class T>
  T *GetAs(intptr_t Index) const { return dyn_cast<T>(GetObj(Index)); }
  TObject *operator[](intptr_t Index) const;
  TObject *GetObj(intptr_t Index) const;
  bool GetOwnsObjects() const { return FOwnsObjects; }
  void SetOwnsObjects(bool Value) { FOwnsObjects = Value; }
  virtual void Notify(void *Ptr, TListNotification Action) override;

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

class NB_CORE_EXPORT TStrings : public TObjectList
{
public:
  static inline bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TStrings); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TStrings) || TObjectList::is(Kind); }
public:
  TStrings();
  explicit TStrings(TObjectClassId Kind);
  virtual ~TStrings();
  intptr_t Add(UnicodeString S, TObject *AObject = nullptr);
  virtual UnicodeString GetTextStr() const;
  virtual void SetTextStr(UnicodeString Text);
  virtual void BeginUpdate();
  virtual void EndUpdate();
  virtual void SetUpdateState(bool Updating);
  virtual intptr_t AddObject(UnicodeString S, TObject *AObject);
  virtual void InsertObject(intptr_t Index, UnicodeString Key, TObject *AObject);
  bool Equals(const TStrings *Value) const;
  virtual void Move(intptr_t CurIndex, intptr_t NewIndex) override;
  virtual intptr_t IndexOf(UnicodeString S) const;
  virtual intptr_t IndexOfName(UnicodeString Name) const;
  UnicodeString ExtractName(UnicodeString S) const;
  void AddStrings(const TStrings *Strings);
  void Append(UnicodeString Value);
  virtual void Insert(intptr_t Index, UnicodeString AString, TObject *AObject = nullptr) = 0;
  void SaveToStream(TStream *Stream) const;
  wchar_t GetDelimiter() const { return FDelimiter; }
  void SetDelimiter(wchar_t Value) { FDelimiter = Value; }
  wchar_t GetQuoteChar() const { return FQuoteChar; }
  void SetQuoteChar(wchar_t Value) { FQuoteChar = Value; }
  UnicodeString GetDelimitedText() const;
  void SetDelimitedText(UnicodeString Value);
  virtual intptr_t CompareStrings(const UnicodeString S1, const UnicodeString S2) const;
  intptr_t GetUpdateCount() const { return FUpdateCount; }
  virtual void Assign(const TPersistent *Source) override;
  virtual intptr_t GetCount() const = 0;

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
  TDuplicatesEnum FDuplicates;
  mutable wchar_t FDelimiter;
  mutable wchar_t FQuoteChar;
  intptr_t FUpdateCount;
};

class TStringList;
typedef intptr_t (TStringListSortCompare)(TStringList *List, intptr_t Index1, intptr_t Index2);

class NB_CORE_EXPORT TStringList : public TStrings
{
  friend intptr_t StringListCompareStrings(TStringList *List, intptr_t Index1, intptr_t Index2);
public:
  static inline bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TStringList); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TStringList) || TStrings::is(Kind); }
public:
  explicit TStringList(TObjectClassId Kind = OBJECT_CLASS_TStringList);
  virtual ~TStringList();

  intptr_t Add(UnicodeString S);
  virtual intptr_t AddObject(UnicodeString S, TObject *AObject) override;
  void LoadFromFile(UnicodeString AFileName);
  TNotifyEvent GetOnChange() const { return FOnChange; }
  void SetOnChange(TNotifyEvent OnChange) { FOnChange = OnChange; }
  TNotifyEvent GetOnChanging() const { return FOnChanging; }
  void SetOnChanging(TNotifyEvent OnChanging) { FOnChanging = OnChanging; }
  void InsertItem(intptr_t Index, UnicodeString S, TObject *AObject);
  void QuickSort(intptr_t L, intptr_t R, TStringListSortCompare SCompare);

  virtual void Assign(const TPersistent *Source) override;
  virtual bool Find(UnicodeString S, intptr_t &Index) const;
  virtual intptr_t IndexOf(UnicodeString S) const override;
  virtual void Delete(intptr_t Index) override;
  virtual void InsertObject(intptr_t Index, UnicodeString Key, TObject *AObject) override;
  virtual void Sort() override;
  virtual void CustomSort(TStringListSortCompare ACompareFunc);

  virtual void SetUpdateState(bool Updating) override;
  virtual void Changing();
  virtual void Changed() override;
  virtual void Insert(intptr_t Index, UnicodeString S, TObject *AObject = nullptr) override;
  virtual intptr_t CompareStrings(UnicodeString S1, UnicodeString S2) const override;
  virtual intptr_t GetCount() const override;

public:
  virtual void SetObj(intptr_t Index, TObject *AObject) override;
  virtual bool GetSorted() const override { return FSorted; }
  virtual void SetSorted(bool Value) override;
  virtual bool GetCaseSensitive() const override { return FCaseSensitive; }
  virtual void SetCaseSensitive(bool Value) override;
  virtual const UnicodeString &GetStringRef(intptr_t Index) const override;
  virtual const UnicodeString &GetString(intptr_t Index) const override;
  virtual UnicodeString GetString(intptr_t Index) override;
  virtual void SetString(intptr_t Index, const UnicodeString S) override;

private:
  TNotifyEvent FOnChange;
  TNotifyEvent FOnChanging;
  rde::vector<UnicodeString> FStrings;
  bool FSorted;
  bool FCaseSensitive;

private:
  void ExchangeItems(intptr_t Index1, intptr_t Index2);

private:
  TStringList(const TStringList &);
  TStringList &operator=(const TStringList &);
};

/// TDateTime: number of days since 12/30/1899
class NB_CORE_EXPORT TDateTime : public TObject
{
public:
  TDateTime() :
    FValue(0.0)
  {}
  explicit TDateTime(double Value) :
    FValue(Value)
  {
  }
  explicit TDateTime(uint16_t Hour,
    uint16_t Min, uint16_t Sec, uint16_t MSec = 0);
  TDateTime(const TDateTime &rhs) :
    FValue(rhs.FValue)
  {
  }
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
  UnicodeString FormatString(wchar_t *fmt) const;
  void DecodeDate(uint16_t &Y, uint16_t &M, uint16_t &D) const;
  void DecodeTime(uint16_t &H, uint16_t &N, uint16_t &S, uint16_t &MS) const;
private:
  double FValue;
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
  TSHFileInfo();
  virtual ~TSHFileInfo();

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
  TStream();
  virtual ~TStream();
  virtual int64_t Read(void *Buffer, int64_t Count) = 0;
  virtual int64_t Write(const void *Buffer, int64_t Count) = 0;
  virtual int64_t Seek(int64_t Offset, int Origin) const = 0;
  virtual int64_t Seek(const int64_t Offset, TSeekOrigin Origin) const = 0;
  void ReadBuffer(void *Buffer, int64_t Count);
  void WriteBuffer(const void *Buffer, int64_t Count);
  int64_t CopyFrom(TStream *Source, int64_t Count);

public:
  int64_t GetPosition() const;
  int64_t GetSize() const;
  virtual void SetSize(const int64_t NewSize) = 0;
  void SetPosition(const int64_t Pos);

  RWProperty<int64_t> Position{nb::bind(&TStream::GetPosition, this), nb::bind(&TStream::SetPosition, this)};
  RWProperty<int64_t> Size{nb::bind(&TStream::GetSize, this), nb::bind(&TStream::SetSize, this)};
};

class NB_CORE_EXPORT THandleStream : public TStream
{
  NB_DISABLE_COPY(THandleStream)
public:
  explicit THandleStream(HANDLE AHandle);
  virtual ~THandleStream();
  virtual int64_t Read(void *Buffer, int64_t Count) override;
  virtual int64_t Write(const void *Buffer, int64_t Count) override;
  virtual int64_t Seek(int64_t Offset, int Origin) const override;
  virtual int64_t Seek(const int64_t Offset, TSeekOrigin Origin) const override;

  HANDLE GetHandle() { return FHandle; }

protected:
  virtual void SetSize(const int64_t NewSize) override;

protected:
  HANDLE FHandle;
};

class NB_CORE_EXPORT TSafeHandleStream : public THandleStream
{
public:
  explicit TSafeHandleStream(THandle AHandle);
  virtual ~TSafeHandleStream() {}
  virtual int64_t Read(void *Buffer, int64_t Count) override;
  virtual int64_t Write(const void *Buffer, int64_t Count) override;
};

class NB_CORE_EXPORT EReadError : public std::runtime_error
{
public:
  explicit EReadError(const char *Msg) :
    std::runtime_error(Msg)
  {}
};

class NB_CORE_EXPORT EWriteError : public std::runtime_error
{
public:
  explicit EWriteError(const char *Msg) :
    std::runtime_error(Msg)
  {}
};

class NB_CORE_EXPORT TMemoryStream : public TStream
{
  NB_DISABLE_COPY(TMemoryStream)
public:
  TMemoryStream();
  virtual ~TMemoryStream();
  virtual int64_t Read(void *Buffer, int64_t Count) override;
  virtual int64_t Seek(int64_t Offset, int Origin) const override;
  virtual int64_t Seek(const int64_t Offset, TSeekOrigin Origin) const override;
  void SaveToStream(TStream *Stream);
  void SaveToFile(UnicodeString AFileName);

  void Clear();
  void LoadFromStream(TStream *Stream);
  __removed void LoadFromFile(const UnicodeString AFileName);
  int64_t GetSize() const { return FSize; }
  virtual void SetSize(const int64_t NewSize) override;
  virtual int64_t Write(const void *Buffer, int64_t Count) override;

  void *GetMemory() const { return FMemory; }

protected:
  void SetPointer(void *Ptr, int64_t Size);
  virtual void *Realloc(int64_t &NewCapacity);
  int64_t GetCapacity() const { return FCapacity; }

private:
  void SetCapacity(int64_t NewCapacity);

private:
  void *FMemory;
  int64_t FSize;
  mutable int64_t FPosition;
  int64_t FCapacity;
};

struct TRegKeyInfo
{
  DWORD NumSubKeys;
  DWORD MaxSubKeyLen;
  DWORD NumValues;
  DWORD MaxValueLen;
  DWORD MaxDataLen;
  FILETIME FileTime;
};

enum TRegDataType
{
  rdUnknown, rdString, rdExpandString, rdInteger, rdBinary
};

struct TRegDataInfo
{
  TRegDataType RegData;
  DWORD DataSize;
};

class NB_CORE_EXPORT TRegistry : public TObject
{
  NB_DISABLE_COPY(TRegistry)
public:
  TRegistry();
  ~TRegistry();
  void GetValueNames(TStrings *Names) const;
  void GetKeyNames(TStrings *Names) const;
  void CloseKey();
  bool OpenKey(UnicodeString Key, bool CanCreate);
  bool DeleteKey(UnicodeString Key);
  bool DeleteValue(UnicodeString Value) const;
  bool KeyExists(UnicodeString SubKey) const;
  bool ValueExists(UnicodeString Value) const;
  bool GetDataInfo(UnicodeString ValueName, TRegDataInfo &Value) const;
  TRegDataType GetDataType(UnicodeString ValueName) const;
  DWORD GetDataSize(UnicodeString ValueName) const;
  bool ReadBool(UnicodeString Name) const;
  TDateTime ReadDateTime(UnicodeString Name) const;
  double ReadFloat(UnicodeString Name) const;
  intptr_t ReadInteger(UnicodeString Name) const;
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
  void WriteInteger(UnicodeString Name, intptr_t Value);
  void WriteInt64(UnicodeString Name, int64_t Value);
  void WriteBinaryData(UnicodeString Name,
    const void *Buffer, size_t BufSize);
private:
  void ChangeKey(HKEY Value, UnicodeString APath);
  HKEY GetBaseKey(bool Relative) const;
  HKEY GetKey(UnicodeString Key) const;
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

private:
  HKEY FCurrentKey;
  HKEY FRootKey;
  // bool FLazyWrite;
  UnicodeString FCurrentPath;
  bool FCloseRootKey;
  mutable uint32_t FAccess;
};

struct TTimeStamp
{
  int Time; // Number of milliseconds since midnight
  int Date; // One plus number of days since 1/1/0001
};

// FIXME
class NB_CORE_EXPORT TShortCut : public TObject
{
public:
  explicit TShortCut();
  explicit TShortCut(intptr_t Value);
  operator intptr_t() const;
  bool operator<(const TShortCut &rhs) const;
  intptr_t Compare(const TShortCut &rhs) const { return FValue - rhs.FValue; }

private:
  intptr_t FValue;
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
  void *Edit;
};

#if 0
typedef void (__closure *TInputDialogInitialize)
(TObject *Sender, TInputDialogData *Data);
#endif // #if 0
typedef nb::FastDelegate2<void,
        TObject * /*Sender*/, TInputDialogData * /*Data*/> TInputDialogInitializeEvent;

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
  virtual ~TGlobalsIntf()
  {
  }

  virtual HINSTANCE GetInstanceHandle() const = 0;
  virtual UnicodeString GetMsg(intptr_t Id) const = 0;
  virtual UnicodeString GetCurrDirectory() const = 0;
  virtual UnicodeString GetStrVersionNumber() const = 0;
  virtual bool InputDialog(const UnicodeString ACaption,
    const UnicodeString APrompt, UnicodeString &Value, const UnicodeString HelpKeyword,
    TStrings *History, bool PathInput,
    TInputDialogInitializeEvent OnInitialize, bool Echo) = 0;
  virtual uintptr_t MoreMessageDialog(const UnicodeString Message,
    TStrings *MoreMessages, TQueryType Type, uint32_t Answers,
    const TMessageParams *Params) = 0;
};

class NB_CORE_EXPORT TGlobals : public TGlobalsIntf, public TObject
{
public:
  TGlobals();
  virtual ~TGlobals();

public:
  wchar_t Win32CSDVersion[128];
  int Win32Platform;
  int Win32MajorVersion;
  int Win32MinorVersion;
  int Win32BuildNumber;

private:
  void InitPlatformId();
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
