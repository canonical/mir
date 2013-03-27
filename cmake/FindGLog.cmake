if (GLog_INCLUDE_DIR)
  # Already in cache, be silent
  set(GLog_FIND_QUIETLY TRUE)
endif ()

find_path(GLog_INCLUDE_DIR glog/logging.h)

find_library(GLog_LIBRARY libglog.so
             HINTS /usr/lib/arm-linux-gnueabihf/)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GLog DEFAULT_MSG GLog_LIBRARY GLog_INCLUDE_DIR)

mark_as_advanced(GLog_LIBRARY GLog_INCLUDE_DIR)
