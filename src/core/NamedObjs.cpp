//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include "NamedObjs.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------
static intptr_t NamedObjectSortProc(const void * Item1, const void * Item2)
{
  bool HasPrefix1 = (static_cast<const TNamedObject *>(Item1))->GetHidden();
  bool HasPrefix2 = (static_cast<const TNamedObject *>(Item2))->GetHidden();
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
    return AnsiCompareStr(
      NB_STATIC_DOWNCAST_CONST(TNamedObject, Item1)->GetName(),
      NB_STATIC_DOWNCAST_CONST(TNamedObject, Item2)->GetName());
  }
}
//--- TNamedObject ----------------------------------------------------------
TNamedObject::TNamedObject(const UnicodeString & AName) :
  FHidden(false)
{
  SetName(AName);
}
//---------------------------------------------------------------------------
void TNamedObject::SetName(const UnicodeString & Value)
{
  FHidden = (Value.SubString(1, TNamedObjectList::HiddenPrefix.Length()) == TNamedObjectList::HiddenPrefix);
  FName = Value;
}
//---------------------------------------------------------------------------
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
//---------------------------------------------------------------------------
void TNamedObject::MakeUniqueIn(TNamedObjectList * List)
{
  // This object can't be item of list, it would create infinite loop
  if (List && (List->IndexOf(this) == -1))
    while (List->FindByName(GetName()))
    {
      intptr_t P = 0;
      intptr_t N = 0;
      // If name already contains number parenthesis remove it (and remember it)
      UnicodeString Name = GetName();
      if ((Name[Name.Length()] == L')') && ((P = Name.LastDelimiter(L'(')) > 0))
      {
        try
        {
          N = Sysutils::StrToInt(Name.SubString(P + 1, Name.Length() - P - 1));
          Name.Delete(P, Name.Length() - P + 1);
          SetName(Name.TrimRight());
        }
        catch (Exception &E)
        {
          (void)E;
          N = 0;
        }
      }
      SetName(Name + L" (" + IntToStr(N+1) + L")");
    }
}
//--- TNamedObjectList ------------------------------------------------------
const UnicodeString TNamedObjectList::HiddenPrefix = L"_!_";
//---------------------------------------------------------------------------
TNamedObjectList::TNamedObjectList():
  TObjectList(),
  AutoSort(true),
  FHiddenCount(0)
{
}
//---------------------------------------------------------------------------
const TNamedObject * TNamedObjectList::AtObject(intptr_t Index) const
{
  return const_cast<TNamedObjectList *>(this)->AtObject(Index);
}
//---------------------------------------------------------------------------
TNamedObject * TNamedObjectList::AtObject(intptr_t Index)
{
  return static_cast<TNamedObject *>(GetItem(Index + GetHiddenCount()));
}
//---------------------------------------------------------------------------
void TNamedObjectList::Recount()
{
  intptr_t I = 0;
  while ((I < TObjectList::GetCount()) && (static_cast<TNamedObject *>(GetItem(I))->GetHidden()))
  {
    ++I;
  }
  FHiddenCount = I;
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
  if (AutoSort && (Action == lnAdded))
  {
    AlphaSort();
  }
  Recount();
}
//---------------------------------------------------------------------------
TNamedObject * TNamedObjectList::FindByName(const UnicodeString & Name,
  Boolean CaseSensitive)
{
  for (Integer Index = 0; Index < TObjectList::GetCount(); ++Index)
  {
    if (!(static_cast<TNamedObject *>(GetItem(Index)))->CompareName(Name, CaseSensitive))
    {
      return static_cast<TNamedObject *>(GetItem(Index));
    }
  }
  return nullptr;
}
//---------------------------------------------------------------------------
void TNamedObjectList::SetCount(intptr_t Value)
{
  TObjectList::SetCount(Value/*+HiddenCount*/);
}
//---------------------------------------------------------------------------
intptr_t TNamedObjectList::GetCount() const
{
  return TObjectList::GetCount() - GetHiddenCount();
}

//------------------------------------------------------------------------------
NB_IMPLEMENT_CLASS(TNamedObject, NB_GET_CLASS_INFO(TPersistent), nullptr);
//------------------------------------------------------------------------------
