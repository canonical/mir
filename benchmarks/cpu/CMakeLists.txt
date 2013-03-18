find_program(
  VALGRIND_EXECUTABLE
  valgrind
)

if(NOT VALGRIND_EXECUTABLE)
  message("valgrind not found, disabling cpu benchmarks")
else()
  set(CPU_BENCHMARKS_SOCKET "/tmp/benchmarks.cpu.socket.mir")

  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/benchmark.sh.in
    ${CMAKE_CURRENT_BINARY_DIR}/benchmark.sh
    )

  add_custom_target(
    cpu_benchmark_one_server_one_client
    "${CMAKE_CURRENT_BINARY_DIR}/benchmark.sh"
    1
    1000
    one_server_one_client_results_for_client
    one_server_one_client_results_for_server
    )

  add_custom_target(
    cpu_benchmark_one_server_multiple_clients
    "${CMAKE_CURRENT_BINARY_DIR}/benchmark.sh"
    10
    100
    one_server_multiple_client_results_for_client
    one_server_multiple_client_results_for_server
    )

  add_custom_target(
    cpu_benchmark_one_server_multiple_clients_heavy_load
    "${CMAKE_CURRENT_BINARY_DIR}/benchmark.sh"
    20
    100
    one_server_multiple_client_heavy_load_results_for_client
    one_server_multiple_client_heavy_load_results_for_server
    )

  add_custom_target(
    cpu_benchmarks
    DEPENDS
    cpu_benchmark_one_server_one_client
    cpu_benchmark_one_server_multiple_clients
    cpu_benchmark_one_server_multiple_clients_heavy_load
    )

endif()
