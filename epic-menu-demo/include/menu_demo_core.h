/*
 * Shared application core for the PIC18F4550 menu demo: a 3-screen
 * LCD control panel driven by epic-taskmgr's cooperative scheduler.
 * One core, two entry points (examples/example_menu_demo.c for real
 * hardware, tests/sim_menu_demo.c for the mdb sim gate) so both
 * toolchains exercise byte-identical application logic; only the
 * button source differs (real RB-change IRQ vs a scripted sim
 * stimulus), via menu_demo_push_event.
 */

#ifndef MENU_DEMO_CORE_H
#define MENU_DEMO_CORE_H

#include <stdint.h>

/** Screens, cycled by SELECT. UP/DOWN only act on MENU_SCREEN_BRIGHTNESS. */
typedef enum {
    MENU_SCREEN_STATUS = 0,   /**< Uptime + live ADC reading. */
    MENU_SCREEN_BRIGHTNESS,   /**< Adjustable 0..10, EEPROM-persisted, drives PWM. */
    MENU_SCREEN_ABOUT,        /**< Tick count + EEPROM write count. */
    MENU_SCREEN_COUNT
} menu_screen_t;

/** Button events, queued from either the real RB-change ISR or a
 *  scripted sim stimulus task; drained by menu_demo_task_ui. */
typedef enum {
    MENU_EVENT_UP = 0,
    MENU_EVENT_DOWN,
    MENU_EVENT_SELECT,
} menu_event_t;

/**
 * @brief Initialize the LCD, ADC, EEPROM-backed settings, and PWM.
 *
 * Call once before spawning any epic-taskmgr task below.
 */
void menu_demo_init(void);

/**
 * @brief Queue a button event (small ring, drops silently if full).
 *
 * Safe to call from ISR context (the real RB-change handler) or from a
 * polled sim-stimulus task.
 *
 * @param ev the event to queue
 */
void menu_demo_push_event(menu_event_t ev);

/** @brief taskmgr task: sample the ADC, redraw if on the status screen. */
void menu_demo_task_adc(void *arg);

/** @brief taskmgr task: drain queued button events, update state, redraw. */
void menu_demo_task_ui(void *arg);

/** @brief taskmgr task: autosave the brightness setting when dirty. */
void menu_demo_task_eeprom(void *arg);

/** @brief taskmgr task: UART heartbeat line + PWM duty update. */
void menu_demo_task_heartbeat(void *arg);

/* Introspection, for the sim oracle's cross-checks. */
uint16_t      menu_demo_adc_value(void);
uint8_t       menu_demo_brightness(void);
menu_screen_t menu_demo_screen(void);
uint16_t      menu_demo_eeprom_writes(void);

#endif /* MENU_DEMO_CORE_H */
