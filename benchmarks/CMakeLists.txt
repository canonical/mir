add_custom_target(
  benchmarks
)

if (MIR_ENABLE_TESTS)
  add_subdirectory(android-input)
endif ()
add_subdirectory(cpu)
add_subdirectory(memory)

add_dependencies(benchmarks cpu_benchmarks memory_benchmarks)
