#pragma once

#include <system.hpp>
#include <contnrs.hpp>

#define CONST_HIDDEN_PREFIX L"_!_"

class TNamedObjectList;
class TNamedObject : public TPersistent
{
public:
  static inline bool classof(const TObject * Obj)
  {
    return
      Obj->GetKind() == OBJECT_CLASS_TNamedObject ||
      Obj->GetKind() == OBJECT_CLASS_TSessionData;
  }
public:
  /*__property UnicodeString Name = { read = FName, write = SetName };
  __property bool Hidden = { read = FHidden };*/
  UnicodeString GetName() const { return FName; }
  void SetName(const UnicodeString & Value);
  bool GetHidden() const { return FHidden; }

  TNamedObject() : TPersistent(OBJECT_CLASS_TNamedObject), FHidden(false) {}
  explicit TNamedObject(TObjectClassId Kind) : TPersistent(Kind), FHidden(false) {}
  explicit TNamedObject(TObjectClassId Kind, const UnicodeString & AName);
  virtual ~TNamedObject() {}

  bool IsSameName(const UnicodeString & Name) const;
  virtual intptr_t Compare(const TNamedObject * Other) const;
  void MakeUniqueIn(TNamedObjectList * List);
private:
  UnicodeString FName;
  bool FHidden;
};

class TNamedObjectList : public TObjectList
{
public:
  static inline bool classof(const TObject * Obj)
  {
    return
      Obj->GetKind() == OBJECT_CLASS_TNamedObjectList ||
      Obj->GetKind() == OBJECT_CLASS_TStoredSessionList;
  }
public:
  intptr_t GetCount() const;
  intptr_t GetCountIncludingHidden() const;
  virtual void Notify(void * Ptr, TListNotification Action);
  void SetCount(intptr_t Value);
protected:
  intptr_t FHiddenCount;
  bool FControlledAdd;
  void Recount();

public:
  bool AutoSort;

  TNamedObjectList();
  explicit TNamedObjectList(TObjectClassId Kind);
  void AlphaSort();
  intptr_t Add(TObject * AObject);
  virtual const TNamedObject * AtObject(intptr_t Index) const;
  virtual TNamedObject * AtObject(intptr_t Index);
  const TNamedObject * FindByName(const UnicodeString & Name) const;
  TNamedObject * FindByName(const UnicodeString & Name);
  /*__property int Count = { read = GetCount, write = SetCount };
  __property int CountIncludingHidden = { read = GetCountIncludingHidden };*/
private:
};

int NamedObjectSortProc(void * Item1, void * Item2);

