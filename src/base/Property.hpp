#pragma once

#include <assert.h>
#include <FastDelegate.h>
#include <FastDelegateBind.h>
#include <nbglobals.h>

template <typename T>
class ROProperty
{
CUSTOM_MEM_ALLOCATION_IMPL
private:
  typedef fastdelegate::FastDelegate0<T> TGetValueDelegate;
  TGetValueDelegate _getter;
//  ROProperty() = delete;
public:
//  ROProperty(const ROProperty &) = default;
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
  friend bool inline operator==(const ROProperty &lhs, const ROProperty &rhs)
  {
    assert(lhs._getter);
    assert(rhs._getter);
    return lhs._getter() == rhs._getter();
  }
//  bool operator==(const T &Value) const
//  {
//    assert(_getter);
//    return _getter() == Value;
//  }
  friend bool inline operator==(const ROProperty &lhs, const T &rhs)
  {
    assert(lhs._getter);
    return lhs._getter() == rhs;
  }
  friend bool inline operator!=(ROProperty &lhs, const T &rhs)
  {
    assert(lhs._getter);
    return lhs._getter() != rhs;
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
//  RWProperty() = delete;
public:
//  RWProperty(const RWProperty &) = default;
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
  T operator->() const
  {
    assert(_getter);
    return _getter();
  }
  void operator()(const T &Value)
  {
    assert(_setter);
    _setter(Value);
  }
  void operator=(const T &Value)
  {
    assert(_setter);
    _setter(Value);
  }
  bool operator==(const T &Value) const
  {
    assert(_getter);
    return _getter() == Value;
  }
//  friend bool inline operator==(const RWProperty &lhs, const T &rhs)
//  {
//    assert(lhs._getter);
//    return lhs._getter() == rhs;
//  }
  friend bool inline operator!=(RWProperty &lhs, const T &rhs)
  {
    assert(lhs._getter);
    return lhs._getter() != rhs;
  }
};
