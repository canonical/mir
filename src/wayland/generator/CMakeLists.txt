pkg_check_modules(XMLPP libxml++-2.6 REQUIRED IMPORTED_TARGET)

add_executable(mir_wayland_generator
  wrapper_generator.cpp
  utils.cpp                 utils.h
  enum.cpp                  enum.h
  argument.cpp              argument.h
  method.cpp                method.h
  request.cpp               request.h
  event.cpp                 event.h
  interface.cpp             interface.h
  global.cpp                global.h
  emitter.cpp               emitter.h
)

target_link_libraries(mir_wayland_generator
  PkgConfig::XMLPP
)

install(TARGETS mir_wayland_generator
    DESTINATION ${CMAKE_INSTALL_BINDIR}
)
