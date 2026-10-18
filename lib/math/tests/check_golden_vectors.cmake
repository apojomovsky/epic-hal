# Gate for tests/golden_vectors.h: regenerates the header with the host
# oracle and diffs it against the checked-in copy, so Tier-2/3 inputs
# cannot rot after a hand edit. Expects GEN (generator binary), REF
# (checked-in header), and OUT (scratch path) as -D defines.
if(NOT EXISTS "${GEN}")
    message(FATAL_ERROR "gen_golden_vectors not built at ${GEN}; run cmake --build build first")
endif()
execute_process(
    COMMAND "${GEN}"
    OUTPUT_FILE "${OUT}"
    RESULT_VARIABLE gen_rc)
if(NOT gen_rc EQUAL 0)
    message(FATAL_ERROR "gen_golden_vectors exited ${gen_rc}")
endif()
execute_process(
    COMMAND "${CMAKE_COMMAND}" -E compare_files "${OUT}" "${REF}"
    RESULT_VARIABLE diff_rc)
if(NOT diff_rc EQUAL 0)
    message(FATAL_ERROR "golden_vectors.h is out of sync with tools/gen_golden_vectors.c; regenerate with ./build/gen_golden_vectors > tests/golden_vectors.h and commit the header")
endif()
