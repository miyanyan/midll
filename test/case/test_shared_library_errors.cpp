#include <iostream>

#include <gtest/gtest.h>

#include <midll/midll.hpp>

TEST(test_shared_library_error, error)
{
    midll::fs::path bad_path("some/path/that/does/not/exist");
    auto good_path = midll::shared_library::decorate("test_library");

    try {
        midll::shared_library lib(bad_path);
    }
    catch (const midll::fs::system_error& e) {
        std::cout << e.what() << "\n";
    }

    try {
        midll::shared_library lib;
        lib.get<int>("variable_or_function_that_does_not_exist");
    }
    catch (const midll::fs::system_error& e) {
        std::cout << e.what() << "\n";
    }

    try {
        midll::shared_library lib("");
    }
    catch (const midll::fs::system_error& e) {
        std::cout << e.what() << '\n';
    }

    try {
        midll::shared_library lib("\0\0");
    }
    catch (const midll::fs::system_error& e) {
        std::cout << e.what() << '\n';
    }

    try {
        midll::shared_library lib;
        lib.location();
    }
    catch (const midll::fs::system_error& e) {
        std::cout << e.what() << '\n';
    }

    try {
        midll::shared_library lib;
        lib.load("\0\0", midll::load_mode::rtld_global);
    }
    catch (const midll::fs::system_error& e) {
        std::cout << e.what() << '\n';
    }

    midll::shared_library sl(good_path);
    try {
        sl.get<int>("variable_or_function_that_does_not_exist");
    }
    catch (const midll::fs::system_error& e) {
        std::cout << e.what() << '\n';
    }

    try {
        midll::library_info lib("\0");
    }
    catch (const std::exception& e) {
        std::cout << e.what() << '\n';
    }

    try {
        std::string not_a_binary("not_a_binary");
        std::ofstream ofs(not_a_binary.c_str());
        ofs << "This is not a binary file, so library_info must report 'Unsupported binary format'";
        ofs.close();
        midll::library_info lib(not_a_binary);
    }
    catch (const std::exception& e) {
        std::cout << e.what() << '\n';
    }
}