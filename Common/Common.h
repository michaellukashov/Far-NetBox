//---------------------------------------------------------------------------
#pragma once

#include <WinBase.h>

#include "Classes.h"

//---------------------------------------------------------------------------
#define EXCEPTION throw ExtException(NULL, "")
#define THROWOSIFFALSE(C) if (!(C)) RaiseLastOSError();
#define SCOPY(dest, source) \
  strncpy(dest, source, sizeof(dest)); \
  dest[sizeof(dest)-1] = '\0'
#define SAFE_DESTROY_EX(CLASS, OBJ) { CLASS * PObj = OBJ; OBJ = NULL; delete PObj; }
#define SAFE_DESTROY(OBJ) SAFE_DESTROY_EX(TObject, OBJ)
#define ASCOPY(dest, source) SCOPY(dest, source.c_str())
// #define FORMAT(S, F) Format(S, ARRAYOFCONST(F))
#define FMTLOAD(I, F) FmtLoadStr(I, ARRAYOFCONST(F))
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
std::wstring CutToChar(std::wstring &Str, char Ch, bool Trim);
std::wstring CopyToChars(const std::wstring & Str, int & From, std::wstring Chs, bool Trim,
  char * Delimiter = NULL);
std::wstring DelimitStr(std::wstring Str, std::wstring Chars);
std::wstring ShellDelimitStr(std::wstring Str, char Quote);
void OemToAnsi(std::wstring & Str);
void AnsiToOem(std::wstring & Str);
std::wstring ExceptionLogString(std::exception *E);
bool IsNumber(const std::wstring Str);
std::wstring SystemTemporaryDirectory();
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
/* struct TSearchRec
    int Time;
    __int64 Size;
    int Attr;
    std::wstring Name;
    int ExcludeAttr;   
    HANDLE FindHandle;
    TWin32FindData FindData;
}; */
//---------------------------------------------------------------------------
typedef void (* TProcessLocalFileEvent)
  (const std::wstring FileName, const WIN32_FIND_DATA Rec, void * Param);
bool FileSearchRec(const std::wstring FileName, WIN32_FIND_DATA &Rec);
void ProcessLocalDirectory(std::wstring DirName,
  TProcessLocalFileEvent CallBackFunc, void * Param = NULL, int FindAttrs = -1);
//---------------------------------------------------------------------------
enum TDSTMode
{
  dstmWin =  0, //
  dstmUnix = 1, // adjust UTC time to Windows "bug"
  dstmKeep = 2
};
bool UsesDaylightHack();
TDateTime EncodeDateVerbose(short int Year, short int Month, short int Day);
TDateTime EncodeTimeVerbose(short int Hour, short int Min, short int Sec, short int MSec);
TDateTime StrToDateTime(std::wstring Value);
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
//---------------------------------------------------------------------------
struct TMethod
{
  void *Code;
  void *Data;
};
//---------------------------------------------------------------------------
template<class MethodT>
void MakeMethod(void * Data, void * Code, MethodT & Method)
{
  ((TMethod*)&Method)->Data = Data;
  ((TMethod*)&Method)->Code = Code;
}
//---------------------------------------------------------------------------
static TMethod MakeMethod(void * Data, void * Code)
{
  TMethod Method = { Data, Code };
  return Method;
}
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
//---------------------------------------------------------------------------
#include <assert.h>
#ifndef _DEBUG
#undef assert
#define assert(p)   ((void)0)
#define CHECK(p) p
#else
#define CHECK(p) { bool __CHECK_RESULT__ = (p); assert(__CHECK_RESULT__); }
#endif
#define USEDPARAM(p) ((p) == (p))
//---------------------------------------------------------------------------
static void Abort()
{
    throw std::exception();
}
//---------------------------------------------------------------------------
class TCompThread : public TObject
{
public:
  TCompThread(bool value) {}
  void Resume() {}
  void Terminate() {}
  void SetEvent(HANDLE Event) {}
  bool GetTerminated() { return true; }
};
//---------------------------------------------------------------------------
std::wstring FORMAT(const wchar_t *fmt, ...);
//---------------------------------------------------------------------------
std::wstring Trim(const std::wstring str);
std::wstring TrimLeft(const std::wstring str);
std::wstring TrimRight(const std::wstring str);
std::wstring UpperCase(const std::wstring Str);
std::wstring LowerCase(const std::wstring str);
wchar_t UpCase(const wchar_t c);
wchar_t LowCase(const wchar_t c);
std::wstring AnsiReplaceStr(const std::wstring str, const std::wstring from, const std::wstring to);
int AnsiPos(const std::wstring str, wchar_t c);
int Pos(const std::wstring str, const std::wstring substr);
std::wstring StringReplace(const std::wstring str, const std::wstring from, const std::wstring to);
bool IsDelimiter(const std::wstring str1, const std::wstring delim, int size);
int LastDelimiter(const std::wstring str1, const std::wstring delim);
//---------------------------------------------------------------------------

bool CompareText(const std::wstring str1, const std::wstring str2);
bool AnsiCompare(const std::wstring str1, const std::wstring str2);
bool AnsiCompareStr(const std::wstring str1, const std::wstring str2);
bool AnsiSameText(const std::wstring str1, const std::wstring str2);
bool SameText(const std::wstring str1, const std::wstring str2);
bool AnsiCompareText(const std::wstring str1, const std::wstring str2);
bool AnsiCompareIC(const std::wstring str1, const std::wstring str2);
bool AnsiContainsText(const std::wstring str1, const std::wstring str2);

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

unsigned long FileRead(HANDLE Handle, void *Buffer, unsigned long Count);
unsigned long FileWrite(HANDLE Handle, const void *Buffer, unsigned long Count);

//---------------------------------------------------------------------------

enum FileAttributesEnum
{
    faReadOnly,
    faHidden,
    faDirectory,
    faSysFile,
    faArchive,
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
bool InheritsFrom(const exception &E1, const exception &from);

//---------------------------------------------------------------------------
