//---------------------------------------------------------------------------
#ifndef SysutilsH
#define SysutilsH

#include <WinBase.h>
#include <functional>

#include <Classes.hpp>
#include <headers.hpp>

namespace Sysutils {

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
UnicodeString MB2W(const char * src, const UINT cp = CP_ACP);
AnsiString W2MB(const wchar_t * src, const UINT cp = CP_ACP);
//---------------------------------------------------------------------------
typedef int TDayTable[12];
extern const TDayTable MonthDays[];

//---------------------------------------------------------------------------
class Exception : public std::runtime_error, public TObject
{
NB_DECLARE_CLASS(Exception)
public:
  explicit Exception(const wchar_t * Msg);
  explicit Exception(const UnicodeString & Msg);
  explicit Exception(Exception * E);
  explicit Exception(std::exception * E);
  explicit Exception(const UnicodeString & Msg, int AHelpContext);
  explicit Exception(Exception * E, int Ident);
  explicit Exception(int Ident);
  ~Exception() throw() {}

public:
  UnicodeString Message;

protected:
  // UnicodeString FHelpKeyword;
};

//---------------------------------------------------------------------------
class EAbort : public Exception
{
NB_DECLARE_CLASS(EAbort)
public:
  explicit EAbort(const UnicodeString & what) : Exception(what)
  {}
};

class EAccessViolation : public Exception
{
NB_DECLARE_CLASS(EAccessViolation)
public:
  explicit EAccessViolation(const UnicodeString & what) : Exception(what)
  {}
};

class EFileNotFoundError : public Exception
{
NB_DECLARE_CLASS(EFileNotFoundError)
public:
  EFileNotFoundError() : Exception("")
  {}
};

//---------------------------------------------------------------------------

class EOSError : public Exception
{
NB_DECLARE_CLASS(EOSError)
public:
  explicit EOSError(const UnicodeString & Msg, DWORD code) : Exception(Msg),
    ErrorCode(code)
  {
  }
  DWORD ErrorCode;
};

void RaiseLastOSError(DWORD Result = 0);
//---------------------------------------------------------------------------

struct TFormatSettings : public TObject
{
public:
  explicit TFormatSettings(int /* LCID */)
  {
    CurrencyFormat = 0;
    NegCurrFormat = 0;
    ThousandSeparator = 0;
    DecimalSeparator = 0;
    CurrencyDecimals = 0;
    DateSeparator = 0;
    TimeSeparator = 0;
    ListSeparator = 0;
    TwoDigitYearCenturyWindow = 0;
  }
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

//---------------------------------------------------------------------------

UnicodeString ExtractShortPathName(const UnicodeString & Path1);
UnicodeString ExtractDirectory(const UnicodeString & Path, wchar_t Delimiter = '/');
UnicodeString ExtractFilename(const UnicodeString & Path, wchar_t Delimiter = '/');
UnicodeString ExtractFileExtension(const UnicodeString & Path, wchar_t Delimiter = '/');
UnicodeString ChangeFileExtension(const UnicodeString & Path, const UnicodeString & Ext, wchar_t Delimiter = '/');

UnicodeString IncludeTrailingBackslash(const UnicodeString & Str);
UnicodeString ExcludeTrailingBackslash(const UnicodeString & Str);
UnicodeString ExtractFileDir(const UnicodeString & Str);
UnicodeString ExtractFilePath(const UnicodeString & Str);
UnicodeString GetCurrentDir();

UnicodeString IncludeTrailingPathDelimiter(const UnicodeString & Str);

UnicodeString StrToHex(const UnicodeString & Str, bool UpperCase = true, char Separator = '\0');
UnicodeString HexToStr(const UnicodeString & Hex);
uintptr_t HexToInt(const UnicodeString & Hex, uintptr_t MinChars = 0);
UnicodeString IntToHex(uintptr_t Int, uintptr_t MinChars = 0);
char HexToChar(const UnicodeString & Hex, uintptr_t MinChars = 0);

UnicodeString ReplaceStrAll(const UnicodeString & Str, const UnicodeString & What, const UnicodeString & ByWhat);
UnicodeString SysErrorMessage(int Code);

bool TryStrToDateTime(const UnicodeString & StrValue, TDateTime & Value, TFormatSettings & FormatSettings);
UnicodeString DateTimeToStr(UnicodeString & Result, const UnicodeString & Format,
                            TDateTime DateTime);
UnicodeString DateTimeToString(TDateTime DateTime);
uint32_t DayOfWeek(const TDateTime & DateTime);

TDateTime Date();
void DecodeDate(const TDateTime & DateTime, uint16_t & Y,
  uint16_t & M, uint16_t & D);
void DecodeTime(const TDateTime & DateTime, uint16_t & H,
  uint16_t & N, uint16_t & S, uint16_t & MS);

UnicodeString FormatDateTime(const UnicodeString & Fmt, TDateTime DateTime);
TDateTime SystemTimeToDateTime(const SYSTEMTIME & SystemTime);

TDateTime EncodeDate(int Year, int Month, int Day);
TDateTime EncodeTime(uint32_t Hour, uint32_t Min, uint32_t Sec, uint32_t MSec);

UnicodeString Trim(const UnicodeString & Str);
UnicodeString TrimLeft(const UnicodeString & Str);
UnicodeString TrimRight(const UnicodeString & Str);
UnicodeString UpperCase(const UnicodeString & Str);
UnicodeString LowerCase(const UnicodeString & Str);
inline wchar_t UpCase(const wchar_t Ch);
inline wchar_t LowCase(const wchar_t Ch);
UnicodeString AnsiReplaceStr(const UnicodeString & Str, const UnicodeString & From, const UnicodeString & To);
intptr_t AnsiPos(const UnicodeString & Str2, wchar_t Ch);
intptr_t Pos(const UnicodeString & Str2, const UnicodeString & Substr);
UnicodeString StringReplace(const UnicodeString & Str, const UnicodeString & From, const UnicodeString & To, const TReplaceFlags & Flags);
bool IsDelimiter(const UnicodeString & Delimiters, const UnicodeString & Str, intptr_t Index);
intptr_t FirstDelimiter(const UnicodeString & Delimiters, const UnicodeString & Str);
intptr_t LastDelimiter(const UnicodeString & Delimiters, const UnicodeString & Str);
//---------------------------------------------------------------------------

intptr_t CompareText(const UnicodeString & Str1, const UnicodeString & Str2);
intptr_t AnsiCompare(const UnicodeString & Str1, const UnicodeString & Str2);
intptr_t AnsiCompareStr(const UnicodeString & Str1, const UnicodeString & Str2);
bool AnsiSameText(const UnicodeString & Str1, const UnicodeString & Str2);
bool SameText(const UnicodeString & Str1, const UnicodeString & Str2);
intptr_t AnsiCompareText(const UnicodeString & Str1, const UnicodeString & Str2);
intptr_t AnsiCompareIC(const UnicodeString & Str1, const UnicodeString & Str2);
bool AnsiContainsText(const UnicodeString & Str1, const UnicodeString & Str2);

int StringCmp(const wchar_t * S1, const wchar_t * S2);
int StringCmpI(const wchar_t * S1, const wchar_t * S2);

//---------------------------------------------------------------------------
UnicodeString IntToStr(intptr_t Value);
UnicodeString Int64ToStr(int64_t Value);
intptr_t StrToInt(const UnicodeString & Value);
int64_t ToInt(const UnicodeString & Value);
intptr_t StrToIntDef(const UnicodeString & Value, intptr_t DefVal);
int64_t StrToInt64(const UnicodeString & Value);
int64_t StrToInt64Def(const UnicodeString & Value, int64_t DefVal);
bool TryStrToInt(const UnicodeString & StrValue, int64_t & Value);

//---------------------------------------------------------------------------
double StrToFloat(const UnicodeString & Value);
double StrToFloatDef(const UnicodeString & Value, double DefVal);
UnicodeString FormatFloat(const UnicodeString & Format, double Value);
bool IsZero(double Value);
//---------------------------------------------------------------------------
TTimeStamp DateTimeToTimeStamp(TDateTime DateTime);
//---------------------------------------------------------------------------

int64_t FileRead(HANDLE Handle, void * Buffer, int64_t Count);
int64_t FileWrite(HANDLE Handle, const void * Buffer, int64_t Count);
int64_t FileSeek(HANDLE Handle, int64_t Offset, DWORD Origin);

//---------------------------------------------------------------------------

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

bool FileExists(const UnicodeString & FileName);
bool RenameFile(const UnicodeString & From, const UnicodeString & To);
bool DirectoryExists(const UnicodeString & Dir);
UnicodeString FileSearch(const UnicodeString & FileName, const UnicodeString & DirectoryList);

inline DWORD FileGetAttr(const UnicodeString & FileName);
inline DWORD FileSetAttr(const UnicodeString & FileName, DWORD LocalFileAttrs);

bool ForceDirectories(const UnicodeString & Dir);
bool DeleteFile(const UnicodeString & File);
bool CreateDir(const UnicodeString & Dir);
bool RemoveDir(const UnicodeString & Dir);

//---------------------------------------------------------------------------
UnicodeString Format(const wchar_t * Format, ...);
UnicodeString Format(const wchar_t * Format, va_list Args);
AnsiString Format(const char * Format, ...);
AnsiString Format(const char * Format, va_list Args);
UnicodeString FmtLoadStr(intptr_t Id, ...);
//---------------------------------------------------------------------------
UnicodeString WrapText(const UnicodeString & Line, intptr_t MaxWidth = 40);
//---------------------------------------------------------------------------
UnicodeString TranslateExceptionMessage(Exception * E);
//---------------------------------------------------------------------------

void AppendWChar(UnicodeString & Str2, const wchar_t Ch);
void AppendChar(std::string & Str2, const char Ch);

void AppendPathDelimiterW(UnicodeString & Str2);
void AppendPathDelimiterA(std::string & Str2);

UnicodeString ExpandEnvVars(const UnicodeString & Str2);

//---------------------------------------------------------------------------

UnicodeString StringOfChar(const wchar_t Ch, intptr_t Len);

UnicodeString ChangeFileExt(const UnicodeString & FileName, const UnicodeString & Ext);
UnicodeString ExtractFileExt(const UnicodeString & FileName);
UnicodeString ExpandUNCFileName(const UnicodeString & FileName);

//---------------------------------------------------------------------------
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

//---------------------------------------------------------------------------

DWORD FindFirst(const UnicodeString & FileName, DWORD LocalFileAttrs, TSearchRec & Rec);
DWORD FindNext(TSearchRec & Rec);
DWORD FindClose(TSearchRec & Rec);

//---------------------------------------------------------------------------
void InitPlatformId();
bool Win32Check(bool RetVal);
//---------------------------------------------------------------------------
class EConvertError : public Exception
{
public:
  EConvertError(const UnicodeString & Msg) :
    Exception(Msg)
  {}
};

//---------------------------------------------------------------------------
UnicodeString UnixExcludeLeadingBackslash(const UnicodeString & Path);
//---------------------------------------------------------------------------
extern int RandSeed;
extern void Randomize();
//---------------------------------------------------------------------------
TDateTime IncYear(const TDateTime & AValue, const Int64 ANumberOfYears = 1);
TDateTime IncMonth(const TDateTime & AValue, const Int64 NumberOfMonths = 1);
TDateTime IncWeek(const TDateTime & AValue, const Int64 ANumberOfWeeks = 1);
TDateTime IncDay(const TDateTime & AValue, const Int64 ANumberOfDays = 1);
TDateTime IncHour(const TDateTime & AValue, const Int64 ANumberOfHours = 1);
TDateTime IncMinute(const TDateTime & AValue, const Int64 ANumberOfMinutes = 1);
TDateTime IncSecond(const TDateTime & AValue, const Int64 ANumberOfSeconds = 1);
TDateTime IncMilliSecond(const TDateTime & AValue, const Int64 ANumberOfMilliSeconds = 1);

Boolean IsLeapYear(Word Year);

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

class TCriticalSection : public TObject
{
public:
  TCriticalSection();
  ~TCriticalSection();

  void Enter();
  void Leave();

  int GetAcquired() const { return FAcquired; }

private:
  CRITICAL_SECTION FSection;
  int FAcquired;
};
//---------------------------------------------------------------------------
UnicodeString StripHotkey(const UnicodeString & AText);
bool StartsText(const UnicodeString & ASubText, const UnicodeString & AText);
//---------------------------------------------------------------------------
struct TVersionInfo
{
  DWORD Major;
  DWORD Minor;
  DWORD Revision;
  DWORD Build;
};
#define MAKEVERSIONNUMBER(major,minor,revision) ( ((major)<<16) | ((minor)<<8) | (revision))
uintptr_t StrToVersionNumber(const UnicodeString & VersionMumberStr);
UnicodeString VersionNumberToStr(uintptr_t VersionNumber);
uintptr_t inline GetVersionNumber219() { return MAKEVERSIONNUMBER(2,1,9); }
uintptr_t inline GetVersionNumber2110() { return MAKEVERSIONNUMBER(2,1,10); }
uintptr_t inline GetVersionNumber2121() { return MAKEVERSIONNUMBER(2,1,21); }
uintptr_t inline GetCurrentVersionNumber() { return StrToVersionNumber(GetGlobalFunctions()->GetStrVersionNumber()); }
//---------------------------------------------------------------------------
class ScopeExit
{
public:
  ScopeExit(const std::function<void()> & f) : m_f(f) {}
  ~ScopeExit() { m_f(); }

private:
  std::function<void()> m_f;
};

#define _SCOPE_EXIT_NAME(name, suffix) name ## suffix
#define SCOPE_EXIT_NAME(name, suffix) _SCOPE_EXIT_NAME(name, suffix)
#define SCOPE_EXIT \
  std::function<void()> SCOPE_EXIT_NAME(scope_exit_func_, __LINE__); \
  ScopeExit SCOPE_EXIT_NAME(scope_exit_, __LINE__) = SCOPE_EXIT_NAME(scope_exit_func_, __LINE__) = [&]() /* lambda body here */
//---------------------------------------------------------------------------
} // namespace Sysutils

using namespace Sysutils;

#endif
