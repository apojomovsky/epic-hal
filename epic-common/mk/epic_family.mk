# Shared Makefile fragment for every 8-bit PIC HAL family's XC8 build.
#
# A family's MCU Makefile sets the family-specific variables (device
# selection, include paths, the source list, the config-word directives)
# and then `include`s this file to pull in the boilerplate that is identical
# across families: the XC8 v3.x device-family-pack flag, the `.c` -> `.p1`
# p-code pattern rule, VPATH source resolution, the generated config-word
# translation unit, the single link step to Intel HEX, and clean.
#
# Variables the caller sets before including this file:
#   CC          xc8-cc (or override for a wrapper)
#   CFLAGS      compiler flags (device, -mcpu, includes, defines, -O2 ...)
#   BUILD_DIR   build output directory
#   TARGET      hex base path, e.g. $(BUILD_DIR)/$(MCU)-firmware
#   SRCS        all real .c sources (HAL + app, NOT the generated config)
#   OBJS        the .p1 set, typically $(patsubst %.c,$(BUILD_DIR)/%.p1,$(notdir $(SRCS)))
#   VPATH       dirs to resolve basenames in SRCS back to real paths
#   CONFIG_SRC  generated config-word .c path, e.g. $(BUILD_DIR)/config_$(MCU).c
#   CONFIG_OBJ  generated config-word .p1 path
#
# The caller sets all of the above before `include`-ing this file: make
# expands explicit-rule prerequisites at parse time, so OBJS / CONFIG_OBJ
# / TARGET must already be defined when these rules are read. The caller
# also provides the recipe that generates $(CONFIG_SRC) (the
# `#pragma config` directives are family-specific: PIC16 has one config
# word, PIC18 has several with unrelated fields) after the include, so
# `all` (defined here) stays the default goal.
#
# Not shared here: the XC8 v3.x Device Family Pack flag. DFP_DIR differs
# per family (PIC16Fxxx_DFP vs PIC18Fxxxx_DFP), so each family Makefile
# derives its own DFP_FLAG before CFLAGS.

# ─────────────────────────── targets ──────────────────────────────
.PHONY: all clean
all: $(TARGET).hex

$(BUILD_DIR):
	mkdir -p $@

# Compile a HAL/app source to p-code. VPATH resolves the basename to the
# real path. The order-only prereq ensures build/ exists, xc8-cc refuses
# to write into a missing output directory.
$(BUILD_DIR)/%.p1: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# The generated config word is its own translation unit.
$(CONFIG_OBJ): $(CONFIG_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Single link step over all .p1 intermediates -> Intel HEX.
$(TARGET).hex: $(OBJS) $(CONFIG_OBJ)
	$(CC) $(CFLAGS) $(OBJS) $(CONFIG_OBJ) -o $@ -ginhx32
	@echo ""
	@echo "Built $@, program with MPLAB X or your favorite programmer."

clean:
	rm -rf $(BUILD_DIR)