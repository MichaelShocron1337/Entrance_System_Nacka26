#include <avr/io.h>
#include <util/delay.h>
#include "spi.h"

#define LATCH_PIN PD2
#define LATCH_PORT PORTD
#define LATCH_DDR DDRD

static uint8_t shift_reg_state = 0b00000000;

static void latch()
{
    LATCH_PORT |= (1 << LATCH_PIN);
    _delay_us(5);
    LATCH_PORT &= ~(1 << LATCH_PIN);
}

void shift_register_set_bits(uint8_t bits)
{
    shift_reg_state |= bits;
    spi_transfer(shift_reg_state);
    latch();

}

void shift_register_clear_bits(uint8_t bits)
{
    shift_reg_state &= ~bits;
    spi_transfer(shift_reg_state);
    latch();

}

void shift_register_init()
{
    LATCH_DDR |= (1 << LATCH_PIN);
    LATCH_PORT &= ~(1 << LATCH_PIN);
    shift_register_clear_bits(0b11111111);

}