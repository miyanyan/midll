#include <functional>
#include <vector>

#include <gtest/gtest.h>

#include <midll/midll.hpp>

// lib functions
using lib_version_func = float();
using say_hello_func = void();
using increment_func = int(int);

using do_share_res_t =
    std::tuple<std::vector<int>, std::vector<int>, std::vector<int>, const std::vector<int>*, std::vector<int>*>;

using do_share_t = std::shared_ptr<do_share_res_t>(std::vector<int> v1, std::vector<int>& v2,
                                                   const std::vector<int>& v3, const std::vector<int>* v4,
                                                   std::vector<int>* v5);

TEST(test_shared_library_get_symbol, refcountable)
{
    auto path = midll::shared_library::decorate("test_library");

    {
        std::function<say_hello_func> say_hello = midll::import_symbol<say_hello_func>(path, "say_hello");
        say_hello();
        say_hello();
        say_hello();
    }

#if defined(__GNUC__) && __GNUC__ >= 4 && defined(__ELF__)
    {
        const int the_answer = midll::import_symbol<int(int)>(path, "protected_function")(0);
        EXPECT_TRUE(the_answer, 42);
    }
#endif

    std::vector<int> v(1000);
    {
        auto foo_bar = midll::import_alias<std::size_t(const std::vector<int>&)>(path, "foo_bar");
        EXPECT_TRUE(foo_bar(v) == v.size());
    }

    {
        std::function<do_share_t> do_share;
        {
            auto f = midll::import_alias<do_share_t>(path, "do_share");
            do_share = f;
        }
        std::vector<int> v1(1, 1), v2(2, 2), v3(3, 3), v4(4, 4), v5(1000, 5);

        auto res = do_share(v1, v2, v3, &v4, &v5);
        EXPECT_TRUE(std::get<0>(*res).size() == v1.size());
        EXPECT_TRUE(std::get<1>(*res).size() == v2.size());
        EXPECT_TRUE(std::get<2>(*res).size() == v3.size());
        EXPECT_TRUE(std::get<3>(*res) == &v4);
        EXPECT_TRUE(std::get<4>(*res) == &v5);

        EXPECT_TRUE(std::get<0>(*res).front() == v1.front());
        EXPECT_TRUE(std::get<1>(*res).front() == v2.front());
        EXPECT_TRUE(std::get<2>(*res).front() == v3.front());
        EXPECT_TRUE(std::get<3>(*res)->front() == v4.front());
        EXPECT_TRUE(std::get<4>(*res)->front() == v5.front());

        EXPECT_TRUE(std::get<1>(*res).back() == 777);
        EXPECT_TRUE(v5.back() == 9990);
    }

    {
        auto integer_g = midll::import_symbol<int>(path, "integer_g");
        EXPECT_TRUE(*integer_g == 100);

        std::shared_ptr<int> integer2;
        integer2.swap(integer_g);
        EXPECT_TRUE(*integer2 == 100);
    }

    {
        std::function<int&()> f = midll::import_alias<int&()>(path, "ref_returning_function");
        EXPECT_TRUE(f() == 0);

        f() = 10;
        EXPECT_TRUE(f() == 10);

        std::function<int&()> f1 = midll::import_alias<int&()>(path, "ref_returning_function");
        EXPECT_TRUE(f1() == 10);

        f1() += 10;
        EXPECT_TRUE(f() == 20);
    }

    {
        std::shared_ptr<const int> i = midll::import_symbol<const int>(path, "const_integer_g");
        EXPECT_TRUE(*i == 777);

        std::shared_ptr<const int> i2 = i;
        i.reset();
        EXPECT_TRUE(*i2 == 777);
    }

    {
        std::shared_ptr<std::string> s = midll::import_alias<std::string>(path, "info");
        EXPECT_TRUE(*s ==
                    "I am a std::string from the test_library (Think of me as of 'Hello world'. Long 'Hello world').");

        std::shared_ptr<std::string> s2;
        s.swap(s2);
        EXPECT_TRUE(*s2 ==
                    "I am a std::string from the test_library (Think of me as of 'Hello world'. Long 'Hello world').");
    }
}

TEST(test_shared_library_get_symbol, get_symbol)
{
    auto path = midll::shared_library::decorate("test_library");
    midll::shared_library sl(path);

    EXPECT_TRUE(sl.get<int>("integer_g") == 100);

    sl.get<int>("integer_g") = 200;
    EXPECT_TRUE(sl.get<int>("integer_g") == 200);
    EXPECT_TRUE(sl.get<int>(std::string("integer_g")) == 200);

    EXPECT_TRUE(sl.get<say_hello_func>("say_hello"));
    sl.get<say_hello_func>("say_hello")();

    float ver = sl.get<lib_version_func>("lib_version")();
    EXPECT_TRUE(ver == 1.0);

    int n = sl.get<increment_func>("increment")(1);
    EXPECT_TRUE(n == 2);

    EXPECT_TRUE(sl.get<const int>("const_integer_g") == 777);

    auto inc = sl.get<int(int)>("increment");
    EXPECT_TRUE(inc(1) == 2);
    EXPECT_TRUE(inc(2) == 3);
    EXPECT_TRUE(inc(3) == 4);

    // Checking that symbols are still available, after another load+unload of the library
    {
        midll::shared_library sl2(path);
    }

    EXPECT_TRUE(inc(1) == 2);
    EXPECT_TRUE(sl.get<int>("integer_g") == 200);

    // test alias
    auto sz = sl.get_alias<std::size_t(const std::vector<int>&)>("foo_bar");
    std::vector<int> v(10);
    EXPECT_TRUE(sz(v) == 10);
    EXPECT_TRUE(sl.get_alias<std::size_t>("foo_variable") == 42);

    sz = sl.get<std::size_t (*)(const std::vector<int>&)>("foo_bar");
    EXPECT_TRUE(sz(v) == 10);
    EXPECT_TRUE(*sl.get<std::size_t*>("foo_variable") == 42);

    int& reference_to_internal_integer = sl.get<int&>("reference_to_internal_integer");
    EXPECT_TRUE(reference_to_internal_integer == 0xFF0000);

    int&& rvalue_reference_to_internal_integer = sl.get<int&&>("rvalue_reference_to_internal_integer");
    EXPECT_TRUE(rvalue_reference_to_internal_integer == 0xFF0000);
}
