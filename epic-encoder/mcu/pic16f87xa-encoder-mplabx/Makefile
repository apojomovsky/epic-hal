# epic-encoder, MPLAB XC8 Makefile -- PIC16F87XA family cross-compile
# sanity check + footprint.
#
# encoder.c has no per-family variant (a pure Gray-code transition-table
# decoder; see docs/ARCHITECTURE.md): correctness is fully proven by the
# host suite (../../tests/test_encoder.c), which tests the exact same
# source file this Makefile compiles. This build exists to (a) catch any
# accidentally non-portable C the host compiler let through, and (b) report
# real flash/RAM footprint on the smallest-RAM part in the family
# (PIC16F873A, 192 B RAM) via `make size`. It links the now-extended HAL
# (the RB-change hook added in Phase 0a), epic-tick (the glitch-gate
# timebase), and encoder.c -- the same set an on-target application using
# a quadrature encoder off RB<7:4> would link.
#
#   export PATH=$PATH:/opt/microchip/xc8/v3.10/bin
#   make MCU=16F877A            # default; also 873A / 874A / 876A
#   make size
#   make clean

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
EPIC_DIR    ?= ../../../pic16f87xa-hal
TICK_DIR   ?= $(EPIC_DIR)/../epic-tick
COMMON_DIR ?= $(EPIC_DIR)/../epic-common
APP_DIR    ?= ../..
BUILD_DIR  ?= build
TARGET     := $(BUILD_DIR)/$(MCU)-encoder-sizecheck

DFP_DIR ?= /opt/microchip/xc8/v3.10/pic/packs/Microchip.PIC16Fxxx_DFP/xc8
ifneq ($(strip $(DFP_DIR)),)
  DFP_FLAG := -mdfp=$(DFP_DIR)
else
  DFP_FLAG :=
endif

CC     := xc8-cc
INC    := -I$(EPIC_DIR)/include/target -I$(EPIC_DIR)/include -I$(COMMON_DIR)/include \
          -I$(TICK_DIR)/include -I$(APP_DIR)/include
CFLAGS := $(DFP_FLAG) \
          -mcpu=$(shell echo $(MCU) | tr A-Z a-z) \
          -O2 -std=c99 -Wall -Wextra \
          $(CFLAGS_DEVICE) \
          $(INC) \
          -DFOSC_HZ=$(FOSC_HZ)

# ─────────────────────────── source layout ─────────────────────────
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

ifeq ($(filter $(MCU),16F874A 16F877A),$(MCU))
  EPIC_SOURCES += $(EPIC_DIR)/src/peripherals/pic16f87xa_psp.c
endif

# epic-tick (the glitch-gate timebase; same source builds on either family).
TICK_SOURCES := $(TICK_DIR)/src/epic_tick.c

# epic-encoder (family-agnostic; the Gray-code decode + atomic read).
ENCODER_SOURCES := $(APP_DIR)/src/encoder.c

# The application: a deliberately minimal standalone program that exercises
# encoder_init/update/get_position so the linker pulls the bodies in and
# reports footprint. Not a correctness test (the host suite is).
APP_SOURCES := $(APP_DIR)/mcu/target_sizecheck.c

SRCS := $(EPIC_SOURCES) $(TICK_SOURCES) $(ENCODER_SOURCES) $(APP_SOURCES)
OBJS := $(patsubst %.c,$(BUILD_DIR)/%.p1,$(notdir $(SRCS)))
VPATH := $(sort $(dir $(SRCS)))

# ─────────────────────────── config word ───────────────────────────
# PIC16F87XA Configuration Word (DS39582B §14.1). Adjust per your hardware.
CONFIG_SRC := $(BUILD_DIR)/config_$(MCU).c
CONFIG_OBJ := $(BUILD_DIR)/config_$(MCU).p1

include $(COMMON_DIR)/mk/epic_family.mk

$(CONFIG_SRC): | $(BUILD_DIR)
	@printf '/* Auto-generated. Do not edit. */\n' > $@
	@printf '#include <xc.h>\n' >> $@
	@printf '#pragma config FOSC = HS\n' >> $@
	@printf '#pragma config WDTE = ON\n' >> $@
	@printf '#pragma config PWRTE = ON\n' >> $@
	@printf '#pragma config BOREN = ON\n' >> $@
	@printf '#pragma config LVP = OFF\n' >> $@
	@printf '#pragma config WRT = OFF\n' >> $@
