#include <cmath>
#include <iostream>

#include <gtest/gtest.h>

#include <midll/runtime_symbol_info.hpp>
#include <midll/shared_library.hpp>

// Makes global error variables dirty. Useful for preventing issues like https://github.com/boostorg/dll/issues/16
void make_error_code_dirty()
{
    using namespace std;
    (void)log(-1.0);

#ifdef MIDLL_OS_WINDOWS
    WCHAR path_hldr[10];
    int some_invalid_value_for_handle = 0xFF004242;
    HMODULE some_invalid_handle;
    memcpy(&some_invalid_handle, &some_invalid_value_for_handle, sizeof(some_invalid_value_for_handle));
    GetModuleFileNameW(some_invalid_handle, path_hldr, 10);
#endif
}

// lib functions
using lib_version_func = float();
using say_hello_func = void();
using increment = int(int);

TEST(test_symbol_runtime_info, runtime_info)
{
    auto path = midll::shared_library::decorate("test_library");

    make_error_code_dirty();

    midll::shared_library lib(path);
    std::cout << std::endl;
    std::cout << "shared_library: " << path << std::endl;
    std::cout << "symbol_location: " << midll::symbol_location(lib.get<int>("integer_g")) << std::endl;
    std::cout << "lib.location():      " << lib.location() << std::endl;
    EXPECT_TRUE(midll::symbol_location(lib.get<int>("integer_g")) == lib.location());

    make_error_code_dirty();

    EXPECT_TRUE(midll::symbol_location(lib.get<say_hello_func>("say_hello")) == lib.location());
    EXPECT_TRUE(midll::symbol_location(lib.get<lib_version_func>("lib_version")) == lib.location());

    make_error_code_dirty();

    EXPECT_TRUE(midll::symbol_location(lib.get<const int>("const_integer_g")) == lib.location());

    // Checking that symbols are still available, after another load+unload of the library
    {
        midll::shared_library sl2(path);
    }

    EXPECT_TRUE(midll::symbol_location(lib.get<int>("integer_g")) == lib.location());

    make_error_code_dirty();

    // Checking aliases
    EXPECT_TRUE(midll::symbol_location(lib.get<std::size_t (*)(const std::vector<int>&)>("foo_bar")) == lib.location());
    EXPECT_TRUE(midll::symbol_location(lib.get_alias<std::size_t(const std::vector<int>&)>("foo_bar")) ==
                lib.location());

    EXPECT_TRUE(midll::symbol_location(lib.get<std::size_t*>("foo_variable")) == lib.location());
    EXPECT_TRUE(midll::symbol_location(lib.get_alias<std::size_t>("foo_variable")) == lib.location());
    EXPECT_TRUE(midll::symbol_location(lib.get_alias<midll::fs::path()>("module_location_from_itself")) ==
                lib.location());
}