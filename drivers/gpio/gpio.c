#include "gpio.h"

void gpio_pin_output(volatile uint8_t *ddr, uint8_t pin)
{
    *ddr |= (1U << pin);
}

void gpio_pin_input(volatile uint8_t *ddr, uint8_t pin)
{
    *ddr &= ~(1U << pin);
}

void gpio_pin_high(volatile uint8_t *port, uint8_t pin)
{
    *port |= (1U << pin);
}

void gpio_pin_low(volatile uint8_t *port, uint8_t pin)
{
    *port &= ~(1U << pin);
}

void gpio_pin_toggle(volatile uint8_t *port, uint8_t pin)
{
    *port ^= (1U << pin);
}

void gpio_pin_input_pullup(volatile uint8_t *ddr, volatile uint8_t *port, uint8_t pin)
{
    gpio_pin_input(ddr, pin);
    gpio_pin_high(port, pin);
}