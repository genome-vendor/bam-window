cmake_minimum_required(VERSION 2.8)

include_directories(include)

set(PROGRAM_OPTIONS_SOURCES
    libs/program_options/src/cmdline.cpp
    libs/program_options/src/config_file.cpp
    libs/program_options/src/options_description.cpp
    libs/program_options/src/parsers.cpp
    libs/program_options/src/variables_map.cpp
    libs/program_options/src/value_semantic.cpp
    libs/program_options/src/positional_options.cpp
    libs/program_options/src/utf8_codecvt_facet.cpp
    libs/program_options/src/convert.cpp
    libs/program_options/src/winmain.cpp
    libs/program_options/src/split.cpp
)

add_library(boostbits ${PROGRAM_OPTIONS_SOURCES})
