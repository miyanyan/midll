#include <gtest/gtest.h>

#ifdef _WIN32
#    include <Windows.h>
#endif // _WIN32

int main(int argc, char** argv)
{
#if _WIN32
    setlocale(LC_ALL, ".utf-8");
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
