// Copyright 2014 Renato Tegon Forti, Antony Polukhin.
// Copyright Antony Polukhin, 2015-2024.
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <midll/config.hpp>

#ifdef MIDLL_OS_WINDOWS
#    include <windows.h>
#else
#    include <dlfcn.h>
#endif

/// \file midll/shared_library_load_mode.hpp
/// \brief Contains only the midll::load_mode::type enum and operators related to it.

namespace midll
{
namespace load_mode
{

/*! Library load modes.
 *
 * Each of system family provides own modes. Flags not supported by a particular platform will be silently ignored.
 *
 * For a detailed description of platform specific options see:
 * <a href="http://msdn.microsoft.com/en-us/library/windows/desktop/ms684179(v=vs.85).aspx">Windows specific
 * options</a>, <a href="http://pubs.opengroup.org/onlinepubs/000095399/functions/dlopen.html">POSIX specific
 * options</a>.
 *
 */

enum type
{
#ifdef MIDLL_OS_WINDOWS
    default_mode = 0,
    dont_resolve_dll_references = DONT_RESOLVE_DLL_REFERENCES,
    load_ignore_code_authz_level = LOAD_IGNORE_CODE_AUTHZ_LEVEL,
    load_with_altered_search_path = LOAD_WITH_ALTERED_SEARCH_PATH,
    rtld_lazy = 0,
    rtld_now = 0,
    rtld_global = 0,
    rtld_local = 0,

    append_decorations = 0x00800000,
    search_system_folders = (append_decorations << 1)
#else
    default_mode = 0,
    dont_resolve_dll_references = 0,
    load_ignore_code_authz_level = 0,
    load_with_altered_search_path = 0,
    rtld_lazy = RTLD_LAZY,
    rtld_now = RTLD_NOW,
    rtld_global = RTLD_GLOBAL,
    rtld_local = RTLD_LOCAL,

    append_decorations = 0x00800000,
    search_system_folders = (append_decorations << 1)
#endif
};

/// Free operators for load_mode::type flag manipulation.
constexpr inline type operator|(type left, type right) noexcept
{
    return static_cast<type>(static_cast<unsigned int>(left) | static_cast<unsigned int>(right));
}
constexpr inline type& operator|=(type& left, type right) noexcept
{
    left = left | right;
    return left;
}

constexpr inline type operator&(type left, type right) noexcept
{
    return static_cast<type>(static_cast<unsigned int>(left) & static_cast<unsigned int>(right));
}
constexpr inline type& operator&=(type& left, type right) noexcept
{
    left = left & right;
    return left;
}

constexpr inline type operator^(type left, type right) noexcept
{
    return static_cast<type>(static_cast<unsigned int>(left) ^ static_cast<unsigned int>(right));
}
constexpr inline type& operator^=(type& left, type right) noexcept
{
    left = left ^ right;
    return left;
}

constexpr inline type operator~(type left) noexcept
{
    return static_cast<type>(~static_cast<unsigned int>(left));
}

} // namespace load_mode
} // namespace midll
