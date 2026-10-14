# lib/pid, vendor-agnostic fixed-point PID controller.

#   cmake -B build && cmake --build build
#   ctest --test-dir build --output-on-failure
#   ./build/test_pid

# The pid core (src/pid.c) is a fixed-point arithmetic engine on plain
# data. It depends only on lib/math (for the 16x16->32 signed multiply
# epic_math_mul_s16 the Q8.8 math needs). lib/math's host build is itself
# HAL-free (see lib/math/CMakeLists.txt for the reasoning: a stateless
# computation library needs no peripheral model), so this CMake build also
# does NOT add_subdirectory() a HAL -- the family dimension for lib/pid
# lives entirely in the XC8 Makefiles under mcu/. The public header pid.h
# is dependency-free (only <stdint.h>/<stdbool.h>); only src/pid.c includes

# epic_math.h.

cmake_minimum_required(VERSION 3.16)
project(epic_pid C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Root anchor: walk up to the shared helpers, so moving this directory
# changes no path below. Cross-module paths hang off EPIC_ROOT.
if(NOT DEFINED EPIC_ROOT)
    set(_EPIC_PROBE "${CMAKE_CURRENT_SOURCE_DIR}")
    while(NOT EXISTS "${_EPIC_PROBE}/common/cmake/epic_family.cmake")
        get_filename_component(_EPIC_PARENT "${_EPIC_PROBE}/.." ABSOLUTE)
        if("${_EPIC_PARENT}" STREQUAL "${_EPIC_PROBE}")
            message(FATAL_ERROR "EPIC_ROOT not found above ${CMAKE_CURRENT_SOURCE_DIR}")
        endif()
        set(_EPIC_PROBE "${_EPIC_PARENT}")
    endwhile()
    set(EPIC_ROOT "${_EPIC_PROBE}")
    unset(_EPIC_PROBE)
    unset(_EPIC_PARENT)
endif()

# Pull in lib/math (the host reference build, which is HAL-free).
set(MATH_DIR ${EPIC_ROOT}/lib/math
    CACHE PATH "Path to the lib/math source tree")
if(NOT TARGET epic_math)
    add_subdirectory(${MATH_DIR} ${CMAKE_CURRENT_BINARY_DIR}/math)
endif()

add_library(pid STATIC src/pid.c)
target_include_directories(pid PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(pid PUBLIC epic_math)

# ---- tests: exercise the exact code that ships on-target ----
enable_testing()
add_executable(test_pid tests/test_pid.c)
target_link_libraries(test_pid PRIVATE pid)
add_test(NAME pid_tests COMMAND test_pid)
