#pragma once

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

template <typename T,
          typename Object
          >
class ROProperty
{
CUSTOM_MEM_ALLOCATION_IMPL
private:
  Object *_obj;
  typedef T (Object::*GetterFunc)() const;
  GetterFunc _getterFunc;
public:
  explicit ROProperty(Object *Obj, GetterFunc Getter) :
    _obj(Obj),
    _getterFunc(Getter)
  {}
  /*T operator()() const
  {
    // return (_obj->*Getter)();
    // assert(_getter);
    return _getter();
  }*/
  operator T() const
  {
    // assert(_getterFunc);
    return (_obj->*_getterFunc)();
  }
  const T operator->() const
  {
    // assert(_getterFunc);
    return (_obj->*_getterFunc)();
  }
  T operator->()
  {
    // assert(_getterFunc);
    return (_obj->*_getterFunc)();
  }
};

template <typename T,
          typename Object
          >
class RWProperty
{
CUSTOM_MEM_ALLOCATION_IMPL
private:
  typedef T (Object::*GetterFunc)() const;
  typedef void (Object::*SetterFunc)(T);
  Object *_obj;
  GetterFunc _getter;
  SetterFunc _setter;
public:
  explicit RWProperty(Object *Obj, GetterFunc Getter, SetterFunc Setter):
    _obj(Obj),
    _getter(Getter),
    _setter(Setter)
  {}
  T operator()() const
  {
    // assert(_getter);
    return (_obj->*_getter)();
  }
  operator T() const
  {
    // assert(_getter);
    return (_obj->*_getter)();
  }
  /*void operator()(const T &value)
  {
    // assert(_setter);
    (*_setter)(value);
  }*/
  void operator=(T Value)
  {
    // assert(_setter);
    (_obj->*_setter)(Value);
  }
  T operator->() const
  {
    // assert(_getter);
    return (_obj->*_getter)();
  }
  bool operator==(T Value)
  {
    // assert(_setter);
    return (_obj->*_getter)() == Value;
  }
};
