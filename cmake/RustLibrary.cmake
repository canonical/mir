# Select syn 2 with prettyplease 0.2, or syn 3 with prettyplease 0.3, from the
# cargo registry this build is configured to use. Cargo locks every optional
# dependency, so Cargo.toml names one pair and this rewrites it. Invoked once
# at build time: cmake -DMIR_CARGO_ROOT=<source> -P cmake/RustLibrary.cmake
if(CMAKE_SCRIPT_MODE_FILE)
  if(NOT MIR_CARGO_ROOT)
    message(FATAL_ERROR "wayland_rs: MIR_CARGO_ROOT is not set")
  endif()
  set(registry "$ENV{CARGO_REGISTRY}")
  if(registry STREQUAL "")
    foreach(candidate
        "$ENV{CARGO_HOME}/config.toml"
        "$ENV{CARGO_HOME}/config"
        "${MIR_CARGO_ROOT}/.cargo/config.toml"
        "${MIR_CARGO_ROOT}/.cargo/config")
      if(NOT EXISTS "${candidate}")
        continue()
      endif()
      file(READ "${candidate}" cargo_cfg)
      if(cargo_cfg MATCHES "(^|\n)[ \t]*directory[ \t]*=[ \t]*\"([^\"]*)\"")
        set(registry "${CMAKE_MATCH_2}")
      endif()
      break()
    endforeach()
  endif()

  set(pair "syn2")
  if(NOT registry STREQUAL "" AND IS_DIRECTORY "${registry}")
    file(GLOB pp3 LIST_DIRECTORIES false "${registry}/prettyplease-0.3*/Cargo.toml")
    file(GLOB pp2 LIST_DIRECTORIES false "${registry}/prettyplease-0.2*/Cargo.toml")
    file(GLOB s3 LIST_DIRECTORIES false "${registry}/syn-3*/Cargo.toml")
    file(GLOB s2 LIST_DIRECTORIES false "${registry}/syn-2*/Cargo.toml")
    if(pp3 AND s3)
      file(READ "${MIR_CARGO_ROOT}/Cargo.toml" manifest)
      string(REGEX REPLACE
        "(^|\n)prettyplease = \"0\\.2\""
        "\\1prettyplease = \"0.3\""
        updated "${manifest}")
      string(REGEX REPLACE
        "(^|\n)syn = \\{ version = \"2\""
        "\\1syn = { version = \"3\""
        updated "${updated}")
      if(NOT updated STREQUAL manifest)
        file(WRITE "${MIR_CARGO_ROOT}/Cargo.toml" "${updated}")
      endif()
      set(pair "syn3")
    elseif(pp2 AND s2)
      set(pair "syn2")
    elseif(pp3 OR pp2 OR s3 OR s2)
      message(FATAL_ERROR
        "wayland_rs: ${registry} has no matching syn and prettyplease.\n"
        "prettyplease 0.3 needs syn 3. prettyplease 0.2 needs syn 2.")
    endif()
  endif()
  message(NOTICE "wayland_rs: Cargo pair ${pair}")
  return()
endif()

set(_mir_rust_library_cmake "${CMAKE_CURRENT_LIST_DIR}/RustLibrary.cmake")

find_program(CARGO_EXECUTABLE cargo REQUIRED)

function(add_rust_cxx_library target)
  set(one_value_args CRATE CXX_BRIDGE_SOURCE_FILE)
  set(multi_value_args DEPENDS LIBRARIES INCLUDES)
  cmake_parse_arguments(arg "" "${one_value_args}" "${multi_value_args}" ${ARGN})

  if("${arg_CRATE}" STREQUAL "")
    message(FATAL_ERROR "add_rust_cxx_library called without CRATE <value>")
  endif()

  if("${arg_CXX_BRIDGE_SOURCE_FILE}" STREQUAL "")
    set(arg_CXX_BRIDGE_SOURCE_FILE "src/lib.rs")
  endif()

  set(rust_target_dir "${CMAKE_BINARY_DIR}/target")
  if("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    set(rust_binary_dir "${rust_target_dir}/$ENV{DEB_HOST_RUST_TYPE}/release")
    set(cargo_release_flag "--release")
  else()
    set(rust_binary_dir "${rust_target_dir}/$ENV{DEB_HOST_RUST_TYPE}/debug")
    set(cargo_release_flag "")
  endif()

  set(cxxbridge_include_dir "${rust_target_dir}/$ENV{DEB_HOST_RUST_TYPE}/cxxbridge")
  set(cxxbridge_header "${cxxbridge_include_dir}/${arg_CRATE}/${arg_CXX_BRIDGE_SOURCE_FILE}.h")
  set(cxxbridge_source "${cxxbridge_include_dir}/${arg_CRATE}/${arg_CXX_BRIDGE_SOURCE_FILE}.cc")
  set(crate_staticlib "${rust_binary_dir}/lib${arg_CRATE}.a")

  # DEB_HOST_RUST_TYPE is set in deb builds, which expect the build under
  # the architecture triplet.
  set(cargo_target_flag "")
  if(NOT "$ENV{DEB_HOST_RUST_TYPE}" STREQUAL "")
    set(cargo_target_flag "--target" "$ENV{DEB_HOST_RUST_TYPE}")
  endif()

  # Retarget syn/prettyplease once, before any Cargo invocation. The command
  # reads the distro registry cargo is configured to use. add_dependencies
  # orders this ahead of every Rust build, including across directories.
  if(NOT TARGET mir_prepare_cargo_deps)
    set(_mir_syn_stamp "${CMAKE_BINARY_DIR}/mir-prepare-cargo-deps.stamp")
    add_custom_command(
      OUTPUT "${_mir_syn_stamp}"
      COMMAND "${CMAKE_COMMAND}"
              "-DMIR_CARGO_ROOT=${PROJECT_SOURCE_DIR}"
              -P "${_mir_rust_library_cmake}"
      COMMAND "${CMAKE_COMMAND}" -E touch "${_mir_syn_stamp}"
      DEPENDS
        "${_mir_rust_library_cmake}"
        "${PROJECT_SOURCE_DIR}/Cargo.toml"
      WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
      COMMENT "Selecting the syn and prettyplease pair"
      VERBATIM)
    add_custom_target(mir_prepare_cargo_deps DEPENDS "${_mir_syn_stamp}")
  endif()

  # Forward the pkg-config CMake resolved to cargo, so build scripts using the
  # pkg-config crate find the same libraries CMake does. When cross-compiling,
  # that crate refuses to run unless PKG_CONFIG_ALLOW_CROSS is set, so set it
  # too.
  set(cargo_env)
  if(PKG_CONFIG_EXECUTABLE)
    list(APPEND cargo_env "PKG_CONFIG=${PKG_CONFIG_EXECUTABLE}")
  endif()
  if(CMAKE_CROSSCOMPILING)
    list(APPEND cargo_env "PKG_CONFIG_ALLOW_CROSS=1")
  endif()

  # JOB_SERVER_AWARE lets cargo (a job server client) take tokens from the
  # Makefile generators' job server, so `-j N` limits the whole build rather
  # than just the C++ part. It is ignored by generators without a job server.
  # For those, set CARGO_BUILD_JOBS to limit cargo's parallelism
  add_custom_command(
    OUTPUT ${cxxbridge_header} ${cxxbridge_source} ${crate_staticlib}
    COMMAND ${CMAKE_COMMAND} -E env ${cargo_env}
            ${CARGO_EXECUTABLE} build ${cargo_release_flag} ${cargo_target_flag} --target-dir ${rust_target_dir} -p ${arg_CRATE}
    DEPENDS ${arg_DEPENDS}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    JOB_SERVER_AWARE TRUE
    COMMENT "Building Rust crate ${arg_CRATE}")

  # We build the Rust target first as it could potentially output C++ code
  # during its build script that other targets depend on.
  add_custom_target(${target}-rust-build
    DEPENDS ${cxxbridge_header} ${cxxbridge_source} ${crate_staticlib})
  add_dependencies(${target}-rust-build mir_prepare_cargo_deps)

  add_library(${target}-rust STATIC IMPORTED)
  set_target_properties(${target}-rust PROPERTIES
    IMPORTED_LOCATION ${crate_staticlib})

  add_library(${target}-cxxbridge STATIC
    ${cxxbridge_source} ${cxxbridge_header})
  add_dependencies(${target}-cxxbridge ${target}-rust-build)
  target_include_directories(${target}-cxxbridge PRIVATE ${cxxbridge_include_dir})

  if(arg_INCLUDES)
    foreach(inc_dir IN LISTS arg_INCLUDES)
      target_include_directories(${target}-cxxbridge PRIVATE "${inc_dir}")
    endforeach()
  endif()

  if(arg_LIBRARIES)
    foreach(library ${arg_LIBRARIES})
      target_include_directories(${target}-cxxbridge PRIVATE
              "$<TARGET_PROPERTY:${library},INTERFACE_INCLUDE_DIRECTORIES>"
      )
    endforeach ()
  endif()

  add_library(${target} INTERFACE)
  target_include_directories(${target} INTERFACE ${cxxbridge_include_dir})
  if(arg_INCLUDES)
    foreach(inc_dir IN LISTS arg_INCLUDES)
      target_include_directories(${target} INTERFACE "${inc_dir}")
    endforeach()
  endif()

  # As described in https://cxx.rs/build/other.html#linking-the-c-and-rust-together
  # the Rust staticlib and cxxbridge generated code are
  # interdependent.  CMake's LINKGROUP:RESCAN generator will produce
  # the required --start-group/--end-group link flags.
  set(LIBRARIES)
  foreach(library ${arg_LIBRARIES})
    if (TARGET ${library})
      get_target_property(libs ${library} INTERFACE_LINK_LIBRARIES)
      if (libs)
        list(APPEND LIBRARIES ${libs})
      endif()
    else()
      list(APPEND LIBRARIES ${library})
    endif()
  endforeach ()
  target_link_libraries(${target} INTERFACE $<LINK_GROUP:RESCAN,${target}-cxxbridge,${target}-rust,${LIBRARIES}>)
endfunction()
