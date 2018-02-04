
#pragma once

#include <rdestl/map.h>

#include <Global.h>
#include <Exceptions.h>
//---------------------------------------------------------------------------
extern const wchar_t EngShortMonthNames[12][4];
#define CONST_BOM "\xEF\xBB\xBF"
extern const wchar_t TokenPrefix;
extern const wchar_t NoReplacement;
extern const wchar_t TokenReplacement;
extern const UnicodeString LocalInvalidChars;
extern const UnicodeString PasswordMask;
extern const UnicodeString Ellipsis;

#define LOCAL_INVALID_CHARS "/\\:*?\"<>|"
#define PASSWORD_MASK "***"
#define sLineBreak L"\n"

#if 1
// Order of the values also define order of the buttons/answers on the prompts
// MessageDlg relies on these to be <= 0x0000FFFF
const uint32_t qaYes      = 0x00000001;
// MessageDlg relies that answer do not conflict with mrCancel (=0x2)
const uint32_t qaNo       = 0x00000004;
const uint32_t qaOK       = 0x00000008;
const uint32_t qaCancel   = 0x00000010;
const uint32_t qaYesToAll = 0x00000020;
const uint32_t qaNoToAll  = 0x00000040;
const uint32_t qaAbort    = 0x00000080;
const uint32_t qaRetry    = 0x00000100;
const uint32_t qaIgnore   = 0x00000200;
const uint32_t qaSkip     = 0x00000400;
const uint32_t qaAll      = 0x00000800;
const uint32_t qaHelp     = 0x00001000;
const uint32_t qaReport   = 0x00002000;

const uint32_t qaFirst = qaYes;
const uint32_t qaLast  = qaReport;

const uint32_t qaNeverAskAgain = 0x00010000;

const intptr_t qpFatalAbort           = 0x01;
const intptr_t qpNeverAskAgainCheck   = 0x02;
const intptr_t qpAllowContinueOnError = 0x04;
const intptr_t qpIgnoreAbort          = 0x08;
const intptr_t qpWaitInBatch          = 0x10;
#endif // #if 1

inline void ThrowExtException() { throw ExtException(static_cast<Exception *>(nullptr), UnicodeString(L"")); }
//---------------------------------------------------------------------------
extern const UnicodeString HttpProtocol;
extern const UnicodeString HttpsProtocol;
extern const UnicodeString ProtocolSeparator;
//---------------------------------------------------------------------------
NB_CORE_EXPORT UnicodeString ReplaceChar(const UnicodeString Str, wchar_t A, wchar_t B);
NB_CORE_EXPORT UnicodeString DeleteChar(const UnicodeString Str, wchar_t C);
NB_CORE_EXPORT void PackStr(UnicodeString &Str);
NB_CORE_EXPORT void PackStr(RawByteString &Str);
NB_CORE_EXPORT void PackStr(AnsiString &Str);
NB_CORE_EXPORT void Shred(UnicodeString &Str);
NB_CORE_EXPORT void Shred(UTF8String &Str);
NB_CORE_EXPORT void Shred(AnsiString &Str);
NB_CORE_EXPORT UnicodeString AnsiToString(const RawByteString S);
NB_CORE_EXPORT UnicodeString AnsiToString(const char *S, size_t Len);
NB_CORE_EXPORT UnicodeString MakeValidFileName(const UnicodeString AFileName);
NB_CORE_EXPORT UnicodeString RootKeyToStr(HKEY RootKey);
NB_CORE_EXPORT UnicodeString BooleanToStr(bool B);
NB_CORE_EXPORT UnicodeString BooleanToEngStr(bool B);
NB_CORE_EXPORT UnicodeString DefaultStr(const UnicodeString Str, const UnicodeString Default);
NB_CORE_EXPORT UnicodeString CutToChar(UnicodeString &Str, wchar_t Ch, bool Trim);
NB_CORE_EXPORT UnicodeString CopyToChars(const UnicodeString Str, intptr_t &From, const UnicodeString Chs, bool Trim,
  wchar_t *Delimiter = nullptr, bool DoubleDelimiterEscapes = false);
NB_CORE_EXPORT UnicodeString CopyToChar(const UnicodeString Str, wchar_t Ch, bool Trim);
NB_CORE_EXPORT UnicodeString DelimitStr(const UnicodeString Str, const UnicodeString Chars);
NB_CORE_EXPORT UnicodeString ShellDelimitStr(const UnicodeString Str, wchar_t Quote);
NB_CORE_EXPORT UnicodeString ExceptionLogString(Exception *E);
NB_CORE_EXPORT UnicodeString MainInstructions(const UnicodeString S);
NB_CORE_EXPORT bool HasParagraphs(const UnicodeString S);
NB_CORE_EXPORT UnicodeString MainInstructionsFirstParagraph(const UnicodeString S);
NB_CORE_EXPORT bool ExtractMainInstructions(UnicodeString &S, UnicodeString &MainInstructions);
NB_CORE_EXPORT UnicodeString RemoveMainInstructionsTag(const UnicodeString S);
NB_CORE_EXPORT UnicodeString UnformatMessage(const UnicodeString S);
NB_CORE_EXPORT UnicodeString RemoveInteractiveMsgTag(const UnicodeString S);
NB_CORE_EXPORT UnicodeString RemoveEmptyLines(const UnicodeString S);
NB_CORE_EXPORT bool IsNumber(const UnicodeString Str);
NB_CORE_EXPORT UnicodeString GetSystemTemporaryDirectory();
NB_CORE_EXPORT UnicodeString GetShellFolderPath(intptr_t CSIdl);
NB_CORE_EXPORT UnicodeString GetPersonalFolder();
NB_CORE_EXPORT UnicodeString GetDesktopFolder();
NB_CORE_EXPORT UnicodeString StripPathQuotes(const UnicodeString APath);
NB_CORE_EXPORT UnicodeString AddQuotes(const UnicodeString AStr);
NB_CORE_EXPORT UnicodeString AddPathQuotes(const UnicodeString APath);
NB_CORE_EXPORT void SplitCommand(const UnicodeString ACommand, UnicodeString &Program,
  UnicodeString &Params, UnicodeString &Dir);
NB_CORE_EXPORT UnicodeString ValidLocalFileName(const UnicodeString AFileName);
NB_CORE_EXPORT UnicodeString ValidLocalFileName(
  const UnicodeString AFileName, wchar_t AInvalidCharsReplacement,
  const UnicodeString ATokenizibleChars, const UnicodeString ALocalInvalidChars);
NB_CORE_EXPORT UnicodeString ExtractProgram(const UnicodeString ACommand);
NB_CORE_EXPORT UnicodeString ExtractProgramName(const UnicodeString ACommand);
NB_CORE_EXPORT UnicodeString FormatCommand(const UnicodeString AProgram, const UnicodeString AParams);
NB_CORE_EXPORT UnicodeString ExpandFileNameCommand(const UnicodeString ACommand,
  const UnicodeString AFileName);
NB_CORE_EXPORT void ReformatFileNameCommand(UnicodeString &ACommand);
NB_CORE_EXPORT UnicodeString EscapeParam(const UnicodeString AParam);
NB_CORE_EXPORT UnicodeString EscapePuttyCommandParam(const UnicodeString AParam);
NB_CORE_EXPORT UnicodeString ExpandEnvironmentVariables(const UnicodeString Str);
NB_CORE_EXPORT bool SamePaths(const UnicodeString APath1, const UnicodeString APath2);
NB_CORE_EXPORT bool IsPathToSameFile(const UnicodeString APath1, const UnicodeString APath2);
NB_CORE_EXPORT intptr_t CompareLogicalText(
  const UnicodeString S1, const UnicodeString S2, bool NaturalOrderNumericalSorting);
NB_CORE_EXPORT bool IsReservedName(const UnicodeString AFileName);
NB_CORE_EXPORT UnicodeString ApiPath(const UnicodeString APath);
NB_CORE_EXPORT UnicodeString DisplayableStr(const RawByteString Str);
NB_CORE_EXPORT UnicodeString ByteToHex(uint8_t B, bool UpperCase = true);
NB_CORE_EXPORT UnicodeString BytesToHex(const uint8_t *B, uintptr_t Length, bool UpperCase = true, wchar_t Separator = L'\0');
NB_CORE_EXPORT UnicodeString BytesToHex(const RawByteString Str, bool UpperCase = true, wchar_t Separator = L'\0');
NB_CORE_EXPORT UnicodeString CharToHex(wchar_t Ch, bool UpperCase = true);
NB_CORE_EXPORT RawByteString HexToBytes(const UnicodeString Hex);
NB_CORE_EXPORT uint8_t HexToByte(const UnicodeString Hex);
NB_CORE_EXPORT bool IsLowerCaseLetter(wchar_t Ch);
NB_CORE_EXPORT bool IsUpperCaseLetter(wchar_t Ch);
NB_CORE_EXPORT bool IsLetter(wchar_t Ch);
NB_CORE_EXPORT bool IsDigit(wchar_t Ch);
NB_CORE_EXPORT bool IsHex(wchar_t Ch);
NB_CORE_EXPORT UnicodeString DecodeUrlChars(const UnicodeString S);
NB_CORE_EXPORT UnicodeString EncodeUrlString(const UnicodeString S);
NB_CORE_EXPORT UnicodeString EncodeUrlPath(const UnicodeString S);
NB_CORE_EXPORT UnicodeString AppendUrlParams(const UnicodeString AURL, const UnicodeString Params);
NB_CORE_EXPORT UnicodeString ExtractFileNameFromUrl(const UnicodeString Url);
NB_CORE_EXPORT bool RecursiveDeleteFile(const UnicodeString AFileName, bool ToRecycleBin);
NB_CORE_EXPORT void RecursiveDeleteFileChecked(const UnicodeString AFileName, bool ToRecycleBin);
NB_CORE_EXPORT void DeleteFileChecked(const UnicodeString AFileName);
NB_CORE_EXPORT uint32_t CancelAnswer(uint32_t Answers);
NB_CORE_EXPORT uint32_t AbortAnswer(uint32_t Answers);
NB_CORE_EXPORT uint32_t ContinueAnswer(uint32_t Answers);
NB_CORE_EXPORT UnicodeString LoadStr(intptr_t Ident, uintptr_t MaxLength = 0);
NB_CORE_EXPORT UnicodeString LoadStrFrom(HINSTANCE Module, intptr_t Ident);
NB_CORE_EXPORT UnicodeString LoadStrPart(intptr_t Ident, intptr_t Part);
NB_CORE_EXPORT UnicodeString EscapeHotkey(const UnicodeString Caption);
NB_CORE_EXPORT bool CutToken(UnicodeString &AStr, UnicodeString &AToken,
  UnicodeString *ARawToken = nullptr, UnicodeString *ASeparator = nullptr);
NB_CORE_EXPORT bool CutTokenEx(UnicodeString &Str, UnicodeString &Token,
  UnicodeString *RawToken = nullptr, UnicodeString *Separator = nullptr);
NB_CORE_EXPORT void AddToList(UnicodeString &List, const UnicodeString Value, const UnicodeString Delimiter);
NB_CORE_EXPORT bool IsWinVista();
NB_CORE_EXPORT bool IsWin7();
NB_CORE_EXPORT bool IsWin8();
NB_CORE_EXPORT bool IsWin10();
NB_CORE_EXPORT bool IsWine();
__removed TLibModule * FindModule(void * Instance);
NB_CORE_EXPORT int64_t Round(double Number);
NB_CORE_EXPORT bool TryRelativeStrToDateTime(const UnicodeString AStr, TDateTime &DateTime, bool Add);
NB_CORE_EXPORT bool TryStrToSize(const UnicodeString SizeStr, int64_t &Size);
NB_CORE_EXPORT UnicodeString SizeToStr(int64_t Size);
NB_CORE_EXPORT LCID GetDefaultLCID();
NB_CORE_EXPORT UnicodeString DefaultEncodingName();
NB_CORE_EXPORT UnicodeString WindowsProductName();
NB_CORE_EXPORT bool GetWindowsProductType(DWORD &Type);
NB_CORE_EXPORT UnicodeString WindowsVersion();
NB_CORE_EXPORT UnicodeString WindowsVersionLong();
NB_CORE_EXPORT bool IsDirectoryWriteable(const UnicodeString APath);
NB_CORE_EXPORT UnicodeString FormatNumber(int64_t Number);
NB_CORE_EXPORT UnicodeString FormatSize(int64_t Size);
NB_CORE_EXPORT UnicodeString ExtractFileBaseName(const UnicodeString APath);
NB_CORE_EXPORT TStringList *TextToStringList(const UnicodeString Text);
NB_CORE_EXPORT UnicodeString StringsToText(TStrings *Strings);
NB_CORE_EXPORT TStrings *CloneStrings(TStrings *Strings);
NB_CORE_EXPORT UnicodeString TrimVersion(const UnicodeString Version);
NB_CORE_EXPORT UnicodeString FormatVersion(intptr_t MajorVersion, intptr_t MinorVersion, intptr_t Patch);
NB_CORE_EXPORT TFormatSettings GetEngFormatSettings();
NB_CORE_EXPORT intptr_t ParseShortEngMonthName(const UnicodeString MonthStr);
// The defaults are equal to defaults of TStringList class (except for Sorted)
NB_CORE_EXPORT TStringList *CreateSortedStringList(bool CaseSensitive = false, TDuplicatesEnum Duplicates = dupIgnore);
NB_CORE_EXPORT UnicodeString FindIdent(const UnicodeString Ident, TStrings *Idents);
NB_CORE_EXPORT void CheckCertificate(const UnicodeString Path);
typedef struct x509_st X509;
typedef struct evp_pkey_st EVP_PKEY;
NB_CORE_EXPORT void ParseCertificate(const UnicodeString Path,
  const UnicodeString Passphrase, X509 *&Certificate, EVP_PKEY *&PrivateKey,
  bool &WrongPassphrase);
NB_CORE_EXPORT bool IsHttpUrl(const UnicodeString S);
NB_CORE_EXPORT bool IsHttpOrHttpsUrl(const UnicodeString S);
NB_CORE_EXPORT UnicodeString ChangeUrlProtocol(const UnicodeString S, const UnicodeString Protocol);
NB_CORE_EXPORT void LoadScriptFromFile(const UnicodeString FileName, TStrings *Lines);
NB_CORE_EXPORT UnicodeString StripEllipsis(const UnicodeString S);
NB_CORE_EXPORT UnicodeString GetFileMimeType(const UnicodeString FileName);
//---------------------------------------------------------------------------
NB_CORE_EXPORT bool CompareFileName(const UnicodeString APath1, const UnicodeString APath2);
NB_CORE_EXPORT bool ComparePaths(const UnicodeString APath1, const UnicodeString APath2);
#if 0
typedef void (__closure* TProcessLocalFileEvent)
  (const UnicodeString FileName, const TSearchRec Rec, void * Param);
#endif // #if 0
typedef nb::FastDelegate3<void,
        UnicodeString /*FileName*/, const TSearchRec & /*Rec*/,
        void * /*Param*/> TProcessLocalFileEvent;

NB_CORE_EXPORT bool FileSearchRec(const UnicodeString AFileName, TSearchRec &Rec);

struct TSearchRecChecked : public TSearchRec
{
  UnicodeString Path;
};

NB_CORE_EXPORT DWORD FindCheck(DWORD Result, const UnicodeString APath);
NB_CORE_EXPORT DWORD FindFirstUnchecked(const UnicodeString APath, DWORD LocalFileAttrs, TSearchRecChecked &F);
NB_CORE_EXPORT DWORD FindFirstChecked(const UnicodeString APath, DWORD LocalFileAttrs, TSearchRecChecked &F);
NB_CORE_EXPORT DWORD FindNextChecked(TSearchRecChecked &F);
NB_CORE_EXPORT void ProcessLocalDirectory(UnicodeString ADirName,
  TProcessLocalFileEvent CallBackFunc, void *Param = nullptr, DWORD FindAttrs = INVALID_FILE_ATTRIBUTES);
NB_CORE_EXPORT DWORD FileGetAttrFix(const UnicodeString AFileName);
//---------------------------------------------------------------------------
extern const wchar_t *DSTModeNames;
enum TDSTMode
{
  dstmWin  = 0, //
  dstmUnix = 1, // adjust UTC time to Windows "bug"
  dstmKeep = 2,
};

NB_CORE_EXPORT bool UsesDaylightHack();
NB_CORE_EXPORT TDateTime EncodeDateVerbose(Word Year, Word Month, Word Day);
NB_CORE_EXPORT TDateTime EncodeTimeVerbose(Word Hour, Word Min, Word Sec, Word MSec);
NB_CORE_EXPORT double DSTDifferenceForTime(const TDateTime &DateTime);
NB_CORE_EXPORT TDateTime SystemTimeToDateTimeVerbose(const SYSTEMTIME &SystemTime);
NB_CORE_EXPORT TDateTime UnixToDateTime(int64_t TimeStamp, TDSTMode DSTMode);
NB_CORE_EXPORT TDateTime ConvertTimestampToUTC(const TDateTime &DateTime);
NB_CORE_EXPORT TDateTime ConvertTimestampFromUTC(const TDateTime &DateTime);
NB_CORE_EXPORT FILETIME DateTimeToFileTime(const TDateTime &DateTime, TDSTMode DSTMode);
NB_CORE_EXPORT TDateTime AdjustDateTimeFromUnix(const TDateTime &DateTime, TDSTMode DSTMode);
NB_CORE_EXPORT void UnifyDateTimePrecision(TDateTime &DateTime1, TDateTime &DateTime2);
NB_CORE_EXPORT TDateTime FileTimeToDateTime(const FILETIME &FileTime);
NB_CORE_EXPORT int64_t ConvertTimestampToUnix(const FILETIME &FileTime,
  TDSTMode DSTMode);
NB_CORE_EXPORT int64_t ConvertTimestampToUnixSafe(const FILETIME &FileTime,
  TDSTMode DSTMode);
NB_CORE_EXPORT UnicodeString FixedLenDateTimeFormat(const UnicodeString Format);
NB_CORE_EXPORT UnicodeString StandardTimestamp(const TDateTime &DateTime);
NB_CORE_EXPORT UnicodeString StandardTimestamp();
NB_CORE_EXPORT UnicodeString StandardDatestamp();
NB_CORE_EXPORT UnicodeString FormatTimeZone(intptr_t Sec);
NB_CORE_EXPORT UnicodeString GetTimeZoneLogString();
NB_CORE_EXPORT bool AdjustClockForDSTEnabled();
NB_CORE_EXPORT intptr_t CompareFileTime(const TDateTime &T1, const TDateTime &T2);
NB_CORE_EXPORT intptr_t TimeToMSec(const TDateTime &T);
NB_CORE_EXPORT intptr_t TimeToSeconds(const TDateTime &T);
NB_CORE_EXPORT intptr_t TimeToMinutes(const TDateTime &T);
UnicodeString FormatDateTimeSpan(const UnicodeString TimeFormat, TDateTime DateTime);
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
enum TAssemblyLanguage { alCSharp, alVBNET, alPowerShell };

#pragma warning(push)
#pragma warning(disable: 4512) // assignment operator could not be generated
//---------------------------------------------------------------------------
__removed #include "Global.h"
//---------------------------------------------------------------------------
template<class T>
class TValueRestorer // : public TObject
{
public:
  inline explicit TValueRestorer(T &Target, const T &Value) :
    FTarget(Target),
    FValue(Value),
    FArmed(true)
  {
  }

  inline explicit TValueRestorer(T &Target) :
    FTarget(Target),
    FValue(Target),
    FArmed(true)
  {
  }

  void Release()
  {
    if (FArmed)
    {
      FTarget = FValue;
      FArmed = false;
    }
  }

  inline ~TValueRestorer()
  {
    Release();
  }

protected:
  T &FTarget;
  T FValue;
  bool FArmed;
};
//---------------------------------------------------------------------------
class TAutoNestingCounter : public TValueRestorer<intptr_t>
{
public:
  inline explicit TAutoNestingCounter(intptr_t &Target) :
    TValueRestorer<intptr_t>(Target)
  {
    DebugAssert(Target >= 0);
    ++Target;
  }

  inline ~TAutoNestingCounter()
  {
    DebugAssert(!FArmed || (FTarget == (FValue + 1)));
  }
};
//---------------------------------------------------------------------------
class TAutoFlag : public TValueRestorer<bool>
{
public:
  explicit TAutoFlag(bool &Target) :
    TValueRestorer<bool>(Target)
  {
    DebugAssert(!Target);
    Target = true;
  }

  ~TAutoFlag()
  {
    DebugAssert(!FArmed || FTarget);
  }
};
#pragma warning(pop)
//---------------------------------------------------------------------------
__removed #include <map>
//---------------------------------------------------------------------------
template<class T1, class T2>
class BiDiMap
{
public:
  typedef rde::map<T1, T2> TFirstToSecond;
  typedef typename TFirstToSecond::const_iterator const_iterator;

  void Add(const T1 &Value1, const T2 &Value2)
  {
    FFirstToSecond.insert(std::make_pair(Value1, Value2));
    FSecondToFirst.insert(std::make_pair(Value2, Value1));
  }

  T1 LookupFirst(const T2 &Value2) const
  {
    typename TSecondToFirst::const_iterator Iterator = FSecondToFirst.find(Value2);
    DebugAssert(Iterator != FSecondToFirst.end());
    return Iterator->second;
  }

  T2 LookupSecond(const T1 &Value1) const
  {
    const_iterator Iterator = FFirstToSecond.find(Value1);
    DebugAssert(Iterator != FFirstToSecond.end());
    return Iterator->second;
  }

  const_iterator begin()
  {
    return FFirstToSecond.begin();
  }

  const_iterator end()
  {
    return FFirstToSecond.end();
  }

private:
  TFirstToSecond FFirstToSecond;
  typedef rde::map<T2, T1> TSecondToFirst;
  TSecondToFirst FSecondToFirst;
};
//---------------------------------------------------------------------------
typedef rde::vector<UnicodeString> TUnicodeStringVector;
//---------------------------------------------------------------------------

namespace base {
//TODO: move to Sysutils.hpp
NB_CORE_EXPORT UnicodeString FormatBytes(int64_t Bytes, bool UseOrders = true);
NB_CORE_EXPORT UnicodeString GetEnvVariable(UnicodeString AEnvVarName);

} // namespace base

// from  RemoteFiles.h

enum TModificationFmt
{
  mfNone,
  mfMDHM,
  mfMDY,
  mfFull,
};

namespace base {

NB_CORE_EXPORT bool IsUnixStyleWindowsPath(const UnicodeString APath);
NB_CORE_EXPORT bool UnixIsAbsolutePath(const UnicodeString APath);
NB_CORE_EXPORT UnicodeString UnixIncludeTrailingBackslash(const UnicodeString APath);
NB_CORE_EXPORT UnicodeString UnixExcludeTrailingBackslash(const UnicodeString APath, bool Simple = false);
NB_CORE_EXPORT UnicodeString SimpleUnixExcludeTrailingBackslash(const UnicodeString APath);
NB_CORE_EXPORT UnicodeString UnixCombinePaths(const UnicodeString APath1, const UnicodeString APath2);
NB_CORE_EXPORT UnicodeString UnixExtractFileDir(const UnicodeString APath);
NB_CORE_EXPORT UnicodeString UnixExtractFilePath(const UnicodeString APath);
NB_CORE_EXPORT UnicodeString UnixExtractFileName(const UnicodeString APath);
NB_CORE_EXPORT UnicodeString UnixExtractFileExt(const UnicodeString APath);
NB_CORE_EXPORT Boolean UnixSamePath(const UnicodeString APath1, const UnicodeString APath2);
NB_CORE_EXPORT bool UnixIsChildPath(const UnicodeString AParent, const UnicodeString AChild);
NB_CORE_EXPORT bool ExtractCommonPath(const TStrings *AFiles, UnicodeString &APath);
NB_CORE_EXPORT bool UnixExtractCommonPath(const TStrings *AFiles, UnicodeString &APath);
NB_CORE_EXPORT UnicodeString ExtractFileName(const UnicodeString APath, bool Unix);
NB_CORE_EXPORT bool IsUnixRootPath(const UnicodeString APath);
NB_CORE_EXPORT UnicodeString GetEnvVariable(const UnicodeString AEnvVarName);
NB_CORE_EXPORT bool IsUnixHiddenFile(const UnicodeString APath);
NB_CORE_EXPORT UnicodeString AbsolutePath(const UnicodeString Base, const UnicodeString APath);
NB_CORE_EXPORT UnicodeString FromUnixPath(const UnicodeString APath);
NB_CORE_EXPORT UnicodeString ToUnixPath(const UnicodeString APath);
NB_CORE_EXPORT UnicodeString MinimizeName(const UnicodeString AFileName, intptr_t MaxLen, bool Unix);
NB_CORE_EXPORT UnicodeString MakeFileList(const TStrings *AFileList);
NB_CORE_EXPORT TDateTime ReduceDateTimePrecision(const TDateTime &ADateTime,
  TModificationFmt Precision);
NB_CORE_EXPORT TModificationFmt LessDateTimePrecision(
  TModificationFmt Precision1, TModificationFmt Precision2);
NB_CORE_EXPORT UnicodeString UserModificationStr(const TDateTime &DateTime,
  TModificationFmt Precision);
NB_CORE_EXPORT UnicodeString ModificationStr(const TDateTime &DateTime,
  TModificationFmt Precision);
NB_CORE_EXPORT int FakeFileImageIndex(const UnicodeString AFileName, uint32_t Attrs = INVALID_FILE_ATTRIBUTES,
  UnicodeString *TypeName = nullptr);
NB_CORE_EXPORT bool SameUserName(const UnicodeString UserName1, const UnicodeString UserName2);
NB_CORE_EXPORT UnicodeString FormatMultiFilesToOneConfirmation(const UnicodeString ATarget, bool Unix);

} // namespace base
