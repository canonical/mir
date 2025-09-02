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
  # TODO: Do we even need Doxyfile.in? 
  # TODO: It's probably not a best practice to write files outside the build
  # dir. But, the other alternative would be jumping between to documentation
  # files outside and inside the build dir.
  configure_file(
      ${CMAKE_SOURCE_DIR}/doc/sphinx/Doxyfile.in
      ${CMAKE_SOURCE_DIR}/doc/sphinx/Doxyfile @ONLY IMMEDIATE
  )

  configure_file(
      ${CMAKE_SOURCE_DIR}/doc/sphinx/reference/introducing_the_miral_api.md.in
      ${CMAKE_SOURCE_DIR}/doc/sphinx/reference/introducing_the_miral_api.md @ONLY IMMEDIATE
  )

  if(BUILD_DOXYGEN)
    set(ALL "ALL")
  endif()

  add_custom_target(doxygen ${ALL}
      COMMAND doxygen ${CMAKE_SOURCE_DIR}/doc/sphinx/Doxyfile
      DEPENDS guides
      BYPRODUCTS ${CMAKE_SOURCE_DIR}/doc/sphinx/xml 
                 ${CMAKE_SOURCE_DIR}/doc/sphinx/api
  )

  install(
      DIRECTORY ${CMAKE_SOURCE_DIR}/doc/sphinx/_build/
      DESTINATION ${CMAKE_INSTALL_PREFIX}/share/doc/mir-doc
      OPTIONAL
  )
endif()

add_custom_target(doc ${ALL}
    COMMAND make -C ${CMAKE_SOURCE_DIR}/sphinx html
    DEPENDS ${CMAKE_SOURCE_DIR}/sphinx/.sphinx/venv
            guides
    BYPRODUCTS ${CMAKE_SOURCE_DIR}/sphinx/_build
)

add_custom_target(doc-serve ${ALL}
    COMMAND make -C ${CMAKE_SOURCE_DIR}/sphinx run
    DEPENDS ${CMAKE_SOURCE_DIR}/sphinx/.sphinx/venv
            guides
    BYPRODUCTS ${CMAKE_SOURCE_DIR}/sphinx/_build
)
