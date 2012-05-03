//---------------------------------------------------------------------------
#ifndef SysutilsH
#define SysutilsH

#ifdef _MSC_VER
#ifdef _MSC_VER
#include <WinBase.h>

#include "boostdefines.hpp"
#include <boost/signals/signal3.hpp>
#include "boost/date_time/gregorian/gregorian.hpp"
#include "boost/date_time/year_month_day.hpp"

#include "Classes.h"
#include "UnicodeString.hpp"
#include "Exceptions.h"

namespace dt = boost::date_time;
namespace bg = boost::gregorian;
#endif

namespace Sysutils {
//---------------------------------------------------------------------------
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

bool TryStrToDateTime(const UnicodeString value, TDateTime &Value, TFormatSettings &FormatSettings);
UnicodeString DateTimeToStr(UnicodeString &Result, const UnicodeString &Format,
  TDateTime DateTime);
UnicodeString DateTimeToString(TDateTime DateTime);
unsigned int DayOfWeek(const TDateTime &DateTime);

TDateTime Date();
void DecodeDate(const TDateTime &DateTime, unsigned short &Y,
  unsigned short &M, unsigned short &D);
void DecodeTime(const TDateTime &DateTime, unsigned short &H,
  unsigned short &N, unsigned short &S, unsigned short &MS);

UnicodeString FormatDateTime(const UnicodeString fmt, TDateTime DateTime);
TDateTime SystemTimeToDateTime(const SYSTEMTIME &SystemTime);

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
size_t AnsiPos(const UnicodeString str, wchar_t c);
size_t Pos(const UnicodeString str, const UnicodeString substr);
UnicodeString StringReplace(const UnicodeString str, const UnicodeString from, const UnicodeString to, TReplaceFlags Flags);
bool IsDelimiter(const UnicodeString str, const UnicodeString delim, size_t index);
size_t LastDelimiter(const UnicodeString str, const UnicodeString delim);
//---------------------------------------------------------------------------

int CompareText(const UnicodeString str1, const UnicodeString str2);
int AnsiCompare(const UnicodeString str1, const UnicodeString str2);
int AnsiCompareStr(const UnicodeString str1, const UnicodeString str2);
bool AnsiSameText(const UnicodeString str1, const UnicodeString str2);
bool SameText(const UnicodeString str1, const UnicodeString str2);
int AnsiCompareText(const UnicodeString str1, const UnicodeString str2);
int AnsiCompareIC(const UnicodeString str1, const UnicodeString str2);
bool AnsiContainsText(const UnicodeString str1, const UnicodeString str2);

int StringCmp(const wchar_t *s1, const wchar_t *s2);
int StringCmpI(const wchar_t *s1, const wchar_t *s2);
//---------------------------------------------------------------------------

class EOSError : public std::exception
{
public:
    EOSError(const UnicodeString msg, DWORD code) : std::exception(W2MB(msg.c_str()).c_str()),
        ErrorCode(code)
    {
    }
    DWORD ErrorCode;
};

void RaiseLastOSError();

//---------------------------------------------------------------------------
UnicodeString IntToStr(int value);
UnicodeString Int64ToStr(__int64 value);
int StrToInt(const UnicodeString value);
__int64 ToInt(const UnicodeString value);
int StrToIntDef(const UnicodeString value, int defval);
__int64 StrToInt64(const UnicodeString value);
__int64 StrToInt64Def(const UnicodeString value, __int64 defval);
bool TryStrToInt(const UnicodeString value, int &Value);
bool TryStrToInt(const UnicodeString value, __int64 &Value);

//---------------------------------------------------------------------------
double StrToFloat(const UnicodeString Value);
double StrToFloatDef(const UnicodeString Value, double defval);
UnicodeString FormatFloat(const UnicodeString Format, double value);
//---------------------------------------------------------------------------
TTimeStamp DateTimeToTimeStamp(TDateTime DateTime);
//---------------------------------------------------------------------------

__int64 FileRead(HANDLE Handle, void *Buffer, __int64 Count);
__int64 FileWrite(HANDLE Handle, const void *Buffer, __int64 Count);

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
bool InheritsFrom(const Base *t)
{
    return dynamic_cast<const Derived *>(t) != NULL;
}

//---------------------------------------------------------------------------
UnicodeString Format(const wchar_t *format, ...);
UnicodeString Format(const wchar_t *format, va_list args);
UnicodeString FmtLoadStr(int id, ...);
//---------------------------------------------------------------------------
UnicodeString WrapText(const UnicodeString Line, int MaxCol = 40);
//---------------------------------------------------------------------------
UnicodeString TranslateExceptionMessage(const std::exception *E);
//---------------------------------------------------------------------------

void AppendWChar(UnicodeString &str, const wchar_t ch);
void AppendChar(std::string &str, const char ch);

void AppendPathDelimiterW(UnicodeString &str);
void AppendPathDelimiterA(std::string &str);

UnicodeString ExpandEnvVars(const UnicodeString str);

//---------------------------------------------------------------------------

UnicodeString StringOfChar(const wchar_t c, size_t len);

char *StrNew(const char *str);

wchar_t *AnsiStrScan(const wchar_t *Str, const wchar_t TokenPrefix);

UnicodeString ChangeFileExt(const UnicodeString FileName, const UnicodeString ext);
UnicodeString ExtractFileExt(const UnicodeString FileName);
UnicodeString ExpandUNCFileName(const UnicodeString FileName);

__int64 FileSeek(HANDLE file, __int64 offset, int Origin);

//---------------------------------------------------------------------------
void InitPlatformId();
bool Win32Check(bool RetVal);
//---------------------------------------------------------------------------
class EConvertError : public ExtException
{
  typedef ExtException parent;
public:
  EConvertError(const UnicodeString Msg) :
    parent(Msg, NULL)
  {}
};

bool ForceDirectories(const UnicodeString Dir);
bool DeleteFile(const UnicodeString File);
bool CreateDir(const UnicodeString Dir);
bool RemoveDir(const UnicodeString Dir);

//---------------------------------------------------------------------------
template <class Base, class Derived>
bool InheritsFrom(const Base *t)
{
    return dynamic_cast<const Derived *>(t) != NULL;
}

//---------------------------------------------------------------------------
UnicodeString Format(const wchar_t *format, ...);
UnicodeString Format(const wchar_t *format, va_list args);
UnicodeString FmtLoadStr(int id, ...);
//---------------------------------------------------------------------------
UnicodeString WrapText(const UnicodeString Line, int MaxCol = 40);
//---------------------------------------------------------------------------
UnicodeString TranslateExceptionMessage(const std::exception *E);
//---------------------------------------------------------------------------

void AppendWChar(UnicodeString &str, const wchar_t ch);
void AppendChar(std::string &str, const char ch);

void AppendPathDelimiterW(UnicodeString &str);
void AppendPathDelimiterA(std::string &str);

UnicodeString ExpandEnvVars(const UnicodeString str);

//---------------------------------------------------------------------------

UnicodeString StringOfChar(const wchar_t c, size_t len);

char *StrNew(const char *str);

wchar_t *AnsiStrScan(const wchar_t *Str, const wchar_t TokenPrefix);

UnicodeString ChangeFileExt(const UnicodeString FileName, const UnicodeString ext);
UnicodeString ExtractFileExt(const UnicodeString FileName);
UnicodeString ExpandUNCFileName(const UnicodeString FileName);

__int64 FileSeek(HANDLE file, __int64 offset, int Origin);

//---------------------------------------------------------------------------
void InitPlatformId();
bool Win32Check(bool RetVal);
//---------------------------------------------------------------------------
class EConvertError : public ExtException
{
  typedef ExtException parent;
public:
  EConvertError(const UnicodeString Msg) :
    parent(Msg, NULL)
  {}
};
//---------------------------------------------------------------------------

} // namespace Sysutils

using namespace Sysutils;

#endif
