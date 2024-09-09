// Copyright 2014 Renato Tegon Forti, Antony Polukhin.
// Copyright Antony Polukhin, 2015-2024.
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstring> // std::memcpy
#include <type_traits>

#include <midll/config.hpp>

#if defined(__GNUC__) && defined(__GNUC_MINOR__) && (__GNUC__ * 100 + __GNUC_MINOR__ > 301)
#    pragma GCC system_header
#endif

namespace midll
{
namespace detail
{

// GCC warns when reinterpret_cast between function pointer and object pointer occur.
// This method suppress the warnings and ensures that such casts are safe.
template<class To, class From>
inline typename std::enable_if_t<
    !std::is_member_pointer_v<To> && !std::is_reference_v<To> && !std::is_member_pointer_v<From>, To>
aggressive_ptr_cast(From v) noexcept
{
    static_assert(std::is_pointer_v<To> && std::is_pointer_v<From>,
                  "`agressive_ptr_cast` function must be used only for pointer casting.");

    static_assert(
        std::is_void_v<typename std::remove_pointer_t<To> > || std::is_void_v<typename std::remove_pointer_t<From> >,
        "`agressive_ptr_cast` function must be used only for casting to or from void pointers.");

    static_assert(sizeof(v) == sizeof(To),
                  "Pointer to function and pointer to object differ in size on your platform.");

    return reinterpret_cast<To>(v);
}

#ifdef _MSC_VER
#    pragma warning(push)
#    pragma warning(disable : 4172) // "returning address of local variable or temporary" but **v is not local!
#endif

template<class To, class From>
inline typename std::enable_if_t<std::is_reference_v<To> && !std::is_member_pointer_v<From>, To> aggressive_ptr_cast(
    From v) noexcept
{
    static_assert(std::is_pointer_v<From>, "`agressive_ptr_cast` function must be used only for pointer casting.");

    static_assert(std::is_void_v<typename std::remove_pointer_t<From> >,
                  "`agressive_ptr_cast` function must be used only for casting to or from void pointers.");

    static_assert(sizeof(v) == sizeof(typename std::remove_reference_t<To>*),
                  "Pointer to function and pointer to object differ in size on your platform.");
    return static_cast<To>(**reinterpret_cast<typename std::remove_reference_t<To>**>(v));
}

#ifdef _MSC_VER
#    pragma warning(pop)
#endif

template<class To, class From>
inline typename std::enable_if_t<std::is_member_pointer_v<To> && !std::is_member_pointer_v<From>, To>
aggressive_ptr_cast(From v) noexcept
{
    static_assert(std::is_pointer_v<From>, "`agressive_ptr_cast` function must be used only for pointer casting.");

    static_assert(std::is_void_v<typename std::remove_pointer_t<From> >,
                  "`agressive_ptr_cast` function must be used only for casting to or from void pointers.");

    To res = 0;
    std::memcpy(&res, &v, sizeof(From));
    return res;
}

template<class To, class From>
inline typename std::enable_if_t<!std::is_member_pointer_v<To> && std::is_member_pointer_v<From>, To>
aggressive_ptr_cast(From /* v */) noexcept
{
    static_assert(std::is_pointer_v<To>, "`agressive_ptr_cast` function must be used only for pointer casting.");

    static_assert(std::is_void_v<typename std::remove_pointer_t<To> >,
                  "`agressive_ptr_cast` function must be used only for casting to or from void pointers.");

    static_assert(!sizeof(From),
                  "Casting from member pointers to void pointer is not implemnted in `agressive_ptr_cast`.");

    return 0;
}

} // namespace detail
} // namespace midll
