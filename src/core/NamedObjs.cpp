//---------------------------------------------------------------------------
#ifndef _MSC_VER
#include <vcl.h>
#pragma hdrstop
#else
#include "stdafx.h"
#endif

#include "NamedObjs.h"
#include "Common.h"
//---------------------------------------------------------------------------
#ifndef _MSC_VER
#pragma package(smart_init)
#endif
//---------------------------------------------------------------------------
int /* __fastcall */ NamedObjectSortProc(void * Item1, void * Item2)
{
  bool HasPrefix1 = (static_cast<TNamedObject *>(Item1))->GetHidden();
  bool HasPrefix2 = (static_cast<TNamedObject *>(Item2))->GetHidden();
  if (HasPrefix1 && !HasPrefix2) { return -1; }
    else
  if (!HasPrefix1 && HasPrefix2) { return 1; }
    else
  {
    return AnsiCompareStr((static_cast<TNamedObject *>(Item1))->GetName(), (static_cast<TNamedObject *>(Item2))->GetName());
  }
}
//--- TNamedObject ----------------------------------------------------------
/* __fastcall */ TNamedObject::TNamedObject(UnicodeString AName) :
  FHidden(false)
{
  SetName(AName);
}
//---------------------------------------------------------------------------
void __fastcall TNamedObject::SetName(UnicodeString value)
{
  FHidden = (value.SubString(1, TNamedObjectList::HiddenPrefix.Length()) == TNamedObjectList::HiddenPrefix);
  FName = value;
}
//---------------------------------------------------------------------------
Integer __fastcall TNamedObject::CompareName(UnicodeString aName,
  Boolean CaseSensitive)
{
  if (CaseSensitive)
  {
    return ::AnsiCompare(GetName(), aName);
  }
  else
  {
    return ::AnsiCompareIC(GetName(), aName);
  }
}
//---------------------------------------------------------------------------
void __fastcall TNamedObject::MakeUniqueIn(TNamedObjectList * List)
{
  // This object can't be item of list, it would create infinite loop
  if (List && (List->IndexOf(this) == -1))
    while (List->FindByName(GetName()))
    {
      Integer N = 0, P = 0;
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
          N = 0;
        };
      Name += L" (" + IntToStr(static_cast<int>(N+1)) + L")";
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
  AutoSort = true;
}
//---------------------------------------------------------------------------
TNamedObject * __fastcall TNamedObjectList::AtObject(Integer Index)
{
  return static_cast<TNamedObject *>(GetItem(Index + GetHiddenCount()));
}
//---------------------------------------------------------------------------
void __fastcall TNamedObjectList::Recount()
{
  int i = 0;
  while ((i < TObjectList::GetCount()) && (static_cast<TNamedObject *>(GetItem(i)))->GetHidden()) { i++; }
  FHiddenCount = i;
}
//---------------------------------------------------------------------------
void __fastcall TNamedObjectList::AlphaSort()
{
  Sort(NamedObjectSortProc);
}
//---------------------------------------------------------------------------
void __fastcall TNamedObjectList::Notify(void *Ptr, TListNotification Action)
{
  TObjectList::Notify(Ptr, Action);
  if (AutoSort && (Action == lnAdded)) { AlphaSort(); }
  Recount();
}
//---------------------------------------------------------------------------
TNamedObject * __fastcall TNamedObjectList::FindByName(UnicodeString Name,
  Boolean CaseSensitive)
{
  for (Integer Index = 0; Index < TObjectList::GetCount(); Index++)
    if (!(static_cast<TNamedObject *>(GetItem(Index)))->CompareName(Name, CaseSensitive))
    {
      return static_cast<TNamedObject *>(GetItem(Index));
    }
  return NULL;
}
//---------------------------------------------------------------------------
void __fastcall TNamedObjectList::SetCount(int value)
{
  TObjectList::SetCount(value/*+HiddenCount*/);
}
//---------------------------------------------------------------------------
int __fastcall TNamedObjectList::GetCount()
{
  return TObjectList::GetCount() - GetHiddenCount();
}
