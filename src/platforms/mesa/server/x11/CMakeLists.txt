include_directories(
  ${server_common_include_dirs}
  ${PROJECT_SOURCE_DIR}/include/platform
  ${PROJECT_SOURCE_DIR}/include/client
  ${PROJECT_SOURCE_DIR}/src/platforms/mesa/server/common
)

include_directories(
    ${DRM_INCLUDE_DIRS}
    ${GBM_INCLUDE_DIRS}
    ${EGL_INCLUDE_DIRS}
    ${GLESv2_INCLUDE_DIRS}
    ${UDEV_INCLUDE_DIRS}
)

add_library(
  mirplatformservermesax11objects OBJECT

  graphics.cpp
  platform.cpp
  display.cpp
  display_configuration.cpp
  gl_context.cpp
  display_group.cpp
  display_buffer.cpp
  input/input.cpp
  input/input_platform.cpp
  input/input_device.cpp
  input/dispatchable.cpp
)

add_library(
  mirplatformservermesax11 MODULE

  $<TARGET_OBJECTS:mirplatformservermesax11objects>
)

target_link_libraries(
  mirplatformservermesax11
  PRIVATE
  mirplatform
  mirclient   # for make_event(). TODO: move make_event out of libmirclient?
  EGL
  GLESv2
  X11
  mirsharedmesaservercommon-static
  ${Boost_PROGRAM_OPTIONS_LIBRARY}
  ${DRM_LDFLAGS} ${DRM_LIBRARIES}
  ${GBM_LDFLAGS} ${GBM_LIBRARIES}
)

set(symbol_map ${CMAKE_CURRENT_SOURCE_DIR}/symbols.map)

set_target_properties(
  mirplatformservermesax11 PROPERTIES
  OUTPUT_NAME server-mesa-x11
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/server-modules
  PREFIX ""
  SUFFIX ".so.${MIR_SERVER_GRAPHICS_PLATFORM_ABI}"
  LINK_FLAGS "-Wl,--exclude-libs=ALL -Wl,--version-script,${symbol_map}"
)

install(TARGETS mirplatformservermesax11 LIBRARY DESTINATION ${MIR_SERVER_PLATFORM_PATH})
