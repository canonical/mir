include_directories(
  ${PROJECT_SOURCE_DIR}/include/miral
  ${PROJECT_SOURCE_DIR}/src/include/server
  ${PROJECT_SOURCE_DIR}/include/platform
  ${PROJECT_SOURCE_DIR}/include/renderers/gl
  ${PROJECT_SOURCE_DIR}/include/renderers/sw
)

mir_add_wrapped_executable(mir_command_line_doc
    server_example.cpp
)

target_link_libraries(mir_command_line_doc
  example-shell-lib
  Boost::system
)
