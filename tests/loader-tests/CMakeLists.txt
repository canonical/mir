
mir_add_wrapped_executable(mir_test_reload_protobuf test_reload.c)
target_link_libraries(mir_test_reload_protobuf dl)

# Only a build-time dependency. Don't link!
add_dependencies(mir_test_reload_protobuf mirprotobuf)

add_test(NAME Protobuf-can-be-reloaded   # LP: #1391976
  COMMAND /bin/sh -c "${CMAKE_BINARY_DIR}/bin/mir_test_reload_protobuf ${CMAKE_BINARY_DIR}/lib/libmirprotobuf.so.*"
)
