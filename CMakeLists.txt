cmake_minimum_required(VERSION 3.14)
project(km10)

add_compile_options(-Wall -g -fdiagnostics-color=always)

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

include(GoogleTest)
enable_testing()

FetchContent_Declare(fmt
    GIT_REPOSITORY "https://github.com/fmtlib/fmt.git"
    GIT_TAG "10.2.1"
)

FetchContent_MakeAvailable(gflags googletest fmt)

add_subdirectory(src)
add_subdirectory(lib)
add_subdirectory(tests)
