include_directories(${MIR_3RD_PARTY_INCLUDE_DIRECTORIES})
include_directories(${MIR_ANDROID_INCLUDE_DIRECTORIES})
add_custom_target(
  benchmarks
)

add_subdirectory(android-input)
add_subdirectory(cpu)
add_subdirectory(memory)

add_dependencies(benchmarks cpu_benchmarks memory_benchmarks)
