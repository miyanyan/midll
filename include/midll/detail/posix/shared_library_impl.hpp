// Copyright 2014 Renato Tegon Forti, Antony Polukhin.
// Copyright Antony Polukhin, 2015-2024.
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <midll/config.hpp>
#include <midll/detail/posix/path_from_handle.hpp>
#include <midll/detail/posix/program_location_impl.hpp>
#include <midll/shared_library_load_mode.hpp>

#include <dlfcn.h>
#include <cstring> // strncmp
#if !defined(MIDLL_OS_MACOS) && !defined(MIDLL_OS_IOS) && !defined(MIDLL_OS_QNX)
#    include <link.h>
#elif defined(MIDLL_OS_QNX)
// QNX's copy of <elf.h> and <link.h> reside in sys folder
#    include <sys/link.h>
#endif

namespace midll
{
namespace detail
{

class shared_library_impl
{
public:
    using native_handle_t = void*;

    shared_library_impl() noexcept
        : handle_(NULL)
    {
    }

    ~shared_library_impl() noexcept { unload(); }

    shared_library_impl(const shared_library_impl&) = delete;
    shared_library_impl& operator=(const shared_library_impl&) = delete;

    shared_library_impl(shared_library_impl&& sl) noexcept
        : handle_(sl.handle_)
    {
        sl.handle_ = NULL;
    }

    shared_library_impl& operator=(shared_library_impl&& sl) noexcept
    {
        swap(sl);
        return *this;
    }

    static midll::fs::path decorate(const midll::fs::path& sl)
    {
        midll::fs::path actual_path =
            (std::strncmp(sl.filename().string().c_str(), "lib", 3)
                 ? midll::fs::path((sl.has_parent_path() ? sl.parent_path() / "lib" : "lib").native() +
                                   sl.filename().native())
                 : sl);
        actual_path += suffix();
        return actual_path;
    }

    void load(midll::fs::path sl, load_mode::type portable_mode, midll::fs::error_code& ec)
    {
        using native_mode_t = int;
        native_mode_t native_mode = static_cast<native_mode_t>(portable_mode);
        unload();

        // Do not allow opening NULL paths. User must use program_location() instead
        if (sl.empty()) {
            midll::detail::reset_dlerror();
            ec = midll::fs::make_error_code(midll::fs::errc::bad_file_descriptor);

            return;
        }

        // Fixing modes
        if (!(native_mode & load_mode::rtld_now)) {
            native_mode |= load_mode::rtld_lazy;
        }

        if (!(native_mode & load_mode::rtld_global)) {
            native_mode |= load_mode::rtld_local;
        }

#if defined(MIDLL_OS_LINUX) || defined(MIDLL_OS_ANDROID)
        if (!sl.has_parent_path() && !(native_mode & load_mode::search_system_folders)) {
            sl = "." / sl;
        }
#else
        if (!sl.is_absolute() && !(native_mode & load_mode::search_system_folders)) {
            midll::fs::error_code current_path_ec;
            midll::fs::path prog_loc = midll::fs::current_path(current_path_ec);
            if (!current_path_ec) {
                prog_loc /= sl;
                sl.swap(prog_loc);
            }
        }
#endif

        native_mode = static_cast<unsigned>(native_mode) & ~static_cast<unsigned>(load_mode::search_system_folders);

        // Trying to open with appended decorations
        if (!!(native_mode & load_mode::append_decorations)) {
            native_mode = static_cast<unsigned>(native_mode) & ~static_cast<unsigned>(load_mode::append_decorations);

            midll::fs::path actual_path = decorate(sl);
            handle_ = dlopen(actual_path.c_str(), native_mode);
            if (handle_) {
                midll::detail::reset_dlerror();
                return;
            }
            midll::fs::error_code prog_loc_err;
            midll::fs::path loc = midll::detail::program_location_impl(prog_loc_err);
            if (midll::fs::exists(actual_path) && !midll::fs::equivalent(sl, loc, prog_loc_err)) {
                // decorated path exists : current error is not a bad file descriptor and we are not trying to load the
                // executable itself
                ec = midll::fs::make_error_code(midll::fs::errc::executable_format_error);
                return;
            }
        }

        // Opening by exactly specified path
        handle_ = dlopen(sl.c_str(), native_mode);
        if (handle_) {
            midll::detail::reset_dlerror();
            return;
        }

        ec = midll::fs::make_error_code(midll::fs::errc::bad_file_descriptor);

        // Maybe user wanted to load the executable itself? Checking...
        // We assume that usually user wants to load a dynamic library not the executable itself, that's why
        // we try this only after traditional load fails.
        midll::fs::error_code prog_loc_err;
        midll::fs::path loc = midll::detail::program_location_impl(prog_loc_err);
        if (!prog_loc_err && midll::fs::equivalent(sl, loc, prog_loc_err) && !prog_loc_err) {
            // As is known the function dlopen() loads the dynamic library file
            // named by the null-terminated string filename and returns an opaque
            // "handle" for the dynamic library. If filename is NULL, then the
            // returned handle is for the main program.
            ec.clear();
            midll::detail::reset_dlerror();
            handle_ = dlopen(NULL, native_mode);
            if (!handle_) {
                ec = midll::fs::make_error_code(midll::fs::errc::bad_file_descriptor);
            }
        }
    }

    bool is_loaded() const noexcept { return (handle_ != 0); }

    void unload() noexcept
    {
        if (!is_loaded()) {
            return;
        }

        dlclose(handle_);
        handle_ = 0;
    }

    void swap(shared_library_impl& rhs) noexcept { std::swap(handle_, rhs.handle_); }

    midll::fs::path full_module_path(midll::fs::error_code& ec) const
    {
        return midll::detail::path_from_handle(handle_, ec);
    }

    static midll::fs::path suffix()
    {
        // https://sourceforge.net/p/predef/wiki/OperatingSystems/
#if defined(MIDLL_OS_MACOS) || defined(MIDLL_OS_IOS)
        return ".dylib";
#else
        return ".so";
#endif
    }

    void* symbol_addr(const char* sb, midll::fs::error_code& ec) const noexcept
    {
        // dlsym - obtain the address of a symbol from a dlopen object
        void* const symbol = dlsym(handle_, sb);
        if (symbol == NULL) {
            ec = midll::fs::make_error_code(midll::fs::errc::invalid_seek);
        }

        // If handle does not refer to a valid object opened by dlopen(),
        // or if the named symbol cannot be found within any of the objects
        // associated with handle, dlsym() shall return NULL.
        // More detailed diagnostic information shall be available through dlerror().

        return symbol;
    }

    native_handle_t native() const noexcept { return handle_; }

private:
    native_handle_t handle_;
};

} // namespace detail
} // namespace midll
