cmake_minimum_required(VERSION 3.14)
project(km10)

set(CPPOptionsForALL -Wall -g)

# Require at least C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(gflags
    GIT_REPOSITORY "https://github.com/gflags/gflags.git"
    GIT_TAG "origin/master"
)

FetchContent_Declare(googletest
    GIT_REPOSITORY "https://github.com/google/googletest.git"
    GIT_TAG "origin/main"
)
FetchContent_MakeAvailable(gflags googletest)

include(GoogleTest)
enable_testing()

add_executable(km10 km10.cpp)
target_compile_options(km10 PUBLIC ${CPPOptionsForALL})
target_link_libraries(${PROJECT_NAME} PRIVATE gflags::gflags GTest::gtest_main )

add_executable(km10-test km10-test.cpp)
target_compile_options(km10-test PUBLIC ${CPPOptionsForALL})
target_link_libraries(km10-test PRIVATE gflags::gflags GTest::gtest_main )

#gtest_discover_tests(km10-test)
