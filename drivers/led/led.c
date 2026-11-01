#include "74hc595.h"

#define RGB_BLUE SHIFT_Q7
#define RGB_GREEN SHIFT_Q6
#define RGB_RED SHIFT_Q5

void rgb_off()
{
    shift_register_clear_bits(RGB_BLUE | RGB_GREEN| RGB_RED);
}

void init_led()
{
    rgb_off();
}

void green_led_on()
{
    rgb_off();
    shift_register_set_bits(RGB_GREEN);
}

void green_led_off()
{
    shift_register_set_bits(RGB_GREEN);
}

void red_led_on()
{
    rgb_off();
    shift_register_set_bits(RGB_RED);
}

void red_led_off()
{
    shift_register_clear_bits(RGB_RED);
}

void blue_led_on()
{
    rgb_off();
    shift_register_set_bits(RGB_BLUE);
}

void blue_led_off()
{
    shift_register_clear_bits(RGB_BLUE);
}

void cyan_led_on()
{   
    rgb_off();
    shift_register_set_bits(RGB_BLUE | RGB_GREEN);
}

void cyan_led_off()
{
    shift_register_clear_bits(RGB_BLUE | RGB_GREEN);
}

void yellow_led_on()
{   
    rgb_off();
    shift_register_set_bits(RGB_GREEN| RGB_RED);
}

void yellow_led_off()
{
    shift_register_clear_bits(RGB_GREEN| RGB_RED);
}

void magenta_led_on()
{
    rgb_off();
    shift_register_set_bits(RGB_BLUE | RGB_RED);
}

void magenta_led_off()
{
    shift_register_clear_bits(RGB_BLUE | RGB_RED);
}

void white_led_on()
{
    shift_register_set_bits(RGB_BLUE | RGB_GREEN| RGB_RED);
}

void white_led_off()
{
    shift_register_clear_bits(RGB_BLUE | RGB_GREEN| RGB_RED);
}