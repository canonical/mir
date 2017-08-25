pkg_check_modules(XMLPP libxml++-2.6 REQUIRED)

include_directories(SYSTEM ${XMLPP_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(wrapper-generator

  wrapper_generator.cpp
)

target_link_libraries(wrapper-generator

  ${XMLPP_LDFLAGS} ${XMLPP_LIBRARIES}
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

add_custom_target(wayland-headers DEPENDS core_generated_interfaces.h)
