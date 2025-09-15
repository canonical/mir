set(BUILDDIR ${CMAKE_CURRENT_BINARY_DIR}/sphinx/_build)
set(MAKEOPTS
  BUILDDIR=${BUILDDIR}
  VENVDIR=${CMAKE_CURRENT_BINARY_DIR}/sphinx/venv
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
  find_package(Doxygen 1.9.8 REQUIRED COMPONENTS dot)
  set(MAKEOPTS
    ${MAKEOPTS}
    MIR_BUILD_API_DOCS=1
    # FIXME: Sphinx gets itself into a bind reading cached API state
    EXTRA_SPHINXOPTS=-E
  )
endif()

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
  set(DOXYGEN_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/doxygen_output)

  add_custom_target(doxygen
    COMMAND
      ${CMAKE_COMMAND} -E env
      DOXYGEN_OUTPUT_DIR=${DOXYGEN_OUTPUT_DIR}
      doxygen
      ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    BYPRODUCTS ${DOXYGEN_OUTPUT_DIR}
  )

  add_custom_target(doxygen-clean
    COMMAND ${CMAKE_COMMAND} -E rm -rf ${DOXYGEN_OUTPUT_DIR}
    COMMAND git clean -xf ${SPHINXDIR}/api/*
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
