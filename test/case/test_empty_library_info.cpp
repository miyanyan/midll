#include <algorithm>

#include <gtest/gtest.h>

#include <midll/library_info.hpp>
#include <midll/shared_library.hpp>

TEST(test_library_info, empty_library_info)
{
    midll::library_info info(midll::shared_library::decorate("empty_library"));
    std::vector<std::string> sec = info.sections();

    std::cout << "sections:\n";
    std::copy(sec.begin(), sec.end(), std::ostream_iterator<std::string>(std::cout, ", "));

    std::cout << "\nsymbols:\n";
    std::vector<std::string> sym = info.symbols();
    std::copy(sym.begin(), sym.end(), std::ostream_iterator<std::string>(std::cout, ", "));

    std::cout << "\n";

    EXPECT_TRUE(info.symbols("midll").empty());
    EXPECT_TRUE(info.symbols("empty").empty());
    EXPECT_TRUE(info.symbols("section_that_does_not_exist").empty());
}