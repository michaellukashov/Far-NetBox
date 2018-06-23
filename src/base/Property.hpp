#pragma once

//#include <function2.hpp>
#include <nbglobals.h>
#include <FastDelegate.h>
//#include <FastDelegateBind.h>
#include <TransientFunction.h>

template <typename T>
class propertyBase
{
CUSTOM_MEM_ALLOCATION_IMPL
public:
  T dummy = T();
  T &obj  = dummy;

  explicit propertyBase(T &Value):
    obj(Value)
  {}
};

// Read/Write specialization
template <typename T, bool canWrite = true, bool isPod = true>
class Property : private propertyBase<T>
{
CUSTOM_MEM_ALLOCATION_IMPL
public:
  Property() = default;

  Property(const Property<T, true, true> &Value):
    propertyBase<T>(Value.obj)
  {}

  explicit Property(T &Value):
    propertyBase<T>(Value)
  {}

  T &operator=(const Property<T, true, true> &Value)
  {
    this->obj = Value.obj;
    return this->obj;
  }

  T &operator=(const T &Value)
  {
    this->obj = Value;
    return this->obj;
  }

  T &operator=(T &Value)
  {
    this->obj = Value;

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

  Property(const Property<T, false, true> &Value):
    propertyBase<T>(Value.obj)
  {}

  explicit Property(T &Value):
    propertyBase<T>(Value)
  {}

  T &operator=(const Property &Value)
  {
    this->obj = Value.obj;
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

  explicit Property(const T &Value):
    T(Value)
  {}
};

// Read only non-pod specialization
template <typename T>
class Property<T, false, false> : public T
{
  const T *const _obj = this;
public:
  using T::T;

  explicit Property(const T &Value):
    T(Value)
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

// 40 bytes using fu2::function
// 16 bytes using FastDelegate
template <typename T>
class ROProperty
{
CUSTOM_MEM_ALLOCATION_IMPL
private:
  typedef fastdelegate::FastDelegate0<T> TGetValueFunctor;
  TGetValueFunctor _getter;
public:
  ROProperty() = delete;
  explicit ROProperty(TGetValueFunctor& Getter) :
    _getter(Getter)
  {
    Expects(_getter != nullptr);
  }
  ROProperty(const ROProperty&) = default;
  ROProperty(ROProperty&&) noexcept = default;
  ROProperty& operator=(const ROProperty&) = default;
  ROProperty& operator=(ROProperty&&) noexcept = default;
  constexpr T operator()() const
  {
    return _getter();
  }
  constexpr operator T() const
  {
    return _getter();
  }
  constexpr const T operator->() const
  {
    return _getter();
  }
  T operator->()
  {
    return _getter();
  }
  constexpr decltype(auto) operator*() const { return *_getter(); }

  template <typename T1>
  friend bool inline operator==(const ROProperty &lhs, const T1 &rhs)
  {
    return lhs._getter() == rhs;
  }
  template <typename T1>
  friend bool inline operator==(const T1 &lhs, const ROProperty &rhs)
  {
    return rhs._getter() == lhs;
  }
  template <typename T1>
  friend bool inline operator!=(ROProperty &lhs, const T &rhs)
  {
    return lhs._getter() != rhs;
  }
  template <typename T1>
  friend bool inline operator!=(const T &lhs, ROProperty &rhs)
  {
    return rhs._getter() != lhs;
  }
};

template <typename T>
class RWProperty
{
CUSTOM_MEM_ALLOCATION_IMPL
private:
  typedef fastdelegate::FastDelegate0<T> TGetValueFunctor;
  typedef fastdelegate::FastDelegate1<void, T> TSetValueFunctor;
  TGetValueFunctor _getter;
  TSetValueFunctor _setter;
public:
  RWProperty() = delete;
  explicit RWProperty(TGetValueFunctor& Getter, TSetValueFunctor& Setter) :
    _getter(Getter),
    _setter(Setter)
  {
    Expects(_getter != nullptr);
    Expects(_setter != nullptr);
  }
  RWProperty(const T&) noexcept = delete;
  RWProperty(const RWProperty&) = default;
  RWProperty(RWProperty&&) noexcept = default;
  RWProperty& operator=(const RWProperty&) = default;
  RWProperty& operator=(RWProperty&&) noexcept = default;
  constexpr T operator()() const
  {
    return _getter();
  }
  constexpr operator T() const
  {
    return _getter();
  }
  /*operator T&() const
  {
    Expects(_getter);
    return _getter();
  }*/
  constexpr T operator->() const
  {
    return _getter();
  }
  constexpr decltype(auto) operator*() const { return *_getter(); }
  void operator()(const T& Value)
  {
    _setter(Value);
  }
  void operator=(const T& Value)
  {
    _setter(Value);
  }
  template <typename T1>
  friend bool inline operator==(const RWProperty& lhs, const T1& rhs)
  {
    return lhs._getter() == rhs;
  }
  template <typename T1>
  friend bool inline operator==(const T1& lhs, const RWProperty<T>& rhs)
  {
    return rhs._getter() == lhs;
  }
  template <typename T1>
  friend bool inline operator!=(const RWProperty<T>& lhs, const T1& rhs)
  {
    return lhs._getter() != rhs;
  }
  template <typename T1>
  friend bool inline operator!=(const T1& lhs, const RWProperty<T>& rhs)
  {
    return rhs._getter() != lhs;
  }
};
