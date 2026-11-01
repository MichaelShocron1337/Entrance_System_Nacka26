#include "twi.h"
#include <avr/io.h>

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#define TWI_READ   1
#define TWI_WRITE  0

void twi_init(uint32_t scl_frequency)
{
    // SCL frequency:
    // SCL = F_CPU / (16 + 2 * TWBR * prescaler)
    TWSR = 0x00; // Prescaler = 1

    TWBR = (uint8_t)(((F_CPU / scl_frequency) - 16) / 2);

    // Enable TWI
    TWCR = (1 << TWEN);
}

uint8_t twi_start(void)
{
    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);

    while (!(TWCR & (1 << TWINT)));

    uint8_t status = TWSR & 0xF8;

    if (status != 0x08 && status != 0x10)
    {
        return TWI_ERROR;
    }

    return TWI_OK;
}

void twi_stop(void)
{
    TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);

    while (TWCR & (1 << TWSTO));
}

uint8_t twi_write(uint8_t data)
{
    TWDR = data;

    TWCR = (1 << TWINT) | (1 << TWEN);

    while (!(TWCR & (1 << TWINT)));

    uint8_t status = TWSR & 0xF8;

    if (status == 0x18 ||  // SLA+W transmitted, ACK received
        status == 0x28 ||  // Data transmitted, ACK received
        status == 0x40)    // SLA+R transmitted, ACK received
    {
        return TWI_OK;
    }

    return TWI_ERROR;
}

uint8_t twi_read_ack(void)
{
    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWEA);

    while (!(TWCR & (1 << TWINT)));

    return TWDR;
}

uint8_t twi_read_nack(void)
{
    TWCR = (1 << TWINT) | (1 << TWEN);

    while (!(TWCR & (1 << TWINT)));

    return TWDR;
}

uint8_t twi_write_register(uint8_t addr, uint8_t reg, uint8_t data)
{
    if (twi_start() != TWI_OK)
        return TWI_ERROR;

    if (twi_write((addr << 1) | TWI_WRITE) != TWI_OK)
        return TWI_ERROR;

    if (twi_write(reg) != TWI_OK)
        return TWI_ERROR;

    if (twi_write(data) != TWI_OK)
        return TWI_ERROR;

    twi_stop();

    return TWI_OK;
}

uint8_t twi_read_register(uint8_t addr, uint8_t reg, uint8_t *data)
{
    if (twi_start() != TWI_OK)
        return TWI_ERROR;

    if (twi_write((addr << 1) | TWI_WRITE) != TWI_OK)
        return TWI_ERROR;

    if (twi_write(reg) != TWI_OK)
        return TWI_ERROR;

    // Repeated start
    if (twi_start() != TWI_OK)
        return TWI_ERROR;

    if (twi_write((addr << 1) | TWI_READ) != TWI_OK)
        return TWI_ERROR;

    *data = twi_read_nack();

    twi_stop();

    return TWI_OK;
}