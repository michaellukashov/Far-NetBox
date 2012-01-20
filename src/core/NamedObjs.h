//---------------------------------------------------------------------------
#ifndef NamedObjsH
#define NamedObjsH

#include "Classes.h"

//---------------------------------------------------------------------------
class TNamedObjectList;
class TNamedObject : public TPersistent
{
public:
  TNamedObject(const std::wstring aName);
  // __property AnsiString Name = { read = FName, write = SetName };
  // __property bool Hidden = { read = FHidden };
  bool GetHidden() { return FHidden; }
  std::wstring GetName() { return FName; }
  void SetName(const std::wstring value);
  TNamedObject(): TPersistent() {};
  int CompareName(const std::wstring aName, bool CaseSensitive = false);
  void MakeUniqueIn(TNamedObjectList * List);
private:
  std::wstring FName;
  bool FHidden;
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
  static const std::wstring HiddenPrefix;
  static bool IsHidden(TNamedObject * Object);

  bool AutoSort;

  TNamedObjectList();
  void AlphaSort();
  virtual TNamedObject * AtObject(int Index);
  TNamedObject * FindByName(const std::wstring Name, bool CaseSensitive = false);
  // __property int Count = { read = GetCount, write = SetCount };
  size_t GetCount();
  void SetCount(size_t value);
  // __property int HiddenCount = { read = FHiddenCount, write = FHiddenCount };
  int GetHiddenCount() { return FHiddenCount; }
  void SetHiddenCount(int value) { FHiddenCount = value; }
};
//---------------------------------------------------------------------------
int NamedObjectSortProc(void * Item1, void * Item2);
//---------------------------------------------------------------------------
#endif
