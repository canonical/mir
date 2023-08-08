find_package(Doxygen 1.8.0)

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
      ${CMAKE_CURRENT_SOURCE_DIR}/sphinx/introducing_the_miral_api.md.in
      ${CMAKE_CURRENT_BINARY_DIR}/sphinx/introducing_the_miral_api.md @ONLY IMMEDIATE
  )

  # use the starter pack Makefile
  add_custom_command(
      OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/sphinx/.sphinx/venv
      COMMAND make -C ${CMAKE_CURRENT_BINARY_DIR}/sphinx install
  )

  add_custom_target(doc
      COMMAND make -C ${CMAKE_CURRENT_BINARY_DIR}/sphinx html
      DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/sphinx/.sphinx/venv
              guides
  )
endif()
