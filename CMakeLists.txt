# -------------------------------------------------------------------
#                       Build system for Packaide
# -------------------------------------------------------------------
# Requirements:
#   - CMake version 3.13+
#   - CGAL
#   - Boost 1.65.1
#   - Boost Python
#   - A C++17 compiler
# -------------------------------------------------------------------

cmake_minimum_required(VERSION 3.13)
project(Packaide VERSION 1.0
             DESCRIPTION "Fast and robust 2D nesting"
             LANGUAGES CXX)

# Default to RELEASE build
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING
      "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
endif(NOT CMAKE_BUILD_TYPE)

# Make sure -fno-omit-frame-pointer is set for profiling
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -fno-omit-frame-pointer")


message(STATUS "--------------- General configuration -------------")
message(STATUS "CMake Generator:                ${CMAKE_GENERATOR}")
message(STATUS "Compiler:                       ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Build type:                     ${CMAKE_BUILD_TYPE}")
message(STATUS "CMAKE_CXX_FLAGS:                ${CMAKE_CXX_FLAGS}")
message(STATUS "CMAKE_CXX_FLAGS_DEBUG:          ${CMAKE_CXX_FLAGS_DEBUG}")
message(STATUS "CMAKE_CXX_FLAGS_RELEASE:        ${CMAKE_CXX_FLAGS_RELEASE}")
message(STATUS "CMAKE_CXX_FLAGS_RELWITHDEBINFO: ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
message(STATUS "CMAKE_EXE_LINKER_FLAGS          ${CMAKE_CXX_LINKER_FLAGS}")
message(STATUS "CMAKE_INSTALL_PREFIX:           ${CMAKE_INSTALL_PREFIX}" )
message(STATUS "---------------------------------------------------")

# -------------------------------------------------------------------
#                       Library definition

add_library(PackaideLib INTERFACE)
set(LIB_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include")
target_include_directories(PackaideLib INTERFACE ${LIB_INCLUDE_DIR})
target_compile_features(PackaideLib INTERFACE cxx_std_17)

# -------------------------------------------------------------------
#                         Link with CGAL

find_package(CGAL)
target_link_libraries(PackaideLib INTERFACE CGAL::CGAL)

# -------------------------------------------------------------------
#                         Python bindings

add_subdirectory(src)

# -------------------------------------------------------------------
#                     Python library installation

add_subdirectory(python)

# -------------------------------------------------------------------
#                              Tests

enable_testing()
add_subdirectory(test)

# -------------------------------------------------------------------
#                           Benchmarks

add_subdirectory(benchmark)
