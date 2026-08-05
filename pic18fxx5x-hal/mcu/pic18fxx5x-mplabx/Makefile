# PIC18F2455 family HAL, MPLAB XC8 Makefile
#
# Builds the HAL on real silicon via MPLAB XC8. Tested with:
#   - MPLAB X IDE v6.x
#   - MPLAB XC8 C Compiler v3.10 (the `xc8-cc` driver)
#   - PIC18F4550 (default; also 2455 / 2550 / 4455)
#
# XC8 v3.x notes:
#   - The `xc8-cc` driver compiles each translation unit to a `.p1`
#     (p-code) intermediate, NOT a `.o`; the final hex comes from a
#     single link step over the `.p1` set with `-ginhx32`.
#   - v3.x moved device support into a Device Family Pack, so the
#     compiler needs `-mdfp <dir>`. Override DFP_DIR below (or pass
#     make DFP_DIR=/path/to/dfp) if your install lives elsewhere. The
#     PIC18F DFP is `Microchip.PIC18Fxxxx_DFP`; it is NOT shipped with
#     XC8 and must be installed via MPLAB X's Pack Manager or downloaded
#     from packs.download.microchip.com (see README.md).
#
# From the command line (no IDE):
#
#   export PATH=$PATH:/opt/microchip/xc8/v3.10/bin
#   make MCU=18F4550          # default; also 2455 / 2550 / 4455
#   make clean
#
# To program the device with PICkit 2/3/4 or ICD, open MPLAB X
# and use the "Make and Program" button. The .hex output here
# is suitable for any external programmer (PK2CMD, ippe, MPLAB IPE).
#
# This file is a thin caller of the shared epic_family.mk fragment
# (epic-common/mk/): the `.c` -> `.p1` pattern rule, the `.hex` link
# step, and clean all live there. Only what is genuinely PIC18F2455-
# specific stays here: device selection, the DFP flag, the source list,
# the include path, and the configuration-word directives.
#
# Phase 1 status: the skeleton has no peripheral drivers yet, so the
# target source list is the family-blind harness target impl, the shared
# interrupt dispatch (empty body in Phase 1), and example_smoke as the
# application. The ISR vector file (pic18_isr_vector.c, using the
# __interrupt(high_priority)/__interrupt(low_priority) syntax confirmed
# in docs/multi-family-plan.md) lands in Phase 2 with the first real
# interrupt source.

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
# CP3 / WRT3 / EBTR3 config setting; the 24 KB parts (18F2455 / 18F4455)
# stop at block 2 (DS39632E §23.1, CONFIG5H/6H/7H exist only on 32 KB).
# Gate the block-3 config directives so one recipe serves all four devices.
ifeq ($(filter $(MCU),18F2550 18F4550),$(MCU))
  HAS_BLOCK3 := 1
else
  HAS_BLOCK3 :=
endif

# SPP is only on the 40/44-pin parts (18F4455 / 18F4550). The driver header
# #errors on 28-pin parts, so its source must not be compiled there.
ifeq ($(filter $(MCU),18F4455 18F4550),$(MCU))
  HAS_SPP := 1
else
  HAS_SPP :=
endif

# ─────────────────────────── paths ────────────────────────────────
# Adjust EPIC_DIR if the firmware project is not a sibling of pic18fxx5x-hal.
# epic-common holds the shared harness target impl and headers; it is a
# sibling of the HAL tree.
EPIC_DIR    ?= ../..
COMMON_DIR ?= $(EPIC_DIR)/../epic-common
BUILD_DIR  ?= build
TARGET     := $(BUILD_DIR)/$(MCU)-firmware

# XC8 v3.x device family pack. The PIC18F DFP is not bundled with XC8;
# install it via MPLAB X Pack Manager or point DFP_DIR at a local copy.
# Empty -> the flag is omitted (use when the toolchain resolves devices
# without an explicit DFP, e.g. older XC8).
DFP_DIR ?= /opt/microchip/xc8/v3.10/pic/packs/Microchip.PIC18Fxxxx_DFP/xc8
ifneq ($(strip $(DFP_DIR)),)
  DFP_FLAG := -mdfp=$(DFP_DIR)
else
  DFP_FLAG :=
endif

CC     := xc8-cc
# include/target ahead of include so pic18_platform.h resolves to the
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
# The HAL sources in src/ compile for both host (sim) and target. The
# sim/ subdirectory is NOT compiled on real targets, XC8 never links
# pic18_sim.c. The host/target SFR mapping is chosen by the include path
# (include/target first), and the host-only harness implementation is
# simply absent from this source list. The family-blind harness target
# impl lives in epic-common and is linked here alongside the PIC18 core.
# Phase 2 MVP: GPIO, Timer0, interrupt core + ISR vector, WDT/Sleep. The
# host-only files (pic18_sim.c, pic18_harness_sim.c,
# pic18fxx5x_wdt_sleep_sim.c) are absent from this list; the host/target
# SFR mapping is chosen by the include path (include/target first).

EPIC_SOURCES := \
  $(EPIC_DIR)/src/peripherals/pic18fxx5x_gpio.c            \
  $(EPIC_DIR)/src/peripherals/pic18fxx5x_timer0.c          \
  $(EPIC_DIR)/src/peripherals/pic18fxx5x_timer1.c          \
  $(EPIC_DIR)/src/peripherals/pic18fxx5x_timer2.c          \
  $(EPIC_DIR)/src/peripherals/pic18fxx5x_timer3.c          \
  $(EPIC_DIR)/src/peripherals/pic18fxx5x_ccp.c             \
  $(EPIC_DIR)/src/peripherals/pic18fxx5x_ssp.c             \
  $(EPIC_DIR)/src/peripherals/pic18fxx5x_usart.c           \
  $(EPIC_DIR)/src/peripherals/pic18fxx5x_comp.c           \
  $(EPIC_DIR)/src/peripherals/pic18fxx5x_eeprom.c         \
  $(EPIC_DIR)/src/peripherals/pic18fxx5x_adc.c           \
  $(EPIC_DIR)/src/core/pic18_irq.c                         \
  $(EPIC_DIR)/src/core/pic18_irq_dispatch.c                \
  $(EPIC_DIR)/src/core/pic18_isr_vector.c                  \
  $(EPIC_DIR)/src/core/pic18fxx5x_wdt_sleep.c              \
  $(EPIC_DIR)/src/core/pic18fxx5x_wdt_sleep_target.c       \
  $(COMMON_DIR)/src/core/epic_harness_target.c

# SPP source only on the 40/44-pin parts that have the peripheral.
ifeq ($(HAS_SPP),1)
  EPIC_SOURCES += $(EPIC_DIR)/src/peripherals/pic18fxx5x_spp.c
endif

# Add your application sources here. example_blink is the canonical
# "turn an LED on" smoke test (Timer0 + GPIO + interrupt). Replace it with
# your project's main.c.
APP_SOURCES := $(EPIC_DIR)/tests/example_blink.c

# All real .c sources (the config word is generated separately, below).
SRCS := $(EPIC_SOURCES) $(APP_SOURCES)
# xc8-cc -c emits <name>.p1; link from the .p1 set.
OBJS := $(patsubst %.c,$(BUILD_DIR)/%.p1,$(notdir $(SRCS)))

# Tell make where to find sources referenced by basename. The pattern
# rule $(BUILD_DIR)/%.p1: %.c uses basename matching; without VPATH
# make would not resolve EPIC_DIR/.../pic18_irq_dispatch.c from the
# target name build/pic18_irq_dispatch.p1.
VPATH := $(sort $(dir $(SRCS)))

# ─────────────────────────── config word ───────────────────────────
# The PIC18F2455 family has several Configuration Words (DS39632E
# §23.1), unrelated fields unlike PIC16's single word. This file emits
# a conservative default set for a 20 MHz crystal + USB-capable board;
# adjust per your hardware. The exact `#pragma config` labels are
# defined by the PIC18F DFP's device header, so this recipe is only
# exercised once the PIC18F DFP is installed (see README.md).
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

# Family-specific: the PIC18F4550-family config-word directives. The
# shared epic_family.mk provides the rule that compiles $(CONFIG_SRC)
# to $(CONFIG_OBJ) and links it into the hex. Labels per DS39632E
# §23.1; the XC8 spelling comes from the PIC18F DFP device header.
$(CONFIG_SRC): | $(BUILD_DIR)
	@printf '/* Auto-generated. Do not edit. */\n' > $@
	@printf '#include <xc.h>\n' >> $@
	@printf '#pragma config FOSC = HS\n'        >> $@
	@printf '#pragma config PLLDIV = 5\n'       >> $@
	@printf '#pragma config CPUDIV = OSC1_PLL2\n' >> $@
	@printf '#pragma config USBDIV = 2\n'       >> $@
	@printf '#pragma config FCMEN = OFF\n'      >> $@
	@printf '#pragma config IESO = OFF\n'       >> $@
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
