//---------------------------------------------------------------------------
#include <iostream>
#include <iomanip>

#include <Classes.hpp>
#include <Sysutils.hpp>
#include "FarPlugin.h"
#include "RemoteFiles.h"

//---------------------------------------------------------------------------

namespace Sysutils {
//---------------------------------------------------------------------------
int RandSeed;
const TDayTable MonthDays[] =
{
  { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
  { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
};

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
/* __fastcall */ Exception::Exception(Exception * E) :
  std::exception(E ? E->what() : ""),
  Message(E ? E->Message : L"")
{
}
//---------------------------------------------------------------------------
/* __fastcall */ Exception::Exception(const UnicodeString & Msg) :
  std::exception(""),
  Message(Msg)
{
}
//---------------------------------------------------------------------------
/* __fastcall */ Exception::Exception(const wchar_t *Msg) :
  std::exception(""),
  Message(Msg)
{
}
//---------------------------------------------------------------------------
/* __fastcall */ Exception::Exception(std::exception * E) :
  std::exception(E ? E->what() : "")
{
}
/* __fastcall */ Exception::Exception(const UnicodeString & Msg, int AHelpContext) :
  std::exception(""),
  Message(Msg)
{
  // TODO: FHelpContext = AHelpContext
  (void)AHelpContext;
}

/* __fastcall */ Exception::Exception(Exception * E, int Ident) :
  std::exception(E ? E->what() : "")
{
  Message = FMTLOAD(Ident);
}

/* __fastcall */ Exception::Exception(int Ident) :
  std::exception()
{
  Message = FMTLOAD(Ident);
}

//---------------------------------------------------------------------------
UnicodeString IntToStr(int Value)
{
  UnicodeString Result;
  Result.sprintf(L"%d", Value);
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString Int64ToStr(__int64 Value)
{
  UnicodeString Result;
  Result.sprintf(L"%lld", Value);
  return Result;
}
//---------------------------------------------------------------------------
int StrToInt(const UnicodeString & Value)
{
  __int64 Result = 0;
  if (TryStrToInt(Value, Result))
  {
    return static_cast<int>(Result);
  }
  else
  {
    return 0;
  }
}

__int64 ToInt(const UnicodeString & Value)
{
  __int64 Result = 0;
  if (TryStrToInt(Value, Result))
  {
    return Result;
  }
  else
  {
    return 0;
  }
}

int StrToIntDef(const UnicodeString & Value, int DefVal)
{
  __int64 Result = DefVal;
  if (TryStrToInt(Value, Result))
  {
    return static_cast<int>(Result);
  }
  else
  {
    return DefVal;
  }
}

__int64 StrToInt64(const UnicodeString & Value)
{
  return ToInt(Value);
}

__int64 StrToInt64Def(const UnicodeString & Value, __int64 DefVal)
{
  __int64 Result = DefVal;
  if (TryStrToInt(Value, Result))
  {
    return Result;
  }
  else
  {
    return DefVal;
  }
}

bool TryStrToInt(const std::wstring & StrValue, __int64 & Value)
{
  bool Result = !StrValue.empty();
  if (Result)
  {
    Value = _wtoi64(StrValue.c_str());
  }
  return Result;
}

bool TryStrToInt(const std::wstring & StrValue, int & Value)
{
  bool Result = !StrValue.empty();
  if (Result)
  {
    Value = _wtoi(StrValue.c_str());
  }
  return Result;
}

//---------------------------------------------------------------------------

UnicodeString Trim(const UnicodeString & Str)
{
  UnicodeString Result = TrimRight(TrimLeft(Str));
  return Result;
}

UnicodeString TrimLeft(const UnicodeString & Str)
{
  UnicodeString Result = Str;
  while (Result.Length() > 0 && Result[1] == L' ')
  {
    Result = Result.SubString(2, Result.Length() - 1);
  }
  return Result;
}

UnicodeString TrimRight(const UnicodeString & Str)
{
  UnicodeString Result = Str;
  while (Result.Length() > 0 &&
    ((Result[Result.Length()] == L' ') || (Result[Result.Length()] == L'\n')))
  {
    Result.SetLength(Result.Length() - 1);
  }
  return Result;
}

UnicodeString UpperCase(const UnicodeString & Str)
{
  std::wstring Result(Str.c_str(), Str.Length());
  // Result.SetLength(Str.Length());
  std::transform(Result.begin(), Result.end(), Result.begin(), ::toupper);
  return Result;
}

UnicodeString LowerCase(const UnicodeString & Str)
{
  std::wstring Result(Str.c_str(), Str.Length());
  std::transform(Result.begin(), Result.end(), Result.begin(), ::tolower);
  return Result;
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

UnicodeString AnsiReplaceStr(const UnicodeString & Str, const UnicodeString & From, const UnicodeString & To)
{
  UnicodeString Result = Str;
  intptr_t Pos = 0;
  while ((Pos = Result.Pos(From)) > 0)
  {
    Result.Replace(Pos, From.size(), To);
  }
  return Result;
}

intptr_t AnsiPos(const UnicodeString & Str, wchar_t c)
{
  intptr_t Result = Str.Pos(c);
  return Result;
}

intptr_t Pos(const UnicodeString & Str, const UnicodeString & Substr)
{
  intptr_t Result = Str.Pos(Substr.c_str());
  return Result;
}

UnicodeString StringReplace(const UnicodeString & Str, const UnicodeString & From, const UnicodeString & To, TReplaceFlags Flags)
{
  return AnsiReplaceStr(Str, From, To);
}

bool IsDelimiter(const UnicodeString & Delimiters, const UnicodeString & Str, intptr_t Index)
{
  if (Index <= Str.Length())
  {
    wchar_t c = Str[Index];
    for (intptr_t i = 1; i <= Delimiters.Length(); i++)
    {
      if (Delimiters[i] == c)
      {
        return true;
      }
    }
  }
  return false;
}

intptr_t LastDelimiter(const UnicodeString & Delimiters, const UnicodeString & Str)
{
  if (Str.Length())
  {
    for (intptr_t i = Str.Length(); i >= 1; --i)
    {
      if (Str.IsDelimiter(Delimiters, i))
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

int CompareText(const UnicodeString & Str1, const UnicodeString & Str2)
{
  return StringCmp(Str1.c_str(), Str2.c_str());
}

int AnsiCompare(const UnicodeString & Str1, const UnicodeString & Str2)
{
  return StringCmp(Str1.c_str(), Str2.c_str());
}

// Case-sensitive compare
int AnsiCompareStr(const UnicodeString & Str1, const UnicodeString & Str2)
{
  return StringCmp(Str1.c_str(), Str2.c_str());
}

bool AnsiSameText(const UnicodeString & Str1, const UnicodeString & Str2)
{
  return StringCmp(Str1.c_str(), Str2.c_str()) == 0;
}

bool SameText(const UnicodeString & Str1, const UnicodeString & Str2)
{
  return AnsiSameText(Str1, Str2);
}

int AnsiCompareText(const UnicodeString & Str1, const UnicodeString & Str2)
{
  return StringCmpI(Str1.c_str(), Str2.c_str());
}

int AnsiCompareIC(const UnicodeString & Str1, const UnicodeString & Str2)
{
  return AnsiCompareText(Str1, Str2);
}

bool AnsiContainsText(const UnicodeString & Str1, const UnicodeString & Str2)
{
  return ::Pos(Str1, Str2) > 0;
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
double StrToFloat(const UnicodeString & Value)
{
  return StrToFloatDef(Value, 0.0);
}
//---------------------------------------------------------------------------
double StrToFloatDef(const UnicodeString & Value, double DefVal)
{
  double Result = 0.0;
  try
  {
    Result = _wtof(Value.c_str());
  }
  catch (...)
  {
    Result = DefVal;
  }
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString FormatFloat(const UnicodeString & Format, double Value)
{
  UnicodeString Result(20, 0);
  swprintf_s(&Result[1], Result.Length(), L"%.2f", Value);
  return Result.c_str();
}

//---------------------------------------------------------------------------
TTimeStamp DateTimeToTimeStamp(TDateTime DateTime)
{
  TTimeStamp Result = {0, 0};
  double fractpart, intpart;
  fractpart = modf(DateTime, &intpart);
  Result.Time = static_cast<int>(fractpart * MSecsPerDay + 0.5);
  Result.Date = static_cast<int>(intpart + DateDelta);
  return Result;
}

//---------------------------------------------------------------------------

__int64 FileRead(HANDLE Handle, void * Buffer, __int64 Count)
{
  __int64 Result = -1;
  DWORD res = 0;
  if (::ReadFile(Handle, reinterpret_cast<LPVOID>(Buffer), static_cast<DWORD>(Count), &res, NULL))
  {
    Result = res;
  }
  else
  {
    Result = -1;
  }
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
  return Result;
}

//---------------------------------------------------------------------------

bool FileExists(const UnicodeString & FileName)
{
  return GetFileAttributes(FileName.c_str()) != 0xFFFFFFFF;
}

bool RenameFile(const UnicodeString & From, const UnicodeString & To)
{
  bool Result = ::MoveFile(From.c_str(), To.c_str()) != 0;
  return Result;
}

bool DirectoryExists(const UnicodeString & Filename)
{
  if ((Filename == THISDIRECTORY) || (Filename == PARENTDIRECTORY))
  {
    return true;
  }

  int attr = GetFileAttributes(Filename.c_str());

  if ((attr != 0xFFFFFFFF) && FLAGSET(attr, FILE_ATTRIBUTE_DIRECTORY))
  {
    return true;
  }
  return false;
}

UnicodeString FileSearch(const UnicodeString & FileName, const UnicodeString & DirectoryList)
{
  intptr_t I;
  UnicodeString Temp;
  UnicodeString Result;
  Temp = DirectoryList;
  UnicodeString PathSeparators = L"/\\";
  do
  {
    I = ::Pos(Temp, PathSeparators);
    while ((Temp.Length() > 0) && (I == 0))
    {
      Temp.Delete(1, 1);
      I = ::Pos(Temp, PathSeparators);
    }
    I = ::Pos(Temp, PathSeparators);
    if (I > 0)
    {
      Result = Temp.SubString(1, I - 1);
      Temp.Delete(1, I);
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
  return Result;
}


int FileGetAttr(const UnicodeString & Filename)
{
  int attr = GetFileAttributes(Filename.c_str());
  return attr;
}

int FileSetAttr(const UnicodeString & Filename, int Attrs)
{
  int res = SetFileAttributes(Filename.c_str(), Attrs);
  return res;
}

bool CreateDir(const UnicodeString & Dir)
{
  return ::CreateDirectory(Dir.c_str(), NULL) != 0;
}

bool RemoveDir(const UnicodeString & Dir)
{
  return ::RemoveDirectory(Dir.c_str()) != 0;
}

bool ForceDirectories(const UnicodeString & Dir)
{
  bool Result = true;
  if (Dir.IsEmpty())
  {
    return false;
  }
  UnicodeString Dir2 = ExcludeTrailingBackslash(Dir);
  if ((Dir2.Length() < 3) || DirectoryExists(Dir2))
  {
    return Result;
  }
  if (ExtractFilePath(Dir2).IsEmpty())
  {
    return ::CreateDir(Dir2);
  }
  Result = ForceDirectories(ExtractFilePath(Dir2)) && CreateDir(Dir2);
  return Result;
}

bool DeleteFile(const UnicodeString & File)
{
  ::DeleteFile(File.c_str());
  return !::FileExists(File);
}

//---------------------------------------------------------------------------

UnicodeString Format(const wchar_t * format, ...)
{
  UnicodeString Result;
  va_list args;
  va_start(args, format);
  Result = ::Format(format, args);
  va_end(args);
  return Result.c_str();
}

//---------------------------------------------------------------------------

UnicodeString Format(const wchar_t * Format, va_list args)
{
  UnicodeString Result;
  if (Format && *Format)
  {
    intptr_t Len = _vscwprintf(Format, args);
    Result.SetLength(Len + 1);
    vswprintf_s(const_cast<wchar_t *>(Result.c_str()), Len + 1, Format, args);
  }
  return Result.c_str();
}

//---------------------------------------------------------------------------

AnsiString Format(const char * format, ...)
{
  AnsiString Result;
  va_list args;
  va_start(args, format);
  Result = ::Format(format, args);
  va_end(args);
  return Result.c_str();
}

//---------------------------------------------------------------------------

AnsiString Format(const char * Format, va_list args)
{
  AnsiString Result;
  if (Format && *Format)
  {
    intptr_t Len = _vscprintf(Format, args);
    Result.SetLength(Len + 1);
    vsprintf_s(&Result[1], Len + 1, Format, args);
  }
  return Result.c_str();
}

//---------------------------------------------------------------------------
UnicodeString FmtLoadStr(int id, ...)
{
  UnicodeString Result;
  UnicodeString Format;
  Format.SetLength(1024);
  HINSTANCE hInstance = FarPlugin ? FarPlugin->GetHandle() : GetModuleHandle(0);
  intptr_t Length = ::LoadString(hInstance, id, reinterpret_cast<LPWSTR>(const_cast<wchar_t *>(Format.c_str())),
    static_cast<int>(Format.Length()));
  Format.SetLength(Length);
  if (!Length)
  {
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
    Result = lpszTemp;
    ::LocalFree(lpszTemp);
    */
    intptr_t Len = _vscwprintf(Format.c_str(), args);
    UnicodeString buf(Len + sizeof(wchar_t), 0);
    vswprintf_s(&buf[1], buf.Length(), Format.c_str(), args);
    va_end(args);
    Result = buf;
  }
  return Result;
}

//---------------------------------------------------------------------------
/*
 * return the next available word, ignoring whitespace
 */
static const wchar_t *
NextWord(const wchar_t * input)
{
  static wchar_t buffer[1024];
  static const wchar_t * text = NULL;

  wchar_t * endOfBuffer = buffer + LENOF(buffer) - 1;
  wchar_t * pBuffer = buffer;

  if (input)
  {
    text = input;
  }

  if (text)
  {
    /* add leading spaces */
    while (iswspace(*text))
    {
      *(pBuffer++) = *(text++);
    }

    /* copy the word to our static buffer */
    while (*text && !iswspace(*text) && pBuffer < endOfBuffer)
    {
      *(pBuffer++) = *(text++);
    }
  }

  *pBuffer = 0;

  return buffer;
}
//---------------------------------------------------------------------------
UnicodeString WrapText(const UnicodeString & Line, intptr_t MaxWidth)
{
  UnicodeString Result;
  const wchar_t * s = 0;
  wchar_t * w = 0;

  intptr_t lineCount = 0;
  intptr_t lenBuffer = 0;
  intptr_t spaceLeft = MaxWidth;

  if (MaxWidth == 0)
  {
    MaxWidth = 78;
  }
  if (MaxWidth < 5)
  {
    MaxWidth = 5;
  }

  /* two passes through the input. the first pass updates the buffer length.
   * the second pass creates and populates the buffer
   */
  while (Result.Length() == 0)
  {
    lineCount = 0;

    if (lenBuffer)
    {
      /* second pass, so create the wrapped buffer */
      Result.SetLength(lenBuffer + 1);
      if (Result.Length() == 0)
      {
        break;
      }
    }
    w = const_cast<wchar_t *>(Result.c_str());

    /* for each Word in Text
     *   if Width(Word) > SpaceLeft
     *     insert line break before Word in Text
     *     SpaceLeft := LineWidth - Width(Word)
     *   else
     *     SpaceLeft := SpaceLeft - Width(Word) + SpaceWidth
     */
    s = NextWord(Line.c_str());
    while (*s)
    {
      spaceLeft = MaxWidth;

      /* force the first word to always be completely copied */
      while (*s)
      {
        if (Result.Length() == 0)
        {
          ++lenBuffer;
        }
        else
        {
          *(w++) = *s;
        }
        --spaceLeft;
        ++s;
      }
      if (!*s)
      {
        s = NextWord(NULL);
      }

      /* copy as many words as will fit onto the current line */
      while (*s && static_cast<intptr_t>(wcslen(s) + 1) <= spaceLeft)
      {
        if (Result.Length() == 0)
        {
          ++lenBuffer;
        }
        --spaceLeft;

        /* then copy the word */
        while (*s)
        {
          if (Result.Length() == 0)
          {
            ++lenBuffer;
          }
          else
          {
            *(w++) = *s;
          }
          --spaceLeft;
          ++s;
        }
        if (!*s)
        {
          s = NextWord(NULL);
        }
      }
      if (!*s)
      {
        s = NextWord(NULL);
      }

      if (*s)
      {
        /* add a new line here */
        if (Result.Length() == 0)
        {
          ++lenBuffer;
        }
        else
        {
          *(w++) = L'\n';
        }
        // Skip whitespace before first word on new line
        while (iswspace(*s))
        {
          ++s;
        }
      }

      ++lineCount;
    }

    lenBuffer += 2;

    if (w)
    {
      *w = 0;
    }
  }

  return Result;
}

//---------------------------------------------------------------------------
UnicodeString TranslateExceptionMessage(std::exception * E)
{
  if (E)
  {
    if (dynamic_cast<Exception *>(E) != NULL)
    {
      return dynamic_cast<Exception *>(E)->Message;
    }
    else
    {
      return E->what();
    }
  }
  else
  {
    return UnicodeString();
  }
}
//---------------------------------------------------------------------------
void AppendWChar(UnicodeString & Str, const wchar_t ch)
{
  if (!Str.IsEmpty() && Str[Str.Length()] != ch)
  {
    Str += ch;
  }
}

void AppendChar(std::string & Str, const char ch)
{
  if (!Str.empty() && Str[Str.length() - 1] != ch)
  {
    Str += ch;
  }
}

void AppendPathDelimiterW(UnicodeString & Str)
{
  if (!Str.IsEmpty() && Str[Str.Length()] != L'/' && Str[Str.Length()] != L'\\')
  {
    Str += L"\\";;
  }
}

void AppendPathDelimiterA(std::string & Str)
{
  if (!Str.empty() && Str[Str.length() - 1] != '/' && Str[Str.length() - 1] != '\\')
  {
    Str += "\\";;
  }
}

//---------------------------------------------------------------------------

UnicodeString ExpandEnvVars(const UnicodeString & Str)
{
  wchar_t buf[MAX_PATH];
  intptr_t size = ExpandEnvironmentStringsW(Str.c_str(), buf, static_cast<DWORD>(sizeof(buf) - 1));
  UnicodeString Result = UnicodeString(buf, size - 1);
  return Result;
}

UnicodeString StringOfChar(const wchar_t c, intptr_t len)
{
  UnicodeString Result;
  if (len < 0) len = 0;
  Result.SetLength(len);
  for (intptr_t i = 1; i <= len; i++) Result[i] = c;
  return Result;
}

// void RaiseLastOSError()
// {
// }

char * StrNew(const char * Str)
{
  const size_t sz = strlen(Str) + 1;
  char * Result = new char[sz];
  strncpy_s(Result, sz, Str, sz);
  return Result;
}

UnicodeString ChangeFileExt(const UnicodeString & FileName, const UnicodeString & Ext)
{
  UnicodeString Result = ::ChangeFileExtension(FileName, Ext);
  return Result;
}

UnicodeString ExtractFileExt(const UnicodeString & FileName)
{
  UnicodeString Result = ExtractFileExtension(FileName, L'.');
  return Result;
}

UnicodeString get_full_path_name(const UnicodeString & Path)
{
  UnicodeString Buf(MAX_PATH, 0);
  intptr_t Size = GetFullPathNameW(Path.c_str(), static_cast<DWORD>(Buf.Length() - 1),
                                 reinterpret_cast<LPWSTR>(const_cast<wchar_t *>(Buf.c_str())), NULL);
  if (Size > Buf.Length())
  {
    Buf.SetLength(Size);
    Size = GetFullPathNameW(Path.c_str(), static_cast<DWORD>(Buf.Length() - 1), reinterpret_cast<LPWSTR>(const_cast<wchar_t *>(Buf.c_str())), NULL);
  }
  return UnicodeString(Buf.c_str(), Size);
}

UnicodeString ExpandFileName(const UnicodeString & FileName)
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

UnicodeString ExpandUNCFileName(const UnicodeString & FileName)
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

//---------------------------------------------------------------------------
static int FindMatchingFile(TSearchRec & Rec)
{
  TFileTime LocalFileTime = {0};
  int Result = 0;
  while ((Rec.FindData.dwFileAttributes && Rec.ExcludeAttr) != 0)
  {
    if (!FindNextFileW(Rec.FindHandle, &Rec.FindData))
    {
      Result = GetLastError();
      return Result;
    }
  }
  FileTimeToLocalFileTime(&Rec.FindData.ftLastWriteTime, (LPFILETIME)&LocalFileTime);
  WORD Hi = (Rec.Time & 0xFFFF0000) >> 16;
  WORD Lo = Rec.Time & 0xFFFF;
  FileTimeToDosDateTime((LPFILETIME)&LocalFileTime, &Hi, &Lo);
  Rec.Time = (Hi << 16) + Lo;
  Rec.Size = Rec.FindData.nFileSizeLow || Int64(Rec.FindData.nFileSizeHigh) << 32;
  Rec.Attr = Rec.FindData.dwFileAttributes;
  Rec.Name = Rec.FindData.cFileName;
  Result = 0;
  return Result;
}

//---------------------------------------------------------------------------
int FindFirst(const UnicodeString & FileName, int Attr, TSearchRec & Rec)
{
  const int faSpecial = faHidden | faSysFile | faDirectory;
  // HANDLE hFind = FindFirstFileW(FileName.c_str(), &Rec);
  // bool Result = (hFind != INVALID_HANDLE_VALUE);
  // if (Result) Classes::FindClose(Rec);
  // return Result;
  Rec.ExcludeAttr = (~Attr) & faSpecial;
  Rec.FindHandle = FindFirstFileW(FileName.c_str(), &Rec.FindData);
  int Result = 0;
  if (Rec.FindHandle != INVALID_HANDLE_VALUE)
  {
    Result = FindMatchingFile(Rec);
    if (Result != 0) FindClose(Rec);
  }
  else
    Result = GetLastError();
  return Result;
}

int FindNext(TSearchRec & Rec)
{
  int Result = 0;
  if (FindNextFileW(Rec.FindHandle, &Rec.FindData))
    Result = FindMatchingFile(Rec);
  else
    Result = GetLastError();
  return Result;
}

int FindClose(TSearchRec & Rec)
{
  int Result = 0;
  if (Rec.FindHandle != INVALID_HANDLE_VALUE)
  {
    ::FindClose(Rec.FindHandle);
    Rec.FindHandle = INVALID_HANDLE_VALUE;
  }
  return Result;
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
  intptr_t Len = ::FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |
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

UnicodeString ReplaceStrAll(const UnicodeString & Str, const UnicodeString & What, const UnicodeString & ByWhat)
{
  UnicodeString Result = Str;
  intptr_t Pos = Result.Pos(What.c_str());
  while (Pos > 0)
  {
    Result.Replace(Pos, What.Length(), ByWhat.c_str(), ByWhat.Length());
    Pos = Result.Pos(What.c_str());
  }
  return Result;
}

UnicodeString ExtractShortPathName(const UnicodeString & Path1)
{
  // FIXME
  return Path1;
}

//
// Returns everything, including the trailing Path separator, except the Filename
// part of the Path.
//
// "/foo/bar/baz.txt" --> "/foo/bar/"
UnicodeString ExtractDirectory(const UnicodeString & Path, wchar_t Delimiter)
{
  return Path.SubString(1, Path.RPos(Delimiter) + 1);
}

//
// Returns only the Filename part of the Path.
//
// "/foo/bar/baz.txt" --> "baz.txt"
UnicodeString ExtractFilename(const UnicodeString & Path, wchar_t Delimiter)
{
  return Path.SubString(Path.RPos(Delimiter) + 1);
}

//
// Returns the file's extension, if any. The period is considered part
// of the extension.
//
// "/foo/bar/baz.txt" --> ".txt"
// "/foo/bar/baz" --> ""
UnicodeString ExtractFileExtension(const UnicodeString & Path, wchar_t Delimiter)
{
  UnicodeString Filename = ExtractFilename(Path, Delimiter);
  intptr_t n = Filename.RPos(L'.');
  if (n > 0)
  {
    return Filename.SubString(n);
  }
  return UnicodeString();
}

//
// Modifies the Filename's extension. The period is considered part
// of the extension.
//
// "/foo/bar/baz.txt", ".dat" --> "/foo/bar/baz.dat"
// "/foo/bar/baz.txt", "" --> "/foo/bar/baz"
// "/foo/bar/baz", ".txt" --> "/foo/bar/baz.txt"
//
UnicodeString ChangeFileExtension(const UnicodeString & Path, const UnicodeString & Ext, wchar_t Delimiter)
{
  UnicodeString Filename = ExtractFilename(Path, Delimiter);
  return ExtractDirectory(Path, Delimiter)
         + Filename.SubString(1, Filename.RPos(L'.'))
         + Ext;
}

//---------------------------------------------------------------------------

UnicodeString ExcludeTrailingBackslash(const UnicodeString & Str)
{
  UnicodeString Result = Str;
  if ((Str.Length() > 0) && ((Str[Str.Length()] == L'/') ||
      (Str[Str.Length()] == L'\\')))
  {
    Result.SetLength(Result.Length() - 1);
  }
  return Result;
}

UnicodeString IncludeTrailingBackslash(const UnicodeString & Str)
{
  UnicodeString Result = Str;
  if ((Str.Length() == 0) || ((Str[Str.Length()] != L'/') &&
    (Str[Str.Length()] != L'\\')))
  {
    Result += L'\\';
  }
  return Result;
}

UnicodeString ExtractFileDir(const UnicodeString & Str)
{
  UnicodeString Result;
  intptr_t Pos = Str.LastDelimiter(L"/\\");
  // it used to return Path when no slash was found
  if (Pos > 0)
  {
    Result = Str.SubString(1, Pos);
  }
  else
  {
    Result = (Pos == 1) ? UnicodeString(L"/") : UnicodeString();
  }
  return Result;
}

UnicodeString ExtractFilePath(const UnicodeString & Str)
{
  UnicodeString Result = ::ExtractFileDir(Str);
  return Result;
}

UnicodeString GetCurrentDir()
{
  UnicodeString Result;
  wchar_t Path[MAX_PATH + 1];
  if (FarPlugin)
  {
    FarPlugin->GetFarStandardFunctions().GetCurrentDirectory(sizeof(Path), Path);
  }
  else
  {
    ::GetCurrentDirectory(sizeof(Path), Path);
  }
  Result = Path;
  return Result;
}

//---------------------------------------------------------------------------
UnicodeString StrToHex(const UnicodeString & Str, bool UpperCase, char Separator)
{
  UnicodeString Result;
  for (int i = 1; i <= Str.Length(); i++)
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
UnicodeString HexToStr(const UnicodeString & Hex)
{
  static std::wstring Digits = L"0123456789ABCDEF";
  std::wstring Result;
  size_t L = Hex.Length() - 1;
  if (L % 2 == 0)
  {
    for (int i = 1; i <= Hex.Length(); i += 2)
    {
      size_t P1 = Digits.find_first_of(static_cast<char>(toupper(Hex[i])));
      size_t P2 = Digits.find_first_of(static_cast<char>(toupper(Hex[i + 1])));
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
unsigned int HexToInt(const UnicodeString & Hex, size_t MinChars)
{
  static std::wstring Digits = L"0123456789ABCDEF";
  int Result = 0;
  int I = 1;
  while (I <= Hex.Length())
  {
    size_t A = Digits.find_first_of(static_cast<wchar_t>(toupper(Hex[I])));
    if (A == std::wstring::npos)
    {
      if ((MinChars == NPOS) || ((size_t)I <= MinChars))
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
  UnicodeString Result;
  Result.sprintf(L"%X", Int);
  intptr_t Pad = MinChars - Result.size();
  if (Pad > 0)
  {
    for (int i = 0; i < Pad; i++)
    {
      Result.Insert(L'0', 1);
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
char HexToChar(const UnicodeString & Hex, size_t MinChars)
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
  unsigned int T = DateTimeToTimeStamp(DateTime).Date;
  if ((int)T <= 0)
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
    unsigned int Y = 1;
    while (T >= D400)
    {
      T -= D400;
      Y += 400;
    }
    unsigned int D = 0;
    unsigned int I = 0;
    DivMod(T, D100, I, D);
    if (I == 4)
    {
      I--;
      D += D100;
    }
    Y += I * 100;
    DivMod(D, D4, I, D);
    Y += I * 4;
    DivMod(D, D1, I, D);
    if (I == 4)
    {
      I--;
      D += (Word)D1;
    }
    Y += I;
    Result = IsLeapYear((Word)Y);
    const TDayTable * DayTable = &MonthDays[Result];
    unsigned int M = 1;
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
    Year = (unsigned short)Y;
    Month = (unsigned short)M;
    Day = (unsigned short)D + 1;
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
  Hour = (unsigned short)H;
  Min = (unsigned short)M;
  Sec = (unsigned short)S;
  MSec = (unsigned short)MS;
}

//---------------------------------------------------------------------------
bool TryEncodeDate(int Year, int Month, int Day, TDateTime & Date)
{
  const TDayTable * DayTable = &MonthDays[IsLeapYear((Word)Year)];
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
  if ((Hour < 24) && (Min < 60) && (Sec < 60) && (MSec < 1000))
  {
    Time = (Hour * 3600000 + Min * 60000 + Sec * 1000 + MSec) / static_cast<double>(MSecsPerDay);
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
  return Result;
}
TDateTime StrToDateTime(const UnicodeString & Value)
{
  (void)Value;
  Classes::Error(SNotImplemented, 145);
  return TDateTime();
}

bool TryStrToDateTime(const UnicodeString & StrValue, TDateTime & Value, TFormatSettings & FormatSettings)
{
  (void)StrValue;
  (void)Value;
  (void)FormatSettings;
  Classes::Error(SNotImplemented, 147);
  return false;
}

UnicodeString DateTimeToStr(UnicodeString & Result, const UnicodeString & Format,
  TDateTime DateTime)
{
  (void)Result;
  (void)Format;
  return DateTime.FormatString(L"");
}

UnicodeString DateTimeToString(TDateTime DateTime)
{
  return DateTime.FormatString(L"");
}


//---------------------------------------------------------------------------
// DayOfWeek returns the day of the week of the given date. The Result is an
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
  TDateTime Result = ::EncodeDate(t.wYear, t.wMonth, t.wDay);
  return Result;
}

UnicodeString FormatDateTime(const UnicodeString & Fmt, TDateTime DateTime)
{
  (void)Fmt;
  (void)DateTime;
  UnicodeString Result;
  Classes::Error(SNotImplemented, 150);
  return Result;
}

TDateTime ComposeDateTime(TDateTime Date, TDateTime Time)
{
  TDateTime Result = TDateTime(double(Date));
  Result += double(Time);
  return Result;
}

TDateTime SystemTimeToDateTime(const SYSTEMTIME & SystemTime)
{
  TDateTime Result(0.0);
  Result = ComposeDateTime(EncodeDate(SystemTime.wYear, SystemTime.wMonth, SystemTime.wDay),
    EncodeTime(SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond, SystemTime.wMilliseconds));
  return Result;
}

//---------------------------------------------------------------------------
UnicodeString UnixExcludeLeadingBackslash(const UnicodeString & Path)
{
  UnicodeString Result = Path;
  while (!Result.IsEmpty() && Result[1] == L'/')
  {
    Result.Delete(1, 1);
  }
  return Result;
}

//---------------------------------------------------------------------------
void __fastcall Randomize()
{
  srand(static_cast<unsigned int>(time(NULL)));
}
//---------------------------------------------------------------------------

void IncAMonth(Word & Year, Word & Month, Word & Day, Int64 NumberOfMonths = 1)
{
  Integer Sign;
  if (NumberOfMonths >= 0)
    Sign = 1;
  else
    Sign = -1;
  Year = Year + (NumberOfMonths % 12);
  NumberOfMonths = NumberOfMonths / 12;
  Month += (Word)NumberOfMonths;
  if (Word(Month-1) > 11) // if Month <= 0, word(Month-1) > 11)
  {
    Year += (Word)Sign;
    Month += -12 * (Word)Sign;
  }
  const TDayTable * DayTable = &MonthDays[IsLeapYear(Year)];
  if (Day > (*DayTable)[Month]) Day = static_cast<Word>(*DayTable[Month]);
}

void ReplaceTime(TDateTime &DateTime, const TDateTime NewTime)
{
  DateTime = Trunc(DateTime);
  if (DateTime >= 0)
    DateTime = DateTime + Abs(Frac(NewTime));
  else
    DateTime = DateTime - Abs(Frac(NewTime));
}

TDateTime IncYear(const TDateTime AValue, const Int64 ANumberOfYears)
{
  TDateTime Result;
  Result = IncMonth(AValue, ANumberOfYears * MonthsPerYear);
  return Result;
}

TDateTime IncMonth(const TDateTime AValue, const Int64 NumberOfMonths)
{
  TDateTime Result;
  Word Year, Month, Day;
  DecodeDate(AValue, Year, Month, Day);
  IncAMonth(Year, Month, Day, NumberOfMonths);
  Result = EncodeDate(Year, Month, Day);
  ReplaceTime(Result, AValue);
  return Result;
}

TDateTime IncWeek(const TDateTime AValue, const Int64 ANumberOfWeeks)
{
  TDateTime Result;
  Result = AValue + ANumberOfWeeks * DaysPerWeek;
  return Result;
}

TDateTime IncDay(const TDateTime AValue, const Int64 ANumberOfDays)
{
  TDateTime Result;
  Result = AValue + ANumberOfDays;
  return Result;
}

TDateTime IncHour(const TDateTime AValue, const Int64 ANumberOfHours)
{
  TDateTime Result;
  if (AValue > 0)
    Result = ((AValue * HoursPerDay) + ANumberOfHours) / HoursPerDay;
  else
    Result = ((AValue * HoursPerDay) - ANumberOfHours) / HoursPerDay;
  return Result;
}

TDateTime IncMinute(const TDateTime AValue, const Int64 ANumberOfMinutes)
{
  TDateTime Result;
  if (AValue > 0)
    Result = ((AValue * MinsPerDay) + ANumberOfMinutes) / MinsPerDay;
  else
    Result = ((AValue * MinsPerDay) - ANumberOfMinutes) / MinsPerDay;
  return Result;
}

TDateTime IncSecond(const TDateTime AValue, const Int64 ANumberOfSeconds)
{
  TDateTime Result;
  if (AValue > 0)
    Result = ((AValue * SecsPerDay) + ANumberOfSeconds) / SecsPerDay;
  else
    Result = ((AValue * SecsPerDay) - ANumberOfSeconds) / SecsPerDay;
  return Result;
}

TDateTime IncMilliSecond(const TDateTime AValue, const Int64 ANumberOfMilliSeconds)
{
  TDateTime Result;
  if (AValue > 0)
    Result = ((AValue * MSecsPerDay) + ANumberOfMilliSeconds) / MSecsPerDay;
  else
    Result = ((AValue * MSecsPerDay) - ANumberOfMilliSeconds) / MSecsPerDay;
  return Result;
}

Boolean IsLeapYear(Word Year)
{
  return (Year % 4 == 0) && ((Year % 100 != 0) || (Year % 400 == 0));
}

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

} // namespace Sysutils
