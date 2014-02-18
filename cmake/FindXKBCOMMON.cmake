pkg_check_modules( PC_XKBCOMMON QUIET xkbcommon )

find_path(XKBCOMMON_INCLUDE_DIR xkbcommon/xkbcommon.h
          HINTS ${PC_XKBCOMMON_INCLUDEDIR} ${PC_XKBCOMMON_INCLUDE_DIRS})

find_library(XKBCOMMON_LIBRARY xkbcommon
             HINTS ${PC_XKBCOMMON_LIBDIR} ${PC_XKBCOMMON_LIBRARY_DIRS})

set(XKBCOMMON_LIBRARIES ${XKBCOMMON_LIBRARY})
set(XKBCOMMON_INCLUDE_DIRS ${XKBCOMMON_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set XKBCOMMON_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(XKBCOMMON  DEFAULT_MSG
                                  XKBCOMMON_LIBRARY XKBCOMMON_INCLUDE_DIR)

mark_as_advanced(XKBCOMMON_INCLUDE_DIR XKBCOMMON_LIBRARY)
