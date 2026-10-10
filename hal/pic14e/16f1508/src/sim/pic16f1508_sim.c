/**
 * Host model of the PIC16F1508/1509 port and interrupt-on-change behaviour.
 * Models only what the GPIO/IOC contract observes: no timers, USART, EEPROM,
 * or ADC, and no time base (one pin sample per step call).
 */
#include <stddef.h>
#include <string.h>
#include "pic16f1508.h"
#include "pic16f1508_sim.h"

uint8_t pic16f1508_sim_sfr[0x1000];

static const uint16_t sim_tris[3] = {PIC_REG_TRISA, PIC_REG_TRISB, PIC_REG_TRISC};
static const uint16_t sim_lat[3]  = {PIC_REG_LATA,  PIC_REG_LATB,  PIC_REG_LATC};
static const uint16_t sim_port[3] = {PIC_REG_PORTA, PIC_REG_PORTB, PIC_REG_PORTC};
static const uint8_t  sim_mask[3] = {PIC16F1508_FAMILY_PORTA_MASK,
                                     PIC16F1508_FAMILY_PORTB_MASK,
                                     PIC16F1508_FAMILY_PORTC_MASK};

/* Externally driven level per port bit; undriven pins read 0. */
static uint8_t sim_input_value[3];
static uint8_t sim_last_portb;
static pic16f1508_sim_irq_cb_t sim_irq_cb;

/**
 * @brief Map a port letter to its table index.
 * @param port 'A', 'B', or 'C'.
 * @return 0..2, or -1 for anything else.
 */
static int port_index(char port)
{
    switch (port)
    {
    case 'A': return 0;
    case 'B': return 1;
    case 'C': return 2;
    default:  return -1;
    }
}

/**
 * @brief Recompute PORTx from the latch (output pins) and driven levels (input pins).
 */
static void sim_refresh_ports(void)
{
    int i;
    for (i = 0; i < 3; i++)
    {
        uint8_t tris = pic16f1508_sim_sfr[sim_tris[i]];
        uint8_t lat  = pic16f1508_sim_sfr[sim_lat[i]];
        uint8_t out  = (uint8_t)(lat & (uint8_t)~tris);
        uint8_t in   = (uint8_t)(sim_input_value[i] & tris);
        pic16f1508_sim_sfr[sim_port[i]] = (uint8_t)((out | in) & sim_mask[i]);
    }
}

/**
 * @brief Set IOCBF/IOCIF on RB4..RB7 edges and run the callback when pending.
 * @details The callback is level-gated on IOCIF, IOCIE, and GIE, matching
 * silicon: a pending flag is taken as soon as GIE allows it.
 */
static void sim_step_ioc(void)
{
    uint8_t portb   = pic16f1508_sim_sfr[PIC_REG_PORTB];
    uint8_t changed = (uint8_t)(portb ^ sim_last_portb);
    uint8_t rise    = (uint8_t)(changed & portb);
    uint8_t fall    = (uint8_t)(changed & (uint8_t)~portb);
    uint8_t hits    = (uint8_t)(((rise & pic16f1508_sim_sfr[PIC_REG_IOCBP]) |
                                 (fall & pic16f1508_sim_sfr[PIC_REG_IOCBN])) &
                                PIC16F1508_FAMILY_IOCB_MASK);

    sim_last_portb = portb;
    if (hits != 0U)
    {
        pic16f1508_sim_sfr[PIC_REG_IOCBF] |= hits;
        pic16f1508_sim_sfr[PIC_REG_INTCON] |= PIC_INTCON_IOCIF;
    }

    if (((pic16f1508_sim_sfr[PIC_REG_INTCON] & (PIC_INTCON_IOCIF | PIC_INTCON_IOCIE | PIC_INTCON_GIE)) ==
         (PIC_INTCON_IOCIF | PIC_INTCON_IOCIE | PIC_INTCON_GIE)) &&
        (sim_irq_cb != NULL))
    {
        sim_irq_cb();
    }
}

/**
 * @brief Reset the simulated SFRs to power-up state with all pins as inputs.
 */
void pic16f1508_sim_reset(void)
{
    memset(pic16f1508_sim_sfr, 0, sizeof pic16f1508_sim_sfr);
    /* Power-up direction: all inputs. OPTION_REG reset value 0xFF. */
    pic16f1508_sim_sfr[PIC_REG_TRISA]  = PIC16F1508_FAMILY_PORTA_MASK;
    pic16f1508_sim_sfr[PIC_REG_TRISB]  = 0xFFU;
    pic16f1508_sim_sfr[PIC_REG_TRISC]  = 0xFFU;
    pic16f1508_sim_sfr[PIC_REG_OPTION] = 0xFFU;
    memset(sim_input_value, 0, sizeof sim_input_value);
    sim_irq_cb = NULL;
    sim_refresh_ports();
    sim_last_portb = pic16f1508_sim_sfr[PIC_REG_PORTB];
}

/**
 * @brief Advance the simulation by one step, refreshing ports and IOC.
 * @param ticks Elapsed tick count, unused by this model.
 */
void pic16f1508_sim_step(uint32_t ticks)
{
    (void)ticks;
    sim_refresh_ports();
    sim_step_ioc();
}

/**
 * @brief Set the external level presented on an input pin.
 * @param port Port letter, 'A', 'B' or 'C'. Invalid ports are ignored.
 * @param pin Pin number 0..7. Values above 7 are ignored.
 * @param level Nonzero drives high, zero drives low.
 */
void pic16f1508_sim_drive_input(char port, uint8_t pin, uint8_t level)
{
    int i = port_index(port);
    uint8_t bit;

    if ((i < 0) || (pin > 7U))
    {
        return;
    }

    bit = (uint8_t)(1U << pin);
    if (level != 0U)
    {
        sim_input_value[i] |= bit;
    }
    else
    {
        sim_input_value[i] &= (uint8_t)~bit;
    }
}

/**
 * @brief Read the simulated output level of one pin.
 * @param port Port letter, 'A', 'B' or 'C'.
 * @param pin Pin number 0..7.
 * @return 1 when the pin drives high, 0 when low or invalid.
 */
uint8_t pic16f1508_sim_read_output(char port, uint8_t pin)
{
    int i = port_index(port);

    if ((i < 0) || (pin > 7U))
    {
        return 0U;
    }

    sim_refresh_ports();
    return (uint8_t)((pic16f1508_sim_sfr[sim_port[i]] >> pin) & 1U);
}

/**
 * @brief Register the callback run when IOCIF and IOCIE and GIE are all set.
 * @param cb Callback, or NULL to disable delivery.
 */
void pic16f1508_sim_set_irq_callback(pic16f1508_sim_irq_cb_t cb)
{
    sim_irq_cb = cb;
}
