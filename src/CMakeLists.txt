add_executable(km10
  apr.cpp
  cca.cpp
  debugger.cpp
  device.cpp
  dte20.cpp
  km10.cpp
  logger.cpp
  mtr.cpp
  pag.cpp
  pi.cpp
  tim.cpp
  word.cpp
)

add_library(km10lib
  bytepointer.cpp
)

target_link_libraries(${PROJECT_NAME} PUBLIC km10lib PRIVATE CLI11::CLI11 fmt::fmt)
