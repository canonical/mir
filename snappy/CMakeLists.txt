set(MIR_PKG_ARCH ${CMAKE_SYSTEM_PROCESSOR})
if (${MIR_PKG_ARCH} STREQUAL x86_64)
  set(MIR_PKG_ARCH amd64)
endif()

configure_file(package.yaml.in package.yaml)
install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/package.yaml
  readme.md
  DESTINATION meta/
)

set(ARCH_LIB_DIR /usr/lib/${CMAKE_LIBRARY_ARCHITECTURE})
set(SNAPPY_IMAGE ${CMAKE_BINARY_DIR}/snappy-image)

add_custom_target(snap
  COMMAND rm -rf ${SNAPPY_IMAGE}
  COMMAND make -j8 -C ${CMAKE_BINARY_DIR} DESTDIR=${SNAPPY_IMAGE} install/strip
  COMMAND mv ${SNAPPY_IMAGE}/${CMAKE_INSTALL_PREFIX}/* ${SNAPPY_IMAGE}
  COMMAND rm -rf ${SNAPPY_IMAGE}/usr
  COMMAND rm -rf ${SNAPPY_IMAGE}/include
  COMMAND rm -rf ${SNAPPY_IMAGE}/lib/pkgconfig
  COMMAND cp -vdp ${ARCH_LIB_DIR}/libdrm_*.so* ${SNAPPY_IMAGE}/lib/
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/copy-external-libs.sh ${SNAPPY_IMAGE}/lib/ ${SNAPPY_IMAGE}/bin/* ${SNAPPY_IMAGE}/lib/*
  COMMAND cp -Rdvp ${ARCH_LIB_DIR}/dri ${SNAPPY_IMAGE}/
  COMMAND snappy build ${SNAPPY_IMAGE}
  COMMAND ls -l ${CMAKE_CURRENT_BINARY_DIR}/*.snap
)

