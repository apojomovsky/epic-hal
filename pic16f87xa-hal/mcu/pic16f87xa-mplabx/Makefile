# PIC16F87XA HAL, MPLAB XC8 Makefile
#
# Builds the HAL on real silicon via MPLAB XC8. Tested with:
#   - MPLAB X IDE v6.x
#   - MPLAB XC8 C Compiler v3.10 (the `xc8-cc` driver)
#   - PIC16F877A
#
# XC8 v3.x notes:
#   - The `xc8-cc` driver compiles each translation unit to a `.p1`
#     (p-code) intermediate, NOT a `.o`; the final hex comes from a
#     single link step over the `.p1` set with `-ginhx32`.
#   - v3.x moved device support into a Device Family Pack, so the
#     compiler needs `-mdfp <dir>`. Override DFP_DIR below (or pass
#     make DFP_DIR=/path/to/dfp) if your install lives elsewhere.
#
# From the command line (no IDE):
#
#   export PATH=$PATH:/opt/microchip/xc8/v3.10/bin
#   make MCU=16F877A            # default; also 873A / 874A / 876A
#   make clean
#
# To program the device with PICkit 2/3/4 or ICD, open MPLAB X
# and use the "Make and Program" button. The .hex output here
# is suitable for any external programmer (PK2CMD, ippe, MPLAB IPE).
#
# This file is a thin caller of the shared epic_family.mk fragment
# (epic-common/mk/): the `.c` -> `.p1` pattern rule, the `.hex` link
# step, and clean all live there. Only what is genuinely PIC16F87XA-
# specific stays here: device selection, the DFP flag, the source list,
# the include path, and the configuration-word directives.

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
# Adjust EPIC_DIR if the firmware project is not a sibling of pic16f87xa-hal.
# epic-common holds the shared harness target impl and headers; it is a
# sibling of the HAL tree.
EPIC_DIR    ?= ../..
COMMON_DIR ?= $(EPIC_DIR)/../epic-common
BUILD_DIR  ?= build
TARGET     := $(BUILD_DIR)/$(MCU)-firmware

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
# epic-common/include provides the shared hal_status.h / epic_harness.h.
INC    := -I$(EPIC_DIR)/include/target -I$(EPIC_DIR)/include -I$(COMMON_DIR)/include
CFLAGS := $(DFP_FLAG) \
          -mcpu=$(shell echo $(MCU) | tr A-Z a-z) \
          -O2 -std=c99 -Wall -Wextra \
          $(CFLAGS_DEVICE) \
          $(INC) \
          -DFOSC_HZ=$(FOSC_HZ)

# ─────────────────────────── source layout ─────────────────────────
# The HAL sources in src/ compile for both host (sim) and target.
# The sim/ subdirectory is NOT compiled on real targets, XC8 never
# links pic16f87xa_sim.c. The host/target SFR mapping is chosen by the
# include path (include/target first), and the host-only harness /
# wdt_sleep-sim implementations are simply absent from this source list.
# The family-blind harness target impl lives in epic-common and is
# linked here alongside the PIC16 core.

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
  $(COMMON_DIR)/src/core/epic_harness_target.c

# PSP is only on 40/44-pin parts.
ifeq ($(filter $(MCU),16F874A 16F877A),$(MCU))
  EPIC_SOURCES += $(EPIC_DIR)/src/peripherals/pic16f87xa_psp.c
endif

# Add your application sources here. example_blink is the canonical
# "turn an LED on" smoke test. Replace it with your project's main.c.
APP_SOURCES := $(EPIC_DIR)/tests/example_blink.c

# All real .c sources (the config word is generated separately, below).
SRCS := $(EPIC_SOURCES) $(APP_SOURCES)
# xc8-cc -c emits <name>.p1; link from the .p1 set.
OBJS := $(patsubst %.c,$(BUILD_DIR)/%.p1,$(notdir $(SRCS)))

# Tell make where to find sources referenced by basename. The pattern
# rule $(BUILD_DIR)/%.p1: %.c uses basename matching; without VPATH
# make would not resolve EPIC_DIR/.../pic16f87xa_gpio.c from the
# target name build/pic16f87xa_gpio.p1.
VPATH := $(sort $(dir $(SRCS)))

# ─────────────────────────── config word ───────────────────────────
# The PIC16F87XA Configuration Word is a flash-locked register that
# defines oscillator, WDT, PWRT, BOR, code protection, and LVP at
# programming time.  This file emits the directives, adjust as
# needed per your hardware.  See DS39582B §14.1 (Register 14-1).
CONFIG_SRC := $(BUILD_DIR)/config_$(MCU).c
CONFIG_OBJ := $(BUILD_DIR)/config_$(MCU).p1

# ─────────────────────────── shared build rules ────────────────────
# Pull in the shared boilerplate (the `.c` -> `.p1` pattern rule, the
# generated config-word translation unit, the single `.hex` link step,
# and clean). Included AFTER all variables above are defined, because
# make expands explicit-rule prerequisites at parse time; included
# BEFORE the family-specific config-word recipe below, so `all` (from
# the fragment) stays the default goal.
include $(COMMON_DIR)/mk/epic_family.mk

# Family-specific: the PIC16 config-word directives. The shared
# epic_family.mk provides the rule that compiles $(CONFIG_SRC) to
# $(CONFIG_OBJ) and links it into the hex.
$(CONFIG_SRC): | $(BUILD_DIR)
	@printf '/* Auto-generated. Do not edit. */\n' > $@
	@printf '#include <xc.h>\n' >> $@
	@printf '#pragma config FOSC = HS\n' >> $@
	@printf '#pragma config WDTE = ON\n' >> $@
	@printf '#pragma config PWRTE = ON\n' >> $@
	@printf '#pragma config BOREN = ON\n' >> $@
	@printf '#pragma config LVP = OFF\n' >> $@
	@printf '#pragma config WRT = OFF\n' >> $@
