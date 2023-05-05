find_package(Doxygen 1.8.0)

if(DOXYGEN_FOUND)
  execute_process(COMMAND "date" "+%Y" OUTPUT_VARIABLE YEAR_TODAY OUTPUT_STRIP_TRAILING_WHITESPACE)

  configure_file(
      ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in
      ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY IMMEDIATE)

  configure_file(
      ${CMAKE_CURRENT_SOURCE_DIR}/conf.py.in
      ${CMAKE_CURRENT_BINARY_DIR}/conf.py @ONLY IMMEDIATE)

  configure_file(
      ${CMAKE_CURRENT_SOURCE_DIR}/index.rst
      ${CMAKE_CURRENT_BINARY_DIR}/index.rst @ONLY IMMEDIATE)

  configure_file(
      ${CMAKE_CURRENT_SOURCE_DIR}/getting_and_using_mir.rst
      ${CMAKE_CURRENT_BINARY_DIR}/getting_and_using_mir.rst @ONLY IMMEDIATE)

  configure_file(
      ${CMAKE_CURRENT_SOURCE_DIR}/getting_involved_in_mir.rst
      ${CMAKE_CURRENT_BINARY_DIR}/getting_involved_in_mir.rst @ONLY IMMEDIATE)

  configure_file(
      ${CMAKE_CURRENT_SOURCE_DIR}/component_reports.rst
      ${CMAKE_CURRENT_BINARY_DIR}/component_reports.rst @ONLY IMMEDIATE)

  configure_file(
      ${CMAKE_CURRENT_SOURCE_DIR}/dso_versioning_guide.rst
      ${CMAKE_CURRENT_BINARY_DIR}/dso_versioning_guide.rst @ONLY IMMEDIATE)

  configure_file(
      ${CMAKE_CURRENT_SOURCE_DIR}/_static/custom.css
      ${CMAKE_CURRENT_BINARY_DIR}/_static/custom.css @ONLY IMMEDIATE)

  set(VENVDIR ${CMAKE_CURRENT_BINARY_DIR}/.venv)
  set(VENV ${VENVDIR}/bin/activate)

  execute_process(
      COMMAND "python3" "-m" "venv" "${VENVDIR}"
      COMMAND sh -c ". ${VENV}; for x in sphinx sphinx_rtd_theme breathe graphviz furo exhale; do pip install $x; done"
  )

  add_custom_target(doc
      COMMAND sh -xc '. ${VENV}\; sphinx-build -b html  ${CMAKE_CURRENT_BINARY_DIR}  ${CMAKE_BINARY_DIR}/doc/html'
      #COMMAND xdg-open ${CMAKE_BINARY_DIR}/doc/html/index.html
  )
endif()
