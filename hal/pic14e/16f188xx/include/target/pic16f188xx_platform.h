/**
 * XC8 target platform: SFR access through absolute pointers. Selected at
 * build time by putting include/target ahead of include/host.
 */
#ifndef PIC16F188XX_PLATFORM_H
#define PIC16F188XX_PLATFORM_H

#include <stdint.h>

#define EPIC_WEAK
#define EPIC_PLACE(addr) __at(addr)
#define EPIC_SFR_PTR(addr) ((volatile uint8_t *)(uintptr_t)(addr))
#define EPIC_REG8(addr) (*(volatile uint8_t *)(uintptr_t)(addr))
#define epic_sfr_read8(addr) (*EPIC_SFR_PTR(addr))
#define epic_sfr_write8(addr, value) do { *EPIC_SFR_PTR(addr) = (uint8_t)(value); } while (0)
/* PPSLOCK key sequences: banksel, two key writes, then an explicit bcf
 * (unlock) or bsf (lock), as the datasheet sequence has it. A C bit clear
 * compiles to movlw/andwf, and MPLAB SIM does not take that as the unlock
 * (2026-10, 16F18877). Interrupts must be off around the sequence. */
#define epic_pps_unlock_sequence() \
    do { \
        asm("banksel PPSLOCK"); \
        asm("movlw 0x55"); \
        asm("movwf PPSLOCK"); \
        asm("movlw 0xAA"); \
        asm("movwf PPSLOCK"); \
        asm("bcf PPSLOCK,0"); \
    } while (0)
#define epic_pps_lock_sequence() \
    do { \
        asm("banksel PPSLOCK"); \
        asm("movlw 0x55"); \
        asm("movwf PPSLOCK"); \
        asm("movlw 0xAA"); \
        asm("movwf PPSLOCK"); \
        asm("bsf PPSLOCK,0"); \
    } while (0)

/* NVM write start (DS40001802D §10.4.2): one banksel, the two key writes
 * and an explicit bsf WR, so no instruction lands inside the unlock
 * window. WREN, the address and the data are set before. Interrupts must
 * be off around the sequence. */
#define epic_nvm_start_write() \
    do { \
        asm("banksel NVMCON2"); \
        asm("movlw 0x55"); \
        asm("movwf NVMCON2"); \
        asm("movlw 0xAA"); \
        asm("movwf NVMCON2"); \
        asm("bsf NVMCON1,1"); \
    } while (0)

#endif
