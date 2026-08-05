# epic-adcfilter, MPLAB XC8 Makefile -- PIC18F2455 cross-compile sanity check.
#
# Same as the PIC16 variant: epic_adcfilter.c has no hardware dependency.
# Correctness is fully proven by the host suite. This build catches non-
# portable C and reports footprint. No HAL, no config word.
#
#   export PATH=$PATH:/opt/microchip/xc8/v3.10/bin
#   make MCU=18F4550
#   make clean

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
  $(error Unsupported MCU=$(MCU); choose 2455, 2550, 4455 or 4550)
endif

APP_DIR   ?= ../..
BUILD_DIR ?= build
TARGET    := $(BUILD_DIR)/$(MCU)-adcfilter-sizecheck

DFP_DIR ?= /opt/microchip/xc8/v3.10/pic/packs/Microchip.PIC18Fxxxx_DFP/xc8
ifneq ($(strip $(DFP_DIR)),)
  DFP_FLAG := -mdfp=$(DFP_DIR)
else
  DFP_FLAG :=
endif

CC     := xc8-cc
INC    := -I$(APP_DIR)/include
CFLAGS := $(DFP_FLAG) -mcpu=$(shell echo $(MCU) | tr A-Z a-z) \
          -O2 -std=c99 -Wall -Wextra $(CFLAGS_DEVICE) $(INC)

SRCS  := $(APP_DIR)/src/epic_adcfilter.c $(APP_DIR)/mcu/target_sizecheck.c
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