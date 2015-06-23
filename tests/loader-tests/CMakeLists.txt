
mir_add_wrapped_executable(mir_test_reload_protobuf test_reload.c)

# XXX I would like to detect the full file name from the mirprotobuf target
#     too, but it seems only the "LOCATION" property works (and provides too
#     much info including full directory path).
get_target_property(mirprotobuf_soname mirprotobuf SOVERSION)
target_compile_definitions(mir_test_reload_protobuf PRIVATE
  -DDEFAULT_LIB_NAME="libmirprotobuf.so.${mirprotobuf_soname}"
)

# Only a build-time dependency. Don't link to it!
add_dependencies(mir_test_reload_protobuf mirprotobuf)

target_link_libraries(mir_test_reload_protobuf dl)

add_test(NAME Protobuf-can-be-reloaded   # LP: #1391976
  COMMAND ${CMAKE_BINARY_DIR}/bin/mir_test_reload_protobuf
)
