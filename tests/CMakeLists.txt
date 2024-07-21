add_executable(km10-test km10-test.cpp test-fixed.cpp test-move.cpp)

add_compile_options(-Woverloaded-virtual=1)

link_directories(../lib)
include_directories(km10-test PUBLIC ../include)
target_link_libraries(km10-test PUBLIC km10lib PRIVATE gflags::gflags GTest::gtest_main fmt::fmt)
