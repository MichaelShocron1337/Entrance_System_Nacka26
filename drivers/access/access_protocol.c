#include "access_protocol.h"
#include "uart.h"
#include "millis.h"

#include <avr/pgmspace.h>
#include <string.h>
#include <stdlib.h>

static const char CMD_UID[]      PROGMEM = "UID";
static const char CMD_PIN[]      PROGMEM = "PIN";
static const char CMD_PING[]     PROGMEM = "PING";
static const char CMD_PONG[]     PROGMEM = "PONG";
static const char CMD_RESET[]    PROGMEM = "RESET";
static const char CMD_NACK[]     PROGMEM = "NACK";

static const char NACK_BAD_CHECKSUM[]    PROGMEM = "BAD_CHECKSUM";
static const char NACK_BAD_LENGTH[]      PROGMEM = "BAD_LENGTH";
static const char NACK_UNKNOWN_COMMAND[] PROGMEM = "UNKNOWN_COMMAND";
static const char NACK_INVALID_STATE[]   PROGMEM = "INVALID_STATE";
static const char NACK_TIMEOUT[]         PROGMEM = "TIMEOUT";
static const char NACK_BAD_FORMAT[]      PROGMEM = "BAD_FORMAT";

typedef enum {
    RX_WAIT_STX = 0,
    RX_WAIT_LEN,
    RX_READ_PAYLOAD,
    RX_WAIT_CHK,
    RX_WAIT_ETX
} rx_state_t;

static rx_state_t rx_state;
static uint8_t rx_len;
static uint8_t rx_index;
static uint8_t rx_chk;
static millis_t rx_last_byte_time;
static char rx_payload[ACCESS_MAX_PAYLOAD + 1];

static uint8_t strlen_pgm_u8(const char *p)
{
    uint8_t len = 0;

    while (pgm_read_byte(p++) != '\0') {
        len++;
    }

    return len;
}

static void uart_write_pgm(const char *p)
{
    char c;

    while ((c = pgm_read_byte(p++)) != '\0') {
        uart_write_char(c);
    }
}

static uint8_t checksum_ram(const char *s)
{
    uint8_t chk = 0;

    while (*s) {
        chk ^= (uint8_t)*s++;
    }

    return chk;
}

static uint8_t checksum_pgm(const char *p)
{
    uint8_t chk = 0;
    char c;

    while ((c = pgm_read_byte(p++)) != '\0') {
        chk ^= (uint8_t)c;
    }

    return chk;
}

static uint8_t uint16_decimal_len(uint16_t value)
{
    if (value >= 10000) return 5;
    if (value >= 1000)  return 4;
    if (value >= 100)   return 3;
    if (value >= 10)    return 2;
    return 1;
}

static void uart_write_uint16_decimal(uint16_t value)
{
    char buf[6];
    uint8_t i = 0;

    if (value == 0) {
        uart_write_char('0');
        return;
    }

    while (value > 0) {
        buf[i++] = (char)('0' + (value % 10));
        value /= 10;
    }

    while (i > 0) {
        uart_write_char(buf[--i]);
    }
}

static uint8_t checksum_uint16_decimal(uint16_t value)
{
    char buf[6];
    uint8_t i = 0;
    uint8_t chk = 0;

    if (value == 0) {
        return (uint8_t)'0';
    }

    while (value > 0) {
        buf[i++] = (char)('0' + (value % 10));
        value /= 10;
    }

    while (i > 0) {
        chk ^= (uint8_t)buf[--i];
    }

    return chk;
}

static void rx_reset(void)
{
    rx_state = RX_WAIT_STX;
    rx_len = 0;
    rx_index = 0;
    rx_chk = 0;
    rx_last_byte_time = 0;
}

void access_init(void)
{
    rx_reset();
}

static bool is_valid_payload_char(char c)
{
    return c >= 32 && c <= 126;
}

static const char *nack_reason_pgm(access_status_t reason)
{
    switch (reason) {
        case ACCESS_STATUS_BAD_CHECKSUM:
            return NACK_BAD_CHECKSUM;

        case ACCESS_STATUS_BAD_LENGTH:
            return NACK_BAD_LENGTH;

        case ACCESS_STATUS_TIMEOUT:
            return NACK_TIMEOUT;

        case ACCESS_STATUS_UNKNOWN_COMMAND:
            return NACK_UNKNOWN_COMMAND;

        case ACCESS_STATUS_BAD_FORMAT:
        case ACCESS_STATUS_INVALID_ASCII:
        case ACCESS_STATUS_BAD_ETX:
        case ACCESS_STATUS_BAD_STX:
        default:
            return NACK_BAD_FORMAT;
    }
}

static access_status_t send_payload_pgm_data(uint16_t sid, const char *cmd_pgm, const char *data_pgm)
{
    uint8_t len = 0;
    uint8_t chk = 0;

    len += 3;
    len += uint16_decimal_len(sid);
    len += 1;
    len += strlen_pgm_u8(cmd_pgm);
    len += 1;
    len += strlen_pgm_u8(data_pgm);

    if (len > ACCESS_MAX_PAYLOAD) {
        return ACCESS_STATUS_BUFFER_TOO_SMALL;
    }

    chk ^= 'V';
    chk ^= '1';
    chk ^= '|';
    chk ^= checksum_uint16_decimal(sid);
    chk ^= '|';
    chk ^= checksum_pgm(cmd_pgm);
    chk ^= '|';
    chk ^= checksum_pgm(data_pgm);

    uart_write_char((char)ACCESS_STX);
    uart_write_char((char)len);

    uart_write_char('V');
    uart_write_char('1');
    uart_write_char('|');

    uart_write_uint16_decimal(sid);
    uart_write_char('|');

    uart_write_pgm(cmd_pgm);
    uart_write_char('|');

    uart_write_pgm(data_pgm);

    uart_write_char((char)chk);
    uart_write_char((char)ACCESS_ETX);

    return ACCESS_STATUS_OK;
}

static access_status_t send_payload(uint16_t sid, const char *cmd_pgm, const char *data)
{
    uint8_t len = 0;
    uint8_t chk = 0;

    if (data == NULL) {
        data = "";
    }

    len += 3;
    len += uint16_decimal_len(sid);
    len += 1;
    len += strlen_pgm_u8(cmd_pgm);
    len += 1;
    len += (uint8_t)strlen(data);

    if (len > ACCESS_MAX_PAYLOAD) {
        return ACCESS_STATUS_BUFFER_TOO_SMALL;
    }

    chk ^= 'V';
    chk ^= '1';
    chk ^= '|';
    chk ^= checksum_uint16_decimal(sid);
    chk ^= '|';
    chk ^= checksum_pgm(cmd_pgm);
    chk ^= '|';
    chk ^= checksum_ram(data);

    uart_write_char((char)ACCESS_STX);
    uart_write_char((char)len);

    uart_write_char('V');
    uart_write_char('1');
    uart_write_char('|');

    uart_write_uint16_decimal(sid);
    uart_write_char('|');

    uart_write_pgm(cmd_pgm);
    uart_write_char('|');

    while (*data) {
        uart_write_char(*data++);
    }

    uart_write_char((char)chk);
    uart_write_char((char)ACCESS_ETX);

    return ACCESS_STATUS_OK;
}

access_status_t access_send_uid(const char *uid)
{
    return send_payload(0, CMD_UID, uid);
}

access_status_t access_send_pin(uint16_t sid, const char *pin)
{
    return send_payload(sid, CMD_PIN, pin);
}

access_status_t access_send_ping(void)
{
    return send_payload(0, CMD_PING, "");
}

access_status_t access_send_pong(void)
{
    return send_payload(0, CMD_PONG, "");
}

access_status_t access_send_reset(uint16_t sid)
{
    return send_payload(sid, CMD_RESET, "");
}

access_status_t access_send_nack(uint16_t sid, access_status_t reason)
{
    return send_payload_pgm_data(sid, CMD_NACK, nack_reason_pgm(reason));
}

static bool str_eq(const char *a, const char *b)
{
    while (*a && *b) {
        if (*a != *b) {
            return false;
        }

        a++;
        b++;
    }

    return *a == '\0' && *b == '\0';
}

static access_command_t parse_command(const char *cmd)
{
    if (str_eq(cmd, "UID"))     return ACCESS_CMD_UID;
    if (str_eq(cmd, "REQ_PIN")) return ACCESS_CMD_REQ_PIN;
    if (str_eq(cmd, "PIN"))     return ACCESS_CMD_PIN;
    if (str_eq(cmd, "OK"))      return ACCESS_CMD_OK;
    if (str_eq(cmd, "ERR"))     return ACCESS_CMD_ERR;
    if (str_eq(cmd, "LOCKED"))  return ACCESS_CMD_LOCKED;
    if (str_eq(cmd, "TIMEOUT")) return ACCESS_CMD_TIMEOUT;
    if (str_eq(cmd, "RESET"))   return ACCESS_CMD_RESET;
    if (str_eq(cmd, "PING"))    return ACCESS_CMD_PING;
    if (str_eq(cmd, "PONG"))    return ACCESS_CMD_PONG;
    if (str_eq(cmd, "NACK"))    return ACCESS_CMD_NACK;

    return ACCESS_CMD_UNKNOWN;
}

static bool parse_sid(const char *s, uint16_t *sid)
{
    uint32_t value = 0;

    if (*s == '\0') {
        return false;
    }

    while (*s) {
        if (*s < '0' || *s > '9') {
            return false;
        }

        value = value * 10 + (uint8_t)(*s - '0');

        if (value > 65535UL) {
            return false;
        }

        s++;
    }

    *sid = (uint16_t)value;
    return true;
}

static access_status_t parse_payload(char *payload, access_message_t *msg)
{
    char *version;
    char *sid_str;
    char *cmd_str;
    char *data_str;
    access_command_t cmd;

    version = payload;

    sid_str = strchr(version, '|');
    if (!sid_str) return ACCESS_STATUS_BAD_FORMAT;
    *sid_str++ = '\0';

    cmd_str = strchr(sid_str, '|');
    if (!cmd_str) return ACCESS_STATUS_BAD_FORMAT;
    *cmd_str++ = '\0';

    data_str = strchr(cmd_str, '|');
    if (!data_str) return ACCESS_STATUS_BAD_FORMAT;
    *data_str++ = '\0';

    if (!str_eq(version, "V1")) {
        return ACCESS_STATUS_BAD_FORMAT;
    }

    if (!parse_sid(sid_str, &msg->sid)) {
        return ACCESS_STATUS_BAD_FORMAT;
    }

    cmd = parse_command(cmd_str);

    if (cmd == ACCESS_CMD_UNKNOWN) {
        msg->command = ACCESS_CMD_UNKNOWN;
        msg->data[0] = '\0';
        return ACCESS_STATUS_UNKNOWN_COMMAND;
    }

    msg->command = cmd;

    strncpy(msg->data, data_str, ACCESS_MAX_DATA - 1);
    msg->data[ACCESS_MAX_DATA - 1] = '\0';

    return ACCESS_STATUS_OK;
}

access_status_t access_read_message(access_message_t *msg)
{
    char c;
    millis_t now = millis_get();

    if (rx_state != RX_WAIT_STX &&
        rx_last_byte_time != 0 &&
        now - rx_last_byte_time > ACCESS_TIMEOUT_MS) {
        rx_reset();
        access_send_nack(0, ACCESS_STATUS_TIMEOUT);
        return ACCESS_STATUS_TIMEOUT;
    }

    while (uart_available() > 0) {
        if (!uart_read_char(&c)) {
            return ACCESS_STATUS_NO_DATA;
        }

        rx_last_byte_time = millis();

        /*
            Resync:
            Om ny STX kommer mitt i en frame börjar vi om.
        */
        if ((uint8_t)c == ACCESS_STX) {
            rx_state = RX_WAIT_LEN;
            rx_len = 0;
            rx_index = 0;
            rx_chk = 0;
            continue;
        }

        switch (rx_state) {
            case RX_WAIT_STX:
                break;

            case RX_WAIT_LEN:
                rx_len = (uint8_t)c;

                if (rx_len == 0 || rx_len > ACCESS_MAX_PAYLOAD) {
                    rx_reset();
                    access_send_nack(0, ACCESS_STATUS_BAD_LENGTH);
                    return ACCESS_STATUS_BAD_LENGTH;
                }

                rx_state = RX_READ_PAYLOAD;
                break;

            case RX_READ_PAYLOAD:
                if (!is_valid_payload_char(c)) {
                    rx_reset();
                    access_send_nack(0, ACCESS_STATUS_INVALID_ASCII);
                    return ACCESS_STATUS_INVALID_ASCII;
                }

                rx_payload[rx_index++] = c;
                rx_chk ^= (uint8_t)c;

                if (rx_index >= rx_len) {
                    rx_payload[rx_index] = '\0';
                    rx_state = RX_WAIT_CHK;
                }

                break;

            case RX_WAIT_CHK:
                if (rx_chk != (uint8_t)c) {
                    rx_reset();
                    access_send_nack(0, ACCESS_STATUS_BAD_CHECKSUM);
                    return ACCESS_STATUS_BAD_CHECKSUM;
                }

                rx_state = RX_WAIT_ETX;
                break;

            case RX_WAIT_ETX:
                if ((uint8_t)c != ACCESS_ETX) {
                    rx_reset();
                    access_send_nack(0, ACCESS_STATUS_BAD_FORMAT);
                    return ACCESS_STATUS_BAD_ETX;
                }

                rx_reset();

                {
                    access_status_t status = parse_payload(rx_payload, msg);

                    if (status != ACCESS_STATUS_OK) {
                        access_send_nack(msg->sid, status);
                    }

                    return status;
                }
        }
    }

    return ACCESS_STATUS_NO_DATA;
}