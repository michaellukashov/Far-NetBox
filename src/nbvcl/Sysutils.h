//---------------------------------------------------------------------------
#ifndef SysutilsH
#define SysutilsH

#ifdef _MSC_VER
#include <WinBase.h>

#include "boostdefines.hpp"

#include "Classes.h"
#include "UnicodeString.hpp"
#endif

namespace Sysutils {

//---------------------------------------------------------------------------
typedef int TDayTable[12];
extern const TDayTable MonthDays[];

//---------------------------------------------------------------------------
class Exception : public std::exception, public TObject
{
public:
  explicit /* __fastcall */ Exception(const wchar_t * Msg);
  explicit /* __fastcall */ Exception(UnicodeString Msg);
  explicit /* __fastcall */ Exception(Exception * E);
  explicit /* __fastcall */ Exception(std::exception * E);
  explicit /* __fastcall */ Exception(UnicodeString Msg, int AHelpContext);
  explicit /* __fastcall */ Exception(int Ident);

  template<typename T>
  bool InheritsFrom() const { return dynamic_cast<const T *>(this) != NULL; }

  // UnicodeString GetHelpKeyword() const { return FHelpKeyword; }
  const UnicodeString GetMessage() const { return FMessage; }
  void SetMessage(const UnicodeString value) { FMessage = value; }
protected:
  UnicodeString FMessage;
  // UnicodeString FHelpKeyword;
};

//---------------------------------------------------------------------------
class EAbort : public Exception
{
public:
  EAbort(std::wstring what) : Exception(what)
  {}
};

class EAccessViolation : public Exception
{
public:
  EAccessViolation(std::wstring what) : Exception(what)
  {}
};

class EFileNotFoundError : public Exception
{
public:
  EFileNotFoundError() : Exception("")
  {}
};

//---------------------------------------------------------------------------

class EOSError : public Exception
{
public:
  EOSError(const UnicodeString msg, DWORD code) : Exception(msg),
    ErrorCode(code)
  {
  }
  DWORD ErrorCode;
};

void RaiseLastOSError();
//---------------------------------------------------------------------------

struct TFormatSettings
{
private:
  // typedef StaticArray<UnicodeString, 12> _TFormatSettings__1;

  // typedef StaticArray<UnicodeString, 12> _TFormatSettings__2;

  // typedef StaticArray<UnicodeString, 7> _TFormatSettings__3;

  // typedef StaticArray<UnicodeString, 7> _TFormatSettings__4;
public:
  TFormatSettings(int LCID) {}
  unsigned char CurrencyFormat;
  unsigned char NegCurrFormat;
  wchar_t ThousandSeparator;
  wchar_t DecimalSeparator;
  unsigned char CurrencyDecimals;
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
  // _TFormatSettings__1 ShortMonthNames;
  // _TFormatSettings__2 LongMonthNames;
  // _TFormatSettings__3 ShortDayNames;
  // _TFormatSettings__4 LongDayNames;
  unsigned short TwoDigitYearCenturyWindow;
};

void __fastcall GetLocaleFormatSettings(int LCID, TFormatSettings & FormatSettings);
// int __fastcall GetDefaultLCID();

//---------------------------------------------------------------------------

UnicodeString ExtractShortPathName(const UnicodeString Path1);
UnicodeString ExtractDirectory(const UnicodeString path, wchar_t delimiter = '/');
UnicodeString ExtractFilename(const UnicodeString path, wchar_t delimiter = '/');
UnicodeString ExtractFileExtension(const UnicodeString path, wchar_t delimiter = '/');
UnicodeString ChangeFileExtension(const UnicodeString path, const UnicodeString ext, wchar_t delimiter = '/');

UnicodeString IncludeTrailingBackslash(const UnicodeString Str);
UnicodeString ExcludeTrailingBackslash(const UnicodeString Str);
UnicodeString ExtractFileDir(const UnicodeString Str);
UnicodeString ExtractFilePath(const UnicodeString Str);
UnicodeString GetCurrentDir();

UnicodeString StrToHex(const UnicodeString Str, bool UpperCase = true, char Separator = '\0');
UnicodeString HexToStr(const UnicodeString Hex);
unsigned int HexToInt(const UnicodeString Hex, size_t MinChars = 0);
UnicodeString IntToHex(unsigned int Int, size_t MinChars = 0);
char HexToChar(const UnicodeString Hex, size_t MinChars = 0);

UnicodeString ReplaceStrAll(const UnicodeString Str, const UnicodeString What, const UnicodeString ByWhat);
UnicodeString SysErrorMessage(int code);

bool TryStrToDateTime(const UnicodeString value, TDateTime & Value, TFormatSettings & FormatSettings);
UnicodeString DateTimeToStr(UnicodeString & Result, const UnicodeString & Format,
                            TDateTime DateTime);
UnicodeString DateTimeToString(TDateTime DateTime);
unsigned int DayOfWeek(const TDateTime & DateTime);

TDateTime Date();
void DecodeDate(const TDateTime & DateTime, unsigned short & Y,
                unsigned short & M, unsigned short & D);
void DecodeTime(const TDateTime & DateTime, unsigned short & H,
                unsigned short & N, unsigned short & S, unsigned short & MS);

UnicodeString FormatDateTime(const UnicodeString fmt, TDateTime DateTime);
TDateTime SystemTimeToDateTime(const SYSTEMTIME & SystemTime);

TDateTime EncodeDate(int Year, int Month, int Day);
TDateTime EncodeTime(unsigned int Hour, unsigned int Min, unsigned int Sec, unsigned int MSec);

UnicodeString Trim(const UnicodeString str);
UnicodeString TrimLeft(const UnicodeString str);
UnicodeString TrimRight(const UnicodeString str);
UnicodeString UpperCase(const UnicodeString str);
UnicodeString LowerCase(const UnicodeString str);
wchar_t UpCase(const wchar_t c);
wchar_t LowCase(const wchar_t c);
UnicodeString AnsiReplaceStr(const UnicodeString str, const UnicodeString from, const UnicodeString to);
int AnsiPos(const UnicodeString str, wchar_t c);
int Pos(const UnicodeString str, const UnicodeString substr);
UnicodeString StringReplace(const UnicodeString str, const UnicodeString from, const UnicodeString to, TReplaceFlags Flags);
bool IsDelimiter(const UnicodeString delimiters, const UnicodeString str, int index);
int LastDelimiter(const UnicodeString delimiters, const UnicodeString str);
//---------------------------------------------------------------------------

int CompareText(const UnicodeString str1, const UnicodeString str2);
int AnsiCompare(const UnicodeString str1, const UnicodeString str2);
int AnsiCompareStr(const UnicodeString str1, const UnicodeString str2);
bool AnsiSameText(const UnicodeString str1, const UnicodeString str2);
bool SameText(const UnicodeString str1, const UnicodeString str2);
int AnsiCompareText(const UnicodeString str1, const UnicodeString str2);
int AnsiCompareIC(const UnicodeString str1, const UnicodeString str2);
bool AnsiContainsText(const UnicodeString str1, const UnicodeString str2);

int StringCmp(const wchar_t * s1, const wchar_t * s2);
int StringCmpI(const wchar_t * s1, const wchar_t * s2);

//---------------------------------------------------------------------------
UnicodeString IntToStr(int value);
UnicodeString Int64ToStr(__int64 value);
int StrToInt(const UnicodeString value);
__int64 ToInt(const UnicodeString value);
int StrToIntDef(const UnicodeString value, int defval);
__int64 StrToInt64(const UnicodeString value);
__int64 StrToInt64Def(const UnicodeString value, __int64 defval);
bool TryStrToInt(const std::wstring value, int & Value);
bool TryStrToInt(const std::wstring value, __int64 & Value);

//---------------------------------------------------------------------------
double StrToFloat(const UnicodeString Value);
double StrToFloatDef(const UnicodeString Value, double defval);
UnicodeString FormatFloat(const UnicodeString Format, double value);
//---------------------------------------------------------------------------
TTimeStamp DateTimeToTimeStamp(TDateTime DateTime);
//---------------------------------------------------------------------------

__int64 FileRead(HANDLE Handle, void * Buffer, __int64 Count);
__int64 FileWrite(HANDLE Handle, const void * Buffer, __int64 Count);

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

bool FileExists(const UnicodeString filename);
bool RenameFile(const UnicodeString from, const UnicodeString to);
bool DirectoryExists(const UnicodeString dir);
UnicodeString FileSearch(const UnicodeString FileName, const UnicodeString DirectoryList);

int FileGetAttr(const UnicodeString filename);
int FileSetAttr(const UnicodeString filename, int attrs);

bool ForceDirectories(const UnicodeString Dir);
bool DeleteFile(const UnicodeString File);
bool CreateDir(const UnicodeString Dir);
bool RemoveDir(const UnicodeString Dir);

//---------------------------------------------------------------------------
template <class Base, class Derived>
bool InheritsFrom(const Base * t)
{
  return dynamic_cast<const Derived *>(t) != NULL;
}

//---------------------------------------------------------------------------
UnicodeString Format(const wchar_t * format, ...);
UnicodeString Format(const wchar_t * format, va_list args);
AnsiString Format(const char * format, ...);
AnsiString Format(const char * format, va_list args);
UnicodeString FmtLoadStr(int id, ...);
//---------------------------------------------------------------------------
UnicodeString WrapText(const UnicodeString Line, int MaxCol = 40);
//---------------------------------------------------------------------------
UnicodeString TranslateExceptionMessage(std::exception * E);
//---------------------------------------------------------------------------

void AppendWChar(UnicodeString & str, const wchar_t ch);
void AppendChar(std::string & str, const char ch);

void AppendPathDelimiterW(UnicodeString & str);
void AppendPathDelimiterA(std::string & str);

UnicodeString ExpandEnvVars(const UnicodeString str);

//---------------------------------------------------------------------------

UnicodeString StringOfChar(const wchar_t c, int len);

char * StrNew(const char * str);

wchar_t * AnsiStrScan(const wchar_t * Str, const wchar_t TokenPrefix);

UnicodeString ChangeFileExt(const UnicodeString FileName, const UnicodeString ext);
UnicodeString ExtractFileExt(const UnicodeString FileName);
UnicodeString ExpandUNCFileName(const UnicodeString FileName);

__int64 FileSeek(HANDLE file, __int64 offset, int Origin);

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
  Integer Time;
  Int64 Size;
  Integer Attr;
  TFileName Name;
  Integer ExcludeAttr;
  THandle FindHandle;
  TWin32FindData FindData;
};

//---------------------------------------------------------------------------

int FindFirst(const UnicodeString FileName, int FindAttrs, TSearchRec & Rec);
int FindNext(TSearchRec & Rec);
int FindClose(TSearchRec & Rec);

//---------------------------------------------------------------------------
void InitPlatformId();
bool Win32Check(bool RetVal);
//---------------------------------------------------------------------------
class EConvertError : public Exception
{
public:
  EConvertError(const UnicodeString Msg) :
    Exception(Msg)
  {}
};

//---------------------------------------------------------------------------
UnicodeString UnixExcludeLeadingBackslash(UnicodeString Path);
//---------------------------------------------------------------------------
extern int RandSeed;
extern void __fastcall Randomize();
//---------------------------------------------------------------------------
TDateTime IncYear(const TDateTime AValue, const Int64 ANumberOfYears = 1);
TDateTime IncMonth(const TDateTime AValue, const Int64 NumberOfMonths = 1);
TDateTime IncWeek(const TDateTime AValue, const Int64 ANumberOfWeeks = 1);
TDateTime IncDay(const TDateTime AValue, const Int64 ANumberOfDays = 1);
TDateTime IncHour(const TDateTime AValue, const Int64 ANumberOfHours = 1);
TDateTime IncMinute(const TDateTime AValue, const Int64 ANumberOfMinutes = 1);
TDateTime IncSecond(const TDateTime AValue, const Int64 ANumberOfSeconds = 1);
TDateTime IncMilliSecond(const TDateTime AValue, const Int64 ANumberOfMilliSeconds = 1);

Boolean IsLeapYear(Word Year);

//---------------------------------------------------------------------------
} // namespace Sysutils

using namespace Sysutils;

#endif
