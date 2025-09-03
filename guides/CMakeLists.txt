ADD_CUSTOM_COMMAND(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/index.html
    # The output directories are created implicitly by CMake backend machinery
    # and there seem to be race conditions where the command is sometimes run
    # before its output directory exists. As a quick hack always create
    # the subdir before running the command.
    COMMAND xsltproc -o ${CMAKE_CURRENT_SOURCE_DIR}/index.html ${CMAKE_CURRENT_SOURCE_DIR}/styleguide.xsl ${CMAKE_CURRENT_SOURCE_DIR}/cppguide.xml
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/styleguide.xsl ${CMAKE_CURRENT_SOURCE_DIR}/cppguide.xml
    )

ADD_CUSTOM_TARGET(guides
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/index.html
            ${CMAKE_CURRENT_SOURCE_DIR}/styleguide.css)
