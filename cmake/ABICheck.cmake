cmake_minimum_required (VERSION 2.6)

#mirclient
set(LIB_DESC_HEADERS "${CMAKE_SOURCE_DIR}/include/client")
get_property(LIB_DESC_LIBS TARGET mirclient PROPERTY LOCATION)
set(LIB_DESC_INCLUDE_PATHS "${CMAKE_SOURCE_DIR}/include/client
    ${CMAKE_SOURCE_DIR}/include/common")
set(LIB_DESC_GCC_OPTS "${CMAKE_C_FLAGS}")
configure_file(${CMAKE_SOURCE_DIR}/tools/lib_descriptor.xml.skel mirclient_desc.xml) 

#mirserver
set(LIB_DESC_HEADERS "${CMAKE_SOURCE_DIR}/include/server")
get_property(LIB_DESC_LIBS TARGET mirserver PROPERTY LOCATION)
set(LIB_DESC_INCLUDE_PATHS "${CMAKE_SOURCE_DIR}/include/server
    ${CMAKE_SOURCE_DIR}/include/common
    ${CMAKE_SOURCE_DIR}/include/platform")
set(LIB_DESC_GCC_OPTS "${CMAKE_CXX_FLAGS}")
configure_file(${CMAKE_SOURCE_DIR}/tools/lib_descriptor.xml.skel mirserver_desc.xml) 

#mirplatform
set(LIB_DESC_HEADERS "${CMAKE_SOURCE_DIR}/include/platform")
get_property(LIB_DESC_LIBS TARGET mirplatform PROPERTY LOCATION)
set(LIB_DESC_INCLUDE_PATHS "${CMAKE_SOURCE_DIR}/include/platform
    ${CMAKE_SOURCE_DIR}/include/common")
configure_file(${CMAKE_SOURCE_DIR}/tools/lib_descriptor.xml.skel mirplatform_desc.xml) 

#mircommon
set(LIB_DESC_HEADERS "${CMAKE_SOURCE_DIR}/include/common")
get_property(LIB_DESC_LIBS TARGET mircommon PROPERTY LOCATION)
set(LIB_DESC_INCLUDE_PATHS "${CMAKE_SOURCE_DIR}/include/server
    ${CMAKE_SOURCE_DIR}/include/common
    /usr/include/android")
configure_file(${CMAKE_SOURCE_DIR}/tools/lib_descriptor.xml.skel mircommon_desc.xml) 

macro(_add_custom_abi_dump_command libname version)
  add_custom_command(OUTPUT ${libname}_${version}.abi.tar.gz
      COMMAND abi-compliance-checker -l ${libname} -v1 ${version} -dump-path ${libname}_${version}.abi.tar.gz -dump-abi ${libname}_desc.xml
      DEPENDS ${libname}
  )
endmacro(_add_custom_abi_dump_command)

macro(_define_abi_dump_for libname)
  _add_custom_abi_dump_command(${libname} next)
  add_custom_target(abi-dump-${libname} DEPENDS ${libname}_next.abi.tar.gz)
endmacro(_define_abi_dump_for)

macro(_define_abi_check_for libname)
    add_custom_target(abi-check-${libname} 
        COMMAND abi-compliance-checker -l ${libname} -old ${CMAKE_SOURCE_DIR}/tools/abi_dumps/${libname}_released.abi.tar.gz -new ${libname}_next.abi.tar.gz
        DEPENDS abi-dump-${libname}
    )
endmacro(_define_abi_check_for)

macro(_define_abi_update_for libname)
  _add_custom_abi_dump_command(${libname} released)
  add_custom_target(abi-update-${libname}
      COMMAND cp ${libname}_released.abi.tar.gz ${CMAKE_SOURCE_DIR}/tools/abi_dumps/${libname}_released.abi.tar.gz
      DEPENDS ${libname}_released.abi.tar.gz
  )
endmacro(_define_abi_update_for)

set(the_libs mirserver mirclient mircommon mirplatform)

foreach(libname ${the_libs})
  _define_abi_dump_for(${libname})
  _define_abi_check_for(${libname})
  _define_abi_update_for(${libname})
  list(APPEND abi-dump-list abi-dump-${libname})
  list(APPEND abi-check-list abi-check-${libname})
  list(APPEND abi-update-list abi-update-${libname})
endforeach(libname)

add_custom_target(abi-dump DEPENDS ${abi-dump-list})
add_custom_target(abi-check DEPENDS ${abi-check-list})
add_custom_target(abi-update DEPENDS ${abi-update-list})
