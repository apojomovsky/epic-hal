# epic-adcfilter, MPLAB XC8 Makefile -- PIC16F87XA cross-compile sanity check.
#
# epic_adcfilter.c has no hardware dependency: correctness is fully proven by
# the host suite (../../tests/test_adcfilter.c). This build catches non-
# portable C and reports flash/RAM footprint. No HAL, no config word.
#
#   export PATH=$PATH:/opt/microchip/xc8/v3.10/bin
#   make MCU=16F877A
#   make clean

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

APP_DIR   ?= ../..
BUILD_DIR ?= build
TARGET    := $(BUILD_DIR)/$(MCU)-adcfilter-sizecheck

DFP_DIR ?= /opt/microchip/xc8/v3.10/pic/packs/Microchip.PIC16Fxxx_DFP/xc8
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