//---------------------------------------------------------------------------
#include "stdafx.h"

#include "NamedObjs.h"
#include "Common.h"
//---------------------------------------------------------------------------
int NamedObjectSortProc(void * Item1, void * Item2)
{
  bool HasPrefix1 = ((TNamedObject *)Item1)->GetHidden();
  bool HasPrefix2 = ((TNamedObject *)Item2)->GetHidden();
  if (HasPrefix1 && !HasPrefix2) return -1;
    else
  if (!HasPrefix1 && HasPrefix2) return 1;
    else
  return ::AnsiCompareStr(((TNamedObject *)Item1)->Name, ((TNamedObject *)Item2)->Name);
}
//--- TNamedObject ----------------------------------------------------------
TNamedObject::TNamedObject(std::wstring AName)
{
  SetName(AName);
}
//---------------------------------------------------------------------------
void TNamedObject::SetName(std::wstring value)
{
  FHidden = (value.substr(0, TNamedObjectList::HiddenPrefix.size()) == TNamedObjectList::HiddenPrefix);
  FName = value;
}

int TNamedObject::CompareName(std::wstring aName,
  bool CaseSensitive)
{
  // DEBUG_PRINTF(L"CaseSensitive = %d, Name = %s, aName = %s", CaseSensitive, Name.c_str(), aName.c_str());
  if (CaseSensitive)
    return ::AnsiCompare(Name, aName);
  else
    return ::AnsiCompareIC(Name, aName);
}
//---------------------------------------------------------------------------
void TNamedObject::MakeUniqueIn(TNamedObjectList * List)
{
  // This object can't be item of list, it would create infinite loop
  if (List && (List->IndexOf(this) == -1))
    while (List->FindByName(Name))
    {
      size_t N = 0, P = 0;
      // If name already contains number parenthesis remove it (and remember it)
      if ((Name[Name.size() - 1] == L')') && ((P = ::LastDelimiter(Name, L"(")) != std::wstring::npos))
        try
        {
          N = StrToInt(Name.substr(P + 1, Name.size() - P - 1));
          Name.erase(P, Name.size() - P + 1);
          Name = ::TrimRight(Name);
        }
        catch (const std::exception &E)
        {
            N = 0;
        };
      Name += L" (" + IntToStr(N+1) + L")";
    }
}
//--- TNamedObjectList ------------------------------------------------------
const std::wstring TNamedObjectList::HiddenPrefix = L"_!_";
//---------------------------------------------------------------------------
TNamedObjectList::TNamedObjectList() :
  TObjectList(),
  FHiddenCount(0),
  AutoSort(true)
{
  AutoSort = true;
}
//---------------------------------------------------------------------------
TNamedObject * TNamedObjectList::AtObject(int Index)
{
    // DEBUG_PRINTF(L"Index = %d, Count = %d, GetHiddenCount = %d", Index, GetCount(), GetHiddenCount());
  return (TNamedObject *)GetItem(Index+GetHiddenCount());
}
//---------------------------------------------------------------------------
void TNamedObjectList::Recount()
{
  size_t i = 0;
  while ((i < TObjectList::GetCount()) && ((TNamedObject *)GetItem(i))->GetHidden()) i++;
  FHiddenCount = i;
}
//---------------------------------------------------------------------------
void TNamedObjectList::AlphaSort()
{
  Sort(NamedObjectSortProc);
}
//---------------------------------------------------------------------------
void TNamedObjectList::Notify(void *Ptr, TListNotification Action)
{
  TObjectList::Notify(Ptr, Action);
  if (AutoSort && (Action == lnAdded)) AlphaSort();
  Recount();
}
//---------------------------------------------------------------------------
TNamedObject * TNamedObjectList::FindByName(std::wstring Name,
  bool CaseSensitive)
{
  for (size_t Index = 0; Index < TObjectList::GetCount(); Index++)
    if (!((TNamedObject *)GetItem(Index))->CompareName(Name, CaseSensitive))
      return (TNamedObject *)GetItem(Index);
  return NULL;
}
//---------------------------------------------------------------------------
void TNamedObjectList::SetCount(size_t value)
{
  TObjectList::SetCount(value/*+HiddenCount*/);
}
//---------------------------------------------------------------------------
size_t TNamedObjectList::GetCount()
{
  return TObjectList::GetCount() - GetHiddenCount();
}
