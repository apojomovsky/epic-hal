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
FINAL_IMAGE=${DISTDIR}/epic-hal-demo-pic16f5x.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
else
IMAGE_TYPE=production
OUTPUT_SUFFIX=hex
DEBUGGABLE_SUFFIX=elf
FINAL_IMAGE=${DISTDIR}/epic-hal-demo-pic16f5x.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
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
SOURCEFILES_QUOTED_IF_SPACED=main.c ../../pic16f5x-hal/src/peripherals/pic16f5x_gpio.c ../../pic16f5x-hal/src/peripherals/pic16f5x_timer0.c ../../pic16f5x-hal/src/core/pic16_irq_stub.c ../../pic16f5x-hal/src/core/pic16_irq_dispatch.c ../../pic16f5x-hal/src/target/pic16f5x_wdt_sleep_target.c ../../epic-common/src/core/epic_harness_target.c

# Object Files Quoted if spaced
OBJECTFILES_QUOTED_IF_SPACED=${OBJECTDIR}/main.p1 ${OBJECTDIR}/_ext/9990001/pic16f5x_gpio.p1 ${OBJECTDIR}/_ext/9990001/pic16f5x_timer0.p1 ${OBJECTDIR}/_ext/9990001/pic16_irq_stub.p1 ${OBJECTDIR}/_ext/9990001/pic16_irq_dispatch.p1 ${OBJECTDIR}/_ext/9990001/pic16f5x_wdt_sleep_target.p1 ${OBJECTDIR}/_ext/9990001/epic_harness_target.p1

POSSIBLE_DEPFILES=${OBJECTDIR}/main.p1.d ${OBJECTDIR}/_ext/9990001/pic16f5x_gpio.p1.d ${OBJECTDIR}/_ext/9990001/pic16f5x_timer0.p1.d ${OBJECTDIR}/_ext/9990001/pic16_irq_stub.p1.d ${OBJECTDIR}/_ext/9990001/pic16_irq_dispatch.p1.d ${OBJECTDIR}/_ext/9990001/pic16f5x_wdt_sleep_target.p1.d ${OBJECTDIR}/_ext/9990001/epic_harness_target.p1.d

# Object Files
OBJECTFILES=${OBJECTDIR}/main.p1 ${OBJECTDIR}/_ext/9990001/pic16f5x_gpio.p1 ${OBJECTDIR}/_ext/9990001/pic16f5x_timer0.p1 ${OBJECTDIR}/_ext/9990001/pic16_irq_stub.p1 ${OBJECTDIR}/_ext/9990001/pic16_irq_dispatch.p1 ${OBJECTDIR}/_ext/9990001/pic16f5x_wdt_sleep_target.p1 ${OBJECTDIR}/_ext/9990001/epic_harness_target.p1

# Source Files
SOURCEFILES=main.c ../../pic16f5x-hal/src/peripherals/pic16f5x_gpio.c ../../pic16f5x-hal/src/peripherals/pic16f5x_timer0.c ../../pic16f5x-hal/src/core/pic16_irq_stub.c ../../pic16f5x-hal/src/core/pic16_irq_dispatch.c ../../pic16f5x-hal/src/target/pic16f5x_wdt_sleep_target.c ../../epic-common/src/core/epic_harness_target.c


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
	${MAKE}  -f nbproject/Makefile-default.mk ${DISTDIR}/epic-hal-demo-pic16f5x.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}

MP_PROCESSOR_OPTION=16F54
# ------------------------------------------------------------------------------------
# Rules for buildStep: compile
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
# DEBUG rules placeholder (the release gate builds the production configuration).
else
${OBJECTDIR}/main.p1: main.c  nbproject/Makefile-${CND_CONF}.mk
	@${RM} ${OBJECTDIR}/main.p1.d
	@${RM} ${OBJECTDIR}/main.p1
	@${MKDIR} "${OBJECTDIR}"
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F54 -DFOSC_HZ=4000000 -xassembler-with-cpp -I"../../pic16f5x-hal/include/target" -I"../../pic16f5x-hal/include" -I"../../epic-common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup  -o ${OBJECTDIR}/main.p1 main.c
	@-${MV} ${OBJECTDIR}/main.d ${OBJECTDIR}/main.p1.d
	@${FIXDEPS} ${OBJECTDIR}/main.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/9990001/pic16f5x_gpio.p1: ../../pic16f5x-hal/src/peripherals/pic16f5x_gpio.c  nbproject/Makefile-${CND_CONF}.mk
	@${RM} ${OBJECTDIR}/_ext/9990001/pic16f5x_gpio.p1.d
	@${RM} ${OBJECTDIR}/_ext/9990001/pic16f5x_gpio.p1
	@${MKDIR} "${OBJECTDIR}/_ext/9990001"
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F54 -DFOSC_HZ=4000000 -xassembler-with-cpp -I"../../pic16f5x-hal/include/target" -I"../../pic16f5x-hal/include" -I"../../epic-common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup  -o ${OBJECTDIR}/_ext/9990001/pic16f5x_gpio.p1 ../../pic16f5x-hal/src/peripherals/pic16f5x_gpio.c
	@-${MV} ${OBJECTDIR}/_ext/9990001/pic16f5x_gpio.d ${OBJECTDIR}/_ext/9990001/pic16f5x_gpio.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/9990001/pic16f5x_gpio.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/9990001/pic16f5x_timer0.p1: ../../pic16f5x-hal/src/peripherals/pic16f5x_timer0.c  nbproject/Makefile-${CND_CONF}.mk
	@${RM} ${OBJECTDIR}/_ext/9990001/pic16f5x_timer0.p1.d
	@${RM} ${OBJECTDIR}/_ext/9990001/pic16f5x_timer0.p1
	@${MKDIR} "${OBJECTDIR}/_ext/9990001"
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F54 -DFOSC_HZ=4000000 -xassembler-with-cpp -I"../../pic16f5x-hal/include/target" -I"../../pic16f5x-hal/include" -I"../../epic-common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup  -o ${OBJECTDIR}/_ext/9990001/pic16f5x_timer0.p1 ../../pic16f5x-hal/src/peripherals/pic16f5x_timer0.c
	@-${MV} ${OBJECTDIR}/_ext/9990001/pic16f5x_timer0.d ${OBJECTDIR}/_ext/9990001/pic16f5x_timer0.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/9990001/pic16f5x_timer0.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/9990001/pic16_irq_stub.p1: ../../pic16f5x-hal/src/core/pic16_irq_stub.c  nbproject/Makefile-${CND_CONF}.mk
	@${RM} ${OBJECTDIR}/_ext/9990001/pic16_irq_stub.p1.d
	@${RM} ${OBJECTDIR}/_ext/9990001/pic16_irq_stub.p1
	@${MKDIR} "${OBJECTDIR}/_ext/9990001"
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F54 -DFOSC_HZ=4000000 -xassembler-with-cpp -I"../../pic16f5x-hal/include/target" -I"../../pic16f5x-hal/include" -I"../../epic-common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup  -o ${OBJECTDIR}/_ext/9990001/pic16_irq_stub.p1 ../../pic16f5x-hal/src/core/pic16_irq_stub.c
	@-${MV} ${OBJECTDIR}/_ext/9990001/pic16_irq_stub.d ${OBJECTDIR}/_ext/9990001/pic16_irq_stub.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/9990001/pic16_irq_stub.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/9990001/pic16_irq_dispatch.p1: ../../pic16f5x-hal/src/core/pic16_irq_dispatch.c  nbproject/Makefile-${CND_CONF}.mk
	@${RM} ${OBJECTDIR}/_ext/9990001/pic16_irq_dispatch.p1.d
	@${RM} ${OBJECTDIR}/_ext/9990001/pic16_irq_dispatch.p1
	@${MKDIR} "${OBJECTDIR}/_ext/9990001"
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F54 -DFOSC_HZ=4000000 -xassembler-with-cpp -I"../../pic16f5x-hal/include/target" -I"../../pic16f5x-hal/include" -I"../../epic-common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup  -o ${OBJECTDIR}/_ext/9990001/pic16_irq_dispatch.p1 ../../pic16f5x-hal/src/core/pic16_irq_dispatch.c
	@-${MV} ${OBJECTDIR}/_ext/9990001/pic16_irq_dispatch.d ${OBJECTDIR}/_ext/9990001/pic16_irq_dispatch.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/9990001/pic16_irq_dispatch.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/9990001/pic16f5x_wdt_sleep_target.p1: ../../pic16f5x-hal/src/target/pic16f5x_wdt_sleep_target.c  nbproject/Makefile-${CND_CONF}.mk
	@${RM} ${OBJECTDIR}/_ext/9990001/pic16f5x_wdt_sleep_target.p1.d
	@${RM} ${OBJECTDIR}/_ext/9990001/pic16f5x_wdt_sleep_target.p1
	@${MKDIR} "${OBJECTDIR}/_ext/9990001"
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F54 -DFOSC_HZ=4000000 -xassembler-with-cpp -I"../../pic16f5x-hal/include/target" -I"../../pic16f5x-hal/include" -I"../../epic-common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup  -o ${OBJECTDIR}/_ext/9990001/pic16f5x_wdt_sleep_target.p1 ../../pic16f5x-hal/src/target/pic16f5x_wdt_sleep_target.c
	@-${MV} ${OBJECTDIR}/_ext/9990001/pic16f5x_wdt_sleep_target.d ${OBJECTDIR}/_ext/9990001/pic16f5x_wdt_sleep_target.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/9990001/pic16f5x_wdt_sleep_target.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

${OBJECTDIR}/_ext/9990001/epic_harness_target.p1: ../../epic-common/src/core/epic_harness_target.c  nbproject/Makefile-${CND_CONF}.mk
	@${RM} ${OBJECTDIR}/_ext/9990001/epic_harness_target.p1.d
	@${RM} ${OBJECTDIR}/_ext/9990001/epic_harness_target.p1
	@${MKDIR} "${OBJECTDIR}/_ext/9990001"
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F54 -DFOSC_HZ=4000000 -xassembler-with-cpp -I"../../pic16f5x-hal/include/target" -I"../../pic16f5x-hal/include" -I"../../epic-common/include" -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup  -o ${OBJECTDIR}/_ext/9990001/epic_harness_target.p1 ../../epic-common/src/core/epic_harness_target.c
	@-${MV} ${OBJECTDIR}/_ext/9990001/epic_harness_target.d ${OBJECTDIR}/_ext/9990001/epic_harness_target.p1.d
	@${FIXDEPS} ${OBJECTDIR}/_ext/9990001/epic_harness_target.p1.d $(SILENT) -rsi ${MP_CC_DIR}../

endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: link
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
${DISTDIR}/epic-hal-demo-pic16f5x.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${DISTDIR}
	${MP_CC} $(MP_EXTRA_LD_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -Wl,-Map=${DISTDIR}/epic-hal-demo-pic16f5x.X.${IMAGE_TYPE}.map  -DXPRJ_default=$(CND_CONF)  -Wl,--defsym=__MPLAB_BUILD=1   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F54 -DFOSC_HZ=4000000 -xassembler-with-cpp -I"../../pic16f5x-hal/include/target" -I"../../pic16f5x-hal/include" -I"../../epic-common/include" -mwarn=-3 -Wa,-a -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup  -D__DEBUG=1  -mdebugger=none   ${OBJECTFILES}  -o ${DISTDIR}/epic-hal-demo-pic16f5x.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
	@${RM} ${DISTDIR}/epic-hal-demo-pic16f5x.X.${IMAGE_TYPE}.hex


else
${DISTDIR}/epic-hal-demo-pic16f5x.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${DISTDIR}
	${MP_CC} $(MP_EXTRA_LD_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -Wl,-Map=${DISTDIR}/epic-hal-demo-pic16f5x.X.${IMAGE_TYPE}.map  -DXPRJ_default=$(CND_CONF)  -Wl,--defsym=__MPLAB_BUILD=1   -mdfp="${DFP_DIR}/xc8"  -O0 -fasmfile -maddrqual=ignore -DPIC16F54 -DFOSC_HZ=4000000 -xassembler-with-cpp -I"../../pic16f5x-hal/include/target" -I"../../pic16f5x-hal/include" -I"../../epic-common/include" -mwarn=-3 -Wa,-a -msummary=-psect,-class,+mem,-hex,-file  -ginhx32 -Wl,--data-init -mno-keep-startup  ${OBJECTFILES}  -o ${DISTDIR}/epic-hal-demo-pic16f5x.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}


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
