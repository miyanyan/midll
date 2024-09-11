#include <algorithm>

#include <gtest/gtest.h>

#include <midll/library_info.hpp>
#include <midll/shared_library.hpp>

TEST(test_shared_library_search_symbol, search_symbol)
{
    auto path = midll::shared_library::decorate("test_library");
    std::cout << "library: " << path << "\n";
    {
        midll::shared_library sl(path);
        EXPECT_TRUE(sl.has("say_hello"));
        EXPECT_TRUE(sl.has("lib_version"));
        EXPECT_TRUE(sl.has("integer_g"));
        EXPECT_TRUE(sl.has(std::string("integer_g")));
        EXPECT_TRUE(!sl.has("i_do_not_exist"));
        EXPECT_TRUE(!sl.has(std::string("i_do_not_exist")));
    }
}