/**
 * @file    target_smoke16.c
 * @brief   PIC16F87XA on-target smoke: calls one representative primitive
 *          per asm-leaf group (multiply, divide, add, BCD) so the linker
 *          pulls in the real asm bodies, proving the backend builds and
 *          links. PIC16's flash/RAM budget is too small for the full
 *          golden-vector self-test (see target_selftest.c, PIC18-only);
 *          see docs/ARCHITECTURE.md "Testing tiers" for the rest of the
 *          PIC16 validation story.
 */

#include "pic8_hal.h"
#include "core/pic8_harness.h"
#include "pic_math.h"

int main(void)
{
    pic8_harness_init(0UL);

    volatile uint16_t r1 = pic_math_mul_u16(0x0102u, 0x0103u);
    pic_math_udiv16_t d  = pic_math_divmod_u16(0x0007u, 0x0002u, 0);
    bool carry = false;
    volatile uint16_t r2 = pic_math_add_u16(0xFFFFu, 0x0002u, &carry);
    volatile uint8_t  r3 = pic_math_bcd_add8(0x55u, 0x55u, &carry);
    volatile uint16_t r4 = pic_math_sqrt_u16(100u);
    (void)r1; (void)d; (void)r2; (void)r3; (void)r4; (void)carry;

    for (uint32_t i = 0; pic8_harness_running(i); i++) {
        pic8_harness_tick();
    }
    return pic8_harness_report(1);
}
