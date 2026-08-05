# Shared CMake helpers for every 8-bit PIC HAL family.
#
# A family's CMakeLists.txt sets a few variables describing that family
# (its source list, include dirs, default device, the optional peripheral
# gate, and the list of devices in the family) and then calls these
# functions to build the static library and the examples. Everything that
# is identical between families — the host-include-first convention, the
# per-target device-select define, the example/link boilerplate, the
# "build this example for every device in the family" loop — lives here,
# so a new family's CMakeLists.txt is a thin caller.
#
# Variables the caller sets before including this file:
#   PIC8_EPIC_LIB              static library target name (e.g. pic16f87xa_hal)
#   PIC8_DEFAULT_DEVICE       device macro for the default build (PIC16F877A)
#   PIC8_FAMILY_INCLUDE_DIR   this family's include/ directory
#   PIC8_HOST_INCLUDE_DIR     this family's include/host directory (resolved
#                            first, so <family>_platform.h picks the host body)
#   PIC8_COMMON_INCLUDE_DIR  pic8-common/include (shared headers: hal_status.h,
#                            pic8_harness.h) — added PUBLIC so consumers
#                            that link the library see them too
#   PIC8_FAMILY_DEVICES       list of every device macro in the family
#
# Variables the caller sets before calling pic8_add_hal_library:
#   PIC8_EPIC_SOURCES          the family's .c source list (per family)
#   PIC8_EPIC_COMPILE_DEFS     PRIVATE compile defs for the lib (optional)
#
# A family that has an optional peripheral only on some devices gates it
# itself before calling pic8_add_hal_library (see pic16f87xa-hal's PSP).

# Build the family's static library. Host include dir goes first so the
# build selects the memory-backed platform header; the shared pic8-common
# headers and the family's own headers are PUBLIC so every consumer that
# links this library resolves them. The default device is a PRIVATE define
# on the library so the host platform is chosen by include path, not #ifdef.
function(pic8_add_hal_library name)
    add_library(${name} STATIC ${PIC8_EPIC_SOURCES})
    target_include_directories(${name} PUBLIC
        ${PIC8_HOST_INCLUDE_DIR}
        ${PIC8_FAMILY_INCLUDE_DIR}
        ${PIC8_COMMON_INCLUDE_DIR})
    target_compile_definitions(${name} PRIVATE -D${PIC8_DEFAULT_DEVICE})
    if(PIC8_EPIC_COMPILE_DEFS)
        target_compile_definitions(${name} PRIVATE ${PIC8_EPIC_COMPILE_DEFS})
    endif()
endfunction()

# Build one example executable against the family library. Device
# selection is the only per-example compile definition; the host platform
# is chosen by the include path inherited from the library.
function(pic8_add_example name src)
    add_executable(${name} ${src})
    target_compile_definitions(${name} PRIVATE -D${PIC8_DEFAULT_DEVICE})
    target_link_libraries(${name} PRIVATE ${PIC8_EPIC_LIB})
endfunction()

# Build the same example source once per device in the family, each with
# its own device-select define. Used for the canonical blink smoke test to
# prove every part in the family compiles and links.
function(pic8_add_example_per_device base src)
    foreach(dev ${PIC8_FAMILY_DEVICES})
        add_executable(${base}_${dev} ${src})
        target_compile_definitions(${base}_${dev} PRIVATE -D${dev})
        target_link_libraries(${base}_${dev} PRIVATE ${PIC8_EPIC_LIB})
    endforeach()
endfunction()
