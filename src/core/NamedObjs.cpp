//---------------------------------------------------------------------------
#include "stdafx.h"

#include "NamedObjs.h"
#include "Common.h"
//---------------------------------------------------------------------------
int NamedObjectSortProc(void * Item1, void * Item2)
{
  bool HasPrefix1 = (static_cast<TNamedObject *>(Item1))->GetHidden();
  bool HasPrefix2 = (static_cast<TNamedObject *>(Item2))->GetHidden();
  if (HasPrefix1 && !HasPrefix2) return -1;
    else
  if (!HasPrefix1 && HasPrefix2) return 1;
    else
  return ::AnsiCompareStr((static_cast<TNamedObject *>(Item1))->GetName(), (static_cast<TNamedObject *>(Item2))->GetName());
}
//--- TNamedObject ----------------------------------------------------------
TNamedObject::TNamedObject(const std::wstring AName) :
    nb::TPersistent()
{
  SetName(AName);
}
//---------------------------------------------------------------------------
void TNamedObject::SetName(const std::wstring value)
{
  FHidden = (value.substr(0, TNamedObjectList::HiddenPrefix.size()) == TNamedObjectList::HiddenPrefix);
  FName = value;
}

int TNamedObject::CompareName(const std::wstring aName,
  bool CaseSensitive)
{
  // DEBUG_PRINTF(L"CaseSensitive = %d, Name = %s, aName = %s", CaseSensitive, Name.c_str(), aName.c_str());
  if (CaseSensitive)
    return ::AnsiCompare(GetName(), aName);
  else
    return ::AnsiCompareIC(GetName(), aName);
}
//---------------------------------------------------------------------------
void TNamedObject::MakeUniqueIn(TNamedObjectList * List)
{
  // This object can't be item of list, it would create infinite loop
  if (List && (List->IndexOf(this) == -1))
    while (List->FindByName(GetName()))
    {
      size_t N = 0, P = 0;
      // If name already contains number parenthesis remove it (and remember it)
      std::wstring Name = GetName();
      if ((Name[Name.size() - 1] == L')') && ((P = ::LastDelimiter(Name, L"(")) != std::wstring::npos))
        try
        {
          N = StrToInt(Name.substr(P + 1, Name.size() - P - 1));
          Name.erase(P, Name.size() - P + 1);
          SetName(::TrimRight(Name));
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
  nb::TObjectList(),
  FHiddenCount(0),
  AutoSort(true)
{
  AutoSort = true;
}
//---------------------------------------------------------------------------
TNamedObject * TNamedObjectList::AtObject(int Index)
{
    // DEBUG_PRINTF(L"Index = %d, Count = %d, GetHiddenCount = %d", Index, GetCount(), GetHiddenCount());
  return static_cast<TNamedObject *>(GetItem(Index+GetHiddenCount()));
}
//---------------------------------------------------------------------------
void TNamedObjectList::Recount()
{
  size_t i = 0;
  while ((i < nb::TObjectList::GetCount()) && (static_cast<TNamedObject *>(GetItem(i)))->GetHidden()) i++;
  FHiddenCount = i;
}
//---------------------------------------------------------------------------
void TNamedObjectList::AlphaSort()
{
  Sort(NamedObjectSortProc);
}
//---------------------------------------------------------------------------
void TNamedObjectList::Notify(void *Ptr, nb::TListNotification Action)
{
  nb::TObjectList::Notify(Ptr, Action);
  if (AutoSort && (Action == nb::lnAdded)) AlphaSort();
  Recount();
}
//---------------------------------------------------------------------------
TNamedObject * TNamedObjectList::FindByName(const std::wstring Name,
  bool CaseSensitive)
{
  for (size_t Index = 0; Index < nb::TObjectList::GetCount(); Index++)
    if (!(static_cast<TNamedObject *>(GetItem(Index)))->CompareName(Name, CaseSensitive))
      return static_cast<TNamedObject *>(GetItem(Index));
  return NULL;
}
//---------------------------------------------------------------------------
void TNamedObjectList::SetCount(size_t value)
{
  nb::TObjectList::SetCount(value/*+HiddenCount*/);
}
//---------------------------------------------------------------------------
size_t TNamedObjectList::GetCount()
{
  return nb::TObjectList::GetCount() - GetHiddenCount();
}
