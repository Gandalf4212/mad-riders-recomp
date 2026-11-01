# mad_riders - ReXGlue Recompiled Project
#
# SDK boilerplate lives in generated/rexglue.cmake.

cmake_minimum_required(VERSION 3.25)
project(mad_riders LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(generated/rexglue.cmake)

# Sources
set(MADRIDERS_SOURCES
    src/main.cpp
)

if(WIN32)
    add_executable(mad_riders WIN32 ${MADRIDERS_SOURCES})
else()
    add_executable(mad_riders ${MADRIDERS_SOURCES})
endif()

rexglue_setup_target(mad_riders)
