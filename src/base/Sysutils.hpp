//---------------------------------------------------------------------------
#ifndef SysutilsH
#define SysutilsH

#include <WinBase.h>

#include <Classes.hpp>
#include <headers.hpp>

namespace Sysutils {

//---------------------------------------------------------------------------
typedef int TDayTable[12];
extern const TDayTable MonthDays[];

//---------------------------------------------------------------------------
class Exception : public std::exception, public TObject
{
public:
  explicit /* __fastcall */ Exception(const wchar_t * Msg);
  explicit /* __fastcall */ Exception(const UnicodeString & Msg);
  explicit /* __fastcall */ Exception(Exception * E);
  explicit /* __fastcall */ Exception(std::exception * E);
  explicit /* __fastcall */ Exception(const UnicodeString & Msg, int AHelpContext);
  explicit /* __fastcall */ Exception(Exception * E, int Ident);
  explicit /* __fastcall */ Exception(int Ident);

  template<typename T>
  bool InheritsFrom() const { return dynamic_cast<const T *>(this) != NULL; }

public:
  UnicodeString Message;

protected:
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
  EOSError(const UnicodeString & Msg, DWORD code) : Exception(Msg),
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
  TFormatSettings(int /* LCID */) {}
  static TFormatSettings Create(int LCID ) { return TFormatSettings(LCID); }
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

UnicodeString ExtractShortPathName(const UnicodeString & Path1);
UnicodeString ExtractDirectory(const UnicodeString & Path, wchar_t delimiter = '/');
UnicodeString ExtractFilename(const UnicodeString & Path, wchar_t delimiter = '/');
UnicodeString ExtractFileExtension(const UnicodeString & Path, wchar_t delimiter = '/');
UnicodeString ChangeFileExtension(const UnicodeString & Path, const UnicodeString & Ext, wchar_t delimiter = '/');

UnicodeString IncludeTrailingBackslash(const UnicodeString & Str);
UnicodeString ExcludeTrailingBackslash(const UnicodeString & Str);
UnicodeString ExtractFileDir(const UnicodeString & Str);
UnicodeString ExtractFilePath(const UnicodeString & Str);
UnicodeString GetCurrentDir();

UnicodeString StrToHex(const UnicodeString & Str, bool UpperCase = true, char Separator = '\0');
UnicodeString HexToStr(const UnicodeString & Hex);
unsigned int HexToInt(const UnicodeString & Hex, size_t MinChars = 0);
UnicodeString IntToHex(unsigned int Int, size_t MinChars = 0);
char HexToChar(const UnicodeString & Hex, size_t MinChars = 0);

UnicodeString ReplaceStrAll(const UnicodeString & Str, const UnicodeString & What, const UnicodeString & ByWhat);
UnicodeString SysErrorMessage(int code);

bool TryStrToDateTime(const UnicodeString & StrValue, TDateTime & Value, TFormatSettings & FormatSettings);
UnicodeString DateTimeToStr(UnicodeString & Result, const UnicodeString & Format,
                            TDateTime DateTime);
UnicodeString DateTimeToString(TDateTime DateTime);
unsigned int DayOfWeek(const TDateTime & DateTime);

TDateTime Date();
void DecodeDate(const TDateTime & DateTime, unsigned short & Y,
                unsigned short & M, unsigned short & D);
void DecodeTime(const TDateTime & DateTime, unsigned short & H,
                unsigned short & N, unsigned short & S, unsigned short & MS);

UnicodeString FormatDateTime(const UnicodeString & Fmt, TDateTime DateTime);
TDateTime SystemTimeToDateTime(const SYSTEMTIME & SystemTime);

TDateTime EncodeDate(int Year, int Month, int Day);
TDateTime EncodeTime(unsigned int Hour, unsigned int Min, unsigned int Sec, unsigned int MSec);

UnicodeString Trim(const UnicodeString & Str);
UnicodeString TrimLeft(const UnicodeString & Str);
UnicodeString TrimRight(const UnicodeString & Str);
UnicodeString UpperCase(const UnicodeString & Str);
UnicodeString LowerCase(const UnicodeString & Str);
wchar_t UpCase(const wchar_t c);
wchar_t LowCase(const wchar_t c);
UnicodeString AnsiReplaceStr(const UnicodeString & Str, const UnicodeString & From, const UnicodeString & To);
intptr_t AnsiPos(const UnicodeString & Str2, wchar_t c);
intptr_t Pos(const UnicodeString & Str2, const UnicodeString & Substr);
UnicodeString StringReplace(const UnicodeString & Str, const UnicodeString & From, const UnicodeString & To, TReplaceFlags Flags);
bool IsDelimiter(const UnicodeString & Delimiters, const UnicodeString & Str, intptr_t index);
intptr_t LastDelimiter(const UnicodeString & Delimiters, const UnicodeString & Str);
//---------------------------------------------------------------------------

int CompareText(const UnicodeString & Str1, const UnicodeString & Str2);
int AnsiCompare(const UnicodeString & Str1, const UnicodeString & Str2);
int AnsiCompareStr(const UnicodeString & Str1, const UnicodeString & Str2);
bool AnsiSameText(const UnicodeString & Str1, const UnicodeString & Str2);
bool SameText(const UnicodeString & Str1, const UnicodeString & Str2);
int AnsiCompareText(const UnicodeString & Str1, const UnicodeString & Str2);
int AnsiCompareIC(const UnicodeString & Str1, const UnicodeString & Str2);
bool AnsiContainsText(const UnicodeString & Str1, const UnicodeString & Str2);

int StringCmp(const wchar_t * s1, const wchar_t * s2);
int StringCmpI(const wchar_t * s1, const wchar_t * s2);

//---------------------------------------------------------------------------
UnicodeString IntToStr(int Value);
UnicodeString Int64ToStr(__int64 Value);
int StrToInt(const UnicodeString & Value);
__int64 ToInt(const UnicodeString & Value);
int StrToIntDef(const UnicodeString & Value, int DefVal);
__int64 StrToInt64(const UnicodeString & Value);
__int64 StrToInt64Def(const UnicodeString & Value, __int64 DefVal);
bool TryStrToInt(const std::wstring & StrValue, int & Value);
bool TryStrToInt(const std::wstring & StrValue, __int64 & Value);

//---------------------------------------------------------------------------
double StrToFloat(const UnicodeString & Value);
double StrToFloatDef(const UnicodeString & Value, double DefVal);
UnicodeString FormatFloat(const UnicodeString & Format, double Value);
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

bool FileExists(const UnicodeString & Filename);
bool RenameFile(const UnicodeString & From, const UnicodeString & To);
bool DirectoryExists(const UnicodeString & Dir);
UnicodeString FileSearch(const UnicodeString & FileName, const UnicodeString & DirectoryList);

int FileGetAttr(const UnicodeString & Filename);
int FileSetAttr(const UnicodeString & Filename, int attrs);

bool ForceDirectories(const UnicodeString & Dir);
bool DeleteFile(const UnicodeString & File);
bool CreateDir(const UnicodeString & Dir);
bool RemoveDir(const UnicodeString & Dir);

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
UnicodeString WrapText(const UnicodeString & Line, intptr_t MaxWidth = 40);
//---------------------------------------------------------------------------
UnicodeString TranslateExceptionMessage(std::exception * E);
//---------------------------------------------------------------------------

void AppendWChar(UnicodeString & Str2, const wchar_t ch);
void AppendChar(std::string & Str2, const char ch);

void AppendPathDelimiterW(UnicodeString & Str2);
void AppendPathDelimiterA(std::string & Str2);

UnicodeString ExpandEnvVars(const UnicodeString & Str2);

//---------------------------------------------------------------------------

UnicodeString StringOfChar(const wchar_t c, intptr_t len);

char * StrNew(const char * Str2);

UnicodeString ChangeFileExt(const UnicodeString & FileName, const UnicodeString & Ext);
UnicodeString ExtractFileExt(const UnicodeString & FileName);
UnicodeString ExpandUNCFileName(const UnicodeString & FileName);

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

int FindFirst(const UnicodeString & FileName, int FindAttrs, TSearchRec & Rec);
int FindNext(TSearchRec & Rec);
int FindClose(TSearchRec & Rec);

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
//---------------------------------------------------------------------------

class TCriticalSection
{
public:
  TCriticalSection();
  ~TCriticalSection();

  void Enter();
  void Leave();

  int GetAcquired() { return FAcquired; }

private:
  CRITICAL_SECTION FSection;
  int FAcquired;
};
//---------------------------------------------------------------------------
} // namespace Sysutils

using namespace Sysutils;

#endif
