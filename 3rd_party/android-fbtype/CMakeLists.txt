# Don't be so strict on warnings etc as android code won't compile with them on.
# A *big* number of warnings would ensue. Therefore locally define CMAKE_CXX_FLAGS
# without things such as -Werror, -Wall, etc
STRING(REGEX REPLACE "-pedantic" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
STRING(REGEX REPLACE "-W[^ ]*" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})

add_library(
  3rd_party_fbtype STATIC
  
  FramebufferNativeWindow.cpp
)
