include_directories(
  ${CMAKE_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}/src/include/client
)

add_library(acceptance-test-staging OBJECT
  test_presentation_chain.cpp
  test_render_surface.cpp
)
