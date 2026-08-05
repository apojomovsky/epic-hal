/**
 * @file    pic_math_scratch.c (PIC16 inline-asm backend)
 * @brief   Definition of the shared 16-byte file-scratch buffer.
 *
 * @details
 *   See pic_math_scratch.h. One object, placed by XC8 in one bank (bank 0,
 *   addresses < 0x80, so inline-asm operand fixups do not overflow). Linked
 *   by the PIC16 XC8 Makefile alongside the other src/pic16/ bodies.
 */

#include "pic_math_scratch.h"

volatile uint8_t pic16_mscratch[16];