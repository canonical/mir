include_directories(SYSTEM ${LIBHARDWARE_INCLUDE_DIRS})
include_directories(
    ${EGL_INCLUDE_DIRS}
    ${GLESv2_INCLUDE_DIRS}
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive")
add_definitions( -DANDROID )

add_library(
  mirplatformgraphicsandroid SHARED

  android_platform.cpp
  android_buffer_allocator.cpp
  buffer.cpp
  android_display.cpp
  android_display_configuration.cpp
  display_buffer.cpp
  output_builder.cpp
  hwc_layerlist.cpp
  hwc_layers.cpp
  hwc_fb_device.cpp
  hwc_device.cpp
  hwc_common_device.cpp
  hwc_vsync.cpp
  android_alloc_adaptor.cpp
  server_render_window.cpp
  resource_factory.cpp
  framebuffers.cpp
  fb_device.cpp
  internal_client_window.cpp
  interpreter_cache.cpp
  internal_client.cpp
  gl_context.cpp
)

set_target_properties(
  mirplatformgraphicsandroid PROPERTIES
  OUTPUT_NAME mirplatformgraphics
  LIBRARY_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_PATH}/android
)

target_link_libraries(
  mirplatformgraphicsandroid

  mirplatform
  mirsharedandroid

  ${LIBHARDWARE_LIBRARIES}
  ${EGL_LDFLAGS} ${EGL_LIBRARIES}
  ${GLESv2_LDFLAGS} ${GLESv2_LIBRARIES}
)

install(TARGETS mirplatformgraphicsandroid LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/mir/platformgraphics/android)

if (MIR_TEST_PLATFORM STREQUAL "android")
  add_custom_command(TARGET mirplatformgraphicsandroid
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E remove libmirplatformgraphics.so
    COMMAND ${CMAKE_COMMAND} -E create_symlink android/$<TARGET_FILE_NAME:mirplatformgraphicsandroid> libmirplatformgraphics.so
    WORKING_DIRECTORY ${LIBRARY_OUTPUT_PATH}
  )

  install(CODE
    "execute_process(
       COMMAND ln -sf mir/platformgraphics/android/libmirplatformgraphics.so
       WORKING_DIRECTORY \$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}
     )"
  )
endif()
