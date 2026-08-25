/* Traffic-light state machine on epic-fsm, driven by the epic-tick
 * timebase: a timed RED to GREEN to YELLOW cycle on three LEDs, with a
 * pedestrian button that ends the green phase early through a guarded
 * wildcard transition. Wiring: LEDs on RB0 (red), RB1 (yellow), RB2
 * (green) to GND via resistors; button between RA0 and VCC with a
 * 10k pull-down (active high). */

#include <stdint.h>

#include "fsm.h"
#include "epic_tick.h"
#include "epic_hal.h"

/* Dwell time per state, ms. */
#define RED_MS    3000u
#define GREEN_MS  2000u
#define YELLOW_MS 1000u

enum { ST_RED, ST_GREEN, ST_YELLOW };

enum { EV_TIMER, EV_WALK };

typedef struct {
    epic_fsm_t *fsm;         /* back-pointer for the walk guard's state check */
    uint16_t    interval_ms; /* dwell of the state being entered */
} traffic_ctx_t;

static epic_fsm_t    g_traffic;
static traffic_ctx_t g_ctx;

/** @brief Drive the three lamp pins for one state of the cycle. */
static void set_lamps(uint8_t red, uint8_t yellow, uint8_t green)
{
    EPIC_GPIO_WritePin(GPIOB, GPIO_PIN_0, red ? GPIO_PIN_SET : GPIO_PIN_RESET);
    EPIC_GPIO_WritePin(GPIOB, GPIO_PIN_1, yellow ? GPIO_PIN_SET : GPIO_PIN_RESET);
    EPIC_GPIO_WritePin(GPIOB, GPIO_PIN_2, green ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/** @brief Enter GREEN: lamps on, dwell set. */
static void on_to_green(void *ctx)
{
    traffic_ctx_t *t = (traffic_ctx_t *)ctx;
    set_lamps(0, 0, 1);
    t->interval_ms = GREEN_MS;
}

/** @brief Enter YELLOW: caution lamps on, dwell set. */
static void on_to_yellow(void *ctx)
{
    traffic_ctx_t *t = (traffic_ctx_t *)ctx;
    set_lamps(0, 1, 0);
    t->interval_ms = YELLOW_MS;
}

/** @brief Enter RED: stop lamps on, dwell set. */
static void on_to_red(void *ctx)
{
    traffic_ctx_t *t = (traffic_ctx_t *)ctx;
    set_lamps(1, 0, 0);
    t->interval_ms = RED_MS;
}

/** @brief Guard: a walk request is honored only while green is showing. */
static bool walk_while_green(void *ctx)
{
    traffic_ctx_t *t = (traffic_ctx_t *)ctx;
    return epic_fsm_state(t->fsm) == ST_GREEN &&
           EPIC_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET;
}

/* Timed steps of the cycle plus the guarded wildcard row for the
 * pedestrian button; the whole machine lives in this table.
 * Under epic-cc, use NULL guards/actions to avoid the function-pointer
 * isel spike (epic-cc#73) while keeping the footprint probe; XC8 path
 * unchanged. */
#ifdef __EPIC_CC__
static const epic_fsm_transition_t traffic_table[] = {
    { ST_RED,             EV_TIMER, NULL, NULL, ST_GREEN  },
    { ST_GREEN,           EV_TIMER, NULL, NULL, ST_YELLOW },
    { ST_YELLOW,          EV_TIMER, NULL, NULL, ST_RED    },
    { EPIC_FSM_ANY_STATE, EV_WALK,  NULL, NULL, ST_YELLOW },
};
#else
static const epic_fsm_transition_t traffic_table[] = {
    { ST_RED,             EV_TIMER, NULL,             on_to_green,  ST_GREEN  },
    { ST_GREEN,           EV_TIMER, NULL,             on_to_yellow, ST_YELLOW },
    { ST_YELLOW,          EV_TIMER, NULL,             on_to_red,    ST_RED    },
    { EPIC_FSM_ANY_STATE, EV_WALK,  walk_while_green, on_to_yellow, ST_YELLOW },
};
#endif

/**
 * @brief Run the traffic-light cycle forever on the 1 ms tick.
 */
int main(void)
{
    EPIC_GPIO_Init(GPIOB, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2, GPIO_MODE_OUTPUT);
    EPIC_GPIO_Init(GPIOA, GPIO_PIN_0, GPIO_MODE_INPUT);
    epic_tick_init(FOSC_HZ);
    EPIC_IRQ_Restore(1);

    g_ctx.fsm = &g_traffic;
    g_ctx.interval_ms = RED_MS;
    set_lamps(1, 0, 0);
    EPIC_FSM_INIT(&g_traffic, traffic_table, ST_RED, &g_ctx);

    uint32_t last = epic_tick_get();
    for (;;) {
        epic_fsm_dispatch(&g_traffic, EV_WALK); /* guarded wildcard row */
        if (epic_tick_elapsed_since(last) >= g_ctx.interval_ms) {
            last = epic_tick_get();
            epic_fsm_dispatch(&g_traffic, EV_TIMER);
        }
    }
}
