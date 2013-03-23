list(APPEND UNIT_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/test_android_buffer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_android_buffer_allocator.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_android_buffer_tex_bind.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_android_fb.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_android_framebuffer_selector.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_android_framebuffer_window.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_android_alloc_adaptor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_android_alloc_adaptor_native_win.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_android_buffer_allocator.cpp
)

set(UNIT_TEST_SOURCES ${UNIT_TEST_SOURCES} PARENT_SCOPE)

