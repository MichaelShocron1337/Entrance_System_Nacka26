#ifndef UART_H
#define UART_H

#include <stdint.h>
#include <stdbool.h>

void uart_init(uint32_t baudrate);
void uart_write_char(char c);
void uart_write_string(const char *str);
bool uart_read_char(char *c);
void uart_newline(void);
void uart_put_uint8(uint8_t value);
void uart_put_hex16(uint16_t value);
void uart_put_hex8(uint8_t value);
uint8_t uart_available(void);

#endif