
# https://isocpp.org/std/standing-documents/sd-6-sg10-feature-test-recommendations#table-of-feature-test-macros
# https://en.cppreference.com/w/cpp/feature_test
# https://en.cppreference.com/w/cpp/utility/feature_test

include_guard(GLOBAL)

#include (CheckSymbolExists)
include (CheckCXXSymbolExists)

macro(REQUIRE_CXX_FEATURE SYMBOL)
  if (NOT DEFINED "${SYMBOL}")
    # Feature testing is only available since C++20
    if (NOT CMAKE_CXX_COMPILE_FEATURES MATCHES "cxx_std_20")
      message(FATAL_ERROR "C++20 support is required!\n")
    endif()

    # Header containing the available C++ standard library features
    set(FILES "version")

    #__CHECK_SYMBOL_EXISTS_IMPL(RequireCXXFeature.cxx "${SYMBOL}" "${FILES}" "${SYMBOL}")
    check_cxx_symbol_exists("${SYMBOL}" "${FILES}" "${SYMBOL}")

    # To us, failure is a fatal error
    if (NOT "${${SYMBOL}}")
      # Unset the variable cached by "symbol exists" check
      unset("${SYMBOL}" CACHE)
      message(FATAL_ERROR "Required C++ feature ${SYMBOL} not found!\nSee the feature test documentation, or https://en.cppreference.com/w/cpp/feature_test for details.\n")
    endif()
  endif()
endmacro()
