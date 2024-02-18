#include <vcl.h>
#pragma hdrstop

#include <Classes.hpp>
#include <Common.h>
#include <Exceptions.h>
#include <Sysutils.hpp>
#include <rtlconsts.h>
#include <FileBuffer.h>

static TGlobals * GlobalFunctions = nullptr;

TGlobals * GetGlobals()
{
  DebugAssert(GlobalFunctions != nullptr);
  return GlobalFunctions;
}

void SetGlobals(TGlobals * Value)
{
  DebugAssert((GlobalFunctions == nullptr) || (Value == nullptr) || (Value == GlobalFunctions));
  GlobalFunctions = Value;
}

#if (_MSC_VER >= 1900)

extern "C" {
  FILE * __iob_func = nullptr;
} // extern "C"
#endif

TPersistent::TPersistent(TObjectClassId Kind) :
  TObject(Kind)
{
}

void TPersistent::Assign(const TPersistent * Source)
{
  if (Source != nullptr)
  {
    Source->AssignTo(this);
  }
  else
  {
    AssignError(nullptr);
  }
}

void TPersistent::AssignTo(TPersistent * Dest) const
{
  Dest->AssignError(this);
}

TPersistent * TPersistent::GetOwner()
{
  return nullptr;
}

[[noreturn]] void TPersistent::AssignError(const TPersistent * Source)
{
  (void)Source;
  throw Exception("Cannot assign");
}

TObjectList::~TObjectList() noexcept
{
  TList::Clear();
}

const TObject * TObjectList::operator [](int32_t Index) const
{
  return cast_to<const TObject>(TList::operator [](Index));
}

const TObject * TObjectList::GetObj(int32_t Index) const
{
  if (Index < GetCount())
  {
    return TList::GetAs<TObject>(Index);
  }
  return nullptr;
}

void TObjectList::Notify(TObject * Ptr, TListNotification Action)
{
  if (GetOwnsObjects())
  {
    if (Action == lnDeleted)
    {
      delete cast_to<TObject>(Ptr);
    }
  }
  TList::Notify(Ptr, Action);
}

constexpr const int32_t MemoryDelta = 0x2000;

void TStrings::SetTextStr(const UnicodeString & AText)
{
  BeginUpdate();
  SCOPE_EXIT
  {
    EndUpdate();
  };
  Clear();
  const wchar_t * P = AText.c_str();
  // if (P != nullptr)
  {
    while (*P != 0x00)
    {
      const wchar_t * Start = P;
      while (!((*P == 0x00) || (*P == 0x0A) || (*P == 0x0D)))
      {
        P++;
      }
      UnicodeString S;
      S.SetLength(nb::ToInt32(P - Start));
      memmove(ToWCharPtr(S), Start, (P - Start) * sizeof(wchar_t));
      Add(S);
      if (*P == 0x0D)
      {
        P++;
      }
      if (*P == 0x0A)
      {
        P++;
      }
    }
  }
}

UnicodeString TStrings::GetCommaText() const
{
  const wchar_t LOldDelimiter = GetDelimiter();
  const wchar_t LOldQuoteChar = GetQuoteChar();
  FDelimiter = L',';
  FQuoteChar = L'"';
  SCOPE_EXIT
  {
    FDelimiter = LOldDelimiter;
    FQuoteChar = LOldQuoteChar;
  };
  UnicodeString Result = GetDelimitedText();
  return Result;
}

UnicodeString TStrings::GetDelimitedText() const
{
  UnicodeString Result;
  const int32_t Count = GetCount();
  if ((Count == 1) && GetString(0).IsEmpty())
  {
    Result = GetQuoteChar() + GetQuoteChar();
  }
  else
  {
    for (int32_t Index = 0; Index < GetCount(); ++Index)
    {
      UnicodeString Line = GetString(Index);
      Result += GetQuoteChar() + Line + GetQuoteChar() + GetDelimiter();
    }
    if (Result.Length() > 0)
      Result.SetLength(Result.Length() - 1);
  }
  return Result;
}

static void tokenize(const UnicodeString & str, nb::vector_t<UnicodeString> & tokens,
  const UnicodeString & delimiters = L" ", const bool trimEmpty = false)
{
  int32_t lastPos = 0;
  while (true)
  {
    int32_t pos = str.FindFirstOf(delimiters.c_str(), lastPos);
    if (pos == nb::NPOS)
    {
      pos = str.Length();

      if (pos != lastPos || !trimEmpty)
      {
        tokens.emplace_back(
          UnicodeString(str.data() + lastPos, pos - lastPos));
      }
      break;
    }
    if (pos != lastPos || !trimEmpty)
    {
      tokens.emplace_back(
        UnicodeString(str.data() + lastPos, pos - lastPos));
    }

    lastPos = pos + 1;
  }
}

void TStrings::SetDelimitedText(const UnicodeString & Value)
{
  BeginUpdate();
  SCOPE_EXIT
  {
    EndUpdate();
  };
  Clear();
  nb::vector_t<UnicodeString> Lines;
  const UnicodeString Delimiter(UnicodeString(1, GetDelimiter()) + L'\n');
  tokenize(Value, Lines, Delimiter, true);
  for (const auto& Line: Lines)

  {
    Add(Line);
  }
}

int32_t TStrings::CompareStrings(const UnicodeString & S1, const UnicodeString & S2) const
{
  return ::AnsiCompareText(S1, S2);
}

void TStrings::Assign(const TPersistent * Source)
{
  const TStrings * pStrings = rtti::dyn_cast_or_null<TStrings>(Source);
  if (pStrings != nullptr)
  {
    BeginUpdate();
    SCOPE_EXIT
    {
      EndUpdate();
    };
    Clear();
    DebugAssert(pStrings);
    FQuoteChar = pStrings->FQuoteChar;
    FDelimiter = pStrings->FDelimiter;
    AddStrings(pStrings);
  }
  else
  {
    TPersistent::Assign(Source);
  }
}

int32_t TStrings::Add(const UnicodeString & S, const TObject * AObject)
{
  const int32_t Result = GetCount();
  Insert(Result, S, AObject);
  return Result;
}

UnicodeString TStrings::GetText() const
{
  return GetTextStr();
}

UnicodeString TStrings::GetTextStr() const
{
  UnicodeString Result;
  const int32_t Count = GetCount();
  int32_t Size = 0;
  const UnicodeString LB(L"\r\n");
  for (int32_t Index = 0; Index < Count; ++Index)
  {
    Size += GetString(Index).Length() + LB.Length();
  }
  wchar_t * Buffer = Result.SetLength(Size);
  wchar_t * P = Buffer;
  for (int32_t Index = 0; Index < Count; ++Index)
  {
    const UnicodeString S = GetString(Index);
    int32_t L = S.Length() * sizeof(wchar_t);
    if (L != 0)
    {
      memmove(P, S.c_str(), L);
      P += S.Length();
    }
    L = LB.Length() * sizeof(wchar_t);
    if (L != 0)
    {
      memmove(P, LB.c_str(), L);
      P += LB.Length();
    }
  }
  return Result;
}

void TStrings::SetText(const UnicodeString & AText)
{
  SetTextStr(AText);
}

void TStrings::SetCommaText(const UnicodeString & Value)
{
  SetDelimiter(L',');
  SetQuoteChar(L'"');
  SetDelimitedText(Value);
}

void TStrings::BeginUpdate()
{
  if (FUpdateCount == 0)
  {
    SetUpdateState(true);
  }
  FUpdateCount++;
}

void TStrings::EndUpdate()
{
  FUpdateCount--;
  if (FUpdateCount == 0)
  {
    SetUpdateState(false);
  }
}

void TStrings::SetUpdateState(bool Updating)
{
  (void)Updating;
}

int32_t TStrings::AddObject(const UnicodeString & S, const TObject * AObject)
{
  const int32_t Result = Add(S, AObject);
  return Result;
}

void TStrings::InsertObject(int32_t Index, const UnicodeString & Key, TObject * AObject)
{
  Insert(Index, Key, AObject);
}

bool TStrings::Equals(const TStrings * Value) const
{
  if (GetCount() != Value->GetCount())
  {
    return false;
  }
  for (int32_t Index = 0; Index < GetCount(); ++Index)
  {
    if (GetString(Index) != Value->GetString(Index))
    {
      return false;
    }
  }
  return true;
}

void TStrings::SetString(int32_t Index, const UnicodeString & S)
{
  TObject * TempObject = Get(Index);
  Delete(Index);
  InsertObject(Index, S, TempObject);
}

void TStrings::SetDuplicates(TDuplicatesEnum Value)
{
  FDuplicates = Value;
}

void TStrings::Move(int32_t CurIndex, int32_t NewIndex)
{
  if (CurIndex != NewIndex)
  {
    BeginUpdate();
    SCOPE_EXIT
    {
      EndUpdate();
    };
    const UnicodeString TempString = GetString(CurIndex);
    TObject * TempObject = Get(CurIndex);
    Delete(CurIndex);
    InsertObject(NewIndex, TempString, TempObject);
  }
}

int32_t TStrings::IndexOf(const UnicodeString & S) const
{
  for (int32_t Result = 0; Result < GetCount(); Result++)
  {
    if (CompareStrings(GetString(Result), S) == 0)
    {
      return Result;
    }
  }
  return nb::NPOS;
}

int32_t TStrings::IndexOfName(const UnicodeString & Name) const
{
  for (int32_t Index = 0; Index < GetCount(); ++Index)
  {
    UnicodeString S = GetString(Index);
    const int32_t P = ::AnsiPos(S, L'=');
    if ((P > 0) && (CompareStrings(S.SubStr(1, P - 1), Name) == 0))
    {
      return Index;
    }
  }
  return nb::NPOS;
}

UnicodeString TStrings::GetName(int32_t Index) const
{
  return ExtractName(GetString(Index));
}

void TStrings::SetName(int32_t /*Index*/, const UnicodeString & /*Value*/)
{
  ThrowNotImplemented(2012);
}

UnicodeString TStrings::ExtractName(const UnicodeString & S)
{
  UnicodeString Result = S;
  const int32_t P = ::AnsiPos(Result, L'=');
  if (P > 0)
  {
    Result.SetLength(P - 1);
  }
  else
  {
    Result.SetLength(0);
  }
  return Result;
}

UnicodeString TStrings::GetValue(const UnicodeString & AName) const
{
  UnicodeString Result;
  const int32_t Index = IndexOfName(AName);
  if (Index >= 0)
  {
    Result = GetString(Index).SubStr(AName.Length() + 2);
  }
  return Result;
}

void TStrings::SetValue(const UnicodeString & AName, const UnicodeString & AValue)
{
  int32_t Index = IndexOfName(AName);
  if (!AValue.IsEmpty())
  {
    if (Index < 0)
    {
      Index = Add("");
    }
    SetString(Index, AName + L'=' + AValue);
  }
  else
  {
    if (Index >= 0)
    {
      Delete(Index);
    }
  }
}

UnicodeString TStrings::GetValueFromIndex(int32_t Index) const
{
  const UnicodeString Name = GetName(Index);
  UnicodeString Result = GetValue(Name);
  return Result;
}

void TStrings::AddStrings(const TStrings * AStrings)
{
  BeginUpdate();
  SCOPE_EXIT
  {
    EndUpdate();
  };
  for (int32_t Index = 0; Index < AStrings->GetCount(); ++Index)
  {
    AddObject(AStrings->GetString(Index), AStrings->GetObj(Index));
  }
}

void TStrings::Append(const UnicodeString & Value)
{
  Insert(GetCount(), Value);
}

void TStrings::SaveToStream(TStream * /*Stream*/) const
{
  ThrowNotImplemented(12);
}

int32_t StringListCompareStrings(TStringList * List, int32_t Index1, int32_t Index2)
{
  const int32_t Result = List->CompareStrings(List->FStrings[Index1],
      List->FStrings[Index2]);
  return Result;
}

TStringList::TStringList(TObjectClassId Kind) noexcept :
  TStrings(Kind)
{
  SetOwnsObjects(false);
}

void TStringList::Assign(const TPersistent * Source)
{
  TStrings::Assign(Source);
}

int32_t TStringList::GetCount() const
{
  DebugAssert(nb::ToInt32(FStrings.size()) == TObjectList::GetCount());
  return nb::ToInt32(FStrings.size());
}

int32_t TStringList::Add(const UnicodeString & S)
{
  return AddObject(S, nullptr);
}

int32_t TStringList::AddObject(const UnicodeString & S, const TObject * AObject)
{
  int32_t Result = 0;
  if (!GetSorted())
  {
    Result = GetCount();
  }
  else
  {
    if (Find(S, Result, false))
    {
      switch (FDuplicates)
      {
      case dupIgnore:
        return Result;
        break;
      case dupError:
        Error(SDuplicateString, 2);
        break;
      case dupAccept:
        ++Result;
        break;
      }
    }
  }
  InsertItem(Result, S, AObject);
  return Result;
}

bool TStringList::Find(const UnicodeString & S, int32_t & Index, bool leftmost) const
{
  bool Result = false;
  if (GetSorted())
  {
#define ASSIGNMENT(SELECTOR) if (SELECTOR) { L = Idx + 1; } else { H = Idx; }
    int32_t L = 0;
    int32_t H = GetCount();
    bool Found = false;
    while (L < H)
    {
      const int32_t Idx = (L + H) >> 1;
      const int32_t C = CompareStrings(FStrings[Idx], S);
      if (leftmost ? (C < 0) : (C > 0))
      {
        ASSIGNMENT(leftmost)
      }
      else
      {
        if (C == 0)
        {
          Found = true;
        }
        ASSIGNMENT(!leftmost)
      }
    }
#undef ASSIGNMENT
      Index = leftmost ? L : (Found ? H - 1 : H);
      return Found;
  }
  else
  {
    Index = nb::NPOS;
    for (int32_t Idx = 0; Idx < GetCount(); Idx++)
    {
      const int32_t C = CompareStrings(FStrings[Idx], S);
      if (C == 0)
      {
        Result = true;
        Index = Idx;
        break;
      }
    }
  }
  return Result;
}

int32_t TStringList::IndexOf(const UnicodeString & S) const
{
  int32_t Result = nb::NPOS;
  if (!GetSorted())
  {
    Result = TStrings::IndexOf(S);
  }
  else
  {
    if (!Find(S, Result))
    {
      Result = nb::NPOS;
    }
  }
  return Result;
}

void TStringList::SetString(int32_t Index, const UnicodeString & S)
{
  if (GetSorted())
  {
    Error(SSortedListError, 0);
  }
  if ((Index == nb::NPOS) || (Index > nb::ToInt32(FStrings.size())))
  {
    Error(SListIndexError, Index);
  }
  Changing();
  if (Index < nb::ToInt32(FStrings.size()))
  {
    FStrings[Index] = S;
  }
  else
  {
    Insert(Index, S);
  }
  Changed();
}

void TStringList::Delete(int32_t Index)
{
  if ((Index == nb::NPOS) || (Index >= nb::ToInt32(FStrings.size())))
  {
    Error(SListIndexError, Index);
  }
  Changing();
  FStrings.erase(FStrings.begin() + Index);
  TObjectList::Delete(Index);
  Changed();
}

void TStringList::InsertObject(int32_t Index, const UnicodeString & Key, TObject * AObject)
{
  if (GetSorted())
  {
    Error(SSortedListError, 0);
  }
  if ((Index == nb::NPOS) || (Index > GetCount()))
  {
    Error(SListIndexError, Index);
  }
  InsertItem(Index, Key, AObject);
}

void TStringList::InsertItem(int32_t Index, const UnicodeString & S, const TObject * AObject)
{
  if ((Index == nb::NPOS) || (Index > GetCount()))
  {
    Error(SListIndexError, Index);
  }
  Changing();
  if (Index == GetCount())
  {
    FStrings.push_back(S);
    TObjectList::Add(const_cast<TObject*>(AObject));
  }
  else
  {
    FStrings.insert(FStrings.begin() + Index, 1, S);
    TObjectList::Insert(Index, const_cast<TObject*>(AObject));
  }
  Changed();
}

const UnicodeString & TStringList::GetStringRef(int32_t Index) const
{
  if ((Index == nb::NPOS) || (Index > nb::ToInt32(FStrings.size())))
  {
    Error(SListIndexError, Index);
  }
  if (Index == nb::ToInt32(FStrings.size()))
  {
    const_cast<TStringList *>(this)->InsertItem(Index, UnicodeString(), nullptr);
  }
  return FStrings[Index];
}

const UnicodeString & TStringList::GetString(int32_t Index) const
{
  return GetStringRef(Index);
}

UnicodeString TStringList::GetString(int32_t Index)
{
  return GetStringRef(Index);
}

void TStringList::SetCaseSensitive(bool Value)
{
  if (Value != FCaseSensitive)
  {
    FCaseSensitive = Value;
    if (GetSorted())
    {
      Sort();
    }
  }
}

void TStringList::SetSorted(bool Value)
{
  if (Value != FSorted)
  {
    if (Value)
    {
      Sort();
    }
    FSorted = Value;
  }
}

void TStringList::LoadFromFile(const UnicodeString & AFileName)
{
  HANDLE FileHandle = ::CreateFile(AFileName.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
  if (CheckHandle(FileHandle))
  {
    TSafeHandleStream Stream(FileHandle);
    const int64_t Size = Stream.GetSize();
    TFileBuffer FileBuffer;
    FileBuffer.LoadStream(&Stream, Size, True);
    bool ConvertToken = false;
    FileBuffer.Convert(eolCRLF, eolCRLF, cpRemoveCtrlZ | cpRemoveBOM, ConvertToken);
    SAFE_CLOSE_HANDLE(FileHandle);
    const UnicodeString Str(FileBuffer.Data(), nb::ToInt32(FileBuffer.Size()));
    SetTextStr(Str);
  }
}

void TStringList::SetObj(int32_t Index, TObject * AObject)
{
  if ((Index == nb::NPOS) || (Index >= TObjectList::GetCount()))
  {
    Error(SListIndexError, Index);
  }
  Changing();
  TObjectList::SetItem(Index, AObject);
  Changed();
}

void TStringList::SetUpdateState(bool Updating)
{
  if (Updating)
  {
    Changing();
  }
  else
  {
    Changed();
  }
}

void TStringList::Changing()
{
  if (GetUpdateCount() == 0 && FOnChanging)
  {
    FOnChanging(this);
  }
}

void TStringList::Changed()
{
  if (GetUpdateCount() == 0 && FOnChange)
  {
    FOnChange(this);
  }
}

void TStringList::Insert(int32_t Index, const UnicodeString & S, const TObject * AObject)
{
  InsertItem(Index, S, AObject);
}

void TStringList::Sort()
{
  CustomSort(StringListCompareStrings);
}

void TStringList::CustomSort(TStringListSortCompare ACompareFunc)
{
  if (!GetSorted() && (GetCount() > 1))
  {
    Changing();
    QuickSort(0, GetCount() - 1, ACompareFunc);
    Changed();
  }
}

void TStringList::QuickSort(int32_t L, int32_t R, TStringListSortCompare SCompare)
{
  int32_t Index;
  do
  {
    Index = L;
    int32_t J = R;
    int32_t P = (L + R) >> 1;
    do
    {
      while (SCompare(this, Index, P) < 0)
      {
        Index++;
      }
      while (SCompare(this, J, P) > 0)
      {
        J--;
      }
      if (Index <= J)
      {
        ExchangeItems(Index, J);
        if (P == Index)
        {
          P = J;
        }
        else if (P == J)
        {
          P = Index;
        }
        Index++;
        J--;
      }
    }
    while (Index <= J);
    if (L < J)
    {
      QuickSort(L, J, SCompare);
    }
    L = Index;
  }
  while (Index < R);
}

void TStringList::ExchangeItems(int32_t Index1, int32_t Index2)
{
  const bool Owns = GetOwnsObjects();
  SetOwnsObjects(false);
  {
    SCOPE_EXIT
    {
      SetOwnsObjects(Owns);
    };
    const UnicodeString SItem1 = FStrings[Index1];
    TObject * OItem1 = TObjectList::Get(Index1);
    FStrings[Index1] = FStrings[Index2];
    TObjectList::SetItem(Index1, TObjectList::Get(Index2));
    FStrings[Index2] = SItem1;
    TObjectList::SetItem(Index2, OItem1);
  }
}

int32_t TStringList::CompareStrings(const UnicodeString & S1, const UnicodeString & S2) const
{
  if (GetCaseSensitive())
  {
    return AnsiCompareStr(S1, S2);
  }
  return AnsiCompareText(S1, S2);
}

TDateTime::TDateTime(uint16_t Hour,
  uint16_t Min, uint16_t Sec, uint16_t MSec)
{
  FValue = ::EncodeTimeVerbose(Hour, Min, Sec, MSec);
}

bool TDateTime::operator ==(const TDateTime & rhs) const
{
  return ::IsZero(FValue - rhs.FValue);
}

UnicodeString TDateTime::GetDateString() const
{
  uint16_t Y, M, D;
  DecodeDate(Y, M, D);
  UnicodeString Result = FORMAT("%02d.%02d.%04d", D, M, Y);
  return Result;
}

UnicodeString TDateTime::GetTimeString(bool Short) const
{
  uint16_t H, N, S, MS;
  DecodeTime(H, N, S, MS);
  UnicodeString Result;
  if (Short)
    Result = FORMAT("%02d.%02d.%02d", H, N, S);
  else
    Result = FORMAT("%02d.%02d.%02d.%03d", H, N, S, MS);
  return Result;
}

UnicodeString TDateTime::FormatString(const wchar_t * fmt) const
{
  (void)fmt;
  uint16_t H, N, S, MS;
  DecodeTime(H, N, S, MS);
  UnicodeString Result = FORMAT("%02d.%02d.%02d.%03d", H, N, S, MS);
  return Result;
}

void TDateTime::DecodeDate(uint16_t & Y, uint16_t & M, uint16_t & D) const
{
  ::DecodeDate(*this, Y, M, D);
}

void TDateTime::DecodeTime(uint16_t & H, uint16_t & N, uint16_t & S, uint16_t & MS) const
{
  ::DecodeTime(*this, H, N, S, MS);
}

TDateTime Now()
{
  SYSTEMTIME SystemTime;
  ::GetLocalTime(&SystemTime);
  TDateTime Result = ::EncodeDate(SystemTime.wYear, SystemTime.wMonth, SystemTime.wDay) +
    ::EncodeTime(SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond, SystemTime.wMilliseconds);
  return Result;
}

TDateTime SpanOfNowAndThen(const TDateTime & ANow, const TDateTime & AThen)
{
  TDateTime Result;
  if (ANow < AThen)
    Result = AThen - ANow;
  else
    Result = ANow - AThen;
  return Result;
}

double MilliSecondSpan(const TDateTime & ANow, const TDateTime & AThen)
{
  const double Result = MSecsPerDay * SpanOfNowAndThen(ANow, AThen);
  return Result;
}

/*
TSHFileInfo::TSHFileInfo() :
  FGetFileInfo(nullptr)
{
  FSHFileInfoLoader.Load(L"Shell32.dll");
  if (FSHFileInfoLoader.Loaded())
  {
    FGetFileInfo = reinterpret_cast<TGetFileInfo>(FSHFileInfoLoader.GetProcAddress("SHGetFileInfo"));
  }
}

TSHFileInfo::~TSHFileInfo()
{
  FSHFileInfoLoader.Unload();
}

int32_t TSHFileInfo::GetFileIconIndex(const UnicodeString & StrFileName, BOOL bSmallIcon) const
{
  SHFILEINFO sfi = {};

  if (bSmallIcon)
  {
    if (FGetFileInfo)
      FGetFileInfo(
        static_cast<LPCTSTR>(StrFileName.c_str()),
        FILE_ATTRIBUTE_NORMAL,
        &sfi,
        sizeof(SHFILEINFO),
        SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
  }
  else
  {
    if (FGetFileInfo)
      FGetFileInfo(
        static_cast<LPCTSTR>(StrFileName.c_str()),
        FILE_ATTRIBUTE_NORMAL,
        &sfi,
        sizeof(SHFILEINFO),
        SHGFI_SYSICONINDEX | SHGFI_LARGEICON | SHGFI_USEFILEATTRIBUTES);
  }
  return sfi.iIcon;
}

int32_t TSHFileInfo::GetDirIconIndex(BOOL bSmallIcon)
{
  SHFILEINFO sfi = {};
  if (bSmallIcon)
  {
    if (FGetFileInfo)
      FGetFileInfo(
        static_cast<LPCTSTR>(L"Doesn't matter"),
        FILE_ATTRIBUTE_DIRECTORY,
        &sfi,
        sizeof(SHFILEINFO),
        SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
  }
  else
  {
    if (FGetFileInfo)
      FGetFileInfo(
        static_cast<LPCTSTR>(L"Doesn't matter"),
        FILE_ATTRIBUTE_DIRECTORY,
        &sfi,
        sizeof(SHFILEINFO),
        SHGFI_SYSICONINDEX | SHGFI_LARGEICON | SHGFI_USEFILEATTRIBUTES);
  }
  return sfi.iIcon;
}

UnicodeString TSHFileInfo::GetFileType(const UnicodeString & StrFileName)
{
  SHFILEINFO sfi;

  if (FGetFileInfo)
    FGetFileInfo(
      reinterpret_cast<LPCTSTR>(StrFileName.c_str()),
      FILE_ATTRIBUTE_NORMAL,
      &sfi,
      sizeof(SHFILEINFO),
      SHGFI_TYPENAME | SHGFI_USEFILEATTRIBUTES);

  return sfi.szTypeName;
}
*/

void TStream::ReadBuffer(void * Buffer, int64_t Count)
{
  if ((Count != 0) && (Read(Buffer, Count) != Count))
  {
    throw Exception(FMTLOAD(SReadError));
  }
}

void TStream::WriteBuffer(const void * Buffer, int64_t Count)
{
  if ((Count != 0) && (Write(Buffer, Count) != Count))
  {
    throw Exception(FMTLOAD(SWriteError));
  }
}

int64_t TStream::CopyFrom(TStream * Source, int64_t Count)
{
  DebugAssert(Source);
  constexpr int64_t MaxSize = 0x20000;
  int64_t Result = 0;
  if (Count == 0)
  {
    Source->SetPosition(0); // This WILL fail for non-seekable streams...
  }
  int64_t BufferSize = MaxSize;
  if ((Count > 0) && (Count < BufferSize))
  {
    BufferSize = Count; // do not allocate more than needed
  }
  void * Buffer = nb::calloc<void *>(1, nb::ToSizeT(BufferSize));
  try__finally
  {
    int64_t I = 0;
    if (Count == 0)
    {
      do
      {
        I = Source->Read(Buffer, BufferSize);
        if (I > 0)
        {
          WriteBuffer(Buffer, I);
        }
        Result += I;
      } while (I < BufferSize);
    }
    else
    {
      while (Count > 0)
      {
        I = (Count > BufferSize) ? BufferSize : Count;
        Source->ReadBuffer(Buffer, I);
        WriteBuffer(Buffer, I);
        Count -= I;
        Result += I;
      }
    }
  }
  __finally
  {
    nb_free(Buffer);
  } end_try__finally
  return Result;
}

int64_t TStream::GetPosition() const
{
  return Seek(0, TSeekOrigin::soCurrent);
}

int64_t TStream::GetSize() const
{
  const int64_t Pos = Seek(0, TSeekOrigin::soCurrent);
  const int64_t Result = Seek(0, TSeekOrigin::soEnd);
  const int64_t Res = Seek(Pos, TSeekOrigin::soBeginning);
  DebugAssert(Res == Pos);
  return Result;
}

void TStream::SetPosition(int64_t Pos)
{
  const int64_t Res = Seek(Pos, TSeekOrigin::soBeginning);
  DebugAssert(Res == Pos);
}

[[noreturn]] static void ReadError(const UnicodeString & Name)
{
  throw Exception(FORMAT("InvalidRegType: %s", Name)); // FIXME ERegistryException.CreateResFmt(@SInvalidRegType, [Name]);
}

THandleStream::THandleStream(HANDLE AHandle) noexcept :
  FHandle(AHandle)
{
}

int64_t THandleStream::Read(void * Buffer, int64_t Count)
{
  int64_t Result = ::FileRead(FHandle, Buffer, Count);
  if (Result == -1)
  {
    Result = 0;
  }
  return Result;
}

int64_t THandleStream::Write(const void * Buffer, int64_t Count)
{
  int64_t Result = ::FileWrite(FHandle, Buffer, Count);
  if (Result == -1)
  {
    Result = 0;
  }
  return Result;
}

int64_t THandleStream::Seek(const int64_t Offset, TSeekOrigin SeekOrigin) const
{
  DWORD Origin = FILE_BEGIN;
  switch (SeekOrigin)
  {
  case TSeekOrigin::soBeginning:
    Origin = FILE_BEGIN;
    break;
  case TSeekOrigin::soCurrent:
    Origin = FILE_CURRENT;
    break;
  case TSeekOrigin::soEnd:
    Origin = FILE_END;
    break;
  }
  const int64_t Result = ::FileSeek(FHandle, Offset, Origin);
  return Result;
}

void THandleStream::SetSize(int64_t NewSize)
{
  const auto res = Seek(NewSize, TSeekOrigin::soBeginning);
  DebugAssert(res == NewSize);
  // LARGE_INTEGER li;
  // li.QuadPart = size;
  // if (SetFilePointer(fh.get(), li.LowPart, &li.HighPart, FILE_BEGIN) == -1)
  // handleLastErrorImpl(_path);
  ::Win32Check(::SetEndOfFile(FHandle) > 0);
}

TFileStream::TFileStream(const UnicodeString & AFileName, uint16_t Mode) :
  FFileName(AFileName)
{
  if (Mode == fmCreate)
  {
    FHandle = ::CreateFile(ApiPath(AFileName).c_str(), GENERIC_ALL, FILE_SHARE_WRITE, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  }
  else
  {
    FHandle = ::CreateFile(ApiPath(AFileName).c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  }
  if (FHandle == INVALID_HANDLE_VALUE)
  {
    if (Mode == fmCreate)
    {
      throw EFCreateError(FMTLOAD(SFCreateError, AFileName));
    }
    else
    {
      throw EFOpenError(FMTLOAD(SFOpenError, AFileName));
    }
  }
}

/*TFileStream::TFileStream(const UnicodeString & AFileName, uint16_t Mode, uint32_t Rights)
{

}*/

TFileStream::~TFileStream() noexcept
{
  SAFE_CLOSE_HANDLE(FHandle);
}

TFileStream * TFileStream::Create(const UnicodeString & AFileName, uint16_t Mode)
{
  return new TFileStream(AFileName, Mode);
}

TSafeHandleStream::TSafeHandleStream(THandle AHandle) noexcept :
  THandleStream(AHandle)
{
}

TSafeHandleStream::TSafeHandleStream(gsl::not_null<THandleStream *> Source, bool Own) :
  TSafeHandleStream(Source->Handle)
{
  FSource = Own ? Source.get() : nullptr;
}

TSafeHandleStream * TSafeHandleStream::CreateFromFile(const UnicodeString & FileName, uint16_t Mode)
{
  return new TSafeHandleStream(new TFileStream(ApiPath(FileName), Mode), true);
}

TSafeHandleStream::~TSafeHandleStream() noexcept
{
  SAFE_DESTROY(FSource);
}

int64_t TSafeHandleStream::Read(void * Buffer, int64_t Count)
{
  // This is invoked for example via CopyFrom from TParallelOperation::Done
  const int64_t Result = ::FileRead(FHandle, Buffer, Count);
  if (Result == nb::ToInt64(-1))
  {
    ::RaiseLastOSError();
  }
  return Result;
}

int64_t TSafeHandleStream::Write(const void * Buffer, int64_t Count)
{
  // This is invoked for example by TIniFileStorage::Flush or via CopyFrom from TParallelOperation::Done
  const int64_t Result = ::FileWrite(FHandle, Buffer, Count);
  if (Result == -1)
  {
    ::RaiseLastOSError();
  }
  return Result;
}

TMemoryStream::TMemoryStream() noexcept
{
}

TMemoryStream::~TMemoryStream() noexcept
{
  Clear();
}

int64_t TMemoryStream::Read(void * Buffer, int64_t Count)
{
  int64_t Result;
  if ((FPosition >= 0) && (Count >= 0))
  {
    Result = FSize - FPosition;
    if (Result > 0)
    {
      if (Result > Count)
      {
        Result = Count;
      }
      memmove(Buffer, static_cast<char *>(FMemory) + FPosition, nb::ToSizeT(Result));
      FPosition += Result;
      return Result;
    }
  }
  Result = 0;
  return Result;
}

int64_t TMemoryStream::Seek(const int64_t Offset, TSeekOrigin Origin) const
{
  switch (Origin)
  {
  case TSeekOrigin::soBeginning:
    FPosition = Offset;
    break;
  case TSeekOrigin::soCurrent:
    FPosition += Offset;
    break;
  case TSeekOrigin::soEnd:
    FPosition = FSize + Offset;
    break;
  }
  const int64_t Result = FPosition;
  return Result;
}

void TMemoryStream::SaveToStream(TStream * Stream)
{
  if (FSize != 0)
  {
    Stream->WriteBuffer(FMemory, FSize);
  }
}

void TMemoryStream::SaveToFile(const UnicodeString & /*AFileName*/)
{
  // TFileStream Stream(FileName, fmCreate);
  // SaveToStream(Stream);
  ThrowNotImplemented(1203);
}

void TMemoryStream::Clear()
{
  SetCapacity(0);
  FSize = 0;
  FPosition = 0;
}

void TMemoryStream::SetSize(int64_t NewSize)
{
  const int64_t OldPosition = FPosition;
  SetCapacity(NewSize);
  FSize = NewSize;
  if (OldPosition > NewSize)
  {
    const auto res = Seek(0, TSeekOrigin::soEnd);
    DebugAssert(res == NewSize);
  }
}

void TMemoryStream::SetCapacity(int64_t NewCapacity)
{
  SetPointer(Realloc(NewCapacity), FSize);
  FCapacity = NewCapacity;
}

void * TMemoryStream::Realloc(int64_t & NewCapacity)
{
  if ((NewCapacity > 0) && (NewCapacity != FSize))
  {
    NewCapacity = (NewCapacity + (MemoryDelta - 1)) & ~(MemoryDelta - 1);
  }
  void * Result = FMemory;
  if (NewCapacity != FCapacity)
  {
    if (NewCapacity == 0)
    {
      nb_free(FMemory);
      FMemory = nullptr;
      Result = nullptr;
    }
    else
    {
      if (FCapacity == 0)
      {
        Result = nb::calloc<void *>(1, nb::ToSizeT(NewCapacity));
      }
      else
      {
        Result = nb::realloc<void *>(FMemory, nb::ToSizeT(NewCapacity));
      }
      if (Result == nullptr)
      {
        throw EStreamError(FMTLOAD(SMemoryStreamError));
      }
    }
  }
  return Result;
}

void TMemoryStream::SetPointer(void * Ptr, int64_t ASize)
{
  FMemory = Ptr;
  FSize = ASize;
}

int64_t TMemoryStream::Write(const void * Buffer, int64_t Count)
{
  int64_t Result = 0;
  if ((FPosition >= 0) && (Count >= 0))
  {
    const int64_t Pos = FPosition + Count;
    if (Pos > 0)
    {
      if (Pos > FSize)
      {
        if (Pos > FCapacity)
        {
          SetCapacity(Pos);
        }
        FSize = Pos;
      }
      memmove(static_cast<char *>(FMemory) + FPosition,
        Buffer, nb::ToSizeT(Count));
      FPosition = Pos;
      Result = Count;
    }
  }
  return Result;
}

bool IsRelative(const UnicodeString & Value)
{
  return !(!Value.IsEmpty() && (Value[1] == Backslash));
}

TRegDataType DataTypeToRegData(DWORD Value)
{
  TRegDataType Result;
  if (Value == REG_SZ)
  {
    Result = rdString;
  }
  else if (Value == REG_EXPAND_SZ)
  {
    Result = rdExpandString;
  }
  else if (Value == REG_DWORD)
  {
    Result = rdInteger;
  }
  else if (Value == REG_QWORD)
  {
    Result = rdIntPtr;
  }
  else if (Value == REG_BINARY)
  {
    Result = rdBinary;
  }
  else
  {
    Result = rdUnknown;
  }
  return Result;
}

DWORD RegDataToDataType(TRegDataType Value)
{
  DWORD Result;
  switch (Value)
  {
  case rdString:
    Result = REG_SZ;
    break;
  case rdExpandString:
    Result = REG_EXPAND_SZ;
    break;
  case rdInteger:
    Result = REG_DWORD;
    break;
  case rdIntPtr:
    Result = REG_QWORD;
    break;
  case rdBinary:
    Result = REG_BINARY;
    break;
  default:
    Result = REG_NONE;
    break;
  }
  return Result;
}

class ERegistryException final // : public std::exception
{
};

TRegistry::TRegistry() noexcept
{
  SetRootKey(HKEY_CURRENT_USER);
  SetAccess(KEY_ALL_ACCESS);
}

TRegistry::~TRegistry() noexcept
{
  CloseKey();
}

void TRegistry::SetAccess(uint32_t Value)
{
  FAccess = Value;
}

void TRegistry::SetRootKey(HKEY ARootKey)
{
  if (FRootKey != ARootKey)
  {
    if (FCloseRootKey)
    {
      ::RegCloseKey(GetRootKey());
      FCloseRootKey = false;
    }
    FRootKey = ARootKey;
    CloseKey();
  }
}

void TRegistry::GetValueNames(TStrings * Names) const
{
  Names->Clear();
  TRegKeyInfo Info{};
  if (GetKeyInfo(Info))
  {
    UnicodeString S(nb::ToInt32(Info.MaxValueLen) + 1, 0);
    for (DWORD Index = 0; Index < Info.NumValues; Index++)
    {
      DWORD Len = Info.MaxValueLen + 1;
      RegEnumValue(GetCurrentKey(), Index, &S[1], &Len, nullptr, nullptr, nullptr, nullptr);
      Names->Add(S);
    }
  }
}

void TRegistry::GetKeyNames(TStrings * Names) const
{
  Names->Clear();
  TRegKeyInfo Info{};
  if (GetKeyInfo(Info))
  {
    UnicodeString S(nb::ToInt32(Info.MaxSubKeyLen) + 1, 0);
    for (DWORD Index = 0; Index < Info.NumSubKeys; Index++)
    {
      DWORD Len = Info.MaxSubKeyLen + 1;
      RegEnumKeyEx(GetCurrentKey(), nb::ToDWord(Index), &S[1], &Len, nullptr, nullptr, nullptr, nullptr);
      Names->Add(S);
    }
  }
}

HKEY TRegistry::GetCurrentKey() const { return FCurrentKey; }
HKEY TRegistry::GetRootKey() const { return FRootKey; }

void TRegistry::CloseKey()
{
  if (GetCurrentKey() != nullptr)
  {
    ::RegCloseKey(GetCurrentKey());
    FCurrentKey = nullptr;
    FCurrentPath.Clear();
  }
}

bool TRegistry::OpenKey(const UnicodeString & AKey, bool CanCreate)
{
  bool Result;
  UnicodeString S = AKey;
  const bool Relative = IsRelative(S);

  HKEY TempKey = nullptr;
  if (!CanCreate || S.IsEmpty())
  {
    Result = ::RegOpenKeyEx(GetBaseKey(Relative), S.c_str(), 0,
      FAccess, &TempKey) == ERROR_SUCCESS;
  }
  else
  {
    Result = ::RegCreateKeyEx(GetBaseKey(Relative), S.c_str(), 0, nullptr,
      REG_OPTION_NON_VOLATILE, FAccess, nullptr, &TempKey, nullptr) == ERROR_SUCCESS;
  }
  if (Result)
  {
    if ((GetCurrentKey() != nullptr) && Relative)
    {
      S = FCurrentPath + Backslash + S;
    }
    ChangeKey(TempKey, S);
  }
  return Result;
}

bool TRegistry::DeleteKey(const UnicodeString & AKey)
{
  const bool Relative = IsRelative(AKey);
  const HKEY OldKey = GetCurrentKey();
  const HKEY DeleteKey = GetKey(AKey);
  if (DeleteKey != nullptr)
  {
    SCOPE_EXIT
    {
      SetCurrentKey(OldKey);
      ::RegCloseKey(DeleteKey);
    };
    SetCurrentKey(DeleteKey);
    TRegKeyInfo Info{};
    if (GetKeyInfo(Info))
    {
      UnicodeString KeyName;
      KeyName.SetLength(Info.MaxSubKeyLen + 1);
      for (int32_t Index = nb::ToInt32(Info.NumSubKeys) - 1; Index >= 0; Index--)
      {
        DWORD Len = Info.MaxSubKeyLen + 1;
        if (RegEnumKeyEx(DeleteKey, nb::ToDWord(Index), &KeyName[1], &Len,
            nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS)
        {
          this->DeleteKey(KeyName);
        }
      }
    }
  }
  const bool Result = RegDeleteKey(GetBaseKey(Relative), AKey.c_str()) == ERROR_SUCCESS;
  return Result;
}

bool TRegistry::DeleteValue(const UnicodeString & Value) const
{
  const bool Result = RegDeleteValue(GetCurrentKey(), Value.c_str()) == ERROR_SUCCESS;
  return Result;
}

bool TRegistry::KeyExists(const UnicodeString & SubKey) const
{
  const uint32_t OldAccess = FAccess;
  SCOPE_EXIT
  {
    FAccess = OldAccess;
  };
  FAccess = STANDARD_RIGHTS_READ | KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS;
  const HKEY TempKey = GetKey(SubKey);
  if (TempKey != nullptr)
  {
    ::RegCloseKey(TempKey);
  }
  const bool Result = (TempKey != nullptr);
  return Result;
}

bool TRegistry::ValueExists(const UnicodeString & Value) const
{
  TRegDataInfo Info{};
  const bool Result = GetDataInfo(Value, Info);
  return Result;
}

bool TRegistry::GetDataInfo(const UnicodeString & ValueName, TRegDataInfo & Value) const
{
  DWORD DataType;
  nb::ClearStruct(Value);
  const bool Result = (::RegQueryValueEx(GetCurrentKey(), ValueName.c_str(), nullptr, &DataType, nullptr,
    &Value.DataSize) == ERROR_SUCCESS);
  Value.RegData = DataTypeToRegData(DataType);
  return Result;
}

TRegDataType TRegistry::GetDataType(const UnicodeString & ValueName) const
{
  TRegDataType Result{};
  TRegDataInfo Info{};
  if (GetDataInfo(ValueName, Info))
  {
    Result = Info.RegData;
  }
  else
  {
    Result = rdUnknown;
  }
  return Result;
}

int32_t TRegistry::GetDataSize(const UnicodeString & ValueName) const
{
  int32_t Result{0};
  TRegDataInfo Info{};
  if (GetDataInfo(ValueName, Info))
  {
    Result = nb::ToInt32(Info.DataSize);
  }
  else
  {
    Result = nb::ToInt32(-1);
  }
  return Result;
}

bool TRegistry::ReadBool(const UnicodeString & Name) const
{
  const bool Result = ReadInteger(Name) != 0;
  return Result;
}

TDateTime TRegistry::ReadDateTime(const UnicodeString & Name) const
{
  TDateTime Result = TDateTime(ReadFloat(Name));
  return Result;
}

double TRegistry::ReadFloat(const UnicodeString & Name) const
{
  double Result = 0.0;
  TRegDataType RegData = rdUnknown;
  const int32_t Len = GetData(Name, &Result, sizeof(double), RegData);
  if ((RegData != rdBinary) || (Len != sizeof(double)))
  {
    ReadError(Name);
  }
  return Result;
}

intptr_t TRegistry::ReadIntPtr(const UnicodeString & Name) const
{
  int32_t Result = 0;
  TRegDataType RegData = rdUnknown;
  const auto res = GetData(Name, &Result, sizeof(Result), RegData);
  DebugAssert(res == sizeof(Result));
  if (RegData != rdIntPtr)
  {
    ReadError(Name);
  }
  return Result;
}

int32_t TRegistry::ReadInteger(const UnicodeString & Name) const
{
  DWORD Result{0};
  TRegDataType RegData = rdUnknown;
  const int32_t res = GetData(Name, &Result, sizeof(Result), RegData);
  DebugAssert(res == sizeof(Result));
  if (RegData != rdInteger)
  {
    ReadError(Name);
  }
  return nb::ToInt32(Result);
}

int64_t TRegistry::ReadInt64(const UnicodeString & Name) const
{
  int64_t Result = 0;
  const auto res = ReadBinaryData(Name, &Result, sizeof(Result));
  DebugAssert(res == sizeof(Result));
  return Result;
}

UnicodeString TRegistry::ReadString(const UnicodeString & Name) const
{
  UnicodeString Result;
  const int32_t Len = GetDataSize(Name);
  if (Len > 0)
  {
    TRegDataType RegData = rdUnknown;
    wchar_t * Buffer = Result.SetLength(Len);
    GetData(Name, nb::ToPtr(Buffer), Len, RegData);
    if ((RegData == rdString) || (RegData == rdExpandString))
    {
      PackStr(Result);
    }
    else
    {
      ReadError(Name);
    }
  }
  else
  {
    Result.Clear();
  }
  return Result;
}

UnicodeString TRegistry::ReadStringRaw(const UnicodeString & Name) const
{
  UnicodeString Result = ReadString(Name);
  return Result;
}

int32_t TRegistry::ReadBinaryData(const UnicodeString & Name,
  void * Buffer, int32_t BufSize) const
{
  int32_t Result{};
  TRegDataInfo Info{};
  if (GetDataInfo(Name, Info))
  {
    Result = nb::ToInt32(Info.DataSize);
    TRegDataType RegData = Info.RegData;
    if (((RegData == rdBinary) || (RegData == rdUnknown)) && (Result <= BufSize))
    {
      const int32_t res = GetData(Name, Buffer, Result, RegData);
      DebugAssert(res == Result);
    }
    else
    {
      ReadError(Name);
    }
  }
  else
  {
    Result = 0;
  }
  return Result;
}

int32_t TRegistry::GetData(const UnicodeString & Name, void * Buffer,
  int32_t ABufSize, TRegDataType & RegData) const
{
  DWORD DataType = REG_NONE;
  DWORD BufSize = nb::ToDWord(ABufSize);
  if (::RegQueryValueEx(GetCurrentKey(), Name.c_str(), nullptr, &DataType,
    static_cast<BYTE *>(Buffer), &BufSize) != ERROR_SUCCESS)
  {
    throw Exception("RegQueryValueEx failed"); // FIXME ERegistryException.CreateResFmt(@SRegGetDataFailed, [Name]);
  }
  RegData = DataTypeToRegData(DataType);
  const int32_t Result = nb::ToInt32(BufSize);
  return Result;
}

void TRegistry::PutData(const UnicodeString & Name, const void * Buffer,
  int32_t ABufSize, TRegDataType RegData)
{
  const DWORD DataType = RegDataToDataType(RegData);
  if (::RegSetValueEx(GetCurrentKey(), Name.c_str(), 0, DataType,
      static_cast<const BYTE *>(Buffer), nb::ToDWord(ABufSize)) != ERROR_SUCCESS)
  {
    throw Exception("RegSetValueEx failed"); // ERegistryException(); // FIXME .CreateResFmt(SRegSetDataFailed, Name.c_str());
  }
}

void TRegistry::WriteBool(const UnicodeString & Name, bool Value)
{
  WriteInteger(Name, Value);
}

void TRegistry::WriteDateTime(const UnicodeString & Name, const TDateTime & Value)
{
  const double Val = Value.GetValue();
  PutData(Name, &Val, sizeof(double), rdBinary);
}

void TRegistry::WriteFloat(const UnicodeString & Name, double Value)
{
  PutData(Name, &Value, sizeof(double), rdBinary);
}

void TRegistry::WriteString(const UnicodeString & Name, const UnicodeString & Value)
{
  WriteStringRaw(Name, Value);
}

void TRegistry::WriteStringRaw(const UnicodeString & Name, const UnicodeString & Value)
{
  PutData(Name, Value.c_str(), Value.Length() * sizeof(wchar_t), rdString);
}

void TRegistry::WriteIntPtr(const UnicodeString & Name, intptr_t Value)
{
  PutData(Name, &Value, sizeof(Value), rdIntPtr);
}

void TRegistry::WriteInteger(const UnicodeString & Name, int32_t Value)
{
  const DWORD Val = nb::ToDWord(Value);
  PutData(Name, &Val, sizeof(Val), rdInteger);
}

void TRegistry::WriteInt64(const UnicodeString & Name, int64_t Value)
{
  WriteBinaryData(Name, nb::ToUInt8Ptr(&Value), sizeof(Value));
}

void TRegistry::WriteBinaryData(const UnicodeString & Name,
  const uint8_t * Buffer, int32_t  BufSize)
{
  PutData(Name, Buffer, BufSize, rdBinary);
}

void TRegistry::ChangeKey(HKEY Value, const UnicodeString & APath)
{
  CloseKey();
  FCurrentKey = Value;
  FCurrentPath = APath;
}

HKEY TRegistry::GetBaseKey(bool Relative) const
{
  HKEY Result;
  if ((FCurrentKey == nullptr) || !Relative)
  {
    Result = GetRootKey();
  }
  else
  {
    Result = FCurrentKey;
  }
  return Result;
}

HKEY TRegistry::GetKey(const UnicodeString & AKey) const
{
  const bool Relative = IsRelative(AKey);
  HKEY Result = nullptr;
  if (::RegOpenKeyEx(GetBaseKey(Relative), AKey.c_str(), 0, FAccess, &Result) == ERROR_SUCCESS)
    return Result;
  return nullptr;
}

bool TRegistry::GetKeyInfo(TRegKeyInfo & Value) const
{
  nb::ClearStruct(Value);
  const bool Result = ::RegQueryInfoKey(GetCurrentKey(), nullptr, nullptr, nullptr, &Value.NumSubKeys,
    &Value.MaxSubKeyLen, nullptr, &Value.NumValues, &Value.MaxValueLen,
    &Value.MaxDataLen, nullptr, &Value.FileTime) == ERROR_SUCCESS;
  return Result;
}

TShortCut::TShortCut(int32_t Value) noexcept :
  FValue(Value)
{
}

TShortCut::operator int32_t() const
{
  return FValue;
}

bool TShortCut::operator <(const TShortCut & rhs) const
{
  return FValue < rhs.FValue;
}

void GetLocaleFormatSettings(int32_t LCID, const TFormatSettings & FormatSettings)
{
  (void)LCID;
  (void)FormatSettings;
  ThrowNotImplemented(1204);
}


TGlobals::TGlobals() noexcept
{
  InitPlatformId();
}

void TGlobals::SetupDbgHandles(const UnicodeString & DbgFileName)
{
  if (!DbgFileName.IsEmpty() && DbgFileName != "-")
  {
    /*if (freopen(err.c_str(), "a", stderr))
    {
      setvbuf(stderr, NULL, _IONBF, 0);
    } else
      perror("freopen stderr");*/
    dbgstream_.open(DbgFileName.c_str(), std::ios_base::out|std::ios_base::trunc);
    if (!dbgstream_.bad())
      icecream::ic.output(dbgstream_);
  }
}

void TGlobals::InitPlatformId()
{
  OSVERSIONINFO OSVersionInfo{};
  OSVersionInfo.dwOSVersionInfoSize = sizeof(OSVersionInfo);
  if (::GetVersionEx(&OSVersionInfo) != FALSE)
  {
    Win32Platform = OSVersionInfo.dwPlatformId;
    Win32MajorVersion = OSVersionInfo.dwMajorVersion;
    Win32MinorVersion = OSVersionInfo.dwMinorVersion;
    Win32BuildNumber = OSVersionInfo.dwBuildNumber;
    nbstr_memcpy(Win32CSDVersion, OSVersionInfo.szCSDVersion, sizeof(OSVersionInfo.szCSDVersion));
  }
}

TTimeSpan::TTimeSpan(int64_t ATicks) : FTicks(ATicks)
{
}

TTimeSpan TTimeSpan::FromSeconds(double Value)
{
  TTimeSpan Result(nb::ToInt64(round(Value * TicksPerSecond)));
  return Result;
}

