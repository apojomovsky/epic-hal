#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Include project Makefile
ifeq "${IGNORE_LOCAL}" "TRUE"
# do not include local makefile. User is passing all local related variables already
else
include Makefile
# Include makefile containing local settings
ifeq "$(wildcard nbproject/Makefile-local-default.mk)" "nbproject/Makefile-local-default.mk"
include nbproject/Makefile-local-default.mk
endif
endif

# Environment
MKDIR=mkdir -p
RM=rm -f
MV=mv
CP=cp

# Macros
CND_CONF=default
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
IMAGE_TYPE=debug
OUTPUT_SUFFIX=elf
DEBUGGABLE_SUFFIX=elf
FINAL_IMAGE=${DISTDIR}/epicurus-demo-pic16f193x.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
else
IMAGE_TYPE=production
OUTPUT_SUFFIX=hex
DEBUGGABLE_SUFFIX=elf
FINAL_IMAGE=${DISTDIR}/epicurus-demo-pic16f193x.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
endif

ifeq ($(COMPARE_BUILD), true)
COMPARISON_BUILD=-mafrlcsj
else
COMPARISON_BUILD=
endif

# Object Directory
OBJECTDIR=build/${CND_CONF}/${IMAGE_TYPE}

# Distribution Directory
DISTDIR=dist/${CND_CONF}/${IMAGE_TYPE}

# Source Files Quoted if spaced
SOURCEFILES_QUOTED_IF_SPACED=../../epic-common/src/core/epic_harness_target.c ../../pic16f193x-hal/src/core/pic16f193x_irq.c ../../pic16f193x-hal/src/core/pic16f193x_irq_dispatch.c ../../pic16f193x-hal/src/target/pic16f193x_isr_vector.c ../../pic16f193x-hal/src/core/pic16f193x_wdt_sleep.c ../../pic16f193x-hal/src/target/pic16f193x_wdt_sleep_target.c ../../pic16f193x-hal/src/peripherals/pic16f193x_adc.c ../../pic16f193x-hal/src/peripherals/pic16f193x_ccp.c ../../pic16f193x-hal/src/peripherals/pic16f193x_comp.c ../../pic16f193x-hal/src/peripherals/pic16f193x_cps.c ../../pic16f193x-hal/src/peripherals/pic16f193x_dac.c ../../pic16f193x-hal/src/peripherals/pic16f193x_eeprom.c ../../pic16f193x-hal/src/peripherals/pic16f193x_fvr.c ../../pic16f193x-hal/src/peripherals/pic16f193x_gpio.c ../../pic16f193x-hal/src/peripherals/pic16f193x_lcd.c ../../pic16f193x-hal/src/peripherals/pic16f193x_srlatch.c ../../pic16f193x-hal/src/peripherals/pic16f193x_ssp.c ../../pic16f193x-hal/src/peripherals/pic16f193x_timer0.c ../../pic16f193x-hal/src/peripherals/pic16f193x_timer1.c ../../pic16f193x-hal/src/peripherals/pic16f193x_timer246.c ../../pic16f193x-hal/src/peripherals/pic16f193x_usart.c main.c

# Object Files Quoted if spaced
OBJECTFILES_QUOTED_IF_SPACED=${OBJECTDIR}/_ext/666212198/epic_harness_target.p1 ${OBJECTDIR}/_ext/839043599/pic16f193x_irq.p1 ${OBJECTDIR}/_ext/839043599/pic16f193x_irq_dispatch.p1 ${OBJECTDIR}/_ext/839043599/pic16f193x_isr_vector.p1 ${OBJECTDIR}/_ext/839043599/pic16f193x_wdt_sleep.p1 ${OBJECTDIR}/_ext/839043599/pic16f193x_wdt_sleep_target.p1 ${OBJECTDIR}/_ext/730952199/pic16f193x_adc.p1 ${OBJECTDIR}/_ext/730952199/pic16f193x_ccp.p1 ${OBJECTDIR}/_ext/730952199/pic16f193x_comp.p1 ${OBJECTDIR}/_ext/730952199/pic16f193x_cps.p1 ${OBJECTDIR}/_ext/730952199/pic16f193x_dac.p1 ${OBJECTDIR}/_ext/730952199/pic16f193x_eeprom.p1 ${OBJECTDIR}/_ext/730952199/pic16f193x_fvr.p1 ${OBJECTDIR}/_ext/730952199/pic16f193x_gpio.p1 ${OBJECTDIR}/_ext/730952199/pic16f193x_lcd.p1 ${OBJECTDIR}/_ext/730952199/pic16f193x_srlatch.p1 ${OBJECTDIR}/_ext/730952199/pic16f193x_ssp.p1 ${OBJECTDIR}/_ext/730952199/pic16f193x_timer0.p1 ${OBJECTDIR}/_ext/730952199/pic16f193x_timer1.p1 ${OBJECTDIR}/_ext/730952199/pic16f193x_timer246.p1 ${OBJECTDIR}/_ext/730952199/pic16f193x_usart.p1 ${OBJECTDIR}/main.p1
POSSIBLE_DEPFILES=${OBJECTDIR}/_ext/666212198/epic_harness_target.p1.d ${OBJECTDIR}/_ext/839043599/pic16f193x_irq.p1.d ${OBJECTDIR}/_ext/839043599/pic16f193x_irq_dispatch.p1.d ${OBJECTDIR}/_ext/839043599/pic16f193x_isr_vector.p1.d ${OBJECTDIR}/_ext/839043599/pic16f193x_wdt_sleep.p1.d ${OBJECTDIR}/_ext/839043599/pic16f193x_wdt_sleep_target.p1.d ${OBJECTDIR}/_ext/730952199/pic16f193x_adc.p1.d ${OBJECTDIR}/_ext/730952199/pic16f193x_ccp.p1.d ${OBJECTDIR}/_ext/730952199/pic16f193x_comp.p1.d ${OBJECTDIR}/_ext/730952199/pic16f193x_cps.p1.d ${OBJECTDIR}/_ext/730952199/pic16f193x_dac.p1.d ${OBJECTDIR}/_ext/730952199/pic16f193x_eeprom.p1.d ${OBJECTDIR}/_ext/730952199/pic16f193x_fvr.p1.d ${OBJECTDIR}/_ext/730952199/pic16f193x_gpio.p1.d ${OBJECTDIR}/_ext/730952199/pic16f193x_lcd.p1.d ${OBJECTDIR}/_ext/730952199/pic16f193x_srlatch.p1.d ${OBJECTDIR}/_ext/730952199/pic16f193x_ssp.p1.d ${OBJECTDIR}/_ext/730952199/pic16f193x_timer0.p1.d ${OBJECTDIR}/_ext/730952199/pic16f193x_timer1.p1.d ${OBJECTDIR}/_ext/730952199/pic16f193x_timer246.p1.d ${OBJECTDIR}/_ext/730952199/pic16f193x_usart.p1.d ${OBJECTDIR}/main.p1.d

# Object Files
OBJECTFILES=${OBJECTDIR}/_ext/666212198/epic_harness_target.p1 ${OBJECTDIR}/_ext/839043599/pic16f193x_irq.p1 ${OBJECTDIR}/_ext/839043599/pic16f193x_irq_dispatch.p1 ${OBJECTDIR}/_ext/839043599/pic16f193x_isr_vector.p1 ${OBJECTDIR}/_ext/839043599/pic16f193x_wdt_sleep.p1 ${OBJECTDIR}/_ext/839043599/pic16f193x_wdt_sleep_target.p1 ${OBJECTDIR}/_ext/730952199/pic16f193x_adc.p1 ${OBJECTDIR}/_ext/730952199/pic16f193x_ccp.p1 ${OBJECTDIR}/_ext/730952199/pic16f193x_comp.p1 ${OBJECTDIR}/_ext/730952199/pic16f193x_cps.p1 ${OBJECTDIR}/_ext/730952199/pic16f193x_dac.p1 ${OBJECTDIR}/_ext/730952199/pic16f193x_eeprom.p1 ${OBJECTDIR}/_ext/730952199/pic16f193x_fvr.p1 ${OBJECTDIR}/_ext/730952199/pic16f193x_gpio.p1 ${OBJECTDIR}/_ext/730952199/pic16f193x_lcd.p1 ${OBJECTDIR}/_ext/730952199/pic16f193x_srlatch.p1 ${OBJECTDIR}/_ext/730952199/pic16f193x_ssp.p1 ${OBJECTDIR}/_ext/730952199/pic16f193x_timer0.p1 ${OBJECTDIR}/_ext/730952199/pic16f193x_timer1.p1 ${OBJECTDIR}/_ext/730952199/pic16f193x_timer246.p1 ${OBJECTDIR}/_ext/730952199/pic16f193x_usart.p1 ${OBJECTDIR}/main.p1

# Source Files
SOURCEFILES=../../epic-common/src/core/epic_harness_target.c ../../pic16f193x-hal/src/core/pic16f193x_irq.c ../../pic16f193x-hal/src/core/pic16f193x_irq_dispatch.c ../../pic16f193x-hal/src/target/pic16f193x_isr_vector.c ../../pic16f193x-hal/src/core/pic16f193x_wdt_sleep.c ../../pic16f193x-hal/src/target/pic16f193x_wdt_sleep_target.c ../../pic16f193x-hal/src/peripherals/pic16f193x_adc.c ../../pic16f193x-hal/src/peripherals/pic16f193x_ccp.c ../../pic16f193x-hal/src/peripherals/pic16f193x_comp.c ../../pic16f193x-hal/src/peripherals/pic16f193x_cps.c ../../pic16f193x-hal/src/peripherals/pic16f193x_dac.c ../../pic16f193x-hal/src/peripherals/pic16f193x_eeprom.c ../../pic16f193x-hal/src/peripherals/pic16f193x_fvr.c ../../pic16f193x-hal/src/peripherals/pic16f193x_gpio.c ../../pic16f193x-hal/src/peripherals/pic16f193x_lcd.c ../../pic16f193x-hal/src/peripherals/pic16f193x_srlatch.c ../../pic16f193x-hal/src/peripherals/pic16f193x_ssp.c ../../pic16f193x-hal/src/peripherals/pic16f193x_timer0.c ../../pic16f193x-hal/src/peripherals/pic16f193x_timer1.c ../../pic16f193x-hal/src/peripherals/pic16f193x_timer246.c ../../pic16f193x-hal/src/peripherals/pic16f193x_usart.c main.c



CFLAGS=
ASFLAGS=
LDLIBSOPTIONS=

############# Tool locations ##########################################
# If you copy a project from one host to another, the path where the  #
# compiler is installed may be different.                             #
# If you open this project with MPLAB X in the new host, this         #
# makefile will be regenerated and the paths will be corrected.       #
#######################################################################
# fixDeps replaces a bunch of sed/cat/printf statements that slow down the build
FIXDEPS=fixDeps

.build-conf:  ${BUILD_SUBPROJECTS}
ifneq ($(INFORMATION_MESSAGE), )
	@echo $(INFORMATION_MESSAGE)
endif
	${MAKE}  -f nbproject/Makefile-default.mk ${DISTDIR}/epicurus-demo-pic16f193x.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}

MP_PROCESSOR_OPTION=16F1937
# ------------------------------------------------------------------------------------
# Rules for buildStep: compile
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
${OBJECTDIR}/_ext/666212198/epic_harness_target.p1: ../../epic-common/src/core/epic_harness_target.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/666212198"
	@${RM} ${OBJECTDIR}/_ext/666212198/epic_harness_target.p1.d
	@${RM} ${OBJECTDIR}/_ext/666212198/epic_harness_target.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F1937 -xassembler-with-cpp -I"../../pic16f193x-hal/include/target" -I"../../pic16f193x-hal/include" -I"../../epic-common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/666212198/epic_harness_target.p1 ../../epic-common/src/core/epic_harness_target.c
	@-${MV} ${OBJECTDIR}/_ext/666212198/epic_harness_target.d ${OBJECTDIR}/_ext/666212198/epic_harness_target.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/666212198/epic_harness_target.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/839043599/pic16f193x_irq.p1: ../../pic16f193x-hal/src/core/pic16f193x_irq.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/839043599"
	@${RM} ${OBJECTDIR}/_ext/839043599/pic16f193x_irq.p1.d
	@${RM} ${OBJECTDIR}/_ext/839043599/pic16f193x_irq.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F1937 -xassembler-with-cpp -I"../../pic16f193x-hal/include/target" -I"../../pic16f193x-hal/include" -I"../../epic-common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/839043599/pic16f193x_irq.p1 ../../pic16f193x-hal/src/core/pic16f193x_irq.c
	@-${MV} ${OBJECTDIR}/_ext/839043599/pic16f193x_irq.d ${OBJECTDIR}/_ext/839043599/pic16f193x_irq.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/839043599/pic16f193x_irq.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/839043599/pic16f193x_irq_dispatch.p1: ../../pic16f193x-hal/src/core/pic16f193x_irq_dispatch.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/839043599"
	@${RM} ${OBJECTDIR}/_ext/839043599/pic16f193x_irq_dispatch.p1.d
	@${RM} ${OBJECTDIR}/_ext/839043599/pic16f193x_irq_dispatch.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F1937 -xassembler-with-cpp -I"../../pic16f193x-hal/include/target" -I"../../pic16f193x-hal/include" -I"../../epic-common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/839043599/pic16f193x_irq_dispatch.p1 ../../pic16f193x-hal/src/core/pic16f193x_irq_dispatch.c
	@-${MV} ${OBJECTDIR}/_ext/839043599/pic16f193x_irq_dispatch.d ${OBJECTDIR}/_ext/839043599/pic16f193x_irq_dispatch.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/839043599/pic16f193x_irq_dispatch.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/839043599/pic16f193x_isr_vector.p1: ../../pic16f193x-hal/src/target/pic16f193x_isr_vector.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/839043599"
	@${RM} ${OBJECTDIR}/_ext/839043599/pic16f193x_isr_vector.p1.d
	@${RM} ${OBJECTDIR}/_ext/839043599/pic16f193x_isr_vector.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F1937 -xassembler-with-cpp -I"../../pic16f193x-hal/include/target" -I"../../pic16f193x-hal/include" -I"../../epic-common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/839043599/pic16f193x_isr_vector.p1 ../../pic16f193x-hal/src/target/pic16f193x_isr_vector.c
	@-${MV} ${OBJECTDIR}/_ext/839043599/pic16f193x_isr_vector.d ${OBJECTDIR}/_ext/839043599/pic16f193x_isr_vector.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/839043599/pic16f193x_isr_vector.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/839043599/pic16f193x_wdt_sleep.p1: ../../pic16f193x-hal/src/core/pic16f193x_wdt_sleep.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/839043599"
	@${RM} ${OBJECTDIR}/_ext/839043599/pic16f193x_wdt_sleep.p1.d
	@${RM} ${OBJECTDIR}/_ext/839043599/pic16f193x_wdt_sleep.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F1937 -xassembler-with-cpp -I"../../pic16f193x-hal/include/target" -I"../../pic16f193x-hal/include" -I"../../epic-common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/839043599/pic16f193x_wdt_sleep.p1 ../../pic16f193x-hal/src/core/pic16f193x_wdt_sleep.c
	@-${MV} ${OBJECTDIR}/_ext/839043599/pic16f193x_wdt_sleep.d ${OBJECTDIR}/_ext/839043599/pic16f193x_wdt_sleep.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/839043599/pic16f193x_wdt_sleep.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/839043599/pic16f193x_wdt_sleep_target.p1: ../../pic16f193x-hal/src/target/pic16f193x_wdt_sleep_target.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/839043599"
	@${RM} ${OBJECTDIR}/_ext/839043599/pic16f193x_wdt_sleep_target.p1.d
	@${RM} ${OBJECTDIR}/_ext/839043599/pic16f193x_wdt_sleep_target.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F1937 -xassembler-with-cpp -I"../../pic16f193x-hal/include/target" -I"../../pic16f193x-hal/include" -I"../../epic-common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/839043599/pic16f193x_wdt_sleep_target.p1 ../../pic16f193x-hal/src/target/pic16f193x_wdt_sleep_target.c
	@-${MV} ${OBJECTDIR}/_ext/839043599/pic16f193x_wdt_sleep_target.d ${OBJECTDIR}/_ext/839043599/pic16f193x_wdt_sleep_target.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/839043599/pic16f193x_wdt_sleep_target.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/730952199/pic16f193x_adc.p1: ../../pic16f193x-hal/src/peripherals/pic16f193x_adc.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/730952199"
	@${RM} ${OBJECTDIR}/_ext/730952199/pic16f193x_adc.p1.d
	@${RM} ${OBJECTDIR}/_ext/730952199/pic16f193x_adc.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F1937 -xassembler-with-cpp -I"../../pic16f193x-hal/include/target" -I"../../pic16f193x-hal/include" -I"../../epic-common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/730952199/pic16f193x_adc.p1 ../../pic16f193x-hal/src/peripherals/pic16f193x_adc.c
	@-${MV} ${OBJECTDIR}/_ext/730952199/pic16f193x_adc.d ${OBJECTDIR}/_ext/730952199/pic16f193x_adc.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/730952199/pic16f193x_adc.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/730952199/pic16f193x_ccp.p1: ../../pic16f193x-hal/src/peripherals/pic16f193x_ccp.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/730952199"
	@${RM} ${OBJECTDIR}/_ext/730952199/pic16f193x_ccp.p1.d
	@${RM} ${OBJECTDIR}/_ext/730952199/pic16f193x_ccp.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F1937 -xassembler-with-cpp -I"../../pic16f193x-hal/include/target" -I"../../pic16f193x-hal/include" -I"../../epic-common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/730952199/pic16f193x_ccp.p1 ../../pic16f193x-hal/src/peripherals/pic16f193x_ccp.c
	@-${MV} ${OBJECTDIR}/_ext/730952199/pic16f193x_ccp.d ${OBJECTDIR}/_ext/730952199/pic16f193x_ccp.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/730952199/pic16f193x_ccp.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/730952199/pic16f193x_comp.p1: ../../pic16f193x-hal/src/peripherals/pic16f193x_comp.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/730952199"
	@${RM} ${OBJECTDIR}/_ext/730952199/pic16f193x_comp.p1.d
	@${RM} ${OBJECTDIR}/_ext/730952199/pic16f193x_comp.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F1937 -xassembler-with-cpp -I"../../pic16f193x-hal/include/target" -I"../../pic16f193x-hal/include" -I"../../epic-common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/730952199/pic16f193x_comp.p1 ../../pic16f193x-hal/src/peripherals/pic16f193x_comp.c
	@-${MV} ${OBJECTDIR}/_ext/730952199/pic16f193x_comp.d ${OBJECTDIR}/_ext/730952199/pic16f193x_comp.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/730952199/pic16f193x_comp.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/730952199/pic16f193x_cps.p1: ../../pic16f193x-hal/src/peripherals/pic16f193x_cps.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/730952199"
	@${RM} ${OBJECTDIR}/_ext/730952199/pic16f193x_cps.p1.d
	@${RM} ${OBJECTDIR}/_ext/730952199/pic16f193x_cps.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F1937 -xassembler-with-cpp -I"../../pic16f193x-hal/include/target" -I"../../pic16f193x-hal/include" -I"../../epic-common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/730952199/pic16f193x_cps.p1 ../../pic16f193x-hal/src/peripherals/pic16f193x_cps.c
	@-${MV} ${OBJECTDIR}/_ext/730952199/pic16f193x_cps.d ${OBJECTDIR}/_ext/730952199/pic16f193x_cps.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/730952199/pic16f193x_cps.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/730952199/pic16f193x_dac.p1: ../../pic16f193x-hal/src/peripherals/pic16f193x_dac.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/730952199"
	@${RM} ${OBJECTDIR}/_ext/730952199/pic16f193x_dac.p1.d
	@${RM} ${OBJECTDIR}/_ext/730952199/pic16f193x_dac.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F1937 -xassembler-with-cpp -I"../../pic16f193x-hal/include/target" -I"../../pic16f193x-hal/include" -I"../../epic-common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/730952199/pic16f193x_dac.p1 ../../pic16f193x-hal/src/peripherals/pic16f193x_dac.c
	@-${MV} ${OBJECTDIR}/_ext/730952199/pic16f193x_dac.d ${OBJECTDIR}/_ext/730952199/pic16f193x_dac.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/730952199/pic16f193x_dac.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/730952199/pic16f193x_eeprom.p1: ../../pic16f193x-hal/src/peripherals/pic16f193x_eeprom.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/730952199"
	@${RM} ${OBJECTDIR}/_ext/730952199/pic16f193x_eeprom.p1.d
	@${RM} ${OBJECTDIR}/_ext/730952199/pic16f193x_eeprom.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F1937 -xassembler-with-cpp -I"../../pic16f193x-hal/include/target" -I"../../pic16f193x-hal/include" -I"../../epic-common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/730952199/pic16f193x_eeprom.p1 ../../pic16f193x-hal/src/peripherals/pic16f193x_eeprom.c
	@-${MV} ${OBJECTDIR}/_ext/730952199/pic16f193x_eeprom.d ${OBJECTDIR}/_ext/730952199/pic16f193x_eeprom.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/730952199/pic16f193x_eeprom.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/730952199/pic16f193x_fvr.p1: ../../pic16f193x-hal/src/peripherals/pic16f193x_fvr.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/730952199"
	@${RM} ${OBJECTDIR}/_ext/730952199/pic16f193x_fvr.p1.d
	@${RM} ${OBJECTDIR}/_ext/730952199/pic16f193x_fvr.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F1937 -xassembler-with-cpp -I"../../pic16f193x-hal/include/target" -I"../../pic16f193x-hal/include" -I"../../epic-common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/730952199/pic16f193x_fvr.p1 ../../pic16f193x-hal/src/peripherals/pic16f193x_fvr.c
	@-${MV} ${OBJECTDIR}/_ext/730952199/pic16f193x_fvr.d ${OBJECTDIR}/_ext/730952199/pic16f193x_fvr.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/730952199/pic16f193x_fvr.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/730952199/pic16f193x_gpio.p1: ../../pic16f193x-hal/src/peripherals/pic16f193x_gpio.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/730952199"
	@${RM} ${OBJECTDIR}/_ext/730952199/pic16f193x_gpio.p1.d
	@${RM} ${OBJECTDIR}/_ext/730952199/pic16f193x_gpio.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F1937 -xassembler-with-cpp -I"../../pic16f193x-hal/include/target" -I"../../pic16f193x-hal/include" -I"../../epic-common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/730952199/pic16f193x_gpio.p1 ../../pic16f193x-hal/src/peripherals/pic16f193x_gpio.c
	@-${MV} ${OBJECTDIR}/_ext/730952199/pic16f193x_gpio.d ${OBJECTDIR}/_ext/730952199/pic16f193x_gpio.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/730952199/pic16f193x_gpio.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/730952199/pic16f193x_lcd.p1: ../../pic16f193x-hal/src/peripherals/pic16f193x_lcd.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/730952199"
	@${RM} ${OBJECTDIR}/_ext/730952199/pic16f193x_lcd.p1.d
	@${RM} ${OBJECTDIR}/_ext/730952199/pic16f193x_lcd.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F1937 -xassembler-with-cpp -I"../../pic16f193x-hal/include/target" -I"../../pic16f193x-hal/include" -I"../../epic-common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/730952199/pic16f193x_lcd.p1 ../../pic16f193x-hal/src/peripherals/pic16f193x_lcd.c
	@-${MV} ${OBJECTDIR}/_ext/730952199/pic16f193x_lcd.d ${OBJECTDIR}/_ext/730952199/pic16f193x_lcd.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/730952199/pic16f193x_lcd.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/730952199/pic16f193x_srlatch.p1: ../../pic16f193x-hal/src/peripherals/pic16f193x_srlatch.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/730952199"
	@${RM} ${OBJECTDIR}/_ext/730952199/pic16f193x_srlatch.p1.d
	@${RM} ${OBJECTDIR}/_ext/730952199/pic16f193x_srlatch.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F1937 -xassembler-with-cpp -I"../../pic16f193x-hal/include/target" -I"../../pic16f193x-hal/include" -I"../../epic-common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/730952199/pic16f193x_srlatch.p1 ../../pic16f193x-hal/src/peripherals/pic16f193x_srlatch.c
	@-${MV} ${OBJECTDIR}/_ext/730952199/pic16f193x_srlatch.d ${OBJECTDIR}/_ext/730952199/pic16f193x_srlatch.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/730952199/pic16f193x_srlatch.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/730952199/pic16f193x_ssp.p1: ../../pic16f193x-hal/src/peripherals/pic16f193x_ssp.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/730952199"
	@${RM} ${OBJECTDIR}/_ext/730952199/pic16f193x_ssp.p1.d
	@${RM} ${OBJECTDIR}/_ext/730952199/pic16f193x_ssp.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F1937 -xassembler-with-cpp -I"../../pic16f193x-hal/include/target" -I"../../pic16f193x-hal/include" -I"../../epic-common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/730952199/pic16f193x_ssp.p1 ../../pic16f193x-hal/src/peripherals/pic16f193x_ssp.c
	@-${MV} ${OBJECTDIR}/_ext/730952199/pic16f193x_ssp.d ${OBJECTDIR}/_ext/730952199/pic16f193x_ssp.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/730952199/pic16f193x_ssp.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/730952199/pic16f193x_timer0.p1: ../../pic16f193x-hal/src/peripherals/pic16f193x_timer0.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/730952199"
	@${RM} ${OBJECTDIR}/_ext/730952199/pic16f193x_timer0.p1.d
	@${RM} ${OBJECTDIR}/_ext/730952199/pic16f193x_timer0.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F1937 -xassembler-with-cpp -I"../../pic16f193x-hal/include/target" -I"../../pic16f193x-hal/include" -I"../../epic-common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/730952199/pic16f193x_timer0.p1 ../../pic16f193x-hal/src/peripherals/pic16f193x_timer0.c
	@-${MV} ${OBJECTDIR}/_ext/730952199/pic16f193x_timer0.d ${OBJECTDIR}/_ext/730952199/pic16f193x_timer0.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/730952199/pic16f193x_timer0.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/730952199/pic16f193x_timer1.p1: ../../pic16f193x-hal/src/peripherals/pic16f193x_timer1.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/730952199"
	@${RM} ${OBJECTDIR}/_ext/730952199/pic16f193x_timer1.p1.d
	@${RM} ${OBJECTDIR}/_ext/730952199/pic16f193x_timer1.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F1937 -xassembler-with-cpp -I"../../pic16f193x-hal/include/target" -I"../../pic16f193x-hal/include" -I"../../epic-common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/730952199/pic16f193x_timer1.p1 ../../pic16f193x-hal/src/peripherals/pic16f193x_timer1.c
	@-${MV} ${OBJECTDIR}/_ext/730952199/pic16f193x_timer1.d ${OBJECTDIR}/_ext/730952199/pic16f193x_timer1.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/730952199/pic16f193x_timer1.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/730952199/pic16f193x_timer246.p1: ../../pic16f193x-hal/src/peripherals/pic16f193x_timer246.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/730952199"
	@${RM} ${OBJECTDIR}/_ext/730952199/pic16f193x_timer246.p1.d
	@${RM} ${OBJECTDIR}/_ext/730952199/pic16f193x_timer246.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F1937 -xassembler-with-cpp -I"../../pic16f193x-hal/include/target" -I"../../pic16f193x-hal/include" -I"../../epic-common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/730952199/pic16f193x_timer246.p1 ../../pic16f193x-hal/src/peripherals/pic16f193x_timer246.c
	@-${MV} ${OBJECTDIR}/_ext/730952199/pic16f193x_timer246.d ${OBJECTDIR}/_ext/730952199/pic16f193x_timer246.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/730952199/pic16f193x_timer246.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/730952199/pic16f193x_usart.p1: ../../pic16f193x-hal/src/peripherals/pic16f193x_usart.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/730952199"
	@${RM} ${OBJECTDIR}/_ext/730952199/pic16f193x_usart.p1.d
	@${RM} ${OBJECTDIR}/_ext/730952199/pic16f193x_usart.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F1937 -xassembler-with-cpp -I"../../pic16f193x-hal/include/target" -I"../../pic16f193x-hal/include" -I"../../epic-common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/730952199/pic16f193x_usart.p1 ../../pic16f193x-hal/src/peripherals/pic16f193x_usart.c
	@-${MV} ${OBJECTDIR}/_ext/730952199/pic16f193x_usart.d ${OBJECTDIR}/_ext/730952199/pic16f193x_usart.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/730952199/pic16f193x_usart.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/main.p1: main.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}"
	@${RM} ${OBJECTDIR}/main.p1.d
	@${RM} ${OBJECTDIR}/main.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F1937 -xassembler-with-cpp -I"../../pic16f193x-hal/include/target" -I"../../pic16f193x-hal/include" -I"../../epic-common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/main.p1 main.c
	@-${MV} ${OBJECTDIR}/main.d ${OBJECTDIR}/main.p1.d
	@${FIXDEPS} ${OBJECTDIR}/main.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

else
${OBJECTDIR}/_ext/666212198/epic_harness_target.p1: ../../epic-common/src/core/epic_harness_target.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/666212198"
	@${RM} ${OBJECTDIR}/_ext/666212198/epic_harness_target.p1.d
	@${RM} ${OBJECTDIR}/_ext/666212198/epic_harness_target.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F1937 -xassembler-with-cpp -I"../../pic16f193x-hal/include/target" -I"../../pic16f193x-hal/include" -I"../../epic-common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/666212198/epic_harness_target.p1 ../../epic-common/src/core/epic_harness_target.c
	@-${MV} ${OBJECTDIR}/_ext/666212198/epic_harness_target.d ${OBJECTDIR}/_ext/666212198/epic_harness_target.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/666212198/epic_harness_target.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/839043599/pic16f193x_irq.p1: ../../pic16f193x-hal/src/core/pic16f193x_irq.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/839043599"
	@${RM} ${OBJECTDIR}/_ext/839043599/pic16f193x_irq.p1.d
	@${RM} ${OBJECTDIR}/_ext/839043599/pic16f193x_irq.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F1937 -xassembler-with-cpp -I"../../pic16f193x-hal/include/target" -I"../../pic16f193x-hal/include" -I"../../epic-common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/839043599/pic16f193x_irq.p1 ../../pic16f193x-hal/src/core/pic16f193x_irq.c
	@-${MV} ${OBJECTDIR}/_ext/839043599/pic16f193x_irq.d ${OBJECTDIR}/_ext/839043599/pic16f193x_irq.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/839043599/pic16f193x_irq.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/839043599/pic16f193x_irq_dispatch.p1: ../../pic16f193x-hal/src/core/pic16f193x_irq_dispatch.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/839043599"
	@${RM} ${OBJECTDIR}/_ext/839043599/pic16f193x_irq_dispatch.p1.d
	@${RM} ${OBJECTDIR}/_ext/839043599/pic16f193x_irq_dispatch.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F1937 -xassembler-with-cpp -I"../../pic16f193x-hal/include/target" -I"../../pic16f193x-hal/include" -I"../../epic-common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/839043599/pic16f193x_irq_dispatch.p1 ../../pic16f193x-hal/src/core/pic16f193x_irq_dispatch.c
	@-${MV} ${OBJECTDIR}/_ext/839043599/pic16f193x_irq_dispatch.d ${OBJECTDIR}/_ext/839043599/pic16f193x_irq_dispatch.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/839043599/pic16f193x_irq_dispatch.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/839043599/pic16f193x_isr_vector.p1: ../../pic16f193x-hal/src/target/pic16f193x_isr_vector.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/839043599"
	@${RM} ${OBJECTDIR}/_ext/839043599/pic16f193x_isr_vector.p1.d
	@${RM} ${OBJECTDIR}/_ext/839043599/pic16f193x_isr_vector.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F1937 -xassembler-with-cpp -I"../../pic16f193x-hal/include/target" -I"../../pic16f193x-hal/include" -I"../../epic-common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/839043599/pic16f193x_isr_vector.p1 ../../pic16f193x-hal/src/target/pic16f193x_isr_vector.c
	@-${MV} ${OBJECTDIR}/_ext/839043599/pic16f193x_isr_vector.d ${OBJECTDIR}/_ext/839043599/pic16f193x_isr_vector.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/839043599/pic16f193x_isr_vector.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/839043599/pic16f193x_wdt_sleep.p1: ../../pic16f193x-hal/src/core/pic16f193x_wdt_sleep.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/839043599"
	@${RM} ${OBJECTDIR}/_ext/839043599/pic16f193x_wdt_sleep.p1.d
	@${RM} ${OBJECTDIR}/_ext/839043599/pic16f193x_wdt_sleep.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F1937 -xassembler-with-cpp -I"../../pic16f193x-hal/include/target" -I"../../pic16f193x-hal/include" -I"../../epic-common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/839043599/pic16f193x_wdt_sleep.p1 ../../pic16f193x-hal/src/core/pic16f193x_wdt_sleep.c
	@-${MV} ${OBJECTDIR}/_ext/839043599/pic16f193x_wdt_sleep.d ${OBJECTDIR}/_ext/839043599/pic16f193x_wdt_sleep.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/839043599/pic16f193x_wdt_sleep.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/839043599/pic16f193x_wdt_sleep_target.p1: ../../pic16f193x-hal/src/target/pic16f193x_wdt_sleep_target.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/839043599"
	@${RM} ${OBJECTDIR}/_ext/839043599/pic16f193x_wdt_sleep_target.p1.d
	@${RM} ${OBJECTDIR}/_ext/839043599/pic16f193x_wdt_sleep_target.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F1937 -xassembler-with-cpp -I"../../pic16f193x-hal/include/target" -I"../../pic16f193x-hal/include" -I"../../epic-common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/839043599/pic16f193x_wdt_sleep_target.p1 ../../pic16f193x-hal/src/target/pic16f193x_wdt_sleep_target.c
	@-${MV} ${OBJECTDIR}/_ext/839043599/pic16f193x_wdt_sleep_target.d ${OBJECTDIR}/_ext/839043599/pic16f193x_wdt_sleep_target.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/839043599/pic16f193x_wdt_sleep_target.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/730952199/pic16f193x_adc.p1: ../../pic16f193x-hal/src/peripherals/pic16f193x_adc.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/730952199"
	@${RM} ${OBJECTDIR}/_ext/730952199/pic16f193x_adc.p1.d
	@${RM} ${OBJECTDIR}/_ext/730952199/pic16f193x_adc.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F1937 -xassembler-with-cpp -I"../../pic16f193x-hal/include/target" -I"../../pic16f193x-hal/include" -I"../../epic-common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/730952199/pic16f193x_adc.p1 ../../pic16f193x-hal/src/peripherals/pic16f193x_adc.c
	@-${MV} ${OBJECTDIR}/_ext/730952199/pic16f193x_adc.d ${OBJECTDIR}/_ext/730952199/pic16f193x_adc.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/730952199/pic16f193x_adc.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/730952199/pic16f193x_ccp.p1: ../../pic16f193x-hal/src/peripherals/pic16f193x_ccp.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/730952199"
	@${RM} ${OBJECTDIR}/_ext/730952199/pic16f193x_ccp.p1.d
	@${RM} ${OBJECTDIR}/_ext/730952199/pic16f193x_ccp.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F1937 -xassembler-with-cpp -I"../../pic16f193x-hal/include/target" -I"../../pic16f193x-hal/include" -I"../../epic-common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/730952199/pic16f193x_ccp.p1 ../../pic16f193x-hal/src/peripherals/pic16f193x_ccp.c
	@-${MV} ${OBJECTDIR}/_ext/730952199/pic16f193x_ccp.d ${OBJECTDIR}/_ext/730952199/pic16f193x_ccp.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/730952199/pic16f193x_ccp.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/730952199/pic16f193x_comp.p1: ../../pic16f193x-hal/src/peripherals/pic16f193x_comp.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/730952199"
	@${RM} ${OBJECTDIR}/_ext/730952199/pic16f193x_comp.p1.d
	@${RM} ${OBJECTDIR}/_ext/730952199/pic16f193x_comp.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F1937 -xassembler-with-cpp -I"../../pic16f193x-hal/include/target" -I"../../pic16f193x-hal/include" -I"../../epic-common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/730952199/pic16f193x_comp.p1 ../../pic16f193x-hal/src/peripherals/pic16f193x_comp.c
	@-${MV} ${OBJECTDIR}/_ext/730952199/pic16f193x_comp.d ${OBJECTDIR}/_ext/730952199/pic16f193x_comp.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/730952199/pic16f193x_comp.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/730952199/pic16f193x_cps.p1: ../../pic16f193x-hal/src/peripherals/pic16f193x_cps.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/730952199"
	@${RM} ${OBJECTDIR}/_ext/730952199/pic16f193x_cps.p1.d
	@${RM} ${OBJECTDIR}/_ext/730952199/pic16f193x_cps.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F1937 -xassembler-with-cpp -I"../../pic16f193x-hal/include/target" -I"../../pic16f193x-hal/include" -I"../../epic-common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/730952199/pic16f193x_cps.p1 ../../pic16f193x-hal/src/peripherals/pic16f193x_cps.c
	@-${MV} ${OBJECTDIR}/_ext/730952199/pic16f193x_cps.d ${OBJECTDIR}/_ext/730952199/pic16f193x_cps.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/730952199/pic16f193x_cps.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/730952199/pic16f193x_dac.p1: ../../pic16f193x-hal/src/peripherals/pic16f193x_dac.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/730952199"
	@${RM} ${OBJECTDIR}/_ext/730952199/pic16f193x_dac.p1.d
	@${RM} ${OBJECTDIR}/_ext/730952199/pic16f193x_dac.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F1937 -xassembler-with-cpp -I"../../pic16f193x-hal/include/target" -I"../../pic16f193x-hal/include" -I"../../epic-common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/730952199/pic16f193x_dac.p1 ../../pic16f193x-hal/src/peripherals/pic16f193x_dac.c
	@-${MV} ${OBJECTDIR}/_ext/730952199/pic16f193x_dac.d ${OBJECTDIR}/_ext/730952199/pic16f193x_dac.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/730952199/pic16f193x_dac.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/730952199/pic16f193x_eeprom.p1: ../../pic16f193x-hal/src/peripherals/pic16f193x_eeprom.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/730952199"
	@${RM} ${OBJECTDIR}/_ext/730952199/pic16f193x_eeprom.p1.d
	@${RM} ${OBJECTDIR}/_ext/730952199/pic16f193x_eeprom.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F1937 -xassembler-with-cpp -I"../../pic16f193x-hal/include/target" -I"../../pic16f193x-hal/include" -I"../../epic-common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/730952199/pic16f193x_eeprom.p1 ../../pic16f193x-hal/src/peripherals/pic16f193x_eeprom.c
	@-${MV} ${OBJECTDIR}/_ext/730952199/pic16f193x_eeprom.d ${OBJECTDIR}/_ext/730952199/pic16f193x_eeprom.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/730952199/pic16f193x_eeprom.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/730952199/pic16f193x_fvr.p1: ../../pic16f193x-hal/src/peripherals/pic16f193x_fvr.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/730952199"
	@${RM} ${OBJECTDIR}/_ext/730952199/pic16f193x_fvr.p1.d
	@${RM} ${OBJECTDIR}/_ext/730952199/pic16f193x_fvr.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F1937 -xassembler-with-cpp -I"../../pic16f193x-hal/include/target" -I"../../pic16f193x-hal/include" -I"../../epic-common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/730952199/pic16f193x_fvr.p1 ../../pic16f193x-hal/src/peripherals/pic16f193x_fvr.c
	@-${MV} ${OBJECTDIR}/_ext/730952199/pic16f193x_fvr.d ${OBJECTDIR}/_ext/730952199/pic16f193x_fvr.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/730952199/pic16f193x_fvr.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/730952199/pic16f193x_gpio.p1: ../../pic16f193x-hal/src/peripherals/pic16f193x_gpio.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/730952199"
	@${RM} ${OBJECTDIR}/_ext/730952199/pic16f193x_gpio.p1.d
	@${RM} ${OBJECTDIR}/_ext/730952199/pic16f193x_gpio.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F1937 -xassembler-with-cpp -I"../../pic16f193x-hal/include/target" -I"../../pic16f193x-hal/include" -I"../../epic-common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/730952199/pic16f193x_gpio.p1 ../../pic16f193x-hal/src/peripherals/pic16f193x_gpio.c
	@-${MV} ${OBJECTDIR}/_ext/730952199/pic16f193x_gpio.d ${OBJECTDIR}/_ext/730952199/pic16f193x_gpio.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/730952199/pic16f193x_gpio.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/730952199/pic16f193x_lcd.p1: ../../pic16f193x-hal/src/peripherals/pic16f193x_lcd.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/730952199"
	@${RM} ${OBJECTDIR}/_ext/730952199/pic16f193x_lcd.p1.d
	@${RM} ${OBJECTDIR}/_ext/730952199/pic16f193x_lcd.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F1937 -xassembler-with-cpp -I"../../pic16f193x-hal/include/target" -I"../../pic16f193x-hal/include" -I"../../epic-common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/730952199/pic16f193x_lcd.p1 ../../pic16f193x-hal/src/peripherals/pic16f193x_lcd.c
	@-${MV} ${OBJECTDIR}/_ext/730952199/pic16f193x_lcd.d ${OBJECTDIR}/_ext/730952199/pic16f193x_lcd.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/730952199/pic16f193x_lcd.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/730952199/pic16f193x_srlatch.p1: ../../pic16f193x-hal/src/peripherals/pic16f193x_srlatch.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/730952199"
	@${RM} ${OBJECTDIR}/_ext/730952199/pic16f193x_srlatch.p1.d
	@${RM} ${OBJECTDIR}/_ext/730952199/pic16f193x_srlatch.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F1937 -xassembler-with-cpp -I"../../pic16f193x-hal/include/target" -I"../../pic16f193x-hal/include" -I"../../epic-common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/730952199/pic16f193x_srlatch.p1 ../../pic16f193x-hal/src/peripherals/pic16f193x_srlatch.c
	@-${MV} ${OBJECTDIR}/_ext/730952199/pic16f193x_srlatch.d ${OBJECTDIR}/_ext/730952199/pic16f193x_srlatch.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/730952199/pic16f193x_srlatch.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/730952199/pic16f193x_ssp.p1: ../../pic16f193x-hal/src/peripherals/pic16f193x_ssp.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/730952199"
	@${RM} ${OBJECTDIR}/_ext/730952199/pic16f193x_ssp.p1.d
	@${RM} ${OBJECTDIR}/_ext/730952199/pic16f193x_ssp.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F1937 -xassembler-with-cpp -I"../../pic16f193x-hal/include/target" -I"../../pic16f193x-hal/include" -I"../../epic-common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/730952199/pic16f193x_ssp.p1 ../../pic16f193x-hal/src/peripherals/pic16f193x_ssp.c
	@-${MV} ${OBJECTDIR}/_ext/730952199/pic16f193x_ssp.d ${OBJECTDIR}/_ext/730952199/pic16f193x_ssp.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/730952199/pic16f193x_ssp.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/730952199/pic16f193x_timer0.p1: ../../pic16f193x-hal/src/peripherals/pic16f193x_timer0.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/730952199"
	@${RM} ${OBJECTDIR}/_ext/730952199/pic16f193x_timer0.p1.d
	@${RM} ${OBJECTDIR}/_ext/730952199/pic16f193x_timer0.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F1937 -xassembler-with-cpp -I"../../pic16f193x-hal/include/target" -I"../../pic16f193x-hal/include" -I"../../epic-common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/730952199/pic16f193x_timer0.p1 ../../pic16f193x-hal/src/peripherals/pic16f193x_timer0.c
	@-${MV} ${OBJECTDIR}/_ext/730952199/pic16f193x_timer0.d ${OBJECTDIR}/_ext/730952199/pic16f193x_timer0.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/730952199/pic16f193x_timer0.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/730952199/pic16f193x_timer1.p1: ../../pic16f193x-hal/src/peripherals/pic16f193x_timer1.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/730952199"
	@${RM} ${OBJECTDIR}/_ext/730952199/pic16f193x_timer1.p1.d
	@${RM} ${OBJECTDIR}/_ext/730952199/pic16f193x_timer1.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F1937 -xassembler-with-cpp -I"../../pic16f193x-hal/include/target" -I"../../pic16f193x-hal/include" -I"../../epic-common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/730952199/pic16f193x_timer1.p1 ../../pic16f193x-hal/src/peripherals/pic16f193x_timer1.c
	@-${MV} ${OBJECTDIR}/_ext/730952199/pic16f193x_timer1.d ${OBJECTDIR}/_ext/730952199/pic16f193x_timer1.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/730952199/pic16f193x_timer1.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/730952199/pic16f193x_timer246.p1: ../../pic16f193x-hal/src/peripherals/pic16f193x_timer246.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/730952199"
	@${RM} ${OBJECTDIR}/_ext/730952199/pic16f193x_timer246.p1.d
	@${RM} ${OBJECTDIR}/_ext/730952199/pic16f193x_timer246.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F1937 -xassembler-with-cpp -I"../../pic16f193x-hal/include/target" -I"../../pic16f193x-hal/include" -I"../../epic-common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/730952199/pic16f193x_timer246.p1 ../../pic16f193x-hal/src/peripherals/pic16f193x_timer246.c
	@-${MV} ${OBJECTDIR}/_ext/730952199/pic16f193x_timer246.d ${OBJECTDIR}/_ext/730952199/pic16f193x_timer246.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/730952199/pic16f193x_timer246.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/730952199/pic16f193x_usart.p1: ../../pic16f193x-hal/src/peripherals/pic16f193x_usart.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/730952199"
	@${RM} ${OBJECTDIR}/_ext/730952199/pic16f193x_usart.p1.d
	@${RM} ${OBJECTDIR}/_ext/730952199/pic16f193x_usart.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F1937 -xassembler-with-cpp -I"../../pic16f193x-hal/include/target" -I"../../pic16f193x-hal/include" -I"../../epic-common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/730952199/pic16f193x_usart.p1 ../../pic16f193x-hal/src/peripherals/pic16f193x_usart.c
	@-${MV} ${OBJECTDIR}/_ext/730952199/pic16f193x_usart.d ${OBJECTDIR}/_ext/730952199/pic16f193x_usart.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/730952199/pic16f193x_usart.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/main.p1: main.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}"
	@${RM} ${OBJECTDIR}/main.p1.d
	@${RM} ${OBJECTDIR}/main.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F1937 -xassembler-with-cpp -I"../../pic16f193x-hal/include/target" -I"../../pic16f193x-hal/include" -I"../../epic-common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/main.p1 main.c
	@-${MV} ${OBJECTDIR}/main.d ${OBJECTDIR}/main.p1.d
	@${FIXDEPS} ${OBJECTDIR}/main.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: assemble
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
else
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: assembleWithPreprocess
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
else
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: link
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
${DISTDIR}/epicurus-demo-pic16f193x.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${DISTDIR}
	${MP_CC} $(MP_EXTRA_LD_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -Wl,-Map=${DISTDIR}/epicurus-demo-pic16f193x.X.${IMAGE_TYPE}.map  -D__DEBUG=1  -mdebugger=none  -DXPRJ_default=$(CND_CONF)  -Wl,--defsym=__MPLAB_BUILD=1   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F1937 -xassembler-with-cpp -I"../../pic16f193x-hal/include/target" -I"../../pic16f193x-hal/include" -I"../../epic-common/include" -mwarn=-3 -Wa,-a -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits -std=c99 -gdwarf-3 -mstack=compiled:auto:auto        $(COMPARISON_BUILD) -Wl,--memorysummary,${DISTDIR}/memoryfile.xml -o ${DISTDIR}/epicurus-demo-pic16f193x.X.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX}  ${OBJECTFILES_QUOTED_IF_SPACED}
	@${RM} ${DISTDIR}/epicurus-demo-pic16f193x.X.${IMAGE_TYPE}.hex


else
${DISTDIR}/epicurus-demo-pic16f193x.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${DISTDIR}
	${MP_CC} $(MP_EXTRA_LD_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -Wl,-Map=${DISTDIR}/epicurus-demo-pic16f193x.X.${IMAGE_TYPE}.map  -DXPRJ_default=$(CND_CONF)  -Wl,--defsym=__MPLAB_BUILD=1   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F1937 -xassembler-with-cpp -I"../../pic16f193x-hal/include/target" -I"../../pic16f193x-hal/include" -I"../../epic-common/include" -mwarn=-3 -Wa,-a -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     $(COMPARISON_BUILD) -Wl,--memorysummary,${DISTDIR}/memoryfile.xml -o ${DISTDIR}/epicurus-demo-pic16f193x.X.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX}  ${OBJECTFILES_QUOTED_IF_SPACED}


endif


# Subprojects
.build-subprojects:


# Subprojects
.clean-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${OBJECTDIR}
	${RM} -r ${DISTDIR}

# Enable dependency checking
.dep.inc: .depcheck-impl

DEPFILES=$(wildcard ${POSSIBLE_DEPFILES})
ifneq (${DEPFILES},)
include ${DEPFILES}
endif
