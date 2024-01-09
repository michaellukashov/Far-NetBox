
#include <vcl.h>
#pragma hdrstop

#include <Common.h>
#include <Sysutils.hpp>
#include <StrUtils.hpp>
#include "NamedObjs.h"

// #pragma package(smart_init)

int32_t NamedObjectSortProc(const void * Item1, const void * Item2)
{
  return cast_to<TNamedObject>(ToObj(Item1))->Compare(cast_to<TNamedObject>(ToObj(Item2)));
}
//--- TNamedObject ----------------------------------------------------------
TNamedObject::TNamedObject(TObjectClassId Kind, const UnicodeString & AName) noexcept :
  TNamedObject(Kind)
{
  SetName(AName);
}

void TNamedObject::SetName(const UnicodeString & Value)
{
  FHidden = (Value.SubString(1, StrLength(TNamedObjectList::HiddenPrefix)) == TNamedObjectList::HiddenPrefix);
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
      UnicodeString AName = GetName();
      if ((AName[AName.Length()] == L')') && ((P = AName.LastDelimiter(UnicodeString(1, L'('))) > 0))
      {
        try
        {
          N = ::StrToInt64(AName.SubString(P + 1, AName.Length() - P - 1));
          AName.Delete(P, AName.Length() - P + 1);
          SetName(AName.TrimRight());
        }
        catch(Exception &)
        {
          N = 0;
        }
      }
      SetName(AName + FORMAT(L" (%s)", ::Int64ToStr(N + 1)));
    }
  }
}
//--- TNamedObjectList ------------------------------------------------------
// const UnicodeString TNamedObjectList::HiddenPrefix = "_!_";

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
  int32_t Result{0};
  const TAutoFlag ControlledAddFlag(FControlledAdd); nb::used(ControlledAddFlag);
  const TNamedObject * NamedObject = cast_to<TNamedObject>(AObject);
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
      int32_t Pos{0};
      const TNamedObject * NamedObject2 = GetSortObject(NamedObject->GetName(), Pos);
      if (!NamedObject2)
      {
        Result = Pos;
        Insert(Result, AObject);
      } 
      else 
      {
        Result= Pos - 1;
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
    const TNamedObject * NamedObject = cast_to<TNamedObject>(Ptr);
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

TNamedObject * TNamedObjectList::GetSortObject(const UnicodeString & Name, int32_t & Position)
{
  // bool Flag = false;
  int32_t L = 0;
  int32_t R = GetCountIncludingHidden() - 1;
  int32_t Mid=0;
  Position = 0;

  if (R < 0)
    return nullptr;

  const TNamedObject Object(OBJECT_CLASS_TNamedObject, Name);
  TNamedObject * NamedObject = nullptr;
  int32_t Cp = 0;

  while (L <= R)
  {
    Mid = (L + R) / 2;
    NamedObject = cast_to<TNamedObject>(Get(Mid));
    Cp = NamedObject->Compare(&Object);
    if (Cp == 0)
      return NamedObject;

    if (Cp > 0)
      R = Mid - 1;
    else
      L = Mid + 1;
  }

  Position = Mid + (Cp > 0? 0 : 1);
  return nullptr;
}

TNamedObject * TNamedObjectList::FindByName(const UnicodeString & AName)
{
  if (FAutoSort)
  {
    int32_t Outpos{0};
    return GetSortObject(AName, Outpos);
  }
  else
  {
    for (Integer Index = 0; Index < GetCountIncludingHidden(); ++Index)
    {
      // Not using AtObject as we iterate even hidden objects here
      TNamedObject * NamedObject = GetAs<TNamedObject>(Index);
      if (NamedObject && NamedObject->IsSameName(AName))
      {
        return NamedObject;
      }
    }
  }
  return nullptr;
}

void TNamedObjectList::SetCount(int32_t Value)
{
  TObjectList::SetCount(Value/*+HiddenCount*/);
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
