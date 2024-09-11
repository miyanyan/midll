#include <algorithm>
#include <iostream>

#include <gtest/gtest.h>

#include <midll/library_info.hpp>
#include <midll/shared_library.hpp>

TEST(test_library_info, library_info)
{
    midll::library_info info(midll::shared_library::decorate("test_library"));
    std::vector<std::string> sec = info.sections();

    std::cout << "sections:\n";
    std::copy(sec.begin(), sec.end(), std::ostream_iterator<std::string>(std::cout, ", "));
    EXPECT_TRUE(std::find(sec.begin(), sec.end(), "midll") != sec.end());

    std::cout << "\nsymbols:\n";
    std::vector<std::string> sym = info.symbols();
    std::copy(sym.begin(), sym.end(), std::ostream_iterator<std::string>(std::cout, ", "));
    std::cout << "\n";
    EXPECT_TRUE(std::find(sym.begin(), sym.end(), "const_integer_g") != sym.end());
    EXPECT_TRUE(std::find(sym.begin(), sym.end(), "say_hello") != sym.end());
#if defined(__GNUC__) && __GNUC__ >= 4 && defined(__ELF__)
    EXPECT_TRUE(std::find(sym.begin(), sym.end(), "protected_function") != sym.end());
#endif

    sym = info.symbols("midll");
    std::cout << "midll symbols:\n";
    std::copy(sym.begin(), sym.end(), std::ostream_iterator<std::string>(std::cout, ", "));
    std::cout << "\n";
    EXPECT_TRUE(std::find(sym.begin(), sym.end(), "const_integer_g_alias") != sym.end());
    EXPECT_TRUE(std::find(sym.begin(), sym.end(), "foo_variable") != sym.end());
    EXPECT_TRUE(std::find(sym.begin(), sym.end(), "const_integer_g") == sym.end());
    EXPECT_TRUE(std::find(sym.begin(), sym.end(), "say_hello") == sym.end());

    EXPECT_TRUE(info.symbols("empty").empty());
    EXPECT_TRUE(info.symbols("section_that_does_not_exist").empty());
}