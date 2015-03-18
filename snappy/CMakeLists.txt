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
  # Create a clean stripped install tree
  COMMAND rm -rf ${SNAPPY_IMAGE}
  COMMAND make -j8 -C ${CMAKE_BINARY_DIR} DESTDIR=${SNAPPY_IMAGE} install/strip
  COMMAND mv ${SNAPPY_IMAGE}/${CMAKE_INSTALL_PREFIX}/* ${SNAPPY_IMAGE}

  COMMAND cp -p ${CMAKE_CURRENT_SOURCE_DIR}/run ${SNAPPY_IMAGE}/bin/

  # Remove bits of Mir we don't need
  COMMAND rm -rf ${SNAPPY_IMAGE}/usr
  COMMAND rm -rf ${SNAPPY_IMAGE}/include
  COMMAND rm -rf ${SNAPPY_IMAGE}/share
  COMMAND rm -rf ${SNAPPY_IMAGE}/lib/pkgconfig

  # Copy in the DRM libraries needed by Mesa
  COMMAND cp -vdp ${ARCH_LIB_DIR}/libdrm_*.so* ${SNAPPY_IMAGE}/lib/

  # Search all the binaries and copy more libraries they need
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/copy-external-libs.sh ${SNAPPY_IMAGE}/lib/ ${SNAPPY_IMAGE}/bin/* ${SNAPPY_IMAGE}/lib/*

  # Copy in the Mesa DRI drivers
  COMMAND cp -Rdvp ${ARCH_LIB_DIR}/dri ${SNAPPY_IMAGE}/

  # Remove duplicate libs already built-in to Ubuntu Core
  COMMAND rm -f ${SNAPPY_IMAGE}/lib/libboost_program_options.so.1.55.0
  COMMAND rm -f ${SNAPPY_IMAGE}/lib/libstdc++.so.6
  COMMAND rm -f ${SNAPPY_IMAGE}/lib/libfreetype.so.6
  COMMAND rm -f ${SNAPPY_IMAGE}/lib/libunwind.so.8
  COMMAND rm -f ${SNAPPY_IMAGE}/lib/libffi.so.6

  COMMAND snappy build ${SNAPPY_IMAGE}
  COMMAND ls -l ${CMAKE_CURRENT_BINARY_DIR}/*.snap
)

