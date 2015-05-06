include_directories(
  ${server_common_include_dirs}
  ${PROJECT_SOURCE_DIR}/include/platform
  ${PROJECT_SOURCE_DIR}/include/client
)

include_directories(
    ${DRM_INCLUDE_DIRS}
    ${GBM_INCLUDE_DIRS}
    ${EGL_INCLUDE_DIRS}
    ${GLESv2_INCLUDE_DIRS}
    ${UDEV_INCLUDE_DIRS}
)

# gbm.h and drm.h have trailing commas at the end of enum definitions
# This is valid C99, but g++ 4.4 flags it as an error with -pedantic
#string(REPLACE "-pedantic" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
#add_definitions(-D__GBM__)
add_definitions(-DDEBUG_SERVER=1)

add_library(
  mirplatformserverXobjects OBJECT

  platform.cpp
  display.cpp
  display_configuration.cpp
  gl_context.cpp
  buffer_allocator.cpp
  anonymous_shm_file.cpp
  shm_buffer.cpp
  ipc_operations.cpp
  display_group.cpp
  display_buffer.cpp
  gbm_buffer.cpp
  display_helpers.cpp
  drm_mode_resources.cpp
  drm_close_threadsafe.cpp
  input/X_input.cpp
  input/X_input_platform.cpp
  input/X_input_device.cpp
  input/X_dispatchable.cpp
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
  ${Boost_PROGRAM_OPTIONS_LIBRARY}
  ${DRM_LDFLAGS} ${DRM_LIBRARIES}
  ${GBM_LDFLAGS} ${GBM_LIBRARIES}
#  ${EGL_LDFLAGS} ${EGL_LIBRARIES}
#  ${GLESv2_LDFLAGS} ${GLESv2_LIBRARIES}
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
