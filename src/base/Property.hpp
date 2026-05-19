#pragma once

#include <type_traits>
#include <nbglobals.h>
#include <FastDelegate.h>
#include <Global.h>

/// Borland CBuilder __property emulation
/// 40 bytes using fu2::function
/// 16 bytes using FastDelegate
template <typename T>
class ROProperty
{
CUSTOM_MEM_ALLOCATION_IMPL
using ValueType = std::conditional_t<sizeof(T) <= 2*sizeof(void*) && std::is_trivially_copyable_v<T>,
  T, const T&>;
using TGetter = fastdelegate::FastDelegate0<T>;
private:
  TGetter _getter;
public:
  ROProperty() = delete;
  explicit ROProperty(TGetter && Getter) noexcept :
    _getter(std::move(Getter))
  {
    DebugCheck(!_getter.empty());
  }
  ROProperty(const ROProperty &) = delete;
  constexpr ROProperty(ROProperty &&) noexcept = default;
  ~ROProperty() = default;
  ROProperty & operator =(const ROProperty &) = delete;
  constexpr ROProperty & operator =(ROProperty &&) noexcept = default;

  constexpr T operator()() const
  {
    DebugCheck(_getter);
    return _getter();
  }

  constexpr operator T() const
  {
    DebugCheck(_getter);
    return _getter();
  }

  // Note: operator->() requires T to be a pointer type (or provide its own operator->()).
  // For non-pointer T (e.g., UnicodeString), using -> on the property is a compile error.
  constexpr T operator ->() const
  {
    return _getter();
  }
  // constexpr decltype(auto) operator *() const { return _getter(); }
  constexpr T operator *() const { return _getter(); }
  constexpr bool operator ==(ValueType Value) const
  {
    DebugCheck(_getter);
    return _getter() == Value;
  }

  constexpr bool operator !=(ValueType rhs)
  {
    DebugCheck(_getter);
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
    DebugCheck(!_getter.empty());
  }
  ROIndexedProperty(const ROIndexedProperty &) = delete;
  constexpr ROIndexedProperty(ROIndexedProperty &&) noexcept = default;
  ~ROIndexedProperty() = default;
  ROIndexedProperty & operator =(const ROIndexedProperty &) = delete;
  constexpr ROIndexedProperty & operator =(ROIndexedProperty &&) noexcept = default;
  constexpr T operator [](int32_t Index) const
  {
    DebugCheck(_getter);
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
    DebugCheck(_value != nullptr);
  }
  ROProperty2(const ROProperty2 &) = delete;
  constexpr ROProperty2(ROProperty2 &&) noexcept = default;
  ~ROProperty2() = default;
  ROProperty2 & operator =(const ROProperty2 &) = delete;
  constexpr ROProperty2 & operator =(ROProperty2 &&) noexcept = default;

  constexpr T operator ()() const
  {
    DebugCheck(_value);
    return *_value;
  }

  constexpr operator T() const
  {
    DebugCheck(_value);
    return *_value;
  }

  // Note: operator->() requires T to be a pointer type (or provide its own operator->()).
  // For non-pointer T (e.g., UnicodeString), using -> on the property is a compile error.
  constexpr T operator ->() const
  {
    DebugCheck(_value);
    return *_value;
  }

  constexpr T operator ->()
  {
    DebugCheck(_value);
    return *_value;
  }

  // constexpr decltype(auto) operator*() const { return *_value; }
  constexpr T operator *() const { return *_value; }

  friend bool constexpr operator ==(const ROProperty2 & lhs, const T & rhs)
  {
    DebugCheck(lhs._value);
    return *lhs._value == rhs;
  }

  friend bool constexpr operator !=(ROProperty2 & lhs, const T & rhs)
  {
    DebugCheck(lhs._value);
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
using ValueType = typename ROProperty<T>::ValueType;
using TSetter = fastdelegate::FastDelegate1<void, ValueType>;
private:
  TSetter _setter;
public:
  RWProperty() = delete;
  explicit RWProperty(TGetter && Getter, TSetter && Setter) noexcept :
    ROProperty<T>(std::move(Getter)),
    _setter(std::move(Setter))
  {
    DebugCheck(!_setter.empty());
  }
  RWProperty(const RWProperty &) = delete;
  constexpr RWProperty(RWProperty &&) noexcept = default;
  ~RWProperty() = default;
  RWProperty & operator =(const RWProperty & Value)
  {
    DebugCheck(_setter);
    _setter(Value());
    return *this;
  }
  constexpr RWProperty & operator =(RWProperty &&) noexcept = default;

  using ROProperty<T>::operator();
  void operator()(ValueType Value)
  {
    DebugCheck(_setter);
    _setter(Value);
  }
  RWProperty & operator =(ValueType Value)
  {
    DebugCheck(_setter);
    _setter(Value);
    return *this;
  }
};

// simple property
template <typename T>
class RWProperty2
{
CUSTOM_MEM_ALLOCATION_IMPL
using ValueType = std::conditional_t<sizeof(T) <= 2*sizeof(void*) && std::is_trivially_copyable_v<T>,
  T, const T&>;
private:
  T * _value{nullptr};
public:
  RWProperty2() = delete;
  explicit RWProperty2(T * Value) noexcept :
    _value(Value)
  {
    DebugCheck(_value != nullptr);
  }
  RWProperty2(const RWProperty2 &) = delete;
  constexpr RWProperty2(RWProperty2 &&) noexcept = default;
  ~RWProperty2() = default;
  RWProperty2 & operator =(const RWProperty2 & Value)
  {
    DebugCheck(_value);
    *_value = Value();
    return *this;
  }
  constexpr RWProperty2 & operator =(RWProperty2 &&) noexcept = default;

  constexpr T operator ()() const
  {
    DebugCheck(_value);
    return *_value;
  }

  constexpr operator T() const
  {
    DebugCheck(_value);
    return *_value;
  }

  // Note: operator->() requires T to be a pointer type (or provide its own operator->()).
  // For non-pointer T (e.g., UnicodeString), using -> on the property is a compile error.
  constexpr T operator ->() const
  {
    DebugCheck(_value);
    return *_value;
  }

  constexpr T operator ->()
  {
    DebugCheck(_value);
    return *_value;
  }
  // constexpr decltype(auto) operator*() const { return *_value; }
  constexpr T operator *() const { return *_value; }

  void operator()(ValueType Value)
  {
    DebugCheck(_value);
    *_value = Value;
  }

  RWProperty2 & operator =(ValueType Value)
  {
    DebugCheck(_value);
    *_value = Value;
    return *this;
  }

  friend bool constexpr operator ==(const RWProperty2 & lhs, ValueType rhs)
  {
    DebugCheck(lhs._value);
    return *lhs._value == rhs;
  }

  friend bool constexpr operator !=(RWProperty2 & lhs, ValueType rhs)
  {
    DebugCheck(lhs._value);
    return *lhs._value != rhs;
  }
};

template <typename T>
class RWPropertySimple
{
CUSTOM_MEM_ALLOCATION_IMPL
using ValueType = std::conditional_t<sizeof(T) <= 2*sizeof(void*) && std::is_trivially_copyable_v<T>,
  T, const T&>;
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
    DebugCheck(_value != nullptr);
    DebugCheck(!_setter.empty());
  }
  RWPropertySimple(const RWPropertySimple &) = delete;
  constexpr RWPropertySimple(RWPropertySimple &&) noexcept = default;
  ~RWPropertySimple() = default;
  RWPropertySimple & operator =(const RWPropertySimple & Value)
  {
    DebugCheck(_setter);
    _setter(Value());
    return *this;
  }
  constexpr RWPropertySimple & operator =(RWPropertySimple &&) noexcept = default;
  constexpr T operator ()() const
  {
    DebugCheck(_value);
    return *_value;
  }
  constexpr operator T() const
  {
    DebugCheck(_value);
    return *_value;
  }
  // Note: operator->() requires T to be a pointer type (or provide its own operator->()).
  // For non-pointer T (e.g., UnicodeString), using -> on the property is a compile error.
  constexpr T operator ->() const
  {
    DebugCheck(_value);
    return *_value;
  }
  constexpr T operator ->()
  {
    DebugCheck(_value);
    return *_value;
  }

  // constexpr decltype(auto) operator *() const { return *_value; }
  constexpr T operator *() const { return *_value; }

  void operator ()(ValueType Value)
  {
    DebugCheck(_setter);
    _setter(Value);
  }
  RWPropertySimple & operator =(ValueType Value)
  {
    DebugCheck(_setter);
    _setter(Value);
    return *this;
  }

  friend bool constexpr operator ==(const RWPropertySimple & lhs, ValueType rhs)
  {
    DebugCheck(lhs._value);
    return *lhs._value == rhs;
  }

  friend bool constexpr operator !=(const RWPropertySimple & lhs, ValueType rhs)
  {
    DebugCheck(lhs._value);
    return *lhs._value != rhs;
  }

};

//template<int s> struct CheckSizeT;
//CheckSizeT<sizeof(ROProperty<int>)> checkSize;
//template<int s> struct CheckSizeT;
//CheckSizeT<sizeof(ROPropertySimple<double>)> checkSize;

// Macros for dual-declaration (Borland __property + modern wrapper)
// Usage inside class body: NB_RW_PROPERTY(bool, CollectUsage, GetCollectUsage, SetCollectUsage)

#define NB_RW_PROPERTY(Type, Name, Getter, Setter) \
  __property Type Name = { read = Getter, write = Setter }; \
  RWProperty<Type> Name{nb::bind(&Getter, this), nb::bind(&Setter, this)}

#define NB_RO_PROPERTY(Type, Name, Getter) \
  __property Type Name = { read = Getter }; \
  const ROProperty<Type> Name{nb::bind(&Getter, this)}

#define NB_RW_PROPERTY_SIMPLE(Type, Name, Field, Setter) \
  __property Type Name = { read = Field, write = Setter }; \
  RWPropertySimple<Type> Name{&Field, nb::bind(&Setter, this)}

#define NB_RW_PROPERTY2(Type, Name, Field) \
  __property Type Name = { read = Field, write = Field }; \
  RWProperty2<Type> Name{&Field}

#define NB_REF_PROPERTY(Type, Name, Field) \
  __property Type Name = { read = Field, write = Field }; \
  Type& Name{Field}
