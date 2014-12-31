set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")

mir_add_wrapped_executable(client-language-test-c99
  c99.c
)
