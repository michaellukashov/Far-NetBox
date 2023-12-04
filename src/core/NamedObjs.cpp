
#include <vcl.h>
#pragma hdrstop

#include <Common.h>
#include <Sysutils.hpp>
#include "NamedObjs.h"

// #pragma package(smart_init)

int32_t NamedObjectSortProc(const TObject * Item1, const TObject * Item2)
{
  return cast_to<TNamedObject>(Item1)->Compare(cast_to<TNamedObject>(Item2));
}
//--- TNamedObject ----------------------------------------------------------
TNamedObject::TNamedObject(TObjectClassId Kind, const UnicodeString & AName) noexcept :
  TPersistent(Kind)
{
  SetName(AName);
}

void TNamedObject::SetName(const UnicodeString & Value)
{
  FHidden = (Value.SubString(1, TNamedObjectList::HiddenPrefix.Length()) == TNamedObjectList::HiddenPrefix);
  FName = Value;
}

int32_t TNamedObject::Compare(const TNamedObject * Other) const
{
  int32_t Result;
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
    Result = CompareLogicalText(GetName(), Other->GetName(), true);
  }
  return Result;
}

bool TNamedObject::IsSameName(const UnicodeString & AName) const
{
  return (GetName().CompareIC(AName) == 0);
}

void TNamedObject::MakeUniqueIn(TNamedObjectList * List)
{
  // This object can't be item of list, it would create infinite loop
  if (List && (List->IndexOf(this) == -1))
  {
    while (List->FindByName(GetName()))
    {
      int64_t N = 0;
      int32_t P = 0;
      // If name already contains number parenthesis remove it (and remember it)
      UnicodeString Name = GetName();
      if ((Name[Name.Length()] == L')') && ((P = Name.LastDelimiter(UnicodeString(1, L'('))) > 0))
      {
        try
        {
          N = ::StrToInt64(Name.SubString(P + 1, Name.Length() - P - 1));
          Name.Delete(P, Name.Length() - P + 1);
          SetName(Name.TrimRight());
        }
        catch(Exception &)
        {
          N = 0;
        }
      }
      SetName(Name + L" (" + ::Int64ToStr(N + 1) + L")");
    }
  }
}
//--- TNamedObjectList ------------------------------------------------------
const UnicodeString TNamedObjectList::HiddenPrefix = "_!_";

TNamedObjectList::TNamedObjectList(TObjectClassId Kind) noexcept :
  TObjectList(Kind)
{
  FAutoSort = True;
  FHiddenCount = 0;
  FControlledAdd = false;
}

TNamedObject * TNamedObjectList::AtObject(int32_t Index)
{
  return GetAs<TNamedObject>(Index + FHiddenCount);
}

void TNamedObjectList::Recount()
{
  int32_t Index = 0;
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

int32_t TNamedObjectList::Add(TObject * AObject)
{
  int32_t Result;
  const TAutoFlag ControlledAddFlag(FControlledAdd); nb::used(ControlledAddFlag);
  const TNamedObject * NamedObject = static_cast<TNamedObject *>(AObject);
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

void TNamedObjectList::Notify(TObject * Ptr, TListNotification Action)
{
  if (Action == lnDeleted)
  {
    const TNamedObject * NamedObject = static_cast<TNamedObject *>(Ptr);
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

TNamedObject * TNamedObjectList::FindByName(const UnicodeString & AName)
{
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
      TNamedObject * NamedObject = GetAs<TNamedObject>(Index);
      if (NamedObject->IsSameName(AName))
      {
        return NamedObject;
      }
    }
  }
  return nullptr;
}

int32_t TNamedObjectList::GetCount() const
{
  DebugAssert(FHiddenCount >= 0);
  return TObjectList::GetCount() - FHiddenCount;
}

int32_t TNamedObjectList::GetCountIncludingHidden() const
{
  DebugAssert(FHiddenCount >= 0);
  return TObjectList::GetCount();
}

const TNamedObject * TNamedObjectList::AtObject(int32_t Index) const
{
  return const_cast<TNamedObjectList *>(this)->AtObject(Index);
}

const TNamedObject * TNamedObjectList::FindByName(const UnicodeString & AName) const
{
  return const_cast<TNamedObjectList *>(this)->FindByName(AName);
}
