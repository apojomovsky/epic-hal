# pic8-settings, MPLAB XC8 Makefile -- PIC16F87XA cross-compile sanity check.
#
# Builds the family-agnostic settings module against the PIC16 HAL and reports
# footprint. Host tests prove behavior; this catches XC8 portability issues.

MCU     ?= 16F877A

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
BUILD_DIR  ?= build
TARGET     := $(BUILD_DIR)/$(MCU)-settings-sizecheck

DFP_DIR ?= /opt/microchip/xc8/v3.10/pic/packs/Microchip.PIC16Fxxx_DFP/xc8
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
  $(EPIC_DIR)/src/peripherals/pic16f87xa_eeprom.c     \
  $(EPIC_DIR)/src/core/pic16_irq.c                    \
  $(EPIC_DIR)/src/core/pic16f87xa_wdt_sleep.c         \
  $(EPIC_DIR)/src/core/pic16f87xa_wdt_sleep_target.c  \
  $(EPIC_DIR)/src/core/pic16_isr_vector.c             \
  $(EPIC_DIR)/src/core/pic16_irq_dispatch.c           \
  $(COMMON_DIR)/src/core/pic8_harness_target.c

SRCS  := $(EPIC_SOURCES) $(APP_DIR)/src/pic8_settings.c $(APP_DIR)/mcu/target_sizecheck.c
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
