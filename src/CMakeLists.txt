add_executable(km10
  apr.cpp
  cca.cpp
  debugger.cpp
  device.cpp
  dte20.cpp
  mtr.cpp
  pag.cpp
  pi.cpp
  tim.cpp
  word.cpp
  logger.cpp
  i-aoxsox.cpp
  i-bitrot.cpp
  i-byte.cpp
  i-cmpand.cpp
  i-dword.cpp
  i-half.cpp
  i-incjs.cpp
  i-intbin.cpp
  i-io.cpp
  i-jump.cpp
  i-move.cpp
  i-muldiv.cpp
  i-tstset.cpp
  i-uuos.cpp
  km10.cpp
)

add_library(km10lib
  bytepointer.cpp
)

target_link_libraries(${PROJECT_NAME} PUBLIC km10lib PRIVATE CLI11::CLI11)
