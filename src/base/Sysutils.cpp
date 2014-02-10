//---------------------------------------------------------------------------
#include <iostream>
#include <iomanip>

#include <headers.hpp>
#include <Classes.hpp>
#include <Sysutils.hpp>
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
Exception::Exception(Exception * E) :
  std::runtime_error(E ? E->what() : ""),
  Message(E ? E->Message : L"")
{
}

//---------------------------------------------------------------------------
Exception::Exception(const UnicodeString & Msg) :
  std::runtime_error(""),
  Message(Msg)
{
}

//---------------------------------------------------------------------------
Exception::Exception(const wchar_t * Msg) :
  std::runtime_error(""),
  Message(Msg)
{
}

//---------------------------------------------------------------------------
Exception::Exception(std::exception * E) :
  std::runtime_error(E ? E->what() : "")
{
}

Exception::Exception(const UnicodeString & Msg, int AHelpContext) :
  std::runtime_error(""),
  Message(Msg)
{
  // TODO: FHelpContext = AHelpContext
  (void)AHelpContext;
}

Exception::Exception(Exception * E, int Ident) :
  std::runtime_error(E ? E->what() : "")
{
  Message = FMTLOAD(Ident);
}

Exception::Exception(int Ident) :
  std::runtime_error("")
{
  Message = FMTLOAD(Ident);
}

//---------------------------------------------------------------------------
UnicodeString IntToStr(intptr_t Value)
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
intptr_t StrToInt(const UnicodeString & Value)
{
  __int64 Result = 0;
  if (TryStrToInt(Value, Result))
  {
    return static_cast<intptr_t>(Result);
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

intptr_t StrToIntDef(const UnicodeString & Value, intptr_t DefVal)
{
  __int64 Result = DefVal;
  if (TryStrToInt(Value, Result))
  {
    return static_cast<intptr_t>(Result);
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
  bool Result = !StrValue.empty() && (StrValue.find_first_not_of(L"+-0123456789") == std::string::npos);
  if (Result)
  {
    errno = 0;
    Value = _wtoi64(StrValue.c_str());
    Result = (errno != EINVAL) && (errno != ERANGE);
  }
  return Result;
}

bool TryStrToInt(const std::wstring & StrValue, int & Value)
{
  bool Result = !StrValue.empty()&& (StrValue.find_first_not_of(L"+-0123456789") == std::string::npos);
  if (Result)
  {
    errno = 0;
    Value = _wtoi(StrValue.c_str());
    Result = (errno != EINVAL) && (errno != ERANGE);
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
  intptr_t Len = Result.Length();
  intptr_t Pos = 1;
  while ((Pos <= Len) && (Result[Pos] == L' ')) Pos++;
  if (Pos > 1)
    return Result.SubString(Pos, Len - Pos + 1);
  else
    return Result;
}

UnicodeString TrimRight(const UnicodeString & Str)
{
  UnicodeString Result = Str;
  intptr_t Len = Result.Length();
  while (Len > 0 &&
    ((Result[Len] == L' ') || (Result[Len] == L'\n')))
  {
    Len--;
  }
  Result.SetLength(Len);
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

inline wchar_t UpCase(const wchar_t Ch)
{
  return static_cast<wchar_t>(::toupper(Ch));
}

inline wchar_t LowCase(const wchar_t Ch)
{
  return static_cast<wchar_t>(::tolower(Ch));
}

//---------------------------------------------------------------------------

UnicodeString AnsiReplaceStr(const UnicodeString & Str, const UnicodeString & From,
  const UnicodeString & To)
{
  UnicodeString Result = Str;
  intptr_t Pos = 0;
  while ((Pos = Result.Pos(From)) > 0)
  {
    Result.Replace(Pos, From.Length(), To);
  }
  return Result;
}

intptr_t AnsiPos(const UnicodeString & Str, wchar_t Ch)
{
  intptr_t Result = Str.Pos(Ch);
  return Result;
}

intptr_t Pos(const UnicodeString & Str, const UnicodeString & Substr)
{
  intptr_t Result = Str.Pos(Substr.c_str());
  return Result;
}

UnicodeString StringReplace(const UnicodeString & Str, const UnicodeString & From, const UnicodeString & To, const TReplaceFlags & Flags)
{
  return AnsiReplaceStr(Str, From, To);
}

bool IsDelimiter(const UnicodeString & Delimiters, const UnicodeString & Str, intptr_t Index)
{
  if (Index <= Str.Length())
  {
    wchar_t Ch = Str[Index];
    for (intptr_t I = 1; I <= Delimiters.Length(); I++)
    {
      if (Delimiters[I] == Ch)
      {
        return true;
      }
    }
  }
  return false;
}
//---------------------------------------------------------------------------
intptr_t FirstDelimiter(const UnicodeString & Delimiters, const UnicodeString & Str)
{
  if (Str.Length())
  {
    for (intptr_t I = 1; I <= Str.Length(); ++I)
    {
      if (Str.IsDelimiter(Delimiters, I))
      {
        return I;
      }
    }
  }
  return 0;
}
//---------------------------------------------------------------------------
intptr_t LastDelimiter(const UnicodeString & Delimiters, const UnicodeString & Str)
{
  if (Str.Length())
  {
    for (intptr_t I = Str.Length(); I >= 1; --I)
    {
      if (Str.IsDelimiter(Delimiters, I))
      {
        return I;
      }
    }
  }
  return 0;
}

//---------------------------------------------------------------------------

int StringCmp(const wchar_t * S1, const wchar_t * S2)
{
  return ::CompareString(0, SORT_STRINGSORT, S1, -1, S2, -1) - 2;
}

int StringCmpI(const wchar_t * S1, const wchar_t * S2)
{
  return ::CompareString(0, NORM_IGNORECASE | SORT_STRINGSORT, S1, -1, S2, -1) - 2;
}

//---------------------------------------------------------------------------

intptr_t CompareText(const UnicodeString & Str1, const UnicodeString & Str2)
{
  return StringCmp(Str1.c_str(), Str2.c_str());
}

intptr_t AnsiCompare(const UnicodeString & Str1, const UnicodeString & Str2)
{
  return StringCmp(Str1.c_str(), Str2.c_str());
}

// Case-sensitive compare
intptr_t AnsiCompareStr(const UnicodeString & Str1, const UnicodeString & Str2)
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

intptr_t AnsiCompareText(const UnicodeString & Str1, const UnicodeString & Str2)
{
  return StringCmpI(Str1.c_str(), Str2.c_str());
}

intptr_t AnsiCompareIC(const UnicodeString & Str1, const UnicodeString & Str2)
{
  return AnsiCompareText(Str1, Str2);
}

bool AnsiContainsText(const UnicodeString & Str1, const UnicodeString & Str2)
{
  return ::Pos(Str1, Str2) > 0;
}

void RaiseLastOSError(DWORD LastError)
{
  if (LastError == 0) LastError = ::GetLastError();
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
  UnicodeString Result(20, L'\0');
  swprintf(&Result[1], L"%.2f", Value);
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
  DWORD Res = 0;
  if (::ReadFile(Handle, reinterpret_cast<LPVOID>(Buffer), static_cast<DWORD>(Count), &Res, nullptr))
  {
    Result = Res;
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
  DWORD Res = 0;
  if (::WriteFile(Handle, Buffer, static_cast<DWORD>(Count), &Res, nullptr))
  {
    Result = Res;
  }
  else
  {
    Result = -1;
  }
  return Result;
}

__int64 FileSeek(HANDLE Handle, __int64 Offset, DWORD Origin)
{
  LONG low = Offset & 0xFFFFFFFF;
  LONG high = Offset >> 32;
  low = ::SetFilePointer(Handle, low, &high, Origin);
  return ((_int64)high << 32) + low;
}

//---------------------------------------------------------------------------

bool FileExists(const UnicodeString & FileName)
{
  return FileGetAttr(FileName) != INVALID_FILE_ATTRIBUTES;
}

bool RenameFile(const UnicodeString & From, const UnicodeString & To)
{
  bool Result = ::MoveFile(From.c_str(), To.c_str()) != 0;
  return Result;
}

bool DirectoryExists(const UnicodeString & FileName)
{
  if ((FileName == THISDIRECTORY) || (FileName == PARENTDIRECTORY))
  {
    return true;
  }

  DWORD LocalFileAttrs = FileGetAttr(FileName);

  if ((LocalFileAttrs != INVALID_FILE_ATTRIBUTES) && FLAGSET(LocalFileAttrs, FILE_ATTRIBUTE_DIRECTORY))
  {
    return true;
  }
  return false;
}

UnicodeString FileSearch(const UnicodeString & FileName, const UnicodeString & DirectoryList)
{
  UnicodeString Temp;
  UnicodeString Result;
  Temp = DirectoryList;
  UnicodeString PathSeparators = L"/\\";
  do
  {
    intptr_t I = ::Pos(Temp, PathSeparators);
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

inline DWORD FileGetAttr(const UnicodeString & FileName)
{
  DWORD LocalFileAttrs = ::GetFileAttributes(FileName.c_str());
  return LocalFileAttrs;
}

inline DWORD FileSetAttr(const UnicodeString & FileName, DWORD LocalFileAttrs)
{
  DWORD Result = ::SetFileAttributes(FileName.c_str(), LocalFileAttrs);
  return Result;
}

bool CreateDir(const UnicodeString & Dir)
{
  return ::CreateDirectory(Dir.c_str(), nullptr) != 0;
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

UnicodeString Format(const wchar_t * Format, ...)
{
  va_list Args;
  va_start(Args, Format);
  UnicodeString Result = ::Format(Format, Args);
  va_end(Args);
  return Result.c_str();
}

//---------------------------------------------------------------------------

UnicodeString Format(const wchar_t * Format, va_list Args)
{
  UnicodeString Result;
  if (Format && *Format)
  {
    intptr_t Len = _vscwprintf(Format, Args);
    Result.SetLength(Len + 1);
    // vswprintf(Buf, Len + 1, Format, args);
    vswprintf(const_cast<wchar_t *>(Result.c_str()), Len + 1, Format, Args);
  }
  return Result.c_str();
}

//---------------------------------------------------------------------------

AnsiString Format(const char * Format, ...)
{
  AnsiString Result(64, 0);
  va_list Args;
  va_start(Args, Format);
  Result = ::Format(Format, Args);
  va_end(Args);
  return Result.c_str();
}

//---------------------------------------------------------------------------

AnsiString Format(const char * Format, va_list Args)
{
  AnsiString Result(64, 0);
  if (Format && *Format)
  {
    intptr_t Len = _vscprintf(Format, Args);
    Result.SetLength(Len + 1);
    vsprintf_s(&Result[1], Len + 1, Format, Args);
  }
  return Result.c_str();
}

//---------------------------------------------------------------------------
UnicodeString FmtLoadStr(intptr_t Id, ...)
{
  UnicodeString Result(64, 0);
  wchar_t Format[1024];
  HINSTANCE hInstance = GetGlobalFunctions()->GetInstanceHandle();
  intptr_t Length = ::LoadString(hInstance, static_cast<UINT>(Id),
    Format, static_cast<int>(sizeof(Format)));
  if (!Length)
  {
    DEBUG_PRINTF(L"Unknown resource string id: %d\n", Id);
  }
  else
  {
    va_list Args;
    va_start(Args, Id);
    intptr_t Len = _vscwprintf(Format, Args);
    Result.SetLength(Len + sizeof(wchar_t));
    vswprintf_s(&Result[1], Result.Length(), Format, Args);
    va_end(Args);
  }
  return Result;
}

//---------------------------------------------------------------------------
// Returns the next available word, ignoring whitespace
static const wchar_t *
NextWord(const wchar_t * Input)
{
  static wchar_t buffer[1024];
  static const wchar_t * text = nullptr;

  wchar_t * endOfBuffer = buffer + LENOF(buffer) - 1;
  wchar_t * pBuffer = buffer;

  if (Input)
  {
    text = Input;
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

  intptr_t LenBuffer = 0;
  intptr_t SpaceLeft = MaxWidth;

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
    intptr_t LineCount = 0;

    if (LenBuffer)
    {
      /* second pass, so create the wrapped buffer */
      Result.SetLength(LenBuffer + 1);
      if (Result.Length() == 0)
      {
        break;
      }
    }
    wchar_t * w = const_cast<wchar_t *>(Result.c_str());

    /* for each Word in Text
         if Width(Word) > SpaceLeft
           insert line break before Word in Text
           SpaceLeft := LineWidth - Width(Word)
         else
           SpaceLeft := SpaceLeft - Width(Word) + SpaceWidth
    */
    const wchar_t * s = NextWord(Line.c_str());
    while (*s)
    {
      SpaceLeft = MaxWidth;

      /* force the first word to always be completely copied */
      while (*s)
      {
        if (Result.Length() == 0)
        {
          ++LenBuffer;
        }
        else
        {
          *(w++) = *s;
        }
        --SpaceLeft;
        ++s;
      }
      if (!*s)
      {
        s = NextWord(nullptr);
      }

      /* copy as many words as will fit onto the current line */
      while (*s && static_cast<intptr_t>(wcslen(s) + 1) <= SpaceLeft)
      {
        if (Result.Length() == 0)
        {
          ++LenBuffer;
        }
        --SpaceLeft;

        /* then copy the word */
        while (*s)
        {
          if (Result.Length() == 0)
          {
            ++LenBuffer;
          }
          else
          {
            *(w++) = *s;
          }
          --SpaceLeft;
          ++s;
        }
        if (!*s)
        {
          s = NextWord(nullptr);
        }
      }
      if (!*s)
      {
        s = NextWord(nullptr);
      }

      if (*s)
      {
        /* add a new line here */
        if (Result.Length() == 0)
        {
          ++LenBuffer;
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

      ++LineCount;
    }

    LenBuffer += 2;

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
    if (dynamic_cast<Exception *>(E) != nullptr)
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
void AppendWChar(UnicodeString & Str, const wchar_t Ch)
{
  if (!Str.IsEmpty() && Str[Str.Length()] != Ch)
  {
    Str += Ch;
  }
}

void AppendChar(std::string & Str, const char Ch)
{
  if (!Str.empty() && Str[Str.length() - 1] != Ch)
  {
    Str += Ch;
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
  intptr_t size = ExpandEnvironmentStringsW(Str.c_str(), buf, static_cast<DWORD>(MAX_PATH - 1));
  UnicodeString Result = UnicodeString(buf, size - 1);
  return Result;
}

UnicodeString StringOfChar(const wchar_t Ch, intptr_t Len)
{
  UnicodeString Result;
  if (Len < 0) Len = 0;
  Result.SetLength(Len);
  for (intptr_t I = 1; I <= Len; I++)
  {
    Result[I] = Ch;
  }
  return Result;
}

// void RaiseLastOSError()
// {
// }

char * StrNew(const char * Str)
{
  rsize_t sz = strlen(Str) + 1;
  char * Result = static_cast<char *>(nb_malloc(sizeof(char) * sz));
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

UnicodeString ExpandFileName(const UnicodeString & FileName)
{
  UnicodeString Result;
  UnicodeString Buf(MAX_PATH, 0);
  intptr_t Size = GetFullPathNameW(FileName.c_str(), static_cast<DWORD>(Buf.Length() - 1),
    reinterpret_cast<LPWSTR>(const_cast<wchar_t *>(Buf.c_str())), nullptr);
  if (Size > Buf.Length())
  {
    Buf.SetLength(Size);
    Size = ::GetFullPathNameW(FileName.c_str(), static_cast<DWORD>(Buf.Length() - 1),
      reinterpret_cast<LPWSTR>(const_cast<wchar_t *>(Buf.c_str())), nullptr);
  }
  return UnicodeString(Buf.c_str(), Size);
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

//---------------------------------------------------------------------------
static int FindMatchingFile(TSearchRec & Rec)
{
  TFileTime LocalFileTime = {0};
  int Result = 0;
  while ((Rec.FindData.dwFileAttributes && Rec.ExcludeAttr) != 0)
  {
    if (!::FindNextFileW(Rec.FindHandle, &Rec.FindData))
    {
      Result = GetLastError();
      return Result;
    }
  }
  FileTimeToLocalFileTime(&Rec.FindData.ftLastWriteTime, reinterpret_cast<LPFILETIME>(&LocalFileTime));
  WORD Hi = (Rec.Time & 0xFFFF0000) >> 16;
  WORD Lo = Rec.Time & 0xFFFF;
  FileTimeToDosDateTime(reinterpret_cast<LPFILETIME>(&LocalFileTime), &Hi, &Lo);
  Rec.Time = (Hi << 16) + Lo;
  Rec.Size = Rec.FindData.nFileSizeLow || static_cast<Int64>(Rec.FindData.nFileSizeHigh) << 32;
  Rec.Attr = Rec.FindData.dwFileAttributes;
  Rec.Name = Rec.FindData.cFileName;
  Result = 0;
  return Result;
}

//---------------------------------------------------------------------------
DWORD FindFirst(const UnicodeString & FileName, DWORD LocalFileAttrs, TSearchRec & Rec)
{
  const DWORD faSpecial = faHidden | faSysFile | faDirectory;
  Rec.ExcludeAttr = (~LocalFileAttrs) & faSpecial;
  Rec.FindHandle = ::FindFirstFileW(FileName.c_str(), &Rec.FindData);
  DWORD Result = 0;
  if (Rec.FindHandle != INVALID_HANDLE_VALUE)
  {
    Result = FindMatchingFile(Rec);
    if (Result != 0)
    {
      FindClose(Rec);
    }
  }
  else
  {
    Result = ::GetLastError();
  }
  return Result;
}

DWORD FindNext(TSearchRec & Rec)
{
  DWORD Result = 0;
  if (::FindNextFileW(Rec.FindHandle, &Rec.FindData))
    Result = FindMatchingFile(Rec);
  else
    Result = GetLastError();
  return Result;
}

DWORD FindClose(TSearchRec & Rec)
{
  DWORD Result = 0;
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
    FORMAT_MESSAGE_ARGUMENT_ARRAY, nullptr, ErrorCode, 0,
    static_cast<LPTSTR>(Buffer),
    sizeof(Buffer), nullptr);
  while ((Len > 0) && ((Buffer[Len - 1] != 0) &&
    ((Buffer[Len - 1] <= 32) || (Buffer[Len - 1] == '.'))))
  {
    Len--;
  }
  Result = UnicodeString(Buffer, Len);
  return Result;
}

UnicodeString ReplaceStrAll(const UnicodeString & Str, const UnicodeString & What, const UnicodeString & ByWhat)
{
  UnicodeString Result = Str;
  intptr_t Pos = Result.Pos(What);
  while (Pos > 0)
  {
    Result.Replace(Pos, What.Length(), ByWhat.c_str(), ByWhat.Length());
    Pos = Result.Pos(What);
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
  UnicodeString FileName = ExtractFilename(Path, Delimiter);
  intptr_t N = FileName.RPos(L'.');
  if (N > 0)
  {
    return FileName.SubString(N);
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
  UnicodeString FileName = ExtractFilename(Path, Delimiter);
  return ExtractDirectory(Path, Delimiter)
         + FileName.SubString(1, FileName.RPos(L'.'))
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

UnicodeString IncludeTrailingPathDelimiter(const UnicodeString & Str)
{
  return IncludeTrailingBackslash(Str);
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
  UnicodeString Result = GetGlobalFunctions()->GetCurrentDirectory();
  return Result;
}

//---------------------------------------------------------------------------
UnicodeString StrToHex(const UnicodeString & Str, bool UpperCase, char Separator)
{
  UnicodeString Result;
  for (intptr_t I = 1; I <= Str.Length(); I++)
  {
    Result += CharToHex(static_cast<char>(Str[I]), UpperCase);
    if ((Separator != L'\0') && (I <= Str.Length()))
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
  intptr_t L = Hex.Length() - 1;
  if (L % 2 == 0)
  {
    for (intptr_t I = 1; I <= Hex.Length(); I += 2)
    {
      intptr_t P1 = Digits.find_first_of(static_cast<char>(toupper(Hex[I])));
      intptr_t P2 = Digits.find_first_of(static_cast<char>(toupper(Hex[I + 1])));
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
uintptr_t HexToInt(const UnicodeString & Hex, uintptr_t MinChars)
{
  static std::wstring Digits = L"0123456789ABCDEF";
  uintptr_t Result = 0;
  intptr_t I = 1;
  while (I <= Hex.Length())
  {
    size_t A = Digits.find_first_of(static_cast<wchar_t>(toupper(Hex[I])));
    if (A == std::wstring::npos)
    {
      if ((MinChars == NPOS) || (I <= static_cast<intptr_t>(MinChars)))
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
UnicodeString IntToHex(uintptr_t Int, uintptr_t MinChars)
{
  UnicodeString Result;
  Result.sprintf(L"%X", Int);
  intptr_t Pad = MinChars - Result.Length();
  if (Pad > 0)
  {
    for (intptr_t I = 0; I < Pad; I++)
    {
      Result.Insert(L'0', 1);
    }
  }
  return Result;
}

//---------------------------------------------------------------------------
char HexToChar(const UnicodeString & Hex, uintptr_t MinChars)
{
  return static_cast<char>(HexToInt(Hex, MinChars));
}

//---------------------------------------------------------------------------
static void ConvertError(intptr_t ErrorID)
{
  UnicodeString Msg = FMTLOAD(ErrorID, 0);
  throw EConvertError(Msg);
}

//---------------------------------------------------------------------------
static void DivMod(const uintptr_t Dividend, uintptr_t Divisor,
  uintptr_t & Result, uintptr_t & Remainder)
{
  Result = Dividend / Divisor;
  Remainder = Dividend % Divisor;
}

//---------------------------------------------------------------------------
static bool DecodeDateFully(const TDateTime & DateTime,
  unsigned short & Year, unsigned short & Month, unsigned short & Day,
  unsigned short & DOW)
{
  static const int D1 = 365;
  static const int D4 = D1 * 4 + 1;
  static const int D100 = D4 * 25 - 1;
  static const int D400 = D100 * 4 + 1;
  bool Result = false;
  uintptr_t T = DateTimeToTimeStamp(DateTime).Date;
  if (static_cast<int>(T) <= 0)
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
    uintptr_t Y = 1;
    while (T >= D400)
    {
      T -= D400;
      Y += 400;
    }
    uintptr_t D = 0;
    uintptr_t I = 0;
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
      D += static_cast<Word>(D1);
    }
    Y += I;
    Result = IsLeapYear(static_cast<Word>(Y));
    const TDayTable * DayTable = &MonthDays[Result];
    uintptr_t M = 1;
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
    Year = static_cast<unsigned short>(Y);
    Month = static_cast<unsigned short>(M);
    Day = static_cast<unsigned short>(D + 1);
  }
  return Result;
}

//---------------------------------------------------------------------------
void DecodeDate(const TDateTime & DateTime, unsigned short & Year,
  unsigned short & Month, unsigned short & Day)
{
  unsigned short Dummy = 0;
  DecodeDateFully(DateTime, Year, Month, Day, Dummy);
}

void DecodeTime(const TDateTime & DateTime, unsigned short & Hour,
  unsigned short & Min, unsigned short & Sec, unsigned short & MSec)
{
  uintptr_t MinCount, MSecCount;
  DivMod(DateTimeToTimeStamp(DateTime).Time, 60000, MinCount, MSecCount);
  uintptr_t H, M, S, MS;
  DivMod(MinCount, 60, H, M);
  DivMod(MSecCount, 1000, S, MS);
  Hour = static_cast<unsigned short>(H);
  Min = static_cast<unsigned short>(M);
  Sec = static_cast<unsigned short>(S);
  MSec = static_cast<unsigned short>(MS);
}

//---------------------------------------------------------------------------
bool TryEncodeDate(int Year, int Month, int Day, TDateTime & Date)
{
  const TDayTable * DayTable = &MonthDays[IsLeapYear(static_cast<Word>(Year))];
  if ((Year >= 1) && (Year <= 9999) && (Month >= 1) && (Month <= 12) &&
      (Day >= 1) && (Day <= (*DayTable)[Month - 1]))
  {
    for (int I = 1; I <= Month - 1; I++)
    {
      Day += (*DayTable)[I - 1];
    }
    int I = Year - 1;
    Date = TDateTime((double)(I * 365 + I / 4 - I / 100 + I / 400 + Day - DateDelta));
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
bool TryEncodeTime(unsigned int Hour, unsigned int Min, unsigned int Sec, unsigned int MSec,
  TDateTime & Time)
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

bool TryStrToDateTime(const UnicodeString & StrValue, TDateTime & Value,
  TFormatSettings & FormatSettings)
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
  TDateTime Result = TDateTime((double)Date);
  Result += Time;
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
void Randomize()
{
  srand(static_cast<unsigned int>(time(nullptr)));
}

//---------------------------------------------------------------------------

static void IncAMonth(Word & Year, Word & Month, Word & Day, Int64 NumberOfMonths = 1)
{
  Integer Sign;
  if (NumberOfMonths >= 0)
    Sign = 1;
  else
    Sign = -1;
  Year = Year + (NumberOfMonths % 12);
  NumberOfMonths = NumberOfMonths / 12;
  Month += static_cast<Word>(NumberOfMonths);
  if (static_cast<Word>(Month-1) > 11) // if Month <= 0, word(Month-1) > 11)
  {
    Year += static_cast<Word>(Sign);
    Month += -12 * static_cast<Word>(Sign);
  }
  const TDayTable * DayTable = &MonthDays[IsLeapYear(Year)];
  if (Day > (*DayTable)[Month]) Day = static_cast<Word>(*DayTable[Month]);
}

static void ReplaceTime(TDateTime & DateTime, const TDateTime & NewTime)
{
  DateTime = Trunc(DateTime);
  if (DateTime >= 0)
    DateTime = DateTime + Abs(Frac(NewTime));
  else
    DateTime = DateTime - Abs(Frac(NewTime));
}

TDateTime IncYear(const TDateTime & AValue, const Int64 ANumberOfYears)
{
  TDateTime Result;
  Result = IncMonth(AValue, ANumberOfYears * MonthsPerYear);
  return Result;
}

TDateTime IncMonth(const TDateTime & AValue, const Int64 NumberOfMonths)
{
  TDateTime Result;
  Word Year, Month, Day;
  DecodeDate(AValue, Year, Month, Day);
  IncAMonth(Year, Month, Day, NumberOfMonths);
  Result = EncodeDate(Year, Month, Day);
  ReplaceTime(Result, AValue);
  return Result;
}

TDateTime IncWeek(const TDateTime & AValue, const Int64 ANumberOfWeeks)
{
  TDateTime Result;
  Result = AValue + ANumberOfWeeks * DaysPerWeek;
  return Result;
}

TDateTime IncDay(const TDateTime & AValue, const Int64 ANumberOfDays)
{
  TDateTime Result;
  Result = AValue + ANumberOfDays;
  return Result;
}

TDateTime IncHour(const TDateTime & AValue, const Int64 ANumberOfHours)
{
  TDateTime Result;
  if (AValue > 0)
    Result = ((AValue * HoursPerDay) + ANumberOfHours) / HoursPerDay;
  else
    Result = ((AValue * HoursPerDay) - ANumberOfHours) / HoursPerDay;
  return Result;
}

TDateTime IncMinute(const TDateTime & AValue, const Int64 ANumberOfMinutes)
{
  TDateTime Result;
  if (AValue > 0)
    Result = ((AValue * MinsPerDay) + ANumberOfMinutes) / MinsPerDay;
  else
    Result = ((AValue * MinsPerDay) - ANumberOfMinutes) / MinsPerDay;
  return Result;
}

TDateTime IncSecond(const TDateTime & AValue, const Int64 ANumberOfSeconds)
{
  TDateTime Result;
  if (AValue > 0)
    Result = ((AValue * SecsPerDay) + ANumberOfSeconds) / SecsPerDay;
  else
    Result = ((AValue * SecsPerDay) - ANumberOfSeconds) / SecsPerDay;
  return Result;
}

TDateTime IncMilliSecond(const TDateTime & AValue, const Int64 ANumberOfMilliSeconds)
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
  ::EnterCriticalSection(&FSection);
  FAcquired++;
}

//---------------------------------------------------------------------------
void TCriticalSection::Leave()
{
  FAcquired--;
  ::LeaveCriticalSection(&FSection);
}

//---------------------------------------------------------------------------
UnicodeString StripHotkey(const UnicodeString & AText)
{
  UnicodeString Result = AText;
  intptr_t Len = Result.Length();
  intptr_t Pos = 1;
  while (Pos <= Len)
  {
    if (Result[Pos] == L'&')
    {
      Result.Delete(Pos, 1);
      Len--;
    }
    else
    {
      Pos++;
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
bool StartsText(const UnicodeString & ASubText, const UnicodeString & AText)
{
  return AText.Pos(ASubText) == 1;
}
//---------------------------------------------------------------------------
uintptr_t StrToVersionNumber(const UnicodeString & VersionMumberStr)
{
  uintptr_t Result = 0;
  UnicodeString Version = VersionMumberStr;
  int Shift = 16;
  while (!Version.IsEmpty())
  {
    UnicodeString Num = CutToChar(Version, L'.', true);
    Result += static_cast<uintptr_t>(Num.ToInt()) << Shift;
    if (Shift >= 8) Shift -= 8;
  }
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString VersionNumberToStr(uintptr_t VersionNumber)
{
  DWORD Major = (VersionNumber>>16) & 0xFF;
  DWORD Minor = (VersionNumber>>8) & 0xFF;
  DWORD Revision = (VersionNumber & 0xFF);
  UnicodeString Result = FORMAT(L"%d.%d.%d", Major, Minor, Revision);
  return Result;
}
//---------------------------------------------------------------------------
} // namespace Sysutils
