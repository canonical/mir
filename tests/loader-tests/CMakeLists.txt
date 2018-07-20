
#
# Note: In order for this test to work (crash in the absence of a fix), it
# must be a pure C program. This is because putting it in a non-trivial C++
# program (like mir_*_tests) somehow provides sufficient preloaded C++ symbols
# that the bug does not occur.
#
mir_add_wrapped_executable(mir_test_reload_protobuf NOINSTALL test_reload.c)

#
# XXX I would like to detect the full file name from the mirprotobuf target
#     too, but it seems only the "LOCATION" property works (and provides too
#     much info including full directory path).
#
get_target_property(mirprotobuf_soname mirprotobuf SOVERSION)

#
# Being a wrapped executable means we already have the correct LD_LIBRARY_PATH
# so can just load the library by file name (which also allows for
# directory portability).
#
target_compile_definitions(mir_test_reload_protobuf PRIVATE
  -DDEFAULT_LIB_NAME="libmirprotobuf.so.${mirprotobuf_soname}"
)

# Only a build-time dependency. Don't link to it!
add_dependencies(mir_test_reload_protobuf mirprotobuf)

target_link_libraries(mir_test_reload_protobuf dl)

add_test(NAME Protobuf-can-be-reloaded   # LP: #1391976
  COMMAND ${CMAKE_BINARY_DIR}/bin/mir_test_reload_protobuf
)
