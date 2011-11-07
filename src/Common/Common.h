//---------------------------------------------------------------------------
#pragma once

#include <WinBase.h>

// #include <boost/type_traits/is_base_of.hpp>
#include "boostdefines.hpp"
#include <boost/signals/signal3.hpp>
#include "boost/date_time/gregorian/gregorian.hpp"
#include "boost/date_time/year_month_day.hpp"

#include "Classes.h"

//---------------------------------------------------------------------------

namespace dt = boost::date_time;
namespace bg = boost::gregorian;

//---------------------------------------------------------------------------
#define EXCEPTION throw ExtException(L"", NULL)
#define THROWOSIFFALSE(C) if (!(C)) RaiseLastOSError();
#define SCOPY(dest, source) \
  strncpy(dest, source, sizeof(dest)); \
  dest[sizeof(dest)-1] = '\0'
#define SAFE_DESTROY_EX(CLASS, OBJ) { CLASS * PObj = OBJ; OBJ = NULL; delete PObj; }
#define SAFE_DESTROY(OBJ) SAFE_DESTROY_EX(TObject, OBJ)
#define ASCOPY(dest, source) SCOPY(dest, source.c_str())
#define FORMAT(S, ...) ::Format(S, __VA_ARGS__)
#define FMTLOAD(I, ...) ::FmtLoadStr(I, __VA_ARGS__)
#define LENOF(x) ( (sizeof((x))) / (sizeof(*(x))))
#define FLAGSET(SET, FLAG) (((SET) & (FLAG)) == (FLAG))
#define FLAGCLEAR(SET, FLAG) (((SET) & (FLAG)) == 0)
#define FLAGMASK(ENABLE, FLAG) ((ENABLE) ? (FLAG) : 0)
#define SWAP(TYPE, FIRST, SECOND) \
  { TYPE __Backup = FIRST; FIRST = SECOND; SECOND = __Backup; }

//---------------------------------------------------------------------------
extern const char EngShortMonthNames[12][4];
//---------------------------------------------------------------------------
std::wstring ReplaceChar(std::wstring Str, char A, char B);
std::wstring DeleteChar(std::wstring Str, char C);
void PackStr(std::wstring &Str);
std::wstring MakeValidFileName(std::wstring FileName);
std::wstring RootKeyToStr(HKEY RootKey);
std::wstring BooleanToStr(bool B);
std::wstring BooleanToEngStr(bool B);
std::wstring DefaultStr(const std::wstring & Str, const std::wstring & Default);
std::wstring CutToChar(std::wstring &Str, wchar_t Ch, bool Trim);
std::wstring CopyToChars(const std::wstring & Str, int & From, std::wstring Chs, bool Trim,
  char * Delimiter = NULL);
std::wstring DelimitStr(std::wstring Str, std::wstring Chars);
std::wstring ShellDelimitStr(std::wstring Str, char Quote);
void OemToAnsi(std::wstring & Str);
void AnsiToOem(std::wstring & Str);
std::wstring ExceptionLogString(const std::exception *E);
bool IsNumber(const std::wstring Str);
std::wstring SystemTemporaryDirectory();
std::wstring SysErrorMessage(int code);
std::wstring GetShellFolderPath(int CSIdl);
std::wstring StripPathQuotes(const std::wstring Path);
std::wstring AddPathQuotes(std::wstring Path);
void SplitCommand(std::wstring Command, std::wstring &Program,
  std::wstring & Params, std::wstring & Dir);
std::wstring ExtractProgram(std::wstring Command);
std::wstring FormatCommand(std::wstring Program, std::wstring Params);
std::wstring ExpandFileNameCommand(const std::wstring Command,
  const std::wstring FileName);
void ReformatFileNameCommand(std::wstring & Command);
std::wstring EscapePuttyCommandParam(std::wstring Param);
std::wstring ExpandEnvironmentVariables(const std::wstring & Str);

std::wstring ExtractShortPathName(const std::wstring & Path1);
std::wstring ExtractDirectory(const std::wstring &path, wchar_t delimiter = '/');
std::wstring ExtractFilename(const std::wstring &path, wchar_t delimiter = '/');
std::wstring ExtractFileExtension(const std::wstring &path, wchar_t delimiter = '/');
std::wstring ChangeFileExtension(const std::wstring &path, const std::wstring &ext, wchar_t delimiter = '/');

std::wstring IncludeTrailingBackslash(const std::wstring Str);
std::wstring ExcludeTrailingBackslash(const std::wstring Str);
std::wstring ExtractFileDir(const std::wstring Str);
std::wstring ExtractFilePath(const std::wstring Str);
std::wstring GetCurrentDir();
bool CreateDir(const std::wstring Dir);
bool RemoveDir(const std::wstring Dir);

bool ComparePaths(const std::wstring & Path1, const std::wstring & Path2);
bool CompareFileName(const std::wstring & Path1, const std::wstring & Path2);
bool IsReservedName(std::wstring FileName);
std::wstring DisplayableStr(const std::wstring Str);
std::wstring CharToHex(char Ch, bool UpperCase = true);
std::wstring StrToHex(const std::wstring Str, bool UpperCase = true, char Separator = '\0');
std::wstring HexToStr(const std::wstring Hex);
unsigned int HexToInt(const std::wstring Hex, int MinChars = 0);
std::wstring IntToHex(unsigned int Int, int MinChars = 0);
char HexToChar(const std::wstring Hex, int MinChars = 0);
std::wstring DecodeUrlChars(std::wstring S);
std::wstring EncodeUrlChars(std::wstring S, std::wstring Ignore = L"");
std::wstring EncodeUrlString(std::wstring S);
bool RecursiveDeleteFile(const std::wstring FileName, bool ToRecycleBin);
int CancelAnswer(int Answers);
int AbortAnswer(int Answers);
int ContinueAnswer(int Answers);
std::wstring LoadStr(int Ident, unsigned int MaxLength = 0);
std::wstring LoadStrPart(int Ident, int Part);
std::wstring EscapeHotkey(const std::wstring & Caption);
bool CutToken(std::wstring & Str, std::wstring & Token);
void AddToList(std::wstring & List, const std::wstring & Value, char Delimiter);
bool Is2000();
bool IsWin7();
bool IsExactly2008R2();
struct TPasLibModule;
TPasLibModule * FindModule(void * Instance);
__int64 Round(double Number);
//---------------------------------------------------------------------------
// typedef void (* TProcessLocalFileEvent)
  // (const std::wstring FileName, const WIN32_FIND_DATA Rec, void * Param);
typedef boost::signal3<void, const std::wstring, const WIN32_FIND_DATA, void *> processlocalfile_signal_type;
typedef processlocalfile_signal_type::slot_type processlocalfile_slot_type;
bool FileSearchRec(const std::wstring FileName, WIN32_FIND_DATA &Rec);
void ProcessLocalDirectory(std::wstring DirName,
  const processlocalfile_slot_type &CallBackFunc, void * Param = NULL, int FindAttrs = -1);
//---------------------------------------------------------------------------
enum TDSTMode
{
  dstmWin =  0, //
  dstmUnix = 1, // adjust UTC time to Windows "bug"
  dstmKeep = 2
};
bool UsesDaylightHack();
TDateTime EncodeDateVerbose(unsigned int Year, unsigned int Month, unsigned int Day);
TDateTime EncodeTimeVerbose(unsigned int Hour, unsigned int Min, unsigned int Sec, unsigned int MSec);
TDateTime StrToDateTime(std::wstring Value);
unsigned int DayOfWeek(const TDateTime DateTime);
TDateTime UnixToDateTime(__int64 TimeStamp, TDSTMode DSTMode);
FILETIME DateTimeToFileTime(const TDateTime DateTime, TDSTMode DSTMode);
TDateTime AdjustDateTimeFromUnix(TDateTime DateTime, TDSTMode DSTMode);
void UnifyDateTimePrecision(TDateTime & DateTime1, TDateTime & DateTime2);
TDateTime FileTimeToDateTime(const FILETIME & FileTime);
__int64 ConvertTimestampToUnix(const FILETIME & FileTime,
  TDSTMode DSTMode);
TDateTime ConvertTimestampToUTC(TDateTime DateTime);
__int64 ConvertTimestampToUnixSafe(const FILETIME & FileTime,
  TDSTMode DSTMode);
std::wstring FixedLenDateTimeFormat(const std::wstring & Format);
int CompareFileTime(TDateTime T1, TDateTime T2);

TDateTime Date();
void DecodeDate(const TDateTime &DateTime, unsigned int &Y,
    unsigned int &M, unsigned int &D);
void DecodeTime(const TDateTime &DateTime, unsigned int &H,
    unsigned int &N, unsigned int &S, unsigned int  &MS);
TDateTime EncodeDateVerbose(unsigned int Y, unsigned int M, unsigned int D);
TDateTime EncodeTimeVerbose(unsigned int H, unsigned int N, unsigned int S, unsigned int MS);

std::wstring FormatDateTime(const std::wstring &fmt, TDateTime DateTime);
TDateTime SystemTimeToDateTime(const SYSTEMTIME &SystemTime);

TDateTime EncodeDate(int Year, int Month, int Day);
TDateTime EncodeTime(unsigned int Hour, unsigned int Min, unsigned int Sec, unsigned int MSec);

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
class TGuard
{
public:
  TGuard(TCriticalSection * ACriticalSection);
  ~TGuard();

private:
  TCriticalSection * FCriticalSection;
};
//---------------------------------------------------------------------------
class TUnguard
{
public:
  TUnguard(TCriticalSection * ACriticalSection);
  ~TUnguard();

private:
  TCriticalSection * FCriticalSection;
};
//---------------------------------------------------------------------------
// C++B TLibModule is invalid (differs from PAS definition)
struct TPasLibModule
{
  TPasLibModule * Next;
  void * Instance;
  void * CodeInstance;
  void * DataInstance;
  void * ResInstance;
};
//---------------------------------------------------------------------------
extern int Win32Platform;
extern int Win32MajorVersion;
extern int Win32MinorVersion;
extern int Win32BuildNumber;
// extern int Win32CSDVersion;
//---------------------------------------------------------------------------
void InitPlatformId();
//---------------------------------------------------------------------------
#include <assert.h>
#ifndef _DEBUG
#undef assert
#define assert(p)   ((void)0)
#define CHECK(p) p
#else
#define CHECK(p) { bool __CHECK_RESULT__ = (p); assert(__CHECK_RESULT__); }
#endif
#define USEDPARAM(p) ((void)p)

//---------------------------------------------------------------------------
void Abort();
//---------------------------------------------------------------------------
void Error(int ErrorID, int data);

//---------------------------------------------------------------------------
std::wstring Trim(const std::wstring str);
std::wstring TrimLeft(const std::wstring str);
std::wstring TrimRight(const std::wstring str);
std::wstring UpperCase(const std::wstring str);
std::wstring LowerCase(const std::wstring str);
wchar_t UpCase(const wchar_t c);
wchar_t LowCase(const wchar_t c);
std::wstring AnsiReplaceStr(const std::wstring str, const std::wstring from, const std::wstring to);
int AnsiPos(const std::wstring str, wchar_t c);
int Pos(const std::wstring str, const std::wstring substr);
std::wstring StringReplace(const std::wstring str, const std::wstring from, const std::wstring to);
bool IsDelimiter(const std::wstring str, const std::wstring delim, int index);
int LastDelimiter(const std::wstring str, const std::wstring delim);
//---------------------------------------------------------------------------

bool CompareText(const std::wstring str1, const std::wstring str2);
int AnsiCompare(const std::wstring str1, const std::wstring str2);
int AnsiCompareStr(const std::wstring str1, const std::wstring str2);
int AnsiSameText(const std::wstring str1, const std::wstring str2);
bool SameText(const std::wstring str1, const std::wstring str2);
int AnsiCompareText(const std::wstring str1, const std::wstring str2);
int AnsiCompareIC(const std::wstring str1, const std::wstring str2);
bool AnsiContainsText(const std::wstring str1, const std::wstring str2);

//---------------------------------------------------------------------------

class EOSError : public std::exception
{
public:
    EOSError(std::wstring msg, DWORD code) : std::exception(::W2MB(msg.c_str()).c_str()),
        ErrorCode(code)
    {
    }
    DWORD ErrorCode;
};

void RaiseLastOSError();

//---------------------------------------------------------------------------
std::wstring IntToStr(int value);
int StrToInt(const std::wstring value);
__int64 ToInt(const std::wstring value);
int StrToIntDef(const std::wstring value, int defval);
__int64 StrToInt64(const std::wstring value);
__int64 StrToInt64Def(const std::wstring value, __int64 defval);
__int64 TryStrToInt(const std::wstring value, __int64 defval);

//---------------------------------------------------------------------------
double StrToFloat(std::wstring Value);
std::wstring FormatFloat(std::wstring Format, double value);
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

bool FileExists(const std::wstring &filename);
bool RenameFile(const std::wstring &from, const std::wstring &to);
bool DirectoryExists(const std::wstring &dir);
std::wstring FileSearch(const std::wstring FileName, const std::wstring DirectoryList);

int FileGetAttr(const std::wstring &filename);
int FileSetAttr(const std::wstring &filename, int attrs);

bool ForceDirectories(const std::wstring Dir);
bool DeleteFile(const std::wstring File);
bool RemoveDir(const std::wstring Dir);

//---------------------------------------------------------------------------
template <class Base, class Derived>
bool InheritsFrom(const Base *t)
{
    // return boost::is_base_of<Base, Derived>::value == true;
    return dynamic_cast<const Derived *>(t) != NULL;
}

//---------------------------------------------------------------------------
std::wstring Format(const wchar_t *format, ...);
std::wstring Format(const wchar_t *format, va_list args);
std::wstring FmtLoadStr(int id, ...);
//---------------------------------------------------------------------------
std::wstring WrapText(const std::wstring Line, int MaxCol = 40);
//---------------------------------------------------------------------------
std::wstring TranslateExceptionMessage(const std::exception *E);
//---------------------------------------------------------------------------

void AppendWChar(std::wstring &str, const wchar_t ch);
void AppendChar(std::string &str, const char ch);

void AppendPathDelimiterW(std::wstring &str);
void AppendPathDelimiterA(std::string &str);

std::wstring ExpandEnvVars(const std::wstring& str);

//---------------------------------------------------------------------------

std::wstring StringOfChar(const wchar_t c, size_t len);

char *StrNew(const char *str);

wchar_t *AnsiStrScan(const wchar_t *Str, const wchar_t TokenPrefix);

std::wstring ChangeFileExt(std::wstring FileName, std::wstring ext);
std::wstring ExtractFileExt(std::wstring FileName);
std::wstring ExpandUNCFileName(std::wstring FileName);

__int64 FileSeek(HANDLE file, __int64 offset, __int64 size);

//---------------------------------------------------------------------------
bool Win32Check(bool RetVal);
//---------------------------------------------------------------------------
