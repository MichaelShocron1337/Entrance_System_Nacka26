#ifndef SHIFT_74hc595
#define SHIFT_74hc595

#include <avr/io.h>

#define SHIFT_Q0 (1 << 0)
#define SHIFT_Q1 (1 << 1)
#define SHIFT_Q2 (1 << 2)
#define SHIFT_Q3 (1 << 3)
#define SHIFT_Q4 (1 << 4)
#define SHIFT_Q5 (1 << 5)
#define SHIFT_Q6 (1 << 6)
#define SHIFT_Q7 (1 << 7)

void shift_register_init();
void shift_register_set_bits(uint8_t bits);
void shift_register_clear_bits(uint8_t bits);

#endif