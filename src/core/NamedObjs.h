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
  bool __fastcall GetHidden() const { return FHidden; }
  UnicodeString __fastcall GetName() const { return FName; }
  void __fastcall SetName(UnicodeString value);
  explicit /* __fastcall */ TNamedObject() : TPersistent(), FHidden(false) {};
  Integer __fastcall CompareName(UnicodeString aName, Boolean CaseSensitive = False);
  explicit /* __fastcall */ TNamedObject(UnicodeString aName);
  void __fastcall MakeUniqueIn(TNamedObjectList * List);
private:
  UnicodeString FName;
  bool FHidden;
};
//---------------------------------------------------------------------------
class TNamedObjectList : public TObjectList
{
private:
  int FHiddenCount;
  virtual void __fastcall Notify(void *Ptr, TListNotification Action);
public:
  int __fastcall GetCount();
  void __fastcall SetCount(int value);
protected:
  void __fastcall Recount();
public:
  static const UnicodeString HiddenPrefix;
  static bool IsHidden(TNamedObject * Object);

  bool AutoSort;

  /* __fastcall */ TNamedObjectList();

  void __fastcall AlphaSort();
  virtual TNamedObject * __fastcall AtObject(Integer Index);
  TNamedObject * __fastcall FindByName(UnicodeString Name, Boolean CaseSensitive = False);
  int __fastcall GetHiddenCount() { return FHiddenCount; }
  void __fastcall SetHiddenCount(int value) { FHiddenCount = value; }
};
//---------------------------------------------------------------------------
int /* __fastcall */ NamedObjectSortProc(void * Item1, void * Item2);
//---------------------------------------------------------------------------
#endif
