#pragma once

#include "stdafx.h"
#include <ShellAPI.h>

#include "boostdefines.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/scope_exit.hpp>

#include "Classes.h"
#include "Common.h"
#include "Exceptions.h"

namespace alg = boost::algorithm;

//---------------------------------------------------------------------------
TPersistent::TPersistent()
{}

TPersistent::~TPersistent()
{}

void TPersistent::Assign(TPersistent *Source)
{
    if (Source != NULL)
        Source->AssignTo(this);
    else
        AssignError(NULL);
}

void TPersistent::AssignTo(TPersistent *Dest)
{
    Dest->AssignError(this);
}

TPersistent *TPersistent::GetOwner()
{
    return NULL;
}

void TPersistent::AssignError(TPersistent *Source)
{
  (void)Source;
  std::wstring SourceName = L"nil";
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
size_t TList::GetCount() const
{
    return FList.size();
}
void TList::SetCount(size_t value)
{
    FList.resize(value);
}

void *TList::operator [](size_t Index) const
{
    return FList[Index];
}
void *TList::GetItem(size_t Index) const
{
    return FList[Index];
}
void TList::SetItem(size_t Index, void *Item)
{
    if ((Index < 0) || (Index > FList.size()))
    {
      ::Error(SListIndexError, Index);
    }
    FList.insert(FList.begin() + Index, Item);
}

size_t TList::Add(void *value)
{
    size_t Result = FList.size();
    FList.push_back(value);
    return Result;
}
void *TList::Extract(void *item)
{
    if (Remove(item) >= 0)
        return item;
    else
        return NULL;
}
int TList::Remove(void *item)
{
    int Result = IndexOf(item);
    if (Result >= 0)
    {
        Delete(Result);
    }
    return Result;
}
void TList::Move(size_t CurIndex, size_t NewIndex)
{
  if (CurIndex != NewIndex)
  {
    if ((NewIndex < 0) || (NewIndex >= FList.size()))
    {
      ::Error(SListIndexError, NewIndex);
    }
    void *Item = GetItem(CurIndex);
    FList[CurIndex] = NULL;
    Delete(CurIndex);
    Insert(NewIndex, NULL);
    FList[NewIndex] = Item;
  }
}
void TList::Delete(size_t Index)
{
    if ((Index < 0) || (Index >= FList.size()))
    {
      ::Error(SListIndexError, Index);
    }
    FList.erase(FList.begin() + Index);
}
void TList::Insert(size_t Index, void *Item)
{
    if ((Index < 0) || (Index > FList.size()))
    {
      ::Error(SListIndexError, Index);
    }
    // if (FCount == FCapacity)
      // Grow();
    if (Index <= FList.size())
    {
        FList.insert(FList.begin() + Index, Item);
    }
    if (Item != NULL)
      Notify(Item, lnAdded);
}
int TList::IndexOf(void *value) const
{
    size_t Result = 0;
    while ((Result < FList.size()) && (FList[Result] != value))
      Result++;
    if (Result == FList.size())
      Result = (size_t)-1;
    return (int)Result;
}
void TList::Clear()
{
    FList.clear();
}

void TList::Sort(CompareFunc func)
{
    ::Error(SNotImplemented, 1);
}
void TList::Notify(void *Ptr, int Action)
{
    (void)Ptr;
    (void)Action;
}
void TList::Sort()
{
  // if (FList.size() > 1)
    // QuickSort(FList, 0, GetCount() - 1, Compare);
  ::Error(SNotImplemented, 15);
}
//---------------------------------------------------------------------------
TObjectList::TObjectList() :
    FOwnsObjects(false)
{
}
TObjectList::~TObjectList()
{
}

TObject *TObjectList::operator [](size_t Index) const
{
    return (TObject *)parent::operator[](Index);
}
TObject *TObjectList::GetItem(size_t Index) const
{
    return (TObject *)parent::GetItem(Index);
}
void TObjectList::SetItem(size_t Index, TObject *Value)
{
    parent::SetItem(Index, Value);
}

size_t TObjectList::Add(TObject *value)
{
    return parent::Add(value);
}
int TObjectList::Remove(TObject *value)
{
    return parent::Remove(value);
}
void TObjectList::Extract(TObject *value)
{
    parent::Extract(value);
}
void TObjectList::Move(size_t Index, size_t To)
{
    parent::Move(Index, To);
}
void TObjectList::Delete(size_t Index)
{
    parent::Delete(Index);
}
void TObjectList::Insert(size_t Index, TObject *value)
{
    parent::Insert(Index, value);
}
int TObjectList::IndexOf(TObject *value) const
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
void TObjectList::Notify(void *Ptr, int Action)
{
  if (GetOwnsObjects())
  {
    if (Action == lnDeleted)
    {
      ((TObject *)Ptr)->Free();
    }
  }
    parent::Notify(Ptr, Action);
}
//---------------------------------------------------------------------------
const std::wstring sLineBreak = L"\n";
static const int MemoryDelta = 0x2000;
//---------------------------------------------------------------------------

void TStrings::SetTextStr(const std::wstring Text)
{
    TStrings *Self = this;
    Self->BeginUpdate();
    BOOST_SCOPE_EXIT( (&Self) )
    {
        Self->EndUpdate();
    } BOOST_SCOPE_EXIT_END
    Clear();
    const wchar_t *P = Text.c_str();
    if (P != NULL)
    {
      while (*P != 0x00)
      {
        const wchar_t *Start = P;
        while (!((*P == 0x00) || (*P == 0x0A) || (*P == 0x0D)))
        {
            P++;
        }
        std::wstring S;
        S.resize(P - Start);
        memcpy((wchar_t *)S.c_str(), Start, (P - Start) * sizeof(wchar_t));
        Add(S);
        if (*P == 0x0D) P++;
        if (*P == 0x0A) P++;
      };
    }
}

std::wstring TStrings::GetCommaText()
{
    wchar_t LOldDelimiter = GetDelimiter();
    wchar_t LOldQuoteChar = GetQuoteChar();
    FDelimiter = L',';
    FQuoteChar = L'"';
    TStrings *Self = this;
    BOOST_SCOPE_EXIT( (&Self) (LOldDelimiter) (LOldQuoteChar) )
    {
        Self->FDelimiter = LOldDelimiter;
        Self->FQuoteChar = LOldQuoteChar;
    } BOOST_SCOPE_EXIT_END

    std::wstring Result = GetDelimitedText();
    return Result;
}
std::wstring TStrings::GetDelimitedText() const
{
    std::wstring Result;
    size_t Count = GetCount();
    if ((Count == 1) && GetString(0).empty())
    {
      Result = GetQuoteChar() + GetQuoteChar();
    }
    else
    {
        for (size_t i = 0; i < GetCount(); i++)
        {
            std::wstring line = GetString(i);
            Result += GetQuoteChar() + line + GetQuoteChar() + GetDelimiter();
        }
        Result.resize(Result.size() - 1);
    }
    return Result;
}
void TStrings::SetDelimitedText(const std::wstring Value)
{
    TStrings *Self = this;
    Self->BeginUpdate();
    BOOST_SCOPE_EXIT( (&Self) )
    {
        Self->EndUpdate();
    } BOOST_SCOPE_EXIT_END
    Clear();
    std::vector<std::wstring> lines;
    std::wstring delim = std::wstring(1, GetDelimiter());
    delim.append(1, L'\n');
    alg::split(lines, Value, alg::is_any_of(delim), alg::token_compress_on);
    std::wstring line;
    // for (std::vector<std::wstring>::const_iterator it = lines.begin(); it != lines.end(); ++it)
    BOOST_FOREACH(line, lines)
    {
        Add(line);
    }
}

int TStrings::CompareStrings(const std::wstring &S1, const std::wstring &S2)
{
    return ::AnsiCompareText(S1, S2);
}

void TStrings::Assign(TPersistent *Source)
{
  if (::InheritsFrom<TPersistent, TStrings>(Source))
  {
    BeginUpdate();
    {
        TStrings *Self = this;
        BOOST_SCOPE_EXIT ( (&Self) )
        {
            Self->EndUpdate();
        } BOOST_SCOPE_EXIT_END
      Clear();
      // FDefined = TStrings(Source).FDefined;
      FQuoteChar = ((TStrings *)Source)->FQuoteChar;
      FDelimiter = ((TStrings *)Source)->FDelimiter;
      AddStrings((TStrings *)(Source));
    }
    return;
  }
  TPersistent::Assign(Source);
}

size_t TStrings::Add(std::wstring S)
{
    int Result = GetCount();
    Insert(Result, S);
    return Result;
}
std::wstring TStrings::GetText()
{
    return GetTextStr();
}
std::wstring TStrings::GetTextStr()
{
    std::wstring Result;
    int I, L, Size, Count;
    wchar_t *P;
    std::wstring S, LB;

    Count = GetCount();
    // DEBUG_PRINTF(L"Count = %d", Count);
    Size = 0;
    LB = sLineBreak;
    for (I = 0; I < Count; I++)
    {
        Size += GetString(I).size() + LB.size();
    }
    Result.resize(Size);
    P = (wchar_t *)Result.c_str();
    for (I = 0; I < Count; I++)
    {
      S = GetString(I);
      // DEBUG_PRINTF(L"  S = %s", S.c_str());
      L = S.size() * sizeof(wchar_t);
      if (L != 0)
      {
        memcpy(P, S.c_str(), L);
        P += S.size();
      };
      L = LB.size() * sizeof(wchar_t);
      if (L != 0)
      {
        memcpy(P, LB.c_str(), L);
        P += LB.size();
      };
    }
    return Result;
}
void TStrings::SetText(const std::wstring Text)
{
    SetTextStr(Text);
}
void TStrings::SetCommaText(std::wstring Value)
{
    SetDelimiter(L',');
    SetQuoteChar(L'"');
    SetDelimitedText(Value);
}
void TStrings::BeginUpdate()
{
    if (FUpdateCount == 0) 
      SetUpdateState(true);
    FUpdateCount++;
}
void TStrings::EndUpdate()
{
    FUpdateCount--;
    if (FUpdateCount == 0)
        SetUpdateState(false);
}
void TStrings::SetUpdateState(bool Updating)
{
    (void)Updating;
}
TObject *TStrings::GetObject(int Index)
{
    (void)Index;
    return NULL;
}
int TStrings::AddObject(std::wstring S, TObject *AObject)
{
    int Result = Add(S);
    PutObject(Result, AObject);
    return Result;
}
void TStrings::InsertObject(int Index, std::wstring Key, TObject *AObject)
{
    Insert(Index, Key);
    PutObject(Index, AObject);
}

bool TStrings::Equals(TStrings *Strings)
{
  bool Result = false;
  size_t Count = GetCount();
  if (Count != Strings->GetCount())
    return false;
  for (size_t I = 0; I < Count; I++)
  {
    if (GetString(I) != Strings->GetString(I))
        return false;
  }
  Result = true;
  return Result;
}
void TStrings::PutObject(int Index, TObject *AObject)
{
    (void)Index;
    (void)AObject;
}
void TStrings::PutString(int Index, std::wstring S)
{
    TObject *TempObject = GetObject(Index);
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
        TStrings *Self = this;
        BOOST_SCOPE_EXIT ( (&Self) )
        {
            Self->EndUpdate();
        } BOOST_SCOPE_EXIT_END
      std::wstring TempString = GetString(CurIndex);
      TObject *TempObject = GetObject(CurIndex);
      Delete(CurIndex);
      InsertObject(NewIndex, TempString, TempObject);
    }
  }
}
int TStrings::IndexOf(const std::wstring S)
{
  // DEBUG_PRINTF(L"begin");
  for (size_t Result = 0; Result < GetCount(); Result++)
  {
    if (CompareStrings(GetString(Result), S) == 0)
    {
        return Result;
    }
  }
  // DEBUG_PRINTF(L"end");
  return -1;
}
int TStrings::IndexOfName(const std::wstring &Name)
{
  size_t Result = (size_t)-1;
  for (Result = 0; Result < GetCount(); Result++)
  {
    std::wstring S = GetString(Result);
    size_t P = ::AnsiPos(S, L'=');
    if ((P != std::wstring::npos) && (CompareStrings(S.substr(0, P), Name) == 0))
    {
        return Result;
    }
  }
  Result = (size_t)-1;
  return (int)Result;
}
const std::wstring TStrings::GetName(int Index)
{
    return ExtractName(GetString(Index));
}
std::wstring TStrings::ExtractName(const std::wstring &S)
{
   std::wstring Result = S;
   size_t P = ::AnsiPos(Result, L'=');
   if (P != std::wstring::npos)
    Result.resize(P);
   else
    Result.resize(0);
   return Result;
}
const std::wstring TStrings::GetValue(const std::wstring Name)
{
  std::wstring Result;
  size_t I = IndexOfName(Name);
  if (I != std::wstring::npos)
  {
    Result = GetString(I).substr(Name.size() + 1, -1);
  }
  return Result;
}
void TStrings::SetValue(const std::wstring Name, const std::wstring Value)
{
  size_t I = IndexOfName(Name);
  if (!Value.empty())
  {
    if (I == std::wstring::npos)
        I = Add(L"");
    PutString(I, Name + L'=' + Value);
  }
  else
  {
    if (I != std::wstring::npos)
        Delete(I);
  }
}
void TStrings::AddStrings(TStrings *Strings)
{
  BeginUpdate();
  {
    TStrings *Self = this;
    BOOST_SCOPE_EXIT ( (&Self) )
    {
        Self->EndUpdate();
    } BOOST_SCOPE_EXIT_END
    for (size_t I = 0; I < Strings->GetCount(); I++)
    {
      AddObject(Strings->GetString(I), Strings->GetObject(I));
    }
  }
}
void TStrings::Append(const std::wstring &value)
{
    Insert(GetCount(), value);
}
void TStrings::SaveToStream(TStream *Stream)
{
    ::Error(SNotImplemented, 12);
}

//---------------------------------------------------------------------------
int StringListCompareStrings(TStringList *List, int Index1, int Index2)
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
void TStringList::Assign(TPersistent *Source)
{
    // ::Error(SNotImplemented, 13);
    parent::Assign(Source);
}
size_t TStringList::GetCount() const
{
    return FList.size();
}
void TStringList::Clear()
{
    FList.clear();
      // SetCount(0);
      // SetCapacity(0);
}
size_t TStringList::Add(std::wstring S)
{
    return AddObject(S, NULL);
}
int TStringList::AddObject(std::wstring S, TObject *AObject)
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
            ::Error(SDuplicateString, 2);
            break;
      }
    }
  }
  InsertItem(Result, S, AObject);
  return Result;
}
bool TStringList::Find(const std::wstring S, int &Index)
{
  bool Result = false;
  int L = 0;
  int H = GetCount() - 1;
  while (L <= H)
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
int TStringList::IndexOf(const std::wstring S)
{
  // DEBUG_PRINTF(L"begin");
  int Result = -1;
  if (!GetSorted())
  {
    Result = parent::IndexOf(S);
  }
  else
  {
    if (!Find(S, Result))
    {
      Result = -1;
    }
  }
  // DEBUG_PRINTF(L"end");
  return Result;
}
void TStringList::PutString(int Index, std::wstring S)
{
    if (GetSorted())
    {
      ::Error(SSortedListError, 0);
    }
    if ((Index < 0) || ((size_t)Index > FList.size()))
    {
      ::Error(SListIndexError, Index);
    }
    Changing();
    // DEBUG_PRINTF(L"Index = %d, size = %d", Index, FList.size());
    if ((size_t)Index < FList.size())
    {
      TStringItem item;
      item.FString = S;
      item.FObject = NULL;
      FList[Index] = item;
    }
    else
    {
        Insert(Index, S);
    }
    Changed();
}
void TStringList::Delete(size_t Index)
{
  if ((Index < 0) || (Index >= FList.size()))
  {
    ::Error(SListIndexError, Index);
  }
  Changing();
  FList.erase(FList.begin() + Index);
  Changed();
}
TObject *TStringList::GetObject(int Index)
{
    if ((Index < 0) || ((size_t)Index >= FList.size()))
    {
        ::Error(SListIndexError, Index);
    }
    return FList[Index].FObject;
}
void TStringList::InsertObject(int Index, std::wstring Key, TObject *AObject)
{
    if (GetSorted())
    {
        ::Error(SSortedListError, 0);
    }
    if ((Index < 0) || ((size_t)Index > GetCount()))
    {
        ::Error(SListIndexError, Index);
    }
    InsertItem(Index, Key, AObject);
}
void TStringList::InsertItem(int Index, const std::wstring S, TObject *AObject)
{
    if ((Index < 0) || ((size_t)Index > GetCount()))
    {
        ::Error(SListIndexError, Index);
    }
    Changing();
    // if (FCount == FCapacity) Grow();
    TStringItem item;
    item.FString = S;
    item.FObject = AObject;
    FList.insert(FList.begin() + Index, item);
    Changed();
}
std::wstring TStringList::GetString(size_t Index) const
{
    // DEBUG_PRINTF(L"Index = %d, FList.size = %d", Index, FList.size());
    if ((Index < 0) || (Index >= FList.size()))
    {
        ::Error(SListIndexError, Index);
    }
    std::wstring Result = FList[Index].FString;
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
void TStringList::LoadFromFile(const std::wstring &FileName)
{
    ::Error(SNotImplemented, 14);
}

void TStringList::PutObject(int Index, TObject *AObject)
{
    if ((Index < 0) || ((size_t)Index >= FList.size()))
    {
      ::Error(SListIndexError, Index);
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
        Changing();
    else
        Changed();
}
void TStringList::Changing()
{
  if (GetUpdateCount() == 0)
    FOnChanging(this);
}
void TStringList::Changed()
{
    if (GetUpdateCount() == 0)
        FOnChange(this);
}
void TStringList::Insert(int Index, const std::wstring S)
{
  if ((Index < 0) || ((size_t)Index > FList.size()))
  {
    ::Error(SListIndexError, Index);
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
      while (SCompare(this, I, P) < 0) I++;
      while (SCompare(this, J, P) > 0) J--;
      // DEBUG_PRINTF(L"I = %d, J = %d, P = %d", I, J, P);
      if (I <= J)
      {
        ExchangeItems(I, J);
        if (P == I)
          P = J;
        else if (P == J)
          P = I;
        I++;
        J--;
      }
    } while (I <= J);
    if (L < J) QuickSort(L, J, SCompare);
    L = I;
  } while (I < R);
}

void TStringList::ExchangeItems(int Index1, int Index2)
{
  TStringItem *Item1 = &FList[Index1];
  TStringItem *Item2 = &FList[Index2];
  std::wstring Temp1 = Item1->FString;
  Item1->FString = Item2->FString;
  Item2->FString = Temp1;
  TObject *Temp2 = Item1->FObject;
  Item1->FObject = Item2->FObject;
  Item2->FObject = Temp2;
}

int TStringList::CompareStrings(const std::wstring &S1, const std::wstring &S2)
{
  if (GetCaseSensitive())
    return ::AnsiCompareStr(S1, S2);
  else
    return ::AnsiCompareText(S1, S2);
}

//---------------------------------------------------------------------------
/**
 * Encoding multibyte to wide std::string
 * \param src source std::string
 * \param cp code page
 * \return wide std::string
 */
std::wstring MB2W(const char *src, const UINT cp)
{
    // assert(src);
    if (!src)
        return L"";

    std::wstring wide;
    const int reqLength = MultiByteToWideChar(cp, 0, src, -1, NULL, 0);
    if (reqLength)
    {
        wide.resize(static_cast<size_t>(reqLength));
        MultiByteToWideChar(cp, 0, src, -1, &wide[0], reqLength);
        wide.erase(wide.length() - 1);  //remove NULL character
    }
    return wide;
}

/**
 * Encoding wide to multibyte std::string
 * \param src source std::string
 * \param cp code page
 * \return multibyte std::string
 */
std::string W2MB(const wchar_t *src, const UINT cp)
{
    assert(src);

    std::string mb;
    const int reqLength = WideCharToMultiByte(cp, 0, src, -1, 0, 0, NULL, NULL);
    if (reqLength)
    {
        mb.resize(static_cast<size_t>(reqLength));
        WideCharToMultiByte(cp, 0, src, -1, &mb[0], reqLength, NULL, NULL);
        mb.erase(mb.length() - 1);  //remove NULL character
    }
    return mb;
}

//---------------------------------------------------------------------------

TDateTime::TDateTime(unsigned int Hour,
    unsigned int Min, unsigned int Sec, unsigned int MSec)
{
    FValue = ::EncodeTimeVerbose(Hour, Min, Sec, MSec);
}
void TDateTime::DecodeDate(unsigned int &Y,
        unsigned int &M, unsigned int &D)
{
    ::DecodeDate(*this, Y, M, D);
}
void TDateTime::DecodeTime(unsigned int &H,
        unsigned int &N, unsigned int &S, unsigned int &MS)
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


HIMAGELIST  TSHFileInfo::GetSystemImageListHandle( BOOL bSmallIcon )
{
    HIMAGELIST  hSystemImageList; 
    SHFILEINFO    ssfi; 

    if (bSmallIcon)
    {
        hSystemImageList = (HIMAGELIST)SHGetFileInfo( 
            (LPCTSTR)std::wstring(L"c:\\").c_str(), 
             0, 
             &ssfi, 
             sizeof(SHFILEINFO), 
             SHGFI_SYSICONINDEX | SHGFI_SMALLICON); 
    }
    else
    {
        hSystemImageList = (HIMAGELIST)SHGetFileInfo( 
            (LPCTSTR)std::wstring(L"c:\\").c_str(), 
             0, 
             &ssfi, 
             sizeof(SHFILEINFO), 
             SHGFI_SYSICONINDEX | SHGFI_LARGEICON); 
    }
    return hSystemImageList;
}


int TSHFileInfo::GetFileIconIndex( std::wstring strFileName , BOOL bSmallIcon )
{
    SHFILEINFO    sfi;

    if (bSmallIcon)
    {
        SHGetFileInfo(
           (LPCTSTR)strFileName.c_str(), 
           FILE_ATTRIBUTE_NORMAL,
           &sfi, 
           sizeof(SHFILEINFO), 
           SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
    }
    else
    {
        SHGetFileInfo(
           (LPCTSTR)strFileName.c_str(), 
           FILE_ATTRIBUTE_NORMAL,
           &sfi, 
           sizeof(SHFILEINFO), 
           SHGFI_SYSICONINDEX | SHGFI_LARGEICON | SHGFI_USEFILEATTRIBUTES);
    }

    return sfi.iIcon;

}

int TSHFileInfo::GetDirIconIndex(BOOL bSmallIcon )
{
    SHFILEINFO    sfi;
    if (bSmallIcon)
    {
         SHGetFileInfo(
             (LPCTSTR)L"Doesn't matter", 
             FILE_ATTRIBUTE_DIRECTORY,
             &sfi, 
             sizeof(SHFILEINFO), 
             SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
    }
    else
    {
         SHGetFileInfo(
             (LPCTSTR)L"Doesn't matter", 
             FILE_ATTRIBUTE_DIRECTORY,
             &sfi, 
             sizeof(SHFILEINFO), 
             SHGFI_SYSICONINDEX | SHGFI_LARGEICON | SHGFI_USEFILEATTRIBUTES);
    }
    return sfi.iIcon;
}
HICON TSHFileInfo::GetFileIconHandle(std::wstring strFileName, BOOL bSmallIcon)
{
    SHFILEINFO    sfi;
    if (bSmallIcon)
    {
        SHGetFileInfo(
           (LPCTSTR)strFileName.c_str(), 
           FILE_ATTRIBUTE_NORMAL,
           &sfi, 
           sizeof(SHFILEINFO), 
           SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
    }
    else
    {
        SHGetFileInfo(
           (LPCTSTR)strFileName.c_str(), 
           FILE_ATTRIBUTE_NORMAL,
           &sfi, 
           sizeof(SHFILEINFO), 
           SHGFI_ICON | SHGFI_LARGEICON | SHGFI_USEFILEATTRIBUTES);
    }
    return sfi.hIcon;
}

HICON TSHFileInfo::GetFolderIconHandle(BOOL bSmallIcon )
{
    SHFILEINFO    sfi;
    if (bSmallIcon)
    {
         SHGetFileInfo(
         (LPCTSTR)"Doesn't matter", 
         FILE_ATTRIBUTE_DIRECTORY,
         &sfi, 
         sizeof(SHFILEINFO), 
         SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
    }
    else
    {
         SHGetFileInfo(
         (LPCTSTR)"Does not matter", 
         FILE_ATTRIBUTE_DIRECTORY,
         &sfi, 
         sizeof(SHFILEINFO), 
         SHGFI_ICON | SHGFI_LARGEICON | SHGFI_USEFILEATTRIBUTES);
    }
    return sfi.hIcon;
}

std::wstring TSHFileInfo::GetFileType(std::wstring strFileName)
{
	SHFILEINFO    sfi;
	
	SHGetFileInfo(
     (LPCTSTR)strFileName.c_str(), 
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
    EStreamError(const std::wstring Msg) :
        ExtException(Msg)
    {}
};

/*
class EWriteError : public ExtException
{
public:
    EWriteError(const std::wstring Msg) :
        ExtException(Msg)
    {}
};

class EReadError : public ExtException
{
public:
    EReadError(const std::wstring Msg) :
        ExtException(Msg)
    {}
};
*/
//---------------------------------------------------------------------------

void TStream::ReadBuffer(void *Buffer, __int64 Count)
{
  if ((Count != 0) && (Read(Buffer, Count) != Count))
    throw EReadError(::W2MB(FMTLOAD(SReadError).c_str()).c_str());
}

void TStream::WriteBuffer(const void *Buffer, __int64 Count)
{
  // DEBUG_PRINTF(L"Count = %d", Count);
  if ((Count != 0) && (Write(Buffer, Count) != Count))
    throw EWriteError(::W2MB(FMTLOAD(SWriteError).c_str()).c_str());
}

//---------------------------------------------------------------------------
void ReadError(const std::wstring &Name)
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

__int64 THandleStream::Read(void *Buffer, __int64 Count)
{
  __int64 Result = ::FileRead(FHandle, Buffer, Count);
  // DEBUG_PRINTF(L"Result = %d, FHandle = %d, Count = %d", Result, FHandle, Count);
  if (Result == -1) Result = 0;
  return Result;
}

__int64 THandleStream::Write(const void *Buffer, __int64 Count)
{
  __int64 Result = ::FileWrite(FHandle, Buffer, Count);
  // DEBUG_PRINTF(L"Result = %d, FHandle = %d, Count = %d", Result, FHandle, Count);
  if (Result == -1) Result = 0;
  return Result;
}
__int64 THandleStream::Seek(__int64 Offset, __int64 Origin)
{
    __int64 Result = ::FileSeek(FHandle, Offset, Origin);
    // DEBUG_PRINTF(L"Result = %d, FHandle = %d, Offset = %d, Origin = %d", Result, FHandle, Offset, Origin);
    return Result;
}
__int64 THandleStream::Seek(const __int64 Offset, TSeekOrigin Origin)
{
    ::Error(SNotImplemented, 1202);
    return 0;
}

void THandleStream::SetSize(const __int64 NewSize)
{
    // __int64 res =
    Seek(NewSize, soFromBeginning);
    // LARGE_INTEGER li;
    // li.QuadPart = size;
    // if (SetFilePointer(fh.get(), li.LowPart, &li.HighPart, FILE_BEGIN) == -1)
        // handleLastErrorImpl(_path);
    // DEBUG_PRINTF(L"FHandle = %d, res = %d", FHandle, res);
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

__int64 TMemoryStream::Read(void *Buffer, __int64 Count)
{
  __int64 Result = 0;
  if ((FPosition >= 0) && (Count >= 0))
  {
    Result = FSize - FPosition;
    if (Result > 0)
    {
      if (Result > Count) Result = Count;
      memmove((char *)FMemory + FPosition, Buffer, (int)Result);
      FPosition += Result;
      return Result;
    }
  }
  Result = 0;
  return Result;
}

__int64 TMemoryStream::Seek(__int64 Offset, __int64 Origin)
{
    // return Seek(Offset, soFromCurrent);
    ::Error(SNotImplemented, 1303);
    return 0;
}

__int64 TMemoryStream::Seek(const __int64 Offset, TSeekOrigin Origin)
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
  __int64 Result = FPosition;
  return Result;
}

void TMemoryStream::SaveToStream(TStream *Stream)
{
    if (FSize != 0) Stream->WriteBuffer(FMemory, FSize);
}

void TMemoryStream::SaveToFile(const std::wstring FileName)
{
  // TFileStream Stream(FileName, fmCreate);
  // SaveToStream(Stream);
  ::Error(SNotImplemented, 1203);
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
  if (OldPosition > NewSize) Seek(0, soFromEnd);
}

void TMemoryStream::SetCapacity(__int64 NewCapacity)
{
  SetPointer(Realloc(NewCapacity), FSize);
  FCapacity = NewCapacity;
}

void *TMemoryStream::Realloc(__int64 &NewCapacity)
{
  // DEBUG_PRINTF(L"FSize = %d, NewCapacity1 = %d", FSize, NewCapacity);
  if ((NewCapacity > 0) && (NewCapacity != FSize))
    NewCapacity = (NewCapacity + (MemoryDelta - 1)) & ~(MemoryDelta - 1);
  // DEBUG_PRINTF(L"NewCapacity2 = %d", NewCapacity);
  void *Result = FMemory;
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
        Result = malloc((int)NewCapacity);
      else
        Result = realloc(FMemory, (int)NewCapacity);
      if (Result == NULL)
        throw EStreamError(FMTLOAD(SMemoryStreamError));
    }
  }
  return Result;
}

void TMemoryStream::SetPointer(void *Ptr, __int64 Size)
{
  FMemory = Ptr;
  FSize = Size;
}

__int64 TMemoryStream::Write(const void *Buffer, __int64 Count)
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
          SetCapacity(Pos);
        FSize = Pos;
      }
      memmove((char *)Buffer, (char *)FMemory + FPosition, (int)Count);
      FPosition = Pos;
      Result = Count;
    }
  }
  return Result;
}

//---------------------------------------------------------------------------

bool IsRelative(const std::wstring &Value)
{
  return  !(!Value.empty() && (Value[0] == L'\\'));
}

TRegDataType DataTypeToRegData(DWORD Value)
{
  TRegDataType Result;
  if (Value == REG_SZ)
    Result = rdString;
  else if (Value == REG_EXPAND_SZ)
    Result = rdExpandString;
  else if (Value == REG_DWORD)
    Result = rdInteger;
  else if (Value == REG_BINARY)
    Result = rdBinary;
  else
    Result = rdUnknown;
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
  std::wstring S;
  if (GetKeyInfo(Info))
  {
    S.resize(Info.MaxValueLen + 1);
    for (size_t I = 0; I < Info.NumSubKeys; I++)
    {
      DWORD Len = Info.MaxValueLen + 1;
      RegEnumValue(GetCurrentKey(), I, &S[0], &Len, NULL, NULL, NULL, NULL);
      Strings->Add(S.c_str());
    }
  }
}

void TRegistry::GetKeyNames(TStrings * Strings)
{
  Strings->Clear();
  TRegKeyInfo Info;
  std::wstring S;
  if (GetKeyInfo(Info))
  {
    S.resize(Info.MaxSubKeyLen + 1);
    for (size_t I = 0; I < Info.NumSubKeys; I++)
    {
      DWORD Len = Info.MaxSubKeyLen + 1;
      RegEnumKeyEx(GetCurrentKey(), I, &S[0], &Len, NULL, NULL, NULL, NULL);
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

bool TRegistry::OpenKey(const std::wstring &Key, bool CanCreate)
{
  // DEBUG_PRINTF(L"key = %s, CanCreate = %d", Key.c_str(), CanCreate);
  bool Result = false;
  std::wstring S = Key;
  bool Relative = ::IsRelative(S);

  // if (!Relative) S.erase(0, 1); // Delete(S, 1, 1);
  HKEY TempKey = 0;
  if (!CanCreate || S.empty())
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
        S = FCurrentPath + L'\\' + S;
    ChangeKey(TempKey, S);
  }
  // DEBUG_PRINTF(L"CurrentKey = %d, Result = %d", GetCurrentKey(), Result);
  return Result;
}

bool TRegistry::DeleteKey(const std::wstring &Key)
{
  bool Result = false;
  std::wstring S = Key;
  bool Relative = ::IsRelative(S);
  // if not Relative then Delete(S, 1, 1);
  HKEY OldKey = GetCurrentKey();
  HKEY DeleteKey = GetKey(Key);
  if (DeleteKey != 0)
  {
    TRegistry *Self = this;
    BOOST_SCOPE_EXIT( (&Self) (&OldKey) (&DeleteKey) )
    {
        Self->SetCurrentKey(OldKey);
        RegCloseKey(DeleteKey);
    } BOOST_SCOPE_EXIT_END
    SetCurrentKey(DeleteKey);
    TRegKeyInfo Info;
    if (GetKeyInfo(Info))
    {
      std::wstring KeyName;
      KeyName.resize(Info.MaxSubKeyLen + 1);
      for (int I = Info.NumSubKeys - 1; I >= 0; I--)
      {
        DWORD Len = Info.MaxSubKeyLen + 1;
        if (RegEnumKeyEx(DeleteKey, (DWORD)I, &KeyName[0], &Len,
            NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
          this->DeleteKey(KeyName);
      }
    }
  }
  Result = RegDeleteKey(GetBaseKey(Relative), S.c_str()) == ERROR_SUCCESS;
  return Result;
}

bool TRegistry::DeleteValue(const std::wstring &Name)
{
  bool Result = RegDeleteValue(GetCurrentKey(), Name.c_str()) == ERROR_SUCCESS;
  return Result;
}

bool TRegistry::KeyExists(const std::wstring Key)
{
  bool Result = false;
  // DEBUG_PRINTF(L"Key = %s", Key.c_str());
  unsigned OldAccess = FAccess;
  {
    BOOST_SCOPE_EXIT( (&FAccess) (&OldAccess) )
    {
        FAccess = OldAccess;
    } BOOST_SCOPE_EXIT_END

    FAccess = STANDARD_RIGHTS_READ || KEY_QUERY_VALUE || KEY_ENUMERATE_SUB_KEYS;
    HKEY TempKey = GetKey(Key);
    if (TempKey != 0) RegCloseKey(TempKey);
    Result = TempKey != 0;
  }
  // DEBUG_PRINTF(L"Result = %d", Result);
  return Result;
}

bool TRegistry::ValueExists(const std::wstring Name)
{
  TRegDataInfo Info;
  bool Result = GetDataInfo(Name, Info);
  // DEBUG_PRINTF(L"Result = %d", Result);
  return Result;
}

bool TRegistry::GetDataInfo(const std::wstring &ValueName, TRegDataInfo &Value)
{
  DWORD DataType;
  memset(&Value, 0, sizeof(Value));
  bool Result = (RegQueryValueEx(GetCurrentKey(), ValueName.c_str(), NULL, &DataType, NULL,
    &Value.DataSize) == ERROR_SUCCESS);
  // DEBUG_PRINTF(L"Result = %d", Result);
  Value.RegData = DataTypeToRegData(DataType);
  return Result;
}

TRegDataType TRegistry::GetDataType(const std::wstring &ValueName)
{
  TRegDataType Result;
  TRegDataInfo Info;
  if (GetDataInfo(ValueName, Info))
    Result = Info.RegData;
  else
    Result = rdUnknown;
  return Result;
}

int TRegistry::GetDataSize(const std::wstring ValueName)
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

bool TRegistry::Readbool(const std::wstring Name)
{
  bool Result = Readint(Name) != 0;
  return Result;
}

TDateTime TRegistry::ReadDateTime(const std::wstring Name)
{
  TDateTime Result = TDateTime(ReadFloat(Name));
  return Result;
}

double TRegistry::ReadFloat(const std::wstring Name)
{
  double Result = 0.0;
  TRegDataType RegData;
  int Len = GetData(Name, &Result, sizeof(double), RegData);
  if ((RegData != rdBinary) || (Len != sizeof(double)))
  {
    ::ReadError(Name);
  }
  return Result;
}

int TRegistry::Readint(const std::wstring Name)
{
  DWORD Result = 0;
  TRegDataType RegData = rdUnknown;
  // DEBUG_PRINTF(L"Name = %s", Name.c_str());
  GetData(Name, &Result, sizeof(Result), RegData);
  // DEBUG_PRINTF(L"Result = %d, RegData = %d, rdInteger = %d", Result, RegData, rdInteger);
  if (RegData != rdInteger)
  {
    ::ReadError(Name);
  }
  return Result;
}

__int64 TRegistry::ReadInt64(const std::wstring Name)
{
  __int64 Result = 0;
  ReadBinaryData(Name, &Result, sizeof(Result));
  return Result;
}

std::wstring TRegistry::ReadString(const std::wstring Name)
{
  std::wstring Result = L"";
  TRegDataType RegData = rdUnknown;
  int Len = GetDataSize(Name);
  if (Len > 0)
  {
    Result.resize(Len);
    GetData(Name, (void *)Result.c_str(), Len, RegData);
    if ((RegData == rdString) || (RegData == rdExpandString))
    {
      PackStr(Result);
    }
    else ReadError(Name);
  }
  else
  {
    Result = L"";
  }
  return Result;
}

std::wstring TRegistry::ReadStringRaw(const std::wstring Name)
{
  std::wstring Result = ReadString(Name);
  return Result;
}

int TRegistry::ReadBinaryData(const std::wstring Name,
  void *Buffer, int BufSize)
{
  int Result = 0;
  TRegDataInfo Info;
  if (GetDataInfo(Name, Info))
  {
    Result = Info.DataSize;
    TRegDataType RegData = Info.RegData;
    if (((RegData == rdBinary) || (RegData == rdUnknown)) && (Result <= BufSize))
        GetData(Name, Buffer, Result, RegData);
    else
        ReadError(Name);
  }
  else
    Result = 0;
  return Result;
}

int TRegistry::GetData(const std::wstring &Name, void *Buffer,
  DWORD BufSize, TRegDataType &RegData)
{
  DWORD DataType = REG_NONE;
  // DEBUG_PRINTF(L"GetCurrentKey = %d", GetCurrentKey());
  if (RegQueryValueEx(GetCurrentKey(), Name.c_str(), NULL, &DataType,
    reinterpret_cast<BYTE *>(Buffer), &BufSize) != ERROR_SUCCESS)
  {
    throw std::exception("RegQueryValueEx failed"); // FIXME ERegistryException.CreateResFmt(@SRegGetDataFailed, [Name]);
  }
  int Result = BufSize;
  RegData = DataTypeToRegData(DataType);
  return Result;
}

void TRegistry::PutData(const std::wstring &Name, const void *Buffer,
  int BufSize, TRegDataType RegData)
{
  int DataType = ::RegDataToDataType(RegData);
  // DEBUG_PRINTF(L"GetCurrentKey = %d, Name = %s, REG_DWORD = %d, DataType = %d, BufSize = %d", GetCurrentKey(), Name.c_str(), REG_DWORD, DataType, BufSize);
  if (RegSetValueEx(GetCurrentKey(), Name.c_str(), 0, DataType, 
    reinterpret_cast<const BYTE *>(Buffer), BufSize) != ERROR_SUCCESS)
    throw std::exception("RegSetValueEx failed"); // ERegistryException(); // FIXME .CreateResFmt(SRegSetDataFailed, Name.c_str());
}

void TRegistry::Writebool(const std::wstring Name, bool Value)
{
    Writeint(Name, Value);
}
void TRegistry::WriteDateTime(const std::wstring Name, TDateTime Value)
{
    double Val = Value.operator double();
    PutData(Name, &Val, sizeof(double), rdBinary);
}
void TRegistry::WriteFloat(const std::wstring Name, double Value)
{
    PutData(Name, &Value, sizeof(double), rdBinary);
}
void TRegistry::WriteString(const std::wstring Name, const std::wstring Value)
{
    // DEBUG_PRINTF(L"Value = %s, Value.size = %d", Value.c_str(), Value.size());
    PutData(Name, (void *)Value.c_str(), Value.size() * sizeof(wchar_t) + 1, rdString);
}
void TRegistry::WriteStringRaw(const std::wstring Name, const std::wstring Value)
{
    PutData(Name, Value.c_str(), Value.size() * sizeof(wchar_t) + 1, rdString);
}
void TRegistry::Writeint(const std::wstring Name, int Value)
{
    DWORD Val = Value;
    PutData(Name, &Val, sizeof(DWORD), rdInteger);
    // WriteInt64(Name, Value);
}

void TRegistry::WriteInt64(const std::wstring Name, __int64 Value)
{
    WriteBinaryData(Name, &Value, sizeof(Value));
}
void TRegistry::WriteBinaryData(const std::wstring Name,
  const void *Buffer, int BufSize)
{
    PutData(Name, Buffer, BufSize, rdBinary);
}

void TRegistry::ChangeKey(HKEY Value, const std::wstring &Path)
{
  CloseKey();
  FCurrentKey = Value;
  FCurrentPath = Path;
}

HKEY TRegistry::GetBaseKey(bool Relative)
{
  HKEY Result = 0;
  if ((FCurrentKey == 0) || !Relative)
    Result = GetRootKey();
  else
    Result = FCurrentKey;
  return Result;
}

HKEY TRegistry::GetKey(const std::wstring &Key)
{
  std::wstring S = Key;
  bool Relative = ::IsRelative(S);
  // if not Relative then Delete(S, 1, 1);
  HKEY Result = 0;
  RegOpenKeyEx(GetBaseKey(Relative), S.c_str(), 0, FAccess, &Result);
  return Result;
}

bool TRegistry::GetKeyInfo(TRegKeyInfo &Value)
{
  memset(&Value, 0, sizeof(Value));
  bool Result = RegQueryInfoKey(GetCurrentKey(), NULL, NULL, NULL, &Value.NumSubKeys,
    &Value.MaxSubKeyLen, NULL, &Value.NumValues, &Value.MaxValueLen,
    &Value.MaxDataLen, NULL, &Value.FileTime) == ERROR_SUCCESS;
  // if SysLocale.FarEast and (Win32Platform = VER_PLATFORM_WIN32_NT) then
    // with Value do
    // begin
      // Inc(MaxSubKeyLen, MaxSubKeyLen);
      // Inc(MaxValueLen, MaxValueLen);
    // end;
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
bool TShortCut::operator < (const TShortCut &rhs) const
{
    return false;
}
