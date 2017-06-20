pkg_check_modules(XMLPP libxml++-2.6 REQUIRED)

include_directories(SYSTEM ${XMLPP_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(wrapper-generator

  wrapper_generator.cpp
)

target_link_libraries(wrapper-generator

  ${XMLPP_LDFLAGS} ${XMLPP_LIBRARIES}
)

add_library(mirwaylandclient OBJECT
  mir_client_protocol.h

  mir_protocol.c
)

add_library(mirwaylandserver OBJECT
  core_generated_interfaces.h
  mir_generated_interfaces.h
  mir_server_protocol.h

  mir_protocol.c
)

# Export the include directories
list(APPEND MIR_GENERATED_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_BINARY_DIR})

set(MIR_GENERATED_INCLUDE_DIRECTORIES
  ${MIR_GENERATED_INCLUDE_DIRECTORIES}
  PARENT_SCOPE
)

add_custom_command(OUTPUT core_generated_interfaces.h
  COMMAND "sh" ARGS "-c" "${CMAKE_BINARY_DIR}/bin/wrapper-generator wl_ /usr/share/wayland/wayland.xml >core_generated_interfaces.h"
  DEPENDS wrapper-generator
  DEPENDS /usr/share/wayland/wayland.xml
)

add_custom_command(OUTPUT mir_generated_interfaces.h
  COMMAND "sh" ARGS "-c" "${CMAKE_BINARY_DIR}/bin/wrapper-generator mir_ ${CMAKE_CURRENT_SOURCE_DIR}/mir_protocol.xml >mir_generated_interfaces.h"
  DEPENDS wrapper-generator
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/mir_protocol.xml
)

add_custom_command(OUTPUT mir_protocol.c
  COMMAND "wayland-scanner" ARGS "code" "${CMAKE_CURRENT_SOURCE_DIR}/mir_protocol.xml" "mir_protocol.c"
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/mir_protocol.xml
)

add_custom_command(OUTPUT mir_server_protocol.h
  COMMAND "wayland-scanner" ARGS "server-header" "${CMAKE_CURRENT_SOURCE_DIR}/mir_protocol.xml" "mir_server_protocol.h"
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/mir_protocol.xml
)

add_custom_command(OUTPUT mir_client_protocol.h
  COMMAND "wayland-scanner" ARGS "client-header" "${CMAKE_CURRENT_SOURCE_DIR}/mir_protocol.xml" "mir_client_protocol.h"
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/mir_protocol.xml
)
