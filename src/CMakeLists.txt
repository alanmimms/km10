add_executable(km10 km10.cpp)
add_library(km10lib bytepointer.cpp)
target_link_libraries(${PROJECT_NAME} PUBLIC km10lib PRIVATE gflags::gflags fmt::fmt)
