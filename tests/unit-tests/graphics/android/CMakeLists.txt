list(APPEND UNIT_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/test_buffer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_android_buffer_allocator.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_buffer_tex_bind.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_android_fb.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_android_display_factory.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_android_framebuffer_window.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_android_alloc_adaptor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_android_alloc_adaptor_native_win.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_android_buffer_allocator.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_hwc_device.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_hwc_display.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_gpu_display.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_framebuffer_factory.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_fb_simple_swapper.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_fb_device.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_hwc_layerlist.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_server_interpreter.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_pixel_format.cpp
)

set(UNIT_TEST_SOURCES ${UNIT_TEST_SOURCES} PARENT_SCOPE)

