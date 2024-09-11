#include <iostream>

#include <midll/midll.hpp>

#define LIBRARY_API MIDLL_SYMBOL_EXPORT

extern "C" void LIBRARY_API say_hello()
{
    std::cout << "Hello hello hello!" << std::endl;
}

extern "C" float LIBRARY_API lib_version()
{
    return 1.0;
}

extern "C" int LIBRARY_API increment(int n)
{
    return ++n;
}

#if defined(__GNUC__) && __GNUC__ >= 4 && defined(__ELF__)
extern "C" int __attribute__((visibility("protected"))) protected_function(int)
{
    return 42;
}
#endif

extern "C" int LIBRARY_API integer_g = 100;
extern "C" const int LIBRARY_API const_integer_g = 777;

namespace foo
{
std::size_t bar(const std::vector<int>& v)
{
    return v.size();
}

std::size_t variable = 42;
} // namespace foo

// Make sure that aliases have no problems with memory allocations and different types of input parameters
namespace namespace1
{
namespace namespace2
{
namespace namespace3
{
using do_share_res_t =
    std::tuple<std::vector<int>, std::vector<int>, std::vector<int>, const std::vector<int>*, std::vector<int>*>;

std::shared_ptr<do_share_res_t> do_share(std::vector<int> v1, std::vector<int>& v2, const std::vector<int>& v3,
                                         const std::vector<int>* v4, std::vector<int>* v5)
{
    v2.back() = 777;
    v5->back() = 9990;
    return std::make_shared<do_share_res_t>(v1, v2, v3, v4, v5);
}

std::string info("I am a std::string from the test_library (Think of me as of 'Hello world'. Long 'Hello world').");

int& ref_returning_function()
{
    static int i = 0;
    return i;
}
} // namespace namespace3
} // namespace namespace2
} // namespace namespace1

MIDLL_ALIAS(foo::bar, foo_bar)
MIDLL_ALIAS(foo::variable, foo_variable)
MIDLL_ALIAS(namespace1::namespace2::namespace3::do_share, do_share)
MIDLL_ALIAS(namespace1::namespace2::namespace3::info, info)
MIDLL_ALIAS(const_integer_g, const_integer_g_alias)
MIDLL_ALIAS(namespace1::namespace2::namespace3::ref_returning_function, ref_returning_function)

midll::fs::path this_module_location_from_itself()
{
    return midll::this_line_location();
}

MIDLL_ALIAS(this_module_location_from_itself, module_location_from_itself)

int internal_integer_i = 0xFF0000;
extern "C" LIBRARY_API int& reference_to_internal_integer = internal_integer_i;
extern "C" LIBRARY_API int&& rvalue_reference_to_internal_integer = static_cast<int&&>(internal_integer_i);
