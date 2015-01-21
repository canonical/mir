include_directories(
    ${server_common_include_dirs}
)

include_directories(SYSTEM ${LIBHARDWARE_INCLUDE_DIRS})
include_directories(
    ${EGL_INCLUDE_DIRS}
    ${GLESv2_INCLUDE_DIRS}
    ${ANDROID_PROPERTIES_INCLUDE_DIRS}
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive")
add_definitions( -DANDROID )

add_library(mirplatformgraphicsandroidobjects OBJECT
  platform.cpp
  android_buffer_allocator.cpp
  buffer.cpp
  display.cpp
  display_configuration.cpp
  display_buffer.cpp
  hal_component_factory.cpp
  hwc_layerlist.cpp
  hwc_layers.cpp
  hwc_fb_device.cpp
  hwc_loggers.cpp
  hwc_device.cpp
  android_alloc_adaptor.cpp
  server_render_window.cpp
  resource_factory.cpp
  framebuffers.cpp
  fb_device.cpp
  interpreter_cache.cpp
  gl_context.cpp
  device_quirks.cpp
  real_hwc_wrapper.cpp
  hwc_fallback_gl_renderer.cpp
  ipc_operations.cpp
  hwc_blanking_control.cpp
)
add_library(mirplatformgraphicsandroid SHARED
  $<TARGET_OBJECTS:mirplatformgraphicsandroidobjects>
)

set_target_properties(
  mirplatformgraphicsandroid PROPERTIES
  OUTPUT_NAME ${MIR_PLATFORM_DRIVER}
  LIBRARY_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_PATH}/android
  LINK_FLAGS "-Wl,--exclude-libs=ALL -Wl,--version-script,${server_symbol_map}"
)

target_link_libraries(mirplatformgraphicsandroid
  mirplatform
  mirsharedandroid
  ${Boost_PROGRAM_OPTIONS_LIBRARY}
  ${LIBHARDWARE_LIBRARIES}
  ${EGL_LDFLAGS} ${EGL_LIBRARIES}
  ${GLESv2_LDFLAGS} ${GLESv2_LIBRARIES}
  ${ANDROID_PROPERTIES_LDFLAGS}
)

install(TARGETS mirplatformgraphicsandroid LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/mir/${PLATFORM_DRIVER}/android)

if (MIR_TEST_PLATFORM STREQUAL "android")
  add_custom_command(TARGET mirplatformgraphicsandroid
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E remove ${MIR_PLATFORM_DRIVER_BINARY}
    COMMAND ${CMAKE_COMMAND} -E create_symlink android/$<TARGET_FILE_NAME:mirplatformgraphicsandroid> ${MIR_PLATFORM_DRIVER_BINARY}
    WORKING_DIRECTORY ${LIBRARY_OUTPUT_PATH}
  )

  install(CODE
    "execute_process(
       COMMAND ln -sf mir/${PLATFORM_DRIVER}/android/${MIR_PLATFORM_DRIVER_BINARY}
       WORKING_DIRECTORY \$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}
     )"
  )
endif()
