# mad_riders - ReXGlue Recompiled Project
#
# SDK boilerplate lives in generated/rexglue.cmake.

cmake_minimum_required(VERSION 3.25)
project(mad_riders LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# The recompiled code in generated/default is committed, so the game builds
# without default.xex (e.g. on GitHub Actions). generated/rexglue.cmake always
# adds a step that re-runs codegen, which needs the xex; when it isn't there we
# include a copy of that file with the step removed and use the committed code.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets/default.xex")
    set(_mr_default_skip OFF)
else()
    set(_mr_default_skip ON)
endif()
option(MADRIDERS_SKIP_CODEGEN "Build from the committed generated code without re-running codegen"
       ${_mr_default_skip})

if(MADRIDERS_SKIP_CODEGEN)
    file(READ "${CMAKE_CURRENT_SOURCE_DIR}/generated/rexglue.cmake" _mr_rexglue)
    string(FIND "${_mr_rexglue}" "add_custom_command(" _mr_start)
    string(FIND "${_mr_rexglue}" "add_custom_target(mad_riders_codegen" _mr_target)
    if(_mr_start EQUAL -1 OR _mr_target EQUAL -1)
        message(FATAL_ERROR "generated/rexglue.cmake changed layout; update the MADRIDERS_SKIP_CODEGEN block")
    endif()
    string(SUBSTRING "${_mr_rexglue}" 0 ${_mr_start} _mr_head)
    string(SUBSTRING "${_mr_rexglue}" ${_mr_target} -1 _mr_tail)
    # Drop the codegen target's dependency on the stamp the removed command made.
    string(REGEX REPLACE "add_custom_target\\(mad_riders_codegen[^)]*\\)"
           "add_custom_target(mad_riders_codegen)" _mr_tail "${_mr_tail}")
    # Paths in the file are relative to the project root.
    string(REPLACE "include(generated/" "include(\${CMAKE_CURRENT_SOURCE_DIR}/generated/"
           _mr_out "${_mr_head}${_mr_tail}")
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/rexglue_prebuilt.cmake" "${_mr_out}")
    message(STATUS "mad_riders: using committed generated code (codegen step skipped)")
    include("${CMAKE_CURRENT_BINARY_DIR}/rexglue_prebuilt.cmake")
else()
    include(generated/rexglue.cmake)
endif()

# Sources
set(MADRIDERS_SOURCES
    src/main.cpp
    src/game_menu.cpp
    src/package_setup.cpp
    src/crash_handler.cpp
    src/debug_dump.cpp
    src/region.cpp src/dlc.cpp src/call_trace.cpp src/video_postproc.cpp src/perf_overlay.cpp src/frame_rate.cpp
)

if(WIN32)
    add_executable(mad_riders WIN32 ${MADRIDERS_SOURCES})
else()
    add_executable(mad_riders ${MADRIDERS_SOURCES})
endif()

rexglue_setup_target(mad_riders GPU_PLUGINS xenos)

if(WIN32)
    target_link_libraries(mad_riders PRIVATE comctl32 comdlg32 gdi32 dbghelp winmm pdh)
endif()

# NativeXRenderer (optional second renderer, off by default; see docs/native-x-renderer.md).
# Needs the SDK as a source tree (REXSDK_DIR) with patches/rexglue-sdk applied.
option(MADRIDERS_NATIVE_X_RENDERER "Build NativeXRenderer in as an optional renderer" OFF)
if(MADRIDERS_NATIVE_X_RENDERER)
    if(NOT REXSDK_DIR)
        message(FATAL_ERROR "MADRIDERS_NATIVE_X_RENDERER needs REXSDK_DIR (the patched SDK source tree)")
    endif()
    # Built from source, the SDK does not pass its ImGui headers on (the menu and overlay include them).
    target_include_directories(mad_riders PRIVATE "${REXSDK_DIR}/thirdparty/imgui")
    add_subdirectory(thirdparty/native_x_renderer)
    target_sources(mad_riders PRIVATE src/native_x_renderer_mad_riders.cpp)
    target_link_libraries(mad_riders PRIVATE native_x_renderer)
    target_compile_definitions(mad_riders PRIVATE MADRIDERS_NATIVE_X_RENDERER=1)
    native_x_renderer_copy_runtime(mad_riders)
endif()

# CI passes the commit so logs say which build produced them.
set(MADRIDERS_BUILD "" CACHE STRING "Build identifier shown in the log")
if(MADRIDERS_BUILD)
    target_compile_definitions(mad_riders PRIVATE MADRIDERS_BUILD="${MADRIDERS_BUILD}")
endif()
