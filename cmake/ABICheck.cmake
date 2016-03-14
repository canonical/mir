cmake_minimum_required (VERSION 2.6)

find_program(ABI_COMPLIANCE_CHECKER abi-compliance-checker)
if (NOT ABI_COMPLIANCE_CHECKER)
  message(WARNING "no ABI checks possible: abi-compliance-checker was not found")
  return()
endif()

set(ENABLE_ABI_CHECK_TEST $ENV{MIR_ENABLE_ABI_CHECK_TEST})
execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpmachine OUTPUT_VARIABLE ABI_CHECK_TARGET_MACH OUTPUT_STRIP_TRAILING_WHITESPACE)

set(ABI_DUMPS_DIR "${CMAKE_BINARY_DIR}/abi_dumps/${ABI_CHECK_TARGET_MACH}")

# Given a list of key value pairs such as "key1 value1 key2 value2...keyN valueN"
# extract the value corresponding to the given key
function(get_value_for_key a_list key value)
  list(FIND a_list ${key} idx)
  if (idx GREATER -1)
    math(EXPR idx "${idx} + 1")
    list(GET a_list ${idx} tmp_value)
    set(${value} "${tmp_value}" PARENT_SCOPE)
  endif()
endfunction()

# Makes a one-entry per line list of all include paths used
# to compile the given library target
function(get_includes libname output)
  get_property(lib_includes TARGET ${libname} PROPERTY INCLUDE_DIRECTORIES)
  list(REMOVE_DUPLICATES lib_includes)
  string(REPLACE ";" "\n    " tmp_out "${lib_includes}")
  set(${output} "${tmp_out}" PARENT_SCOPE)
endfunction()

# Creates the XML descriptor file that describes the given library target
# suitable for abi-compliance-checker
function(make_lib_descriptor name)
  set(libname "mir${name}")

  # Optional argument LIBRARY_HEADER - use the given header to describe
  # the binary library instead of assuming its described by include/<name>
  get_value_for_key("${ARGN}" "LIBRARY_HEADER" library_header)
  if ("${library_header}" STREQUAL "")
    set(LIB_DESC_HEADERS "${CMAKE_SOURCE_DIR}/include/${name}\n    ${private_headers}")
  else()
    set(LIB_DESC_HEADERS ${library_header})
  endif()

  # FIXME: Property "LOCATION" is now deprecated
  if (NOT ${CMAKE_MAJOR_VERSION} LESS 3)
    cmake_policy(SET CMP0026 OLD)
  endif()
  get_property(LIB_DESC_LIBS TARGET ${libname} PROPERTY LOCATION)
    
  get_includes(${libname} LIB_DESC_INCLUDE_PATHS)
  set(LIB_DESC_GCC_OPTS "${CMAKE_CXX_FLAGS}")

  # Optional EXCLUDE_HEADERS - a list 
  # while attempting an abi dump
  get_value_for_key("${ARGN}" "EXCLUDE_HEADERS" LIB_DESC_SKIP_HEADERS)

  configure_file(${CMAKE_SOURCE_DIR}/tools/lib_descriptor.xml.skel ${libname}_desc.xml)
endfunction()

#These headers are not part of the libmirplatform ABI
set(mirplatform-exclude-headers "${CMAKE_SOURCE_DIR}/include/platform/mir/input")

make_lib_descriptor(client)
make_lib_descriptor(server)
make_lib_descriptor(common)
make_lib_descriptor(cookie)
make_lib_descriptor(platform EXCLUDE_HEADERS ${mirplatform-exclude-headers})
if(MIR_BUILD_PLATFORM_MESA_KMS)
make_lib_descriptor(clientplatformmesa LIBRARY_HEADER ${CMAKE_SOURCE_DIR}/src/include/client/mir/client_platform_factory.h)
make_lib_descriptor(platformgraphicsmesakms LIBRARY_HEADER ${CMAKE_SOURCE_DIR}/include/platform/mir/graphics/platform.h)
endif()
if(MIR_BUILD_PLATFORM_ANDROID)
make_lib_descriptor(clientplatformandroid LIBRARY_HEADER ${CMAKE_SOURCE_DIR}/src/include/client/mir/client_platform_factory.h)
make_lib_descriptor(platformgraphicsandroid LIBRARY_HEADER ${CMAKE_SOURCE_DIR}/include/platform/mir/graphics/platform.h)
endif()
make_lib_descriptor(platforminputevdev LIBRARY_HEADER ${CMAKE_SOURCE_DIR}/include/platform/mir/input/)


macro(_define_abi_dump_for libname)
  set(ABI_DUMP_NAME ${ABI_DUMPS_DIR}/${libname}_next.abi.tar.gz)

  add_custom_command(OUTPUT ${ABI_DUMP_NAME}
    COMMAND abi-compliance-checker -gcc-path ${CMAKE_CXX_COMPILER} -l ${libname} -v1 next -dump-path ${ABI_DUMP_NAME} -dump-abi ${libname}_desc.xml
    DEPENDS ${libname}
  )
  add_custom_target(abi-dump-${libname} DEPENDS ${ABI_DUMP_NAME})
endmacro(_define_abi_dump_for)

macro(_define_abi_check_for libname) 
  add_custom_target(abi-check-${libname} 
    COMMAND /bin/bash -c '${CMAKE_SOURCE_DIR}/tools/abi_check.sh ${libname} ${ABI_DUMPS_DIR} ${CMAKE_SOURCE_DIR}'
    DEPENDS abi-dump-${libname}
  )
endmacro(_define_abi_check_for)

set(the_libs mirserver mirclient mircommon mirplatform mircookie mirplatforminputevdev)
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
  list(APPEND abi-check-list abi-check-${libname})
endforeach(libname)

add_custom_target(abi-dump DEPENDS ${abi-dump-list})
add_custom_target(abi-check DEPENDS ${abi-check-list})

if (MIR_ENABLE_TESTS AND ENABLE_ABI_CHECK_TEST)
  add_test(abi-compliance-check make abi-check)
endif()
