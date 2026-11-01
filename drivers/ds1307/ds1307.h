#ifndef DS1307_H
#define DS1307_H

#include <stdint.h>

#define DS1307_ADDR 0x68

typedef struct {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t dayOfWeek;
    uint8_t day;
    uint8_t month;
    uint8_t year;
} DateTime;

void ds1307_set_datetime(DateTime *dt);
void ds1307_get_datetime(DateTime *dt);
void ds1307_start_clock(void);
void ds1307_stop_clock(void);

#endif