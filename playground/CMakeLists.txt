
include_directories(
  ${PROJECT_SOURCE_DIR}/src/include/server
  ${PROJECT_SOURCE_DIR}/src/include/platform
  ${PROJECT_SOURCE_DIR}/src/include/common
  ${PROJECT_SOURCE_DIR}/src/include/gl
  ${PROJECT_SOURCE_DIR}/src/include/client
  ${PROJECT_SOURCE_DIR}/src/renderers
  ${PROJECT_SOURCE_DIR}/include/client
  ${PROJECT_SOURCE_DIR}/include/server
  ${PROJECT_SOURCE_DIR}/include/platform
  ${PROJECT_SOURCE_DIR}/include/renderers/gl
  ${PROJECT_SOURCE_DIR}/examples/
)

add_library(playgroundserverconfig STATIC
    server_configuration.cpp
)

add_subdirectory(demo-shell/)

mir_add_wrapped_executable(mir_demo_prerendered_frames
    mir_demo_prerendered_frames.c
)

target_link_libraries(mir_demo_prerendered_frames
    mirclient
)
