/*
 * Shared application core for the PIC18F4550 control demo: a UART
 * console driven PID loop (ADC AN0 sense, CCP1 PWM drive) with
 * EEPROM-backed settings, run by epic-taskmgr. One core, two entry
 * points (examples/example_control_demo.c real hardware,
 * tests/sim_control_demo.c the mdb gate); only the stimulus source
 * (live UART RX plus live ADC vs scripted sim injections, via
 * control_demo_inject_console_byte and control_demo_inject_adc)
 * differs.
 */

#ifndef CONTROL_DEMO_CORE_H
#define CONTROL_DEMO_CORE_H

#include <stdint.h>

/**
 * @brief Initialize the serial console, ADC, EEPROM-backed settings, PID, and PWM.
 *
 * Call once before spawning any epic-taskmgr task below.
 */
void control_demo_init(void);

/**
 * @brief Feed one console byte into the line parser.
 *
 * The sim stimulus seam: the console task feeds live UART bytes through
 * the same parser, so both sources share one command path.
 *
 * @param b the received byte to parse
 */
void control_demo_inject_console_byte(uint8_t b);

/**
 * @brief Override the ADC plant with a scripted raw reading.
 *
 * The sim stimulus seam: once set, the control task samples this value
 * instead of starting real conversions (MPLAB SIM has no analog
 * stimulus, and each conversion costs a console warning there).
 *
 * @param v the scripted raw ADC value, 0..1023
 */
void control_demo_inject_adc(uint16_t v);

/**
 * @brief taskmgr task: sample AN0, filter, step the PID, drive PWM.
 * @param arg unused (epic_taskmgr_fn_t signature)
 */
void control_demo_task_control(void *arg);

/**
 * @brief taskmgr task: drain the UART RX ring into the line parser.
 * @param arg unused (epic_taskmgr_fn_t signature)
 */
void control_demo_task_console(void *arg);

/**
 * @brief taskmgr task: autosave the settings struct when dirty.
 * @param arg unused (epic_taskmgr_fn_t signature)
 */
void control_demo_task_eeprom(void *arg);

/**
 * @brief taskmgr task: UART heartbeat line with loop state.
 * @param arg unused (epic_taskmgr_fn_t signature)
 */
void control_demo_task_heartbeat(void *arg);

/**
 * @brief Filtered measurement in the 12-bit control domain, for the sim oracle.
 * @return the latest averaged measurement, 0..4092
 */
uint16_t control_demo_measurement(void);

/**
 * @brief Latest PID output, for the sim oracle.
 * @return the last computed output, 0..1000
 */
int16_t control_demo_output(void);

/**
 * @brief Current setpoint, for the sim oracle.
 * @return the setpoint in the 12-bit control domain
 */
int16_t control_demo_setpoint(void);

/**
 * @brief Current proportional gain, for the sim oracle.
 * @return kp in Q8.8
 */
int16_t control_demo_gain_kp(void);

/**
 * @brief Current integral gain, for the sim oracle.
 * @return ki in Q8.8, pre-multiplied by the control period
 */
int16_t control_demo_gain_ki(void);

/**
 * @brief Current derivative gain, for the sim oracle.
 * @return kd in Q8.8, pre-divided by the control period
 */
int16_t control_demo_gain_kd(void);

/**
 * @brief Completed EEPROM write count, for the sim oracle.
 * @return the number of completed EEPROM byte writes
 */
uint16_t control_demo_eeprom_writes(void);

#endif /* CONTROL_DEMO_CORE_H */
