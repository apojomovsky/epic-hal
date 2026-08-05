# epic-math, MPLAB XC8 Makefile -- PIC18F2455 family variant.
#
# Builds the SAME epic-math library + on-target self-test against the
# pic18fxx5x-hal, using the PIC18 inline-asm backend (src/pic18/), which
# exploits the hardware `MULWF` 8x8->16 multiply that neither PIC16 nor
# the PIC17C42 AN544 targeted had. The epic-math header (pic_math.h/.c) is
# family-agnostic and builds unmodified against either family; only this
# Makefile's EPIC_DIR, device list, DFP, source list, and config word differ
# from the PIC16 variant under mcu/pic16f87xa-math-mplabx/.
#
# Tested with MPLAB X IDE v6.x, MPLAB XC8 v3.10, PIC18F4550.
#
#   export PATH=$PATH:/opt/microchip/xc8/v3.10/bin
#   make MCU=18F4550          # default; also 2455 / 2550 / 4455
#   make clean
#
# The PIC18Fxxxx DFP is NOT bundled with XC8; it is installed under
# /opt/microchip/xc8/v3.10/pic/packs/Microchip.PIC18Fxxxx_DFP in this
# environment. Point DFP_DIR elsewhere or leave it empty if your layout
# differs (an empty DFP_DIR omits the -mdfp flag).
#
# This file is a thin caller of the shared epic_family.mk fragment
# (epic-common/mk/). Only the math/PIC18-specific bits stay here.

# ─────────────────────────── device selection ─────────────────────
MCU     ?= 18F4550
FOSC_HZ ?= 48000000

ifeq ($(MCU),18F2455)
  CFLAGS_DEVICE := -DPIC18F2455
else ifeq ($(MCU),18F2550)
  CFLAGS_DEVICE := -DPIC18F2550
else ifeq ($(MCU),18F4455)
  CFLAGS_DEVICE := -DPIC18F4455
else ifeq ($(MCU),18F4550)
  CFLAGS_DEVICE := -DPIC18F4550
else
  $(error Unsupported MCU=$(MCU); choose 2455, 2550, 4455 or 4550)
endif

# The 32 KB-flash parts (18F2550 / 18F4550) have a 4th code block and so a
# CP3 / WRT3 / EBTR3 config setting; the 24 KB parts stop at block 2.
ifeq ($(filter $(MCU),18F2550 18F4550),$(MCU))
  HAS_BLOCK3 := 1
else
  HAS_BLOCK3 :=
endif

# ─────────────────────────── paths ────────────────────────────────
EPIC_DIR    ?= ../../../pic18fxx5x-hal
COMMON_DIR ?= $(EPIC_DIR)/../epic-common
APP_DIR    ?= ../..
BUILD_DIR  ?= build
TARGET     := $(BUILD_DIR)/$(MCU)-math-selftest

DFP_DIR ?= /opt/microchip/xc8/v3.10/pic/packs/Microchip.PIC18Fxxxx_DFP/xc8
ifneq ($(strip $(DFP_DIR)),)
  DFP_FLAG := -mdfp=$(DFP_DIR)
else
  DFP_FLAG :=
endif

CC     := xc8-cc
# include/target ahead of include so pic18_platform.h resolves to the
# real-target version. APP_DIR/include is epic-math's own header.
INC    := -I$(EPIC_DIR)/include/target -I$(EPIC_DIR)/include -I$(COMMON_DIR)/include -I$(APP_DIR)/include -I$(APP_DIR)/tests
CFLAGS := $(DFP_FLAG) \
          -mcpu=$(shell echo $(MCU) | tr A-Z a-z) \
          -O2 -std=c99 -Wall -Wextra \
          $(CFLAGS_DEVICE) \
          $(INC) \
          -DFOSC_HZ=$(FOSC_HZ)

# ─────────────────────────── source layout ─────────────────────────
# The full PIC18 HAL peripheral/core sources (Phase 4 done: GPIO, Timer0-3,
# ECCP1/CCP2, MSSP, EUSART, Comparator, EEPROM, ADC, SPP). The host-only
# sim/harness-sim/wdt_sleep-sim files are absent; the family-blind harness
# target impl lives in epic-common.

EPIC_SOURCES := \
  $(EPIC_DIR)/src/peripherals/pic18fxx5x_gpio.c            \
  $(EPIC_DIR)/src/peripherals/pic18fxx5x_timer0.c          \
  $(EPIC_DIR)/src/peripherals/pic18fxx5x_timer1.c          \
  $(EPIC_DIR)/src/peripherals/pic18fxx5x_timer2.c          \
  $(EPIC_DIR)/src/peripherals/pic18fxx5x_timer3.c          \
  $(EPIC_DIR)/src/peripherals/pic18fxx5x_ccp.c             \
  $(EPIC_DIR)/src/peripherals/pic18fxx5x_usart.c           \
  $(EPIC_DIR)/src/peripherals/pic18fxx5x_ssp.c            \
  $(EPIC_DIR)/src/peripherals/pic18fxx5x_adc.c             \
  $(EPIC_DIR)/src/peripherals/pic18fxx5x_comp.c           \
  $(EPIC_DIR)/src/peripherals/pic18fxx5x_eeprom.c         \
  $(EPIC_DIR)/src/peripherals/pic18fxx5x_spp.c            \
  $(EPIC_DIR)/src/core/pic18_irq.c                         \
  $(EPIC_DIR)/src/core/pic18_irq_dispatch.c                \
  $(EPIC_DIR)/src/core/pic18_isr_vector.c                  \
  $(EPIC_DIR)/src/core/pic18fxx5x_wdt_sleep.c              \
  $(EPIC_DIR)/src/core/pic18fxx5x_wdt_sleep_target.c       \
  $(COMMON_DIR)/src/core/epic_harness_target.c

# epic-math: the PIC18 inline-asm leaf primitives (src/pic18/, incl. the
# hardware-MULWF multiply path) plus the shared portable-C derived
# routines (src/common/). The host src/host/ backend is NOT compiled here.
MATH_SOURCES := \
  $(APP_DIR)/src/pic18/pic_math_mul.c      \
  $(APP_DIR)/src/pic18/pic_math_div.c      \
  $(APP_DIR)/src/pic18/pic_math_addsub.c   \
  $(APP_DIR)/src/pic18/pic_math_bcd.c      \
  $(APP_DIR)/src/common/pic_math_sqrt.c    \
  $(APP_DIR)/src/common/pic_math_numeric.c \
  $(APP_DIR)/src/common/pic_math_rand.c

# The application: the on-target self-test (Tier 3). Phase 5 fills in the
# golden_vectors.h replay + USART PASS/FAIL reporting; Phase 0 builds the
# harness-only stub to prove the XC8 pipeline links.
APP_SOURCES := $(APP_DIR)/tests/target_selftest.c

SRCS := $(EPIC_SOURCES) $(MATH_SOURCES) $(APP_SOURCES)
OBJS := $(patsubst %.c,$(BUILD_DIR)/%.p1,$(notdir $(SRCS)))
VPATH := $(sort $(dir $(SRCS)))

# ─────────────────────────── config word ───────────────────────────
# Same PIC18F4550-family config as the HAL Makefile; see DS39632E §23.1.
CONFIG_SRC := $(BUILD_DIR)/config_$(MCU).c
CONFIG_OBJ := $(BUILD_DIR)/config_$(MCU).p1

include $(COMMON_DIR)/mk/epic_family.mk

$(CONFIG_SRC): | $(BUILD_DIR)
	@printf '/* Auto-generated. Do not edit. */\n' > $@
	@printf '#include <xc.h>\n' >> $@
	@printf '#pragma config FOSC = HS\n'        >> $@
	@printf '#pragma config PLLDIV = 5\n'       >> $@
	@printf '#pragma config CPUDIV = OSC1_PLL2\n' >> $@
	@printf '#pragma config USBDIV = 2\n'       >> $@
	@printf '#pragma config FCMEN = OFF\n'      >> $@
	@printf '#pragma config IESO = OFF\n'      >> $@
	@printf '#pragma config PWRT = ON\n'        >> $@
	@printf '#pragma config BOR = OFF\n'        >> $@
	@printf '#pragma config BORV = 3\n'         >> $@
	@printf '#pragma config VREGEN = OFF\n'     >> $@
	@printf '#pragma config WDT = ON\n'         >> $@
	@printf '#pragma config WDTPS = 32768\n'    >> $@
	@printf '#pragma config MCLRE = ON\n'       >> $@
	@printf '#pragma config LPT1OSC = OFF\n'    >> $@
	@printf '#pragma config PBADEN = OFF\n'     >> $@
	@printf '#pragma config CCP2MX = ON\n'      >> $@
	@printf '#pragma config STVREN = ON\n'      >> $@
	@printf '#pragma config LVP = OFF\n'        >> $@
	@printf '#pragma config XINST = OFF\n'      >> $@
	@printf '#pragma config DEBUG = OFF\n'      >> $@
ifdef HAS_BLOCK3
	@printf '#pragma config CP0 = OFF, CP1 = OFF, CP2 = OFF, CP3 = OFF\n' >> $@
	@printf '#pragma config WRT0 = OFF, WRT1 = OFF, WRT2 = OFF, WRT3 = OFF\n' >> $@
	@printf '#pragma config EBTR0 = OFF, EBTR1 = OFF, EBTR2 = OFF, EBTR3 = OFF\n' >> $@
else
	@printf '#pragma config CP0 = OFF, CP1 = OFF, CP2 = OFF\n' >> $@
	@printf '#pragma config WRT0 = OFF, WRT1 = OFF, WRT2 = OFF\n' >> $@
	@printf '#pragma config EBTR0 = OFF, EBTR1 = OFF, EBTR2 = OFF\n' >> $@
endif
	@printf '#pragma config CPB = OFF, CPD = OFF\n' >> $@
	@printf '#pragma config WRTC = OFF, WRTB = OFF, WRTD = OFF\n' >> $@
	@printf '#pragma config EBTRB = OFF\n'      >> $@
