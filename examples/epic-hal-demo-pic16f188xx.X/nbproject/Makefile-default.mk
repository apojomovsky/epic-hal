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
FINAL_IMAGE=${DISTDIR}/epic-hal-demo-pic16f188xx.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
else
IMAGE_TYPE=production
OUTPUT_SUFFIX=hex
DEBUGGABLE_SUFFIX=elf
FINAL_IMAGE=${DISTDIR}/epic-hal-demo-pic16f188xx.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
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
SOURCEFILES_QUOTED_IF_SPACED=../../common/src/core/epic_harness_target.c ../../hal/pic14e/16f188xx/src/core/pic16f188xx_irq.c ../../hal/pic14e/16f188xx/src/core/pic16f188xx_irq_dispatch.c ../../hal/pic14e/16f188xx/src/target/pic16f188xx_isr_vector.c ../../hal/pic14e/16f188xx/src/core/pic16f188xx_wdt_sleep.c ../../hal/pic14e/16f188xx/src/target/pic16f188xx_wdt_sleep_target.c ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_gpio.c ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_timer0.c ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_timer1.c ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_timer2.c ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_timer4.c ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_ccp.c ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_pwm.c ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_pps.c ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_usart.c ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_ssp.c ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_eeprom.c ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_nco.c main.c

# Object Files Quoted if spaced
OBJECTFILES_QUOTED_IF_SPACED=${OBJECTDIR}/_ext/common/epic_harness_target.p1 ${OBJECTDIR}/_ext/hal_core/pic16f188xx_irq.p1 ${OBJECTDIR}/_ext/hal_core/pic16f188xx_irq_dispatch.p1 ${OBJECTDIR}/_ext/hal_target/pic16f188xx_isr_vector.p1 ${OBJECTDIR}/_ext/hal_core/pic16f188xx_wdt_sleep.p1 ${OBJECTDIR}/_ext/hal_target/pic16f188xx_wdt_sleep_target.p1 ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_gpio.p1 ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_timer0.p1 ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_timer1.p1 ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_timer2.p1 ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_timer4.p1 ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ccp.p1 ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pwm.p1 ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.p1 ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.p1 ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.p1 ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_eeprom.p1 ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_nco.p1 ${OBJECTDIR}/main.p1
POSSIBLE_DEPFILES=${OBJECTDIR}/_ext/common/epic_harness_target.p1.d ${OBJECTDIR}/_ext/hal_core/pic16f188xx_irq.p1.d ${OBJECTDIR}/_ext/hal_core/pic16f188xx_irq_dispatch.p1.d ${OBJECTDIR}/_ext/hal_target/pic16f188xx_isr_vector.p1 ${OBJECTDIR}/_ext/hal_core/pic16f188xx_wdt_sleep.p1 ${OBJECTDIR}/_ext/hal_target/pic16f188xx_wdt_sleep_target.p1.d ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_gpio.p1.d ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_timer0.p1.d ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_timer1.p1.d ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_timer2.p1.d ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_timer4.p1.d ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ccp.p1.d ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pwm.p1.d ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.p1.d ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.p1.d ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.p1 ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_eeprom.p1 ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_nco.p1.d ${OBJECTDIR}/main.p1.d

# Object Files
OBJECTFILES=${OBJECTDIR}/_ext/common/epic_harness_target.p1 ${OBJECTDIR}/_ext/hal_core/pic16f188xx_irq.p1 ${OBJECTDIR}/_ext/hal_core/pic16f188xx_irq_dispatch.p1 ${OBJECTDIR}/_ext/hal_target/pic16f188xx_isr_vector.p1 ${OBJECTDIR}/_ext/hal_core/pic16f188xx_wdt_sleep.p1 ${OBJECTDIR}/_ext/hal_target/pic16f188xx_wdt_sleep_target.p1 ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_gpio.p1 ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_timer0.p1 ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_timer1.p1 ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_timer2.p1 ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_timer4.p1 ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ccp.p1 ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pwm.p1 ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.p1 ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.p1 ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.p1 ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_eeprom.p1 ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_nco.p1 ${OBJECTDIR}/main.p1

# Source Files
SOURCEFILES=../../common/src/core/epic_harness_target.c ../../hal/pic14e/16f188xx/src/core/pic16f188xx_irq.c ../../hal/pic14e/16f188xx/src/core/pic16f188xx_irq_dispatch.c ../../hal/pic14e/16f188xx/src/target/pic16f188xx_isr_vector.c ../../hal/pic14e/16f188xx/src/core/pic16f188xx_wdt_sleep.c ../../hal/pic14e/16f188xx/src/target/pic16f188xx_wdt_sleep_target.c ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_gpio.c ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_timer0.c ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_timer1.c ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_timer2.c ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_timer4.c ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_ccp.c ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_pwm.c ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_pps.c ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_usart.c ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_ssp.c ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_eeprom.c ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_nco.c main.c



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
	${MAKE}  -f nbproject/Makefile-default.mk ${DISTDIR}/epic-hal-demo-pic16f188xx.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}

MP_PROCESSOR_OPTION=16F18855
# ------------------------------------------------------------------------------------
# Rules for buildStep: compile
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
${OBJECTDIR}/_ext/common/epic_harness_target.p1: ../../common/src/core/epic_harness_target.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/common"
	@${RM} ${OBJECTDIR}/_ext/common/epic_harness_target.p1.d
	@${RM} ${OBJECTDIR}/_ext/common/epic_harness_target.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/common/epic_harness_target.p1 ../../common/src/core/epic_harness_target.c
	@-${MV} ${OBJECTDIR}/_ext/common/epic_harness_target.d ${OBJECTDIR}/_ext/common/epic_harness_target.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/common/epic_harness_target.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/hal_core/pic16f188xx_irq.p1: ../../hal/pic14e/16f188xx/src/core/pic16f188xx_irq.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/hal_core"
	@${RM} ${OBJECTDIR}/_ext/hal_core/pic16f188xx_irq.p1.d
	@${RM} ${OBJECTDIR}/_ext/hal_core/pic16f188xx_irq.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/hal_core/pic16f188xx_irq.p1 ../../hal/pic14e/16f188xx/src/core/pic16f188xx_irq.c
	@-${MV} ${OBJECTDIR}/_ext/hal_core/pic16f188xx_irq.d ${OBJECTDIR}/_ext/hal_core/pic16f188xx_irq.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/hal_core/pic16f188xx_irq.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/hal_core/pic16f188xx_irq_dispatch.p1: ../../hal/pic14e/16f188xx/src/core/pic16f188xx_irq_dispatch.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/hal_core"
	@${RM} ${OBJECTDIR}/_ext/hal_core/pic16f188xx_irq_dispatch.p1.d
	@${RM} ${OBJECTDIR}/_ext/hal_core/pic16f188xx_irq_dispatch.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/hal_core/pic16f188xx_irq_dispatch.p1 ../../hal/pic14e/16f188xx/src/core/pic16f188xx_irq_dispatch.c
	@-${MV} ${OBJECTDIR}/_ext/hal_core/pic16f188xx_irq_dispatch.d ${OBJECTDIR}/_ext/hal_core/pic16f188xx_irq_dispatch.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/hal_core/pic16f188xx_irq_dispatch.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/hal_core/pic16f188xx_wdt_sleep.p1: ../../hal/pic14e/16f188xx/src/core/pic16f188xx_wdt_sleep.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/hal_core"
	@${RM} ${OBJECTDIR}/_ext/hal_core/pic16f188xx_wdt_sleep.p1.d
	@${RM} ${OBJECTDIR}/_ext/hal_core/pic16f188xx_wdt_sleep.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/hal_core/pic16f188xx_wdt_sleep.p1 ../../hal/pic14e/16f188xx/src/core/pic16f188xx_wdt_sleep.c
	@-${MV} ${OBJECTDIR}/_ext/hal_core/pic16f188xx_wdt_sleep.d ${OBJECTDIR}/_ext/hal_core/pic16f188xx_wdt_sleep.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/hal_core/pic16f188xx_wdt_sleep.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/hal_target/pic16f188xx_isr_vector.p1: ../../hal/pic14e/16f188xx/src/target/pic16f188xx_isr_vector.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/hal_target"
	@${RM} ${OBJECTDIR}/_ext/hal_target/pic16f188xx_isr_vector.p1.d
	@${RM} ${OBJECTDIR}/_ext/hal_target/pic16f188xx_isr_vector.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/hal_target/pic16f188xx_isr_vector.p1 ../../hal/pic14e/16f188xx/src/target/pic16f188xx_isr_vector.c
	@-${MV} ${OBJECTDIR}/_ext/hal_target/pic16f188xx_isr_vector.d ${OBJECTDIR}/_ext/hal_target/pic16f188xx_isr_vector.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/hal_target/pic16f188xx_isr_vector.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/hal_target/pic16f188xx_wdt_sleep_target.p1: ../../hal/pic14e/16f188xx/src/target/pic16f188xx_wdt_sleep_target.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/hal_target"
	@${RM} ${OBJECTDIR}/_ext/hal_target/pic16f188xx_wdt_sleep_target.p1.d
	@${RM} ${OBJECTDIR}/_ext/hal_target/pic16f188xx_wdt_sleep_target.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/hal_target/pic16f188xx_wdt_sleep_target.p1 ../../hal/pic14e/16f188xx/src/target/pic16f188xx_wdt_sleep_target.c
	@-${MV} ${OBJECTDIR}/_ext/hal_target/pic16f188xx_wdt_sleep_target.d ${OBJECTDIR}/_ext/hal_target/pic16f188xx_wdt_sleep_target.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/hal_target/pic16f188xx_wdt_sleep_target.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_gpio.p1: ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_gpio.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/hal_peripherals"
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_gpio.p1.d
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_gpio.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_gpio.p1 ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_gpio.c
	@-${MV} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_gpio.d ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_gpio.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_gpio.p1.d $(SILENT) -rsi ${MP_CC_DIR}../
${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_timer0.p1: ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_timer0.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/hal_peripherals"
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_timer0.p1.d
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_timer0.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_timer0.p1 ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_timer0.c
	@-${MV} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_timer0.d ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_timer0.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_timer0.p1.d $(SILENT) -rsi ${MP_CC_DIR}../
${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.p1: ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_pps.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/hal_peripherals"
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.p1.d
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.p1 ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_pps.c
	@-${MV} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.d ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.p1.d $(SILENT) -rsi ${MP_CC_DIR}../
${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.p1: ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_usart.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/hal_peripherals"
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.p1.d
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.p1 ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_usart.c
	@-${MV} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.d ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.p1.d $(SILENT) -rsi ${MP_CC_DIR}../
${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.p1: ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_ssp.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/hal_peripherals"
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.p1.d
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.p1 ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_ssp.c
	@-${MV} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.d ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_nco.p1: ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_nco.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/hal_peripherals"
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_nco.p1.d
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_nco.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_nco.p1 ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_nco.c
	@-${MV} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_nco.d ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_nco.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_nco.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_eeprom.p1: ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_eeprom.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/hal_peripherals"
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_eeprom.p1.d
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_eeprom.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_eeprom.p1 ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_eeprom.c
	@-${MV} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_eeprom.d ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_eeprom.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_eeprom.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_timer1.p1: ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_timer1.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/hal_peripherals"
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_timer1.p1.d
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_timer1.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_timer1.p1 ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_timer1.c
	@-${MV} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_timer1.d ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_timer1.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_timer1.p1.d $(SILENT) -rsi ${MP_CC_DIR}../
${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.p1: ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_pps.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/hal_peripherals"
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.p1.d
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.p1 ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_pps.c
	@-${MV} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.d ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.p1.d $(SILENT) -rsi ${MP_CC_DIR}../
${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.p1: ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_usart.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/hal_peripherals"
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.p1.d
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.p1 ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_usart.c
	@-${MV} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.d ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.p1.d $(SILENT) -rsi ${MP_CC_DIR}../
${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.p1: ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_ssp.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/hal_peripherals"
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.p1.d
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.p1 ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_ssp.c
	@-${MV} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.d ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_nco.p1: ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_nco.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/hal_peripherals"
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_nco.p1.d
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_nco.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_nco.p1 ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_nco.c
	@-${MV} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_nco.d ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_nco.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_nco.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_eeprom.p1: ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_eeprom.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/hal_peripherals"
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_eeprom.p1.d
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_eeprom.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_eeprom.p1 ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_eeprom.c
	@-${MV} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_eeprom.d ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_eeprom.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_eeprom.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_timer2.p1: ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_timer2.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/hal_peripherals"
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_timer2.p1.d
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_timer2.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_timer2.p1 ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_timer2.c
	@-${MV} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_timer2.d ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_timer2.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_timer2.p1.d $(SILENT) -rsi ${MP_CC_DIR}../
${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_timer4.p1: ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_timer4.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/hal_peripherals"
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_timer4.p1.d
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_timer4.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_timer4.p1 ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_timer4.c
	@-${MV} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_timer4.d ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_timer4.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_timer4.p1.d $(SILENT) -rsi ${MP_CC_DIR}../
${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.p1: ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_pps.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/hal_peripherals"
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.p1.d
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.p1 ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_pps.c
	@-${MV} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.d ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.p1.d $(SILENT) -rsi ${MP_CC_DIR}../
${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.p1: ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_usart.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/hal_peripherals"
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.p1.d
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.p1 ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_usart.c
	@-${MV} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.d ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.p1.d $(SILENT) -rsi ${MP_CC_DIR}../
${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.p1: ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_ssp.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/hal_peripherals"
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.p1.d
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.p1 ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_ssp.c
	@-${MV} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.d ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.p1.d $(SILENT) -rsi ${MP_CC_DIR}../
${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ccp.p1: ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_ccp.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/hal_peripherals"
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ccp.p1.d
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ccp.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ccp.p1 ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_ccp.c
	@-${MV} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ccp.d ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ccp.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ccp.p1.d $(SILENT) -rsi ${MP_CC_DIR}../
${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.p1: ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_pps.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/hal_peripherals"
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.p1.d
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.p1 ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_pps.c
	@-${MV} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.d ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.p1.d $(SILENT) -rsi ${MP_CC_DIR}../
${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.p1: ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_usart.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/hal_peripherals"
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.p1.d
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.p1 ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_usart.c
	@-${MV} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.d ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.p1.d $(SILENT) -rsi ${MP_CC_DIR}../
${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.p1: ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_ssp.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/hal_peripherals"
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.p1.d
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.p1 ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_ssp.c
	@-${MV} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.d ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.p1.d $(SILENT) -rsi ${MP_CC_DIR}../
${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pwm.p1: ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_pwm.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/hal_peripherals"
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pwm.p1.d
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pwm.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pwm.p1 ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_pwm.c
	@-${MV} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pwm.d ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pwm.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pwm.p1.d $(SILENT) -rsi ${MP_CC_DIR}../
${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.p1: ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_pps.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/hal_peripherals"
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.p1.d
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.p1 ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_pps.c
	@-${MV} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.d ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.p1.d $(SILENT) -rsi ${MP_CC_DIR}../
${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.p1: ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_usart.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/hal_peripherals"
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.p1.d
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.p1 ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_usart.c
	@-${MV} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.d ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.p1.d $(SILENT) -rsi ${MP_CC_DIR}../
${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.p1: ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_ssp.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/hal_peripherals"
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.p1.d
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.p1 ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_ssp.c
	@-${MV} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.d ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_nco.p1: ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_nco.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/hal_peripherals"
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_nco.p1.d
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_nco.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_nco.p1 ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_nco.c
	@-${MV} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_nco.d ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_nco.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_nco.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_eeprom.p1: ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_eeprom.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/hal_peripherals"
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_eeprom.p1.d
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_eeprom.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_eeprom.p1 ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_eeprom.c
	@-${MV} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_eeprom.d ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_eeprom.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_eeprom.p1.d $(SILENT) -rsi ${MP_CC_DIR}../


${OBJECTDIR}/main.p1: main.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}"
	@${RM} ${OBJECTDIR}/main.p1.d
	@${RM} ${OBJECTDIR}/main.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/main.p1 main.c
	@-${MV} ${OBJECTDIR}/main.d ${OBJECTDIR}/main.p1.d
	@${FIXDEPS} ${OBJECTDIR}/main.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

else
${OBJECTDIR}/_ext/common/epic_harness_target.p1: ../../common/src/core/epic_harness_target.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/common"
	@${RM} ${OBJECTDIR}/_ext/common/epic_harness_target.p1.d
	@${RM} ${OBJECTDIR}/_ext/common/epic_harness_target.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/common/epic_harness_target.p1 ../../common/src/core/epic_harness_target.c
	@-${MV} ${OBJECTDIR}/_ext/common/epic_harness_target.d ${OBJECTDIR}/_ext/common/epic_harness_target.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/common/epic_harness_target.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/hal_core/pic16f188xx_irq.p1: ../../hal/pic14e/16f188xx/src/core/pic16f188xx_irq.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/hal_core"
	@${RM} ${OBJECTDIR}/_ext/hal_core/pic16f188xx_irq.p1.d
	@${RM} ${OBJECTDIR}/_ext/hal_core/pic16f188xx_irq.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD) -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/hal_core/pic16f188xx_irq.p1 ../../hal/pic14e/16f188xx/src/core/pic16f188xx_irq.c
	@-${MV} ${OBJECTDIR}/_ext/hal_core/pic16f188xx_irq.d ${OBJECTDIR}/_ext/hal_core/pic16f188xx_irq.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/hal_core/pic16f188xx_irq.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/hal_core/pic16f188xx_irq_dispatch.p1: ../../hal/pic14e/16f188xx/src/core/pic16f188xx_irq_dispatch.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/hal_core"
	@${RM} ${OBJECTDIR}/_ext/hal_core/pic16f188xx_irq_dispatch.p1.d
	@${RM} ${OBJECTDIR}/_ext/hal_core/pic16f188xx_irq_dispatch.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD) -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/hal_core/pic16f188xx_irq_dispatch.p1 ../../hal/pic14e/16f188xx/src/core/pic16f188xx_irq_dispatch.c
	@-${MV} ${OBJECTDIR}/_ext/hal_core/pic16f188xx_irq_dispatch.d ${OBJECTDIR}/_ext/hal_core/pic16f188xx_irq_dispatch.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/hal_core/pic16f188xx_irq_dispatch.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/hal_core/pic16f188xx_wdt_sleep.p1: ../../hal/pic14e/16f188xx/src/core/pic16f188xx_wdt_sleep.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/hal_core"
	@${RM} ${OBJECTDIR}/_ext/hal_core/pic16f188xx_wdt_sleep.p1.d
	@${RM} ${OBJECTDIR}/_ext/hal_core/pic16f188xx_wdt_sleep.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD) -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/hal_core/pic16f188xx_wdt_sleep.p1 ../../hal/pic14e/16f188xx/src/core/pic16f188xx_wdt_sleep.c
	@-${MV} ${OBJECTDIR}/_ext/hal_core/pic16f188xx_wdt_sleep.d ${OBJECTDIR}/_ext/hal_core/pic16f188xx_wdt_sleep.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/hal_core/pic16f188xx_wdt_sleep.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/hal_target/pic16f188xx_isr_vector.p1: ../../hal/pic14e/16f188xx/src/target/pic16f188xx_isr_vector.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/hal_target"
	@${RM} ${OBJECTDIR}/_ext/hal_target/pic16f188xx_isr_vector.p1.d
	@${RM} ${OBJECTDIR}/_ext/hal_target/pic16f188xx_isr_vector.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD) -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/hal_target/pic16f188xx_isr_vector.p1 ../../hal/pic14e/16f188xx/src/target/pic16f188xx_isr_vector.c
	@-${MV} ${OBJECTDIR}/_ext/hal_target/pic16f188xx_isr_vector.d ${OBJECTDIR}/_ext/hal_target/pic16f188xx_isr_vector.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/hal_target/pic16f188xx_isr_vector.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/hal_target/pic16f188xx_wdt_sleep_target.p1: ../../hal/pic14e/16f188xx/src/target/pic16f188xx_wdt_sleep_target.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/hal_target"
	@${RM} ${OBJECTDIR}/_ext/hal_target/pic16f188xx_wdt_sleep_target.p1.d
	@${RM} ${OBJECTDIR}/_ext/hal_target/pic16f188xx_wdt_sleep_target.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD) -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/hal_target/pic16f188xx_wdt_sleep_target.p1 ../../hal/pic14e/16f188xx/src/target/pic16f188xx_wdt_sleep_target.c
	@-${MV} ${OBJECTDIR}/_ext/hal_target/pic16f188xx_wdt_sleep_target.d ${OBJECTDIR}/_ext/hal_target/pic16f188xx_wdt_sleep_target.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/hal_target/pic16f188xx_wdt_sleep_target.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_gpio.p1: ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_gpio.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/hal_peripherals"
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_gpio.p1.d
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_gpio.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD) -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_gpio.p1 ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_gpio.c
	@-${MV} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_gpio.d ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_gpio.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_gpio.p1.d $(SILENT) -rsi ${MP_CC_DIR}../
${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_timer0.p1: ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_timer0.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/hal_peripherals"
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_timer0.p1.d
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_timer0.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD) -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_timer0.p1 ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_timer0.c
	@-${MV} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_timer0.d ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_timer0.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_timer0.p1.d $(SILENT) -rsi ${MP_CC_DIR}../
${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.p1: ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_pps.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/hal_peripherals"
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.p1.d
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD) -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.p1 ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_pps.c
	@-${MV} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.d ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.p1.d $(SILENT) -rsi ${MP_CC_DIR}../
${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.p1: ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_usart.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/hal_peripherals"
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.p1.d
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD) -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.p1 ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_usart.c
	@-${MV} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.d ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.p1.d $(SILENT) -rsi ${MP_CC_DIR}../
${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.p1: ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_ssp.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/hal_peripherals"
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.p1.d
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD) -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.p1 ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_ssp.c
	@-${MV} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.d ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_nco.p1: ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_nco.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/hal_peripherals"
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_nco.p1.d
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_nco.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD) -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_nco.p1 ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_nco.c
	@-${MV} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_nco.d ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_nco.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_nco.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_eeprom.p1: ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_eeprom.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/hal_peripherals"
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_eeprom.p1.d
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_eeprom.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD) -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_eeprom.p1 ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_eeprom.c
	@-${MV} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_eeprom.d ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_eeprom.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_eeprom.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_timer1.p1: ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_timer1.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/hal_peripherals"
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_timer1.p1.d
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_timer1.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD) -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_timer1.p1 ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_timer1.c
	@-${MV} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_timer1.d ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_timer1.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_timer1.p1.d $(SILENT) -rsi ${MP_CC_DIR}../
${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.p1: ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_pps.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/hal_peripherals"
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.p1.d
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD) -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.p1 ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_pps.c
	@-${MV} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.d ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.p1.d $(SILENT) -rsi ${MP_CC_DIR}../
${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.p1: ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_usart.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/hal_peripherals"
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.p1.d
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD) -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.p1 ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_usart.c
	@-${MV} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.d ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.p1.d $(SILENT) -rsi ${MP_CC_DIR}../
${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.p1: ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_ssp.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/hal_peripherals"
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.p1.d
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD) -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.p1 ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_ssp.c
	@-${MV} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.d ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_nco.p1: ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_nco.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/hal_peripherals"
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_nco.p1.d
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_nco.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD) -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_nco.p1 ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_nco.c
	@-${MV} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_nco.d ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_nco.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_nco.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_eeprom.p1: ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_eeprom.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/hal_peripherals"
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_eeprom.p1.d
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_eeprom.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD) -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_eeprom.p1 ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_eeprom.c
	@-${MV} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_eeprom.d ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_eeprom.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_eeprom.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_timer2.p1: ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_timer2.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/hal_peripherals"
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_timer2.p1.d
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_timer2.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD) -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_timer2.p1 ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_timer2.c
	@-${MV} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_timer2.d ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_timer2.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_timer2.p1.d $(SILENT) -rsi ${MP_CC_DIR}../
${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_timer4.p1: ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_timer4.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/hal_peripherals"
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_timer4.p1.d
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_timer4.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD) -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_timer4.p1 ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_timer4.c
	@-${MV} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_timer4.d ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_timer4.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_timer4.p1.d $(SILENT) -rsi ${MP_CC_DIR}../
${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.p1: ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_pps.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/hal_peripherals"
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.p1.d
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD) -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.p1 ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_pps.c
	@-${MV} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.d ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.p1.d $(SILENT) -rsi ${MP_CC_DIR}../
${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.p1: ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_usart.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/hal_peripherals"
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.p1.d
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD) -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.p1 ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_usart.c
	@-${MV} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.d ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.p1.d $(SILENT) -rsi ${MP_CC_DIR}../
${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.p1: ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_ssp.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/hal_peripherals"
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.p1.d
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD) -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.p1 ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_ssp.c
	@-${MV} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.d ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.p1.d $(SILENT) -rsi ${MP_CC_DIR}../
${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ccp.p1: ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_ccp.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/hal_peripherals"
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ccp.p1.d
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ccp.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD) -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ccp.p1 ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_ccp.c
	@-${MV} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ccp.d ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ccp.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ccp.p1.d $(SILENT) -rsi ${MP_CC_DIR}../
${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.p1: ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_pps.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/hal_peripherals"
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.p1.d
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD) -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.p1 ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_pps.c
	@-${MV} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.d ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.p1.d $(SILENT) -rsi ${MP_CC_DIR}../
${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.p1: ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_usart.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/hal_peripherals"
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.p1.d
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD) -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.p1 ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_usart.c
	@-${MV} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.d ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.p1.d $(SILENT) -rsi ${MP_CC_DIR}../
${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.p1: ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_ssp.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/hal_peripherals"
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.p1.d
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD) -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.p1 ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_ssp.c
	@-${MV} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.d ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.p1.d $(SILENT) -rsi ${MP_CC_DIR}../
${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pwm.p1: ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_pwm.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/hal_peripherals"
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pwm.p1.d
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pwm.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD) -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pwm.p1 ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_pwm.c
	@-${MV} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pwm.d ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pwm.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pwm.p1.d $(SILENT) -rsi ${MP_CC_DIR}../
${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.p1: ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_pps.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/hal_peripherals"
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.p1.d
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD) -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.p1 ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_pps.c
	@-${MV} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.d ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_pps.p1.d $(SILENT) -rsi ${MP_CC_DIR}../
${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.p1: ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_usart.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/hal_peripherals"
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.p1.d
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD) -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.p1 ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_usart.c
	@-${MV} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.d ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_usart.p1.d $(SILENT) -rsi ${MP_CC_DIR}../
${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.p1: ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_ssp.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/hal_peripherals"
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.p1.d
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD) -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.p1 ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_ssp.c
	@-${MV} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.d ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_ssp.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_nco.p1: ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_nco.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/hal_peripherals"
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_nco.p1.d
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_nco.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD) -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_nco.p1 ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_nco.c
	@-${MV} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_nco.d ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_nco.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_nco.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_eeprom.p1: ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_eeprom.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/hal_peripherals"
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_eeprom.p1.d
	@${RM} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_eeprom.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD) -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_eeprom.p1 ../../hal/pic14e/16f188xx/src/peripherals/pic16f188xx_eeprom.c
	@-${MV} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_eeprom.d ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_eeprom.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/hal_peripherals/pic16f188xx_eeprom.p1.d $(SILENT) -rsi ${MP_CC_DIR}../


${OBJECTDIR}/main.p1: main.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}"
	@${RM} ${OBJECTDIR}/main.p1.d
	@${RM} ${OBJECTDIR}/main.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD) -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/main.p1 main.c
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
${DISTDIR}/epic-hal-demo-pic16f188xx.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${DISTDIR}
	${MP_CC} $(MP_EXTRA_LD_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -Wl,-Map=${DISTDIR}/epic-hal-demo-pic16f188xx.X.${IMAGE_TYPE}.map  -D__DEBUG=1  -mdebugger=none  -DXPRJ_default=$(CND_CONF)  -Wl,--defsym=__MPLAB_BUILD=1   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits -std=c99 -gdwarf-3 -mstack=compiled:auto:auto        $(COMPARISON_BUILD) -Wl,--memorysummary,${DISTDIR}/memoryfile.xml -o ${DISTDIR}/epic-hal-demo-pic16f188xx.X.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX}  ${OBJECTFILES_QUOTED_IF_SPACED}
	@${RM} ${DISTDIR}/epic-hal-demo-pic16f188xx.X.${IMAGE_TYPE}.hex


else
${DISTDIR}/epic-hal-demo-pic16f188xx.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${DISTDIR}
	${MP_CC} $(MP_EXTRA_LD_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -Wl,-Map=${DISTDIR}/epic-hal-demo-pic16f188xx.X.${IMAGE_TYPE}.map  -DXPRJ_default=$(CND_CONF)  -Wl,--defsym=__MPLAB_BUILD=1   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F18855 -xassembler-with-cpp -I"../../hal/pic14e/16f188xx/include/target" -I"../../hal/pic14e/16f188xx/include" -I"../../common/include" -mwarn=-3 -Wa,-a -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     $(COMPARISON_BUILD) -Wl,--memorysummary,${DISTDIR}/memoryfile.xml -o ${DISTDIR}/epic-hal-demo-pic16f188xx.X.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX}  ${OBJECTFILES_QUOTED_IF_SPACED}


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
