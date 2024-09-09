// Copyright Antony Polukhin, 2018-2024.
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <filesystem>
#include <system_error>

#if defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) || defined(__TOS_WIN__) || defined(__WINDOWS__)
#    define MIDLL_OS_WINDOWS
#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
#    define MIDLL_OS_LINUX
#elif defined(__APPLE__) || defined(__MACH__) || defined(macintosh) || defined(Macintosh)
#    define MIDLL_OS_MACOS
#elif defined(__QNX__) || defined(__QNXNTO__)
#    define MIDLL_OS_QNX
#elif defined(__FreeBSD__)
#    define MIDLL_OS_FREEBSD
#endif

#if defined(__APPLE__) && defined(__MACH__) && defined(__ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__)
#    define MIDLL_OS_IOS
#endif

#if defined(__ANDROID__) || defined(ANDROID)
#    define MIDLL_OS_ANDROID
#endif

#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32) || defined(__CYGWIN__)
// All Win32 development environments, including 64-bit Windows and MinGW, define
// _WIN32 or one of its variant spellings. Note that Cygwin is a POSIX environment,
// so does not define _WIN32 or its variants, but still supports dllexport/dllimport.
#    define MIDLL_SYMBOL_EXPORT __declspec(dllexport)
#    define MIDLL_SYMBOL_IMPORT __declspec(dllimport)
#else
#    define MIDLL_SYMBOL_EXPORT __attribute__((__visibility__("default")))
#    define MIDLL_SYMBOL_IMPORT
#endif

namespace midll
{
namespace fs
{

using namespace std::filesystem;

using std::errc;
using std::error_code;
using std::make_error_code;
using std::system_category;
using std::system_error;

} // namespace fs
} // namespace midll
