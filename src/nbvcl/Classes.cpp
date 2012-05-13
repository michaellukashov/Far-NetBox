#pragma once

#include "stdafx.h"

#include "boostdefines.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/scope_exit.hpp>

#include "Classes.h"
#include "Common.h"
#include "Exceptions.h"
#include "Sysutils.h"

namespace alg = boost::algorithm;

namespace Classes {

int __cdecl debug_printf(const wchar_t * format, ...)
{
  (void)format;
  int len = 0;
#ifdef NETBOX_DEBUG
  va_list args;
  va_start(args, format);
  len = _vscwprintf(format, args);
  std::wstring buf(len + 1, 0);
  vswprintf((wchar_t *)buf.c_str(), buf.size(), format, args);
  va_end(args);
  OutputDebugStringW(buf.c_str());
#endif
  return len;
}

int __cdecl debug_printf2(const char * format, ...)
{
  (void)format;
  int len = 0;
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
  throw Sysutils::EAbort("");
}
//---------------------------------------------------------------------------
void Error(int ErrorID, int data)
{
  DEBUG_PRINTF(L"begin: ErrorID = %d, data = %d", ErrorID, data);
  UnicodeString Msg = FMTLOAD(ErrorID, data);
  // DEBUG_PRINTF(L"Msg = %s", Msg.c_str());
  throw ExtException(Msg);
}

//---------------------------------------------------------------------------
TPersistent::TPersistent()
{}

TPersistent::~TPersistent()
{}

void __fastcall TPersistent::Assign(TPersistent * Source)
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

void __fastcall TPersistent::AssignTo(TPersistent * Dest)
{
  Dest->AssignError(this);
}

TPersistent * __fastcall TPersistent::GetOwner()
{
  return NULL;
}

void __fastcall TPersistent::AssignError(TPersistent * Source)
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
}
TList::~TList()
{
  Clear();
}
int TList::GetCount() const
{
  return FList.size();
}
void TList::SetCount(int NewCount)
{
  if (NewCount == NPOS)
  {
    Classes::Error(SListCountError, NewCount);
  }
  if (NewCount <= FList.size())
  {
    int sz = FList.size();
    for (int I = sz - 1; (I != NPOS) && (I >= NewCount); I--)
    {
      Delete(I);
    }
  }
  FList.resize(NewCount);
}

void * TList::operator [](int Index) const
{
  return FList[Index];
}
void * TList::GetItem(int Index) const
{
  return FList[Index];
}
void TList::SetItem(int Index, void * Item)
{
  if ((Index == NPOS) || (Index >= FList.size()))
  {
    Classes::Error(SListIndexError, Index);
  }
  FList.insert(FList.begin() + Index, Item);
}

int TList::Add(void * value)
{
  int Result = FList.size();
  FList.push_back(value);
  return Result;
}
void * TList::Extract(void * item)
{
  if (Remove(item) != NPOS)
  {
    return item;
  }
  else
  {
    return NULL;
  }
}
int TList::Remove(void * item)
{
  int Result = IndexOf(item);
  if (Result != NPOS)
  {
    Delete(Result);
  }
  return Result;
}
void TList::Move(int CurIndex, int NewIndex)
{
  if (CurIndex != NewIndex)
  {
    if ((NewIndex == NPOS) || (NewIndex >= FList.size()))
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
void TList::Delete(int Index)
{
  if ((Index == NPOS) || (Index >= FList.size()))
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
void TList::Insert(int Index, void * Item)
{
  if ((Index == NPOS) || (Index > FList.size()))
  {
    Classes::Error(SListIndexError, Index);
  }
  if (Index <= FList.size())
  {
    FList.insert(FList.begin() + Index, Item);
  }
  if (Item != NULL)
  {
    Notify(Item, lnAdded);
  }
}
int TList::IndexOf(void * value) const
{
  int Result = 0;
  while ((Result < FList.size()) && (FList[Result] != value))
  {
    Result++;
  }
  if (Result == FList.size())
  {
    Result = NPOS;
  }
  return Result;
}
void TList::Clear()
{
  SetCount(0);
}

void TList::Sort(CompareFunc func)
{
  Classes::Error(SNotImplemented, 1);
}
void TList::Notify(void * Ptr, int Action)
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
}
TObjectList::~TObjectList()
{
  Clear();
}

TObject * TObjectList::operator [](int Index) const
{
  return static_cast<TObject *>(parent::operator[](Index));
}
TObject * TObjectList::GetItem(int Index) const
{
  return static_cast<TObject *>(parent::GetItem(Index));
}
void TObjectList::SetItem(int Index, TObject * Value)
{
  parent::SetItem(Index, Value);
}

int TObjectList::Add(TObject * value)
{
  return parent::Add(value);
}
int TObjectList::Remove(TObject * value)
{
  return parent::Remove(value);
}
void TObjectList::Extract(TObject * value)
{
  parent::Extract(value);
}
void TObjectList::Move(int Index, int To)
{
  parent::Move(Index, To);
}
void TObjectList::Delete(int Index)
{
  parent::Delete(Index);
}
void TObjectList::Insert(int Index, TObject * value)
{
  parent::Insert(Index, value);
}
int TObjectList::IndexOf(TObject * value) const
{
  return parent::IndexOf(value);
}
void TObjectList::Clear()
{
  parent::Clear();
}
bool TObjectList::GetOwnsObjects()
{
  return FOwnsObjects;
}

void TObjectList::SetOwnsObjects(bool value)
{
  FOwnsObjects = value;
}

void TObjectList::Sort(CompareFunc func)
{
  parent::Sort(func);
}
void TObjectList::Notify(void * Ptr, int Action)
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

void TStrings::SetTextStr(const UnicodeString Text)
{
  TStrings * Self = this;
  Self->BeginUpdate();
  BOOST_SCOPE_EXIT( (&Self) )
  {
    Self->EndUpdate();
  } BOOST_SCOPE_EXIT_END
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
    };
  }
}

UnicodeString TStrings::GetCommaText()
{
  wchar_t LOldDelimiter = GetDelimiter();
  wchar_t LOldQuoteChar = GetQuoteChar();
  FDelimiter = L',';
  FQuoteChar = L'"';
  TStrings * Self = this;
  BOOST_SCOPE_EXIT( (&Self) (&LOldDelimiter) (&LOldQuoteChar) )
  {
    Self->FDelimiter = LOldDelimiter;
    Self->FQuoteChar = LOldQuoteChar;
  } BOOST_SCOPE_EXIT_END

  UnicodeString Result = GetDelimitedText();
  return Result;
}
UnicodeString TStrings::GetDelimitedText() const
{
  UnicodeString Result;
  int Count = GetCount();
  if ((Count == 1) && GetStrings(0).IsEmpty())
  {
    Result = GetQuoteChar() + GetQuoteChar();
  }
  else
  {
    for (int i = 0; i < GetCount(); i++)
    {
      UnicodeString line = GetStrings(i);
      Result += GetQuoteChar() + line + GetQuoteChar() + GetDelimiter();
    }
    if (Result.Length() > 0)
      Result.SetLength(Result.Length() - 1);
  }
  return Result;
}
void TStrings::SetDelimitedText(const UnicodeString Value)
{
  TStrings * Self = this;
  Self->BeginUpdate();
  BOOST_SCOPE_EXIT( (&Self) )
  {
    Self->EndUpdate();
  } BOOST_SCOPE_EXIT_END
  Clear();
  std::vector<std::wstring> lines;
  std::wstring delim = std::wstring(1, GetDelimiter());
  delim.append(1, L'\n');
  alg::split(lines, std::wstring(Value.c_str()), alg::is_any_of(delim), alg::token_compress_on);
  UnicodeString line;
  BOOST_FOREACH(line, lines)
  {
    Add(line);
  }
}

int TStrings::CompareStrings(const UnicodeString S1, const UnicodeString S2)
{
  return ::AnsiCompareText(S1, S2);
}

void TStrings::Assign(TPersistent * Source)
{
  if (::InheritsFrom<TPersistent, TStrings>(Source))
  {
    BeginUpdate();
    {
      TStrings * Self = this;
      BOOST_SCOPE_EXIT ( (&Self) )
      {
        Self->EndUpdate();
      } BOOST_SCOPE_EXIT_END
      Clear();
      // FDefined = TStrings(Source).FDefined;
      FQuoteChar = static_cast<TStrings *>(Source)->FQuoteChar;
      FDelimiter = static_cast<TStrings *>(Source)->FDelimiter;
      AddStrings(static_cast<TStrings *>(Source));
    }
    return;
  }
  TPersistent::Assign(Source);
}

int TStrings::Add(const UnicodeString S)
{
  int Result = GetCount();
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
  int I, L, Size, Count;
  wchar_t * P = NULL;
  UnicodeString S, LB;

  Count = GetCount();
  // DEBUG_PRINTF(L"Count = %d", Count);
  Size = 0;
  LB = sLineBreak;
  for (I = 0; I < Count; I++)
  {
    Size += GetStrings(I).Length() + LB.Length();
  }
  Result.SetLength(Size);
  P = const_cast<wchar_t *>(Result.c_str());
  for (I = 0; I < Count; I++)
  {
    S = GetStrings(I);
    // DEBUG_PRINTF(L"  S = %s", S.c_str());
    L = S.Length() * sizeof(wchar_t);
    if (L != 0)
    {
      memmove(P, S.c_str(), L);
      P += S.Length();
    };
    L = LB.Length() * sizeof(wchar_t);
    if (L != 0)
    {
      memmove(P, LB.c_str(), L);
      P += LB.Length();
    };
  }
  return Result;
}

void TStrings::SetText(const UnicodeString Text)
{
  SetTextStr(Text);
}

void TStrings::SetCommaText(const UnicodeString Value)
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

TObject * TStrings::GetObjects(int Index)
{
  (void)Index;
  return NULL;
}

int TStrings::AddObject(const UnicodeString S, TObject * AObject)
{
  int Result = Add(S);
  PutObject(Result, AObject);
  return Result;
}

void TStrings::InsertObject(int Index, const UnicodeString Key, TObject * AObject)
{
  Insert(Index, Key);
  PutObject(Index, AObject);
}

bool TStrings::Equals(TStrings * Strings)
{
  bool Result = false;
  int Count = GetCount();
  if (Count != Strings->GetCount())
  {
    return false;
  }
  for (int I = 0; I < Count; I++)
  {
    if (GetStrings(I) != Strings->GetStrings(I))
    {
      return false;
    }
  }
  Result = true;
  return Result;
}

void TStrings::PutObject(int Index, TObject * AObject)
{
  (void)Index;
  (void)AObject;
}

void TStrings::PutString(int Index, const UnicodeString S)
{
  TObject * TempObject = GetObjects(Index);
  Delete(Index);
  InsertObject(Index, S, TempObject);
}

void TStrings::SetDuplicates(TDuplicatesEnum value)
{
  FDuplicates = value;
}

void TStrings::Move(int CurIndex, int NewIndex)
{
  if (CurIndex != NewIndex)
  {
    BeginUpdate();
    {
      TStrings * Self = this;
      BOOST_SCOPE_EXIT ( (&Self) )
      {
        Self->EndUpdate();
      } BOOST_SCOPE_EXIT_END
      UnicodeString TempString = GetStrings(CurIndex);
      TObject * TempObject = GetObjects(CurIndex);
      Delete(CurIndex);
      InsertObject(NewIndex, TempString, TempObject);
    }
  }
}

int TStrings::IndexOf(const UnicodeString S)
{
  // DEBUG_PRINTF(L"begin");
  for (int Result = 0; Result < GetCount(); Result++)
  {
    if (CompareStrings(GetStrings(Result), S) == 0)
    {
      return Result;
    }
  }
  // DEBUG_PRINTF(L"end");
  return NPOS;
}
int TStrings::IndexOfName(const UnicodeString Name)
{
  for (int Index = 0; Index < GetCount(); Index++)
  {
    UnicodeString S = GetStrings(Index);
    int P = ::AnsiPos(S, L'=');
    if ((P > 0) && (CompareStrings(S.SubStr(1, P), Name) == 0))
    {
      return Index;
    }
  }
  return NPOS;
}

const UnicodeString TStrings::GetName(int Index)
{
  return ExtractName(GetStrings(Index));
}

UnicodeString TStrings::ExtractName(const UnicodeString S)
{
  UnicodeString Result = S;
  int P = ::AnsiPos(Result, L'=');
  if (P > 0)
  {
    Result.SetLength(P);
  }
  else
  {
    Result.SetLength(0);
  }
  return Result;
}

const UnicodeString TStrings::GetValue(const UnicodeString Name)
{
  UnicodeString Result;
  int I = IndexOfName(Name);
  if (I >= 0)
  {
    Result = GetStrings(I).SubStr(Name.Length() + 1, static_cast<int>(-1));
  }
  return Result;
}

void TStrings::SetValue(const UnicodeString Name, const UnicodeString Value)
{
  int I = IndexOfName(Name);
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
    TStrings * Self = this;
    BOOST_SCOPE_EXIT ( (&Self) )
    {
      Self->EndUpdate();
    } BOOST_SCOPE_EXIT_END
    for (int I = 0; I < Strings->GetCount(); I++)
    {
      AddObject(Strings->GetStrings(I), Strings->GetObjects(I));
    }
  }
}

void TStrings::Append(const UnicodeString value)
{
  Insert(GetCount(), value);
}

void TStrings::SaveToStream(TStream * Stream)
{
  Classes::Error(SNotImplemented, 12);
}

//---------------------------------------------------------------------------
int StringListCompareStrings(TStringList * List, int Index1, int Index2)
{
  int Result = List->CompareStrings(List->FList[Index1].FString,
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

int TStringList::GetCount() const
{
  return FList.size();
}

void TStringList::Clear()
{
  FList.clear();
  // SetCount(0);
  // SetCapacity(0);
}

int TStringList::Add(const UnicodeString S)
{
  return AddObject(S, NULL);
}

int TStringList::AddObject(const UnicodeString S, TObject * AObject)
{
  // DEBUG_PRINTF(L"S = %s, Duplicates = %d", S.c_str(), FDuplicates);
  int Result = 0;
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

bool TStringList::Find(const UnicodeString S, int & Index)
{
  bool Result = false;
  int L = 0;
  int H = GetCount() - 1;
  while ((H != NPOS) && (L <= H))
  {
    int I = (L + H) >> 1;
    int C = CompareStrings(FList[I].FString, S);
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
int TStringList::IndexOf(const UnicodeString S)
{
  // DEBUG_PRINTF(L"begin");
  int Result = NPOS;
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
void TStringList::PutString(int Index, const UnicodeString S)
{
  if (GetSorted())
  {
    Classes::Error(SSortedListError, 0);
  }
  if ((Index == NPOS) || (Index > FList.size()))
  {
    Classes::Error(SListIndexError, Index);
  }
  Changing();
  // DEBUG_PRINTF(L"Index = %d, size = %d", Index, FList.size());
  if (Index < FList.size())
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
void TStringList::Delete(int Index)
{
  if ((Index == NPOS) || (Index >= FList.size()))
  {
    Classes::Error(SListIndexError, Index);
  }
  Changing();
  DEBUG_PRINTF(L"Index = %d, Count = %d, Value = %s", Index, GetCount(), FList[Index].FString.c_str());
  FList.erase(FList.begin() + Index);
  Changed();
}
TObject * TStringList::GetObjects(int Index)
{
  if ((Index == NPOS) || (Index >= FList.size()))
  {
    Classes::Error(SListIndexError, Index);
  }
  return FList[Index].FObject;
}
void TStringList::InsertObject(int Index, const UnicodeString Key, TObject * AObject)
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
void TStringList::InsertItem(int Index, const UnicodeString S, TObject * AObject)
{
  if ((Index == NPOS) || (Index > GetCount()))
  {
    Classes::Error(SListIndexError, Index);
  }
  Changing();
  // if (FCount == FCapacity) Grow();
  TStringItem item;
  item.FString = S;
  item.FObject = AObject;
  FList.insert(FList.begin() + Index, item);
  Changed();
}
UnicodeString TStringList::GetStrings(int Index) const
{
  // DEBUG_PRINTF(L"Index = %d, FList.size = %d", Index, FList.size());
  if ((Index == NPOS) || (Index >= FList.size()))
  {
    Classes::Error(SListIndexError, Index);
  }
  UnicodeString Result = FList[Index].FString;
  return Result;
}
bool TStringList::GetCaseSensitive() const
{
  return FCaseSensitive;
}
void TStringList::SetCaseSensitive(bool value)
{
  if (value != FCaseSensitive)
  {
    FCaseSensitive = value;
    if (GetSorted())
    {
      Sort();
    }
  }
}
bool TStringList::GetSorted() const
{
  return FSorted;
}
void TStringList::SetSorted(bool value)
{
  if (value != FSorted)
  {
    if (value)
    {
      Sort();
    }
    FSorted = value;
  }
}
void TStringList::LoadFromFile(const UnicodeString FileName)
{
  Classes::Error(SNotImplemented, 14);
}

void TStringList::PutObject(int Index, TObject * AObject)
{
  if ((Index == NPOS) || (Index >= FList.size()))
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
  if (GetUpdateCount() == 0)
  {
    FOnChanging(this);
  }
}
void TStringList::Changed()
{
  if (GetUpdateCount() == 0)
  {
    FOnChange(this);
  }
}
void TStringList::Insert(int Index, const UnicodeString S)
{
  if ((Index == NPOS) || (Index > FList.size()))
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
void TStringList::CustomSort(TStringListSortCompare CompareFunc)
{
  if (!GetSorted() && (GetCount() > 1))
  {
    Changing();
    QuickSort(0, GetCount() - 1, CompareFunc);
    Changed();
  }
}
void TStringList::QuickSort(int L, int R, TStringListSortCompare SCompare)
{
  int I, J, P;
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

void TStringList::ExchangeItems(int Index1, int Index2)
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

int TStringList::CompareStrings(const UnicodeString S1, const UnicodeString S2)
{
  if (GetCaseSensitive())
  {
    return ::AnsiCompareStr(S1, S2);
  }
  else
  {
    return ::AnsiCompareText(S1, S2);
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

  std::wstring wide;
  const int reqLength = MultiByteToWideChar(cp, 0, src, -1, NULL, 0);
  if (reqLength)
  {
    wide.resize(static_cast<int>(reqLength));
    MultiByteToWideChar(cp, 0, src, -1, &wide[0], reqLength);
    wide.resize(wide.size() - 1);  //remove NULL character
  }
  return wide;
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

  std::string mb;
  const int reqLength = WideCharToMultiByte(cp, 0, src, -1, 0, 0, NULL, NULL);
  if (reqLength)
  {
    mb.resize(static_cast<int>(reqLength));
    WideCharToMultiByte(cp, 0, src, -1, &mb[0], reqLength, NULL, NULL);
    mb.erase(mb.length() - 1);  //remove NULL character
  }
  return mb;
}

//---------------------------------------------------------------------------

TDateTime::TDateTime(unsigned short Hour,
                     unsigned short Min, unsigned short Sec, unsigned short MSec)
{
  FValue = ::EncodeTimeVerbose(Hour, Min, Sec, MSec);
}
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
  TDateTime result(0.0);
  SYSTEMTIME SystemTime;
  ::GetLocalTime(&SystemTime);
  result = EncodeDate(SystemTime.wYear, SystemTime.wMonth, SystemTime.wDay) +
           EncodeTime(SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond, SystemTime.wMilliseconds);
  return result;
}
//---------------------------------------------------------------------------
TSHFileInfo::TSHFileInfo()
{
}

TSHFileInfo::~TSHFileInfo()
{
}

int TSHFileInfo::GetFileIconIndex(UnicodeString strFileName, BOOL bSmallIcon)
{
  SHFILEINFO sfi;

  if (bSmallIcon)
  {
    SHGetFileInfo(
      static_cast<LPCTSTR>(strFileName.c_str()),
      FILE_ATTRIBUTE_NORMAL,
      &sfi,
      sizeof(SHFILEINFO),
      SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
  }
  else
  {
    SHGetFileInfo(
      static_cast<LPCTSTR>(strFileName.c_str()),
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

UnicodeString TSHFileInfo::GetFileType(const UnicodeString strFileName)
{
  SHFILEINFO sfi;

  SHGetFileInfo(
    reinterpret_cast<LPCTSTR>(strFileName.c_str()),
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
  EStreamError(const UnicodeString Msg) :
    ExtException(Msg)
  {}
};

/*
class EWriteError : public ExtException
{
public:
    EWriteError(const UnicodeString Msg) :
        ExtException(Msg)
    {}
};

class EReadError : public ExtException
{
public:
    EReadError(const UnicodeString Msg) :
        ExtException(Msg)
    {}
};
*/
//---------------------------------------------------------------------------

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
void ReadError(const UnicodeString Name)
{
  throw std::exception("InvalidRegType"); // FIXME ERegistryException.CreateResFmt(@SInvalidRegType, [Name]);
}

//---------------------------------------------------------------------------
TStream::TStream()
{
}

TStream::~TStream()
{
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

__int64 __fastcall THandleStream::Read(void * Buffer, __int64 Count)
{
  __int64 Result = ::FileRead(FHandle, Buffer, Count);
  // DEBUG_PRINTF(L"Result = %d, FHandle = %d, Count = %d", Result, FHandle, Count);
  if (Result == -1) { Result = 0; }
  return Result;
}

__int64 __fastcall THandleStream::Write(const void * Buffer, __int64 Count)
{
  __int64 Result = ::FileWrite(FHandle, Buffer, Count);
  if (Result == -1) { Result = 0; }
  return Result;
}
__int64 __fastcall THandleStream::Seek(__int64 Offset, __int64 Origin)
{
  __int64 Result = ::FileSeek(FHandle, Offset, Origin);
  return Result;
}
__int64 __fastcall THandleStream::Seek(const __int64 Offset, TSeekOrigin Origin)
{
  Classes::Error(SNotImplemented, 1202);
  return 0;
}

void __fastcall THandleStream::SetSize(const __int64 NewSize)
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

__int64 __fastcall TMemoryStream::Read(void * Buffer, __int64 Count)
{
  __int64 Result = 0;
  if ((FPosition >= 0) && (Count >= 0))
  {
    Result = FSize - FPosition;
    if (Result > 0)
    {
      if (Result > Count) { Result = Count; }
      memmove(Buffer, reinterpret_cast<char *>(FMemory) + FPosition, static_cast<int>(Result));
      FPosition += Result;
      return Result;
    }
  }
  Result = 0;
  return Result;
}

__int64 __fastcall TMemoryStream::Seek(__int64 Offset, __int64 Origin)
{
  // return Seek(Offset, Classes::soFromCurrent);
  Classes::Error(SNotImplemented, 1303);
  return 0;
}

__int64 __fastcall TMemoryStream::Seek(const __int64 Offset, TSeekOrigin Origin)
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

void __fastcall TMemoryStream::SaveToFile(const UnicodeString FileName)
{
  // TFileStream Stream(FileName, fmCreate);
  // SaveToStream(Stream);
  Classes::Error(SNotImplemented, 1203);
}

void __fastcall TMemoryStream::Clear()
{
  SetCapacity(0);
  FSize = 0;
  FPosition = 0;
}

void __fastcall TMemoryStream::SetSize(const __int64 NewSize)
{
  __int64 OldPosition = FPosition;
  SetCapacity(NewSize);
  FSize = NewSize;
  if (OldPosition > NewSize) { Seek(0, Classes::soFromEnd); }
}

void __fastcall TMemoryStream::SetCapacity(__int64 NewCapacity)
{
  SetPointer(Realloc(NewCapacity), FSize);
  FCapacity = NewCapacity;
}

void * __fastcall TMemoryStream::Realloc(__int64 & NewCapacity)
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
      free(FMemory);
      Result = NULL;
    }
    else
    {
      if (FCapacity == 0)
      {
        Result = malloc(static_cast<int>(NewCapacity));
      }
      else
      {
        Result = realloc(FMemory, static_cast<int>(NewCapacity));
      }
      if (Result == NULL)
      {
        throw EStreamError(FMTLOAD(SMemoryStreamError));
      }
    }
  }
  return Result;
}

void __fastcall TMemoryStream::SetPointer(void * Ptr, __int64 Size)
{
  FMemory = Ptr;
  FSize = Size;
}

__int64 __fastcall TMemoryStream::Write(const void * Buffer, __int64 Count)
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
              Buffer, static_cast<int>(Count));
      FPosition = Pos;
      Result = Count;
    }
  }
  return Result;
}

//---------------------------------------------------------------------------

bool IsRelative(const UnicodeString Value)
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
  SetRootKey(HKEY_CURRENT_USER);
  SetAccess(KEY_ALL_ACCESS);
  // LazyWrite = True;
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
void TRegistry::GetValueNames(TStrings * Strings)
{
  Strings->Clear();
  TRegKeyInfo Info;
  UnicodeString S;
  if (GetKeyInfo(Info))
  {
    S.SetLength(Info.MaxValueLen + 1);
    for (int I = 0; I < Info.NumSubKeys; I++)
    {
      DWORD Len = Info.MaxValueLen + 1;
      RegEnumValue(GetCurrentKey(), static_cast<DWORD>(I), &S[1], &Len, NULL, NULL, NULL, NULL);
      Strings->Add(S.c_str());
    }
  }
}

void TRegistry::GetKeyNames(TStrings * Strings)
{
  Strings->Clear();
  TRegKeyInfo Info;
  UnicodeString S;
  if (GetKeyInfo(Info))
  {
    S.SetLength(Info.MaxSubKeyLen + 1);
    for (int I = 0; I < Info.NumSubKeys; I++)
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

bool TRegistry::OpenKey(const UnicodeString Key, bool CanCreate)
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

bool TRegistry::DeleteKey(const UnicodeString Key)
{
  bool Result = false;
  UnicodeString S = Key;
  bool Relative = Classes::IsRelative(S);
  // if not Relative then Delete(S, 1, 1);
  HKEY OldKey = GetCurrentKey();
  HKEY DeleteKey = GetKey(Key);
  if (DeleteKey != 0)
  {
    TRegistry * Self = this;
    BOOST_SCOPE_EXIT( (&Self) (&OldKey) (&DeleteKey) )
    {
      Self->SetCurrentKey(OldKey);
      RegCloseKey(DeleteKey);
    } BOOST_SCOPE_EXIT_END
    SetCurrentKey(DeleteKey);
    TRegKeyInfo Info;
    if (GetKeyInfo(Info))
    {
      UnicodeString KeyName;
      KeyName.SetLength(Info.MaxSubKeyLen + 1);
      for (int I = Info.NumSubKeys - 1; I >= 0; I--)
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
  Result = RegDeleteKey(GetBaseKey(Relative), S.c_str()) == ERROR_SUCCESS;
  return Result;
}

bool TRegistry::DeleteValue(const UnicodeString Name)
{
  bool Result = RegDeleteValue(GetCurrentKey(), Name.c_str()) == ERROR_SUCCESS;
  return Result;
}

bool TRegistry::KeyExists(const UnicodeString Key)
{
  bool Result = false;
  // DEBUG_PRINTF(L"Key = %s", Key.c_str());
  unsigned OldAccess = FAccess;
  {
    TRegistry * Self = this;
    BOOST_SCOPE_EXIT( (&Self) (&OldAccess) )
    {
      Self->FAccess = OldAccess;
    } BOOST_SCOPE_EXIT_END

    FAccess = STANDARD_RIGHTS_READ | KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS;
    HKEY TempKey = GetKey(Key);
    if (TempKey != 0) { RegCloseKey(TempKey); }
    Result = TempKey != 0;
  }
  // DEBUG_PRINTF(L"Result = %d", Result);
  return Result;
}

bool TRegistry::ValueExists(const UnicodeString Name)
{
  TRegDataInfo Info;
  bool Result = GetDataInfo(Name, Info);
  // DEBUG_PRINTF(L"Result = %d", Result);
  return Result;
}

bool TRegistry::GetDataInfo(const UnicodeString ValueName, TRegDataInfo & Value)
{
  DWORD DataType;
  memset(&Value, 0, sizeof(Value));
  bool Result = (RegQueryValueEx(GetCurrentKey(), ValueName.c_str(), NULL, &DataType, NULL,
                                 &Value.DataSize) == ERROR_SUCCESS);
  // DEBUG_PRINTF(L"Result = %d", Result);
  Value.RegData = DataTypeToRegData(DataType);
  return Result;
}

TRegDataType TRegistry::GetDataType(const UnicodeString ValueName)
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

int TRegistry::GetDataSize(const UnicodeString ValueName)
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

bool TRegistry::ReadBool(const UnicodeString Name)
{
  bool Result = ReadInteger(Name) != 0;
  return Result;
}

TDateTime TRegistry::ReadDateTime(const UnicodeString Name)
{
  TDateTime Result = TDateTime(ReadFloat(Name));
  return Result;
}

double TRegistry::ReadFloat(const UnicodeString Name)
{
  double Result = 0.0;
  TRegDataType RegData;
  int Len = GetData(Name, &Result, sizeof(double), RegData);
  if ((RegData != rdBinary) || (Len != sizeof(double)))
  {
    Classes::ReadError(Name);
  }
  return Result;
}

int TRegistry::ReadInteger(const UnicodeString Name)
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

__int64 TRegistry::ReadInt64(const UnicodeString Name)
{
  __int64 Result = 0;
  ReadBinaryData(Name, &Result, sizeof(Result));
  return Result;
}

UnicodeString TRegistry::ReadString(const UnicodeString Name)
{
  UnicodeString Result = L"";
  TRegDataType RegData = rdUnknown;
  int Len = GetDataSize(Name);
  if (Len > 0)
  {
    Result.SetLength(Len);
    GetData(Name, static_cast<void *>(const_cast<wchar_t *>(Result.c_str())), static_cast<DWORD>(Len), RegData);
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

UnicodeString TRegistry::ReadStringRaw(const UnicodeString Name)
{
  UnicodeString Result = ReadString(Name);
  return Result;
}

int TRegistry::ReadBinaryData(const UnicodeString Name,
                              void * Buffer, int BufSize)
{
  int Result = 0;
  TRegDataInfo Info;
  if (GetDataInfo(Name, Info))
  {
    Result = Info.DataSize;
    TRegDataType RegData = Info.RegData;
    if (((RegData == rdBinary) || (RegData == rdUnknown)) && (Result <= BufSize))
    {
      GetData(Name, Buffer, static_cast<DWORD>(Result), RegData);
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

int TRegistry::GetData(const UnicodeString Name, void * Buffer,
                       DWORD BufSize, TRegDataType & RegData)
{
  DWORD DataType = REG_NONE;
  // DEBUG_PRINTF(L"GetCurrentKey = %d", GetCurrentKey());
  if (RegQueryValueEx(GetCurrentKey(), Name.c_str(), NULL, &DataType,
    reinterpret_cast<BYTE *>(Buffer), &BufSize) != ERROR_SUCCESS)
  {
    throw std::exception("RegQueryValueEx failed"); // FIXME ERegistryException.CreateResFmt(@SRegGetDataFailed, [Name]);
  }
  RegData = DataTypeToRegData(DataType);
  int Result = BufSize;
  return Result;
}

void TRegistry::PutData(const UnicodeString Name, const void * Buffer,
  int BufSize, TRegDataType RegData)
{
  int DataType = Classes::RegDataToDataType(RegData);
  // DEBUG_PRINTF(L"GetCurrentKey = %d, Name = %s, REG_DWORD = %d, DataType = %d, BufSize = %d", GetCurrentKey(), Name.c_str(), REG_DWORD, DataType, BufSize);
  if (RegSetValueEx(GetCurrentKey(), Name.c_str(), 0, DataType,
                    reinterpret_cast<const BYTE *>(Buffer), static_cast<DWORD>(BufSize)) != ERROR_SUCCESS)
  {
    throw std::exception("RegSetValueEx failed");    // ERegistryException(); // FIXME .CreateResFmt(SRegSetDataFailed, Name.c_str());
  }
}

void TRegistry::WriteBool(const UnicodeString Name, bool Value)
{
  WriteInteger(Name, Value);
}
void TRegistry::WriteDateTime(const UnicodeString Name, TDateTime & Value)
{
  double Val = Value.operator double();
  PutData(Name, &Val, sizeof(double), rdBinary);
}
void TRegistry::WriteFloat(const UnicodeString Name, double Value)
{
  PutData(Name, &Value, sizeof(double), rdBinary);
}
void TRegistry::WriteString(const UnicodeString Name, const UnicodeString Value)
{
  // DEBUG_PRINTF(L"Value = %s, Value.size = %d", Value.c_str(), Value.size());
  PutData(Name, static_cast<const void *>(Value.c_str()), Value.Length() * sizeof(wchar_t) + 1, rdString);
}
void TRegistry::WriteStringRaw(const UnicodeString Name, const UnicodeString Value)
{
  PutData(Name, Value.c_str(), Value.Length() * sizeof(wchar_t) + 1, rdString);
}
void TRegistry::WriteInteger(const UnicodeString Name, int Value)
{
  DWORD Val = Value;
  PutData(Name, &Val, sizeof(DWORD), rdInteger);
  // WriteInt64(Name, Value);
}

void TRegistry::WriteInt64(const UnicodeString Name, __int64 Value)
{
  WriteBinaryData(Name, &Value, sizeof(Value));
}
void TRegistry::WriteBinaryData(const UnicodeString Name,
                                const void * Buffer, int BufSize)
{
  PutData(Name, Buffer, BufSize, rdBinary);
}

void TRegistry::ChangeKey(HKEY Value, const UnicodeString Path)
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

HKEY TRegistry::GetKey(const UnicodeString Key)
{
  UnicodeString S = Key;
  bool Relative = Classes::IsRelative(S);
  // if not Relative then Delete(S, 1, 1);
  HKEY Result = 0;
  RegOpenKeyEx(GetBaseKey(Relative), S.c_str(), 0, FAccess, &Result);
  return Result;
}

bool TRegistry::GetKeyInfo(TRegKeyInfo & Value)
{
  memset(&Value, 0, sizeof(Value));
  bool Result = RegQueryInfoKey(GetCurrentKey(), NULL, NULL, NULL, &Value.NumSubKeys,
    &Value.MaxSubKeyLen, NULL, &Value.NumValues, &Value.MaxValueLen,
    &Value.MaxDataLen, NULL, &Value.FileTime) == ERROR_SUCCESS;
  return Result;
}

//---------------------------------------------------------------------------
TShortCut::TShortCut()
{
}

TShortCut::TShortCut(int value)
{
}
TShortCut::operator int() const
{
  return 0;
}
bool TShortCut::operator < (const TShortCut & rhs) const
{
  return false;
}

//---------------------------------------------------------------------------

void __fastcall GetLocaleFormatSettings(int LCID, TFormatSettings & FormatSettings)
{
  Classes::Error(SNotImplemented, 1204);
}

//---------------------------------------------------------------------------

} // namespace Classes
