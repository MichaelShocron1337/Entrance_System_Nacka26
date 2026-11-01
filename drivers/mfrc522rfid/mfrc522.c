#include <avr/io.h>
#include <util/delay.h>
#include "mfrc522.h"
#include "spi.h"

/* MFRC522 connections on Arduino:
   SDA/SS -> D10 / PB2
   SCK    -> D13 / PB5
   MOSI   -> D11 / PB3
   MISO   -> D12 / PB4
   RST    -> D8  / PB0
*/
#define MFRC522_CS_DDR   DDRB
#define MFRC522_CS_PORT  PORTB
#define MFRC522_CS_PIN   PB2

#define MFRC522_RST_DDR  DDRB
#define MFRC522_RST_PORT PORTB
#define MFRC522_RST_PIN  PB0

static void mfrc522_cs_low(void)
{
    MFRC522_CS_PORT &= ~(1 << MFRC522_CS_PIN);
}

static void mfrc522_cs_high(void)
{
    MFRC522_CS_PORT |= (1 << MFRC522_CS_PIN);
}

static void mfrc522_rst_low(void)
{
    MFRC522_RST_PORT &= ~(1 << MFRC522_RST_PIN);
}

static void mfrc522_rst_high(void)
{
    MFRC522_RST_PORT |= (1 << MFRC522_RST_PIN);
}

static void mfrc522_fifo_flush(void)
{
    mfrc522_write_reg(FIFOLevelReg, 0x80);
}

void mfrc522_write_reg(uint8_t reg, uint8_t value)
{
    mfrc522_cs_low();
    spi_transfer((uint8_t)((reg << 1) & 0x7E));
    spi_transfer(value);
    mfrc522_cs_high();
}

uint8_t mfrc522_read_reg(uint8_t reg)
{
    uint8_t value;

    mfrc522_cs_low();
    spi_transfer((uint8_t)(((reg << 1) & 0x7E) | 0x80));
    value = spi_transfer(0x00);
    mfrc522_cs_high();

    return value;
}

void mfrc522_set_bitmask(uint8_t reg, uint8_t mask)
{
    uint8_t tmp = mfrc522_read_reg(reg);
    mfrc522_write_reg(reg, tmp | mask);
}

void mfrc522_clear_bitmask(uint8_t reg, uint8_t mask)
{
    uint8_t tmp = mfrc522_read_reg(reg);
    mfrc522_write_reg(reg, tmp & (uint8_t)(~mask));
}

void mfrc522_reset(void)
{
    mfrc522_write_reg(CommandReg, PCD_SOFTRESET);
    _delay_ms(50);
}

uint8_t mfrc522_get_version(void)
{
    return mfrc522_read_reg(VersionReg);
}

void mfrc522_antenna_on(void)
{
    uint8_t val = mfrc522_read_reg(TxControlReg);
    if ((val & 0x03) != 0x03)
    {
        mfrc522_set_bitmask(TxControlReg, 0x03);
    }
}

void mfrc522_antenna_off(void)
{
    mfrc522_clear_bitmask(TxControlReg, 0x03);
}

void mfrc522_init(void)
{
    MFRC522_CS_DDR |= (1 << MFRC522_CS_PIN);
    MFRC522_RST_DDR |= (1 << MFRC522_RST_PIN);

    mfrc522_cs_high();
    mfrc522_rst_low();
    _delay_ms(2);
    mfrc522_rst_high();
    _delay_ms(10);

    mfrc522_reset();

    mfrc522_write_reg(TModeReg, 0x8D);
    mfrc522_write_reg(TPrescalerReg, 0x3E);
    mfrc522_write_reg(TReloadRegL, 30);
    mfrc522_write_reg(TReloadRegH, 0);

    mfrc522_write_reg(TxASKReg, 0x40);
    mfrc522_write_reg(ModeReg, 0x3D);

    mfrc522_antenna_on();
}

static mfrc522_status_t mfrc522_calculate_crc(const uint8_t *data,
                                              uint8_t length,
                                              uint8_t *result)
{
    uint16_t i;

    mfrc522_write_reg(CommandReg, PCD_IDLE);
    mfrc522_write_reg(DivIrqReg, 0x04);
    mfrc522_fifo_flush();

    for (i = 0; i < length; i++)
    {
        mfrc522_write_reg(FIFODataReg, data[i]);
    }

    mfrc522_write_reg(CommandReg, PCD_CALC_CRC);

    for (i = 0; i < 5000; i++)
    {
        uint8_t n = mfrc522_read_reg(DivIrqReg);
        if (n & 0x04)
        {
            mfrc522_write_reg(CommandReg, PCD_IDLE);
            result[0] = mfrc522_read_reg(CRCResultRegL);
            result[1] = mfrc522_read_reg(CRCResultRegH);
            return MFRC522_OK;
        }
    }

    return MFRC522_TIMEOUT;
}

static mfrc522_status_t mfrc522_transceive(const uint8_t *send_data,
                                           uint8_t send_len,
                                           uint8_t *back_data,
                                           uint8_t *back_len,
                                           uint8_t *valid_bits)
{
    uint16_t i;
    uint8_t wait_irq = 0x30;
    uint8_t last_bits;
    uint8_t n;

    mfrc522_write_reg(CommandReg, PCD_IDLE);
    mfrc522_write_reg(ComIrqReg, 0x7F);
    mfrc522_fifo_flush();

    for (i = 0; i < send_len; i++)
    {
        mfrc522_write_reg(FIFODataReg, send_data[i]);
    }

    mfrc522_write_reg(CommandReg, PCD_TRANSCEIVE);
    mfrc522_set_bitmask(BitFramingReg, 0x80);

    for (i = 0; i < 5000; i++)
    {
        n = mfrc522_read_reg(ComIrqReg);
        if (n & wait_irq)
        {
            break;
        }
        if (n & 0x01)
        {
            return MFRC522_TIMEOUT;
        }
    }

    mfrc522_clear_bitmask(BitFramingReg, 0x80);

    if (i == 5000)
    {
        return MFRC522_TIMEOUT;
    }

    if (mfrc522_read_reg(ErrorReg) & 0x13)
    {
        return MFRC522_ERROR;
    }

    if (n & 0x08)
    {
        return MFRC522_COLLISION;
    }

    n = mfrc522_read_reg(FIFOLevelReg);
    last_bits = mfrc522_read_reg(ControlReg) & 0x07;

    if (valid_bits)
    {
        *valid_bits = last_bits;
    }

    if (back_data && back_len)
    {
        uint8_t bytes_to_read = n;
        if (bytes_to_read > *back_len)
        {
            bytes_to_read = *back_len;
        }

        for (i = 0; i < bytes_to_read; i++)
        {
            back_data[i] = mfrc522_read_reg(FIFODataReg);
        }
        *back_len = bytes_to_read;
    }

    return MFRC522_OK;
}

mfrc522_status_t mfrc522_request_a(uint8_t *atqa, uint8_t *atqa_len)
{
    uint8_t valid_bits = 7;
    uint8_t cmd = PICC_CMD_REQA;
    uint8_t back_len = 2;
    mfrc522_status_t status;

    if (!atqa || !atqa_len)
    {
        return MFRC522_ERROR;
    }

    mfrc522_write_reg(BitFramingReg, 0x07);

    status = mfrc522_transceive(&cmd, 1, atqa, &back_len, &valid_bits);

    if (status != MFRC522_OK)
    {
        return status;
    }

    if (back_len != 2)
    {
        return MFRC522_ERROR;
    }

    *atqa_len = back_len;
    return MFRC522_OK;
}

mfrc522_status_t mfrc522_anticoll_select(mfrc522_uid_t *uid)
{
    uint8_t buffer[9];
    uint8_t back_len;
    uint8_t valid_bits;
    uint8_t bcc;
    mfrc522_status_t status;

    if (!uid)
    {
        return MFRC522_ERROR;
    }

    buffer[0] = PICC_CMD_SEL_CL1;
    buffer[1] = 0x20;

    mfrc522_write_reg(BitFramingReg, 0x00);

    back_len = 5;
    valid_bits = 0;
    status = mfrc522_transceive(buffer, 2, buffer, &back_len, &valid_bits);
    if (status != MFRC522_OK)
    {
        return status;
    }

    if (back_len != 5)
    {
        return MFRC522_ERROR;
    }

    bcc = buffer[0] ^ buffer[1] ^ buffer[2] ^ buffer[3];
    if (bcc != buffer[4])
    {
        return MFRC522_ERROR;
    }

    uid->uid[0] = buffer[0];
    uid->uid[1] = buffer[1];
    uid->uid[2] = buffer[2];
    uid->uid[3] = buffer[3];
    uid->size = 4;

    buffer[0] = PICC_CMD_SEL_CL1;
    buffer[1] = 0x70;
    buffer[2] = uid->uid[0];
    buffer[3] = uid->uid[1];
    buffer[4] = uid->uid[2];
    buffer[5] = uid->uid[3];
    buffer[6] = buffer[2] ^ buffer[3] ^ buffer[4] ^ buffer[5];

    if (mfrc522_calculate_crc(buffer, 7, &buffer[7]) != MFRC522_OK)
    {
        return MFRC522_ERROR;
    }

    back_len = 3;
    status = mfrc522_transceive(buffer, 9, buffer, &back_len, &valid_bits);
    if (status != MFRC522_OK)
    {
        return status;
    }

    if (back_len != 3)
    {
        return MFRC522_ERROR;
    }

    uid->sak = buffer[0];
    return MFRC522_OK;
}

mfrc522_status_t mfrc522_halt(void)
{
    uint8_t buffer[4];
    uint8_t back_len = 0;
    uint8_t valid_bits = 0;

    buffer[0] = PICC_CMD_HLTA;
    buffer[1] = 0x00;

    if (mfrc522_calculate_crc(buffer, 2, &buffer[2]) != MFRC522_OK)
    {
        return MFRC522_ERROR;
    }

    return mfrc522_transceive(buffer, 4, 0, &back_len, &valid_bits);
}
