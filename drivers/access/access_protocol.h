#ifndef ACCESS_PROTOCOL_H
#define ACCESS_PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>

#define ACCESS_STX 0x02
#define ACCESS_ETX 0x03

#define ACCESS_MAX_PAYLOAD 48
#define ACCESS_MAX_DATA    24
#define ACCESS_TIMEOUT_MS  1000

typedef enum {
    ACCESS_STATUS_OK = 0,
    ACCESS_STATUS_NO_DATA,
    ACCESS_STATUS_BAD_STX,
    ACCESS_STATUS_BAD_ETX,
    ACCESS_STATUS_BAD_LENGTH,
    ACCESS_STATUS_BAD_CHECKSUM,
    ACCESS_STATUS_BAD_FORMAT,
    ACCESS_STATUS_TIMEOUT,
    ACCESS_STATUS_UNKNOWN_COMMAND,
    ACCESS_STATUS_INVALID_ASCII,
    ACCESS_STATUS_BUFFER_TOO_SMALL
} access_status_t;

typedef enum {
    ACCESS_CMD_UNKNOWN = 0,
    ACCESS_CMD_UID,
    ACCESS_CMD_REQ_PIN,
    ACCESS_CMD_PIN,
    ACCESS_CMD_OK,
    ACCESS_CMD_ERR,
    ACCESS_CMD_LOCKED,
    ACCESS_CMD_TIMEOUT,
    ACCESS_CMD_RESET,
    ACCESS_CMD_PING,
    ACCESS_CMD_PONG,
    ACCESS_CMD_NACK
} access_command_t;

typedef struct {
    uint16_t sid;
    access_command_t command;
    char data[ACCESS_MAX_DATA];
} access_message_t;

void access_init(void);

access_status_t access_send_uid(const char *uid);
access_status_t access_send_pin(uint16_t sid, const char *pin);
access_status_t access_send_ping(void);
access_status_t access_send_pong(void);
access_status_t access_send_reset(uint16_t sid);
access_status_t access_send_nack(uint16_t sid, access_status_t reason);

access_status_t access_read_message(access_message_t *msg);

#endif