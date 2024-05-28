cmake_minimum_required(VERSION 3.14)
project(km10)

set(COptionsForALL -Wall -g)
set(CPPOptionsForALL -Wall -g)

include(FetchContent)

FetchContent_Declare(replxx
    GIT_REPOSITORY "https://github.com/AmokHuginnsson/replxx.git"
    GIT_TAG "origin/master"
)
FetchContent_MakeAvailable(replxx)
target_include_directories(replxx PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include)

FetchContent_Declare(gflags
    GIT_REPOSITORY "https://github.com/gflags/gflags.git"
    GIT_TAG "origin/master"
)
FetchContent_MakeAvailable(gflags)

FetchContent_Declare(gtest
    GIT_REPOSITORY "https://github.com/google/googletest.git"
    GIT_TAG "origin/main"
)
FetchContent_MakeAvailable(gtest)

add_executable(km10 km10.cpp)
target_compile_options(km10 PUBLIC --std=c++20 -DTEST_KM10=1 ${CPPOptionsForALL})
target_link_libraries(${PROJECT_NAME} PRIVATE replxx::replxx gflags::gflags)
