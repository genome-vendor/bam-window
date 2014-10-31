cmake_minimum_required(VERSION 2.8)

project(bam_window)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
include(PlatformChecks)
include(ExternalProject)

find_cxx11_flags(CXX11_FLAGS CXX11_FOUND)
if(NOT CXX11_FOUND)
    message(FATAL_ERROR
        "Failed to find C++11 support with compiler:
         ${CMAKE_CXX_COMPILER} (${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}).")
endif()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX11_FLAGS} -Wall -pedantic")

if(${CMAKE_CXX_COMPILER_ID} MATCHES "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-deduce-init-list")
endif()

##############################################################################
#### Fixed dependencies

# Everybody loves threads
find_package(Threads REQUIRED)
link_libraries(${CMAKE_THREAD_LIBS_INIT})

find_package(ZLIB REQUIRED)
include_directories(${ZLIB_INCLUDE_DIRS})

find_package(Boost 1.51 COMPONENTS program_options REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})


##############################################################################

# Build 3rd party libs
add_subdirectory(vendor/samtools)
include_directories(vendor/samtools)
set(Samtools_LIBRARIES bam ${ZLIB_LIBRARIES} m)

add_subdirectory(vendor/gtest-1.7.0)
include_directories(vendor/gtest-1.7.0/include)
set(GTEST_BOTH_LIBRARIES gtest gtest_main)

enable_testing(true)
add_subdirectory(src)
