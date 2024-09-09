// Copyright 2014-2015 Renato Tegon Forti, Antony Polukhin.
// Copyright Antony Polukhin, 2016-2024.
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <midll/config.hpp>
#include <midll/detail/posix/program_location_impl.hpp>
#include <midll/detail/system_error.hpp>

#if defined(MIDLL_OS_MACOS) || defined(MIDLL_OS_IOS)

#    include <mach-o/dyld.h>
#    include <mach-o/nlist.h>
#    include <cstddef> // for std::ptrdiff_t

namespace midll
{
namespace detail
{
inline void* strip_handle(void* handle) noexcept
{
    return reinterpret_cast<void*>((reinterpret_cast<std::ptrdiff_t>(handle) >> 2) << 2);
}

inline midll::fs::path path_from_handle(void* handle, midll::fs::error_code& ec)
{
    handle = strip_handle(handle);

    // Iterate through all images currently in memory
    // https://developer.apple.com/library/mac/documentation/Darwin/Reference/ManPages/man3/dyld.3.html
    const uint32_t count = _dyld_image_count(); // not thread safe: other thread my [un]load images
    for (uint32_t i = 0; i <= count; ++i) {
        // on last iteration `i` is equal to `count` which is out of range, so `_dyld_get_image_name`
        // will return NULL. `dlopen(NULL, RTLD_LAZY)` call will open the current executable.
        const char* image_name = _dyld_get_image_name(i);

        // dlopen/dlclose must not affect `_dyld_image_count()`, because libraries are already loaded and only the
        // internal counter is affected
        void* probe_handle = dlopen(image_name, RTLD_LAZY);
        dlclose(probe_handle);

        // If the handle is the same as what was passed in (modulo mode bits), return this image name
        if (handle == strip_handle(probe_handle)) {
            midll::detail::reset_dlerror();
            return image_name;
        }
    }

    midll::detail::reset_dlerror();
    ec = midll::fs::make_error_code(midll::fs::errc::bad_file_descriptor);

    return midll::fs::path();
}

} // namespace detail
} // namespace midll

#elif defined(MIDLL_OS_ANDROID)

#    include <midll/runtime_symbol_info.hpp>

namespace midll
{
namespace detail
{

struct soinfo
{
    // if defined(__work_around_b_24465209__), then an array of char[128] goes here.
    // Unfortunately, __work_around_b_24465209__ is visible only during compilation of Android's linker
    const void* phdr;
    size_t phnum;
    void* entry;
    void* base;
    // ...          // Ignoring remaning parts of the structure
};

inline midll::fs::path path_from_handle(const void* handle, midll::fs::error_code& ec)
{
    static const std::size_t work_around_b_24465209__offset = 128;
    const struct soinfo* si =
        reinterpret_cast<const struct soinfo*>(static_cast<const char*>(handle) + work_around_b_24465209__offset);
    midll::fs::path ret = midll::symbol_location_ptr(si->base, ec);

    if (ec) {
        ec.clear();
        si = static_cast<const struct soinfo*>(handle);
        return midll::symbol_location_ptr(si->base, ec);
    }

    return ret;
}

} // namespace detail
} // namespace midll

#else

// for dlinfo
#    include <dlfcn.h>

#    ifdef MIDLL_OS_QNX
// QNX's copy of <elf.h> and <link.h> reside in sys folder
#        include <sys/link.h>
#    else
#        include <link.h> // struct link_map
#    endif

namespace midll
{
namespace detail
{

#    ifdef MIDLL_OS_QNX
// Android and QNX miss struct link_map. QNX misses ElfW macro, so avoiding it.
struct link_map
{
    void *l_addr; // Base address shared object is loaded at
    char *l_name; // Absolute file name object was found in
    // ...          // Ignoring remaning parts of the structure
};
#    endif

inline midll::fs::path path_from_handle(void *handle, midll::fs::error_code &ec)
{
    // RTLD_DI_LINKMAP (RTLD_DI_ORIGIN returns only folder and is not suitable for this case)
    // Obtain the Link_map for the handle  that  is  specified.
    // The  p  argument  points to a Link_map pointer (Link_map
    // **p). The actual storage for the Link_map  structure  is
    // maintained by ld.so.1.
    //
    // Unfortunately we can not use `dlinfo(handle, RTLD_DI_LINKMAP, &link_map) < 0`
    // because it is not supported on MacOS X 10.3, NetBSD 3.0, OpenBSD 3.8, AIX 5.1,
    // HP-UX 11, IRIX 6.5, OSF/1 5.1, Cygwin, mingw, Interix 3.5, BeOS.
    // Fortunately investigating the sources of open source projects brought the understanding, that
    // `handle` is just a `struct link_map*` that contains full library name.

    const struct link_map *link_map = 0;
#    ifdef MIDLL_OS_FREEBSD
    // FreeBSD has it's own logic http://code.metager.de/source/xref/freebsd/libexec/rtld-elf/rtld.c
    // Fortunately it has the dlinfo call.
    if (dlinfo(handle, RTLD_DI_LINKMAP, &link_map) < 0) {
        link_map = 0;
    }
#    else
    link_map = static_cast<const struct link_map *>(handle);
#    endif
    if (!link_map) {
        midll::detail::reset_dlerror();
        ec = midll::fs::make_error_code(midll::fs::errc::bad_file_descriptor);

        return midll::fs::path();
    }

    if (!link_map->l_name || *link_map->l_name == '\0') {
        return program_location_impl(ec);
    }

    return midll::fs::path(link_map->l_name);
}

} // namespace detail
} // namespace midll

#endif
