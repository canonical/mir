set(CUKE_SOURCES
    ContextManager.cpp
    CukeCommands.cpp
    CukeEngine.cpp
    CukeEngineImpl.cpp
    StepManager.cpp
    HookRegistrar.cpp
    Regex.cpp
    Scenario.cpp
    Table.cpp
    Tag.cpp
    connectors/wire/WireServer.cpp
    connectors/wire/WireProtocol.cpp
    connectors/wire/WireProtocolCommands.cpp
    main.cpp
)

include_directories(${GTEST_INCLUDE_DIR})
set(CUKE_SOURCES ${CUKE_SOURCES} drivers/GTestDriver.cpp)

#if(CPPSPEC_FOUND)
#    include_directories(${CPPSPEC_INCLUDE_DIRS})
#    set(CUKE_SOURCES ${CUKE_SOURCES} drivers/CppSpecDriver.cpp)
#endif()

#if(Boost_UNIT_TEST_FRAMEWORK_FOUND)
#    set(CUKE_SOURCES ${CUKE_SOURCES} drivers/BoostDriver.cpp)
#endif()

add_library(cucumber-cpp STATIC ${CUKE_SOURCES})
target_link_libraries(cucumber-cpp ${Boost_LIBRARIES})
  
