// Copyright 2014 Renato Tegon Forti, Antony Polukhin.
// Copyright Antony Polukhin, 2015-2024.
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <midll/config.hpp>
#include <midll/detail/aggressive_ptr_cast.hpp>
#include <midll/detail/system_error.hpp>

#ifdef MIDLL_OS_WINDOWS
#    include <midll/detail/windows/shared_library_impl.hpp>
#else
#    include <midll/detail/posix/shared_library_impl.hpp>
#endif

namespace midll
{

/*!
 * \brief This class can be used to load a
 *        Dynamic link libraries (DLL's) or Shared Libraries, also know
 *        as dynamic shared objects (DSO's) and get their exported
 *        symbols (functions and variables).
 *
 * shared_library instances share reference count to an actual loaded DLL/DSO, so it
 * is safe and memory efficient to have multiple instances of shared_library referencing the same DLL/DSO
 * even if those instances were loaded using different paths (relative + absolute) referencing the same object.
 *
 * On Linux/POSIX link with library "dl". "-fvisibility=hidden" flag is also recommended for use on Linux/POSIX.
 */
class shared_library
    /// @cond
    : private midll::detail::shared_library_impl
/// @endcond
{
    using base_t = midll::detail::shared_library_impl;

public:
    using native_handle_t = shared_library_impl::native_handle_t;

    /*!
     * Creates in anstance that does not reference any DLL/DSO.
     *
     * \post this->is_loaded() returns false.
     * \throw Nothing.
     */
    shared_library() noexcept {}

    /*!
     * Copy constructor that increments the reference count of an underlying shared library.
     * Same as calling constructor with `lib.location()` parameter.
     *
     * \param lib A library to copy.
     * \post lib == *this
     * \throw \forcedlinkfs{system_error}, std::bad_alloc in case of insufficient memory.
     */
    shared_library(const shared_library& lib)
        : base_t()
    {
        assign(lib);
    }

    /*!
     * Copy constructor that increments the reference count of an underlying shared library.
     * Same as calling constructor with `lib.location(), ec` parameters.
     *
     * \param lib A shared library to copy.
     * \param ec Variable that will be set to the result of the operation.
     * \post lib == *this
     * \throw std::bad_alloc in case of insufficient memory.
     */
    shared_library(const shared_library& lib, midll::fs::error_code& ec)
        : base_t()
    {
        assign(lib, ec);
    }

    /*!
     * Move constructor. Does not invalidate existing symbols and functions loaded from lib.
     *
     * \param lib A shared library to move from.
     * \post lib.is_loaded() returns false, this->is_loaded() return true.
     * \throw Nothing.
     */
    shared_library(shared_library&& lib) noexcept
        : base_t(std::move(static_cast<base_t&>(lib)))
    {
    }

    /*!
     * Loads a library by specified path with a specified mode.
     *
     * \param lib_path Library file name. Can handle std::string, const char*, std::wstring,
     *           const wchar_t* or \forcedlinkfs{path}.
     * \param mode A mode that will be used on library load.
     * \throw \forcedlinkfs{system_error}, std::bad_alloc in case of insufficient memory.
     */
    explicit shared_library(const midll::fs::path& lib_path, load_mode::type mode = load_mode::default_mode)
    {
        shared_library::load(lib_path, mode);
    }

    /*!
     * Loads a library by specified path with a specified mode.
     *
     * \param lib_path Library file name. Can handle std::string, const char*, std::wstring,
     *           const wchar_t* or \forcedlinkfs{path}.
     * \param mode A mode that will be used on library load.
     * \param ec Variable that will be set to the result of the operation.
     * \throw std::bad_alloc in case of insufficient memory.
     */
    shared_library(const midll::fs::path& lib_path, midll::fs::error_code& ec,
                   load_mode::type mode = load_mode::default_mode)
    {
        shared_library::load(lib_path, mode, ec);
    }

    //! \overload shared_library(const midll::fs::path& lib_path, midll::fs::error_code& ec, load_mode::type mode =
    //! load_mode::default_mode)
    shared_library(const midll::fs::path& lib_path, load_mode::type mode, midll::fs::error_code& ec)
    {
        shared_library::load(lib_path, mode, ec);
    }

    /*!
     * Assignment operator. If this->is_loaded() then calls this->unload(). Does not invalidate existing symbols and
     * functions loaded from lib.
     *
     * \param lib A shared library to assign from.
     * \post lib == *this
     * \throw \forcedlinkfs{system_error}, std::bad_alloc in case of insufficient memory.
     */
    shared_library& operator=(const shared_library& lib)
    {
        midll::fs::error_code ec;
        assign(lib, ec);
        if (ec) {
            midll::detail::report_error(ec, "midll::shared_library::operator= failed");
        }

        return *this;
    }

    /*!
     * Move assignment operator. If this->is_loaded() then calls this->unload(). Does not invalidate existing symbols
     * and functions loaded from lib.
     *
     * \param lib A library to move from.
     * \post lib.is_loaded() returns false.
     * \throw Nothing.
     */
    shared_library& operator=(shared_library&& lib) noexcept
    {
        if (lib.native() != native()) {
            swap(lib);
        }

        return *this;
    }

    /*!
     * Destroys the object by calling `unload()`. If library was loaded multiple times
     * by different instances, the actual DLL/DSO won't be unloaded until
     * there is at least one instance that references the DLL/DSO.
     *
     * \throw Nothing.
     */
    ~shared_library() noexcept {}

    /*!
     * Makes *this share the same shared object as lib. If *this is loaded, then unloads it.
     *
     * \post lib.location() == this->location(), lib == *this
     * \param lib A library to copy.
     * \param ec Variable that will be set to the result of the operation.
     * \throw std::bad_alloc in case of insufficient memory.
     */
    shared_library& assign(const shared_library& lib, midll::fs::error_code& ec)
    {
        ec.clear();

        if (native() == lib.native()) {
            return *this;
        }

        if (!lib) {
            unload();
            return *this;
        }

        midll::fs::path loc = lib.location(ec);
        if (ec) {
            return *this;
        }

        shared_library copy(loc, ec);
        if (ec) {
            return *this;
        }

        swap(copy);
        return *this;
    }

    /*!
     * Makes *this share the same shared object as lib. If *this is loaded, then unloads it.
     *
     * \param lib A library instance to assign from.
     * \post lib.location() == this->location()
     * \throw \forcedlinkfs{system_error}, std::bad_alloc in case of insufficient memory.
     */
    shared_library& assign(const shared_library& lib)
    {
        midll::fs::error_code ec;
        assign(lib, ec);
        if (ec) {
            midll::detail::report_error(ec, "midll::shared_library::assign() failed");
        }

        return *this;
    }

    /*!
     * Loads a library by specified path with a specified mode.
     *
     * Note that if some library is already loaded in this instance, load will
     * call unload() and then load the new provided library.
     *
     * \param lib_path Library file name. Can handle std::string, const char*, std::wstring,
     *           const wchar_t* or \forcedlinkfs{path}.
     * \param mode A mode that will be used on library load.
     * \throw \forcedlinkfs{system_error}, std::bad_alloc in case of insufficient memory.
     *
     */
    void load(const midll::fs::path& lib_path, load_mode::type mode = load_mode::default_mode)
    {
        midll::fs::error_code ec;

        base_t::load(lib_path, mode, ec);

        if (ec) {
            midll::detail::report_error(ec, "midll::shared_library::load() failed");
        }
    }

    /*!
     * Loads a library by specified path with a specified mode.
     *
     * Note that if some library is already loaded in this instance, load will
     * call unload() and then load the new provided library.
     *
     * \param lib_path Library file name. Can handle std::string, const char*, std::wstring,
     *           const wchar_t* or \forcedlinkfs{path}.
     * \param ec Variable that will be set to the result of the operation.
     * \param mode A mode that will be used on library load.
     * \throw std::bad_alloc in case of insufficient memory.
     */
    void load(const midll::fs::path& lib_path, midll::fs::error_code& ec,
              load_mode::type mode = load_mode::default_mode)
    {
        ec.clear();
        base_t::load(lib_path, mode, ec);
    }

    //! \overload void load(const midll::fs::path& lib_path, midll::fs::error_code& ec, load_mode::type mode =
    //! load_mode::default_mode)
    void load(const midll::fs::path& lib_path, load_mode::type mode, midll::fs::error_code& ec)
    {
        ec.clear();
        base_t::load(lib_path, mode, ec);
    }

    /*!
     * Unloads a shared library.  If library was loaded multiple times
     * by different instances, the actual DLL/DSO won't be unloaded until
     * there is at least one instance that references the DLL/DSO.
     *
     * \post this->is_loaded() returns false.
     * \throw Nothing.
     */
    void unload() noexcept { base_t::unload(); }

    /*!
     * Check if an library is loaded.
     *
     * \return true if a library has been loaded.
     * \throw Nothing.
     */
    bool is_loaded() const noexcept { return base_t::is_loaded(); }

    /*!
     * Check if an library is not loaded.
     *
     * \return true if a library has not been loaded.
     * \throw Nothing.
     */
    bool operator!() const noexcept { return !is_loaded(); }

    /*!
     * Check if an library is loaded.
     *
     * \return true if a library has been loaded.
     * \throw Nothing.
     */
    operator bool() const noexcept { return is_loaded(); }

    /*!
     * Search for a given symbol on loaded library. Works for all symbols, including alias names.
     *
     * \param symbol_name Null-terminated symbol name. Can handle std::string, char*, const char*.
     * \return `true` if the loaded library contains a symbol with a given name.
     * \throw Nothing.
     */
    bool has(const char* symbol_name) const noexcept
    {
        midll::fs::error_code ec;
        return is_loaded() && !!base_t::symbol_addr(symbol_name, ec) && !ec;
    }

    //! \overload bool has(const char* symbol_name) const
    bool has(const std::string& symbol_name) const noexcept { return has(symbol_name.c_str()); }

    /*!
     * Returns reference to the symbol (function or variable) with the given name from the loaded library.
     * This call will always succeed and throw nothing if call to `has(const char* )`
     * member function with the same symbol name returned `true`.
     *
     * \b Example:
     * \code
     * int& i0 = lib.get<int>("integer_name");
     * int& i1 = *lib.get<int*>("integer_alias_name");
     * \endcode
     *
     * \tparam T Type of the symbol that we are going to import. Must be explicitly specified.
     * \param symbol_name Null-terminated symbol name. Can handle std::string, char*, const char*.
     * \return Reference to the symbol.
     * \throw \forcedlinkfs{system_error} if symbol does not exist or if the DLL/DSO was not loaded.
     */
    template<typename T>
    inline auto get(const std::string& symbol_name) const
        -> std::conditional_t<std::is_member_pointer_v<T> || std::is_reference_v<T>, T, T&>
    {
        return get<T>(symbol_name.c_str());
    }

    template<typename T>
    inline auto get(const char* symbol_name) const
        -> std::conditional_t<std::is_member_pointer_v<T> || std::is_reference_v<T>, T, T&>
    {
        if constexpr (std::is_member_pointer_v<T> || std::is_reference_v<T>) {
            return midll::detail::aggressive_ptr_cast<T>(get_void(symbol_name));
        }
        else {
            return *midll::detail::aggressive_ptr_cast<T*>(get_void(symbol_name));
        }
    }

    /*!
     * Returns a symbol (function or variable) from a shared library by alias name of the symbol.
     *
     * \b Example:
     * \code
     * int& i = lib.get_alias<int>("integer_alias_name");
     * \endcode
     *
     * \tparam T Type of the symbol that we are going to import. Must be explicitly specified..
     * \param alias_name Null-terminated alias symbol name. Can handle std::string, char*, const char*.
     * \throw \forcedlinkfs{system_error} if symbol does not exist or if the DLL/DSO was not loaded.
     */
    template<typename T>
    inline T& get_alias(const char* alias_name) const
    {
        return *get<T*>(alias_name);
    }

    template<typename T>
    inline T& get_alias(const std::string& alias_name) const
    {
        return *get<T*>(alias_name.c_str());
    }

private:
    /// @cond
    // get_void is required to reduce binary size: it does not depend on a template
    // parameter and will be instantiated only once.
    void* get_void(const char* sb) const
    {
        midll::fs::error_code ec;

        if (!is_loaded()) {
            ec = midll::fs::make_error_code(midll::fs::errc::bad_file_descriptor);

            // report_error() calls dlsym, do not use it here!
            throw midll::fs::system_error(ec, "midll::shared_library::get() failed: no library was loaded");
        }

        void* const ret = base_t::symbol_addr(sb, ec);
        if (ec || !ret) {
            midll::detail::report_error(ec, "midll::shared_library::get() failed");
        }

        return ret;
    }
    /// @endcond

public:
    /*!
     * Returns the native handler of the loaded library.
     *
     * \return Platform-specific handle.
     */
    native_handle_t native() const noexcept { return base_t::native(); }

    /*!
     * Returns full path and name of this shared object.
     *
     * \b Example:
     * \code
     * shared_library lib("test_lib.dll");
     * filesystem::path full_path = lib.location(); // C:\Windows\System32\test_lib.dll
     * \endcode
     *
     * \return Full path to the shared library.
     * \throw \forcedlinkfs{system_error}, std::bad_alloc.
     */
    midll::fs::path location() const
    {
        midll::fs::error_code ec;
        if (!is_loaded()) {
            ec = midll::fs::make_error_code(midll::fs::errc::bad_file_descriptor);

            throw midll::fs::system_error(ec, "midll::shared_library::location() failed (no library was loaded)");
        }

        midll::fs::path full_path = base_t::full_module_path(ec);

        if (ec) {
            midll::detail::report_error(ec, "midll::shared_library::location() failed");
        }

        return full_path;
    }

    /*!
     * Returns full path and name of shared module.
     *
     * \b Example:
     * \code
     * shared_library lib("test_lib.dll");
     * filesystem::path full_path = lib.location(); // C:\Windows\System32\test_lib.dll
     * \endcode
     *
     * \param ec Variable that will be set to the result of the operation.
     * \return Full path to the shared library.
     * \throw std::bad_alloc.
     */
    midll::fs::path location(midll::fs::error_code& ec) const
    {
        if (!is_loaded()) {
            ec = midll::fs::make_error_code(midll::fs::errc::bad_file_descriptor);

            return midll::fs::path();
        }

        ec.clear();
        return base_t::full_module_path(ec);
    }

    /*!
     * Returns suffix of shared module:
     * in a call to load() or the constructor/load.
     *
     * \return The suffix od shared module: ".dll" (Windows), ".so" (Unix/Linux/BSD), ".dylib" (MacOS/IOS)
     */
    static midll::fs::path suffix() { return base_t::suffix(); }

    /*!
     * Returns the decorated path to a shared module name, i.e. with needed prefix/suffix added.
     *
     * \b Recommendations: Use `load` with `load_mode::append_decorations` instead of constructing the decorated path
     * via `decorate()` and loading by it.
     *
     * For instance, for a path like "path/to/boost" it returns :
     * - path/to/libboost.so on posix platforms
     * - path/to/libboost.dylib on OSX
     * - path/to/boost.dll on Windows
     *
     * Method handles both relative and absolute paths.
     *
     * - Windows note: `decorate()` does not prepend "lib" to the decorated path. Use `load` with
     * `load_mode::append_decorations` for MinGW compatibility purpose.
     * - Posix note: if the initial module name is already prepended with lib, only the suffix() is appended to the path
     *
     * \param sl the module name and path to decorate - for instance : /usr/lib/boost
     *
     * \return The decorated unportable path that may not exists in the filesystem or could be wrong due to platform
     * specifics.
     */
    static midll::fs::path decorate(const midll::fs::path& sl) { return base_t::decorate(sl); }

    /*!
     * Swaps two libraries. Does not invalidate existing symbols and functions loaded from libraries.
     *
     * \param rhs Library to swap with.
     * \throw Nothing.
     */
    void swap(shared_library& rhs) noexcept { base_t::swap(rhs); }
};

/// Very fast equality check that compares the actual DLL/DSO objects. Throws nothing.
inline bool operator==(const shared_library& lhs, const shared_library& rhs) noexcept
{
    return lhs.native() == rhs.native();
}

/// Very fast inequality check that compares the actual DLL/DSO objects. Throws nothing.
inline bool operator!=(const shared_library& lhs, const shared_library& rhs) noexcept
{
    return lhs.native() != rhs.native();
}

/// Compare the actual DLL/DSO objects without any guarantee to be stable between runs. Throws nothing.
inline bool operator<(const shared_library& lhs, const shared_library& rhs) noexcept
{
    return lhs.native() < rhs.native();
}

/// Swaps two shared libraries. Does not invalidate symbols and functions loaded from libraries. Throws nothing.
inline void swap(shared_library& lhs, shared_library& rhs) noexcept
{
    lhs.swap(rhs);
}

} // namespace midll