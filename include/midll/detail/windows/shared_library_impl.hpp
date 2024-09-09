// Copyright 2014 Renato Tegon Forti, Antony Polukhin.
// Copyright Antony Polukhin, 2015-2024.
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <windows.h>

#include <midll/config.hpp>
#include <midll/detail/system_error.hpp>
#include <midll/detail/windows/path_from_handle.hpp>
#include <midll/shared_library_load_mode.hpp>

namespace midll
{
namespace detail
{

class shared_library_impl
{
public:
    using native_handle_t = HMODULE;

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
        midll::fs::path actual_path = sl;
        actual_path += suffix();
        return actual_path;
    }

    void load(midll::fs::path sl, load_mode::type portable_mode, midll::fs::error_code& ec)
    {
        using native_mode_t = DWORD;
        native_mode_t native_mode = static_cast<native_mode_t>(portable_mode);
        unload();

        if (!sl.is_absolute() && !(native_mode & load_mode::search_system_folders)) {
            midll::fs::error_code current_path_ec;
            midll::fs::path prog_loc = midll::fs::current_path(current_path_ec);

            if (!current_path_ec) {
                prog_loc /= sl;
                sl.swap(prog_loc);
            }
        }
        native_mode = static_cast<unsigned>(native_mode) & ~static_cast<unsigned>(load_mode::search_system_folders);

        // Trying to open with appended decorations
        if (!!(native_mode & load_mode::append_decorations)) {
            native_mode = static_cast<unsigned>(native_mode) & ~static_cast<unsigned>(load_mode::append_decorations);

            if (load_impl(decorate(sl), native_mode, ec)) {
                return;
            }

            // MinGW loves 'lib' prefix and puts it even on Windows platform.
            const midll::fs::path mingw_load_path =
                (sl.has_parent_path() ? sl.parent_path() / L"lib" : L"lib").native() + sl.filename().native() +
                suffix().native();
            if (load_impl(mingw_load_path, native_mode, ec)) {
                return;
            }
        }

        // From MSDN: If the string specifies a module name without a path and the
        // file name extension is omitted, the function appends the default library
        // extension .dll to the module name.
        //
        // From experiments: Default library extension appended to the module name even if
        // we have some path. So we do not check for path, only for extension. We can not be sure that
        // such behavior remain across all platforms, so we add L"." by hand.
        if (sl.has_extension()) {
            handle_ = LoadLibraryExW(sl.c_str(), 0, native_mode);
        }
        else {
            handle_ = LoadLibraryExW((sl.native() + L".").c_str(), 0, native_mode);
        }

        // LoadLibraryExW method is capable of self loading from program_location() path. No special actions
        // must be taken to allow self loading.
        if (!handle_) {
            ec = midll::detail::last_error_code();
        }
    }

    bool is_loaded() const noexcept { return (handle_ != 0); }

    void unload() noexcept
    {
        if (handle_) {
            FreeLibrary(handle_);
            handle_ = 0;
        }
    }

    void swap(shared_library_impl& rhs) noexcept { std::swap(handle_, rhs.handle_); }

    midll::fs::path full_module_path(midll::fs::error_code& ec) const
    {
        return midll::detail::path_from_handle(handle_, ec);
    }

    static midll::fs::path suffix() { return L".dll"; }

    void* symbol_addr(const char* sb, midll::fs::error_code& ec) const noexcept
    {
        if (is_resource()) {
            // `GetProcAddress` could not be called for libraries loaded with
            // `LOAD_LIBRARY_AS_DATAFILE`, `LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE`
            // or `LOAD_LIBRARY_AS_IMAGE_RESOURCE`.
            ec = midll::fs::make_error_code(midll::fs::errc::operation_not_supported);

            return NULL;
        }

        // Judging by the documentation of GetProcAddress
        // there is no version for UNICODE on desktop/server Windows, because
        // names of functions are stored in narrow characters.
        void* const symbol = reinterpret_cast<void*>(GetProcAddress(handle_, sb));
        if (symbol == NULL) {
            ec = midll::detail::last_error_code();
        }

        return symbol;
    }

    native_handle_t native() const noexcept { return handle_; }

private:
    // Returns true if this load attempt should be the last one.
    bool load_impl(const midll::fs::path& load_path, DWORD mode, midll::fs::error_code& ec)
    {
        handle_ = LoadLibraryExW(load_path.c_str(), 0, mode);
        if (handle_) {
            return true;
        }

        ec = midll::detail::last_error_code();
        if (midll::fs::exists(load_path)) {
            // decorated path exists : current error is not a bad file descriptor
            return true;
        }

        ec.clear();
        return false;
    }

    bool is_resource() const noexcept
    {
        return false; /*!!(
            reinterpret_cast<boost::winapi::ULONG_PTR_>(handle_) & static_cast<boost::winapi::ULONG_PTR_>(3)
        );*/
    }

    native_handle_t handle_;
};

} // namespace detail
} // namespace midll
