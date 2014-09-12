cmake_minimum_required (VERSION 2.6)

execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpmachine OUTPUT_VARIABLE ABI_CHECK_TARGET_MACH OUTPUT_STRIP_TRAILING_WHITESPACE)
set(REL_ABI_DUMPS_DIR $ENV{REL_ABI_DUMPS_DIR})
if ("${REL_ABI_DUMPS_DIR}" STREQUAL "")
    set(REL_ABI_DUMPS_DIR ${CMAKE_BINARY_DIR}/abi_dumps)
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
  
  set(LIB_DESC_HEADERS "${CMAKE_SOURCE_DIR}/include/${name}
    ${private_headers}"
  )

  get_property(LIB_DESC_LIBS TARGET ${libname} PROPERTY LOCATION)

  get_includes(${libname} LIB_DESC_INCLUDE_PATHS)
  set(LIB_DESC_GCC_OPTS "${CMAKE_CXX_FLAGS}")

  get_value_for_arg("${ARGN}" "EXCLUDE_HEADERS" LIB_DESC_SKIP_HEADERS)

  configure_file(${CMAKE_SOURCE_DIR}/tools/lib_descriptor.xml.skel ${libname}_desc.xml)
endfunction()

#These headers are not part of the libmircommon interface
set(android-platform-headers "${CMAKE_SOURCE_DIR}/src/include/common/mir/graphics/android")

make_lib_descriptor(client)
make_lib_descriptor(server)
make_lib_descriptor(common INCLUDE_PRIVATE EXCLUDE_HEADERS ${android-platform-headers})
make_lib_descriptor(platform INCLUDE_PRIVATE)

macro(_add_custom_abi_dump_command libname version)
  set(ABI_DUMP_NAME ${ABI_DUMPS_DIR_PREFIX}/${libname}_${version}.abi.tar.gz)
  add_custom_command(OUTPUT ${ABI_DUMP_NAME}
    COMMAND abi-compliance-checker -gcc-path ${CMAKE_C_COMPILER} -l ${libname} -v1 ${version} -dump-path ${ABI_DUMP_NAME} -dump-abi ${libname}_desc.xml
    DEPENDS ${libname}
  )
endmacro(_add_custom_abi_dump_command)

macro(_define_abi_dump_for libname)
  _add_custom_abi_dump_command(${libname} next)
  add_custom_target(abi-dump-${libname} DEPENDS ${ABI_DUMP_NAME})
  _add_custom_abi_dump_command(${libname} base)
  add_custom_target(abi-dump-base-${libname} DEPENDS ${ABI_DUMP_NAME})
endmacro(_define_abi_dump_for)

macro(_define_abi_check_for libname) 
  set(OLD_ABI_DUMP "${REL_ABI_DUMPS_DIR}/${ABI_CHECK_TARGET_MACH}/${libname}_base.abi.tar.gz")
  set(NEW_ABI_DUMP ${ABI_DUMPS_DIR_PREFIX}/${libname}_next.abi.tar.gz)
  add_custom_target(abi-check-${libname} 
    COMMAND abi-compliance-checker -l ${libname} -old "${OLD_ABI_DUMP}" -new "${NEW_ABI_DUMP}" -check-implementation
    DEPENDS abi-dump-${libname}
  )
endmacro(_define_abi_check_for)

set(the_libs mirserver mirclient mircommon mirplatform)

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
