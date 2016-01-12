ADD_CUSTOM_COMMAND(
    OUTPUT ${CMAKE_BINARY_DIR}/doc/html/cppguide/index.html
    # The output directories are created implicitly by CMake backend machinery
    # and there seem to be race conditions where the command is sometimes run
    # before its output directory exists. As a quick hack always create
    # the subdir before running the command.
    COMMAND mkdir -p ${CMAKE_BINARY_DIR}/doc/html/cppguide/
    COMMAND xsltproc -o ${CMAKE_BINARY_DIR}/doc/html/cppguide/index.html ${CMAKE_CURRENT_SOURCE_DIR}/styleguide.xsl ${CMAKE_CURRENT_SOURCE_DIR}/cppguide.xml
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/styleguide.xsl ${CMAKE_CURRENT_SOURCE_DIR}/cppguide.xml
    )

ADD_CUSTOM_COMMAND(
    OUTPUT ${CMAKE_BINARY_DIR}/doc/html/cppguide/styleguide.css
    # See discussion above.
    COMMAND mkdir -p ${CMAKE_BINARY_DIR}/doc/html/cppguide
    COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/styleguide.css ${CMAKE_CURRENT_SOURCE_DIR}/favicon.ico ${CMAKE_BINARY_DIR}/doc/html/cppguide/
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/styleguide.css
    )

ADD_CUSTOM_TARGET(guides
    DEPENDS ${CMAKE_BINARY_DIR}/doc/html/cppguide/index.html
            ${CMAKE_BINARY_DIR}/doc/html/cppguide/styleguide.css)
