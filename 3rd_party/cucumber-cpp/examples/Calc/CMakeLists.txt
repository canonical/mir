project(Calc)

include_directories(${CUKE_INCLUDE_DIRS} Calc)

add_library(Calc Calc/Calculator)

if(GTEST_FOUND)
    include_directories(${GTEST_INCLUDE_DIRS})
    add_executable(GTestCalculatorSteps CalcFeatures/GTestCalculatorSteps)
    target_link_libraries(GTestCalculatorSteps Calc ${GTEST_LIBRARIES} ${CUKE_LIBRARIES})
endif()

if(CPPSPEC_FOUND)
    include_directories(${CPPSPEC_INCLUDE_DIRS})
    add_executable(CppSpecCalculatorSteps CalcFeatures/CppSpecCalculatorSteps)
    target_link_libraries(CppSpecCalculatorSteps Calc ${CPPSPEC_LIBRARY} ${CUKE_LIBRARIES})
endif()

if(Boost_UNIT_TEST_FRAMEWORK_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    add_executable(BoostCalculatorSteps CalcFeatures/BoostCalculatorSteps)
    target_link_libraries(BoostCalculatorSteps Calc ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} ${CUKE_LIBRARIES})
endif()
