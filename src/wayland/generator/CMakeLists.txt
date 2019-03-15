pkg_check_modules(XMLPP libxml++-2.6 REQUIRED)

include_directories(SYSTEM ${XMLPP_INCLUDE_DIRS})

add_executable(wrapper-generator
  wrapper_generator.cpp
  utils.cpp                 utils.h
  enum.cpp                  enum.h
  argument.cpp              argument.h
  method.cpp                method.h
  request.cpp               request.h
  event.cpp                 event.h
  interface.cpp             interface.h
  emitter.cpp               emitter.h
)

target_link_libraries(wrapper-generator
  ${XMLPP_LDFLAGS} ${XMLPP_LIBRARIES}
)
