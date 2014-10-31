cmake_minimum_required(VERSION 2.8)

function(get_git_version_info)
    find_package(Git)
    if (GIT_FOUND)
        execute_process(
            COMMAND ${GIT_EXECUTABLE} describe --long --dirty
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE VERSION_OUT
            ERROR_VARIABLE VERSION_ERR
            OUTPUT_STRIP_TRAILING_WHITESPACE
            )

        if (NOT ${VERSION_ERR})
            message(FATAL_ERROR "Failed to get git revision, abort: ${VERSION_ERR}!")
        endif()

        set(GIT_VERSION_INFO ${VERSION_OUT} PARENT_SCOPE)
        message(STATUS "Git version info: ${VERSION_OUT}")
    else()
        set(GIT_VERSION_INFO "unknown" PARENT_SCOPE)
    endif()

endfunction()
