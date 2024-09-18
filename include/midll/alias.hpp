// Copyright 2014 Renato Tegon Forti, Antony Polukhin.
// Copyright Antony Polukhin, 2015-2024.
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <midll/config.hpp>

namespace midll
{

#if defined(_MSC_VER) // MSVC, Clang-cl, and ICC on Windows

#    define MIDLL_SELECTANY __declspec(selectany)

#    define MIDLL_SECTION(SectionName, Permissions)                                                             \
        static_assert(sizeof(#SectionName) < 10, "Some platforms require section names to be at most 8 bytes"); \
        __pragma(section(#SectionName, Permissions)) __declspec(allocate(#SectionName)) /**/

#else

#    if defined(MIDLL_OS_WINDOWS) || defined(MIDLL_OS_ANDROID)
// There are some problems with mixing `__dllexport__` and `weak` using MinGW
// See https://sourceware.org/bugzilla/show_bug.cgi?id=17480
//
// Android had an issue with exporting weak symbols
// https://code.google.com/p/android/issues/detail?id=70206
#        define MIDLL_SELECTANY
#    else
/*!
 * \brief Macro that allows linker to select any occurrence of this symbol instead of
 * failing with 'multiple definitions' error at linktime.
 *
 * This macro does not work on Android, IBM XL C/C++ and MinGW+Windows
 * because of linker problems with exporting weak symbols
 * (See https://code.google.com/p/android/issues/detail?id=70206, https://sourceware.org/bugzilla/show_bug.cgi?id=17480)
 */
#        define MIDLL_SELECTANY __attribute__((weak))
#    endif

// TODO: improve section permissions using following info:
// http://stackoverflow.com/questions/6252812/what-does-the-aw-flag-in-the-section-attribute-mean

#    if !defined(MIDLL_OS_MACOS) && !defined(MIDLL_OS_IOS)
/*!
 * \brief Macro that puts symbol to a specific section. On MacOS all the sections are put into "__DATA" segment.
 * \param SectionName Name of the section. Must be a valid C identifier without quotes not longer than 8 bytes.
 * \param Permissions Can be "read" or "write" (without quotes!).
 */
#        define MIDLL_SECTION(SectionName, Permissions)                                                             \
            static_assert(sizeof(#SectionName) < 10, "Some platforms require section names to be at most 8 bytes"); \
            __attribute__((section(#SectionName))) /**/
#    else

#        define MIDLL_SECTION(SectionName, Permissions)                                                             \
            static_assert(sizeof(#SectionName) < 10, "Some platforms require section names to be at most 8 bytes"); \
            __attribute__((section("__DATA," #SectionName))) /**/

#    endif

#endif

// Alias - is just a variable that pointers to original data
//
// A few attempts were made to avoid additional indirection:
// 1)
//          // Does not work on Windows, work on Linux
//          extern "C" MIDLL_SYMBOL_EXPORT void AliasName() {
//              reinterpret_cast<void (*)()>(Function)();
//          }
//
// 2)
//          // Does not work on Linux (changes permissions of .text section and produces incorrect DSO)
//          extern "C" MIDLL_SYMBOL_EXPORT void* __attribute__ ((section(".text#")))
//                  func_ptr = *reinterpret_cast<std::ptrdiff_t**>(&foo::bar);
//
// 3)       // requires mangled name of `Function`
//          //  AliasName() __attribute__ ((weak, alias ("Function")))
//
//          // hard to use
//          `#pragma comment(linker, "/alternatename:_pWeakValue=_pDefaultWeakValue")`

/*!
 * \brief Makes an alias name for exported function or variable.
 *
 * This macro is useful in cases of long mangled C++ names. For example some `void boost::foo(std::string)`
 * function name will change to something like `N5boostN3foosE` after mangling.
 * Importing function by `N5boostN3foosE` name does not looks user friendly, especially assuming the fact
 * that different compilers have different mangling schemes. AliasName is the name that won't be mangled
 * and can be used as a portable import name.
 *
 *
 * Can be used in any namespace, including global. FunctionOrVar must be fully qualified,
 * so that address of it could be taken. Multiple different aliases for a single variable/function
 * are allowed.
 *
 * Make sure that AliasNames are unique per library/executable. Functions or variables
 * in global namespace must not have names same as AliasNames.
 *
 * Same AliasName in different translation units must point to the same FunctionOrVar.
 *
 * Puts all the aliases into the \b "midll" read only section of the binary. Equal to
 * \forcedmacrolink{BOOST_DLL_ALIAS_SECTIONED}(FunctionOrVar, AliasName, midll).
 *
 * \param FunctionOrVar Function or variable for which an alias must be made.
 * \param AliasName Name of the alias. Must be a valid C identifier.
 *
 * \b Example:
 * \code
 * namespace foo {
 *   void bar(std::string&);
 *
 *   BOOST_DLL_ALIAS(foo::bar, foo_bar)
 * }
 *
 * BOOST_DLL_ALIAS(foo::bar, foo_bar_another_alias_name)
 * \endcode
 *
 * \b See: \forcedmacrolink{BOOST_DLL_ALIAS_SECTIONED} for making alias in a specific section.
 */
#define MIDLL_ALIAS(FunctionOrVar, AliasName)              \
    MIDLL_ALIAS_SECTIONED(FunctionOrVar, AliasName, midll) \
    /**/

#if ((defined(__GNUC__) && defined(MIDLL_OS_WINDOWS)) || defined(MIDLL_OS_ANDROID) || \
     defined(MIDLL_FORCE_NO_WEAK_EXPORTS)) &&                                         \
    !defined(MIDLL_FORCE_ALIAS_INSTANTIATION)

#    define MIDLL_ALIAS_SECTIONED(FunctionOrVar, AliasName, SectionName) \
        namespace _autoaliases                                           \
        {                                                                \
        extern "C" MIDLL_SYMBOL_EXPORT const void *AliasName;            \
        } /* namespace _autoaliases */                                   \
        /**/

#    define MIDLL_AUTO_ALIAS(FunctionOrVar)                       \
        namespace _autoaliases                                    \
        {                                                         \
        extern "C" MIDLL_SYMBOL_EXPORT const void *FunctionOrVar; \
        } /* namespace _autoaliases */                            \
        /**/
#else
// Note: we can not use `aggressive_ptr_cast` here, because in that case GCC applies
// different permissions to the section and it causes Segmentation fault.
// Note: we can not use `boost::addressof()` here, because in that case GCC
// may optimize away the FunctionOrVar instance and we'll get a pointer to unexisting symbol.
/*!
 * \brief Same as \forcedmacrolink{MIDLL_ALIAS} but puts alias name into the user specified section.
 *
 * \param FunctionOrVar Function or variable for which an alias must be made.
 * \param AliasName Name of the alias. Must be a valid C identifier.
 * \param SectionName Name of the section. Must be a valid C identifier without quotes not longer than 8 bytes.
 *
 * \b Example:
 * \code
 * namespace foo {
 *   void bar(std::string&);
 *
 *   MIDLL_ALIAS_SECTIONED(foo::bar, foo_bar, sect_1) // section "sect_1" now exports "foo_bar"
 * }
 * \endcode
 *
 */
#    define MIDLL_ALIAS_SECTIONED(FunctionOrVar, AliasName, SectionName)                \
        namespace _autoaliases                                                          \
        {                                                                               \
        extern "C" MIDLL_SYMBOL_EXPORT const void *AliasName;                           \
        MIDLL_SECTION(SectionName, read)                                                \
        MIDLL_SELECTANY const void *AliasName =                                         \
            reinterpret_cast<const void *>(reinterpret_cast<intptr_t>(&FunctionOrVar)); \
        } /* namespace _autoaliases */                                                  \
        /**/

/*!
 * \brief Exports variable or function with unmangled alias name.
 *
 * This macro is useful in cases of long mangled C++ names. For example some `void boost::foo(std::string)`
 * function name will change to something like `N5boostN3foosE` after mangling.
 * Importing function by `N5boostN3foosE` name does not looks user friendly, especially assuming the fact
 * that different compilers have different mangling schemes.*
 *
 * Must be used in scope where FunctionOrVar declared. FunctionOrVar must be a valid C name, which means that
 * it must not contain `::`.
 *
 * Functions or variables
 * in global namespace must not have names same as FunctionOrVar.
 *
 * Puts all the aliases into the \b "midll" read only section of the binary. Almost same as
 * \forcedmacrolink{MIDLL_ALIAS}(FunctionOrVar, FunctionOrVar).
 *
 * \param FunctionOrVar Function or variable for which an unmangled alias must be made.
 *
 * \b Example:
 * \code
 * namespace foo {
 *   void bar(std::string&);
 *   MIDLL_AUTO_ALIAS(bar)
 * }
 *
 * \endcode
 *
 * \b See: \forcedmacrolink{MIDLL_ALIAS} for making an alias with different names.
 */

#    define MIDLL_AUTO_ALIAS(FunctionOrVar)                                                           \
        namespace _autoaliases                                                                        \
        {                                                                                             \
        MIDLL_SELECTANY const void *dummy_##FunctionOrVar =                                           \
            reinterpret_cast<const void *>(reinterpret_cast<intptr_t>(&FunctionOrVar));               \
        extern "C" MIDLL_SYMBOL_EXPORT const void *FunctionOrVar;                                     \
        MIDLL_SECTION(midll, read) MIDLL_SELECTANY const void *FunctionOrVar = dummy_##FunctionOrVar; \
        } /* namespace _autoaliases */                                                                \
        /**/

#endif

} // namespace midll
