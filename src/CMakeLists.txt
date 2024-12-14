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
  km10.cpp
  i-byte.cpp
  i-move.cpp
)

add_library(km10lib
  bytepointer.cpp
)

target_link_libraries(${PROJECT_NAME} PUBLIC km10lib PRIVATE CLI11::CLI11 fmt::fmt)
