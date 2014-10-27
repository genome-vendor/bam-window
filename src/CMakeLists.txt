cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")

set(LIB_SOURCES
    BamEntry.cpp
    BamEntry.hpp
    BamHeader.cpp
    BamHeader.hpp
    BamReader.cpp
    BamReader.hpp
    BamWindow.cpp
    BamWindow.hpp
    ColumnAssigner.cpp
    ColumnAssigner.hpp
    Options.cpp
    Options.hpp
    RowAssigner.cpp
    RowAssigner.hpp
    TableBuilder.hpp
)

add_library(bwin ${LIB_SOURCES})
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

add_executable(bam-window main.cpp)
target_link_libraries(bam-window bwin ${Samtools_LIBRARIES} ${Boost_LIBRARIES})

include_directories(.)
add_subdirectory(test)