// Copyright 2014 Renato Tegon Forti, Antony Polukhin.
// Copyright Antony Polukhin, 2015-2024.
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <midll/config.hpp>

#ifndef MIDLL_OS_WINDOWS
#    include <dlfcn.h>
#endif

namespace midll
{
namespace detail
{

inline void reset_dlerror() noexcept
{
#ifndef MIDLL_OS_WINDOWS
    const char* const error_txt = dlerror();
    (void)error_txt;
#endif
}

inline void report_error(const midll::fs::error_code& ec, const char* message)
{
#ifndef MIDLL_OS_WINDOWS
    const char* const error_txt = dlerror();
    if (error_txt) {
        throw midll::fs::system_error(
            ec, message + std::string(" (dlerror system message: ") + error_txt + std::string(")"));
    }
#endif

    throw midll::fs::system_error(ec, message);
}

} // namespace detail
} // namespace midll
