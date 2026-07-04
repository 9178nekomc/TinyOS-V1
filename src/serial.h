/* TinyOS V1 — COM1 Serial Driver (16550 UART)
 *
 * COM1 is mapped at I/O port 0x3F8. This must be the FIRST thing
 * initialized in efi_main — before any UEFI API calls.
 *
 * Baud rate: 115200 (divisor = 1)
 * Format: 8 data bits, no parity, 1 stop bit (8N1)
 * FIFO: enabled
 */

#ifndef TINYOS_SERIAL_H
#define TINYOS_SERIAL_H

#include "uefi/types.h"

/* Initialize COM1 UART to 115200 8N1.
 * Must be called BEFORE any UEFI Boot Services calls. */
void serial_init();

/* Output a single character to COM1.
 * Blocks until the transmit FIFO has space. */
void serial_putc(char c);

/* Output a null-terminated string to COM1. */
void serial_print(const char *str);

/* Output a 64-bit value as hexadecimal to COM1. */
void serial_print_hex(UINT64 val);

/* Output a decimal UINT64 to COM1. */
void serial_print_dec(UINT64 val);

/* Check if a character is available on COM1. */
BOOLEAN serial_can_read();

/* Read a single character from COM1 (blocking). */
char serial_getc();

#endif /* TINYOS_SERIAL_H */
