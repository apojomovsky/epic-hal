# pic8-math, MPLAB XC8 Makefile -- PIC16F87XA family variant.
#
# Builds the pic8-math PIC16 inline-asm backend (src/pic16/) plus the
# shared portable-C derived routines (src/common/) and the on-target
# self-test (tests/target_selftest.c) on real silicon via MPLAB XC8.
# Tested with:
#   - MPLAB X IDE v6.x
#   - MPLAB XC8 C Compiler v3.10 (the `xc8-cc` driver)
#   - PIC16F877A  (and 873A / 874A / 876A)
#
# This Makefile mirrors pic8-taskmgr's mcu/pic16f87xa-taskmgr-mplabx/Makefile:
# the host/target split is done at build time, not with #ifdef, include/target
# is put ahead of include on the include path so pic16f87xa_platform.h
# resolves to the real-target (volatile-dereference) version, and the
# target-side harness impl (pic8_harness_target.c) is linked. The host-only
# src/host/ backend and the family's *_sim.c files are absent from the
# source list.
#
# From the command line (no IDE):
#
#   export PATH=$PATH:/opt/microchip/xc8/v3.10/bin
#   make MCU=16F877A            # default; also 873A / 874A / 876A
#   make clean
#
# This file is a thin caller of the shared pic8_family.mk fragment
# (pic8-common/mk/): the `.c` -> `.p1` pattern rule, the `.hex` link step,
# and clean all live there. Only what is math/PIC16-specific stays here:
# device selection, the DFP flag, the source list, the include path, and
# the configuration-word directives.

# ─────────────────────────── device selection ─────────────────────
MCU     ?= 16F877A
FOSC_HZ ?= 20000000

ifeq ($(MCU),16F873A)
  CFLAGS_DEVICE := -DPIC16F873A
else ifeq ($(MCU),16F874A)
  CFLAGS_DEVICE := -DPIC16F874A
else ifeq ($(MCU),16F876A)
  CFLAGS_DEVICE := -DPIC16F876A
else ifeq ($(MCU),16F877A)
  CFLAGS_DEVICE := -DPIC16F877A
else
  $(error Unsupported MCU=$(MCU); choose 873A, 874A, 876A or 877A)
endif

# ─────────────────────────── paths ────────────────────────────────
# EPIC_DIR points at the pic16f87xa-hal tree; APP_DIR at this pic8-math
# tree; COMMON_DIR at the shared pic8-common layer. Adjust if the layout
# differs from the default sibling layout.
EPIC_DIR    ?= ../../../pic16f87xa-hal
COMMON_DIR ?= $(EPIC_DIR)/../pic8-common
APP_DIR    ?= ../..
BUILD_DIR  ?= build
TARGET     := $(BUILD_DIR)/$(MCU)-math-selftest

# XC8 v3.x device family pack. Empty -> the flag is omitted (use when
# the toolchain resolves devices without an explicit DFP, e.g. older XC8).
DFP_DIR ?= /opt/microchip/xc8/v3.10/pic/packs/Microchip.PIC16Fxxx_DFP/xc8
ifneq ($(strip $(DFP_DIR)),)
  DFP_FLAG := -mdfp=$(DFP_DIR)
else
  DFP_FLAG :=
endif

CC     := xc8-cc
# include/target ahead of include so pic16f87xa_platform.h resolves to the
# real-target (volatile-dereference) version, not the host memory-backed one.
# pic8-math's own header lives under APP_DIR/include; the shared HAL
# status / harness headers live under pic8-common/include.
INC    := -I$(EPIC_DIR)/include/target -I$(EPIC_DIR)/include -I$(COMMON_DIR)/include -I$(APP_DIR)/include -I$(APP_DIR)/tests
CFLAGS := $(DFP_FLAG) \
          -mcpu=$(shell echo $(MCU) | tr A-Z a-z) \
          -O2 -std=c99 -Wall -Wextra \
          $(CFLAGS_DEVICE) \
          $(INC) \
          -DFOSC_HZ=$(FOSC_HZ)

# ─────────────────────────── source layout ─────────────────────────
# The full HAL peripheral/core sources compile for the target exactly as they
# do for the host; the sim/ backend and the *_sim.c harness are NOT compiled
# (XC8 never links pic16f87xa_sim.c). The family-blind harness target impl
# lives in pic8-common and is linked here alongside the PIC16 core and the
# pic8-math sources.
#
# The on-target app is the PIC16 smoke (tests/target_smoke16.c), not the
# full golden-vector self-test: the PIC16F87XA family tops out at 8 K words
# flash / 368 B RAM, too small for math + full HAL + golden_vectors.h +
# target_selftest.c. The full Tier-3 self-test is the PIC18 artifact
# (mcu/pic18fxx5x-math-mplabx). See docs/ARCHITECTURE.md "Testing tiers".

EPIC_SOURCES := \
  $(EPIC_DIR)/src/peripherals/pic16f87xa_gpio.c     \
  $(EPIC_DIR)/src/peripherals/pic16f87xa_timer0.c   \
  $(EPIC_DIR)/src/peripherals/pic16f87xa_timer1.c   \
  $(EPIC_DIR)/src/peripherals/pic16f87xa_timer2.c   \
  $(EPIC_DIR)/src/peripherals/pic16f87xa_ccp.c      \
  $(EPIC_DIR)/src/peripherals/pic16f87xa_usart.c    \
  $(EPIC_DIR)/src/peripherals/pic16f87xa_ssp.c      \
  $(EPIC_DIR)/src/peripherals/pic16f87xa_adc.c      \
  $(EPIC_DIR)/src/peripherals/pic16f87xa_comp.c     \
  $(EPIC_DIR)/src/peripherals/pic16f87xa_vref.c     \
  $(EPIC_DIR)/src/peripherals/pic16f87xa_eeprom.c   \
  $(EPIC_DIR)/src/core/pic16_irq.c                  \
  $(EPIC_DIR)/src/core/pic16f87xa_wdt_sleep.c       \
  $(EPIC_DIR)/src/core/pic16f87xa_wdt_sleep_target.c \
  $(EPIC_DIR)/src/core/pic16_isr_vector.c          \
  $(EPIC_DIR)/src/core/pic16_irq_dispatch.c        \
  $(COMMON_DIR)/src/core/pic8_harness_target.c

# PSP is only on 40/44-pin parts.
ifeq ($(filter $(MCU),16F874A 16F877A),$(MCU))
  EPIC_SOURCES += $(EPIC_DIR)/src/peripherals/pic16f87xa_psp.c
endif

# pic8-math: the PIC16 inline-asm leaf primitives (src/pic16/) plus the
# shared portable-C derived routines (src/common/), which every backend
# links. The host src/host/ backend is NOT compiled here.
MATH_SOURCES := \
  $(APP_DIR)/src/pic16/pic_math_mul.c      \
  $(APP_DIR)/src/pic16/pic_math_div.c      \
  $(APP_DIR)/src/pic16/pic_math_addsub.c   \
  $(APP_DIR)/src/pic16/pic_math_bcd.c      \
  $(APP_DIR)/src/pic16/pic_math_scratch.c  \
  $(APP_DIR)/src/common/pic_math_sqrt.c    \
  $(APP_DIR)/src/common/pic_math_numeric.c \
  $(APP_DIR)/src/common/pic_math_rand.c

# The application: the PIC16 on-target smoke (tests/target_smoke16.c), which
# links the real PIC16 inline-asm math backend and exercises a few routines.
# The full golden-vector self-test (tests/target_selftest.c) is the PIC18
# Tier-3 artifact: the 8 K-word / 368 B PIC16F87XA cannot hold math + full HAL
# + golden_vectors.h + the self-test, and the HAL's ISR/irq code addresses
# only bank 0, so the self-test's data (USART handle + counters + golden-
# vector access) spills to bank 1 and overflows. PIC16 Tier-3 therefore relies
# on Tier-1 (exhaustive host tests) + the asm build (the smoke proves the
# backends link) + the hand-traces. See docs/ARCHITECTURE.md "Testing tiers".
APP_SOURCES := $(APP_DIR)/tests/target_smoke16.c

# All real .c sources (the config word is generated separately, below).
SRCS := $(EPIC_SOURCES) $(MATH_SOURCES) $(APP_SOURCES)
# xc8-cc -c emits <name>.p1; link from the .p1 set.
OBJS := $(patsubst %.c,$(BUILD_DIR)/%.p1,$(notdir $(SRCS)))

# Tell make where to find sources referenced by basename.
VPATH := $(sort $(dir $(SRCS)))

# ─────────────────────────── config word ───────────────────────────
# The PIC16F87XA Configuration Word is flash-locked; this file emits the
# directives, adjust per your hardware. See DS39582B §14.1 (Register 14-1).
CONFIG_SRC := $(BUILD_DIR)/config_$(MCU).c
CONFIG_OBJ := $(BUILD_DIR)/config_$(MCU).p1

# ─────────────────────────── shared build rules ────────────────────
# Pull in the shared boilerplate (the `.c` -> `.p1` pattern rule, the
# generated config-word translation unit, the single `.hex` link step,
# and clean). Included AFTER all variables above are defined, and
# BEFORE the family-specific config-word recipe below so `all` stays
# the default goal.
include $(COMMON_DIR)/mk/pic8_family.mk

$(CONFIG_SRC): | $(BUILD_DIR)
	@printf '/* Auto-generated. Do not edit. */\n' > $@
	@printf '#include <xc.h>\n' >> $@
	@printf '#pragma config FOSC = HS\n' >> $@
	@printf '#pragma config WDTE = ON\n' >> $@
	@printf '#pragma config PWRTE = ON\n' >> $@
	@printf '#pragma config BOREN = ON\n' >> $@
	@printf '#pragma config LVP = OFF\n' >> $@
	@printf '#pragma config WRT = OFF\n' >> $@
