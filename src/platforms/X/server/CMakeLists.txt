include_directories(
    ${server_common_include_dirs}
)

#include_directories(
#    ${DRM_INCLUDE_DIRS}
#    ${GBM_INCLUDE_DIRS}
#    ${EGL_INCLUDE_DIRS}
#    ${GLESv2_INCLUDE_DIRS}
#    ${UDEV_INCLUDE_DIRS}
#)

# gbm.h and drm.h have trailing commas at the end of enum definitions
# This is valid C99, but g++ 4.4 flags it as an error with -pedantic
#string(REPLACE "-pedantic" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
#add_definitions(-D__GBM__)

add_library(
  mirplatformgraphicsXobjects OBJECT

  platform.cpp
  display.cpp
  display_configuration.cpp
  gl_context.cpp
  buffer_allocator.cpp
  anonymous_shm_file.cpp
  shm_buffer.cpp
  ipc_operations.cpp
)

add_library(
  mirplatformgraphicsX MODULE

  $<TARGET_OBJECTS:mirplatformgraphicsXobjects>
)

target_link_libraries(
  mirplatformgraphicsX

  mirplatform
  GL
#  ${Boost_PROGRAM_OPTIONS_LIBRARY}
#  ${DRM_LDFLAGS} ${DRM_LIBRARIES}
#  ${GBM_LDFLAGS} ${GBM_LIBRARIES}
#  ${EGL_LDFLAGS} ${EGL_LIBRARIES}
#  ${GLESv2_LDFLAGS} ${GLESv2_LIBRARIES}
)

set(symbol_map ${CMAKE_CURRENT_SOURCE_DIR}/symbols.map)

set_target_properties(
  mirplatformgraphicsX PROPERTIES
  OUTPUT_NAME graphics-X
  LIBRARY_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_PATH}/server-modules
  PREFIX ""
  SUFFIX ".so.${MIR_SERVER_GRAPHICS_PLATFORM_ABI}"
  LINK_FLAGS "-Wl,--exclude-libs=ALL -Wl,--version-script,${symbol_map}"
)

install(TARGETS mirplatformgraphicsX LIBRARY DESTINATION ${MIR_SERVER_PLATFORM_PATH})
