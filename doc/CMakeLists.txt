find_package(Doxygen 1.8.0)

if(DOXYGEN_FOUND)
  if(NOT ${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    file(
        COPY ${CMAKE_CURRENT_SOURCE_DIR}
        DESTINATION ${CMAKE_BINARY_DIR}
        PATTERN *.in EXCLUDE
    )
  endif()

  configure_file(
      ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in
      ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY IMMEDIATE
  )

  configure_file(
      ${CMAKE_CURRENT_SOURCE_DIR}/introducing_the_miral_api.md.in
      ${CMAKE_CURRENT_BINARY_DIR}/introducing_the_miral_api.md @ONLY IMMEDIATE
  )

  # use the starter pack Makefile
  add_custom_command(
      OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/.sphinx/venv
      COMMAND make -C ${CMAKE_CURRENT_BINARY_DIR} install
  )

  add_custom_target(doc
      COMMAND make -C ${CMAKE_CURRENT_BINARY_DIR} html
      DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/.sphinx/venv
  )
endif()
