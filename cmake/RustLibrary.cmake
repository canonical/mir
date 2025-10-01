find_program(CARGO_EXECUTABLE cargo)

function(add_rust_cxx_library target)
  set(one_value_args CRATE CXX_BRIDGE_SOURCE_FILE)
  set(multi_value_args DEPENDS)
  set(multi_value_args LIBRARIES)
  cmake_parse_arguments(arg "" "${one_value_args}" "${multi_value_args}" ${ARGN})

  if("${arg_CRATE}" STREQUAL "")
    message(FATAL_ERROR "add_rust_cxx_library called without CRATE <value>")
  endif()

  if("${arg_CXX_BRIDGE_SOURCE_FILE}" STREQUAL "")
    set(arg_CXX_BRIDGE_SOURCE_FILE "src/lib.rs")
  endif()

  set(rust_target_dir "${CMAKE_BINARY_DIR}/target")
  if("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    set(rust_binary_dir "${rust_target_dir}/release")
    set(cargo_release_flag "--release")
  else()
    set(rust_binary_dir "${rust_target_dir}/debug")
    set(cargo_release_flag "")
  endif()

  set(cxxbridge_include_dir "${rust_target_dir}/cxxbridge")
  set(cxxbridge_header "${cxxbridge_include_dir}/${arg_CRATE}/${arg_CXX_BRIDGE_SOURCE_FILE}.h")
  set(cxxbridge_source "${cxxbridge_include_dir}/${arg_CRATE}/${arg_CXX_BRIDGE_SOURCE_FILE}.cc")
  set(crate_staticlib "${rust_binary_dir}/lib${arg_CRATE}.a")

  add_custom_command(
    OUTPUT ${cxxbridge_header} ${cxxbridge_source} ${crate_staticlib}
    COMMAND ${CARGO_EXECUTABLE} build ${cargo_release_flag} --target-dir ${rust_target_dir} -p ${arg_CRATE}
    DEPENDS ${arg_DEPENDS}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})

  add_library(${target}-rust STATIC IMPORTED)
  set_target_properties(${target}-rust PROPERTIES
    IMPORTED_LOCATION ${crate_staticlib})

  add_library(${target}-cxxbridge
    ${cxxbridge_source} ${cxxbridge_header})
  # rust-cxx generates symbols named like "cxxbridge1$foo", which
  # triggers a warning in Clang.
  check_cxx_compiler_flag(-Wdollar-in-identifier-extension SUPPORTS_DOLLAR_IN_ID_WARNING)
  if(SUPPORTS_DOLLAR_IN_ID_WARNING)
    target_compile_options(${target}-cxxbridge
      PRIVATE -Wno-error=dollar-in-identifier-extension)
  endif()

  add_library(${target} INTERFACE)
  target_include_directories(${target} INTERFACE ${cxxbridge_include_dir})

  # As described in https://cxx.rs/build/other.html#linking-the-c-and-rust-together
  # the Rust staticlib and cxxbridge generated code are
  # interdependent.  CMake's LINKGROUP:RESCAN generator will produce
  # the required --start-group/--end-group link flags.
  set(LIBRARIES)
  foreach(library ${arg_LIBRARIES})
    message(${library})
    get_target_property(libs ${library} INTERFACE_LINK_LIBRARIES)
    list(APPEND LIBRARIES ${libs})
  endforeach ()
  target_link_libraries(${target} INTERFACE $<LINK_GROUP:RESCAN,${target}-cxxbridge,${target}-rust,${LIBRARIES}>)
endfunction()
