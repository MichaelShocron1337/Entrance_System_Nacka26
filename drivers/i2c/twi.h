#ifndef TWI_H
#define TWI_H

#include <stdint.h>

#define TWI_OK      0
#define TWI_ERROR   1

void twi_init(uint32_t scl_frequency);

uint8_t twi_start(void);
void twi_stop(void);

uint8_t twi_write(uint8_t data);
uint8_t twi_read_ack(void);
uint8_t twi_read_nack(void);

uint8_t twi_write_register(uint8_t addr, uint8_t reg, uint8_t data);
uint8_t twi_read_register(uint8_t addr, uint8_t reg, uint8_t *data);

#endif