include_directories(
  ${server_common_include_dirs}
  ${PROJECT_SOURCE_DIR}/include/platform
  ${PROJECT_SOURCE_DIR}/include/client
  ${PROJECT_SOURCE_DIR}/src/platforms/mesa/server/common
#  ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)

include_directories(
    ${DRM_INCLUDE_DIRS}
    ${GBM_INCLUDE_DIRS}
    ${EGL_INCLUDE_DIRS}
    ${GLESv2_INCLUDE_DIRS}
    ${UDEV_INCLUDE_DIRS}
)

add_definitions(-DDEBUG_SERVER=0)

add_library(
  mirplatformserverXobjects OBJECT

  platform.cpp
  display.cpp
  display_configuration.cpp
  gl_context.cpp
  display_group.cpp
  display_buffer.cpp
  input/X_input.cpp
  input/X_input_platform.cpp
  input/X_input_device.cpp
  input/X_dispatchable.cpp
#  ../KMS/buffer_allocator.cpp
#  ../KMS/drm_mode_resources.cpp
#  ../KMS/anonymous_shm_file.cpp
#  ../KMS/shm_buffer.cpp
#  ../KMS/ipc_operations.cpp
#  ../KMS/gbm_buffer.cpp
#  ../KMS/display_helpers.cpp
#  ../KMS/drm_close_threadsafe.cpp
)

add_library(
  mirplatformserverX MODULE

  $<TARGET_OBJECTS:mirplatformserverXobjects>
)

target_link_libraries(
  mirplatformserverX

  mirplatform
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
  mirplatformserverX PROPERTIES
  OUTPUT_NAME server-X
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/server-modules
  PREFIX ""
  SUFFIX ".so.${MIR_SERVER_GRAPHICS_PLATFORM_ABI}"
  LINK_FLAGS "-Wl,--exclude-libs=ALL -Wl,--version-script,${symbol_map}"
)

install(TARGETS mirplatformserverX LIBRARY DESTINATION ${MIR_SERVER_PLATFORM_PATH})
