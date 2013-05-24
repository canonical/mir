# - Try to find LTTng-UST
# Once done this will define
#  LTTNG_UST_FOUND - System has LTTng-UST
#  LTTNG_UST_INCLUDE_DIRS - The LTTng-UST include directories
#  LTTNG_UST_LIBRARIES - The libraries needed to use LTTng-UST

find_package(PkgConfig)

# pkgconfig is currently broken for LTTng-UST and urcu-bp
#pkg_check_modules(PC_LTTNG_UST QUIET lttng-ust)
#pkg_check_modules(PC_LIBURCU_BP QUIET liburcu-bp)

find_path(LTTNG_UST_INCLUDE_DIR lttng/tracepoint.h
          HINTS ${PC_LTTNG_UST_INCLUDEDIR} ${PC_LTTNG_UST_INCLUDE_DIRS})

find_library(LTTNG_UST_LIBRARY lttng-ust
             HINTS ${PC_LTTNG_UST_LIBDIR} ${PC_LTTNG_UST_LIBRARY_DIRS})

find_library(LIBURCU_BP_LIBRARY urcu-bp
             HINTS ${PC_LIBURCU_BP_LIBDIR} ${PC_LIBURCU_BP_LIBRARY_DIRS})

set(LTTNG_UST_LIBRARIES ${LTTNG_UST_LIBRARY} ${LIBURCU_BP_LIBRARY} -ldl)
set(LTTNG_UST_INCLUDE_DIRS ${LTTNG_UST_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set LTTNG_UST_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(LTTNG_UST DEFAULT_MSG
                                  LTTNG_UST_LIBRARY
                                  LIBURCU_BP_LIBRARY
                                  LTTNG_UST_INCLUDE_DIR)

mark_as_advanced(LTTNG_UST_INCLUDE_DIR LTTNG_UST_LIBRARY)
