cmake_minimum_required(VERSION 3.14)
project(km10)

set(COptionsForALL -Wall -g)
set(CPPOptionsForALL -Wall -g)

add_custom_command(
  OUTPUT ${PROJECT_SOURCE_DIR}/km10.c
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/jsc
  COMMAND ./js2c km10.jsc ${PROJECT_SOURCE_DIR}/km10.c
  DEPENDS ${PROJECT_SOURCE_DIR}/jsc/js2c
  DEPENDS ${PROJECT_SOURCE_DIR}/jsc/km10.jsc
)

include_directories("${PROJECT_SOURCE_DIR}/acutest/include")

add_executable(km10js km10.c loada10.c disasm.c)
target_compile_options(km10js PUBLIC -DTEST_KM10=1 ${COptionsForALL})

include(FetchContent)

FetchContent_Declare(cmdlime
    GIT_REPOSITORY "https://github.com/kamchatka-volcano/cmdlime.git"
    GIT_TAG "origin/master"
)

#uncomment if you need to install cmdlime with your target
#set(INSTALL_CMDLIME ON)
FetchContent_MakeAvailable(cmdlime)

FetchContent_Declare(replxx
    GIT_REPOSITORY "https://github.com/AmokHuginnsson/replxx.git"
    GIT_TAG "origin/master"
)
FetchContent_MakeAvailable(replxx)

add_executable(km10 km10.cpp)
target_compile_options(km10 PUBLIC --std=c++20 -DTEST_KM10=1 ${CPPOptionsForALL})
target_link_libraries(${PROJECT_NAME} PRIVATE cmdlime::cmdlime)

add_executable(test-kl10 test-kl10.c)
target_compile_options(test-kl10 PUBLIC -DTEST_KL10=1 ${COptionsForALL})

add_executable(disasm disasm.c)
target_compile_options(disasm PUBLIC -DTEST_DISASM=1 ${COptionsForALL})

add_executable(loada10 loada10.c disasm.c)
target_compile_options(loada10 PUBLIC -DTEST_LOADA10=1 ${COptionsForALL})
