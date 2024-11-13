cpmaddpackage(
  NAME
  googletest
  GITHUB_REPOSITORY
  google/googletest
  GIT_TAG
  v1.15.2
  VERSION
  1.15.2)

include(GoogleTest)

add_executable(unittests test_pdo_map.cpp)
target_link_libraries(unittests PRIVATE GTest::gtest_main canopen-stack-master)

gtest_add_tests(TARGET unittests)
