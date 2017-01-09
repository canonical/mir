include_directories(
  ${PROJECT_SOURCE_DIR}/src/include/client
)

add_library(acceptance-test-staging OBJECT
  test_presentation_chain.cpp
  test_render_surface.cpp
  test_buffer_stream_arrangement.cpp
)
