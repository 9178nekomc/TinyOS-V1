/* TinyOS V1 — Console Input
 *
 * Abstracts keyboard input (via UEFI Simple Text Input Protocol)
 * and serial input fallback. Provides a line-editing readline()
 * for the shell REPL.
 *
 * Output always goes to both serial and the UEFI console.
 */

#ifndef TINYOS_CONSOLE_H
#define TINYOS_CONSOLE_H

#include "uefi/types.h"

/* Maximum command line length */
#define CONSOLE_MAX_LINE 256

/* Print a string to both serial and the UEFI text console. */
void console_print(const char *str);

/* Print a single character to both serial and UEFI console. */
void console_putc(char c);

/* Read a line of input with basic editing (backspace).
 * Returns the length of the line (excluding null terminator).
 * Blocks until Enter is pressed.
 *
 * Input source priority:
 *   1. UEFI keyboard (ST->ConIn) — if available
 *   2. Serial COM1 — fallback
 *
 * The line buffer receives len characters (no trailing newline)
 * followed by a null terminator. */
int console_readline(char *buf, int max_len);

#endif /* TINYOS_CONSOLE_H */
