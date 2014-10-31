cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")

set(LIB_SOURCES
    BamEntry.cpp
    BamEntry.hpp
    BamFilter.hpp
    BamHeader.cpp
    BamHeader.hpp
    BamReader.cpp
    BamReader.hpp
    BamWindow.cpp
    BamWindow.hpp
    ColumnAssigner.cpp
    ColumnAssigner.hpp
    MurmurHash2.hpp
    Options.cpp
    Options.hpp
    RowAssigner.cpp
    RowAssigner.hpp
    StreamJoin.hpp
    TableBuilder.hpp
    WarningCollector.cpp
    WarningCollector.hpp
)

add_library(bwin ${LIB_SOURCES})
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

include_directories(.)
add_subdirectory(test)

set(EXECUTABLE_NAME bam-window)
add_executable(${EXECUTABLE_NAME} main.cpp)
target_link_libraries(${EXECUTABLE_NAME} bwin ${Samtools_LIBRARIES} ${Boost_LIBRARIES})
set_target_properties(${EXECUTABLE_NAME} PROPERTIES OUTPUT_NAME ${EXECUTABLE_NAME}${EXE_VERSION_SUFFIX})
install(TARGETS ${EXECUTABLE_NAME} DESTINATION bin/)
