ADD_CUSTOM_COMMAND(
    OUTPUT ${CMAKE_BINARY_DIR}/doc/html/cppguide/index.html
    COMMAND xsltproc -o ${CMAKE_BINARY_DIR}/doc/html/cppguide/index.html ${CMAKE_CURRENT_SOURCE_DIR}/styleguide.xsl ${CMAKE_CURRENT_SOURCE_DIR}/cppguide.xml
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/styleguide.xsl ${CMAKE_CURRENT_SOURCE_DIR}/cppguide.xml
    )

ADD_CUSTOM_COMMAND(
    OUTPUT ${CMAKE_BINARY_DIR}/doc/html/cppguide/styleguide.css
    COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/styleguide.css ${CMAKE_BINARY_DIR}/doc/html/cppguide/styleguide.css
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/styleguide.css
    )

ADD_CUSTOM_TARGET(guides
    DEPENDS ${CMAKE_BINARY_DIR}/doc/html/cppguide/index.html
            ${CMAKE_BINARY_DIR}/doc/html/cppguide/styleguide.css)
