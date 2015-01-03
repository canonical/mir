project(mir_stress)

mir_add_wrapped_executable(mir_stress
  src/client.cpp
  src/mir-stress.cpp
  src/results.cpp
  src/threading.cpp
)

target_link_libraries(mir_stress 
  mirclient
)
