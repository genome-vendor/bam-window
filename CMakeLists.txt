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
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX11_FLAGS} -Wall -pedantic -fno-deduce-init-list")


##############################################################################
#### Configurable dependencies

###
## htslib
##
## samtools >= 1.0 requires htslib
## The placeholder code is here to enable building that first and making the
## samtools ExternalProject depend on it. There is a loss of # functionality
## between samtools 0.1.19 and 1.1, so we'll stick with the former for now.
#set(HTSLIB_GIT_URL http://github.com/samtools/htslib.git
    #CACHE string "Git url of htslib repository")

#set(HTSLIB_GIT_TAG 1.1
    #CACHE string "Git commit-ish (tag, branch, ...) of htslib to use")

#set(HTSLIB_ROOT ${CMAKE_BINARY_DIR}/vendor/htslib)
#set(HTSLIB_SOURCE_DIR ${CMAKE_BINARY_DIR}/vendor/htslib-src)
#ExternalProject_Add(
    #htslib
    #GIT_REPOSITORY ${HTSLIB_GIT_URL}
    #GIT_TAG ${HTSLIB_GIT_TAG}
    #BUILD_IN_SOURCE 1
    #SOURCE_DIR ${HTSLIB_SOURCE_DIR}
    #CONFIGURE_COMMAND ""
    #BUILD_COMMAND make
    #INSTALL_COMMAND make prefix=${HTSLIB_ROOT} install
    #BUILD_COMMAND ""
    #INSTALL_COMMAND ""
#)
#include_directories(${HTSLIB_ROOT}/include)
#set(Htslib_LIBRARIES ${HTSLIB_ROOT}/lib/libhts.a)
#set(SAMTOOLS_DEPS htslib)


##
# samtools
#
set(SAMTOOLS_GIT_URL http://github.com/samtools/samtools.git
    CACHE string "Git url of samtools repository")

set(SAMTOOLS_GIT_TAG 0.1.19
    CACHE string "Git commit-ish (tag, branch, ...) of samtools to use")
set(SAMTOOLS_ROOT ${CMAKE_BINARY_DIR}/vendor/samtools)

ExternalProject_Add(
    samtools
    DEPENDS ${SAMTOOLS_DEPS}
    GIT_REPOSITORY ${SAMTOOLS_GIT_URL}
    GIT_TAG ${SAMTOOLS_GIT_TAG}
    SOURCE_DIR ${SAMTOOLS_ROOT}
    BUILD_IN_SOURCE 1
    CONFIGURE_COMMAND ""
    BUILD_COMMAND make HTSDIR=${HTSLIB_SOURCE_DIR} lib
    INSTALL_COMMAND ""
)

include_directories(${SAMTOOLS_ROOT})
set(Samtools_LIBRARIES
    ${SAMTOOLS_ROOT}/libbam.a
    ${Htslib_LIBRARIES}
    z m)
##############################################################################



##############################################################################
#### Fixed dependencies

# Everybody loves threads
find_package(Threads REQUIRED)
link_libraries(${CMAKE_THREAD_LIBS_INIT})

find_package(Boost 1.51 COMPONENTS program_options REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

set(GTEST_LIB_DIR ${CMAKE_BINARY_DIR}/vendor/gtest)
ExternalProject_Add(
    google_test
    URL ${CMAKE_CURRENT_SOURCE_DIR}/vendor/gtest-1.7.0
    INSTALL_COMMAND ""
    )
ExternalProject_Get_Property(google_test source_dir)
ExternalProject_Get_Property(google_test binary_dir)
set(BDIR ${binary_dir})
include_directories(${source_dir}/include)

set(GTEST_LIBRARY
    ${BDIR}/${CMAKE_FIND_LIBRARY_PREFIXES}gtest${CMAKE_STATIC_LIBRARY_SUFFIX}
)

set(GTEST_MAIN_LIBRARY
    ${BDIR}/${CMAKE_FIND_LIBRARY_PREFIXES}gtest_main${CMAKE_STATIC_LIBRARY_SUFFIX}
)

set(GTEST_BOTH_LIBRARIES ${GTEST_MAIN_LIBRARY} ${GTEST_LIBRARY})

enable_testing(true)
##############################################################################

add_subdirectory(src)
