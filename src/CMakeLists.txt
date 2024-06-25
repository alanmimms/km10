include_directories(../include)
link_directories(../lib)
add_executable(km10 km10.cpp)
target_link_libraries(${PROJECT_NAME} PUBLIC km10lib PRIVATE gflags::gflags fmt::fmt)
