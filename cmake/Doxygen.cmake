# Check if doxygen is present and add 'make doc' target
find_package(Doxygen)

option(
  BUILD_DOXYGEN
  "Build Doxygen documentation as part of the default build"
  OFF
)

if(DOXYGEN_FOUND AND (DOXYGEN_VERSION VERSION_GREATER "1.8"))
  message(STATUS "doxygen ${DOXYGEN_VERSION} (>= 1.8.0) available - enabling make target doc")
  EXECUTE_PROCESS(COMMAND "date" "-u" OUTPUT_VARIABLE DATE_TODAY)
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
                    COMMAND ${DOXYGEN_EXECUTABLE} ${PROJECT_BINARY_DIR}/Doxyfile
                    SOURCES ${PROJECT_BINARY_DIR}/Doxyfile
                    DEPENDS guides)
  install(DIRECTORY ${CMAKE_BINARY_DIR}/doc/html DESTINATION ${CMAKE_INSTALL_PREFIX}/share/doc/mir-doc/ OPTIONAL)
endif()
