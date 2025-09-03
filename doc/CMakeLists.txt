add_custom_target(doc-setup
    COMMAND make -C ${CMAKE_SOURCE_DIR}/doc/sphinx install
    BYPRODUCTS ${CMAKE_SOURCE_DIR}/doc/sphinx/.sphinx/venv
)

add_custom_target(doc ${ALL}
    # We pass `-i` (ignore errors) because if exhale and breathe are not run
    # (when doxygen is not run), some warnings pop up. Make treats warnings as
    # errors and stops. 
    COMMAND make -C ${CMAKE_SOURCE_DIR}/doc/sphinx html
    DEPENDS ${CMAKE_SOURCE_DIR}/doc/sphinx/.sphinx/venv
            guides
    BYPRODUCTS ${CMAKE_SOURCE_DIR}/doc/sphinx/_build
)

add_custom_target(doc-serve
    COMMAND make -C ${CMAKE_SOURCE_DIR}/doc/sphinx serve
)
add_dependencies(doc-serve doc)

add_custom_target(doc-clean
    COMMAND make -C ${CMAKE_SOURCE_DIR}/doc/sphinx clean
)

add_custom_target(doc-clean-all
    COMMAND rm -r ${CMAKE_SOURCE_DIR}/doc/sphinx/api
)
add_dependencies(doc-clean-all doc-clean)

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

  if(BUILD_DOXYGEN)
    set(ALL "ALL")
  endif()

  add_custom_target(doxygen ${ALL}
      COMMAND doxygen ${WORKING_DIR}/Doxyfile
      BYPRODUCTS ${WORKING_DIR}/doxygen_output/xml 
  )

  add_custom_target(doxygen-clean
      COMMAND rm -r ${WORKING_DIR}/doxygen_output/xml
  ) 
  add_dependencies(doc-clean-all doxygen-clean)

  install(
      DIRECTORY ${CMAKE_SOURCE_DIR}/doc/sphinx/_build/
      DESTINATION ${CMAKE_INSTALL_PREFIX}/share/doc/mir-doc
      OPTIONAL
  )
endif()

