# Check if doxygen is present and add 'make doc' target
find_package(Doxygen)

if(DOXYGEN_FOUND AND (DOXYGEN_VERSION VERSION_GREATER "1.8"))
  message(STATUS "doxygen ${DOXYGEN_VERSION} (>= 1.8.0) available - enabling make target doc")
  configure_file(doc/Doxyfile.in
                 ${PROJECT_BINARY_DIR}/Doxyfile @ONLY IMMEDIATE)
  add_custom_target(doc
                    COMMAND ${DOXYGEN_EXECUTABLE} ${PROJECT_BINARY_DIR}/Doxyfile
                    SOURCES ${PROJECT_BINARY_DIR}/Doxyfile)
endif()
