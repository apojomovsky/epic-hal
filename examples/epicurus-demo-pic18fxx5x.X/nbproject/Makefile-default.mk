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
FINAL_IMAGE=${DISTDIR}/epicurus-demo-pic18fxx5x.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
else
IMAGE_TYPE=production
OUTPUT_SUFFIX=hex
DEBUGGABLE_SUFFIX=elf
FINAL_IMAGE=${DISTDIR}/epicurus-demo-pic18fxx5x.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
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
SOURCEFILES_QUOTED_IF_SPACED=../../epic-math/src/common/epic_math_rand.c ../../epic-math/src/common/epic_math_numeric.c ../../epic-math/src/common/epic_math_sqrt.c ../../epic-common/src/core/epic_harness_target.c ../../pic18fxx5x-hal/src/core/pic18_irq.c ../../pic18fxx5x-hal/src/target/pic18_isr_vector.c ../../pic18fxx5x-hal/src/core/pic18_irq_dispatch.c ../../pic18fxx5x-hal/src/core/pic18fxx5x_wdt_sleep.c ../../pic18fxx5x-hal/src/target/pic18fxx5x_wdt_sleep_target.c ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_adc.c ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_ccp.c ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_comp.c ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_eeprom.c ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_gpio.c ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_spp.c ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_ssp.c ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_timer0.c ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_timer1.c ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_timer2.c ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_timer3.c ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_usart.c ../../epic-math/src/pic18/epic_math_addsub.c ../../epic-math/src/pic18/epic_math_bcd.c ../../epic-math/src/pic18/epic_math_div.c ../../epic-math/src/pic18/epic_math_mul.c ../../epic-adcfilter/src/epic_adcfilter.c ../../epic-bus/src/epic_bus.c ../../epic-debounce/src/debounce.c ../../epic-encoder/src/encoder.c ../../epic-fsm/src/fsm.c ../../epic-modbus/src/epic_modbus.c ../../epic-pid/src/pid.c ../../epic-serial/src/epic_serial.c ../../epic-tick/src/epic_tick.c main.c

# Object Files Quoted if spaced
OBJECTFILES_QUOTED_IF_SPACED=${OBJECTDIR}/_ext/1230679883/epic_math_rand.p1 ${OBJECTDIR}/_ext/1230679883/epic_math_numeric.p1 ${OBJECTDIR}/_ext/1230679883/epic_math_sqrt.p1 ${OBJECTDIR}/_ext/666212198/epic_harness_target.p1 ${OBJECTDIR}/_ext/1662029371/pic18_irq.p1 ${OBJECTDIR}/_ext/1662029371/pic18_isr_vector.p1 ${OBJECTDIR}/_ext/1662029371/pic18_irq_dispatch.p1 ${OBJECTDIR}/_ext/1662029371/pic18fxx5x_wdt_sleep.p1 ${OBJECTDIR}/_ext/1662029371/pic18fxx5x_wdt_sleep_target.p1 ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_adc.p1 ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_ccp.p1 ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_comp.p1 ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_eeprom.p1 ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_gpio.p1 ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_spp.p1 ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_ssp.p1 ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_timer0.p1 ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_timer1.p1 ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_timer2.p1 ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_timer3.p1 ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_usart.p1 ${OBJECTDIR}/_ext/443525849/epic_math_addsub.p1 ${OBJECTDIR}/_ext/443525849/epic_math_bcd.p1 ${OBJECTDIR}/_ext/443525849/epic_math_div.p1 ${OBJECTDIR}/_ext/443525849/epic_math_mul.p1 ${OBJECTDIR}/_ext/1576634437/epic_adcfilter.p1 ${OBJECTDIR}/_ext/966866771/epic_bus.p1 ${OBJECTDIR}/_ext/1879587514/debounce.p1 ${OBJECTDIR}/_ext/154072393/encoder.p1 ${OBJECTDIR}/_ext/1774618771/fsm.p1 ${OBJECTDIR}/_ext/213266107/epic_modbus.p1 ${OBJECTDIR}/_ext/1784119752/pid.p1 ${OBJECTDIR}/_ext/103087759/epic_serial.p1 ${OBJECTDIR}/_ext/1067072870/epic_tick.p1 ${OBJECTDIR}/main.p1
POSSIBLE_DEPFILES=${OBJECTDIR}/_ext/1230679883/epic_math_rand.p1.d ${OBJECTDIR}/_ext/1230679883/epic_math_numeric.p1.d ${OBJECTDIR}/_ext/1230679883/epic_math_sqrt.p1.d ${OBJECTDIR}/_ext/666212198/epic_harness_target.p1.d ${OBJECTDIR}/_ext/1662029371/pic18_irq.p1.d ${OBJECTDIR}/_ext/1662029371/pic18_isr_vector.p1.d ${OBJECTDIR}/_ext/1662029371/pic18_irq_dispatch.p1.d ${OBJECTDIR}/_ext/1662029371/pic18fxx5x_wdt_sleep.p1.d ${OBJECTDIR}/_ext/1662029371/pic18fxx5x_wdt_sleep_target.p1.d ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_adc.p1.d ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_ccp.p1.d ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_comp.p1.d ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_eeprom.p1.d ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_gpio.p1.d ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_spp.p1.d ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_ssp.p1.d ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_timer0.p1.d ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_timer1.p1.d ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_timer2.p1.d ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_timer3.p1.d ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_usart.p1.d ${OBJECTDIR}/_ext/443525849/epic_math_addsub.p1.d ${OBJECTDIR}/_ext/443525849/epic_math_bcd.p1.d ${OBJECTDIR}/_ext/443525849/epic_math_div.p1.d ${OBJECTDIR}/_ext/443525849/epic_math_mul.p1.d ${OBJECTDIR}/_ext/1576634437/epic_adcfilter.p1.d ${OBJECTDIR}/_ext/966866771/epic_bus.p1.d ${OBJECTDIR}/_ext/1879587514/debounce.p1.d ${OBJECTDIR}/_ext/154072393/encoder.p1.d ${OBJECTDIR}/_ext/1774618771/fsm.p1.d ${OBJECTDIR}/_ext/213266107/epic_modbus.p1.d ${OBJECTDIR}/_ext/1784119752/pid.p1.d ${OBJECTDIR}/_ext/103087759/epic_serial.p1.d ${OBJECTDIR}/_ext/1067072870/epic_tick.p1.d ${OBJECTDIR}/main.p1.d

# Object Files
OBJECTFILES=${OBJECTDIR}/_ext/1230679883/epic_math_rand.p1 ${OBJECTDIR}/_ext/1230679883/epic_math_numeric.p1 ${OBJECTDIR}/_ext/1230679883/epic_math_sqrt.p1 ${OBJECTDIR}/_ext/666212198/epic_harness_target.p1 ${OBJECTDIR}/_ext/1662029371/pic18_irq.p1 ${OBJECTDIR}/_ext/1662029371/pic18_isr_vector.p1 ${OBJECTDIR}/_ext/1662029371/pic18_irq_dispatch.p1 ${OBJECTDIR}/_ext/1662029371/pic18fxx5x_wdt_sleep.p1 ${OBJECTDIR}/_ext/1662029371/pic18fxx5x_wdt_sleep_target.p1 ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_adc.p1 ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_ccp.p1 ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_comp.p1 ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_eeprom.p1 ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_gpio.p1 ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_spp.p1 ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_ssp.p1 ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_timer0.p1 ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_timer1.p1 ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_timer2.p1 ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_timer3.p1 ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_usart.p1 ${OBJECTDIR}/_ext/443525849/epic_math_addsub.p1 ${OBJECTDIR}/_ext/443525849/epic_math_bcd.p1 ${OBJECTDIR}/_ext/443525849/epic_math_div.p1 ${OBJECTDIR}/_ext/443525849/epic_math_mul.p1 ${OBJECTDIR}/_ext/1576634437/epic_adcfilter.p1 ${OBJECTDIR}/_ext/966866771/epic_bus.p1 ${OBJECTDIR}/_ext/1879587514/debounce.p1 ${OBJECTDIR}/_ext/154072393/encoder.p1 ${OBJECTDIR}/_ext/1774618771/fsm.p1 ${OBJECTDIR}/_ext/213266107/epic_modbus.p1 ${OBJECTDIR}/_ext/1784119752/pid.p1 ${OBJECTDIR}/_ext/103087759/epic_serial.p1 ${OBJECTDIR}/_ext/1067072870/epic_tick.p1 ${OBJECTDIR}/main.p1

# Source Files
SOURCEFILES=../../epic-math/src/common/epic_math_rand.c ../../epic-math/src/common/epic_math_numeric.c ../../epic-math/src/common/epic_math_sqrt.c ../../epic-common/src/core/epic_harness_target.c ../../pic18fxx5x-hal/src/core/pic18_irq.c ../../pic18fxx5x-hal/src/target/pic18_isr_vector.c ../../pic18fxx5x-hal/src/core/pic18_irq_dispatch.c ../../pic18fxx5x-hal/src/core/pic18fxx5x_wdt_sleep.c ../../pic18fxx5x-hal/src/target/pic18fxx5x_wdt_sleep_target.c ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_adc.c ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_ccp.c ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_comp.c ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_eeprom.c ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_gpio.c ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_spp.c ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_ssp.c ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_timer0.c ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_timer1.c ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_timer2.c ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_timer3.c ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_usart.c ../../epic-math/src/pic18/epic_math_addsub.c ../../epic-math/src/pic18/epic_math_bcd.c ../../epic-math/src/pic18/epic_math_div.c ../../epic-math/src/pic18/epic_math_mul.c ../../epic-adcfilter/src/epic_adcfilter.c ../../epic-bus/src/epic_bus.c ../../epic-debounce/src/debounce.c ../../epic-encoder/src/encoder.c ../../epic-fsm/src/fsm.c ../../epic-modbus/src/epic_modbus.c ../../epic-pid/src/pid.c ../../epic-serial/src/epic_serial.c ../../epic-tick/src/epic_tick.c main.c



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
	${MAKE}  -f nbproject/Makefile-default.mk ${DISTDIR}/epicurus-demo-pic18fxx5x.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}

MP_PROCESSOR_OPTION=18F4550
# ------------------------------------------------------------------------------------
# Rules for buildStep: compile
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
${OBJECTDIR}/_ext/1230679883/epic_math_rand.p1: ../../epic-math/src/common/epic_math_rand.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1230679883"
	@${RM} ${OBJECTDIR}/_ext/1230679883/epic_math_rand.p1.d
	@${RM} ${OBJECTDIR}/_ext/1230679883/epic_math_rand.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/1230679883/epic_math_rand.p1 ../../epic-math/src/common/epic_math_rand.c
	@-${MV} ${OBJECTDIR}/_ext/1230679883/epic_math_rand.d ${OBJECTDIR}/_ext/1230679883/epic_math_rand.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1230679883/epic_math_rand.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1230679883/epic_math_numeric.p1: ../../epic-math/src/common/epic_math_numeric.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1230679883"
	@${RM} ${OBJECTDIR}/_ext/1230679883/epic_math_numeric.p1.d
	@${RM} ${OBJECTDIR}/_ext/1230679883/epic_math_numeric.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/1230679883/epic_math_numeric.p1 ../../epic-math/src/common/epic_math_numeric.c
	@-${MV} ${OBJECTDIR}/_ext/1230679883/epic_math_numeric.d ${OBJECTDIR}/_ext/1230679883/epic_math_numeric.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1230679883/epic_math_numeric.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1230679883/epic_math_sqrt.p1: ../../epic-math/src/common/epic_math_sqrt.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1230679883"
	@${RM} ${OBJECTDIR}/_ext/1230679883/epic_math_sqrt.p1.d
	@${RM} ${OBJECTDIR}/_ext/1230679883/epic_math_sqrt.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/1230679883/epic_math_sqrt.p1 ../../epic-math/src/common/epic_math_sqrt.c
	@-${MV} ${OBJECTDIR}/_ext/1230679883/epic_math_sqrt.d ${OBJECTDIR}/_ext/1230679883/epic_math_sqrt.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1230679883/epic_math_sqrt.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/666212198/epic_harness_target.p1: ../../epic-common/src/core/epic_harness_target.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/666212198"
	@${RM} ${OBJECTDIR}/_ext/666212198/epic_harness_target.p1.d
	@${RM} ${OBJECTDIR}/_ext/666212198/epic_harness_target.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/666212198/epic_harness_target.p1 ../../epic-common/src/core/epic_harness_target.c
	@-${MV} ${OBJECTDIR}/_ext/666212198/epic_harness_target.d ${OBJECTDIR}/_ext/666212198/epic_harness_target.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/666212198/epic_harness_target.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1662029371/pic18_irq.p1: ../../pic18fxx5x-hal/src/core/pic18_irq.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1662029371"
	@${RM} ${OBJECTDIR}/_ext/1662029371/pic18_irq.p1.d
	@${RM} ${OBJECTDIR}/_ext/1662029371/pic18_irq.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/1662029371/pic18_irq.p1 ../../pic18fxx5x-hal/src/core/pic18_irq.c
	@-${MV} ${OBJECTDIR}/_ext/1662029371/pic18_irq.d ${OBJECTDIR}/_ext/1662029371/pic18_irq.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1662029371/pic18_irq.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1662029371/pic18_isr_vector.p1: ../../pic18fxx5x-hal/src/target/pic18_isr_vector.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1662029371"
	@${RM} ${OBJECTDIR}/_ext/1662029371/pic18_isr_vector.p1.d
	@${RM} ${OBJECTDIR}/_ext/1662029371/pic18_isr_vector.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/1662029371/pic18_isr_vector.p1 ../../pic18fxx5x-hal/src/target/pic18_isr_vector.c
	@-${MV} ${OBJECTDIR}/_ext/1662029371/pic18_isr_vector.d ${OBJECTDIR}/_ext/1662029371/pic18_isr_vector.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1662029371/pic18_isr_vector.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1662029371/pic18_irq_dispatch.p1: ../../pic18fxx5x-hal/src/core/pic18_irq_dispatch.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1662029371"
	@${RM} ${OBJECTDIR}/_ext/1662029371/pic18_irq_dispatch.p1.d
	@${RM} ${OBJECTDIR}/_ext/1662029371/pic18_irq_dispatch.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/1662029371/pic18_irq_dispatch.p1 ../../pic18fxx5x-hal/src/core/pic18_irq_dispatch.c
	@-${MV} ${OBJECTDIR}/_ext/1662029371/pic18_irq_dispatch.d ${OBJECTDIR}/_ext/1662029371/pic18_irq_dispatch.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1662029371/pic18_irq_dispatch.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1662029371/pic18fxx5x_wdt_sleep.p1: ../../pic18fxx5x-hal/src/core/pic18fxx5x_wdt_sleep.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1662029371"
	@${RM} ${OBJECTDIR}/_ext/1662029371/pic18fxx5x_wdt_sleep.p1.d
	@${RM} ${OBJECTDIR}/_ext/1662029371/pic18fxx5x_wdt_sleep.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/1662029371/pic18fxx5x_wdt_sleep.p1 ../../pic18fxx5x-hal/src/core/pic18fxx5x_wdt_sleep.c
	@-${MV} ${OBJECTDIR}/_ext/1662029371/pic18fxx5x_wdt_sleep.d ${OBJECTDIR}/_ext/1662029371/pic18fxx5x_wdt_sleep.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1662029371/pic18fxx5x_wdt_sleep.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1662029371/pic18fxx5x_wdt_sleep_target.p1: ../../pic18fxx5x-hal/src/target/pic18fxx5x_wdt_sleep_target.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1662029371"
	@${RM} ${OBJECTDIR}/_ext/1662029371/pic18fxx5x_wdt_sleep_target.p1.d
	@${RM} ${OBJECTDIR}/_ext/1662029371/pic18fxx5x_wdt_sleep_target.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/1662029371/pic18fxx5x_wdt_sleep_target.p1 ../../pic18fxx5x-hal/src/target/pic18fxx5x_wdt_sleep_target.c
	@-${MV} ${OBJECTDIR}/_ext/1662029371/pic18fxx5x_wdt_sleep_target.d ${OBJECTDIR}/_ext/1662029371/pic18fxx5x_wdt_sleep_target.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1662029371/pic18fxx5x_wdt_sleep_target.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1660085339/pic18fxx5x_adc.p1: ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_adc.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1660085339"
	@${RM} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_adc.p1.d
	@${RM} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_adc.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_adc.p1 ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_adc.c
	@-${MV} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_adc.d ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_adc.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_adc.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1660085339/pic18fxx5x_ccp.p1: ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_ccp.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1660085339"
	@${RM} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_ccp.p1.d
	@${RM} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_ccp.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_ccp.p1 ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_ccp.c
	@-${MV} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_ccp.d ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_ccp.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_ccp.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1660085339/pic18fxx5x_comp.p1: ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_comp.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1660085339"
	@${RM} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_comp.p1.d
	@${RM} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_comp.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_comp.p1 ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_comp.c
	@-${MV} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_comp.d ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_comp.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_comp.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1660085339/pic18fxx5x_eeprom.p1: ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_eeprom.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1660085339"
	@${RM} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_eeprom.p1.d
	@${RM} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_eeprom.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_eeprom.p1 ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_eeprom.c
	@-${MV} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_eeprom.d ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_eeprom.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_eeprom.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1660085339/pic18fxx5x_gpio.p1: ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_gpio.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1660085339"
	@${RM} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_gpio.p1.d
	@${RM} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_gpio.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_gpio.p1 ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_gpio.c
	@-${MV} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_gpio.d ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_gpio.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_gpio.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1660085339/pic18fxx5x_spp.p1: ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_spp.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1660085339"
	@${RM} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_spp.p1.d
	@${RM} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_spp.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_spp.p1 ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_spp.c
	@-${MV} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_spp.d ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_spp.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_spp.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1660085339/pic18fxx5x_ssp.p1: ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_ssp.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1660085339"
	@${RM} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_ssp.p1.d
	@${RM} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_ssp.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_ssp.p1 ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_ssp.c
	@-${MV} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_ssp.d ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_ssp.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_ssp.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1660085339/pic18fxx5x_timer0.p1: ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_timer0.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1660085339"
	@${RM} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_timer0.p1.d
	@${RM} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_timer0.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_timer0.p1 ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_timer0.c
	@-${MV} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_timer0.d ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_timer0.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_timer0.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1660085339/pic18fxx5x_timer1.p1: ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_timer1.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1660085339"
	@${RM} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_timer1.p1.d
	@${RM} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_timer1.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_timer1.p1 ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_timer1.c
	@-${MV} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_timer1.d ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_timer1.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_timer1.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1660085339/pic18fxx5x_timer2.p1: ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_timer2.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1660085339"
	@${RM} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_timer2.p1.d
	@${RM} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_timer2.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_timer2.p1 ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_timer2.c
	@-${MV} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_timer2.d ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_timer2.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_timer2.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1660085339/pic18fxx5x_timer3.p1: ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_timer3.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1660085339"
	@${RM} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_timer3.p1.d
	@${RM} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_timer3.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_timer3.p1 ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_timer3.c
	@-${MV} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_timer3.d ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_timer3.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_timer3.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1660085339/pic18fxx5x_usart.p1: ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_usart.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1660085339"
	@${RM} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_usart.p1.d
	@${RM} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_usart.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_usart.p1 ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_usart.c
	@-${MV} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_usart.d ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_usart.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_usart.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/443525849/epic_math_addsub.p1: ../../epic-math/src/pic18/epic_math_addsub.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/443525849"
	@${RM} ${OBJECTDIR}/_ext/443525849/epic_math_addsub.p1.d
	@${RM} ${OBJECTDIR}/_ext/443525849/epic_math_addsub.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/443525849/epic_math_addsub.p1 ../../epic-math/src/pic18/epic_math_addsub.c
	@-${MV} ${OBJECTDIR}/_ext/443525849/epic_math_addsub.d ${OBJECTDIR}/_ext/443525849/epic_math_addsub.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/443525849/epic_math_addsub.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/443525849/epic_math_bcd.p1: ../../epic-math/src/pic18/epic_math_bcd.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/443525849"
	@${RM} ${OBJECTDIR}/_ext/443525849/epic_math_bcd.p1.d
	@${RM} ${OBJECTDIR}/_ext/443525849/epic_math_bcd.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/443525849/epic_math_bcd.p1 ../../epic-math/src/pic18/epic_math_bcd.c
	@-${MV} ${OBJECTDIR}/_ext/443525849/epic_math_bcd.d ${OBJECTDIR}/_ext/443525849/epic_math_bcd.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/443525849/epic_math_bcd.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/443525849/epic_math_div.p1: ../../epic-math/src/pic18/epic_math_div.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/443525849"
	@${RM} ${OBJECTDIR}/_ext/443525849/epic_math_div.p1.d
	@${RM} ${OBJECTDIR}/_ext/443525849/epic_math_div.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/443525849/epic_math_div.p1 ../../epic-math/src/pic18/epic_math_div.c
	@-${MV} ${OBJECTDIR}/_ext/443525849/epic_math_div.d ${OBJECTDIR}/_ext/443525849/epic_math_div.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/443525849/epic_math_div.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/443525849/epic_math_mul.p1: ../../epic-math/src/pic18/epic_math_mul.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/443525849"
	@${RM} ${OBJECTDIR}/_ext/443525849/epic_math_mul.p1.d
	@${RM} ${OBJECTDIR}/_ext/443525849/epic_math_mul.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/443525849/epic_math_mul.p1 ../../epic-math/src/pic18/epic_math_mul.c
	@-${MV} ${OBJECTDIR}/_ext/443525849/epic_math_mul.d ${OBJECTDIR}/_ext/443525849/epic_math_mul.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/443525849/epic_math_mul.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1576634437/epic_adcfilter.p1: ../../epic-adcfilter/src/epic_adcfilter.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1576634437"
	@${RM} ${OBJECTDIR}/_ext/1576634437/epic_adcfilter.p1.d
	@${RM} ${OBJECTDIR}/_ext/1576634437/epic_adcfilter.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/1576634437/epic_adcfilter.p1 ../../epic-adcfilter/src/epic_adcfilter.c
	@-${MV} ${OBJECTDIR}/_ext/1576634437/epic_adcfilter.d ${OBJECTDIR}/_ext/1576634437/epic_adcfilter.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1576634437/epic_adcfilter.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/966866771/epic_bus.p1: ../../epic-bus/src/epic_bus.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/966866771"
	@${RM} ${OBJECTDIR}/_ext/966866771/epic_bus.p1.d
	@${RM} ${OBJECTDIR}/_ext/966866771/epic_bus.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/966866771/epic_bus.p1 ../../epic-bus/src/epic_bus.c
	@-${MV} ${OBJECTDIR}/_ext/966866771/epic_bus.d ${OBJECTDIR}/_ext/966866771/epic_bus.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/966866771/epic_bus.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1879587514/debounce.p1: ../../epic-debounce/src/debounce.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1879587514"
	@${RM} ${OBJECTDIR}/_ext/1879587514/debounce.p1.d
	@${RM} ${OBJECTDIR}/_ext/1879587514/debounce.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/1879587514/debounce.p1 ../../epic-debounce/src/debounce.c
	@-${MV} ${OBJECTDIR}/_ext/1879587514/debounce.d ${OBJECTDIR}/_ext/1879587514/debounce.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1879587514/debounce.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/154072393/encoder.p1: ../../epic-encoder/src/encoder.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/154072393"
	@${RM} ${OBJECTDIR}/_ext/154072393/encoder.p1.d
	@${RM} ${OBJECTDIR}/_ext/154072393/encoder.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/154072393/encoder.p1 ../../epic-encoder/src/encoder.c
	@-${MV} ${OBJECTDIR}/_ext/154072393/encoder.d ${OBJECTDIR}/_ext/154072393/encoder.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/154072393/encoder.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1774618771/fsm.p1: ../../epic-fsm/src/fsm.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1774618771"
	@${RM} ${OBJECTDIR}/_ext/1774618771/fsm.p1.d
	@${RM} ${OBJECTDIR}/_ext/1774618771/fsm.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/1774618771/fsm.p1 ../../epic-fsm/src/fsm.c
	@-${MV} ${OBJECTDIR}/_ext/1774618771/fsm.d ${OBJECTDIR}/_ext/1774618771/fsm.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1774618771/fsm.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/213266107/epic_modbus.p1: ../../epic-modbus/src/epic_modbus.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/213266107"
	@${RM} ${OBJECTDIR}/_ext/213266107/epic_modbus.p1.d
	@${RM} ${OBJECTDIR}/_ext/213266107/epic_modbus.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/213266107/epic_modbus.p1 ../../epic-modbus/src/epic_modbus.c
	@-${MV} ${OBJECTDIR}/_ext/213266107/epic_modbus.d ${OBJECTDIR}/_ext/213266107/epic_modbus.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/213266107/epic_modbus.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1784119752/pid.p1: ../../epic-pid/src/pid.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1784119752"
	@${RM} ${OBJECTDIR}/_ext/1784119752/pid.p1.d
	@${RM} ${OBJECTDIR}/_ext/1784119752/pid.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/1784119752/pid.p1 ../../epic-pid/src/pid.c
	@-${MV} ${OBJECTDIR}/_ext/1784119752/pid.d ${OBJECTDIR}/_ext/1784119752/pid.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1784119752/pid.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/103087759/epic_serial.p1: ../../epic-serial/src/epic_serial.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/103087759"
	@${RM} ${OBJECTDIR}/_ext/103087759/epic_serial.p1.d
	@${RM} ${OBJECTDIR}/_ext/103087759/epic_serial.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/103087759/epic_serial.p1 ../../epic-serial/src/epic_serial.c
	@-${MV} ${OBJECTDIR}/_ext/103087759/epic_serial.d ${OBJECTDIR}/_ext/103087759/epic_serial.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/103087759/epic_serial.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1067072870/epic_tick.p1: ../../epic-tick/src/epic_tick.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1067072870"
	@${RM} ${OBJECTDIR}/_ext/1067072870/epic_tick.p1.d
	@${RM} ${OBJECTDIR}/_ext/1067072870/epic_tick.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/1067072870/epic_tick.p1 ../../epic-tick/src/epic_tick.c
	@-${MV} ${OBJECTDIR}/_ext/1067072870/epic_tick.d ${OBJECTDIR}/_ext/1067072870/epic_tick.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1067072870/epic_tick.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/main.p1: main.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}"
	@${RM} ${OBJECTDIR}/main.p1.d
	@${RM} ${OBJECTDIR}/main.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/main.p1 main.c
	@-${MV} ${OBJECTDIR}/main.d ${OBJECTDIR}/main.p1.d
	@${FIXDEPS} ${OBJECTDIR}/main.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

else
${OBJECTDIR}/_ext/1230679883/epic_math_rand.p1: ../../epic-math/src/common/epic_math_rand.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1230679883"
	@${RM} ${OBJECTDIR}/_ext/1230679883/epic_math_rand.p1.d
	@${RM} ${OBJECTDIR}/_ext/1230679883/epic_math_rand.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/1230679883/epic_math_rand.p1 ../../epic-math/src/common/epic_math_rand.c
	@-${MV} ${OBJECTDIR}/_ext/1230679883/epic_math_rand.d ${OBJECTDIR}/_ext/1230679883/epic_math_rand.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1230679883/epic_math_rand.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1230679883/epic_math_numeric.p1: ../../epic-math/src/common/epic_math_numeric.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1230679883"
	@${RM} ${OBJECTDIR}/_ext/1230679883/epic_math_numeric.p1.d
	@${RM} ${OBJECTDIR}/_ext/1230679883/epic_math_numeric.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/1230679883/epic_math_numeric.p1 ../../epic-math/src/common/epic_math_numeric.c
	@-${MV} ${OBJECTDIR}/_ext/1230679883/epic_math_numeric.d ${OBJECTDIR}/_ext/1230679883/epic_math_numeric.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1230679883/epic_math_numeric.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1230679883/epic_math_sqrt.p1: ../../epic-math/src/common/epic_math_sqrt.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1230679883"
	@${RM} ${OBJECTDIR}/_ext/1230679883/epic_math_sqrt.p1.d
	@${RM} ${OBJECTDIR}/_ext/1230679883/epic_math_sqrt.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/1230679883/epic_math_sqrt.p1 ../../epic-math/src/common/epic_math_sqrt.c
	@-${MV} ${OBJECTDIR}/_ext/1230679883/epic_math_sqrt.d ${OBJECTDIR}/_ext/1230679883/epic_math_sqrt.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1230679883/epic_math_sqrt.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/666212198/epic_harness_target.p1: ../../epic-common/src/core/epic_harness_target.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/666212198"
	@${RM} ${OBJECTDIR}/_ext/666212198/epic_harness_target.p1.d
	@${RM} ${OBJECTDIR}/_ext/666212198/epic_harness_target.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/666212198/epic_harness_target.p1 ../../epic-common/src/core/epic_harness_target.c
	@-${MV} ${OBJECTDIR}/_ext/666212198/epic_harness_target.d ${OBJECTDIR}/_ext/666212198/epic_harness_target.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/666212198/epic_harness_target.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1662029371/pic18_irq.p1: ../../pic18fxx5x-hal/src/core/pic18_irq.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1662029371"
	@${RM} ${OBJECTDIR}/_ext/1662029371/pic18_irq.p1.d
	@${RM} ${OBJECTDIR}/_ext/1662029371/pic18_irq.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/1662029371/pic18_irq.p1 ../../pic18fxx5x-hal/src/core/pic18_irq.c
	@-${MV} ${OBJECTDIR}/_ext/1662029371/pic18_irq.d ${OBJECTDIR}/_ext/1662029371/pic18_irq.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1662029371/pic18_irq.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1662029371/pic18_isr_vector.p1: ../../pic18fxx5x-hal/src/target/pic18_isr_vector.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1662029371"
	@${RM} ${OBJECTDIR}/_ext/1662029371/pic18_isr_vector.p1.d
	@${RM} ${OBJECTDIR}/_ext/1662029371/pic18_isr_vector.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/1662029371/pic18_isr_vector.p1 ../../pic18fxx5x-hal/src/target/pic18_isr_vector.c
	@-${MV} ${OBJECTDIR}/_ext/1662029371/pic18_isr_vector.d ${OBJECTDIR}/_ext/1662029371/pic18_isr_vector.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1662029371/pic18_isr_vector.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1662029371/pic18_irq_dispatch.p1: ../../pic18fxx5x-hal/src/core/pic18_irq_dispatch.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1662029371"
	@${RM} ${OBJECTDIR}/_ext/1662029371/pic18_irq_dispatch.p1.d
	@${RM} ${OBJECTDIR}/_ext/1662029371/pic18_irq_dispatch.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/1662029371/pic18_irq_dispatch.p1 ../../pic18fxx5x-hal/src/core/pic18_irq_dispatch.c
	@-${MV} ${OBJECTDIR}/_ext/1662029371/pic18_irq_dispatch.d ${OBJECTDIR}/_ext/1662029371/pic18_irq_dispatch.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1662029371/pic18_irq_dispatch.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1662029371/pic18fxx5x_wdt_sleep.p1: ../../pic18fxx5x-hal/src/core/pic18fxx5x_wdt_sleep.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1662029371"
	@${RM} ${OBJECTDIR}/_ext/1662029371/pic18fxx5x_wdt_sleep.p1.d
	@${RM} ${OBJECTDIR}/_ext/1662029371/pic18fxx5x_wdt_sleep.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/1662029371/pic18fxx5x_wdt_sleep.p1 ../../pic18fxx5x-hal/src/core/pic18fxx5x_wdt_sleep.c
	@-${MV} ${OBJECTDIR}/_ext/1662029371/pic18fxx5x_wdt_sleep.d ${OBJECTDIR}/_ext/1662029371/pic18fxx5x_wdt_sleep.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1662029371/pic18fxx5x_wdt_sleep.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1662029371/pic18fxx5x_wdt_sleep_target.p1: ../../pic18fxx5x-hal/src/target/pic18fxx5x_wdt_sleep_target.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1662029371"
	@${RM} ${OBJECTDIR}/_ext/1662029371/pic18fxx5x_wdt_sleep_target.p1.d
	@${RM} ${OBJECTDIR}/_ext/1662029371/pic18fxx5x_wdt_sleep_target.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/1662029371/pic18fxx5x_wdt_sleep_target.p1 ../../pic18fxx5x-hal/src/target/pic18fxx5x_wdt_sleep_target.c
	@-${MV} ${OBJECTDIR}/_ext/1662029371/pic18fxx5x_wdt_sleep_target.d ${OBJECTDIR}/_ext/1662029371/pic18fxx5x_wdt_sleep_target.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1662029371/pic18fxx5x_wdt_sleep_target.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1660085339/pic18fxx5x_adc.p1: ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_adc.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1660085339"
	@${RM} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_adc.p1.d
	@${RM} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_adc.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_adc.p1 ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_adc.c
	@-${MV} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_adc.d ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_adc.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_adc.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1660085339/pic18fxx5x_ccp.p1: ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_ccp.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1660085339"
	@${RM} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_ccp.p1.d
	@${RM} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_ccp.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_ccp.p1 ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_ccp.c
	@-${MV} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_ccp.d ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_ccp.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_ccp.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1660085339/pic18fxx5x_comp.p1: ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_comp.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1660085339"
	@${RM} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_comp.p1.d
	@${RM} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_comp.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_comp.p1 ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_comp.c
	@-${MV} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_comp.d ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_comp.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_comp.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1660085339/pic18fxx5x_eeprom.p1: ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_eeprom.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1660085339"
	@${RM} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_eeprom.p1.d
	@${RM} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_eeprom.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_eeprom.p1 ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_eeprom.c
	@-${MV} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_eeprom.d ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_eeprom.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_eeprom.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1660085339/pic18fxx5x_gpio.p1: ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_gpio.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1660085339"
	@${RM} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_gpio.p1.d
	@${RM} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_gpio.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_gpio.p1 ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_gpio.c
	@-${MV} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_gpio.d ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_gpio.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_gpio.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1660085339/pic18fxx5x_spp.p1: ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_spp.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1660085339"
	@${RM} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_spp.p1.d
	@${RM} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_spp.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_spp.p1 ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_spp.c
	@-${MV} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_spp.d ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_spp.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_spp.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1660085339/pic18fxx5x_ssp.p1: ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_ssp.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1660085339"
	@${RM} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_ssp.p1.d
	@${RM} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_ssp.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_ssp.p1 ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_ssp.c
	@-${MV} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_ssp.d ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_ssp.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_ssp.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1660085339/pic18fxx5x_timer0.p1: ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_timer0.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1660085339"
	@${RM} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_timer0.p1.d
	@${RM} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_timer0.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_timer0.p1 ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_timer0.c
	@-${MV} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_timer0.d ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_timer0.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_timer0.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1660085339/pic18fxx5x_timer1.p1: ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_timer1.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1660085339"
	@${RM} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_timer1.p1.d
	@${RM} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_timer1.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_timer1.p1 ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_timer1.c
	@-${MV} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_timer1.d ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_timer1.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_timer1.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1660085339/pic18fxx5x_timer2.p1: ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_timer2.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1660085339"
	@${RM} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_timer2.p1.d
	@${RM} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_timer2.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_timer2.p1 ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_timer2.c
	@-${MV} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_timer2.d ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_timer2.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_timer2.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1660085339/pic18fxx5x_timer3.p1: ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_timer3.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1660085339"
	@${RM} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_timer3.p1.d
	@${RM} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_timer3.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_timer3.p1 ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_timer3.c
	@-${MV} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_timer3.d ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_timer3.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_timer3.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1660085339/pic18fxx5x_usart.p1: ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_usart.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1660085339"
	@${RM} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_usart.p1.d
	@${RM} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_usart.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_usart.p1 ../../pic18fxx5x-hal/src/peripherals/pic18fxx5x_usart.c
	@-${MV} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_usart.d ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_usart.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1660085339/pic18fxx5x_usart.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/443525849/epic_math_addsub.p1: ../../epic-math/src/pic18/epic_math_addsub.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/443525849"
	@${RM} ${OBJECTDIR}/_ext/443525849/epic_math_addsub.p1.d
	@${RM} ${OBJECTDIR}/_ext/443525849/epic_math_addsub.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/443525849/epic_math_addsub.p1 ../../epic-math/src/pic18/epic_math_addsub.c
	@-${MV} ${OBJECTDIR}/_ext/443525849/epic_math_addsub.d ${OBJECTDIR}/_ext/443525849/epic_math_addsub.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/443525849/epic_math_addsub.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/443525849/epic_math_bcd.p1: ../../epic-math/src/pic18/epic_math_bcd.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/443525849"
	@${RM} ${OBJECTDIR}/_ext/443525849/epic_math_bcd.p1.d
	@${RM} ${OBJECTDIR}/_ext/443525849/epic_math_bcd.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/443525849/epic_math_bcd.p1 ../../epic-math/src/pic18/epic_math_bcd.c
	@-${MV} ${OBJECTDIR}/_ext/443525849/epic_math_bcd.d ${OBJECTDIR}/_ext/443525849/epic_math_bcd.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/443525849/epic_math_bcd.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/443525849/epic_math_div.p1: ../../epic-math/src/pic18/epic_math_div.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/443525849"
	@${RM} ${OBJECTDIR}/_ext/443525849/epic_math_div.p1.d
	@${RM} ${OBJECTDIR}/_ext/443525849/epic_math_div.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/443525849/epic_math_div.p1 ../../epic-math/src/pic18/epic_math_div.c
	@-${MV} ${OBJECTDIR}/_ext/443525849/epic_math_div.d ${OBJECTDIR}/_ext/443525849/epic_math_div.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/443525849/epic_math_div.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/443525849/epic_math_mul.p1: ../../epic-math/src/pic18/epic_math_mul.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/443525849"
	@${RM} ${OBJECTDIR}/_ext/443525849/epic_math_mul.p1.d
	@${RM} ${OBJECTDIR}/_ext/443525849/epic_math_mul.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/443525849/epic_math_mul.p1 ../../epic-math/src/pic18/epic_math_mul.c
	@-${MV} ${OBJECTDIR}/_ext/443525849/epic_math_mul.d ${OBJECTDIR}/_ext/443525849/epic_math_mul.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/443525849/epic_math_mul.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1576634437/epic_adcfilter.p1: ../../epic-adcfilter/src/epic_adcfilter.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1576634437"
	@${RM} ${OBJECTDIR}/_ext/1576634437/epic_adcfilter.p1.d
	@${RM} ${OBJECTDIR}/_ext/1576634437/epic_adcfilter.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/1576634437/epic_adcfilter.p1 ../../epic-adcfilter/src/epic_adcfilter.c
	@-${MV} ${OBJECTDIR}/_ext/1576634437/epic_adcfilter.d ${OBJECTDIR}/_ext/1576634437/epic_adcfilter.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1576634437/epic_adcfilter.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/966866771/epic_bus.p1: ../../epic-bus/src/epic_bus.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/966866771"
	@${RM} ${OBJECTDIR}/_ext/966866771/epic_bus.p1.d
	@${RM} ${OBJECTDIR}/_ext/966866771/epic_bus.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/966866771/epic_bus.p1 ../../epic-bus/src/epic_bus.c
	@-${MV} ${OBJECTDIR}/_ext/966866771/epic_bus.d ${OBJECTDIR}/_ext/966866771/epic_bus.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/966866771/epic_bus.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1879587514/debounce.p1: ../../epic-debounce/src/debounce.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1879587514"
	@${RM} ${OBJECTDIR}/_ext/1879587514/debounce.p1.d
	@${RM} ${OBJECTDIR}/_ext/1879587514/debounce.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/1879587514/debounce.p1 ../../epic-debounce/src/debounce.c
	@-${MV} ${OBJECTDIR}/_ext/1879587514/debounce.d ${OBJECTDIR}/_ext/1879587514/debounce.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1879587514/debounce.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/154072393/encoder.p1: ../../epic-encoder/src/encoder.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/154072393"
	@${RM} ${OBJECTDIR}/_ext/154072393/encoder.p1.d
	@${RM} ${OBJECTDIR}/_ext/154072393/encoder.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/154072393/encoder.p1 ../../epic-encoder/src/encoder.c
	@-${MV} ${OBJECTDIR}/_ext/154072393/encoder.d ${OBJECTDIR}/_ext/154072393/encoder.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/154072393/encoder.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1774618771/fsm.p1: ../../epic-fsm/src/fsm.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1774618771"
	@${RM} ${OBJECTDIR}/_ext/1774618771/fsm.p1.d
	@${RM} ${OBJECTDIR}/_ext/1774618771/fsm.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/1774618771/fsm.p1 ../../epic-fsm/src/fsm.c
	@-${MV} ${OBJECTDIR}/_ext/1774618771/fsm.d ${OBJECTDIR}/_ext/1774618771/fsm.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1774618771/fsm.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/213266107/epic_modbus.p1: ../../epic-modbus/src/epic_modbus.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/213266107"
	@${RM} ${OBJECTDIR}/_ext/213266107/epic_modbus.p1.d
	@${RM} ${OBJECTDIR}/_ext/213266107/epic_modbus.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/213266107/epic_modbus.p1 ../../epic-modbus/src/epic_modbus.c
	@-${MV} ${OBJECTDIR}/_ext/213266107/epic_modbus.d ${OBJECTDIR}/_ext/213266107/epic_modbus.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/213266107/epic_modbus.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1784119752/pid.p1: ../../epic-pid/src/pid.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1784119752"
	@${RM} ${OBJECTDIR}/_ext/1784119752/pid.p1.d
	@${RM} ${OBJECTDIR}/_ext/1784119752/pid.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/1784119752/pid.p1 ../../epic-pid/src/pid.c
	@-${MV} ${OBJECTDIR}/_ext/1784119752/pid.d ${OBJECTDIR}/_ext/1784119752/pid.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1784119752/pid.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/103087759/epic_serial.p1: ../../epic-serial/src/epic_serial.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/103087759"
	@${RM} ${OBJECTDIR}/_ext/103087759/epic_serial.p1.d
	@${RM} ${OBJECTDIR}/_ext/103087759/epic_serial.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/103087759/epic_serial.p1 ../../epic-serial/src/epic_serial.c
	@-${MV} ${OBJECTDIR}/_ext/103087759/epic_serial.d ${OBJECTDIR}/_ext/103087759/epic_serial.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/103087759/epic_serial.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1067072870/epic_tick.p1: ../../epic-tick/src/epic_tick.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1067072870"
	@${RM} ${OBJECTDIR}/_ext/1067072870/epic_tick.p1.d
	@${RM} ${OBJECTDIR}/_ext/1067072870/epic_tick.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/_ext/1067072870/epic_tick.p1 ../../epic-tick/src/epic_tick.c
	@-${MV} ${OBJECTDIR}/_ext/1067072870/epic_tick.d ${OBJECTDIR}/_ext/1067072870/epic_tick.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1067072870/epic_tick.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/main.p1: main.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}"
	@${RM} ${OBJECTDIR}/main.p1.d
	@${RM} ${OBJECTDIR}/main.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/main.p1 main.c
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
${DISTDIR}/epicurus-demo-pic18fxx5x.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${DISTDIR}
	${MP_CC} $(MP_EXTRA_LD_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -Wl,-Map=${DISTDIR}/epicurus-demo-pic18fxx5x.X.${IMAGE_TYPE}.map  -D__DEBUG=1  -mdebugger=none  -DXPRJ_default=$(CND_CONF)  -Wl,--defsym=__MPLAB_BUILD=1   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto        $(COMPARISON_BUILD) -Wl,--memorysummary,${DISTDIR}/memoryfile.xml -o ${DISTDIR}/epicurus-demo-pic18fxx5x.X.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX}  ${OBJECTFILES_QUOTED_IF_SPACED}
	@${RM} ${DISTDIR}/epicurus-demo-pic18fxx5x.X.${IMAGE_TYPE}.hex


else
${DISTDIR}/epicurus-demo-pic18fxx5x.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${DISTDIR}
	${MP_CC} $(MP_EXTRA_LD_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -Wl,-Map=${DISTDIR}/epicurus-demo-pic18fxx5x.X.${IMAGE_TYPE}.map  -DXPRJ_default=$(CND_CONF)  -Wl,--defsym=__MPLAB_BUILD=1   -mdfp="${DFP_DIR}/xc8"  -memi=wordwrite -O0 -fasmfile -maddrqual=ignore -DPIC18F4550 -DFOSC_HZ=48000000 -xassembler-with-cpp -I"../../pic18fxx5x-hal/include/target" -I"../../pic18fxx5x-hal/include" -I"../../epic-common/include" -I"../../epic-adcfilter/include" -I"../../epic-bus/include" -I"../../epic-debounce/include" -I"../../epic-encoder/include" -I"../../epic-fsm/include" -I"../../epic-math/include" -I"../../epic-math/tests" -I"../../epic-modbus/include" -I"../../epic-pid/include" -I"../../epic-serial/include" -I"../../epic-tick/include" -mwarn=-3 -Wa,-a -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-download -mno-default-config-bits -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     $(COMPARISON_BUILD) -Wl,--memorysummary,${DISTDIR}/memoryfile.xml -o ${DISTDIR}/epicurus-demo-pic18fxx5x.X.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX}  ${OBJECTFILES_QUOTED_IF_SPACED}


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
