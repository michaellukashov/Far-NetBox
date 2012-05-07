//---------------------------------------------------------------------------
#ifndef _MSC_VER
#define NO_WIN32_LEAN_AND_MEAN
#include <vcl.h>
#pragma hdrstop
#else
#include <iostream>
#include <iomanip>

#include "stdafx.h"

#include "boostdefines.hpp"
#include <boost/scope_exit.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include "Classes.h"
#include "FarPlugin.h"
#include "RemoteFiles.h"
#include "Sysutils.h"
#endif

#ifdef _MSC_VER
namespace alg = boost::algorithm;
#endif
//---------------------------------------------------------------------------
#ifndef _MSC_VER
#pragma package(smart_init)
#endif

namespace Sysutils {
//---------------------------------------------------------------------------
int RandSeed;
const TDayTable MonthDays[] =
{
  { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
  { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
};

//---------------------------------------------------------------------------
UnicodeString IntToStr(int value)
{
  std::string result = boost::lexical_cast<std::string>(value);
  return MB2W(result.c_str());
}
//---------------------------------------------------------------------------
UnicodeString Int64ToStr(__int64 value)
{
  std::string result = boost::lexical_cast<std::string>(value);
  return MB2W(result.c_str());
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
  while (result.Length() > 0 && result[1] == ' ')
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
  UnicodeString result = str;
  // result.SetLength(str.Length());
  // std::transform(str.begin(), str.end(), result.begin(), ::toupper);
  return result.UpperCase();
}

UnicodeString LowerCase(const UnicodeString str)
{
  UnicodeString result = str;
  // result.SetLength(str.Length());
  // std::transform(str.begin(), str.end(), result.begin(), ::tolower);
  return result.LowerCase();
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
  std::wstring result = str;
  alg::replace_all(result, std::wstring(from.c_str()), std::wstring(to.c_str()));
  return result;
}

int AnsiPos(const UnicodeString str, wchar_t c)
{
  int result = str.Pos(c);
  return result;
}

int Pos(const UnicodeString str, const UnicodeString substr)
{
  int result = str.Pos(substr.c_str());
  return result;
}

UnicodeString StringReplace(const UnicodeString str, const UnicodeString from, const UnicodeString to, TReplaceFlags Flags)
{
  return AnsiReplaceStr(str, from, to);
}

bool IsDelimiter(const UnicodeString str, const UnicodeString delimiters, int index)
{
  if (index < str.Length())
  {
    wchar_t c = str[index];
    for (int  i = 1; i <= delimiters.Length(); i++)
    {
      if (delimiters[i] == c)
      {
        return true;
      }
    }
  }
  return false;
}

int LastDelimiter(const UnicodeString str, const UnicodeString delimiters)
{
  if (str.Length())
  {
    for (int i = str.Length(); i >= 1; --i)
    {
      if (::IsDelimiter(str, delimiters, i))
      {
        return i;
      }
    }
  }
  return 0;
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
  return ::Pos(str1, str2) >= 0;
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
    result = boost::lexical_cast<double>(W2MB(Value.c_str()));
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
  swprintf_s(&result[1], result.Length(), L"%.2f", value);
  return result.c_str();
}

//---------------------------------------------------------------------------
TTimeStamp DateTimeToTimeStamp(TDateTime DateTime)
{
  TTimeStamp result = {0, 0};
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
    if (i > 0)
    {
      Result = Temp.SubString(1, i - 1);
      Temp.Delete(1, i);
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
    vswprintf_s(&result[1], len + 1, format, args);
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
    vswprintf_s(&buf[1], buf.Length(), format.c_str(), args);
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
    return MB2W(E->what());
  }
  else
  {
    return UnicodeString();
  }
}
//---------------------------------------------------------------------------
void AppendWChar(UnicodeString & str, const wchar_t ch)
{
  if (!str.IsEmpty() && str[str.Length() - 1] != ch)
  {
    str += ch;
  }
}

void AppendChar(std::string & str, const char ch)
{
  if (!str.empty() && str[str.length() - 1] != ch)
  {
    str += ch;
  }
}

void AppendPathDelimiterW(UnicodeString & str)
{
  if (!str.IsEmpty() && str[str.Length() - 1] != L'/' && str[str.Length() - 1] != L'\\')
  {
    str += L"\\";;
  }
}

void AppendPathDelimiterA(std::string & str)
{
  if (!str.empty() && str[str.length() - 1] != '/' && str[str.length() - 1] != '\\')
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
  UnicodeString Result;
  if (int(len) < 0) len = 0;
  Result.SetLength(len);
  for (int i = 0; i < len; i++) Result[i] = c;
  return Result;
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
  Error(SNotImplemented, 31);
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
  if ((Result.Length() >= 3) && (Result[1] == L':') && (::UpCase(Result[1]) >= 'A')
      && (::UpCase(Result[1]) <= 'Z'))
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
  size_t pos = result.Pos(What.c_str());
  while (pos > 0)
  {
    result.Replace(pos, What.Length(), ByWhat.c_str(), ByWhat.Length());
    pos = result.Pos(What.c_str());
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
  return path.SubString(0, path.RPos(delimiter) + 1);
}

//
// Returns only the filename part of the path.
//
// "/foo/bar/baz.txt" --> "baz.txt"
UnicodeString ExtractFilename(const UnicodeString path, wchar_t delimiter)
{
  return path.SubString(path.RPos(delimiter) + 1);
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
  int n = filename.RPos(L'.');
  if (n > 0)
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
         + filename.SubString(0, filename.RPos(L'.'))
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
  if ((str.Length() == 0) || ((str[str.Length()] != L'/') &&
    (str[str.Length()] != L'\\')))
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
  if (Pos > 0)
  {
    result = str.SubString(1, Pos + 1);
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
UnicodeString StrToHex(const UnicodeString Str, bool UpperCase, char Separator)
{
  UnicodeString Result;
  for (size_t i = 1; i <= Str.Length(); i++)
  {
    Result += CharToHex(static_cast<char>(Str[i]), UpperCase);
    if ((Separator != L'\0') && (i <= Str.Length()))
    {
      Result += Separator;
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString HexToStr(const UnicodeString Hex)
{
  static std::wstring Digits = L"0123456789ABCDEF";
  std::wstring Result;
  size_t L, P1, P2;
  L = Hex.Length() - 1;
  if (L % 2 == 0)
  {
    for (size_t i = 1; i <= Hex.Length(); i += 2)
    {
      P1 = Digits.find_first_of(static_cast<char>(toupper(Hex[i])));
      P2 = Digits.find_first_of(static_cast<char>(toupper(Hex[i + 1])));
      if ((P1 == std::wstring::npos) || (P2 == std::wstring::npos))
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
  return UnicodeString(Result);
}
//---------------------------------------------------------------------------
unsigned int HexToInt(const UnicodeString Hex, size_t MinChars)
{
  static std::wstring Digits = L"0123456789ABCDEF";
  int Result = 0;
  size_t I = 1;
  while (I < Hex.Length())
  {
    size_t A = Digits.find_first_of(static_cast<wchar_t>(toupper(Hex[I])));
    if (A == std::wstring::npos)
    {
      if ((MinChars == NPOS) || (I <= MinChars))
      {
          Result = 0;
      }
      break;
    }

    Result = (Result * 16) + (static_cast<int>(A) - 1);

    I++;
  }
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString IntToHex(unsigned int Int, size_t MinChars)
{
  std::wstringstream ss;
  ss << std::setfill(L'0') << std::setw(MinChars) << std::hex << Int;
  return ss.str();
}
//---------------------------------------------------------------------------
char HexToChar(const UnicodeString Hex, size_t MinChars)
{
  return static_cast<char>(HexToInt(Hex, MinChars));
}
//---------------------------------------------------------------------------
void ConvertError(int ErrorID)
{
  UnicodeString Msg = FMTLOAD(ErrorID, 0);
  throw EConvertError(Msg);
}
//---------------------------------------------------------------------------
static void DivMod(const int Dividend, const unsigned int Divisor,
  unsigned int & Result, unsigned int & Remainder)
{
    Result = Dividend / Divisor;
    Remainder = Dividend % Divisor;
}

//---------------------------------------------------------------------------
static bool DecodeDateFully(const TDateTime & DateTime,
  unsigned short & Year, unsigned short & Month, unsigned short & Day, unsigned short & DOW)
{
  static const int D1 = 365;
  static const int D4 = D1 * 4 + 1;
  static const int D100 = D4 * 25 - 1;
  static const int D400 = D100 * 4 + 1;
  bool Result = false;
  int T = DateTimeToTimeStamp(DateTime).Date;
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
    Result = IsLeapYear(Y);
    const TDayTable * DayTable = &MonthDays[Result];
    M = 1;
    while (true)
    {
      I = (*DayTable)[M - 1];
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
//---------------------------------------------------------------------------
void DecodeDate(const TDateTime &DateTime, unsigned short &Year,
  unsigned short &Month, unsigned short &Day)
{
  unsigned short Dummy = 0;
  DecodeDateFully(DateTime, Year, Month, Day, Dummy);
}

void DecodeTime(const TDateTime &DateTime, unsigned short &Hour,
  unsigned short &Min, unsigned short &Sec, unsigned short &MSec)
{
  unsigned int MinCount, MSecCount;
  DivMod(DateTimeToTimeStamp(DateTime).Time, 60000, MinCount, MSecCount);
  unsigned int H, M, S, MS;
  DivMod(MinCount, 60, H, M);
  DivMod(MSecCount, 1000, S, MS);
  Hour = H;
  Min = M;
  Sec = S;
  MSec = MS;
}

//---------------------------------------------------------------------------
bool TryEncodeDate(int Year, int Month, int Day, TDateTime & Date)
{
  const TDayTable * DayTable = &MonthDays[IsLeapYear(Year)];
  if ((Year >= 1) && (Year <= 9999) && (Month >= 1) && (Month <= 12) &&
      (Day >= 1) && (Day <= (*DayTable)[Month - 1]))
  {
    for (int I = 1; I <= Month - 1; I++)
    {
      Day += (*DayTable)[I - 1];
    }
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
    ::ConvertError(SDateEncodeError);
  }
  return Result;
}

//---------------------------------------------------------------------------
bool TryEncodeTime(unsigned int Hour, unsigned int Min, unsigned int Sec, unsigned int MSec, TDateTime & Time)
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
TDateTime StrToDateTime(const UnicodeString Value)
{
  Error(SNotImplemented, 145);
  return TDateTime();
}

bool TryStrToDateTime(const UnicodeString value, TDateTime & Value, TFormatSettings & FormatSettings)
{
  Error(SNotImplemented, 147);
  return false;
}

UnicodeString DateTimeToStr(UnicodeString & Result, const UnicodeString & Format,
  TDateTime DateTime)
{
  Error(SNotImplemented, 148);
  return L"";
}

UnicodeString DateTimeToString(TDateTime DateTime)
{
  Error(SNotImplemented, 146);
  return L"";
}


//---------------------------------------------------------------------------
// DayOfWeek returns the day of the week of the given date. The result is an
// integer between 1 and 7, corresponding to Sunday through Saturday.
// This function is not ISO 8601 compliant, for that see the DateUtils unit.
unsigned int DayOfWeek(const TDateTime & DateTime)
{
  return ::DateTimeToTimeStamp(DateTime).Date % 7 + 1;
}


TDateTime Date()
{
  SYSTEMTIME t;
  ::GetLocalTime(&t);
  TDateTime result = ::EncodeDate(t.wYear, t.wMonth, t.wDay);
  return result;
}

UnicodeString FormatDateTime(const UnicodeString fmt, TDateTime DateTime)
{
  UnicodeString Result;
  Error(SNotImplemented, 150);
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

TDateTime SystemTimeToDateTime(const SYSTEMTIME & SystemTime)
{
  TDateTime Result(0.0);
  // ComposeDateTime(DoEncodeDate(SystemTime.Year, SystemTime.Month, SystemTime.Day), DoEncodeTime(SystemTime.Hour, SystemTime.Minute, SystemTime.Second, SystemTime.MilliSecond));
  ::TryEncodeDate(SystemTime.wYear, SystemTime.wMonth, SystemTime.wDay, Result);
  return Result;
}

//---------------------------------------------------------------------------
UnicodeString UnixExcludeLeadingBackslash(UnicodeString Path)
{
  while (!Path.IsEmpty() && Path[1] == L'/')
  {
    Path.Delete(1, 1);
  }
  return Path;
}

//---------------------------------------------------------------------------
void __fastcall Randomize()
{
  srand(static_cast<unsigned int>(time(NULL)));
}
//---------------------------------------------------------------------------

} // namespace Sysutils
