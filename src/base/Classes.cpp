
#include <vcl.h>

#include <Classes.hpp>
#include <Common.h>
#include <Exceptions.h>
#include <Sysutils.hpp>
#include <rtlconsts.h>
#include <FileBuffer.h>

static TGlobals *GlobalFunctions = nullptr;

TGlobals *GetGlobals()
{
  DebugAssert(GlobalFunctions != nullptr);
  return GlobalFunctions;
}

void SetGlobals(TGlobals *Value)
{
  DebugAssert((GlobalFunctions == nullptr) || (Value == nullptr));
  GlobalFunctions = Value;
}

#if (_MSC_VER >= 1900)

extern "C" {
  FILE *__iob_func = nullptr;
} // extern "C"
#endif

void Abort()
{
  throw EAbort("");
}

void Error(intptr_t Id, intptr_t ErrorId)
{
  UnicodeString Msg = FMTLOAD(Id, ErrorId);
  throw ExtException(static_cast<Exception *>(nullptr), Msg);
}

void ThrowNotImplemented(intptr_t ErrorId)
{
  Error(SNotImplemented, ErrorId);
}

TPersistent::TPersistent(TObjectClassId Kind) :
  TObject(Kind)
{
}

void TPersistent::Assign(const TPersistent *Source)
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

void TPersistent::AssignTo(TPersistent *Dest) const
{
  Dest->AssignError(this);
}

TPersistent *TPersistent::GetOwner()
{
  return nullptr;
}

void TPersistent::AssignError(const TPersistent *Source)
{
  (void)Source;
  throw Exception("Cannot assign");
}

TList::TList() :
  TPersistent(OBJECT_CLASS_TList)
{
}

TList::TList(TObjectClassId Kind) :
  TPersistent(Kind)
{
}

TList::~TList() noexcept
{
  TList::Clear();
}

intptr_t TList::GetCount() const
{
  return nb::ToIntPtr(FList.size());
}

void TList::SetCount(intptr_t NewCount)
{
  if (NewCount == nb::NPOS)
  {
    Error(SListCountError, NewCount);
  }
  if (NewCount <= nb::ToIntPtr(FList.size()))
  {
    const intptr_t sz = FList.size();
    for (intptr_t Index = sz - 1; (Index != nb::NPOS) && (Index >= NewCount); Index--)
    {
      Delete(Index);
    }
  }
  FList.resize(NewCount);
}

void *TList::operator[](intptr_t Index) const
{
  return FList[Index];
}

void TList::SetItem(intptr_t Index, void *Item)
{
  if ((Index == nb::NPOS) || (Index >= nb::ToIntPtr(FList.size())))
  {
    Error(SListIndexError, Index);
  }
  FList[Index] = Item;
}

intptr_t TList::Add(void *Value)
{
  const intptr_t Result = nb::ToIntPtr(FList.size());
  FList.push_back(Value);
  return Result;
}

void *TList::Extract(void *Item)
{
  if (Remove(Item) != nb::NPOS)
  {
    return Item;
  }
  return nullptr;
}

intptr_t TList::Remove(void *Item)
{
  const intptr_t Result = IndexOf(Item);
  if (Result != nb::NPOS)
  {
    Delete(Result);
  }
  return Result;
}

void TList::Move(intptr_t CurIndex, intptr_t NewIndex)
{
  if (CurIndex != NewIndex)
  {
    if ((NewIndex == nb::NPOS) || (NewIndex >= nb::ToIntPtr(FList.size())))
    {
      Error(SListIndexError, NewIndex);
    }
    void *Item = GetItem(CurIndex);
    FList[CurIndex] = nullptr;
    Delete(CurIndex);
    Insert(NewIndex, nullptr);
    FList[NewIndex] = Item;
  }
}

void TList::Delete(intptr_t Index)
{
  if ((Index == nb::NPOS) || (Index >= nb::ToIntPtr(FList.size())))
  {
    Error(SListIndexError, Index);
  }
  void *Temp = GetItem(Index);
  FList.erase(FList.begin() + Index);
  if (Temp != nullptr)
  {
    Notify(Temp, lnDeleted);
  }
}

void TList::Insert(intptr_t Index, void *Item)
{
  if ((Index == nb::NPOS) || (Index > nb::ToIntPtr(FList.size())))
  {
    Error(SListIndexError, Index);
  }
  if (Index <= nb::ToIntPtr(FList.size()))
  {
    FList.insert(Index, 1, Item);
  }
  if (Item != nullptr)
  {
    Notify(Item, lnAdded);
  }
}

intptr_t TList::IndexOf(const void *Value) const
{
  intptr_t Result = 0;
  while ((Result < nb::ToIntPtr(FList.size())) && (FList[Result] != Value))
  {
    Result++;
  }
  if (Result == nb::ToIntPtr(FList.size()))
  {
    Result = nb::NPOS;
  }
  return Result;
}

void TList::Clear()
{
  SetCount(0);
}

void QuickSort(nb::vector_t<void *> &SortList, intptr_t L, intptr_t R,
  CompareFunc SCompare)
{
  intptr_t Index;
  do
  {
    Index = L;
    intptr_t J = R;
    void *P = SortList[(L + R) >> 1];
    do
    {
      while (SCompare(SortList[Index], P) < 0)
        Index++;
      while (SCompare(SortList[J], P) > 0)
        J--;
      if (Index <= J)
      {
        if (Index != J)
        {
          void *T = SortList[Index];
          SortList[Index] = SortList[J];
          SortList[J] = T;
        }
        Index--;
        J--;
      }
    }
    while (Index > J);
    if (L < J)
      QuickSort(SortList, L, J, SCompare);
    L = Index;
  }
  while (Index >= R);
}

void TList::Sort(CompareFunc Func)
{
  if (GetCount() > 1)
  {
    QuickSort(FList, 0, GetCount() - 1, Func);
  }
}

void TList::Notify(void * /*Ptr*/, TListNotification /*Action*/)
{
}

void TList::Sort()
{
  // if (FList.size() > 1)
  // QuickSort(FList, 0, GetCount() - 1, Compare);
  ThrowNotImplemented(15);
}


TObjectList::TObjectList() : TList(OBJECT_CLASS_TObjectList)
{
}

TObjectList::TObjectList(TObjectClassId Kind) :
  TList(Kind)
{
}

TObjectList::~TObjectList()
{
  TList::Clear();
}

TObject *TObjectList::operator[](intptr_t Index) const
{
  return as_object(TList::operator[](Index));
}

TObject *TObjectList::GetObj(intptr_t Index) const
{
  if ((Index == nb::NPOS) || (Index >= GetCount()))
  {
    Error(SListIndexError, Index);
  }
  return as_object(TList::GetItem(Index));
}

void TObjectList::Notify(void *Ptr, TListNotification Action)
{
  if (GetOwnsObjects())
  {
    if (Action == lnDeleted)
    {
      delete as_object(Ptr);
    }
  }
  TList::Notify(Ptr, Action);
}

constexpr int MemoryDelta = 0x2000;

TStrings::TStrings() noexcept :
  TObjectList(OBJECT_CLASS_TStrings),
  FDelimiter(L','),
  FQuoteChar(L'"')
{
}

TStrings::TStrings(TObjectClassId Kind) noexcept :
  TObjectList(Kind),
  FDelimiter(L','),
  FQuoteChar(L'"')
{
}

void TStrings::SetTextStr(UnicodeString Text)
{
  BeginUpdate();
  SCOPE_EXIT
  {
    EndUpdate();
  };
  Clear();
  const wchar_t *P = Text.c_str();
  // if (P != nullptr)
  {
    while (*P != 0x00)
    {
      const wchar_t *Start = P;
      while (!((*P == 0x00) || (*P == 0x0A) || (*P == 0x0D)))
      {
        P++;
      }
      UnicodeString S;
      S.SetLength(P - Start);
      memmove(ToWChar(S), Start, (P - Start) * sizeof(wchar_t));
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
  wchar_t LOldDelimiter = GetDelimiter();
  wchar_t LOldQuoteChar = GetQuoteChar();
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
  const intptr_t Count = GetCount();
  if ((Count == 1) && GetString(0).IsEmpty())
  {
    Result = GetQuoteChar() + GetQuoteChar();
  }
  else
  {
    for (intptr_t Index = 0; Index < GetCount(); ++Index)
    {
      UnicodeString Line = GetString(Index);
      Result += GetQuoteChar() + Line + GetQuoteChar() + GetDelimiter();
    }
    if (Result.Length() > 0)
      Result.SetLength(Result.Length() - 1);
  }
  return Result;
}

static void tokenize(UnicodeString str, nb::vector_t<UnicodeString> &tokens,
  UnicodeString delimiters = L" ", const bool trimEmpty = false)
{
  intptr_t lastPos = 0;
  while (true)
  {
    intptr_t pos = str.FindFirstOf(delimiters.c_str(), lastPos);
    if (pos == nb::NPOS)
    {
      pos = str.Length();

      if (pos != lastPos || !trimEmpty)
      {
        tokens.push_back(
          UnicodeString(str.data() + lastPos, pos - lastPos));
      }
      break;
    }
    if (pos != lastPos || !trimEmpty)
    {
      tokens.push_back(
        UnicodeString(str.data() + lastPos, pos - lastPos));
    }

    lastPos = pos + 1;
  }
}

void TStrings::SetDelimitedText(UnicodeString Value)
{
  BeginUpdate();
  SCOPE_EXIT
  {
    EndUpdate();
  };
  Clear();
  nb::vector_t<UnicodeString> Lines;
  UnicodeString Delimiter(UnicodeString(GetDelimiter()) + L'\n');
  tokenize(Value, Lines, Delimiter, true);
  for (const auto& Line: Lines)
  {
    Add(Line);
  }
}

intptr_t TStrings::CompareStrings(UnicodeString S1, UnicodeString S2) const
{
  return ::AnsiCompareText(S1, S2);
}

void TStrings::Assign(const TPersistent *Source)
{
  const TStrings *Strings = dyn_cast<TStrings>(Source);
  if (Strings != nullptr)
  {
    BeginUpdate();
    SCOPE_EXIT
    {
      EndUpdate();
    };
    Clear();
    DebugAssert(Strings);
    FQuoteChar = Strings->FQuoteChar;
    FDelimiter = Strings->FDelimiter;
    AddStrings(Strings);
  }
  else
  {
    TPersistent::Assign(Source);
  }
}

intptr_t TStrings::Add(UnicodeString S, TObject *AObject)
{
  const intptr_t Result = GetCount();
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
  const intptr_t Count = GetCount();
  intptr_t Size = 0;
  UnicodeString LB(L"\r\n");
  for (intptr_t Index = 0; Index < Count; ++Index)
  {
    Size += GetString(Index).Length() + LB.Length();
  }
  wchar_t *Buffer = Result.SetLength(Size);
  wchar_t *P = Buffer;
  for (intptr_t Index = 0; Index < Count; ++Index)
  {
    UnicodeString S = GetString(Index);
    ::size_t L = S.Length() * sizeof(wchar_t);
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

void TStrings::SetText(UnicodeString Text)
{
  SetTextStr(Text);
}

void TStrings::SetCommaText(UnicodeString Value)
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

intptr_t TStrings::AddObject(UnicodeString S, TObject *AObject)
{
  const intptr_t Result = Add(S, AObject);
  return Result;
}

void TStrings::InsertObject(intptr_t Index, UnicodeString Key, TObject *AObject)
{
  Insert(Index, Key, AObject);
}

bool TStrings::Equals(const TStrings *Value) const
{
  if (GetCount() != Value->GetCount())
  {
    return false;
  }
  for (intptr_t Index = 0; Index < GetCount(); ++Index)
  {
    if (GetString(Index) != Value->GetString(Index))
    {
      return false;
    }
  }
  return true;
}

void TStrings::SetString(intptr_t Index, UnicodeString S)
{
  TObject *TempObject = GetObj(Index);
  Delete(Index);
  InsertObject(Index, S, TempObject);
}

void TStrings::SetDuplicates(TDuplicatesEnum Value)
{
  FDuplicates = Value;
}

void TStrings::Move(intptr_t CurIndex, intptr_t NewIndex)
{
  if (CurIndex != NewIndex)
  {
    BeginUpdate();
    SCOPE_EXIT
    {
      EndUpdate();
    };
    UnicodeString TempString = GetString(CurIndex);
    TObject *TempObject = GetObj(CurIndex);
    Delete(CurIndex);
    InsertObject(NewIndex, TempString, TempObject);
  }
}

intptr_t TStrings::IndexOf(UnicodeString S) const
{
  for (intptr_t Result = 0; Result < GetCount(); Result++)
  {
    if (CompareStrings(GetString(Result), S) == 0)
    {
      return Result;
    }
  }
  return nb::NPOS;
}

intptr_t TStrings::IndexOfName(UnicodeString Name) const
{
  for (intptr_t Index = 0; Index < GetCount(); ++Index)
  {
    UnicodeString S = GetString(Index);
    const intptr_t P = ::AnsiPos(S, L'=');
    if ((P > 0) && (CompareStrings(S.SubStr(1, P - 1), Name) == 0))
    {
      return Index;
    }
  }
  return nb::NPOS;
}

UnicodeString TStrings::GetName(intptr_t Index) const
{
  return ExtractName(GetString(Index));
}

void TStrings::SetName(intptr_t /*Index*/, UnicodeString /*Value*/)
{
  ThrowNotImplemented(2012);
}

UnicodeString TStrings::ExtractName(UnicodeString S) const
{
  UnicodeString Result = S;
  const intptr_t P = ::AnsiPos(Result, L'=');
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

UnicodeString TStrings::GetValue(UnicodeString Name) const
{
  UnicodeString Result;
  const intptr_t Index = IndexOfName(Name);
  if (Index >= 0)
  {
    Result = GetString(Index).SubStr(Name.Length() + 2);
  }
  return Result;
}

void TStrings::SetValue(UnicodeString Name, UnicodeString Value)
{
  intptr_t Index = IndexOfName(Name);
  if (!Value.IsEmpty())
  {
    if (Index < 0)
    {
      Index = Add("");
    }
    SetString(Index, Name + L'=' + Value);
  }
  else
  {
    if (Index >= 0)
    {
      Delete(Index);
    }
  }
}

UnicodeString TStrings::GetValueFromIndex(intptr_t Index) const
{
  UnicodeString Name = GetName(Index);
  UnicodeString Result = GetValue(Name);
  return Result;
}

void TStrings::AddStrings(const TStrings *Strings)
{
  BeginUpdate();
  SCOPE_EXIT
  {
    EndUpdate();
  };
  for (intptr_t Index = 0; Index < Strings->GetCount(); ++Index)
  {
    AddObject(Strings->GetString(Index), Strings->GetObj(Index));
  }
}

void TStrings::Append(UnicodeString Value)
{
  Insert(GetCount(), Value);
}

void TStrings::SaveToStream(TStream * /*Stream*/) const
{
  ThrowNotImplemented(12);
}

intptr_t StringListCompareStrings(TStringList *List, intptr_t Index1, intptr_t Index2)
{
  const intptr_t Result = List->CompareStrings(List->FStrings[Index1],
      List->FStrings[Index2]);
  return Result;
}

TStringList::TStringList(TObjectClassId Kind) noexcept :
  TStrings(Kind),
  FSorted(false),
  FCaseSensitive(false)
{
  SetOwnsObjects(false);
}

void TStringList::Assign(const TPersistent *Source)
{
  TStrings::Assign(Source);
}

intptr_t TStringList::GetCount() const
{
  DebugAssert(nb::ToIntPtr(FStrings.size()) == TObjectList::GetCount());
  return nb::ToIntPtr(FStrings.size());
}

intptr_t TStringList::Add(UnicodeString S)
{
  return AddObject(S, nullptr);
}

intptr_t TStringList::AddObject(UnicodeString S, TObject *AObject)
{
  intptr_t Result = 0;
  if (!GetSorted())
  {
    Result = GetCount();
  }
  else
  {
    if (Find(S, Result))
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
        break;
      }
    }
    else
    {
      Result = GetCount();
    }
  }
  InsertItem(Result, S, AObject);
  return Result;
}

bool TStringList::Find(UnicodeString S, intptr_t &Index) const
{
  bool Result = false;
  intptr_t L = 0;
  intptr_t H = GetCount() - 1;
  while ((H != nb::NPOS) && (L <= H))
  {
    const intptr_t Idx = (L + H) >> 1;
    const intptr_t C = CompareStrings(FStrings[Idx], S);
    if (C < 0)
    {
      L = Idx + 1;
    }
    else
    {
      H = Idx - 1;
      if (C == 0)
      {
        Result = true;
        if (FDuplicates != dupAccept)
        {
          L = Idx;
        }
      }
    }
  }
  Index = L;
  return Result;
}

intptr_t TStringList::IndexOf(UnicodeString S) const
{
  intptr_t Result = nb::NPOS;
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

void TStringList::SetString(intptr_t Index, UnicodeString S)
{
  if (GetSorted())
  {
    Error(SSortedListError, 0);
  }
  if ((Index == nb::NPOS) || (Index > nb::ToIntPtr(FStrings.size())))
  {
    Error(SListIndexError, Index);
  }
  Changing();
  if (Index < nb::ToIntPtr(FStrings.size()))
  {
    FStrings[Index] = S;
  }
  else
  {
    Insert(Index, S);
  }
  Changed();
}

void TStringList::Delete(intptr_t Index)
{
  if ((Index == nb::NPOS) || (Index >= nb::ToIntPtr(FStrings.size())))
  {
    Error(SListIndexError, Index);
  }
  Changing();
  FStrings.erase(FStrings.begin() + Index);
  TObjectList::Delete(Index);
  Changed();
}

void TStringList::InsertObject(intptr_t Index, UnicodeString Key, TObject *AObject)
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

void TStringList::InsertItem(intptr_t Index, UnicodeString S, TObject *AObject)
{
  if ((Index == nb::NPOS) || (Index > GetCount()))
  {
    Error(SListIndexError, Index);
  }
  Changing();
  if (Index == GetCount())
  {
    FStrings.push_back(S);
    TObjectList::Add(AObject);
  }
  else
  {
    FStrings.insert(FStrings.begin() + Index, S);
    TObjectList::Insert(Index, AObject);
  }
  Changed();
}

const UnicodeString &TStringList::GetStringRef(intptr_t Index) const
{
  if ((Index == nb::NPOS) || (Index > nb::ToIntPtr(FStrings.size())))
  {
    Error(SListIndexError, Index);
  }
  if (Index == nb::ToIntPtr(FStrings.size()))
  {
    const_cast<TStringList *>(this)->InsertItem(Index, UnicodeString(), nullptr);
  }
  return FStrings[Index];
}

const UnicodeString &TStringList::GetString(intptr_t Index) const
{
  return GetStringRef(Index);
}

UnicodeString TStringList::GetString(intptr_t Index)
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

void TStringList::LoadFromFile(UnicodeString AFileName)
{
  HANDLE FileHandle = ::CreateFile(AFileName.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
  if (FileHandle != INVALID_HANDLE_VALUE)
  {
    TSafeHandleStream Stream(FileHandle);
    const int64_t Size = Stream.GetSize();
    TFileBuffer FileBuffer;
    FileBuffer.LoadStream(&Stream, Size, True);
    bool ConvertToken = false;
    FileBuffer.Convert(eolCRLF, eolCRLF, cpRemoveCtrlZ | cpRemoveBOM, ConvertToken);
    SAFE_CLOSE_HANDLE(FileHandle);
    UnicodeString Str(FileBuffer.GetData(), nb::ToIntPtr(FileBuffer.GetSize()));
    SetTextStr(Str);
  }
}

void TStringList::SetObj(intptr_t Index, TObject *AObject)
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

void TStringList::Insert(intptr_t Index, UnicodeString S, TObject *AObject)
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

void TStringList::QuickSort(intptr_t L, intptr_t R, TStringListSortCompare SCompare)
{
  intptr_t Index;
  do
  {
    Index = L;
    intptr_t J = R;
    intptr_t P = (L + R) >> 1;
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

void TStringList::ExchangeItems(intptr_t Index1, intptr_t Index2)
{
  bool Owns = GetOwnsObjects();
  SetOwnsObjects(false);
  {
    SCOPE_EXIT
    {
      SetOwnsObjects(Owns);
    };
    UnicodeString SItem1 = FStrings[Index1];
    TObject *OItem1 = TObjectList::GetObj(Index1);
    FStrings[Index1] = FStrings[Index2];
    TObjectList::SetItem(Index1, TObjectList::GetObj(Index2));
    FStrings[Index2] = SItem1;
    TObjectList::SetItem(Index2, OItem1);
  }
}

intptr_t TStringList::CompareStrings(UnicodeString S1, UnicodeString S2) const
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

bool TDateTime::operator==(const TDateTime &rhs) const
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

UnicodeString TDateTime::FormatString(const wchar_t *fmt) const
{
  (void)fmt;
  uint16_t H, N, S, MS;
  DecodeTime(H, N, S, MS);
  UnicodeString Result = FORMAT("%02d.%02d.%02d.%03d", H, N, S, MS);
  return Result;
}

void TDateTime::DecodeDate(uint16_t &Y,
  uint16_t &M, uint16_t &D) const
{
  ::DecodeDate(*this, Y, M, D);
}

void TDateTime::DecodeTime(uint16_t &H,
  uint16_t &N, uint16_t &S, uint16_t &MS) const
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

TDateTime SpanOfNowAndThen(const TDateTime &ANow, const TDateTime &AThen)
{
  TDateTime Result;
  if (ANow < AThen)
    Result = AThen - ANow;
  else
    Result = ANow - AThen;
  return Result;
}

double MilliSecondSpan(const TDateTime &ANow, const TDateTime &AThen)
{
  const double Result = MSecsPerDay * SpanOfNowAndThen(ANow, AThen);
  return Result;
}

int64_t MilliSecondsBetween(const TDateTime &ANow, const TDateTime &AThen)
{
  const double Result = floor(MilliSecondSpan(ANow, AThen));
  return nb::ToInt64(Result);
}

int64_t SecondsBetween(const TDateTime &ANow, const TDateTime &AThen)
{
  return MilliSecondsBetween(ANow, AThen);
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

int TSHFileInfo::GetFileIconIndex(UnicodeString StrFileName, BOOL bSmallIcon) const
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

int TSHFileInfo::GetDirIconIndex(BOOL bSmallIcon)
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

UnicodeString TSHFileInfo::GetFileType(UnicodeString StrFileName)
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

NB_DEFINE_CLASS_ID(EStreamError);
class EStreamError : public ExtException
{
public:
  explicit EStreamError(UnicodeString Msg) :
    ExtException(OBJECT_CLASS_EStreamError, static_cast<const Exception *>(nullptr), Msg)
  {
  }
};

void TStream::ReadBuffer(void *Buffer, int64_t Count)
{
  if ((Count != 0) && (Read(Buffer, Count) != Count))
  {
    throw Exception(FMTLOAD(SReadError));
  }
}

void TStream::WriteBuffer(const void *Buffer, int64_t Count)
{
  if ((Count != 0) && (Write(Buffer, Count) != Count))
  {
    throw Exception(FMTLOAD(SWriteError));
  }
}

int64_t TStream::GetPosition() const
{
  return Seek(0, soFromCurrent);
}

int64_t TStream::GetSize() const
{
  const int64_t Pos = Seek(0, soFromCurrent);
  const int64_t Result = Seek(0, soFromEnd);
  const auto res = Seek(Pos, soFromBeginning);
  DebugAssert(res == Pos);
  return Result;
}

void TStream::SetPosition(const int64_t Pos)
{
  const auto res = Seek(Pos, soFromBeginning);
  DebugAssert(res == Pos);
}

static void ReadError(UnicodeString Name)
{
  throw Exception(FORMAT("InvalidRegType: %s", Name)); // FIXME ERegistryException.CreateResFmt(@SInvalidRegType, [Name]);
}

THandleStream::THandleStream(HANDLE AHandle) noexcept :
  FHandle(AHandle)
{
}

int64_t THandleStream::Read(void *Buffer, int64_t Count)
{
  int64_t Result = ::FileRead(FHandle, Buffer, Count);
  if (Result == -1)
  {
    Result = 0;
  }
  return Result;
}

int64_t THandleStream::Write(const void *Buffer, int64_t Count)
{
  int64_t Result = ::FileWrite(FHandle, Buffer, Count);
  if (Result == -1)
  {
    Result = 0;
  }
  return Result;
}

int64_t THandleStream::Seek(int64_t Offset, int Origin) const
{
  const int64_t Result = ::FileSeek(FHandle, Offset, nb::ToDWord(Origin));
  return Result;
}

int64_t THandleStream::Seek(const int64_t Offset, TSeekOrigin Origin) const
{
  int origin = FILE_BEGIN;
  switch (Origin)
  {
  case soFromBeginning:
    origin = FILE_BEGIN;
    break;
  case soFromCurrent:
    origin = FILE_CURRENT;
    break;
  case soFromEnd:
    origin = FILE_END;
    break;
  }
  return Seek(Offset, origin);
}

void THandleStream::SetSize(const int64_t NewSize)
{
  const auto res = Seek(NewSize, soFromBeginning);
  DebugAssert(res == NewSize);
  // LARGE_INTEGER li;
  // li.QuadPart = size;
  // if (SetFilePointer(fh.get(), li.LowPart, &li.HighPart, FILE_BEGIN) == -1)
  // handleLastErrorImpl(_path);
  ::Win32Check(::SetEndOfFile(FHandle) > 0);
}

TSafeHandleStream::TSafeHandleStream(THandle AHandle) noexcept :
  THandleStream(AHandle)
{
}

int64_t TSafeHandleStream::Read(void *Buffer, int64_t Count)
{
  const int64_t Result = ::FileRead(FHandle, Buffer, Count);
  if (Result == nb::ToInt64(-1))
  {
    ::RaiseLastOSError();
  }
  return Result;
}

int64_t TSafeHandleStream::Write(const void *Buffer, int64_t Count)
{
  const int64_t Result = ::FileWrite(FHandle, Buffer, Count);
  if (Result == -1)
  {
    ::RaiseLastOSError();
  }
  return Result;
}

TMemoryStream::TMemoryStream() noexcept :
  FMemory(nullptr),
  FSize(0),
  FPosition(0),
  FCapacity(0)
{
}

TMemoryStream::~TMemoryStream() noexcept
{
  Clear();
}

int64_t TMemoryStream::Read(void *Buffer, int64_t Count)
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
      memmove(Buffer, reinterpret_cast<char *>(FMemory) + FPosition, static_cast<::size_t>(Result));
      FPosition += Result;
      return Result;
    }
  }
  Result = 0;
  return Result;
}

int64_t TMemoryStream::Seek(int64_t Offset, int Origin) const
{
  return Seek(Offset, static_cast<TSeekOrigin>(Origin));
}

int64_t TMemoryStream::Seek(const int64_t Offset, TSeekOrigin Origin) const
{
  switch (Origin)
  {
  case soFromBeginning:
    FPosition = Offset;
    break;
  case soFromCurrent:
    FPosition += Offset;
    break;
  case soFromEnd:
    FPosition = FSize + Offset;
    break;
  }
  const int64_t Result = FPosition;
  return Result;
}

void TMemoryStream::SaveToStream(TStream *Stream)
{
  if (FSize != 0)
  {
    Stream->WriteBuffer(FMemory, FSize);
  }
}

void TMemoryStream::SaveToFile(UnicodeString /*AFileName*/)
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

void TMemoryStream::SetSize(const int64_t NewSize)
{
  const int64_t OldPosition = FPosition;
  SetCapacity(NewSize);
  FSize = NewSize;
  if (OldPosition > NewSize)
  {
    const auto res = Seek(0, soFromEnd);
    DebugAssert(res == NewSize);
  }
}

void TMemoryStream::SetCapacity(int64_t NewCapacity)
{
  SetPointer(Realloc(NewCapacity), FSize);
  FCapacity = NewCapacity;
}

void *TMemoryStream::Realloc(int64_t &NewCapacity)
{
  if ((NewCapacity > 0) && (NewCapacity != FSize))
  {
    NewCapacity = (NewCapacity + (MemoryDelta - 1)) & ~(MemoryDelta - 1);
  }
  void *Result = FMemory;
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
        Result = nb::calloc<void *>(1, static_cast<::size_t>(NewCapacity));
      }
      else
      {
        Result = nb::realloc<void *>(FMemory, static_cast<::size_t>(NewCapacity));
      }
      if (Result == nullptr)
      {
        throw EStreamError(FMTLOAD(SMemoryStreamError));
      }
    }
  }
  return Result;
}

void TMemoryStream::SetPointer(void *Ptr, int64_t ASize)
{
  FMemory = Ptr;
  FSize = ASize;
}

int64_t TMemoryStream::Write(const void *Buffer, int64_t Count)
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
        Buffer, static_cast<::size_t>(Count));
      FPosition = Pos;
      Result = Count;
    }
  }
  return Result;
}

bool IsRelative(UnicodeString Value)
{
  return !(!Value.IsEmpty() && (Value[1] == L'\\'));
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

class ERegistryException final : public std::exception
{
};

TRegistry::TRegistry() noexcept :
  FCurrentKey(nullptr),
  FRootKey(nullptr),
  FCloseRootKey(false),
  FAccess(KEY_ALL_ACCESS)
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

void TRegistry::GetValueNames(TStrings *Names) const
{
  Names->Clear();
  TRegKeyInfo Info{};
  UnicodeString S;
  if (GetKeyInfo(Info))
  {
    S.SetLength(Info.MaxValueLen + 1);
    for (DWORD Index = 0; Index < Info.NumValues; Index++)
    {
      DWORD Len = Info.MaxValueLen + 1;
      RegEnumValue(GetCurrentKey(), Index, &S[1], &Len, nullptr, nullptr, nullptr, nullptr);
      Names->Add(S);
    }
  }
}

void TRegistry::GetKeyNames(TStrings *Names) const
{
  Names->Clear();
  TRegKeyInfo Info{};
  UnicodeString S;
  if (GetKeyInfo(Info))
  {
    S.SetLength(nb::ToIntPtr(Info.MaxSubKeyLen) + 1);
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

bool TRegistry::OpenKey(UnicodeString AKey, bool CanCreate)
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
      S = FCurrentPath + L'\\' + S;
    }
    ChangeKey(TempKey, S);
  }
  return Result;
}

bool TRegistry::DeleteKey(UnicodeString AKey)
{
  const bool Relative = IsRelative(AKey);
  HKEY OldKey = GetCurrentKey();
  HKEY DeleteKey = GetKey(AKey);
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
      for (intptr_t Index = nb::ToIntPtr(Info.NumSubKeys) - 1; Index >= 0; Index--)
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

bool TRegistry::DeleteValue(UnicodeString Value) const
{
  const bool Result = RegDeleteValue(GetCurrentKey(), Value.c_str()) == ERROR_SUCCESS;
  return Result;
}

bool TRegistry::KeyExists(UnicodeString SubKey) const
{
  uint32_t OldAccess = FAccess;
  SCOPE_EXIT
  {
    FAccess = OldAccess;
  };
  FAccess = STANDARD_RIGHTS_READ | KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS;
  HKEY TempKey = GetKey(SubKey);
  if (TempKey != nullptr)
  {
    ::RegCloseKey(TempKey);
  }
  const bool Result = TempKey != nullptr;
  return Result;
}

bool TRegistry::ValueExists(UnicodeString Value) const
{
  TRegDataInfo Info{};
  const bool Result = GetDataInfo(Value, Info);
  return Result;
}

bool TRegistry::GetDataInfo(UnicodeString ValueName, TRegDataInfo &Value) const
{
  DWORD DataType;
  nb::ClearStruct(Value);
  const bool Result = (::RegQueryValueEx(GetCurrentKey(), ValueName.c_str(), nullptr, &DataType, nullptr,
        &Value.DataSize) == ERROR_SUCCESS);
  Value.RegData = DataTypeToRegData(DataType);
  return Result;
}

TRegDataType TRegistry::GetDataType(UnicodeString ValueName) const
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

DWORD TRegistry::GetDataSize(UnicodeString ValueName) const
{
  DWORD Result{};
  TRegDataInfo Info{};
  if (GetDataInfo(ValueName, Info))
  {
    Result = Info.DataSize;
  }
  else
  {
    Result = nb::ToDWord(-1);
  }
  return Result;
}

bool TRegistry::ReadBool(UnicodeString Name) const
{
  const bool Result = ReadInteger(Name) != 0;
  return Result;
}

TDateTime TRegistry::ReadDateTime(UnicodeString Name) const
{
  TDateTime Result = TDateTime(ReadFloat(Name));
  return Result;
}

double TRegistry::ReadFloat(UnicodeString Name) const
{
  double Result = 0.0;
  TRegDataType RegData = rdUnknown;
  const int Len = GetData(Name, &Result, sizeof(double), RegData);
  if ((RegData != rdBinary) || (Len != sizeof(double)))
  {
    ReadError(Name);
  }
  return Result;
}

intptr_t TRegistry::ReadIntPtr(UnicodeString Name) const
{
  intptr_t Result = 0;
  TRegDataType RegData = rdUnknown;
  const auto res = GetData(Name, &Result, sizeof(Result), RegData);
  DebugAssert(res == sizeof(Result));
  if (RegData != rdIntPtr)
  {
    ReadError(Name);
  }
  return Result;
}

int TRegistry::ReadInteger(UnicodeString Name) const
{
  DWORD Result = 0;
  TRegDataType RegData = rdUnknown;
  int res = GetData(Name, &Result, sizeof(Result), RegData);
  DebugAssert(res == sizeof(Result));
  if (RegData != rdInteger)
  {
    ReadError(Name);
  }
  return Result;
}

int64_t TRegistry::ReadInt64(UnicodeString Name) const
{
  int64_t Result = 0;
  const auto res = ReadBinaryData(Name, &Result, sizeof(Result));
  DebugAssert(res == sizeof(Result));
  return Result;
}

UnicodeString TRegistry::ReadString(UnicodeString Name) const
{
  UnicodeString Result;
  const intptr_t Len = GetDataSize(Name);
  if (Len > 0)
  {
    TRegDataType RegData = rdUnknown;
    wchar_t *Buffer = Result.SetLength(Len);
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

UnicodeString TRegistry::ReadStringRaw(UnicodeString Name) const
{
  UnicodeString Result = ReadString(Name);
  return Result;
}

::size_t TRegistry::ReadBinaryData(UnicodeString Name,
  void *Buffer, ::size_t BufSize) const
{
  ::size_t Result{};
  TRegDataInfo Info{};
  if (GetDataInfo(Name, Info))
  {
    Result = static_cast<::size_t>(Info.DataSize);
    TRegDataType RegData = Info.RegData;
    if (((RegData == rdBinary) || (RegData == rdUnknown)) && (Result <= BufSize))
    {
      const size_t res = GetData(Name, Buffer, Result, RegData);
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

int TRegistry::GetData(UnicodeString Name, void *Buffer,
  intptr_t ABufSize, TRegDataType &RegData) const
{
  DWORD DataType = REG_NONE;
  DWORD BufSize = nb::ToDWord(ABufSize);
  if (::RegQueryValueEx(GetCurrentKey(), Name.c_str(), nullptr, &DataType,
      reinterpret_cast<BYTE *>(Buffer), &BufSize) != ERROR_SUCCESS)
  {
    throw Exception("RegQueryValueEx failed"); // FIXME ERegistryException.CreateResFmt(@SRegGetDataFailed, [Name]);
  }
  RegData = DataTypeToRegData(DataType);
  int Result = nb::ToInt(BufSize);
  return Result;
}

void TRegistry::PutData(UnicodeString Name, const void *Buffer,
  intptr_t ABufSize, TRegDataType RegData)
{
  const DWORD DataType = RegDataToDataType(RegData);
  if (::RegSetValueEx(GetCurrentKey(), Name.c_str(), 0, DataType,
      reinterpret_cast<const BYTE *>(Buffer), nb::ToDWord(ABufSize)) != ERROR_SUCCESS)
  {
    throw Exception("RegSetValueEx failed"); // ERegistryException(); // FIXME .CreateResFmt(SRegSetDataFailed, Name.c_str());
  }
}

void TRegistry::WriteBool(UnicodeString Name, bool Value)
{
  WriteInteger(Name, Value);
}

void TRegistry::WriteDateTime(UnicodeString Name, const TDateTime &Value)
{
  double Val = Value.GetValue();
  PutData(Name, &Val, sizeof(double), rdBinary);
}

void TRegistry::WriteFloat(UnicodeString Name, double Value)
{
  PutData(Name, &Value, sizeof(double), rdBinary);
}

void TRegistry::WriteString(UnicodeString Name, UnicodeString Value)
{
  WriteStringRaw(Name, Value);
}

void TRegistry::WriteStringRaw(UnicodeString Name, UnicodeString Value)
{
  PutData(Name, Value.c_str(), Value.Length() * sizeof(wchar_t), rdString);
}

void TRegistry::WriteIntPtr(UnicodeString Name, intptr_t Value)
{
  PutData(Name, &Value, sizeof(Value), rdIntPtr);
}

void TRegistry::WriteInteger(UnicodeString Name, int Value)
{
  DWORD Val = nb::ToDWord(Value);
  PutData(Name, &Val, sizeof(Val), rdInteger);
}

void TRegistry::WriteInt64(UnicodeString Name, int64_t Value)
{
  WriteBinaryData(Name, &Value, sizeof(Value));
}

void TRegistry::WriteBinaryData(UnicodeString Name,
  const void *Buffer, ::size_t BufSize)
{
  PutData(Name, Buffer, BufSize, rdBinary);
}

void TRegistry::ChangeKey(HKEY Value, UnicodeString APath)
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

HKEY TRegistry::GetKey(UnicodeString AKey) const
{
  const bool Relative = IsRelative(AKey);
  HKEY Result = nullptr;
  if (::RegOpenKeyEx(GetBaseKey(Relative), AKey.c_str(), 0, FAccess, &Result) == ERROR_SUCCESS)
    return Result;
  return nullptr;
}

bool TRegistry::GetKeyInfo(TRegKeyInfo &Value) const
{
  nb::ClearStruct(Value);
  const bool Result = ::RegQueryInfoKey(GetCurrentKey(), nullptr, nullptr, nullptr, &Value.NumSubKeys,
      &Value.MaxSubKeyLen, nullptr, &Value.NumValues, &Value.MaxValueLen,
      &Value.MaxDataLen, nullptr, &Value.FileTime) == ERROR_SUCCESS;
  return Result;
}

TShortCut::TShortCut(intptr_t Value) noexcept :
  FValue(Value)
{
}

TShortCut::operator intptr_t() const
{
  return FValue;
}

bool TShortCut::operator<(const TShortCut &rhs) const
{
  return FValue < rhs.FValue;
}

void GetLocaleFormatSettings(int LCID, TFormatSettings &FormatSettings)
{
  (void)LCID;
  (void)FormatSettings;
  ThrowNotImplemented(1204);
}


TGlobals::TGlobals() noexcept
{
  InitPlatformId();
}

void TGlobals::InitPlatformId()
{
  OSVERSIONINFO OSVersionInfo;
  OSVersionInfo.dwOSVersionInfoSize = sizeof(OSVersionInfo);
  if (::GetVersionEx(&OSVersionInfo) != FALSE)
  {
    Win32Platform = OSVersionInfo.dwPlatformId;
    Win32MajorVersion = OSVersionInfo.dwMajorVersion;
    Win32MinorVersion = OSVersionInfo.dwMinorVersion;
    Win32BuildNumber = OSVersionInfo.dwBuildNumber;
    memcpy(Win32CSDVersion, OSVersionInfo.szCSDVersion, sizeof(OSVersionInfo.szCSDVersion));
  }
}
