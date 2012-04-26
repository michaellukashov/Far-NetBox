//---------------------------------------------------------------------------
#ifndef CommonH
#define CommonH

#ifdef _MSC_VER
#include <WinBase.h>

#include "boostdefines.hpp"
#include <boost/signals/signal3.hpp>
#include "boost/date_time/gregorian/gregorian.hpp"
#include "boost/date_time/year_month_day.hpp"

#include "Classes.h"

namespace dt = boost::date_time;
namespace bg = boost::gregorian;
#endif
//---------------------------------------------------------------------------
#define EXCEPTION throw ExtException(L"", NULL)
#define THROWOSIFFALSE(C) if (!(C)) RaiseLastOSError();
#define SCOPY(dest, source) \
  strncpy(dest, source, sizeof(dest)); \
  dest[sizeof(dest)-1] = '\0'
#define SAFE_DESTROY_EX(CLASS, OBJ) { CLASS * PObj = OBJ; OBJ = NULL; delete PObj; }
#define SAFE_DESTROY(OBJ) SAFE_DESTROY_EX(System::TObject, OBJ)
#define ASCOPY(dest, source) \
  { \
    AnsiString CopyBuf = source; \
    strncpy(dest, CopyBuf.c_str(), LENOF(dest)); \
    dest[LENOF(dest)-1] = '\0'; \
  }
#ifndef _MSC_VER
#define FORMAT(S, F) Format(S, ARRAYOFCONST(F))
#define FMTLOAD(I, F) FmtLoadStr(I, ARRAYOFCONST(F))
#else
#define FORMAT(S, ...) ::Format(S, __VA_ARGS__)
#define FMTLOAD(I, ...) ::FmtLoadStr(I, __VA_ARGS__)
#endif
#define LENOF(x) ( (sizeof((x))) / (sizeof(*(x))))
#define FLAGSET(SET, FLAG) (((SET) & (FLAG)) == (FLAG))
#define FLAGCLEAR(SET, FLAG) (((SET) & (FLAG)) == 0)
#define FLAGMASK(ENABLE, FLAG) ((ENABLE) ? (FLAG) : 0)
#define SWAP(TYPE, FIRST, SECOND) \
  { TYPE __Backup = FIRST; FIRST = SECOND; SECOND = __Backup; }
//---------------------------------------------------------------------------
extern const wchar_t EngShortMonthNames[12][4];
extern const char Bom[3];
extern const wchar_t TokenPrefix;
extern const wchar_t NoReplacement;
extern const wchar_t TokenReplacement;
extern const UnicodeString LocalInvalidChars;
//---------------------------------------------------------------------------
UnicodeString ReplaceChar(UnicodeString Str, wchar_t A, wchar_t B);
UnicodeString DeleteChar(UnicodeString Str, wchar_t C);
void PackStr(UnicodeString &Str);
void PackStr(RawByteString &Str);
UnicodeString MakeValidFileName(UnicodeString FileName);
UnicodeString RootKeyToStr(HKEY RootKey);
UnicodeString BooleanToStr(bool B);
UnicodeString BooleanToEngStr(bool B);
UnicodeString DefaultStr(const UnicodeString & Str, const UnicodeString & Default);
UnicodeString CutToChar(UnicodeString &Str, wchar_t Ch, bool Trim);
UnicodeString CopyToChars(const UnicodeString & Str, int & From, UnicodeString Chs, bool Trim,
  wchar_t * Delimiter = NULL, bool DoubleDelimiterEscapes = false);
UnicodeString DelimitStr(UnicodeString Str, UnicodeString Chars);
UnicodeString ShellDelimitStr(UnicodeString Str, wchar_t Quote);
UnicodeString ExceptionLogString(Exception *E);
bool IsNumber(const UnicodeString Str);
UnicodeString __fastcall SystemTemporaryDirectory();
UnicodeString __fastcall GetShellFolderPath(int CSIdl);
UnicodeString __fastcall StripPathQuotes(const UnicodeString Path);
UnicodeString __fastcall AddPathQuotes(UnicodeString Path);
void __fastcall SplitCommand(UnicodeString Command, UnicodeString &Program,
  UnicodeString & Params, UnicodeString & Dir);
UnicodeString __fastcall ValidLocalFileName(UnicodeString FileName);
UnicodeString __fastcall ValidLocalFileName(
  UnicodeString FileName, wchar_t InvalidCharsReplacement,
  const UnicodeString & TokenizibleChars, const UnicodeString & LocalInvalidChars);
UnicodeString __fastcall ExtractProgram(UnicodeString Command);
UnicodeString __fastcall FormatCommand(UnicodeString Program, UnicodeString Params);
UnicodeString __fastcall ExpandFileNameCommand(const UnicodeString Command,
  const UnicodeString FileName);
void __fastcall ReformatFileNameCommand(UnicodeString & Command);
UnicodeString __fastcall EscapePuttyCommandParam(UnicodeString Param);
UnicodeString __fastcall ExpandEnvironmentVariables(const UnicodeString & Str);
bool __fastcall ComparePaths(const UnicodeString & Path1, const UnicodeString & Path2);
bool __fastcall CompareFileName(const UnicodeString & Path1, const UnicodeString & Path2);
bool __fastcall IsReservedName(UnicodeString FileName);
UnicodeString __fastcall DisplayableStr(const RawByteString & Str);
UnicodeString __fastcall ByteToHex(unsigned char B, bool UpperCase = true);
UnicodeString __fastcall BytesToHex(const unsigned char * B, size_t Length, bool UpperCase = true, wchar_t Separator = L'\0');
UnicodeString __fastcall BytesToHex(RawByteString Str, bool UpperCase = true, wchar_t Separator = L'\0');
UnicodeString __fastcall CharToHex(wchar_t Ch, bool UpperCase = true);
RawByteString __fastcall HexToBytes(const UnicodeString Hex);
unsigned char __fastcall HexToByte(const UnicodeString Hex);
UnicodeString __fastcall DecodeUrlChars(UnicodeString S);
UnicodeString __fastcall EncodeUrlChars(UnicodeString S, UnicodeString Ignore = L"");
UnicodeString __fastcall EncodeUrlString(UnicodeString S);
bool __fastcall RecursiveDeleteFile(const UnicodeString FileName, bool ToRecycleBin);
unsigned int __fastcall CancelAnswer(unsigned int Answers);
unsigned int __fastcall AbortAnswer(unsigned int Answers);
unsigned int __fastcall ContinueAnswer(unsigned int Answers);
UnicodeString __fastcall LoadStr(int Ident, unsigned int MaxLength);
UnicodeString __fastcall LoadStrPart(int Ident, int Part);
UnicodeString __fastcall EscapeHotkey(const UnicodeString & Caption);
bool __fastcall CutToken(UnicodeString & Str, UnicodeString & Token);
void __fastcall AddToList(UnicodeString & List, const UnicodeString & Value, const UnicodeString & Delimiter);
bool __fastcall Is2000();
bool __fastcall IsWin7();
bool __fastcall IsExactly2008R2();
TLibModule * __fastcall FindModule(void * Instance);
__int64 __fastcall Round(double Number);
bool __fastcall TryRelativeStrToDateTime(UnicodeString S, TDateTime & DateTime);
LCID __fastcall GetDefaultLCID();

std::wstring StripPathQuotes(const std::wstring Path);
std::wstring AddPathQuotes(const std::wstring Path);
void SplitCommand(const std::wstring Command, std::wstring &Program,
                  std::wstring &Params, std::wstring &Dir);
std::wstring ExtractProgram(const std::wstring Command);
std::wstring FormatCommand(const std::wstring Program, const std::wstring Params);
std::wstring ExpandFileNameCommand(const std::wstring Command,
                                   const std::wstring FileName);
void ReformatFileNameCommand(std::wstring &Command);
std::wstring EscapePuttyCommandParam(const std::wstring Param);
std::wstring ExpandEnvironmentVariables(const std::wstring Str);

std::wstring ExtractShortPathName(const std::wstring Path1);
std::wstring ExtractDirectory(const std::wstring path, wchar_t delimiter = '/');
std::wstring ExtractFilename(const std::wstring path, wchar_t delimiter = '/');
std::wstring ExtractFileExtension(const std::wstring path, wchar_t delimiter = '/');
std::wstring ChangeFileExtension(const std::wstring path, const std::wstring ext, wchar_t delimiter = '/');

std::wstring IncludeTrailingBackslash(const std::wstring Str);
std::wstring ExcludeTrailingBackslash(const std::wstring Str);
std::wstring ExtractFileDir(const std::wstring Str);
std::wstring ExtractFilePath(const std::wstring Str);
std::wstring GetCurrentDir();

bool ComparePaths(const std::wstring Path1, const std::wstring Path2);
bool CompareFileName(const std::wstring Path1, const std::wstring Path2);
bool IsReservedName(const std::wstring FileName);
std::wstring DisplayableStr(const std::wstring Str);
std::wstring CharToHex(char Ch, bool UpperCase = true);
std::wstring StrToHex(const std::wstring Str, bool UpperCase = true, char Separator = '\0');
std::wstring HexToStr(const std::wstring Hex);
unsigned int HexToInt(const std::wstring Hex, size_t MinChars = 0);
std::wstring IntToHex(unsigned int Int, size_t MinChars = 0);
char HexToChar(const std::wstring Hex, size_t MinChars = 0);
std::wstring DecodeUrlChars(const std::wstring S);
std::wstring EncodeUrlChars(const std::wstring S, const std::wstring Ignore = L"");
std::wstring EncodeUrlString(const std::wstring S);
bool RecursiveDeleteFile(const std::wstring FileName, bool ToRecycleBin);
int CancelAnswer(int Answers);
int AbortAnswer(int Answers);
int ContinueAnswer(int Answers);
std::wstring LoadStr(int Ident, unsigned int MaxLength = 0);
std::wstring LoadStrPart(int Ident, int Part);
std::wstring EscapeHotkey(const std::wstring Caption);
bool CutToken(std::wstring &Str, std::wstring &Token);
void AddToList(std::wstring &List, const std::wstring Value, const std::wstring &Delimiter);
bool Is2000();
bool IsWin7();
bool IsExactly2008R2();
__int64 Round(double Number);

#ifdef _MSC_VER
std::wstring ReplaceStrAll(const std::wstring Str, const std::wstring What, const std::wstring ByWhat);
std::wstring SysErrorMessage(int code);
#endif
//---------------------------------------------------------------------------
#ifndef _MSC_VER
typedef void __fastcall (__closure* TProcessLocalFileEvent)
  (const UnicodeString FileName, const TSearchRec Rec, void * Param);
bool __fastcall FileSearchRec(const UnicodeString FileName, TSearchRec & Rec);
void __fastcall ProcessLocalDirectory(UnicodeString DirName,
  TProcessLocalFileEvent CallBackFunc, void * Param = NULL, int FindAttrs = -1);
#else
typedef boost::signal3<void, const std::wstring, const WIN32_FIND_DATA /* Rec */, void * /* Param */ > processlocalfile_signal_type;
typedef processlocalfile_signal_type::slot_type TProcessLocalFileEvent;
bool FileSearchRec(const UnicodeString FileName, WIN32_FIND_DATA & Rec);
void ProcessLocalDirectory(const std::wstring DirName,
                           const TProcessLocalFileEvent &CallBackFunc, void * Param = NULL, int FindAttrs = -1);
#endif
//---------------------------------------------------------------------------
enum TDSTMode
{
  dstmWin =  0, //
  dstmUnix = 1, // adjust UTC time to Windows "bug"
  dstmKeep = 2
};
bool __fastcall UsesDaylightHack();
TDateTime __fastcall EncodeDateVerbose(Word Year, Word Month, Word Day);
TDateTime __fastcall EncodeTimeVerbose(Word Hour, Word Min, Word Sec, Word MSec);
TDateTime __fastcall UnixToDateTime(__int64 TimeStamp, TDSTMode DSTMode);
FILETIME __fastcall DateTimeToFileTime(const TDateTime DateTime, TDSTMode DSTMode);
TDateTime __fastcall AdjustDateTimeFromUnix(TDateTime DateTime, TDSTMode DSTMode);
void __fastcall UnifyDateTimePrecision(TDateTime & DateTime1, TDateTime & DateTime2);
TDateTime __fastcall FileTimeToDateTime(const FILETIME & FileTime);
__int64 __fastcall ConvertTimestampToUnix(const FILETIME & FileTime,
  TDSTMode DSTMode);
TDateTime __fastcall ConvertTimestampToUTC(TDateTime DateTime);
__int64 __fastcall ConvertTimestampToUnixSafe(const FILETIME & FileTime,
  TDSTMode DSTMode);
UnicodeString __fastcall FixedLenDateTimeFormat(const UnicodeString & Format);
int __fastcall CompareFileTime(TDateTime T1, TDateTime T2);

bool UsesDaylightHack();
System::TDateTime EncodeDateVerbose(unsigned int Year, unsigned int Month, unsigned int Day);
System::TDateTime EncodeTimeVerbose(unsigned int Hour, unsigned int Min, unsigned int Sec, unsigned int MSec);
System::TDateTime StrToDateTime(const std::wstring Value);
bool TryStrToDateTime(const std::wstring value, System::TDateTime &Value, System::TFormatSettings &FormatSettings);
bool TryRelativeStrToDateTime(const std::wstring value, System::TDateTime &Value);
std::wstring DateTimeToStr(std::wstring &Result, const std::wstring &Format,
  System::TDateTime DateTime);
std::wstring DateTimeToString(System::TDateTime DateTime);
unsigned int DayOfWeek(const System::TDateTime &DateTime);
System::TDateTime UnixToDateTime(__int64 TimeStamp, TDSTMode DSTMode);
FILETIME DateTimeToFileTime(const System::TDateTime &DateTime, TDSTMode DSTMode);
System::TDateTime AdjustDateTimeFromUnix(System::TDateTime &DateTime, TDSTMode DSTMode);
void UnifyDateTimePrecision(System::TDateTime &DateTime1, System::TDateTime &DateTime2);
System::TDateTime FileTimeToDateTime(const FILETIME &FileTime);
__int64 ConvertTimestampToUnix(const FILETIME &FileTime,
                               TDSTMode DSTMode);
System::TDateTime ConvertTimestampToUTC(System::TDateTime DateTime);
__int64 ConvertTimestampToUnixSafe(const FILETIME &FileTime,
                                   TDSTMode DSTMode);
std::wstring FixedLenDateTimeFormat(const std::wstring Format);
int CompareFileTime(System::TDateTime T1, System::TDateTime T2);

System::TDateTime Date();
void DecodeDate(const System::TDateTime &DateTime, unsigned int &Y,
                unsigned int &M, unsigned int &D);
void DecodeTime(const System::TDateTime &DateTime, unsigned int &H,
                unsigned int &N, unsigned int &S, unsigned int  &MS);
System::TDateTime EncodeDateVerbose(unsigned int Y, unsigned int M, unsigned int D);
System::TDateTime EncodeTimeVerbose(unsigned int H, unsigned int N, unsigned int S, unsigned int MS);

std::wstring FormatDateTime(const std::wstring fmt, System::TDateTime DateTime);
System::TDateTime SystemTimeToDateTime(const SYSTEMTIME &SystemTime);

System::TDateTime EncodeDate(int Year, int Month, int Day);
System::TDateTime EncodeTime(unsigned int Hour, unsigned int Min, unsigned int Sec, unsigned int MSec);

//---------------------------------------------------------------------------
template<class MethodT>
MethodT __fastcall MakeMethod(void * Data, void * Code)
{
  MethodT Method;
  ((TMethod*)&Method)->Data = Data;
  ((TMethod*)&Method)->Code = Code;
  return Method;
}
//---------------------------------------------------------------------------
class TGuard
{
public:
  explicit TGuard(TCriticalSection * ACriticalSection);
  ~TGuard();

private:
  TCriticalSection * FCriticalSection;
};
//---------------------------------------------------------------------------
class TUnguard
{
public:
  explicit TUnguard(TCriticalSection * ACriticalSection);
  ~TUnguard();

private:
  TCriticalSection * FCriticalSection;
};
//---------------------------------------------------------------------------
extern int Win32Platform;
extern int Win32MajorVersion;
extern int Win32MinorVersion;
extern int Win32BuildNumber;
// extern int Win32CSDVersion;
//---------------------------------------------------------------------------
extern const int HoursPerDay;
extern const int MinsPerDay;
extern const int SecsPerDay;
extern const int MSecsPerDay;
//---------------------------------------------------------------------------
void InitPlatformId();
//---------------------------------------------------------------------------
#include <assert.h>
#ifndef _DEBUG
#undef assert
#define assert(p)   ((void)0)
#define CHECK(p) p
#define FAIL
#else
#define CHECK(p) { bool __CHECK_RESULT__ = (p); assert(__CHECK_RESULT__); }
#define FAIL assert(false)
#endif
#ifndef USEDPARAM
#define USEDPARAM(p) ((&p) == (&p))
#endif

//---------------------------------------------------------------------------
std::wstring Trim(const std::wstring str);
std::wstring TrimLeft(const std::wstring str);
std::wstring TrimRight(const std::wstring str);
std::wstring UpperCase(const std::wstring str);
std::wstring LowerCase(const std::wstring str);
wchar_t UpCase(const wchar_t c);
wchar_t LowCase(const wchar_t c);
std::wstring AnsiReplaceStr(const std::wstring str, const std::wstring from, const std::wstring to);
size_t AnsiPos(const std::wstring str, wchar_t c);
size_t Pos(const std::wstring str, const std::wstring substr);
std::wstring StringReplace(const std::wstring str, const std::wstring from, const std::wstring to);
bool IsDelimiter(const std::wstring str, const std::wstring delim, size_t index);
size_t LastDelimiter(const std::wstring str, const std::wstring delim);
//---------------------------------------------------------------------------

int CompareText(const std::wstring str1, const std::wstring str2);
int AnsiCompare(const std::wstring str1, const std::wstring str2);
int AnsiCompareStr(const std::wstring str1, const std::wstring str2);
bool AnsiSameText(const std::wstring str1, const std::wstring str2);
bool SameText(const std::wstring str1, const std::wstring str2);
int AnsiCompareText(const std::wstring str1, const std::wstring str2);
int AnsiCompareIC(const std::wstring str1, const std::wstring str2);
bool AnsiContainsText(const std::wstring str1, const std::wstring str2);

int StringCmp(const wchar_t *s1, const wchar_t *s2);
int StringCmpI(const wchar_t *s1, const wchar_t *s2);
//---------------------------------------------------------------------------

class EOSError : public std::exception
{
public:
    EOSError(const std::wstring msg, DWORD code) : std::exception(System::W2MB(msg.c_str()).c_str()),
        ErrorCode(code)
    {
    }
    DWORD ErrorCode;
};

void RaiseLastOSError();

//---------------------------------------------------------------------------
std::wstring IntToStr(int value);
std::wstring Int64ToStr(__int64 value);
int StrToInt(const std::wstring value);
__int64 ToInt(const std::wstring value);
int StrToIntDef(const std::wstring value, int defval);
__int64 StrToInt64(const std::wstring value);
__int64 StrToInt64Def(const std::wstring value, __int64 defval);
bool TryStrToInt(const std::wstring value, int &Value);
bool TryStrToInt(const std::wstring value, __int64 &Value);

//---------------------------------------------------------------------------
double StrToFloat(const std::wstring Value);
double StrToFloatDef(const std::wstring Value, double defval);
std::wstring FormatFloat(const std::wstring Format, double value);
//---------------------------------------------------------------------------
System::TTimeStamp DateTimeToTimeStamp(System::TDateTime DateTime);
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

bool FileExists(const std::wstring filename);
bool RenameFile(const std::wstring from, const std::wstring to);
bool DirectoryExists(const std::wstring dir);
std::wstring FileSearch(const std::wstring FileName, const std::wstring DirectoryList);

int FileGetAttr(const std::wstring filename);
int FileSetAttr(const std::wstring filename, int attrs);

bool ForceDirectories(const std::wstring Dir);
bool DeleteFile(const std::wstring File);
bool CreateDir(const std::wstring Dir);
bool RemoveDir(const std::wstring Dir);

//---------------------------------------------------------------------------
template <class Base, class Derived>
bool InheritsFrom(const Base *t)
{
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

std::wstring ExpandEnvVars(const std::wstring str);

//---------------------------------------------------------------------------

std::wstring StringOfChar(const wchar_t c, size_t len);

char *StrNew(const char *str);

wchar_t *AnsiStrScan(const wchar_t *Str, const wchar_t TokenPrefix);

std::wstring ChangeFileExt(const std::wstring FileName, const std::wstring ext);
std::wstring ExtractFileExt(const std::wstring FileName);
std::wstring ExpandUNCFileName(const std::wstring FileName);

__int64 FileSeek(HANDLE file, __int64 offset, int Origin);

//---------------------------------------------------------------------------
bool Win32Check(bool RetVal);
//---------------------------------------------------------------------------
#endif
