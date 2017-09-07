pkg_check_modules(XMLPP libxml++-2.6 REQUIRED)

include_directories(SYSTEM ${XMLPP_INCLUDE_DIRS})

add_executable(wrapper-generator

  wrapper_generator.cpp
)

target_link_libraries(wrapper-generator

  ${XMLPP_LDFLAGS} ${XMLPP_LIBRARIES}
)

get_filename_component(
  GENERATED_HEADER src/server/frontend/wayland/core_generated_interfaces.h
  ABSOLUTE
  BASE_DIR ${PROJECT_SOURCE_DIR}
)

add_custom_target(refresh-wayland-wrapper
  COMMAND "sh" "-c" "${CMAKE_BINARY_DIR}/bin/wrapper-generator wl_ /usr/share/wayland/wayland.xml >${GENERATED_HEADER}"
  VERBATIM
  DEPENDS wrapper-generator
  DEPENDS /usr/share/wayland/wayland.xml
  SOURCES ${GENERATED_HEADER}
)

