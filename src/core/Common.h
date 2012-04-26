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
#include "UnicodeString.hpp"
#include "Exceptions.h"

namespace dt = boost::date_time;
namespace bg = boost::gregorian;
#endif
//---------------------------------------------------------------------------
#define EXCEPTION throw ExtException(L"", NULL)
#define THROWOSIFFALSE(C) if (!(C)) RaiseLastOSError();
#define SAFE_DESTROY_EX(CLASS, OBJ) { CLASS * PObj = OBJ; OBJ = NULL; delete PObj; }
#define SAFE_DESTROY(OBJ) SAFE_DESTROY_EX(TObject, OBJ)
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
extern const char Bom[4];
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
#ifndef _MSC_VER
TLibModule * __fastcall FindModule(void * Instance);
#endif
__int64 __fastcall Round(double Number);
bool __fastcall TryRelativeStrToDateTime(UnicodeString S, TDateTime & DateTime);
LCID __fastcall GetDefaultLCID();
//---------------------------------------------------------------------------
#ifndef _MSC_VER
typedef void __fastcall (__closure* TProcessLocalFileEvent)
  (const UnicodeString FileName, const TSearchRec Rec, void * Param);
bool __fastcall FileSearchRec(const UnicodeString FileName, TSearchRec & Rec);
void __fastcall ProcessLocalDirectory(UnicodeString DirName,
  TProcessLocalFileEvent CallBackFunc, void * Param = NULL, int FindAttrs = -1);
#else
typedef boost::signal3<void, const UnicodeString, const WIN32_FIND_DATA /* Rec */, void * /* Param */ > processlocalfile_signal_type;
typedef processlocalfile_signal_type::slot_type TProcessLocalFileEvent;
bool FileSearchRec(const UnicodeString FileName, WIN32_FIND_DATA & Rec);
void ProcessLocalDirectory(const UnicodeString DirName,
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

bool TryStrToDateTime(const UnicodeString value, System::TDateTime &Value, System::TFormatSettings &FormatSettings);
UnicodeString DateTimeToStr(UnicodeString &Result, const UnicodeString &Format,
  System::TDateTime DateTime);
UnicodeString DateTimeToString(System::TDateTime DateTime);
unsigned int DayOfWeek(const System::TDateTime &DateTime);

System::TDateTime Date();
void DecodeDate(const System::TDateTime &DateTime, unsigned int &Y,
                unsigned int &M, unsigned int &D);
void DecodeTime(const System::TDateTime &DateTime, unsigned int &H,
                unsigned int &N, unsigned int &S, unsigned int  &MS);

UnicodeString FormatDateTime(const UnicodeString fmt, System::TDateTime DateTime);
System::TDateTime SystemTimeToDateTime(const SYSTEMTIME &SystemTime);

System::TDateTime EncodeDate(int Year, int Month, int Day);
System::TDateTime EncodeTime(unsigned int Hour, unsigned int Min, unsigned int Sec, unsigned int MSec);

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
UnicodeString StringReplace(const UnicodeString str, const UnicodeString from, const UnicodeString to);
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
    EOSError(const UnicodeString msg, DWORD code) : std::exception(System::W2MB(msg.c_str()).c_str()),
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
#endif
