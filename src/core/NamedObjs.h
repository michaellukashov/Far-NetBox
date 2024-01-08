
#pragma once

#include <System.hpp>
#include <contnrs.hpp>

constexpr const wchar_t * CONST_HIDDEN_PREFIX = L"_!_";

class TNamedObjectList;
class NB_CORE_EXPORT TNamedObject : public TPersistent
{
public:
  static bool classof(const TObject * Obj) { return Obj->is(OBJECT_CLASS_TNamedObject); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TNamedObject) || TPersistent::is(Kind); }
public:
  __property UnicodeString Name = { read = FName, write = SetName };
  RWProperty<UnicodeString> Name{nb::bind(&TNamedObject::GetName, this), nb::bind(&TNamedObject::SetName, this)};
  __property bool Hidden = { read = FHidden };
  const bool& Hidden{FHidden};
  explicit TNamedObject() noexcept : TNamedObject(OBJECT_CLASS_TNamedObject) {}

  UnicodeString GetName() const { return FName; }
  void SetName(const UnicodeString & Value);
  bool GetHidden() const { return FHidden; }

  explicit TNamedObject(TObjectClassId Kind) noexcept : TPersistent(Kind) {}
  explicit TNamedObject(TObjectClassId Kind, const UnicodeString & AName) noexcept;
  virtual ~TNamedObject() override = default;

  bool IsSameName(const UnicodeString & AName) const;
  virtual int32_t Compare(const TNamedObject * Other) const;
  void MakeUniqueIn(TNamedObjectList * List);
private:
  UnicodeString FName;
  bool FHidden{false};
};

class NB_CORE_EXPORT TNamedObjectList : public TObjectList
{
public:
  static bool classof(const TObject * Obj) { return Obj->is(OBJECT_CLASS_TNamedObjectList); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TNamedObjectList) || TObjectList::is(Kind); }
public:
  virtual int32_t GetCount() const override;
  virtual void SetCount(int32_t Value) override;
  int32_t GetCountIncludingHidden() const;
  TNamedObject * GetSortObject(const UnicodeString & Name, int32_t & Position);
  virtual void Notify(TObject * Ptr, TListNotification Action) override;
protected:
  void Recount();

  int32_t FHiddenCount{0};
  bool FAutoSort{true};
  bool FControlledAdd{false};
public:
  static constexpr const wchar_t * HiddenPrefix = CONST_HIDDEN_PREFIX;

  bool GetAutoSort() const { return FAutoSort; }
  void SetAutoSort(bool Value) { FAutoSort = Value; }

  explicit TNamedObjectList(TObjectClassId Kind = OBJECT_CLASS_TNamedObjectList) noexcept;
  void AlphaSort();
  virtual int32_t Add(TObject * AObject) override;
  virtual const TNamedObject * AtObject(int32_t Index) const;
  virtual TNamedObject * AtObject(int32_t Index);
  const TNamedObject * FindByName(const UnicodeString & AName) const;
  TNamedObject * FindByName(const UnicodeString & AName);
  __property int32_t Count = { read = GetCount };
  __property int32_t CountIncludingHidden = { read = GetCountIncludingHidden };
};

int32_t NamedObjectSortProc(const TObject * Item1, const TObject * Item2);
