//---------------------------------------------------------------------------
#ifndef NamedObjsH
#define NamedObjsH

// #include <system.hpp>
// #include <contnrs.hpp>
#include "Classes.h"

//---------------------------------------------------------------------------
class TNamedObjectList;
class TNamedObject : public TPersistent
{
public:
  wstring Name;
  TNamedObject(): TPersistent() {};
  int CompareName(wstring aName, bool CaseSensitive = false);
  TNamedObject(wstring aName): TPersistent(), Name(aName) {}
  void MakeUniqueIn(TNamedObjectList * List);
};
//---------------------------------------------------------------------------
enum TListNotification
{
  lnAdded,
  lnExtracted,
  lnDeleted,
};

//---------------------------------------------------------------------------
class TNamedObjectList : public TObjectList
{
private:
  int FHiddenCount;
  virtual void Notify(void *Ptr, TListNotification Action);
protected:
  void Recount();
public:
  static const wstring HiddenPrefix;
  static bool IsHidden(TNamedObject * Object);

  bool AutoSort;

  TNamedObjectList();
  void AlphaSort();
  virtual TNamedObject * AtObject(int Index);
  TNamedObject * FindByName(wstring Name, bool CaseSensitive = false);
  // __property int Count = { read = GetCount, write = SetCount };
  int GetCount();
  void SetCount(int value);
  // __property int HiddenCount = { read = FHiddenCount, write = FHiddenCount };
  int GetHiddenCount() { return FHiddenCount; }
  void SetHiddenCount(int value) { FHiddenCount = value; }
};
//---------------------------------------------------------------------------
int NamedObjectSortProc(void * Item1, void * Item2);
//---------------------------------------------------------------------------
#endif
