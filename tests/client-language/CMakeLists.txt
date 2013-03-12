set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c89 -Werror -Wall -Wextra" )

add_executable(client-language-test-c89
  c89.c
)
