# epic-math, fixed-point math utility library for the 8-bit PIC HAL.
#
#   cmake -B build && cmake --build build
#   ctest --testdir build --output-on-failure
#   ./build/test_mul
#
# This is the host reference build: src/host/ (portable-C leaf primitives)
# plus src/common/ (the portable-C derived routines every backend shares),
# built as a host static library and exercised by the Tier-1 unit tests
# under tests/. The family-specific inline-asm backends (src/pic16/,
# src/pic18/) are built by the XC8 Makefiles under mcu/, not by CMake --
# that asm targets XC8's PIC16/PIC18 instruction sets, not the host.
#
# Unlike epic-taskmgr's CMake, this build does NOT add_subdirectory() a HAL.
# epic-math is a stateless computation library: the public header needs only
# <stdint.h>/<stdbool.h>, and the host reference backend needs no peripheral
# model. The family dimension (PIC16 vs PIC18, and the HAL each on-target
# self-test links) lives entirely in the XC8 Makefile targets, which is the
# only place a HAL is actually required. Keeping the host build HAL-free is
# the plan-consistent reading of "Build src/host/ + src/common/ as a host
# static lib and test it directly" (Tier 1); see docs/ARCHITECTURE.md.
#
# The host/target split is done at build time, not with #ifdef, exactly as
# in the HAL: CMake links src/host/, the XC8 Makefiles link src/pic16/ or
# src/pic18/. No math source file has a preprocessor branch for either
# build or either family.

cmake_minimum_required(VERSION 3.16)
project(epic_math C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# The host reference static library: portable-C leaf primitives (src/host/)
# plus the shared derived routines (src/common/, linked by every backend).
add_library(pic_math STATIC
    src/host/pic_math_mul.c
    src/host/pic_math_div.c
    src/host/pic_math_addsub.c
    src/host/pic_math_bcd.c
    src/common/pic_math_sqrt.c
    src/common/pic_math_numeric.c
    src/common/pic_math_rand.c)
target_include_directories(pic_math PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)

# ─── Tier 1: host unit tests (automated, the bulk of coverage) ────
# Each test is a standalone main() that returns 0 on pass, 1 on fail, and
# is registered with CTest. Tests link the host reference library and use
# it as the oracle; the per-family asm backends are validated separately
# (Tier 2: optional gpsim for PIC16; Tier 3: on-target USART self-test).
enable_testing()

# Phase 0 smoke: proves the host build links and the header compiles.
add_executable(test_smoke tests/test_smoke.c)
target_link_libraries(test_smoke PRIVATE pic_math)
add_test(NAME smoke COMMAND test_smoke)

# Phase 1: core arithmetic. Each test is a standalone main() returning 0 on
# pass; pic_math_test.h provides the CHECK macro and the reproducible LCG.
function(pic_math_add_test name src)
    add_executable(${name} ${src})
    target_link_libraries(${name} PRIVATE pic_math)
    # test_sqrt uses double sqrt() as the oracle -> needs libm on Unix hosts.
    target_link_libraries(${name} PRIVATE m)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

pic_math_add_test(test_mul    tests/test_mul.c)
pic_math_add_test(test_div    tests/test_div.c)
pic_math_add_test(test_addsub tests/test_addsub.c)
pic_math_add_test(test_bcd    tests/test_bcd.c)
pic_math_add_test(test_sqrt   tests/test_sqrt.c)
pic_math_add_test(test_numeric tests/test_numeric.c)
pic_math_add_test(test_rand   tests/test_rand.c)
pic_math_add_test(test_properties tests/test_properties.c)

# ─── golden-vector generator (host tool) ─────────────────────────
# Builds against the host oracle and emits tests/golden_vectors.h: the
# curated input/expected-output table shared by all three test tiers. Not a
# test itself -- run it to (re)generate the checked-in header:
#   ./build/gen_golden_vectors > tests/golden_vectors.h
add_executable(gen_golden_vectors tools/gen_golden_vectors.c)
target_link_libraries(gen_golden_vectors PRIVATE pic_math)
