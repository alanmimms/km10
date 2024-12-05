add_executable(km10-test km10-test.cpp test-fixed.cpp test-move.cpp ../src/bytepointer.cpp)

add_compile_options(-Woverloaded-virtual=1)

include_directories(km10-test PUBLIC ../src)
link_directories(../src)
target_link_libraries(km10-test PUBLIC km10lib PRIVATE GTest::gtest_main fmt::fmt)
