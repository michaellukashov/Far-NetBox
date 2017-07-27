#pragma once

#include <Global.h>
#include <Classes.hpp>

//#define EXCEPTION throw ExtException(nullptr, L"")
#define THROWOSIFFALSE(C) { if (!(C)) ::RaiseLastOSError(); }
#define SAFE_DESTROY_EX(CLASS, OBJ) { CLASS * PObj = OBJ; OBJ = nullptr; delete PObj; }
#define SAFE_DESTROY(OBJ) SAFE_DESTROY_EX(TObject, OBJ)
#define SAFE_CLOSE_HANDLE(H) { if ((H) && (H) != INVALID_HANDLE_VALUE) { HANDLE HH = (H); (H) = nullptr; if (HH != nullptr) { ::CloseHandle(HH); } } }
#define NULL_TERMINATE(S) S[LENOF(S) - 1] = L'\0'

#define SWAP(TYPE, FIRST, SECOND) \
  { TYPE __Backup = FIRST; FIRST = SECOND; SECOND = __Backup; }

#if !defined(_MSC_VER)

#define TODO(s)
#define WARNING(s)
#define PRAGMA_ERROR(s)

#else

#define STRING2(x) #x
#define STRING(x) STRING2(x)
#define FILE_LINE __FILE__ "(" STRING(__LINE__) "): "
#ifdef HIDE_TODO
#define TODO(s)
#define WARNING(s)
#else
#define TODO(s) __pragma(message (FILE_LINE /*"warning: "*/ "TODO: " s))
#define WARNING(s) __pragma(message (FILE_LINE /*"warning: "*/ "WARN: " s))
#endif
#define PRAGMA_ERROR(s) __pragma(message (FILE_LINE "error: " s))

#endif

#define PARENTDIRECTORY L".."
#define THISDIRECTORY L"."
#define ROOTDIRECTORY L"/"
#define SLASH L"/"
#define BACKSLASH L"\\"
#define QUOTE L"\'"
#define DOUBLEQUOTE L"\""

enum FileAttributesEnum
{
  faReadOnly = 0x00000001,
  faHidden = 0x00000002,
  faSysFile = 0x00000004,
  faVolumeId = 0x00000008,
  faDirectory = 0x00000010,
  faArchive = 0x00000020,
  faSymLink = 0x00000040,
  faAnyFile = 0x0000003f,
};

NB_CORE_EXPORT intptr_t __cdecl debug_printf(const wchar_t * format, ...);
NB_CORE_EXPORT intptr_t __cdecl debug_printf2(const char * format, ...);

#define NB_TEXT(T) L#T
#ifndef NDEBUG
#if defined(_MSC_VER)
#if (_MSC_VER >= 1900)
#define DEBUG_PRINTF(format, ...) nb::Sprintf(L"Plugin: [%s:%d] %s: " format L"\n", ::ExtractFilename(__FILEW__, L'\\'), __LINE__, ::MB2W(__FUNCTION__), __VA_ARGS__)
#define DEBUG_PRINTF2(format, ...) nb::Sprintf("Plugin: [%s:%d] %s: " format "\n", W2MB(::ExtractFilename(__FILEW__, '\\')), __LINE__, __FUNCTION__, __VA_ARGS__)
#else
#define DEBUG_PRINTF(format, ...) nb::Sprintf(L"Plugin: [%s:%d] %s: "NB_TEXT(format) L"\n", ::ExtractFilename(__FILEW__, L'\\'), __LINE__, ::MB2W(__FUNCTION__), __VA_ARGS__)
#define DEBUG_PRINTF2(format, ...) nb::Sprintf("Plugin: [%s:%d] %s: "format "\n", W2MB(::ExtractFilename(__FILEW__, '\\')), __LINE__, __FUNCTION__, __VA_ARGS__)
#endif
#else
#define DEBUG_PRINTF(format, ...) nb::Sprintf(L"Plugin: [%s:%d] %s: " format L"\n", ::ExtractFilename(MB2W(__FILE__), L'\\'), __LINE__, ::MB2W(__FUNCTION__), __VA_ARGS__)
#define DEBUG_PRINTF2(format, ...) nb::Sprintf("Plugin: [%s:%d] %s: " format "\n", W2MB(::ExtractFilename(MB2W(__FILE__), '\\')), __LINE__, __FUNCTION__, __VA_ARGS__)
#endif
#else
#define DEBUG_PRINTF(format, ...)
#define DEBUG_PRINTF2(format, ...)
#endif

NB_CORE_EXPORT UnicodeString MB2W(const char * src, const UINT cp = CP_ACP);
NB_CORE_EXPORT AnsiString W2MB(const wchar_t * src, const UINT cp = CP_ACP);

typedef int TDayTable[12];
extern const TDayTable MonthDays[];

class NB_CORE_EXPORT Exception : public std::runtime_error
{
CUSTOM_MEM_ALLOCATION_IMPL
public:
  static inline bool classof(const Exception * Obj) { return Obj->is(OBJECT_CLASS_Exception); }
  virtual bool is(TObjectClassId Kind) const { return (Kind == FKind); }
public:
  explicit Exception(TObjectClassId Kind, const wchar_t * Msg);
  explicit Exception(const wchar_t * Msg);
  explicit Exception(TObjectClassId Kind, UnicodeString Msg);
  explicit Exception(UnicodeString Msg);
  explicit Exception(TObjectClassId Kind, Exception * E);
  explicit Exception(TObjectClassId Kind, std::exception * E);
  explicit Exception(TObjectClassId Kind, UnicodeString Msg, intptr_t AHelpContext);
  explicit Exception(TObjectClassId Kind, Exception * E, intptr_t Ident);
  explicit Exception(TObjectClassId Kind, intptr_t Ident);
  ~Exception() {}

protected:
  // UnicodeString FHelpKeyword;
private:
  TObjectClassId FKind;
public:
  UnicodeString Message;
};

class NB_CORE_EXPORT EAbort : public Exception
{
public:
  static inline bool classof(const Exception * Obj) { return Obj->is(OBJECT_CLASS_EAbort); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_EAbort) || Exception::is(Kind); }
public:
  explicit EAbort(UnicodeString what) : Exception(OBJECT_CLASS_EAbort, what)
  {
  }
  explicit EAbort(TObjectClassId Kind, UnicodeString what) : Exception(Kind, what)
  {
  }
};

class NB_CORE_EXPORT EAccessViolation : public Exception
{
public:
  static inline bool classof(const Exception * Obj) { return Obj->is(OBJECT_CLASS_EAccessViolation); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_EAccessViolation) || Exception::is(Kind); }
public:
  explicit EAccessViolation(UnicodeString what) : Exception(OBJECT_CLASS_EAccessViolation, what)
  {
  }
};

class NB_CORE_EXPORT EFileNotFoundError : public Exception
{
public:
  static inline bool classof(const Exception * Obj) { return Obj->is(OBJECT_CLASS_EFileNotFoundError); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_EFileNotFoundError) || Exception::is(Kind); }
public:
  EFileNotFoundError() : Exception(OBJECT_CLASS_EFileNotFoundError, L"")
  {
  }
};

class NB_CORE_EXPORT EOSError : public Exception
{
public:
  static inline bool classof(const Exception * Obj) { return Obj->is(OBJECT_CLASS_EOSError); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_EOSError) || Exception::is(Kind); }
public:
  explicit EOSError(UnicodeString Msg, DWORD code) :
    Exception(OBJECT_CLASS_EOSError, Msg),
    ErrorCode(code)
  {
  }
  DWORD ErrorCode;
};

class NB_CORE_EXPORT EInvalidOperation : public Exception
{
public:
  static inline bool classof(const Exception * Obj) { return Obj->is(OBJECT_CLASS_EInvalidOperation); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_EInvalidOperation) || Exception::is(Kind); }
public:
  explicit EInvalidOperation(UnicodeString Msg) :
    Exception(OBJECT_CLASS_EInvalidOperation, Msg)
  {
  }
};

extern int RandSeed;
extern int random(int range);
extern void Randomize();

NB_CORE_EXPORT void RaiseLastOSError(DWORD LastError = 0);
//NB_CORE_EXPORT void ShowExtendedException(Exception * E);
NB_CORE_EXPORT bool AppendExceptionStackTraceAndForget(TStrings *& MoreMessages);

namespace Sysutils {

struct NB_CORE_EXPORT TFormatSettings : public TObject
{
public:
  explicit TFormatSettings(int /*LCID*/);
  static TFormatSettings Create(int LCID ) { return TFormatSettings(LCID); }
  uint8_t CurrencyFormat;
  uint8_t NegCurrFormat;
  wchar_t ThousandSeparator;
  wchar_t DecimalSeparator;
  uint8_t CurrencyDecimals;
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
  uint16_t TwoDigitYearCenturyWindow;
};

NB_CORE_EXPORT void GetLocaleFormatSettings(int LCID, TFormatSettings & FormatSettings);

NB_CORE_EXPORT UnicodeString ExtractShortPathName(UnicodeString APath);
NB_CORE_EXPORT UnicodeString ExtractDirectory(UnicodeString APath, wchar_t Delimiter = L'/');
NB_CORE_EXPORT UnicodeString ExtractFilename(UnicodeString APath, wchar_t Delimiter = L'/');
NB_CORE_EXPORT UnicodeString ExtractFileExtension(UnicodeString APath, wchar_t Delimiter = L'/');
NB_CORE_EXPORT UnicodeString ChangeFileExtension(UnicodeString APath, UnicodeString Ext, wchar_t Delimiter = L'/');

NB_CORE_EXPORT UnicodeString IncludeTrailingBackslash(UnicodeString Str);
NB_CORE_EXPORT UnicodeString ExcludeTrailingBackslash(UnicodeString Str);
NB_CORE_EXPORT UnicodeString ExtractFileDir(UnicodeString Str);
NB_CORE_EXPORT UnicodeString ExtractFilePath(UnicodeString Str);
NB_CORE_EXPORT UnicodeString GetCurrentDir();

NB_CORE_EXPORT UnicodeString IncludeTrailingPathDelimiter(UnicodeString Str);

NB_CORE_EXPORT UnicodeString StrToHex(UnicodeString Str, bool UpperCase = true, wchar_t Separator = L'\0');
NB_CORE_EXPORT UnicodeString HexToStr(UnicodeString Hex);
NB_CORE_EXPORT uintptr_t HexToInt(UnicodeString Hex, uintptr_t MinChars = 0);
NB_CORE_EXPORT UnicodeString IntToHex(uintptr_t Int, uintptr_t MinChars = 0);
NB_CORE_EXPORT char HexToChar(UnicodeString Hex, uintptr_t MinChars = 0);

NB_CORE_EXPORT UnicodeString ReplaceStrAll(UnicodeString Str, UnicodeString What, UnicodeString ByWhat);
NB_CORE_EXPORT UnicodeString SysErrorMessage(intptr_t ErrorCode);

NB_CORE_EXPORT bool TryStrToDateTime(UnicodeString StrValue, TDateTime & Value, TFormatSettings & FormatSettings);
NB_CORE_EXPORT UnicodeString DateTimeToStr(UnicodeString & Result, UnicodeString Format,
  const TDateTime & DateTime);
NB_CORE_EXPORT UnicodeString DateTimeToString(const TDateTime & DateTime);
NB_CORE_EXPORT uint32_t DayOfWeek(const TDateTime & DateTime);

NB_CORE_EXPORT TDateTime Date();
NB_CORE_EXPORT void DecodeDate(const TDateTime & DateTime, uint16_t & Year,
  uint16_t & Month, uint16_t & Day);
NB_CORE_EXPORT void DecodeTime(const TDateTime & DateTime, uint16_t & Hour,
  uint16_t & Min, uint16_t & Sec, uint16_t & MSec);

NB_CORE_EXPORT UnicodeString FormatDateTime(UnicodeString Fmt, const TDateTime & ADateTime);
NB_CORE_EXPORT TDateTime SystemTimeToDateTime(const SYSTEMTIME & SystemTime);

NB_CORE_EXPORT TDateTime EncodeDate(int Year, int Month, int Day);
NB_CORE_EXPORT TDateTime EncodeTime(uint32_t Hour, uint32_t Min, uint32_t Sec, uint32_t MSec);

NB_CORE_EXPORT UnicodeString Trim(UnicodeString Str);
NB_CORE_EXPORT UnicodeString TrimLeft(UnicodeString Str);
NB_CORE_EXPORT UnicodeString TrimRight(UnicodeString Str);
NB_CORE_EXPORT UnicodeString UpperCase(UnicodeString Str);
NB_CORE_EXPORT UnicodeString LowerCase(UnicodeString Str);
NB_CORE_EXPORT wchar_t UpCase(const wchar_t Ch);
NB_CORE_EXPORT wchar_t LowCase(const wchar_t Ch);
NB_CORE_EXPORT UnicodeString AnsiReplaceStr(UnicodeString Str, UnicodeString From, UnicodeString To);
NB_CORE_EXPORT intptr_t AnsiPos(UnicodeString Str, wchar_t Ch);
NB_CORE_EXPORT intptr_t Pos(UnicodeString Str, UnicodeString Substr);
NB_CORE_EXPORT UnicodeString StringReplaceAll(UnicodeString Str, UnicodeString From, UnicodeString To);
NB_CORE_EXPORT bool IsDelimiter(UnicodeString Delimiters, UnicodeString Str, intptr_t AIndex);
NB_CORE_EXPORT intptr_t FirstDelimiter(UnicodeString Delimiters, UnicodeString Str);
NB_CORE_EXPORT intptr_t LastDelimiter(UnicodeString Delimiters, UnicodeString Str);

NB_CORE_EXPORT intptr_t CompareText(UnicodeString Str1, UnicodeString Str2);
NB_CORE_EXPORT intptr_t AnsiCompare(UnicodeString Str1, UnicodeString Str2);
NB_CORE_EXPORT intptr_t AnsiCompareStr(UnicodeString Str1, UnicodeString Str2);
NB_CORE_EXPORT bool AnsiSameText(UnicodeString Str1, UnicodeString Str2);
NB_CORE_EXPORT bool SameText(UnicodeString Str1, UnicodeString Str2);
NB_CORE_EXPORT intptr_t AnsiCompareText(UnicodeString Str1, UnicodeString Str2);
NB_CORE_EXPORT intptr_t AnsiCompareIC(UnicodeString Str1, UnicodeString Str2);
NB_CORE_EXPORT bool AnsiSameStr(UnicodeString Str1, UnicodeString Str2);
NB_CORE_EXPORT bool AnsiContainsText(UnicodeString Str1, UnicodeString Str2);
NB_CORE_EXPORT bool ContainsStr(const AnsiString & Str1, const AnsiString & Str2);
NB_CORE_EXPORT bool ContainsText(UnicodeString Str1, UnicodeString Str2);
NB_CORE_EXPORT UnicodeString RightStr(UnicodeString Str, intptr_t ACount);
NB_CORE_EXPORT intptr_t PosEx(UnicodeString SubStr, UnicodeString Str, intptr_t Offset = 1);

NB_CORE_EXPORT UnicodeString UTF8ToString(const RawByteString & Str);
NB_CORE_EXPORT UnicodeString UTF8ToString(const char * Str, intptr_t Len);

NB_CORE_EXPORT int StringCmp(const wchar_t * S1, const wchar_t * S2);
NB_CORE_EXPORT int StringCmpI(const wchar_t * S1, const wchar_t * S2);

NB_CORE_EXPORT UnicodeString IntToStr(intptr_t Value);
NB_CORE_EXPORT UnicodeString Int64ToStr(int64_t Value);
NB_CORE_EXPORT intptr_t StrToInt(UnicodeString Value);
NB_CORE_EXPORT int64_t ToInt(UnicodeString Value);
NB_CORE_EXPORT intptr_t StrToIntDef(UnicodeString Value, intptr_t DefVal);
NB_CORE_EXPORT int64_t StrToInt64(UnicodeString Value);
NB_CORE_EXPORT int64_t StrToInt64Def(UnicodeString Value, int64_t DefVal);
NB_CORE_EXPORT bool TryStrToInt(UnicodeString StrValue, int64_t & Value);
bool TryStrToInt64(UnicodeString StrValue, int64_t & Value);

NB_CORE_EXPORT double StrToFloat(UnicodeString Value);
NB_CORE_EXPORT double StrToFloatDef(UnicodeString Value, double DefVal);
NB_CORE_EXPORT UnicodeString FormatFloat(UnicodeString Format, double Value);
NB_CORE_EXPORT bool IsZero(double Value);

NB_CORE_EXPORT TTimeStamp DateTimeToTimeStamp(const TDateTime & DateTime);

NB_CORE_EXPORT int64_t FileRead(HANDLE AHandle, void * Buffer, int64_t Count);
NB_CORE_EXPORT int64_t FileWrite(HANDLE AHandle, const void * Buffer, int64_t Count);
NB_CORE_EXPORT int64_t FileSeek(HANDLE AHandle, int64_t Offset, DWORD Origin);

NB_CORE_EXPORT bool FileExists(UnicodeString AFileName);
NB_CORE_EXPORT bool RenameFile(UnicodeString From, UnicodeString To);
NB_CORE_EXPORT bool DirectoryExists(UnicodeString ADir);
NB_CORE_EXPORT UnicodeString FileSearch(UnicodeString AFileName, UnicodeString DirectoryList);
NB_CORE_EXPORT void FileAge(UnicodeString AFileName, TDateTime & ATimestamp);

NB_CORE_EXPORT DWORD FileGetAttr(UnicodeString AFileName, bool FollowLink = true);
NB_CORE_EXPORT bool FileSetAttr(UnicodeString AFileName, DWORD LocalFileAttrs);

NB_CORE_EXPORT bool ForceDirectories(UnicodeString ADir);
NB_CORE_EXPORT bool RemoveFile(UnicodeString AFileName);
NB_CORE_EXPORT bool CreateDir(UnicodeString ADir, LPSECURITY_ATTRIBUTES SecurityAttributes = nullptr);
NB_CORE_EXPORT bool RemoveDir(UnicodeString ADir);

NB_CORE_EXPORT UnicodeString Format(const wchar_t * Format, ...);
NB_CORE_EXPORT UnicodeString FormatV(const wchar_t * Format, va_list Args);
NB_CORE_EXPORT AnsiString FormatA(const char * Format, ...);
NB_CORE_EXPORT AnsiString FormatA(const char * Format, va_list Args);
NB_CORE_EXPORT UnicodeString FmtLoadStr(intptr_t Id, ...);

NB_CORE_EXPORT UnicodeString WrapText(UnicodeString Line, intptr_t MaxWidth = 40);

NB_CORE_EXPORT UnicodeString TranslateExceptionMessage(Exception * E);

NB_CORE_EXPORT void AppendWChar(UnicodeString & Str, const wchar_t Ch);
NB_CORE_EXPORT void AppendChar(std::string & Str, const char Ch);

NB_CORE_EXPORT void AppendPathDelimiterW(UnicodeString & Str);

NB_CORE_EXPORT UnicodeString ExpandEnvVars(UnicodeString Str);

NB_CORE_EXPORT UnicodeString StringOfChar(const wchar_t Ch, intptr_t Len);

NB_CORE_EXPORT UnicodeString ChangeFileExt(UnicodeString AFileName, UnicodeString AExt,
  wchar_t Delimiter = L'/');
NB_CORE_EXPORT UnicodeString ExtractFileExt(UnicodeString AFileName);
NB_CORE_EXPORT UnicodeString ExpandUNCFileName(UnicodeString AFileName);

typedef WIN32_FIND_DATA TWin32FindData;
typedef UnicodeString TFileName;

struct NB_CORE_EXPORT TSystemTime
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

struct NB_CORE_EXPORT TFileTime
{
  Integer LowTime;
  Integer HighTime;
};

struct NB_CORE_EXPORT TSearchRec : public TObject
{
NB_DISABLE_COPY(TSearchRec)
public:
  TSearchRec() :
    Time(0),
    Size(0),
    Attr(0),
    ExcludeAttr(0),
    FindHandle(INVALID_HANDLE_VALUE)
  {
    ClearStruct(FindData);
  }
  Integer Time;
  Int64 Size;
  Integer Attr;
  TFileName Name;
  Integer ExcludeAttr;
  THandle FindHandle;
  TWin32FindData FindData;
};

NB_CORE_EXPORT void InitPlatformId();
NB_CORE_EXPORT bool Win32Check(bool RetVal);

class NB_CORE_EXPORT EConvertError : public Exception
{
public:
  explicit EConvertError(UnicodeString Msg) :
    Exception(OBJECT_CLASS_EConvertError, Msg)
  {
  }
};

NB_CORE_EXPORT UnicodeString UnixExcludeLeadingBackslash(UnicodeString APath);

NB_CORE_EXPORT TDateTime IncYear(const TDateTime & AValue, const Int64 ANumberOfYears = 1);
NB_CORE_EXPORT TDateTime IncMonth(const TDateTime & AValue, const Int64 NumberOfMonths = 1);
NB_CORE_EXPORT TDateTime IncWeek(const TDateTime & AValue, const Int64 ANumberOfWeeks = 1);
NB_CORE_EXPORT TDateTime IncDay(const TDateTime & AValue, const Int64 ANumberOfDays = 1);
NB_CORE_EXPORT TDateTime IncHour(const TDateTime & AValue, const Int64 ANumberOfHours = 1);
NB_CORE_EXPORT TDateTime IncMinute(const TDateTime & AValue, const Int64 ANumberOfMinutes = 1);
NB_CORE_EXPORT TDateTime IncSecond(const TDateTime & AValue, const Int64 ANumberOfSeconds = 1);
NB_CORE_EXPORT TDateTime IncMilliSecond(const TDateTime & AValue, const Int64 ANumberOfMilliSeconds = 1);

NB_CORE_EXPORT Boolean IsLeapYear(Word Year);

NB_CORE_EXPORT UnicodeString StripHotkey(UnicodeString AText);
NB_CORE_EXPORT bool StartsText(UnicodeString ASubText, UnicodeString AText);

struct NB_CORE_EXPORT TVersionInfo
{
  DWORD Major;
  DWORD Minor;
  DWORD Revision;
  DWORD Build;
};

#define MAKEVERSIONNUMBER(major, minor, revision) ( ((major)<<16) | ((minor)<<8) | (revision))
NB_CORE_EXPORT uintptr_t StrToVersionNumber(UnicodeString VersionMumberStr);
NB_CORE_EXPORT UnicodeString VersionNumberToStr(uintptr_t VersionNumber);
NB_CORE_EXPORT uintptr_t inline GetVersionNumber219() { return MAKEVERSIONNUMBER(2,1,9); }
NB_CORE_EXPORT uintptr_t inline GetVersionNumber2110() { return MAKEVERSIONNUMBER(2,1,10); }
NB_CORE_EXPORT uintptr_t inline GetVersionNumber2121() { return MAKEVERSIONNUMBER(2,1,21); }
NB_CORE_EXPORT uintptr_t inline GetCurrentVersionNumber() { return StrToVersionNumber(GetGlobals()->GetStrVersionNumber()); }

#if defined(__MINGW32__) && (__MINGW_GCC_VERSION < 50100)
typedef struct _TIME_DYNAMIC_ZONE_INFORMATION
{
  LONG       Bias;
  WCHAR      StandardName[32];
  SYSTEMTIME StandardDate;
  LONG       StandardBias;
  WCHAR      DaylightName[32];
  SYSTEMTIME DaylightDate;
  LONG       DaylightBias;
  WCHAR      TimeZoneKeyName[128];
  BOOLEAN    DynamicDaylightTimeDisabled;
} DYNAMIC_TIME_ZONE_INFORMATION, *PDYNAMIC_TIME_ZONE_INFORMATION;
#endif

class NB_CORE_EXPORT ScopeExit
{
public:
  explicit ScopeExit(const std::function<void()> & f) : m_f(f) {}
  ~ScopeExit() { m_f(); }

private:
  std::function<void()> m_f;
};

#define DETAIL_CONCATENATE_IMPL(s1, s2) s1 ## s2
#define CONCATENATE(s1, s2) DETAIL_CONCATENATE_IMPL(s1, s2)

#define ANONYMOUS_VARIABLE(str) CONCATENATE(str, __LINE__)

#define SCOPED_ACTION(RAII_type) \
const RAII_type ANONYMOUS_VARIABLE(scoped_object_)

//#define STR(x) #x
#define WSTR(x) L###x

namespace detail
{
  template<typename F>
  class scope_guard
  {
  public:
    explicit scope_guard(F && f) : m_f(std::move(f)) {}
    ~scope_guard() { m_f(); }

  private:
    scope_guard & operator=(const scope_guard &);
    const F m_f;
  };

  class make_scope_guard
  {
  public:
    template<typename F>
    scope_guard<F> operator<<(F && f) { return scope_guard<F>(std::move(f)); }
  };

} // namespace detail

#define SCOPE_EXIT \
  const auto ANONYMOUS_VARIABLE(scope_exit_guard) = detail::make_scope_guard() << [&]() /* lambda body here */

class NullFunc
{
public:
  NullFunc(const std::function<void()> & f) { (void)(f); }
  ~NullFunc() { }
};

#define try__catch
#define try__finally

#define __finally \
  std::function<void()> CONCATENATE(null_func_, __LINE__); \
  NullFunc ANONYMOUS_VARIABLE(null_) = CONCATENATE(null_func_, __LINE__) = []() /* lambda body here */

#if (defined _MSC_VER && _MSC_VER > 1900)

#define NONCOPYABLE(Type) \
Type(const Type&) = delete; \
Type& operator=(const Type&) = delete;

#define MOVABLE(Type) \
Type(Type&&) = default; \
Type& operator=(Type&&) = default;

template<typename T, T Default = T{}>
class movable
{
public:
  movable(T Value): m_Value(Value){}
  auto& operator=(T Value) { m_Value = Value; return *this; }

  movable(const movable& rhs) { *this = rhs; }
  auto& operator=(const movable& rhs) { m_Value = rhs.m_Value; return *this; }

  movable(movable&& rhs) noexcept { *this = std::move(rhs); }
  auto& operator=(movable&& rhs) noexcept { m_Value = rhs.m_Value; rhs.m_Value = Default; return *this; }

  auto& operator*() const { return m_Value; }
  auto& operator*() { return m_Value; }

private:
  T m_Value;
};

namespace detail
{
  struct nop_deleter { void operator()(void *) const {} };
}

template<class T>
using movable_ptr = std::unique_ptr<T, detail::nop_deleter>;

namespace scope_exit
{
  class uncaught_exceptions_counter
  {
  public:
    bool is_new() const noexcept { return std::uncaught_exceptions() > m_Count; }
    int m_Count{ std::uncaught_exceptions() }; // int... "a camel is a horse designed by a committee" :(
  };

  enum class scope_type
  {
    exit,
    fail,

    success
  };

  template<typename F, scope_type Type>
  class scope_guard
  {
  public:
    NONCOPYABLE(scope_guard);
    MOVABLE(scope_guard);

    explicit scope_guard(F&& f): m_f(std::forward<F>(f)) {}

    ~scope_guard() noexcept(Type == scope_type::fail)
    {
      if (*m_Active && (Type == scope_type::exit || (Type == scope_type::fail) == m_Ec.is_new()))
        m_f();
    }

  private:
    F m_f;
    movable<bool> m_Active{ true };
    uncaught_exceptions_counter m_Ec;
  };

  template<scope_type Type>
  class make_scope_guard
  {
  public:
    template<typename F>
    auto operator<<(F&& f) { return scope_guard<F, Type>(std::forward<F>(f)); }
  };
}

#define DETAIL_SCOPE_IMPL(type) \
const auto ANONYMOUS_VARIABLE(scope_##type##_guard) = scope_exit::make_scope_guard<scope_exit::scope_type::type>() << [&]() /* lambda body here */

#undef SCOPE_EXIT
#define SCOPE_EXIT DETAIL_SCOPE_IMPL(exit)
#define SCOPE_FAIL DETAIL_SCOPE_IMPL(fail) noexcept
#define SCOPE_SUCCESS DETAIL_SCOPE_IMPL(success)

#endif // #if (defined _MSC_VER && _MSC_VER > 1900)

class NB_CORE_EXPORT TPath : public TObject
{
public:
  static UnicodeString Combine(UnicodeString APath, UnicodeString FileName);
};

} // namespace Sysutils

using namespace Sysutils;

namespace base {

DWORD FindFirst(UnicodeString AFileName, DWORD LocalFileAttrs, TSearchRec & Rec);
DWORD FindNext(TSearchRec & Rec);
DWORD FindClose(TSearchRec & Rec);

} // namespace base
