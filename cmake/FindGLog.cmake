if (GLog_INCLUDE_DIR)
  # Already in cache, be silent
  set(GLog_FIND_QUIETLY TRUE)
endif ()

find_path(GLog_INCLUDE_DIR glog/logging.h)

if (MIR_PLATFORM STREQUAL "android") 
find_library(GLog_LIBRARY libglog.so.0
             HINTS /usr/lib/arm-linux-gnueabihf/)
else()
find_library(GLog_LIBRARY glog)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GLog DEFAULT_MSG GLog_LIBRARY GLog_INCLUDE_DIR)

#mark_as_advanced(GLog_LIBRARY GLog_INCLUDE_DIR)
