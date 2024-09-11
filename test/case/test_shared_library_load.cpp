#include <filesystem>
#include <iostream>

#include <gtest/gtest.h>

#include <midll/midll.hpp>

auto relpath = midll::shared_library::decorate("test_library");
auto abspath = std::filesystem::absolute(relpath);

TEST(test_shared_library_load, load)
{
    std::cout << "Library: " << abspath << "\n";
    {
        midll::shared_library sl(abspath);
        EXPECT_TRUE(sl.is_loaded());
        EXPECT_TRUE(sl.location() == abspath);

        midll::shared_library sl2;
        EXPECT_TRUE(!sl2.is_loaded());
        EXPECT_TRUE(!sl2);

        // test swap
        midll::swap(sl, sl2);
        EXPECT_TRUE(!sl.is_loaded());
        EXPECT_TRUE(!sl);
        EXPECT_TRUE(sl2.is_loaded());
        EXPECT_TRUE(sl2);

        // test assign
        sl.assign(sl2);
        EXPECT_TRUE(sl.is_loaded());
        EXPECT_TRUE(sl);
        EXPECT_TRUE(sl2.is_loaded());
        EXPECT_TRUE(sl2);
        EXPECT_TRUE(sl.location() == sl2.location());

        // test assign multi times
        sl.assign(sl2);
        EXPECT_TRUE(sl.is_loaded());
        EXPECT_TRUE(sl);
        EXPECT_TRUE(sl2.is_loaded());
        EXPECT_TRUE(sl2);
        EXPECT_TRUE(sl.location() == sl2.location());

        sl2.assign(sl);
        EXPECT_TRUE(sl.is_loaded());
        EXPECT_TRUE(sl);
        EXPECT_TRUE(sl2.is_loaded());
        EXPECT_TRUE(sl2);
        EXPECT_TRUE(sl.location() == sl2.location());

        // test assign empty lib
        sl2.assign(midll::shared_library());
        EXPECT_TRUE(sl.is_loaded());
        EXPECT_TRUE(sl);
        EXPECT_TRUE(!sl2.is_loaded());
        EXPECT_TRUE(!sl2);
        midll::fs::error_code ec;
        EXPECT_TRUE(sl.location() != sl2.location(ec));
        EXPECT_TRUE(ec);
    }

    {
        // test construct with error code
        midll::fs::error_code ec;
        midll::shared_library sl(abspath, ec);
        EXPECT_TRUE(!ec);
        EXPECT_TRUE(sl);
        EXPECT_TRUE(sl.is_loaded());
        EXPECT_TRUE(sl.location() == abspath);
        EXPECT_TRUE(sl.location(ec) == abspath);
        EXPECT_TRUE(!ec);

        // test self assign
        sl.assign(sl);
        EXPECT_TRUE(sl);
        EXPECT_TRUE(sl.is_loaded());
        EXPECT_TRUE(sl.location() == abspath);
        EXPECT_TRUE(sl.location(ec) == abspath);
        EXPECT_TRUE(!ec);

        sl.assign(sl, ec);
        EXPECT_TRUE(!ec);
        EXPECT_TRUE(sl);
        EXPECT_TRUE(sl.is_loaded());
        EXPECT_TRUE(sl.location() == abspath);
        EXPECT_TRUE(sl.location(ec) == abspath);
        EXPECT_TRUE(!ec);
    }

    {
        midll::shared_library sl;
        EXPECT_TRUE(!sl.is_loaded());

        sl.assign(sl);
        EXPECT_TRUE(!sl);

        midll::shared_library sl2(sl);
        EXPECT_TRUE(!sl);
        EXPECT_TRUE(!sl2);

        sl2.assign(sl);
        EXPECT_TRUE(!sl);
        EXPECT_TRUE(!sl2);
    }

    {
        // test construct and load
        midll::shared_library sl;
        sl.load(abspath);
        EXPECT_TRUE(sl.is_loaded());
        EXPECT_TRUE(sl);
        EXPECT_TRUE(sl.location() == abspath);
    }

    {
        // test construct and load with error code
        midll::shared_library sl;
        midll::fs::error_code ec;
        sl.load(abspath, ec);
        EXPECT_TRUE(sl.is_loaded());
        EXPECT_TRUE(sl);
        EXPECT_TRUE(!ec);
        EXPECT_TRUE(sl.location() == abspath);
    }
}

TEST(test_shared_library_load, load_mode)
{
    {
#if defined(MIDLL_OS_WINDOWS)
        midll::shared_library sl("winmm.dll", midll::load_mode::search_system_folders);
#elif defined(MIDLL_OS_LINUX)
        middll::shared_library sl("libz.so", midll::load_mode::search_system_folders);
#endif
    }

    {
#if defined(MIDLL_OS_WINDOWS)
        midll::shared_library sl("winmm",
                                 midll::load_mode::append_decorations | midll::load_mode::search_system_folders);
#elif defined(MIDLL_OS_LINUX)
        midll::shared_library sl("z", midll::load_mode::append_decorations | midll::load_mode::search_system_folders);
#endif
    }

    {
        midll::shared_library sl(abspath, midll::load_mode::rtld_lazy | midll::load_mode::rtld_global);
        EXPECT_TRUE(sl.is_loaded());
        EXPECT_TRUE(sl);
        EXPECT_TRUE(sl.location() == abspath);
    }

    {
        midll::shared_library sl(abspath, midll::load_mode::rtld_now);
        EXPECT_TRUE(sl.is_loaded());
        EXPECT_TRUE(sl);
        EXPECT_TRUE(sl.location() == abspath);
    }

    {
        midll::shared_library sl(abspath, midll::load_mode::rtld_local);
        EXPECT_TRUE(sl.is_loaded());
        EXPECT_TRUE(sl);
        EXPECT_TRUE(sl.location() == abspath);
    }

    {
        midll::shared_library sl(abspath, midll::load_mode::load_with_altered_search_path);
        EXPECT_TRUE(sl.is_loaded());
        EXPECT_TRUE(sl);
        EXPECT_TRUE(sl.location() == abspath);
    }
}

TEST(test_shared_library_load, unload)
{
    midll::shared_library sl(abspath);
    EXPECT_TRUE(sl.is_loaded());
    EXPECT_TRUE(sl);
    EXPECT_TRUE(sl.location() == abspath);
    sl.unload();
    EXPECT_TRUE(!sl.is_loaded());
    EXPECT_TRUE(!sl);
}

TEST(test_shared_library_load, load_bad_path)
{
    // error_code load calls test
    midll::fs::error_code ec;
    midll::shared_library sl(abspath / "dir_that_does_not_exist", ec);
    EXPECT_TRUE(ec);
    EXPECT_TRUE(!sl.is_loaded());
    EXPECT_TRUE(!sl);

    midll::fs::path bad_path(abspath);
    bad_path += ".1.1.1.1.1.1";
    sl.load(bad_path, ec);
    EXPECT_TRUE(ec);
    EXPECT_TRUE(!sl.is_loaded());
    EXPECT_TRUE(!sl);

    sl.load(abspath, ec);
    EXPECT_TRUE(!ec);
    EXPECT_TRUE(sl.is_loaded());
    EXPECT_TRUE(sl);

    midll::shared_library sl2(bad_path, ec);
    EXPECT_TRUE(ec);
    EXPECT_TRUE(!sl2.is_loaded());
    EXPECT_TRUE(!sl2);

    midll::shared_library sl3(abspath, ec);
    EXPECT_TRUE(!ec);
    EXPECT_TRUE(sl3.is_loaded());
    EXPECT_TRUE(sl3);

    sl.load("", ec);
    EXPECT_TRUE(ec);
    EXPECT_TRUE(!sl.is_loaded());
    EXPECT_TRUE(!sl);
}
