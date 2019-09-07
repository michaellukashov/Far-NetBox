
#pragma once

#include <rdestl/map.h>

#include <Global.h>
#include <Exceptions.h>

//---------------------------------------------------------------------------
#if 0
#define EXCEPTION throw ExtException(nullptr, L"")
#define THROWOSIFFALSE(C) { if (!(C)) RaiseLastOSError(); }
#define SAFE_DESTROY_EX(CLASS, OBJ) { CLASS * PObj = OBJ; OBJ = nullptr; delete PObj; }
#define SAFE_DESTROY(OBJ) SAFE_DESTROY_EX(TObject, OBJ)
#define NULL_TERMINATE(S) S[LENOF(S) - 1] = L'\0'
#define ASCOPY(dest, source) \
  { \
    AnsiString CopyBuf = source; \
    strncpy(dest, CopyBuf.c_str(), LENOF(dest)); \
    dest[LENOF(dest)-1] = '\0'; \
  }
#define SWAP(TYPE, FIRST, SECOND) \
  { TYPE __Backup = FIRST; FIRST = SECOND; SECOND = __Backup; }
#endif // if 0
//---------------------------------------------------------------------------
#define PARENTDIRECTORY L".."
#define THISDIRECTORY L"."
//---------------------------------------------------------------------------
extern const wchar_t EngShortMonthNames[12][4];
__removed extern const char Bom[3];
#define CONST_BOM "\xEF\xBB\xBF"
extern const wchar_t TokenPrefix;
extern const wchar_t NoReplacement;
extern const wchar_t TokenReplacement;
extern UnicodeString LocalInvalidChars;
extern UnicodeString PasswordMask;
extern UnicodeString Ellipsis;
extern UnicodeString EmptyString;
//---------------------------------------------------------------------------
extern UnicodeString HttpProtocol;
extern UnicodeString HttpsProtocol;
extern UnicodeString ProtocolSeparator;
//---------------------------------------------------------------------------
NB_CORE_EXPORT UnicodeString ReplaceChar(UnicodeString Str, wchar_t A, wchar_t B);
NB_CORE_EXPORT UnicodeString DeleteChar(UnicodeString Str, wchar_t C);
NB_CORE_EXPORT void PackStr(UnicodeString &Str);
NB_CORE_EXPORT void PackStr(RawByteString &Str);
NB_CORE_EXPORT void PackStr(AnsiString &Str);
NB_CORE_EXPORT void Shred(UnicodeString &Str);
NB_CORE_EXPORT void Shred(UTF8String &Str);
NB_CORE_EXPORT void Shred(AnsiString &Str);
NB_CORE_EXPORT void Shred(RawByteString &Str);
NB_CORE_EXPORT UnicodeString AnsiToString(RawByteString S);
NB_CORE_EXPORT UnicodeString AnsiToString(const char *S, size_t Len);
NB_CORE_EXPORT UnicodeString MakeValidFileName(UnicodeString AFileName);
NB_CORE_EXPORT UnicodeString RootKeyToStr(HKEY RootKey);
NB_CORE_EXPORT UnicodeString BooleanToStr(bool B);
NB_CORE_EXPORT UnicodeString BooleanToEngStr(bool B);
NB_CORE_EXPORT UnicodeString DefaultStr(UnicodeString Str, UnicodeString Default);
NB_CORE_EXPORT UnicodeString CutToChar(UnicodeString &Str, wchar_t Ch, bool Trim);
NB_CORE_EXPORT UnicodeString CopyToChars(UnicodeString Str, intptr_t &From, UnicodeString Chs, bool Trim,
  wchar_t *Delimiter = nullptr, bool DoubleDelimiterEscapes = false);
NB_CORE_EXPORT UnicodeString CopyToChar(UnicodeString Str, wchar_t Ch, bool Trim);
NB_CORE_EXPORT UnicodeString DelimitStr(UnicodeString Str, UnicodeString Chars);
NB_CORE_EXPORT UnicodeString ShellDelimitStr(UnicodeString Str, wchar_t Quote);
NB_CORE_EXPORT UnicodeString ExceptionLogString(Exception *E);
NB_CORE_EXPORT UnicodeString MainInstructions(UnicodeString S);
NB_CORE_EXPORT bool HasParagraphs(UnicodeString S);
NB_CORE_EXPORT UnicodeString MainInstructionsFirstParagraph(UnicodeString S);
NB_CORE_EXPORT bool ExtractMainInstructions(UnicodeString &S, UnicodeString &MainInstructions);
NB_CORE_EXPORT UnicodeString RemoveMainInstructionsTag(UnicodeString S);
NB_CORE_EXPORT UnicodeString UnformatMessage(UnicodeString S);
NB_CORE_EXPORT UnicodeString RemoveInteractiveMsgTag(UnicodeString S);
NB_CORE_EXPORT UnicodeString RemoveEmptyLines(UnicodeString S);
NB_CORE_EXPORT UnicodeString NormalizeString(UnicodeString S);
NB_CORE_EXPORT bool IsNumber(UnicodeString Str);
NB_CORE_EXPORT UnicodeString GetSystemTemporaryDirectory();
NB_CORE_EXPORT UnicodeString GetShellFolderPath(intptr_t CSIdl);
NB_CORE_EXPORT UnicodeString GetPersonalFolder();
NB_CORE_EXPORT UnicodeString GetDesktopFolder();
NB_CORE_EXPORT UnicodeString StripPathQuotes(UnicodeString APath);
NB_CORE_EXPORT UnicodeString AddQuotes(UnicodeString AStr);
NB_CORE_EXPORT UnicodeString AddPathQuotes(UnicodeString APath);
NB_CORE_EXPORT void SplitCommand(UnicodeString ACommand, UnicodeString &Program,
  UnicodeString &Params, UnicodeString &Dir);
NB_CORE_EXPORT UnicodeString ValidLocalFileName(UnicodeString AFileName);
NB_CORE_EXPORT UnicodeString ValidLocalFileName(
  UnicodeString AFileName, wchar_t AInvalidCharsReplacement,
  UnicodeString ATokenizibleChars, UnicodeString ALocalInvalidChars);
NB_CORE_EXPORT UnicodeString ExtractProgram(UnicodeString ACommand);
NB_CORE_EXPORT UnicodeString ExtractProgramName(UnicodeString ACommand);
NB_CORE_EXPORT UnicodeString FormatCommand(UnicodeString AProgram, UnicodeString AParams);
NB_CORE_EXPORT UnicodeString ExpandFileNameCommand(UnicodeString ACommand,
  UnicodeString AFileName);
NB_CORE_EXPORT void ReformatFileNameCommand(UnicodeString &ACommand);
NB_CORE_EXPORT UnicodeString EscapeParam(UnicodeString AParam);
NB_CORE_EXPORT UnicodeString EscapePuttyCommandParam(UnicodeString AParam);
NB_CORE_EXPORT UnicodeString StringsToParams(TStrings * Strings);
NB_CORE_EXPORT UnicodeString ExpandEnvironmentVariables(UnicodeString Str);
NB_CORE_EXPORT bool SamePaths(UnicodeString APath1, UnicodeString APath2);
NB_CORE_EXPORT bool IsPathToSameFile(UnicodeString APath1, UnicodeString APath2);
NB_CORE_EXPORT intptr_t CompareLogicalText(
  UnicodeString S1, UnicodeString S2, bool NaturalOrderNumericalSorting);
NB_CORE_EXPORT bool ContainsTextSemiCaseSensitive(UnicodeString Text, UnicodeString SubText);
NB_CORE_EXPORT bool IsReservedName(UnicodeString AFileName);
NB_CORE_EXPORT UnicodeString ApiPath(UnicodeString APath);
NB_CORE_EXPORT UnicodeString DisplayableStr(RawByteString Str);
NB_CORE_EXPORT UnicodeString ByteToHex(uint8_t B, bool UpperCase = true);
NB_CORE_EXPORT UnicodeString BytesToHex(const uint8_t *B, uintptr_t Length, bool UpperCase = true, wchar_t Separator = L'\0');
NB_CORE_EXPORT UnicodeString BytesToHex(RawByteString Str, bool UpperCase = true, wchar_t Separator = L'\0');
NB_CORE_EXPORT UnicodeString CharToHex(wchar_t Ch, bool UpperCase = true);
NB_CORE_EXPORT RawByteString HexToBytes(UnicodeString Hex);
NB_CORE_EXPORT uint8_t HexToByte(UnicodeString Hex);
NB_CORE_EXPORT bool IsLowerCaseLetter(wchar_t Ch);
NB_CORE_EXPORT bool IsUpperCaseLetter(wchar_t Ch);
NB_CORE_EXPORT bool IsLetter(wchar_t Ch);
NB_CORE_EXPORT bool IsDigit(wchar_t Ch);
NB_CORE_EXPORT bool IsHex(wchar_t Ch);
NB_CORE_EXPORT UnicodeString DecodeUrlChars(UnicodeString S);
NB_CORE_EXPORT UnicodeString EncodeUrlString(UnicodeString S, UnicodeString DoNotEncode = UnicodeString());
NB_CORE_EXPORT UnicodeString EncodeUrlPath(UnicodeString S);
NB_CORE_EXPORT UnicodeString AppendUrlParams(UnicodeString AURL, UnicodeString Params);
NB_CORE_EXPORT UnicodeString ExtractFileNameFromUrl(UnicodeString Url);
NB_CORE_EXPORT bool RecursiveDeleteFile(UnicodeString AFileName, bool ToRecycleBin);
NB_CORE_EXPORT intptr_t RecursiveDeleteFileChecked(UnicodeString AFileName, bool ToRecycleBin);
NB_CORE_EXPORT void DeleteFileChecked(UnicodeString AFileName);
NB_CORE_EXPORT uint32_t CancelAnswer(uint32_t Answers);
NB_CORE_EXPORT uint32_t AbortAnswer(uint32_t Answers);
NB_CORE_EXPORT uint32_t ContinueAnswer(uint32_t Answers);
NB_CORE_EXPORT UnicodeString LoadStr(intptr_t Ident, uintptr_t MaxLength = 0);
NB_CORE_EXPORT UnicodeString LoadStrFrom(HINSTANCE Module, intptr_t Ident);
NB_CORE_EXPORT UnicodeString LoadStrPart(intptr_t Ident, intptr_t Part);
NB_CORE_EXPORT UnicodeString EscapeHotkey(UnicodeString Caption);
NB_CORE_EXPORT bool CutToken(UnicodeString &AStr, UnicodeString &AToken,
  UnicodeString *ARawToken = nullptr, UnicodeString *ASeparator = nullptr);
NB_CORE_EXPORT bool CutTokenEx(UnicodeString &Str, UnicodeString &Token,
  UnicodeString *RawToken = nullptr, UnicodeString *Separator = nullptr);
NB_CORE_EXPORT void AddToList(UnicodeString &List, UnicodeString Value, UnicodeString Delimiter);
NB_CORE_EXPORT bool IsWinVista();
NB_CORE_EXPORT bool IsWin7();
NB_CORE_EXPORT bool IsWin8();
NB_CORE_EXPORT bool IsWin10();
NB_CORE_EXPORT bool IsWine();
NB_CORE_EXPORT bool IsUWP();
__removed TLibModule * FindModule(void * Instance);
NB_CORE_EXPORT int64_t Round(double Number);
NB_CORE_EXPORT bool TryRelativeStrToDateTime(UnicodeString AStr, TDateTime &DateTime, bool Add);
NB_CORE_EXPORT bool TryStrToDateTimeStandard(UnicodeString S, TDateTime &Value);
NB_CORE_EXPORT bool TryStrToSize(UnicodeString ASizeStr, int64_t &ASize);
NB_CORE_EXPORT UnicodeString SizeToStr(int64_t ASize);
NB_CORE_EXPORT LCID GetDefaultLCID();
NB_CORE_EXPORT UnicodeString DefaultEncodingName();
NB_CORE_EXPORT UnicodeString WindowsProductName();
NB_CORE_EXPORT bool GetWindowsProductType(DWORD &Type);
NB_CORE_EXPORT int GetWindowsBuild();
NB_CORE_EXPORT UnicodeString WindowsVersion();
NB_CORE_EXPORT UnicodeString WindowsVersionLong();
NB_CORE_EXPORT bool IsDirectoryWriteable(UnicodeString APath);
NB_CORE_EXPORT UnicodeString FormatNumber(int64_t Number);
NB_CORE_EXPORT UnicodeString FormatSize(int64_t ASize);
NB_CORE_EXPORT UnicodeString ExtractFileBaseName(UnicodeString APath);
NB_CORE_EXPORT TStringList *TextToStringList(UnicodeString Text);
NB_CORE_EXPORT UnicodeString StringsToText(TStrings *Strings);
NB_CORE_EXPORT TStrings *CloneStrings(TStrings *Strings);
NB_CORE_EXPORT UnicodeString TrimVersion(UnicodeString Version);
NB_CORE_EXPORT UnicodeString FormatVersion(intptr_t MajorVersion, intptr_t MinorVersion, intptr_t Patch);
NB_CORE_EXPORT TFormatSettings GetEngFormatSettings();
NB_CORE_EXPORT intptr_t ParseShortEngMonthName(UnicodeString MonthStr);
// The defaults are equal to defaults of TStringList class (except for Sorted)
NB_CORE_EXPORT TStringList *CreateSortedStringList(bool CaseSensitive = false, TDuplicatesEnum Duplicates = dupIgnore);
NB_CORE_EXPORT UnicodeString FindIdent(UnicodeString Ident, TStrings *Idents);
NB_CORE_EXPORT void CheckCertificate(UnicodeString Path);
typedef struct x509_st X509;
typedef struct evp_pkey_st EVP_PKEY;
NB_CORE_EXPORT void ParseCertificate(UnicodeString Path,
  UnicodeString Passphrase, X509 *&Certificate, EVP_PKEY *&PrivateKey,
  bool &WrongPassphrase);
NB_CORE_EXPORT bool IsHttpUrl(UnicodeString S);
NB_CORE_EXPORT bool IsHttpOrHttpsUrl(UnicodeString S);
NB_CORE_EXPORT UnicodeString ChangeUrlProtocol(UnicodeString S, UnicodeString Protocol);
NB_CORE_EXPORT void LoadScriptFromFile(UnicodeString AFileName, TStrings *Lines);
NB_CORE_EXPORT UnicodeString StripEllipsis(UnicodeString S);
NB_CORE_EXPORT UnicodeString GetFileMimeType(UnicodeString AFileName);
NB_CORE_EXPORT bool IsRealFile(UnicodeString AFileName);
NB_CORE_EXPORT UnicodeString GetOSInfo();
NB_CORE_EXPORT UnicodeString GetEnvironmentInfo();
//---------------------------------------------------------------------------
struct NB_CORE_EXPORT TSearchRecSmart : public TSearchRec
{
public:
  TSearchRecSmart() noexcept;
  TDateTime GetLastWriteTime() const;
  bool IsRealFile() const;
  bool IsDirectory() const;
  bool IsHidden() const;
private:
  mutable FILETIME FLastWriteTimeSource{};
  mutable TDateTime FLastWriteTime{};
};
//---------------------------------------------------------------------------
#if 0
typedef void (__closure* TProcessLocalFileEvent)
  (UnicodeString & FileName, const TSearchRecSmart & Rec, void * Param);
#endif // #if 0
using TProcessLocalFileEvent = nb::FastDelegate3<void,
  UnicodeString /*FileName*/, const TSearchRecSmart & /*Rec*/, void * /*Param*/>;

NB_CORE_EXPORT bool FileSearchRec(UnicodeString FileName, TSearchRec & Rec);
NB_CORE_EXPORT void CopySearchRec(const TSearchRec & Source, TSearchRec & Dest);
struct NB_CORE_EXPORT TSearchRecChecked : public TSearchRecSmart
{
  UnicodeString Path;
  bool Opened{false};
};
struct NB_CORE_EXPORT TSearchRecOwned : public TSearchRecChecked
{
  ~TSearchRecOwned() noexcept;
  void Close();
};
NB_CORE_EXPORT DWORD FindCheck(DWORD Result, UnicodeString APath);
NB_CORE_EXPORT DWORD FindFirstUnchecked(UnicodeString APath, DWORD LocalFileAttrs, TSearchRecChecked &F);
NB_CORE_EXPORT DWORD FindFirstChecked(UnicodeString APath, DWORD LocalFileAttrs, TSearchRecChecked &F);
NB_CORE_EXPORT DWORD FindNextChecked(TSearchRecChecked &F);
NB_CORE_EXPORT void ProcessLocalDirectory(UnicodeString ADirName,
  TProcessLocalFileEvent CallBackFunc, void *Param = nullptr, DWORD FindAttrs = INVALID_FILE_ATTRIBUTES);
NB_CORE_EXPORT DWORD FileGetAttrFix(UnicodeString AFileName);
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
NB_CORE_EXPORT UnicodeString FixedLenDateTimeFormat(UnicodeString Format);
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
NB_CORE_EXPORT UnicodeString FormatDateTimeSpan(UnicodeString TimeFormat, TDateTime DateTime);
NB_CORE_EXPORT TStrings * TlsCipherList();
//---------------------------------------------------------------------------
#if 0
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
extern UnicodeString RtfPara;
extern UnicodeString AssemblyNamespace;
extern UnicodeString SessionClassName;
extern UnicodeString TransferOptionsClassName;
//---------------------------------------------------------------------
UnicodeString RtfText(UnicodeString & Text, bool Rtf = true);
UnicodeString RtfColor(int Index);
UnicodeString RtfOverrideColorText(UnicodeString & Text);
UnicodeString RtfColorItalicText(int Color, UnicodeString & Text);
UnicodeString RtfColorText(int Color, UnicodeString & Text);
UnicodeString RtfKeyword(UnicodeString & Text);
UnicodeString RtfParameter(UnicodeString & Text);
UnicodeString RtfString(UnicodeString & Text);
UnicodeString RtfLink(UnicodeString & Link, UnicodeString & RtfText);
UnicodeString RtfSwitch(
  UnicodeString & Name, UnicodeString & Link, bool Rtf = true);
UnicodeString RtfSwitchValue(
  UnicodeString & Name, UnicodeString & Link, UnicodeString & Value, bool Rtf = true);
UnicodeString RtfSwitch(
  UnicodeString & Name, UnicodeString & Link, UnicodeString & Value, bool Rtf = true);
UnicodeString RtfSwitch(
  UnicodeString & Name, UnicodeString & Link, int Value, bool Rtf = true);
UnicodeString RtfEscapeParam(UnicodeString Param, bool PowerShellEscape);
UnicodeString RtfRemoveHyperlinks(UnicodeString Text);
UnicodeString ScriptCommandLink(UnicodeString & Command);
UnicodeString AssemblyBoolean(TAssemblyLanguage Language, bool Value);
UnicodeString AssemblyString(TAssemblyLanguage Language, UnicodeString S);
UnicodeString AssemblyCommentLine(TAssemblyLanguage Language, UnicodeString & Text);
UnicodeString AssemblyPropertyRaw(
  TAssemblyLanguage Language, UnicodeString & ClassName, UnicodeString & Name,
  UnicodeString & Value, bool Inline);
UnicodeString AssemblyProperty(
  TAssemblyLanguage Language, UnicodeString & ClassName, UnicodeString & Name,
  UnicodeString & Type, UnicodeString & Member, bool Inline);
UnicodeString AssemblyProperty(
  TAssemblyLanguage Language, UnicodeString & ClassName, UnicodeString & Name,
  UnicodeString & Value, bool Inline);
UnicodeString AssemblyProperty(
  TAssemblyLanguage Language, UnicodeString & ClassName, UnicodeString & Name, int Value, bool Inline);
UnicodeString AssemblyProperty(
  TAssemblyLanguage Language, UnicodeString & ClassName, UnicodeString & Name, bool Value, bool Inline);
UnicodeString RtfLibraryMethod(UnicodeString & ClassName, UnicodeString & MethodName, bool Inpage);
UnicodeString RtfLibraryClass(UnicodeString & ClassName);
UnicodeString AssemblyVariableName(TAssemblyLanguage Language, UnicodeString & ClassName);
UnicodeString AssemblyStatementSeparator(TAssemblyLanguage Language);
UnicodeString AssemblyVariableDeclaration(TAssemblyLanguage Language);
UnicodeString AssemblyNewClassInstance(
  TAssemblyLanguage Language, UnicodeString & ClassName, bool Inline);
UnicodeString AssemblyNewClassInstanceStart(
  TAssemblyLanguage Language, UnicodeString & ClassName, bool Inline);
UnicodeString AssemblyNewClassInstanceEnd(TAssemblyLanguage Language, bool Inline);
UnicodeString AssemblyAddRawSettings(
  TAssemblyLanguage Language, TStrings * RawSettings, UnicodeString & ClassName,
  UnicodeString & MethodName);
#endif // if 0

#pragma warning(push)
#pragma warning(disable: 4512) // assignment operator could not be generated
//---------------------------------------------------------------------------
__removed #include "Global.h"
//---------------------------------------------------------------------------
template<class T>
class TValueRestorer // : public TObject
{
public:
  TValueRestorer() = delete;
  inline explicit TValueRestorer(T &Target, const T &Value) :
    FTarget(Target),
    FValue(Value)
  {
  }

  inline explicit TValueRestorer(T &Target) :
    FTarget(Target),
    FValue(Target)
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
  bool FArmed{true};
};
//---------------------------------------------------------------------------
class TAutoNestingCounter : public TValueRestorer<intptr_t>
{
public:
  TAutoNestingCounter() = delete;
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

  TAutoFlag() = default;
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
  using TFirstToSecond = nb::map_t<T1, T2>;
  using const_iterator = typename TFirstToSecond::const_iterator;

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
  using TSecondToFirst = nb::map_t<T2, T1>;
  TSecondToFirst FSecondToFirst;
};
//---------------------------------------------------------------------------
using TUnicodeStringVector = nb::vector_t<UnicodeString>;
//---------------------------------------------------------------------------
enum TModificationFmt
{
  mfNone,
  mfMDHM,
  mfMDY,
  mfFull,
};
//---------------------------------------------------------------------------
namespace base {

NB_CORE_EXPORT bool IsUnixStyleWindowsPath(UnicodeString APath);
NB_CORE_EXPORT bool UnixIsAbsolutePath(UnicodeString APath);
NB_CORE_EXPORT UnicodeString UnixIncludeTrailingBackslash(UnicodeString APath);
NB_CORE_EXPORT UnicodeString UnixExcludeTrailingBackslash(UnicodeString APath, bool Simple = false);
NB_CORE_EXPORT UnicodeString SimpleUnixExcludeTrailingBackslash(UnicodeString APath);
NB_CORE_EXPORT UnicodeString UnixCombinePaths(UnicodeString APath1, UnicodeString APath2);
NB_CORE_EXPORT UnicodeString UnixExtractFileDir(UnicodeString APath);
NB_CORE_EXPORT UnicodeString UnixExtractFilePath(UnicodeString APath);
NB_CORE_EXPORT UnicodeString UnixExtractFileName(UnicodeString APath);
NB_CORE_EXPORT UnicodeString UnixExtractFileExt(UnicodeString APath);
NB_CORE_EXPORT Boolean UnixSamePath(UnicodeString APath1, UnicodeString APath2);
NB_CORE_EXPORT bool UnixIsChildPath(UnicodeString AParent, UnicodeString AChild);
NB_CORE_EXPORT bool ExtractCommonPath(const TStrings *AFiles, UnicodeString &APath);
NB_CORE_EXPORT bool UnixExtractCommonPath(const TStrings *AFiles, UnicodeString &APath);
NB_CORE_EXPORT UnicodeString ExtractFileName(UnicodeString APath, bool Unix);
NB_CORE_EXPORT bool IsUnixRootPath(UnicodeString APath);
NB_CORE_EXPORT bool IsUnixHiddenFile(UnicodeString AFileName);
NB_CORE_EXPORT UnicodeString AbsolutePath(UnicodeString Base, UnicodeString APath);
NB_CORE_EXPORT UnicodeString FromUnixPath(UnicodeString APath);
NB_CORE_EXPORT UnicodeString ToUnixPath(UnicodeString APath);
NB_CORE_EXPORT UnicodeString MinimizeName(UnicodeString AFileName, intptr_t MaxLen, bool Unix);
NB_CORE_EXPORT UnicodeString MakeFileList(const TStrings *AFileList);
NB_CORE_EXPORT TDateTime ReduceDateTimePrecision(const TDateTime &ADateTime,
  TModificationFmt Precision);
NB_CORE_EXPORT TModificationFmt LessDateTimePrecision(
  TModificationFmt Precision1, TModificationFmt Precision2);
NB_CORE_EXPORT UnicodeString UserModificationStr(const TDateTime &DateTime,
  TModificationFmt Precision);
NB_CORE_EXPORT UnicodeString ModificationStr(const TDateTime &DateTime,
  TModificationFmt Precision);
NB_CORE_EXPORT int FakeFileImageIndex(UnicodeString AFileName, uint32_t Attrs = INVALID_FILE_ATTRIBUTES,
  UnicodeString *TypeName = nullptr);
NB_CORE_EXPORT bool SameUserName(UnicodeString UserName1, UnicodeString UserName2);
NB_CORE_EXPORT UnicodeString FormatMultiFilesToOneConfirmation(UnicodeString ATarget, bool Unix);

} // namespace base

namespace base {
//TODO: move to Sysutils.hpp
NB_CORE_EXPORT UnicodeString GetEnvVariable(UnicodeString AEnvVarName);
NB_CORE_EXPORT UnicodeString FormatBytes(int64_t Bytes, bool UseOrders = true);
NB_CORE_EXPORT UnicodeString GetEnvVariable(UnicodeString AEnvVarName);
} // namespace base
//---------------------------------------------------------------------------
constexpr char * LOCAL_INVALID_CHARS = "/\\:*?\"<>|";
constexpr char * PASSWORD_MASK = "***";
constexpr char * sLineBreak = "\n";

// Order of the values also define order of the buttons/answers on the prompts
// MessageDlg relies on these to be <= 0x0000FFFF
constexpr uint32_t qaYes      = 0x00000001;
// MessageDlg relies that answer do not conflict with mrCancel (=0x2)
constexpr uint32_t qaNo       = 0x00000004;
constexpr uint32_t qaOK       = 0x00000008;
constexpr uint32_t qaCancel   = 0x00000010;
constexpr uint32_t qaYesToAll = 0x00000020;
constexpr uint32_t qaNoToAll  = 0x00000040;
constexpr uint32_t qaAbort    = 0x00000080;
constexpr uint32_t qaRetry    = 0x00000100;
constexpr uint32_t qaIgnore   = 0x00000200;
constexpr uint32_t qaSkip     = 0x00000400;
constexpr uint32_t qaAll      = 0x00000800;
constexpr uint32_t qaHelp     = 0x00001000;
constexpr uint32_t qaReport   = 0x00002000;

constexpr uint32_t qaFirst = qaYes;
constexpr uint32_t qaLast  = qaReport;

constexpr uint32_t qaNeverAskAgain = 0x00010000;

constexpr intptr_t qpFatalAbort           = 0x01;
constexpr intptr_t qpNeverAskAgainCheck   = 0x02;
constexpr intptr_t qpAllowContinueOnError = 0x04;
constexpr intptr_t qpIgnoreAbort          = 0x08;
constexpr intptr_t qpWaitInBatch          = 0x10;

inline void ThrowExtException() { throw ExtException(static_cast<Exception *>(nullptr), UnicodeString(L"")); }
NB_CORE_EXPORT bool CompareFileName(UnicodeString APath1, UnicodeString APath2);
NB_CORE_EXPORT bool ComparePaths(UnicodeString APath1, UnicodeString APath2);
//---------------------------------------------------------------------------
