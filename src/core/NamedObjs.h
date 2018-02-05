
#pragma once

#include <System.hpp>
#include <contnrs.hpp>

#define CONST_HIDDEN_PREFIX L"_!_"
//---------------------------------------------------------------------------
class TNamedObjectList;
class NB_CORE_EXPORT TNamedObject : public TPersistent
{
public:
  static inline bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TNamedObject); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TNamedObject) || TPersistent::is(Kind); }
public:
  __property UnicodeString Name = { read = FName, write = SetName };
  __property bool Hidden = { read = FHidden };

  UnicodeString GetName() const { return FName; }
  void SetName(const UnicodeString Value);
  bool GetHidden() const { return FHidden; }

  explicit TNamedObject() : TPersistent(OBJECT_CLASS_TNamedObject), FHidden(false) {}
  explicit TNamedObject(TObjectClassId Kind) : TPersistent(Kind), FHidden(false) {}
  explicit TNamedObject(TObjectClassId Kind, const UnicodeString AName);
  virtual ~TNamedObject() {}

  bool IsSameName(const UnicodeString AName) const;
  virtual intptr_t Compare(const TNamedObject *Other) const;
  void MakeUniqueIn(TNamedObjectList *List);
private:
  UnicodeString FName;
  bool FHidden;

  __removed void SetName(const UnicodeString value);
};
//---------------------------------------------------------------------------
class NB_CORE_EXPORT TNamedObjectList : public TObjectList
{
public:
  static inline bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TNamedObjectList); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TNamedObjectList) || TObjectList::is(Kind); }
public:
  intptr_t GetCount() const;
  intptr_t GetCountIncludingHidden() const;
  virtual void Notify(void *Ptr, TListNotification Action) override;
  void SetCount(intptr_t Value);
protected:
  intptr_t FHiddenCount;
  bool FAutoSort;
  bool FControlledAdd;
  void Recount();
public:
  static const UnicodeString HiddenPrefix;

  bool GetAutoSort() const { return FAutoSort; }
  void SetAutoSort(bool Value) { FAutoSort = Value; }

  explicit TNamedObjectList(TObjectClassId Kind = OBJECT_CLASS_TNamedObjectList);
  void AlphaSort();
  intptr_t Add(TObject *AObject);
  virtual const TNamedObject * AtObject(intptr_t Index) const;
  virtual TNamedObject * AtObject(intptr_t Index);
  const TNamedObject * FindByName(const UnicodeString AName) const;
  TNamedObject * FindByName(const UnicodeString Name);
  __property int Count = { read = GetCount, write = SetCount };
  __property int CountIncludingHidden = { read = GetCountIncludingHidden };
};
//---------------------------------------------------------------------------
int NamedObjectSortProc(void *Item1, void *Item2);
//---------------------------------------------------------------------------
