# pic8-tick, MPLAB XC8 Makefile -- PIC18F2455 family variant.
#
# Builds the SAME pic8-tick timebase + example_tick smoke against the
# pic18fxx5x-hal. pic8_tick.c is family-agnostic and builds unmodified
# against either family; only this Makefile's EPIC_DIR, device list, DFP,
# source list, and config word differ from the PIC16 variant.
#
# Tested with MPLAB X v6.x, XC8 v3.10, PIC18F4550.
#
#   export PATH=$PATH:/opt/microchip/xc8/v3.10/bin
#   make MCU=18F4550          # default; also 2455 / 2550 / 4455
#   make clean
#
# Thin caller of the shared pic8_family.mk fragment (pic8-common/mk/).

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

ifeq ($(filter $(MCU),18F2550 18F4550),$(MCU))
  HAS_BLOCK3 := 1
else
  HAS_BLOCK3 :=
endif

# ─────────────────────────── paths ────────────────────────────────
EPIC_DIR    ?= ../../../pic18fxx5x-hal
COMMON_DIR ?= $(EPIC_DIR)/../pic8-common
APP_DIR    ?= ../..
BUILD_DIR  ?= build
TARGET     := $(BUILD_DIR)/$(MCU)-tick

DFP_DIR ?= /opt/microchip/xc8/v3.10/pic/packs/Microchip.PIC18Fxxxx_DFP/xc8
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
  HARNESS_SRC := $(EPIC_DIR)/src/core/pic18_harness_sim_target.c
  # WDT=OFF only for this variant, mirroring
  # pic16f87xa-tick-mplabx/Makefile's own CONFIG_WDTE: real-target
  # firmware genuinely needs the watchdog running unattended; a
  # diagnostic build that terminates and reports over USART does not,
  # and this Makefile's own WDT=ON default has nothing petting it, only
  # a fixed cycle count bounding the harness. Applied by the same
  # reasoning as PIC16's fix, not because it was confirmed to be the
  # cause of any specific observed symptom here (it wasn't: the sim-test
  # hang has a separate, deeper cause, see docs/ci-plan.md Phase 4's
  # PIC18 follow-up and pic18fxx5x-hal/docs/ARCHITECTURE.md).
  CONFIG_WDT := OFF
else
  HARNESS_SRC := $(COMMON_DIR)/src/core/pic8_harness_target.c
  CONFIG_WDT := ON
endif

CC     := xc8-cc
INC    := -I$(EPIC_DIR)/include/target -I$(EPIC_DIR)/include -I$(COMMON_DIR)/include -I$(APP_DIR)/include
CFLAGS := $(DFP_FLAG) \
          -mcpu=$(shell echo $(MCU) | tr A-Z a-z) \
          -O2 -std=c99 -Wall -Wextra \
          $(CFLAGS_DEVICE) \
          $(INC) \
          -DFOSC_HZ=$(FOSC_HZ)

# ─────────────────────────── source layout ─────────────────────────
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
  $(HARNESS_SRC)

TICK_SOURCES := $(APP_DIR)/src/pic8_tick.c
APP_SOURCES  := $(APP_DIR)/examples/example_tick.c

SRCS := $(EPIC_SOURCES) $(TICK_SOURCES) $(APP_SOURCES)
OBJS := $(patsubst %.c,$(BUILD_DIR)/%.p1,$(notdir $(SRCS)))
VPATH := $(sort $(dir $(SRCS)))

# ─────────────────────────── config word ───────────────────────────
# PIC18F4550-family config (DS39632E §23.1).
CONFIG_SRC := $(BUILD_DIR)/config_$(MCU).c
CONFIG_OBJ := $(BUILD_DIR)/config_$(MCU).p1

include $(COMMON_DIR)/mk/pic8_family.mk

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
	@printf '#pragma config WDT = $(CONFIG_WDT)\n' >> $@
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
