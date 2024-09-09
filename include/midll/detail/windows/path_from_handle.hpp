// Copyright 2014 Renato Tegon Forti, Antony Polukhin.
// Copyright Antony Polukhin, 2015-2024.
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <midll/config.hpp>
#include <midll/detail/system_error.hpp>

namespace midll
{
namespace detail
{

inline midll::fs::error_code last_error_code() noexcept
{
    DWORD err = ::GetLastError();
    return midll::fs::error_code(static_cast<int>(err), midll::fs::system_category());
}

inline midll::fs::path path_from_handle(HMODULE handle, midll::fs::error_code &ec)
{
    static constexpr DWORD ERROR_INSUFFICIENT_BUFFER_ = 0x7A;
    static constexpr DWORD DEFAULT_PATH_SIZE_ = 260;

    // If `handle` parameter is NULL, GetModuleFileName retrieves the path of the
    // executable file of the current process.
    WCHAR path_hldr[DEFAULT_PATH_SIZE_];
    const DWORD ret = GetModuleFileNameW(handle, path_hldr, DEFAULT_PATH_SIZE_);
    if (ret) {
        // On success, GetModuleFileNameW() doesn't reset last error to ERROR_SUCCESS. Resetting it manually.
        ec.clear();
        return midll::fs::path(path_hldr);
    }

    ec = midll::detail::last_error_code();
    for (unsigned i = 2; i < 1025 && static_cast<DWORD>(ec.value()) == ERROR_INSUFFICIENT_BUFFER_; i *= 2) {
        std::wstring p(DEFAULT_PATH_SIZE_ * i, L'\0');
        const std::size_t size = GetModuleFileNameW(handle, &p[0], DEFAULT_PATH_SIZE_ * i);
        if (size) {
            // On success, GetModuleFileNameW() doesn't reset last error to ERROR_SUCCESS. Resetting it manually.
            ec.clear();
            p.resize(size);
            return midll::fs::path(p);
        }

        ec = midll::detail::last_error_code();
    }

    // Error other than ERROR_INSUFFICIENT_BUFFER_ occurred or failed to allocate buffer big enough.
    return midll::fs::path();
}

} // namespace detail
} // namespace midll
