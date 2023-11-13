
#pragma once

#include <System.hpp>
#include <contnrs.hpp>

#define CONST_HIDDEN_PREFIX L"_!_"

class TNamedObjectList;
NB_DEFINE_CLASS_ID(TNamedObject);
class NB_CORE_EXPORT TNamedObject : public TPersistent
{
public:
  static bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TNamedObject); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TNamedObject) || TPersistent::is(Kind); }
public:
  __property UnicodeString Name = { read = FName, write = SetName };
  RWProperty<UnicodeString> Name{nb::bind(&TNamedObject::GetName, this), nb::bind(&TNamedObject::SetName, this)};
  __property bool Hidden = { read = FHidden };
  const bool& Hidden{FHidden};
  explicit TNamedObject() noexcept : TPersistent(OBJECT_CLASS_TNamedObject), FHidden(false) {}

  UnicodeString GetName() const { return FName; }
  void SetName(const UnicodeString & Value);
  bool GetHidden() const { return FHidden; }

  explicit TNamedObject(TObjectClassId Kind) noexcept : TPersistent(Kind), FHidden(false) {}
  explicit TNamedObject(TObjectClassId Kind, const UnicodeString & AName) noexcept;
  virtual ~TNamedObject() = default;

  bool IsSameName(const UnicodeString & AName) const;
  virtual int32_t Compare(const TNamedObject * Other) const;
  void MakeUniqueIn(TNamedObjectList * List);
private:
  UnicodeString FName;
  bool FHidden{false};

  __removed void SetName(const UnicodeString & value);
};

NB_DEFINE_CLASS_ID(TNamedObjectList);
class NB_CORE_EXPORT TNamedObjectList : public TObjectList
{
public:
  static bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TNamedObjectList); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TNamedObjectList) || TObjectList::is(Kind); }
public:
  int32_t GetCount() const;
  int32_t GetCountIncludingHidden() const;
  virtual void Notify(void * Ptr, TListNotification Action) override;
protected:
  int32_t FHiddenCount{0};
  bool FAutoSort{true};
  bool FControlledAdd{false};
  void Recount();
public:
  static const UnicodeString HiddenPrefix;

  bool GetAutoSort() const { return FAutoSort; }
  void SetAutoSort(bool Value) { FAutoSort = Value; }

  explicit TNamedObjectList(TObjectClassId Kind = OBJECT_CLASS_TNamedObjectList) noexcept;
  void AlphaSort();
  int32_t Add(TObject * AObject);
  virtual const TNamedObject * AtObject(int32_t Index) const;
  virtual TNamedObject * AtObject(int32_t Index);
  const TNamedObject * FindByName(const UnicodeString & AName) const;
  TNamedObject * FindByName(const UnicodeString & Name);
  __property int32_t Count = { read = GetCount };
  __property int32_t CountIncludingHidden = { read = GetCountIncludingHidden };
};

int32_t NamedObjectSortProc(const void * Item1, const void * Item2);

