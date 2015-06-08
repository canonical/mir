include_directories(
    ${server_common_include_dirs}
    ${PROJECT_SOURCE_DIR}/src/platforms/mesa/server/common
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
string(REPLACE "-pedantic" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
add_definitions(-D__GBM__)

add_library(
  mirplatformgraphicsmesakmsobjects OBJECT

  bypass.cpp
  cursor.cpp
  display.cpp
  display_buffer.cpp
  guest_platform.cpp
  kms_page_flipper.cpp
  linux_virtual_terminal.cpp
  nested_authentication.cpp
  platform.cpp
  real_kms_display_configuration.cpp
  real_kms_output.cpp
  real_kms_output_container.cpp
)

add_library(
  mirplatformgraphicsmesakms MODULE

  $<TARGET_OBJECTS:mirplatformgraphicsmesakmsobjects>
)

target_link_libraries(
  mirplatformgraphicsmesakms
  PRIVATE
  mirplatform
  mirsharedmesaservercommon-static
  ${Boost_PROGRAM_OPTIONS_LIBRARY}
  ${DRM_LDFLAGS} ${DRM_LIBRARIES}
  ${GBM_LDFLAGS} ${GBM_LIBRARIES}
  ${EGL_LDFLAGS} ${EGL_LIBRARIES}
  ${GLESv2_LDFLAGS} ${GLESv2_LIBRARIES}
)

set(symbol_map ${CMAKE_CURRENT_SOURCE_DIR}/symbols.map)

set_target_properties(
  mirplatformgraphicsmesakms PROPERTIES
  OUTPUT_NAME graphics-mesa-kms
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/server-modules
  PREFIX ""
  SUFFIX ".so.${MIR_SERVER_GRAPHICS_PLATFORM_ABI}"
  LINK_FLAGS "-Wl,--exclude-libs=ALL -Wl,--version-script,${symbol_map}"
)

install(TARGETS mirplatformgraphicsmesakms LIBRARY DESTINATION ${MIR_SERVER_PLATFORM_PATH})
