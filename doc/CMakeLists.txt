set(VENVDIR ${CMAKE_CURRENT_BINARY_DIR}/sphinx/venv)
set(BUILDDIR ${CMAKE_CURRENT_BINARY_DIR}/sphinx/_build)
set(MAKEOPTS
  VENVDIR=${VENVDIR}
  BUILDDIR=${BUILDDIR}
  DOCTREESDIR=${CMAKE_CURRENT_BINARY_DIR}/sphinx/.doctrees
)
set(SPHINXDIR ${CMAKE_CURRENT_SOURCE_DIR}/sphinx)

option(
  BUILD_DOXYGEN
  "Build Doxygen documentation as part of the default build"
  OFF
)

# Modifications to `MAKEOPTS` have to be made before targets are generated
# below.
if(BUILD_DOXYGEN)
  find_package(Doxygen 1.8.0 "REQUIRED")
  set(MAKEOPTS
    ${MAKEOPTS}
    MIR_BUILD_API_DOCS=1
  )
endif()

add_custom_command(
  COMMAND make -C ${SPHINXDIR} install
          VENVDIR=${VENVDIR}
  OUTPUT ${VENVDIR}
)

set(SPHINX_TARGETS
  clean
  clean-doc
  html
  linkcheck
  lint-md
  pa11y
  run
  serve
  spelling
  vale
  woke
)

foreach(target IN LISTS SPHINX_TARGETS)
  add_custom_target(doc-${target}
    # In `VAR1=<value> make <target> VAR2=<value>, only `VAR1` is inherited by
    # `make`'s children, `VAR2` only applies within `make`.
    COMMAND ${CMAKE_COMMAND} -E env "MIR_BUILD_DIR=${CMAKE_BINARY_DIR}"
            make -C ${SPHINXDIR} ${target} ${MAKEOPTS}
    DEPENDS ${VENVDIR}
    USES_TERMINAL
  )
endforeach()

add_custom_target(doc DEPENDS doc-html)
add_custom_target(
  doc-checks
  DEPENDS doc-linkcheck
          doc-lint-md
          doc-spelling
          doc-vale
          doc-woke
)

if(BUILD_DOXYGEN)
  set(WORKING_DIR ${CMAKE_CURRENT_BINARY_DIR}/sphinx)
  set(DOXYGEN_OUTPUT_DIR ${WORKING_DIR}/doxygen_output)

  configure_file(
    ${SPHINXDIR}/Doxyfile.in
    ${WORKING_DIR}/Doxyfile @ONLY IMMEDIATE
  )

  add_custom_target(doxygen
    COMMAND doxygen ${WORKING_DIR}/Doxyfile
    BYPRODUCT ${DOXYGEN_OUTPUT_DIR}
  )

  add_custom_target(doxygen-clean
    COMMAND ${CMAKE_COMMAND} -E rm -rf ${DOXYGEN_OUTPUT_DIR}
    # Deletes all the files inside the `api` directory except `library_root.rst`
    VERBATIM
    COMMAND /bin/bash -c  "rm -f $(ls -1d ${SPHINXDIR}/api/* | sed  '/.*library_root.rst/d')"
  )

  add_dependencies(doc-html doxygen)
  add_dependencies(doc-run doxygen)
  add_dependencies(doc-clean doxygen-clean)
endif()

install(
  DIRECTORY ${BUILDDIR}
  DESTINATION ${CMAKE_INSTALL_PREFIX}/share/doc/mir-doc
  OPTIONAL
)

