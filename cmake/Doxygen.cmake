# Check if doxygen is present and add 'make doc' target
find_package(Doxygen 1.8.0)

option(
  BUILD_DOXYGEN
  "Build Doxygen documentation as part of the default build"
  OFF
)

if(DOXYGEN_FOUND)
  message(STATUS "doxygen ${DOXYGEN_VERSION} (>= 1.8.0) available - enabling make target doc")
  EXECUTE_PROCESS(COMMAND "date" "-u" OUTPUT_VARIABLE DATE_TODAY)
  EXECUTE_PROCESS(COMMAND "date" "+%Y" OUTPUT_VARIABLE YEAR_TODAY)
  set(MIR_GENERATED_INCLUDE_DIRECTORIES_FLATTENED)
  foreach(GENERATED_DIR ${MIR_GENERATED_INCLUDE_DIRECTORIES})
    set(MIR_GENERATED_INCLUDE_DIRECTORIES_FLATTENED
        "${MIR_GENERATED_INCLUDE_DIRECTORIES_FLATTENED} ${GENERATED_DIR}")
  endforeach(GENERATED_DIR)

  configure_file(doc/Doxyfile.in
                 ${PROJECT_BINARY_DIR}/Doxyfile @ONLY IMMEDIATE)
  configure_file(doc/footer.html.in
                 ${PROJECT_BINARY_DIR}/doc/footer.html @ONLY IMMEDIATE)
  configure_file(doc/extra.css
                 ${PROJECT_BINARY_DIR}/doc/extra.css @ONLY IMMEDIATE)
  if (BUILD_DOXYGEN)
    set(ALL "ALL")
  endif()
  add_custom_target(doc ${ALL}
                    COMMAND find ${CMAKE_BINARY_DIR}/doc/html/ -mindepth 1 -maxdepth 1 -not -name cppguide -exec rm -rf {} +
                    COMMAND rm -rf ${CMAKE_BINARY_DIR}/doc/xml
                    COMMAND ${DOXYGEN_EXECUTABLE} ${PROJECT_BINARY_DIR}/Doxyfile
                    SOURCES ${PROJECT_BINARY_DIR}/Doxyfile
                    DEPENDS guides)
  install(DIRECTORY ${CMAKE_BINARY_DIR}/doc/html DESTINATION ${CMAKE_INSTALL_PREFIX}/share/doc/mir-doc/ OPTIONAL)

  find_program(SPHINX sphinx-build)
  if(SPHINX)
    # TODO: This setup shouldn't be repeated
    EXECUTE_PROCESS(COMMAND "pip" "install" "sphinx_rtd_theme" "breathe" "graphviz")

    add_custom_target(sphinx
            COMMAND mkdir -p ${CMAKE_BINARY_DIR}/doc/sphinx
            COMMAND sphinx-build -Dbreathe_projects.Mir=${CMAKE_BINARY_DIR}/doc/xml ${PROJECT_SOURCE_DIR}/doc/sphinx  ${CMAKE_BINARY_DIR}/doc/sphinx
            COMMAND xdg-open ${CMAKE_BINARY_DIR}/doc/sphinx/index.html
    )
  endif()
endif()
