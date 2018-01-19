#pragma once

template <typename T>
class propertyBase
{
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
          // T (Object::*Getter)()
          >
class ROProperty
{
//  Object *_obj;
  typedef T (Object::*Getter)();
  Getter *_getter;
public:
//  explicit ROProperty(Object *obj, Getter *getter):
//    _obj(obj),
  explicit ROProperty(Getter *getter):
    _getter(getter)
  {}
  /*T operator()() const
  {
    // return (_obj->*Getter)();
    // assert(_getter);
    return _getter();
  }*/
  operator T() const
  {
    // assert(_getter);
    return (*_getter)();
  }
};

template <typename T,
          typename Object
          >
class RWProperty
{
  typedef T (Object::*Getter)();
  typedef void (Object::*Setter)(const T &);
  Getter *_getter;
  Setter *_setter;
public:
//  explicit ROProperty(Object *obj, Getter *getter):
//    _obj(obj),
  explicit RWProperty(Getter *getter, Setter *setter):
    _getter(getter),
    _setter(setter)
  {}
  /*T operator()() const
  {
    // assert(_getter);
    return (*_getter)();
  }*/
  operator T() const
  {
    // assert(_getter);
    return (*_getter)();
  }
  /*void operator()(const T &value)
  {
    // assert(_setter);
    (*_setter)(value);
  }*/
  void operator=(const T &value)
  {
    // assert(_setter);
    (*_setter)(value);
  }
};
