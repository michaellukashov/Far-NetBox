//---------------------------------------------------------------------------
#ifndef _MSC_VER
#define NO_WIN32_LEAN_AND_MEAN
#include <vcl.h>
#pragma hdrstop
#else

#include "stdafx.h"

#include "boostdefines.hpp"
#include <boost/scope_exit.hpp>
#include <boost/algorithm/string.hpp>
#include "boost/date_time.hpp"
#include "boost/date_time/local_time/local_time.hpp"
#include "Classes.h"
#include "FarPlugin.h"
#endif

#include "Common.h"
#include "Exceptions.h"
#include "TextsCore.h"
#include "Interface.h"
#ifndef _MSC_VER
#include <StrUtils.hpp>
#include <DateUtils.hpp>
#endif
#include <math.h>
#include <shlobj.h>

#ifdef _MSC_VER
namespace alg = boost::algorithm;
#endif
//---------------------------------------------------------------------------
#ifndef _MSC_VER
#pragma package(smart_init)
#endif
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// TGuard
//---------------------------------------------------------------------------
TGuard::TGuard(TCriticalSection * ACriticalSection) :
  FCriticalSection(ACriticalSection)
{
  assert(ACriticalSection != NULL);
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
  assert(ACriticalSection != NULL);
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
const char Bom[4] = "\xEF\xBB\xBF";
const wchar_t TokenPrefix = L'%';
const wchar_t NoReplacement = wchar_t(false);
const wchar_t TokenReplacement = wchar_t(true);
const UnicodeString LocalInvalidChars = L"/\\:*?\"<>|";
//---------------------------------------------------------------------------
UnicodeString ReplaceChar(UnicodeString Str, wchar_t A, wchar_t B)
{
  for (Integer Index = 0; Index < Str.Length(); Index++)
    if (Str[Index+1] == A) Str[Index+1] = B;
  return Str;
}
//---------------------------------------------------------------------------
UnicodeString DeleteChar(UnicodeString Str, wchar_t C)
{
  int P;
  while ((P = Str.Pos(C)) > 0)
  {
    Str.Delete(P, 1);
  }
  return Str;
}
//---------------------------------------------------------------------------
void PackStr(UnicodeString &Str)
{
  // Following will free unnecessary bytes
  Str = Str.c_str();
}
//---------------------------------------------------------------------------
void PackStr(RawByteString &Str)
{
  // Following will free unnecessary bytes
  Str = Str.c_str();
}
//---------------------------------------------------------------------------
UnicodeString MakeValidFileName(UnicodeString FileName)
{
  UnicodeString IllegalChars = L":;,=+<>|\"[] \\/?*";
  for (int Index = 0; Index < IllegalChars.Length(); Index++)
  {
    FileName = ReplaceChar(FileName, IllegalChars[Index+1], L'-');
  }
  return FileName;
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
  {  Abort(); return L""; };
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
UnicodeString CutToChar(UnicodeString &Str, wchar_t Ch, bool Trim)
{
  Integer P = Str.Pos(Ch);
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
UnicodeString CopyToChars(const UnicodeString & Str, int & From, UnicodeString Chs, bool Trim,
  wchar_t * Delimiter, bool DoubleDelimiterEscapes)
{
  UnicodeString Result;

  int P;
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
    if (Delimiter != NULL)
    {
      *Delimiter = Str[P];
    }
  }
  else
  {
    if (Delimiter != NULL)
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
UnicodeString DelimitStr(UnicodeString Str, UnicodeString Chars)
{
  for (int i = 1; i <= Str.Length(); i++)
  {
    if (Str.IsDelimiter(Chars, i))
    {
      Str.Insert(L"\\", i);
      i++;
    }
  }
  return Str;
}
//---------------------------------------------------------------------------
UnicodeString ShellDelimitStr(UnicodeString Str, wchar_t Quote)
{
  UnicodeString Chars = L"$\\";
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
  if (E->InheritsFrom<Exception>())
  {
    UnicodeString Msg;
#ifndef _MSC_VER
    Msg = FORMAT(L"(%s) %s", (E->ClassName(), E->GetMessage().c_str()));
#else
    Msg = FORMAT(L"%s", ::MB2W(E->what()).c_str());
#endif
    if (E->InheritsFrom<ExtException>())
    {
      TStrings * MoreMessages = dynamic_cast<ExtException *>(E)->GetMoreMessages();
      if (MoreMessages)
      {
        Msg += L"\n" +
          StringReplace(MoreMessages->GetText(), L"\r", L"", TReplaceFlags::Init(rfReplaceAll));
      }
    }
    return Msg;
  }
  else
  {
#ifndef _MSC_VER
    wchar_t Buffer[1024];
    ExceptionErrorMessage(ExceptObject(), ExceptAddr(), Buffer, LENOF(Buffer));
    return UnicodeString(Buffer);
#else
    return UnicodeString(E->what());
#endif
  }
}
//---------------------------------------------------------------------------
bool IsNumber(const UnicodeString Str)
{
  int Value;
  return TryStrToInt(Str, Value);
}
//---------------------------------------------------------------------------
UnicodeString __fastcall SystemTemporaryDirectory()
{
  UnicodeString TempDir;
  TempDir.SetLength(MAX_PATH);
  TempDir.SetLength(GetTempPath(MAX_PATH, (LPWSTR)TempDir.c_str()));
  return TempDir;
}
//---------------------------------------------------------------------------
UnicodeString __fastcall GetShellFolderPath(int CSIdl)
{
  UnicodeString Result;
  wchar_t Path[2 * MAX_PATH + 10] = L"\0";
  if (SUCCEEDED(SHGetFolderPath(NULL, CSIdl, NULL, SHGFP_TYPE_CURRENT, Path)))
  {
    Result = Path;
  }
/*
  HMODULE Shell32Lib = LoadLibrary(L"SHELL32.DLL");
  if (Shell32Lib != NULL)
  {
    typedef HRESULT (__stdcall *PFNSHGETFOLDERPATH)(HWND, int, HANDLE, DWORD, LPTSTR);
    PFNSHGETFOLDERPATH SHGetFolderPath = reinterpret_cast<PFNSHGETFOLDERPATH>(
                                           GetProcAddress(Shell32Lib, "SHGetFolderPathA"));
    if (SHGetFolderPath != NULL)
    {
      wchar_t Path[2 * MAX_PATH + 10] = L"\0";
      const int SHGFP_TYPE_CURRENT = 0;
      if (SUCCEEDED(SHGetFolderPath(NULL, CSIdl, NULL, SHGFP_TYPE_CURRENT, Path)))
      {
        Result = Path;
      }
    }
  }
*/
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString __fastcall StripPathQuotes(const UnicodeString Path)
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
UnicodeString __fastcall AddPathQuotes(UnicodeString Path)
{
  Path = StripPathQuotes(Path);
  if (Path.Pos(L" "))
  {
    Path = L"\"" + Path + L"\"";
  }
  return Path;
}
//---------------------------------------------------------------------------
static wchar_t * __fastcall ReplaceChar(
  UnicodeString & FileName, wchar_t * InvalidChar, wchar_t InvalidCharsReplacement)
{
  int Index = InvalidChar - FileName.c_str() + 1;
  if (InvalidCharsReplacement == TokenReplacement)
  {
    // currently we do not support unicode chars replacement
    if (FileName[Index] > 0xFF)
    {
      EXCEPTION;
    }

    FileName.Insert(ByteToHex(static_cast<unsigned char>(FileName[Index])), Index + 1);
    FileName[Index] = TokenPrefix;
    InvalidChar = (wchar_t *)FileName.c_str() + Index + 2;
  }
  else
  {
    FileName[Index] = InvalidCharsReplacement;
    InvalidChar++;
  }
  return InvalidChar;
}
//---------------------------------------------------------------------------
UnicodeString __fastcall ValidLocalFileName(UnicodeString FileName)
{
  return ValidLocalFileName(FileName, L'_', L"", LocalInvalidChars);
}
//---------------------------------------------------------------------------
UnicodeString __fastcall ValidLocalFileName(
  UnicodeString FileName, wchar_t InvalidCharsReplacement,
  const UnicodeString & TokenizibleChars, const UnicodeString & LocalInvalidChars)
{
  if (InvalidCharsReplacement != NoReplacement)
  {
    bool ATokenReplacement = (InvalidCharsReplacement == TokenReplacement);
    const wchar_t * Chars =
      (ATokenReplacement ? TokenizibleChars : LocalInvalidChars).c_str();
    wchar_t * InvalidChar = (wchar_t *)FileName.c_str();
    while ((InvalidChar = wcspbrk(InvalidChar, Chars)) != NULL)
    {
      int Pos = (InvalidChar - FileName.c_str() + 1);
      wchar_t Char;
      if (ATokenReplacement &&
          (*InvalidChar == TokenPrefix) &&
          (((FileName.Length() - Pos) <= 1) ||
           (((Char = static_cast<wchar_t>(HexToByte(FileName.SubString(Pos + 1, 2)))) == L'\0') ||
            (TokenizibleChars.Pos(Char) == 0))))
      {
        InvalidChar++;
      }
      else
      {
        InvalidChar = ReplaceChar(FileName, InvalidChar, InvalidCharsReplacement);
      }
    }

    // Windows trim trailing space or dot, hence we must encode it to preserve it
    if (!FileName.IsEmpty() &&
        ((FileName[FileName.Length()] == L' ') ||
         (FileName[FileName.Length()] == L'.')))
    {
      ReplaceChar(FileName, (wchar_t *)FileName.c_str() + FileName.Length() - 1, InvalidCharsReplacement);
    }

    if (IsReservedName(FileName))
    {
      int P = FileName.Pos(L'.');
      if (P == 0)
      {
        P = FileName.Length() + 1;
      }
      FileName.Insert(L"%00", P);
    }
  }
  return FileName;
}
//---------------------------------------------------------------------------
void __fastcall SplitCommand(UnicodeString Command, UnicodeString &Program,
  UnicodeString & Params, UnicodeString & Dir)
{
  Command = Command.Trim();
  Params = L"";
  Dir = L"";
  if (!Command.IsEmpty() && (Command[1] == L'\"'))
  {
    Command.Delete(1, 1);
    int P = Command.Pos(L'"');
    if (P)
    {
      Program = Command.SubString(1, P-1).Trim();
      Params = Command.SubString(P + 1, Command.Length() - P).Trim();
    }
    else
    {
      throw Exception(FMTLOAD(INVALID_SHELL_COMMAND, (L"\"" + Command)));
    }
  }
  else
  {
    int P = Command.Pos(L" ");
    if (P)
    {
      Program = Command.SubString(1, P).Trim();
      Params = Command.SubString(P + 1, Command.Length() - P).Trim();
    }
    else
    {
      Program = Command;
    }
  }
  int B = Program.LastDelimiter(L"\\/");
  if (B)
  {
    Dir = Program.SubString(1, B).Trim();
  }
}
//---------------------------------------------------------------------------
UnicodeString __fastcall ExtractProgram(UnicodeString Command)
{
  UnicodeString Program;
  UnicodeString Params;
  UnicodeString Dir;

  SplitCommand(Command, Program, Params, Dir);

  return Program;
}
//---------------------------------------------------------------------------
UnicodeString __fastcall FormatCommand(UnicodeString Program, UnicodeString Params)
{
  Program = Program.Trim();
  Params = Params.Trim();
  if (!Params.IsEmpty()) Params = L" " + Params;
  if (Program.Pos(L" ")) Program = L"\"" + Program + L"\"";
  return Program + Params;
}
//---------------------------------------------------------------------------
const wchar_t ShellCommandFileNamePattern[] = L"!.!";
//---------------------------------------------------------------------------
void __fastcall ReformatFileNameCommand(UnicodeString & Command)
{
  if (!Command.IsEmpty())
  {
    UnicodeString Program, Params, Dir;
    SplitCommand(Command, Program, Params, Dir);
    size_t nPos = 0;
    if (!Params.Pos(nPos, ShellCommandFileNamePattern))
    {
      Params = Params + (Params.IsEmpty() ? L"" : L" ") + ShellCommandFileNamePattern;
    }
    Command = FormatCommand(Program, Params);
  }
}
//---------------------------------------------------------------------------
UnicodeString __fastcall ExpandFileNameCommand(const UnicodeString Command,
  const UnicodeString FileName)
{
  return AnsiReplaceStr(Command, ShellCommandFileNamePattern,
    AddPathQuotes(FileName));
}
//---------------------------------------------------------------------------
UnicodeString __fastcall EscapePuttyCommandParam(UnicodeString Param)
{
  bool Space = false;

  for (int i = 1; i <= Param.Length(); i++)
  {
    switch (Param[i])
    {
      case L'"':
        Param.Insert(L"\\", i);
        i++;
        break;

      case L' ':
        Space = true;
        break;

      case L'\\':
        int i2 = i;
        while ((i2 <= Param.Length()) && (Param[i2] == L'\\'))
        {
          i2++;
        }
        if ((i2 <= Param.Length()) && (Param[i2] == L'"'))
        {
          while (Param[i] == L'\\')
          {
            Param.Insert(L"\\", i);
            i += 2;
          }
          i--;
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
//---------------------------------------------------------------------------
UnicodeString __fastcall ExpandEnvironmentVariables(const UnicodeString & Str)
{
  UnicodeString Buf;
  unsigned int Size = 1024;

  Buf.SetLength(Size);
  Buf.Unique();
  unsigned int Len = ExpandEnvironmentStrings(Str.c_str(), (LPWSTR)Buf.c_str(), Size);

  if (Len > Size)
  {
    Buf.SetLength(Len);
    Buf.Unique();
    ExpandEnvironmentStrings(Str.c_str(), (LPWSTR)Buf.c_str(), Len);
  }

  PackStr(Buf);

  return Buf;
}
//---------------------------------------------------------------------------
bool __fastcall CompareFileName(const UnicodeString & Path1, const UnicodeString & Path2)
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
bool __fastcall ComparePaths(const UnicodeString & Path1, const UnicodeString & Path2)
{
  // TODO: ExpandUNCFileName
  return AnsiSameText(IncludeTrailingBackslash(Path1), IncludeTrailingBackslash(Path2));
}
//---------------------------------------------------------------------------
bool __fastcall IsReservedName(UnicodeString FileName)
{
  int P = FileName.Pos(L".");
  int Len = (P > 0) ? P - 1 : FileName.Length();
  if ((Len == 3) || (Len == 4))
  {
    if (P > 0)
    {
      FileName.SetLength(P - 1);
    }
    static UnicodeString Reserved[] = {
      L"CON", L"PRN", L"AUX", L"NUL",
      L"COM1", L"COM2", L"COM3", L"COM4", L"COM5", L"COM6", L"COM7", L"COM8", L"COM9",
      L"LPT1", L"LPT2", L"LPT3", L"LPT4", L"LPT5", L"LPT6", L"LPT7", L"LPT8", L"LPT9" };
    for (unsigned int Index = 0; Index < LENOF(Reserved); Index++)
    {
      if (SameText(FileName, Reserved[Index]))
      {
        return true;
      }
    }
  }
  return false;
}
//---------------------------------------------------------------------------
UnicodeString __fastcall DisplayableStr(const RawByteString & Str)
{
  bool Displayable = true;
  int Index = 1;
  while ((Index <= Str.Length()) && Displayable)
  {
    if (((Str[Index] < '\x20') || (static_cast<unsigned char>(Str[Index]) >= static_cast<unsigned char>('\x80'))) &&
        (Str[Index] != '\n') && (Str[Index] != '\r') && (Str[Index] != '\t') && (Str[Index] != '\b'))
    {
      Displayable = false;
    }
    Index++;
  }

  UnicodeString Result;
  if (Displayable)
  {
    Result = L"\"";
    for (int Index = 1; Index <= Str.Length(); Index++)
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
UnicodeString __fastcall ByteToHex(unsigned char B, bool UpperCase)
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
UnicodeString __fastcall BytesToHex(const unsigned char * B, size_t Length, bool UpperCase, wchar_t Separator)
{
  UnicodeString Result;
  for (size_t i = 0; i < Length; i++)
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
UnicodeString __fastcall BytesToHex(RawByteString Str, bool UpperCase, wchar_t Separator)
{
  return BytesToHex(reinterpret_cast<const unsigned char *>(Str.c_str()), Str.Length(), UpperCase, Separator);
}
//---------------------------------------------------------------------------
UnicodeString __fastcall CharToHex(wchar_t Ch, bool UpperCase)
{
  return BytesToHex(reinterpret_cast<const unsigned char *>(&Ch), sizeof(Ch), UpperCase);
}
//---------------------------------------------------------------------------
RawByteString __fastcall HexToBytes(const UnicodeString Hex)
{
  static UnicodeString Digits = L"0123456789ABCDEF";
  RawByteString Result;
  int L, P1, P2;
  L = Hex.Length();
  if (L % 2 == 0)
  {
    for (int i = 1; i <= Hex.Length(); i += 2)
    {
      P1 = Digits.Pos((wchar_t)toupper(Hex[i]));
      P2 = Digits.Pos((wchar_t)toupper(Hex[i + 1]));
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
unsigned char __fastcall HexToByte(const UnicodeString Hex)
{
  static UnicodeString Digits = L"0123456789ABCDEF";
  assert(Hex.Length() == 2);
  int P1 = Digits.Pos((wchar_t)toupper(Hex[1]));
  int P2 = Digits.Pos((wchar_t)toupper(Hex[2]));

  return
    static_cast<unsigned char>(((P1 <= 0) || (P2 <= 0)) ? 0 : (((P1 - 1) << 4) + (P2 - 1)));
}
//---------------------------------------------------------------------------
bool __fastcall FileSearchRec(const UnicodeString FileName, TSearchRec & Rec)
{
  int FindAttrs = faReadOnly | faHidden | faSysFile | faDirectory | faArchive;
  bool Result = (FindFirst(FileName, FindAttrs, Rec) == 0);
  if (Result)
  {
    FindClose(Rec);
  }
  return Result;
}
//---------------------------------------------------------------------------
void __fastcall ProcessLocalDirectory(UnicodeString DirName,
  const TProcessLocalFileEvent & CallBackFunc, void * Param,
  int FindAttrs)
{
  // assert(CallBackFunc);
  if (FindAttrs < 0)
  {
    FindAttrs = faReadOnly | faHidden | faSysFile | faDirectory | faArchive;
  }
  TSearchRec SearchRec = {0};

  DirName = IncludeTrailingBackslash(DirName);
  if (FindFirst(DirName + L"*.*", FindAttrs, SearchRec) == 0)
  {
#ifndef _MSC_VER
    try
#endif
    {
#ifdef _MSC_VER
      BOOST_SCOPE_EXIT ( (&SearchRec) )
      {
        FindClose(SearchRec);
      } BOOST_SCOPE_EXIT_END
#endif
      do
      {
        if ((SearchRec.Name != L".") && (SearchRec.Name != L".."))
        {
          CallBackFunc(DirName + SearchRec.Name, SearchRec, Param);
        }

      } while (FindNext(SearchRec) == 0);
    }
#ifndef _MSC_VER
    __finally
    {
      FindClose(SearchRec);
    }
#endif
  }
/*
  WIN32_FIND_DATA SearchRec;

  UnicodeString dirName = IncludeTrailingBackslash(DirName);
  UnicodeString FileName = dirName + L"*.*";
  HANDLE h = ::FindFirstFileW(FileName.c_str(), &SearchRec);
  if (h != INVALID_HANDLE_VALUE)
  {
    BOOST_SCOPE_EXIT ( (&h) )
    {
      ::FindClose(h);
    } BOOST_SCOPE_EXIT_END
    processlocalfile_signal_type sig;
    sig.connect(CallBackFunc);
    do
    {
      if ((wcscmp(SearchRec.cFileName, THISDIRECTORY) != 0) && (wcscmp(SearchRec.cFileName, PARENTDIRECTORY) != 0))
      {
        if ((SearchRec.dwFileAttributes & FindAttrs) != 0)
        {
          sig(dirName + SearchRec.cFileName, SearchRec, Param);
        }
      }
    }
    while (::FindNextFile(h, &SearchRec));
  }
*/
}
//---------------------------------------------------------------------------
TDateTime __fastcall EncodeDateVerbose(Word Year, Word Month, Word Day)
{
  try
  {
    return EncodeDate(Year, Month, Day);
  }
  catch (EConvertError & E)
  {
    throw EConvertError(FORMAT(L"%s [%04u-%02u-%02u]", E.GetMessage().c_str(), int(Year), int(Month), int(Day)));
  }
  return TDateTime();
}
//---------------------------------------------------------------------------
TDateTime __fastcall EncodeTimeVerbose(Word Hour, Word Min, Word Sec, Word MSec)
{
  try
  {
    return EncodeTime(Hour, Min, Sec, MSec);
  }
  catch (EConvertError & E)
  {
    throw EConvertError(FORMAT(L"%s [%02u:%02u:%02u.%04u]", E.GetMessage().c_str(), int(Hour), int(Min), int(Sec), int(MSec)));
  }
  return System::TDateTime();
}

//---------------------------------------------------------------------------
struct TDateTimeParams
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
typedef std::map<int, TDateTimeParams> TYearlyDateTimeParams;
static TYearlyDateTimeParams YearlyDateTimeParams;
static std::auto_ptr<TCriticalSection> DateTimeParamsSection(new TCriticalSection());
static void __fastcall EncodeDSTMargin(const SYSTEMTIME & Date, unsigned short Year,
  TDateTime & Result);
//---------------------------------------------------------------------------
static unsigned short __fastcall DecodeYear(const TDateTime & DateTime)
{
  unsigned short Year, Month, Day;
  DecodeDate(DateTime, Year, Month, Day);
  return Year;
}
//---------------------------------------------------------------------------
static const TDateTimeParams * __fastcall GetDateTimeParams(unsigned short Year)
{
  TGuard Guard(DateTimeParamsSection.get());

  TDateTimeParams * Result;

  TYearlyDateTimeParams::iterator i = YearlyDateTimeParams.find(Year);
  if (i != YearlyDateTimeParams.end())
  {
    Result = &(*i).second;
  }
  else
  {
    // creates new entry as a side effect
    Result = &YearlyDateTimeParams[Year];
    TIME_ZONE_INFORMATION TZI;

    unsigned long GTZI;

    HINSTANCE Kernel32 = GetModuleHandle(kernel32);
    typedef BOOL WINAPI (* TGetTimeZoneInformationForYear)(USHORT wYear, PDYNAMIC_TIME_ZONE_INFORMATION pdtzi, LPTIME_ZONE_INFORMATION ptzi);
    TGetTimeZoneInformationForYear GetTimeZoneInformationForYear =
      (TGetTimeZoneInformationForYear)GetProcAddress(Kernel32, "GetTimeZoneInformationForYear");

    if ((Year == 0) || (GetTimeZoneInformationForYear == NULL))
    {
      GTZI = GetTimeZoneInformation(&TZI);
    }
    else
    {
      GetTimeZoneInformationForYear(Year, NULL, &TZI);
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
        throw Exception(TIMEZONE_ERROR);
    }

    Result->BaseDifferenceSec = TZI.Bias;
    Result->BaseDifference = double(TZI.Bias) / MinsPerDay;
    Result->BaseDifferenceSec *= SecsPerMin;

    Result->CurrentDifferenceSec = TZI.Bias +
      Result->CurrentDaylightDifferenceSec;
    Result->CurrentDifference =
      double(Result->CurrentDifferenceSec) / MinsPerDay;
    Result->CurrentDifferenceSec *= SecsPerMin;

    Result->CurrentDaylightDifference =
      double(Result->CurrentDaylightDifferenceSec) / MinsPerDay;
    Result->CurrentDaylightDifferenceSec *= SecsPerMin;

    Result->DaylightDifferenceSec = TZI.DaylightBias * SecsPerMin;
    Result->DaylightDifference = double(TZI.DaylightBias) / MinsPerDay;
    Result->StandardDifferenceSec = TZI.StandardBias * SecsPerMin;
    Result->StandardDifference = double(TZI.StandardBias) / MinsPerDay;

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

    Result->DaylightHack = !IsWin7() || IsExactly2008R2();
  }

  return Result;
}
//---------------------------------------------------------------------------
static void __fastcall EncodeDSTMargin(const SYSTEMTIME & Date, unsigned short Year,
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
    Result = Result + EncodeTimeVerbose(Date.wHour, Date.wMinute, Date.wSecond,
      Date.wMilliseconds);
  }
  else
  {
    Result = EncodeDateVerbose(Year, Date.wMonth, Date.wDay) +
      EncodeTimeVerbose(Date.wHour, Date.wMinute, Date.wSecond, Date.wMilliseconds);
  }
}
//---------------------------------------------------------------------------
static bool __fastcall IsDateInDST(const TDateTime & DateTime)
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
bool __fastcall UsesDaylightHack()
{
  return GetDateTimeParams(0)->DaylightHack;
}
//---------------------------------------------------------------------------
TDateTime __fastcall UnixToDateTime(__int64 TimeStamp, TDSTMode DSTMode)
{
  assert(int(EncodeDateVerbose(1970, 1, 1)) == UnixDateDelta);

  TDateTime Result = UnixDateDelta + (double(TimeStamp) / SecsPerDay);

  const TDateTimeParams * Params = GetDateTimeParams(DecodeYear(Result));

  if (Params->DaylightHack)
  {
    if ((DSTMode == dstmWin) || (DSTMode == dstmUnix))
    {
      const TDateTimeParams * CurrentParams = GetDateTimeParams(0);
      Result = Result - CurrentParams->CurrentDifference;
    }
    else if (DSTMode == dstmKeep)
    {
      Result = Result - Params->BaseDifference;
    }
  }
  else
  {
    Result = Result - Params->BaseDifference;
  }

  if ((DSTMode == dstmUnix) || (DSTMode == dstmKeep))
  {
    Result = Result - (IsDateInDST(Result) ?
      Params->DaylightDifference : Params->StandardDifference);
  }

  return Result;
}
//---------------------------------------------------------------------------
__int64 __fastcall Round(double Number)
{
  double Floor = floor(Number);
  double Ceil = ceil(Number);
  return static_cast<__int64>(((Number - Floor) > (Ceil - Number)) ? Ceil : Floor);
}
//---------------------------------------------------------------------------
bool __fastcall TryRelativeStrToDateTime(UnicodeString S, TDateTime & DateTime)
{
  S = S.Trim();
  int Index = 1;
  while ((Index <= S.Length()) && (S[Index] >= '0') && (S[Index] <= '9'))
  {
    Index++;
  }
  UnicodeString NumberStr = S.SubString(1, Index - 1);
  int Number;
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
static __int64 __fastcall DateTimeToUnix(const TDateTime DateTime)
{
  const TDateTimeParams * CurrentParams = GetDateTimeParams(0);

  assert(int(EncodeDateVerbose(1970, 1, 1)) == UnixDateDelta);

  return Round(double(DateTime - UnixDateDelta) * SecsPerDay) +
    CurrentParams->CurrentDifferenceSec;
}
//---------------------------------------------------------------------------
FILETIME __fastcall DateTimeToFileTime(const TDateTime DateTime,
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
  (*(__int64*)&(Result) = (__int64(UnixTimeStamp) + 11644473600LL) * 10000000LL);

  return Result;
}
//---------------------------------------------------------------------------
TDateTime __fastcall FileTimeToDateTime(const FILETIME & FileTime)
{
  // duplicated in DirView.pas
  SYSTEMTIME SysTime;
  if (!UsesDaylightHack())
  {
    SYSTEMTIME UniverzalSysTime;
    FileTimeToSystemTime(&FileTime, &UniverzalSysTime);
    SystemTimeToTzSpecificLocalTime(NULL, &UniverzalSysTime, &SysTime);
  }
  else
  {
    FILETIME LocalFileTime;
    FileTimeToLocalFileTime(&FileTime, &LocalFileTime);
    FileTimeToSystemTime(&LocalFileTime, &SysTime);
  }
  TDateTime Result = SystemTimeToDateTime(SysTime);
  return Result;
}
//---------------------------------------------------------------------------
__int64 __fastcall ConvertTimestampToUnix(const FILETIME & FileTime,
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
      TDateTime DateTime = SystemTimeToDateTime(SystemTime);
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
      TDateTime DateTime = SystemTimeToDateTime(SystemTime);
      const TDateTimeParams * Params = GetDateTimeParams(DecodeYear(DateTime));
      Result -= (IsDateInDST(DateTime) ?
        Params->DaylightDifferenceSec : Params->StandardDifferenceSec);
    }
  }

  return Result;
}
//---------------------------------------------------------------------------
TDateTime __fastcall ConvertTimestampToUTC(TDateTime DateTime)
{

  const TDateTimeParams * CurrentParams = GetDateTimeParams(0);
  DateTime += CurrentParams->CurrentDifference;

  const TDateTimeParams * Params = GetDateTimeParams(DecodeYear(DateTime));
  DateTime +=
    (IsDateInDST(DateTime) ?
      Params->DaylightDifference : Params->StandardDifference);

  return DateTime;
}
//---------------------------------------------------------------------------
__int64 __fastcall ConvertTimestampToUnixSafe(const FILETIME & FileTime,
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
TDateTime __fastcall AdjustDateTimeFromUnix(TDateTime DateTime, TDSTMode DSTMode)
{
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
      if (IsDateInDST(DateTime))
      {
        DateTime = DateTime + Params->DaylightDifference;
      }
      else
      {
        DateTime = DateTime + Params->StandardDifference;
      }
    }
  }

  return DateTime;
}
//---------------------------------------------------------------------------
UnicodeString __fastcall FixedLenDateTimeFormat(const UnicodeString & Format)
{
  UnicodeString Result = Format;
  bool AsIs = false;

  int Index = 1;
  while (Index <= Result.Length())
  {
    wchar_t F = Result[Index];
    if ((F == L'\'') || (F == L'\"'))
    {
      AsIs = !AsIs;
      Index++;
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
        Index++;
      }
    }
    else
    {
      if (!AsIs && (wcschr(L"dDeEmMhHnNsS", F) != NULL) &&
          ((Index == Result.Length()) || (Result[Index + 1] != F)))
      {
        Result.Insert(F, Index);
      }

      while ((Index <= Result.Length()) && (F == Result[Index]))
      {
        Index++;
      }
    }
  }

  return Result;
}
//---------------------------------------------------------------------------
static TDateTime TwoSeconds(0, 0, 2, 0);
int __fastcall CompareFileTime(TDateTime T1, TDateTime T2)
{
  // "FAT" time precision
  // (when one time is seconds-precision and other is millisecond-precision,
  // we may have times like 12:00:00.000 and 12:00:01.999, which should
  // be treated the same)
  int Result;
  if (T1 == T2)
  {
    // just optimalisation
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
bool __fastcall RecursiveDeleteFile(const UnicodeString FileName, bool ToRecycleBin)
{
  SHFILEOPSTRUCT Data;

  memset(&Data, 0, sizeof(Data));
  Data.hwnd = NULL;
  Data.wFunc = FO_DELETE;
  UnicodeString FileList(FileName);
  FileList.SetLength(FileList.Length() + 2);
  FileList[FileList.Length() - 1] = L'\0';
  FileList[FileList.Length()] = L'\0';
  Data.pFrom = FileList.c_str();
  Data.pTo = L"";
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
unsigned int __fastcall CancelAnswer(unsigned int Answers)
{
  unsigned int Result;
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
unsigned int __fastcall AbortAnswer(unsigned int Answers)
{
  unsigned int Result;
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
unsigned int __fastcall ContinueAnswer(unsigned int Answers)
{
  unsigned int Result;
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
#ifndef _MSC_VER
TLibModule * __fastcall FindModule(void * Instance)
{
  TLibModule * CurModule;
  CurModule = reinterpret_cast<TLibModule*>(LibModuleList);

  while (CurModule)
  {
    if (CurModule->instance == (long)Instance)
    {
      break;
    }
    else
    {
      CurModule = CurModule->next;
    }
  }
  return CurModule;
}
#endif
//---------------------------------------------------------------------------
UnicodeString __fastcall LoadStr(int Ident, unsigned int MaxLength)
{
  TLibModule * MainModule = FindModule(HInstance);
  assert(MainModule != NULL);

  UnicodeString Result;
  Result.SetLength(MaxLength);
#ifndef _MSC_VER
  int Length = LoadString((HINSTANCE)MainModule->resinstance, Ident, Result.c_str(), MaxLength);
#else
  HINSTANCE hInstance = FarPlugin ? FarPlugin->GetHandle() : GetModuleHandle(0);
  // DEBUG_PRINTF(L"hInstance = %u", hInstance);
  assert(hInstance != 0);

  Result.SetLength(MaxLength > 0 ? MaxLength : 255);
  size_t Length = ::LoadString(hInstance, Ident, reinterpret_cast<LPWSTR>(const_cast<wchar_t *>(Result.c_str())),
                               static_cast<int>(Result.Length()));
#endif
  Result.SetLength(Length);

  return Result;
}
//---------------------------------------------------------------------------
UnicodeString __fastcall LoadStrPart(int Ident, int Part)
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
UnicodeString __fastcall DecodeUrlChars(UnicodeString S)
{
  int i = 1;
  while (i <= S.Length())
  {
    switch (S[i])
    {
      case L'+':
        S[i] = ' ';
        break;

      case L'%':
        if (i <= S.Length() - 2)
        {
          unsigned char B = HexToByte(S.SubString(i + 1, 2));
          if (B > 0)
          {
            S[i] = (wchar_t)B;
            S.Delete(i + 1, 2);
          }
        }
        break;
    }
    i++;
  }
  return S;
}
//---------------------------------------------------------------------------
UnicodeString __fastcall DoEncodeUrl(UnicodeString S, UnicodeString Chars)
{
  int i = 1;
  while (i <= S.Length())
  {
    if (Chars.Pos(S[i]) > 0)
    {
      UnicodeString H = CharToHex(S[i]);
      S.Insert(H, i + 1);
      S[i] = '%';
      i += H.Length();
    }
    i++;
  }
  return S;
}
//---------------------------------------------------------------------------
UnicodeString __fastcall EncodeUrlChars(UnicodeString S, UnicodeString Ignore)
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
UnicodeString __fastcall NonUrlChars()
{
  UnicodeString S;
  for (unsigned int I = 0; I <= 127; I++)
  {
    wchar_t C = static_cast<wchar_t>(I);
    if (((C >= L'a') && (C <= L'z')) ||
        ((C >= L'A') && (C <= L'Z')) ||
        ((C >= L'0') && (C <= L'9')) ||
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
UnicodeString __fastcall EncodeUrlString(UnicodeString S)
{
  return DoEncodeUrl(S, NonUrlChars());
}
//---------------------------------------------------------------------------
UnicodeString __fastcall EscapeHotkey(const UnicodeString & Caption)
{
  return StringReplace(Caption, L"&", L"&&", TReplaceFlags::Init(rfReplaceAll));
}
//---------------------------------------------------------------------------
// duplicated in console's Main.cpp
bool __fastcall CutToken(UnicodeString & Str, UnicodeString & Token)
{
  bool Result;

  Token = L"";

  // inspired by Putty's sftp_getcmd() from PSFTP.C
  int Index = 1;
  while ((Index <= Str.Length()) &&
    ((Str[Index] == L' ') || (Str[Index] == L'\t')))
  {
    Index++;
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
        Index++;
        Quoting = !Quoting;
      }
      else
      {
        Token += Str[Index];
        Index++;
      }
    }

    if (Index <= Str.Length())
    {
      Index++;
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
void __fastcall AddToList(UnicodeString & List, const UnicodeString & Value, const UnicodeString & Delimiter)
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
bool __fastcall Is2000()
{
  return (Win32MajorVersion >= 5);
}
//---------------------------------------------------------------------------
bool __fastcall IsWin7()
{
  return
    (Win32MajorVersion > 6) ||
    ((Win32MajorVersion == 6) && (Win32MinorVersion >= 1));
}
//---------------------------------------------------------------------------
bool __fastcall IsExactly2008R2()
{
  HINSTANCE Kernel32 = GetModuleHandle(kernel32);
  typedef BOOL WINAPI (* TGetProductInfo)(DWORD, DWORD, DWORD, DWORD, PDWORD);
  TGetProductInfo GetProductInfo =
      (TGetProductInfo)GetProcAddress(Kernel32, "GetProductInfo");
  bool Result;
  if (GetProductInfo == NULL)
  {
    Result = false;
  }
  else
  {
    DWORD Type;
    GetProductInfo(Win32MajorVersion, Win32MinorVersion, 0, 0, &Type);
    switch (Type)
    {
      case 0x0008 /*PRODUCT_DATACENTER_SERVER*/:
      case 0x000C /*PRODUCT_DATACENTER_SERVER_CORE}*/:
      case 0x0027 /*PRODUCT_DATACENTER_SERVER_CORE_V*/:
      case 0x0025 /*PRODUCT_DATACENTER_SERVER_V*/:
      case 0x000A /*PRODUCT_ENTERPRISE_SERVE*/:
      case 0x000E /*PRODUCT_ENTERPRISE_SERVER_COR*/:
      case 0x0029 /*PRODUCT_ENTERPRISE_SERVER_CORE_*/:
      case 0x000F /*PRODUCT_ENTERPRISE_SERVER_IA6*/:
      case 0x0026 /*PRODUCT_ENTERPRISE_SERVER_*/:
      case 0x002A /*PRODUCT_HYPER*/:
      case 0x001E /*PRODUCT_MEDIUMBUSINESS_SERVER_MANAGEMEN*/:
      case 0x0020 /*PRODUCT_MEDIUMBUSINESS_SERVER_MESSAGIN*/:
      case 0x001F /*PRODUCT_MEDIUMBUSINESS_SERVER_SECURIT*/:
      case 0x0018 /*PRODUCT_SERVER_FOR_SMALLBUSINES*/:
      case 0x0023 /*PRODUCT_SERVER_FOR_SMALLBUSINESS_*/:
      case 0x0021 /*PRODUCT_SERVER_FOUNDATIO*/:
      case 0x0009 /*PRODUCT_SMALLBUSINESS_SERVE*/:
      case 0x0038 /*PRODUCT_SOLUTION_EMBEDDEDSERVE*/:
      case 0x0007 /*PRODUCT_STANDARD_SERVE*/:
      case 0x000D /*PRODUCT_STANDARD_SERVER_COR*/:
      case 0x0028 /*PRODUCT_STANDARD_SERVER_CORE_*/:
      case 0x0024 /*PRODUCT_STANDARD_SERVER_*/:
      case 0x0017 /*PRODUCT_STORAGE_ENTERPRISE_SERVE*/:
      case 0x0014 /*PRODUCT_STORAGE_EXPRESS_SERVE*/:
      case 0x0015 /*PRODUCT_STORAGE_STANDARD_SERVE*/:
      case 0x0016 /*PRODUCT_STORAGE_WORKGROUP_SERVE*/:
      case 0x0011 /*PRODUCT_WEB_SERVE*/:
      case 0x001D /*PRODUCT_WEB_SERVER_COR*/:
        Result = true;
        break;

      default:
        Result = false;
        break;
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
LCID __fastcall GetDefaultLCID()
{
  return Is2000() ? GetUserDefaultLCID() : GetThreadLocale();
}
//---------------------------------------------------------------------------
#ifndef _MSC_VER
// Suppress warning about unused constants in DateUtils.hpp
#pragma warn -8080
#endif
//---------------------------------------------------------------------------
UnicodeString IntToStr(int value)
{
  std::string result = boost::lexical_cast<std::string>(value);
  return System::MB2W(result.c_str());
}
//---------------------------------------------------------------------------
UnicodeString Int64ToStr(__int64 value)
{
  std::string result = boost::lexical_cast<std::string>(value);
  return System::MB2W(result.c_str());
}
//---------------------------------------------------------------------------
int StrToInt(const UnicodeString value)
{
  __int64 Value = 0;
  if (TryStrToInt(value, Value))
  {
    return static_cast<int>(Value);
  }
  else
  {
    return 0;
  }
}

__int64 ToInt(const UnicodeString value)
{
  __int64 Value = 0;
  if (TryStrToInt(value, Value))
  {
    return Value;
  }
  else
  {
    return 0;
  }
}

int StrToIntDef(const UnicodeString value, int defval)
{
  __int64 Value = 0;
  if (TryStrToInt(value, Value))
  {
    return static_cast<int>(Value);
  }
  else
  {
    return defval;
  }
}

__int64 StrToInt64(const UnicodeString value)
{
  return ToInt(value);
}

__int64 StrToInt64Def(const UnicodeString value, __int64 defval)
{
  __int64 Value = 0;
  if (TryStrToInt(value, Value))
  {
    return Value;
  }
  else
  {
    return defval;
  }
}

bool TryStrToInt(const UnicodeString value, __int64 & Value)
{
  bool result = false;
  try
  {
    Value = boost::lexical_cast<__int64>(value);
    result = true;
  }
  catch (const boost::bad_lexical_cast &)
  {
    result = false;
  }
  return result;
}

bool TryStrToInt(const UnicodeString value, int & Value)
{
  bool result = false;
  try
  {
    Value = boost::lexical_cast<int>(value);
    result = true;
  }
  catch (const boost::bad_lexical_cast &)
  {
    result = false;
  }
  return result;
}

//---------------------------------------------------------------------------

UnicodeString Trim(const UnicodeString str)
{
  UnicodeString result = TrimRight(TrimLeft(str));
  return result;
}

UnicodeString TrimLeft(const UnicodeString str)
{
  UnicodeString result = str;
  while (result.Length() > 0 && result[0] == ' ')
  {
    result = result.SubString(1, result.Length() - 1);
  }
  return result;
}

UnicodeString TrimRight(const UnicodeString str)
{
  UnicodeString result = str;
  while (result.Length() > 0 &&
         ((result[result.Length() - 1] == ' ') || (result[result.Length() - 1] == '\n')))
  {
    result.SetLength(result.Length() - 1);
  }
  return result;
}

UnicodeString UpperCase(const UnicodeString str)
{
  UnicodeString result;
  result.SetLength(str.Length());
  std::transform(str.begin(), str.end(), result.begin(), ::toupper);
  return result;
}

UnicodeString LowerCase(const UnicodeString str)
{
  UnicodeString result;
  result.SetLength(str.Length());
  std::transform(str.begin(), str.end(), result.begin(), ::tolower);
  return result;
}

//---------------------------------------------------------------------------

wchar_t UpCase(const wchar_t c)
{
  return c;
}

wchar_t LowCase(const wchar_t c)
{
  return c;
}

//---------------------------------------------------------------------------

UnicodeString AnsiReplaceStr(const UnicodeString str, const UnicodeString from, const UnicodeString to)
{
  UnicodeString result = str;
  alg::replace_all(result, from, to);
  return result;
}

size_t AnsiPos(const UnicodeString str, wchar_t c)
{
  size_t result = str.find_first_of(c);
  return result;
}

size_t Pos(const UnicodeString str, const UnicodeString substr)
{
  size_t result = str.Pos(substr);
  return result;
}

UnicodeString StringReplace(const UnicodeString str, const UnicodeString from, const UnicodeString to, TReplaceFlags Flags)
{
  return AnsiReplaceStr(str, from, to);
}

bool IsDelimiter(const UnicodeString str, const UnicodeString delim, size_t index)
{
  if (index < str.Length())
  {
    wchar_t c = str[index];
    for (size_t i = 0; i < delim.Length(); i++)
    {
      if (delim[i] == c)
      {
        return true;
      }
    }
  }
  return false;
}

size_t LastDelimiter(const UnicodeString str, const UnicodeString delim)
{
  if (str.Length())
  {
    for (size_t i = str.Length() - 1; i != UnicodeString::npos; --i)
    {
      if (::IsDelimiter(str, delim, i))
      {
        return i;
      }
    }
  }
  return UnicodeString::npos;
}

//---------------------------------------------------------------------------

int StringCmp(const wchar_t * s1, const wchar_t * s2)
{
  return ::CompareString(0, SORT_STRINGSORT, s1, -1, s2, -1) - 2;
}

int StringCmpI(const wchar_t * s1, const wchar_t * s2)
{
  return ::CompareString(0, NORM_IGNORECASE | SORT_STRINGSORT, s1, -1, s2, -1) - 2;
}

//---------------------------------------------------------------------------

int CompareText(const UnicodeString str1, const UnicodeString str2)
{
  return StringCmp(str1.c_str(), str2.c_str());
}

int AnsiCompare(const UnicodeString str1, const UnicodeString str2)
{
  return StringCmp(str1.c_str(), str2.c_str());
}

// Case-sensitive compare
int AnsiCompareStr(const UnicodeString str1, const UnicodeString str2)
{
  return StringCmp(str1.c_str(), str2.c_str());
}

bool AnsiSameText(const UnicodeString str1, const UnicodeString str2)
{
  return StringCmp(str1.c_str(), str2.c_str()) == 0;
}

bool SameText(const UnicodeString str1, const UnicodeString str2)
{
  return AnsiSameText(str1, str2) == 0;
}

int AnsiCompareText(const UnicodeString str1, const UnicodeString str2)
{
  return StringCmpI(str1.c_str(), str2.c_str());
}

int AnsiCompareIC(const UnicodeString str1, const UnicodeString str2)
{
  return AnsiCompareText(str1, str2);
}

bool AnsiContainsText(const UnicodeString str1, const UnicodeString str2)
{
  return ::Pos(str1, str2) != NPOS;
}

void RaiseLastOSError()
{
  int LastError = ::GetLastError();
  UnicodeString ErrorMsg;
  if (LastError != 0)
  {
    ErrorMsg = FMTLOAD(SOSError, LastError, ::SysErrorMessage(LastError).c_str());
  }
  else
  {
    ErrorMsg = FMTLOAD(SUnkOSError);
  }
  throw EOSError(ErrorMsg, LastError);
}

//---------------------------------------------------------------------------
double StrToFloat(const UnicodeString Value)
{
  return StrToFloatDef(Value, 0.0);
}
//---------------------------------------------------------------------------
double StrToFloatDef(const UnicodeString Value, double defval)
{
  double result = 0.0;
  try
  {
    result = boost::lexical_cast<double>(System::W2MB(Value.c_str()));
  }
  catch (const boost::bad_lexical_cast &)
  {
    result = defval;
  }
  return result;
}
//---------------------------------------------------------------------------
UnicodeString FormatFloat(const UnicodeString Format, double value)
{
  // DEBUG_PRINTF(L"Format = %s", Format.c_str());
  // #,##0 "B"
  UnicodeString result(20, 0);
  swprintf_s(&result[0], result.Length(), L"%.2f", value);
  return result.c_str();
}

//---------------------------------------------------------------------------
System::TTimeStamp DateTimeToTimeStamp(System::TDateTime DateTime)
{
  System::TTimeStamp result = {0, 0};
  double fractpart, intpart;
  fractpart = modf(DateTime, &intpart);
  result.Time = static_cast<int>(fractpart * MSecsPerDay);
  result.Date = static_cast<int>(intpart + DateDelta);
  // DEBUG_PRINTF(L"DateTime = %f, time = %u, Date = %u", DateTime, result.Time, result.Date);
  return result;
}

//---------------------------------------------------------------------------

__int64 FileRead(HANDLE Handle, void * Buffer, __int64 Count)
{
  __int64 Result = -1;
  // DEBUG_PRINTF(L"Handle = %d, Count = %d", Handle, Count);
  DWORD res = 0;
  if (::ReadFile(Handle, reinterpret_cast<LPVOID>(Buffer), static_cast<DWORD>(Count), &res, NULL))
  {
    Result = res;
  }
  else
  {
    Result = -1;
  }
  // DEBUG_PRINTF(L"Result = %d, Handle = %d, Count = %d", (int)Result, Handle, Count);
  return Result;
}

__int64 FileWrite(HANDLE Handle, const void * Buffer, __int64 Count)
{
  __int64 Result = -1;
  DWORD res = 0;
  if (::WriteFile(Handle, Buffer, static_cast<DWORD>(Count), &res, NULL))
  {
    Result = res;
  }
  else
  {
    Result = -1;
  }
  // DEBUG_PRINTF(L" Result = %d, Handle = %d, Count = %d", (int)Result, Handle, Count);
  return Result;
}

//---------------------------------------------------------------------------

bool FileExists(const UnicodeString fileName)
{
  return GetFileAttributes(fileName.c_str()) != 0xFFFFFFFF;
}

bool RenameFile(const UnicodeString from, const UnicodeString to)
{
  bool Result = ::MoveFile(from.c_str(), to.c_str());
  return Result;
}

bool DirectoryExists(const UnicodeString filename)
{
  // DEBUG_PRINTF(L"filename = %s", filename.c_str());
  if ((filename == THISDIRECTORY) || (filename == PARENTDIRECTORY))
  {
    return true;
  }

  int attr = GetFileAttributes(filename.c_str());
  // DEBUG_PRINTF(L"attr = %d, FILE_ATTRIBUTE_DIRECTORY = %d", attr, FILE_ATTRIBUTE_DIRECTORY);

  if ((attr != 0xFFFFFFFF) && FLAGSET(attr, FILE_ATTRIBUTE_DIRECTORY))
  {
    return true;
  }
  return false;
}

UnicodeString FileSearch(const UnicodeString FileName, const UnicodeString DirectoryList)
{
  // DEBUG_PRINTF(L"FileName = %s, DirectoryList = %s", FileName.c_str(), DirectoryList.c_str());
  size_t i;
  UnicodeString Temp;
  UnicodeString Result;
  Temp = DirectoryList;
  wchar_t PathSeparator = L'\\';
  UnicodeString PathSeparators = L"/\\";
  do
  {
    i = ::Pos(Temp, PathSeparators);
    while ((Temp.Length() > 0) && (i == 0))
    {
      Temp.Delete(0, 1);
      i = ::Pos(Temp, PathSeparators);
    }
    i = ::Pos(Temp, PathSeparators);
    if (i != UnicodeString::npos)
    {
      Result = Temp.SubString(0, i - 1);
      Temp.Delete(0, i);
      // DEBUG_PRINTF(L"Result = %s, Temp = %s", Result.c_str(), Temp.c_str());
    }
    else
    {
      Result = Temp;
      Temp = L"";
    }
    Result = ::IncludeTrailingBackslash(Result);
    Result = Result + FileName;
    if (!::FileExists(Result))
    {
      Result = L"";
    }
  }
  while (!(Temp.Length() == 0) || (Result.Length() != 0));
  // DEBUG_PRINTF(L"Result = %s", Result.c_str());
  return Result;
}


int FileGetAttr(const UnicodeString filename)
{
  int attr = GetFileAttributes(filename.c_str());
  return attr;
}

int FileSetAttr(const UnicodeString filename, int attrs)
{
  int res = SetFileAttributes(filename.c_str(), attrs);
  return res;
}

bool CreateDir(const UnicodeString Dir)
{
  // DEBUG_PRINTF(L"Dir = %s", Dir.c_str());
  return ::CreateDirectory(Dir.c_str(), NULL) != 0;
}

bool RemoveDir(const UnicodeString Dir)
{
  return ::RemoveDirectory(Dir.c_str()) != 0;
}

bool ForceDirectories(const UnicodeString Dir)
{
  // DEBUG_PRINTF(L"Dir = %s", Dir.c_str());
  bool Result = true;
  if (Dir.IsEmpty())
  {
    return false;
  }
  UnicodeString Dir2 = ExcludeTrailingBackslash(Dir);
  // DEBUG_PRINTF(L"Dir2 = %s", Dir2.c_str());
  if ((Dir2.Length() < 3) || DirectoryExists(Dir2))
  {
    return Result;
  }
  if (ExtractFilePath(Dir2).IsEmpty())
  {
    return ::CreateDir(Dir2);
  }
  Result = ForceDirectories(ExtractFilePath(Dir2)) && CreateDir(Dir2);
  // DEBUG_PRINTF(L"Result = %d", Result);
  return Result;
}

bool DeleteFile(const UnicodeString File)
{
  // DEBUG_PRINTF(L"File = %s, FileExists(File) = %d", File.c_str(), ::FileExists(File));
  ::DeleteFile(File.c_str());
  // DEBUG_PRINTF(L"FileExists(File) = %d", ::FileExists(File));
  return !::FileExists(File);
}

//---------------------------------------------------------------------------
// bool InheritsFrom(const std::exception &E1, const std::exception &from)
// {
// return false;
// }

//---------------------------------------------------------------------------

UnicodeString Format(const wchar_t * format, ...)
{
  UnicodeString result;
  va_list args;
  va_start(args, format);
  result = ::Format(format, args);
  va_end(args);
  return result.c_str();
}

//---------------------------------------------------------------------------

UnicodeString Format(const wchar_t * format, va_list args)
{
  UnicodeString result;
  if (format && *format)
  {
    size_t len = _vscwprintf(format, args);
    result.SetLength(len + 1);
    vswprintf_s(&result[0], len + 1, format, args);
  }
  return result.c_str();
}

//---------------------------------------------------------------------------
UnicodeString FmtLoadStr(int id, ...)
{
  // DEBUG_PRINTF(L"begin: id = %d", id)
  UnicodeString result;
  UnicodeString format;
  HINSTANCE hInstance = FarPlugin ? FarPlugin->GetHandle() : GetModuleHandle(0);
  // DEBUG_PRINTF(L"hInstance = %u", hInstance);
  format.SetLength(255);
  size_t Length = ::LoadString(hInstance, id, reinterpret_cast<LPWSTR>(const_cast<wchar_t *>(format.c_str())), static_cast<int>(format.Length()));
  format.SetLength(Length);
  // DEBUG_PRINTF(L"format = %s", format.c_str());
  if (!Length)
  {
    // TRACE(_T("Unknown resource string id : %d"), id);
    DEBUG_PRINTF(L"Unknown resource string id: %d\n", id);
  }
  else
  {
    va_list args;
    va_start(args, id);
    /*
    LPTSTR lpszTemp;
    if (::FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                        (LPCVOID)format.c_str(), 0, 0, (LPTSTR)&lpszTemp, 0, &args) == 0 ||
        lpszTemp == NULL)
    {
      // AfxThrowMemoryException();
        DEBUG_PRINTF(L"FormatMessage error");
    }
    // DEBUG_PRINTF(L"lpszTemp = %s", lpszTemp);
    result = lpszTemp;
    ::LocalFree(lpszTemp);
    */
    size_t len = _vscwprintf(format.c_str(), args);
    UnicodeString buf(len + sizeof(wchar_t), 0);
    vswprintf_s(&buf[0], buf.Length(), format.c_str(), args);
    va_end(args);
    result = buf;
  }
  // DEBUG_PRINTF(L"result = %s", result.c_str());
  return result;
}
//---------------------------------------------------------------------------
UnicodeString WrapText(const UnicodeString Line, int MaxCol)
{
  UnicodeString Result = Line;
  /*
  Col := 1;
  Pos := 1;
  LinePos := 1;
  BreakPos := 0;
  QuoteChar := ' ';
  ExistingBreak := False;
  LineLen := Length(Line);
  BreakLen := Length(BreakStr);
  Result := '';
  while Pos <= LineLen do
  begin
  CurChar := Line[Pos];
  if CurChar in LeadBytes then
  begin
    L := CharLength(Line, Pos) - 1;
    Inc(Pos, L);
    Inc(Col, L);
  end
  else
  begin
    if CurChar = BreakStr[1] then
    begin
      if QuoteChar = ' ' then
      begin
        ExistingBreak := CompareText(BreakStr, Copy(Line, Pos, BreakLen)) = 0;
        if ExistingBreak then
        begin
          Inc(Pos, BreakLen-1);
          BreakPos := Pos;
        end;
      end
    end
    else if CurChar in BreakChars then
    begin
      if QuoteChar = ' ' then BreakPos := Pos
    end
    else if CurChar in QuoteChars then
    begin
      if CurChar = QuoteChar then
        QuoteChar := ' '
      else if QuoteChar = ' ' then
        QuoteChar := CurChar;
    end;
  end;
  Inc(Pos);
  Inc(Col);
  if not (QuoteChar in QuoteChars) and (ExistingBreak or
    ((Col > MaxCol) and (BreakPos > LinePos))) then
  begin
    Col := Pos - BreakPos;
    Result := Result + Copy(Line, LinePos, BreakPos - LinePos + 1);
    if not (CurChar in QuoteChars) then
      while Pos <= LineLen do
      begin
        if Line[Pos] in BreakChars then
          Inc(Pos)
        else if Copy(Line, Pos, Length(sLineBreak)) = sLineBreak then
          Inc(Pos, Length(sLineBreak))
        else
          break;
      end;
    if not ExistingBreak and (Pos < LineLen) then
      Result := Result + BreakStr;
    Inc(BreakPos);
    LinePos := BreakPos;
    ExistingBreak := False;
  end;
  end;
  Result := Result + Copy(Line, LinePos, MaxInt);
  */
  return Result;
}

//---------------------------------------------------------------------------
UnicodeString TranslateExceptionMessage(const std::exception * E)
{
  if (E)
  {
    return System::MB2W(E->what());
  }
  else
  {
    return UnicodeString();
  }
}
//---------------------------------------------------------------------------
void AppendWChar(UnicodeString & str, const wchar_t ch)
{
  if (!str.IsEmpty() && str[str.length() - 1] != ch)
  {
    str += ch;
  }
}

void AppendChar(std::string & str, const char ch)
{
  if (!str.IsEmpty() && str[str.length() - 1] != ch)
  {
    str += ch;
  }
}

void AppendPathDelimiterW(UnicodeString & str)
{
  if (!str.IsEmpty() && str[str.length() - 1] != L'/' && str[str.length() - 1] != L'\\')
  {
    str += L"\\";;
  }
}

void AppendPathDelimiterA(std::string & str)
{
  if (!str.IsEmpty() && str[str.length() - 1] != '/' && str[str.length() - 1] != '\\')
  {
    str += "\\";;
  }
}

//---------------------------------------------------------------------------

UnicodeString ExpandEnvVars(const UnicodeString & str)
{
  wchar_t buf[MAX_PATH];
  unsigned size = ExpandEnvironmentStringsW(str.c_str(), buf, static_cast<DWORD>(sizeof(buf) - 1));
  UnicodeString result = UnicodeString(buf, size - 1);
  // DEBUG_PRINTF(L"result = %s", result.c_str());
  return result;
}

UnicodeString StringOfChar(const wchar_t c, size_t len)
{
  UnicodeString result;
  if (int(len) < 0) len = 0;
  result.SetLength(len, c);
  return result;
}

// void RaiseLastOSError()
// {
// }

char * StrNew(const char * str)
{
  const size_t sz = strlen(str) + 1;
  char * Result = new char[sz];
  strncpy_s(Result, sz, str, sz);
  return Result;
}

wchar_t * AnsiStrScan(const wchar_t * Str, const wchar_t TokenPrefix)
{
  System::Error(SNotImplemented, 31);
  wchar_t * result = NULL;
  return result;
}

UnicodeString ChangeFileExt(const UnicodeString FileName, const UnicodeString ext)
{
  UnicodeString result = ::ChangeFileExtension(FileName, ext);
  return result;
}

UnicodeString ExtractFileExt(const UnicodeString FileName)
{
  UnicodeString Result = ExtractFileExtension(FileName, L'.');
  return Result;
}

UnicodeString get_full_path_name(const UnicodeString path)
{
  UnicodeString buf(MAX_PATH, 0);
  size_t size = GetFullPathNameW(path.c_str(), static_cast<DWORD>(buf.Length() - 1),
                                 reinterpret_cast<LPWSTR>(const_cast<wchar_t *>(buf.c_str())), NULL);
  if (size > buf.Length())
  {
    buf.SetLength(size);
    size = GetFullPathNameW(path.c_str(), static_cast<DWORD>(buf.Length() - 1), reinterpret_cast<LPWSTR>(const_cast<wchar_t *>(buf.c_str())), NULL);
  }
  return UnicodeString(buf.c_str(), size);
}

UnicodeString ExpandFileName(const UnicodeString FileName)
{
  UnicodeString Result;
  Result = get_full_path_name(FileName);
  return Result;
}

UnicodeString GetUniversalName(UnicodeString & FileName)
{
  UnicodeString Result = FileName;
  return Result;
}

UnicodeString ExpandUNCFileName(const UnicodeString FileName)
{
  UnicodeString Result = ExpandFileName(FileName);
  if ((Result.Length() >= 3) && (Result[1] == L':') && (::UpCase(Result[0]) >= 'A')
      && (::UpCase(Result[0]) <= 'Z'))
  {
    Result = GetUniversalName(Result);
  }
  return Result;
}

__int64 FileSeek(HANDLE file, __int64 offset, int Origin)
{
  LONG low = offset & 0xFFFFFFFF;
  LONG high = offset >> 32;
  low = ::SetFilePointer(file, low, &high, static_cast<DWORD>(Origin));
  return ((_int64)high << 32) + low;
}

void InitPlatformId()
{
  OSVERSIONINFO OSVersionInfo;
  OSVersionInfo.dwOSVersionInfoSize = sizeof(OSVersionInfo);
  if (GetVersionEx(&OSVersionInfo) != 0)
  {
    Win32Platform = OSVersionInfo.dwPlatformId;
    Win32MajorVersion = OSVersionInfo.dwMajorVersion;
    Win32MinorVersion = OSVersionInfo.dwMinorVersion;
    Win32BuildNumber = OSVersionInfo.dwBuildNumber;
    // Win32CSDVersion = OSVersionInfo.szCSDVersion;
  }
}

//---------------------------------------------------------------------------
bool Win32Check(bool RetVal)
{
  if (!RetVal)
  {
    RaiseLastOSError();
  }
  return RetVal;
}
//---------------------------------------------------------------------------

UnicodeString SysErrorMessage(int ErrorCode)
{
  UnicodeString Result;
  wchar_t Buffer[255];
  int Len = ::FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |
                            FORMAT_MESSAGE_ARGUMENT_ARRAY, NULL, ErrorCode, 0,
                            static_cast<LPTSTR>(Buffer),
                            sizeof(Buffer), NULL);
  while ((Len > 0) && ((Buffer[Len - 1] != 0) &&
                       (Buffer[Len - 1] <= 32) || (Buffer[Len - 1] == '.')))
  {
    Len--;
  }
  // SetString(Result, Buffer, Len);
  Result = UnicodeString(Buffer, Len);
  return Result;
}

UnicodeString ReplaceStrAll(const UnicodeString Str, const UnicodeString What, const UnicodeString ByWhat)
{
  UnicodeString result = Str;
  size_t pos = result.Pos(What);
  while (pos != UnicodeString::npos)
  {
    result.replace(pos, What.Length(), ByWhat);
    pos = result.Pos(What);
  }
  return result;
}

UnicodeString ExtractShortPathName(const UnicodeString Path1)
{
  // FIXME
  return Path1;
}

//
// Returns everything, including the trailing path separator, except the filename
// part of the path.
//
// "/foo/bar/baz.txt" --> "/foo/bar/"
UnicodeString ExtractDirectory(const UnicodeString path, wchar_t delimiter)
{
  return path.SubString(0,path.find_last_of(delimiter) + 1);
}

//
// Returns only the filename part of the path.
//
// "/foo/bar/baz.txt" --> "baz.txt"
UnicodeString ExtractFilename(const UnicodeString path, wchar_t delimiter)
{
  return path.SubString(path.find_last_of(delimiter) + 1);
}

//
// Returns the file's extension, if any. The period is considered part
// of the extension.
//
// "/foo/bar/baz.txt" --> ".txt"
// "/foo/bar/baz" --> ""
UnicodeString ExtractFileExtension(const UnicodeString path, wchar_t delimiter)
{
  UnicodeString filename = ExtractFilename(path, delimiter);
  UnicodeString::size_type n = filename.find_last_of('.');
  if (n != UnicodeString::npos)
  {
    return filename.SubString(n);
  }
  return UnicodeString();
}

//
// Modifies the filename's extension. The period is considered part
// of the extension.
//
// "/foo/bar/baz.txt", ".dat" --> "/foo/bar/baz.dat"
// "/foo/bar/baz.txt", "" --> "/foo/bar/baz"
// "/foo/bar/baz", ".txt" --> "/foo/bar/baz.txt"
//
UnicodeString ChangeFileExtension(const UnicodeString path, const UnicodeString ext, wchar_t delimiter)
{
  UnicodeString filename = ExtractFilename(path, delimiter);
  return ExtractDirectory(path, delimiter)
         + filename.SubString(0, filename.find_last_of('.'))
         + ext;
}

//---------------------------------------------------------------------------

UnicodeString ExcludeTrailingBackslash(const UnicodeString str)
{
  UnicodeString result = str;
  if ((str.Length() > 0) && ((str[str.Length() - 1] == L'/') ||
                             (str[str.Length() - 1] == L'\\')))
  {
    result.SetLength(result.Length() - 1);
  }
  return result;
}

UnicodeString IncludeTrailingBackslash(const UnicodeString str)
{
  UnicodeString result = str;
  if ((str.Length() == 0) || ((str[str.Length() - 1] != L'/') &&
                              (str[str.Length() - 1] != L'\\')))
  {
    result += L'\\';
  }
  return result;
}

UnicodeString ExtractFileDir(const UnicodeString str)
{
  UnicodeString result;
  size_t Pos = ::LastDelimiter(str, L"/\\");
  // DEBUG_PRINTF(L"Pos = %d", Pos);
  // it used to return Path when no slash was found
  if (Pos != UnicodeString::npos)
  {
    result = str.SubString(0, Pos + 1);
  }
  else
  {
    result = (Pos == 0) ? UnicodeString(L"/") : UnicodeString();
  }
  return result;
}

UnicodeString ExtractFilePath(const UnicodeString str)
{
  UnicodeString result = ::ExtractFileDir(str);
  // DEBUG_PRINTF(L"str = %s, result = %s", str.c_str(), result.c_str());
  return result;
}

UnicodeString GetCurrentDir()
{
  UnicodeString result;
  wchar_t path[MAX_PATH + 1];
  if (FarPlugin)
  {
    FarPlugin->GetFarStandardFunctions().GetCurrentDirectory(sizeof(path), path);
  }
  else
  {
    ::GetCurrentDirectory(sizeof(path), path);
  }
  // DEBUG_PRINTF(L"path = %s", path);
  result = path;
  return result;
}

//---------------------------------------------------------------------------
class EConvertError : public ExtException
{
  typedef ExtException parent;
public:
  EConvertError(const UnicodeString Msg) :
    parent(Msg, NULL)
  {}
};

//---------------------------------------------------------------------------
void ConvertError(int ErrorID)
{
  UnicodeString Msg = FMTLOAD(ErrorID, 0);
  throw EConvertError(Msg);
}

//---------------------------------------------------------------------------
typedef int TDayTable[12];
static const TDayTable MonthDays[] =
{
  { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
  { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
};

const int HoursPerDay = 24;
const int MinsPerDay  = HoursPerDay * 60;
const int SecsPerDay  = MinsPerDay * 60;
const int MSecsPerDay = SecsPerDay * 1000;

// Days between 1/1/0001 and 12/31/1899
static const int DateDelta = 693594;

//---------------------------------------------------------------------------
bool TryEncodeDate(int Year, int Month, int Day, System::TDateTime & Date)
{
  const TDayTable * DayTable = &MonthDays[bg::gregorian_calendar::is_leap_year(Year)];
  if ((Year >= 1) && (Year <= 9999) && (Month >= 1) && (Month <= 12) &&
      (Day >= 1) && (Day <= (*DayTable)[Month - 1]))
  {
    for (int I = 1; I <= Month - 1; I++)
    {
      Day += (*DayTable)[I - 1];
    }
    int I = Year - 1;
    Date = System::TDateTime(I * 365 + I / 4 - I / 100 + I / 400 + Day - DateDelta);
    // DEBUG_PRINTF(L"Year = %d, Month = %d, Day = %d, Date = %f", Year, Month, Day, Date);
    return true;
  }
  return false;
}

System::TDateTime EncodeDate(int Year, int Month, int Day)
{
  System::TDateTime Result;
  if (!TryEncodeDate(Year, Month, Day, Result))
  {
    ::ConvertError(SDateEncodeError);
  }
  return Result;
}

//---------------------------------------------------------------------------
bool TryEncodeTime(unsigned int Hour, unsigned int Min, unsigned int Sec, unsigned int MSec, System::TDateTime & Time)
{
  bool Result = false;
  // DEBUG_PRINTF(L"Hour = %d, Min = %d, Sec = %d, MSec = %d", Hour, Min, Sec, MSec);
  if ((Hour < 24) && (Min < 60) && (Sec < 60) && (MSec < 1000))
  {
    Time = (Hour * 3600000 + Min * 60000 + Sec * 1000 + MSec) / static_cast<double>(MSecsPerDay);
    // DEBUG_PRINTF(L"Time = %f", Time);
    Result = true;
  }
  return Result;
}

System::TDateTime EncodeTime(unsigned int Hour, unsigned int Min, unsigned int Sec, unsigned int MSec)
{
  System::TDateTime Result;
  if (!TryEncodeTime(Hour, Min, Sec, MSec, Result))
  {
    ::ConvertError(STimeEncodeError);
  }
  // DEBUG_PRINTF(L"Result = %f", Result);
  return Result;
}
System::TDateTime StrToDateTime(const UnicodeString Value)
{
  System::Error(SNotImplemented, 145);
  return System::TDateTime();
}

bool TryStrToDateTime(const UnicodeString value, System::TDateTime & Value, System::TFormatSettings & FormatSettings)
{
  System::Error(SNotImplemented, 147);
  return false;
}

bool TryRelativeStrToDateTime(const UnicodeString value, System::TDateTime & Value)
{
  System::Error(SNotImplemented, 149);
  return false;
}

UnicodeString DateTimeToStr(UnicodeString & Result, const UnicodeString & Format,
                           System::TDateTime DateTime)
{
  System::Error(SNotImplemented, 148);
  return L"";
}

UnicodeString DateTimeToString(System::TDateTime DateTime)
{
  System::Error(SNotImplemented, 146);
  return L"";
}


//---------------------------------------------------------------------------
// DayOfWeek returns the day of the week of the given date. The result is an
// integer between 1 and 7, corresponding to Sunday through Saturday.
// This function is not ISO 8601 compliant, for that see the DateUtils unit.
unsigned int DayOfWeek(const System::TDateTime & DateTime)
{
  return ::DateTimeToTimeStamp(DateTime).Date % 7 + 1;
}


System::TDateTime Date()
{
  SYSTEMTIME t;
  ::GetLocalTime(&t);
  System::TDateTime result = ::EncodeDate(t.wYear, t.wMonth, t.wDay);
  return result;
}

void DivMod(const int Dividend, const unsigned int Divisor,
            unsigned int & Result, unsigned int & Remainder)
{
  Result = Dividend / Divisor;
  Remainder = Dividend % Divisor;
}

bool DecodeDateFully(const System::TDateTime & DateTime,
                     unsigned int & Year, unsigned int & Month, unsigned int & Day, unsigned int & DOW)
{
  static const int D1 = 365;
  static const int D4 = D1 * 4 + 1;
  static const int D100 = D4 * 25 - 1;
  static const int D400 = D100 * 4 + 1;
  bool Result = false;
  int T = DateTimeToTimeStamp(DateTime).Date;
  // DEBUG_PRINTF(L"DateTime = %f, T = %d", DateTime, T);
  unsigned int Y = 0;
  unsigned int M = 0;
  unsigned int D = 0;
  unsigned int I = 0;
  if (T <= 0)
  {
    Year = 0;
    Month = 0;
    Day = 0;
    DOW = 0;
    return false;
  }
  else
  {
    DOW = T % 7 + 1;
    T--;
    Y = 1;
    while (T >= D400)
    {
      T -= D400;
      Y += 400;
    }
    DivMod(T, D100, I, D);
    // DEBUG_PRINTF(L"T = %u, D100 = %u, I = %u, D = %u", T, D100, I, D);
    if (I == 4)
    {
      I--;
      D += D100;
    }
    Y += I * 100;
    DivMod(D, D4, I, D);
    // DEBUG_PRINTF(L"D4 = %u, I = %u, D = %u", D4, I, D);
    Y += I * 4;
    DivMod(D, D1, I, D);
    // DEBUG_PRINTF(L"D1 = %u, I = %u, D = %u", D1, I, D);
    if (I == 4)
    {
      I--;
      D += D1;
    }
    Y += I;
    Result = bg::gregorian_calendar::is_leap_year(Y);
    const TDayTable * DayTable = &MonthDays[Result];
    M = 1;
    while (true)
    {
      I = (*DayTable)[M - 1];
      // DEBUG_PRINTF(L"I = %u, D = %u", I, D);
      if (D < I)
      {
        break;
      }
      D -= I;
      M++;
    }
    Year = Y;
    Month = M;
    Day = D + 1;
  }
  return Result;
}

void DecodeDate(const System::TDateTime & DateTime, unsigned int & Year,
                unsigned int & Month, unsigned int & Day)
{
  unsigned int Dummy = 0;
  DecodeDateFully(DateTime, Year, Month, Day, Dummy);
}

void DecodeTime(const System::TDateTime & DateTime, unsigned int & Hour,
                unsigned int & Min, unsigned int & Sec, unsigned int & MSec)
{
  unsigned int MinCount, MSecCount;
  DivMod(DateTimeToTimeStamp(DateTime).Time, 60000, MinCount, MSecCount);
  DivMod(MinCount, 60, Hour, Min);
  DivMod(MSecCount, 1000, Sec, MSec);
}

UnicodeString FormatDateTime(const UnicodeString fmt, System::TDateTime DateTime)
{
  // DEBUG_PRINTF(L"fmt = %s", fmt.c_str());
  UnicodeString Result;
  // DateTimeToStr(Result, fmt, DateTime);
  boost::local_time::local_time_facet * output_facet = new boost::local_time::local_time_facet();
  UnicodeStringstream ss;
  ss.imbue(std::locale(std::locale::classic(), output_facet));
  output_facet->format(System::W2MB(fmt.c_str()).c_str());
  // boost::local_time::local_date_time ldt;
  unsigned int Y, M, D;
  DateTime.DecodeDate(Y, M, D);
  bg::date d(Y, M, D);
  ss << d;
  Result = ss.str();
  return Result;
}
/*
System::TDateTime ComposeDateTime(System::TDateTime Date, System::TDateTime Time)
{
  System::TDateTime Result = Trunc(Date);
  Result.Set(Time.GetHour(), Time.GetMinute(), Time.GetSecond(), Time.GetMillisecond());
  return Result;
}
*/

System::TDateTime SystemTimeToDateTime(const SYSTEMTIME & SystemTime)
{
  System::TDateTime Result(0.0);
  // ComposeDateTime(DoEncodeDate(SystemTime.Year, SystemTime.Month, SystemTime.Day), DoEncodeTime(SystemTime.Hour, SystemTime.Minute, SystemTime.Second, SystemTime.MilliSecond));
  ::TryEncodeDate(SystemTime.wYear, SystemTime.wMonth, SystemTime.wDay, Result);
  return Result;
}
