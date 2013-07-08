project(mir_stress)

set(SOURCES
	src/client.cpp
	src/mir-stress.cpp
	src/results.cpp
	src/threading.cpp
	)

add_executable(mir_stress ${SOURCES})

target_link_libraries(
  mir_stress

  mirclient
)
install(
	TARGETS mir_stress
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
