cmake_minimum_required(VERSION 3.14)
project(km10)

add_compile_options(-Wall -g -fdiagnostics-color=always)

# Require at least C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(CLI
    QUIET
    GIT_REPOSITORY "https://github.com/CLIUtils/CLI11"
    GIT_TAG "v2.4.2"
)

FetchContent_Declare(googletest
    GIT_REPOSITORY "https://github.com/google/googletest.git"
    GIT_TAG "v1.15.2"
)

include(GoogleTest)
enable_testing()

FetchContent_MakeAvailable(CLI googletest)

add_subdirectory(src)
add_subdirectory(tests)
