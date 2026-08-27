#ifndef EPIC_CC_H
#define EPIC_CC_H

/* Absolute placement: pins a global to a fixed address. epic-cc reads the
 * address back out of the section name; see irparse's EPIC_AT handling. */
#define EPIC_AT(addr) __attribute__((section(".epicat." #addr)))

/* Config words: exactly one EPIC_CONFIG(...) is permitted across the whole
 * program. epic-cc finds it two ways: a cheap raw-text pre-scan (to derive
 * EPIC_FOSC_HZ before clang runs) and, authoritatively, this section-tagged
 * dummy symbol after the whole program is merged. */
#define EPIC_CONFIG(spec) \
    static const char __epic_config[] __attribute__((used, section(".epiccfg." spec))) = spec

/* Derived from the resolved config words; see the driver's pre-scan. Not
 * usable as a link-time-only symbol on purpose: it must work in #if and in
 * a compile-time array bound, so it is a real preprocessor macro. */
#ifndef EPIC_FOSC_HZ
#define EPIC_FOSC_HZ 0
#endif

#define EPIC_NAKED __attribute__((naked))
#define __epic_nop()    asm volatile("nop")
#define __epic_clrwdt() asm volatile("clrwdt")
#define __epic_sleep()  asm volatile("sleep")
#define __epic_di()     asm volatile("bcf INTCON, 7")
#define __epic_ei()     asm volatile("bsf INTCON, 7")

#endif /* EPIC_CC_H */
