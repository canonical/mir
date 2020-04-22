add_library(mirplatformgraphicsrpidispmanxobjects OBJECT
  platform_symbols.cpp
  platform.cpp
  platform.h
  display_platform.cpp
  display_platform.h
  rendering_platform.cpp
  rendering_platform.h
  buffer_allocator.cpp
  buffer_allocator.h
  display.cpp
  display.h
  display_buffer.cpp
  display_buffer.h
  helpers.h helpers.cpp)

target_include_directories(
  mirplatformgraphicsrpidispmanxobjects
    PRIVATE
      ${BCM_HOST_INCLUDE_DIRS}
      ${server_common_include_dirs}
      ${PROJECT_SOURCE_DIR}/src/include/common
      ${WAYLAND_SERVER_INCLUDE_DIRS}
      ${UDEV_INCLUDE_DIRS}
)

target_compile_options(
  mirplatformgraphicsrpidispmanxobjects
    PRIVATE
      -Wno-error=variadic-macros
)

add_library(
  mirplatformgraphicsrpidispmanx MODULE

  $<TARGET_OBJECTS:mirplatformgraphicsrpidispmanxobjects>
)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/symbols.map.in
  ${CMAKE_CURRENT_BINARY_DIR}/symbols.map
)
set(symbol_map ${CMAKE_CURRENT_BINARY_DIR}/symbols.map)

target_link_libraries(
  mirplatformgraphicsrpidispmanx
  PRIVATE
  mirplatform
  server_platform_common
  ${Boost_PROGRAM_OPTIONS_LIBRARY}
  ${Boost_IOSTREAMS_LIBRARY}
  ${Boost_THREAD_LIBRARY}
  ${BCM_HOST_LDFLAGS} ${BCM_HOST_LIBRARIES}
  ${WAYLAND_SERVER_LDFLAGS} ${WAYLAND_SERVER_LIBRARIES}
)

set_target_properties(
  mirplatformgraphicsrpidispmanx PROPERTIES
  OUTPUT_NAME graphics-rpi-dispmanx
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/server-modules
  PREFIX ""
  SUFFIX ".so.${MIR_SERVER_GRAPHICS_PLATFORM_ABI}"
  LINK_FLAGS "-Wl,--exclude-libs=ALL -Wl,--version-script,${symbol_map}"
  LINK_DEPENDS ${symbol_map}
)

install(TARGETS mirplatformgraphicsrpidispmanx LIBRARY DESTINATION ${MIR_SERVER_PLATFORM_PATH})

