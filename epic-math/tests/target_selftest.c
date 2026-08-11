/*
 * On-target self-test: replays golden_vectors.h through the real
 * inline-asm routines on silicon, comparing against the host-oracle
 * expectations (tools/gen_golden_vectors.c), and streams a PASS/FAIL
 * summary over the family HAL's USART. Runs once at startup, then
 * idles. The USART baud setup is the only family-specific branch;
 * everything else is family-neutral.
 */

#include "epic_hal.h"
#include "core/epic_harness.h"
#include "pic_math.h"
#include "golden_vectors.h"

/**
 * @brief  No-op TX callback placeholder; non-null so EPIC_USART_Init
 *         arms TXEN (both families gate TXEN on the callback being
 *         present). Never actually invoked: transmission here is polled
 *         and GIE stays off.
 *
 * USART output helpers (target has no stdio). s_tx_noop is non-null so
 * EPIC_USART_Init arms TXEN (both families gate TXEN on the callback
 * being present); transmission here is polled and GIE stays off, so it
 * is never actually invoked. putc_ polls the shift register first:
 * EPIC_USART_Transmit without waiting overruns on real hardware (TXREG
 * is one deep) and garbles the report under MPLAB SIM. */
static void s_tx_noop(void)
{
}
/** @brief Transmit one byte, polling the shift register until it drains. */
static void putc_(uint8_t c)
{
    while (!EPIC_USART_IsTxShiftRegisterEmpty()) {
        /* wait for the shift register to drain */
    }
    EPIC_USART_Transmit(c);
}
/** @brief Transmit a NUL-terminated string. */
static void puts_(const char *s) { while (*s) putc_((uint8_t)*s++); }
/** @brief Transmit a 16-bit value as 4 hex digits. */
static void putx16(uint16_t v) {  /* 4-digit hex */
    static const char hex[] = "0123456789ABCDEF";
    putc_((uint8_t)hex[(v >> 12) & 0xF]);
    putc_((uint8_t)hex[(v >>  8) & 0xF]);
    putc_((uint8_t)hex[(v >>  4) & 0xF]);
    putc_((uint8_t)hex[(v >>  0) & 0xF]);
}
/** @brief Transmit a 32-bit value as 8 hex digits. */
static void putx32(uint32_t v) {
    putx16((uint16_t)(v >> 16)); putx16((uint16_t)v);
}
/** @brief Transmit an unsigned value in decimal. */
static void putd(uint32_t v) {  /* unsigned decimal */
    char buf[11]; int n = 0;
    if (v == 0u) { putc_('0'); return; }
    while (v) { buf[n++] = (char)('0' + (v % 10u)); v /= 10u; }
    while (n) putc_((uint8_t)buf[--n]);
}

static uint32_t g_pass = 0u, g_fail = 0u;

/** @brief Log one FAIL record (label, got, exp) over USART and bump g_fail.
 *  A FUNCTION, not an inlined macro body: CHECK_EQ appears at ~150 call
 *  sites, so inlining the print would balloon the code size past the
 *  8 K-word PIC16F87XA flash. */
static void report_fail(const char *label, uint32_t got, uint32_t exp)
{
    g_fail++;
    puts_("FAIL ");
    puts_(label);
    puts_(" got=");
    putx32(got);
    puts_(" exp=");
    putx32(exp);
    puts_("\r\n");
}

#define CHECK_EQ(got, exp, label) do {             \
    if ((got) == (exp)) g_pass++;                  \
    else report_fail(label, (uint32_t)(got), (uint32_t)(exp)); \
} while (0)

/* Per-table runners. */
/** @brief Replay the mul_u8 golden vectors against the on-target asm. */
static void run_mul_u8(void)   { for (int i=0;i<(int)GV_MUL_U8_N;i++)   CHECK_EQ(pic_math_mul_u8(gv_mul_u8[i].a,gv_mul_u8[i].b), gv_mul_u8[i].e, "mul_u8"); }
/** @brief Replay the mul_u16 golden vectors against the on-target asm. */
static void run_mul_u16(void)  { for (int i=0;i<(int)GV_MUL_U16_N;i++)  CHECK_EQ(pic_math_mul_u16(gv_mul_u16[i].a,gv_mul_u16[i].b), gv_mul_u16[i].e, "mul_u16"); }
/** @brief Replay the div_u16 golden vectors against the on-target asm. */
static void run_div_u16(void)  { for (int i=0;i<(int)GV_DIV_U16_N;i++)  { pic_math_udiv16_t r=pic_math_divmod_u16(gv_div_u16[i].n,gv_div_u16[i].d,0);
    CHECK_EQ(r.quotient, gv_div_u16[i].q, "div_u16 q"); CHECK_EQ(r.remainder, gv_div_u16[i].r, "div_u16 r"); } }
/** @brief Replay the div_u32 golden vectors against the on-target asm. */
static void run_div_u32(void) { for (int i=0;i<(int)GV_DIV_U32_16_N;i++){ pic_math_udiv16_t r=pic_math_divmod_u32_16(gv_div_u32_16[i].n,gv_div_u32_16[i].d,0);
    CHECK_EQ(r.quotient, gv_div_u32_16[i].q,"div_u32 q"); CHECK_EQ(r.remainder, gv_div_u32_16[i].r,"div_u32 r"); } }
/** @brief Replay the add_u16 golden vectors against the on-target asm. */
static void run_add(void)      { for (int i=0;i<(int)GV_ADD_U16_N;i++)  { bool c; uint16_t r=pic_math_add_u16(gv_add_u16[i].a,gv_add_u16[i].b,&c);
    CHECK_EQ(r,(uint16_t)gv_add_u16[i].r,"add r"); CHECK_EQ((uint16_t)(c?1u:0u),(uint16_t)gv_add_u16[i].c,"add c"); } }
/** @brief Replay the sub_u16 golden vectors against the on-target asm. */
static void run_sub(void)      { for (int i=0;i<(int)GV_SUB_U16_N;i++)  { bool b; uint16_t r=pic_math_sub_u16(gv_sub_u16[i].a,gv_sub_u16[i].b,&b);
    CHECK_EQ(r,(uint16_t)gv_sub_u16[i].r,"sub r"); CHECK_EQ((uint16_t)(b?1u:0u),(uint16_t)gv_sub_u16[i].b_out,"sub b"); } }
/** @brief Replay the neg_s16 golden vectors against the on-target asm. */
static void run_neg_s16(void)  { for (int i=0;i<(int)GV_NEG_S16_N;i++)  CHECK_EQ((uint16_t)pic_math_negate_s16(gv_neg_s16[i].v),(uint16_t)gv_neg_s16[i].e,"neg_s16"); }
/** @brief Replay the neg_s32 golden vectors against the on-target asm. */
static void run_neg_s32(void)  { for (int i=0;i<(int)GV_NEG_S32_N;i++)  CHECK_EQ((uint32_t)pic_math_negate_s32(gv_neg_s32[i].v),(uint32_t)gv_neg_s32[i].e,"neg_s32"); }
/** @brief Replay the bcd_add8 golden vectors against the on-target asm. */
static void run_bcd_add8(void) { for (int i=0;i<(int)GV_BCD_ADD8_N;i++){ bool c; uint8_t r=pic_math_bcd_add8(gv_bcd_add8[i].a,gv_bcd_add8[i].b,&c);
    CHECK_EQ(r,gv_bcd_add8[i].r,"bcd_add r"); CHECK_EQ((uint16_t)(c?1u:0u),(uint16_t)gv_bcd_add8[i].c,"bcd_add c"); } }
/** @brief Replay the bcd_sub8 golden vectors against the on-target asm. */
static void run_bcd_sub8(void) { for (int i=0;i<(int)GV_BCD_SUB8_N;i++){ bool b; uint8_t r=pic_math_bcd_sub8(gv_bcd_sub8[i].a,gv_bcd_sub8[i].b,&b);
    CHECK_EQ(r,gv_bcd_sub8[i].r,"bcd_sub r"); CHECK_EQ((uint16_t)(b?1u:0u),(uint16_t)gv_bcd_sub8[i].bo,"bcd_sub b"); } }
/* The C-over-asm wrappers (mul_s16/divmod_s16/BCD conversions) and the
 * portable-C derived routines (sqrt/diff3/simpson38/rand) are validated by
 * the exhaustive/randomized Tier-1 host tests; they share one C body across
 * backends, so there is no separate asm to verify on target. The on-target
 * set stays the asm leaves -- it also fits the 8 K-word PIC16F87XA flash. */

/** @brief Replay all golden vectors on silicon, stream a PASS/FAIL summary, then idle. */
int main(void)
{
    epic_harness_init(0UL);

    /* USART @ 9600 8N1. Family branch: PIC18 has the 16-bit BRG + 5-arg
     * compute; PIC16 has the 8-bit BRG + 4-arg compute. */
    USART_HandleTypeDef h = USART_HANDLE_DEFAULT;
    h.TxCpltCallback = s_tx_noop;   /* TXEN is only armed when the
                                     * callback is non-null; never
                                     * actually called (GIE stays off,
                                     * transmission is polled). */
#if defined(PIC18F2455) || defined(PIC18F2550) || defined(PIC18F4455) || defined(PIC18F4550)
    h.BaudGen = USART_BAUDGEN_16BIT;
    uint16_t sp = USART_ComputeSPBRG((uint32_t)FOSC_HZ, 9600u,
                                     USART_MODE_ASYNCHRONOUS, USART_BRGH_HIGH,
                                     USART_BAUDGEN_16BIT);
    h.SPBRG  = (uint8_t)(sp & 0xFFu);
    h.SPBRGH = (uint8_t)(sp >> 8);
#else
    uint16_t sp = USART_ComputeSPBRG((uint32_t)FOSC_HZ, 9600u,
                                     USART_MODE_ASYNCHRONOUS, USART_BRGH_HIGH);
    h.SPBRG = (uint8_t)sp;
#endif
    h.Mode      = USART_MODE_ASYNCHRONOUS;
    h.BaudHigh  = USART_BRGH_HIGH;
    h.DataWidth = USART_DATA_8BITS;
    EPIC_USART_Init(&h);

    puts_("\r\nepic-math target self-test\r\n");

    run_mul_u8();  run_mul_u16();
    run_div_u16(); run_div_u32();
    run_add();     run_sub();      run_neg_s16(); run_neg_s32();
    run_bcd_add8(); run_bcd_sub8();

    puts_("PASS="); putd(g_pass);
    puts_(" FAIL="); putd(g_fail);
    puts_("\r\n");
    if (g_fail == 0u) puts_("RESULT: ALL PASS\r\n");
    else              puts_("RESULT: FAILURES PRESENT\r\n");

    /* Firmware runs forever on a real target. */
    for (uint32_t i = 0; epic_harness_running(i); i++) {
        epic_harness_tick();
    }
    return epic_harness_report(g_fail == 0u);
}
