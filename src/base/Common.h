
#pragma once

#include <Global.h>
#include <Exceptions.h>

#define EXCEPTION throw ExtException(nullptr, L"")
#if defined(__BORLANDC__)
// moved to Sysutils.hpp
#define THROWOSIFFALSE(C) { if (!(C)) RaiseLastOSError(); }
#define SAFE_DESTROY_EX(CLASS, OBJ) { CLASS * PObj = OBJ; OBJ = nullptr; delete PObj; }
#define SAFE_DESTROY(OBJ) SAFE_DESTROY_EX(TObject, OBJ)
#define NULL_TERMINATE(S) S[LENOF(S) - 1] = L'\0'
#define ASCOPY(dest, source) \
  { \
    AnsiString CopyBuf = source; \
    strncpy_s(dest, LENOF(dest), CopyBuf.c_str(), CopyBuf.Length()); \
    dest[LENOF(dest)-1] = '\0'; \
  }
#define SWAP(TYPE, FIRST, SECOND) \
  { TYPE __Backup = FIRST; FIRST = SECOND; SECOND = __Backup; }
#endif // defined(__BORLANDC__)

constexpr const wchar_t * PARENTDIRECTORY = L"..";
constexpr const wchar_t * THISDIRECTORY = L".";
constexpr const wchar_t * ROOTDIRECTORY = L"/";
constexpr const wchar_t * SLASH = L"/";
constexpr const wchar_t * BACKSLASH = L"\\";
constexpr const wchar_t * QUOTE = L"\'";
constexpr const wchar_t * DOUBLEQUOTE = L"\"";

constexpr const wchar_t * AnyMask = L"*.*";
//extern const wchar_t EngShortMonthNames[12][4];
constexpr const wchar_t EngShortMonthNames[12][4] =
{
  L"Jan", L"Feb", L"Mar", L"Apr", L"May", L"Jun",
  L"Jul", L"Aug", L"Sep", L"Oct", L"Nov", L"Dec"
};

// extern const char Bom[3];
constexpr const char * CONST_BOM = "\xEF\xBB\xBF";
constexpr const wchar_t TokenPrefix = L'%';
constexpr const wchar_t NoReplacement = static_cast<wchar_t>(0);
constexpr const wchar_t TokenReplacement = static_cast<wchar_t>(1);
constexpr const wchar_t * LocalInvalidChars = L"/\\:*?\"<>|";
constexpr const wchar_t * PasswordMask = L"***";
constexpr const wchar_t * Ellipsis = L"...";
constexpr const wchar_t * TitleSeparator = L" \u2013 "; // En-Dash
constexpr const wchar_t * OfficialPackage = L"MartinPrikryl.WinSCP";

constexpr const wchar_t * HttpProtocol = L"http";
constexpr const wchar_t * HttpsProtocol = L"https";
constexpr const wchar_t * ProtocolSeparator = L"://";

NB_CORE_EXPORT UnicodeString ReplaceChar(const UnicodeString & Str, wchar_t A, wchar_t B);
NB_CORE_EXPORT UnicodeString DeleteChar(const UnicodeString & Str, wchar_t C);
NB_CORE_EXPORT void PackStr(UnicodeString & Str);
NB_CORE_EXPORT void PackStr(RawByteString & Str);
NB_CORE_EXPORT void PackStr(AnsiString & Str);
NB_CORE_EXPORT void Shred(UnicodeString & Str);
NB_CORE_EXPORT void Shred(UTF8String & Str);
NB_CORE_EXPORT void Shred(AnsiString & Str);
NB_CORE_EXPORT void Shred(RawByteString & Str);
NB_CORE_EXPORT UnicodeString AnsiToString(const RawByteString & S);
NB_CORE_EXPORT UnicodeString AnsiToString(const char * S, int32_t Len);
NB_CORE_EXPORT UnicodeString MakeValidFileName(const UnicodeString & AFileName);
UnicodeString RootKeyToStr(HKEY RootKey, const UnicodeString & Default = EmptyStr);
UnicodeString BooleanToStr(bool B);
UnicodeString BooleanToEngStr(bool B);
NB_CORE_EXPORT UnicodeString DefaultStr(const UnicodeString & Str, const UnicodeString & Default);
NB_CORE_EXPORT UnicodeString CutToChar(UnicodeString & Str, wchar_t Ch, bool Trim);
NB_CORE_EXPORT UnicodeString CopyToChars(const UnicodeString & Str, int32_t & From, const UnicodeString & Chs, bool Trim,
  wchar_t * Delimiter = nullptr, bool DoubleDelimiterEscapes = false);
NB_CORE_EXPORT UnicodeString CopyToChar(const UnicodeString & Str, wchar_t Ch, bool Trim);
NB_CORE_EXPORT UnicodeString RemoveSuffix(const UnicodeString & Str, const UnicodeString & Suffix, bool RemoveNumbersAfterSuffix = false);
UnicodeString DelimitStr(const UnicodeString & Str, wchar_t Quote = L'"');
UnicodeString MidStr(const UnicodeString & Text, int32_t Start);
UnicodeString ShellQuoteStr(const UnicodeString & Str);
NB_CORE_EXPORT UnicodeString ExceptionLogString(Exception * E);
NB_CORE_EXPORT UnicodeString MainInstructions(const UnicodeString & S);
NB_CORE_EXPORT bool HasParagraphs(const UnicodeString & S);
NB_CORE_EXPORT UnicodeString MainInstructionsFirstParagraph(const UnicodeString & S);
NB_CORE_EXPORT bool ExtractMainInstructions(UnicodeString & S, UnicodeString & MainInstructions);
NB_CORE_EXPORT UnicodeString RemoveMainInstructionsTag(const UnicodeString & S);
NB_CORE_EXPORT UnicodeString UnformatMessage(const UnicodeString & S);
NB_CORE_EXPORT UnicodeString RemoveInteractiveMsgTag(const UnicodeString & S);
NB_CORE_EXPORT UnicodeString RemoveEmptyLines(const UnicodeString & S);
bool IsNumber(const UnicodeString & Str);
constexpr const wchar_t NormalizedFingerprintSeparator = L'-';
UnicodeString EncodeStrToBase64(const RawByteString & Str);
RawByteString DecodeBase64ToStr(const UnicodeString & Str);
UnicodeString Base64ToUrlSafe(const UnicodeString & S);
UnicodeString MD5ToUrlSafe(const UnicodeString & S);
bool SameChecksum(const UnicodeString & AChecksum1, const UnicodeString & AChecksum2, bool Base64);
UnicodeString SystemTemporaryDirectory();
UnicodeString GetShellFolderPath(int32_t CSIdl);
UnicodeString GetPersonalFolder();
UnicodeString GetDesktopFolder();
UnicodeString StripPathQuotes(const UnicodeString & Path);
NB_CORE_EXPORT UnicodeString AddQuotes(const UnicodeString & AStr);
NB_CORE_EXPORT UnicodeString AddPathQuotes(const UnicodeString & APath);
NB_CORE_EXPORT void SplitCommand(const UnicodeString & ACommand, UnicodeString & Program,
  UnicodeString & Params, UnicodeString & Dir);
NB_CORE_EXPORT UnicodeString ValidLocalFileName(const UnicodeString & AFileName);
NB_CORE_EXPORT UnicodeString ValidLocalFileName(
  const UnicodeString & AFileName, wchar_t AInvalidCharsReplacement,
  const UnicodeString & ATokenizibleChars, const UnicodeString & ALocalInvalidChars);
NB_CORE_EXPORT UnicodeString ExtractProgram(const UnicodeString & ACommand);
NB_CORE_EXPORT UnicodeString ExtractProgramName(const UnicodeString & ACommand);
NB_CORE_EXPORT UnicodeString FormatCommand(const UnicodeString & AProgram, const UnicodeString & AParams);
NB_CORE_EXPORT UnicodeString ExpandFileNameCommand(const UnicodeString & ACommand,
  const UnicodeString & AFileName);
NB_CORE_EXPORT void ReformatFileNameCommand(UnicodeString & ACommand);
NB_CORE_EXPORT UnicodeString EscapeParam(const UnicodeString & AParam);
NB_CORE_EXPORT UnicodeString EscapePuttyCommandParam(const UnicodeString & AParam);
NB_CORE_EXPORT UnicodeString StringsToParams(const TStrings * Strings);
NB_CORE_EXPORT UnicodeString ExpandEnvironmentVariables(const UnicodeString & Str);
NB_CORE_EXPORT bool SamePaths(const UnicodeString & APath1, const UnicodeString & APath2);
UnicodeString CombinePaths(const UnicodeString & APath1, const UnicodeString & APath2);
UnicodeString GetNormalizedPath(const UnicodeString & Path);
UnicodeString GetCanonicalPath(const UnicodeString & Path);
NB_CORE_EXPORT bool IsPathToSameFile(const UnicodeString & APath1, const UnicodeString & APath2);
NB_CORE_EXPORT int32_t CompareLogicalText(
  const UnicodeString & S1, const UnicodeString & S2, bool NaturalOrderNumericalSorting);
int32_t CompareNumber(int64_t Value1, int64_t Value2);
NB_CORE_EXPORT bool ContainsTextSemiCaseSensitive(const UnicodeString & Text, const UnicodeString & SubText);
NB_CORE_EXPORT bool IsReservedName(const UnicodeString & AFileName);
NB_CORE_EXPORT UnicodeString ApiPath(const UnicodeString & APath);
constexpr bool IsWideChar(wchar_t Ch) { return (Ch >= L'\x80'); }
NB_CORE_EXPORT UnicodeString DisplayableStr(const RawByteString & Str);
NB_CORE_EXPORT UnicodeString ByteToHex(uint8_t B, bool UpperCase = true);
NB_CORE_EXPORT UnicodeString BytesToHex(const uint8_t * B, uint32_t Length, bool UpperCase = true, wchar_t Separator = L'\0');
NB_CORE_EXPORT UnicodeString BytesToHex(const RawByteString & Str, bool UpperCase = true, wchar_t Separator = L'\0');
NB_CORE_EXPORT UnicodeString CharToHex(wchar_t Ch, bool UpperCase = true);
NB_CORE_EXPORT RawByteString HexToBytes(const UnicodeString & Hex);
NB_CORE_EXPORT uint8_t HexToByte(const UnicodeString & Hex);
NB_CORE_EXPORT bool IsLowerCaseLetter(wchar_t Ch);
NB_CORE_EXPORT bool IsUpperCaseLetter(wchar_t Ch);
NB_CORE_EXPORT bool IsLetter(wchar_t Ch);
NB_CORE_EXPORT bool IsDigit(wchar_t Ch);
NB_CORE_EXPORT bool IsHex(wchar_t Ch);
NB_CORE_EXPORT UnicodeString DecodeUrlChars(const UnicodeString & S);
NB_CORE_EXPORT UnicodeString EncodeUrlString(const UnicodeString & S);
NB_CORE_EXPORT UnicodeString EncodeUrlPath(const UnicodeString & S);
NB_CORE_EXPORT UnicodeString AppendUrlParams(const UnicodeString & AURL, const UnicodeString & Params);
NB_CORE_EXPORT UnicodeString ExtractFileNameFromUrl(const UnicodeString & Url);
bool IsDomainOrSubdomain(const UnicodeString & FullDomain, const UnicodeString & Domain);
NB_CORE_EXPORT bool RecursiveDeleteFile(const UnicodeString & AFileName, bool ToRecycleBin = false);
NB_CORE_EXPORT int32_t RecursiveDeleteFileChecked(const UnicodeString & AFileName, bool ToRecycleBin);
NB_CORE_EXPORT void DeleteFileChecked(const UnicodeString & AFileName);
NB_CORE_EXPORT uint32_t CancelAnswer(uint32_t Answers);
NB_CORE_EXPORT uint32_t AbortAnswer(uint32_t Answers);
NB_CORE_EXPORT uint32_t ContinueAnswer(uint32_t Answers);
NB_CORE_EXPORT UnicodeString LoadStr(int32_t Ident, uint32_t MaxLength = 0);
NB_CORE_EXPORT UnicodeString LoadStrFrom(HINSTANCE Module, int32_t Ident);
NB_CORE_EXPORT UnicodeString LoadStrPart(int32_t Ident, int32_t Part);
NB_CORE_EXPORT UnicodeString EscapeHotkey(const UnicodeString & Caption);
bool CutToken(UnicodeString & AStr, UnicodeString & AToken,
  UnicodeString * ARawToken = nullptr, UnicodeString * ASeparator = nullptr);
bool CutTokenEx(UnicodeString & Str, UnicodeString & Token,
  UnicodeString * RawToken = nullptr, UnicodeString * Separator = nullptr);
NB_CORE_EXPORT void AddToList(UnicodeString & List, const UnicodeString & Value, const UnicodeString & Delimiter);
void AddToShellFileListCommandLine(UnicodeString & List, const UnicodeString & Value);
bool IsWinVista();
bool IsWin7();
bool IsWin8();
bool IsWin10();
bool IsWin10Build(uint32_t BuildNumber);
bool IsWin11();
bool IsWine();
void EnableUWPTestMode();
bool IsUWP();
UnicodeString GetPackageName();
bool IsOfficialPackage();
// TLibModule * FindModule(void * Instance);
NB_CORE_EXPORT int64_t Round(double Number);
NB_CORE_EXPORT bool TryRelativeStrToDateTime(const UnicodeString & AStr, TDateTime & DateTime, bool Add);
NB_CORE_EXPORT bool TryStrToDateTimeStandard(const UnicodeString & S, TDateTime & Value);
NB_CORE_EXPORT bool TryStrToSize(const UnicodeString & ASizeStr, int64_t & ASize);
NB_CORE_EXPORT UnicodeString SizeToStr(int64_t ASize);
NB_CORE_EXPORT LCID GetDefaultLCID();
NB_CORE_EXPORT UnicodeString DefaultEncodingName();
NB_CORE_EXPORT UnicodeString WindowsProductName();
NB_CORE_EXPORT bool GetWindowsProductType(DWORD & Type);
NB_CORE_EXPORT int32_t GetWindowsBuild();
NB_CORE_EXPORT UnicodeString WindowsVersion();
NB_CORE_EXPORT UnicodeString WindowsVersionLong();
NB_CORE_EXPORT bool IsDirectoryWriteable(const UnicodeString & APath);
NB_CORE_EXPORT UnicodeString FormatNumber(int64_t Number);
NB_CORE_EXPORT UnicodeString FormatSize(int64_t ASize);
NB_CORE_EXPORT UnicodeString ExtractFileBaseName(const UnicodeString & APath);
NB_CORE_EXPORT TStringList * TextToStringList(const UnicodeString & Text);
NB_CORE_EXPORT UnicodeString StringsToText(TStrings * Strings);
TStringList * CommaTextToStringList(const UnicodeString & CommaText);
NB_CORE_EXPORT TStrings * CloneStrings(TStrings * Strings);
NB_CORE_EXPORT UnicodeString TrimVersion(const UnicodeString & Version);
NB_CORE_EXPORT UnicodeString FormatVersion(int32_t MajorVersion, int32_t MinorVersion, int32_t Patch);
NB_CORE_EXPORT TFormatSettings GetEngFormatSettings();
NB_CORE_EXPORT int32_t ParseShortEngMonthName(const UnicodeString & MonthStr);
// The defaults are equal to defaults of TStringList class (except for Sorted)
NB_CORE_EXPORT TStringList * CreateSortedStringList(bool CaseSensitive = false, TDuplicatesEnum Duplicates = dupIgnore);
UnicodeString FindIdent(const UnicodeString & Ident, TStrings * Idents);
UnicodeString GetTlsErrorStr(uint32_t Err);
UnicodeString GetTlsErrorStrs();
bool SameIdent(const UnicodeString & Ident1, const UnicodeString & Ident2);
UnicodeString GetTlsErrorStr(uint32_t Err);
UnicodeString GetTlsErrorStrs();
NB_CORE_EXPORT void CheckCertificate(const UnicodeString & Path);
using X509 = struct x509_st;
using EVP_PKEY = struct evp_pkey_st;
NB_CORE_EXPORT void ParseCertificate(const UnicodeString & Path,
  const UnicodeString & Passphrase, X509 *& Certificate, EVP_PKEY *& PrivateKey,
  bool & WrongPassphrase);
NB_CORE_EXPORT bool IsHttpUrl(const UnicodeString & S);
NB_CORE_EXPORT bool IsHttpOrHttpsUrl(const UnicodeString & S);
NB_CORE_EXPORT UnicodeString ChangeUrlProtocol(const UnicodeString & S, const UnicodeString & Protocol);
NB_CORE_EXPORT void LoadScriptFromFile(const UnicodeString & AFileName, TStrings * Lines, bool FallbackToAnsi = false);
NB_CORE_EXPORT UnicodeString StripEllipsis(const UnicodeString & S);
NB_CORE_EXPORT UnicodeString GetFileMimeType(const UnicodeString & AFileName);
NB_CORE_EXPORT bool IsRealFile(const UnicodeString & AFileName);
NB_CORE_EXPORT UnicodeString GetOSInfo();
NB_CORE_EXPORT UnicodeString GetEnvironmentInfo();
void SetStringValueEvenIfEmpty(TStrings * Strings, const UnicodeString & Name, const UnicodeString & Value);
UnicodeString GetAncestorProcessName(int32_t Levels = 1);
UnicodeString GetAncestorProcessNames();
[[noreturn]] void NotSupported();
[[noreturn]] void NotImplemented();
UnicodeString GetDividerLine();
TStrings * ProcessFeatures(TStrings * Features, const UnicodeString & FeaturesOverride);

struct NB_CORE_EXPORT TSearchRecSmart : public TSearchRec
{
public:
  TSearchRecSmart() noexcept;
  void Clear();
  TDateTime GetLastWriteTime() const;
  bool IsRealFile() const;
  bool IsDirectory() const;
  bool IsHidden() const;
private:
  mutable FILETIME FLastWriteTimeSource{};
  mutable TDateTime FLastWriteTime{};
};

using TProcessLocalFileEvent = nb::FastDelegate3<void,
  const UnicodeString & /*FileName*/, const TSearchRecSmart & /*Rec*/, void * /*Param*/>;

NB_CORE_EXPORT bool FileSearchRec(const UnicodeString & FileName, TSearchRec & Rec);
NB_CORE_EXPORT void CopySearchRec(const TSearchRec & Source, TSearchRec & Dest);
struct NB_CORE_EXPORT TSearchRecChecked : public TSearchRecSmart
{
  UnicodeString Path;
  UnicodeString Dir;
  bool Opened{false};
  UnicodeString GetFilePath() const;
};
struct NB_CORE_EXPORT TSearchRecOwned : public TSearchRecChecked
{
  virtual ~TSearchRecOwned() noexcept override;
  void Close();
};
NB_CORE_EXPORT DWORD FindCheck(DWORD Result, const UnicodeString & APath);
NB_CORE_EXPORT DWORD FindFirstUnchecked(const UnicodeString & APath, DWORD LocalFileAttrs, TSearchRecChecked & F);
NB_CORE_EXPORT DWORD FindFirstChecked(const UnicodeString & APath, DWORD LocalFileAttrs, TSearchRecChecked & F);
NB_CORE_EXPORT DWORD FindNextChecked(TSearchRecChecked & F);
DWORD FindNextUnchecked(TSearchRecChecked & F);
NB_CORE_EXPORT void ProcessLocalDirectory(const UnicodeString & ADirName,
  TProcessLocalFileEvent && CallBackFunc, void * Param = nullptr, DWORD FindAttrs = INVALID_FILE_ATTRIBUTES);
NB_CORE_EXPORT DWORD FileGetAttrFix(const UnicodeString & AFileName);

constexpr const wchar_t * DSTModeNames = L"Win;Unix;Keep";
enum TDSTMode
{
  dstmWin  = 0, //
  dstmUnix = 1, // adjust UTC time to Windows "bug"
  dstmKeep = 2,
};

NB_CORE_EXPORT bool UsesDaylightHack();
NB_CORE_EXPORT TDateTime EncodeDateVerbose(Word Year, Word Month, Word Day);
NB_CORE_EXPORT TDateTime EncodeTimeVerbose(Word Hour, Word Min, Word Sec, Word MSec);
NB_CORE_EXPORT double DSTDifferenceForTime(const TDateTime & DateTime);
NB_CORE_EXPORT TDateTime SystemTimeToDateTimeVerbose(const SYSTEMTIME & SystemTime);
NB_CORE_EXPORT TDateTime UnixToDateTime(int64_t TimeStamp, TDSTMode DSTMode);
NB_CORE_EXPORT TDateTime ConvertTimestampToUTC(const TDateTime & DateTime);
NB_CORE_EXPORT TDateTime ConvertTimestampFromUTC(const TDateTime & DateTime);
NB_CORE_EXPORT FILETIME DateTimeToFileTime(const TDateTime & DateTime, TDSTMode DSTMode);
NB_CORE_EXPORT TDateTime AdjustDateTimeFromUnix(const TDateTime & DateTime, TDSTMode DSTMode);
NB_CORE_EXPORT void UnifyDateTimePrecision(TDateTime & DateTime1, TDateTime & DateTime2);
NB_CORE_EXPORT TDateTime FileTimeToDateTime(const FILETIME & FileTime);
NB_CORE_EXPORT int64_t ConvertTimestampToUnix(const FILETIME & FileTime,
  TDSTMode DSTMode);
NB_CORE_EXPORT int64_t ConvertTimestampToUnixSafe(const FILETIME & FileTime,
  TDSTMode DSTMode);
NB_CORE_EXPORT UnicodeString FixedLenDateTimeFormat(const UnicodeString & Format);
NB_CORE_EXPORT UnicodeString StandardTimestamp(const TDateTime & DateTime);
NB_CORE_EXPORT UnicodeString StandardTimestamp();
NB_CORE_EXPORT UnicodeString StandardDatestamp();
NB_CORE_EXPORT UnicodeString FormatTimeZone(int32_t Sec);
NB_CORE_EXPORT UnicodeString GetTimeZoneLogString();
NB_CORE_EXPORT bool AdjustClockForDSTEnabled();
NB_CORE_EXPORT int32_t CompareFileTime(const TDateTime & T1, const TDateTime & T2);
NB_CORE_EXPORT int32_t TimeToMSec(const TDateTime & T);
NB_CORE_EXPORT int32_t TimeToSeconds(const TDateTime & T);
NB_CORE_EXPORT int32_t TimeToMinutes(const TDateTime & T);
UnicodeString FormatDateTimeSpan(const TDateTime & DateTime);
UnicodeString FormatRelativeTime(const TDateTime & ANow, const TDateTime & AThen, bool DateOnly);
NB_CORE_EXPORT TStrings * TlsCipherList();

#if defined(__BORLANDC__)

template<class MethodT>
MethodT MakeMethod(void * Data, void * Code)
{
  MethodT Method;
  ((TMethod*)&Method)->Data = Data;
  ((TMethod*)&Method)->Code = Code;
  return Method;
}

enum TAssemblyLanguage { alCSharp, alVBNET, alPowerShell };
extern const UnicodeString RtfPara;
extern const UnicodeString AssemblyNamespace;
extern const UnicodeString SessionClassName;
extern const UnicodeString TransferOptionsClassName;

UnicodeString RtfText(const UnicodeString & Text, bool Rtf = true);
UnicodeString RtfColor(int32_t Index);
UnicodeString RtfOverrideColorText(const UnicodeString & Text);
UnicodeString RtfColorItalicText(int32_t Color, const UnicodeString & Text);
UnicodeString RtfColorText(int32_t Color, const UnicodeString & Text);
UnicodeString RtfKeyword(const UnicodeString & Text);
UnicodeString RtfParameter(const UnicodeString & Text);
UnicodeString RtfString(const UnicodeString & Text);
UnicodeString RtfLink(const UnicodeString & Link, const UnicodeString & RtfText);
UnicodeString RtfSwitch(
  const UnicodeString & Name, const UnicodeString & Link, bool Rtf = true);
UnicodeString RtfSwitchValue(
  const UnicodeString & Name, const UnicodeString & Link, const UnicodeString & Value, bool Rtf = true);
UnicodeString RtfSwitch(
  const UnicodeString & Name, const UnicodeString & Link, const UnicodeString & Value, bool Rtf = true);
UnicodeString RtfSwitch(
  const UnicodeString & Name, const UnicodeString & Link, int32_t Value, bool Rtf = true);
UnicodeString RtfEscapeParam(const UnicodeString & Param, bool PowerShellEscape);
UnicodeString RtfRemoveHyperlinks(const UnicodeString & Text);
UnicodeString ScriptCommandLink(const UnicodeString & Command);
UnicodeString AssemblyBoolean(TAssemblyLanguage Language, bool Value);
UnicodeString AssemblyString(TAssemblyLanguage Language, const UnicodeString & S);
UnicodeString AssemblyCommentLine(TAssemblyLanguage Language, const UnicodeString & Text);
UnicodeString AssemblyPropertyRaw(
  TAssemblyLanguage Language, const UnicodeString & ClassName, const UnicodeString & Name,
  const UnicodeString & Value, bool Inline);
UnicodeString AssemblyProperty(
  TAssemblyLanguage Language, const UnicodeString & ClassName, const UnicodeString & Name,
  const UnicodeString & Type, const UnicodeString & Member, bool Inline);
UnicodeString AssemblyProperty(
  TAssemblyLanguage Language, const UnicodeString & ClassName, const UnicodeString & Name,
  const UnicodeString & Value, bool Inline);
UnicodeString AssemblyProperty(
  TAssemblyLanguage Language, const UnicodeString & ClassName, const UnicodeString & Name, int32_t Value, bool Inline);
UnicodeString AssemblyProperty(
  TAssemblyLanguage Language, const UnicodeString & ClassName, const UnicodeString & Name, bool Value, bool Inline);
UnicodeString RtfLibraryMethod(const UnicodeString & ClassName, const UnicodeString & MethodName, bool Inpage);
UnicodeString RtfLibraryClass(const UnicodeString & ClassName);
UnicodeString AssemblyVariableName(TAssemblyLanguage Language, const UnicodeString & ClassName);
UnicodeString AssemblyStatementSeparator(TAssemblyLanguage Language);
UnicodeString AssemblyVariableDeclaration(TAssemblyLanguage Language);
UnicodeString AssemblyNewClassInstance(
  TAssemblyLanguage Language, const UnicodeString & ClassName, bool Inline);
UnicodeString AssemblyNewClassInstanceStart(
  TAssemblyLanguage Language, const UnicodeString & ClassName, bool Inline);
UnicodeString AssemblyNewClassInstanceEnd(TAssemblyLanguage Language, bool Inline);
UnicodeString AssemblyAddRawSettings(
  TAssemblyLanguage Language, TStrings * RawSettings, const UnicodeString & ClassName,
  const UnicodeString & MethodName);

#endif // if 0

#pragma warning(push)
// #pragma warning(disable: 4512) // assignment operator could not be generated

// #include "Global.h"

template<typename T>
class TValueRestorer // : public TObject
{
public:
  TValueRestorer() = delete;
  TValueRestorer(const TValueRestorer &) = delete;
  TValueRestorer(T & Target) :
    FTarget(Target),
    FValue(Target)
  {
  }

  TValueRestorer(T & Target, const T & Value) :
    FTarget(Target),
    FValue(Target),
    FArmed(true)
  {
    FTarget = Value;
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
  T & FTarget;
  T FValue;
  bool FArmed{true};
};

class TAutoNestingCounter : public TValueRestorer<int32_t>
{
  using parent = TValueRestorer<int32_t>;
public:
  TAutoNestingCounter() = delete;
  inline explicit TAutoNestingCounter(int32_t & Target) :
    parent(Target, Target + 1)
  {
    DebugAssert(FValue >= 0);
  }

  inline ~TAutoNestingCounter()
  {
    DebugAssert(!FArmed || (FTarget == (FValue + 1)));
  }
};

class TAutoFlag : public TValueRestorer<bool>
{
  using parent = TValueRestorer<bool>;
public:
  TAutoFlag() = default;
  explicit TAutoFlag(bool & Target) :
    parent(Target, true)
  {
    DebugAssert(!FValue);
  }

  ~TAutoFlag()
  {
    DebugAssert(!FArmed || FTarget);
  }
};

#pragma warning(pop)

// #include <map>

template<class T1, class T2>
class BiDiMap
{
public:
  using TFirstToSecond = nb::map_t<T1, T2>;
  using const_iterator = typename TFirstToSecond::const_iterator;

  void Add(const T1 & Value1, const T2 & Value2)
  {
    FFirstToSecond.emplace(std::make_pair(Value1, Value2));
    FSecondToFirst.emplace(std::make_pair(Value2, Value1));
  }

  T1 LookupFirst(const T2 & Value2) const
  {
    const typename TSecondToFirst::const_iterator Iterator = FSecondToFirst.find(Value2);
    DebugAssert(Iterator != FSecondToFirst.end());
    return Iterator->second;
  }

  T2 LookupSecond(const T1 & Value1) const
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
  using TSecondToFirst = nb::map_t<T2, T1>;
  TSecondToFirst FSecondToFirst;
};

template<class T>
class TMulticastEvent
{
public:
  TMulticastEvent()
  {
    // noop
  }

  TMulticastEvent(const TMulticastEvent & Other) :
    FEventHandlers(Other.FEventHandlers)
  {
  }

  explicit TMulticastEvent(T EventHandler)
  {
    Add(EventHandler);
  }

  void Add(T EventHandler)
  {
    DebugAssert(EventHandler != nullptr);
    DebugAssert(Find(EventHandler) == FEventHandlers.end());
    FEventHandlers.emplace_back(EventHandler);
  }

  void Remove(T EventHandler)
  {
    typename TEventHandlers::const_iterator I = Find(EventHandler);
    if (DebugAlwaysTrue(I != FEventHandlers.end()))
    {
      FEventHandlers.erase(I);
    }
  }

  // #pragma warn -inl
  template<typename P>
  void Invoke(const P & p)
  {
    typename TEventHandlers::iterator I = FEventHandlers.begin();
    while (I != FEventHandlers.end())
    {
      (*I)(p);
      ++I;
    }
  }
  // #pragma warn .inl

  bool Contains(T EventHandler)
  {
    return (Find(EventHandler) != FEventHandlers.end());
  }

  bool Any() const
  {
    return (FEventHandlers.size() > 0);
  }

  bool operator ==(const TMulticastEvent<T> & Other) const
  {
    return (FEventHandlers == Other.FEventHandlers);
  }

  void operator =(const TMulticastEvent<T> & Other)
  {
    FEventHandlers = Other.FEventHandlers;
  }

private:
  using TEventHandlers = nb::vector_t<T>;
  TEventHandlers FEventHandlers;

  typename TEventHandlers::iterator Find(T EventHandler)
  {
    return std::find(FEventHandlers.begin(), FEventHandlers.end(), EventHandler);
  }

};

using TUnicodeStringVector = nb::vector_t<UnicodeString>;

namespace base {
//TODO: move to Sysutils.hpp
NB_CORE_EXPORT UnicodeString GetEnvironmentVariable(const UnicodeString & AEnvVarName);
NB_CORE_EXPORT UnicodeString FormatBytes(int64_t Bytes, bool UseOrders = true);
} // namespace base

constexpr const char * sLineBreak = "\n";

// Order of the values also define order of the buttons/answers on the prompts
// MessageDlg relies on these to be <= 0x0000FFFF
constexpr uint32_t qaYes      = 0x00000001;
// MessageDlg relies that answer do not conflict with mrCancel (=0x2)
constexpr const uint32_t qaNo       = 0x00000004;
constexpr const uint32_t qaOK       = 0x00000008;
constexpr const uint32_t qaCancel   = 0x00000010;
constexpr const uint32_t qaYesToAll = 0x00000020;
constexpr const uint32_t qaNoToAll  = 0x00000040;
constexpr const uint32_t qaAbort    = 0x00000080;
constexpr const uint32_t qaRetry    = 0x00000100;
constexpr const uint32_t qaIgnore   = 0x00000200;
constexpr const uint32_t qaSkip     = 0x00000400;
constexpr const uint32_t qaAll      = 0x00000800;
constexpr const uint32_t qaHelp     = 0x00001000;
constexpr const uint32_t qaReport   = 0x00002000;

constexpr const uint32_t qaFirst = qaYes;
constexpr const uint32_t qaLast  = qaReport;

constexpr const uint32_t qaNeverAskAgain = 0x00010000;

constexpr const int32_t qpFatalAbort           = 0x01;
constexpr const int32_t qpNeverAskAgainCheck   = 0x02;
constexpr const int32_t qpAllowContinueOnError = 0x04;
constexpr const int32_t qpIgnoreAbort          = 0x08;
constexpr const int32_t qpWaitInBatch          = 0x10;

inline void ThrowExtException() { throw ExtException(static_cast<Exception *>(nullptr), UnicodeString(L"")); }
NB_CORE_EXPORT bool CompareFileName(const UnicodeString & APath1, const UnicodeString & APath2);
NB_CORE_EXPORT bool ComparePaths(const UnicodeString & APath1, const UnicodeString & APath2);

// from RemoteFiles
namespace base {

NB_CORE_EXPORT bool IsUnixStyleWindowsPath(const UnicodeString & APath);
NB_CORE_EXPORT bool UnixIsAbsolutePath(const UnicodeString & APath);
NB_CORE_EXPORT UnicodeString UnixIncludeTrailingBackslash(const UnicodeString & APath);
NB_CORE_EXPORT UnicodeString UnixExcludeTrailingBackslash(const UnicodeString & APath, bool Simple = false);
NB_CORE_EXPORT UnicodeString SimpleUnixExcludeTrailingBackslash(const UnicodeString & APath);
NB_CORE_EXPORT UnicodeString UnixCombinePaths(const UnicodeString & APath1, const UnicodeString & APath2);
NB_CORE_EXPORT UnicodeString UnixExtractFileDir(const UnicodeString & APath);
NB_CORE_EXPORT UnicodeString UnixExtractFilePath(const UnicodeString & APath);
NB_CORE_EXPORT UnicodeString UnixExtractFileName(const UnicodeString & APath);
UnicodeString ExtractShortName(const UnicodeString & Path, bool Unix);
NB_CORE_EXPORT UnicodeString UnixExtractFileExt(const UnicodeString & APath);
NB_CORE_EXPORT Boolean UnixSamePath(const UnicodeString & APath1, const UnicodeString & APath2);
NB_CORE_EXPORT bool UnixIsChildPath(const UnicodeString & AParent, const UnicodeString & AChild);
NB_CORE_EXPORT bool ExtractCommonPath(const TStrings * AFiles, UnicodeString & APath);
NB_CORE_EXPORT bool UnixExtractCommonPath(const TStrings * AFiles, UnicodeString & APath);
NB_CORE_EXPORT UnicodeString ExtractFileName(const UnicodeString & APath, bool Unix = false);
NB_CORE_EXPORT bool IsUnixRootPath(const UnicodeString & APath);
NB_CORE_EXPORT bool IsUnixHiddenFile(const UnicodeString & AFileName);
NB_CORE_EXPORT UnicodeString AbsolutePath(const UnicodeString & Base, const UnicodeString & APath);
NB_CORE_EXPORT UnicodeString FromUnixPath(const UnicodeString & APath);
NB_CORE_EXPORT UnicodeString ToUnixPath(const UnicodeString & APath);
NB_CORE_EXPORT UnicodeString MinimizeName(const UnicodeString & AFileName, int32_t MaxLen, bool Unix);
NB_CORE_EXPORT UnicodeString MakeFileList(const TStrings * AFileList);
NB_CORE_EXPORT TDateTime ReduceDateTimePrecision(const TDateTime & ADateTime,
  TModificationFmt Precision);
NB_CORE_EXPORT TModificationFmt LessDateTimePrecision(
  TModificationFmt Precision1, TModificationFmt Precision2);
NB_CORE_EXPORT UnicodeString UserModificationStr(const TDateTime & DateTime,
  TModificationFmt Precision);
NB_CORE_EXPORT UnicodeString ModificationStr(const TDateTime & DateTime,
  TModificationFmt Precision);
int32_t GetPartialFileExtLen(const UnicodeString & FileName);
NB_CORE_EXPORT int32_t FakeFileImageIndex(const UnicodeString & AFileName, uint32_t Attrs = INVALID_FILE_ATTRIBUTES,
  const UnicodeString * TypeName = nullptr);
NB_CORE_EXPORT bool SameUserName(const UnicodeString & UserName1, const UnicodeString & UserName2);
NB_CORE_EXPORT UnicodeString FormatMultiFilesToOneConfirmation(const UnicodeString & ATarget, bool Unix);

} // namespace base

