project(FeatureShowcase)

include_directories(${CUKE_INCLUDE_DIRS})

if(GTEST_FOUND)
    include_directories(${GTEST_INCLUDE_DIRS})

    add_executable(TagSteps tag/TagSteps)
    target_link_libraries(TagSteps ${GTEST_LIBRARIES} ${CUKE_LIBRARIES})

    add_executable(TableSteps table/TableSteps)
    target_link_libraries(TableSteps ${GTEST_LIBRARIES} ${CUKE_LIBRARIES})
endif()

