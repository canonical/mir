set(MIR_PKG_ARCH ${CMAKE_SYSTEM_PROCESSOR})
# FIXME: This is blank on armhf cross-compiles. But don't care right now.
if ("${MIR_PKG_ARCH}" STREQUAL x86_64)
  set(MIR_PKG_ARCH amd64)
endif()

configure_file(mir.package.yaml.in mir.package.yaml)
configure_file(mir.framework.in mir.framework)
configure_file(mir-demos.package.yaml.in mir-demos.package.yaml)

set(ARCH_LIB_DIR /usr/lib/${CMAKE_LIBRARY_ARCHITECTURE})
set(MIR_IMAGE ${CMAKE_BINARY_DIR}/mir-image)
set(MIR_DEMOS_IMAGE ${CMAKE_BINARY_DIR}/mir-demos-image)

add_custom_target(snap
  # Create a clean stripped install tree
  COMMAND rm -rf ${MIR_IMAGE} ${MIR_DEMOS_IMAGE}
  COMMAND make -j8 -C ${CMAKE_BINARY_DIR} DESTDIR=${MIR_IMAGE} install/strip

  COMMAND mv -f ${MIR_IMAGE}/${CMAKE_INSTALL_PREFIX}/lib ${MIR_IMAGE}/
  COMMAND mv -f ${MIR_IMAGE}/${CMAKE_INSTALL_PREFIX}/bin ${MIR_DEMOS_IMAGE}/

  COMMAND mkdir ${MIR_IMAGE}/meta
  COMMAND cp ${CMAKE_CURRENT_BINARY_DIR}/mir.package.yaml ${MIR_IMAGE}/meta/package.yaml
  COMMAND cp ${CMAKE_CURRENT_BINARY_DIR}/mir.framework ${MIR_IMAGE}/meta/
  COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/readme.md ${MIR_IMAGE}/meta/
  COMMAND mkdir ${MIR_IMAGE}/bin
  COMMAND cp -p ${CMAKE_CURRENT_SOURCE_DIR}/mir-run ${MIR_IMAGE}/bin/

  COMMAND mkdir -p ${MIR_DEMOS_IMAGE}/meta
  COMMAND cp ${CMAKE_CURRENT_BINARY_DIR}/mir-demos.package.yaml ${MIR_DEMOS_IMAGE}/meta/package.yaml
  COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/readme.md ${MIR_DEMOS_IMAGE}/meta/

  # Remove bits of Mir we don't need
  COMMAND rm -rf ${MIR_IMAGE}/${CMAKE_INSTALL_PREFIX}

  # Copy in the DRM libraries needed by Mesa
  COMMAND cp -vdp ${ARCH_LIB_DIR}/libdrm_*.so* ${MIR_IMAGE}/lib/

  # Search all the binaries and copy more libraries they need
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/copy-external-libs.sh ${MIR_IMAGE}/lib/ ${MIR_DEMOS_IMAGE}/bin/* ${MIR_IMAGE}/lib/*

  # Copy in the Mesa DRI drivers
  COMMAND cp -Rdvp ${ARCH_LIB_DIR}/dri ${MIR_IMAGE}/

  # Remove duplicate libs already built-in to Ubuntu Core
  COMMAND rm -f ${MIR_IMAGE}/lib/libboost_program_options.so.1.55.0
  COMMAND rm -f ${MIR_IMAGE}/lib/libstdc++.so.6
  COMMAND rm -f ${MIR_IMAGE}/lib/libfreetype.so.6
  COMMAND rm -f ${MIR_IMAGE}/lib/libunwind.so.8
  COMMAND rm -f ${MIR_IMAGE}/lib/libffi.so.6

  COMMAND snappy build ${MIR_IMAGE}
  COMMAND snappy build ${MIR_DEMOS_IMAGE}
  COMMAND ls -l ${CMAKE_CURRENT_BINARY_DIR}/*.snap
)

