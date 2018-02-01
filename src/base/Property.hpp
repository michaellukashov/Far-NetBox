#pragma once

#include <assert.h>
#include <FastDelegate.h>
#include <FastDelegateBind.h>
#include <nbglobals.h>

template <typename T>
class propertyBase
{
CUSTOM_MEM_ALLOCATION_IMPL
public:
  T dummy = T();
  T &obj  = dummy;

  propertyBase(T &value):
    obj(value)
  {}
};

// Read/Write specialization
template <typename T, bool canWrite = true, bool isPod = true>
class Property : private propertyBase<T>
{
CUSTOM_MEM_ALLOCATION_IMPL
public:
  Property() = default;

  Property(const Property<T, true, true> &value):
    propertyBase<T>(value.obj)
  {}

  /*Property(Property<T, true, true>& value):
      propertyBase<T>(value.obj)
  {}*/

  Property(T &value):
    propertyBase<T>(value)
  {}

  T &operator=(const Property<T, true, true> &value)
  {
    this->obj = value.obj;
    return this->obj;
  }

  T &operator=(const T &value)
  {
    this->obj = value;
    return this->obj;
  }

  T &operator=(T &value)
  {
    this->obj = value;

    return this->obj;
  }

  operator T &() const
  {
    return this->obj;
  }

};

// Read only specialization
template <typename T>
class Property<T, false, true> : private propertyBase<T>
{
CUSTOM_MEM_ALLOCATION_IMPL
public:
  Property() = default;

  Property(const Property<T, false, true> &value):
    propertyBase<T>(value.obj)
  {}

  Property(T &value):
    propertyBase<T>(value)
  {}

  T &operator=(const Property &value)
  {
    this->obj = value.obj;
    return this->obj;
  }

  operator T() const
  {
    return this->obj;
  }

};

// Read/Write non-pod specialization
template <typename T>
class Property<T, true, false> : public T
{
CUSTOM_MEM_ALLOCATION_IMPL
public:
  using T::T;

  Property(const T &value):
    T(value)
  {}
};

// Read only non-pod specialization
template <typename T>
class Property<T, false, false> : public T
{
  const T *const _obj = this;
public:
  using T::T;

  Property(const T &value):
    T(value)
  {}

  const T *const operator->() const
  {
    return this->_obj;
  }
};

template <typename T>
using roProperty = Property<T, false, std::is_pod<T>::value>;

template <typename T>
using rwProperty = Property<T, true, std::is_pod<T>::value>;

template <typename T>
class ROProperty
{
CUSTOM_MEM_ALLOCATION_IMPL
private:
  typedef fastdelegate::FastDelegate0<T> TGetValueDelegate;
  TGetValueDelegate _getter;
public:
  explicit ROProperty(TGetValueDelegate Getter) :
    _getter(Getter)
  {}
  T operator()() const
  {
    assert(_getter);
    return _getter();
  }
  operator T() const
  {
    assert(_getter);
    return _getter();
  }
  const T operator->() const
  {
    assert(_getter);
    return _getter();
  }
  T operator->()
  {
    assert(_getter);
    return _getter();
  }
  bool operator==(const T Value) const
  {
    assert(_getter);
    return _getter() == Value;
  }
};

template <typename T>
class RWProperty
{
CUSTOM_MEM_ALLOCATION_IMPL
private:
  typedef fastdelegate::FastDelegate0<T> TGetValueDelegate;
  typedef fastdelegate::FastDelegate1<void, T> TSetValueDelegate;
  TGetValueDelegate _getter;
  TSetValueDelegate _setter;
public:
  explicit RWProperty(TGetValueDelegate Getter, TSetValueDelegate Setter) :
    _getter(Getter),
    _setter(Setter)
  {}
  T operator()() const
  {
    assert(_getter);
    return _getter();
  }
  operator T() const
  {
    assert(_getter);
    return _getter();
  }
  void operator()(const T &Value)
  {
    assert(_setter);
    _setter(Value);
  }
  void operator=(const T Value)
  {
    assert(_setter);
    _setter(Value);
  }
  T operator->() const
  {
    assert(_getter);
    return _getter();
  }
  bool operator==(const T Value) const
  {
    // assert(!"false");
    assert(_getter);
    // assert(!"false2");
    return _getter() == Value;
  }
};
