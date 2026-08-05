# pic8-debounce, MPLAB XC8 Makefile -- PIC16F87XA family variant.
#
# Builds the debounce module + pic8-tick (its timebase) + the example on real
# silicon. Links the full HAL (for GPIO + Timer2) the way pic8-tick's own
# Makefile does.
#
#   export PATH=$PATH:/opt/microchip/xc8/v3.10/bin
#   make MCU=16F877A
#   make clean
#
# Thin caller of the shared pic8_family.mk fragment (pic8-common/mk/).

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

EPIC_DIR    ?= ../../../pic16f87xa-hal
COMMON_DIR ?= $(EPIC_DIR)/../pic8-common
APP_DIR    ?= ../..
TICK_DIR   ?= $(APP_DIR)/../pic8-tick
BUILD_DIR  ?= build
TARGET     := $(BUILD_DIR)/$(MCU)-debounce

DFP_DIR ?= /opt/microchip/xc8/v3.10/pic/packs/Microchip.PIC16Fxxx_DFP/xc8
ifneq ($(strip $(DFP_DIR)),)
  DFP_FLAG := -mdfp=$(DFP_DIR)
else
  DFP_FLAG :=
endif

CC     := xc8-cc
INC    := -I$(EPIC_DIR)/include/target -I$(EPIC_DIR)/include -I$(COMMON_DIR)/include \
          -I$(TICK_DIR)/include -I$(APP_DIR)/include
CFLAGS := $(DFP_FLAG) -mcpu=$(shell echo $(MCU) | tr A-Z a-z) -O2 -std=c99 -Wall -Wextra \
          $(CFLAGS_DEVICE) $(INC) -DFOSC_HZ=$(FOSC_HZ)

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

ifeq ($(filter $(MCU),16F874A 16F877A),$(MCU))
  EPIC_SOURCES += $(EPIC_DIR)/src/peripherals/pic16f87xa_psp.c
endif

# pic8-tick (the timebase) + pic8-debounce (the module) + the example.
TICK_SOURCES      := $(TICK_DIR)/src/pic8_tick.c
DEBOUNCE_SOURCES  := $(APP_DIR)/src/debounce.c
APP_SOURCES       := $(APP_DIR)/examples/example_debounce_hal.c

SRCS := $(EPIC_SOURCES) $(TICK_SOURCES) $(DEBOUNCE_SOURCES) $(APP_SOURCES)
OBJS := $(patsubst %.c,$(BUILD_DIR)/%.p1,$(notdir $(SRCS)))
VPATH := $(sort $(dir $(SRCS)))

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
