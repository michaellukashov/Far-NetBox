#pragma once

#include <iostream>
#include <string>
#include <sstream>

#include <Classes.hpp>
#include "Common.h"
#include "Exceptions.h"
#include <FileBuffer.h>
#include <Sysutils.hpp>

namespace Classes {

intptr_t __cdecl debug_printf(const wchar_t * format, ...)
{
  (void)format;
  intptr_t len = 0;
#ifdef NETBOX_DEBUG
  va_list args;
  va_start(args, format);
  len = _vscwprintf(format, args);
  std::wstring buf(len + 1, 0);
  vswprintf(const_cast<wchar_t *>(buf.c_str()), buf.size(), format, args);
  va_end(args);
  OutputDebugStringW(buf.c_str());
#endif
  return len;
}

intptr_t __cdecl debug_printf2(const char * format, ...)
{
  (void)format;
  intptr_t len = 0;
#ifdef NETBOX_DEBUG
  va_list args;
  va_start(args, format);
  len = _vscprintf(format, args);
  std::string buf(len + sizeof(char), 0);
  vsprintf_s(&buf[0], buf.size(), format, args);
  va_end(args);
  OutputDebugStringA(buf.c_str());
#endif
  return len;
}

//---------------------------------------------------------------------------
void Abort()
{
  throw Sysutils::EAbort(L"");
}

//---------------------------------------------------------------------------
void Error(int ErrorID, intptr_t data)
{
  DEBUG_PRINTF(L"begin: ErrorID = %d, data = %d", ErrorID, data);
  UnicodeString Msg = FMTLOAD(ErrorID, data);
  // DEBUG_PRINTF(L"Msg = %s", Msg.c_str());
  throw ExtException((Exception *)NULL, Msg);
}

//---------------------------------------------------------------------------
TPersistent::TPersistent()
{}

TPersistent::~TPersistent()
{}

void TPersistent::Assign(TPersistent * Source)
{
  if (Source != NULL)
  {
    Source->AssignTo(this);
  }
  else
  {
    AssignError(NULL);
  }
}

void TPersistent::AssignTo(TPersistent * Dest)
{
  Dest->AssignError(this);
}

TPersistent * TPersistent::GetOwner()
{
  return NULL;
}

void TPersistent::AssignError(TPersistent * Source)
{
  (void)Source;
  UnicodeString SourceName = L"nil";
  // if (Source != NULL)
  // SourceName = Source.ClassName
  // else
  // SourceName = "nil";
  // throw EConvertError(FMTLOAD(SAssignError, SourceName.c_str(), "nil"));
  throw std::exception("Cannot assign");
}

//---------------------------------------------------------------------------
TList::TList()
{
  Items(this);
}

TList::~TList()
{
  Clear();
}

intptr_t TList::GetCount() const
{
  return static_cast<intptr_t>(FList.size());
}

void TList::SetCount(intptr_t NewCount)
{
  if (NewCount == NPOS)
  {
    Classes::Error(SListCountError, NewCount);
  }
  if (NewCount <= static_cast<intptr_t>(FList.size()))
  {
    intptr_t sz = FList.size();
    for (intptr_t I = sz - 1; (I != NPOS) && (I >= NewCount); I--)
    {
      Delete(I);
    }
  }
  FList.resize(NewCount);
}

void * TList::operator [](intptr_t Index) const
{
  return FList[Index];
}

void *& TList::GetItem(intptr_t Index)
{
  return FList[Index];
}

void TList::SetItem(intptr_t Index, void * Item)
{
  if ((Index == NPOS) || (Index >= static_cast<intptr_t>(FList.size())))
  {
    Classes::Error(SListIndexError, Index);
  }
  FList.insert(FList.begin() + Index, Item);
}

intptr_t TList::Add(void * Value)
{
  intptr_t Result = static_cast<intptr_t>(FList.size());
  FList.push_back(Value);
  return Result;
}

void * TList::Extract(void * Item)
{
  if (Remove(Item) != NPOS)
  {
    return Item;
  }
  else
  {
    return NULL;
  }
}

intptr_t TList::Remove(void * Item)
{
  intptr_t Result = IndexOf(Item);
  if (Result != NPOS)
  {
    Delete(Result);
  }
  return Result;
}

void TList::Move(intptr_t CurIndex, intptr_t NewIndex)
{
  if (CurIndex != NewIndex)
  {
    if ((NewIndex == NPOS) || (NewIndex >= static_cast<intptr_t>(FList.size())))
    {
      Classes::Error(SListIndexError, NewIndex);
    }
    void * Item = GetItem(CurIndex);
    FList[CurIndex] = NULL;
    Delete(CurIndex);
    Insert(NewIndex, NULL);
    FList[NewIndex] = Item;
  }
}

void TList::Delete(intptr_t Index)
{
  if ((Index == NPOS) || (Index >= static_cast<intptr_t>(FList.size())))
  {
    Classes::Error(SListIndexError, Index);
  }
  void * Temp = GetItem(Index);
  FList.erase(FList.begin() + Index);
  if (Temp != NULL)
  {
    Notify(Temp, lnDeleted);
  }
}

void TList::Insert(intptr_t Index, void * Item)
{
  if ((Index == NPOS) || (Index > static_cast<intptr_t>(FList.size())))
  {
    Classes::Error(SListIndexError, Index);
  }
  if (Index <= static_cast<intptr_t>(FList.size()))
  {
    FList.insert(FList.begin() + Index, Item);
  }
  if (Item != NULL)
  {
    Notify(Item, lnAdded);
  }
}

intptr_t TList::IndexOf(void * Value) const
{
  intptr_t Result = 0;
  while ((Result < static_cast<intptr_t>(FList.size())) && (FList[Result] != Value))
  {
    Result++;
  }
  if (Result == static_cast<intptr_t>(FList.size()))
  {
    Result = NPOS;
  }
  return Result;
}

void TList::Clear()
{
  SetCount(0);
}

void QuickSort(std::vector<void *> & SortList, intptr_t L, intptr_t R,
  CompareFunc SCompare)
{
  intptr_t I;
  intptr_t J;
  do
  {
    I = L;
    J = R;
    void * P = SortList[(L + R) >> 1];
    do
    {
      while (SCompare(SortList[I], P) < 0)
        I++;
      while (SCompare(SortList[J], P) > 0)
        J--;
      if (I <= J)
      {
        if (I != J)
        {
          void * T = SortList[I];
          SortList[I] = SortList[J];
          SortList[J] = T;
        }
        I--;
        J--;
      }
    } while (I > J);
    if (L < J)
      QuickSort(SortList, L, J, SCompare);
    L = I;
  } while (I >= R);
}

void TList::Sort(CompareFunc Func)
{
  if (GetCount() > 1)
  {
    // qsort(FList.front(), GetCount().get(), sizeof(void *), Func);
    QuickSort(FList, 0, GetCount() - 1, Func);
  }
}

void TList::Notify(void * Ptr, TListNotification Action)
{
  (void)Ptr;
  (void)Action;
}

void TList::Sort()
{
  // if (FList.size() > 1)
    // QuickSort(FList, 0, GetCount() - 1, Compare);
  Classes::Error(SNotImplemented, 15);
}

//---------------------------------------------------------------------------
TObjectList::TObjectList() :
  FOwnsObjects(true)
{
  Items(this);
}

TObjectList::~TObjectList()
{
  Clear();
}

TObject * TObjectList::operator [](intptr_t Index) const
{
  return static_cast<TObject *>(parent::operator[](Index));
}

TObject *& TObjectList::GetItem(intptr_t Index)
{
  return reinterpret_cast<TObject *&>(parent::GetItem(Index));
}

void TObjectList::SetItem(intptr_t Index, TObject * Value)
{
  parent::SetItem(Index, Value);
}

intptr_t TObjectList::Add(TObject * Value)
{
  return parent::Add(Value);
}

intptr_t TObjectList::Remove(TObject * Value)
{
  return parent::Remove(Value);
}

void TObjectList::Extract(TObject * Value)
{
  parent::Extract(Value);
}

void TObjectList::Move(intptr_t Index, intptr_t To)
{
  parent::Move(Index, To);
}

void TObjectList::Delete(intptr_t Index)
{
  parent::Delete(Index);
}

void TObjectList::Insert(intptr_t Index, TObject * Value)
{
  parent::Insert(Index, Value);
}

intptr_t TObjectList::IndexOf(TObject * Value) const
{
  return parent::IndexOf(Value);
}

void TObjectList::Clear()
{
  parent::Clear();
}

bool TObjectList::GetOwnsObjects() const
{
  return FOwnsObjects;
}

void TObjectList::SetOwnsObjects(bool Value)
{
  FOwnsObjects = Value;
}

void TObjectList::Sort(CompareFunc func)
{
  parent::Sort(func);
}

void TObjectList::Notify(void * Ptr, TListNotification Action)
{
  if (GetOwnsObjects())
  {
    if (Action == lnDeleted)
    {
      // ((TObject *)Ptr)->Free();
      delete static_cast<TObject *>(Ptr);
    }
  }
  parent::Notify(Ptr, Action);
}

//---------------------------------------------------------------------------
const UnicodeString sLineBreak = L"\r\n";
const int MonthsPerYear = 12;
const int DaysPerWeek = 7;
const int MinsPerHour = 60;
const int SecsPerMin = 60;
const int SecsPerHour = MinsPerHour * SecsPerMin;
const int HoursPerDay = 24;
const int MinsPerDay  = HoursPerDay * MinsPerHour;
const int SecsPerDay  = MinsPerDay * SecsPerMin;
const int MSecsPerDay = SecsPerDay * MSecsPerSec;
const int MSecsPerSec = 1000;
// Days between 1/1/0001 and 12/31/1899
const int DateDelta = 693594;
const int UnixDateDelta = 25569;
const UnicodeString kernel32 = L"kernel32";
static const int MemoryDelta = 0x2000;
//---------------------------------------------------------------------------
TStrings::TStrings() :
  FDuplicates(dupAccept),
  FDelimiter(L','),
  FQuoteChar(L'"'),
  FUpdateCount(0)
{
  Text(this);
  CommaText(this);
  CaseSensitive(this);
  Sorted(this);
  Duplicates(this);
  Strings(this);
  Objects(this);
  Names(this);
  Values(this);
}

TStrings::~TStrings()
{
}

void TStrings::SetTextStr(const UnicodeString & Text)
{
  BeginUpdate();
  TRY_FINALLY (
  {
    Clear();
    const wchar_t * P = Text.c_str();
    if (P != NULL)
    {
      while (*P != 0x00)
      {
        const wchar_t * Start = P;
        while (!((*P == 0x00) || (*P == 0x0A) || (*P == 0x0D)))
        {
          P++;
        }
        UnicodeString S;
        S.SetLength(P - Start);
        memmove(const_cast<wchar_t *>(S.c_str()), Start, (P - Start) * sizeof(wchar_t));
        Add(S);
        if (*P == 0x0D) { P++; }
        if (*P == 0x0A) { P++; }
      }
    }
  }
  ,
  {
    EndUpdate();
  }
  );
}

UnicodeString TStrings::GetCommaText()
{
  wchar_t LOldDelimiter = GetDelimiter();
  wchar_t LOldQuoteChar = GetQuoteChar();
  FDelimiter = L',';
  FQuoteChar = L'"';
  UnicodeString Result;
  TRY_FINALLY (
  {
    Result = GetDelimitedText();
  }
  ,
  {
    FDelimiter = LOldDelimiter;
    FQuoteChar = LOldQuoteChar;
  }
  );
  return Result;
}

UnicodeString TStrings::GetDelimitedText()
{
  UnicodeString Result;
  intptr_t Count = GetCount();
  if ((Count == 1) && GetString(0).IsEmpty())
  {
    Result = GetQuoteChar() + GetQuoteChar();
  }
  else
  {
    for (intptr_t I = 0; I < GetCount(); I++)
    {
      UnicodeString line = GetString(I);
      Result += GetQuoteChar() + line + GetQuoteChar() + GetDelimiter();
    }
    if (Result.Length() > 0)
      Result.SetLength(Result.Length() - 1);
  }
  return Result;
}

template <class ContainerT>
void tokenize(const std::wstring & str, ContainerT & tokens,
  const std::wstring & delimiters = L" ", const bool trimEmpty = false)
{
  std::string::size_type pos, lastPos = 0;
  while(true)
  {
    pos = str.find_first_of(delimiters, lastPos);
    if(pos == std::string::npos)
    {
       pos = str.length();

       if(pos != lastPos || !trimEmpty)
          tokens.push_back(ContainerT::value_type(str.data()+lastPos,
                (ContainerT::value_type::size_type)pos-lastPos ));

       break;
    }
    else
    {
       if(pos != lastPos || !trimEmpty)
          tokens.push_back(ContainerT::value_type(str.data()+lastPos,
                (ContainerT::value_type::size_type)pos-lastPos ));
    }

    lastPos = pos + 1;
  }
  };

void TStrings::SetDelimitedText(const UnicodeString & Value)
{
  BeginUpdate();
  TRY_FINALLY (
  {
    Clear();
    std::vector<std::wstring> lines;
    std::wstring delim = std::wstring(1, GetDelimiter());
    delim.append(1, L'\n');
    std::wstring StrValue = Value.c_str();
    tokenize(StrValue, lines, delim, true);
    for (size_t i = 0; i < lines.size(); i++)
    {
      Add(lines[i]);
    }
  }
  ,
  {
    EndUpdate();
  }
  );
}

intptr_t TStrings::CompareStrings(const UnicodeString & S1, const UnicodeString & S2)
{
  return static_cast<intptr_t>(::AnsiCompareText(S1, S2));
}

void TStrings::Assign(TPersistent * Source)
{
  if (::InheritsFrom<TPersistent, TStrings>(Source))
  {
    BeginUpdate();
    {
      TRY_FINALLY (
      {
        Clear();
        // FDefined = TStrings(Source).FDefined;
        FQuoteChar = static_cast<TStrings *>(Source)->FQuoteChar;
        FDelimiter = static_cast<TStrings *>(Source)->FDelimiter;
        AddStrings(static_cast<TStrings *>(Source));
      }
      ,
      {
        EndUpdate();
      }
      );
    }
    return;
  }
  TPersistent::Assign(Source);
}

intptr_t TStrings::Add(const UnicodeString & S)
{
  intptr_t Result = GetCount();
  Insert(Result, S);
  return Result;
}

UnicodeString TStrings::GetText()
{
  return GetTextStr();
}

UnicodeString TStrings::GetTextStr()
{
  UnicodeString Result;
  intptr_t I, L, Size, Count;
  wchar_t * P = NULL;
  UnicodeString S, LB;

  Count = GetCount();
  // DEBUG_PRINTF(L"Count = %d", Count);
  Size = 0;
  LB = sLineBreak;
  for (I = 0; I < Count; I++)
  {
    Size += GetString(I).Length() + LB.Length();
  }
  Result.SetLength(Size);
  P = const_cast<wchar_t *>(Result.c_str());
  for (I = 0; I < Count; I++)
  {
    S = GetString(I);
    // DEBUG_PRINTF(L"  S = %s", S.c_str());
    L = S.Length() * sizeof(wchar_t);
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

void TStrings::SetText(const UnicodeString & Text)
{
  SetTextStr(Text);
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

intptr_t TStrings::AddObject(const UnicodeString & S, TObject * AObject)
{
  intptr_t Result = Add(S);
  PutObject(Result, AObject);
  return Result;
}

void TStrings::InsertObject(intptr_t Index, const UnicodeString & Key, TObject * AObject)
{
  Insert(Index, Key);
  PutObject(Index, AObject);
}

bool TStrings::Equals(TStrings * Strings)
{
  bool Result = false;
  if (GetCount() != Strings->GetCount())
  {
    return false;
  }
  for (intptr_t I = 0; I < GetCount(); ++I)
  {
    if (GetString(I) != Strings->GetString(I))
    {
      return false;
    }
  }
  Result = true;
  return Result;
}

void TStrings::PutString(intptr_t Index, const UnicodeString & S)
{
  TObject * TempObject = GetObjects(Index);
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
    {
      TRY_FINALLY (
      {
        UnicodeString TempString = GetString(CurIndex);
        TObject * TempObject = GetObjects(CurIndex);
        Delete(CurIndex);
        InsertObject(NewIndex, TempString, TempObject);
      }
      ,
      {
        EndUpdate();
      }
      );
    }
  }
}

intptr_t TStrings::IndexOf(const UnicodeString & S)
{
  // DEBUG_PRINTF(L"begin");
  for (intptr_t Result = 0; Result < GetCount(); Result++)
  {
    if (CompareStrings(GetString(Result), S) == 0)
    {
      return Result;
    }
  }
  // DEBUG_PRINTF(L"end");
  return NPOS;
}

intptr_t TStrings::IndexOfName(const UnicodeString & Name)
{
  for (intptr_t Index = 0; Index < GetCount(); ++Index)
  {
    UnicodeString S = GetString(Index);
    intptr_t P = ::AnsiPos(S, L'=');
    if ((P > 0) && (CompareStrings(S.SubStr(1, P - 1), Name) == 0))
    {
      return Index;
    }
  }
  return NPOS;
}

const UnicodeString TStrings::GetName(intptr_t Index)
{
  return ExtractName(GetString(Index));
}

UnicodeString TStrings::ExtractName(const UnicodeString & S) const
{
  UnicodeString Result = S;
  intptr_t P = ::AnsiPos(Result, L'=');
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

const UnicodeString TStrings::GetValue(const UnicodeString & Name)
{
  UnicodeString Result;
  intptr_t I = IndexOfName(Name);
  // DEBUG_PRINTF(L"Name = %s, I = %d", Name.c_str(), I);
  if (I >= 0)
  {
    Result = GetString(I).SubStr(Name.Length() + 2, -1);
  }
  // DEBUG_PRINTF(L"Result = %s", Result.c_str());
  return Result;
}

void TStrings::SetValue(const UnicodeString & Name, const UnicodeString & Value)
{
  intptr_t I = IndexOfName(Name);
  if (!Value.IsEmpty())
  {
    if (I < 0)
    {
      I = Add(L"");
    }
    PutString(I, Name + L'=' + Value);
  }
  else
  {
    if (I >= 0)
    {
      Delete(I);
    }
  }
}

void TStrings::AddStrings(TStrings * Strings)
{
  BeginUpdate();
  {
    TRY_FINALLY (
    {
      for (intptr_t I = 0; I < Strings->GetCount(); I++)
      {
        AddObject(Strings->GetString(I), Strings->GetObjects(I));
      }
    }
    ,
    {
      EndUpdate();
    }
    );
  }
}

void TStrings::Append(const UnicodeString & Value)
{
  Insert(GetCount(), Value);
}

void TStrings::SaveToStream(TStream * /*Stream*/) const
{
  Classes::Error(SNotImplemented, 12);
}

//---------------------------------------------------------------------------
intptr_t StringListCompareStrings(TStringList * List, intptr_t Index1, intptr_t Index2)
{
  intptr_t Result = List->CompareStrings(List->FList[Index1].FString,
                                    List->FList[Index2].FString);
  return Result;
}

TStringList::TStringList() :
  FSorted(false),
  FCaseSensitive(false)
{
}

TStringList::~TStringList()
{}

void TStringList::Assign(TPersistent * Source)
{
  parent::Assign(Source);
}

intptr_t TStringList::GetCount() const
{
  return static_cast<intptr_t>(FList.size());
}

void TStringList::Clear()
{
  FList.clear();
  // SetCount(0);
  // SetCapacity(0);
}

intptr_t TStringList::Add(const UnicodeString & S)
{
  return AddObject(S, NULL);
}

intptr_t TStringList::AddObject(const UnicodeString & S, TObject * AObject)
{
  // DEBUG_PRINTF(L"S = %s, Duplicates = %d", S.c_str(), FDuplicates);
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
          Classes::Error(SDuplicateString, 2);
          break;
      }
    }
  }
  InsertItem(Result, S, AObject);
  return Result;
}

bool TStringList::Find(const UnicodeString & S, intptr_t & Index)
{
  bool Result = false;
  intptr_t L = 0;
  intptr_t H = GetCount() - 1;
  while ((H != NPOS) && (L <= H))
  {
    intptr_t I = (L + H) >> 1;
    intptr_t C = CompareStrings(FList[I].FString, S);
    if (C < 0)
    {
      L = I + 1;
    }
    else
    {
      H = I - 1;
      if (C == 0)
      {
        Result = true;
        if (FDuplicates != dupAccept)
        {
          L = I;
        }
      }
    }
  }
  Index = L;
  return Result;
}

intptr_t TStringList::IndexOf(const UnicodeString & S)
{
  // DEBUG_PRINTF(L"begin");
  intptr_t Result = NPOS;
  if (!GetSorted())
  {
    Result = parent::IndexOf(S);
  }
  else
  {
    if (!Find(S, Result))
    {
      Result = NPOS;
    }
  }
  // DEBUG_PRINTF(L"end");
  return Result;
}

void TStringList::PutString(intptr_t Index, const UnicodeString & S)
{
  if (GetSorted())
  {
    Classes::Error(SSortedListError, 0);
  }
  if ((Index == NPOS) || (Index > static_cast<intptr_t>(FList.size())))
  {
    Classes::Error(SListIndexError, Index);
  }
  Changing();
  // DEBUG_PRINTF(L"Index = %d, size = %d", Index, FList.size());
  if (Index < static_cast<intptr_t>(FList.size()))
  {
    TObject * Temp = GetObjects(Index);
    TStringItem item;
    item.FString = S;
    item.FObject = Temp;
    FList[Index] = item;
  }
  else
  {
    Insert(Index, S);
  }
  Changed();
}

void TStringList::Delete(intptr_t Index)
{
  if ((Index == NPOS) || (Index >= static_cast<intptr_t>(FList.size())))
  {
    Classes::Error(SListIndexError, Index);
  }
  Changing();
  // DEBUG_PRINTF(L"Index = %d, Count = %d, Value = %s", Index, GetCount(), FList[Index].FString.c_str());
  FList.erase(FList.begin() + Index);
  Changed();
}

TObject *& TStringList::GetObjects(intptr_t Index)
{
  if ((Index == NPOS) || (Index >= static_cast<intptr_t>(FList.size())))
  {
    Classes::Error(SListIndexError, Index);
  }
  return FList[Index].FObject;
}

void TStringList::InsertObject(intptr_t Index, const UnicodeString & Key, TObject * AObject)
{
  if (GetSorted())
  {
    Classes::Error(SSortedListError, 0);
  }
  if ((Index == NPOS) || (Index > GetCount()))
  {
    Classes::Error(SListIndexError, Index);
  }
  InsertItem(Index, Key, AObject);
}

void TStringList::InsertItem(intptr_t Index, const UnicodeString & S, TObject * AObject)
{
  if ((Index == NPOS) || (Index > GetCount()))
  {
    Classes::Error(SListIndexError, Index);
  }
  Changing();
  // if (FCount == FCapacity) Grow();
  TStringItem Item;
  Item.FString = S;
  Item.FObject = AObject;
  FList.insert(FList.begin() + Index, Item);
  Changed();
}

UnicodeString & TStringList::GetString(intptr_t Index)
{
  // DEBUG_PRINTF(L"Index = %d, FList.size = %d", Index, FList.size());
  if ((Index == NPOS) || (Index > static_cast<intptr_t>(FList.size())))
  {
    Classes::Error(SListIndexError, Index);
  }
  if (Index == static_cast<intptr_t>(FList.size()))
  {
    InsertItem(Index, UnicodeString(), NULL);
  }
  return FList[Index].FString;
}

bool TStringList::GetCaseSensitive() const
{
  return FCaseSensitive;
}

void TStringList::SetCaseSensitive(bool Value)
{
  if (Value != FCaseSensitive)
  {
    FCaseSensitive = Value;
    if (Sorted)
    {
      Sort();
    }
  }
}

bool TStringList::GetSorted() const
{
  return FSorted;
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

void TStringList::LoadFromFile(const UnicodeString & FileName)
{
  HANDLE FileHandle = ::CreateFile(FileName.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
  if (FileHandle != INVALID_HANDLE_VALUE)
  {
    TSafeHandleStream Stream(FileHandle);
    __int64 Size = Stream.GetSize();
    TFileBuffer FileBuffer;
    __int64 Read = FileBuffer.LoadStream(&Stream, Size, True);
    bool ConvertToken = false;
    FileBuffer.Convert(eolCRLF, eolCRLF, cpRemoveCtrlZ | cpRemoveBOM, ConvertToken);
    ::CloseHandle(FileHandle);
    UnicodeString Str(FileBuffer.GetData(), static_cast<intptr_t>(FileBuffer.GetSize()));
    // DEBUG_PRINTF(L"Str = %s", Str.c_str());
    SetTextStr(Str);
  }
  /* FILE * f = NULL;
  _wfopen_s(&f, FileName, L"rb");
  if (!f)
    return;
  fseek(f, 0, SEEK_END);
  size_t sz = ftell(f);
  DEBUG_PRINTF(L"sz = %lu", sz);
  void * content = calloc(sz, 1);
  fseek(f, 0, SEEK_SET);
  size_t read = fread(content, 1, sz, f);
  DEBUG_PRINTF(L"read = %lu", read);
  fclose(f);
  if (read == sz)
  {
    // parse file content
    // GDisk.Tab.Caption=&GDisk
  }
  nb_free(content);*/
}

void TStringList::PutObject(intptr_t Index, TObject * AObject)
{
  if ((Index == NPOS) || (Index >= static_cast<intptr_t>(FList.size())))
  {
    Classes::Error(SListIndexError, Index);
  }
  Changing();
  TStringItem item;
  item.FString = FList[Index].FString;
  item.FObject = AObject;
  FList[Index] = item;
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

void TStringList::Insert(intptr_t Index, const UnicodeString & S)
{
  if ((Index == NPOS) || (Index > static_cast<intptr_t>(FList.size())))
  {
    Classes::Error(SListIndexError, Index);
  }
  TStringItem item;
  item.FString = S;
  item.FObject = NULL;
  FList.insert(FList.begin() + Index, item);
  Changed();
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
  intptr_t I, J, P;
  do
  {
    I = L;
    J = R;
    P = (L + R) >> 1;
    // DEBUG_PRINTF(L"L = %d, R = %d, P = %d", L, R, P);
    do
    {
      while (SCompare(this, I, P) < 0) { I++; }
      while (SCompare(this, J, P) > 0) { J--; }
      // DEBUG_PRINTF(L"I = %d, J = %d, P = %d", I, J, P);
      if (I <= J)
      {
        ExchangeItems(I, J);
        if (P == I)
        {
          P = J;
        }
        else if (P == J)
        {
          P = I;
        }
        I++;
        J--;
      }
    }
    while (I <= J);
    if (L < J) { QuickSort(L, J, SCompare); }
    L = I;
  }
  while (I < R);
}

void TStringList::ExchangeItems(intptr_t Index1, intptr_t Index2)
{
  TStringItem * Item1 = &FList[Index1];
  TStringItem * Item2 = &FList[Index2];
  UnicodeString Temp1 = Item1->FString;
  Item1->FString = Item2->FString;
  Item2->FString = Temp1;
  TObject * Temp2 = Item1->FObject;
  Item1->FObject = Item2->FObject;
  Item2->FObject = Temp2;
}

intptr_t TStringList::CompareStrings(const UnicodeString & S1, const UnicodeString & S2)
{
  if (GetCaseSensitive())
  {
    return static_cast<intptr_t>(::AnsiCompareStr(S1, S2));
  }
  else
  {
    return static_cast<intptr_t>(::AnsiCompareText(S1, S2));
  }
}

//---------------------------------------------------------------------------
/**
 * @brief Encoding multibyte to wide std::string
 * @param $src source char *
 * @param $cp code page
 * @return UnicodeString
 */
UnicodeString MB2W(const char * src, const UINT cp)
{
  // assert(src);
  if (!src || !*src)
  {
    return UnicodeString(L"");
  }

  intptr_t reqLength = MultiByteToWideChar(cp, 0, src, -1, NULL, 0);
  std::wstring wide(reqLength, 0);
  if (reqLength)
  {
    wide.resize(reqLength);
    MultiByteToWideChar(cp, 0, src, -1, &wide[0], static_cast<int>(reqLength));
    wide.resize(wide.size() - 1);  //remove NULL character
  }
  return UnicodeString(wide.c_str());
}

/**
 * @brief Encoding wide to multibyte std::string
 * @param $src UnicodeString
 * @param $cp code page
 * @return multibyte std::string
 */
std::string W2MB(const wchar_t * src, const UINT cp)
{
  // assert(src);
  if (!src || !*src)
  {
    return std::string("");
  }

  intptr_t reqLength = WideCharToMultiByte(cp, 0, src, -1, 0, 0, NULL, NULL);
  std::string mb(reqLength, 0);
  if (reqLength)
  {
    mb.resize(reqLength);
    WideCharToMultiByte(cp, 0, src, -1, &mb[0], static_cast<int>(reqLength), NULL, NULL);
    mb.resize(mb.length() - 1);  //remove NULL character
  }
  return mb.c_str();
}

//---------------------------------------------------------------------------

TDateTime::TDateTime(unsigned short Hour,
                     unsigned short Min, unsigned short Sec, unsigned short MSec)
{
  FValue = ::EncodeTimeVerbose(Hour, Min, Sec, MSec);
}

//---------------------------------------------------------------------------
UnicodeString TDateTime::DateString() const
{
  unsigned short Y, M, D;
  DecodeDate(Y, M, D);
  UnicodeString Result = FORMAT(L"%02d.%02d.%04d", D, M, Y);
  return Result;
}

//---------------------------------------------------------------------------
UnicodeString TDateTime::TimeString() const
{
  unsigned short H, N, S, MS;
  DecodeTime(H, N, S, MS);
  UnicodeString Result = FORMAT(L"%02d.%02d.%02d.%03d", H, N, S, MS);
  return Result;
}

//---------------------------------------------------------------------------
UnicodeString TDateTime::FormatString(wchar_t * fmt) const
{
  (void)fmt;
  unsigned short H, N, S, MS;
  DecodeTime(H, N, S, MS);
  UnicodeString Result = FORMAT(L"%02d.%02d.%02d.%03d", H, N, S, MS);
  return Result;
}

//---------------------------------------------------------------------------
void TDateTime::DecodeDate(unsigned short & Y,
                           unsigned short & M, unsigned short & D) const
{
  ::DecodeDate(*this, Y, M, D);
}

void TDateTime::DecodeTime(unsigned short & H,
                           unsigned short & N, unsigned short & S, unsigned short & MS) const
{
  ::DecodeTime(*this, H, N, S, MS);
}

//---------------------------------------------------------------------------
TDateTime Now()
{
  TDateTime Result(0.0);
  SYSTEMTIME SystemTime;
  ::GetLocalTime(&SystemTime);
  Result = EncodeDate(SystemTime.wYear, SystemTime.wMonth, SystemTime.wDay) +
    EncodeTime(SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond, SystemTime.wMilliseconds);
  return Result;
}

//---------------------------------------------------------------------------
TSHFileInfo::TSHFileInfo()
{
}

TSHFileInfo::~TSHFileInfo()
{
}

int TSHFileInfo::GetFileIconIndex(const UnicodeString & StrFileName, BOOL bSmallIcon) const
{
  SHFILEINFO sfi;

  if (bSmallIcon)
  {
    SHGetFileInfo(
      static_cast<LPCTSTR>(StrFileName.c_str()),
      FILE_ATTRIBUTE_NORMAL,
      &sfi,
      sizeof(SHFILEINFO),
      SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
  }
  else
  {
    SHGetFileInfo(
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
  SHFILEINFO sfi;
  if (bSmallIcon)
  {
    SHGetFileInfo(
      static_cast<LPCTSTR>(L"Doesn't matter"),
      FILE_ATTRIBUTE_DIRECTORY,
      &sfi,
      sizeof(SHFILEINFO),
      SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
  }
  else
  {
    SHGetFileInfo(
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

  SHGetFileInfo(
    reinterpret_cast<LPCTSTR>(StrFileName.c_str()),
    FILE_ATTRIBUTE_NORMAL,
    &sfi,
    sizeof(SHFILEINFO),
    SHGFI_TYPENAME | SHGFI_USEFILEATTRIBUTES);

  return sfi.szTypeName;
}

//---------------------------------------------------------------------------
class EStreamError : public ExtException
{
public:
  EStreamError(const UnicodeString & Msg) :
    ExtException((Exception * )NULL, Msg)
  {}
};

/*
class EWriteError : public ExtException
{
public:
    EWriteError(const UnicodeString & Msg) :
        ExtException(Msg)
    {}
};

class EReadError : public ExtException
{
public:
    EReadError(const UnicodeString & Msg) :
        ExtException(Msg)
    {}
};
*/
//---------------------------------------------------------------------------
TStream::TStream()
{
  Position(this);
  Size(this);
}

TStream::~TStream()
{
}

void TStream::ReadBuffer(void * Buffer, __int64 Count)
{
  if ((Count != 0) && (Read(Buffer, Count) != Count))
  {
    throw EReadError(Classes::W2MB(FMTLOAD(SReadError).c_str()).c_str());
  }
}

void TStream::WriteBuffer(const void * Buffer, __int64 Count)
{
  // DEBUG_PRINTF(L"Count = %d", Count);
  if ((Count != 0) && (Write(Buffer, Count) != Count))
  {
    throw EWriteError(Classes::W2MB(FMTLOAD(SWriteError).c_str()).c_str());
  }
}

//---------------------------------------------------------------------------
void ReadError(const UnicodeString & Name)
{
  throw std::exception("InvalidRegType"); // FIXME ERegistryException.CreateResFmt(@SInvalidRegType, [Name]);
}

//---------------------------------------------------------------------------
THandleStream::THandleStream(HANDLE AHandle) :
  FHandle(AHandle)
{
}

THandleStream::~THandleStream()
{
  // if (FHandle > 0)
  //   CloseHandle(FHandle);
}

__int64 THandleStream::Read(void * Buffer, __int64 Count)
{
  __int64 Result = ::FileRead(FHandle, Buffer, Count);
  // DEBUG_PRINTF(L"Result = %d, FHandle = %d, Count = %d", Result, FHandle, Count);
  if (Result == -1) { Result = 0; }
  return Result;
}

__int64 THandleStream::Write(const void * Buffer, __int64 Count)
{
  __int64 Result = ::FileWrite(FHandle, Buffer, Count);
  if (Result == -1) { Result = 0; }
  return Result;
}

__int64 THandleStream::Seek(__int64 Offset, int Origin)
{
  __int64 Result = ::FileSeek(FHandle, Offset, Origin);
  return Result;
}

__int64 THandleStream::Seek(const __int64 Offset, TSeekOrigin Origin)
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

void THandleStream::SetSize(const __int64 NewSize)
{
  Seek(NewSize, Classes::soFromBeginning);
  // LARGE_INTEGER li;
  // li.QuadPart = size;
  // if (SetFilePointer(fh.get(), li.LowPart, &li.HighPart, FILE_BEGIN) == -1)
  // handleLastErrorImpl(_path);
  ::Win32Check(::SetEndOfFile(FHandle) > 0);
}

//---------------------------------------------------------------------------
TMemoryStream::TMemoryStream() :
  FMemory(NULL),
  FSize(0),
  FPosition(0),
  FCapacity(0)
{
}

TMemoryStream::~TMemoryStream()
{
  Clear();
}

__int64 TMemoryStream::Read(void * Buffer, __int64 Count)
{
  __int64 Result = 0;
  if ((FPosition >= 0) && (Count >= 0))
  {
    Result = FSize - FPosition;
    if (Result > 0)
    {
      if (Result > Count) { Result = Count; }
      memmove(Buffer, reinterpret_cast<char *>(FMemory) + FPosition, static_cast<size_t>(Result));
      FPosition += Result;
      return Result;
    }
  }
  Result = 0;
  return Result;
}

__int64 TMemoryStream::Seek(__int64 Offset, int Origin)
{
  return Seek(Offset, static_cast<TSeekOrigin>(Origin));
}

__int64 TMemoryStream::Seek(const __int64 Offset, TSeekOrigin Origin)
{
  switch (Origin)
  {
    case Classes::soFromBeginning:
      FPosition = Offset;
      break;
    case Classes::soFromCurrent:
      FPosition += Offset;
      break;
    case Classes::soFromEnd:
      FPosition = FSize + Offset;
      break;
  }
  __int64 Result = FPosition;
  return Result;
}

void TMemoryStream::SaveToStream(TStream * Stream)
{
  if (FSize != 0) { Stream->WriteBuffer(FMemory, FSize); }
}

void TMemoryStream::SaveToFile(const UnicodeString & FileName)
{
  // TFileStream Stream(FileName, fmCreate);
  // SaveToStream(Stream);
  Classes::Error(SNotImplemented, 1203);
}

void TMemoryStream::Clear()
{
  SetCapacity(0);
  FSize = 0;
  FPosition = 0;
}

void TMemoryStream::SetSize(const __int64 NewSize)
{
  __int64 OldPosition = FPosition;
  SetCapacity(NewSize);
  FSize = NewSize;
  if (OldPosition > NewSize) { Seek(0, Classes::soFromEnd); }
}

void TMemoryStream::SetCapacity(__int64 NewCapacity)
{
  SetPointer(Realloc(NewCapacity), FSize);
  FCapacity = NewCapacity;
}

void * TMemoryStream::Realloc(__int64 & NewCapacity)
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
      Result = NULL;
    }
    else
    {
      if (FCapacity == 0)
      {
        Result = nb_malloc(static_cast<size_t>(NewCapacity));
      }
      else
      {
        Result = nb_realloc(FMemory, static_cast<size_t>(NewCapacity));
      }
      if (Result == NULL)
      {
        throw EStreamError(FMTLOAD(SMemoryStreamError));
      }
    }
  }
  return Result;
}

void TMemoryStream::SetPointer(void * Ptr, __int64 Size)
{
  FMemory = Ptr;
  FSize = Size;
}

__int64 TMemoryStream::Write(const void * Buffer, __int64 Count)
{
  __int64 Result = 0;
  if ((FPosition >= 0) && (Count >= 0))
  {
    __int64 Pos = FPosition + Count;
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
              Buffer, static_cast<size_t>(Count));
      FPosition = Pos;
      Result = Count;
    }
  }
  return Result;
}

//---------------------------------------------------------------------------

bool IsRelative(const UnicodeString & Value)
{
  return  !(!Value.IsEmpty() && (Value[1] == L'\\'));
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
  DWORD Result = 0;
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
    case rdBinary:
      Result = REG_BINARY;
      break;
    default:
      Result = REG_NONE;
      break;
  }
  return Result;
}

//---------------------------------------------------------------------------
class ERegistryException : public std::exception
{
};

//---------------------------------------------------------------------------
TRegistry::TRegistry() :
  FCurrentKey(0),
  FRootKey(0),
  FCloseRootKey(false),
  FAccess(KEY_ALL_ACCESS)
{
  // LazyWrite = True;
  Access(this);
  CurrentKey(this);
  RootKey(this);

  SetRootKey(HKEY_CURRENT_USER);
  SetAccess(KEY_ALL_ACCESS);
}

TRegistry::~TRegistry()
{
  CloseKey();
}

void TRegistry::SetAccess(int access)
{
  FAccess = access;
}

void TRegistry::SetRootKey(HKEY ARootKey)
{
  if (FRootKey != ARootKey)
  {
    if (FCloseRootKey)
    {
      RegCloseKey(GetRootKey());
      FCloseRootKey = false;
    }
    FRootKey = ARootKey;
    CloseKey();
  }
}

void TRegistry::GetValueNames(TStrings * Strings) const
{
  Strings->Clear();
  TRegKeyInfo Info;
  UnicodeString S;
  if (GetKeyInfo(Info))
  {
    S.SetLength(Info.MaxValueLen + 1);
    for (DWORD I = 0; I < Info.NumValues; I++)
    {
      DWORD Len = Info.MaxValueLen + 1;
      RegEnumValue(GetCurrentKey(), I, &S[1], &Len, NULL, NULL, NULL, NULL);
      Strings->Add(S.c_str());
    }
  }
}

void TRegistry::GetKeyNames(TStrings * Strings) const
{
  Strings->Clear();
  TRegKeyInfo Info;
  UnicodeString S;
  if (GetKeyInfo(Info))
  {
    S.SetLength(static_cast<intptr_t>(Info.MaxSubKeyLen) + 1);
    for (unsigned int I = 0; I < Info.NumSubKeys; I++)
    {
      DWORD Len = Info.MaxSubKeyLen + 1;
      RegEnumKeyEx(GetCurrentKey(), static_cast<DWORD>(I), &S[1], &Len, NULL, NULL, NULL, NULL);
      Strings->Add(S.c_str());
    }
  }
}

HKEY TRegistry::GetCurrentKey() const { return FCurrentKey; }
HKEY TRegistry::GetRootKey() const { return FRootKey; }

void TRegistry::CloseKey()
{
  if (GetCurrentKey() != 0)
  {
    // if LazyWrite then
    RegCloseKey(GetCurrentKey()); //else RegFlushKey(CurrentKey);
    FCurrentKey = 0;
    FCurrentPath = L"";
  }
}

bool TRegistry::OpenKey(const UnicodeString & Key, bool CanCreate)
{
  // DEBUG_PRINTF(L"key = %s, CanCreate = %d", Key.c_str(), CanCreate);
  bool Result = false;
  UnicodeString S = Key;
  bool Relative = Classes::IsRelative(S);

  // if (!Relative) S.erase(0, 1); // Delete(S, 1, 1);
  HKEY TempKey = 0;
  if (!CanCreate || S.IsEmpty())
  {
    // DEBUG_PRINTF(L"RegOpenKeyEx");
    Result = RegOpenKeyEx(GetBaseKey(Relative), S.c_str(), 0,
                          FAccess, &TempKey) == ERROR_SUCCESS;
  }
  else
  {
    // int Disposition = 0;
    // DEBUG_PRINTF(L"RegCreateKeyEx: Relative = %d", Relative);
    Result = RegCreateKeyEx(GetBaseKey(Relative), S.c_str(), 0, NULL,
                            REG_OPTION_NON_VOLATILE, FAccess, NULL, &TempKey, NULL) == ERROR_SUCCESS;
  }
  if (Result)
  {
    if ((GetCurrentKey() != 0) && Relative)
    {
      S = FCurrentPath + L'\\' + S;
    }
    ChangeKey(TempKey, S);
  }
  // DEBUG_PRINTF(L"CurrentKey = %d, Result = %d", GetCurrentKey(), Result);
  return Result;
}

bool TRegistry::DeleteKey(const UnicodeString & Key)
{
  bool Result = false;
  UnicodeString S = Key;
  bool Relative = Classes::IsRelative(S);
  // if not Relative then Delete(S, 1, 1);
  HKEY OldKey = GetCurrentKey();
  HKEY DeleteKey = GetKey(Key);
  if (DeleteKey != 0)
  {
    TRY_FINALLY (
    {
      SetCurrentKey(DeleteKey);
      TRegKeyInfo Info;
      if (GetKeyInfo(Info))
      {
        UnicodeString KeyName;
        KeyName.SetLength(Info.MaxSubKeyLen + 1);
        for (intptr_t I = Info.NumSubKeys - 1; I >= 0; I--)
        {
          DWORD Len = Info.MaxSubKeyLen + 1;
          if (RegEnumKeyEx(DeleteKey, static_cast<DWORD>(I), &KeyName[1], &Len,
                           NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
          {
            this->DeleteKey(KeyName);
          }
        }
      }
    }
    ,
    {
      SetCurrentKey(OldKey);
      RegCloseKey(DeleteKey);
    }
    );
  }
  Result = RegDeleteKey(GetBaseKey(Relative), S.c_str()) == ERROR_SUCCESS;
  return Result;
}

bool TRegistry::DeleteValue(const UnicodeString & Name) const
{
  bool Result = RegDeleteValue(GetCurrentKey(), Name.c_str()) == ERROR_SUCCESS;
  return Result;
}

bool TRegistry::KeyExists(const UnicodeString & Key)
{
  bool Result = false;
  // DEBUG_PRINTF(L"Key = %s", Key.c_str());
  unsigned OldAccess = FAccess;
  TRY_FINALLY (
  {
    FAccess = STANDARD_RIGHTS_READ | KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS;
    HKEY TempKey = GetKey(Key);
    if (TempKey != 0) { RegCloseKey(TempKey); }
    Result = TempKey != 0;
  }
  ,
  {
    FAccess = OldAccess;
  }
  );
  // DEBUG_PRINTF(L"Result = %d", Result);
  return Result;
}

bool TRegistry::ValueExists(const UnicodeString & Name) const
{
  TRegDataInfo Info;
  bool Result = GetDataInfo(Name, Info);
  // DEBUG_PRINTF(L"Result = %d", Result);
  return Result;
}

bool TRegistry::GetDataInfo(const UnicodeString & ValueName, TRegDataInfo & Value) const
{
  DWORD DataType;
  memset(&Value, 0, sizeof(Value));
  bool Result = (RegQueryValueEx(GetCurrentKey(), ValueName.c_str(), NULL, &DataType, NULL,
                                 &Value.DataSize) == ERROR_SUCCESS);
  // DEBUG_PRINTF(L"Result = %d", Result);
  Value.RegData = DataTypeToRegData(DataType);
  return Result;
}

TRegDataType TRegistry::GetDataType(const UnicodeString & ValueName) const
{
  TRegDataType Result;
  TRegDataInfo Info;
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

int TRegistry::GetDataSize(const UnicodeString & ValueName) const
{
  int Result = 0;
  TRegDataInfo Info;
  if (GetDataInfo(ValueName, Info))
  {
    Result = Info.DataSize;
  }
  else
  {
    Result = -1;
  }
  return Result;
}

bool TRegistry::ReadBool(const UnicodeString & Name)
{
  bool Result = ReadInteger(Name) != 0;
  return Result;
}

TDateTime TRegistry::ReadDateTime(const UnicodeString & Name)
{
  TDateTime Result = TDateTime(ReadFloat(Name));
  return Result;
}

double TRegistry::ReadFloat(const UnicodeString & Name) const
{
  double Result = 0.0;
  TRegDataType RegData = rdUnknown;
  int Len = GetData(Name, &Result, sizeof(double), RegData);
  if ((RegData != rdBinary) || (Len != sizeof(double)))
  {
    Classes::ReadError(Name);
  }
  return Result;
}

intptr_t TRegistry::ReadInteger(const UnicodeString & Name) const
{
  DWORD Result = 0;
  TRegDataType RegData = rdUnknown;
  // DEBUG_PRINTF(L"Name = %s", Name.c_str());
  GetData(Name, &Result, sizeof(Result), RegData);
  // DEBUG_PRINTF(L"Result = %d, RegData = %d, rdInteger = %d", Result, RegData, rdInteger);
  if (RegData != rdInteger)
  {
    Classes::ReadError(Name);
  }
  return Result;
}

__int64 TRegistry::ReadInt64(const UnicodeString & Name)
{
  __int64 Result = 0;
  ReadBinaryData(Name, &Result, sizeof(Result));
  return Result;
}

UnicodeString TRegistry::ReadString(const UnicodeString & Name)
{
  UnicodeString Result = L"";
  TRegDataType RegData = rdUnknown;
  intptr_t Len = GetDataSize(Name);
  if (Len > 0)
  {
    Result.SetLength(Len);
    GetData(Name, static_cast<void *>(const_cast<wchar_t *>(Result.c_str())), Len, RegData);
    if ((RegData == rdString) || (RegData == rdExpandString))
    {
      PackStr(Result);
    }
    else { ReadError(Name); }
  }
  else
  {
    Result = L"";
  }
  return Result;
}

UnicodeString TRegistry::ReadStringRaw(const UnicodeString & Name)
{
  UnicodeString Result = ReadString(Name);
  return Result;
}

size_t TRegistry::ReadBinaryData(const UnicodeString & Name,
  void * Buffer, size_t BufSize) const
{
  size_t Result = 0;
  TRegDataInfo Info;
  if (GetDataInfo(Name, Info))
  {
    Result = static_cast<size_t>(Info.DataSize);
    TRegDataType RegData = Info.RegData;
    if (((RegData == rdBinary) || (RegData == rdUnknown)) && (Result <= BufSize))
    {
      GetData(Name, Buffer, Result, RegData);
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

int TRegistry::GetData(const UnicodeString & Name, void * Buffer,
  intptr_t BufSize, TRegDataType & RegData) const
{
  DWORD DataType = REG_NONE;
  DWORD bufSize = static_cast<DWORD>(BufSize);
  // DEBUG_PRINTF(L"GetCurrentKey = %d", GetCurrentKey());
  if (RegQueryValueEx(GetCurrentKey(), Name.c_str(), NULL, &DataType,
    reinterpret_cast<BYTE *>(Buffer), &bufSize) != ERROR_SUCCESS)
  {
    throw std::exception("RegQueryValueEx failed"); // FIXME ERegistryException.CreateResFmt(@SRegGetDataFailed, [Name]);
  }
  RegData = DataTypeToRegData(DataType);
  int Result = static_cast<int>(BufSize);
  return Result;
}

void TRegistry::PutData(const UnicodeString & Name, const void * Buffer,
  intptr_t BufSize, TRegDataType RegData)
{
  int DataType = Classes::RegDataToDataType(RegData);
  // DEBUG_PRINTF(L"GetCurrentKey = %d, Name = %s, REG_DWORD = %d, DataType = %d, BufSize = %d", GetCurrentKey(), Name.c_str(), REG_DWORD, DataType, BufSize);
  if (RegSetValueEx(GetCurrentKey(), Name.c_str(), 0, DataType,
                    reinterpret_cast<const BYTE *>(Buffer), static_cast<DWORD>(BufSize)) != ERROR_SUCCESS)
  {
    throw std::exception("RegSetValueEx failed");    // ERegistryException(); // FIXME .CreateResFmt(SRegSetDataFailed, Name.c_str());
  }
}

void TRegistry::WriteBool(const UnicodeString & Name, bool Value)
{
  WriteInteger(Name, Value);
}

void TRegistry::WriteDateTime(const UnicodeString & Name, TDateTime & Value)
{
  double Val = Value.operator double();
  PutData(Name, &Val, sizeof(double), rdBinary);
}

void TRegistry::WriteFloat(const UnicodeString & Name, double Value)
{
  PutData(Name, &Value, sizeof(double), rdBinary);
}

void TRegistry::WriteString(const UnicodeString & Name, const UnicodeString & Value)
{
  // DEBUG_PRINTF(L"Value = %s, Value.size = %d", Value.c_str(), Value.size());
  PutData(Name, static_cast<const void *>(Value.c_str()), Value.Length() * sizeof(wchar_t) + 1, rdString);
}

void TRegistry::WriteStringRaw(const UnicodeString & Name, const UnicodeString & Value)
{
  PutData(Name, Value.c_str(), Value.Length() * sizeof(wchar_t) + 1, rdString);
}

void TRegistry::WriteInteger(const UnicodeString & Name, intptr_t Value)
{
  DWORD Val = static_cast<DWORD>(Value);
  PutData(Name, &Val, sizeof(Val), rdInteger);
  // WriteInt64(Name, Value);
}

void TRegistry::WriteInt64(const UnicodeString & Name, __int64 Value)
{
  WriteBinaryData(Name, &Value, sizeof(Value));
}

void TRegistry::WriteBinaryData(const UnicodeString & Name,
                                const void * Buffer, size_t BufSize)
{
  PutData(Name, Buffer, BufSize, rdBinary);
}

void TRegistry::ChangeKey(HKEY Value, const UnicodeString & Path)
{
  CloseKey();
  FCurrentKey = Value;
  FCurrentPath = Path;
}

HKEY TRegistry::GetBaseKey(bool Relative)
{
  HKEY Result = 0;
  if ((FCurrentKey == 0) || !Relative)
  {
    Result = GetRootKey();
  }
  else
  {
    Result = FCurrentKey;
  }
  return Result;
}

HKEY TRegistry::GetKey(const UnicodeString & Key)
{
  UnicodeString S = Key;
  bool Relative = Classes::IsRelative(S);
  // if not Relative then Delete(S, 1, 1);
  HKEY Result = 0;
  RegOpenKeyEx(GetBaseKey(Relative), S.c_str(), 0, FAccess, &Result);
  return Result;
}

bool TRegistry::GetKeyInfo(TRegKeyInfo & Value) const
{
  memset(&Value, 0, sizeof(Value));
  bool Result = RegQueryInfoKey(GetCurrentKey(), NULL, NULL, NULL, &Value.NumSubKeys,
    &Value.MaxSubKeyLen, NULL, &Value.NumValues, &Value.MaxValueLen,
    &Value.MaxDataLen, NULL, &Value.FileTime) == ERROR_SUCCESS;
  return Result;
}

//---------------------------------------------------------------------------
TShortCut::TShortCut() : FValue(0)
{
}

TShortCut::TShortCut(intptr_t Value)
{
  FValue = Value;
}

TShortCut::operator intptr_t() const
{
  return FValue;
}

bool TShortCut::operator < (const TShortCut & rhs) const
{
  return FValue < rhs.FValue;
}

//---------------------------------------------------------------------------

void GetLocaleFormatSettings(int LCID, TFormatSettings & FormatSettings)
{
  (void)LCID;
  (void)FormatSettings;
  Classes::Error(SNotImplemented, 1204);
}

//---------------------------------------------------------------------------

} // namespace Classes
