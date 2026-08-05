# epic-settings, MPLAB XC8 Makefile -- PIC18F2455 family cross-compile sanity check.

MCU     ?= 18F4550

ifeq ($(MCU),18F2455)
  CFLAGS_DEVICE := -DPIC18F2455
else ifeq ($(MCU),18F2550)
  CFLAGS_DEVICE := -DPIC18F2550
else ifeq ($(MCU),18F4455)
  CFLAGS_DEVICE := -DPIC18F4455
else ifeq ($(MCU),18F4550)
  CFLAGS_DEVICE := -DPIC18F4550
else
  $(error Unsupported MCU=$(MCU); choose 18F2455, 18F2550, 18F4455 or 18F4550)
endif

EPIC_DIR    ?= ../../../pic18fxx5x-hal
COMMON_DIR ?= $(EPIC_DIR)/../epic-common
APP_DIR    ?= ../..
BUILD_DIR  ?= build
TARGET     := $(BUILD_DIR)/$(MCU)-settings-sizecheck

DFP_DIR ?= /opt/microchip/mplabx/v6.25/packs/Microchip/PIC18Fxxxx_DFP/1.6.161/xc8
ifneq ($(strip $(DFP_DIR)),)
  DFP_FLAG := -mdfp=$(DFP_DIR)
else
  DFP_FLAG :=
endif

CC     := xc8-cc
INC    := -I$(EPIC_DIR)/include/target -I$(EPIC_DIR)/include -I$(COMMON_DIR)/include -I$(APP_DIR)/include
CFLAGS := $(DFP_FLAG) -mcpu=$(shell echo $(MCU) | tr A-Z a-z) \
          -O2 -std=c99 -Wall -Wextra $(CFLAGS_DEVICE) $(INC)

EPIC_SOURCES := \
  $(EPIC_DIR)/src/peripherals/pic18fxx5x_eeprom.c     \
  $(EPIC_DIR)/src/core/pic18_irq.c                    \
  $(EPIC_DIR)/src/core/pic18fxx5x_wdt_sleep.c         \
  $(EPIC_DIR)/src/core/pic18fxx5x_wdt_sleep_target.c  \
  $(EPIC_DIR)/src/core/pic18_isr_vector.c             \
  $(EPIC_DIR)/src/core/pic18_irq_dispatch.c           \
  $(COMMON_DIR)/src/core/epic_harness_target.c

SRCS  := $(EPIC_SOURCES) $(APP_DIR)/src/epic_settings.c $(APP_DIR)/mcu/target_sizecheck.c
OBJS  := $(patsubst %.c,$(BUILD_DIR)/%.p1,$(notdir $(SRCS)))
VPATH := $(sort $(dir $(SRCS)))

.PHONY: all clean
all: $(TARGET).hex

$(BUILD_DIR):
	mkdir -p $@

$(BUILD_DIR)/%.p1: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET).hex: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ -ginhx32

clean:
	rm -rf $(BUILD_DIR)
