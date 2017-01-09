add_library(acceptance-test-throwback OBJECT

  test_client_cursor_api.cpp
  test_client_library_errors.cpp
  test_shell_control_of_surface_configuration.cpp
  test_client_library.cpp
)

# uses BasicWindowManager<>
set_source_files_properties(test_client_cursor_api.cpp PROPERTIES COMPILE_FLAGS
        "${CMAKE_CXXFLAGS} -I ${PROJECT_SOURCE_DIR}/src/include/server")

# uses UsingClientPlatform
set_source_files_properties(test_client_library_errors.cpp PROPERTIES COMPILE_FLAGS
        "${CMAKE_CXXFLAGS} -I ${CMAKE_SOURCE_DIR} -I ${PROJECT_SOURCE_DIR}/src/include/client -I ${PROJECT_SOURCE_DIR}/tests/include")

# uses CanonicalWindowManager
set_source_files_properties(test_shell_control_of_surface_configuration.cpp PROPERTIES COMPILE_FLAGS
        "${CMAKE_CXXFLAGS} -I ${PROJECT_SOURCE_DIR}/src/include/server -I ${PROJECT_SOURCE_DIR}/src/include/client")

# uses src/include/common/mir/protobuf/protocol_version.h
set_source_files_properties(test_client_library.cpp PROPERTIES COMPILE_FLAGS "${CMAKE_CXXFLAGS} -I ${CMAKE_SOURCE_DIR}")
