# Shared CMake helpers for every 8-bit PIC HAL family. The caller sets
# EPIC_LIB/EPIC_DEFAULT_DEVICE/EPIC_FAMILY_INCLUDE_DIR (plus optional
# EPIC_HOST_INCLUDE_DIR, EPIC_COMMON_INCLUDE_DIR, EPIC_CORE_INCLUDE_DIR,
# EPIC_FAMILY_DEVICES, EPIC_SOURCES, EPIC_COMPILE_DEFS), then calls the
# functions below. Everything identical between families lives here, so
# a new family's CMakeLists.txt stays a thin caller. A family with an
# optional peripheral on some devices gates it itself first (see the
# 87XA's PSP gate).

# Build the family's static library. Host include dir goes first so the
# build selects the memory-backed platform header; the shared epic-common
# headers and the family's own headers are PUBLIC so every consumer that
# links this library resolves them. The default device is a PRIVATE define
# on the library so the host platform is chosen by include path, not #ifdef.
function(epic_add_hal_library name)
    add_library(${name} STATIC ${EPIC_SOURCES})
    target_include_directories(${name} PUBLIC
        ${EPIC_HOST_INCLUDE_DIR}
        ${EPIC_FAMILY_INCLUDE_DIR}
        ${EPIC_COMMON_INCLUDE_DIR})
    if(EPIC_CORE_INCLUDE_DIR)
        target_include_directories(${name} PUBLIC ${EPIC_CORE_INCLUDE_DIR})
    endif()
    target_compile_definitions(${name} PRIVATE -D${EPIC_DEFAULT_DEVICE})
    if(EPIC_COMPILE_DEFS)
        target_compile_definitions(${name} PRIVATE ${EPIC_COMPILE_DEFS})
    endif()
endfunction()

# Build one example executable against the family library. Device
# selection is the only per-example compile definition; the host platform
# is chosen by the include path inherited from the library.
function(epic_add_example name src)
    add_executable(${name} ${src})
    target_compile_definitions(${name} PRIVATE -D${EPIC_DEFAULT_DEVICE})
    target_link_libraries(${name} PRIVATE ${EPIC_LIB})
endfunction()

# Build the same example source once per device in the family, each with
# its own device-select define. Used for the canonical blink smoke test to
# prove every part in the family compiles and links.
function(epic_add_example_per_device base src)
    foreach(dev ${EPIC_FAMILY_DEVICES})
        add_executable(${base}_${dev} ${src})
        target_compile_definitions(${base}_${dev} PRIVATE -D${dev})
        target_link_libraries(${base}_${dev} PRIVATE ${EPIC_LIB})
    endforeach()
endfunction()
