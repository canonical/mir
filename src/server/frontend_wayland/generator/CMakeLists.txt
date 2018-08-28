pkg_check_modules(XMLPP libxml++-2.6 REQUIRED)

include_directories(SYSTEM ${XMLPP_INCLUDE_DIRS})

add_executable(wayland-wrapper-generator
  wrapper_generator.cpp
  utils.cpp                 utils.h
  argument.cpp              argument.h
  method.cpp                method.h
  interface.cpp             interface.h
  emitter.cpp               emitter.h
)

target_link_libraries(wayland-wrapper-generator
  ${XMLPP_LDFLAGS} ${XMLPP_LIBRARIES}
)
