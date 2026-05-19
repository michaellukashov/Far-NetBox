#pragma once

//===- llvm/Support/type_traits.h - Simplfied type traits -------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file provides useful additions to the standard type_traits library.
//
//===----------------------------------------------------------------------===//

#include <type_traits>
#include <utility>

#ifndef __has_feature
#define NB_DEFINED_HAS_FEATURE
#define __has_feature(x) 0
#endif

namespace nb {


/// Metafunction that determines whether the given type is either an
/// integral type or an enumeration type, including enum classes.
///
/// Note that this accepts potentially more integral types than is_integral
/// because it is based on being implicitly convertible to an integral type.
/// Also note that enum classes aren't implicitly convertible to integral types,
/// the value may therefore need to be explicitly converted before being used.
template <typename T> class is_integral_or_enum
{
  using UnderlyingT = std::remove_reference_t<T>;

public:
  static const bool value =
    !std::is_class_v<UnderlyingT>&&  // Filter conversion operators.
    !std::is_pointer_v<UnderlyingT>&&
    !std::is_floating_point_v<UnderlyingT>&&
    (std::is_enum_v<UnderlyingT> ||
      std::is_convertible_v<UnderlyingT, unsigned long long>);
};

/// If T is a pointer, just return it. If it is not, return T&.
template <typename T> struct add_lvalue_reference_if_not_pointer
{
  using type = std::conditional_t<std::is_pointer_v<T>, T, T&>;
};

/// If T is a pointer to X, return a pointer to const X. If it is not,
/// return const T.
template <typename T> struct add_const_past_pointer
{
  using type = std::conditional_t<std::is_pointer_v<T>,
        const std::remove_pointer_t<T> *, const T>;
};

template <typename T> struct const_pointer_or_const_ref
{
  using type =
    std::conditional_t<std::is_pointer_v<T>,
    typename add_const_past_pointer<T>::type, const T&>;
};

} // namespace nb

// If the compiler supports detecting whether a class is final, define
// an NB_IS_FINAL macro. If it cannot be defined properly, this
// macro will be left undefined.
#if (__cplusplus >= 201402L || _MSC_VER >= 1900)
#define NB_IS_FINAL(Ty) std::is_final<Ty>()
#elif __has_feature(is_final) // || NB_GNUC_PREREQ(4, 7, 0)
#define NB_IS_FINAL(Ty) __is_final(Ty)
#endif

#define NB_NODISCARD [[nodiscard]]

#ifdef NB_DEFINED_HAS_FEATURE
#undef __has_feature
#endif
