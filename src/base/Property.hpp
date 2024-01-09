#pragma once

#include <type_traits>
#include <nbglobals.h>
#include <FastDelegate.h>

/// Borland CBuilder __property emulation
/// 40 bytes using fu2::function
/// 16 bytes using FastDelegate
template <typename T>
class ROProperty
{
CUSTOM_MEM_ALLOCATION_IMPL
using ValueType = std::conditional_t<std::is_trivially_copyable_v<T>, T, const T&>;
using TGetter = fastdelegate::FastDelegate0<T>;
private:
  TGetter _getter;
public:
  ROProperty() = delete;
  explicit ROProperty(TGetter && Getter) noexcept :
    _getter(std::move(Getter))
  {
    Expects(!_getter.empty());
  }
  ROProperty(const ROProperty &) = default;
  ROProperty(ROProperty &&) noexcept = default;
  ROProperty & operator =(const ROProperty &) = default;
  ROProperty & operator =(ROProperty &&) noexcept = default;

  constexpr T operator()() const
  {
    Expects(_getter);
    return _getter();
  }

  constexpr operator T() const
  {
    Expects(_getter);
    return _getter();
  }

  constexpr T operator ->() const
  {
    return _getter();
  }
  // constexpr decltype(auto) operator *() const { return _getter(); }
  constexpr T operator *() const { return _getter(); }
  constexpr bool operator ==(ValueType Value) const
  {
    Expects(_getter);
    return _getter() == Value;
  }

  constexpr bool operator !=(ValueType rhs)
  {
    Expects(_getter);
    return _getter() != rhs;
  }
};

template <typename T>
class ROIndexedProperty
{
CUSTOM_MEM_ALLOCATION_IMPL
using TGetter = fastdelegate::FastDelegate1<T, int32_t>;
private:
  TGetter _getter;

public:
  ROIndexedProperty() = delete;
  explicit ROIndexedProperty(TGetter && Getter) noexcept :
    _getter(std::move(Getter))
  {
    Expects(!_getter.empty());
  }
  ROIndexedProperty(const ROIndexedProperty &) = default;
  ROIndexedProperty(ROIndexedProperty &&) noexcept = default;
  ROIndexedProperty & operator =(const ROIndexedProperty &) = default;
  ROIndexedProperty & operator =(ROIndexedProperty &&) noexcept = default;
  constexpr T operator [](int32_t Index)
  {
    Expects(_getter);
    return _getter(Index);
  }
};

template <typename T>
class ROProperty2 //8 bytes
{
CUSTOM_MEM_ALLOCATION_IMPL
private:
  const T * _value{nullptr};
public:
  ROProperty2() = delete;
  explicit ROProperty2(const T * Value) noexcept :
    _value(Value)
  {
    Expects(_value != nullptr);
  }
  ROProperty2(const ROProperty2 &) = default;
  ROProperty2(ROProperty2 &&) noexcept = default;
  ROProperty2 & operator =(const ROProperty2 &) = default;
  ROProperty2 & operator =(ROProperty2 &&) noexcept = default;

  constexpr T operator ()() const
  {
    Expects(_value);
    return *_value;
  }

  constexpr operator T() const
  {
    Expects(_value);
    return *_value;
  }

  constexpr T operator ->() const
  {
    Expects(_value);
    return _value();
  }

  constexpr T operator ->()
  {
    Expects(_value);
    return *_value;
  }

  // constexpr decltype(auto) operator*() const { return *_value; }
  constexpr T operator *() const { return *_value; }

  friend bool constexpr inline operator ==(const ROProperty2 & lhs, const T & rhs)
  {
    Expects(lhs._value);
    return *lhs._value == rhs;
  }

  friend bool constexpr inline operator !=(ROProperty2 & lhs, const T & rhs)
  {
    Expects(lhs._value);
    return *lhs._value != rhs;
  }
};

// property with getter and const setter for complex value
// 80 bytes using fu2::function
// 32 bytes using FastDelegate
template <typename T>
class RWProperty : public ROProperty<T>
{
CUSTOM_MEM_ALLOCATION_IMPL
using ValueType = ROProperty<T>::ValueType;
using TSetter = fastdelegate::FastDelegate1<void, ValueType>;
private:
  TSetter _setter;
public:
  RWProperty() = delete;
  explicit RWProperty(TGetter && Getter, TSetter && Setter) noexcept :
    ROProperty<T>(std::move(Getter)),
    _setter(std::move(Setter))
  {
    Expects(!_setter.empty());
  }
  RWProperty(const RWProperty &) = default;
  RWProperty(RWProperty &&) noexcept = default;
  RWProperty & operator =(const RWProperty &) = default;
  RWProperty & operator =(RWProperty &&) noexcept = default;

  using ROProperty<T>::operator();
  void operator()(ValueType Value)
  {
    Expects(_setter);
    _setter(Value);
  }
  void operator =(ValueType Value)
  {
    Expects(_setter);
    _setter(Value);
  }
};

// simple property
template <typename T>
class RWProperty2
{
CUSTOM_MEM_ALLOCATION_IMPL
using ValueType = std::conditional_t<std::is_trivially_copyable_v<T>, T, const T&>;
private:
  T * _value{nullptr};
public:
  RWProperty2() = delete;
  explicit RWProperty2(T * Value) noexcept :
    _value(Value)
  {
    Expects(_value != nullptr);
  }
  RWProperty2(const RWProperty2 &) = default;
  RWProperty2(RWProperty2 &&) noexcept = default;
  RWProperty2 & operator =(const RWProperty2 &) = default;
  RWProperty2 & operator =(RWProperty2 &&) noexcept = default;

  constexpr T operator ()() const
  {
    Expects(_value);
    return *_value;
  }

  constexpr operator T() const
  {
    Expects(_value);
    return *_value;
  }

  constexpr T operator ->() const
  {
    Expects(_value);
    return _value();
  }

  constexpr T operator ->()
  {
    Expects(_value);
    return *_value;
  }
  // constexpr decltype(auto) operator*() const { return *_value; }
  constexpr T operator *() const { return *_value; }

  void operator()(ValueType Value)
  {
    Expects(_value);
    *_value = Value;
  }

  RWProperty2 & operator =(ValueType Value)
  {
    Expects(_value);
    *_value = Value;
    return *this;
  }

  friend bool constexpr inline operator ==(const RWProperty2 & lhs, ValueType rhs)
  {
    Expects(lhs._value);
    return *lhs._value == rhs;
  }

  friend bool constexpr inline operator !=(RWProperty2 & lhs, ValueType rhs)
  {
    Expects(lhs._value);
    return *lhs._value != rhs;
  }
};

template <typename T>
class RWPropertySimple
{
CUSTOM_MEM_ALLOCATION_IMPL
using ValueType = std::conditional_t<std::is_trivially_copyable_v<T>, T, const T&>;
using TSetter = fastdelegate::FastDelegate1<void, ValueType>;
private:
  T * _value{nullptr};
  TSetter _setter;
public:
  RWPropertySimple() = delete;
  explicit RWPropertySimple(T * Value, TSetter && Setter) noexcept :
    _value(Value),
    _setter(std::move(Setter))
  {
    Expects(_value != nullptr);
    Expects(!_setter.empty());
  }
  RWPropertySimple(const RWPropertySimple &) = default;
  RWPropertySimple(RWPropertySimple &&) noexcept = default;
  RWPropertySimple & operator =(const RWPropertySimple &) = default;
  RWPropertySimple & operator =(RWPropertySimple &&) noexcept = default;
  constexpr T operator ()() const
  {
    Expects(_value);
    return *_value;
  }
  constexpr operator T() const
  {
    Expects(_value);
    return *_value;
  }
  constexpr T operator ->() const
  {
    Expects(_value);
    return _value();
  }
  constexpr T operator ->()
  {
    Expects(_value);
    return *_value;
  }

  // constexpr decltype(auto) operator *() const { return *_value; }
  constexpr T operator *() const { return *_value; }

  void operator ()(ValueType Value)
  {
    Expects(_setter);
    _setter(Value);
  }
  void operator =(ValueType Value)
  {
    Expects(_setter);
    _setter(Value);
  }

  friend bool constexpr inline operator ==(const RWPropertySimple & lhs, ValueType rhs)
  {
    Expects(lhs._value);
    return *lhs._value == rhs;
  }

  friend bool constexpr inline operator !=(const RWPropertySimple & lhs, ValueType rhs)
  {
    Expects(lhs._value);
    return *lhs._value != rhs;
  }

};

//template<int s> struct CheckSizeT;
//CheckSizeT<sizeof(ROProperty<int>)> checkSize;
//template<int s> struct CheckSizeT;
//CheckSizeT<sizeof(ROPropertySimple<double>)> checkSize;
