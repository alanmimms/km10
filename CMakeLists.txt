cmake_minimum_required(VERSION 3.0)
project(km10)

add_executable(loada10 loada10.c)
target_compile_options(loada10 PUBLIC -DTEST=1 -Wall -g)
