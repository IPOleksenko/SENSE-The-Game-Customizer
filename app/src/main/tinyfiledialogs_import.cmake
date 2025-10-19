include(FetchContent)

FetchContent_Declare(
    tinyfiledialogs_upstream
    GIT_REPOSITORY https://github.com/native-toolkit/tinyfiledialogs.git
    GIT_TAG master
    SOURCE_DIR ${PROJECT_ROOT}/external/tinyfiledialogs
)

FetchContent_MakeAvailable(tinyfiledialogs_upstream)

set(_tiny_src_c "${tinyfiledialogs_upstream_SOURCE_DIR}/tinyfiledialogs.c")
set(_tiny_src_h "${tinyfiledialogs_upstream_SOURCE_DIR}/tinyfiledialogs.h")
set(_tiny_cmake "${tinyfiledialogs_upstream_SOURCE_DIR}/CMakeLists.txt")

if(NOT EXISTS ${_tiny_src_c} OR NOT EXISTS ${_tiny_src_h})
    message(FATAL_ERROR "tinyfiledialogs source files not found in ${tinyfiledialogs_upstream_SOURCE_DIR}")
endif()

if(NOT EXISTS ${_tiny_cmake})
    file(WRITE ${_tiny_cmake} "
cmake_minimum_required(VERSION 3.21)

project(tinyfiledialogs LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_library(tinyfiledialogs STATIC
    \${CMAKE_CURRENT_LIST_DIR}/tinyfiledialogs.c
)

target_include_directories(tinyfiledialogs PUBLIC
    \${CMAKE_CURRENT_LIST_DIR}
)
")
endif()

if(NOT TARGET tinyfiledialogs)
    add_subdirectory(${tinyfiledialogs_upstream_SOURCE_DIR} ${CMAKE_BINARY_DIR}/tinyfiledialogs_build)
endif()
