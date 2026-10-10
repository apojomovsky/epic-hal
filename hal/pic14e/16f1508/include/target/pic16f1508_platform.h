/**
 * XC8 target platform: SFR access through absolute pointers. Selected at
 * build time by putting include/target ahead of include/host.
 */
#ifndef PIC16F1508_PLATFORM_H
#define PIC16F1508_PLATFORM_H

#include <stdint.h>

#define EPIC_WEAK
#define EPIC_PLACE(addr) __at(addr)
#define EPIC_SFR_PTR(addr) ((volatile uint8_t *)(uintptr_t)(addr))
#define EPIC_REG8(addr) (*(volatile uint8_t *)(uintptr_t)(addr))
#define epic_sfr_read8(addr) (*EPIC_SFR_PTR(addr))
#define epic_sfr_write8(addr, value) do { *EPIC_SFR_PTR(addr) = (uint8_t)(value); } while (0)

#endif
