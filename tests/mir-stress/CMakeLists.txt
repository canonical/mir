project(mir_stress)

set( CMAKE_CXX_FLAGS "-std=c++11 -ggdb")

add_executable(mir_stress src/mir-stress.cpp src/client.cpp src/threading.cpp src/results.cpp)

target_link_libraries(
  mir_stress

  mirclient
)
install(
	TARGETS mir_stress
	RUNTIME DESTINATION bin
)
