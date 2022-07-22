# -------------------------------------------------------------------
#                       Build system for Packaide
# -------------------------------------------------------------------
# Requirements:
#   - CMake version 3.0+
#   - A C++17 compiler
#   - CGAL
#   - Python 3.6+
# -------------------------------------------------------------------

cmake_minimum_required(VERSION 3.0)
project(Packaide VERSION 2.0
             DESCRIPTION "Fast and robust 2D nesting"
             LANGUAGES CXX)

# Default to RELEASE build
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING
      "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
endif(NOT CMAKE_BUILD_TYPE)

# Allow CMAKE_PREFIX_PATH to be added to by an environment variable
list(APPEND CMAKE_PREFIX_PATH $ENV{CMAKE_PREFIX_PATH})

message(STATUS "--------------- General configuration -------------")
message(STATUS "CMake Generator:                ${CMAKE_GENERATOR}")
message(STATUS "Compiler:                       ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Build type:                     ${CMAKE_BUILD_TYPE}")
message(STATUS "CMAKE_CXX_FLAGS:                ${CMAKE_CXX_FLAGS}")
message(STATUS "CMAKE_CXX_FLAGS_DEBUG:          ${CMAKE_CXX_FLAGS_DEBUG}")
message(STATUS "CMAKE_CXX_FLAGS_RELEASE:        ${CMAKE_CXX_FLAGS_RELEASE}")
message(STATUS "CMAKE_CXX_FLAGS_RELWITHDEBINFO: ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
message(STATUS "CMAKE_EXE_LINKER_FLAGS          ${CMAKE_EXE_LINKER_FLAGS}")
message(STATUS "CMAKE_LINKER_FLAGS_DEBUG        ${CMAKE_LINKER_FLAGS_DEBUG}")
message(STATUS "CMAKE_SHARED_LINKER_FLAGS       ${CMAKE_SHARED_LINKER_FLAGS}")
message(STATUS "CMAKE_INSTALL_PREFIX:           ${CMAKE_INSTALL_PREFIX}" )
message(STATUS "CMAKE_PREFIX_PATH:              ${CMAKE_PREFIX_PATH}" )
message(STATUS "CMAKE_TOOLCHAIN_FILE:           ${CMAKE_TOOLCHAIN_FILE}" )
message(STATUS "---------------------------------------------------")

# -------------------------------------------------------------------
#                       Library definition

add_library(PackaideLib INTERFACE)
set(LIB_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include")
target_include_directories(PackaideLib INTERFACE ${LIB_INCLUDE_DIR})
target_compile_features(PackaideLib INTERFACE cxx_std_17)

# -------------------------------------------------------------------
#                         Link with CGAL

set(CGAL_DO_NOT_WARN_ABOUT_CMAKE_BUILD_TYPE TRUE)
find_package(CGAL)
target_link_libraries(PackaideLib INTERFACE CGAL::CGAL)

# -------------------------------------------------------------------
#                         Python bindings

# Download Pybind11
include(FetchContent)
FetchContent_Declare(pybind
  GIT_REPOSITORY  https://github.com/pybind/pybind11
  GIT_TAG         master
)
FetchContent_GetProperties(pybind)
if(NOT pybind_POPULATED)
  message(STATUS "Configuring Pybind11")
  FetchContent_Populate(pybind)
  add_subdirectory(${pybind_SOURCE_DIR} EXCLUDE_FROM_ALL)
endif()

if (PYBIND11_PYTHON_VERSION)
  set(FIND_PYTHON_VERSION_EXACT ${PYBIND11_PYTHON_VERSION} EXACT)
endif()

message(STATUS "PYTHON_EXECUTABLE = " ${PYTHON_EXECUTABLE})

add_subdirectory(src)

# -------------------------------------------------------------------
#                              Tests

enable_testing()
add_subdirectory(test)

# -------------------------------------------------------------------
#                           Benchmarks

add_subdirectory(benchmark)
