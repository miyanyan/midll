// Copyright 2014 Renato Tegon Forti, Antony Polukhin.
// Copyright Antony Polukhin, 2015-2024.
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <midll/config.hpp>
#include <midll/shared_library.hpp>

/// \file midll/import.hpp
/// \brief Contains all the midll::import* reference counting
/// functions that hold a shared pointer to the instance of
/// midll::shared_library.

namespace midll
{
namespace detail
{

template<class T>
class library_function
{
    // Copying of `midll::shared_library` is very expensive, so we use a `shared_ptr` to make it faster.
    std::shared_ptr<T> f_;

public:
    inline library_function(const std::shared_ptr<shared_library>& lib, T* func_ptr) noexcept
        : f_(lib, func_ptr)
    {
    }

    // Compilation error at this point means that imported function
    // was called with unmatching parameters.
    //
    // Example:
    // auto f = dll::import_symbol<void(int)>("function", "lib.so");
    // f("Hello");  // error: invalid conversion from 'const char*' to 'int'
    // f(1, 2);     // error: too many arguments to function
    // f();         // error: too few arguments to function
    template<class... Args>
    inline auto operator()(Args&&... args) const -> decltype((*f_)(static_cast<Args&&>(args)...))
    {
        return (*f_)(static_cast<Args&&>(args)...);
    }
};

template<class T>
struct import_type
{
    using base_type = std::conditional_t<std::is_object_v<T>, std::shared_ptr<T>, detail::library_function<T> >;
};
} // namespace detail

/*!
 * Returns callable object or std::shared_ptr<T> that holds the symbol imported
 * from the loaded library. Returned value refcounts usage
 * of the loaded shared library, so that it won't get unload until all copies of return value
 * are not destroyed.
 *
 * This call will succeed if call to \forcedlink{shared_library}`::has(const char* )`
 * function with the same symbol name returned `true`.
 *
 * For importing symbols by \b alias names use \forcedlink{import_alias} method.
 *
 * \b Examples:
 *
 * \code
 * std::function<int(int)> f = import_symbol<int(int)>("test_lib.so", "integer_func_name");
 *
 * auto f_cpp11 = import_symbol<int(int)>("test_lib.so", "integer_func_name");
 * \endcode
 *
 * \code
 * std::shared_ptr<int> i = import_symbol<int>("test_lib.so", "integer_name");
 * \endcode
 *
 * \b Template \b parameter \b T:    Type of the symbol that we are going to import. Must be explicitly specified.
 *
 * \param lib Path to shared library or shared library to load function from.
 * \param name Null-terminated C or C++ mangled name of the function to import. Can handle std::string, char*, const
 * char*. \param mode An mode that will be used on library load.
 *
 * \return callable object if T is a function type, or std::shared_ptr<T> if T is an object type.
 *
 * \throw \forcedlinkfs{system_error} if symbol does not exist or if the DLL/DSO was not loaded.
 *       Overload that accepts path also throws std::bad_alloc in case of insufficient memory.
 */
template<class T>
auto import_symbol(const midll::fs::path& lib, const char* name, load_mode::type mode = load_mode::default_mode)
{
    using type = typename midll::detail::import_type<T>::base_type;

    std::shared_ptr<midll::shared_library> p = std::make_shared<midll::shared_library>(lib, mode);
    return type(p, std::addressof(p->get<T>(name)));
}

//! \overload midll::import_symbol(const midll::fs::path& lib, const char* name, load_mode::type mode)
template<class T>
auto import_symbol(const midll::fs::path& lib, const std::string& name, load_mode::type mode = load_mode::default_mode)
{
    return dll::import_symbol<T>(lib, name.c_str(), mode);
}

//! \overload midll::import_symbol(const midll::fs::path& lib, const char* name, load_mode::type mode)
template<class T>
auto import_symbol(const shared_library& lib, const char* name)
{
    using type = typename midll::detail::import_type<T>::base_type;

    std::shared_ptr<midll::shared_library> p = std::make_shared<midll::shared_library>(lib);
    return type(p, std::addressof(p->get<T>(name)));
}

//! \overload midll::import_symbol(const midll::fs::path& lib, const char* name, load_mode::type mode)
template<class T>
auto import_symbol(const shared_library& lib, const std::string& name)
{
    return dll::import_symbol<T>(lib, name.c_str());
}

//! \overload midll::import_symbol(const midll::fs::path& lib, const char* name, load_mode::type mode)
template<class T>
auto import_symbol(shared_library&& lib, const char* name)
{
    using type = typename midll::detail::import_type<T>::base_type;

    std::shared_ptr<midll::shared_library> p = std::make_shared<midll::shared_library>(std::move(lib));
    return type(p, std::addressof(p->get<T>(name)));
}

//! \overload midll::import_symbol(const midll::fs::path& lib, const char* name, load_mode::type mode)
template<class T>
auto import_symbol(shared_library&& lib, const std::string& name)
{
    return dll::import_symbol<T>(std::move(lib), name.c_str());
}

/*!
 * Returns callable object or std::shared_ptr<T> that holds the symbol imported
 * from the loaded library. Returned value refcounts usage
 * of the loaded shared library, so that it won't get unload until all copies of return value
 * are not destroyed.
 *
 * This call will succeed if call to \forcedlink{shared_library}`::has(const char* )`
 * function with the same symbol name returned `true`.
 *
 * For importing symbols by \b non \b alias names use \forcedlink{import} method.
 *
 * \b Examples:
 *
 * \code
 * std::function<int(int)> f = import_alias<int(int)>("test_lib.so", "integer_func_alias_name");
 *
 * auto f_cpp11 = import_alias<int(int)>("test_lib.so", "integer_func_alias_name");
 * \endcode
 *
 * \code
 * std::shared_ptr<int> i = import_alias<int>("test_lib.so", "integer_alias_name");
 * \endcode
 *
 * \code
 * \endcode
 *
 * \b Template \b parameter \b T:    Type of the symbol alias that we are going to import. Must be explicitly specified.
 *
 * \param lib Path to shared library or shared library to load function from.
 * \param name Null-terminated C or C++ mangled name of the function or variable to import. Can handle std::string,
 * char*, const char*. \param mode An mode that will be used on library load.
 *
 * \return callable object if T is a function type, or std::shared_ptr<T> if T is an object type.
 *
 * \throw \forcedlinkfs{system_error} if symbol does not exist or if the DLL/DSO was not loaded.
 *       Overload that accepts path also throws std::bad_alloc in case of insufficient memory.
 */
template<class T>
auto import_alias(const midll::fs::path& lib, const char* name, load_mode::type mode = load_mode::default_mode)
{
    using type = typename midll::detail::import_type<T>::base_type;

    std::shared_ptr<midll::shared_library> p = std::make_shared<midll::shared_library>(lib, mode);
    return type(p, p->get<T*>(name));
}

//! \overload midll::import_alias(const midll::fs::path& lib, const char* name, load_mode::type mode)
template<class T>
auto import_alias(const midll::fs::path& lib, const std::string& name, load_mode::type mode = load_mode::default_mode)
{
    return dll::import_alias<T>(lib, name.c_str(), mode);
}

//! \overload midll::import_alias(const midll::fs::path& lib, const char* name, load_mode::type mode)
template<class T>
auto import_alias(const shared_library& lib, const char* name)
{
    using type = typename midll::detail::import_type<T>::base_type;

    std::shared_ptr<midll::shared_library> p = std::make_shared<midll::shared_library>(lib);
    return type(p, p->get<T*>(name));
}

//! \overload midll::import_alias(const midll::fs::path& lib, const char* name, load_mode::type mode)
template<class T>
auto import_alias(const shared_library& lib, const std::string& name)
{
    return dll::import_alias<T>(lib, name.c_str());
}

//! \overload midll::import_alias(const midll::fs::path& lib, const char* name, load_mode::type mode)
template<class T>
auto import_alias(shared_library&& lib, const char* name)
{
    using type = typename midll::detail::import_type<T>::base_type;

    std::shared_ptr<midll::shared_library> p = std::make_shared<midll::shared_library>(std::move(lib));
    return type(p, p->get<T*>(name));
}

//! \overload midll::import_alias(const midll::fs::path& lib, const char* name, load_mode::type mode)
template<class T>
auto import_alias(shared_library&& lib, const std::string& name)
{
    return dll::import_alias<T>(std::move(lib), name.c_str());
}

} // namespace midll
