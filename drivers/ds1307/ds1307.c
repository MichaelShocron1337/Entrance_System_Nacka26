#include "ds1307.h"
#include "twi.h"
#include <avr/io.h>

#define F_CPU 16000000UL
#define SCL_CLOCK 100000UL

static uint8_t dec_to_bcd(uint8_t val) {
    return ((val / 10) << 4) | (val % 10);
}

static uint8_t bcd_to_dec(uint8_t val) {
    return ((val >> 4) * 10) + (val & 0x0F);
}

static void ds1307_write_register(uint8_t reg, uint8_t value) {
    twi_start();
    twi_write((DS1307_ADDR << 1) | 0); // Write
    twi_write(reg);
    twi_write(value);
    twi_stop();
}

static uint8_t ds1307_read_register(uint8_t reg) {
    uint8_t value;

    twi_start();
    twi_write((DS1307_ADDR << 1) | 0); // Write
    twi_write(reg);

    twi_start();
    twi_write((DS1307_ADDR << 1) | 1); // Read
    value = twi_read_nack();

    twi_stop();

    return value;
}

void ds1307_start_clock(void) {
    uint8_t sec = ds1307_read_register(0x00);
    sec &= ~(1 << 7);   // CH = 0
    ds1307_write_register(0x00, sec);
}

void ds1307_stop_clock(void) {
    uint8_t sec = ds1307_read_register(0x00);
    sec |= (1 << 7);    // CH = 1
    ds1307_write_register(0x00, sec);
}

void ds1307_set_datetime(DateTime *dt) {
    twi_start();
    twi_write((DS1307_ADDR << 1) | 0);

    twi_write(0x00);

    twi_write(dec_to_bcd(dt->second) & 0x7F);
    twi_write(dec_to_bcd(dt->minute));
    twi_write(dec_to_bcd(dt->hour));       // 24h-format
    twi_write(dec_to_bcd(dt->dayOfWeek));
    twi_write(dec_to_bcd(dt->day));
    twi_write(dec_to_bcd(dt->month));
    twi_write(dec_to_bcd(dt->year));

    twi_stop();
}

void ds1307_get_datetime(DateTime *dt) {
    twi_start();
    twi_write((DS1307_ADDR << 1) | 0);
    twi_write(0x00);

    twi_start();
    twi_write((DS1307_ADDR << 1) | 1);

    dt->second    = bcd_to_dec(twi_read_ack() & 0x7F);
    dt->minute    = bcd_to_dec(twi_read_ack());
    dt->hour      = bcd_to_dec(twi_read_ack() & 0x3F);
    dt->dayOfWeek = bcd_to_dec(twi_read_ack());
    dt->day       = bcd_to_dec(twi_read_ack());
    dt->month     = bcd_to_dec(twi_read_ack());
    dt->year      = bcd_to_dec(twi_read_nack());

    twi_stop();
}