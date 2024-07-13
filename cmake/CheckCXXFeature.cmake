
# https://isocpp.org/std/standing-documents/sd-6-sg10-feature-test-recommendations#table-of-feature-test-macros
# https://en.cppreference.com/w/cpp/feature_test
# https://en.cppreference.com/w/cpp/utility/feature_test

include_guard(GLOBAL)

#include (CheckSymbolExists)
include (CheckCXXSymbolExists)

macro(CHECK_CXX_FEATURE SYMBOL VARIABLE)
  if (NOT DEFINED "${SYMBOL}")
    # Feature testing is only available since C++20
    if (NOT CMAKE_CXX_COMPILE_FEATURES MATCHES "cxx_std_20")
      message(FATAL_ERROR "C++20 support is required!\n")
    endif()

    # Header containing the available C++ standard library features
    set(FILES "version")

    #__CHECK_SYMBOL_EXISTS_IMPL(RequireCXXFeature.cxx "${SYMBOL}" "${FILES}" "${VARIABLE}")
    check_cxx_symbol_exists("${SYMBOL}" "${FILES}" "${VARIABLE}")
  endif()
endmacro()
