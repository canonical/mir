add_custom_target(
  benchmarks
)

add_subdirectory(android-input)
add_subdirectory(cpu)
add_subdirectory(memory)

add_dependencies(benchmarks cpu_benchmarks memory_benchmarks)
