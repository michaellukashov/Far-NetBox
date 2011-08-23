//---------------------------------------------------------------------------
#pragma once

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
wstring ReplaceChar(wstring Str, char A, char B);
wstring DeleteChar(wstring Str, char C);
void PackStr(wstring &Str);
wstring MakeValidFileName(wstring FileName);
wstring RootKeyToStr(HKEY RootKey);
wstring BooleanToStr(bool B);
wstring BooleanToEngStr(bool B);
wstring DefaultStr(const wstring & Str, const wstring & Default);
wstring CutToChar(wstring &Str, char Ch, bool Trim);
wstring CopyToChars(const wstring & Str, int & From, wstring Chs, bool Trim,
  char * Delimiter = NULL);
wstring DelimitStr(wstring Str, wstring Chars);
wstring ShellDelimitStr(wstring Str, char Quote);
void OemToAnsi(wstring & Str);
void AnsiToOem(wstring & Str);
wstring ExceptionLogString(exception *E);
bool IsNumber(const wstring Str);
wstring SystemTemporaryDirectory();
wstring GetShellFolderPath(int CSIdl);
wstring StripPathQuotes(const wstring Path);
wstring AddPathQuotes(wstring Path);
void SplitCommand(wstring Command, wstring &Program,
  wstring & Params, wstring & Dir);
wstring ExtractProgram(wstring Command);
wstring FormatCommand(wstring Program, wstring Params);
wstring ExpandFileNameCommand(const wstring Command,
  const wstring FileName);
void ReformatFileNameCommand(wstring & Command);
wstring EscapePuttyCommandParam(wstring Param);
wstring ExpandEnvironmentVariables(const wstring & Str);
bool ComparePaths(const wstring & Path1, const wstring & Path2);
bool CompareFileName(const wstring & Path1, const wstring & Path2);
bool IsReservedName(wstring FileName);
wstring DisplayableStr(const wstring Str);
wstring CharToHex(char Ch, bool UpperCase = true);
wstring StrToHex(const wstring Str, bool UpperCase = true, char Separator = '\0');
wstring HexToStr(const wstring Hex);
unsigned int HexToInt(const wstring Hex, int MinChars = 0);
char HexToChar(const wstring Hex, int MinChars = 0);
wstring DecodeUrlChars(wstring S);
wstring EncodeUrlChars(wstring S, wstring Ignore = L"");
wstring EncodeUrlString(wstring S);
bool RecursiveDeleteFile(const wstring FileName, bool ToRecycleBin);
int CancelAnswer(int Answers);
int AbortAnswer(int Answers);
int ContinueAnswer(int Answers);
static wstring LoadStr(int Ident, unsigned int MaxLength = 0)
{
    return L"";
}
wstring LoadStrPart(int Ident, int Part);
wstring EscapeHotkey(const wstring & Caption);
bool CutToken(wstring & Str, wstring & Token);
void AddToList(wstring & List, const wstring & Value, char Delimiter);
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
    wstring Name;
    int ExcludeAttr;   
    HANDLE FindHandle;
    TWin32FindData FindData;
}; */
//---------------------------------------------------------------------------
typedef void (* TProcessLocalFileEvent)
  (const wstring FileName, const WIN32_FIND_DATA Rec, void * Param);
bool FileSearchRec(const wstring FileName, WIN32_FIND_DATA &Rec);
void ProcessLocalDirectory(wstring DirName,
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
wstring FixedLenDateTimeFormat(const wstring & Format);
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
wstring FORMAT(const wstring &fmt, ...);
//---------------------------------------------------------------------------
