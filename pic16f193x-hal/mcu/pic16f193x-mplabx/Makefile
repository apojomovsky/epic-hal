# PIC16F193X HAL, MPLAB XC8 Makefile
#
# Builds the HAL on real silicon via MPLAB XC8. Tested against:
#   - MPLAB X IDE v6.x
#   - MPLAB XC8 C Compiler (the `xc8-cc` driver)
#   - PIC16F1937
#
# The PIC16F193X is an Enhanced Mid-range part that lives in the
# Microchip.PIC12-16F1xxx_DFP, which is NOT bundled with the XC8
# installer and is NOT pinned in this repo's CI toolchain image yet.
# Install it (via MPLAB X Tools > Packs or unpack the .atpack from
# packs.download.microchip.com) and point DFP_DIR at its xc8/ dir.
#
# XC8 v3.x notes:
#   - The `xc8-cc` driver compiles each translation unit to a `.p1`
#     (p-code) intermediate, NOT a `.o`; the final hex comes from a
#     single link step over the `.p1` set with `-ginhx32`.
#   - v3.x moved device support into the DFP, so the compiler needs
#     `-mdfp <dir>`.
#
# From the command line (no IDE):
#
#   export PATH=$PATH:/opt/microchip/xc8/v<ver>/bin
#   make MCU=16F1937            # default; also 1933/1934/1936/1938/1939
#   make clean
#
# This file is a thin caller of the shared pic8_family.mk fragment
# (pic8-common/mk/). Only what is genuinely PIC16F193X-specific stays
# here: device selection, the DFP flag, the source list, the include
# path, and the configuration-word directives.
#
# STATUS: draft pending the PIC12-16F1xxx_DFP. The `#pragma config`
# directive names below are the standard Enhanced Mid-range set; the
# exact accepted spellings come from the DFP's device `.PIC`, so the
# real-target build + the §4 mdb gate (docs/adding-a-device.md) verify
# them the first time the Makefile is run with the pack installed.

# ─────────────────────────── device selection ─────────────────────
MCU     ?= 16F1937
FOSC_HZ ?= 32000000

ifeq ($(MCU),16F1933)
  CFLAGS_DEVICE := -DPIC16F1933
else ifeq ($(MCU),16F1934)
  CFLAGS_DEVICE := -DPIC16F1934
else ifeq ($(MCU),16F1936)
  CFLAGS_DEVICE := -DPIC16F1936
else ifeq ($(MCU),16F1937)
  CFLAGS_DEVICE := -DPIC16F1937
else ifeq ($(MCU),16F1938)
  CFLAGS_DEVICE := -DPIC16F1938
else ifeq ($(MCU),16F1939)
  CFLAGS_DEVICE := -DPIC16F1939
else
  $(error Unsupported MCU=$(MCU); choose 1933, 1934, 1936, 1937, 1938 or 1939)
endif

# PORTD/PORTE are 40/44-pin only (16F1934/1937/1939).
ifeq ($(filter $(MCU),16F1934 16F1937 16F1939),$(MCU))
  HAS_PORTD := 1
else
  HAS_PORTD := 0
endif

# ─────────────────────────── paths ────────────────────────────────
HAL_DIR    ?= ../..
COMMON_DIR ?= $(HAL_DIR)/../pic8-common
BUILD_DIR  ?= build
TARGET     := $(BUILD_DIR)/$(MCU)-firmware

# HARNESS=target (default): real-target harness, infinite loop, no
# stdout, unchanged from the foundation. HARNESS=sim: bounded harness
# that drives RA0 (PORTA bit 0) to indicate PASS/FAIL, for driving
# under MPLAB SIM with scripts/sim-mdb-run.sh MODE=gpio. Needs its
# own TARGET suffix so a sim build doesn't clobber a target build's
# .hex in the same build/ dir.
HARNESS ?= target
ifeq ($(HARNESS),sim)
  TARGET     := $(BUILD_DIR)/$(MCU)-firmware-sim
  HARNESS_SRC := $(HAL_DIR)/src/core/pic16f193x_harness_sim_target.c
  # HARNESS=sim exercises the Timer1 driver (the §4 gate's load-bearing
  # peripheral for Phase 0). The Timer1 example's main loop halts in a
  # tight `for(;;)` after pic8_harness_report() so the HARNESS=sim
  # marker's RA0 stays set across the mdb `print PORTA` readback; the
  # default real-target example (example_blink) returns the report's
  # result and re-enters main via `ljmp start`, which flickers RA0 in
  # the simulator and breaks the readback.
  # SIM_APP selects which diagnostic example HARNESS=sim builds.
  # Default stays example_timer1.c so Timer1's own MODE=gpio gate keeps
  # working unchanged; override per invocation for a different
  # peripheral's gate, e.g. make ... HARNESS=sim SIM_APP=example_timer246.c
  SIM_APP     ?= example_timer1.c
  APP_SOURCES := $(HAL_DIR)/tests/$(SIM_APP)
  # WDTE=OFF only for this variant, see the config-word recipe below
  # for why: real-target firmware genuinely needs the watchdog
  # running unattended; a diagnostic build that terminates and
  # reports does not, and PIC16F193X's WDTE is a config-word fuse
  # burned at program time, not a runtime-togglable bit, so this is
  # the only place it can be turned off for this variant.
  CONFIG_WDTE := OFF
else
  HARNESS_SRC := $(COMMON_DIR)/src/core/pic8_harness_target.c
  # example_blink is the canonical "turn an LED on" smoke test for the
  # real-target (HARNESS=target, default) build. Replace it with your
  # project's main.c.
  APP_SOURCES := $(HAL_DIR)/tests/example_blink.c
  CONFIG_WDTE := ON
endif

# Enhanced Mid-range DFP. Override with make DFP_DIR=/path/to/.../xc8.
DFP_DIR ?= /opt/microchip/xc8/v3.10/pic/packs/Microchip.PIC12-16F1xxx_DFP/xc8
ifneq ($(strip $(DFP_DIR)),)
  DFP_FLAG := -mdfp=$(DFP_DIR)
else
  DFP_FLAG :=
endif

CC     := xc8-cc
# include/target ahead of include so pic16f193x_platform.h resolves to the
# real-target (volatile-dereference) version, not the host memory-backed one.
INC    := -I$(HAL_DIR)/include/target -I$(HAL_DIR)/include -I$(COMMON_DIR)/include
CFLAGS := $(DFP_FLAG) \
          -mcpu=$(shell echo $(MCU) | tr A-Z a-z) \
          -O2 -std=c99 -Wall -Wextra \
          $(CFLAGS_DEVICE) \
          $(INC) \
          -DFOSC_HZ=$(FOSC_HZ)

# ─────────────────────────── source layout ─────────────────────────
# The HAL sources in src/ compile for both host (sim) and target. The
# sim/ subdirectory is NOT compiled on real targets (XC8 never links
# pic16f193x_sim.c). The host/target SFR mapping is chosen by the include
# path (include/target first); the host-only harness / wdt_sleep-sim
# implementations are simply absent from this source list. The
# family-blind harness target impl lives in pic8-common.

HAL_SOURCES := \
  $(HAL_DIR)/src/peripherals/pic16f193x_gpio.c     \
  $(HAL_DIR)/src/peripherals/pic16f193x_timer0.c   \
  $(HAL_DIR)/src/peripherals/pic16f193x_timer1.c   \
  $(HAL_DIR)/src/peripherals/pic16f193x_timer246.c \
  $(HAL_DIR)/src/peripherals/pic16f193x_ccp.c \
  $(HAL_DIR)/src/core/pic16f193x_irq.c             \
  $(HAL_DIR)/src/core/pic16f193x_wdt_sleep.c       \
  $(HAL_DIR)/src/core/pic16f193x_wdt_sleep_target.c \
  $(HAL_DIR)/src/core/pic16f193x_isr_vector.c      \
  $(HAL_DIR)/src/core/pic16f193x_irq_dispatch.c    \
  $(HARNESS_SRC)

# All real .c sources (the config word is generated separately, below).
SRCS := $(HAL_SOURCES) $(APP_SOURCES)
OBJS := $(patsubst %.c,$(BUILD_DIR)/%.p1,$(notdir $(SRCS)))
VPATH := $(sort $(dir $(SRCS)))

# ─────────────────────────── config word ───────────────────────────
# PIC16F193X has two config words: CONFIG1 (0x2007) and CONFIG2 (0x2008),
# DS41364B §10.0, Registers 10-1 / 10-2. Adjust per your hardware.
CONFIG_SRC := $(BUILD_DIR)/config_$(MCU).c
CONFIG_OBJ := $(BUILD_DIR)/config_$(MCU).p1

# ─────────────────────────── shared build rules ────────────────────
include $(COMMON_DIR)/mk/pic8_family.mk

# Family-specific: the PIC16F193X config-word directives.
$(CONFIG_SRC): | $(BUILD_DIR)
	@printf '/* Auto-generated. Do not edit. */\n' > $@
	@printf '#include <xc.h>\n' >> $@
	@printf '#pragma config FOSC = INTOSC\n' >> $@
	@printf '#pragma config WDTE = $(CONFIG_WDTE)\n' >> $@
	@printf '#pragma config PWRTE = ON\n' >> $@
	@printf '#pragma config MCLRE = ON\n' >> $@
	@printf '#pragma config CP = OFF\n' >> $@
	@printf '#pragma config CPD = OFF\n' >> $@
	@printf '#pragma config BOREN = ON\n' >> $@
	@printf '#pragma config CLKOUTEN = OFF\n' >> $@
	@printf '#pragma config IESO = OFF\n' >> $@
	@printf '#pragma config FCMEN = OFF\n' >> $@
	@printf '#pragma config LVP = OFF\n' >> $@
	@printf '#pragma config STVREN = ON\n' >> $@
	@printf '#pragma config PLLEN = OFF\n' >> $@
	@printf '#pragma config WRT = OFF\n' >> $@
