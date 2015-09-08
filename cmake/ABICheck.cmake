cmake_minimum_required (VERSION 2.6)

find_program(ABI_COMPLIANCE_CHECKER abi-compliance-checker)

if (NOT ABI_COMPLIANCE_CHECKER)
  message(WARNING "no ABI checks possible: abi-compliance-checker was not found")
  return()
endif()

execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpmachine OUTPUT_VARIABLE ABI_CHECK_TARGET_MACH OUTPUT_STRIP_TRAILING_WHITESPACE)

set(ABI_CHECK_BASE_DIR $ENV{MIR_ABI_CHECK_BASE_DIR})
set(ABI_DUMP_PREBUILT_LIBDIR $ENV{MIR_ABI_DUMP_PREBUILT_LIBDIR})
set(ENABLE_ABI_CHECK_TEST $ENV{MIR_ENABLE_ABI_CHECK_TEST})

if ("${ABI_CHECK_BASE_DIR}" STREQUAL "")
  set(ABI_CHECK_BASE_DIR ${CMAKE_BINARY_DIR}/mir-prev-release/obj-${ABI_CHECK_TARGET_MACH}/abi_dumps)
endif()

set(ABI_DUMPS_DIR_PREFIX "abi_dumps/${ABI_CHECK_TARGET_MACH}")

function(get_value_for_arg alist name output)
  list(FIND alist ${name} idx)
  if (idx GREATER -1)
    math(EXPR idx "${idx} + 1")
    list(GET alist ${idx} tmp_out)
    set(${output} "${tmp_out}" PARENT_SCOPE)
  endif()
endfunction()

function(get_includes libname output)
  get_property(lib_includes TARGET ${libname} PROPERTY INCLUDE_DIRECTORIES)
  list(REMOVE_DUPLICATES lib_includes)
  string(REPLACE ";" "\n    " tmp_out "${lib_includes}")
  set(${output} "${tmp_out}" PARENT_SCOPE)
endfunction()

function(make_lib_descriptor name)
  set(libname "mir${name}")

  list(FIND ARGN "INCLUDE_PRIVATE" include_private)
  if (include_private GREATER -1)
    set(private_headers "${CMAKE_SOURCE_DIR}/src/include/${name}")
  endif()
  
  get_value_for_arg("${ARGN}" "LIBRARY_HEADER" library_header)
  if ("${library_header}" STREQUAL "")
    set(LIB_DESC_HEADERS "${CMAKE_SOURCE_DIR}/include/${name}\n    ${private_headers}")
  else()
    set(LIB_DESC_HEADERS ${library_header})
  endif()

  if (NOT ${CMAKE_MAJOR_VERSION} LESS 3)
    cmake_policy(SET CMP0026 OLD)
  endif()
  # TODO: Deprecate use of "LOCATION" (CMP0026) ...
  if ("${ABI_DUMP_PREBUILT_LIBDIR}" STREQUAL "")
    get_property(LIB_DESC_LIBS TARGET ${libname} PROPERTY LOCATION)
  else()
    get_property(liblocation TARGET ${libname} PROPERTY LOCATION)
    get_filename_component(libfilename ${liblocation} NAME)
    set(LIB_DESC_LIBS "${ABI_DUMP_PREBUILT_LIBDIR}/${libfilename}")
  endif()

  get_includes(${libname} LIB_DESC_INCLUDE_PATHS)
  set(LIB_DESC_GCC_OPTS "${CMAKE_CXX_FLAGS}")

  get_value_for_arg("${ARGN}" "EXCLUDE_HEADERS" LIB_DESC_SKIP_HEADERS)

  configure_file(${CMAKE_SOURCE_DIR}/tools/lib_descriptor.xml.skel ${libname}_desc.xml)
endfunction()

#These headers are not part of the libmircommon ABI
set(mircommon-exclude-headers "${CMAKE_SOURCE_DIR}/src/include/common/mir/graphics/android\n    ${CMAKE_SOURCE_DIR}/src/include/common/mir/input\n    ${CMAKE_SOURCE_DIR}/src/include/common/mir/events")

#These headers are not part of the libmirplatform ABI
set(mirplatform-exclude-headers "${CMAKE_SOURCE_DIR}/include/platform/mir/input")

make_lib_descriptor(client)
make_lib_descriptor(server)
make_lib_descriptor(common INCLUDE_PRIVATE EXCLUDE_HEADERS ${mircommon-exclude-headers})
make_lib_descriptor(cookie)
make_lib_descriptor(platform INCLUDE_PRIVATE EXCLUDE_HEADERS ${mirplatform-exclude-headers})
if(MIR_BUILD_PLATFORM_MESA_KMS)
make_lib_descriptor(clientplatformmesa LIBRARY_HEADER ${CMAKE_SOURCE_DIR}/src/include/client/mir/client_platform_factory.h)
make_lib_descriptor(platformgraphicsmesakms LIBRARY_HEADER ${CMAKE_SOURCE_DIR}/include/platform/mir/graphics/platform.h)
endif()
if(MIR_BUILD_PLATFORM_ANDROID)
make_lib_descriptor(clientplatformandroid LIBRARY_HEADER ${CMAKE_SOURCE_DIR}/src/include/client/mir/client_platform_factory.h)
make_lib_descriptor(platformgraphicsandroid LIBRARY_HEADER ${CMAKE_SOURCE_DIR}/include/platform/mir/graphics/platform.h)
endif()

add_custom_target(abi-release-dump
  COMMAND /bin/sh -c '${CMAKE_SOURCE_DIR}/tools/generate-abi-base-dump.sh ${CMAKE_SOURCE_DIR}'
)

macro(_add_custom_abi_dump_command libname version)
  set(ABI_DUMP_NAME ${ABI_DUMPS_DIR_PREFIX}/${libname}_${version}.abi.tar.gz)

  if ("${ABI_DUMP_PREBUILT_LIBDIR}" STREQUAL "")
    set(dump_depends ${libname})
  else()
    set(dump_depends "")
  endif()

  add_custom_command(OUTPUT ${ABI_DUMP_NAME}
    COMMAND abi-compliance-checker -gcc-path ${CMAKE_C_COMPILER} -l ${libname} -v1 ${version} -dump-path ${ABI_DUMP_NAME} -dump-abi ${libname}_desc.xml
    DEPENDS ${dump_depends}
  )
endmacro(_add_custom_abi_dump_command)

macro(_define_abi_dump_for libname)
  _add_custom_abi_dump_command(${libname} next)
  add_custom_target(abi-dump-${libname} DEPENDS ${ABI_DUMP_NAME})
  _add_custom_abi_dump_command(${libname} base)
  add_custom_target(abi-dump-base-${libname} DEPENDS ${ABI_DUMP_NAME})
endmacro(_define_abi_dump_for)

macro(_define_abi_check_for libname) 
  set(OLD_ABI_DUMP "${ABI_CHECK_BASE_DIR}/${ABI_CHECK_TARGET_MACH}/${libname}_base.abi.tar.gz")
  set(NEW_ABI_DUMP ${ABI_DUMPS_DIR_PREFIX}/${libname}_next.abi.tar.gz)
  add_custom_target(abi-check-${libname} 
    COMMAND /bin/bash -c '${CMAKE_SOURCE_DIR}/tools/run_abi_compliance_checker.sh ${libname} ${CMAKE_BINARY_DIR}/mir-prev-release ${CMAKE_SOURCE_DIR} ${OLD_ABI_DUMP} ${NEW_ABI_DUMP}'
    DEPENDS abi-dump-${libname} abi-release-dump
  )
endmacro(_define_abi_check_for)

set(the_libs mirserver mirclient mircommon mirplatform mircookie)
if(MIR_BUILD_PLATFORM_MESA_KMS)
  set(the_libs ${the_libs} mirclientplatformmesa mirplatformgraphicsmesakms)
endif()
if(MIR_BUILD_PLATFORM_ANDROID)
  set(the_libs ${the_libs} mirclientplatformandroid mirplatformgraphicsandroid)
endif()

foreach(libname ${the_libs})
  _define_abi_dump_for(${libname})
  _define_abi_check_for(${libname})
  list(APPEND abi-dump-list abi-dump-${libname})
  list(APPEND abi-dump-base-list abi-dump-base-${libname})
  list(APPEND abi-check-list abi-check-${libname})
endforeach(libname)

add_custom_target(abi-dump DEPENDS ${abi-dump-list})
add_custom_target(abi-dump-base DEPENDS ${abi-dump-base-list})
add_custom_target(abi-check DEPENDS ${abi-check-list})

if (MIR_ENABLE_TESTS AND ENABLE_ABI_CHECK_TEST)
  add_test(abi-compliance-check make abi-check)
endif()
