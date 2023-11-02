//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___TYPE_TRAITS_IS_TRIVIALLY_DESTRUCTIBLE_H
#define _LIBCPP___TYPE_TRAITS_IS_TRIVIALLY_DESTRUCTIBLE_H

#include <__config>
#include <__type_traits/integral_constant.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if __has_keyword(__is_trivially_destructible) && !defined(__CUDACC__)

template <class _Tp> struct _LIBCPP_TEMPLATE_VIS is_trivially_destructible
    : public integral_constant<bool, __is_trivially_destructible(_Tp)> {};

#elif __has_feature(has_trivial_destructor) || defined(_LIBCPP_COMPILER_GCC)

template <class _Tp> struct _LIBCPP_TEMPLATE_VIS is_trivially_destructible
    : public integral_constant<bool, is_destructible<_Tp>::value && __has_trivial_destructor(_Tp)> {};

#else

template <class _Tp> struct __libcpp_trivial_destructor
    : public integral_constant<bool, is_scalar<_Tp>::value ||
                                     is_reference<_Tp>::value> {};

template <class _Tp> struct _LIBCPP_TEMPLATE_VIS is_trivially_destructible
    : public __libcpp_trivial_destructor<typename remove_all_extents<_Tp>::type> {};

template <class _Tp> struct _LIBCPP_TEMPLATE_VIS is_trivially_destructible<_Tp[]>
    : public false_type {};

#endif

#if _LIBCPP_STD_VER > 14
template <class _Tp>
inline constexpr bool is_trivially_destructible_v = is_trivially_destructible<_Tp>::value;
#endif

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___TYPE_TRAITS_IS_TRIVIALLY_DESTRUCTIBLE_H
