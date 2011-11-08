//---------------------------------------------------------------------------
#include "stdafx.h"
// #include <ShFolder.h>
#include <shlobj.h>
#include <ShellAPI.h>

#include "boostdefines.hpp"
#include <boost/scope_exit.hpp>
#include <boost/algorithm/string.hpp>
#include "boost/date_time.hpp"
#include "boost/date_time/local_time/local_time.hpp"

#include "Common.h"
#include "Classes.h"
#include "Exceptions.h"
#include "TextsCore.h"
#include "Interface.h"

#include "FarPlugin.h"

namespace alg = boost::algorithm;
//---------------------------------------------------------------------------
int Win32Platform = 0;
int Win32MajorVersion = 0;
int Win32MinorVersion = 0;
int Win32BuildNumber = 0;
// int Win32CSDVersion = 0;
//---------------------------------------------------------------------------

inline int StrCmp(const wchar_t *s1, const wchar_t *s2)
{
    return ::CompareString(0, SORT_STRINGSORT, s1, -1, s2, -1) - 2;
}

inline int StrCmpI(const wchar_t *s1, const wchar_t *s2)
{
    return ::CompareString(0, NORM_IGNORECASE | SORT_STRINGSORT, s1, -1, s2, -1) - 2;
}

//---------------------------------------------------------------------------
void Abort()
{
    throw EAbort("");
}
//---------------------------------------------------------------------------
void Error(int ErrorID, int data)
{
    DEBUG_PRINTF(L"begin: ErrorID = %d, data = %d", ErrorID, data);
    std::wstring Msg = FMTLOAD(ErrorID, data);
    // DEBUG_PRINTF(L"Msg = %s", Msg.c_str());
    throw ExtException(Msg);
}

//---------------------------------------------------------------------------
// TCriticalSection
//---------------------------------------------------------------------------
TCriticalSection::TCriticalSection()
{
  FAcquired = 0;
  InitializeCriticalSection(&FSection);
}
//---------------------------------------------------------------------------
TCriticalSection::~TCriticalSection()
{
  assert(FAcquired == 0);
  DeleteCriticalSection(&FSection);
}
//---------------------------------------------------------------------------
void TCriticalSection::Enter()
{
  EnterCriticalSection(&FSection);
  FAcquired++;
}
//---------------------------------------------------------------------------
void TCriticalSection::Leave()
{
  FAcquired--;
  LeaveCriticalSection(&FSection);
}
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
const char EngShortMonthNames[12][4] =
  {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
   "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
//---------------------------------------------------------------------------
std::wstring ReplaceChar(std::wstring Str, wchar_t A, wchar_t B)
{
  for (size_t Index = 0; Index < Str.size(); Index++)
    if (Str[Index] == A) Str[Index] = B;
  return Str;
}
//---------------------------------------------------------------------------
std::wstring DeleteChar(std::wstring Str, wchar_t C)
{
  size_t P;
  while ((P = Str.find_first_of(C, 0)) != std::wstring::npos)
  {
    Str.erase(P, 1);
  }
  return Str;
}
//---------------------------------------------------------------------------
void PackStr(std::wstring &Str)
{
  // Following will free unnecessary bytes
  Str = Str.c_str();
}
//---------------------------------------------------------------------------
std::wstring MakeValidFileName(std::wstring FileName)
{
  std::wstring IllegalChars = L":;,=+<>|\"[] \\/?*";
  for (size_t Index = 0; Index < IllegalChars.size(); Index++)
  {
    FileName = ReplaceChar(FileName, IllegalChars[Index], L'-');
  }
  return FileName;
}
//---------------------------------------------------------------------------
std::wstring RootKeyToStr(HKEY RootKey)
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
  {  /*Abort(); */return L""; };
}
//---------------------------------------------------------------------------
std::wstring BooleanToEngStr(bool B)
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
std::wstring BooleanToStr(bool B)
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
std::wstring DefaultStr(const std::wstring & Str, const std::wstring & Default)
{
  if (!Str.empty())
  {
    return Str;
  }
  else
  {
    return Default;
  }
}
//---------------------------------------------------------------------------
std::wstring CutToChar(std::wstring &Str, wchar_t Ch, bool Trim)
{
  size_t P = Str.find_first_of(Ch, 0);
  std::wstring Result;
  // DEBUG_PRINTF(L"P = %d", P);
  if (P != std::wstring::npos)
  {
    Result = Str.substr(0, P);
    Str.erase(0, P + 1);
  }
  else
  {
    Result = Str;
    Str = L"";
  }
  // DEBUG_PRINTF(L"Result = %s", Result.c_str());
  if (Trim)
  {
    Str = TrimLeft(Str);
    Result = ::Trim(Result);
  }
  // DEBUG_PRINTF(L"Str = %s, Result = %s", Str.c_str(), Result.c_str());
  return Result;
}
//---------------------------------------------------------------------------
std::wstring CopyToChars(const std::wstring &Str, size_t &From, std::wstring Chars,
    bool Trim, char *Delimiter)
{
  size_t P;
  for (P = From; P < Str.size(); P++)
  {
    if (::IsDelimiter(Str, Chars, P))
    {
      break;
    }
  }
  // DEBUG_PRINTF(L"CopyToChars: Str = %s, Chars = %s, From = %d, P = %d", Str.c_str(), Chars.c_str(), From, P);

  std::wstring Result;
  if (P < Str.size())
  {
    if (Delimiter != NULL)
    {
      *Delimiter = (char)Str[P];
    }
    Result = Str.substr(From, P - From);
    From = P + 1;
  }
  else
  {
    if (Delimiter != NULL)
    {
      *Delimiter = L'\0';
    }
    Result = Str.substr(From, Str.size() - From + 1);
    From = P;
  }
  if (Trim)
  {
    Result = ::TrimRight(Result);
    while ((P < Str.size()) && (Str[P] == L' '))
    {
      P++;
    }
  }
  // DEBUG_PRINTF(L"CopyToChars: Result = %s", Result.c_str());
  return Result;
}
//---------------------------------------------------------------------------
std::wstring DelimitStr(std::wstring Str, std::wstring Chars)
{
  for (size_t i = 0; i < Str.size(); i++)
  {
    if (::IsDelimiter(Str, Chars, i))
    {
      Str.insert(i, L"\\");
      i++;
    }
  }
  return Str;
}
//---------------------------------------------------------------------------
std::wstring ShellDelimitStr(std::wstring Str, char Quote)
{
  std::wstring Chars = L"$\\";
  if (Quote == '"')
  {
    Chars += L"`\"";
  }
  return DelimitStr(Str, Chars);
}
//---------------------------------------------------------------------------
std::wstring ExceptionLogString(const std::exception *E)
{
  assert(E);
  if (::InheritsFrom<std::exception, std::exception>(E))
  {
    std::wstring Msg;
    Msg = FORMAT(L"(%s) %s", L"exception", ::MB2W(E->what()).c_str());
    if (::InheritsFrom<std::exception, ExtException>(E))
    {
      TStrings * MoreMessages = dynamic_cast<const ExtException *>(E)->GetMoreMessages();
      if (MoreMessages)
      {
        Msg += L"\n" +
          ::StringReplace(MoreMessages->GetText(), L"\r", L""); //, TReplaceFlags() << rfReplaceAll);
      }
    }
    return Msg;
  }
  else
  {
    // wchar_t Buffer[1024] = {0};
    // FIXME ExceptionErrorMessage(ExceptObject(), ExceptAddr(), Buffer, sizeof(Buffer));
    return std::wstring(::MB2W(E->what()));
  }
}
//---------------------------------------------------------------------------
bool IsNumber(const std::wstring Str)
{
  return _wtoi(Str.c_str()) != 0;
}
//---------------------------------------------------------------------------
std::wstring SystemTemporaryDirectory()
{
  std::wstring TempDir;
  TempDir.resize(MAX_PATH);
  TempDir.resize(GetTempPath(MAX_PATH, (wchar_t *)TempDir.c_str()));
  return TempDir;
}

std::wstring SysErrorMessage(int ErrorCode)
{
    // ::Error(SNotImplemented, 41); 
    std::wstring Result;
    // LPTSTR lpszTemp;
    wchar_t Buffer[255];
    int Len = ::FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |
      FORMAT_MESSAGE_ARGUMENT_ARRAY, NULL, ErrorCode, 0,
      (LPTSTR)Buffer,
      sizeof(Buffer), NULL);
    while ((Len > 0) && ((Buffer[Len - 1] >= 0) && 
      (Buffer[Len - 1] <= 32) || (Buffer[Len - 1] == '.')))
      Len--;
    // SetString(Result, Buffer, Len);
    Result = std::wstring(Buffer, Len);
    return Result;
}

//---------------------------------------------------------------------------
std::wstring GetShellFolderPath(int CSIdl)
{
  std::wstring Result;
  HMODULE Shell32Lib = LoadLibrary(L"SHELL32.DLL");
  if (Shell32Lib != NULL)
  {
    typedef HRESULT (__stdcall *PFNSHGETFOLDERPATH)(HWND, int, HANDLE, DWORD, LPTSTR);
    PFNSHGETFOLDERPATH SHGetFolderPath = (PFNSHGETFOLDERPATH)
      GetProcAddress(Shell32Lib, "SHGetFolderPathA");
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
  return Result;
}
//---------------------------------------------------------------------------
std::wstring StripPathQuotes(const std::wstring Path)
{
  if ((Path.size() >= 2) &&
      (Path[0] == L'\"') && (Path[Path.size() - 1] == L'\"'))
  {
    return Path.substr(2, Path.size() - 2);
  }
  else
  {
    return Path;
  }
}
//---------------------------------------------------------------------------
std::wstring AddPathQuotes(std::wstring Path)
{
  Path = StripPathQuotes(Path);
  if (Path.find_first_of(L" ") != std::wstring::npos)
  {
    Path = L"\"" + Path + L"\"";
  }
  return Path;
}

std::wstring ReplaceStrAll(std::wstring Str, std::wstring What, std::wstring ByWhat)
{
    std::wstring result = Str;
    size_t pos = result.find(What);
    while (pos != std::wstring::npos)
    {
        result.replace(pos, What.size(), ByWhat);
        pos = result.find(What);
    }
    return result;
}

//---------------------------------------------------------------------------
void SplitCommand(std::wstring Command, std::wstring &Program,
  std::wstring & Params, std::wstring & Dir)
{
  Command = ::Trim(Command);
  Params = L"";
  Dir = L"";
  if (!Command.empty() && (Command[0] == L'\"'))
  {
    Command.erase(0, 1);
    int P = Command.find_first_of(L'"');
    if (P)
    {
      Program = ::Trim(Command.substr(0, P-1));
      Params = ::Trim(Command.substr(P + 1, Command.size() - P));
    }
    else
    {
      throw ExtException(FMTLOAD(INVALID_SHELL_COMMAND, (L"\"" + Command).c_str()));
    }
  }
  else
  {
    int P = Command.find_first_of(L" ");
    if (P)
    {
      Program = ::Trim(Command.substr(0, P));
      Params = ::Trim(Command.substr(P + 1, Command.size() - P));
    }
    else
    {
      Program = Command;
    }
  }
  int B = Program.find_last_of(L"\\");
  if (B)
  {
    Dir = ::Trim(Program.substr(0, B));
  }
  else
  {
    B = Program.find_last_of(L"/");
    if (B)
    {
      Dir = ::Trim(Program.substr(0, B));
    }
  }
}
//---------------------------------------------------------------------------
std::wstring ExtractProgram(std::wstring Command)
{
  std::wstring Program;
  std::wstring Params;
  std::wstring Dir;

  SplitCommand(Command, Program, Params, Dir);

  return Program;
}
//---------------------------------------------------------------------------
std::wstring FormatCommand(std::wstring Program, std::wstring Params)
{
  Program = ::Trim(Program);
  Params = ::Trim(Params);
  if (!Params.empty()) Params = L" " + Params;
  if (Program.find_first_of(L" ")) Program = L"\"" + Program + L"\"";
  return Program + Params;
}
//---------------------------------------------------------------------------
const wchar_t ShellCommandFileNamePattern[] = L"!.!";
//---------------------------------------------------------------------------
void ReformatFileNameCommand(std::wstring & Command)
{
  if (!Command.empty())
  {
    std::wstring Program, Params, Dir;
    SplitCommand(Command, Program, Params, Dir);
    if (Params.find(ShellCommandFileNamePattern) == 0)
    {
      Params = Params + (Params.empty() ? L"" : L" ") + ShellCommandFileNamePattern;
    }
    Command = FormatCommand(Program, Params);
  }
}
//---------------------------------------------------------------------------
std::wstring ExpandFileNameCommand(const std::wstring Command,
  const std::wstring FileName)
{
  return ReplaceStrAll(Command, ShellCommandFileNamePattern,
    AddPathQuotes(FileName));
}
//---------------------------------------------------------------------------
std::wstring EscapePuttyCommandParam(std::wstring Param)
{
  bool Space = false;

  for (size_t i = 0; i < Param.size(); i++)
  {
    switch (Param[i])
    {
      case L'"':
        Param.insert(i, L"\\");
        i++;
        break;

      case L' ':
        Space = true;
        break;

      case L'\\':
        size_t i2 = i;
        while ((i2 < Param.size()) && (Param[i2] == L'\\'))
        {
          i2++;
        }
        if ((i2 < Param.size()) && (Param[i2] == L'"'))
        {
          while (Param[i] == L'\\')
          {
            Param.insert(i, L"\\");
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
std::wstring ExpandEnvironmentVariables(const std::wstring & Str)
{
  std::wstring Buf;
  unsigned int Size = 1024;

  Buf.resize(Size);
  // Buf.Unique(); //FIXME
  unsigned int Len = ExpandEnvironmentStrings(Str.c_str(), (wchar_t *)Buf.c_str(), Size);

  if (Len > Size)
  {
    Buf.resize(Len);
    // Buf.Unique();
    ExpandEnvironmentStrings(Str.c_str(), (wchar_t *)Buf.c_str(), Len);
  }

  PackStr(Buf);

  return Buf;
}
//---------------------------------------------------------------------------
std::wstring ExtractShortPathName(const std::wstring & Path1)
{
    return Path1; //FIXME
}

std::wstring ExtractDirectory(const std::wstring &path, wchar_t delimiter)
  //
  // Returns everything, including the trailing path separator, except the filename
  // part of the path.
  //
  // "/foo/bar/baz.txt" --> "/foo/bar/"
{
  return path.substr(0,path.find_last_of(delimiter) + 1);
}

std::wstring ExtractFilename(const std::wstring &path, wchar_t delimiter)
//
// Returns only the filename part of the path.
//
// "/foo/bar/baz.txt" --> "baz.txt"
{
    return path.substr(path.find_last_of(delimiter) + 1);
}

std::wstring ExtractFileExtension(const std::wstring &path, wchar_t delimiter)
//
// Returns the file's extension, if any. The period is considered part
// of the extension.
//
// "/foo/bar/baz.txt" --> ".txt"
// "/foo/bar/baz" --> ""
{
    std::wstring filename = ExtractFilename(path, delimiter);
    std::wstring::size_type n = filename.find_last_of('.');
    if (n != std::wstring::npos)
        return filename.substr(n);
    return std::wstring();
}

std::wstring ChangeFileExtension(const std::wstring &path, const std::wstring &ext, wchar_t delimiter)
  //
  // Modifies the filename's extension. The period is considered part
  // of the extension.
  //
  // "/foo/bar/baz.txt", ".dat" --> "/foo/bar/baz.dat"
  // "/foo/bar/baz.txt", "" --> "/foo/bar/baz"
  // "/foo/bar/baz", ".txt" --> "/foo/bar/baz.txt"
  //
{
  std::wstring filename = ExtractFilename(path, delimiter);
  return ExtractDirectory(path, delimiter)
       + filename.substr(0, filename.find_last_of('.'))
       + ext;
}
  
//---------------------------------------------------------------------------

std::wstring ExcludeTrailingBackslash(const std::wstring str)
{
    std::wstring result = str;
    if ((str.size() > 0) && ((str[str.size() - 1] == L'/') ||
        (str[str.size() - 1] == L'\\')))
    {
        result.resize(result.size() - 1);
    }
    return result;
}

std::wstring IncludeTrailingBackslash(const std::wstring str)
{
    std::wstring result = str;
    if ((str.size() == 0) || ((str[str.size() - 1] != L'/') &&
        (str[str.size() - 1] != L'\\')))
    {
        result += L'\\';
    }
    return result;
}

std::wstring ExtractFileDir(const std::wstring str)
{
    std::wstring result;
    int Pos = ::LastDelimiter(str, L"/\\");
    // DEBUG_PRINTF(L"Pos = %d", Pos);
    // it used to return Path when no slash was found
    if (Pos > 0)
    {
      result = str.substr(0, Pos + 1);
    }
    else
    {
      result = (Pos == 0) ? std::wstring(L"/") : std::wstring();
    }
    return result;
}

std::wstring ExtractFilePath(const std::wstring str)
{
    std::wstring result = ::ExtractFileDir(str);
    // DEBUG_PRINTF(L"str = %s, result = %s", str.c_str(), result.c_str());
    return result;
}

std::wstring GetCurrentDir()
{
    std::wstring result;
    wchar_t path[MAX_PATH + 1];
    GetCurrentDirectory(sizeof(path), path);
    result = path;
    return result;
}

//---------------------------------------------------------------------------
bool CompareFileName(const std::wstring & Path1, const std::wstring & Path2)
{
  std::wstring ShortPath1 = ExtractShortPathName(Path1);
  std::wstring ShortPath2 = ExtractShortPathName(Path2);

  bool Result;
  // ExtractShortPathName returns empty string if file does not exist
  if (ShortPath1.empty() || ShortPath2.empty())
  {
    Result = AnsiSameText(Path1, Path2) == 1;
  }
  else
  {
    Result = AnsiSameText(ShortPath1, ShortPath2) == 1;
  }
  return Result;
}
//---------------------------------------------------------------------------
bool ComparePaths(const std::wstring & Path1, const std::wstring & Path2)
{
  // TODO: ExpandUNCFileName
  return AnsiSameText(IncludeTrailingBackslash(Path1), IncludeTrailingBackslash(Path2)) == 1;
}
//---------------------------------------------------------------------------
bool IsReservedName(std::wstring FileName)
{
  int P = FileName.find_first_of(L".");
  int Len = (P > 0) ? P - 1 : FileName.size();
  if ((Len == 3) || (Len == 4))
  {
    if (P > 0)
    {
      FileName.resize(P - 1);
    }
    static std::wstring Reserved[] = {
      L"CON", L"PRN", L"AUX", L"NUL",
      L"COM1", L"COM2", L"COM3", L"COM4", L"COM5", L"COM6", L"COM7", L"COM8", L"COM9",
      L"LPT1", L"LPT2", L"LPT3", L"LPT4", L"LPT5", L"LPT6", L"LPT7", L"LPT8", L"LPT9" };
    for (int Index = 0; Index < LENOF(Reserved); Index++)
    {
      if (AnsiSameText(FileName, Reserved[Index]))
      {
        return true;
      }
    }
  }
  return false;
}
//---------------------------------------------------------------------------
std::wstring DisplayableStr(const std::wstring Str)
{
  bool Displayable = true;
  size_t Index = 0;
  while ((Index < Str.size()) && Displayable)
  {
    if ((Str[Index] < L'\32') &&
        (Str[Index] != L'\n') && (Str[Index] != L'\r') && (Str[Index] != L'\t') && (Str[Index] != L'\b'))
    {
      Displayable = false;
    }
    Index++;
  }

  std::wstring Result;
  if (Displayable)
  {
    Result = L"\"";
    for (size_t Index = 1; Index <= Str.size(); Index++)
    {
      switch (Str[Index])
      {
        case L'\n':
          Result += L"\\n";
          break;

        case L'\r':
          Result += L"\\r";
          break;

        case L'\t':
          Result += L"\\t";
          break;

        case L'\b':
          Result += L"\\b";
          break;

        case L'\\':
          Result += L"\\\\";
          break;

        case L'"':
          Result += L"\\\"";
          break;

        default:
          Result += Str[Index];
          break;
      }
    }
    Result += L"\"";
  }
  else
  {
    Result = L"0x" + StrToHex(Str);
  }
  return Result;
}
//---------------------------------------------------------------------------
std::wstring CharToHex(char Ch, bool UpperCase)
{
  static char UpperDigits[] = "0123456789ABCDEF";
  static char LowerDigits[] = "0123456789abcdef";

  const char * Digits = (UpperCase ? UpperDigits : LowerDigits);
  std::wstring Result;
  Result.resize(2);
  Result[0] = Digits[((unsigned char)Ch & 0xF0) >> 4];
  Result[1] = Digits[ (unsigned char)Ch & 0x0F];
  return Result;
}
//---------------------------------------------------------------------------
std::wstring StrToHex(const std::wstring Str, bool UpperCase, char Separator)
{
  std::wstring Result;
  for (size_t i = 1; i < Str.size(); i++)
  {
    Result += CharToHex(Str[i], UpperCase);
    if ((Separator != L'\0') && (i < Str.size()))
    {
      Result += Separator;
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
std::wstring HexToStr(const std::wstring Hex)
{
  static std::wstring Digits = L"0123456789ABCDEF";
  std::wstring Result;
  size_t L, P1, P2;
  L = Hex.size() - 1;
  if (L % 2 == 0)
  {
    for (size_t i = 0; i < Hex.size(); i += 2)
    {
      P1 = Digits.find_first_of((char)toupper(Hex[i]));
      P2 = Digits.find_first_of((char)toupper(Hex[i + 1]));
      if (P1 == std::wstring::npos || P2 == std::wstring::npos)
      {
        Result = L"";
        break;
      }
      else
      {
        Result += static_cast<wchar_t>((P1 - 1) * 16 + P2 - 1);
      }
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
unsigned int HexToInt(const std::wstring Hex, int MinChars)
{
  static std::wstring Digits = L"0123456789ABCDEF";
  int Result = 0;
  size_t I = 0;
  while (I < Hex.size())
  {
    size_t A = Digits.find_first_of((wchar_t)toupper(Hex[I]));
    if (A == std::wstring::npos)
    {
      if ((MinChars < 0) || (I <= (size_t)MinChars))
      {
        Result = 0;
      }
      break;
    }

    Result = (Result * 16) + (A - 1);

    I++;
  }
  return Result;
}

std::wstring IntToHex(unsigned int Int, int MinChars)
{
    std::wstringstream ss;
    ss << std::setfill(L'0') << std::setw(MinChars) << std::hex << Int;
    return ss.str();
}

//---------------------------------------------------------------------------
char HexToChar(const std::wstring Hex, int MinChars)
{
  return (char)HexToInt(Hex, MinChars);
}
//---------------------------------------------------------------------------
bool FileSearchRec(const std::wstring FileName, WIN32_FIND_DATA &Rec)
{
    HANDLE hFind = FindFirstFileW(FileName.c_str(), &Rec);
    bool Result = (hFind != INVALID_HANDLE_VALUE);
    FindClose(hFind);
    return Result;
}
//---------------------------------------------------------------------------
void ProcessLocalDirectory(std::wstring DirName,
  const processlocalfile_slot_type &CallBackFunc, void * Param,
  int FindAttrs)
{
  if (FindAttrs < 0)
  {
    FindAttrs = faReadOnly | faHidden | faSysFile | faDirectory | faArchive;
  }
  WIN32_FIND_DATA SearchRec;

  DirName = IncludeTrailingBackslash(DirName);
  std::wstring FileName = DirName + L"*.*";
  HANDLE h = ::FindFirstFileW(FileName.c_str(), &SearchRec);
  if (h != INVALID_HANDLE_VALUE)
  {
    BOOST_SCOPE_EXIT ( (h) )
    {
        ::FindClose(h);
    } BOOST_SCOPE_EXIT_END
    processlocalfile_signal_type sig;
    sig.connect(CallBackFunc);
    do
    {
      if ((wcscmp(SearchRec.cFileName, L".") != 0) && (wcscmp(SearchRec.cFileName, L"..") != 0))
      {
        if ((SearchRec.dwFileAttributes & FindAttrs) != 0)
        {
            sig(DirName + SearchRec.cFileName, SearchRec, Param);
        }
      }
    } while (::FindNextFile(h, &SearchRec));
  }
}
//---------------------------------------------------------------------------
class EConvertError : public ExtException
{
    typedef ExtException parent;
public:
    EConvertError(std::wstring Msg) :
        parent(Msg, NULL)
    {}
};

//---------------------------------------------------------------------------
void ConvertError(int ErrorID)
{
    std::wstring Msg = FMTLOAD(ErrorID, 0);
    throw EConvertError(Msg);
}

//---------------------------------------------------------------------------
typedef int TDayTable[12];
static const TDayTable MonthDays[] = {
  { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
  { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
};

static const int HoursPerDay = 24;
static const int MinsPerDay  = HoursPerDay * 60;
static const int SecsPerDay  = MinsPerDay * 60;
static const int MSecsPerDay = SecsPerDay * 1000;

// Days between 1/1/0001 and 12/31/1899
static const int DateDelta = 693594;

//---------------------------------------------------------------------------
bool TryEncodeDate(int Year, int Month, int Day, TDateTime &Date)
{
  const TDayTable *DayTable = &MonthDays[bg::gregorian_calendar::is_leap_year(Year)];
  if ((Year >= 1) && (Year <= 9999) && (Month >= 1) && (Month <= 12) &&
    (Day >= 1) && (Day <= (*DayTable)[Month - 1]))
  {
    for (int I = 1; I <= Month - 1; I++)
        Day += (*DayTable)[I - 1];
    int I = Year - 1;
    Date = TDateTime(I * 365 + I / 4 - I / 100 + I / 400 + Day - DateDelta);
    // DEBUG_PRINTF(L"Year = %d, Month = %d, Day = %d, Date = %f", Year, Month, Day, Date);
    return true;
  }
  return false;
}

TDateTime EncodeDate(int Year, int Month, int Day)
{
  TDateTime Result;
  if (!TryEncodeDate(Year, Month, Day, Result))
  {
    ::ConvertError(SDateEncodeError);
  }
  return Result;
}
//---------------------------------------------------------------------------
TDateTime EncodeDateVerbose(unsigned int Year, unsigned int Month, unsigned int Day)
{
  try
  {
    return EncodeDate(Year, Month, Day);
  }
  catch (const EConvertError &E)
  {
    throw EConvertError(FORMAT(L"%s [%d-%d-%d]", E.GetMessage().c_str(), int(Year), int(Month), int(Day)));
  }
  return TDateTime();
}
//---------------------------------------------------------------------------
bool TryEncodeTime(unsigned int Hour, unsigned int Min, unsigned int Sec, unsigned int MSec, TDateTime &Time)
{
  bool Result = false;
  // DEBUG_PRINTF(L"Hour = %d, Min = %d, Sec = %d, MSec = %d", Hour, Min, Sec, MSec);
  if ((Hour < 24) && (Min < 60) && (Sec < 60) && (MSec < 1000))
  {
    Time = (Hour * 3600000 + Min * 60000 + Sec * 1000 + MSec) / (double)MSecsPerDay;
    // DEBUG_PRINTF(L"Time = %f", Time);
    Result = true;
  }
  return Result;
}

TDateTime EncodeTime(unsigned int Hour, unsigned int Min, unsigned int Sec, unsigned int MSec)
{
  TDateTime Result;
  if (!TryEncodeTime(Hour, Min, Sec, MSec, Result))
  {
    ::ConvertError(STimeEncodeError);
  }
  // DEBUG_PRINTF(L"Result = %f", Result);
  return Result;
}
//---------------------------------------------------------------------------
TDateTime EncodeTimeVerbose(unsigned int Hour, unsigned int Min, unsigned int Sec, unsigned int MSec)
{
  try
  {
    return ::EncodeTime(Hour, Min, Sec, MSec);
  }
  catch (EConvertError & E)
  {
    throw EConvertError(FORMAT(L"%s [%d:%d:%d.%d]", E.GetMessage().c_str(), int(Hour), int(Min), int(Sec), int(MSec)));
  }
  return TDateTime();
}

TDateTime StrToDateTime(std::wstring Value)
{
    ::Error(SNotImplemented, 145);
  return TDateTime();
}

//---------------------------------------------------------------------------
// DayOfWeek returns the day of the week of the given date. The result is an
// integer between 1 and 7, corresponding to Sunday through Saturday.
// This function is not ISO 8601 compliant, for that see the DateUtils unit.
unsigned int DayOfWeek(const TDateTime DateTime)
{
  return ::DateTimeToTimeStamp(DateTime).Date % 7 + 1;
}

//---------------------------------------------------------------------------
struct TDateTimeParams
{
  TDateTime UnixEpoch;
  double BaseDifference;
  long BaseDifferenceSec;
  double CurrentDaylightDifference;
  long CurrentDaylightDifferenceSec;
  double CurrentDifference;
  long CurrentDifferenceSec;
  double StandardDifference;
  long StandardDifferenceSec;
  double DaylightDifference;
  long DaylightDifferenceSec;
  SYSTEMTIME StandardDate;
  SYSTEMTIME DaylightDate;
  bool DaylightHack;
};
static bool DateTimeParamsInitialized = false;
static TDateTimeParams DateTimeParams;
static TCriticalSection DateTimeParamsSection;
//---------------------------------------------------------------------------
static TDateTimeParams * GetDateTimeParams()
{
  if (!DateTimeParamsInitialized)
  {
    TGuard Guard(&DateTimeParamsSection);
    if (!DateTimeParamsInitialized)
    {
      TIME_ZONE_INFORMATION TZI;
      unsigned long GTZI;

      GTZI = GetTimeZoneInformation(&TZI);
      switch (GTZI)
      {
        case TIME_ZONE_ID_UNKNOWN:
          DateTimeParams.CurrentDaylightDifferenceSec = 0;
          break;

        case TIME_ZONE_ID_STANDARD:
          DateTimeParams.CurrentDaylightDifferenceSec = TZI.StandardBias;
          break;

        case TIME_ZONE_ID_DAYLIGHT:
          DateTimeParams.CurrentDaylightDifferenceSec = TZI.DaylightBias;
          break;

        case TIME_ZONE_ID_INVALID:
        default:
          throw std::exception(); // FIXME (TIMEZONE_ERROR);
      }
      // Is it same as SysUtils::UnixDateDelta = 25569 ??
      DateTimeParams.UnixEpoch = EncodeDateVerbose(1970, 1, 1);

      DateTimeParams.BaseDifferenceSec = TZI.Bias;
      DateTimeParams.BaseDifference = double(TZI.Bias) / 1440;
      DateTimeParams.BaseDifferenceSec *= 60;

      DateTimeParams.CurrentDifferenceSec = TZI.Bias +
        DateTimeParams.CurrentDaylightDifferenceSec;
      DateTimeParams.CurrentDifference =
        double(DateTimeParams.CurrentDifferenceSec) / 1440;
      DateTimeParams.CurrentDifferenceSec *= 60;

      DateTimeParams.CurrentDaylightDifference =
        double(DateTimeParams.CurrentDaylightDifferenceSec) / 1440;
      DateTimeParams.CurrentDaylightDifferenceSec *= 60;

      DateTimeParams.DaylightDifferenceSec = TZI.DaylightBias * 60;
      DateTimeParams.DaylightDifference = double(TZI.DaylightBias) / 1440;
      DateTimeParams.StandardDifferenceSec = TZI.StandardBias * 60;
      DateTimeParams.StandardDifference = double(TZI.StandardBias) / 1440;

      DateTimeParams.StandardDate = TZI.StandardDate;
      DateTimeParams.DaylightDate = TZI.DaylightDate;

      DateTimeParams.DaylightHack = !IsWin7() || IsExactly2008R2();

      DateTimeParamsInitialized = true;
    }
  }
  return &DateTimeParams;
}
//---------------------------------------------------------------------------
static void EncodeDSTMargin(const SYSTEMTIME &Date, unsigned short Year,
  TDateTime &Result)
{
  if (Date.wYear == 0)
  {
    TDateTime Temp = EncodeDateVerbose(Year, Date.wMonth, 1);
    
    Result = ((Date.wDayOfWeek - ::DayOfWeek(Temp) + 8) % 7) +
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
        Result = Result - 7;
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
  // ::Error(SNotImplemented, 46);
}
//---------------------------------------------------------------------------
static bool IsDateInDST(const TDateTime & DateTime)
{
  struct TDSTCache
  {
    bool Filled;
    unsigned short Year;
    TDateTime StandardDate;
    TDateTime DaylightDate;
    bool SummerDST;
  };
  static TDSTCache DSTCache[10];
  static int DSTCacheCount = 0;
  static TCriticalSection Section;

  TDateTimeParams *Params = GetDateTimeParams();
  bool Result;

  // On some systems it occurs that StandardDate is unset, while
  // DaylightDate is set. MSDN states that this is invalid and
  // should be treated as if there is no daylinght saving.
  // So check both.
  if ((Params->StandardDate.wMonth == 0) ||
      (Params->DaylightDate.wMonth == 0))
  {
    Result = false;
  }
  else
  {
    unsigned int Year, Month, Day;
    ::DecodeDate(DateTime, Year, Month, Day);

    TDSTCache * CurrentCache = &DSTCache[0];

    int CacheIndex = 0;
    while ((CacheIndex < DSTCacheCount) && (CacheIndex < LENOF(DSTCache)) &&
      CurrentCache->Filled && (CurrentCache->Year != Year))
    {
      CacheIndex++;
      CurrentCache++;
    }

    TDSTCache NewCache;
    if ((CacheIndex < DSTCacheCount) && (CacheIndex < LENOF(DSTCache)) &&
        CurrentCache->Filled)
    {
      assert(CurrentCache->Year == Year);
    }
    else
    {

      EncodeDSTMargin(Params->StandardDate, Year, NewCache.StandardDate);
      EncodeDSTMargin(Params->DaylightDate, Year, NewCache.DaylightDate);
      NewCache.SummerDST = (NewCache.DaylightDate < NewCache.StandardDate);
      if (DSTCacheCount < LENOF(DSTCache))
      {
        TGuard Guard(&Section);
        if (DSTCacheCount < LENOF(DSTCache))
        {
          NewCache.Year = (unsigned short)Year;
          DSTCache[DSTCacheCount] = NewCache;
          DSTCache[DSTCacheCount].Filled = true;
          DSTCacheCount++;
        }
      }
      CurrentCache = &NewCache;
    }

    if (CurrentCache->SummerDST)
    {
      Result =
        (DateTime >= CurrentCache->DaylightDate) &&
        (DateTime < CurrentCache->StandardDate);
    }
    else
    {
      Result =
        (DateTime < CurrentCache->StandardDate) ||
        (DateTime >= CurrentCache->DaylightDate);
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
bool UsesDaylightHack()
{
  return GetDateTimeParams()->DaylightHack;
}
//---------------------------------------------------------------------------
TDateTime UnixToDateTime(__int64 TimeStamp, TDSTMode DSTMode)
{
  TDateTimeParams * Params = GetDateTimeParams();

  TDateTime Result;
  // DEBUG_PRINTF(L"TimeStamp = %u, DSTMode = %d", TimeStamp, DSTMode);
  // ::Error(SNotImplemented, 49);
  Result = TDateTime(Params->UnixEpoch + (TimeStamp / 86400.0));

  if (Params->DaylightHack)
  {
    if ((DSTMode == dstmWin) || (DSTMode == dstmUnix))
    {
      Result = Result - Params->CurrentDifference;
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
__int64 Round(double Number)
{
  double Floor = floor(Number);
  double Ceil = ceil(Number);
  return ((Number - Floor) > (Ceil - Number)) ? Ceil : Floor;
}
//---------------------------------------------------------------------------
#define TIME_POSIX_TO_WIN(t, ft) (*(LONGLONG*)&(ft) = \
    ((LONGLONG) (t) + (LONGLONG) 11644473600) * (LONGLONG) 10000000)
#define TIME_WIN_TO_POSIX(ft, t) ((t) = (__int64) \
    ((*(LONGLONG*)&(ft)) / (LONGLONG) 10000000 - (LONGLONG) 11644473600))
//---------------------------------------------------------------------------
static __int64 DateTimeToUnix(const TDateTime DateTime)
{
  TDateTimeParams *Params = GetDateTimeParams();
  double value = double(DateTime - Params->UnixEpoch) * 86400;
  double intpart;
  modf(value, &intpart);
  return intpart + Params->CurrentDifferenceSec;
}
//---------------------------------------------------------------------------
FILETIME DateTimeToFileTime(const TDateTime DateTime,
  TDSTMode /*DSTMode*/)
{
  FILETIME Result;
  __int64 UnixTimeStamp = DateTimeToUnix(DateTime);
  // DEBUG_PRINTF(L"UnixTimeStamp = %d", UnixTimeStamp);

  TDateTimeParams *Params = GetDateTimeParams();
  // DEBUG_PRINTF(L"Params->DaylightHack = %d", Params->DaylightHack);
  if (!Params->DaylightHack)
  {
    UnixTimeStamp += (IsDateInDST(DateTime) ?
      Params->DaylightDifferenceSec : Params->StandardDifferenceSec);
    UnixTimeStamp -= Params->CurrentDaylightDifferenceSec;
  }
  // DEBUG_PRINTF(L"UnixTimeStamp = %d", UnixTimeStamp);
  TIME_POSIX_TO_WIN(UnixTimeStamp, Result);
  // DEBUG_PRINTF(L"Result = %d", Result.dwLowDateTime);

  return Result;
}
//---------------------------------------------------------------------------
TDateTime FileTimeToDateTime(const FILETIME & FileTime)
{
  // duplicated in DirView.pas
  SYSTEMTIME SysTime;
  TDateTimeParams * Params = GetDateTimeParams();
  if (!Params->DaylightHack)
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
__int64 ConvertTimestampToUnix(const FILETIME & FileTime,
  TDSTMode DSTMode)
{
  __int64 Result;
  TIME_WIN_TO_POSIX(FileTime, Result);

  TDateTimeParams * Params = GetDateTimeParams();
  if (Params->DaylightHack)
  {
    if ((DSTMode == dstmUnix) || (DSTMode == dstmKeep))
    {
      FILETIME LocalFileTime;
      SYSTEMTIME SystemTime;
      FileTimeToLocalFileTime(&FileTime, &LocalFileTime);
      FileTimeToSystemTime(&LocalFileTime, &SystemTime);
      TDateTime DateTime = SystemTimeToDateTime(SystemTime);
      Result += (IsDateInDST(DateTime) ?
        Params->DaylightDifferenceSec : Params->StandardDifferenceSec);

      if (DSTMode == dstmKeep)
      {
        Result -= Params->CurrentDaylightDifferenceSec;
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
      Result -= (IsDateInDST(DateTime) ?
        Params->DaylightDifferenceSec : Params->StandardDifferenceSec);
    }
  }

  return Result;
}
//---------------------------------------------------------------------------
TDateTime ConvertTimestampToUTC(TDateTime DateTime)
{

  TDateTimeParams * Params = GetDateTimeParams();
  ::Error(SNotImplemented, 54);
  // FIXME
  // DateTime += Params->CurrentDifference;
  // DateTime +=
    // (IsDateInDST(DateTime) ?
      // Params->DaylightDifference : Params->StandardDifference);

  return DateTime;
}
//---------------------------------------------------------------------------
__int64 ConvertTimestampToUnixSafe(const FILETIME & FileTime,
  TDSTMode DSTMode)
{
  __int64 Result;
  if ((FileTime.dwLowDateTime == 0) &&
      (FileTime.dwHighDateTime == 0))
  {
    Result = DateTimeToUnix(Now());
  }
  else
  {
    Result = ConvertTimestampToUnix(FileTime, DSTMode);
  }
  return Result;
}
//---------------------------------------------------------------------------
TDateTime AdjustDateTimeFromUnix(TDateTime DateTime, TDSTMode DSTMode)
{
  TDateTimeParams * Params = GetDateTimeParams();

  if (Params->DaylightHack)
  {
    if ((DSTMode == dstmWin) || (DSTMode == dstmUnix))
    {
      // ::Error(SNotImplemented, 55);
      DateTime = DateTime - Params->CurrentDaylightDifference;
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
std::wstring FixedLenDateTimeFormat(const std::wstring & Format)
{
  std::wstring Result = Format;
  bool AsIs = false;

  size_t Index = 0;
  while (Index < Result.size())
  {
    wchar_t F = Result[Index];
    if ((F == L'\'') || (F == L'\"'))
    {
      AsIs = !AsIs;
      Index++;
    }
    else if (!AsIs && ((F == L'a') || (F == L'A')))
    {
      if (::LowerCase(Result.substr(Index, 5)) == L"am/pm")
      {
        Index += 5;
      }
      else if (::LowerCase(Result.substr(Index, 3)) == L"a/p")
      {
        Index += 3;
      }
      else if (::LowerCase(Result.substr(Index, 4)) == L"ampm")
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
      if (!AsIs && (strchr("dDeEmMhHnNsS", F) != NULL) &&
          ((Index == Result.size()) || (Result[Index + 1] != F)))
      {
        ::Error(SNotImplemented, 56);
        // FIXME Result.insert(Index, F);
      }

      while ((Index <= Result.size()) && (F == Result[Index]))
      {
        Index++;
      }
    }
  }

  return Result;
}
//---------------------------------------------------------------------------
int CompareFileTime(TDateTime T1, TDateTime T2)
{
  // "FAT" time precision
  // (when one time is seconds-precision and other is millisecond-precision,
  // we may have times like 12:00:00.000 and 12:00:01.999, which should
  // be treated the same)
  //  FIXME
  ::Error(SNotImplemented, 57);
  /*
  static TDateTime TwoSeconds(0, 0, 2, 0);
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
  */
  return 0;
}

TDateTime Date()
{
    SYSTEMTIME t;
    ::GetLocalTime(&t);
    TDateTime result = ::EncodeDate(t.wYear, t.wMonth, t.wDay);
    return result;
}

void DivMod(const int Dividend, const unsigned int Divisor,
  unsigned int &Result, unsigned int &Remainder)
{
    Result = Dividend / Divisor;
    Remainder = Dividend % Divisor;
}

bool DecodeDateFully(const TDateTime &DateTime,
    unsigned int &Year, unsigned int &Month, unsigned int &Day, unsigned int &DOW)
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
    const TDayTable *DayTable = &MonthDays[Result];
    M = 1;
    while (true)
    {
      I = (*DayTable)[M - 1];
      // DEBUG_PRINTF(L"I = %u, D = %u", I, D);
      if (D < I)
        break;
      D -= I;
      M++;
    }
    Year = Y;
    Month = M;
    Day = D + 1;
  }
  return Result;
}

void DecodeDate(const TDateTime &DateTime, unsigned int &Year,
    unsigned int &Month, unsigned int &Day)
{
  unsigned int Dummy = 0;
  DecodeDateFully(DateTime, Year, Month, Day, Dummy);
}

void DecodeTime(const TDateTime &DateTime, unsigned int &Hour,
    unsigned int &Min, unsigned int &Sec, unsigned int &MSec)
{
  unsigned int MinCount, MSecCount;
  DivMod(DateTimeToTimeStamp(DateTime).Time, 60000, MinCount, MSecCount);
  DivMod(MinCount, 60, Hour, Min);
  DivMod(MSecCount, 1000, Sec, MSec);
}

std::wstring FormatDateTime(const std::wstring &fmt, TDateTime DateTime)
{
    // DEBUG_PRINTF(L"fmt = %s", fmt.c_str());
    // ::Error(SNotImplemented, 59);
    std::wstring Result;
    // DateTimeToString(Result, fmt, DateTime);
    boost::local_time::local_time_facet *output_facet = new boost::local_time::local_time_facet();
    std::wstringstream ss;
    ss.imbue(std::locale(std::locale::classic(), output_facet));
    output_facet->format(::W2MB(fmt.c_str()).c_str());
    // boost::local_time::local_date_time ldt;
    unsigned int Y, M, D;
    DateTime.DecodeDate(Y, M, D);
    bg::date d(Y, M, D);
    ss << d;
    Result = ss.str();
    return Result;
}
/*
TDateTime ComposeDateTime(TDateTime Date, TDateTime Time)
{
  TDateTime Result = Trunc(Date);
  Result.Set(Time.GetHour(), Time.GetMinute(), Time.GetSecond(), Time.GetMillisecond());
  return Result;
}
*/

TDateTime SystemTimeToDateTime(const SYSTEMTIME &SystemTime)
{
  TDateTime Result(0.0);
  // ComposeDateTime(DoEncodeDate(SystemTime.Year, SystemTime.Month, SystemTime.Day), DoEncodeTime(SystemTime.Hour, SystemTime.Minute, SystemTime.Second, SystemTime.MilliSecond));
  ::TryEncodeDate(SystemTime.wYear, SystemTime.wMonth, SystemTime.wDay, Result);
  return Result;
}

//---------------------------------------------------------------------------
bool RecursiveDeleteFile(const std::wstring FileName, bool ToRecycleBin)
{
  SHFILEOPSTRUCT Data;

  memset(&Data, 0, sizeof(Data));
  Data.hwnd = NULL;
  Data.wFunc = FO_DELETE;
  std::wstring FileList(FileName);
  FileList.resize(FileList.size() + 2);
  FileList[FileList.size() - 1] = '\0';
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
int CancelAnswer(int Answers)
{
  int Result;
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
int AbortAnswer(int Answers)
{
  int Result;
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
int ContinueAnswer(int Answers)
{
  int Result;
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
/*
TPasLibModule * FindModule(void * Instance)
{
  TPasLibModule * CurModule;
  CurModule = reinterpret_cast<TPasLibModule*>(LibModuleList);

  while (CurModule)
  {
    if (CurModule->Instance == Instance)
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
*/
//---------------------------------------------------------------------------
std::wstring LoadStr(int Ident, unsigned int MaxLength)
{
    std::wstring Result;
    HINSTANCE hInstance = FarPlugin ? FarPlugin->GetHandle() : GetModuleHandle(0);
    // DEBUG_PRINTF(L"hInstance = %u", hInstance);
    assert(hInstance != 0);

    Result.resize(MaxLength > 0 ? MaxLength : 255);
    int Length = ::LoadString(hInstance, Ident, (LPWSTR)Result.c_str(), Result.size());
    Result.resize(Length);

    return Result;
}
//---------------------------------------------------------------------------
std::wstring LoadStrPart(int Ident, int Part)
{
  std::wstring Result;

  std::wstring Str = LoadStr(Ident);
  // DEBUG_PRINTF(L"Str = %s", Str.c_str());

  while (Part > 0)
  {
    Result = ::CutToChar(Str, '|', false);
    Part--;
  }
  return Result;
}

//---------------------------------------------------------------------------
std::wstring DecodeUrlChars(std::wstring S)
{
  size_t i = 1;
  while (i <= S.size())
  {
    switch (S[i])
    {
      case L'+':
        S[i] = L' ';
        break;

      case L'%':
        if (i <= S.size() - 2)
        {
          std::wstring C = HexToStr(S.substr(i + 1, 2));
          if (C.size() == 1)
          {
            S[i] = C[1];
            S.erase(i + 1, 2);
          }
        }
        break;
    }
    i++;
  }
  return S;
}
//---------------------------------------------------------------------------
std::wstring DoEncodeUrl(std::wstring S, std::wstring Chars)
{
  size_t i = 0;
  while (i < S.size())
  {
    if (Chars.find_first_of(S[i]) != std::wstring::npos)
    {
      std::wstring H = CharToHex(S[i]);
      S.insert(i + 1, H);
      S[i] = L'%';
      i += H.size();
    }
    i++;
  }
  return S;
}
//---------------------------------------------------------------------------
std::wstring EncodeUrlChars(std::wstring S, std::wstring Ignore)
{
  std::wstring Chars;
  if (Ignore.find_first_of(L' ') == std::wstring::npos)
  {
    Chars += L' ';
  }
  if (Ignore.find_first_of(L'/') == std::wstring::npos)
  {
    Chars += L'/';
  }
  return DoEncodeUrl(S, Chars);
}
//---------------------------------------------------------------------------
std::wstring NonUrlChars()
{
  std::wstring S;
  for (unsigned int I = 0; I < 256; I++)
  {
    char C = static_cast<char>(I);
    if (((C >= 'a') && (C <= 'z')) ||
        ((C >= 'A') && (C <= 'Z')) ||
        ((C >= '0') && (C <= '9')) ||
        (C == '_') || (C == '-') || (C == '.'))
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
std::wstring EncodeUrlString(std::wstring S)
{
  return DoEncodeUrl(S, NonUrlChars());
}
//---------------------------------------------------------------------------
void OemToAnsi(std::wstring & Str)
{
  if (!Str.empty())
  {
    // Str.Unique();
    // FIXME OemToChar(Str.c_str(), Str.c_str());
    ::Error(SNotImplemented, 61);
  }
}
//---------------------------------------------------------------------------
void AnsiToOem(std::wstring & Str)
{
  if (!Str.empty())
  {
    // Str.Unique();
    // FIXME CharToOem(Str.c_str(), Str.c_str());
    ::Error(SNotImplemented, 62);
  }
}
//---------------------------------------------------------------------------
std::wstring EscapeHotkey(const std::wstring & Caption)
{
  return ::StringReplace(Caption, L"&", L"&&");
}
//---------------------------------------------------------------------------
// duplicated in console's Main.cpp
bool CutToken(std::wstring & Str, std::wstring & Token)
{
  bool Result;

  Token = L"";

  // inspired by Putty's sftp_getcmd() from PSFTP.C
  size_t Index = 0;
  while ((Index < Str.size()) &&
    ((Str[Index] == L' ') || (Str[Index] == L'\t')))
  {
    Index++;
  }

  if (Index < Str.size())
  {
    bool Quoting = false;

    while (Index < Str.size())
    {
      if (!Quoting && ((Str[Index] == L' ') || (Str[Index] == L'\t')))
      {
        break;
      }
      else if ((Str[Index] == L'"') && (Index + 1 <= Str.size()) &&
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

    if (Index < Str.size())
    {
      Index++;
    }

    Str = Str.substr(Index, Str.size());

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
void AddToList(std::wstring & List, const std::wstring & Value, wchar_t Delimiter)
{
  if (!List.empty() && (List[List.size()] != Delimiter))
  {
    List += Delimiter;
  }
  List += Value;
}
//---------------------------------------------------------------------------
bool Is2000()
{
  return (Win32MajorVersion >= 5);
}
//---------------------------------------------------------------------------
bool IsWin7()
{
  return (Win32MajorVersion > 6) ||
    ((Win32MajorVersion == 6) && (Win32MinorVersion >= 1));
}
//---------------------------------------------------------------------------
bool IsExactly2008R2()
{
  bool Result = false;
  HMODULE Kernel32 = GetModuleHandle(L"kernel32.dll");
  // typedef bool BOOL;
  // typedef unsigned long DWORD;
  // typedef unsigned long *PDWORD;
  typedef BOOL (WINAPI *TGetProductInfo)(DWORD, DWORD, DWORD, DWORD, PDWORD);
  TGetProductInfo GetProductInfo =
      (TGetProductInfo)GetProcAddress(Kernel32, "GetProductInfoA");
  if (GetProductInfo != NULL)
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
std::wstring IntToStr(int value)
{
    std::wstring result = boost::lexical_cast<std::wstring>(value);
    return result;
}

int StrToInt(const std::wstring value)
{
    return TryStrToInt(value, 0);
}

__int64 ToInt(const std::wstring value)
{
    return TryStrToInt(value, 0);
}

int StrToIntDef(const std::wstring value, int defval)
{
    return TryStrToInt(value, defval);
}

__int64 StrToInt64(const std::wstring value)
{
    return TryStrToInt(value, 0);
}

__int64 StrToInt64Def(const std::wstring value, __int64 defval)
{
    return TryStrToInt(value, defval);
}

__int64 TryStrToInt(const std::wstring value, __int64 defval)
{
    __int64 result = 0;
    try
    {
        result = boost::lexical_cast<__int64>(value);
    }
    catch (const boost::bad_lexical_cast &)
    {
        result = defval;
    }
    return result;
}

//---------------------------------------------------------------------------

std::wstring Trim(const std::wstring str)
{
    std::wstring result = TrimRight(TrimLeft(str));
    return result;
}

std::wstring TrimLeft(const std::wstring str)
{
    std::wstring result = str;
    while (result.size() > 0 && result[0] == ' ')
    {
        result = result.substr(1, result.size() - 1);
    }
    return result;
}

std::wstring TrimRight(const std::wstring str)
{
    std::wstring result = str;
    while (result.size() > 0 && 
        ((result[result.size() - 1] == ' ') || (result[result.size() - 1] == '\n')))
    {
        result.resize(result.size() - 1);
    }
    return result;
}

std::wstring UpperCase(const std::wstring str)
{
    std::wstring result;
    result.resize(str.size());
    std::transform(str.begin(), str.end(), result.begin(), ::toupper);
    return result;
}

std::wstring LowerCase(const std::wstring str)
{
    std::wstring result;
    result.resize(str.size());
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

std::wstring AnsiReplaceStr(const std::wstring str, const std::wstring from, const std::wstring to)
{
    std::wstring result = str;
    alg::replace_all(result, from, to);
    return result;
}

size_t AnsiPos(const std::wstring str, wchar_t c)
{
    size_t result = str.find_first_of(c);
    return result;
}

size_t Pos(const std::wstring str, const std::wstring substr)
{
    size_t result = str.find(substr);
    return result;
}

std::wstring StringReplace(const std::wstring str, const std::wstring from, const std::wstring to)
{
    std::wstring result = str;
    alg::replace_all(result, from, to);
    return result;
}

bool IsDelimiter(const std::wstring str, const std::wstring delim, int index)
{
    wchar_t c = str[index];
    for (size_t i = 0; i < delim.size(); i++)
    {
        if (delim[i] == c)
        {
            return true;
        }
    }
    return false;
}

size_t LastDelimiter(const std::wstring str, const std::wstring delim)
{
    for (size_t i = str.size() - 1; i >= 0; i--)
    {
        if (::IsDelimiter(str, delim, i))
        {
            return i;
        }
    }
    return std::string::npos;
}

//---------------------------------------------------------------------------

bool CompareText(const std::wstring str1, const std::wstring str2)
{
    // FIXME
    ::Error(SNotImplemented, 74);
    return false;
}

bool AnsiCompare(const std::wstring str1, const std::wstring str2)
{
    return StrCmp(str1.c_str(), str2.c_str()) == 0;
}

// Case-sensitive compare
bool AnsiCompareStr(const std::wstring str1, const std::wstring str2)
{
    return StrCmp(str1.c_str(), str2.c_str()) == 0;
}

bool AnsiSameText(const std::wstring str1, const std::wstring str2)
{
    return StrCmp(str1.c_str(), str2.c_str()) == 0;
}

bool SameText(const std::wstring str1, const std::wstring str2)
{
    return StrCmp(str1.c_str(), str2.c_str()) == 0;
}

bool AnsiCompareText(const std::wstring str1, const std::wstring str2)
{
    return StrCmpI(str1.c_str(), str2.c_str()) == 0;
}

bool AnsiCompareIC(const std::wstring str1, const std::wstring str2)
{
    return StrCmpI(str1.c_str(), str2.c_str()) == 0;
}

bool AnsiContainsText(const std::wstring str1, const std::wstring str2)
{
    // FIXME
    ::Error(SNotImplemented, 76);
    return false;
}

void RaiseLastOSError()
{
  int LastError = ::GetLastError();
  std::wstring ErrorMsg;
  if (LastError != 0)
    ErrorMsg = FMTLOAD(SOSError, LastError, ::SysErrorMessage(LastError).c_str());
  else
    ErrorMsg = FMTLOAD(SUnkOSError);
  throw EOSError(ErrorMsg, LastError);
}

//---------------------------------------------------------------------------
double StrToFloat(std::wstring Value)
{
    // FIXME
    ::Error(SNotImplemented, 77);
    return 0;
}

std::wstring FormatFloat(std::wstring Format, double value)
{
    // DEBUG_PRINTF(L"Format = %s", Format.c_str());
    // #,##0 "B"
    // FIXME
    // ::Error(SNotImplemented, 78);
    std::wstring result(20, 0);
    swprintf_s(&result[0], result.size(), L"%.2f", value);
    return result.c_str();
}

//---------------------------------------------------------------------------
TTimeStamp DateTimeToTimeStamp(TDateTime DateTime)
{
    TTimeStamp result = {0, 0};
    double fractpart, intpart;
    fractpart = modf(DateTime, &intpart);
    result.Time = (int)(fractpart * MSecsPerDay);
    result.Date = (int)(intpart + DateDelta);
    // DEBUG_PRINTF(L"DateTime = %f, time = %u, Date = %u", DateTime, result.Time, result.Date);
    return result;
}

//---------------------------------------------------------------------------

__int64 FileRead(HANDLE Handle, void *Buffer, __int64 Count)
{
  __int64 Result = -1;
  // DEBUG_PRINTF(L"Handle = %d, Count = %d", Handle, Count);
  DWORD res = 0;
  if (::ReadFile(Handle, (LPVOID)Buffer, (DWORD)Count, &res, NULL))
    Result = res;
  else
    Result = -1;
  // DEBUG_PRINTF(L"Result = %d, Handle = %d, Count = %d", (int)Result, Handle, Count);
  return Result;
}

__int64 FileWrite(HANDLE Handle, const void *Buffer, __int64 Count)
{
  __int64 Result = -1;
  DWORD res = 0;
  if (::WriteFile(Handle, Buffer, Count, &res, NULL))
    Result = res;
  else
    Result = -1;
  // DEBUG_PRINTF(L" Result = %d, Handle = %d, Count = %d", (int)Result, Handle, Count);
  return Result;
}

//---------------------------------------------------------------------------

bool FileExists(const std::wstring &fileName)
{
    return GetFileAttributes(fileName.c_str()) != 0xFFFFFFFF;
}

bool RenameFile(const std::wstring &from, const std::wstring &to)
{
    ::Error(SNotImplemented, 40); 
    return false;
}

bool DirectoryExists(const std::wstring &filename)
{
    // DEBUG_PRINTF(L"filename = %s", filename.c_str());
    if ((filename == L".") || (filename == L".."))
      return true;

    int attr = GetFileAttributes(filename.c_str());
    // DEBUG_PRINTF(L"attr = %d, FILE_ATTRIBUTE_DIRECTORY = %d", attr, FILE_ATTRIBUTE_DIRECTORY);

    if ((attr != 0xFFFFFFFF) && FLAGSET(attr, FILE_ATTRIBUTE_DIRECTORY))
      return true;
    return false;
}

std::wstring FileSearch(const std::wstring FileName, const std::wstring DirectoryList)
{
    // FIXME
    ::Error(SNotImplemented, 84);
    return std::wstring(L"");
}


int FileGetAttr(const std::wstring &filename)
{
    // FIXME
    // ::Error(SNotImplemented, 85);
    int attr = GetFileAttributes(filename.c_str());
    return attr;
}

int FileSetAttr(const std::wstring &filename, int attrs)
{
    // FIXME
    // ::Error(SNotImplemented, 86);
    int res = SetFileAttributes(filename.c_str(), attrs);
    return res;
}

bool CreateDir(const std::wstring Dir)
{
  // DEBUG_PRINTF(L"Dir = %s", Dir.c_str());
  return ::CreateDirectory(Dir.c_str(), NULL) == 0;
}

bool RemoveDir(const std::wstring Dir)
{
  return ::RemoveDirectory(Dir.c_str()) == 0;
}

bool ForceDirectories(const std::wstring Dir)
{
  // DEBUG_PRINTF(L"Dir = %s", Dir.c_str());
  bool Result = true;
  if (Dir.empty())
  {
    return false;
  }
  std::wstring Dir2 = ExcludeTrailingBackslash(Dir);
  // DEBUG_PRINTF(L"Dir2 = %s", Dir2.c_str());
  if ((Dir2.size() < 3) || DirectoryExists(Dir2))
  {
    return Result;
  }
  if (ExtractFilePath(Dir2).empty())
  {
    return ::CreateDir(Dir2);
  }
  Result = ForceDirectories(ExtractFilePath(Dir2)) && CreateDir(Dir2);
  // DEBUG_PRINTF(L"Result = %d", Result);
  return Result;
}

bool DeleteFile(const std::wstring File)
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

std::wstring Format(const wchar_t *format, ...)
{
    std::wstring result;
    va_list args;
    va_start(args, format);
    result = ::Format(format, args);
    va_end(args);
    return result.c_str();
}

//---------------------------------------------------------------------------

std::wstring Format(const wchar_t *format, va_list args)
{
    std::wstring result;
    if (format && *format)
    {
        int len = _vscwprintf(format, args);
        result.resize(len + 1); // sizeof(wchar_t));
        vswprintf_s(&result[0], len + 1, format, args);
    }
    return result.c_str();
}

//---------------------------------------------------------------------------
std::wstring FmtLoadStr(int id, ...)
{
    // DEBUG_PRINTF(L"begin: id = %d", id)
    std::wstring result;
    std::wstring format;
    HINSTANCE hInstance = FarPlugin ? FarPlugin->GetHandle() : GetModuleHandle(0);
    // DEBUG_PRINTF(L"hInstance = %u", hInstance);
    format.resize(255);
    int Length = ::LoadString(hInstance, id, (LPWSTR)format.c_str(), format.size());
    format.resize(Length);
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
        int len = _vscwprintf(format.c_str(), args);
        std::wstring buf(len + sizeof(wchar_t), 0);
        vswprintf_s(&buf[0], buf.size(), format.c_str(), args);
        va_end(args);
        result = buf;
    }
    // DEBUG_PRINTF(L"result = %s", result.c_str());
    return result;
}
//---------------------------------------------------------------------------
std::wstring WrapText(const std::wstring Line, int MaxCol)
{
    std::wstring Result;
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
std::wstring TranslateExceptionMessage(const std::exception *E)
{
    if (E)
        return ::MB2W(E->what());
    else
        return std::wstring();
}
//---------------------------------------------------------------------------
void AppendWChar(std::wstring &str, const wchar_t ch)
{
    if (!str.empty() && str[str.length() - 1] != ch)
    {
        str += ch;
    }
}

void AppendChar(std::string &str, const char ch)
{
    if (!str.empty() && str[str.length() - 1] != ch)
    {
        str += ch;
    }
}

void AppendPathDelimiterW(std::wstring &str)
{
    if (!str.empty() && str[str.length() - 1] != L'/' && str[str.length() - 1] != L'\\')
    {
        str += L"\\";;
    }
}

void AppendPathDelimiterA(std::string &str)
{
    if (!str.empty() && str[str.length() - 1] != '/' && str[str.length() - 1] != '\\')
    {
        str += "\\";;
    }
}

//---------------------------------------------------------------------------

std::wstring ExpandEnvVars(const std::wstring& str)
{
    wchar_t buf[MAX_PATH];
    unsigned size = ExpandEnvironmentStringsW(str.c_str(), buf, static_cast<DWORD>(sizeof(buf) - 1));
    std::wstring result = std::wstring(buf, size - 1);
    // DEBUG_PRINTF(L"result = %s", result.c_str());
    return result;
}

std::wstring StringOfChar(const wchar_t c, size_t len)
{
    std::wstring result;
    result.resize(len, c);
    return result;
}

// void RaiseLastOSError()
// {
// }

char *StrNew(const char *str)
{
    const size_t sz = strlen(str) + 1;
    char *Result = new char[sz];
    strncpy_s(Result, 1, str, sz);
    return Result;
}

wchar_t *AnsiStrScan(const wchar_t *Str, const wchar_t TokenPrefix)
{
    ::Error(SNotImplemented, 31); 
    wchar_t *result = NULL;
    return result;
}

std::wstring ChangeFileExt(std::wstring FileName, std::wstring ext)
{
    std::wstring result = ::ChangeFileExtension(FileName, ext, L'.');
    return result;
}

std::wstring ExtractFileExt(std::wstring FileName)
{
    std::wstring Result = ExtractFileExtension(FileName, L'.');
    return Result;
}

std::wstring get_full_path_name(const std::wstring &path)
{
  std::wstring buf(MAX_PATH, 0);
  DWORD size = GetFullPathNameW(path.c_str(), static_cast<DWORD>(buf.size() - 1), (LPWSTR)buf.c_str(), NULL);
  if (size > buf.size())
  {
    buf.resize(size);
    size = GetFullPathNameW(path.c_str(), static_cast<DWORD>(buf.size() - 1), (LPWSTR)buf.c_str(), NULL);
  }
  return std::wstring(buf.c_str(), size);
}

std::wstring ExpandFileName(const std::wstring FileName)
{
  std::wstring Result;
  Result = get_full_path_name(FileName);
  return Result;
}

std::wstring GetUniversalName(std::wstring FileName)
{
    // ::Error(SNotImplemented, 35);
    std::wstring Result = FileName;
    return Result;
}

std::wstring ExpandUNCFileName(std::wstring FileName)
{
    std::wstring Result = ExpandFileName(FileName);
    if ((Result.size() >= 3) && (Result[1] == L':') && (::UpCase(Result[0]) >= 'A')
      && (::UpCase(Result[0]) <= 'Z'))
    {
      Result = GetUniversalName(Result);
    }
    return Result;
}

__int64 FileSeek(HANDLE file, __int64 offset, __int64 size)
{
    ::Error(SNotImplemented, 300);
    return 0;
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
    RaiseLastOSError();
  return RetVal;
}
//---------------------------------------------------------------------------
