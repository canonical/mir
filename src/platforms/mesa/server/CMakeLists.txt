include_directories(
    ${server_common_include_dirs}
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

add_library(mirplatformgraphicsmesaobjects OBJECT
  platform.cpp
  buffer_allocator.cpp
  gbm_buffer.cpp
  cursor.cpp
  display_helpers.cpp
  display.cpp
  real_kms_display_configuration.cpp
  drm_mode_resources.cpp
  display_buffer.cpp
  real_kms_output.cpp
  real_kms_output_container.cpp
  kms_page_flipper.cpp
  linux_virtual_terminal.cpp
  drm_close_threadsafe.cpp
  guest_platform.cpp
  anonymous_shm_file.cpp
  shm_buffer.cpp
  bypass.cpp
  ipc_operations.cpp
  nested_authentication.cpp
)

add_library(mirplatformgraphicsmesa SHARED
  $<TARGET_OBJECTS:mirplatformgraphicsmesaobjects>
)

set_target_properties(
  mirplatformgraphicsmesa PROPERTIES
  OUTPUT_NAME ${MIR_PLATFORM_DRIVER}
  LIBRARY_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_PATH}/mesa
  LINK_FLAGS "-Wl,--exclude-libs=ALL -Wl,--version-script,${server_symbol_map}"
)

target_link_libraries(mirplatformgraphicsmesa
  mirplatform
  ${Boost_PROGRAM_OPTIONS_LIBRARY}
  ${DRM_LDFLAGS} ${DRM_LIBRARIES}
  ${GBM_LDFLAGS} ${GBM_LIBRARIES}
  ${EGL_LDFLAGS} ${EGL_LIBRARIES}
  ${GLESv2_LDFLAGS} ${GLESv2_LIBRARIES}
)

install(TARGETS mirplatformgraphicsmesa LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/mir/${PLATFORM_DRIVER}/mesa)

if (MIR_TEST_PLATFORM STREQUAL "mesa")
  add_custom_command(TARGET mirplatformgraphicsmesa
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E remove ${MIR_PLATFORM_DRIVER_BINARY}
    COMMAND ${CMAKE_COMMAND} -E create_symlink mesa/$<TARGET_FILE_NAME:mirplatformgraphicsmesa> ${MIR_PLATFORM_DRIVER_BINARY}
    WORKING_DIRECTORY ${LIBRARY_OUTPUT_PATH}
  )

  install(CODE
    "execute_process(
       COMMAND ln -sf mir/${PLATFORM_DRIVER}/mesa/${MIR_PLATFORM_DRIVER_BINARY}
       WORKING_DIRECTORY \$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}
     )"
  )
endif()
