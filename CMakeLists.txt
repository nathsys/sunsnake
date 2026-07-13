cmake_minimum_required(VERSION 3.21)
project(sunsnake CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type: Debug, Release, PGOGenerate, PGOUse" FORCE)
endif()

option(SUNSNAKE_NATIVE_ARCH "Optimize for this machine's CPU (-march=native)" ON)
option(SUNSNAKE_BUILD_TESTS "Enable the internal self-test suite" OFF)
option(SUNSNAKE_GENERATE_MAGICS "Build a helper that (re)generates magic bitboard numbers" OFF)

add_executable(sunsnake main.cpp)

set(SUNSNAKE_OPT_FLAGS -Ofast -funroll-loops)
if(SUNSNAKE_NATIVE_ARCH)
    list(APPEND SUNSNAKE_OPT_FLAGS -march=native)
endif()

# PGOGenerate/PGOUse must be run from the same build directory so the
# profile data lines up with the object files that produced/consume it.
set(SUNSNAKE_PGO_DIR "${CMAKE_BINARY_DIR}/pgo-data")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(sunsnake PRIVATE -O0 -g -fno-omit-frame-pointer)
elseif(CMAKE_BUILD_TYPE STREQUAL "PGOGenerate")
    target_compile_options(sunsnake PRIVATE ${SUNSNAKE_OPT_FLAGS} -fprofile-generate=${SUNSNAKE_PGO_DIR})
    target_link_options(sunsnake PRIVATE -fprofile-generate=${SUNSNAKE_PGO_DIR})
elseif(CMAKE_BUILD_TYPE STREQUAL "PGOUse")
    target_compile_options(sunsnake PRIVATE ${SUNSNAKE_OPT_FLAGS} -fprofile-use=${SUNSNAKE_PGO_DIR} -fprofile-correction)
    target_link_options(sunsnake PRIVATE -fprofile-use=${SUNSNAKE_PGO_DIR})
else()
    target_compile_options(sunsnake PRIVATE ${SUNSNAKE_OPT_FLAGS})
endif()

if(SUNSNAKE_BUILD_TESTS)
    target_compile_definitions(sunsnake PRIVATE run_tests)
endif()

if(SUNSNAKE_GENERATE_MAGICS)
    target_compile_definitions(sunsnake PRIVATE generate_magics)
endif()
