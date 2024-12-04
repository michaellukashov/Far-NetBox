#pragma once

#include <random>
#include <Global.h>
#include <Classes.hpp>

#define THROWOSIFFALSE(C) do { if (!(C)) ::RaiseLastOSError(); } while(0)
#define SAFE_DESTROY_EX(CLASS, OBJ) do { if ((OBJ)) { CLASS * PObj = (OBJ); (OBJ) = nullptr; std::destroy_at(PObj); } } while(0)
#define SAFE_DESTROY(OBJ) SAFE_DESTROY_EX(TObject, (OBJ))
#define SAFE_CLOSE_HANDLE(H) do { if ((H) && (H) != INVALID_HANDLE_VALUE) { HANDLE HH = (H); (H) = nullptr; if (HH != nullptr) { ::CloseHandle(HH); } } } while (0)
#define NULL_TERMINATE(S) S[LENOF(S) - 1] = L'\0'

#define SWAP(TYPE, FIRST, SECOND) \
  { TYPE __Backup = (FIRST); (FIRST) = (SECOND); (SECOND) = __Backup; }

#if !defined(_MSC_VER)

#define TODO(s)
#define WARNING(s)
#define PRAGMA_ERROR(s)

#else

#define FILE_LINE __FILE__ "(" GSL_STRINGIFY(__LINE__) "): "
#ifdef HIDE_TODO
#define TODO(s)
#define WARNING(s)
#else
#define TODO(s) __pragma(message (FILE_LINE /*"warning: "*/ "TODO: " s))
#define WARNING(s) __pragma(message (FILE_LINE /*"warning: "*/ "WARN: " s))
#endif
#define PRAGMA_ERROR(s) __pragma(message (FILE_LINE "error: " s))

#endif

enum TModificationFmt { mfNone, mfMDHM, mfYMDHM, mfMDY, mfFull };

constexpr const wchar_t Slash = L'/';
constexpr const wchar_t Backslash = L'\\';

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

#ifndef NDEBUG
#if defined(_MSC_VER)
#if (_MSC_VER >= 1900)
#define DEBUG_PRINTF(format, ...) OutputDebugStringW(nb::Sprintf("Plugin: [%s:%d] %s: " format L"\n", Sysutils::ExtractFilename(__FILEW__, Backslash), __LINE__, ::MB2W(__FUNCTION__), __VA_ARGS__).c_str())
#define DEBUG_PRINTFA(format, ...) OutputDebugStringW(nb::Sprintf("Plugin: [%s:%d] %s: " format "\n", W2MB(Sysutils::ExtractFilename(__FILEW__, Backslash)), __LINE__, __FUNCTION__, __VA_ARGS__).c_str())
#else
#define DEBUG_PRINTF(format, ...) OutputDebugStringW(nb::Sprintf("Plugin: [%s:%d] %s: "TEXT(format) L"\n", Sysutils::ExtractFilename(__FILEW__, Backslash), __LINE__, ::MB2W(__FUNCTION__), __VA_ARGS__).c_str())
#define DEBUG_PRINTFA(format, ...) OutputDebugStringW(nb::Sprintf("Plugin: [%s:%d] %s: "format "\n", W2MB(Sysutils::ExtractFilename(__FILEW__, Backslash)), __LINE__, __FUNCTION__, __VA_ARGS__).c_str())
#endif
#else
#define DEBUG_PRINTF(format, ...) OutputDebugStringW(nb::Sprintf(L"Plugin: [%s:%d] %s: " format L"\n", Sysutils::ExtractFilename(MB2W(__FILE__), Backslash), __LINE__, ::MB2W(__FUNCTION__), __VA_ARGS__).c_str())
#define DEBUG_PRINTFA(format, ...) OutputDebugStringW(nb::Sprintf("Plugin: [%s:%d] %s: " format "\n", W2MB(Sysutils::ExtractFilename(MB2W(__FILE__), Backslash)), __LINE__, __FUNCTION__, __VA_ARGS__).c_str())
#endif
#else
#define DEBUG_PRINTF(format, ...)
#define DEBUG_PRINTF2(format, ...)
#endif

NB_CORE_EXPORT UnicodeString MB2W(const char * src, const UINT cp = CP_ACP);
NB_CORE_EXPORT AnsiString W2MB(const wchar_t * src, const UINT cp = CP_ACP);

using TDayTable = int32_t[12];
constexpr const TDayTable MonthDays[] = {
  {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
  {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
};

class NB_CORE_EXPORT Exception // : public std::runtime_error
{
  CUSTOM_MEM_ALLOCATION_IMPL
public:
  static bool classof(const Exception * Obj) { return Obj != nullptr; }
  virtual bool is(TObjectClassId Kind) const { return Kind == FKind; }
public:
  Exception() noexcept : Exception(OBJECT_CLASS_Exception) {}
  explicit Exception(TObjectClassId Kind) noexcept : FKind(Kind) {}
  explicit Exception(TObjectClassId Kind, const wchar_t * Msg) noexcept;
  explicit Exception(const wchar_t * Msg) noexcept;
  explicit Exception(TObjectClassId Kind, const UnicodeString & Msg) noexcept;
  explicit Exception(const UnicodeString & Msg) noexcept;
  explicit Exception(TObjectClassId Kind, const Exception * E) noexcept;
  explicit Exception(TObjectClassId Kind, const std::exception * E) noexcept;
  explicit Exception(TObjectClassId Kind, const UnicodeString & Msg, int32_t AHelpContext) noexcept;
  explicit Exception(TObjectClassId Kind, const Exception * E, int32_t Ident) noexcept;
  explicit Exception(TObjectClassId Kind, int32_t Ident) noexcept;
  virtual ~Exception() = default;

//  void Createfmt(int32_t Id, fmt::ArgList Args);
//  FMT_VARIADIC_W(void, Createfmt, int32_t)

private:
  TObjectClassId FKind{0};
public:
  UnicodeString Message;
};

class NB_CORE_EXPORT EAbort : public Exception
{
public:
  static bool classof(const Exception * Obj) { return Obj->is(OBJECT_CLASS_EAbort); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_EAbort) || Exception::is(Kind); }
public:
  explicit EAbort(const UnicodeString & What) noexcept : EAbort(OBJECT_CLASS_EAbort, What) {}
  explicit EAbort(TObjectClassId Kind, const UnicodeString & What) noexcept : Exception(Kind, What) {}
};

class NB_CORE_EXPORT EAccessViolation : public Exception
{
public:
  static bool classof(const Exception * Obj) { return Obj->is(OBJECT_CLASS_EAccessViolation); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_EAccessViolation) || Exception::is(Kind); }
public:
  explicit EAccessViolation(const UnicodeString & What) noexcept : Exception(OBJECT_CLASS_EAccessViolation, What) {}
};

class NB_CORE_EXPORT EFileNotFoundError : public Exception
{
public:
  static bool classof(const Exception * Obj) { return Obj->is(OBJECT_CLASS_EFileNotFoundError); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_EFileNotFoundError) || Exception::is(Kind); }
public:
  EFileNotFoundError() noexcept : Exception(OBJECT_CLASS_EFileNotFoundError, L"") {}
};

class NB_CORE_EXPORT EOSError : public Exception
{
public:
  static bool classof(const Exception * Obj) { return Obj->is(OBJECT_CLASS_EOSError); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_EOSError) || Exception::is(Kind); }
public:
  explicit EOSError(const UnicodeString & Msg, DWORD Code) noexcept :
    Exception(OBJECT_CLASS_EOSError, Msg),
    ErrorCode(Code)
  {
  }
  DWORD ErrorCode{0};
};

class NB_CORE_EXPORT EInvalidOperation : public Exception
{
public:
  static bool classof(const Exception * Obj) { return Obj->is(OBJECT_CLASS_EInvalidOperation); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_EInvalidOperation) || Exception::is(Kind); }
public:
  explicit EInvalidOperation(const UnicodeString & Msg) noexcept :
    Exception(OBJECT_CLASS_EInvalidOperation, Msg)
  {
  }
};

extern void Randomize();
extern int32_t Random(int32_t Max);

NB_CORE_EXPORT [[noreturn]] void RaiseLastOSError(DWORD LastError = 0);
//NB_CORE_EXPORT void ShowExtendedException(Exception * E);
NB_CORE_EXPORT bool AppendExceptionStackTraceAndForget(TStrings *& MoreMessages);

namespace Sysutils {
//  TFormatSettings = record
//using TMonthNameArray = rde::fixed_array<UnicodeString, 12>;
//using TWeekNameArray = rde::fixed_array<UnicodeString, 7>;

struct NB_CORE_EXPORT TFormatSettings : public TObject
{
public:
  TFormatSettings() = delete;
  explicit TFormatSettings(LCID id) noexcept;
  static TFormatSettings Create(LCID id) { return TFormatSettings(id); }
  uint8_t CurrencyFormat{0};
  uint8_t NegCurrFormat{0};
  wchar_t ThousandSeparator{};
  wchar_t DecimalSeparator{};
  uint8_t CurrencyDecimals{0};
  wchar_t DateSeparator{};
  wchar_t TimeSeparator{};
  wchar_t ListSeparator{};
  UnicodeString CurrencyString;
  UnicodeString ShortDateFormat;
  UnicodeString LongDateFormat;
  UnicodeString TimeAMString;
  UnicodeString TimePMString;
  UnicodeString ShortTimeFormat{"hh:nn"};
  UnicodeString LongTimeFormat{"hh:nn:ss"};
  UnicodeString ShortMonthNames[12]{"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  UnicodeString LongMonthNames[12]{"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
  UnicodeString ShortDayNames[7]{"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
  UnicodeString LongDayNames[7]{"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
  uint16_t TwoDigitYearCenturyWindow{50};
  LCID id{0};
};

NB_CORE_EXPORT UnicodeString ExtractShortPathName(const UnicodeString & APath);
NB_CORE_EXPORT UnicodeString ExtractDirectory(const UnicodeString & APath, wchar_t Delimiter = Slash);
UnicodeString ExtractFileDrive(const UnicodeString & FileName);
NB_CORE_EXPORT UnicodeString ExtractFilename(const UnicodeString & APath, wchar_t Delimiter = Slash);
NB_CORE_EXPORT UnicodeString ExtractFileExtension(const UnicodeString & APath, wchar_t Delimiter = Slash);
NB_CORE_EXPORT UnicodeString ExpandFileName(const UnicodeString & AFileName);
NB_CORE_EXPORT UnicodeString ChangeFileExtension(const UnicodeString & APath, const UnicodeString & Ext, wchar_t Delimiter = Slash);

NB_CORE_EXPORT UnicodeString IncludeTrailingBackslash(const UnicodeString & Str);
NB_CORE_EXPORT UnicodeString ExcludeTrailingBackslash(const UnicodeString & Str);
NB_CORE_EXPORT UnicodeString ExtractFileDir(const UnicodeString & Str);
NB_CORE_EXPORT UnicodeString ExtractFilePath(const UnicodeString & Str);
NB_CORE_EXPORT UnicodeString GetCurrentDir();

NB_CORE_EXPORT UnicodeString IncludeTrailingPathDelimiter(const UnicodeString & Str);

NB_CORE_EXPORT UnicodeString StrToHex(const UnicodeString & Str, bool UpperCase = true, wchar_t Separator = L'\0');
NB_CORE_EXPORT UnicodeString HexToStr(const UnicodeString & Hex);
NB_CORE_EXPORT uint32_t HexToIntPtr(const UnicodeString & Hex, uint32_t MinChars = 0);
NB_CORE_EXPORT UnicodeString IntToHex(uint32_t Int, uint32_t MinChars = 0);
NB_CORE_EXPORT char HexToChar(const UnicodeString & Hex, uint32_t MinChars = 0);

NB_CORE_EXPORT UnicodeString ReplaceStrAll(const UnicodeString & Str, const UnicodeString & What, const UnicodeString & ByWhat);
NB_CORE_EXPORT UnicodeString SysErrorMessage(DWORD ErrorCode);

NB_CORE_EXPORT bool TryStrToDateTime(const UnicodeString & StrValue, TDateTime & Value, const TFormatSettings & FormatSettings);
NB_CORE_EXPORT UnicodeString  DateTimeToString(const UnicodeString & Format,
  const TDateTime & DateTime);
NB_CORE_EXPORT UnicodeString DateTimeToString(const TDateTime & DateTime);
NB_CORE_EXPORT uint32_t DayOfWeek(const TDateTime & DateTime);

NB_CORE_EXPORT TDateTime Date();
NB_CORE_EXPORT void DecodeDate(const TDateTime & DateTime, uint16_t & Year,
  uint16_t & Month, uint16_t & Day);
NB_CORE_EXPORT void DecodeTime(const TDateTime & DateTime, uint16_t & Hour,
  uint16_t & Min, uint16_t & Sec, uint16_t & MSec);

NB_CORE_EXPORT UnicodeString FormatDateTime(const UnicodeString & Fmt, const TDateTime & ADateTime);
NB_CORE_EXPORT TDateTime SystemTimeToDateTime(const SYSTEMTIME & SystemTime);

NB_CORE_EXPORT TDateTime EncodeDate(uint16_t Year, uint16_t Month, uint16_t Day);
NB_CORE_EXPORT TDateTime EncodeTime(uint32_t Hour, uint32_t Min, uint32_t Sec, uint32_t MSec);

NB_CORE_EXPORT UnicodeString Trim(const UnicodeString & Str);
NB_CORE_EXPORT UnicodeString TrimLeft(const UnicodeString & Str);
NB_CORE_EXPORT UnicodeString TrimRight(const UnicodeString & Str);
NB_CORE_EXPORT UnicodeString RightCutToLength(const UnicodeString& Str, int32_t MaxLength, const UnicodeString& Ellipsis = L"\u2026");
NB_CORE_EXPORT UnicodeString UpperCase(const UnicodeString & Str);
NB_CORE_EXPORT UnicodeString LowerCase(const UnicodeString & Str);
NB_CORE_EXPORT UnicodeString AnsiLowerCase(const UnicodeString & Str);
NB_CORE_EXPORT wchar_t UpCase(const wchar_t Ch);
NB_CORE_EXPORT wchar_t LowCase(const wchar_t Ch);
NB_CORE_EXPORT UnicodeString AnsiReplaceStr(const UnicodeString & Str, const UnicodeString & From, const UnicodeString & To);
NB_CORE_EXPORT int32_t AnsiPos(const UnicodeString & Str, wchar_t Ch);
NB_CORE_EXPORT int32_t Pos(const UnicodeString & Str, const UnicodeString & Substr);
NB_CORE_EXPORT UnicodeString StringReplaceAll(const UnicodeString & Str, const UnicodeString & From, const UnicodeString & To);
NB_CORE_EXPORT bool IsDelimiter(const UnicodeString & Delimiters, const UnicodeString & Str, int32_t AIndex);
NB_CORE_EXPORT int32_t FirstDelimiter(const UnicodeString & Delimiters, const UnicodeString & Str);
NB_CORE_EXPORT int32_t LastDelimiter(const UnicodeString & Delimiters, const UnicodeString & Str);

NB_CORE_EXPORT int32_t CompareText(const UnicodeString & Str1, const UnicodeString & Str2);
NB_CORE_EXPORT int32_t AnsiCompare(const UnicodeString & Str1, const UnicodeString & Str2);
NB_CORE_EXPORT int32_t AnsiCompareStr(const UnicodeString & Str1, const UnicodeString & Str2);
NB_CORE_EXPORT bool AnsiSameText(const UnicodeString & Str1, const UnicodeString & Str2);
NB_CORE_EXPORT bool SameText(const UnicodeString & Str1, const UnicodeString & Str2);
NB_CORE_EXPORT int32_t AnsiCompareText(const UnicodeString & Str1, const UnicodeString & Str2);
NB_CORE_EXPORT int32_t AnsiCompareIC(const UnicodeString & Str1, const UnicodeString & Str2);
NB_CORE_EXPORT bool SameStr(const UnicodeString & Str1, const UnicodeString & Str2);
NB_CORE_EXPORT bool AnsiSameStr(const UnicodeString & Str1, const UnicodeString & Str2);
NB_CORE_EXPORT bool AnsiContainsText(const UnicodeString & Str1, const UnicodeString & Str2);
NB_CORE_EXPORT bool ContainsStr(const AnsiString & Str1, const AnsiString & Str2);
NB_CORE_EXPORT bool ContainsStr(const UnicodeString & Str1, const UnicodeString & Str2);
NB_CORE_EXPORT bool ContainsText(const UnicodeString & Str1, const UnicodeString & Str2);
NB_CORE_EXPORT UnicodeString RightStr(const UnicodeString & Str, int32_t ACount);
NB_CORE_EXPORT int32_t PosEx(const UnicodeString & SubStr, const UnicodeString & Str, int32_t Offset = 1);
int32_t FindDelimiter(const UnicodeString & Delimiters, const UnicodeString & Str);

NB_CORE_EXPORT UnicodeString UTF8ToString(const RawByteString & Str);
NB_CORE_EXPORT UnicodeString UTF8ToString(const char * Str, int32_t Len);
UnicodeString UTF8ArrayToString(const char * Str, int32_t Len);

NB_CORE_EXPORT int32_t StringCmp(const wchar_t * S1, const wchar_t * S2);
NB_CORE_EXPORT int32_t StringCmpI(const wchar_t * S1, const wchar_t * S2);

NB_CORE_EXPORT UnicodeString IntToStr(int32_t Value);
NB_CORE_EXPORT UnicodeString UIntToStr(uint32_t Value);
NB_CORE_EXPORT UnicodeString Int64ToStr(int64_t Value);
NB_CORE_EXPORT int32_t StrToIntPtr(const UnicodeString & Value);
NB_CORE_EXPORT int32_t StrToIntDef(const UnicodeString & Value, int32_t DefVal);
NB_CORE_EXPORT int64_t StrToInt64(const UnicodeString & Value);
NB_CORE_EXPORT int64_t StrToInt64Def(const UnicodeString & Value, int64_t DefVal);
NB_CORE_EXPORT bool TryStrToInt64(const UnicodeString & StrValue, int64_t & Value);
NB_CORE_EXPORT bool TryStrToInt(const UnicodeString & StrValue, int32_t & Value);

NB_CORE_EXPORT double StrToFloat(const UnicodeString & Value);
NB_CORE_EXPORT double StrToFloatDef(const UnicodeString & Value, double DefVal);
NB_CORE_EXPORT UnicodeString FormatFloat(const UnicodeString & Format, double Value);
NB_CORE_EXPORT bool IsZero(double Value);

NB_CORE_EXPORT TTimeStamp DateTimeToTimeStamp(const TDateTime & DateTime);

NB_CORE_EXPORT int64_t FileRead(HANDLE AHandle, void * Buffer, int64_t Count);
NB_CORE_EXPORT int64_t FileWrite(HANDLE AHandle, const void * Buffer, int64_t Count);
NB_CORE_EXPORT int64_t FileSeek(HANDLE AHandle, int64_t Offset, DWORD Origin);

NB_CORE_EXPORT bool FileExists(const UnicodeString & AFileName);
NB_CORE_EXPORT bool RenameFile(const UnicodeString & From, const UnicodeString & To);
NB_CORE_EXPORT bool DirectoryExists(const UnicodeString & ADir);
NB_CORE_EXPORT UnicodeString FileSearch(const UnicodeString & AFileName, const UnicodeString & DirectoryList);
NB_CORE_EXPORT void FileAge(const UnicodeString & AFileName, TDateTime & ATimestamp);

NB_CORE_EXPORT DWORD FileGetAttr(const UnicodeString & AFileName, bool FollowLink = true);
NB_CORE_EXPORT bool FileSetAttr(const UnicodeString & AFileName, DWORD LocalFileAttrs);

NB_CORE_EXPORT bool ForceDirectories(const UnicodeString & ADir);
NB_CORE_EXPORT bool RemoveFile(const UnicodeString & AFileName);
NB_CORE_EXPORT bool CreateDir(const UnicodeString & ADir, LPSECURITY_ATTRIBUTES SecurityAttributes = nullptr);
NB_CORE_EXPORT bool MoveFile(const UnicodeString & LocalFileName, const UnicodeString & NewLocalFileName, DWORD AFlags);
NB_CORE_EXPORT bool RemoveDir(const UnicodeString & ADir);

NB_CORE_EXPORT UnicodeString WrapText(const UnicodeString & Line, int32_t MaxWidth = 40);

NB_CORE_EXPORT UnicodeString TranslateExceptionMessage(Exception * E);

NB_CORE_EXPORT void AppendWChar(UnicodeString & Str, wchar_t Ch);

NB_CORE_EXPORT void AppendPathDelimiter(UnicodeString & Str);

NB_CORE_EXPORT UnicodeString ExpandEnvVars(const UnicodeString & Str);

NB_CORE_EXPORT UnicodeString StringOfChar(wchar_t Ch, int32_t Len);

NB_CORE_EXPORT UnicodeString ChangeFileExt(const UnicodeString & AFileName, const UnicodeString & AExt,
  wchar_t Delimiter = Slash);
NB_CORE_EXPORT UnicodeString ExtractFileExt(const UnicodeString & AFileName);
NB_CORE_EXPORT UnicodeString ExpandUNCFileName(const UnicodeString & AFileName);

using TWin32FindData = WIN32_FIND_DATA;
using TFileName = UnicodeString;

struct NB_CORE_EXPORT TSystemTime
{
  Word wYear{0};
  Word wMonth{0};
  Word wDayOfWeek{0};
  Word wDay{0};
  Word wHour{0};
  Word wMinute{0};
  Word wSecond{0};
  Word wMilliseconds{0};
};

struct NB_CORE_EXPORT TFileTime
{
  Integer LowTime{0};
  Integer HighTime{0};
};

struct NB_CORE_EXPORT TSearchRec : public TObject
{
  NB_DISABLE_COPY(TSearchRec)
public:
  TSearchRec() = default;
  void Assign(const TSearchRec & rhs)
  {
    Time = rhs.Time;
    Size = rhs.Size;
    Attr = rhs.Attr;
    Name = rhs.Name;
    ExcludeAttr = rhs.ExcludeAttr;
    FindHandle  = rhs.FindHandle;
    FindData = rhs.FindData;
  }
  Integer Time{0};
  Int64 Size{0};
  DWORD Attr{0};
  TFileName Name;
  DWORD ExcludeAttr{0};
  THandle FindHandle{INVALID_HANDLE_VALUE};
  TWin32FindData FindData{};
};

// NB_CORE_EXPORT void InitPlatformId();
NB_CORE_EXPORT bool Win32Check(bool RetVal);

class NB_CORE_EXPORT EConvertError : public Exception
{
public:
  static bool classof(const Exception * Obj) { return Obj->is(OBJECT_CLASS_EConvertError); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_EConvertError) || Exception::is(Kind); }
public:
  explicit EConvertError(const UnicodeString & Msg) noexcept : Exception(OBJECT_CLASS_EConvertError, Msg) {}
};

NB_CORE_EXPORT UnicodeString UnixExcludeLeadingBackslash(const UnicodeString & APath);

NB_CORE_EXPORT TDateTime IncYear(const TDateTime & AValue, Int64 ANumberOfYears = 1);
NB_CORE_EXPORT TDateTime IncMonth(const TDateTime & AValue, Int64 NumberOfMonths = 1);
NB_CORE_EXPORT TDateTime IncWeek(const TDateTime & AValue, Int64 ANumberOfWeeks = 1);
NB_CORE_EXPORT TDateTime IncDay(const TDateTime & AValue, Int64 ANumberOfDays = 1);
NB_CORE_EXPORT TDateTime IncHour(const TDateTime & AValue, Int64 ANumberOfHours = 1);
NB_CORE_EXPORT TDateTime IncMinute(const TDateTime & AValue, Int64 ANumberOfMinutes = 1);
NB_CORE_EXPORT TDateTime IncSecond(const TDateTime & AValue, Int64 ANumberOfSeconds = 1);
NB_CORE_EXPORT TDateTime IncMilliSecond(const TDateTime & AValue, Int64 ANumberOfMilliSeconds = 1);
uint16_t MilliSecondOfTheSecond(const TDateTime & AValue);
int32_t MilliSecondOfTheMinute(const TDateTime & AValue);
int32_t MilliSecondOfTheHour(const TDateTime & AValue);
int32_t MilliSecondOfTheDay(const TDateTime & AValue);
int64_t MilliSecondOfTheYear(const TDateTime & AValue);
NB_CORE_EXPORT Boolean IsLeapYear(Word Year);
bool IsSameDay(const TDateTime & AValue, const TDateTime & ABasis);
int32_t YearsBetween(const TDateTime & ANow, const TDateTime & AThen);
int32_t MonthsBetween(const TDateTime & ANow, const TDateTime & AThen);
int32_t DaysBetween(const TDateTime & ANow, const TDateTime & AThen);
int64_t HoursBetween(const TDateTime & ANow, const TDateTime & AThen);
int64_t MinutesBetween(const TDateTime & ANow, const TDateTime & AThen);
NB_CORE_EXPORT int64_t SecondsBetween(const TDateTime & ANow, const TDateTime & AThen);
NB_CORE_EXPORT int64_t MilliSecondsBetween(const TDateTime & ANow, const TDateTime & AThen);

NB_CORE_EXPORT UnicodeString StripHotkey(const UnicodeString & AText);
NB_CORE_EXPORT bool StartsText(const UnicodeString & ASubText, const UnicodeString & AText);

struct NB_CORE_EXPORT TVersionInfo
{
  DWORD Major{0};
  DWORD Minor{0};
  DWORD Revision{0};
  DWORD Build{0};
};

#define MAKEVERSIONNUMBER(major, minor, revision) (((major)<<16) | ((minor)<<8) | (revision))
NB_CORE_EXPORT uint32_t StrToVersionNumber(const UnicodeString & VersionNumberStr);
NB_CORE_EXPORT UnicodeString VersionNumberToStr(uint32_t VersionNumber);
NB_CORE_EXPORT uint32_t inline GetVersionNumber219() { return MAKEVERSIONNUMBER(2, 1, 9); }
NB_CORE_EXPORT uint32_t inline GetVersionNumber2110() { return MAKEVERSIONNUMBER(2, 1, 10); }
NB_CORE_EXPORT uint32_t inline GetVersionNumber2121() { return MAKEVERSIONNUMBER(2, 1, 21); }
NB_CORE_EXPORT uint32_t inline GetCurrentVersionNumber() { return StrToVersionNumber(GetGlobals()->GetStrVersionNumber()); }
bool CheckWin32Version(int32_t Major, int32_t Minor);


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

#define DETAIL_CONCATENATE_IMPL(s1, s2) s1 ## s2
#define CONCATENATE(s1, s2) DETAIL_CONCATENATE_IMPL(s1, s2)

#define ANONYMOUS_VARIABLE(str) CONCATENATE(str, __LINE__)

#define SCOPED_ACTION(RAII_type) \
const RAII_type ANONYMOUS_VARIABLE(scoped_object_)

namespace detail {

template<typename F, typename F2>
class scope_guard final
{
public:
  scope_guard(scope_guard &&) noexcept = delete;
  scope_guard & operator=(scope_guard &&) noexcept = delete;
  scope_guard(const scope_guard &) = delete;
  scope_guard & operator =(const scope_guard &) = delete;
  explicit scope_guard(F && f, F2 && f2) : m_f(std::move(f)), m_f2(std::move(f2))
  {
    try
    {
      m_f();
    }
    catch(...)
    {
      m_f2();
      throw;
    }
    m_f2();
  }
  ~scope_guard() noexcept = default;

private:
  const F m_f{};
  const F2 m_f2{};
};

template<typename F, typename F2>
scope_guard<F, F2> make_try_finally(F && f, F2 && f2) { return scope_guard<F, F2>(std::forward<F>(f), std::forward<F2>(f2)); }

struct nop_deleter { void operator()(void *) const {} };

} // namespace detail

#define SCOPE_EXIT \
  const auto ANONYMOUS_VARIABLE(scope_exit_guard) = detail::make_scope_guard() << [&]() /* lambda body here */

#define ON_SCOPE_EXIT(FUNC, T, PARAM) \
  const auto ANONYMOUS_VARIABLE(scope_exit_guard) = \
    detail::scope_guard1<nb::FastDelegate1<void, (T)>, (T)>(nb::bind(&(FUNC), this), (PARAM))

#define try__removed
#define try__catch \
  { const auto ANONYMOUS_VARIABLE(try_catch) = Sysutils::detail::make_try_finally([&]()

#define end_try__catch ); }

#define try__finally \
  { const auto ANONYMOUS_VARIABLE(try_finally) = Sysutils::detail::make_try_finally([&]()

#define __finally \
  ,[&]() // lambda body here

#define end_try__finally ); }

#define __catch__removed ,[]()
#define __finally__removed ,[]()

#if (defined _MSC_VER && _MSC_VER > 1900)

#define NB_NONCOPYABLE(Type) \
  Type(const Type &) = delete; \
  Type & operator =(const Type &) = delete;

//  Type(Type&&) = delete; \
//  Type& operator =(Type&&) = delete;

#define NB_MOVABLE(Type) \
  Type(Type &&) noexcept = default; \
  Type & operator =(Type &&) noexcept = default;

template < typename T, T Default = T{} >
class movable
{
public:
  movable(T Value): m_Value(Value) {}
  movable& operator =(T Value) { m_Value = Value; return *this; }

  movable(const movable &rhs) { *this = rhs; }
  movable& operator =(const movable &rhs) { m_Value = rhs.m_Value; return *this; }

  movable(movable &&rhs) noexcept { *this = std::move(rhs); }
  movable& operator =(movable &&rhs) noexcept { m_Value = rhs.m_Value; rhs.m_Value = Default; return *this; }

  T& operator*() const { return m_Value; }
  T& operator*() { return m_Value; }

private:
  T m_Value{};
};

template<class T>
using movable_ptr = std::unique_ptr<T, Sysutils::detail::nop_deleter>;

namespace scope_exit {

class uncaught_exceptions_counter
{
public:
  bool is_new() const noexcept { return std::uncaught_exceptions() > m_Count; }
  int m_Count{std::uncaught_exceptions()}; // int... "a camel is a horse designed by a committee" :(
};

enum class scope_type { exit, fail, success };

template<typename F, scope_type Type>
class scope_guard0
{
public:
  NB_NONCOPYABLE(scope_guard0)
  NB_MOVABLE(scope_guard0)

  explicit scope_guard0(F&& f) noexcept : m_f(std::move(f)) {}

  ~scope_guard0() noexcept(Type == scope_type::fail)
  {
    if (*m_Active && (Type == scope_type::exit || (Type == scope_type::fail) == m_Ec.is_new()))
      m_f();
  }

private:
  F m_f;
  movable<bool> m_Active{true};
  uncaught_exceptions_counter m_Ec;
};

template<scope_type Type>
class make_scope_guard
{
public:
  template<typename F>
  auto operator<<(F &&f) { return scope_guard0<F, Type>(std::forward<F>(f)); }
};

} // namespace scope_exit

#define DETAIL_SCOPE_IMPL(type) \
  const auto ANONYMOUS_VARIABLE(scope_##type##_guard) = scope_exit::make_scope_guard<scope_exit::scope_type::type>() << [&]() /* lambda body here */

#undef SCOPE_EXIT
#define SCOPE_EXIT DETAIL_SCOPE_IMPL(exit)
#define SCOPE_FAIL DETAIL_SCOPE_IMPL(fail) noexcept
#define SCOPE_SUCCESS DETAIL_SCOPE_IMPL(success)

#endif // #if (defined _MSC_VER && _MSC_VER > 1900)

class TPath
{
public:
  static UnicodeString Combine(const UnicodeString & APath, const UnicodeString & AFileName);
  static bool IsDriveRooted(const UnicodeString & /*APath*/) { return false; } // TODO: implement
};

class TUnixPath
{
public:
  static UnicodeString Join(const UnicodeString & APath, const UnicodeString & AFileName);
};

extern UnicodeString EmptyStr;

bool FileGetSymLinkTarget(const UnicodeString & AFileName, UnicodeString & TargetName);

class NB_CORE_EXPORT EDirectoryNotFoundException final : public Exception
{
public:
  static bool classof(const Exception * Obj) { return Obj->is(OBJECT_CLASS_EDirectoryNotFoundException); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_EDirectoryNotFoundException) || Exception::is(Kind); }
public:
  EDirectoryNotFoundException() = delete;
  explicit EDirectoryNotFoundException(const UnicodeString & What) noexcept : EDirectoryNotFoundException(OBJECT_CLASS_EDirectoryNotFoundException, What) {}
  explicit EDirectoryNotFoundException(TObjectClassId Kind, const UnicodeString & What) noexcept : Exception(Kind, What) {}
};

inline bool CheckHandle(const HANDLE AFileHandle)
{
  return (AFileHandle != nullptr) && (AFileHandle != INVALID_HANDLE_VALUE);
}

inline bool CheckAttribute(DWORD Attr)
{
  return Attr != INVALID_FILE_ATTRIBUTES;
}

} // namespace Sysutils

using namespace Sysutils;

namespace base {

FILE * LocalOpenFileForWriting(const UnicodeString & LogFileName, bool Append = false);
bool WriteAndFlush(FILE * File, void const * Data, size_t Size);

bool FileExists(const UnicodeString & AFileName);
bool FileRemove(const UnicodeString & AFileName);
bool FileExistsFix(const UnicodeString & Path);
bool RenameFile(const UnicodeString & From, const UnicodeString & To);
bool DirectoryExists(const UnicodeString & ADir);
DWORD FindFirst(const UnicodeString & AFileName, DWORD LocalFileAttrs, TSearchRec & Rec);
DWORD FindNext(TSearchRec & Rec);
DWORD FindClose(TSearchRec & Rec);

} // namespace base

NB_CORE_EXPORT UnicodeString FileNameFormatString(const UnicodeString & SingleFileFormat,
  const UnicodeString & MultiFilesFormat, const TStrings * AFiles, bool Remote);
