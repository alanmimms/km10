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

add_custom_command(
  OUTPUT ${PROJECT_SOURCE_DIR}/km10.c
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/compiler
  COMMAND ./compile10 > ${PROJECT_SOURCE_DIR}/km10.c
  DEPENDS ${PROJECT_SOURCE_DIR}/compiler/compile10
  DEPENDS ${PROJECT_SOURCE_DIR}/compiler/compile10.js
)

add_executable(km10 km10.c loada10.c disasm.c)
target_compile_options(km10 PUBLIC -DTEST_KM10=1 ${COptionsForALL})

