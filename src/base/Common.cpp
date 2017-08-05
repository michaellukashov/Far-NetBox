
//#define NO_WIN32_LEAN_AND_MEAN
#include <vcl.h>
#pragma hdrstop

#include <Exceptions.h>
#include <TextsCore.h>
//#include <Interface.h>
#include <Common.h>
#include <Global.h>
#include <StrUtils.hpp>
#include <math.h>
#include <rdestl/map.h>
#include <rdestl/vector.h>
#if defined(_MSC_VER) && !defined(__clang__)
//#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#endif // if defined(_MSC_VER) && !defined(__clang__)
#if defined(HAVE_OPENSSL)
#include <openssl/pkcs12.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#endif // HAVE_OPENSSL


#pragma warning(disable: 4996) // https://msdn.microsoft.com/en-us/library/ttcz0bys.aspx The compiler encountered a deprecated declaration

const wchar_t * DSTModeNames = L"Win;Unix;Keep";


const wchar_t EngShortMonthNames[12][4] =
{
  L"Jan", L"Feb", L"Mar", L"Apr", L"May", L"Jun",
  L"Jul", L"Aug", L"Sep", L"Oct", L"Nov", L"Dec"
};
const char Bom[4] = "\xEF\xBB\xBF";
const wchar_t TokenPrefix = L'%';
const wchar_t NoReplacement = wchar_t(0);
const wchar_t TokenReplacement = wchar_t(1);
const UnicodeString LocalInvalidChars(TraceInitStr(L"/\\:*?\"<>|"));
const UnicodeString PasswordMask(TraceInitStr(L"***"));
const UnicodeString Ellipsis(TraceInitStr(L"..."));

UnicodeString ReplaceChar(UnicodeString Str, wchar_t A, wchar_t B)
{
  UnicodeString Result = Str;
  wchar_t * Buffer = const_cast<wchar_t *>(Result.c_str());
  for (wchar_t * Ch = Buffer; Ch && *Ch; ++Ch)
    if (*Ch == A)
    {
      *Ch = B;
    }
  return Result;
}

UnicodeString DeleteChar(UnicodeString Str, wchar_t C)
{
  UnicodeString Result = Str;
  intptr_t P;
  while ((P = Result.Pos(C)) > 0)
  {
    Result.Delete(P, 1);
  }
  return Result;
}

template <typename T>
void DoPackStr(T & Str)
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
    ::ZeroMemory(ToPtr(Str.c_str()), Str.Length() * sizeof(*Str.c_str()));
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

UnicodeString AnsiToString(const RawByteString & S)
{
  return UnicodeString(AnsiString(S));
}

UnicodeString AnsiToString(const char * S, size_t Len)
{
  return UnicodeString(AnsiString(S, Len));
}

UnicodeString MakeValidFileName(UnicodeString AFileName)
{
  UnicodeString Result = AFileName;
  UnicodeString IllegalChars(L":;,=+<>|\"[] \\/?*");
  for (intptr_t Index = 0; Index < IllegalChars.Length(); ++Index)
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

UnicodeString DefaultStr(UnicodeString Str, UnicodeString Default)
{
  if (!Str.IsEmpty())
  {
    return Str;
  }
  return Default;
}

UnicodeString CutToChar(UnicodeString & Str, wchar_t Ch, bool Trim)
{
  intptr_t P = Str.Pos(Ch);
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

UnicodeString CopyToChars(UnicodeString Str, intptr_t & From, UnicodeString Chs, bool Trim,
  wchar_t * Delimiter, bool DoubleDelimiterEscapes)
{
  UnicodeString Result;

  intptr_t P;
  for (P = From; P <= Str.Length(); P++)
  {
    if (::IsDelimiter(Chs, Str, P))
    {
      if (DoubleDelimiterEscapes &&
          (P < Str.Length()) &&
          ::IsDelimiter(Chs, Str, P + 1))
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

UnicodeString CopyToChar(UnicodeString Str, wchar_t Ch, bool Trim)
{
  intptr_t From = 1;
  return CopyToChars(Str, From, UnicodeString(Ch), Trim);
}

UnicodeString DelimitStr(UnicodeString Str, UnicodeString Chars)
{
  UnicodeString Result = Str;

  for (intptr_t Index = 1; Index <= Result.Length(); ++Index)
  {
    if (Result.IsDelimiter(Chars, Index))
    {
      Result.Insert(L"\\", Index);
      ++Index;
    }
  }
  return Result;
}

UnicodeString ShellDelimitStr(UnicodeString Str, wchar_t Quote)
{
  UnicodeString Chars(L"$\\");
  if (Quote == L'"')
  {
    Chars += L"`\"";
  }
  return DelimitStr(Str, Chars);
}

UnicodeString ExceptionLogString(Exception * E)
{
  DebugAssert(E);
  if (isa<Exception>(E))
  {
    UnicodeString Msg = FORMAT("%s", UnicodeString(E->what()));
    if (isa<ExtException>(E))
    {
      TStrings * MoreMessages = dyn_cast<ExtException>(E)->GetMoreMessages();
      if (MoreMessages)
      {
        Msg += L"\n" +
          ReplaceStr(MoreMessages->GetText(), L"\r", L"");
      }
    }
    return Msg;
  }
#if defined(__BORLANDC__)
    wchar_t Buffer[1024];
    ExceptionErrorMessage(ExceptObject(), ExceptAddr(), Buffer, _countof(Buffer));
    return UnicodeString(Buffer);
#else
  return UnicodeString(E->what());
#endif
}

UnicodeString MainInstructions(UnicodeString S)
{
  UnicodeString MainMsgTag = LoadStr(MAIN_MSG_TAG);
  return MainMsgTag + S + MainMsgTag;
}

bool HasParagraphs(UnicodeString S)
{
  return (S.Pos(L"\n\n") > 0);
}

UnicodeString MainInstructionsFirstParagraph(UnicodeString S)
{
  // WORKAROUND, we consider it bad practice, the highlighting should better
  // be localized (but maybe we change our mind later)
  UnicodeString Result;
  intptr_t Pos = S.Pos(L"\n\n");
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
    intptr_t EndTagPos =
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

static intptr_t FindInteractiveMsgStart(UnicodeString S)
{
  intptr_t Result = 0;
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

UnicodeString RemoveMainInstructionsTag(UnicodeString S)
{
  UnicodeString Result = S;

  UnicodeString MainInstruction;
  if (ExtractMainInstructions(Result, MainInstruction))
  {
    Result = MainInstruction + Result;
  }
  return Result;
}

UnicodeString UnformatMessage(UnicodeString S)
{
  UnicodeString Result = RemoveMainInstructionsTag(S);

  intptr_t InteractiveMsgStart = FindInteractiveMsgStart(Result);
  if (InteractiveMsgStart > 0)
  {
    Result = Result.SubString(1, InteractiveMsgStart - 1);
  }
  return Result;
}

UnicodeString RemoveInteractiveMsgTag(UnicodeString S)
{
  UnicodeString Result = S;

  intptr_t InteractiveMsgStart = FindInteractiveMsgStart(Result);
  if (InteractiveMsgStart > 0)
  {
    UnicodeString InteractiveMsgTag = LoadStr(INTERACTIVE_MSG_TAG);
    Result.Delete(InteractiveMsgStart, InteractiveMsgTag.Length());
    Result.Delete(Result.Length() - InteractiveMsgTag.Length() + 1, InteractiveMsgTag.Length());
  }
  return Result;
}

UnicodeString RemoveEmptyLines(UnicodeString S)
{
  return
    ReplaceStr(
      ReplaceStr(S.TrimRight(), L"\n\n", L"\n"),
      L"\n \n", L"\n");
}

bool IsNumber(UnicodeString Str)
{
  int64_t Value = 0;
  if (Str == L"0")
    return true;
  return TryStrToInt(Str, Value);
}

UnicodeString GetSystemTemporaryDirectory()
{
  UnicodeString TempDir;
  TempDir.SetLength(NB_MAX_PATH);
  TempDir.SetLength(::GetTempPath(NB_MAX_PATH, const_cast<LPWSTR>(TempDir.c_str())));
  PackStr(TempDir);
  return TempDir;
}

UnicodeString GetShellFolderPath(intptr_t CSIdl)
{
  UnicodeString Result;
#if defined(_MSC_VER) && !defined(__clang__)
  wchar_t Path[2 * MAX_PATH + 10] = L"\0";
  if (SUCCEEDED(::SHGetFolderPath(nullptr, static_cast<int>(CSIdl), nullptr, SHGFP_TYPE_CURRENT, Path)))
  {
    Result = Path;
  }
#endif // if defined(_MSC_VER) && !defined(__clang__)
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

  if (!DirectoryExists(Result))
  {
    Result = L"";
  }

  return Result;
}

UnicodeString GetPersonalFolder()
{
#if defined(_MSC_VER) && !defined(__clang__)
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
        IncludeTrailingBackslash(WineHome) + L"Documents";
      if (DirectoryExists(WineDocuments))
      {
        Result = WineDocuments;
      }
    }
  }
#endif // if defined(_MSC_VER) && !defined(__clang__)
  return Result;
}

UnicodeString GetDesktopFolder()
{
#if defined(_MSC_VER) && !defined(__clang__)
  UnicodeString Result = GetShellFolderPath(CSIDL_DESKTOPDIRECTORY);

  if (IsWine())
  {
    UnicodeString WineHome = GetWineHomeFolder();

    if (!WineHome.IsEmpty())
    {
      UnicodeString WineDesktop =
        IncludeTrailingBackslash(WineHome) + L"Desktop";
      if (DirectoryExists(WineHome))
      {
        Result = WineDesktop;
      }
    }
  }
#endif // if defined(_MSC_VER) && !defined(__clang__)
  return Result;
}

// Particularly needed when using file name selected by TFilenameEdit,
// as it wraps a path to double-quotes, when there is a space in the path.
UnicodeString StripPathQuotes(UnicodeString APath)
{
  if ((APath.Length() >= 2) &&
      (APath[1] == L'\"') && (APath[APath.Length()] == L'\"'))
  {
    return APath.SubString(2, APath.Length() - 2);
  }
  return APath;
}

UnicodeString AddQuotes(UnicodeString AStr)
{
  UnicodeString Result = AStr;
  if (Result.Pos(L" ") > 0)
  {
    Result = L"\"" + Result + L"\"";
  }
  return Result;
}

UnicodeString AddPathQuotes(UnicodeString APath)
{
  UnicodeString Result = StripPathQuotes(APath);
  return AddQuotes(Result);
}

static wchar_t * ReplaceChar(
  UnicodeString & AFileName, wchar_t * InvalidChar, wchar_t InvalidCharsReplacement)
{
  intptr_t Index = InvalidChar - AFileName.c_str() + 1;
  if (InvalidCharsReplacement == TokenReplacement)
  {
    // currently we do not support unicode chars replacement
    if (AFileName[Index] > 0xFF)
    {
      ThrowExtException();
    }

    AFileName.Insert(ByteToHex(static_cast<uint8_t>(AFileName[Index])), Index + 1);
    AFileName[Index] = TokenPrefix;
    InvalidChar = const_cast<wchar_t *>(AFileName.c_str() + Index + 2);
  }
  else
  {
    AFileName[Index] = InvalidCharsReplacement;
    InvalidChar = const_cast<wchar_t *>(AFileName.c_str() + Index);
  }
  return InvalidChar;
}

UnicodeString ValidLocalFileName(UnicodeString AFileName)
{
  return ValidLocalFileName(AFileName, L'_', L"", LOCAL_INVALID_CHARS);
}

UnicodeString ValidLocalFileName(
  UnicodeString AFileName, wchar_t AInvalidCharsReplacement,
  UnicodeString ATokenizibleChars, UnicodeString ALocalInvalidChars)
{
  UnicodeString Result = AFileName;

  if (AInvalidCharsReplacement != NoReplacement)
  {
    bool ATokenReplacement = (AInvalidCharsReplacement == TokenReplacement);
    UnicodeString CharsStr = ATokenReplacement ? ATokenizibleChars : ALocalInvalidChars;
    const wchar_t * Chars = CharsStr.c_str();
    wchar_t * InvalidChar = const_cast<wchar_t *>(Result.c_str());
    while ((InvalidChar = wcspbrk(InvalidChar, Chars)) != nullptr)
    {
      intptr_t Pos = (InvalidChar - Result.c_str() + 1);
      wchar_t Char;
      if (ATokenReplacement &&
          (*InvalidChar == TokenPrefix) &&
          (((Result.Length() - Pos) <= 1) ||
           (((Char = static_cast<wchar_t>(HexToByte(Result.SubString(Pos + 1, 2)))) == L'\0') ||
            (ATokenizibleChars.Pos(Char) == 0))))
      {
        InvalidChar++;
      }
      else
      {
        InvalidChar = ReplaceChar(Result, InvalidChar, AInvalidCharsReplacement);
      }
    }

    // Windows trim trailing space or dot, hence we must encode it to preserve it
    if (!Result.IsEmpty() &&
        ((Result[Result.Length()] == L' ') ||
         (Result[Result.Length()] == L'.')))
    {
      ReplaceChar(Result, const_cast<wchar_t *>(Result.c_str() + Result.Length() - 1), AInvalidCharsReplacement);
    }

    if (IsReservedName(Result))
    {
      intptr_t P = Result.Pos(L".");
      if (P == 0)
      {
        P = Result.Length() + 1;
      }
      Result.Insert(L"%00", P);
    }
  }
  return Result;
}

void SplitCommand(UnicodeString Command, UnicodeString & Program,
  UnicodeString & Params, UnicodeString & Dir)
{
  UnicodeString Cmd = Command.Trim();
  Params.Clear();
  Dir.Clear();
  if (!Cmd.IsEmpty() && (Cmd[1] == L'\"'))
  {
    Cmd.Delete(1, 1);
    intptr_t P = Cmd.Pos(L'"');
    if (P > 0)
    {
      Program = Cmd.SubString(1, P - 1).Trim();
      Params = Cmd.SubString(P + 1, Cmd.Length() - P).Trim();
    }
    else
    {
      throw Exception(FMTLOAD(INVALID_SHELL_COMMAND, UnicodeString(L"\"" + Cmd)));
    }
  }
  else
  {
    intptr_t P = Cmd.Pos(L" ");
    if (P > 0)
    {
      Program = Cmd.SubString(1, P).Trim();
      Params = Cmd.SubString(P + 1, Cmd.Length() - P).Trim();
    }
    else
    {
      Program = Cmd;
    }
  }
  intptr_t B = Program.LastDelimiter(L"\\/");
  if (B > 0)
  {
    Dir = Program.SubString(1, B).Trim();
  }
}

UnicodeString ExtractProgram(UnicodeString Command)
{
  UnicodeString Program;
  UnicodeString Params;
  UnicodeString Dir;

  SplitCommand(Command, Program, Params, Dir);

  return Program;
}

UnicodeString ExtractProgramName(UnicodeString Command)
{
  UnicodeString Name = base::ExtractFileName(ExtractProgram(Command), false);
  intptr_t Dot = Name.LastDelimiter(L".");
  if (Dot > 0)
  {
    Name = Name.SubString(1, Dot - 1);
  }
  return Name;
}

UnicodeString FormatCommand(UnicodeString Program, UnicodeString AParams)
{
  UnicodeString Result = Program.Trim();
  UnicodeString Params = AParams.Trim();
  if (!Params.IsEmpty())
    Params = L" " + Params;
  Result = AddQuotes(Result);
  return Result + Params;
}

const wchar_t ShellCommandFileNamePattern[] = L"!.!";

void ReformatFileNameCommand(UnicodeString & Command)
{
  if (!Command.IsEmpty())
  {
    UnicodeString Program, Params, Dir;
    SplitCommand(Command, Program, Params, Dir);
    if (Params.Pos(ShellCommandFileNamePattern) == 0)
    {
      Params = Params + (Params.IsEmpty() ? L"" : L" ") + ShellCommandFileNamePattern;
    }
    Command = FormatCommand(Program, Params);
  }
}

UnicodeString ExpandFileNameCommand(UnicodeString Command,
  UnicodeString AFileName)
{
  return AnsiReplaceStr(Command, ShellCommandFileNamePattern,
    AddPathQuotes(AFileName));
}

UnicodeString EscapeParam(UnicodeString AParam)
{
  // Make sure this won't break RTF syntax
  return ReplaceStr(AParam, L"\"", L"\"\"");
}

UnicodeString EscapePuttyCommandParam(UnicodeString AParam)
{
  UnicodeString Result = AParam;

  bool Space = false;

  for (intptr_t Index = 1; Index <= Result.Length(); ++Index)
  {
    switch (Result[Index])
    {
    case L'"':
      Result.Insert(L"\\", Index);
      ++Index;
      break;

    case L' ':
      Space = true;
      break;

    case L'\\':
      intptr_t I2 = Index;
      while ((I2 <= Result.Length()) && (Result[I2] == L'\\'))
      {
        I2++;
      }
      if ((I2 <= Result.Length()) && (Result[I2] == L'"'))
      {
        while (Result[Index] == L'\\')
        {
          Result.Insert(L"\\", Index);
          Index += 2;
        }
        Index--;
      }
      break;
    }
  }

  if (Space)
  {
    Result = L"\"" + Result + L'"';
  }

  return Result;
}

UnicodeString ExpandEnvironmentVariables(UnicodeString Str)
{
  UnicodeString Buf;
  intptr_t Size = 1024;

  Buf.SetLength(Size);
  intptr_t Len = ::ExpandEnvironmentStringsW(Str.c_str(), const_cast<LPWSTR>(Buf.c_str()), static_cast<DWORD>(Size));

  if (Len > Size)
  {
    Buf.SetLength(Len);
    ::ExpandEnvironmentStringsW(Str.c_str(), const_cast<LPWSTR>(Buf.c_str()), static_cast<DWORD>(Len));
  }

  PackStr(Buf);

  return Buf;
}

bool IsPathToSameFile(UnicodeString APath1, UnicodeString APath2)
{
  UnicodeString ShortPath1 = ExtractShortPathName(APath1);
  UnicodeString ShortPath2 = ExtractShortPathName(APath2);

  bool Result;
  // ExtractShortPathName returns empty string if file does not exist
  if (ShortPath1.IsEmpty() || ShortPath2.IsEmpty())
  {
    Result = AnsiSameText(APath1, APath2);
  }
  else
  {
    Result = AnsiSameText(ShortPath1, ShortPath2);
  }
  return Result;
}

bool CompareFileName(UnicodeString APath1, UnicodeString APath2)
{
  return IsPathToSameFile(APath1, APath2);
}

bool ComparePaths(UnicodeString APath1, UnicodeString APath2)
{
  TODO("ExpandUNCFileName");
  return AnsiSameText(::IncludeTrailingBackslash(APath1), ::IncludeTrailingBackslash(APath2));
}

bool SamePaths(UnicodeString APath1, UnicodeString APath2)
{
  TODO("ExpandUNCFileName");
  // TODO: ExpandUNCFileName
  return AnsiSameText(::IncludeTrailingBackslash(APath1), ::IncludeTrailingBackslash(APath2));
}

intptr_t CompareLogicalText(UnicodeString S1, UnicodeString S2)
{
  if (S1.Length() > S2.Length())
  {
    return 1;
  }
  if (S1.Length() < S2.Length())
  {
    return -1;
  }
#if defined(_MSC_VER) && !defined(__clang__)
  return ::StrCmpNCW(S1.c_str(), S2.c_str(), static_cast<int>(S1.Length()));
#else
    return S1.Compare(S2);
#endif
}

bool IsReservedName(UnicodeString AFileName)
{
  UnicodeString FileName = AFileName;

  intptr_t P = FileName.Pos(L".");
  intptr_t Len = (P > 0) ? P - 1 : FileName.Length();
  if ((Len == 3) || (Len == 4))
  {
    if (P > 0)
    {
      FileName.SetLength(P - 1);
    }
    static UnicodeString Reserved[] =
    {
      "CON", "PRN", "AUX", "NUL",
      "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
      "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"
    };
    for (intptr_t Index = 0; Index < static_cast<intptr_t>(_countof(Reserved)); ++Index)
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
// http://stackoverflow.com/q/18580945/850848
// This can be reimplemented using PathCchCanonicalizeEx on Windows 8 and later
enum PATH_PREFIX_TYPE
{
  PPT_UNKNOWN,
  PPT_ABSOLUTE,           //Found absolute path that is none of the other types
  PPT_UNC,                //Found \\server\share\ prefix
  PPT_LONG_UNICODE,       //Found \\?\ prefix
  PPT_LONG_UNICODE_UNC,   //Found \\?\UNC\ prefix
};

static intptr_t PathRootLength(UnicodeString APath)
{
  // Correction for PathSkipRoot API

  // Replace all /'s with \'s because PathSkipRoot can't handle /'s
#if defined(_MSC_VER) && !defined(__clang__)
  UnicodeString Result = ReplaceChar(APath, L'/', L'\\');

  // Now call the API
  LPCTSTR Buffer = ::PathSkipRoot(Result.c_str());

  return (Buffer != nullptr) ? (Buffer - Result.c_str()) : -1;
#else
  return 0;
#endif // if defined(_MSC_VER) && !defined(__clang__)
}

static bool PathIsRelative_CorrectedForMicrosoftStupidity(UnicodeString APath)
{
  // Correction for PathIsRelative API

  // Replace all /'s with \'s because PathIsRelative can't handle /'s
  UnicodeString Result = ReplaceChar(APath, L'/', L'\\');

  //Now call the API
#if defined(_MSC_VER) && !defined(__clang__)
  return ::PathIsRelative(Result.c_str()) != FALSE;
#else
  return false;
#endif // if defined(_MSC_VER) && !defined(__clang__)
}

static intptr_t GetOffsetAfterPathRoot(UnicodeString APath, PATH_PREFIX_TYPE & PrefixType)
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
  intptr_t Result = 0;

  PrefixType = PPT_UNKNOWN;

  if (!APath.IsEmpty())
  {
    intptr_t Len = APath.Length();

    bool WinXPOnly = !IsWinVista();

    // The PathSkipRoot() API doesn't work correctly on Windows XP
    if (!WinXPOnly)
    {
      // Works since Vista and up, but still needs correction :)
      intptr_t RootLength = PathRootLength(APath);
      if (RootLength >= 0)
      {
        Result = RootLength + 1;
      }
    }

    // Now determine the type of prefix
    intptr_t IndCheckUNC = -1;

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
      intptr_t Index = IndCheckUNC;
      for (intptr_t SkipSlashes = 2; SkipSlashes > 0; SkipSlashes--)
      {
        for (; Index <= Len; ++Index)
        {
          TCHAR z = APath[Index];
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
      intptr_t RootLength = PathRootLength(APath.SubString(Result, APath.Length() - Result + 1));
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

static UnicodeString MakeUnicodeLargePath(UnicodeString APath)
{
  // Convert path from 'into a larger Unicode path, that allows up to 32,767 character length
  UnicodeString Result;

  if (!APath.IsEmpty())
  {
    // Determine the type of the existing prefix
    PATH_PREFIX_TYPE PrefixType;
    GetOffsetAfterPathRoot(APath, PrefixType);

    // Assume path to be without change
    Result = APath;

    switch (PrefixType)
    {
    case PPT_ABSOLUTE:
      {
        // First we need to check if its an absolute path relative to the root
        bool AddPrefix = true;
        if ((APath.Length() >= 1) &&
          ((APath[1] == L'\\') || (APath[1] == L'/')))
        {
          AddPrefix = false;

          // Get current root path
          UnicodeString CurrentDir = GetCurrentDir();
          PATH_PREFIX_TYPE PrefixType2; // unused
          intptr_t Following = GetOffsetAfterPathRoot(CurrentDir, PrefixType2);
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

UnicodeString ApiPath(UnicodeString APath)
{
  UnicodeString Result = APath;

  if (IsWin7() || (Result.Length() >= MAX_PATH))
  {
//    if (GetConfiguration() != nullptr)
//    {
//      GetConfiguration()->Usage->Inc(L"LongPath");
//    }
    Result = MakeUnicodeLargePath(Result);
  }
  return Result;
}

UnicodeString DisplayableStr(const RawByteString & Str)
{
  bool Displayable = true;
  intptr_t Index1 = 1;
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
    for (intptr_t Index2 = 1; Index2 <= Str.Length(); ++Index2)
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

UnicodeString BytesToHex(const uint8_t * B, uintptr_t Length, bool UpperCase, wchar_t Separator)
{
  UnicodeString Result;
  for (uintptr_t Index = 0; Index < Length; ++Index)
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
  return BytesToHex(reinterpret_cast<const uint8_t *>(Str.c_str()), Str.Length(), UpperCase, Separator);
}

UnicodeString CharToHex(wchar_t Ch, bool UpperCase)
{
  return BytesToHex(reinterpret_cast<const uint8_t *>(&Ch), sizeof(Ch), UpperCase);
}

RawByteString HexToBytes(UnicodeString Hex)
{
  UnicodeString Digits = "0123456789ABCDEF";
  RawByteString Result;
  intptr_t L = Hex.Length();
  if (L % 2 == 0)
  {
    for (intptr_t Index = 1; Index <= Hex.Length(); Index += 2)
    {
      intptr_t P1 = Digits.Pos(::UpCase(Hex[Index]));
      intptr_t P2 = Digits.Pos(::UpCase(Hex[Index + 1]));
      if (P1 <= 0 || P2 <= 0)
      {
        Result.Clear();
        break;
      }
      Result += static_cast<int8_t>((P1 - 1) * 16 + P2 - 1);
    }
  }
  return Result;
}

uint8_t HexToByte(UnicodeString Hex)
{
  UnicodeString Digits = "0123456789ABCDEF";
  DebugAssert(Hex.Length() == 2);
  intptr_t P1 = Digits.Pos(::UpCase(Hex[1]));
  intptr_t P2 = Digits.Pos(::UpCase(Hex[2]));

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

DWORD FindCheck(DWORD Result, UnicodeString APath)
{
  if ((Result != ERROR_SUCCESS) &&
    (Result != ERROR_FILE_NOT_FOUND) &&
    (Result != ERROR_NO_MORE_FILES))
  {
    throw EOSExtException(FMTLOAD(FIND_FILE_ERROR, APath), Result);
  }
  return Result;
}

DWORD FindFirstUnchecked(UnicodeString APath, DWORD LocalFileAttrs, TSearchRecChecked & F)
{
  F.Path = APath;
  return base::FindFirst(ApiPath(APath), LocalFileAttrs, F);
}

DWORD FindFirstChecked(UnicodeString APath, DWORD LocalFileAttrs, TSearchRecChecked & F)
{
  // return FindCheck(FindFirst(Path, LocalFileAttrs, F));
  DWORD Result = FindFirstUnchecked(APath, LocalFileAttrs, F);
  return FindCheck(Result, F.Path);
}

// Equivalent to FindNext, just to complement to FindFirstUnchecked
DWORD FindNextUnchecked(TSearchRecChecked & F)
{
  return base::FindNext(F);
}

// It can make sense to use FindNextChecked, even if unchecked FindFirst is used.
// I.e. even if we do not care that FindFirst failed, if FindNext
// fails after successful FindFirst, it mean some terrible problem
DWORD FindNextChecked(TSearchRecChecked & F)
{
  return FindCheck(FindNextUnchecked(F), F.Path);
}

bool FileSearchRec(UnicodeString AFileName, TSearchRec & Rec)
{
  DWORD FindAttrs = faReadOnly | faHidden | faSysFile | faDirectory | faArchive;
  bool Result = (base::FindFirst(ApiPath(AFileName), FindAttrs, Rec) == 0);
  if (Result)
  {
    base::FindClose(Rec);
  }
  return Result;
}

void ProcessLocalDirectory(UnicodeString ADirName,
  TProcessLocalFileEvent CallBackFunc, void * Param,
  DWORD FindAttrs)
{
  DebugAssert(CallBackFunc);
  if (FindAttrs == INVALID_FILE_ATTRIBUTES)
  {
    FindAttrs = faReadOnly | faHidden | faSysFile | faDirectory | faArchive;
  }
  TSearchRecChecked SearchRec;

  UnicodeString DirName = ApiPath(::IncludeTrailingBackslash(ADirName));
  if (FindFirstChecked(DirName + L"*.*", FindAttrs, SearchRec) == 0)
  {
    SCOPE_EXIT
    {
      base::FindClose(SearchRec);
    };
    do
    {
      if ((SearchRec.Name != THISDIRECTORY) && (SearchRec.Name != PARENTDIRECTORY))
      {
        UnicodeString FileName = DirName + SearchRec.Name;
        CallBackFunc(FileName, SearchRec, Param);
      }
    }
    while (FindNextChecked(SearchRec) == 0);
  }
}

DWORD FileGetAttrFix(UnicodeString AFileName)
{
  // The default for FileGetAttr is to follow links
  bool FollowLink = true;
  // But the FileGetAttr whe called for link with FollowLink set will always fail
  // as its calls InternalGetFileNameFromSymLink, which test for CheckWin32Version(6, 0)
  if (!IsWinVista())
  {
    FollowLink = false;
  }
  return ::FileGetAttr(AFileName, FollowLink);
}

TDateTime EncodeDateVerbose(Word Year, Word Month, Word Day)
{
  TDateTime Result;
  try
  {
    Result = EncodeDate(Year, Month, Day);
  }
  catch (EConvertError & E)
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
  catch (EConvertError & E)
  {
    throw EConvertError(FORMAT("%s [%02u:%02u:%02u.%04u]", E.Message, int(Hour), int(Min), int(Sec), int(MSec)));
  }
  return Result;
}

TDateTime SystemTimeToDateTimeVerbose(const SYSTEMTIME & SystemTime)
{
  try
  {
    TDateTime DateTime = SystemTimeToDateTime(SystemTime);
    return DateTime;
  }
  catch (EConvertError & E)
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
    ClearStruct(SystemStandardDate);
    ClearStruct(SystemDaylightDate);
  }

  TDateTime UnixEpoch;
  double BaseDifference;
  intptr_t BaseDifferenceSec;
  // All Current* are actually global, not per-year
  // are valid for Year 0 (current) only
  double CurrentDaylightDifference;
  intptr_t CurrentDaylightDifferenceSec;
  double CurrentDifference;
  intptr_t CurrentDifferenceSec;
  double StandardDifference;
  intptr_t StandardDifferenceSec;
  double DaylightDifference;
  intptr_t DaylightDifferenceSec;
  SYSTEMTIME SystemStandardDate;
  SYSTEMTIME SystemDaylightDate;
  TDateTime StandardDate;
  TDateTime DaylightDate;
  UnicodeString StandardName;
  UnicodeString DaylightName;
  // This is actually global, not per-year
  bool DaylightHack;

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

typedef rde::map<int, TDateTimeParams> TYearlyDateTimeParams;
static TYearlyDateTimeParams YearlyDateTimeParams;
static TCriticalSection DateTimeParamsSection;
static void EncodeDSTMargin(const SYSTEMTIME & Date, uint16_t Year,
  TDateTime & Result);

static uint16_t DecodeYear(const TDateTime & DateTime)
{
  uint16_t Year, Month, Day;
  DecodeDate(DateTime, Year, Month, Day);
  return Year;
}

static const TDateTimeParams * GetDateTimeParams(uint16_t Year)
{
  TGuard Guard(DateTimeParamsSection);

  TDateTimeParams * Result;

  TYearlyDateTimeParams::iterator it = YearlyDateTimeParams.find(Year);
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

    HINSTANCE Kernel32 = ::GetModuleHandle(L"kernel32.dll");
    typedef BOOL (WINAPI * TGetTimeZoneInformationForYear)
      (USHORT wYear, PDYNAMIC_TIME_ZONE_INFORMATION pdtzi, LPTIME_ZONE_INFORMATION ptzi);
    TGetTimeZoneInformationForYear GetTimeZoneInformationForYear =
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
    Result->BaseDifference = static_cast<double>(TZI.Bias) / MinsPerDay;
    Result->BaseDifferenceSec *= SecsPerMin;

    Result->CurrentDifferenceSec = TZI.Bias +
      Result->CurrentDaylightDifferenceSec;
    Result->CurrentDifference =
      static_cast<double>(Result->CurrentDifferenceSec) / MinsPerDay;
    Result->CurrentDifferenceSec *= SecsPerMin;

    Result->CurrentDaylightDifference =
      static_cast<double>(Result->CurrentDaylightDifferenceSec) / MinsPerDay;
    Result->CurrentDaylightDifferenceSec *= SecsPerMin;

    Result->DaylightDifferenceSec = TZI.DaylightBias * SecsPerMin;
    Result->DaylightDifference = static_cast<double>(TZI.DaylightBias) / MinsPerDay;
    Result->StandardDifferenceSec = TZI.StandardBias * SecsPerMin;
    Result->StandardDifference = static_cast<double>(TZI.StandardBias) / MinsPerDay;

    Result->SystemStandardDate = TZI.StandardDate;
    Result->SystemDaylightDate = TZI.DaylightDate;

    uint16_t AYear = (Year != 0) ? Year : DecodeYear(Now());
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

static void EncodeDSTMargin(const SYSTEMTIME & Date, uint16_t Year,
  TDateTime & Result)
{
  if (Date.wYear == 0)
  {
    TDateTime Temp = EncodeDateVerbose(Year, Date.wMonth, 1);
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

  TDateTime Result = TDateTime(UnixDateDelta + (static_cast<double>(TimeStamp) / SecsPerDay));
  const TDateTimeParams * Params = GetDateTimeParams(DecodeYear(Result));

  if (Params->DaylightHack)
  {
    if ((DSTMode == dstmWin) || (DSTMode == dstmUnix))
    {
      const TDateTimeParams * CurrentParams = GetDateTimeParams(0);
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
  return static_cast<int64_t>(((Number - Floor) > (Ceil - Number)) ? Ceil : Floor);
}

bool TryRelativeStrToDateTime(UnicodeString AStr, TDateTime & DateTime, bool Add)
{
  UnicodeString S = AStr.Trim();
  intptr_t Index = 1;
  while ((Index <= S.Length()) && IsDigit(S[Index]))
  {
    ++Index;
  }
  UnicodeString NumberStr = S.SubString(1, Index - 1);
  int64_t Number = 0;
  bool Result = TryStrToInt(NumberStr, Number);
  if (Result)
  {
    if (!Add)
    {
      Number = -Number;
    }
    S.Delete(1, Index - 1);
    S = S.Trim().UpperCase();
    DateTime = Now();
    // These may not overlap with ParseSize (K, M and G)
    if (S == "S")
    {
      DateTime = IncSecond(DateTime, Number);
    }
    else if (S == "N")
    {
      DateTime = IncMinute(DateTime, Number);
    }
    else if (S == "H")
    {
      DateTime = IncHour(DateTime, Number);
    }
    else if (S == "D")
    {
      DateTime = IncDay(DateTime, Number);
    }
    else if (S == "Y")
    {
      DateTime = IncYear(DateTime, Number);
    }
    else
    {
      Result = false;
    }
  }
  return Result;
}

const wchar_t KiloSize = L'K';
const wchar_t MegaSize = L'M';
const wchar_t GigaSize = L'G';

// Keep consistent with parse_blocksize
bool TryStrToSize(UnicodeString SizeStr, int64_t & Size)
{
  intptr_t Index = 0;
  while ((Index + 1 <= SizeStr.Length()) && IsDigit(SizeStr[Index + 1]))
  {
    Index++;
  }
  bool Result =
    (Index > 0) && TryStrToInt64(SizeStr.SubString(1, Index), Size);
  if (Result)
  {
    SizeStr = SizeStr.SubString(Index + 1, SizeStr.Length() - Index).Trim();
    if (!SizeStr.IsEmpty())
    {
      Result = (SizeStr.Length() == 1);
      if (Result)
      {
        wchar_t Unit = static_cast<wchar_t>(toupper(SizeStr[1]));
        switch (Unit)
        {
        case GigaSize:
          Size *= 1024;
          // fallthru
        case MegaSize:
          Size *= 1024;
          // fallthru
        case KiloSize:
          Size *= 1024;
          break;
        default:
          Result = false;
        }
      }
    }
  }
  return Result;
}

UnicodeString SizeToStr(int64_t Size)
{
  UnicodeString Result;
  if ((Size <= 0) || ((Size % 1024) != 0))
  {
    Result = Int64ToStr(Size);
  }
  else
  {
    Size /= 1024;
    if ((Size % 1024) != 0)
    {
      Result = Int64ToStr(Size) + KiloSize;
    }
    else
    {
      Size /= 1024;
      if ((Size % 1024) != 0)
      {
        Result = Int64ToStr(Size) + MegaSize;
      }
      else
      {
        Size /= 1024;
        Result = Int64ToStr(Size) + GigaSize;
      }
    }
  }
  return Result;
}

static int64_t DateTimeToUnix(const TDateTime & DateTime)
{
  const TDateTimeParams * CurrentParams = GetDateTimeParams(0);

  DebugAssert(int(EncodeDateVerbose(1970, 1, 1)) == UnixDateDelta);

  return Round(static_cast<double>(DateTime - UnixDateDelta) * SecsPerDay) +
    CurrentParams->CurrentDifferenceSec;
}

FILETIME DateTimeToFileTime(const TDateTime & DateTime,
  TDSTMode /*DSTMode*/)
{
  int64_t UnixTimeStamp = ::DateTimeToUnix(DateTime);

  const TDateTimeParams * Params = GetDateTimeParams(DecodeYear(DateTime));
  if (!Params->DaylightHack)
  {
    // We should probably use reversed code of FileTimeToDateTime here instead of custom implementation

    // We are incrementing and decrementing BaseDifferenceSec because it
    // can actually change between years
    // (as it did in Belarus from GMT+2 to GMT+3 between 2011 and 2012)

    UnixTimeStamp += (IsDateInDST(DateTime) ?
      Params->DaylightDifferenceSec : Params->StandardDifferenceSec) +
      Params->BaseDifferenceSec;

    const TDateTimeParams * CurrentParams = GetDateTimeParams(0);
    UnixTimeStamp -=
      CurrentParams->CurrentDaylightDifferenceSec +
      CurrentParams->BaseDifferenceSec;
  }

  FILETIME Result;
  (*reinterpret_cast<int64_t*>(&(Result)) = (static_cast<int64_t>(UnixTimeStamp) + 11644473600LL) * 10000000LL);

  return Result;
}

TDateTime FileTimeToDateTime(const FILETIME & FileTime)
{
  // duplicated in DirView.pas
  TDateTime Result;
  // The 0xFFF... is sometime seen for invalid timestamps,
  // it would cause failure in SystemTimeToDateTime below
  if (FileTime.dwLowDateTime == DWORD(-1) / sizeof(DWORD)) //std::numeric_limits<DWORD>::max) //
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
#if 0
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
  TDateTime Result = SystemTimeToDateTime(SysTime);
#endif // #if 0
  return Result;
}

int64_t ConvertTimestampToUnix(const FILETIME & FileTime,
  TDSTMode DSTMode)
{
  int64_t Result = ((*(int64_t *)& (FileTime)) / 10000000LL - 11644473600LL);
  if (UsesDaylightHack())
  {
    if ((DSTMode == dstmUnix) || (DSTMode == dstmKeep))
    {
      FILETIME LocalFileTime;
      SYSTEMTIME SystemTime;
      FileTimeToLocalFileTime(&FileTime, &LocalFileTime);
      FileTimeToSystemTime(&LocalFileTime, &SystemTime);
      TDateTime DateTime = SystemTimeToDateTimeVerbose(SystemTime);
      const TDateTimeParams * Params = GetDateTimeParams(DecodeYear(DateTime));
      Result += (IsDateInDST(DateTime) ?
        Params->DaylightDifferenceSec : Params->StandardDifferenceSec);

      if (DSTMode == dstmKeep)
      {
        const TDateTimeParams * CurrentParams = GetDateTimeParams(0);
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
      TDateTime DateTime = SystemTimeToDateTimeVerbose(SystemTime);
      const TDateTimeParams * Params = GetDateTimeParams(DecodeYear(DateTime));
      Result -= (IsDateInDST(DateTime) ?
        Params->DaylightDifferenceSec : Params->StandardDifferenceSec);
    }
  }

  return Result;
}

TDateTime ConvertTimestampToUTC(const TDateTime & DateTime)
{
  TDateTime Result = DateTime;

  const TDateTimeParams * Params = GetDateTimeParams(DecodeYear(DateTime));
  Result += DSTDifferenceForTime(DateTime);
  Result += Params->BaseDifference;

  if (Params->DaylightHack)
  {
    const TDateTimeParams * CurrentParams = GetDateTimeParams(0);
    Result += CurrentParams->CurrentDaylightDifference;
  }

  return Result;
}

TDateTime ConvertTimestampFromUTC(const TDateTime & DateTime)
{
  TDateTime Result = DateTime;

  const TDateTimeParams * Params = GetDateTimeParams(DecodeYear(Result));
  Result -= DSTDifferenceForTime(DateTime);
  Result -= Params->BaseDifference;

  if (Params->DaylightHack)
  {
    const TDateTimeParams * CurrentParams = GetDateTimeParams(0);
    Result -= CurrentParams->CurrentDaylightDifference;
  }

  return Result;
}

int64_t ConvertTimestampToUnixSafe(const FILETIME & FileTime,
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

double DSTDifferenceForTime(const TDateTime & DateTime)
{
  double Result;
  const TDateTimeParams * Params = GetDateTimeParams(DecodeYear(DateTime));
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

TDateTime AdjustDateTimeFromUnix(const TDateTime & DateTime, TDSTMode DSTMode)
{
  TDateTime Result = DateTime;

  const TDateTimeParams * Params = GetDateTimeParams(DecodeYear(Result));

  if (Params->DaylightHack)
  {
    if ((DSTMode == dstmWin) || (DSTMode == dstmUnix))
    {
      const TDateTimeParams * CurrentParams = GetDateTimeParams(0);
      Result = Result - CurrentParams->CurrentDaylightDifference;
    }

    if (!IsDateInDST(Result))
    {
      if (DSTMode == dstmWin)
      {
        Result = Result - Params->DaylightDifference;
      }
    }
    else
    {
      Result = Result - Params->StandardDifference;
    }
  }
  else
  {
    if (DSTMode == dstmWin)
    {
      Result = Result + DSTDifferenceForTime(Result);
    }
  }

  return Result;
}

UnicodeString FixedLenDateTimeFormat(UnicodeString Format)
{
  UnicodeString Result = Format;
  bool AsIs = false;

  intptr_t Index = 1;
  while (Index <= Result.Length())
  {
    wchar_t F = Result[Index];
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

UnicodeString FormatTimeZone(intptr_t /*Sec*/)
{
  UnicodeString Str;
  TODO("implement class TTimeSpan");
  /*
  TTimeSpan Span = TTimeSpan::FromSeconds(Sec);
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
  Str = ((Span <= TTimeSpan::Zero) ? L"+" : L"") + Str;*/
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
        CurrentParams->DaylightDate.DateString(),
        CurrentParams->StandardDate.DateString());
  }

  return Result;
}

bool AdjustClockForDSTEnabled()
{
  // Windows XP deletes the DisableAutoDaylightTimeSet value when it is off
  // (the later versions set it to DynamicDaylightTimeDisabled to 0)
  bool DynamicDaylightTimeDisabled = false;
  std::unique_ptr<TRegistry> Registry(new TRegistry());
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

UnicodeString StandardTimestamp(const TDateTime & DateTime)
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

intptr_t CompareFileTime(const TDateTime & T1, const TDateTime & T2)
{
  TDateTime TwoSeconds(0, 0, 2, 0);
  // "FAT" time precision
  // (when one time is seconds-precision and other is millisecond-precision,
  // we may have times like 12:00:00.000 and 12:00:01.999, which should
  // be treated the same)
  intptr_t Result;
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

intptr_t TimeToMSec(const TDateTime & T)
{
  return int(Round(double(T) * double(MSecsPerDay)));
}

intptr_t TimeToSeconds(const TDateTime & T)
{
  return TimeToMSec(T) / MSecsPerSec;
}

intptr_t TimeToMinutes(const TDateTime & T)
{
  return TimeToSeconds(T) / SecsPerMin;
}

static bool DoRecursiveDeleteFile(UnicodeString AFileName, bool ToRecycleBin, UnicodeString & AErrorPath)
{
  bool Result;

  UnicodeString ErrorPath = AFileName;

  if (!ToRecycleBin)
  {
    TSearchRecChecked SearchRec;
    Result = FileSearchRec(AFileName, SearchRec);
    if (Result)
    {
      if (FLAGCLEAR(SearchRec.Attr, faDirectory))
      {
        Result = ::RemoveFile(AFileName);
      }
      else
      {
        Result = (FindFirstUnchecked(AFileName + L"\\*", faAnyFile, SearchRec) == 0);

        if (Result)
        {
          {
            SCOPE_EXIT
            {
              base::FindClose(SearchRec);
            };
            do
            {
              UnicodeString FileName2 = AFileName + L"\\" + SearchRec.Name;
              if (FLAGSET(SearchRec.Attr, faDirectory))
              {
                if ((SearchRec.Name != L".") && (SearchRec.Name != L".."))
                {
                  Result = DoRecursiveDeleteFile(FileName2, DebugAlwaysFalse(ToRecycleBin), ErrorPath);
                }
              }
              else
              {
                Result = ::RemoveFile(FileName2);
                if (!Result)
                {
                  ErrorPath = FileName2;
                }
              }
            }
            while (Result && (FindNextUnchecked(SearchRec) == 0));
          }

          if (Result)
          {
            Result = ::RemoveDir(AFileName);
          }
        }
      }
    }
  }
  else
  {
    SHFILEOPSTRUCT Data;

    ClearStruct(Data);
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
      ::SetLastError(ErrorCode);
    }
  }

  if (!Result)
  {
    AErrorPath = ErrorPath;
  }

  return Result;
}

bool RecursiveDeleteFile(UnicodeString AFileName, bool ToRecycleBin)
{
  UnicodeString ErrorPath; // unused
  bool Result = DoRecursiveDeleteFile(AFileName, ToRecycleBin, ErrorPath);
  return Result;
}

void RecursiveDeleteFileChecked(UnicodeString AFileName, bool ToRecycleBin)
{
  UnicodeString ErrorPath;
  if (!DoRecursiveDeleteFile(AFileName, ToRecycleBin, ErrorPath))
  {
    throw EOSExtException(FMTLOAD(CORE_DELETE_LOCAL_FILE_ERROR, ErrorPath));
  }
}

void DeleteFileChecked(UnicodeString AFileName)
{
  if (!::RemoveFile(AFileName))
  {
    throw EOSExtException(FMTLOAD(CORE_DELETE_LOCAL_FILE_ERROR, AFileName));
  }
}

uintptr_t CancelAnswer(uintptr_t Answers)
{
  uintptr_t Result;
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

uintptr_t AbortAnswer(uintptr_t Answers)
{
  uintptr_t Result;
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

uintptr_t ContinueAnswer(uintptr_t Answers)
{
  uintptr_t Result;
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

UnicodeString LoadStr(intptr_t Ident, uintptr_t /*MaxLength*/)
{
  UnicodeString Result = GetGlobals()->GetMsg(Ident);
  return Result;
}

UnicodeString LoadStrPart(intptr_t Ident, intptr_t Part)
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

UnicodeString DecodeUrlChars(UnicodeString S)
{
  UnicodeString Result = S;

  intptr_t Index = 1;
  while (Index <= Result.Length())
  {
    switch (Result[Index])
    {
    case L'+':
      Result[Index] = L' ';
      break;

    case L'%':
      UnicodeString Hex;
      while ((Index + 2 <= Result.Length()) && (Result[Index] == L'%') &&
        IsHex(Result[Index + 1]) && IsHex(Result[Index + 2]))
      {
        Hex += Result.SubString(Index + 1, 2);
        Result.Delete(Index, 3);
      }

      if (!Hex.IsEmpty())
      {
        RawByteString Bytes = HexToBytes(Hex);
        UTF8String UTF8(Bytes.c_str(), Bytes.Length());
        UnicodeString Chars(UTF8);
        Result.Insert(Chars, Index);
        Index += Chars.Length() - 1;
      }
      break;
    }
    ++Index;
  }
  return Result;
}

UnicodeString DoEncodeUrl(UnicodeString S, bool EncodeSlash)
{
  UnicodeString Result = S;

  intptr_t Index = 1;
  while (Index <= Result.Length())
  {
    wchar_t C = Result[Index];
    if (IsLetter(C) ||
        IsDigit(C) ||
        (C == L'_') || (C == L'-') || (C == L'.') ||
        ((C == L'/') && !EncodeSlash))
    {
      ++Index;
    }
    else
    {
      UTF8String UtfS(Result.SubString(Index, 1));
      UnicodeString H;
      for (intptr_t Index2 = 1; Index2 <= UtfS.Length(); ++Index2)
      {
        H += L"%" + ByteToHex(static_cast<uint8_t>(UtfS[Index2]));
      }
      Result.Delete(Index, 1);
      Result.Insert(H, Index);
      Index += H.Length();
    }
#if 0
    if (Chars.Pos(Result[Index]) > 0)
    {
      UnicodeString H = ByteToHex(AnsiString(UnicodeString(Result[Index]))[1]);
      Result.Insert(H, Index + 1);
      Result[Index] = L'%';
      Index += H.Length();
    }
    ++Index;
#endif
  }
  return Result;
}

UnicodeString EncodeUrlString(UnicodeString S)
{
  return DoEncodeUrl(S, true);
}

UnicodeString EncodeUrlPath(UnicodeString S)
{
  return DoEncodeUrl(S, false);
}

UnicodeString AppendUrlParams(UnicodeString AURL, UnicodeString Params)
{
  UnicodeString URL = AURL;
  // see also TWebHelpSystem::ShowHelp
  const wchar_t FragmentSeparator = L'#';
  UnicodeString Result = ::CutToChar(URL, FragmentSeparator, false);

  if (Result.Pos(L"?") == 0)
  {
    Result += L"?";
  }
  else
  {
    Result += L"&";
  }

  Result += Params;

  AddToList(Result, URL, FragmentSeparator);

  return Result;
}

UnicodeString ExtractFileNameFromUrl(UnicodeString Url)
{
  UnicodeString Result = Url;
  intptr_t P = Result.Pos(L"?");
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

UnicodeString EscapeHotkey(UnicodeString Caption)
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
  intptr_t Index = 1;
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
      if ((AStr[Index] == L'"') && (Index + 1 <= AStr.Length()) &&
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

void AddToList(UnicodeString & List, UnicodeString Value, UnicodeString Delimiter)
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

static bool CheckWin32Version(int Major, int Minor)
{
  return (GetGlobals()->Win32MajorVersion >= Major) && (GetGlobals()->Win32MinorVersion >= Minor);
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

bool IsWine()
{
  HMODULE NtDll = ::GetModuleHandle(L"ntdll.dll");
  return
    DebugAlwaysTrue(NtDll != nullptr) &&
    (::GetProcAddress(NtDll, "wine_get_version") != nullptr);
}

LCID GetDefaultLCID()
{
  return GetUserDefaultLCID();
}

UnicodeString DefaultEncodingName()
{
  static UnicodeString DefaultEncodingName;
  if (DefaultEncodingName.IsEmpty())
  {
    CPINFOEX Info;
    GetCPInfoEx(CP_ACP, 0, &Info);
    DefaultEncodingName = Info.CodePageName;
  }
  return DefaultEncodingName;
}

bool GetWindowsProductType(DWORD & Type)
{
  bool Result;
  HINSTANCE Kernel32 = ::GetModuleHandle(L"kernel32.dll");
  typedef BOOL (WINAPI
  * TGetProductInfo
  )
  (DWORD , DWORD , DWORD , DWORD , PDWORD);
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
  std::unique_ptr<TRegistry> Registry(new TRegistry());
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
  catch (...)
  {
    DEBUG_PRINTF("WindowsProductName: error");
  }
  return Result;
}

UnicodeString WindowsVersion()
{
  UnicodeString Result;
  OSVERSIONINFO OSVersionInfo;
  OSVersionInfo.dwOSVersionInfoSize = sizeof(OSVersionInfo);
  // Cannot use the VCL Win32MajorVersion+Win32MinorVersion+Win32BuildNumber as
  // on Windows 10 due to some hacking in InitPlatformId, the Win32BuildNumber is lost
  if (GetVersionEx(&OSVersionInfo) != 0)
  {
    Result = FORMAT("%d.%d.%d", int(OSVersionInfo.dwMajorVersion), int(OSVersionInfo.dwMinorVersion), int(OSVersionInfo.dwBuildNumber));
  }
  return Result;
}

UnicodeString WindowsVersionLong()
{
  UnicodeString Result = WindowsVersion();
  AddToList(Result, GetGlobals()->Win32CSDVersion, L" ");
  return Result;
}

bool IsDirectoryWriteable(UnicodeString APath)
{
  UnicodeString FileName =
    ::IncludeTrailingPathDelimiter(APath) +
    FORMAT("wscp_%s_%d.tmp", FormatDateTime(L"nnzzz", Now()), int(GetCurrentProcessId()));
  HANDLE LocalFileHandle = ::CreateFile(ApiPath(FileName).c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
    CREATE_NEW, FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
  bool Result = (LocalFileHandle != INVALID_HANDLE_VALUE);
  if (Result)
  {
    SAFE_CLOSE_HANDLE(LocalFileHandle);
  }
  return Result;
}

UnicodeString FormatNumber(int64_t Number)
{
//  return FormatFloat(L"#,##0", Number);
  return FORMAT("%.0f", ToDouble(Number));
}

// simple alternative to FormatBytes
UnicodeString FormatSize(int64_t Size)
{
  return FormatNumber(Size);
}

UnicodeString FormatDateTimeSpan(const UnicodeString TimeFormat, TDateTime DateTime)
{
  UnicodeString Result;
  try
  {
    if (static_cast<int64_t>(DateTime) > 0)
    {
      Result = Int64ToStr(static_cast<int64_t>(DateTime)) + L", ";
    }
    // days are decremented, because when there are to many of them,
    // "integer overflow" error occurs
    Result += FormatDateTime(TimeFormat, DateTime - TDateTime(static_cast<double>(static_cast<int64_t>(DateTime))));
  }
  catch (...)
  {
    //log error
    DEBUG_PRINTF("FormatDateTimeSpan: error, params: %s, %f", TimeFormat, DateTime.GetValue());
  }
  return Result;
}


UnicodeString ExtractFileBaseName(UnicodeString APath)
{
  return ChangeFileExt(base::ExtractFileName(APath, false), L"");
}

TStringList * TextToStringList(UnicodeString Text)
{
  std::unique_ptr<TStringList> List(new TStringList());
  List->SetText(Text);
  return List.release();
}

UnicodeString StringsToText(TStrings * Strings)
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

TStrings * CloneStrings(TStrings * Strings)
{
  std::unique_ptr<TStringList> List(new TStringList());
  List->AddStrings(Strings);
  return List.release();
}

UnicodeString TrimVersion(UnicodeString Version)
{
  UnicodeString Result = Version;

  while ((Result.Pos(L".") != Result.LastDelimiter(L".")) &&
    (Result.SubString(Result.Length() - 1, 2) == L".0"))
  {
    Result.SetLength(Result.Length() - 2);
  }
  return Result;
}

UnicodeString FormatVersion(intptr_t MajorVersion, intptr_t MinorVersion, intptr_t Patch)
{
  return FORMAT("%d.%d.%d", static_cast<int>(MajorVersion), static_cast<int>(MinorVersion), static_cast<int>(Patch));
}

TFormatSettings GetEngFormatSettings()
{
  return TFormatSettings::Create(1033);
}

static intptr_t IndexStr(UnicodeString AStr)
{
  intptr_t Result = -1;
  for (intptr_t Index = 0; Index < 12; ++Index)
  {
    if (AStr.CompareIC(EngShortMonthNames[Index]) == 0)
    {
      Result = Index;
      break;
    }
  }
  return Result;
}

intptr_t ParseShortEngMonthName(UnicodeString MonthStr)
{
  // TFormatSettings FormatSettings = GetEngFormatSettings();
  // return IndexStr(MonthStr, FormatSettings.ShortMonthNames, FormatSettings.ShortMonthNames.size()) + 1;
  return IndexStr(MonthStr) + 1;
}

TStringList * CreateSortedStringList(bool CaseSensitive, TDuplicatesEnum Duplicates)
{
  TStringList * Result = new TStringList();
  Result->SetCaseSensitive(CaseSensitive);
  Result->SetSorted(true);
  Result->SetDuplicates(Duplicates);
  return Result;
}

static UnicodeString NormalizeIdent(UnicodeString Ident)
{
  UnicodeString Result = Ident;
  intptr_t Index = 1;
  while (Index <= Result.Length())
  {
    if (Result[Index] == L'-')
    {
      Result.Delete(Index, 1);
    }
    else
    {
      Index++;
    }
  }
  return Result;
}

UnicodeString FindIdent(UnicodeString Ident, TStrings * Idents)
{
  UnicodeString NormalizedIdent(NormalizeIdent(Ident));
  for (intptr_t Index = 0; Index < Idents->GetCount(); Index++)
  {
    if (SameText(NormalizedIdent, NormalizeIdent(Idents->GetString(Index))))
    {
      return Idents->GetString(Index);
    }
  }
  return Ident;
}

#if defined(HAVE_OPENSSL)

static UnicodeString GetTlsErrorStr(int Err)
{
  char Buffer[512];
  ERR_error_string(Err, Buffer);
  // not sure about the UTF8
  return UnicodeString(UTF8String(Buffer));
}


static FILE * OpenCertificate(UnicodeString Path)
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
  UnicodeString * Passphrase;
};


static int PemPasswordCallback(char * Buf, int Size, int /*RWFlag*/, void * UserData)
{
  TPemPasswordCallbackData & Data = *reinterpret_cast<TPemPasswordCallbackData *>(UserData);
  UTF8String UtfPassphrase = UTF8String(*Data.Passphrase);
  strncpy(Buf, UtfPassphrase.c_str(), Size);
  Shred(UtfPassphrase);
  Buf[Size - 1] = '\0';
  return static_cast<int>(strlen(Buf));
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
    (HasPassphrase && (ErrorLib == ERR_LIB_EVP) &&
      ((ErrorReason == PEM_R_BAD_DECRYPT) || (ErrorReason == PEM_R_BAD_BASE64_DECODE)));

  return Result;
}


static void ThrowTlsCertificateErrorIgnorePassphraseErrors(UnicodeString Path, bool HasPassphrase)
{
  int Error = ERR_get_error();
  if (!IsTlsPassphraseError(Error, HasPassphrase))
  {
    throw ExtException(MainInstructions(FMTLOAD(CERTIFICATE_READ_ERROR, Path)), GetTlsErrorStr(Error));
  }
}


void ParseCertificate(const UnicodeString Path,
  const UnicodeString Passphrase, X509 *& Certificate, EVP_PKEY *& PrivateKey,
  bool & WrongPassphrase)
{
  Certificate = nullptr;
  PrivateKey = nullptr;
  bool HasPassphrase = !Passphrase.IsEmpty();

  // Inspired by neon's ne_ssl_clicert_read
  FILE * File = OpenCertificate(Path);
  // openssl pkcs12 -inkey cert.pem -in cert.crt -export -out cert.pfx
  // Binary file
  PKCS12 * Pkcs12 = d2i_PKCS12_fp(File, nullptr);
  fclose(File);

  if (Pkcs12 != nullptr)
  {
    // Modeled after OPENSSL_asc2uni (reversed bitness to what UnicodeString/wchar_t use)
    rde::vector<char> Buf;
    Buf.resize(Passphrase.Length() * sizeof(wchar_t) + sizeof(wchar_t));
    for (intptr_t Index = 0; Index <= Passphrase.Length(); Index++)
    {
      Buf[(Index * 2)] = (Passphrase.c_str()[Index] >> 8);
      Buf[(Index * 2) + 1] = (Passphrase.c_str()[Index] & 0x00FF);
    }

    bool Result =
      (PKCS12_parse(Pkcs12, &Buf[0], &PrivateKey, &Certificate, nullptr) == 1);
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
      SCOPE_EXIT
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
      };
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
        int Error = ERR_get_error();
        // unlikely
        if (IsTlsPassphraseError(Error, HasPassphrase))
        {
          WrongPassphrase = true;
        }
        else
        {
          UnicodeString CertificatePath = ChangeFileExt(Path, L".cer");
          if (!FileExists(CertificatePath))
          {
            CertificatePath = ChangeFileExt(Path, L".crt");
          }

          if (!FileExists(CertificatePath))
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
              int Base64Error = ERR_get_error();

              File = OpenCertificate(CertificatePath);
              // Binary DER-encoded certificate
              // (as above, with BEGIN/END removed, and decoded from Base64 to binary)
              // openssl x509 -in cert.crt -out client.der.crt -outform DER
              Certificate = d2i_X509_fp(File, nullptr);
              fclose(File);

              if (Certificate == nullptr)
              {
                int DERError = ERR_get_error();

                UnicodeString Message = MainInstructions(FMTLOAD(CERTIFICATE_READ_ERROR, CertificatePath));
                UnicodeString MoreMessages =
                  FORMAT("Base64: %s\nDER: %s", GetTlsErrorStr(Base64Error), GetTlsErrorStr(DERError));
                throw ExtException(Message, MoreMessages);
              }
            }
          }
        }
      }
    }
    __finally
    {
/*
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
*/
    };
  }
}


void CheckCertificate(UnicodeString Path)
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

#endif // HAVE_OPENSSL

const UnicodeString HttpProtocol(L"http");
const UnicodeString HttpsProtocol(L"https");
const UnicodeString ProtocolSeparator(L"://");

bool IsHttpUrl(UnicodeString S)
{
  return StartsText(HttpProtocol + ProtocolSeparator, S);
}

bool IsHttpOrHttpsUrl(UnicodeString S)
{
  return
    IsHttpUrl(S) ||
    StartsText(HttpsProtocol + ProtocolSeparator, S);
}

UnicodeString ChangeUrlProtocol(UnicodeString S, UnicodeString Protocol)
{
  intptr_t P = S.Pos(ProtocolSeparator);
  DebugAssert(P > 0);
  return Protocol + ProtocolSeparator + RightStr(S, S.Length() - P - ProtocolSeparator.Length() + 1);
}

#if 0
// implemented in FarInterface.cpp
const UnicodeString RtfPara = L"\\par\n";
const UnicodeString RtfHyperlinkField = L"HYPERLINK";
const UnicodeString RtfHyperlinkFieldPrefix = RtfHyperlinkField + L" \"";
#endif // #if 0

UnicodeString StripEllipsis(UnicodeString S)
{
  UnicodeString Result = S;
  if (Result.SubString(Result.Length() - Ellipsis.Length() + 1, Ellipsis.Length()) == Ellipsis)
  {
    Result.SetLength(Result.Length() - Ellipsis.Length());
    Result = Result.TrimRight();
  }
  return Result;
}

namespace base {

UnicodeString FormatBytes(int64_t Bytes, bool UseOrders)
{
  UnicodeString Result;

  if (!UseOrders || (Bytes < static_cast<int64_t>(100 * 1024)))
  {
    // Result = FormatFloat(L"#,##0 \"B\"", Bytes);
    Result = FORMAT("%.0f B", ToDouble(Bytes));
  }
  else if (Bytes < static_cast<int64_t>(100 * 1024 * 1024))
  {
    // Result = FormatFloat(L"#,##0 \"KB\"", Bytes / 1024);
    Result = FORMAT("%.0f KB", ToDouble(Bytes / 1024.0));
  }
  else
  {
    // Result = FormatFloat(L"#,##0 \"MiB\"", Bytes / (1024*1024));
    Result = FORMAT("%.0f MiB", ToDouble(Bytes / (1024 * 1024.0)));
  }
  return Result;
}

UnicodeString UnixExtractFileName(UnicodeString APath)
{
  intptr_t Pos = APath.LastDelimiter(L'/');
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

UnicodeString UnixExtractFileExt(UnicodeString APath)
{
  UnicodeString FileName = base::UnixExtractFileName(APath);
  intptr_t Pos = FileName.LastDelimiter(L".");
  return (Pos > 0) ? APath.SubString(Pos, APath.Length() - Pos + 1) : UnicodeString();
}

UnicodeString ExtractFileName(UnicodeString APath, bool Unix)
{
  if (Unix)
  {
    return UnixExtractFileName(APath);
  }
  return ExtractFilename(APath, L'\\');
}

UnicodeString GetEnvVariable(UnicodeString AEnvVarName)
{
  UnicodeString Result;
  intptr_t Len = ::GetEnvironmentVariableW(AEnvVarName.c_str(), nullptr, 0);
  if (Len > 0)
  {
    wchar_t * Buffer = Result.SetLength(Len - 1);
    ::GetEnvironmentVariableW(AEnvVarName.c_str(), reinterpret_cast<LPWSTR>(Buffer), static_cast<DWORD>(Len));
  }
  return Result;
}

} // namespace base

// from RemoteFiles.cpp

namespace base {

bool IsUnixStyleWindowsPath(UnicodeString APath)
{
  return (APath.Length() >= 3) && IsLetter(APath[1]) && (APath[2] == L':') && (APath[3] == L'/');
}

bool UnixIsAbsolutePath(UnicodeString APath)
{
  return
    ((APath.Length() >= 1) && (APath[1] == L'/')) ||
    // we need this for FTP only, but this is unfortunately used in a static context
    base::IsUnixStyleWindowsPath(APath);
}

UnicodeString UnixIncludeTrailingBackslash(UnicodeString APath)
{
  // it used to return "/" when input path was empty
  if (!APath.IsEmpty() && !APath.IsDelimiter(SLASH, APath.Length()))
  {
    return APath + SLASH;
  }
  return APath;
}

// Keeps "/" for root path
UnicodeString UnixExcludeTrailingBackslash(UnicodeString APath, bool Simple)
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

UnicodeString SimpleUnixExcludeTrailingBackslash(UnicodeString APath)
{
  return base::UnixExcludeTrailingBackslash(APath, true);
}

UnicodeString UnixCombinePaths(UnicodeString APath1, UnicodeString APath2)
{
  return UnixIncludeTrailingBackslash(APath1) + APath2;
}

Boolean UnixSamePath(UnicodeString APath1, UnicodeString APath2)
{
  return (base::UnixIncludeTrailingBackslash(APath1) == base::UnixIncludeTrailingBackslash(APath2));
}

bool UnixIsChildPath(UnicodeString AParent, UnicodeString AChild)
{
  UnicodeString Parent = base::UnixIncludeTrailingBackslash(AParent);
  UnicodeString Child = base::UnixIncludeTrailingBackslash(AChild);
  return (Child.SubString(1, Parent.Length()) == Parent);
}

UnicodeString UnixExtractFileDir(UnicodeString APath)
{
  intptr_t Pos = APath.LastDelimiter(L'/');
  // it used to return Path when no slash was found
  if (Pos > 1)
  {
    return APath.SubString(1, Pos - 1);
  }
  return (Pos == 1) ? UnicodeString(L"/") : UnicodeString();
}

// must return trailing backslash
UnicodeString UnixExtractFilePath(UnicodeString APath)
{
  intptr_t Pos = APath.LastDelimiter(L'/');
  // it used to return Path when no slash was found
  if (Pos > 0)
  {
    return APath.SubString(1, Pos);
  }
  return UnicodeString();
}

#if 0

UnicodeString UnixExtractFileName(UnicodeString APath)
{
  intptr_t Pos = APath.LastDelimiter(L'/');
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

UnicodeString UnixExtractFileExt(UnicodeString APath)
{
  UnicodeString FileName = UnixExtractFileName(APath);
  intptr_t Pos = FileName.LastDelimiter(L".");
  if (Pos > 0)
    return APath.SubString(Pos, APath.Length() - Pos + 1);
  else
    return UnicodeString();
}

UnicodeString ExtractFileName(UnicodeString APath, bool Unix)
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

#endif // #if 0

bool ExtractCommonPath(const TStrings * AFiles, OUT UnicodeString & APath)
{
  DebugAssert(AFiles->GetCount() > 0);

  APath = ::ExtractFilePath(AFiles->GetString(0));
  bool Result = !APath.IsEmpty();
  if (Result)
  {
    for (intptr_t Index = 1; Index < AFiles->GetCount(); ++Index)
    {
      while (!APath.IsEmpty() &&
        (AFiles->GetString(Index).SubString(1, APath.Length()) != APath))
      {
        intptr_t PrevLen = APath.Length();
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

bool UnixExtractCommonPath(const TStrings * const AFiles, OUT UnicodeString & APath)
{
  DebugAssert(AFiles->GetCount() > 0);

  APath = base::UnixExtractFilePath(AFiles->GetString(0));
  bool Result = !APath.IsEmpty();
  if (Result)
  {
    for (intptr_t Index = 1; Index < AFiles->GetCount(); ++Index)
    {
      while (!APath.IsEmpty() &&
        (AFiles->GetString(Index).SubString(1, APath.Length()) != APath))
      {
        intptr_t PrevLen = APath.Length();
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

bool IsUnixRootPath(UnicodeString APath)
{
  return APath.IsEmpty() || (APath == ROOTDIRECTORY);
}

bool IsUnixHiddenFile(UnicodeString APath)
{
  return (APath != THISDIRECTORY) && (APath != PARENTDIRECTORY) &&
    !APath.IsEmpty() && (APath[1] == L'.');
}

UnicodeString AbsolutePath(UnicodeString Base, UnicodeString APath)
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
    intptr_t P;
    while ((P = Result.Pos(L"/../")) > 0)
    {
      // special case, "/../" => "/"
      if (P == 1)
      {
        Result = ROOTDIRECTORY;
      }
      else
      {
        intptr_t P2 = Result.SubString(1, P - 1).LastDelimiter(L"/");
        DebugAssert(P2 > 0);
        Result.Delete(P2, P - P2 + 3);
      }
    }
    while ((P = Result.Pos(L"/./")) > 0)
    {
      Result.Delete(P, 2);
    }
    Result = base::UnixExcludeTrailingBackslash(Result);
  }
  return Result;
}

UnicodeString FromUnixPath(UnicodeString APath)
{
  return ReplaceStr(APath, SLASH, BACKSLASH);
}

UnicodeString ToUnixPath(UnicodeString APath)
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
    intptr_t P;
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
    P = S.Pos(Sep[1]);
    if (P)
    {
      S.Delete(1, P);
      S = L"..." + Sep + S;
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

UnicodeString MinimizeName(UnicodeString AFileName, intptr_t MaxLen, bool Unix)
{
  UnicodeString Drive, Dir, Name;
  UnicodeString Sep = Unix ? SLASH : BACKSLASH;

  UnicodeString Result = AFileName;
  if (Unix)
  {
    intptr_t P = Result.LastDelimiter(SLASH);
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
    if (Dir == Sep + L"..." + Sep)
    {
      Dir = L"..." + Sep;
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
  for (intptr_t Index = 0; Index < AFileList->GetCount(); ++Index)
  {
    UnicodeString FileName = AFileList->GetString(Index);
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
    return FORMAT(L"%3s %2d %2d", EngShortMonthNames[Month-1], Day, Year);
  case mfMDHM:
    return FORMAT(L"%3s %2d %2d:%2.2d",
      EngShortMonthNames[Month-1], Day, Hour, Min);
  case mfFull:
    return FORMAT(L"%3s %2d %2d:%2.2d:%2.2d %4d",
      EngShortMonthNames[Month-1], Day, Hour, Min, Sec, Year);
  default:
    DebugAssert(false);
  }
  return UnicodeString();
}

UnicodeString ModificationStr(const TDateTime & DateTime,
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
    return FORMAT(L"%3s %2d %2d", EngShortMonthNames[Month-1], Day, Year);

  case mfMDHM:
    return FORMAT(L"%3s %2d %2d:%2.2d",
      EngShortMonthNames[Month-1], Day, Hour, Min);

  default:
    DebugFail();
    // fall thru

  case mfFull:
    return FORMAT(L"%3s %2d %2d:%2.2d:%2.2d %4d",
      EngShortMonthNames[Month-1], Day, Hour, Min, Sec, Year);
  }
}

int FakeFileImageIndex(UnicodeString /*AFileName*/, uint32_t /*Attrs*/,
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
  if (::SameText(base::UnixExtractFileExt(FileName), PARTIAL_EXT))
  {
    static const size_t PartialExtLen = _countof(PARTIAL_EXT) - 1;
    FileName.SetLength(FileName.Length() - PartialExtLen);
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

bool SameUserName(UnicodeString UserName1, UnicodeString UserName2)
{
  // Bitvise reports file owner as "user@host", but we login with "user" only.
  UnicodeString AUserName1 = CopyToChar(UserName1, L'@', true);
  UnicodeString AUserName2 = CopyToChar(UserName2, L'@', true);
  return ::SameText(AUserName1, AUserName2);
}

UnicodeString FormatMultiFilesToOneConfirmation(UnicodeString ATarget, bool Unix)
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

} // namespace base

