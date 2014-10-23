
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
  Integer CompareName(const UnicodeString & AName, Boolean CaseSensitive = False);
  void MakeUniqueIn(TNamedObjectList * List);

private:
  UnicodeString FName;
  bool FHidden;
};

class TNamedObjectList : public TObjectList
{
public:
  intptr_t GetCount() const;
  void SetCount(intptr_t Value);
public:
  static bool IsHidden(TNamedObject * Object);

  bool AutoSort;

  TNamedObjectList();

  virtual void Notify(void * Ptr, TListNotification Action);
  void AlphaSort();
  virtual TNamedObject * AtObject(intptr_t Index);
  virtual const TNamedObject * AtObject(intptr_t Index) const;
  TNamedObject * FindByName(const UnicodeString & Name, Boolean CaseSensitive = False) const;
  TNamedObject * FindByName(const UnicodeString & Name, Boolean CaseSensitive = False);
  intptr_t GetHiddenCount() const { return FHiddenCount; }
  void SetHiddenCount(intptr_t Value) { FHiddenCount = Value; }

protected:
  void Recount();

private:
  intptr_t FHiddenCount;
};

int NamedObjectSortProc(void * Item1, void * Item2);

