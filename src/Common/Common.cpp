//---------------------------------------------------------------------------
#include "stdafx.h"
// #include <ShFolder.h>
#include <shlobj.h>

#include "boostdefines.hpp"
#include <boost/algorithm/string.hpp>
// #include "boost/date_time/gregorian/greg_day.hpp"
// #include "boost/date_time/gregorian_calendar.hpp"
#include "boost/date_time/gregorian/gregorian.hpp"
#include "boost/date_time/year_month_day.hpp"

#include "Common.h"
#include "Exceptions.h"
#include "TextsCore.h"
#include "Interface.h"

#include "FarPlugin.h"
// #include <StrUtils.hpp>
// #include <math.h>

namespace alg = boost::algorithm;
namespace dt = boost::date_time;
namespace bg = boost::gregorian;
// namespace gc = boost::gregorian_calendar;

// typedef boost::date_time::year_month_day_base<
    // unsigned long, 
    // unsigned short, 
    // unsigned short> simple_ymd_type;

// typedef boost::date_time::gregorian_calendar_base<
    // simple_ymd_type,
    // unsigned long> gregorian_calendar;

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
void Error(int ErrorID, int data)
{
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
std::wstring ReplaceChar(std::wstring Str, char A, char B)
{
  for (int Index = 0; Index < Str.size(); Index++)
    if (Str[Index] == A) Str[Index] = B;
  return Str;
}
//---------------------------------------------------------------------------
std::wstring DeleteChar(std::wstring Str, char C)
{
  int P;
  while ((P = Str.find_first_of(C, 0)) > 0)
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
  for (int Index = 0; Index < IllegalChars.size(); Index++)
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
std::wstring CutToChar(std::wstring &Str, char Ch, bool Trim)
{
  int P = Str.find_first_of(Ch, 0);
  std::wstring Result;
  if (P)
  {
    Result = Str.substr(0, P);
    Str.erase(0, P + 1);
  }
  else
  {
    Result = Str;
    Str = L"";
  }
  if (Trim)
  {
    Str = TrimLeft(Str);
    Result = TrimRight(Str);
  }
  // DEBUG_PRINTF(L"Str = %s, Result = %s", Str.c_str(), Result.c_str());
  return Result;
}
//---------------------------------------------------------------------------
std::wstring CopyToChars(const std::wstring &Str, int &From, std::wstring Chars,
    bool Trim, char *Delimiter)
{
  int P;
  for (P = From; P <= Str.size(); P++)
  {
    if (::IsDelimiter(Str, Chars, P))
    {
      break;
    }
  }
  // DEBUG_PRINTF(L"CopyToChars: Str = %s, Chars = %s, From = %d, P = %d", Str.c_str(), Chars.c_str(), From, P);

  std::wstring Result;
  if (P <= Str.size())
  {
    if (Delimiter != NULL)
    {
      *Delimiter = Str[P];
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
    while ((P <= Str.size()) && (Str[P] == L' '))
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
  for (int i = 1; i <= Str.size(); i++)
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
      TStrings * MoreMessages = ((ExtException *)E)->GetMoreMessages();
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

std::wstring SysErrorMessage(int code)
{
    ::Error(SNotImplemented, 41); 
    std::wstring result;
    return result;
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
      (Path[1] == L'\"') && (Path[Path.size()] == L'\"'))
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
  if (Path.find_first_of(L" "))
  {
    Path = L"\"" + Path + L"\"";
  }
  return Path;
}

std::wstring ReplaceStrAll(std::wstring Str, std::wstring What, std::wstring ByWhat)
{
    std::wstring result = Str;
    size_t pos = result.find_first_of(What);
    while (pos >= 0)
    {
        result.replace(pos, What.size(), ByWhat);
        pos = result.find_first_of(What);
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
  if (!Command.empty() && (Command[1] == L'\"'))
  {
    Command.erase(1, 1);
    int P = Command.find_first_of(L'"');
    if (P)
    {
      Program = ::Trim(Command.substr(1, P-1));
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
      Program = ::Trim(Command.substr(1, P));
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
    Dir = ::Trim(Program.substr(1, B));
  }
  else
  {
    B = Program.find_last_of(L"/");
    if (B)
    {
      Dir = ::Trim(Program.substr(1, B));
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
    if (Params.find_first_of(ShellCommandFileNamePattern) == 0)
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

  for (int i = 1; i <= Param.size(); i++)
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
        int i2 = i;
        while ((i2 <= Param.size()) && (Param[i2] == L'\\'))
        {
          i2++;
        }
        if ((i2 <= Param.size()) && (Param[i2] == L'"'))
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
    if ((str.size() == 0) || (str[str.size() - 1] != L'/') ||
        (str[str.size() - 1] != L'\\'))
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
      result = str.substr(0, Pos);
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
    Result = AnsiSameText(Path1, Path2);
  }
  else
  {
    Result = AnsiSameText(ShortPath1, ShortPath2);
  }
  return Result;
}
//---------------------------------------------------------------------------
bool ComparePaths(const std::wstring & Path1, const std::wstring & Path2)
{
  // TODO: ExpandUNCFileName
  return AnsiSameText(IncludeTrailingBackslash(Path1), IncludeTrailingBackslash(Path2));
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
  int Index = 1;
  while ((Index <= Str.size()) && Displayable)
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
    for (int Index = 1; Index <= Str.size(); Index++)
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
  Result[1] = Digits[((unsigned char)Ch & 0xF0) >> 4];
  Result[2] = Digits[ (unsigned char)Ch & 0x0F];
  return Result;
}
//---------------------------------------------------------------------------
std::wstring StrToHex(const std::wstring Str, bool UpperCase, char Separator)
{
  std::wstring Result;
  for (int i = 1; i <= Str.size(); i++)
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
  int L, P1, P2;
  L = Hex.size();
  if (L % 2 == 0)
  {
    for (int i = 1; i <= Hex.size(); i += 2)
    {
      P1 = Digits.find_first_of((char)toupper(Hex[i]));
      P2 = Digits.find_first_of((char)toupper(Hex[i + 1]));
      if (P1 <= 0 || P2 <= 0)
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
  int I = 1;
  while (I <= Hex.size())
  {
    int A = Digits.find_first_of((wchar_t)toupper(Hex[I]));
    if (A <= 0)
    {
      if ((MinChars < 0) || (I <= MinChars))
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
    return L"";
}

//---------------------------------------------------------------------------
char HexToChar(const std::wstring Hex, int MinChars)
{
  return (char)HexToInt(Hex, MinChars);
}
//---------------------------------------------------------------------------
bool FileSearchRec(const std::wstring FileName, WIN32_FIND_DATA &Rec)
{
::Error(SNotImplemented, 42);
/*
  int FindAttrs = faReadOnly | faHidden | faSysFile | faDirectory | faArchive;
  bool Result = (FindFirst(FileName, FindAttrs, Rec) == 0);
  if (Result)
  {
    FindClose(Rec);
  }
  return Result;
*/ 
    return false;
}
//---------------------------------------------------------------------------
void ProcessLocalDirectory(std::wstring DirName,
  const processlocalfile_slot_type &CallBackFunc, void * Param,
  int FindAttrs)
{
::Error(SNotImplemented, 43);
/*
  assert(CallBackFunc);
  if (FindAttrs < 0)
  {
    FindAttrs = faReadOnly | faHidden | faSysFile | faDirectory | faArchive;
  }
  WIN32_FIND_DATA SearchRec;

  DirName = IncludeTrailingBackslash(DirName);
  if (FindFirst(DirName + "*.*", FindAttrs, SearchRec) == 0)
  {
    {
        BOOST_SCOPE_EXIT ( (&SearchRec) )
        {
            ::FindClose(SearchRec);
        }
      processlocalfile_signal_type sig;
      sig.connect(CallBackFunc);
      do
      {
        if ((SearchRec.Name != ".") && (SearchRec.Name != ".."))
        {
          sig(DirName + SearchRec.Name, SearchRec, Param);
        }

      } while (FindNext(SearchRec) == 0);
    }
  }
  */
}
//---------------------------------------------------------------------------
class EConvertError : public ExtException
{
    typedef ExtException parent;
public:
    EConvertError(std::wstring Msg) :
        parent(Msg)
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
// Days between 1/1/0001 and 12/31/1899
static const int DateDelta = 693594;

//---------------------------------------------------------------------------
bool TryEncodeDate(int Year, int Month, int Day, TDateTime &Date)
{
  const TDayTable *DayTable = &MonthDays[bg::gregorian_calendar::is_leap_year(Year)];
  if ((Year >= 1) && (Year <= 9999) && (Month >= 1) && (Month <= 12) &&
    (Day >= 1) && (Day <= (*DayTable)[Month]))
  {
    for (int I = 1; I <= Month - 1; I++)
        Day += (*DayTable)[I - 1];
    int I = Year - 1;
    Date = TDateTime(I * 365 + I / 4 - I / 100 + I / 400 + Day - DateDelta);
    return true;
  }
  return false;
}

TDateTime EncodeDate(int Year, int Month, int Day)
{
  TDateTime Result;
  if (!TryEncodeDate(Year, Month, Day, Result))
  {
    ConvertError(SDateEncodeError);
  }
  return Result;
}
//---------------------------------------------------------------------------
TDateTime EncodeDateVerbose(short int Year, short int Month, short int Day)
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
TDateTime EncodeTimeVerbose(short int Hour, short int Min, short int Sec, short int MSec)
{
::Error(SNotImplemented, 45);
/*
  try
  {
    return EncodeTime(Hour, Min, Sec, MSec);
  }
  catch (EConvertError & E)
  {
    throw EConvertError(FORMAT(L"%s [%d:%d:%d.%d]", (E.Message, int(Hour), int(Min), int(Sec), int(MSec))));
  }
  */
  return TDateTime();
}

TDateTime StrToDateTime(std::wstring Value)
{
  return TDateTime();
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
static void EncodeDSTMargin(const SYSTEMTIME & Date, unsigned short Year,
  TDateTime & Result)
{
  ::Error(SNotImplemented, 46);
  if (Date.wYear == 0)
  {
    TDateTime Temp = EncodeDateVerbose(Year, Date.wMonth, 1);
    
    Result = Temp; // FIXME + ((Date.wDayOfWeek - DayOfWeek(Temp) + 8) % 7) +
      // (7 * (Date.wDay - 1));
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
        // Result -= 7;
      }
    }
    // Result += EncodeTimeVerbose(Date.wHour, Date.wMinute, Date.wSecond,
      // Date.wMilliseconds);
  }
  else
  {
    // Result = EncodeDateVerbose(Year, Date.wMonth, Date.wDay) +
      // EncodeTimeVerbose(Date.wHour, Date.wMinute, Date.wSecond, Date.wMilliseconds);
  }
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

  TDateTimeParams * Params = GetDateTimeParams();
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
    unsigned short Year, Month, Day;
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
          NewCache.Year = Year;
          DSTCache[DSTCacheCount] = NewCache;
          DSTCache[DSTCacheCount].Filled = true;
          DSTCacheCount++;
        }
      }
      CurrentCache = &NewCache;
    }

    if (CurrentCache->SummerDST)
    {
      ::Error(SNotImplemented, 47);
      Result = false; // FIXME
        // (DateTime >= CurrentCache->DaylightDate) &&
        // (DateTime < CurrentCache->StandardDate);
    }
    else
    {
      ::Error(SNotImplemented, 48);
      Result = false; // FIXME
        // (DateTime < CurrentCache->StandardDate) ||
        // (DateTime >= CurrentCache->DaylightDate);
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
  ::Error(SNotImplemented, 49);
  Result = TDateTime(); // FIXME Params->UnixEpoch + (double(TimeStamp) / 86400);

  if (Params->DaylightHack)
  {
    if ((DSTMode == dstmWin) || (DSTMode == dstmUnix))
    {
      // Result -= Params->CurrentDifference;
    }
    else if (DSTMode == dstmKeep)
    {
      // Result -= Params->BaseDifference;
    }
  }
  else
  {
    // Result -= Params->BaseDifference;
  }
/*
  if ((DSTMode == dstmUnix) || (DSTMode == dstmKeep))
  {
    Result -= (IsDateInDST(Result) ?
      Params->DaylightDifference : Params->StandardDifference);
  }
*/
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
  TDateTimeParams * Params = GetDateTimeParams();

  ::Error(SNotImplemented, 50);
  return 0; // Round(double(DateTime - Params->UnixEpoch) * 86400) +
    // Params->CurrentDifferenceSec;
}
//---------------------------------------------------------------------------
FILETIME DateTimeToFileTime(const TDateTime DateTime,
  TDSTMode /*DSTMode*/)
{
  FILETIME Result;
  __int64 UnixTimeStamp = DateTimeToUnix(DateTime);

  TDateTimeParams * Params = GetDateTimeParams();
  if (!Params->DaylightHack)
  {
    UnixTimeStamp += (IsDateInDST(DateTime) ?
      Params->DaylightDifferenceSec : Params->StandardDifferenceSec);
    UnixTimeStamp -= Params->CurrentDaylightDifferenceSec;
  }

  TIME_POSIX_TO_WIN(UnixTimeStamp, Result);

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
  ::Error(SNotImplemented, 51);
  TDateTime Result = TDateTime(); // SystemTimeToDateTime(SysTime);
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
      ::Error(SNotImplemented, 52);
      TDateTime DateTime = TDateTime(); // FIXME SystemTimeToDateTime(SystemTime);
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
      ::Error(SNotImplemented, 53);
      TDateTime DateTime = TDateTime(); // FIXME SystemTimeToDateTime(SystemTime);
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
      ::Error(SNotImplemented, 55);
      // FIXME DateTime = DateTime - Params->CurrentDaylightDifference;
    }

    if (!IsDateInDST(DateTime))
    {
      if (DSTMode == dstmWin)
      {
        // DateTime = DateTime - Params->DaylightDifference;
      }
    }
    else
    {
      // DateTime = DateTime - Params->StandardDifference;
    }
  }
  else
  {
    if (DSTMode == dstmWin)
    {
      if (IsDateInDST(DateTime))
      {
        // DateTime = DateTime + Params->DaylightDifference;
      }
      else
      {
        // DateTime = DateTime + Params->StandardDifference;
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

  int Index = 1;
  while (Index <= Result.size())
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
    TDateTime result;
    return result;
}

void DecodeDate(const TDateTime &DateTime, unsigned short &Y,
    unsigned short &M, unsigned short &D)
{
    // FIXME
    ::Error(SNotImplemented, 58);
}

void DecodeTime(const TDateTime &DateTime, unsigned short &H,
    unsigned short &N, unsigned short &S, unsigned short &MS)
{
    ::Error(SNotImplemented, 40);
}

std::wstring FormatDateTime(const std::wstring &fmt, TDateTime DateTime)
{
    ::Error(SNotImplemented, 59);
    std::wstring result;
    return result;
}

//---------------------------------------------------------------------------
bool RecursiveDeleteFile(const std::wstring FileName, bool ToRecycleBin)
{
//FIXME
  ::Error(SNotImplemented, 60);
/*
  SHFILEOPSTRUCT Data;

  memset(&Data, 0, sizeof(Data));
  Data.hwnd = NULL;
  Data.wFunc = FO_DELETE;
  std::wstring FileList(FileName);
  FileList.resize(FileList.size() + 2);
  FileList[FileList.size() - 1] = '\0';
  FileList[FileList.size()] = '\0';
  Data.pFrom = FileList.c_str();
  Data.pTo = "";
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
  */
  return false;
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
    HINSTANCE hInstance = FarPlugin->GetHandle();
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
  int i = 1;
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
  int i = 1;
  while (i <= S.size())
  {
    if (Chars.find_first_of(S[i]) > 0)
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
  if (Ignore.find_first_of(L' ') == 0)
  {
    Chars += L' ';
  }
  if (Ignore.find_first_of(L'/') == 0)
  {
    Chars += L'/';
  }
  return DoEncodeUrl(S, Chars);
}
//---------------------------------------------------------------------------
std::wstring NonUrlChars()
{
  std::wstring S;
  for (unsigned int I = 0; I <= 255; I++)
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
  int Index = 1;
  while ((Index <= Str.size()) &&
    ((Str[Index] == L' ') || (Str[Index] == L'\t')))
  {
    Index++;
  }

  if (Index <= Str.size())
  {
    bool Quoting = false;

    while (Index <= Str.size())
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

    if (Index <= Str.size())
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
  ::Error(SNotImplemented, 63);
  return false; // FIXME (Win32MajorVersion >= 5);
}
//---------------------------------------------------------------------------
bool IsWin7()
{
  ::Error(SNotImplemented, 65);
  return false; // FIXME
    // (Win32MajorVersion > 6) ||
    // ((Win32MajorVersion == 6) && (Win32MinorVersion >= 1));
}
//---------------------------------------------------------------------------
bool IsExactly2008R2()
{
::Error(SNotImplemented, 66);
// FIXME
return false;
  // HANDLE Kernel32 = GetModuleHandle(kernel32);
  // typedef BOOL WINAPI (* TGetProductInfo)(DWORD, DWORD, DWORD, DWORD, PDWORD);
  // TGetProductInfo GetProductInfo =
      // (TGetProductInfo)GetProcAddress(Kernel32, "GetProductInfo");
  // bool Result;
  // if (GetProductInfo == NULL)
  // {
    // Result = false;
  // }
  // else
  // {
    // DWORD Type;
    // GetProductInfo(Win32MajorVersion, Win32MinorVersion, 0, 0, &Type);
    // switch (Type)
    // {
      // case 0x0008 /*PRODUCT_DATACENTER_SERVER*/:
      // case 0x000C /*PRODUCT_DATACENTER_SERVER_CORE}*/:
      // case 0x0027 /*PRODUCT_DATACENTER_SERVER_CORE_V*/:
      // case 0x0025 /*PRODUCT_DATACENTER_SERVER_V*/:
      // case 0x000A /*PRODUCT_ENTERPRISE_SERVE*/:
      // case 0x000E /*PRODUCT_ENTERPRISE_SERVER_COR*/:
      // case 0x0029 /*PRODUCT_ENTERPRISE_SERVER_CORE_*/:
      // case 0x000F /*PRODUCT_ENTERPRISE_SERVER_IA6*/:
      // case 0x0026 /*PRODUCT_ENTERPRISE_SERVER_*/:
      // case 0x002A /*PRODUCT_HYPER*/:
      // case 0x001E /*PRODUCT_MEDIUMBUSINESS_SERVER_MANAGEMEN*/:
      // case 0x0020 /*PRODUCT_MEDIUMBUSINESS_SERVER_MESSAGIN*/:
      // case 0x001F /*PRODUCT_MEDIUMBUSINESS_SERVER_SECURIT*/:
      // case 0x0018 /*PRODUCT_SERVER_FOR_SMALLBUSINES*/:
      // case 0x0023 /*PRODUCT_SERVER_FOR_SMALLBUSINESS_*/:
      // case 0x0021 /*PRODUCT_SERVER_FOUNDATIO*/:
      // case 0x0009 /*PRODUCT_SMALLBUSINESS_SERVE*/:
      // case 0x0038 /*PRODUCT_SOLUTION_EMBEDDEDSERVE*/:
      // case 0x0007 /*PRODUCT_STANDARD_SERVE*/:
      // case 0x000D /*PRODUCT_STANDARD_SERVER_COR*/:
      // case 0x0028 /*PRODUCT_STANDARD_SERVER_CORE_*/:
      // case 0x0024 /*PRODUCT_STANDARD_SERVER_*/:
      // case 0x0017 /*PRODUCT_STORAGE_ENTERPRISE_SERVE*/:
      // case 0x0014 /*PRODUCT_STORAGE_EXPRESS_SERVE*/:
      // case 0x0015 /*PRODUCT_STORAGE_STANDARD_SERVE*/:
      // case 0x0016 /*PRODUCT_STORAGE_WORKGROUP_SERVE*/:
      // case 0x0011 /*PRODUCT_WEB_SERVE*/:
      // case 0x001D /*PRODUCT_WEB_SERVER_COR*/:
        // Result = true;
        // break;

      // default:
        // Result = false;
        // break;
    // }
  // }
  // return Result;
}

//---------------------------------------------------------------------------
std::wstring IntToStr(int value)
{
    // FIXME
    ::Error(SNotImplemented, 67);
    std::wstring result;
    return result;
}

int StrToInt(const std::wstring value)
{
    // FIXME
    ::Error(SNotImplemented, 68);
    return 0;
}

__int64 ToInt(const std::wstring value)
{
    // FIXME
    ::Error(SNotImplemented, 69);
    return 0;
}

int StrToIntDef(const std::wstring value, int defval)
{
    // FIXME
    ::Error(SNotImplemented, 70);
    return 0;
}

__int64 StrToInt64(const std::wstring value)
{
    // FIXME
    ::Error(SNotImplemented, 71);
    return 0;
}

__int64 StrToInt64Def(const std::wstring value, __int64 defval)
{
    // FIXME
    ::Error(SNotImplemented, 72);
    return 0;
}

__int64 TryStrToInt(const std::wstring value, __int64 defval)
{
    // FIXME
    ::Error(SNotImplemented, 73);
    return 0;
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
    while (result.size() > 0 && result[result.size() - 1] == ' ')
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

int AnsiPos(const std::wstring str, wchar_t c)
{
    int result = str.find_first_of(c);
    return result == std::wstring::npos ? -1 : result;
}

int Pos(const std::wstring str, const std::wstring substr)
{
    int result = str.find_first_of(substr);
    return result == std::wstring::npos ? -1 : result;
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
    for (int i = 0; i < delim.size(); i++)
    {
        if (delim[i] == c)
        {
            return true;
        }
    }
    return false;
}

int LastDelimiter(const std::wstring str, const std::wstring delim)
{
    for (int i = str.size() - 1; i >= 0; i--)
    {
        if (::IsDelimiter(str, delim, i))
        {
            return i;
        }
    }
    return -1;
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
    // FIXME
    ::Error(SNotImplemented, 75);
    return false;
}

// Case-sensitive compare
int AnsiCompareStr(const std::wstring str1, const std::wstring str2)
{
    return StrCmp(str1.c_str(), str2.c_str());
}

bool AnsiSameText(const std::wstring str1, const std::wstring str2)
{
    return StrCmp(str1.c_str(), str2.c_str()) == 0;
}

bool SameText(const std::wstring str1, const std::wstring str2)
{
    return StrCmp(str1.c_str(), str2.c_str()) == 0;
}

int AnsiCompareText(const std::wstring str1, const std::wstring str2)
{
    return StrCmpI(str1.c_str(), str2.c_str());
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

//---------------------------------------------------------------------------

class EOSError : public std::exception
{
public:
    EOSError(std::wstring msg, DWORD code) : std::exception(::W2MB(msg.c_str()).c_str()),
        ErrorCode(code)
    {
    }
    DWORD ErrorCode;
};

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
    // FIXME
    ::Error(SNotImplemented, 78);
    return std::wstring(L"");
}

//---------------------------------------------------------------------------
TTimeStamp DateTimeToTimeStamp(TDateTime DateTime)
{
    // FIXME
    ::Error(SNotImplemented, 79);
    TTimeStamp result = {0, 0};
    return result;
}

//---------------------------------------------------------------------------

unsigned long FileRead(HANDLE Handle, void *Buffer, unsigned long Count)
{
    // FIXME
    ::Error(SNotImplemented, 80);
    return 0;
}

unsigned long FileWrite(HANDLE Handle, const void *Buffer, unsigned long Count)
{
    // FIXME
    ::Error(SNotImplemented, 81);
    return 0;
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
    if ((filename == L".") ||
        (filename == L".."))
      return true;

    int attr = GetFileAttributes(filename.c_str());

    if (FLAGSET(attr, FILE_ATTRIBUTE_DIRECTORY))
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
    ::Error(SNotImplemented, 85);
    return 0;
}

int FileSetAttr(const std::wstring &filename, int attrs)
{
    // FIXME
    ::Error(SNotImplemented, 86);
    return 0;
}

bool ForceDirectories(const std::wstring Dir)
{
    // FIXME
    ::Error(SNotImplemented, 87);
    return false;
}

bool DeleteFile(const std::wstring File)
{
    // FIXME
    ::Error(SNotImplemented, 88);
    return false;
}

bool RemoveDir(const std::wstring Dir)
{
    // FIXME
    ::Error(SNotImplemented, 89);
    return false;
}

//---------------------------------------------------------------------------
// bool InheritsFrom(const std::exception &E1, const std::exception &from)
// {
    // return false;
// }

//---------------------------------------------------------------------------

std::wstring Format(const wchar_t *format, ...)
{
    va_list args;
    va_start(args, format);
    int len = _vscwprintf(format, args);
    std::wstring result(len + sizeof(wchar_t), 0);
    vswprintf_s(&result[0], result.size(), format, args);
    va_end(args);
    return result;
}

//---------------------------------------------------------------------------
std::wstring FmtLoadStr(int id, ...)
{
    std::wstring result;
    std::wstring format;
    HINSTANCE hInstance = FarPlugin->GetHandle();
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
    ::Error(SNotImplemented, 30); 
    return (char *)str;
}

wchar_t *AnsiStrScan(const wchar_t *Str, const wchar_t TokenPrefix)
{
    ::Error(SNotImplemented, 31); 
    wchar_t *result = NULL;
    return result;
}

std::wstring EncryptPassword(std::wstring Password, std::wstring Key)
{
    ::Error(SNotImplemented, 32); 
    std::wstring result;
    return result;
}

std::wstring DecryptPassword(std::wstring Password, std::wstring Key)
{
    ::Error(SNotImplemented, 33); 
    std::wstring result;
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
  std::wstring buf(0, MAX_PATH);
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
    ::Error(SNotImplemented, 35);
    std::wstring Result = FileName;
    return Result;
}

std::wstring ExpandUNCFileName(std::wstring FileName)
{
    std::wstring Result = ExpandFileName(FileName);
    if ((Result.size() >= 3) && (Result[2] == L':') && (::UpCase(Result[1]) >= 'A')
      && (::UpCase(Result[1]) <= 'Z'))
    {
      Result = GetUniversalName(Result);
    }
    return Result;
}

void FileSeek(HANDLE file, __int64 offset, __int64 size)
{
    ::Error(SNotImplemented, 300);
}
