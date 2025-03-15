find_package(GTest REQUIRED)
include(GoogleTest)

add_executable(unittests test_pdo_map.cpp)
target_link_libraries(unittests PRIVATE GTest::gtest_main canopen-stack-master)

gtest_add_tests(TARGET unittests)
