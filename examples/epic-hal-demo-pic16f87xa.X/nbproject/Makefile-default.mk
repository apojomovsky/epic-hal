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
FINAL_IMAGE=${DISTDIR}/epic-hal-demo-pic16f87xa.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
else
IMAGE_TYPE=production
OUTPUT_SUFFIX=hex
DEBUGGABLE_SUFFIX=elf
FINAL_IMAGE=${DISTDIR}/epic-hal-demo-pic16f87xa.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
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
SOURCEFILES_QUOTED_IF_SPACED=../../lib/math/src/common/epic_math_rand.c ../../lib/math/src/common/epic_math_numeric.c ../../lib/math/src/common/epic_math_sqrt.c ../../common/src/core/epic_harness_target.c ../../hal/pic14/core/src/core/pic14_irq.c ../../hal/pic14/core/src/target/pic16_isr_vector.c ../../hal/pic14/core/src/core/pic14_irq_dispatch.c ../../hal/pic14/core/src/core/pic14_wdt_sleep.c ../../hal/pic14/core/src/target/pic14_wdt_sleep_target.c ../../hal/pic14/core/src/peripherals/pic14_timer0.c ../../hal/pic14/core/src/peripherals/pic14_usart.c ../../hal/pic14/core/src/peripherals/pic14_ccp.c ../../hal/pic14/core/src/peripherals/pic14_comp.c ../../hal/pic14/core/src/peripherals/pic14_timer1.c ../../hal/pic14/core/src/peripherals/pic14_timer2.c ../../hal/pic14/core/src/peripherals/pic14_adc.c ../../hal/pic14/core/src/peripherals/pic14_eeprom.c ../../hal/pic14/core/src/peripherals/pic14_vref.c ../../hal/pic14/core/src/peripherals/pic14_gpio.c ../../hal/pic14/16f87xa/src/peripherals/pic16f87xa_psp.c ../../hal/pic14/core/src/peripherals/pic14_ssp.c ../../lib/math/src/pic16/epic_math_div.c ../../lib/math/src/pic16/epic_math_scratch.c ../../lib/math/src/pic16/epic_math_mul.c ../../lib/math/src/pic16/epic_math_bcd.c ../../lib/math/src/pic16/epic_math_addsub.c ../../lib/adcfilter/src/epic_adcfilter.c ../../lib/bus/src/epic_bus.c ../../lib/debounce/src/debounce.c ../../lib/encoder/src/encoder.c ../../lib/fsm/src/fsm.c ../../lib/pid/src/pid.c ../../lib/serial/src/epic_serial.c ../../lib/taskmgr/src/epic_taskmgr.c ../../lib/tick/src/epic_tick.c main.c ../../hal/pic14/16f87xa/src/core/pic16_irq_table.c

# Object Files Quoted if spaced
OBJECTFILES_QUOTED_IF_SPACED=${OBJECTDIR}/_ext/1230679883/epic_math_rand.p1 ${OBJECTDIR}/_ext/1230679883/epic_math_numeric.p1 ${OBJECTDIR}/_ext/1230679883/epic_math_sqrt.p1 ${OBJECTDIR}/_ext/666212198/epic_harness_target.p1 ${OBJECTDIR}/_ext/463256514/pic14_irq.p1 ${OBJECTDIR}/_ext/463256514/pic16_isr_vector.p1 ${OBJECTDIR}/_ext/463256514/pic14_irq_dispatch.p1 ${OBJECTDIR}/_ext/463256514/pic14_wdt_sleep.p1 ${OBJECTDIR}/_ext/463256514/pic14_wdt_sleep_target.p1 ${OBJECTDIR}/_ext/1853308684/pic14_timer0.p1 ${OBJECTDIR}/_ext/1853308684/pic14_usart.p1 ${OBJECTDIR}/_ext/1853308684/pic14_ccp.p1 ${OBJECTDIR}/_ext/1853308684/pic14_comp.p1 ${OBJECTDIR}/_ext/1853308684/pic14_timer1.p1 ${OBJECTDIR}/_ext/1853308684/pic14_timer2.p1 ${OBJECTDIR}/_ext/1853308684/pic14_adc.p1 ${OBJECTDIR}/_ext/1853308684/pic14_eeprom.p1 ${OBJECTDIR}/_ext/1853308684/pic14_vref.p1 ${OBJECTDIR}/_ext/1853308684/pic14_gpio.p1 ${OBJECTDIR}/_ext/1685690910/pic16f87xa_psp.p1 ${OBJECTDIR}/_ext/1853308684/pic14_ssp.p1 ${OBJECTDIR}/_ext/443525851/epic_math_div.p1 ${OBJECTDIR}/_ext/443525851/epic_math_scratch.p1 ${OBJECTDIR}/_ext/443525851/epic_math_mul.p1 ${OBJECTDIR}/_ext/443525851/epic_math_bcd.p1 ${OBJECTDIR}/_ext/443525851/epic_math_addsub.p1 ${OBJECTDIR}/_ext/1576634437/epic_adcfilter.p1 ${OBJECTDIR}/_ext/966866771/epic_bus.p1 ${OBJECTDIR}/_ext/1879587514/debounce.p1 ${OBJECTDIR}/_ext/154072393/encoder.p1 ${OBJECTDIR}/_ext/1774618771/fsm.p1 ${OBJECTDIR}/_ext/1784119752/pid.p1 ${OBJECTDIR}/_ext/103087759/epic_serial.p1 ${OBJECTDIR}/_ext/551636192/epic_taskmgr.p1 ${OBJECTDIR}/_ext/1067072870/epic_tick.p1 ${OBJECTDIR}/main.p1 ${OBJECTDIR}/_ext/1866815852/pic16_irq_table.p1
POSSIBLE_DEPFILES=${OBJECTDIR}/_ext/1230679883/epic_math_rand.p1.d ${OBJECTDIR}/_ext/1230679883/epic_math_numeric.p1.d ${OBJECTDIR}/_ext/1230679883/epic_math_sqrt.p1.d ${OBJECTDIR}/_ext/666212198/epic_harness_target.p1.d ${OBJECTDIR}/_ext/463256514/pic14_irq.p1.d ${OBJECTDIR}/_ext/463256514/pic16_isr_vector.p1.d ${OBJECTDIR}/_ext/463256514/pic14_irq_dispatch.p1.d ${OBJECTDIR}/_ext/463256514/pic14_wdt_sleep.p1.d ${OBJECTDIR}/_ext/463256514/pic14_wdt_sleep_target.p1.d ${OBJECTDIR}/_ext/1853308684/pic14_timer0.p1.d ${OBJECTDIR}/_ext/1853308684/pic14_usart.p1.d ${OBJECTDIR}/_ext/1853308684/pic14_ccp.p1.d ${OBJECTDIR}/_ext/1853308684/pic14_comp.p1.d ${OBJECTDIR}/_ext/1853308684/pic14_timer1.p1.d ${OBJECTDIR}/_ext/1853308684/pic14_timer2.p1.d ${OBJECTDIR}/_ext/1853308684/pic14_adc.p1.d ${OBJECTDIR}/_ext/1853308684/pic14_eeprom.p1.d ${OBJECTDIR}/_ext/1853308684/pic14_vref.p1.d ${OBJECTDIR}/_ext/1853308684/pic14_gpio.p1.d ${OBJECTDIR}/_ext/1685690910/pic16f87xa_psp.p1.d ${OBJECTDIR}/_ext/1853308684/pic14_ssp.p1.d ${OBJECTDIR}/_ext/443525851/epic_math_div.p1.d ${OBJECTDIR}/_ext/443525851/epic_math_scratch.p1.d ${OBJECTDIR}/_ext/443525851/epic_math_mul.p1.d ${OBJECTDIR}/_ext/443525851/epic_math_bcd.p1.d ${OBJECTDIR}/_ext/443525851/epic_math_addsub.p1.d ${OBJECTDIR}/_ext/1576634437/epic_adcfilter.p1.d ${OBJECTDIR}/_ext/966866771/epic_bus.p1.d ${OBJECTDIR}/_ext/1879587514/debounce.p1.d ${OBJECTDIR}/_ext/154072393/encoder.p1.d ${OBJECTDIR}/_ext/1774618771/fsm.p1.d ${OBJECTDIR}/_ext/1784119752/pid.p1.d ${OBJECTDIR}/_ext/103087759/epic_serial.p1.d ${OBJECTDIR}/_ext/551636192/epic_taskmgr.p1.d ${OBJECTDIR}/_ext/1067072870/epic_tick.p1.d ${OBJECTDIR}/main.p1.d ${OBJECTDIR}/_ext/1866815852/pic16_irq_table.p1.d

# Object Files
OBJECTFILES=${OBJECTDIR}/_ext/1230679883/epic_math_rand.p1 ${OBJECTDIR}/_ext/1230679883/epic_math_numeric.p1 ${OBJECTDIR}/_ext/1230679883/epic_math_sqrt.p1 ${OBJECTDIR}/_ext/666212198/epic_harness_target.p1 ${OBJECTDIR}/_ext/463256514/pic14_irq.p1 ${OBJECTDIR}/_ext/463256514/pic16_isr_vector.p1 ${OBJECTDIR}/_ext/463256514/pic14_irq_dispatch.p1 ${OBJECTDIR}/_ext/463256514/pic14_wdt_sleep.p1 ${OBJECTDIR}/_ext/463256514/pic14_wdt_sleep_target.p1 ${OBJECTDIR}/_ext/1853308684/pic14_timer0.p1 ${OBJECTDIR}/_ext/1853308684/pic14_usart.p1 ${OBJECTDIR}/_ext/1853308684/pic14_ccp.p1 ${OBJECTDIR}/_ext/1853308684/pic14_comp.p1 ${OBJECTDIR}/_ext/1853308684/pic14_timer1.p1 ${OBJECTDIR}/_ext/1853308684/pic14_timer2.p1 ${OBJECTDIR}/_ext/1853308684/pic14_adc.p1 ${OBJECTDIR}/_ext/1853308684/pic14_eeprom.p1 ${OBJECTDIR}/_ext/1853308684/pic14_vref.p1 ${OBJECTDIR}/_ext/1853308684/pic14_gpio.p1 ${OBJECTDIR}/_ext/1685690910/pic16f87xa_psp.p1 ${OBJECTDIR}/_ext/1853308684/pic14_ssp.p1 ${OBJECTDIR}/_ext/443525851/epic_math_div.p1 ${OBJECTDIR}/_ext/443525851/epic_math_scratch.p1 ${OBJECTDIR}/_ext/443525851/epic_math_mul.p1 ${OBJECTDIR}/_ext/443525851/epic_math_bcd.p1 ${OBJECTDIR}/_ext/443525851/epic_math_addsub.p1 ${OBJECTDIR}/_ext/1576634437/epic_adcfilter.p1 ${OBJECTDIR}/_ext/966866771/epic_bus.p1 ${OBJECTDIR}/_ext/1879587514/debounce.p1 ${OBJECTDIR}/_ext/154072393/encoder.p1 ${OBJECTDIR}/_ext/1774618771/fsm.p1 ${OBJECTDIR}/_ext/1784119752/pid.p1 ${OBJECTDIR}/_ext/103087759/epic_serial.p1 ${OBJECTDIR}/_ext/551636192/epic_taskmgr.p1 ${OBJECTDIR}/_ext/1067072870/epic_tick.p1 ${OBJECTDIR}/main.p1 ${OBJECTDIR}/_ext/1866815852/pic16_irq_table.p1

# Source Files
SOURCEFILES=../../lib/math/src/common/epic_math_rand.c ../../lib/math/src/common/epic_math_numeric.c ../../lib/math/src/common/epic_math_sqrt.c ../../common/src/core/epic_harness_target.c ../../hal/pic14/core/src/core/pic14_irq.c ../../hal/pic14/core/src/target/pic16_isr_vector.c ../../hal/pic14/core/src/core/pic14_irq_dispatch.c ../../hal/pic14/core/src/core/pic14_wdt_sleep.c ../../hal/pic14/core/src/target/pic14_wdt_sleep_target.c ../../hal/pic14/core/src/peripherals/pic14_timer0.c ../../hal/pic14/core/src/peripherals/pic14_usart.c ../../hal/pic14/core/src/peripherals/pic14_ccp.c ../../hal/pic14/core/src/peripherals/pic14_comp.c ../../hal/pic14/core/src/peripherals/pic14_timer1.c ../../hal/pic14/core/src/peripherals/pic14_timer2.c ../../hal/pic14/core/src/peripherals/pic14_adc.c ../../hal/pic14/core/src/peripherals/pic14_eeprom.c ../../hal/pic14/core/src/peripherals/pic14_vref.c ../../hal/pic14/core/src/peripherals/pic14_gpio.c ../../hal/pic14/16f87xa/src/peripherals/pic16f87xa_psp.c ../../hal/pic14/core/src/peripherals/pic14_ssp.c ../../lib/math/src/pic16/epic_math_div.c ../../lib/math/src/pic16/epic_math_scratch.c ../../lib/math/src/pic16/epic_math_mul.c ../../lib/math/src/pic16/epic_math_bcd.c ../../lib/math/src/pic16/epic_math_addsub.c ../../lib/adcfilter/src/epic_adcfilter.c ../../lib/bus/src/epic_bus.c ../../lib/debounce/src/debounce.c ../../lib/encoder/src/encoder.c ../../lib/fsm/src/fsm.c ../../lib/pid/src/pid.c ../../lib/serial/src/epic_serial.c ../../lib/taskmgr/src/epic_taskmgr.c ../../lib/tick/src/epic_tick.c main.c ../../hal/pic14/16f87xa/src/core/pic16_irq_table.c



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
	${MAKE}  -f nbproject/Makefile-default.mk ${DISTDIR}/epic-hal-demo-pic16f87xa.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}

MP_PROCESSOR_OPTION=16F877A
# ------------------------------------------------------------------------------------
# Rules for buildStep: compile
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
${OBJECTDIR}/_ext/1230679883/epic_math_rand.p1: ../../lib/math/src/common/epic_math_rand.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1230679883"
	@${RM} ${OBJECTDIR}/_ext/1230679883/epic_math_rand.p1.d
	@${RM} ${OBJECTDIR}/_ext/1230679883/epic_math_rand.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/1230679883/epic_math_rand.p1 ../../lib/math/src/common/epic_math_rand.c
	@-${MV} ${OBJECTDIR}/_ext/1230679883/epic_math_rand.d ${OBJECTDIR}/_ext/1230679883/epic_math_rand.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1230679883/epic_math_rand.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1230679883/epic_math_numeric.p1: ../../lib/math/src/common/epic_math_numeric.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1230679883"
	@${RM} ${OBJECTDIR}/_ext/1230679883/epic_math_numeric.p1.d
	@${RM} ${OBJECTDIR}/_ext/1230679883/epic_math_numeric.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/1230679883/epic_math_numeric.p1 ../../lib/math/src/common/epic_math_numeric.c
	@-${MV} ${OBJECTDIR}/_ext/1230679883/epic_math_numeric.d ${OBJECTDIR}/_ext/1230679883/epic_math_numeric.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1230679883/epic_math_numeric.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1230679883/epic_math_sqrt.p1: ../../lib/math/src/common/epic_math_sqrt.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1230679883"
	@${RM} ${OBJECTDIR}/_ext/1230679883/epic_math_sqrt.p1.d
	@${RM} ${OBJECTDIR}/_ext/1230679883/epic_math_sqrt.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/1230679883/epic_math_sqrt.p1 ../../lib/math/src/common/epic_math_sqrt.c
	@-${MV} ${OBJECTDIR}/_ext/1230679883/epic_math_sqrt.d ${OBJECTDIR}/_ext/1230679883/epic_math_sqrt.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1230679883/epic_math_sqrt.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/666212198/epic_harness_target.p1: ../../common/src/core/epic_harness_target.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/666212198"
	@${RM} ${OBJECTDIR}/_ext/666212198/epic_harness_target.p1.d
	@${RM} ${OBJECTDIR}/_ext/666212198/epic_harness_target.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/666212198/epic_harness_target.p1 ../../common/src/core/epic_harness_target.c
	@-${MV} ${OBJECTDIR}/_ext/666212198/epic_harness_target.d ${OBJECTDIR}/_ext/666212198/epic_harness_target.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/666212198/epic_harness_target.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/463256514/pic14_irq.p1: ../../hal/pic14/core/src/core/pic14_irq.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/463256514"
	@${RM} ${OBJECTDIR}/_ext/463256514/pic14_irq.p1.d
	@${RM} ${OBJECTDIR}/_ext/463256514/pic14_irq.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/463256514/pic14_irq.p1 ../../hal/pic14/core/src/core/pic14_irq.c
	@-${MV} ${OBJECTDIR}/_ext/463256514/pic14_irq.d ${OBJECTDIR}/_ext/463256514/pic14_irq.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/463256514/pic14_irq.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/463256514/pic16_isr_vector.p1: ../../hal/pic14/core/src/target/pic16_isr_vector.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/463256514"
	@${RM} ${OBJECTDIR}/_ext/463256514/pic16_isr_vector.p1.d
	@${RM} ${OBJECTDIR}/_ext/463256514/pic16_isr_vector.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/463256514/pic16_isr_vector.p1 ../../hal/pic14/core/src/target/pic16_isr_vector.c
	@-${MV} ${OBJECTDIR}/_ext/463256514/pic16_isr_vector.d ${OBJECTDIR}/_ext/463256514/pic16_isr_vector.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/463256514/pic16_isr_vector.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/463256514/pic14_irq_dispatch.p1: ../../hal/pic14/core/src/core/pic14_irq_dispatch.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/463256514"
	@${RM} ${OBJECTDIR}/_ext/463256514/pic14_irq_dispatch.p1.d
	@${RM} ${OBJECTDIR}/_ext/463256514/pic14_irq_dispatch.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/463256514/pic14_irq_dispatch.p1 ../../hal/pic14/core/src/core/pic14_irq_dispatch.c
	@-${MV} ${OBJECTDIR}/_ext/463256514/pic14_irq_dispatch.d ${OBJECTDIR}/_ext/463256514/pic14_irq_dispatch.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/463256514/pic14_irq_dispatch.p1.d $(SILENT) -rsi ${MP_CC_DIR}../
${OBJECTDIR}/_ext/1866815852/pic16_irq_table.p1: ../../hal/pic14/16f87xa/src/core/pic16_irq_table.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1866815852"
	@${RM} ${OBJECTDIR}/_ext/1866815852/pic16_irq_table.p1.d
	@${RM} ${OBJECTDIR}/_ext/1866815852/pic16_irq_table.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/1866815852/pic16_irq_table.p1 ../../hal/pic14/16f87xa/src/core/pic16_irq_table.c
	@-${MV} ${OBJECTDIR}/_ext/1866815852/pic16_irq_table.d ${OBJECTDIR}/_ext/1866815852/pic16_irq_table.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1866815852/pic16_irq_table.p1.d $(SILENT) -rsi ${MP_CC_DIR}../


${OBJECTDIR}/_ext/463256514/pic14_wdt_sleep.p1: ../../hal/pic14/core/src/core/pic14_wdt_sleep.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/463256514"
	@${RM} ${OBJECTDIR}/_ext/463256514/pic14_wdt_sleep.p1.d
	@${RM} ${OBJECTDIR}/_ext/463256514/pic14_wdt_sleep.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/463256514/pic14_wdt_sleep.p1 ../../hal/pic14/core/src/core/pic14_wdt_sleep.c
	@-${MV} ${OBJECTDIR}/_ext/463256514/pic14_wdt_sleep.d ${OBJECTDIR}/_ext/463256514/pic14_wdt_sleep.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/463256514/pic14_wdt_sleep.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/463256514/pic14_wdt_sleep_target.p1: ../../hal/pic14/core/src/target/pic14_wdt_sleep_target.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/463256514"
	@${RM} ${OBJECTDIR}/_ext/463256514/pic14_wdt_sleep_target.p1.d
	@${RM} ${OBJECTDIR}/_ext/463256514/pic14_wdt_sleep_target.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/463256514/pic14_wdt_sleep_target.p1 ../../hal/pic14/core/src/target/pic14_wdt_sleep_target.c
	@-${MV} ${OBJECTDIR}/_ext/463256514/pic14_wdt_sleep_target.d ${OBJECTDIR}/_ext/463256514/pic14_wdt_sleep_target.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/463256514/pic14_wdt_sleep_target.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1853308684/pic14_timer0.p1: ../../hal/pic14/core/src/peripherals/pic14_timer0.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1853308684"
	@${RM} ${OBJECTDIR}/_ext/1853308684/pic14_timer0.p1.d
	@${RM} ${OBJECTDIR}/_ext/1853308684/pic14_timer0.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/1853308684/pic14_timer0.p1 ../../hal/pic14/core/src/peripherals/pic14_timer0.c
	@-${MV} ${OBJECTDIR}/_ext/1853308684/pic14_timer0.d ${OBJECTDIR}/_ext/1853308684/pic14_timer0.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1853308684/pic14_timer0.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1853308684/pic14_usart.p1: ../../hal/pic14/core/src/peripherals/pic14_usart.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1853308684"
	@${RM} ${OBJECTDIR}/_ext/1853308684/pic14_usart.p1.d
	@${RM} ${OBJECTDIR}/_ext/1853308684/pic14_usart.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/1853308684/pic14_usart.p1 ../../hal/pic14/core/src/peripherals/pic14_usart.c
	@-${MV} ${OBJECTDIR}/_ext/1853308684/pic14_usart.d ${OBJECTDIR}/_ext/1853308684/pic14_usart.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1853308684/pic14_usart.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1853308684/pic14_ccp.p1: ../../hal/pic14/core/src/peripherals/pic14_ccp.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1853308684"
	@${RM} ${OBJECTDIR}/_ext/1853308684/pic14_ccp.p1.d
	@${RM} ${OBJECTDIR}/_ext/1853308684/pic14_ccp.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/1853308684/pic14_ccp.p1 ../../hal/pic14/core/src/peripherals/pic14_ccp.c
	@-${MV} ${OBJECTDIR}/_ext/1853308684/pic14_ccp.d ${OBJECTDIR}/_ext/1853308684/pic14_ccp.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1853308684/pic14_ccp.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1853308684/pic14_comp.p1: ../../hal/pic14/core/src/peripherals/pic14_comp.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1853308684"
	@${RM} ${OBJECTDIR}/_ext/1853308684/pic14_comp.p1.d
	@${RM} ${OBJECTDIR}/_ext/1853308684/pic14_comp.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/1853308684/pic14_comp.p1 ../../hal/pic14/core/src/peripherals/pic14_comp.c
	@-${MV} ${OBJECTDIR}/_ext/1853308684/pic14_comp.d ${OBJECTDIR}/_ext/1853308684/pic14_comp.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1853308684/pic14_comp.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1853308684/pic14_timer1.p1: ../../hal/pic14/core/src/peripherals/pic14_timer1.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1853308684"
	@${RM} ${OBJECTDIR}/_ext/1853308684/pic14_timer1.p1.d
	@${RM} ${OBJECTDIR}/_ext/1853308684/pic14_timer1.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/1853308684/pic14_timer1.p1 ../../hal/pic14/core/src/peripherals/pic14_timer1.c
	@-${MV} ${OBJECTDIR}/_ext/1853308684/pic14_timer1.d ${OBJECTDIR}/_ext/1853308684/pic14_timer1.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1853308684/pic14_timer1.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1853308684/pic14_timer2.p1: ../../hal/pic14/core/src/peripherals/pic14_timer2.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1853308684"
	@${RM} ${OBJECTDIR}/_ext/1853308684/pic14_timer2.p1.d
	@${RM} ${OBJECTDIR}/_ext/1853308684/pic14_timer2.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/1853308684/pic14_timer2.p1 ../../hal/pic14/core/src/peripherals/pic14_timer2.c
	@-${MV} ${OBJECTDIR}/_ext/1853308684/pic14_timer2.d ${OBJECTDIR}/_ext/1853308684/pic14_timer2.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1853308684/pic14_timer2.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1853308684/pic14_adc.p1: ../../hal/pic14/core/src/peripherals/pic14_adc.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1853308684"
	@${RM} ${OBJECTDIR}/_ext/1853308684/pic14_adc.p1.d
	@${RM} ${OBJECTDIR}/_ext/1853308684/pic14_adc.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/1853308684/pic14_adc.p1 ../../hal/pic14/core/src/peripherals/pic14_adc.c
	@-${MV} ${OBJECTDIR}/_ext/1853308684/pic14_adc.d ${OBJECTDIR}/_ext/1853308684/pic14_adc.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1853308684/pic14_adc.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1853308684/pic14_eeprom.p1: ../../hal/pic14/core/src/peripherals/pic14_eeprom.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1853308684"
	@${RM} ${OBJECTDIR}/_ext/1853308684/pic14_eeprom.p1.d
	@${RM} ${OBJECTDIR}/_ext/1853308684/pic14_eeprom.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/1853308684/pic14_eeprom.p1 ../../hal/pic14/core/src/peripherals/pic14_eeprom.c
	@-${MV} ${OBJECTDIR}/_ext/1853308684/pic14_eeprom.d ${OBJECTDIR}/_ext/1853308684/pic14_eeprom.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1853308684/pic14_eeprom.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1853308684/pic14_vref.p1: ../../hal/pic14/core/src/peripherals/pic14_vref.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1853308684"
	@${RM} ${OBJECTDIR}/_ext/1853308684/pic14_vref.p1.d
	@${RM} ${OBJECTDIR}/_ext/1853308684/pic14_vref.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/1853308684/pic14_vref.p1 ../../hal/pic14/core/src/peripherals/pic14_vref.c
	@-${MV} ${OBJECTDIR}/_ext/1853308684/pic14_vref.d ${OBJECTDIR}/_ext/1853308684/pic14_vref.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1853308684/pic14_vref.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1853308684/pic14_gpio.p1: ../../hal/pic14/core/src/peripherals/pic14_gpio.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1853308684"
	@${RM} ${OBJECTDIR}/_ext/1853308684/pic14_gpio.p1.d
	@${RM} ${OBJECTDIR}/_ext/1853308684/pic14_gpio.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/1853308684/pic14_gpio.p1 ../../hal/pic14/core/src/peripherals/pic14_gpio.c
	@-${MV} ${OBJECTDIR}/_ext/1853308684/pic14_gpio.d ${OBJECTDIR}/_ext/1853308684/pic14_gpio.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1853308684/pic14_gpio.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1685690910/pic16f87xa_psp.p1: ../../hal/pic14/16f87xa/src/peripherals/pic16f87xa_psp.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1685690910"
	@${RM} ${OBJECTDIR}/_ext/1685690910/pic16f87xa_psp.p1.d
	@${RM} ${OBJECTDIR}/_ext/1685690910/pic16f87xa_psp.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/1685690910/pic16f87xa_psp.p1 ../../hal/pic14/16f87xa/src/peripherals/pic16f87xa_psp.c
	@-${MV} ${OBJECTDIR}/_ext/1685690910/pic16f87xa_psp.d ${OBJECTDIR}/_ext/1685690910/pic16f87xa_psp.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1685690910/pic16f87xa_psp.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1853308684/pic14_ssp.p1: ../../hal/pic14/core/src/peripherals/pic14_ssp.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1853308684"
	@${RM} ${OBJECTDIR}/_ext/1853308684/pic14_ssp.p1.d
	@${RM} ${OBJECTDIR}/_ext/1853308684/pic14_ssp.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/1853308684/pic14_ssp.p1 ../../hal/pic14/core/src/peripherals/pic14_ssp.c
	@-${MV} ${OBJECTDIR}/_ext/1853308684/pic14_ssp.d ${OBJECTDIR}/_ext/1853308684/pic14_ssp.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1853308684/pic14_ssp.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/443525851/epic_math_div.p1: ../../lib/math/src/pic16/epic_math_div.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/443525851"
	@${RM} ${OBJECTDIR}/_ext/443525851/epic_math_div.p1.d
	@${RM} ${OBJECTDIR}/_ext/443525851/epic_math_div.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/443525851/epic_math_div.p1 ../../lib/math/src/pic16/epic_math_div.c
	@-${MV} ${OBJECTDIR}/_ext/443525851/epic_math_div.d ${OBJECTDIR}/_ext/443525851/epic_math_div.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/443525851/epic_math_div.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/443525851/epic_math_scratch.p1: ../../lib/math/src/pic16/epic_math_scratch.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/443525851"
	@${RM} ${OBJECTDIR}/_ext/443525851/epic_math_scratch.p1.d
	@${RM} ${OBJECTDIR}/_ext/443525851/epic_math_scratch.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/443525851/epic_math_scratch.p1 ../../lib/math/src/pic16/epic_math_scratch.c
	@-${MV} ${OBJECTDIR}/_ext/443525851/epic_math_scratch.d ${OBJECTDIR}/_ext/443525851/epic_math_scratch.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/443525851/epic_math_scratch.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/443525851/epic_math_mul.p1: ../../lib/math/src/pic16/epic_math_mul.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/443525851"
	@${RM} ${OBJECTDIR}/_ext/443525851/epic_math_mul.p1.d
	@${RM} ${OBJECTDIR}/_ext/443525851/epic_math_mul.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/443525851/epic_math_mul.p1 ../../lib/math/src/pic16/epic_math_mul.c
	@-${MV} ${OBJECTDIR}/_ext/443525851/epic_math_mul.d ${OBJECTDIR}/_ext/443525851/epic_math_mul.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/443525851/epic_math_mul.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/443525851/epic_math_bcd.p1: ../../lib/math/src/pic16/epic_math_bcd.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/443525851"
	@${RM} ${OBJECTDIR}/_ext/443525851/epic_math_bcd.p1.d
	@${RM} ${OBJECTDIR}/_ext/443525851/epic_math_bcd.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/443525851/epic_math_bcd.p1 ../../lib/math/src/pic16/epic_math_bcd.c
	@-${MV} ${OBJECTDIR}/_ext/443525851/epic_math_bcd.d ${OBJECTDIR}/_ext/443525851/epic_math_bcd.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/443525851/epic_math_bcd.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/443525851/epic_math_addsub.p1: ../../lib/math/src/pic16/epic_math_addsub.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/443525851"
	@${RM} ${OBJECTDIR}/_ext/443525851/epic_math_addsub.p1.d
	@${RM} ${OBJECTDIR}/_ext/443525851/epic_math_addsub.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/443525851/epic_math_addsub.p1 ../../lib/math/src/pic16/epic_math_addsub.c
	@-${MV} ${OBJECTDIR}/_ext/443525851/epic_math_addsub.d ${OBJECTDIR}/_ext/443525851/epic_math_addsub.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/443525851/epic_math_addsub.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1576634437/epic_adcfilter.p1: ../../lib/adcfilter/src/epic_adcfilter.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1576634437"
	@${RM} ${OBJECTDIR}/_ext/1576634437/epic_adcfilter.p1.d
	@${RM} ${OBJECTDIR}/_ext/1576634437/epic_adcfilter.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/1576634437/epic_adcfilter.p1 ../../lib/adcfilter/src/epic_adcfilter.c
	@-${MV} ${OBJECTDIR}/_ext/1576634437/epic_adcfilter.d ${OBJECTDIR}/_ext/1576634437/epic_adcfilter.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1576634437/epic_adcfilter.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/966866771/epic_bus.p1: ../../lib/bus/src/epic_bus.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/966866771"
	@${RM} ${OBJECTDIR}/_ext/966866771/epic_bus.p1.d
	@${RM} ${OBJECTDIR}/_ext/966866771/epic_bus.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/966866771/epic_bus.p1 ../../lib/bus/src/epic_bus.c
	@-${MV} ${OBJECTDIR}/_ext/966866771/epic_bus.d ${OBJECTDIR}/_ext/966866771/epic_bus.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/966866771/epic_bus.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1879587514/debounce.p1: ../../lib/debounce/src/debounce.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1879587514"
	@${RM} ${OBJECTDIR}/_ext/1879587514/debounce.p1.d
	@${RM} ${OBJECTDIR}/_ext/1879587514/debounce.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/1879587514/debounce.p1 ../../lib/debounce/src/debounce.c
	@-${MV} ${OBJECTDIR}/_ext/1879587514/debounce.d ${OBJECTDIR}/_ext/1879587514/debounce.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1879587514/debounce.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/154072393/encoder.p1: ../../lib/encoder/src/encoder.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/154072393"
	@${RM} ${OBJECTDIR}/_ext/154072393/encoder.p1.d
	@${RM} ${OBJECTDIR}/_ext/154072393/encoder.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/154072393/encoder.p1 ../../lib/encoder/src/encoder.c
	@-${MV} ${OBJECTDIR}/_ext/154072393/encoder.d ${OBJECTDIR}/_ext/154072393/encoder.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/154072393/encoder.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1774618771/fsm.p1: ../../lib/fsm/src/fsm.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1774618771"
	@${RM} ${OBJECTDIR}/_ext/1774618771/fsm.p1.d
	@${RM} ${OBJECTDIR}/_ext/1774618771/fsm.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/1774618771/fsm.p1 ../../lib/fsm/src/fsm.c
	@-${MV} ${OBJECTDIR}/_ext/1774618771/fsm.d ${OBJECTDIR}/_ext/1774618771/fsm.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1774618771/fsm.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1784119752/pid.p1: ../../lib/pid/src/pid.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1784119752"
	@${RM} ${OBJECTDIR}/_ext/1784119752/pid.p1.d
	@${RM} ${OBJECTDIR}/_ext/1784119752/pid.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/1784119752/pid.p1 ../../lib/pid/src/pid.c
	@-${MV} ${OBJECTDIR}/_ext/1784119752/pid.d ${OBJECTDIR}/_ext/1784119752/pid.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1784119752/pid.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/103087759/epic_serial.p1: ../../lib/serial/src/epic_serial.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/103087759"
	@${RM} ${OBJECTDIR}/_ext/103087759/epic_serial.p1.d
	@${RM} ${OBJECTDIR}/_ext/103087759/epic_serial.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/103087759/epic_serial.p1 ../../lib/serial/src/epic_serial.c
	@-${MV} ${OBJECTDIR}/_ext/103087759/epic_serial.d ${OBJECTDIR}/_ext/103087759/epic_serial.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/103087759/epic_serial.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/551636192/epic_taskmgr.p1: ../../lib/taskmgr/src/epic_taskmgr.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/551636192"
	@${RM} ${OBJECTDIR}/_ext/551636192/epic_taskmgr.p1.d
	@${RM} ${OBJECTDIR}/_ext/551636192/epic_taskmgr.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/551636192/epic_taskmgr.p1 ../../lib/taskmgr/src/epic_taskmgr.c
	@-${MV} ${OBJECTDIR}/_ext/551636192/epic_taskmgr.d ${OBJECTDIR}/_ext/551636192/epic_taskmgr.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/551636192/epic_taskmgr.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1067072870/epic_tick.p1: ../../lib/tick/src/epic_tick.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1067072870"
	@${RM} ${OBJECTDIR}/_ext/1067072870/epic_tick.p1.d
	@${RM} ${OBJECTDIR}/_ext/1067072870/epic_tick.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/1067072870/epic_tick.p1 ../../lib/tick/src/epic_tick.c
	@-${MV} ${OBJECTDIR}/_ext/1067072870/epic_tick.d ${OBJECTDIR}/_ext/1067072870/epic_tick.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1067072870/epic_tick.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/main.p1: main.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}"
	@${RM} ${OBJECTDIR}/main.p1.d
	@${RM} ${OBJECTDIR}/main.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1  -mdebugger=none   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/main.p1 main.c
	@-${MV} ${OBJECTDIR}/main.d ${OBJECTDIR}/main.p1.d
	@${FIXDEPS} ${OBJECTDIR}/main.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

else
${OBJECTDIR}/_ext/1230679883/epic_math_rand.p1: ../../lib/math/src/common/epic_math_rand.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1230679883"
	@${RM} ${OBJECTDIR}/_ext/1230679883/epic_math_rand.p1.d
	@${RM} ${OBJECTDIR}/_ext/1230679883/epic_math_rand.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/1230679883/epic_math_rand.p1 ../../lib/math/src/common/epic_math_rand.c
	@-${MV} ${OBJECTDIR}/_ext/1230679883/epic_math_rand.d ${OBJECTDIR}/_ext/1230679883/epic_math_rand.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1230679883/epic_math_rand.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1230679883/epic_math_numeric.p1: ../../lib/math/src/common/epic_math_numeric.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1230679883"
	@${RM} ${OBJECTDIR}/_ext/1230679883/epic_math_numeric.p1.d
	@${RM} ${OBJECTDIR}/_ext/1230679883/epic_math_numeric.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/1230679883/epic_math_numeric.p1 ../../lib/math/src/common/epic_math_numeric.c
	@-${MV} ${OBJECTDIR}/_ext/1230679883/epic_math_numeric.d ${OBJECTDIR}/_ext/1230679883/epic_math_numeric.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1230679883/epic_math_numeric.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1230679883/epic_math_sqrt.p1: ../../lib/math/src/common/epic_math_sqrt.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1230679883"
	@${RM} ${OBJECTDIR}/_ext/1230679883/epic_math_sqrt.p1.d
	@${RM} ${OBJECTDIR}/_ext/1230679883/epic_math_sqrt.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/1230679883/epic_math_sqrt.p1 ../../lib/math/src/common/epic_math_sqrt.c
	@-${MV} ${OBJECTDIR}/_ext/1230679883/epic_math_sqrt.d ${OBJECTDIR}/_ext/1230679883/epic_math_sqrt.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1230679883/epic_math_sqrt.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/666212198/epic_harness_target.p1: ../../common/src/core/epic_harness_target.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/666212198"
	@${RM} ${OBJECTDIR}/_ext/666212198/epic_harness_target.p1.d
	@${RM} ${OBJECTDIR}/_ext/666212198/epic_harness_target.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/666212198/epic_harness_target.p1 ../../common/src/core/epic_harness_target.c
	@-${MV} ${OBJECTDIR}/_ext/666212198/epic_harness_target.d ${OBJECTDIR}/_ext/666212198/epic_harness_target.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/666212198/epic_harness_target.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/463256514/pic14_irq.p1: ../../hal/pic14/core/src/core/pic14_irq.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/463256514"
	@${RM} ${OBJECTDIR}/_ext/463256514/pic14_irq.p1.d
	@${RM} ${OBJECTDIR}/_ext/463256514/pic14_irq.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/463256514/pic14_irq.p1 ../../hal/pic14/core/src/core/pic14_irq.c
	@-${MV} ${OBJECTDIR}/_ext/463256514/pic14_irq.d ${OBJECTDIR}/_ext/463256514/pic14_irq.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/463256514/pic14_irq.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/463256514/pic16_isr_vector.p1: ../../hal/pic14/core/src/target/pic16_isr_vector.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/463256514"
	@${RM} ${OBJECTDIR}/_ext/463256514/pic16_isr_vector.p1.d
	@${RM} ${OBJECTDIR}/_ext/463256514/pic16_isr_vector.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/463256514/pic16_isr_vector.p1 ../../hal/pic14/core/src/target/pic16_isr_vector.c
	@-${MV} ${OBJECTDIR}/_ext/463256514/pic16_isr_vector.d ${OBJECTDIR}/_ext/463256514/pic16_isr_vector.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/463256514/pic16_isr_vector.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/463256514/pic14_irq_dispatch.p1: ../../hal/pic14/core/src/core/pic14_irq_dispatch.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/463256514"
	@${RM} ${OBJECTDIR}/_ext/463256514/pic14_irq_dispatch.p1.d
	@${RM} ${OBJECTDIR}/_ext/463256514/pic14_irq_dispatch.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/463256514/pic14_irq_dispatch.p1 ../../hal/pic14/core/src/core/pic14_irq_dispatch.c
	@-${MV} ${OBJECTDIR}/_ext/463256514/pic14_irq_dispatch.d ${OBJECTDIR}/_ext/463256514/pic14_irq_dispatch.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/463256514/pic14_irq_dispatch.p1.d $(SILENT) -rsi ${MP_CC_DIR}../
${OBJECTDIR}/_ext/1866815852/pic16_irq_table.p1: ../../hal/pic14/16f87xa/src/core/pic16_irq_table.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1866815852"
	@${RM} ${OBJECTDIR}/_ext/1866815852/pic16_irq_table.p1.d
	@${RM} ${OBJECTDIR}/_ext/1866815852/pic16_irq_table.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/1866815852/pic16_irq_table.p1 ../../hal/pic14/16f87xa/src/core/pic16_irq_table.c
	@-${MV} ${OBJECTDIR}/_ext/1866815852/pic16_irq_table.d ${OBJECTDIR}/_ext/1866815852/pic16_irq_table.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1866815852/pic16_irq_table.p1.d $(SILENT) -rsi ${MP_CC_DIR}../


${OBJECTDIR}/_ext/463256514/pic14_wdt_sleep.p1: ../../hal/pic14/core/src/core/pic14_wdt_sleep.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/463256514"
	@${RM} ${OBJECTDIR}/_ext/463256514/pic14_wdt_sleep.p1.d
	@${RM} ${OBJECTDIR}/_ext/463256514/pic14_wdt_sleep.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/463256514/pic14_wdt_sleep.p1 ../../hal/pic14/core/src/core/pic14_wdt_sleep.c
	@-${MV} ${OBJECTDIR}/_ext/463256514/pic14_wdt_sleep.d ${OBJECTDIR}/_ext/463256514/pic14_wdt_sleep.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/463256514/pic14_wdt_sleep.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/463256514/pic14_wdt_sleep_target.p1: ../../hal/pic14/core/src/target/pic14_wdt_sleep_target.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/463256514"
	@${RM} ${OBJECTDIR}/_ext/463256514/pic14_wdt_sleep_target.p1.d
	@${RM} ${OBJECTDIR}/_ext/463256514/pic14_wdt_sleep_target.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/463256514/pic14_wdt_sleep_target.p1 ../../hal/pic14/core/src/target/pic14_wdt_sleep_target.c
	@-${MV} ${OBJECTDIR}/_ext/463256514/pic14_wdt_sleep_target.d ${OBJECTDIR}/_ext/463256514/pic14_wdt_sleep_target.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/463256514/pic14_wdt_sleep_target.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1853308684/pic14_timer0.p1: ../../hal/pic14/core/src/peripherals/pic14_timer0.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1853308684"
	@${RM} ${OBJECTDIR}/_ext/1853308684/pic14_timer0.p1.d
	@${RM} ${OBJECTDIR}/_ext/1853308684/pic14_timer0.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/1853308684/pic14_timer0.p1 ../../hal/pic14/core/src/peripherals/pic14_timer0.c
	@-${MV} ${OBJECTDIR}/_ext/1853308684/pic14_timer0.d ${OBJECTDIR}/_ext/1853308684/pic14_timer0.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1853308684/pic14_timer0.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1853308684/pic14_usart.p1: ../../hal/pic14/core/src/peripherals/pic14_usart.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1853308684"
	@${RM} ${OBJECTDIR}/_ext/1853308684/pic14_usart.p1.d
	@${RM} ${OBJECTDIR}/_ext/1853308684/pic14_usart.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/1853308684/pic14_usart.p1 ../../hal/pic14/core/src/peripherals/pic14_usart.c
	@-${MV} ${OBJECTDIR}/_ext/1853308684/pic14_usart.d ${OBJECTDIR}/_ext/1853308684/pic14_usart.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1853308684/pic14_usart.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1853308684/pic14_ccp.p1: ../../hal/pic14/core/src/peripherals/pic14_ccp.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1853308684"
	@${RM} ${OBJECTDIR}/_ext/1853308684/pic14_ccp.p1.d
	@${RM} ${OBJECTDIR}/_ext/1853308684/pic14_ccp.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/1853308684/pic14_ccp.p1 ../../hal/pic14/core/src/peripherals/pic14_ccp.c
	@-${MV} ${OBJECTDIR}/_ext/1853308684/pic14_ccp.d ${OBJECTDIR}/_ext/1853308684/pic14_ccp.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1853308684/pic14_ccp.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1853308684/pic14_comp.p1: ../../hal/pic14/core/src/peripherals/pic14_comp.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1853308684"
	@${RM} ${OBJECTDIR}/_ext/1853308684/pic14_comp.p1.d
	@${RM} ${OBJECTDIR}/_ext/1853308684/pic14_comp.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/1853308684/pic14_comp.p1 ../../hal/pic14/core/src/peripherals/pic14_comp.c
	@-${MV} ${OBJECTDIR}/_ext/1853308684/pic14_comp.d ${OBJECTDIR}/_ext/1853308684/pic14_comp.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1853308684/pic14_comp.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1853308684/pic14_timer1.p1: ../../hal/pic14/core/src/peripherals/pic14_timer1.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1853308684"
	@${RM} ${OBJECTDIR}/_ext/1853308684/pic14_timer1.p1.d
	@${RM} ${OBJECTDIR}/_ext/1853308684/pic14_timer1.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/1853308684/pic14_timer1.p1 ../../hal/pic14/core/src/peripherals/pic14_timer1.c
	@-${MV} ${OBJECTDIR}/_ext/1853308684/pic14_timer1.d ${OBJECTDIR}/_ext/1853308684/pic14_timer1.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1853308684/pic14_timer1.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1853308684/pic14_timer2.p1: ../../hal/pic14/core/src/peripherals/pic14_timer2.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1853308684"
	@${RM} ${OBJECTDIR}/_ext/1853308684/pic14_timer2.p1.d
	@${RM} ${OBJECTDIR}/_ext/1853308684/pic14_timer2.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/1853308684/pic14_timer2.p1 ../../hal/pic14/core/src/peripherals/pic14_timer2.c
	@-${MV} ${OBJECTDIR}/_ext/1853308684/pic14_timer2.d ${OBJECTDIR}/_ext/1853308684/pic14_timer2.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1853308684/pic14_timer2.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1853308684/pic14_adc.p1: ../../hal/pic14/core/src/peripherals/pic14_adc.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1853308684"
	@${RM} ${OBJECTDIR}/_ext/1853308684/pic14_adc.p1.d
	@${RM} ${OBJECTDIR}/_ext/1853308684/pic14_adc.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/1853308684/pic14_adc.p1 ../../hal/pic14/core/src/peripherals/pic14_adc.c
	@-${MV} ${OBJECTDIR}/_ext/1853308684/pic14_adc.d ${OBJECTDIR}/_ext/1853308684/pic14_adc.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1853308684/pic14_adc.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1853308684/pic14_eeprom.p1: ../../hal/pic14/core/src/peripherals/pic14_eeprom.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1853308684"
	@${RM} ${OBJECTDIR}/_ext/1853308684/pic14_eeprom.p1.d
	@${RM} ${OBJECTDIR}/_ext/1853308684/pic14_eeprom.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/1853308684/pic14_eeprom.p1 ../../hal/pic14/core/src/peripherals/pic14_eeprom.c
	@-${MV} ${OBJECTDIR}/_ext/1853308684/pic14_eeprom.d ${OBJECTDIR}/_ext/1853308684/pic14_eeprom.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1853308684/pic14_eeprom.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1853308684/pic14_vref.p1: ../../hal/pic14/core/src/peripherals/pic14_vref.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1853308684"
	@${RM} ${OBJECTDIR}/_ext/1853308684/pic14_vref.p1.d
	@${RM} ${OBJECTDIR}/_ext/1853308684/pic14_vref.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/1853308684/pic14_vref.p1 ../../hal/pic14/core/src/peripherals/pic14_vref.c
	@-${MV} ${OBJECTDIR}/_ext/1853308684/pic14_vref.d ${OBJECTDIR}/_ext/1853308684/pic14_vref.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1853308684/pic14_vref.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1853308684/pic14_gpio.p1: ../../hal/pic14/core/src/peripherals/pic14_gpio.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1853308684"
	@${RM} ${OBJECTDIR}/_ext/1853308684/pic14_gpio.p1.d
	@${RM} ${OBJECTDIR}/_ext/1853308684/pic14_gpio.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/1853308684/pic14_gpio.p1 ../../hal/pic14/core/src/peripherals/pic14_gpio.c
	@-${MV} ${OBJECTDIR}/_ext/1853308684/pic14_gpio.d ${OBJECTDIR}/_ext/1853308684/pic14_gpio.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1853308684/pic14_gpio.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1685690910/pic16f87xa_psp.p1: ../../hal/pic14/16f87xa/src/peripherals/pic16f87xa_psp.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1685690910"
	@${RM} ${OBJECTDIR}/_ext/1685690910/pic16f87xa_psp.p1.d
	@${RM} ${OBJECTDIR}/_ext/1685690910/pic16f87xa_psp.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/1685690910/pic16f87xa_psp.p1 ../../hal/pic14/16f87xa/src/peripherals/pic16f87xa_psp.c
	@-${MV} ${OBJECTDIR}/_ext/1685690910/pic16f87xa_psp.d ${OBJECTDIR}/_ext/1685690910/pic16f87xa_psp.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1685690910/pic16f87xa_psp.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1853308684/pic14_ssp.p1: ../../hal/pic14/core/src/peripherals/pic14_ssp.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1853308684"
	@${RM} ${OBJECTDIR}/_ext/1853308684/pic14_ssp.p1.d
	@${RM} ${OBJECTDIR}/_ext/1853308684/pic14_ssp.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/1853308684/pic14_ssp.p1 ../../hal/pic14/core/src/peripherals/pic14_ssp.c
	@-${MV} ${OBJECTDIR}/_ext/1853308684/pic14_ssp.d ${OBJECTDIR}/_ext/1853308684/pic14_ssp.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1853308684/pic14_ssp.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/443525851/epic_math_div.p1: ../../lib/math/src/pic16/epic_math_div.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/443525851"
	@${RM} ${OBJECTDIR}/_ext/443525851/epic_math_div.p1.d
	@${RM} ${OBJECTDIR}/_ext/443525851/epic_math_div.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/443525851/epic_math_div.p1 ../../lib/math/src/pic16/epic_math_div.c
	@-${MV} ${OBJECTDIR}/_ext/443525851/epic_math_div.d ${OBJECTDIR}/_ext/443525851/epic_math_div.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/443525851/epic_math_div.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/443525851/epic_math_scratch.p1: ../../lib/math/src/pic16/epic_math_scratch.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/443525851"
	@${RM} ${OBJECTDIR}/_ext/443525851/epic_math_scratch.p1.d
	@${RM} ${OBJECTDIR}/_ext/443525851/epic_math_scratch.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/443525851/epic_math_scratch.p1 ../../lib/math/src/pic16/epic_math_scratch.c
	@-${MV} ${OBJECTDIR}/_ext/443525851/epic_math_scratch.d ${OBJECTDIR}/_ext/443525851/epic_math_scratch.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/443525851/epic_math_scratch.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/443525851/epic_math_mul.p1: ../../lib/math/src/pic16/epic_math_mul.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/443525851"
	@${RM} ${OBJECTDIR}/_ext/443525851/epic_math_mul.p1.d
	@${RM} ${OBJECTDIR}/_ext/443525851/epic_math_mul.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/443525851/epic_math_mul.p1 ../../lib/math/src/pic16/epic_math_mul.c
	@-${MV} ${OBJECTDIR}/_ext/443525851/epic_math_mul.d ${OBJECTDIR}/_ext/443525851/epic_math_mul.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/443525851/epic_math_mul.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/443525851/epic_math_bcd.p1: ../../lib/math/src/pic16/epic_math_bcd.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/443525851"
	@${RM} ${OBJECTDIR}/_ext/443525851/epic_math_bcd.p1.d
	@${RM} ${OBJECTDIR}/_ext/443525851/epic_math_bcd.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/443525851/epic_math_bcd.p1 ../../lib/math/src/pic16/epic_math_bcd.c
	@-${MV} ${OBJECTDIR}/_ext/443525851/epic_math_bcd.d ${OBJECTDIR}/_ext/443525851/epic_math_bcd.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/443525851/epic_math_bcd.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/443525851/epic_math_addsub.p1: ../../lib/math/src/pic16/epic_math_addsub.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/443525851"
	@${RM} ${OBJECTDIR}/_ext/443525851/epic_math_addsub.p1.d
	@${RM} ${OBJECTDIR}/_ext/443525851/epic_math_addsub.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/443525851/epic_math_addsub.p1 ../../lib/math/src/pic16/epic_math_addsub.c
	@-${MV} ${OBJECTDIR}/_ext/443525851/epic_math_addsub.d ${OBJECTDIR}/_ext/443525851/epic_math_addsub.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/443525851/epic_math_addsub.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1576634437/epic_adcfilter.p1: ../../lib/adcfilter/src/epic_adcfilter.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1576634437"
	@${RM} ${OBJECTDIR}/_ext/1576634437/epic_adcfilter.p1.d
	@${RM} ${OBJECTDIR}/_ext/1576634437/epic_adcfilter.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/1576634437/epic_adcfilter.p1 ../../lib/adcfilter/src/epic_adcfilter.c
	@-${MV} ${OBJECTDIR}/_ext/1576634437/epic_adcfilter.d ${OBJECTDIR}/_ext/1576634437/epic_adcfilter.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1576634437/epic_adcfilter.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/966866771/epic_bus.p1: ../../lib/bus/src/epic_bus.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/966866771"
	@${RM} ${OBJECTDIR}/_ext/966866771/epic_bus.p1.d
	@${RM} ${OBJECTDIR}/_ext/966866771/epic_bus.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/966866771/epic_bus.p1 ../../lib/bus/src/epic_bus.c
	@-${MV} ${OBJECTDIR}/_ext/966866771/epic_bus.d ${OBJECTDIR}/_ext/966866771/epic_bus.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/966866771/epic_bus.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1879587514/debounce.p1: ../../lib/debounce/src/debounce.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1879587514"
	@${RM} ${OBJECTDIR}/_ext/1879587514/debounce.p1.d
	@${RM} ${OBJECTDIR}/_ext/1879587514/debounce.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/1879587514/debounce.p1 ../../lib/debounce/src/debounce.c
	@-${MV} ${OBJECTDIR}/_ext/1879587514/debounce.d ${OBJECTDIR}/_ext/1879587514/debounce.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1879587514/debounce.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/154072393/encoder.p1: ../../lib/encoder/src/encoder.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/154072393"
	@${RM} ${OBJECTDIR}/_ext/154072393/encoder.p1.d
	@${RM} ${OBJECTDIR}/_ext/154072393/encoder.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/154072393/encoder.p1 ../../lib/encoder/src/encoder.c
	@-${MV} ${OBJECTDIR}/_ext/154072393/encoder.d ${OBJECTDIR}/_ext/154072393/encoder.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/154072393/encoder.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1774618771/fsm.p1: ../../lib/fsm/src/fsm.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1774618771"
	@${RM} ${OBJECTDIR}/_ext/1774618771/fsm.p1.d
	@${RM} ${OBJECTDIR}/_ext/1774618771/fsm.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/1774618771/fsm.p1 ../../lib/fsm/src/fsm.c
	@-${MV} ${OBJECTDIR}/_ext/1774618771/fsm.d ${OBJECTDIR}/_ext/1774618771/fsm.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1774618771/fsm.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1784119752/pid.p1: ../../lib/pid/src/pid.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1784119752"
	@${RM} ${OBJECTDIR}/_ext/1784119752/pid.p1.d
	@${RM} ${OBJECTDIR}/_ext/1784119752/pid.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/1784119752/pid.p1 ../../lib/pid/src/pid.c
	@-${MV} ${OBJECTDIR}/_ext/1784119752/pid.d ${OBJECTDIR}/_ext/1784119752/pid.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1784119752/pid.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/103087759/epic_serial.p1: ../../lib/serial/src/epic_serial.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/103087759"
	@${RM} ${OBJECTDIR}/_ext/103087759/epic_serial.p1.d
	@${RM} ${OBJECTDIR}/_ext/103087759/epic_serial.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/103087759/epic_serial.p1 ../../lib/serial/src/epic_serial.c
	@-${MV} ${OBJECTDIR}/_ext/103087759/epic_serial.d ${OBJECTDIR}/_ext/103087759/epic_serial.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/103087759/epic_serial.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/551636192/epic_taskmgr.p1: ../../lib/taskmgr/src/epic_taskmgr.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/551636192"
	@${RM} ${OBJECTDIR}/_ext/551636192/epic_taskmgr.p1.d
	@${RM} ${OBJECTDIR}/_ext/551636192/epic_taskmgr.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/551636192/epic_taskmgr.p1 ../../lib/taskmgr/src/epic_taskmgr.c
	@-${MV} ${OBJECTDIR}/_ext/551636192/epic_taskmgr.d ${OBJECTDIR}/_ext/551636192/epic_taskmgr.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/551636192/epic_taskmgr.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/1067072870/epic_tick.p1: ../../lib/tick/src/epic_tick.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1067072870"
	@${RM} ${OBJECTDIR}/_ext/1067072870/epic_tick.p1.d
	@${RM} ${OBJECTDIR}/_ext/1067072870/epic_tick.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/_ext/1067072870/epic_tick.p1 ../../lib/tick/src/epic_tick.c
	@-${MV} ${OBJECTDIR}/_ext/1067072870/epic_tick.d ${OBJECTDIR}/_ext/1067072870/epic_tick.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/1067072870/epic_tick.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/main.p1: main.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}"
	@${RM} ${OBJECTDIR}/main.p1.d
	@${RM} ${OBJECTDIR}/main.p1
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     -o ${OBJECTDIR}/main.p1 main.c
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
${DISTDIR}/epic-hal-demo-pic16f87xa.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${DISTDIR}
	${MP_CC} $(MP_EXTRA_LD_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -Wl,-Map=${DISTDIR}/epic-hal-demo-pic16f87xa.X.${IMAGE_TYPE}.map  -D__DEBUG=1  -mdebugger=none  -DXPRJ_default=$(CND_CONF)  -Wl,--defsym=__MPLAB_BUILD=1   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits -std=c99 -gdwarf-3 -mstack=compiled:auto:auto        $(COMPARISON_BUILD) -Wl,--memorysummary,${DISTDIR}/memoryfile.xml -o ${DISTDIR}/epic-hal-demo-pic16f87xa.X.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX}  ${OBJECTFILES_QUOTED_IF_SPACED}
	@${RM} ${DISTDIR}/epic-hal-demo-pic16f87xa.X.${IMAGE_TYPE}.hex


else
${DISTDIR}/epic-hal-demo-pic16f87xa.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${DISTDIR}
	${MP_CC} $(MP_EXTRA_LD_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -Wl,-Map=${DISTDIR}/epic-hal-demo-pic16f87xa.X.${IMAGE_TYPE}.map  -DXPRJ_default=$(CND_CONF)  -Wl,--defsym=__MPLAB_BUILD=1   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F877A -DFOSC_HZ=20000000 -xassembler-with-cpp -I"../../hal/pic14/16f87xa/include/target" -I"../../hal/pic14/16f87xa/include" -I"../../hal/pic14/core/include" -I"../../common/include" -I"../../lib/adcfilter/include" -I"../../lib/bus/include" -I"../../lib/debounce/include" -I"../../lib/encoder/include" -I"../../lib/fsm/include" -I"../../lib/math/include" -I"../../lib/math/tests" -I"../../lib/pid/include" -I"../../lib/serial/include" -I"../../lib/taskmgr/include" -I"../../lib/tick/include" -mwarn=-3 -Wa,-a -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mno-default-config-bits -std=c99 -gdwarf-3 -mstack=compiled:auto:auto     $(COMPARISON_BUILD) -Wl,--memorysummary,${DISTDIR}/memoryfile.xml -o ${DISTDIR}/epic-hal-demo-pic16f87xa.X.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX}  ${OBJECTFILES_QUOTED_IF_SPACED}


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
