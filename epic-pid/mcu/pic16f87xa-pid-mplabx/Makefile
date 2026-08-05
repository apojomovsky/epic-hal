# epic-pid, MPLAB XC8 Makefile -- PIC16F87XA family cross-compile sanity check.
#
# pid.c has no per-family variant (Q8.8 arithmetic + epic-math calls; see
# docs/epic-pid-plan.md): correctness is already fully proven by the host
# suite (../../tests/test_pid.c), which tests the exact same source file
# this Makefile compiles. This build exists to (a) catch any accidentally
# non-portable C the host compiler let through, and (b) report real
# flash/RAM footprint on the smallest-RAM part in the family (PIC16F873A,
# 192 B RAM) via `make size`. No HAL, no config word beyond XC8's own
# defaults, no interrupt vector -- target_sizecheck.c is a deliberately
# minimal standalone program, not firmware meant to be flashed.
#
# The epic-math PIC16 inline-asm backend (src/pic16/) is linked so the
# real AN526 shift-add 16x16->32 signed multiply is what pid.c calls
# (matches what an on-target application would actually link).
#
#   export PATH=$PATH:/opt/microchip/xc8/v3.10/bin
#   make MCU=16F877A            # default; also 873A / 874A / 876A
#   make size
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

APP_DIR    ?= ../..
MATH_DIR   ?= $(APP_DIR)/../epic-math
BUILD_DIR  ?= build
TARGET     := $(BUILD_DIR)/$(MCU)-pid-sizecheck

DFP_DIR ?= /opt/microchip/xc8/v3.10/pic/packs/Microchip.PIC16Fxxx_DFP/xc8
ifneq ($(strip $(DFP_DIR)),)
  DFP_FLAG := -mdfp=$(DFP_DIR)
else
  DFP_FLAG :=
endif

CC     := xc8-cc
INC    := -I$(APP_DIR)/include -I$(MATH_DIR)/include
CFLAGS := $(DFP_FLAG) -mcpu=$(shell echo $(MCU) | tr A-Z a-z) \
          -O2 -std=c99 -Wall -Wextra $(CFLAGS_DEVICE) $(INC)

SRCS  := $(APP_DIR)/src/pid.c                         \
         $(MATH_DIR)/src/pic16/pic_math_mul.c          \
         $(MATH_DIR)/src/pic16/pic_math_div.c          \
         $(MATH_DIR)/src/pic16/pic_math_addsub.c       \
         $(MATH_DIR)/src/pic16/pic_math_bcd.c          \
         $(MATH_DIR)/src/pic16/pic_math_scratch.c      \
         $(MATH_DIR)/src/common/pic_math_sqrt.c        \
         $(MATH_DIR)/src/common/pic_math_numeric.c     \
         $(MATH_DIR)/src/common/pic_math_rand.c        \
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
