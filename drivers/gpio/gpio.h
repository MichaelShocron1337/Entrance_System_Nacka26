#ifndef GPIO_H
#define GPIO_H

#include <avr/io.h>
#include <stdint.h>

void gpio_pin_output(volatile uint8_t *ddr, uint8_t pin);
void gpio_pin_input(volatile uint8_t *ddr, uint8_t pin);
void gpio_pin_input_pullup(volatile uint8_t *ddr, volatile uint8_t *port, uint8_t pin);
void gpio_pin_high(volatile uint8_t *port, uint8_t pin);
void gpio_pin_low(volatile uint8_t *port, uint8_t pin);
void gpio_pin_toggle(volatile uint8_t *port, uint8_t pin);

#endif