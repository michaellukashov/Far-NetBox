//---------------------------------------------------------------------------
#ifndef CommonH
#define CommonH

#include <CoreDefs.hpp>

#include "Exceptions.h"
#include "plugin_version.hpp"
//---------------------------------------------------------------------------
#define EXCEPTION throw ExtException((Exception* )NULL, UnicodeString(L""))
#define THROWOSIFFALSE(C) if (!(C)) RaiseLastOSError();
#define SAFE_DESTROY_EX(CLASS, OBJ) { CLASS * PObj = OBJ; OBJ = NULL; delete PObj; }
#define SAFE_DESTROY(OBJ) SAFE_DESTROY_EX(TObject, OBJ)
#define ASCOPY(dest, source) \
  { \
    AnsiString CopyBuf = ::W2MB(source).c_str(); \
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
extern const std::string Bom;
extern const wchar_t TokenPrefix;
extern const wchar_t NoReplacement;
extern const wchar_t TokenReplacement;
extern const UnicodeString LocalInvalidChars;
//---------------------------------------------------------------------------
UnicodeString ReplaceChar(const UnicodeString & Str, wchar_t A, wchar_t B);
UnicodeString DeleteChar(const UnicodeString & Str, wchar_t C);
void PackStr(UnicodeString & Str);
void PackStr(RawByteString & Str);
void Shred(UnicodeString & Str);
UnicodeString MakeValidFileName(const UnicodeString & FileName);
UnicodeString RootKeyToStr(HKEY RootKey);
UnicodeString BooleanToStr(bool B);
UnicodeString BooleanToEngStr(bool B);
UnicodeString DefaultStr(const UnicodeString & Str, const UnicodeString & Default);
UnicodeString CutToChar(UnicodeString & Str, wchar_t Ch, bool Trim);
UnicodeString CopyToChars(const UnicodeString & Str, intptr_t & From, UnicodeString Chs, bool Trim,
  wchar_t * Delimiter = NULL, bool DoubleDelimiterEscapes = false);
UnicodeString DelimitStr(const UnicodeString & Str, const UnicodeString & Chars);
UnicodeString ShellDelimitStr(const UnicodeString & Str, wchar_t Quote);
UnicodeString ExceptionLogString(Exception *E);
bool IsNumber(const UnicodeString & Str);
UnicodeString SystemTemporaryDirectory();
UnicodeString GetShellFolderPath(int CSIdl);
UnicodeString StripPathQuotes(const UnicodeString & Path);
UnicodeString AddPathQuotes(const UnicodeString & Path);
void SplitCommand(const UnicodeString & Command, UnicodeString & Program,
  UnicodeString & Params, UnicodeString & Dir);
UnicodeString ValidLocalFileName(const UnicodeString & FileName);
UnicodeString ValidLocalFileName(
  const UnicodeString & FileName, wchar_t InvalidCharsReplacement,
  const UnicodeString & TokenizibleChars, const UnicodeString & LocalInvalidChars);
UnicodeString ExtractProgram(const UnicodeString & Command);
UnicodeString FormatCommand(const UnicodeString & Program, const UnicodeString & Params);
UnicodeString ExpandFileNameCommand(const UnicodeString & Command,
  const UnicodeString & FileName);
void ReformatFileNameCommand(UnicodeString & Command);
UnicodeString EscapePuttyCommandParam(const UnicodeString & Param);
UnicodeString ExpandEnvironmentVariables(const UnicodeString & Str);
bool ComparePaths(const UnicodeString & Path1, const UnicodeString & Path2);
bool CompareFileName(const UnicodeString & Path1, const UnicodeString & Path2);
bool IsReservedName(const UnicodeString & FileName);
UnicodeString DisplayableStr(const RawByteString & Str);
UnicodeString ByteToHex(unsigned char B, bool UpperCase = true);
UnicodeString BytesToHex(const unsigned char * B, uintptr_t Length, bool UpperCase = true, wchar_t Separator = L'\0');
UnicodeString BytesToHex(const RawByteString & Str, bool UpperCase = true, wchar_t Separator = L'\0');
UnicodeString CharToHex(wchar_t Ch, bool UpperCase = true);
RawByteString HexToBytes(const UnicodeString & Hex);
unsigned char HexToByte(const UnicodeString & Hex);
UnicodeString DecodeUrlChars(const UnicodeString & S);
UnicodeString EncodeUrlChars(const UnicodeString & S, const UnicodeString & Ignore = UnicodeString());
UnicodeString EncodeUrlString(const UnicodeString & S);
bool RecursiveDeleteFile(const UnicodeString & FileName, bool ToRecycleBin);
intptr_t CancelAnswer(intptr_t Answers);
intptr_t AbortAnswer(intptr_t Answers);
intptr_t ContinueAnswer(intptr_t Answers);
UnicodeString LoadStr(int Ident, intptr_t MaxLength = 0);
UnicodeString LoadStrPart(int Ident, int Part);
UnicodeString EscapeHotkey(const UnicodeString & Caption);
bool CutToken(UnicodeString & Str, UnicodeString & Token,
  UnicodeString * RawToken = NULL);
void AddToList(UnicodeString & List, const UnicodeString & Value, const UnicodeString & Delimiter);
bool Is2000();
bool IsWin7();
bool IsExactly2008R2();
#ifndef _MSC_VER
TLibModule * FindModule(void * Instance);
#endif
__int64 Round(double Number);
bool TryRelativeStrToDateTime(const UnicodeString & S, TDateTime & DateTime);
LCID GetDefaultLCID();
UnicodeString DefaultEncodingName();
UnicodeString WindowsProductName();
//---------------------------------------------------------------------------
DEFINE_CALLBACK_TYPE3(TProcessLocalFileEvent, void,
  const UnicodeString & /* FileName */, const TSearchRec & /* Rec */, void * /* Param */);
bool FileSearchRec(const UnicodeString & FileName, TSearchRec & Rec);
int FindCheck(int Result);
int FindFirstChecked(const UnicodeString & Path, int Attr, TSearchRec & F);
int FindNextChecked(TSearchRec & F);
void ProcessLocalDirectory(const UnicodeString & DirName,
  TProcessLocalFileEvent CallBackFunc, void * Param = NULL, int FindAttrs = -1);
//---------------------------------------------------------------------------
enum TDSTMode
{
  dstmWin =  0, //
  dstmUnix = 1, // adjust UTC time to Windows "bug"
  dstmKeep = 2
};
bool UsesDaylightHack();
TDateTime EncodeDateVerbose(Word Year, Word Month, Word Day);
TDateTime EncodeTimeVerbose(Word Hour, Word Min, Word Sec, Word MSec);
TDateTime UnixToDateTime(__int64 TimeStamp, TDSTMode DSTMode);
TDateTime ConvertFileTimestampFromUTC(TDateTime DateTime);
FILETIME DateTimeToFileTime(const TDateTime DateTime, TDSTMode DSTMode);
TDateTime AdjustDateTimeFromUnix(TDateTime DateTime, TDSTMode DSTMode);
void UnifyDateTimePrecision(TDateTime & DateTime1, TDateTime & DateTime2);
TDateTime FileTimeToDateTime(const FILETIME & FileTime);
__int64 ConvertTimestampToUnix(const FILETIME & FileTime,
  TDSTMode DSTMode);
__int64 ConvertTimestampToUnixSafe(const FILETIME & FileTime,
  TDSTMode DSTMode);
UnicodeString FixedLenDateTimeFormat(const UnicodeString & Format);
UnicodeString StandardTimestamp(const TDateTime & DateTime);
UnicodeString StandardTimestamp();
UnicodeString GetTimeZoneLogString();
int CompareFileTime(TDateTime T1, TDateTime T2);
//---------------------------------------------------------------------------
template<class MethodT>
MethodT MakeMethod(void * Data, void * Code)
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
template<class T>
class TValueRestorer
{
public:
  inline TValueRestorer(T & Target, const T & Value) :
    FTarget(Target),
    FValue(Value)
  {
  }

  inline ~TValueRestorer()
  {
    FTarget = FValue;
  }

private:
  T & FTarget;
  const T & FValue;
};
//---------------------------------------------------------------------------
class TBoolRestorer : TValueRestorer<bool>
{
public:
  inline TBoolRestorer(bool & Target) :
    TValueRestorer<bool>(Target, !Target)
  {
  }
};
//---------------------------------------------------------------------------
//!CLEANBEGIN
#undef TRACE_IN_MEMORY
#include <tchar.h>
#undef TEXT
#define TEXT(x) (wchar_t *)MB2W(x).c_str()
#define TRACING  (_TRACE,_TRACEFMT,Callstack,__callstack)
#define NOTRACING  (_TRACE_FAKE, _TRACEFMT_FAKE, , )
#define _TRACING_TRACE(TRACE, TRACEFMT, CALLSTACK, CALLSTACKI) TRACE
#define _TRACING_TRACEFMT(TRACE, TRACEFMT, CALLSTACK, CALLSTACKI) TRACEFMT
#define _TRACING_CALLSTACK(TRACE, TRACEFMT, CALLSTACK, CALLSTACKI) CALLSTACK
#define _TRACING_CALLSTACKI(TRACE, TRACEFMT, CALLSTACK, CALLSTACKI) CALLSTACKI
#define _TRACE(SOURCE, FUNC, LINE, MESSAGE) \
  if (IsTracing) Trace(TEXT(__FILE__), TEXT(__FUNCTION__), __LINE__, MESSAGE)
#define _TRACEFMT(SOURCE, FUNC, LINE, FORMAT, ...) \
  if (IsTracing) TraceFmt(TEXT(__FILE__), TEXT(__FUNCTION__), __LINE__, FORMAT, __VA_ARGS__)
#define _TRACE_FAKE(SOURCE, FUNC, LINE, MESSAGE)
#define _TRACEFMT_FAKE(SOURCE, FUNC, LINE, FORMAT, ...)

#ifdef _DEBUG
void SetTraceFile(HANDLE TraceFile);
void CleanupTracing();
#define TRACEENV "WINSCPTRACE"
extern bool IsTracing;
const unsigned int CallstackTlsOff = (unsigned int)-1;
extern unsigned int CallstackTls;
void Trace(const wchar_t * SourceFile, const wchar_t * Func,
  int Line, const wchar_t * Message);
#ifndef _MSC_VER
void TraceFmt(const wchar_t * SourceFile, const wchar_t * Func,
  int Line, const wchar_t * Format, TVarRec * Args, const int Args_Size);
#else
void TraceFmt(const wchar_t * SourceFile, const wchar_t * Func,
  int Line, const wchar_t * Format, ...);
#endif
#ifdef TRACE_IN_MEMORY
void TraceDumpToFile();
void TraceInMemoryCallback(System::UnicodeString Msg);
#endif
#define CTRACEIMPL(TRACING, MESSAGE) \
  _TRACING_TRACE TRACING (TEXT(__FILE__), TEXT(__FUNCTION__), __LINE__, (MESSAGE))
// #define CTRACEIMPL(TRACING, MESSAGE) DEBUG_PRINTF(MESSAGE);
#define CTRACEFMTIMPL(TRACING, MESSAGE, ...) \
  _TRACING_TRACEFMT TRACING (TEXT(__FILE__), TEXT(__FUNCTION__), __LINE__, MESSAGE, __VA_ARGS__)
class Callstack
{
public:
  inline Callstack(const wchar_t * File, const wchar_t * Func, unsigned int Line, const wchar_t * Message) :
    FFile(File), FFunc(Func), FLine(Line), FMessage(Message ? Message : L""), FDepth(0)
  {
    if (IsTracing)
    {
      #ifndef TRACE_IN_MEMORY
      if (CallstackTls != CallstackTlsOff)
      {
        FDepth = reinterpret_cast<intptr_t>(TlsGetValue(CallstackTls)) + 1;
        TlsSetValue(CallstackTls, reinterpret_cast<void *>(FDepth));
      }
      #endif
#ifndef _MSC_VER
      TraceFmt(FFile, FFunc, FLine, L"Entry: %s [%d]", ARRAYOFCONST((FMessage, int(FDepth))));
#else
      TraceFmt(FFile, FFunc, FLine, L"Entry: %s [%d]", FMessage, int(FDepth));
#endif
    }
  }

  inline ~Callstack()
  {
    if (IsTracing)
    {
      #ifndef TRACE_IN_MEMORY
      if (FDepth > 0)
      {
        TlsSetValue(CallstackTls, reinterpret_cast<void *>(FDepth - 1));
      }
      #endif
#ifndef _MSC_VER
      TraceFmt(FFile, FFunc, FLine, L"Exit: %s [%d]", ARRAYOFCONST((FMessage, int(FDepth))));
#else
      TraceFmt(FFile, FFunc, FLine, L"Exit: %s [%d]", FMessage, int(FDepth));
#endif
    }
  }

private:
  const wchar_t * FFile;
  const wchar_t * FFunc;
  unsigned int FLine;
  const wchar_t * FMessage;
  intptr_t FDepth;
};
void __callstack(const wchar_t*, const wchar_t*, unsigned int, const wchar_t*);
#define __callstack1 __callstack
#define CCALLSTACKIMPL(TRACING, X) // _TRACING_CALLSTACK TRACING X(TEXT(__FILE__), TEXT(__FUNCTION__), __LINE__, L"")
#else // ifdef _DEBUG
#define CTRACEIMPL(TRACING, ...)
#define CTRACEFMTIMPL(TRACING, MESSAGE, ...)
#define CCALLSTACKIMPL(TRACING, X)
#endif // ifdef _DEBUG

#undef TRACE
#define TRACE(MESSAGE) CTRACEIMPL(TRACING, _T(MESSAGE))
#define TRACEFMT(MESSAGE, ...) CTRACEFMTIMPL(TRACING, _T(MESSAGE), __VA_ARGS__)
#define CTRACE(TRACING, MESSAGE) CTRACEIMPL(TRACING, _T(MESSAGE))
#define CTRACEFMT(TRACING, MESSAGE, ...) CTRACEFMTIMPL(TRACING, _T(MESSAGE), __VA_ARGS__)

#define CCALLSTACK(TRACING) CCALLSTACKIMPL(TRACING, _TRACING_CALLSTACKI TRACING)
#define CCALLSTACK1(TRACING) CCALLSTACKIMPL(TRACING ,_TRACING_CALLSTACKI TRACING ## 1)
#define CALLSTACK CCALLSTACK(TRACING)
#define CALLSTACK1 CCALLSTACK1(TRACING)
//!CLEANEND
//---------------------------------------------------------------------------
#include <assert.h>
#ifndef _DEBUG
#undef assert
#define assert(p)   ((void)0)
#define CHECK(p) p
#define FAIL
#define TRACE_EXCEPT_BEGIN
#define TRACE_EXCEPT_END
#define TRACE_CATCH_ALL catch(...)
#define CLEAN_INLINE
#define TRACEE_(E)
#define TRACEE
#define TRACE_EXCEPT
#define ALWAYS_TRUE(p) p
#else
//!CLEANBEGIN
#ifndef DESIGN_ONLY
#undef assert
void DoAssert(wchar_t * Message, wchar_t * Filename, int LineNumber);
inline bool DoAlwaysTrue(bool Value, wchar_t * Message, wchar_t * Filename, int LineNumber)
{
  if (!Value)
  {
    DoAssert(Message, Filename, LineNumber);
  }
  return Value;
}
#define assert(p) ((p) ? (void)0 : DoAssert(TEXT(#p), TEXT(__FILE__), __LINE__))
#endif // ifndef DESIGN_ONLY
//!CLEANEND
#define CHECK(p) { bool __CHECK_RESULT__ = (p); assert(__CHECK_RESULT__); }
#define FAIL assert(false)
#define TRACE_EXCEPT_BEGIN try {
#define TRACE_EXCEPT_END } catch (Exception & TraceE) { TRACEFMT("E [%s]", TraceE.Message.c_str()); throw; }
#define TRACE_CATCH_ALL catch (Exception & TraceE)
#define TRACEE_(E) TRACEFMT(#E" [%s]", E.Message.c_str())
#define TRACEE TRACEE_(E)
#define TRACE_EXCEPT TRACEE_(TraceE)
#define ALWAYS_TRUE(p) DoAlwaysTrue(p, TEXT(#p), TEXT(__FILE__), __LINE__)
#define CLEAN_INLINE
#endif
#ifndef USEDPARAM
#define USEDPARAM(p) void(p);
#endif
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
uintptr_t inline GetCurrentVersionNumber() { return StrToVersionNumber(NETBOX_VERSION_NUMBER); }
//---------------------------------------------------------------------------
UnicodeString FormatBytes(__int64 Bytes, bool UseOrders = true);
//---------------------------------------------------------------------------
#endif
