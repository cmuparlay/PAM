cmake_minimum_required(VERSION 3.13)
project(PAM VERSION 1.0 DESCRIPTION "Parallel Augmented Maps" LANGUAGES CXX)

# Set a default build type
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type (Debug Release RelWithDebInfo MinSizeRel)" FORCE)
  message(STATUS "No build type specified. Defaulted to RELEASE.")
  message(STATUS "To specify a build type, add -DCMAKE_BUILD_TYPE=<DEBUG/RELEASE/RELWITHDEBINFO/MINSIZEREL>")
endif(NOT CMAKE_BUILD_TYPE)

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

# -------------------------------------------------------------------
#                       Library definition

add_library(pam INTERFACE)
set(PAM_INCLUDE_DIR "${PAM_SOURCE_DIR}/include")
target_include_directories(pam INTERFACE ${PAM_INCLUDE_DIR})
target_compile_features(pam INTERFACE cxx_std_17)
target_compile_options(pam INTERFACE -mcx16 -march=native)

# Find threading library
find_package(Threads REQUIRED)

# Link against ParlayLib
#
# If the option -DDOWNLOAD_PARLAY is set to true, then we will pull a fresh
# copy of Parlay and use that. Otherwise, we will look for an installed version
# of Parlay on the system.
#
option(DOWNLOAD_PARLAY "Download and use a local copy of Parlay rather than looking for an installed version")
if(DOWNLOAD_PARLAY)
  message(STATUS "Downloading a local copy of ParlayLib")
  # Download and configure GoogleTest
  include(FetchContent)
  FetchContent_Declare(parlaylib
    GIT_REPOSITORY  https://github.com/cmuparlay/parlaylib.git
    GIT_TAG         master
  )
  FetchContent_GetProperties(parlaylib)
  if(NOT parlaylib_POPULATED)
    FetchContent_Populate(parlaylib)  
    add_subdirectory(${parlaylib_SOURCE_DIR} EXCLUDE_FROM_ALL)
  endif()
  target_link_libraries(pam INTERFACE parlay)
else()
  find_package(Parlay REQUIRED)
  if(Parlay_FOUND)
    message(STATUS "Parlay configuration found at " ${Parlay_DIR})
    target_link_libraries(pam INTERFACE Parlay::parlay)
  else()
    message(FATAL_ERROR "ParlayLib is required but could not be found. Either install it, or set the option -DDOWNLOAD_PARLAY=True to download a local copy of Parlay to use")
  endif()
endif()

# Link against jemalloc
find_library(JEMALLOC_LIB jemalloc)
if(NOT JEMALLOC_LIB)
  message(FATAL_ERROR "Could not find jemalloc.")
else()
  message("-- Found jemalloc: ${JEMALLOC_LIB}")
  target_link_libraries(pam INTERFACE optimized ${JEMALLOC_LIB})    
endif()


# -------------------------------------------------------------------
#                         Unit tests

enable_testing()
add_subdirectory(tests)

# -------------------------------------------------------------------
#                       Benchmarks

add_subdirectory(benchmark)

# -------------------------------------------------------------------
#                       Examples

add_subdirectory(examples)
