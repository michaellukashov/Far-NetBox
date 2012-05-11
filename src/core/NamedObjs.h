//---------------------------------------------------------------------------
#ifndef NamedObjsH
#define NamedObjsH

#include "Classes.h"
#ifndef _MSC_VER
#include <system.hpp>
#include <contnrs.hpp>
#endif
//---------------------------------------------------------------------------
class TNamedObjectList;
class TNamedObject : public TPersistent
{
public:
#ifndef _MSC_VER
  __property UnicodeString Name = { read = FName, write = SetName };
  __property bool Hidden = { read = FHidden };
#else
  bool __fastcall GetHidden() { return FHidden; }
  UnicodeString __fastcall GetName() { return FName; }
#endif
  /* __fastcall */ TNamedObject() : TPersistent() {};
  Integer __fastcall CompareName(UnicodeString aName, Boolean CaseSensitive = False);
  explicit /* __fastcall */ TNamedObject(UnicodeString aName);
  void __fastcall MakeUniqueIn(TNamedObjectList * List);
private:
  UnicodeString FName;
  bool FHidden;

public:
  void __fastcall SetName(UnicodeString value);
};
//---------------------------------------------------------------------------
class TNamedObjectList : public TObjectList
{
private:
  int FHiddenCount;
public:
  int __fastcall GetCount();
  virtual void __fastcall Notify(void *Ptr, TListNotification Action);
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
#ifndef _MSC_VER
  __property int Count = { read = GetCount, write = SetCount };
  __property int HiddenCount = { read = FHiddenCount, write = FHiddenCount };
#else
  int GetHiddenCount() { return FHiddenCount; }
  void SetHiddenCount(int value) { FHiddenCount = value; }
#endif
};
//---------------------------------------------------------------------------
int /* __fastcall */ NamedObjectSortProc(void * Item1, void * Item2);
//---------------------------------------------------------------------------
#endif
