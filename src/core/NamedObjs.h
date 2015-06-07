#pragma once

#include <system.hpp>
#include <contnrs.hpp>

#define CONST_HIDDEN_PREFIX L"_!_"

class TNamedObjectList;
class TNamedObject : public TPersistent
{
NB_DECLARE_CLASS(TNamedObject)
public:
  explicit TNamedObject(const UnicodeString & AName);
  explicit TNamedObject() : TPersistent(), FHidden(false) {}
  virtual ~TNamedObject() {}

  bool GetHidden() const { return FHidden; }
  UnicodeString GetName() const { return FName; }
  void SetName(const UnicodeString & Value);
  bool IsSameName(const UnicodeString & Name) const;
  virtual intptr_t Compare(const TNamedObject * Other) const;
  void MakeUniqueIn(TNamedObjectList * List);

private:
  UnicodeString FName;
  bool FHidden;
};

class TNamedObjectList : public TObjectList
{
public:

public:
  // static bool IsHidden(TNamedObject * Object);

  bool AutoSort;

  TNamedObjectList();

  virtual void Notify(void * Ptr, TListNotification Action);
  void AlphaSort();
  virtual const TNamedObject * AtObject(intptr_t Index) const;
  virtual TNamedObject * AtObject(intptr_t Index);
  const TNamedObject * FindByName(const UnicodeString & Name) const;
  TNamedObject * FindByName(const UnicodeString & Name);
/*
  __property int Count = { read = GetCount, write = SetCount };
  __property int CountIncludingHidden = { read = GetCountIncludingHidden };
*/
  intptr_t GetCount() const;
  void SetCount(intptr_t Value);
  intptr_t GetCountIncludingHidden() const;
  //intptr_t GetHiddenCount() const { return FHiddenCount; }
  //void SetHiddenCount(intptr_t Value) { FHiddenCount = Value; }

protected:
  intptr_t FHiddenCount;
  void Recount();

private:
};

int NamedObjectSortProc(void * Item1, void * Item2);

