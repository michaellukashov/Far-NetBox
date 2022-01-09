#include <vcl.h>
#pragma hdrstop

#include <Common.h>
#include <Sysutils.hpp>

#include "NamedObjs.h"

static intptr_t NamedObjectSortProc(const void *Item1, const void *Item2)
{
  return get_as<TNamedObject>(Item1)->Compare(get_as<TNamedObject>(Item2));
}

//--- TNamedObject ----------------------------------------------------------
TNamedObject::TNamedObject(TObjectClassId Kind, UnicodeString AName) :
  TPersistent(Kind),
  FHidden(false)
{
  SetName(AName);
}

void TNamedObject::SetName(UnicodeString Value)
{
  FHidden = (Value.SubString(1, TNamedObjectList::HiddenPrefix.Length()) == TNamedObjectList::HiddenPrefix);
  FName = Value;
}

intptr_t TNamedObject::Compare(const TNamedObject *Other) const
{
  intptr_t Result;
  if (GetHidden() && !Other->GetHidden())
  {
    Result = -1;
  }
  else if (!GetHidden() && Other->GetHidden())
  {
    Result = 1;
  }
  else
  {
    Result = CompareLogicalText(GetName(), Other->GetName());
  }
  return Result;
}

bool TNamedObject::IsSameName(UnicodeString AName) const
{
  return (GetName().CompareIC(AName) == 0);
}

void TNamedObject::MakeUniqueIn(TNamedObjectList *List)
{
  // This object can't be item of list, it would create infinite loop
  if (List && (List->IndexOf(this) == -1))
  {
    while (List->FindByName(GetName()))
    {
      int64_t N = 0;
      intptr_t P;
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
        catch (Exception &E)
        {
          (void)E;
          N = 0;
        }
      }
      SetName(Name + L" (" + ::Int64ToStr(N + 1) + L")");
    }
  }
}

const UnicodeString TNamedObjectList::HiddenPrefix = "_!_";

TNamedObjectList::TNamedObjectList(TObjectClassId Kind) :
  TObjectList(Kind),
  FHiddenCount(0),
  FAutoSort(true),
  FControlledAdd(false)
{
}

const TNamedObject *TNamedObjectList::AtObject(intptr_t Index) const
{
  return const_cast<TNamedObjectList *>(this)->AtObject(Index);
}

TNamedObject *TNamedObjectList::AtObject(intptr_t Index)
{
  return GetAs<TNamedObject>(Index + FHiddenCount);
}

void TNamedObjectList::Recount()
{
  intptr_t Index = 0;
  while ((Index < TObjectList::GetCount()) && GetAs<TNamedObject>(Index)->GetHidden())
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

intptr_t TNamedObjectList::Add(TObject *AObject)
{
  intptr_t Result;
  TAutoFlag ControlledAddFlag(FControlledAdd);
  TNamedObject *NamedObject = static_cast<TNamedObject *>(AObject);
  // If temporarily not auto-sorting (when loading session list),
  // keep the hidden objects in front, so that HiddenCount is correct
  if (!FAutoSort && NamedObject->GetHidden())
  {
    Result = 0;
    Insert(Result, AObject);
    FHiddenCount++;
  }
  else
  {
    Result = -1;
    if (FAutoSort)
    {
      intptr_t pos;
      TNamedObject* NamedObject2 = GetSortObject(NamedObject->GetName(), pos);
      if (!NamedObject2)
      {
        Result = pos;
        Insert(Result, AObject);
      } 
      else 
      {
        Result= pos - 1;
      }
    }
    else 
    {
      Result = TObjectList::Add(AObject);
    }
  }
  return Result;
}

void TNamedObjectList::Notify(void *Ptr, TListNotification Action)
{
  if (Action == lnDeleted)
  {
    TNamedObject *NamedObject = static_cast<TNamedObject *>(Ptr);
    if (NamedObject->GetHidden() && (FHiddenCount >= 0))
    {
      FHiddenCount--;
    }
  }
  TObjectList::Notify(Ptr, Action);
  if (Action == lnAdded)
  {
    if (!FControlledAdd)
    {
      FHiddenCount = -1;
    }
  }
}

const TNamedObject *TNamedObjectList::FindByName(UnicodeString Name) const
{
  return const_cast<TNamedObjectList *>(this)->FindByName(Name);
}

TNamedObject* TNamedObjectList::GetSortObject(UnicodeString Name, intptr_t &outPosition)
{
  bool flag = false;
  int l = 0;
  int r = GetCountIncludingHidden() - 1;
  int mid=0;
  outPosition = 0;

  if (r < 0)
    return nullptr;

  TNamedObject tn(OBJECT_CLASS_TNamedObject,Name);
  TNamedObject* NamedObject = nullptr;
  int cp=0;

  while (l <= r)
  {
    mid = (l + r) / 2;
    NamedObject = static_cast<TNamedObject*>(GetObj(mid));
    cp = NamedObject->Compare(&tn);
    if (cp == 0)
      return NamedObject;

    if (cp > 0)
      r = mid - 1;
    else
      l = mid + 1;
  }

  outPosition = mid + (cp > 0? 0 : 1);
  return nullptr;
}

TNamedObject *TNamedObjectList::FindByName(UnicodeString Name)
{

  if (FAutoSort)
  {
    intptr_t outpos;
    return GetSortObject(Name, outpos);
  }
  else 
  {
    for (Integer Index = 0; Index < GetCountIncludingHidden(); ++Index)
    {
      // Not using AtObject as we iterate even hidden objects here
      TNamedObject *NamedObject = static_cast<TNamedObject *>(GetObj(Index));
      if (NamedObject->IsSameName(Name))
      {
        return NamedObject;
      }
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
  DebugAssert(FHiddenCount >= 0);
  return TObjectList::GetCount() - FHiddenCount;
}

intptr_t TNamedObjectList::GetCountIncludingHidden() const
{
  DebugAssert(FHiddenCount >= 0);
  return TObjectList::GetCount();
}
