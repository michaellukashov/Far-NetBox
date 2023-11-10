
#ifndef NO_WIN32_LEAN_AND_MEAN
#define NO_WIN32_LEAN_AND_MEAN
#endif
#include <vcl.h>

#include <System.ShlObj.hpp>
#include <Exceptions.h>
#include <TextsCore.h>
//#include <Interface.h>
#include <Common.h>
#include <Global.h>
#include <StrUtils.hpp>
#include <Sysutils.hpp>
//#include <CoreMain.h>
#include <System.IOUtils.hpp>
#include <cmath>
#include <limits>
#include <algorithm>
#include <rdestl/map.h>
#include <rdestl/vector.h>
#include <CoreMain.h>
#include <SessionInfo.h>
#include <Soap.EncdDecd.hpp>

__removed #pragma package(smart_init)

#pragma warning(disable: 4996) // https://msdn.microsoft.com/en-us/library/ttcz0bys.aspx The compiler encountered a deprecated declaration

#include "Exceptions.h"
#include "Interface.h"
#include "Terminal.h"
#include "TextsCore.h"
#include "HelpCore.h"
#include "Cryptography.h"

const UnicodeString PartialExt(L".filepart");

namespace base { // from RemoteFiles.cpp

/* TODO 1 : Path class instead of UnicodeString (handle relativity...) */

bool IsUnixStyleWindowsPath(const UnicodeString & APath)
{
  return (APath.Length() >= 3) && IsLetter(APath[1]) && (APath[2] == L':') && (APath[3] == L'/');
}

bool UnixIsAbsolutePath(const UnicodeString & APath)
{
  return
    ((APath.Length() >= 1) && (APath[1] == L'/')) ||
    // we need this for FTP only, but this is unfortunately used in a static context
    base::IsUnixStyleWindowsPath(APath);
}

UnicodeString UnixIncludeTrailingBackslash(const UnicodeString & APath)
{
  // it used to return "/" when input path was empty
  if (!APath.IsEmpty() && !APath.IsDelimiter(SLASH, APath.Length()))
  {
    return APath + SLASH;
  }
  return APath;
}

// Keeps "/" for root path
UnicodeString UnixExcludeTrailingBackslash(const UnicodeString & APath, bool Simple)
{
  UnicodeString Result;
  if (APath.IsEmpty() ||
      (APath == ROOTDIRECTORY) ||
      !APath.IsDelimiter(SLASH, APath.Length()) ||
      (!Simple && ((APath.Length() == 3) && base::IsUnixStyleWindowsPath(APath))))
  {
    Result = APath;
  }
  else
  {
    Result = APath.SubString(1, APath.Length() - 1);
  }
  if (Result.IsEmpty())
  {
    Result = ROOTDIRECTORY;
  }
  return Result;
}

UnicodeString SimpleUnixExcludeTrailingBackslash(const UnicodeString & APath)
{
  return base::UnixExcludeTrailingBackslash(APath, true);
}

UnicodeString UnixCombinePaths(const UnicodeString & APath1, const UnicodeString & APath2)
{
  return UnixIncludeTrailingBackslash(APath1) + APath2;
}

Boolean UnixSamePath(const UnicodeString & APath1, const UnicodeString & APath2)
{
  return (base::UnixIncludeTrailingBackslash(APath1) == base::UnixIncludeTrailingBackslash(APath2));
}

bool UnixIsChildPath(const UnicodeString & AParent, const UnicodeString & AChild)
{
  UnicodeString Parent = base::UnixIncludeTrailingBackslash(AParent);
  UnicodeString Child = base::UnixIncludeTrailingBackslash(AChild);
  return (Child.SubString(1, Parent.Length()) == Parent);
}

UnicodeString UnixExtractFileDir(const UnicodeString & APath)
{
  const int32_t Pos = APath.LastDelimiter(L'/');
  // it used to return Path when no slash was found
  if (Pos > 1)
  {
    return APath.SubString(1, Pos - 1);
  }
  else
  {
    return (Pos == 1) ? UnicodeString(ROOTDIRECTORY) : UnicodeString();
  }
}

// must return trailing backslash
UnicodeString UnixExtractFilePath(const UnicodeString & APath)
{
  const int32_t Pos = APath.LastDelimiter(L'/');
  // it used to return Path when no slash was found
  if (Pos > 0)
  {
    return APath.SubString(1, Pos);
  }
  return UnicodeString();
}

UnicodeString UnixExtractFileName(const UnicodeString & APath)
{
  int32_t Pos = APath.LastDelimiter(L'/');
  UnicodeString Result;
  if (Pos > 0)
  {
    Result = APath.SubString(Pos + 1, APath.Length() - Pos);
  }
  else
  {
    Result = APath;
  }
  return Result;
}

UnicodeString ExtractShortName(const UnicodeString & Path, bool Unix)
{
  UnicodeString Result = ExtractFileName(Path, Unix);
  if (Result.IsEmpty())
  {
    Result = Path;
  }
  return Result;
}

UnicodeString UnixExtractFileExt(const UnicodeString & APath)
{
  UnicodeString FileName = UnixExtractFileName(APath);
  int32_t Pos = FileName.LastDelimiter(L".");
  if (Pos > 0)
    return APath.SubString(Pos, APath.Length() - Pos + 1);
  else
    return UnicodeString();
}

UnicodeString ExtractFileName(const UnicodeString & APath, bool Unix)
{
  if (Unix)
  {
    return UnixExtractFileName(APath);
  }
  else
  {
    return base::ExtractFileName(APath, Unix);
  }
}

bool ExtractCommonPath(const TStrings * AFiles, UnicodeString & APath)
{
  DebugAssert(AFiles->GetCount() > 0);

  APath = ::ExtractFilePath(AFiles->GetString(0));
  bool Result = !APath.IsEmpty();
  if (Result)
  {
    for (int32_t Index = 1; Index < AFiles->GetCount(); ++Index)
    {
      while (!APath.IsEmpty() &&
        (AFiles->GetString(Index).SubString(1, APath.Length()) != APath))
      {
        const int32_t PrevLen = APath.Length();
        APath = ::ExtractFilePath(::ExcludeTrailingBackslash(APath));
        if (APath.Length() == PrevLen)
        {
          APath.Clear();
          Result = false;
        }
      }
    }
  }

  return Result;
}

static UnicodeString GetFileListItemPath(const TStrings * Files, int32_t Index)
{
  UnicodeString Result;
  if (Files->GetObj(Index) != nullptr)
  {
    Result = DebugNotNull(dyn_cast<TRemoteFile>(Files->GetObj(Index)))->FullFileName();
  }
  else
  {
    Result = Files->GetString(Index);
  }
  return Result;
}

bool UnixExtractCommonPath(const TStrings * AFiles, UnicodeString & APath)
{
  DebugAssert(AFiles->GetCount() > 0);

  APath = base::UnixExtractFilePath(GetFileListItemPath(AFiles, 0));
  bool Result = !APath.IsEmpty();
  if (Result)
  {
    for (int32_t Index = 1; Index < AFiles->GetCount(); ++Index)
    {
      while (!APath.IsEmpty() &&
        (GetFileListItemPath(AFiles, Index).SubString(1, APath.Length()) != APath))
      {
        const int32_t PrevLen = APath.Length();
        APath = base::UnixExtractFilePath(base::UnixExcludeTrailingBackslash(APath));
        if (APath.Length() == PrevLen)
        {
          APath.Clear();
          Result = false;
        }
      }
    }
  }

  return Result;
}

bool IsUnixRootPath(const UnicodeString & APath)
{
  return APath.IsEmpty() || (APath == ROOTDIRECTORY);
}

bool IsUnixHiddenFile(const UnicodeString & AFileName)
{
//  return (APath != THISDIRECTORY) && (APath != PARENTDIRECTORY) &&
//    !APath.IsEmpty() && (APath[1] == L'.');
  return IsRealFile(AFileName) && !AFileName.IsEmpty() && (AFileName[1] == L'.');
}

UnicodeString AbsolutePath(const UnicodeString & Base, const UnicodeString & APath)
{
  // There's a duplicate implementation in TTerminal::ExpandFileName()
  UnicodeString Result;
  if (APath.IsEmpty())
  {
    Result = Base;
  }
  else if (APath[1] == L'/')
  {
    Result = base::UnixExcludeTrailingBackslash(APath);
  }
  else
  {
    Result = base::UnixIncludeTrailingBackslash(
      base::UnixIncludeTrailingBackslash(Base) + APath);
    int32_t P;
    while ((P = Result.Pos("/../")) > 0)
    {
      // special case, "/../" => "/"
      if (P == 1)
      {
        Result = ROOTDIRECTORY;
      }
      else
      {
        const int32_t P2 = Result.SubString(1, P - 1).LastDelimiter(L"/");
        DebugAssert(P2 > 0);
        Result.Delete(P2, P - P2 + 3);
      }
    }
    while ((P = Result.Pos("/./")) > 0)
    {
      Result.Delete(P, 2);
    }
    Result = base::UnixExcludeTrailingBackslash(Result);
  }
  return Result;
}

UnicodeString FromUnixPath(const UnicodeString & APath)
{
  return ReplaceStr(APath, SLASH, BACKSLASH);
}

UnicodeString ToUnixPath(const UnicodeString & APath)
{
  return ReplaceStr(APath, BACKSLASH, SLASH);
}

static void CutFirstDirectory(UnicodeString & S, bool Unix)
{
  UnicodeString Sep = Unix ? SLASH : BACKSLASH;
  if (S == Sep)
  {
    S.Clear();
  }
  else
  {
    bool Root;
    if (S[1] == Sep[1])
    {
      Root = true;
      S.Delete(1, 1);
    }
    else
    {
      Root = false;
    }
    if (S[1] == L'.')
    {
      S.Delete(1, 4);
    }
    const int32_t P = S.Pos(Sep[1]);
    if (P)
    {
      S.Delete(1, P);
      S = Ellipsis + Sep + S;
    }
    else
    {
      S.Clear();
    }
    if (Root)
    {
      S = Sep + S;
    }
  }
}

UnicodeString MinimizeName(const UnicodeString & AFileName, int32_t MaxLen, bool Unix)
{
  UnicodeString Drive, Dir, Name;
  UnicodeString Sep = Unix ? SLASH : BACKSLASH;

  UnicodeString Result = AFileName;
  if (Unix)
  {
    const int32_t P = Result.LastDelimiter(SLASH);
    if (P)
    {
      Dir = Result.SubString(1, P);
      Name = Result.SubString(P + 1, Result.Length() - P);
    }
    else
    {
      Dir.Clear();
      Name = Result;
    }
  }
  else
  {
    Dir = ::ExtractFilePath(Result);
    Name = base::ExtractFileName(Result, false);

    if (Dir.Length() >= 2 && Dir[2] == L':')
    {
      Drive = Dir.SubString(1, 2);
      Dir.Delete(1, 2);
    }
  }

  while ((!Dir.IsEmpty() || !Drive.IsEmpty()) && (Result.Length() > MaxLen))
  {
    if (Dir == Sep + Ellipsis + Sep)
    {
      Dir = Ellipsis + Sep;
    }
    else if (Dir.IsEmpty())
    {
      Drive.Clear();
    }
    else
    {
      CutFirstDirectory(Dir, Unix);
    }
    Result = Drive + Dir + Name;
  }

  if (Result.Length() > MaxLen)
  {
    Result = Result.SubString(1, MaxLen);
  }
  return Result;
}

UnicodeString MakeFileList(const TStrings * AFileList)
{
  UnicodeString Result;
  for (int32_t Index = 0; Index < AFileList->GetCount(); ++Index)
  {
    const UnicodeString& FileName = AFileList->GetString(Index);
    // currently this is used for local file only, so no delimiting is done
    AddToList(Result, AddQuotes(FileName), L" ");
  }
  return Result;
}

// copy from BaseUtils.pas
TDateTime ReduceDateTimePrecision(const TDateTime & ADateTime,
  TModificationFmt Precision)
{
  TDateTime DateTime = ADateTime;
  if (Precision == mfNone)
  {
    DateTime = double(0.0);
  }
  else if (Precision != mfFull)
  {
    uint16_t Y, M, D, H, N, S, MS;

    ::DecodeDate(DateTime, Y, M, D);
    ::DecodeTime(DateTime, H, N, S, MS);
    switch (Precision)
    {
      case mfYMDHM:
      case mfMDHM:
        S = 0;
        MS = 0;
        break;

      case mfMDY:
        H = 0;
        N = 0;
        S = 0;
        MS = 0;
        break;

      default:
        DebugFail();
    }

    DateTime = EncodeDateVerbose(Y, M, D) + EncodeTimeVerbose(H, N, S, MS);
  }
  return DateTime;
}

TModificationFmt LessDateTimePrecision(
  TModificationFmt Precision1, TModificationFmt Precision2)
{
  return (Precision1 < Precision2) ? Precision1 : Precision2;
}

UnicodeString UserModificationStr(const TDateTime & DateTime,
  TModificationFmt Precision)
{
  Word Year, Month, Day, Hour, Min, Sec, MSec;
  DateTime.DecodeDate(Year, Month, Day);
  DateTime.DecodeTime(Hour, Min, Sec, MSec);
  switch (Precision)
  {
    case mfNone:
      return L"";
    case mfMDY:
      return FORMAT("%3s %2d %2d", EngShortMonthNames[Month - 1], Day, Year);
    case mfYMDHM:
    case mfMDHM:
      return FORMAT("%3s %2d %2d:%2.2d",
          EngShortMonthNames[Month - 1], Day, Hour, Min);
    case mfFull:
      return FORMAT("%3s %2d %2d:%2.2d:%2.2d %4d",
          EngShortMonthNames[Month - 1], Day, Hour, Min, Sec, Year);
    default:
      DebugAssert(false);
  }
  return UnicodeString();
}

UnicodeString ModificationStr(const TDateTime &DateTime,
  TModificationFmt Precision)
{
  uint16_t Year, Month, Day, Hour, Min, Sec, MSec;
  DateTime.DecodeDate(Year, Month, Day);
  DateTime.DecodeTime(Hour, Min, Sec, MSec);
  switch (Precision)
  {
    case mfNone:
      return L"";

    case mfMDY:
      return FORMAT("%3s %2d %2d", EngShortMonthNames[Month - 1], Day, Year);

    case mfMDHM:
      return FORMAT("%3s %2d %2d:%2.2d",
          EngShortMonthNames[Month - 1], Day, Hour, Min);

    default:
      DebugFail();
    // fall thru

    case mfFull:
      return FORMAT("%3s %2d %2d:%2.2d:%2.2d %4d",
        EngShortMonthNames[Month - 1], Day, Hour, Min, Sec, Year);
  }
}

int32_t GetPartialFileExtLen(const UnicodeString & FileName)
{
  int32_t Result = 0;
  if (EndsText(PartialExt, FileName))
  {
    Result = PartialExt.Length();
  }
  else
  {
    int32_t P = FileName.LastDelimiter(L".");
    if ((P > 0) && (P < FileName.Length()))
    {
      if (IsNumber(MidStr(FileName, P + 1)) &&
          EndsText(PartialExt, FileName.SubString(1, P - 1)))
      {
        Result = PartialExt.Length() + (FileName.Length() - P + 1);
      }
    }
  }
  return Result;
}

int32_t FakeFileImageIndex(const UnicodeString & /*AFileName*/, uint32_t /*Attrs*/,
  UnicodeString * /*TypeName*/)
{
#if 0
  Attrs |= FILE_ATTRIBUTE_NORMAL;

  TSHFileInfoW SHFileInfo = {0};
  // On Win2k we get icon of "ZIP drive" for ".." (parent directory)
  if ((FileName == L"..") ||
      ((FileName.Length() == 2) && (FileName[2] == L':') && IsLetter(FileName[1])) ||
      IsReservedName(FileName))
  {
    FileName = L"dumb";
  }
  // this should be somewhere else, probably in TUnixDirView,
  // as the "partial" overlay is added there too
  int PartialFileExtLen = GetPartialFileExtLen(FileName);
  if (GetPartialFileExtLen(FileName) > 0)
  {
    FileName.SetLength(FileName.Length() - PartialFileExtLen);
  }

  int Icon;
  if (SHGetFileInfo(FileName.c_str(),
        Attrs, &SHFileInfo, sizeof(SHFileInfo),
        SHGFI_SYSICONINDEX | SHGFI_USEFILEATTRIBUTES | SHGFI_TYPENAME) != 0)
  {
    if (TypeName != nullptr)
    {
      *TypeName = SHFileInfo.szTypeName;
    }
    Icon = SHFileInfo.iIcon;
  }
  else
  {
    if (TypeName != nullptr)
    {
      *TypeName = L"";
    }
    Icon = -1;
  }

  return Icon;
#endif // #if 0
  return -1;
}

bool SameUserName(const UnicodeString & UserName1, const UnicodeString & UserName2)
{
  // Bitvise reports file owner as "user@host", but we login with "user" only.
  UnicodeString AUserName1 = CopyToChar(UserName1, L'@', true);
  UnicodeString AUserName2 = CopyToChar(UserName2, L'@', true);
  return ::SameText(AUserName1, AUserName2);
}

UnicodeString FormatMultiFilesToOneConfirmation(const UnicodeString & ATarget, bool Unix)
{
  UnicodeString Dir;
  UnicodeString Name;
  UnicodeString Path;
  if (Unix)
  {
    Dir = UnixExtractFileDir(ATarget);
    Name = UnixExtractFileName(ATarget);
    Path = UnixIncludeTrailingBackslash(ATarget);
  }
  else
  {
    Dir = ::ExtractFilePath(ATarget);
    Name = ExtractFileName(ATarget, Unix);
    Path = ::IncludeTrailingBackslash(ATarget);
  }
  return FMTLOAD(MULTI_FILES_TO_ONE, Name, Dir, Path);
}


UnicodeString FormatBytes(int64_t Bytes, bool UseOrders)
{
  UnicodeString Result;

  if (!UseOrders || (Bytes < nb::ToInt64(100 * 1024)))
  {
    // Result = FormatFloat(L"#,##0 \"B\"", Bytes);
    Result = FORMAT("%.0f B", nb::ToDouble(Bytes));
  }
  else if (Bytes < nb::ToInt64(100 * 1024 * 1024))
  {
    // Result = FormatFloat(L"#,##0 \"KB\"", Bytes / 1024);
    Result = FORMAT("%.0f KB", nb::ToDouble(Bytes / 1024.0));
  }
  else
  {
    // Result = FormatFloat(L"#,##0 \"MiB\"", Bytes / (1024*1024));
    Result = FORMAT("%.0f MiB", nb::ToDouble(Bytes / (1024 * 1024.0)));
  }
  return Result;
}

UnicodeString GetEnvVariable(const UnicodeString & AEnvVarName)
{
  UnicodeString Result;
  const int32_t Len = ::GetEnvironmentVariableW(AEnvVarName.c_str(), nullptr, 0);
  if (Len > 0)
  {
    wchar_t *Buffer = Result.SetLength(Len - 1);
    ::GetEnvironmentVariableW(AEnvVarName.c_str(), reinterpret_cast<LPWSTR>(Buffer), nb::ToDWord(Len));
  }
  return Result;
}

} // namespace base


#include <openssl/pkcs12.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/ssl.h>

//#pragma package(smart_init)

const wchar_t * DSTModeNames = L"Win;Unix;Keep";


const UnicodeString AnyMask = L"*.*";
const wchar_t EngShortMonthNames[12][4] =
{
  L"Jan", L"Feb", L"Mar", L"Apr", L"May", L"Jun",
  L"Jul", L"Aug", L"Sep", L"Oct", L"Nov", L"Dec"
};
const char Bom[4] = "\xEF\xBB\xBF";
const wchar_t TokenPrefix = L'%';
const wchar_t NoReplacement = wchar_t(0);
const wchar_t TokenReplacement = wchar_t(1);
// Note similar list in MakeValidFileName
const UnicodeString LocalInvalidChars(TraceInitStr(L"/\\:*?\"<>|"));
const UnicodeString PasswordMask(TraceInitStr(L"***"));
const UnicodeString Ellipsis(TraceInitStr(L"..."));
const UnicodeString TitleSeparator(TraceInitStr(L" \u2013 ")); // En-Dash
const UnicodeString OfficialPackage(TraceInitStr(L"MartinPikryl.WinSCP_tvv458r3h9r5m"));

UnicodeString ReplaceChar(const UnicodeString & Str, wchar_t A, wchar_t B)
{
  UnicodeString Result = Str;
  wchar_t *Buffer = ToWChar(Result);
  for (wchar_t *Ch = Buffer; Ch && *Ch; ++Ch)
    if (*Ch == A)
    {
      *Ch = B;
    }
  return Result;
}

UnicodeString DeleteChar(const UnicodeString & Str, wchar_t C)
{
  UnicodeString Result = Str;
  int32_t P;
  while ((P = Result.Pos(C)) > 0)
  {
    Result.Delete(P, 1);
  }
  return Result;
}

template <typename T>
void DoPackStr(T &Str)
{
  // Following will free unnecessary bytes
  Str = Str.c_str();
}

void PackStr(UnicodeString & Str)
{
  DoPackStr(Str);
}

void PackStr(RawByteString & Str)
{
  DoPackStr(Str);
}

void PackStr(AnsiString & Str)
{
  DoPackStr(Str);
}

template <typename T>
void DoShred(T & Str)
{
  if (!Str.IsEmpty())
  {
    Str.Unique();
    ::ZeroMemory(nb::ToPtr(Str.c_str()), Str.Length() * sizeof(*Str.c_str()));
    Str = L"";
  }
}

void Shred(UnicodeString & Str)
{
  DoShred(Str);
}

void Shred(UTF8String & Str)
{
  DoShred(Str);
}

void Shred(AnsiString & Str)
{
  DoShred(Str);
}

void Shred(RawByteString & Str)
{
  DoShred(Str);
}

UnicodeString AnsiToString(const RawByteString & S)
{
  return UnicodeString(AnsiString(S));
}

UnicodeString AnsiToString(const char *S, size_t Len)
{
  return UnicodeString(AnsiString(S, Len));
}

// Note similar function ValidLocalFileName
UnicodeString MakeValidFileName(const UnicodeString & AFileName)
{
  UnicodeString Result = AFileName;
  // Note similar list in LocalInvalidChars
  UnicodeString IllegalChars(":;,=+<>|\"[] \\/?*");
  for (int32_t Index = 0; Index < IllegalChars.Length(); ++Index)
  {
    Result = ReplaceChar(Result, IllegalChars[Index + 1], L'-');
  }
  return Result;
}

UnicodeString RootKeyToStr(HKEY RootKey)
{
  if (RootKey == HKEY_USERS)
    return "HKU";
  if (RootKey == HKEY_LOCAL_MACHINE)
    return "HKLM";
  if (RootKey == HKEY_CURRENT_USER)
    return "HKCU";
  if (RootKey == HKEY_CLASSES_ROOT)
    return "HKCR";
  if (RootKey == HKEY_CURRENT_CONFIG)
    return "HKCC";
  if (RootKey == HKEY_DYN_DATA)
    return "HKDD";
  Abort();
  return "";
}

UnicodeString BooleanToEngStr(bool B)
{
  if (B)
  {
    return "Yes";
  }
  return "No";
}

UnicodeString BooleanToStr(bool B)
{
  if (B)
  {
    return LoadStr(YES_STR);
  }
  return LoadStr(NO_STR);
}

UnicodeString DefaultStr(const UnicodeString & Str, const UnicodeString & Default)
{
  if (!Str.IsEmpty())
  {
    return Str;
  }
  return Default;
}

// For alternative with quoting support, see TFTPFileSystem::CutFeature
UnicodeString CutToChar(UnicodeString & Str, wchar_t Ch, bool Trim)
{
  const int32_t P = Str.Pos(Ch);
  UnicodeString Result;
  if (P)
  {
    Result = Str.SubString(1, P - 1);
    Str.Delete(1, P);
  }
  else
  {
    Result = Str;
    Str.Clear();
  }
  if (Trim)
  {
    Result = Result.TrimRight();
    Str = Str.TrimLeft();
  }
  return Result;
}

UnicodeString CopyToChars(const UnicodeString & Str, int32_t & From, const UnicodeString & Chs, bool Trim,
  wchar_t * Delimiter, bool DoubleDelimiterEscapes)
{
  UnicodeString Result;

  int32_t P;
  for (P = From; P <= Str.Length(); P++)
  {
    if (::IsDelimiter(Chs, Str, P))
    {
      if (DoubleDelimiterEscapes &&
          (P < Str.Length()) &&
          IsDelimiter(Chs, Str, P + 1) &&
          (Str[P + 1] == Str[P]))
      {
        Result += Str[P];
        P++;
      }
      else
      {
        break;
      }
    }
    else
    {
      Result += Str[P];
    }
  }

  if (P <= Str.Length())
  {
    if (Delimiter != nullptr)
    {
      *Delimiter = Str[P];
    }
  }
  else
  {
    if (Delimiter != nullptr)
    {
      *Delimiter = L'\0';
    }
  }
  // even if we reached the end, return index, as if there were the delimiter,
  // so caller can easily find index of the end of the piece by subtracting
  // 2 from From (as long as he did not asked for trimming)
  From = P + 1;
  if (Trim)
  {
    Result = Result.TrimRight();
    while ((From <= Str.Length()) && (Str[From] == L' '))
    {
      From++;
    }
  }
  return Result;
}

UnicodeString CopyToChar(const UnicodeString & Str, wchar_t Ch, bool Trim)
{
  int From = 1;
  return CopyToChars(Str, From, UnicodeString(Ch), Trim);
}

UnicodeString RemoveSuffix(const UnicodeString & Str, const UnicodeString & Suffix, bool RemoveNumbersAfterSuffix)
{
  UnicodeString Result = Str;
  UnicodeString Buf = Str;
  if (RemoveNumbersAfterSuffix)
  {
    while (!Buf.IsEmpty() && IsDigit(Buf[Buf.Length()]))
    {
      Buf.SetLength(Buf.Length() - 1);
    }
  }
  if (EndsStr(Suffix, Buf))
  {
    Result.SetLength(Buf.Length() - Suffix.Length());
  }
  return Result;
}

UnicodeString DelimitStr(const UnicodeString & Str, wchar_t Quote)
{
  UnicodeString SpecialChars;
  if (Quote != L'\'')
  {
    SpecialChars = L"$\\";
    if (Quote == L'"')
    {
      SpecialChars += L"`\"";
    }
  }
  UnicodeString Result(Str);
  for (int i = 1; i <= Result.Length(); i++)
  {
    if (Result.IsDelimiter(SpecialChars, i))
    {
      Result.Insert(L"\\", i);
      i++;
    }
  }
  if (Result.IsDelimiter(L"-", 1))
  {
    Result.Insert(L"./", 1);
  }
  return Result;
}

UnicodeString MidStr(const UnicodeString & Text, int Start)
{
  return Text.SubString(Start, Text.Length() - Start + 1);
}

UnicodeString ShellQuoteStr(const UnicodeString & Str)
{
  wchar_t Quote = L'"';
  UnicodeString QuoteStr(Quote);
  return QuoteStr + DelimitStr(Str, Quote) + QuoteStr;
}

UnicodeString ExceptionLogString(Exception *E)
{
  DebugAssert(E);
  if (isa<Exception>(E))
  {
    UnicodeString Msg = FORMAT("%s", UnicodeString(E->what()));
    if (isa<ExtException>(E))
    {
      TStrings *MoreMessages = dyn_cast<ExtException>(E)->GetMoreMessages();
      if (MoreMessages)
      {
        Msg += L"\n" +
          ReplaceStr(MoreMessages->GetText(), L"\r", L"");
      }
    }
    return Msg;
  }
  else
  {
#if defined(__BORLANDC__)
    wchar_t Buffer[1024];
    ExceptionErrorMessage(ExceptObject(), ExceptAddr(), Buffer, _countof(Buffer));
    return UnicodeString(Buffer);
#else
    return UnicodeString(E->what());
#endif
  }
}

UnicodeString MainInstructions(const UnicodeString & S)
{
  UnicodeString MainMsgTag = LoadStr(MAIN_MSG_TAG);
  return MainMsgTag + S + MainMsgTag;
}

bool HasParagraphs(const UnicodeString & S)
{
  return (S.Pos("\n\n") > 0);
}

UnicodeString MainInstructionsFirstParagraph(const UnicodeString & S)
{
  // WORKAROUND, we consider it bad practice, the highlighting should better
  // be localized (but maybe we change our mind later)
  UnicodeString Result;
  const int32_t Pos = S.Pos("\n\n");
  // we would not be calling this on single paragraph message
  if (DebugAlwaysTrue(Pos > 0))
  {
    Result =
      MainInstructions(S.SubString(1, Pos - 1)) +
      S.SubString(Pos, S.Length() - Pos + 1);
  }
  else
  {
    Result = MainInstructions(S);
  }
  return Result;
}

bool ExtractMainInstructions(UnicodeString & S, UnicodeString & MainInstructions)
{
  bool Result = false;
  UnicodeString MainMsgTag = LoadStr(MAIN_MSG_TAG);
  if (::StartsStr(MainMsgTag, S))
  {
    const int32_t EndTagPos =
      S.SubString(MainMsgTag.Length() + 1, S.Length() - MainMsgTag.Length()).Pos(MainMsgTag);
    if (EndTagPos > 0)
    {
      MainInstructions = S.SubString(MainMsgTag.Length() + 1, EndTagPos - 1);
      S.Delete(1, EndTagPos + (2 * MainMsgTag.Length()) - 1);
      Result = true;
    }
  }

  DebugAssert(MainInstructions.Pos(MainMsgTag) == 0);
  DebugAssert(S.Pos(MainMsgTag) == 0);

  return Result;
}

static int32_t FindInteractiveMsgStart(const UnicodeString & S)
{
  int32_t Result = 0;
  UnicodeString InteractiveMsgTag = LoadStr(INTERACTIVE_MSG_TAG);
  if (EndsStr(InteractiveMsgTag, S) &&
      (S.Length() >= 2 * InteractiveMsgTag.Length()))
  {
    Result = S.Length() - 2 * InteractiveMsgTag.Length() + 1;
    while ((Result > 0) && (S.SubString(Result, InteractiveMsgTag.Length()) != InteractiveMsgTag))
    {
      Result--;
    }
  }
  return Result;
}

UnicodeString RemoveMainInstructionsTag(const UnicodeString & S)
{
  UnicodeString Result = S;

  UnicodeString MainInstruction;
  if (ExtractMainInstructions(Result, MainInstruction))
  {
    Result = MainInstruction + Result;
  }
  return Result;
}

UnicodeString UnformatMessage(const UnicodeString & S)
{
  UnicodeString Result = RemoveMainInstructionsTag(S);

  const int32_t InteractiveMsgStart = FindInteractiveMsgStart(Result);
  if (InteractiveMsgStart > 0)
  {
    Result = Result.SubString(1, InteractiveMsgStart - 1);
  }
  return Result;
}

UnicodeString RemoveInteractiveMsgTag(const UnicodeString & S)
{
  UnicodeString Result = S;

  const int32_t InteractiveMsgStart = FindInteractiveMsgStart(Result);
  if (InteractiveMsgStart > 0)
  {
    UnicodeString InteractiveMsgTag = LoadStr(INTERACTIVE_MSG_TAG);
    Result.Delete(InteractiveMsgStart, InteractiveMsgTag.Length());
    Result.Delete(Result.Length() - InteractiveMsgTag.Length() + 1, InteractiveMsgTag.Length());
  }
  return Result;
}

UnicodeString RemoveEmptyLines(const UnicodeString & S)
{
  return
    ReplaceStr(
      ReplaceStr(S.TrimRight(), L"\n\n", L"\n"),
      L"\n \n", L"\n");
}

bool IsNumber(const UnicodeString & Str)
{
  bool Result = (Str.Length() > 0);
  for (int32_t Index = 1; (Index < Str.Length()) && Result; Index++)
  {
    wchar_t C = Str[Index];
    if ((C < L'0') || (C > L'9'))
    {
      Result = false;
    }
  }
  return Result;
}

UnicodeString EncodeStrToBase64(const RawByteString & Str)
{
  UnicodeString Result = EncodeBase64(Str.c_str(), Str.Length());
  Result = ReplaceStr(Result, sLineBreak, EmptyStr);
  return Result;
}

RawByteString DecodeBase64ToStr(const UnicodeString & Str)
{
  TBytes Bytes = DecodeBase64(Str);
  // This might be the same as TEncoding::ASCII->GetString.
  // const_cast: The operator[] const is (badly?) implemented to return by value
  return RawByteString(reinterpret_cast<const char *>(&const_cast<TBytes &>(Bytes)[0]), Bytes.size());
}

UnicodeString Base64ToUrlSafe(const UnicodeString & S)
{
  UnicodeString Result = S;
  while (EndsStr(L"=", Result))
  {
    Result.SetLength(Result.Length() - 1);
  }
  // See https://en.wikipedia.org/wiki/Base64#Implementations_and_history
  Result = ReplaceChar(Result, L'+', L'-');
  Result = ReplaceChar(Result, L'/', L'_');
  return Result;
}

const wchar_t NormalizedFingerprintSeparator = L'-';

UnicodeString MD5ToUrlSafe(const UnicodeString & S)
{
  return ReplaceChar(S, L':', NormalizedFingerprintSeparator);
}

bool SameChecksum(const UnicodeString & AChecksum1, const UnicodeString & AChecksum2, bool Base64)
{
  UnicodeString Checksum1(AChecksum1);
  UnicodeString Checksum2(AChecksum2);
  bool Result;
  if (Base64)
  {
    Checksum1 = Base64ToUrlSafe(Checksum1);
    Checksum2 = Base64ToUrlSafe(Checksum2);
    Result = SameStr(Checksum1, Checksum2);
  }
  else
  {
    Checksum1 = MD5ToUrlSafe(Checksum1);
    Checksum2 = MD5ToUrlSafe(Checksum2);
    Result = SameText(Checksum1, Checksum2);
  }
  return Result;
}

UnicodeString SystemTemporaryDirectory()
{
  UnicodeString TempDir;
  TempDir.SetLength(nb::NB_MAX_PATH);
  TempDir.SetLength(::GetTempPath(nb::NB_MAX_PATH, const_cast<LPWSTR>(TempDir.c_str())));
  return TempDir;
}

UnicodeString GetShellFolderPath(int32_t CSIdl)
{
  UnicodeString Result;
  wchar_t Path[2 * MAX_PATH + 10] = L"\0";
  if (SUCCEEDED(SHGetFolderPath(nullptr, CSIdl, nullptr, SHGFP_TYPE_CURRENT, Path)))
  {
    Result = Path;
  }
  return Result;
}

static UnicodeString GetWineHomeFolder()
{
  UnicodeString Result;

  UnicodeString WineHostHome = base::GetEnvVariable(L"WINE_HOST_HOME");
  if (!WineHostHome.IsEmpty())
  {
    Result = L"Z:" + base::FromUnixPath(WineHostHome);
  }
  else
  {
    // Should we use WinAPI GetUserName() instead?
    UnicodeString UserName = base::GetEnvVariable(L"USERNAME");
    if (!UserName.IsEmpty())
    {
      Result = L"Z:\\home\\" + UserName;
    }
  }

  if (!base::DirectoryExists(Result))
  {
    Result = L"";
  }

  return Result;
}

UnicodeString GetPersonalFolder()
{
  UnicodeString Result = GetShellFolderPath(CSIDL_PERSONAL);

  if (IsWine())
  {
    UnicodeString WineHome = GetWineHomeFolder();

    if (!WineHome.IsEmpty())
    {
      // if at least home exists, use it
      Result = WineHome;

      // but try to go deeper to "Documents"
      UnicodeString WineDocuments =
        IncludeTrailingBackslash(WineHome) + "Documents";
      if (base::DirectoryExists(WineDocuments))
      {
        Result = WineDocuments;
      }
    }
  }
  return Result;
}

UnicodeString GetDesktopFolder()
{
  UnicodeString Result = GetShellFolderPath(CSIDL_DESKTOPDIRECTORY);

  if (IsWine())
  {
    UnicodeString WineHome = GetWineHomeFolder();

    if (!WineHome.IsEmpty())
    {
      UnicodeString WineDesktop =
        IncludeTrailingBackslash(WineHome) + L"Desktop";
      if (base::DirectoryExists(WineHome))
      {
        Result = WineDesktop;
      }
    }
  }
  return Result;
}

// Particularly needed when using file name selected by TFilenameEdit,
// as it wraps a path to double-quotes, when there is a space in the path.
UnicodeString StripPathQuotes(const UnicodeString & APath)
{
  if ((APath.Length() >= 2) &&
      (APath[1] == L'\"') && (APath[APath.Length()] == L'\"'))
  {
    return APath.SubString(2, APath.Length() - 2);
  }
  return APath;
}

UnicodeString AddQuotes(const UnicodeString & AStr)
{
  UnicodeString Result = AStr;
  if (Result.Pos(" ") > 0)
  {
    Result = L"\"" + Result + L"\"";
  }
  return Result;
}

UnicodeString AddPathQuotes(const UnicodeString & APath)
{
  UnicodeString Result = StripPathQuotes(APath);
  return AddQuotes(Result);
}

static wchar_t * ReplaceChar(
  UnicodeString & AFileName, wchar_t * InvalidChar, wchar_t InvalidCharsReplacement)
{
  const int32_t Index = InvalidChar - AFileName.c_str() + 1;
  if (InvalidCharsReplacement == TokenReplacement)
  {
    // currently we do not support unicode chars replacement
    if (AFileName[Index] > 0xFF)
    {
      ThrowExtException();
    }

    AFileName.Insert(ByteToHex(static_cast<uint8_t>(AFileName[Index])), Index + 1);
    AFileName[Index] = TokenPrefix;
    InvalidChar = ToWChar(AFileName) + Index + 2;
  }
  else
  {
    AFileName[Index] = InvalidCharsReplacement;
    InvalidChar = ToWChar(AFileName) + Index;
  }
  return InvalidChar;
}

//  Note similar function MakeValidFileName
UnicodeString ValidLocalFileName(const UnicodeString & AFileName)
{
  return ValidLocalFileName(AFileName, L'_', L"", LOCAL_INVALID_CHARS);
}

UnicodeString ValidLocalFileName(
  const UnicodeString & AFileName, wchar_t AInvalidCharsReplacement,
  const UnicodeString & ATokenizibleChars, const UnicodeString & ALocalInvalidChars)
{
  UnicodeString FileName = AFileName;

  if (AInvalidCharsReplacement != NoReplacement)
  {
    const bool ATokenReplacement = (AInvalidCharsReplacement == TokenReplacement);
    const wchar_t *Chars = 
      (ATokenReplacement ? ATokenizibleChars : ALocalInvalidChars).c_str();
    wchar_t * InvalidChar = ToWChar(FileName);
    while ((InvalidChar = wcspbrk(InvalidChar, Chars)) != nullptr)
    {
      const int32_t Pos = (InvalidChar - FileName.c_str() + 1);
      wchar_t Char;
      if (ATokenReplacement &&
          (*InvalidChar == TokenPrefix) &&
          (((FileName.Length() - Pos) <= 1) ||
           (((Char = static_cast<wchar_t>(HexToByte(FileName.SubString(Pos + 1, 2)))) == L'\0') ||
            (ATokenizibleChars.Pos(Char) == 0))))
      {
        InvalidChar++;
      }
      else
      {
        InvalidChar = ReplaceChar(FileName, InvalidChar, AInvalidCharsReplacement);
      }
    }

    // Windows trim trailing space or dot, hence we must encode it to preserve it
    if (!FileName.IsEmpty() &&
        ((FileName[FileName.Length()] == L' ') ||
         (FileName[FileName.Length()] == L'.')))
    {
      ReplaceChar(FileName, ToWChar(FileName) + FileName.Length() - 1, AInvalidCharsReplacement);
    }

    if (IsReservedName(FileName))
    {
      int32_t P = FileName.Pos(".");
      if (P == 0)
      {
        P = FileName.Length() + 1;
      }
      FileName.Insert(L"%00", P);
    }
  }
  return FileName;
}

void SplitCommand(const UnicodeString & ACommand, UnicodeString & Program,
  UnicodeString & Params, UnicodeString & Dir)
{
  UnicodeString Command = ACommand.Trim();
  Params.Clear();
  Dir.Clear();
  if (!Command.IsEmpty() && (Command[1] == L'\"'))
  {
    Command.Delete(1, 1);
    const int32_t P = Command.Pos('"');
    if (P > 0)
    {
      Program = Command.SubString(1, P - 1).Trim();
      Params = Command.SubString(P + 1, Command.Length() - P).Trim();
    }
    else
    {
      throw Exception(FMTLOAD(INVALID_SHELL_COMMAND, UnicodeString(L"\"" + Command)));
    }
  }
  else
  {
    const int32_t P = Command.Pos(" ");
    if (P > 0)
    {
      Program = Command.SubString(1, P).Trim();
      Params = Command.SubString(P + 1, Command.Length() - P).Trim();
    }
    else
    {
      Program = Command;
    }
  }
  const int32_t B = Program.LastDelimiter(L"\\/");
  if (B > 0)
  {
    Dir = Program.SubString(1, B).Trim();
  }
}

UnicodeString ExtractProgram(const UnicodeString & ACommand)
{
  UnicodeString Program;
  UnicodeString Params;
  UnicodeString Dir;

  SplitCommand(ACommand, Program, Params, Dir);

  return Program;
}

UnicodeString ExtractProgramName(const UnicodeString & ACommand)
{
  UnicodeString Name = base::ExtractFileName(ExtractProgram(ACommand), false);
  const int32_t Dot = Name.LastDelimiter(L".");
  if (Dot > 0)
  {
    Name = Name.SubString(1, Dot - 1);
  }
  return Name;
}

UnicodeString FormatCommand(const UnicodeString & AProgram, const UnicodeString & AParams)
{
  UnicodeString Program = AProgram.Trim();
  UnicodeString Params = AParams.Trim();
  if (!Params.IsEmpty()) Params = L" " + Params;
  Program = AddQuotes(Program);
  return Program + Params;
}

const wchar_t ShellCommandFileNamePattern[] = L"!.!";

void ReformatFileNameCommand(UnicodeString & ACommand)
{
  if (!ACommand.IsEmpty())
  {
    UnicodeString Program, Params, Dir;
    SplitCommand(ACommand, Program, Params, Dir);
    if (Params.Pos(ShellCommandFileNamePattern) == 0)
    {
      Params = Params + (Params.IsEmpty() ? L"" : L" ") + ShellCommandFileNamePattern;
    }
    ACommand = FormatCommand(Program, Params);
  }
}

UnicodeString ExpandFileNameCommand(const UnicodeString & ACommand,
  const UnicodeString & AFileName)
{
  return AnsiReplaceStr(ACommand, ShellCommandFileNamePattern,
    AddPathQuotes(AFileName));
}

UnicodeString EscapeParam(const UnicodeString & AParam)
{
  // Make sure this won't break RTF syntax
  return ReplaceStr(AParam, L"\"", L"\"\"");
}

UnicodeString EscapePuttyCommandParam(const UnicodeString & AParam)
{
  UnicodeString Param = AParam;

  bool Space = false;

  for (int32_t Index = 1; Index <= Param.Length(); ++Index)
  {
    switch (Param[Index])
    {
      case L'"':
        Param.Insert(L"\\", Index);
        ++Index;
        break;

      case L' ':
        Space = true;
        break;

      case L'\\':
        int32_t I2 = Index;
        while ((I2 <= Param.Length()) && (Param[I2] == L'\\'))
        {
          I2++;
        }
        if ((I2 <= Param.Length()) && (Param[I2] == L'"'))
        {
          while (Param[Index] == L'\\')
          {
            Param.Insert(L"\\", Index);
            Index += 2;
          }
          Index--;
        }
        break;
    }
  }

  if (Space)
  {
    Param = L"\"" + Param + L'"';
  }

  return Param;
}

UnicodeString StringsToParams(TStrings * Strings)
{
  UnicodeString Result;

  for (int32_t Index = 0; Index < Strings->Count(); Index++)
  {
    UnicodeString Name = Strings->GetName(Index);
    UnicodeString Value = Strings->GetValueFromIndex(Index);
    // Do not quote if it is all-numeric
    if (IntToStr(StrToIntDef(Value, -1)) != Value)
    {
      Value = FORMAT(L"\"%s\"", EscapeParam(Value));
    }
    Result += FORMAT(L" %s=%s", Name, Value);
  }
  return Result;
}

UnicodeString ExpandEnvironmentVariables(const UnicodeString & Str)
{
  UnicodeString Buf;
  const int32_t Size = 1024;

  Buf.SetLength(Size);
  const int32_t Len = ::ExpandEnvironmentStringsW(Str.c_str(), const_cast<LPWSTR>(Buf.c_str()), nb::ToDWord(Size));

  if (Len > Size)
  {
    Buf.SetLength(Len);
    ::ExpandEnvironmentStringsW(Str.c_str(), const_cast<LPWSTR>(Buf.c_str()), nb::ToDWord(Len));
  }

  PackStr(Buf);

  return Buf;
}

UnicodeString GetNormalizedPath(const UnicodeString & Path)
{
  UnicodeString Result = ExcludeTrailingBackslash(Path);
  Result = base::FromUnixPath(Result);
  return Result;
}

UnicodeString GetCanonicalPath(const UnicodeString & Path)
{
  UnicodeString Result = ExtractShortPathName(Path);
  if (Result.IsEmpty())
  {
    Result = Path;
  }
  Result = GetNormalizedPath(Result);
  return Result;
}

bool IsPathToSameFile(const UnicodeString & Path1, const UnicodeString & Path2)
{
  UnicodeString CanonicalPath1 = GetCanonicalPath(Path1);
  UnicodeString CanonicalPath2 = GetCanonicalPath(Path2);

  bool Result = SameText(CanonicalPath1, CanonicalPath2);
  return Result;
}

bool SamePaths(const UnicodeString & Path1, const UnicodeString & Path2)
{
  // TODO: ExpandUNCFileName
  return AnsiSameText(IncludeTrailingBackslash(Path1), IncludeTrailingBackslash(Path2));
}

int32_t CompareLogicalText(
  const UnicodeString & S1, const UnicodeString & S2, bool NaturalOrderNumericalSorting)
{
  // Keep in sync with CompareLogicalTextPas

  int Result;
  if (NaturalOrderNumericalSorting)
  {
    Result = StrCmpLogicalW(S1.c_str(), S2.c_str());
  }
  else
  {
    Result = lstrcmpi(S1.c_str(), S2.c_str());
  }
  // For deterministics results
  if (Result == 0)
  {
    Result = lstrcmp(S1.c_str(), S2.c_str());
  }
  return Result;
}

int CompareNumber(__int64 Value1, __int64 Value2)
{
  int Result;
  if (Value1 < Value2)
  {
    Result = -1;
  }
  else if (Value1 == Value2)
  {
    Result = 0;
  }
  else
  {
    Result = 1;
  }
  return Result;
}

bool ContainsTextSemiCaseSensitive(const UnicodeString & Text, const UnicodeString & SubText)
{
  bool Result;
  if (AnsiLowerCase(SubText) == SubText)
  {
    Result = ContainsText(Text, SubText);
  }
  else
  {
    Result = ContainsStr(Text, SubText);
  }
  return Result;
}

bool IsReservedName(const UnicodeString & AFileName)
{
  UnicodeString FileName = AFileName;

  const int32_t P = FileName.Pos(".");
  const int32_t Len = (P > 0) ? P - 1 : FileName.Length();
  if ((Len == 3) || (Len == 4))
  {
    if (P > 0)
    {
      FileName.SetLength(P - 1);
    }
    static UnicodeString Reserved[] = {
      "CON", "PRN", "AUX", "NUL",
      "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
      "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9" };
    for (uint32_t Index = 0; Index < _countof(Reserved); ++Index)
    {
      if (SameText(FileName, Reserved[Index]))
      {
        return true;
      }
    }
  }
  return false;
}

// ApiPath support functions
// Inspired by
// https://stackoverflow.com/q/18580945/850848
// This can be reimplemented using PathCchCanonicalizeEx on Windows 8 and later
enum PATH_PREFIX_TYPE
{
  PPT_UNKNOWN,
  PPT_ABSOLUTE,           //Found absolute path that is none of the other types
  PPT_UNC,                //Found \\server\share\ prefix
  PPT_LONG_UNICODE,       //Found \\?\ prefix
  PPT_LONG_UNICODE_UNC,   //Found \\?\UNC\ prefix
};

static int32_t PathRootLength(const UnicodeString & APath)
{
  // Correction for PathSkipRoot API

  // Replace all /'s with \'s because PathSkipRoot can't handle /'s
  UnicodeString Result = ReplaceChar(APath, L'/', L'\\');

  // Now call the API
  const LPCTSTR Buffer = ::PathSkipRoot(Result.c_str());

  return (Buffer != nullptr) ? (Buffer - Result.c_str()) : -1;
}

static bool PathIsRelative_CorrectedForMicrosoftStupidity(const UnicodeString & APath)
{
  // Correction for PathIsRelative API

  // Replace all /'s with \'s because PathIsRelative can't handle /'s
  UnicodeString Result = ReplaceChar(APath, L'/', L'\\');

  //Now call the API
  return ::PathIsRelative(Result.c_str()) != FALSE;
}

static int32_t GetOffsetAfterPathRoot(const UnicodeString & APath, PATH_PREFIX_TYPE & PrefixType)
{
  // Checks if 'pPath' begins with the drive, share, prefix, etc
  // EXAMPLES:
  //    Path                          Return:   Points at:                 PrefixType:
  //   Relative\Folder\File.txt        0         Relative\Folder\File.txt   PPT_UNKNOWN
  //   \RelativeToRoot\Folder          1         RelativeToRoot\Folder      PPT_ABSOLUTE
  //   C:\Windows\Folder               3         Windows\Folder             PPT_ABSOLUTE
  //   \\server\share\Desktop          15        Desktop                    PPT_UNC
  //   \\?\C:\Windows\Folder           7         Windows\Folder             PPT_LONG_UNICODE
  //   \\?\UNC\server\share\Desktop    21        Desktop                    PPT_LONG_UNICODE_UNC
  // RETURN:
  //      = Index in 'pPath' after the root, or
  //      = 0 if no root was found
  int32_t Result = 0;

  PrefixType = PPT_UNKNOWN;

  if (!APath.IsEmpty())
  {
    const int32_t Len = APath.Length();

    const bool WinXPOnly = !IsWinVista();

    // The PathSkipRoot() API doesn't work correctly on Windows XP
    if (!WinXPOnly)
    {
      // Works since Vista and up, but still needs correction :)
      const int32_t RootLength = PathRootLength(APath);
      if (RootLength >= 0)
      {
        Result = RootLength + 1;
      }
    }

    // Now determine the type of prefix
    int32_t IndCheckUNC = -1;

    if ((Len >= 8) &&
        (APath[1] == L'\\' || APath[1] == L'/') &&
        (APath[2] == L'\\' || APath[2] == L'/') &&
        (APath[3] == L'?') &&
        (APath[4] == L'\\' || APath[4] == L'/') &&
        (APath[5] == L'U' || APath[5] == L'u') &&
        (APath[6] == L'N' || APath[6] == L'n') &&
        (APath[7] == L'C' || APath[7] == L'c') &&
        (APath[8] == L'\\' || APath[8] == L'/'))
    {
      // Found \\?\UNC\ prefix
      PrefixType = PPT_LONG_UNICODE_UNC;

      if (WinXPOnly)
      {
          //For older OS
          Result += 8;
      }

      //Check for UNC share later
      IndCheckUNC = 8;
    }
    else if ((Len >= 4) &&
        (APath[1] == L'\\' || APath[1] == L'/') &&
        (APath[2] == L'\\' || APath[2] == L'/') &&
        (APath[3] == L'?') &&
        (APath[4] == L'\\' || APath[4] == L'/'))
    {
      // Found \\?\ prefix
      PrefixType = PPT_LONG_UNICODE;

      if (WinXPOnly)
      {
          //For older OS
          Result += 4;
      }
    }
    else if ((Len >= 2) &&
        (APath[1] == L'\\' || APath[1] == L'/') &&
        (APath[2] == L'\\' || APath[2] == L'/'))
    {
      // Check for UNC share later
      IndCheckUNC = 2;
    }

    if (IndCheckUNC >= 0)
    {
      // Check for UNC, i.e. \\server\share\ part
      int32_t Index = IndCheckUNC;
      for (int32_t SkipSlashes = 2; SkipSlashes > 0; SkipSlashes--)
      {
        for (; Index <= Len; ++Index)
        {
          const TCHAR z = APath[Index];
          if ((z == L'\\') || (z == L'/') || (Index >= Len))
          {
            ++Index;
            if (SkipSlashes == 1)
            {
              if (PrefixType == PPT_UNKNOWN)
              {
                PrefixType = PPT_UNC;
              }

              if (WinXPOnly)
              {
                  //For older OS
                  Result = Index;
              }
            }

            break;
          }
        }
      }
    }

    if (WinXPOnly)
    {
      // Only if we didn't determine any other type
      if (PrefixType == PPT_UNKNOWN)
      {
        if (!PathIsRelative_CorrectedForMicrosoftStupidity(APath.SubString(Result, APath.Length() - Result + 1)))
        {
          PrefixType = PPT_ABSOLUTE;
        }
      }

      // For older OS only
      const int32_t RootLength = PathRootLength(APath.SubString(Result, APath.Length() - Result + 1));
      if (RootLength >= 0)
      {
        Result = RootLength + 1;
      }
    }
    else
    {
      // Only if we didn't determine any other type
      if (PrefixType == PPT_UNKNOWN)
      {
        if (!PathIsRelative_CorrectedForMicrosoftStupidity(APath))
        {
          PrefixType = PPT_ABSOLUTE;
        }
      }
    }
  }

  return Result;
}

UnicodeString MakeUnicodeLargePath(const UnicodeString & APath)
{
  // Convert path from 'into a larger Unicode path, that allows up to 32,767 character length
  UnicodeString Path = APath;
  UnicodeString Result;

  if (!Path.IsEmpty())
  {
    // Determine the type of the existing prefix
    PATH_PREFIX_TYPE PrefixType;
    GetOffsetAfterPathRoot(Path, PrefixType);

    // Assume path to be without change
    Result = Path;

    switch (PrefixType)
    {
      case PPT_ABSOLUTE:
        {
          // First we need to check if its an absolute path relative to the root
          bool AddPrefix = true;
          if ((Path.Length() >= 1) &&
              ((Path[1] == L'\\') || (Path[1] == L'/')))
          {
            AddPrefix = FALSE;

            // Get current root path
            UnicodeString CurrentDir = GetCurrentDir();
            PATH_PREFIX_TYPE PrefixType2; // unused
            int Following = GetOffsetAfterPathRoot(CurrentDir, PrefixType2);
            if (Following > 0)
            {
              AddPrefix = true;
              Result = CurrentDir.SubString(1, Following - 1) + Result.SubString(2, Result.Length() - 1);
            }
          }

          if (AddPrefix)
          {
            // Add \\?\ prefix
            Result = L"\\\\?\\" + Result;
          }
        }
        break;

      case PPT_UNC:
        // First we need to remove the opening slashes for UNC share
        if ((Result.Length() >= 2) &&
            ((Result[1] == L'\\') || (Result[1] == L'/')) &&
            ((Result[2] == L'\\') || (Result[2] == L'/')))
        {
          Result = Result.SubString(3, Result.Length() - 2);
        }

        // Add \\?\UNC\ prefix
        Result = L"\\\\?\\UNC\\" + Result;
        break;

      case PPT_LONG_UNICODE:
      case PPT_LONG_UNICODE_UNC:
        // nothing to do
        break;
    }

  }

  return Result;
}

UnicodeString ApiPath(const UnicodeString & APath)
{
  UnicodeString Path = APath;

  UnicodeString Drive = ExtractFileDrive(Path);
  // This may match even a path like "C:" or "\\server\\share", but we do not really care
  if (Drive.IsEmpty() || (Path.SubString(Drive.Length() + 1, 1) != L"\\"))
  {
    Path = ExpandFileName(Path);
  }

  // Max path for directories is 12 characters shorter than max path for files
  if (Path.Length() >= (nb::NB_MAX_PATH - 12))
  {
    /*if (GetConfiguration() != nullptr)
    {
      GetConfiguration()->Usage->Inc(L"LongPath");
    }*/
    Path = MakeUnicodeLargePath(Path);
  }
  return Path;
}

UnicodeString DisplayableStr(const RawByteString & Str)
{
  bool Displayable = true;
  int32_t Index1 = 1;
  while ((Index1 <= Str.Length()) && Displayable)
  {
    if (((Str[Index1] < '\x20') || (static_cast<uint8_t>(Str[Index1]) >= static_cast<uint8_t >('\x80'))) &&
        (Str[Index1] != '\n') && (Str[Index1] != '\r') && (Str[Index1] != '\t') && (Str[Index1] != '\b'))
    {
      Displayable = false;
    }
    ++Index1;
  }

  UnicodeString Result;
  if (Displayable)
  {
    Result = L"\"";
    for (int32_t Index2 = 1; Index2 <= Str.Length(); ++Index2)
    {
      switch (Str[Index2])
      {
        case '\n':
          Result += L"\\n";
          break;

        case '\r':
          Result += L"\\r";
          break;

        case '\t':
          Result += L"\\t";
          break;

        case '\b':
          Result += L"\\b";
          break;

        case '\\':
          Result += L"\\\\";
          break;

        case '"':
          Result += L"\\\"";
          break;

        default:
          Result += wchar_t(Str[Index2]);
          break;
      }
    }
    Result += L"\"";
  }
  else
  {
    Result = L"0x" + BytesToHex(Str);
  }
  return Result;
}

UnicodeString ByteToHex(uint8_t B, bool UpperCase)
{
  UnicodeString UpperDigits = "0123456789ABCDEF";
  UnicodeString LowerDigits = "0123456789abcdef";

  const wchar_t * Digits = (UpperCase ? UpperDigits.c_str() : LowerDigits.c_str());
  UnicodeString Result;
  Result.SetLength(2);
  Result[1] = Digits[(B & 0xF0) >> 4];
  Result[2] = Digits[(B & 0x0F) >> 0];
  return Result;
}

UnicodeString BytesToHex(const uint8_t *B, uint32_t Length, bool UpperCase, wchar_t Separator)
{
  UnicodeString Result;
  for (uint32_t Index = 0; Index < Length; ++Index)
  {
    Result += ByteToHex(B[Index], UpperCase);
    if ((Separator != L'\0') && (Index < Length - 1))
    {
      Result += Separator;
    }
  }
  return Result;
}

UnicodeString BytesToHex(const RawByteString & Str, bool UpperCase, wchar_t Separator)
{
  return BytesToHex(reinterpret_cast<const uint8_t *>(Str.c_str()), static_cast<uint32_t>(Str.Length()), UpperCase, Separator);
}

UnicodeString CharToHex(wchar_t Ch, bool UpperCase)
{
  return BytesToHex(reinterpret_cast<const uint8_t *>(&Ch), sizeof(Ch), UpperCase);
}

RawByteString HexToBytes(const UnicodeString & Hex)
{
  UnicodeString Digits = "0123456789ABCDEF";
  RawByteString Result;
  const int32_t L = Hex.Length();
  if (L % 2 == 0)
  {
    for (int32_t Index = 1; Index <= Hex.Length(); Index += 2)
    {
      const int32_t P1 = Digits.Pos(::UpCase(Hex[Index]));
      const int32_t P2 = Digits.Pos(::UpCase(Hex[Index + 1]));
      if (P1 <= 0 || P2 <= 0)
      {
        Result.Clear();
        break;
      }
      else
      {
        Result += static_cast<uint8_t>((P1 - 1) * 16 + P2 - 1);
      }
    }
  }
  return Result;
}

uint8_t HexToByte(const UnicodeString & Hex)
{
  static UnicodeString Digits = "0123456789ABCDEF";
  DebugAssert(Hex.Length() == 2);
  const int32_t P1 = Digits.Pos(::UpCase(Hex[1]));
  const int32_t P2 = Digits.Pos(::UpCase(Hex[2]));

  return
    static_cast<uint8_t>(((P1 <= 0) || (P2 <= 0)) ? 0 : (((P1 - 1) << 4) + (P2 - 1)));
}

bool IsLowerCaseLetter(wchar_t Ch)
{
  return (Ch >= L'a') && (Ch <= L'z');
}

bool IsUpperCaseLetter(wchar_t Ch)
{
  return (Ch >= L'A') && (Ch <= L'Z');
}

bool IsLetter(wchar_t Ch)
{
  return IsLowerCaseLetter(Ch) || IsUpperCaseLetter(Ch);
}

bool IsDigit(wchar_t Ch)
{
  return (Ch >= L'0') && (Ch <= L'9');
}

bool IsHex(wchar_t Ch)
{
  return
    IsDigit(Ch) ||
    ((Ch >= L'A') && (Ch <= L'F')) ||
    ((Ch >= L'a') && (Ch <= L'f'));
}

TSearchRecSmart::TSearchRecSmart() noexcept
{
  FLastWriteTimeSource.dwLowDateTime = 0;
  FLastWriteTimeSource.dwHighDateTime = 0;
}

void TSearchRecSmart::Clear()
{
  Size = 0;
  Attr = 0;
  Name = TFileName();
  ExcludeAttr = 0;
  FindHandle = 0;
  memset(&FindData, 0, sizeof(FindData));
  FLastWriteTimeSource.dwLowDateTime = 0;
  FLastWriteTimeSource.dwHighDateTime = 0;
}

TDateTime TSearchRecSmart::GetLastWriteTime() const
{
  if ((FindData.ftLastWriteTime.dwLowDateTime != FLastWriteTimeSource.dwLowDateTime) ||
      (FindData.ftLastWriteTime.dwHighDateTime != FLastWriteTimeSource.dwHighDateTime))
  {
    FLastWriteTimeSource = FindData.ftLastWriteTime;
    FLastWriteTime = FileTimeToDateTime(FLastWriteTimeSource);
  }
  return FLastWriteTime;
}

bool TSearchRecSmart::IsRealFile() const
{
  return ::IsRealFile(Name);
}

bool TSearchRecSmart::IsDirectory() const
{
  return FLAGSET(Attr, faDirectory);
}

bool TSearchRecSmart::IsHidden() const
{
  return FLAGSET(Attr, faHidden);
}

UnicodeString TSearchRecChecked::GetFilePath() const
{
  return TPath::Combine(Dir, Name);
}

TSearchRecOwned::~TSearchRecOwned() noexcept
{
  if (Opened)
  {
    base::FindClose(*this);
  }
}

void TSearchRecOwned::Close()
{
  base::FindClose(*this);
  Opened = false;
}

DWORD FindCheck(DWORD Result, const UnicodeString & APath)
{
  if ((Result != ERROR_SUCCESS) &&
      (Result != ERROR_FILE_NOT_FOUND) &&
      (Result != ERROR_NO_MORE_FILES))
  {
    throw EOSExtException(FMTLOAD(FIND_FILE_ERROR, APath), Result);
  }
  return Result;
}

DWORD FindFirstUnchecked(const UnicodeString & APath, DWORD LocalFileAttrs, TSearchRecChecked &F)
{
  F.Path = APath;
  F.Dir = ExtractFilePath(APath);
  DWORD Result = base::FindFirst(ApiPath(APath), LocalFileAttrs, F);
  F.Opened = (Result == 0);
  return Result;
}

DWORD FindFirstChecked(const UnicodeString & APath, DWORD LocalFileAttrs, TSearchRecChecked & F)
{
  const DWORD Result = FindFirstUnchecked(APath, LocalFileAttrs, F);
  return FindCheck(Result, F.Path);
}

// Equivalent to FindNext, just to complement to FindFirstUnchecked
DWORD FindNextUnchecked(TSearchRecChecked & F)
{
  return base::FindNext(F);
}

// It can make sense to use FindNextChecked, even if unchecked FindFirst is used.
// I.e. even if we do not care that FindFirst failed, if FindNext
// fails after successful FindFirst, it means some terrible problem
DWORD FindNextChecked(TSearchRecChecked & F)
{
  return FindCheck(FindNextUnchecked(F), F.Path);
}

bool FileSearchRec(const UnicodeString & AFileName, TSearchRec &Rec)
{
  const DWORD FindAttrs = faReadOnly | faHidden | faSysFile | faDirectory | faArchive;
  const bool Result = (base::FindFirst(ApiPath(AFileName), FindAttrs, Rec) == 0);
  if (Result)
  {
    base::FindClose(Rec);
  }
  return Result;
}

void CopySearchRec(const TSearchRec & Source, TSearchRec & Dest)
{
  // Strangely issues a compiler warning (W8111 due to TSearchRec::Time), when used in Script.cpp, but not here.
  Dest.Assign(Source);
}

bool IsRealFile(const UnicodeString & AFileName)
{
  return (AFileName != THISDIRECTORY) && (AFileName != PARENTDIRECTORY);
}

UnicodeString GetOSInfo()
{
  UnicodeString Result = WindowsVersionLong();
  AddToList(Result, WindowsProductName(), TitleSeparator);
  return Result;
}

UnicodeString GetEnvironmentInfo()
{
  UnicodeString Result; //TODO: = FORMAT("WinSCP %s (OS %s)", GetConfiguration()->GetVersionStr(), GetOSInfo());
  return Result;
}

void ProcessLocalDirectory(const UnicodeString & ADirName,
  TProcessLocalFileEvent CallBackFunc, void * Param,
  DWORD FindAttrs)
{
  DebugAssert(CallBackFunc);
  if (FindAttrs == INVALID_FILE_ATTRIBUTES)
  {
    FindAttrs = faReadOnly | faHidden | faSysFile | faDirectory | faArchive;
  }

  TSearchRecOwned SearchRec;
  if (FindFirstChecked(TPath::Combine(ADirName, AnyMask), FindAttrs, SearchRec) == 0)
  {
    do
    {
      if (SearchRec.IsRealFile())
      {
        CallBackFunc(SearchRec.GetFilePath(), SearchRec, Param);
      }

    } while (FindNextChecked(SearchRec) == 0);
  }
}

DWORD FileGetAttrFix(const UnicodeString & AFileName)
{
  // Already called with ApiPath

  int Result;
  int Tries = 2;
  do
  {
    // WORKAROUND:
    // FileGetAttr when called for link with FollowLink set (default) will always fail on pre-Vista
    // as it calls InternalGetFileNameFromSymLink, which test for CheckWin32Version(6, 0)
    Result = GetFileAttributes(AFileName.c_str());
    if ((Result >= 0) && FLAGSET(Result, faSymLink) && IsWinVista())
    {
      try
      {
        UnicodeString TargetName;
        // WORKAROUND:
        // On Samba, InternalGetFileNameFromSymLink fails and returns true but empty target.
        // That confuses FileGetAttr, which returns attributes of the parent folder instead.
        // Using FileGetSymLinkTarget solves the problem, as it returns false.
        if (!FileGetSymLinkTarget(AFileName, TargetName))
        {
          // FileGetAttr would return faInvalid (-1), but we want to allow an upload from Samba,
          // so returning the symlink attributes => noop
        }
        else
        {
          Result = GetFileAttributes(ApiPath(TargetName).c_str());
        }
      }
      catch (EOSError & DebugUsedArg(E))
      {
        Result = -1;
      }
      catch (EDirectoryNotFoundException & DebugUsedArg(E)) // throws by FileSystemAttributes
      {
        Result = -1;
      }
    }
    Tries--;
  }
  // When referring to files in some special symlinked locations
  // (like a deduplicated drive or a commvault archive), the first call to FileGetAttr failed.
  // Possibly this issue is resolved by our re-implementation of FileGetAttr above.
  // But as we have no way to test it, keeping the retry here.
  while ((Result < 0) && (Tries > 0));

  return Result;
}

TDateTime EncodeDateVerbose(Word Year, Word Month, Word Day)
{
  TDateTime Result;
  try
  {
    Result = EncodeDate(Year, Month, Day);
  }
  catch (EConvertError &E)
  {
    throw EConvertError(FORMAT("%s [%04u-%02u-%02u]", E.Message, int(Year), int(Month), int(Day)));
  }
  return Result;
}

TDateTime EncodeTimeVerbose(Word Hour, Word Min, Word Sec, Word MSec)
{
  TDateTime Result;
  try
  {
    Result = EncodeTime(Hour, Min, Sec, MSec);
  }
  catch (EConvertError &E)
  {
    throw EConvertError(FORMAT("%s [%02u:%02u:%02u.%04u]", E.Message, int(Hour), int(Min), int(Sec), int(MSec)));
  }
  return Result;
}

TDateTime SystemTimeToDateTimeVerbose(const SYSTEMTIME &SystemTime)
{
  try
  {
    TDateTime DateTime = SystemTimeToDateTime(SystemTime);
    return DateTime;
  }
  catch (EConvertError &E)
  {
    throw EConvertError(FORMAT("%s [%d-%2.2d-%2.2d %2.2d:%2.2d:%2.2d.%3.3d]", E.Message, int(SystemTime.wYear), int(SystemTime.wMonth), int(SystemTime.wDay), int(SystemTime.wHour), int(SystemTime.wMinute), int(SystemTime.wSecond), int(SystemTime.wMilliseconds)));
  }
}

struct TDateTimeParams : public TObject
{
  TDateTimeParams() :
    BaseDifference(0.0),
    BaseDifferenceSec(0),
    CurrentDaylightDifference(0.0),
    CurrentDaylightDifferenceSec(0),
    CurrentDifference(0.0),
    CurrentDifferenceSec(0),
    StandardDifference(0.0),
    StandardDifferenceSec(0),
    DaylightDifference(0.0),
    DaylightDifferenceSec(0),
    DaylightHack(false)
  {
    nb::ClearStruct(SystemStandardDate);
    nb::ClearStruct(SystemDaylightDate);
  }

  TDateTime UnixEpoch{};
  double BaseDifference{0.};
  int32_t BaseDifferenceSec{};
  // All Current* are actually global, not per-year and
  // are valid for Year 0 (current) only
  double CurrentDaylightDifference{};
  int32_t CurrentDaylightDifferenceSec{};
  double CurrentDifference{};
  int32_t CurrentDifferenceSec{};
  double StandardDifference{};
  int32_t StandardDifferenceSec{};
  double DaylightDifference{};
  int32_t DaylightDifferenceSec{};
  SYSTEMTIME SystemStandardDate{};
  SYSTEMTIME SystemDaylightDate{};
  TDateTime StandardDate{};
  TDateTime DaylightDate{};
  UnicodeString StandardName;
  UnicodeString DaylightName;
  // This is actually global, not per-year
  bool DaylightHack{};

  bool HasDST() const
  {
    // On some systems it occurs that StandardDate is unset, while
    // DaylightDate is set. MSDN states that this is invalid and
    // should be treated as if there is no daylight saving.
    // So check both.
    return
      (SystemStandardDate.wMonth != 0) &&
      (SystemDaylightDate.wMonth != 0);
  }

  bool SummerDST() const
  {
    return HasDST() && (DaylightDate < StandardDate);
  }
};

using TYearlyDateTimeParams = nb::map_t<int, TDateTimeParams>;
static TYearlyDateTimeParams YearlyDateTimeParams;
static TCriticalSection DateTimeParamsSection;
static void EncodeDSTMargin(const SYSTEMTIME &Date, uint16_t Year,
  TDateTime & Result);

static uint16_t DecodeYear(const TDateTime &DateTime)
{
  uint16_t Year, Month, Day;
  DecodeDate(DateTime, Year, Month, Day);
  return Year;
}

static const TDateTimeParams *GetDateTimeParams(uint16_t Year)
{
  TGuard Guard(DateTimeParamsSection); nb::used(Guard);

  TDateTimeParams *Result;

  const TYearlyDateTimeParams::iterator it = YearlyDateTimeParams.find(Year);
  if (it != YearlyDateTimeParams.end())
  {
    Result = &(*it).second;
  }
  else
  {
    // creates new entry as a side effect
    Result = &YearlyDateTimeParams[Year];
    TIME_ZONE_INFORMATION TZI;

    uint32_t GTZI;

    HINSTANCE const Kernel32 = ::GetModuleHandle(L"kernel32.dll");
    typedef BOOL (WINAPI * TGetTimeZoneInformationForYear)(USHORT wYear, PDYNAMIC_TIME_ZONE_INFORMATION pdtzi, LPTIME_ZONE_INFORMATION ptzi);
    TGetTimeZoneInformationForYear const GetTimeZoneInformationForYear =
      reinterpret_cast<TGetTimeZoneInformationForYear>(::GetProcAddress(Kernel32, "GetTimeZoneInformationForYear"));

    if ((Year == 0) || (GetTimeZoneInformationForYear == nullptr))
    {
      GTZI = GetTimeZoneInformation(&TZI);
    }
    else
    {
      GetTimeZoneInformationForYear(Year, nullptr, &TZI);
      GTZI = TIME_ZONE_ID_UNKNOWN;
    }

    switch (GTZI)
    {
      case TIME_ZONE_ID_UNKNOWN:
        Result->CurrentDaylightDifferenceSec = 0;
        break;

      case TIME_ZONE_ID_STANDARD:
        Result->CurrentDaylightDifferenceSec = TZI.StandardBias;
        break;

      case TIME_ZONE_ID_DAYLIGHT:
        Result->CurrentDaylightDifferenceSec = TZI.DaylightBias;
        break;

      case TIME_ZONE_ID_INVALID:
      default:
        throw Exception(FMTLOAD(TIMEZONE_ERROR));
    }

    Result->BaseDifferenceSec = TZI.Bias;
    Result->BaseDifference = nb::ToDouble(TZI.Bias) / MinsPerDay;
    Result->BaseDifferenceSec *= SecsPerMin;

    Result->CurrentDifferenceSec = TZI.Bias +
      Result->CurrentDaylightDifferenceSec;
    Result->CurrentDifference =
      nb::ToDouble(Result->CurrentDifferenceSec) / MinsPerDay;
    Result->CurrentDifferenceSec *= SecsPerMin;

    Result->CurrentDaylightDifference =
      nb::ToDouble(Result->CurrentDaylightDifferenceSec) / MinsPerDay;
    Result->CurrentDaylightDifferenceSec *= SecsPerMin;

    Result->DaylightDifferenceSec = TZI.DaylightBias * SecsPerMin;
    Result->DaylightDifference = nb::ToDouble(TZI.DaylightBias) / MinsPerDay;
    Result->StandardDifferenceSec = TZI.StandardBias * SecsPerMin;
    Result->StandardDifference = nb::ToDouble(TZI.StandardBias) / MinsPerDay;

    Result->SystemStandardDate = TZI.StandardDate;
    Result->SystemDaylightDate = TZI.DaylightDate;

    const uint16_t AYear = (Year != 0) ? Year : DecodeYear(Now());
    if (Result->HasDST())
    {
      EncodeDSTMargin(Result->SystemStandardDate, AYear, Result->StandardDate);
      EncodeDSTMargin(Result->SystemDaylightDate, AYear, Result->DaylightDate);
    }

    Result->StandardName = TZI.StandardName;
    Result->DaylightName = TZI.DaylightName;

    Result->DaylightHack = !IsWin7();
  }

  return Result;
}

static void EncodeDSTMargin(const SYSTEMTIME &Date, uint16_t Year,
  TDateTime & Result)
{
  if (Date.wYear == 0)
  {
    const TDateTime Temp = EncodeDateVerbose(Year, Date.wMonth, 1);
    Result = Temp + ((Date.wDayOfWeek - DayOfWeek(Temp) + 8) % 7) +
      (7 * (Date.wDay - 1));
    // Day 5 means, the last occurrence of day-of-week in month
    if (Date.wDay == 5)
    {
      uint16_t Month = static_cast<uint16_t>(Date.wMonth + 1);
      if (Month > 12)
      {
        Month = static_cast<uint16_t>(Month - 12);
        Year++;
      }

      if (Result >= EncodeDateVerbose(Year, Month, 1))
      {
        Result -= 7;
      }
    }
    Result += EncodeTimeVerbose(Date.wHour, Date.wMinute, Date.wSecond,
      Date.wMilliseconds);
  }
  else
  {
    Result = EncodeDateVerbose(Year, Date.wMonth, Date.wDay) +
      EncodeTimeVerbose(Date.wHour, Date.wMinute, Date.wSecond, Date.wMilliseconds);
  }
}

static bool IsDateInDST(const TDateTime & DateTime)
{

  const TDateTimeParams * Params = GetDateTimeParams(DecodeYear(DateTime));

  bool Result;

  // On some systems it occurs that StandardDate is unset, while
  // DaylightDate is set. MSDN states that this is invalid and
  // should be treated as if there is no daylight saving.
  // So check both.
  if (!Params->HasDST())
  {
    Result = false;
  }
  else
  {

    if (Params->SummerDST())
    {
      Result =
        (DateTime >= Params->DaylightDate) &&
        (DateTime < Params->StandardDate);
    }
    else
    {
      Result =
        (DateTime < Params->StandardDate) ||
        (DateTime >= Params->DaylightDate);
    }
  }
  return Result;
}

bool UsesDaylightHack()
{
  return GetDateTimeParams(0)->DaylightHack;
}

TDateTime UnixToDateTime(int64_t TimeStamp, TDSTMode DSTMode)
{
  DebugAssert(int(EncodeDateVerbose(1970, 1, 1)) == UnixDateDelta);

  TDateTime Result = TDateTime(UnixDateDelta + (nb::ToDouble(TimeStamp) / SecsPerDay));
  const TDateTimeParams *Params = GetDateTimeParams(DecodeYear(Result));

  if (Params->DaylightHack)
  {
    if ((DSTMode == dstmWin) || (DSTMode == dstmUnix))
    {
      const TDateTimeParams *CurrentParams = GetDateTimeParams(0);
      Result -= CurrentParams->CurrentDifference;
    }
    else if (DSTMode == dstmKeep)
    {
      Result -= Params->BaseDifference;
    }
  }
  else
  {
    Result -= Params->BaseDifference;
  }

  if ((DSTMode == dstmUnix) || (DSTMode == dstmKeep))
  {
    Result -= DSTDifferenceForTime(Result);
  }

  return Result;
}

int64_t Round(double Number)
{
  double Floor = floor(Number);
  double Ceil = ceil(Number);
  return nb::ToInt64(((Number - Floor) > (Ceil - Number)) ? Ceil : Floor);
}

bool TryRelativeStrToDateTime(const UnicodeString & AStr, TDateTime & DateTime, bool Add)
{
  UnicodeString S = AStr.Trim();
  int32_t Index = 1;
  if (SameText(S, L"today"))
  {
    S = L"0DS";
  }
  else if (SameText(S, L"yesterday"))
  {
    S = L"1DS";
  }

  while ((Index <= S.Length()) && IsDigit(S[Index]))
  {
    ++Index;
  }
  UnicodeString NumberStr = S.SubString(1, Index - 1);
  int64_t Number = 0;
  bool Result = TryStrToInt64(NumberStr, Number);
  if (Result)
  {
    if (!Add)
    {
      Number = -Number;
    }
    S.Delete(1, Index - 1);
    S = S.Trim().UpperCase();
    bool Start = (S.Length() == 2) && (S[2] == L'S');
    if (Start)
    {
      S.SetLength(S.Length() - 1);
    }
    DateTime = Now();
    // These may not overlap with ParseSize (K, M and G)
    if (S == "S")
    {
      DateTime = IncSecond(DateTime, Number);
      if (Start)
      {
        DateTime = IncMilliSecond(DateTime, -static_cast<int>(MilliSecondOfTheSecond(DateTime)));
      }
    }
    else if (S == "N")
    {
      DateTime = IncMinute(DateTime, Number);
      if (Start)
      {
        DateTime = IncMilliSecond(DateTime, -static_cast<int>(MilliSecondOfTheMinute(DateTime)));
      }
    }
    else if (S == "H")
    {
      DateTime = IncHour(DateTime, Number);
      if (Start)
      {
        DateTime = IncMilliSecond(DateTime, -static_cast<int>(MilliSecondOfTheHour(DateTime)));
      }
    }
    else if (S == "D")
    {
      DateTime = IncDay(DateTime, Number);
      if (Start)
      {
        DateTime = IncMilliSecond(DateTime, -static_cast<int>(MilliSecondOfTheDay(DateTime)));
      }
    }
    else if (S == "Y")
    {
      DateTime = IncYear(DateTime, Number);
      if (Start)
      {
        DateTime = IncMilliSecond(DateTime, -MilliSecondOfTheYear(DateTime));
      }
    }
    else
    {
      Result = false;
    }
  }
  return Result;
}

bool TryStrToDateTimeStandard(const UnicodeString & S, TDateTime & Value)
{
  TFormatSettings FormatSettings = TFormatSettings::Create(GetDefaultLCID());
  FormatSettings.DateSeparator = L'-';
  FormatSettings.TimeSeparator = L':';
  FormatSettings.ShortDateFormat = "yyyy/mm/dd";
  FormatSettings.ShortTimeFormat = "hh:nn:ss";

  return TryStrToDateTime(S, Value, FormatSettings);
}

constexpr wchar_t KiloSize = L'K';
constexpr wchar_t MegaSize = L'M';
constexpr wchar_t GigaSize = L'G';

// Keep consistent with parse_blocksize
bool TryStrToSize(const UnicodeString & ASizeStr, int64_t &ASize)
{
  UnicodeString SizeStr = ASizeStr;
  int32_t Index = 0;
  while ((Index + 1 <= SizeStr.Length()) && IsDigit(SizeStr[Index + 1]))
  {
    Index++;
  }
  bool Result =
    (Index > 0) && TryStrToInt64(SizeStr.SubString(1, Index), ASize);
  if (Result)
  {
    SizeStr = SizeStr.SubString(Index + 1, SizeStr.Length() - Index).Trim();
    if (!SizeStr.IsEmpty())
    {
      Result = (SizeStr.Length() == 1);
      if (Result)
      {
        const wchar_t Unit = towupper(SizeStr[1]);
        switch (Unit)
        {
          case GigaSize:
            ASize *= 1024;
            // fallthru
          case MegaSize:
            ASize *= 1024;
            // fallthru
          case KiloSize:
            ASize *= 1024;
            break;
          default:
            Result = false;
        }
      }
    }
  }
  return Result;
}

UnicodeString SizeToStr(int64_t ASize)
{
  UnicodeString Result;
  if ((ASize <= 0) || ((ASize % 1024) != 0))
  {
    Result = Int64ToStr(ASize);
  }
  else
  {
    ASize /= 1024;
    if ((ASize % 1024) != 0)
    {
      Result = Int64ToStr(ASize) + KiloSize;
    }
    else
    {
      ASize /= 1024;
      if ((ASize % 1024) != 0)
      {
        Result = Int64ToStr(ASize) + MegaSize;
      }
      else
      {
        ASize /= 1024;
        Result = Int64ToStr(ASize) + GigaSize;
      }
    }
  }
  return Result;
}

static int64_t DateTimeToUnix(const TDateTime &DateTime)
{
  const TDateTimeParams *CurrentParams = GetDateTimeParams(0);

  DebugAssert(int(EncodeDateVerbose(1970, 1, 1)) == UnixDateDelta);

  return Round(nb::ToDouble(DateTime - UnixDateDelta) * SecsPerDay) +
    CurrentParams->CurrentDifferenceSec;
}

FILETIME DateTimeToFileTime(const TDateTime &DateTime,
  TDSTMode /*DSTMode*/)
{
  int64_t UnixTimeStamp = ::DateTimeToUnix(DateTime);

  const TDateTimeParams *Params = GetDateTimeParams(DecodeYear(DateTime));
  if (!Params->DaylightHack)
  {
    // We should probably use reversed code of FileTimeToDateTime here instead of custom implementation

    // We are incrementing and decrementing BaseDifferenceSec because it
    // can actually change between years
    // (as it did in Belarus from GMT+2 to GMT+3 between 2011 and 2012)

    UnixTimeStamp += (IsDateInDST(DateTime) ?
      Params->DaylightDifferenceSec : Params->StandardDifferenceSec) +
      Params->BaseDifferenceSec;

    const TDateTimeParams *CurrentParams = GetDateTimeParams(0);
    UnixTimeStamp -=
      CurrentParams->CurrentDaylightDifferenceSec +
      CurrentParams->BaseDifferenceSec;

  }

  FILETIME Result;
  (*reinterpret_cast<int64_t *>(&(Result)) = (nb::ToInt64(UnixTimeStamp) + 11644473600LL) * 10000000LL);

  return Result;
}

TDateTime FileTimeToDateTime(const FILETIME &FileTime)
{
  // duplicated in DirView.pas
  TDateTime Result;
  // The 0xFFF... is sometime seen for invalid timestamps,
  // it would cause failure in SystemTimeToDateTime below
  if (FileTime.dwLowDateTime == std::numeric_limits<DWORD>::max())
  {
    Result = MinDateTime;
  }
  else
  {
    SYSTEMTIME SysTime;
    if (!UsesDaylightHack())
    {
      SYSTEMTIME UniversalSysTime;
      FileTimeToSystemTime(&FileTime, &UniversalSysTime);
      SystemTimeToTzSpecificLocalTime(nullptr, &UniversalSysTime, &SysTime);
    }
    else
    {
      FILETIME LocalFileTime;
      FileTimeToLocalFileTime(&FileTime, &LocalFileTime);
      FileTimeToSystemTime(&LocalFileTime, &SysTime);
    }
    Result = SystemTimeToDateTimeVerbose(SysTime);
  }
  return Result;
}

int64_t ConvertTimestampToUnix(const FILETIME &FileTime,
  TDSTMode DSTMode)
{
  NB_STATIC_ASSERT(sizeof(FILETIME) == sizeof(int64_t), "ConvertTimestampToUnix: unexpected FILETIME size");
  int64_t Result = *reinterpret_cast<int64_t *>(const_cast<FILETIME *>(&(FileTime))) / 10000000LL - 11644473600LL;
  if (UsesDaylightHack())
  {
    if ((DSTMode == dstmUnix) || (DSTMode == dstmKeep))
    {
      FILETIME LocalFileTime;
      SYSTEMTIME SystemTime;
      FileTimeToLocalFileTime(&FileTime, &LocalFileTime);
      FileTimeToSystemTime(&LocalFileTime, &SystemTime);
      const TDateTime DateTime = SystemTimeToDateTimeVerbose(SystemTime);
      const TDateTimeParams *Params = GetDateTimeParams(DecodeYear(DateTime));
      Result += (IsDateInDST(DateTime) ?
        Params->DaylightDifferenceSec : Params->StandardDifferenceSec);

      if (DSTMode == dstmKeep)
      {
        const TDateTimeParams *CurrentParams = GetDateTimeParams(0);
        Result -= CurrentParams->CurrentDaylightDifferenceSec;
      }
    }
  }
  else
  {
    if (DSTMode == dstmWin)
    {
      FILETIME LocalFileTime;
      SYSTEMTIME SystemTime;
      FileTimeToLocalFileTime(&FileTime, &LocalFileTime);
      FileTimeToSystemTime(&LocalFileTime, &SystemTime);
      const TDateTime DateTime = SystemTimeToDateTimeVerbose(SystemTime);
      const TDateTimeParams * Params = GetDateTimeParams(DecodeYear(DateTime));
      Result -= (IsDateInDST(DateTime) ?
        Params->DaylightDifferenceSec : Params->StandardDifferenceSec);
    }
  }

  return Result;
}

TDateTime ConvertTimestampToUTC(const TDateTime &ADateTime)
{
  TDateTime DateTime = ADateTime;

  const TDateTimeParams *Params = GetDateTimeParams(DecodeYear(DateTime));
  DateTime += DSTDifferenceForTime(DateTime);
  DateTime += Params->BaseDifference;

  if (Params->DaylightHack)
  {
    const TDateTimeParams *CurrentParams = GetDateTimeParams(0);
    DateTime += CurrentParams->CurrentDaylightDifference;
  }

  return DateTime;
}

TDateTime ConvertTimestampFromUTC(const TDateTime & ADateTime)
{
  TDateTime DateTime = ADateTime;

  const TDateTimeParams * Params = GetDateTimeParams(DecodeYear(DateTime));
  DateTime -= DSTDifferenceForTime(DateTime);
  DateTime -= Params->BaseDifference;

  if (Params->DaylightHack)
  {
    const TDateTimeParams *CurrentParams = GetDateTimeParams(0);
    DateTime -= CurrentParams->CurrentDaylightDifference;
  }

  return DateTime;
}

int64_t ConvertTimestampToUnixSafe(const FILETIME &FileTime,
  TDSTMode DSTMode)
{
  int64_t Result;
  if ((FileTime.dwLowDateTime == 0) &&
      (FileTime.dwHighDateTime == 0))
  {
    Result = ::DateTimeToUnix(Now());
  }
  else
  {
    Result = ::ConvertTimestampToUnix(FileTime, DSTMode);
  }
  return Result;
}

double DSTDifferenceForTime(const TDateTime &DateTime)
{
  double Result;
  const TDateTimeParams *Params = GetDateTimeParams(DecodeYear(DateTime));
  if (IsDateInDST(DateTime))
  {
    Result = Params->DaylightDifference;
  }
  else
  {
    Result = Params->StandardDifference;
  }
  return Result;
}

TDateTime AdjustDateTimeFromUnix(const TDateTime & ADateTime, TDSTMode DSTMode)
{
  TDateTime DateTime = ADateTime;

  const TDateTimeParams * Params = GetDateTimeParams(DecodeYear(DateTime));

  if (Params->DaylightHack)
  {
    if ((DSTMode == dstmWin) || (DSTMode == dstmUnix))
    {
      const TDateTimeParams * CurrentParams = GetDateTimeParams(0);
      DateTime = DateTime - CurrentParams->CurrentDaylightDifference;
    }

    if (!IsDateInDST(DateTime))
    {
      if (DSTMode == dstmWin)
      {
        DateTime = DateTime - Params->DaylightDifference;
      }
    }
    else
    {
      DateTime = DateTime - Params->StandardDifference;
    }
  }
  else
  {
    if (DSTMode == dstmWin)
    {
      DateTime = DateTime + DSTDifferenceForTime(DateTime);
    }
  }

  return DateTime;
}

UnicodeString FixedLenDateTimeFormat(const UnicodeString & Format)
{
  UnicodeString Result = Format;
  bool AsIs = false;

  int32_t Index = 1;
  while (Index <= Result.Length())
  {
    const wchar_t F = Result[Index];
    if ((F == L'\'') || (F == L'\"'))
    {
      AsIs = !AsIs;
      ++Index;
    }
    else if (!AsIs && ((F == L'a') || (F == L'A')))
    {
      if (Result.SubString(Index, 5).LowerCase() == L"am/pm")
      {
        Index += 5;
      }
      else if (Result.SubString(Index, 3).LowerCase() == L"a/p")
      {
        Index += 3;
      }
      else if (Result.SubString(Index, 4).LowerCase() == L"ampm")
      {
        Index += 4;
      }
      else
      {
        ++Index;
      }
    }
    else
    {
      if (!AsIs && (wcschr(L"dDeEmMhHnNsS", F) != nullptr) &&
          ((Index == Result.Length()) || (Result[Index + 1] != F)))
      {
        Result.Insert(F, Index);
      }

      while ((Index <= Result.Length()) && (F == Result[Index]))
      {
        ++Index;
      }
    }
  }

  return Result;
}

UnicodeString FormatTimeZone(int32_t Sec)
{
  TTimeSpan Span = TTimeSpan::FromSeconds(Sec);
  UnicodeString Str;
  if ((Span.Seconds == 0) && (Span.Minutes == 0))
  {
    Str = FORMAT("%d", -Span.Hours);
  }
  else if (Span.Seconds == 0)
  {
    Str = FORMAT("%d:%2.2d", -Span.Hours, abs(Span.Minutes));
  }
  else
  {
    Str = FORMAT("%d:%2.2d:%2.2d", -Span.Hours, abs(Span.Minutes), abs(Span.Seconds));
  }
  Str = ((Span <= TTimeSpan::GetZero()) ? L"+" : L"") + Str;
  return Str;
}

UnicodeString GetTimeZoneLogString()
{
  const TDateTimeParams * CurrentParams = GetDateTimeParams(0);

  UnicodeString Result =
    FORMAT("Current: GMT%s", FormatTimeZone(CurrentParams->CurrentDifferenceSec));

  if (!CurrentParams->HasDST())
  {
    Result += FORMAT(" (%s), No DST", CurrentParams->StandardName);
  }
  else
  {
    Result +=
      FORMAT(", Standard: GMT%s (%s), DST: GMT%s (%s), DST Start: %s, DST End: %s",
        FormatTimeZone(CurrentParams->BaseDifferenceSec + CurrentParams->StandardDifferenceSec),
         CurrentParams->StandardName,
         FormatTimeZone(CurrentParams->BaseDifferenceSec + CurrentParams->DaylightDifferenceSec),
         CurrentParams->DaylightName,
         CurrentParams->DaylightDate.GetDateString(),
         CurrentParams->StandardDate.GetDateString());
  }

  return Result;
}

bool AdjustClockForDSTEnabled()
{
  // Windows XP deletes the DisableAutoDaylightTimeSet value when it is off
  // (the later versions set it to DynamicDaylightTimeDisabled to 0)
  bool DynamicDaylightTimeDisabled = false;
  std::unique_ptr<TRegistry> Registry(std::make_unique<TRegistry>());
  Registry->SetAccess(KEY_READ);
  try
  {
    Registry->SetRootKey(HKEY_LOCAL_MACHINE);
    if (Registry->OpenKey("SYSTEM", false) &&
        Registry->OpenKey("CurrentControlSet", false) &&
        Registry->OpenKey("Control", false) &&
        Registry->OpenKey("TimeZoneInformation", false))
    {
      if (Registry->ValueExists("DynamicDaylightTimeDisabled"))
      {
        DynamicDaylightTimeDisabled = Registry->ReadBool("DynamicDaylightTimeDisabled");
      }
      // WORKAROUND
      // Windows XP equivalent
      else if (Registry->ValueExists("DisableAutoDaylightTimeSet"))
      {
        DynamicDaylightTimeDisabled = Registry->ReadBool("DisableAutoDaylightTimeSet");
      }
    }
  }
  catch (...)
  {
    DEBUG_PRINTF("AdjustClockForDSTEnabled: error");
  }
  return !DynamicDaylightTimeDisabled;
}

UnicodeString StandardDatestamp()
{
#if defined(__BORLANDC__)
  return FormatDateTime(L"yyyy'-'mm'-'dd", ConvertTimestampToUTC(Now()));
#else
  TDateTime DT = ::ConvertTimestampToUTC(Now());
  uint16_t Y, M, D, H, N, S, MS;
  DT.DecodeDate(Y, M, D);
  DT.DecodeTime(H, N, S, MS);
  UnicodeString Result = FORMAT("%04d-%02d-%02d", Y, M, D);
  return Result;
#endif
}

UnicodeString StandardTimestamp(const TDateTime &DateTime)
{
#if defined(__BORLANDC__)
  return FormatDateTime(L"yyyy'-'mm'-'dd'T'hh':'nn':'ss'.'zzz'Z'", ConvertTimestampToUTC(DateTime));
#else
  TDateTime DT = ::ConvertTimestampToUTC(DateTime);
  uint16_t Y, M, D, H, N, S, MS;
  DT.DecodeDate(Y, M, D);
  DT.DecodeTime(H, N, S, MS);
  UnicodeString Result = FORMAT("%04d-%02d-%02dT%02d:%02d:%02d.%03dZ", Y, M, D, H, N, S, MS);
  return Result;
#endif
}

UnicodeString StandardTimestamp()
{
  return StandardTimestamp(Now());
}

static TDateTime TwoSeconds(0, 0, 2, 0);
int32_t CompareFileTime(const TDateTime &T1, const TDateTime &T2)
{
  const TDateTime TwoSeconds(0, 0, 2, 0);
  // "FAT" time precision
  // (when one time is seconds-precision and other is millisecond-precision,
  // we may have times like 12:00:00.000 and 12:00:01.999, which should
  // be treated the same)
  int32_t Result;
  if (T1 == T2)
  {
    // just optimization
    Result = 0;
  }
  else if ((T1 < T2) && (T2 - T1 >= TwoSeconds))
  {
    Result = -1;
  }
  else if ((T1 > T2) && (T1 - T2 >= TwoSeconds))
  {
    Result = 1;
  }
  else
  {
    Result = 0;
  }
  return Result;
}

int32_t TimeToMSec(const TDateTime &T)
{
  return int(Round(double(T) * double(MSecsPerDay)));
}

int32_t TimeToSeconds(const TDateTime &T)
{
  return TimeToMSec(T) / MSecsPerSec;
}

int32_t TimeToMinutes(const TDateTime &T)
{
  return TimeToSeconds(T) / SecsPerMin;
}

static bool DoRecursiveDeleteFile(
  const UnicodeString & AFileName, bool ToRecycleBin, UnicodeString & AErrorPath, int32_t& Deleted)
{
  bool Result;
  Deleted = 0;

  UnicodeString ErrorPath = AFileName;

  if (!ToRecycleBin)
  {
    TSearchRecChecked InitialSearchRec;
    Result = FileSearchRec(AFileName, InitialSearchRec);
    if (Result)
    {
      if (!InitialSearchRec.IsDirectory())
      {
        Result = ::SysUtulsRemoveFile(ApiPath(AFileName));
        if (Result)
        {
          Deleted++;
        }
      }
      else
      {
        TSearchRecOwned SearchRec;
        Result = (FindFirstUnchecked(AFileName + L"\\*", faAnyFile, SearchRec) == 0);

        if (Result)
        {
          do
          {
            UnicodeString FileName2 = SearchRec.GetFilePath();
            if (SearchRec.IsDirectory())
            {
              if (SearchRec.IsRealFile())
              {
                Result = DoRecursiveDeleteFile(FileName2, DebugAlwaysFalse(ToRecycleBin), AErrorPath, Deleted);
              }
            }
            else
            {
              Result = ::SysUtulsRemoveFile(ApiPath(FileName2));
              if (!Result)
              {
                AErrorPath = FileName2;
              }
              else
              {
                Deleted++;
              }
            }
          }
          while (Result && (FindNextUnchecked(SearchRec) == 0));

          if (Result)
          {
            Result = ::SysUtulsRemoveDir(ApiPath(AFileName));
            if (Result)
            {
              Deleted++;
            }
          }
        }
      }
    }
  }
  else
  {
    SHFILEOPSTRUCT Data;

    __removed memset(&Data, 0, sizeof(Data));
    nb::ClearStruct(Data);
    Data.hwnd = nullptr;
    Data.wFunc = FO_DELETE;
    // SHFileOperation does not support long paths anyway
    UnicodeString FileList(ApiPath(AFileName));
    FileList.SetLength(FileList.Length() + 2);
    FileList[FileList.Length() - 1] = L'\0';
    FileList[FileList.Length()] = L'\0';
    Data.pFrom = FileList.c_str();
    Data.pTo = L"\0\0"; // this will actually give one null more than needed
    Data.fFlags = FOF_NOCONFIRMATION | FOF_RENAMEONCOLLISION | FOF_NOCONFIRMMKDIR |
      FOF_NOERRORUI | FOF_SILENT;
    if (DebugAlwaysTrue(ToRecycleBin))
    {
      Data.fFlags |= FOF_ALLOWUNDO;
    }
    int ErrorCode = ::SHFileOperation(&Data);
    Result = (ErrorCode == 0);
    if (!Result)
    {
      // according to MSDN, SHFileOperation may return following non-Win32
      // error codes
      if (((ErrorCode >= 0x71) && (ErrorCode <= 0x88)) ||
          (ErrorCode == 0xB7) || (ErrorCode == 0x402) || (ErrorCode == 0x10000) ||
          (ErrorCode == 0x10074))
      {
        ErrorCode = 0;
      }
      ::SetLastError(static_cast<DWORD>(ErrorCode));
    }

    if (Result)
    {
      Deleted = 1;
    }
  }

  if (!Result)
  {
    AErrorPath = ErrorPath;
  }

  return Result;
}

bool RecursiveDeleteFile(const UnicodeString & AFileName, bool ToRecycleBin)
{
  UnicodeString ErrorPath; // unused
  int32_t Deleted;
  bool Result = DoRecursiveDeleteFile(AFileName, ToRecycleBin, ErrorPath, Deleted);
  return Result;
}

int32_t RecursiveDeleteFileChecked(const UnicodeString & AFileName, bool ToRecycleBin)
{
  UnicodeString ErrorPath;
  int32_t Deleted;
  if (!DoRecursiveDeleteFile(AFileName, ToRecycleBin, ErrorPath, Deleted))
  {
    throw EOSExtException(FMTLOAD(CORE_DELETE_LOCAL_FILE_ERROR, ErrorPath));
  }
  return Deleted;
}

void DeleteFileChecked(const UnicodeString & AFileName)
{
  if (!::SysUtulsRemoveFile(ApiPath(AFileName)))
  {
    throw EOSExtException(FMTLOAD(CORE_DELETE_LOCAL_FILE_ERROR, AFileName));
  }
}

uint32_t CancelAnswer(uint32_t Answers)
{
  uint32_t Result;
  if ((Answers & qaCancel) != 0)
  {
    Result = qaCancel;
  }
  else if ((Answers & qaNo) != 0)
  {
    Result = qaNo;
  }
  else if ((Answers & qaAbort) != 0)
  {
    Result = qaAbort;
  }
  else if ((Answers & qaOK) != 0)
  {
    Result = qaOK;
  }
  else
  {
    DebugFail();
    Result = qaCancel;
  }
  return Result;
}

uint32_t AbortAnswer(uint32_t Answers)
{
  uint32_t Result;
  if (FLAGSET(Answers, qaAbort))
  {
    Result = qaAbort;
  }
  else
  {
    Result = CancelAnswer(Answers);
  }
  return Result;
}

uint32_t ContinueAnswer(uint32_t Answers)
{
  uint32_t Result;
  if (FLAGSET(Answers, qaSkip))
  {
    Result = qaSkip;
  }
  else if (FLAGSET(Answers, qaIgnore))
  {
    Result = qaIgnore;
  }
  else if (FLAGSET(Answers, qaYes))
  {
    Result = qaYes;
  }
  else if (FLAGSET(Answers, qaOK))
  {
    Result = qaOK;
  }
  else if (FLAGSET(Answers, qaRetry))
  {
    Result = qaRetry;
  }
  else
  {
    Result = CancelAnswer(Answers);
  }
  return Result;
}

#if 0
TLibModule * FindModule(void * Instance)
{
  TLibModule * CurModule;
  CurModule = reinterpret_cast<TLibModule*>(LibModuleList);

  while (CurModule)
  {
    if (CurModule->Instance == (unsigned)Instance)
    {
      break;
    }
    else
    {
      CurModule = CurModule->Next;
    }
  }
  return CurModule;
}
#endif // if 0

static UnicodeString DoLoadStrFrom(HINSTANCE Module, int32_t Ident, uint32_t MaxLength)
{
  UnicodeString Result;
  Result.SetLength(static_cast<int32_t>(MaxLength));
  const int Length = ::LoadStringW(Module, static_cast<UINT>(Ident), const_cast<LPWSTR>(Result.c_str()), nb::ToInt(MaxLength));
  Result.SetLength(Length);

  return Result;
}

UnicodeString LoadStrFrom(HINSTANCE Module, int32_t Ident)
{
  // 1024 = what VCL LoadStr limits the string to
  return DoLoadStrFrom(Module, Ident, 1024);
}

UnicodeString LoadStr(int32_t Ident, uint32_t /*MaxLength*/)
{
  UnicodeString Result = GetGlobals()->GetMsg(Ident);
  return Result;
#if 0
  TLibModule * MainModule = FindModule(HInstance);
  DebugAssert(MainModule != nullptr);
  return DoLoadStrFrom((HINSTANCE)MainModule->ResInstance, Ident, MaxLength);
#endif // if 0
}

UnicodeString LoadStrPart(int32_t Ident, int32_t Part)
{
  UnicodeString Result;
  UnicodeString Str = LoadStr(Ident);

  while (Part > 0)
  {
    Result = CutToChar(Str, L'|', false);
    Part--;
  }
  return Result;
}

UnicodeString DecodeUrlChars(const UnicodeString & Url)
{
  UnicodeString S = Url;

  int32_t i = 1;
  while (i <= S.Length())
  {
    switch (S[i])
    {
      case L'+':
        S[i] = L' ';
        break;

      case L'%':
        {
          UnicodeString Hex;
          while ((i + 2 <= S.Length()) && (S[i] == L'%') &&
                 IsHex(S[i + 1]) && IsHex(S[i + 2]))
          {
            Hex += S.SubString(i + 1, 2);
            S.Delete(i, 3);
          }

          if (!Hex.IsEmpty())
          {
            RawByteString Bytes = HexToBytes(Hex);
            UnicodeString Chars(UTF8ToString(Bytes));
            S.Insert(Chars, i);
            i += Chars.Length() - 1;
          }
        }
        break;
    }
    ++i;
  }
  return S;
}

UnicodeString DoEncodeUrl(const UnicodeString & S, const UnicodeString & DoNotEncode)
{
  UnicodeString Result = S;

  int32_t Index = 1;
  while (Index <= Result.Length())
  {
    const wchar_t C = Result[Index];
    if (IsLetter(C) ||
        IsDigit(C) ||
        (C == L'_') || (C == L'-') || (C == L'.') || (C == L'*') ||
        (DoNotEncode.Pos(C) > 0))
    {
      ++Index;
    }
    else
    {
      UTF8String UtfS(Result.SubString(Index, 1));
      UnicodeString H;
      for (int32_t Index2 = 1; Index2 <= UtfS.Length(); ++Index2)
      {
        H += L"%" + ByteToHex(static_cast<uint8_t>(UtfS[Index2]));
      }
      Result.Delete(Index, 1);
      Result.Insert(H, Index);
      Index += H.Length();
    }
  }
  return Result;
}

UnicodeString EncodeUrlString(const UnicodeString & S)
{
  return DoEncodeUrl(S, UnicodeString());
}

UnicodeString EncodeUrlPath(const UnicodeString & S)
{
  return DoEncodeUrl(S, L"/");
}

UnicodeString AppendUrlParams(const UnicodeString & AURL, const UnicodeString & Params)
{
  // see also TWebHelpSystem::ShowHelp
  const wchar_t FragmentSeparator = L'#';
  UnicodeString URL = AURL;
  URL = ::CutToChar(URL, FragmentSeparator, false);

  if (URL.Pos("?") == 0)
  {
    URL += L"?";
  }
  else
  {
    URL += L"&";
  }

  URL += Params;

  AddToList(URL, AURL, FragmentSeparator);

  return URL;
}

UnicodeString ExtractFileNameFromUrl(const UnicodeString & Url)
{
  UnicodeString Result = Url;
  int32_t P = Result.Pos("?");
  if (P > 0)
  {
    Result.SetLength(P - 1);
  }
  P = Result.LastDelimiter("/");
  if (DebugAlwaysTrue(P > 0))
  {
    Result.Delete(1, P);
  }
  return Result;
}

UnicodeString EscapeHotkey(const UnicodeString & Caption)
{
  return ReplaceStr(Caption, L"&", L"&&");
}

// duplicated in console's Main.cpp
static bool DoCutToken(UnicodeString & AStr, UnicodeString & AToken,
  UnicodeString * ARawToken, UnicodeString * ASeparator, bool EscapeQuotesInQuotesOnly)
{
  bool Result;

  AToken.Clear();

  // inspired by Putty's sftp_getcmd() from PSFTP.C
  int32_t Index = 1;
  while ((Index <= AStr.Length()) &&
    ((AStr[Index] == L' ') || (AStr[Index] == L'\t')))
  {
    ++Index;
  }

  if (Index <= AStr.Length())
  {
    bool Quoting = false;

    while (Index <= AStr.Length())
    {
      if (!Quoting && ((AStr[Index] == L' ') || (AStr[Index] == L'\t')))
      {
        break;
      }
      // With EscapeQuotesInQuotesOnly we escape quotes only within quotes
      // otherwise the "" means " (quote), but it should mean empty string.
      else if ((AStr[Index] == L'"') && (Index + 1 <= AStr.Length()) &&
        (AStr[Index + 1] == L'"') && (!EscapeQuotesInQuotesOnly || Quoting))
      {
        Index += 2;
        AToken += L'"';
      }
      else if (AStr[Index] == L'"')
      {
        ++Index;
        Quoting = !Quoting;
      }
      else
      {
        AToken += AStr[Index];
        ++Index;
      }
    }

    if (ARawToken != nullptr)
    {
      (*ARawToken) = AStr.SubString(1, Index - 1);
    }

    if (Index <= AStr.Length())
    {
      if (ASeparator != nullptr)
      {
        *ASeparator = AStr.SubString(Index, 1);
      }
      ++Index;
    }
    else
    {
      if (ASeparator != nullptr)
      {
        *ASeparator = UnicodeString();
      }
    }

    AStr = AStr.SubString(Index, AStr.Length());

    Result = true;
  }
  else
  {
    Result = false;
    AStr.Clear();
  }

  return Result;
}

bool CutToken(UnicodeString & AStr, UnicodeString & AToken,
  UnicodeString * ARawToken, UnicodeString * ASeparator)
{
  return DoCutToken(AStr, AToken, ARawToken, ASeparator, false);
}

bool CutTokenEx(UnicodeString & Str, UnicodeString & Token,
  UnicodeString * RawToken, UnicodeString * Separator)
{
  return DoCutToken(Str, Token, RawToken, Separator, true);
}

void AddToList(UnicodeString & List, const UnicodeString & Value, const UnicodeString & Delimiter)
{
  if (!Value.IsEmpty())
  {
    if (!List.IsEmpty() &&
        ((List.Length() < Delimiter.Length()) ||
         (List.SubString(List.Length() - Delimiter.Length() + 1, Delimiter.Length()) != Delimiter)))
    {
      List += Delimiter;
    }
    List += Value;
  }
}

void AddToShellFileListCommandLine(UnicodeString & List, const UnicodeString & Value)
{
  UnicodeString Arg = ShellQuoteStr(Value);
  AddToList(List, Arg, L" ");
}

bool IsWinVista()
{
  // Vista is 6.0
  // Win XP is 5.1
  // There also 5.2, what is Windows 2003 or Windows XP 64bit
  // (we consider it WinXP for now)
  return CheckWin32Version(6, 0);
}

bool IsWin7()
{
  return CheckWin32Version(6, 1);
}

bool IsWin8()
{
  return CheckWin32Version(6, 2);
}

bool IsWin10()
{
  return CheckWin32Version(10, 0);
}

static OSVERSIONINFO GetWindowsVersion()
{
  OSVERSIONINFO Result;
  memset(&Result, 0, sizeof(Result));
  Result.dwOSVersionInfoSize = sizeof(Result);
  // Cannot use the VCL Win32MajorVersion+Win32MinorVersion+Win32BuildNumber as
  // on Windows 10 due to some hacking in InitPlatformId, the Win32BuildNumber is lost
  GetVersionEx(&Result);
  return Result;
}

bool IsWin10Build(unsigned int BuildNumber)
{
  // It might be enough to check the dwBuildNumber, as we do in TWinConfiguration::IsDDExtBroken()
  OSVERSIONINFO OSVersionInfo = GetWindowsVersion();
  return
    (OSVersionInfo.dwMajorVersion > 10) ||
    ((OSVersionInfo.dwMajorVersion == 10) && (OSVersionInfo.dwMinorVersion > 0)) ||
    ((OSVersionInfo.dwMajorVersion == 10) && (OSVersionInfo.dwMinorVersion == 0) &&
     (OSVersionInfo.dwBuildNumber >= BuildNumber));
}

bool IsWin11()
{
  return IsWin10Build(22000);
}

bool IsWine()
{
  HMODULE NtDll = ::GetModuleHandle(L"ntdll.dll");
  return
    DebugAlwaysTrue(NtDll != nullptr) &&
    (::GetProcAddress(NtDll, "wine_get_version") != nullptr);
}

int GIsUWP = -1;
UnicodeString GPackageName;

static void NeedUWPData()
{
  if (GIsUWP < 0)
  {
    GIsUWP = 0;

    // HINSTANCE Kernel32 = GetModuleHandle(kernel32);
    HINSTANCE const Kernel32 = ::GetModuleHandle(L"kernel32.dll");
    typedef LONG (WINAPI * GetCurrentPackageFamilyNameProc)(UINT32 * /*packageFamilyNameLength*/, PWSTR /*packageFamilyName*/);
    GetCurrentPackageFamilyNameProc GetCurrentPackageFamilyName =
      (GetCurrentPackageFamilyNameProc)::GetProcAddress(Kernel32, "GetCurrentPackageFamilyName");
    UINT32 NameLen = 0;
    if ((GetCurrentPackageFamilyName != nullptr) &&
        (GetCurrentPackageFamilyName(&NameLen, nullptr) == ERROR_INSUFFICIENT_BUFFER))
    {
      GIsUWP = 1;
      AppLog(L"Is UWP application");
      GPackageName.SetLength(NameLen);
      if (GetCurrentPackageFamilyName(&NameLen, ToWChar(GPackageName)) == ERROR_SUCCESS)
      {
        PackStr(GPackageName);
      }
      else
      {
        GPackageName = L"err";
      }
      AppLogFmt(L"Package name: %s", GPackageName);
    }
    else
    {
      AppLog(L"Is not UWP application");
    }
    ::FreeLibrary(Kernel32);
  }
}

bool IsUWP()
{
  NeedUWPData();
  return (GIsUWP > 0);
}

UnicodeString GetPackageName()
{
  NeedUWPData();
  return GPackageName;
}

bool IsOfficialPackage()
{
  return (GetPackageName() == OfficialPackage);
}

LCID GetDefaultLCID()
{
  return GetUserDefaultLCID();
}

static UnicodeString ADefaultEncodingName;
UnicodeString DefaultEncodingName()
{
  if (ADefaultEncodingName.IsEmpty())
  {
    CPINFOEX Info;
    GetCPInfoEx(CP_ACP, 0, &Info);
    ADefaultEncodingName = Info.CodePageName;
  }
  return ADefaultEncodingName;
}

bool GetWindowsProductType(DWORD &Type)
{
  bool Result;
  HINSTANCE Kernel32 = ::GetModuleHandle(L"kernel32.dll");
  typedef BOOL (WINAPI * TGetProductInfo)(DWORD, DWORD, DWORD, DWORD, PDWORD);
  TGetProductInfo GetProductInfo =
      reinterpret_cast<TGetProductInfo>(::GetProcAddress(Kernel32, "GetProductInfo"));
  if (GetProductInfo == nullptr)
  {
    Result = false;
  }
  else
  {
    GetProductInfo(GetGlobals()->Win32MajorVersion, GetGlobals()->Win32MinorVersion, 0, 0, &Type);
    Result = true;
  }
  return Result;
}

UnicodeString WindowsProductName()
{
  UnicodeString Result;
  std::unique_ptr<TRegistry> Registry(std::make_unique<TRegistry>());
  Registry->SetAccess(KEY_READ);
  try
  {
    Registry->SetRootKey(HKEY_LOCAL_MACHINE);
    if (Registry->OpenKey("SOFTWARE", false) &&
        Registry->OpenKey("Microsoft", false) &&
        Registry->OpenKey("Windows NT", false) &&
        Registry->OpenKey("CurrentVersion", false))
    {
      Result = Registry->ReadString("ProductName");
    }
  }
  catch(...)
  {
    DEBUG_PRINTF("WindowsProductName: error");
  }
  return Result;
}

int GetWindowsBuild()
{
  return static_cast<int>(GetWindowsVersion().dwBuildNumber);
}

UnicodeString WindowsVersion()
{
  OSVERSIONINFO OSVersionInfo = GetWindowsVersion();
  UnicodeString Result = FORMAT("%d.%d.%d", int(OSVersionInfo.dwMajorVersion), int(OSVersionInfo.dwMinorVersion), int(OSVersionInfo.dwBuildNumber));
  return Result;
}

UnicodeString WindowsVersionLong()
{
  UnicodeString Result = WindowsVersion();
  AddToList(Result, GetGlobals()->Win32CSDVersion, L" ");
  return Result;
}

bool IsDirectoryWriteable(const UnicodeString & APath)
{
  UnicodeString FileName =
    ::IncludeTrailingPathDelimiter(APath) +
    FORMAT("wscp_%s_%d.tmp", FormatDateTime(L"nnzzz", Now()), int(GetCurrentProcessId()));
  HANDLE LocalFileHandle = ::CreateFile(ApiPath(FileName).c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
    CREATE_NEW, FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
  const bool Result = (LocalFileHandle != INVALID_HANDLE_VALUE);
  if (Result)
  {
    SAFE_CLOSE_HANDLE(LocalFileHandle);
  }
  return Result;
}

UnicodeString FormatNumber(int64_t Number)
{
//  return FormatFloat(L"#,##0", Number);
  return FORMAT("%.0f", nb::ToDouble(Number));
}

// simple alternative to FormatBytes
UnicodeString FormatSize(int64_t ASize)
{
  return FormatNumber(ASize);
}

UnicodeString FormatDateTimeSpan(const TDateTime & DateTime)
{
  TFormatSettings FormatSettings = TFormatSettings::Create(GetDefaultLCID());
  UnicodeString Result;
  if ((0 <= DateTime) && (DateTime <= MaxDateTime))
  {
    TTimeStamp TimeStamp = DateTimeToTimeStamp(DateTime);
    int Days = TimeStamp.Date - DateDelta;
    if (abs(Days) >= 4)
    {
      Result = FMTLOAD(DAYS_SPAN, (Days));
    }
    else
    {
      unsigned short Hour, Min, Sec, Dummy;
      DecodeTime(DateTime, Hour, Min, Sec, Dummy);
      int TotalHours = static_cast<int>(Hour) + (Days * HoursPerDay);
      Result = FORMAT(L"%d%s%.2d%s%.2d", TotalHours, FormatSettings.TimeSeparator, Min, FormatSettings.TimeSeparator, Sec);
    }
  }
  return Result;
}

UnicodeString FormatRelativeTime(const TDateTime & ANow, const TDateTime & AThen, bool DateOnly)
{
  UnicodeString Result;
  if (DateOnly)
  {
    if (IsSameDay(AThen, TDateTime(ANow - 1.0)))
    {
      Result = LoadStrPart(TIME_RELATIVE, 3);
    }
    else if (IsSameDay(AThen, ANow))
    {
      Result = LoadStrPart(TIME_RELATIVE, 2);
    }
  }

  if (Result.IsEmpty())
  {
    int32_t Part, Num;

    Num = YearsBetween(ANow, AThen);
    if (Num > 1)
    {
      Part = 18;
    }
    else if (Num == 1)
    {
      Part = 17;
    }
    else
    {
      Num = MonthsBetween(ANow, AThen);
      if (Num > 1)
      {
        Part = 16;
      }
      else if (Num == 1)
      {
        Part = 15;
      }
      else
      {
        Num = DaysBetween(ANow, AThen);
        if (Num > 1)
        {
          Part = 12;
        }
        else if (Num == 1)
        {
          Part = 11;
        }
        else
        {
          Num = static_cast<int32_t>(HoursBetween(ANow, AThen));
          if (Num > 1)
          {
            Part = 10;
          }
          else if (Num == 1)
          {
            Part = 9;
          }
          else
          {
            Num = static_cast<int32_t>(MinutesBetween(ANow, AThen));
            if (Num > 1)
            {
              Part = 8;
            }
            else if (Num == 1)
            {
              Part = 7;
            }
            else
            {
              Num = static_cast<int>(SecondsBetween(ANow, AThen));
              if (Num > 1)
              {
                Part = 6;
              }
              else if (Num == 1)
              {
                Part = 5;
              }
              else if (Num == 0)
              {
                Part = 1;
              }
              else
              {
                DebugFail();
                Part = -1;
              }
            }
          }
        }
      }
    }

    if (DebugAlwaysTrue(Part >= 0))
    {
      Result = FORMAT(LoadStrPart(TIME_RELATIVE, Part), abs(Num));
    }
    else
    {
      Result = FormatDateTime(L"ddddd", AThen);
    }
  }
  return Result;
}

UnicodeString ExtractFileBaseName(const UnicodeString & APath)
{
  return ChangeFileExt(base::ExtractFileName(APath, false), L"");
}

TStringList *TextToStringList(const UnicodeString & Text)
{
  std::unique_ptr<TStringList> List(std::make_unique<TStringList>());
  List->SetText(Text);
  return List.release();
}

UnicodeString StringsToText(TStrings *Strings)
{
  UnicodeString Result;
  if (Strings->GetCount() == 1)
  {
    Result = Strings->GetString(0);
  }
  else
  {
    Result = Strings->GetText();
  }
  return Result;
}

TStringList * CommaTextToStringList(const UnicodeString & CommaText)
{
  std::unique_ptr<TStringList> List(std::make_unique<TStringList>());
  List->SetCommaText(CommaText);
  return List.release();
}

TStrings * CloneStrings(TStrings * Strings)
{
  std::unique_ptr<TStringList> List(std::make_unique<TStringList>());
  List->AddStrings(Strings);
  return List.release();
}

UnicodeString TrimVersion(const UnicodeString & AVersion)
{
  UnicodeString Version = AVersion;

  while ((Version.Pos(".") != Version.LastDelimiter(L".")) &&
    (Version.SubString(Version.Length() - 1, 2) == L".0"))
  {
    Version.SetLength(Version.Length() - 2);
  }
  return Version;
}

UnicodeString FormatVersion(int32_t MajorVersion, int32_t MinorVersion, int32_t Release)
{
  return
    FORMAT("%d.%d.%d",
      nb::ToInt(MajorVersion), nb::ToInt(MinorVersion), nb::ToInt(Release));
}

TFormatSettings GetEngFormatSettings()
{
  return TFormatSettings::Create(1033);
}

template<size_t N>
int32_t IndexStr(const UnicodeString & AText, const UnicodeString (&AValues)[N])
{
  for(size_t i = 0; i < N; i++)
  {
   if (AValues[i] == AText)
       return i;
  }
  return -1;
}

int32_t ParseShortEngMonthName(const UnicodeString & MonthStr)
{
  TFormatSettings FormatSettings = GetEngFormatSettings();
  return IndexStr<_countof(FormatSettings.ShortMonthNames)>(MonthStr, FormatSettings.ShortMonthNames) + 1;
}

TStringList * CreateSortedStringList(bool CaseSensitive, TDuplicatesEnum Duplicates)
{
  TStringList * Result = new TStringList();
  Result->SetCaseSensitive(CaseSensitive);
  Result->SetSorted(true);
  Result->SetDuplicates(Duplicates);
  return Result;
}

static UnicodeString NormalizeIdent(const UnicodeString & AIdent)
{
  UnicodeString Ident = AIdent;
  int32_t Index = 1;
  while (Index <= Ident.Length())
  {
    if (Ident[Index] == L'-')
    {
      Ident.Delete(Index, 1);
    }
    else
    {
      Index++;
    }
  }
  return Ident;
}

bool SameIdent(const UnicodeString & Ident1, const UnicodeString & Ident2)
{
  const UnicodeString Dash(L"-");
  return SameText(ReplaceStr(Ident1, Dash, EmptyStr), ReplaceStr(Ident2, Dash, EmptyStr));
}

UnicodeString FindIdent(const UnicodeString & Ident, TStrings * Idents)
{
  UnicodeString NormalizedIdent(NormalizeIdent(Ident));
  for (int32_t Index = 0; Index < Idents->GetCount(); Index++)
  {
    if (SameText(NormalizedIdent, NormalizeIdent(Idents->GetString(Index))))
    {
      return Idents->GetString(Index);
    }
  }
  return Ident;
}

static UnicodeString GetTlsErrorStr(int Err)
{
  char Buffer[512];
  ERR_error_string(Err, Buffer);
  // not sure about the UTF8
  return UnicodeString(UTF8String(Buffer));
}

static FILE * OpenCertificate(const UnicodeString & Path)
{
  FILE * Result = _wfopen(ApiPath(Path).c_str(), L"rb");
  if (Result == nullptr)
  {
    int Error = errno;
    throw EOSExtException(MainInstructions(FMTLOAD(CERTIFICATE_OPEN_ERROR, Path)), Error);
  }

  return Result;
}

struct TPemPasswordCallbackData
{
  UnicodeString *Passphrase;
};

static int PemPasswordCallback(char * Buf, int ASize, int /*RWFlag*/, void *UserData)
{
  TPemPasswordCallbackData &Data = *reinterpret_cast<TPemPasswordCallbackData *>(UserData);
  UTF8String UtfPassphrase = UTF8String(*Data.Passphrase);
  strncpy(Buf, UtfPassphrase.c_str(), static_cast<size_t>(ASize));
  Shred(UtfPassphrase);
  Buf[ASize - 1] = '\0';
  return nb::ToInt(NBChTraitsCRT<char>::SafeStringLen(Buf));
}

static bool IsTlsPassphraseError(int Error, bool HasPassphrase)
{
  int ErrorLib = ERR_GET_LIB(Error);
  int ErrorReason = ERR_GET_REASON(Error);

  bool Result =
    ((ErrorLib == ERR_LIB_PKCS12) &&
     (ErrorReason == PKCS12_R_MAC_VERIFY_FAILURE)) ||
    ((ErrorLib == ERR_LIB_PEM) &&
     (ErrorReason == PEM_R_BAD_PASSWORD_READ)) ||
    (HasPassphrase && (ERR_LIB_EVP == ERR_LIB_EVP) &&
     ((ErrorReason == PEM_R_BAD_DECRYPT) || (ErrorReason == PEM_R_BAD_BASE64_DECODE)));

  return Result;
}

static void ThrowTlsCertificateErrorIgnorePassphraseErrors(const UnicodeString & Path, bool HasPassphrase)
{
  unsigned long Error = ERR_get_error();
  if (!IsTlsPassphraseError(Error, HasPassphrase))
  {
    throw ExtException(MainInstructions(FMTLOAD(CERTIFICATE_READ_ERROR, Path)), GetTlsErrorStr(Error));
  }
}

void ParseCertificate(const UnicodeString & Path,
  const UnicodeString & Passphrase, X509 *& Certificate, EVP_PKEY *& PrivateKey,
  bool & WrongPassphrase)
{
  Certificate = nullptr;
  PrivateKey = nullptr;
  WrongPassphrase = false;
  bool HasPassphrase = !Passphrase.IsEmpty();

  FILE * File;

  // Inspired by neon's ne_ssl_clicert_read
  File = OpenCertificate(Path);
  // openssl pkcs12 -inkey cert.pem -in cert.crt -export -out cert.pfx
  // Binary file
  PKCS12 *Pkcs12 = d2i_PKCS12_fp(File, nullptr);
  fclose(File);

  if (Pkcs12 != nullptr)
  {
    UTF8String PassphraseUtf(Passphrase);

    bool Result =
      (PKCS12_parse(Pkcs12, PassphraseUtf.c_str(), &PrivateKey, &Certificate, nullptr) == 1);
    PKCS12_free(Pkcs12);

    if (!Result)
    {
      ThrowTlsCertificateErrorIgnorePassphraseErrors(Path, HasPassphrase);
      WrongPassphrase = true;
    }
  }
  else
  {
    ERR_clear_error();

    TPemPasswordCallbackData CallbackUserData;
    // PemPasswordCallback never writes to the .Passphrase
    CallbackUserData.Passphrase = const_cast<UnicodeString *>(&Passphrase);

    File = OpenCertificate(Path);
    // Encrypted:
    // openssl req -x509 -newkey rsa:2048 -keyout cert.pem -out cert.crt
    // -----BEGIN ENCRYPTED PRIVATE KEY-----
    // ...
    // -----END ENCRYPTED PRIVATE KEY-----

    // Not encrypted (add -nodes):
    // -----BEGIN PRIVATE KEY-----
    // ...
    // -----END PRIVATE KEY-----
    // Or (openssl genrsa -out client.key 1024   # used for certificate signing request)
    // -----BEGIN RSA PRIVATE KEY-----
    // ...
    // -----END RSA PRIVATE KEY-----
    PrivateKey = PEM_read_PrivateKey(File, nullptr, PemPasswordCallback, &CallbackUserData);
    fclose(File);

    try__finally
    {
      if (PrivateKey == nullptr)
      {
        ThrowTlsCertificateErrorIgnorePassphraseErrors(Path, HasPassphrase);
        WrongPassphrase = true;
      }

      File = OpenCertificate(Path);
      // The file can contain both private and public key
      // (basically cert.pem and cert.crt appended one to each other)
      // -----BEGIN ENCRYPTED PRIVATE KEY-----
      // ...
      // -----END ENCRYPTED PRIVATE KEY-----
      // -----BEGIN CERTIFICATE-----
      // ...
      // -----END CERTIFICATE-----
      Certificate = PEM_read_X509(File, nullptr, PemPasswordCallback, &CallbackUserData);
      fclose(File);

      if (Certificate == nullptr)
      {
        uint32_t Error = ERR_get_error();
        // unlikely
        if (IsTlsPassphraseError(Error, HasPassphrase))
        {
          WrongPassphrase = true;
        }
        else
        {
          UnicodeString CertificatePath = ChangeFileExt(Path, L".cer");
          if (!base::FileExists(CertificatePath))
          {
            CertificatePath = ChangeFileExt(Path, L".crt");
          }

          if (!base::FileExists(CertificatePath))
          {
            throw Exception(MainInstructions(FMTLOAD(CERTIFICATE_PUBLIC_KEY_NOT_FOUND, Path)));
          }
          else
          {
            File = OpenCertificate(CertificatePath);
            // -----BEGIN CERTIFICATE-----
            // ...
            // -----END CERTIFICATE-----
            Certificate = PEM_read_X509(File, nullptr, PemPasswordCallback, &CallbackUserData);
            fclose(File);

            if (Certificate == nullptr)
            {
              uint32_t Base64Error = ERR_get_error();

              File = OpenCertificate(CertificatePath);
              // Binary DER-encoded certificate
              // (as above, with BEGIN/END removed, and decoded from Base64 to binary)
              // openssl x509 -in cert.crt -out client.der.crt -outform DER
              Certificate = d2i_X509_fp(File, nullptr);
              fclose(File);

              if (Certificate == nullptr)
              {
                uint32_t DERError = ERR_get_error();

                UnicodeString Message = MainInstructions(FMTLOAD(CERTIFICATE_READ_ERROR, CertificatePath));
                UnicodeString MoreMessages =
                  FORMAT("Base64: %s\nDER: %s", GetTlsErrorStr(Base64Error), GetTlsErrorStr(DERError));
                throw ExtException(Message, MoreMessages);
              }
            }
          }
        }
      }
    },
    __finally
    {
      // We loaded private key, but failed to load certificate, discard the certificate
      // (either exception was thrown or WrongPassphrase)
      if ((PrivateKey != nullptr) && (Certificate == nullptr))
      {
        EVP_PKEY_free(PrivateKey);
        PrivateKey = nullptr;
      }
      // Certificate was verified, but passphrase was wrong when loading private key,
      // so discard the certificate
      else if ((Certificate != nullptr) && (PrivateKey == nullptr))
      {
        X509_free(Certificate);
        Certificate = nullptr;
      }
    } end_try__finally
  }
}

void CheckCertificate(const UnicodeString & Path)
{
  X509 * Certificate;
  EVP_PKEY * PrivateKey;
  bool WrongPassphrase;

  ParseCertificate(Path, L"", Certificate, PrivateKey, WrongPassphrase);

  if (PrivateKey != nullptr)
  {
    EVP_PKEY_free(PrivateKey);
  }
  if (Certificate != nullptr)
  {
    X509_free(Certificate);
  }
}

const UnicodeString HttpProtocol(L"http");
const UnicodeString HttpsProtocol(L"https");
const UnicodeString ProtocolSeparator(L"://");

bool IsHttpUrl(const UnicodeString & S)
{
  return StartsText(HttpProtocol + ProtocolSeparator, S);
}

bool IsHttpOrHttpsUrl(const UnicodeString & S)
{
  return
    IsHttpUrl(S) ||
    StartsText(HttpsProtocol + ProtocolSeparator, S);
}

UnicodeString ChangeUrlProtocol(const UnicodeString & S, const UnicodeString & Protocol)
{
  const int32_t P = S.Pos(ProtocolSeparator);
  DebugAssert(P > 0);
  return Protocol + ProtocolSeparator + RightStr(S, S.Length() - P - ProtocolSeparator.Length() + 1);
}

#if 0

const UnicodeString RtfPara(TraceInitStr(L"\\par\n"));
const UnicodeString AssemblyNamespace(TraceInitStr(L"WinSCP"));
const UnicodeString TransferOptionsClassName(TraceInitStr(L"TransferOptions"));
const UnicodeString SessionClassName(TraceInitStr(L"Session"));
const UnicodeString RtfHyperlinkField(TraceInitStr(L"HYPERLINK"));
const UnicodeString RtfHyperlinkFieldPrefix(TraceInitStr(RtfHyperlinkField + L" \""));
const UnicodeString RtfHyperlinkFieldSuffix(TraceInitStr(L"\" "));

UnicodeString RtfColor(int Index)
{
  return FORMAT(L"\\cf%d", (Index));
}

UnicodeString RtfText(const UnicodeString & Text, bool Rtf)
{
  UnicodeString Result = Text;
  if (Rtf)
  {
    int Index = 1;
    while (Index <= Result.Length())
    {
      UnicodeString Replacement;
      wchar_t Ch = Result[Index];
      if ((Ch == L'\\') || (Ch == L'{') || (Ch == L'}'))
      {
        Replacement = FORMAT(L"\\%s", Ch);
      }
      else if (Ch >= 0x0080)
      {
        Replacement = FORMAT(L"\\u%d?", int(Ch));
      }

      if (!Replacement.IsEmpty())
      {
        Result.Delete(Index, 1);
        Result.Insert(Replacement, Index);
        Index += Replacement.Length();
      }
      else
      {
        Index++;
      }
    }
  }
  return Result;
}

UnicodeString RtfColorText(int Color, const UnicodeString Text)
{
  return RtfColor(Color) + L" " + RtfText(Text) + RtfColor(0) + L" ";
}

UnicodeString RtfColorItalicText(int Color, const UnicodeString Text)
{
  return RtfColor(Color) + L"\\i " + RtfText(Text) + L"\\i0" + RtfColor(0) + L" ";
}

UnicodeString RtfOverrideColorText(const UnicodeString & Text)
{
  return RtfColorText(1, Text);
}

UnicodeString RtfKeyword(const UnicodeString & Text)
{
  return RtfColorText(5, Text);
}

UnicodeString RtfParameter(const UnicodeString & Text)
{
  return RtfColorText(6, Text);
}

UnicodeString RtfString(const UnicodeString & Text)
{
  return RtfColorText(4, Text);
}

UnicodeString RtfLink(const UnicodeString & Link, const UnicodeString & RtfText)
{
  return
    L"{\\field{\\*\\fldinst{" + RtfHyperlinkFieldPrefix + Link + RtfHyperlinkFieldSuffix + L"}}{\\fldrslt{" +
    RtfText + L"}}}";
}

UnicodeString ScriptCommandLink(const UnicodeString & Command)
{
  return L"scriptcommand_" + Command;
}

UnicodeString RtfSwitch(
  const UnicodeString & Switch, const UnicodeString & Link, bool Rtf)
{
  UnicodeString Result = FORMAT(L"-%s", (Switch));
  if (Rtf)
  {
    Result = RtfLink(Link + L"#" + Switch.LowerCase(), RtfParameter(Result));
  }
  return L" " + Result;
}

UnicodeString RtfSwitchValue(
  const UnicodeString & Name, const UnicodeString & Link, const UnicodeString & Value, bool Rtf)
{
  return RtfSwitch(Name, Link, Rtf) + L"=" + Value;
}

UnicodeString RtfSwitch(
  const UnicodeString & Name, const UnicodeString & Link, const UnicodeString & Value, bool Rtf)
{
  return RtfSwitchValue(Name, Link, RtfText(FORMAT("\"%s\"", (EscapeParam(Value))), Rtf), Rtf);
}
UnicodeString RtfSwitch(
  const UnicodeString & Name, const UnicodeString & Link, int Value, bool Rtf)
{
  return RtfSwitchValue(Name, Link, RtfText(IntToStr(Value), Rtf), Rtf);
}

UnicodeString RtfRemoveHyperlinks(UnicodeString Text)
{
  // Remove all tags HYPERLINK "https://www.example.com".
  // See also RtfEscapeParam
  int Index = 1;
  int P;
  while ((P = PosEx(RtfHyperlinkFieldPrefix, Text, Index)) > 0)
  {
    int Index2 = P + RtfHyperlinkFieldPrefix.Length();
    int P2 = PosEx(RtfHyperlinkFieldSuffix, Text, Index2);
    if (P2 > 0)
    {
      Text.Delete(P, P2 - P + RtfHyperlinkFieldSuffix.Length());
    }
    else
    {
      Index = Index2;
    }
  }
  return Text;
}

UnicodeString RtfEscapeParam(UnicodeString Param, bool PowerShellEscape)
{
  const UnicodeString Quote(L"\"");
  UnicodeString Escape(Quote);
  if (PowerShellEscape)
  {
    Escape = "`" + Escape + "`";
  }
  // Equivalent of EscapeParam, except that it does not double quotes in HYPERLINK.
  // See also RtfRemoveHyperlinks.
  int Index = 1;
  while (true)
  {
    int P1 = PosEx(Quote, Param, Index);
    if (P1 == 0)
    {
      // no more quotes
      break;
    }
    else
    {
      int P2 = PosEx(RtfHyperlinkFieldPrefix, Param, Index);
      int P3;
      if ((P2 > 0) && (P2 < P1) && ((P3 = PosEx(RtfHyperlinkFieldSuffix, Param, P2)) > 0))
      {
        // skip HYPERLINK
        Index = P3 + RtfHyperlinkFieldSuffix.Length();
      }
      else
      {
        Param.Insert(Escape, P1);
        Index = P1 + (Escape.Length() + Quote.Length());
      }
    }
  }

  return Param;
}

static UnicodeString RtfCodeComment(const UnicodeString & Text)
{
  return RtfColorItalicText(2, Text);
}

UnicodeString AssemblyCommentLine(TAssemblyLanguage Language, const UnicodeString & Text)
{
  UnicodeString Prefix;
  switch (Language)
  {
    case alCSharp:
      Prefix = L"//";
      break;

    case alVBNET:
      Prefix = L"'";
      break;

    case alPowerShell:
      Prefix = L"#";
      break;
  }

  return RtfCodeComment(Prefix + L" " + Text) + RtfPara;
}

UnicodeString AssemblyString(TAssemblyLanguage Language, UnicodeString S)
{
  switch (Language)
  {
    case alCSharp:
      if (S.Pos(L"\\") > 0)
      {
        S = FORMAT(L"@\"%s\"", (ReplaceStr(S, L"\"", L"\"\"")));
      }
      else
      {
        S = FORMAT(L"\"%s\"", (ReplaceStr(S, L"\"", L"\\\"")));
      }
      break;

    case alVBNET:
      S = FORMAT(L"\"%s\"", (ReplaceStr(S, L"\"", L"\"\"")));
      break;

    case alPowerShell:
      S = FORMAT(L"\"%s\"", (ReplaceStr(ReplaceStr(ReplaceStr(S, L"`", L"``"), L"$", L"`$"), L"\"", L"`\"")));
      break;

    default:
      DebugFail();
      break;
  }

  return RtfString(S);
}

static UnicodeString RtfClass(const UnicodeString & Text)
{
  return RtfColorText(3, Text);
}

UnicodeString RtfLibraryClass(const UnicodeString & ClassName)
{
  return RtfLink(L"library_" + ClassName.LowerCase(), RtfClass(ClassName));
}

UnicodeString RtfLibraryMethod(const UnicodeString & ClassName, const UnicodeString & MethodName, bool InPage)
{
  return RtfLink(L"library_" + ClassName.LowerCase() + (InPage ? L"#" : L"_") + MethodName.LowerCase(), RtfOverrideColorText(MethodName));
}

static UnicodeString RtfLibraryProperty(const UnicodeString & ClassName, const UnicodeString & PropertyName)
{
  return RtfLink(L"library_" + ClassName.LowerCase() + L"#" + PropertyName.LowerCase(), RtfOverrideColorText(PropertyName));
}

UnicodeString AssemblyVariableName(TAssemblyLanguage Language, const UnicodeString & ClassName)
{
  UnicodeString Result = ClassName.SubString(1, 1).LowerCase() + ClassName.SubString(2, ClassName.Length() - 1);
  if (Language == alPowerShell)
  {
    Result = L"$" + Result;
  }
  return Result;
}

UnicodeString AssemblyStatementSeparator(TAssemblyLanguage Language)
{
  UnicodeString Result;
  switch (Language)
  {
    case alCSharp:
      Result = L";";
      break;

    case alVBNET:
    case alPowerShell:
      // noop
      break;
  }
  return Result;
}

UnicodeString AssemblyPropertyRaw(
  TAssemblyLanguage Language, const UnicodeString & ClassName, const UnicodeString & Name,
  const UnicodeString & Value, bool Inline)
{
  UnicodeString Result;
  UnicodeString RtfPropertyAndValue = RtfLibraryProperty(ClassName, Name) + L" = " + Value;
  UnicodeString Indetation = (Inline ? L"" : L"    ");
  UnicodeString SpaceOrPara = (Inline ? UnicodeString(L" ") : RtfPara);
  switch (Language)
  {
    case alCSharp:
      Result = Indetation + RtfPropertyAndValue + (Inline ? L"" : L",") + SpaceOrPara;
      break;

    case alVBNET:
      Result = Indetation + L"." + RtfPropertyAndValue + SpaceOrPara;
      break;

    case alPowerShell:
      Result = Indetation + RtfPropertyAndValue + SpaceOrPara;
      break;
  }
  return Result;
}

UnicodeString AssemblyProperty(
  TAssemblyLanguage Language, const UnicodeString & ClassName, const UnicodeString & Name,
  const UnicodeString & Type, const UnicodeString & Member, bool Inline)
{
  UnicodeString PropertyValue;

  switch (Language)
  {
    case alCSharp:
    case alVBNET:
      PropertyValue = RtfClass(Type) + RtfText(L"." + Member);
      break;

    case alPowerShell:
      PropertyValue = RtfText(L"[" + AssemblyNamespace + L".") + RtfClass(Type) + RtfText(L"]::" + Member);
      break;
  }

  return AssemblyPropertyRaw(Language, ClassName, Name, PropertyValue, Inline);
}

UnicodeString AssemblyProperty(
  TAssemblyLanguage Language, const UnicodeString & ClassName,
  const UnicodeString & Name, const UnicodeString & Value, bool Inline)
{
  return AssemblyPropertyRaw(Language, ClassName, Name, AssemblyString(Language, Value), Inline);
}

UnicodeString AssemblyProperty(
  TAssemblyLanguage Language, const UnicodeString & ClassName,
  const UnicodeString & Name, int Value, bool Inline)
{
  return AssemblyPropertyRaw(Language, ClassName, Name, IntToStr(Value), Inline);
}

UnicodeString AssemblyBoolean(TAssemblyLanguage Language, bool Value)
{
  UnicodeString Result;

  switch (Language)
  {
    case alCSharp:
      Result = (Value ? L"true" : L"false");
      break;

    case alVBNET:
      Result = (Value ? L"True" : L"False");
      break;

    case alPowerShell:
      Result = (Value ? L"$True" : L"$False");
      break;
  }

  return Result;
}

UnicodeString AssemblyProperty(
  TAssemblyLanguage Language, const UnicodeString & ClassName, const UnicodeString & Name, bool Value, bool Inline)
{
  UnicodeString PropertyValue = AssemblyBoolean(Language, Value);

  return AssemblyPropertyRaw(Language, ClassName, Name, PropertyValue, Inline);
}

UnicodeString AssemblyNewClassInstance(TAssemblyLanguage Language, const UnicodeString & ClassName, bool Inline)
{
  UnicodeString VariableName = AssemblyVariableName(Language, ClassName);
  UnicodeString RtfClass = RtfLibraryClass(ClassName);

  UnicodeString Result;
  switch (Language)
  {
    case alCSharp:
      if (!Inline)
      {
        Result += RtfClass + RtfText(L" " + VariableName  + L" = ");
      }
      Result += RtfKeyword(L"new") + RtfText(L" ") + RtfClass;
      break;

    case alVBNET:
      if (!Inline)
      {
        Result += RtfText(VariableName + L" ") + RtfKeyword(L"As") + RtfText(L" ");
      }
      Result += RtfKeyword(L"New") + RtfText(" ") + RtfClass;
      break;

    case alPowerShell:
      if (!Inline)
      {
        Result += RtfText(VariableName + L" = ");
      }
      Result += RtfKeyword(L"New-Object") + RtfText(L" " + AssemblyNamespace + L".") + RtfClass;
      break;
  }
  return Result;
}

UnicodeString AssemblyVariableDeclaration(TAssemblyLanguage Language)
{
  UnicodeString Result;
  switch (Language)
  {
    case alVBNET:
      Result = RtfKeyword(L"Dim") + RtfText(L" ");
      break;
  }
  return Result;
}

UnicodeString AssemblyNewClassInstanceStart(
  TAssemblyLanguage Language, const UnicodeString & ClassName, bool Inline)
{
  UnicodeString SpaceOrPara = (Inline ? UnicodeString(L" ") : RtfPara);

  UnicodeString Result;
  if (!Inline)
  {
    Result += AssemblyVariableDeclaration(Language);
  }
  Result += AssemblyNewClassInstance(Language, ClassName, Inline);

  switch (Language)
  {
    case alCSharp:
      Result += SpaceOrPara + RtfText(L"{") + SpaceOrPara;
      break;

    case alVBNET:
      // Historically we use Dim .. With instead of object initilizer.
      // But for inline use, we have to use object initialize.
      // We should consistently always use object initilizers.
      // Unfortunatelly VB.NET object initializer (contrary to C#) does not allow trailing comma.
      Result += SpaceOrPara + RtfKeyword(L"With");
      if (Inline)
      {
        Result += RtfText(L" { ");
      }
      else
      {
        Result += RtfText(L" " + AssemblyVariableName(Language, ClassName)) + RtfPara;
      }
      break;

    case alPowerShell:
      Result += RtfText(" -Property @{") + SpaceOrPara;
      break;
  }
  return Result;
}

UnicodeString AssemblyNewClassInstanceEnd(TAssemblyLanguage Language, bool Inline)
{
  UnicodeString InlineEnd = RtfText(L"}");

  UnicodeString Result;
  switch (Language)
  {
    case alCSharp:
      if (Inline)
      {
        Result = InlineEnd;
      }
      else
      {
        Result = RtfText(L"};") + RtfPara;
      }
      break;

    case alVBNET:
      if (Inline)
      {
        Result = InlineEnd;
      }
      else
      {
        Result = RtfKeyword(L"End With") + RtfPara;
      }
      break;

    case alPowerShell:
      if (Inline)
      {
        Result = InlineEnd;
      }
      else
      {
        Result = RtfText(L"}") + RtfPara;
      }
      break;
  }
  return Result;
}

UnicodeString AssemblyAddRawSettings(
  TAssemblyLanguage Language, TStrings * RawSettings, const UnicodeString & ClassName,
  const UnicodeString & MethodName)
{
  UnicodeString Result;
  for (int Index = 0; Index < RawSettings->Count; Index++)
  {
    UnicodeString Name = RawSettings->Names[Index];
    UnicodeString Value = RawSettings->ValueFromIndex[Index];
    UnicodeString AddRawSettingsMethod =
      RtfLibraryMethod(ClassName, MethodName, false) +
      FORMAT(L"(%s, %s)", (AssemblyString(Language, Name), AssemblyString(Language, Value)));
    UnicodeString VariableName = AssemblyVariableName(Language, ClassName);
    Result += RtfText(VariableName + L".") + AddRawSettingsMethod + AssemblyStatementSeparator(Language) + RtfPara;
  }
  return Result;
}

void LoadScriptFromFile(UnicodeString FileName, TStrings * Lines, bool FallbackToAnsi)
{
  std::unique_ptr<TFileStream> Stream(std::make_unique<TFileStream>(ApiPath(FileName), fmOpenRead | fmShareDenyWrite));

  // Simple stream reading, to make it work with named pipes too, not only with physical files
  TBytes Buffer;
  Buffer.Length = 10*1024;
  int Read;
  int Offset = 0;
  do
  {
    Read = Stream->Read(Buffer, Offset, Buffer.Length - Offset);
    Offset += Read;
    if (Offset > Buffer.Length / 2)
    {
      Buffer.Length = Buffer.Length * 2;
    }
  }
  while (Read > 0);
  Buffer.Length = Offset;

  TEncoding * Encoding = nullptr;
  int PreambleSize = TEncoding::GetBufferEncoding(Buffer, Encoding, TEncoding::UTF8);
  UnicodeString S;
  try
  {
    S = Encoding->GetString(Buffer, PreambleSize, Buffer.Length - PreambleSize);
  }
  catch (EEncodingError & E)
  {
    if (FallbackToAnsi)
    {
      S = TEncoding::ANSI->GetString(Buffer);
    }
    else
    {
      throw ExtException(LoadStr(TEXT_FILE_ENCODING), &E);
    }
  }

  Lines->Text = S;
}

#endif // #if 0

UnicodeString StripEllipsis(const UnicodeString & S)
{
  UnicodeString Result = S;
  if (Result.SubString(Result.Length() - Ellipsis.Length() + 1, Ellipsis.Length()) == Ellipsis)
  {
    Result.SetLength(Result.Length() - Ellipsis.Length());
    Result = Result.TrimRight();
  }
  return Result;
}

UnicodeString GetFileMimeType(const UnicodeString & FileName)
{
  wchar_t * MimeOut = nullptr;
  UnicodeString Result;
#if 0
  wchar_t * MimeOut = nullptr;
  if (::FindMimeFromData(nullptr, FileName.c_str(), nullptr, 0, nullptr, FMFD_URLASFILENAME, &MimeOut, 0) == S_OK)
  {
    Result = MimeOut;
    CoTaskMemFree(MimeOut);
  }
#endif // if 0
  return Result;
}

TStrings * TlsCipherList()
{
  std::unique_ptr<TStrings> Result(std::make_unique<TStringList>());
  const SSL_METHOD * Method = DTLS_client_method();
  SSL_CTX * Ctx = SSL_CTX_new(Method);
  SSL * Ssl = SSL_new(Ctx);

  int Index = 0;
  const char * CipherName;
  do
  {
    CipherName = SSL_get_cipher_list(Ssl, Index);
    Index++;
    if (CipherName != nullptr)
    {
      Result->Add(UnicodeString(CipherName));
    }
  }
  while (CipherName != nullptr);

  return Result.release();
}

void SetStringValueEvenIfEmpty(TStrings * Strings, const UnicodeString & Name, const UnicodeString & Value)
{
  if (Value.IsEmpty())
  {
    int Index = Strings->IndexOfName(Name);
    if (Index < 0)
    {
      Index = Strings->Add(L"");
    }
    UnicodeString Line = Name + Strings->GetNameValueSeparator();
    Strings->SetString(Index, Line);
  }
  else
  {
    Strings->SetValue(Name, Value);
  }
}

DWORD GetParentProcessId(HANDLE Snapshot, DWORD ProcessId)
{
  DWORD Result = 0;
  ThrowNotImplemented(3036);
#if 0
  PROCESSENTRY32 ProcessEntry;
  memset(&ProcessEntry, sizeof(ProcessEntry), 0);
  ProcessEntry.dwSize = sizeof(PROCESSENTRY32);

  if (Process32First(Snapshot, &ProcessEntry))
  {
    do
    {
      if (ProcessEntry.th32ProcessID == ProcessId)
      {
        Result = ProcessEntry.th32ParentProcessID;
      }
    } while (Process32Next(Snapshot, &ProcessEntry));
  }
#endif // #if 0
  return Result;
}

static UnicodeString GetProcessName(DWORD ProcessId)
{
  UnicodeString Result;
  ThrowNotImplemented(3037);
#if 0
  HANDLE Process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, ProcessId);
   // is common, when the parent process is installer, so we ignore it
  if (Process)
  {
    Result.SetLength(MAX_PATH);
    DWORD Len = GetModuleFileNameEx(Process, nullptr, Result.c_str(), Result.Length());
    Result.SetLength(Len);
    // is common too, for some reason
    if (!Result.IsEmpty())
    {
      Result = ExtractProgramName(FormatCommand(Result, UnicodeString()));
    }
    CloseHandle(Process);
  }
#endif // #if 0
  return Result;
}

UnicodeString ParentProcessName;

UnicodeString GetAncestorProcessName(int Levels)
{
  UnicodeString Result;
  ThrowNotImplemented(3038);
#if 0
  bool Parent = (Levels == 1);
  if (Parent && !ParentProcessName.IsEmpty())
  {
    Result = ParentProcessName;
  }
  else
  {
    try
    {
      HANDLE Snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

      DWORD ProcessId = GetCurrentProcessId();

      typedef std::vector<DWORD> TProcesses;
      TProcesses Processes;
      // Either more to go (>0) or collecting all levels (-1 from GetAncestorProcessNames)
      while ((Levels != 0) &&
             (Levels > -20) && // prevent infinite loops
             (ProcessId != 0))
      {
        const UnicodeString Sep(L", ");
        ProcessId = GetParentProcessId(Snapshot, ProcessId);
        // When ancestor process is terminated and another process reuses its ID, we may get a cycle.
        TProcesses::const_iterator I = std::find(Processes.begin(), Processes.end(), ProcessId);
        if (I != Processes.end())
        {
          int Index = I - Processes.begin();
          AddToList(Result, FORMAT(L"cycle-%d", (Index)), Sep);
          ProcessId = 0;
        }
        else
        {
          Processes.push_back(ProcessId);

          if ((Levels < 0) && (ProcessId != 0))
          {
            UnicodeString Name = GetProcessName(ProcessId);
            if (Name.IsEmpty())
            {
              Name = L"...";
              ProcessId = 0;
            }
            AddToList(Result, Name, Sep);
          }
          Levels--;
        }
      }

      if (Levels >= 0)
      {
        if (ProcessId == 0)
        {
          Result = L"err-notfound";
        }
        else
        {
          Result = GetProcessName(ProcessId);
        }
      }
      else if (Result.IsEmpty())
      {
        Result = L"n/a";
      }

      CloseHandle(Snapshot);
    }
    catch (...)
    {
      Result = L"err-except";
    }

    if (Parent)
    {
      ParentProcessName = Result;
    }
  }
#endif // #if 0
  return Result;
}

UnicodeString AncestorProcessNames;

UnicodeString GetAncestorProcessNames()
{
  if (AncestorProcessNames.IsEmpty())
  {
    AncestorProcessNames = GetAncestorProcessName(-1);
  }
  return AncestorProcessNames;
}

void NotImplemented()
{
  DebugFail();
  throw Exception(L"Not implemented");
}

UnicodeString GetDividerLine()
{
  return UnicodeString::StringOfChar(L'-', 27);
}
