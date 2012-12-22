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
  void __fastcall SetName(const UnicodeString & Value);
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
  intptr_t FHiddenCount;
public:
  intptr_t __fastcall GetCount();
  void __fastcall SetCount(intptr_t Value);
protected:
  void __fastcall Recount();
public:
  static const UnicodeString HiddenPrefix;
  static bool IsHidden(TNamedObject * Object);

  bool AutoSort;

  /* __fastcall */ TNamedObjectList();

  virtual void __fastcall Notify(void *Ptr, TListNotification Action);
  void __fastcall AlphaSort();
  virtual TNamedObject * __fastcall AtObject(intptr_t Index);
  TNamedObject * __fastcall FindByName(UnicodeString Name, Boolean CaseSensitive = False);
  intptr_t __fastcall GetHiddenCount() { return FHiddenCount; }
  void __fastcall SetHiddenCount(intptr_t Value) { FHiddenCount = Value; }
};
//---------------------------------------------------------------------------
int /* __fastcall */ NamedObjectSortProc(void * Item1, void * Item2);
//---------------------------------------------------------------------------
#endif
