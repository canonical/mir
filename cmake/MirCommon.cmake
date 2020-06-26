cmake_minimum_required (VERSION 2.6)
# Create target to discover tests
include (CMakeParseArguments)

include(CMakeDependentOption)
file(REMOVE ${CMAKE_BINARY_DIR}/discover_all_tests.sh)

option(
  ENABLE_MEMCHECK_OPTION
  "If set to ON, enables automatic creation of memcheck targets"
  OFF
)

option(
  MIR_USE_PRECOMPILED_HEADERS
  "Use precompiled headers"
  ON
)

if(ENABLE_MEMCHECK_OPTION)
  find_program(
    VALGRIND_EXECUTABLE
    valgrind)

  if(VALGRIND_EXECUTABLE)
    set(VALGRIND_CMD "${VALGRIND_EXECUTABLE}" "--error-exitcode=1" "--trace-children=yes")
    set(VALGRIND_CMD ${VALGRIND_CMD} "--leak-check=full" "--show-leak-kinds=definite" "--errors-for-leak-kinds=definite")
    set(VALGRIND_CMD ${VALGRIND_CMD} "--track-fds=yes")
    set(VALGRIND_CMD ${VALGRIND_CMD} "--track-origins=yes")
    set(VALGRIND_CMD ${VALGRIND_CMD} "--num-callers=128")
    set(VALGRIND_CMD ${VALGRIND_CMD} "--suppressions=${CMAKE_SOURCE_DIR}/tools/valgrind_suppressions_generic")
    set(VALGRIND_CMD ${VALGRIND_CMD} "--suppressions=${CMAKE_SOURCE_DIR}/tools/valgrind_suppressions_glibc_2.23")
    set(VALGRIND_CMD ${VALGRIND_CMD} "--suppressions=${CMAKE_SOURCE_DIR}/tools/valgrind_suppressions_libhybris")
    if (TARGET_ARCH STREQUAL "arm-linux-gnueabihf")
      set(VALGRIND_CMD ${VALGRIND_CMD} "--suppressions=${CMAKE_SOURCE_DIR}/tools/valgrind_suppressions_armhf")
    endif()
  else(VALGRIND_EXECUTABLE)
    message("Not enabling memcheck as valgrind is missing on your system")
  endif(VALGRIND_EXECUTABLE)
endif(ENABLE_MEMCHECK_OPTION)

if(CMAKE_CROSSCOMPILING)
    set(SYSTEM_SUPPORTS_O_TMPFILE 0)
else()
    try_run(SYSTEM_SUPPORTS_O_TMPFILE SYSTEM_HEADERS_SUPPORT_O_TMPFILE
      ${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR}/cmake/src/mir/mir_test_tmpfile.cpp
      )
endif()

function (list_to_string LIST_VAR PREFIX STR_VAR)
  foreach (value ${LIST_VAR})
    set(tmp_str "${tmp_str} ${PREFIX} ${value}")
  endforeach()
  set(${STR_VAR} "${tmp_str}" PARENT_SCOPE)
endfunction()

function (mir_discover_tests_internal EXECUTABLE TEST_ENV_OPTIONS DETECT_FD_LEAKS )
  # Set vars
  set(test_cmd_no_memcheck "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${EXECUTABLE}")
  set(test_cmd "${test_cmd_no_memcheck}")
  set(test_env ${ARGN} ${TEST_ENV_OPTIONS})

  if (TEST_ENV_OPTIONS)
      set(test_name ${EXECUTABLE}---${TEST_ENV_OPTIONS}---)
  else()
      set(test_name ${EXECUTABLE})
  endif()
  set(test_no_memcheck_filter)
  set(test_exclusion_filter)

  if(ENABLE_MEMCHECK_OPTION)
    set(test_cmd ${VALGRIND_CMD} ${test_cmd_no_memcheck})
    set(test_no_memcheck_filter "*DeathTest.*:ClientLatency.*")
  endif()

  if(cmake_build_type_lower MATCHES "threadsanitizer")
    if (NOT CMAKE_COMPILER_IS_GNUCXX)
      find_program(LLVM_SYMBOLIZER llvm-symbolizer-3.6)
      if (LLVM_SYMBOLIZER)
        set(TSAN_EXTRA_OPTIONS "external_symbolizer_path=${LLVM_SYMBOLIZER}")
      endif()
    endif()
    # Space after ${TSAN_EXTRA_OPTIONS} works around bug in TSAN env. variable parsing 
    list(APPEND test_env "TSAN_OPTIONS=\"suppressions=${CMAKE_SOURCE_DIR}/tools/tsan-suppressions second_deadlock_stack=1 halt_on_error=1 history_size=7 die_after_fork=0 ${TSAN_EXTRA_OPTIONS} \"")
     # TSan does not support starting threads after fork
    set(test_exclusion_filter "${test_exclusion_filter}:ThreadedDispatcherSignalTest.keeps_dispatching_after_signal_interruption")
    # tsan "eats" SIGQUIT, so ignore two more tests that involve it
    set(test_exclusion_filter "${test_exclusion_filter}:ServerSignal/AbortDeathTest.cleanup_handler_is_called_for/0")
    set(test_exclusion_filter "${test_exclusion_filter}:ServerShutdown/OnSignalDeathTest.removes_endpoint/0")
  endif()

  if(cmake_build_type_lower MATCHES "ubsanitizer")
    list(APPEND test_env "UBSAN_OPTIONS=\"suppressions=${CMAKE_SOURCE_DIR}/tools/ubsan-suppressions print_stacktrace=1 die_after_fork=0\"")
    set(test_exclusion_filter "${test_exclusion_filter}:*DeathTest*")
  endif()

  if(SYSTEM_SUPPORTS_O_TMPFILE EQUAL 1)
      set(test_exclusion_filter "${test_exclusion_filter}:AnonymousShmFile.*:MesaBufferAllocatorTest.software_buffers_dont_bypass:MesaBufferAllocatorTest.creates_software_rendering_buffer")
  endif()

  # liblttng-ust is unsafe if you fork() without exec(), such as we do in the test suite
  # We need to load liblttng-ust-fork.so to make this work reliably.
  list(APPEND test_env "LD_PRELOAD=liblttng-ust-fork.so")

  # However, we *also* need to respect any existing LD_PRELOADs in the environment,
  # and only the last LD_PRELOAD set will win. So we need to coalesce the LD_PRELOADs
  set(env_without_preloads ${test_env})
  set(env_only_preloads ${test_env})

  # Split the list into the LD_PRELOAD elements, and everything else…
  list(FILTER env_without_preloads EXCLUDE REGEX "^LD_PRELOAD=.+")
  list(FILTER env_only_preloads INCLUDE REGEX "^LD_PRELOAD=.+")

  foreach(preload ${env_only_preloads})
    # Concatenate all the preloads
    string(SUBSTRING "${preload}" 11 -1 preload)
    list(APPEND env_preloads "${preload}")
  endforeach()

  if (env_preloads)
    # Join the list with colons
    string(REPLACE ";" ":" env_preloads "${env_preloads}")
    # Add the LD_PRELOAD=… to the end of the non-LD_PRELOAD list…
    list(APPEND env_without_preloads "LD_PRELOAD=${env_preloads}")
    # …and now replace the original environment list.
    set(test_env ${env_without_preloads})
  endif()

  # Final commands
  set(test_cmd "${test_cmd}" "--gtest_filter=-${test_no_memcheck_filter}:${test_exclusion_filter}")
  set(test_cmd_no_memcheck "${test_cmd_no_memcheck}" "--gtest_death_test_style=threadsafe" "--gtest_filter=${test_no_memcheck_filter}:-${test_exclusion_filter}")
  if(DETECT_FD_LEAKS)
    set(test_cmd ${CMAKE_SOURCE_DIR}/tools/detect_fd_leaks.bash ${test_cmd})
  endif()

  # Normal
  add_test(${test_name} ${test_cmd})
  set_property(TEST ${test_name} PROPERTY ENVIRONMENT ${test_env})
  if (test_no_memcheck_filter)
    add_test(${test_name}_no_memcheck ${test_cmd_no_memcheck})
    set_property(TEST ${test_name}_no_memcheck PROPERTY ENVIRONMENT ${test_env})
  endif()

  # ptest
  list_to_string("${test_env}" "--env" discover_env)
  list_to_string("${test_cmd}" "" discover_cmd)
  list_to_string("${test_cmd_no_memcheck}" "" discover_cmd_no_memcheck)

  file(APPEND ${CMAKE_BINARY_DIR}/discover_all_tests.sh
    "sh ${CMAKE_SOURCE_DIR}/tools/discover_gtests.sh ${discover_env} --test-name ${test_name} -- ${discover_cmd}\n")
  if (test_no_memcheck_filter)
    file(APPEND ${CMAKE_BINARY_DIR}/discover_all_tests.sh
      "sh ${CMAKE_SOURCE_DIR}/tools/discover_gtests.sh ${discover_env} --test-name ${test_name}_no_memcheck -- ${discover_cmd_no_memcheck}\n")
  endif()
endfunction ()

function (mir_discover_tests EXECUTABLE)
  mir_discover_tests_internal(${EXECUTABLE} "" FALSE ${ARGN})
endfunction()

function (mir_discover_tests_with_fd_leak_detection EXECUTABLE)
  mir_discover_tests_internal(${EXECUTABLE} "" TRUE ${ARGN})
endfunction()

function (mir_discover_external_gtests)
  set(one_value_args NAME COMMAND WORKING_DIRECTORY)
  set(multi_value_args EXPECTED_FAILURES ARGS)
  cmake_parse_arguments(TEST "" "${one_value_args}" "${multi_value_args}" ${ARGN})

  # The expected failures, in a colon-delimited list for GTest
  string(REPLACE ";" ":" EXPECTED_FAILURE_STRING "${TEST_EXPECTED_FAILURES}")
  # The command line arguments, as would be passed to the shell
  string(REPLACE ";" " " TEST_ARGS_STRING "${TEST_ARGS}")

  add_test(NAME ${TEST_NAME} COMMAND ${TEST_COMMAND} "--gtest_filter=-${EXPECTED_FAILURE_STRING}" ${TEST_ARGS})
  if (TEST_WORKING_DIRECTORY)
    set_tests_properties(${TEST_NAME} PROPERTIES WORKING_DIRECTORY ${TEST_WORKING_DIRECTORY})
  endif()

  file(APPEND ${CMAKE_BINARY_DIR}/discover_all_tests.sh
    "sh ${CMAKE_SOURCE_DIR}/tools/discover_gtests.sh --test-name ${TEST_NAME} --gtest-executable \"${TEST_COMMAND}\" -- ${TEST_COMMAND} --gtest_filter=-${EXPECTED_FAILURE_STRING} ${TEST_ARGS_STRING}\n")

  foreach (xfail IN LISTS TEST_EXPECTED_FAILURES)
    # Add a test verifying that the expected failures really do fail
    add_test(
      NAME "${TEST_NAME}_${xfail}_fails"
      COMMAND
        ${CMAKE_BINARY_DIR}/mir_gtest/xfail_if_gtest_exists.sh ${TEST_COMMAND}
        ${xfail}
        ${TEST_ARGS}
    )
    if (TEST_WORKING_DIRECTORY)
      set_tests_properties("${TEST_NAME}_${xfail}_fails" PROPERTIES WORKING_DIRECTORY ${TEST_WORKING_DIRECTORY})
    endif()

    # Unsupported for ptest, but this should be ok
  endforeach ()
endfunction()

function (mir_add_memcheck_test)
  if (ENABLE_MEMCHECK_OPTION)
    add_custom_target(memcheck_test ALL)
    mir_add_test(NAME "memcheck-test"
      COMMAND ${CMAKE_BINARY_DIR}/mir_gtest/fail_on_success.sh ${VALGRIND_CMD} ${CMAKE_BINARY_DIR}/mir_gtest/mir_test_memory_error)
    add_dependencies(memcheck_test mir_test_memory_error)
  endif()
endfunction()

function (mir_add_detect_fd_leaks_test)
  if (ENABLE_MEMCHECK_OPTION)
    add_custom_target(detect_fd_leaks_catches_fd_leak_test ALL)
    mir_add_test(NAME "detect-fd-leaks-catches-fd-leak"
      COMMAND ${CMAKE_BINARY_DIR}/mir_gtest/fail_on_success.sh ${CMAKE_SOURCE_DIR}/tools/detect_fd_leaks.bash ${VALGRIND_CMD} ${CMAKE_BINARY_DIR}/mir_gtest/mir_test_fd_leak)
    add_dependencies(detect_fd_leaks_catches_fd_leak_test mir_test_fd_leak)

    add_custom_target(detect_fd_leaks_propagates_test_failure_test ALL)
    mir_add_test(NAME "detect-fd-leaks-propagates-test-failure"
      COMMAND ${CMAKE_BINARY_DIR}/mir_gtest/fail_on_success.sh ${CMAKE_SOURCE_DIR}/tools/detect_fd_leaks.bash ${VALGRIND_CMD} ${CMAKE_BINARY_DIR}/mir_gtest/mir_test_memory_error)
    add_dependencies(detect_fd_leaks_propagates_test_failure_test mir_test_memory_error)
  endif()
endfunction()

function (mir_precompiled_header TARGET HEADER)
  if (MIR_USE_PRECOMPILED_HEADERS)
    get_filename_component(HEADER_NAME ${HEADER} NAME)

    get_property(TARGET_INCLUDE_DIRECTORIES TARGET ${TARGET} PROPERTY INCLUDE_DIRECTORIES)

    set(TARGET_COMPILE_DEFINITIONS "$<TARGET_PROPERTY:${TARGET},COMPILE_DEFINITIONS>")
    set(TARGET_COMPILE_DEFINITIONS "$<$<BOOL:${TARGET_COMPILE_DEFINITIONS}>:-D$<JOIN:${TARGET_COMPILE_DEFINITIONS},\n-D>\n>")

    foreach(dir ${TARGET_INCLUDE_DIRECTORIES})
      set(TARGET_INCLUDE_DIRECTORIES_STRING "${TARGET_INCLUDE_DIRECTORIES_STRING} -I${dir}")
    endforeach()

    # So.
    # ${CMAKE_CXX_FLAGS} *only* includes the base flags, not any extra flags set by the build target.
    # The build targets set flags which affect the precompiled headers - -g verses no debug for the Debug build,
    # -g -O2 -NDEBUG versus no specified optimisation for RelWithDebugInfo, etc.
    #
    # The various CMAKE_CXX_FLAGS_DEBUG, CMAKE_CXX_FLAGS_RELWITHDEBUGINFO, etc variables have the extra flags
    # to add. CMAKE_BUILD_TYPE contains "Debug" or "RelWithDebugInfo" or "Release" etc, however, so first
    # we need to uppercase CMAKE_BUILD_TYPE, then dereference ${CMAKE_CXX_FLAGS_${UC_BUILD_TYPE}}.
    #
    # I'm unaware of a less roundabout method of getting the *actual* build flags for a target.
    string(TOUPPER "${CMAKE_BUILD_TYPE}" UC_BUILD_TYPE)

    # Lllloook at you, haaacker. A pa-pa-pathetic creature of meat and bone.
    #
    # It appears that we can *only* get the COMPILE_DEFINITIONS as a generator expression.
    # This wouldn't be so bad if http://www.kwwidgets.org/Bug/view.php?id=14353#c33712 didn't mean
    # that you can't use generator expressions in custom commands.
    #
    # So!
    # What we *can* do is generate a file with the contents of the generator expressions,
    # then use gcc's @file mechanism...
    set(FLAGS_FILE "${CMAKE_CURRENT_BINARY_DIR}/${HEADER_NAME}.compileflags")

    file(
      GENERATE
      OUTPUT "${FLAGS_FILE}"
      CONTENT "
        ${CMAKE_CXX_FLAGS}
        ${CMAKE_CXX_FLAGS_${UC_BUILD_TYPE}}
        ${TARGET_INCLUDE_DIRECTORIES_STRING}
        ${TARGET_COMPILE_DEFINITIONS}"
    )

    # HA HA!
    #
    # Of course, that has unescaped all the escaped \"s we have in the compile definitions.
    # gcc treats the contents of @file exactly as if it came from the command line, so we need to
    # re-escape them.
    add_custom_command(
      OUTPUT ${FLAGS_FILE}.processed
      DEPENDS ${FLAGS_FILE}
      # ESCAPE ALL THE THINGS!
      COMMAND sh -c "sed s_\\\"_\\\\\\\\\\\"_g ${FLAGS_FILE} > ${FLAGS_FILE}.processed"
      VERBATIM
    )
    add_custom_command(
      OUTPUT ${TARGET}_precompiled.hpp.gch
      DEPENDS ${HEADER} ${FLAGS_FILE}.processed
      COMMAND ${CMAKE_CXX_COMPILER} @${FLAGS_FILE}.processed -x c++-header -c ${HEADER} -o ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}_precompiled.hpp.gch
    )

    set_property(TARGET ${TARGET} APPEND_STRING PROPERTY COMPILE_FLAGS " -include ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}_precompiled.hpp -Winvalid-pch ")

    add_custom_target(${TARGET}_pch DEPENDS ${TARGET}_precompiled.hpp.gch)
    add_dependencies(${TARGET} ${TARGET}_pch)
  endif()
endfunction()

function (mir_add_wrapped_executable TARGET)
  set(REAL_EXECUTABLE ${TARGET}.bin)

  list(GET ARGN 0 modifier)
  if ("${modifier}" STREQUAL "NOINSTALL")
    list(REMOVE_AT ARGN 0)
  else()
    install(PROGRAMS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${REAL_EXECUTABLE}
      DESTINATION ${CMAKE_INSTALL_BINDIR}
      RENAME ${TARGET}
    )
  endif()

  add_executable(${TARGET} ${ARGN})
  set_target_properties(${TARGET} PROPERTIES
    OUTPUT_NAME ${REAL_EXECUTABLE}
    SKIP_BUILD_RPATH TRUE
  )

  add_custom_target(${TARGET}-wrapped
    ln -fs wrapper ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${TARGET}
  )
  add_dependencies(${TARGET} ${TARGET}-wrapped)
endfunction()

function (mir_add_test)
  # Add test normally
  add_test(${ARGN})

  # Add to to discovery for parallel test running
  set(one_value_args "NAME" WORKING_DIRECTORY)
  set(multi_value_args "COMMAND")
  cmake_parse_arguments(MAT "" "${one_value_args}" "${multi_value_args}" ${ARGN})

  foreach (cmd ${MAT_COMMAND})
    set(cmdstr "${cmdstr} \\\"${cmd}\\\"")
  endforeach()

  file(APPEND ${CMAKE_BINARY_DIR}/discover_all_tests.sh
    "echo \"add_test(${MAT_NAME} ${cmdstr})\"\n")

  if (MAT_WORKING_DIRECTORY)
    file(APPEND ${CMAKE_BINARY_DIR}/discover_all_tests.sh
      "echo \"set_tests_properties(${MAT_NAME} PROPERTIES WORKING_DIRECTORY \\\"${MAT_WORKING_DIRECTORY}\\\")\"\n")
  endif()
endfunction()

function (mir_check_no_unreleased_symbols TARGET DEPENDENT_TARGET)
  set(TARGET_NAME "Checking-${TARGET}-for-unreleased-symbols")
  add_custom_target(
    ${TARGET_NAME}
    # Some sort of documentation for this monstrosity:
    #
    # Objdump format is:
    #
    # $ADDRESS $FLAGS $SECTION $SIZE $SYMVER $NAME
    #
    # Cut first five lines which don't contain any symbol information
    # Cut any lines for undefined symbols
    # Cut adress and flags which are fixed width
    # Convert tabs to spaces
    # Whitespace between fields is collapsed to one character
    # Extract the symbol version (3rd field)
    # Check for unreleased symbols - grep will set exit code to 0 if any are found
    # finally invert the exit code
    COMMAND /bin/sh -c "objdump -T $<TARGET_FILE:${TARGET}> | tail -n+5 | grep -v 'UND' | cut -c 26- | sed $'s/\t/ /g' | tr -s ' ' | cut -d ' ' -f 3 | grep -q unreleased ; test $? -gt 0"
    VERBATIM
  )
  add_dependencies(${DEPENDENT_TARGET} ${TARGET_NAME})
endfunction()
