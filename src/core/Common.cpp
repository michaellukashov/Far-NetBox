//---------------------------------------------------------------------------
#define NO_WIN32_LEAN_AND_MEAN
#include <vcl.h>
#pragma hdrstop

#include "Common.h"
#include "Exceptions.h"
#include "TextsCore.h"
#include "Interface.h"
#include <StrUtils.hpp>
#include <DateUtils.hpp>
#include <assert.h>
#include <math.h>
#include <shlobj.h>
#include <limits>
//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------

#if defined(__MINGW32__)
typedef struct _TIME_DYNAMIC_ZONE_INFORMATION {
  LONG       Bias;
  WCHAR      StandardName[32];
  SYSTEMTIME StandardDate;
  LONG       StandardBias;
  WCHAR      DaylightName[32];
  SYSTEMTIME DaylightDate;
  LONG       DaylightBias;
  WCHAR      TimeZoneKeyName[128];
  BOOLEAN    DynamicDaylightTimeDisabled;
} DYNAMIC_TIME_ZONE_INFORMATION, *PDYNAMIC_TIME_ZONE_INFORMATION;
#endif
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// TGuard
//---------------------------------------------------------------------------
TGuard::TGuard(TCriticalSection * ACriticalSection) :
  FCriticalSection(ACriticalSection)
{
  assert(ACriticalSection != nullptr);
  FCriticalSection->Enter();
}
//---------------------------------------------------------------------------
TGuard::~TGuard()
{
  FCriticalSection->Leave();
}
//---------------------------------------------------------------------------
// TUnguard
//---------------------------------------------------------------------------
TUnguard::TUnguard(TCriticalSection * ACriticalSection) :
  FCriticalSection(ACriticalSection)
{
  assert(ACriticalSection != nullptr);
  FCriticalSection->Leave();
}
//---------------------------------------------------------------------------
TUnguard::~TUnguard()
{
  FCriticalSection->Enter();
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
const wchar_t EngShortMonthNames[12][4] =
  {L"Jan", L"Feb", L"Mar", L"Apr", L"May", L"Jun",
   L"Jul", L"Aug", L"Sep", L"Oct", L"Nov", L"Dec"};
const std::string Bom = "\xEF\xBB\xBF";
const wchar_t TokenPrefix = L'%';
const wchar_t NoReplacement = wchar_t(0);
const wchar_t TokenReplacement = wchar_t(1);
const UnicodeString LocalInvalidChars = L"/\\:*?\"<>|";
//---------------------------------------------------------------------------
UnicodeString ReplaceChar(const UnicodeString & Str, wchar_t A, wchar_t B)
{
  UnicodeString Result = Str;
  for (wchar_t * Ch = const_cast<wchar_t *>(Result.c_str()); Ch && *Ch; ++Ch)
    if (*Ch == A)
    {
      *Ch = B;
    }
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString DeleteChar(const UnicodeString & Str, wchar_t C)
{
  UnicodeString Result = Str;
  intptr_t P;
  while ((P = Result.Pos(C)) > 0)
  {
    Result.Delete(P, 1);
  }
  return Result;
}
//---------------------------------------------------------------------------
void PackStr(UnicodeString & Str)
{
  // Following will free unnecessary bytes
  Str = Str.c_str();
}
//---------------------------------------------------------------------------
void PackStr(RawByteString & Str)
{
  // Following will free unnecessary bytes
  Str = Str.c_str();
}
//---------------------------------------------------------------------------
void Shred(UnicodeString & Str)
{
  if (!Str.IsEmpty())
  {
    wmemset(const_cast<wchar_t *>(Str.c_str()), 0, Str.Length());
    Str = L"";
  }
}
//---------------------------------------------------------------------------
UnicodeString MakeValidFileName(const UnicodeString & FileName)
{
  UnicodeString Result = FileName;
  static UnicodeString IllegalChars = L":;,=+<>|\"[] \\/?*";
  for (intptr_t Index = 0; Index < IllegalChars.Length(); ++Index)
  {
    Result = ReplaceChar(Result, IllegalChars[Index+1], L'-');
  }
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString RootKeyToStr(HKEY RootKey)
{
  if (RootKey == HKEY_USERS) return L"HKEY_USERS";
    else
  if (RootKey == HKEY_LOCAL_MACHINE) return L"HKEY_LOCAL_MACHINE";
    else
  if (RootKey == HKEY_CURRENT_USER) return L"HKEY_CURRENT_USER";
    else
  if (RootKey == HKEY_CLASSES_ROOT) return L"HKEY_CLASSES_ROOT";
    else
  if (RootKey == HKEY_CURRENT_CONFIG) return L"HKEY_CURRENT_CONFIG";
    else
  if (RootKey == HKEY_DYN_DATA) return L"HKEY_DYN_DATA";
    else
  {  Abort(); return L""; }
}
//---------------------------------------------------------------------------
UnicodeString BooleanToEngStr(bool B)
{
  if (B)
  {
    return L"Yes";
  }
  else
  {
    return L"No";
  }
}
//---------------------------------------------------------------------------
UnicodeString BooleanToStr(bool B)
{
  if (B)
  {
    return LoadStr(YES_STR);
  }
  else
  {
    return LoadStr(NO_STR);
  }
}
//---------------------------------------------------------------------------
UnicodeString DefaultStr(const UnicodeString & Str, const UnicodeString & Default)
{
  if (!Str.IsEmpty())
  {
    return Str;
  }
  else
  {
    return Default;
  }
}
//---------------------------------------------------------------------------
UnicodeString CutToChar(UnicodeString & Str, wchar_t Ch, bool Trim)
{
  intptr_t P = Str.Pos(Ch);
  UnicodeString Result;
  if (P)
  {
    Result = Str.SubString(1, P-1);
    Str.Delete(1, P);
  }
  else
  {
    Result = Str;
    Str = L"";
  }
  if (Trim)
  {
    Result = Result.TrimRight();
    Str = Str.TrimLeft();
  }
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString CopyToChars(const UnicodeString & Str, intptr_t & From,
  const UnicodeString & Chs, bool Trim,
  wchar_t * Delimiter, bool DoubleDelimiterEscapes)
{
  UnicodeString Result;

  intptr_t P;
  for (P = From; P <= Str.Length(); P++)
  {
    if (IsDelimiter(Chs, Str, P))
    {
      if (DoubleDelimiterEscapes &&
          (P < Str.Length()) &&
          IsDelimiter(Chs, Str, P + 1))
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
  From = P+1;
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
//---------------------------------------------------------------------------
UnicodeString DelimitStr(const UnicodeString & Str, const UnicodeString & Chars)
{
  UnicodeString Result = Str;
  for (intptr_t I = 1; I <= Result.Length(); I++)
  {
    if (Result.IsDelimiter(Chars, I))
    {
      Result.Insert(L"\\", I);
      I++;
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString ShellDelimitStr(const UnicodeString & Str, wchar_t Quote)
{
  static UnicodeString Chars = L"$\\";
  if (Quote == L'"')
  {
    Chars += L"`\"";
  }
  return DelimitStr(Str, Chars);
}
//---------------------------------------------------------------------------
UnicodeString ExceptionLogString(Exception *E)
{
  assert(E);
  if (NB_STATIC_DOWNCAST(Exception, E) != nullptr)
  {
    UnicodeString Msg;
#if defined(__BORLANDC__)
    Msg = FORMAT(L"(%s) %s", (E->ClassName(), E->Message.c_str()));
#else
    Msg = FORMAT(L"%s", UnicodeString(E->what()).c_str());
#endif
    if (NB_STATIC_DOWNCAST(ExtException, E) != nullptr)
    {
      TStrings * MoreMessages = NB_STATIC_DOWNCAST(ExtException, E)->GetMoreMessages();
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
    ExceptionErrorMessage(ExceptObject(), ExceptAddr(), Buffer, LENOF(Buffer));
    return UnicodeString(Buffer);
#else
    return UnicodeString(E->what());
#endif
  }
}
//---------------------------------------------------------------------------
UnicodeString MainInstructions(const UnicodeString & S)
{
  UnicodeString MainMsgTag = LoadStr(MAIN_MSG_TAG);
  return MainMsgTag + S + MainMsgTag;
}
//---------------------------------------------------------------------------
UnicodeString MainInstructionsFirstParagraph(const UnicodeString & S)
{
  // WORKAROUND, we consider it bad practice, the highlighting should better
  // be localized (but maybe we change our mind later)
  UnicodeString Result;
  intptr_t Pos = S.Pos(L"\n\n");
  // we would not be calling this on single paragraph message
  if (ALWAYS_TRUE(Pos > 0))
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
//---------------------------------------------------------------------------
bool ExtractMainInstructions(UnicodeString & S, UnicodeString & MainInstructions)
{
  bool Result = false;
  UnicodeString MainMsgTag = LoadStr(MAIN_MSG_TAG);
  if (StartsStr(MainMsgTag, S))
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

  assert(MainInstructions.Pos(MainMsgTag) == 0);
  assert(S.Pos(MainMsgTag) == 0);

  return Result;
}
//---------------------------------------------------------------------------
UnicodeString UnformatMessage(const UnicodeString & S)
{
  UnicodeString Result = S;
  UnicodeString MainInstruction;
  if (ExtractMainInstructions(Result, MainInstruction))
  {
    Result = MainInstruction + Result;
  }
  return Result;
}
//---------------------------------------------------------------------------
bool IsNumber(const UnicodeString & Str)
{
  intptr_t Value = 0;
  if (Str == L"0") return true;
  return TryStrToInt(Str, Value);
}
//---------------------------------------------------------------------------
UnicodeString SystemTemporaryDirectory()
{
  UnicodeString TempDir;
  TempDir.SetLength(MAX_PATH);
  TempDir.SetLength(GetTempPath(MAX_PATH, const_cast<LPWSTR>(TempDir.c_str())));
  return TempDir;
}
//---------------------------------------------------------------------------
UnicodeString GetShellFolderPath(int CSIdl)
{
  UnicodeString Result;
  wchar_t Path[2 * MAX_PATH + 10] = L"\0";
  if (SUCCEEDED(SHGetFolderPath(nullptr, CSIdl, nullptr, SHGFP_TYPE_CURRENT, Path)))
  {
    Result = Path;
  }
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString StripPathQuotes(const UnicodeString & Path)
{
  if ((Path.Length() >= 2) &&
      (Path[1] == L'\"') && (Path[Path.Length()] == L'\"'))
  {
    return Path.SubString(2, Path.Length() - 2);
  }
  else
  {
    return Path;
  }
}
//---------------------------------------------------------------------------
UnicodeString AddPathQuotes(const UnicodeString & Path)
{
  UnicodeString Result = StripPathQuotes(Path);
  if (Result.Pos(L" ") > 0)
  {
    Result = L"\"" + Result + L"\"";
  }
  return Result;
}
//---------------------------------------------------------------------------
static wchar_t * ReplaceChar(
  UnicodeString & FileName, wchar_t * InvalidChar, wchar_t InvalidCharsReplacement)
{
  intptr_t Index = InvalidChar - FileName.c_str() + 1;
  if (InvalidCharsReplacement == TokenReplacement)
  {
    // currently we do not support unicode chars replacement
    if (FileName[Index] > 0xFF)
    {
      ThrowExtException();
    }

    FileName.Insert(ByteToHex(static_cast<uint8_t>(FileName[Index])), Index + 1);
    FileName[Index] = TokenPrefix;
    InvalidChar = const_cast<wchar_t *>(FileName.c_str() + Index + 2);
  }
  else
  {
    FileName[Index] = InvalidCharsReplacement;
    InvalidChar = const_cast<wchar_t *>(FileName.c_str() + Index);
  }
  return InvalidChar;
}
//---------------------------------------------------------------------------
UnicodeString ValidLocalFileName(const UnicodeString & FileName)
{
  return ValidLocalFileName(FileName, L'_', L"", LocalInvalidChars);
}
//---------------------------------------------------------------------------
UnicodeString ValidLocalFileName(
  const UnicodeString & FileName, wchar_t InvalidCharsReplacement,
  const UnicodeString & TokenizibleChars, const UnicodeString & LocalInvalidChars)
{
  UnicodeString FileName2 = FileName;
  if (InvalidCharsReplacement != NoReplacement)
  {
    bool ATokenReplacement = (InvalidCharsReplacement == TokenReplacement);
    UnicodeString CharsStr = ATokenReplacement ? TokenizibleChars : LocalInvalidChars;
    const wchar_t * Chars = CharsStr.c_str();
    wchar_t * InvalidChar = const_cast<wchar_t *>(FileName2.c_str());
    while ((InvalidChar = wcspbrk(InvalidChar, Chars)) != nullptr)
    {
      intptr_t Pos = (InvalidChar - FileName2.c_str() + 1);
      wchar_t Char;
      if (ATokenReplacement &&
          (*InvalidChar == TokenPrefix) &&
          (((FileName2.Length() - Pos) <= 1) ||
           (((Char = static_cast<wchar_t>(HexToByte(FileName2.SubString(Pos + 1, 2)))) == L'\0') ||
            (TokenizibleChars.Pos(Char) == 0))))
      {
        InvalidChar++;
      }
      else
      {
        InvalidChar = ReplaceChar(FileName2, InvalidChar, InvalidCharsReplacement);
      }
    }

    // Windows trim trailing space or dot, hence we must encode it to preserve it
    if (!FileName2.IsEmpty() &&
        ((FileName2[FileName2.Length()] == L' ') ||
         (FileName2[FileName2.Length()] == L'.')))
    {
      ReplaceChar(FileName2, const_cast<wchar_t *>(FileName2.c_str() + FileName2.Length() - 1), InvalidCharsReplacement);
    }

    if (IsReservedName(FileName2))
    {
      intptr_t P = FileName2.Pos(".");
      if (P == 0)
      {
        P = FileName2.Length() + 1;
      }
      FileName2.Insert(L"%00", P);
    }
  }
  return FileName2;
}
//---------------------------------------------------------------------------
void SplitCommand(const UnicodeString & Command, UnicodeString & Program,
  UnicodeString & Params, UnicodeString & Dir)
{
  UnicodeString Cmd = Command.Trim();
  Params = L"";
  Dir = L"";
  if (!Cmd.IsEmpty() && (Cmd[1] == L'\"'))
  {
    Cmd.Delete(1, 1);
    intptr_t P = Cmd.Pos(L'"');
    if (P)
    {
      Program = Cmd.SubString(1, P-1).Trim();
      Params = Cmd.SubString(P + 1, Cmd.Length() - P).Trim();
    }
    else
    {
      throw Exception(FMTLOAD(INVALID_SHELL_COMMAND, UnicodeString(L"\"" + Cmd).c_str()));
    }
  }
  else
  {
    intptr_t P = Cmd.Pos(L" ");
    if (P)
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
  if (B)
  {
    Dir = Program.SubString(1, B).Trim();
  }
}
//---------------------------------------------------------------------------
UnicodeString ExtractProgram(const UnicodeString & Command)
{
  UnicodeString Program;
  UnicodeString Params;
  UnicodeString Dir;

  SplitCommand(Command, Program, Params, Dir);

  return Program;
}
//---------------------------------------------------------------------------
UnicodeString ExtractProgramName(const UnicodeString & Command)
{
  UnicodeString Name = ::ExtractFileName(ExtractProgram(Command), false);
  intptr_t Dot = Name.LastDelimiter(L".");
  if (Dot > 0)
  {
    Name = Name.SubString(1, Dot - 1);
  }
  return Name;
}
//---------------------------------------------------------------------------
UnicodeString FormatCommand(const UnicodeString & Program, const UnicodeString & Params)
{
  UnicodeString Result = Program.Trim();
  UnicodeString Params2 = Params.Trim();
  if (!Params2.IsEmpty()) Params2 = L" " + Params2;
  if (Result.Pos(L" ")) Result = L"\"" + Result + L"\"";
  return Result + Params2;
}
//---------------------------------------------------------------------------
const wchar_t ShellCommandFileNamePattern[] = L"!.!";
//---------------------------------------------------------------------------
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
//---------------------------------------------------------------------------
UnicodeString ExpandFileNameCommand(const UnicodeString & Command,
  const UnicodeString & FileName)
{
  return AnsiReplaceStr(Command, ShellCommandFileNamePattern,
    AddPathQuotes(FileName));
}
//---------------------------------------------------------------------------
UnicodeString EscapePuttyCommandParam(const UnicodeString & Param)
{
  UnicodeString Result = Param;
  bool Space = false;

  for (intptr_t I = 1; I <= Result.Length(); I++)
  {
    switch (Result[I])
    {
      case L'"':
        Result.Insert(L"\\", I);
        I++;
        break;

      case L' ':
        Space = true;
        break;

      case L'\\':
        intptr_t I2 = I;
        while ((I2 <= Result.Length()) && (Result[I2] == L'\\'))
        {
          I2++;
        }
        if ((I2 <= Result.Length()) && (Result[I2] == L'"'))
        {
          while (Result[I] == L'\\')
          {
            Result.Insert(L"\\", I);
            I += 2;
          }
          I--;
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
//---------------------------------------------------------------------------
UnicodeString ExpandEnvironmentVariables(const UnicodeString & Str)
{
  UnicodeString Buf;
  intptr_t Size = 1024;

  Buf.SetLength(Size);
  intptr_t Len = ExpandEnvironmentStrings(Str.c_str(), const_cast<LPWSTR>(Buf.c_str()), (DWORD)Size);

  if (Len > Size)
  {
    Buf.SetLength(Len);
    ExpandEnvironmentStrings(Str.c_str(), const_cast<LPWSTR>(Buf.c_str()), static_cast<DWORD>(Len));
  }

  PackStr(Buf);

  return Buf;
}
//---------------------------------------------------------------------------
bool CompareFileName(const UnicodeString & Path1, const UnicodeString & Path2)
{
  UnicodeString ShortPath1 = ExtractShortPathName(Path1);
  UnicodeString ShortPath2 = ExtractShortPathName(Path2);

  bool Result;
  // ExtractShortPathName returns empty string if file does not exist
  if (ShortPath1.IsEmpty() || ShortPath2.IsEmpty())
  {
    Result = AnsiSameText(Path1, Path2);
  }
  else
  {
    Result = AnsiSameText(ShortPath1, ShortPath2);
  }
  return Result;
}
//---------------------------------------------------------------------------
bool ComparePaths(const UnicodeString & Path1, const UnicodeString & Path2)
{
  // TODO: ExpandUNCFileName
  return AnsiSameText(IncludeTrailingBackslash(Path1), IncludeTrailingBackslash(Path2));
}
//---------------------------------------------------------------------------
bool IsReservedName(const UnicodeString & FileName)
{
  UnicodeString fileName = FileName;
  intptr_t P = fileName.Pos(L".");
  intptr_t Len = (P > 0) ? P - 1 : fileName.Length();
  if ((Len == 3) || (Len == 4))
  {
    if (P > 0)
    {
      fileName.SetLength(P - 1);
    }
    static UnicodeString Reserved[] = {
      L"CON", L"PRN", L"AUX", L"NUL",
      L"COM1", L"COM2", L"COM3", L"COM4", L"COM5", L"COM6", L"COM7", L"COM8", L"COM9",
      L"LPT1", L"LPT2", L"LPT3", L"LPT4", L"LPT5", L"LPT6", L"LPT7", L"LPT8", L"LPT9" };
    for (intptr_t Index = 0; Index < static_cast<intptr_t>(LENOF(Reserved)); ++Index)
    {
      if (SameText(fileName, Reserved[Index]))
      {
        return true;
      }
    }
  }
  return false;
}
//---------------------------------------------------------------------------
UnicodeString DisplayableStr(const RawByteString & Str)
{
  bool Displayable = true;
  intptr_t Index = 1;
  while ((Index <= Str.Length()) && Displayable)
  {
    if (((Str[Index] < '\x20') || (static_cast<uint8_t>(Str[Index]) >= static_cast<uint8_t >('\x80'))) &&
        (Str[Index] != '\n') && (Str[Index] != '\r') && (Str[Index] != '\t') && (Str[Index] != '\b'))
    {
      Displayable = false;
    }
    ++Index;
  }

  UnicodeString Result;
  if (Displayable)
  {
    Result = L"\"";
    for (intptr_t Index = 1; Index <= Str.Length(); ++Index)
    {
      switch (Str[Index])
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
          Result += wchar_t(Str[Index]);
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
//---------------------------------------------------------------------------
UnicodeString ByteToHex(uint8_t B, bool UpperCase)
{
  static wchar_t UpperDigits[] = L"0123456789ABCDEF";
  static wchar_t LowerDigits[] = L"0123456789abcdef";

  const wchar_t * Digits = (UpperCase ? UpperDigits : LowerDigits);
  UnicodeString Result;
  Result.SetLength(2);
  Result[1] = Digits[(B & 0xF0) >> 4];
  Result[2] = Digits[(B & 0x0F) >> 0];
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString BytesToHex(const uint8_t * B, uintptr_t Length, bool UpperCase, wchar_t Separator)
{
  UnicodeString Result;
  for (uintptr_t i = 0; i < Length; i++)
  {
    Result += ByteToHex(B[i], UpperCase);
    if ((Separator != L'\0') && (i < Length - 1))
    {
      Result += Separator;
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString BytesToHex(const RawByteString & Str, bool UpperCase, wchar_t Separator)
{
  return BytesToHex(reinterpret_cast<const uint8_t *>(Str.c_str()), Str.Length(), UpperCase, Separator);
}
//---------------------------------------------------------------------------
UnicodeString CharToHex(wchar_t Ch, bool UpperCase)
{
  return BytesToHex(reinterpret_cast<const uint8_t *>(&Ch), sizeof(Ch), UpperCase);
}
//---------------------------------------------------------------------------
RawByteString HexToBytes(const UnicodeString & Hex)
{
  static UnicodeString Digits = L"0123456789ABCDEF";
  RawByteString Result;
  intptr_t L = Hex.Length();
  if (L % 2 == 0)
  {
    for (intptr_t I = 1; I <= Hex.Length(); I += 2)
    {
      intptr_t P1 = Digits.Pos(UpCase(Hex[I]));
      intptr_t P2 = Digits.Pos(UpCase(Hex[I + 1]));
      if (P1 <= 0 || P2 <= 0)
      {
        Result = L"";
        break;
      }
      else
      {
        Result += static_cast<char>((P1 - 1) * 16 + P2 - 1);
      }
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
uint8_t HexToByte(const UnicodeString & Hex)
{
  static UnicodeString Digits = L"0123456789ABCDEF";
  assert(Hex.Length() == 2);
  intptr_t P1 = Digits.Pos(UpCase(Hex[1]));
  intptr_t P2 = Digits.Pos(UpCase(Hex[2]));

  return
    static_cast<uint8_t>(((P1 <= 0) || (P2 <= 0)) ? 0 : (((P1 - 1) << 4) + (P2 - 1)));
}
//---------------------------------------------------------------------------
bool IsLowerCaseLetter(wchar_t Ch)
{
  return (Ch >= 'a') && (Ch <= 'z');
}
//---------------------------------------------------------------------------
bool IsUpperCaseLetter(wchar_t Ch)
{
  return (Ch >= 'A') && (Ch <= 'Z');
}
//---------------------------------------------------------------------------
bool IsLetter(wchar_t Ch)
{
  return IsLowerCaseLetter(Ch) || IsUpperCaseLetter(Ch);
}
//---------------------------------------------------------------------------
bool IsDigit(wchar_t Ch)
{
  return (Ch >= '0') && (Ch <= '9');
}
//---------------------------------------------------------------------------
bool IsHex(wchar_t Ch)
{
  return
    IsDigit(Ch) ||
    ((Ch >= 'A') && (Ch <= 'F')) ||
    ((Ch >= 'a') && (Ch <= 'f'));
}
//---------------------------------------------------------------------------
DWORD FindCheck(DWORD Result)
{
  if ((Result != ERROR_SUCCESS) &&
      (Result != ERROR_FILE_NOT_FOUND) &&
      (Result != ERROR_NO_MORE_FILES))
  {
    RaiseLastOSError(Result);
  }
  return Result;
}
//---------------------------------------------------------------------------
DWORD FindFirstChecked(const UnicodeString & Path, DWORD LocalFileAttrs, TSearchRec & F)
{
  return FindCheck(FindFirst(Path, LocalFileAttrs, F));
}
//---------------------------------------------------------------------------
// It can make sense to use FindNextChecked, even if unchecked FindFirst is used.
// I.e. even if we do not care that FindFirst failed, if FindNext
// fails after successful FindFirst, it mean some terrible problem
DWORD FindNextChecked(TSearchRec & F)
{
  return FindCheck(FindNext(F));
}
//---------------------------------------------------------------------------
bool FileSearchRec(const UnicodeString & FileName, TSearchRec & Rec)
{
  DWORD FindAttrs = faReadOnly | faHidden | faSysFile | faDirectory | faArchive;
  bool Result = (FindFirst(FileName, FindAttrs, Rec) == 0);
  if (Result)
  {
    FindClose(Rec);
  }
  return Result;
}
//---------------------------------------------------------------------------
void ProcessLocalDirectory(const UnicodeString & DirName,
  TProcessLocalFileEvent CallBackFunc, void * Param,
  DWORD FindAttrs)
{
  assert(CallBackFunc);
  if (FindAttrs == INVALID_FILE_ATTRIBUTES)
  {
    FindAttrs = faReadOnly | faHidden | faSysFile | faDirectory | faArchive;
  }
  TSearchRec SearchRec;

  UnicodeString DirName2 = IncludeTrailingBackslash(DirName);
  if (FindFirstChecked(DirName2 + L"*.*", FindAttrs, SearchRec) == 0)
  {
    SCOPE_EXIT
    {
      FindClose(SearchRec);
    };
    do
    {
      if ((SearchRec.Name != L".") && (SearchRec.Name != L".."))
      {
        UnicodeString FileName = DirName2 + SearchRec.Name;
        CallBackFunc(FileName, SearchRec, Param);
      }

    } while (FindNextChecked(SearchRec) == 0);
  }
}
//---------------------------------------------------------------------------
TDateTime EncodeDateVerbose(Word Year, Word Month, Word Day)
{
  try
  {
    TDateTime DateTime = EncodeDate(Year, Month, Day);
    return DateTime;
  }
  catch (EConvertError & E)
  {
    throw EConvertError(FORMAT(L"%s [%04u-%02u-%02u]", E.Message.c_str(), int(Year), int(Month), int(Day)));
  }
  return TDateTime();
}
//---------------------------------------------------------------------------
TDateTime EncodeTimeVerbose(Word Hour, Word Min, Word Sec, Word MSec)
{
  try
  {
    TDateTime DateTime = EncodeTime(Hour, Min, Sec, MSec);
    return DateTime;
  }
  catch (EConvertError & E)
  {
    throw EConvertError(FORMAT(L"%s [%02u:%02u:%02u.%04u]", E.Message.c_str(), int(Hour), int(Min), int(Sec), int(MSec)));
  }
  return TDateTime();
}
//---------------------------------------------------------------------------
TDateTime SystemTimeToDateTimeVerbose(const SYSTEMTIME & SystemTime)
{
  try
  {
    TDateTime DateTime = SystemTimeToDateTime(SystemTime);
    return DateTime;
  }
  catch (EConvertError & E)
  {
    throw EConvertError(FORMAT(L"%s [%d-%2.2d-%2.2d %2.2d:%2.2d:%2.2d.%3.3d]", E.Message.c_str(), int(SystemTime.wYear), int(SystemTime.wMonth), int(SystemTime.wDay), int(SystemTime.wHour), int(SystemTime.wMinute), int(SystemTime.wSecond), int(SystemTime.wMilliseconds)));
  }
}
//---------------------------------------------------------------------------
struct TDateTimeParams : public TObject
{
  TDateTime UnixEpoch;
  double BaseDifference;
  long BaseDifferenceSec;
  // All Current* are actually global, not per-year
  double CurrentDaylightDifference;
  long CurrentDaylightDifferenceSec;
  double CurrentDifference;
  long CurrentDifferenceSec;
  double StandardDifference;
  long StandardDifferenceSec;
  double DaylightDifference;
  long DaylightDifferenceSec;
  SYSTEMTIME SystemStandardDate;
  SYSTEMTIME SystemDaylightDate;
  TDateTime StandardDate;
  TDateTime DaylightDate;
  bool SummerDST;
  // This is actually global, not per-year
  bool DaylightHack;
};
typedef rde::map<int, TDateTimeParams> TYearlyDateTimeParams;
static TYearlyDateTimeParams YearlyDateTimeParams;
static std::unique_ptr<TCriticalSection> DateTimeParamsSection(new TCriticalSection());
static void EncodeDSTMargin(const SYSTEMTIME & Date, unsigned short Year,
  TDateTime & Result);
//---------------------------------------------------------------------------
static unsigned short DecodeYear(const TDateTime & DateTime)
{
  unsigned short Year, Month, Day;
  DecodeDate(DateTime, Year, Month, Day);
  return Year;
}
//---------------------------------------------------------------------------
static const TDateTimeParams * GetDateTimeParams(unsigned short Year)
{
  TGuard Guard(DateTimeParamsSection.get());

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

    unsigned long GTZI;

    HINSTANCE Kernel32 = GetModuleHandle(kernel32);
    typedef BOOL (WINAPI * TGetTimeZoneInformationForYear)(USHORT wYear, PDYNAMIC_TIME_ZONE_INFORMATION pdtzi, LPTIME_ZONE_INFORMATION ptzi);
    TGetTimeZoneInformationForYear GetTimeZoneInformationForYear =
      (TGetTimeZoneInformationForYear)GetProcAddress(Kernel32, "GetTimeZoneInformationForYear");

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
    Result->BaseDifference = (double)(TZI.Bias) / MinsPerDay;
    Result->BaseDifferenceSec *= SecsPerMin;

    Result->CurrentDifferenceSec = TZI.Bias +
      Result->CurrentDaylightDifferenceSec;
    Result->CurrentDifference =
      (double)(Result->CurrentDifferenceSec) / MinsPerDay;
    Result->CurrentDifferenceSec *= SecsPerMin;

    Result->CurrentDaylightDifference =
      (double)(Result->CurrentDaylightDifferenceSec) / MinsPerDay;
    Result->CurrentDaylightDifferenceSec *= SecsPerMin;

    Result->DaylightDifferenceSec = TZI.DaylightBias * SecsPerMin;
    Result->DaylightDifference = (double)(TZI.DaylightBias) / MinsPerDay;
    Result->StandardDifferenceSec = TZI.StandardBias * SecsPerMin;
    Result->StandardDifference = (double)(TZI.StandardBias) / MinsPerDay;

    Result->SystemStandardDate = TZI.StandardDate;
    Result->SystemDaylightDate = TZI.DaylightDate;

    unsigned short AYear = (Year != 0) ? Year : DecodeYear(Now());
    if (Result->SystemStandardDate.wMonth != 0)
    {
      EncodeDSTMargin(Result->SystemStandardDate, AYear, Result->StandardDate);
    }
    if (Result->SystemDaylightDate.wMonth != 0)
    {
      EncodeDSTMargin(Result->SystemDaylightDate, AYear, Result->DaylightDate);
    }
    Result->SummerDST = (Result->DaylightDate < Result->StandardDate);

    Result->DaylightHack = !IsWin7();
  }

  return Result;
}
//---------------------------------------------------------------------------
static void EncodeDSTMargin(const SYSTEMTIME & Date, unsigned short Year,
  TDateTime & Result)
{
  if (Date.wYear == 0)
  {
    TDateTime Temp = EncodeDateVerbose(Year, Date.wMonth, 1);
    Result = Temp + ((Date.wDayOfWeek - DayOfWeek(Temp) + 8) % 7) +
      (7 * (Date.wDay - 1));
    if (Date.wDay == 5)
    {
      unsigned short Month = static_cast<unsigned short>(Date.wMonth + 1);
      if (Month > 12)
      {
        Month = static_cast<unsigned short>(Month - 12);
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
//---------------------------------------------------------------------------
static bool IsDateInDST(const TDateTime & DateTime)
{

  const TDateTimeParams * Params = GetDateTimeParams(DecodeYear(DateTime));

  bool Result;

  // On some systems it occurs that StandardDate is unset, while
  // DaylightDate is set. MSDN states that this is invalid and
  // should be treated as if there is no daylight saving.
  // So check both.
  if ((Params->SystemStandardDate.wMonth == 0) ||
      (Params->SystemDaylightDate.wMonth == 0))
  {
    Result = false;
  }
  else
  {
    if (Params->SummerDST)
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
//---------------------------------------------------------------------------
bool UsesDaylightHack()
{
  return GetDateTimeParams(0)->DaylightHack;
}
//---------------------------------------------------------------------------
TDateTime UnixToDateTime(__int64 TimeStamp, TDSTMode DSTMode)
{
  assert(int(EncodeDateVerbose(1970, 1, 1)) == UnixDateDelta);

  TDateTime Result = TDateTime(UnixDateDelta + ((double)(TimeStamp) / SecsPerDay));
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
    Result -= (IsDateInDST(Result) ?
      Params->DaylightDifference : Params->StandardDifference);
  }
  return Result;
}
//---------------------------------------------------------------------------
__int64 Round(double Number)
{
  double Floor = floor(Number);
  double Ceil = ceil(Number);
  return static_cast<__int64>(((Number - Floor) > (Ceil - Number)) ? Ceil : Floor);
}
//---------------------------------------------------------------------------
bool TryRelativeStrToDateTime(const UnicodeString & Str, TDateTime & DateTime)
{
  UnicodeString S = Str.Trim();
  intptr_t Index = 1;
  while ((Index <= S.Length()) && IsDigit(S[Index]))
  {
    ++Index;
  }
  UnicodeString NumberStr = S.SubString(1, Index - 1);
  intptr_t Number = 0;
  bool Result = TryStrToInt(NumberStr, Number);
  if (Result)
  {
    S.Delete(1, Index - 1);
    S = S.Trim().UpperCase();
    DateTime = Now();
    // These may not overlap with ParseSize (K, M and G)
    if (S == "S")
    {
      DateTime = IncSecond(DateTime, -Number);
    }
    else if (S == "N")
    {
      DateTime = IncMinute(DateTime, -Number);
    }
    else if (S == "H")
    {
      DateTime = IncHour(DateTime, -Number);
    }
    else if (S == "D")
    {
      DateTime = IncDay(DateTime, -Number);
    }
    else if (S == "Y")
    {
      DateTime = IncYear(DateTime, -Number);
    }
    else
    {
      Result = false;
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
static __int64 DateTimeToUnix(const TDateTime & DateTime)
{
  const TDateTimeParams * CurrentParams = GetDateTimeParams(0);

  assert(int(EncodeDateVerbose(1970, 1, 1)) == UnixDateDelta);

  return Round((double)(DateTime - UnixDateDelta) * SecsPerDay) +
    CurrentParams->CurrentDifferenceSec;
}
//---------------------------------------------------------------------------
FILETIME DateTimeToFileTime(const TDateTime & DateTime,
  TDSTMode /*DSTMode*/)
{
  __int64 UnixTimeStamp = ::DateTimeToUnix(DateTime);

  const TDateTimeParams * Params = GetDateTimeParams(DecodeYear(DateTime));
  if (!Params->DaylightHack)
  {
    UnixTimeStamp += (IsDateInDST(DateTime) ?
      Params->DaylightDifferenceSec : Params->StandardDifferenceSec);

    const TDateTimeParams * CurrentParams = GetDateTimeParams(0);
    UnixTimeStamp -= CurrentParams->CurrentDaylightDifferenceSec;
  }
  FILETIME Result;
  (*(__int64*)&(Result) = ((__int64)(UnixTimeStamp) + 11644473600LL) * 10000000LL);

  return Result;
}
//---------------------------------------------------------------------------
TDateTime FileTimeToDateTime(const FILETIME & FileTime)
{
  // duplicated in DirView.pas
  TDateTime Result;
  // The 0xFFF... is sometime seen for invalid timestamps,
  // it would cause failure in SystemTimeToDateTime below
  if (FileTime.dwLowDateTime == DWORD(-1) / sizeof(DWORD)) // std::numeric_limits<DWORD>::max())
  {
    Result = MinDateTime;
  }
  else
  {
    SYSTEMTIME SysTime;
    if (!UsesDaylightHack())
    {
      SYSTEMTIME UniverzalSysTime;
      FileTimeToSystemTime(&FileTime, &UniverzalSysTime);
      SystemTimeToTzSpecificLocalTime(nullptr, &UniverzalSysTime, &SysTime);
    }
    else
    {
      FILETIME LocalFileTime;
      FileTimeToLocalFileTime(&FileTime, &LocalFileTime);
      FileTimeToSystemTime(&LocalFileTime, &SysTime);
    }
    Result = SystemTimeToDateTimeVerbose(SysTime);
  }
/*
  SYSTEMTIME SysTime;
  if (!UsesDaylightHack())
  {
    SYSTEMTIME UniverzalSysTime;
    FileTimeToSystemTime(&FileTime, &UniverzalSysTime);
    SystemTimeToTzSpecificLocalTime(nullptr, &UniverzalSysTime, &SysTime);
  }
  else
  {
    FILETIME LocalFileTime;
    FileTimeToLocalFileTime(&FileTime, &LocalFileTime);
    FileTimeToSystemTime(&LocalFileTime, &SysTime);
  }
  TDateTime Result = SystemTimeToDateTime(SysTime);
*/
  return Result;
}
//---------------------------------------------------------------------------
__int64 ConvertTimestampToUnix(const FILETIME & FileTime,
  TDSTMode DSTMode)
{
  __int64 Result = ((*(__int64*)&(FileTime)) / 10000000LL - 11644473600LL);
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
//---------------------------------------------------------------------------
TDateTime ConvertTimestampToUTC(const TDateTime & DateTime)
{
  TDateTime Result = DateTime;
  const TDateTimeParams * Params = GetDateTimeParams(DecodeYear(Result));
  Result +=
    (IsDateInDST(Result) ?
      Params->DaylightDifference : Params->StandardDifference);
  Result += Params->BaseDifference;

  if (Params->DaylightHack)
  {
    const TDateTimeParams * CurrentParams = GetDateTimeParams(0);
    Result += CurrentParams->CurrentDaylightDifference;
  }

  return Result;
}
//---------------------------------------------------------------------------
TDateTime ConvertTimestampFromUTC(const TDateTime & DateTime)
{
  TDateTime Result = DateTime;
  const TDateTimeParams * Params = GetDateTimeParams(DecodeYear(Result));
  Result -=
    (IsDateInDST(Result) ?
      Params->DaylightDifference : Params->StandardDifference);
  Result -= Params->BaseDifference;

  if (Params->DaylightHack)
  {
    const TDateTimeParams * CurrentParams = GetDateTimeParams(0);
    Result -= CurrentParams->CurrentDaylightDifference;
  }

  return Result;
}
//---------------------------------------------------------------------------
__int64 ConvertTimestampToUnixSafe(const FILETIME & FileTime,
  TDSTMode DSTMode)
{
  __int64 Result;
  if ((FileTime.dwLowDateTime == 0) &&
      (FileTime.dwHighDateTime == 0))
  {
    Result = ::DateTimeToUnix(Now());
  }
  else
  {
    Result = ConvertTimestampToUnix(FileTime, DSTMode);
  }
  return Result;
}
//---------------------------------------------------------------------------
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
      if (IsDateInDST(Result))
      {
        Result = Result + Params->DaylightDifference;
      }
      else
      {
        Result = Result + Params->StandardDifference;
      }
    }
  }

  return Result;
}
//---------------------------------------------------------------------------
UnicodeString FixedLenDateTimeFormat(const UnicodeString & Format)
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
//---------------------------------------------------------------------------
static UnicodeString FormatTimeZone(long Sec)
{
  // TTimeSpan Span = TTimeSpan::FromSeconds(Sec);
  UnicodeString Str;
  /* if ((Span.Seconds == 0) && (Span.Minutes == 0))
  {
    Str = FORMAT(L"%d", -Span.Hours);
  }
  else if (Span.Seconds == 0)
  {
    Str = FORMAT(L"%d:%2.2d", -Span.Hours, abs(Span.Minutes));
  }
  else
  {
    Str = FORMAT(L"%d:%2.2d:%2.2d", -Span.Hours, abs(Span.Minutes), abs(Span.Seconds));
  }
  Str = ((Span <= TTimeSpan::Zero) ? L"+" : L"") + Str;
  */
  return Str;
}
//---------------------------------------------------------------------------
UnicodeString GetTimeZoneLogString()
{
  const TDateTimeParams * Params = GetDateTimeParams(0);

  UnicodeString Result =
    FORMAT(L"Current: GMT%s, Standard: GMT%s, DST: GMT%s, DST Start: %s, DST End: %s",
      FormatTimeZone(Params->CurrentDifferenceSec).c_str(),
       FormatTimeZone(Params->BaseDifferenceSec + Params->StandardDifferenceSec).c_str(),
       FormatTimeZone(Params->BaseDifferenceSec + Params->DaylightDifferenceSec).c_str(),
       Params->DaylightDate.DateString().c_str(),
       Params->StandardDate.DateString().c_str());
  return Result;
}
//---------------------------------------------------------------------------
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
    if (Registry->OpenKey(L"SYSTEM", false) &&
        Registry->OpenKey(L"CurrentControlSet", false) &&
        Registry->OpenKey(L"Control", false) &&
        Registry->OpenKey(L"TimeZoneInformation", false))
    {
      if (Registry->ValueExists(L"DynamicDaylightTimeDisabled"))
      {
        DynamicDaylightTimeDisabled = Registry->ReadBool(L"DynamicDaylightTimeDisabled");
      }
      // WORKAROUND
      // Windows XP equivalent
      else if (Registry->ValueExists(L"DisableAutoDaylightTimeSet"))
      {
        DynamicDaylightTimeDisabled = Registry->ReadBool(L"DisableAutoDaylightTimeSet");
      }
    }
  }
  catch (...)
  {
  }
  return !DynamicDaylightTimeDisabled;
}
//---------------------------------------------------------------------------
UnicodeString StandardTimestamp(const TDateTime & DateTime)
{
#if defined(__BORLANDC__)
  return FormatDateTime(L"yyyy'-'mm'-'dd'T'hh':'nn':'ss'.'zzz'Z'", ConvertTimestampToUTC(DateTime));
#else
  TDateTime DT = ConvertTimestampToUTC(DateTime);
  unsigned short Y, M, D, H, N, S, MS;
  DT.DecodeDate(Y, M, D);
  DT.DecodeTime(H, N, S, MS);
  UnicodeString Result = FORMAT(L"%04d-%02d-%02dT%02d:%02d:%02d.%03dZ", Y, M, D, H, N, S, MS);
  return Result;
#endif
}
//---------------------------------------------------------------------------
UnicodeString StandardTimestamp()
{
  return StandardTimestamp(Now());
}
//---------------------------------------------------------------------------
static TDateTime TwoSeconds(0, 0, 2, 0);

intptr_t CompareFileTime(const TDateTime & T1, const TDateTime & T2)
{
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
//---------------------------------------------------------------------------
intptr_t TimeToMSec(const TDateTime & T)
{
  return int(Round(double(T) * double(MSecsPerDay)));
}
//---------------------------------------------------------------------------
intptr_t TimeToMinutes(const TDateTime & T)
{
  return TimeToMSec(T) / MSecsPerSec / SecsPerMin;
}
//---------------------------------------------------------------------------
bool RecursiveDeleteFile(const UnicodeString & FileName, bool ToRecycleBin)
{
  SHFILEOPSTRUCT Data;

  ClearStruct(Data);
  Data.hwnd = nullptr;
  Data.wFunc = FO_DELETE;
  UnicodeString FileList(FileName);
  FileList.SetLength(FileList.Length() + 2);
  FileList[FileList.Length() - 1] = L'\0';
  FileList[FileList.Length()] = L'\0';
  Data.pFrom = FileList.c_str();
  Data.pTo = L"\0\0";
  Data.fFlags = FOF_NOCONFIRMATION | FOF_RENAMEONCOLLISION | FOF_NOCONFIRMMKDIR |
    FOF_NOERRORUI | FOF_SILENT;
  if (ToRecycleBin)
  {
    Data.fFlags |= FOF_ALLOWUNDO;
  }
  int ErrorCode = SHFileOperation(&Data);
  bool Result = (ErrorCode == 0);
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
    SetLastError(ErrorCode);
  }
  return Result;
}
//---------------------------------------------------------------------------
void DeleteFileChecked(const UnicodeString & FileName)
{
  if (!DeleteFile(FileName))
  {
    throw EOSExtException(FMTLOAD(CORE_DELETE_LOCAL_FILE_ERROR, FileName.c_str()));
  }
}
//---------------------------------------------------------------------------
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
    assert(false);
    Result = qaCancel;
  }
  return Result;
}
//---------------------------------------------------------------------------
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
//---------------------------------------------------------------------------
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
//---------------------------------------------------------------------------
UnicodeString LoadStr(int Ident, intptr_t MaxLength)
{
  UnicodeString Result;
  Result.SetLength(MaxLength > 0 ? MaxLength : 1024);
  HINSTANCE hInstance =  GetGlobalFunctions()->GetInstanceHandle();
  assert(hInstance != 0);
  intptr_t Length = static_cast<intptr_t>(::LoadString(hInstance, Ident, reinterpret_cast<LPWSTR>(const_cast<wchar_t *>(Result.c_str())), (int)Result.Length()));
  Result.SetLength(Length);
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString LoadStrPart(int Ident, int Part)
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
//---------------------------------------------------------------------------
UnicodeString DecodeUrlChars(const UnicodeString & S)
{
  UnicodeString Result = S;
  intptr_t I = 1;
  while (I <= Result.Length())
  {
    switch (Result[I])
    {
      case L'+':
        Result[I] = ' ';
        break;

      case L'%':
        UnicodeString Hex;
        while ((I + 2 <= Result.Length()) && (Result[I] == L'%') &&
               IsHex(Result[I + 1]) && IsHex(Result[I + 2]))
        {
          Hex += Result.SubString(I + 1, 2);
          Result.Delete(I, 3);
        }

        if (!Hex.IsEmpty())
        {
          RawByteString Bytes = HexToBytes(Hex);
          UTF8String UTF8(Bytes.c_str(), Bytes.Length());
          UnicodeString Chars(UTF8);
          Result.Insert(Chars, I);
          I += Chars.Length() - 1;
        }
        break;
    }
    I++;
  }
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString DoEncodeUrl(const UnicodeString & S, const UnicodeString & Chars)
{
  UnicodeString Result = S;
  intptr_t i = 1;
  while (i <= Result.Length())
  {
    if (Chars.Pos(Result[i]) > 0)
    {
      UnicodeString H = ByteToHex(AnsiString(UnicodeString(Result[i]))[1]);
      Result.Insert(H, i + 1);
      Result[i] = '%';
      i += H.Length();
    }
    i++;
  }
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString EncodeUrlChars(const UnicodeString & S, const UnicodeString & Ignore)
{
  UnicodeString Chars;
  if (Ignore.Pos(L' ') == 0)
  {
    Chars += L' ';
  }
  if (Ignore.Pos(L'/') == 0)
  {
    Chars += L'/';
  }
  return DoEncodeUrl(S, Chars);
}
//---------------------------------------------------------------------------
UnicodeString NonUrlChars()
{
  UnicodeString S;
  for (uintptr_t I = 0; I <= 127; I++)
  {
    wchar_t C = static_cast<wchar_t>(I);
    if (IsLetter(C) ||
        IsDigit(C) ||
        (C == L'_') || (C == L'-') || (C == L'.'))
    {
      // noop
    }
    else
    {
      S += C;
    }
  }
  return S;
}
//---------------------------------------------------------------------------
UnicodeString EncodeUrlString(const UnicodeString & S)
{
  return DoEncodeUrl(S, NonUrlChars());
}
//---------------------------------------------------------------------------
UnicodeString EscapeHotkey(const UnicodeString & Caption)
{
  return ReplaceStr(Caption, L"&", L"&&");
}
//---------------------------------------------------------------------------
// duplicated in console's Main.cpp
bool CutToken(UnicodeString & Str, UnicodeString & Token,
  UnicodeString * RawToken)
{
  bool Result;

  Token = L"";

  // inspired by Putty's sftp_getcmd() from PSFTP.C
  intptr_t Index = 1;
  while ((Index <= Str.Length()) &&
    ((Str[Index] == L' ') || (Str[Index] == L'\t')))
  {
    ++Index;
  }

  if (Index <= Str.Length())
  {
    bool Quoting = false;

    while (Index <= Str.Length())
    {
      if (!Quoting && ((Str[Index] == L' ') || (Str[Index] == L'\t')))
      {
        break;
      }
      else if ((Str[Index] == L'"') && (Index + 1 <= Str.Length()) &&
        (Str[Index + 1] == L'"'))
      {
        Index += 2;
        Token += L'"';
      }
      else if (Str[Index] == L'"')
      {
        ++Index;
        Quoting = !Quoting;
      }
      else
      {
        Token += Str[Index];
        ++Index;
      }
    }

    if (RawToken != nullptr)
    {
      (*RawToken) = Str.SubString(1, Index - 1);
    }

    if (Index <= Str.Length())
    {
      ++Index;
    }

    Str = Str.SubString(Index, Str.Length());

    Result = true;
  }
  else
  {
    Result = false;
    Str = L"";
  }

  return Result;
}
//---------------------------------------------------------------------------
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
//---------------------------------------------------------------------------
bool CheckWin32Version(int Major, int Minor)
{
  return (Win32MajorVersion >= Major) && (Win32MinorVersion >= Minor);
}

//---------------------------------------------------------------------------
bool IsWinVista()
{
  // Vista is 6.0
  // Win XP is 5.1
  // There also 5.2, what is Windows 2003 or Windows XP 64bit
  // (we consider it WinXP for now)
  return CheckWin32Version(6, 0);
}
//---------------------------------------------------------------------------
bool IsWin7()
{
//  return CheckWin32Version(6, 1);
  return
    (Win32MajorVersion > 6) ||
    ((Win32MajorVersion == 6) && (Win32MinorVersion >= 1));
}
//---------------------------------------------------------------------------
LCID GetDefaultLCID()
{
  return GetUserDefaultLCID();
}
//---------------------------------------------------------------------------
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
//---------------------------------------------------------------------------
bool GetWindowsProductType(DWORD & Type)
{
  bool Result;
  HINSTANCE Kernel32 = GetModuleHandle(kernel32);
  typedef BOOL (WINAPI * TGetProductInfo)(DWORD, DWORD, DWORD, DWORD, PDWORD);
  TGetProductInfo GetProductInfo =
      (TGetProductInfo)GetProcAddress(Kernel32, "GetProductInfo");
  if (GetProductInfo == nullptr)
  {
    Result = false;
  }
  else
  {
    GetProductInfo(Win32MajorVersion, Win32MinorVersion, 0, 0, &Type);
    Result = true;
  }
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString WindowsProductName()
{
  UnicodeString Result;
  std::unique_ptr<TRegistry> Registry(new TRegistry());
  Registry->SetAccess(KEY_READ);
  try
  {
    Registry->SetRootKey(HKEY_LOCAL_MACHINE);
    if (Registry->OpenKey(L"SOFTWARE", false) &&
        Registry->OpenKey(L"Microsoft", false) &&
        Registry->OpenKey(L"Windows NT", false) &&
        Registry->OpenKey(L"CurrentVersion", false))
    {
      Result = Registry->ReadString(L"ProductName");
    }
  }
  catch (...)
  {
  }
  return Result;
}
//---------------------------------------------------------------------------
bool IsDirectoryWriteable(const UnicodeString & Path)
{
  UnicodeString FileName =
    ::IncludeTrailingPathDelimiter(Path) +
    FORMAT(L"wscp_%s_%d.tmp", FormatDateTime(L"nnzzz", Now()).c_str(), int(GetCurrentProcessId()));
  HANDLE Handle = ::CreateFile(FileName.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
    CREATE_NEW, FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, 0);
  bool Result = (Handle != INVALID_HANDLE_VALUE);
  if (Result)
  {
    ::CloseHandle(Handle);
  }
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString FormatNumber(__int64 Number)
{
//  return FormatFloat(L"#,##0", Number);
  return FORMAT(L"%.0f", static_cast<double>(Number));
}
//---------------------------------------------------------------------------
// simple alternative to FormatBytes
UnicodeString FormatSize(__int64 Size)
{
  return FormatNumber(Size);
}
//---------------------------------------------------------------------------
UnicodeString ExtractFileBaseName(const UnicodeString & Path)
{
  return ChangeFileExt(::ExtractFileName(Path, false), L"");
}
//---------------------------------------------------------------------
UnicodeString FormatBytes(__int64 Bytes, bool UseOrders)
{
  UnicodeString Result;

  if (!UseOrders || (Bytes < static_cast<__int64>(100*1024)))
  {
    // Result = FormatFloat(L"#,##0 \"B\"", Bytes);
    Result = FORMAT(L"%.0f B", static_cast<double>(Bytes));
  }
  else if (Bytes < static_cast<__int64>(100*1024*1024))
  {
    // Result = FormatFloat(L"#,##0 \"KiB\"", Bytes / 1024);
    Result = FORMAT(L"%.0f KiB", static_cast<double>(Bytes / 1024.0));
  }
  else
  {
    // Result = FormatFloat(L"#,##0 \"MiB\"", Bytes / (1024*1024));
    Result = FORMAT(L"%.0f MiB", static_cast<double>(Bytes / (1024*1024.0)));
  }
  return Result;
}
//---------------------------------------------------------------------------
// Suppress warning about unused constants in DateUtils.hpp
#pragma warn -8080

