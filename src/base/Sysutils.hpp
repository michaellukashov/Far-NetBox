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

intptr_t __cdecl debug_printf(const wchar_t * format, ...);
intptr_t __cdecl debug_printf2(const char * format, ...);

#define NB_TEXT(T) L#T
#ifndef NDEBUG
#if defined(_MSC_VER)
#if (_MSC_VER >= 1900)
#define DEBUG_PRINTF(format, ...) ::debug_printf(L"Plugin: [%s:%d] %s: " format L"\n", ::ExtractFilename(__FILEW__, L'\\').c_str(), __LINE__, ::MB2W(__FUNCTION__).c_str(), __VA_ARGS__)
#define DEBUG_PRINTF2(format, ...) ::debug_printf2("Plugin: [%s:%d] %s: " format "\n", W2MB(::ExtractFilename(__FILEW__, '\\').c_str()).c_str(), __LINE__, __FUNCTION__, __VA_ARGS__)
#else
#define DEBUG_PRINTF(format, ...) ::debug_printf(L"Plugin: [%s:%d] %s: "NB_TEXT(format) L"\n", ::ExtractFilename(__FILEW__, L'\\').c_str(), __LINE__, ::MB2W(__FUNCTION__).c_str(), __VA_ARGS__)
#define DEBUG_PRINTF2(format, ...) ::debug_printf2("Plugin: [%s:%d] %s: "format "\n", W2MB(::ExtractFilename(__FILEW__, '\\').c_str()).c_str(), __LINE__, __FUNCTION__, __VA_ARGS__)
#endif
#else
#define DEBUG_PRINTF(format, ...) ::debug_printf(L"Plugin: [%s:%d] %s: " format L"\n", ::ExtractFilename(MB2W(__FILE__).c_str(), L'\\').c_str(), __LINE__, ::MB2W(__FUNCTION__).c_str(), ##__VA_ARGS__)
#define DEBUG_PRINTF2(format, ...) ::debug_printf2("Plugin: [%s:%d] %s: " format "\n", W2MB(::ExtractFilename(MB2W(__FILE__).c_str(), '\\').c_str()).c_str(), __LINE__, __FUNCTION__, ##__VA_ARGS__)
#endif
#else
#define DEBUG_PRINTF(format, ...)
#define DEBUG_PRINTF2(format, ...)
#endif

UnicodeString MB2W(const char * src, const UINT cp = CP_ACP);
AnsiString W2MB(const wchar_t * src, const UINT cp = CP_ACP);

typedef int TDayTable[12];
extern const TDayTable MonthDays[];

class Exception : public std::runtime_error//, public TObject
{
CUSTOM_MEM_ALLOCATION_IMPL
public:
  inline TObjectClassId GetKind() const { return FKind; }
  static bool classof(const Exception * Obj)
  {
    TObjectClassId Kind = Obj->GetKind();
    return
      Kind == OBJECT_CLASS_Exception ||
      Kind == OBJECT_CLASS_ExtException ||
      Kind == OBJECT_CLASS_EAbort ||
      Kind == OBJECT_CLASS_EAccessViolation ||
      Kind == OBJECT_CLASS_EFileNotFoundError ||
      Kind == OBJECT_CLASS_EOSError ||
      Kind == OBJECT_CLASS_EInvalidOperation ||
      Kind == OBJECT_CLASS_EFatal ||
      Kind == OBJECT_CLASS_ESshFatal ||
      Kind == OBJECT_CLASS_ESshTerminate ||
      Kind == OBJECT_CLASS_ECallbackGuardAbort ||
      Kind == OBJECT_CLASS_EFileSkipped ||
      Kind == OBJECT_CLASS_ESkipFile;
  }
public:
  explicit Exception(TObjectClassId Kind, const wchar_t * Msg);
  explicit Exception(const wchar_t * Msg);
  explicit Exception(TObjectClassId Kind, const UnicodeString & Msg);
  explicit Exception(const UnicodeString & Msg);
  explicit Exception(TObjectClassId Kind, Exception * E);
  explicit Exception(TObjectClassId Kind, std::exception * E);
  explicit Exception(TObjectClassId Kind, const UnicodeString & Msg, intptr_t AHelpContext);
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

class EAbort : public Exception
{
public:
  static inline bool classof(const Exception * Obj)
  {
    return
      Obj->GetKind() == OBJECT_CLASS_EAbort ||
      Obj->GetKind() == OBJECT_CLASS_ECallbackGuardAbort;
  }
public:
  explicit EAbort(const UnicodeString & what) : Exception(OBJECT_CLASS_EAbort, what)
  {
  }
  explicit EAbort(TObjectClassId Kind, const UnicodeString & what) : Exception(Kind, what)
  {
  }
};

class EAccessViolation : public Exception
{
public:
  static inline bool classof(const Exception * Obj)
  {
    return
      Obj->GetKind() == OBJECT_CLASS_EAccessViolation;
  }
public:
  explicit EAccessViolation(const UnicodeString & what) : Exception(OBJECT_CLASS_EAccessViolation, what)
  {
  }
};

class EFileNotFoundError : public Exception
{
public:
  static inline bool classof(const Exception * Obj)
  {
    return
      Obj->GetKind() == OBJECT_CLASS_EFileNotFoundError;
  }
public:
  EFileNotFoundError() : Exception(OBJECT_CLASS_EFileNotFoundError, L"")
  {
  }
};

class EOSError : public Exception
{
public:
  static inline bool classof(const Exception * Obj)
  {
    return
      Obj->GetKind() == OBJECT_CLASS_EOSError;
  }
public:
  explicit EOSError(const UnicodeString & Msg, DWORD code) :
    Exception(OBJECT_CLASS_EOSError, Msg),
    ErrorCode(code)
  {
  }
  DWORD ErrorCode;
};

class EInvalidOperation : public Exception
{
public:
  static inline bool classof(const Exception * Obj)
  {
    return
      Obj->GetKind() == OBJECT_CLASS_EInvalidOperation;
  }
public:
  explicit EInvalidOperation(const UnicodeString & Msg) :
    Exception(OBJECT_CLASS_EInvalidOperation, Msg)
  {
  }
};

extern int RandSeed;
extern int random(int range);
extern void Randomize();

void RaiseLastOSError(DWORD LastError = 0);
//void ShowExtendedException(Exception * E);
bool AppendExceptionStackTraceAndForget(TStrings *& MoreMessages);

namespace Sysutils {

struct TFormatSettings : public TObject
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

void GetLocaleFormatSettings(int LCID, TFormatSettings & FormatSettings);

UnicodeString ExtractShortPathName(const UnicodeString & APath);
UnicodeString ExtractDirectory(const UnicodeString & APath, wchar_t Delimiter = L'/');
UnicodeString ExtractFilename(const UnicodeString & APath, wchar_t Delimiter = L'/');
UnicodeString ExtractFileExtension(const UnicodeString & APath, wchar_t Delimiter = L'/');
UnicodeString ChangeFileExtension(const UnicodeString & APath, const UnicodeString & Ext, wchar_t Delimiter = L'/');

UnicodeString IncludeTrailingBackslash(const UnicodeString & Str);
UnicodeString ExcludeTrailingBackslash(const UnicodeString & Str);
UnicodeString ExtractFileDir(const UnicodeString & Str);
UnicodeString ExtractFilePath(const UnicodeString & Str);
UnicodeString GetCurrentDir();

UnicodeString IncludeTrailingPathDelimiter(const UnicodeString & Str);

UnicodeString StrToHex(const UnicodeString & Str, bool UpperCase = true, wchar_t Separator = L'\0');
UnicodeString HexToStr(const UnicodeString & Hex);
uintptr_t HexToInt(const UnicodeString & Hex, uintptr_t MinChars = 0);
UnicodeString IntToHex(uintptr_t Int, uintptr_t MinChars = 0);
char HexToChar(const UnicodeString & Hex, uintptr_t MinChars = 0);

UnicodeString ReplaceStrAll(const UnicodeString & Str, const UnicodeString & What, const UnicodeString & ByWhat);
UnicodeString SysErrorMessage(intptr_t ErrorCode);

bool TryStrToDateTime(const UnicodeString & StrValue, TDateTime & Value, TFormatSettings & FormatSettings);
UnicodeString DateTimeToStr(UnicodeString & Result, const UnicodeString & Format,
  const TDateTime & DateTime);
UnicodeString DateTimeToString(const TDateTime & DateTime);
uint32_t DayOfWeek(const TDateTime & DateTime);

TDateTime Date();
void DecodeDate(const TDateTime & DateTime, uint16_t & Year,
  uint16_t & Month, uint16_t & Day);
void DecodeTime(const TDateTime & DateTime, uint16_t & Hour,
  uint16_t & Min, uint16_t & Sec, uint16_t & MSec);

UnicodeString FormatDateTime(const UnicodeString & Fmt, const TDateTime & ADateTime);
TDateTime SystemTimeToDateTime(const SYSTEMTIME & SystemTime);

TDateTime EncodeDate(int Year, int Month, int Day);
TDateTime EncodeTime(uint32_t Hour, uint32_t Min, uint32_t Sec, uint32_t MSec);

UnicodeString Trim(const UnicodeString & Str);
UnicodeString TrimLeft(const UnicodeString & Str);
UnicodeString TrimRight(const UnicodeString & Str);
UnicodeString UpperCase(const UnicodeString & Str);
UnicodeString LowerCase(const UnicodeString & Str);
wchar_t UpCase(const wchar_t Ch);
wchar_t LowCase(const wchar_t Ch);
UnicodeString AnsiReplaceStr(const UnicodeString & Str, const UnicodeString & From, const UnicodeString & To);
intptr_t AnsiPos(const UnicodeString & Str, wchar_t Ch);
intptr_t Pos(const UnicodeString & Str, const UnicodeString & Substr);
UnicodeString StringReplaceAll(const UnicodeString & Str, const UnicodeString & From, const UnicodeString & To);
bool IsDelimiter(const UnicodeString & Delimiters, const UnicodeString & Str, intptr_t AIndex);
intptr_t FirstDelimiter(const UnicodeString & Delimiters, const UnicodeString & Str);
intptr_t LastDelimiter(const UnicodeString & Delimiters, const UnicodeString & Str);

intptr_t CompareText(const UnicodeString & Str1, const UnicodeString & Str2);
intptr_t AnsiCompare(const UnicodeString & Str1, const UnicodeString & Str2);
intptr_t AnsiCompareStr(const UnicodeString & Str1, const UnicodeString & Str2);
bool AnsiSameText(const UnicodeString & Str1, const UnicodeString & Str2);
bool SameText(const UnicodeString & Str1, const UnicodeString & Str2);
intptr_t AnsiCompareText(const UnicodeString & Str1, const UnicodeString & Str2);
intptr_t AnsiCompareIC(const UnicodeString & Str1, const UnicodeString & Str2);
bool AnsiSameStr(const UnicodeString & Str1, const UnicodeString & Str2);
bool AnsiContainsText(const UnicodeString & Str1, const UnicodeString & Str2);
bool ContainsStr(const AnsiString & Str1, const AnsiString & Str2);
bool ContainsText(const UnicodeString & Str1, const UnicodeString & Str2);
UnicodeString RightStr(const UnicodeString & Str, intptr_t ACount);
intptr_t PosEx(const UnicodeString & SubStr, const UnicodeString & Str, intptr_t Offset = 1);

UnicodeString UTF8ToString(const RawByteString & Str);
UnicodeString UTF8ToString(const char * Str, intptr_t Len);

int StringCmp(const wchar_t * S1, const wchar_t * S2);
int StringCmpI(const wchar_t * S1, const wchar_t * S2);

UnicodeString IntToStr(intptr_t Value);
UnicodeString Int64ToStr(int64_t Value);
intptr_t StrToInt(const UnicodeString & Value);
int64_t ToInt(const UnicodeString & Value);
intptr_t StrToIntDef(const UnicodeString & Value, intptr_t DefVal);
int64_t StrToInt64(const UnicodeString & Value);
int64_t StrToInt64Def(const UnicodeString & Value, int64_t DefVal);
bool TryStrToInt(const UnicodeString & StrValue, int64_t & Value);
bool TryStrToInt64(const UnicodeString & StrValue, int64_t & Value);

double StrToFloat(const UnicodeString & Value);
double StrToFloatDef(const UnicodeString & Value, double DefVal);
UnicodeString FormatFloat(const UnicodeString & Format, double Value);
bool IsZero(double Value);

TTimeStamp DateTimeToTimeStamp(const TDateTime & DateTime);

int64_t FileRead(HANDLE AHandle, void * Buffer, int64_t Count);
int64_t FileWrite(HANDLE AHandle, const void * Buffer, int64_t Count);
int64_t FileSeek(HANDLE AHandle, int64_t Offset, DWORD Origin);

bool FileExists(const UnicodeString & AFileName);
bool RenameFile(const UnicodeString & From, const UnicodeString & To);
bool DirectoryExists(const UnicodeString & ADir);
UnicodeString FileSearch(const UnicodeString & AFileName, const UnicodeString & DirectoryList);
void FileAge(const UnicodeString & AFileName, TDateTime & ATimestamp);

DWORD FileGetAttr(const UnicodeString & AFileName, bool FollowLink = true);
DWORD FileSetAttr(const UnicodeString & AFileName, DWORD LocalFileAttrs);

bool ForceDirectories(const UnicodeString & ADir);
bool RemoveFile(const UnicodeString & AFileName);
bool CreateDir(const UnicodeString & ADir, LPSECURITY_ATTRIBUTES SecurityAttributes = nullptr);
bool RemoveDir(const UnicodeString & ADir);

UnicodeString Format(const wchar_t * Format, ...);
UnicodeString FormatV(const wchar_t * Format, va_list Args);
AnsiString FormatA(const char * Format, ...);
AnsiString FormatA(const char * Format, va_list Args);
UnicodeString FmtLoadStr(intptr_t Id, ...);

UnicodeString WrapText(const UnicodeString & Line, intptr_t MaxWidth = 40);

UnicodeString TranslateExceptionMessage(Exception * E);

void AppendWChar(UnicodeString & Str, const wchar_t Ch);
void AppendChar(std::string & Str, const char Ch);

void AppendPathDelimiterW(UnicodeString & Str);

UnicodeString ExpandEnvVars(const UnicodeString & Str);

UnicodeString StringOfChar(const wchar_t Ch, intptr_t Len);

UnicodeString ChangeFileExt(const UnicodeString & AFileName, const UnicodeString & AExt,
  wchar_t Delimiter = L'/');
UnicodeString ExtractFileExt(const UnicodeString & AFileName);
UnicodeString ExpandUNCFileName(const UnicodeString & AFileName);

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

struct TSearchRec : public TObject
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

void InitPlatformId();
bool Win32Check(bool RetVal);

class EConvertError : public Exception
{
public:
  explicit EConvertError(const UnicodeString & Msg) :
    Exception(OBJECT_CLASS_EConvertError, Msg)
  {
  }
};

UnicodeString UnixExcludeLeadingBackslash(const UnicodeString & APath);

TDateTime IncYear(const TDateTime & AValue, const Int64 ANumberOfYears = 1);
TDateTime IncMonth(const TDateTime & AValue, const Int64 NumberOfMonths = 1);
TDateTime IncWeek(const TDateTime & AValue, const Int64 ANumberOfWeeks = 1);
TDateTime IncDay(const TDateTime & AValue, const Int64 ANumberOfDays = 1);
TDateTime IncHour(const TDateTime & AValue, const Int64 ANumberOfHours = 1);
TDateTime IncMinute(const TDateTime & AValue, const Int64 ANumberOfMinutes = 1);
TDateTime IncSecond(const TDateTime & AValue, const Int64 ANumberOfSeconds = 1);
TDateTime IncMilliSecond(const TDateTime & AValue, const Int64 ANumberOfMilliSeconds = 1);

Boolean IsLeapYear(Word Year);

UnicodeString StripHotkey(const UnicodeString & AText);
bool StartsText(const UnicodeString & ASubText, const UnicodeString & AText);

struct TVersionInfo
{
  DWORD Major;
  DWORD Minor;
  DWORD Revision;
  DWORD Build;
};

#define MAKEVERSIONNUMBER(major, minor, revision) ( ((major)<<16) | ((minor)<<8) | (revision))
uintptr_t StrToVersionNumber(const UnicodeString & VersionMumberStr);
UnicodeString VersionNumberToStr(uintptr_t VersionNumber);
uintptr_t inline GetVersionNumber219() { return MAKEVERSIONNUMBER(2,1,9); }
uintptr_t inline GetVersionNumber2110() { return MAKEVERSIONNUMBER(2,1,10); }
uintptr_t inline GetVersionNumber2121() { return MAKEVERSIONNUMBER(2,1,21); }
uintptr_t inline GetCurrentVersionNumber() { return StrToVersionNumber(GetGlobals()->GetStrVersionNumber()); }

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

class ScopeExit
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
  struct nop_deleter { void operator()(void*) const {} };
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

class TPath : public TObject
{
public:
  static UnicodeString Combine(UnicodeString APath, UnicodeString FileName);
};

} // namespace Sysutils

using namespace Sysutils;

namespace base {

DWORD FindFirst(const UnicodeString & AFileName, DWORD LocalFileAttrs, TSearchRec & Rec);
DWORD FindNext(TSearchRec & Rec);
DWORD FindClose(TSearchRec & Rec);

} // namespace base
