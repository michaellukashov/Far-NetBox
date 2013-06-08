//---------------------------------------------------------------------------
#ifndef CommonH
#define CommonH

#include <CoreDefs.hpp>
#include <headers.hpp>

#include "Exceptions.h"
//---------------------------------------------------------------------------
#define EXCEPTION throw ExtException((Exception* )NULL, UnicodeString(L""))
#define THROWOSIFFALSE(C) if (!(C)) { RaiseLastOSError(); }
#define SAFE_DESTROY_EX(CLASS, OBJ) { CLASS * PObj = OBJ; OBJ = NULL; delete PObj; }
#define SAFE_DESTROY(OBJ) SAFE_DESTROY_EX(TObject, OBJ)
#define ASCOPY(dest, source) \
  { \
    AnsiString CopyBuf = ::W2MB(source).c_str(); \
    strncpy(dest, CopyBuf.c_str(), LENOF(dest)); \
    dest[LENOF(dest)-1] = '\0'; \
  }
#if defined(__BORLANDC__)
#define FORMAT(S, F) Format(S, ARRAYOFCONST(F))
#define FMTLOAD(I, F) FmtLoadStr(I, ARRAYOFCONST(F))
#else
#define FORMAT(S, ...) ::Format(S, ##__VA_ARGS__)
#define FMTLOAD(I, ...) ::FmtLoadStr(I, ##__VA_ARGS__)
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
UnicodeString CopyToChars(const UnicodeString & Str, intptr_t & From,
  const UnicodeString & Chs, bool Trim,
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
uintptr_t CancelAnswer(uintptr_t Answers);
uintptr_t AbortAnswer(uintptr_t Answers);
uintptr_t ContinueAnswer(uintptr_t Answers);
UnicodeString LoadStr(int Ident, intptr_t MaxLength = 0);
UnicodeString LoadStrPart(int Ident, int Part);
UnicodeString EscapeHotkey(const UnicodeString & Caption);
bool CutToken(UnicodeString & Str, UnicodeString & Token,
  UnicodeString * RawToken = NULL);
void AddToList(UnicodeString & List, const UnicodeString & Value, const UnicodeString & Delimiter);
bool Is2000();
bool IsWin7();
bool IsExactly2008R2();
#if defined(__BORLANDC__)
TLibModule * FindModule(void * Instance);
#endif
__int64 Round(double Number);
bool TryRelativeStrToDateTime(const UnicodeString & S, TDateTime & DateTime);
LCID GetDefaultLCID();
UnicodeString DefaultEncodingName();
UnicodeString WindowsProductName();
bool IsDirectoryWriteable(const UnicodeString & Path);
UnicodeString FormatNumber(__int64 Size);
UnicodeString FormatSize(__int64 Size);
//---------------------------------------------------------------------------
DEFINE_CALLBACK_TYPE3(TProcessLocalFileEvent, void,
  const UnicodeString & /* FileName */, const TSearchRec & /* Rec */, void * /* Param */);
bool FileSearchRec(const UnicodeString & FileName, TSearchRec & Rec);
DWORD FindCheck(DWORD Result);
DWORD FindFirstChecked(const UnicodeString & Path, DWORD LocalFileAttrs, TSearchRec & F);
DWORD FindNextChecked(TSearchRec & F);
void ProcessLocalDirectory(const UnicodeString & DirName,
  TProcessLocalFileEvent CallBackFunc, void * Param = NULL, DWORD FindAttrs = INVALID_FILE_ATTRIBUTES);
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
TDateTime ConvertTimestampToUTC(const TDateTime & DateTime);
TDateTime ConvertTimestampFromUTC(const TDateTime & DateTime);
FILETIME DateTimeToFileTime(const TDateTime & DateTime, TDSTMode DSTMode);
TDateTime AdjustDateTimeFromUnix(const TDateTime & DateTime, TDSTMode DSTMode);
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
intptr_t CompareFileTime(const TDateTime & T1, const TDateTime & T2);
intptr_t TimeToMSec(const TDateTime & T);
intptr_t TimeToMinutes(const TDateTime & T);
//---------------------------------------------------------------------------
#if defined(__BORLANDC__)
template<class MethodT>
MethodT MakeMethod(void * Data, void * Code)
{
  MethodT Method;
  ((TMethod*)&Method)->Data = Data;
  ((TMethod*)&Method)->Code = Code;
  return Method;
}
#endif
//---------------------------------------------------------------------------
class TGuard : public TObject
{
NB_DISABLE_COPY(TGuard)
public:
  explicit TGuard(TCriticalSection * ACriticalSection);
  ~TGuard();

private:
  TCriticalSection * FCriticalSection;
};
//---------------------------------------------------------------------------
class TUnguard : public TObject
{
NB_DISABLE_COPY(TUnguard)
public:
  explicit TUnguard(TCriticalSection * ACriticalSection);
  ~TUnguard();

private:
  TCriticalSection * FCriticalSection;
};
//---------------------------------------------------------------------------
#undef TEXT
#define TEXT(x) const_cast<wchar_t *>(MB2W(x).c_str())
#define CALLSTACK 
#define CCALLSTACK(TRACING) 
#define TRACING 
#undef TRACE
#define TRACE(MESSAGE) 
#define TRACEFMT(MESSAGE, ...) 
#define CTRACE(TRACING, MESSAGE) 
#define CTRACEFMT(TRACING, MESSAGE, ...) 
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
#define ALWAYS_FALSE(p) p
#define NOT_NULL(P) P
#else
#define CHECK(p) { bool __CHECK_RESULT__ = (p); assert(__CHECK_RESULT__); }
#define FAIL assert(false)
#define ALWAYS_TRUE(p) p
#define ALWAYS_FALSE(p) p
#define NOT_NULL(P) P
#define CLEAN_INLINE
#endif
#ifndef USEDPARAM
#define USEDPARAM(p) void(p);
#endif
//---------------------------------------------------------------------------
template<class T>
class TValueRestorer : public TObject
{
public:
  inline explicit TValueRestorer(T & Target, const T & Value) :
    FTarget(Target),
    FValue(Value)
  {
  }

  inline ~TValueRestorer()
  {
    FTarget = FValue;
  }

protected:
  T & FTarget;
  T FValue;
};
//---------------------------------------------------------------------------
class TBoolRestorer : TValueRestorer<bool>
{
public:
  inline explicit TBoolRestorer(bool & Target) :
    TValueRestorer<bool>(Target, !Target)
  {
  }
};
//---------------------------------------------------------------------------
class TAutoNestingCounter : TValueRestorer<int>
{
public:
  inline explicit TAutoNestingCounter(int & Target) :
    TValueRestorer<int>(Target, Target)
  {
    assert(Target >= 0);
    ++Target;
  }

  inline ~TAutoNestingCounter()
  {
    assert(FTarget == (FValue + 1));
  }
};
//---------------------------------------------------------------------------
UnicodeString FormatBytes(__int64 Bytes, bool UseOrders = true);
//---------------------------------------------------------------------------
#endif
