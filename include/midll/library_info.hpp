// Copyright 2014 Renato Tegon Forti, Antony Polukhin.
// Copyright Antony Polukhin, 2015-2024.
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <fstream>

#include <midll/config.hpp>
#include <midll/detail/elf_info.hpp>
#include <midll/detail/macho_info.hpp>
#include <midll/detail/pe_info.hpp>

/// \file midll/library_info.hpp
/// \brief Contains only the midll::library_info class that is capable of
/// extracting different information from binaries.

namespace midll
{

/*!
 * \brief Class that is capable of extracting different information from a library or binary file.
 * Currently understands ELF, MACH-O and PE formats on all the platforms.
 */
class library_info
{
private:
    std::ifstream f_;

    enum
    {
        fmt_elf_info32,
        fmt_elf_info64,
        fmt_pe_info32,
        fmt_pe_info64,
        fmt_macho_info32,
        fmt_macho_info64
    } fmt_;

    inline static void throw_if_in_32bit()
    {
        if constexpr (sizeof(void*) == 4) {
            throw std::runtime_error("Not native format: 64bit binary");
        }
    }

    static void throw_if_in_windows()
    {
#ifdef MIDLL_OS_WINDOWS
        throw std::runtime_error("Not native format: not a PE binary");
#endif
    }

    static void throw_if_in_linux()
    {
#if !defined(MIDLL_OS_WINDOWS) && !defined(MIDLL_OS_MACOS) && !defined(MIDLL_OS_IOS)
        throw std::runtime_error("Not native format: not an ELF binary");
#endif
    }

    static void throw_if_in_macos()
    {
#if defined(BOOST_OS_MACOS) || defined(BOOST_OS_IOS)
        throw std::runtime_error("Not native format: not an Mach-O binary");
#endif
    }

    void init(bool throw_if_not_native)
    {
        if (midll::detail::elf_info32::parsing_supported(f_)) {
            if (throw_if_not_native) {
                throw_if_in_windows();
                throw_if_in_macos();
            }

            fmt_ = fmt_elf_info32;
        }
        else if (midll::detail::elf_info64::parsing_supported(f_)) {
            if (throw_if_not_native) {
                throw_if_in_windows();
                throw_if_in_macos();
                throw_if_in_32bit();
            }

            fmt_ = fmt_elf_info64;
        }
        else if (midll::detail::pe_info32::parsing_supported(f_)) {
            if (throw_if_not_native) {
                throw_if_in_linux();
                throw_if_in_macos();
            }

            fmt_ = fmt_pe_info32;
        }
        else if (midll::detail::pe_info64::parsing_supported(f_)) {
            if (throw_if_not_native) {
                throw_if_in_linux();
                throw_if_in_macos();
                throw_if_in_32bit();
            }

            fmt_ = fmt_pe_info64;
        }
        else if (midll::detail::macho_info32::parsing_supported(f_)) {
            if (throw_if_not_native) {
                throw_if_in_linux();
                throw_if_in_windows();
            }

            fmt_ = fmt_macho_info32;
        }
        else if (midll::detail::macho_info64::parsing_supported(f_)) {
            if (throw_if_not_native) {
                throw_if_in_linux();
                throw_if_in_windows();
                throw_if_in_32bit();
            }

            fmt_ = fmt_macho_info64;
        }
        else {
            throw std::runtime_error("Unsupported binary format");
        }
    }
    /// @endcond

public:
    /*!
     * Opens file with specified path and prepares for information extraction.
     * \param library_path Path to the binary file from which the info must be extracted.
     * \param throw_if_not_native_format Throw an exception if this file format is not
     * supported by OS.
     */
    explicit library_info(const midll::fs::path& library_path, bool throw_if_not_native_format = true)
        : f_(library_path, std::ios_base::in | std::ios_base::binary)
    {
        f_.exceptions(std::ios_base::failbit | std::ifstream::badbit | std::ifstream::eofbit);

        init(throw_if_not_native_format);
    }

    library_info(const library_info&) = delete;
    library_info& operator=(const library_info&) = delete;

    /*!
     * \return List of sections that exist in binary file.
     */
    std::vector<std::string> sections()
    {
        switch (fmt_) {
            case fmt_elf_info32:
                return midll::detail::elf_info32::sections(f_);
            case fmt_elf_info64:
                return midll::detail::elf_info64::sections(f_);
            case fmt_pe_info32:
                return midll::detail::pe_info32::sections(f_);
            case fmt_pe_info64:
                return midll::detail::pe_info64::sections(f_);
            case fmt_macho_info32:
                return midll::detail::macho_info32::sections(f_);
            case fmt_macho_info64:
                return midll::detail::macho_info64::sections(f_);
        };
        return {};
    }

    /*!
     * \return List of all the exportable symbols from all the sections that exist in binary file.
     */
    std::vector<std::string> symbols()
    {
        switch (fmt_) {
            case fmt_elf_info32:
                return midll::detail::elf_info32::symbols(f_);
            case fmt_elf_info64:
                return midll::detail::elf_info64::symbols(f_);
            case fmt_pe_info32:
                return midll::detail::pe_info32::symbols(f_);
            case fmt_pe_info64:
                return midll::detail::pe_info64::symbols(f_);
            case fmt_macho_info32:
                return midll::detail::macho_info32::symbols(f_);
            case fmt_macho_info64:
                return midll::detail::macho_info64::symbols(f_);
        };
        return {};
    }

    /*!
     * \param section_name Name of the section from which symbol names must be returned.
     * \return List of symbols from the specified section.
     */
    std::vector<std::string> symbols(const char* section_name)
    {
        switch (fmt_) {
            case fmt_elf_info32:
                return midll::detail::elf_info32::symbols(f_, section_name);
            case fmt_elf_info64:
                return midll::detail::elf_info64::symbols(f_, section_name);
            case fmt_pe_info32:
                return midll::detail::pe_info32::symbols(f_, section_name);
            case fmt_pe_info64:
                return midll::detail::pe_info64::symbols(f_, section_name);
            case fmt_macho_info32:
                return midll::detail::macho_info32::symbols(f_, section_name);
            case fmt_macho_info64:
                return midll::detail::macho_info64::symbols(f_, section_name);
        };
        return {};
    }

    //! \overload std::vector<std::string> symbols(const char* section_name)
    std::vector<std::string> symbols(const std::string& section_name) { return symbols(section_name.c_str()); }
};

} // namespace midll
