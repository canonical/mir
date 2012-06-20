PROJECT(guides)

ADD_CUSTOM_COMMAND(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/cppguide.html
    COMMAND xsltproc -o ${CMAKE_CURRENT_BINARY_DIR}/cppguide.html ${CMAKE_CURRENT_SOURCE_DIR}/styleguide.xsl ${CMAKE_CURRENT_SOURCE_DIR}/cppguide.xml
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/styleguide.xsl ${CMAKE_CURRENT_SOURCE_DIR}/cppguide.xml
    )

ADD_CUSTOM_COMMAND(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/styleguide.css
    COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/styleguide.css ${CMAKE_CURRENT_BINARY_DIR}/styleguide.css
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/styleguide.css
    )

ADD_CUSTOM_TARGET(guides ALL
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/cppguide.html
          ${CMAKE_CURRENT_BINARY_DIR}/styleguide.css)
