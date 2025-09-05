option(
  BUILD_DOXYGEN
  "Build Doxygen documentation as part of the default build"
  OFF
)

if(BUILD_DOXYGEN)
  set(REQUIRED "REQUIRED")
endif()

find_package(Doxygen 1.8.0 ${REQUIRED})

if(DOXYGEN_FOUND)
  cmake_path(RELATIVE_PATH CMAKE_SOURCE_DIR BASE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} OUTPUT_VARIABLE MIR_SOURCE_RELATIVE_PATH)
  if(NOT ${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    file(
        COPY ${CMAKE_CURRENT_SOURCE_DIR}/sphinx/
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/sphinx/
        PATTERN *.in EXCLUDE
    )
  endif()

  configure_file(
      ${CMAKE_CURRENT_SOURCE_DIR}/sphinx/Doxyfile.in
      ${CMAKE_CURRENT_BINARY_DIR}/sphinx/Doxyfile @ONLY IMMEDIATE
  )

  configure_file(
      ${CMAKE_CURRENT_SOURCE_DIR}/sphinx/reference/introducing_the_miral_api.md.in
      ${CMAKE_CURRENT_BINARY_DIR}/sphinx/reference/introducing_the_miral_api.md @ONLY IMMEDIATE
  )

  # use the starter pack Makefile
  add_custom_command(
      OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/sphinx/.sphinx/venv
      COMMAND make -C ${CMAKE_CURRENT_BINARY_DIR}/sphinx install
  )

  if(BUILD_DOXYGEN)
    set(ALL "ALL")
  endif()

  add_custom_target(doc ${ALL}
      COMMAND make -C ${CMAKE_CURRENT_BINARY_DIR}/sphinx html
      DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/sphinx/.sphinx/venv
      BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/sphinx/_build
                 ${CMAKE_CURRENT_BINARY_DIR}/sphinx/xml
  )

  install(
      DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/sphinx/_build/
      DESTINATION ${CMAKE_INSTALL_PREFIX}/share/doc/mir-doc
      OPTIONAL
  )
endif()
