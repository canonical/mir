list(APPEND UNIT_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/hwc_struct_helpers.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_buffer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_sync_fence.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_android_buffer_allocator.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_buffer_tex_bind.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_android_fb.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_android_alloc_adaptor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_android_buffer_allocator.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_hwc_device.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_hwc_fb_device.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_hwc_common_device.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_hwc_display.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_internal_client_interpreter.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_internal_client.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_resource_factory.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_fb_simple_swapper.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_fb_device.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_hwc_layers.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_hwc_layerlist.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_server_interpreter.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_pixel_format.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_android_platform.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_interpreter_buffer_cache.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_external_refcount.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_output_builder.cpp
)

set(UNIT_TEST_SOURCES ${UNIT_TEST_SOURCES} PARENT_SCOPE)

