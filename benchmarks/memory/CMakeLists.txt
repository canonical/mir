find_program(
  VALGRIND_EXECUTABLE
  valgrind
)

if(NOT VALGRIND_EXECUTABLE)
  message("valgrind not found, disabling memory benchmarks")
else()

  set(MEMORY_BENCHMARKS_SOCKET "/tmp/benchmarks.memory.socket.mir")

  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/benchmark.sh.in
    ${CMAKE_CURRENT_BINARY_DIR}/benchmark.sh
    )

  add_custom_target(
    memory_benchmark_one_server_one_client
    "${CMAKE_CURRENT_BINARY_DIR}/benchmark.sh"
    1
    100
    one_server_one_client_results_for_client
    one_server_one_client_results_for_server
    )

  add_custom_target(
    memory_benchmark_one_server_multiple_clients
    "${CMAKE_CURRENT_BINARY_DIR}/benchmark.sh"
    10
    100
    one_server_multiple_clients_results_for_client
    one_server_multiple_clients_results_for_server
    )

  add_custom_target(
    memory_benchmark_one_server_multiple_clients_heavy_load
    "${CMAKE_CURRENT_BINARY_DIR}/benchmark.sh"
    20
    100
    one_server_multiple_clients_heavy_load_results_for_client
    one_server_multiple_clients_heavy_load_results_for_server
    )

  add_custom_target(
    memory_benchmarks
    DEPENDS
    memory_benchmark_one_server_one_client
    memory_benchmark_one_server_multiple_clients
    memory_benchmark_one_server_multiple_clients_heavy_load
    )

endif()
