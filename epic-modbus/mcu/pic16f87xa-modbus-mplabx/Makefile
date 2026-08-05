# epic-modbus, MPLAB XC8 Makefile -- PIC16F87XA family variant.
#
# Builds the Modbus RTU slave + its epic-serial (UART) and epic-tick (T3.5
# timebase) dependencies + the example on real silicon. Mirrors
# epic-debounce's Makefile (which adds a TICK_DIR the same way); here both
# SERIAL_DIR and TICK_DIR are added alongside the module's own sources.
#
#   export PATH=$PATH:/opt/microchip/xc8/v3.10/bin
#   make MCU=16F877A            # default; also 873A / 874A / 876A
#   make clean
#
# Thin caller of the shared epic_family.mk fragment (epic-common/mk/).

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
COMMON_DIR ?= $(EPIC_DIR)/../epic-common
APP_DIR    ?= ../..
SERIAL_DIR ?= $(APP_DIR)/../epic-serial
TICK_DIR   ?= $(APP_DIR)/../epic-tick
BUILD_DIR  ?= build
TARGET     := $(BUILD_DIR)/$(MCU)-modbus

DFP_DIR ?= /opt/microchip/xc8/v3.10/pic/packs/Microchip.PIC16Fxxx_DFP/xc8
ifneq ($(strip $(DFP_DIR)),)
  DFP_FLAG := -mdfp=$(DFP_DIR)
else
  DFP_FLAG :=
endif

CC     := xc8-cc
INC    := -I$(EPIC_DIR)/include/target -I$(EPIC_DIR)/include -I$(COMMON_DIR)/include \
          -I$(SERIAL_DIR)/include -I$(TICK_DIR)/include -I$(APP_DIR)/include
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
  $(COMMON_DIR)/src/core/epic_harness_target.c

ifeq ($(filter $(MCU),16F874A 16F877A),$(MCU))
  EPIC_SOURCES += $(EPIC_DIR)/src/peripherals/pic16f87xa_psp.c
endif

# epic-serial (UART) + epic-tick (T3.5 timebase) + epic-modbus (the module) + the example.
SERIAL_SOURCES := $(SERIAL_DIR)/src/epic_serial.c
TICK_SOURCES   := $(TICK_DIR)/src/epic_tick.c
MODBUS_SOURCES := $(APP_DIR)/src/epic_modbus.c
APP_SOURCES    := $(APP_DIR)/examples/example_modbus_target.c

SRCS := $(EPIC_SOURCES) $(SERIAL_SOURCES) $(TICK_SOURCES) $(MODBUS_SOURCES) $(APP_SOURCES)
OBJS := $(patsubst %.c,$(BUILD_DIR)/%.p1,$(notdir $(SRCS)))
VPATH := $(sort $(dir $(SRCS)))

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
