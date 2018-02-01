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
