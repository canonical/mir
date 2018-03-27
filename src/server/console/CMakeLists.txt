include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_library(
  mirconsole OBJECT

  linux_virtual_terminal.cpp                linux_virtual_terminal.h
  null_console_services.h
  default_configuration.cpp
)
