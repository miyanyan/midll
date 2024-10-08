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
#    include <midll/detail/windows/path_from_handle.hpp>
#else
#    include <dlfcn.h>
#    include <midll/detail/posix/program_location_impl.hpp>
#endif

/// \file midll/runtime_symbol_info.hpp
/// \brief Provides methods for getting acceptable by midll::shared_library location of symbol, source line or program.
namespace midll
{

#ifdef MIDLL_OS_WINDOWS
namespace detail
{
inline midll::fs::path program_location_impl(midll::fs::error_code& ec)
{
    return midll::detail::path_from_handle(NULL, ec);
}
} // namespace detail
#endif

/*!
 * On success returns full path and name to the binary object that holds symbol pointed by ptr_to_symbol.
 *
 * \param ptr_to_symbol Pointer to symbol which location is to be determined.
 * \param ec Variable that will be set to the result of the operation.
 * \return Path to the binary object that holds symbol or empty path in case error.
 * \throws std::bad_alloc in case of insufficient memory. Overload that does not accept \forcedlinkfs{error_code} also
 * throws \forcedlinkfs{system_error}.
 *
 * \b Examples:
 * \code
 * int main() {
 *    midll::symbol_location_ptr(std::set_terminate(0));       // returns "/some/path/libmy_terminate_handler.so"
 *    midll::symbol_location_ptr(::signal(SIGSEGV, SIG_DFL));  // returns "/some/path/libmy_symbol_handler.so"
 * }
 * \endcode
 */
template<class T>
inline midll::fs::path symbol_location_ptr(T ptr_to_symbol, midll::fs::error_code& ec)
{
    static_assert(std::is_pointer<T>::value,
                  "midll::symbol_location_ptr works only with pointers! `ptr_to_symbol` must be a pointer");
    midll::fs::path ret;
    if (!ptr_to_symbol) {
        ec = midll::fs::make_error_code(midll::fs::errc::bad_address);

        return ret;
    }
    ec.clear();

    const void* ptr = reinterpret_cast<const void*>(ptr_to_symbol);

#ifdef MIDLL_OS_WINDOWS
    MEMORY_BASIC_INFORMATION mbi;
    if (!::VirtualQuery(ptr, &mbi, sizeof(mbi))) {
        ec = midll::detail::last_error_code();
        return ret;
    }

    return midll::detail::path_from_handle(reinterpret_cast<::HMODULE>(mbi.AllocationBase), ec);
#else
    Dl_info info;

    // Some of the libc headers miss `const` in `dladdr(const void*, Dl_info*)`
    const int res = dladdr(const_cast<void*>(ptr), &info);

    if (res) {
        ret = info.dli_fname;
    }
    else {
        midll::detail::reset_dlerror();
        ec = midll::fs::make_error_code(midll::fs::errc::bad_address);
    }

    return ret;
#endif
}

//! \overload symbol_location_ptr(const void* ptr_to_symbol, midll::fs::error_code& ec)
template<class T>
inline midll::fs::path symbol_location_ptr(T ptr_to_symbol)
{
    midll::fs::path ret;
    midll::fs::error_code ec;
    ret = midll::symbol_location_ptr(ptr_to_symbol, ec);

    if (ec) {
        midll::detail::report_error(ec, "midll::symbol_location_ptr(T ptr_to_symbol) failed");
    }

    return ret;
}

/*!
 * On success returns full path and name of the binary object that holds symbol.
 *
 * \tparam T Type of the symbol, must not be explicitly specified.
 * \param symbol Symbol which location is to be determined.
 * \param ec Variable that will be set to the result of the operation.
 * \return Path to the binary object that holds symbol or empty path in case error.
 * \throws std::bad_alloc in case of insufficient memory. Overload that does not accept \forcedlinkfs{error_code} also
 * throws \forcedlinkfs{system_error}.
 *
 * \b Examples:
 * \code
 * int var;
 * void foo() {}
 *
 * int main() {
 *    midll::symbol_location(var);                     // returns program location
 *    midll::symbol_location(foo);                     // returns program location
 *    midll::symbol_location(std::cerr);               // returns location of libstdc++:
 * "/usr/lib/x86_64-linux-gnu/libstdc++.so.6" midll::symbol_location(std::placeholders::_1);   // returns location of
 * libstdc++: "/usr/lib/x86_64-linux-gnu/libstdc++.so.6" midll::symbol_location(std::puts);               // returns
 * location of libc: "/lib/x86_64-linux-gnu/libc.so.6"
 * }
 * \endcode
 */
template<class T>
inline midll::fs::path symbol_location(const T& symbol, midll::fs::error_code& ec)
{
    ec.clear();
    return midll::symbol_location_ptr(reinterpret_cast<const void*>(std::addressof(symbol)), ec);
}

//! \overload symbol_location(const T& symbol, midll::fs::error_code& ec)
template<class T>
inline midll::fs::path symbol_location(const T& symbol)
{
    midll::fs::path ret;
    midll::fs::error_code ec;
    ret = midll::symbol_location_ptr(reinterpret_cast<const void*>(std::addressof(symbol)), ec);

    if (ec) {
        midll::detail::report_error(ec, "midll::symbol_location(const T& symbol) failed");
    }

    return ret;
}

/// @cond
// We have anonymous namespace here to make sure that `this_line_location()` method is instantiated in
// current translation unit and is not shadowed by instantiations from other units.
//
namespace
{
/// @endcond

/*!
 * On success returns full path and name of the binary object that holds the current line of code
 * (the line in which the `this_line_location()` method was called).
 *
 * \param ec Variable that will be set to the result of the operation.
 * \throws std::bad_alloc in case of insufficient memory. Overload that does not accept \forcedlinkfs{error_code} also
 * throws \forcedlinkfs{system_error}.
 */
static inline midll::fs::path this_line_location(midll::fs::error_code& ec)
{
    using func_t = midll::fs::path(midll::fs::error_code&);
    func_t& f = this_line_location;
    return midll::symbol_location(f, ec);
}

//! \overload this_line_location(midll::fs::error_code& ec)
static inline midll::fs::path this_line_location()
{
    midll::fs::path ret;
    midll::fs::error_code ec;
    ret = this_line_location(ec);

    if (ec) {
        midll::detail::report_error(ec, "midll::this_line_location() failed");
    }

    return ret;
}

/// @cond
} // anonymous namespace
/// @endcond

/*!
 * On success returns full path and name of the currently running program (the one which contains the `main()`
 * function).
 *
 * Return value can be used as a parameter for shared_library. See Tutorial "Linking plugin into the executable"
 * for usage example. Flag '-rdynamic' must be used when linking the plugin into the executable
 * on Linux OS.
 *
 * \param ec Variable that will be set to the result of the operation.
 * \throws std::bad_alloc in case of insufficient memory. Overload that does not accept \forcedlinkfs{error_code} also
 * throws \forcedlinkfs{system_error}.
 */
inline midll::fs::path program_location(midll::fs::error_code& ec)
{
    ec.clear();
    return midll::detail::program_location_impl(ec);
}

//! \overload program_location(midll::fs::error_code& ec) {
inline midll::fs::path program_location()
{
    midll::fs::path ret;
    midll::fs::error_code ec;
    ret = midll::detail::program_location_impl(ec);

    if (ec) {
        midll::detail::report_error(ec, "midll::program_location() failed");
    }

    return ret;
}

} // namespace midll
