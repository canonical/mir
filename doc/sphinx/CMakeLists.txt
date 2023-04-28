find_program(SPHINX sphinx-build)
find_package(Doxygen 1.8.0)

if(DOXYGEN_FOUND)
if(SPHINX)
  execute_process(COMMAND "date" "+%Y" OUTPUT_VARIABLE YEAR_TODAY OUTPUT_STRIP_TRAILING_WHITESPACE)
  execute_process(COMMAND "pip" "install" "sphinx_rtd_theme" "breathe" "graphviz" "furo" "exhale")

  configure_file(
      ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in
      ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY IMMEDIATE)

  configure_file(
      ${CMAKE_CURRENT_SOURCE_DIR}/conf.py.in
      ${CMAKE_CURRENT_BINARY_DIR}/conf.py @ONLY IMMEDIATE)

  configure_file(
      ${CMAKE_CURRENT_SOURCE_DIR}/index.rst
      ${CMAKE_CURRENT_BINARY_DIR}/index.rst @ONLY IMMEDIATE)

  add_custom_target(doc
          COMMAND sphinx-build -b html  ${CMAKE_CURRENT_BINARY_DIR}  ${CMAKE_BINARY_DIR}/doc/html
          #COMMAND xdg-open ${CMAKE_BINARY_DIR}/doc/html/index.html
        )
endif()
endif()
