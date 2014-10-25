
#include <vcl.h>
#pragma hdrstop

#include <Sysutils.hpp>
#include "NamedObjs.h"

static intptr_t NamedObjectSortProc(const void * Item1, const void * Item2)
{
  bool HasPrefix1 = NB_STATIC_DOWNCAST_CONST(TNamedObject, Item1)->GetHidden();
  bool HasPrefix2 = NB_STATIC_DOWNCAST_CONST(TNamedObject, Item2)->GetHidden();
  if (HasPrefix1 && !HasPrefix2)
  {
    return -1;
  }
  else if (!HasPrefix1 && HasPrefix2)
  {
    return 1;
  }
  else
  {
    return ::CompareLogicalText(
      NB_STATIC_DOWNCAST_CONST(TNamedObject, Item1)->GetName(),
      NB_STATIC_DOWNCAST_CONST(TNamedObject, Item2)->GetName());
  }
}

TNamedObject::TNamedObject(const UnicodeString & AName) :
  FHidden(false)
{
  SetName(AName);
}

void TNamedObject::SetName(const UnicodeString & Value)
{
  UnicodeString HiddenPrefix(CONST_HIDDEN_PREFIX);
  FHidden = (Value.SubString(1, HiddenPrefix.Length()) == HiddenPrefix);
  FName = Value;
}

Integer TNamedObject::CompareName(const UnicodeString & AName,
  Boolean CaseSensitive)
{
  if (CaseSensitive)
  {
    return GetName().Compare(AName);
  }
  else
  {
    return GetName().CompareIC(AName);
  }
}

void TNamedObject::MakeUniqueIn(TNamedObjectList * List)
{
  // This object can't be item of list, it would create infinite loop
  if (List && (List->IndexOf(this) == -1))
    while (List->FindByName(GetName()))
    {
      intptr_t P = 0;
      int64_t N = 0;
      // If name already contains number parenthesis remove it (and remember it)
      UnicodeString Name = GetName();
      if ((Name[Name.Length()] == L')') && ((P = Name.LastDelimiter(L'(')) > 0))
      {
        try
        {
          N = ::StrToInt64(Name.SubString(P + 1, Name.Length() - P - 1));
          Name.Delete(P, Name.Length() - P + 1);
          SetName(Name.TrimRight());
        }
        catch (Exception & E)
        {
          (void)E;
          N = 0;
        }
      }
      SetName(Name + L" (" + ::Int64ToStr(N+1) + L")");
    }
}

TNamedObjectList::TNamedObjectList() :
  TObjectList(),
  AutoSort(true),
  FHiddenCount(0)
{
}

const TNamedObject * TNamedObjectList::AtObject(intptr_t Index) const
{
  return const_cast<TNamedObjectList *>(this)->AtObject(Index);
}

TNamedObject * TNamedObjectList::AtObject(intptr_t Index)
{
  return NB_STATIC_DOWNCAST(TNamedObject, GetItem(Index + GetHiddenCount()));
}

void TNamedObjectList::Recount()
{
  intptr_t Index = 0;
  while ((Index < TObjectList::GetCount()) && (NB_STATIC_DOWNCAST(TNamedObject, GetItem(Index))->GetHidden()))
  {
    ++Index;
  }
  FHiddenCount = Index;
}

void TNamedObjectList::AlphaSort()
{
  Sort(NamedObjectSortProc);
  Recount();
}

void TNamedObjectList::Notify(void * Ptr, TListNotification Action)
{
  TObjectList::Notify(Ptr, Action);
  if (Action == lnAdded)
  {
    FHiddenCount = -1;
    if (AutoSort)
    {
      AlphaSort();
    }
  }
}

TNamedObject * TNamedObjectList::FindByName(const UnicodeString & Name, Boolean CaseSensitive) const
{
  return const_cast<TNamedObjectList *>(this)->FindByName(Name, CaseSensitive);
}

TNamedObject * TNamedObjectList::FindByName(const UnicodeString & Name,
  Boolean CaseSensitive)
{
  // this should/can be optimized when list is sorted
  for (Integer Index = 0; Index < TObjectList::GetCount(); ++Index)
  {
    if (!(NB_STATIC_DOWNCAST(TNamedObject, GetItem(Index)))->CompareName(Name, CaseSensitive))
    {
      return NB_STATIC_DOWNCAST(TNamedObject, GetItem(Index));
    }
  }
  return nullptr;
}

void TNamedObjectList::SetCount(intptr_t Value)
{
  TObjectList::SetCount(Value/*+HiddenCount*/);
}

intptr_t TNamedObjectList::GetCount() const
{
  assert(FHiddenCount >= 0);
  return TObjectList::GetCount() - GetHiddenCount();
}

intptr_t TNamedObjectList::GetCountIncludingHidden() const
{
  assert(FHiddenCount >= 0);
  return TObjectList::GetCount();
}

NB_IMPLEMENT_CLASS(TNamedObject, NB_GET_CLASS_INFO(TPersistent), nullptr);

