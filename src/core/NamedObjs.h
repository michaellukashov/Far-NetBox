
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

  bool __fastcall IsSameName(const UnicodeString AName) const;
  virtual intptr_t __fastcall Compare(const TNamedObject *Other) const;
  void __fastcall MakeUniqueIn(TNamedObjectList *List);
private:
  UnicodeString FName;
  bool FHidden;

  __removed void __fastcall SetName(UnicodeString value);
};
//---------------------------------------------------------------------------
class NB_CORE_EXPORT TNamedObjectList : public TObjectList
{
public:
  static inline bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TNamedObjectList); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TNamedObjectList) || TObjectList::is(Kind); }
public:
  intptr_t __fastcall GetCount() const;
  intptr_t __fastcall GetCountIncludingHidden() const;
  virtual void __fastcall Notify(void *Ptr, TListNotification Action) override;
  void __fastcall SetCount(intptr_t Value);
protected:
  intptr_t FHiddenCount;
  bool FAutoSort;
  bool FControlledAdd;
  void __fastcall Recount();
public:
  static const UnicodeString HiddenPrefix;

  bool GetAutoSort() const { return FAutoSort; }
  void SetAutoSort(bool Value) { FAutoSort = Value; }

  explicit __fastcall TNamedObjectList(TObjectClassId Kind = OBJECT_CLASS_TNamedObjectList);
  void __fastcall AlphaSort();
  intptr_t __fastcall Add(TObject *AObject);
  virtual const TNamedObject * __fastcall AtObject(intptr_t Index) const;
  virtual TNamedObject * __fastcall AtObject(intptr_t Index);
  const TNamedObject * __fastcall FindByName(const UnicodeString AName) const;
  TNamedObject * __fastcall FindByName(UnicodeString Name);
  __property int Count = { read = GetCount, write = SetCount };
  __property int CountIncludingHidden = { read = GetCountIncludingHidden };
};
//---------------------------------------------------------------------------
int NamedObjectSortProc(void *Item1, void *Item2);
//---------------------------------------------------------------------------
