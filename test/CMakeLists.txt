# test library
add_library(empty_library SHARED lib/empty_library.cpp)

add_library(test_library SHARED lib/test_library.cpp)
target_link_libraries(test_library PRIVATE midll)

# test case
file(GLOB_RECURSE source CONFIGURE_DEPENDS case/*.h case/*.cpp)
find_package(GTest CONFIG REQUIRED)
add_executable(test ${source})
target_link_libraries(test PRIVATE midll)
target_link_libraries(test PRIVATE GTest::gtest)