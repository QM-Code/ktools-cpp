cmake_minimum_required(VERSION 3.20)

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/cmake/00_toolchain.cmake")
    include("${CMAKE_CURRENT_LIST_DIR}/cmake/00_toolchain.cmake")
endif()

project(ki18n VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(KI18N_BUILD_STATIC "Build ki18n static library target" ON)
option(KI18N_BUILD_SHARED "Build ki18n shared library target" ON)

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/cmake/10_dependencies.cmake")
    include("${CMAKE_CURRENT_LIST_DIR}/cmake/10_dependencies.cmake")
endif()
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/cmake/20_targets.cmake")
    include("${CMAKE_CURRENT_LIST_DIR}/cmake/20_targets.cmake")
endif()

include(CTest)
if(BUILD_TESTING AND EXISTS "${CMAKE_CURRENT_LIST_DIR}/cmake/tests/CMakeLists.txt")
    add_subdirectory(cmake/tests)
endif()

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/cmake/50_install_export.cmake")
    include("${CMAKE_CURRENT_LIST_DIR}/cmake/50_install_export.cmake")
endif()
