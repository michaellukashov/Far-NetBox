//---------------------------------------------------------------------------
#include <ShFolder.h>
#include "stdafx.h"
// #define NO_WIN32_LEAN_AND_MEAN

#include "Common.h"
#include "Exceptions.h"
#include "TextsCore.h"
#include "Interface.h"
// #include <StrUtils.hpp>
// #include <math.h>

//---------------------------------------------------------------------------
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
wstring ReplaceChar(wstring Str, char A, char B)
{
  for (int Index = 0; Index < Str.size(); Index++)
    if (Str[Index+1] == A) Str[Index+1] = B;
  return Str;
}
//---------------------------------------------------------------------------
wstring DeleteChar(wstring Str, char C)
{
  int P;
  while ((P = Str.find_first_of(C, 0)) > 0)
  {
    Str.erase(P, 1);
  }
  return Str;
}
//---------------------------------------------------------------------------
void PackStr(wstring &Str)
{
  // Following will free unnecessary bytes
  Str = Str.c_str();
}
//---------------------------------------------------------------------------
wstring MakeValidFileName(wstring FileName)
{
  wstring IllegalChars = L":;,=+<>|\"[] \\/?*";
  for (int Index = 0; Index < IllegalChars.size(); Index++)
  {
    FileName = ReplaceChar(FileName, IllegalChars[Index+1], L'-');
  }
  return FileName;
}
//---------------------------------------------------------------------------
wstring RootKeyToStr(HKEY RootKey)
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
wstring BooleanToEngStr(bool B)
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
wstring BooleanToStr(bool B)
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
wstring DefaultStr(const wstring & Str, const wstring & Default)
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
wstring CutToChar(wstring &Str, char Ch, bool Trim)
{
  int P = Str.find_first_of(Ch, 0);
  wstring Result;
  if (P)
  {
    Result = Str.substr(1, P-1);
    Str.erase(1, P);
  }
  else
  {
    Result = Str;
    Str = L"";
  }
  if (Trim)
  {
    // Result = Result.TrimRight();
    // Str = Str.TrimLeft();
  }
  return Result;
}
//---------------------------------------------------------------------------
wstring CopyToChars(const wstring & Str, int & From, wstring Chs, bool Trim,
  char * Delimiter)
{
  int P;
  for (P = From; P <= Str.size(); P++)
  {
    if (false) //FIXME IsDelimiter(Chs, Str, P))
    {
      break;
    }
  }

  wstring Result;
  if (P <= Str.size())
  {
    if (Delimiter != NULL)
    {
      *Delimiter = Str[P];
    }
    Result = Str.substr(From, P-From);
    From = P+1;
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
    // Result = Result.TrimRight();
    while ((P <= Str.size()) && (Str[P] == L' '))
    {
      P++;
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
wstring DelimitStr(wstring Str, wstring Chars)
{
  for (int i = 1; i <= Str.size(); i++)
  {
    if (false) // FIXME Str.IsDelimiter(Chars, i))
    {
      Str.insert(i, L"\\");
      i++;
    }
  }
  return Str;
}
//---------------------------------------------------------------------------
wstring ShellDelimitStr(wstring Str, char Quote)
{
  wstring Chars = L"$\\";
  if (Quote == '"')
  {
    Chars += L"`\"";
  }
  return DelimitStr(Str, Chars);
}
//---------------------------------------------------------------------------
wstring ExceptionLogString(exception *E)
{
  assert(E);
  // if (E->InheritsFrom(__classid(exception)))
  if (true) // FIXME dynamic_cast<E>(E) != NULL) // ->InheritsFrom(__classid(exception)))
  {
    wstring Msg;
    // Msg = FORMAT("(%s) %s", (E->ClassName(), E->Message));
    Msg = ::MB2W(E->what());
    if (false) // FIXME E->InheritsFrom(__classid(ExtException)))
    {
      /*
      TStrings * MoreMessages = ((ExtException*)E)->MoreMessages;
      if (MoreMessages)
      {
        Msg += "\n" +
          StringReplace(MoreMessages->Text, "\r", "", TReplaceFlags() << rfReplaceAll);
      }
      */
    }
    return Msg;
  }
  else
  {
    wchar_t Buffer[1024];
    // ExceptionErrorMessage(ExceptObject(), ExceptAddr(), Buffer, sizeof(Buffer));
    return wstring(Buffer);
  }
}
//---------------------------------------------------------------------------
bool IsNumber(const wstring Str)
{
  return _wtoi(Str.c_str()) != 0;
}
//---------------------------------------------------------------------------
wstring SystemTemporaryDirectory()
{
  wstring TempDir;
  TempDir.resize(MAX_PATH);
  TempDir.resize(GetTempPath(MAX_PATH, (wchar_t *)TempDir.c_str()));
  return TempDir;
}
//---------------------------------------------------------------------------
wstring GetShellFolderPath(int CSIdl)
{
  wstring Result;
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
wstring StripPathQuotes(const wstring Path)
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
wstring AddPathQuotes(wstring Path)
{
  Path = StripPathQuotes(Path);
  if (Path.find_first_of(L" "))
  {
    Path = L"\"" + Path + L"\"";
  }
  return Path;
}

wstring ReplaceStrAll(wstring Str, wstring What, wstring ByWhat)
{
    wstring result = Str;
    size_t pos = result.find_first_of(What);
    while (pos >= 0)
    {
        result.replace(pos, What.size(), ByWhat);
        pos = result.find_first_of(What);
    }
}

//---------------------------------------------------------------------------
void SplitCommand(wstring Command, wstring &Program,
  wstring & Params, wstring & Dir)
{
  // Command = Command.Trim(); //FIXME
  Params = L"";
  Dir = L"";
  if (!Command.empty() && (Command[1] == L'\"'))
  {
    Command.erase(1, 1);
    int P = Command.find_first_of(L'"');
    if (P)
    {
      Program = Command.substr(1, P-1); // .Trim();
      Params = Command.substr(P + 1, Command.size() - P); //.Trim();
    }
    else
    {
      throw exception(); // FIXME FMTLOAD(INVALID_SHELL_COMMAND, ("\"" + Command)));
    }
  }
  else
  {
    int P = Command.find_first_of(L" ");
    if (P)
    {
      Program = Command.substr(1, P); // .Trim();
      Params = Command.substr(P + 1, Command.size() - P); // .Trim();
    }
    else
    {
      Program = Command;
    }
  }
  int B = Program.find_last_of(L"\\");
  if (B)
  {
    Dir = Program.substr(1, B); // .Trim();
  }
  else
  {
    B = Program.find_last_of(L"/");
    if (B)
    {
      Dir = Program.substr(1, B); // .Trim();
    }
  }
}
//---------------------------------------------------------------------------
wstring ExtractProgram(wstring Command)
{
  wstring Program;
  wstring Params;
  wstring Dir;

  SplitCommand(Command, Program, Params, Dir);

  return Program;
}
//---------------------------------------------------------------------------
wstring FormatCommand(wstring Program, wstring Params)
{
  // Program = Program.Trim();
  // Params = Params.Trim();
  if (!Params.empty()) Params = L" " + Params;
  if (Program.find_first_of(L" ")) Program = L"\"" + Program + L"\"";
  return Program + Params;
}
//---------------------------------------------------------------------------
const wchar_t ShellCommandFileNamePattern[] = L"!.!";
//---------------------------------------------------------------------------
void ReformatFileNameCommand(wstring & Command)
{
  if (!Command.empty())
  {
    wstring Program, Params, Dir;
    SplitCommand(Command, Program, Params, Dir);
    if (Params.find_first_of(ShellCommandFileNamePattern) == 0)
    {
      Params = Params + (Params.empty() ? L"" : L" ") + ShellCommandFileNamePattern;
    }
    Command = FormatCommand(Program, Params);
  }
}
//---------------------------------------------------------------------------
wstring ExpandFileNameCommand(const wstring Command,
  const wstring FileName)
{
  return ReplaceStrAll(Command, ShellCommandFileNamePattern,
    AddPathQuotes(FileName));
}
//---------------------------------------------------------------------------
wstring EscapePuttyCommandParam(wstring Param)
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
wstring ExpandEnvironmentVariables(const wstring & Str)
{
  wstring Buf;
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
wstring ExtractShortPathName(const wstring & Path1)
{
    return Path1; //FIXME
}

bool AnsiSameText(const wstring &Str1, const wstring &Str2)
{
    return Str1 == Str2; //FIXME
}

wstring IncludeTrailingBackslash(const wstring &Path1)
{
    wstring result = Path1;
    if (Path1[Path1.size() - 1] != L'/' ||
        Path1[Path1.size() - 1] != L'\\')
    {
        result += L'\\';
    }
    return result;
}

//---------------------------------------------------------------------------
bool CompareFileName(const wstring & Path1, const wstring & Path2)
{
  wstring ShortPath1 = ExtractShortPathName(Path1);
  wstring ShortPath2 = ExtractShortPathName(Path2);

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
bool ComparePaths(const wstring & Path1, const wstring & Path2)
{
  // TODO: ExpandUNCFileName
  return AnsiSameText(IncludeTrailingBackslash(Path1), IncludeTrailingBackslash(Path2));
}
//---------------------------------------------------------------------------
bool IsReservedName(wstring FileName)
{
  int P = FileName.find_first_of(L".");
  int Len = (P > 0) ? P - 1 : FileName.size();
  if ((Len == 3) || (Len == 4))
  {
    if (P > 0)
    {
      FileName.resize(P - 1);
    }
    static wstring Reserved[] = {
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
wstring DisplayableStr(const wstring Str)
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

  wstring Result;
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
wstring CharToHex(char Ch, bool UpperCase)
{
  static char UpperDigits[] = "0123456789ABCDEF";
  static char LowerDigits[] = "0123456789abcdef";

  const char * Digits = (UpperCase ? UpperDigits : LowerDigits);
  wstring Result;
  Result.resize(2);
  Result[1] = Digits[((unsigned char)Ch & 0xF0) >> 4];
  Result[2] = Digits[ (unsigned char)Ch & 0x0F];
  return Result;
}
//---------------------------------------------------------------------------
wstring StrToHex(const wstring Str, bool UpperCase, char Separator)
{
  wstring Result;
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
wstring HexToStr(const wstring Hex)
{
  static wstring Digits = L"0123456789ABCDEF";
  wstring Result;
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
unsigned int HexToInt(const wstring Hex, int MinChars)
{
  static wstring Digits = L"0123456789ABCDEF";
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
//---------------------------------------------------------------------------
char HexToChar(const wstring Hex, int MinChars)
{
  return (char)HexToInt(Hex, MinChars);
}
//---------------------------------------------------------------------------
bool FileSearchRec(const wstring FileName, WIN32_FIND_DATA &Rec)
{
// FIXME
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
void ProcessLocalDirectory(wstring DirName,
  TProcessLocalFileEvent CallBackFunc, void * Param,
  int FindAttrs)
{
// FIXME
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
    try
    {
      do
      {
        if ((SearchRec.Name != ".") && (SearchRec.Name != ".."))
        {
          CallBackFunc(DirName + SearchRec.Name, SearchRec, Param);
        }

      } while (FindNext(SearchRec) == 0);
    }
    catch (...)
    {
    }
    FindClose(SearchRec);
  }
  */
}
//---------------------------------------------------------------------------
TDateTime EncodeDateVerbose(short int Year, short int Month, short int Day)
{
// FIXME
/*
  try
  {
    return EncodeDate(Year, Month, Day);
  }
  catch (EConvertError & E)
  {
    throw EConvertError(FORMAT("%s [%d-%d-%d]", (E.Message, int(Year), int(Month), int(Day))));
  }
  */
  return TDateTime();
}
//---------------------------------------------------------------------------
TDateTime EncodeTimeVerbose(short int Hour, short int Min, short int Sec, short int MSec)
{
// FIXME
/*
  try
  {
    return EncodeTime(Hour, Min, Sec, MSec);
  }
  catch (EConvertError & E)
  {
    throw EConvertError(FORMAT("%s [%d:%d:%d.%d]", (E.Message, int(Hour), int(Min), int(Sec), int(MSec))));
  }
  */
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
          throw exception(); // FIXME (TIMEZONE_ERROR);
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
    // DecodeDate(DateTime, Year, Month, Day);

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
      Result = false; // FIXME
        // (DateTime >= CurrentCache->DaylightDate) &&
        // (DateTime < CurrentCache->StandardDate);
    }
    else
    {
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

  return 0; // FIXME Round(double(DateTime - Params->UnixEpoch) * 86400) +
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
  TDateTime Result = TDateTime(); // FIXME SystemTimeToDateTime(SysTime);
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
wstring FixedLenDateTimeFormat(const wstring & Format)
{
  wstring Result = Format;
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
      if (Result.substr(Index, 5)/*.LowerCase()*/ == L"am/pm")
      {
        Index += 5;
      }
      else if (Result.substr(Index, 3)/*.LowerCase()*/ == L"a/p")
      {
        Index += 3;
      }
      else if (Result.substr(Index, 4)/*.LowerCase()*/ == L"ampm")
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
//---------------------------------------------------------------------------
bool RecursiveDeleteFile(const wstring FileName, bool ToRecycleBin)
{
//FIXME
/*
  SHFILEOPSTRUCT Data;

  memset(&Data, 0, sizeof(Data));
  Data.hwnd = NULL;
  Data.wFunc = FO_DELETE;
  wstring FileList(FileName);
  FileList.SetLength(FileList.Length() + 2);
  FileList[FileList.Length() - 1] = '\0';
  FileList[FileList.Length()] = '\0';
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
//---------------------------------------------------------------------------
wstring LoadStr(int Ident, unsigned int MaxLength)
{
  TPasLibModule * MainModule = FindModule(HInstance);
  assert(MainModule != NULL);

  wstring Result;
  Result.SetLength(MaxLength);
  int Length = LoadString(MainModule->ResInstance, Ident, Result.c_str(), MaxLength);
  Result.SetLength(Length);

  return Result;
}
//---------------------------------------------------------------------------
wstring LoadStrPart(int Ident, int Part)
{
  wstring Result;
  wstring Str = LoadStr(Ident);

  while (Part > 0)
  {
    Result = CutToChar(Str, '|', false);
    Part--;
  }
  return Result;
}
*/
//---------------------------------------------------------------------------
wstring DecodeUrlChars(wstring S)
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
          wstring C = HexToStr(S.substr(i + 1, 2));
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
wstring DoEncodeUrl(wstring S, wstring Chars)
{
  int i = 1;
  while (i <= S.size())
  {
    if (Chars.find_first_of(S[i]) > 0)
    {
      wstring H = CharToHex(S[i]);
      S.insert(i + 1, H);
      S[i] = L'%';
      i += H.size();
    }
    i++;
  }
  return S;
}
//---------------------------------------------------------------------------
wstring EncodeUrlChars(wstring S, wstring Ignore)
{
  wstring Chars;
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
wstring NonUrlChars()
{
  wstring S;
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
wstring EncodeUrlString(wstring S)
{
  return DoEncodeUrl(S, NonUrlChars());
}
//---------------------------------------------------------------------------
void OemToAnsi(wstring & Str)
{
  if (!Str.empty())
  {
    // Str.Unique();
    // FIXME OemToChar(Str.c_str(), Str.c_str());
  }
}
//---------------------------------------------------------------------------
void AnsiToOem(wstring & Str)
{
  if (!Str.empty())
  {
    // Str.Unique();
    // FIXME CharToOem(Str.c_str(), Str.c_str());
  }
}
//---------------------------------------------------------------------------
wstring EscapeHotkey(const wstring & Caption)
{
  return Caption; // FIXME StringReplace(Caption, "&", "&&", TReplaceFlags() << rfReplaceAll);
}
//---------------------------------------------------------------------------
// duplicated in console's Main.cpp
bool CutToken(wstring & Str, wstring & Token)
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
void AddToList(wstring & List, const wstring & Value, wchar_t Delimiter)
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
  return false; // FIXME (Win32MajorVersion >= 5);
}
//---------------------------------------------------------------------------
bool IsWin7()
{
  return false; // FIXME
    // (Win32MajorVersion > 6) ||
    // ((Win32MajorVersion == 6) && (Win32MinorVersion >= 1));
}
//---------------------------------------------------------------------------
bool IsExactly2008R2()
{
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
