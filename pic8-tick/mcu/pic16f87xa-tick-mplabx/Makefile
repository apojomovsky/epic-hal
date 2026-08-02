# pic8-tick, MPLAB XC8 Makefile -- PIC16F87XA family variant.
#
# Builds the pic8-tick timebase + the example_tick smoke on real silicon via
# MPLAB XC8. Tested with MPLAB X v6.x, XC8 v3.10, PIC16F877A (and 873A /
# 874A / 876A). Mirrors pic8-taskmgr's mcu/pic16f87xa-taskmgr-mplabx/Makefile;
# only the device list, source list, and app differ.
#
#   export PATH=$PATH:/opt/microchip/xc8/v3.10/bin
#   make MCU=16F877A            # default; also 873A / 874A / 876A
#   make clean
#
# This file is a thin caller of the shared pic8_family.mk fragment
# (pic8-common/mk/): only what is tick/PIC16-specific stays here.

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
HAL_DIR    ?= ../../../pic16f87xa-hal
COMMON_DIR ?= $(HAL_DIR)/../pic8-common
APP_DIR    ?= ../..
BUILD_DIR  ?= build
TARGET     := $(BUILD_DIR)/$(MCU)-tick

DFP_DIR ?= /opt/microchip/xc8/v3.10/pic/packs/Microchip.PIC16Fxxx_DFP/xc8
ifneq ($(strip $(DFP_DIR)),)
  DFP_FLAG := -mdfp=$(DFP_DIR)
else
  DFP_FLAG :=
endif

# HARNESS=target (default): real-target harness, infinite loop, no
# stdout, unchanged from before Phase 3. HARNESS=sim: bounded harness
# that reports PASS/FAIL over the USART, for driving under MPLAB SIM
# (docs/ci-plan.md Phase 3); needs its own TARGET suffix so a sim build
# doesn't clobber a target build's .hex in the same build/ dir.
HARNESS ?= target
ifeq ($(HARNESS),sim)
  TARGET     := $(BUILD_DIR)/$(MCU)-tick-sim
  HARNESS_SRC := $(HAL_DIR)/src/core/pic16_harness_sim_target.c
else
  HARNESS_SRC := $(COMMON_DIR)/src/core/pic8_harness_target.c
endif

CC     := xc8-cc
INC    := -I$(HAL_DIR)/include/target -I$(HAL_DIR)/include -I$(COMMON_DIR)/include -I$(APP_DIR)/include
CFLAGS := $(DFP_FLAG) \
          -mcpu=$(shell echo $(MCU) | tr A-Z a-z) \
          -O2 -std=c99 -Wall -Wextra \
          $(CFLAGS_DEVICE) \
          $(INC) \
          -DFOSC_HZ=$(FOSC_HZ)

# ─────────────────────────── source layout ─────────────────────────
HAL_SOURCES := \
  $(HAL_DIR)/src/peripherals/pic16f87xa_gpio.c     \
  $(HAL_DIR)/src/peripherals/pic16f87xa_timer0.c   \
  $(HAL_DIR)/src/peripherals/pic16f87xa_timer1.c   \
  $(HAL_DIR)/src/peripherals/pic16f87xa_timer2.c   \
  $(HAL_DIR)/src/peripherals/pic16f87xa_ccp.c      \
  $(HAL_DIR)/src/peripherals/pic16f87xa_usart.c    \
  $(HAL_DIR)/src/peripherals/pic16f87xa_ssp.c      \
  $(HAL_DIR)/src/peripherals/pic16f87xa_adc.c      \
  $(HAL_DIR)/src/peripherals/pic16f87xa_comp.c     \
  $(HAL_DIR)/src/peripherals/pic16f87xa_vref.c     \
  $(HAL_DIR)/src/peripherals/pic16f87xa_eeprom.c   \
  $(HAL_DIR)/src/core/pic16_irq.c                  \
  $(HAL_DIR)/src/core/pic16f87xa_wdt_sleep.c       \
  $(HAL_DIR)/src/core/pic16f87xa_wdt_sleep_target.c \
  $(HAL_DIR)/src/core/pic16_isr_vector.c          \
  $(HAL_DIR)/src/core/pic16_irq_dispatch.c        \
  $(HARNESS_SRC)

ifeq ($(filter $(MCU),16F874A 16F877A),$(MCU))
  HAL_SOURCES += $(HAL_DIR)/src/peripherals/pic16f87xa_psp.c
endif

# pic8-tick itself (family-agnostic; same source builds on either family).
TICK_SOURCES := $(APP_DIR)/src/pic8_tick.c

# The application: the tick smoke (delays, reports PASS/FAIL).
APP_SOURCES := $(APP_DIR)/examples/example_tick.c

SRCS := $(HAL_SOURCES) $(TICK_SOURCES) $(APP_SOURCES)
OBJS := $(patsubst %.c,$(BUILD_DIR)/%.p1,$(notdir $(SRCS)))
VPATH := $(sort $(dir $(SRCS)))

# ─────────────────────────── config word ───────────────────────────
# PIC16F87XA Configuration Word (DS39582B §14.1). Adjust per your hardware.
CONFIG_SRC := $(BUILD_DIR)/config_$(MCU).c
CONFIG_OBJ := $(BUILD_DIR)/config_$(MCU).p1

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
