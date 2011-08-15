//---------------------------------------------------------------------------
#ifndef NamedObjsH
#define NamedObjsH

#include <system.hpp>
#include <contnrs.hpp>
//---------------------------------------------------------------------------
class TNamedObjectList;
class TNamedObject : public TPersistent
{
public:
  wstring Name;
  TNamedObject(): TPersistent() {};
  Integer CompareName(wstring aName, Boolean CaseSensitive = False);
  TNamedObject(wstring aName): TPersistent(), Name(aName) {}
  void MakeUniqueIn(TNamedObjectList * List);
};
//---------------------------------------------------------------------------
class TNamedObjectList : public TObjectList
{
private:
  int FHiddenCount;
  int GetCount();
  virtual void Notify(void *Ptr, TListNotification Action);
  void SetCount(int value);
protected:
  void Recount();
public:
  static const wstring HiddenPrefix;
  static bool IsHidden(TNamedObject * Object);

  bool AutoSort;

  TNamedObjectList();
  void AlphaSort();
  virtual TNamedObject * AtObject(Integer Index);
  TNamedObject * FindByName(wstring Name, Boolean CaseSensitive = False);
  __property int Count = { read = GetCount, write = SetCount };
  __property int HiddenCount = { read = FHiddenCount, write = FHiddenCount };
};
//---------------------------------------------------------------------------
int NamedObjectSortProc(void * Item1, void * Item2);
//---------------------------------------------------------------------------
#endif
