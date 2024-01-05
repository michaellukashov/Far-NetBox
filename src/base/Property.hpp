#pragma once

#include <type_traits>
#include <nbglobals.h>
#include <FastDelegate.h>

// 40 bytes using fu2::function
// 16 bytes using FastDelegate
template <typename T>
class ROProperty
{
CUSTOM_MEM_ALLOCATION_IMPL
using ValueType = typename std::conditional<std::is_trivially_copyable<T>::value, T, const T&>::type;
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

  constexpr T operator->() const
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
  friend bool inline operator ==(const ROProperty & lhs, const ROProperty & rhs)
  {
    Expects(lhs._getter);
    return (lhs._getter == rhs._getter);
  }
  friend bool inline operator !=(ROProperty & lhs, ValueType rhs)
  {
    Expects(lhs._getter);
    return lhs._getter() != rhs;
  }
};

template <typename T>
class ROIndexedProperty
{
CUSTOM_MEM_ALLOCATION_IMPL
// using ValueType = typename std::conditional<std::is_trivially_copyable<T>::value, T, const T&>::type;
using TGetter = fastdelegate::FastDelegate1<T, int32_t>;
private:
  TGetter _getter;

public:
  ROIndexedProperty() = delete;
  explicit ROIndexedProperty(TGetter && Getter) noexcept
    : _getter(std::move(Getter))
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

  friend bool constexpr inline operator ==(const ROIndexedProperty & lhs, const ROIndexedProperty & rhs)
  {
    Expects(lhs._getter);
    Expects(rhs._getter);
    return lhs._getter() == rhs._getter();
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

  constexpr T operator->() const
  {
    Expects(_value);
    return _value();
  }

  constexpr T operator->()
  {
    Expects(_value);
    return *_value;
  }

  // constexpr decltype(auto) operator*() const { return *_value; }
  constexpr T operator *() const { return *_value; }

  friend bool constexpr inline operator ==(const ROProperty2 & lhs, const ROProperty2 & rhs)
  {
    Expects(lhs._value);
    Expects(rhs._value);
    return *lhs._value == *rhs._value;
  }

  friend bool constexpr inline operator ==(const ROProperty2 & lhs, const T & rhs)
  {
    Expects(lhs._value);
    return *lhs._value == rhs;
  }

  friend bool constexpr inline operator !=(const ROProperty2 & lhs, const ROProperty2 & rhs)
  {
    Expects(lhs._value);
    Expects(rhs._value);
    return *lhs._value != *rhs._value;
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

// pointers property with setter
template <typename T>
class RWProperty1
{
CUSTOM_MEM_ALLOCATION_IMPL
private:
  using TGetter = fastdelegate::FastDelegate0<const T*>;
  using TSetter = fastdelegate::FastDelegate1<void, const T*>;
  TGetter _getter;
  TSetter _setter;
public:
  RWProperty1() = delete;
  explicit RWProperty1(TGetter && Getter, TSetter && Setter) noexcept :
    _getter(std::move(Getter)),
    _setter(std::move(Setter))
  {
    Expects(!_getter.empty());
    Expects(!_setter.empty());
  }
  RWProperty1(const RWProperty1 &) = default;
  RWProperty1(RWProperty1 &&) noexcept = default;
  RWProperty1 & operator =(const RWProperty1 &) = default;
  RWProperty1 & operator =(RWProperty1 &&) noexcept = default;

  constexpr const T * operator()() const
  {
    Expects(_getter);
    return _getter();
  }

  constexpr operator const T*() const
  {
    Expects(_getter);
    return _getter();
  }

  constexpr const T * operator->() const
  {
    return _getter();
  }
  // constexpr decltype(auto) operator*() const { return *_getter(); }
  constexpr T operator *() const { return *_getter(); }

  void operator()(const T * Value)
  {
    Expects(_setter);
    _setter(Value);
  }

  void operator =(const T * Value)
  {
    Expects(_setter);
    _setter(Value);
  }

  constexpr bool operator ==(const T * Value) const
  {
    Expects(_getter);
    return _getter() == Value;
  }

  friend bool inline operator ==(const RWProperty1 & lhs, const RWProperty1 & rhs)
  {
    Expects(lhs._getter);
    return (lhs._getter == rhs._getter) && (lhs._setter == rhs._setter);
  }

  friend bool inline operator !=(RWProperty1 & lhs, const T & rhs)
  {
    Expects(lhs._getter);
    return lhs._getter() != rhs;
  }
};

// simple property
template <typename T>
class RWProperty2
{
CUSTOM_MEM_ALLOCATION_IMPL
using ValueType = typename std::conditional<std::is_trivially_copyable<T>::value, T, const T&>::type;
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

  constexpr T operator->() const
  {
    Expects(_value);
    return _value();
  }

  constexpr T operator->()
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

  friend bool constexpr inline operator ==(const RWProperty2 & lhs, const RWProperty2 & rhs)
  {
    Expects(lhs._value);
    Expects(rhs._value);
    return *lhs._value == *rhs._value;
  }

  friend bool constexpr inline operator ==(const RWProperty2 & lhs, const T & rhs)
  {
    Expects(lhs._value);
    return *lhs._value == rhs;
  }

  friend bool constexpr inline operator !=(const RWProperty2 & lhs, const RWProperty2 & rhs)
  {
    Expects(lhs._value);
    Expects(rhs._value);
    return *lhs._value != *rhs._value;
  }

  friend bool constexpr inline operator!=(RWProperty2 & lhs, const T & rhs)
  {
    Expects(lhs._value);
    return *lhs._value != rhs;
  }
};

template <typename T>
class RWPropertySimple
{
CUSTOM_MEM_ALLOCATION_IMPL
using ValueType = typename std::conditional<std::is_trivially_copyable<T>::value, T, const T&>::type;
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

  friend bool constexpr inline operator ==(const RWPropertySimple & lhs, const RWPropertySimple & rhs)
  {
    Expects(lhs._value);
    Expects(rhs._value);
    return *lhs._value == *rhs._value;
  }

  friend bool constexpr inline operator ==(const RWPropertySimple & lhs, const T & rhs)
  {
    Expects(lhs._value);
    return *lhs._value == rhs;
  }

  friend bool constexpr inline operator !=(const RWPropertySimple & lhs, const RWPropertySimple & rhs)
  {
    Expects(lhs._value);
    Expects(rhs._value);
    return *lhs._value != *rhs._value;
  }
};

//template<int s> struct CheckSizeT;
//CheckSizeT<sizeof(ROProperty<int>)> checkSize;
//template<int s> struct CheckSizeT;
//CheckSizeT<sizeof(ROPropertySimple<double>)> checkSize;
