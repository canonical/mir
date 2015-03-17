configure_file(package.yaml.in package.yaml)
install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/package.yaml
  readme.md
  DESTINATION meta/
)

set(SNAPPY_IMAGE ${CMAKE_BINARY_DIR}/snappy-image)
add_custom_target(snap
  COMMAND make -j8 -C ${CMAKE_BINARY_DIR} DESTDIR=${SNAPPY_IMAGE} install/strip
  COMMAND snappy build ${SNAPPY_IMAGE}
  COMMAND ls -l *.snap
)

