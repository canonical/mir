add_custom_target(doc-setup
  COMMAND make -C ${CMAKE_SOURCE_DIR}/doc/sphinx install
  BYPRODUCTS ${CMAKE_SOURCE_DIR}/doc/sphinx/.sphinx/venv
)

# The difference between the `VAR=<value> make <target>` and `make <target>
# VAR=<value>` is the the former passes the variable down to child processes of
# make. In this case, sphinx. Which is then read back when `conf.py` is
# executed to determine where the build directory is to read doxygen's output.
# The later only overrides variables in the makefile.
add_custom_target(doc
  COMMAND ${CMAKE_COMMAND} -E env "MIR_CMAKE_BUILD_DIR=${CMAKE_BINARY_DIR}" 
          make -C ${CMAKE_SOURCE_DIR}/doc/sphinx html 
          BUILDDIR=${CMAKE_CURRENT_BINARY_DIR}/sphinx/_build
  DEPENDS ${CMAKE_SOURCE_DIR}/doc/sphinx/.sphinx/venv
  BYPRODUCTS ${CMAKE_SOURCE_DIR}/doc/sphinx/_build
)

add_custom_target(doc-with-api
  COMMAND ${CMAKE_COMMAND} -E env "MIR_BUILD_API_DOCS=1" 
          ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target doc
)

add_custom_target(doc-serve
  COMMAND ${CMAKE_COMMAND} -E env "MIR_CMAKE_BUILD_DIR=${CMAKE_BINARY_DIR}" 
          make -C ${CMAKE_SOURCE_DIR}/doc/sphinx serve
          BUILDDIR=${CMAKE_CURRENT_BINARY_DIR}/sphinx/_build
  USES_TERMINAL
)

add_custom_target(doc-clean
  COMMAND make -C ${CMAKE_SOURCE_DIR}/doc/sphinx clean
)

add_custom_target(doc-clean-all
  COMMAND rm -r ${CMAKE_SOURCE_DIR}/doc/sphinx/api
)
add_dependencies(doc-clean-all doc-clean)

install(
  DIRECTORY ${CMAKE_SOURCE_DIR}/doc/sphinx/_build/
  DESTINATION ${CMAKE_INSTALL_PREFIX}/share/doc/mir-doc
  OPTIONAL
)

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
  set(WORKING_DIR ${CMAKE_CURRENT_BINARY_DIR}/sphinx)

  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/sphinx/Doxyfile.in
    ${WORKING_DIR}/Doxyfile @ONLY IMMEDIATE
  )

  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/sphinx/reference/introducing_the_miral_api.md.in
    ${CMAKE_CURRENT_SOURCE_DIR}/sphinx/reference/introducing_the_miral_api.md @ONLY IMMEDIATE
  )


  add_custom_target(doxygen
    COMMAND doxygen ${WORKING_DIR}/Doxyfile
    BYPRODUCTS ${WORKING_DIR}/doxygen_output/xml 
  )

  add_custom_target(doxygen-clean
    COMMAND rm -r ${WORKING_DIR}/doxygen_output/xml
  ) 
  add_dependencies(doc-clean-all doxygen-clean)
endif()

