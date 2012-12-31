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
  bool GetHidden() const { return FHidden; }
  UnicodeString GetName() const { return FName; }
  void SetName(const UnicodeString & Value);
  explicit TNamedObject() : TPersistent(), FHidden(false) {};
  Integer CompareName(UnicodeString aName, Boolean CaseSensitive = False);
  explicit TNamedObject(UnicodeString aName);
  void MakeUniqueIn(TNamedObjectList * List);
private:
  UnicodeString FName;
  bool FHidden;
};
//---------------------------------------------------------------------------
class TNamedObjectList : public TObjectList
{
private:
  intptr_t FHiddenCount;
public:
  intptr_t GetCount();
  void SetCount(intptr_t Value);
protected:
  void Recount();
public:
  static const UnicodeString HiddenPrefix;
  static bool IsHidden(TNamedObject * Object);

  bool AutoSort;

  TNamedObjectList();

  virtual void Notify(void *Ptr, TListNotification Action);
  void AlphaSort();
  virtual TNamedObject * AtObject(intptr_t Index);
  TNamedObject * FindByName(UnicodeString Name, Boolean CaseSensitive = False);
  intptr_t GetHiddenCount() { return FHiddenCount; }
  void SetHiddenCount(intptr_t Value) { FHiddenCount = Value; }
};
//---------------------------------------------------------------------------
int NamedObjectSortProc(void * Item1, void * Item2);
//---------------------------------------------------------------------------
#endif
