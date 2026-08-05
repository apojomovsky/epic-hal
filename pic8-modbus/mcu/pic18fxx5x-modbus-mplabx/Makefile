# pic8-modbus, MPLAB XC8 Makefile -- PIC18F2455 family variant.
#
# Builds the SAME pic8-modbus RTU slave + its pic8-serial (UART) and
# pic8-tick (T3.5 timebase) dependencies against pic18fxx5x-hal.
# pic8_modbus.c is family-agnostic; only this Makefile's EPIC_DIR, device
# list, DFP, source list, and config word differ from the PIC16 variant.
#
#   export PATH=$PATH:/opt/microchip/xc8/v3.10/bin
#   make MCU=18F4550          # default; also 2455 / 2550 / 4455
#   make clean
#
# Thin caller of the shared pic8_family.mk fragment (pic8-common/mk/).

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

EPIC_DIR    ?= ../../../pic18fxx5x-hal
COMMON_DIR ?= $(EPIC_DIR)/../pic8-common
APP_DIR    ?= ../..
SERIAL_DIR ?= $(APP_DIR)/../pic8-serial
TICK_DIR   ?= $(APP_DIR)/../pic8-tick
BUILD_DIR  ?= build
TARGET     := $(BUILD_DIR)/$(MCU)-modbus

DFP_DIR ?= /opt/microchip/xc8/v3.10/pic/packs/Microchip.PIC18Fxxxx_DFP/xc8
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
  $(COMMON_DIR)/src/core/pic8_harness_target.c

# pic8-serial (UART) + pic8-tick (T3.5 timebase) + pic8-modbus (the module) + the example.
SERIAL_SOURCES := $(SERIAL_DIR)/src/pic8_serial.c
TICK_SOURCES   := $(TICK_DIR)/src/pic8_tick.c
MODBUS_SOURCES := $(APP_DIR)/src/pic8_modbus.c
APP_SOURCES    := $(APP_DIR)/examples/example_modbus_target.c

SRCS := $(EPIC_SOURCES) $(SERIAL_SOURCES) $(TICK_SOURCES) $(MODBUS_SOURCES) $(APP_SOURCES)
OBJS := $(patsubst %.c,$(BUILD_DIR)/%.p1,$(notdir $(SRCS)))
VPATH := $(sort $(dir $(SRCS)))

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
