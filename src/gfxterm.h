/* TinyOS V1 — Graphical Terminal on GOP Framebuffer
 *
 * Renders text directly onto the linear framebuffer using an embedded
 * 8x16 bitmap font.  Supports:
 *   - Character output with automatic scrolling
 *   - Cursor positioning
 *   - Color text (foreground/background)
 *   - Clear screen
 *   - Line input with echo
 */

#ifndef TINYOS_GFXTERM_H
#define TINYOS_GFXTERM_H

#include "uefi/types.h"

/* Terminal dimensions (in characters) — computed at init from screen size */
struct GfxTerm {
    int cols;       /* Characters per row */
    int rows;       /* Rows of text */
    int cur_x;      /* Cursor column (0-based) */
    int cur_y;      /* Cursor row (0-based) */
    UINT32 fg;      /* Foreground color (packed 32-bit: 0x00BBGGRR or 0x00RRGGBB depending on BGR) */
    UINT32 bg;      /* Background color */
};

extern GfxTerm g_term;

/* Initialize the graphical terminal. Must be called after GOP init.
 * Returns true on success. */
bool gfxterm_init();

/* Clear the screen to the background color and home cursor. */
void gfxterm_clear();

/* Scroll the terminal up by one line. */
void gfxterm_scroll();

/* Set cursor position (0-based). */
void gfxterm_set_cursor(int x, int y);

/* Print a single character. Handles \n and \r. */
void gfxterm_putc(char c);

/* Print a null-terminated string. */
void gfxterm_print(const char *str);

/* Print a decimal integer. */
void gfxterm_print_dec(UINT64 val);

/* Print a hex integer. */
void gfxterm_print_hex(UINT64 val);

/* Read a line of input with echo. Returns length excluding null.
 * max_len should be <= 256. */
int gfxterm_readline(char *buf, int max_len);

/* Draw one character glyph at (x, y) in character coordinates. */
void gfxterm_draw_char(int x, int y, char c, UINT32 fg, UINT32 bg);

#endif /* TINYOS_GFXTERM_H */
