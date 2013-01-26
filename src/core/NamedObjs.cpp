//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include "NamedObjs.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------
int /* __fastcall */ NamedObjectSortProc(const void * Item1, const void * Item2)
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
    return AnsiCompareStr((static_cast<const TNamedObject *>(Item1))->GetName(), (static_cast<const TNamedObject *>(Item2))->GetName());
  }
}
//--- TNamedObject ----------------------------------------------------------
/* __fastcall */ TNamedObject::TNamedObject(UnicodeString AName) :
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
Integer TNamedObject::CompareName(UnicodeString aName,
  Boolean CaseSensitive)
{
  if (CaseSensitive)
  {
    return GetName().Compare(aName);
  }
  else
  {
    return GetName().CompareIC(aName);
  }
}
//---------------------------------------------------------------------------
void TNamedObject::MakeUniqueIn(TNamedObjectList * List)
{
  // This object can't be item of list, it would create infinite loop
  if (List && (List->IndexOf(this) == -1))
    while (List->FindByName(GetName()))
    {
      intptr_t N = 0, P = 0;
      // If name already contains number parenthesis remove it (and remember it)
      UnicodeString Name = GetName();
      if ((Name[Name.Length()] == L')') && ((P = Name.LastDelimiter(L'(')) > 0))
        try
        {
          N = StrToInt(Name.SubString(P + 1, Name.Length() - P - 1));
          Name.Delete(P, Name.Length() - P + 1);
          SetName(Name.TrimRight());
        }
        catch (Exception &E)
        {
          (void)E;
          N = 0;
        }
      SetName(Name + L" (" + IntToStr(static_cast<int>(N+1)) + L")");
    }
}
//--- TNamedObjectList ------------------------------------------------------
const UnicodeString TNamedObjectList::HiddenPrefix = L"_!_";
//---------------------------------------------------------------------------
/* __fastcall */ TNamedObjectList::TNamedObjectList():
  TObjectList(),
  FHiddenCount(0),
  AutoSort(true)
{
  AutoSort = True;
}
//---------------------------------------------------------------------------
TNamedObject * TNamedObjectList::AtObject(intptr_t Index)
{
  return static_cast<TNamedObject *>(Items[Index + GetHiddenCount()]);
}
//---------------------------------------------------------------------------
void TNamedObjectList::Recount()
{
  intptr_t I = 0;
  while ((I < TObjectList::GetCount()) && (static_cast<TNamedObject *>(Items[I])->GetHidden()))
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
  if (AutoSort && (Action == lnAdded)) { AlphaSort(); }
  Recount();
}
//---------------------------------------------------------------------------
TNamedObject * TNamedObjectList::FindByName(UnicodeString Name,
  Boolean CaseSensitive)
{
  for (Integer Index = 0; Index < TObjectList::GetCount(); ++Index)
  {
    if (!(static_cast<TNamedObject *>(Items[Index]))->CompareName(Name, CaseSensitive))
    {
      return static_cast<TNamedObject *>(Items[Index]);
    }
  }
  return NULL;
}
//---------------------------------------------------------------------------
void TNamedObjectList::SetCount(intptr_t Value)
{
  TObjectList::SetCount(Value/*+HiddenCount*/);
}
//---------------------------------------------------------------------------
intptr_t TNamedObjectList::GetCount()
{
  return TObjectList::GetCount() - GetHiddenCount();
}

