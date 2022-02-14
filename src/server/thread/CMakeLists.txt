set(
  MIR_THREAD_SRCS

  basic_thread_pool.cpp
)

ADD_LIBRARY(
  mirthread OBJECT

  ${MIR_THREAD_SRCS}
)

