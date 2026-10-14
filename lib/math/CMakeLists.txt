# lib/math, fixed-point math utility library for the 8-bit PIC HAL.
#
#   cmake -B build && cmake --build build
#   ctest --testdir build --output-on-failure
#   ./build/test_mul
#
# Host reference build: src/host/ plus src/common/ as a static lib
# (docs/ARCHITECTURE.md); asm backends build under mcu/, no HAL here.

cmake_minimum_required(VERSION 3.16)
project(epic_math C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# The host reference static library: portable-C leaf primitives (src/host/)
# plus the shared derived routines (src/common/, linked by every backend).
add_library(epic_math STATIC
    src/host/epic_math_mul.c
    src/host/epic_math_div.c
    src/host/epic_math_addsub.c
    src/host/epic_math_bcd.c
    src/common/epic_math_sqrt.c
    src/common/epic_math_numeric.c
    src/common/epic_math_rand.c)
target_include_directories(epic_math PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)

# ─── Tier 1: host unit tests (automated, the bulk of coverage) ────
# Each test is a standalone main() that returns 0 on pass, 1 on fail, and
# is registered with CTest. Tests link the host reference library and use
# it as the oracle; the per-family asm backends are validated separately
# (Tier 2: optional gpsim for PIC16; Tier 3: on-target USART self-test).
enable_testing()

# Phase 0 smoke: proves the host build links and the header compiles.
add_executable(test_smoke tests/test_smoke.c)
target_link_libraries(test_smoke PRIVATE epic_math)
add_test(NAME smoke COMMAND test_smoke)

# Phase 1: core arithmetic. Each test is a standalone main() returning 0 on
# pass; epic_math_test.h provides the CHECK macro and the reproducible LCG.
function(epic_math_add_test name src)
    add_executable(${name} ${src})
    target_link_libraries(${name} PRIVATE epic_math)
    # test_sqrt uses double sqrt() as the oracle -> needs libm on Unix hosts.
    target_link_libraries(${name} PRIVATE m)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

epic_math_add_test(test_mul    tests/test_mul.c)
epic_math_add_test(test_div    tests/test_div.c)
epic_math_add_test(test_addsub tests/test_addsub.c)
epic_math_add_test(test_bcd    tests/test_bcd.c)
epic_math_add_test(test_sqrt   tests/test_sqrt.c)
epic_math_add_test(test_numeric tests/test_numeric.c)
epic_math_add_test(test_rand   tests/test_rand.c)
epic_math_add_test(test_properties tests/test_properties.c)

# ─── golden-vector generator (host tool) ─────────────────────────
# Builds against the host oracle and emits tests/golden_vectors.h: the
# curated input/expected-output table shared by all three test tiers. Not a
# test itself -- run it to (re)generate the checked-in header:
#   ./build/gen_golden_vectors > tests/golden_vectors.h
add_executable(gen_golden_vectors tools/gen_golden_vectors.c)
target_link_libraries(gen_golden_vectors PRIVATE epic_math)
