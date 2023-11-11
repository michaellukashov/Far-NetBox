
#include <vcl.h>
#pragma hdrstop

#include <Common.h>
#include <Sysutils.hpp>

#include "NamedObjs.h"

__removed #pragma package(smart_init)

static int32_t NamedObjectSortProc(const void * Item1, const void * Item2)
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

int32_t TNamedObject::Compare(const TNamedObject *Other) const
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

void TNamedObject::MakeUniqueIn(TNamedObjectList *List)
{
  // This object can't be item of list, it would create infinite loop
  if (List && (List->IndexOf(this) == -1))
  {
    while (List->FindByName(GetName()))
    {
      int64_t N = 0;
      int32_t P;
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
        catch (Exception &/*E*/)
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
}

const TNamedObject * TNamedObjectList::AtObject(int32_t Index) const
{
  return const_cast<TNamedObjectList *>(this)->AtObject(Index);
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
  TAutoFlag ControlledAddFlag(FControlledAdd); nb::used(ControlledAddFlag);
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
    Result = TObjectList::Add(AObject);
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
    if (FAutoSort)
    {
      AlphaSort();
    }
  }
}

const TNamedObject * TNamedObjectList::FindByName(const UnicodeString & AName) const
{
  return const_cast<TNamedObjectList *>(this)->FindByName(AName);
}

TNamedObject * TNamedObjectList::FindByName(const UnicodeString & AName)
{
  // This should/can be optimized when list is sorted
  for (Integer Index = 0; Index < GetCountIncludingHidden(); ++Index)
  {
    // Not using AtObject as we iterate even hidden objects here
    TNamedObject *NamedObject = static_cast<TNamedObject *>(GetObj(Index));
    if (NamedObject->IsSameName(AName))
    {
      return NamedObject;
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
