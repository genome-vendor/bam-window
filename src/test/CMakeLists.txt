set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/test-bin)

set(TEST_SOURCES
    TestColumnAssigner.cpp
    TestRowAssigner.cpp
    TestTableBuilder.cpp
)

add_executable(TestBamWindow ${TEST_SOURCES})

target_link_libraries(TestBamWindow bwin ${Samtools_LIBRARIES} ${GTEST_BOTH_LIBRARIES} pthread)
add_test(NAME TestBamWindow COMMAND TestBamWindow)

set_tests_properties(TestBamWindow PROPERTIES LABELS unit)
