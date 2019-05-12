
#include <vcl.h>

#include <iomanip>
#include <ctime>

#include <Classes.hpp>
#include <Common.h>
#include <rtlconsts.h>
#include <Sysutils.hpp>
#include <nbutils.h>

UnicodeString MB2W(const char *src, const UINT cp)
{
  UnicodeString Result(src, NBChTraitsCRT<char>::SafeStringLen(src), cp);
  return Result;
}

AnsiString W2MB(const wchar_t *src, const UINT cp)
{
  AnsiString Result(src, NBChTraitsCRT<wchar_t>::SafeStringLen(src), cp);
  return Result;
}

const TDayTable MonthDays[] =
{
  {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
  {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
};

Exception::Exception(TObjectClassId Kind, Exception *E) noexcept :
  std::runtime_error(E ? E->what() : ""),
  FKind(Kind),
  Message(E ? E->Message : L"")
{
}

Exception::Exception(const UnicodeString Msg) noexcept :
  std::runtime_error(""),
  FKind(OBJECT_CLASS_Exception),
  Message(Msg)
{
}

Exception::Exception(TObjectClassId Kind, const wchar_t *Msg) noexcept :
  std::runtime_error(""),
  FKind(Kind),
  Message(Msg)
{
}

Exception::Exception(const wchar_t *Msg) noexcept :
  std::runtime_error(""),
  FKind(OBJECT_CLASS_Exception),
  Message(Msg)
{
}

Exception::Exception(TObjectClassId Kind, const UnicodeString Msg) noexcept :
  std::runtime_error(""),
  FKind(Kind),
  Message(Msg)
{
}

Exception::Exception(TObjectClassId Kind, std::exception *E) noexcept :
  std::runtime_error(E ? E->what() : ""),
  FKind(Kind)
{
}

Exception::Exception(TObjectClassId Kind, const UnicodeString Msg, intptr_t AHelpContext) noexcept :
  std::runtime_error(""),
  FKind(Kind),
  Message(Msg)
{
  TODO("FHelpContext = AHelpContext");
  (void)AHelpContext;
}

Exception::Exception(TObjectClassId Kind, Exception *E, intptr_t Ident) noexcept :
  std::runtime_error(E ? E->what() : ""),
  FKind(Kind)
{
  Message = FMTLOAD(Ident);
}

Exception::Exception(TObjectClassId Kind, intptr_t Ident) noexcept :
  std::runtime_error(""),
  FKind(Kind)
{
  Message = FMTLOAD(Ident);
}

void RaiseLastOSError(DWORD LastError)
{
  if (LastError == 0)
    LastError = ::GetLastError();
  UnicodeString ErrorMsg;
  if (LastError != 0)
  {
    ErrorMsg = FMTLOAD(SOSError, LastError, ::SysErrorMessage(LastError));
  }
  else
  {
    ErrorMsg = FMTLOAD(SUnkOSError);
  }
  throw EOSError(ErrorMsg, LastError);
}

int RandSeed = 0;

int random(int range)
{
  return nb::ToInt(nb::ToDouble(rand()) / (nb::ToDouble(RAND_MAX) / range));
}

void Randomize()
{
  srand(nb::ToUInt32(time(nullptr)));
}


namespace Sysutils {

UnicodeString IntToStr(intptr_t Value)
{
  UnicodeString Result = FORMAT("%d", Value);
  return Result;
}

UnicodeString UIntToStr(uintptr_t Value)
{
  UnicodeString Result = FORMAT("%u", Value);
  return Result;
}

UnicodeString Int64ToStr(int64_t Value)
{
  UnicodeString Result = FORMAT("%lld", Value);
  return Result;
}

intptr_t StrToIntPtr(const UnicodeString Value)
{
  int64_t Result = 0;
  if (TryStrToInt64(Value, Result))
  {
    return nb::ToIntPtr(Result);
  }
  return 0;
}

int64_t StrToInt64(const UnicodeString Value)
{
  int64_t Result = 0;
  if (TryStrToInt64(Value, Result))
  {
    return Result;
  }
  return 0;
}

intptr_t StrToIntDef(const UnicodeString Value, intptr_t DefVal)
{
  int64_t Result = DefVal;
  if (TryStrToInt64(Value, Result))
  {
    return nb::ToIntPtr(Result);
  }
  return DefVal;
}

int64_t StrToInt64Def(const UnicodeString Value, int64_t DefVal)
{
  int64_t Result = DefVal;
  if (TryStrToInt64(Value, Result))
  {
    return Result;
  }
  return DefVal;
}

bool TryStrToInt64(const UnicodeString StrValue, int64_t &Value)
{
  bool Result = !StrValue.IsEmpty(); // && (StrValue.FindFirstNotOf(L"+-0123456789") == -1);
  if (Result)
  {
    errno = 0;
    Value = _wtoi64(StrValue.c_str());
    Result = (errno != EINVAL) && (errno != ERANGE);
    if (Result && (Value == 0))
      Result = (StrValue == L"0");
  }
  return Result;
}

bool TryStrToInt(const UnicodeString StrValue, intptr_t &Value)
{
  int64_t Val{0};
  bool res = TryStrToInt64(StrValue, Val);
  Value = nb::ToIntPtr(Val);
  return res;
}

UnicodeString Trim(const UnicodeString Str)
{
  UnicodeString Result = TrimRight(TrimLeft(Str));
  return Result;
}

UnicodeString TrimLeft(const UnicodeString Str)
{
  UnicodeString Result = Str;
  const intptr_t Len = Result.Length();
  intptr_t Pos = 1;
  while ((Pos <= Len) && (Result[Pos] == L' '))
    Pos++;
  if (Pos > 1)
    return Result.SubString(Pos, Len - Pos + 1);
  return Result;
}

UnicodeString TrimRight(const UnicodeString Str)
{
  UnicodeString Result = Str;
  intptr_t Len = Result.Length();
  while (Len > 0 &&
    ((Result[Len] == L' ') || (Result[Len] == L'\n') || (Result[Len] == L'\r') || (Result[Len] == L'\x00')))
  {
    Len--;
  }
  Result.SetLength(Len);
  return Result;
}

UnicodeString UpperCase(const UnicodeString Str)
{
  UnicodeString Result(Str);
  return Result.MakeUpper();
}

UnicodeString LowerCase(const UnicodeString Str)
{
  UnicodeString Result(Str);
  return Result.MakeLower();
}

wchar_t UpCase(const wchar_t Ch)
{
  return static_cast<wchar_t>(::towupper(Ch));
}

wchar_t LowCase(const wchar_t Ch)
{
  return static_cast<wchar_t>(::towlower(Ch));
}

UnicodeString AnsiReplaceStr(const UnicodeString Str, const UnicodeString From,
  const UnicodeString To)
{
  UnicodeString Result = Str;
  intptr_t Pos;
  while ((Pos = Result.Pos(From)) > 0)
  {
    Result.Replace(Pos, From.Length(), To);
  }
  return Result;
}

intptr_t AnsiPos(const UnicodeString Str, wchar_t Ch)
{
  const intptr_t Result = Str.Pos(Ch);
  return Result;
}

intptr_t Pos(const UnicodeString Str, const UnicodeString Substr)
{
  const intptr_t Result = Str.Pos(Substr.c_str());
  return Result;
}

UnicodeString StringReplaceAll(const UnicodeString Str, const UnicodeString From, const UnicodeString To)
{
  return AnsiReplaceStr(Str, From, To);
}

bool IsDelimiter(const UnicodeString Delimiters, const UnicodeString Str, intptr_t AIndex)
{
  if (AIndex <= Str.Length())
  {
    const wchar_t Ch = Str[AIndex];
    for (intptr_t Index = 1; Index <= Delimiters.Length(); ++Index)
    {
      if (Delimiters[Index] == Ch)
      {
        return true;
      }
    }
  }
  return false;
}

intptr_t FirstDelimiter(const UnicodeString Delimiters, const UnicodeString Str)
{
  if (Str.Length())
  {
    for (intptr_t Index = 1; Index <= Str.Length(); ++Index)
    {
      if (Str.IsDelimiter(Delimiters, Index))
      {
        return Index;
      }
    }
  }
  return 0;
}

intptr_t LastDelimiter(const UnicodeString Delimiters, const UnicodeString Str)
{
  if (Str.Length())
  {
    for (intptr_t Index = Str.Length(); Index >= 1; --Index)
    {
      if (Str.IsDelimiter(Delimiters, Index))
      {
        return Index;
      }
    }
  }
  return 0;
}

int StringCmp(const wchar_t *S1, const wchar_t *S2)
{
  return ::CompareString(0, SORT_STRINGSORT, S1, -1, S2, -1) - 2;
}

int StringCmpI(const wchar_t *S1, const wchar_t *S2)
{
  return ::CompareString(0, NORM_IGNORECASE | SORT_STRINGSORT, S1, -1, S2, -1) - 2;
}

intptr_t CompareText(const UnicodeString Str1, const UnicodeString Str2)
{
  return StringCmp(Str1.c_str(), Str2.c_str());
}

intptr_t AnsiCompare(const UnicodeString Str1, const UnicodeString Str2)
{
  return StringCmp(Str1.c_str(), Str2.c_str());
}

// Case-sensitive compare
intptr_t AnsiCompareStr(const UnicodeString Str1, const UnicodeString Str2)
{
  return StringCmp(Str1.c_str(), Str2.c_str());
}

bool AnsiSameText(const UnicodeString Str1, const UnicodeString Str2)
{
  return StringCmpI(Str1.c_str(), Str2.c_str()) == 0;
}

bool SameText(const UnicodeString Str1, const UnicodeString Str2)
{
  return AnsiSameText(Str1, Str2);
}

intptr_t AnsiCompareText(const UnicodeString Str1, const UnicodeString Str2)
{
  return StringCmpI(Str1.c_str(), Str2.c_str());
}

intptr_t AnsiCompareIC(const UnicodeString Str1, const UnicodeString Str2)
{
  return AnsiCompareText(Str1, Str2);
}

bool AnsiSameStr(const UnicodeString Str1, const UnicodeString Str2)
{
  return AnsiCompareIC(Str1, Str2) == 0;
}

bool AnsiContainsText(const UnicodeString Str1, const UnicodeString Str2)
{
  return ::Pos(Str1, Str2) > 0;
}

bool ContainsStr(const AnsiString Str1, const AnsiString Str2)
{
  return Str1.Pos(Str2) > 0;
}

bool ContainsStr(const UnicodeString Str1, const UnicodeString Str2)
{
  return Str1.Pos(Str2) > 0;
}

bool ContainsText(const UnicodeString Str1, const UnicodeString Str2)
{
  return AnsiContainsText(Str1, Str2);
}

UnicodeString RightStr(const UnicodeString Str, intptr_t ACount)
{
  UnicodeString Result = Str.SubString(Str.Length() - ACount, ACount);
  return Result;
}

intptr_t PosEx(const UnicodeString SubStr, const UnicodeString Str, intptr_t Offset)
{
  UnicodeString S = Str.SubString(Offset);
  const intptr_t Result = S.Pos(SubStr) + Offset;
  return Result;
}

UnicodeString UTF8ToString(const RawByteString Str)
{
  return UnicodeString(Str.c_str(), Str.GetLength(), CP_UTF8);
}

UnicodeString UTF8ToString(const char *Str, intptr_t Len)
{
  if (!Str || !*Str || !Len)
  {
    return UnicodeString(L"");
  }

  const intptr_t reqLength = ::MultiByteToWideChar(CP_UTF8, 0, Str, nb::ToInt(Len), nullptr, 0);
  UnicodeString Result;
  if (reqLength)
  {
    Result.SetLength(reqLength);
    ::MultiByteToWideChar(CP_UTF8, 0, Str, nb::ToInt(Len), const_cast<LPWSTR>(Result.c_str()), nb::ToInt(reqLength));
    Result.SetLength(Result.Length() - 1); //remove NULL character
  }
  return Result;
}

double StrToFloat(const UnicodeString Value)
{
  return StrToFloatDef(Value, 0.0);
}

double StrToFloatDef(const UnicodeString Value, double DefVal)
{
  double Result;
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

UnicodeString FormatFloat(const UnicodeString /*Format*/, double Value)
{
  UnicodeString Result(20, L'\0');
  swprintf_s(&Result[1], Result.Length(), L"%.2f", Value);
  PackStr(Result);
  return Result;
}

bool IsZero(double Value)
{
  return fabs(Value) < std::numeric_limits<double>::epsilon();
}

TTimeStamp DateTimeToTimeStamp(const TDateTime &DateTime)
{
  TTimeStamp Result{};
  double intpart;
  const double fractpart = modf(DateTime, &intpart);
  Result.Time = nb::ToInt(fractpart * MSecsPerDay + 0.5);
  Result.Date = nb::ToInt(intpart + DateDelta);
  return Result;
}

int64_t FileRead(HANDLE AHandle, void *Buffer, int64_t Count)
{
  int64_t Result;
  DWORD Res = 0;
  if (::ReadFile(AHandle, reinterpret_cast<LPVOID>(Buffer), nb::ToDWord(Count), &Res, nullptr))
  {
    Result = Res;
  }
  else
  {
    Result = -1;
  }
  return Result;
}

int64_t FileWrite(HANDLE AHandle, const void *Buffer, int64_t Count)
{
  int64_t Result;
  DWORD Res = 0;
  if (::WriteFile(AHandle, Buffer, nb::ToDWord(Count), &Res, nullptr))
  {
    Result = Res;
  }
  else
  {
    Result = -1;
  }
  return Result;
}

int64_t FileSeek(HANDLE AHandle, int64_t Offset, DWORD Origin)
{
  LONG low = Offset & 0xFFFFFFFF;
  LONG high = Offset >> 32;
  low = ::SetFilePointer(AHandle, low, &high, Origin);
  return (nb::ToInt64(high) << 32) + low;
}

bool SysUtulsFileExists(const UnicodeString AFileName)
{
  return SysUtulsFileGetAttr(AFileName) != INVALID_FILE_ATTRIBUTES;
}

bool SysUtulsRenameFile(const UnicodeString From, const UnicodeString To)
{
  const bool Result = ::MoveFileW(ApiPath(From).c_str(), ApiPath(To).c_str()) != FALSE;
  return Result;
}

bool SysUtulsDirectoryExists(const UnicodeString ADir)
{
  if ((ADir == THISDIRECTORY) || (ADir == PARENTDIRECTORY))
  {
    return true;
  }

  const DWORD LocalFileAttrs = SysUtulsFileGetAttr(ADir);

  if ((LocalFileAttrs != INVALID_FILE_ATTRIBUTES) && FLAGSET(LocalFileAttrs, FILE_ATTRIBUTE_DIRECTORY))
  {
    return true;
  }
  return false;
}

UnicodeString SysUtulsFileSearch(const UnicodeString AFileName, const UnicodeString DirectoryList)
{
  UnicodeString Result;
  UnicodeString Temp = DirectoryList;
  const UnicodeString PathSeparators = L"/\\";
  do
  {
    intptr_t Index = ::Pos(Temp, PathSeparators);
    while ((Temp.Length() > 0) && (Index == 0))
    {
      Temp.Delete(1, 1);
      Index = ::Pos(Temp, PathSeparators);
    }
    Index = ::Pos(Temp, PathSeparators);
    if (Index > 0)
    {
      Result = Temp.SubString(1, Index - 1);
      Temp.Delete(1, Index);
    }
    else
    {
      Result = Temp;
      Temp.Clear();
    }
    Result = ::IncludeTrailingBackslash(Result);
    Result = Result + AFileName;
    if (!::SysUtulsFileExists(Result))
    {
      Result.Clear();
    }
  }
  while (!(Temp.Length() == 0) || (Result.Length() != 0));
  return Result;
}

void SysUtulsFileAge(const UnicodeString AFileName, TDateTime &ATimestamp)
{
  WIN32_FIND_DATA FindData;
  const HANDLE LocalFileHandle = ::FindFirstFileW(ApiPath(AFileName).c_str(), &FindData);
  if (LocalFileHandle != INVALID_HANDLE_VALUE)
  {
    ATimestamp =
      UnixToDateTime(
        ConvertTimestampToUnixSafe(FindData.ftLastWriteTime, dstmUnix),
        dstmUnix);
    ::FindClose(LocalFileHandle);
  }
}

DWORD SysUtulsFileGetAttr(const UnicodeString AFileName, bool /*FollowLink*/)
{
  TODO("FollowLink");
  const DWORD LocalFileAttrs = ::GetFileAttributesW(ApiPath(AFileName).c_str());
  return LocalFileAttrs;
}

bool SysUtulsFileSetAttr(const UnicodeString AFileName, DWORD LocalFileAttrs)
{
  const bool Result = ::SetFileAttributesW(ApiPath(AFileName).c_str(), LocalFileAttrs) != FALSE;
  return Result;
}

bool SysUtulsCreateDir(const UnicodeString ADir, LPSECURITY_ATTRIBUTES SecurityAttributes)
{
  return ::CreateDirectoryW(ApiPath(ADir).c_str(), SecurityAttributes) != FALSE;
}

bool SysUtulsRemoveDir(const UnicodeString ADir)
{
  return ::RemoveDirectoryW(ApiPath(ADir).c_str()) != FALSE;
}

bool SysUtulsMoveFile(const UnicodeString LocalFileName, const UnicodeString NewLocalFileName, DWORD AFlags)
{
  return ::MoveFileExW(ApiPath(LocalFileName).c_str(), ApiPath(NewLocalFileName).c_str(), AFlags) != FALSE;
}

bool SysUtulsForceDirectories(const UnicodeString ADir)
{
  bool Result = true;
  if (ADir.IsEmpty())
  {
    return false;
  }
  UnicodeString Dir = ::ExcludeTrailingBackslash(ADir);
  if ((Dir.Length() < 3 + 4) || ::SysUtulsDirectoryExists(Dir)) // \\?\C:
  {
    return Result;
  }
  if (::ExtractFilePath(Dir).IsEmpty())
  {
    return ::SysUtulsCreateDir(Dir);
  }
  Result = ::SysUtulsForceDirectories(::ExtractFilePath(Dir)) && ::SysUtulsCreateDir(Dir);
  return Result;
}

bool SysUtulsRemoveFile(const UnicodeString AFileName)
{
  ::DeleteFileW(ApiPath(AFileName).c_str());
  return !::SysUtulsFileExists(AFileName);
}

// Returns the next available word, ignoring whitespace
static const wchar_t *
NextWord(const wchar_t *Input)
{
  static UnicodeString Buffer;
  wchar_t *pBuffer = Buffer.SetLength(1024);
  static const wchar_t *text = nullptr;

  wchar_t *endOfBuffer = ToWChar(Buffer) + Buffer.GetLength() - 1;

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

  return Buffer.c_str();
}

UnicodeString WrapText(const UnicodeString Line, intptr_t MaxWidth)
{
  UnicodeString Result;

  intptr_t LenBuffer = 0;

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
    wchar_t *w = nullptr;

    if (LenBuffer)
    {
      /* second pass, so create the wrapped buffer */
      w = Result.SetLength(LenBuffer + 1);
      if (Result.Length() == 0)
      {
        break;
      }
    }

    /* for each Word in Text
         if Width(Word) > SpaceLeft
           insert line break before Word in Text
           SpaceLeft := LineWidth - Width(Word)
         else
           SpaceLeft := SpaceLeft - Width(Word) + SpaceWidth
    */
    const wchar_t *s = NextWord(Line.c_str());
    while (s && *s)
    {
      intptr_t SpaceLeft = MaxWidth;

      /* force the first word to always be completely copied */
      while (s && *s)
      {
        if (Result.Length() == 0)
        {
          ++LenBuffer;
        }
        else
        {
          if (w) *(w++) = *s;
        }
        --SpaceLeft;
        ++s;
      }
      if (!*s)
      {
        s = NextWord(nullptr);
      }

      /* copy as many words as will fit onto the current line */
      while (*s && (nb::StrLength(s) + 1) <= SpaceLeft)
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
            if (w) *(w++) = *s;
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
          if (w) *(w++) = L'\n';
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

UnicodeString TranslateExceptionMessage(Exception *E)
{
  if (E)
  {
    if (isa<Exception>(E))
    {
      return dyn_cast<Exception>(E)->Message;
    }
    return E->what();
  }
  return UnicodeString();
}

void AppendWChar(UnicodeString &Str, const wchar_t Ch)
{
  if (!Str.IsEmpty() && Str[Str.Length()] != Ch)
  {
    Str += Ch;
  }
}

void AppendPathDelimiterW(UnicodeString &Str)
{
  if (!Str.IsEmpty() && Str[Str.Length()] != L'/' && Str[Str.Length()] != L'\\')
  {
    Str += L"\\";
  }
}

UnicodeString ExpandEnvVars(const UnicodeString Str)
{
  UnicodeString Buf(NB_MAX_PATH, 0);
  const intptr_t Size = ::ExpandEnvironmentStringsW(Str.c_str(), ToWChar(Buf), nb::ToDWord(NB_MAX_PATH - 1));
  UnicodeString Result = UnicodeString(Buf.c_str(), Size - 1);
  return Result;
}

UnicodeString StringOfChar(const wchar_t Ch, intptr_t Len)
{
  UnicodeString Result;
  if (Len < 0)
    Len = 0;
  Result.SetLength(Len);
  for (intptr_t Index = 1; Index <= Len; ++Index)
  {
    Result[Index] = Ch;
  }
  return Result;
}

UnicodeString ChangeFileExt(const UnicodeString AFileName, const UnicodeString AExt,
  wchar_t Delimiter)
{
  UnicodeString Result = ::ChangeFileExtension(AFileName, AExt, Delimiter);
  return Result;
}

UnicodeString ExtractFileExt(const UnicodeString AFileName)
{
  UnicodeString Result = ::ExtractFileExtension(AFileName, L'.');
  return Result;
}

UnicodeString ExpandFileName(const UnicodeString AFileName)
{
  UnicodeString Buf(NB_MAX_PATH + 1, 0);
  intptr_t Size = ::GetFullPathNameW(ApiPath(AFileName).c_str(), nb::ToDWord(Buf.Length() - 1),
      reinterpret_cast<LPWSTR>(ToWChar(Buf)), nullptr);
  if (Size > Buf.Length())
  {
    Buf.SetLength(Size);
    Size = ::GetFullPathNameW(ApiPath(AFileName).c_str(), nb::ToDWord(Buf.Length() - 1),
        reinterpret_cast<LPWSTR>(ToWChar(Buf)), nullptr);
  }
  UnicodeString Result = UnicodeString(Buf.c_str(), Size);
  return Result;
}

static UnicodeString GetUniversalName(const UnicodeString AFileName)
{
  UnicodeString Result = AFileName;
  return Result;
}

UnicodeString ExpandUNCFileName(const UnicodeString AFileName)
{
  UnicodeString Result = ExpandFileName(AFileName);
  if ((Result.Length() >= 3) && (Result[1] == L':') && (::UpCase(Result[1]) >= L'A') &&
    (::UpCase(Result[1]) <= L'Z'))
  {
    Result = GetUniversalName(Result);
  }
  return Result;
}

static DWORD FindMatchingFile(TSearchRec &Rec)
{
  TFileTime LocalFileTime{};
  DWORD Result;
  while ((Rec.FindData.dwFileAttributes && Rec.ExcludeAttr) != 0)
  {
    if (!::FindNextFileW(Rec.FindHandle, &Rec.FindData))
    {
      Result = ::GetLastError();
      return Result;
    }
  }
  FileTimeToLocalFileTime(&Rec.FindData.ftLastWriteTime, reinterpret_cast<LPFILETIME>(&LocalFileTime));
  WORD Hi = (Rec.Time & 0xFFFF0000) >> 16;
  WORD Lo = Rec.Time & 0xFFFF;
  FileTimeToDosDateTime(reinterpret_cast<LPFILETIME>(&LocalFileTime), &Hi, &Lo);
  Rec.Time = (nb::ToIntPtr(Hi) << 16) + Lo;
  Rec.Size = Rec.FindData.nFileSizeLow || nb::ToInt64(Rec.FindData.nFileSizeHigh) << 32;
  Rec.Attr = Rec.FindData.dwFileAttributes;
  Rec.Name = Rec.FindData.cFileName;
  Result = ERROR_SUCCESS;
  return Result;
}

bool Win32Check(bool RetVal)
{
  if (!RetVal)
  {
    RaiseLastOSError();
  }
  return RetVal;
}

UnicodeString SysErrorMessage(intptr_t ErrorCode)
{
  wchar_t Buffer[255];
  intptr_t Len = ::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM |
      FORMAT_MESSAGE_ARGUMENT_ARRAY, nullptr, nb::ToInt(ErrorCode), 0,
      static_cast<LPTSTR>(Buffer),
      _countof(Buffer), nullptr);
  while ((Len > 0) && ((Buffer[Len - 1] != 0) &&
      ((Buffer[Len - 1] <= 32) || (Buffer[Len - 1] == L'.'))))
  {
    Len--;
  }
  UnicodeString Result = UnicodeString(Buffer, Len);
  return Result;
}

UnicodeString ReplaceStrAll(const UnicodeString Str, const UnicodeString What, const UnicodeString ByWhat)
{
  UnicodeString Result = Str;
  if (What != ByWhat)
  {
    intptr_t Pos = Result.Pos(What);
    while (Pos > 0)
    {
      Result.Replace(Pos, What.Length(), ByWhat.c_str(), ByWhat.Length());
      Pos = Result.Pos(What);
    }
  }
  return Result;
}

UnicodeString ExtractShortPathName(const UnicodeString APath)
{
  // FIXME
  return APath;
}

//
// Returns everything, including the trailing Path separator, except the Filename
// part of the Path.
//
// "/foo/bar/baz.txt" --> "/foo/bar/"
UnicodeString ExtractDirectory(const UnicodeString APath, wchar_t Delimiter)
{
  UnicodeString Result = APath.SubString(1, APath.RPos(Delimiter));
  return Result;
}

//
// Returns only the Filename part of the Path.
//
// "/foo/bar/baz.txt" --> "baz.txt"
UnicodeString ExtractFilename(const UnicodeString APath, wchar_t Delimiter)
{
  return APath.SubString(APath.RPos(Delimiter) + 1);
}

//
// Returns the file's extension, if any. The period is considered part
// of the extension.
//
// "/foo/bar/baz.txt" --> ".txt"
// "/foo/bar/baz" --> ""
UnicodeString ExtractFileExtension(const UnicodeString APath, wchar_t Delimiter)
{
  UnicodeString FileName = ::ExtractFilename(APath, Delimiter);
  const intptr_t N = FileName.RPos(L'.');
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
UnicodeString ChangeFileExtension(const UnicodeString APath, const UnicodeString Ext, wchar_t Delimiter)
{
  UnicodeString FileName = ::ExtractFilename(APath, Delimiter);
  if (FileName.RPos(L'.') > 1)
  {
    return ExtractDirectory(APath, Delimiter) +
      FileName.SubString(1, FileName.RPos(L'.') - 1) +
      Ext;
  }
  return ExtractDirectory(APath, Delimiter) +
    FileName +
    Ext;
}

UnicodeString ExcludeTrailingBackslash(const UnicodeString Str)
{
  UnicodeString Result = Str;
  if ((Result.Length() > 0) && ((Result[Result.Length()] == L'/') ||
      (Result[Result.Length()] == L'\\')))
  {
    Result.SetLength(Result.Length() - 1);
  }
  return Result;
}

UnicodeString IncludeTrailingBackslash(const UnicodeString Str)
{
  UnicodeString Result = Str;
  const intptr_t L = Result.Length();
  if ((L == 0) || ((Result[L] != L'/') && (Result[L] != L'\\')))
  {
    Result += L'\\';
  }
  return Result;
}

UnicodeString IncludeTrailingPathDelimiter(const UnicodeString Str)
{
  return ::IncludeTrailingBackslash(Str);
}

UnicodeString ExtractFileDir(const UnicodeString Str)
{
  UnicodeString Result;
  const intptr_t Pos = Str.LastDelimiter(L"/\\");
  // it used to return Path when no slash was found
  if (Pos > 1)
  {
    Result = Str.SubString(1, Pos);
  }
  else
  {
    Result = (Pos == 1) ? UnicodeString(ROOTDIRECTORY) : UnicodeString();
  }
  return Result;
}

UnicodeString ExtractFilePath(const UnicodeString Str)
{
  UnicodeString Result = ::ExtractFileDir(Str);
  return Result;
}

UnicodeString GetCurrentDir()
{
  UnicodeString Result = GetGlobals()->GetCurrDirectory();
  return Result;
}

UnicodeString StrToHex(const UnicodeString Str, bool UpperCase, wchar_t Separator)
{
  UnicodeString Result;
  for (intptr_t Index = 1; Index <= Str.Length(); ++Index)
  {
    Result += CharToHex(Str[Index], UpperCase);
    if ((Separator != L'\0') && (Index <= Str.Length()))
    {
      Result += Separator;
    }
  }
  return Result;
}

UnicodeString HexToStr(const UnicodeString Hex)
{
  UnicodeString Digits = "0123456789ABCDEF";
  UnicodeString Result;
  const intptr_t L = Hex.Length() - 1;
  if (L % 2 == 0)
  {
    for (intptr_t Index = 1; Index <= Hex.Length(); Index += 2)
    {
      const intptr_t P1 = Digits.FindFirstOf(::UpCase(Hex[Index]));
      const intptr_t P2 = Digits.FindFirstOf(::UpCase(Hex[Index + 1]));
      if ((P1 == NPOS) || (P2 == NPOS))
      {
        Result.Clear();
        break;
      }
      Result += static_cast<wchar_t>((P1 - 1) * 16 + P2 - 1);
    }
  }
  return Result;
}

uintptr_t HexToIntPtr(const UnicodeString Hex, uintptr_t MinChars)
{
  UnicodeString Digits = "0123456789ABCDEF";
  uintptr_t Result = 0;
  intptr_t Index = 1;
  while (Index <= Hex.Length())
  {
    const intptr_t A = Digits.FindFirstOf(UpCase(Hex[Index]));
    if (A == NPOS)
    {
      if ((nb::ToIntPtr(MinChars) == NPOS) || (Index <= nb::ToIntPtr(MinChars)))
      {
        Result = 0;
      }
      break;
    }

    Result = (Result * 16) + (nb::ToInt(A) - 1);

    ++Index;
  }
  return Result;
}

UnicodeString IntToHex(uintptr_t Int, uintptr_t MinChars)
{
  UnicodeString Result = FORMAT("%X", Int);
  const intptr_t Pad = MinChars - Result.Length();
  if (Pad > 0)
  {
    for (intptr_t Index = 0; Index < Pad; ++Index)
    {
      Result.Insert(L'0', 1);
    }
  }
  return Result;
}

char HexToChar(const UnicodeString Hex, uintptr_t MinChars)
{
  return static_cast<char>(HexToIntPtr(Hex, MinChars));
}

static void ConvertError(intptr_t ErrorID)
{
  const UnicodeString Msg = FMTLOAD(ErrorID, 0);
  throw EConvertError(Msg);
}

static void DivMod(const uintptr_t Dividend, uintptr_t Divisor,
  uintptr_t &Result, uintptr_t &Remainder)
{
  Result = Dividend / Divisor;
  Remainder = Dividend % Divisor;
}

static bool DecodeDateFully(const TDateTime &DateTime,
  uint16_t &Year, uint16_t &Month, uint16_t &Day,
  uint16_t &DOW)
{
  static const int D1 = 365;
  static const int D4 = D1 * 4 + 1;
  static const int D100 = D4 * 25 - 1;
  static const int D400 = D100 * 4 + 1;
  int T = DateTimeToTimeStamp(DateTime).Date;
  if (T <= 0)
  {
    Year = 0;
    Month = 0;
    Day = 0;
    DOW = 0;
    return false;
  }
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
    D += nb::ToWord(D1);
  }
  Y += I;
  const bool Result = IsLeapYear(nb::ToWord(Y));
  const TDayTable *DayTable = &MonthDays[Result];
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
  Year = static_cast<uint16_t>(Y);
  Month = static_cast<uint16_t>(M);
  Day = static_cast<uint16_t>(D + 1);
  return Result;
}

void DecodeDate(const TDateTime &DateTime, uint16_t &Year,
  uint16_t &Month, uint16_t &Day)
{
  uint16_t Dummy = 0;
  DecodeDateFully(DateTime, Year, Month, Day, Dummy);
}

void DecodeTime(const TDateTime &DateTime, uint16_t &Hour,
  uint16_t &Min, uint16_t &Sec, uint16_t &MSec)
{
  uintptr_t MinCount, MSecCount;
  DivMod(DateTimeToTimeStamp(DateTime).Time, 60000, MinCount, MSecCount);
  uintptr_t H, M, S, MS;
  DivMod(MinCount, 60, H, M);
  DivMod(MSecCount, 1000, S, MS);
  Hour = static_cast<uint16_t>(H);
  Min = static_cast<uint16_t>(M);
  Sec = static_cast<uint16_t>(S);
  MSec = static_cast<uint16_t>(MS);
}

static bool TryEncodeDate(int Year, int Month, int Day, TDateTime &Date)
{
  const TDayTable *DayTable = &MonthDays[IsLeapYear(nb::ToWord(Year))];
  if ((Year >= 1) && (Year <= 9999) && (Month >= 1) && (Month <= 12) &&
    (Day >= 1) && (Day <= (*DayTable)[Month - 1]))
  {
    for (int Index = 1; Index <= Month - 1; Index++)
    {
      Day += (*DayTable)[Index - 1];
    }
    const int Idx = Year - 1;
    Date = TDateTime(nb::ToDouble(Idx * 365 + Idx / 4 - Idx / 100 + Idx / 400 + Day - DateDelta));
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

static bool TryEncodeTime(uint32_t Hour, uint32_t Min, uint32_t Sec, uint32_t MSec,
  TDateTime &Time)
{
  bool Result = false;
  if ((Hour < 24) && (Min < 60) && (Sec < 60) && (MSec < 1000))
  {
    Time = (Hour * 3600000 + Min * 60000 + Sec * 1000 + MSec) / nb::ToDouble(MSecsPerDay);
    Result = true;
  }
  return Result;
}

TDateTime EncodeTime(uint32_t Hour, uint32_t Min, uint32_t Sec, uint32_t MSec)
{
  TDateTime Result;
  if (!TryEncodeTime(Hour, Min, Sec, MSec, Result))
  {
    ::ConvertError(STimeEncodeError);
  }
  return Result;
}

TDateTime StrToDateTime(const UnicodeString Value)
{
  (void)Value;
  ThrowNotImplemented(145);
  return TDateTime();
}

bool TryStrToDateTime(const UnicodeString StrValue, TDateTime &Value,
  TFormatSettings &FormatSettings)
{
  (void)StrValue;
  (void)Value;
  (void)FormatSettings;
  ThrowNotImplemented(147);
  return false;
}

UnicodeString DateTimeToStr(UnicodeString &Result, const UnicodeString Format,
  const TDateTime &DateTime)
{
  (void)Result;
  (void)Format;
  return DateTime.FormatString(const_cast<wchar_t *>(L""));
}

UnicodeString DateTimeToString(const TDateTime &DateTime)
{
  return DateTime.FormatString(const_cast<wchar_t *>(L""));
}

// DayOfWeek returns the day of the week of the given date. The Result is an
// integer between 1 and 7, corresponding to Sunday through Saturday.
// This function is not ISO 8601 compliant, for that see the DateUtils unit.
uint32_t DayOfWeek(const TDateTime &DateTime)
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

UnicodeString FormatDateTime(const UnicodeString Fmt, const TDateTime &ADateTime)
{
  (void)Fmt;
  UnicodeString Result;
  uint16_t Year;
  uint16_t Month;
  uint16_t Day;
  uint16_t Hour;
  uint16_t Minutes;
  uint16_t Seconds;
  uint16_t Milliseconds;

  ADateTime.DecodeDate(Year, Month, Day);
  ADateTime.DecodeTime(Hour, Minutes, Seconds, Milliseconds);

  if (Fmt == L"ddddd tt")
  {
    /*
    return FormatDateTime(L"ddddd tt",
        EncodeDateVerbose(
            static_cast<uint16_t>(ValidityTime.Year), static_cast<uint16_t>(ValidityTime.Month),
            static_cast<uint16_t>(ValidityTime.Day)) +
        EncodeTimeVerbose(
            static_cast<uint16_t>(ValidityTime.Hour), static_cast<uint16_t>(ValidityTime.Min),
            static_cast<uint16_t>(ValidityTime.Sec), 0));
    */
    uint16_t Y, M, D, H, Mm, S, MS;
    TDateTime DateTime =
      EncodeDateVerbose(Year, Month, Day) +
      EncodeTimeVerbose(Hour, Minutes, Seconds, Milliseconds);
    DateTime.DecodeDate(Y, M, D);
    DateTime.DecodeTime(H, Mm, S, MS);
    Result = FORMAT("%02d.%02d.%04d %02d:%02d:%02d ", D, M, Y, H, Mm, S);
  }
  else if (Fmt == L"nnzzz")
  {
    Result = FORMAT("%02d%03d ", Seconds, Milliseconds);
  }
  else if (Fmt == L" yyyy-mm-dd hh:nn:ss.zzz ")
  {
    Result = FORMAT(" %04d-%02d-%02d %02d:%02d:%02d.%03d ", Year, Month, Day, Hour, Minutes, Seconds, Milliseconds);
  }
  else if (Fmt == L"h:nn:ss")
  {
    Result = FORMAT("%02d:%02d:%02d", Hour, Minutes, Seconds);
  }
  else
  {
    ThrowNotImplemented(150);
  }
  return Result;
}

static TDateTime ComposeDateTime(const TDateTime &Date, const TDateTime &Time)
{
  TDateTime Result = TDateTime(Date);
  Result += Time;
  return Result;
}

TDateTime SystemTimeToDateTime(const SYSTEMTIME &SystemTime)
{
  TDateTime Result = ComposeDateTime(EncodeDate(SystemTime.wYear, SystemTime.wMonth, SystemTime.wDay),
      EncodeTime(SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond, SystemTime.wMilliseconds));
  return Result;
}

UnicodeString UnixExcludeLeadingBackslash(const UnicodeString APath)
{
  UnicodeString Result = APath;
  while (!Result.IsEmpty() && Result[1] == L'/')
  {
    Result.Delete(1, 1);
  }
  return Result;
}

static void IncAMonth(Word &Year, Word &Month, Word &Day, Int64 NumberOfMonths = 1)
{
  Integer Sign;
  if (NumberOfMonths >= 0)
    Sign = 1;
  else
    Sign = -1;
  Year = Year + (NumberOfMonths % 12);
  NumberOfMonths = NumberOfMonths / 12;
  Month += nb::ToWord(NumberOfMonths);
  if (nb::ToWord(Month - 1) > 11) // if Month <= 0, word(Month-1) > 11)
  {
    Year += nb::ToWord(Sign);
    Month += -12 * nb::ToWord(Sign);
  }
  const TDayTable *DayTable = &MonthDays[IsLeapYear(Year)];
  if (Day > (*DayTable)[Month])
    Day = nb::ToWord(*DayTable[Month]);
}

static void ReplaceTime(TDateTime &DateTime, const TDateTime &NewTime)
{
  DateTime = Trunc(DateTime);
  if (DateTime >= 0)
    DateTime = DateTime + Abs(Frac(NewTime));
  else
    DateTime = DateTime - Abs(Frac(NewTime));
}

TDateTime IncYear(const TDateTime &AValue, const Int64 ANumberOfYears)
{
  TDateTime Result = IncMonth(AValue, ANumberOfYears * MonthsPerYear);
  return Result;
}

TDateTime IncMonth(const TDateTime &AValue, const Int64 NumberOfMonths)
{
  Word Year, Month, Day;
  DecodeDate(AValue, Year, Month, Day);
  IncAMonth(Year, Month, Day, NumberOfMonths);
  TDateTime Result = EncodeDate(Year, Month, Day);
  ReplaceTime(Result, AValue);
  return Result;
}

TDateTime IncWeek(const TDateTime &AValue, const Int64 ANumberOfWeeks)
{
  TDateTime Result(AValue + ANumberOfWeeks * DaysPerWeek);
  return Result;
}

TDateTime IncDay(const TDateTime &AValue, const Int64 ANumberOfDays)
{
  TDateTime Result(AValue + ANumberOfDays);
  return Result;
}

TDateTime IncHour(const TDateTime &AValue, const Int64 ANumberOfHours)
{
  TDateTime Result;
  if (AValue > 0)
    Result = ((AValue * HoursPerDay) + ANumberOfHours) / HoursPerDay;
  else
    Result = ((AValue * HoursPerDay) - ANumberOfHours) / HoursPerDay;
  return Result;
}

TDateTime IncMinute(const TDateTime &AValue, const Int64 ANumberOfMinutes)
{
  TDateTime Result;
  if (AValue > 0)
    Result = ((AValue * MinsPerDay) + ANumberOfMinutes) / MinsPerDay;
  else
    Result = ((AValue * MinsPerDay) - ANumberOfMinutes) / MinsPerDay;
  return Result;
}

TDateTime IncSecond(const TDateTime &AValue, const Int64 ANumberOfSeconds)
{
  TDateTime Result;
  if (AValue > 0)
    Result = ((AValue * SecsPerDay) + ANumberOfSeconds) / SecsPerDay;
  else
    Result = ((AValue * SecsPerDay) - ANumberOfSeconds) / SecsPerDay;
  return Result;
}

TDateTime IncMilliSecond(const TDateTime &AValue, const Int64 ANumberOfMilliSeconds)
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

UnicodeString StripHotkey(const UnicodeString AText)
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

bool StartsText(const UnicodeString ASubText, const UnicodeString AText)
{
  return AText.Pos(ASubText) == 1;
}

uintptr_t StrToVersionNumber(const UnicodeString VersionMumberStr)
{
  uintptr_t Result = 0;
  UnicodeString Version = VersionMumberStr;
  int Shift = 16;
  while (!Version.IsEmpty())
  {
    UnicodeString Num = CutToChar(Version, L'.', true);
    Result += nb::ToUIntPtr(Num.ToIntPtr()) << Shift;
    if (Shift >= 8)
      Shift -= 8;
  }
  return Result;
}

UnicodeString VersionNumberToStr(uintptr_t VersionNumber)
{
  const DWORD Major = (VersionNumber >> 16) & 0xFF;
  const DWORD Minor = (VersionNumber >> 8) & 0xFF;
  const DWORD Revision = (VersionNumber & 0xFF);
  UnicodeString Result = FORMAT("%d.%d.%d", Major, Minor, Revision);
  return Result;
}

TFormatSettings::TFormatSettings(LCID /*LCID*/) noexcept :
  CurrencyFormat(0),
  NegCurrFormat(0),
  ThousandSeparator(0),
  DecimalSeparator(0),
  CurrencyDecimals(0),
  DateSeparator(0),
  TimeSeparator(0),
  ListSeparator(0),
  TwoDigitYearCenturyWindow(0)
{
}

UnicodeString TPath::Combine(const UnicodeString APath, const UnicodeString AFileName)
{
  UnicodeString Result = ::IncludeTrailingBackslash(APath) + AFileName;
  return Result;
}

} // namespace Sysutils

namespace base {

DWORD FindFirst(const UnicodeString AFileName, DWORD LocalFileAttrs, TSearchRec &Rec)
{
  const DWORD faSpecial = faHidden | faSysFile | faDirectory;
  Rec.ExcludeAttr = (~LocalFileAttrs) & faSpecial;
  Rec.FindHandle = ::FindFirstFileW(ApiPath(AFileName).c_str(), &Rec.FindData);
  DWORD Result;
  if (Rec.FindHandle != INVALID_HANDLE_VALUE)
  {
    Result = FindMatchingFile(Rec);
    if (Result != ERROR_SUCCESS)
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

DWORD FindNext(TSearchRec &Rec)
{
  DWORD Result;
  if (::FindNextFileW(Rec.FindHandle, &Rec.FindData))
    Result = FindMatchingFile(Rec);
  else
    Result = ::GetLastError();
  return Result;
}

DWORD FindClose(TSearchRec &Rec)
{
  DWORD Result = 0;
  if (Rec.FindHandle != INVALID_HANDLE_VALUE)
  {
    Result = FALSE != ::FindClose(Rec.FindHandle);
    Rec.FindHandle = INVALID_HANDLE_VALUE;
  }
  return Result;
}

} // namespace base
