# The generator runs during the build, so it is built for the build machine:
# unlike add_rust_cxx_library, never pass DEB_HOST_RUST_TYPE as --target.
set(generator_target_dir "${CMAKE_BINARY_DIR}/target")
if("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
  set(generator_executable "${generator_target_dir}/release/mir_wayland_generator")
  set(generator_release_flag "--release")
else()
  set(generator_executable "${generator_target_dir}/debug/mir_wayland_generator")
  set(generator_release_flag "")
endif()

add_custom_command(
  OUTPUT ${generator_executable}
  COMMAND ${CARGO_EXECUTABLE} build ${generator_release_flag} --target-dir ${generator_target_dir} -p mir_wayland_generator
  DEPENDS
    Cargo.toml
    ${wayland_rs_cargo_lock}
    src/emit.rs
    src/main.rs
    src/model.rs
    ../build_script/protocol_parser.rs
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  JOB_SERVER_AWARE TRUE
  COMMENT "Building mir_wayland_generator")

# mir_generate_protocol_wrapper() runs the executable at this target's
# MIR_WAYLAND_GENERATOR_EXECUTABLE property.
add_custom_target(mir_wayland_generator DEPENDS ${generator_executable})
set_target_properties(mir_wayland_generator PROPERTIES MIR_WAYLAND_GENERATOR_EXECUTABLE ${generator_executable})
