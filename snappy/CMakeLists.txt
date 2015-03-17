configure_file(package.yaml.in package.yaml)
install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/package.yaml
  readme.md
  DESTINATION meta/
)

set(ARCH_LIB_DIR /usr/lib/${CMAKE_SYSTEM_PROCESSOR}-linux-gnu)
set(SNAPPY_IMAGE ${CMAKE_BINARY_DIR}/snappy-image)

add_custom_target(snap
  COMMAND rm -rf ${SNAPPY_IMAGE}
  COMMAND make -j8 -C ${CMAKE_BINARY_DIR} DESTDIR=${SNAPPY_IMAGE} install/strip
  COMMAND mv ${SNAPPY_IMAGE}/${CMAKE_INSTALL_PREFIX}/* ${SNAPPY_IMAGE}
  COMMAND rm -rf ${SNAPPY_IMAGE}/usr
  COMMAND rm -rf ${SNAPPY_IMAGE}/include
  COMMAND rm -rf ${SNAPPY_IMAGE}/lib/pkgconfig
  COMMAND cp ${ARCH_LIB_DIR}/libdrm_* ${SNAPPY_IMAGE}/lib/
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/copy-external-libs.sh ${SNAPPY_IMAGE}/lib/ ${SNAPPY_IMAGE}/bin/* ${SNAPPY_IMAGE}/lib/*
  COMMAND cp -Rvp ${ARCH_LIB_DIR}/dri ${SNAPPY_IMAGE}/
  COMMAND snappy build ${SNAPPY_IMAGE}
  COMMAND ls -l ${CMAKE_CURRENT_BINARY_DIR}/*.snap
)

