// Copyright 2014 Renato Tegon Forti, Antony Polukhin.
// Copyright Antony Polukhin, 2015-2024.
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <midll/config.hpp>
#include <midll/detail/system_error.hpp>

#if defined(MIDLL_OS_MACOS) || defined(MIDLL_OS_IOS)

#    include <mach-o/dyld.h>

namespace midll
{
namespace detail
{
inline midll::fs::path program_location_impl(midll::fs::error_code &ec)
{
    ec.clear();

    char path[1024];
    uint32_t size = sizeof(path);
    if (_NSGetExecutablePath(path, &size) == 0) return midll::fs::path(path);

    char *p = new char[size];
    if (_NSGetExecutablePath(p, &size) != 0) {
        ec = midll::fs::make_error_code(midll::fs::errc::bad_file_descriptor);
    }

    midll::fs::path ret(p);
    delete[] p;
    return ret;
}
} // namespace detail
} // namespace midll

#elif defined(MIDLL_OS_FREEBSD)

#    include <stdlib.h>
#    include <sys/sysctl.h>
#    include <sys/types.h>

namespace midll
{
namespace detail
{
inline midll::fs::path program_location_impl(midll::fs::error_code& ec)
{
    ec.clear();

    int mib[4];
    mib[0] = CTL_KERN;
    mib[1] = KERN_PROC;
    mib[2] = KERN_PROC_PATHNAME;
    mib[3] = -1;
    char buf[10240];
    size_t cb = sizeof(buf);
    sysctl(mib, 4, buf, &cb, NULL, 0);

    return midll::fs::path(buf);
}
} // namespace detail
} // namespace midll

#elif defined(MIDLL_OS_QNX)

#    include <fstream>
#    include <string> // for std::getline
namespace midll
{
namespace detail
{
inline midll::fs::path program_location_impl(midll::fs::error_code &ec)
{
    ec.clear();

    std::string s;
    std::ifstream ifs("/proc/self/exefile");
    std::getline(ifs, s);

    if (ifs.fail() || s.empty()) {
        ec = midll::fs::make_error_code(midll::fs::errc::bad_file_descriptor);
    }

    return midll::fs::path(s);
}
} // namespace detail
} // namespace midll

#else

namespace midll
{
namespace detail
{
inline midll::fs::path program_location_impl(midll::fs::error_code &ec)
{
    // We can not use
    // midll::detail::path_from_handle(dlopen(NULL, RTLD_LAZY | RTLD_LOCAL), ignore);
    // because such code returns empty path.

    return midll::fs::read_symlink("/proc/self/exe", ec); // Linux specific
}
} // namespace detail
} // namespace midll

#endif
