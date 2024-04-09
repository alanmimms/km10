cmake_minimum_required(VERSION 3.0)
project(km10)

set(COptionsForALL -Wall -g)

include_directories("${PROJECT_SOURCE_DIR}/acutest/include")

add_executable(test-kl10 test-kl10.c)
target_compile_options(test-kl10 PUBLIC -DTEST_KL10=1 ${COptionsForALL})

add_executable(disasm disasm.c)
target_compile_options(disasm PUBLIC -DTEST_DISASM=1 ${COptionsForALL})

add_executable(loada10 loada10.c disasm.c)
target_compile_options(loada10 PUBLIC -DTEST_LOADA10=1 ${COptionsForALL})
