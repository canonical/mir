
add_executable(load_twice load_twice.c)
target_link_libraries(load_twice dl)

# Only a build-time dependency. Don't link!
add_dependencies(load_twice mirprotobuf)

add_test(NAME Protobuf-can-be-reloaded   # LP: #1391976
  COMMAND /bin/sh -c "${CMAKE_BINARY_DIR}/bin/load_twice ${CMAKE_BINARY_DIR}/lib/libmirprotobuf.so.*"
)
