# epic-pid, MPLAB XC8 Makefile -- PIC18F2455 family cross-compile sanity check.
#
# Same rationale as ../pic16f87xa-pid-mplabx/Makefile: pid.c has no
# hardware dependency and no per-family variant, so this build exists only
# to prove the exact same source the host suite tests also compiles
# cleanly with real XC8 for PIC18, and to report flash/RAM footprint.
#
# The epic-math PIC18 inline-asm backend (src/pic18/) is linked so the
# real hardware-MULWF 16x16->32 signed multiply is what pid.c calls
# (matches what an on-target application would actually link).
#
# The PIC18Fxxxx DFP is NOT bundled with XC8; install it once (see
# pic18fxx5x-hal/mcu/pic18fxx5x-mplabx/README.md) or this fails with
# `error: (2104) no device-support files found`.
#
#   export PATH=$PATH:/opt/microchip/xc8/v3.10/bin
#   make MCU=18F4550          # default; also 2455 / 2550 / 4455
#   make size
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

APP_DIR    ?= ../..
MATH_DIR   ?= $(APP_DIR)/../epic-math
BUILD_DIR  ?= build
TARGET     := $(BUILD_DIR)/$(MCU)-pid-sizecheck

DFP_DIR ?= /opt/microchip/xc8/v3.10/pic/packs/Microchip.PIC18Fxxxx_DFP/xc8
ifneq ($(strip $(DFP_DIR)),)
  DFP_FLAG := -mdfp=$(DFP_DIR)
else
  DFP_FLAG :=
endif

CC     := xc8-cc
INC    := -I$(APP_DIR)/include -I$(MATH_DIR)/include
CFLAGS := $(DFP_FLAG) -mcpu=$(shell echo $(MCU) | tr A-Z a-z) \
          -O2 -std=c99 -Wall -Wextra $(CFLAGS_DEVICE) $(INC)

SRCS  := $(MATH_DIR)/src/pic18/pic_math_mul.c          \
         $(MATH_DIR)/src/pic18/pic_math_div.c          \
         $(MATH_DIR)/src/pic18/pic_math_addsub.c       \
         $(MATH_DIR)/src/pic18/pic_math_bcd.c          \
         $(MATH_DIR)/src/common/pic_math_sqrt.c        \
         $(MATH_DIR)/src/common/pic_math_numeric.c     \
         $(MATH_DIR)/src/common/pic_math_rand.c        \
         $(APP_DIR)/src/pid.c                          \
         $(APP_DIR)/mcu/target_sizecheck.c
OBJS  := $(patsubst %.c,$(BUILD_DIR)/%.p1,$(notdir $(SRCS)))
VPATH := $(sort $(dir $(SRCS)))

.PHONY: all size clean
all: $(TARGET).hex

$(BUILD_DIR):
	mkdir -p $@

$(BUILD_DIR)/%.p1: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET).hex: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ -ginhx32

size: $(TARGET).hex

clean:
	rm -rf $(BUILD_DIR)
