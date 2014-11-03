add_custom_target(benchmarks)   # XXX Is this used by anything?

add_subdirectory(cpu)
add_subdirectory(memory)
add_dependencies(benchmarks cpu_benchmarks memory_benchmarks)

if (MIR_ENABLE_TESTS)
  # Shouldn't tests dependent things be in tests/?
  add_subdirectory(android-input)
  add_subdirectory(frame-uniformity)
  add_dependencies(benchmarks frame_uniformity_test_client)
endif ()
