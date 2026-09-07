pkg_check_modules(XMLPP libxml++-5.0 IMPORTED_TARGET)

# Alpine 3.24 (and, at the time of writing, edge) has libxml++-2.6, but not libxml++-5.0, so fall back to that
if (NOT XMLPP_FOUND)
    message(STATUS "libxml++-5.0 not found, falling back to libxml++-2.6")
    pkg_check_modules(XMLPP libxml++-2.6 REQUIRED IMPORTED_TARGET)
endif()

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
