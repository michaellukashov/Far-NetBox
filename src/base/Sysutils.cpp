
#include <vcl.h>
#include <System.ShlObj.hpp>

#include <iomanip>
#include <ctime>

#include <Classes.hpp>
#include <Common.h>
#include <rtlconsts.h>
#include <Sysutils.hpp>
#include <nbutils.h>

UnicodeString MB2W(const char * src, const UINT cp)
{
  const UnicodeString Result(src, NBChTraitsCRT<char>::SafeStringLen(src), cp);
  return Result;
}

AnsiString W2MB(const wchar_t * src, const UINT cp)
{
  AnsiString Result(src, NBChTraitsCRT<wchar_t>::SafeStringLen(src), cp);
  return Result;
}

/*const TDayTable MonthDays[] =
{
  {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
  {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
};*/

Exception::Exception(TObjectClassId Kind, const Exception * E) noexcept :
  Exception(Kind, UnicodeString(E ? E->Message : ""))
{
}

Exception::Exception(const UnicodeString & Msg) noexcept :
  Exception(OBJECT_CLASS_Exception, Msg)
{
}

Exception::Exception(TObjectClassId Kind, const wchar_t * Msg) noexcept :
  Exception(Kind, UnicodeString(Msg))
{
}

Exception::Exception(const wchar_t * Msg) noexcept :
  Exception(OBJECT_CLASS_Exception, UnicodeString(Msg))
{
}

Exception::Exception(TObjectClassId Kind, const UnicodeString & Msg) noexcept :
  FKind(Kind),
  Message(Msg)
{
}

Exception::Exception(TObjectClassId Kind, const std::exception * E) noexcept :
  Exception(Kind, UnicodeString(E ? E->what() : ""))
{
}

Exception::Exception(TObjectClassId Kind, const UnicodeString & Msg, int32_t AHelpContext) noexcept :
  Exception(Kind, Msg)
{
  TODO("use HelpContext");
  (void)AHelpContext;
}

Exception::Exception(TObjectClassId Kind, const Exception * E, int32_t Ident) noexcept :
  Exception(Kind, UnicodeString(E ? E->Message : ""))
{
  Message = FMTLOAD(Ident);
}

Exception::Exception(TObjectClassId Kind, int32_t Ident) noexcept :
  Exception(Kind, UnicodeString())
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

int32_t RandSeed = 0;

int32_t random(int32_t range)
{
  return nb::ToInt32(nb::ToDouble(rand()) / (nb::ToDouble(RAND_MAX) / range));
}

void Randomize()
{
  srand(nb::ToUInt32(time(nullptr)));
}


namespace Sysutils {

UnicodeString EmptyStr;

UnicodeString IntToStr(int32_t Value)
{
  const UnicodeString Result = FORMAT("%d", Value);
  return Result;
}

UnicodeString UIntToStr(uint32_t Value)
{
  const UnicodeString Result = FORMAT("%u", Value);
  return Result;
}

UnicodeString Int64ToStr(int64_t Value)
{
  const UnicodeString Result = FORMAT("%lld", Value);
  return Result;
}

int32_t StrToIntPtr(const UnicodeString & Value)
{
  int64_t Result = 0;
  if (TryStrToInt64(Value, Result))
  {
    return nb::ToInt32(Result);
  }
  return 0;
}

int64_t StrToInt64(const UnicodeString & Value)
{
  int64_t Result = 0;
  if (TryStrToInt64(Value, Result))
  {
    return Result;
  }
  return 0;
}

int32_t StrToIntDef(const UnicodeString & Value, int32_t DefVal)
{
  int64_t Result = DefVal;
  if (TryStrToInt64(Value, Result))
  {
    return nb::ToInt32(Result);
  }
  return DefVal;
}

int64_t StrToInt64Def(const UnicodeString & Value, int64_t DefVal)
{
  int64_t Result = DefVal;
  if (TryStrToInt64(Value, Result))
  {
    return Result;
  }
  return DefVal;
}

bool TryStrToInt64(const UnicodeString & StrValue, int64_t & Value)
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

bool TryStrToInt(const UnicodeString & StrValue, int32_t & Value)
{
  int64_t Val{0};
  const bool Result = TryStrToInt64(StrValue, Val);
  Value = nb::ToInt32(Val);
  return Result;
}

UnicodeString Trim(const UnicodeString & Str)
{
  const UnicodeString Result = TrimRight(TrimLeft(Str));
  return Result;
}

UnicodeString TrimLeft(const UnicodeString & Str)
{
  UnicodeString Result = Str;
  const int32_t Len = Result.Length();
  int32_t Pos = 1;
  while ((Pos <= Len) && (Result[Pos] == L' '))
    Pos++;
  if (Pos > 1)
    return Result.SubString(Pos, Len - Pos + 1);
  return Result;
}

UnicodeString TrimRight(const UnicodeString & Str)
{
  UnicodeString Result = Str;
  int32_t Len = Result.Length();
  while (Len > 0 &&
    ((Result[Len] == L' ') || (Result[Len] == L'\n') || (Result[Len] == L'\r') || (Result[Len] == L'\x00')))
  {
    Len--;
  }
  Result.SetLength(Len);
  return Result;
}

UnicodeString UpperCase(const UnicodeString & Str)
{
  UnicodeString Result(Str);
  return Result.MakeUpper();
}

UnicodeString LowerCase(const UnicodeString & Str)
{
  UnicodeString Result(Str);
  return Result.MakeLower();
}

UnicodeString AnsiLowerCase(const UnicodeString & Str)
{
  return LowerCase(Str);
}

wchar_t UpCase(const wchar_t Ch)
{
  return static_cast<wchar_t>(::towupper(Ch));
}

wchar_t LowCase(const wchar_t Ch)
{
  return static_cast<wchar_t>(::towlower(Ch));
}

UnicodeString AnsiReplaceStr(const UnicodeString & Str, const UnicodeString & From,
  const UnicodeString & To)
{
  UnicodeString Result = Str;
  int32_t Pos;
  while ((Pos = Result.Pos(From)) > 0)
  {
    Result.Replace(Pos, From.Length(), To);
  }
  return Result;
}

int32_t AnsiPos(const UnicodeString & Str, wchar_t Ch)
{
  const int32_t Result = Str.Pos(Ch);
  return Result;
}

int32_t Pos(const UnicodeString & Str, const UnicodeString & Substr)
{
  const int32_t Result = Str.Pos(Substr.c_str());
  return Result;
}

UnicodeString StringReplaceAll(const UnicodeString & Str, const UnicodeString & From, const UnicodeString & To)
{
  return AnsiReplaceStr(Str, From, To);
}

bool IsDelimiter(const UnicodeString & Delimiters, const UnicodeString & Str, int32_t AIndex)
{
  if (AIndex <= Str.Length())
  {
    const wchar_t Ch = Str[AIndex];
    for (int32_t Index = 1; Index <= Delimiters.Length(); ++Index)
    {
      if (Delimiters[Index] == Ch)
      {
        return true;
      }
    }
  }
  return false;
}

int32_t FirstDelimiter(const UnicodeString & Delimiters, const UnicodeString & Str)
{
  if (Str.Length())
  {
    for (int32_t Index = 1; Index <= Str.Length(); ++Index)
    {
      if (Str.IsDelimiter(Delimiters, Index))
      {
        return Index;
      }
    }
  }
  return 0;
}

int32_t LastDelimiter(const UnicodeString & Delimiters, const UnicodeString & Str)
{
  if (Str.Length())
  {
    for (int32_t Index = Str.Length(); Index >= 1; --Index)
    {
      if (Str.IsDelimiter(Delimiters, Index))
      {
        return Index;
      }
    }
  }
  return 0;
}

int32_t StringCmp(const wchar_t * S1, const wchar_t * S2)
{
  return ::CompareString(0, SORT_STRINGSORT, S1, -1, S2, -1) - 2;
}

int32_t StringCmpI(const wchar_t * S1, const wchar_t * S2)
{
  return ::CompareString(0, NORM_IGNORECASE | SORT_STRINGSORT, S1, -1, S2, -1) - 2;
}

int32_t CompareText(const UnicodeString & Str1, const UnicodeString & Str2)
{
  return StringCmp(Str1.c_str(), Str2.c_str());
}

int32_t AnsiCompare(const UnicodeString & Str1, const UnicodeString & Str2)
{
  return StringCmp(Str1.c_str(), Str2.c_str());
}

// Case-sensitive compare
int32_t AnsiCompareStr(const UnicodeString & Str1, const UnicodeString & Str2)
{
  return StringCmp(Str1.c_str(), Str2.c_str());
}

bool AnsiSameText(const UnicodeString & Str1, const UnicodeString & Str2)
{
  return StringCmpI(Str1.c_str(), Str2.c_str()) == 0;
}

bool SameText(const UnicodeString & Str1, const UnicodeString & Str2)
{
  return AnsiSameText(Str1, Str2);
}

int32_t AnsiCompareText(const UnicodeString & Str1, const UnicodeString & Str2)
{
  return StringCmpI(Str1.c_str(), Str2.c_str());
}

int32_t AnsiCompareIC(const UnicodeString & Str1, const UnicodeString & Str2)
{
  return AnsiCompareText(Str1, Str2);
}

bool SameStr(const UnicodeString & Str1, const UnicodeString & Str2)
{
  return AnsiCompareIC(Str1, Str2) == 0;
}

bool AnsiSameStr(const UnicodeString & Str1, const UnicodeString & Str2)
{
  return AnsiCompareIC(Str1, Str2) == 0;
}

bool AnsiContainsText(const UnicodeString & Str1, const UnicodeString & Str2)
{
  return ::Pos(Str1, Str2) > 0;
}

bool ContainsStr(const AnsiString & Str1, const AnsiString & Str2)
{
  return Str1.Pos(Str2) > 0;
}

bool ContainsStr(const UnicodeString & Str1, const UnicodeString & Str2)
{
  return Str1.Pos(Str2) > 0;
}

bool ContainsText(const UnicodeString & Str1, const UnicodeString & Str2)
{
  return AnsiContainsText(Str1, Str2);
}

UnicodeString RightStr(const UnicodeString & Str, int32_t ACount)
{
  const UnicodeString Result = Str.SubString(Str.Length() - ACount + 1, ACount);
  return Result;
}

int32_t PosEx(const UnicodeString & SubStr, const UnicodeString & Str, int32_t Offset)
{
  const UnicodeString S = Str.SubString(Offset);
  const int32_t Result = S.Pos(SubStr) + Offset;
  return Result;
}

int32_t FindDelimiter(const UnicodeString & Delimiters, const UnicodeString & Str)
{
  return FirstDelimiter(Delimiters, Str);
}

UnicodeString UTF8ToString(const RawByteString & Str)
{
  return UnicodeString(Str.c_str(), Str.GetLength(), CP_UTF8);
}

UnicodeString UTF8ToString(const char * Str, int32_t Len)
{
  if (!Str || !*Str || !Len)
  {
    return UnicodeString(L"");
  }

  const int32_t reqLength = ::MultiByteToWideChar(CP_UTF8, 0, Str, nb::ToInt32(Len), nullptr, 0);
  UnicodeString Result;
  if (reqLength)
  {
    Result.SetLength(reqLength);
    ::MultiByteToWideChar(CP_UTF8, 0, Str, nb::ToInt32(Len), const_cast<LPWSTR>(Result.c_str()), nb::ToInt32(reqLength));
    Result.SetLength(Result.Length() - 1); //remove NULL character
  }
  return Result;
}

double StrToFloat(const UnicodeString & Value)
{
  return StrToFloatDef(Value, 0.0);
}

double StrToFloatDef(const UnicodeString & Value, double DefVal)
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

UnicodeString FormatFloat(const UnicodeString & /*Format*/, double Value)
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

TTimeStamp DateTimeToTimeStamp(const TDateTime & DateTime)
{
  TTimeStamp Result{};
  double intpart{0.0};
  const double fractpart = modf(DateTime, &intpart);
  Result.Time = nb::ToInt32(fractpart * MSecsPerDay + 0.5);
  Result.Date = nb::ToInt32(intpart + DateDelta);
  return Result;
}

int64_t FileRead(HANDLE AHandle, void * Buffer, int64_t Count)
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

int64_t FileWrite(HANDLE AHandle, const void * Buffer, int64_t Count)
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
  const LONG Low = static_cast<LONG>(Offset) & 0xFFFFFFFF;
  LONG High = static_cast<LONG>(Offset >> 32);
  const DWORD Res = ::SetFilePointer(AHandle, Low, &High, Origin);
  return (nb::ToInt64(High) << 32) + nb::ToInt64(Res);
}

bool FileExists(const UnicodeString & AFileName)
{
  return FileGetAttr(AFileName) != INVALID_FILE_ATTRIBUTES;
}

bool RenameFile(const UnicodeString & From, const UnicodeString & To)
{
  const bool Result = ::MoveFileW(ApiPath(From).c_str(), ApiPath(To).c_str()) != FALSE;
  return Result;
}

bool DirectoryExists(const UnicodeString & ADir)
{
  if ((ADir == THISDIRECTORY) || (ADir == PARENTDIRECTORY))
  {
    return true;
  }

  const DWORD LocalFileAttrs = FileGetAttr(ADir);

  if ((LocalFileAttrs != INVALID_FILE_ATTRIBUTES) && FLAGSET(LocalFileAttrs, FILE_ATTRIBUTE_DIRECTORY))
  {
    return true;
  }
  return false;
}

UnicodeString FileSearch(const UnicodeString & AFileName, const UnicodeString & DirectoryList)
{
  UnicodeString Result;
  UnicodeString Temp = DirectoryList;
  const UnicodeString PathSeparators = L"/\\";
  do
  {
    int32_t Index = ::Pos(Temp, PathSeparators);
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
    if (!base::FileExists(Result))
    {
      Result.Clear();
    }
  }
  while (!(Temp.Length() == 0) || (Result.Length() != 0));
  return Result;
}

void FileAge(const UnicodeString & AFileName, TDateTime & ATimestamp)
{
  WIN32_FIND_DATA FindData;
  const HANDLE LocalFileHandle = ::FindFirstFileW(ApiPath(AFileName).c_str(), &FindData);
  if (CheckHandle(LocalFileHandle))
  {
    ATimestamp =
      UnixToDateTime(
        ConvertTimestampToUnixSafe(FindData.ftLastWriteTime, dstmUnix),
        dstmUnix);
    ::FindClose(LocalFileHandle);
  }
}

DWORD FileGetAttr(const UnicodeString & AFileName, bool /*FollowLink*/)
{
  TODO("FollowLink");
  const DWORD LocalFileAttrs = ::GetFileAttributesW(ApiPath(AFileName).c_str());
  return LocalFileAttrs;
}

bool FileSetAttr(const UnicodeString & AFileName, DWORD LocalFileAttrs)
{
  const bool Result = ::SetFileAttributesW(ApiPath(AFileName).c_str(), LocalFileAttrs) != FALSE;
  return Result;
}

bool CreateDir(const UnicodeString & ADir, LPSECURITY_ATTRIBUTES SecurityAttributes)
{
  return ::CreateDirectoryW(ApiPath(ADir).c_str(), SecurityAttributes) != FALSE;
}

bool RemoveDir(const UnicodeString & ADir)
{
  return ::RemoveDirectoryW(ApiPath(ADir).c_str()) != FALSE;
}

bool MoveFile(const UnicodeString & LocalFileName, const UnicodeString & NewLocalFileName, DWORD AFlags)
{
  return ::MoveFileExW(ApiPath(LocalFileName).c_str(), ApiPath(NewLocalFileName).c_str(), AFlags) != FALSE;
}

bool ForceDirectories(const UnicodeString & ADir)
{
  bool Result = true;
  if (ADir.IsEmpty())
  {
    return false;
  }
  const UnicodeString Dir = ::ExcludeTrailingBackslash(ADir);
  if ((Dir.Length() < 3 + 4) || base::DirectoryExists(Dir)) // \\?\C:
  {
    return Result;
  }
  if (::ExtractFilePath(Dir).IsEmpty())
  {
    return ::CreateDir(Dir);
  }
  Result = ::ForceDirectories(::ExtractFilePath(Dir)) && ::CreateDir(Dir);
  return Result;
}

bool RemoveFile(const UnicodeString & AFileName)
{
  ::DeleteFileW(ApiPath(AFileName).c_str());
  return !base::FileExists(AFileName);
}

// Returns the next available word, ignoring whitespace
static const wchar_t *
NextWord(const wchar_t * Input)
{
  static UnicodeString Buffer;
  wchar_t * pBuffer = Buffer.SetLength(1024);
  static const wchar_t * Text = nullptr;

  const wchar_t * endOfBuffer = ToWCharPtr(Buffer) + Buffer.GetLength() - 1;

  if (Input)
  {
    Text = Input;
  }

  if (Text)
  {
    /* add leading spaces */
    while (iswspace(*Text))
    {
      *(pBuffer++) = *(Text++);
    }

    /* copy the word to our static buffer */
    while (*Text && !iswspace(*Text) && pBuffer < endOfBuffer)
    {
      *(pBuffer++) = *(Text++);
    }
  }

  *pBuffer = 0;

  return Buffer.c_str();
}

UnicodeString WrapText(const UnicodeString & Line, int32_t MaxWidth)
{
  UnicodeString Result;

  int32_t LenBuffer = 0;

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
    int32_t LineCount = 0;
    wchar_t * W = nullptr;

    if (LenBuffer)
    {
      /* second pass, so create the wrapped buffer */
      W = Result.SetLength(LenBuffer + 1);
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
    const wchar_t * S = NextWord(Line.c_str());
    while (S && *S)
    {
      int32_t SpaceLeft = MaxWidth;

      /* force the first word to always be completely copied */
      while (S && *S)
      {
        if (Result.Length() == 0)
        {
          ++LenBuffer;
        }
        else
        {
          if (W) *(W++) = *S;
        }
        --SpaceLeft;
        ++S;
      }
      if (S && !*S)
      {
        S = NextWord(nullptr);
      }

      /* copy as many words as will fit onto the current line */
      while (*S && (nb::StrLength(S) + 1) <= SpaceLeft)
      {
        if (Result.Length() == 0)
        {
          ++LenBuffer;
        }
        --SpaceLeft;

        /* then copy the word */
        while (*S)
        {
          if (Result.Length() == 0)
          {
            ++LenBuffer;
          }
          else
          {
            if (W) *(W++) = *S;
          }
          --SpaceLeft;
          ++S;
        }
        if (!*S)
        {
          S = NextWord(nullptr);
        }
      }
      if (!*S)
      {
        S = NextWord(nullptr);
      }

      if (*S)
      {
        /* add a new line here */
        if (Result.Length() == 0)
        {
          ++LenBuffer;
        }
        else
        {
          if (W) *(W++) = L'\n';
        }
        // Skip whitespace before first word on new line
        while (iswspace(*S))
        {
          ++S;
        }
      }

      ++LineCount;
    }

    LenBuffer += 2;

    if (W)
    {
      *W = 0;
    }
  }

  return Result;
}

UnicodeString TranslateExceptionMessage(Exception * E)
{
  if (E)
  {
    if (rtti::isa<Exception>(E))
    {
      return rtti::dyn_cast_or_null<Exception>(E)->Message;
    }
    return E->Message;
  }
  return UnicodeString();
}

void AppendWChar(UnicodeString & Str, const wchar_t Ch)
{
  if (!Str.IsEmpty() && Str[Str.Length()] != Ch)
  {
    Str += Ch;
  }
}

void AppendPathDelimiter(UnicodeString & Str)
{
  if (!Str.IsEmpty() && (Str[Str.Length()] != Slash) && (Str[Str.Length()] != Backslash))
  {
    Str += BACKSLASH;
  }
}

UnicodeString ExpandEnvVars(const UnicodeString & Str)
{
  const UnicodeString Buf(nb::NB_MAX_PATH, 0);
  const int32_t Size = ::ExpandEnvironmentStringsW(Str.c_str(), ToWCharPtr(Buf), nb::ToDWord(nb::NB_MAX_PATH - 1));
  const UnicodeString Result = UnicodeString(Buf.c_str(), Size - 1);
  return Result;
}

UnicodeString StringOfChar(const wchar_t Ch, int32_t Len)
{
  UnicodeString Result;
  if (Len < 0)
    Len = 0;
  Result.SetLength(Len);
  for (int32_t Index = 1; Index <= Len; ++Index)
  {
    Result[Index] = Ch;
  }
  return Result;
}

UnicodeString ChangeFileExt(const UnicodeString & AFileName, const UnicodeString & AExt,
  wchar_t Delimiter)
{
  const UnicodeString Result = ::ChangeFileExtension(AFileName, AExt, Delimiter);
  return Result;
}

UnicodeString ExtractFileExt(const UnicodeString & AFileName)
{
  const UnicodeString Result = ::ExtractFileExtension(AFileName, L'.');
  return Result;
}

UnicodeString ExpandFileName(const UnicodeString & AFileName)
{
  UnicodeString Buf(nb::NB_MAX_PATH + 1, 0);
  int32_t Size = ::GetFullPathNameW(AFileName.c_str(), nb::ToDWord(Buf.Length() - 1),
    reinterpret_cast<LPWSTR>(ToWCharPtr(Buf)), nullptr);
  if (Size > Buf.Length())
  {
    Buf.SetLength(Size);
    Size = ::GetFullPathNameW(AFileName.c_str(), nb::ToDWord(Buf.Length() - 1),
      reinterpret_cast<LPWSTR>(ToWCharPtr(Buf)), nullptr);
  }
  const UnicodeString Result = UnicodeString(Buf.c_str(), Size);
  return Result;
}

static UnicodeString GetUniversalName(const UnicodeString & AFileName)
{
  const UnicodeString Result = AFileName;
  return Result;
}

UnicodeString ExpandUNCFileName(const UnicodeString & AFileName)
{
  UnicodeString Result = ExpandFileName(AFileName);
  if ((Result.Length() >= 3) && (Result[1] == L':') && (::UpCase(Result[1]) >= L'A') &&
    (::UpCase(Result[1]) <= L'Z'))
  {
    Result = GetUniversalName(Result);
  }
  return Result;
}

static DWORD FindMatchingFile(TSearchRec & Rec)
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
  ::FileTimeToLocalFileTime(&Rec.FindData.ftLastWriteTime, reinterpret_cast<LPFILETIME>(&LocalFileTime));
  WORD Hi = (Rec.Time & 0xFFFF0000) >> 16;
  WORD Lo = Rec.Time & 0xFFFF;
  ::FileTimeToDosDateTime(reinterpret_cast<LPFILETIME>(&LocalFileTime), &Hi, &Lo);
  Rec.Time = nb::ToInt32((nb::ToInt64(Hi) << 16) + Lo);
  Rec.Size = Rec.FindData.nFileSizeLow | (nb::ToInt64(Rec.FindData.nFileSizeHigh) << 32);
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

UnicodeString SysErrorMessage(DWORD ErrorCode)
{
  wchar_t Buffer[255]{};
  int32_t Len = ::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM |
    FORMAT_MESSAGE_ARGUMENT_ARRAY, nullptr, nb::ToInt32(ErrorCode), 0,
    static_cast<LPTSTR>(Buffer),
    _countof(Buffer), nullptr);
  while ((Len > 0) && ((Buffer[Len - 1] != 0) &&
    ((Buffer[Len - 1] <= 32) || (Buffer[Len - 1] == L'.'))))
  {
    Len--;
  }
  const UnicodeString Result = UnicodeString(Buffer, Len);
  return Result;
}

UnicodeString ReplaceStrAll(const UnicodeString & Str, const UnicodeString & What, const UnicodeString & ByWhat)
{
  UnicodeString Result = Str;
  if (What != ByWhat)
  {
    int32_t Pos = Result.Pos(What);
    while (Pos > 0)
    {
      Result.Replace(Pos, What.Length(), ByWhat.c_str(), ByWhat.Length());
      Pos = Result.Pos(What);
    }
  }
  return Result;
}

UnicodeString ExtractShortPathName(const UnicodeString & APath)
{
  // FIXME
  return APath;
}

//
// Returns everything, including the trailing Path separator, except the Filename
// part of the Path.
//
// "/foo/bar/baz.txt" --> "/foo/bar/"
UnicodeString ExtractDirectory(const UnicodeString & APath, wchar_t Delimiter)
{
  const UnicodeString Result = APath.SubString(1, APath.RPos(Delimiter));
  return Result;
}

constexpr const wchar_t * AllowDirectorySeparators = L"\\/";
constexpr const wchar_t * AllowDriveSeparators = L":";

bool CharInSet(const wchar_t Ch, const UnicodeString & S)
{
  const int32_t Index = S.Pos(Ch);
  return Index > 0;
}

UnicodeString ExtractFileDrive(const UnicodeString & FileName)
{
  UnicodeString Result;
  const int32_t L = FileName.Length();
  if (L < 2)
    return Result;
  if (CharInSet(FileName[2], AllowDriveSeparators))
  {
    Result = FileName.SubStr(0,2);
  }
  else if (CharInSet(FileName[1], AllowDirectorySeparators) &&
           CharInSet(FileName[2], AllowDirectorySeparators))
  {
    int32_t I = 2;
    // skip share
    while ((I < L) && !CharInSet(FileName[I + 1], AllowDirectorySeparators))
    {
      I++;
    }
    I++;
   while ((I < L) && !CharInSet(FileName[I + 1], AllowDirectorySeparators))
    {
      I++;
    }
    Result = FileName.SubStr(0, I);
  }
  return Result;
}

//
// Returns only the Filename part of the Path.
//
// "/foo/bar/baz.txt" --> "baz.txt"
UnicodeString ExtractFilename(const UnicodeString & APath, wchar_t Delimiter)
{
  return APath.SubString(APath.RPos(Delimiter) + 1);
}

//
// Returns the file's extension, if any. The period is considered part
// of the extension.
//
// "/foo/bar/baz.txt" --> ".txt"
// "/foo/bar/baz" --> ""
UnicodeString ExtractFileExtension(const UnicodeString & APath, wchar_t Delimiter)
{
  const UnicodeString FileName = Sysutils::ExtractFilename(APath, Delimiter);
  const int32_t N = FileName.RPos(L'.');
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
UnicodeString ChangeFileExtension(const UnicodeString & APath, const UnicodeString & Ext, wchar_t Delimiter)
{
  const UnicodeString FileName = Sysutils::ExtractFilename(APath, Delimiter);
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

UnicodeString ExcludeTrailingBackslash(const UnicodeString & Str)
{
  UnicodeString Result = Str;
  if ((Result.Length() > 0) && ((Result[Result.Length()] == Slash) ||
      (Result[Result.Length()] == Backslash)))
  {
    Result.SetLength(Result.Length() - 1);
  }
  return Result;
}

UnicodeString IncludeTrailingBackslash(const UnicodeString & Str)
{
  UnicodeString Result = Str;
  const int32_t L = Result.Length();
  if ((L == 0) || ((Result[L] != Slash) && (Result[L] != Backslash)))
  {
    Result += Backslash;
  }
  return Result;
}

UnicodeString IncludeTrailingPathDelimiter(const UnicodeString & Str)
{
  return ::IncludeTrailingBackslash(Str);
}

UnicodeString ExtractFileDir(const UnicodeString & Str)
{
  UnicodeString Result;
  const int32_t Pos = Str.LastDelimiter(L"/\\");
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

UnicodeString ExtractFilePath(const UnicodeString & Str)
{
  const UnicodeString Result = ::ExtractFileDir(Str);
  return Result;
}

UnicodeString GetCurrentDir()
{
  const UnicodeString Result = GetGlobals()->GetCurrentDirectory();
  return Result;
}

UnicodeString StrToHex(const UnicodeString & Str, bool UpperCase, wchar_t Separator)
{
  UnicodeString Result;
  for (int32_t Index = 1; Index <= Str.Length(); ++Index)
  {
    Result += CharToHex(Str[Index], UpperCase);
    if ((Separator != L'\0') && (Index <= Str.Length()))
    {
      Result += Separator;
    }
  }
  return Result;
}

UnicodeString HexToStr(const UnicodeString & Hex)
{
  const UnicodeString Digits = "0123456789ABCDEF";
  UnicodeString Result;
  const int32_t L = Hex.Length() - 1;
  if (L % 2 == 0)
  {
    for (int32_t Index = 1; Index <= Hex.Length(); Index += 2)
    {
      const int32_t P1 = Digits.FindFirstOf(::UpCase(Hex[Index]));
      const int32_t P2 = Digits.FindFirstOf(::UpCase(Hex[Index + 1]));
      if ((P1 == nb::NPOS) || (P2 == nb::NPOS))
      {
        Result.Clear();
        break;
      }
      Result += static_cast<wchar_t>((P1 - 1) * 16 + P2 - 1);
    }
  }
  return Result;
}

uint32_t HexToIntPtr(const UnicodeString & Hex, uint32_t MinChars)
{
  const UnicodeString Digits = "0123456789ABCDEF";
  uint32_t Result = 0;
  int32_t Index = 1;
  while (Index <= Hex.Length())
  {
    const int32_t A = Digits.FindFirstOf(UpCase(Hex[Index]));
    if (A == nb::NPOS)
    {
      if ((nb::ToIntPtr(MinChars) == nb::NPOS) || (Index <= nb::ToIntPtr(MinChars)))
      {
        Result = 0;
      }
      break;
    }

    Result = (Result * 16) + (nb::ToInt32(A) - 1);

    ++Index;
  }
  return Result;
}

UnicodeString IntToHex(uint32_t Int, uint32_t MinChars)
{
  UnicodeString Result = FORMAT("%X", Int);
  const int32_t Pad = nb::ToInt32(MinChars - Result.Length());
  if (Pad > 0)
  {
    for (int32_t Index = 0; Index < Pad; ++Index)
    {
      Result.Insert(L'0', 1);
    }
  }
  return Result;
}

char HexToChar(const UnicodeString & Hex, uint32_t MinChars)
{
  return static_cast<char>(HexToIntPtr(Hex, MinChars));
}

static void ConvertError(int32_t ErrorID)
{
  const UnicodeString Msg = FMTLOAD(ErrorID, 0);
  throw EConvertError(Msg);
}

static void DivMod(const uint32_t Dividend, uint32_t Divisor,
  uint32_t & Result, uint32_t & Remainder)
{
  Result = Dividend / Divisor;
  Remainder = Dividend % Divisor;
}

static bool DecodeDateFully(const TDateTime & DateTime,
  uint16_t & Year, uint16_t & Month, uint16_t & Day,
  uint16_t & DOW)
{
  constexpr int32_t D1 = 365;
  constexpr int32_t D4 = D1 * 4 + 1;
  constexpr int32_t D100 = D4 * 25 - 1;
  constexpr int32_t D400 = D100 * 4 + 1;
  int32_t T = DateTimeToTimeStamp(DateTime).Date;
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
  uint32_t Y = 1;
  while (T >= D400)
  {
    T -= D400;
    Y += 400;
  }
  uint32_t D = 0;
  uint32_t I = 0;
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
  const TDayTable * DayTable = &MonthDays[Result];
  uint32_t M = 1;
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

void DecodeDate(const TDateTime & DateTime, uint16_t & Year,
  uint16_t & Month, uint16_t & Day)
{
  uint16_t Dummy = 0;
  DecodeDateFully(DateTime, Year, Month, Day, Dummy);
}

void DecodeTime(const TDateTime & DateTime, uint16_t & Hour,
  uint16_t & Min, uint16_t & Sec, uint16_t & MSec)
{
  uint32_t MinCount, MSecCount;
  DivMod(DateTimeToTimeStamp(DateTime).Time, 60000, MinCount, MSecCount);
  uint32_t H, M, S, MS;
  DivMod(MinCount, 60, H, M);
  DivMod(MSecCount, 1000, S, MS);
  Hour = static_cast<uint16_t>(H);
  Min = static_cast<uint16_t>(M);
  Sec = static_cast<uint16_t>(S);
  MSec = static_cast<uint16_t>(MS);
}

static bool TryEncodeDate(uint16_t Year, uint16_t Month, uint16_t Day, TDateTime & Date)
{
  const TDayTable * DayTable = &MonthDays[IsLeapYear(nb::ToWord(Year))];
  if ((Year >= 1) && (Year <= 9999) && (Month >= 1) && (Month <= 12) &&
    (Day >= 1) && (Day <= (*DayTable)[Month - 1]))
  {
    for (int32_t Index = 1; Index <= Month - 1; Index++)
    {
      Day += (*DayTable)[Index - 1];
    }
    const int32_t Idx = Year - 1;
    Date = TDateTime(nb::ToDouble(Idx * 365 + Idx / 4 - Idx / 100 + Idx / 400 + Day - DateDelta));
    return true;
  }
  return false;
}

TDateTime EncodeDate(uint16_t Year, uint16_t Month, uint16_t Day)
{
  TDateTime Result;
  if (!TryEncodeDate(Year, Month, Day, Result))
  {
    ::ConvertError(SDateEncodeError);
  }
  return Result;
}

static bool TryEncodeTime(uint32_t Hour, uint32_t Min, uint32_t Sec, uint32_t MSec,
  TDateTime & Time)
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

TDateTime StrToDateTime(const UnicodeString & Value)
{
  (void)Value;
  ThrowNotImplemented(145);
  return TDateTime();
}

bool TryStrToDateTime(const UnicodeString & StrValue, TDateTime & Value,
  const TFormatSettings & FormatSettings)
{
  (void)StrValue;
  (void)Value;
  (void)FormatSettings;
  ThrowNotImplemented(147);
  return false;
}

UnicodeString DateTimeToString(const UnicodeString & Format,
  const TDateTime & DateTime)
{
  UnicodeString Result;
  // SYSTEMTIME st;
  // ::ZeroMemory(&st, sizeof(SYSTEMTIME));

  uint16_t Y; uint16_t M; uint16_t D;
  uint16_t H; uint16_t N; uint16_t S; uint16_t MS;

  DateTime.DecodeDate(Y, M, D);
  DateTime.DecodeTime(H, N, S, MS);

  std::tm tm{};
  tm.tm_sec = S;
  tm.tm_min = N;
  tm.tm_hour = H;
  tm.tm_mday = D;
  tm.tm_mon = M - 1;
  tm.tm_year = Y - 1900;
  tm.tm_isdst = -1;

  const std::time_t t = std::mktime(&tm);
  std::tm dt{};
  if (0 != localtime_s(&dt, &t))
    return Result;

  AnsiString Buffer(80, 0);
  if (0 != strftime(const_cast<char *>(Buffer.data()), sizeof(Buffer), AnsiString(Format).c_str(), &dt))
    Result = Buffer;

  return Result;
}

UnicodeString DateTimeToString(const TDateTime & DateTime)
{
  // return DateTime.FormatString(const_cast<wchar_t *>(L""));
  return DateTimeToString("%Y-%m-%dT%H:%M:%S", DateTime);
}

// DayOfWeek returns the day of the week of the given date. The Result is an
// integer between 1 and 7, corresponding to Sunday through Saturday.
// This function is not ISO 8601 compliant, for that see the DateUtils unit.
uint32_t DayOfWeek(const TDateTime & DateTime)
{
  return ::DateTimeToTimeStamp(DateTime).Date % 7 + 1;
}

TDateTime Date()
{
  SYSTEMTIME st;
  ::GetLocalTime(&st);
  TDateTime Result = ::EncodeDate(st.wYear, st.wMonth, st.wDay);
  return Result;
}

UnicodeString FormatDateTime(const UnicodeString & Fmt, const TDateTime & ADateTime)
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

  if (Fmt == L"dddddd tt")
  {
    /*
    return FormatDateTime(L"dddddd tt",
        EncodeDateVerbose(
            static_cast<uint16_t>(ValidityTime.Year), static_cast<uint16_t>(ValidityTime.Month),
            static_cast<uint16_t>(ValidityTime.Day)) +
        EncodeTimeVerbose(
            static_cast<uint16_t>(ValidityTime.Hour), static_cast<uint16_t>(ValidityTime.Min),
            static_cast<uint16_t>(ValidityTime.Sec), 0));
    */
    uint16_t Y, M, D, H, Mm, S, MS;
    const TDateTime DateTime =
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

static TDateTime ComposeDateTime(const TDateTime & Date, const TDateTime & Time)
{
  TDateTime Result = TDateTime(Date);
  Result += Time;
  return Result;
}

TDateTime SystemTimeToDateTime(const SYSTEMTIME & SystemTime)
{
  TDateTime Result = ComposeDateTime(EncodeDate(SystemTime.wYear, SystemTime.wMonth, SystemTime.wDay),
    EncodeTime(SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond, SystemTime.wMilliseconds));
  return Result;
}

UnicodeString UnixExcludeLeadingBackslash(const UnicodeString & APath)
{
  UnicodeString Result = APath;
  while (!Result.IsEmpty() && (Result[1] == Slash))
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
  const TDayTable * DayTable = &MonthDays[IsLeapYear(Year)];
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

TDateTime IncYear(const TDateTime & AValue, const Int64 ANumberOfYears)
{
  TDateTime Result = IncMonth(AValue, ANumberOfYears * MonthsPerYear);
  return Result;
}

TDateTime IncMonth(const TDateTime & AValue, const Int64 NumberOfMonths)
{
  Word Year, Month, Day;
  DecodeDate(AValue, Year, Month, Day);
  IncAMonth(Year, Month, Day, NumberOfMonths);
  TDateTime Result = EncodeDate(Year, Month, Day);
  ReplaceTime(Result, AValue);
  return Result;
}

TDateTime IncWeek(const TDateTime & AValue, const Int64 ANumberOfWeeks)
{
  TDateTime Result(AValue + ANumberOfWeeks * DaysPerWeek);
  return Result;
}

TDateTime IncDay(const TDateTime & AValue, const Int64 ANumberOfDays)
{
  TDateTime Result(AValue + ANumberOfDays);
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
    Result = ((AValue * MinutesPerDay) + ANumberOfMinutes) / MinutesPerDay;
  else
    Result = ((AValue * MinutesPerDay) - ANumberOfMinutes) / MinutesPerDay;
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

constexpr const double TDateTimeEpsilon = 2.2204460493e-16;
constexpr const double OneMillisecond = 0.001;
//constexpr double HalfMilliSecond = OneMillisecond / 2.0;
constexpr const double HalfMilliSecond = 0.0005;
constexpr const double ApproxDaysPerMonth = 30.4375;
constexpr const double ApproxDaysPerYear = 365.25;
//constexpr const int32_t HoursPerDay = 24; // Adjust the value accordingly
//constexpr const int32_t MinutesPerDay = 1440;

bool IsSameDay(const TDateTime & AValue, const TDateTime & ABasis)
{
  const double D = AValue - floor(ABasis);
  const bool Result = (D >= 0) && (D < 1);
  return Result;
}

double DateTimeToNumber(const TDateTime & ADateTime)
{
  if (ADateTime >= 0)
    return ADateTime;
  else
    return std::trunc(ADateTime) - std::fmod(ADateTime, 1.0);
}

TDateTime NumberToDateTime(double AValue)
{
  if (AValue >= 0)
    return TDateTime(AValue);
  else
    return TDateTime(std::trunc(AValue) + std::fmod(AValue, 1.0));
}

TDateTime DateTimeDiff(const TDateTime & ANow, const TDateTime & AThen)
{
  return TDateTime(NumberToDateTime(DateTimeToNumber(ANow) - DateTimeToNumber(AThen)));
}

int32_t YearsBetween(const TDateTime & ANow, const TDateTime & AThen)
{
  const double result = floor((abs(DateTimeDiff(ANow, AThen)) + HalfMilliSecond) / ApproxDaysPerYear);
  return static_cast<int32_t>(result);
}

int32_t MonthsBetween(const TDateTime & ANow, const TDateTime & AThen)
{
  return static_cast<int32_t>((std::abs(DateTimeDiff(ANow, AThen)) + HalfMilliSecond) / ApproxDaysPerMonth);
}

int32_t DaysBetween(const TDateTime & ANow, const TDateTime & AThen)
{
  if (ANow > AThen)
  {
    return nb::ToInt32(std::trunc(std::abs(DateTimeDiff(ANow, AThen)) + HalfMilliSecond));
  } else
  {
    return nb::ToInt32(std::trunc(std::abs(DateTimeDiff(AThen, ANow)) + HalfMilliSecond));
  }
}

int64_t HoursBetween(const TDateTime & ANow, const TDateTime & AThen)
{
  return static_cast<int64_t>(std::trunc((std::abs(DateTimeDiff(ANow, AThen)) + HalfMilliSecond) * HoursPerDay));
}

int64_t MinutesBetween(const TDateTime& ANow, const TDateTime & AThen)
{
  return static_cast<int64_t>(std::trunc((std::abs(DateTimeDiff(ANow, AThen)) + HalfMilliSecond) * MinutesPerDay));
}

int64_t MilliSecondsBetween(const TDateTime & ANow, const TDateTime & AThen)
{
  const double Result = floor(MilliSecondSpan(ANow, AThen));
  return nb::ToInt64(Result);
}

int64_t SecondsBetween(const TDateTime & ANow, const TDateTime & AThen)
{
  return MilliSecondsBetween(ANow, AThen);
}

UnicodeString StripHotkey(const UnicodeString & AText)
{
  UnicodeString Result = AText;
  int32_t Len = Result.Length();
  int32_t Pos = 1;
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

bool StartsText(const UnicodeString & ASubText, const UnicodeString & AText)
{
  return AText.Pos(ASubText) == 1;
}

uint32_t StrToVersionNumber(const UnicodeString & VersionNumberStr)
{
  uint32_t Result = 0;
  UnicodeString Version = VersionNumberStr;
  int32_t Shift = 16;
  while (!Version.IsEmpty())
  {
    const UnicodeString Num = CutToChar(Version, L'.', true);
    Result += nb::ToUInt32(Num.ToInt32()) << Shift;
    if (Shift >= 8)
      Shift -= 8;
  }
  return Result;
}

UnicodeString VersionNumberToStr(uint32_t VersionNumber)
{
  const DWORD Major = (VersionNumber >> 16) & 0xFF;
  const DWORD Minor = (VersionNumber >> 8) & 0xFF;
  const DWORD Revision = (VersionNumber & 0xFF);
  const UnicodeString Result = FORMAT("%d.%d.%d", Major, Minor, Revision);
  return Result;
}

bool CheckWin32Version(int32_t Major, int32_t Minor)
{
  return (GetGlobals()->Win32MajorVersion >= Major) && (GetGlobals()->Win32MinorVersion >= Minor);
}

TFormatSettings::TFormatSettings(LCID id) noexcept : id(id)
{
}

UnicodeString TPath::Combine(const UnicodeString & APath, const UnicodeString & AFileName)
{
  const UnicodeString Result = ::IncludeTrailingBackslash(APath) + AFileName;
  return Result;
}

UnicodeString TPath::Join(const UnicodeString & APath, const UnicodeString & AFileName)
{
  return base::UnixIncludeTrailingBackslash(APath) + AFileName;
}

uint16_t MilliSecondOf(const TDateTime & AValue)
{
  uint16_t Hour{0}, Min{0}, Sec{0}, MSec{0};
  DecodeTime(AValue, Hour, Min, Sec, MSec);
  return MSec;
}

uint16_t MilliSecondOfTheSecond(const TDateTime & AValue)
{
  return MilliSecondOf(AValue);
}

int32_t MilliSecondOfTheMinute(const TDateTime & AValue)
{
  uint16_t Hour{0}, Min{0}, Sec{0}, MSec{0};
  DecodeTime(AValue, Hour, Min, Sec, MSec);
  const int32_t Result = Sec*1000+MSec;
  return Result;
}

int32_t MilliSecondOfTheHour(const TDateTime & AValue)
{
  uint16_t Hour{0}, Min{0}, Sec{0}, MSec{0};
  DecodeTime(AValue, Hour, Min, Sec, MSec);
  const int32_t Result =(Min*60+Sec)*1000+MSec;
  return Result;
}

int32_t MilliSecondOfTheDay(const TDateTime & AValue)
{
  uint16_t Hour{0}, Min{0}, Sec{0}, MSec{0};
  DecodeTime(AValue, Hour, Min, Sec, MSec);
  const int32_t Result = (((Hour*60)+Min)*60+Sec)*1000+MSec;
  return Result;
}

DWORD YearOf(const TDateTime & AValue)
{
  uint16_t Year{0}, Month{0}, Day{0};
  DecodeDate(AValue, Year, Month, Day);
  return Year;
}

TDateTime StartOfTheYear(const TDateTime & AValue)
{
  TDateTime Result = EncodeDate(nb::ToUInt16(YearOf(AValue)), 1, 1);
  return Result;
}

DWORD DayOfTheYear(const TDateTime & AValue)
{
  const DWORD Result = nb::ToDWord(Trunc(AValue - StartOfTheYear(AValue)+1));
  return Result;
}

int64_t MilliSecondOfTheYear(const TDateTime & AValue)
{
  uint16_t Hour{0}, Min{0}, Sec{0}, MSec{0};
  DecodeTime(AValue, Hour, Min, Sec, MSec);
  const WORD Result = nb::ToWord(((Min+(Hour+((nb::ToInt64(DayOfTheYear(AValue))-1)*24))*60)*60+Sec)*1000+MSec);
  return Result;
}

int32_t Random(int32_t Max)
{
  return nb::ToInt32(nb::ToInt64(rand()) / (std::numeric_limits<int>::max() / Max));
}

static bool TryStringToGUID(const UnicodeString & S, GUID & Guid)
{
  bool E;
  const char * P;

  auto rb = [&P, &E]() -> unsigned char
  {
    unsigned char Result{0};
    if ((*P >= '0') && (*P <= '9'))
    {
      Result = *P - '0';
    }
    else if ((*P >= 'a') && (*P <= 'f'))
    {
      Result = *P - 'a' + 10;
    }
    else if ((*P >= 'A') && (*P <= 'F'))
    {
      Result = *P - 'A' + 10;
    }
    else
    {
      E = false;
    }
    ++P;
    return Result;
  };

  auto nextChar = [&P, &E](char C)
  {
    if (*P != C)
      E = false;
    ++P;
  };

  if (S.Length() != 38)
    return false;

  E = true;
  const AnsiString str(S.c_str());
  P = str.c_str();
  nextChar('{');
  Guid.Data1 = (rb() << 28) | (rb() << 24) | (rb() << 20) | (rb() << 16) | (rb() << 12) | (rb() << 8) | (rb() << 4) | rb();
  nextChar('-');
  Guid.Data2 = (rb() << 12) | (rb() << 8) | (rb() << 4) | rb();
  nextChar('-');
  Guid.Data3 = (rb() << 12) | (rb() << 8) | (rb() << 4) | rb();
  nextChar('-');
  Guid.Data4[0] = (rb() << 4) | rb();
  Guid.Data4[1] = (rb() << 4) | rb();
  nextChar('-');
  Guid.Data4[2] = (rb() << 4) | rb();
  Guid.Data4[3] = (rb() << 4) | rb();
  Guid.Data4[4] = (rb() << 4) | rb();
  Guid.Data4[5] = (rb() << 4) | rb();
  Guid.Data4[6] = (rb() << 4) | rb();
  Guid.Data4[7] = (rb() << 4) | rb();
  nextChar('}');
  return E;
}

bool FileGetSymLinkTarget(const UnicodeString & AFileName, UnicodeString & TargetName)
{

  struct TUnicodeSymLinkRec
  {
    // Define the members of TUnicodeSymLinkRec structure
    UnicodeString TargetName;
    DWORD Attr{0};
    int64_t Size{0};
  };

  enum TSymLinkResult
  {
    slrError,
    slrNoSymLink,
    slrOk
  };

  // Reparse point specific declarations from Windows headers
#ifndef IO_REPARSE_TAG_MOUNT_POINT
  constexpr const ULONG IO_REPARSE_TAG_MOUNT_POINT = 0xA0000003;
#endif
#ifndef IO_REPARSE_TAG_SYMLINK
  constexpr const ULONG IO_REPARSE_TAG_SYMLINK = 0xA000000C;
#endif
#ifndef ERROR_REPARSE_TAG_INVALID
  constexpr const DWORD ERROR_REPARSE_TAG_INVALID = 4393;
#endif
#ifndef FSCTL_GET_REPARSE_POINT
  constexpr DWORD FSCTL_GET_REPARSE_POINT = 0x900A8;
#endif
//    const DWORD MAXIMUM_REPARSE_DATA_BUFFER_SIZE = 16 * 1024;
#ifndef SYMLINK_FLAG_RELATIVE
  constexpr const ULONG SYMLINK_FLAG_RELATIVE = 1;
#endif
#ifndef FILE_FLAG_OPEN_REPARSE_POINT
  constexpr const DWORD FILE_FLAG_OPEN_REPARSE_POINT = 0x200000;
#endif
#ifndef FILE_READ_EA
  constexpr const DWORD FILE_READ_EA = 0x8;
#endif
  struct TReparseDataBuffer
  {
    ULONG ReparseTag{0};
    WORD ReparseDataLength{0};
    WORD Reserved{0};
    WORD SubstituteNameOffset{0};
    WORD SubstituteNameLength{0};
    WORD PrintNameOffset{0};
    WORD PrintNameLength{0};
    union
    {
      WCHAR PathBufferMount[4096]{};
      struct
      {
        ULONG Flags;
        WCHAR PathBufferSym[4096];
      };
    };
  };

  constexpr DWORD CShareAny = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
  constexpr DWORD COpenReparse = FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS;
  const UnicodeString CVolumePrefix = L"Volume";
  const UnicodeString CGlobalPrefix = L"\\\\?\\";

  DWORD BytesReturned{0};
  GUID guid{};

  TSymLinkResult Result = slrError;
  TUnicodeSymLinkRec SymLinkRec;

  const HANDLE HFile = CreateFileW(AFileName.c_str(), FILE_READ_EA, CShareAny, nullptr, OPEN_EXISTING, COpenReparse, nullptr);
  if (CheckHandle(HFile))
  {
    try
    {
      TReparseDataBuffer * PBuffer = static_cast<TReparseDataBuffer*>(nb_malloc(MAXIMUM_REPARSE_DATA_BUFFER_SIZE));
      if (PBuffer != nullptr)
      {
        if (DeviceIoControl(HFile, FSCTL_GET_REPARSE_POINT, nullptr, 0,
          PBuffer, MAXIMUM_REPARSE_DATA_BUFFER_SIZE, &BytesReturned, nullptr))
        {
          switch (PBuffer->ReparseTag)
          {
            case IO_REPARSE_TAG_MOUNT_POINT:
            {
              SymLinkRec.TargetName = UnicodeString(
                &PBuffer->PathBufferMount[4 + PBuffer->SubstituteNameOffset / sizeof(WCHAR)],
                (PBuffer->SubstituteNameLength / sizeof(WCHAR)) - 4);
              if (SymLinkRec.TargetName.Length() == (CVolumePrefix.Length() + 2 + 32 + 4 + 1)
                  && SymLinkRec.TargetName.SubStr(0, CVolumePrefix.Length()) == CVolumePrefix
                  && TryStringToGUID(UnicodeString(
                                      SymLinkRec.TargetName.SubStr(CVolumePrefix.Length() + 1)),
                                      guid))
              {
                SymLinkRec.TargetName = CGlobalPrefix + SymLinkRec.TargetName;
              }
              break;
            }
            case IO_REPARSE_TAG_SYMLINK:
            {
              SymLinkRec.TargetName = UnicodeString(
                &PBuffer->PathBufferSym[PBuffer->PrintNameOffset / sizeof(WCHAR)],
                  PBuffer->PrintNameLength / sizeof(WCHAR));
              if ((PBuffer->Flags & SYMLINK_FLAG_RELATIVE) != 0)
              {
                SymLinkRec.TargetName = ExpandFileName(ExtractFilePath(AFileName) + SymLinkRec.TargetName);
              }
              break;
            }
          }

          if (!SymLinkRec.TargetName.IsEmpty())
          {
            WIN32_FILE_ATTRIBUTE_DATA fileAttributeData{};
            if (GetFileAttributesExW(SymLinkRec.TargetName.c_str(), GetFileExInfoStandard, &fileAttributeData))
            {
              SymLinkRec.Attr = fileAttributeData.dwFileAttributes;
              SymLinkRec.Size = static_cast<uint64_t>(fileAttributeData.nFileSizeHigh) << 32 | fileAttributeData.nFileSizeLow;
            }
            /*else if (RaiseErrorOnMissing)
            {
                throw std::runtime_error("Directory not found: " + std::to_string(GetLastError()));
            }*/
            else
            {
              SymLinkRec.TargetName.Clear();
            }
          }
          else
          {
            ::SetLastError(ERROR_REPARSE_TAG_INVALID);
            Result = slrNoSymLink;
          }
        }
        else
        {
          ::SetLastError(ERROR_REPARSE_TAG_INVALID);
        }

        nb_free(PBuffer);
      }
    }
    catch (...)
    {
      ::CloseHandle(HFile);
      throw;
    }

    ::CloseHandle(HFile);
  }

  if (!SymLinkRec.TargetName.IsEmpty())
  {
    Result = slrOk;
  }

  return Result == slrOk;
}

} // namespace Sysutils

namespace base {

FILE * LocalOpenFileForWriting(const UnicodeString & LogFileName, bool Append)
{
  const UnicodeString NewFileName = StripPathQuotes(::ExpandEnvironmentVariables(LogFileName));
  FILE * Result = _wfsopen(ApiPath(NewFileName).c_str(), Append ? L"ab" : L"wb", SH_DENYWR);
  if (Result != nullptr)
  {
    constexpr size_t BUFSIZE = 4 * 1024;
    setvbuf(Result, nullptr, _IONBF, BUFSIZE);
  }
  return Result;
}

bool WriteAndFlush(FILE * File, void const * Data, size_t Size)
{
  assert(File);
  assert(Data);
  size_t nWrite = 0;
  while ((nWrite = ::fwrite(Data, 1, Size - nWrite, File)) != 0)
  {
    if ((static_cast<intptr_t>(nWrite) < 0) && (errno != EINTR))
    {
      // error
      break;
    }
    else if (nWrite == Size)
    {
      // All write
      break;
    }
    else if (nWrite > 0)
    {
      // Half write
    }
  }

  // error
  if (static_cast<intptr_t>(nWrite) < 0)
    return false; // TODO: throw exception

  ::fflush(File);
  return true;
}

bool FileExists(const UnicodeString & AFileName)
{
  return ::FileExists(AFileName);
}

bool FileRemove(const UnicodeString & AFileName)
{
  return ::RemoveFile(AFileName);
}

bool DoExists(bool R, const UnicodeString & Path)
{
  bool Result = R;
  if (!Result)
  {
    const int32_t Error = GetLastError();
    if ((Error == ERROR_CANT_ACCESS_FILE) || // returned when resolving symlinks in %LOCALAPPDATA%\Microsoft\WindowsApps
        (Error == ERROR_ACCESS_DENIED)) // returned for %USERPROFILE%\Application Data symlink
    {
      Result = base::DirectoryExists(ExtractFileDir(Path));
    }
  }
  return Result;
}

bool FileExistsFix(const UnicodeString & Path)
{
  // WORKAROUND
  ::SetLastError(ERROR_SUCCESS);
  const bool Result = DoExists(base::FileExists(Path), Path);
  return Result;
}

bool RenameFile(const UnicodeString & From, const UnicodeString & To)
{
  return ::RenameFile(From, To);
}

bool DirectoryExists(const UnicodeString & ADir)
{
  return ::DirectoryExists(ApiPath(ADir));
}

DWORD FindFirst(const UnicodeString & AFileName, DWORD LocalFileAttrs, TSearchRec & Rec)
{
  constexpr DWORD faSpecial = faHidden | faSysFile | faDirectory;
  Rec.ExcludeAttr = (~LocalFileAttrs) & faSpecial;
  Rec.FindHandle = ::FindFirstFileW(ApiPath(AFileName).c_str(), &Rec.FindData);
  DWORD Result;
  if (CheckHandle(Rec.FindHandle))
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

DWORD FindNext(TSearchRec & Rec)
{
  DWORD Result;
  if (::FindNextFileW(Rec.FindHandle, &Rec.FindData))
    Result = FindMatchingFile(Rec);
  else
    Result = ::GetLastError();
  return Result;
}

DWORD FindClose(TSearchRec & Rec)
{
  DWORD Result = 0;
  if (CheckHandle(Rec.FindHandle))
  {
    Result = FALSE != ::FindClose(Rec.FindHandle);
    Rec.FindHandle = INVALID_HANDLE_VALUE;
  }
  return Result;
}

} // namespace base
