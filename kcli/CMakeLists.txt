cmake_minimum_required(VERSION 3.20)

include("${CMAKE_CURRENT_LIST_DIR}/cmake/00_toolchain.cmake")
project(kcli VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(KCLI_BUILD_STATIC "Build kcli static library target" ON)
option(KCLI_BUILD_SHARED "Build kcli shared library target" ON)

include("${CMAKE_CURRENT_LIST_DIR}/cmake/10_dependencies.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/cmake/20_targets.cmake")
include(CTest)
if(BUILD_TESTING AND EXISTS "${CMAKE_CURRENT_LIST_DIR}/cmake/tests/CMakeLists.txt")
    add_subdirectory(cmake/tests)
endif()
include("${CMAKE_CURRENT_LIST_DIR}/cmake/50_install_export.cmake")
